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

#اگر_تعریف_نشده VERILATOR_V3ASTATTR_H_
#تعریف_کن VERILATOR_V3ASTATTR_H_

#اگر_تعریف_نشده VERILATOR_V3AST_H_
#خطا "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کن VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#پایان_اگر

// Hint class so we can choose constructors
class VFlag_بیت_Packed {};
class VFlag_فرزند_DType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbort_نوع final {
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
    constexpr VAbort_نوع(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت نامها[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        أرجع نامها[m_e];
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
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"RD", "WR", "RW", "--"};
        أرجع نامها[m_e];
    }
    ثابت حرف* arrow() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        أرجع نامها[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    VAccess invert() ثابت {
        أرجع (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ثابت { أرجع m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ثابت { أرجع m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ثابت { أرجع m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ثابت { أرجع m_e == WRITE || m_e == READWRITE; }
    bool isRW() ثابت { أرجع m_e == READWRITE; }
};
constexpr bool عملگر==(ثابت VAccess& lhs, ثابت VAccess& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عملگر==(ثابت VAccess& lhs, VAccess::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VAccess::en lhs, ثابت VAccess& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VAccess& rhs) { أرجع os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
عام:
    تعداد en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_تخصیص_بده };
    تعداد en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VAlwaysKwd& lhs, ثابت VAlwaysKwd& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VAlwaysKwd::en lhs, ثابت VAlwaysKwd& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
عام:
    // IEEE 1800-2023 Table 20-5
    تعداد en : uint8_t {
        مقصد_BE_EVALUATED = 0,
        قفل = 1,
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
        : m_e{مقصد_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        // IEEE 1800-2023 20.11
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"",
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
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VAssertCtlType& lhs, ثابت VAssertCtlType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عملگر==(VAssertCtlType::en lhs, ثابت VAssertCtlType& rhs) {
    أرجع lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
عام:
    // IEEE 1800-2023 Table 20-7
    تعداد en : uint8_t {
        داخلی = 0,  // Non IEEE type, for directives to be evaluated from expression.
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
        إذا (m_e == داخلی)
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
        UASSERT(!str.خالی(), "Assert should be of one of types");
        أرجع str.substr(0, str.اندازه() - 1);
    }
    constexpr عملگر en() ثابت { أرجع m_e; }
};
constexpr bool عملگر==(ثابت VAssertDirectiveType& lhs, ثابت VAssertDirectiveType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عملگر==(VAssertDirectiveType::en lhs, ثابت VAssertDirectiveType& rhs) {
    أرجع lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en عملگر|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    أرجع VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
عام:
    // IEEE 1800-2023 Table 20-6
    تعداد en : uint8_t {
        داخلی = 0,  // Non IEEE type, for assertions that should not be controlled.
        CONCURRENT = (1 << 0),
        SIMPLE_فوری = (1 << 1),
        OBSERVED_DEFERRED_فوری = (1 << 2),
        FINAL_DEFERRED_فوری = (1 << 3),
        EXPECT = (1 << 4),
        UNIQUE = (1 << 5),
        UNIQUE0 = (1 << 6),
        اولویت = (1 << 7),
    };
    تعداد en m_e;
    VAssertType()
        : m_e{داخلی} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool شامل_است_Any(VAssertType other) ثابت { أرجع m_e & other.m_e; }
    string ascii() ثابت {
        std::stringstream types;
        إذا (m_e == داخلی)
            types << "INTERNAL ";
        وإلا {
            إذا (m_e & CONCURRENT) types << "CONCURRENT ";
            إذا (m_e & SIMPLE_فوری) types << "SIMPLE_IMMEDIATE ";
            إذا (m_e & OBSERVED_DEFERRED_فوری) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            إذا (m_e & FINAL_DEFERRED_فوری) types << "FINAL_DEFERRED_IMMEDIATE ";
            إذا (m_e & EXPECT) types << "EXPECT ";
            إذا (m_e & UNIQUE) types << "UNIQUE ";
            إذا (m_e & UNIQUE0) types << "UNIQUE0 ";
            إذا (m_e & اولویت) types << "PRIORITY ";
        }
        ثابت string str = types.str();
        UASSERT(!str.خالی(), "Assert should be of one of types");
        أرجع str.substr(0, str.اندازه() - 1);
    }
    constexpr عملگر en() ثابت { أرجع m_e; }
};
constexpr bool عملگر==(ثابت VAssertType& lhs, ثابت VAssertType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VAssertType& lhs, VAssertType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VAssertType::en lhs, ثابت VAssertType& rhs) { أرجع lhs == rhs.m_e; }
constexpr VAssertType::en عملگر|(VAssertType::en lhs, VAssertType::en rhs) {
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
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"default", "weak", "strong"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VPropStrength& lhs, ثابت VPropStrength& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عملگر!=(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    أرجع lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
عام:
    // clang-format off
    تعداد en: uint8_t  {
        ILLEGAL,
        //
        DIM_بیتها,                       // V3Const converts to constant
        DIM_بیتها_OR_عدد,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_بالا,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_چپ,                       // V3Width processes
        DIM_کم,                        // V3Width processes
        DIM_راست,                      // V3Width processes
        DIM_اندازه,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_عمومی,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_اولی,                     // V3Width processes
        ENUM_آخرین,                      // V3Width processes
        ENUM_رقم,                       // V3Width processes
        ENUM_بعدی,                      // V3Width processes
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
        VAR_پایه,                       // V3LinkResolve creates for AstPreSel, V3LinkParam removes
        VAR_FORCEABLE,                  // V3LinkParse moves to AstVar::isForceable
        VAR_FSM_ARC_INCLUDE_COND,       // V3LinkParse moves to AstVar::attrFsmArcInclCond
        VAR_FSM_ARC_INCLUDE_COND_AUTO,  // Flavors of above option - setting expand mode per FSM
        VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND,
        VAR_FSM_ARC_INCLUDE_COND_پر,
        VAR_FSM_STATE,                  // V3LinkParse moves to AstVar::attrFsmState
        VAR_FSM_STATE_AUTO,             // Flavors of above option - setting expand mode per FSM
        VAR_FSM_STATE_AUTO_EXPAND,
        VAR_FSM_STATE_پر,
        VAR_پورت_DTYPE,                 // V3LinkDot for V3Width to check port dtype
        VAR_عمومی,                     // V3LinkParse moves to AstVar::sigPublic
        VAR_عمومی_FLAT,                // V3LinkParse moves to AstVar::sigPublic
        VAR_عمومی_FLAT_RD,             // V3LinkParse moves to AstVar::sigPublic
        VAR_عمومی_FLAT_RW,             // V3LinkParse moves to AstVar::sigPublic
        VAR_SC_BIGUINT,                 // V3LinkParse moves to AstVar::attrScBigUint
        VAR_SC_BV,                      // V3LinkParse moves to AstVar::attrScBv
        VAR_SFORMAT,                    // V3LinkParse moves to AstVar::attrSFormat
        VAR_تقسیم_VAR                   // V3LinkParse moves to AstVar::attrSplitVar
    };
    // clang-format on
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        // clang-format off
        ساكن constexpr ثابت حرف* ثابت نامها[] = {
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
        أرجع نامها[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ثابت {
        بدل (m_e) {
        حالة DIM_بیتها:
        حالة DIM_بیتها_OR_عدد:
        حالة DIM_DIMENSIONS:
        حالة DIM_بالا:
        حالة DIM_INCREMENT:
        حالة DIM_چپ:
        حالة DIM_کم:
        حالة DIM_راست:
        حالة DIM_اندازه:
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
    constexpr عملگر en() ثابت { أرجع m_e; }
};
constexpr bool عملگر==(ثابت VAttrType& lhs, ثابت VAttrType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VAttrType& lhs, VAttrType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VAttrType::en lhs, ثابت VAttrType& rhs) { أرجع lhs == rhs.m_e; }

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
        string بیرون;
        إذا (m_initial) بیرون = VString::dot(بیرون, " ", "initial");
        إذا (m_extends) بیرون = VString::dot(بیرون, " ", "extends");
        إذا (m_final) بیرون = VString::dot(بیرون, " ", "final");
        أرجع بیرون;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
عام:
    تعداد en : uint8_t {
        UNKNOWN,
        بیت,
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
        زمان,
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
    ثابت حرف* ascii() ثابت VL_MT_امن {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"%E-unk",
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
        أرجع نامها[m_e];
    }
    ثابت حرف* typeName() ثابت VL_MT_امن {
        بدل (m_e) {
        حالة LOGIC2STATE: أرجع VBasicDTypeKwd{LOGIC}.ascii();
        حالة INTEGER2STATE: أرجع VBasicDTypeKwd{INTEGER}.ascii();
        حالة TIME2STATE: أرجع VBasicDTypeKwd{زمان}.ascii();
        افتراضي: أرجع ascii();
        }
    }
    ثابت حرف* dpiType() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"%E-unk",
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
        أرجع نامها[m_e];
    }
    ساكن فراغ selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_امن : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_امن : m_e{_e} {}
    explicit VBasicDTypeKwd(صحيح _e) VL_MT_امن  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    صحيح عرض() ثابت {
        بدل (m_e) {
        حالة بیت: أرجع 1;  // scalar, can't bit extract unless ranged
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
        حالة زمان: أرجع 64;
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
               || m_e == UINT32 || m_e == UINT64 || m_e == بیت || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == زمان || m_e == TIME2STATE;
    }
    bool isFourstate() ثابت {
        أرجع m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == زمان;
    }
    bool isZeroInit() ثابت {  // Otherwise initializes to X
        أرجع (m_e == بیت || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ثابت {  // Enum increment supported
        أرجع (m_e == بیت || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == زمان || m_e == TIME2STATE);
    }
    bool is_بیت() ثابت { أرجع m_e == بیت; }
    bool is_بیت_Logic() ثابت {  // Bit/logic vector types; can form a packed array
        أرجع (m_e == LOGIC || m_e == LOGIC2STATE || m_e == بیت);
    }
    bool isDpiUnsignable() ثابت {  // Can add "unsigned" to DPI
        أرجع (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ثابت {  // Uses standard C layout, for DPI runtime access
        أرجع (m_e == بیت || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ثابت VL_MT_امن {  // IE not a simple number we can bit optimize
        أرجع (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ثابت VL_MT_امن { أرجع m_e == CHANDLE; }
    bool isDouble() ثابت VL_MT_امن { أرجع m_e == DOUBLE; }
    bool isEvent() ثابت { أرجع m_e == EVENT; }
    bool isString() ثابت VL_MT_امن { أرجع m_e == STRING; }
    bool isMTaskState() ثابت VL_MT_امن { أرجع m_e == MTASKSTATE; }
    bool isSameish(ثابت VBasicDTypeKwd& other) ثابت VL_MT_امن {
        بدل (other.m_e) {
        حالة LOGIC:
        حالة LOGIC2STATE:
        حالة LOGIC_IMPLICIT: أرجع m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        حالة INTEGER:
        حالة INTEGER2STATE: أرجع m_e == INTEGER || m_e == INTEGER2STATE;
        حالة زمان:
        حالة TIME2STATE: أرجع m_e == زمان || m_e == TIME2STATE;
        افتراضي: أرجع m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_امن {
        بدل (m_e) {
        حالة بیت:
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
constexpr bool عملگر==(ثابت VBasicDTypeKwd& lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_امن {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_امن {
    أرجع lhs.m_e == rhs;
}
constexpr bool عملگر==(VBasicDTypeKwd::en lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_امن {
    أرجع lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
عام:
    تعداد en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, ENUM_انتها };
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
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"FALSE", "TRUE", "UNK"};
        أرجع نامها[m_e];
    }
    bool is_معلوم() ثابت { أرجع m_e != BU_UNKNOWN; }
    bool true_معلوم() ثابت { أرجع m_e == BU_TRUE; }
    فراغ مجموعه_True_Or_False(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool عملگر==(ثابت VBoolOrUnknown& lhs, ثابت VBoolOrUnknown& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عملگر==(VBoolOrUnknown::en lhs, ثابت VBoolOrUnknown& rhs) {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VBoolOrUnknown& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
عام:
    تعداد en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, ENUM_انتها };
    تعداد en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    bool unknown() ثابت { أرجع m_e == BP_UNKNOWN; }
    bool likely() ثابت { أرجع m_e == BP_LIKELY; }
    bool unlikely() ثابت { أرجع m_e == BP_UNLIKELY; }
    VBranchPred invert() ثابت {
        إذا (m_e == BP_UNLIKELY) أرجع BP_LIKELY;
        إذا (m_e == BP_LIKELY) أرجع BP_UNLIKELY;
        أرجع m_e;
    }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        أرجع نامها[m_e];
    }
    ثابت حرف* asciiShort() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"", "L", "!L"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VBranchPred& lhs, ثابت VBranchPred& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VBranchPred& lhs, VBranchPred::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VBranchPred::en lhs, ثابت VBranchPred& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VBranchPred& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ساكن constexpr bool validate_آرگومانها_Descriptor(ثابت حرف* descrp) {
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
#تعریف_کن FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     خالص_2,   CLEAN,  "r") \
    macro(ARRAY_AT_پشت,                      "atBack",                 خالص_2,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                خالص_2,   CLEAN,  "r") \
    macro(ARRAY_پیدا_کن,                         "find",                   خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کن_اولی,                   "find_first",             خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کن_اولی_شاخص,             "find_first_index",       خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کن_شاخص,                   "find_index",             خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کن_آخرین,                    "find_last",              خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کن_آخرین_شاخص,              "find_last_index",        خالص_2,   CLEAN,  "") \
    macro(ARRAY_اولی,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 خالص_2,   CLEAN,  "r") \
    macro(ARRAY_آخرین,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_بعدی,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_برداشت_پشت,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_برداشت_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_افزودن_پشت,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_افزودن_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_معکوس,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  خالص_2,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   خالص_2,   CLEAN,  "") \
    macro(ARRAY_R_حاصلضرب,                    "r_product",              خالص_2,   false,  "") \
    macro(ARRAY_R_جمع,                        "r_sum",                  خالص_2,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  خالص_2,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_برش,                        "slice",                  خالص_2,   CLEAN,  "r") \
    macro(ARRAY_مرتب_سازی,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 خالص_2,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_شاخص,                 "unique_index",           خالص_2,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 خالص_2,   CLEAN,  "r") \
    macro(ASSOC_اولی,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_بعدی,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_اندازه,                         "size",                   خالص_2,   CLEAN,  "") \
    macro(CLASS_مجموعه_RANDMODE,                 "set_randmode",           false,  CLEAN,  "r") \
    macro(COVERGROUP_جمع_کن_ARRAY_NAMER,         "addArrayNamer",          false,  CLEAN,  "r+") \
    macro(COVERGROUP_جمع_کن_BIN,                 "addBin",                 false,  CLEAN,  "r+") \
    macro(COVERGROUP_جمع_کن_COVERPOINT,          "addCoverpoint",          false,  CLEAN,  "") \
    macro(COVERGROUP_جمع_کن_CROSS,               "addCross",               false,  CLEAN,  "") \
    macro(COVERGROUP_جمع_کن_CROSS_DYN,           "addCrossDyn",            false,  CLEAN,  "") \
    macro(COVERGROUP_جمع_کن_NUMBERED_NAMER,      "addNumberedNamer",       false,  CLEAN,  "r+") \
    macro(COVERGROUP_جمع_کن_SINGLE_NAMER,        "addSingleNamer",         false,  CLEAN,  "r+") \
    macro(COVERGROUP_ATTACH,                  "attach",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_CLEAR_اصابت_LIST,          "clearHitList",           false,  CLEAN,  "") \
    macro(COVERGROUP_COVERAGE,                "coverage",               false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_BEGIN,        "withBegin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_CROSS_WITH_انتها,          "withEnd",                false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_TEST,         "withTest",               false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_مقدار,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_مقدار_W,      "withValueW",             false,  CLEAN,  "rw") \
    macro(COVERGROUP_FINALIZE_BINS,           "finalizeBins",           false,  CLEAN,  "") \
    macro(COVERGROUP_INCREMENT_BIN,           "incrementBin",           false,  CLEAN,  "r") \
    macro(COVERGROUP_INIT,                    "init",                   false,  CLEAN,  "r+") \
    macro(COVERGROUP_INST_P,                  "p",                      خالص_2,   CLEAN,  "") \
    macro(COVERGROUP_LEND_وزن,             "lendWeight",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_رکورد_اصابت,              "recordHit",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ثبات_BINS,           "registerBins",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_SAMPLE,                  "sample",                 false,  CLEAN,  "") \
    macro(COVERGROUP_SAMPLE_IFFS,             "sample",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_SELECT_ALL,              "selectAll",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_AND,              "selectAnd",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_BIN,              "selectBin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM,              "selectDim",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM_انتها,          "selectDimEnd",           false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_OR,               "selectOr",               false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_RANGE,            "selectRange",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SELECT_RANGE_W,          "selectRangeW",           false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_انتها,               "sizedEnd",               خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_پایان,            "sizedFinish",            false,  CLEAN,  "r+") \
    macro(COVERGROUP_SIZED_اولی,             "sizedFirst",             خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_RANGE,             "sizedRange",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_RANGE_W,           "sizedRangeW",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_SAMPLE,            "sizedSample",            false,  CLEAN,  "rrr") \
    macro(COVERGROUP_SIZED_SAMPLE_W,          "sizedSampleW",           false,  CLEAN,  "rrr") \
    macro(COVERGROUP_TYPE_COVERAGE,           "typeCoverage",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_TYPE_وزن,             "typeWeight",             false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_EXCLUDED,          "valueExcluded",          خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_مقدار_EXCLUDED_W,        "valueExcludedW",         خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_مقدار_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_مقدار_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_مقدار_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_مقدار_TYPE,              "valueType",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_وزن,                  "weight",                 false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_BEGIN,              "withBegin",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_پایان,             "withFinish",             false,  CLEAN,  "r+") \
    macro(COVERGROUP_WITH_HI,                 "withHi",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_HI_W,               "withHiW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_LO,                 "withLo",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_LO_W,               "withLoW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_بعدی,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_پیاپی,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_پیاپی_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_پشت,           "atWriteAppendBack",      false,  CLEAN,  "r") \
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
    macro(DYN_اندازه,                           "size",                   خالص_2,   CLEAN,  "") \
    macro(DYN_برش,                          "slice",                  خالص_2,   CLEAN,  "rr") \
    macro(DYN_برش_تخصیص_بده,                   "sliceAssign",            false,  CLEAN,  "rrr") \
    macro(DYN_برش_تخصیص_بده_پشت_پشت,         "sliceAssignBackBack",    false,  CLEAN,  "rrr") \
    macro(DYN_برش_تخصیص_بده_FRONT_پشت,        "sliceAssignFrontBack",   false,  CLEAN,  "rrr") \
    macro(DYN_برش_پشت_پشت,                "sliceBackBack",          خالص_2,   CLEAN,  "rr") \
    macro(DYN_برش_FRONT_پشت,               "sliceFrontBack",         خالص_2,   CLEAN,  "rr") \
    macro(EVENT_CLEAR_FIRED,                  "clearFired",             false,  CLEAN,  "") \
    macro(EVENT_CLEAR_TRIGGERED,              "clearTriggered",         false,  CLEAN,  "") \
    macro(EVENT_FIRE,                         "fire",                   false,  CLEAN,  "") \
    macro(EVENT_IS_FIRED,                     "isFired",                خالص_2,   CLEAN,  "") \
    macro(EVENT_IS_TRIGGERED,                 "isTriggered",            خالص_2,   CLEAN,  "") \
    macro(FORCE_جمع_کن,                          "addForce",               false,  CLEAN,  "rrrr+") \
    macro(FORCE_READ,                         "read",                   خالص_2,   CLEAN,  "r") \
    macro(FORCE_READ_شاخص,                   "readIndex",              خالص_2,   CLEAN,  "rr") \
    macro(FORCE_READ_SEL,                     "readSel",                خالص_2,   CLEAN,  "TODO") \
    macro(FORCE_RELEASE,                      "release",                false,  CLEAN,  "rr+") \
    macro(FORCE_TOUCH,                        "touch",                  false,  CLEAN,  "") \
    macro(FORK_انجامشده,                          "done",                   false,  CLEAN,  "rr") \
    macro(FORK_INIT,                          "init",                   false,  CLEAN,  "rr") \
    macro(FORK_JOIN,                          "join",                   false,  CLEAN,  "rrr") \
    macro(FORK_ON_KILL,                       "onKill",                 false,  CLEAN,  "r") \
    macro(NBA_COMMIT,                         "commit",                 false,  CLEAN,  "w") \
    macro(NBA_افزودن_به_صف,                        "enqueue",                false,  CLEAN,  "r+") \
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
    macro(RANDOMIZER_مجموعه_VAR_DISABLED,        "set_var_disabled",       false,  CLEAN,  "r") \
    macro(RANDOMIZER_CLEAR_VAR_DISABLED,      "clear_var_disabled",     false,  CLEAN,  "r") \
    macro(RANDOMIZER_MARK_VAR_STATIC,         "mark_var_static",        false,  CLEAN,  "r") \
    macro(RANDOMIZER_مجموعه_STATIC_RANDMODE,     "set_static_randmode",    false,  CLEAN,  "r") \
    macro(RNG_GET_RANDSTATE,                  "__Vm_rng.get_randstate", خالص_2,   CLEAN,  "") \
    macro(RNG_مجموعه_RANDSTATE,                  "__Vm_rng.set_randstate", false,  CLEAN,  "r") \
    macro(SCHED_ANY_TRIGGERED,                "anyTriggered",           false,  CLEAN,  "r") \
    macro(SCHED_AWAITING_CURRENT_زمان,        "awaitingCurrentTime",    خالص_2,   CLEAN,  "") \
    macro(SCHED_AWAITING_ZERO_DELAY,          "awaitingZeroDelay",      خالص_2,   CLEAN,  "") \
    macro(SCHED_READY,                        "ready",                  false,  CLEAN,  "r") \
    macro(SCHED_MOVE_مقصد_RESUME_صف,         "moveToResumeQueue",      false,  CLEAN,  "r") \
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
    macro(UNPACKED_تخصیص_بده,                    "assign",                 false,  CLEAN,  "r") \
    macro(UNPACKED_FILL,                      "fill",                   false,  CLEAN,  "r") \
    macro(UNPACKED_NEQ,                       "neq",                    خالص_2,   CLEAN,  "r")
// clang-format on

class VCMethod final {
    ساكن constexpr bool خالص_2 = true;  // For macro expansion of 'pure' field only
    ساكن constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

عام:
    تعداد en : uint8_t {
#تعریف_کن VL_CMETHOD_ID(id, روش, خالص_3, clean, آرگومانها) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#لغو_تعریف VL_CMETHOD_ID
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت مقادیر[] = {
#تعریف_کن VL_CMETHOD_NAME(id, روش, خالص_3, clean, آرگومانها) روش,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#لغو_تعریف VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        أرجع مقادیر[m_e];
    }
    bool is_خالص() ثابت VL_خالص {
        ساكن constexpr bool مقادیر[] = {
#تعریف_کن VL_CMETHOD_خالص(id, روش, خالص_3, clean, آرگومانها) خالص_3,
            FOR_EACH_CMETHOD(VL_CMETHOD_خالص)
#لغو_تعریف VL_CMETHOD_خالص
                false  //
        };
        أرجع مقادیر[m_e];
    }
    bool isClean() ثابت VL_خالص {
        ساكن constexpr bool مقادیر[] = {
#تعریف_کن VL_CMETHOD_CLEAN(id, روش, خالص_3, clean, آرگومانها) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#لغو_تعریف VL_CMETHOD_CLEAN
                false  //
        };
        أرجع مقادیر[m_e];
    }
    ثابت حرف* آرگومانها() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت مقادیر[] = {
#تعریف_کن VL_CMETHOD_آرگومانها(id, روش, خالص_3, clean, آرگومانها) آرگومانها,
            FOR_EACH_CMETHOD(VL_CMETHOD_آرگومانها)
#لغو_تعریف VL_CMETHOD_آرگومانها
                ""  //
        };
        أرجع مقادیر[m_e];
    }
    // Return array method for given name
    ساكن VCMethod array_روش(ثابت string& name);
};
constexpr bool عملگر==(ثابت VCMethod& lhs, ثابت VCMethod& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عملگر==(ثابت VCMethod& lhs, VCMethod::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VCMethod::en lhs, ثابت VCMethod& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VCMethod& rhs) {
    أرجع os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#تعریف_کن VL_CMETHOD_آرگومانها_CHECK(id, روش, خالص_3, clean, آرگومانها) \
    static_assert(validate_آرگومانها_Descriptor(آرگومانها), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_آرگومانها_CHECK)
#لغو_تعریف VL_CMETHOD_آرگومانها_CHECK

#لغو_تعریف FOR_EACH_CMETHOD

// clang-format off
#تعریف_کن FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_مقصد_SOLVER_HEX,           "vlToSolverHex",          خالص_2,   "r")
// clang-format on

class VCFunction final {
    ساكن constexpr bool خالص_2 = true;  // For macro expansion of 'pure' field only

عام:
    تعداد en : uint8_t {
#تعریف_کن VL_CFUNCTION_ID(id, روش, خالص_3, آرگومانها) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#لغو_تعریف VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن ثابت حرف* ثابت مقادیر[] = {
#تعریف_کن VL_CFUNCTION_NAME(id, function, خالص_3, آرگومانها) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#لغو_تعریف VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        أرجع مقادیر[m_e];
    }
    bool is_خالص() ثابت VL_خالص {
        ساكن ثابت bool مقادیر[] = {
#تعریف_کن VL_CFUNCTION_خالص(id, روش, خالص_3, آرگومانها) خالص_3,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_خالص)
#لغو_تعریف VL_CFUNCTION_خالص
                false  //
        };
        أرجع مقادیر[m_e];
    }
    ثابت حرف* آرگومانها() ثابت VL_خالص {
        ساكن ثابت حرف* ثابت مقادیر[] = {
#تعریف_کن VL_CFUNCTION_آرگومانها(id, روش, خالص_3, آرگومانها) آرگومانها,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_آرگومانها)
#لغو_تعریف VL_CFUNCTION_آرگومانها
                ""  //
        };
        أرجع مقادیر[m_e];
    }
};

// Static assert all argument descriptors are well formed
#تعریف_کن VL_CFUNCTION_آرگومانها_CHECK(id, روش, خالص_3, آرگومانها) \
    static_assert(validate_آرگومانها_Descriptor(آرگومانها), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_آرگومانها_CHECK)
#لغو_تعریف VL_CFUNCTION_آرگومانها_CHECK

#لغو_تعریف FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
عام:
    تعداد en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

خاص:
    بنية قلم final {
        تعداد en m_e;  // Statement's enum mnemonic, for checking
        ثابت حرف* m_name;  // Statements name, for debugging
    };
    ساكن قلم s_قلم_داده[];

عام:
    تعداد en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"none", "ctor_var_reset_call"};
        أرجع نامها[m_e];
    }
    bool isNone() ثابت { أرجع m_e == NONE; }
};
constexpr bool عملگر==(ثابت VCStmtType& lhs, ثابت VCStmtType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VCStmtType& lhs, VCStmtType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VCStmtType::en lhs, ثابت VCStmtType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VCStmtType& rhs) {
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VCaseType& lhs, ثابت VCaseType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VCaseType& lhs, VCaseType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VCaseType::en lhs, ثابت VCaseType& rhs) { أرجع lhs == rhs.m_e; }

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
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        أرجع نامها[m_e];
    }
    bool isAssignable() ثابت { أرجع m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
};
constexpr bool عملگر==(ثابت VCastable& lhs, ثابت VCastable& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VCastable& lhs, VCastable::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VCastable::en lhs, ثابت VCastable& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VCastable& rhs) {
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
    constexpr عملگر en() ثابت { أرجع m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        أرجع نامها[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        أرجع نامها[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ثابت حرف* bin_مجموعه_Enum() ثابت {
        بدل (m_e) {
        حالة BINS_IGNORE: أرجع "VlCovBinKind::KIND_IGNORE";
        حالة BINS_ILLEGAL: أرجع "VlCovBinKind::KIND_ILLEGAL";
        حالة BINS_DEFAULT: أرجع "VlCovBinKind::KIND_DEFAULT";
        افتراضي: أرجع "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool bin_Is_نرمال() ثابت {
        أرجع m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool عملگر==(ثابت VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    أرجع lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
عام:
    تعداد en : uint8_t {
        // Shared by option.* and type_option.*
        وزن,
        GOAL,
        AT_کمترین,
        AUTO_BIN_MAX,
        در_هر_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_رقم_PRINT_MISSING,
        CROSS_RETAIN_AUTO_BINS,
        DETECT_OVERLAP,
        GET_INST_COVERAGE,
        // type_option.* only (IEEE 1800-2023 Table 19-3)
        STROBE,
        ادغام_INSTANCES,
        DISTRIBUTE_اولی,
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
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"weight",
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
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VCoverOptionType& lhs, VCoverOptionType::en rhs) {
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
    constexpr عملگر en() ثابت { أرجع m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"", "[*]", "[->]", "[=]"};
        أرجع نامها[m_e];
    }
    ثابت حرف* asciiJson() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        أرجع نامها[m_e];
    }
};

//######################################################################

class VDirection final {
عام:
    تعداد en : uint8_t { NONE, درون_داد, OUTPUT, INOUT, REF, CONSTREF };
    تعداد en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت VL_MT_امن { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        أرجع نامها[m_e];
    }
    string verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        أرجع نامها[m_e];
    }
    string prettyName() ثابت { أرجع verilogKwd(); }
    bool isAny() ثابت { أرجع m_e != NONE; }
    bool isInout() ثابت { أرجع m_e == INOUT; }
    bool isInoutOrRef() ثابت { أرجع m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_درون_داد() ثابت { أرجع m_e == درون_داد; }
    bool isNonOutput() ثابت {
        أرجع m_e == درون_داد || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ثابت { أرجع m_e == OUTPUT; }
    bool isReadOnly() ثابت VL_MT_امن { أرجع m_e == درون_داد || m_e == CONSTREF; }
    bool isWritable() ثابت VL_MT_امن { أرجع m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ثابت VL_MT_امن { أرجع m_e == REF; }
    bool isConstRef() ثابت VL_MT_امن { أرجع m_e == CONSTREF; }
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
        حالة درون_داد:
        حالة CONSTREF: أرجع VAccess::READ;
        حالة OUTPUT: أرجع VAccess::WRITE;
        حالة INOUT:
        حالة REF: أرجع VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool عملگر==(ثابت VDirection& lhs, ثابت VDirection& rhs) VL_MT_امن {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VDirection& lhs, VDirection::en rhs) VL_MT_امن {
    أرجع lhs.m_e == rhs;
}
constexpr bool عملگر==(VDirection::en lhs, ثابت VDirection& rhs) VL_MT_امن {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VDirection& rhs) {
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    bool جمع_کن_Newline() ثابت { أرجع m_e != DT_WRITE; }
    bool needScopeTracking() ثابت { أرجع m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VDisplayType& lhs, ثابت VDisplayType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VDisplayType& lhs, VDisplayType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VDisplayType::en lhs, ثابت VDisplayType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
عام:
    تعداد en : uint8_t { FILE, VARS, ALL, FLUSH, حد_2, OFF, ON };
    تعداد en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VDumpCtlType& lhs, ثابت VDumpCtlType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VDumpCtlType& lhs, VDumpCtlType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VDumpCtlType::en lhs, ثابت VDumpCtlType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
عام:
    // REMEMBER to edit the strings below too
    تعداد en : uint8_t {
        // These must be in general -> most specific order, as we sort by it
        // in V3Const::visit AstSenTree
        // Involving a variable
        ET_تغییریافته,  // Value changed
        ET_BOTHEDGE,  // POSEDGE | NEGEDGE (i.e.: 'edge' in Verilog)
        ET_POSEDGE,
        ET_NEGEDGE,
        ET_EVENT,  // VlEventBase::isFired
        // Involving an expression
        ET_TRUE,
        ET_INITIAL_NBA,  // Event that is fired initially and never again
        //
        ET_COMBO,  // Sensitive to all combo inputs to this block
        ET_COMBO_ستاره,  // Sensitive to all combo inputs to this block (from .*)
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
    bool an_یال() ثابت { أرجع m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ثابت {
        بدل (m_e) {
        حالة ET_تغییریافته: أرجع ET_تغییریافته;
        حالة ET_BOTHEDGE: أرجع ET_BOTHEDGE;
        حالة ET_POSEDGE: أرجع ET_NEGEDGE;
        حالة ET_NEGEDGE: أرجع ET_POSEDGE;
        افتراضي: UASSERT_STATIC(0, "Inverting bad edgeType()"); أرجع ET_NEGEDGE;
        }
    }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        أرجع نامها[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        أرجع نامها[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusive_یال(ثابت VEdgeType& other) ثابت {
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
    constexpr عملگر en() ثابت { أرجع m_e; }
};
constexpr bool عملگر==(ثابت VEdgeType& lhs, ثابت VEdgeType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VEdgeType& lhs, VEdgeType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VEdgeType::en lhs, ثابت VEdgeType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#تعریف_کن FOR_EACH_EVAL(macro) \
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
#تعریف_کن VL_EVAL_ID(id, iterated, triggers, اولی, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#لغو_تعریف VL_EVAL_ID
            ENUM_انتها
    };
    تعداد en m_e;

    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت مقادیر[] = {
#تعریف_کن VL_EVAL_NAME(id, iterated, triggers, اولی, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#لغو_تعریف VL_EVAL_NAME
                "_ENUM_END"  //
        };
        أرجع مقادیر[m_e];
    }
    bool isIterated() ثابت {
        ساكن constexpr bool مقادیر[] = {
#تعریف_کن VL_EVAL_IS_ITERATED(id, iterated, triggers, اولی, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#لغو_تعریف VL_EVAL_IS_ITERATED
                false  //
        };
        أرجع مقادیر[m_e];
    }
    bool hasTriggers() ثابت {
        ساكن constexpr bool مقادیر[] = {
#تعریف_کن VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, اولی, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#لغو_تعریف VL_EVAL_HAS_TRIGGERS
                false  //
        };
        أرجع مقادیر[m_e];
    }
    bool اولی_Iteration() ثابت {
        ساكن constexpr bool مقادیر[] = {
#تعریف_کن VL_EVAL_اولی(id, iterated, triggers, اولی, slow) اولی,
            FOR_EACH_EVAL(VL_EVAL_اولی)
#لغو_تعریف VL_EVAL_اولی
                false  //
        };
        أرجع مقادیر[m_e];
    }
    bool slow() ثابت {
        ساكن constexpr bool مقادیر[] = {
#تعریف_کن VL_EVAL_SLOW(id, iterated, triggers, اولی, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#لغو_تعریف VL_EVAL_SLOW
                false  //
        };
        أرجع مقادیر[m_e];
    }

    // Short name
    std::string برچسب() ثابت { أرجع VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ثابت { أرجع "_eval_" + برچسب(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string eval_روش() ثابت { أرجع "eval" + capitalized_برچسب(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ثابت { أرجع "_eval_dump_triggers__" + برچسب(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dump_Triggers_روش() ثابت { أرجع "dumpTriggers" + capitalized_برچسب(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(صحيح _e)
        : m_e(static_cast<en>(_e)) {}
    عملگر en() ثابت { أرجع m_e; }

خاص:
    std::string capitalized_برچسب() ثابت {
        ثابت std::string name = ascii();
        أرجع name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#لغو_تعریف FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
عام:
    تعداد en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        أرجع نامها[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    // Is a node type compatible with the declaration
    bool is_گره_Compatible(ثابت Ast_گره* nodep) ثابت;
};
constexpr bool عملگر==(ثابت VFwdType& lhs, ثابت VFwdType& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عملگر==(ثابت VFwdType& lhs, VFwdType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VFwdType::en lhs, ثابت VFwdType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VFwdType& rhs) {
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
    فراغ مجموعه(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    فراغ clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ساكن فراغ clear_Cache_درخت() {
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        أرجع نامها[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"join", "join_any", "join_none"};
        أرجع نامها[m_e];
    }
    bool join() ثابت { أرجع m_e == JOIN; }
    bool joinAny() ثابت { أرجع m_e == JOIN_ANY; }
    bool joinNone() ثابت { أرجع m_e == JOIN_NONE; }
};
constexpr bool عملگر==(ثابت VJoinType& lhs, ثابت VJoinType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VJoinType& lhs, VJoinType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VJoinType::en lhs, ثابت VJoinType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VJoinType& rhs) {
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
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        أرجع نامها[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
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
constexpr bool عملگر==(ثابت VLifetime& lhs, ثابت VLifetime& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VLifetime& lhs, VLifetime::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VLifetime::en lhs, ثابت VLifetime& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VLifetime& rhs) {
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
            صحيح m_شاخص;  // Current index
            ثابت صحيح m_inc;  // Step, +1 or -1
        عام:
            Iterator(صحيح شاخص, صحيح inc)
                : m_شاخص{شاخص}
                , m_inc{inc} {}
            صحيح عملگر*() ثابت { أرجع m_شاخص; }
            Iterator& عملگر++() {
                m_شاخص += m_inc;
                أرجع *this;
            }
            bool عملگر!=(ثابت Iterator& that) ثابت { أرجع m_شاخص != that.m_شاخص; }
        };
        ثابت صحيح m_begin;  // First index
        ثابت صحيح m_انتها;  // One past the last index
        ثابت صحيح m_inc;  // Step, +1 or -1
    عام:
        Sequence(صحيح اولی, صحيح آخرین, صحيح inc)
            : m_begin{اولی}
            , m_انتها{آخرین + inc}
            , m_inc{inc} {}
        Iterator begin() ثابت { أرجع Iterator{m_begin, m_inc}; }
        Iterator انتها() ثابت { أرجع Iterator{m_انتها, m_inc}; }
    };

    صحيح m_چپ = 0;  // Left side of range (pre-':')
    صحيح m_راست = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool عملگر==(ثابت VNumRange& rhs) ثابت {
        أرجع m_چپ == rhs.m_چپ && m_راست == rhs.m_راست && m_ranged == rhs.m_ranged;
    }
    bool عملگر<(ثابت VNumRange& rhs) ثابت {
        إذا ((m_چپ < rhs.m_چپ)) أرجع true;
        إذا (!(m_چپ == rhs.m_چپ)) أرجع false;  // lhs > rhs
        إذا ((m_راست < rhs.m_راست)) أرجع true;
        إذا (!(m_راست == rhs.m_راست)) أرجع false;  // lhs > rhs
        إذا ((m_ranged < rhs.m_ranged)) أرجع true;
        إذا (!(m_ranged == rhs.m_ranged)) أرجع false;  // lhs > rhs
        أرجع false;
    }
    //
    VNumRange() = افتراضي;
    VNumRange(صحيح hi, صحيح lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(صحيح چپ_2, صحيح راست_2)
        : m_چپ{چپ_2}
        , m_راست{راست_2}
        , m_ranged{true} {}
    ~VNumRange() = افتراضي;
    // MEMBERS
    فراغ init(صحيح hi, صحيح lo, bool ascending) {
        إذا (lo > hi) std::جابجا(hi, lo);
        m_چپ = ascending ? lo : hi;
        m_راست = ascending ? hi : lo;
        m_ranged = true;
    }
    صحيح چپ_2() ثابت { أرجع m_چپ; }
    صحيح راست_2() ثابت { أرجع m_راست; }
    صحيح hi() ثابت VL_MT_امن {
        أرجع m_چپ > m_راست ? m_چپ : m_راست;
    }  // How to show a declaration
    صحيح lo() ثابت VL_MT_امن {
        أرجع m_چپ > m_راست ? m_راست : m_چپ;
    }  // How to show a declaration
    صحيح چپ_مقصد_راست_Inc() ثابت { أرجع ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_چپ_مقصد_راست() ثابت { أرجع {m_چپ, m_راست, چپ_مقصد_راست_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_راست_مقصد_چپ() ثابت { أرجع {m_راست, m_چپ, -چپ_مقصد_راست_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seq_Lo_مقصد_Hi() ثابت { أرجع {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seq_Hi_مقصد_Lo() ثابت { أرجع {hi(), lo(), -1}; }
    صحيح elements() ثابت VL_MT_امن { أرجع hi() - lo() + 1; }
    bool ranged() ثابت { أرجع m_ranged; }
    bool ascending() ثابت { أرجع m_چپ < m_راست; }
    صحيح hiMaxSelect() ثابت {
        أرجع (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ثابت {
        إذا (ranged()) {
            أرجع "["s + std::مقصد_string(چپ_2()) + ":" + std::مقصد_string(راست_2()) + "]";
        } وإلا {
            أرجع "[norg]";
        }
    }
    فراغ dump(std::ostream& str) ثابت { str << ascii(); }
};
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VNumRange& rhs) {
    rhs.dump(os);
    أرجع os;
}

// ######################################################################

class VPragmaType final {
عام:
    تعداد en : uint8_t {
        COVERAGE_بلوک_OFF,
        HIER_بلوک,
        HIER_PARAMS,
        INLINE_MODULE,
        NO_INLINE_MODULE,
        NO_INLINE_TASK,
        عمومی_MODULE,
        عمومی_TASK,
        TIMEUNIT_مجموعه,
        UNROLL_DISABLE,
        UNROLL_پر,
        پر_CASE,
        PARALLEL_CASE,
        VERILATOR_LIB,
        ENUM_اندازه
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {
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
        أرجع نامها[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
};
constexpr bool عملگر==(ثابت VPragmaType& lhs, ثابت VPragmaType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VPragmaType& lhs, VPragmaType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VPragmaType::en lhs, ثابت VPragmaType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VPragmaType& rhs) {
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
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        أرجع نامها[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr عملگر en() ثابت { أرجع m_e; }
    bool isRandomizable() ثابت { أرجع m_e != NONE; }
    bool isRand() ثابت { أرجع m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ثابت { أرجع m_e == RAND_CYCLIC; }
};
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VRandAttr& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelf_اشاره_گر_متن final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ساكن ثابت std::shared_ptr<ثابت string> s_emptyp;  // Holds ""
    ساكن ثابت std::shared_ptr<ثابت string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ثابت string> m_strp;

عام:
    // CONSTRUCTORS
    class خالی_2 {};  // for creator type-overload selection
    explicit VSelf_اشاره_گر_متن(خالی_2)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelf_اشاره_گر_متن(This)
        : m_strp{s_thisp} {}
    VSelf_اشاره_گر_متن(This, ثابت string& فیلد)
        : m_strp{std::make_shared<ثابت string>("this->" + فیلد)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelf_اشاره_گر_متن(VlSyms, ثابت string& فیلد)
        : m_strp{std::make_shared<ثابت string>("(&vlSymsp->vlInst." + فیلد + ')')} {}

    // METHODS
    bool is_خالی() ثابت { أرجع m_strp == s_emptyp; }
    bool isVlSym() ثابت { أرجع m_strp->پیدا_کن("vlSymsp") != string::npos; }
    bool hasThis() ثابت { أرجع m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ثابت;
    ساكن string replaceThis(bool useSelfForThis, ثابت string& متن);
    ثابت std::string& asString() ثابت { أرجع *m_strp; }
    bool عملگر==(ثابت VSelf_اشاره_گر_متن& other) ثابت { أرجع *m_strp == *other.m_strp; }
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
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        أرجع نامها[m_e];
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    bool isSigned() ثابت VL_MT_امن { أرجع m_e == SIGNED; }
    bool isNosign() ثابت VL_MT_امن { أرجع m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool عملگر==(ثابت VSigning& lhs, ثابت VSigning& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عملگر==(ثابت VSigning& lhs, VSigning::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VSigning::en lhs, ثابت VSigning& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VSigning& rhs) {
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VStrength& lhs, ثابت VStrength& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VStrength& lhs, VStrength::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VStrength::en lhs, ثابت VStrength& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VStrength& rhs) {
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
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        أرجع نامها[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr عملگر en() ثابت { أرجع m_e; }
};

//######################################################################

class VTrace_پیشوند_Type final {
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
    constexpr VTrace_پیشوند_Type(en _e)
        : m_e{_e} {}
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VTrace_پیشوند_Type& lhs, ثابت VTrace_پیشوند_Type& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VTrace_پیشوند_Type& lhs, VTrace_پیشوند_Type::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool عملگر==(VTrace_پیشوند_Type::en lhs, ثابت VTrace_پیشوند_Type& rhs) {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VTrace_پیشوند_Type& rhs) {
    أرجع os << rhs.ascii();
}

//######################################################################

class VTraceType final {
عام:
    تعداد en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        پر,  // Full value dump (always emitted)
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"CONSTANT", "FULL", "CHANGE"};
        أرجع نامها[m_e];
    }
    ثابت حرف* func_پیشوند() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"trace_const", "trace_full", "trace_chg"};
        أرجع نامها[m_e];
    }
};
constexpr bool عملگر==(ثابت VTraceType& lhs, ثابت VTraceType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VTraceType& lhs, VTraceType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VTraceType::en lhs, ثابت VTraceType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VTraceType& rhs) {
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    bool شامل_است_Any(VUseType other) ثابت { أرجع m_e & other.m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        أرجع نامها[m_e - 1];
    }
};
constexpr bool عملگر==(ثابت VUseType& lhs, ثابت VUseType& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool عملگر==(ثابت VUseType& lhs, VUseType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VUseType::en lhs, ثابت VUseType& rhs) { أرجع lhs == rhs.m_e; }
constexpr VUseType::en عملگر|(VUseType::en lhs, VUseType::en rhs) {
    أرجع VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en عملگر&(VUseType::en lhs, VUseType::en rhs) {
    أرجع VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VUseType& rhs) {
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
        پورت,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    تعداد en m_e;
    VVarType() VL_MT_امن : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_امن : m_e{_e} {}
    explicit VVarType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عملگر en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نامها[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        أرجع نامها[m_e];
    }
    bool isParam() ثابت { أرجع m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ثابت {
        أرجع (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == پورت || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
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
                || m_e == TRI0 || m_e == TRI1 || m_e == پورت || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ثابت {
        أرجع (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool is_موقت() ثابت {
        أرجع (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ثابت {
        أرجع (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == پورت
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ثابت حرف* trace_Sig_نوع() ثابت {
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
constexpr bool عملگر==(ثابت VVarType& lhs, ثابت VVarType& rhs) VL_MT_امن {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VVarType& lhs, VVarType::en rhs) VL_MT_امن {
    أرجع lhs.m_e == rhs;
}
constexpr bool عملگر==(VVarType::en lhs, ثابت VVarType& rhs) VL_MT_امن {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VVarType& rhs) VL_MT_امن {
    أرجع os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_کلید final {
عام:
    ثابت صحيح m_عرض;  // From AstNodeDType: Bit width of operation
    ثابت صحيح m_عرض_Min;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ثابت VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ثابت VSigning m_numeric;  // From AstNodeDType: Node is signed
    ثابت VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool عملگر==(ثابت VBasic_Type_کلید& rhs) ثابت {
        أرجع m_عرض == rhs.m_عرض && m_عرض_Min == rhs.m_عرض_Min && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool عملگر<(ثابت VBasic_Type_کلید& rhs) ثابت {
        إذا ((m_عرض < rhs.m_عرض)) أرجع true;
        إذا (!(m_عرض == rhs.m_عرض)) أرجع false;  // lhs > rhs
        إذا ((m_عرض_Min < rhs.m_عرض_Min)) أرجع true;
        إذا (!(m_عرض_Min == rhs.m_عرض_Min)) أرجع false;  // lhs > rhs
        إذا ((m_numeric < rhs.m_numeric)) أرجع true;
        إذا (!(m_numeric == rhs.m_numeric)) أرجع false;  // lhs > rhs
        إذا ((m_keyword < rhs.m_keyword)) أرجع true;
        إذا (!(m_keyword == rhs.m_keyword)) أرجع false;  // lhs > rhs
        إذا ((m_nrange < rhs.m_nrange)) أرجع true;
        إذا (!(m_nrange == rhs.m_nrange)) أرجع false;  // lhs > rhs
        أرجع false;
    }
    VBasic_Type_کلید(صحيح عرض, صحيح عرض_Min, VSigning numeric, VBasicDTypeKwd kwd,
                  ثابت VNumRange& nrange)
        : m_عرض{عرض}
        , m_عرض_Min{عرض_Min}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_کلید() = افتراضي;
};

#پایان_اگر  // Guard
