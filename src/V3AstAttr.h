// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode attributes and sub-types
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
//
// This files contains small classes that contain attributes or enum classes
// used by V3AstNode*.h.  Classes that are part of the base AST structure
// belong in V3Ast.h instead.
//
// The classes in this file should kept in mostly sorted order, with
// a few earlier-definition dependent exception at the end.
//
//*************************************************************************

#ifndef VERILATOR_V3ASTATTR_H_
#define VERILATOR_V3ASTATTR_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#include "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
public:
    // IEEE 1800-2023 16.12.14 property abort operators.
    ga_nanaa en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    ga_nanaa en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        ပြန်ပေး names[m_e];
    }
    bool isAccept() const { ပြန်ပေး m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
public:
    ga_nanaa en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"RD", "WR", "RW", "--"};
        ပြန်ပေး names[m_e];
    }
    const var_na* arrow() const {
        static constexpr const var_na* const names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        ပြန်ပေး names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    VAccess invert() const {
        ပြန်ပေး (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() const { ပြန်ပေး m_e == READ; }  // False with READWRITE
    bool isWriteOnly() const { ပြန်ပေး m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() const { ပြန်ပေး m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() const { ပြန်ပေး m_e == WRITE || m_e == READWRITE; }
    bool isRW() const { ပြန်ပေး m_e == READWRITE; }
};
constexpr bool operator==(const VAccess& lhs, const VAccess& rhs) { ပြန်ပေး lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VAccess& lhs, VAccess::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, const VAccess& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VAccess& rhs) { ပြန်ပေး os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
public:
    ga_nanaa en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    ga_nanaa en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VAlwaysKwd& lhs, const VAlwaysKwd& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, const VAlwaysKwd& rhs) { ပြန်ပေး lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
public:
    // IEEE 1800-2023 Table 20-5
    ga_nanaa en : uint8_t {
        _TO_BE_EVALUATED = 0,
        LOCK = 1,
        UNLOCK = 2,
        ON = 3,
        OFF = 4,
        KILL = 5,
        PASS_ON = 6,
        PASS_OFF = 7,
        FAIL_ON = 8,
        FAIL_OFF = 9,
        NONVACUOUS_ON = 10,
        VACUOUS_OFF = 11
    };
    ga_nanaa en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        // IEEE 1800-2023 20.11
        static constexpr const var_na* const names[] = {"",
                                                      "",
                                                      "",
                                                      "$asserton",
                                                      "$assertoff",
                                                      "$assertkill",
                                                      "$assertpasson",
                                                      "$assertpassoff",
                                                      "$assertfailon",
                                                      "$assertfailoff",
                                                      "$assertnonvacuouson",
                                                      "$assertvacuousoff"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VAssertCtlType& lhs, const VAssertCtlType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    ပြန်ပေး lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, const VAssertCtlType& rhs) {
    ပြန်ပေး lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
public:
    // IEEE 1800-2023 Table 20-7
    ga_nanaa en : uint8_t {
        INTERNAL = 0,  // Non IEEE type, for directives to be evaluated from expression.
        ASSERT = (1 << 0),
        COVER = (1 << 1),
        ASSUME = (1 << 2),
        VIOLATION_CASE = (1 << 3),  // Non IEEE type, for case constructs
                                    // with unique, unique0 or priority pragmas.
        VIOLATION_IF = (1 << 4),  // Non IEEE type, for if constructs
                                  // with unique, unique0 or priority pragmas.
        INTRINSIC = (1 << 5),  // Non IEEE type, for intrinsic assertions.
        RESTRICT = (1 << 6),  // Non IEEE type, for ignored restrict assertions.
    };
    ga_nanaa en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() const {
        std::stringstream types;
        အကယ်၍ (m_e == INTERNAL)
            types << "INTERNAL ";
        မဟုတ်ပါက {
            အကယ်၍ (m_e & ASSERT) types << "ASSERT ";
            အကယ်၍ (m_e & COVER) types << "COVER ";
            အကယ်၍ (m_e & ASSUME) types << "ASSUME ";
            အကယ်၍ (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            အကယ်၍ (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            အကယ်၍ (m_e & INTRINSIC) types << "INTRINSIC ";
            အကယ်၍ (m_e & RESTRICT) types << "RESTRICT ";
        }
        const string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        ပြန်ပေး str.substr(0, str.size() - 1);
    }
    constexpr operator en() const { ပြန်ပေး m_e; }
};
constexpr bool operator==(const VAssertDirectiveType& lhs, const VAssertDirectiveType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    ပြန်ပေး lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, const VAssertDirectiveType& rhs) {
    ပြန်ပေး lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    ပြန်ပေး VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
public:
    // IEEE 1800-2023 Table 20-6
    ga_nanaa en : uint8_t {
        INTERNAL = 0,  // Non IEEE type, for assertions that should not be controlled.
        CONCURRENT = (1 << 0),
        SIMPLE_IMMEDIATE = (1 << 1),
        OBSERVED_DEFERRED_IMMEDIATE = (1 << 2),
        FINAL_DEFERRED_IMMEDIATE = (1 << 3),
        EXPECT = (1 << 4),
        UNIQUE = (1 << 5),
        UNIQUE0 = (1 << 6),
        PRIORITY = (1 << 7),
    };
    ga_nanaa en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) const { ပြန်ပေး m_e & other.m_e; }
    string ascii() const {
        std::stringstream types;
        အကယ်၍ (m_e == INTERNAL)
            types << "INTERNAL ";
        မဟုတ်ပါက {
            အကယ်၍ (m_e & CONCURRENT) types << "CONCURRENT ";
            အကယ်၍ (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            အကယ်၍ (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            အကယ်၍ (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            အကယ်၍ (m_e & EXPECT) types << "EXPECT ";
            အကယ်၍ (m_e & UNIQUE) types << "UNIQUE ";
            အကယ်၍ (m_e & UNIQUE0) types << "UNIQUE0 ";
            အကယ်၍ (m_e & PRIORITY) types << "PRIORITY ";
        }
        const string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        ပြန်ပေး str.substr(0, str.size() - 1);
    }
    constexpr operator en() const { ပြန်ပေး m_e; }
};
constexpr bool operator==(const VAssertType& lhs, const VAssertType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAssertType& lhs, VAssertType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, const VAssertType& rhs) { ပြန်ပေး lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    ပြန်ပေး VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
public:
    ga_nanaa en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    ga_nanaa en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"default", "weak", "strong"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VPropStrength& lhs, const VPropStrength& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VPropStrength& lhs, VPropStrength::en rhs) {
    ပြန်ပေး lhs.m_e == rhs;
}
constexpr bool operator!=(const VPropStrength& lhs, VPropStrength::en rhs) {
    ပြန်ပေး lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
public:
    // clang-format off
    ga_nanaa en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_NUMBER,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_HIGH,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_LEFT,                       // V3Width processes
        DIM_LOW,                        // V3Width processes
        DIM_RIGHT,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_NUM,                       // V3Width processes
        ENUM_NEXT,                      // V3Width processes
        ENUM_PREV,                      // V3Width processes
        ENUM_NAME,                      // V3Width processes
        ENUM_VALID,                     // V3Width processes
        //
        FUNC_ARG_PROTO,                 // V3WidthCommit processes
        FUNC_RETURN_PROTO,              // V3WidthCommit processes
        //
        TYPEID,                         // V3Width processes
        TYPENAME,                       // V3Width processes
        //
        VAR_BASE,                       // V3LinkResolve creates for AstPreSel, V3LinkParam removes
        VAR_FORCEABLE,                  // V3LinkParse moves to AstVar::isForceable
        VAR_FSM_ARC_INCLUDE_COND,       // V3LinkParse moves to AstVar::attrFsmArcInclCond
        VAR_FSM_ARC_INCLUDE_COND_AUTO,  // Flavors of above option - setting expand mode per FSM
        VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND,
        VAR_FSM_ARC_INCLUDE_COND_FULL,
        VAR_FSM_STATE,                  // V3LinkParse moves to AstVar::attrFsmState
        VAR_FSM_STATE_AUTO,             // Flavors of above option - setting expand mode per FSM
        VAR_FSM_STATE_AUTO_EXPAND,
        VAR_FSM_STATE_FULL,
        VAR_PORT_DTYPE,                 // V3LinkDot for V3Width to check port dtype
        VAR_PUBLIC,                     // V3LinkParse moves to AstVar::sigPublic
        VAR_PUBLIC_FLAT,                // V3LinkParse moves to AstVar::sigPublic
        VAR_PUBLIC_FLAT_RD,             // V3LinkParse moves to AstVar::sigPublic
        VAR_PUBLIC_FLAT_RW,             // V3LinkParse moves to AstVar::sigPublic
        VAR_SC_BIGUINT,                 // V3LinkParse moves to AstVar::attrScBigUint
        VAR_SC_BV,                      // V3LinkParse moves to AstVar::attrScBv
        VAR_SFORMAT,                    // V3LinkParse moves to AstVar::attrSFormat
        VAR_SPLIT_VAR                   // V3LinkParse moves to AstVar::attrSplitVar
    };
    // clang-format on
    ga_nanaa en m_e;
    const var_na* ascii() const {
        // clang-format off
        static constexpr const var_na* const names[] = {
            "%E-AT",
            "DIM_BITS", "DIM_BITS_OR_NUMBER", "DIM_DIMENSIONS",
            "DIM_HIGH", "DIM_INCREMENT", "DIM_LEFT",
            "DIM_LOW", "DIM_RIGHT", "DIM_SIZE", "DIM_UNPK_DIMENSIONS",
            "DT_PUBLIC",
            "ENUM_FIRST", "ENUM_LAST", "ENUM_NUM",
            "ENUM_NEXT", "ENUM_PREV", "ENUM_NAME", "ENUM_VALID",
            "FUNC_ARG_PROTO", "FUNC_RETURN_PROTO",
            "TYPEID", "TYPENAME",
            "VAR_BASE", "VAR_FORCEABLE", "VAR_FSM_ARC_INCLUDE_COND",
            "VAR_FSM_ARC_INCLUDE_COND_AUTO",
            "VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND",
            "VAR_FSM_ARC_INCLUDE_COND_FULL",
            "VAR_FSM_STATE", "VAR_FSM_STATE_AUTO",
            "VAR_FSM_STATE_AUTO_EXPAND", "VAR_FSM_STATE_FULL",
            "VAR_PORT_DTYPE", "VAR_PUBLIC", "VAR_PUBLIC_FLAT",
            "VAR_PUBLIC_FLAT_RD", "VAR_PUBLIC_FLAT_RW",
            "VAR_SC_BIGUINT", "VAR_SC_BV", "VAR_SFORMAT", "VAR_SPLIT_VAR"
        };
        // clang-format on
        ပြန်ပေး names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() const {
        switch (m_e) {
        pak_sa DIM_BITS:
        pak_sa DIM_BITS_OR_NUMBER:
        pak_sa DIM_DIMENSIONS:
        pak_sa DIM_HIGH:
        pak_sa DIM_INCREMENT:
        pak_sa DIM_LEFT:
        pak_sa DIM_LOW:
        pak_sa DIM_RIGHT:
        pak_sa DIM_SIZE:
        pak_sa DIM_UNPK_DIMENSIONS:
        pak_sa TYPEID:
        pak_sa TYPENAME: ပြန်ပေး true;
        default: ပြန်ပေး false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
};
constexpr bool operator==(const VAttrType& lhs, const VAttrType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAttrType& lhs, VAttrType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, const VAttrType& rhs) { ပြန်ပေး lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

public:
    VBaseOverride()
        : m_extends{false}
        , m_final{false}
        , m_initial{false} {}
    class Extends {};
    explicit VBaseOverride(Extends)
        : m_extends{true}
        , m_final{false}
        , m_initial{false} {}
    class Final {};
    explicit VBaseOverride(Final)
        : m_extends{false}
        , m_final{true}
        , m_initial{false} {}
    class Initial {};
    explicit VBaseOverride(Initial)
        : m_extends{false}
        , m_final{false}
        , m_initial{true} {}
    void combine(const VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() const { ပြန်ပေး m_extends | m_final | m_initial; }
    bool isExtends() const { ပြန်ပေး m_extends; }
    bool isFinal() const { ပြန်ပေး m_final; }
    bool isInitial() const { ပြန်ပေး m_initial; }
    string ascii() const {
        string out;
        အကယ်၍ (m_initial) out = VString::dot(out, " ", "initial");
        အကယ်၍ (m_extends) out = VString::dot(out, " ", "extends");
        အကယ်၍ (m_final) out = VString::dot(out, " ", "final");
        ပြန်ပေး out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
public:
    ga_nanaa en : uint8_t {
        UNKNOWN,
        BIT,
        BYTE,
        CHANDLE,
        // Void type for tagged union members (CVOID to avoid Windows VOID macro)
        CVOID,
        NULLTYPE,  // type of SystemVerilog 'null' literal, which is self-determined
        EVENT,
        INT,
        INTEGER,
        INTEGER2STATE,  // Integer but two-state domain (to save info about original DType)
        LOGIC,
        LOGIC2STATE,  // Logic but two-state domain (to save info about original DType)
        LONGINT,
        DOUBLE,
        SHORTINT,
        TIME,
        TIME2STATE,  // Time but two-state domain (to save info about original DType)
        // Closer to a class type, but limited usage
        STRING,
        // Property / Sequence argument type
        UNTYPED,
        // Internal types for mid-steps
        SCOPEPTR,
        CHARPTR,
        MTASKSTATE,
        DELAY_SCHEDULER,
        TRIGGER_SCHEDULER,
        DYNAMIC_TRIGGER_SCHEDULER,
        FORK_SYNC,
        PROCESS_REFERENCE,
        RANDOM_GENERATOR,
        RANDOM_STDGENERATOR,
        COVERGROUP_INSTHANDLE,
        // Unsigned and two state; fundamental types
        UINT32,
        UINT64,
        // Internal types, eliminated after parsing
        LOGIC_IMPLICIT,
        // Leave last
        _ENUM_MAX
    };
    ga_nanaa en m_e;
    const var_na* ascii() const VL_MT_SAFE {
        static constexpr const var_na* const names[] = {"%E-unk",
                                                      "bit",
                                                      "byte",
                                                      "chandle",
                                                      "void",
                                                      "null",
                                                      "event",
                                                      "int",
                                                      "integer",
                                                      "integer2state",
                                                      "logic",
                                                      "logic2state",
                                                      "longint",
                                                      "real",
                                                      "shortint",
                                                      "time",
                                                      "time2state",
                                                      "string",
                                                      "untyped",
                                                      "VerilatedScope*",
                                                      "char*",
                                                      "VlMTaskState",
                                                      "VlDelayScheduler",
                                                      "VlTriggerScheduler",
                                                      "VlDynamicTriggerScheduler",
                                                      "VlFork",
                                                      "VlProcessRef",
                                                      "VlRandomizer",
                                                      "VlStdRandomizer",
                                                      "VlCovInstHandle",
                                                      "IData",
                                                      "QData",
                                                      "LOGIC_IMPLICIT",
                                                      " MAX"};
        ပြန်ပေး names[m_e];
    }
    const var_na* typeName() const VL_MT_SAFE {
        switch (m_e) {
        pak_sa LOGIC2STATE: ပြန်ပေး VBasicDTypeKwd{LOGIC}.ascii();
        pak_sa INTEGER2STATE: ပြန်ပေး VBasicDTypeKwd{INTEGER}.ascii();
        pak_sa TIME2STATE: ပြန်ပေး VBasicDTypeKwd{TIME}.ascii();
        default: ပြန်ပေး ascii();
        }
    }
    const var_na* dpiType() const {
        static constexpr const var_na* const names[] = {"%E-unk",
                                                      "svBit",
                                                      "char",
                                                      "void*",
                                                      "void",
                                                      "null",
                                                      "char",
                                                      "int",
                                                      "%E-integer",
                                                      "%E-integer",
                                                      "svLogic",
                                                      "svLogic",
                                                      "long long",
                                                      "double",
                                                      "short",
                                                      "%E-time",
                                                      "%E-time",
                                                      "const char*",
                                                      "%E-untyped",
                                                      "dpiScope",
                                                      "const char*",
                                                      "%E-mtaskstate",
                                                      "%E-dly-sched",
                                                      "%E-trig-sched",
                                                      "%E-dyn-sched",
                                                      "%E-fork",
                                                      "%E-proc-ref",
                                                      "%E-rand-gen",
                                                      "%E-stdrand-gen",
                                                      "%E-cg-insthandle",
                                                      "IData",
                                                      "QData",
                                                      "%E-logic-implct",
                                                      " MAX"};
        ပြန်ပေး names[m_e];
    }
    static void selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(ကိန်းပြည့် _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    ကိန်းပြည့် width() const {
        switch (m_e) {
        pak_sa BIT: ပြန်ပေး 1;  // scalar, can't bit extract unless ranged
        pak_sa BYTE: ပြန်ပေး 8;
        pak_sa CHANDLE: ပြန်ပေး 64;
        pak_sa NULLTYPE: ပြန်ပေး 1;  // Historical - bit was used instead of NULLTYPE
        pak_sa EVENT: ပြန်ပေး 1;
        pak_sa INT: ပြန်ပေး 32;
        pak_sa INTEGER: ပြန်ပေး 32;
        pak_sa INTEGER2STATE: ပြန်ပေး 32;
        pak_sa LOGIC: ပြန်ပေး 1;  // scalar, can't bit extract unless ranged
        pak_sa LOGIC2STATE: ပြန်ပေး 1;  // scalar, can't bit extract unless ranged
        pak_sa LONGINT: ပြန်ပေး 64;
        pak_sa DOUBLE: ပြန်ပေး 64;  // opaque
        pak_sa SHORTINT: ပြန်ပေး 16;
        pak_sa TIME: ပြန်ပေး 64;
        pak_sa TIME2STATE: ပြန်ပေး 64;
        pak_sa STRING: ပြန်ပေး 64;  // opaque  // Just the pointer, for today
        pak_sa SCOPEPTR: ပြန်ပေး 0;  // opaque
        pak_sa CHARPTR: ပြန်ပေး 0;  // opaque
        pak_sa MTASKSTATE: ပြန်ပေး 0;  // opaque
        pak_sa DELAY_SCHEDULER: ပြန်ပေး 0;  // opaque
        pak_sa TRIGGER_SCHEDULER: ပြန်ပေး 0;  // opaque
        pak_sa DYNAMIC_TRIGGER_SCHEDULER: ပြန်ပေး 0;  // opaque
        pak_sa FORK_SYNC: ပြန်ပေး 0;  // opaque
        pak_sa PROCESS_REFERENCE: ပြန်ပေး 0;  // opaque
        pak_sa RANDOM_GENERATOR: ပြန်ပေး 0;  // opaque
        pak_sa RANDOM_STDGENERATOR: ပြန်ပေး 0;  // opaque
        pak_sa COVERGROUP_INSTHANDLE: ပြန်ပေး 0;  // opaque
        pak_sa UINT32: ပြန်ပေး 32;
        pak_sa UINT64: ပြန်ပေး 64;
        default: ပြန်ပေး 0;
        }
    }
    bool isSigned() const {
        ပြန်ပေး m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() const {
        ပြန်ပေး m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() const {
        ပြန်ပေး m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() const {  // Otherwise initializes to X
        ပြန်ပေး (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() const {  // Enum increment supported
        ပြန်ပေး (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() const { ပြန်ပေး m_e == BIT; }
    bool isBitLogic() const {  // Bit/logic vector types; can form a packed array
        ပြန်ပေး (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() const {  // Can add "unsigned" to DPI
        ပြန်ပေး (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() const {  // Uses standard C layout, for DPI runtime access
        ပြန်ပေး (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() const VL_MT_SAFE {  // IE not a simple number we can bit optimize
        ပြန်ပေး (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() const VL_MT_SAFE { ပြန်ပေး m_e == CHANDLE; }
    bool isDouble() const VL_MT_SAFE { ပြန်ပေး m_e == DOUBLE; }
    bool isEvent() const { ပြန်ပေး m_e == EVENT; }
    bool isString() const VL_MT_SAFE { ပြန်ပေး m_e == STRING; }
    bool isMTaskState() const VL_MT_SAFE { ပြန်ပေး m_e == MTASKSTATE; }
    bool isSameish(const VBasicDTypeKwd& other) const VL_MT_SAFE {
        switch (other.m_e) {
        pak_sa LOGIC:
        pak_sa LOGIC2STATE:
        pak_sa LOGIC_IMPLICIT: ပြန်ပေး m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        pak_sa INTEGER:
        pak_sa INTEGER2STATE: ပြန်ပေး m_e == INTEGER || m_e == INTEGER2STATE;
        pak_sa TIME:
        pak_sa TIME2STATE: ပြန်ပေး m_e == TIME || m_e == TIME2STATE;
        default: ပြန်ပေး m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() const VL_MT_SAFE {
        switch (m_e) {
        pak_sa BIT:
        pak_sa BYTE:
        pak_sa CHANDLE:
        pak_sa INT:
        pak_sa INTEGER:
        pak_sa INTEGER2STATE:
        pak_sa LOGIC:
        pak_sa LOGIC2STATE:
        pak_sa LONGINT:
        pak_sa DOUBLE:
        pak_sa SHORTINT:
        pak_sa SCOPEPTR:
        pak_sa CHARPTR:
        pak_sa UINT32:
        pak_sa UINT64: ပြန်ပေး true;
        default: ပြန်ပေး false;
        }
    }

    const var_na* traceSigType() const {
        // VerilatedTraceSigType to used in trace signal declaration
        static constexpr const var_na* const lut[] = {
            /* UNKNOWN:                   */ "",  // Should not be traced
            /* BIT:                       */ "BIT",
            /* BYTE:                      */ "BYTE",
            /* CHANDLE:                   */ "LONGINT",
            /* CVOID:                     */ "",  // Should not be traced
            /* NULLTYPE:                  */ "",  // Should not be traced
            /* EVENT:                     */ "EVENT",
            /* INT:                       */ "INT",
            /* INTEGER:                   */ "INTEGER",
            /* INTEGER2STATE              */ "INTEGER",
            /* LOGIC:                     */ "LOGIC",
            /* LOGIC2STATE:               */ "LOGIC",
            /* LONGINT:                   */ "LONGINT",
            /* DOUBLE:                    */ "DOUBLE",
            /* SHORTINT:                  */ "SHORTINT",
            /* TIME:                      */ "TIME",
            /* TIME2STATE                 */ "TIME",
            /* STRING:                    */ "",
            /* UNTYPED:                   */ "",  // Should not be traced
            /* SCOPEPTR:                  */ "",  // Should not be traced
            /* CHARPTR:                   */ "",  // Should not be traced
            /* MTASKSTATE:                */ "",  // Should not be traced
            /* DELAY_SCHEDULER:           */ "",  // Should not be traced
            /* TRIGGER_SCHEDULER:         */ "",  // Should not be traced
            /* DYNAMIC_TRIGGER_SCHEDULER: */ "",  // Should not be traced
            /* FORK_SYNC:                 */ "",  // Should not be traced
            /* PROCESS_REFERENCE:         */ "",  // Should not be traced
            /* RANDOM_GENERATOR:          */ "",  // Should not be traced
            /* RANDOM_STD_GENERATOR:      */ "",  // Should not be traced
            /* COVERGROUP_INSTHANDLE:     */ "",  // Should not be traced
            /* UINT32:                    */ "BIT",
            /* UINT64:                    */ "BIT",
            /* LOGIC_IMPLICIT:            */ "",  // Should not be traced
        };
        static_assert(parimaa_na(lut) / parimaa_na(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        ပြန်ပေး lut[m_e];
    }
};
constexpr bool operator==(const VBasicDTypeKwd& lhs, const VBasicDTypeKwd& rhs) VL_MT_SAFE {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    ပြန်ပေး lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, const VBasicDTypeKwd& rhs) VL_MT_SAFE {
    ပြန်ပေး lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
public:
    ga_nanaa en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    ga_nanaa en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"FALSE", "TRUE", "UNK"};
        ပြန်ပေး names[m_e];
    }
    bool isKnown() const { ပြန်ပေး m_e != BU_UNKNOWN; }
    bool trueKnown() const { ပြန်ပေး m_e == BU_TRUE; }
    void setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(const VBoolOrUnknown& lhs, const VBoolOrUnknown& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    ပြန်ပေး lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, const VBoolOrUnknown& rhs) {
    ပြန်ပေး lhs == rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VBoolOrUnknown& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
public:
    ga_nanaa en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    ga_nanaa en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    bool unknown() const { ပြန်ပေး m_e == BP_UNKNOWN; }
    bool likely() const { ပြန်ပေး m_e == BP_LIKELY; }
    bool unlikely() const { ပြန်ပေး m_e == BP_UNLIKELY; }
    VBranchPred invert() const {
        အကယ်၍ (m_e == BP_UNLIKELY) ပြန်ပေး BP_LIKELY;
        အကယ်၍ (m_e == BP_LIKELY) ပြန်ပေး BP_UNLIKELY;
        ပြန်ပေး m_e;
    }
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        ပြန်ပေး names[m_e];
    }
    const var_na* asciiShort() const {
        static constexpr const var_na* const names[] = {"", "L", "!L"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VBranchPred& lhs, const VBranchPred& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VBranchPred& lhs, VBranchPred::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, const VBranchPred& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VBranchPred& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
static constexpr bool validateArgsDescriptor(const var_na* descrp) {
    // Is "TODO"
    အကယ်၍ (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        ပြန်ပေး true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    အတွက် (const var_na* cp = descrp; *cp; ++cp) {
        အကယ်၍ (*cp == '+') ပြန်ပေး cp != descrp && !cp[1];
        အကယ်၍ (*cp != 'r' && *cp != 'w' && *cp != 'm') ပြန်ပေး false;
    }
    ပြန်ပေး true;
}
// LCOV_EXCL_STOP
};  //namespace

// C++ methods invoked on runtime library data types via AstCMethodHard.
// The clean flag indicates whether unused result bits are guaranteed to be zero.
// The argument descriptor gives the access direction of each argument:
// - 'r' read by the call
// - 'w' written by the call (Use only if unconditionally and wholly written,
//   that is: a preceding write can be removed. Otherwise use 'm'.)
// - 'm' read and written (modified) by the call
// - '+' repeats the entry before it for all remaining arguments, must be last
// - "" if the method takes no arguments
// - "TODO" if not yet checked due to existing issues
// clang-format off
#define FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     PURE,   CLEAN,  "r") \
    macro(ARRAY_AT_BACK,                      "atBack",                 PURE,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                PURE,   CLEAN,  "r") \
    macro(ARRAY_FIND,                         "find",                   PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_FIRST,                   "find_first",             PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_FIRST_INDEX,             "find_first_index",       PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_INDEX,                   "find_index",             PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_LAST,                    "find_last",              PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_LAST_INDEX,              "find_last_index",        PURE,   CLEAN,  "") \
    macro(ARRAY_FIRST,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 PURE,   CLEAN,  "r") \
    macro(ARRAY_LAST,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    PURE,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    PURE,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    PURE,   CLEAN,  "") \
    macro(ARRAY_NEXT,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_POP_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_POP_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_PUSH_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_PUSH_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_REVERSE,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  PURE,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   PURE,   CLEAN,  "") \
    macro(ARRAY_R_PRODUCT,                    "r_product",              PURE,   false,  "") \
    macro(ARRAY_R_SUM,                        "r_sum",                  PURE,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  PURE,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_SLICE,                        "slice",                  PURE,   CLEAN,  "r") \
    macro(ARRAY_SORT,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 PURE,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_INDEX,                 "unique_index",           PURE,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 PURE,   CLEAN,  "r") \
    macro(ASSOC_FIRST,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_NEXT,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_SIZE,                         "size",                   PURE,   CLEAN,  "") \
    macro(CLASS_SET_RANDMODE,                 "set_randmode",           false,  CLEAN,  "r") \
    macro(COVERGROUP_ADD_ARRAY_NAMER,         "addArrayNamer",          false,  CLEAN,  "r+") \
    macro(COVERGROUP_ADD_BIN,                 "addBin",                 false,  CLEAN,  "r+") \
    macro(COVERGROUP_ADD_COVERPOINT,          "addCoverpoint",          false,  CLEAN,  "") \
    macro(COVERGROUP_ADD_CROSS,               "addCross",               false,  CLEAN,  "") \
    macro(COVERGROUP_ADD_CROSS_DYN,           "addCrossDyn",            false,  CLEAN,  "") \
    macro(COVERGROUP_ADD_NUMBERED_NAMER,      "addNumberedNamer",       false,  CLEAN,  "r+") \
    macro(COVERGROUP_ADD_SINGLE_NAMER,        "addSingleNamer",         false,  CLEAN,  "r+") \
    macro(COVERGROUP_ATTACH,                  "attach",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_CLEAR_HIT_LIST,          "clearHitList",           false,  CLEAN,  "") \
    macro(COVERGROUP_COVERAGE,                "coverage",               false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_BEGIN,        "withBegin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_CROSS_WITH_END,          "withEnd",                false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_TEST,         "withTest",               false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_VALUE,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_VALUE_W,      "withValueW",             false,  CLEAN,  "rw") \
    macro(COVERGROUP_FINALIZE_BINS,           "finalizeBins",           false,  CLEAN,  "") \
    macro(COVERGROUP_INCREMENT_BIN,           "incrementBin",           false,  CLEAN,  "r") \
    macro(COVERGROUP_INIT,                    "init",                   false,  CLEAN,  "r+") \
    macro(COVERGROUP_INST_P,                  "p",                      PURE,   CLEAN,  "") \
    macro(COVERGROUP_LEND_WEIGHT,             "lendWeight",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_RECORD_HIT,              "recordHit",              false,  CLEAN,  "r") \
    macro(COVERGROUP_REGISTER_BINS,           "registerBins",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_SAMPLE,                  "sample",                 false,  CLEAN,  "") \
    macro(COVERGROUP_SAMPLE_IFFS,             "sample",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_SELECT_ALL,              "selectAll",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_AND,              "selectAnd",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_BIN,              "selectBin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM,              "selectDim",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM_END,          "selectDimEnd",           false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_OR,               "selectOr",               false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_RANGE,            "selectRange",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SELECT_RANGE_W,          "selectRangeW",           false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_END,               "sizedEnd",               PURE,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_FINISH,            "sizedFinish",            false,  CLEAN,  "r+") \
    macro(COVERGROUP_SIZED_FIRST,             "sizedFirst",             PURE,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_RANGE,             "sizedRange",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_RANGE_W,           "sizedRangeW",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_SAMPLE,            "sizedSample",            false,  CLEAN,  "rrr") \
    macro(COVERGROUP_SIZED_SAMPLE_W,          "sizedSampleW",           false,  CLEAN,  "rrr") \
    macro(COVERGROUP_TYPE_COVERAGE,           "typeCoverage",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_TYPE_WEIGHT,             "typeWeight",             false,  CLEAN,  "r") \
    macro(COVERGROUP_VALUE_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_VALUE_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_VALUE_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_VALUE_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_VALUE_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_VALUE_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_VALUE_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_VALUE_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_VALUE_TYPE,              "valueType",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_WEIGHT,                  "weight",                 false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_BEGIN,              "withBegin",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_FINISH,             "withFinish",             false,  CLEAN,  "r+") \
    macro(COVERGROUP_WITH_HI,                 "withHi",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_HI_W,               "withHiW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_LO,                 "withLo",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_LO_W,               "withLoW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_NEXT,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_RUN,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_RUN_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_BACK,           "atWriteAppendBack",      false,  CLEAN,  "r") \
    macro(DYN_CLEAR,                          "clear",                  false,  CLEAN,  "") \
    macro(DYN_ERASE,                          "erase",                  false,  CLEAN,  "r") \
    macro(DYN_INSERT,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_POP,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_POP_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_PUSH,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_PUSH_FRONT,                     "push_front",             false,  CLEAN,  "r") \
    macro(DYN_RENEW,                          "renew",                  false,  CLEAN,  "r") \
    macro(DYN_RENEW_COPY,                     "renew_copy",             false,  CLEAN,  "rr") \
    macro(DYN_RESIZE,                         "resize",                 false,  CLEAN,  "TODO") \
    macro(DYN_SIZE,                           "size",                   PURE,   CLEAN,  "") \
    macro(DYN_SLICE,                          "slice",                  PURE,   CLEAN,  "rr") \
    macro(DYN_SLICE_ASSIGN,                   "sliceAssign",            false,  CLEAN,  "rrr") \
    macro(DYN_SLICE_ASSIGN_BACK_BACK,         "sliceAssignBackBack",    false,  CLEAN,  "rrr") \
    macro(DYN_SLICE_ASSIGN_FRONT_BACK,        "sliceAssignFrontBack",   false,  CLEAN,  "rrr") \
    macro(DYN_SLICE_BACK_BACK,                "sliceBackBack",          PURE,   CLEAN,  "rr") \
    macro(DYN_SLICE_FRONT_BACK,               "sliceFrontBack",         PURE,   CLEAN,  "rr") \
    macro(EVENT_CLEAR_FIRED,                  "clearFired",             false,  CLEAN,  "") \
    macro(EVENT_CLEAR_TRIGGERED,              "clearTriggered",         false,  CLEAN,  "") \
    macro(EVENT_FIRE,                         "fire",                   false,  CLEAN,  "") \
    macro(EVENT_IS_FIRED,                     "isFired",                PURE,   CLEAN,  "") \
    macro(EVENT_IS_TRIGGERED,                 "isTriggered",            PURE,   CLEAN,  "") \
    macro(FORCE_ADD,                          "addForce",               false,  CLEAN,  "rrrr+") \
    macro(FORCE_READ,                         "read",                   PURE,   CLEAN,  "r") \
    macro(FORCE_READ_INDEX,                   "readIndex",              PURE,   CLEAN,  "rr") \
    macro(FORCE_READ_SEL,                     "readSel",                PURE,   CLEAN,  "TODO") \
    macro(FORCE_RELEASE,                      "release",                false,  CLEAN,  "rr+") \
    macro(FORCE_TOUCH,                        "touch",                  false,  CLEAN,  "") \
    macro(FORK_DONE,                          "done",                   false,  CLEAN,  "rr") \
    macro(FORK_INIT,                          "init",                   false,  CLEAN,  "rr") \
    macro(FORK_JOIN,                          "join",                   false,  CLEAN,  "rrr") \
    macro(FORK_ON_KILL,                       "onKill",                 false,  CLEAN,  "r") \
    macro(NBA_COMMIT,                         "commit",                 false,  CLEAN,  "w") \
    macro(NBA_ENQUEUE,                        "enqueue",                false,  CLEAN,  "r+") \
    macro(RANDOMIZER_BASIC_STD_RANDOMIZATION, "basicStdRandomization",  false,  CLEAN,  "mr") \
    macro(RANDOMIZER_CLEARCONSTRAINTS,        "clearConstraints",       false,  CLEAN,  "") \
    macro(RANDOMIZER_CLEARALL,                "clearAll",               false,  CLEAN,  "") \
    macro(RANDOMIZER_DISABLE_SOFT,            "disable_soft",           false,  CLEAN,  "r") \
    macro(RANDOMIZER_HARD,                    "hard",                   false,  CLEAN,  "r+") \
    macro(RANDOMIZER_SOFT,                    "soft",                   false,  CLEAN,  "rrrr") \
    macro(RANDOMIZER_UNIQUE,                  "rand_unique",            false,  CLEAN,  "rr") \
    macro(RANDOMIZER_MARK_RANDC,              "markRandc",              false,  CLEAN,  "r") \
    macro(RANDOMIZER_SOLVE_BEFORE,            "solveBefore",            false,  CLEAN,  "rr") \
    macro(RANDOMIZER_PIN_VAR,                 "pin_var",                false,  CLEAN,  "rrr") \
    macro(RANDOMIZER_UPDATE_VAR,              "update_var",             false,  CLEAN,  "TODO") \
    macro(RANDOMIZER_WRITE_VAR,               "write_var",              false,  CLEAN,  "TODO") \
    macro(RANDOMIZER_SET_VAR_DISABLED,        "set_var_disabled",       false,  CLEAN,  "r") \
    macro(RANDOMIZER_CLEAR_VAR_DISABLED,      "clear_var_disabled",     false,  CLEAN,  "r") \
    macro(RANDOMIZER_MARK_VAR_STATIC,         "mark_var_static",        false,  CLEAN,  "r") \
    macro(RANDOMIZER_SET_STATIC_RANDMODE,     "set_static_randmode",    false,  CLEAN,  "r") \
    macro(RNG_GET_RANDSTATE,                  "__Vm_rng.get_randstate", PURE,   CLEAN,  "") \
    macro(RNG_SET_RANDSTATE,                  "__Vm_rng.set_randstate", false,  CLEAN,  "r") \
    macro(SCHED_ANY_TRIGGERED,                "anyTriggered",           false,  CLEAN,  "r") \
    macro(SCHED_AWAITING_CURRENT_TIME,        "awaitingCurrentTime",    PURE,   CLEAN,  "") \
    macro(SCHED_AWAITING_ZERO_DELAY,          "awaitingZeroDelay",      PURE,   CLEAN,  "") \
    macro(SCHED_READY,                        "ready",                  false,  CLEAN,  "r") \
    macro(SCHED_MOVE_TO_RESUME_QUEUE,         "moveToResumeQueue",      false,  CLEAN,  "r") \
    macro(SCHED_DELAY,                        "delay",                  false,  CLEAN,  "rrrr") \
    macro(SCHED_DO_POST_UPDATES,              "doPostUpdates",          false,  CLEAN,  "") \
    macro(SCHED_EVALUATE,                     "evaluate",               false,  CLEAN,  "") \
    macro(SCHED_EVALUATION,                   "evaluation",             false,  CLEAN,  "rrrr") \
    macro(SCHED_POST_UPDATE,                  "postUpdate",             false,  CLEAN,  "rrrr") \
    macro(SCHED_RESUME,                       "resume",                 false,  CLEAN,  "TODO") \
    macro(SCHED_RESUME_ZERO_DELAY,            "resumeZeroDelay",        false,  CLEAN,  "") \
    macro(SCHED_RESUMPTION,                   "resumption",             false,  CLEAN,  "rrrr") \
    macro(SCHED_TRIGGER,                      "trigger",                false,  CLEAN,  "rrrrr") \
    macro(SCHED_WAIT_FOREVER,                 "waitForever",            false,  CLEAN,  "rrr") \
    macro(UNPACKED_ASSIGN,                    "assign",                 false,  CLEAN,  "r") \
    macro(UNPACKED_FILL,                      "fill",                   false,  CLEAN,  "r") \
    macro(UNPACKED_NEQ,                       "neq",                    PURE,   CLEAN,  "r")
// clang-format on

class VCMethod final {
    static constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    static constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

public:
    ga_nanaa en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    ga_nanaa en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const VL_PURE {
        static constexpr const var_na* const values[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        ပြန်ပေး values[m_e];
    }
    bool isPure() const VL_PURE {
        static constexpr bool values[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        ပြန်ပေး values[m_e];
    }
    bool isClean() const VL_PURE {
        static constexpr bool values[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        ပြန်ပေး values[m_e];
    }
    const var_na* args() const VL_PURE {
        static constexpr const var_na* const values[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        ပြန်ပေး values[m_e];
    }
    // Return array method for given name
    static VCMethod arrayMethod(const string& name);
};
constexpr bool operator==(const VCMethod& lhs, const VCMethod& rhs) { ပြန်ပေး lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VCMethod& lhs, VCMethod::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, const VCMethod& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VCMethod& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#define VL_CMETHOD_ARGS_CHECK(id, method, pure, clean, args) \
    static_assert(validateArgsDescriptor(args), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_ARGS_CHECK)
#undef VL_CMETHOD_ARGS_CHECK

#undef FOR_EACH_CMETHOD

// clang-format off
#define FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_TO_SOLVER_HEX,           "vlToSolverHex",          PURE,   "r")
// clang-format on

class VCFunction final {
    static constexpr bool PURE = true;  // For macro expansion of 'pure' field only

public:
    ga_nanaa en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    ga_nanaa en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    const var_na* ascii() const VL_PURE {
        static const var_na* const values[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        ပြန်ပေး values[m_e];
    }
    bool isPure() const VL_PURE {
        static const bool values[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        ပြန်ပေး values[m_e];
    }
    const var_na* args() const VL_PURE {
        static const var_na* const values[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        ပြန်ပေး values[m_e];
    }
};

// Static assert all argument descriptors are well formed
#define VL_CFUNCTION_ARGS_CHECK(id, method, pure, args) \
    static_assert(validateArgsDescriptor(args), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS_CHECK)
#undef VL_CFUNCTION_ARGS_CHECK

#undef FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
public:
    ga_nanaa en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

private:
    sa_nracanaa Item final {
        ga_nanaa en m_e;  // Statement's enum mnemonic, for checking
        const var_na* m_name;  // Statements name, for debugging
    };
    static Item s_itemData[];

public:
    ga_nanaa en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const VL_PURE {
        static constexpr const var_na* const names[] = {"none", "ctor_var_reset_call"};
        ပြန်ပေး names[m_e];
    }
    bool isNone() const { ပြန်ပေး m_e == NONE; }
};
constexpr bool operator==(const VCStmtType& lhs, const VCStmtType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VCStmtType& lhs, VCStmtType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, const VCStmtType& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VCStmtType& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
public:
    ga_nanaa en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    ga_nanaa en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const VL_PURE {
        static constexpr const var_na* const names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VCaseType& lhs, const VCaseType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VCaseType& lhs, VCaseType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, const VCaseType& rhs) { ပြန်ပေး lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
public:
    ga_nanaa en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        ပြန်ပေး names[m_e];
    }
    bool isAssignable() const { ပြန်ပေး m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
};
constexpr bool operator==(const VCastable& lhs, const VCastable& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VCastable& lhs, VCastable::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, const VCastable& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VCastable& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
public:
    ga_nanaa en : uint8_t {
        BINS_ARRAY,  // Array of bins with user-speciifed size
        BINS_AUTO,  // Automatic bins of a 'bins auto[N]' declaration
        BINS_AUTO_IMPLICIT,  // Automatic bins of a coverpoint without bins (IEEE 1800-2023 19.5.3)
        BINS_DEFAULT,  // Default bin
        BINS_IGNORE,  // Ignore bin
        BINS_ILLEGAL,  // Illegal bin
        BINS_TRANSITION,  // Transition bin
        BINS_USER,  // Single bin with one or more values/ranges
        BINS_WILDCARD  // Wildcard bin
    };
    ga_nanaa en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() const { ပြန်ပေး m_e; }  // LCOV_EXCL_LINE
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        ပြန်ပေး names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    const var_na* verilogKwd() const {
        static constexpr const var_na* const names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        ပြန်ပေး names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    const var_na* binSetEnum() const {
        switch (m_e) {
        pak_sa BINS_IGNORE: ပြန်ပေး "VlCovBinKind::KIND_IGNORE";
        pak_sa BINS_ILLEGAL: ပြန်ပေး "VlCovBinKind::KIND_ILLEGAL";
        pak_sa BINS_DEFAULT: ပြန်ပေး "VlCovBinKind::KIND_DEFAULT";
        default: ပြန်ပေး "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() const {
        ပြန်ပေး m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(const VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    ပြန်ပေး lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
public:
    ga_nanaa en : uint8_t {
        // Shared by option.* and type_option.*
        WEIGHT,
        GOAL,
        AT_LEAST,
        AUTO_BIN_MAX,
        PER_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_NUM_PRINT_MISSING,
        CROSS_RETAIN_AUTO_BINS,
        DETECT_OVERLAP,
        GET_INST_COVERAGE,
        // type_option.* only (IEEE 1800-2023 Table 19-3)
        STROBE,
        MERGE_INSTANCES,
        DISTRIBUTE_FIRST,
        REAL_INTERVAL,
        // Legacy option.* accepted for compatibility
        CROSS_AUTO_BIN_MAX,
        // sentinel - should never appear after parse-time validation
        UNKNOWN
    };
    ga_nanaa en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"weight",
                                                      "goal",
                                                      "at_least",
                                                      "auto_bin_max",
                                                      "per_instance",
                                                      "comment",
                                                      "name",
                                                      "cross_num_print_missing",
                                                      "cross_retain_auto_bins",
                                                      "detect_overlap",
                                                      "get_inst_coverage",
                                                      "strobe",
                                                      "merge_instances",
                                                      "distribute_first",
                                                      "real_interval",
                                                      "cross_auto_bin_max",
                                                      "unknown"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    ပြန်ပေး lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
public:
    ga_nanaa en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    ga_nanaa en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(ကိန်းပြည့် _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }  // LCOV_EXCL_LINE
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"", "[*]", "[->]", "[=]"};
        ပြန်ပေး names[m_e];
    }
    const var_na* asciiJson() const {
        static constexpr const var_na* const names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        ပြန်ပေး names[m_e];
    }
};

//######################################################################

class VDirection final {
public:
    ga_nanaa en : uint8_t { NONE, INPUT, OUTPUT, INOUT, REF, CONSTREF };
    ga_nanaa en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const VL_MT_SAFE { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        ပြန်ပေး names[m_e];
    }
    string verilogKwd() const {
        static constexpr const var_na* const names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        ပြန်ပေး names[m_e];
    }
    string prettyName() const { ပြန်ပေး verilogKwd(); }
    bool isAny() const { ပြန်ပေး m_e != NONE; }
    bool isInout() const { ပြန်ပေး m_e == INOUT; }
    bool isInoutOrRef() const { ပြန်ပေး m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool isInput() const { ပြန်ပေး m_e == INPUT; }
    bool isNonOutput() const {
        ပြန်ပေး m_e == INPUT || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() const { ပြန်ပေး m_e == OUTPUT; }
    bool isReadOnly() const VL_MT_SAFE { ပြန်ပေး m_e == INPUT || m_e == CONSTREF; }
    bool isWritable() const VL_MT_SAFE { ပြန်ပေး m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() const VL_MT_SAFE { ပြန်ပေး m_e == REF; }
    bool isConstRef() const VL_MT_SAFE { ပြန်ပေး m_e == CONSTREF; }
    string traceSigDirection() const {
        အကယ်၍ (isInout()) {
            ပြန်ပေး "VerilatedTraceSigDirection::INOUT";
        } မဟုတ်ပါက အကယ်၍ (isWritable()) {
            ပြန်ပေး "VerilatedTraceSigDirection::OUTPUT";
        } မဟုတ်ပါက အကယ်၍ (isNonOutput()) {
            ပြန်ပေး "VerilatedTraceSigDirection::INPUT";
        }
        ပြန်ပေး "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() const {
        switch (m_e) {
        pak_sa NONE: ပြန်ပေး VAccess::NOCHANGE;
        pak_sa INPUT:
        pak_sa CONSTREF: ပြန်ပေး VAccess::READ;
        pak_sa OUTPUT: ပြန်ပေး VAccess::WRITE;
        pak_sa INOUT:
        pak_sa REF: ပြန်ပေး VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(const VDirection& lhs, const VDirection& rhs) VL_MT_SAFE {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    ပြန်ပေး lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, const VDirection& rhs) VL_MT_SAFE {
    ပြန်ပေး lhs == rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VDirection& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
public:
    ga_nanaa en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    ga_nanaa en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    bool addNewline() const { ပြန်ပေး m_e != DT_WRITE; }
    bool needScopeTracking() const { ပြန်ပေး m_e != DT_DISPLAY && m_e != DT_WRITE; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VDisplayType& lhs, const VDisplayType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VDisplayType& lhs, VDisplayType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, const VDisplayType& rhs) { ပြန်ပေး lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
public:
    ga_nanaa en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    ga_nanaa en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VDumpCtlType& lhs, const VDumpCtlType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VDumpCtlType& lhs, VDumpCtlType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, const VDumpCtlType& rhs) { ပြန်ပေး lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
public:
    // REMEMBER to edit the strings below too
    ga_nanaa en : uint8_t {
        // These must be in general -> most specific order, as we sort by it
        // in V3Const::visit AstSenTree
        // Involving a variable
        ET_CHANGED,  // Value changed
        ET_BOTHEDGE,  // POSEDGE | NEGEDGE (i.e.: 'edge' in Verilog)
        ET_POSEDGE,
        ET_NEGEDGE,
        ET_EVENT,  // VlEventBase::isFired
        // Involving an expression
        ET_TRUE,
        ET_INITIAL_NBA,  // Event that is fired initially and never again
        //
        ET_COMBO,  // Sensitive to all combo inputs to this block
        ET_COMBO_STAR,  // Sensitive to all combo inputs to this block (from .*)
        ET_HYBRID,  // This is like ET_COMB, but with explicit sensitivity to an expression
        ET_STATIC,  // static variable initializers (runs before 'initial')
        ET_INITIAL,  // 'initial' statements
        ET_FINAL,  // 'final' statements
        ET_NEVER  // Never occurs (optimized away)
    };
    ga_nanaa en m_e;
    bool clockedStmt() const {
        static constexpr bool clocked[] = {
            true,  // ET_CHANGED
            true,  // ET_BOTHEDGE
            true,  // ET_POSEDGE
            true,  // ET_NEGEDGE
            true,  // ET_EVENT
            true,  // ET_TRUE
            true,  // ET_INITIAL_NBA

            false,  // ET_COMBO
            false,  // ET_COMBO_STAR
            false,  // ET_HYBRID
            false,  // ET_STATIC
            false,  // ET_INITIAL
            false,  // ET_FINAL
            false,  // ET_NEVER
        };
        ပြန်ပေး clocked[m_e];
    }
    bool anEdge() const { ပြန်ပေး m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() const {
        switch (m_e) {
        pak_sa ET_CHANGED: ပြန်ပေး ET_CHANGED;
        pak_sa ET_BOTHEDGE: ပြန်ပေး ET_BOTHEDGE;
        pak_sa ET_POSEDGE: ပြန်ပေး ET_NEGEDGE;
        pak_sa ET_NEGEDGE: ပြန်ပေး ET_POSEDGE;
        default: UASSERT_STATIC(0, "Inverting bad edgeType()"); ပြန်ပေး ET_NEGEDGE;
        }
    }
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        ပြန်ပေး names[m_e];
    }
    const var_na* verilogKwd() const {
        static constexpr const var_na* const names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        ပြန်ပေး names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(const VEdgeType& other) const {
        switch (m_e) {
        pak_sa VEdgeType::ET_POSEDGE:
            အကယ်၍ (other.m_e == VEdgeType::ET_NEGEDGE) ပြန်ပေး true;
            bha_nga;
        pak_sa VEdgeType::ET_NEGEDGE:
            အကယ်၍ (other.m_e == VEdgeType::ET_POSEDGE) ပြန်ပေး true;
            bha_nga;
        default: bha_nga;
        }
        ပြန်ပေး false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
};
constexpr bool operator==(const VEdgeType& lhs, const VEdgeType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VEdgeType& lhs, VEdgeType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, const VEdgeType& rhs) { ပြန်ပေး lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#define FOR_EACH_EVAL(macro) \
    /*    id,        iterated,  hasTrigs, firstIt, slow */ \
    macro(STATIC,    false,     false,    false,   true) \
    macro(INITIAL,   false,     false,    false,   true) \
    macro(STL,       true,      true,     true,    true) \
    macro(SAMPLE,    false,     false,    false,   false) \
    macro(ICO,       true,      true,     true,    false) \
    macro(ACT,       true,      true,     false,   false) \
    macro(INACT,     true,      false,    false,   false) \
    macro(NBA,       true,      true,     false,   false) \
    macro(OBS,       true,      true,     false,   false) \
    macro(REACT,     true,      true,     false,   false) \
    macro(POSTPONED, false,     false,    false,   false) \
    macro(FINAL,     false,     false,    false,   true)
// clang-format on

class VEval final {
public:
    ga_nanaa en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    ga_nanaa en m_e;

    const var_na* ascii() const {
        static constexpr const var_na* const values[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        ပြန်ပေး values[m_e];
    }
    bool isIterated() const {
        static constexpr bool values[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        ပြန်ပေး values[m_e];
    }
    bool hasTriggers() const {
        static constexpr bool values[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        ပြန်ပေး values[m_e];
    }
    bool firstIteration() const {
        static constexpr bool values[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        ပြန်ပေး values[m_e];
    }
    bool slow() const {
        static constexpr bool values[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        ပြန်ပေး values[m_e];
    }

    // Short name
    std::string tag() const { ပြန်ပေး VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() const { ပြန်ပေး "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() const { ပြန်ပေး "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() const { ပြန်ပေး "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() const { ပြန်ပေး "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() const { ပြန်ပေး m_e; }

private:
    std::string capitalizedTag() const {
        const std::string name = ascii();
        ပြန်ပေး name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
public:
    ga_nanaa en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        ပြန်ပေး names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    // Is a node type compatible with the declaration
    bool isNodeCompatible(const AstNode* nodep) const;
};
constexpr bool operator==(const VFwdType& lhs, const VFwdType& rhs) { ပြန်ပေး lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VFwdType& lhs, VFwdType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, const VFwdType& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VFwdType& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    static uint64_t s_cachedCntGbl;  // Global computed count

public:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() const { ပြန်ပေး m_cachedCnt == s_cachedCntGbl; }
    bool get() const { ပြန်ပေး m_state; }
    void set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    void clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    static void clearCacheTree() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
public:
    ga_nanaa en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    ga_nanaa en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        ပြန်ပေး names[m_e];
    }
    const var_na* verilogKwd() const {
        static constexpr const var_na* const names[] = {"join", "join_any", "join_none"};
        ပြန်ပေး names[m_e];
    }
    bool join() const { ပြန်ပေး m_e == JOIN; }
    bool joinAny() const { ပြန်ပေး m_e == JOIN_ANY; }
    bool joinNone() const { ပြန်ပေး m_e == JOIN_NONE; }
};
constexpr bool operator==(const VJoinType& lhs, const VJoinType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VJoinType& lhs, VJoinType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, const VJoinType& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VJoinType& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
public:
    ga_nanaa en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        ပြန်ပေး names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    bool isNone() const { ပြန်ပေး m_e == NONE; }
    bool isAutomatic() const { ပြန်ပေး m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() const { ပြန်ပေး m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() const { ပြန်ပေး m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() const {
        switch (m_e) {
        pak_sa AUTOMATIC_EXPLICIT: ပြန်ပေး AUTOMATIC_IMPLICIT;
        pak_sa STATIC_EXPLICIT: ပြန်ပေး STATIC_IMPLICIT;
        default: ပြန်ပေး m_e;
        }
    }
};
constexpr bool operator==(const VLifetime& lhs, const VLifetime& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VLifetime& lhs, VLifetime::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, const VLifetime& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VLifetime& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
public:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            ကိန်းပြည့် m_index;  // Current index
            const ကိန်းပြည့် m_inc;  // Step, +1 or -1
        public:
            Iterator(ကိန်းပြည့် index, ကိန်းပြည့် inc)
                : m_index{index}
                , m_inc{inc} {}
            ကိန်းပြည့် operator*() const { ပြန်ပေး m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                ပြန်ပေး *this;
            }
            bool operator!=(const Iterator& that) const { ပြန်ပေး m_index != that.m_index; }
        };
        const ကိန်းပြည့် m_begin;  // First index
        const ကိန်းပြည့် m_end;  // One past the last index
        const ကိန်းပြည့် m_inc;  // Step, +1 or -1
    public:
        Sequence(ကိန်းပြည့် first, ကိန်းပြည့် last, ကိန်းပြည့် inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() const { ပြန်ပေး Iterator{m_begin, m_inc}; }
        Iterator end() const { ပြန်ပေး Iterator{m_end, m_inc}; }
    };

    ကိန်းပြည့် m_left = 0;  // Left side of range (pre-':')
    ကိန်းပြည့် m_right = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(const VNumRange& rhs) const {
        ပြန်ပေး m_left == rhs.m_left && m_right == rhs.m_right && m_ranged == rhs.m_ranged;
    }
    bool operator<(const VNumRange& rhs) const {
        အကယ်၍ ((m_left < rhs.m_left)) ပြန်ပေး true;
        အကယ်၍ (!(m_left == rhs.m_left)) ပြန်ပေး false;  // lhs > rhs
        အကယ်၍ ((m_right < rhs.m_right)) ပြန်ပေး true;
        အကယ်၍ (!(m_right == rhs.m_right)) ပြန်ပေး false;  // lhs > rhs
        အကယ်၍ ((m_ranged < rhs.m_ranged)) ပြန်ပေး true;
        အကယ်၍ (!(m_ranged == rhs.m_ranged)) ပြန်ပေး false;  // lhs > rhs
        ပြန်ပေး false;
    }
    //
    VNumRange() = default;
    VNumRange(ကိန်းပြည့် hi, ကိန်းပြည့် lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(ကိန်းပြည့် left, ကိန်းပြည့် right)
        : m_left{left}
        , m_right{right}
        , m_ranged{true} {}
    ~VNumRange() = default;
    // MEMBERS
    void init(ကိန်းပြည့် hi, ကိန်းပြည့် lo, bool ascending) {
        အကယ်၍ (lo > hi) std::swap(hi, lo);
        m_left = ascending ? lo : hi;
        m_right = ascending ? hi : lo;
        m_ranged = true;
    }
    ကိန်းပြည့် left() const { ပြန်ပေး m_left; }
    ကိန်းပြည့် right() const { ပြန်ပေး m_right; }
    ကိန်းပြည့် hi() const VL_MT_SAFE {
        ပြန်ပေး m_left > m_right ? m_left : m_right;
    }  // How to show a declaration
    ကိန်းပြည့် lo() const VL_MT_SAFE {
        ပြန်ပေး m_left > m_right ? m_right : m_left;
    }  // How to show a declaration
    ကိန်းပြည့် leftToRightInc() const { ပြန်ပေး ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seqLeftToRight() const { ပြန်ပေး {m_left, m_right, leftToRightInc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seqRightToLeft() const { ပြန်ပေး {m_right, m_left, -leftToRightInc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() const { ပြန်ပေး {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() const { ပြန်ပေး {hi(), lo(), -1}; }
    ကိန်းပြည့် elements() const VL_MT_SAFE { ပြန်ပေး hi() - lo() + 1; }
    bool ranged() const { ပြန်ပေး m_ranged; }
    bool ascending() const { ပြန်ပေး m_left < m_right; }
    ကိန်းပြည့် hiMaxSelect() const {
        ပြန်ပေး (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() const {
        အကယ်၍ (ranged()) {
            ပြန်ပေး "["s + std::to_string(left()) + ":" + std::to_string(right()) + "]";
        } မဟုတ်ပါက {
            ပြန်ပေး "[norg]";
        }
    }
    void dump(std::ostream& str) const { str << ascii(); }
};
anta_hstha std::ostream& operator<<(std::ostream& os, const VNumRange& rhs) {
    rhs.dump(os);
    ပြန်ပေး os;
}

// ######################################################################

class VPragmaType final {
public:
    ga_nanaa en : uint8_t {
        COVERAGE_BLOCK_OFF,
        HIER_BLOCK,
        HIER_PARAMS,
        INLINE_MODULE,
        NO_INLINE_MODULE,
        NO_INLINE_TASK,
        PUBLIC_MODULE,
        PUBLIC_TASK,
        TIMEUNIT_SET,
        UNROLL_DISABLE,
        UNROLL_FULL,
        FULL_CASE,
        PARALLEL_CASE,
        VERILATOR_LIB,
        _ENUM_SIZE
    };
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {
            "COVERAGE_BLOCK_OFF",  //
            "HIER_BLOCK",  //
            "HIER_PARAMS",  //
            "INLINE_MODULE",  //
            "NO_INLINE_MODULE",  //
            "NO_INLINE_TASK",  //
            "PUBLIC_MODULE",  //
            "PUBLIC_TASK",  //
            "TIMEUNIT_SET",  //
            "UNROLL_DISABLE",  //
            "UNROLL_FULL",  //
            "FULL_CASE",  //
            "PARALLEL_CASE",  //
            "VERILATOR_LIB",  //
            "_ENUM_SIZE"  //
        };
        ပြန်ပေး names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
};
constexpr bool operator==(const VPragmaType& lhs, const VPragmaType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VPragmaType& lhs, VPragmaType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, const VPragmaType& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VPragmaType& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
public:
    ga_nanaa en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        ပြန်ပေး names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() const { ပြန်ပေး m_e; }
    bool isRandomizable() const { ပြန်ပေး m_e != NONE; }
    bool isRand() const { ပြန်ပေး m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() const { ပြန်ပေး m_e == RAND_CYCLIC; }
};
anta_hstha std::ostream& operator<<(std::ostream& os, const VRandAttr& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    static const std::shared_ptr<const string> s_emptyp;  // Holds ""
    static const std::shared_ptr<const string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<const string> m_strp;

public:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, const string& field)
        : m_strp{std::make_shared<const string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, const string& field)
        : m_strp{std::make_shared<const string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() const { ပြန်ပေး m_strp == s_emptyp; }
    bool isVlSym() const { ပြန်ပေး m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() const { ပြန်ပေး m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) const;
    static string replaceThis(bool useSelfForThis, const string& text);
    const std::string& asString() const { ပြန်ပေး *m_strp; }
    bool operator==(const VSelfPointerText& other) const { ပြန်ပေး *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
public:
    ga_nanaa en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        ပြန်ပေး names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    static VSigning fromBool(bool isSigned) {  // Factory method
        ပြန်ပေး isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    bool isSigned() const VL_MT_SAFE { ပြန်ပေး m_e == SIGNED; }
    bool isNosign() const VL_MT_SAFE { ပြန်ပေး m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(const VSigning& lhs, const VSigning& rhs) { ပြန်ပေး lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VSigning& lhs, VSigning::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, const VSigning& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VSigning& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

class VStrength final {
public:
    ga_nanaa en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    ga_nanaa en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VStrength& lhs, const VStrength& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VStrength& lhs, VStrength::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, const VStrength& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VStrength& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
public:
    ga_nanaa en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    ga_nanaa en m_e;
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        ပြန်ပေး names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() const { ပြန်ပေး m_e; }
};

//######################################################################

class VTracePrefixType final {
public:
    ga_nanaa en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    ga_nanaa en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VTracePrefixType& lhs, const VTracePrefixType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    ပြန်ပေး lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, const VTracePrefixType& rhs) {
    ပြန်ပေး lhs == rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VTracePrefixType& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

//######################################################################

class VTraceType final {
public:
    ga_nanaa en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    ga_nanaa en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"CONSTANT", "FULL", "CHANGE"};
        ပြန်ပေး names[m_e];
    }
    const var_na* func_prefix() const {
        static constexpr const var_na* const names[] = {"trace_const", "trace_full", "trace_chg"};
        ပြန်ပေး names[m_e];
    }
};
constexpr bool operator==(const VTraceType& lhs, const VTraceType& rhs) {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTraceType& lhs, VTraceType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, const VTraceType& rhs) { ပြန်ပေး lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VTraceType& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

//######################################################################

class VUseType final {
public:
    ga_nanaa en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    ga_nanaa en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    bool containsAny(VUseType other) const { ပြန်ပေး m_e & other.m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        ပြန်ပေး names[m_e - 1];
    }
};
constexpr bool operator==(const VUseType& lhs, const VUseType& rhs) { ပြန်ပေး lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VUseType& lhs, VUseType::en rhs) { ပြန်ပေး lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, const VUseType& rhs) { ပြန်ပေး lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    ပြန်ပေး VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    ပြန်ပေး VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VUseType& rhs) {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

class VVarType final {
public:
    ga_nanaa en : uint8_t {
        UNKNOWN,
        GPARAM,
        LPARAM,
        SPECPARAM,
        GENVAR,
        VAR,  // Reg, integer, logic, etc
        SUPPLY0,
        SUPPLY1,
        WIRE,
        WREAL,
        TRIAND,
        TRIOR,
        TRIWIRE,
        TRI0,
        TRI1,
        PORT,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    ga_nanaa en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(ကိန်းပြည့် _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ပြန်ပေး m_e; }
    const var_na* ascii() const {
        static constexpr const var_na* const names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        ပြန်ပေး names[m_e];
    }
    bool isParam() const { ပြန်ပေး m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() const {
        ပြန်ပေး (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() const {
        ပြန်ပေး (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() const { ပြန်ပေး (m_e == TRIOR); }
    bool isWiredNet() const { ပြန်ပေး (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() const {  // In Verilog, always ok in SystemVerilog
        ပြန်ပေး (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() const {
        ပြန်ပေး (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() const {
        ပြန်ပေး (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() const {
        ပြန်ပေး (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    const var_na* traceSigKind() const {
        // VerilatedTraceSigKind to used in trace signal declaration
        static constexpr const var_na* const lut[] = {
            /* UNKNOWN:      */ "",  // Should not be traced
            /* GPARAM:       */ "PARAMETER",
            /* LPARAM:       */ "PARAMETER",
            /* SPECPARAM:    */ "PARAMETER",
            /* GENVAR:       */ "PARAMETER",
            /* VAR:          */ "VAR",
            /* SUPPLY0:      */ "SUPPLY0",
            /* SUPPLY1:      */ "SUPPLY1",
            /* WIRE:         */ "WIRE",
            /* WREAL:        */ "WIRE",
            /* TRIAND:       */ "TRIAND",
            /* TRIOR:        */ "TRIOR",
            /* TRIWIRE:      */ "TRI",
            /* TRI0:         */ "TRI0",
            /* TRI1:         */ "TRI1",
            /* PORT:         */ "WIRE",
            /* BLOCKTEMP:    */ "VAR",
            /* MODULETEMP:   */ "VAR",
            /* STMTTEMP:     */ "VAR",
            /* XTEMP:        */ "VAR",
            /* IFACEREF:     */ "",  // Should not be traced directly
            /* MEMBER:       */ "VAR",
        };
        ပြန်ပေး lut[m_e];
    }
};
constexpr bool operator==(const VVarType& lhs, const VVarType& rhs) VL_MT_SAFE {
    ပြန်ပေး lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    ပြန်ပေး lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, const VVarType& rhs) VL_MT_SAFE {
    ပြန်ပေး lhs == rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VVarType& rhs) VL_MT_SAFE {
    ပြန်ပေး os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasicTypeKey final {
public:
    const ကိန်းပြည့် m_width;  // From AstNodeDType: Bit width of operation
    const ကိန်းပြည့် m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    const VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    const VSigning m_numeric;  // From AstNodeDType: Node is signed
    const VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(const VBasicTypeKey& rhs) const {
        ပြန်ပေး m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(const VBasicTypeKey& rhs) const {
        အကယ်၍ ((m_width < rhs.m_width)) ပြန်ပေး true;
        အကယ်၍ (!(m_width == rhs.m_width)) ပြန်ပေး false;  // lhs > rhs
        အကယ်၍ ((m_widthMin < rhs.m_widthMin)) ပြန်ပေး true;
        အကယ်၍ (!(m_widthMin == rhs.m_widthMin)) ပြန်ပေး false;  // lhs > rhs
        အကယ်၍ ((m_numeric < rhs.m_numeric)) ပြန်ပေး true;
        အကယ်၍ (!(m_numeric == rhs.m_numeric)) ပြန်ပေး false;  // lhs > rhs
        အကယ်၍ ((m_keyword < rhs.m_keyword)) ပြန်ပေး true;
        အကယ်၍ (!(m_keyword == rhs.m_keyword)) ပြန်ပေး false;  // lhs > rhs
        အကယ်၍ ((m_nrange < rhs.m_nrange)) ပြန်ပေး true;
        အကယ်၍ (!(m_nrange == rhs.m_nrange)) ပြန်ပေး false;  // lhs > rhs
        ပြန်ပေး false;
    }
    VBasicTypeKey(ကိန်းပြည့် width, ကိန်းပြည့် widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  const VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasicTypeKey() = default;
};

#endif  // Guard
