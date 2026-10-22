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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Broken.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3FunctionTraits.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Number.h"
#अन्तर्भाव "V3StdFuture.h"

#अन्तर्भाव "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#अन्तर्भाव <array>
#अन्तर्भाव <cmath>
#अन्तर्भाव <cstdint>
#अन्तर्भाव <functional>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <type_traits>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#यदि defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#undef VL_USER_TYPE_CHECKS
#endif
#ifdef VL_USER_TYPE_CHECKS
#अन्तर्भाव <variant>
#endif

// clang-format off
#अन्तर्भाव "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3Graph;
class ExecMTask;
class VIfaceCaptureTag;

//######################################################################

// For broken() function, return error string if have a match
#define BROKEN_RTN(test) \
    गर { \
        यदि (VL_UNCOVERABLE(test)) फर्काऊ "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } जबसम्म (false)
// For broken() function, return error string if a base of this class has a match
#define BROKEN_BASE_RTN(test) \
    गर { \
        अचर वर्ण* अचर reasonp = (test); \
        यदि (VL_UNCOVERABLE(reasonp)) फर्काऊ reasonp; \
    } जबसम्म (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#define VN_IS(nodep, nodetypename) (Ast_गाँठो::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#define VN_CAST(nodep, nodetypename) (Ast_गाँठो::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#define VN_AS(nodep, nodetypename) (Ast_गाँठो::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#यदि defined(VL_DEBUG) || defined(VL_ASAN)
#define VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#अन्यथा
#define VN_DBG_AS(nodep, nodetypename) (Ast_गाँठो::unsafePrivateAs<Ast##nodetypename>(nodep))
#endif

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#define VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

संरचना VNTypeInfo final {
    अचर वर्ण* m_namep;
    गणना OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    अचर वर्ण* m_opNamep[4];
    माप_प्रकार m_sizeof;
};

class VNType final {
    स्थिर अचर VNTypeInfo s_typeInfoTable[];

सार्वजनिक:
#अन्तर्भाव "V3Ast__gen_type_enum.h"  // From ./astgen
    अचर en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr चालक en() अचर VL_MT_SAFE { फर्काऊ m_e; }

    // Retrieve VNTypeInfo for the given type
    स्थिर अचर VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { फर्काऊ s_typeInfoTable[t.m_e]; }
};
constexpr bool चालक==(अचर VNType& lhs, अचर VNType& rhs) VL_PURE {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VNType& lhs, VNType::en rhs) VL_PURE { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VNType::en lhs, अचर VNType& rhs) VL_PURE { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VNType& rhs) { फर्काऊ os << rhs.ascii(); }

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
    std::variant<std::monostate, uint64_t, शून्य*> m_u;
#अन्यथा
    सङ्घ {
        शून्य* up;
        uint64_t uq;
    } m_u;
#endif

सार्वजनिक:
#ifdef VL_USER_TYPE_CHECKS
    VNUser() = स्वभाव;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        यदि (i) m_u = i;
    }
    explicit VNUser(शून्य* p) {
        // VNUser{nullptr} represents the monostate
        यदि (p) m_u = p;
    }
    ~VNUser() = स्वभाव;
    // Casters
    ढाँचा <typename T>
    typename std::enable_if<std::is_pointer<T>::मान, T>::type to() अचर VL_MT_SAFE {
        यदि (std::holds_alternative<std::monostate>(m_u)) फर्काऊ nullptr;
        शून्य* अचर* अचर upp = std::get_if<शून्य*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        फर्काऊ reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() अचर {
        यदि (std::holds_alternative<std::monostate>(m_u)) फर्काऊ 0;
        अचर uint64_t* अचर uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        फर्काऊ *uip;
    }
#अन्यथा
    VNUser() = स्वभाव;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.up = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(शून्य* p) { m_u.up = p; }
    ~VNUser() = स्वभाव;
    // Casters
    ढाँचा <typename T>
    typename std::enable_if<std::is_pointer<T>::मान, T>::type to() अचर VL_MT_SAFE {
        फर्काऊ reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() अचर { फर्काऊ m_u.uq; }
#endif
    VSymEnt* toSymEnt() अचर { फर्काऊ to<VSymEnt*>(); }
    Ast_गाँठो* toNodep() अचर VL_MT_SAFE { फर्काऊ to<Ast_गाँठो*>(); }
    V3GraphVertex* toGraphVertex() अचर { फर्काऊ to<V3GraphVertex*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(अचर शून्य*)) अचर;
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
VL_NOT_FINAL{रक्षित : स्थिर शून्य allocate(पूर्णाङ्क id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvtToStr(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
स्थिर शून्य मुक्त(पूर्णाङ्क id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvtToStr(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
स्थिर शून्य clearcnt(पूर्णाङ्क id, uint32_t& cntGblRef, अचर bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
स्थिर शून्य checkcnt(पूर्णाङ्क id, uint32_t&, अचर bool& userBusyRef) {
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
रक्षित:
    मित्र class Ast_गाँठो;
    स्थिर uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    स्थिर bool s_userBusy;  // Count is in use
सार्वजनिक:
    VNUser1InUse()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1InUse()     { मुक्त    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2InUse final : VNUserInUseBase {
रक्षित:
    मित्र class Ast_गाँठो;
    स्थिर uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    स्थिर bool s_userBusy;  // Count is in use
सार्वजनिक:
    VNUser2InUse()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2InUse()     { मुक्त    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3InUse final : VNUserInUseBase {
रक्षित:
    मित्र class Ast_गाँठो;
    स्थिर uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    स्थिर bool s_userBusy;  // Count is in use
सार्वजनिक:
    VNUser3InUse()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3InUse()     { मुक्त    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4InUse final : VNUserInUseBase {
रक्षित:
    मित्र class Ast_गाँठो;
    स्थिर uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    स्थिर bool s_userBusy;  // Count is in use
सार्वजनिक:
    VNUser4InUse()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4InUse()     { मुक्त    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::vector<Ast_गाँठो*> m_deleteps;  // Nodes to delete

सार्वजनिक:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    शून्य थप_Deletep(Ast_गाँठो* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.थप_back(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    शून्य doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    मित्र class Ast_गाँठो;

सार्वजनिक:
    /// Call visit()s on nodep
    अन्तःस्थ शून्य iterateConst(Ast_गाँठो* nodep);
    /// Call visit()s on nodep
    अन्तःस्थ शून्य iterateConstNull(Ast_गाँठो* nodep);
    /// Call visit()s on const nodep's children
    अन्तःस्थ शून्य iterateChildrenConst(Ast_गाँठो* nodep);
    /// Call visit()s on nodep's children in backp() order
    अन्तःस्थ शून्य iterateChildrenBackwardsConst(Ast_गाँठो* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    अन्तःस्थ शून्य iterateAndNextConstNull(Ast_गाँठो* nodep);

    आभास शून्य visit(Ast_गाँठो* nodep) = 0;
    आभास ~VNVisitorConst() {}
#अन्तर्भाव "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : सार्वजनिक VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
सार्वजनिक:
    /// Call visit()s on nodep
    अन्तःस्थ शून्य iterate(Ast_गाँठो* nodep);
    /// Call visit()s on nodep
    अन्तःस्थ शून्य iterateNull(Ast_गाँठो* nodep);
    /// Call visit()s on nodep's children
    अन्तःस्थ शून्य iterateChildren(Ast_गाँठो* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    अन्तःस्थ शून्य iterateAndNextNull(Ast_गाँठो* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    अन्तःस्थ Ast_गाँठो* iterateSubtreeReturnEdits(Ast_गाँठो* nodep);

    VNDeleter& deleter() { फर्काऊ m_deleter; }
    शून्य थप_Deletep(Ast_गाँठो* nodep) { deleter().थप_Deletep(nodep); }
    शून्य doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
रक्षित:
    मित्र class Ast_गाँठो;
    गणना RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_NEXT,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_गाँठो* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_गाँठो* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_गाँठो** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

सार्वजनिक:
    VNRelinker() = स्वभाव;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    अन्तःस्थ शून्य relink(Ast_गाँठो* newp);
    शून्य dump(std::ostream& str = std::cout) अचर;
};
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VNRelinker& rhs) {
    rhs.dump(os);
    फर्काऊ os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{सार्वजनिक : आभास bool गाँठो_Match(अचर Ast_गाँठो* nodep) अचर {फर्काऊ true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#define ASTNODE_PREFETCH_NON_NULL(nodep) \
    गर { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } जबसम्म (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#define ASTNODE_PREFETCH(nodep) \
    गर { \
        यदि (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } जबसम्म (false)

class Ast_गाँठो VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_गाँठो* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_गाँठो* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_गाँठो* m_op1p = nullptr;  // Generic pointer 1
    Ast_गाँठो* m_op2p = nullptr;  // Generic pointer 2
    Ast_गाँठो* m_op3p = nullptr;  // Generic pointer 3
    Ast_गाँठो* m_op4p = nullptr;  // Generic pointer 4
    Ast_गाँठो** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    अचर VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    संरचना {
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

    पूर्णाङ्क m_cloneCnt = 0;  // Sequence number for when last clone was made

#यदि defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(परिमाण(m_type) + परिमाण(m_flags) + परिमाण(m_brokenState) + परिमाण(m_cloneCnt)
                      <= परिमाण(शून्य*),
                  "packing requires padding");
#endif

    Ast_गाँठो_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_गाँठो* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_पङ्क्ति* m_fileline;  // Where it was declared
#ifdef VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_गन्ती;  // When it was last edited
#endif
    स्थिर uint64_t s_editCntGbl;  // Global edit counter
    स्थिर uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_गाँठो* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    स्थिर पूर्णाङ्क s_cloneCntGbl;  // Count of which userp is set

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
    शून्य op1p(Ast_गाँठो* nodep) {
        m_op1p = nodep;
        यदि (nodep) nodep->m_backp = this;
    }
    शून्य op2p(Ast_गाँठो* nodep) {
        m_op2p = nodep;
        यदि (nodep) nodep->m_backp = this;
    }
    शून्य op3p(Ast_गाँठो* nodep) {
        m_op3p = nodep;
        यदि (nodep) nodep->m_backp = this;
    }
    शून्य op4p(Ast_गाँठो* nodep) {
        m_op4p = nodep;
        यदि (nodep) nodep->m_backp = this;
    }

गुप्त:
    Ast_गाँठो* clone_रूख_Iter(bool needPure);
    Ast_गाँठो* clone_रूख_Iter_List(bool needPure);
    शून्य check_रूख_Iter(अचर Ast_गाँठो* prevBackp) अचर VL_MT_STABLE;
    bool gate_रूख_Iter() अचर;
    स्थिर bool same_रूख_Iter(अचर Ast_गाँठो* node1p, अचर Ast_गाँठो* node2p, bool ignNext,
                             bool gateOnly);
    शून्य delete_रूख_Iter();
    शून्य delete_गाँठो();
    string instanceStr() अचर;

सार्वजनिक:
    शून्य purityCheck();
    स्थिर शून्य relinkOneLink(Ast_गाँठो*& pointpr, Ast_गाँठो* newp);
    // cppcheck-suppress functionConst
    स्थिर शून्य debug_रूख_Change(अचर Ast_गाँठो* nodep, अचर वर्ण* prefix, पूर्णाङ्क lineno, bool next);

रक्षित:
    // CONSTRUCTORS
    Ast_गाँठो(VNType t, File_पङ्क्ति* fl);
    आभास ~Ast_गाँठो() = स्वभाव;  // Use 'deleteTree' instead
    आभास Ast_गाँठो* clone() = 0;  // Generally, cloneTree is what you want instead
    आभास शून्य cloneRelink() { cloneRelinkGen(); }
    आभास शून्य cloneRelinkGen() {};  // Overrides generated by 'astgen'
    शून्य clone_Relink_रूख();

    // METHODS
    शून्य setOp1p(Ast_गाँठो* newp);  // Set non-list-type op1 to non-list element
    शून्य setOp2p(Ast_गाँठो* newp);  // Set non-list-type op2 to non-list element
    शून्य setOp3p(Ast_गाँठो* newp);  // Set non-list-type op3 to non-list element
    शून्य setOp4p(Ast_गाँठो* newp);  // Set non-list-type op4 to non-list element

    शून्य addOp1p(Ast_गाँठो* newp);  // Append newp to end of op1
    शून्य addOp2p(Ast_गाँठो* newp);  // Append newp to end of op2
    शून्य addOp3p(Ast_गाँठो* newp);  // Append newp to end of op3
    शून्य addOp4p(Ast_गाँठो* newp);  // Append newp to end of op4

    शून्य prependOp1p(Ast_गाँठो* newp);  // Prepend newp to front of op1
    शून्य prependOp2p(Ast_गाँठो* newp);  // Prepend newp to front of op2
    शून्य prependOp3p(Ast_गाँठो* newp);  // Prepend newp to front of op3
    शून्य prependOp4p(Ast_गाँठो* newp);  // Prepend newp to front of op4

    // clang-format off
    शून्य setNOp1p(Ast_गाँठो* newp) { यदि (newp) setOp1p(newp); }
    शून्य setNOp2p(Ast_गाँठो* newp) { यदि (newp) setOp2p(newp); }
    शून्य setNOp3p(Ast_गाँठो* newp) { यदि (newp) setOp3p(newp); }
    शून्य setNOp4p(Ast_गाँठो* newp) { यदि (newp) setOp4p(newp); }

    शून्य addNOp1p(Ast_गाँठो* newp) { यदि (newp) addOp1p(newp); }
    शून्य addNOp2p(Ast_गाँठो* newp) { यदि (newp) addOp2p(newp); }
    शून्य addNOp3p(Ast_गाँठो* newp) { यदि (newp) addOp3p(newp); }
    शून्य addNOp4p(Ast_गाँठो* newp) { यदि (newp) addOp4p(newp); }

    शून्य prependNOp1p(Ast_गाँठो* newp) { यदि (newp) prependOp1p(newp); }
    शून्य prependNOp2p(Ast_गाँठो* newp) { यदि (newp) prependOp2p(newp); }
    शून्य prependNOp3p(Ast_गाँठो* newp) { यदि (newp) prependOp3p(newp); }
    शून्य prependNOp4p(Ast_गाँठो* newp) { यदि (newp) prependOp4p(newp); }
    // clang-format on

    शून्य clonep(Ast_गाँठो* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    स्थिर शून्य clone_Clear_रूख() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    आभास bool same_गाँठो(अचर Ast_गाँठो*) अचर { फर्काऊ true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    आभास bool wouldBreakGen(अचर Ast_गाँठो* अचर oldp,
                               अचर Ast_गाँठो* अचर newp) अचर
        = 0;  // Generated by 'astgen'

सार्वजनिक:
    // ACCESSORS
    VNType type() अचर VL_MT_SAFE { फर्काऊ m_type; }
    अचर वर्ण* typeName() अचर VL_MT_SAFE { फर्काऊ type().ascii(); }  // See also prettyTypeName
    Ast_गाँठो* nextp() अचर VL_MT_STABLE { फर्काऊ m_nextp; }
    Ast_गाँठो* backp() अचर VL_MT_STABLE { फर्काऊ m_backp; }
    Ast_गाँठो* abovep() अचर;  // Get parent node above, only for list head and tail
    Ast_गाँठो* aboveLoopp() अचर;  // Get parent node above, may have performance issues as loops
    Ast_गाँठो* lastp() अचर {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        फर्काऊ m_headtailp;
    }
    Ast_गाँठो* prevp() अचर {  // Previous node in list, nullptr for head of list
        यदि (m_backp->m_nextp != this) फर्काऊ nullptr;
        फर्काऊ m_backp;
    }
    Ast_गाँठो* op1p() अचर VL_MT_STABLE { फर्काऊ m_op1p; }
    Ast_गाँठो* op2p() अचर VL_MT_STABLE { फर्काऊ m_op2p; }
    Ast_गाँठो* op3p() अचर VL_MT_STABLE { फर्काऊ m_op3p; }
    Ast_गाँठो* op4p() अचर VL_MT_STABLE { फर्काऊ m_op4p; }
    Ast_गाँठो_DType* dtypep() अचर VL_MT_STABLE { फर्काऊ m_dtypep; }
    Ast_गाँठो* clonep() अचर { फर्काऊ ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_गाँठो* firstAbovep() अचर {  // Returns nullptr when second or later in list
        फर्काऊ ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() अचर VL_MT_SAFE { फर्काऊ m_brokenState; }
    शून्य brokenState(uint8_t मान) { m_brokenState = मान; }

    // Used by AstNode::broken()
    bool brokeExists() अचर { फर्काऊ V3Broken::isLinkable(this); }
    bool brokeExistsAbove() अचर { फर्काऊ brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    आभास शून्य foreachLink(std::function<शून्य(Ast_गाँठो** linkpp, अचर वर्ण* namep)> f) = 0;

#यदि defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    स्थिर शून्य* चालक new(माप_प्रकार size);
    स्थिर शून्य चालक delete(शून्य* obj, माप_प्रकार size);
#endif

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_BRANCH = 4;  // Branch
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_CALL = INSTR_गन्ती_BRANCH + 10;  // Subroutine call
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_LD = 2;  // Load memory
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_INT_MUL = 3;  // Integer multiply
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_INT_DIV = 10;  // Integer divide
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_DBL = 8;  // Convert or do float ops
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_DBL_DIV = 40;  // Double divide
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_DBL_TRIG = 200;  // Double trigonometric ops
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_STR = 100;  // String ops
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_TIME = INSTR_गन्ती_CALL + 5;  // Determine simulation time
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_PLI = 20;  // PLI routines
    स्थिर constexpr पूर्णाङ्क INSTR_गन्ती_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    आभास string name() अचर VL_MT_STABLE { फर्काऊ ""; }
    आभास string origName() अचर { फर्काऊ ""; }
    string prettyDehashOrigOrName() अचर {
        फर्काऊ prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    आभास शून्य name(अचर string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    आभास शून्य tag(अचर string& text) {}
    आभास string tag() अचर { फर्काऊ ""; }
    आभास uint32_t decl_Token_अङ्क() अचर { फर्काऊ 0; }
    आभास शून्य decl_Token_अङ्क_Set_Min(uint32_t token_अङ्क) {}
    आभास string verilogKwd() अचर { फर्काऊ ""; }
    string nameProtect() अचर VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() अचर;  // origName with --protect-id applied
    string shortName() अचर;  // Name with __PVT__ removed for concatenating scopes
    स्थिर string dedotName(अचर string& namein);  // Name with dots removed
    स्थिर string nameNoArray(अचर string& namein);  // Name with any array index removed
    स्थिर string prettyName(अचर string& namein) VL_PURE;  // Name for printing out to the user
    स्थिर string vpiName(अचर string& namein);  // Name for vpi access
    स्थिर string prettyNameQ(अचर string& namein) {  // Quoted pretty name (for errors)
        फर्काऊ "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    स्थिर string encodeName(अचर string& namein);
    स्थिर string encode_सङ्ख्या(int64_t अङ्क);  // Encode number into internal C representation
    स्थिर string vcdName(अचर string& namein);  // Name for printing out to vcd files
    string prettyName() अचर { फर्काऊ prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    आभास string pretty_Name_सन्देश() अचर { फर्काऊ prettyName(); }
    string prettyNameQ() अचर { फर्काऊ "'"s + pretty_Name_सन्देश() + "'"; }  // Quoted, for messages
    string verilogName() अचर { फर्काऊ vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() अचर;
    आभास string prettyOperatorName() अचर { फर्काऊ "operator " + prettyTypeName(); }
    File_पङ्क्ति* fileline() अचर VL_MT_SAFE { फर्काऊ m_fileline; }
    शून्य fileline(File_पङ्क्ति* fl) { m_fileline = fl; }
    अन्तःस्थ bool width1() अचर;
    अन्तःस्थ पूर्णाङ्क widthInstrs() अचर;
    bool didWidth() अचर { फर्काऊ m_flags.didWidth; }
    शून्य didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        यदि (didWidth()) फर्काऊ true;
        didWidth(true);
        फर्काऊ false;
    }
    bool doingWidth() अचर { फर्काऊ m_flags.doingWidth; }
    शून्य doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() अचर VL_MT_SAFE { फर्काऊ m_flags.protect; }
    शून्य protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    अन्तःस्थ पूर्णाङ्क width() अचर VL_MT_STABLE;
    अन्तःस्थ पूर्णाङ्क widthMin() अचर VL_MT_STABLE;
    पूर्णाङ्क widthMinV() अचर {
        फर्काऊ v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    पूर्णाङ्क width_शब्दहरू() अचर { फर्काऊ VL_शब्दहरू_I(width()); }
    bool isQuad() अचर VL_MT_STABLE { फर्काऊ (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() अचर VL_MT_STABLE { फर्काऊ (width() > VL_QUADSIZE); }
    अन्तःस्थ bool isCHandle() अचर VL_MT_STABLE;
    अन्तःस्थ bool isDouble() अचर VL_MT_STABLE;
    अन्तःस्थ bool isSigned() अचर VL_MT_STABLE;
    अन्तःस्थ bool isString() अचर VL_MT_STABLE;
    अन्तःस्थ bool isEvent() अचर VL_MT_STABLE;

    // clang-format off
    VNUser user1u() अचर VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        फर्काऊ ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_गाँठो* user1p() अचर VL_MT_STABLE { फर्काऊ user1u().toNodep(); }
    शून्य user1u(अचर VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    शून्य user1p(शून्य* userp) { user1u(VNUser{userp}); }
    शून्य user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() अचर { फर्काऊ user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { अचर uint64_t v = user1(); user1(v + val); फर्काऊ v; }
    uint64_t user1Or(uint64_t val) { अचर uint64_t v = user1(); user1(v | val); फर्काऊ v; }
    uint64_t user1SetOnce() { अचर uint64_t v = user1(); यदि (!v) user1(1); फर्काऊ v; }  // Better for cache than user1Inc()
    स्थिर शून्य user1_Clear_रूख() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() अचर VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        फर्काऊ ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_गाँठो* user2p() अचर VL_MT_STABLE { फर्काऊ user2u().toNodep(); }
    शून्य user2u(अचर VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    शून्य user2p(शून्य* userp) { user2u(VNUser{userp}); }
    शून्य user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() अचर { फर्काऊ user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { अचर uint64_t v = user2(); user2(v + val); फर्काऊ v; }
    uint64_t user2Or(uint64_t val) { अचर uint64_t v = user2(); user2(v | val); फर्काऊ v; }
    uint64_t user2SetOnce() { अचर uint64_t v = user2(); यदि (!v) user2(1); फर्काऊ v; }  // Better for cache than user2Inc()
    स्थिर शून्य user2_Clear_रूख() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() अचर VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        फर्काऊ ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_गाँठो* user3p() अचर VL_MT_STABLE { फर्काऊ user3u().toNodep(); }
    शून्य user3u(अचर VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    शून्य user3p(शून्य* userp) { user3u(VNUser{userp}); }
    शून्य user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() अचर { फर्काऊ user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { अचर uint64_t v = user3(); user3(v + val); फर्काऊ v; }
    uint64_t user3Or(uint64_t val) { अचर uint64_t v = user3(); user3(v | val); फर्काऊ v; }
    uint64_t user3SetOnce() { अचर uint64_t v = user3(); यदि (!v) user3(1); फर्काऊ v; }  // Better for cache than user3Inc()
    स्थिर शून्य user3_Clear_रूख() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() अचर VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        फर्काऊ ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_गाँठो* user4p() अचर VL_MT_STABLE { फर्काऊ user4u().toNodep(); }
    शून्य user4u(अचर VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    शून्य user4p(शून्य* userp) { user4u(VNUser{userp}); }
    शून्य user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() अचर { फर्काऊ user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { अचर uint64_t v = user4(); user4(v | val); फर्काऊ v; }
    uint64_t user4Inc(uint64_t val = 1) { अचर uint64_t v = user4(); user4(v + val); फर्काऊ v; }
    uint64_t user4SetOnce() { अचर uint64_t v = user4(); यदि (!v) user4(1); फर्काऊ v; }  // Better for cache than user4Inc()
    स्थिर शून्य user4_Clear_रूख() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ifdef VL_DEBUG
    uint64_t edit_गन्ती() अचर { फर्काऊ m_edit_गन्ती; }
    शून्य edit_गन्ती_Inc() {
        m_edit_गन्ती = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_रूख();  // Any edit clears all caching
    }
#अन्यथा
    शून्य edit_गन्ती_Inc() { ++s_editCntGbl; }
#endif
    स्थिर uint64_t edit_गन्ती_Last() VL_MT_SAFE { फर्काऊ s_editCntLast; }
    स्थिर uint64_t edit_गन्ती_Gbl() VL_MT_SAFE { फर्काऊ s_editCntGbl; }
    स्थिर शून्य edit_गन्ती_Set_Last() { s_editCntLast = edit_गन्ती_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_थप_Self_Stmt();
    अन्तःस्थ bool is_Class_Handle_मान() अचर;
    अन्तःस्थ bool isNull() अचर;
    अन्तःस्थ bool isZero() अचर;
    अन्तःस्थ bool isOne() अचर;
    अन्तःस्थ bool isNeqZero() अचर;
    अन्तःस्थ bool isAllOnes() अचर;
    अन्तःस्थ bool isAllOnesV() अचर;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    शून्य dtypep(Ast_गाँठो_DType* nodep) {
        यदि (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_गन्ती_Inc();
        }
    }
    शून्य dtypeFrom(अचर Ast_गाँठो* fromp) {
        यदि (fromp) dtypep(fromp->dtypep());
    }
    शून्य dtypeChgSigned(bool flag = true);
    शून्य dtypeChgWidth(पूर्णाङ्क width, पूर्णाङ्क widthMin);
    शून्य dtypeChgWidthSigned(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric);
    शून्य dtypeSetBitUnsized(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) {
        dtypep(findBitDType(width, widthMin, numeric));
    }
    शून्य dtypeSetBitSized(पूर्णाङ्क width, VSigning numeric) {
        dtypep(findBitDType(width, width, numeric));  // Since sized, widthMin is width
    }
    शून्य dtypeSetLogicUnsized(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) {
        dtypep(findLogicDType(width, widthMin, numeric));
    }
    शून्य dtypeSetLogicSized(पूर्णाङ्क width, VSigning numeric) {
        dtypep(findLogicDType(width, width, numeric));  // Since sized, widthMin is width
    }
    शून्य dtypeSetLogic2StateUnsized(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) {
        dtypep(findLogic2StateDType(width, widthMin, numeric));
    }
    शून्य dtypeSetLogic2StateSized(पूर्णाङ्क width, VSigning numeric) {
        dtypep(findLogic2StateDType(width, width, numeric));  // Since sized, widthMin is width
    }
    शून्य dtypeSetBitOrLogicUnsized(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, widthMin, numeric, isFourstate));
    }
    शून्य dtypeSetBitOrLogicSized(पूर्णाङ्क width, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, width, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    शून्य dtypeSetBit() { dtypep(findBitDType()); }
    शून्य dtypeSetLogic() { dtypep(findLogicDType()); }
    शून्य dtypeSetLogic2State() { dtypep(findLogic2StateDType()); }
    शून्य dtypeSetDouble() { dtypep(findDoubleDType()); }
    शून्य dtypeSetInt() { dtypep(findIntDType()); }
    शून्य dtypeSetInteger() { dtypep(findIntegerDType()); }
    शून्य dtypeSetInteger2State() { dtypep(findInteger2StateDType()); }
    शून्य dtypeSetString() { dtypep(findStringDType()); }
    शून्य dtypeSetUInt32() { dtypep(findUInt32DType()); }  // Twostate
    शून्य dtypeSetUInt64() { dtypep(findUInt64DType()); }  // Twostate
    शून्य dtypeSetTime() { dtypep(findTimeDType()); }
    शून्य dtypeSetTime2State() { dtypep(findTime2StateDType()); }
    शून्य dtypeSetEmptyQueue() { dtypep(findEmptyQueueDType()); }
    शून्य dtypeSetStream() { dtypep(findStreamDType()); }
    शून्य dtypeSetVoid() { dtypep(findVoidDType()); }
    शून्य dtypeSetBitOrLogic(bool isFourstate) {
        dtypep(isFourstate ? findLogicDType() : findBitDType());
    }

    // Data type locators
    Ast_गाँठो_DType* findBitDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_गाँठो_DType* findLogicDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_गाँठो_DType* findNullDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_गाँठो_DType* findLogic2StateDType() अचर {
        फर्काऊ findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_गाँठो_DType* findDoubleDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_गाँठो_DType* findIntDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::INT); }
    Ast_गाँठो_DType* findIntegerDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_गाँठो_DType* findInteger2StateDType() अचर {
        फर्काऊ findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_गाँठो_DType* findStringDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_गाँठो_DType* findSigned8DType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_गाँठो_DType* findUInt32DType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_गाँठो_DType* findUInt64DType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_गाँठो_DType* findTimeDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_गाँठो_DType* findTime2StateDType() अचर {
        फर्काऊ findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_गाँठो_DType* findCHandleDType() अचर { फर्काऊ findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_गाँठो_DType* findConstraintRefDType() अचर;
    Ast_गाँठो_DType* findEmptyQueueDType() अचर;
    Ast_गाँठो_DType* findQueueIndexDType() अचर;
    Ast_गाँठो_DType* findStreamDType() अचर;
    Ast_गाँठो_DType* findVoidDType() अचर;
    Ast_गाँठो_DType* findBitDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) अचर;
    Ast_गाँठो_DType* findLogicDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) अचर;
    Ast_गाँठो_DType* findLogic2StateDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) अचर;
    Ast_गाँठो_DType* findBitOrLogicDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric,
                                      bool isFourstate) अचर {
        फर्काऊ isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_गाँठो_DType* findBitOrLogicRangeDType(अचर VNumRange& range, अचर पूर्णाङ्क widthMin,
                                           अचर VSigning numeric,
                                           अचर bool isFourstate) अचर VL_MT_STABLE {
        फर्काऊ isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_गाँठो_DType* findLogicRangeDType(अचर VNumRange& range, पूर्णाङ्क widthMin,
                                      VSigning numeric) अचर VL_MT_STABLE;
    Ast_गाँठो_DType* findBitRangeDType(अचर VNumRange& range, पूर्णाङ्क widthMin,
                                    VSigning numeric) अचर VL_MT_STABLE;
    Ast_गाँठो_DType* findBasicDType(VBasicDTypeKwd kwd) अचर;
    स्थिर AstBasicDType* find_घुसाउ_Same_DType(AstBasicDType* nodep);

    स्थिर VCastable computeCastable(अचर Ast_गाँठो_DType* toDtp, अचर Ast_गाँठो_DType* fromDtp,
                                     अचर Ast_गाँठो* fromConstp, bool checkIfaceArgCompat = false);
    स्थिर Ast_गाँठो_DType* getCommonClassTypep(Ast_गाँठो* nodep1, Ast_गाँठो* nodep2);

    // METHODS - dump and error
    शून्य v3errorEnd(अचर std::ostringstream& str) अचर VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(अचर std::ostringstream& str) अचर VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() अचर VL_REQUIRES(V3Error::s().m_mutex) {
        फर्काऊ fileline()->warnContextPrimary();
    }
    string warnContextSecondary() अचर { फर्काऊ fileline()->warnContextSecondary(); }
    string warnMore() अचर VL_REQUIRES(V3Error::s().m_mutex) { फर्काऊ fileline()->warnMore(); }
    string warnOther() अचर VL_REQUIRES(V3Error::s().m_mutex) { फर्काऊ fileline()->warnOther(); }

    आभास शून्य dump(std::ostream& str = std::cout) अचर;
    स्थिर वर्ण* dump_रूख_Json_Gdb(अचर Ast_गाँठो* nodep);  // For GDB only, free()d by caller
    स्थिर वर्ण* dump_रूख_Json_Gdb(अचर वर्ण* str);  // For GDB only, free()d by caller
    स्थिर वर्ण* dump_रूख_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    स्थिर शून्य dumpGdb(अचर Ast_गाँठो* nodep);  // For GDB only
    शून्य dumpGdbHeader() अचर;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    ढाँचा <typename T_गाँठो_Result, typename T_गाँठो_Next>
    स्थिर T_गाँठो_Result* addNext(T_गाँठो_Result* nodep, T_गाँठो_Next* newp) {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो_Result>::मान,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_गाँठो_Result, T_गाँठो_Next>::मान,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        यदि (!newp) फर्काऊ nodep;
        फर्काऊ static_cast<T_गाँठो_Result*>(addNext<Ast_गाँठो, Ast_गाँठो>(nodep, newp));
    }
    ढाँचा <typename T_गाँठो_Result, typename T_गाँठो_Next>
    स्थिर T_गाँठो_Result* addNextNull(T_गाँठो_Result* nodep, T_गाँठो_Next* newp) {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो_Result>::मान,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_गाँठो_Result, T_गाँठो_Next>::मान,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        यदि (!newp) फर्काऊ nodep;
        यदि (!nodep) फर्काऊ newp;
        फर्काऊ static_cast<T_गाँठो_Result*>(addNext<Ast_गाँठो, Ast_गाँठो>(nodep, newp));
    }
    अन्तःस्थ Ast_गाँठो* addNext(Ast_गाँठो* newp);
    शून्य addNextHere(Ast_गाँठो* newp);  // Insert newp at this->nextp
    शून्य addHereThisAsNext(Ast_गाँठो* newp);  // Adds at old place of this, this becomes next
    शून्य replaceWith(Ast_गाँठो* newp);  // Replace current node in tree with new node
    शून्य replaceWithKeepDType(Ast_गाँठो* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_गाँठो* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_गाँठो* unlinkFrBackWithNext(VNRelinker* linkerp = nullptr);
    शून्य relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    शून्य clone_Relink_गाँठो() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_गाँठो* clone_रूख(bool cloneNextLink,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_गाँठो* clone_रूख_Pure(bool cloneNextLink) { फर्काऊ clone_रूख(cloneNextLink, true); }
    bool gate_रूख() { फर्काऊ gate_रूख_Iter(); }  // Is tree isGateOptimizable?
    अन्तःस्थ bool same_रूख(अचर Ast_गाँठो* node2p) अचर;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    अन्तःस्थ bool same_Gate_रूख(अचर Ast_गाँठो* node2p) अचर;
    शून्य delete_रूख();  // Always deletes the next link
    शून्य check_रूख() अचर VL_MT_STABLE {
        यदि (v3Global.opt.debugCheck()) check_रूख_Iter(backp());
    }
    शून्य checkIter() अचर;
    शून्य dumpPtrs(std::ostream& os = std::cout) अचर;
    शून्य dump_रूख(std::ostream& os = std::cout, अचर string& indent = "    ",
                  पूर्णाङ्क maxDepth = 0) अचर;
    शून्य dump_रूख(अचर string& indent, पूर्णाङ्क maxDepth = 0) अचर {
        dump_रूख(std::cout, indent, maxDepth);
    }
    स्थिर शून्य dump_रूख_Gdb(अचर Ast_गाँठो* nodep);  // For GDB only
    शून्य dump_रूख_And_Next(std::ostream& os = std::cout, अचर string& indent = "    ",
                         पूर्णाङ्क maxDepth = 0) अचर;
    शून्य dump_रूख_File(अचर string& filename, bool doDump = true);
    स्थिर शून्य dump_रूख_File_Gdb(अचर Ast_गाँठो* nodep, अचर वर्ण* filenamep = nullptr);
    शून्य dump_रूख_Dot(std::ostream& os = std::cout) अचर;
    शून्य dump_रूख_Dot_File(अचर string& filename, bool doDump = true);
    आभास शून्य dumpJson(std::ostream& os) अचर { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    शून्य dumpJsonGen(std::ostream& os) अचर {};
    आभास शून्य dump_रूख_Json_Op_Gen(std::ostream& os, अचर string& indent) अचर {};
    शून्य dump_रूख_Json(std::ostream& os, अचर string& indent = "") अचर;
    शून्य dump_रूख_Json_File(अचर string& filename, bool doDump = true);
    स्थिर शून्य dumpJsonMetaFileGdb(अचर वर्ण* filename);
    स्थिर शून्य dumpJsonMetaFile(अचर string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    स्थिर std::string गाँठो_Addr(अचर Ast_गाँठो* nodep) {
        फर्काऊ v3Global.opt.dump_रूख_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    स्थिर Ast_गाँठो* afterCommentp(Ast_गाँठो* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        जबसम्म (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        फर्काऊ nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    आभास bool isBrancher() अचर { फर्काऊ false; }
    // Else a AstTime etc that can't be pushed out
    आभास bool isGateOptimizable() अचर { फर्काऊ !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    आभास bool isGateDedupable() अचर { फर्काऊ isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    आभास bool is_अभिव्यक्ति_Coverage_Eligible() अचर { फर्काऊ isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    आभास bool isOutputter() { फर्काऊ false; }
    // Else a AstTime etc which output can't be predicted from input
    आभास bool isPredictOptimizable() अचर { फर्काऊ !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    आभास bool isPure() { फर्काऊ true; }
    // Iff isPure on current node and any nextp()
    bool isPureAndNext() { फर्काऊ isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    आभास bool isTimingControl() अचर { फर्काऊ false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    आभास bool isUnlikely() अचर { फर्काऊ false; }
    // Is an IEEE system function (versus internally-generated)
    आभास bool isSystemFunc() अचर { फर्काऊ false; }
    आभास पूर्णाङ्क instr_गन्ती() अचर { फर्काऊ 0; }
    // Iff node is identical to another node
    आभास bool isSame(अचर Ast_गाँठो* samep) अचर {
        फर्काऊ type() == samep->type() && same_गाँठो(samep);
    }
    // Iff has a data type; dtype() must be non-null
    आभास bool hasDType() अचर VL_MT_SAFE { फर्काऊ false; }
    // Iff has a non-null childDTypep(), as generic node function
    आभास Ast_गाँठो_DType* getChildDTypep() अचर { फर्काऊ nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    आभास Ast_गाँठो_DType* getChild2DTypep() अचर { फर्काऊ nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    आभास bool maybePointedTo() अचर VL_MT_SAFE { फर्काऊ false; }
    // Don't reclaim this node in V3Dead
    आभास bool undead() अचर { फर्काऊ false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    आभास अचर वर्ण* broken() अचर { फर्काऊ nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    आभास अचर वर्ण* brokenGen() अचर = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(अचर Ast_गाँठो* अचर newp) अचर { फर्काऊ backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    आभास शून्य accept(VNVisitorConst& v) = 0;

रक्षित:
    // All VNVisitor related functions are called as methods off the visitor
    मित्र class VNVisitor;
    मित्र class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    शून्य iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    शून्य iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    शून्य iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    शून्य iterateAndNext(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    शून्य iterateAndNextConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_गाँठो* iterateSubtreeReturnEdits(VNVisitor& v);

    स्थिर शून्य dump_Json_अङ्क(std::ostream& os, अचर std::string& name, int64_t val);
    स्थिर शून्य dumpJsonBoolIf(std::ostream& os, अचर std::string& name, bool val);
    स्थिर शून्य dumpJsonStr(std::ostream& os, अचर std::string& name, अचर std::string& val);
    स्थिर शून्य dumpJsonPtr(std::ostream& os, अचर std::string& name, अचर Ast_गाँठो* अचर valp);

रक्षित:
    शून्य iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    ढाँचा <typename T>
    अन्तःस्थ स्थिर bool privateTypeTest(अचर Ast_गाँठो* nodep);

    // For internal use only.
    ढाँचा <typename T_TargetType, typename T_DeclType>
    constexpr स्थिर bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        फर्काऊ std::is_base_of<T_TargetType, NonCV>::मान;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    ढाँचा <typename T_TargetType, typename T_DeclType>
    constexpr स्थिर bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        फर्काऊ !std::is_base_of<NonCV, T_TargetType>::मान;
    }  // LCOV_EXCL_STOP

सार्वजनिक:
    // For use via the VN_IS macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाँठो>
    स्थिर bool is(अचर T_गाँठो* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        फर्काऊ nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाँठो>
    स्थिर T_TargetType* cast(T_गाँठो* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        फर्काऊ nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    ढाँचा <typename T_TargetType, typename T_गाँठो>
    स्थिर अचर T_TargetType* cast(अचर T_गाँठो* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        फर्काऊ nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<अचर T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाँठो>
    स्थिर T_TargetType* as(T_गाँठो* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        फर्काऊ unsafePrivateAs<T_TargetType, T_गाँठो>(nodep);
    }
    ढाँचा <typename T_TargetType, typename T_गाँठो>
    स्थिर अचर T_TargetType* as(अचर T_गाँठो* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        फर्काऊ unsafePrivateAs<T_TargetType, T_गाँठो>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    ढाँचा <typename T_TargetType, typename T_गाँठो>
    स्थिर T_TargetType* unsafePrivateAs(T_गाँठो* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        फर्काऊ reinterpret_cast<T_TargetType*>(nodep);
    }
    ढाँचा <typename T_TargetType, typename T_गाँठो>
    स्थिर अचर T_TargetType* unsafePrivateAs(अचर T_गाँठो* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठो>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        फर्काऊ reinterpret_cast<अचर T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ढाँचा <typename T_गाँठो>
    स्थिर bool mayBeUnder(अचर Ast_गाँठो* nodep) {
        static_assert(!std::is_const<T_गाँठो>::मान,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        फर्काऊ true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ढाँचा <typename T_गाँठो>
    स्थिर constexpr bool isLeaf() {
        static_assert(!std::is_const<T_गाँठो>::मान,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        फर्काऊ false;
    }

गुप्त:
    // Using std::conditional for const correctness in the public 'foreach' functions
    ढाँचा <typename T_Arg>
    using Const_Correct_Ast_गाँठो =
        typename std::conditional<std::is_const<T_Arg>::मान, अचर Ast_गाँठो, Ast_गाँठो>::type;

    ढाँचा <typename T_Arg, typename T_Callable>
    अन्तःस्थ स्थिर शून्य foreachImpl(Const_Correct_Ast_गाँठो<T_Arg>* nodep, अचर T_Callable& f,
                                   bool visitNext);

    ढाँचा <typename T_Arg, bool N_Default, typename T_Callable>
    अन्तःस्थ स्थिर bool predicateImpl(Const_Correct_Ast_गाँठो<T_Arg>* nodep, अचर T_Callable& p,
                                     bool visitNext);

सार्वजनिक:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    ढाँचा <typename T_Callable>
    शून्य foreach(T_Callable&& f) {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_गाँठो*>::मान
                          && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_गाँठो>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    शून्य foreach(T_Callable&& f) अचर {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, अचर T_गाँठो*>::मान
                && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<अचर T_गाँठो>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    ढाँचा <typename T_Callable>
    शून्य foreachAndNext(T_Callable&& f) {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_गाँठो*>::मान
                          && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_गाँठो>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    शून्य foreachAndNext(T_Callable&& f) अचर {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, अचर T_गाँठो*>::मान
                && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<अचर T_गाँठो>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    ढाँचा <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_गाँठो*>::मान
                          && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        फर्काऊ predicateImpl<T_गाँठो, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool exists(T_Callable&& p) अचर {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, अचर T_गाँठो*>::मान
                          && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        फर्काऊ predicateImpl<अचर T_गाँठो, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    ढाँचा <typename T_Callable>
    bool existsAndNext(T_Callable&& p) {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, अचर T_गाँठो*>::मान
                          && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        फर्काऊ predicateImpl<T_गाँठो, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool existsAndNext(T_Callable&& p) अचर {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, अचर T_गाँठो*>::मान
                          && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        फर्काऊ predicateImpl<अचर T_गाँठो, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    ढाँचा <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_गाँठो*>::मान
                          && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        फर्काऊ predicateImpl<T_गाँठो, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool forall(T_Callable&& p) अचर {
        using T_गाँठो = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, अचर T_गाँठो*>::मान
                          && std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        फर्काऊ predicateImpl<अचर T_गाँठो, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    पूर्णाङ्क गाँठो_गन्ती() अचर {
        // TODO: this should really return size_t, but need to fix use sites
        पूर्णाङ्क गन्ती = 0;
        this->foreach([&गन्ती](अचर Ast_गाँठो*) { ++गन्ती; });
        फर्काऊ गन्ती;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(पूर्णाङ्क limit) अचर {
        पूर्णाङ्क गन्ती = 0;
        फर्काऊ this->exists([&गन्ती, limit](अचर Ast_गाँठो*) { फर्काऊ ++गन्ती > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
ढाँचा <>
Ast_गाँठो* Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(Ast_गाँठो* nodep, Ast_गाँठो* newp);

// Inline method implementations
Ast_गाँठो* Ast_गाँठो::addNext(Ast_गाँठो* newp) { फर्काऊ addNext(this, newp); }

// Specializations of privateTypeTest
#अन्तर्भाव "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
ढाँचा <>
अन्तःस्थ bool Ast_गाँठो::mayBeUnder<AstCell>(अचर Ast_गाँठो* nodep) {
    फर्काऊ !VN_IS(nodep, गाँठो_Stmt) && !VN_IS(nodep, गाँठो_अभिव्यक्ति);
}
ढाँचा <>
अन्तःस्थ bool Ast_गाँठो::mayBeUnder<Ast_गाँठो_Assign>(अचर Ast_गाँठो* nodep) {
    फर्काऊ !VN_IS(nodep, गाँठो_अभिव्यक्ति);
}
ढाँचा <>
अन्तःस्थ bool Ast_गाँठो::mayBeUnder<AstVarScope>(अचर Ast_गाँठो* nodep) {
    यदि (VN_IS(nodep, VarScope)) फर्काऊ false;  // Should not nest
    यदि (VN_IS(nodep, Var)) फर्काऊ false;
    यदि (VN_IS(nodep, Active)) फर्काऊ false;
    यदि (VN_IS(nodep, गाँठो_Stmt)) फर्काऊ false;
    यदि (VN_IS(nodep, गाँठो_अभिव्यक्ति)) फर्काऊ false;
    फर्काऊ true;
}
ढाँचा <>
अन्तःस्थ bool Ast_गाँठो::mayBeUnder<AstExecGraph>(अचर Ast_गाँठो* nodep) {
    यदि (VN_IS(nodep, ExecGraph)) फर्काऊ false;  // Should not nest
    यदि (VN_IS(nodep, गाँठो_Stmt)) फर्काऊ false;  // Should be directly under CFunc
    फर्काऊ true;
}
ढाँचा <>
अन्तःस्थ bool Ast_गाँठो::mayBeUnder<AstActive>(अचर Ast_गाँठो* nodep) {
    फर्काऊ !VN_IS(nodep, Active);  // AstActives do not nest
}
ढाँचा <>
अन्तःस्थ bool Ast_गाँठो::mayBeUnder<AstScope>(अचर Ast_गाँठो* nodep) {
    फर्काऊ !VN_IS(nodep, Scope);  // AstScopes do not nest
}
ढाँचा <>
अन्तःस्थ bool Ast_गाँठो::mayBeUnder<Ast_Sen_रूख>(अचर Ast_गाँठो* nodep) {
    फर्काऊ !VN_IS(nodep, Sen_रूख);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
ढाँचा <>
constexpr bool Ast_गाँठो::isLeaf<Ast_गाँठो_Var_Ref>() {
    फर्काऊ true;
}
ढाँचा <>
constexpr bool Ast_गाँठो::isLeaf<AstVarRef>() {
    फर्काऊ true;
}
ढाँचा <>
constexpr bool Ast_गाँठो::isLeaf<AstVarXRef>() {
    फर्काऊ true;
}

// foreach implementation
ढाँचा <typename T_Arg, typename T_Callable>
शून्य Ast_गाँठो::foreachImpl(Const_Correct_Ast_गाँठो<T_Arg>* nodep, अचर T_Callable& f, bool visitNext) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using गाँठो_2 = Const_Correct_Ast_गाँठो<T_Arg>;

    // We prefetch this far into the stack
    constexpr पूर्णाङ्क PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr माप_प्रकार MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr माप_प्रकार Initial_थाक_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    माप_प्रकार थाक_Size = Initial_थाक_Size;
    गाँठो_2* in_पङ्क्ति_थाक[Initial_थाक_Size];  // In-line initial stack storage
    std::unique_ptr<गाँठो_2*[]> out_Of_पङ्क्ति_Stackp{nullptr};  // In case we need more, we will allocate
    गाँठो_2** basep = in_पङ्क्ति_थाक + PrefetchDistance;  // Pointer to base of stack
    गाँठो_2** topp = basep;  // Pointer to top of stack
    गाँठो_2** limp = in_पङ्क्ति_थाक + थाक_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    लागि (पूर्णाङ्क i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    अचर स्वतः visit = [&](गाँठो_2* currp) {
        // Type test this node
        यदि (Ast_गाँठो::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            यदि VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) फर्काऊ;
        }

        // Enqueue children for traversal, unless futile
        यदि (mayBeUnder<T_Arg_NonConst>(currp)) {
            यदि (Ast_गाँठो* अचर op4p = currp->op4p()) *topp++ = op4p;
            यदि (Ast_गाँठो* अचर op3p = currp->op3p()) *topp++ = op3p;
            यदि (Ast_गाँठो* अचर op2p = currp->op2p()) *topp++ = op2p;
            यदि (Ast_गाँठो* अचर op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    यदि (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    जबसम्म (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        गाँठो_2* अचर headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        यदि (VL_UNLIKELY(topp >= limp)) {
            अचर सूचक_भेद_प्रकार occupancy = topp - basep;
            {
                अचर माप_प्रकार new_थाक_Size = थाक_Size * 2;
                गाँठो_2** अचर newStackp = new गाँठो_2*[new_थाक_Size];
                std::स_नकल(newStackp, basep - PrefetchDistance,
                            परिमाण(गाँठो_2**) * (occupancy + PrefetchDistance));
                थाक_Size = new_थाक_Size;
                out_Of_पङ्क्ति_Stackp.reset(newStackp);
            }
            basep = out_Of_पङ्क्ति_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_पङ्क्ति_Stackp.get() + थाक_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        यदि (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
ढाँचा <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_गाँठो::predicateImpl(Const_Correct_Ast_गाँठो<T_Arg>* nodep, अचर T_Callable& p,
                            bool visitNext) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using गाँठो_2 = Const_Correct_Ast_गाँठो<T_Arg>;

    // We prefetch this far into the stack
    constexpr पूर्णाङ्क PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr माप_प्रकार MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr माप_प्रकार Initial_थाक_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    माप_प्रकार थाक_Size = Initial_थाक_Size;
    गाँठो_2* in_पङ्क्ति_थाक[Initial_थाक_Size];  // In-line initial stack storage
    std::unique_ptr<गाँठो_2*[]> out_Of_पङ्क्ति_Stackp{nullptr};  // In case we need more, we will allocate
    गाँठो_2** basep = in_पङ्क्ति_थाक + PrefetchDistance;  // Pointer to base of stack
    गाँठो_2** topp = basep;  // Pointer to top of stack
    गाँठो_2** limp = in_पङ्क्ति_थाक + थाक_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    लागि (पूर्णाङ्क i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    अचर स्वतः visit = [&](गाँठो_2* currp) {
        // Type test this node
        यदि (Ast_गाँठो::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            यदि (p(static_cast<T_Arg*>(currp)) != N_Default) फर्काऊ true;
            // Short circuit if iterating leaf nodes
            यदि VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) फर्काऊ false;
        }

        // Enqueue children for traversal, unless futile
        यदि (mayBeUnder<T_Arg_NonConst>(currp)) {
            यदि (Ast_गाँठो* अचर op4p = currp->op4p()) *topp++ = op4p;
            यदि (Ast_गाँठो* अचर op3p = currp->op3p()) *topp++ = op3p;
            यदि (Ast_गाँठो* अचर op2p = currp->op2p()) *topp++ = op2p;
            यदि (Ast_गाँठो* अचर op1p = currp->op1p()) *topp++ = op1p;
        }

        फर्काऊ false;
    };

    // Enqueue the next of the root node, if required
    यदि (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    यदि (visit(nodep)) फर्काऊ !N_Default;

    // Visit the rest of the tree
    जबसम्म (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        गाँठो_2* अचर headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        यदि (VL_UNLIKELY(topp >= limp)) {
            अचर सूचक_भेद_प्रकार occupancy = topp - basep;
            {
                अचर माप_प्रकार new_थाक_Size = थाक_Size * 2;
                गाँठो_2** अचर newStackp = new गाँठो_2*[new_थाक_Size];
                std::स_नकल(newStackp, basep - PrefetchDistance,
                            परिमाण(गाँठो_2**) * (occupancy + PrefetchDistance));
                थाक_Size = new_थाक_Size;
                out_Of_पङ्क्ति_Stackp.reset(newStackp);
            }
            basep = out_Of_पङ्क्ति_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_पङ्क्ति_Stackp.get() + थाक_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        यदि (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        यदि (visit(headp)) फर्काऊ !N_Default;
    }

    फर्काऊ N_Default;
}

अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर Ast_गाँठो* rhs) {
    यदि (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } अन्यथा यदि (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } अन्यथा {
        rhs->dump(os);
    }
    फर्काऊ os;
}
शून्य VNRelinker::relink(Ast_गाँठो* newp) { newp->Ast_गाँठो::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
ढाँचा <typename T_गाँठो>
class VNRef final : सार्वजनिक std::reference_wrapper<T_गाँठो> {
    static_assert(std::is_base_of<Ast_गाँठो, T_गाँठो>::मान,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

सार्वजनिक:
    ढाँचा <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_गाँठो>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(अचर std::reference_wrapper<T_गाँठो>& other)
        : std::reference_wrapper<T_गाँठो>{other} {}
};

static_assert(परिमाण(VNRef<Ast_गाँठो>) == परिमाण(std::reference_wrapper<Ast_गाँठो>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
माप_प्रकार V3HasherUncachedHash(अचर Ast_गाँठो&);

// Specialization of std::hash for VNRef
ढाँचा <typename T_गाँठो>
संरचना std::hash<VNRef<T_गाँठो>> final {
    माप_प्रकार चालक()(VNRef<T_गाँठो> r) अचर { फर्काऊ V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
ढाँचा <typename T_गाँठो>
संरचना std::equal_to<VNRef<T_गाँठो>> final {
    माप_प्रकार चालक()(VNRef<T_गाँठो> ra, VNRef<T_गाँठो> rb) अचर {
        फर्काऊ ra.get().same_रूख(&(rb.get()));
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
    std::unordered_map<अचर Ast_गाँठो*, Ast_गाँठो*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<अचर Ast_गाँठो*, std::vector<Ast_गाँठो*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<अचर Ast_गाँठो*, std::string> m_prefixes;
    अचर VDTypeNameScopes* अचर m_prevp;  // Index in use before this one
    स्थिर अचर VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    शून्य index(Ast_गाँठो* nodep, Ast_गाँठो* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

सार्वजनिक:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    स्थिर अचर VDTypeNameScopes* currentp() { फर्काऊ s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(अचर Ast_गाँठो* nodep, Ast_गाँठो*& outerpr) अचर;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    अचर std::vector<Ast_गाँठो*>* paramsp(अचर Ast_गाँठो* modp) अचर;
    // The prefix kept for scope 'scopep', or nullptr if none
    अचर std::string* prefixp(अचर Ast_गाँठो* scopep) अचर;
    // Keep 'prefix', final, for scope 'scopep'
    शून्य prefix(अचर Ast_गाँठो* scopep, अचर std::string& prefix) अचर;
};

//######################################################################
// Inline VNVisitor METHODS

शून्य VNVisitorConst::iterateConst(Ast_गाँठो* nodep) { nodep->accept(*this); }
शून्य VNVisitorConst::iterateConstNull(Ast_गाँठो* nodep) {
    यदि (VL_LIKELY(nodep)) nodep->accept(*this);
}
शून्य VNVisitorConst::iterateChildrenConst(Ast_गाँठो* nodep) { nodep->iterateChildrenConst(*this); }
शून्य VNVisitorConst::iterateChildrenBackwardsConst(Ast_गाँठो* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
शून्य VNVisitorConst::iterateAndNextConstNull(Ast_गाँठो* nodep) {
    यदि (VL_LIKELY(nodep)) nodep->iterateAndNextConst(*this);
}

शून्य VNVisitor::iterate(Ast_गाँठो* nodep) { nodep->accept(*this); }
शून्य VNVisitor::iterateNull(Ast_गाँठो* nodep) {
    यदि (VL_LIKELY(nodep)) nodep->accept(*this);
}
शून्य VNVisitor::iterateChildren(Ast_गाँठो* nodep) { nodep->iterateChildren(*this); }
शून्य VNVisitor::iterateAndNextNull(Ast_गाँठो* nodep) {
    यदि (VL_LIKELY(nodep)) nodep->iterateAndNext(*this);
}
Ast_गाँठो* VNVisitor::iterateSubtreeReturnEdits(Ast_गाँठो* nodep) {
    फर्काऊ nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#अन्तर्भाव "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#अन्तर्भाव "V3AstNodeDType.h"
#अन्तर्भाव "V3AstNodeOther.h"
#अन्तर्भाव "V3AstNodeExpr.h"
#अन्तर्भाव "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#अन्तर्भाव "V3AstInlines.h"
शून्य dump_गाँठो_List_Json(std::ostream& os, अचर Ast_गाँठो* nodep, अचर std::string& listName,
                      अचर string& indent);

#endif  // Guard
