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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ASTATTR_H_
#وصف_ڪريو VERILATOR_V3ASTATTR_H_

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3AST_H_
#غلطي "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#وصف_ڪريو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#جيڪڏهن_پورو

// Hint class so we can choose constructors
class VFlag_ٻٽو_انگ_Packed {};
class VFlag_ٻار_DType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbort_قسم final {
عام:
    // IEEE 1800-2023 16.12.14 property abort operators.
    تعداد en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbort_قسم(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت نالا[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        أرجع نالا[m_e];
    }
    bool isAccept() ثابت { أرجع m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
عام:
    تعداد en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"RD", "WR", "RW", "--"};
        أرجع نالا[m_e];
    }
    ثابت حرف* arrow() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        أرجع نالا[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    VAccess invert() ثابت {
        أرجع (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ثابت { أرجع m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ثابت { أرجع m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ثابت { أرجع m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ثابت { أرجع m_e == WRITE || m_e == READWRITE; }
    bool isRW() ثابت { أرجع m_e == READWRITE; }
};
constexpr bool عامل==(ثابت VAccess& lhs, ثابت VAccess& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VAccess& lhs, VAccess::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VAccess::en lhs, ثابت VAccess& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VAccess& rhs) { أرجع os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
عام:
    تعداد en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_سونپيو };
    تعداد en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VAlwaysKwd& lhs, ثابت VAlwaysKwd& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VAlwaysKwd::en lhs, ثابت VAlwaysKwd& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
عام:
    // IEEE 1800-2023 Table 20-5
    تعداد en : uint8_t {
        منزل_BE_EVALUATED = 0,
        تالو = 1,
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
    تعداد en m_e;
    VAssertCtlType()
        : m_e{منزل_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        // IEEE 1800-2023 20.11
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"",
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
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VAssertCtlType& lhs, ثابت VAssertCtlType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل==(VAssertCtlType::en lhs, ثابت VAssertCtlType& rhs) {
    أرجع lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
عام:
    // IEEE 1800-2023 Table 20-7
    تعداد en : uint8_t {
        اندروني = 0,  // Non IEEE type, for directives to be evaluated from expression.
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
    تعداد en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ثابت {
        std::stringstream types;
        إذا (m_e == اندروني)
            types << "INTERNAL ";
        وإلا {
            إذا (m_e & ASSERT) types << "ASSERT ";
            إذا (m_e & COVER) types << "COVER ";
            إذا (m_e & ASSUME) types << "ASSUME ";
            إذا (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            إذا (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            إذا (m_e & INTRINSIC) types << "INTRINSIC ";
            إذا (m_e & RESTRICT) types << "RESTRICT ";
        }
        ثابت string str = types.str();
        UASSERT(!str.خالي_جايون(), "Assert should be of one of types");
        أرجع str.substr(0, str.سائيز() - 1);
    }
    constexpr عامل en() ثابت { أرجع m_e; }
};
constexpr bool عامل==(ثابت VAssertDirectiveType& lhs, ثابت VAssertDirectiveType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل==(VAssertDirectiveType::en lhs, ثابت VAssertDirectiveType& rhs) {
    أرجع lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en عامل|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    أرجع VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
عام:
    // IEEE 1800-2023 Table 20-6
    تعداد en : uint8_t {
        اندروني = 0,  // Non IEEE type, for assertions that should not be controlled.
        CONCURRENT = (1 << 0),
        SIMPLE_فوري = (1 << 1),
        OBSERVED_DEFERRED_فوري = (1 << 2),
        FINAL_DEFERRED_فوري = (1 << 3),
        EXPECT = (1 << 4),
        UNIQUE = (1 << 5),
        UNIQUE0 = (1 << 6),
        ترجيح = (1 << 7),
    };
    تعداد en m_e;
    VAssertType()
        : m_e{اندروني} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool شامل_آهي_Any(VAssertType other) ثابت { أرجع m_e & other.m_e; }
    string ascii() ثابت {
        std::stringstream types;
        إذا (m_e == اندروني)
            types << "INTERNAL ";
        وإلا {
            إذا (m_e & CONCURRENT) types << "CONCURRENT ";
            إذا (m_e & SIMPLE_فوري) types << "SIMPLE_IMMEDIATE ";
            إذا (m_e & OBSERVED_DEFERRED_فوري) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            إذا (m_e & FINAL_DEFERRED_فوري) types << "FINAL_DEFERRED_IMMEDIATE ";
            إذا (m_e & EXPECT) types << "EXPECT ";
            إذا (m_e & UNIQUE) types << "UNIQUE ";
            إذا (m_e & UNIQUE0) types << "UNIQUE0 ";
            إذا (m_e & ترجيح) types << "PRIORITY ";
        }
        ثابت string str = types.str();
        UASSERT(!str.خالي_جايون(), "Assert should be of one of types");
        أرجع str.substr(0, str.سائيز() - 1);
    }
    constexpr عامل en() ثابت { أرجع m_e; }
};
constexpr bool عامل==(ثابت VAssertType& lhs, ثابت VAssertType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAssertType& lhs, VAssertType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VAssertType::en lhs, ثابت VAssertType& rhs) { أرجع lhs == rhs.m_e; }
constexpr VAssertType::en عامل|(VAssertType::en lhs, VAssertType::en rhs) {
    أرجع VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
عام:
    تعداد en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"default", "weak", "strong"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VPropStrength& lhs, ثابت VPropStrength& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل!=(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    أرجع lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
عام:
    // clang-format off
    تعداد en: uint8_t  {
        ILLEGAL,
        //
        DIM_بٽ,                       // V3Const converts to constant
        DIM_بٽ_OR_عدد,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_مٿاهون,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_کاٻو,                       // V3Width processes
        DIM_هيٺيون,                        // V3Width processes
        DIM_ساڄو,                      // V3Width processes
        DIM_سائيز,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_عوامي,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_پهريون,                     // V3Width processes
        ENUM_آخري,                      // V3Width processes
        ENUM_انگ,                       // V3Width processes
        ENUM_ايندڙ,                      // V3Width processes
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
        VAR_بنياد,                       // V3LinkResolve creates for AstPreSel, V3LinkParam removes
        VAR_FORCEABLE,                  // V3LinkParse moves to AstVar::isForceable
        VAR_FSM_ARC_INCLUDE_COND,       // V3LinkParse moves to AstVar::attrFsmArcInclCond
        VAR_FSM_ARC_INCLUDE_COND_AUTO,  // Flavors of above option - setting expand mode per FSM
        VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND,
        VAR_FSM_ARC_INCLUDE_COND_ڀريل,
        VAR_FSM_STATE,                  // V3LinkParse moves to AstVar::attrFsmState
        VAR_FSM_STATE_AUTO,             // Flavors of above option - setting expand mode per FSM
        VAR_FSM_STATE_AUTO_EXPAND,
        VAR_FSM_STATE_ڀريل,
        VAR_پورٽ_DTYPE,                 // V3LinkDot for V3Width to check port dtype
        VAR_عوامي,                     // V3LinkParse moves to AstVar::sigPublic
        VAR_عوامي_FLAT,                // V3LinkParse moves to AstVar::sigPublic
        VAR_عوامي_FLAT_RD,             // V3LinkParse moves to AstVar::sigPublic
        VAR_عوامي_FLAT_RW,             // V3LinkParse moves to AstVar::sigPublic
        VAR_SC_BIGUINT,                 // V3LinkParse moves to AstVar::attrScBigUint
        VAR_SC_BV,                      // V3LinkParse moves to AstVar::attrScBv
        VAR_SFORMAT,                    // V3LinkParse moves to AstVar::attrSFormat
        VAR_ورهايو_VAR                   // V3LinkParse moves to AstVar::attrSplitVar
    };
    // clang-format on
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        // clang-format off
        ساكن constexpr ثابت حرف* ثابت نالا[] = {
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
        أرجع نالا[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ثابت {
        بدل (m_e) {
        حالة DIM_بٽ:
        حالة DIM_بٽ_OR_عدد:
        حالة DIM_DIMENSIONS:
        حالة DIM_مٿاهون:
        حالة DIM_INCREMENT:
        حالة DIM_کاٻو:
        حالة DIM_هيٺيون:
        حالة DIM_ساڄو:
        حالة DIM_سائيز:
        حالة DIM_UNPK_DIMENSIONS:
        حالة TYPEID:
        حالة TYPENAME: أرجع true;
        افتراضي: أرجع false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
};
constexpr bool عامل==(ثابت VAttrType& lhs, ثابت VAttrType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VAttrType& lhs, VAttrType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VAttrType::en lhs, ثابت VAttrType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

عام:
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
    فراغ combine(ثابت VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ثابت { أرجع m_extends | m_final | m_initial; }
    bool isExtends() ثابت { أرجع m_extends; }
    bool isFinal() ثابت { أرجع m_final; }
    bool isInitial() ثابت { أرجع m_initial; }
    string ascii() ثابت {
        string ٻاهريون;
        إذا (m_initial) ٻاهريون = VString::dot(ٻاهريون, " ", "initial");
        إذا (m_extends) ٻاهريون = VString::dot(ٻاهريون, " ", "extends");
        إذا (m_final) ٻاهريون = VString::dot(ٻاهريون, " ", "final");
        أرجع ٻاهريون;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
عام:
    تعداد en : uint8_t {
        UNKNOWN,
        ٻٽو_انگ,
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
    تعداد en m_e;
    ثابت حرف* ascii() ثابت VL_MT_محفوظ {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"%E-unk",
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
        أرجع نالا[m_e];
    }
    ثابت حرف* typeName() ثابت VL_MT_محفوظ {
        بدل (m_e) {
        حالة LOGIC2STATE: أرجع VBasicDTypeKwd{LOGIC}.ascii();
        حالة INTEGER2STATE: أرجع VBasicDTypeKwd{INTEGER}.ascii();
        حالة TIME2STATE: أرجع VBasicDTypeKwd{وقت}.ascii();
        افتراضي: أرجع ascii();
        }
    }
    ثابت حرف* dpiType() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"%E-unk",
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
        أرجع نالا[m_e];
    }
    ساكن فراغ selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_محفوظ : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_محفوظ : m_e{_e} {}
    explicit VBasicDTypeKwd(صحيح _e) VL_MT_محفوظ  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    صحيح ويڪر() ثابت {
        بدل (m_e) {
        حالة ٻٽو_انگ: أرجع 1;  // scalar, can't bit extract unless ranged
        حالة BYTE: أرجع 8;
        حالة CHANDLE: أرجع 64;
        حالة NULLTYPE: أرجع 1;  // Historical - bit was used instead of NULLTYPE
        حالة EVENT: أرجع 1;
        حالة INT: أرجع 32;
        حالة INTEGER: أرجع 32;
        حالة INTEGER2STATE: أرجع 32;
        حالة LOGIC: أرجع 1;  // scalar, can't bit extract unless ranged
        حالة LOGIC2STATE: أرجع 1;  // scalar, can't bit extract unless ranged
        حالة LONGINT: أرجع 64;
        حالة DOUBLE: أرجع 64;  // opaque
        حالة SHORTINT: أرجع 16;
        حالة وقت: أرجع 64;
        حالة TIME2STATE: أرجع 64;
        حالة STRING: أرجع 64;  // opaque  // Just the pointer, for today
        حالة SCOPEPTR: أرجع 0;  // opaque
        حالة CHARPTR: أرجع 0;  // opaque
        حالة MTASKSTATE: أرجع 0;  // opaque
        حالة DELAY_SCHEDULER: أرجع 0;  // opaque
        حالة TRIGGER_SCHEDULER: أرجع 0;  // opaque
        حالة DYNAMIC_TRIGGER_SCHEDULER: أرجع 0;  // opaque
        حالة FORK_SYNC: أرجع 0;  // opaque
        حالة PROCESS_REFERENCE: أرجع 0;  // opaque
        حالة RANDOM_GENERATOR: أرجع 0;  // opaque
        حالة RANDOM_STDGENERATOR: أرجع 0;  // opaque
        حالة COVERGROUP_INSTHANDLE: أرجع 0;  // opaque
        حالة UINT32: أرجع 32;
        حالة UINT64: أرجع 64;
        افتراضي: أرجع 0;
        }
    }
    bool isSigned() ثابت {
        أرجع m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ثابت {
        أرجع m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == ٻٽو_انگ || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == وقت || m_e == TIME2STATE;
    }
    bool isFourstate() ثابت {
        أرجع m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == وقت;
    }
    bool isZeroInit() ثابت {  // Otherwise initializes to X
        أرجع (m_e == ٻٽو_انگ || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ثابت {  // Enum increment supported
        أرجع (m_e == ٻٽو_انگ || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == وقت || m_e == TIME2STATE);
    }
    bool is_ٻٽو_انگ() ثابت { أرجع m_e == ٻٽو_انگ; }
    bool is_ٻٽو_انگ_Logic() ثابت {  // Bit/logic vector types; can form a packed array
        أرجع (m_e == LOGIC || m_e == LOGIC2STATE || m_e == ٻٽو_انگ);
    }
    bool isDpiUnsignable() ثابت {  // Can add "unsigned" to DPI
        أرجع (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ثابت {  // Uses standard C layout, for DPI runtime access
        أرجع (m_e == ٻٽو_انگ || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ثابت VL_MT_محفوظ {  // IE not a simple number we can bit optimize
        أرجع (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ثابت VL_MT_محفوظ { أرجع m_e == CHANDLE; }
    bool isDouble() ثابت VL_MT_محفوظ { أرجع m_e == DOUBLE; }
    bool isEvent() ثابت { أرجع m_e == EVENT; }
    bool isString() ثابت VL_MT_محفوظ { أرجع m_e == STRING; }
    bool isMTaskState() ثابت VL_MT_محفوظ { أرجع m_e == MTASKSTATE; }
    bool isSameish(ثابت VBasicDTypeKwd& other) ثابت VL_MT_محفوظ {
        بدل (other.m_e) {
        حالة LOGIC:
        حالة LOGIC2STATE:
        حالة LOGIC_IMPLICIT: أرجع m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        حالة INTEGER:
        حالة INTEGER2STATE: أرجع m_e == INTEGER || m_e == INTEGER2STATE;
        حالة وقت:
        حالة TIME2STATE: أرجع m_e == وقت || m_e == TIME2STATE;
        افتراضي: أرجع m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_محفوظ {
        بدل (m_e) {
        حالة ٻٽو_انگ:
        حالة BYTE:
        حالة CHANDLE:
        حالة INT:
        حالة INTEGER:
        حالة INTEGER2STATE:
        حالة LOGIC:
        حالة LOGIC2STATE:
        حالة LONGINT:
        حالة DOUBLE:
        حالة SHORTINT:
        حالة SCOPEPTR:
        حالة CHARPTR:
        حالة UINT32:
        حالة UINT64: أرجع true;
        افتراضي: أرجع false;
        }
    }

    ثابت حرف* traceSigType() ثابت {
        // VerilatedTraceSigType to used in trace signal declaration
        ساكن constexpr ثابت حرف* ثابت lut[] = {
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
        أرجع lut[m_e];
    }
};
constexpr bool عامل==(ثابت VBasicDTypeKwd& lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_محفوظ {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_محفوظ {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل==(VBasicDTypeKwd::en lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_محفوظ {
    أرجع lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
عام:
    تعداد en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, ENUM_پڇاڙي };
    تعداد en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"FALSE", "TRUE", "UNK"};
        أرجع نالا[m_e];
    }
    bool is_معلوم() ثابت { أرجع m_e != BU_UNKNOWN; }
    bool true_معلوم() ثابت { أرجع m_e == BU_TRUE; }
    فراغ سيٽ_True_Or_False(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool عامل==(ثابت VBoolOrUnknown& lhs, ثابت VBoolOrUnknown& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل==(VBoolOrUnknown::en lhs, ثابت VBoolOrUnknown& rhs) {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VBoolOrUnknown& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
عام:
    تعداد en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, ENUM_پڇاڙي };
    تعداد en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    bool unknown() ثابت { أرجع m_e == BP_UNKNOWN; }
    bool likely() ثابت { أرجع m_e == BP_LIKELY; }
    bool unlikely() ثابت { أرجع m_e == BP_UNLIKELY; }
    VBranchPred invert() ثابت {
        إذا (m_e == BP_UNLIKELY) أرجع BP_LIKELY;
        إذا (m_e == BP_LIKELY) أرجع BP_UNLIKELY;
        أرجع m_e;
    }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        أرجع نالا[m_e];
    }
    ثابت حرف* asciiShort() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"", "L", "!L"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VBranchPred& lhs, ثابت VBranchPred& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VBranchPred& lhs, VBranchPred::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VBranchPred::en lhs, ثابت VBranchPred& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VBranchPred& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ساكن constexpr bool validate_دليلون_Descriptor(ثابت حرف* descrp) {
    // Is "TODO"
    إذا (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        أرجع true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    لكل (ثابت حرف* cp = descrp; *cp; ++cp) {
        إذا (*cp == '+') أرجع cp != descrp && !cp[1];
        إذا (*cp != 'r' && *cp != 'w' && *cp != 'm') أرجع false;
    }
    أرجع true;
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
#وصف_ڪريو FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     خالص_2,   CLEAN,  "r") \
    macro(ARRAY_AT_پٺ,                      "atBack",                 خالص_2,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                خالص_2,   CLEAN,  "r") \
    macro(ARRAY_ڳوليو,                         "find",                   خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڳوليو_پهريون,                   "find_first",             خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڳوليو_پهريون_اشاريو,             "find_first_index",       خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڳوليو_اشاريو,                   "find_index",             خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڳوليو_آخري,                    "find_last",              خالص_2,   CLEAN,  "") \
    macro(ARRAY_ڳوليو_آخري_اشاريو,              "find_last_index",        خالص_2,   CLEAN,  "") \
    macro(ARRAY_پهريون,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 خالص_2,   CLEAN,  "r") \
    macro(ARRAY_آخري,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_ايندڙ,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_ڪڍو_پٺ,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_ڪڍو_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ڌڪو_پٺ,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ڌڪو_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_ابتو,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  خالص_2,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   خالص_2,   CLEAN,  "") \
    macro(ARRAY_R_حاصل_ضرب,                    "r_product",              خالص_2,   false,  "") \
    macro(ARRAY_R_جوڙ,                        "r_sum",                  خالص_2,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  خالص_2,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_ٽڪرو,                        "slice",                  خالص_2,   CLEAN,  "r") \
    macro(ARRAY_ترتيب_ڏيو,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 خالص_2,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_اشاريو,                 "unique_index",           خالص_2,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 خالص_2,   CLEAN,  "r") \
    macro(ASSOC_پهريون,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_ايندڙ,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_سائيز,                         "size",                   خالص_2,   CLEAN,  "") \
    macro(CLASS_سيٽ_RANDMODE,                 "set_randmode",           false,  CLEAN,  "r") \
    macro(COVERGROUP_جوڙيو_ARRAY_NAMER,         "addArrayNamer",          false,  CLEAN,  "r+") \
    macro(COVERGROUP_جوڙيو_BIN,                 "addBin",                 false,  CLEAN,  "r+") \
    macro(COVERGROUP_جوڙيو_COVERPOINT,          "addCoverpoint",          false,  CLEAN,  "") \
    macro(COVERGROUP_جوڙيو_CROSS,               "addCross",               false,  CLEAN,  "") \
    macro(COVERGROUP_جوڙيو_CROSS_DYN,           "addCrossDyn",            false,  CLEAN,  "") \
    macro(COVERGROUP_جوڙيو_NUMBERED_NAMER,      "addNumberedNamer",       false,  CLEAN,  "r+") \
    macro(COVERGROUP_جوڙيو_SINGLE_NAMER,        "addSingleNamer",         false,  CLEAN,  "r+") \
    macro(COVERGROUP_ATTACH,                  "attach",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_CLEAR_ڪاميابي_LIST,          "clearHitList",           false,  CLEAN,  "") \
    macro(COVERGROUP_COVERAGE,                "coverage",               false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_BEGIN,        "withBegin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_CROSS_WITH_پڇاڙي,          "withEnd",                false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_TEST,         "withTest",               false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_قدر,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_قدر_W,      "withValueW",             false,  CLEAN,  "rw") \
    macro(COVERGROUP_FINALIZE_BINS,           "finalizeBins",           false,  CLEAN,  "") \
    macro(COVERGROUP_INCREMENT_BIN,           "incrementBin",           false,  CLEAN,  "r") \
    macro(COVERGROUP_INIT,                    "init",                   false,  CLEAN,  "r+") \
    macro(COVERGROUP_INST_P,                  "p",                      خالص_2,   CLEAN,  "") \
    macro(COVERGROUP_LEND_وزن,             "lendWeight",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_رڪارڊ_ڪاميابي,              "recordHit",              false,  CLEAN,  "r") \
    macro(COVERGROUP_رجسٽر_BINS,           "registerBins",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_SAMPLE,                  "sample",                 false,  CLEAN,  "") \
    macro(COVERGROUP_SAMPLE_IFFS,             "sample",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_SELECT_ALL,              "selectAll",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_AND,              "selectAnd",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_BIN,              "selectBin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM,              "selectDim",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM_پڇاڙي,          "selectDimEnd",           false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_OR,               "selectOr",               false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_RANGE,            "selectRange",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SELECT_RANGE_W,          "selectRangeW",           false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_پڇاڙي,               "sizedEnd",               خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_پڄاڻي,            "sizedFinish",            false,  CLEAN,  "r+") \
    macro(COVERGROUP_SIZED_پهريون,             "sizedFirst",             خالص_2,   CLEAN,  "r") \
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
    macro(COVERGROUP_WITH_پڄاڻي,             "withFinish",             false,  CLEAN,  "r+") \
    macro(COVERGROUP_WITH_HI,                 "withHi",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_HI_W,               "withHiW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_LO,                 "withLo",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_LO_W,               "withLoW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_ايندڙ,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_لاڳيتو,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_لاڳيتو_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_پٺ,           "atWriteAppendBack",      false,  CLEAN,  "r") \
    macro(DYN_CLEAR,                          "clear",                  false,  CLEAN,  "") \
    macro(DYN_ERASE,                          "erase",                  false,  CLEAN,  "r") \
    macro(DYN_وجهو,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_ڪڍو,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_ڪڍو_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ڌڪو,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ڌڪو_FRONT,                     "push_front",             false,  CLEAN,  "r") \
    macro(DYN_RENEW,                          "renew",                  false,  CLEAN,  "r") \
    macro(DYN_RENEW_COPY,                     "renew_copy",             false,  CLEAN,  "rr") \
    macro(DYN_RESIZE,                         "resize",                 false,  CLEAN,  "TODO") \
    macro(DYN_سائيز,                           "size",                   خالص_2,   CLEAN,  "") \
    macro(DYN_ٽڪرو,                          "slice",                  خالص_2,   CLEAN,  "rr") \
    macro(DYN_ٽڪرو_سونپيو,                   "sliceAssign",            false,  CLEAN,  "rrr") \
    macro(DYN_ٽڪرو_سونپيو_پٺ_پٺ,         "sliceAssignBackBack",    false,  CLEAN,  "rrr") \
    macro(DYN_ٽڪرو_سونپيو_FRONT_پٺ,        "sliceAssignFrontBack",   false,  CLEAN,  "rrr") \
    macro(DYN_ٽڪرو_پٺ_پٺ,                "sliceBackBack",          خالص_2,   CLEAN,  "rr") \
    macro(DYN_ٽڪرو_FRONT_پٺ,               "sliceFrontBack",         خالص_2,   CLEAN,  "rr") \
    macro(EVENT_CLEAR_FIRED,                  "clearFired",             false,  CLEAN,  "") \
    macro(EVENT_CLEAR_TRIGGERED,              "clearTriggered",         false,  CLEAN,  "") \
    macro(EVENT_FIRE,                         "fire",                   false,  CLEAN,  "") \
    macro(EVENT_IS_FIRED,                     "isFired",                خالص_2,   CLEAN,  "") \
    macro(EVENT_IS_TRIGGERED,                 "isTriggered",            خالص_2,   CLEAN,  "") \
    macro(FORCE_جوڙيو,                          "addForce",               false,  CLEAN,  "rrrr+") \
    macro(FORCE_READ,                         "read",                   خالص_2,   CLEAN,  "r") \
    macro(FORCE_READ_اشاريو,                   "readIndex",              خالص_2,   CLEAN,  "rr") \
    macro(FORCE_READ_SEL,                     "readSel",                خالص_2,   CLEAN,  "TODO") \
    macro(FORCE_RELEASE,                      "release",                false,  CLEAN,  "rr+") \
    macro(FORCE_TOUCH,                        "touch",                  false,  CLEAN,  "") \
    macro(FORK_پورو,                          "done",                   false,  CLEAN,  "rr") \
    macro(FORK_INIT,                          "init",                   false,  CLEAN,  "rr") \
    macro(FORK_JOIN,                          "join",                   false,  CLEAN,  "rrr") \
    macro(FORK_ON_KILL,                       "onKill",                 false,  CLEAN,  "r") \
    macro(NBA_COMMIT,                         "commit",                 false,  CLEAN,  "w") \
    macro(NBA_قطار_اندر_وجهو,                        "enqueue",                false,  CLEAN,  "r+") \
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
    macro(RANDOMIZER_سيٽ_VAR_DISABLED,        "set_var_disabled",       false,  CLEAN,  "r") \
    macro(RANDOMIZER_CLEAR_VAR_DISABLED,      "clear_var_disabled",     false,  CLEAN,  "r") \
    macro(RANDOMIZER_MARK_VAR_STATIC,         "mark_var_static",        false,  CLEAN,  "r") \
    macro(RANDOMIZER_سيٽ_STATIC_RANDMODE,     "set_static_randmode",    false,  CLEAN,  "r") \
    macro(RNG_GET_RANDSTATE,                  "__Vm_rng.get_randstate", خالص_2,   CLEAN,  "") \
    macro(RNG_سيٽ_RANDSTATE,                  "__Vm_rng.set_randstate", false,  CLEAN,  "r") \
    macro(SCHED_ANY_TRIGGERED,                "anyTriggered",           false,  CLEAN,  "r") \
    macro(SCHED_AWAITING_CURRENT_وقت,        "awaitingCurrentTime",    خالص_2,   CLEAN,  "") \
    macro(SCHED_AWAITING_ZERO_DELAY,          "awaitingZeroDelay",      خالص_2,   CLEAN,  "") \
    macro(SCHED_READY,                        "ready",                  false,  CLEAN,  "r") \
    macro(SCHED_MOVE_منزل_RESUME_قطار,         "moveToResumeQueue",      false,  CLEAN,  "r") \
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
    macro(UNPACKED_سونپيو,                    "assign",                 false,  CLEAN,  "r") \
    macro(UNPACKED_FILL,                      "fill",                   false,  CLEAN,  "r") \
    macro(UNPACKED_NEQ,                       "neq",                    خالص_2,   CLEAN,  "r")
// clang-format on

class VCMethod final {
    ساكن constexpr bool خالص_2 = true;  // For macro expansion of 'pure' field only
    ساكن constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

عام:
    تعداد en : uint8_t {
#وصف_ڪريو VL_CMETHOD_ID(id, طريقو, خالص_3, clean, دليلون) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#وصف_رد VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    تعداد en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت قدرون[] = {
#وصف_ڪريو VL_CMETHOD_NAME(id, طريقو, خالص_3, clean, دليلون) طريقو,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#وصف_رد VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        أرجع قدرون[m_e];
    }
    bool is_خالص() ثابت VL_خالص {
        ساكن constexpr bool قدرون[] = {
#وصف_ڪريو VL_CMETHOD_خالص(id, طريقو, خالص_3, clean, دليلون) خالص_3,
            FOR_EACH_CMETHOD(VL_CMETHOD_خالص)
#وصف_رد VL_CMETHOD_خالص
                false  //
        };
        أرجع قدرون[m_e];
    }
    bool isClean() ثابت VL_خالص {
        ساكن constexpr bool قدرون[] = {
#وصف_ڪريو VL_CMETHOD_CLEAN(id, طريقو, خالص_3, clean, دليلون) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#وصف_رد VL_CMETHOD_CLEAN
                false  //
        };
        أرجع قدرون[m_e];
    }
    ثابت حرف* دليلون() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت قدرون[] = {
#وصف_ڪريو VL_CMETHOD_دليلون(id, طريقو, خالص_3, clean, دليلون) دليلون,
            FOR_EACH_CMETHOD(VL_CMETHOD_دليلون)
#وصف_رد VL_CMETHOD_دليلون
                ""  //
        };
        أرجع قدرون[m_e];
    }
    // Return array method for given name
    ساكن VCMethod array_طريقو(ثابت string& name);
};
constexpr bool عامل==(ثابت VCMethod& lhs, ثابت VCMethod& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VCMethod& lhs, VCMethod::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VCMethod::en lhs, ثابت VCMethod& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VCMethod& rhs) {
    أرجع os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#وصف_ڪريو VL_CMETHOD_دليلون_CHECK(id, طريقو, خالص_3, clean, دليلون) \
    static_assert(validate_دليلون_Descriptor(دليلون), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_دليلون_CHECK)
#وصف_رد VL_CMETHOD_دليلون_CHECK

#وصف_رد FOR_EACH_CMETHOD

// clang-format off
#وصف_ڪريو FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_منزل_SOLVER_HEX,           "vlToSolverHex",          خالص_2,   "r")
// clang-format on

class VCFunction final {
    ساكن constexpr bool خالص_2 = true;  // For macro expansion of 'pure' field only

عام:
    تعداد en : uint8_t {
#وصف_ڪريو VL_CFUNCTION_ID(id, طريقو, خالص_3, دليلون) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#وصف_رد VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن ثابت حرف* ثابت قدرون[] = {
#وصف_ڪريو VL_CFUNCTION_NAME(id, function, خالص_3, دليلون) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#وصف_رد VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        أرجع قدرون[m_e];
    }
    bool is_خالص() ثابت VL_خالص {
        ساكن ثابت bool قدرون[] = {
#وصف_ڪريو VL_CFUNCTION_خالص(id, طريقو, خالص_3, دليلون) خالص_3,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_خالص)
#وصف_رد VL_CFUNCTION_خالص
                false  //
        };
        أرجع قدرون[m_e];
    }
    ثابت حرف* دليلون() ثابت VL_خالص {
        ساكن ثابت حرف* ثابت قدرون[] = {
#وصف_ڪريو VL_CFUNCTION_دليلون(id, طريقو, خالص_3, دليلون) دليلون,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_دليلون)
#وصف_رد VL_CFUNCTION_دليلون
                ""  //
        };
        أرجع قدرون[m_e];
    }
};

// Static assert all argument descriptors are well formed
#وصف_ڪريو VL_CFUNCTION_دليلون_CHECK(id, طريقو, خالص_3, دليلون) \
    static_assert(validate_دليلون_Descriptor(دليلون), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_دليلون_CHECK)
#وصف_رد VL_CFUNCTION_دليلون_CHECK

#وصف_رد FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
عام:
    تعداد en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

خاص:
    بنية شي final {
        تعداد en m_e;  // Statement's enum mnemonic, for checking
        ثابت حرف* m_name;  // Statements name, for debugging
    };
    ساكن شي s_شي_معلومات[];

عام:
    تعداد en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"none", "ctor_var_reset_call"};
        أرجع نالا[m_e];
    }
    bool isNone() ثابت { أرجع m_e == NONE; }
};
constexpr bool عامل==(ثابت VCStmtType& lhs, ثابت VCStmtType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VCStmtType& lhs, VCStmtType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VCStmtType::en lhs, ثابت VCStmtType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VCStmtType& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
عام:
    تعداد en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    تعداد en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VCaseType& lhs, ثابت VCaseType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VCaseType& lhs, VCaseType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VCaseType::en lhs, ثابت VCaseType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
عام:
    تعداد en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        أرجع نالا[m_e];
    }
    bool isAssignable() ثابت { أرجع m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
};
constexpr bool عامل==(ثابت VCastable& lhs, ثابت VCastable& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VCastable& lhs, VCastable::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VCastable::en lhs, ثابت VCastable& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VCastable& rhs) {
    أرجع os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
عام:
    تعداد en : uint8_t {
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
    تعداد en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr عامل en() ثابت { أرجع m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        أرجع نالا[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        أرجع نالا[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ثابت حرف* bin_سيٽ_Enum() ثابت {
        بدل (m_e) {
        حالة BINS_IGNORE: أرجع "VlCovBinKind::KIND_IGNORE";
        حالة BINS_ILLEGAL: أرجع "VlCovBinKind::KIND_ILLEGAL";
        حالة BINS_DEFAULT: أرجع "VlCovBinKind::KIND_DEFAULT";
        افتراضي: أرجع "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool bin_Is_عام() ثابت {
        أرجع m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool عامل==(ثابت VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    أرجع lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
عام:
    تعداد en : uint8_t {
        // Shared by option.* and type_option.*
        وزن,
        GOAL,
        AT_گهٽ_کان_گهٽ,
        AUTO_BIN_MAX,
        في_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_انگ_PRINT_MISSING,
        CROSS_RETAIN_AUTO_BINS,
        DETECT_OVERLAP,
        GET_INST_COVERAGE,
        // type_option.* only (IEEE 1800-2023 Table 19-3)
        STROBE,
        ملايو_INSTANCES,
        DISTRIBUTE_پهريون,
        REAL_INTERVAL,
        // Legacy option.* accepted for compatibility
        CROSS_AUTO_BIN_MAX,
        // sentinel - should never appear after parse-time validation
        UNKNOWN
    };
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"weight",
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
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    أرجع lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
عام:
    تعداد en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    تعداد en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(صحيح _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"", "[*]", "[->]", "[=]"};
        أرجع نالا[m_e];
    }
    ثابت حرف* asciiJson() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        أرجع نالا[m_e];
    }
};

//######################################################################

class VDirection final {
عام:
    تعداد en : uint8_t { NONE, داخلا, OUTPUT, INOUT, REF, CONSTREF };
    تعداد en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت VL_MT_محفوظ { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        أرجع نالا[m_e];
    }
    string verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        أرجع نالا[m_e];
    }
    string prettyName() ثابت { أرجع verilogKwd(); }
    bool isAny() ثابت { أرجع m_e != NONE; }
    bool isInout() ثابت { أرجع m_e == INOUT; }
    bool isInoutOrRef() ثابت { أرجع m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_داخلا() ثابت { أرجع m_e == داخلا; }
    bool isNonOutput() ثابت {
        أرجع m_e == داخلا || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ثابت { أرجع m_e == OUTPUT; }
    bool isReadOnly() ثابت VL_MT_محفوظ { أرجع m_e == داخلا || m_e == CONSTREF; }
    bool isWritable() ثابت VL_MT_محفوظ { أرجع m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ثابت VL_MT_محفوظ { أرجع m_e == REF; }
    bool isConstRef() ثابت VL_MT_محفوظ { أرجع m_e == CONSTREF; }
    string traceSigDirection() ثابت {
        إذا (isInout()) {
            أرجع "VerilatedTraceSigDirection::INOUT";
        } وإلا إذا (isWritable()) {
            أرجع "VerilatedTraceSigDirection::OUTPUT";
        } وإلا إذا (isNonOutput()) {
            أرجع "VerilatedTraceSigDirection::INPUT";
        }
        أرجع "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ثابت {
        بدل (m_e) {
        حالة NONE: أرجع VAccess::NOCHANGE;
        حالة داخلا:
        حالة CONSTREF: أرجع VAccess::READ;
        حالة OUTPUT: أرجع VAccess::WRITE;
        حالة INOUT:
        حالة REF: أرجع VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool عامل==(ثابت VDirection& lhs, ثابت VDirection& rhs) VL_MT_محفوظ {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VDirection& lhs, VDirection::en rhs) VL_MT_محفوظ {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل==(VDirection::en lhs, ثابت VDirection& rhs) VL_MT_محفوظ {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VDirection& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
عام:
    تعداد en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    تعداد en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    bool جوڙيو_Newline() ثابت { أرجع m_e != DT_WRITE; }
    bool needScopeTracking() ثابت { أرجع m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VDisplayType& lhs, ثابت VDisplayType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VDisplayType& lhs, VDisplayType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VDisplayType::en lhs, ثابت VDisplayType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
عام:
    تعداد en : uint8_t { FILE, VARS, ALL, FLUSH, حد_بندي_2, OFF, ON };
    تعداد en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VDumpCtlType& lhs, ثابت VDumpCtlType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VDumpCtlType& lhs, VDumpCtlType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VDumpCtlType::en lhs, ثابت VDumpCtlType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
عام:
    // REMEMBER to edit the strings below too
    تعداد en : uint8_t {
        // These must be in general -> most specific order, as we sort by it
        // in V3Const::visit AstSenTree
        // Involving a variable
        ET_بدليو,  // Value changed
        ET_BOTHEDGE,  // POSEDGE | NEGEDGE (i.e.: 'edge' in Verilog)
        ET_POSEDGE,
        ET_NEGEDGE,
        ET_EVENT,  // VlEventBase::isFired
        // Involving an expression
        ET_TRUE,
        ET_INITIAL_NBA,  // Event that is fired initially and never again
        //
        ET_COMBO,  // Sensitive to all combo inputs to this block
        ET_COMBO_تارو,  // Sensitive to all combo inputs to this block (from .*)
        ET_HYBRID,  // This is like ET_COMB, but with explicit sensitivity to an expression
        ET_STATIC,  // static variable initializers (runs before 'initial')
        ET_INITIAL,  // 'initial' statements
        ET_FINAL,  // 'final' statements
        ET_NEVER  // Never occurs (optimized away)
    };
    تعداد en m_e;
    bool clockedStmt() ثابت {
        ساكن constexpr bool clocked[] = {
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
        أرجع clocked[m_e];
    }
    bool an_ڪنارو() ثابت { أرجع m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ثابت {
        بدل (m_e) {
        حالة ET_بدليو: أرجع ET_بدليو;
        حالة ET_BOTHEDGE: أرجع ET_BOTHEDGE;
        حالة ET_POSEDGE: أرجع ET_NEGEDGE;
        حالة ET_NEGEDGE: أرجع ET_POSEDGE;
        افتراضي: UASSERT_STATIC(0, "Inverting bad edgeType()"); أرجع ET_NEGEDGE;
        }
    }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        أرجع نالا[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        أرجع نالا[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusive_ڪنارو(ثابت VEdgeType& other) ثابت {
        بدل (m_e) {
        حالة VEdgeType::ET_POSEDGE:
            إذا (other.m_e == VEdgeType::ET_NEGEDGE) أرجع true;
            اكسر;
        حالة VEdgeType::ET_NEGEDGE:
            إذا (other.m_e == VEdgeType::ET_POSEDGE) أرجع true;
            اكسر;
        افتراضي: اكسر;
        }
        أرجع false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
};
constexpr bool عامل==(ثابت VEdgeType& lhs, ثابت VEdgeType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VEdgeType& lhs, VEdgeType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VEdgeType::en lhs, ثابت VEdgeType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#وصف_ڪريو FOR_EACH_EVAL(macro) \
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
عام:
    تعداد en : uint8_t {
#وصف_ڪريو VL_EVAL_ID(id, iterated, triggers, پهريون, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#وصف_رد VL_EVAL_ID
            ENUM_پڇاڙي
    };
    تعداد en m_e;

    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت قدرون[] = {
#وصف_ڪريو VL_EVAL_NAME(id, iterated, triggers, پهريون, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#وصف_رد VL_EVAL_NAME
                "_ENUM_END"  //
        };
        أرجع قدرون[m_e];
    }
    bool isIterated() ثابت {
        ساكن constexpr bool قدرون[] = {
#وصف_ڪريو VL_EVAL_IS_ITERATED(id, iterated, triggers, پهريون, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#وصف_رد VL_EVAL_IS_ITERATED
                false  //
        };
        أرجع قدرون[m_e];
    }
    bool hasTriggers() ثابت {
        ساكن constexpr bool قدرون[] = {
#وصف_ڪريو VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, پهريون, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#وصف_رد VL_EVAL_HAS_TRIGGERS
                false  //
        };
        أرجع قدرون[m_e];
    }
    bool پهريون_Iteration() ثابت {
        ساكن constexpr bool قدرون[] = {
#وصف_ڪريو VL_EVAL_پهريون(id, iterated, triggers, پهريون, slow) پهريون,
            FOR_EACH_EVAL(VL_EVAL_پهريون)
#وصف_رد VL_EVAL_پهريون
                false  //
        };
        أرجع قدرون[m_e];
    }
    bool slow() ثابت {
        ساكن constexpr bool قدرون[] = {
#وصف_ڪريو VL_EVAL_SLOW(id, iterated, triggers, پهريون, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#وصف_رد VL_EVAL_SLOW
                false  //
        };
        أرجع قدرون[m_e];
    }

    // Short name
    std::string ٽيگ() ثابت { أرجع VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ثابت { أرجع "_eval_" + ٽيگ(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string eval_طريقو() ثابت { أرجع "eval" + capitalized_ٽيگ(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ثابت { أرجع "_eval_dump_triggers__" + ٽيگ(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dump_Triggers_طريقو() ثابت { أرجع "dumpTriggers" + capitalized_ٽيگ(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(صحيح _e)
        : m_e(static_cast<en>(_e)) {}
    عامل en() ثابت { أرجع m_e; }

خاص:
    std::string capitalized_ٽيگ() ثابت {
        ثابت std::string name = ascii();
        أرجع name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#وصف_رد FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
عام:
    تعداد en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        أرجع نالا[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    // Is a node type compatible with the declaration
    bool is_ڳنڍ_Compatible(ثابت Ast_ڳنڍ* nodep) ثابت;
};
constexpr bool عامل==(ثابت VFwdType& lhs, ثابت VFwdType& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VFwdType& lhs, VFwdType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VFwdType::en lhs, ثابت VFwdType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VFwdType& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    ساكن uint64_t s_cachedCntGbl;  // Global computed count

عام:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() ثابت { أرجع m_cachedCnt == s_cachedCntGbl; }
    bool get() ثابت { أرجع m_state; }
    فراغ سيٽ(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    فراغ clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ساكن فراغ clear_Cache_وڻ() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
عام:
    تعداد en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    تعداد en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        أرجع نالا[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"join", "join_any", "join_none"};
        أرجع نالا[m_e];
    }
    bool join() ثابت { أرجع m_e == JOIN; }
    bool joinAny() ثابت { أرجع m_e == JOIN_ANY; }
    bool joinNone() ثابت { أرجع m_e == JOIN_NONE; }
};
constexpr bool عامل==(ثابت VJoinType& lhs, ثابت VJoinType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VJoinType& lhs, VJoinType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VJoinType::en lhs, ثابت VJoinType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VJoinType& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
عام:
    تعداد en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        أرجع نالا[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    bool isNone() ثابت { أرجع m_e == NONE; }
    bool isAutomatic() ثابت { أرجع m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ثابت { أرجع m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ثابت { أرجع m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ثابت {
        بدل (m_e) {
        حالة AUTOMATIC_EXPLICIT: أرجع AUTOMATIC_IMPLICIT;
        حالة STATIC_EXPLICIT: أرجع STATIC_IMPLICIT;
        افتراضي: أرجع m_e;
        }
    }
};
constexpr bool عامل==(ثابت VLifetime& lhs, ثابت VLifetime& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VLifetime& lhs, VLifetime::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VLifetime::en lhs, ثابت VLifetime& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VLifetime& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
عام:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            صحيح m_اشاريو;  // Current index
            ثابت صحيح m_inc;  // Step, +1 or -1
        عام:
            Iterator(صحيح اشاريو, صحيح inc)
                : m_اشاريو{اشاريو}
                , m_inc{inc} {}
            صحيح عامل*() ثابت { أرجع m_اشاريو; }
            Iterator& عامل++() {
                m_اشاريو += m_inc;
                أرجع *this;
            }
            bool عامل!=(ثابت Iterator& that) ثابت { أرجع m_اشاريو != that.m_اشاريو; }
        };
        ثابت صحيح m_begin;  // First index
        ثابت صحيح m_پڇاڙي;  // One past the last index
        ثابت صحيح m_inc;  // Step, +1 or -1
    عام:
        Sequence(صحيح پهريون, صحيح آخري, صحيح inc)
            : m_begin{پهريون}
            , m_پڇاڙي{آخري + inc}
            , m_inc{inc} {}
        Iterator begin() ثابت { أرجع Iterator{m_begin, m_inc}; }
        Iterator پڇاڙي() ثابت { أرجع Iterator{m_پڇاڙي, m_inc}; }
    };

    صحيح m_کاٻو = 0;  // Left side of range (pre-':')
    صحيح m_ساڄو = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool عامل==(ثابت VNumRange& rhs) ثابت {
        أرجع m_کاٻو == rhs.m_کاٻو && m_ساڄو == rhs.m_ساڄو && m_ranged == rhs.m_ranged;
    }
    bool عامل<(ثابت VNumRange& rhs) ثابت {
        إذا ((m_کاٻو < rhs.m_کاٻو)) أرجع true;
        إذا (!(m_کاٻو == rhs.m_کاٻو)) أرجع false;  // lhs > rhs
        إذا ((m_ساڄو < rhs.m_ساڄو)) أرجع true;
        إذا (!(m_ساڄو == rhs.m_ساڄو)) أرجع false;  // lhs > rhs
        إذا ((m_ranged < rhs.m_ranged)) أرجع true;
        إذا (!(m_ranged == rhs.m_ranged)) أرجع false;  // lhs > rhs
        أرجع false;
    }
    //
    VNumRange() = افتراضي;
    VNumRange(صحيح hi, صحيح lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(صحيح کاٻو, صحيح ساڄو)
        : m_کاٻو{کاٻو}
        , m_ساڄو{ساڄو}
        , m_ranged{true} {}
    ~VNumRange() = افتراضي;
    // MEMBERS
    فراغ init(صحيح hi, صحيح lo, bool ascending) {
        إذا (lo > hi) std::مٽا_سٽا(hi, lo);
        m_کاٻو = ascending ? lo : hi;
        m_ساڄو = ascending ? hi : lo;
        m_ranged = true;
    }
    صحيح کاٻو() ثابت { أرجع m_کاٻو; }
    صحيح ساڄو() ثابت { أرجع m_ساڄو; }
    صحيح hi() ثابت VL_MT_محفوظ {
        أرجع m_کاٻو > m_ساڄو ? m_کاٻو : m_ساڄو;
    }  // How to show a declaration
    صحيح lo() ثابت VL_MT_محفوظ {
        أرجع m_کاٻو > m_ساڄو ? m_ساڄو : m_کاٻو;
    }  // How to show a declaration
    صحيح کاٻو_منزل_ساڄو_Inc() ثابت { أرجع ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_کاٻو_منزل_ساڄو() ثابت { أرجع {m_کاٻو, m_ساڄو, کاٻو_منزل_ساڄو_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_ساڄو_منزل_کاٻو() ثابت { أرجع {m_ساڄو, m_کاٻو, -کاٻو_منزل_ساڄو_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seq_Lo_منزل_Hi() ثابت { أرجع {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seq_Hi_منزل_Lo() ثابت { أرجع {hi(), lo(), -1}; }
    صحيح elements() ثابت VL_MT_محفوظ { أرجع hi() - lo() + 1; }
    bool ranged() ثابت { أرجع m_ranged; }
    bool ascending() ثابت { أرجع m_کاٻو < m_ساڄو; }
    صحيح hiMaxSelect() ثابت {
        أرجع (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ثابت {
        إذا (ranged()) {
            أرجع "["s + std::منزل_string(کاٻو()) + ":" + std::منزل_string(ساڄو()) + "]";
        } وإلا {
            أرجع "[norg]";
        }
    }
    فراغ dump(std::ostream& str) ثابت { str << ascii(); }
};
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VNumRange& rhs) {
    rhs.dump(os);
    أرجع os;
}

// ######################################################################

class VPragmaType final {
عام:
    تعداد en : uint8_t {
        COVERAGE_بلاڪ_OFF,
        HIER_بلاڪ,
        HIER_PARAMS,
        INLINE_MODULE,
        NO_INLINE_MODULE,
        NO_INLINE_TASK,
        عوامي_MODULE,
        عوامي_TASK,
        TIMEUNIT_سيٽ,
        UNROLL_DISABLE,
        UNROLL_ڀريل,
        ڀريل_CASE,
        PARALLEL_CASE,
        VERILATOR_LIB,
        ENUM_سائيز
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {
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
        أرجع نالا[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
};
constexpr bool عامل==(ثابت VPragmaType& lhs, ثابت VPragmaType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VPragmaType& lhs, VPragmaType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VPragmaType::en lhs, ثابت VPragmaType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VPragmaType& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
عام:
    تعداد en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        أرجع نالا[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr عامل en() ثابت { أرجع m_e; }
    bool isRandomizable() ثابت { أرجع m_e != NONE; }
    bool isRand() ثابت { أرجع m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ثابت { أرجع m_e == RAND_CYCLIC; }
};
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VRandAttr& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelf_اشارو_متن final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ساكن ثابت std::shared_ptr<ثابت string> s_emptyp;  // Holds ""
    ساكن ثابت std::shared_ptr<ثابت string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ثابت string> m_strp;

عام:
    // CONSTRUCTORS
    class خالي_جايون_2 {};  // for creator type-overload selection
    explicit VSelf_اشارو_متن(خالي_جايون_2)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelf_اشارو_متن(This)
        : m_strp{s_thisp} {}
    VSelf_اشارو_متن(This, ثابت string& ميدان)
        : m_strp{std::make_shared<ثابت string>("this->" + ميدان)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelf_اشارو_متن(VlSyms, ثابت string& ميدان)
        : m_strp{std::make_shared<ثابت string>("(&vlSymsp->vlInst." + ميدان + ')')} {}

    // METHODS
    bool is_خالي_جايون() ثابت { أرجع m_strp == s_emptyp; }
    bool isVlSym() ثابت { أرجع m_strp->ڳوليو("vlSymsp") != string::npos; }
    bool hasThis() ثابت { أرجع m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ثابت;
    ساكن string replaceThis(bool useSelfForThis, ثابت string& متن);
    ثابت std::string& asString() ثابت { أرجع *m_strp; }
    bool عامل==(ثابت VSelf_اشارو_متن& other) ثابت { أرجع *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
عام:
    تعداد en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        أرجع نالا[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    ساكن VSigning fromBool(bool isSigned) {  // Factory method
        أرجع isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    bool isSigned() ثابت VL_MT_محفوظ { أرجع m_e == SIGNED; }
    bool isNosign() ثابت VL_MT_محفوظ { أرجع m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool عامل==(ثابت VSigning& lhs, ثابت VSigning& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VSigning& lhs, VSigning::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VSigning::en lhs, ثابت VSigning& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VSigning& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VStrength final {
عام:
    تعداد en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    تعداد en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VStrength& lhs, ثابت VStrength& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VStrength& lhs, VStrength::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VStrength::en lhs, ثابت VStrength& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VStrength& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
عام:
    تعداد en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        أرجع نالا[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr عامل en() ثابت { أرجع m_e; }
};

//######################################################################

class VTrace_اڳياڙي_Type final {
عام:
    تعداد en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTrace_اڳياڙي_Type(en _e)
        : m_e{_e} {}
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VTrace_اڳياڙي_Type& lhs, ثابت VTrace_اڳياڙي_Type& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VTrace_اڳياڙي_Type& lhs, VTrace_اڳياڙي_Type::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل==(VTrace_اڳياڙي_Type::en lhs, ثابت VTrace_اڳياڙي_Type& rhs) {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VTrace_اڳياڙي_Type& rhs) {
    أرجع os << rhs.ascii();
}

//######################################################################

class VTraceType final {
عام:
    تعداد en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        ڀريل,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    تعداد en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"CONSTANT", "FULL", "CHANGE"};
        أرجع نالا[m_e];
    }
    ثابت حرف* func_اڳياڙي() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"trace_const", "trace_full", "trace_chg"};
        أرجع نالا[m_e];
    }
};
constexpr bool عامل==(ثابت VTraceType& lhs, ثابت VTraceType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VTraceType& lhs, VTraceType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VTraceType::en lhs, ثابت VTraceType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VTraceType& rhs) {
    أرجع os << rhs.ascii();
}

//######################################################################

class VUseType final {
عام:
    تعداد en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    تعداد en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    bool شامل_آهي_Any(VUseType other) ثابت { أرجع m_e & other.m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        أرجع نالا[m_e - 1];
    }
};
constexpr bool عامل==(ثابت VUseType& lhs, ثابت VUseType& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عامل==(ثابت VUseType& lhs, VUseType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VUseType::en lhs, ثابت VUseType& rhs) { أرجع lhs == rhs.m_e; }
constexpr VUseType::en عامل|(VUseType::en lhs, VUseType::en rhs) {
    أرجع VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en عامل&(VUseType::en lhs, VUseType::en rhs) {
    أرجع VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VUseType& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VVarType final {
عام:
    تعداد en : uint8_t {
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
        پورٽ,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    تعداد en m_e;
    VVarType() VL_MT_محفوظ : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_محفوظ : m_e{_e} {}
    explicit VVarType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نالا[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        أرجع نالا[m_e];
    }
    bool isParam() ثابت { أرجع m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ثابت {
        أرجع (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == پورٽ || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ثابت {
        أرجع (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ثابت { أرجع (m_e == TRIOR); }
    bool isWiredNet() ثابت { أرجع (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ثابت {  // In Verilog, always ok in SystemVerilog
        أرجع (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == پورٽ || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ثابت {
        أرجع (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool is_عارضي() ثابت {
        أرجع (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ثابت {
        أرجع (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == پورٽ
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ثابت حرف* trace_Sig_قسم() ثابت {
        // VerilatedTraceSigKind to used in trace signal declaration
        ساكن constexpr ثابت حرف* ثابت lut[] = {
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
        أرجع lut[m_e];
    }
};
constexpr bool عامل==(ثابت VVarType& lhs, ثابت VVarType& rhs) VL_MT_محفوظ {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VVarType& lhs, VVarType::en rhs) VL_MT_محفوظ {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل==(VVarType::en lhs, ثابت VVarType& rhs) VL_MT_محفوظ {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VVarType& rhs) VL_MT_محفوظ {
    أرجع os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_ڪنجي final {
عام:
    ثابت صحيح m_ويڪر;  // From AstNodeDType: Bit width of operation
    ثابت صحيح m_ويڪر_Min;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ثابت VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ثابت VSigning m_numeric;  // From AstNodeDType: Node is signed
    ثابت VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool عامل==(ثابت VBasic_Type_ڪنجي& rhs) ثابت {
        أرجع m_ويڪر == rhs.m_ويڪر && m_ويڪر_Min == rhs.m_ويڪر_Min && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool عامل<(ثابت VBasic_Type_ڪنجي& rhs) ثابت {
        إذا ((m_ويڪر < rhs.m_ويڪر)) أرجع true;
        إذا (!(m_ويڪر == rhs.m_ويڪر)) أرجع false;  // lhs > rhs
        إذا ((m_ويڪر_Min < rhs.m_ويڪر_Min)) أرجع true;
        إذا (!(m_ويڪر_Min == rhs.m_ويڪر_Min)) أرجع false;  // lhs > rhs
        إذا ((m_numeric < rhs.m_numeric)) أرجع true;
        إذا (!(m_numeric == rhs.m_numeric)) أرجع false;  // lhs > rhs
        إذا ((m_keyword < rhs.m_keyword)) أرجع true;
        إذا (!(m_keyword == rhs.m_keyword)) أرجع false;  // lhs > rhs
        إذا ((m_nrange < rhs.m_nrange)) أرجع true;
        إذا (!(m_nrange == rhs.m_nrange)) أرجع false;  // lhs > rhs
        أرجع false;
    }
    VBasic_Type_ڪنجي(صحيح ويڪر, صحيح ويڪر_Min, VSigning numeric, VBasicDTypeKwd kwd,
                  ثابت VNumRange& nrange)
        : m_ويڪر{ويڪر}
        , m_ويڪر_Min{ويڪر_Min}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_ڪنجي() = افتراضي;
};

#جيڪڏهن_پورو  // Guard
