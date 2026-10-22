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
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
ಸಾರ್ವಜನಿಕ:
    // IEEE 1800-2023 16.12.14 property abort operators.
    ಗಣನಾ en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    ಗಣನಾ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    bool isAccept() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"RD", "WR", "RW", "--"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* arrow() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    VAccess invert() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == WRITE || m_e == READWRITE; }
    bool isRW() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == READWRITE; }
};
constexpr bool operator==(ಧ್ರುವ VAccess& lhs, ಧ್ರುವ VAccess& rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ಧ್ರುವ VAccess& lhs, VAccess::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, ಧ್ರುವ VAccess& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VAccess& rhs) { ಪ್ರತ್ಯಾಗಮ os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    ಗಣನಾ en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VAlwaysKwd& lhs, ಧ್ರುವ VAlwaysKwd& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, ಧ್ರುವ VAlwaysKwd& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
ಸಾರ್ವಜನಿಕ:
    // IEEE 1800-2023 Table 20-5
    ಗಣನಾ en : uint8_t {
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
    ಗಣನಾ en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        // IEEE 1800-2023 20.11
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"",
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
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VAssertCtlType& lhs, ಧ್ರುವ VAssertCtlType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, ಧ್ರುವ VAssertCtlType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
ಸಾರ್ವಜನಿಕ:
    // IEEE 1800-2023 Table 20-7
    ಗಣನಾ en : uint8_t {
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
    ಗಣನಾ en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ಧ್ರುವ {
        std::stringstream types;
        ಯದಿ (m_e == INTERNAL)
            types << "INTERNAL ";
        ಅನ್ಯಥಾ {
            ಯದಿ (m_e & ASSERT) types << "ASSERT ";
            ಯದಿ (m_e & COVER) types << "COVER ";
            ಯದಿ (m_e & ASSUME) types << "ASSUME ";
            ಯದಿ (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            ಯದಿ (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            ಯದಿ (m_e & INTRINSIC) types << "INTRINSIC ";
            ಯದಿ (m_e & RESTRICT) types << "RESTRICT ";
        }
        ಧ್ರುವ string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        ಪ್ರತ್ಯಾಗಮ str.substr(0, str.size() - 1);
    }
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
};
constexpr bool operator==(ಧ್ರುವ VAssertDirectiveType& lhs, ಧ್ರುವ VAssertDirectiveType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, ಧ್ರುವ VAssertDirectiveType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
ಸಾರ್ವಜನಿಕ:
    // IEEE 1800-2023 Table 20-6
    ಗಣನಾ en : uint8_t {
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
    ಗಣನಾ en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e & other.m_e; }
    string ascii() ಧ್ರುವ {
        std::stringstream types;
        ಯದಿ (m_e == INTERNAL)
            types << "INTERNAL ";
        ಅನ್ಯಥಾ {
            ಯದಿ (m_e & CONCURRENT) types << "CONCURRENT ";
            ಯದಿ (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            ಯದಿ (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            ಯದಿ (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            ಯದಿ (m_e & EXPECT) types << "EXPECT ";
            ಯದಿ (m_e & UNIQUE) types << "UNIQUE ";
            ಯದಿ (m_e & UNIQUE0) types << "UNIQUE0 ";
            ಯದಿ (m_e & PRIORITY) types << "PRIORITY ";
        }
        ಧ್ರುವ string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        ಪ್ರತ್ಯಾಗಮ str.substr(0, str.size() - 1);
    }
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
};
constexpr bool operator==(ಧ್ರುವ VAssertType& lhs, ಧ್ರುವ VAssertType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VAssertType& lhs, VAssertType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, ಧ್ರುವ VAssertType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    ಗಣನಾ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"default", "weak", "strong"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VPropStrength& lhs, ಧ್ರುವ VPropStrength& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VPropStrength& lhs, VPropStrength::en rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator!=(ಧ್ರುವ VPropStrength& lhs, VPropStrength::en rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
ಸಾರ್ವಜನಿಕ:
    // clang-format off
    ಗಣನಾ en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_ಸಂಖ್ಯೆ,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_ಮೇಲಿನ,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_ಎಡ,                       // V3Width processes
        DIM_ಕೆಳಗಿನ,                        // V3Width processes
        DIM_ಬಲ,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_ಅಂಕೆ,                       // V3Width processes
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
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        // clang-format off
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {
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
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ಧ್ರುವ {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ DIM_BITS:
        ಪಕ್ಷ DIM_BITS_OR_ಸಂಖ್ಯೆ:
        ಪಕ್ಷ DIM_DIMENSIONS:
        ಪಕ್ಷ DIM_ಮೇಲಿನ:
        ಪಕ್ಷ DIM_INCREMENT:
        ಪಕ್ಷ DIM_ಎಡ:
        ಪಕ್ಷ DIM_ಕೆಳಗಿನ:
        ಪಕ್ಷ DIM_ಬಲ:
        ಪಕ್ಷ DIM_SIZE:
        ಪಕ್ಷ DIM_UNPK_DIMENSIONS:
        ಪಕ್ಷ TYPEID:
        ಪಕ್ಷ TYPENAME: ಪ್ರತ್ಯಾಗಮ true;
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
};
constexpr bool operator==(ಧ್ರುವ VAttrType& lhs, ಧ್ರುವ VAttrType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VAttrType& lhs, VAttrType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, ಧ್ರುವ VAttrType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

ಸಾರ್ವಜನಿಕ:
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
    ಶೂನ್ಯ combine(ಧ್ರುವ VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_extends | m_final | m_initial; }
    bool isExtends() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_extends; }
    bool isFinal() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_final; }
    bool isInitial() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_initial; }
    string ascii() ಧ್ರುವ {
        string out;
        ಯದಿ (m_initial) out = VString::dot(out, " ", "initial");
        ಯದಿ (m_extends) out = VString::dot(out, " ", "extends");
        ಯದಿ (m_final) out = VString::dot(out, " ", "final");
        ಪ್ರತ್ಯಾಗಮ out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
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
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ VL_MT_SAFE {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"%E-unk",
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
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* typeName() ಧ್ರುವ VL_MT_SAFE {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ LOGIC2STATE: ಪ್ರತ್ಯಾಗಮ VBasicDTypeKwd{LOGIC}.ascii();
        ಪಕ್ಷ INTEGER2STATE: ಪ್ರತ್ಯಾಗಮ VBasicDTypeKwd{INTEGER}.ascii();
        ಪಕ್ಷ TIME2STATE: ಪ್ರತ್ಯಾಗಮ VBasicDTypeKwd{TIME}.ascii();
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ ascii();
        }
    }
    ಧ್ರುವ ವರ್ಣ* dpiType() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"%E-unk",
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
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(ಪೂರ್ಣಾಙ್ಕ _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಪೂರ್ಣಾಙ್ಕ width() ಧ್ರುವ {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ BIT: ಪ್ರತ್ಯಾಗಮ 1;  // scalar, can't bit extract unless ranged
        ಪಕ್ಷ BYTE: ಪ್ರತ್ಯಾಗಮ 8;
        ಪಕ್ಷ CHANDLE: ಪ್ರತ್ಯಾಗಮ 64;
        ಪಕ್ಷ NULLTYPE: ಪ್ರತ್ಯಾಗಮ 1;  // Historical - bit was used instead of NULLTYPE
        ಪಕ್ಷ EVENT: ಪ್ರತ್ಯಾಗಮ 1;
        ಪಕ್ಷ INT: ಪ್ರತ್ಯಾಗಮ 32;
        ಪಕ್ಷ INTEGER: ಪ್ರತ್ಯಾಗಮ 32;
        ಪಕ್ಷ INTEGER2STATE: ಪ್ರತ್ಯಾಗಮ 32;
        ಪಕ್ಷ LOGIC: ಪ್ರತ್ಯಾಗಮ 1;  // scalar, can't bit extract unless ranged
        ಪಕ್ಷ LOGIC2STATE: ಪ್ರತ್ಯಾಗಮ 1;  // scalar, can't bit extract unless ranged
        ಪಕ್ಷ LONGINT: ಪ್ರತ್ಯಾಗಮ 64;
        ಪಕ್ಷ DOUBLE: ಪ್ರತ್ಯಾಗಮ 64;  // opaque
        ಪಕ್ಷ SHORTINT: ಪ್ರತ್ಯಾಗಮ 16;
        ಪಕ್ಷ TIME: ಪ್ರತ್ಯಾಗಮ 64;
        ಪಕ್ಷ TIME2STATE: ಪ್ರತ್ಯಾಗಮ 64;
        ಪಕ್ಷ STRING: ಪ್ರತ್ಯಾಗಮ 64;  // opaque  // Just the pointer, for today
        ಪಕ್ಷ SCOPEPTR: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ CHARPTR: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ MTASKSTATE: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ DELAY_SCHEDULER: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ TRIGGER_SCHEDULER: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ DYNAMIC_TRIGGER_SCHEDULER: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ FORK_SYNC: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ PROCESS_REFERENCE: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ RANDOM_GENERATOR: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ RANDOM_STDGENERATOR: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ COVERGROUP_INSTHANDLE: ಪ್ರತ್ಯಾಗಮ 0;  // opaque
        ಪಕ್ಷ UINT32: ಪ್ರತ್ಯಾಗಮ 32;
        ಪಕ್ಷ UINT64: ಪ್ರತ್ಯಾಗಮ 64;
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ 0;
        }
    }
    bool isSigned() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() ಧ್ರುವ {  // Otherwise initializes to X
        ಪ್ರತ್ಯಾಗಮ (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ಧ್ರುವ {  // Enum increment supported
        ಪ್ರತ್ಯಾಗಮ (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == BIT; }
    bool isBitLogic() ಧ್ರುವ {  // Bit/logic vector types; can form a packed array
        ಪ್ರತ್ಯಾಗಮ (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() ಧ್ರುವ {  // Can add "unsigned" to DPI
        ಪ್ರತ್ಯಾಗಮ (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ಧ್ರುವ {  // Uses standard C layout, for DPI runtime access
        ಪ್ರತ್ಯಾಗಮ (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ಧ್ರುವ VL_MT_SAFE {  // IE not a simple number we can bit optimize
        ಪ್ರತ್ಯಾಗಮ (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == CHANDLE; }
    bool isDouble() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == DOUBLE; }
    bool isEvent() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == EVENT; }
    bool isString() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == STRING; }
    bool isMTaskState() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == MTASKSTATE; }
    bool isSameish(ಧ್ರುವ VBasicDTypeKwd& other) ಧ್ರುವ VL_MT_SAFE {
        ವಿಕಲ್ಪ (other.m_e) {
        ಪಕ್ಷ LOGIC:
        ಪಕ್ಷ LOGIC2STATE:
        ಪಕ್ಷ LOGIC_IMPLICIT: ಪ್ರತ್ಯಾಗಮ m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        ಪಕ್ಷ INTEGER:
        ಪಕ್ಷ INTEGER2STATE: ಪ್ರತ್ಯಾಗಮ m_e == INTEGER || m_e == INTEGER2STATE;
        ಪಕ್ಷ TIME:
        ಪಕ್ಷ TIME2STATE: ಪ್ರತ್ಯಾಗಮ m_e == TIME || m_e == TIME2STATE;
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ಧ್ರುವ VL_MT_SAFE {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ BIT:
        ಪಕ್ಷ BYTE:
        ಪಕ್ಷ CHANDLE:
        ಪಕ್ಷ INT:
        ಪಕ್ಷ INTEGER:
        ಪಕ್ಷ INTEGER2STATE:
        ಪಕ್ಷ LOGIC:
        ಪಕ್ಷ LOGIC2STATE:
        ಪಕ್ಷ LONGINT:
        ಪಕ್ಷ DOUBLE:
        ಪಕ್ಷ SHORTINT:
        ಪಕ್ಷ SCOPEPTR:
        ಪಕ್ಷ CHARPTR:
        ಪಕ್ಷ UINT32:
        ಪಕ್ಷ UINT64: ಪ್ರತ್ಯಾಗಮ true;
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ false;
        }
    }

    ಧ್ರುವ ವರ್ಣ* traceSigType() ಧ್ರುವ {
        // VerilatedTraceSigType to used in trace signal declaration
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ lut[] = {
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
        static_assert(ಪರಿಮಾಣ(lut) / ಪರಿಮಾಣ(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        ಪ್ರತ್ಯಾಗಮ lut[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VBasicDTypeKwd& lhs, ಧ್ರುವ VBasicDTypeKwd& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, ಧ್ರುವ VBasicDTypeKwd& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    ಗಣನಾ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"FALSE", "TRUE", "UNK"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    bool isKnown() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e != BU_UNKNOWN; }
    bool trueKnown() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == BU_TRUE; }
    ಶೂನ್ಯ setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(ಧ್ರುವ VBoolOrUnknown& lhs, ಧ್ರುವ VBoolOrUnknown& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, ಧ್ರುವ VBoolOrUnknown& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
}
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VBoolOrUnknown& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    ಗಣನಾ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    bool unknown() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == BP_UNKNOWN; }
    bool likely() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == BP_LIKELY; }
    bool unlikely() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == BP_UNLIKELY; }
    VBranchPred invert() ಧ್ರುವ {
        ಯದಿ (m_e == BP_UNLIKELY) ಪ್ರತ್ಯಾಗಮ BP_LIKELY;
        ಯದಿ (m_e == BP_LIKELY) ಪ್ರತ್ಯಾಗಮ BP_UNLIKELY;
        ಪ್ರತ್ಯಾಗಮ m_e;
    }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* asciiShort() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"", "L", "!L"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VBranchPred& lhs, ಧ್ರುವ VBranchPred& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VBranchPred& lhs, VBranchPred::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, ಧ್ರುವ VBranchPred& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VBranchPred& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ಸ್ಥಿರ constexpr bool validateArgsDescriptor(ಧ್ರುವ ವರ್ಣ* descrp) {
    // Is "TODO"
    ಯದಿ (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        ಪ್ರತ್ಯಾಗಮ true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* cp = descrp; *cp; ++cp) {
        ಯದಿ (*cp == '+') ಪ್ರತ್ಯಾಗಮ cp != descrp && !cp[1];
        ಯದಿ (*cp != 'r' && *cp != 'w' && *cp != 'm') ಪ್ರತ್ಯಾಗಮ false;
    }
    ಪ್ರತ್ಯಾಗಮ true;
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
    macro(ARRAY_ತೆಗೆ_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_ತೆಗೆ_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ತಳ್ಳು_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ತಳ್ಳು_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_ಮೌಲ್ಯ,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_ಮೌಲ್ಯ_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_ಮೌಲ್ಯ_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_ಮೌಲ್ಯ_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_ಮೌಲ್ಯ_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_ಸೇರಿಸು,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_ತೆಗೆ,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_ತೆಗೆ_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ತಳ್ಳು,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ತಳ್ಳು_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    ಸ್ಥಿರ constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    ಸ್ಥಿರ constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    ಗಣನಾ en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ ಮೌಲ್ಯಗಳು[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool isPure() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool isClean() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* args() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ ಮೌಲ್ಯಗಳು[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    // Return array method for given name
    ಸ್ಥಿರ VCMethod arrayMethod(ಧ್ರುವ string& name);
};
constexpr bool operator==(ಧ್ರುವ VCMethod& lhs, ಧ್ರುವ VCMethod& rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ಧ್ರುವ VCMethod& lhs, VCMethod::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, ಧ್ರುವ VCMethod& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VCMethod& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
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
    ಸ್ಥಿರ constexpr bool PURE = true;  // For macro expansion of 'pure' field only

ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    ಗಣನಾ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ ಮೌಲ್ಯಗಳು[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool isPure() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ ಧ್ರುವ bool ಮೌಲ್ಯಗಳು[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* args() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ ಮೌಲ್ಯಗಳು[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
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
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

ಗುಪ್ತ:
    ಸಂರಚನಾ Item final {
        ಗಣನಾ en m_e;  // Statement's enum mnemonic, for checking
        ಧ್ರುವ ವರ್ಣ* m_name;  // Statements name, for debugging
    };
    ಸ್ಥಿರ Item s_itemData[];

ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"none", "ctor_var_reset_call"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    bool isNone() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == NONE; }
};
constexpr bool operator==(ಧ್ರುವ VCStmtType& lhs, ಧ್ರುವ VCStmtType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VCStmtType& lhs, VCStmtType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, ಧ್ರುವ VCStmtType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VCStmtType& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    ಗಣನಾ en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ VL_PURE {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VCaseType& lhs, ಧ್ರುವ VCaseType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VCaseType& lhs, VCaseType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, ಧ್ರುವ VCaseType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    bool isAssignable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
};
constexpr bool operator==(ಧ್ರುವ VCastable& lhs, ಧ್ರುವ VCastable& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VCastable& lhs, VCastable::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, ಧ್ರುವ VCastable& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VCastable& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
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
    ಗಣನಾ en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }  // LCOV_EXCL_LINE
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ಧ್ರುವ ವರ್ಣ* verilogKwd() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ಧ್ರುವ ವರ್ಣ* binSetEnum() ಧ್ರುವ {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ BINS_IGNORE: ಪ್ರತ್ಯಾಗಮ "VlCovBinKind::KIND_IGNORE";
        ಪಕ್ಷ BINS_ILLEGAL: ಪ್ರತ್ಯಾಗಮ "VlCovBinKind::KIND_ILLEGAL";
        ಪಕ್ಷ BINS_DEFAULT: ಪ್ರತ್ಯಾಗಮ "VlCovBinKind::KIND_DEFAULT";
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(ಧ್ರುವ VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        // Shared by option.* and type_option.*
        WEIGHT,
        GOAL,
        AT_LEAST,
        AUTO_BIN_MAX,
        PER_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_ಅಂಕೆ_PRINT_MISSING,
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
    ಗಣನಾ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"weight",
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
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    ಗಣನಾ en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(ಪೂರ್ಣಾಙ್ಕ _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }  // LCOV_EXCL_LINE
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"", "[*]", "[->]", "[=]"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* asciiJson() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};

//######################################################################

class VDirection final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { NONE, ಒಳಹರಿವು, OUTPUT, INOUT, REF, CONSTREF };
    ಗಣನಾ en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    string verilogKwd() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    string prettyName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ verilogKwd(); }
    bool isAny() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e != NONE; }
    bool isInout() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == INOUT; }
    bool isInoutOrRef() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_ಒಳಹರಿವು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == ಒಳಹರಿವು; }
    bool isNonOutput() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_e == ಒಳಹರಿವು || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == OUTPUT; }
    bool isReadOnly() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == ಒಳಹರಿವು || m_e == CONSTREF; }
    bool isWritable() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == REF; }
    bool isConstRef() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == CONSTREF; }
    string traceSigDirection() ಧ್ರುವ {
        ಯದಿ (isInout()) {
            ಪ್ರತ್ಯಾಗಮ "VerilatedTraceSigDirection::INOUT";
        } ಅನ್ಯಥಾ ಯದಿ (isWritable()) {
            ಪ್ರತ್ಯಾಗಮ "VerilatedTraceSigDirection::OUTPUT";
        } ಅನ್ಯಥಾ ಯದಿ (isNonOutput()) {
            ಪ್ರತ್ಯಾಗಮ "VerilatedTraceSigDirection::INPUT";
        }
        ಪ್ರತ್ಯಾಗಮ "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ಧ್ರುವ {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ NONE: ಪ್ರತ್ಯಾಗಮ VAccess::NOCHANGE;
        ಪಕ್ಷ ಒಳಹರಿವು:
        ಪಕ್ಷ CONSTREF: ಪ್ರತ್ಯಾಗಮ VAccess::READ;
        ಪಕ್ಷ OUTPUT: ಪ್ರತ್ಯಾಗಮ VAccess::WRITE;
        ಪಕ್ಷ INOUT:
        ಪಕ್ಷ REF: ಪ್ರತ್ಯಾಗಮ VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(ಧ್ರುವ VDirection& lhs, ಧ್ರುವ VDirection& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, ಧ್ರುವ VDirection& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
}
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VDirection& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    ಗಣನಾ en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    bool addNewline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e != DT_WRITE; }
    bool needScopeTracking() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VDisplayType& lhs, ಧ್ರುವ VDisplayType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VDisplayType& lhs, VDisplayType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, ಧ್ರುವ VDisplayType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    ಗಣನಾ en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VDumpCtlType& lhs, ಧ್ರುವ VDumpCtlType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VDumpCtlType& lhs, VDumpCtlType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, ಧ್ರುವ VDumpCtlType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
ಸಾರ್ವಜನಿಕ:
    // REMEMBER to edit the strings below too
    ಗಣನಾ en : uint8_t {
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
    ಗಣನಾ en m_e;
    bool clockedStmt() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr bool clocked[] = {
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
        ಪ್ರತ್ಯಾಗಮ clocked[m_e];
    }
    bool anEdge() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ಧ್ರುವ {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ ET_CHANGED: ಪ್ರತ್ಯಾಗಮ ET_CHANGED;
        ಪಕ್ಷ ET_BOTHEDGE: ಪ್ರತ್ಯಾಗಮ ET_BOTHEDGE;
        ಪಕ್ಷ ET_POSEDGE: ಪ್ರತ್ಯಾಗಮ ET_NEGEDGE;
        ಪಕ್ಷ ET_NEGEDGE: ಪ್ರತ್ಯಾಗಮ ET_POSEDGE;
        ಸ್ವಭಾವ: UASSERT_STATIC(0, "Inverting bad edgeType()"); ಪ್ರತ್ಯಾಗಮ ET_NEGEDGE;
        }
    }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* verilogKwd() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(ಧ್ರುವ VEdgeType& other) ಧ್ರುವ {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ VEdgeType::ET_POSEDGE:
            ಯದಿ (other.m_e == VEdgeType::ET_NEGEDGE) ಪ್ರತ್ಯಾಗಮ true;
            ಭಙ್ಗ;
        ಪಕ್ಷ VEdgeType::ET_NEGEDGE:
            ಯದಿ (other.m_e == VEdgeType::ET_POSEDGE) ಪ್ರತ್ಯಾಗಮ true;
            ಭಙ್ಗ;
        ಸ್ವಭಾವ: ಭಙ್ಗ;
        }
        ಪ್ರತ್ಯಾಗಮ false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
};
constexpr bool operator==(ಧ್ರುವ VEdgeType& lhs, ಧ್ರುವ VEdgeType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VEdgeType& lhs, VEdgeType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, ಧ್ರುವ VEdgeType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }

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
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    ಗಣನಾ en m_e;

    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ ಮೌಲ್ಯಗಳು[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool isIterated() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool hasTriggers() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool firstIteration() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool slow() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }

    // Short name
    std::string tag() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }

ಗುಪ್ತ:
    std::string capitalizedTag() ಧ್ರುವ {
        ಧ್ರುವ std::string name = ascii();
        ಪ್ರತ್ಯಾಗಮ name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    // Is a node type compatible with the declaration
    bool is_ಗಂಟು_Compatible(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ;
};
constexpr bool operator==(ಧ್ರುವ VFwdType& lhs, ಧ್ರುವ VFwdType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ಧ್ರುವ VFwdType& lhs, VFwdType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, ಧ್ರುವ VFwdType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VFwdType& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    ಸ್ಥಿರ uint64_t s_cachedCntGbl;  // Global computed count

ಸಾರ್ವಜನಿಕ:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cachedCnt == s_cachedCntGbl; }
    bool get() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_state; }
    ಶೂನ್ಯ set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    ಶೂನ್ಯ clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ clear_Cache_ವೃಕ್ಷ() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    ಗಣನಾ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* verilogKwd() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"join", "join_any", "join_none"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    bool join() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == JOIN; }
    bool joinAny() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == JOIN_ANY; }
    bool joinNone() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == JOIN_NONE; }
};
constexpr bool operator==(ಧ್ರುವ VJoinType& lhs, ಧ್ರುವ VJoinType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VJoinType& lhs, VJoinType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, ಧ್ರುವ VJoinType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VJoinType& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    bool isNone() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == NONE; }
    bool isAutomatic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ಧ್ರುವ {
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ AUTOMATIC_EXPLICIT: ಪ್ರತ್ಯಾಗಮ AUTOMATIC_IMPLICIT;
        ಪಕ್ಷ STATIC_EXPLICIT: ಪ್ರತ್ಯಾಗಮ STATIC_IMPLICIT;
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ m_e;
        }
    }
};
constexpr bool operator==(ಧ್ರುವ VLifetime& lhs, ಧ್ರುವ VLifetime& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VLifetime& lhs, VLifetime::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, ಧ್ರುವ VLifetime& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VLifetime& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
ಸಾರ್ವಜನಿಕ:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            ಪೂರ್ಣಾಙ್ಕ m_index;  // Current index
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_inc;  // Step, +1 or -1
        ಸಾರ್ವಜನಿಕ:
            Iterator(ಪೂರ್ಣಾಙ್ಕ index, ಪೂರ್ಣಾಙ್ಕ inc)
                : m_index{index}
                , m_inc{inc} {}
            ಪೂರ್ಣಾಙ್ಕ operator*() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                ಪ್ರತ್ಯಾಗಮ *this;
            }
            bool operator!=(ಧ್ರುವ Iterator& that) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_index != that.m_index; }
        };
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_begin;  // First index
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_end;  // One past the last index
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_inc;  // Step, +1 or -1
    ಸಾರ್ವಜನಿಕ:
        Sequence(ಪೂರ್ಣಾಙ್ಕ first, ಪೂರ್ಣಾಙ್ಕ last, ಪೂರ್ಣಾಙ್ಕ inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ Iterator{m_begin, m_inc}; }
        Iterator end() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ Iterator{m_end, m_inc}; }
    };

    ಪೂರ್ಣಾಙ್ಕ m_ಎಡ = 0;  // Left side of range (pre-':')
    ಪೂರ್ಣಾಙ್ಕ m_ಬಲ = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(ಧ್ರುವ VNumRange& rhs) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_ಎಡ == rhs.m_ಎಡ && m_ಬಲ == rhs.m_ಬಲ && m_ranged == rhs.m_ranged;
    }
    bool operator<(ಧ್ರುವ VNumRange& rhs) ಧ್ರುವ {
        ಯದಿ ((m_ಎಡ < rhs.m_ಎಡ)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (!(m_ಎಡ == rhs.m_ಎಡ)) ಪ್ರತ್ಯಾಗಮ false;  // lhs > rhs
        ಯದಿ ((m_ಬಲ < rhs.m_ಬಲ)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (!(m_ಬಲ == rhs.m_ಬಲ)) ಪ್ರತ್ಯಾಗಮ false;  // lhs > rhs
        ಯದಿ ((m_ranged < rhs.m_ranged)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (!(m_ranged == rhs.m_ranged)) ಪ್ರತ್ಯಾಗಮ false;  // lhs > rhs
        ಪ್ರತ್ಯಾಗಮ false;
    }
    //
    VNumRange() = ಸ್ವಭಾವ;
    VNumRange(ಪೂರ್ಣಾಙ್ಕ hi, ಪೂರ್ಣಾಙ್ಕ lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(ಪೂರ್ಣಾಙ್ಕ ಎಡ, ಪೂರ್ಣಾಙ್ಕ ಬಲ)
        : m_ಎಡ{ಎಡ}
        , m_ಬಲ{ಬಲ}
        , m_ranged{true} {}
    ~VNumRange() = ಸ್ವಭಾವ;
    // MEMBERS
    ಶೂನ್ಯ init(ಪೂರ್ಣಾಙ್ಕ hi, ಪೂರ್ಣಾಙ್ಕ lo, bool ascending) {
        ಯದಿ (lo > hi) std::swap(hi, lo);
        m_ಎಡ = ascending ? lo : hi;
        m_ಬಲ = ascending ? hi : lo;
        m_ranged = true;
    }
    ಪೂರ್ಣಾಙ್ಕ ಎಡ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಎಡ; }
    ಪೂರ್ಣಾಙ್ಕ ಬಲ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಬಲ; }
    ಪೂರ್ಣಾಙ್ಕ hi() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_ಎಡ > m_ಬಲ ? m_ಎಡ : m_ಬಲ;
    }  // How to show a declaration
    ಪೂರ್ಣಾಙ್ಕ lo() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_ಎಡ > m_ಬಲ ? m_ಬಲ : m_ಎಡ;
    }  // How to show a declaration
    ಪೂರ್ಣಾಙ್ಕ ಎಡ_To_ಬಲ_Inc() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_ಎಡ_To_ಬಲ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ {m_ಎಡ, m_ಬಲ, ಎಡ_To_ಬಲ_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_ಬಲ_To_ಎಡ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ {m_ಬಲ, m_ಎಡ, -ಎಡ_To_ಬಲ_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ {hi(), lo(), -1}; }
    ಪೂರ್ಣಾಙ್ಕ elements() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ hi() - lo() + 1; }
    bool ranged() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ranged; }
    bool ascending() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಎಡ < m_ಬಲ; }
    ಪೂರ್ಣಾಙ್ಕ hiMaxSelect() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ಧ್ರುವ {
        ಯದಿ (ranged()) {
            ಪ್ರತ್ಯಾಗಮ "["s + std::to_string(ಎಡ()) + ":" + std::to_string(ಬಲ()) + "]";
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ "[norg]";
        }
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ { str << ascii(); }
};
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VNumRange& rhs) {
    rhs.dump(os);
    ಪ್ರತ್ಯಾಗಮ os;
}

