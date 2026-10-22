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

#अगर_अघोषित VERILATOR_V3AST_H_
#घोषणा VERILATOR_V3AST_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Broken.h"
#समावेश "V3Error.h"
#समावेश "V3FileLine.h"
#समावेश "V3FunctionTraits.h"
#समावेश "V3Global.h"
#समावेश "V3Number.h"
#समावेश "V3StdFuture.h"

#समावेश "V3Ast__gen_forward_class_decls.h"  // From ./astgen

#समावेश <array>
#समावेश <cmath>
#समावेश <cstdint>
#समावेश <functional>
#समावेश <map>
#समावेश <set>
#समावेश <type_traits>
#समावेश <unordered_map>
#समावेश <unordered_set>
#समावेश <utility>
#समावेश <vector>

// VL_USER_TYPE_CHECKS requires C++17 for std::variant. Ignore on older standards
#अगर defined(VL_USER_TYPE_CHECKS) && __cplusplus < 201703L
#अघोषित VL_USER_TYPE_CHECKS
#पूर्ण_अगर
#अगर_घोषित VL_USER_TYPE_CHECKS
#समावेश <variant>
#पूर्ण_अगर

// clang-format off
#समावेश "V3AstAttr.h"
// clang-format on

// Forward declarations
class V3Graph;
class ExecMTask;
class VIfaceCaptureTag;

//######################################################################

// For broken() function, return error string if have a match
#घोषणा BROKEN_RTN(test) \
    करो { \
        अगर (VL_UNCOVERABLE(test)) वापस "'" #test "' @ " __खाता__ ":" VL_STRINGIFY(__LINE__); \
    } जबतक (false)
// For broken() function, return error string if a base of this class has a match
#घोषणा BROKEN_BASE_RTN(test) \
    करो { \
        स्थिर अक्षर* स्थिर reasonp = (test); \
        अगर (VL_UNCOVERABLE(reasonp)) वापस reasonp; \
    } जबतक (false)

// (V)erilator (N)ode is: Returns true if and only if AstNode is of the given AstNode subtype,
// and not nullptr.
#घोषणा VN_IS(nodep, nodetypename) (Ast_गाँठ::is<Ast##nodetypename>(nodep))

// (V)erilator (N)ode cast: Similar to dynamic_cast, but more efficient, use this instead.
// Cast to given type if node is of such type, otherwise returns nullptr. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*',
// a 'const Ast<nodetypename>*' is returned.
#घोषणा VN_CAST(nodep, nodetypename) (Ast_गाँठ::cast<Ast##nodetypename>(nodep))

// (V)erilator (N)ode as: Assert node is of given type then cast to that type. Use this to
// downcast instead of VN_CAST when you know the true type of the node. If 'nodep' is nullptr,
// return nullptr. Pointer constness is preserved, i.e.: given a 'const AstNode*', a 'const
// Ast<nodetypename>*' is returned.
#घोषणा VN_AS(nodep, nodetypename) (Ast_गाँठ::as<Ast##nodetypename>(nodep))

// Same as VN_AS, but only checks the type in debug builds. Type checking is less critical in node
// child getters (the strongly-typed functions that wrap op*p pointers). This is because the op*p
// pointers are usually populated by code that already asserts the correct type. Having fewer type
// assertions yields better performance in release builds.
#अगर defined(VL_DEBUG) || defined(VL_ASAN)
#घोषणा VN_DBG_AS(nodep, nodetypename) VN_AS(nodep, nodetypename)
#अन्यथा
#घोषणा VN_DBG_AS(nodep, nodetypename) (Ast_गाँठ::unsafePrivateAs<Ast##nodetypename>(nodep))
#पूर्ण_अगर

// (V)erilator (N)ode deleted: Pointer to deleted AstNode (for assertions only)
#घोषणा VN_DELETED(nodep) VL_UNLIKELY(reinterpret_cast<uint64_t>(nodep) == 0x1)

//######################################################################

काष्ठा VNTypeInfo final {
    स्थिर अक्षर* m_namep;
    क्रमागत OpEn : uint8_t {
        OP_UNUSED,
        OP_USED,
        OP_LIST,
        OP_OPTIONAL,
    } m_opType[4];
    स्थिर अक्षर* m_opNamep[4];
    माप_प्रकार m_sizeof;
};

class VNType final {
    अटल स्थिर VNTypeInfo s_typeInfoTable[];

खुला:
#समावेश "V3Ast__gen_type_enum.h"  // From ./astgen
    स्थिर en m_e;
    VNType() = delete;

    // VNType is interconvetible with VNType::en
    // cppcheck-suppress noExplicitConstructor
    constexpr VNType(en _e) VL_MT_SAFE : m_e{_e} {}
    constexpr चालक en() स्थिर VL_MT_SAFE { वापस m_e; }

    // Retrieve VNTypeInfo for the given type
    अटल स्थिर VNTypeInfo& typeInfo(VNType t) VL_MT_SAFE { वापस s_typeInfoTable[t.m_e]; }
};
constexpr bool चालक==(स्थिर VNType& lhs, स्थिर VNType& rhs) VL_PURE {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VNType& lhs, VNType::en rhs) VL_PURE { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VNType::en lhs, स्थिर VNType& rhs) VL_PURE { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VNType& rhs) { वापस os << rhs.ascii(); }

//######################################################################
// AstNUser - Generic base class for AST User nodes.
//          - Also used to allow parameter passing up/down iterate calls

class WidthVP;
class V3GraphVertex;
class VSymEnt;

class VNUser final {
// Defining VL_USER_TYPE_CHECKS replaces the VNUser union with a std::variant
// which remembers which form of VNUser is being stored to catch mismatched reads.
#अगर_घोषित VL_USER_TYPE_CHECKS
    // monostate is an unwritten / cleared slot. It can be read as either form
    // and yields nullptr/0.
    std::variant<std::monostate, uint64_t, व्योम*> m_u;
#अन्यथा
    जोड़ {
        व्योम* up;
        uint64_t uq;
    } m_u;
#पूर्ण_अगर

खुला:
#अगर_घोषित VL_USER_TYPE_CHECKS
    VNUser() = शेष;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        // VNUser{0} represents the monostate
        अगर (i) m_u = i;
    }
    explicit VNUser(व्योम* p) {
        // VNUser{nullptr} represents the monostate
        अगर (p) m_u = p;
    }
    ~VNUser() = शेष;
    // Casters
    ढाँचा <typename T>
    typename std::enable_if<std::is_pointer<T>::मूल्य, T>::type to() स्थिर VL_MT_SAFE {
        अगर (std::holds_alternative<std::monostate>(m_u)) वापस nullptr;
        व्योम* स्थिर* स्थिर upp = std::get_if<व्योम*>(&m_u);
        UASSERT_STATIC(upp, "AstNode user() slot written as uint64_t, read as pointer");
        वापस reinterpret_cast<T>(*upp);
    }
    uint64_t toUQuad() स्थिर {
        अगर (std::holds_alternative<std::monostate>(m_u)) वापस 0;
        स्थिर uint64_t* स्थिर uip = std::get_if<uint64_t>(&m_u);
        UASSERT_STATIC(uip, "AstNode user() slot written as pointer, read as uint64_t");
        वापस *uip;
    }
#अन्यथा
    VNUser() = शेष;
    // non-explicit:
    // cppcheck-suppress noExplicitConstructor
    VNUser(uint64_t i) {
        m_u.up = nullptr;
        m_u.uq = i;
    }
    explicit VNUser(व्योम* p) { m_u.up = p; }
    ~VNUser() = शेष;
    // Casters
    ढाँचा <typename T>
    typename std::enable_if<std::is_pointer<T>::मूल्य, T>::type to() स्थिर VL_MT_SAFE {
        वापस reinterpret_cast<T>(m_u.up);
    }
    uint64_t toUQuad() स्थिर { वापस m_u.uq; }
