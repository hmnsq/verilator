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

#నిర్వచితం_కాకపోతే VERILATOR_V3AST_H_
#నిర్వచించు VERILATOR_V3AST_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Broken.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3FunctionTraits.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Number.h"
#అన్తర్భావ "V3StdFuture.h"

#అన్తర్భావ "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#అన్తర్భావ <array>
#అన్తర్భావ <cmath>
#అన్తర్భావ <cstdint>
#అన్తర్భావ <functional>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <type_traits>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#ఒకవేళ defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#నిర్వచనం_రద్దు VL_USER_TYPE_CHECKS
#అయితే_ముగింపు
#నిర్వచితమైతే VL_USER_TYPE_CHECKS
#అన్తర్భావ <variant>
#అయితే_ముగింపు

// clang-format off
#అన్తర్భావ "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3_లేఖాచిత్రం;
class ExecMTask;
class VIface_Capture_ట్యాగ్;

//######################################################################

// For broken() function, return error string if have a match
#నిర్వచించు BROKEN_RTN(test) \
    చేయి { \
        ఒకవేళ (VL_UNCOVERABLE(test)) తిరిగి "'" #test "' @ " __FILE__ ":" VL_STRINGIFY(__LINE__); \
    } అయితే (false)
// For broken() function, return error string if a base of this class has a match
#నిర్వచించు BROKEN_పునాది_RTN(test) \
    చేయి { \
        స్థిరరాశి అక్షరం* స్థిరరాశి reasonp = (test); \
        ఒకవేళ (VL_UNCOVERABLE(reasonp)) తిరిగి reasonp; \
    } అయితే (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#నిర్వచించు VN_IS(nodep, nodetypename) (Ast_కణుపు::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#నిర్వచించు VN_CAST(nodep, nodetypename) (Ast_కణుపు::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#నిర్వచించు VN_AS(nodep, nodetypename) (Ast_కణుపు::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#ఒకవేళ defined(VL_DEBUG) || defined(VL_ASAN)
#నిర్వచించు VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#లేకపోతే
#నిర్వచించు VN_DBG_AS(nodep, nodetypename) (Ast_కణుపు::unsafe_వ్యక్తిగత_As<Ast##nodetypename>(nodep))
#అయితే_ముగింపు

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#నిర్వచించు VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

నిర్మాణం VNTypeInfo final {
    స్థిరరాశి అక్షరం* m_namep;
    గణన OpEn : uint8_t {
        OP_వాడని,
        OP_వాడిన,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    స్థిరరాశి అక్షరం* m_opNamep[4];
    కొలత_t m_sizeof;
};

class VNType final {
    స్థిర స్థిరరాశి VNTypeInfo s_type_Info_పట్టిక[];

బహిరంగ:
#అన్తర్భావ "V3Ast__gen_type_enum.h"  // From ./astgen
    స్థిరరాశి en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_సురక్షిత : m_e{_e} {}
    constexpr క్రియాకారకం en() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e; }

    // Retrieve VNTypeInfo for the given type
    స్థిర స్థిరరాశి VNTypeInfo& typeInfo(VNType t) VL_MT_సురక్షిత { తిరిగి s_type_Info_పట్టిక[t.m_e]; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VNType& lhs, స్థిరరాశి VNType& rhs) VL_శుద్ధ {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VNType& lhs, VNType::en rhs) VL_శుద్ధ { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VNType::en lhs, స్థిరరాశి VNType& rhs) VL_శుద్ధ { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VNType& rhs) { తిరిగి os << rhs.ascii(); }

//######################################################################
// AstNUser - Generic base class for AST User nodes.
//          - Also used to allow parameter passing up/down iterate calls

class వెడల్పు_VP;
class V3_లేఖాచిత్రం_శీర్షం;
class VSymEnt;

class VNUser final {
// Defining VL_USER_TYPE_CHECKS replaces the VNUser union with a std::variant
// which remembers which form of VNUser is being stored to catch mismatched reads.
#నిర్వచితమైతే VL_USER_TYPE_CHECKS
    // monostate is an unwritten / cleared slot. It can be read as either form
    // and yields nullptr/0.
    std::variant<std::monostate, uint64_t, శూన్య*> m_u;
#లేకపోతే
    సంఘం {
        శూన్య* పైకి;
        uint64_t uq;
    } m_u;
#అయితే_ముగింపు

బహిరంగ:
#నిర్వచితమైతే VL_USER_TYPE_CHECKS
    VNUser() = అప్రమేయ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        ఒకవేళ (i) m_u = i;
    }
    explicit VNUser(శూన్య* p) {
        // VNUser{nullptr} represents the monostate
        ఒకవేళ (p) m_u = p;
    }
    ~VNUser() = అప్రమేయ;
    // Casters
    మూస <typename T>
    typename std::enable_if<std::is_సూచిక<T>::విలువ, T>::type గమ్యం() స్థిరరాశి VL_MT_సురక్షిత {
        ఒకవేళ (std::holds_alternative<std::monostate>(m_u)) తిరిగి nullptr;
        శూన్య* స్థిరరాశి* స్థిరరాశి upp = std::get_if<శూన్య*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        తిరిగి reinterpret_cast<T>(*upp);
    }
    uint64_t గమ్యం_UQuad() స్థిరరాశి {
        ఒకవేళ (std::holds_alternative<std::monostate>(m_u)) తిరిగి 0;
        స్థిరరాశి uint64_t* స్థిరరాశి uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        తిరిగి *uip;
    }
#లేకపోతే
    VNUser() = అప్రమేయ;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.పైకి = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(శూన్య* p) { m_u.పైకి = p; }
    ~VNUser() = అప్రమేయ;
    // Casters
    మూస <typename T>
    typename std::enable_if<std::is_సూచిక<T>::విలువ, T>::type గమ్యం() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి reinterpret_cast<T>(m_u.పైకి);
    }
    uint64_t గమ్యం_UQuad() స్థిరరాశి { తిరిగి m_u.uq; }
#అయితే_ముగింపు
    VSymEnt* గమ్యం_Sym_Ent() స్థిరరాశి { తిరిగి గమ్యం<VSymEnt*>(); }
    Ast_కణుపు* గమ్యం_Nodep() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి గమ్యం<Ast_కణుపు*>(); }
    V3_లేఖాచిత్రం_శీర్షం* గమ్యం_లేఖాచిత్రం_శీర్షం() స్థిరరాశి { తిరిగి గమ్యం<V3_లేఖాచిత్రం_శీర్షం*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(స్థిరరాశి శూన్య*)) స్థిరరాశి;
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