// ######################################################################

class VPragmaType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
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
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {
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
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
};
constexpr bool operator==(ಧ್ರುವ VPragmaType& lhs, ಧ್ರುವ VPragmaType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VPragmaType& lhs, VPragmaType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, ಧ್ರುವ VPragmaType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VPragmaType& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    bool isRandomizable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e != NONE; }
    bool isRand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == RAND_CYCLIC; }
};
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VRandAttr& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ಸ್ಥಿರ ಧ್ರುವ std::shared_ptr<ಧ್ರುವ string> s_emptyp;  // Holds ""
    ಸ್ಥಿರ ಧ್ರುವ std::shared_ptr<ಧ್ರುವ string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ಧ್ರುವ string> m_strp;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, ಧ್ರುವ string& field)
        : m_strp{std::make_shared<ಧ್ರುವ string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, ಧ್ರುವ string& field)
        : m_strp{std::make_shared<ಧ್ರುವ string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_strp == s_emptyp; }
    bool isVlSym() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ಧ್ರುವ;
    ಸ್ಥಿರ string replaceThis(bool useSelfForThis, ಧ್ರುವ string& text);
    ಧ್ರುವ std::string& asString() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ *m_strp; }
    bool operator==(ಧ್ರುವ VSelfPointerText& other) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    ಸ್ಥಿರ VSigning fromBool(bool isSigned) {  // Factory method
        ಪ್ರತ್ಯಾಗಮ isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    bool isSigned() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == SIGNED; }
    bool isNosign() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(ಧ್ರುವ VSigning& lhs, ಧ್ರುವ VSigning& rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ಧ್ರುವ VSigning& lhs, VSigning::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, ಧ್ರುವ VSigning& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VSigning& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

class VStrength final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    ಗಣನಾ en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VStrength& lhs, ಧ್ರುವ VStrength& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VStrength& lhs, VStrength::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, ಧ್ರುವ VStrength& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VStrength& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    ಗಣನಾ en m_e;
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
};

