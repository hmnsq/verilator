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

#که_نه_تعریف_شوی VERILATOR_V3ASTATTR_H_
#تعریف_کړه VERILATOR_V3ASTATTR_H_

#که_نه_تعریف_شوی VERILATOR_V3AST_H_
#تېروتنه "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کړه VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#د_که_پای

// Hint class so we can choose constructors
class VFlag_بټ_Packed {};
class VFlag_ماشوم_DType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbort_ډول final {
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
    constexpr VAbort_ډول(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت نومونه[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        أرجع نومونه[m_e];
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
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"RD", "WR", "RW", "--"};
        أرجع نومونه[m_e];
    }
    ثابت حرف* arrow() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        أرجع نومونه[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    VAccess invert() ثابت {
        أرجع (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ثابت { أرجع m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ثابت { أرجع m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ثابت { أرجع m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ثابت { أرجع m_e == WRITE || m_e == READWRITE; }
    bool isRW() ثابت { أرجع m_e == READWRITE; }
};
constexpr bool چلوونکی==(ثابت VAccess& lhs, ثابت VAccess& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool چلوونکی==(ثابت VAccess& lhs, VAccess::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VAccess::en lhs, ثابت VAccess& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VAccess& rhs) { أرجع os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
عام:
    تعداد en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ټاکل };
    تعداد en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VAlwaysKwd& lhs, ثابت VAlwaysKwd& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VAlwaysKwd::en lhs, ثابت VAlwaysKwd& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
عام:
    // IEEE 1800-2023 Table 20-5
    تعداد en : uint8_t {
        منزل_BE_EVALUATED = 0,
        قلف = 1,
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        // IEEE 1800-2023 20.11
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"",
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
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VAssertCtlType& lhs, ثابت VAssertCtlType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی==(VAssertCtlType::en lhs, ثابت VAssertCtlType& rhs) {
    أرجع lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
عام:
    // IEEE 1800-2023 Table 20-7
    تعداد en : uint8_t {
        دننی = 0,  // Non IEEE type, for directives to be evaluated from expression.
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
        إذا (m_e == دننی)
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
        UASSERT(!str.تشه(), "Assert should be of one of types");
        أرجع str.substr(0, str.اندازه() - 1);
    }
    constexpr چلوونکی en() ثابت { أرجع m_e; }
};
constexpr bool چلوونکی==(ثابت VAssertDirectiveType& lhs, ثابت VAssertDirectiveType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی==(VAssertDirectiveType::en lhs, ثابت VAssertDirectiveType& rhs) {
    أرجع lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en چلوونکی|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    أرجع VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
عام:
    // IEEE 1800-2023 Table 20-6
    تعداد en : uint8_t {
        دننی = 0,  // Non IEEE type, for assertions that should not be controlled.
        CONCURRENT = (1 << 0),
        SIMPLE_سمدستي = (1 << 1),
        OBSERVED_DEFERRED_سمدستي = (1 << 2),
        FINAL_DEFERRED_سمدستي = (1 << 3),
        EXPECT = (1 << 4),
        UNIQUE = (1 << 5),
        UNIQUE0 = (1 << 6),
        لومړیتوب = (1 << 7),
    };
    تعداد en m_e;
    VAssertType()
        : m_e{دننی} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool لري_Any(VAssertType other) ثابت { أرجع m_e & other.m_e; }
    string ascii() ثابت {
        std::stringstream types;
        إذا (m_e == دننی)
            types << "INTERNAL ";
        وإلا {
            إذا (m_e & CONCURRENT) types << "CONCURRENT ";
            إذا (m_e & SIMPLE_سمدستي) types << "SIMPLE_IMMEDIATE ";
            إذا (m_e & OBSERVED_DEFERRED_سمدستي) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            إذا (m_e & FINAL_DEFERRED_سمدستي) types << "FINAL_DEFERRED_IMMEDIATE ";
            إذا (m_e & EXPECT) types << "EXPECT ";
            إذا (m_e & UNIQUE) types << "UNIQUE ";
            إذا (m_e & UNIQUE0) types << "UNIQUE0 ";
            إذا (m_e & لومړیتوب) types << "PRIORITY ";
        }
        ثابت string str = types.str();
        UASSERT(!str.تشه(), "Assert should be of one of types");
        أرجع str.substr(0, str.اندازه() - 1);
    }
    constexpr چلوونکی en() ثابت { أرجع m_e; }
};
constexpr bool چلوونکی==(ثابت VAssertType& lhs, ثابت VAssertType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VAssertType& lhs, VAssertType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VAssertType::en lhs, ثابت VAssertType& rhs) { أرجع lhs == rhs.m_e; }
constexpr VAssertType::en چلوونکی|(VAssertType::en lhs, VAssertType::en rhs) {
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
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"default", "weak", "strong"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VPropStrength& lhs, ثابت VPropStrength& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی!=(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    أرجع lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
عام:
    // clang-format off
    تعداد en: uint8_t  {
        ILLEGAL,
        //
        DIM_بټونه,                       // V3Const converts to constant
        DIM_بټونه_OR_شمېره,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_لوړ,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_کیڼ,                       // V3Width processes
        DIM_ټیټ,                        // V3Width processes
        DIM_ښی,                      // V3Width processes
        DIM_اندازه,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_عامه,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_لومړی,                     // V3Width processes
        ENUM_وروستی,                      // V3Width processes
        ENUM_عدد,                       // V3Width processes
        ENUM_بل,                      // V3Width processes
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
        VAR_بنسټ,                       // V3LinkResolve creates for AstPreSel, V3LinkParam removes
        VAR_FORCEABLE,                  // V3LinkParse moves to AstVar::isForceable
        VAR_FSM_ARC_INCLUDE_COND,       // V3LinkParse moves to AstVar::attrFsmArcInclCond
        VAR_FSM_ARC_INCLUDE_COND_AUTO,  // Flavors of above option - setting expand mode per FSM
        VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND,
        VAR_FSM_ARC_INCLUDE_COND_ډک,
        VAR_FSM_STATE,                  // V3LinkParse moves to AstVar::attrFsmState
        VAR_FSM_STATE_AUTO,             // Flavors of above option - setting expand mode per FSM
        VAR_FSM_STATE_AUTO_EXPAND,
        VAR_FSM_STATE_ډک,
        VAR_پورټ_DTYPE,                 // V3LinkDot for V3Width to check port dtype
        VAR_عامه,                     // V3LinkParse moves to AstVar::sigPublic
        VAR_عامه_FLAT,                // V3LinkParse moves to AstVar::sigPublic
        VAR_عامه_FLAT_RD,             // V3LinkParse moves to AstVar::sigPublic
        VAR_عامه_FLAT_RW,             // V3LinkParse moves to AstVar::sigPublic
        VAR_SC_BIGUINT,                 // V3LinkParse moves to AstVar::attrScBigUint
        VAR_SC_BV,                      // V3LinkParse moves to AstVar::attrScBv
        VAR_SFORMAT,                    // V3LinkParse moves to AstVar::attrSFormat
        VAR_وېشل_VAR                   // V3LinkParse moves to AstVar::attrSplitVar
    };
    // clang-format on
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        // clang-format off
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {
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
        أرجع نومونه[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ثابت {
        بدل (m_e) {
        حالة DIM_بټونه:
        حالة DIM_بټونه_OR_شمېره:
        حالة DIM_DIMENSIONS:
        حالة DIM_لوړ:
        حالة DIM_INCREMENT:
        حالة DIM_کیڼ:
        حالة DIM_ټیټ:
        حالة DIM_ښی:
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
};
constexpr bool چلوونکی==(ثابت VAttrType& lhs, ثابت VAttrType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VAttrType& lhs, VAttrType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VAttrType::en lhs, ثابت VAttrType& rhs) { أرجع lhs == rhs.m_e; }

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
        string بهر;
        إذا (m_initial) بهر = VString::dot(بهر, " ", "initial");
        إذا (m_extends) بهر = VString::dot(بهر, " ", "extends");
        إذا (m_final) بهر = VString::dot(بهر, " ", "final");
        أرجع بهر;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
عام:
    تعداد en : uint8_t {
        UNKNOWN,
        بټ,
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
        وخت,
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
    ثابت حرف* ascii() ثابت VL_MT_خوندي {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"%E-unk",
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
        أرجع نومونه[m_e];
    }
    ثابت حرف* typeName() ثابت VL_MT_خوندي {
        بدل (m_e) {
        حالة LOGIC2STATE: أرجع VBasicDTypeKwd{LOGIC}.ascii();
        حالة INTEGER2STATE: أرجع VBasicDTypeKwd{INTEGER}.ascii();
        حالة TIME2STATE: أرجع VBasicDTypeKwd{وخت}.ascii();
        افتراضي: أرجع ascii();
        }
    }
    ثابت حرف* dpiType() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"%E-unk",
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
        أرجع نومونه[m_e];
    }
    ساكن فراغ selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_خوندي : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_خوندي : m_e{_e} {}
    explicit VBasicDTypeKwd(صحيح _e) VL_MT_خوندي  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    صحيح پلنوالی() ثابت {
        بدل (m_e) {
        حالة بټ: أرجع 1;  // scalar, can't bit extract unless ranged
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
        حالة وخت: أرجع 64;
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
               || m_e == UINT32 || m_e == UINT64 || m_e == بټ || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == وخت || m_e == TIME2STATE;
    }
    bool isFourstate() ثابت {
        أرجع m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == وخت;
    }
    bool isZeroInit() ثابت {  // Otherwise initializes to X
        أرجع (m_e == بټ || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ثابت {  // Enum increment supported
        أرجع (m_e == بټ || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == وخت || m_e == TIME2STATE);
    }
    bool is_بټ() ثابت { أرجع m_e == بټ; }
    bool is_بټ_Logic() ثابت {  // Bit/logic vector types; can form a packed array
        أرجع (m_e == LOGIC || m_e == LOGIC2STATE || m_e == بټ);
    }
    bool isDpiUnsignable() ثابت {  // Can add "unsigned" to DPI
        أرجع (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ثابت {  // Uses standard C layout, for DPI runtime access
        أرجع (m_e == بټ || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ثابت VL_MT_خوندي {  // IE not a simple number we can bit optimize
        أرجع (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ثابت VL_MT_خوندي { أرجع m_e == CHANDLE; }
    bool isDouble() ثابت VL_MT_خوندي { أرجع m_e == DOUBLE; }
    bool isEvent() ثابت { أرجع m_e == EVENT; }
    bool isString() ثابت VL_MT_خوندي { أرجع m_e == STRING; }
    bool isMTaskState() ثابت VL_MT_خوندي { أرجع m_e == MTASKSTATE; }
    bool isSameish(ثابت VBasicDTypeKwd& other) ثابت VL_MT_خوندي {
        بدل (other.m_e) {
        حالة LOGIC:
        حالة LOGIC2STATE:
        حالة LOGIC_IMPLICIT: أرجع m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        حالة INTEGER:
        حالة INTEGER2STATE: أرجع m_e == INTEGER || m_e == INTEGER2STATE;
        حالة وخت:
        حالة TIME2STATE: أرجع m_e == وخت || m_e == TIME2STATE;
        افتراضي: أرجع m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_خوندي {
        بدل (m_e) {
        حالة بټ:
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
constexpr bool چلوونکی==(ثابت VBasicDTypeKwd& lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_خوندي {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_خوندي {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی==(VBasicDTypeKwd::en lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_خوندي {
    أرجع lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
عام:
    تعداد en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, ENUM_پای_ټکی };
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
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"FALSE", "TRUE", "UNK"};
        أرجع نومونه[m_e];
    }
    bool is_معلوم() ثابت { أرجع m_e != BU_UNKNOWN; }
    bool true_معلوم() ثابت { أرجع m_e == BU_TRUE; }
    فراغ سېټ_True_Or_False(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool چلوونکی==(ثابت VBoolOrUnknown& lhs, ثابت VBoolOrUnknown& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی==(VBoolOrUnknown::en lhs, ثابت VBoolOrUnknown& rhs) {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VBoolOrUnknown& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
عام:
    تعداد en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, ENUM_پای_ټکی };
    تعداد en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    bool unknown() ثابت { أرجع m_e == BP_UNKNOWN; }
    bool likely() ثابت { أرجع m_e == BP_LIKELY; }
    bool unlikely() ثابت { أرجع m_e == BP_UNLIKELY; }
    VBranchPred invert() ثابت {
        إذا (m_e == BP_UNLIKELY) أرجع BP_LIKELY;
        إذا (m_e == BP_LIKELY) أرجع BP_UNLIKELY;
        أرجع m_e;
    }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        أرجع نومونه[m_e];
    }
    ثابت حرف* asciiShort() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"", "L", "!L"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VBranchPred& lhs, ثابت VBranchPred& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VBranchPred& lhs, VBranchPred::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VBranchPred::en lhs, ثابت VBranchPred& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VBranchPred& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ساكن constexpr bool validate_دلیلونه_Descriptor(ثابت حرف* descrp) {
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
#تعریف_کړه FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     خالص_2,   CLEAN,  "r") \
    macro(ARRAY_AT_شا,                      "atBack",                 خالص_2,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                خالص_2,   CLEAN,  "r") \
    macro(ARRAY_پیدا_کړه,                         "find",                   خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کړه_لومړی,                   "find_first",             خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کړه_لومړی_شاخص,             "find_first_index",       خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کړه_شاخص,                   "find_index",             خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کړه_وروستی,                    "find_last",              خالص_2,   CLEAN,  "") \
    macro(ARRAY_پیدا_کړه_وروستی_شاخص,              "find_last_index",        خالص_2,   CLEAN,  "") \
    macro(ARRAY_لومړی,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 خالص_2,   CLEAN,  "r") \
    macro(ARRAY_وروستی,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    خالص_2,   CLEAN,  "") \
    macro(ARRAY_بل,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_راوباسه_شا,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_راوباسه_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ټېل_شا,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ټېل_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_سرچپه,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  خالص_2,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   خالص_2,   CLEAN,  "") \
    macro(ARRAY_R_حاصل_ضرب,                    "r_product",              خالص_2,   false,  "") \
    macro(ARRAY_R_مجموعه,                        "r_sum",                  خالص_2,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  خالص_2,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_ټوټه,                        "slice",                  خالص_2,   CLEAN,  "r") \
    macro(ARRAY_ترتیبول,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 خالص_2,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_شاخص,                 "unique_index",           خالص_2,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 خالص_2,   CLEAN,  "r") \
    macro(ASSOC_لومړی,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_بل,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_اندازه,                         "size",                   خالص_2,   CLEAN,  "") \
    macro(CLASS_سېټ_RANDMODE,                 "set_randmode",           false,  CLEAN,  "r") \
    macro(COVERGROUP_جمع_کړه_ARRAY_NAMER,         "addArrayNamer",          false,  CLEAN,  "r+") \
    macro(COVERGROUP_جمع_کړه_BIN,                 "addBin",                 false,  CLEAN,  "r+") \
    macro(COVERGROUP_جمع_کړه_COVERPOINT,          "addCoverpoint",          false,  CLEAN,  "") \
    macro(COVERGROUP_جمع_کړه_CROSS,               "addCross",               false,  CLEAN,  "") \
    macro(COVERGROUP_جمع_کړه_CROSS_DYN,           "addCrossDyn",            false,  CLEAN,  "") \
    macro(COVERGROUP_جمع_کړه_NUMBERED_NAMER,      "addNumberedNamer",       false,  CLEAN,  "r+") \
    macro(COVERGROUP_جمع_کړه_SINGLE_NAMER,        "addSingleNamer",         false,  CLEAN,  "r+") \
    macro(COVERGROUP_ATTACH,                  "attach",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_CLEAR_موندنه_LIST,          "clearHitList",           false,  CLEAN,  "") \
    macro(COVERGROUP_COVERAGE,                "coverage",               false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_BEGIN,        "withBegin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_CROSS_WITH_پای_ټکی,          "withEnd",                false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_TEST,         "withTest",               false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_ارزښت,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_ارزښت_W,      "withValueW",             false,  CLEAN,  "rw") \
    macro(COVERGROUP_FINALIZE_BINS,           "finalizeBins",           false,  CLEAN,  "") \
    macro(COVERGROUP_INCREMENT_BIN,           "incrementBin",           false,  CLEAN,  "r") \
    macro(COVERGROUP_INIT,                    "init",                   false,  CLEAN,  "r+") \
    macro(COVERGROUP_INST_P,                  "p",                      خالص_2,   CLEAN,  "") \
    macro(COVERGROUP_LEND_وزن,             "lendWeight",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_ریکارډ_موندنه,              "recordHit",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ثبتوونکی_BINS,           "registerBins",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_SAMPLE,                  "sample",                 false,  CLEAN,  "") \
    macro(COVERGROUP_SAMPLE_IFFS,             "sample",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_SELECT_ALL,              "selectAll",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_AND,              "selectAnd",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_BIN,              "selectBin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM,              "selectDim",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM_پای_ټکی,          "selectDimEnd",           false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_OR,               "selectOr",               false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_RANGE,            "selectRange",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SELECT_RANGE_W,          "selectRangeW",           false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_پای_ټکی,               "sizedEnd",               خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_پای,            "sizedFinish",            false,  CLEAN,  "r+") \
    macro(COVERGROUP_SIZED_لومړی,             "sizedFirst",             خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_RANGE,             "sizedRange",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_RANGE_W,           "sizedRangeW",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_SAMPLE,            "sizedSample",            false,  CLEAN,  "rrr") \
    macro(COVERGROUP_SIZED_SAMPLE_W,          "sizedSampleW",           false,  CLEAN,  "rrr") \
    macro(COVERGROUP_TYPE_COVERAGE,           "typeCoverage",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_TYPE_وزن,             "typeWeight",             false,  CLEAN,  "r") \
    macro(COVERGROUP_ارزښت_EXCLUDED,          "valueExcluded",          خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_ارزښت_EXCLUDED_W,        "valueExcludedW",         خالص_2,   CLEAN,  "r") \
    macro(COVERGROUP_ارزښت_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_ارزښت_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_ارزښت_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_ارزښت_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_ارزښت_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ارزښت_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_ارزښت_TYPE,              "valueType",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_وزن,                  "weight",                 false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_BEGIN,              "withBegin",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_پای,             "withFinish",             false,  CLEAN,  "r+") \
    macro(COVERGROUP_WITH_HI,                 "withHi",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_HI_W,               "withHiW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_LO,                 "withLo",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_LO_W,               "withLoW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_بل,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_پرلپسې,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_پرلپسې_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_شا,           "atWriteAppendBack",      false,  CLEAN,  "r") \
    macro(DYN_CLEAR,                          "clear",                  false,  CLEAN,  "") \
    macro(DYN_ERASE,                          "erase",                  false,  CLEAN,  "r") \
    macro(DYN_ورننباسه,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_راوباسه,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_راوباسه_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ټېل,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ټېل_FRONT,                     "push_front",             false,  CLEAN,  "r") \
    macro(DYN_RENEW,                          "renew",                  false,  CLEAN,  "r") \
    macro(DYN_RENEW_COPY,                     "renew_copy",             false,  CLEAN,  "rr") \
    macro(DYN_RESIZE,                         "resize",                 false,  CLEAN,  "TODO") \
    macro(DYN_اندازه,                           "size",                   خالص_2,   CLEAN,  "") \
    macro(DYN_ټوټه,                          "slice",                  خالص_2,   CLEAN,  "rr") \
    macro(DYN_ټوټه_ټاکل,                   "sliceAssign",            false,  CLEAN,  "rrr") \
    macro(DYN_ټوټه_ټاکل_شا_شا,         "sliceAssignBackBack",    false,  CLEAN,  "rrr") \
    macro(DYN_ټوټه_ټاکل_FRONT_شا,        "sliceAssignFrontBack",   false,  CLEAN,  "rrr") \
    macro(DYN_ټوټه_شا_شا,                "sliceBackBack",          خالص_2,   CLEAN,  "rr") \
    macro(DYN_ټوټه_FRONT_شا,               "sliceFrontBack",         خالص_2,   CLEAN,  "rr") \
    macro(EVENT_CLEAR_FIRED,                  "clearFired",             false,  CLEAN,  "") \
    macro(EVENT_CLEAR_TRIGGERED,              "clearTriggered",         false,  CLEAN,  "") \
    macro(EVENT_FIRE,                         "fire",                   false,  CLEAN,  "") \
    macro(EVENT_IS_FIRED,                     "isFired",                خالص_2,   CLEAN,  "") \
    macro(EVENT_IS_TRIGGERED,                 "isTriggered",            خالص_2,   CLEAN,  "") \
    macro(FORCE_جمع_کړه,                          "addForce",               false,  CLEAN,  "rrrr+") \
    macro(FORCE_READ,                         "read",                   خالص_2,   CLEAN,  "r") \
    macro(FORCE_READ_شاخص,                   "readIndex",              خالص_2,   CLEAN,  "rr") \
    macro(FORCE_READ_SEL,                     "readSel",                خالص_2,   CLEAN,  "TODO") \
    macro(FORCE_RELEASE,                      "release",                false,  CLEAN,  "rr+") \
    macro(FORCE_TOUCH,                        "touch",                  false,  CLEAN,  "") \
    macro(FORK_بشپړ,                          "done",                   false,  CLEAN,  "rr") \
    macro(FORK_INIT,                          "init",                   false,  CLEAN,  "rr") \
    macro(FORK_JOIN,                          "join",                   false,  CLEAN,  "rrr") \
    macro(FORK_ON_KILL,                       "onKill",                 false,  CLEAN,  "r") \
    macro(NBA_COMMIT,                         "commit",                 false,  CLEAN,  "w") \
    macro(NBA_کتار_ته_ورزیاتول,                        "enqueue",                false,  CLEAN,  "r+") \
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
    macro(RANDOMIZER_سېټ_VAR_DISABLED,        "set_var_disabled",       false,  CLEAN,  "r") \
    macro(RANDOMIZER_CLEAR_VAR_DISABLED,      "clear_var_disabled",     false,  CLEAN,  "r") \
    macro(RANDOMIZER_MARK_VAR_STATIC,         "mark_var_static",        false,  CLEAN,  "r") \
    macro(RANDOMIZER_سېټ_STATIC_RANDMODE,     "set_static_randmode",    false,  CLEAN,  "r") \
    macro(RNG_GET_RANDSTATE,                  "__Vm_rng.get_randstate", خالص_2,   CLEAN,  "") \
    macro(RNG_سېټ_RANDSTATE,                  "__Vm_rng.set_randstate", false,  CLEAN,  "r") \
    macro(SCHED_ANY_TRIGGERED,                "anyTriggered",           false,  CLEAN,  "r") \
    macro(SCHED_AWAITING_CURRENT_وخت,        "awaitingCurrentTime",    خالص_2,   CLEAN,  "") \
    macro(SCHED_AWAITING_ZERO_DELAY,          "awaitingZeroDelay",      خالص_2,   CLEAN,  "") \
    macro(SCHED_READY,                        "ready",                  false,  CLEAN,  "r") \
    macro(SCHED_MOVE_منزل_RESUME_کتار,         "moveToResumeQueue",      false,  CLEAN,  "r") \
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
    macro(UNPACKED_ټاکل,                    "assign",                 false,  CLEAN,  "r") \
    macro(UNPACKED_FILL,                      "fill",                   false,  CLEAN,  "r") \
    macro(UNPACKED_NEQ,                       "neq",                    خالص_2,   CLEAN,  "r")
// clang-format on

class VCMethod final {
    ساكن constexpr bool خالص_2 = true;  // For macro expansion of 'pure' field only
    ساكن constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

عام:
    تعداد en : uint8_t {
#تعریف_کړه VL_CMETHOD_ID(id, لاره, خالص_3, clean, دلیلونه) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#تعریف_لغوه VL_CMETHOD_ID
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت ارزښتونه[] = {
#تعریف_کړه VL_CMETHOD_NAME(id, لاره, خالص_3, clean, دلیلونه) لاره,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#تعریف_لغوه VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        أرجع ارزښتونه[m_e];
    }
    bool is_خالص() ثابت VL_خالص {
        ساكن constexpr bool ارزښتونه[] = {
#تعریف_کړه VL_CMETHOD_خالص(id, لاره, خالص_3, clean, دلیلونه) خالص_3,
            FOR_EACH_CMETHOD(VL_CMETHOD_خالص)
#تعریف_لغوه VL_CMETHOD_خالص
                false  //
        };
        أرجع ارزښتونه[m_e];
    }
    bool isClean() ثابت VL_خالص {
        ساكن constexpr bool ارزښتونه[] = {
#تعریف_کړه VL_CMETHOD_CLEAN(id, لاره, خالص_3, clean, دلیلونه) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#تعریف_لغوه VL_CMETHOD_CLEAN
                false  //
        };
        أرجع ارزښتونه[m_e];
    }
    ثابت حرف* دلیلونه() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت ارزښتونه[] = {
#تعریف_کړه VL_CMETHOD_دلیلونه(id, لاره, خالص_3, clean, دلیلونه) دلیلونه,
            FOR_EACH_CMETHOD(VL_CMETHOD_دلیلونه)
#تعریف_لغوه VL_CMETHOD_دلیلونه
                ""  //
        };
        أرجع ارزښتونه[m_e];
    }
    // Return array method for given name
    ساكن VCMethod array_لاره(ثابت string& name);
};
constexpr bool چلوونکی==(ثابت VCMethod& lhs, ثابت VCMethod& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool چلوونکی==(ثابت VCMethod& lhs, VCMethod::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VCMethod::en lhs, ثابت VCMethod& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VCMethod& rhs) {
    أرجع os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#تعریف_کړه VL_CMETHOD_دلیلونه_CHECK(id, لاره, خالص_3, clean, دلیلونه) \
    static_assert(validate_دلیلونه_Descriptor(دلیلونه), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_دلیلونه_CHECK)
#تعریف_لغوه VL_CMETHOD_دلیلونه_CHECK

#تعریف_لغوه FOR_EACH_CMETHOD

// clang-format off
#تعریف_کړه FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_منزل_SOLVER_HEX,           "vlToSolverHex",          خالص_2,   "r")
// clang-format on

class VCFunction final {
    ساكن constexpr bool خالص_2 = true;  // For macro expansion of 'pure' field only

عام:
    تعداد en : uint8_t {
#تعریف_کړه VL_CFUNCTION_ID(id, لاره, خالص_3, دلیلونه) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#تعریف_لغوه VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن ثابت حرف* ثابت ارزښتونه[] = {
#تعریف_کړه VL_CFUNCTION_NAME(id, function, خالص_3, دلیلونه) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#تعریف_لغوه VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        أرجع ارزښتونه[m_e];
    }
    bool is_خالص() ثابت VL_خالص {
        ساكن ثابت bool ارزښتونه[] = {
#تعریف_کړه VL_CFUNCTION_خالص(id, لاره, خالص_3, دلیلونه) خالص_3,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_خالص)
#تعریف_لغوه VL_CFUNCTION_خالص
                false  //
        };
        أرجع ارزښتونه[m_e];
    }
    ثابت حرف* دلیلونه() ثابت VL_خالص {
        ساكن ثابت حرف* ثابت ارزښتونه[] = {
#تعریف_کړه VL_CFUNCTION_دلیلونه(id, لاره, خالص_3, دلیلونه) دلیلونه,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_دلیلونه)
#تعریف_لغوه VL_CFUNCTION_دلیلونه
                ""  //
        };
        أرجع ارزښتونه[m_e];
    }
};

// Static assert all argument descriptors are well formed
#تعریف_کړه VL_CFUNCTION_دلیلونه_CHECK(id, لاره, خالص_3, دلیلونه) \
    static_assert(validate_دلیلونه_Descriptor(دلیلونه), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_دلیلونه_CHECK)
#تعریف_لغوه VL_CFUNCTION_دلیلونه_CHECK

#تعریف_لغوه FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
عام:
    تعداد en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

خاص:
    بنية توکی final {
        تعداد en m_e;  // Statement's enum mnemonic, for checking
        ثابت حرف* m_name;  // Statements name, for debugging
    };
    ساكن توکی s_توکی_معلومات[];

عام:
    تعداد en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"none", "ctor_var_reset_call"};
        أرجع نومونه[m_e];
    }
    bool isNone() ثابت { أرجع m_e == NONE; }
};
constexpr bool چلوونکی==(ثابت VCStmtType& lhs, ثابت VCStmtType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VCStmtType& lhs, VCStmtType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VCStmtType::en lhs, ثابت VCStmtType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VCStmtType& rhs) {
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_خالص {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VCaseType& lhs, ثابت VCaseType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VCaseType& lhs, VCaseType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VCaseType::en lhs, ثابت VCaseType& rhs) { أرجع lhs == rhs.m_e; }

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
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        أرجع نومونه[m_e];
    }
    bool isAssignable() ثابت { أرجع m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
};
constexpr bool چلوونکی==(ثابت VCastable& lhs, ثابت VCastable& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VCastable& lhs, VCastable::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VCastable::en lhs, ثابت VCastable& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VCastable& rhs) {
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        أرجع نومونه[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        أرجع نومونه[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ثابت حرف* bin_سېټ_Enum() ثابت {
        بدل (m_e) {
        حالة BINS_IGNORE: أرجع "VlCovBinKind::KIND_IGNORE";
        حالة BINS_ILLEGAL: أرجع "VlCovBinKind::KIND_ILLEGAL";
        حالة BINS_DEFAULT: أرجع "VlCovBinKind::KIND_DEFAULT";
        افتراضي: أرجع "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool bin_Is_نورمال() ثابت {
        أرجع m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool چلوونکی==(ثابت VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    أرجع lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
عام:
    تعداد en : uint8_t {
        // Shared by option.* and type_option.*
        وزن,
        GOAL,
        AT_لږترلږه,
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
        یوځای_INSTANCES,
        DISTRIBUTE_لومړی,
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
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"weight",
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
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VCoverOptionType& lhs, VCoverOptionType::en rhs) {
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"", "[*]", "[->]", "[=]"};
        أرجع نومونه[m_e];
    }
    ثابت حرف* asciiJson() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        أرجع نومونه[m_e];
    }
};

//######################################################################

class VDirection final {
عام:
    تعداد en : uint8_t { NONE, ننوت, OUTPUT, INOUT, REF, CONSTREF };
    تعداد en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت VL_MT_خوندي { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        أرجع نومونه[m_e];
    }
    string verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        أرجع نومونه[m_e];
    }
    string prettyName() ثابت { أرجع verilogKwd(); }
    bool isAny() ثابت { أرجع m_e != NONE; }
    bool isInout() ثابت { أرجع m_e == INOUT; }
    bool isInoutOrRef() ثابت { أرجع m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_ننوت() ثابت { أرجع m_e == ننوت; }
    bool isNonOutput() ثابت {
        أرجع m_e == ننوت || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ثابت { أرجع m_e == OUTPUT; }
    bool isReadOnly() ثابت VL_MT_خوندي { أرجع m_e == ننوت || m_e == CONSTREF; }
    bool isWritable() ثابت VL_MT_خوندي { أرجع m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ثابت VL_MT_خوندي { أرجع m_e == REF; }
    bool isConstRef() ثابت VL_MT_خوندي { أرجع m_e == CONSTREF; }
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
        حالة ننوت:
        حالة CONSTREF: أرجع VAccess::READ;
        حالة OUTPUT: أرجع VAccess::WRITE;
        حالة INOUT:
        حالة REF: أرجع VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool چلوونکی==(ثابت VDirection& lhs, ثابت VDirection& rhs) VL_MT_خوندي {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VDirection& lhs, VDirection::en rhs) VL_MT_خوندي {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی==(VDirection::en lhs, ثابت VDirection& rhs) VL_MT_خوندي {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VDirection& rhs) {
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    bool جمع_کړه_Newline() ثابت { أرجع m_e != DT_WRITE; }
    bool needScopeTracking() ثابت { أرجع m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VDisplayType& lhs, ثابت VDisplayType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VDisplayType& lhs, VDisplayType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VDisplayType::en lhs, ثابت VDisplayType& rhs) { أرجع lhs == rhs.m_e; }

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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VDumpCtlType& lhs, ثابت VDumpCtlType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VDumpCtlType& lhs, VDumpCtlType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VDumpCtlType::en lhs, ثابت VDumpCtlType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
عام:
    // REMEMBER to edit the strings below too
    تعداد en : uint8_t {
        // These must be in general -> most specific order, as we sort by it
        // in V3Const::visit AstSenTree
        // Involving a variable
        ET_بدل_شوی,  // Value changed
        ET_BOTHEDGE,  // POSEDGE | NEGEDGE (i.e.: 'edge' in Verilog)
        ET_POSEDGE,
        ET_NEGEDGE,
        ET_EVENT,  // VlEventBase::isFired
        // Involving an expression
        ET_TRUE,
        ET_INITIAL_NBA,  // Event that is fired initially and never again
        //
        ET_COMBO,  // Sensitive to all combo inputs to this block
        ET_COMBO_ستوری,  // Sensitive to all combo inputs to this block (from .*)
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
    bool an_څنډه() ثابت { أرجع m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ثابت {
        بدل (m_e) {
        حالة ET_بدل_شوی: أرجع ET_بدل_شوی;
        حالة ET_BOTHEDGE: أرجع ET_BOTHEDGE;
        حالة ET_POSEDGE: أرجع ET_NEGEDGE;
        حالة ET_NEGEDGE: أرجع ET_POSEDGE;
        افتراضي: UASSERT_STATIC(0, "Inverting bad edgeType()"); أرجع ET_NEGEDGE;
        }
    }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        أرجع نومونه[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        أرجع نومونه[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusive_څنډه(ثابت VEdgeType& other) ثابت {
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
};
constexpr bool چلوونکی==(ثابت VEdgeType& lhs, ثابت VEdgeType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VEdgeType& lhs, VEdgeType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VEdgeType::en lhs, ثابت VEdgeType& rhs) { أرجع lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#تعریف_کړه FOR_EACH_EVAL(macro) \
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
#تعریف_کړه VL_EVAL_ID(id, iterated, triggers, لومړی, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#تعریف_لغوه VL_EVAL_ID
            ENUM_پای_ټکی
    };
    تعداد en m_e;

    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت ارزښتونه[] = {
#تعریف_کړه VL_EVAL_NAME(id, iterated, triggers, لومړی, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#تعریف_لغوه VL_EVAL_NAME
                "_ENUM_END"  //
        };
        أرجع ارزښتونه[m_e];
    }
    bool isIterated() ثابت {
        ساكن constexpr bool ارزښتونه[] = {
#تعریف_کړه VL_EVAL_IS_ITERATED(id, iterated, triggers, لومړی, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#تعریف_لغوه VL_EVAL_IS_ITERATED
                false  //
        };
        أرجع ارزښتونه[m_e];
    }
    bool hasTriggers() ثابت {
        ساكن constexpr bool ارزښتونه[] = {
#تعریف_کړه VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, لومړی, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#تعریف_لغوه VL_EVAL_HAS_TRIGGERS
                false  //
        };
        أرجع ارزښتونه[m_e];
    }
    bool لومړی_Iteration() ثابت {
        ساكن constexpr bool ارزښتونه[] = {
#تعریف_کړه VL_EVAL_لومړی(id, iterated, triggers, لومړی, slow) لومړی,
            FOR_EACH_EVAL(VL_EVAL_لومړی)
#تعریف_لغوه VL_EVAL_لومړی
                false  //
        };
        أرجع ارزښتونه[m_e];
    }
    bool slow() ثابت {
        ساكن constexpr bool ارزښتونه[] = {
#تعریف_کړه VL_EVAL_SLOW(id, iterated, triggers, لومړی, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#تعریف_لغوه VL_EVAL_SLOW
                false  //
        };
        أرجع ارزښتونه[m_e];
    }

    // Short name
    std::string نښان() ثابت { أرجع VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ثابت { أرجع "_eval_" + نښان(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string eval_لاره() ثابت { أرجع "eval" + capitalized_نښان(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ثابت { أرجع "_eval_dump_triggers__" + نښان(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dump_Triggers_لاره() ثابت { أرجع "dumpTriggers" + capitalized_نښان(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(صحيح _e)
        : m_e(static_cast<en>(_e)) {}
    چلوونکی en() ثابت { أرجع m_e; }

خاص:
    std::string capitalized_نښان() ثابت {
        ثابت std::string name = ascii();
        أرجع name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#تعریف_لغوه FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
عام:
    تعداد en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        أرجع نومونه[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    // Is a node type compatible with the declaration
    bool is_غوټه_Compatible(ثابت Ast_غوټه* nodep) ثابت;
};
constexpr bool چلوونکی==(ثابت VFwdType& lhs, ثابت VFwdType& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool چلوونکی==(ثابت VFwdType& lhs, VFwdType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VFwdType::en lhs, ثابت VFwdType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VFwdType& rhs) {
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
    فراغ سېټ(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    فراغ clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ساكن فراغ clear_Cache_ونه() {
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        أرجع نومونه[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"join", "join_any", "join_none"};
        أرجع نومونه[m_e];
    }
    bool join() ثابت { أرجع m_e == JOIN; }
    bool joinAny() ثابت { أرجع m_e == JOIN_ANY; }
    bool joinNone() ثابت { أرجع m_e == JOIN_NONE; }
};
constexpr bool چلوونکی==(ثابت VJoinType& lhs, ثابت VJoinType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VJoinType& lhs, VJoinType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VJoinType::en lhs, ثابت VJoinType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VJoinType& rhs) {
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
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        أرجع نومونه[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
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
constexpr bool چلوونکی==(ثابت VLifetime& lhs, ثابت VLifetime& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VLifetime& lhs, VLifetime::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VLifetime::en lhs, ثابت VLifetime& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VLifetime& rhs) {
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
            صحيح چلوونکی*() ثابت { أرجع m_شاخص; }
            Iterator& چلوونکی++() {
                m_شاخص += m_inc;
                أرجع *this;
            }
            bool چلوونکی!=(ثابت Iterator& that) ثابت { أرجع m_شاخص != that.m_شاخص; }
        };
        ثابت صحيح m_begin;  // First index
        ثابت صحيح m_پای_ټکی;  // One past the last index
        ثابت صحيح m_inc;  // Step, +1 or -1
    عام:
        Sequence(صحيح لومړی, صحيح وروستی, صحيح inc)
            : m_begin{لومړی}
            , m_پای_ټکی{وروستی + inc}
            , m_inc{inc} {}
        Iterator begin() ثابت { أرجع Iterator{m_begin, m_inc}; }
        Iterator پای_ټکی() ثابت { أرجع Iterator{m_پای_ټکی, m_inc}; }
    };

    صحيح m_کیڼ = 0;  // Left side of range (pre-':')
    صحيح m_ښی = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool چلوونکی==(ثابت VNumRange& rhs) ثابت {
        أرجع m_کیڼ == rhs.m_کیڼ && m_ښی == rhs.m_ښی && m_ranged == rhs.m_ranged;
    }
    bool چلوونکی<(ثابت VNumRange& rhs) ثابت {
        إذا ((m_کیڼ < rhs.m_کیڼ)) أرجع true;
        إذا (!(m_کیڼ == rhs.m_کیڼ)) أرجع false;  // lhs > rhs
        إذا ((m_ښی < rhs.m_ښی)) أرجع true;
        إذا (!(m_ښی == rhs.m_ښی)) أرجع false;  // lhs > rhs
        إذا ((m_ranged < rhs.m_ranged)) أرجع true;
        إذا (!(m_ranged == rhs.m_ranged)) أرجع false;  // lhs > rhs
        أرجع false;
    }
    //
    VNumRange() = افتراضي;
    VNumRange(صحيح hi, صحيح lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(صحيح کیڼ_2, صحيح ښی_2)
        : m_کیڼ{کیڼ_2}
        , m_ښی{ښی_2}
        , m_ranged{true} {}
    ~VNumRange() = افتراضي;
    // MEMBERS
    فراغ init(صحيح hi, صحيح lo, bool ascending) {
        إذا (lo > hi) std::بدلون(hi, lo);
        m_کیڼ = ascending ? lo : hi;
        m_ښی = ascending ? hi : lo;
        m_ranged = true;
    }
    صحيح کیڼ_2() ثابت { أرجع m_کیڼ; }
    صحيح ښی_2() ثابت { أرجع m_ښی; }
    صحيح hi() ثابت VL_MT_خوندي {
        أرجع m_کیڼ > m_ښی ? m_کیڼ : m_ښی;
    }  // How to show a declaration
    صحيح lo() ثابت VL_MT_خوندي {
        أرجع m_کیڼ > m_ښی ? m_ښی : m_کیڼ;
    }  // How to show a declaration
    صحيح کیڼ_منزل_ښی_Inc() ثابت { أرجع ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_کیڼ_منزل_ښی() ثابت { أرجع {m_کیڼ, m_ښی, کیڼ_منزل_ښی_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_ښی_منزل_کیڼ() ثابت { أرجع {m_ښی, m_کیڼ, -کیڼ_منزل_ښی_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seq_Lo_منزل_Hi() ثابت { أرجع {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seq_Hi_منزل_Lo() ثابت { أرجع {hi(), lo(), -1}; }
    صحيح elements() ثابت VL_MT_خوندي { أرجع hi() - lo() + 1; }
    bool ranged() ثابت { أرجع m_ranged; }
    bool ascending() ثابت { أرجع m_کیڼ < m_ښی; }
    صحيح hiMaxSelect() ثابت {
        أرجع (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ثابت {
        إذا (ranged()) {
            أرجع "["s + std::منزل_string(کیڼ_2()) + ":" + std::منزل_string(ښی_2()) + "]";
        } وإلا {
            أرجع "[norg]";
        }
    }
    فراغ dump(std::ostream& str) ثابت { str << ascii(); }
};
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VNumRange& rhs) {
    rhs.dump(os);
    أرجع os;
}

// ######################################################################

class VPragmaType final {
عام:
    تعداد en : uint8_t {
        COVERAGE_بلاک_OFF,
        HIER_بلاک,
        HIER_PARAMS,
        INLINE_MODULE,
        NO_INLINE_MODULE,
        NO_INLINE_TASK,
        عامه_MODULE,
        عامه_TASK,
        TIMEUNIT_سېټ,
        UNROLL_DISABLE,
        UNROLL_ډک,
        ډک_CASE,
        PARALLEL_CASE,
        VERILATOR_LIB,
        ENUM_اندازه
    };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {
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
        أرجع نومونه[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
};
constexpr bool چلوونکی==(ثابت VPragmaType& lhs, ثابت VPragmaType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VPragmaType& lhs, VPragmaType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VPragmaType::en lhs, ثابت VPragmaType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VPragmaType& rhs) {
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
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        أرجع نومونه[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    bool isRandomizable() ثابت { أرجع m_e != NONE; }
    bool isRand() ثابت { أرجع m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ثابت { أرجع m_e == RAND_CYCLIC; }
};
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VRandAttr& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelf_اشاره_ګر_متن final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ساكن ثابت std::shared_ptr<ثابت string> s_emptyp;  // Holds ""
    ساكن ثابت std::shared_ptr<ثابت string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ثابت string> m_strp;

عام:
    // CONSTRUCTORS
    class تشه_2 {};  // for creator type-overload selection
    explicit VSelf_اشاره_ګر_متن(تشه_2)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelf_اشاره_ګر_متن(This)
        : m_strp{s_thisp} {}
    VSelf_اشاره_ګر_متن(This, ثابت string& ډګر)
        : m_strp{std::make_shared<ثابت string>("this->" + ډګر)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelf_اشاره_ګر_متن(VlSyms, ثابت string& ډګر)
        : m_strp{std::make_shared<ثابت string>("(&vlSymsp->vlInst." + ډګر + ')')} {}

    // METHODS
    bool is_تشه() ثابت { أرجع m_strp == s_emptyp; }
    bool isVlSym() ثابت { أرجع m_strp->پیدا_کړه("vlSymsp") != string::npos; }
    bool hasThis() ثابت { أرجع m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ثابت;
    ساكن string replaceThis(bool useSelfForThis, ثابت string& متن);
    ثابت std::string& asString() ثابت { أرجع *m_strp; }
    bool چلوونکی==(ثابت VSelf_اشاره_ګر_متن& other) ثابت { أرجع *m_strp == *other.m_strp; }
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
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        أرجع نومونه[m_e];
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    bool isSigned() ثابت VL_MT_خوندي { أرجع m_e == SIGNED; }
    bool isNosign() ثابت VL_MT_خوندي { أرجع m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool چلوونکی==(ثابت VSigning& lhs, ثابت VSigning& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool چلوونکی==(ثابت VSigning& lhs, VSigning::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VSigning::en lhs, ثابت VSigning& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VSigning& rhs) {
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VStrength& lhs, ثابت VStrength& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VStrength& lhs, VStrength::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VStrength::en lhs, ثابت VStrength& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VStrength& rhs) {
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
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        أرجع نومونه[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr چلوونکی en() ثابت { أرجع m_e; }
};

//######################################################################

class VTrace_مختاړی_Type final {
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
    constexpr VTrace_مختاړی_Type(en _e)
        : m_e{_e} {}
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VTrace_مختاړی_Type& lhs, ثابت VTrace_مختاړی_Type& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VTrace_مختاړی_Type& lhs, VTrace_مختاړی_Type::en rhs) {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی==(VTrace_مختاړی_Type::en lhs, ثابت VTrace_مختاړی_Type& rhs) {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VTrace_مختاړی_Type& rhs) {
    أرجع os << rhs.ascii();
}

//######################################################################

class VTraceType final {
عام:
    تعداد en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        ډک,  // Full value dump (always emitted)
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"CONSTANT", "FULL", "CHANGE"};
        أرجع نومونه[m_e];
    }
    ثابت حرف* func_مختاړی() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"trace_const", "trace_full", "trace_chg"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VTraceType& lhs, ثابت VTraceType& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VTraceType& lhs, VTraceType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VTraceType::en lhs, ثابت VTraceType& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VTraceType& rhs) {
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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    bool لري_Any(VUseType other) ثابت { أرجع m_e & other.m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        أرجع نومونه[m_e - 1];
    }
};
constexpr bool چلوونکی==(ثابت VUseType& lhs, ثابت VUseType& rhs) { أرجع lhs.m_e == rhs.m_e; }
constexpr bool چلوونکی==(ثابت VUseType& lhs, VUseType::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VUseType::en lhs, ثابت VUseType& rhs) { أرجع lhs == rhs.m_e; }
constexpr VUseType::en چلوونکی|(VUseType::en lhs, VUseType::en rhs) {
    أرجع VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en چلوونکی&(VUseType::en lhs, VUseType::en rhs) {
    أرجع VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VUseType& rhs) {
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
        پورټ,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    تعداد en m_e;
    VVarType() VL_MT_خوندي : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_خوندي : m_e{_e} {}
    explicit VVarType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت نومونه[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        أرجع نومونه[m_e];
    }
    bool isParam() ثابت { أرجع m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ثابت {
        أرجع (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == پورټ || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
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
                || m_e == TRI0 || m_e == TRI1 || m_e == پورټ || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ثابت {
        أرجع (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool is_لنډمهاله() ثابت {
        أرجع (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ثابت {
        أرجع (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == پورټ
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ثابت حرف* trace_Sig_ډول() ثابت {
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
constexpr bool چلوونکی==(ثابت VVarType& lhs, ثابت VVarType& rhs) VL_MT_خوندي {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VVarType& lhs, VVarType::en rhs) VL_MT_خوندي {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی==(VVarType::en lhs, ثابت VVarType& rhs) VL_MT_خوندي {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VVarType& rhs) VL_MT_خوندي {
    أرجع os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_کلی final {
عام:
    ثابت صحيح m_پلنوالی;  // From AstNodeDType: Bit width of operation
    ثابت صحيح m_پلنوالی_Min;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ثابت VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ثابت VSigning m_numeric;  // From AstNodeDType: Node is signed
    ثابت VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool چلوونکی==(ثابت VBasic_Type_کلی& rhs) ثابت {
        أرجع m_پلنوالی == rhs.m_پلنوالی && m_پلنوالی_Min == rhs.m_پلنوالی_Min && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool چلوونکی<(ثابت VBasic_Type_کلی& rhs) ثابت {
        إذا ((m_پلنوالی < rhs.m_پلنوالی)) أرجع true;
        إذا (!(m_پلنوالی == rhs.m_پلنوالی)) أرجع false;  // lhs > rhs
        إذا ((m_پلنوالی_Min < rhs.m_پلنوالی_Min)) أرجع true;
        إذا (!(m_پلنوالی_Min == rhs.m_پلنوالی_Min)) أرجع false;  // lhs > rhs
        إذا ((m_numeric < rhs.m_numeric)) أرجع true;
        إذا (!(m_numeric == rhs.m_numeric)) أرجع false;  // lhs > rhs
        إذا ((m_keyword < rhs.m_keyword)) أرجع true;
        إذا (!(m_keyword == rhs.m_keyword)) أرجع false;  // lhs > rhs
        إذا ((m_nrange < rhs.m_nrange)) أرجع true;
        إذا (!(m_nrange == rhs.m_nrange)) أرجع false;  // lhs > rhs
        أرجع false;
    }
    VBasic_Type_کلی(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric, VBasicDTypeKwd kwd,
                  ثابت VNumRange& nrange)
        : m_پلنوالی{پلنوالی}
        , m_پلنوالی_Min{پلنوالی_Min}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_کلی() = افتراضي;
};

#د_که_پای  // Guard