#पूर्ण_अगर
    VSymEnt* toSymEnt() स्थिर { वापस to<VSymEnt*>(); }
    Ast_गाँठ* toNodep() स्थिर VL_MT_SAFE { वापस to<Ast_गाँठ*>(); }
    V3GraphVertex* toGraphVertex() स्थिर { वापस to<V3GraphVertex*>(); }
    // Render for dumps without asserting on the form held: "" if unset, "#<int>"
    // if an int, else the pointer via fmtAddrp
    std::string dumpStr(std::string (*fmtAddrp)(स्थिर व्योम*)) स्थिर;
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
VL_NOT_FINAL{रक्षित : अटल व्योम allocate(पूर्णांक id, uint32_t& cntGblRef, bool& userBusyRef){
    // Perhaps there's still a AstUserInUse in scope for this?
    UASSERT_STATIC(!userBusyRef, "Conflicting user use; AstUser" + cvtToStr(id)
                                     + "InUse request when under another AstUserInUse");
userBusyRef = true;
clearcnt(id, cntGblRef, userBusyRef);
}
अटल व्योम मुक्त(पूर्णांक id, uint32_t& cntGblRef, bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Free of User" + cvtToStr(id) + "() not under AstUserInUse");
    clearcnt(id, cntGblRef, userBusyRef);  // Includes a checkUse for us
    userBusyRef = false;
}
अटल व्योम clearcnt(पूर्णांक id, uint32_t& cntGblRef, स्थिर bool& userBusyRef) {
    UASSERT_STATIC(userBusyRef, "Clear of User" + cvtToStr(id) + "() not under AstUserInUse");
    // If this really fires and is real (after 2^32 edits???)
    // we could just walk the tree and clear manually
    ++cntGblRef;
    UASSERT_STATIC(cntGblRef, "User*() overflowed!");
}
अटल व्योम checkcnt(पूर्णांक id, uint32_t&, स्थिर bool& userBusyRef) {
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
    मित्र class Ast_गाँठ;
    अटल uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    अटल bool s_userBusy;  // Count is in use
खुला:
    VNUser1InUse()      { allocate(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser1InUse()     { मुक्त    (1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    अटल व्योम clear() { clearcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    अटल व्योम check() { checkcnt(1, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser2InUse final : VNUserInUseBase {
रक्षित:
    मित्र class Ast_गाँठ;
    अटल uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    अटल bool s_userBusy;  // Count is in use
खुला:
    VNUser2InUse()      { allocate(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser2InUse()     { मुक्त    (2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    अटल व्योम clear() { clearcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    अटल व्योम check() { checkcnt(2, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser3InUse final : VNUserInUseBase {
रक्षित:
    मित्र class Ast_गाँठ;
    अटल uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    अटल bool s_userBusy;  // Count is in use
खुला:
    VNUser3InUse()      { allocate(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser3InUse()     { मुक्त    (3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    अटल व्योम clear() { clearcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    अटल व्योम check() { checkcnt(3, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
class VNUser4InUse final : VNUserInUseBase {
रक्षित:
    मित्र class Ast_गाँठ;
    अटल uint32_t s_userCntGbl;  // Count of which usage of userp() this is
    अटल bool s_userBusy;  // Count is in use
खुला:
    VNUser4InUse()      { allocate(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    ~VNUser4InUse()     { मुक्त    (4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    अटल व्योम clear() { clearcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
    अटल व्योम check() { checkcnt(4, s_userCntGbl/*ref*/, s_userBusy/*ref*/); }
};
// clang-format on

//######################################################################
// Node deleter, deletes all enqueued AstNode* on destruction, or when
// explicitly told to do so. This is useful when the deletion of removed
// nodes needs to be deferred to a later time, because pointers to the
// removed nodes might still exist.

class VNDeleter VL_NOT_FINAL {
    // MEMBERS
    std::vector<Ast_गाँठ*> m_deleteps;  // Nodes to delete

खुला:
    // METHODS

    // Enqueue node for deletion on next 'doDelete' (or destruction)
    व्योम धकेलो_Deletep(Ast_गाँठ* nodep) {
        UASSERT_STATIC(nodep, "Cannot delete nullptr node");
        m_deleteps.धकेलो_back(nodep);
    }

    // Delete all previously pushed nodes (by calling deleteTree)
    व्योम doDeletes();

    // Do the deletions on destruction
    ~VNDeleter() { doDeletes(); }
};

//######################################################################
// VNVisitorConst -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".
// This only has the constant fuctions for non-modifying visitors.
// For more typical usage see VNVisitor

class VNVisitorConst VL_NOT_FINAL {
    मित्र class Ast_गाँठ;

खुला:
    /// Call visit()s on nodep
    अंतरभूत व्योम iterateConst(Ast_गाँठ* nodep);
    /// Call visit()s on nodep
    अंतरभूत व्योम iterateConstNull(Ast_गाँठ* nodep);
    /// Call visit()s on const nodep's children
    अंतरभूत व्योम iterateChildrenConst(Ast_गाँठ* nodep);
    /// Call visit()s on nodep's children in backp() order
    अंतरभूत व्योम iterateChildrenBackwardsConst(Ast_गाँठ* nodep);
    /// Call visit()s on const nodep (maybe nullptr) and nodep's nextp() list
    अंतरभूत व्योम iterate_And_अग्र_Const_Null(Ast_गाँठ* nodep);

    भव व्योम visit(Ast_गाँठ* nodep) = 0;
    भव ~VNVisitorConst() {}
#समावेश "V3Ast__gen_visitor_decls.h"  // From ./astgen
};

//######################################################################
// VNVisitor -- Allows new functions to be called on each node
// type without changing the base classes.  See "Modern C++ Design".

class VNVisitor VL_NOT_FINAL : खुला VNVisitorConst {
    VNDeleter m_deleter;  // Used to delay deletion of nodes
खुला:
    /// Call visit()s on nodep
    अंतरभूत व्योम iterate(Ast_गाँठ* nodep);
    /// Call visit()s on nodep
    अंतरभूत व्योम iterateNull(Ast_गाँठ* nodep);
    /// Call visit()s on nodep's children
    अंतरभूत व्योम iterateChildren(Ast_गाँठ* nodep);
    /// Call visit()s on nodep (maybe nullptr) and nodep's nextp() list
    अंतरभूत व्योम iterate_And_अग्र_Null(Ast_गाँठ* nodep);
    /// Return edited nodep; see comments in V3Ast.cpp
    अंतरभूत Ast_गाँठ* iterateSubtreeReturnEdits(Ast_गाँठ* nodep);

    VNDeleter& deleter() { वापस m_deleter; }
    व्योम धकेलो_Deletep(Ast_गाँठ* nodep) { deleter().धकेलो_Deletep(nodep); }
    व्योम doDeletes() { deleter().doDeletes(); }
};

//######################################################################
// VNRelinker -- Holds the state of a unlink so a new node can be
// added at the same point.

class VNRelinker final {
रक्षित:
    मित्र class Ast_गाँठ;
    क्रमागत RelinkWhatEn : uint8_t {
        RELINK_BAD,
        RELINK_अग्र,
        RELINK_OP1,
        RELINK_OP2,
        RELINK_OP3,
        RELINK_OP4
    };
    Ast_गाँठ* m_oldp = nullptr;  // The old node that was linked to this point in the tree
    Ast_गाँठ* m_backp = nullptr;  // Saved AstNode::m_backp of the unlinked node, to relink
    Ast_गाँठ** m_iterpp = nullptr;
    RelinkWhatEn m_chg = RELINK_BAD;

खुला:
    VNRelinker() = शेष;
    ~VNRelinker() {
        // Relink is needed so m_iterpp's get restored, e.g. can't have:
        // ->unlinkFrBack(relinker);
        // if (only_sometimes) relinker.relink(newp);
        UDEBUGONLY(
            UASSERT_STATIC(!m_backp, "Active linker must be relink()ed before destruction"););
    }
    अंतरभूत व्योम relink(Ast_गाँठ* newp);
    व्योम dump(std::ostream& str = std::cout) स्थिर;
};
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VNRelinker& rhs) {
    rhs.dump(os);
    वापस os;
}

// ######################################################################
//  Callback base class to determine if node matches some formula

class VNodeMatcher
VL_NOT_FINAL{खुला : भव bool गाँठ_Match(स्थिर Ast_गाँठ* nodep) स्थिर {वापस true;
}
}
;

// ######################################################################
//   AstNode -- Base type of all Ast types

// Prefetch a node.
#घोषणा ASTNODE_PREFETCH_NON_NULL(nodep) \
    करो { \
        VL_PREFETCH_RD(&((nodep)->m_nextp)); \
        VL_PREFETCH_RD(&((nodep)->m_type)); \
    } जबतक (false)
// The if() makes it faster, even though prefetch won't fault on null pointers
#घोषणा ASTNODE_PREFETCH(nodep) \
    करो { \
        अगर (nodep) ASTNODE_PREFETCH_NON_NULL(nodep); \
    } जबतक (false)

class Ast_गाँठ VL_NOT_FINAL {
    // v ASTNODE_PREFETCH depends on below ordering of members
    Ast_गाँठ* m_nextp = nullptr;  // Next peer in the parent's list
    Ast_गाँठ* m_backp = nullptr;  // Node that points to this one (via next/op1/op2/...)
    Ast_गाँठ* m_op1p = nullptr;  // Generic pointer 1
    Ast_गाँठ* m_op2p = nullptr;  // Generic pointer 2
    Ast_गाँठ* m_op3p = nullptr;  // Generic pointer 3
    Ast_गाँठ* m_op4p = nullptr;  // Generic pointer 4
    Ast_गाँठ** m_iterpp
        = nullptr;  // Pointer to node iterating on, change it if we replace this node.
    स्थिर VNType m_type;  // Node sub-type identifier
    // ^ ASTNODE_PREFETCH depends on above ordering of members

    // VNType is 2 bytes, so we can stick another 6 bytes after it to utilize what would
    // otherwise be padding (on a 64-bit system). We stick the attribute flags, broken state,
    // and the clone count here.

    काष्ठा {
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

#अगर defined(__x86_64__) && defined(__gnu_linux__)
    // Only assert this on known platforms, as it only affects performance, not correctness
    static_assert(माप(m_type) + माप(m_flags) + माप(m_brokenState) + माप(m_cloneCnt)
                      <= माप(व्योम*),
                  "packing requires padding");
#पूर्ण_अगर

    Ast_गाँठ_DType* m_dtypep = nullptr;  // Data type of output or assignment (etc)
    Ast_गाँठ* m_headtailp;  // When at begin/end of list, the opposite end of the list, else nullptr
    File_पंक्ति* m_fileline;  // Where it was declared
#अगर_घोषित VL_DEBUG
    // Only keep track of the edit count in the node in the debug build.
    // In the release build we will take the space saving instead.
    uint64_t m_edit_गिनती;  // When it was last edited
#पूर्ण_अगर
    अटल uint64_t s_editCntGbl;  // Global edit counter
    अटल uint64_t s_editCntLast;  // Last committed value of global edit counter

    Ast_गाँठ* m_clonep = nullptr;  // Pointer to clone/source of node (only for *LAST* cloneTree())
    अटल पूर्णांक s_cloneCntGbl;  // Count of which userp is set

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
    व्योम op1p(Ast_गाँठ* nodep) {
        m_op1p = nodep;
        अगर (nodep) nodep->m_backp = this;
    }
    व्योम op2p(Ast_गाँठ* nodep) {
        m_op2p = nodep;
        अगर (nodep) nodep->m_backp = this;
    }
    व्योम op3p(Ast_गाँठ* nodep) {
        m_op3p = nodep;
        अगर (nodep) nodep->m_backp = this;
    }
    व्योम op4p(Ast_गाँठ* nodep) {
        m_op4p = nodep;
        अगर (nodep) nodep->m_backp = this;
    }

निजी:
    Ast_गाँठ* clone_वृक्ष_Iter(bool needPure);
    Ast_गाँठ* clone_वृक्ष_Iter_List(bool needPure);
    व्योम check_वृक्ष_Iter(स्थिर Ast_गाँठ* पूर्व_Backp) स्थिर VL_MT_STABLE;
    bool gate_वृक्ष_Iter() स्थिर;
    अटल bool same_वृक्ष_Iter(स्थिर Ast_गाँठ* node1p, स्थिर Ast_गाँठ* node2p, bool ign_अग्र,
                             bool gateOnly);
    व्योम delete_वृक्ष_Iter();
    व्योम delete_गाँठ();
    string instanceStr() स्थिर;

खुला:
    व्योम purityCheck();
    अटल व्योम relinkOneLink(Ast_गाँठ*& pointpr, Ast_गाँठ* newp);
    // cppcheck-suppress functionConst
    अटल व्योम debug_वृक्ष_Change(स्थिर Ast_गाँठ* nodep, स्थिर अक्षर* prefix, पूर्णांक lineno, bool अग्र);

रक्षित:
    // CONSTRUCTORS
    Ast_गाँठ(VNType t, File_पंक्ति* fl);
    भव ~Ast_गाँठ() = शेष;  // Use 'deleteTree' instead
    भव Ast_गाँठ* clone() = 0;  // Generally, cloneTree is what you want instead
    भव व्योम cloneRelink() { cloneRelinkGen(); }
    भव व्योम cloneRelinkGen() {};  // Overrides generated by 'astgen'
    व्योम clone_Relink_वृक्ष();

    // METHODS
    व्योम setOp1p(Ast_गाँठ* newp);  // Set non-list-type op1 to non-list element
    व्योम setOp2p(Ast_गाँठ* newp);  // Set non-list-type op2 to non-list element
    व्योम setOp3p(Ast_गाँठ* newp);  // Set non-list-type op3 to non-list element
    व्योम setOp4p(Ast_गाँठ* newp);  // Set non-list-type op4 to non-list element

    व्योम addOp1p(Ast_गाँठ* newp);  // Append newp to end of op1
    व्योम addOp2p(Ast_गाँठ* newp);  // Append newp to end of op2
    व्योम addOp3p(Ast_गाँठ* newp);  // Append newp to end of op3
    व्योम addOp4p(Ast_गाँठ* newp);  // Append newp to end of op4

    व्योम prependOp1p(Ast_गाँठ* newp);  // Prepend newp to front of op1
    व्योम prependOp2p(Ast_गाँठ* newp);  // Prepend newp to front of op2
    व्योम prependOp3p(Ast_गाँठ* newp);  // Prepend newp to front of op3
    व्योम prependOp4p(Ast_गाँठ* newp);  // Prepend newp to front of op4

    // clang-format off
    व्योम setNOp1p(Ast_गाँठ* newp) { अगर (newp) setOp1p(newp); }
    व्योम setNOp2p(Ast_गाँठ* newp) { अगर (newp) setOp2p(newp); }
    व्योम setNOp3p(Ast_गाँठ* newp) { अगर (newp) setOp3p(newp); }
    व्योम setNOp4p(Ast_गाँठ* newp) { अगर (newp) setOp4p(newp); }

    व्योम addNOp1p(Ast_गाँठ* newp) { अगर (newp) addOp1p(newp); }
    व्योम addNOp2p(Ast_गाँठ* newp) { अगर (newp) addOp2p(newp); }
    व्योम addNOp3p(Ast_गाँठ* newp) { अगर (newp) addOp3p(newp); }
    व्योम addNOp4p(Ast_गाँठ* newp) { अगर (newp) addOp4p(newp); }

    व्योम prependNOp1p(Ast_गाँठ* newp) { अगर (newp) prependOp1p(newp); }
    व्योम prependNOp2p(Ast_गाँठ* newp) { अगर (newp) prependOp2p(newp); }
    व्योम prependNOp3p(Ast_गाँठ* newp) { अगर (newp) prependOp3p(newp); }
    व्योम prependNOp4p(Ast_गाँठ* newp) { अगर (newp) prependOp4p(newp); }
    // clang-format on

    व्योम clonep(Ast_गाँठ* nodep) {
        m_clonep = nodep;
        m_cloneCnt = s_cloneCntGbl;
    }
    अटल व्योम clone_Clear_वृक्ष() {
        s_cloneCntGbl++;
        UASSERT_STATIC(s_cloneCntGbl, "Rollover");
    }

    // Use instead isSame(), this is for each Ast* class, and assumes node is of same type
    भव bool same_गाँठ(स्थिर Ast_गाँठ*) स्थिर { वापस true; }
    // Generated by 'astgen'. If do an oldp->replaceNode(newp), would cause a broken()
    भव bool wouldBreakGen(स्थिर Ast_गाँठ* स्थिर oldp,
                               स्थिर Ast_गाँठ* स्थिर newp) स्थिर
        = 0;  // Generated by 'astgen'

खुला:
    // ACCESSORS
    VNType type() स्थिर VL_MT_SAFE { वापस m_type; }
    स्थिर अक्षर* typeName() स्थिर VL_MT_SAFE { वापस type().ascii(); }  // See also prettyTypeName
    Ast_गाँठ* nextp() स्थिर VL_MT_STABLE { वापस m_nextp; }
    Ast_गाँठ* backp() स्थिर VL_MT_STABLE { वापस m_backp; }
    Ast_गाँठ* abovep() स्थिर;  // Get parent node above, only for list head and tail
    Ast_गाँठ* aboveLoopp() स्थिर;  // Get parent node above, may have performance issues as loops
    Ast_गाँठ* lastp() स्थिर {  // Get last node in list, only for list head
        UASSERT_OBJ(m_backp->m_nextp != this, this, "lastp() only allowed on head of list");
        वापस m_headtailp;
    }
    Ast_गाँठ* prevp() स्थिर {  // Previous node in list, nullptr for head of list
        अगर (m_backp->m_nextp != this) वापस nullptr;
        वापस m_backp;
    }
    Ast_गाँठ* op1p() स्थिर VL_MT_STABLE { वापस m_op1p; }
    Ast_गाँठ* op2p() स्थिर VL_MT_STABLE { वापस m_op2p; }
    Ast_गाँठ* op3p() स्थिर VL_MT_STABLE { वापस m_op3p; }
    Ast_गाँठ* op4p() स्थिर VL_MT_STABLE { वापस m_op4p; }
    Ast_गाँठ_DType* dtypep() स्थिर VL_MT_STABLE { वापस m_dtypep; }
    Ast_गाँठ* clonep() स्थिर { वापस ((m_cloneCnt == s_cloneCntGbl) ? m_clonep : nullptr); }
    Ast_गाँठ* firstAbovep() स्थिर {  // Returns nullptr when second or later in list
        वापस ((backp() && backp()->nextp() != this) ? backp() : nullptr);
    }
    uint8_t brokenState() स्थिर VL_MT_SAFE { वापस m_brokenState; }
    व्योम brokenState(uint8_t मूल्य) { m_brokenState = मूल्य; }

    // Used by AstNode::broken()
    bool brokeExists() स्थिर { वापस V3Broken::isLinkable(this); }
    bool brokeExistsAbove() स्थिर { वापस brokeExists() && (m_brokenState >> 7); }

    // Perform a function on every link in a node
    भव व्योम foreachLink(std::function<व्योम(Ast_गाँठ** linkpp, स्थिर अक्षर* namep)> f) = 0;

#अगर defined(VL_LEAK_CHECKS) || defined(VL_ALLOC_RANDOM_CHECKS)
    अटल व्योम* चालक new(माप_प्रकार आकार);
    अटल व्योम चालक delete(व्योम* obj, माप_प्रकार आकार);
#पूर्ण_अगर

    // CONSTANTS
    // The following are relative dynamic costs (~ execution cycle count) of various operations.
    // They are used by V3InstCount to estimate the relative execution time of code fragments.
    अटल constexpr पूर्णांक INSTR_गिनती_BRANCH = 4;  // Branch
    अटल constexpr पूर्णांक INSTR_गिनती_CALL = INSTR_गिनती_BRANCH + 10;  // Subroutine call
    अटल constexpr पूर्णांक INSTR_गिनती_LD = 2;  // Load memory
    अटल constexpr पूर्णांक INSTR_गिनती_INT_MUL = 3;  // Integer multiply
    अटल constexpr पूर्णांक INSTR_गिनती_INT_DIV = 10;  // Integer divide
    अटल constexpr पूर्णांक INSTR_गिनती_DBL = 8;  // Convert or do float ops
    अटल constexpr पूर्णांक INSTR_गिनती_DBL_DIV = 40;  // Double divide
    अटल constexpr पूर्णांक INSTR_गिनती_DBL_TRIG = 200;  // Double trigonometric ops
    अटल constexpr पूर्णांक INSTR_गिनती_STR = 100;  // String ops
    अटल constexpr पूर्णांक INSTR_गिनती_TIME = INSTR_गिनती_CALL + 5;  // Determine simulation time
    अटल constexpr पूर्णांक INSTR_गिनती_PLI = 20;  // PLI routines
    अटल constexpr पूर्णांक INSTR_गिनती_CTOR_DTOR = 5;  // Syms and Module ctor/dtor statements

    // ACCESSORS
    भव string name() स्थिर VL_MT_STABLE { वापस ""; }
    भव string origName() स्थिर { वापस ""; }
    string prettyDehashOrigOrName() स्थिर {
        वापस prettyName(VName::dehash(origName().empty() ? name() : origName()));
    }
    भव व्योम name(स्थिर string& name) {
        this->v3fatalSrc("name() called on object without name() method");
    }
    भव व्योम tag(स्थिर string& text) {}
    भव string tag() स्थिर { वापस ""; }
    भव uint32_t decl_Token_अंक() स्थिर { वापस 0; }
    भव व्योम decl_Token_अंक_Set_Min(uint32_t token_अंक) {}
    भव string verilogKwd() स्थिर { वापस ""; }
    string nameProtect() स्थिर VL_MT_STABLE;  // Name with --protect-id applied
    string origNameProtect() स्थिर;  // origName with --protect-id applied
    string shortName() स्थिर;  // Name with __PVT__ removed for concatenating scopes
    अटल string dedotName(स्थिर string& namein);  // Name with dots removed
    अटल string nameNoArray(स्थिर string& namein);  // Name with any array index removed
    अटल string prettyName(स्थिर string& namein) VL_PURE;  // Name for printing out to the user
    अटल string vpiName(स्थिर string& namein);  // Name for vpi access
    अटल string prettyNameQ(स्थिर string& namein) {  // Quoted pretty name (for errors)
        वापस "'"s + prettyName(namein) + "'";
    }
    // Encode user name into internal C representation
    अटल string encodeName(स्थिर string& namein);
    अटल string encode_संख्या(int64_t अंक);  // Encode number into internal C representation
    अटल string vcdName(स्थिर string& namein);  // Name for printing out to vcd files
    string prettyName() स्थिर { वापस prettyName(name()); }
    // Name for messages to the user, by default prettyName(), but friendlier for some nodes,
    // such as a specialized class, whose name() is internal
    भव string pretty_Name_संदेश() स्थिर { वापस prettyName(); }
    string prettyNameQ() स्थिर { वापस "'"s + pretty_Name_संदेश() + "'"; }  // Quoted, for messages
    string verilogName() स्थिर { वापस vpiName(origName()); }  // Decoded original Verilog name
    // "VARREF" for error messages (NOT dtype's pretty name)
    string prettyTypeName() स्थिर;
    भव string prettyOperatorName() स्थिर { वापस "operator " + prettyTypeName(); }
    File_पंक्ति* fileline() स्थिर VL_MT_SAFE { वापस m_fileline; }
    व्योम fileline(File_पंक्ति* fl) { m_fileline = fl; }
    अंतरभूत bool width1() स्थिर;
    अंतरभूत पूर्णांक widthInstrs() स्थिर;
    bool didWidth() स्थिर { वापस m_flags.didWidth; }
    व्योम didWidth(bool flag) { m_flags.didWidth = flag; }
    bool didWidthAndSet() {
        अगर (didWidth()) वापस true;
        didWidth(true);
        वापस false;
    }
    bool doingWidth() स्थिर { वापस m_flags.doingWidth; }
    व्योम doingWidth(bool flag) { m_flags.doingWidth = flag; }
    bool protect() स्थिर VL_MT_SAFE { वापस m_flags.protect; }
    व्योम protect(bool flag) { m_flags.protect = flag; }

    // TODO stomp these width functions out, and call via dtypep() instead
    अंतरभूत पूर्णांक width() स्थिर VL_MT_STABLE;
    अंतरभूत पूर्णांक widthMin() स्थिर VL_MT_STABLE;
    पूर्णांक widthMinV() स्थिर {
        वापस v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH ? widthMin() : width();
    }
    पूर्णांक width_शब्दगण() स्थिर { वापस VL_शब्दगण_I(width()); }
    bool isQuad() स्थिर VL_MT_STABLE { वापस (width() > VL_IDATASIZE && width() <= VL_QUADSIZE); }
    bool isWide() स्थिर VL_MT_STABLE { वापस (width() > VL_QUADSIZE); }
    अंतरभूत bool isCHandle() स्थिर VL_MT_STABLE;
    अंतरभूत bool isDouble() स्थिर VL_MT_STABLE;
    अंतरभूत bool isSigned() स्थिर VL_MT_STABLE;
    अंतरभूत bool isString() स्थिर VL_MT_STABLE;
    अंतरभूत bool isEvent() स्थिर VL_MT_STABLE;

    // clang-format off
    VNUser user1u() स्थिर VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser1InUse::s_userBusy, "user1p used without AstUserInUse");
        वापस ((m_user1Cnt == VNUser1InUse::s_userCntGbl) ? m_user1u : VNUser{nullptr});
    }
    Ast_गाँठ* user1p() स्थिर VL_MT_STABLE { वापस user1u().toNodep(); }
    व्योम user1u(स्थिर VNUser& user) { m_user1u = user; m_user1Cnt = VNUser1InUse::s_userCntGbl; }
    व्योम user1p(व्योम* userp) { user1u(VNUser{userp}); }
    व्योम user1(uint64_t val) { user1u(VNUser{val}); }
    uint64_t user1() स्थिर { वापस user1u().toUQuad(); }
    uint64_t user1Inc(uint64_t val = 1) { स्थिर uint64_t v = user1(); user1(v + val); वापस v; }
    uint64_t user1Or(uint64_t val) { स्थिर uint64_t v = user1(); user1(v | val); वापस v; }
    uint64_t user1SetOnce() { स्थिर uint64_t v = user1(); अगर (!v) user1(1); वापस v; }  // Better for cache than user1Inc()
    अटल व्योम user1_Clear_वृक्ष() { VNUser1InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user2u() स्थिर VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser2InUse::s_userBusy, "user2p used without AstUserInUse");
        वापस ((m_user2Cnt == VNUser2InUse::s_userCntGbl) ? m_user2u : VNUser{nullptr});
    }
    Ast_गाँठ* user2p() स्थिर VL_MT_STABLE { वापस user2u().toNodep(); }
    व्योम user2u(स्थिर VNUser& user) { m_user2u = user; m_user2Cnt = VNUser2InUse::s_userCntGbl; }
    व्योम user2p(व्योम* userp) { user2u(VNUser{userp}); }
    व्योम user2(uint64_t val) { user2u(VNUser{val}); }
    uint64_t user2() स्थिर { वापस user2u().toUQuad(); }
    uint64_t user2Inc(uint64_t val = 1) { स्थिर uint64_t v = user2(); user2(v + val); वापस v; }
    uint64_t user2Or(uint64_t val) { स्थिर uint64_t v = user2(); user2(v | val); वापस v; }
    uint64_t user2SetOnce() { स्थिर uint64_t v = user2(); अगर (!v) user2(1); वापस v; }  // Better for cache than user2Inc()
    अटल व्योम user2_Clear_वृक्ष() { VNUser2InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user3u() स्थिर VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser3InUse::s_userBusy, "user3p used without AstUserInUse");
        वापस ((m_user3Cnt == VNUser3InUse::s_userCntGbl) ? m_user3u : VNUser{nullptr});
    }
    Ast_गाँठ* user3p() स्थिर VL_MT_STABLE { वापस user3u().toNodep(); }
    व्योम user3u(स्थिर VNUser& user) { m_user3u = user; m_user3Cnt = VNUser3InUse::s_userCntGbl; }
    व्योम user3p(व्योम* userp) { user3u(VNUser{userp}); }
    व्योम user3(uint64_t val) { user3u(VNUser{val}); }
    uint64_t user3() स्थिर { वापस user3u().toUQuad(); }
    uint64_t user3Inc(uint64_t val = 1) { स्थिर uint64_t v = user3(); user3(v + val); वापस v; }
    uint64_t user3Or(uint64_t val) { स्थिर uint64_t v = user3(); user3(v | val); वापस v; }
    uint64_t user3SetOnce() { स्थिर uint64_t v = user3(); अगर (!v) user3(1); वापस v; }  // Better for cache than user3Inc()
    अटल व्योम user3_Clear_वृक्ष() { VNUser3InUse::clear(); }  // Clear userp()'s across the entire tree

    VNUser user4u() स्थिर VL_MT_STABLE {
        // Slows things down measurably, so disabled by default
        //UASSERT_STATIC(VNUser4InUse::s_userBusy, "user4p used without AstUserInUse");
        वापस ((m_user4Cnt == VNUser4InUse::s_userCntGbl) ? m_user4u : VNUser{nullptr});
    }
    Ast_गाँठ* user4p() स्थिर VL_MT_STABLE { वापस user4u().toNodep(); }
    व्योम user4u(स्थिर VNUser& user) { m_user4u = user; m_user4Cnt = VNUser4InUse::s_userCntGbl; }
    व्योम user4p(व्योम* userp) { user4u(VNUser{userp}); }
    व्योम user4(uint64_t val) { user4u(VNUser{val}); }
    uint64_t user4() स्थिर { वापस user4u().toUQuad(); }
    uint64_t user4Or(uint64_t val) { स्थिर uint64_t v = user4(); user4(v | val); वापस v; }
    uint64_t user4Inc(uint64_t val = 1) { स्थिर uint64_t v = user4(); user4(v + val); वापस v; }
    uint64_t user4SetOnce() { स्थिर uint64_t v = user4(); अगर (!v) user4(1); वापस v; }  // Better for cache than user4Inc()
    अटल व्योम user4_Clear_वृक्ष() { VNUser4InUse::clear(); }  // Clear userp()'s across the entire tree
    // clang-format on

#अगर_घोषित VL_DEBUG
    uint64_t edit_गिनती() स्थिर { वापस m_edit_गिनती; }
    व्योम edit_गिनती_Inc() {
        m_edit_गिनती = ++s_editCntGbl;  // Preincrement, so can "watch AstNode::s_editCntGbl=##"
        VIsCached::clear_Cache_वृक्ष();  // Any edit clears all caching
    }
#अन्यथा
    व्योम edit_गिनती_Inc() { ++s_editCntGbl; }
#पूर्ण_अगर
    अटल uint64_t edit_गिनती_Last() VL_MT_SAFE { वापस s_editCntLast; }
    अटल uint64_t edit_गिनती_Gbl() VL_MT_SAFE { वापस s_editCntGbl; }
    अटल व्योम edit_गिनती_Set_Last() { s_editCntLast = edit_गिनती_Gbl(); }

    // ACCESSORS for specific types
    // Alas these can't be virtual or they break when passed a nullptr
    bool is_Disable_Queue_धकेलो_Self_Stmt();
    अंतरभूत bool is_Class_Handle_मूल्य() स्थिर;
    अंतरभूत bool isNull() स्थिर;
    अंतरभूत bool isZero() स्थिर;
    अंतरभूत bool isOne() स्थिर;
    अंतरभूत bool isNeqZero() स्थिर;
    अंतरभूत bool isAllOnes() स्थिर;
    अंतरभूत bool isAllOnesV() स्थिर;  // Verilog width rules apply

    // METHODS - data type changes especially for initial creation
    व्योम dtypep(Ast_गाँठ_DType* nodep) {
        अगर (m_dtypep != nodep) {
            m_dtypep = nodep;
            edit_गिनती_Inc();
        }
    }
    व्योम dtypeFrom(स्थिर Ast_गाँठ* fromp) {
        अगर (fromp) dtypep(fromp->dtypep());
    }
    व्योम dtypeChgSigned(bool flag = true);
    व्योम dtypeChgWidth(पूर्णांक width, पूर्णांक widthMin);
    व्योम dtypeChgWidthSigned(पूर्णांक width, पूर्णांक widthMin, VSigning numeric);
    व्योम dtypeSetBitUnsized(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) {
        dtypep(findBitDType(width, widthMin, numeric));
    }
    व्योम dtypeSetBitSized(पूर्णांक width, VSigning numeric) {
        dtypep(findBitDType(width, width, numeric));  // Since sized, widthMin is width
    }
    व्योम dtypeSetLogicUnsized(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) {
        dtypep(findLogicDType(width, widthMin, numeric));
    }
    व्योम dtypeSetLogicSized(पूर्णांक width, VSigning numeric) {
        dtypep(findLogicDType(width, width, numeric));  // Since sized, widthMin is width
    }
    व्योम dtypeSetLogic2StateUnsized(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) {
        dtypep(findLogic2StateDType(width, widthMin, numeric));
    }
    व्योम dtypeSetLogic2StateSized(पूर्णांक width, VSigning numeric) {
        dtypep(findLogic2StateDType(width, width, numeric));  // Since sized, widthMin is width
    }
    व्योम dtypeSetBitOrLogicUnsized(पूर्णांक width, पूर्णांक widthMin, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, widthMin, numeric, isFourstate));
    }
    व्योम dtypeSetBitOrLogicSized(पूर्णांक width, VSigning numeric, bool isFourstate) {
        dtypep(findBitOrLogicDType(width, width, numeric,
                                   isFourstate));  // Since sized, widthMin is width
    }
    व्योम dtypeSetBit() { dtypep(findBitDType()); }
    व्योम dtypeSetLogic() { dtypep(findLogicDType()); }
    व्योम dtypeSetLogic2State() { dtypep(findLogic2StateDType()); }
    व्योम dtypeSetDouble() { dtypep(findDoubleDType()); }
    व्योम dtypeSetInt() { dtypep(findIntDType()); }
    व्योम dtypeSetInteger() { dtypep(findIntegerDType()); }
    व्योम dtypeSetInteger2State() { dtypep(findInteger2StateDType()); }
    व्योम dtypeSetString() { dtypep(findStringDType()); }
    व्योम dtypeSetUInt32() { dtypep(findUInt32DType()); }  // Twostate
    व्योम dtypeSetUInt64() { dtypep(findUInt64DType()); }  // Twostate
    व्योम dtypeSetTime() { dtypep(findTimeDType()); }
    व्योम dtypeSetTime2State() { dtypep(findTime2StateDType()); }
    व्योम dtypeSetEmptyQueue() { dtypep(findEmptyQueueDType()); }
    व्योम dtypeSetStream() { dtypep(findStreamDType()); }
    व्योम dtypeSetVoid() { dtypep(findVoidDType()); }
    व्योम dtypeSetBitOrLogic(bool isFourstate) {
        dtypep(isFourstate ? findLogicDType() : findBitDType());
    }

    // Data type locators
    Ast_गाँठ_DType* findBitDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::BIT); }
    Ast_गाँठ_DType* findLogicDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::LOGIC); }
    Ast_गाँठ_DType* findNullDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::NULLTYPE); }
    Ast_गाँठ_DType* findLogic2StateDType() स्थिर {
        वापस findBasicDType(VBasicDTypeKwd::LOGIC2STATE);
    }
    Ast_गाँठ_DType* findDoubleDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::DOUBLE); }
    Ast_गाँठ_DType* findIntDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::INT); }
    Ast_गाँठ_DType* findIntegerDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::INTEGER); }
    Ast_गाँठ_DType* findInteger2StateDType() स्थिर {
        वापस findBasicDType(VBasicDTypeKwd::INTEGER2STATE);
    }
    Ast_गाँठ_DType* findStringDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::STRING); }
    Ast_गाँठ_DType* findSigned8DType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::BYTE); }
    Ast_गाँठ_DType* findUInt32DType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::UINT32); }
    Ast_गाँठ_DType* findUInt64DType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::UINT64); }
    Ast_गाँठ_DType* findTimeDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::TIME); }
    Ast_गाँठ_DType* findTime2StateDType() स्थिर {
        वापस findBasicDType(VBasicDTypeKwd::TIME2STATE);
    }
    Ast_गाँठ_DType* findCHandleDType() स्थिर { वापस findBasicDType(VBasicDTypeKwd::CHANDLE); }
    Ast_गाँठ_DType* findConstraintRefDType() स्थिर;
    Ast_गाँठ_DType* findEmptyQueueDType() स्थिर;
    Ast_गाँठ_DType* find_Queue_सूचकांक_DType() स्थिर;
    Ast_गाँठ_DType* findStreamDType() स्थिर;
    Ast_गाँठ_DType* findVoidDType() स्थिर;
    Ast_गाँठ_DType* findBitDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिर;
    Ast_गाँठ_DType* findLogicDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिर;
    Ast_गाँठ_DType* findLogic2StateDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिर;
    Ast_गाँठ_DType* findBitOrLogicDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric,
                                      bool isFourstate) स्थिर {
        वापस isFourstate ? findLogicDType(width, widthMin, numeric)
                           : findBitDType(width, widthMin, numeric);
    }
    Ast_गाँठ_DType* findBitOrLogicRangeDType(स्थिर VNumRange& range, स्थिर पूर्णांक widthMin,
                                           स्थिर VSigning numeric,
                                           स्थिर bool isFourstate) स्थिर VL_MT_STABLE {
        वापस isFourstate ? findLogicRangeDType(range, widthMin, numeric)
                           : findBitRangeDType(range, widthMin, numeric);
    }
    Ast_गाँठ_DType* findLogicRangeDType(स्थिर VNumRange& range, पूर्णांक widthMin,
                                      VSigning numeric) स्थिर VL_MT_STABLE;
    Ast_गाँठ_DType* findBitRangeDType(स्थिर VNumRange& range, पूर्णांक widthMin,
                                    VSigning numeric) स्थिर VL_MT_STABLE;
    Ast_गाँठ_DType* findBasicDType(VBasicDTypeKwd kwd) स्थिर;
    अटल AstBasicDType* find_जोड़ो_Same_DType(AstBasicDType* nodep);

    अटल VCastable computeCastable(स्थिर Ast_गाँठ_DType* toDtp, स्थिर Ast_गाँठ_DType* fromDtp,
                                     स्थिर Ast_गाँठ* fromConstp, bool checkIfaceArgCompat = false);
    अटल Ast_गाँठ_DType* getCommonClassTypep(Ast_गाँठ* nodep1, Ast_गाँठ* nodep2);

    // METHODS - dump and error
    व्योम v3errorEnd(स्थिर std::ostringstream& str) स्थिर VL_RELEASE(V3Error::s().m_mutex);
    व्योम v3errorEndFatal(स्थिर std::ostringstream& str) स्थिर VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    string warnContextPrimary() स्थिर VL_REQUIRES(V3Error::s().m_mutex) {
        वापस fileline()->warnContextPrimary();
    }
    string warnContextSecondary() स्थिर { वापस fileline()->warnContextSecondary(); }
    string warnMore() स्थिर VL_REQUIRES(V3Error::s().m_mutex) { वापस fileline()->warnMore(); }
    string warnOther() स्थिर VL_REQUIRES(V3Error::s().m_mutex) { वापस fileline()->warnOther(); }

    भव व्योम dump(std::ostream& str = std::cout) स्थिर;
    अटल अक्षर* dump_वृक्ष_Json_Gdb(स्थिर Ast_गाँठ* nodep);  // For GDB only, free()d by caller
    अटल अक्षर* dump_वृक्ष_Json_Gdb(स्थिर अक्षर* str);  // For GDB only, free()d by caller
    अटल अक्षर* dump_वृक्ष_Json_Gdb(intptr_t nodep);  // For GDB only, free()d by caller
    अटल व्योम dumpGdb(स्थिर Ast_गाँठ* nodep);  // For GDB only
    व्योम dumpGdbHeader() स्थिर;

    // METHODS - Tree modifications
    // Returns nodep. If newp is not nullptr, then add it to end of nodep's list.
    ढाँचा <typename T_गाँठ_परिणाम, typename T_गाँठ_अग्र>
    अटल T_गाँठ_परिणाम* add_अग्र(T_गाँठ_परिणाम* nodep, T_गाँठ_अग्र* newp) {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ_परिणाम>::मूल्य,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_गाँठ_परिणाम, T_गाँठ_अग्र>::मूल्य,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        अगर (!newp) वापस nodep;
        वापस static_cast<T_गाँठ_परिणाम*>(add_अग्र<Ast_गाँठ, Ast_गाँठ>(nodep, newp));
    }
    ढाँचा <typename T_गाँठ_परिणाम, typename T_गाँठ_अग्र>
    अटल T_गाँठ_परिणाम* add_अग्र_Null(T_गाँठ_परिणाम* nodep, T_गाँठ_अग्र* newp) {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ_परिणाम>::मूल्य,
                      "'T_NodeResult' must be a subtype of AstNode");
        static_assert(std::is_base_of<T_गाँठ_परिणाम, T_गाँठ_अग्र>::मूल्य,
                      "'T_NodeNext' must be a subtype of 'T_NodeResult'");
        अगर (!newp) वापस nodep;
        अगर (!nodep) वापस newp;
        वापस static_cast<T_गाँठ_परिणाम*>(add_अग्र<Ast_गाँठ, Ast_गाँठ>(nodep, newp));
    }
    अंतरभूत Ast_गाँठ* add_अग्र(Ast_गाँठ* newp);
    व्योम add_अग्र_Here(Ast_गाँठ* newp);  // Insert newp at this->nextp
    व्योम add_Here_This_As_अग्र(Ast_गाँठ* newp);  // Adds at old place of this, this becomes next
    व्योम replaceWith(Ast_गाँठ* newp);  // Replace current node in tree with new node
    व्योम replaceWithKeepDType(Ast_गाँठ* newp);  // Replace current node in tree, keep old dtype
    // Unlink this from whoever points to it.
    Ast_गाँठ* unlinkFrBack(VNRelinker* linkerp = nullptr);
    // Unlink this from whoever points to it, keep entire next list with unlinked node
    Ast_गाँठ* unlink_Fr_Back_With_अग्र(VNRelinker* linkerp = nullptr);
    व्योम relink(VNRelinker* linkerp);  // Generally use linker->relink() instead
    व्योम clone_Relink_गाँठ() { cloneRelink(); }

    // METHODS - Iterate on a tree
    Ast_गाँठ* clone_वृक्ष(bool clone_अग्र_Link,
                       bool needPure = false);  // Not const, as sets clonep() on original nodep
    Ast_गाँठ* clone_वृक्ष_Pure(bool clone_अग्र_Link) { वापस clone_वृक्ष(clone_अग्र_Link, true); }
    bool gate_वृक्ष() { वापस gate_वृक्ष_Iter(); }  // Is tree isGateOptimizable?
    अंतरभूत bool same_वृक्ष(स्थिर Ast_गाँठ* node2p) स्थिर;  // Does tree of this == node2p?
    // Does tree of this == node2p?, not allowing non-isGateOptimizable
    अंतरभूत bool same_Gate_वृक्ष(स्थिर Ast_गाँठ* node2p) स्थिर;
    व्योम delete_वृक्ष();  // Always deletes the next link
    व्योम check_वृक्ष() स्थिर VL_MT_STABLE {
        अगर (v3Global.opt.debugCheck()) check_वृक्ष_Iter(backp());
    }
    व्योम checkIter() स्थिर;
    व्योम dumpPtrs(std::ostream& os = std::cout) स्थिर;
    व्योम dump_वृक्ष(std::ostream& os = std::cout, स्थिर string& indent = "    ",
                  पूर्णांक maxDepth = 0) स्थिर;
    व्योम dump_वृक्ष(स्थिर string& indent, पूर्णांक maxDepth = 0) स्थिर {
        dump_वृक्ष(std::cout, indent, maxDepth);
    }
    अटल व्योम dump_वृक्ष_Gdb(स्थिर Ast_गाँठ* nodep);  // For GDB only
    व्योम dump_वृक्ष_And_अग्र(std::ostream& os = std::cout, स्थिर string& indent = "    ",
                         पूर्णांक maxDepth = 0) स्थिर;
    व्योम dump_वृक्ष_File(स्थिर string& filename, bool doDump = true);
    अटल व्योम dump_वृक्ष_File_Gdb(स्थिर Ast_गाँठ* nodep, स्थिर अक्षर* filenamep = nullptr);
    व्योम dump_वृक्ष_Dot(std::ostream& os = std::cout) स्थिर;
    व्योम dump_वृक्ष_Dot_File(स्थिर string& filename, bool doDump = true);
    भव व्योम dumpJson(std::ostream& os) स्थिर { dumpJsonGen(os); };  // node-specific fields
    // Generated by 'astgen'. Dumps node-specific pointers and calls 'dumpJson()' of parent class
    // Note that we don't make it virtual as it would result in infinite recursion
    व्योम dumpJsonGen(std::ostream& os) स्थिर {};
    भव व्योम dump_वृक्ष_Json_Op_Gen(std::ostream& os, स्थिर string& indent) स्थिर {};
    व्योम dump_वृक्ष_Json(std::ostream& os, स्थिर string& indent = "") स्थिर;
    व्योम dump_वृक्ष_Json_File(स्थिर string& filename, bool doDump = true);
    अटल व्योम dumpJsonMetaFileGdb(स्थिर अक्षर* filename);
    अटल व्योम dumpJsonMetaFile(स्थिर string& filename);

    // Render node address for dumps. By default this is just the address
    // printed as hex, but with --dump-tree-addrids we map addresses to short
    // strings with a bijection to aid human readability. Observe that this might
    // not actually be a unique identifier as the address can get reused after a
    // node has been freed.
    अटल std::string गाँठ_Addr(स्थिर Ast_गाँठ* nodep) {
        वापस v3Global.opt.dump_वृक्ष_Addrids() ? v3Global.ptrToId(nodep) : cvtToHex(nodep);
    }

    // METHODS - static advancement
    अटल Ast_गाँठ* afterCommentp(Ast_गाँठ* nodep) {
        // Skip over comments starting at provided nodep,
        // such as to determine if a AstIf is empty.
        // nodep may be null, if so return nullptr.
        जबतक (nodep && VN_IS(nodep, Comment)) nodep = nodep->nextp();
        वापस nodep;
    }

    // METHODS - queries
    // Changes control flow, disable some optimizations
    भव bool isBrancher() स्थिर { वापस false; }
    // Else a AstTime etc that can't be pushed out
    भव bool isGateOptimizable() स्थिर { वापस !isTimingControl(); }
    // GateDedupable is a slightly larger superset of GateOptimzable (eg, AstNodeIf)
    भव bool isGateDedupable() स्थिर { वापस isGateOptimizable(); }
    // Whether the node can be used in expression coverage
    भव bool is_व्यंजक_Coverage_Eligible() स्थिर { वापस isGateDedupable(); }
    // Else creates output or exits, etc, not unconsumed
    भव bool isOutputter() { वापस false; }
    // Else a AstTime etc which output can't be predicted from input
    भव bool isPredictOptimizable() स्थिर { वापस !isTimingControl(); }
    // Else a $display, etc, that must be ordered with other displays
    भव bool isPure() { वापस true; }
    // Iff isPure on current node and any nextp()
    bool is_Pure_And_अग्र() { वापस isPure() && (!nextp() || nextp()->isPure()); }
    // An event control, delay, wait, etc.
    भव bool isTimingControl() स्थिर { वापस false; }
    // isUnlikely handles $stop or similar statement which means an above IF
    // statement is unlikely to be taken
    भव bool isUnlikely() स्थिर { वापस false; }
    // Is an IEEE system function (versus internally-generated)
    भव bool isSystemFunc() स्थिर { वापस false; }
    भव पूर्णांक instr_गिनती() स्थिर { वापस 0; }
    // Iff node is identical to another node
    भव bool isSame(स्थिर Ast_गाँठ* samep) स्थिर {
        वापस type() == samep->type() && same_गाँठ(samep);
    }
    // Iff has a data type; dtype() must be non-null
    भव bool hasDType() स्थिर VL_MT_SAFE { वापस false; }
    // Iff has a non-null childDTypep(), as generic node function
    भव Ast_गाँठ_DType* getChildDTypep() स्थिर { वापस nullptr; }
    // Iff has a non-null child2DTypep(), as generic node function
    भव Ast_गाँठ_DType* getChild2DTypep() स्थिर { वापस nullptr; }
    // Another AstNode* may have a pointer into this node, other then normal front/back/etc.
    भव bool maybePointedTo() स्थिर VL_MT_SAFE { वापस false; }
    // Don't reclaim this node in V3Dead
    भव bool undead() स्थिर { वापस false; }
    // Check if node is consistent, return nullptr if ok, else reason string
    भव स्थिर अक्षर* broken() स्थिर { वापस nullptr; }
    // Generated by 'astgen'. Calls 'broken()', which can be used to add extra checks
    भव स्थिर अक्षर* brokenGen() स्थिर = 0;  // Generated by 'astgen'
    // If do a this->replaceNode(newp), would cause a broken()
    bool wouldBreak(स्थिर Ast_गाँठ* स्थिर newp) स्थिर { वापस backp()->wouldBreakGen(this, newp); }

    // INVOKERS
    भव व्योम accept(VNVisitorConst& v) = 0;

रक्षित:
    // All VNVisitor related functions are called as methods off the visitor
    मित्र class VNVisitor;
    मित्र class VNVisitorConst;
    // Use instead VNVisitor::iterateChildren
    व्योम iterateChildren(VNVisitor& v);
    // Use instead VNVisitor::iterateChildrenBackwardsConst
    व्योम iterateChildrenBackwardsConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateChildrenConst
    व्योम iterateChildrenConst(VNVisitorConst& v);
    // Use instead VNVisitor::iterateAndNextNull
    व्योम iterate_And_अग्र(VNVisitor& v);
    // Use instead VNVisitor::iterateAndNextConstNull
    व्योम iterate_And_अग्र_Const(VNVisitorConst& v);
    // Use instead VNVisitor::iterateSubtreeReturnEdits
    Ast_गाँठ* iterateSubtreeReturnEdits(VNVisitor& v);

    अटल व्योम dump_Json_अंक(std::ostream& os, स्थिर std::string& name, int64_t val);
    अटल व्योम dumpJsonBoolIf(std::ostream& os, स्थिर std::string& name, bool val);
    अटल व्योम dumpJsonStr(std::ostream& os, स्थिर std::string& name, स्थिर std::string& val);
    अटल व्योम dumpJsonPtr(std::ostream& os, स्थिर std::string& name, स्थिर Ast_गाँठ* स्थिर valp);

रक्षित:
    व्योम iterateListBackwardsConst(VNVisitorConst& v);

    // For internal use only.
    // Note: specializations for particular node types are provided by 'astgen'
    ढाँचा <typename T>
    अंतरभूत अटल bool privateTypeTest(स्थिर Ast_गाँठ* nodep);

    // For internal use only.
    ढाँचा <typename T_TargetType, typename T_DeclType>
    constexpr अटल bool uselessCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        वापस std::is_base_of<T_TargetType, NonCV>::मूल्य;
    }  // LCOV_EXCL_STOP

    // For internal use only.
    ढाँचा <typename T_TargetType, typename T_DeclType>
    constexpr अटल bool impossibleCast() VL_PURE {  // LCOV_EXCL_START
        using NonRef = typename std::remove_reference<T_DeclType>::type;
        using NonPtr = typename std::remove_pointer<NonRef>::type;
        using NonCV = typename std::remove_cv<NonPtr>::type;
        वापस !std::is_base_of<NonCV, T_TargetType>::मूल्य;
    }  // LCOV_EXCL_STOP