//######################################################################

class VTracePrefixType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    ಗಣನಾ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VTracePrefixType& lhs, ಧ್ರುವ VTracePrefixType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, ಧ್ರುವ VTracePrefixType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
}
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VTracePrefixType& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

//######################################################################

class VTraceType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    ಗಣನಾ en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"CONSTANT", "FULL", "CHANGE"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಧ್ರುವ ವರ್ಣ* func_prefix() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"trace_const", "trace_full", "trace_chg"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VTraceType& lhs, ಧ್ರುವ VTraceType& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VTraceType& lhs, VTraceType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, ಧ್ರುವ VTraceType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VTraceType& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

//######################################################################

class VUseType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    ಗಣನಾ en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    bool containsAny(VUseType other) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e & other.m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        ಪ್ರತ್ಯಾಗಮ names[m_e - 1];
    }
};
constexpr bool operator==(ಧ್ರುವ VUseType& lhs, ಧ್ರುವ VUseType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ಧ್ರುವ VUseType& lhs, VUseType::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, ಧ್ರುವ VUseType& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    ಪ್ರತ್ಯಾಗಮ VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VUseType& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

class VVarType final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
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
    ಗಣನಾ en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    bool isParam() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ (m_e == TRIOR); }
    bool isWiredNet() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ಧ್ರುವ {  // In Verilog, always ok in SystemVerilog
        ಪ್ರತ್ಯಾಗಮ (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ಧ್ರುವ ವರ್ಣ* traceSigKind() ಧ್ರುವ {
        // VerilatedTraceSigKind to used in trace signal declaration
        ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ lut[] = {
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
        ಪ್ರತ್ಯಾಗಮ lut[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VVarType& lhs, ಧ್ರುವ VVarType& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, ಧ್ರುವ VVarType& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
}
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VVarType& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_ಕೀಲಿ final {
ಸಾರ್ವಜನಿಕ:
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_width;  // From AstNodeDType: Bit width of operation
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ಧ್ರುವ VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ಧ್ರುವ VSigning m_numeric;  // From AstNodeDType: Node is signed
    ಧ್ರುವ VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(ಧ್ರುವ VBasic_Type_ಕೀಲಿ& rhs) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(ಧ್ರುವ VBasic_Type_ಕೀಲಿ& rhs) ಧ್ರುವ {
        ಯದಿ ((m_width < rhs.m_width)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (!(m_width == rhs.m_width)) ಪ್ರತ್ಯಾಗಮ false;  // lhs > rhs
        ಯದಿ ((m_widthMin < rhs.m_widthMin)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (!(m_widthMin == rhs.m_widthMin)) ಪ್ರತ್ಯಾಗಮ false;  // lhs > rhs
        ಯದಿ ((m_numeric < rhs.m_numeric)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (!(m_numeric == rhs.m_numeric)) ಪ್ರತ್ಯಾಗಮ false;  // lhs > rhs
        ಯದಿ ((m_keyword < rhs.m_keyword)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (!(m_keyword == rhs.m_keyword)) ಪ್ರತ್ಯಾಗಮ false;  // lhs > rhs
        ಯದಿ ((m_nrange < rhs.m_nrange)) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (!(m_nrange == rhs.m_nrange)) ಪ್ರತ್ಯಾಗಮ false;  // lhs > rhs
        ಪ್ರತ್ಯಾಗಮ false;
    }
    VBasic_Type_ಕೀಲಿ(ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  ಧ್ರುವ VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_ಕೀಲಿ() = ಸ್ವಭಾವ;
};

#endif  // Guard
