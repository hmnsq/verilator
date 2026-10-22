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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ASTATTR_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ASTATTR_H_

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3AST_H_
#ਗਲਤੀ "Use V3Ast.h as the include"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ਜੇ_ਅੰਤ

// Hint class so we can choose constructors
class VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed {};
class VFlag_ਬੱਚਾ_DType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbort_ਕਿਸਮ final {
ਜਨਤਕ:
    // IEEE 1800-2023 16.12.14 property abort operators.
    ਗਿਣਤੀ en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    ਗਿਣਤੀ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbort_ਕਿਸਮ(en _e)
        : m_e{_e} {}
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    bool isAccept() ਅਚਲ { ਵਾਪਸ m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"RD", "WR", "RW", "--"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* arrow() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    VAccess invert() ਅਚਲ {
        ਵਾਪਸ (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ਅਚਲ { ਵਾਪਸ m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ਅਚਲ { ਵਾਪਸ m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ਅਚਲ { ਵਾਪਸ m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ਅਚਲ { ਵਾਪਸ m_e == WRITE || m_e == READWRITE; }
    bool isRW() ਅਚਲ { ਵਾਪਸ m_e == READWRITE; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAccess& lhs, ਅਚਲ VAccess& rhs) { ਵਾਪਸ lhs.m_e == rhs.m_e; }
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAccess& lhs, VAccess::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VAccess::en lhs, ਅਚਲ VAccess& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VAccess& rhs) { ਵਾਪਸ os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ਸੌਂਪੋ };
    ਗਿਣਤੀ en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAlwaysKwd& lhs, ਅਚਲ VAlwaysKwd& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VAlwaysKwd::en lhs, ਅਚਲ VAlwaysKwd& rhs) { ਵਾਪਸ lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
ਜਨਤਕ:
    // IEEE 1800-2023 Table 20-5
    ਗਿਣਤੀ en : uint8_t {
        ਮੰਜ਼ਿਲ_BE_EVALUATED = 0,
        ਤਾਲਾ = 1,
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
    ਗਿਣਤੀ en m_e;
    VAssertCtlType()
        : m_e{ਮੰਜ਼ਿਲ_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        // IEEE 1800-2023 20.11
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"",
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
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAssertCtlType& lhs, ਅਚਲ VAssertCtlType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VAssertCtlType::en lhs, ਅਚਲ VAssertCtlType& rhs) {
    ਵਾਪਸ lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
ਜਨਤਕ:
    // IEEE 1800-2023 Table 20-7
    ਗਿਣਤੀ en : uint8_t {
        ਅੰਦਰੂਨੀ = 0,  // Non IEEE type, for directives to be evaluated from expression.
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
    ਗਿਣਤੀ en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ਅਚਲ {
        std::stringstream types;
        ਜੇ (m_e == ਅੰਦਰੂਨੀ)
            types << "INTERNAL ";
        ਨਹੀਂਤਾਂ {
            ਜੇ (m_e & ASSERT) types << "ASSERT ";
            ਜੇ (m_e & COVER) types << "COVER ";
            ਜੇ (m_e & ASSUME) types << "ASSUME ";
            ਜੇ (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            ਜੇ (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            ਜੇ (m_e & INTRINSIC) types << "INTRINSIC ";
            ਜੇ (m_e & RESTRICT) types << "RESTRICT ";
        }
        ਅਚਲ string str = types.str();
        UASSERT(!str.ਖਾਲੀ_ਥਾਵਾਂ(), "Assert should be of one of types");
        ਵਾਪਸ str.substr(0, str.ਮਾਪ() - 1);
    }
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAssertDirectiveType& lhs, ਅਚਲ VAssertDirectiveType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VAssertDirectiveType::en lhs, ਅਚਲ VAssertDirectiveType& rhs) {
    ਵਾਪਸ lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en ਸੰਚਾਲਕ|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    ਵਾਪਸ VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
ਜਨਤਕ:
    // IEEE 1800-2023 Table 20-6
    ਗਿਣਤੀ en : uint8_t {
        ਅੰਦਰੂਨੀ = 0,  // Non IEEE type, for assertions that should not be controlled.
        CONCURRENT = (1 << 0),
        SIMPLE_ਤੁਰੰਤ = (1 << 1),
        OBSERVED_DEFERRED_ਤੁਰੰਤ = (1 << 2),
        FINAL_DEFERRED_ਤੁਰੰਤ = (1 << 3),
        EXPECT = (1 << 4),
        UNIQUE = (1 << 5),
        UNIQUE0 = (1 << 6),
        ਤਰਜੀਹ = (1 << 7),
    };
    ਗਿਣਤੀ en m_e;
    VAssertType()
        : m_e{ਅੰਦਰੂਨੀ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool ਸ਼ਾਮਲ_ਹੈ_Any(VAssertType other) ਅਚਲ { ਵਾਪਸ m_e & other.m_e; }
    string ascii() ਅਚਲ {
        std::stringstream types;
        ਜੇ (m_e == ਅੰਦਰੂਨੀ)
            types << "INTERNAL ";
        ਨਹੀਂਤਾਂ {
            ਜੇ (m_e & CONCURRENT) types << "CONCURRENT ";
            ਜੇ (m_e & SIMPLE_ਤੁਰੰਤ) types << "SIMPLE_IMMEDIATE ";
            ਜੇ (m_e & OBSERVED_DEFERRED_ਤੁਰੰਤ) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            ਜੇ (m_e & FINAL_DEFERRED_ਤੁਰੰਤ) types << "FINAL_DEFERRED_IMMEDIATE ";
            ਜੇ (m_e & EXPECT) types << "EXPECT ";
            ਜੇ (m_e & UNIQUE) types << "UNIQUE ";
            ਜੇ (m_e & UNIQUE0) types << "UNIQUE0 ";
            ਜੇ (m_e & ਤਰਜੀਹ) types << "PRIORITY ";
        }
        ਅਚਲ string str = types.str();
        UASSERT(!str.ਖਾਲੀ_ਥਾਵਾਂ(), "Assert should be of one of types");
        ਵਾਪਸ str.substr(0, str.ਮਾਪ() - 1);
    }
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAssertType& lhs, ਅਚਲ VAssertType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAssertType& lhs, VAssertType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VAssertType::en lhs, ਅਚਲ VAssertType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
constexpr VAssertType::en ਸੰਚਾਲਕ|(VAssertType::en lhs, VAssertType::en rhs) {
    ਵਾਪਸ VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    ਗਿਣਤੀ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"default", "weak", "strong"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VPropStrength& lhs, ਅਚਲ VPropStrength& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VPropStrength& lhs, VPropStrength::en rhs) {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ!=(ਅਚਲ VPropStrength& lhs, VPropStrength::en rhs) {
    ਵਾਪਸ lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
ਜਨਤਕ:
    // clang-format off
    ਗਿਣਤੀ en: uint8_t  {
        ILLEGAL,
        //
        DIM_ਬਿੱਟ,                       // V3Const converts to constant
        DIM_ਬਿੱਟ_OR_ਸੰਖਿਆ,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_ਉੱਚਾ,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_ਖੱਬਾ,                       // V3Width processes
        DIM_ਨੀਵਾਂ,                        // V3Width processes
        DIM_ਸੱਜਾ,                      // V3Width processes
        DIM_ਮਾਪ,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_ਜਨਤਕ,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_ਪਹਿਲਾ,                     // V3Width processes
        ENUM_ਆਖਰੀ,                      // V3Width processes
        ENUM_ਅੰਕ,                       // V3Width processes
        ENUM_ਅਗਲਾ,                      // V3Width processes
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
        VAR_ਅਧਾਰ,                       // V3LinkResolve creates for AstPreSel, V3LinkParam removes
        VAR_FORCEABLE,                  // V3LinkParse moves to AstVar::isForceable
        VAR_FSM_ARC_INCLUDE_COND,       // V3LinkParse moves to AstVar::attrFsmArcInclCond
        VAR_FSM_ARC_INCLUDE_COND_AUTO,  // Flavors of above option - setting expand mode per FSM
        VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND,
        VAR_FSM_ARC_INCLUDE_COND_ਭਰਿਆ,
        VAR_FSM_STATE,                  // V3LinkParse moves to AstVar::attrFsmState
        VAR_FSM_STATE_AUTO,             // Flavors of above option - setting expand mode per FSM
        VAR_FSM_STATE_AUTO_EXPAND,
        VAR_FSM_STATE_ਭਰਿਆ,
        VAR_ਪੋਰਟ_DTYPE,                 // V3LinkDot for V3Width to check port dtype
        VAR_ਜਨਤਕ,                     // V3LinkParse moves to AstVar::sigPublic
        VAR_ਜਨਤਕ_FLAT,                // V3LinkParse moves to AstVar::sigPublic
        VAR_ਜਨਤਕ_FLAT_RD,             // V3LinkParse moves to AstVar::sigPublic
        VAR_ਜਨਤਕ_FLAT_RW,             // V3LinkParse moves to AstVar::sigPublic
        VAR_SC_BIGUINT,                 // V3LinkParse moves to AstVar::attrScBigUint
        VAR_SC_BV,                      // V3LinkParse moves to AstVar::attrScBv
        VAR_SFORMAT,                    // V3LinkParse moves to AstVar::attrSFormat
        VAR_ਵੰਡੋ_VAR                   // V3LinkParse moves to AstVar::attrSplitVar
    };
    // clang-format on
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        // clang-format off
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {
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
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ਅਚਲ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ DIM_ਬਿੱਟ:
        ਹਾਲਤ DIM_ਬਿੱਟ_OR_ਸੰਖਿਆ:
        ਹਾਲਤ DIM_DIMENSIONS:
        ਹਾਲਤ DIM_ਉੱਚਾ:
        ਹਾਲਤ DIM_INCREMENT:
        ਹਾਲਤ DIM_ਖੱਬਾ:
        ਹਾਲਤ DIM_ਨੀਵਾਂ:
        ਹਾਲਤ DIM_ਸੱਜਾ:
        ਹਾਲਤ DIM_ਮਾਪ:
        ਹਾਲਤ DIM_UNPK_DIMENSIONS:
        ਹਾਲਤ TYPEID:
        ਹਾਲਤ TYPENAME: ਵਾਪਸ true;
        ਮੂਲ: ਵਾਪਸ false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAttrType& lhs, ਅਚਲ VAttrType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VAttrType& lhs, VAttrType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VAttrType::en lhs, ਅਚਲ VAttrType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

ਜਨਤਕ:
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
    ਖਾਲੀ combine(ਅਚਲ VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ਅਚਲ { ਵਾਪਸ m_extends | m_final | m_initial; }
    bool isExtends() ਅਚਲ { ਵਾਪਸ m_extends; }
    bool isFinal() ਅਚਲ { ਵਾਪਸ m_final; }
    bool isInitial() ਅਚਲ { ਵਾਪਸ m_initial; }
    string ascii() ਅਚਲ {
        string ਬਾਹਰਲਾ;
        ਜੇ (m_initial) ਬਾਹਰਲਾ = VString::dot(ਬਾਹਰਲਾ, " ", "initial");
        ਜੇ (m_extends) ਬਾਹਰਲਾ = VString::dot(ਬਾਹਰਲਾ, " ", "extends");
        ਜੇ (m_final) ਬਾਹਰਲਾ = VString::dot(ਬਾਹਰਲਾ, " ", "final");
        ਵਾਪਸ ਬਾਹਰਲਾ;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        UNKNOWN,
        ਦੋਆਧਾਰੀ_ਅੰਕ,
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
        ਸਮਾਂ,
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
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"%E-unk",
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
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* typeName() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ LOGIC2STATE: ਵਾਪਸ VBasicDTypeKwd{LOGIC}.ascii();
        ਹਾਲਤ INTEGER2STATE: ਵਾਪਸ VBasicDTypeKwd{INTEGER}.ascii();
        ਹਾਲਤ TIME2STATE: ਵਾਪਸ VBasicDTypeKwd{ਸਮਾਂ}.ascii();
        ਮੂਲ: ਵਾਪਸ ascii();
        }
    }
    ਅਚਲ ਅੱਖਰ* dpiType() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"%E-unk",
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
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਸਥਿਰ ਖਾਲੀ selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_ਸੁਰੱਖਿਅਤ : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_ਸੁਰੱਖਿਅਤ : m_e{_e} {}
    explicit VBasicDTypeKwd(ਪੂਰਨ _e) VL_MT_ਸੁਰੱਖਿਅਤ  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਪੂਰਨ ਚੌੜਾਈ() ਅਚਲ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ ਦੋਆਧਾਰੀ_ਅੰਕ: ਵਾਪਸ 1;  // scalar, can't bit extract unless ranged
        ਹਾਲਤ BYTE: ਵਾਪਸ 8;
        ਹਾਲਤ CHANDLE: ਵਾਪਸ 64;
        ਹਾਲਤ NULLTYPE: ਵਾਪਸ 1;  // Historical - bit was used instead of NULLTYPE
        ਹਾਲਤ EVENT: ਵਾਪਸ 1;
        ਹਾਲਤ INT: ਵਾਪਸ 32;
        ਹਾਲਤ INTEGER: ਵਾਪਸ 32;
        ਹਾਲਤ INTEGER2STATE: ਵਾਪਸ 32;
        ਹਾਲਤ LOGIC: ਵਾਪਸ 1;  // scalar, can't bit extract unless ranged
        ਹਾਲਤ LOGIC2STATE: ਵਾਪਸ 1;  // scalar, can't bit extract unless ranged
        ਹਾਲਤ LONGINT: ਵਾਪਸ 64;
        ਹਾਲਤ DOUBLE: ਵਾਪਸ 64;  // opaque
        ਹਾਲਤ SHORTINT: ਵਾਪਸ 16;
        ਹਾਲਤ ਸਮਾਂ: ਵਾਪਸ 64;
        ਹਾਲਤ TIME2STATE: ਵਾਪਸ 64;
        ਹਾਲਤ STRING: ਵਾਪਸ 64;  // opaque  // Just the pointer, for today
        ਹਾਲਤ SCOPEPTR: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ CHARPTR: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ MTASKSTATE: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ DELAY_SCHEDULER: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ TRIGGER_SCHEDULER: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ DYNAMIC_TRIGGER_SCHEDULER: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ FORK_SYNC: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ PROCESS_REFERENCE: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ RANDOM_GENERATOR: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ RANDOM_STDGENERATOR: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ COVERGROUP_INSTHANDLE: ਵਾਪਸ 0;  // opaque
        ਹਾਲਤ UINT32: ਵਾਪਸ 32;
        ਹਾਲਤ UINT64: ਵਾਪਸ 64;
        ਮੂਲ: ਵਾਪਸ 0;
        }
    }
    bool isSigned() ਅਚਲ {
        ਵਾਪਸ m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ਅਚਲ {
        ਵਾਪਸ m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == ਦੋਆਧਾਰੀ_ਅੰਕ || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == ਸਮਾਂ || m_e == TIME2STATE;
    }
    bool isFourstate() ਅਚਲ {
        ਵਾਪਸ m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == ਸਮਾਂ;
    }
    bool isZeroInit() ਅਚਲ {  // Otherwise initializes to X
        ਵਾਪਸ (m_e == ਦੋਆਧਾਰੀ_ਅੰਕ || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ਅਚਲ {  // Enum increment supported
        ਵਾਪਸ (m_e == ਦੋਆਧਾਰੀ_ਅੰਕ || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == ਸਮਾਂ || m_e == TIME2STATE);
    }
    bool is_ਦੋਆਧਾਰੀ_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_e == ਦੋਆਧਾਰੀ_ਅੰਕ; }
    bool is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic() ਅਚਲ {  // Bit/logic vector types; can form a packed array
        ਵਾਪਸ (m_e == LOGIC || m_e == LOGIC2STATE || m_e == ਦੋਆਧਾਰੀ_ਅੰਕ);
    }
    bool isDpiUnsignable() ਅਚਲ {  // Can add "unsigned" to DPI
        ਵਾਪਸ (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ਅਚਲ {  // Uses standard C layout, for DPI runtime access
        ਵਾਪਸ (m_e == ਦੋਆਧਾਰੀ_ਅੰਕ || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {  // IE not a simple number we can bit optimize
        ਵਾਪਸ (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == CHANDLE; }
    bool isDouble() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == DOUBLE; }
    bool isEvent() ਅਚਲ { ਵਾਪਸ m_e == EVENT; }
    bool isString() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == STRING; }
    bool isMTaskState() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == MTASKSTATE; }
    bool isSameish(ਅਚਲ VBasicDTypeKwd& other) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਚੋਣ (other.m_e) {
        ਹਾਲਤ LOGIC:
        ਹਾਲਤ LOGIC2STATE:
        ਹਾਲਤ LOGIC_IMPLICIT: ਵਾਪਸ m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        ਹਾਲਤ INTEGER:
        ਹਾਲਤ INTEGER2STATE: ਵਾਪਸ m_e == INTEGER || m_e == INTEGER2STATE;
        ਹਾਲਤ ਸਮਾਂ:
        ਹਾਲਤ TIME2STATE: ਵਾਪਸ m_e == ਸਮਾਂ || m_e == TIME2STATE;
        ਮੂਲ: ਵਾਪਸ m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ ਦੋਆਧਾਰੀ_ਅੰਕ:
        ਹਾਲਤ BYTE:
        ਹਾਲਤ CHANDLE:
        ਹਾਲਤ INT:
        ਹਾਲਤ INTEGER:
        ਹਾਲਤ INTEGER2STATE:
        ਹਾਲਤ LOGIC:
        ਹਾਲਤ LOGIC2STATE:
        ਹਾਲਤ LONGINT:
        ਹਾਲਤ DOUBLE:
        ਹਾਲਤ SHORTINT:
        ਹਾਲਤ SCOPEPTR:
        ਹਾਲਤ CHARPTR:
        ਹਾਲਤ UINT32:
        ਹਾਲਤ UINT64: ਵਾਪਸ true;
        ਮੂਲ: ਵਾਪਸ false;
        }
    }

    ਅਚਲ ਅੱਖਰ* traceSigType() ਅਚਲ {
        // VerilatedTraceSigType to used in trace signal declaration
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ lut[] = {
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
        static_assert(ਆਕਾਰ(lut) / ਆਕਾਰ(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        ਵਾਪਸ lut[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VBasicDTypeKwd& lhs, ਅਚਲ VBasicDTypeKwd& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VBasicDTypeKwd::en lhs, ਅਚਲ VBasicDTypeKwd& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, ENUM_ਸਿਰਾ };
    ਗਿਣਤੀ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"FALSE", "TRUE", "UNK"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    bool is_ਗਿਆਤ() ਅਚਲ { ਵਾਪਸ m_e != BU_UNKNOWN; }
    bool true_ਗਿਆਤ() ਅਚਲ { ਵਾਪਸ m_e == BU_TRUE; }
    ਖਾਲੀ ਸੈੱਟ_True_Or_False(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VBoolOrUnknown& lhs, ਅਚਲ VBoolOrUnknown& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VBoolOrUnknown::en lhs, ਅਚਲ VBoolOrUnknown& rhs) {
    ਵਾਪਸ lhs == rhs.m_e;
}
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VBoolOrUnknown& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, ENUM_ਸਿਰਾ };
    ਗਿਣਤੀ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    bool unknown() ਅਚਲ { ਵਾਪਸ m_e == BP_UNKNOWN; }
    bool likely() ਅਚਲ { ਵਾਪਸ m_e == BP_LIKELY; }
    bool unlikely() ਅਚਲ { ਵਾਪਸ m_e == BP_UNLIKELY; }
    VBranchPred invert() ਅਚਲ {
        ਜੇ (m_e == BP_UNLIKELY) ਵਾਪਸ BP_LIKELY;
        ਜੇ (m_e == BP_LIKELY) ਵਾਪਸ BP_UNLIKELY;
        ਵਾਪਸ m_e;
    }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* asciiShort() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"", "L", "!L"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VBranchPred& lhs, ਅਚਲ VBranchPred& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VBranchPred& lhs, VBranchPred::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VBranchPred::en lhs, ਅਚਲ VBranchPred& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VBranchPred& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ਸਥਿਰ constexpr bool validate_ਦਲੀਲਾਂ_Descriptor(ਅਚਲ ਅੱਖਰ* descrp) {
    // Is "TODO"
    ਜੇ (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        ਵਾਪਸ true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    ਲਈ (ਅਚਲ ਅੱਖਰ* cp = descrp; *cp; ++cp) {
        ਜੇ (*cp == '+') ਵਾਪਸ cp != descrp && !cp[1];
        ਜੇ (*cp != 'r' && *cp != 'w' && *cp != 'm') ਵਾਪਸ false;
    }
    ਵਾਪਸ true;
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
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(ARRAY_AT_ਪਿੱਠ,                      "atBack",                 ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(ARRAY_ਲੱਭੋ,                         "find",                   ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_ਲੱਭੋ_ਪਹਿਲਾ,                   "find_first",             ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_ਲੱਭੋ_ਪਹਿਲਾ_ਸੂਚਕਾਂਕ,             "find_first_index",       ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_ਲੱਭੋ_ਸੂਚਕਾਂਕ,                   "find_index",             ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_ਲੱਭੋ_ਆਖਰੀ,                    "find_last",              ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_ਲੱਭੋ_ਆਖਰੀ_ਸੂਚਕਾਂਕ,              "find_last_index",        ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_ਪਹਿਲਾ,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(ARRAY_ਆਖਰੀ,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_ਅਗਲਾ,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_ਕੱਢੋ_ਪਿੱਠ,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_ਕੱਢੋ_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ਧੱਕੋ_ਪਿੱਠ,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ਧੱਕੋ_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_ਉਲਟ,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_R_ਗੁਣਨਫਲ,                    "r_product",              ਸ਼ੁੱਧ_2,   false,  "") \
    macro(ARRAY_R_ਜੋੜ,                        "r_sum",                  ਸ਼ੁੱਧ_2,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_ਟੁਕੜਾ,                        "slice",                  ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(ARRAY_ਛਾਂਟੋ,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_ਸੂਚਕਾਂਕ,                 "unique_index",           ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(ASSOC_ਪਹਿਲਾ,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_ਅਗਲਾ,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_ਮਾਪ,                         "size",                   ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(CLASS_ਸੈੱਟ_RANDMODE,                 "set_randmode",           false,  CLEAN,  "r") \
    macro(COVERGROUP_ਜੋੜੋ_ARRAY_NAMER,         "addArrayNamer",          false,  CLEAN,  "r+") \
    macro(COVERGROUP_ਜੋੜੋ_BIN,                 "addBin",                 false,  CLEAN,  "r+") \
    macro(COVERGROUP_ਜੋੜੋ_COVERPOINT,          "addCoverpoint",          false,  CLEAN,  "") \
    macro(COVERGROUP_ਜੋੜੋ_CROSS,               "addCross",               false,  CLEAN,  "") \
    macro(COVERGROUP_ਜੋੜੋ_CROSS_DYN,           "addCrossDyn",            false,  CLEAN,  "") \
    macro(COVERGROUP_ਜੋੜੋ_NUMBERED_NAMER,      "addNumberedNamer",       false,  CLEAN,  "r+") \
    macro(COVERGROUP_ਜੋੜੋ_SINGLE_NAMER,        "addSingleNamer",         false,  CLEAN,  "r+") \
    macro(COVERGROUP_ATTACH,                  "attach",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_CLEAR_ਪ੍ਰਾਪਤੀ_LIST,          "clearHitList",           false,  CLEAN,  "") \
    macro(COVERGROUP_COVERAGE,                "coverage",               false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_BEGIN,        "withBegin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_CROSS_WITH_ਸਿਰਾ,          "withEnd",                false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_TEST,         "withTest",               false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_ਮੁੱਲ,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_ਮੁੱਲ_W,      "withValueW",             false,  CLEAN,  "rw") \
    macro(COVERGROUP_FINALIZE_BINS,           "finalizeBins",           false,  CLEAN,  "") \
    macro(COVERGROUP_INCREMENT_BIN,           "incrementBin",           false,  CLEAN,  "r") \
    macro(COVERGROUP_INIT,                    "init",                   false,  CLEAN,  "r+") \
    macro(COVERGROUP_INST_P,                  "p",                      ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(COVERGROUP_LEND_ਭਾਰ,             "lendWeight",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_ਰਿਕਾਰਡ_ਪ੍ਰਾਪਤੀ,              "recordHit",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ਰਜਿਸਟਰ_BINS,           "registerBins",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_SAMPLE,                  "sample",                 false,  CLEAN,  "") \
    macro(COVERGROUP_SAMPLE_IFFS,             "sample",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_SELECT_ALL,              "selectAll",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_AND,              "selectAnd",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_BIN,              "selectBin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM,              "selectDim",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM_ਸਿਰਾ,          "selectDimEnd",           false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_OR,               "selectOr",               false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_RANGE,            "selectRange",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SELECT_RANGE_W,          "selectRangeW",           false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_ਸਿਰਾ,               "sizedEnd",               ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_ਅੰਤ,            "sizedFinish",            false,  CLEAN,  "r+") \
    macro(COVERGROUP_SIZED_ਪਹਿਲਾ,             "sizedFirst",             ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_RANGE,             "sizedRange",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_RANGE_W,           "sizedRangeW",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_SAMPLE,            "sizedSample",            false,  CLEAN,  "rrr") \
    macro(COVERGROUP_SIZED_SAMPLE_W,          "sizedSampleW",           false,  CLEAN,  "rrr") \
    macro(COVERGROUP_TYPE_COVERAGE,           "typeCoverage",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_TYPE_ਭਾਰ,             "typeWeight",             false,  CLEAN,  "r") \
    macro(COVERGROUP_ਮੁੱਲ_EXCLUDED,          "valueExcluded",          ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(COVERGROUP_ਮੁੱਲ_EXCLUDED_W,        "valueExcludedW",         ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(COVERGROUP_ਮੁੱਲ_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_ਮੁੱਲ_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_ਮੁੱਲ_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_ਮੁੱਲ_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_ਮੁੱਲ_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ਮੁੱਲ_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_ਮੁੱਲ_TYPE,              "valueType",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_ਭਾਰ,                  "weight",                 false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_BEGIN,              "withBegin",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_ਅੰਤ,             "withFinish",             false,  CLEAN,  "r+") \
    macro(COVERGROUP_WITH_HI,                 "withHi",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_HI_W,               "withHiW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_LO,                 "withLo",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_LO_W,               "withLoW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_ਅਗਲਾ,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_ਲੜੀਵਾਰ,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_ਲੜੀਵਾਰ_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_ਪਿੱਠ,           "atWriteAppendBack",      false,  CLEAN,  "r") \
    macro(DYN_CLEAR,                          "clear",                  false,  CLEAN,  "") \
    macro(DYN_ERASE,                          "erase",                  false,  CLEAN,  "r") \
    macro(DYN_ਪਾਓ,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_ਕੱਢੋ,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_ਕੱਢੋ_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ਧੱਕੋ,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ਧੱਕੋ_FRONT,                     "push_front",             false,  CLEAN,  "r") \
    macro(DYN_RENEW,                          "renew",                  false,  CLEAN,  "r") \
    macro(DYN_RENEW_COPY,                     "renew_copy",             false,  CLEAN,  "rr") \
    macro(DYN_RESIZE,                         "resize",                 false,  CLEAN,  "TODO") \
    macro(DYN_ਮਾਪ,                           "size",                   ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(DYN_ਟੁਕੜਾ,                          "slice",                  ਸ਼ੁੱਧ_2,   CLEAN,  "rr") \
    macro(DYN_ਟੁਕੜਾ_ਸੌਂਪੋ,                   "sliceAssign",            false,  CLEAN,  "rrr") \
    macro(DYN_ਟੁਕੜਾ_ਸੌਂਪੋ_ਪਿੱਠ_ਪਿੱਠ,         "sliceAssignBackBack",    false,  CLEAN,  "rrr") \
    macro(DYN_ਟੁਕੜਾ_ਸੌਂਪੋ_FRONT_ਪਿੱਠ,        "sliceAssignFrontBack",   false,  CLEAN,  "rrr") \
    macro(DYN_ਟੁਕੜਾ_ਪਿੱਠ_ਪਿੱਠ,                "sliceBackBack",          ਸ਼ੁੱਧ_2,   CLEAN,  "rr") \
    macro(DYN_ਟੁਕੜਾ_FRONT_ਪਿੱਠ,               "sliceFrontBack",         ਸ਼ੁੱਧ_2,   CLEAN,  "rr") \
    macro(EVENT_CLEAR_FIRED,                  "clearFired",             false,  CLEAN,  "") \
    macro(EVENT_CLEAR_TRIGGERED,              "clearTriggered",         false,  CLEAN,  "") \
    macro(EVENT_FIRE,                         "fire",                   false,  CLEAN,  "") \
    macro(EVENT_IS_FIRED,                     "isFired",                ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(EVENT_IS_TRIGGERED,                 "isTriggered",            ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(FORCE_ਜੋੜੋ,                          "addForce",               false,  CLEAN,  "rrrr+") \
    macro(FORCE_READ,                         "read",                   ਸ਼ੁੱਧ_2,   CLEAN,  "r") \
    macro(FORCE_READ_ਸੂਚਕਾਂਕ,                   "readIndex",              ਸ਼ੁੱਧ_2,   CLEAN,  "rr") \
    macro(FORCE_READ_SEL,                     "readSel",                ਸ਼ੁੱਧ_2,   CLEAN,  "TODO") \
    macro(FORCE_RELEASE,                      "release",                false,  CLEAN,  "rr+") \
    macro(FORCE_TOUCH,                        "touch",                  false,  CLEAN,  "") \
    macro(FORK_ਪੂਰਾ,                          "done",                   false,  CLEAN,  "rr") \
    macro(FORK_INIT,                          "init",                   false,  CLEAN,  "rr") \
    macro(FORK_JOIN,                          "join",                   false,  CLEAN,  "rrr") \
    macro(FORK_ON_KILL,                       "onKill",                 false,  CLEAN,  "r") \
    macro(NBA_COMMIT,                         "commit",                 false,  CLEAN,  "w") \
    macro(NBA_ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ,                        "enqueue",                false,  CLEAN,  "r+") \
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
    macro(RANDOMIZER_ਸੈੱਟ_VAR_DISABLED,        "set_var_disabled",       false,  CLEAN,  "r") \
    macro(RANDOMIZER_CLEAR_VAR_DISABLED,      "clear_var_disabled",     false,  CLEAN,  "r") \
    macro(RANDOMIZER_MARK_VAR_STATIC,         "mark_var_static",        false,  CLEAN,  "r") \
    macro(RANDOMIZER_ਸੈੱਟ_STATIC_RANDMODE,     "set_static_randmode",    false,  CLEAN,  "r") \
    macro(RNG_GET_RANDSTATE,                  "__Vm_rng.get_randstate", ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(RNG_ਸੈੱਟ_RANDSTATE,                  "__Vm_rng.set_randstate", false,  CLEAN,  "r") \
    macro(SCHED_ANY_TRIGGERED,                "anyTriggered",           false,  CLEAN,  "r") \
    macro(SCHED_AWAITING_CURRENT_ਸਮਾਂ,        "awaitingCurrentTime",    ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(SCHED_AWAITING_ZERO_DELAY,          "awaitingZeroDelay",      ਸ਼ੁੱਧ_2,   CLEAN,  "") \
    macro(SCHED_READY,                        "ready",                  false,  CLEAN,  "r") \
    macro(SCHED_MOVE_ਮੰਜ਼ਿਲ_RESUME_ਕਤਾਰ,         "moveToResumeQueue",      false,  CLEAN,  "r") \
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
    macro(UNPACKED_ਸੌਂਪੋ,                    "assign",                 false,  CLEAN,  "r") \
    macro(UNPACKED_FILL,                      "fill",                   false,  CLEAN,  "r") \
    macro(UNPACKED_NEQ,                       "neq",                    ਸ਼ੁੱਧ_2,   CLEAN,  "r")
// clang-format on

class VCMethod final {
    ਸਥਿਰ constexpr bool ਸ਼ੁੱਧ_2 = true;  // For macro expansion of 'pure' field only
    ਸਥਿਰ constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CMETHOD_ID(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, clean, ਦਲੀਲਾਂ) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    ਗਿਣਤੀ en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CMETHOD_NAME(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, clean, ਦਲੀਲਾਂ) ਵਿਧੀ,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    bool is_ਸ਼ੁੱਧ() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ constexpr bool ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CMETHOD_ਸ਼ੁੱਧ(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, clean, ਦਲੀਲਾਂ) ਸ਼ੁੱਧ_3,
            FOR_EACH_CMETHOD(VL_CMETHOD_ਸ਼ੁੱਧ)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CMETHOD_ਸ਼ੁੱਧ
                false  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    bool isClean() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ constexpr bool ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CMETHOD_CLEAN(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, clean, ਦਲੀਲਾਂ) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CMETHOD_CLEAN
                false  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* ਦਲੀਲਾਂ() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CMETHOD_ਦਲੀਲਾਂ(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, clean, ਦਲੀਲਾਂ) ਦਲੀਲਾਂ,
            FOR_EACH_CMETHOD(VL_CMETHOD_ਦਲੀਲਾਂ)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CMETHOD_ਦਲੀਲਾਂ
                ""  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    // Return array method for given name
    ਸਥਿਰ VCMethod array_ਵਿਧੀ(ਅਚਲ string& name);
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCMethod& lhs, ਅਚਲ VCMethod& rhs) { ਵਾਪਸ lhs.m_e == rhs.m_e; }
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCMethod& lhs, VCMethod::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VCMethod::en lhs, ਅਚਲ VCMethod& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VCMethod& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CMETHOD_ਦਲੀਲਾਂ_CHECK(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, clean, ਦਲੀਲਾਂ) \
    static_assert(validate_ਦਲੀਲਾਂ_Descriptor(ਦਲੀਲਾਂ), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_ਦਲੀਲਾਂ_CHECK)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CMETHOD_ਦਲੀਲਾਂ_CHECK

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ FOR_EACH_CMETHOD

// clang-format off
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_ਮੰਜ਼ਿਲ_SOLVER_HEX,           "vlToSolverHex",          ਸ਼ੁੱਧ_2,   "r")
// clang-format on

class VCFunction final {
    ਸਥਿਰ constexpr bool ਸ਼ੁੱਧ_2 = true;  // For macro expansion of 'pure' field only

ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CFUNCTION_ID(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, ਦਲੀਲਾਂ) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    ਗਿਣਤੀ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CFUNCTION_NAME(id, function, ਸ਼ੁੱਧ_3, ਦਲੀਲਾਂ) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    bool is_ਸ਼ੁੱਧ() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ ਅਚਲ bool ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CFUNCTION_ਸ਼ੁੱਧ(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, ਦਲੀਲਾਂ) ਸ਼ੁੱਧ_3,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ਸ਼ੁੱਧ)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CFUNCTION_ਸ਼ੁੱਧ
                false  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* ਦਲੀਲਾਂ() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CFUNCTION_ਦਲੀਲਾਂ(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, ਦਲੀਲਾਂ) ਦਲੀਲਾਂ,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ਦਲੀਲਾਂ)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CFUNCTION_ਦਲੀਲਾਂ
                ""  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
};

// Static assert all argument descriptors are well formed
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_CFUNCTION_ਦਲੀਲਾਂ_CHECK(id, ਵਿਧੀ, ਸ਼ੁੱਧ_3, ਦਲੀਲਾਂ) \
    static_assert(validate_ਦਲੀਲਾਂ_Descriptor(ਦਲੀਲਾਂ), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_ਦਲੀਲਾਂ_CHECK)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_CFUNCTION_ਦਲੀਲਾਂ_CHECK

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

ਨਿਜੀ:
    ਬਣਤਰ ਵਸਤੂ_2 final {
        ਗਿਣਤੀ en m_e;  // Statement's enum mnemonic, for checking
        ਅਚਲ ਅੱਖਰ* m_name;  // Statements name, for debugging
    };
    ਸਥਿਰ ਵਸਤੂ_2 s_ਵਸਤੂ_ਅੰਕੜੇ[];

ਜਨਤਕ:
    ਗਿਣਤੀ en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"none", "ctor_var_reset_call"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    bool isNone() ਅਚਲ { ਵਾਪਸ m_e == NONE; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCStmtType& lhs, ਅਚਲ VCStmtType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCStmtType& lhs, VCStmtType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VCStmtType::en lhs, ਅਚਲ VCStmtType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VCStmtType& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    ਗਿਣਤੀ en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ VL_ਸ਼ੁੱਧ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCaseType& lhs, ਅਚਲ VCaseType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCaseType& lhs, VCaseType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VCaseType::en lhs, ਅਚਲ VCaseType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    bool isAssignable() ਅਚਲ { ਵਾਪਸ m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCastable& lhs, ਅਚਲ VCastable& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCastable& lhs, VCastable::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VCastable::en lhs, ਅਚਲ VCastable& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VCastable& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
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
    ਗਿਣਤੀ en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }  // LCOV_EXCL_LINE
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ਅਚਲ ਅੱਖਰ* verilogKwd() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ਅਚਲ ਅੱਖਰ* bin_ਸੈੱਟ_Enum() ਅਚਲ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ BINS_IGNORE: ਵਾਪਸ "VlCovBinKind::KIND_IGNORE";
        ਹਾਲਤ BINS_ILLEGAL: ਵਾਪਸ "VlCovBinKind::KIND_ILLEGAL";
        ਹਾਲਤ BINS_DEFAULT: ਵਾਪਸ "VlCovBinKind::KIND_DEFAULT";
        ਮੂਲ: ਵਾਪਸ "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool bin_Is_ਸਾਧਾਰਨ() ਅਚਲ {
        ਵਾਪਸ m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    ਵਾਪਸ lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        // Shared by option.* and type_option.*
        ਭਾਰ,
        GOAL,
        AT_ਘੱਟ_ਤੋਂ_ਘੱਟ,
        AUTO_BIN_MAX,
        ਪ੍ਰਤੀ_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_ਅੰਕ_PRINT_MISSING,
        CROSS_RETAIN_AUTO_BINS,
        DETECT_OVERLAP,
        GET_INST_COVERAGE,
        // type_option.* only (IEEE 1800-2023 Table 19-3)
        STROBE,
        ਮਿਲਾਓ_INSTANCES,
        DISTRIBUTE_ਪਹਿਲਾ,
        REAL_INTERVAL,
        // Legacy option.* accepted for compatibility
        CROSS_AUTO_BIN_MAX,
        // sentinel - should never appear after parse-time validation
        UNKNOWN
    };
    ਗਿਣਤੀ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"weight",
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
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    ਵਾਪਸ lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    ਗਿਣਤੀ en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(ਪੂਰਨ _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }  // LCOV_EXCL_LINE
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"", "[*]", "[->]", "[=]"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* asciiJson() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};

//######################################################################

class VDirection final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { NONE, ਨਿਵੇਸ਼, OUTPUT, INOUT, REF, CONSTREF };
    ਗਿਣਤੀ en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    string verilogKwd() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    string prettyName() ਅਚਲ { ਵਾਪਸ verilogKwd(); }
    bool isAny() ਅਚਲ { ਵਾਪਸ m_e != NONE; }
    bool isInout() ਅਚਲ { ਵਾਪਸ m_e == INOUT; }
    bool isInoutOrRef() ਅਚਲ { ਵਾਪਸ m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_ਨਿਵੇਸ਼() ਅਚਲ { ਵਾਪਸ m_e == ਨਿਵੇਸ਼; }
    bool isNonOutput() ਅਚਲ {
        ਵਾਪਸ m_e == ਨਿਵੇਸ਼ || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ਅਚਲ { ਵਾਪਸ m_e == OUTPUT; }
    bool isReadOnly() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == ਨਿਵੇਸ਼ || m_e == CONSTREF; }
    bool isWritable() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == REF; }
    bool isConstRef() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == CONSTREF; }
    string traceSigDirection() ਅਚਲ {
        ਜੇ (isInout()) {
            ਵਾਪਸ "VerilatedTraceSigDirection::INOUT";
        } ਨਹੀਂਤਾਂ ਜੇ (isWritable()) {
            ਵਾਪਸ "VerilatedTraceSigDirection::OUTPUT";
        } ਨਹੀਂਤਾਂ ਜੇ (isNonOutput()) {
            ਵਾਪਸ "VerilatedTraceSigDirection::INPUT";
        }
        ਵਾਪਸ "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ਅਚਲ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ NONE: ਵਾਪਸ VAccess::NOCHANGE;
        ਹਾਲਤ ਨਿਵੇਸ਼:
        ਹਾਲਤ CONSTREF: ਵਾਪਸ VAccess::READ;
        ਹਾਲਤ OUTPUT: ਵਾਪਸ VAccess::WRITE;
        ਹਾਲਤ INOUT:
        ਹਾਲਤ REF: ਵਾਪਸ VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VDirection& lhs, ਅਚਲ VDirection& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VDirection& lhs, VDirection::en rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VDirection::en lhs, ਅਚਲ VDirection& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs == rhs.m_e;
}
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VDirection& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    ਗਿਣਤੀ en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    bool ਜੋੜੋ_Newline() ਅਚਲ { ਵਾਪਸ m_e != DT_WRITE; }
    bool needScopeTracking() ਅਚਲ { ਵਾਪਸ m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VDisplayType& lhs, ਅਚਲ VDisplayType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VDisplayType& lhs, VDisplayType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VDisplayType::en lhs, ਅਚਲ VDisplayType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { FILE, VARS, ALL, FLUSH, ਹੱਦਬੰਦੀ_2, OFF, ON };
    ਗਿਣਤੀ en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VDumpCtlType& lhs, ਅਚਲ VDumpCtlType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VDumpCtlType& lhs, VDumpCtlType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VDumpCtlType::en lhs, ਅਚਲ VDumpCtlType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
ਜਨਤਕ:
    // REMEMBER to edit the strings below too
    ਗਿਣਤੀ en : uint8_t {
        // These must be in general -> most specific order, as we sort by it
        // in V3Const::visit AstSenTree
        // Involving a variable
        ET_ਬਦਲਿਆ,  // Value changed
        ET_BOTHEDGE,  // POSEDGE | NEGEDGE (i.e.: 'edge' in Verilog)
        ET_POSEDGE,
        ET_NEGEDGE,
        ET_EVENT,  // VlEventBase::isFired
        // Involving an expression
        ET_TRUE,
        ET_INITIAL_NBA,  // Event that is fired initially and never again
        //
        ET_COMBO,  // Sensitive to all combo inputs to this block
        ET_COMBO_ਤਾਰਾ,  // Sensitive to all combo inputs to this block (from .*)
        ET_HYBRID,  // This is like ET_COMB, but with explicit sensitivity to an expression
        ET_STATIC,  // static variable initializers (runs before 'initial')
        ET_INITIAL,  // 'initial' statements
        ET_FINAL,  // 'final' statements
        ET_NEVER  // Never occurs (optimized away)
    };
    ਗਿਣਤੀ en m_e;
    bool clockedStmt() ਅਚਲ {
        ਸਥਿਰ constexpr bool clocked[] = {
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
        ਵਾਪਸ clocked[m_e];
    }
    bool an_ਕਿਨਾਰਾ() ਅਚਲ { ਵਾਪਸ m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ਅਚਲ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ ET_ਬਦਲਿਆ: ਵਾਪਸ ET_ਬਦਲਿਆ;
        ਹਾਲਤ ET_BOTHEDGE: ਵਾਪਸ ET_BOTHEDGE;
        ਹਾਲਤ ET_POSEDGE: ਵਾਪਸ ET_NEGEDGE;
        ਹਾਲਤ ET_NEGEDGE: ਵਾਪਸ ET_POSEDGE;
        ਮੂਲ: UASSERT_STATIC(0, "Inverting bad edgeType()"); ਵਾਪਸ ET_NEGEDGE;
        }
    }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* verilogKwd() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusive_ਕਿਨਾਰਾ(ਅਚਲ VEdgeType& other) ਅਚਲ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ VEdgeType::ET_POSEDGE:
            ਜੇ (other.m_e == VEdgeType::ET_NEGEDGE) ਵਾਪਸ true;
            ਰੋਕੋ;
        ਹਾਲਤ VEdgeType::ET_NEGEDGE:
            ਜੇ (other.m_e == VEdgeType::ET_POSEDGE) ਵਾਪਸ true;
            ਰੋਕੋ;
        ਮੂਲ: ਰੋਕੋ;
        }
        ਵਾਪਸ false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VEdgeType& lhs, ਅਚਲ VEdgeType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VEdgeType& lhs, VEdgeType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VEdgeType::en lhs, ਅਚਲ VEdgeType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ FOR_EACH_EVAL(macro) \
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
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_EVAL_ID(id, iterated, triggers, ਪਹਿਲਾ, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_EVAL_ID
            ENUM_ਸਿਰਾ
    };
    ਗਿਣਤੀ en m_e;

    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_EVAL_NAME(id, iterated, triggers, ਪਹਿਲਾ, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_EVAL_NAME
                "_ENUM_END"  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    bool isIterated() ਅਚਲ {
        ਸਥਿਰ constexpr bool ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_EVAL_IS_ITERATED(id, iterated, triggers, ਪਹਿਲਾ, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_EVAL_IS_ITERATED
                false  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    bool hasTriggers() ਅਚਲ {
        ਸਥਿਰ constexpr bool ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, ਪਹਿਲਾ, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_EVAL_HAS_TRIGGERS
                false  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    bool ਪਹਿਲਾ_Iteration() ਅਚਲ {
        ਸਥਿਰ constexpr bool ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_EVAL_ਪਹਿਲਾ(id, iterated, triggers, ਪਹਿਲਾ, slow) ਪਹਿਲਾ,
            FOR_EACH_EVAL(VL_EVAL_ਪਹਿਲਾ)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_EVAL_ਪਹਿਲਾ
                false  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
    bool slow() ਅਚਲ {
        ਸਥਿਰ constexpr bool ਮੁੱਲਾਂ[] = {
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_EVAL_SLOW(id, iterated, triggers, ਪਹਿਲਾ, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ VL_EVAL_SLOW
                false  //
        };
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }

    // Short name
    std::string ਟੈਗ() ਅਚਲ { ਵਾਪਸ VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ਅਚਲ { ਵਾਪਸ "_eval_" + ਟੈਗ(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string eval_ਵਿਧੀ() ਅਚਲ { ਵਾਪਸ "eval" + capitalized_ਟੈਗ(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ਅਚਲ { ਵਾਪਸ "_eval_dump_triggers__" + ਟੈਗ(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dump_Triggers_ਵਿਧੀ() ਅਚਲ { ਵਾਪਸ "dumpTriggers" + capitalized_ਟੈਗ(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}
    ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }

ਨਿਜੀ:
    std::string capitalized_ਟੈਗ() ਅਚਲ {
        ਅਚਲ std::string name = ascii();
        ਵਾਪਸ name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#ਪਰਿਭਾਸ਼ਾ_ਰੱਦ FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    // Is a node type compatible with the declaration
    bool is_ਗੰਢ_Compatible(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ;
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VFwdType& lhs, ਅਚਲ VFwdType& rhs) { ਵਾਪਸ lhs.m_e == rhs.m_e; }
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VFwdType& lhs, VFwdType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VFwdType::en lhs, ਅਚਲ VFwdType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VFwdType& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    ਸਥਿਰ uint64_t s_cachedCntGbl;  // Global computed count

ਜਨਤਕ:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() ਅਚਲ { ਵਾਪਸ m_cachedCnt == s_cachedCntGbl; }
    bool get() ਅਚਲ { ਵਾਪਸ m_state; }
    ਖਾਲੀ ਸੈੱਟ(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    ਖਾਲੀ clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ਸਥਿਰ ਖਾਲੀ clear_Cache_ਰੁੱਖ() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    ਗਿਣਤੀ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* verilogKwd() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"join", "join_any", "join_none"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    bool join() ਅਚਲ { ਵਾਪਸ m_e == JOIN; }
    bool joinAny() ਅਚਲ { ਵਾਪਸ m_e == JOIN_ANY; }
    bool joinNone() ਅਚਲ { ਵਾਪਸ m_e == JOIN_NONE; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VJoinType& lhs, ਅਚਲ VJoinType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VJoinType& lhs, VJoinType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VJoinType::en lhs, ਅਚਲ VJoinType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VJoinType& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    bool isNone() ਅਚਲ { ਵਾਪਸ m_e == NONE; }
    bool isAutomatic() ਅਚਲ { ਵਾਪਸ m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ਅਚਲ { ਵਾਪਸ m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ਅਚਲ { ਵਾਪਸ m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ਅਚਲ {
        ਚੋਣ (m_e) {
        ਹਾਲਤ AUTOMATIC_EXPLICIT: ਵਾਪਸ AUTOMATIC_IMPLICIT;
        ਹਾਲਤ STATIC_EXPLICIT: ਵਾਪਸ STATIC_IMPLICIT;
        ਮੂਲ: ਵਾਪਸ m_e;
        }
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VLifetime& lhs, ਅਚਲ VLifetime& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VLifetime& lhs, VLifetime::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VLifetime::en lhs, ਅਚਲ VLifetime& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VLifetime& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
ਜਨਤਕ:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            ਪੂਰਨ m_ਸੂਚਕਾਂਕ;  // Current index
            ਅਚਲ ਪੂਰਨ m_inc;  // Step, +1 or -1
        ਜਨਤਕ:
            Iterator(ਪੂਰਨ ਸੂਚਕਾਂਕ, ਪੂਰਨ inc)
                : m_ਸੂਚਕਾਂਕ{ਸੂਚਕਾਂਕ}
                , m_inc{inc} {}
            ਪੂਰਨ ਸੰਚਾਲਕ*() ਅਚਲ { ਵਾਪਸ m_ਸੂਚਕਾਂਕ; }
            Iterator& ਸੰਚਾਲਕ++() {
                m_ਸੂਚਕਾਂਕ += m_inc;
                ਵਾਪਸ *this;
            }
            bool ਸੰਚਾਲਕ!=(ਅਚਲ Iterator& that) ਅਚਲ { ਵਾਪਸ m_ਸੂਚਕਾਂਕ != that.m_ਸੂਚਕਾਂਕ; }
        };
        ਅਚਲ ਪੂਰਨ m_begin;  // First index
        ਅਚਲ ਪੂਰਨ m_ਸਿਰਾ;  // One past the last index
        ਅਚਲ ਪੂਰਨ m_inc;  // Step, +1 or -1
    ਜਨਤਕ:
        Sequence(ਪੂਰਨ ਪਹਿਲਾ, ਪੂਰਨ ਆਖਰੀ, ਪੂਰਨ inc)
            : m_begin{ਪਹਿਲਾ}
            , m_ਸਿਰਾ{ਆਖਰੀ + inc}
            , m_inc{inc} {}
        Iterator begin() ਅਚਲ { ਵਾਪਸ Iterator{m_begin, m_inc}; }
        Iterator ਸਿਰਾ() ਅਚਲ { ਵਾਪਸ Iterator{m_ਸਿਰਾ, m_inc}; }
    };

    ਪੂਰਨ m_ਖੱਬਾ = 0;  // Left side of range (pre-':')
    ਪੂਰਨ m_ਸੱਜਾ = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool ਸੰਚਾਲਕ==(ਅਚਲ VNumRange& rhs) ਅਚਲ {
        ਵਾਪਸ m_ਖੱਬਾ == rhs.m_ਖੱਬਾ && m_ਸੱਜਾ == rhs.m_ਸੱਜਾ && m_ranged == rhs.m_ranged;
    }
    bool ਸੰਚਾਲਕ<(ਅਚਲ VNumRange& rhs) ਅਚਲ {
        ਜੇ ((m_ਖੱਬਾ < rhs.m_ਖੱਬਾ)) ਵਾਪਸ true;
        ਜੇ (!(m_ਖੱਬਾ == rhs.m_ਖੱਬਾ)) ਵਾਪਸ false;  // lhs > rhs
        ਜੇ ((m_ਸੱਜਾ < rhs.m_ਸੱਜਾ)) ਵਾਪਸ true;
        ਜੇ (!(m_ਸੱਜਾ == rhs.m_ਸੱਜਾ)) ਵਾਪਸ false;  // lhs > rhs
        ਜੇ ((m_ranged < rhs.m_ranged)) ਵਾਪਸ true;
        ਜੇ (!(m_ranged == rhs.m_ranged)) ਵਾਪਸ false;  // lhs > rhs
        ਵਾਪਸ false;
    }
    //
    VNumRange() = ਮੂਲ;
    VNumRange(ਪੂਰਨ hi, ਪੂਰਨ lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(ਪੂਰਨ ਖੱਬਾ, ਪੂਰਨ ਸੱਜਾ)
        : m_ਖੱਬਾ{ਖੱਬਾ}
        , m_ਸੱਜਾ{ਸੱਜਾ}
        , m_ranged{true} {}
    ~VNumRange() = ਮੂਲ;
    // MEMBERS
    ਖਾਲੀ init(ਪੂਰਨ hi, ਪੂਰਨ lo, bool ascending) {
        ਜੇ (lo > hi) std::ਅਦਲਾ_ਬਦਲੀ(hi, lo);
        m_ਖੱਬਾ = ascending ? lo : hi;
        m_ਸੱਜਾ = ascending ? hi : lo;
        m_ranged = true;
    }
    ਪੂਰਨ ਖੱਬਾ() ਅਚਲ { ਵਾਪਸ m_ਖੱਬਾ; }
    ਪੂਰਨ ਸੱਜਾ() ਅਚਲ { ਵਾਪਸ m_ਸੱਜਾ; }
    ਪੂਰਨ hi() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_ਖੱਬਾ > m_ਸੱਜਾ ? m_ਖੱਬਾ : m_ਸੱਜਾ;
    }  // How to show a declaration
    ਪੂਰਨ lo() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_ਖੱਬਾ > m_ਸੱਜਾ ? m_ਸੱਜਾ : m_ਖੱਬਾ;
    }  // How to show a declaration
    ਪੂਰਨ ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ_Inc() ਅਚਲ { ਵਾਪਸ ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ() ਅਚਲ { ਵਾਪਸ {m_ਖੱਬਾ, m_ਸੱਜਾ, ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_ਸੱਜਾ_ਮੰਜ਼ਿਲ_ਖੱਬਾ() ਅਚਲ { ਵਾਪਸ {m_ਸੱਜਾ, m_ਖੱਬਾ, -ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seq_Lo_ਮੰਜ਼ਿਲ_Hi() ਅਚਲ { ਵਾਪਸ {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seq_Hi_ਮੰਜ਼ਿਲ_Lo() ਅਚਲ { ਵਾਪਸ {hi(), lo(), -1}; }
    ਪੂਰਨ elements() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ hi() - lo() + 1; }
    bool ranged() ਅਚਲ { ਵਾਪਸ m_ranged; }
    bool ascending() ਅਚਲ { ਵਾਪਸ m_ਖੱਬਾ < m_ਸੱਜਾ; }
    ਪੂਰਨ hiMaxSelect() ਅਚਲ {
        ਵਾਪਸ (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ਅਚਲ {
        ਜੇ (ranged()) {
            ਵਾਪਸ "["s + std::ਮੰਜ਼ਿਲ_string(ਖੱਬਾ()) + ":" + std::ਮੰਜ਼ਿਲ_string(ਸੱਜਾ()) + "]";
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ "[norg]";
        }
    }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ { str << ascii(); }
};
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VNumRange& rhs) {
    rhs.dump(os);
    ਵਾਪਸ os;
}

// ######################################################################

class VPragmaType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        COVERAGE_ਖੰਡ_OFF,
        HIER_ਖੰਡ,
        HIER_PARAMS,
        INLINE_MODULE,
        NO_INLINE_MODULE,
        NO_INLINE_TASK,
        ਜਨਤਕ_MODULE,
        ਜਨਤਕ_TASK,
        TIMEUNIT_ਸੈੱਟ,
        UNROLL_DISABLE,
        UNROLL_ਭਰਿਆ,
        ਭਰਿਆ_CASE,
        PARALLEL_CASE,
        VERILATOR_LIB,
        ENUM_ਮਾਪ
    };
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {
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
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VPragmaType& lhs, ਅਚਲ VPragmaType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VPragmaType& lhs, VPragmaType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VPragmaType::en lhs, ਅਚਲ VPragmaType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VPragmaType& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    bool isRandomizable() ਅਚਲ { ਵਾਪਸ m_e != NONE; }
    bool isRand() ਅਚਲ { ਵਾਪਸ m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ਅਚਲ { ਵਾਪਸ m_e == RAND_CYCLIC; }
};
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VRandAttr& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelf_ਸੂਚਕ_ਪਾਠ final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ਸਥਿਰ ਅਚਲ std::shared_ptr<ਅਚਲ string> s_emptyp;  // Holds ""
    ਸਥਿਰ ਅਚਲ std::shared_ptr<ਅਚਲ string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ਅਚਲ string> m_strp;

ਜਨਤਕ:
    // CONSTRUCTORS
    class ਖਾਲੀ_ਥਾਵਾਂ_2 {};  // for creator type-overload selection
    explicit VSelf_ਸੂਚਕ_ਪਾਠ(ਖਾਲੀ_ਥਾਵਾਂ_2)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelf_ਸੂਚਕ_ਪਾਠ(This)
        : m_strp{s_thisp} {}
    VSelf_ਸੂਚਕ_ਪਾਠ(This, ਅਚਲ string& ਖੇਤਰ)
        : m_strp{std::make_shared<ਅਚਲ string>("this->" + ਖੇਤਰ)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelf_ਸੂਚਕ_ਪਾਠ(VlSyms, ਅਚਲ string& ਖੇਤਰ)
        : m_strp{std::make_shared<ਅਚਲ string>("(&vlSymsp->vlInst." + ਖੇਤਰ + ')')} {}

    // METHODS
    bool is_ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_strp == s_emptyp; }
    bool isVlSym() ਅਚਲ { ਵਾਪਸ m_strp->ਲੱਭੋ("vlSymsp") != string::npos; }
    bool hasThis() ਅਚਲ { ਵਾਪਸ m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ਅਚਲ;
    ਸਥਿਰ string replaceThis(bool useSelfForThis, ਅਚਲ string& ਪਾਠ);
    ਅਚਲ std::string& asString() ਅਚਲ { ਵਾਪਸ *m_strp; }
    bool ਸੰਚਾਲਕ==(ਅਚਲ VSelf_ਸੂਚਕ_ਪਾਠ& other) ਅਚਲ { ਵਾਪਸ *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    ਸਥਿਰ VSigning fromBool(bool isSigned) {  // Factory method
        ਵਾਪਸ isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    bool isSigned() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == SIGNED; }
    bool isNosign() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VSigning& lhs, ਅਚਲ VSigning& rhs) { ਵਾਪਸ lhs.m_e == rhs.m_e; }
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VSigning& lhs, VSigning::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VSigning::en lhs, ਅਚਲ VSigning& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VSigning& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

class VStrength final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    ਗਿਣਤੀ en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VStrength& lhs, ਅਚਲ VStrength& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VStrength& lhs, VStrength::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VStrength::en lhs, ਅਚਲ VStrength& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VStrength& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    ਗਿਣਤੀ en m_e;
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
};

//######################################################################

class VTrace_ਅਗੇਤਰ_Type final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    ਗਿਣਤੀ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTrace_ਅਗੇਤਰ_Type(en _e)
        : m_e{_e} {}
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VTrace_ਅਗੇਤਰ_Type& lhs, ਅਚਲ VTrace_ਅਗੇਤਰ_Type& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VTrace_ਅਗੇਤਰ_Type& lhs, VTrace_ਅਗੇਤਰ_Type::en rhs) {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VTrace_ਅਗੇਤਰ_Type::en lhs, ਅਚਲ VTrace_ਅਗੇਤਰ_Type& rhs) {
    ਵਾਪਸ lhs == rhs.m_e;
}
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VTrace_ਅਗੇਤਰ_Type& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

//######################################################################

class VTraceType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        ਭਰਿਆ,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    ਗਿਣਤੀ en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"CONSTANT", "FULL", "CHANGE"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਅਚਲ ਅੱਖਰ* func_ਅਗੇਤਰ() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"trace_const", "trace_full", "trace_chg"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VTraceType& lhs, ਅਚਲ VTraceType& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VTraceType& lhs, VTraceType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VTraceType::en lhs, ਅਚਲ VTraceType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VTraceType& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

//######################################################################

class VUseType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    ਗਿਣਤੀ en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    bool ਸ਼ਾਮਲ_ਹੈ_Any(VUseType other) ਅਚਲ { ਵਾਪਸ m_e & other.m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        ਵਾਪਸ ਨਾਂ[m_e - 1];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VUseType& lhs, ਅਚਲ VUseType& rhs) { ਵਾਪਸ lhs.m_e == rhs.m_e; }
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VUseType& lhs, VUseType::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VUseType::en lhs, ਅਚਲ VUseType& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
constexpr VUseType::en ਸੰਚਾਲਕ|(VUseType::en lhs, VUseType::en rhs) {
    ਵਾਪਸ VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en ਸੰਚਾਲਕ&(VUseType::en lhs, VUseType::en rhs) {
    ਵਾਪਸ VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VUseType& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

class VVarType final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
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
        ਪੋਰਟ,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    ਗਿਣਤੀ en m_e;
    VVarType() VL_MT_ਸੁਰੱਖਿਅਤ : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_ਸੁਰੱਖਿਅਤ : m_e{_e} {}
    explicit VVarType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    bool isParam() ਅਚਲ { ਵਾਪਸ m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ਅਚਲ {
        ਵਾਪਸ (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == ਪੋਰਟ || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ਅਚਲ {
        ਵਾਪਸ (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ਅਚਲ { ਵਾਪਸ (m_e == TRIOR); }
    bool isWiredNet() ਅਚਲ { ਵਾਪਸ (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ਅਚਲ {  // In Verilog, always ok in SystemVerilog
        ਵਾਪਸ (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == ਪੋਰਟ || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ਅਚਲ {
        ਵਾਪਸ (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool is_ਆਰਜ਼ੀ() ਅਚਲ {
        ਵਾਪਸ (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ਅਚਲ {
        ਵਾਪਸ (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == ਪੋਰਟ
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ਅਚਲ ਅੱਖਰ* trace_Sig_ਕਿਸਮ() ਅਚਲ {
        // VerilatedTraceSigKind to used in trace signal declaration
        ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ lut[] = {
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
        ਵਾਪਸ lut[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VVarType& lhs, ਅਚਲ VVarType& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VVarType& lhs, VVarType::en rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VVarType::en lhs, ਅਚਲ VVarType& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs == rhs.m_e;
}
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VVarType& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_ਕੁੰਜੀ final {
ਜਨਤਕ:
    ਅਚਲ ਪੂਰਨ m_ਚੌੜਾਈ;  // From AstNodeDType: Bit width of operation
    ਅਚਲ ਪੂਰਨ m_ਚੌੜਾਈ_Min;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ਅਚਲ VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ਅਚਲ VSigning m_numeric;  // From AstNodeDType: Node is signed
    ਅਚਲ VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool ਸੰਚਾਲਕ==(ਅਚਲ VBasic_Type_ਕੁੰਜੀ& rhs) ਅਚਲ {
        ਵਾਪਸ m_ਚੌੜਾਈ == rhs.m_ਚੌੜਾਈ && m_ਚੌੜਾਈ_Min == rhs.m_ਚੌੜਾਈ_Min && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool ਸੰਚਾਲਕ<(ਅਚਲ VBasic_Type_ਕੁੰਜੀ& rhs) ਅਚਲ {
        ਜੇ ((m_ਚੌੜਾਈ < rhs.m_ਚੌੜਾਈ)) ਵਾਪਸ true;
        ਜੇ (!(m_ਚੌੜਾਈ == rhs.m_ਚੌੜਾਈ)) ਵਾਪਸ false;  // lhs > rhs
        ਜੇ ((m_ਚੌੜਾਈ_Min < rhs.m_ਚੌੜਾਈ_Min)) ਵਾਪਸ true;
        ਜੇ (!(m_ਚੌੜਾਈ_Min == rhs.m_ਚੌੜਾਈ_Min)) ਵਾਪਸ false;  // lhs > rhs
        ਜੇ ((m_numeric < rhs.m_numeric)) ਵਾਪਸ true;
        ਜੇ (!(m_numeric == rhs.m_numeric)) ਵਾਪਸ false;  // lhs > rhs
        ਜੇ ((m_keyword < rhs.m_keyword)) ਵਾਪਸ true;
        ਜੇ (!(m_keyword == rhs.m_keyword)) ਵਾਪਸ false;  // lhs > rhs
        ਜੇ ((m_nrange < rhs.m_nrange)) ਵਾਪਸ true;
        ਜੇ (!(m_nrange == rhs.m_nrange)) ਵਾਪਸ false;  // lhs > rhs
        ਵਾਪਸ false;
    }
    VBasic_Type_ਕੁੰਜੀ(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric, VBasicDTypeKwd kwd,
                  ਅਚਲ VNumRange& nrange)
        : m_ਚੌੜਾਈ{ਚੌੜਾਈ}
        , m_ਚੌੜਾਈ_Min{ਚੌੜਾਈ_Min}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_ਕੁੰਜੀ() = ਮੂਲ;
};

#ਜੇ_ਅੰਤ  // Guard
