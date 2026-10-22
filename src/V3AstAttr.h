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
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
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
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        إرجاع names[m_e];
    }
    bool isAccept() ثابت { إرجاع m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
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
        ساكن constexpr ثابت حرف* ثابت names[] = {"RD", "WR", "RW", "--"};
        إرجاع names[m_e];
    }
    ثابت حرف* arrow() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        إرجاع names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    VAccess invert() ثابت {
        إرجاع (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ثابت { إرجاع m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ثابت { إرجاع m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ثابت { إرجاع m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ثابت { إرجاع m_e == WRITE || m_e == READWRITE; }
    bool isRW() ثابت { إرجاع m_e == READWRITE; }
};
constexpr bool operator==(ثابت VAccess& lhs, ثابت VAccess& rhs) { إرجاع lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VAccess& lhs, VAccess::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, ثابت VAccess& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VAccess& rhs) { إرجاع os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
عام:
    تعداد en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    تعداد en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VAlwaysKwd& lhs, ثابت VAlwaysKwd& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, ثابت VAlwaysKwd& rhs) { إرجاع lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
عام:
    // IEEE 1800-2023 Table 20-5
    تعداد en : uint8_t {
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
    تعداد en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        // IEEE 1800-2023 20.11
        ساكن constexpr ثابت حرف* ثابت names[] = {"",
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
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VAssertCtlType& lhs, ثابت VAssertCtlType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, ثابت VAssertCtlType& rhs) {
    إرجاع lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
عام:
    // IEEE 1800-2023 Table 20-7
    تعداد en : uint8_t {
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
        إذا (m_e == INTERNAL)
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
        UASSERT(!str.empty(), "Assert should be of one of types");
        إرجاع str.substr(0, str.size() - 1);
    }
    constexpr operator en() ثابت { إرجاع m_e; }
};
constexpr bool operator==(ثابت VAssertDirectiveType& lhs, ثابت VAssertDirectiveType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, ثابت VAssertDirectiveType& rhs) {
    إرجاع lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    إرجاع VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
عام:
    // IEEE 1800-2023 Table 20-6
    تعداد en : uint8_t {
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
    تعداد en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) ثابت { إرجاع m_e & other.m_e; }
    string ascii() ثابت {
        std::stringstream types;
        إذا (m_e == INTERNAL)
            types << "INTERNAL ";
        وإلا {
            إذا (m_e & CONCURRENT) types << "CONCURRENT ";
            إذا (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            إذا (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            إذا (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            إذا (m_e & EXPECT) types << "EXPECT ";
            إذا (m_e & UNIQUE) types << "UNIQUE ";
            إذا (m_e & UNIQUE0) types << "UNIQUE0 ";
            إذا (m_e & PRIORITY) types << "PRIORITY ";
        }
        ثابت string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        إرجاع str.substr(0, str.size() - 1);
    }
    constexpr operator en() ثابت { إرجاع m_e; }
};
constexpr bool operator==(ثابت VAssertType& lhs, ثابت VAssertType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAssertType& lhs, VAssertType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, ثابت VAssertType& rhs) { إرجاع lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    إرجاع VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
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
        ساكن constexpr ثابت حرف* ثابت names[] = {"default", "weak", "strong"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VPropStrength& lhs, ثابت VPropStrength& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator!=(ثابت VPropStrength& lhs, VPropStrength::en rhs) {
    إرجاع lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
عام:
    // clang-format off
    تعداد en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_العدد,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_أعلى,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_يسار,                       // V3Width processes
        DIM_أدنى,                        // V3Width processes
        DIM_يمين,                      // V3Width processes
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
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        // clang-format off
        ساكن constexpr ثابت حرف* ثابت names[] = {
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
        إرجاع names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ثابت {
        بدل (m_e) {
        حالة DIM_BITS:
        حالة DIM_BITS_OR_العدد:
        حالة DIM_DIMENSIONS:
        حالة DIM_أعلى:
        حالة DIM_INCREMENT:
        حالة DIM_يسار:
        حالة DIM_أدنى:
        حالة DIM_يمين:
        حالة DIM_SIZE:
        حالة DIM_UNPK_DIMENSIONS:
        حالة TYPEID:
        حالة TYPENAME: إرجاع true;
        افتراضي: إرجاع false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
};
constexpr bool operator==(ثابت VAttrType& lhs, ثابت VAttrType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VAttrType& lhs, VAttrType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, ثابت VAttrType& rhs) { إرجاع lhs == rhs.m_e; }

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
    bool isAny() ثابت { إرجاع m_extends | m_final | m_initial; }
    bool isExtends() ثابت { إرجاع m_extends; }
    bool isFinal() ثابت { إرجاع m_final; }
    bool isInitial() ثابت { إرجاع m_initial; }
    string ascii() ثابت {
        string out;
        إذا (m_initial) out = VString::dot(out, " ", "initial");
        إذا (m_extends) out = VString::dot(out, " ", "extends");
        إذا (m_final) out = VString::dot(out, " ", "final");
        إرجاع out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
عام:
    تعداد en : uint8_t {
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
    تعداد en m_e;
    ثابت حرف* ascii() ثابت VL_MT_SAFE {
        ساكن constexpr ثابت حرف* ثابت names[] = {"%E-unk",
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
        إرجاع names[m_e];
    }
    ثابت حرف* typeName() ثابت VL_MT_SAFE {
        بدل (m_e) {
        حالة LOGIC2STATE: إرجاع VBasicDTypeKwd{LOGIC}.ascii();
        حالة INTEGER2STATE: إرجاع VBasicDTypeKwd{INTEGER}.ascii();
        حالة TIME2STATE: إرجاع VBasicDTypeKwd{TIME}.ascii();
        افتراضي: إرجاع ascii();
        }
    }
    ثابت حرف* dpiType() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"%E-unk",
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
        إرجاع names[m_e];
    }
    ساكن فراغ selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(صحيح _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    صحيح width() ثابت {
        بدل (m_e) {
        حالة BIT: إرجاع 1;  // scalar, can't bit extract unless ranged
        حالة BYTE: إرجاع 8;
        حالة CHANDLE: إرجاع 64;
        حالة NULLTYPE: إرجاع 1;  // Historical - bit was used instead of NULLTYPE
        حالة EVENT: إرجاع 1;
        حالة INT: إرجاع 32;
        حالة INTEGER: إرجاع 32;
        حالة INTEGER2STATE: إرجاع 32;
        حالة LOGIC: إرجاع 1;  // scalar, can't bit extract unless ranged
        حالة LOGIC2STATE: إرجاع 1;  // scalar, can't bit extract unless ranged
        حالة LONGINT: إرجاع 64;
        حالة DOUBLE: إرجاع 64;  // opaque
        حالة SHORTINT: إرجاع 16;
        حالة TIME: إرجاع 64;
        حالة TIME2STATE: إرجاع 64;
        حالة STRING: إرجاع 64;  // opaque  // Just the pointer, for today
        حالة SCOPEPTR: إرجاع 0;  // opaque
        حالة CHARPTR: إرجاع 0;  // opaque
        حالة MTASKSTATE: إرجاع 0;  // opaque
        حالة DELAY_SCHEDULER: إرجاع 0;  // opaque
        حالة TRIGGER_SCHEDULER: إرجاع 0;  // opaque
        حالة DYNAMIC_TRIGGER_SCHEDULER: إرجاع 0;  // opaque
        حالة FORK_SYNC: إرجاع 0;  // opaque
        حالة PROCESS_REFERENCE: إرجاع 0;  // opaque
        حالة RANDOM_GENERATOR: إرجاع 0;  // opaque
        حالة RANDOM_STDGENERATOR: إرجاع 0;  // opaque
        حالة COVERGROUP_INSTHANDLE: إرجاع 0;  // opaque
        حالة UINT32: إرجاع 32;
        حالة UINT64: إرجاع 64;
        افتراضي: إرجاع 0;
        }
    }
    bool isSigned() ثابت {
        إرجاع m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ثابت {
        إرجاع m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() ثابت {
        إرجاع m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() ثابت {  // Otherwise initializes to X
        إرجاع (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ثابت {  // Enum increment supported
        إرجاع (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() ثابت { إرجاع m_e == BIT; }
    bool isBitLogic() ثابت {  // Bit/logic vector types; can form a packed array
        إرجاع (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() ثابت {  // Can add "unsigned" to DPI
        إرجاع (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ثابت {  // Uses standard C layout, for DPI runtime access
        إرجاع (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ثابت VL_MT_SAFE {  // IE not a simple number we can bit optimize
        إرجاع (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ثابت VL_MT_SAFE { إرجاع m_e == CHANDLE; }
    bool isDouble() ثابت VL_MT_SAFE { إرجاع m_e == DOUBLE; }
    bool isEvent() ثابت { إرجاع m_e == EVENT; }
    bool isString() ثابت VL_MT_SAFE { إرجاع m_e == STRING; }
    bool isMTaskState() ثابت VL_MT_SAFE { إرجاع m_e == MTASKSTATE; }
    bool isSameish(ثابت VBasicDTypeKwd& other) ثابت VL_MT_SAFE {
        بدل (other.m_e) {
        حالة LOGIC:
        حالة LOGIC2STATE:
        حالة LOGIC_IMPLICIT: إرجاع m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        حالة INTEGER:
        حالة INTEGER2STATE: إرجاع m_e == INTEGER || m_e == INTEGER2STATE;
        حالة TIME:
        حالة TIME2STATE: إرجاع m_e == TIME || m_e == TIME2STATE;
        افتراضي: إرجاع m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ثابت VL_MT_SAFE {
        بدل (m_e) {
        حالة BIT:
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
        حالة UINT64: إرجاع true;
        افتراضي: إرجاع false;
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
        إرجاع lut[m_e];
    }
};
constexpr bool operator==(ثابت VBasicDTypeKwd& lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_SAFE {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, ثابت VBasicDTypeKwd& rhs) VL_MT_SAFE {
    إرجاع lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
عام:
    تعداد en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
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
        ساكن constexpr ثابت حرف* ثابت names[] = {"FALSE", "TRUE", "UNK"};
        إرجاع names[m_e];
    }
    bool isKnown() ثابت { إرجاع m_e != BU_UNKNOWN; }
    bool trueKnown() ثابت { إرجاع m_e == BU_TRUE; }
    فراغ setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(ثابت VBoolOrUnknown& lhs, ثابت VBoolOrUnknown& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, ثابت VBoolOrUnknown& rhs) {
    إرجاع lhs == rhs.m_e;
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VBoolOrUnknown& rhs) {
    إرجاع os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
عام:
    تعداد en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    تعداد en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    bool unknown() ثابت { إرجاع m_e == BP_UNKNOWN; }
    bool likely() ثابت { إرجاع m_e == BP_LIKELY; }
    bool unlikely() ثابت { إرجاع m_e == BP_UNLIKELY; }
    VBranchPred invert() ثابت {
        إذا (m_e == BP_UNLIKELY) إرجاع BP_LIKELY;
        إذا (m_e == BP_LIKELY) إرجاع BP_UNLIKELY;
        إرجاع m_e;
    }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        إرجاع names[m_e];
    }
    ثابت حرف* asciiShort() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"", "L", "!L"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VBranchPred& lhs, ثابت VBranchPred& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VBranchPred& lhs, VBranchPred::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, ثابت VBranchPred& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VBranchPred& rhs) {
    إرجاع os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ساكن constexpr bool validateArgsDescriptor(ثابت حرف* descrp) {
    // Is "TODO"
    إذا (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        إرجاع true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    لكل (ثابت حرف* cp = descrp; *cp; ++cp) {
        إذا (*cp == '+') إرجاع cp != descrp && !cp[1];
        إذا (*cp != 'r' && *cp != 'w' && *cp != 'm') إرجاع false;
    }
    إرجاع true;
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
    macro(ARRAY_اسحب_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_اسحب_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ادفع_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ادفع_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_قيمة,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_قيمة_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_قيمة_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_قيمة_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_قيمة_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_قيمة_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_قيمة_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_قيمة_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_قيمة_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_قيمة_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_قيمة_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_أدرج,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_اسحب,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_اسحب_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ادفع,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ادفع_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    ساكن constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    ساكن constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

عام:
    تعداد en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
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
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت VL_PURE {
        ساكن constexpr ثابت حرف* ثابت قيم[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        إرجاع قيم[m_e];
    }
    bool isPure() ثابت VL_PURE {
        ساكن constexpr bool قيم[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        إرجاع قيم[m_e];
    }
    bool isClean() ثابت VL_PURE {
        ساكن constexpr bool قيم[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        إرجاع قيم[m_e];
    }
    ثابت حرف* args() ثابت VL_PURE {
        ساكن constexpr ثابت حرف* ثابت قيم[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        إرجاع قيم[m_e];
    }
    // Return array method for given name
    ساكن VCMethod arrayMethod(ثابت string& name);
};
constexpr bool operator==(ثابت VCMethod& lhs, ثابت VCMethod& rhs) { إرجاع lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VCMethod& lhs, VCMethod::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, ثابت VCMethod& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VCMethod& rhs) {
    إرجاع os << rhs.ascii();
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
    ساكن constexpr bool PURE = true;  // For macro expansion of 'pure' field only

عام:
    تعداد en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت VL_PURE {
        ساكن ثابت حرف* ثابت قيم[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        إرجاع قيم[m_e];
    }
    bool isPure() ثابت VL_PURE {
        ساكن ثابت bool قيم[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        إرجاع قيم[m_e];
    }
    ثابت حرف* args() ثابت VL_PURE {
        ساكن ثابت حرف* ثابت قيم[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        إرجاع قيم[m_e];
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
عام:
    تعداد en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

خاص:
    بنية Item final {
        تعداد en m_e;  // Statement's enum mnemonic, for checking
        ثابت حرف* m_name;  // Statements name, for debugging
    };
    ساكن Item s_itemData[];

عام:
    تعداد en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت VL_PURE {
        ساكن constexpr ثابت حرف* ثابت names[] = {"none", "ctor_var_reset_call"};
        إرجاع names[m_e];
    }
    bool isNone() ثابت { إرجاع m_e == NONE; }
};
constexpr bool operator==(ثابت VCStmtType& lhs, ثابت VCStmtType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VCStmtType& lhs, VCStmtType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, ثابت VCStmtType& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VCStmtType& rhs) {
    إرجاع os << rhs.ascii();
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
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت VL_PURE {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VCaseType& lhs, ثابت VCaseType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VCaseType& lhs, VCaseType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, ثابت VCaseType& rhs) { إرجاع lhs == rhs.m_e; }

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
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        إرجاع names[m_e];
    }
    bool isAssignable() ثابت { إرجاع m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
};
constexpr bool operator==(ثابت VCastable& lhs, ثابت VCastable& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VCastable& lhs, VCastable::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, ثابت VCastable& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VCastable& rhs) {
    إرجاع os << rhs.ascii();
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
    constexpr operator en() ثابت { إرجاع m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        إرجاع names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        إرجاع names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ثابت حرف* binSetEnum() ثابت {
        بدل (m_e) {
        حالة BINS_IGNORE: إرجاع "VlCovBinKind::KIND_IGNORE";
        حالة BINS_ILLEGAL: إرجاع "VlCovBinKind::KIND_ILLEGAL";
        حالة BINS_DEFAULT: إرجاع "VlCovBinKind::KIND_DEFAULT";
        افتراضي: إرجاع "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() ثابت {
        إرجاع m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(ثابت VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    إرجاع lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
عام:
    تعداد en : uint8_t {
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
    تعداد en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"weight",
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
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    إرجاع lhs.m_e == rhs;
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
    constexpr operator en() ثابت { إرجاع m_e; }  // LCOV_EXCL_LINE
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"", "[*]", "[->]", "[=]"};
        إرجاع names[m_e];
    }
    ثابت حرف* asciiJson() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        إرجاع names[m_e];
    }
};

//######################################################################

class VDirection final {
عام:
    تعداد en : uint8_t { NONE, إدخال, OUTPUT, INOUT, REF, CONSTREF };
    تعداد en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت VL_MT_SAFE { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        إرجاع names[m_e];
    }
    string verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        إرجاع names[m_e];
    }
    string prettyName() ثابت { إرجاع verilogKwd(); }
    bool isAny() ثابت { إرجاع m_e != NONE; }
    bool isInout() ثابت { إرجاع m_e == INOUT; }
    bool isInoutOrRef() ثابت { إرجاع m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_إدخال() ثابت { إرجاع m_e == إدخال; }
    bool isNonOutput() ثابت {
        إرجاع m_e == إدخال || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ثابت { إرجاع m_e == OUTPUT; }
    bool isReadOnly() ثابت VL_MT_SAFE { إرجاع m_e == إدخال || m_e == CONSTREF; }
    bool isWritable() ثابت VL_MT_SAFE { إرجاع m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ثابت VL_MT_SAFE { إرجاع m_e == REF; }
    bool isConstRef() ثابت VL_MT_SAFE { إرجاع m_e == CONSTREF; }
    string traceSigDirection() ثابت {
        إذا (isInout()) {
            إرجاع "VerilatedTraceSigDirection::INOUT";
        } وإلا إذا (isWritable()) {
            إرجاع "VerilatedTraceSigDirection::OUTPUT";
        } وإلا إذا (isNonOutput()) {
            إرجاع "VerilatedTraceSigDirection::INPUT";
        }
        إرجاع "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ثابت {
        بدل (m_e) {
        حالة NONE: إرجاع VAccess::NOCHANGE;
        حالة إدخال:
        حالة CONSTREF: إرجاع VAccess::READ;
        حالة OUTPUT: إرجاع VAccess::WRITE;
        حالة INOUT:
        حالة REF: إرجاع VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(ثابت VDirection& lhs, ثابت VDirection& rhs) VL_MT_SAFE {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, ثابت VDirection& rhs) VL_MT_SAFE {
    إرجاع lhs == rhs.m_e;
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VDirection& rhs) {
    إرجاع os << rhs.ascii();
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
    constexpr operator en() ثابت { إرجاع m_e; }
    bool addNewline() ثابت { إرجاع m_e != DT_WRITE; }
    bool needScopeTracking() ثابت { إرجاع m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VDisplayType& lhs, ثابت VDisplayType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VDisplayType& lhs, VDisplayType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, ثابت VDisplayType& rhs) { إرجاع lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
عام:
    تعداد en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    تعداد en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VDumpCtlType& lhs, ثابت VDumpCtlType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VDumpCtlType& lhs, VDumpCtlType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, ثابت VDumpCtlType& rhs) { إرجاع lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
عام:
    // REMEMBER to edit the strings below too
    تعداد en : uint8_t {
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
        إرجاع clocked[m_e];
    }
    bool anEdge() ثابت { إرجاع m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ثابت {
        بدل (m_e) {
        حالة ET_CHANGED: إرجاع ET_CHANGED;
        حالة ET_BOTHEDGE: إرجاع ET_BOTHEDGE;
        حالة ET_POSEDGE: إرجاع ET_NEGEDGE;
        حالة ET_NEGEDGE: إرجاع ET_POSEDGE;
        افتراضي: UASSERT_STATIC(0, "Inverting bad edgeType()"); إرجاع ET_NEGEDGE;
        }
    }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        إرجاع names[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        إرجاع names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(ثابت VEdgeType& other) ثابت {
        بدل (m_e) {
        حالة VEdgeType::ET_POSEDGE:
            إذا (other.m_e == VEdgeType::ET_NEGEDGE) إرجاع true;
            اكسر;
        حالة VEdgeType::ET_NEGEDGE:
            إذا (other.m_e == VEdgeType::ET_POSEDGE) إرجاع true;
            اكسر;
        افتراضي: اكسر;
        }
        إرجاع false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
};
constexpr bool operator==(ثابت VEdgeType& lhs, ثابت VEdgeType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VEdgeType& lhs, VEdgeType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, ثابت VEdgeType& rhs) { إرجاع lhs == rhs.m_e; }

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
عام:
    تعداد en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    تعداد en m_e;

    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت قيم[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        إرجاع قيم[m_e];
    }
    bool isIterated() ثابت {
        ساكن constexpr bool قيم[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        إرجاع قيم[m_e];
    }
    bool hasTriggers() ثابت {
        ساكن constexpr bool قيم[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        إرجاع قيم[m_e];
    }
    bool firstIteration() ثابت {
        ساكن constexpr bool قيم[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        إرجاع قيم[m_e];
    }
    bool slow() ثابت {
        ساكن constexpr bool قيم[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        إرجاع قيم[m_e];
    }

    // Short name
    std::string tag() ثابت { إرجاع VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ثابت { إرجاع "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() ثابت { إرجاع "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ثابت { إرجاع "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() ثابت { إرجاع "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(صحيح _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() ثابت { إرجاع m_e; }

خاص:
    std::string capitalizedTag() ثابت {
        ثابت std::string name = ascii();
        إرجاع name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
عام:
    تعداد en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        إرجاع names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    // Is a node type compatible with the declaration
    bool is_عقدة_Compatible(ثابت Ast_عقدة* nodep) ثابت;
};
constexpr bool operator==(ثابت VFwdType& lhs, ثابت VFwdType& rhs) { إرجاع lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VFwdType& lhs, VFwdType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, ثابت VFwdType& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VFwdType& rhs) {
    إرجاع os << rhs.ascii();
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
    bool isCached() ثابت { إرجاع m_cachedCnt == s_cachedCntGbl; }
    bool get() ثابت { إرجاع m_state; }
    فراغ set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    فراغ clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ساكن فراغ clear_Cache_شجرة() {
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
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        إرجاع names[m_e];
    }
    ثابت حرف* verilogKwd() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"join", "join_any", "join_none"};
        إرجاع names[m_e];
    }
    bool join() ثابت { إرجاع m_e == JOIN; }
    bool joinAny() ثابت { إرجاع m_e == JOIN_ANY; }
    bool joinNone() ثابت { إرجاع m_e == JOIN_NONE; }
};
constexpr bool operator==(ثابت VJoinType& lhs, ثابت VJoinType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VJoinType& lhs, VJoinType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, ثابت VJoinType& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VJoinType& rhs) {
    إرجاع os << rhs.ascii();
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
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        إرجاع names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    bool isNone() ثابت { إرجاع m_e == NONE; }
    bool isAutomatic() ثابت { إرجاع m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ثابت { إرجاع m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ثابت { إرجاع m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ثابت {
        بدل (m_e) {
        حالة AUTOMATIC_EXPLICIT: إرجاع AUTOMATIC_IMPLICIT;
        حالة STATIC_EXPLICIT: إرجاع STATIC_IMPLICIT;
        افتراضي: إرجاع m_e;
        }
    }
};
constexpr bool operator==(ثابت VLifetime& lhs, ثابت VLifetime& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VLifetime& lhs, VLifetime::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, ثابت VLifetime& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VLifetime& rhs) {
    إرجاع os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
عام:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            صحيح m_index;  // Current index
            ثابت صحيح m_inc;  // Step, +1 or -1
        عام:
            Iterator(صحيح index, صحيح inc)
                : m_index{index}
                , m_inc{inc} {}
            صحيح operator*() ثابت { إرجاع m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                إرجاع *this;
            }
            bool operator!=(ثابت Iterator& that) ثابت { إرجاع m_index != that.m_index; }
        };
        ثابت صحيح m_begin;  // First index
        ثابت صحيح m_end;  // One past the last index
        ثابت صحيح m_inc;  // Step, +1 or -1
    عام:
        Sequence(صحيح first, صحيح last, صحيح inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() ثابت { إرجاع Iterator{m_begin, m_inc}; }
        Iterator end() ثابت { إرجاع Iterator{m_end, m_inc}; }
    };

    صحيح m_يسار = 0;  // Left side of range (pre-':')
    صحيح m_يمين = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(ثابت VNumRange& rhs) ثابت {
        إرجاع m_يسار == rhs.m_يسار && m_يمين == rhs.m_يمين && m_ranged == rhs.m_ranged;
    }
    bool operator<(ثابت VNumRange& rhs) ثابت {
        إذا ((m_يسار < rhs.m_يسار)) إرجاع true;
        إذا (!(m_يسار == rhs.m_يسار)) إرجاع false;  // lhs > rhs
        إذا ((m_يمين < rhs.m_يمين)) إرجاع true;
        إذا (!(m_يمين == rhs.m_يمين)) إرجاع false;  // lhs > rhs
        إذا ((m_ranged < rhs.m_ranged)) إرجاع true;
        إذا (!(m_ranged == rhs.m_ranged)) إرجاع false;  // lhs > rhs
        إرجاع false;
    }
    //
    VNumRange() = افتراضي;
    VNumRange(صحيح hi, صحيح lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(صحيح يسار, صحيح يمين)
        : m_يسار{يسار}
        , m_يمين{يمين}
        , m_ranged{true} {}
    ~VNumRange() = افتراضي;
    // MEMBERS
    فراغ init(صحيح hi, صحيح lo, bool ascending) {
        إذا (lo > hi) std::swap(hi, lo);
        m_يسار = ascending ? lo : hi;
        m_يمين = ascending ? hi : lo;
        m_ranged = true;
    }
    صحيح يسار() ثابت { إرجاع m_يسار; }
    صحيح يمين() ثابت { إرجاع m_يمين; }
    صحيح hi() ثابت VL_MT_SAFE {
        إرجاع m_يسار > m_يمين ? m_يسار : m_يمين;
    }  // How to show a declaration
    صحيح lo() ثابت VL_MT_SAFE {
        إرجاع m_يسار > m_يمين ? m_يمين : m_يسار;
    }  // How to show a declaration
    صحيح يسار_To_يمين_Inc() ثابت { إرجاع ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_يسار_To_يمين() ثابت { إرجاع {m_يسار, m_يمين, يسار_To_يمين_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_يمين_To_يسار() ثابت { إرجاع {m_يمين, m_يسار, -يسار_To_يمين_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() ثابت { إرجاع {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() ثابت { إرجاع {hi(), lo(), -1}; }
    صحيح elements() ثابت VL_MT_SAFE { إرجاع hi() - lo() + 1; }
    bool ranged() ثابت { إرجاع m_ranged; }
    bool ascending() ثابت { إرجاع m_يسار < m_يمين; }
    صحيح hiMaxSelect() ثابت {
        إرجاع (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ثابت {
        إذا (ranged()) {
            إرجاع "["s + std::to_string(يسار()) + ":" + std::to_string(يمين()) + "]";
        } وإلا {
            إرجاع "[norg]";
        }
    }
    فراغ dump(std::ostream& str) ثابت { str << ascii(); }
};
مضمن std::ostream& operator<<(std::ostream& os, ثابت VNumRange& rhs) {
    rhs.dump(os);
    إرجاع os;
}

// ######################################################################

class VPragmaType final {
عام:
    تعداد en : uint8_t {
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
    تعداد en m_e;
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {
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
        إرجاع names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
};
constexpr bool operator==(ثابت VPragmaType& lhs, ثابت VPragmaType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VPragmaType& lhs, VPragmaType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, ثابت VPragmaType& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VPragmaType& rhs) {
    إرجاع os << rhs.ascii();
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
        ساكن constexpr ثابت حرف* ثابت names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        إرجاع names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() ثابت { إرجاع m_e; }
    bool isRandomizable() ثابت { إرجاع m_e != NONE; }
    bool isRand() ثابت { إرجاع m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ثابت { إرجاع m_e == RAND_CYCLIC; }
};
مضمن std::ostream& operator<<(std::ostream& os, ثابت VRandAttr& rhs) {
    إرجاع os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ساكن ثابت std::shared_ptr<ثابت string> s_emptyp;  // Holds ""
    ساكن ثابت std::shared_ptr<ثابت string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ثابت string> m_strp;

عام:
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
    bool isEmpty() ثابت { إرجاع m_strp == s_emptyp; }
    bool isVlSym() ثابت { إرجاع m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() ثابت { إرجاع m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ثابت;
    ساكن string replaceThis(bool useSelfForThis, ثابت string& text);
    ثابت std::string& asString() ثابت { إرجاع *m_strp; }
    bool operator==(ثابت VSelfPointerText& other) ثابت { إرجاع *m_strp == *other.m_strp; }
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
        ساكن constexpr ثابت حرف* ثابت names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        إرجاع names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    ساكن VSigning fromBool(bool isSigned) {  // Factory method
        إرجاع isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    bool isSigned() ثابت VL_MT_SAFE { إرجاع m_e == SIGNED; }
    bool isNosign() ثابت VL_MT_SAFE { إرجاع m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(ثابت VSigning& lhs, ثابت VSigning& rhs) { إرجاع lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VSigning& lhs, VSigning::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, ثابت VSigning& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VSigning& rhs) {
    إرجاع os << rhs.ascii();
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
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VStrength& lhs, ثابت VStrength& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VStrength& lhs, VStrength::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, ثابت VStrength& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VStrength& rhs) {
    إرجاع os << rhs.ascii();
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
        ساكن constexpr ثابت حرف* ثابت names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        إرجاع names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() ثابت { إرجاع m_e; }
};

//######################################################################

class VTracePrefixType final {
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
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VTracePrefixType& lhs, ثابت VTracePrefixType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, ثابت VTracePrefixType& rhs) {
    إرجاع lhs == rhs.m_e;
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VTracePrefixType& rhs) {
    إرجاع os << rhs.ascii();
}

//######################################################################

class VTraceType final {
عام:
    تعداد en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
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
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"CONSTANT", "FULL", "CHANGE"};
        إرجاع names[m_e];
    }
    ثابت حرف* func_prefix() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"trace_const", "trace_full", "trace_chg"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VTraceType& lhs, ثابت VTraceType& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VTraceType& lhs, VTraceType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, ثابت VTraceType& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VTraceType& rhs) {
    إرجاع os << rhs.ascii();
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
    constexpr operator en() ثابت { إرجاع m_e; }
    bool containsAny(VUseType other) ثابت { إرجاع m_e & other.m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        إرجاع names[m_e - 1];
    }
};
constexpr bool operator==(ثابت VUseType& lhs, ثابت VUseType& rhs) { إرجاع lhs.m_e == rhs.m_e; }
constexpr bool operator==(ثابت VUseType& lhs, VUseType::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, ثابت VUseType& rhs) { إرجاع lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    إرجاع VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    إرجاع VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VUseType& rhs) {
    إرجاع os << rhs.ascii();
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
        PORT,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    تعداد en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن constexpr ثابت حرف* ثابت names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        إرجاع names[m_e];
    }
    bool isParam() ثابت { إرجاع m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ثابت {
        إرجاع (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ثابت {
        إرجاع (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ثابت { إرجاع (m_e == TRIOR); }
    bool isWiredNet() ثابت { إرجاع (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ثابت {  // In Verilog, always ok in SystemVerilog
        إرجاع (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ثابت {
        إرجاع (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() ثابت {
        إرجاع (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ثابت {
        إرجاع (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ثابت حرف* traceSigKind() ثابت {
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
        إرجاع lut[m_e];
    }
};
constexpr bool operator==(ثابت VVarType& lhs, ثابت VVarType& rhs) VL_MT_SAFE {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, ثابت VVarType& rhs) VL_MT_SAFE {
    إرجاع lhs == rhs.m_e;
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VVarType& rhs) VL_MT_SAFE {
    إرجاع os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_مفتاح final {
عام:
    ثابت صحيح m_width;  // From AstNodeDType: Bit width of operation
    ثابت صحيح m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ثابت VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ثابت VSigning m_numeric;  // From AstNodeDType: Node is signed
    ثابت VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(ثابت VBasic_Type_مفتاح& rhs) ثابت {
        إرجاع m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(ثابت VBasic_Type_مفتاح& rhs) ثابت {
        إذا ((m_width < rhs.m_width)) إرجاع true;
        إذا (!(m_width == rhs.m_width)) إرجاع false;  // lhs > rhs
        إذا ((m_widthMin < rhs.m_widthMin)) إرجاع true;
        إذا (!(m_widthMin == rhs.m_widthMin)) إرجاع false;  // lhs > rhs
        إذا ((m_numeric < rhs.m_numeric)) إرجاع true;
        إذا (!(m_numeric == rhs.m_numeric)) إرجاع false;  // lhs > rhs
        إذا ((m_keyword < rhs.m_keyword)) إرجاع true;
        إذا (!(m_keyword == rhs.m_keyword)) إرجاع false;  // lhs > rhs
        إذا ((m_nrange < rhs.m_nrange)) إرجاع true;
        إذا (!(m_nrange == rhs.m_nrange)) إرجاع false;  // lhs > rhs
        إرجاع false;
    }
    VBasic_Type_مفتاح(صحيح width, صحيح widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  ثابت VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_مفتاح() = افتراضي;
};

#endif  // Guard