खुला:
    // For use via the VN_IS macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाँठ>
    अटल bool is(स्थिर T_गाँठ* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य, "VN_IS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_IS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_IS, node cannot be this type.");
        वापस nodep && privateTypeTest<T_TargetType>(nodep);
    }

    // For use via the VN_CAST macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाँठ>
    अटल T_TargetType* cast(T_गाँठ* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        वापस nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<T_TargetType*>(nodep)
                   : nullptr;
    }
    ढाँचा <typename T_TargetType, typename T_गाँठ>
    अटल स्थिर T_TargetType* cast(स्थिर T_गाँठ* nodep) VL_MT_SAFE {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य, "VN_CAST called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_CAST, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_CAST, node cannot be this type.");
        वापस nodep && privateTypeTest<T_TargetType>(nodep)
                   ? reinterpret_cast<स्थिर T_TargetType*>(nodep)
                   : nullptr;
    }

    // For use via the VN_AS macro only, or in templated code
    ढाँचा <typename T_TargetType, typename T_गाँठ>
    अटल T_TargetType* as(T_गाँठ* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        वापस unsafePrivateAs<T_TargetType, T_गाँठ>(nodep);
    }
    ढाँचा <typename T_TargetType, typename T_गाँठ>
    अटल स्थिर T_TargetType* as(स्थिर T_गाँठ* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य, "VN_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_AS, node cannot be this type.");
        UASSERT_OBJ(!nodep || privateTypeTest<T_TargetType>(nodep), nodep,
                    "AstNode is not of expected type, but instead has type '" << nodep->typeName()
                                                                              << "'");
        वापस unsafePrivateAs<T_TargetType, T_गाँठ>(nodep);
    }

    // For use via privateAs or the VN_DBG_AS macro only
    ढाँचा <typename T_TargetType, typename T_गाँठ>
    अटल T_TargetType* unsafePrivateAs(T_गाँठ* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        वापस reinterpret_cast<T_TargetType*>(nodep);
    }
    ढाँचा <typename T_TargetType, typename T_गाँठ>
    अटल स्थिर T_TargetType* unsafePrivateAs(स्थिर T_गाँठ* nodep) VL_PURE {
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य, "VN_DBG_AS called on non-AstNode");
        static_assert(!uselessCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_DBG_AS, node known to have target type.");
        static_assert(!impossibleCast<T_TargetType, T_गाँठ>(),
                      "Unnecessary VN_DBG_AS, node cannot be this type.");
        वापस reinterpret_cast<स्थिर T_TargetType*>(nodep);
    }

    // Predicate that returns true if the given 'nodep' might have a descendant of type 'T_Node'.
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ढाँचा <typename T_गाँठ>
    अटल bool mayBeUnder(स्थिर Ast_गाँठ* nodep) {
        static_assert(!std::is_const<T_गाँठ>::मूल्य,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        वापस true;
    }

    // Predicate that is true for node subtypes 'T_Node' that do not have any children
    // This is conservative and is used to speed up traversals.
    // Note: specializations for particular node types are provided below
    ढाँचा <typename T_गाँठ>
    अटल constexpr bool isLeaf() {
        static_assert(!std::is_const<T_गाँठ>::मूल्य,
                      "Type parameter 'T_Node' should not be const qualified");
        static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "Type parameter 'T_Node' must be a subtype of AstNode");
        वापस false;
    }

