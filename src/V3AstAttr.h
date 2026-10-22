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
#અન્તર્ભાવ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
જાહેર:
    // IEEE 1800-2023 16.12.14 property abort operators.
    ગણના en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    ગણના en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર અચલ અક્ષર* અચલ names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        પાછા names[m_e];
    }
    bool isAccept() અચલ { પાછા m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
જાહેર:
    ગણના en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"RD", "WR", "RW", "--"};
        પાછા names[m_e];
    }
    અચલ અક્ષર* arrow() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        પાછા names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    VAccess invert() અચલ {
        પાછા (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() અચલ { પાછા m_e == READ; }  // False with READWRITE
    bool isWriteOnly() અચલ { પાછા m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() અચલ { પાછા m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() અચલ { પાછા m_e == WRITE || m_e == READWRITE; }
    bool isRW() અચલ { પાછા m_e == READWRITE; }
};
constexpr bool operator==(અચલ VAccess& lhs, અચલ VAccess& rhs) { પાછા lhs.m_e == rhs.m_e; }
constexpr bool operator==(અચલ VAccess& lhs, VAccess::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, અચલ VAccess& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VAccess& rhs) { પાછા os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
જાહેર:
    ગણના en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    ગણના en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VAlwaysKwd& lhs, અચલ VAlwaysKwd& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, અચલ VAlwaysKwd& rhs) { પાછા lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
જાહેર:
    // IEEE 1800-2023 Table 20-5
    ગણના en : uint8_t {
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
    ગણના en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        // IEEE 1800-2023 20.11
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"",
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
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VAssertCtlType& lhs, અચલ VAssertCtlType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, અચલ VAssertCtlType& rhs) {
    પાછા lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
જાહેર:
    // IEEE 1800-2023 Table 20-7
    ગણના en : uint8_t {
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
    ગણના en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() અચલ {
        std::stringstream types;
        જો (m_e == INTERNAL)
            types << "INTERNAL ";
        નહીંતર {
            જો (m_e & ASSERT) types << "ASSERT ";
            જો (m_e & COVER) types << "COVER ";
            જો (m_e & ASSUME) types << "ASSUME ";
            જો (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            જો (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            જો (m_e & INTRINSIC) types << "INTRINSIC ";
            જો (m_e & RESTRICT) types << "RESTRICT ";
        }
        અચલ string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        પાછા str.substr(0, str.size() - 1);
    }
    constexpr operator en() અચલ { પાછા m_e; }
};
constexpr bool operator==(અચલ VAssertDirectiveType& lhs, અચલ VAssertDirectiveType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, અચલ VAssertDirectiveType& rhs) {
    પાછા lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    પાછા VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
જાહેર:
    // IEEE 1800-2023 Table 20-6
    ગણના en : uint8_t {
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
    ગણના en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) અચલ { પાછા m_e & other.m_e; }
    string ascii() અચલ {
        std::stringstream types;
        જો (m_e == INTERNAL)
            types << "INTERNAL ";
        નહીંતર {
            જો (m_e & CONCURRENT) types << "CONCURRENT ";
            જો (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            જો (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            જો (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            જો (m_e & EXPECT) types << "EXPECT ";
            જો (m_e & UNIQUE) types << "UNIQUE ";
            જો (m_e & UNIQUE0) types << "UNIQUE0 ";
            જો (m_e & PRIORITY) types << "PRIORITY ";
        }
        અચલ string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        પાછા str.substr(0, str.size() - 1);
    }
    constexpr operator en() અચલ { પાછા m_e; }
};
constexpr bool operator==(અચલ VAssertType& lhs, અચલ VAssertType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VAssertType& lhs, VAssertType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, અચલ VAssertType& rhs) { પાછા lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    પાછા VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
જાહેર:
    ગણના en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    ગણના en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"default", "weak", "strong"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VPropStrength& lhs, અચલ VPropStrength& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VPropStrength& lhs, VPropStrength::en rhs) {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator!=(અચલ VPropStrength& lhs, VPropStrength::en rhs) {
    પાછા lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
જાહેર:
    // clang-format off
    ગણના en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_સંખ્યા,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_ઉપલું,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_ડાબું,                       // V3Width processes
        DIM_નીચલું,                        // V3Width processes
        DIM_જમણું,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_અંક,                       // V3Width processes
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
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        // clang-format off
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {
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
        પાછા names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() અચલ {
        પસંદગી (m_e) {
        સ્થિતિ DIM_BITS:
        સ્થિતિ DIM_BITS_OR_સંખ્યા:
        સ્થિતિ DIM_DIMENSIONS:
        સ્થિતિ DIM_ઉપલું:
        સ્થિતિ DIM_INCREMENT:
        સ્થિતિ DIM_ડાબું:
        સ્થિતિ DIM_નીચલું:
        સ્થિતિ DIM_જમણું:
        સ્થિતિ DIM_SIZE:
        સ્થિતિ DIM_UNPK_DIMENSIONS:
        સ્થિતિ TYPEID:
        સ્થિતિ TYPENAME: પાછા true;
        મૂળભૂત: પાછા false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
};
constexpr bool operator==(અચલ VAttrType& lhs, અચલ VAttrType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VAttrType& lhs, VAttrType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, અચલ VAttrType& rhs) { પાછા lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

જાહેર:
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
    શૂન્ય combine(અચલ VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() અચલ { પાછા m_extends | m_final | m_initial; }
    bool isExtends() અચલ { પાછા m_extends; }
    bool isFinal() અચલ { પાછા m_final; }
    bool isInitial() અચલ { પાછા m_initial; }
    string ascii() અચલ {
        string out;
        જો (m_initial) out = VString::dot(out, " ", "initial");
        જો (m_extends) out = VString::dot(out, " ", "extends");
        જો (m_final) out = VString::dot(out, " ", "final");
        પાછા out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
જાહેર:
    ગણના en : uint8_t {
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
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ VL_MT_SAFE {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"%E-unk",
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
        પાછા names[m_e];
    }
    અચલ અક્ષર* typeName() અચલ VL_MT_SAFE {
        પસંદગી (m_e) {
        સ્થિતિ LOGIC2STATE: પાછા VBasicDTypeKwd{LOGIC}.ascii();
        સ્થિતિ INTEGER2STATE: પાછા VBasicDTypeKwd{INTEGER}.ascii();
        સ્થિતિ TIME2STATE: પાછા VBasicDTypeKwd{TIME}.ascii();
        મૂળભૂત: પાછા ascii();
        }
    }
    અચલ અક્ષર* dpiType() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"%E-unk",
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
        પાછા names[m_e];
    }
    સ્થિર શૂન્ય selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(પૂર્ણાંક _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    પૂર્ણાંક width() અચલ {
        પસંદગી (m_e) {
        સ્થિતિ BIT: પાછા 1;  // scalar, can't bit extract unless ranged
        સ્થિતિ BYTE: પાછા 8;
        સ્થિતિ CHANDLE: પાછા 64;
        સ્થિતિ NULLTYPE: પાછા 1;  // Historical - bit was used instead of NULLTYPE
        સ્થિતિ EVENT: પાછા 1;
        સ્થિતિ INT: પાછા 32;
        સ્થિતિ INTEGER: પાછા 32;
        સ્થિતિ INTEGER2STATE: પાછા 32;
        સ્થિતિ LOGIC: પાછા 1;  // scalar, can't bit extract unless ranged
        સ્થિતિ LOGIC2STATE: પાછા 1;  // scalar, can't bit extract unless ranged
        સ્થિતિ LONGINT: પાછા 64;
        સ્થિતિ DOUBLE: પાછા 64;  // opaque
        સ્થિતિ SHORTINT: પાછા 16;
        સ્થિતિ TIME: પાછા 64;
        સ્થિતિ TIME2STATE: પાછા 64;
        સ્થિતિ STRING: પાછા 64;  // opaque  // Just the pointer, for today
        સ્થિતિ SCOPEPTR: પાછા 0;  // opaque
        સ્થિતિ CHARPTR: પાછા 0;  // opaque
        સ્થિતિ MTASKSTATE: પાછા 0;  // opaque
        સ્થિતિ DELAY_SCHEDULER: પાછા 0;  // opaque
        સ્થિતિ TRIGGER_SCHEDULER: પાછા 0;  // opaque
        સ્થિતિ DYNAMIC_TRIGGER_SCHEDULER: પાછા 0;  // opaque
        સ્થિતિ FORK_SYNC: પાછા 0;  // opaque
        સ્થિતિ PROCESS_REFERENCE: પાછા 0;  // opaque
        સ્થિતિ RANDOM_GENERATOR: પાછા 0;  // opaque
        સ્થિતિ RANDOM_STDGENERATOR: પાછા 0;  // opaque
        સ્થિતિ COVERGROUP_INSTHANDLE: પાછા 0;  // opaque
        સ્થિતિ UINT32: પાછા 32;
        સ્થિતિ UINT64: પાછા 64;
        મૂળભૂત: પાછા 0;
        }
    }
    bool isSigned() અચલ {
        પાછા m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() અચલ {
        પાછા m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() અચલ {
        પાછા m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() અચલ {  // Otherwise initializes to X
        પાછા (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() અચલ {  // Enum increment supported
        પાછા (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() અચલ { પાછા m_e == BIT; }
    bool isBitLogic() અચલ {  // Bit/logic vector types; can form a packed array
        પાછા (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() અચલ {  // Can add "unsigned" to DPI
        પાછા (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() અચલ {  // Uses standard C layout, for DPI runtime access
        પાછા (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() અચલ VL_MT_SAFE {  // IE not a simple number we can bit optimize
        પાછા (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() અચલ VL_MT_SAFE { પાછા m_e == CHANDLE; }
    bool isDouble() અચલ VL_MT_SAFE { પાછા m_e == DOUBLE; }
    bool isEvent() અચલ { પાછા m_e == EVENT; }
    bool isString() અચલ VL_MT_SAFE { પાછા m_e == STRING; }
    bool isMTaskState() અચલ VL_MT_SAFE { પાછા m_e == MTASKSTATE; }
    bool isSameish(અચલ VBasicDTypeKwd& other) અચલ VL_MT_SAFE {
        પસંદગી (other.m_e) {
        સ્થિતિ LOGIC:
        સ્થિતિ LOGIC2STATE:
        સ્થિતિ LOGIC_IMPLICIT: પાછા m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        સ્થિતિ INTEGER:
        સ્થિતિ INTEGER2STATE: પાછા m_e == INTEGER || m_e == INTEGER2STATE;
        સ્થિતિ TIME:
        સ્થિતિ TIME2STATE: પાછા m_e == TIME || m_e == TIME2STATE;
        મૂળભૂત: પાછા m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() અચલ VL_MT_SAFE {
        પસંદગી (m_e) {
        સ્થિતિ BIT:
        સ્થિતિ BYTE:
        સ્થિતિ CHANDLE:
        સ્થિતિ INT:
        સ્થિતિ INTEGER:
        સ્થિતિ INTEGER2STATE:
        સ્થિતિ LOGIC:
        સ્થિતિ LOGIC2STATE:
        સ્થિતિ LONGINT:
        સ્થિતિ DOUBLE:
        સ્થિતિ SHORTINT:
        સ્થિતિ SCOPEPTR:
        સ્થિતિ CHARPTR:
        સ્થિતિ UINT32:
        સ્થિતિ UINT64: પાછા true;
        મૂળભૂત: પાછા false;
        }
    }

    અચલ અક્ષર* traceSigType() અચલ {
        // VerilatedTraceSigType to used in trace signal declaration
        સ્થિર constexpr અચલ અક્ષર* અચલ lut[] = {
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
        static_assert(કદ(lut) / કદ(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        પાછા lut[m_e];
    }
};
constexpr bool operator==(અચલ VBasicDTypeKwd& lhs, અચલ VBasicDTypeKwd& rhs) VL_MT_SAFE {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, અચલ VBasicDTypeKwd& rhs) VL_MT_SAFE {
    પાછા lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
જાહેર:
    ગણના en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    ગણના en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"FALSE", "TRUE", "UNK"};
        પાછા names[m_e];
    }
    bool isKnown() અચલ { પાછા m_e != BU_UNKNOWN; }
    bool trueKnown() અચલ { પાછા m_e == BU_TRUE; }
    શૂન્ય setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(અચલ VBoolOrUnknown& lhs, અચલ VBoolOrUnknown& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, અચલ VBoolOrUnknown& rhs) {
    પાછા lhs == rhs.m_e;
}
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VBoolOrUnknown& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
જાહેર:
    ગણના en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    ગણના en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    bool unknown() અચલ { પાછા m_e == BP_UNKNOWN; }
    bool likely() અચલ { પાછા m_e == BP_LIKELY; }
    bool unlikely() અચલ { પાછા m_e == BP_UNLIKELY; }
    VBranchPred invert() અચલ {
        જો (m_e == BP_UNLIKELY) પાછા BP_LIKELY;
        જો (m_e == BP_LIKELY) પાછા BP_UNLIKELY;
        પાછા m_e;
    }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        પાછા names[m_e];
    }
    અચલ અક્ષર* asciiShort() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"", "L", "!L"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VBranchPred& lhs, અચલ VBranchPred& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VBranchPred& lhs, VBranchPred::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, અચલ VBranchPred& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VBranchPred& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
સ્થિર constexpr bool validateArgsDescriptor(અચલ અક્ષર* descrp) {
    // Is "TODO"
    જો (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        પાછા true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    માટે (અચલ અક્ષર* cp = descrp; *cp; ++cp) {
        જો (*cp == '+') પાછા cp != descrp && !cp[1];
        જો (*cp != 'r' && *cp != 'w' && *cp != 'm') પાછા false;
    }
    પાછા true;
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
    macro(ARRAY_કાઢો_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_કાઢો_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ધકેલો_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ધકેલો_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_કિંમત,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_કિંમત_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_કિંમત_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_કિંમત_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_કિંમત_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_કિંમત_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_કિંમત_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_કિંમત_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_કિંમત_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_કિંમત_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_કિંમત_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_ઉમેરો,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_કાઢો,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_કાઢો_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ધકેલો,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ધકેલો_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    સ્થિર constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    સ્થિર constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

જાહેર:
    ગણના en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    ગણના en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ VL_PURE {
        સ્થિર constexpr અચલ અક્ષર* અચલ કિંમતો[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        પાછા કિંમતો[m_e];
    }
    bool isPure() અચલ VL_PURE {
        સ્થિર constexpr bool કિંમતો[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        પાછા કિંમતો[m_e];
    }
    bool isClean() અચલ VL_PURE {
        સ્થિર constexpr bool કિંમતો[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        પાછા કિંમતો[m_e];
    }
    અચલ અક્ષર* args() અચલ VL_PURE {
        સ્થિર constexpr અચલ અક્ષર* અચલ કિંમતો[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        પાછા કિંમતો[m_e];
    }
    // Return array method for given name
    સ્થિર VCMethod arrayMethod(અચલ string& name);
};
constexpr bool operator==(અચલ VCMethod& lhs, અચલ VCMethod& rhs) { પાછા lhs.m_e == rhs.m_e; }
constexpr bool operator==(અચલ VCMethod& lhs, VCMethod::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, અચલ VCMethod& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VCMethod& rhs) {
    પાછા os << rhs.ascii();
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
    સ્થિર constexpr bool PURE = true;  // For macro expansion of 'pure' field only

જાહેર:
    ગણના en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    ગણના en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    અચલ અક્ષર* ascii() અચલ VL_PURE {
        સ્થિર અચલ અક્ષર* અચલ કિંમતો[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        પાછા કિંમતો[m_e];
    }
    bool isPure() અચલ VL_PURE {
        સ્થિર અચલ bool કિંમતો[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        પાછા કિંમતો[m_e];
    }
    અચલ અક્ષર* args() અચલ VL_PURE {
        સ્થિર અચલ અક્ષર* અચલ કિંમતો[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        પાછા કિંમતો[m_e];
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
જાહેર:
    ગણના en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

ખાનગી:
    સંરચના Item final {
        ગણના en m_e;  // Statement's enum mnemonic, for checking
        અચલ અક્ષર* m_name;  // Statements name, for debugging
    };
    સ્થિર Item s_itemData[];

જાહેર:
    ગણના en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ VL_PURE {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"none", "ctor_var_reset_call"};
        પાછા names[m_e];
    }
    bool isNone() અચલ { પાછા m_e == NONE; }
};
constexpr bool operator==(અચલ VCStmtType& lhs, અચલ VCStmtType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VCStmtType& lhs, VCStmtType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, અચલ VCStmtType& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VCStmtType& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
જાહેર:
    ગણના en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    ગણના en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ VL_PURE {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VCaseType& lhs, અચલ VCaseType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VCaseType& lhs, VCaseType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, અચલ VCaseType& rhs) { પાછા lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
જાહેર:
    ગણના en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        પાછા names[m_e];
    }
    bool isAssignable() અચલ { પાછા m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
};
constexpr bool operator==(અચલ VCastable& lhs, અચલ VCastable& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VCastable& lhs, VCastable::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, અચલ VCastable& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VCastable& rhs) {
    પાછા os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
જાહેર:
    ગણના en : uint8_t {
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
    ગણના en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() અચલ { પાછા m_e; }  // LCOV_EXCL_LINE
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        પાછા names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    અચલ અક્ષર* verilogKwd() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        પાછા names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    અચલ અક્ષર* binSetEnum() અચલ {
        પસંદગી (m_e) {
        સ્થિતિ BINS_IGNORE: પાછા "VlCovBinKind::KIND_IGNORE";
        સ્થિતિ BINS_ILLEGAL: પાછા "VlCovBinKind::KIND_ILLEGAL";
        સ્થિતિ BINS_DEFAULT: પાછા "VlCovBinKind::KIND_DEFAULT";
        મૂળભૂત: પાછા "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() અચલ {
        પાછા m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(અચલ VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    પાછા lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
જાહેર:
    ગણના en : uint8_t {
        // Shared by option.* and type_option.*
        WEIGHT,
        GOAL,
        AT_LEAST,
        AUTO_BIN_MAX,
        PER_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_અંક_PRINT_MISSING,
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
    ગણના en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"weight",
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
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    પાછા lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
જાહેર:
    ગણના en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    ગણના en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(પૂર્ણાંક _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }  // LCOV_EXCL_LINE
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"", "[*]", "[->]", "[=]"};
        પાછા names[m_e];
    }
    અચલ અક્ષર* asciiJson() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        પાછા names[m_e];
    }
};

//######################################################################

class VDirection final {
જાહેર:
    ગણના en : uint8_t { NONE, આગત, OUTPUT, INOUT, REF, CONSTREF };
    ગણના en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ VL_MT_SAFE { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        પાછા names[m_e];
    }
    string verilogKwd() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        પાછા names[m_e];
    }
    string prettyName() અચલ { પાછા verilogKwd(); }
    bool isAny() અચલ { પાછા m_e != NONE; }
    bool isInout() અચલ { પાછા m_e == INOUT; }
    bool isInoutOrRef() અચલ { પાછા m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_આગત() અચલ { પાછા m_e == આગત; }
    bool isNonOutput() અચલ {
        પાછા m_e == આગત || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() અચલ { પાછા m_e == OUTPUT; }
    bool isReadOnly() અચલ VL_MT_SAFE { પાછા m_e == આગત || m_e == CONSTREF; }
    bool isWritable() અચલ VL_MT_SAFE { પાછા m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() અચલ VL_MT_SAFE { પાછા m_e == REF; }
    bool isConstRef() અચલ VL_MT_SAFE { પાછા m_e == CONSTREF; }
    string traceSigDirection() અચલ {
        જો (isInout()) {
            પાછા "VerilatedTraceSigDirection::INOUT";
        } નહીંતર જો (isWritable()) {
            પાછા "VerilatedTraceSigDirection::OUTPUT";
        } નહીંતર જો (isNonOutput()) {
            પાછા "VerilatedTraceSigDirection::INPUT";
        }
        પાછા "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() અચલ {
        પસંદગી (m_e) {
        સ્થિતિ NONE: પાછા VAccess::NOCHANGE;
        સ્થિતિ આગત:
        સ્થિતિ CONSTREF: પાછા VAccess::READ;
        સ્થિતિ OUTPUT: પાછા VAccess::WRITE;
        સ્થિતિ INOUT:
        સ્થિતિ REF: પાછા VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(અચલ VDirection& lhs, અચલ VDirection& rhs) VL_MT_SAFE {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, અચલ VDirection& rhs) VL_MT_SAFE {
    પાછા lhs == rhs.m_e;
}
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VDirection& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
જાહેર:
    ગણના en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    ગણના en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    bool addNewline() અચલ { પાછા m_e != DT_WRITE; }
    bool needScopeTracking() અચલ { પાછા m_e != DT_DISPLAY && m_e != DT_WRITE; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VDisplayType& lhs, અચલ VDisplayType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VDisplayType& lhs, VDisplayType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, અચલ VDisplayType& rhs) { પાછા lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
જાહેર:
    ગણના en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    ગણના en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VDumpCtlType& lhs, અચલ VDumpCtlType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VDumpCtlType& lhs, VDumpCtlType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, અચલ VDumpCtlType& rhs) { પાછા lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
જાહેર:
    // REMEMBER to edit the strings below too
    ગણના en : uint8_t {
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
    ગણના en m_e;
    bool clockedStmt() અચલ {
        સ્થિર constexpr bool clocked[] = {
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
        પાછા clocked[m_e];
    }
    bool anEdge() અચલ { પાછા m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() અચલ {
        પસંદગી (m_e) {
        સ્થિતિ ET_CHANGED: પાછા ET_CHANGED;
        સ્થિતિ ET_BOTHEDGE: પાછા ET_BOTHEDGE;
        સ્થિતિ ET_POSEDGE: પાછા ET_NEGEDGE;
        સ્થિતિ ET_NEGEDGE: પાછા ET_POSEDGE;
        મૂળભૂત: UASSERT_STATIC(0, "Inverting bad edgeType()"); પાછા ET_NEGEDGE;
        }
    }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        પાછા names[m_e];
    }
    અચલ અક્ષર* verilogKwd() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        પાછા names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(અચલ VEdgeType& other) અચલ {
        પસંદગી (m_e) {
        સ્થિતિ VEdgeType::ET_POSEDGE:
            જો (other.m_e == VEdgeType::ET_NEGEDGE) પાછા true;
            તોડો;
        સ્થિતિ VEdgeType::ET_NEGEDGE:
            જો (other.m_e == VEdgeType::ET_POSEDGE) પાછા true;
            તોડો;
        મૂળભૂત: તોડો;
        }
        પાછા false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
};
constexpr bool operator==(અચલ VEdgeType& lhs, અચલ VEdgeType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VEdgeType& lhs, VEdgeType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, અચલ VEdgeType& rhs) { પાછા lhs == rhs.m_e; }

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
જાહેર:
    ગણના en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    ગણના en m_e;

    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ કિંમતો[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        પાછા કિંમતો[m_e];
    }
    bool isIterated() અચલ {
        સ્થિર constexpr bool કિંમતો[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        પાછા કિંમતો[m_e];
    }
    bool hasTriggers() અચલ {
        સ્થિર constexpr bool કિંમતો[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        પાછા કિંમતો[m_e];
    }
    bool firstIteration() અચલ {
        સ્થિર constexpr bool કિંમતો[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        પાછા કિંમતો[m_e];
    }
    bool slow() અચલ {
        સ્થિર constexpr bool કિંમતો[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        પાછા કિંમતો[m_e];
    }

    // Short name
    std::string tag() અચલ { પાછા VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() અચલ { પાછા "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() અચલ { પાછા "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() અચલ { પાછા "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() અચલ { પાછા "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() અચલ { પાછા m_e; }

ખાનગી:
    std::string capitalizedTag() અચલ {
        અચલ std::string name = ascii();
        પાછા name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
જાહેર:
    ગણના en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        પાછા names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    // Is a node type compatible with the declaration
    bool is_ગાંઠ_Compatible(અચલ Ast_ગાંઠ* nodep) અચલ;
};
constexpr bool operator==(અચલ VFwdType& lhs, અચલ VFwdType& rhs) { પાછા lhs.m_e == rhs.m_e; }
constexpr bool operator==(અચલ VFwdType& lhs, VFwdType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, અચલ VFwdType& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VFwdType& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    સ્થિર uint64_t s_cachedCntGbl;  // Global computed count

જાહેર:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() અચલ { પાછા m_cachedCnt == s_cachedCntGbl; }
    bool get() અચલ { પાછા m_state; }
    શૂન્ય set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    શૂન્ય clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    સ્થિર શૂન્ય clear_Cache_વૃક્ષ() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
જાહેર:
    ગણના en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    ગણના en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        પાછા names[m_e];
    }
    અચલ અક્ષર* verilogKwd() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"join", "join_any", "join_none"};
        પાછા names[m_e];
    }
    bool join() અચલ { પાછા m_e == JOIN; }
    bool joinAny() અચલ { પાછા m_e == JOIN_ANY; }
    bool joinNone() અચલ { પાછા m_e == JOIN_NONE; }
};
constexpr bool operator==(અચલ VJoinType& lhs, અચલ VJoinType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VJoinType& lhs, VJoinType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, અચલ VJoinType& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VJoinType& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
જાહેર:
    ગણના en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        પાછા names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    bool isNone() અચલ { પાછા m_e == NONE; }
    bool isAutomatic() અચલ { પાછા m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() અચલ { પાછા m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() અચલ { પાછા m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() અચલ {
        પસંદગી (m_e) {
        સ્થિતિ AUTOMATIC_EXPLICIT: પાછા AUTOMATIC_IMPLICIT;
        સ્થિતિ STATIC_EXPLICIT: પાછા STATIC_IMPLICIT;
        મૂળભૂત: પાછા m_e;
        }
    }
};
constexpr bool operator==(અચલ VLifetime& lhs, અચલ VLifetime& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VLifetime& lhs, VLifetime::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, અચલ VLifetime& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VLifetime& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
જાહેર:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            પૂર્ણાંક m_index;  // Current index
            અચલ પૂર્ણાંક m_inc;  // Step, +1 or -1
        જાહેર:
            Iterator(પૂર્ણાંક index, પૂર્ણાંક inc)
                : m_index{index}
                , m_inc{inc} {}
            પૂર્ણાંક operator*() અચલ { પાછા m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                પાછા *this;
            }
            bool operator!=(અચલ Iterator& that) અચલ { પાછા m_index != that.m_index; }
        };
        અચલ પૂર્ણાંક m_begin;  // First index
        અચલ પૂર્ણાંક m_end;  // One past the last index
        અચલ પૂર્ણાંક m_inc;  // Step, +1 or -1
    જાહેર:
        Sequence(પૂર્ણાંક first, પૂર્ણાંક last, પૂર્ણાંક inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() અચલ { પાછા Iterator{m_begin, m_inc}; }
        Iterator end() અચલ { પાછા Iterator{m_end, m_inc}; }
    };

    પૂર્ણાંક m_ડાબું = 0;  // Left side of range (pre-':')
    પૂર્ણાંક m_જમણું = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(અચલ VNumRange& rhs) અચલ {
        પાછા m_ડાબું == rhs.m_ડાબું && m_જમણું == rhs.m_જમણું && m_ranged == rhs.m_ranged;
    }
    bool operator<(અચલ VNumRange& rhs) અચલ {
        જો ((m_ડાબું < rhs.m_ડાબું)) પાછા true;
        જો (!(m_ડાબું == rhs.m_ડાબું)) પાછા false;  // lhs > rhs
        જો ((m_જમણું < rhs.m_જમણું)) પાછા true;
        જો (!(m_જમણું == rhs.m_જમણું)) પાછા false;  // lhs > rhs
        જો ((m_ranged < rhs.m_ranged)) પાછા true;
        જો (!(m_ranged == rhs.m_ranged)) પાછા false;  // lhs > rhs
        પાછા false;
    }
    //
    VNumRange() = મૂળભૂત;
    VNumRange(પૂર્ણાંક hi, પૂર્ણાંક lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(પૂર્ણાંક ડાબું, પૂર્ણાંક જમણું)
        : m_ડાબું{ડાબું}
        , m_જમણું{જમણું}
        , m_ranged{true} {}
    ~VNumRange() = મૂળભૂત;
    // MEMBERS
    શૂન્ય init(પૂર્ણાંક hi, પૂર્ણાંક lo, bool ascending) {
        જો (lo > hi) std::swap(hi, lo);
        m_ડાબું = ascending ? lo : hi;
        m_જમણું = ascending ? hi : lo;
        m_ranged = true;
    }
    પૂર્ણાંક ડાબું() અચલ { પાછા m_ડાબું; }
    પૂર્ણાંક જમણું() અચલ { પાછા m_જમણું; }
    પૂર્ણાંક hi() અચલ VL_MT_SAFE {
        પાછા m_ડાબું > m_જમણું ? m_ડાબું : m_જમણું;
    }  // How to show a declaration
    પૂર્ણાંક lo() અચલ VL_MT_SAFE {
        પાછા m_ડાબું > m_જમણું ? m_જમણું : m_ડાબું;
    }  // How to show a declaration
    પૂર્ણાંક ડાબું_To_જમણું_Inc() અચલ { પાછા ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_ડાબું_To_જમણું() અચલ { પાછા {m_ડાબું, m_જમણું, ડાબું_To_જમણું_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_જમણું_To_ડાબું() અચલ { પાછા {m_જમણું, m_ડાબું, -ડાબું_To_જમણું_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() અચલ { પાછા {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() અચલ { પાછા {hi(), lo(), -1}; }
    પૂર્ણાંક elements() અચલ VL_MT_SAFE { પાછા hi() - lo() + 1; }
    bool ranged() અચલ { પાછા m_ranged; }
    bool ascending() અચલ { પાછા m_ડાબું < m_જમણું; }
    પૂર્ણાંક hiMaxSelect() અચલ {
        પાછા (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() અચલ {
        જો (ranged()) {
            પાછા "["s + std::to_string(ડાબું()) + ":" + std::to_string(જમણું()) + "]";
        } નહીંતર {
            પાછા "[norg]";
        }
    }
    શૂન્ય dump(std::ostream& str) અચલ { str << ascii(); }
};
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VNumRange& rhs) {
    rhs.dump(os);
    પાછા os;
}

// ######################################################################

class VPragmaType final {
જાહેર:
    ગણના en : uint8_t {
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
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {
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
        પાછા names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
};
constexpr bool operator==(અચલ VPragmaType& lhs, અચલ VPragmaType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VPragmaType& lhs, VPragmaType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, અચલ VPragmaType& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VPragmaType& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
જાહેર:
    ગણના en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        પાછા names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() અચલ { પાછા m_e; }
    bool isRandomizable() અચલ { પાછા m_e != NONE; }
    bool isRand() અચલ { પાછા m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() અચલ { પાછા m_e == RAND_CYCLIC; }
};
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VRandAttr& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    સ્થિર અચલ std::shared_ptr<અચલ string> s_emptyp;  // Holds ""
    સ્થિર અચલ std::shared_ptr<અચલ string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<અચલ string> m_strp;

જાહેર:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, અચલ string& field)
        : m_strp{std::make_shared<અચલ string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, અચલ string& field)
        : m_strp{std::make_shared<અચલ string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() અચલ { પાછા m_strp == s_emptyp; }
    bool isVlSym() અચલ { પાછા m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() અચલ { પાછા m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) અચલ;
    સ્થિર string replaceThis(bool useSelfForThis, અચલ string& text);
    અચલ std::string& asString() અચલ { પાછા *m_strp; }
    bool operator==(અચલ VSelfPointerText& other) અચલ { પાછા *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
જાહેર:
    ગણના en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        પાછા names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    સ્થિર VSigning fromBool(bool isSigned) {  // Factory method
        પાછા isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    bool isSigned() અચલ VL_MT_SAFE { પાછા m_e == SIGNED; }
    bool isNosign() અચલ VL_MT_SAFE { પાછા m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(અચલ VSigning& lhs, અચલ VSigning& rhs) { પાછા lhs.m_e == rhs.m_e; }
constexpr bool operator==(અચલ VSigning& lhs, VSigning::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, અચલ VSigning& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VSigning& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

class VStrength final {
જાહેર:
    ગણના en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    ગણના en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VStrength& lhs, અચલ VStrength& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VStrength& lhs, VStrength::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, અચલ VStrength& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VStrength& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
જાહેર:
    ગણના en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    ગણના en m_e;
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        પાછા names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() અચલ { પાછા m_e; }
};

//######################################################################

class VTracePrefixType final {
જાહેર:
    ગણના en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    ગણના en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VTracePrefixType& lhs, અચલ VTracePrefixType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, અચલ VTracePrefixType& rhs) {
    પાછા lhs == rhs.m_e;
}
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VTracePrefixType& rhs) {
    પાછા os << rhs.ascii();
}

//######################################################################

class VTraceType final {
જાહેર:
    ગણના en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    ગણના en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"CONSTANT", "FULL", "CHANGE"};
        પાછા names[m_e];
    }
    અચલ અક્ષર* func_prefix() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"trace_const", "trace_full", "trace_chg"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VTraceType& lhs, અચલ VTraceType& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VTraceType& lhs, VTraceType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, અચલ VTraceType& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VTraceType& rhs) {
    પાછા os << rhs.ascii();
}

//######################################################################

class VUseType final {
જાહેર:
    ગણના en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    ગણના en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    bool containsAny(VUseType other) અચલ { પાછા m_e & other.m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        પાછા names[m_e - 1];
    }
};
constexpr bool operator==(અચલ VUseType& lhs, અચલ VUseType& rhs) { પાછા lhs.m_e == rhs.m_e; }
constexpr bool operator==(અચલ VUseType& lhs, VUseType::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, અચલ VUseType& rhs) { પાછા lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    પાછા VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    પાછા VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VUseType& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

class VVarType final {
જાહેર:
    ગણના en : uint8_t {
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
    ગણના en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર constexpr અચલ અક્ષર* અચલ names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        પાછા names[m_e];
    }
    bool isParam() અચલ { પાછા m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() અચલ {
        પાછા (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() અચલ {
        પાછા (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() અચલ { પાછા (m_e == TRIOR); }
    bool isWiredNet() અચલ { પાછા (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() અચલ {  // In Verilog, always ok in SystemVerilog
        પાછા (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() અચલ {
        પાછા (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() અચલ {
        પાછા (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() અચલ {
        પાછા (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    અચલ અક્ષર* traceSigKind() અચલ {
        // VerilatedTraceSigKind to used in trace signal declaration
        સ્થિર constexpr અચલ અક્ષર* અચલ lut[] = {
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
        પાછા lut[m_e];
    }
};
constexpr bool operator==(અચલ VVarType& lhs, અચલ VVarType& rhs) VL_MT_SAFE {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, અચલ VVarType& rhs) VL_MT_SAFE {
    પાછા lhs == rhs.m_e;
}
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VVarType& rhs) VL_MT_SAFE {
    પાછા os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_ચાવી final {
જાહેર:
    અચલ પૂર્ણાંક m_width;  // From AstNodeDType: Bit width of operation
    અચલ પૂર્ણાંક m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    અચલ VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    અચલ VSigning m_numeric;  // From AstNodeDType: Node is signed
    અચલ VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(અચલ VBasic_Type_ચાવી& rhs) અચલ {
        પાછા m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(અચલ VBasic_Type_ચાવી& rhs) અચલ {
        જો ((m_width < rhs.m_width)) પાછા true;
        જો (!(m_width == rhs.m_width)) પાછા false;  // lhs > rhs
        જો ((m_widthMin < rhs.m_widthMin)) પાછા true;
        જો (!(m_widthMin == rhs.m_widthMin)) પાછા false;  // lhs > rhs
        જો ((m_numeric < rhs.m_numeric)) પાછા true;
        જો (!(m_numeric == rhs.m_numeric)) પાછા false;  // lhs > rhs
        જો ((m_keyword < rhs.m_keyword)) પાછા true;
        જો (!(m_keyword == rhs.m_keyword)) પાછા false;  // lhs > rhs
        જો ((m_nrange < rhs.m_nrange)) પાછા true;
        જો (!(m_nrange == rhs.m_nrange)) પાછા false;  // lhs > rhs
        પાછા false;
    }
    VBasic_Type_ચાવી(પૂર્ણાંક width, પૂર્ણાંક widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  અચલ VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_ચાવી() = મૂળભૂત;
};

#endif  // Guard
