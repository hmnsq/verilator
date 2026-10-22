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
    enum en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    enum en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    const char* ascii() const {
        static const char* const names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        trả_về names[m_e];
    }
    bool isAccept() const { trả_về m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
public:
    enum en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    enum en m_e;
    const char* ascii() const {
        static constexpr const char* const names[] = {"RD", "WR", "RW", "--"};
        trả_về names[m_e];
    }
    const char* arrow() const {
        static constexpr const char* const names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        trả_về names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    VAccess invert() const {
        trả_về (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() const { trả_về m_e == READ; }  // False with READWRITE
    bool isWriteOnly() const { trả_về m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() const { trả_về m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() const { trả_về m_e == WRITE || m_e == READWRITE; }
    bool isRW() const { trả_về m_e == READWRITE; }
};
constexpr bool operator==(const VAccess& lhs, const VAccess& rhs) { trả_về lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VAccess& lhs, VAccess::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, const VAccess& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VAccess& rhs) { trả_về os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
public:
    enum en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    enum en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VAlwaysKwd& lhs, const VAlwaysKwd& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, const VAlwaysKwd& rhs) { trả_về lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
public:
    // IEEE 1800-2023 Table 20-5
    enum en : uint8_t {
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
    enum en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const {
        // IEEE 1800-2023 20.11
        static constexpr const char* const names[] = {"",
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
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VAssertCtlType& lhs, const VAssertCtlType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    trả_về lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, const VAssertCtlType& rhs) {
    trả_về lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
public:
    // IEEE 1800-2023 Table 20-7
    enum en : uint8_t {
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
    enum en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() const {
        std::stringstream types;
        nếu (m_e == INTERNAL)
            types << "INTERNAL ";
        không_thì {
            nếu (m_e & ASSERT) types << "ASSERT ";
            nếu (m_e & COVER) types << "COVER ";
            nếu (m_e & ASSUME) types << "ASSUME ";
            nếu (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            nếu (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            nếu (m_e & INTRINSIC) types << "INTRINSIC ";
            nếu (m_e & RESTRICT) types << "RESTRICT ";
        }
        const string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        trả_về str.substr(0, str.size() - 1);
    }
    constexpr operator en() const { trả_về m_e; }
};
constexpr bool operator==(const VAssertDirectiveType& lhs, const VAssertDirectiveType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    trả_về lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, const VAssertDirectiveType& rhs) {
    trả_về lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    trả_về VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
public:
    // IEEE 1800-2023 Table 20-6
    enum en : uint8_t {
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
    enum en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) const { trả_về m_e & other.m_e; }
    string ascii() const {
        std::stringstream types;
        nếu (m_e == INTERNAL)
            types << "INTERNAL ";
        không_thì {
            nếu (m_e & CONCURRENT) types << "CONCURRENT ";
            nếu (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            nếu (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            nếu (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            nếu (m_e & EXPECT) types << "EXPECT ";
            nếu (m_e & UNIQUE) types << "UNIQUE ";
            nếu (m_e & UNIQUE0) types << "UNIQUE0 ";
            nếu (m_e & PRIORITY) types << "PRIORITY ";
        }
        const string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        trả_về str.substr(0, str.size() - 1);
    }
    constexpr operator en() const { trả_về m_e; }
};
constexpr bool operator==(const VAssertType& lhs, const VAssertType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAssertType& lhs, VAssertType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, const VAssertType& rhs) { trả_về lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    trả_về VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
public:
    enum en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    enum en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    const char* ascii() const {
        static constexpr const char* const names[] = {"default", "weak", "strong"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VPropStrength& lhs, const VPropStrength& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VPropStrength& lhs, VPropStrength::en rhs) {
    trả_về lhs.m_e == rhs;
}
constexpr bool operator!=(const VPropStrength& lhs, VPropStrength::en rhs) {
    trả_về lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
public:
    // clang-format off
    enum en: uint8_t  {
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
    enum en m_e;
    const char* ascii() const {
        // clang-format off
        static constexpr const char* const names[] = {
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
        trả_về names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() const {
        switch (m_e) {
        case DIM_BITS:
        case DIM_BITS_OR_NUMBER:
        case DIM_DIMENSIONS:
        case DIM_HIGH:
        case DIM_INCREMENT:
        case DIM_LEFT:
        case DIM_LOW:
        case DIM_RIGHT:
        case DIM_SIZE:
        case DIM_UNPK_DIMENSIONS:
        case TYPEID:
        case TYPENAME: trả_về true;
        default: trả_về false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
};
constexpr bool operator==(const VAttrType& lhs, const VAttrType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VAttrType& lhs, VAttrType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, const VAttrType& rhs) { trả_về lhs == rhs.m_e; }

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
    bool isAny() const { trả_về m_extends | m_final | m_initial; }
    bool isExtends() const { trả_về m_extends; }
    bool isFinal() const { trả_về m_final; }
    bool isInitial() const { trả_về m_initial; }
    string ascii() const {
        string out;
        nếu (m_initial) out = VString::dot(out, " ", "initial");
        nếu (m_extends) out = VString::dot(out, " ", "extends");
        nếu (m_final) out = VString::dot(out, " ", "final");
        trả_về out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
public:
    enum en : uint8_t {
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
    enum en m_e;
    const char* ascii() const VL_MT_SAFE {
        static constexpr const char* const names[] = {"%E-unk",
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
        trả_về names[m_e];
    }
    const char* typeName() const VL_MT_SAFE {
        switch (m_e) {
        case LOGIC2STATE: trả_về VBasicDTypeKwd{LOGIC}.ascii();
        case INTEGER2STATE: trả_về VBasicDTypeKwd{INTEGER}.ascii();
        case TIME2STATE: trả_về VBasicDTypeKwd{TIME}.ascii();
        default: trả_về ascii();
        }
    }
    const char* dpiType() const {
        static constexpr const char* const names[] = {"%E-unk",
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
        trả_về names[m_e];
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
    explicit VBasicDTypeKwd(số_nguyên _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    số_nguyên width() const {
        switch (m_e) {
        case BIT: trả_về 1;  // scalar, can't bit extract unless ranged
        case BYTE: trả_về 8;
        case CHANDLE: trả_về 64;
        case NULLTYPE: trả_về 1;  // Historical - bit was used instead of NULLTYPE
        case EVENT: trả_về 1;
        case INT: trả_về 32;
        case INTEGER: trả_về 32;
        case INTEGER2STATE: trả_về 32;
        case LOGIC: trả_về 1;  // scalar, can't bit extract unless ranged
        case LOGIC2STATE: trả_về 1;  // scalar, can't bit extract unless ranged
        case LONGINT: trả_về 64;
        case DOUBLE: trả_về 64;  // opaque
        case SHORTINT: trả_về 16;
        case TIME: trả_về 64;
        case TIME2STATE: trả_về 64;
        case STRING: trả_về 64;  // opaque  // Just the pointer, for today
        case SCOPEPTR: trả_về 0;  // opaque
        case CHARPTR: trả_về 0;  // opaque
        case MTASKSTATE: trả_về 0;  // opaque
        case DELAY_SCHEDULER: trả_về 0;  // opaque
        case TRIGGER_SCHEDULER: trả_về 0;  // opaque
        case DYNAMIC_TRIGGER_SCHEDULER: trả_về 0;  // opaque
        case FORK_SYNC: trả_về 0;  // opaque
        case PROCESS_REFERENCE: trả_về 0;  // opaque
        case RANDOM_GENERATOR: trả_về 0;  // opaque
        case RANDOM_STDGENERATOR: trả_về 0;  // opaque
        case COVERGROUP_INSTHANDLE: trả_về 0;  // opaque
        case UINT32: trả_về 32;
        case UINT64: trả_về 64;
        default: trả_về 0;
        }
    }
    bool isSigned() const {
        trả_về m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() const {
        trả_về m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() const {
        trả_về m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() const {  // Otherwise initializes to X
        trả_về (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() const {  // Enum increment supported
        trả_về (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() const { trả_về m_e == BIT; }
    bool isBitLogic() const {  // Bit/logic vector types; can form a packed array
        trả_về (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() const {  // Can add "unsigned" to DPI
        trả_về (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() const {  // Uses standard C layout, for DPI runtime access
        trả_về (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() const VL_MT_SAFE {  // IE not a simple number we can bit optimize
        trả_về (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() const VL_MT_SAFE { trả_về m_e == CHANDLE; }
    bool isDouble() const VL_MT_SAFE { trả_về m_e == DOUBLE; }
    bool isEvent() const { trả_về m_e == EVENT; }
    bool isString() const VL_MT_SAFE { trả_về m_e == STRING; }
    bool isMTaskState() const VL_MT_SAFE { trả_về m_e == MTASKSTATE; }
    bool isSameish(const VBasicDTypeKwd& other) const VL_MT_SAFE {
        switch (other.m_e) {
        case LOGIC:
        case LOGIC2STATE:
        case LOGIC_IMPLICIT: trả_về m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        case INTEGER:
        case INTEGER2STATE: trả_về m_e == INTEGER || m_e == INTEGER2STATE;
        case TIME:
        case TIME2STATE: trả_về m_e == TIME || m_e == TIME2STATE;
        default: trả_về m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() const VL_MT_SAFE {
        switch (m_e) {
        case BIT:
        case BYTE:
        case CHANDLE:
        case INT:
        case INTEGER:
        case INTEGER2STATE:
        case LOGIC:
        case LOGIC2STATE:
        case LONGINT:
        case DOUBLE:
        case SHORTINT:
        case SCOPEPTR:
        case CHARPTR:
        case UINT32:
        case UINT64: trả_về true;
        default: trả_về false;
        }
    }

    const char* traceSigType() const {
        // VerilatedTraceSigType to used in trace signal declaration
        static constexpr const char* const lut[] = {
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
        static_assert(sizeof(lut) / sizeof(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        trả_về lut[m_e];
    }
};
constexpr bool operator==(const VBasicDTypeKwd& lhs, const VBasicDTypeKwd& rhs) VL_MT_SAFE {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    trả_về lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, const VBasicDTypeKwd& rhs) VL_MT_SAFE {
    trả_về lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
public:
    enum en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    enum en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    const char* ascii() const {
        static constexpr const char* const names[] = {"FALSE", "TRUE", "UNK"};
        trả_về names[m_e];
    }
    bool isKnown() const { trả_về m_e != BU_UNKNOWN; }
    bool trueKnown() const { trả_về m_e == BU_TRUE; }
    void setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(const VBoolOrUnknown& lhs, const VBoolOrUnknown& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    trả_về lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, const VBoolOrUnknown& rhs) {
    trả_về lhs == rhs.m_e;
}
inline std::ostream& operator<<(std::ostream& os, const VBoolOrUnknown& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
public:
    enum en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    enum en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    bool unknown() const { trả_về m_e == BP_UNKNOWN; }
    bool likely() const { trả_về m_e == BP_LIKELY; }
    bool unlikely() const { trả_về m_e == BP_UNLIKELY; }
    VBranchPred invert() const {
        nếu (m_e == BP_UNLIKELY) trả_về BP_LIKELY;
        nếu (m_e == BP_LIKELY) trả_về BP_UNLIKELY;
        trả_về m_e;
    }
    const char* ascii() const {
        static constexpr const char* const names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        trả_về names[m_e];
    }
    const char* asciiShort() const {
        static constexpr const char* const names[] = {"", "L", "!L"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VBranchPred& lhs, const VBranchPred& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VBranchPred& lhs, VBranchPred::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, const VBranchPred& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VBranchPred& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
static constexpr bool validateArgsDescriptor(const char* descrp) {
    // Is "TODO"
    nếu (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        trả_về true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    cho (const char* cp = descrp; *cp; ++cp) {
        nếu (*cp == '+') trả_về cp != descrp && !cp[1];
        nếu (*cp != 'r' && *cp != 'w' && *cp != 'm') trả_về false;
    }
    trả_về true;
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
    enum en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    enum en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const VL_PURE {
        static constexpr const char* const values[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        trả_về values[m_e];
    }
    bool isPure() const VL_PURE {
        static constexpr bool values[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        trả_về values[m_e];
    }
    bool isClean() const VL_PURE {
        static constexpr bool values[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        trả_về values[m_e];
    }
    const char* args() const VL_PURE {
        static constexpr const char* const values[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        trả_về values[m_e];
    }
    // Return array method for given name
    static VCMethod arrayMethod(const string& name);
};
constexpr bool operator==(const VCMethod& lhs, const VCMethod& rhs) { trả_về lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VCMethod& lhs, VCMethod::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, const VCMethod& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VCMethod& rhs) {
    trả_về os << rhs.ascii();
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
    enum en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    enum en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    const char* ascii() const VL_PURE {
        static const char* const values[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        trả_về values[m_e];
    }
    bool isPure() const VL_PURE {
        static const bool values[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        trả_về values[m_e];
    }
    const char* args() const VL_PURE {
        static const char* const values[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        trả_về values[m_e];
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
    enum en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

private:
    struct Item final {
        enum en m_e;  // Statement's enum mnemonic, for checking
        const char* m_name;  // Statements name, for debugging
    };
    static Item s_itemData[];

public:
    enum en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const VL_PURE {
        static constexpr const char* const names[] = {"none", "ctor_var_reset_call"};
        trả_về names[m_e];
    }
    bool isNone() const { trả_về m_e == NONE; }
};
constexpr bool operator==(const VCStmtType& lhs, const VCStmtType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VCStmtType& lhs, VCStmtType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, const VCStmtType& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VCStmtType& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
public:
    enum en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    enum en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const VL_PURE {
        static constexpr const char* const names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VCaseType& lhs, const VCaseType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VCaseType& lhs, VCaseType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, const VCaseType& rhs) { trả_về lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
public:
    enum en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    enum en m_e;
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        trả_về names[m_e];
    }
    bool isAssignable() const { trả_về m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
};
constexpr bool operator==(const VCastable& lhs, const VCastable& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VCastable& lhs, VCastable::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, const VCastable& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VCastable& rhs) {
    trả_về os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
public:
    enum en : uint8_t {
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
    enum en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() const { trả_về m_e; }  // LCOV_EXCL_LINE
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        trả_về names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    const char* verilogKwd() const {
        static constexpr const char* const names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        trả_về names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    const char* binSetEnum() const {
        switch (m_e) {
        case BINS_IGNORE: trả_về "VlCovBinKind::KIND_IGNORE";
        case BINS_ILLEGAL: trả_về "VlCovBinKind::KIND_ILLEGAL";
        case BINS_DEFAULT: trả_về "VlCovBinKind::KIND_DEFAULT";
        default: trả_về "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() const {
        trả_về m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(const VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    trả_về lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
public:
    enum en : uint8_t {
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
    enum en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    const char* ascii() const {
        static constexpr const char* const names[] = {"weight",
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
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    trả_về lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
public:
    enum en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    enum en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(số_nguyên _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }  // LCOV_EXCL_LINE
    const char* ascii() const {
        static constexpr const char* const names[] = {"", "[*]", "[->]", "[=]"};
        trả_về names[m_e];
    }
    const char* asciiJson() const {
        static constexpr const char* const names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        trả_về names[m_e];
    }
};

//######################################################################

class VDirection final {
public:
    enum en : uint8_t { NONE, INPUT, OUTPUT, INOUT, REF, CONSTREF };
    enum en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const VL_MT_SAFE { trả_về m_e; }
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        trả_về names[m_e];
    }
    string verilogKwd() const {
        static constexpr const char* const names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        trả_về names[m_e];
    }
    string prettyName() const { trả_về verilogKwd(); }
    bool isAny() const { trả_về m_e != NONE; }
    bool isInout() const { trả_về m_e == INOUT; }
    bool isInoutOrRef() const { trả_về m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool isInput() const { trả_về m_e == INPUT; }
    bool isNonOutput() const {
        trả_về m_e == INPUT || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() const { trả_về m_e == OUTPUT; }
    bool isReadOnly() const VL_MT_SAFE { trả_về m_e == INPUT || m_e == CONSTREF; }
    bool isWritable() const VL_MT_SAFE { trả_về m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() const VL_MT_SAFE { trả_về m_e == REF; }
    bool isConstRef() const VL_MT_SAFE { trả_về m_e == CONSTREF; }
    string traceSigDirection() const {
        nếu (isInout()) {
            trả_về "VerilatedTraceSigDirection::INOUT";
        } không_thì nếu (isWritable()) {
            trả_về "VerilatedTraceSigDirection::OUTPUT";
        } không_thì nếu (isNonOutput()) {
            trả_về "VerilatedTraceSigDirection::INPUT";
        }
        trả_về "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() const {
        switch (m_e) {
        case NONE: trả_về VAccess::NOCHANGE;
        case INPUT:
        case CONSTREF: trả_về VAccess::READ;
        case OUTPUT: trả_về VAccess::WRITE;
        case INOUT:
        case REF: trả_về VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(const VDirection& lhs, const VDirection& rhs) VL_MT_SAFE {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    trả_về lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, const VDirection& rhs) VL_MT_SAFE {
    trả_về lhs == rhs.m_e;
}
inline std::ostream& operator<<(std::ostream& os, const VDirection& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
public:
    enum en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    enum en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    bool addNewline() const { trả_về m_e != DT_WRITE; }
    bool needScopeTracking() const { trả_về m_e != DT_DISPLAY && m_e != DT_WRITE; }
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VDisplayType& lhs, const VDisplayType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VDisplayType& lhs, VDisplayType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, const VDisplayType& rhs) { trả_về lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
public:
    enum en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    enum en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VDumpCtlType& lhs, const VDumpCtlType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VDumpCtlType& lhs, VDumpCtlType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, const VDumpCtlType& rhs) { trả_về lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
public:
    // REMEMBER to edit the strings below too
    enum en : uint8_t {
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
    enum en m_e;
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
        trả_về clocked[m_e];
    }
    bool anEdge() const { trả_về m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() const {
        switch (m_e) {
        case ET_CHANGED: trả_về ET_CHANGED;
        case ET_BOTHEDGE: trả_về ET_BOTHEDGE;
        case ET_POSEDGE: trả_về ET_NEGEDGE;
        case ET_NEGEDGE: trả_về ET_POSEDGE;
        default: UASSERT_STATIC(0, "Inverting bad edgeType()"); trả_về ET_NEGEDGE;
        }
    }
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        trả_về names[m_e];
    }
    const char* verilogKwd() const {
        static constexpr const char* const names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        trả_về names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(const VEdgeType& other) const {
        switch (m_e) {
        case VEdgeType::ET_POSEDGE:
            nếu (other.m_e == VEdgeType::ET_NEGEDGE) trả_về true;
            break;
        case VEdgeType::ET_NEGEDGE:
            nếu (other.m_e == VEdgeType::ET_POSEDGE) trả_về true;
            break;
        default: break;
        }
        trả_về false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
};
constexpr bool operator==(const VEdgeType& lhs, const VEdgeType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VEdgeType& lhs, VEdgeType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, const VEdgeType& rhs) { trả_về lhs == rhs.m_e; }

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
    enum en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    enum en m_e;

    const char* ascii() const {
        static constexpr const char* const values[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        trả_về values[m_e];
    }
    bool isIterated() const {
        static constexpr bool values[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        trả_về values[m_e];
    }
    bool hasTriggers() const {
        static constexpr bool values[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        trả_về values[m_e];
    }
    bool firstIteration() const {
        static constexpr bool values[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        trả_về values[m_e];
    }
    bool slow() const {
        static constexpr bool values[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        trả_về values[m_e];
    }

    // Short name
    std::string tag() const { trả_về VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() const { trả_về "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() const { trả_về "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() const { trả_về "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() const { trả_về "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() const { trả_về m_e; }

private:
    std::string capitalizedTag() const {
        const std::string name = ascii();
        trả_về name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
public:
    enum en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    enum en m_e;
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        trả_về names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    // Is a node type compatible with the declaration
    bool isNodeCompatible(const AstNode* nodep) const;
};
constexpr bool operator==(const VFwdType& lhs, const VFwdType& rhs) { trả_về lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VFwdType& lhs, VFwdType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, const VFwdType& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VFwdType& rhs) {
    trả_về os << rhs.ascii();
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
    bool isCached() const { trả_về m_cachedCnt == s_cachedCntGbl; }
    bool get() const { trả_về m_state; }
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
    enum en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    enum en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const {
        static constexpr const char* const names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        trả_về names[m_e];
    }
    const char* verilogKwd() const {
        static constexpr const char* const names[] = {"join", "join_any", "join_none"};
        trả_về names[m_e];
    }
    bool join() const { trả_về m_e == JOIN; }
    bool joinAny() const { trả_về m_e == JOIN_ANY; }
    bool joinNone() const { trả_về m_e == JOIN_NONE; }
};
constexpr bool operator==(const VJoinType& lhs, const VJoinType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VJoinType& lhs, VJoinType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, const VJoinType& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VJoinType& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
public:
    enum en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    enum en m_e;
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        trả_về names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    bool isNone() const { trả_về m_e == NONE; }
    bool isAutomatic() const { trả_về m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() const { trả_về m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() const { trả_về m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() const {
        switch (m_e) {
        case AUTOMATIC_EXPLICIT: trả_về AUTOMATIC_IMPLICIT;
        case STATIC_EXPLICIT: trả_về STATIC_IMPLICIT;
        default: trả_về m_e;
        }
    }
};
constexpr bool operator==(const VLifetime& lhs, const VLifetime& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VLifetime& lhs, VLifetime::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, const VLifetime& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VLifetime& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
public:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            số_nguyên m_index;  // Current index
            const số_nguyên m_inc;  // Step, +1 or -1
        public:
            Iterator(số_nguyên index, số_nguyên inc)
                : m_index{index}
                , m_inc{inc} {}
            số_nguyên operator*() const { trả_về m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                trả_về *this;
            }
            bool operator!=(const Iterator& that) const { trả_về m_index != that.m_index; }
        };
        const số_nguyên m_begin;  // First index
        const số_nguyên m_end;  // One past the last index
        const số_nguyên m_inc;  // Step, +1 or -1
    public:
        Sequence(số_nguyên first, số_nguyên last, số_nguyên inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() const { trả_về Iterator{m_begin, m_inc}; }
        Iterator end() const { trả_về Iterator{m_end, m_inc}; }
    };

    số_nguyên m_left = 0;  // Left side of range (pre-':')
    số_nguyên m_right = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(const VNumRange& rhs) const {
        trả_về m_left == rhs.m_left && m_right == rhs.m_right && m_ranged == rhs.m_ranged;
    }
    bool operator<(const VNumRange& rhs) const {
        nếu ((m_left < rhs.m_left)) trả_về true;
        nếu (!(m_left == rhs.m_left)) trả_về false;  // lhs > rhs
        nếu ((m_right < rhs.m_right)) trả_về true;
        nếu (!(m_right == rhs.m_right)) trả_về false;  // lhs > rhs
        nếu ((m_ranged < rhs.m_ranged)) trả_về true;
        nếu (!(m_ranged == rhs.m_ranged)) trả_về false;  // lhs > rhs
        trả_về false;
    }
    //
    VNumRange() = default;
    VNumRange(số_nguyên hi, số_nguyên lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(số_nguyên left, số_nguyên right)
        : m_left{left}
        , m_right{right}
        , m_ranged{true} {}
    ~VNumRange() = default;
    // MEMBERS
    void init(số_nguyên hi, số_nguyên lo, bool ascending) {
        nếu (lo > hi) std::swap(hi, lo);
        m_left = ascending ? lo : hi;
        m_right = ascending ? hi : lo;
        m_ranged = true;
    }
    số_nguyên left() const { trả_về m_left; }
    số_nguyên right() const { trả_về m_right; }
    số_nguyên hi() const VL_MT_SAFE {
        trả_về m_left > m_right ? m_left : m_right;
    }  // How to show a declaration
    số_nguyên lo() const VL_MT_SAFE {
        trả_về m_left > m_right ? m_right : m_left;
    }  // How to show a declaration
    số_nguyên leftToRightInc() const { trả_về ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seqLeftToRight() const { trả_về {m_left, m_right, leftToRightInc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seqRightToLeft() const { trả_về {m_right, m_left, -leftToRightInc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() const { trả_về {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() const { trả_về {hi(), lo(), -1}; }
    số_nguyên elements() const VL_MT_SAFE { trả_về hi() - lo() + 1; }
    bool ranged() const { trả_về m_ranged; }
    bool ascending() const { trả_về m_left < m_right; }
    số_nguyên hiMaxSelect() const {
        trả_về (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() const {
        nếu (ranged()) {
            trả_về "["s + std::to_string(left()) + ":" + std::to_string(right()) + "]";
        } không_thì {
            trả_về "[norg]";
        }
    }
    void dump(std::ostream& str) const { str << ascii(); }
};
inline std::ostream& operator<<(std::ostream& os, const VNumRange& rhs) {
    rhs.dump(os);
    trả_về os;
}

// ######################################################################

class VPragmaType final {
public:
    enum en : uint8_t {
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
    enum en m_e;
    const char* ascii() const {
        static constexpr const char* const names[] = {
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
        trả_về names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
};
constexpr bool operator==(const VPragmaType& lhs, const VPragmaType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VPragmaType& lhs, VPragmaType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, const VPragmaType& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VPragmaType& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
public:
    enum en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    enum en m_e;
    const char* ascii() const {
        static constexpr const char* const names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        trả_về names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() const { trả_về m_e; }
    bool isRandomizable() const { trả_về m_e != NONE; }
    bool isRand() const { trả_về m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() const { trả_về m_e == RAND_CYCLIC; }
};
inline std::ostream& operator<<(std::ostream& os, const VRandAttr& rhs) {
    trả_về os << rhs.ascii();
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
    bool isEmpty() const { trả_về m_strp == s_emptyp; }
    bool isVlSym() const { trả_về m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() const { trả_về m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) const;
    static string replaceThis(bool useSelfForThis, const string& text);
    const std::string& asString() const { trả_về *m_strp; }
    bool operator==(const VSelfPointerText& other) const { trả_về *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
public:
    enum en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    enum en m_e;
    const char* ascii() const {
        static constexpr const char* const names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        trả_về names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    static VSigning fromBool(bool isSigned) {  // Factory method
        trả_về isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    bool isSigned() const VL_MT_SAFE { trả_về m_e == SIGNED; }
    bool isNosign() const VL_MT_SAFE { trả_về m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(const VSigning& lhs, const VSigning& rhs) { trả_về lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VSigning& lhs, VSigning::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, const VSigning& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VSigning& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################

class VStrength final {
public:
    enum en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    enum en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VStrength& lhs, const VStrength& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VStrength& lhs, VStrength::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, const VStrength& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VStrength& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
public:
    enum en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    enum en m_e;
    const char* ascii() const {
        static constexpr const char* const names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        trả_về names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() const { trả_về m_e; }
};

//######################################################################

class VTracePrefixType final {
public:
    enum en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    enum en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VTracePrefixType& lhs, const VTracePrefixType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    trả_về lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, const VTracePrefixType& rhs) {
    trả_về lhs == rhs.m_e;
}
inline std::ostream& operator<<(std::ostream& os, const VTracePrefixType& rhs) {
    trả_về os << rhs.ascii();
}

//######################################################################

class VTraceType final {
public:
    enum en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    enum en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const {
        static constexpr const char* const names[] = {"CONSTANT", "FULL", "CHANGE"};
        trả_về names[m_e];
    }
    const char* func_prefix() const {
        static constexpr const char* const names[] = {"trace_const", "trace_full", "trace_chg"};
        trả_về names[m_e];
    }
};
constexpr bool operator==(const VTraceType& lhs, const VTraceType& rhs) {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTraceType& lhs, VTraceType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, const VTraceType& rhs) { trả_về lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VTraceType& rhs) {
    trả_về os << rhs.ascii();
}

//######################################################################

class VUseType final {
public:
    enum en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    enum en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    bool containsAny(VUseType other) const { trả_về m_e & other.m_e; }
    const char* ascii() const {
        static constexpr const char* const names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        trả_về names[m_e - 1];
    }
};
constexpr bool operator==(const VUseType& lhs, const VUseType& rhs) { trả_về lhs.m_e == rhs.m_e; }
constexpr bool operator==(const VUseType& lhs, VUseType::en rhs) { trả_về lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, const VUseType& rhs) { trả_về lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    trả_về VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    trả_về VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
inline std::ostream& operator<<(std::ostream& os, const VUseType& rhs) {
    trả_về os << rhs.ascii();
}

// ######################################################################

class VVarType final {
public:
    enum en : uint8_t {
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
    enum en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(số_nguyên _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { trả_về m_e; }
    const char* ascii() const {
        static constexpr const char* const names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        trả_về names[m_e];
    }
    bool isParam() const { trả_về m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() const {
        trả_về (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() const {
        trả_về (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() const { trả_về (m_e == TRIOR); }
    bool isWiredNet() const { trả_về (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() const {  // In Verilog, always ok in SystemVerilog
        trả_về (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() const {
        trả_về (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() const {
        trả_về (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() const {
        trả_về (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    const char* traceSigKind() const {
        // VerilatedTraceSigKind to used in trace signal declaration
        static constexpr const char* const lut[] = {
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
        trả_về lut[m_e];
    }
};
constexpr bool operator==(const VVarType& lhs, const VVarType& rhs) VL_MT_SAFE {
    trả_về lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    trả_về lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, const VVarType& rhs) VL_MT_SAFE {
    trả_về lhs == rhs.m_e;
}
inline std::ostream& operator<<(std::ostream& os, const VVarType& rhs) VL_MT_SAFE {
    trả_về os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasicTypeKey final {
public:
    const số_nguyên m_width;  // From AstNodeDType: Bit width of operation
    const số_nguyên m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    const VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    const VSigning m_numeric;  // From AstNodeDType: Node is signed
    const VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(const VBasicTypeKey& rhs) const {
        trả_về m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(const VBasicTypeKey& rhs) const {
        nếu ((m_width < rhs.m_width)) trả_về true;
        nếu (!(m_width == rhs.m_width)) trả_về false;  // lhs > rhs
        nếu ((m_widthMin < rhs.m_widthMin)) trả_về true;
        nếu (!(m_widthMin == rhs.m_widthMin)) trả_về false;  // lhs > rhs
        nếu ((m_numeric < rhs.m_numeric)) trả_về true;
        nếu (!(m_numeric == rhs.m_numeric)) trả_về false;  // lhs > rhs
        nếu ((m_keyword < rhs.m_keyword)) trả_về true;
        nếu (!(m_keyword == rhs.m_keyword)) trả_về false;  // lhs > rhs
        nếu ((m_nrange < rhs.m_nrange)) trả_về true;
        nếu (!(m_nrange == rhs.m_nrange)) trả_về false;  // lhs > rhs
        trả_về false;
    }
    VBasicTypeKey(số_nguyên width, số_nguyên widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  const VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasicTypeKey() = default;
};

#endif  // Guard