निजी:
    // Using std::conditional for const correctness in the public 'foreach' functions
    ढाँचा <typename T_Arg>
    using Const_Correct_Ast_गाँठ =
        typename std::conditional<std::is_const<T_Arg>::मूल्य, स्थिर Ast_गाँठ, Ast_गाँठ>::type;

    ढाँचा <typename T_Arg, typename T_Callable>
    अंतरभूत अटल व्योम foreachImpl(Const_Correct_Ast_गाँठ<T_Arg>* nodep, स्थिर T_Callable& f,
                                   bool visit_अग्र);

    ढाँचा <typename T_Arg, bool N_Default, typename T_Callable>
    अंतरभूत अटल bool predicateImpl(Const_Correct_Ast_गाँठ<T_Arg>* nodep, स्थिर T_Callable& p,
                                     bool visit_अग्र);

खुला:
    // Given a callable 'f' that takes a single argument of some AstNode subtype 'T_Node', traverse
    // the tree rooted at this node, and call 'f' in pre-order on each node that is of type
    // 'T_Node'. The node passed to the callable 'f' can be removed or replaced, but other editing
    // of the iterated tree is not safe. Prefer 'foreach' over simple VNVisitor that only needs to
    // handle a single (or a few) node types, as it's easier to write, but more importantly, the
    // dispatch to the callable in 'foreach' should be completely predictable by branch target
    // caches in modern CPUs, while it is basically unpredictable for VNVisitor.
    ढाँचा <typename T_Callable>
    व्योम foreach(T_Callable&& f) {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_गाँठ*>::मूल्य
                          && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_गाँठ>(this, f, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    व्योम foreach(T_Callable&& f) स्थिर {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, स्थिर T_गाँठ*>::मूल्य
                && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<स्थिर T_गाँठ>(this, f, /* visitNext: */ false);
    }
    // Same as 'foreach' but also traverses 'this->nextp()' transitively
    ढाँचा <typename T_Callable>
    व्योम foreach_And_अग्र(T_Callable&& f) {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable<T_Callable, T_गाँठ*>::मूल्य
                          && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "T_Callable 'f' must have a signature compatible with 'void(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<T_गाँठ>(this, f, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    व्योम foreach_And_अग्र(T_Callable&& f) स्थिर {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, स्थिर T_गाँठ*>::मूल्य
                && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
            "T_Callable 'f' must have a signature compatible with 'void(const T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        foreachImpl<स्थिर T_गाँठ>(this, f, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if there exists a node of type 'T_Node' in the tree rooted at this node,
    // that satisfies the predicate 'p'. Returns false if no node of type 'T_Node' is present.
    // Traversal is performed in some arbitrary order and is terminated as soon as the result can
    // be determined.
    ढाँचा <typename T_Callable>
    bool exists(T_Callable&& p) {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_गाँठ*>::मूल्य
                          && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        वापस predicateImpl<T_गाँठ, /* N_Default: */ false>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool exists(T_Callable&& p) स्थिर {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, स्थिर T_गाँठ*>::मूल्य
                          && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        वापस predicateImpl<स्थिर T_गाँठ, /* N_Default: */ false>(this, p,
                                                                   /* visitNext: */ false);
    }
    // Same as 'exists' but also traverses 'this->nextp()' transitively
    ढाँचा <typename T_Callable>
    bool exists_And_अग्र(T_Callable&& p) {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, स्थिर T_गाँठ*>::मूल्य
                          && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        वापस predicateImpl<T_गाँठ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool exists_And_अग्र(T_Callable&& p) स्थिर {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, स्थिर T_गाँठ*>::मूल्य
                          && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        वापस predicateImpl<स्थिर T_गाँठ, /* N_Default: */ false>(this, p, /* visitNext: */ true);
    }

    // Given a predicate 'p' that takes a single argument of some AstNode subtype 'T_Node', return
    // true if and only if all nodes of type 'T_Node' in the tree rooted at this node satisfy the
    // predicate 'p'. Returns true if no node of type 'T_Node' is present. Traversal is performed
    // in some arbitrary order and is terminated as soon as the result can be determined.
    ढाँचा <typename T_Callable>
    bool forall(T_Callable&& p) {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, T_गाँठ*>::मूल्य
                          && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        वापस predicateImpl<T_गाँठ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }
    // Same as above, but for 'const' nodes
    ढाँचा <typename T_Callable>
    bool forall(T_Callable&& p) स्थिर {
        using T_गाँठ = typename FunctionArgNoPointerNoCV<T_Callable, 0>::type;
        static_assert(vlstd::is_invocable_r<bool, T_Callable, स्थिर T_गाँठ*>::मूल्य
                          && std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                      "Predicate 'p' must have a signature compatible with 'bool(const T_Node*)', "
                      "with 'T_Node' being a subtype of 'AstNode'");
        वापस predicateImpl<स्थिर T_गाँठ, /* N_Default: */ true>(this, p, /* visitNext: */ false);
    }

    पूर्णांक गाँठ_गिनती() स्थिर {
        // TODO: this should really return size_t, but need to fix use sites
        पूर्णांक गिनती = 0;
        this->foreach([&गिनती](स्थिर Ast_गाँठ*) { ++गिनती; });
        वापस गिनती;
    }

    // Return true if and only if the tree rooted at this node has more than 'limit' nodes.
    // Traversal terminates as soon as the result is known, so unlike comparing 'nodeCount',
    // this is cheap on a large tree.
    bool isLargerThan(पूर्णांक limit) स्थिर {
        पूर्णांक गिनती = 0;
        वापस this->exists([&गिनती, limit](स्थिर Ast_गाँठ*) { वापस ++गिनती > limit; });
    }
};

// Forward declarations of specializations defined in V3Ast.cpp
ढाँचा <>
Ast_गाँठ* Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(Ast_गाँठ* nodep, Ast_गाँठ* newp);

// Inline method implementations
Ast_गाँठ* Ast_गाँठ::add_अग्र(Ast_गाँठ* newp) { वापस add_अग्र(this, newp); }

// Specializations of privateTypeTest
#समावेश "V3Ast__gen_type_tests.h"  // From ./astgen

// Specializations of AstNode::mayBeUnder
ढाँचा <>
अंतरभूत bool Ast_गाँठ::mayBeUnder<AstCell>(स्थिर Ast_गाँठ* nodep) {
    वापस !VN_IS(nodep, गाँठ_Stmt) && !VN_IS(nodep, गाँठ_व्यंजक);
}
ढाँचा <>
अंतरभूत bool Ast_गाँठ::mayBeUnder<Ast_गाँठ_Assign>(स्थिर Ast_गाँठ* nodep) {
    वापस !VN_IS(nodep, गाँठ_व्यंजक);
}
ढाँचा <>
अंतरभूत bool Ast_गाँठ::mayBeUnder<AstVarScope>(स्थिर Ast_गाँठ* nodep) {
    अगर (VN_IS(nodep, VarScope)) वापस false;  // Should not nest
    अगर (VN_IS(nodep, Var)) वापस false;
    अगर (VN_IS(nodep, Active)) वापस false;
    अगर (VN_IS(nodep, गाँठ_Stmt)) वापस false;
    अगर (VN_IS(nodep, गाँठ_व्यंजक)) वापस false;
    वापस true;
}
ढाँचा <>
अंतरभूत bool Ast_गाँठ::mayBeUnder<AstExecGraph>(स्थिर Ast_गाँठ* nodep) {
    अगर (VN_IS(nodep, ExecGraph)) वापस false;  // Should not nest
    अगर (VN_IS(nodep, गाँठ_Stmt)) वापस false;  // Should be directly under CFunc
    वापस true;
}
ढाँचा <>
अंतरभूत bool Ast_गाँठ::mayBeUnder<AstActive>(स्थिर Ast_गाँठ* nodep) {
    वापस !VN_IS(nodep, Active);  // AstActives do not nest
}
ढाँचा <>
अंतरभूत bool Ast_गाँठ::mayBeUnder<AstScope>(स्थिर Ast_गाँठ* nodep) {
    वापस !VN_IS(nodep, Scope);  // AstScopes do not nest
}
ढाँचा <>
अंतरभूत bool Ast_गाँठ::mayBeUnder<Ast_Sen_वृक्ष>(स्थिर Ast_गाँठ* nodep) {
    वापस !VN_IS(nodep, Sen_वृक्ष);  // AstSenTree do not nest
}

// Specializations of AstNode::isLeaf
ढाँचा <>
constexpr bool Ast_गाँठ::isLeaf<Ast_गाँठ_Var_Ref>() {
    वापस true;
}
ढाँचा <>
constexpr bool Ast_गाँठ::isLeaf<AstVarRef>() {
    वापस true;
}
ढाँचा <>
constexpr bool Ast_गाँठ::isLeaf<AstVarXRef>() {
    वापस true;
}

// foreach implementation
ढाँचा <typename T_Arg, typename T_Callable>
व्योम Ast_गाँठ::foreachImpl(Const_Correct_Ast_गाँठ<T_Arg>* nodep, स्थिर T_Callable& f, bool visit_अग्र) {
    // Pre-order traversal implemented directly (without recursion) for speed reasons. The very
    // first iteration (the one that operates on the input nodep) is special, as we might or
    // might not need to enqueue nodep->nextp() depending on VisitNext, while in all other
    // iterations, we do want to enqueue nodep->nextp(). Duplicating code (via
    // 'foreachImplVisit') for the initial iteration here to avoid an extra branch in the loop

    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using गाँठ_2 = Const_Correct_Ast_गाँठ<T_Arg>;

    // We prefetch this far into the stack
    constexpr पूर्णांक PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr माप_प्रकार MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr माप_प्रकार Initial_ढेर_आकार = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    माप_प्रकार ढेर_आकार = Initial_ढेर_आकार;
    गाँठ_2* in_पंक्ति_ढेर[Initial_ढेर_आकार];  // In-line initial stack storage
    std::unique_ptr<गाँठ_2*[]> out_Of_पंक्ति_Stackp{nullptr};  // In case we need more, we will allocate
    गाँठ_2** basep = in_पंक्ति_ढेर + PrefetchDistance;  // Pointer to base of stack
    गाँठ_2** topp = basep;  // Pointer to top of stack
    गाँठ_2** limp = in_पंक्ति_ढेर + ढेर_आकार - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    क्रम (पूर्णांक i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal
    स्थिर स्वतः visit = [&](गाँठ_2* currp) {
        // Type test this node
        अगर (Ast_गाँठ::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            f(static_cast<T_Arg*>(currp));
            // Short circuit if iterating leaf nodes
            अगर VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) वापस;
        }

        // Enqueue children for traversal, unless futile
        अगर (mayBeUnder<T_Arg_NonConst>(currp)) {
            अगर (Ast_गाँठ* स्थिर op4p = currp->op4p()) *topp++ = op4p;
            अगर (Ast_गाँठ* स्थिर op3p = currp->op3p()) *topp++ = op3p;
            अगर (Ast_गाँठ* स्थिर op2p = currp->op2p()) *topp++ = op2p;
            अगर (Ast_गाँठ* स्थिर op1p = currp->op1p()) *topp++ = op1p;
        }
    };

    // Enqueue the next of the root node, if required
    अगर (visit_अग्र && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    visit(nodep);

    // Visit the rest of the tree
    जबतक (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        गाँठ_2* स्थिर headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        अगर (VL_UNLIKELY(topp >= limp)) {
            स्थिर सूचक_भेद_प्रकार occupancy = topp - basep;
            {
                स्थिर माप_प्रकार new_ढेर_आकार = ढेर_आकार * 2;
                गाँठ_2** स्थिर newStackp = new गाँठ_2*[new_ढेर_आकार];
                std::स_नकल(newStackp, basep - PrefetchDistance,
                            माप(गाँठ_2**) * (occupancy + PrefetchDistance));
                ढेर_आकार = new_ढेर_आकार;
                out_Of_पंक्ति_Stackp.reset(newStackp);
            }
            basep = out_Of_पंक्ति_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_पंक्ति_Stackp.get() + ढेर_आकार - MaxPushesPerIteration;
        }

        // Enqueue the next node
        अगर (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        visit(headp);
    }
}

// predicate implementation
ढाँचा <typename T_Arg, bool N_Default, typename T_Callable>
bool Ast_गाँठ::predicateImpl(Const_Correct_Ast_गाँठ<T_Arg>* nodep, स्थिर T_Callable& p,
                            bool visit_अग्र) {
    // Implementation similar to foreach, but abort traversal as soon as result is determined
    using T_Arg_NonConst = typename std::remove_const<T_Arg>::type;
    using गाँठ_2 = Const_Correct_Ast_गाँठ<T_Arg>;

    // We prefetch this far into the stack
    constexpr पूर्णांक PrefetchDistance = 2;
    // We push max 5 items per iteration
    constexpr माप_प्रकार MaxPushesPerIteration = 5;
    // Initial stack sie
    constexpr माप_प्रकार Initial_ढेर_आकार = 32;

    // Traversal stack. Because we often iterate over small trees, use an in-line initial stack
    माप_प्रकार ढेर_आकार = Initial_ढेर_आकार;
    गाँठ_2* in_पंक्ति_ढेर[Initial_ढेर_आकार];  // In-line initial stack storage
    std::unique_ptr<गाँठ_2*[]> out_Of_पंक्ति_Stackp{nullptr};  // In case we need more, we will allocate
    गाँठ_2** basep = in_पंक्ति_ढेर + PrefetchDistance;  // Pointer to base of stack
    गाँठ_2** topp = basep;  // Pointer to top of stack
    गाँठ_2** limp = in_पंक्ति_ढेर + ढेर_आकार - MaxPushesPerIteration;  // Pointer to grow limit

    // We want some non-null pointers at the beginning. These will be prefetched, but not
    // visited, so the root node will suffice. This eliminates needing branches in the loop.
    क्रम (पूर्णांक i = -PrefetchDistance; i; ++i) basep[i] = nodep;

    // Visit given node, enqueue children for traversal, return true if result determined.
    स्थिर स्वतः visit = [&](गाँठ_2* currp) {
        // Type test this node
        अगर (Ast_गाँठ::privateTypeTest<T_Arg_NonConst>(currp)) {
            // Call the client function
            अगर (p(static_cast<T_Arg*>(currp)) != N_Default) वापस true;
            // Short circuit if iterating leaf nodes
            अगर VL_CONSTEXPR_CXX17 (isLeaf<T_Arg_NonConst>()) वापस false;
        }

        // Enqueue children for traversal, unless futile
        अगर (mayBeUnder<T_Arg_NonConst>(currp)) {
            अगर (Ast_गाँठ* स्थिर op4p = currp->op4p()) *topp++ = op4p;
            अगर (Ast_गाँठ* स्थिर op3p = currp->op3p()) *topp++ = op3p;
            अगर (Ast_गाँठ* स्थिर op2p = currp->op2p()) *topp++ = op2p;
            अगर (Ast_गाँठ* स्थिर op1p = currp->op1p()) *topp++ = op1p;
        }

        वापस false;
    };

    // Enqueue the next of the root node, if required
    अगर (visit_अग्र && nodep->nextp()) *topp++ = nodep->nextp();

    // Visit the root node
    अगर (visit(nodep)) वापस !N_Default;

    // Visit the rest of the tree
    जबतक (VL_LIKELY(topp > basep)) {
        // Pop next node in the traversal
        गाँठ_2* स्थिर headp = *--topp;

        // Prefetch in case we are ascending the tree
        ASTNODE_PREFETCH_NON_NULL(topp[-PrefetchDistance]);

        // Ensure we have stack space for nextp and the 4 children
        अगर (VL_UNLIKELY(topp >= limp)) {
            स्थिर सूचक_भेद_प्रकार occupancy = topp - basep;
            {
                स्थिर माप_प्रकार new_ढेर_आकार = ढेर_आकार * 2;
                गाँठ_2** स्थिर newStackp = new गाँठ_2*[new_ढेर_आकार];
                std::स_नकल(newStackp, basep - PrefetchDistance,
                            माप(गाँठ_2**) * (occupancy + PrefetchDistance));
                ढेर_आकार = new_ढेर_आकार;
                out_Of_पंक्ति_Stackp.reset(newStackp);
            }
            basep = out_Of_पंक्ति_Stackp.get() + PrefetchDistance;
            topp = basep + occupancy;
            limp = out_Of_पंक्ति_Stackp.get() + ढेर_आकार - MaxPushesPerIteration;
        }

        // Enqueue the next node
        अगर (headp->nextp()) *topp++ = headp->nextp();

        // Visit the head node
        अगर (visit(headp)) वापस !N_Default;
    }

    वापस N_Default;
}

अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर Ast_गाँठ* rhs) {
    अगर (!rhs) {  // LCOV_EXCL_LINE
        os << "nullptr";  // LCOV_EXCL_LINE
    } अन्यथा अगर (VN_DELETED(rhs)) {  // LCOV_EXCL_LINE
        os << "%E-0x1/deleted!";  // LCOV_EXCL_LINE
    } अन्यथा {
        rhs->dump(os);
    }
    वापस os;
}
व्योम VNRelinker::relink(Ast_गाँठ* newp) { newp->Ast_गाँठ::relink(this); }

//######################################################################

// VNRef is std::reference_wrapper that can only hold AstNode subtypes
ढाँचा <typename T_गाँठ>
class VNRef final : खुला std::reference_wrapper<T_गाँठ> {
    static_assert(std::is_base_of<Ast_गाँठ, T_गाँठ>::मूल्य,
                  "Type parameter 'T_Node' must be a subtype of AstNode");

खुला:
    ढाँचा <typename U>
    // cppcheck-suppress noExplicitConstructor
    VNRef(U&& x)
        : std::reference_wrapper<T_गाँठ>{x} {}
    // cppcheck-suppress noExplicitConstructor
    VNRef(स्थिर std::reference_wrapper<T_गाँठ>& other)
        : std::reference_wrapper<T_गाँठ>{other} {}
};

static_assert(माप(VNRef<Ast_गाँठ>) == माप(std::reference_wrapper<Ast_गाँठ>),
              "VNRef should not contain extra members");

// Specializations of std::hash and std::equal_to for VNRef. This in turn
// enables us to use for example std::unordered_set<VNRef<AstNode>> for
// sets using equality (AstNode::sameTree) rather than identity comparisons,
// without having to copy nodes into the collections.

// Forward declaration to avoid including V3Hasher.h which needs V3Ast.h (this file).
माप_प्रकार V3HasherUncachedHash(स्थिर Ast_गाँठ&);

// Specialization of std::hash for VNRef
ढाँचा <typename T_गाँठ>
काष्ठा std::hash<VNRef<T_गाँठ>> final {
    माप_प्रकार चालक()(VNRef<T_गाँठ> r) स्थिर { वापस V3HasherUncachedHash(r); }
};

// Specialization of std::equal_to for VNRef
ढाँचा <typename T_गाँठ>
काष्ठा std::equal_to<VNRef<T_गाँठ>> final {
    माप_प्रकार चालक()(VNRef<T_गाँठ> ra, VNRef<T_गाँठ> rb) स्थिर {
        वापस ra.get().same_वृक्ष(&(rb.get()));
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
    std::unordered_map<स्थिर Ast_गाँठ*, Ast_गाँठ*> m_outerps;
    // The parameters of each design unit that its name holds
    std::unordered_map<स्थिर Ast_गाँठ*, std::vector<Ast_गाँठ*>> m_paramps;
    // Prefix of the names of the types each scope declares, once final
    mutable std::unordered_map<स्थिर Ast_गाँठ*, std::string> m_prefixes;
    स्थिर VDTypeNameScopes* स्थिर m_prevp;  // Index in use before this one
    अटल स्थिर VDTypeNameScopes* s_currentp;  // Index in use, or nullptr if none

    // METHODS
    व्योम सूचकांक_2(Ast_गाँठ* nodep, Ast_गाँठ* scopep);
    VL_UNCOPYABLE(VDTypeNameScopes);
    VL_UNMOVABLE(VDTypeNameScopes);

खुला:
    // CONSTRUCTORS
    // Index the netlist, in use until destroyed
    explicit VDTypeNameScopes(AstNetlist* nodep);
    ~VDTypeNameScopes();

    // METHODS
    // The index in use, or nullptr if none
    अटल स्थिर VDTypeNameScopes* currentp() { वापस s_currentp; }
    // Whether 'nodep' is indexed, and if so 'outerpr', the generate block or design unit
    // declaring it, or nullptr if none
    bool outerp(स्थिर Ast_गाँठ* nodep, Ast_गाँठ*& outerpr) स्थिर;
    // The parameters of design unit 'modp' that its name holds, or nullptr if not indexed
    स्थिर std::vector<Ast_गाँठ*>* paramsp(स्थिर Ast_गाँठ* modp) स्थिर;
    // The prefix kept for scope 'scopep', or nullptr if none
    स्थिर std::string* prefixp(स्थिर Ast_गाँठ* scopep) स्थिर;
    // Keep 'prefix', final, for scope 'scopep'
    व्योम prefix(स्थिर Ast_गाँठ* scopep, स्थिर std::string& prefix) स्थिर;
};

//######################################################################
// Inline VNVisitor METHODS

व्योम VNVisitorConst::iterateConst(Ast_गाँठ* nodep) { nodep->accept(*this); }
व्योम VNVisitorConst::iterateConstNull(Ast_गाँठ* nodep) {
    अगर (VL_LIKELY(nodep)) nodep->accept(*this);
}
व्योम VNVisitorConst::iterateChildrenConst(Ast_गाँठ* nodep) { nodep->iterateChildrenConst(*this); }
व्योम VNVisitorConst::iterateChildrenBackwardsConst(Ast_गाँठ* nodep) {
    nodep->iterateChildrenBackwardsConst(*this);
}
व्योम VNVisitorConst::iterate_And_अग्र_Const_Null(Ast_गाँठ* nodep) {
    अगर (VL_LIKELY(nodep)) nodep->iterate_And_अग्र_Const(*this);
}

व्योम VNVisitor::iterate(Ast_गाँठ* nodep) { nodep->accept(*this); }
व्योम VNVisitor::iterateNull(Ast_गाँठ* nodep) {
    अगर (VL_LIKELY(nodep)) nodep->accept(*this);
}
व्योम VNVisitor::iterateChildren(Ast_गाँठ* nodep) { nodep->iterateChildren(*this); }
व्योम VNVisitor::iterate_And_अग्र_Null(Ast_गाँठ* nodep) {
    अगर (VL_LIKELY(nodep)) nodep->iterate_And_अग्र(*this);
}
Ast_गाँठ* VNVisitor::iterateSubtreeReturnEdits(Ast_गाँठ* nodep) {
    वापस nodep->iterateSubtreeReturnEdits(*this);
}

// Include macros generated by 'astgen'. These include ASTGEN_MEMBERS_Ast<Node>
// for each AstNode sub-type, and ASTGEN_SUPER_<Node> for concrete final
// AstNode sub-types. The generated members include boilerplate methods related
// to cloning, visitor dispatch, and other functionality. ASTGEN_SUPER_<Node>
// is the necessary constructor invocation for concrete AstNode sub-types
// that passes the generated type-id numbers all the way back to AstNode.
// For precise details please read the generated macros.
#समावेश "V3Ast__gen_macros.h"

// AstNode subclasses - dependency order matters, so fmt off
// clang-format off
#समावेश "V3AstNodeDType.h"
#समावेश "V3AstNodeOther.h"
#समावेश "V3AstNodeExpr.h"
#समावेश "V3AstNodeStmt.h"
// clang-format on

// Inline function definitions need to go last
#समावेश "V3AstInlines.h"
व्योम dump_गाँठ_List_Json(std::ostream& os, स्थिर Ast_गाँठ* nodep, स्थिर std::string& listName,
                      स्थिर string& indent);

#पूर्ण_अगर  // Guard
