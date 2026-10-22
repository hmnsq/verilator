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
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
عمومی:
    // IEEE 1800-2023 16.12.14 property abort operators.
    شمارش en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    شمارش en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    ثابت نویسه* ascii() ثابت {
        ایستا ثابت نویسه* ثابت names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        بازگشت names[m_e];
    }
    bool isAccept() ثابت { بازگشت m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
عمومی:
    شمارش en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"RD", "WR", "RW", "--"};
        بازگشت names[m_e];
    }
    ثابت نویسه* arrow() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        بازگشت names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    VAccess invert() ثابت {
        بازگشت (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ثابت { بازگشت m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ثابت { بازگشت m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ثابت { بازگشت m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ثابت { بازگشت m_e == WRITE || m_e == READWRITE; }
    bool isRW() ثابت { بازگشت m_e == READWRITE; }
};
constexpr bool operator==(ثابت VAccess& lhs, ثابت VAccess& rhs) { بازگشت lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VAccess& lhs, VAccess::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, ثابت VAccess& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VAccess& rhs) { بازگشت os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
عمومی:
    شمارش en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    شمارش en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VAlwaysKwd& lhs, ثابت VAlwaysKwd& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, ثابت VAlwaysKwd& rhs) { بازگشت lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
عمومی:
    // IEEE 1800-2023 Table 20-5
    شمارش en : uint8_t {
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
    شمارش en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        // IEEE 1800-2023 20.11
        ایستا constexpr ثابت نویسه* ثابت names[] = {"",
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
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VAssertCtlType& lhs, ثابت VAssertCtlType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, ثابت VAssertCtlType& rhs) {
    بازگشت lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
عمومی:
    // IEEE 1800-2023 Table 20-7
    شمارش en : uint8_t {
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
    شمارش en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ثابت {
        std::stringstream types;
        اگر (m_e == INTERNAL)
            types << "INTERNAL ";
        وگرنه {
            اگر (m_e & ASSERT) types << "ASSERT ";
            اگر (m_e & COVER) types << "COVER ";
            اگر (m_e & ASSUME) types << "ASSUME ";
            اگر (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            اگر (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            اگر (m_e & INTRINSIC) types << "INTRINSIC ";
            اگر (m_e & RESTRICT) types << "RESTRICT ";
        }
        ثابت string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        بازگشت str.substr(0, str.size() - 1);
    }
    constexpr operator en() ثابت { بازگشت m_e; }
};
constexpr bool operator==(ثابت VAssertDirectiveType& lhs, ثابت VAssertDirectiveType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, ثابت VAssertDirectiveType& rhs) {
    بازگشت lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    بازگشت VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
عمومی:
    // IEEE 1800-2023 Table 20-6
    شمارش en : uint8_t {
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
    شمارش en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) ثابت { بازگشت m_e & other.m_e; }
    string ascii() ثابت {
        std::stringstream types;
        اگر (m_e == INTERNAL)
            types << "INTERNAL ";
        وگرنه {
            اگر (m_e & CONCURRENT) types << "CONCURRENT ";
            اگر (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            اگر (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            اگر (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            اگر (m_e & EXPECT) types << "EXPECT ";
            اگر (m_e & UNIQUE) types << "UNIQUE ";
            اگر (m_e & UNIQUE0) types << "UNIQUE0 ";
            اگر (m_e & PRIORITY) types << "PRIORITY ";
        }
        ثابت string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        بازگشت str.substr(0, str.size() - 1);
    }
    constexpr operator en() ثابت { بازگشت m_e; }
};
constexpr bool operator==(ثابت VAssertType& lhs, ثابت VAssertType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAssertType& lhs, VAssertType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, ثابت VAssertType& rhs) { بازگشت lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    بازگشت VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
عمومی:
    شمارش en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    شمارش en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"default", "weak", "strong"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VPropStrength& lhs, ثابت VPropStrength& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator!=(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    بازگشت lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
عمومی:
    // clang-format off
    شمارش en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_عدد,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_بالا,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_چپ,                       // V3Width processes
        DIM_کم,                        // V3Width processes
        DIM_راست,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_رقم,                       // V3Width processes
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
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        // clang-format off
        ایستا constexpr ثابت نویسه* ثابت names[] = {
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
        بازگشت names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ثابت {
        انتخاب (m_e) {
        حالت DIM_BITS:
        حالت DIM_BITS_OR_عدد:
        حالت DIM_DIMENSIONS:
        حالت DIM_بالا:
        حالت DIM_INCREMENT:
        حالت DIM_چپ:
        حالت DIM_کم:
        حالت DIM_راست:
        حالت DIM_SIZE:
        حالت DIM_UNPK_DIMENSIONS:
        حالت TYPEID:
        حالت TYPENAME: بازگشت true;
        پیشفرض: بازگشت false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
};
constexpr bool operator==(ثابت VAttrType& lhs, ثابت VAttrType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAttrType& lhs, VAttrType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, ثابت VAttrType& rhs) { بازگشت lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

عمومی:
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
    تهی combine(ثابت VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ثابت { بازگشت m_extends | m_final | m_initial; }
    bool isExtends() ثابت { بازگشت m_extends; }
    bool isFinal() ثابت { بازگشت m_final; }
    bool isInitial() ثابت { بازگشت m_initial; }
    string ascii() ثابت {
        string out;
        اگر (m_initial) out = VString::dot(out, " ", "initial");
        اگر (m_extends) out = VString::dot(out, " ", "extends");
        اگر (m_final) out = VString::dot(out, " ", "final");
        بازگشت out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
عمومی:
    شمارش en : uint8_t {
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
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت VL_MT_SAFE {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"%E-unk",
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
        بازگشت names[m_e];
    }
    ثابت نویسه* typeName() ثابت VL_MT_SAFE {
        انتخاب (m_e) {
        حالت LOGIC2STATE: بازگشت VBasicDTypeKwd{LOGIC}.ascii();
        حالت INTEGER2STATE: بازگشت VBasicDTypeKwd{INTEGER}.ascii();
        حالت TIME2STATE: بازگشت VBasicDTypeKwd{TIME}.ascii();
        پیشفرض: بازگشت ascii();
        }
    }
    ثابت نویسه* dpiType() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"%E-unk",
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
        بازگشت names[m_e];
    }
    ایستا تهی selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(صحیح _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    صحیح width() ثابت {
        انتخاب (m_e) {
        حالت BIT: بازگشت 1;  // scalar, can't bit extract unless ranged
        حالت BYTE: بازگشت 8;
        حالت CHANDLE: بازگشت 64;
        حالت NULLTYPE: بازگشت 1;  // Historical - bit was used instead of NULLTYPE
        حالت EVENT: بازگشت 1;
        حالت INT: بازگشت 32;
        حالت INTEGER: بازگشت 32;
        حالت INTEGER2STATE: بازگشت 32;
        حالت LOGIC: بازگشت 1;  // scalar, can't bit extract unless ranged
        حالت LOGIC2STATE: بازگشت 1;  // scalar, can't bit extract unless ranged
        حالت LONGINT: بازگشت 64;
        حالت DOUBLE: بازگشت 64;  // opaque
        حالت SHORTINT: بازگشت 16;
        حالت TIME: بازگشت 64;
        حالت TIME2STATE: بازگشت 64;
        حالت STRING: بازگشت 64;  // opaque  // Just the pointer, for today
        حالت SCOPEPTR: بازگشت 0;  // opaque
        حالت CHARPTR: بازگشت 0;  // opaque
        حالت MTASKSTATE: بازگشت 0;  // opaque
        حالت DELAY_SCHEDULER: بازگشت 0;  // opaque
        حالت TRIGGER_SCHEDULER: بازگشت 0;  // opaque
        حالت DYNAMIC_TRIGGER_SCHEDULER: بازگشت 0;  // opaque
        حالت FORK_SYNC: بازگشت 0;  // opaque
        حالت PROCESS_REFERENCE: بازگشت 0;  // opaque
        حالت RANDOM_GENERATOR: بازگشت 0;  // opaque
        حالت RANDOM_STDGENERATOR: بازگشت 0;  // opaque
        حالت COVERGROUP_INSTHANDLE: بازگشت 0;  // opaque
        حالت UINT32: بازگشت 32;
        حالت UINT64: بازگشت 64;
        پیشفرض: بازگشت 0;
        }
    }
    bool isSigned() ثابت {
        بازگشت m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ثابت {
        بازگشت m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() ثابت {
        بازگشت m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() ثابت {  // Otherwise initializes to X
        بازگشت (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ثابت {  // Enum increment supported
        بازگشت (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() ثابت { بازگشت m_e == BIT; }
    bool isBitLogic() ثابت {  // Bit/logic vector types; can form a packed array
        بازگشت (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() ثابت {  // Can add "unsigned" to DPI
        بازگشت (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ثابت {  // Uses standard C layout, for DPI runtime access
        بازگشت (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ثابت VL_MT_SAFE {  // IE not a simple number we can bit optimize
        بازگشت (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ثابت VL_MT_SAFE { بازگشت m_e == CHANDLE; }
    bool isDouble() ثابت VL_MT_SAFE { بازگشت m_e == DOUBLE; }
    bool isEvent() ثابت { بازگشت m_e == EVENT; }
    bool isString() ثابت VL_MT_SAFE { بازگشت m_e == STRING; }
    bool isMTaskState() ثابت VL_MT_SAFE { بازگشت m_e == MTASKSTATE; }
    bool isSameish(ثابت VBasicDTypeKwd& other) ثابت VL_MT_SAFE {
        انتخاب (other.m_e) {
        حالت LOGIC:
        حالت LOGIC2STATE:
        حالت LOGIC_IMPLICIT: بازگشت m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        حالت INTEGER:
        حالت INTEGER2STATE: بازگشت m_e == INTEGER || m_e == INTEGER2STATE;
        حالت TIME:
        حالت TIME2STATE: بازگشت m_e == TIME || m_e == TIME2STATE;
        پیشفرض: بازگشت m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_SAFE {
        انتخاب (m_e) {
        حالت BIT:
        حالت BYTE:
        حالت CHANDLE:
        حالت INT:
        حالت INTEGER:
        حالت INTEGER2STATE:
        حالت LOGIC:
        حالت LOGIC2STATE:
        حالت LONGINT:
        حالت DOUBLE:
        حالت SHORTINT:
        حالت SCOPEPTR:
        حالت CHARPTR:
        حالت UINT32:
        حالت UINT64: بازگشت true;
        پیشفرض: بازگشت false;
        }
    }

    ثابت نویسه* traceSigType() ثابت {
        // VerilatedTraceSigType to used in trace signal declaration
        ایستا constexpr ثابت نویسه* ثابت lut[] = {
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
        static_assert(اندازه(lut) / اندازه(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        بازگشت lut[m_e];
    }
};
constexpr bool operator==(ثابت VBasicDTypeKwd& lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_SAFE {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_SAFE {
    بازگشت lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
عمومی:
    شمارش en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    شمارش en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"FALSE", "TRUE", "UNK"};
        بازگشت names[m_e];
    }
    bool isKnown() ثابت { بازگشت m_e != BU_UNKNOWN; }
    bool trueKnown() ثابت { بازگشت m_e == BU_TRUE; }
    تهی setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(ثابت VBoolOrUnknown& lhs, ثابت VBoolOrUnknown& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, ثابت VBoolOrUnknown& rhs) {
    بازگشت lhs == rhs.m_e;
}
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VBoolOrUnknown& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
عمومی:
    شمارش en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    شمارش en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    bool unknown() ثابت { بازگشت m_e == BP_UNKNOWN; }
    bool likely() ثابت { بازگشت m_e == BP_LIKELY; }
    bool unlikely() ثابت { بازگشت m_e == BP_UNLIKELY; }
    VBranchPred invert() ثابت {
        اگر (m_e == BP_UNLIKELY) بازگشت BP_LIKELY;
        اگر (m_e == BP_LIKELY) بازگشت BP_UNLIKELY;
        بازگشت m_e;
    }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        بازگشت names[m_e];
    }
    ثابت نویسه* asciiShort() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"", "L", "!L"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VBranchPred& lhs, ثابت VBranchPred& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VBranchPred& lhs, VBranchPred::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, ثابت VBranchPred& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VBranchPred& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ایستا constexpr bool validateArgsDescriptor(ثابت نویسه* descrp) {
    // Is "TODO"
    اگر (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        بازگشت true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    برای (ثابت نویسه* cp = descrp; *cp; ++cp) {
        اگر (*cp == '+') بازگشت cp != descrp && !cp[1];
        اگر (*cp != 'r' && *cp != 'w' && *cp != 'm') بازگشت false;
    }
    بازگشت true;
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
    macro(ARRAY_برداشت_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_برداشت_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_افزودن_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_افزودن_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_مقدار,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_مقدار_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_مقدار_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_مقدار_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_مقدار_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_مقدار_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_مقدار_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_درج,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_برداشت,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_برداشت_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_افزودن,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_افزودن_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    ایستا constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    ایستا constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

عمومی:
    شمارش en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    شمارش en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت VL_PURE {
        ایستا constexpr ثابت نویسه* ثابت مقادیر[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        بازگشت مقادیر[m_e];
    }
    bool isPure() ثابت VL_PURE {
        ایستا constexpr bool مقادیر[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        بازگشت مقادیر[m_e];
    }
    bool isClean() ثابت VL_PURE {
        ایستا constexpr bool مقادیر[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        بازگشت مقادیر[m_e];
    }
    ثابت نویسه* args() ثابت VL_PURE {
        ایستا constexpr ثابت نویسه* ثابت مقادیر[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        بازگشت مقادیر[m_e];
    }
    // Return array method for given name
    ایستا VCMethod arrayMethod(ثابت string& name);
};
constexpr bool operator==(ثابت VCMethod& lhs, ثابت VCMethod& rhs) { بازگشت lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VCMethod& lhs, VCMethod::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, ثابت VCMethod& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VCMethod& rhs) {
    بازگشت os << rhs.ascii();
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
    ایستا constexpr bool PURE = true;  // For macro expansion of 'pure' field only

عمومی:
    شمارش en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    شمارش en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ثابت نویسه* ascii() ثابت VL_PURE {
        ایستا ثابت نویسه* ثابت مقادیر[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        بازگشت مقادیر[m_e];
    }
    bool isPure() ثابت VL_PURE {
        ایستا ثابت bool مقادیر[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        بازگشت مقادیر[m_e];
    }
    ثابت نویسه* args() ثابت VL_PURE {
        ایستا ثابت نویسه* ثابت مقادیر[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        بازگشت مقادیر[m_e];
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
عمومی:
    شمارش en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

خصوصی:
    ساختار Item final {
        شمارش en m_e;  // Statement's enum mnemonic, for checking
        ثابت نویسه* m_name;  // Statements name, for debugging
    };
    ایستا Item s_itemData[];

عمومی:
    شمارش en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت VL_PURE {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"none", "ctor_var_reset_call"};
        بازگشت names[m_e];
    }
    bool isNone() ثابت { بازگشت m_e == NONE; }
};
constexpr bool operator==(ثابت VCStmtType& lhs, ثابت VCStmtType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VCStmtType& lhs, VCStmtType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, ثابت VCStmtType& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VCStmtType& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
عمومی:
    شمارش en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    شمارش en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت VL_PURE {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VCaseType& lhs, ثابت VCaseType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VCaseType& lhs, VCaseType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, ثابت VCaseType& rhs) { بازگشت lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
عمومی:
    شمارش en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        بازگشت names[m_e];
    }
    bool isAssignable() ثابت { بازگشت m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
};
constexpr bool operator==(ثابت VCastable& lhs, ثابت VCastable& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VCastable& lhs, VCastable::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, ثابت VCastable& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VCastable& rhs) {
    بازگشت os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
عمومی:
    شمارش en : uint8_t {
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
    شمارش en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() ثابت { بازگشت m_e; }  // LCOV_EXCL_LINE
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        بازگشت names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ثابت نویسه* verilogKwd() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        بازگشت names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ثابت نویسه* binSetEnum() ثابت {
        انتخاب (m_e) {
        حالت BINS_IGNORE: بازگشت "VlCovBinKind::KIND_IGNORE";
        حالت BINS_ILLEGAL: بازگشت "VlCovBinKind::KIND_ILLEGAL";
        حالت BINS_DEFAULT: بازگشت "VlCovBinKind::KIND_DEFAULT";
        پیشفرض: بازگشت "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() ثابت {
        بازگشت m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(ثابت VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    بازگشت lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
عمومی:
    شمارش en : uint8_t {
        // Shared by option.* and type_option.*
        WEIGHT,
        GOAL,
        AT_LEAST,
        AUTO_BIN_MAX,
        PER_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_رقم_PRINT_MISSING,
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
    شمارش en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"weight",
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
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    بازگشت lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
عمومی:
    شمارش en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    شمارش en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(صحیح _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }  // LCOV_EXCL_LINE
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"", "[*]", "[->]", "[=]"};
        بازگشت names[m_e];
    }
    ثابت نویسه* asciiJson() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        بازگشت names[m_e];
    }
};

//######################################################################

class VDirection final {
عمومی:
    شمارش en : uint8_t { NONE, درون_داد, OUTPUT, INOUT, REF, CONSTREF };
    شمارش en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت VL_MT_SAFE { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        بازگشت names[m_e];
    }
    string verilogKwd() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        بازگشت names[m_e];
    }
    string prettyName() ثابت { بازگشت verilogKwd(); }
    bool isAny() ثابت { بازگشت m_e != NONE; }
    bool isInout() ثابت { بازگشت m_e == INOUT; }
    bool isInoutOrRef() ثابت { بازگشت m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_درون_داد() ثابت { بازگشت m_e == درون_داد; }
    bool isNonOutput() ثابت {
        بازگشت m_e == درون_داد || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ثابت { بازگشت m_e == OUTPUT; }
    bool isReadOnly() ثابت VL_MT_SAFE { بازگشت m_e == درون_داد || m_e == CONSTREF; }
    bool isWritable() ثابت VL_MT_SAFE { بازگشت m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ثابت VL_MT_SAFE { بازگشت m_e == REF; }
    bool isConstRef() ثابت VL_MT_SAFE { بازگشت m_e == CONSTREF; }
    string traceSigDirection() ثابت {
        اگر (isInout()) {
            بازگشت "VerilatedTraceSigDirection::INOUT";
        } وگرنه اگر (isWritable()) {
            بازگشت "VerilatedTraceSigDirection::OUTPUT";
        } وگرنه اگر (isNonOutput()) {
            بازگشت "VerilatedTraceSigDirection::INPUT";
        }
        بازگشت "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ثابت {
        انتخاب (m_e) {
        حالت NONE: بازگشت VAccess::NOCHANGE;
        حالت درون_داد:
        حالت CONSTREF: بازگشت VAccess::READ;
        حالت OUTPUT: بازگشت VAccess::WRITE;
        حالت INOUT:
        حالت REF: بازگشت VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(ثابت VDirection& lhs, ثابت VDirection& rhs) VL_MT_SAFE {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, ثابت VDirection& rhs) VL_MT_SAFE {
    بازگشت lhs == rhs.m_e;
}
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VDirection& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
عمومی:
    شمارش en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    شمارش en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    bool addNewline() ثابت { بازگشت m_e != DT_WRITE; }
    bool needScopeTracking() ثابت { بازگشت m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VDisplayType& lhs, ثابت VDisplayType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VDisplayType& lhs, VDisplayType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, ثابت VDisplayType& rhs) { بازگشت lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
عمومی:
    شمارش en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    شمارش en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VDumpCtlType& lhs, ثابت VDumpCtlType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VDumpCtlType& lhs, VDumpCtlType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, ثابت VDumpCtlType& rhs) { بازگشت lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
عمومی:
    // REMEMBER to edit the strings below too
    شمارش en : uint8_t {
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
    شمارش en m_e;
    bool clockedStmt() ثابت {
        ایستا constexpr bool clocked[] = {
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
        بازگشت clocked[m_e];
    }
    bool anEdge() ثابت { بازگشت m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ثابت {
        انتخاب (m_e) {
        حالت ET_CHANGED: بازگشت ET_CHANGED;
        حالت ET_BOTHEDGE: بازگشت ET_BOTHEDGE;
        حالت ET_POSEDGE: بازگشت ET_NEGEDGE;
        حالت ET_NEGEDGE: بازگشت ET_POSEDGE;
        پیشفرض: UASSERT_STATIC(0, "Inverting bad edgeType()"); بازگشت ET_NEGEDGE;
        }
    }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        بازگشت names[m_e];
    }
    ثابت نویسه* verilogKwd() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        بازگشت names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(ثابت VEdgeType& other) ثابت {
        انتخاب (m_e) {
        حالت VEdgeType::ET_POSEDGE:
            اگر (other.m_e == VEdgeType::ET_NEGEDGE) بازگشت true;
            بشکن;
        حالت VEdgeType::ET_NEGEDGE:
            اگر (other.m_e == VEdgeType::ET_POSEDGE) بازگشت true;
            بشکن;
        پیشفرض: بشکن;
        }
        بازگشت false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
};
constexpr bool operator==(ثابت VEdgeType& lhs, ثابت VEdgeType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VEdgeType& lhs, VEdgeType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, ثابت VEdgeType& rhs) { بازگشت lhs == rhs.m_e; }

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
عمومی:
    شمارش en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    شمارش en m_e;

    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت مقادیر[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        بازگشت مقادیر[m_e];
    }
    bool isIterated() ثابت {
        ایستا constexpr bool مقادیر[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        بازگشت مقادیر[m_e];
    }
    bool hasTriggers() ثابت {
        ایستا constexpr bool مقادیر[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        بازگشت مقادیر[m_e];
    }
    bool firstIteration() ثابت {
        ایستا constexpr bool مقادیر[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        بازگشت مقادیر[m_e];
    }
    bool slow() ثابت {
        ایستا constexpr bool مقادیر[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        بازگشت مقادیر[m_e];
    }

    // Short name
    std::string tag() ثابت { بازگشت VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ثابت { بازگشت "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() ثابت { بازگشت "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ثابت { بازگشت "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() ثابت { بازگشت "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(صحیح _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() ثابت { بازگشت m_e; }

خصوصی:
    std::string capitalizedTag() ثابت {
        ثابت std::string name = ascii();
        بازگشت name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
عمومی:
    شمارش en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        بازگشت names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    // Is a node type compatible with the declaration
    bool is_گره_Compatible(ثابت Ast_گره* nodep) ثابت;
};
constexpr bool operator==(ثابت VFwdType& lhs, ثابت VFwdType& rhs) { بازگشت lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VFwdType& lhs, VFwdType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, ثابت VFwdType& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VFwdType& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    ایستا uint64_t s_cachedCntGbl;  // Global computed count

عمومی:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() ثابت { بازگشت m_cachedCnt == s_cachedCntGbl; }
    bool get() ثابت { بازگشت m_state; }
    تهی set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    تهی clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ایستا تهی clear_Cache_درخت() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
عمومی:
    شمارش en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    شمارش en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        بازگشت names[m_e];
    }
    ثابت نویسه* verilogKwd() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"join", "join_any", "join_none"};
        بازگشت names[m_e];
    }
    bool join() ثابت { بازگشت m_e == JOIN; }
    bool joinAny() ثابت { بازگشت m_e == JOIN_ANY; }
    bool joinNone() ثابت { بازگشت m_e == JOIN_NONE; }
};
constexpr bool operator==(ثابت VJoinType& lhs, ثابت VJoinType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VJoinType& lhs, VJoinType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, ثابت VJoinType& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VJoinType& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
عمومی:
    شمارش en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        بازگشت names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    bool isNone() ثابت { بازگشت m_e == NONE; }
    bool isAutomatic() ثابت { بازگشت m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ثابت { بازگشت m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ثابت { بازگشت m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ثابت {
        انتخاب (m_e) {
        حالت AUTOMATIC_EXPLICIT: بازگشت AUTOMATIC_IMPLICIT;
        حالت STATIC_EXPLICIT: بازگشت STATIC_IMPLICIT;
        پیشفرض: بازگشت m_e;
        }
    }
};
constexpr bool operator==(ثابت VLifetime& lhs, ثابت VLifetime& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VLifetime& lhs, VLifetime::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, ثابت VLifetime& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VLifetime& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
عمومی:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            صحیح m_index;  // Current index
            ثابت صحیح m_inc;  // Step, +1 or -1
        عمومی:
            Iterator(صحیح index, صحیح inc)
                : m_index{index}
                , m_inc{inc} {}
            صحیح operator*() ثابت { بازگشت m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                بازگشت *this;
            }
            bool operator!=(ثابت Iterator& that) ثابت { بازگشت m_index != that.m_index; }
        };
        ثابت صحیح m_begin;  // First index
        ثابت صحیح m_end;  // One past the last index
        ثابت صحیح m_inc;  // Step, +1 or -1
    عمومی:
        Sequence(صحیح first, صحیح last, صحیح inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() ثابت { بازگشت Iterator{m_begin, m_inc}; }
        Iterator end() ثابت { بازگشت Iterator{m_end, m_inc}; }
    };

    صحیح m_چپ = 0;  // Left side of range (pre-':')
    صحیح m_راست = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(ثابت VNumRange& rhs) ثابت {
        بازگشت m_چپ == rhs.m_چپ && m_راست == rhs.m_راست && m_ranged == rhs.m_ranged;
    }
    bool operator<(ثابت VNumRange& rhs) ثابت {
        اگر ((m_چپ < rhs.m_چپ)) بازگشت true;
        اگر (!(m_چپ == rhs.m_چپ)) بازگشت false;  // lhs > rhs
        اگر ((m_راست < rhs.m_راست)) بازگشت true;
        اگر (!(m_راست == rhs.m_راست)) بازگشت false;  // lhs > rhs
        اگر ((m_ranged < rhs.m_ranged)) بازگشت true;
        اگر (!(m_ranged == rhs.m_ranged)) بازگشت false;  // lhs > rhs
        بازگشت false;
    }
    //
    VNumRange() = پیشفرض;
    VNumRange(صحیح hi, صحیح lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(صحیح چپ, صحیح راست)
        : m_چپ{چپ}
        , m_راست{راست}
        , m_ranged{true} {}
    ~VNumRange() = پیشفرض;
    // MEMBERS
    تهی init(صحیح hi, صحیح lo, bool ascending) {
        اگر (lo > hi) std::swap(hi, lo);
        m_چپ = ascending ? lo : hi;
        m_راست = ascending ? hi : lo;
        m_ranged = true;
    }
    صحیح چپ() ثابت { بازگشت m_چپ; }
    صحیح راست() ثابت { بازگشت m_راست; }
    صحیح hi() ثابت VL_MT_SAFE {
        بازگشت m_چپ > m_راست ? m_چپ : m_راست;
    }  // How to show a declaration
    صحیح lo() ثابت VL_MT_SAFE {
        بازگشت m_چپ > m_راست ? m_راست : m_چپ;
    }  // How to show a declaration
    صحیح چپ_To_راست_Inc() ثابت { بازگشت ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_چپ_To_راست() ثابت { بازگشت {m_چپ, m_راست, چپ_To_راست_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_راست_To_چپ() ثابت { بازگشت {m_راست, m_چپ, -چپ_To_راست_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() ثابت { بازگشت {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() ثابت { بازگشت {hi(), lo(), -1}; }
    صحیح elements() ثابت VL_MT_SAFE { بازگشت hi() - lo() + 1; }
    bool ranged() ثابت { بازگشت m_ranged; }
    bool ascending() ثابت { بازگشت m_چپ < m_راست; }
    صحیح hiMaxSelect() ثابت {
        بازگشت (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ثابت {
        اگر (ranged()) {
            بازگشت "["s + std::to_string(چپ()) + ":" + std::to_string(راست()) + "]";
        } وگرنه {
            بازگشت "[norg]";
        }
    }
    تهی dump(std::ostream& str) ثابت { str << ascii(); }
};
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VNumRange& rhs) {
    rhs.dump(os);
    بازگشت os;
}

// ######################################################################

class VPragmaType final {
عمومی:
    شمارش en : uint8_t {
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
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {
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
        بازگشت names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
};
constexpr bool operator==(ثابت VPragmaType& lhs, ثابت VPragmaType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VPragmaType& lhs, VPragmaType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, ثابت VPragmaType& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VPragmaType& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
عمومی:
    شمارش en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        بازگشت names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() ثابت { بازگشت m_e; }
    bool isRandomizable() ثابت { بازگشت m_e != NONE; }
    bool isRand() ثابت { بازگشت m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ثابت { بازگشت m_e == RAND_CYCLIC; }
};
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VRandAttr& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ایستا ثابت std::shared_ptr<ثابت string> s_emptyp;  // Holds ""
    ایستا ثابت std::shared_ptr<ثابت string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ثابت string> m_strp;

عمومی:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, ثابت string& field)
        : m_strp{std::make_shared<ثابت string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, ثابت string& field)
        : m_strp{std::make_shared<ثابت string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() ثابت { بازگشت m_strp == s_emptyp; }
    bool isVlSym() ثابت { بازگشت m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() ثابت { بازگشت m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ثابت;
    ایستا string replaceThis(bool useSelfForThis, ثابت string& text);
    ثابت std::string& asString() ثابت { بازگشت *m_strp; }
    bool operator==(ثابت VSelfPointerText& other) ثابت { بازگشت *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
عمومی:
    شمارش en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        بازگشت names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    ایستا VSigning fromBool(bool isSigned) {  // Factory method
        بازگشت isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    bool isSigned() ثابت VL_MT_SAFE { بازگشت m_e == SIGNED; }
    bool isNosign() ثابت VL_MT_SAFE { بازگشت m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(ثابت VSigning& lhs, ثابت VSigning& rhs) { بازگشت lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VSigning& lhs, VSigning::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, ثابت VSigning& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VSigning& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

class VStrength final {
عمومی:
    شمارش en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    شمارش en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VStrength& lhs, ثابت VStrength& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VStrength& lhs, VStrength::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, ثابت VStrength& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VStrength& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
عمومی:
    شمارش en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    شمارش en m_e;
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        بازگشت names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() ثابت { بازگشت m_e; }
};

//######################################################################

class VTracePrefixType final {
عمومی:
    شمارش en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    شمارش en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VTracePrefixType& lhs, ثابت VTracePrefixType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, ثابت VTracePrefixType& rhs) {
    بازگشت lhs == rhs.m_e;
}
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VTracePrefixType& rhs) {
    بازگشت os << rhs.ascii();
}

//######################################################################

class VTraceType final {
عمومی:
    شمارش en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    شمارش en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"CONSTANT", "FULL", "CHANGE"};
        بازگشت names[m_e];
    }
    ثابت نویسه* func_prefix() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"trace_const", "trace_full", "trace_chg"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VTraceType& lhs, ثابت VTraceType& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VTraceType& lhs, VTraceType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, ثابت VTraceType& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VTraceType& rhs) {
    بازگشت os << rhs.ascii();
}

//######################################################################

class VUseType final {
عمومی:
    شمارش en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    شمارش en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    bool containsAny(VUseType other) ثابت { بازگشت m_e & other.m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        بازگشت names[m_e - 1];
    }
};
constexpr bool operator==(ثابت VUseType& lhs, ثابت VUseType& rhs) { بازگشت lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VUseType& lhs, VUseType::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, ثابت VUseType& rhs) { بازگشت lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    بازگشت VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    بازگشت VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VUseType& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

class VVarType final {
عمومی:
    شمارش en : uint8_t {
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
    شمارش en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا constexpr ثابت نویسه* ثابت names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        بازگشت names[m_e];
    }
    bool isParam() ثابت { بازگشت m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ثابت {
        بازگشت (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ثابت {
        بازگشت (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ثابت { بازگشت (m_e == TRIOR); }
    bool isWiredNet() ثابت { بازگشت (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ثابت {  // In Verilog, always ok in SystemVerilog
        بازگشت (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ثابت {
        بازگشت (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() ثابت {
        بازگشت (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ثابت {
        بازگشت (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ثابت نویسه* traceSigKind() ثابت {
        // VerilatedTraceSigKind to used in trace signal declaration
        ایستا constexpr ثابت نویسه* ثابت lut[] = {
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
        بازگشت lut[m_e];
    }
};
constexpr bool operator==(ثابت VVarType& lhs, ثابت VVarType& rhs) VL_MT_SAFE {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, ثابت VVarType& rhs) VL_MT_SAFE {
    بازگشت lhs == rhs.m_e;
}
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VVarType& rhs) VL_MT_SAFE {
    بازگشت os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_کلید final {
عمومی:
    ثابت صحیح m_width;  // From AstNodeDType: Bit width of operation
    ثابت صحیح m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ثابت VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ثابت VSigning m_numeric;  // From AstNodeDType: Node is signed
    ثابت VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(ثابت VBasic_Type_کلید& rhs) ثابت {
        بازگشت m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(ثابت VBasic_Type_کلید& rhs) ثابت {
        اگر ((m_width < rhs.m_width)) بازگشت true;
        اگر (!(m_width == rhs.m_width)) بازگشت false;  // lhs > rhs
        اگر ((m_widthMin < rhs.m_widthMin)) بازگشت true;
        اگر (!(m_widthMin == rhs.m_widthMin)) بازگشت false;  // lhs > rhs
        اگر ((m_numeric < rhs.m_numeric)) بازگشت true;
        اگر (!(m_numeric == rhs.m_numeric)) بازگشت false;  // lhs > rhs
        اگر ((m_keyword < rhs.m_keyword)) بازگشت true;
        اگر (!(m_keyword == rhs.m_keyword)) بازگشت false;  // lhs > rhs
        اگر ((m_nrange < rhs.m_nrange)) بازگشت true;
        اگر (!(m_nrange == rhs.m_nrange)) بازگشت false;  // lhs > rhs
        بازگشت false;
    }
    VBasic_Type_کلید(صحیح width, صحیح widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  ثابت VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_کلید() = پیشفرض;
};

#endif  // Guard
