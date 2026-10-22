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
#অন্তর্ভাৱ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
সার্ৱজনিক:
    // IEEE 1800-2023 16.12.14 property abort operators.
    গণনা en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    গণনা en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        ঘূৰাই names[m_e];
    }
    bool isAccept() ধ্রুৱ { ঘূৰাই m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"RD", "WR", "RW", "--"};
        ঘূৰাই names[m_e];
    }
    ধ্রুৱ ৱর্ণ* arrow() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        ঘূৰাই names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    VAccess invert() ধ্রুৱ {
        ঘূৰাই (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ধ্রুৱ { ঘূৰাই m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ধ্রুৱ { ঘূৰাই m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ধ্রুৱ { ঘূৰাই m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ধ্রুৱ { ঘূৰাই m_e == WRITE || m_e == READWRITE; }
    bool isRW() ধ্রুৱ { ঘূৰাই m_e == READWRITE; }
};
constexpr bool operator==(ধ্রুৱ VAccess& lhs, ধ্রুৱ VAccess& rhs) { ঘূৰাই lhs.m_e == rhs.m_e; }
constexpr bool operator==(ধ্রুৱ VAccess& lhs, VAccess::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, ধ্রুৱ VAccess& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VAccess& rhs) { ঘূৰাই os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
সার্ৱজনিক:
    গণনা en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    গণনা en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VAlwaysKwd& lhs, ধ্রুৱ VAlwaysKwd& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, ধ্রুৱ VAlwaysKwd& rhs) { ঘূৰাই lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
সার্ৱজনিক:
    // IEEE 1800-2023 Table 20-5
    গণনা en : uint8_t {
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
    গণনা en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        // IEEE 1800-2023 20.11
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"",
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
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VAssertCtlType& lhs, ধ্রুৱ VAssertCtlType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    ঘূৰাই lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, ধ্রুৱ VAssertCtlType& rhs) {
    ঘূৰাই lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
সার্ৱজনিক:
    // IEEE 1800-2023 Table 20-7
    গণনা en : uint8_t {
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
    গণনা en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ধ্রুৱ {
        std::stringstream types;
        যদি (m_e == INTERNAL)
            types << "INTERNAL ";
        অন্যথা {
            যদি (m_e & ASSERT) types << "ASSERT ";
            যদি (m_e & COVER) types << "COVER ";
            যদি (m_e & ASSUME) types << "ASSUME ";
            যদি (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            যদি (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            যদি (m_e & INTRINSIC) types << "INTRINSIC ";
            যদি (m_e & RESTRICT) types << "RESTRICT ";
        }
        ধ্রুৱ string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        ঘূৰাই str.substr(0, str.size() - 1);
    }
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
};
constexpr bool operator==(ধ্রুৱ VAssertDirectiveType& lhs, ধ্রুৱ VAssertDirectiveType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    ঘূৰাই lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, ধ্রুৱ VAssertDirectiveType& rhs) {
    ঘূৰাই lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    ঘূৰাই VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
সার্ৱজনিক:
    // IEEE 1800-2023 Table 20-6
    গণনা en : uint8_t {
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
    গণনা en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) ধ্রুৱ { ঘূৰাই m_e & other.m_e; }
    string ascii() ধ্রুৱ {
        std::stringstream types;
        যদি (m_e == INTERNAL)
            types << "INTERNAL ";
        অন্যথা {
            যদি (m_e & CONCURRENT) types << "CONCURRENT ";
            যদি (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            যদি (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            যদি (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            যদি (m_e & EXPECT) types << "EXPECT ";
            যদি (m_e & UNIQUE) types << "UNIQUE ";
            যদি (m_e & UNIQUE0) types << "UNIQUE0 ";
            যদি (m_e & PRIORITY) types << "PRIORITY ";
        }
        ধ্রুৱ string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        ঘূৰাই str.substr(0, str.size() - 1);
    }
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
};
constexpr bool operator==(ধ্রুৱ VAssertType& lhs, ধ্রুৱ VAssertType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VAssertType& lhs, VAssertType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, ধ্রুৱ VAssertType& rhs) { ঘূৰাই lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    ঘূৰাই VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    গণনা en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"default", "weak", "strong"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VPropStrength& lhs, ধ্রুৱ VPropStrength& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VPropStrength& lhs, VPropStrength::en rhs) {
    ঘূৰাই lhs.m_e == rhs;
}
constexpr bool operator!=(ধ্রুৱ VPropStrength& lhs, VPropStrength::en rhs) {
    ঘূৰাই lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
সার্ৱজনিক:
    // clang-format off
    গণনা en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_সংখ্যা,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_উপৰেৰ,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_বাম,                       // V3Width processes
        DIM_নিচেৰ,                        // V3Width processes
        DIM_ডান,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_অঙ্ক,                       // V3Width processes
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
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        // clang-format off
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {
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
        ঘূৰাই names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ধ্রুৱ {
        ৱিকল্প (m_e) {
        পক্ষ DIM_BITS:
        পক্ষ DIM_BITS_OR_সংখ্যা:
        পক্ষ DIM_DIMENSIONS:
        পক্ষ DIM_উপৰেৰ:
        পক্ষ DIM_INCREMENT:
        পক্ষ DIM_বাম:
        পক্ষ DIM_নিচেৰ:
        পক্ষ DIM_ডান:
        পক্ষ DIM_SIZE:
        পক্ষ DIM_UNPK_DIMENSIONS:
        পক্ষ TYPEID:
        পক্ষ TYPENAME: ঘূৰাই true;
        স্ৱভাৱ: ঘূৰাই false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
};
constexpr bool operator==(ধ্রুৱ VAttrType& lhs, ধ্রুৱ VAttrType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VAttrType& lhs, VAttrType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, ধ্রুৱ VAttrType& rhs) { ঘূৰাই lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

সার্ৱজনিক:
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
    শূন্য combine(ধ্রুৱ VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ধ্রুৱ { ঘূৰাই m_extends | m_final | m_initial; }
    bool isExtends() ধ্রুৱ { ঘূৰাই m_extends; }
    bool isFinal() ধ্রুৱ { ঘূৰাই m_final; }
    bool isInitial() ধ্রুৱ { ঘূৰাই m_initial; }
    string ascii() ধ্রুৱ {
        string out;
        যদি (m_initial) out = VString::dot(out, " ", "initial");
        যদি (m_extends) out = VString::dot(out, " ", "extends");
        যদি (m_final) out = VString::dot(out, " ", "final");
        ঘূৰাই out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
সার্ৱজনিক:
    গণনা en : uint8_t {
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
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ VL_MT_SAFE {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"%E-unk",
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
        ঘূৰাই names[m_e];
    }
    ধ্রুৱ ৱর্ণ* typeName() ধ্রুৱ VL_MT_SAFE {
        ৱিকল্প (m_e) {
        পক্ষ LOGIC2STATE: ঘূৰাই VBasicDTypeKwd{LOGIC}.ascii();
        পক্ষ INTEGER2STATE: ঘূৰাই VBasicDTypeKwd{INTEGER}.ascii();
        পক্ষ TIME2STATE: ঘূৰাই VBasicDTypeKwd{TIME}.ascii();
        স্ৱভাৱ: ঘূৰাই ascii();
        }
    }
    ধ্রুৱ ৱর্ণ* dpiType() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"%E-unk",
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
        ঘূৰাই names[m_e];
    }
    স্থির শূন্য selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(পূর্ণাঙ্ক _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    পূর্ণাঙ্ক width() ধ্রুৱ {
        ৱিকল্প (m_e) {
        পক্ষ BIT: ঘূৰাই 1;  // scalar, can't bit extract unless ranged
        পক্ষ BYTE: ঘূৰাই 8;
        পক্ষ CHANDLE: ঘূৰাই 64;
        পক্ষ NULLTYPE: ঘূৰাই 1;  // Historical - bit was used instead of NULLTYPE
        পক্ষ EVENT: ঘূৰাই 1;
        পক্ষ INT: ঘূৰাই 32;
        পক্ষ INTEGER: ঘূৰাই 32;
        পক্ষ INTEGER2STATE: ঘূৰাই 32;
        পক্ষ LOGIC: ঘূৰাই 1;  // scalar, can't bit extract unless ranged
        পক্ষ LOGIC2STATE: ঘূৰাই 1;  // scalar, can't bit extract unless ranged
        পক্ষ LONGINT: ঘূৰাই 64;
        পক্ষ DOUBLE: ঘূৰাই 64;  // opaque
        পক্ষ SHORTINT: ঘূৰাই 16;
        পক্ষ TIME: ঘূৰাই 64;
        পক্ষ TIME2STATE: ঘূৰাই 64;
        পক্ষ STRING: ঘূৰাই 64;  // opaque  // Just the pointer, for today
        পক্ষ SCOPEPTR: ঘূৰাই 0;  // opaque
        পক্ষ CHARPTR: ঘূৰাই 0;  // opaque
        পক্ষ MTASKSTATE: ঘূৰাই 0;  // opaque
        পক্ষ DELAY_SCHEDULER: ঘূৰাই 0;  // opaque
        পক্ষ TRIGGER_SCHEDULER: ঘূৰাই 0;  // opaque
        পক্ষ DYNAMIC_TRIGGER_SCHEDULER: ঘূৰাই 0;  // opaque
        পক্ষ FORK_SYNC: ঘূৰাই 0;  // opaque
        পক্ষ PROCESS_REFERENCE: ঘূৰাই 0;  // opaque
        পক্ষ RANDOM_GENERATOR: ঘূৰাই 0;  // opaque
        পক্ষ RANDOM_STDGENERATOR: ঘূৰাই 0;  // opaque
        পক্ষ COVERGROUP_INSTHANDLE: ঘূৰাই 0;  // opaque
        পক্ষ UINT32: ঘূৰাই 32;
        পক্ষ UINT64: ঘূৰাই 64;
        স্ৱভাৱ: ঘূৰাই 0;
        }
    }
    bool isSigned() ধ্রুৱ {
        ঘূৰাই m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ধ্রুৱ {
        ঘূৰাই m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() ধ্রুৱ {
        ঘূৰাই m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() ধ্রুৱ {  // Otherwise initializes to X
        ঘূৰাই (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ধ্রুৱ {  // Enum increment supported
        ঘূৰাই (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() ধ্রুৱ { ঘূৰাই m_e == BIT; }
    bool isBitLogic() ধ্রুৱ {  // Bit/logic vector types; can form a packed array
        ঘূৰাই (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() ধ্রুৱ {  // Can add "unsigned" to DPI
        ঘূৰাই (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ধ্রুৱ {  // Uses standard C layout, for DPI runtime access
        ঘূৰাই (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ধ্রুৱ VL_MT_SAFE {  // IE not a simple number we can bit optimize
        ঘূৰাই (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == CHANDLE; }
    bool isDouble() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == DOUBLE; }
    bool isEvent() ধ্রুৱ { ঘূৰাই m_e == EVENT; }
    bool isString() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == STRING; }
    bool isMTaskState() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == MTASKSTATE; }
    bool isSameish(ধ্রুৱ VBasicDTypeKwd& other) ধ্রুৱ VL_MT_SAFE {
        ৱিকল্প (other.m_e) {
        পক্ষ LOGIC:
        পক্ষ LOGIC2STATE:
        পক্ষ LOGIC_IMPLICIT: ঘূৰাই m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        পক্ষ INTEGER:
        পক্ষ INTEGER2STATE: ঘূৰাই m_e == INTEGER || m_e == INTEGER2STATE;
        পক্ষ TIME:
        পক্ষ TIME2STATE: ঘূৰাই m_e == TIME || m_e == TIME2STATE;
        স্ৱভাৱ: ঘূৰাই m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ধ্রুৱ VL_MT_SAFE {
        ৱিকল্প (m_e) {
        পক্ষ BIT:
        পক্ষ BYTE:
        পক্ষ CHANDLE:
        পক্ষ INT:
        পক্ষ INTEGER:
        পক্ষ INTEGER2STATE:
        পক্ষ LOGIC:
        পক্ষ LOGIC2STATE:
        পক্ষ LONGINT:
        পক্ষ DOUBLE:
        পক্ষ SHORTINT:
        পক্ষ SCOPEPTR:
        পক্ষ CHARPTR:
        পক্ষ UINT32:
        পক্ষ UINT64: ঘূৰাই true;
        স্ৱভাৱ: ঘূৰাই false;
        }
    }

    ধ্রুৱ ৱর্ণ* traceSigType() ধ্রুৱ {
        // VerilatedTraceSigType to used in trace signal declaration
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ lut[] = {
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
        static_assert(পরিমাণ(lut) / পরিমাণ(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        ঘূৰাই lut[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VBasicDTypeKwd& lhs, ধ্রুৱ VBasicDTypeKwd& rhs) VL_MT_SAFE {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    ঘূৰাই lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, ধ্রুৱ VBasicDTypeKwd& rhs) VL_MT_SAFE {
    ঘূৰাই lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
সার্ৱজনিক:
    গণনা en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    গণনা en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"FALSE", "TRUE", "UNK"};
        ঘূৰাই names[m_e];
    }
    bool isKnown() ধ্রুৱ { ঘূৰাই m_e != BU_UNKNOWN; }
    bool trueKnown() ধ্রুৱ { ঘূৰাই m_e == BU_TRUE; }
    শূন্য setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(ধ্রুৱ VBoolOrUnknown& lhs, ধ্রুৱ VBoolOrUnknown& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    ঘূৰাই lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, ধ্রুৱ VBoolOrUnknown& rhs) {
    ঘূৰাই lhs == rhs.m_e;
}
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VBoolOrUnknown& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
সার্ৱজনিক:
    গণনা en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    গণনা en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    bool unknown() ধ্রুৱ { ঘূৰাই m_e == BP_UNKNOWN; }
    bool likely() ধ্রুৱ { ঘূৰাই m_e == BP_LIKELY; }
    bool unlikely() ধ্রুৱ { ঘূৰাই m_e == BP_UNLIKELY; }
    VBranchPred invert() ধ্রুৱ {
        যদি (m_e == BP_UNLIKELY) ঘূৰাই BP_LIKELY;
        যদি (m_e == BP_LIKELY) ঘূৰাই BP_UNLIKELY;
        ঘূৰাই m_e;
    }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        ঘূৰাই names[m_e];
    }
    ধ্রুৱ ৱর্ণ* asciiShort() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"", "L", "!L"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VBranchPred& lhs, ধ্রুৱ VBranchPred& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VBranchPred& lhs, VBranchPred::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, ধ্রুৱ VBranchPred& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VBranchPred& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
স্থির constexpr bool validateArgsDescriptor(ধ্রুৱ ৱর্ণ* descrp) {
    // Is "TODO"
    যদি (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        ঘূৰাই true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    বাবে (ধ্রুৱ ৱর্ণ* cp = descrp; *cp; ++cp) {
        যদি (*cp == '+') ঘূৰাই cp != descrp && !cp[1];
        যদি (*cp != 'r' && *cp != 'w' && *cp != 'm') ঘূৰাই false;
    }
    ঘূৰাই true;
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
    macro(ARRAY_বেৰ_কৰো_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_বেৰ_কৰো_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ঠেলো_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ঠেলো_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_মান,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_মান_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_মান_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_মান_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_মান_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_মান_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_মান_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_মান_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_মান_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_মান_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_মান_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_ঢোকাও,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_বেৰ_কৰো,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_বেৰ_কৰো_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ঠেলো,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ঠেলো_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    স্থির constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    স্থির constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

সার্ৱজনিক:
    গণনা en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    গণনা en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ VL_PURE {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ মানসমূহ[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    bool isPure() ধ্রুৱ VL_PURE {
        স্থির constexpr bool মানসমূহ[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    bool isClean() ধ্রুৱ VL_PURE {
        স্থির constexpr bool মানসমূহ[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    ধ্রুৱ ৱর্ণ* args() ধ্রুৱ VL_PURE {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ মানসমূহ[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    // Return array method for given name
    স্থির VCMethod arrayMethod(ধ্রুৱ string& name);
};
constexpr bool operator==(ধ্রুৱ VCMethod& lhs, ধ্রুৱ VCMethod& rhs) { ঘূৰাই lhs.m_e == rhs.m_e; }
constexpr bool operator==(ধ্রুৱ VCMethod& lhs, VCMethod::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, ধ্রুৱ VCMethod& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VCMethod& rhs) {
    ঘূৰাই os << rhs.ascii();
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
    স্থির constexpr bool PURE = true;  // For macro expansion of 'pure' field only

সার্ৱজনিক:
    গণনা en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    গণনা en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ VL_PURE {
        স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ মানসমূহ[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    bool isPure() ধ্রুৱ VL_PURE {
        স্থির ধ্রুৱ bool মানসমূহ[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    ধ্রুৱ ৱর্ণ* args() ধ্রুৱ VL_PURE {
        স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ মানসমূহ[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        ঘূৰাই মানসমূহ[m_e];
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
সার্ৱজনিক:
    গণনা en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

গুপ্ত:
    সংরচনা Item final {
        গণনা en m_e;  // Statement's enum mnemonic, for checking
        ধ্রুৱ ৱর্ণ* m_name;  // Statements name, for debugging
    };
    স্থির Item s_itemData[];

সার্ৱজনিক:
    গণনা en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ VL_PURE {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"none", "ctor_var_reset_call"};
        ঘূৰাই names[m_e];
    }
    bool isNone() ধ্রুৱ { ঘূৰাই m_e == NONE; }
};
constexpr bool operator==(ধ্রুৱ VCStmtType& lhs, ধ্রুৱ VCStmtType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VCStmtType& lhs, VCStmtType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, ধ্রুৱ VCStmtType& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VCStmtType& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    গণনা en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ VL_PURE {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VCaseType& lhs, ধ্রুৱ VCaseType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VCaseType& lhs, VCaseType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, ধ্রুৱ VCaseType& rhs) { ঘূৰাই lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        ঘূৰাই names[m_e];
    }
    bool isAssignable() ধ্রুৱ { ঘূৰাই m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
};
constexpr bool operator==(ধ্রুৱ VCastable& lhs, ধ্রুৱ VCastable& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VCastable& lhs, VCastable::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, ধ্রুৱ VCastable& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VCastable& rhs) {
    ঘূৰাই os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
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
    গণনা en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }  // LCOV_EXCL_LINE
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        ঘূৰাই names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ধ্রুৱ ৱর্ণ* verilogKwd() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        ঘূৰাই names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ধ্রুৱ ৱর্ণ* binSetEnum() ধ্রুৱ {
        ৱিকল্প (m_e) {
        পক্ষ BINS_IGNORE: ঘূৰাই "VlCovBinKind::KIND_IGNORE";
        পক্ষ BINS_ILLEGAL: ঘূৰাই "VlCovBinKind::KIND_ILLEGAL";
        পক্ষ BINS_DEFAULT: ঘূৰাই "VlCovBinKind::KIND_DEFAULT";
        স্ৱভাৱ: ঘূৰাই "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() ধ্রুৱ {
        ঘূৰাই m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(ধ্রুৱ VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    ঘূৰাই lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        // Shared by option.* and type_option.*
        WEIGHT,
        GOAL,
        AT_LEAST,
        AUTO_BIN_MAX,
        PER_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_অঙ্ক_PRINT_MISSING,
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
    গণনা en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"weight",
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
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    ঘূৰাই lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    গণনা en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(পূর্ণাঙ্ক _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }  // LCOV_EXCL_LINE
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"", "[*]", "[->]", "[=]"};
        ঘূৰাই names[m_e];
    }
    ধ্রুৱ ৱর্ণ* asciiJson() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        ঘূৰাই names[m_e];
    }
};

//######################################################################

class VDirection final {
সার্ৱজনিক:
    গণনা en : uint8_t { NONE, প্ৰবেশ, OUTPUT, INOUT, REF, CONSTREF };
    গণনা en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        ঘূৰাই names[m_e];
    }
    string verilogKwd() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        ঘূৰাই names[m_e];
    }
    string prettyName() ধ্রুৱ { ঘূৰাই verilogKwd(); }
    bool isAny() ধ্রুৱ { ঘূৰাই m_e != NONE; }
    bool isInout() ধ্রুৱ { ঘূৰাই m_e == INOUT; }
    bool isInoutOrRef() ধ্রুৱ { ঘূৰাই m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_প্ৰবেশ() ধ্রুৱ { ঘূৰাই m_e == প্ৰবেশ; }
    bool isNonOutput() ধ্রুৱ {
        ঘূৰাই m_e == প্ৰবেশ || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ধ্রুৱ { ঘূৰাই m_e == OUTPUT; }
    bool isReadOnly() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == প্ৰবেশ || m_e == CONSTREF; }
    bool isWritable() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == REF; }
    bool isConstRef() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == CONSTREF; }
    string traceSigDirection() ধ্রুৱ {
        যদি (isInout()) {
            ঘূৰাই "VerilatedTraceSigDirection::INOUT";
        } অন্যথা যদি (isWritable()) {
            ঘূৰাই "VerilatedTraceSigDirection::OUTPUT";
        } অন্যথা যদি (isNonOutput()) {
            ঘূৰাই "VerilatedTraceSigDirection::INPUT";
        }
        ঘূৰাই "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ধ্রুৱ {
        ৱিকল্প (m_e) {
        পক্ষ NONE: ঘূৰাই VAccess::NOCHANGE;
        পক্ষ প্ৰবেশ:
        পক্ষ CONSTREF: ঘূৰাই VAccess::READ;
        পক্ষ OUTPUT: ঘূৰাই VAccess::WRITE;
        পক্ষ INOUT:
        পক্ষ REF: ঘূৰাই VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(ধ্রুৱ VDirection& lhs, ধ্রুৱ VDirection& rhs) VL_MT_SAFE {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    ঘূৰাই lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, ধ্রুৱ VDirection& rhs) VL_MT_SAFE {
    ঘূৰাই lhs == rhs.m_e;
}
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VDirection& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    গণনা en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    bool addNewline() ধ্রুৱ { ঘূৰাই m_e != DT_WRITE; }
    bool needScopeTracking() ধ্রুৱ { ঘূৰাই m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VDisplayType& lhs, ধ্রুৱ VDisplayType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VDisplayType& lhs, VDisplayType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, ধ্রুৱ VDisplayType& rhs) { ঘূৰাই lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
সার্ৱজনিক:
    গণনা en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    গণনা en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VDumpCtlType& lhs, ধ্রুৱ VDumpCtlType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VDumpCtlType& lhs, VDumpCtlType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, ধ্রুৱ VDumpCtlType& rhs) { ঘূৰাই lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
সার্ৱজনিক:
    // REMEMBER to edit the strings below too
    গণনা en : uint8_t {
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
    গণনা en m_e;
    bool clockedStmt() ধ্রুৱ {
        স্থির constexpr bool clocked[] = {
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
        ঘূৰাই clocked[m_e];
    }
    bool anEdge() ধ্রুৱ { ঘূৰাই m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ধ্রুৱ {
        ৱিকল্প (m_e) {
        পক্ষ ET_CHANGED: ঘূৰাই ET_CHANGED;
        পক্ষ ET_BOTHEDGE: ঘূৰাই ET_BOTHEDGE;
        পক্ষ ET_POSEDGE: ঘূৰাই ET_NEGEDGE;
        পক্ষ ET_NEGEDGE: ঘূৰাই ET_POSEDGE;
        স্ৱভাৱ: UASSERT_STATIC(0, "Inverting bad edgeType()"); ঘূৰাই ET_NEGEDGE;
        }
    }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        ঘূৰাই names[m_e];
    }
    ধ্রুৱ ৱর্ণ* verilogKwd() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        ঘূৰাই names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(ধ্রুৱ VEdgeType& other) ধ্রুৱ {
        ৱিকল্প (m_e) {
        পক্ষ VEdgeType::ET_POSEDGE:
            যদি (other.m_e == VEdgeType::ET_NEGEDGE) ঘূৰাই true;
            ভঙ্গ;
        পক্ষ VEdgeType::ET_NEGEDGE:
            যদি (other.m_e == VEdgeType::ET_POSEDGE) ঘূৰাই true;
            ভঙ্গ;
        স্ৱভাৱ: ভঙ্গ;
        }
        ঘূৰাই false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
};
constexpr bool operator==(ধ্রুৱ VEdgeType& lhs, ধ্রুৱ VEdgeType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VEdgeType& lhs, VEdgeType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, ধ্রুৱ VEdgeType& rhs) { ঘূৰাই lhs == rhs.m_e; }

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
সার্ৱজনিক:
    গণনা en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    গণনা en m_e;

    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ মানসমূহ[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    bool isIterated() ধ্রুৱ {
        স্থির constexpr bool মানসমূহ[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    bool hasTriggers() ধ্রুৱ {
        স্থির constexpr bool মানসমূহ[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    bool firstIteration() ধ্রুৱ {
        স্থির constexpr bool মানসমূহ[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }
    bool slow() ধ্রুৱ {
        স্থির constexpr bool মানসমূহ[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        ঘূৰাই মানসমূহ[m_e];
    }

    // Short name
    std::string tag() ধ্রুৱ { ঘূৰাই VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ধ্রুৱ { ঘূৰাই "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() ধ্রুৱ { ঘূৰাই "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ধ্রুৱ { ঘূৰাই "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() ধ্রুৱ { ঘূৰাই "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() ধ্রুৱ { ঘূৰাই m_e; }

গুপ্ত:
    std::string capitalizedTag() ধ্রুৱ {
        ধ্রুৱ std::string name = ascii();
        ঘূৰাই name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
সার্ৱজনিক:
    গণনা en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        ঘূৰাই names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    // Is a node type compatible with the declaration
    bool is_গ্ৰন্থি_Compatible(ধ্রুৱ Ast_গ্ৰন্থি* nodep) ধ্রুৱ;
};
constexpr bool operator==(ধ্রুৱ VFwdType& lhs, ধ্রুৱ VFwdType& rhs) { ঘূৰাই lhs.m_e == rhs.m_e; }
constexpr bool operator==(ধ্রুৱ VFwdType& lhs, VFwdType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, ধ্রুৱ VFwdType& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VFwdType& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    স্থির uint64_t s_cachedCntGbl;  // Global computed count

সার্ৱজনিক:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() ধ্রুৱ { ঘূৰাই m_cachedCnt == s_cachedCntGbl; }
    bool get() ধ্রুৱ { ঘূৰাই m_state; }
    শূন্য set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    শূন্য clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    স্থির শূন্য clear_Cache_বৃক্ষ() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
সার্ৱজনিক:
    গণনা en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    গণনা en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        ঘূৰাই names[m_e];
    }
    ধ্রুৱ ৱর্ণ* verilogKwd() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"join", "join_any", "join_none"};
        ঘূৰাই names[m_e];
    }
    bool join() ধ্রুৱ { ঘূৰাই m_e == JOIN; }
    bool joinAny() ধ্রুৱ { ঘূৰাই m_e == JOIN_ANY; }
    bool joinNone() ধ্রুৱ { ঘূৰাই m_e == JOIN_NONE; }
};
constexpr bool operator==(ধ্রুৱ VJoinType& lhs, ধ্রুৱ VJoinType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VJoinType& lhs, VJoinType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, ধ্রুৱ VJoinType& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VJoinType& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        ঘূৰাই names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    bool isNone() ধ্রুৱ { ঘূৰাই m_e == NONE; }
    bool isAutomatic() ধ্রুৱ { ঘূৰাই m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ধ্রুৱ { ঘূৰাই m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ধ্রুৱ { ঘূৰাই m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ধ্রুৱ {
        ৱিকল্প (m_e) {
        পক্ষ AUTOMATIC_EXPLICIT: ঘূৰাই AUTOMATIC_IMPLICIT;
        পক্ষ STATIC_EXPLICIT: ঘূৰাই STATIC_IMPLICIT;
        স্ৱভাৱ: ঘূৰাই m_e;
        }
    }
};
constexpr bool operator==(ধ্রুৱ VLifetime& lhs, ধ্রুৱ VLifetime& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VLifetime& lhs, VLifetime::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, ধ্রুৱ VLifetime& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VLifetime& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
সার্ৱজনিক:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            পূর্ণাঙ্ক m_index;  // Current index
            ধ্রুৱ পূর্ণাঙ্ক m_inc;  // Step, +1 or -1
        সার্ৱজনিক:
            Iterator(পূর্ণাঙ্ক index, পূর্ণাঙ্ক inc)
                : m_index{index}
                , m_inc{inc} {}
            পূর্ণাঙ্ক operator*() ধ্রুৱ { ঘূৰাই m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                ঘূৰাই *this;
            }
            bool operator!=(ধ্রুৱ Iterator& that) ধ্রুৱ { ঘূৰাই m_index != that.m_index; }
        };
        ধ্রুৱ পূর্ণাঙ্ক m_begin;  // First index
        ধ্রুৱ পূর্ণাঙ্ক m_end;  // One past the last index
        ধ্রুৱ পূর্ণাঙ্ক m_inc;  // Step, +1 or -1
    সার্ৱজনিক:
        Sequence(পূর্ণাঙ্ক first, পূর্ণাঙ্ক last, পূর্ণাঙ্ক inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() ধ্রুৱ { ঘূৰাই Iterator{m_begin, m_inc}; }
        Iterator end() ধ্রুৱ { ঘূৰাই Iterator{m_end, m_inc}; }
    };

    পূর্ণাঙ্ক m_বাম = 0;  // Left side of range (pre-':')
    পূর্ণাঙ্ক m_ডান = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(ধ্রুৱ VNumRange& rhs) ধ্রুৱ {
        ঘূৰাই m_বাম == rhs.m_বাম && m_ডান == rhs.m_ডান && m_ranged == rhs.m_ranged;
    }
    bool operator<(ধ্রুৱ VNumRange& rhs) ধ্রুৱ {
        যদি ((m_বাম < rhs.m_বাম)) ঘূৰাই true;
        যদি (!(m_বাম == rhs.m_বাম)) ঘূৰাই false;  // lhs > rhs
        যদি ((m_ডান < rhs.m_ডান)) ঘূৰাই true;
        যদি (!(m_ডান == rhs.m_ডান)) ঘূৰাই false;  // lhs > rhs
        যদি ((m_ranged < rhs.m_ranged)) ঘূৰাই true;
        যদি (!(m_ranged == rhs.m_ranged)) ঘূৰাই false;  // lhs > rhs
        ঘূৰাই false;
    }
    //
    VNumRange() = স্ৱভাৱ;
    VNumRange(পূর্ণাঙ্ক hi, পূর্ণাঙ্ক lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(পূর্ণাঙ্ক বাম, পূর্ণাঙ্ক ডান)
        : m_বাম{বাম}
        , m_ডান{ডান}
        , m_ranged{true} {}
    ~VNumRange() = স্ৱভাৱ;
    // MEMBERS
    শূন্য init(পূর্ণাঙ্ক hi, পূর্ণাঙ্ক lo, bool ascending) {
        যদি (lo > hi) std::swap(hi, lo);
        m_বাম = ascending ? lo : hi;
        m_ডান = ascending ? hi : lo;
        m_ranged = true;
    }
    পূর্ণাঙ্ক বাম() ধ্রুৱ { ঘূৰাই m_বাম; }
    পূর্ণাঙ্ক ডান() ধ্রুৱ { ঘূৰাই m_ডান; }
    পূর্ণাঙ্ক hi() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই m_বাম > m_ডান ? m_বাম : m_ডান;
    }  // How to show a declaration
    পূর্ণাঙ্ক lo() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই m_বাম > m_ডান ? m_ডান : m_বাম;
    }  // How to show a declaration
    পূর্ণাঙ্ক বাম_To_ডান_Inc() ধ্রুৱ { ঘূৰাই ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_বাম_To_ডান() ধ্রুৱ { ঘূৰাই {m_বাম, m_ডান, বাম_To_ডান_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_ডান_To_বাম() ধ্রুৱ { ঘূৰাই {m_ডান, m_বাম, -বাম_To_ডান_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() ধ্রুৱ { ঘূৰাই {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() ধ্রুৱ { ঘূৰাই {hi(), lo(), -1}; }
    পূর্ণাঙ্ক elements() ধ্রুৱ VL_MT_SAFE { ঘূৰাই hi() - lo() + 1; }
    bool ranged() ধ্রুৱ { ঘূৰাই m_ranged; }
    bool ascending() ধ্রুৱ { ঘূৰাই m_বাম < m_ডান; }
    পূর্ণাঙ্ক hiMaxSelect() ধ্রুৱ {
        ঘূৰাই (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ধ্রুৱ {
        যদি (ranged()) {
            ঘূৰাই "["s + std::to_string(বাম()) + ":" + std::to_string(ডান()) + "]";
        } অন্যথা {
            ঘূৰাই "[norg]";
        }
    }
    শূন্য dump(std::ostream& str) ধ্রুৱ { str << ascii(); }
};
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VNumRange& rhs) {
    rhs.dump(os);
    ঘূৰাই os;
}

// ######################################################################

class VPragmaType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
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
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {
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
        ঘূৰাই names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
};
constexpr bool operator==(ধ্রুৱ VPragmaType& lhs, ধ্রুৱ VPragmaType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VPragmaType& lhs, VPragmaType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, ধ্রুৱ VPragmaType& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VPragmaType& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        ঘূৰাই names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    bool isRandomizable() ধ্রুৱ { ঘূৰাই m_e != NONE; }
    bool isRand() ধ্রুৱ { ঘূৰাই m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ধ্রুৱ { ঘূৰাই m_e == RAND_CYCLIC; }
};
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VRandAttr& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    স্থির ধ্রুৱ std::shared_ptr<ধ্রুৱ string> s_emptyp;  // Holds ""
    স্থির ধ্রুৱ std::shared_ptr<ধ্রুৱ string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ধ্রুৱ string> m_strp;

সার্ৱজনিক:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, ধ্রুৱ string& field)
        : m_strp{std::make_shared<ধ্রুৱ string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, ধ্রুৱ string& field)
        : m_strp{std::make_shared<ধ্রুৱ string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() ধ্রুৱ { ঘূৰাই m_strp == s_emptyp; }
    bool isVlSym() ধ্রুৱ { ঘূৰাই m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() ধ্রুৱ { ঘূৰাই m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ধ্রুৱ;
    স্থির string replaceThis(bool useSelfForThis, ধ্রুৱ string& text);
    ধ্রুৱ std::string& asString() ধ্রুৱ { ঘূৰাই *m_strp; }
    bool operator==(ধ্রুৱ VSelfPointerText& other) ধ্রুৱ { ঘূৰাই *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        ঘূৰাই names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    স্থির VSigning fromBool(bool isSigned) {  // Factory method
        ঘূৰাই isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    bool isSigned() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == SIGNED; }
    bool isNosign() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(ধ্রুৱ VSigning& lhs, ধ্রুৱ VSigning& rhs) { ঘূৰাই lhs.m_e == rhs.m_e; }
constexpr bool operator==(ধ্রুৱ VSigning& lhs, VSigning::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, ধ্রুৱ VSigning& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VSigning& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

class VStrength final {
সার্ৱজনিক:
    গণনা en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    গণনা en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VStrength& lhs, ধ্রুৱ VStrength& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VStrength& lhs, VStrength::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, ধ্রুৱ VStrength& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VStrength& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    গণনা en m_e;
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        ঘূৰাই names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
};

//######################################################################

class VTracePrefixType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    গণনা en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VTracePrefixType& lhs, ধ্রুৱ VTracePrefixType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    ঘূৰাই lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, ধ্রুৱ VTracePrefixType& rhs) {
    ঘূৰাই lhs == rhs.m_e;
}
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VTracePrefixType& rhs) {
    ঘূৰাই os << rhs.ascii();
}

//######################################################################

class VTraceType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    গণনা en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"CONSTANT", "FULL", "CHANGE"};
        ঘূৰাই names[m_e];
    }
    ধ্রুৱ ৱর্ণ* func_prefix() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"trace_const", "trace_full", "trace_chg"};
        ঘূৰাই names[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VTraceType& lhs, ধ্রুৱ VTraceType& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VTraceType& lhs, VTraceType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, ধ্রুৱ VTraceType& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VTraceType& rhs) {
    ঘূৰাই os << rhs.ascii();
}

//######################################################################

class VUseType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    গণনা en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    bool containsAny(VUseType other) ধ্রুৱ { ঘূৰাই m_e & other.m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        ঘূৰাই names[m_e - 1];
    }
};
constexpr bool operator==(ধ্রুৱ VUseType& lhs, ধ্রুৱ VUseType& rhs) { ঘূৰাই lhs.m_e == rhs.m_e; }
constexpr bool operator==(ধ্রুৱ VUseType& lhs, VUseType::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, ধ্রুৱ VUseType& rhs) { ঘূৰাই lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    ঘূৰাই VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    ঘূৰাই VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VUseType& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

class VVarType final {
সার্ৱজনিক:
    গণনা en : uint8_t {
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
    গণনা en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ {
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        ঘূৰাই names[m_e];
    }
    bool isParam() ধ্রুৱ { ঘূৰাই m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ধ্রুৱ {
        ঘূৰাই (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ধ্রুৱ {
        ঘূৰাই (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ধ্রুৱ { ঘূৰাই (m_e == TRIOR); }
    bool isWiredNet() ধ্রুৱ { ঘূৰাই (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ধ্রুৱ {  // In Verilog, always ok in SystemVerilog
        ঘূৰাই (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ধ্রুৱ {
        ঘূৰাই (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() ধ্রুৱ {
        ঘূৰাই (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ধ্রুৱ {
        ঘূৰাই (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ধ্রুৱ ৱর্ণ* traceSigKind() ধ্রুৱ {
        // VerilatedTraceSigKind to used in trace signal declaration
        স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ lut[] = {
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
        ঘূৰাই lut[m_e];
    }
};
constexpr bool operator==(ধ্রুৱ VVarType& lhs, ধ্রুৱ VVarType& rhs) VL_MT_SAFE {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    ঘূৰাই lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, ধ্রুৱ VVarType& rhs) VL_MT_SAFE {
    ঘূৰাই lhs == rhs.m_e;
}
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ VVarType& rhs) VL_MT_SAFE {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_চাবি final {
সার্ৱজনিক:
    ধ্রুৱ পূর্ণাঙ্ক m_width;  // From AstNodeDType: Bit width of operation
    ধ্রুৱ পূর্ণাঙ্ক m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ধ্রুৱ VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ধ্রুৱ VSigning m_numeric;  // From AstNodeDType: Node is signed
    ধ্রুৱ VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(ধ্রুৱ VBasic_Type_চাবি& rhs) ধ্রুৱ {
        ঘূৰাই m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(ধ্রুৱ VBasic_Type_চাবি& rhs) ধ্রুৱ {
        যদি ((m_width < rhs.m_width)) ঘূৰাই true;
        যদি (!(m_width == rhs.m_width)) ঘূৰাই false;  // lhs > rhs
        যদি ((m_widthMin < rhs.m_widthMin)) ঘূৰাই true;
        যদি (!(m_widthMin == rhs.m_widthMin)) ঘূৰাই false;  // lhs > rhs
        যদি ((m_numeric < rhs.m_numeric)) ঘূৰাই true;
        যদি (!(m_numeric == rhs.m_numeric)) ঘূৰাই false;  // lhs > rhs
        যদি ((m_keyword < rhs.m_keyword)) ঘূৰাই true;
        যদি (!(m_keyword == rhs.m_keyword)) ঘূৰাই false;  // lhs > rhs
        যদি ((m_nrange < rhs.m_nrange)) ঘূৰাই true;
        যদি (!(m_nrange == rhs.m_nrange)) ঘূৰাই false;  // lhs > rhs
        ঘূৰাই false;
    }
    VBasic_Type_চাবি(পূর্ণাঙ্ক width, পূর্ণাঙ্ক widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  ধ্রুৱ VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_চাবি() = স্ৱভাৱ;
};

#endif  // Guard