class VNUser_లోపల_Use_పునాది
VL_NOT_FINAL{రక్షిత : స్థిర శూన్య allocate(పూర్ణాంకం id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvt_గమ్యం_Str(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
స్థిర శూన్య విడుదల(పూర్ణాంకం id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvt_గమ్యం_Str(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
స్థిర శూన్య clearcnt(పూర్ణాంకం id, uint32_t& cntGblRef, స్థిరరాశి bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvt_గమ్యం_Str(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
స్థిర శూన్య checkcnt(పూర్ణాంకం id, uint32_t&, స్థిరరాశి bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef,
                   "Check of User" + cvt_గమ్యం_Str(id) + "() failed, not under AstUserInUse");
}
}
;

// For each user() declare the in use structure
// We let AstNode peek into here, because when under low optimization even
// an accessor would be way too slow.
// clang-format off
class VNUser1_లోపల_Use final : VNUser_లోపల_Use_పునాది {
రక్షిత:
    మిత్ర class Ast_కణుపు;
    స్థిర uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    స్థిర bool s_userBusy;  // Count is in use
బహిరంగ:
    VNUser1_లోపల_Use()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1_లోపల_Use()     { విడుదల    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    స్థిర శూన్య clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    స్థిర శూన్య check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2_లోపల_Use final : VNUser_లోపల_Use_పునాది {
రక్షిత:
    మిత్ర class Ast_కణుపు;
    స్థిర uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    స్థిర bool s_userBusy;  // Count is in use
బహిరంగ:
    VNUser2_లోపల_Use()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2_లోపల_Use()     { విడుదల    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    స్థిర శూన్య clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    స్థిర శూన్య check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3_లోపల_Use final : VNUser_లోపల_Use_పునాది {
రక్షిత:
    మిత్ర class Ast_కణుపు;
    స్థిర uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    స్థిర bool s_userBusy;  // Count is in use
బహిరంగ:
    VNUser3_లోపల_Use()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3_లోపల_Use()     { విడుదల    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    స్థిర శూన్య clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    స్థిర శూన్య check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4_లోపల_Use final : VNUser_లోపల_Use_పునాది {
రక్షిత:
    మిత్ర class Ast_కణుపు;
    స్థిర uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    స్థిర bool s_userBusy;  // Count is in use
బహిరంగ:
    VNUser4_లోపల_Use()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4_లోపల_Use()     { విడుదల    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    స్థిర శూన్య clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    స్థిర శూన్య check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::సదిశ<Ast_కణుపు*> m_deleteps;  // Nodes to delete

బహిరంగ:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    శూన్య నెట్టు_Deletep(Ast_కణుపు* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.నెట్టు_వెనుక(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    శూన్య doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    మిత్ర class Ast_కణుపు;

బహిరంగ:
    /// Call visit()s on nodep
    అంతఃవరుస శూన్య iterateConst(Ast_కణుపు* nodep);
    /// Call visit()s on nodep
    అంతఃవరుస శూన్య iterateConstNull(Ast_కణుపు* nodep);
    /// Call visit()s on const nodep's children
    అంతఃవరుస శూన్య iterateChildrenConst(Ast_కణుపు* nodep);
    /// Call visit()s on nodep's children in backp() order
    అంతఃవరుస శూన్య iterateChildrenBackwardsConst(Ast_కణుపు* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    అంతఃవరుస శూన్య iterate_And_తదుపరి_Const_Null(Ast_కణుపు* nodep);

    మాయా శూన్య సందర్శించు(Ast_కణుపు* nodep) = 0;
    మాయా ~VNVisitorConst() {}
#అన్తర్భావ "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : బహిరంగ VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
బహిరంగ:
    /// Call visit()s on nodep
    అంతఃవరుస శూన్య iterate(Ast_కణుపు* nodep);
    /// Call visit()s on nodep
    అంతఃవరుస శూన్య iterateNull(Ast_కణుపు* nodep);
    /// Call visit()s on nodep's children
    అంతఃవరుస శూన్య iterateChildren(Ast_కణుపు* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    అంతఃవరుస శూన్య iterate_And_తదుపరి_Null(Ast_కణుపు* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    అంతఃవరుస Ast_కణుపు* iterateSubtreeReturnEdits(Ast_కణుపు* nodep);

    VNDeleter& deleter() { తిరిగి m_deleter; }
    శూన్య నెట్టు_Deletep(Ast_కణుపు* nodep) { deleter().నెట్టు_Deletep(nodep); }
    శూన్య doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
రక్షిత:
    మిత్ర class Ast_కణుపు;
    గణన RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_తదుపరి,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_కణుపు* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_కణుపు* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_కణుపు** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

బహిరంగ:
    VNRelinker() = అప్రమేయ;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    అంతఃవరుస శూన్య relink(Ast_కణుపు* newp);
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి;
};
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VNRelinker& rhs) {
    rhs.dump(os);
    తిరిగి os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{బహిరంగ : మాయా bool కణుపు_సరిపోలిక(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి {తిరిగి true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#నిర్వచించు ASTNODE_PREFETCH_NON_NULL(nodep) \
    చేయి { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } అయితే (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#నిర్వచించు ASTNODE_PREFETCH(nodep) \
    చేయి { \
        ఒకవేళ (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } అయితే (false)

class Ast_కణుపు VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_కణుపు* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_కణుపు* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_కణుపు* m_op1p = nullptr;  // Generic pointer 1
    Ast_కణుపు* m_op2p = nullptr;  // Generic pointer 2
    Ast_కణుపు* m_op3p = nullptr;  // Generic pointer 3
    Ast_కణుపు* m_op4p = nullptr;  // Generic pointer 4
    Ast_కణుపు** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    స్థిరరాశి VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    నిర్మాణం {
        bool did_వెడల్పు : 1;  // Did V3Width computation
        bool doing_వెడల్పు : 1;  // Inside V3Width
        bool protect : 1;  // Protect name if protection is on
        // Space for more flags here (there must be 8 bits in total)
        uint8_t వాడని : 5;
    } m_flags;  // Attribute flags

    // State variable used by V3Broken for consistency checking. The top bit of this is byte is a
    // flag, representing V3Broken is currently proceeding under this node. The bottom 7 bits are
    // a generation number. This is hot when --debug-checks so we access as a whole to avoid bit
    // field masking resulting in unnecessary read-modify-write ops.
    uint8_t m_brokenState = 0;

    పూర్ణాంకం m_cloneCnt = 0;  // Sequence number for when last clone was made

#ఒకవేళ defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(పరిమాణం(m_type) + పరిమాణం(m_flags) + పరిమాణం(m_brokenState) + పరిమాణం(m_cloneCnt)
                      <= పరిమాణం(శూన్య*),
                  "packing requires padding");
#అయితే_ముగింపు

    Ast_కణుపు_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_కణుపు* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_పంక్తి* m_fileline;  // Where it was declared
#నిర్వచితమైతే VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_లెక్క;  // When it was last edited
#అయితే_ముగింపు
    స్థిర uint64_t s_editCntGbl;  // Global edit counter
    స్థిర uint64_t s_edit_Cnt_చివరి;  // Last committed value of global edit counter

    Ast_కణుపు* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    స్థిర పూర్ణాంకం s_cloneCntGbl;  // Count of which userp is set

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
    శూన్య op1p(Ast_కణుపు* nodep) {
        m_op1p = nodep;
        ఒకవేళ (nodep) nodep->m_backp = this;
    }
    శూన్య op2p(Ast_కణుపు* nodep) {
        m_op2p = nodep;
        ఒకవేళ (nodep) nodep->m_backp = this;
    }
    శూన్య op3p(Ast_కణుపు* nodep) {
        m_op3p = nodep;
        ఒకవేళ (nodep) nodep->m_backp = this;
    }
    శూన్య op4p(Ast_కణుపు* nodep) {
        m_op4p = nodep;
        ఒకవేళ (nodep) nodep->m_backp = this;
    }

అంతరంగ:
    Ast_కణుపు* clone_వృక్షం_Iter(bool need_శుద్ధ);
    Ast_కణుపు* clone_వృక్షం_Iter_List(bool need_శుద్ధ);
    శూన్య check_వృక్షం_Iter(స్థిరరాశి Ast_కణుపు* prevBackp) స్థిరరాశి VL_MT_STABLE;
    bool gate_వృక్షం_Iter() స్థిరరాశి;
    స్థిర bool same_వృక్షం_Iter(స్థిరరాశి Ast_కణుపు* node1p, స్థిరరాశి Ast_కణుపు* node2p, bool ign_తదుపరి,
                             bool gateOnly);
    శూన్య delete_వృక్షం_Iter();
    శూన్య delete_కణుపు();
    string instanceStr() స్థిరరాశి;

బహిరంగ:
    శూన్య purityCheck();
    స్థిర శూన్య relinkOneLink(Ast_కణుపు*& pointpr, Ast_కణుపు* newp);
    // cppcheck-suppress functionConst
    స్థిర శూన్య debug_వృక్షం_Change(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి అక్షరం* ఉపసర్గ, పూర్ణాంకం lineno, bool తదుపరి);

రక్షిత:
    // CONSTRUCTORS
    Ast_కణుపు(VNType t, File_పంక్తి* fl);
    మాయా ~Ast_కణుపు() = అప్రమేయ;  // Use 'deleteTree' instead
    మాయా Ast_కణుపు* clone() = 0;  // Generally, cloneTree is what you want instead
    మాయా శూన్య cloneRelink() { cloneRelinkGen(); }
    మాయా శూన్య cloneRelinkGen() {};  // Overrides generated by 'astgen'
    శూన్య clone_Relink_వృక్షం();

    // METHODS
    శూన్య సమితి_Op1p(Ast_కణుపు* newp);  // Set non-list-type op1 to non-list element
    శూన్య సమితి_Op2p(Ast_కణుపు* newp);  // Set non-list-type op2 to non-list element
    శూన్య సమితి_Op3p(Ast_కణుపు* newp);  // Set non-list-type op3 to non-list element
    శూన్య సమితి_Op4p(Ast_కణుపు* newp);  // Set non-list-type op4 to non-list element

    శూన్య కూడు_Op1p(Ast_కణుపు* newp);  // Append newp to end of op1
    శూన్య కూడు_Op2p(Ast_కణుపు* newp);  // Append newp to end of op2
    శూన్య కూడు_Op3p(Ast_కణుపు* newp);  // Append newp to end of op3
    శూన్య కూడు_Op4p(Ast_కణుపు* newp);  // Append newp to end of op4

    శూన్య prependOp1p(Ast_కణుపు* newp);  // Prepend newp to front of op1
    శూన్య prependOp2p(Ast_కణుపు* newp);  // Prepend newp to front of op2
    శూన్య prependOp3p(Ast_కణుపు* newp);  // Prepend newp to front of op3
    శూన్య prependOp4p(Ast_కణుపు* newp);  // Prepend newp to front of op4

    // clang-format off
    శూన్య సమితి_NOp1p(Ast_కణుపు* newp) { ఒకవేళ (newp) సమితి_Op1p(newp); }
    శూన్య సమితి_NOp2p(Ast_కణుపు* newp) { ఒకవేళ (newp) సమితి_Op2p(newp); }
    శూన్య సమితి_NOp3p(Ast_కణుపు* newp) { ఒకవేళ (newp) సమితి_Op3p(newp); }
    శూన్య సమితి_NOp4p(Ast_కణుపు* newp) { ఒకవేళ (newp) సమితి_Op4p(newp); }

    శూన్య కూడు_NOp1p(Ast_కణుపు* newp) { ఒకవేళ (newp) కూడు_Op1p(newp); }
    శూన్య కూడు_NOp2p(Ast_కణుపు* newp) { ఒకవేళ (newp) కూడు_Op2p(newp); }
    శూన్య కూడు_NOp3p(Ast_కణుపు* newp) { ఒకవేళ (newp) కూడు_Op3p(newp); }
    శూన్య కూడు_NOp4p(Ast_కణుపు* newp) { ఒకవేళ (newp) కూడు_Op4p(newp); }

    శూన్య prependNOp1p(Ast_కణుపు* newp) { ఒకవేళ (newp) prependOp1p(newp); }
    శూన్య prependNOp2p(Ast_కణుపు* newp) { ఒకవేళ (newp) prependOp2p(newp); }
    శూన్య prependNOp3p(Ast_కణుపు* newp) { ఒకవేళ (newp) prependOp3p(newp); }
    శూన్య prependNOp4p(Ast_కణుపు* newp) { ఒకవేళ (newp) prependOp4p(newp); }
    // clang-format on

    శూన్య clonep(Ast_కణుపు* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    స్థిర శూన్య clone_Clear_వృక్షం() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    మాయా bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి { తిరిగి true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    మాయా bool wouldBreakGen(స్థిరరాశి Ast_కణుపు* స్థిరరాశి oldp,
                               స్థిరరాశి Ast_కణుపు* స్థిరరాశి newp) స్థిరరాశి
        = 0;  // Generated by 'astgen'

బహిరంగ:
    // ACCESSORS
    VNType type() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_type; }
    స్థిరరాశి అక్షరం* typeName() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి type().ascii(); }  // See also prettyTypeName
    Ast_కణుపు* nextp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_nextp; }
    Ast_కణుపు* backp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_backp; }
    Ast_కణుపు* abovep() స్థిరరాశి;  // Get parent node above, only for list head and tail
    Ast_కణుపు* aboveLoopp() స్థిరరాశి;  // Get parent node above, may have performance issues as loops
    Ast_కణుపు* lastp() స్థిరరాశి {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        తిరిగి m_headtailp;
    }
    Ast_కణుపు* prevp() స్థిరరాశి {  // Previous node in list, nullptr for head of list
        ఒకవేళ (m_backp->m_nextp != this) తిరిగి nullptr;
        తిరిగి m_backp;
    }
    Ast_కణుపు* op1p() స్థిరరాశి VL_MT_STABLE { తిరిగి m_op1p; }
    Ast_కణుపు* op2p() స్థిరరాశి VL_MT_STABLE { తిరిగి m_op2p; }
    Ast_కణుపు* op3p() స్థిరరాశి VL_MT_STABLE { తిరిగి m_op3p; }
    Ast_కణుపు* op4p() స్థిరరాశి VL_MT_STABLE { తిరిగి m_op4p; }
    Ast_కణుపు_DType* dtypep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_dtypep; }
    Ast_కణుపు* clonep() స్థిరరాశి { తిరిగి ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_కణుపు* మొదటి_Abovep() స్థిరరాశి {  // Returns nullptr when second or later in list
        తిరిగి ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_brokenState; }
    శూన్య brokenState(uint8_t విలువ) { m_brokenState = విలువ; }

    // Used by AstNode::broken()
    bool brokeExists() స్థిరరాశి { తిరిగి V3Broken::isLinkable(this); }
    bool brokeExistsAbove() స్థిరరాశి { తిరిగి brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    మాయా శూన్య foreachLink(std::function<శూన్య(Ast_కణుపు** linkpp, స్థిరరాశి అక్షరం* namep)> f) = 0;

#ఒకవేళ defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    స్థిర శూన్య* క్రియాకారకం new(కొలత_t కొలత);
    స్థిర శూన్య క్రియాకారకం delete(శూన్య* obj, కొలత_t కొలత);
#అయితే_ముగింపు

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_BRANCH = 4;  // Branch
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_CALL = INSTR_లెక్క_BRANCH + 10;  // Subroutine call
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_LD = 2;  // Load memory
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_INT_MUL = 3;  // Integer multiply
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_INT_DIV = 10;  // Integer divide
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_DBL = 8;  // Convert or do float ops
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_DBL_DIV = 40;  // Double divide
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_DBL_TRIG = 200;  // Double trigonometric ops
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_STR = 100;  // String ops
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_సమయం = INSTR_లెక్క_CALL + 5;  // Determine simulation time
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_PLI = 20;  // PLI routines
    స్థిర constexpr పూర్ణాంకం INSTR_లెక్క_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    మాయా string name() స్థిరరాశి VL_MT_STABLE { తిరిగి ""; }
    మాయా string origName() స్థిరరాశి { తిరిగి ""; }
    string prettyDehashOrigOrName() స్థిరరాశి {
        తిరిగి prettyName(VName::dehash(origName().ఖాళీలు() ? name() : origName()));
    }
    మాయా శూన్య name(స్థిరరాశి string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    మాయా శూన్య ట్యాగ్(స్థిరరాశి string& పాఠ్యం) {}
    మాయా string ట్యాగ్() స్థిరరాశి { తిరిగి ""; }
    మాయా uint32_t decl_Token_అంకె() స్థిరరాశి { తిరిగి 0; }
    మాయా శూన్య decl_Token_అంకె_సమితి_Min(uint32_t token_అంకె) {}
    మాయా string verilogKwd() స్థిరరాశి { తిరిగి ""; }
    string nameProtect() స్థిరరాశి VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() స్థిరరాశి;  // origName with --protect-id applied
    string shortName() స్థిరరాశి;  // Name with __PVT__ removed for concatenating scopes
    స్థిర string dedotName(స్థిరరాశి string& namein);  // Name with dots removed
    స్థిర string nameNoArray(స్థిరరాశి string& namein);  // Name with any array index removed
    స్థిర string prettyName(స్థిరరాశి string& namein) VL_శుద్ధ;  // Name for printing out to the user
    స్థిర string vpiName(స్థిరరాశి string& namein);  // Name for vpi access
    స్థిర string prettyNameQ(స్థిరరాశి string& namein) {  // Quoted pretty name (for errors)
        తిరిగి "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    స్థిర string encodeName(స్థిరరాశి string& namein);
    స్థిర string encode_సంఖ్య(int64_t అంకె);  // Encode number into internal C representation
    స్థిర string vcdName(స్థిరరాశి string& namein);  // Name for printing out to vcd files
    string prettyName() స్థిరరాశి { తిరిగి prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    మాయా string pretty_Name_సందేశం() స్థిరరాశి { తిరిగి prettyName(); }
    string prettyNameQ() స్థిరరాశి { తిరిగి "'"s + pretty_Name_సందేశం() + "'"; }  // Quoted, for messages
    string verilogName() స్థిరరాశి { తిరిగి vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() స్థిరరాశి;
    మాయా string prettyOperatorName() స్థిరరాశి { తిరిగి "operator " + prettyTypeName(); }
    File_పంక్తి* fileline() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_fileline; }
    శూన్య fileline(File_పంక్తి* fl) { m_fileline = fl; }
    అంతఃవరుస bool width1() స్థిరరాశి;
    అంతఃవరుస పూర్ణాంకం వెడల్పు_Instrs() స్థిరరాశి;
    bool did_వెడల్పు() స్థిరరాశి { తిరిగి m_flags.did_వెడల్పు; }
    శూన్య did_వెడల్పు(bool flag) { m_flags.did_వెడల్పు = flag; }
    bool did_వెడల్పు_And_సమితి() {
        ఒకవేళ (did_వెడల్పు()) తిరిగి true;
        did_వెడల్పు(true);
        తిరిగి false;
    }
    bool doing_వెడల్పు() స్థిరరాశి { తిరిగి m_flags.doing_వెడల్పు; }
    శూన్య doing_వెడల్పు(bool flag) { m_flags.doing_వెడల్పు = flag; }
    bool protect() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_flags.protect; }
    శూన్య protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    అంతఃవరుస పూర్ణాంకం వెడల్పు() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస పూర్ణాంకం వెడల్పు_Min() స్థిరరాశి VL_MT_STABLE;
    పూర్ణాంకం వెడల్పు_Min_V() స్థిరరాశి {
        తిరిగి v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::VERILOG_వెడల్పు ? వెడల్పు_Min() : వెడల్పు();
    }
    పూర్ణాంకం వెడల్పు_పదాలు() స్థిరరాశి { తిరిగి VL_పదాలు_I(వెడల్పు()); }
    bool isQuad() స్థిరరాశి VL_MT_STABLE { తిరిగి (వెడల్పు() > VL_IDATASIZE && వెడల్పు() <= VL_QUADSIZE); }
    bool isWide() స్థిరరాశి VL_MT_STABLE { తిరిగి (వెడల్పు() > VL_QUADSIZE); }
    అంతఃవరుస bool isCHandle() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస bool isDouble() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస bool isSigned() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస bool isString() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస bool isEvent() స్థిరరాశి VL_MT_STABLE;

    // clang-format off
    VNUser user1u() స్థిరరాశి VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        తిరిగి ((m_user1Cnt == VNUser1_లోపల_Use::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_కణుపు* user1p() స్థిరరాశి VL_MT_STABLE { తిరిగి user1u().గమ్యం_Nodep(); }
    శూన్య user1u(స్థిరరాశి VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1_లోపల_Use::s_userCntGbl; }
    శూన్య user1p(శూన్య* userp) { user1u(VNUser{userp}); }
    శూన్య user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() స్థిరరాశి { తిరిగి user1u().గమ్యం_UQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { స్థిరరాశి uint64_t v = user1(); user1(v + val); తిరిగి v; }
    uint64_t user1Or(uint64_t val) { స్థిరరాశి uint64_t v = user1(); user1(v | val); తిరిగి v; }
    uint64_t user1_సమితి_Once() { స్థిరరాశి uint64_t v = user1(); ఒకవేళ (!v) user1(1); తిరిగి v; }  // Better for cache than user1Inc()
    స్థిర శూన్య user1_Clear_వృక్షం() { VNUser1_లోపల_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() స్థిరరాశి VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        తిరిగి ((m_user2Cnt == VNUser2_లోపల_Use::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_కణుపు* user2p() స్థిరరాశి VL_MT_STABLE { తిరిగి user2u().గమ్యం_Nodep(); }
    శూన్య user2u(స్థిరరాశి VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2_లోపల_Use::s_userCntGbl; }
    శూన్య user2p(శూన్య* userp) { user2u(VNUser{userp}); }
    శూన్య user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() స్థిరరాశి { తిరిగి user2u().గమ్యం_UQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { స్థిరరాశి uint64_t v = user2(); user2(v + val); తిరిగి v; }
    uint64_t user2Or(uint64_t val) { స్థిరరాశి uint64_t v = user2(); user2(v | val); తిరిగి v; }
    uint64_t user2_సమితి_Once() { స్థిరరాశి uint64_t v = user2(); ఒకవేళ (!v) user2(1); తిరిగి v; }  // Better for cache than user2Inc()
    స్థిర శూన్య user2_Clear_వృక్షం() { VNUser2_లోపల_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() స్థిరరాశి VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        తిరిగి ((m_user3Cnt == VNUser3_లోపల_Use::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_కణుపు* user3p() స్థిరరాశి VL_MT_STABLE { తిరిగి user3u().గమ్యం_Nodep(); }
    శూన్య user3u(స్థిరరాశి VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3_లోపల_Use::s_userCntGbl; }
    శూన్య user3p(శూన్య* userp) { user3u(VNUser{userp}); }
    శూన్య user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() స్థిరరాశి { తిరిగి user3u().గమ్యం_UQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { స్థిరరాశి uint64_t v = user3(); user3(v + val); తిరిగి v; }
    uint64_t user3Or(uint64_t val) { స్థిరరాశి uint64_t v = user3(); user3(v | val); తిరిగి v; }
    uint64_t user3_సమితి_Once() { స్థిరరాశి uint64_t v = user3(); ఒకవేళ (!v) user3(1); తిరిగి v; }  // Better for cache than user3Inc()
    స్థిర శూన్య user3_Clear_వృక్షం() { VNUser3_లోపల_Use::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() స్థిరరాశి VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        తిరిగి ((m_user4Cnt == VNUser4_లోపల_Use::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_కణుపు* user4p() స్థిరరాశి VL_MT_STABLE { తిరిగి user4u().గమ్యం_Nodep(); }
    శూన్య user4u(స్థిరరాశి VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4_లోపల_Use::s_userCntGbl; }
    శూన్య user4p(శూన్య* userp) { user4u(VNUser{userp}); }
    శూన్య user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() స్థిరరాశి { తిరిగి user4u().గమ్యం_UQuad(); }
    uint64_t user4Or(uint64_t val) { స్థిరరాశి uint64_t v = user4(); user4(v | val); తిరిగి v; }
    uint64_t user4Inc(uint64_t val = 1) { స్థిరరాశి uint64_t v = user4(); user4(v + val); తిరిగి v; }
    uint64_t user4_సమితి_Once() { స్థిరరాశి uint64_t v = user4(); ఒకవేళ (!v) user4(1); తిరిగి v; }  // Better for cache than user4Inc()
    స్థిర శూన్య user4_Clear_వృక్షం() { VNUser4_లోపల_Use::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#నిర్వచితమైతే VL_DEBUG
    uint64_t edit_లెక్క() స్థిరరాశి { తిరిగి m_edit_లెక్క; }
    శూన్య edit_లెక్క_Inc() {
        m_edit_లెక్క = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_వృక్షం();  // Any edit clears all caching
    }
#లేకపోతే
    శూన్య edit_లెక్క_Inc() { ++s_editCntGbl; }
#అయితే_ముగింపు
    స్థిర uint64_t edit_లెక్క_చివరి() VL_MT_సురక్షిత { తిరిగి s_edit_Cnt_చివరి; }
    స్థిర uint64_t edit_లెక్క_Gbl() VL_MT_సురక్షిత { తిరిగి s_editCntGbl; }
    స్థిర శూన్య edit_లెక్క_సమితి_చివరి() { s_edit_Cnt_చివరి = edit_లెక్క_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_వరుస_నెట్టు_Self_Stmt();
    అంతఃవరుస bool is_Class_Handle_విలువ() స్థిరరాశి;
    అంతఃవరుస bool isNull() స్థిరరాశి;
    అంతఃవరుస bool isZero() స్థిరరాశి;
    అంతఃవరుస bool isOne() స్థిరరాశి;
    అంతఃవరుస bool isNeqZero() స్థిరరాశి;
    అంతఃవరుస bool isAllOnes() స్థిరరాశి;
    అంతఃవరుస bool isAllOnesV() స్థిరరాశి;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    శూన్య dtypep(Ast_కణుపు_DType* nodep) {
        ఒకవేళ (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_లెక్క_Inc();
        }
    }
    శూన్య dtypeFrom(స్థిరరాశి Ast_కణుపు* fromp) {
        ఒకవేళ (fromp) dtypep(fromp->dtypep());
    }
    శూన్య dtypeChgSigned(bool flag = true);
    శూన్య dtype_Chg_వెడల్పు(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min);
    శూన్య dtype_Chg_వెడల్పు_Signed(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric);
    శూన్య dtype_సమితి_బిట్టు_Unsized(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) {
        dtypep(కనుగొను_బిట్టు_DType(వెడల్పు, వెడల్పు_Min, numeric));
    }
    శూన్య dtype_సమితి_బిట్టు_Sized(పూర్ణాంకం వెడల్పు, VSigning numeric) {
        dtypep(కనుగొను_బిట్టు_DType(వెడల్పు, వెడల్పు, numeric));  // Since sized, widthMin is width
    }
    శూన్య dtype_సమితి_Logic_Unsized(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) {
        dtypep(కనుగొను_Logic_DType(వెడల్పు, వెడల్పు_Min, numeric));
    }
    శూన్య dtype_సమితి_Logic_Sized(పూర్ణాంకం వెడల్పు, VSigning numeric) {
        dtypep(కనుగొను_Logic_DType(వెడల్పు, వెడల్పు, numeric));  // Since sized, widthMin is width
    }
    శూన్య dtype_సమితి_Logic2_State_Unsized(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) {
        dtypep(కనుగొను_Logic2_State_DType(వెడల్పు, వెడల్పు_Min, numeric));
    }
    శూన్య dtype_సమితి_Logic2_State_Sized(పూర్ణాంకం వెడల్పు, VSigning numeric) {
        dtypep(కనుగొను_Logic2_State_DType(వెడల్పు, వెడల్పు, numeric));  // Since sized, widthMin is width
    }
    శూన్య dtype_సమితి_బిట్టు_Or_Logic_Unsized(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric, bool isFourstate) {
        dtypep(కనుగొను_బిట్టు_Or_Logic_DType(వెడల్పు, వెడల్పు_Min, numeric, isFourstate));
    }
    శూన్య dtype_సమితి_బిట్టు_Or_Logic_Sized(పూర్ణాంకం వెడల్పు, VSigning numeric, bool isFourstate) {
        dtypep(కనుగొను_బిట్టు_Or_Logic_DType(వెడల్పు, వెడల్పు, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    శూన్య dtype_సమితి_బిట్టు() { dtypep(కనుగొను_బిట్టు_DType()); }
    శూన్య dtype_సమితి_Logic() { dtypep(కనుగొను_Logic_DType()); }
    శూన్య dtype_సమితి_Logic2_State() { dtypep(కనుగొను_Logic2_State_DType()); }
    శూన్య dtype_సమితి_Double() { dtypep(కనుగొను_Double_DType()); }
    శూన్య dtype_సమితి_Int() { dtypep(కనుగొను_Int_DType()); }
    శూన్య dtype_సమితి_Integer() { dtypep(కనుగొను_Integer_DType()); }
    శూన్య dtype_సమితి_Integer2_State() { dtypep(కనుగొను_Integer2_State_DType()); }
    శూన్య dtype_సమితి_String() { dtypep(కనుగొను_String_DType()); }
    శూన్య dtype_సమితి_UInt32() { dtypep(కనుగొను_UInt32_DType()); }  // Twostate
    శూన్య dtype_సమితి_UInt64() { dtypep(కనుగొను_UInt64_DType()); }  // Twostate
    శూన్య dtype_సమితి_సమయం() { dtypep(కనుగొను_సమయం_DType()); }
    శూన్య dtype_సమితి_Time2_State() { dtypep(కనుగొను_Time2_State_DType()); }
    శూన్య dtype_సమితి_ఖాళీలు_వరుస() { dtypep(కనుగొను_ఖాళీలు_వరుస_DType()); }
    శూన్య dtype_సమితి_ప్రవాహం() { dtypep(కనుగొను_ప్రవాహం_DType()); }
    శూన్య dtype_సమితి_Void() { dtypep(కనుగొను_Void_DType()); }
    శూన్య dtype_సమితి_బిట్టు_Or_Logic(bool isFourstate) {
        dtypep(isFourstate ? కనుగొను_Logic_DType() : కనుగొను_బిట్టు_DType());
    }

    // Data type locators
    Ast_కణుపు_DType* కనుగొను_బిట్టు_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::బిట్టు); }
    Ast_కణుపు_DType* కనుగొను_Logic_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::LOGIC); }
    Ast_కణుపు_DType* కనుగొను_Null_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::NULLTYPE); }
    Ast_కణుపు_DType* కనుగొను_Logic2_State_DType() స్థిరరాశి {
        తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_కణుపు_DType* కనుగొను_Double_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::DOUBLE); }
    Ast_కణుపు_DType* కనుగొను_Int_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::INT); }
    Ast_కణుపు_DType* కనుగొను_Integer_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::INTEGER); }
    Ast_కణుపు_DType* కనుగొను_Integer2_State_DType() స్థిరరాశి {
        తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_కణుపు_DType* కనుగొను_String_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::STRING); }
    Ast_కణుపు_DType* కనుగొను_Signed8_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::BYTE); }
    Ast_కణుపు_DType* కనుగొను_UInt32_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::UINT32); }
    Ast_కణుపు_DType* కనుగొను_UInt64_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::UINT64); }
    Ast_కణుపు_DType* కనుగొను_సమయం_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::సమయం); }
    Ast_కణుపు_DType* కనుగొను_Time2_State_DType() స్థిరరాశి {
        తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_కణుపు_DType* కనుగొను_CHandle_DType() స్థిరరాశి { తిరిగి కనుగొను_Basic_DType(VBasicDTypeKwd::CHANDLE); }
    Ast_కణుపు_DType* కనుగొను_Constraint_Ref_DType() స్థిరరాశి;
    Ast_కణుపు_DType* కనుగొను_ఖాళీలు_వరుస_DType() స్థిరరాశి;
    Ast_కణుపు_DType* కనుగొను_వరుస_సూచీ_DType() స్థిరరాశి;
    Ast_కణుపు_DType* కనుగొను_ప్రవాహం_DType() స్థిరరాశి;
    Ast_కణుపు_DType* కనుగొను_Void_DType() స్థిరరాశి;
    Ast_కణుపు_DType* కనుగొను_బిట్టు_DType(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) స్థిరరాశి;
    Ast_కణుపు_DType* కనుగొను_Logic_DType(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) స్థిరరాశి;
    Ast_కణుపు_DType* కనుగొను_Logic2_State_DType(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) స్థిరరాశి;
    Ast_కణుపు_DType* కనుగొను_బిట్టు_Or_Logic_DType(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric,
                                      bool isFourstate) స్థిరరాశి {
        తిరిగి isFourstate ? కనుగొను_Logic_DType(వెడల్పు, వెడల్పు_Min, numeric)
                           : కనుగొను_బిట్టు_DType(వెడల్పు, వెడల్పు_Min, numeric);
    }
    Ast_కణుపు_DType* కనుగొను_బిట్టు_Or_Logic_Range_DType(స్థిరరాశి VNumRange& range, స్థిరరాశి పూర్ణాంకం వెడల్పు_Min,
                                           స్థిరరాశి VSigning numeric,
                                           స్థిరరాశి bool isFourstate) స్థిరరాశి VL_MT_STABLE {
        తిరిగి isFourstate ? కనుగొను_Logic_Range_DType(range, వెడల్పు_Min, numeric)
                           : కనుగొను_బిట్టు_Range_DType(range, వెడల్పు_Min, numeric);
    }
    Ast_కణుపు_DType* కనుగొను_Logic_Range_DType(స్థిరరాశి VNumRange& range, పూర్ణాంకం వెడల్పు_Min,
                                      VSigning numeric) స్థిరరాశి VL_MT_STABLE;
    Ast_కణుపు_DType* కనుగొను_బిట్టు_Range_DType(స్థిరరాశి VNumRange& range, పూర్ణాంకం వెడల్పు_Min,
                                    VSigning numeric) స్థిరరాశి VL_MT_STABLE;
    Ast_కణుపు_DType* కనుగొను_Basic_DType(VBasicDTypeKwd kwd) స్థిరరాశి;
    స్థిర AstBasicDType* కనుగొను_చొప్పించు_Same_DType(AstBasicDType* nodep);

    స్థిర VCastable computeCastable(స్థిరరాశి Ast_కణుపు_DType* గమ్యం_Dtp, స్థిరరాశి Ast_కణుపు_DType* fromDtp,
                                     స్థిరరాశి Ast_కణుపు* fromConstp, bool checkIfaceArgCompat = false);
    స్థిర Ast_కణుపు_DType* getCommonClassTypep(Ast_కణుపు* nodep1, Ast_కణుపు* nodep2);

    // METHODS - dump and error
    శూన్య v3error_చివర(స్థిరరాశి std::ostringstream& str) స్థిరరాశి VL_RELEASE(V3Error::s().m_mutex);
    శూన్య v3error_చివర_Fatal(స్థిరరాశి std::ostringstream& str) స్థిరరాశి VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex) {
        తిరిగి fileline()->warnContextPrimary();
    }
    string warnContextSecondary() స్థిరరాశి { తిరిగి fileline()->warnContextSecondary(); }
    string warn_మరిన్ని() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex) { తిరిగి fileline()->warn_మరిన్ని(); }
    string warnOther() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex) { తిరిగి fileline()->warnOther(); }

    మాయా శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి;
    స్థిర అక్షరం* dump_వృక్షం_Json_Gdb(స్థిరరాశి Ast_కణుపు* nodep);  // For GDB only, free()d by caller
    స్థిర అక్షరం* dump_వృక్షం_Json_Gdb(స్థిరరాశి అక్షరం* str);  // For GDB only, free()d by caller
    స్థిర అక్షరం* dump_వృక్షం_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    స్థిర శూన్య dumpGdb(స్థిరరాశి Ast_కణుపు* nodep);  // For GDB only
    శూన్య dump_Gdb_శీర్షిక() స్థిరరాశి;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    మూస <typename T_కణుపు_ఫలితం, typename T_కణుపు_తదుపరి>
    స్థిర T_కణుపు_ఫలితం* కూడు_తదుపరి(T_కణుపు_ఫలితం* nodep, T_కణుపు_తదుపరి* newp) {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు_ఫలితం>::విలువ,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_పునాది_యొక్క<T_కణుపు_ఫలితం, T_కణుపు_తదుపరి>::విలువ,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        ఒకవేళ (!newp) తిరిగి nodep;
        తిరిగి static_cast<T_కణుపు_ఫలితం*>(కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(nodep, newp));
    }
    మూస <typename T_కణుపు_ఫలితం, typename T_కణుపు_తదుపరి>
    స్థిర T_కణుపు_ఫలితం* కూడు_తదుపరి_Null(T_కణుపు_ఫలితం* nodep, T_కణుపు_తదుపరి* newp) {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు_ఫలితం>::విలువ,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_పునాది_యొక్క<T_కణుపు_ఫలితం, T_కణుపు_తదుపరి>::విలువ,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        ఒకవేళ (!newp) తిరిగి nodep;
        ఒకవేళ (!nodep) తిరిగి newp;
        తిరిగి static_cast<T_కణుపు_ఫలితం*>(కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(nodep, newp));
    }
    అంతఃవరుస Ast_కణుపు* కూడు_తదుపరి(Ast_కణుపు* newp);
    శూన్య కూడు_తదుపరి_ఇక్కడ(Ast_కణుపు* newp);  // Insert newp at this->nextp
    శూన్య కూడు_ఇక్కడ_This_As_తదుపరి(Ast_కణుపు* newp);  // Adds at old place of this, this becomes next
    శూన్య replaceWith(Ast_కణుపు* newp);  // Replace current node in tree with new node
    శూన్య replaceWithKeepDType(Ast_కణుపు* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_కణుపు* unlink_Fr_వెనుక(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_కణుపు* unlink_Fr_వెనుక_With_తదుపరి(VNRelinker* linkerp = nullptr);
    శూన్య relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    శూన్య clone_Relink_కణుపు() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_కణుపు* clone_వృక్షం(bool clone_తదుపరి_Link,
                       bool need_శుద్ధ = false);  // Not const, as sets clonep() on original nodep
    Ast_కణుపు* clone_వృక్షం_శుద్ధ(bool clone_తదుపరి_Link) { తిరిగి clone_వృక్షం(clone_తదుపరి_Link, true); }
    bool gate_వృక్షం() { తిరిగి gate_వృక్షం_Iter(); }  // Is tree isGateOptimizable?
    అంతఃవరుస bool same_వృక్షం(స్థిరరాశి Ast_కణుపు* node2p) స్థిరరాశి;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    అంతఃవరుస bool same_Gate_వృక్షం(స్థిరరాశి Ast_కణుపు* node2p) స్థిరరాశి;
    శూన్య delete_వృక్షం();  // Always deletes the next link
    శూన్య check_వృక్షం() స్థిరరాశి VL_MT_STABLE {
        ఒకవేళ (v3Global.opt.debugCheck()) check_వృక్షం_Iter(backp());
    }
    శూన్య checkIter() స్థిరరాశి;
    శూన్య dumpPtrs(std::ostream& os = std::cout) స్థిరరాశి;
    శూన్య dump_వృక్షం(std::ostream& os = std::cout, స్థిరరాశి string& indent = "    ",
                  పూర్ణాంకం max_లోతు = 0) స్థిరరాశి;
    శూన్య dump_వృక్షం(స్థిరరాశి string& indent, పూర్ణాంకం max_లోతు = 0) స్థిరరాశి {
        dump_వృక్షం(std::cout, indent, max_లోతు);
    }
    స్థిర శూన్య dump_వృక్షం_Gdb(స్థిరరాశి Ast_కణుపు* nodep);  // For GDB only
    శూన్య dump_వృక్షం_And_తదుపరి(std::ostream& os = std::cout, స్థిరరాశి string& indent = "    ",
                         పూర్ణాంకం max_లోతు = 0) స్థిరరాశి;
    శూన్య dump_వృక్షం_File(స్థిరరాశి string& filename, bool doDump = true);
    స్థిర శూన్య dump_వృక్షం_File_Gdb(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి అక్షరం* filenamep = nullptr);
    శూన్య dump_వృక్షం_Dot(std::ostream& os = std::cout) స్థిరరాశి;
    శూన్య dump_వృక్షం_Dot_File(స్థిరరాశి string& filename, bool doDump = true);
    మాయా శూన్య dumpJson(std::ostream& os) స్థిరరాశి { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    శూన్య dumpJsonGen(std::ostream& os) స్థిరరాశి {};
    మాయా శూన్య dump_వృక్షం_Json_Op_Gen(std::ostream& os, స్థిరరాశి string& indent) స్థిరరాశి {};
    శూన్య dump_వృక్షం_Json(std::ostream& os, స్థిరరాశి string& indent = "") స్థిరరాశి;
    శూన్య dump_వృక్షం_Json_File(స్థిరరాశి string& filename, bool doDump = true);
    స్థిర శూన్య dumpJsonMetaFileGdb(స్థిరరాశి అక్షరం* filename);
    స్థిర శూన్య dumpJsonMetaFile(స్థిరరాశి string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    స్థిర std::string కణుపు_Addr(స్థిరరాశి Ast_కణుపు* nodep) {
        తిరిగి v3Global.opt.dump_వృక్షం_Addrids() ? v3Global.ptr_గమ్యం_Id(nodep) : cvt_గమ్యం_Hex(nodep);
    }

    // METHODS - static advancement
    స్థిర Ast_కణుపు* afterCommentp(Ast_కణుపు* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        అయితే (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        తిరిగి nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    మాయా bool isBrancher() స్థిరరాశి { తిరిగి false; }
    // Else a AstTime etc that can't be pushed out
    మాయా bool isGateOptimizable() స్థిరరాశి { తిరిగి !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    మాయా bool isGateDedupable() స్థిరరాశి { తిరిగి isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    మాయా bool is_సమాసం_Coverage_Eligible() స్థిరరాశి { తిరిగి isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    మాయా bool isOutputter() { తిరిగి false; }
    // Else a AstTime etc which output can't be predicted from input
    మాయా bool isPredictOptimizable() స్థిరరాశి { తిరిగి !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    మాయా bool is_శుద్ధ() { తిరిగి true; }
    // Iff isPure on current node and any nextp()
    bool is_శుద్ధ_And_తదుపరి() { తిరిగి is_శుద్ధ() && (!nextp() || nextp()->is_శుద్ధ()); }
    // An event control, delay, wait, etc.
    మాయా bool isTimingControl() స్థిరరాశి { తిరిగి false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    మాయా bool isUnlikely() స్థిరరాశి { తిరిగి false; }
    // Is an IEEE system function (versus internally-generated)
    మాయా bool isSystemFunc() స్థిరరాశి { తిరిగి false; }
    మాయా పూర్ణాంకం instr_లెక్క() స్థిరరాశి { తిరిగి 0; }
    // Iff node is identical to another node
    మాయా bool isSame(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి {
        తిరిగి type() == samep->type() && same_కణుపు(samep);
    }
    // Iff has a data type; dtype() must be non-null
    మాయా bool hasDType() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి false; }
    // Iff has a non-null childDTypep(), as generic node function
    మాయా Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి { తిరిగి nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    మాయా Ast_కణుపు_DType* getChild2DTypep() స్థిరరాశి { తిరిగి nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    మాయా bool maybe_Pointed_గమ్యం() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి false; }
    // Don't reclaim this node in V3Dead
    మాయా bool undead() స్థిరరాశి { తిరిగి false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    మాయా స్థిరరాశి అక్షరం* broken() స్థిరరాశి { తిరిగి nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    మాయా స్థిరరాశి అక్షరం* brokenGen() స్థిరరాశి = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(స్థిరరాశి Ast_కణుపు* స్థిరరాశి newp) స్థిరరాశి { తిరిగి backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    మాయా శూన్య accept(VNVisitorConst& v) = 0;

రక్షిత:
    // All VNVisitor related functions are called as methods off the visitor
    మిత్ర class VNVisitor;
    మిత్ర class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    శూన్య iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    శూన్య iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    శూన్య iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    శూన్య iterate_And_తదుపరి(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    శూన్య iterate_And_తదుపరి_Const(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_కణుపు* iterateSubtreeReturnEdits(VNVisitor& v);

    స్థిర శూన్య dump_Json_అంకె(std::ostream& os, స్థిరరాశి std::string& name, int64_t val);
    స్థిర శూన్య dumpJsonBoolIf(std::ostream& os, స్థిరరాశి std::string& name, bool val);
    స్థిర శూన్య dumpJsonStr(std::ostream& os, స్థిరరాశి std::string& name, స్థిరరాశి std::string& val);
    స్థిర శూన్య dumpJsonPtr(std::ostream& os, స్థిరరాశి std::string& name, స్థిరరాశి Ast_కణుపు* స్థిరరాశి valp);

రక్షిత:
    శూన్య iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    మూస <typename T>
    అంతఃవరుస స్థిర bool వ్యక్తిగత_Type_Test(స్థిరరాశి Ast_కణుపు* nodep);

    // For internal use only.
    మూస <typename T_లక్ష్యం_Type, typename T_DeclType>
    constexpr స్థిర bool uselessCast() VL_శుద్ధ {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_సూచిక<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        తిరిగి std::is_పునాది_యొక్క<T_లక్ష్యం_Type, NonCV>::విలువ;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    మూస <typename T_లక్ష్యం_Type, typename T_DeclType>
    constexpr స్థిర bool impossibleCast() VL_శుద్ధ {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_సూచిక<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        తిరిగి !std::is_పునాది_యొక్క<NonCV, T_లక్ష్యం_Type>::విలువ;
    }  // LCOV_EXCL_STOP

బహిరంగ:
    // For use via the VN_IS macro only, or in templated code
    మూస <typename T_లక్ష్యం_Type, typename T_కణుపు>
    స్థిర bool is(స్థిరరాశి T_కణుపు* nodep) VL_MT_సురక్షిత {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        తిరిగి nodep && వ్యక్తిగత_Type_Test<T_లక్ష్యం_Type>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    మూస <typename T_లక్ష్యం_Type, typename T_కణుపు>
    స్థిర T_లక్ష్యం_Type* cast(T_కణుపు* nodep) VL_MT_సురక్షిత {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        తిరిగి nodep && వ్యక్తిగత_Type_Test<T_లక్ష్యం_Type>(nodep)
                   ? reinterpret_cast<T_లక్ష్యం_Type*>(nodep)
                   : nullptr;
    }
    మూస <typename T_లక్ష్యం_Type, typename T_కణుపు>
    స్థిర స్థిరరాశి T_లక్ష్యం_Type* cast(స్థిరరాశి T_కణుపు* nodep) VL_MT_సురక్షిత {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        తిరిగి nodep && వ్యక్తిగత_Type_Test<T_లక్ష్యం_Type>(nodep)
                   ? reinterpret_cast<స్థిరరాశి T_లక్ష్యం_Type*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    మూస <typename T_లక్ష్యం_Type, typename T_కణుపు>
    స్థిర T_లక్ష్యం_Type* as(T_కణుపు* nodep) VL_శుద్ధ {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || వ్యక్తిగత_Type_Test<T_లక్ష్యం_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        తిరిగి unsafe_వ్యక్తిగత_As<T_లక్ష్యం_Type, T_కణుపు>(nodep);
    }
    మూస <typename T_లక్ష్యం_Type, typename T_కణుపు>
    స్థిర స్థిరరాశి T_లక్ష్యం_Type* as(స్థిరరాశి T_కణుపు* nodep) VL_శుద్ధ {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || వ్యక్తిగత_Type_Test<T_లక్ష్యం_Type>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        తిరిగి unsafe_వ్యక్తిగత_As<T_లక్ష్యం_Type, T_కణుపు>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    మూస <typename T_లక్ష్యం_Type, typename T_కణుపు>
    స్థిర T_లక్ష్యం_Type* unsafe_వ్యక్తిగత_As(T_కణుపు* nodep) VL_శుద్ధ {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        తిరిగి reinterpret_cast<T_లక్ష్యం_Type*>(nodep);
    }
    మూస <typename T_లక్ష్యం_Type, typename T_కణుపు>
    స్థిర స్థిరరాశి T_లక్ష్యం_Type* unsafe_వ్యక్తిగత_As(స్థిరరాశి T_కణుపు* nodep) VL_శుద్ధ {
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_లక్ష్యం_Type, T_కణుపు>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        తిరిగి reinterpret_cast<స్థిరరాశి T_లక్ష్యం_Type*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    మూస <typename T_కణుపు>
    స్థిర bool mayBeUnder(స్థిరరాశి Ast_కణుపు* nodep) {
        static_assert(!std::is_const<T_కణుపు>::విలువ,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        తిరిగి true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    మూస <typename T_కణుపు>
    స్థిర constexpr bool is_ఆకు() {
        static_assert(!std::is_const<T_కణుపు>::విలువ,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        తిరిగి false;
    }

అంతరంగ:
    // Using std::conditional for const correctness in the public 'foreach' functions
    మూస <typename T_Arg>
    using Const_Correct_Ast_కణుపు =
        typename std::conditional<std::is_const<T_Arg>::విలువ, స్థిరరాశి Ast_కణుపు, Ast_కణుపు>::type;

    మూస <typename T_Arg, typename T_Callable>
    అంతఃవరుస స్థిర శూన్య foreachImpl(Const_Correct_Ast_కణుపు<T_Arg>* nodep, స్థిరరాశి T_Callable& f,
                                   bool సందర్శించు_తదుపరి);

    మూస <typename T_Arg, bool N_Default, typename T_Callable>
    అంతఃవరుస స్థిర bool predicateImpl(Const_Correct_Ast_కణుపు<T_Arg>* nodep, స్థిరరాశి T_Callable& p,
                                     bool సందర్శించు_తదుపరి);

బహిరంగ:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    మూస <typename T_Callable>
    శూన్య foreach(T_Callable&& f) {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_కణుపు*>::విలువ
                          && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_కణుపు>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    మూస <typename T_Callable>
    శూన్య foreach(T_Callable&& f) స్థిరరాశి {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, స్థిరరాశి T_కణుపు*>::విలువ
                && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<స్థిరరాశి T_కణుపు>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    మూస <typename T_Callable>
    శూన్య foreach_And_తదుపరి(T_Callable&& f) {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_కణుపు*>::విలువ
                          && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_కణుపు>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    మూస <typename T_Callable>
    శూన్య foreach_And_తదుపరి(T_Callable&& f) స్థిరరాశి {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, స్థిరరాశి T_కణుపు*>::విలువ
                && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<స్థిరరాశి T_కణుపు>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    మూస <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_కణుపు*>::విలువ
                          && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        తిరిగి predicateImpl<T_కణుపు, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    మూస <typename T_Callable>
    bool exists(T_Callable&& p) స్థిరరాశి {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, స్థిరరాశి T_కణుపు*>::విలువ
                          && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        తిరిగి predicateImpl<స్థిరరాశి T_కణుపు, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    మూస <typename T_Callable>
    bool exists_And_తదుపరి(T_Callable&& p) {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, స్థిరరాశి T_కణుపు*>::విలువ
                          && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        తిరిగి predicateImpl<T_కణుపు, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    మూస <typename T_Callable>
    bool exists_And_తదుపరి(T_Callable&& p) స్థిరరాశి {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, స్థిరరాశి T_కణుపు*>::విలువ
                          && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        తిరిగి predicateImpl<స్థిరరాశి T_కణుపు, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    మూస <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_కణుపు*>::విలువ
                          && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        తిరిగి predicateImpl<T_కణుపు, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    మూస <typename T_Callable>
    bool forall(T_Callable&& p) స్థిరరాశి {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, స్థిరరాశి T_కణుపు*>::విలువ
                          && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        తిరిగి predicateImpl<స్థిరరాశి T_కణుపు, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    పూర్ణాంకం కణుపు_లెక్క() స్థిరరాశి {
        // TODO: this should really return size_t, but need to fix use sites
        పూర్ణాంకం లెక్క = 0;
        this->foreach([&లెక్క](స్థిరరాశి Ast_కణుపు*) { ++లెక్క; });
        తిరిగి లెక్క;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(పూర్ణాంకం హద్దు) స్థిరరాశి {
        పూర్ణాంకం లెక్క = 0;
        తిరిగి this->exists([&లెక్క, హద్దు](స్థిరరాశి Ast_కణుపు*) { తిరిగి ++లెక్క > హద్దు; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
మూస <>
Ast_కణుపు* Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(Ast_కణుపు* nodep, Ast_కణుపు* newp);

// Inline method implementations
Ast_కణుపు* Ast_కణుపు::కూడు_తదుపరి(Ast_కణుపు* newp) { తిరిగి కూడు_తదుపరి(this, newp); }

// Specializations of privateTypeTest
#అన్తర్భావ "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
మూస <>
అంతఃవరుస bool Ast_కణుపు::mayBeUnder<AstCell>(స్థిరరాశి Ast_కణుపు* nodep) {
    తిరిగి !VN_IS(nodep, కణుపు_Stmt) && !VN_IS(nodep, కణుపు_సమాసం);
}
మూస <>
అంతఃవరుస bool Ast_కణుపు::mayBeUnder<Ast_కణుపు_కేటాయించు>(స్థిరరాశి Ast_కణుపు* nodep) {
    తిరిగి !VN_IS(nodep, కణుపు_సమాసం);
}
మూస <>
అంతఃవరుస bool Ast_కణుపు::mayBeUnder<AstVarScope>(స్థిరరాశి Ast_కణుపు* nodep) {
    ఒకవేళ (VN_IS(nodep, VarScope)) తిరిగి false;  // Should not nest
    ఒకవేళ (VN_IS(nodep, Var)) తిరిగి false;
    ఒకవేళ (VN_IS(nodep, చురుకైన)) తిరిగి false;
    ఒకవేళ (VN_IS(nodep, కణుపు_Stmt)) తిరిగి false;
    ఒకవేళ (VN_IS(nodep, కణుపు_సమాసం)) తిరిగి false;
    తిరిగి true;
}
మూస <>
అంతఃవరుస bool Ast_కణుపు::mayBeUnder<Ast_Exec_లేఖాచిత్రం>(స్థిరరాశి Ast_కణుపు* nodep) {
    ఒకవేళ (VN_IS(nodep, Exec_లేఖాచిత్రం)) తిరిగి false;  // Should not nest
    ఒకవేళ (VN_IS(nodep, కణుపు_Stmt)) తిరిగి false;  // Should be directly under CFunc
    తిరిగి true;
}
మూస <>
అంతఃవరుస bool Ast_కణుపు::mayBeUnder<Ast_చురుకైన>(స్థిరరాశి Ast_కణుపు* nodep) {
    తిరిగి !VN_IS(nodep, చురుకైన);  // AstActives do not nest
}
మూస <>
అంతఃవరుస bool Ast_కణుపు::mayBeUnder<AstScope>(స్థిరరాశి Ast_కణుపు* nodep) {
    తిరిగి !VN_IS(nodep, Scope);  // AstScopes do not nest
}
మూస <>
అంతఃవరుస bool Ast_కణుపు::mayBeUnder<Ast_Sen_వృక్షం>(స్థిరరాశి Ast_కణుపు* nodep) {
    తిరిగి !VN_IS(nodep, Sen_వృక్షం);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
మూస <>
constexpr bool Ast_కణుపు::is_ఆకు<Ast_కణుపు_Var_Ref>() {
    తిరిగి true;
}
మూస <>
constexpr bool Ast_కణుపు::is_ఆకు<AstVarRef>() {
    తిరిగి true;
}
మూస <>
constexpr bool Ast_కణుపు::is_ఆకు<AstVarXRef>() {
    తిరిగి true;
}

// foreach implementation
మూస <typename T_Arg, typename T_Callable>
శూన్య Ast_కణుపు::foreachImpl(Const_Correct_Ast_కణుపు<T_Arg>* nodep, స్థిరరాశి T_Callable& f, bool సందర్శించు_తదుపరి) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using కణుపు_2 = Const_Correct_Ast_కణుపు<T_Arg>;

    // We prefetch this far into the stack
    constexpr పూర్ణాంకం Prefetch_దూరం = 2;
    // We push max 5 items per iteration
    constexpr కొలత_t Max_Pushes_చొప్పున_Iteration = 5;
    // Initial stack sie
    constexpr కొలత_t Initial_దొంతర_కొలత = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    కొలత_t దొంతర_కొలత = Initial_దొంతర_కొలత;
    కణుపు_2* లోపల_పంక్తి_దొంతర[Initial_దొంతర_కొలత];  // In-line initial stack storage
    std::unique_ptr<కణుపు_2*[]> బహిర్గతం_యొక్క_పంక్తి_Stackp{nullptr};  // In case we need more, we will allocate
    కణుపు_2** basep = లోపల_పంక్తి_దొంతర + Prefetch_దూరం;  // Pointer to base of stack
    కణుపు_2** topp = basep;  // Pointer to top of stack
    కణుపు_2** limp = లోపల_పంక్తి_దొంతర + దొంతర_కొలత - Max_Pushes_చొప్పున_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    కొరకు (పూర్ణాంకం i = -Prefetch_దూరం; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    స్థిరరాశి స్వయంచాలక సందర్శించు = [&](కణుపు_2* currp) {
        // Type test this node
        ఒకవేళ (Ast_కణుపు::వ్యక్తిగత_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            ఒకవేళ VL_CONSTEXPR_CXX17 (is_ఆకు<T_Arg_NonConst>()) తిరిగి;
        }

        // Enqueue children for traversal, unless futile
        ఒకవేళ (mayBeUnder<T_Arg_NonConst>(currp)) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి op4p = currp->op4p()) *topp++ = op4p;
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి op3p = currp->op3p()) *topp++ = op3p;
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి op2p = currp->op2p()) *topp++ = op2p;
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    ఒకవేళ (సందర్శించు_తదుపరి && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    సందర్శించు(nodep);

    // Visit the rest of the tree
    అయితే (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        కణుపు_2* స్థిరరాశి headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_దూరం]);

        // Ensure we have stack space for nextp and the 4 children
        ఒకవేళ (VL_UNLIKELY(topp >= limp)) {
            స్థిరరాశి ptrdiff_t occupancy = topp - basep;
            {
                స్థిరరాశి కొలత_t new_దొంతర_కొలత = దొంతర_కొలత * 2;
                కణుపు_2** స్థిరరాశి newStackp = new కణుపు_2*[new_దొంతర_కొలత];
                std::జ్ఞాపకం_నకలు(newStackp, basep - Prefetch_దూరం,
                            పరిమాణం(కణుపు_2**) * (occupancy + Prefetch_దూరం));
                దొంతర_కొలత = new_దొంతర_కొలత;
                బహిర్గతం_యొక్క_పంక్తి_Stackp.reset(newStackp);
            }
            basep = బహిర్గతం_యొక్క_పంక్తి_Stackp.get() + Prefetch_దూరం;
            topp = basep + occupancy;
            limp = బహిర్గతం_యొక్క_పంక్తి_Stackp.get() + దొంతర_కొలత - Max_Pushes_చొప్పున_Iteration;
        }

        // Enqueue the next node
        ఒకవేళ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        సందర్శించు(headp);
    }
}

// predicate implementation
మూస <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_కణుపు::predicateImpl(Const_Correct_Ast_కణుపు<T_Arg>* nodep, స్థిరరాశి T_Callable& p,
                            bool సందర్శించు_తదుపరి) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using కణుపు_2 = Const_Correct_Ast_కణుపు<T_Arg>;

    // We prefetch this far into the stack
    constexpr పూర్ణాంకం Prefetch_దూరం = 2;
    // We push max 5 items per iteration
    constexpr కొలత_t Max_Pushes_చొప్పున_Iteration = 5;
    // Initial stack sie
    constexpr కొలత_t Initial_దొంతర_కొలత = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    కొలత_t దొంతర_కొలత = Initial_దొంతర_కొలత;
    కణుపు_2* లోపల_పంక్తి_దొంతర[Initial_దొంతర_కొలత];  // In-line initial stack storage
    std::unique_ptr<కణుపు_2*[]> బహిర్గతం_యొక్క_పంక్తి_Stackp{nullptr};  // In case we need more, we will allocate
    కణుపు_2** basep = లోపల_పంక్తి_దొంతర + Prefetch_దూరం;  // Pointer to base of stack
    కణుపు_2** topp = basep;  // Pointer to top of stack
    కణుపు_2** limp = లోపల_పంక్తి_దొంతర + దొంతర_కొలత - Max_Pushes_చొప్పున_Iteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    కొరకు (పూర్ణాంకం i = -Prefetch_దూరం; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    స్థిరరాశి స్వయంచాలక సందర్శించు = [&](కణుపు_2* currp) {
        // Type test this node
        ఒకవేళ (Ast_కణుపు::వ్యక్తిగత_Type_Test<T_Arg_NonConst>(currp)) {
            // Call the client function
            ఒకవేళ (p(static_cast<T_Arg*>(currp)) != N_Default) తిరిగి true;
            // Short circuit if iterating leaf nodes
            ఒకవేళ VL_CONSTEXPR_CXX17 (is_ఆకు<T_Arg_NonConst>()) తిరిగి false;
        }

        // Enqueue children for traversal, unless futile
        ఒకవేళ (mayBeUnder<T_Arg_NonConst>(currp)) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి op4p = currp->op4p()) *topp++ = op4p;
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి op3p = currp->op3p()) *topp++ = op3p;
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి op2p = currp->op2p()) *topp++ = op2p;
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి op1p = currp->op1p()) *topp++ = op1p;
        }

        తిరిగి false;
    };

    // Enqueue the next of the root node, if required
    ఒకవేళ (సందర్శించు_తదుపరి && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    ఒకవేళ (సందర్శించు(nodep)) తిరిగి !N_Default;

    // Visit the rest of the tree
    అయితే (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        కణుపు_2* స్థిరరాశి headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-Prefetch_దూరం]);

        // Ensure we have stack space for nextp and the 4 children
        ఒకవేళ (VL_UNLIKELY(topp >= limp)) {
            స్థిరరాశి ptrdiff_t occupancy = topp - basep;
            {
                స్థిరరాశి కొలత_t new_దొంతర_కొలత = దొంతర_కొలత * 2;
                కణుపు_2** స్థిరరాశి newStackp = new కణుపు_2*[new_దొంతర_కొలత];
                std::జ్ఞాపకం_నకలు(newStackp, basep - Prefetch_దూరం,
                            పరిమాణం(కణుపు_2**) * (occupancy + Prefetch_దూరం));
                దొంతర_కొలత = new_దొంతర_కొలత;
                బహిర్గతం_యొక్క_పంక్తి_Stackp.reset(newStackp);
            }
            basep = బహిర్గతం_యొక్క_పంక్తి_Stackp.get() + Prefetch_దూరం;
            topp = basep + occupancy;
            limp = బహిర్గతం_యొక్క_పంక్తి_Stackp.get() + దొంతర_కొలత - Max_Pushes_చొప్పున_Iteration;
        }

        // Enqueue the next node
        ఒకవేళ (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        ఒకవేళ (సందర్శించు(headp)) తిరిగి !N_Default;
    }

    తిరిగి N_Default;
}

అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి Ast_కణుపు* rhs) {
    ఒకవేళ (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } లేకపోతే ఒకవేళ (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } లేకపోతే {
        rhs->dump(os);
    }
    తిరిగి os;
}
శూన్య VNRelinker::relink(Ast_కణుపు* newp) { newp->Ast_కణుపు::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
మూస <typename T_కణుపు>
class VNRef final : బహిరంగ std::reference_wrapper<T_కణుపు> {
    static_assert(std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

బహిరంగ:
    మూస <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_కణుపు>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(స్థిరరాశి std::reference_wrapper<T_కణుపు>& other)
        : std::reference_wrapper<T_కణుపు>{other} {}
};

static_assert(పరిమాణం(VNRef<Ast_కణుపు>) == పరిమాణం(std::reference_wrapper<Ast_కణుపు>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
కొలత_t V3HasherUncachedHash(స్థిరరాశి Ast_కణుపు&);

// Specialization of std::hash for VNRef
మూస <typename T_కణుపు>
నిర్మాణం std::hash<VNRef<T_కణుపు>> final {
    కొలత_t క్రియాకారకం()(VNRef<T_కణుపు> r) స్థిరరాశి { తిరిగి V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
మూస <typename T_కణుపు>
నిర్మాణం std::equal_గమ్యం<VNRef<T_కణుపు>> final {
    కొలత_t క్రియాకారకం()(VNRef<T_కణుపు> ra, VNRef<T_కణుపు> rb) స్థిరరాశి {
        తిరిగి ra.get().same_వృక్షం(&(rb.get()));
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
    std::unordered_map<స్థిరరాశి Ast_కణుపు*, Ast_కణుపు*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<స్థిరరాశి Ast_కణుపు*, std::సదిశ<Ast_కణుపు*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<స్థిరరాశి Ast_కణుపు*, std::string> m_prefixes;
    స్థిరరాశి VDTypeNameScopes* స్థిరరాశి m_prevp;  // Index in use before this one
    స్థిర స్థిరరాశి VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    శూన్య సూచీ(Ast_కణుపు* nodep, Ast_కణుపు* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

బహిరంగ:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    స్థిర స్థిరరాశి VDTypeNameScopes* currentp() { తిరిగి s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(స్థిరరాశి Ast_కణుపు* nodep, Ast_కణుపు*& outerpr) స్థిరరాశి;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    స్థిరరాశి std::సదిశ<Ast_కణుపు*>* paramsp(స్థిరరాశి Ast_కణుపు* modp) స్థిరరాశి;
    // The prefix kept for scope 'scopep', or nullptr if none
    స్థిరరాశి std::string* prefixp(స్థిరరాశి Ast_కణుపు* scopep) స్థిరరాశి;
    // Keep 'prefix', final, for scope 'scopep'
    శూన్య ఉపసర్గ(స్థిరరాశి Ast_కణుపు* scopep, స్థిరరాశి std::string& ఉపసర్గ) స్థిరరాశి;
};

//######################################################################
// Inline VNVisitor METHODS

శూన్య VNVisitorConst::iterateConst(Ast_కణుపు* nodep) { nodep->accept(*this); }
శూన్య VNVisitorConst::iterateConstNull(Ast_కణుపు* nodep) {
    ఒకవేళ (VL_LIKELY(nodep)) nodep->accept(*this);
}
శూన్య VNVisitorConst::iterateChildrenConst(Ast_కణుపు* nodep) { nodep->iterateChildrenConst(*this); }
శూన్య VNVisitorConst::iterateChildrenBackwardsConst(Ast_కణుపు* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
శూన్య VNVisitorConst::iterate_And_తదుపరి_Const_Null(Ast_కణుపు* nodep) {
    ఒకవేళ (VL_LIKELY(nodep)) nodep->iterate_And_తదుపరి_Const(*this);
}

శూన్య VNVisitor::iterate(Ast_కణుపు* nodep) { nodep->accept(*this); }
శూన్య VNVisitor::iterateNull(Ast_కణుపు* nodep) {
    ఒకవేళ (VL_LIKELY(nodep)) nodep->accept(*this);
}
శూన్య VNVisitor::iterateChildren(Ast_కణుపు* nodep) { nodep->iterateChildren(*this); }
శూన్య VNVisitor::iterate_And_తదుపరి_Null(Ast_కణుపు* nodep) {
    ఒకవేళ (VL_LIKELY(nodep)) nodep->iterate_And_తదుపరి(*this);
}
Ast_కణుపు* VNVisitor::iterateSubtreeReturnEdits(Ast_కణుపు* nodep) {
    తిరిగి nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#అన్తర్భావ "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#అన్తర్భావ "V3AstNodeDType.h"
#అన్తర్భావ "V3AstNodeOther.h"
#అన్తర్భావ "V3AstNodeExpr.h"
#అన్తర్భావ "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#అన్తర్భావ "V3AstInlines.h"
శూన్య dump_కణుపు_List_Json(std::ostream& os, స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి std::string& listName,
                      స్థిరరాశి string& indent);

#అయితే_ముగింపు  // Guard
