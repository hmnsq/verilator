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
#जर defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
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
    करा { \
        जर (VL_UNCOVERABLE(test)) परत "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } पर्यंत (false)
// For broken() function, return error string if a base of this class has a match
#define BROKEN_BASE_RTN(test) \
    करा { \
        स्थिरांक अक्षर* स्थिरांक reasonp = (test); \
        जर (VL_UNCOVERABLE(reasonp)) परत reasonp; \
    } पर्यंत (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#define VN_IS(nodep, nodetypename) (Ast_गाठ::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#define VN_CAST(nodep, nodetypename) (Ast_गाठ::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#define VN_AS(nodep, nodetypename) (Ast_गाठ::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#जर defined(VL_DEBUG) || defined(VL_ASAN)
#define VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#अन्यथा
#define VN_DBG_AS(nodep, nodetypename) (Ast_गाठ::unsafePrivateAs<Ast##nodetypename>(nodep))
#endif

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#define VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

संरचना VNTypeInfo final {
    स्थिरांक अक्षर* m_namep;
    गणना OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    स्थिरांक अक्षर* m_opNamep[4];
    माप_प्रकार m_sizeof;
};

class VNType final {
    स्थिर स्थिरांक VNTypeInfo s_typeInfoTable[];

सार्वजनिक:
#अन्तर्भाव "V3Ast__gen_type_enum.h"  // From ./astgen
    स्थिरांक en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr चालक en() स्थिरांक VL_MT_SAFE { परत m_e; }

    // Retrieve VNTypeInfo for the given type
    स्थिर स्थिरांक VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { परत s_typeInfoTable[t.m_e]; }
};
constexpr bool चालक==(स्थिरांक VNType& lhs, स्थिरांक VNType& rhs) VL_PURE {
    परत lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिरांक VNType& lhs, VNType::en rhs) VL_PURE { परत lhs.m_e == rhs; }
constexpr bool चालक==(VNType::en lhs, स्थिरांक VNType& rhs) VL_PURE { परत lhs == rhs.m_e; }
ओळीत std::ostream& चालक<<(std::ostream& os, स्थिरांक VNType& rhs) { परत os << rhs.ascii(); }

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
    संघ {
        शून्य* up;
        uint64_t uq;
    } m_u;
#endif

सार्वजनिक:
#ifdef VL_USER_TYPE_CHECKS
    VNUser() = पूर्वनिर्धारित;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        जर (i) m_u = i;
    }
    explicit VNUser(शून्य* p) {
        // VNUser{nullptr} represents the monostate
        जर (p) m_u = p;
    }
    ~VNUser() = पूर्वनिर्धारित;
    // Casters
    ढाँचा <typename T>
    typename std::enable_if<std::is_pointer<T>::मूल्य, T>::type to() स्थिरांक VL_MT_SAFE {
        जर (std::holds_alternative<std::monostate>(m_u)) परत nullptr;
        शून्य* स्थिरांक* स्थिरांक upp = std::get_if<शून्य*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        परत reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() स्थिरांक {
        जर (std::holds_alternative<std::monostate>(m_u)) परत 0;
        स्थिरांक uint64_t* स्थिरांक uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        परत *uip;
    }
#अन्यथा
    VNUser() = पूर्वनिर्धारित;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.up = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(शून्य* p) { m_u.up = p; }
    ~VNUser() = पूर्वनिर्धारित;
    // Casters
    ढाँचा <typename T>
    typename std::enable_if<std::is_pointer<T>::मूल्य, T>::type to() स्थिरांक VL_MT_SAFE {
        परत reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() स्थिरांक { परत m_u.uq; }
#endif
    VSymEnt* toSymEnt() स्थिरांक { परत to<VSymEnt*>(); }
    Ast_गाठ* toNodep() स्थिरांक VL_MT_SAFE { परत to<Ast_गाठ*>(); }
    V3GraphVertex* toGraphVertex() स्थिरांक { परत to<V3GraphVertex*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(स्थिरांक शून्य*)) स्थिरांक;
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
VL_NOT_FINAL{संरक्षित : स्थिर शून्य allocate(पूर्णांक id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvtToStr(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
स्थिर शून्य मुक्त(पूर्णांक id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvtToStr(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
स्थिर शून्य clearcnt(पूर्णांक id, uint32_t& cntGblRef, स्थिरांक bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
स्थिर शून्य checkcnt(पूर्णांक id, uint32_t&, स्थिरांक bool& userBusyRef) {
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
संरक्षित:
    मित्र class Ast_गाठ;
    स्थिर uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    स्थिर bool s_userBusy;  // Count is in use
सार्वजनिक:
    VNUser1InUse()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1InUse()     { मुक्त    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2InUse final : VNUserInUseBase {
संरक्षित:
    मित्र class Ast_गाठ;
    स्थिर uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    स्थिर bool s_userBusy;  // Count is in use
सार्वजनिक:
    VNUser2InUse()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2InUse()     { मुक्त    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3InUse final : VNUserInUseBase {
संरक्षित:
    मित्र class Ast_गाठ;
    स्थिर uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    स्थिर bool s_userBusy;  // Count is in use
सार्वजनिक:
    VNUser3InUse()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3InUse()     { मुक्त    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    स्थिर शून्य check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4InUse final : VNUserInUseBase {
संरक्षित:
    मित्र class Ast_गाठ;
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
    std::vector<Ast_गाठ*> m_deleteps;  // Nodes to delete

सार्वजनिक:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    शून्य ढकला_Deletep(Ast_गाठ* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.ढकला_back(nodep);
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
    मित्र class Ast_गाठ;

सार्वजनिक:
    /// Call visit()s on nodep
    ओळीत शून्य iterateConst(Ast_गाठ* nodep);
    /// Call visit()s on nodep
    ओळीत शून्य iterateConstNull(Ast_गाठ* nodep);
    /// Call visit()s on const nodep's children
    ओळीत शून्य iterateChildrenConst(Ast_गाठ* nodep);
    /// Call visit()s on nodep's children in backp() order
    ओळीत शून्य iterateChildrenBackwardsConst(Ast_गाठ* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    ओळीत शून्य iterateAndNextConstNull(Ast_गाठ* nodep);

    आभासी शून्य visit(Ast_गाठ* nodep) = 0;
    आभासी ~VNVisitorConst() {}
#अन्तर्भाव "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : सार्वजनिक VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
सार्वजनिक:
    /// Call visit()s on nodep
    ओळीत शून्य iterate(Ast_गाठ* nodep);
    /// Call visit()s on nodep
    ओळीत शून्य iterateNull(Ast_गाठ* nodep);
    /// Call visit()s on nodep's children
    ओळीत शून्य iterateChildren(Ast_गाठ* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    ओळीत शून्य iterateAndNextNull(Ast_गाठ* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    ओळीत Ast_गाठ* iterateSubtreeReturnEdits(Ast_गाठ* nodep);

    VNDeleter& deleter() { परत m_deleter; }
    शून्य ढकला_Deletep(Ast_गाठ* nodep) { deleter().ढकला_Deletep(nodep); }
    शून्य doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
संरक्षित:
    मित्र class Ast_गाठ;
    गणना RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_NEXT,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_गाठ* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_गाठ* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_गाठ** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

सार्वजनिक:
    VNRelinker() = पूर्वनिर्धारित;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    ओळीत शून्य relink(Ast_गाठ* newp);
    शून्य dump(std::ostream& str = std::cout) स्थिरांक;
};
ओळीत std::ostream& चालक<<(std::ostream& os, स्थिरांक VNRelinker& rhs) {
    rhs.dump(os);
    परत os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{सार्वजनिक : आभासी bool गाठ_Match(स्थिरांक Ast_गाठ* nodep) स्थिरांक {परत true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#define ASTNODE_PREFETCH_NON_NULL(nodep) \
    करा { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } पर्यंत (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#define ASTNODE_PREFETCH(nodep) \
    करा { \
        जर (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } पर्यंत (false)

class Ast_गाठ VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_गाठ* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_गाठ* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_गाठ* m_op1p = nullptr;  // Generic pointer 1
    Ast_गाठ* m_op2p = nullptr;  // Generic pointer 2
    Ast_गाठ* m_op3p = nullptr;  // Generic pointer 3
    Ast_गाठ* m_op4p = nullptr;  // Generic pointer 4
    Ast_गाठ** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    स्थिरांक VNType m_type;  // Node sub-type identifier
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

    पूर्णांक m_cloneCnt = 0;  // Sequence number for when last clone was made

#जर defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(आकार(m_type) + आकार(m_flags) + आकार(m_brokenState) + आकार(m_cloneCnt)
                      <= आकार(शून्य*),
                  "packing requires padding");
#endif

    Ast_गाठ_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_गाठ* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_ओळ* m_fileline;  // Where it was declared
#ifdef VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_मोजणी;  // When it was last edited
#endif
    स्थिर uint64_t s_editCntGbl;  // Global edit counter
    स्थिर uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_गाठ* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    स्थिर पूर्णांक s_cloneCntGbl;  // Count of which userp is set

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
    शून्य op1p(Ast_गाठ* nodep) {
        m_op1p = nodep;
        जर (nodep) nodep->m_backp = this;
    }
    शून्य op2p(Ast_गाठ* nodep) {
        m_op2p = nodep;
        जर (nodep) nodep->m_backp = this;
    }
    शून्य op3p(Ast_गाठ* nodep) {
        m_op3p = nodep;
        जर (nodep) nodep->m_backp = this;
    }
    शून्य op4p(Ast_गाठ* nodep) {
        m_op4p = nodep;
        जर (nodep) nodep->m_backp = this;
    }

खाजगी:
    Ast_गाठ* clone_वृक्ष_Iter(bool needPure);
    Ast_गाठ* clone_वृक्ष_Iter_List(bool needPure);
    शून्य check_वृक्ष_Iter(स्थिरांक Ast_गाठ* prevBackp) स्थिरांक VL_MT_STABLE;
    bool gate_वृक्ष_Iter() स्थिरांक;
    स्थिर bool same_वृक्ष_Iter(स्थिरांक Ast_गाठ* node1p, स्थिरांक Ast_गाठ* node2p, bool ignNext,
                             bool gateOnly);
    शून्य delete_वृक्ष_Iter();
    शून्य delete_गाठ();
    string instanceStr() स्थिरांक;

सार्वजनिक:
    शून्य purityCheck();
    स्थिर शून्य relinkOneLink(Ast_गाठ*& pointpr, Ast_गाठ* newp);
    // cppcheck-suppress functionConst
    स्थिर शून्य debug_वृक्ष_Change(स्थिरांक Ast_गाठ* nodep, स्थिरांक अक्षर* prefix, पूर्णांक lineno, bool next);

संरक्षित:
    // CONSTRUCTORS
    Ast_गाठ(VNType t, File_ओळ* fl);
    आभासी ~Ast_गाठ() = पूर्वनिर्धारित;  // Use 'deleteTree' instead
    आभासी Ast_गाठ* clone() = 0;  // Generally, cloneTree is what you want instead
    आभासी शून्य cloneRelink() { cloneRelinkGen(); }
    आभासी शून्य cloneRelinkGen() {};  // Overrides generated by 'astgen'
    शून्य clone_Relink_वृक्ष();

    // METHODS
    शून्य setOp1p(Ast_गाठ* newp);  // Set non-list-type op1 to non-list element
    शून्य setOp2p(Ast_गाठ* newp);  // Set non-list-type op2 to non-list element
    शून्य setOp3p(Ast_गाठ* newp);  // Set non-list-type op3 to non-list element
    शून्य setOp4p(Ast_गाठ* newp);  // Set non-list-type op4 to non-list element

    शून्य addOp1p(Ast_गाठ* newp);  // Append newp to end of op1
    शून्य addOp2p(Ast_गाठ* newp);  // Append newp to end of op2
    शून्य addOp3p(Ast_गाठ* newp);  // Append newp to end of op3
    शून्य addOp4p(Ast_गाठ* newp);  // Append newp to end of op4

    शून्य prependOp1p(Ast_गाठ* newp);  // Prepend newp to front of op1
    शून्य prependOp2p(Ast_गाठ* newp);  // Prepend newp to front of op2
    शून्य prependOp3p(Ast_गाठ* newp);  // Prepend newp to front of op3
    शून्य prependOp4p(Ast_गाठ* newp);  // Prepend newp to front of op4

    // clang-format off
    शून्य setNOp1p(Ast_गाठ* newp) { जर (newp) setOp1p(newp); }
    शून्य setNOp2p(Ast_गाठ* newp) { जर (newp) setOp2p(newp); }
    शून्य setNOp3p(Ast_गाठ* newp) { जर (newp) setOp3p(newp); }
    शून्य setNOp4p(Ast_गाठ* newp) { जर (newp) setOp4p(newp); }

    शून्य addNOp1p(Ast_गाठ* newp) { जर (newp) addOp1p(newp); }
    शून्य addNOp2p(Ast_गाठ* newp) { जर (newp) addOp2p(newp); }
    शून्य addNOp3p(Ast_गाठ* newp) { जर (newp) addOp3p(newp); }
    शून्य addNOp4p(Ast_गाठ* newp) { जर (newp) addOp4p(newp); }

    शून्य prependNOp1p(Ast_गाठ* newp) { जर (newp) prependOp1p(newp); }
    शून्य prependNOp2p(Ast_गाठ* newp) { जर (newp) prependOp2p(newp); }
    शून्य prependNOp3p(Ast_गाठ* newp) { जर (newp) prependOp3p(newp); }
    शून्य prependNOp4p(Ast_गाठ* newp) { जर (newp) prependOp4p(newp); }
    // clang-format on

    शून्य clonep(Ast_गाठ* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    स्थिर शून्य clone_Clear_वृक्ष() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    आभासी bool same_गाठ(स्थिरांक Ast_गाठ*) स्थिरांक { परत true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    आभासी bool wouldBreakGen(स्थिरांक Ast_गाठ* स्थिरांक oldp,
                               स्थिरांक Ast_गाठ* स्थिरांक newp) स्थिरांक
        = 0;  // Generated by 'astgen'

सार्वजनिक:
    // ACCESSORS
    VNType type() स्थिरांक VL_MT_SAFE { परत m_type; }
    स्थिरांक अक्षर* typeName() स्थिरांक VL_MT_SAFE { परत type().ascii(); }  // See also prettyTypeName
    Ast_गाठ* nextp() स्थिरांक VL_MT_STABLE { परत m_nextp; }
    Ast_गाठ* backp() स्थिरांक VL_MT_STABLE { परत m_backp; }
    Ast_गाठ* abovep() स्थिरांक;  // Get parent node above, only for list head and tail
    Ast_गाठ* aboveLoopp() स्थिरांक;  // Get parent node above, may have performance issues as loops
    Ast_गाठ* lastp() स्थिरांक {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        परत m_headtailp;
    }
    Ast_गाठ* prevp() स्थिरांक {  // Previous node in list, nullptr for head of list
        जर (m_backp->m_nextp != this) परत nullptr;
        परत m_backp;
    }
    Ast_गाठ* op1p() स्थिरांक VL_MT_STABLE { परत m_op1p; }
    Ast_गाठ* op2p() स्थिरांक VL_MT_STABLE { परत m_op2p; }
    Ast_गाठ* op3p() स्थिरांक VL_MT_STABLE { परत m_op3p; }
    Ast_गाठ* op4p() स्थिरांक VL_MT_STABLE { परत m_op4p; }
    Ast_गाठ_DType* dtypep() स्थिरांक VL_MT_STABLE { परत m_dtypep; }
    Ast_गाठ* clonep() स्थिरांक { परत ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_गाठ* firstAbovep() स्थिरांक {  // Returns nullptr when second or later in list
        परत ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() स्थिरांक VL_MT_SAFE { परत m_brokenState; }
    शून्य brokenState(uint8_t मूल्य) { m_brokenState = मूल्य; }

    // Used by AstNode::broken()
    bool brokeExists() स्थिरांक { परत V3Broken::isLinkable(this); }
    bool brokeExistsAbove() स्थिरांक { परत brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    आभासी शून्य foreachLink(std::function<शून्य(Ast_गाठ** linkpp, स्थिरांक अक्षर* namep)> f) = 0;

#जर defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    स्थिर शून्य* चालक new(माप_प्रकार size);
    स्थिर शून्य चालक delete(शून्य* obj, माप_प्रकार size);
#endif

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    स्थिर constexpr पूर्णांक INSTR_मोजणी_BRANCH = 4;  // Branch
    स्थिर constexpr पूर्णांक INSTR_मोजणी_CALL = INSTR_मोजणी_BRANCH + 10;  // Subroutine call
    स्थिर constexpr पूर्णांक INSTR_मोजणी_LD = 2;  // Load memory
    स्थिर constexpr पूर्णांक INSTR_मोजणी_INT_MUL = 3;  // Integer multiply
    स्थिर constexpr पूर्णांक INSTR_मोजणी_INT_DIV = 10;  // Integer divide
    स्थिर constexpr पूर्णांक INSTR_मोजणी_DBL = 8;  // Convert or do float ops
    स्थिर constexpr पूर्णांक INSTR_मोजणी_DBL_DIV = 40;  // Double divide
    स्थिर constexpr पूर्णांक INSTR_मोजणी_DBL_TRIG = 200;  // Double trigonometric ops
    स्थिर constexpr पूर्णांक INSTR_मोजणी_STR = 100;  // String ops
    स्थिर constexpr पूर्णांक INSTR_मोजणी_TIME = INSTR_मोजणी_CALL + 5;  // Determine simulation time
    स्थिर constexpr पूर्णांक INSTR_मोजणी_PLI = 20;  // PLI routines
    स्थिर constexpr पूर्णांक INSTR_मोजणी_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    आभासी string name() स्थिरांक VL_MT_STABLE { परत ""; }
    आभासी string origName() स्थिरांक { परत ""; }
    string prettyDehashOrigOrName() स्थिरांक {
        परत prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    आभासी शून्य name(स्थिरांक string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    आभासी शून्य tag(स्थिरांक string& text) {}
    आभासी string tag() स्थिरांक { परत ""; }
    आभासी uint32_t decl_Token_अंक() स्थिरांक { परत 0; }
    आभासी शून्य decl_Token_अंक_Set_Min(uint32_t token_अंक) {}
    आभासी string verilogKwd() स्थिरांक { परत ""; }
    string nameProtect() स्थिरांक VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() स्थिरांक;  // origName with --protect-id applied
    string shortName() स्थिरांक;  // Name with __PVT__ removed for concatenating scopes
    स्थिर string dedotName(स्थिरांक string& namein);  // Name with dots removed
    स्थिर string nameNoArray(स्थिरांक string& namein);  // Name with any array index removed
    स्थिर string prettyName(स्थिरांक string& namein) VL_PURE;  // Name for printing out to the user
    स्थिर string vpiName(स्थिरांक string& namein);  // Name for vpi access
    स्थिर string prettyNameQ(स्थिरांक string& namein) {  // Quoted pretty name (for errors)
        परत "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    स्थिर string encodeName(स्थिरांक string& namein);
    स्थिर string encode_संख्या(int64_t अंक);  // Encode number into internal C representation
    स्थिर string vcdName(स्थिरांक string& namein);  // Name for printing out to vcd files
    string prettyName() स्थिरांक { परत prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    आभासी string pretty_Name_संदेश() स्थिरांक { परत prettyName(); }
    string prettyNameQ() स्थिरांक { परत "'"s + pretty_Name_संदेश() + "'"; }  // Quoted, for messages
    string verilogName() स्थिरांक { परत vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() स्थिरांक;
    आभासी string prettyOperatorName() स्थिरांक { परत "operator " + prettyTypeName(); }
    File_ओळ* fileline() स्थिरांक VL_MT_SAFE { परत m_fileline; }
    शून्य fileline(File_ओळ* fl) { m_fileline = fl; }
    ओळीत bool width1() स्थिरांक;
    ओळीत पूर्णांक widthInstrs() स्थिरांक;
    bool didWidth() स्थिरांक { परत m_flags.didWidth; }
    शून्य didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        जर (didWidth()) परत true;
        didWidth(true);
        परत false;
    }
    bool doingWidth() स्थिरांक { परत m_flags.doingWidth; }
    शून्य doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() स्थिरांक VL_MT_SAFE { परत m_flags.protect; }
    शून्य protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    ओळीत पूर्णांक width() स्थिरांक VL_MT_STABLE;
    ओळीत पूर्णांक widthMin() स्थिरांक VL_MT_STABLE;
    पूर्णांक widthMinV() स्थिरांक {
        परत v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    पूर्णांक width_शब्द() स्थिरांक { परत VL_शब्द_I(width()); }
    bool isQuad() स्थिरांक VL_MT_STABLE { परत (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() स्थिरांक VL_MT_STABLE { परत (width() > VL_QUADSIZE); }
    ओळीत bool isCHandle() स्थिरांक VL_MT_STABLE;
    ओळीत bool isDouble() स्थिरांक VL_MT_STABLE;
    ओळीत bool isSigned() स्थिरांक VL_MT_STABLE;
    ओळीत bool isString() स्थिरांक VL_MT_STABLE;
    ओळीत bool isEvent() स्थिरांक VL_MT_STABLE;

    // clang-format off
    VNUser user1u() स्थिरांक VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        परत ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_गाठ* user1p() स्थिरांक VL_MT_STABLE { परत user1u().toNodep(); }
    शून्य user1u(स्थिरांक VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    शून्य user1p(शून्य* userp) { user1u(VNUser{userp}); }
    शून्य user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() स्थिरांक { परत user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { स्थिरांक uint64_t v = user1(); user1(v + val); परत v; }
    uint64_t user1Or(uint64_t val) { स्थिरांक uint64_t v = user1(); user1(v | val); परत v; }
    uint64_t user1SetOnce() { स्थिरांक uint64_t v = user1(); जर (!v) user1(1); परत v; }  // Better for cache than user1Inc()
    स्थिर शून्य user1_Clear_वृक्ष() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() स्थिरांक VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        परत ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_गाठ* user2p() स्थिरांक VL_MT_STABLE { परत user2u().toNodep(); }
    शून्य user2u(स्थिरांक VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    शून्य user2p(शून्य* userp) { user2u(VNUser{userp}); }
    शून्य user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() स्थिरांक { परत user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { स्थिरांक uint64_t v = user2(); user2(v + val); परत v; }
    uint64_t user2Or(uint64_t val) { स्थिरांक uint64_t v = user2(); user2(v | val); परत v; }
    uint64_t user2SetOnce() { स्थिरांक uint64_t v = user2(); जर (!v) user2(1); परत v; }  // Better for cache than user2Inc()
    स्थिर शून्य user2_Clear_वृक्ष() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() स्थिरांक VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        परत ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_गाठ* user3p() स्थिरांक VL_MT_STABLE { परत user3u().toNodep(); }
    शून्य user3u(स्थिरांक VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    शून्य user3p(शून्य* userp) { user3u(VNUser{userp}); }
    शून्य user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() स्थिरांक { परत user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { स्थिरांक uint64_t v = user3(); user3(v + val); परत v; }
    uint64_t user3Or(uint64_t val) { स्थिरांक uint64_t v = user3(); user3(v | val); परत v; }
    uint64_t user3SetOnce() { स्थिरांक uint64_t v = user3(); जर (!v) user3(1); परत v; }  // Better for cache than user3Inc()
    स्थिर शून्य user3_Clear_वृक्ष() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() स्थिरांक VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        परत ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_गाठ* user4p() स्थिरांक VL_MT_STABLE { परत user4u().toNodep(); }
    शून्य user4u(स्थिरांक VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    शून्य user4p(शून्य* userp) { user4u(VNUser{userp}); }
    शून्य user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() स्थिरांक { परत user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { स्थिरांक uint64_t v = user4(); user4(v | val); परत v; }
    uint64_t user4Inc(uint64_t val = 1) { स्थिरांक uint64_t v = user4(); user4(v + val); परत v; }
    uint64_t user4SetOnce() { स्थिरांक uint64_t v = user4(); जर (!v) user4(1); परत v; }  // Better for cache than user4Inc()
    स्थिर शून्य user4_Clear_वृक्ष() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ifdef VL_DEBUG
    uint64_t edit_मोजणी() स्थिरांक { परत m_edit_मोजणी; }
    शून्य edit_मोजणी_Inc() {
        m_edit_मोजणी = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_वृक्ष();  // Any edit clears all caching
    }
#अन्यथा
    शून्य edit_मोजणी_Inc() { ++s_editCntGbl; }
#endif
    स्थिर uint64_t edit_मोजणी_Last() VL_MT_SAFE { परत s_editCntLast; }
    स्थिर uint64_t edit_मोजणी_Gbl() VL_MT_SAFE { परत s_editCntGbl; }
    स्थिर शून्य edit_मोजणी_Set_Last() { s_editCntLast = edit_मोजणी_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_ढकला_Self_Stmt();
    ओळीत bool is_Class_Handle_मूल्य() स्थिरांक;
    ओळीत bool isNull() स्थिरांक;
    ओळीत bool isZero() स्थिरांक;
    ओळीत bool isOne() स्थिरांक;
    ओळीत bool isNeqZero() स्थिरांक;
    ओळीत bool isAllOnes() स्थिरांक;
    ओळीत bool isAllOnesV() स्थिरांक;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    शून्य dtypep(Ast_गाठ_DType* nodep) {
        जर (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_मोजणी_Inc();
        }
    }
    शून्य dtypeFrom(स्थिरांक Ast_गाठ* fromp) {
        जर (fromp) dtypep(fromp->dtypep());
    }
    शून्य dtypeChgSigned(bool flag = true);
    शून्य dtypeChgWidth(पूर्णांक width, पूर्णांक widthMin);
    शून्य dtypeChgWidthSigned(पूर्णांक width, पूर्णांक widthMin, VSigning numeric);
    शून्य dtypeSetBitUnsized(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) {
        dtypep(findBitDType(width, widthMin, numeric));
    }
    शून्य dtypeSetBitSized(पूर्णांक width, VSigning numeric) {
        dtypep(findBitDType(width, width, numeric));  // Since sized, widthMin is width
    }
    शून्य dtypeSetLogicUnsized(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) {
        dtypep(findLogicDType(width, widthMin, numeric));
    }
    शून्य dtypeSetLogicSized(पूर्णांक width, VSigning numeric) {
        dtypep(findLogicDType(width, width, numeric));  // Since sized, widthMin is width
    }
    शून्य dtypeSetLogic2StateUnsized(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) {
        dtypep(findLogic2StateDType(width, widthMin, numeric));
    }
    शून्य dtypeSetLogic2StateSized(पूर्णांक width, VSigning numeric) {
        dtypep(findLogic2StateDType(width, width, numeric));  // Since sized, widthMin is width
    }
    शून्य dtypeSetBitOrLogicUnsized(पूर्णांक width, पूर्णांक widthMin, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, widthMin, numeric, isFourstate));
    }
    शून्य dtypeSetBitOrLogicSized(पूर्णांक width, VSigning numeric, bool isFourstate) {
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
    Ast_गाठ_DType* findBitDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_गाठ_DType* findLogicDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_गाठ_DType* findNullDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_गाठ_DType* findLogic2StateDType() स्थिरांक {
        परत findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_गाठ_DType* findDoubleDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_गाठ_DType* findIntDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::INT); }
    Ast_गाठ_DType* findIntegerDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_गाठ_DType* findInteger2StateDType() स्थिरांक {
        परत findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_गाठ_DType* findStringDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_गाठ_DType* findSigned8DType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_गाठ_DType* findUInt32DType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_गाठ_DType* findUInt64DType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_गाठ_DType* findTimeDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_गाठ_DType* findTime2StateDType() स्थिरांक {
        परत findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_गाठ_DType* findCHandleDType() स्थिरांक { परत findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_गाठ_DType* findConstraintRefDType() स्थिरांक;
    Ast_गाठ_DType* findEmptyQueueDType() स्थिरांक;
    Ast_गाठ_DType* findQueueIndexDType() स्थिरांक;
    Ast_गाठ_DType* findStreamDType() स्थिरांक;
    Ast_गाठ_DType* findVoidDType() स्थिरांक;
    Ast_गाठ_DType* findBitDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिरांक;
    Ast_गाठ_DType* findLogicDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिरांक;
    Ast_गाठ_DType* findLogic2StateDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिरांक;
    Ast_गाठ_DType* findBitOrLogicDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric,
                                      bool isFourstate) स्थिरांक {
        परत isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_गाठ_DType* findBitOrLogicRangeDType(स्थिरांक VNumRange& range, स्थिरांक पूर्णांक widthMin,
                                           स्थिरांक VSigning numeric,
                                           स्थिरांक bool isFourstate) स्थिरांक VL_MT_STABLE {
        परत isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_गाठ_DType* findLogicRangeDType(स्थिरांक VNumRange& range, पूर्णांक widthMin,
                                      VSigning numeric) स्थिरांक VL_MT_STABLE;
    Ast_गाठ_DType* findBitRangeDType(स्थिरांक VNumRange& range, पूर्णांक widthMin,
                                    VSigning numeric) स्थिरांक VL_MT_STABLE;
    Ast_गाठ_DType* findBasicDType(VBasicDTypeKwd kwd) स्थिरांक;
    स्थिर AstBasicDType* find_घाला_Same_DType(AstBasicDType* nodep);

    स्थिर VCastable computeCastable(स्थिरांक Ast_गाठ_DType* toDtp, स्थिरांक Ast_गाठ_DType* fromDtp,
                                     स्थिरांक Ast_गाठ* fromConstp, bool checkIfaceArgCompat = false);
    स्थिर Ast_गाठ_DType* getCommonClassTypep(Ast_गाठ* nodep1, Ast_गाठ* nodep2);

    // METHODS - dump and error
    शून्य v3errorEnd(स्थिरांक std::ostringstream& str) स्थिरांक VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(स्थिरांक std::ostringstream& str) स्थिरांक VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex) {
        परत fileline()->warnContextPrimary();
    }
    string warnContextSecondary() स्थिरांक { परत fileline()->warnContextSecondary(); }
    string warnMore() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex) { परत fileline()->warnMore(); }
    string warnOther() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex) { परत fileline()->warnOther(); }

    आभासी शून्य dump(std::ostream& str = std::cout) स्थिरांक;
    स्थिर अक्षर* dump_वृक्ष_Json_Gdb(स्थिरांक Ast_गाठ* nodep);  // For GDB only, free()d by caller
    स्थिर अक्षर* dump_वृक्ष_Json_Gdb(स्थिरांक अक्षर* str);  // For GDB only, free()d by caller
    स्थिर अक्षर* dump_वृक्ष_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    स्थिर शून्य dumpGdb(स्थिरांक Ast_गाठ* nodep);  // For GDB only
    शून्य dumpGdbHeader() स्थिरांक;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    ढाँचा <typename T_गाठ_Result, typename T_गाठ_Next>
    स्थिर T_गाठ_Result* addNext(T_गाठ_Result* nodep, T_गाठ_Next* newp) {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ_Result>::मूल्य,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_गाठ_Result, T_गाठ_Next>::मूल्य,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        जर (!newp) परत nodep;
        परत static_cast<T_गाठ_Result*>(addNext<Ast_गाठ, Ast_गाठ>(nodep, newp));
    }
    ढाँचा <typename T_गाठ_Result, typename T_गाठ_Next>
    स्थिर T_गाठ_Result* addNextNull(T_गाठ_Result* nodep, T_गाठ_Next* newp) {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ_Result>::मूल्य,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_गाठ_Result, T_गाठ_Next>::मूल्य,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        जर (!newp) परत nodep;
        जर (!nodep) परत newp;
        परत static_cast<T_गाठ_Result*>(addNext<Ast_गाठ, Ast_गाठ>(nodep, newp));
    }
    ओळीत Ast_गाठ* addNext(Ast_गाठ* newp);
    शून्य addNextHere(Ast_गाठ* newp);  // Insert newp at this->nextp
    शून्य addHereThisAsNext(Ast_गाठ* newp);  // Adds at old place of this, this becomes next
    शून्य replaceWith(Ast_गाठ* newp);  // Replace current node in tree with new node
    शून्य replaceWithKeepDType(Ast_गाठ* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_गाठ* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_गाठ* unlinkFrBackWithNext(VNRelinker* linkerp = nullptr);
    शून्य relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    शून्य clone_Relink_गाठ() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_गाठ* clone_वृक्ष(bool cloneNextLink,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_गाठ* clone_वृक्ष_Pure(bool cloneNextLink) { परत clone_वृक्ष(cloneNextLink, true); }
    bool gate_वृक्ष() { परत gate_वृक्ष_Iter(); }  // Is tree isGateOptimizable?
    ओळीत bool same_वृक्ष(स्थिरांक Ast_गाठ* node2p) स्थिरांक;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    ओळीत bool same_Gate_वृक्ष(स्थिरांक Ast_गाठ* node2p) स्थिरांक;
    शून्य delete_वृक्ष();  // Always deletes the next link
    शून्य check_वृक्ष() स्थिरांक VL_MT_STABLE {
        जर (v3Global.opt.debugCheck()) check_वृक्ष_Iter(backp());
    }
    शून्य checkIter() स्थिरांक;
    शून्य dumpPtrs(std::ostream& os = std::cout) स्थिरांक;
    शून्य dump_वृक्ष(std::ostream& os = std::cout, स्थिरांक string& indent = "    ",
                  पूर्णांक maxDepth = 0) स्थिरांक;
    शून्य dump_वृक्ष(स्थिरांक string& indent, पूर्णांक maxDepth = 0) स्थिरांक {
        dump_वृक्ष(std::cout, indent, maxDepth);
    }
    स्थिर शून्य dump_वृक्ष_Gdb(स्थिरांक Ast_गाठ* nodep);  // For GDB only
    शून्य dump_वृक्ष_And_Next(std::ostream& os = std::cout, स्थिरांक string& indent = "    ",
                         पूर्णांक maxDepth = 0) स्थिरांक;
    शून्य dump_वृक्ष_File(स्थिरांक string& filename, bool doDump = true);
    स्थिर शून्य dump_वृक्ष_File_Gdb(स्थिरांक Ast_गाठ* nodep, स्थिरांक अक्षर* filenamep = nullptr);
    शून्य dump_वृक्ष_Dot(std::ostream& os = std::cout) स्थिरांक;
    शून्य dump_वृक्ष_Dot_File(स्थिरांक string& filename, bool doDump = true);
    आभासी शून्य dumpJson(std::ostream& os) स्थिरांक { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    शून्य dumpJsonGen(std::ostream& os) स्थिरांक {};
    आभासी शून्य dump_वृक्ष_Json_Op_Gen(std::ostream& os, स्थिरांक string& indent) स्थिरांक {};
    शून्य dump_वृक्ष_Json(std::ostream& os, स्थिरांक string& indent = "") स्थिरांक;
    शून्य dump_वृक्ष_Json_File(स्थिरांक string& filename, bool doDump = true);
    स्थिर शून्य dumpJsonMetaFileGdb(स्थिरांक अक्षर* filename);
    स्थिर शून्य dumpJsonMetaFile(स्थिरांक string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    स्थिर std::string गाठ_Addr(स्थिरांक Ast_गाठ* nodep) {
        परत v3Global.opt.dump_वृक्ष_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    स्थिर Ast_गाठ* afterCommentp(Ast_गाठ* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        पर्यंत (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        परत nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    आभासी bool isBrancher() स्थिरांक { परत false; }
    // Else a AstTime etc that can't be pushed out
    आभासी bool isGateOptimizable() स्थिरांक { परत !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    आभासी bool isGateDedupable() स्थिरांक { परत isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    आभासी bool is_अभिव्यक्ती_Coverage_Eligible() स्थिरांक { परत isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    आभासी bool isOutputter() { परत false; }
    // Else a AstTime etc which output can't be predicted from input
    आभासी bool isPredictOptimizable() स्थिरांक { परत !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    आभासी bool isPure() { परत true; }
    // Iff isPure on current node and any nextp()
    bool isPureAndNext() { परत isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    आभासी bool isTimingControl() स्थिरांक { परत false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    आभासी bool isUnlikely() स्थिरांक { परत false; }
    // Is an IEEE system function (versus internally-generated)
    आभासी bool isSystemFunc() स्थिरांक { परत false; }
    आभासी पूर्णांक instr_मोजणी() स्थिरांक { परत 0; }
    // Iff node is identical to another node
    आभासी bool isSame(स्थिरांक Ast_गाठ* samep) स्थिरांक {
        परत type() == samep->type() && same_गाठ(samep);
    }
    // Iff has a data type; dtype() must be non-null
    आभासी bool hasDType() स्थिरांक VL_MT_SAFE { परत false; }
    // Iff has a non-null childDTypep(), as generic node function
    आभासी Ast_गाठ_DType* getChildDTypep() स्थिरांक { परत nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    आभासी Ast_गाठ_DType* getChild2DTypep() स्थिरांक { परत nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    आभासी bool maybePointedTo() स्थिरांक VL_MT_SAFE { परत false; }
    // Don't reclaim this node in V3Dead
    आभासी bool undead() स्थिरांक { परत false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    आभासी स्थिरांक अक्षर* broken() स्थिरांक { परत nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    आभासी स्थिरांक अक्षर* brokenGen() स्थिरांक = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(स्थिरांक Ast_गाठ* स्थिरांक newp) स्थिरांक { परत backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    आभासी शून्य accept(VNVisitorConst& v) = 0;

संरक्षित:
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
    Ast_गाठ* iterateSubtreeReturnEdits(VNVisitor& v);

    स्थिर शून्य dump_Json_अंक(std::ostream& os, स्थिरांक std::string& name, int64_t val);
    स्थिर शून्य dumpJsonBoolIf(std::ostream& os, स्थिरांक std::string& name, bool val);
    स्थिर शून्य dumpJsonStr(std::ostream& os, स्थिरांक std::string& name, स्थिरांक std::string& val);
    स्थिर शून्य dumpJsonPtr(std::ostream& os, स्थिरांक std::string& name, स्थिरांक Ast_गाठ* स्थिरांक valp);

संरक्षित:
    शून्य iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    ढाँचा <typename T>
    ओळीत स्थिर bool privateTypeTest(स्थिरांक Ast_गाठ* nodep);

    // For internal use only.
    ढाँचा <typename T_TargetType, typename T_DeclType>
    constexpr स्थिर bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        परत std::is_base_of<T_TargetType, NonCV>::मूल्य;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    ढाँचा <typename T_TargetType, typename T_DeclType>
    constexpr स्थिर bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        परत !std::is_base_of<NonCV, T_TargetType>::मूल्य;
    }  // LCOV_EXCL_STOP

सार्वजनिक:
    // For use via the VN_IS macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाठ>
    स्थिर bool is(स्थिरांक T_गाठ* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        परत nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाठ>
    स्थिर T_TargetType* cast(T_गाठ* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        परत nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    ढाँचा <typename T_TargetType, typename T_गाठ>
    स्थिर स्थिरांक T_TargetType* cast(स्थिरांक T_गाठ* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        परत nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<स्थिरांक T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाठ>
    स्थिर T_TargetType* as(T_गाठ* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        परत unsafePrivateAs<T_TargetType, T_गाठ>(nodep);
    }
    ढाँचा <typename T_TargetType, typename T_गाठ>
    स्थिर स्थिरांक T_TargetType* as(स्थिरांक T_गाठ* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        परत unsafePrivateAs<T_TargetType, T_गाठ>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    ढाँचा <typename T_TargetType, typename T_गाठ>
    स्थिर T_TargetType* unsafePrivateAs(T_गाठ* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        परत reinterpret_cast<T_TargetType*>(nodep);
    }
    ढाँचा <typename T_TargetType, typename T_गाठ>
    स्थिर स्थिरांक T_TargetType* unsafePrivateAs(स्थिरांक T_गाठ* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाठ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        परत reinterpret_cast<स्थिरांक T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ढाँचा <typename T_गाठ>
    स्थिर bool mayBeUnder(स्थिरांक Ast_गाठ* nodep) {
        static_assert(!std::is_const<T_गाठ>::मूल्य,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        परत true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ढाँचा <typename T_गाठ>
    स्थिर constexpr bool isLeaf() {
        static_assert(!std::is_const<T_गाठ>::मूल्य,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        परत false;
    }

खाजगी:
    // Using std::conditional for const correctness in the public 'foreach' functions
    ढाँचा <typename T_Arg>
    using Const_Correct_Ast_गाठ =
        typename std::conditional<std::is_const<T_Arg>::मूल्य, स्थिरांक Ast_गाठ, Ast_गाठ>::type;

    ढाँचा <typename T_Arg, typename T_Callable>
    ओळीत स्थिर शून्य foreachImpl(Const_Correct_Ast_गाठ<T_Arg>* nodep, स्थिरांक T_Callable& f,
                                   bool visitNext);

    ढाँचा <typename T_Arg, bool N_Default, typename T_Callable>
    ओळीत स्थिर bool predicateImpl(Const_Correct_Ast_गाठ<T_Arg>* nodep, स्थिरांक T_Callable& p,
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
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_गाठ*>::मूल्य
                          && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_गाठ>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    शून्य foreach(T_Callable&& f) स्थिरांक {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, स्थिरांक T_गाठ*>::मूल्य
                && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<स्थिरांक T_गाठ>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    ढाँचा <typename T_Callable>
    शून्य foreachAndNext(T_Callable&& f) {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_गाठ*>::मूल्य
                          && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_गाठ>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    शून्य foreachAndNext(T_Callable&& f) स्थिरांक {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, स्थिरांक T_गाठ*>::मूल्य
                && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<स्थिरांक T_गाठ>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    ढाँचा <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_गाठ*>::मूल्य
                          && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        परत predicateImpl<T_गाठ, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool exists(T_Callable&& p) स्थिरांक {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, स्थिरांक T_गाठ*>::मूल्य
                          && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        परत predicateImpl<स्थिरांक T_गाठ, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    ढाँचा <typename T_Callable>
    bool existsAndNext(T_Callable&& p) {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, स्थिरांक T_गाठ*>::मूल्य
                          && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        परत predicateImpl<T_गाठ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool existsAndNext(T_Callable&& p) स्थिरांक {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, स्थिरांक T_गाठ*>::मूल्य
                          && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        परत predicateImpl<स्थिरांक T_गाठ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    ढाँचा <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_गाठ*>::मूल्य
                          && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        परत predicateImpl<T_गाठ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool forall(T_Callable&& p) स्थिरांक {
        using T_गाठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, स्थिरांक T_गाठ*>::मूल्य
                          && std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        परत predicateImpl<स्थिरांक T_गाठ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    पूर्णांक गाठ_मोजणी() स्थिरांक {
        // TODO: this should really return size_t, but need to fix use sites
        पूर्णांक मोजणी = 0;
        this->foreach([&मोजणी](स्थिरांक Ast_गाठ*) { ++मोजणी; });
        परत मोजणी;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(पूर्णांक limit) स्थिरांक {
        पूर्णांक मोजणी = 0;
        परत this->exists([&मोजणी, limit](स्थिरांक Ast_गाठ*) { परत ++मोजणी > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
ढाँचा <>
Ast_गाठ* Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(Ast_गाठ* nodep, Ast_गाठ* newp);

// Inline method implementations
Ast_गाठ* Ast_गाठ::addNext(Ast_गाठ* newp) { परत addNext(this, newp); }

// Specializations of privateTypeTest
#अन्तर्भाव "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
ढाँचा <>
ओळीत bool Ast_गाठ::mayBeUnder<AstCell>(स्थिरांक Ast_गाठ* nodep) {
    परत !VN_IS(nodep, गाठ_Stmt) && !VN_IS(nodep, गाठ_अभिव्यक्ती);
}
ढाँचा <>
ओळीत bool Ast_गाठ::mayBeUnder<Ast_गाठ_Assign>(स्थिरांक Ast_गाठ* nodep) {
    परत !VN_IS(nodep, गाठ_अभिव्यक्ती);
}
ढाँचा <>
ओळीत bool Ast_गाठ::mayBeUnder<AstVarScope>(स्थिरांक Ast_गाठ* nodep) {
    जर (VN_IS(nodep, VarScope)) परत false;  // Should not nest
    जर (VN_IS(nodep, Var)) परत false;
    जर (VN_IS(nodep, Active)) परत false;
    जर (VN_IS(nodep, गाठ_Stmt)) परत false;
    जर (VN_IS(nodep, गाठ_अभिव्यक्ती)) परत false;
    परत true;
}
ढाँचा <>
ओळीत bool Ast_गाठ::mayBeUnder<AstExecGraph>(स्थिरांक Ast_गाठ* nodep) {
    जर (VN_IS(nodep, ExecGraph)) परत false;  // Should not nest
    जर (VN_IS(nodep, गाठ_Stmt)) परत false;  // Should be directly under CFunc
    परत true;
}
ढाँचा <>
ओळीत bool Ast_गाठ::mayBeUnder<AstActive>(स्थिरांक Ast_गाठ* nodep) {
    परत !VN_IS(nodep, Active);  // AstActives do not nest
}
ढाँचा <>
ओळीत bool Ast_गाठ::mayBeUnder<AstScope>(स्थिरांक Ast_गाठ* nodep) {
    परत !VN_IS(nodep, Scope);  // AstScopes do not nest
}
ढाँचा <>
ओळीत bool Ast_गाठ::mayBeUnder<Ast_Sen_वृक्ष>(स्थिरांक Ast_गाठ* nodep) {
    परत !VN_IS(nodep, Sen_वृक्ष);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
ढाँचा <>
constexpr bool Ast_गाठ::isLeaf<Ast_गाठ_Var_Ref>() {
    परत true;
}
ढाँचा <>
constexpr bool Ast_गाठ::isLeaf<AstVarRef>() {
    परत true;
}
ढाँचा <>
constexpr bool Ast_गाठ::isLeaf<AstVarXRef>() {
    परत true;
}

// foreach implementation
ढाँचा <typename T_Arg, typename T_Callable>
शून्य Ast_गाठ::foreachImpl(Const_Correct_Ast_गाठ<T_Arg>* nodep, स्थिरांक T_Callable& f, bool visitNext) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using गाठ_2 = Const_Correct_Ast_गाठ<T_Arg>;

    // We prefetch this far into the stack
    constexpr पूर्णांक PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr माप_प्रकार MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr माप_प्रकार Initial_चळत_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    माप_प्रकार चळत_Size = Initial_चळत_Size;
    गाठ_2* in_ओळ_चळत[Initial_चळत_Size];  // In-line initial stack storage
    std::unique_ptr<गाठ_2*[]> out_Of_ओळ_Stackp{nullptr};  // In case we need more, we will allocate
    गाठ_2** basep = in_ओळ_चळत + PrefetchDistance;  // Pointer to base of stack
    गाठ_2** topp = basep;  // Pointer to top of stack
    गाठ_2** limp = in_ओळ_चळत + चळत_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    साठी (पूर्णांक i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    स्थिरांक स्वतः visit = [&](गाठ_2* currp) {
        // Type test this node
        जर (Ast_गाठ::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            जर VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) परत;
        }

        // Enqueue children for traversal, unless futile
        जर (mayBeUnder<T_Arg_NonConst>(currp)) {
            जर (Ast_गाठ* स्थिरांक op4p = currp->op4p()) *topp++ = op4p;
            जर (Ast_गाठ* स्थिरांक op3p = currp->op3p()) *topp++ = op3p;
            जर (Ast_गाठ* स्थिरांक op2p = currp->op2p()) *topp++ = op2p;
            जर (Ast_गाठ* स्थिरांक op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    जर (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    पर्यंत (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        गाठ_2* स्थिरांक headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        जर (VL_UNLIKELY(topp >= limp)) {
            स्थिरांक सूचक_भेद_प्रकार occupancy = topp - basep;
            {
                स्थिरांक माप_प्रकार new_चळत_Size = चळत_Size * 2;
                गाठ_2** स्थिरांक newStackp = new गाठ_2*[new_चळत_Size];
                std::स_नकल(newStackp, basep - PrefetchDistance,
                            आकार(गाठ_2**) * (occupancy + PrefetchDistance));
                चळत_Size = new_चळत_Size;
                out_Of_ओळ_Stackp.reset(newStackp);
            }
            basep = out_Of_ओळ_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_ओळ_Stackp.get() + चळत_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        जर (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
ढाँचा <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_गाठ::predicateImpl(Const_Correct_Ast_गाठ<T_Arg>* nodep, स्थिरांक T_Callable& p,
                            bool visitNext) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using गाठ_2 = Const_Correct_Ast_गाठ<T_Arg>;

    // We prefetch this far into the stack
    constexpr पूर्णांक PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr माप_प्रकार MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr माप_प्रकार Initial_चळत_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    माप_प्रकार चळत_Size = Initial_चळत_Size;
    गाठ_2* in_ओळ_चळत[Initial_चळत_Size];  // In-line initial stack storage
    std::unique_ptr<गाठ_2*[]> out_Of_ओळ_Stackp{nullptr};  // In case we need more, we will allocate
    गाठ_2** basep = in_ओळ_चळत + PrefetchDistance;  // Pointer to base of stack
    गाठ_2** topp = basep;  // Pointer to top of stack
    गाठ_2** limp = in_ओळ_चळत + चळत_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    साठी (पूर्णांक i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    स्थिरांक स्वतः visit = [&](गाठ_2* currp) {
        // Type test this node
        जर (Ast_गाठ::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            जर (p(static_cast<T_Arg*>(currp)) != N_Default) परत true;
            // Short circuit if iterating leaf nodes
            जर VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) परत false;
        }

        // Enqueue children for traversal, unless futile
        जर (mayBeUnder<T_Arg_NonConst>(currp)) {
            जर (Ast_गाठ* स्थिरांक op4p = currp->op4p()) *topp++ = op4p;
            जर (Ast_गाठ* स्थिरांक op3p = currp->op3p()) *topp++ = op3p;
            जर (Ast_गाठ* स्थिरांक op2p = currp->op2p()) *topp++ = op2p;
            जर (Ast_गाठ* स्थिरांक op1p = currp->op1p()) *topp++ = op1p;
        }

        परत false;
    };

    // Enqueue the next of the root node, if required
    जर (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    जर (visit(nodep)) परत !N_Default;

    // Visit the rest of the tree
    पर्यंत (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        गाठ_2* स्थिरांक headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        जर (VL_UNLIKELY(topp >= limp)) {
            स्थिरांक सूचक_भेद_प्रकार occupancy = topp - basep;
            {
                स्थिरांक माप_प्रकार new_चळत_Size = चळत_Size * 2;
                गाठ_2** स्थिरांक newStackp = new गाठ_2*[new_चळत_Size];
                std::स_नकल(newStackp, basep - PrefetchDistance,
                            आकार(गाठ_2**) * (occupancy + PrefetchDistance));
                चळत_Size = new_चळत_Size;
                out_Of_ओळ_Stackp.reset(newStackp);
            }
            basep = out_Of_ओळ_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_ओळ_Stackp.get() + चळत_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        जर (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        जर (visit(headp)) परत !N_Default;
    }

    परत N_Default;
}

ओळीत std::ostream& चालक<<(std::ostream& os, स्थिरांक Ast_गाठ* rhs) {
    जर (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } अन्यथा जर (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } अन्यथा {
        rhs->dump(os);
    }
    परत os;
}
शून्य VNRelinker::relink(Ast_गाठ* newp) { newp->Ast_गाठ::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
ढाँचा <typename T_गाठ>
class VNRef final : सार्वजनिक std::reference_wrapper<T_गाठ> {
    static_assert(std::is_base_of<Ast_गाठ, T_गाठ>::मूल्य,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

सार्वजनिक:
    ढाँचा <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_गाठ>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(स्थिरांक std::reference_wrapper<T_गाठ>& other)
        : std::reference_wrapper<T_गाठ>{other} {}
};

static_assert(आकार(VNRef<Ast_गाठ>) == आकार(std::reference_wrapper<Ast_गाठ>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
माप_प्रकार V3HasherUncachedHash(स्थिरांक Ast_गाठ&);

// Specialization of std::hash for VNRef
ढाँचा <typename T_गाठ>
संरचना std::hash<VNRef<T_गाठ>> final {
    माप_प्रकार चालक()(VNRef<T_गाठ> r) स्थिरांक { परत V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
ढाँचा <typename T_गाठ>
संरचना std::equal_to<VNRef<T_गाठ>> final {
    माप_प्रकार चालक()(VNRef<T_गाठ> ra, VNRef<T_गाठ> rb) स्थिरांक {
        परत ra.get().same_वृक्ष(&(rb.get()));
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
    std::unordered_map<स्थिरांक Ast_गाठ*, Ast_गाठ*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<स्थिरांक Ast_गाठ*, std::vector<Ast_गाठ*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<स्थिरांक Ast_गाठ*, std::string> m_prefixes;
    स्थिरांक VDTypeNameScopes* स्थिरांक m_prevp;  // Index in use before this one
    स्थिर स्थिरांक VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    शून्य index(Ast_गाठ* nodep, Ast_गाठ* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

सार्वजनिक:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    स्थिर स्थिरांक VDTypeNameScopes* currentp() { परत s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(स्थिरांक Ast_गाठ* nodep, Ast_गाठ*& outerpr) स्थिरांक;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    स्थिरांक std::vector<Ast_गाठ*>* paramsp(स्थिरांक Ast_गाठ* modp) स्थिरांक;
    // The prefix kept for scope 'scopep', or nullptr if none
    स्थिरांक std::string* prefixp(स्थिरांक Ast_गाठ* scopep) स्थिरांक;
    // Keep 'prefix', final, for scope 'scopep'
    शून्य prefix(स्थिरांक Ast_गाठ* scopep, स्थिरांक std::string& prefix) स्थिरांक;
};

//######################################################################
// Inline VNVisitor METHODS

शून्य VNVisitorConst::iterateConst(Ast_गाठ* nodep) { nodep->accept(*this); }
शून्य VNVisitorConst::iterateConstNull(Ast_गाठ* nodep) {
    जर (VL_LIKELY(nodep)) nodep->accept(*this);
}
शून्य VNVisitorConst::iterateChildrenConst(Ast_गाठ* nodep) { nodep->iterateChildrenConst(*this); }
शून्य VNVisitorConst::iterateChildrenBackwardsConst(Ast_गाठ* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
शून्य VNVisitorConst::iterateAndNextConstNull(Ast_गाठ* nodep) {
    जर (VL_LIKELY(nodep)) nodep->iterateAndNextConst(*this);
}

शून्य VNVisitor::iterate(Ast_गाठ* nodep) { nodep->accept(*this); }
शून्य VNVisitor::iterateNull(Ast_गाठ* nodep) {
    जर (VL_LIKELY(nodep)) nodep->accept(*this);
}
शून्य VNVisitor::iterateChildren(Ast_गाठ* nodep) { nodep->iterateChildren(*this); }
शून्य VNVisitor::iterateAndNextNull(Ast_गाठ* nodep) {
    जर (VL_LIKELY(nodep)) nodep->iterateAndNext(*this);
}
Ast_गाठ* VNVisitor::iterateSubtreeReturnEdits(Ast_गाठ* nodep) {
    परत nodep->iterateSubtreeReturnEdits(*this);
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
शून्य dump_गाठ_List_Json(std::ostream& os, स्थिरांक Ast_गाठ* nodep, स्थिरांक std::string& listName,
                      स्थिरांक string& indent);

#endif  // Guard
