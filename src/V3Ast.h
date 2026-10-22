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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Broken.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3FunctionTraits.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Number.h"
#ଅନ୍ତର୍ଭାଵ "V3StdFuture.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#ଅନ୍ତର୍ଭାଵ <array>
#ଅନ୍ତର୍ଭାଵ <cmath>
#ଅନ୍ତର୍ଭାଵ <cstdint>
#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <type_traits>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#ଯଦି defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#undef VL_USER_TYPE_CHECKS
#endif
#ifdef VL_USER_TYPE_CHECKS
#ଅନ୍ତର୍ଭାଵ <variant>
#endif

// clang-format off
#ଅନ୍ତର୍ଭାଵ "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3Graph;
class ExecMTask;
class VIfaceCaptureTag;

//######################################################################

// For broken() function, return error string if have a match
#define BROKEN_RTN(test) \
    କର { \
        ଯଦି (VL_UNCOVERABLE(test)) ଫେରନ୍ତୁ "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } ଯେତେବେଳେ (false)
// For broken() function, return error string if a base of this class has a match
#define BROKEN_BASE_RTN(test) \
    କର { \
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ reasonp = (test); \
        ଯଦି (VL_UNCOVERABLE(reasonp)) ଫେରନ୍ତୁ reasonp; \
    } ଯେତେବେଳେ (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#define VN_IS(nodep, nodetypename) (Ast_ଗଣ୍ଠି::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#define VN_CAST(nodep, nodetypename) (Ast_ଗଣ୍ଠି::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#define VN_AS(nodep, nodetypename) (Ast_ଗଣ୍ଠି::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#ଯଦି defined(VL_DEBUG) || defined(VL_ASAN)
#define VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#ଅନ୍ୟଥା
#define VN_DBG_AS(nodep, nodetypename) (Ast_ଗଣ୍ଠି::unsafePrivateAs<Ast##nodetypename>(nodep))
#endif

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#define VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

ସଂରଚନା VNTypeInfo final {
    ଅଚଳ ଅକ୍ଷର* m_namep;
    ଗଣନା OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    ଅଚଳ ଅକ୍ଷର* m_opNamep[4];
    size_t m_sizeof;
};

class VNType final {
    ସ୍ଥିର ଅଚଳ VNTypeInfo s_typeInfoTable[];

ସର୍ବସାଧାରଣ:
#ଅନ୍ତର୍ଭାଵ "V3Ast__gen_type_enum.h"  // From ./astgen
    ଅଚଳ en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr operator en() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e; }

    // Retrieve VNTypeInfo for the given type
    ସ୍ଥିର ଅଚଳ VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { ଫେରନ୍ତୁ s_typeInfoTable[t.m_e]; }
};
constexpr bool operator==(ଅଚଳ VNType& lhs, ଅଚଳ VNType& rhs) VL_PURE {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VNType& lhs, VNType::en rhs) VL_PURE { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VNType::en lhs, ଅଚଳ VNType& rhs) VL_PURE { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VNType& rhs) { ଫେରନ୍ତୁ os << rhs.ascii(); }

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
    std::variant<std::monostate, uint64_t, ଶୂନ୍ୟ*> m_u;
#ଅନ୍ୟଥା
    ସଂଘ {
        ଶୂନ୍ୟ* up;
        uint64_t uq;
    } m_u;
#endif

ସର୍ବସାଧାରଣ:
#ifdef VL_USER_TYPE_CHECKS
    VNUser() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        ଯଦି (i) m_u = i;
    }
    explicit VNUser(ଶୂନ୍ୟ* p) {
        // VNUser{nullptr} represents the monostate
        ଯଦି (p) m_u = p;
    }
    ~VNUser() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::ମୂଲ୍ୟ, T>::type to() ଅଚଳ VL_MT_SAFE {
        ଯଦି (std::holds_alternative<std::monostate>(m_u)) ଫେରନ୍ତୁ nullptr;
        ଶୂନ୍ୟ* ଅଚଳ* ଅଚଳ upp = std::get_if<ଶୂନ୍ୟ*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        ଫେରନ୍ତୁ reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() ଅଚଳ {
        ଯଦି (std::holds_alternative<std::monostate>(m_u)) ଫେରନ୍ତୁ 0;
        ଅଚଳ uint64_t* ଅଚଳ uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        ଫେରନ୍ତୁ *uip;
    }
#ଅନ୍ୟଥା
    VNUser() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.up = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(ଶୂନ୍ୟ* p) { m_u.up = p; }
    ~VNUser() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // Casters
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::ମୂଲ୍ୟ, T>::type to() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() ଅଚଳ { ଫେରନ୍ତୁ m_u.uq; }
#endif
    VSymEnt* toSymEnt() ଅଚଳ { ଫେରନ୍ତୁ to<VSymEnt*>(); }
    Ast_ଗଣ୍ଠି* toNodep() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ to<Ast_ଗଣ୍ଠି*>(); }
    V3GraphVertex* toGraphVertex() ଅଚଳ { ଫେରନ୍ତୁ to<V3GraphVertex*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(ଅଚଳ ଶୂନ୍ୟ*)) ଅଚଳ;
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
VL_NOT_FINAL{ସୁରକ୍ଷିତ : ସ୍ଥିର ଶୂନ୍ୟ allocate(ପୂର୍ଣ୍ଣସଂଖ୍ୟା id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvtToStr(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
ସ୍ଥିର ଶୂନ୍ୟ ମୁକ୍ତ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvtToStr(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
ସ୍ଥିର ଶୂନ୍ୟ clearcnt(ପୂର୍ଣ୍ଣସଂଖ୍ୟା id, uint32_t& cntGblRef, ଅଚଳ bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
ସ୍ଥିର ଶୂନ୍ୟ checkcnt(ପୂର୍ଣ୍ଣସଂଖ୍ୟା id, uint32_t&, ଅଚଳ bool& userBusyRef) {
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
ସୁରକ୍ଷିତ:
    friend class Ast_ଗଣ୍ଠି;
    ସ୍ଥିର uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ସ୍ଥିର bool s_userBusy;  // Count is in use
ସର୍ବସାଧାରଣ:
    VNUser1InUse()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1InUse()     { ମୁକ୍ତ    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ସ୍ଥିର ଶୂନ୍ୟ clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ସ୍ଥିର ଶୂନ୍ୟ check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2InUse final : VNUserInUseBase {
ସୁରକ୍ଷିତ:
    friend class Ast_ଗଣ୍ଠି;
    ସ୍ଥିର uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ସ୍ଥିର bool s_userBusy;  // Count is in use
ସର୍ବସାଧାରଣ:
    VNUser2InUse()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2InUse()     { ମୁକ୍ତ    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ସ୍ଥିର ଶୂନ୍ୟ clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ସ୍ଥିର ଶୂନ୍ୟ check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3InUse final : VNUserInUseBase {
ସୁରକ୍ଷିତ:
    friend class Ast_ଗଣ୍ଠି;
    ସ୍ଥିର uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ସ୍ଥିର bool s_userBusy;  // Count is in use
ସର୍ବସାଧାରଣ:
    VNUser3InUse()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3InUse()     { ମୁକ୍ତ    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ସ୍ଥିର ଶୂନ୍ୟ clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ସ୍ଥିର ଶୂନ୍ୟ check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4InUse final : VNUserInUseBase {
ସୁରକ୍ଷିତ:
    friend class Ast_ଗଣ୍ଠି;
    ସ୍ଥିର uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    ସ୍ଥିର bool s_userBusy;  // Count is in use
ସର୍ବସାଧାରଣ:
    VNUser4InUse()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4InUse()     { ମୁକ୍ତ    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ସ୍ଥିର ଶୂନ୍ୟ clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ସ୍ଥିର ଶୂନ୍ୟ check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::vector<Ast_ଗଣ୍ଠି*> m_deleteps;  // Nodes to delete

ସର୍ବସାଧାରଣ:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    ଶୂନ୍ୟ ଠେଲ_Deletep(Ast_ଗଣ୍ଠି* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.ଠେଲ_back(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    ଶୂନ୍ୟ doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    friend class Ast_ଗଣ୍ଠି;

ସର୍ବସାଧାରଣ:
    /// Call visit()s on nodep
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterateConst(Ast_ଗଣ୍ଠି* nodep);
    /// Call visit()s on nodep
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterateConstNull(Ast_ଗଣ୍ଠି* nodep);
    /// Call visit()s on const nodep's children
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterateChildrenConst(Ast_ଗଣ୍ଠି* nodep);
    /// Call visit()s on nodep's children in backp() order
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterateChildrenBackwardsConst(Ast_ଗଣ୍ଠି* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterateAndNextConstNull(Ast_ଗଣ୍ଠି* nodep);

    ଆଭାସୀ ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) = 0;
    ଆଭାସୀ ~VNVisitorConst() {}
#ଅନ୍ତର୍ଭାଵ "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : ସର୍ବସାଧାରଣ VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
ସର୍ବସାଧାରଣ:
    /// Call visit()s on nodep
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterate(Ast_ଗଣ୍ଠି* nodep);
    /// Call visit()s on nodep
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterateNull(Ast_ଗଣ୍ଠି* nodep);
    /// Call visit()s on nodep's children
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterateChildren(Ast_ଗଣ୍ଠି* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ iterateAndNextNull(Ast_ଗଣ୍ଠି* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    ଧାଡ଼ିଭିତରେ Ast_ଗଣ୍ଠି* iterateSubtreeReturnEdits(Ast_ଗଣ୍ଠି* nodep);

    VNDeleter& deleter() { ଫେରନ୍ତୁ m_deleter; }
    ଶୂନ୍ୟ ଠେଲ_Deletep(Ast_ଗଣ୍ଠି* nodep) { deleter().ଠେଲ_Deletep(nodep); }
    ଶୂନ୍ୟ doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
ସୁରକ୍ଷିତ:
    friend class Ast_ଗଣ୍ଠି;
    ଗଣନା RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_NEXT,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_ଗଣ୍ଠି* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_ଗଣ୍ଠି* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_ଗଣ୍ଠି** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

ସର୍ବସାଧାରଣ:
    VNRelinker() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ relink(Ast_ଗଣ୍ଠି* newp);
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ;
};
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VNRelinker& rhs) {
    rhs.dump(os);
    ଫେରନ୍ତୁ os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{ସର୍ବସାଧାରଣ : ଆଭାସୀ bool ଗଣ୍ଠି_Match(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {ଫେରନ୍ତୁ true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#define ASTNODE_PREFETCH_NON_NULL(nodep) \
    କର { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } ଯେତେବେଳେ (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#define ASTNODE_PREFETCH(nodep) \
    କର { \
        ଯଦି (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } ଯେତେବେଳେ (false)

class Ast_ଗଣ୍ଠି VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_ଗଣ୍ଠି* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_ଗଣ୍ଠି* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_ଗଣ୍ଠି* m_op1p = nullptr;  // Generic pointer 1
    Ast_ଗଣ୍ଠି* m_op2p = nullptr;  // Generic pointer 2
    Ast_ଗଣ୍ଠି* m_op3p = nullptr;  // Generic pointer 3
    Ast_ଗଣ୍ଠି* m_op4p = nullptr;  // Generic pointer 4
    Ast_ଗଣ୍ଠି** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    ଅଚଳ VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    ସଂରଚନା {
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

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_cloneCnt = 0;  // Sequence number for when last clone was made

#ଯଦି defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(ଆକାର(m_type) + ଆକାର(m_flags) + ଆକାର(m_brokenState) + ଆକାର(m_cloneCnt)
                      <= ଆକାର(ଶୂନ୍ୟ*),
                  "packing requires padding");
#endif

    Ast_ଗଣ୍ଠି_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_ଗଣ୍ଠି* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_ଧାଡ଼ି* m_fileline;  // Where it was declared
#ifdef VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_ଗଣନା;  // When it was last edited
#endif
    ସ୍ଥିର uint64_t s_editCntGbl;  // Global edit counter
    ସ୍ଥିର uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_ଗଣ୍ଠି* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_cloneCntGbl;  // Count of which userp is set

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
    ଶୂନ୍ୟ op1p(Ast_ଗଣ୍ଠି* nodep) {
        m_op1p = nodep;
        ଯଦି (nodep) nodep->m_backp = this;
    }
    ଶୂନ୍ୟ op2p(Ast_ଗଣ୍ଠି* nodep) {
        m_op2p = nodep;
        ଯଦି (nodep) nodep->m_backp = this;
    }
    ଶୂନ୍ୟ op3p(Ast_ଗଣ୍ଠି* nodep) {
        m_op3p = nodep;
        ଯଦି (nodep) nodep->m_backp = this;
    }
    ଶୂନ୍ୟ op4p(Ast_ଗଣ୍ଠି* nodep) {
        m_op4p = nodep;
        ଯଦି (nodep) nodep->m_backp = this;
    }

ବ୍ୟକ୍ତିଗତ:
    Ast_ଗଣ୍ଠି* clone_ବୃକ୍ଷ_Iter(bool needPure);
    Ast_ଗଣ୍ଠି* clone_ବୃକ୍ଷ_Iter_List(bool needPure);
    ଶୂନ୍ୟ check_ବୃକ୍ଷ_Iter(ଅଚଳ Ast_ଗଣ୍ଠି* prevBackp) ଅଚଳ VL_MT_STABLE;
    bool gate_ବୃକ୍ଷ_Iter() ଅଚଳ;
    ସ୍ଥିର bool same_ବୃକ୍ଷ_Iter(ଅଚଳ Ast_ଗଣ୍ଠି* node1p, ଅଚଳ Ast_ଗଣ୍ଠି* node2p, bool ignNext,
                             bool gateOnly);
    ଶୂନ୍ୟ delete_ବୃକ୍ଷ_Iter();
    ଶୂନ୍ୟ delete_ଗଣ୍ଠି();
    string instanceStr() ଅଚଳ;

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ purityCheck();
    ସ୍ଥିର ଶୂନ୍ୟ relinkOneLink(Ast_ଗଣ୍ଠି*& pointpr, Ast_ଗଣ୍ଠି* newp);
    // cppcheck-suppress functionConst
    ସ୍ଥିର ଶୂନ୍ୟ debug_ବୃକ୍ଷ_Change(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ ଅକ୍ଷର* prefix, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno, bool next);

ସୁରକ୍ଷିତ:
    // CONSTRUCTORS
    Ast_ଗଣ୍ଠି(VNType t, File_ଧାଡ଼ି* fl);
    ଆଭାସୀ ~Ast_ଗଣ୍ଠି() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;  // Use 'deleteTree' instead
    ଆଭାସୀ Ast_ଗଣ୍ଠି* clone() = 0;  // Generally, cloneTree is what you want instead
    ଆଭାସୀ ଶୂନ୍ୟ cloneRelink() { cloneRelinkGen(); }
    ଆଭାସୀ ଶୂନ୍ୟ cloneRelinkGen() {};  // Overrides generated by 'astgen'
    ଶୂନ୍ୟ clone_Relink_ବୃକ୍ଷ();

    // METHODS
    ଶୂନ୍ୟ setOp1p(Ast_ଗଣ୍ଠି* newp);  // Set non-list-type op1 to non-list element
    ଶୂନ୍ୟ setOp2p(Ast_ଗଣ୍ଠି* newp);  // Set non-list-type op2 to non-list element
    ଶୂନ୍ୟ setOp3p(Ast_ଗଣ୍ଠି* newp);  // Set non-list-type op3 to non-list element
    ଶୂନ୍ୟ setOp4p(Ast_ଗଣ୍ଠି* newp);  // Set non-list-type op4 to non-list element

    ଶୂନ୍ୟ addOp1p(Ast_ଗଣ୍ଠି* newp);  // Append newp to end of op1
    ଶୂନ୍ୟ addOp2p(Ast_ଗଣ୍ଠି* newp);  // Append newp to end of op2
    ଶୂନ୍ୟ addOp3p(Ast_ଗଣ୍ଠି* newp);  // Append newp to end of op3
    ଶୂନ୍ୟ addOp4p(Ast_ଗଣ୍ଠି* newp);  // Append newp to end of op4

    ଶୂନ୍ୟ prependOp1p(Ast_ଗଣ୍ଠି* newp);  // Prepend newp to front of op1
    ଶୂନ୍ୟ prependOp2p(Ast_ଗଣ୍ଠି* newp);  // Prepend newp to front of op2
    ଶୂନ୍ୟ prependOp3p(Ast_ଗଣ୍ଠି* newp);  // Prepend newp to front of op3
    ଶୂନ୍ୟ prependOp4p(Ast_ଗଣ୍ଠି* newp);  // Prepend newp to front of op4

    // clang-format off
    ଶୂନ୍ୟ setNOp1p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) setOp1p(newp); }
    ଶୂନ୍ୟ setNOp2p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) setOp2p(newp); }
    ଶୂନ୍ୟ setNOp3p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) setOp3p(newp); }
    ଶୂନ୍ୟ setNOp4p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) setOp4p(newp); }

    ଶୂନ୍ୟ addNOp1p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) addOp1p(newp); }
    ଶୂନ୍ୟ addNOp2p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) addOp2p(newp); }
    ଶୂନ୍ୟ addNOp3p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) addOp3p(newp); }
    ଶୂନ୍ୟ addNOp4p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) addOp4p(newp); }

    ଶୂନ୍ୟ prependNOp1p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) prependOp1p(newp); }
    ଶୂନ୍ୟ prependNOp2p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) prependOp2p(newp); }
    ଶୂନ୍ୟ prependNOp3p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) prependOp3p(newp); }
    ଶୂନ୍ୟ prependNOp4p(Ast_ଗଣ୍ଠି* newp) { ଯଦି (newp) prependOp4p(newp); }
    // clang-format on

    ଶୂନ୍ୟ clonep(Ast_ଗଣ୍ଠି* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    ସ୍ଥିର ଶୂନ୍ୟ clone_Clear_ବୃକ୍ଷ() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    ଆଭାସୀ bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ { ଫେରନ୍ତୁ true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    ଆଭାସୀ bool wouldBreakGen(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ oldp,
                               ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ newp) ଅଚଳ
        = 0;  // Generated by 'astgen'

ସର୍ବସାଧାରଣ:
    // ACCESSORS
    VNType type() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_type; }
    ଅଚଳ ଅକ୍ଷର* typeName() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ type().ascii(); }  // See also prettyTypeName
    Ast_ଗଣ୍ଠି* nextp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_nextp; }
    Ast_ଗଣ୍ଠି* backp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_backp; }
    Ast_ଗଣ୍ଠି* abovep() ଅଚଳ;  // Get parent node above, only for list head and tail
    Ast_ଗଣ୍ଠି* aboveLoopp() ଅଚଳ;  // Get parent node above, may have performance issues as loops
    Ast_ଗଣ୍ଠି* lastp() ଅଚଳ {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        ଫେରନ୍ତୁ m_headtailp;
    }
    Ast_ଗଣ୍ଠି* prevp() ଅଚଳ {  // Previous node in list, nullptr for head of list
        ଯଦି (m_backp->m_nextp != this) ଫେରନ୍ତୁ nullptr;
        ଫେରନ୍ତୁ m_backp;
    }
    Ast_ଗଣ୍ଠି* op1p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_op1p; }
    Ast_ଗଣ୍ଠି* op2p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_op2p; }
    Ast_ଗଣ୍ଠି* op3p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_op3p; }
    Ast_ଗଣ୍ଠି* op4p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_op4p; }
    Ast_ଗଣ୍ଠି_DType* dtypep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_dtypep; }
    Ast_ଗଣ୍ଠି* clonep() ଅଚଳ { ଫେରନ୍ତୁ ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_ଗଣ୍ଠି* firstAbovep() ଅଚଳ {  // Returns nullptr when second or later in list
        ଫେରନ୍ତୁ ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_brokenState; }
    ଶୂନ୍ୟ brokenState(uint8_t ମୂଲ୍ୟ) { m_brokenState = ମୂଲ୍ୟ; }

    // Used by AstNode::broken()
    bool brokeExists() ଅଚଳ { ଫେରନ୍ତୁ V3Broken::isLinkable(this); }
    bool brokeExistsAbove() ଅଚଳ { ଫେରନ୍ତୁ brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    ଆଭାସୀ ଶୂନ୍ୟ foreachLink(std::function<ଶୂନ୍ୟ(Ast_ଗଣ୍ଠି** linkpp, ଅଚଳ ଅକ୍ଷର* namep)> f) = 0;

#ଯଦି defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    ସ୍ଥିର ଶୂନ୍ୟ* operator new(size_t size);
    ସ୍ଥିର ଶୂନ୍ୟ operator delete(ଶୂନ୍ୟ* obj, size_t size);
#endif

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_BRANCH = 4;  // Branch
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_CALL = INSTR_ଗଣନା_BRANCH + 10;  // Subroutine call
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_LD = 2;  // Load memory
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_INT_MUL = 3;  // Integer multiply
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_INT_DIV = 10;  // Integer divide
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_DBL = 8;  // Convert or do float ops
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_DBL_DIV = 40;  // Double divide
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_DBL_TRIG = 200;  // Double trigonometric ops
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_STR = 100;  // String ops
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_TIME = INSTR_ଗଣନା_CALL + 5;  // Determine simulation time
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_PLI = 20;  // PLI routines
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INSTR_ଗଣନା_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    ଆଭାସୀ string name() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ ""; }
    ଆଭାସୀ string origName() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    string prettyDehashOrigOrName() ଅଚଳ {
        ଫେରନ୍ତୁ prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    ଆଭାସୀ ଶୂନ୍ୟ name(ଅଚଳ string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    ଆଭାସୀ ଶୂନ୍ୟ tag(ଅଚଳ string& text) {}
    ଆଭାସୀ string tag() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    ଆଭାସୀ uint32_t decl_Token_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ 0; }
    ଆଭାସୀ ଶୂନ୍ୟ decl_Token_ଅଙ୍କ_Set_Min(uint32_t token_ଅଙ୍କ) {}
    ଆଭାସୀ string verilogKwd() ଅଚଳ { ଫେରନ୍ତୁ ""; }
    string nameProtect() ଅଚଳ VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() ଅଚଳ;  // origName with --protect-id applied
    string shortName() ଅଚଳ;  // Name with __PVT__ removed for concatenating scopes
    ସ୍ଥିର string dedotName(ଅଚଳ string& namein);  // Name with dots removed
    ସ୍ଥିର string nameNoArray(ଅଚଳ string& namein);  // Name with any array index removed
    ସ୍ଥିର string prettyName(ଅଚଳ string& namein) VL_PURE;  // Name for printing out to the user
    ସ୍ଥିର string vpiName(ଅଚଳ string& namein);  // Name for vpi access
    ସ୍ଥିର string prettyNameQ(ଅଚଳ string& namein) {  // Quoted pretty name (for errors)
        ଫେରନ୍ତୁ "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    ସ୍ଥିର string encodeName(ଅଚଳ string& namein);
    ସ୍ଥିର string encode_ସଂଖ୍ୟା(int64_t ଅଙ୍କ);  // Encode number into internal C representation
    ସ୍ଥିର string vcdName(ଅଚଳ string& namein);  // Name for printing out to vcd files
    string prettyName() ଅଚଳ { ଫେରନ୍ତୁ prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    ଆଭାସୀ string pretty_Name_ସନ୍ଦେଶ() ଅଚଳ { ଫେରନ୍ତୁ prettyName(); }
    string prettyNameQ() ଅଚଳ { ଫେରନ୍ତୁ "'"s + pretty_Name_ସନ୍ଦେଶ() + "'"; }  // Quoted, for messages
    string verilogName() ଅଚଳ { ଫେରନ୍ତୁ vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() ଅଚଳ;
    ଆଭାସୀ string prettyOperatorName() ଅଚଳ { ଫେରନ୍ତୁ "operator " + prettyTypeName(); }
    File_ଧାଡ଼ି* fileline() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_fileline; }
    ଶୂନ୍ୟ fileline(File_ଧାଡ଼ି* fl) { m_fileline = fl; }
    ଧାଡ଼ିଭିତରେ bool width1() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthInstrs() ଅଚଳ;
    bool didWidth() ଅଚଳ { ଫେରନ୍ତୁ m_flags.didWidth; }
    ଶୂନ୍ୟ didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        ଯଦି (didWidth()) ଫେରନ୍ତୁ true;
        didWidth(true);
        ଫେରନ୍ତୁ false;
    }
    bool doingWidth() ଅଚଳ { ଫେରନ୍ତୁ m_flags.doingWidth; }
    ଶୂନ୍ୟ doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_flags.protect; }
    ଶୂନ୍ୟ protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin() ଅଚଳ VL_MT_STABLE;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMinV() ଅଚଳ {
        ଫେରନ୍ତୁ v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା width_ଶବ୍ଦଗୁଡ଼ିକ() ଅଚଳ { ଫେରନ୍ତୁ VL_ଶବ୍ଦଗୁଡ଼ିକ_I(width()); }
    bool isQuad() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ (width() > VL_QUADSIZE); }
    ଧାଡ଼ିଭିତରେ bool isCHandle() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ bool isDouble() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ bool isSigned() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ bool isString() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ bool isEvent() ଅଚଳ VL_MT_STABLE;

    // clang-format off
    VNUser user1u() ଅଚଳ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        ଫେରନ୍ତୁ ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_ଗଣ୍ଠି* user1p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ user1u().toNodep(); }
    ଶୂନ୍ୟ user1u(ଅଚଳ VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    ଶୂନ୍ୟ user1p(ଶୂନ୍ୟ* userp) { user1u(VNUser{userp}); }
    ଶୂନ୍ୟ user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() ଅଚଳ { ଫେରନ୍ତୁ user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { ଅଚଳ uint64_t v = user1(); user1(v + val); ଫେରନ୍ତୁ v; }
    uint64_t user1Or(uint64_t val) { ଅଚଳ uint64_t v = user1(); user1(v | val); ଫେରନ୍ତୁ v; }
    uint64_t user1SetOnce() { ଅଚଳ uint64_t v = user1(); ଯଦି (!v) user1(1); ଫେରନ୍ତୁ v; }  // Better for cache than user1Inc()
    ସ୍ଥିର ଶୂନ୍ୟ user1_Clear_ବୃକ୍ଷ() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() ଅଚଳ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        ଫେରନ୍ତୁ ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_ଗଣ୍ଠି* user2p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ user2u().toNodep(); }
    ଶୂନ୍ୟ user2u(ଅଚଳ VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    ଶୂନ୍ୟ user2p(ଶୂନ୍ୟ* userp) { user2u(VNUser{userp}); }
    ଶୂନ୍ୟ user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() ଅଚଳ { ଫେରନ୍ତୁ user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { ଅଚଳ uint64_t v = user2(); user2(v + val); ଫେରନ୍ତୁ v; }
    uint64_t user2Or(uint64_t val) { ଅଚଳ uint64_t v = user2(); user2(v | val); ଫେରନ୍ତୁ v; }
    uint64_t user2SetOnce() { ଅଚଳ uint64_t v = user2(); ଯଦି (!v) user2(1); ଫେରନ୍ତୁ v; }  // Better for cache than user2Inc()
    ସ୍ଥିର ଶୂନ୍ୟ user2_Clear_ବୃକ୍ଷ() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() ଅଚଳ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        ଫେରନ୍ତୁ ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_ଗଣ୍ଠି* user3p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ user3u().toNodep(); }
    ଶୂନ୍ୟ user3u(ଅଚଳ VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    ଶୂନ୍ୟ user3p(ଶୂନ୍ୟ* userp) { user3u(VNUser{userp}); }
    ଶୂନ୍ୟ user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() ଅଚଳ { ଫେରନ୍ତୁ user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { ଅଚଳ uint64_t v = user3(); user3(v + val); ଫେରନ୍ତୁ v; }
    uint64_t user3Or(uint64_t val) { ଅଚଳ uint64_t v = user3(); user3(v | val); ଫେରନ୍ତୁ v; }
    uint64_t user3SetOnce() { ଅଚଳ uint64_t v = user3(); ଯଦି (!v) user3(1); ଫେରନ୍ତୁ v; }  // Better for cache than user3Inc()
    ସ୍ଥିର ଶୂନ୍ୟ user3_Clear_ବୃକ୍ଷ() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() ଅଚଳ VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        ଫେରନ୍ତୁ ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_ଗଣ୍ଠି* user4p() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ user4u().toNodep(); }
    ଶୂନ୍ୟ user4u(ଅଚଳ VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    ଶୂନ୍ୟ user4p(ଶୂନ୍ୟ* userp) { user4u(VNUser{userp}); }
    ଶୂନ୍ୟ user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() ଅଚଳ { ଫେରନ୍ତୁ user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { ଅଚଳ uint64_t v = user4(); user4(v | val); ଫେରନ୍ତୁ v; }
    uint64_t user4Inc(uint64_t val = 1) { ଅଚଳ uint64_t v = user4(); user4(v + val); ଫେରନ୍ତୁ v; }
    uint64_t user4SetOnce() { ଅଚଳ uint64_t v = user4(); ଯଦି (!v) user4(1); ଫେରନ୍ତୁ v; }  // Better for cache than user4Inc()
    ସ୍ଥିର ଶୂନ୍ୟ user4_Clear_ବୃକ୍ଷ() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#ifdef VL_DEBUG
    uint64_t edit_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_edit_ଗଣନା; }
    ଶୂନ୍ୟ edit_ଗଣନା_Inc() {
        m_edit_ଗଣନା = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_ବୃକ୍ଷ();  // Any edit clears all caching
    }
#ଅନ୍ୟଥା
    ଶୂନ୍ୟ edit_ଗଣନା_Inc() { ++s_editCntGbl; }
#endif
    ସ୍ଥିର uint64_t edit_ଗଣନା_Last() VL_MT_SAFE { ଫେରନ୍ତୁ s_editCntLast; }
    ସ୍ଥିର uint64_t edit_ଗଣନା_Gbl() VL_MT_SAFE { ଫେରନ୍ତୁ s_editCntGbl; }
    ସ୍ଥିର ଶୂନ୍ୟ edit_ଗଣନା_Set_Last() { s_editCntLast = edit_ଗଣନା_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_ଠେଲ_Self_Stmt();
    ଧାଡ଼ିଭିତରେ bool is_Class_Handle_ମୂଲ୍ୟ() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool isNull() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool isZero() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool isOne() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool isNeqZero() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool isAllOnes() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool isAllOnesV() ଅଚଳ;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    ଶୂନ୍ୟ dtypep(Ast_ଗଣ୍ଠି_DType* nodep) {
        ଯଦି (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_ଗଣନା_Inc();
        }
    }
    ଶୂନ୍ୟ dtypeFrom(ଅଚଳ Ast_ଗଣ୍ଠି* fromp) {
        ଯଦି (fromp) dtypep(fromp->dtypep());
    }
    ଶୂନ୍ୟ dtypeChgSigned(bool flag = true);
    ଶୂନ୍ୟ dtypeChgWidth(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin);
    ଶୂନ୍ୟ dtypeChgWidthSigned(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric);
    ଶୂନ୍ୟ dtypeSetBitUnsized(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) {
        dtypep(findBitDType(width, widthMin, numeric));
    }
    ଶୂନ୍ୟ dtypeSetBitSized(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, VSigning numeric) {
        dtypep(findBitDType(width, width, numeric));  // Since sized, widthMin is width
    }
    ଶୂନ୍ୟ dtypeSetLogicUnsized(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) {
        dtypep(findLogicDType(width, widthMin, numeric));
    }
    ଶୂନ୍ୟ dtypeSetLogicSized(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, VSigning numeric) {
        dtypep(findLogicDType(width, width, numeric));  // Since sized, widthMin is width
    }
    ଶୂନ୍ୟ dtypeSetLogic2StateUnsized(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) {
        dtypep(findLogic2StateDType(width, widthMin, numeric));
    }
    ଶୂନ୍ୟ dtypeSetLogic2StateSized(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, VSigning numeric) {
        dtypep(findLogic2StateDType(width, width, numeric));  // Since sized, widthMin is width
    }
    ଶୂନ୍ୟ dtypeSetBitOrLogicUnsized(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, widthMin, numeric, isFourstate));
    }
    ଶୂନ୍ୟ dtypeSetBitOrLogicSized(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, width, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    ଶୂନ୍ୟ dtypeSetBit() { dtypep(findBitDType()); }
    ଶୂନ୍ୟ dtypeSetLogic() { dtypep(findLogicDType()); }
    ଶୂନ୍ୟ dtypeSetLogic2State() { dtypep(findLogic2StateDType()); }
    ଶୂନ୍ୟ dtypeSetDouble() { dtypep(findDoubleDType()); }
    ଶୂନ୍ୟ dtypeSetInt() { dtypep(findIntDType()); }
    ଶୂନ୍ୟ dtypeSetInteger() { dtypep(findIntegerDType()); }
    ଶୂନ୍ୟ dtypeSetInteger2State() { dtypep(findInteger2StateDType()); }
    ଶୂନ୍ୟ dtypeSetString() { dtypep(findStringDType()); }
    ଶୂନ୍ୟ dtypeSetUInt32() { dtypep(findUInt32DType()); }  // Twostate
    ଶୂନ୍ୟ dtypeSetUInt64() { dtypep(findUInt64DType()); }  // Twostate
    ଶୂନ୍ୟ dtypeSetTime() { dtypep(findTimeDType()); }
    ଶୂନ୍ୟ dtypeSetTime2State() { dtypep(findTime2StateDType()); }
    ଶୂନ୍ୟ dtypeSetEmptyQueue() { dtypep(findEmptyQueueDType()); }
    ଶୂନ୍ୟ dtypeSetStream() { dtypep(findStreamDType()); }
    ଶୂନ୍ୟ dtypeSetVoid() { dtypep(findVoidDType()); }
    ଶୂନ୍ୟ dtypeSetBitOrLogic(bool isFourstate) {
        dtypep(isFourstate ? findLogicDType() : findBitDType());
    }

    // Data type locators
    Ast_ଗଣ୍ଠି_DType* findBitDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_ଗଣ୍ଠି_DType* findLogicDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_ଗଣ୍ଠି_DType* findNullDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_ଗଣ୍ଠି_DType* findLogic2StateDType() ଅଚଳ {
        ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_ଗଣ୍ଠି_DType* findDoubleDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_ଗଣ୍ଠି_DType* findIntDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::INT); }
    Ast_ଗଣ୍ଠି_DType* findIntegerDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_ଗଣ୍ଠି_DType* findInteger2StateDType() ଅଚଳ {
        ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_ଗଣ୍ଠି_DType* findStringDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_ଗଣ୍ଠି_DType* findSigned8DType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_ଗଣ୍ଠି_DType* findUInt32DType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_ଗଣ୍ଠି_DType* findUInt64DType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_ଗଣ୍ଠି_DType* findTimeDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_ଗଣ୍ଠି_DType* findTime2StateDType() ଅଚଳ {
        ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_ଗଣ୍ଠି_DType* findCHandleDType() ଅଚଳ { ଫେରନ୍ତୁ findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_ଗଣ୍ଠି_DType* findConstraintRefDType() ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* findEmptyQueueDType() ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* findQueueIndexDType() ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* findStreamDType() ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* findVoidDType() ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* findBitDType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* findLogicDType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* findLogic2StateDType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* findBitOrLogicDType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric,
                                      bool isFourstate) ଅଚଳ {
        ଫେରନ୍ତୁ isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_ଗଣ୍ଠି_DType* findBitOrLogicRangeDType(ଅଚଳ VNumRange& range, ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin,
                                           ଅଚଳ VSigning numeric,
                                           ଅଚଳ bool isFourstate) ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_ଗଣ୍ଠି_DType* findLogicRangeDType(ଅଚଳ VNumRange& range, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin,
                                      VSigning numeric) ଅଚଳ VL_MT_STABLE;
    Ast_ଗଣ୍ଠି_DType* findBitRangeDType(ଅଚଳ VNumRange& range, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin,
                                    VSigning numeric) ଅଚଳ VL_MT_STABLE;
    Ast_ଗଣ୍ଠି_DType* findBasicDType(VBasicDTypeKwd kwd) ଅଚଳ;
    ସ୍ଥିର AstBasicDType* find_ଭର୍ତ୍ତି_Same_DType(AstBasicDType* nodep);

    ସ୍ଥିର VCastable computeCastable(ଅଚଳ Ast_ଗଣ୍ଠି_DType* toDtp, ଅଚଳ Ast_ଗଣ୍ଠି_DType* fromDtp,
                                     ଅଚଳ Ast_ଗଣ୍ଠି* fromConstp, bool checkIfaceArgCompat = false);
    ସ୍ଥିର Ast_ଗଣ୍ଠି_DType* getCommonClassTypep(Ast_ଗଣ୍ଠି* nodep1, Ast_ଗଣ୍ଠି* nodep2);

    // METHODS - dump and error
    ଶୂନ୍ୟ v3errorEnd(ଅଚଳ std::ostringstream& str) ଅଚଳ VL_RELEASE(V3Error::s().m_mutex);
    ଶୂନ୍ୟ v3errorEndFatal(ଅଚଳ std::ostringstream& str) ଅଚଳ VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex) {
        ଫେରନ୍ତୁ fileline()->warnContextPrimary();
    }
    string warnContextSecondary() ଅଚଳ { ଫେରନ୍ତୁ fileline()->warnContextSecondary(); }
    string warnMore() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex) { ଫେରନ୍ତୁ fileline()->warnMore(); }
    string warnOther() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex) { ଫେରନ୍ତୁ fileline()->warnOther(); }

    ଆଭାସୀ ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ;
    ସ୍ଥିର ଅକ୍ଷର* dump_ବୃକ୍ଷ_Json_Gdb(ଅଚଳ Ast_ଗଣ୍ଠି* nodep);  // For GDB only, free()d by caller
    ସ୍ଥିର ଅକ୍ଷର* dump_ବୃକ୍ଷ_Json_Gdb(ଅଚଳ ଅକ୍ଷର* str);  // For GDB only, free()d by caller
    ସ୍ଥିର ଅକ୍ଷର* dump_ବୃକ୍ଷ_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    ସ୍ଥିର ଶୂନ୍ୟ dumpGdb(ଅଚଳ Ast_ଗଣ୍ଠି* nodep);  // For GDB only
    ଶୂନ୍ୟ dumpGdbHeader() ଅଚଳ;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    template <typename T_ଗଣ୍ଠି_Result, typename T_ଗଣ୍ଠି_Next>
    ସ୍ଥିର T_ଗଣ୍ଠି_Result* addNext(T_ଗଣ୍ଠି_Result* nodep, T_ଗଣ୍ଠି_Next* newp) {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି_Result>::ମୂଲ୍ୟ,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_ଗଣ୍ଠି_Result, T_ଗଣ୍ଠି_Next>::ମୂଲ୍ୟ,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        ଯଦି (!newp) ଫେରନ୍ତୁ nodep;
        ଫେରନ୍ତୁ static_cast<T_ଗଣ୍ଠି_Result*>(addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(nodep, newp));
    }
    template <typename T_ଗଣ୍ଠି_Result, typename T_ଗଣ୍ଠି_Next>
    ସ୍ଥିର T_ଗଣ୍ଠି_Result* addNextNull(T_ଗଣ୍ଠି_Result* nodep, T_ଗଣ୍ଠି_Next* newp) {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି_Result>::ମୂଲ୍ୟ,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_ଗଣ୍ଠି_Result, T_ଗଣ୍ଠି_Next>::ମୂଲ୍ୟ,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        ଯଦି (!newp) ଫେରନ୍ତୁ nodep;
        ଯଦି (!nodep) ଫେରନ୍ତୁ newp;
        ଫେରନ୍ତୁ static_cast<T_ଗଣ୍ଠି_Result*>(addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(nodep, newp));
    }
    ଧାଡ଼ିଭିତରେ Ast_ଗଣ୍ଠି* addNext(Ast_ଗଣ୍ଠି* newp);
    ଶୂନ୍ୟ addNextHere(Ast_ଗଣ୍ଠି* newp);  // Insert newp at this->nextp
    ଶୂନ୍ୟ addHereThisAsNext(Ast_ଗଣ୍ଠି* newp);  // Adds at old place of this, this becomes next
    ଶୂନ୍ୟ replaceWith(Ast_ଗଣ୍ଠି* newp);  // Replace current node in tree with new node
    ଶୂନ୍ୟ replaceWithKeepDType(Ast_ଗଣ୍ଠି* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_ଗଣ୍ଠି* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_ଗଣ୍ଠି* unlinkFrBackWithNext(VNRelinker* linkerp = nullptr);
    ଶୂନ୍ୟ relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    ଶୂନ୍ୟ clone_Relink_ଗଣ୍ଠି() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_ଗଣ୍ଠି* clone_ବୃକ୍ଷ(bool cloneNextLink,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_ଗଣ୍ଠି* clone_ବୃକ୍ଷ_Pure(bool cloneNextLink) { ଫେରନ୍ତୁ clone_ବୃକ୍ଷ(cloneNextLink, true); }
    bool gate_ବୃକ୍ଷ() { ଫେରନ୍ତୁ gate_ବୃକ୍ଷ_Iter(); }  // Is tree isGateOptimizable?
    ଧାଡ଼ିଭିତରେ bool same_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* node2p) ଅଚଳ;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    ଧାଡ଼ିଭିତରେ bool same_Gate_ବୃକ୍ଷ(ଅଚଳ Ast_ଗଣ୍ଠି* node2p) ଅଚଳ;
    ଶୂନ୍ୟ delete_ବୃକ୍ଷ();  // Always deletes the next link
    ଶୂନ୍ୟ check_ବୃକ୍ଷ() ଅଚଳ VL_MT_STABLE {
        ଯଦି (v3Global.opt.debugCheck()) check_ବୃକ୍ଷ_Iter(backp());
    }
    ଶୂନ୍ୟ checkIter() ଅଚଳ;
    ଶୂନ୍ୟ dumpPtrs(std::ostream& os = std::cout) ଅଚଳ;
    ଶୂନ୍ୟ dump_ବୃକ୍ଷ(std::ostream& os = std::cout, ଅଚଳ string& indent = "    ",
                  ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxDepth = 0) ଅଚଳ;
    ଶୂନ୍ୟ dump_ବୃକ୍ଷ(ଅଚଳ string& indent, ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxDepth = 0) ଅଚଳ {
        dump_ବୃକ୍ଷ(std::cout, indent, maxDepth);
    }
    ସ୍ଥିର ଶୂନ୍ୟ dump_ବୃକ୍ଷ_Gdb(ଅଚଳ Ast_ଗଣ୍ଠି* nodep);  // For GDB only
    ଶୂନ୍ୟ dump_ବୃକ୍ଷ_And_Next(std::ostream& os = std::cout, ଅଚଳ string& indent = "    ",
                         ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxDepth = 0) ଅଚଳ;
    ଶୂନ୍ୟ dump_ବୃକ୍ଷ_File(ଅଚଳ string& filename, bool doDump = true);
    ସ୍ଥିର ଶୂନ୍ୟ dump_ବୃକ୍ଷ_File_Gdb(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ ଅକ୍ଷର* filenamep = nullptr);
    ଶୂନ୍ୟ dump_ବୃକ୍ଷ_Dot(std::ostream& os = std::cout) ଅଚଳ;
    ଶୂନ୍ୟ dump_ବୃକ୍ଷ_Dot_File(ଅଚଳ string& filename, bool doDump = true);
    ଆଭାସୀ ଶୂନ୍ୟ dumpJson(std::ostream& os) ଅଚଳ { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    ଶୂନ୍ୟ dumpJsonGen(std::ostream& os) ଅଚଳ {};
    ଆଭାସୀ ଶୂନ୍ୟ dump_ବୃକ୍ଷ_Json_Op_Gen(std::ostream& os, ଅଚଳ string& indent) ଅଚଳ {};
    ଶୂନ୍ୟ dump_ବୃକ୍ଷ_Json(std::ostream& os, ଅଚଳ string& indent = "") ଅଚଳ;
    ଶୂନ୍ୟ dump_ବୃକ୍ଷ_Json_File(ଅଚଳ string& filename, bool doDump = true);
    ସ୍ଥିର ଶୂନ୍ୟ dumpJsonMetaFileGdb(ଅଚଳ ଅକ୍ଷର* filename);
    ସ୍ଥିର ଶୂନ୍ୟ dumpJsonMetaFile(ଅଚଳ string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    ସ୍ଥିର std::string ଗଣ୍ଠି_Addr(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଫେରନ୍ତୁ v3Global.opt.dump_ବୃକ୍ଷ_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    ସ୍ଥିର Ast_ଗଣ୍ଠି* afterCommentp(Ast_ଗଣ୍ଠି* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        ଯେତେବେଳେ (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        ଫେରନ୍ତୁ nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    ଆଭାସୀ bool isBrancher() ଅଚଳ { ଫେରନ୍ତୁ false; }
    // Else a AstTime etc that can't be pushed out
    ଆଭାସୀ bool isGateOptimizable() ଅଚଳ { ଫେରନ୍ତୁ !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    ଆଭାସୀ bool isGateDedupable() ଅଚଳ { ଫେରନ୍ତୁ isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    ଆଭାସୀ bool is_ପଦସମଷ୍ଟି_Coverage_Eligible() ଅଚଳ { ଫେରନ୍ତୁ isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    ଆଭାସୀ bool isOutputter() { ଫେରନ୍ତୁ false; }
    // Else a AstTime etc which output can't be predicted from input
    ଆଭାସୀ bool isPredictOptimizable() ଅଚଳ { ଫେରନ୍ତୁ !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    ଆଭାସୀ bool isPure() { ଫେରନ୍ତୁ true; }
    // Iff isPure on current node and any nextp()
    bool isPureAndNext() { ଫେରନ୍ତୁ isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    ଆଭାସୀ bool isTimingControl() ଅଚଳ { ଫେରନ୍ତୁ false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    ଆଭାସୀ bool isUnlikely() ଅଚଳ { ଫେରନ୍ତୁ false; }
    // Is an IEEE system function (versus internally-generated)
    ଆଭାସୀ bool isSystemFunc() ଅଚଳ { ଫେରନ୍ତୁ false; }
    ଆଭାସୀ ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ 0; }
    // Iff node is identical to another node
    ଆଭାସୀ bool isSame(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ {
        ଫେରନ୍ତୁ type() == samep->type() && same_ଗଣ୍ଠି(samep);
    }
    // Iff has a data type; dtype() must be non-null
    ଆଭାସୀ bool hasDType() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ false; }
    // Iff has a non-null childDTypep(), as generic node function
    ଆଭାସୀ Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ { ଫେରନ୍ତୁ nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    ଆଭାସୀ Ast_ଗଣ୍ଠି_DType* getChild2DTypep() ଅଚଳ { ଫେରନ୍ତୁ nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    ଆଭାସୀ bool maybePointedTo() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ false; }
    // Don't reclaim this node in V3Dead
    ଆଭାସୀ bool undead() ଅଚଳ { ଫେରନ୍ତୁ false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    ଆଭାସୀ ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ { ଫେରନ୍ତୁ nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    ଆଭାସୀ ଅଚଳ ଅକ୍ଷର* brokenGen() ଅଚଳ = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ newp) ଅଚଳ { ଫେରନ୍ତୁ backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    ଆଭାସୀ ଶୂନ୍ୟ accept(VNVisitorConst& v) = 0;

ସୁରକ୍ଷିତ:
    // All VNVisitor related functions are called as methods off the visitor
    friend class VNVisitor;
    friend class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    ଶୂନ୍ୟ iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    ଶୂନ୍ୟ iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    ଶୂନ୍ୟ iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    ଶୂନ୍ୟ iterateAndNext(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    ଶୂନ୍ୟ iterateAndNextConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_ଗଣ୍ଠି* iterateSubtreeReturnEdits(VNVisitor& v);

    ସ୍ଥିର ଶୂନ୍ୟ dump_Json_ଅଙ୍କ(std::ostream& os, ଅଚଳ std::string& name, int64_t val);
    ସ୍ଥିର ଶୂନ୍ୟ dumpJsonBoolIf(std::ostream& os, ଅଚଳ std::string& name, bool val);
    ସ୍ଥିର ଶୂନ୍ୟ dumpJsonStr(std::ostream& os, ଅଚଳ std::string& name, ଅଚଳ std::string& val);
    ସ୍ଥିର ଶୂନ୍ୟ dumpJsonPtr(std::ostream& os, ଅଚଳ std::string& name, ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ valp);

ସୁରକ୍ଷିତ:
    ଶୂନ୍ୟ iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    template <typename T>
    ଧାଡ଼ିଭିତରେ ସ୍ଥିର bool privateTypeTest(ଅଚଳ Ast_ଗଣ୍ଠି* nodep);

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr ସ୍ଥିର bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        ଫେରନ୍ତୁ std::is_base_of<T_TargetType, NonCV>::ମୂଲ୍ୟ;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    template <typename T_TargetType, typename T_DeclType>
    constexpr ସ୍ଥିର bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        ଫେରନ୍ତୁ !std::is_base_of<NonCV, T_TargetType>::ମୂଲ୍ୟ;
    }  // LCOV_EXCL_STOP

ସର୍ବସାଧାରଣ:
    // For use via the VN_IS macro only, or in templated code
    template <typename T_TargetType, typename T_ଗଣ୍ଠି>
    ସ୍ଥିର bool is(ଅଚଳ T_ଗଣ୍ଠି* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        ଫେରନ୍ତୁ nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    template <typename T_TargetType, typename T_ଗଣ୍ଠି>
    ସ୍ଥିର T_TargetType* cast(T_ଗଣ୍ଠି* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        ଫେରନ୍ତୁ nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    template <typename T_TargetType, typename T_ଗଣ୍ଠି>
    ସ୍ଥିର ଅଚଳ T_TargetType* cast(ଅଚଳ T_ଗଣ୍ଠି* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        ଫେରନ୍ତୁ nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<ଅଚଳ T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    template <typename T_TargetType, typename T_ଗଣ୍ଠି>
    ସ୍ଥିର T_TargetType* as(T_ଗଣ୍ଠି* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        ଫେରନ୍ତୁ unsafePrivateAs<T_TargetType, T_ଗଣ୍ଠି>(nodep);
    }
    template <typename T_TargetType, typename T_ଗଣ୍ଠି>
    ସ୍ଥିର ଅଚଳ T_TargetType* as(ଅଚଳ T_ଗଣ୍ଠି* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        ଫେରନ୍ତୁ unsafePrivateAs<T_TargetType, T_ଗଣ୍ଠି>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    template <typename T_TargetType, typename T_ଗଣ୍ଠି>
    ସ୍ଥିର T_TargetType* unsafePrivateAs(T_ଗଣ୍ଠି* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        ଫେରନ୍ତୁ reinterpret_cast<T_TargetType*>(nodep);
    }
    template <typename T_TargetType, typename T_ଗଣ୍ଠି>
    ସ୍ଥିର ଅଚଳ T_TargetType* unsafePrivateAs(ଅଚଳ T_ଗଣ୍ଠି* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_ଗଣ୍ଠି>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        ଫେରନ୍ତୁ reinterpret_cast<ଅଚଳ T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_ଗଣ୍ଠି>
    ସ୍ଥିର bool mayBeUnder(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        static_assert(!std::is_const<T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        ଫେରନ୍ତୁ true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    template <typename T_ଗଣ୍ଠି>
    ସ୍ଥିର constexpr bool isLeaf() {
        static_assert(!std::is_const<T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        ଫେରନ୍ତୁ false;
    }

ବ୍ୟକ୍ତିଗତ:
    // Using std::conditional for const correctness in the public 'foreach' functions
    template <typename T_Arg>
    using Const_Correct_Ast_ଗଣ୍ଠି =
        typename std::conditional<std::is_const<T_Arg>::ମୂଲ୍ୟ, ଅଚଳ Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>::type;

    template <typename T_Arg, typename T_Callable>
    ଧାଡ଼ିଭିତରେ ସ୍ଥିର ଶୂନ୍ୟ foreachImpl(Const_Correct_Ast_ଗଣ୍ଠି<T_Arg>* nodep, ଅଚଳ T_Callable& f,
                                   bool visitNext);

    template <typename T_Arg, bool N_Default, typename T_Callable>
    ଧାଡ଼ିଭିତରେ ସ୍ଥିର bool predicateImpl(Const_Correct_Ast_ଗଣ୍ଠି<T_Arg>* nodep, ଅଚଳ T_Callable& p,
                                     bool visitNext);

ସର୍ବସାଧାରଣ:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    template <typename T_Callable>
    ଶୂନ୍ୟ foreach(T_Callable&& f) {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                          && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_ଗଣ୍ଠି>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    ଶୂନ୍ୟ foreach(T_Callable&& f) ଅଚଳ {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ଅଚଳ T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ଅଚଳ T_ଗଣ୍ଠି>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    ଶୂନ୍ୟ foreachAndNext(T_Callable&& f) {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                          && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_ଗଣ୍ଠି>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    ଶୂନ୍ୟ foreachAndNext(T_Callable&& f) ଅଚଳ {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, ଅଚଳ T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<ଅଚଳ T_ଗଣ୍ଠି>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    template <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                          && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ଫେରନ୍ତୁ predicateImpl<T_ଗଣ୍ଠି, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool exists(T_Callable&& p) ଅଚଳ {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ଅଚଳ T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                          && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ଫେରନ୍ତୁ predicateImpl<ଅଚଳ T_ଗଣ୍ଠି, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ଅଚଳ T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                          && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ଫେରନ୍ତୁ predicateImpl<T_ଗଣ୍ଠି, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool existsAndNext(T_Callable&& p) ଅଚଳ {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ଅଚଳ T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                          && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ଫେରନ୍ତୁ predicateImpl<ଅଚଳ T_ଗଣ୍ଠି, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    template <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                          && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ଫେରନ୍ତୁ predicateImpl<T_ଗଣ୍ଠି, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    template <typename T_Callable>
    bool forall(T_Callable&& p) ଅଚଳ {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, ଅଚଳ T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                          && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        ଫେରନ୍ତୁ predicateImpl<ଅଚଳ T_ଗଣ୍ଠି, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣ୍ଠି_ଗଣନା() ଅଚଳ {
        // TODO: this should really return size_t, but need to fix use sites
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣନା_2 = 0;
        this->foreach([&ଗଣନା_2](ଅଚଳ Ast_ଗଣ୍ଠି*) { ++ଗଣନା_2; });
        ଫେରନ୍ତୁ ଗଣନା_2;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(ପୂର୍ଣ୍ଣସଂଖ୍ୟା limit) ଅଚଳ {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣନା_2 = 0;
        ଫେରନ୍ତୁ this->exists([&ଗଣନା_2, limit](ଅଚଳ Ast_ଗଣ୍ଠି*) { ଫେରନ୍ତୁ ++ଗଣନା_2 > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
template <>
Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* newp);

// Inline method implementations
Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::addNext(Ast_ଗଣ୍ଠି* newp) { ଫେରନ୍ତୁ addNext(this, newp); }

// Specializations of privateTypeTest
#ଅନ୍ତର୍ଭାଵ "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
template <>
ଧାଡ଼ିଭିତରେ bool Ast_ଗଣ୍ଠି::mayBeUnder<AstCell>(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଫେରନ୍ତୁ !VN_IS(nodep, ଗଣ୍ଠି_Stmt) && !VN_IS(nodep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
}
template <>
ଧାଡ଼ିଭିତରେ bool Ast_ଗଣ୍ଠି::mayBeUnder<Ast_ଗଣ୍ଠି_Assign>(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଫେରନ୍ତୁ !VN_IS(nodep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
}
template <>
ଧାଡ଼ିଭିତରେ bool Ast_ଗଣ୍ଠି::mayBeUnder<AstVarScope>(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (VN_IS(nodep, VarScope)) ଫେରନ୍ତୁ false;  // Should not nest
    ଯଦି (VN_IS(nodep, Var)) ଫେରନ୍ତୁ false;
    ଯଦି (VN_IS(nodep, Active)) ଫେରନ୍ତୁ false;
    ଯଦି (VN_IS(nodep, ଗଣ୍ଠି_Stmt)) ଫେରନ୍ତୁ false;
    ଯଦି (VN_IS(nodep, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) ଫେରନ୍ତୁ false;
    ଫେରନ୍ତୁ true;
}
template <>
ଧାଡ଼ିଭିତରେ bool Ast_ଗଣ୍ଠି::mayBeUnder<AstExecGraph>(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (VN_IS(nodep, ExecGraph)) ଫେରନ୍ତୁ false;  // Should not nest
    ଯଦି (VN_IS(nodep, ଗଣ୍ଠି_Stmt)) ଫେରନ୍ତୁ false;  // Should be directly under CFunc
    ଫେରନ୍ତୁ true;
}
template <>
ଧାଡ଼ିଭିତରେ bool Ast_ଗଣ୍ଠି::mayBeUnder<AstActive>(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଫେରନ୍ତୁ !VN_IS(nodep, Active);  // AstActives do not nest
}
template <>
ଧାଡ଼ିଭିତରେ bool Ast_ଗଣ୍ଠି::mayBeUnder<AstScope>(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଫେରନ୍ତୁ !VN_IS(nodep, Scope);  // AstScopes do not nest
}
template <>
ଧାଡ଼ିଭିତରେ bool Ast_ଗଣ୍ଠି::mayBeUnder<Ast_Sen_ବୃକ୍ଷ>(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଫେରନ୍ତୁ !VN_IS(nodep, Sen_ବୃକ୍ଷ);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
template <>
constexpr bool Ast_ଗଣ୍ଠି::isLeaf<Ast_ଗଣ୍ଠି_Var_Ref>() {
    ଫେରନ୍ତୁ true;
}
template <>
constexpr bool Ast_ଗଣ୍ଠି::isLeaf<AstVarRef>() {
    ଫେରନ୍ତୁ true;
}
template <>
constexpr bool Ast_ଗଣ୍ଠି::isLeaf<AstVarXRef>() {
    ଫେରନ୍ତୁ true;
}

// foreach implementation
template <typename T_Arg, typename T_Callable>
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::foreachImpl(Const_Correct_Ast_ଗଣ୍ଠି<T_Arg>* nodep, ଅଚଳ T_Callable& f, bool visitNext) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using ଗଣ୍ଠି_2 = Const_Correct_Ast_ଗଣ୍ଠି<T_Arg>;

    // We prefetch this far into the stack
    constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_ଗଦା_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t ଗଦା_Size = Initial_ଗଦା_Size;
    ଗଣ୍ଠି_2* in_ଧାଡ଼ି_ଗଦା[Initial_ଗଦା_Size];  // In-line initial stack storage
    std::unique_ptr<ଗଣ୍ଠି_2*[]> out_Of_ଧାଡ଼ି_Stackp{nullptr};  // In case we need more, we will allocate
    ଗଣ୍ଠି_2** basep = in_ଧାଡ଼ି_ଗଦା + PrefetchDistance;  // Pointer to base of stack
    ଗଣ୍ଠି_2** topp = basep;  // Pointer to top of stack
    ଗଣ୍ଠି_2** limp = in_ଧାଡ଼ି_ଗଦା + ଗଦା_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    ଅଚଳ auto visit = [&](ଗଣ୍ଠି_2* currp) {
        // Type test this node
        ଯଦି (Ast_ଗଣ୍ଠି::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            ଯଦି VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) ଫେରନ୍ତୁ;
        }

        // Enqueue children for traversal, unless futile
        ଯଦି (mayBeUnder<T_Arg_NonConst>(currp)) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ op4p = currp->op4p()) *topp++ = op4p;
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ op3p = currp->op3p()) *topp++ = op3p;
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ op2p = currp->op2p()) *topp++ = op2p;
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    ଯଦି (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    ଯେତେବେଳେ (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        ଗଣ୍ଠି_2* ଅଚଳ headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        ଯଦି (VL_UNLIKELY(topp >= limp)) {
            ଅଚଳ ptrdiff_t occupancy = topp - basep;
            {
                ଅଚଳ size_t new_ଗଦା_Size = ଗଦା_Size * 2;
                ଗଣ୍ଠି_2** ଅଚଳ newStackp = new ଗଣ୍ଠି_2*[new_ଗଦା_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            ଆକାର(ଗଣ୍ଠି_2**) * (occupancy + PrefetchDistance));
                ଗଦା_Size = new_ଗଦା_Size;
                out_Of_ଧାଡ଼ି_Stackp.reset(newStackp);
            }
            basep = out_Of_ଧାଡ଼ି_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_ଧାଡ଼ି_Stackp.get() + ଗଦା_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        ଯଦି (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
template <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_ଗଣ୍ଠି::predicateImpl(Const_Correct_Ast_ଗଣ୍ଠି<T_Arg>* nodep, ଅଚଳ T_Callable& p,
                            bool visitNext) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using ଗଣ୍ଠି_2 = Const_Correct_Ast_ଗଣ୍ଠି<T_Arg>;

    // We prefetch this far into the stack
    constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr size_t MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr size_t Initial_ଗଦା_Size = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    size_t ଗଦା_Size = Initial_ଗଦା_Size;
    ଗଣ୍ଠି_2* in_ଧାଡ଼ି_ଗଦା[Initial_ଗଦା_Size];  // In-line initial stack storage
    std::unique_ptr<ଗଣ୍ଠି_2*[]> out_Of_ଧାଡ଼ି_Stackp{nullptr};  // In case we need more, we will allocate
    ଗଣ୍ଠି_2** basep = in_ଧାଡ଼ି_ଗଦା + PrefetchDistance;  // Pointer to base of stack
    ଗଣ୍ଠି_2** topp = basep;  // Pointer to top of stack
    ଗଣ୍ଠି_2** limp = in_ଧାଡ଼ି_ଗଦା + ଗଦା_Size - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    ଅଚଳ auto visit = [&](ଗଣ୍ଠି_2* currp) {
        // Type test this node
        ଯଦି (Ast_ଗଣ୍ଠି::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            ଯଦି (p(static_cast<T_Arg*>(currp)) != N_Default) ଫେରନ୍ତୁ true;
            // Short circuit if iterating leaf nodes
            ଯଦି VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) ଫେରନ୍ତୁ false;
        }

        // Enqueue children for traversal, unless futile
        ଯଦି (mayBeUnder<T_Arg_NonConst>(currp)) {
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ op4p = currp->op4p()) *topp++ = op4p;
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ op3p = currp->op3p()) *topp++ = op3p;
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ op2p = currp->op2p()) *topp++ = op2p;
            ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ op1p = currp->op1p()) *topp++ = op1p;
        }

        ଫେରନ୍ତୁ false;
    };

    // Enqueue the next of the root node, if required
    ଯଦି (visitNext && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    ଯଦି (visit(nodep)) ଫେରନ୍ତୁ !N_Default;

    // Visit the rest of the tree
    ଯେତେବେଳେ (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        ଗଣ୍ଠି_2* ଅଚଳ headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        ଯଦି (VL_UNLIKELY(topp >= limp)) {
            ଅଚଳ ptrdiff_t occupancy = topp - basep;
            {
                ଅଚଳ size_t new_ଗଦା_Size = ଗଦା_Size * 2;
                ଗଣ୍ଠି_2** ଅଚଳ newStackp = new ଗଣ୍ଠି_2*[new_ଗଦା_Size];
                std::memcpy(newStackp, basep - PrefetchDistance,
                            ଆକାର(ଗଣ୍ଠି_2**) * (occupancy + PrefetchDistance));
                ଗଦା_Size = new_ଗଦା_Size;
                out_Of_ଧାଡ଼ି_Stackp.reset(newStackp);
            }
            basep = out_Of_ଧାଡ଼ି_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_ଧାଡ଼ି_Stackp.get() + ଗଦା_Size - MaxPushesPerIteration;
        }

        // Enqueue the next node
        ଯଦି (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        ଯଦି (visit(headp)) ଫେରନ୍ତୁ !N_Default;
    }

    ଫେରନ୍ତୁ N_Default;
}

ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ Ast_ଗଣ୍ଠି* rhs) {
    ଯଦି (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } ଅନ୍ୟଥା ଯଦି (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } ଅନ୍ୟଥା {
        rhs->dump(os);
    }
    ଫେରନ୍ତୁ os;
}
ଶୂନ୍ୟ VNRelinker::relink(Ast_ଗଣ୍ଠି* newp) { newp->Ast_ଗଣ୍ଠି::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
template <typename T_ଗଣ୍ଠି>
class VNRef final : ସର୍ବସାଧାରଣ std::reference_wrapper<T_ଗଣ୍ଠି> {
    static_assert(std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

ସର୍ବସାଧାରଣ:
    template <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_ଗଣ୍ଠି>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(ଅଚଳ std::reference_wrapper<T_ଗଣ୍ଠି>& other)
        : std::reference_wrapper<T_ଗଣ୍ଠି>{other} {}
};

static_assert(ଆକାର(VNRef<Ast_ଗଣ୍ଠି>) == ଆକାର(std::reference_wrapper<Ast_ଗଣ୍ଠି>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
size_t V3HasherUncachedHash(ଅଚଳ Ast_ଗଣ୍ଠି&);

// Specialization of std::hash for VNRef
template <typename T_ଗଣ୍ଠି>
ସଂରଚନା std::hash<VNRef<T_ଗଣ୍ଠି>> final {
    size_t operator()(VNRef<T_ଗଣ୍ଠି> r) ଅଚଳ { ଫେରନ୍ତୁ V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
template <typename T_ଗଣ୍ଠି>
ସଂରଚନା std::equal_to<VNRef<T_ଗଣ୍ଠି>> final {
    size_t operator()(VNRef<T_ଗଣ୍ଠି> ra, VNRef<T_ଗଣ୍ଠି> rb) ଅଚଳ {
        ଫେରନ୍ତୁ ra.get().same_ବୃକ୍ଷ(&(rb.get()));
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
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି*, Ast_ଗଣ୍ଠି*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି*, std::vector<Ast_ଗଣ୍ଠି*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି*, std::string> m_prefixes;
    ଅଚଳ VDTypeNameScopes* ଅଚଳ m_prevp;  // Index in use before this one
    ସ୍ଥିର ଅଚଳ VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    ଶୂନ୍ୟ index(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    ସ୍ଥିର ଅଚଳ VDTypeNameScopes* currentp() { ଫେରନ୍ତୁ s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି*& outerpr) ଅଚଳ;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    ଅଚଳ std::vector<Ast_ଗଣ୍ଠି*>* paramsp(ଅଚଳ Ast_ଗଣ୍ଠି* modp) ଅଚଳ;
    // The prefix kept for scope 'scopep', or nullptr if none
    ଅଚଳ std::string* prefixp(ଅଚଳ Ast_ଗଣ୍ଠି* scopep) ଅଚଳ;
    // Keep 'prefix', final, for scope 'scopep'
    ଶୂନ୍ୟ prefix(ଅଚଳ Ast_ଗଣ୍ଠି* scopep, ଅଚଳ std::string& prefix) ଅଚଳ;
};

//######################################################################
// Inline VNVisitor METHODS

ଶୂନ୍ୟ VNVisitorConst::iterateConst(Ast_ଗଣ୍ଠି* nodep) { nodep->accept(*this); }
ଶୂନ୍ୟ VNVisitorConst::iterateConstNull(Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (VL_LIKELY(nodep)) nodep->accept(*this);
}
ଶୂନ୍ୟ VNVisitorConst::iterateChildrenConst(Ast_ଗଣ୍ଠି* nodep) { nodep->iterateChildrenConst(*this); }
ଶୂନ୍ୟ VNVisitorConst::iterateChildrenBackwardsConst(Ast_ଗଣ୍ଠି* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
ଶୂନ୍ୟ VNVisitorConst::iterateAndNextConstNull(Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (VL_LIKELY(nodep)) nodep->iterateAndNextConst(*this);
}

ଶୂନ୍ୟ VNVisitor::iterate(Ast_ଗଣ୍ଠି* nodep) { nodep->accept(*this); }
ଶୂନ୍ୟ VNVisitor::iterateNull(Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (VL_LIKELY(nodep)) nodep->accept(*this);
}
ଶୂନ୍ୟ VNVisitor::iterateChildren(Ast_ଗଣ୍ଠି* nodep) { nodep->iterateChildren(*this); }
ଶୂନ୍ୟ VNVisitor::iterateAndNextNull(Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (VL_LIKELY(nodep)) nodep->iterateAndNext(*this);
}
Ast_ଗଣ୍ଠି* VNVisitor::iterateSubtreeReturnEdits(Ast_ଗଣ୍ଠି* nodep) {
    ଫେରନ୍ତୁ nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#ଅନ୍ତର୍ଭାଵ "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#ଅନ୍ତର୍ଭାଵ "V3AstNodeDType.h"
#ଅନ୍ତର୍ଭାଵ "V3AstNodeOther.h"
#ଅନ୍ତର୍ଭାଵ "V3AstNodeExpr.h"
#ଅନ୍ତର୍ଭାଵ "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#ଅନ୍ତର୍ଭାଵ "V3AstInlines.h"
ଶୂନ୍ୟ dump_ଗଣ୍ଠି_List_Json(std::ostream& os, ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ std::string& listName,
                      ଅଚଳ string& indent);

#endif  // Guard
