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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ASTATTR_H_
#تعریف_کرو VERILATOR_V3ASTATTR_H_

#اگر_غیر_تعریف_شدہ VERILATOR_V3AST_H_
#غلطی "Use V3Ast.h as the include"
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کرو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#اگر_ختم

// Hint class so we can choose constructors
class VFlag_ثنائی_ہندسہ_Packed {};
class VFlag_بچہ_DType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbort_قسم final {
عوامی:
    // IEEE 1800-2023 16.12.14 property abort operators.
    شمار en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    شمار en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbort_قسم(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساکن ثابت حرف* ثابت نام[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        واپس نام[m_e];
    }
    bool isAccept() ثابت { واپس m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
عوامی:
    شمار en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"RD", "WR", "RW", "--"};
        واپس نام[m_e];
    }
    ثابت حرف* arrow() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        واپس نام[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    VAccess invert() ثابت {
        واپس (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ثابت { واپس m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ثابت { واپس m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ثابت { واپس m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ثابت { واپس m_e == WRITE || m_e == READWRITE; }
    bool isRW() ثابت { واپس m_e == READWRITE; }
};
constexpr bool عامل==(ثابت VAccess& lhs, ثابت VAccess& rhs) { واپس lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VAccess& lhs, VAccess::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VAccess::en lhs, ثابت VAccess& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VAccess& rhs) { واپس os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
عوامی:
    شمار en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_تفویض };
    شمار en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VAlwaysKwd& lhs, ثابت VAlwaysKwd& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VAlwaysKwd::en lhs, ثابت VAlwaysKwd& rhs) { واپس lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
عوامی:
    // IEEE 1800-2023 Table 20-5
    شمار en : uint8_t {
        مقصد_BE_EVALUATED = 0,
        تالا = 1,
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
    شمار en m_e;
    VAssertCtlType()
        : m_e{مقصد_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        // IEEE 1800-2023 20.11
        ساکن constexpr ثابت حرف* ثابت نام[] = {"",
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
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VAssertCtlType& lhs, ثابت VAssertCtlType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VAssertCtlType::en lhs, ثابت VAssertCtlType& rhs) {
    واپس lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
عوامی:
    // IEEE 1800-2023 Table 20-7
    شمار en : uint8_t {
        اندرونی_2 = 0,  // Non IEEE type, for directives to be evaluated from expression.
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
    شمار en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ثابت {
        std::stringstream types;
        اگر (m_e == اندرونی_2)
            types << "INTERNAL ";
        ورنہ {
            اگر (m_e & ASSERT) types << "ASSERT ";
            اگر (m_e & COVER) types << "COVER ";
            اگر (m_e & ASSUME) types << "ASSUME ";
            اگر (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            اگر (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            اگر (m_e & INTRINSIC) types << "INTRINSIC ";
            اگر (m_e & RESTRICT) types << "RESTRICT ";
        }
        ثابت string str = types.str();
        UASSERT(!str.خالی_خانے(), "Assert should be of one of types");
        واپس str.substr(0, str.سائز() - 1);
    }
    constexpr عامل en() ثابت { واپس m_e; }
};
constexpr bool عامل==(ثابت VAssertDirectiveType& lhs, ثابت VAssertDirectiveType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VAssertDirectiveType::en lhs, ثابت VAssertDirectiveType& rhs) {
    واپس lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en عامل|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    واپس VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
عوامی:
    // IEEE 1800-2023 Table 20-6
    شمار en : uint8_t {
        اندرونی_2 = 0,  // Non IEEE type, for assertions that should not be controlled.
        CONCURRENT = (1 << 0),
        SIMPLE_فوری = (1 << 1),
        OBSERVED_DEFERRED_فوری = (1 << 2),
        FINAL_DEFERRED_فوری = (1 << 3),
        EXPECT = (1 << 4),
        UNIQUE = (1 << 5),
        UNIQUE0 = (1 << 6),
        ترجیح = (1 << 7),
    };
    شمار en m_e;
    VAssertType()
        : m_e{اندرونی_2} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool شامل_ہے_Any(VAssertType other) ثابت { واپس m_e & other.m_e; }
    string ascii() ثابت {
        std::stringstream types;
        اگر (m_e == اندرونی_2)
            types << "INTERNAL ";
        ورنہ {
            اگر (m_e & CONCURRENT) types << "CONCURRENT ";
            اگر (m_e & SIMPLE_فوری) types << "SIMPLE_IMMEDIATE ";
            اگر (m_e & OBSERVED_DEFERRED_فوری) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            اگر (m_e & FINAL_DEFERRED_فوری) types << "FINAL_DEFERRED_IMMEDIATE ";
            اگر (m_e & EXPECT) types << "EXPECT ";
            اگر (m_e & UNIQUE) types << "UNIQUE ";
            اگر (m_e & UNIQUE0) types << "UNIQUE0 ";
            اگر (m_e & ترجیح) types << "PRIORITY ";
        }
        ثابت string str = types.str();
        UASSERT(!str.خالی_خانے(), "Assert should be of one of types");
        واپس str.substr(0, str.سائز() - 1);
    }
    constexpr عامل en() ثابت { واپس m_e; }
};
constexpr bool عامل==(ثابت VAssertType& lhs, ثابت VAssertType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAssertType& lhs, VAssertType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VAssertType::en lhs, ثابت VAssertType& rhs) { واپس lhs == rhs.m_e; }
constexpr VAssertType::en عامل|(VAssertType::en lhs, VAssertType::en rhs) {
    واپس VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
عوامی:
    شمار en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    شمار en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"default", "weak", "strong"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VPropStrength& lhs, ثابت VPropStrength& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل!=(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    واپس lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
عوامی:
    // clang-format off
    شمار en: uint8_t  {
        ILLEGAL,
        //
        DIM_بٹ,                       // V3Const converts to constant
        DIM_بٹ_OR_نمبر,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_بلند,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_بایاں,                       // V3Width processes
        DIM_کم,                        // V3Width processes
        DIM_دایاں,                      // V3Width processes
        DIM_سائز,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_عوامی,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_پہلا,                     // V3Width processes
        ENUM_آخری,                      // V3Width processes
        ENUM_عدد,                       // V3Width processes
        ENUM_اگلا,                      // V3Width processes
        ENUM_پچھلا,                      // V3Width processes
        ENUM_NAME,                      // V3Width processes
        ENUM_VALID,                     // V3Width processes
        //
        FUNC_ARG_PROTO,                 // V3WidthCommit processes
        FUNC_RETURN_PROTO,              // V3WidthCommit processes
        //
        TYPEID,                         // V3Width processes
        TYPENAME,                       // V3Width processes
        //
        VAR_بنیاد,                       // V3LinkResolve creates for AstPreSel, V3LinkParam removes
        VAR_FORCEABLE,                  // V3LinkParse moves to AstVar::isForceable
        VAR_FSM_ARC_INCLUDE_COND,       // V3LinkParse moves to AstVar::attrFsmArcInclCond
        VAR_FSM_ARC_INCLUDE_COND_AUTO,  // Flavors of above option - setting expand mode per FSM
        VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND,
        VAR_FSM_ARC_INCLUDE_COND_بھرا,
        VAR_FSM_STATE,                  // V3LinkParse moves to AstVar::attrFsmState
        VAR_FSM_STATE_AUTO,             // Flavors of above option - setting expand mode per FSM
        VAR_FSM_STATE_AUTO_EXPAND,
        VAR_FSM_STATE_بھرا,
        VAR_پورٹ_DTYPE,                 // V3LinkDot for V3Width to check port dtype
        VAR_عوامی,                     // V3LinkParse moves to AstVar::sigPublic
        VAR_عوامی_FLAT,                // V3LinkParse moves to AstVar::sigPublic
        VAR_عوامی_FLAT_RD,             // V3LinkParse moves to AstVar::sigPublic
        VAR_عوامی_FLAT_RW,             // V3LinkParse moves to AstVar::sigPublic
        VAR_SC_BIGUINT,                 // V3LinkParse moves to AstVar::attrScBigUint
        VAR_SC_BV,                      // V3LinkParse moves to AstVar::attrScBv
        VAR_SFORMAT,                    // V3LinkParse moves to AstVar::attrSFormat
        VAR_بانٹو_VAR                   // V3LinkParse moves to AstVar::attrSplitVar
    };
    // clang-format on
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        // clang-format off
        ساکن constexpr ثابت حرف* ثابت نام[] = {
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
        واپس نام[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ثابت {
        انتخاب (m_e) {
        حالت DIM_بٹ:
        حالت DIM_بٹ_OR_نمبر:
        حالت DIM_DIMENSIONS:
        حالت DIM_بلند:
        حالت DIM_INCREMENT:
        حالت DIM_بایاں:
        حالت DIM_کم:
        حالت DIM_دایاں:
        حالت DIM_سائز:
        حالت DIM_UNPK_DIMENSIONS:
        حالت TYPEID:
        حالت TYPENAME: واپس true;
        طےشدہ: واپس false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
};
constexpr bool عامل==(ثابت VAttrType& lhs, ثابت VAttrType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAttrType& lhs, VAttrType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VAttrType::en lhs, ثابت VAttrType& rhs) { واپس lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

عوامی:
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
    خالی combine(ثابت VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ثابت { واپس m_extends | m_final | m_initial; }
    bool isExtends() ثابت { واپس m_extends; }
    bool isFinal() ثابت { واپس m_final; }
    bool isInitial() ثابت { واپس m_initial; }
    string ascii() ثابت {
        string برآمد;
        اگر (m_initial) برآمد = VString::dot(برآمد, " ", "initial");
        اگر (m_extends) برآمد = VString::dot(برآمد, " ", "extends");
        اگر (m_final) برآمد = VString::dot(برآمد, " ", "final");
        واپس برآمد;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
عوامی:
    شمار en : uint8_t {
        UNKNOWN,
        ثنائی_ہندسہ,
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
        وقت,
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
    شمار en m_e;
    ثابت حرف* ascii() ثابت VL_MT_محفوظ {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"%E-unk",
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
        واپس نام[m_e];
    }
    ثابت حرف* typeName() ثابت VL_MT_محفوظ {
        انتخاب (m_e) {
        حالت LOGIC2STATE: واپس VBasicDTypeKwd{LOGIC}.ascii();
        حالت INTEGER2STATE: واپس VBasicDTypeKwd{INTEGER}.ascii();
        حالت TIME2STATE: واپس VBasicDTypeKwd{وقت}.ascii();
        طےشدہ: واپس ascii();
        }
    }
    ثابت حرف* dpiType() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"%E-unk",
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
        واپس نام[m_e];
    }
    ساکن خالی selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_محفوظ : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_محفوظ : m_e{_e} {}
    explicit VBasicDTypeKwd(صحیح _e) VL_MT_محفوظ  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    صحیح چوڑائی() ثابت {
        انتخاب (m_e) {
        حالت ثنائی_ہندسہ: واپس 1;  // scalar, can't bit extract unless ranged
        حالت BYTE: واپس 8;
        حالت CHANDLE: واپس 64;
        حالت NULLTYPE: واپس 1;  // Historical - bit was used instead of NULLTYPE
        حالت EVENT: واپس 1;
        حالت INT: واپس 32;
        حالت INTEGER: واپس 32;
        حالت INTEGER2STATE: واپس 32;
        حالت LOGIC: واپس 1;  // scalar, can't bit extract unless ranged
        حالت LOGIC2STATE: واپس 1;  // scalar, can't bit extract unless ranged
        حالت LONGINT: واپس 64;
        حالت DOUBLE: واپس 64;  // opaque
        حالت SHORTINT: واپس 16;
        حالت وقت: واپس 64;
        حالت TIME2STATE: واپس 64;
        حالت STRING: واپس 64;  // opaque  // Just the pointer, for today
        حالت SCOPEPTR: واپس 0;  // opaque
        حالت CHARPTR: واپس 0;  // opaque
        حالت MTASKSTATE: واپس 0;  // opaque
        حالت DELAY_SCHEDULER: واپس 0;  // opaque
        حالت TRIGGER_SCHEDULER: واپس 0;  // opaque
        حالت DYNAMIC_TRIGGER_SCHEDULER: واپس 0;  // opaque
        حالت FORK_SYNC: واپس 0;  // opaque
        حالت PROCESS_REFERENCE: واپس 0;  // opaque
        حالت RANDOM_GENERATOR: واپس 0;  // opaque
        حالت RANDOM_STDGENERATOR: واپس 0;  // opaque
        حالت COVERGROUP_INSTHANDLE: واپس 0;  // opaque
        حالت UINT32: واپس 32;
        حالت UINT64: واپس 64;
        طےشدہ: واپس 0;
        }
    }
    bool isSigned() ثابت {
        واپس m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ثابت {
        واپس m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == ثنائی_ہندسہ || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == وقت || m_e == TIME2STATE;
    }
    bool isFourstate() ثابت {
        واپس m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == وقت;
    }
    bool isZeroInit() ثابت {  // Otherwise initializes to X
        واپس (m_e == ثنائی_ہندسہ || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ثابت {  // Enum increment supported
        واپس (m_e == ثنائی_ہندسہ || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == وقت || m_e == TIME2STATE);
    }
    bool is_ثنائی_ہندسہ() ثابت { واپس m_e == ثنائی_ہندسہ; }
    bool is_ثنائی_ہندسہ_Logic() ثابت {  // Bit/logic vector types; can form a packed array
        واپس (m_e == LOGIC || m_e == LOGIC2STATE || m_e == ثنائی_ہندسہ);
    }
    bool isDpiUnsignable() ثابت {  // Can add "unsigned" to DPI
        واپس (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ثابت {  // Uses standard C layout, for DPI runtime access
        واپس (m_e == ثنائی_ہندسہ || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ثابت VL_MT_محفوظ {  // IE not a simple number we can bit optimize
        واپس (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ثابت VL_MT_محفوظ { واپس m_e == CHANDLE; }
    bool isDouble() ثابت VL_MT_محفوظ { واپس m_e == DOUBLE; }
    bool isEvent() ثابت { واپس m_e == EVENT; }
    bool isString() ثابت VL_MT_محفوظ { واپس m_e == STRING; }
    bool isMTaskState() ثابت VL_MT_محفوظ { واپس m_e == MTASKSTATE; }
    bool isSameish(ثابت VBasicDTypeKwd& other) ثابت VL_MT_محفوظ {
        انتخاب (other.m_e) {
        حالت LOGIC:
        حالت LOGIC2STATE:
        حالت LOGIC_IMPLICIT: واپس m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        حالت INTEGER:
        حالت INTEGER2STATE: واپس m_e == INTEGER || m_e == INTEGER2STATE;
        حالت وقت:
        حالت TIME2STATE: واپس m_e == وقت || m_e == TIME2STATE;
        طےشدہ: واپس m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_محفوظ {
        انتخاب (m_e) {
        حالت ثنائی_ہندسہ:
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
        حالت UINT64: واپس true;
        طےشدہ: واپس false;
        }
    }

    ثابت حرف* traceSigType() ثابت {
        // VerilatedTraceSigType to used in trace signal declaration
        ساکن constexpr ثابت حرف* ثابت lut[] = {
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
        static_assert(حجم(lut) / حجم(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        واپس lut[m_e];
    }
};
constexpr bool عامل==(ثابت VBasicDTypeKwd& lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_محفوظ {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_محفوظ {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VBasicDTypeKwd::en lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_محفوظ {
    واپس lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
عوامی:
    شمار en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, ENUM_انتہا };
    شمار en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"FALSE", "TRUE", "UNK"};
        واپس نام[m_e];
    }
    bool is_معلوم() ثابت { واپس m_e != BU_UNKNOWN; }
    bool true_معلوم() ثابت { واپس m_e == BU_TRUE; }
    خالی سیٹ_True_Or_False(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool عامل==(ثابت VBoolOrUnknown& lhs, ثابت VBoolOrUnknown& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VBoolOrUnknown::en lhs, ثابت VBoolOrUnknown& rhs) {
    واپس lhs == rhs.m_e;
}
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VBoolOrUnknown& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
عوامی:
    شمار en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, ENUM_انتہا };
    شمار en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    bool unknown() ثابت { واپس m_e == BP_UNKNOWN; }
    bool likely() ثابت { واپس m_e == BP_LIKELY; }
    bool unlikely() ثابت { واپس m_e == BP_UNLIKELY; }
    VBranchPred invert() ثابت {
        اگر (m_e == BP_UNLIKELY) واپس BP_LIKELY;
        اگر (m_e == BP_LIKELY) واپس BP_UNLIKELY;
        واپس m_e;
    }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        واپس نام[m_e];
    }
    ثابت حرف* asciiShort() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"", "L", "!L"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VBranchPred& lhs, ثابت VBranchPred& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VBranchPred& lhs, VBranchPred::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VBranchPred::en lhs, ثابت VBranchPred& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VBranchPred& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ساکن constexpr bool validate_دلائل_Descriptor(ثابت حرف* descrp) {
    // Is "TODO"
    اگر (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        واپس true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    برائے (ثابت حرف* cp = descrp; *cp; ++cp) {
        اگر (*cp == '+') واپس cp != descrp && !cp[1];
        اگر (*cp != 'r' && *cp != 'w' && *cp != 'm') واپس false;
    }
    واپس true;
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
#تعریف_کرو FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     خالص_2,   CLEAN,  "r") \
    macro(ARRAY_AT_عقب,                      "atBack",                 خالص_2,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                خالص_2,   CLEAN,  "r") \
    macro(ARRAY_ڈھونڈو,                         "find",                   خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڈھونڈو_پہلا,                   "find_first",             خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڈھونڈو_پہلا_اشاریہ,             "find_first_index",       خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڈھونڈو_اشاریہ,                   "find_index",             خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڈھونڈو_آخری,                    "find_last",              خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڈھونڈو_آخری_اشاریہ,              "find_last_index",        خالص_2,   CLEAN,  "") \
    macro(ARRAY_پہلا,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 خالص_2,   CLEAN,  "r") \
    macro(ARRAY_آخری,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_اگلا,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_نکالو_عقب,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_نکالو_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_پچھلا,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ڈالو_عقب,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ڈالو_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_الٹ,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  خالص_2,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   خالص_2,   CLEAN,  "") \
    macro(ARRAY_R_حاصل_ضرب,                    "r_product",              خالص_2,   false,  "") \
    macro(ARRAY_R_مجموع,                        "r_sum",                  خالص_2,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  خالص_2,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_قاش,                        "slice",                  خالص_2,   CLEAN,  "r") \
    macro(ARRAY_ترتیب_دو,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 خالص_2,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_اشاریہ,                 "unique_index",           خالص_2,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 خالص_2,   CLEAN,  "r") \
    macro(ASSOC_پہلا,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_اگلا,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_سائز,                         "size",                   خالص_2,   CLEAN,  "") \
    macro(CLASS_سیٹ_RANDMODE,                 "set_randmode",           false,  CLEAN,  "r") \
    macro(COVERGROUP_جوڑو_ARRAY_NAMER,         "addArrayNamer",          false,  CLEAN,  "r+") \
    macro(COVERGROUP_جوڑو_BIN,                 "addBin",                 false,  CLEAN,  "r+") \
    macro(COVERGROUP_جوڑو_COVERPOINT,          "addCoverpoint",          false,  CLEAN,  "") \
    macro(COVERGROUP_جوڑو_CROSS,               "addCross",               false,  CLEAN,  "") \
    macro(COVERGROUP_جوڑو_CROSS_DYN,           "addCrossDyn",            false,  CLEAN,  "") \
    macro(COVERGROUP_جوڑو_NUMBERED_NAMER,      "addNumberedNamer",       false,  CLEAN,  "r+") \
    macro(COVERGROUP_جوڑو_SINGLE_NAMER,        "addSingleNamer",         false,  CLEAN,  "r+") \
    macro(COVERGROUP_ATTACH,                  "attach",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_CLEAR_کامیابی_LIST,          "clearHitList",           false,  CLEAN,  "") \
    macro(COVERGROUP_COVERAGE,                "coverage",               false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_BEGIN,        "withBegin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_CROSS_WITH_انتہا,          "withEnd",                false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_TEST,         "withTest",               false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_قدر,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_قدر_W,      "withValueW",             false,  CLEAN,  "rw") \
    macro(COVERGROUP_FINALIZE_BINS,           "finalizeBins",           false,  CLEAN,  "") \
    macro(COVERGROUP_INCREMENT_BIN,           "incrementBin",           false,  CLEAN,  "r") \
    macro(COVERGROUP_INIT,                    "init",                   false,  CLEAN,  "r+") \
    macro(COVERGROUP_INST_P,                  "p",                      خالص_2,   CLEAN,  "") \
    macro(COVERGROUP_LEND_وزن,             "lendWeight",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_ریکارڈ_کامیابی,              "recordHit",              false,  CLEAN,  "r") \
    macro(COVERGROUP_رجسٹر_BINS,           "registerBins",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_SAMPLE,                  "sample",                 false,  CLEAN,  "") \
    macro(COVERGROUP_SAMPLE_IFFS,             "sample",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_SELECT_ALL,              "selectAll",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_AND,              "selectAnd",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_BIN,              "selectBin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM,              "selectDim",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM_انتہا,          "selectDimEnd",           false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_OR,               "selectOr",               false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_RANGE,            "selectRange",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SELECT_RANGE_W,          "selectRangeW",           false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_انتہا,               "sizedEnd",               خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_اختتام,            "sizedFinish",            false,  CLEAN,  "r+") \
    macro(COVERGROUP_SIZED_پہلا,             "sizedFirst",             خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_RANGE,             "sizedRange",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_RANGE_W,           "sizedRangeW",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_SAMPLE,            "sizedSample",            false,  CLEAN,  "rrr") \
    macro(COVERGROUP_SIZED_SAMPLE_W,          "sizedSampleW",           false,  CLEAN,  "rrr") \
    macro(COVERGROUP_TYPE_COVERAGE,           "typeCoverage",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_TYPE_وزن,             "typeWeight",             false,  CLEAN,  "r") \
    macro(COVERGROUP_قدر_EXCLUDED,          "valueExcluded",          خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_قدر_EXCLUDED_W,        "valueExcludedW",         خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_قدر_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_قدر_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_قدر_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_قدر_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_قدر_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_قدر_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_قدر_TYPE,              "valueType",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_وزن,                  "weight",                 false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_BEGIN,              "withBegin",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_اختتام,             "withFinish",             false,  CLEAN,  "r+") \
    macro(COVERGROUP_WITH_HI,                 "withHi",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_HI_W,               "withHiW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_LO,                 "withLo",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_LO_W,               "withLoW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_اگلا,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_تسلسل,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_تسلسل_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_عقب,           "atWriteAppendBack",      false,  CLEAN,  "r") \
    macro(DYN_CLEAR,                          "clear",                  false,  CLEAN,  "") \
    macro(DYN_ERASE,                          "erase",                  false,  CLEAN,  "r") \
    macro(DYN_داخل_کرو,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_نکالو,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_نکالو_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ڈالو,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ڈالو_FRONT,                     "push_front",             false,  CLEAN,  "r") \
    macro(DYN_RENEW,                          "renew",                  false,  CLEAN,  "r") \
    macro(DYN_RENEW_COPY,                     "renew_copy",             false,  CLEAN,  "rr") \
    macro(DYN_RESIZE,                         "resize",                 false,  CLEAN,  "TODO") \
    macro(DYN_سائز,                           "size",                   خالص_2,   CLEAN,  "") \
    macro(DYN_قاش,                          "slice",                  خالص_2,   CLEAN,  "rr") \
    macro(DYN_قاش_تفویض,                   "sliceAssign",            false,  CLEAN,  "rrr") \
    macro(DYN_قاش_تفویض_عقب_عقب,         "sliceAssignBackBack",    false,  CLEAN,  "rrr") \
    macro(DYN_قاش_تفویض_FRONT_عقب,        "sliceAssignFrontBack",   false,  CLEAN,  "rrr") \
    macro(DYN_قاش_عقب_عقب,                "sliceBackBack",          خالص_2,   CLEAN,  "rr") \
    macro(DYN_قاش_FRONT_عقب,               "sliceFrontBack",         خالص_2,   CLEAN,  "rr") \
    macro(EVENT_CLEAR_FIRED,                  "clearFired",             false,  CLEAN,  "") \
    macro(EVENT_CLEAR_TRIGGERED,              "clearTriggered",         false,  CLEAN,  "") \
    macro(EVENT_FIRE,                         "fire",                   false,  CLEAN,  "") \
    macro(EVENT_IS_FIRED,                     "isFired",                خالص_2,   CLEAN,  "") \
    macro(EVENT_IS_TRIGGERED,                 "isTriggered",            خالص_2,   CLEAN,  "") \
    macro(FORCE_جوڑو,                          "addForce",               false,  CLEAN,  "rrrr+") \
    macro(FORCE_READ,                         "read",                   خالص_2,   CLEAN,  "r") \
    macro(FORCE_READ_اشاریہ,                   "readIndex",              خالص_2,   CLEAN,  "rr") \
    macro(FORCE_READ_SEL,                     "readSel",                خالص_2,   CLEAN,  "TODO") \
    macro(FORCE_RELEASE,                      "release",                false,  CLEAN,  "rr+") \
    macro(FORCE_TOUCH,                        "touch",                  false,  CLEAN,  "") \
    macro(FORK_مکمل,                          "done",                   false,  CLEAN,  "rr") \
    macro(FORK_INIT,                          "init",                   false,  CLEAN,  "rr") \
    macro(FORK_JOIN,                          "join",                   false,  CLEAN,  "rrr") \
    macro(FORK_ON_KILL,                       "onKill",                 false,  CLEAN,  "r") \
    macro(NBA_COMMIT,                         "commit",                 false,  CLEAN,  "w") \
    macro(NBA_قطار_میں_ڈالو,                        "enqueue",                false,  CLEAN,  "r+") \
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
    macro(RANDOMIZER_سیٹ_VAR_DISABLED,        "set_var_disabled",       false,  CLEAN,  "r") \
    macro(RANDOMIZER_CLEAR_VAR_DISABLED,      "clear_var_disabled",     false,  CLEAN,  "r") \
    macro(RANDOMIZER_MARK_VAR_STATIC,         "mark_var_static",        false,  CLEAN,  "r") \
    macro(RANDOMIZER_سیٹ_STATIC_RANDMODE,     "set_static_randmode",    false,  CLEAN,  "r") \
    macro(RNG_GET_RANDSTATE,                  "__Vm_rng.get_randstate", خالص_2,   CLEAN,  "") \
    macro(RNG_سیٹ_RANDSTATE,                  "__Vm_rng.set_randstate", false,  CLEAN,  "r") \
    macro(SCHED_ANY_TRIGGERED,                "anyTriggered",           false,  CLEAN,  "r") \
    macro(SCHED_AWAITING_CURRENT_وقت,        "awaitingCurrentTime",    خالص_2,   CLEAN,  "") \
    macro(SCHED_AWAITING_ZERO_DELAY,          "awaitingZeroDelay",      خالص_2,   CLEAN,  "") \
    macro(SCHED_READY,                        "ready",                  false,  CLEAN,  "r") \
    macro(SCHED_MOVE_مقصد_RESUME_قطار,         "moveToResumeQueue",      false,  CLEAN,  "r") \
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
    macro(UNPACKED_تفویض,                    "assign",                 false,  CLEAN,  "r") \
    macro(UNPACKED_FILL,                      "fill",                   false,  CLEAN,  "r") \
    macro(UNPACKED_NEQ,                       "neq",                    خالص_2,   CLEAN,  "r")
// clang-format on

class VCMethod final {
    ساکن constexpr bool خالص_2 = true;  // For macro expansion of 'pure' field only
    ساکن constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

عوامی:
    شمار en : uint8_t {
#تعریف_کرو VL_CMETHOD_ID(id, طریقہ, خالص_3, clean, دلائل) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#تعریف_منسوخ VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    شمار en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساکن constexpr ثابت حرف* ثابت اقدار[] = {
#تعریف_کرو VL_CMETHOD_NAME(id, طریقہ, خالص_3, clean, دلائل) طریقہ,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#تعریف_منسوخ VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        واپس اقدار[m_e];
    }
    bool is_خالص() ثابت VL_خالص {
        ساکن constexpr bool اقدار[] = {
#تعریف_کرو VL_CMETHOD_خالص(id, طریقہ, خالص_3, clean, دلائل) خالص_3,
            FOR_EACH_CMETHOD(VL_CMETHOD_خالص)
#تعریف_منسوخ VL_CMETHOD_خالص
                false  //
        };
        واپس اقدار[m_e];
    }
    bool isClean() ثابت VL_خالص {
        ساکن constexpr bool اقدار[] = {
#تعریف_کرو VL_CMETHOD_CLEAN(id, طریقہ, خالص_3, clean, دلائل) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#تعریف_منسوخ VL_CMETHOD_CLEAN
                false  //
        };
        واپس اقدار[m_e];
    }
    ثابت حرف* دلائل() ثابت VL_خالص {
        ساکن constexpr ثابت حرف* ثابت اقدار[] = {
#تعریف_کرو VL_CMETHOD_دلائل(id, طریقہ, خالص_3, clean, دلائل) دلائل,
            FOR_EACH_CMETHOD(VL_CMETHOD_دلائل)
#تعریف_منسوخ VL_CMETHOD_دلائل
                ""  //
        };
        واپس اقدار[m_e];
    }
    // Return array method for given name
    ساکن VCMethod array_طریقہ(ثابت string& name);
};
constexpr bool عامل==(ثابت VCMethod& lhs, ثابت VCMethod& rhs) { واپس lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VCMethod& lhs, VCMethod::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VCMethod::en lhs, ثابت VCMethod& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VCMethod& rhs) {
    واپس os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#تعریف_کرو VL_CMETHOD_دلائل_CHECK(id, طریقہ, خالص_3, clean, دلائل) \
    static_assert(validate_دلائل_Descriptor(دلائل), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_دلائل_CHECK)
#تعریف_منسوخ VL_CMETHOD_دلائل_CHECK

#تعریف_منسوخ FOR_EACH_CMETHOD

// clang-format off
#تعریف_کرو FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_مقصد_SOLVER_HEX,           "vlToSolverHex",          خالص_2,   "r")
// clang-format on

class VCFunction final {
    ساکن constexpr bool خالص_2 = true;  // For macro expansion of 'pure' field only

عوامی:
    شمار en : uint8_t {
#تعریف_کرو VL_CFUNCTION_ID(id, طریقہ, خالص_3, دلائل) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#تعریف_منسوخ VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    شمار en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت VL_خالص {
        ساکن ثابت حرف* ثابت اقدار[] = {
#تعریف_کرو VL_CFUNCTION_NAME(id, function, خالص_3, دلائل) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#تعریف_منسوخ VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        واپس اقدار[m_e];
    }
    bool is_خالص() ثابت VL_خالص {
        ساکن ثابت bool اقدار[] = {
#تعریف_کرو VL_CFUNCTION_خالص(id, طریقہ, خالص_3, دلائل) خالص_3,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_خالص)
#تعریف_منسوخ VL_CFUNCTION_خالص
                false  //
        };
        واپس اقدار[m_e];
    }
    ثابت حرف* دلائل() ثابت VL_خالص {
        ساکن ثابت حرف* ثابت اقدار[] = {
#تعریف_کرو VL_CFUNCTION_دلائل(id, طریقہ, خالص_3, دلائل) دلائل,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_دلائل)
#تعریف_منسوخ VL_CFUNCTION_دلائل
                ""  //
        };
        واپس اقدار[m_e];
    }
};

// Static assert all argument descriptors are well formed
#تعریف_کرو VL_CFUNCTION_دلائل_CHECK(id, طریقہ, خالص_3, دلائل) \
    static_assert(validate_دلائل_Descriptor(دلائل), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_دلائل_CHECK)
#تعریف_منسوخ VL_CFUNCTION_دلائل_CHECK

#تعریف_منسوخ FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
عوامی:
    شمار en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

نجی:
    ساخت شے_2 final {
        شمار en m_e;  // Statement's enum mnemonic, for checking
        ثابت حرف* m_name;  // Statements name, for debugging
    };
    ساکن شے_2 s_شے_ڈیٹا[];

عوامی:
    شمار en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"none", "ctor_var_reset_call"};
        واپس نام[m_e];
    }
    bool isNone() ثابت { واپس m_e == NONE; }
};
constexpr bool عامل==(ثابت VCStmtType& lhs, ثابت VCStmtType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VCStmtType& lhs, VCStmtType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VCStmtType::en lhs, ثابت VCStmtType& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VCStmtType& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
عوامی:
    شمار en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    شمار en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VCaseType& lhs, ثابت VCaseType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VCaseType& lhs, VCaseType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VCaseType::en lhs, ثابت VCaseType& rhs) { واپس lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
عوامی:
    شمار en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        واپس نام[m_e];
    }
    bool isAssignable() ثابت { واپس m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
};
constexpr bool عامل==(ثابت VCastable& lhs, ثابت VCastable& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VCastable& lhs, VCastable::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VCastable::en lhs, ثابت VCastable& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VCastable& rhs) {
    واپس os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
عوامی:
    شمار en : uint8_t {
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
    شمار en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr عامل en() ثابت { واپس m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        واپس نام[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ثابت حرف* verilogKwd() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        واپس نام[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ثابت حرف* bin_سیٹ_Enum() ثابت {
        انتخاب (m_e) {
        حالت BINS_IGNORE: واپس "VlCovBinKind::KIND_IGNORE";
        حالت BINS_ILLEGAL: واپس "VlCovBinKind::KIND_ILLEGAL";
        حالت BINS_DEFAULT: واپس "VlCovBinKind::KIND_DEFAULT";
        طےشدہ: واپس "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool bin_Is_معمول() ثابت {
        واپس m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool عامل==(ثابت VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    واپس lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
عوامی:
    شمار en : uint8_t {
        // Shared by option.* and type_option.*
        وزن,
        GOAL,
        AT_کم_سے_کم,
        AUTO_BIN_MAX,
        فی_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_عدد_PRINT_MISSING,
        CROSS_RETAIN_AUTO_BINS,
        DETECT_OVERLAP,
        GET_INST_COVERAGE,
        // type_option.* only (IEEE 1800-2023 Table 19-3)
        STROBE,
        ملاؤ_INSTANCES,
        DISTRIBUTE_پہلا,
        REAL_INTERVAL,
        // Legacy option.* accepted for compatibility
        CROSS_AUTO_BIN_MAX,
        // sentinel - should never appear after parse-time validation
        UNKNOWN
    };
    شمار en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"weight",
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
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    واپس lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
عوامی:
    شمار en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    شمار en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(صحیح _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"", "[*]", "[->]", "[=]"};
        واپس نام[m_e];
    }
    ثابت حرف* asciiJson() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        واپس نام[m_e];
    }
};

//######################################################################

class VDirection final {
عوامی:
    شمار en : uint8_t { NONE, داخلہ, OUTPUT, INOUT, REF, CONSTREF };
    شمار en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت VL_MT_محفوظ { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        واپس نام[m_e];
    }
    string verilogKwd() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        واپس نام[m_e];
    }
    string prettyName() ثابت { واپس verilogKwd(); }
    bool isAny() ثابت { واپس m_e != NONE; }
    bool isInout() ثابت { واپس m_e == INOUT; }
    bool isInoutOrRef() ثابت { واپس m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_داخلہ() ثابت { واپس m_e == داخلہ; }
    bool isNonOutput() ثابت {
        واپس m_e == داخلہ || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ثابت { واپس m_e == OUTPUT; }
    bool isReadOnly() ثابت VL_MT_محفوظ { واپس m_e == داخلہ || m_e == CONSTREF; }
    bool isWritable() ثابت VL_MT_محفوظ { واپس m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ثابت VL_MT_محفوظ { واپس m_e == REF; }
    bool isConstRef() ثابت VL_MT_محفوظ { واپس m_e == CONSTREF; }
    string traceSigDirection() ثابت {
        اگر (isInout()) {
            واپس "VerilatedTraceSigDirection::INOUT";
        } ورنہ اگر (isWritable()) {
            واپس "VerilatedTraceSigDirection::OUTPUT";
        } ورنہ اگر (isNonOutput()) {
            واپس "VerilatedTraceSigDirection::INPUT";
        }
        واپس "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ثابت {
        انتخاب (m_e) {
        حالت NONE: واپس VAccess::NOCHANGE;
        حالت داخلہ:
        حالت CONSTREF: واپس VAccess::READ;
        حالت OUTPUT: واپس VAccess::WRITE;
        حالت INOUT:
        حالت REF: واپس VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool عامل==(ثابت VDirection& lhs, ثابت VDirection& rhs) VL_MT_محفوظ {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VDirection& lhs, VDirection::en rhs) VL_MT_محفوظ {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VDirection::en lhs, ثابت VDirection& rhs) VL_MT_محفوظ {
    واپس lhs == rhs.m_e;
}
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VDirection& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
عوامی:
    شمار en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    شمار en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    bool جوڑو_Newline() ثابت { واپس m_e != DT_WRITE; }
    bool needScopeTracking() ثابت { واپس m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VDisplayType& lhs, ثابت VDisplayType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VDisplayType& lhs, VDisplayType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VDisplayType::en lhs, ثابت VDisplayType& rhs) { واپس lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
عوامی:
    شمار en : uint8_t { FILE, VARS, ALL, FLUSH, حد_بندی_2, OFF, ON };
    شمار en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VDumpCtlType& lhs, ثابت VDumpCtlType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VDumpCtlType& lhs, VDumpCtlType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VDumpCtlType::en lhs, ثابت VDumpCtlType& rhs) { واپس lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
عوامی:
    // REMEMBER to edit the strings below too
    شمار en : uint8_t {
        // These must be in general -> most specific order, as we sort by it
        // in V3Const::visit AstSenTree
        // Involving a variable
        ET_بدلا,  // Value changed
        ET_BOTHEDGE,  // POSEDGE | NEGEDGE (i.e.: 'edge' in Verilog)
        ET_POSEDGE,
        ET_NEGEDGE,
        ET_EVENT,  // VlEventBase::isFired
        // Involving an expression
        ET_TRUE,
        ET_INITIAL_NBA,  // Event that is fired initially and never again
        //
        ET_COMBO,  // Sensitive to all combo inputs to this block
        ET_COMBO_ستارہ,  // Sensitive to all combo inputs to this block (from .*)
        ET_HYBRID,  // This is like ET_COMB, but with explicit sensitivity to an expression
        ET_STATIC,  // static variable initializers (runs before 'initial')
        ET_INITIAL,  // 'initial' statements
        ET_FINAL,  // 'final' statements
        ET_NEVER  // Never occurs (optimized away)
    };
    شمار en m_e;
    bool clockedStmt() ثابت {
        ساکن constexpr bool clocked[] = {
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
        واپس clocked[m_e];
    }
    bool an_کنارہ() ثابت { واپس m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ثابت {
        انتخاب (m_e) {
        حالت ET_بدلا: واپس ET_بدلا;
        حالت ET_BOTHEDGE: واپس ET_BOTHEDGE;
        حالت ET_POSEDGE: واپس ET_NEGEDGE;
        حالت ET_NEGEDGE: واپس ET_POSEDGE;
        طےشدہ: UASSERT_STATIC(0, "Inverting bad edgeType()"); واپس ET_NEGEDGE;
        }
    }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        واپس نام[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        واپس نام[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusive_کنارہ(ثابت VEdgeType& other) ثابت {
        انتخاب (m_e) {
        حالت VEdgeType::ET_POSEDGE:
            اگر (other.m_e == VEdgeType::ET_NEGEDGE) واپس true;
            توڑو;
        حالت VEdgeType::ET_NEGEDGE:
            اگر (other.m_e == VEdgeType::ET_POSEDGE) واپس true;
            توڑو;
        طےشدہ: توڑو;
        }
        واپس false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
};
constexpr bool عامل==(ثابت VEdgeType& lhs, ثابت VEdgeType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VEdgeType& lhs, VEdgeType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VEdgeType::en lhs, ثابت VEdgeType& rhs) { واپس lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#تعریف_کرو FOR_EACH_EVAL(macro) \
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
عوامی:
    شمار en : uint8_t {
#تعریف_کرو VL_EVAL_ID(id, iterated, triggers, پہلا, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#تعریف_منسوخ VL_EVAL_ID
            ENUM_انتہا
    };
    شمار en m_e;

    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت اقدار[] = {
#تعریف_کرو VL_EVAL_NAME(id, iterated, triggers, پہلا, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#تعریف_منسوخ VL_EVAL_NAME
                "_ENUM_END"  //
        };
        واپس اقدار[m_e];
    }
    bool isIterated() ثابت {
        ساکن constexpr bool اقدار[] = {
#تعریف_کرو VL_EVAL_IS_ITERATED(id, iterated, triggers, پہلا, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#تعریف_منسوخ VL_EVAL_IS_ITERATED
                false  //
        };
        واپس اقدار[m_e];
    }
    bool hasTriggers() ثابت {
        ساکن constexpr bool اقدار[] = {
#تعریف_کرو VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, پہلا, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#تعریف_منسوخ VL_EVAL_HAS_TRIGGERS
                false  //
        };
        واپس اقدار[m_e];
    }
    bool پہلا_Iteration() ثابت {
        ساکن constexpr bool اقدار[] = {
#تعریف_کرو VL_EVAL_پہلا(id, iterated, triggers, پہلا, slow) پہلا,
            FOR_EACH_EVAL(VL_EVAL_پہلا)
#تعریف_منسوخ VL_EVAL_پہلا
                false  //
        };
        واپس اقدار[m_e];
    }
    bool slow() ثابت {
        ساکن constexpr bool اقدار[] = {
#تعریف_کرو VL_EVAL_SLOW(id, iterated, triggers, پہلا, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#تعریف_منسوخ VL_EVAL_SLOW
                false  //
        };
        واپس اقدار[m_e];
    }

    // Short name
    std::string لیبل() ثابت { واپس VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ثابت { واپس "_eval_" + لیبل(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string eval_طریقہ() ثابت { واپس "eval" + capitalized_لیبل(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ثابت { واپس "_eval_dump_triggers__" + لیبل(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dump_Triggers_طریقہ() ثابت { واپس "dumpTriggers" + capitalized_لیبل(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(صحیح _e)
        : m_e(static_cast<en>(_e)) {}
    عامل en() ثابت { واپس m_e; }

نجی:
    std::string capitalized_لیبل() ثابت {
        ثابت std::string name = ascii();
        واپس name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#تعریف_منسوخ FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
عوامی:
    شمار en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        واپس نام[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    // Is a node type compatible with the declaration
    bool is_گرہ_Compatible(ثابت Ast_گرہ* nodep) ثابت;
};
constexpr bool عامل==(ثابت VFwdType& lhs, ثابت VFwdType& rhs) { واپس lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VFwdType& lhs, VFwdType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VFwdType::en lhs, ثابت VFwdType& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VFwdType& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    ساکن uint64_t s_cachedCntGbl;  // Global computed count

عوامی:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() ثابت { واپس m_cachedCnt == s_cachedCntGbl; }
    bool get() ثابت { واپس m_state; }
    خالی سیٹ(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    خالی clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ساکن خالی clear_Cache_درخت() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
عوامی:
    شمار en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    شمار en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        واپس نام[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"join", "join_any", "join_none"};
        واپس نام[m_e];
    }
    bool join() ثابت { واپس m_e == JOIN; }
    bool joinAny() ثابت { واپس m_e == JOIN_ANY; }
    bool joinNone() ثابت { واپس m_e == JOIN_NONE; }
};
constexpr bool عامل==(ثابت VJoinType& lhs, ثابت VJoinType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VJoinType& lhs, VJoinType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VJoinType::en lhs, ثابت VJoinType& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VJoinType& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
عوامی:
    شمار en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        واپس نام[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    bool isNone() ثابت { واپس m_e == NONE; }
    bool isAutomatic() ثابت { واپس m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ثابت { واپس m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ثابت { واپس m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ثابت {
        انتخاب (m_e) {
        حالت AUTOMATIC_EXPLICIT: واپس AUTOMATIC_IMPLICIT;
        حالت STATIC_EXPLICIT: واپس STATIC_IMPLICIT;
        طےشدہ: واپس m_e;
        }
    }
};
constexpr bool عامل==(ثابت VLifetime& lhs, ثابت VLifetime& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VLifetime& lhs, VLifetime::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VLifetime::en lhs, ثابت VLifetime& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VLifetime& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
عوامی:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            صحیح m_اشاریہ;  // Current index
            ثابت صحیح m_inc;  // Step, +1 or -1
        عوامی:
            Iterator(صحیح اشاریہ, صحیح inc)
                : m_اشاریہ{اشاریہ}
                , m_inc{inc} {}
            صحیح عامل*() ثابت { واپس m_اشاریہ; }
            Iterator& عامل++() {
                m_اشاریہ += m_inc;
                واپس *this;
            }
            bool عامل!=(ثابت Iterator& that) ثابت { واپس m_اشاریہ != that.m_اشاریہ; }
        };
        ثابت صحیح m_begin;  // First index
        ثابت صحیح m_انتہا;  // One past the last index
        ثابت صحیح m_inc;  // Step, +1 or -1
    عوامی:
        Sequence(صحیح پہلا, صحیح آخری, صحیح inc)
            : m_begin{پہلا}
            , m_انتہا{آخری + inc}
            , m_inc{inc} {}
        Iterator begin() ثابت { واپس Iterator{m_begin, m_inc}; }
        Iterator انتہا() ثابت { واپس Iterator{m_انتہا, m_inc}; }
    };

    صحیح m_بایاں = 0;  // Left side of range (pre-':')
    صحیح m_دایاں = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool عامل==(ثابت VNumRange& rhs) ثابت {
        واپس m_بایاں == rhs.m_بایاں && m_دایاں == rhs.m_دایاں && m_ranged == rhs.m_ranged;
    }
    bool عامل<(ثابت VNumRange& rhs) ثابت {
        اگر ((m_بایاں < rhs.m_بایاں)) واپس true;
        اگر (!(m_بایاں == rhs.m_بایاں)) واپس false;  // lhs > rhs
        اگر ((m_دایاں < rhs.m_دایاں)) واپس true;
        اگر (!(m_دایاں == rhs.m_دایاں)) واپس false;  // lhs > rhs
        اگر ((m_ranged < rhs.m_ranged)) واپس true;
        اگر (!(m_ranged == rhs.m_ranged)) واپس false;  // lhs > rhs
        واپس false;
    }
    //
    VNumRange() = طےشدہ;
    VNumRange(صحیح hi, صحیح lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(صحیح بایاں, صحیح دایاں)
        : m_بایاں{بایاں}
        , m_دایاں{دایاں}
        , m_ranged{true} {}
    ~VNumRange() = طےشدہ;
    // MEMBERS
    خالی init(صحیح hi, صحیح lo, bool ascending) {
        اگر (lo > hi) std::تبادلہ(hi, lo);
        m_بایاں = ascending ? lo : hi;
        m_دایاں = ascending ? hi : lo;
        m_ranged = true;
    }
    صحیح بایاں() ثابت { واپس m_بایاں; }
    صحیح دایاں() ثابت { واپس m_دایاں; }
    صحیح hi() ثابت VL_MT_محفوظ {
        واپس m_بایاں > m_دایاں ? m_بایاں : m_دایاں;
    }  // How to show a declaration
    صحیح lo() ثابت VL_MT_محفوظ {
        واپس m_بایاں > m_دایاں ? m_دایاں : m_بایاں;
    }  // How to show a declaration
    صحیح بایاں_مقصد_دایاں_Inc() ثابت { واپس ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_بایاں_مقصد_دایاں() ثابت { واپس {m_بایاں, m_دایاں, بایاں_مقصد_دایاں_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_دایاں_مقصد_بایاں() ثابت { واپس {m_دایاں, m_بایاں, -بایاں_مقصد_دایاں_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seq_Lo_مقصد_Hi() ثابت { واپس {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seq_Hi_مقصد_Lo() ثابت { واپس {hi(), lo(), -1}; }
    صحیح elements() ثابت VL_MT_محفوظ { واپس hi() - lo() + 1; }
    bool ranged() ثابت { واپس m_ranged; }
    bool ascending() ثابت { واپس m_بایاں < m_دایاں; }
    صحیح hiMaxSelect() ثابت {
        واپس (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ثابت {
        اگر (ranged()) {
            واپس "["s + std::مقصد_string(بایاں()) + ":" + std::مقصد_string(دایاں()) + "]";
        } ورنہ {
            واپس "[norg]";
        }
    }
    خالی dump(std::ostream& str) ثابت { str << ascii(); }
};
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VNumRange& rhs) {
    rhs.dump(os);
    واپس os;
}

// ######################################################################

class VPragmaType final {
عوامی:
    شمار en : uint8_t {
        COVERAGE_بلاک_OFF,
        HIER_بلاک,
        HIER_PARAMS,
        INLINE_MODULE,
        NO_INLINE_MODULE,
        NO_INLINE_TASK,
        عوامی_MODULE,
        عوامی_TASK,
        TIMEUNIT_سیٹ,
        UNROLL_DISABLE,
        UNROLL_بھرا,
        بھرا_CASE,
        PARALLEL_CASE,
        VERILATOR_LIB,
        ENUM_سائز
    };
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {
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
        واپس نام[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
};
constexpr bool عامل==(ثابت VPragmaType& lhs, ثابت VPragmaType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VPragmaType& lhs, VPragmaType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VPragmaType::en lhs, ثابت VPragmaType& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VPragmaType& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
عوامی:
    شمار en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        واپس نام[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr عامل en() ثابت { واپس m_e; }
    bool isRandomizable() ثابت { واپس m_e != NONE; }
    bool isRand() ثابت { واپس m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ثابت { واپس m_e == RAND_CYCLIC; }
};
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VRandAttr& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelf_اشارہ_گر_متن final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ساکن ثابت std::shared_ptr<ثابت string> s_emptyp;  // Holds ""
    ساکن ثابت std::shared_ptr<ثابت string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ثابت string> m_strp;

عوامی:
    // CONSTRUCTORS
    class خالی_خانے_2 {};  // for creator type-overload selection
    explicit VSelf_اشارہ_گر_متن(خالی_خانے_2)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelf_اشارہ_گر_متن(This)
        : m_strp{s_thisp} {}
    VSelf_اشارہ_گر_متن(This, ثابت string& میدان)
        : m_strp{std::make_shared<ثابت string>("this->" + میدان)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelf_اشارہ_گر_متن(VlSyms, ثابت string& میدان)
        : m_strp{std::make_shared<ثابت string>("(&vlSymsp->vlInst." + میدان + ')')} {}

    // METHODS
    bool is_خالی_خانے() ثابت { واپس m_strp == s_emptyp; }
    bool isVlSym() ثابت { واپس m_strp->ڈھونڈو("vlSymsp") != string::npos; }
    bool hasThis() ثابت { واپس m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ثابت;
    ساکن string replaceThis(bool useSelfForThis, ثابت string& متن);
    ثابت std::string& asString() ثابت { واپس *m_strp; }
    bool عامل==(ثابت VSelf_اشارہ_گر_متن& other) ثابت { واپس *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
عوامی:
    شمار en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        واپس نام[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    ساکن VSigning fromBool(bool isSigned) {  // Factory method
        واپس isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    bool isSigned() ثابت VL_MT_محفوظ { واپس m_e == SIGNED; }
    bool isNosign() ثابت VL_MT_محفوظ { واپس m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool عامل==(ثابت VSigning& lhs, ثابت VSigning& rhs) { واپس lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VSigning& lhs, VSigning::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VSigning::en lhs, ثابت VSigning& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VSigning& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

class VStrength final {
عوامی:
    شمار en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    شمار en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VStrength& lhs, ثابت VStrength& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VStrength& lhs, VStrength::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VStrength::en lhs, ثابت VStrength& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VStrength& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
عوامی:
    شمار en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    شمار en m_e;
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        واپس نام[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr عامل en() ثابت { واپس m_e; }
};

//######################################################################

class VTrace_سابقہ_Type final {
عوامی:
    شمار en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    شمار en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTrace_سابقہ_Type(en _e)
        : m_e{_e} {}
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VTrace_سابقہ_Type& lhs, ثابت VTrace_سابقہ_Type& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VTrace_سابقہ_Type& lhs, VTrace_سابقہ_Type::en rhs) {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VTrace_سابقہ_Type::en lhs, ثابت VTrace_سابقہ_Type& rhs) {
    واپس lhs == rhs.m_e;
}
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VTrace_سابقہ_Type& rhs) {
    واپس os << rhs.ascii();
}

//######################################################################

class VTraceType final {
عوامی:
    شمار en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        بھرا,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    شمار en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"CONSTANT", "FULL", "CHANGE"};
        واپس نام[m_e];
    }
    ثابت حرف* func_سابقہ() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"trace_const", "trace_full", "trace_chg"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VTraceType& lhs, ثابت VTraceType& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VTraceType& lhs, VTraceType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VTraceType::en lhs, ثابت VTraceType& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VTraceType& rhs) {
    واپس os << rhs.ascii();
}

//######################################################################

class VUseType final {
عوامی:
    شمار en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    شمار en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    bool شامل_ہے_Any(VUseType other) ثابت { واپس m_e & other.m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        واپس نام[m_e - 1];
    }
};
constexpr bool عامل==(ثابت VUseType& lhs, ثابت VUseType& rhs) { واپس lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VUseType& lhs, VUseType::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VUseType::en lhs, ثابت VUseType& rhs) { واپس lhs == rhs.m_e; }
constexpr VUseType::en عامل|(VUseType::en lhs, VUseType::en rhs) {
    واپس VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en عامل&(VUseType::en lhs, VUseType::en rhs) {
    واپس VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VUseType& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

class VVarType final {
عوامی:
    شمار en : uint8_t {
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
        پورٹ,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    شمار en m_e;
    VVarType() VL_MT_محفوظ : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_محفوظ : m_e{_e} {}
    explicit VVarType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن constexpr ثابت حرف* ثابت نام[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        واپس نام[m_e];
    }
    bool isParam() ثابت { واپس m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ثابت {
        واپس (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == پورٹ || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ثابت {
        واپس (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ثابت { واپس (m_e == TRIOR); }
    bool isWiredNet() ثابت { واپس (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ثابت {  // In Verilog, always ok in SystemVerilog
        واپس (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == پورٹ || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ثابت {
        واپس (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool is_عارضی() ثابت {
        واپس (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ثابت {
        واپس (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == پورٹ
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ثابت حرف* trace_Sig_قسم() ثابت {
        // VerilatedTraceSigKind to used in trace signal declaration
        ساکن constexpr ثابت حرف* ثابت lut[] = {
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
        واپس lut[m_e];
    }
};
constexpr bool عامل==(ثابت VVarType& lhs, ثابت VVarType& rhs) VL_MT_محفوظ {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VVarType& lhs, VVarType::en rhs) VL_MT_محفوظ {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VVarType::en lhs, ثابت VVarType& rhs) VL_MT_محفوظ {
    واپس lhs == rhs.m_e;
}
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VVarType& rhs) VL_MT_محفوظ {
    واپس os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_کلید final {
عوامی:
    ثابت صحیح m_چوڑائی;  // From AstNodeDType: Bit width of operation
    ثابت صحیح m_چوڑائی_Min;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ثابت VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ثابت VSigning m_numeric;  // From AstNodeDType: Node is signed
    ثابت VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool عامل==(ثابت VBasic_Type_کلید& rhs) ثابت {
        واپس m_چوڑائی == rhs.m_چوڑائی && m_چوڑائی_Min == rhs.m_چوڑائی_Min && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool عامل<(ثابت VBasic_Type_کلید& rhs) ثابت {
        اگر ((m_چوڑائی < rhs.m_چوڑائی)) واپس true;
        اگر (!(m_چوڑائی == rhs.m_چوڑائی)) واپس false;  // lhs > rhs
        اگر ((m_چوڑائی_Min < rhs.m_چوڑائی_Min)) واپس true;
        اگر (!(m_چوڑائی_Min == rhs.m_چوڑائی_Min)) واپس false;  // lhs > rhs
        اگر ((m_numeric < rhs.m_numeric)) واپس true;
        اگر (!(m_numeric == rhs.m_numeric)) واپس false;  // lhs > rhs
        اگر ((m_keyword < rhs.m_keyword)) واپس true;
        اگر (!(m_keyword == rhs.m_keyword)) واپس false;  // lhs > rhs
        اگر ((m_nrange < rhs.m_nrange)) واپس true;
        اگر (!(m_nrange == rhs.m_nrange)) واپس false;  // lhs > rhs
        واپس false;
    }
    VBasic_Type_کلید(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric, VBasicDTypeKwd kwd,
                  ثابت VNumRange& nrange)
        : m_چوڑائی{چوڑائی}
        , m_چوڑائی_Min{چوڑائی_Min}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_کلید() = طےشدہ;
};

#اگر_ختم  // Guard
