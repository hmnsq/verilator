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
#जइ defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
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
    करेहि { \
        जइ (VL_UNCOVERABLE(test)) प्रत्यागम "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } जाव (false)
// For broken() function, return error string if a base of this class has a match
#define BROKEN_BASE_RTN(test) \
    करेहि { \
        ध्रुव वर्ण* ध्रुव reasonp = (test); \
        जइ (VL_UNCOVERABLE(reasonp)) प्रत्यागम reasonp; \
    } जाव (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#define VN_IS(nodep, nodetypename) (Ast_ग्रन्थिः::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#define VN_CAST(nodep, nodetypename) (Ast_ग्रन्थिः::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#define VN_AS(nodep, nodetypename) (Ast_ग्रन्थिः::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#जइ defined(VL_DEBUG) || defined(VL_ASAN)
#define VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#अण्णहा
#define VN_DBG_AS(nodep, nodetypename) (Ast_ग्रन्थिः::unsafePrivateAs<Ast##nodetypename>(nodep))
#endif

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#define VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

संरचना VNTypeInfo final {
    ध्रुव वर्ण* m_namep;
    गणना OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ध्रुव वर्ण* m_opNamep[4];
    size_t m_sizeof;
};

class VNType final {
    स्थिर ध्रुव VNTypeInfo s_typeInfoTable[];

सार्वजनिक:
#अन्तर्भाव "V3Ast__gen_type_enum.h"  // From ./astgen
    ध्रुव en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr operator en() ध्रुव VL_MT_SAFE { प्रत्यागम m_e; }

    // Retrieve VNTypeInfo for the given type
    स्थिर ध्रुव VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { प्रत्यागम s_typeInfoTable[t.m_e]; }
};
constexpr bool operator==(ध्रुव VNType& lhs, ध्रुव VNType& rhs) VL_PURE {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VNType& lhs, VNType::en rhs) VL_PURE { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VNType::en lhs, ध्रुव VNType& rhs) VL_PURE { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VNType& rhs) { प्रत्यागम os << rhs.ascii(); }

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
#अण्णहा
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
        जइ (i) m_u = i;
    }
    explicit VNUser(शून्य* p) {
        // VNUser{nullptr} represents the monostate
        जइ (p) m_u = p;
    }
    ~VNUser() = स्वभाव;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::मानम्, T>::type to() ध्रुव VL_MT_SAFE {
        जइ (std::holds_alternative<std::monostate>(m_u)) प्रत्यागम nullptr;
        शून्य* ध्रुव* ध्रुव upp = std::get_if<शून्य*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        प्रत्यागम reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() ध्रुव {
        जइ (std::holds_alternative<std::monostate>(m_u)) प्रत्यागम 0;
        ध्रुव uint64_t* ध्रुव uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        प्रत्यागम *uip;
    }
#अण्णहा
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
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::मानम्, T>::type to() ध्रुव VL_MT_SAFE {
        प्रत्यागम reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() ध्रुव { प्रत्यागम m_u.uq; }
#endif
    VSymEnt* toSymEnt() ध्रुव { प्रत्यागम to<VSymEnt*>(); }
    Ast_ग्रन्थिः* toNodep() ध्रुव VL_MT_SAFE { प्रत्यागम to<Ast_ग्रन्थिः*>(); }
    V3GraphVertex* toGraphVertex() ध्रुव { प्रत्यागम to<V3GraphVertex*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(ध्रुव शून्य*)) ध्रुव;
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
स्थिर शून्य clearcnt(पूर्णाङ्क id, uint32_t& cntGblRef, ध्रुव bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
स्थिर शून्य checkcnt(पूर्णाङ्क id, uint32_t&, ध्रुव bool& userBusyRef) {
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
    friend class Ast_ग्रन्थिः;
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
    friend class Ast_ग्रन्थिः;
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
    friend class Ast_ग्रन्थिः;
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
    friend class Ast_ग्रन्थिः;
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
    std::vector<Ast_ग्रन्थिः*> m_deleteps;  // Nodes to delete

सार्वजनिक:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    शून्य प्रक्षिप_Deletep(Ast_ग्रन्थिः* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.प्रक्षिप_back(nodep);
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
    friend class Ast_ग्रन्थिः;

सार्वजनिक:
    /// Call visit()s on nodep
    अन्तःस्थ शून्य iterateConst(Ast_ग्रन्थिः* nodep);
    /// Call visit()s on nodep
    अन्तःस्थ शून्य iterateConstNull(Ast_ग्रन्थिः* nodep);
    /// Call visit()s on const nodep's children
    अन्तःस्थ शून्य iterateChildrenConst(Ast_ग्रन्थिः* nodep);
    /// Call visit()s on nodep's children in backp() order
    अन्तःस्थ शून्य iterateChildrenBackwardsConst(Ast_ग्रन्थिः* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    अन्तःस्थ शून्य iterateAndNextConstNull(Ast_ग्रन्थिः* nodep);

    आभास शून्य visit(Ast_ग्रन्थिः* nodep) = 0;
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
    अन्तःस्थ शून्य iterate(Ast_ग्रन्थिः* nodep);
    /// Call visit()s on nodep
    अन्तःस्थ शून्य iterateNull(Ast_ग्रन्थिः* nodep);
    /// Call visit()s on nodep's children
    अन्तःस्थ शून्य iterateChildren(Ast_ग्रन्थिः* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    अन्तःस्थ शून्य iterateAndNextNull(Ast_ग्रन्थिः* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    अन्तःस्थ Ast_ग्रन्थिः* iterateSubtreeReturnEdits(Ast_ग्रन्थिः* nodep);

    VNDeleter& deleter() { प्रत्यागम m_deleter; }
    शून्य प्रक्षिप_Deletep(Ast_ग्रन्थिः* nodep) { deleter().प्रक्षिप_Deletep(nodep); }
    शून्य doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
रक्षित:
    friend class Ast_ग्रन्थिः;
    गणना RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_NEXT,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_ग्रन्थिः* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_ग्रन्थिः* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_ग्रन्थिः** m_iterpp = nullptr;
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
    अन्तःस्थ शून्य relink(Ast_ग्रन्थिः* newp);
    शून्य dump(std::ostream& str = std::cout) ध्रुव;
};
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VNRelinker& rhs) {
    rhs.dump(os);
    प्रत्यागम os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{सार्वजनिक : आभास bool ग्रन्थिः_Match(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {प्रत्यागम true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#define ASTNODE_PREFETCH_NON_NULL(nodep) \
    करेहि { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } जाव (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#define ASTNODE_PREFETCH(nodep) \
    करेहि { \
        जइ (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } जाव (false)

class Ast_ग्रन्थिः VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_ग्रन्थिः* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_ग्रन्थिः* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_ग्रन्थिः* m_op1p = nullptr;  // Generic pointer 1
    Ast_ग्रन्थिः* m_op2p = nullptr;  // Generic pointer 2
    Ast_ग्रन्थिः* m_op3p = nullptr;  // Generic pointer 3
    Ast_ग्रन्थिः* m_op4p = nullptr;  // Generic pointer 4
    Ast_ग्रन्थिः** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    ध्रुव VNType m_type;  // Node sub-type identifier
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

#जइ defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(परिमाण(m_type) + परिमाण(m_flags) + परिमाण(m_brokenState) + परिमाण(m_cloneCnt)
                      <= परिमाण(शून्य*),
                  "packing requires padding");
#endif

    Ast_ग्रन्थिः_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_ग्रन्थिः* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_पङ्क्तिः* m_fileline;  // Where it was declared
#ifdef VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_गणनम्;  // When it was last edited
#endif
    स्थिर uint64_t s_editCntGbl;  // Global edit counter
    स्थिर uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_ग्रन्थिः* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
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
    शून्य op1p(Ast_ग्रन्थिः* nodep) {
        m_op1p = nodep;
        जइ (nodep) nodep->m_backp = this;
    }
    शून्य op2p(Ast_ग्रन्थिः* nodep) {
        m_op2p = nodep;
        जइ (nodep) nodep->m_backp = this;
    }
    शून्य op3p(Ast_ग्रन्थिः* nodep) {
        m_op3p = nodep;
        जइ (nodep) nodep->m_backp = this;
    }
    शून्य op4p(Ast_ग्रन्थिः* nodep) {
        m_op4p = nodep;
        जइ (nodep) nodep->m_backp = this;
    }

गुप्त:
    Ast_ग्रन्थिः* clone_वृक्षः_Iter(bool needPure);
    Ast_ग्रन्थिः* clone_वृक्षः_Iter_List(bool needPure);
    शून्य check_वृक्षः_Iter(ध्रुव Ast_ग्रन्थिः* prevBackp) ध्रुव VL_MT_STABLE;
    bool gate_वृक्षः_Iter() ध्रुव;
    स्थिर bool same_वृक्षः_Iter(ध्रुव Ast_ग्रन्थिः* node1p, ध्रुव Ast_ग्रन्थिः* node2p, bool ignNext,
                             bool gateOnly);
    शून्य delete_वृक्षः_Iter();
    शून्य delete_ग्रन्थिः();
    string instanceStr() ध्रुव;

सार्वजनिक:
    शून्य purityCheck();
    स्थिर शून्य relinkOneLink(Ast_ग्रन्थिः*& pointpr, Ast_ग्रन्थिः* newp);
    // cppcheck-suppress functionConst
    स्थिर शून्य debug_वृक्षः_Change(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* prefix, पूर्णाङ्क lineno, bool next);

रक्षित:
    // CONSTRUCTORS
    Ast_ग्रन्थिः(VNType t, File_पङ्क्तिः* fl);
    आभास ~Ast_ग्रन्थिः() = स्वभाव;  // Use 'deleteTree' instead
    आभास Ast_ग्रन्थिः* clone() = 0;  // Generally, cloneTree is what you want instead
    आभास शून्य cloneRelink() { cloneRelinkGen(); }
    आभास शून्य cloneRelinkGen() {};  // Overrides generated by 'astgen'
    शून्य clone_Relink_वृक्षः();

    // METHODS
    शून्य setOp1p(Ast_ग्रन्थिः* newp);  // Set non-list-type op1 to non-list element
    शून्य setOp2p(Ast_ग्रन्थिः* newp);  // Set non-list-type op2 to non-list element
    शून्य setOp3p(Ast_ग्रन्थिः* newp);  // Set non-list-type op3 to non-list element
    शून्य setOp4p(Ast_ग्रन्थिः* newp);  // Set non-list-type op4 to non-list element

    शून्य addOp1p(Ast_ग्रन्थिः* newp);  // Append newp to end of op1
    शून्य addOp2p(Ast_ग्रन्थिः* newp);  // Append newp to end of op2
    शून्य addOp3p(Ast_ग्रन्थिः* newp);  // Append newp to end of op3
    शून्य addOp4p(Ast_ग्रन्थिः* newp);  // Append newp to end of op4

    शून्य prependOp1p(Ast_ग्रन्थिः* newp);  // Prepend newp to front of op1
    शून्य prependOp2p(Ast_ग्रन्थिः* newp);  // Prepend newp to front of op2
    शून्य prependOp3p(Ast_ग्रन्थिः* newp);  // Prepend newp to front of op3
    शून्य prependOp4p(Ast_ग्रन्थिः* newp);  // Prepend newp to front of op4

    // clang-format off
    शून्य setNOp1p(Ast_ग्रन्थिः* newp) { जइ (newp) setOp1p(newp); }
    शून्य setNOp2p(Ast_ग्रन्थिः* newp) { जइ (newp) setOp2p(newp); }
    शून्य setNOp3p(Ast_ग्रन्थिः* newp) { जइ (newp) setOp3p(newp); }
    शून्य setNOp4p(Ast_ग्रन्थिः* newp) { जइ (newp) setOp4p(newp); }

    शून्य addNOp1p(Ast_ग्रन्थिः* newp) { जइ (newp) addOp1p(newp); }
    शून्य addNOp2p(Ast_ग्रन्थिः* newp) { जइ (newp) addOp2p(newp); }
    शून्य addNOp3p(Ast_ग्रन्थिः* newp) { जइ (newp) addOp3p(newp); }
    शून्य addNOp4p(Ast_ग्रन्थिः* newp) { जइ (newp) addOp4p(newp); }

    शून्य prependNOp1p(Ast_ग्रन्थिः* newp) { जइ (newp) prependOp1p(newp); }
    शून्य prependNOp2p(Ast_ग्रन्थिः* newp) { जइ (newp) prependOp2p(newp); }
    शून्य prependNOp3p(Ast_ग्रन्थिः* newp) { जइ (newp) prependOp3p(newp); }
    शून्य prependNOp4p(Ast_ग्रन्थिः* newp) { जइ (newp) prependOp4p(newp); }
    // clang-format on

    शून्य clonep(Ast_ग्रन्थिः* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    स्थिर शून्य clone_Clear_वृक्षः() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    आभास bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव { प्रत्यागम true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    आभास bool wouldBreakGen(ध्रुव Ast_ग्रन्थिः* ध्रुव oldp,
                               ध्रुव Ast_ग्रन्थिः* ध्रुव newp) ध्रुव
        = 0;  // Generated by 'astgen'

सार्वजनिक:
    // ACCESSORS
    VNType type() ध्रुव VL_MT_SAFE { प्रत्यागम m_type; }
    ध्रुव वर्ण* typeName() ध्रुव VL_MT_SAFE { प्रत्यागम type().ascii(); }  // See also prettyTypeName
    Ast_ग्रन्थिः* nextp() ध्रुव VL_MT_STABLE { प्रत्यागम m_nextp; }
    Ast_ग्रन्थिः* backp() ध्रुव VL_MT_STABLE { प्रत्यागम m_backp; }
    Ast_ग्रन्थिः* abovep() ध्रुव;  // Get parent node above, only for list head and tail
    Ast_ग्रन्थिः* aboveLoopp() ध्रुव;  // Get parent node above, may have performance issues as loops
    Ast_ग्रन्थिः* lastp() ध्रुव {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        प्रत्यागम m_headtailp;
    }
    Ast_ग्रन्थिः* prevp() ध्रुव {  // Previous node in list, nullptr for head of list
        जइ (m_backp->m_nextp != this) प्रत्यागम nullptr;
        प्रत्यागम m_backp;
    }
    Ast_ग्रन्थिः* op1p() ध्रुव VL_MT_STABLE { प्रत्यागम m_op1p; }
    Ast_ग्रन्थिः* op2p() ध्रुव VL_MT_STABLE { प्रत्यागम m_op2p; }
    Ast_ग्रन्थिः* op3p() ध्रुव VL_MT_STABLE { प्रत्यागम m_op3p; }
    Ast_ग्रन्थिः* op4p() ध्रुव VL_MT_STABLE { प्रत्यागम m_op4p; }
    Ast_ग्रन्थिः_DType* dtypep() ध्रुव VL_MT_STABLE { प्रत्यागम m_dtypep; }
    Ast_ग्रन्थिः* clonep() ध्रुव { प्रत्यागम ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_ग्रन्थिः* firstAbovep() ध्रुव {  // Returns nullptr when second or later in list
        प्रत्यागम ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ध्रुव VL_MT_SAFE { प्रत्यागम m_brokenState; }
    शून्य brokenState(uint8_t मानम्) { m_brokenState = मानम्; }

    // Used by AstNode::broken()
    bool brokeExists() ध्रुव { प्रत्यागम V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ध्रुव { प्रत्यागम brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    आभास शून्य foreachLink(std::function<शून्य(Ast_ग्रन्थिः** linkpp, ध्रुव वर्ण* namep)> f) = 0;

#जइ defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    स्थिर शून्य* operator new(size_t size);
    स्थिर शून्य operator delete(शून्य* obj, size_t size);
#endif

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_BRANCH = 4;  // Branch
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_CALL = INSTR_गणनम्_BRANCH + 10;  // Subroutine call
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_LD = 2;  // Load memory
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_INT_MUL = 3;  // Integer multiply
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_INT_DIV = 10;  // Integer divide
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_DBL = 8;  // Convert or do float ops
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_DBL_DIV = 40;  // Double divide
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_DBL_TRIG = 200;  // Double trigonometric ops
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_STR = 100;  // String ops
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_TIME = INSTR_गणनम्_CALL + 5;  // Determine simulation time
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_PLI = 20;  // PLI routines
    स्थिर constexpr पूर्णाङ्क INSTR_गणनम्_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    आभास string name() ध्रुव VL_MT_STABLE { प्रत्यागम ""; }
    आभास string origName() ध्रुव { प्रत्यागम ""; }
    string prettyDehashOrigOrName() ध्रुव {
        प्रत्यागम prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    आभास शून्य name(ध्रुव string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    आभास शून्य tag(ध्रुव string& text) {}
    आभास string tag() ध्रुव { प्रत्यागम ""; }
    आभास uint32_t decl_Token_अङ्कः() ध्रुव { प्रत्यागम 0; }
    आभास शून्य decl_Token_अङ्कः_Set_Min(uint32_t token_अङ्कः) {}
    आभास string verilogKwd() ध्रुव { प्रत्यागम ""; }
    string nameProtect() ध्रुव VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() ध्रुव;  // origName with --protect-id applied
    string shortName() ध्रुव;  // Name with __PVT__ removed for concatenating scopes
    स्थिर string dedotName(ध्रुव string& namein);  // Name with dots removed
    स्थिर string nameNoArray(ध्रुव string& namein);  // Name with any array index removed
    स्थिर string prettyName(ध्रुव string& namein) VL_PURE;  // Name for printing out to the user
    स्थिर string vpiName(ध्रुव string& namein);  // Name for vpi access
    स्थिर string prettyNameQ(ध्रुव string& namein) {  // Quoted pretty name (for errors)
        प्रत्यागम "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    स्थिर string encodeName(ध्रुव string& namein);
    स्थिर string encode_सङ्ख्या(int64_t अङ्कः);  // Encode number into internal C representation
    स्थिर string vcdName(ध्रुव string& namein);  // Name for printing out to vcd files
    string prettyName() ध्रुव { प्रत्यागम prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    आभास string pretty_Name_सन्देशः() ध्रुव { प्रत्यागम prettyName(); }
    string prettyNameQ() ध्रुव { प्रत्यागम "'"s + pretty_Name_सन्देशः() + "'"; }  // Quoted, for messages
    string verilogName() ध्रुव { प्रत्यागम vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ध्रुव;
    आभास string prettyOperatorName() ध्रुव { प्रत्यागम "operator " + prettyTypeName(); }
    File_पङ्क्तिः* fileline() ध्रुव VL_MT_SAFE { प्रत्यागम m_fileline; }
    शून्य fileline(File_पङ्क्तिः* fl) { m_fileline = fl; }
    अन्तःस्थ bool width1() ध्रुव;
    अन्तःस्थ पूर्णाङ्क widthInstrs() ध्रुव;
    bool didWidth() ध्रुव { प्रत्यागम m_flags.didWidth; }
    शून्य didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        जइ (didWidth()) प्रत्यागम true;
        didWidth(true);
        प्रत्यागम false;
    }
    bool doingWidth() ध्रुव { प्रत्यागम m_flags.doingWidth; }
    शून्य doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() ध्रुव VL_MT_SAFE { प्रत्यागम m_flags.protect; }
    शून्य protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    अन्तःस्थ पूर्णाङ्क width() ध्रुव VL_MT_STABLE;
    अन्तःस्थ पूर्णाङ्क widthMin() ध्रुव VL_MT_STABLE;
    पूर्णाङ्क widthMinV() ध्रुव {
        प्रत्यागम v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    पूर्णाङ्क width_शब्दाः() ध्रुव { प्रत्यागम VL_शब्दाः_I(width()); }
    bool isQuad() ध्रुव VL_MT_STABLE { प्रत्यागम (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() ध्रुव VL_MT_STABLE { प्रत्यागम (width() > VL_QUADSIZE); }
    अन्तःस्थ bool isCHandle() ध्रुव VL_MT_STABLE;
    अन्तःस्थ bool isDouble() ध्रुव VL_MT_STABLE;
    अन्तःस्थ bool isSigned() ध्रुव VL_MT_STABLE;
    अन्तःस्थ bool isString() ध्रुव VL_MT_STABLE;
    अन्तःस्थ bool isEvent() ध्रुव VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ध्रुव VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        प्रत्यागम ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_ग्रन्थिः* user1p() ध्रुव VL_MT_STABLE { प्रत्यागम user1u().toNodep(); }
    शून्य user1u(ध्रुव VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    शून्य user1p(शून्य* userp) { user1u(VNUser{userp}); }
    शून्य user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ध्रुव { प्रत्यागम user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ध्रुव uint64_t v = user1(); user1(v + val); प्रत्यागम v; }
    uint64_t user1Or(uint64_t val) { ध्रुव uint64_t v = user1(); user1(v | val); प्रत्यागम v; }
    uint64_t user1SetOnce() { ध्रुव uint64_t v = user1(); जइ (!v) user1(1); प्रत्यागम v; }  // Better for cache than user1Inc()
    स्थिर शून्य user1_Clear_वृक्षः() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ध्रुव VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        प्रत्यागम ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_ग्रन्थिः* user2p() ध्रुव VL_MT_STABLE { प्रत्यागम user2u().toNodep(); }
    शून्य user2u(ध्रुव VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    शून्य user2p(शून्य* userp) { user2u(VNUser{userp}); }
    शून्य user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ध्रुव { प्रत्यागम user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ध्रुव uint64_t v = user2(); user2(v + val); प्रत्यागम v; }
    uint64_t user2Or(uint64_t val) { ध्रुव uint64_t v = user2(); user2(v | val); प्रत्यागम v; }
    uint64_t user2SetOnce() { ध्रुव uint64_t v = user2(); जइ (!v) user2(1); प्रत्यागम v; }  // Better for cache than user2Inc()
    स्थिर शून्य user2_Clear_वृक्षः() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ध्रुव VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        प्रत्यागम ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_ग्रन्थिः* user3p() ध्रुव VL_MT_STABLE { प्रत्यागम user3u().toNodep(); }
    शून्य user3u(ध्रुव VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    शून्य user3p(शून्य* userp) { user3u(VNUser{userp}); }
    शून्य user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ध्रुव { प्रत्यागम user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ध्रुव uint64_t v = user3(); user3(v + val); प्रत्यागम v; }
    uint64_t user3Or(uint64_t val) { ध्रुव uint64_t v = user3(); user3(v | val); प्रत्यागम v; }
    uint64_t user3SetOnce() { ध्रुव uint64_t v = user3(); जइ (!v) user3(1); प्रत्यागम v; }  // Better for cache than user3Inc()
    स्थिर शून्य user3_Clear_वृक्षः() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ध्रुव VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        प्रत्यागम ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_ग्रन्थिः* user4p() ध्रुव VL_MT_STABLE { प्रत्यागम user4u().toNodep(); }
    शून्य user4u(ध्रुव VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    शून्य user4p(शून्य* userp) { user4u(VNUser{userp}); }
    शून्य user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ध्रुव { प्रत्यागम user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { ध्रुव uint64_t v = user4(); user4(v | val); प्रत्यागम v; }
    uint64_t user4Inc(uint64_t val = 1) { ध्रुव uint64_t v = user4(); user4(v + val); प्रत्यागम v; }
    uint64_t user4SetOnce() { ध्रुव uint64_t v = user4(); जइ (!v) user4(1); प्रत्यागम v; }  // Better for cache than user4Inc()
    स्थिर शून्य user4_Clear_वृक्षः() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ifdef VL_DEBUG
    uint64_t edit_गणनम्() ध्रुव { प्रत्यागम m_edit_गणनम्; }
    शून्य edit_गणनम्_Inc() {
        m_edit_गणनम् = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_वृक्षः();  // Any edit clears all caching
    }
#अण्णहा
    शून्य edit_गणनम्_Inc() { ++s_editCntGbl; }
#endif
    स्थिर uint64_t edit_गणनम्_Last() VL_MT_SAFE { प्रत्यागम s_editCntLast; }
    स्थिर uint64_t edit_गणनम्_Gbl() VL_MT_SAFE { प्रत्यागम s_editCntGbl; }
    स्थिर शून्य edit_गणनम्_Set_Last() { s_editCntLast = edit_गणनम्_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_प्रक्षिप_Self_Stmt();
    अन्तःस्थ bool is_Class_Handle_मानम्() ध्रुव;
    अन्तःस्थ bool isNull() ध्रुव;
    अन्तःस्थ bool isZero() ध्रुव;
    अन्तःस्थ bool isOne() ध्रुव;
    अन्तःस्थ bool isNeqZero() ध्रुव;
    अन्तःस्थ bool isAllOnes() ध्रुव;
    अन्तःस्थ bool isAllOnesV() ध्रुव;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    शून्य dtypep(Ast_ग्रन्थिः_DType* nodep) {
        जइ (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_गणनम्_Inc();
        }
    }
    शून्य dtypeFrom(ध्रुव Ast_ग्रन्थिः* fromp) {
        जइ (fromp) dtypep(fromp->dtypep());
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
    Ast_ग्रन्थिः_DType* findBitDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_ग्रन्थिः_DType* findLogicDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_ग्रन्थिः_DType* findNullDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_ग्रन्थिः_DType* findLogic2StateDType() ध्रुव {
        प्रत्यागम findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_ग्रन्थिः_DType* findDoubleDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_ग्रन्थिः_DType* findIntDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::INT); }
    Ast_ग्रन्थिः_DType* findIntegerDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_ग्रन्थिः_DType* findInteger2StateDType() ध्रुव {
        प्रत्यागम findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_ग्रन्थिः_DType* findStringDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_ग्रन्थिः_DType* findSigned8DType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_ग्रन्थिः_DType* findUInt32DType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_ग्रन्थिः_DType* findUInt64DType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_ग्रन्थिः_DType* findTimeDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_ग्रन्थिः_DType* findTime2StateDType() ध्रुव {
        प्रत्यागम findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_ग्रन्थिः_DType* findCHandleDType() ध्रुव { प्रत्यागम findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_ग्रन्थिः_DType* findConstraintRefDType() ध्रुव;
    Ast_ग्रन्थिः_DType* findEmptyQueueDType() ध्रुव;
    Ast_ग्रन्थिः_DType* findQueueIndexDType() ध्रुव;
    Ast_ग्रन्थिः_DType* findStreamDType() ध्रुव;
    Ast_ग्रन्थिः_DType* findVoidDType() ध्रुव;
    Ast_ग्रन्थिः_DType* findBitDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) ध्रुव;
    Ast_ग्रन्थिः_DType* findLogicDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) ध्रुव;
    Ast_ग्रन्थिः_DType* findLogic2StateDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) ध्रुव;
    Ast_ग्रन्थिः_DType* findBitOrLogicDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric,
                                      bool isFourstate) ध्रुव {
        प्रत्यागम isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_ग्रन्थिः_DType* findBitOrLogicRangeDType(ध्रुव VNumRange& range, ध्रुव पूर्णाङ्क widthMin,
                                           ध्रुव VSigning numeric,
                                           ध्रुव bool isFourstate) ध्रुव VL_MT_STABLE {
        प्रत्यागम isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_ग्रन्थिः_DType* findLogicRangeDType(ध्रुव VNumRange& range, पूर्णाङ्क widthMin,
                                      VSigning numeric) ध्रुव VL_MT_STABLE;
    Ast_ग्रन्थिः_DType* findBitRangeDType(ध्रुव VNumRange& range, पूर्णाङ्क widthMin,
                                    VSigning numeric) ध्रुव VL_MT_STABLE;
    Ast_ग्रन्थिः_DType* findBasicDType(VBasicDTypeKwd kwd) ध्रुव;
    स्थिर AstBasicDType* find_निवेशय_Same_DType(AstBasicDType* nodep);

    स्थिर VCastable computeCastable(ध्रुव Ast_ग्रन्थिः_DType* toDtp, ध्रुव Ast_ग्रन्थिः_DType* fromDtp,
                                     ध्रुव Ast_ग्रन्थिः* fromConstp, bool checkIfaceArgCompat = false);
    स्थिर Ast_ग्रन्थिः_DType* getCommonClassTypep(Ast_ग्रन्थिः* nodep1, Ast_ग्रन्थिः* nodep2);

    // METHODS - dump and error
    शून्य v3errorEnd(ध्रुव std::ostringstream& str) ध्रुव VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(ध्रुव std::ostringstream& str) ध्रुव VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) {
        प्रत्यागम fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ध्रुव { प्रत्यागम fileline()->warnContextSecondary(); }
    string warnMore() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) { प्रत्यागम fileline()->warnMore(); }
    string warnOther() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) { प्रत्यागम fileline()->warnOther(); }

    आभास शून्य dump(std::ostream& str = std::cout) ध्रुव;
    स्थिर वर्ण* dump_वृक्षः_Json_Gdb(ध्रुव Ast_ग्रन्थिः* nodep);  // For GDB only, free()d by caller
    स्थिर वर्ण* dump_वृक्षः_Json_Gdb(ध्रुव वर्ण* str);  // For GDB only, free()d by caller
    स्थिर वर्ण* dump_वृक्षः_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    स्थिर शून्य dumpGdb(ध्रुव Ast_ग्रन्थिः* nodep);  // For GDB only
    शून्य dumpGdbHeader() ध्रुव;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    template <typename T_ग्रन्थिः_Result, typename T_ग्रन्थिः_Next>
    स्थिर T_ग्रन्थिः_Result* addNext(T_ग्रन्थिः_Result* nodep, T_ग्रन्थिः_Next* newp) {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः_Result>::मानम्,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_ग्रन्थिः_Result, T_ग्रन्थिः_Next>::मानम्,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        जइ (!newp) प्रत्यागम nodep;
        प्रत्यागम static_cast<T_ग्रन्थिः_Result*>(addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, newp));
    }
    template <typename T_ग्रन्थिः_Result, typename T_ग्रन्थिः_Next>
    स्थिर T_ग्रन्थिः_Result* addNextNull(T_ग्रन्थिः_Result* nodep, T_ग्रन्थिः_Next* newp) {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः_Result>::मानम्,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_ग्रन्थिः_Result, T_ग्रन्थिः_Next>::मानम्,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        जइ (!newp) प्रत्यागम nodep;
        जइ (!nodep) प्रत्यागम newp;
        प्रत्यागम static_cast<T_ग्रन्थिः_Result*>(addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, newp));
    }
    अन्तःस्थ Ast_ग्रन्थिः* addNext(Ast_ग्रन्थिः* newp);
    शून्य addNextHere(Ast_ग्रन्थिः* newp);  // Insert newp at this->nextp
    शून्य addHereThisAsNext(Ast_ग्रन्थिः* newp);  // Adds at old place of this, this becomes next
    शून्य replaceWith(Ast_ग्रन्थिः* newp);  // Replace current node in tree with new node
    शून्य replaceWithKeepDType(Ast_ग्रन्थिः* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_ग्रन्थिः* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_ग्रन्थिः* unlinkFrBackWithNext(VNRelinker* linkerp = nullptr);
    शून्य relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    शून्य clone_Relink_ग्रन्थिः() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_ग्रन्थिः* clone_वृक्षः(bool cloneNextLink,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_ग्रन्थिः* clone_वृक्षः_Pure(bool cloneNextLink) { प्रत्यागम clone_वृक्षः(cloneNextLink, true); }
    bool gate_वृक्षः() { प्रत्यागम gate_वृक्षः_Iter(); }  // Is tree isGateOptimizable?
    अन्तःस्थ bool same_वृक्षः(ध्रुव Ast_ग्रन्थिः* node2p) ध्रुव;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    अन्तःस्थ bool same_Gate_वृक्षः(ध्रुव Ast_ग्रन्थिः* node2p) ध्रुव;
    शून्य delete_वृक्षः();  // Always deletes the next link
    शून्य check_वृक्षः() ध्रुव VL_MT_STABLE {
        जइ (v3Global.opt.debugCheck()) check_वृक्षः_Iter(backp());
    }
    शून्य checkIter() ध्रुव;
    शून्य dumpPtrs(std::ostream& os = std::cout) ध्रुव;
    शून्य dump_वृक्षः(std::ostream& os = std::cout, ध्रुव string& indent = "    ",
                  पूर्णाङ्क maxDepth = 0) ध्रुव;
    शून्य dump_वृक्षः(ध्रुव string& indent, पूर्णाङ्क maxDepth = 0) ध्रुव {
        dump_वृक्षः(std::cout, indent, maxDepth);
    }
    स्थिर शून्य dump_वृक्षः_Gdb(ध्रुव Ast_ग्रन्थिः* nodep);  // For GDB only
    शून्य dump_वृक्षः_And_Next(std::ostream& os = std::cout, ध्रुव string& indent = "    ",
                         पूर्णाङ्क maxDepth = 0) ध्रुव;
    शून्य dump_वृक्षः_File(ध्रुव string& filename, bool doDump = true);
    स्थिर शून्य dump_वृक्षः_File_Gdb(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* filenamep = nullptr);
    शून्य dump_वृक्षः_Dot(std::ostream& os = std::cout) ध्रुव;
    शून्य dump_वृक्षः_Dot_File(ध्रुव string& filename, bool doDump = true);
    आभास शून्य dumpJson(std::ostream& os) ध्रुव { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    शून्य dumpJsonGen(std::ostream& os) ध्रुव {};
    आभास शून्य dump_वृक्षः_Json_Op_Gen(std::ostream& os, ध्रुव string& indent) ध्रुव {};
    शून्य dump_वृक्षः_Json(std::ostream& os, ध्रुव string& indent = "") ध्रुव;
    शून्य dump_वृक्षः_Json_File(ध्रुव string& filename, bool doDump = true);
    स्थिर शून्य dumpJsonMetaFileGdb(ध्रुव वर्ण* filename);
    स्थिर शून्य dumpJsonMetaFile(ध्रुव string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    स्थिर std::string ग्रन्थिः_Addr(ध्रुव Ast_ग्रन्थिः* nodep) {
        प्रत्यागम v3Global.opt.dump_वृक्षः_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    स्थिर Ast_ग्रन्थिः* afterCommentp(Ast_ग्रन्थिः* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        जाव (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        प्रत्यागम nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    आभास bool isBrancher() ध्रुव { प्रत्यागम false; }
    // Else a AstTime etc that can't be pushed out
    आभास bool isGateOptimizable() ध्रुव { प्रत्यागम !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    आभास bool isGateDedupable() ध्रुव { प्रत्यागम isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    आभास bool is_व्यञ्जकम्_Coverage_Eligible() ध्रुव { प्रत्यागम isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    आभास bool isOutputter() { प्रत्यागम false; }
    // Else a AstTime etc which output can't be predicted from input
    आभास bool isPredictOptimizable() ध्रुव { प्रत्यागम !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    आभास bool isPure() { प्रत्यागम true; }
    // Iff isPure on current node and any nextp()
    bool isPureAndNext() { प्रत्यागम isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    आभास bool isTimingControl() ध्रुव { प्रत्यागम false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    आभास bool isUnlikely() ध्रुव { प्रत्यागम false; }
    // Is an IEEE system function (versus internally-generated)
    आभास bool isSystemFunc() ध्रुव { प्रत्यागम false; }
    आभास पूर्णाङ्क instr_गणनम्() ध्रुव { प्रत्यागम 0; }
    // Iff node is identical to another node
    आभास bool isSame(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव {
        प्रत्यागम type() == samep->type() && same_ग्रन्थिः(samep);
    }
    // Iff has a data type; dtype() must be non-null
    आभास bool hasDType() ध्रुव VL_MT_SAFE { प्रत्यागम false; }
    // Iff has a non-null childDTypep(), as generic node function
    आभास Ast_ग्रन्थिः_DType* getChildDTypep() ध्रुव { प्रत्यागम nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    आभास Ast_ग्रन्थिः_DType* getChild2DTypep() ध्रुव { प्रत्यागम nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    आभास bool maybePointedTo() ध्रुव VL_MT_SAFE { प्रत्यागम false; }
    // Don't reclaim this node in V3Dead
    आभास bool undead() ध्रुव { प्रत्यागम false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    आभास ध्रुव वर्ण* broken() ध्रुव { प्रत्यागम nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    आभास ध्रुव वर्ण* brokenGen() ध्रुव = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ध्रुव Ast_ग्रन्थिः* ध्रुव newp) ध्रुव { प्रत्यागम backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    आभास शून्य accept(VNVisitorConst& v) = 0;

रक्षित:
    // All VNVisitor related functions are called as methods off the visitor
    friend class VNVisitor;
    friend class VNVisitorConst;
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
    Ast_ग्रन्थिः* iterateSubtreeReturnEdits(VNVisitor& v);

    स्थिर शून्य dump_Json_अङ्कः(std::ostream& os, ध्रुव std::string& name, int64_t val);
    स्थिर शून्य dumpJsonBoolIf(std::ostream& os, ध्रुव std::string& name, bool val);
    स्थिर शून्य dumpJsonStr(std::ostream& os, ध्रुव std::string& name, ध्रुव std::string& val);
    स्थिर शून्य dumpJsonPtr(std::ostream& os, ध्रुव std::string& name, ध्रुव Ast_ग्रन्थिः* ध्रुव valp);

रक्षित:
    शून्य iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    template <typename T>
    अन्तःस्थ स्थिर bool privateTypeTest(ध्रुव Ast_ग्रन्थिः* nodep);

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr स्थिर bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        प्रत्यागम std::is_base_of<T_TargetType, NonCV>::मानम्;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr स्थिर bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        प्रत्यागम !std::is_base_of<NonCV, T_TargetType>::मानम्;
    }  // LCOV_EXCL_STOP

सार्वजनिक:
    // For use via the VN_IS macro only, or in templated code
    template <typename T_TargetType, typename T_ग्रन्थिः>
    स्थिर bool is(ध्रुव T_ग्रन्थिः* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        प्रत्यागम nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    template <typename T_TargetType, typename T_ग्रन्थिः>
    स्थिर T_TargetType* cast(T_ग्रन्थिः* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        प्रत्यागम nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    template <typename T_TargetType, typename T_ग्रन्थिः>
    स्थिर ध्रुव T_TargetType* cast(ध्रुव T_ग्रन्थिः* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        प्रत्यागम nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<ध्रुव T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    template <typename T_TargetType, typename T_ग्रन्थिः>
    स्थिर T_TargetType* as(T_ग्रन्थिः* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        प्रत्यागम unsafePrivateAs<T_TargetType, T_ग्रन्थिः>(nodep);
    }
    template <typename T_TargetType, typename T_ग्रन्थिः>
    स्थिर ध्रुव T_TargetType* as(ध्रुव T_ग्रन्थिः* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        प्रत्यागम unsafePrivateAs<T_TargetType, T_ग्रन्थिः>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    template <typename T_TargetType, typename T_ग्रन्थिः>
    स्थिर T_TargetType* unsafePrivateAs(T_ग्रन्थिः* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        प्रत्यागम reinterpret_cast<T_TargetType*>(nodep);
    }
    template <typename T_TargetType, typename T_ग्रन्थिः>
    स्थिर ध्रुव T_TargetType* unsafePrivateAs(ध्रुव T_ग्रन्थिः* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ग्रन्थिः>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        प्रत्यागम reinterpret_cast<ध्रुव T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_ग्रन्थिः>
    स्थिर bool mayBeUnder(ध्रुव Ast_ग्रन्थिः* nodep) {
        static_assert(!std::is_const<T_ग्रन्थिः>::मानम्,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        प्रत्यागम true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_ग्रन्थिः>
    स्थिर constexpr bool isLeaf() {
        static_assert(!std::is_const<T_ग्रन्थिः>::मानम्,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        प्रत्यागम false;
    }

गुप्त:
    // Using std::conditional for const correctness in the public 'foreach' functions
    template <typename T_Arg>
    using Const_Correct_Ast_ग्रन्थिः =
        typename std::conditional<std::is_const<T_Arg>::मानम्, ध्रुव Ast_ग्रन्थिः, Ast_ग्रन्थिः>::type;

    template <typename T_Arg, typename T_Callable>
    अन्तःस्थ स्थिर शून्य foreachImpl(Const_Correct_Ast_ग्रन्थिः<T_Arg>* nodep, ध्रुव T_Callable& f,
                                   bool visitNext);

    template <typename T_Arg, bool N_Default, typename T_Callable>
    अन्तःस्थ स्थिर bool predicateImpl(Const_Correct_Ast_ग्रन्थिः<T_Arg>* nodep, ध्रुव T_Callable& p,
                                     bool visitNext);

सार्वजनिक:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    template <typename T_Callable>
    शून्य foreach(T_Callable&& f) {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_ग्रन्थिः*>::मानम्
                          && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_ग्रन्थिः>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    शून्य foreach(T_Callable&& f) ध्रुव {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ध्रुव T_ग्रन्थिः*>::मानम्
                && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ध्रुव T_ग्रन्थिः>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    शून्य foreachAndNext(T_Callable&& f) {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_ग्रन्थिः*>::मानम्
                          && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_ग्रन्थिः>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    शून्य foreachAndNext(T_Callable&& f) ध्रुव {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ध्रुव T_ग्रन्थिः*>::मानम्
                && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ध्रुव T_ग्रन्थिः>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    template <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_ग्रन्थिः*>::मानम्
                          && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        प्रत्यागम predicateImpl<T_ग्रन्थिः, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool exists(T_Callable&& p) ध्रुव {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ध्रुव T_ग्रन्थिः*>::मानम्
                          && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        प्रत्यागम predicateImpl<ध्रुव T_ग्रन्थिः, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ध्रुव T_ग्रन्थिः*>::मानम्
                          && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        प्रत्यागम predicateImpl<T_ग्रन्थिः, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) ध्रुव {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ध्रुव T_ग्रन्थिः*>::मानम्
                          && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        प्रत्यागम predicateImpl<ध्रुव T_ग्रन्थिः, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    template <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_ग्रन्थिः*>::मानम्
                          && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        प्रत्यागम predicateImpl<T_ग्रन्थिः, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool forall(T_Callable&& p) ध्रुव {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ध्रुव T_ग्रन्थिः*>::मानम्
                          && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        प्रत्यागम predicateImpl<ध्रुव T_ग्रन्थिः, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    पूर्णाङ्क ग्रन्थिः_गणनम्() ध्रुव {
        // TODO: this should really return size_t, but need to fix use sites
        पूर्णाङ्क गणनम् = 0;
        this->foreach([&गणनम्](ध्रुव Ast_ग्रन्थिः*) { ++गणनम्; });
        प्रत्यागम गणनम्;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(पूर्णाङ्क limit) ध्रुव {
        पूर्णाङ्क गणनम् = 0;
        प्रत्यागम this->exists([&गणनम्, limit](ध्रुव Ast_ग्रन्थिः*) { प्रत्यागम ++गणनम् > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
template <>
Ast_ग्रन्थिः* Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* newp);

// Inline method implementations
Ast_ग्रन्थिः* Ast_ग्रन्थिः::addNext(Ast_ग्रन्थिः* newp) { प्रत्यागम addNext(this, newp); }

// Specializations of privateTypeTest
#अन्तर्भाव "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
template <>
अन्तःस्थ bool Ast_ग्रन्थिः::mayBeUnder<AstCell>(ध्रुव Ast_ग्रन्थिः* nodep) {
    प्रत्यागम !VN_IS(nodep, ग्रन्थिः_Stmt) && !VN_IS(nodep, ग्रन्थिः_व्यञ्जकम्);
}
template <>
अन्तःस्थ bool Ast_ग्रन्थिः::mayBeUnder<Ast_ग्रन्थिः_Assign>(ध्रुव Ast_ग्रन्थिः* nodep) {
    प्रत्यागम !VN_IS(nodep, ग्रन्थिः_व्यञ्जकम्);
}
template <>
अन्तःस्थ bool Ast_ग्रन्थिः::mayBeUnder<AstVarScope>(ध्रुव Ast_ग्रन्थिः* nodep) {
    जइ (VN_IS(nodep, VarScope)) प्रत्यागम false;  // Should not nest
    जइ (VN_IS(nodep, Var)) प्रत्यागम false;
    जइ (VN_IS(nodep, Active)) प्रत्यागम false;
    जइ (VN_IS(nodep, ग्रन्थिः_Stmt)) प्रत्यागम false;
    जइ (VN_IS(nodep, ग्रन्थिः_व्यञ्जकम्)) प्रत्यागम false;
    प्रत्यागम true;
}
template <>
अन्तःस्थ bool Ast_ग्रन्थिः::mayBeUnder<AstExecGraph>(ध्रुव Ast_ग्रन्थिः* nodep) {
    जइ (VN_IS(nodep, ExecGraph)) प्रत्यागम false;  // Should not nest
    जइ (VN_IS(nodep, ग्रन्थिः_Stmt)) प्रत्यागम false;  // Should be directly under CFunc
    प्रत्यागम true;
}
template <>
अन्तःस्थ bool Ast_ग्रन्थिः::mayBeUnder<AstActive>(ध्रुव Ast_ग्रन्थिः* nodep) {
    प्रत्यागम !VN_IS(nodep, Active);  // AstActives do not nest
}
template <>
अन्तःस्थ bool Ast_ग्रन्थिः::mayBeUnder<AstScope>(ध्रुव Ast_ग्रन्थिः* nodep) {
    प्रत्यागम !VN_IS(nodep, Scope);  // AstScopes do not nest
}
template <>
अन्तःस्थ bool Ast_ग्रन्थिः::mayBeUnder<Ast_Sen_वृक्षः>(ध्रुव Ast_ग्रन्थिः* nodep) {
    प्रत्यागम !VN_IS(nodep, Sen_वृक्षः);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
template <>
constexpr bool Ast_ग्रन्थिः::isLeaf<Ast_ग्रन्थिः_Var_Ref>() {
    प्रत्यागम true;
}
template <>
constexpr bool Ast_ग्रन्थिः::isLeaf<AstVarRef>() {
    प्रत्यागम true;
}
template <>
constexpr bool Ast_ग्रन्थिः::isLeaf<AstVarXRef>() {
    प्रत्यागम true;
}

// foreach implementation
template <typename T_Arg, typename T_Callable>
शून्य Ast_ग्रन्थिः::foreachImpl(Const_Correct_Ast_ग्रन्थिः<T_Arg>* nodep, ध्रुव T_Callable& f, bool visitNext) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using ग्रन्थिः_2 = Const_Correct_Ast_ग्रन्थिः<T_Arg>;

    // We prefetch this far into the stack
    constexpr पूर्णाङ्क PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_राशिः_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t राशिः_Size = Initial_राशिः_Size;
    ग्रन्थिः_2* in_पङ्क्तिः_राशिः[Initial_राशिः_Size];  // In-line initial stack storage
    std::unique_ptr<ग्रन्थिः_2*[]> out_Of_पङ्क्तिः_Stackp{nullptr};  // In case we need more, we will allocate
    ग्रन्थिः_2** basep = in_पङ्क्तिः_राशिः + PrefetchDistance;  // Pointer to base of stack
    ग्रन्थिः_2** topp = basep;  // Pointer to top of stack
    ग्रन्थिः_2** limp = in_पङ्क्तिः_राशिः + राशिः_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    प्रति (पूर्णाङ्क i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ध्रुव auto visit = [&](ग्रन्थिः_2* currp) {
        // Type test this node
        जइ (Ast_ग्रन्थिः::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            जइ VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) प्रत्यागम;
        }

        // Enqueue children for traversal, unless futile
        जइ (mayBeUnder<T_Arg_NonConst>(currp)) {
            जइ (Ast_ग्रन्थिः* ध्रुव op4p = currp->op4p()) *topp++ = op4p;
            जइ (Ast_ग्रन्थिः* ध्रुव op3p = currp->op3p()) *topp++ = op3p;
            जइ (Ast_ग्रन्थिः* ध्रुव op2p = currp->op2p()) *topp++ = op2p;
            जइ (Ast_ग्रन्थिः* ध्रुव op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    जइ (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    जाव (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        ग्रन्थिः_2* ध्रुव headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        जइ (VL_UNLIKELY(topp >= limp)) {
            ध्रुव ptrdiff_t occupancy = topp - basep;
            {
                ध्रुव size_t new_राशिः_Size = राशिः_Size * 2;
                ग्रन्थिः_2** ध्रुव newStackp = new ग्रन्थिः_2*[new_राशिः_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            परिमाण(ग्रन्थिः_2**) * (occupancy + PrefetchDistance));
                राशिः_Size = new_राशिः_Size;
                out_Of_पङ्क्तिः_Stackp.reset(newStackp);
            }
            basep = out_Of_पङ्क्तिः_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_पङ्क्तिः_Stackp.get() + राशिः_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        जइ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
template <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_ग्रन्थिः::predicateImpl(Const_Correct_Ast_ग्रन्थिः<T_Arg>* nodep, ध्रुव T_Callable& p,
                            bool visitNext) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using ग्रन्थिः_2 = Const_Correct_Ast_ग्रन्थिः<T_Arg>;

    // We prefetch this far into the stack
    constexpr पूर्णाङ्क PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_राशिः_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t राशिः_Size = Initial_राशिः_Size;
    ग्रन्थिः_2* in_पङ्क्तिः_राशिः[Initial_राशिः_Size];  // In-line initial stack storage
    std::unique_ptr<ग्रन्थिः_2*[]> out_Of_पङ्क्तिः_Stackp{nullptr};  // In case we need more, we will allocate
    ग्रन्थिः_2** basep = in_पङ्क्तिः_राशिः + PrefetchDistance;  // Pointer to base of stack
    ग्रन्थिः_2** topp = basep;  // Pointer to top of stack
    ग्रन्थिः_2** limp = in_पङ्क्तिः_राशिः + राशिः_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    प्रति (पूर्णाङ्क i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ध्रुव auto visit = [&](ग्रन्थिः_2* currp) {
        // Type test this node
        जइ (Ast_ग्रन्थिः::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            जइ (p(static_cast<T_Arg*>(currp)) != N_Default) प्रत्यागम true;
            // Short circuit if iterating leaf nodes
            जइ VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) प्रत्यागम false;
        }

        // Enqueue children for traversal, unless futile
        जइ (mayBeUnder<T_Arg_NonConst>(currp)) {
            जइ (Ast_ग्रन्थिः* ध्रुव op4p = currp->op4p()) *topp++ = op4p;
            जइ (Ast_ग्रन्थिः* ध्रुव op3p = currp->op3p()) *topp++ = op3p;
            जइ (Ast_ग्रन्थिः* ध्रुव op2p = currp->op2p()) *topp++ = op2p;
            जइ (Ast_ग्रन्थिः* ध्रुव op1p = currp->op1p()) *topp++ = op1p;
        }

        प्रत्यागम false;
    };

    // Enqueue the next of the root node, if required
    जइ (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    जइ (visit(nodep)) प्रत्यागम !N_Default;

    // Visit the rest of the tree
    जाव (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        ग्रन्थिः_2* ध्रुव headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        जइ (VL_UNLIKELY(topp >= limp)) {
            ध्रुव ptrdiff_t occupancy = topp - basep;
            {
                ध्रुव size_t new_राशिः_Size = राशिः_Size * 2;
                ग्रन्थिः_2** ध्रुव newStackp = new ग्रन्थिः_2*[new_राशिः_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            परिमाण(ग्रन्थिः_2**) * (occupancy + PrefetchDistance));
                राशिः_Size = new_राशिः_Size;
                out_Of_पङ्क्तिः_Stackp.reset(newStackp);
            }
            basep = out_Of_पङ्क्तिः_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_पङ्क्तिः_Stackp.get() + राशिः_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        जइ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        जइ (visit(headp)) प्रत्यागम !N_Default;
    }

    प्रत्यागम N_Default;
}

अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव Ast_ग्रन्थिः* rhs) {
    जइ (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } अण्णहा जइ (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } अण्णहा {
        rhs->dump(os);
    }
    प्रत्यागम os;
}
शून्य VNRelinker::relink(Ast_ग्रन्थिः* newp) { newp->Ast_ग्रन्थिः::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
template <typename T_ग्रन्थिः>
class VNRef final : सार्वजनिक std::reference_wrapper<T_ग्रन्थिः> {
    static_assert(std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

सार्वजनिक:
    template <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_ग्रन्थिः>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(ध्रुव std::reference_wrapper<T_ग्रन्थिः>& other)
        : std::reference_wrapper<T_ग्रन्थिः>{other} {}
};

static_assert(परिमाण(VNRef<Ast_ग्रन्थिः>) == परिमाण(std::reference_wrapper<Ast_ग्रन्थिः>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
size_t V3HasherUncachedHash(ध्रुव Ast_ग्रन्थिः&);

// Specialization of std::hash for VNRef
template <typename T_ग्रन्थिः>
संरचना std::hash<VNRef<T_ग्रन्थिः>> final {
    size_t operator()(VNRef<T_ग्रन्थिः> r) ध्रुव { प्रत्यागम V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
template <typename T_ग्रन्थिः>
संरचना std::equal_to<VNRef<T_ग्रन्थिः>> final {
    size_t operator()(VNRef<T_ग्रन्थिः> ra, VNRef<T_ग्रन्थिः> rb) ध्रुव {
        प्रत्यागम ra.get().same_वृक्षः(&(rb.get()));
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
    std::unordered_map<ध्रुव Ast_ग्रन्थिः*, Ast_ग्रन्थिः*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<ध्रुव Ast_ग्रन्थिः*, std::vector<Ast_ग्रन्थिः*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<ध्रुव Ast_ग्रन्थिः*, std::string> m_prefixes;
    ध्रुव VDTypeNameScopes* ध्रुव m_prevp;  // Index in use before this one
    स्थिर ध्रुव VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    शून्य index(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

सार्वजनिक:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    स्थिर ध्रुव VDTypeNameScopes* currentp() { प्रत्यागम s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(ध्रुव Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः*& outerpr) ध्रुव;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ध्रुव std::vector<Ast_ग्रन्थिः*>* paramsp(ध्रुव Ast_ग्रन्थिः* modp) ध्रुव;
    // The prefix kept for scope 'scopep', or nullptr if none
    ध्रुव std::string* prefixp(ध्रुव Ast_ग्रन्थिः* scopep) ध्रुव;
    // Keep 'prefix', final, for scope 'scopep'
    शून्य prefix(ध्रुव Ast_ग्रन्थिः* scopep, ध्रुव std::string& prefix) ध्रुव;
};

//######################################################################
// Inline VNVisitor METHODS

शून्य VNVisitorConst::iterateConst(Ast_ग्रन्थिः* nodep) { nodep->accept(*this); }
शून्य VNVisitorConst::iterateConstNull(Ast_ग्रन्थिः* nodep) {
    जइ (VL_LIKELY(nodep)) nodep->accept(*this);
}
शून्य VNVisitorConst::iterateChildrenConst(Ast_ग्रन्थिः* nodep) { nodep->iterateChildrenConst(*this); }
शून्य VNVisitorConst::iterateChildrenBackwardsConst(Ast_ग्रन्थिः* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
शून्य VNVisitorConst::iterateAndNextConstNull(Ast_ग्रन्थिः* nodep) {
    जइ (VL_LIKELY(nodep)) nodep->iterateAndNextConst(*this);
}

शून्य VNVisitor::iterate(Ast_ग्रन्थिः* nodep) { nodep->accept(*this); }
शून्य VNVisitor::iterateNull(Ast_ग्रन्थिः* nodep) {
    जइ (VL_LIKELY(nodep)) nodep->accept(*this);
}
शून्य VNVisitor::iterateChildren(Ast_ग्रन्थिः* nodep) { nodep->iterateChildren(*this); }
शून्य VNVisitor::iterateAndNextNull(Ast_ग्रन्थिः* nodep) {
    जइ (VL_LIKELY(nodep)) nodep->iterateAndNext(*this);
}
Ast_ग्रन्थिः* VNVisitor::iterateSubtreeReturnEdits(Ast_ग्रन्थिः* nodep) {
    प्रत्यागम nodep->iterateSubtreeReturnEdits(*this);
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
शून्य dump_ग्रन्थिः_List_Json(std::ostream& os, ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव std::string& listName,
                      ध्रुव string& indent);

#endif  // Guard
