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
#അന്തര്ഭാവ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
പൊതു:
    // IEEE 1800-2023 16.12.14 property abort operators.
    എണ്ണൽ en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    എണ്ണൽ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        തിരികെ names[m_e];
    }
    bool isAccept() സ്ഥിരം { തിരികെ m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"RD", "WR", "RW", "--"};
        തിരികെ names[m_e];
    }
    സ്ഥിരം അക്ഷരം* arrow() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        തിരികെ names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    VAccess invert() സ്ഥിരം {
        തിരികെ (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() സ്ഥിരം { തിരികെ m_e == READ; }  // False with READWRITE
    bool isWriteOnly() സ്ഥിരം { തിരികെ m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() സ്ഥിരം { തിരികെ m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() സ്ഥിരം { തിരികെ m_e == WRITE || m_e == READWRITE; }
    bool isRW() സ്ഥിരം { തിരികെ m_e == READWRITE; }
};
constexpr bool operator==(സ്ഥിരം VAccess& lhs, സ്ഥിരം VAccess& rhs) { തിരികെ lhs.m_e == rhs.m_e; }
constexpr bool operator==(സ്ഥിരം VAccess& lhs, VAccess::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, സ്ഥിരം VAccess& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VAccess& rhs) { തിരികെ os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
പൊതു:
    എണ്ണൽ en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    എണ്ണൽ en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VAlwaysKwd& lhs, സ്ഥിരം VAlwaysKwd& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, സ്ഥിരം VAlwaysKwd& rhs) { തിരികെ lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
പൊതു:
    // IEEE 1800-2023 Table 20-5
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        // IEEE 1800-2023 20.11
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"",
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
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VAssertCtlType& lhs, സ്ഥിരം VAssertCtlType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, സ്ഥിരം VAssertCtlType& rhs) {
    തിരികെ lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
പൊതു:
    // IEEE 1800-2023 Table 20-7
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() സ്ഥിരം {
        std::stringstream types;
        എങ്കിൽ (m_e == INTERNAL)
            types << "INTERNAL ";
        അല്ലെങ്കിൽ {
            എങ്കിൽ (m_e & ASSERT) types << "ASSERT ";
            എങ്കിൽ (m_e & COVER) types << "COVER ";
            എങ്കിൽ (m_e & ASSUME) types << "ASSUME ";
            എങ്കിൽ (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            എങ്കിൽ (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            എങ്കിൽ (m_e & INTRINSIC) types << "INTRINSIC ";
            എങ്കിൽ (m_e & RESTRICT) types << "RESTRICT ";
        }
        സ്ഥിരം string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        തിരികെ str.substr(0, str.size() - 1);
    }
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
};
constexpr bool operator==(സ്ഥിരം VAssertDirectiveType& lhs, സ്ഥിരം VAssertDirectiveType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, സ്ഥിരം VAssertDirectiveType& rhs) {
    തിരികെ lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    തിരികെ VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
പൊതു:
    // IEEE 1800-2023 Table 20-6
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) സ്ഥിരം { തിരികെ m_e & other.m_e; }
    string ascii() സ്ഥിരം {
        std::stringstream types;
        എങ്കിൽ (m_e == INTERNAL)
            types << "INTERNAL ";
        അല്ലെങ്കിൽ {
            എങ്കിൽ (m_e & CONCURRENT) types << "CONCURRENT ";
            എങ്കിൽ (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            എങ്കിൽ (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            എങ്കിൽ (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            എങ്കിൽ (m_e & EXPECT) types << "EXPECT ";
            എങ്കിൽ (m_e & UNIQUE) types << "UNIQUE ";
            എങ്കിൽ (m_e & UNIQUE0) types << "UNIQUE0 ";
            എങ്കിൽ (m_e & PRIORITY) types << "PRIORITY ";
        }
        സ്ഥിരം string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        തിരികെ str.substr(0, str.size() - 1);
    }
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
};
constexpr bool operator==(സ്ഥിരം VAssertType& lhs, സ്ഥിരം VAssertType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VAssertType& lhs, VAssertType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, സ്ഥിരം VAssertType& rhs) { തിരികെ lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    തിരികെ VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    എണ്ണൽ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"default", "weak", "strong"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VPropStrength& lhs, സ്ഥിരം VPropStrength& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VPropStrength& lhs, VPropStrength::en rhs) {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator!=(സ്ഥിരം VPropStrength& lhs, VPropStrength::en rhs) {
    തിരികെ lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
പൊതു:
    // clang-format off
    എണ്ണൽ en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_സംഖ്യ,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_മുകളിലെ,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_ഇടത്,                       // V3Width processes
        DIM_താഴെയുള്ള,                        // V3Width processes
        DIM_വലത്,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_അക്കം,                       // V3Width processes
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
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        // clang-format off
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {
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
        തിരികെ names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() സ്ഥിരം {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ DIM_BITS:
        അവസ്ഥ DIM_BITS_OR_സംഖ്യ:
        അവസ്ഥ DIM_DIMENSIONS:
        അവസ്ഥ DIM_മുകളിലെ:
        അവസ്ഥ DIM_INCREMENT:
        അവസ്ഥ DIM_ഇടത്:
        അവസ്ഥ DIM_താഴെയുള്ള:
        അവസ്ഥ DIM_വലത്:
        അവസ്ഥ DIM_SIZE:
        അവസ്ഥ DIM_UNPK_DIMENSIONS:
        അവസ്ഥ TYPEID:
        അവസ്ഥ TYPENAME: തിരികെ true;
        സ്ഥിരസ്ഥിതി: തിരികെ false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
};
constexpr bool operator==(സ്ഥിരം VAttrType& lhs, സ്ഥിരം VAttrType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VAttrType& lhs, VAttrType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, സ്ഥിരം VAttrType& rhs) { തിരികെ lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

പൊതു:
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
    ശൂന്യം combine(സ്ഥിരം VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() സ്ഥിരം { തിരികെ m_extends | m_final | m_initial; }
    bool isExtends() സ്ഥിരം { തിരികെ m_extends; }
    bool isFinal() സ്ഥിരം { തിരികെ m_final; }
    bool isInitial() സ്ഥിരം { തിരികെ m_initial; }
    string ascii() സ്ഥിരം {
        string out;
        എങ്കിൽ (m_initial) out = VString::dot(out, " ", "initial");
        എങ്കിൽ (m_extends) out = VString::dot(out, " ", "extends");
        എങ്കിൽ (m_final) out = VString::dot(out, " ", "final");
        തിരികെ out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
പൊതു:
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം VL_MT_SAFE {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"%E-unk",
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
        തിരികെ names[m_e];
    }
    സ്ഥിരം അക്ഷരം* typeName() സ്ഥിരം VL_MT_SAFE {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ LOGIC2STATE: തിരികെ VBasicDTypeKwd{LOGIC}.ascii();
        അവസ്ഥ INTEGER2STATE: തിരികെ VBasicDTypeKwd{INTEGER}.ascii();
        അവസ്ഥ TIME2STATE: തിരികെ VBasicDTypeKwd{TIME}.ascii();
        സ്ഥിരസ്ഥിതി: തിരികെ ascii();
        }
    }
    സ്ഥിരം അക്ഷരം* dpiType() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"%E-unk",
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
        തിരികെ names[m_e];
    }
    സ്ഥിതം ശൂന്യം selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(പൂർണ്ണസംഖ്യ _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    പൂർണ്ണസംഖ്യ width() സ്ഥിരം {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ BIT: തിരികെ 1;  // scalar, can't bit extract unless ranged
        അവസ്ഥ BYTE: തിരികെ 8;
        അവസ്ഥ CHANDLE: തിരികെ 64;
        അവസ്ഥ NULLTYPE: തിരികെ 1;  // Historical - bit was used instead of NULLTYPE
        അവസ്ഥ EVENT: തിരികെ 1;
        അവസ്ഥ INT: തിരികെ 32;
        അവസ്ഥ INTEGER: തിരികെ 32;
        അവസ്ഥ INTEGER2STATE: തിരികെ 32;
        അവസ്ഥ LOGIC: തിരികെ 1;  // scalar, can't bit extract unless ranged
        അവസ്ഥ LOGIC2STATE: തിരികെ 1;  // scalar, can't bit extract unless ranged
        അവസ്ഥ LONGINT: തിരികെ 64;
        അവസ്ഥ DOUBLE: തിരികെ 64;  // opaque
        അവസ്ഥ SHORTINT: തിരികെ 16;
        അവസ്ഥ TIME: തിരികെ 64;
        അവസ്ഥ TIME2STATE: തിരികെ 64;
        അവസ്ഥ STRING: തിരികെ 64;  // opaque  // Just the pointer, for today
        അവസ്ഥ SCOPEPTR: തിരികെ 0;  // opaque
        അവസ്ഥ CHARPTR: തിരികെ 0;  // opaque
        അവസ്ഥ MTASKSTATE: തിരികെ 0;  // opaque
        അവസ്ഥ DELAY_SCHEDULER: തിരികെ 0;  // opaque
        അവസ്ഥ TRIGGER_SCHEDULER: തിരികെ 0;  // opaque
        അവസ്ഥ DYNAMIC_TRIGGER_SCHEDULER: തിരികെ 0;  // opaque
        അവസ്ഥ FORK_SYNC: തിരികെ 0;  // opaque
        അവസ്ഥ PROCESS_REFERENCE: തിരികെ 0;  // opaque
        അവസ്ഥ RANDOM_GENERATOR: തിരികെ 0;  // opaque
        അവസ്ഥ RANDOM_STDGENERATOR: തിരികെ 0;  // opaque
        അവസ്ഥ COVERGROUP_INSTHANDLE: തിരികെ 0;  // opaque
        അവസ്ഥ UINT32: തിരികെ 32;
        അവസ്ഥ UINT64: തിരികെ 64;
        സ്ഥിരസ്ഥിതി: തിരികെ 0;
        }
    }
    bool isSigned() സ്ഥിരം {
        തിരികെ m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() സ്ഥിരം {
        തിരികെ m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() സ്ഥിരം {
        തിരികെ m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() സ്ഥിരം {  // Otherwise initializes to X
        തിരികെ (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() സ്ഥിരം {  // Enum increment supported
        തിരികെ (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() സ്ഥിരം { തിരികെ m_e == BIT; }
    bool isBitLogic() സ്ഥിരം {  // Bit/logic vector types; can form a packed array
        തിരികെ (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() സ്ഥിരം {  // Can add "unsigned" to DPI
        തിരികെ (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() സ്ഥിരം {  // Uses standard C layout, for DPI runtime access
        തിരികെ (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() സ്ഥിരം VL_MT_SAFE {  // IE not a simple number we can bit optimize
        തിരികെ (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == CHANDLE; }
    bool isDouble() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == DOUBLE; }
    bool isEvent() സ്ഥിരം { തിരികെ m_e == EVENT; }
    bool isString() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == STRING; }
    bool isMTaskState() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == MTASKSTATE; }
    bool isSameish(സ്ഥിരം VBasicDTypeKwd& other) സ്ഥിരം VL_MT_SAFE {
        തിരഞ്ഞെടുപ്പ് (other.m_e) {
        അവസ്ഥ LOGIC:
        അവസ്ഥ LOGIC2STATE:
        അവസ്ഥ LOGIC_IMPLICIT: തിരികെ m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        അവസ്ഥ INTEGER:
        അവസ്ഥ INTEGER2STATE: തിരികെ m_e == INTEGER || m_e == INTEGER2STATE;
        അവസ്ഥ TIME:
        അവസ്ഥ TIME2STATE: തിരികെ m_e == TIME || m_e == TIME2STATE;
        സ്ഥിരസ്ഥിതി: തിരികെ m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() സ്ഥിരം VL_MT_SAFE {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ BIT:
        അവസ്ഥ BYTE:
        അവസ്ഥ CHANDLE:
        അവസ്ഥ INT:
        അവസ്ഥ INTEGER:
        അവസ്ഥ INTEGER2STATE:
        അവസ്ഥ LOGIC:
        അവസ്ഥ LOGIC2STATE:
        അവസ്ഥ LONGINT:
        അവസ്ഥ DOUBLE:
        അവസ്ഥ SHORTINT:
        അവസ്ഥ SCOPEPTR:
        അവസ്ഥ CHARPTR:
        അവസ്ഥ UINT32:
        അവസ്ഥ UINT64: തിരികെ true;
        സ്ഥിരസ്ഥിതി: തിരികെ false;
        }
    }

    സ്ഥിരം അക്ഷരം* traceSigType() സ്ഥിരം {
        // VerilatedTraceSigType to used in trace signal declaration
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം lut[] = {
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
        static_assert(വലുപ്പം(lut) / വലുപ്പം(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        തിരികെ lut[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VBasicDTypeKwd& lhs, സ്ഥിരം VBasicDTypeKwd& rhs) VL_MT_SAFE {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, സ്ഥിരം VBasicDTypeKwd& rhs) VL_MT_SAFE {
    തിരികെ lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
പൊതു:
    എണ്ണൽ en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    എണ്ണൽ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"FALSE", "TRUE", "UNK"};
        തിരികെ names[m_e];
    }
    bool isKnown() സ്ഥിരം { തിരികെ m_e != BU_UNKNOWN; }
    bool trueKnown() സ്ഥിരം { തിരികെ m_e == BU_TRUE; }
    ശൂന്യം setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(സ്ഥിരം VBoolOrUnknown& lhs, സ്ഥിരം VBoolOrUnknown& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, സ്ഥിരം VBoolOrUnknown& rhs) {
    തിരികെ lhs == rhs.m_e;
}
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VBoolOrUnknown& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
പൊതു:
    എണ്ണൽ en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    എണ്ണൽ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    bool unknown() സ്ഥിരം { തിരികെ m_e == BP_UNKNOWN; }
    bool likely() സ്ഥിരം { തിരികെ m_e == BP_LIKELY; }
    bool unlikely() സ്ഥിരം { തിരികെ m_e == BP_UNLIKELY; }
    VBranchPred invert() സ്ഥിരം {
        എങ്കിൽ (m_e == BP_UNLIKELY) തിരികെ BP_LIKELY;
        എങ്കിൽ (m_e == BP_LIKELY) തിരികെ BP_UNLIKELY;
        തിരികെ m_e;
    }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        തിരികെ names[m_e];
    }
    സ്ഥിരം അക്ഷരം* asciiShort() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"", "L", "!L"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VBranchPred& lhs, സ്ഥിരം VBranchPred& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VBranchPred& lhs, VBranchPred::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, സ്ഥിരം VBranchPred& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VBranchPred& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
സ്ഥിതം constexpr bool validateArgsDescriptor(സ്ഥിരം അക്ഷരം* descrp) {
    // Is "TODO"
    എങ്കിൽ (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        തിരികെ true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    വേണ്ടി (സ്ഥിരം അക്ഷരം* cp = descrp; *cp; ++cp) {
        എങ്കിൽ (*cp == '+') തിരികെ cp != descrp && !cp[1];
        എങ്കിൽ (*cp != 'r' && *cp != 'w' && *cp != 'm') തിരികെ false;
    }
    തിരികെ true;
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
    macro(ARRAY_എടുക്കുക_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_എടുക്കുക_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_തള്ളുക_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_തള്ളുക_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_മൂല്യം,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_മൂല്യം_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_മൂല്യം_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_മൂല്യം_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_മൂല്യം_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_മൂല്യം_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_മൂല്യം_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_മൂല്യം_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_മൂല്യം_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_മൂല്യം_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_മൂല്യം_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_തിരുകുക,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_എടുക്കുക,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_എടുക്കുക_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_തള്ളുക,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_തള്ളുക_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    സ്ഥിതം constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    സ്ഥിതം constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

പൊതു:
    എണ്ണൽ en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    എണ്ണൽ en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം VL_PURE {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം മൂല്യങ്ങൾ[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    bool isPure() സ്ഥിരം VL_PURE {
        സ്ഥിതം constexpr bool മൂല്യങ്ങൾ[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    bool isClean() സ്ഥിരം VL_PURE {
        സ്ഥിതം constexpr bool മൂല്യങ്ങൾ[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    സ്ഥിരം അക്ഷരം* args() സ്ഥിരം VL_PURE {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം മൂല്യങ്ങൾ[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    // Return array method for given name
    സ്ഥിതം VCMethod arrayMethod(സ്ഥിരം string& name);
};
constexpr bool operator==(സ്ഥിരം VCMethod& lhs, സ്ഥിരം VCMethod& rhs) { തിരികെ lhs.m_e == rhs.m_e; }
constexpr bool operator==(സ്ഥിരം VCMethod& lhs, VCMethod::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, സ്ഥിരം VCMethod& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VCMethod& rhs) {
    തിരികെ os << rhs.ascii();
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
    സ്ഥിതം constexpr bool PURE = true;  // For macro expansion of 'pure' field only

പൊതു:
    എണ്ണൽ en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    എണ്ണൽ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം VL_PURE {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം മൂല്യങ്ങൾ[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    bool isPure() സ്ഥിരം VL_PURE {
        സ്ഥിതം സ്ഥിരം bool മൂല്യങ്ങൾ[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    സ്ഥിരം അക്ഷരം* args() സ്ഥിരം VL_PURE {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം മൂല്യങ്ങൾ[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
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
പൊതു:
    എണ്ണൽ en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

സ്വകാര്യ:
    ഘടന Item final {
        എണ്ണൽ en m_e;  // Statement's enum mnemonic, for checking
        സ്ഥിരം അക്ഷരം* m_name;  // Statements name, for debugging
    };
    സ്ഥിതം Item s_itemData[];

പൊതു:
    എണ്ണൽ en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം VL_PURE {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"none", "ctor_var_reset_call"};
        തിരികെ names[m_e];
    }
    bool isNone() സ്ഥിരം { തിരികെ m_e == NONE; }
};
constexpr bool operator==(സ്ഥിരം VCStmtType& lhs, സ്ഥിരം VCStmtType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VCStmtType& lhs, VCStmtType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, സ്ഥിരം VCStmtType& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VCStmtType& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    എണ്ണൽ en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം VL_PURE {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VCaseType& lhs, സ്ഥിരം VCaseType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VCaseType& lhs, VCaseType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, സ്ഥിരം VCaseType& rhs) { തിരികെ lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        തിരികെ names[m_e];
    }
    bool isAssignable() സ്ഥിരം { തിരികെ m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
};
constexpr bool operator==(സ്ഥിരം VCastable& lhs, സ്ഥിരം VCastable& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VCastable& lhs, VCastable::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, സ്ഥിരം VCastable& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VCastable& rhs) {
    തിരികെ os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }  // LCOV_EXCL_LINE
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        തിരികെ names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    സ്ഥിരം അക്ഷരം* verilogKwd() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        തിരികെ names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    സ്ഥിരം അക്ഷരം* binSetEnum() സ്ഥിരം {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ BINS_IGNORE: തിരികെ "VlCovBinKind::KIND_IGNORE";
        അവസ്ഥ BINS_ILLEGAL: തിരികെ "VlCovBinKind::KIND_ILLEGAL";
        അവസ്ഥ BINS_DEFAULT: തിരികെ "VlCovBinKind::KIND_DEFAULT";
        സ്ഥിരസ്ഥിതി: തിരികെ "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() സ്ഥിരം {
        തിരികെ m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(സ്ഥിരം VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    തിരികെ lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        // Shared by option.* and type_option.*
        WEIGHT,
        GOAL,
        AT_LEAST,
        AUTO_BIN_MAX,
        PER_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_അക്കം_PRINT_MISSING,
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
    എണ്ണൽ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"weight",
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
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    തിരികെ lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    എണ്ണൽ en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(പൂർണ്ണസംഖ്യ _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }  // LCOV_EXCL_LINE
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"", "[*]", "[->]", "[=]"};
        തിരികെ names[m_e];
    }
    സ്ഥിരം അക്ഷരം* asciiJson() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        തിരികെ names[m_e];
    }
};

//######################################################################

class VDirection final {
പൊതു:
    എണ്ണൽ en : uint8_t { NONE, നിവേശം, OUTPUT, INOUT, REF, CONSTREF };
    എണ്ണൽ en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        തിരികെ names[m_e];
    }
    string verilogKwd() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        തിരികെ names[m_e];
    }
    string prettyName() സ്ഥിരം { തിരികെ verilogKwd(); }
    bool isAny() സ്ഥിരം { തിരികെ m_e != NONE; }
    bool isInout() സ്ഥിരം { തിരികെ m_e == INOUT; }
    bool isInoutOrRef() സ്ഥിരം { തിരികെ m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_നിവേശം() സ്ഥിരം { തിരികെ m_e == നിവേശം; }
    bool isNonOutput() സ്ഥിരം {
        തിരികെ m_e == നിവേശം || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() സ്ഥിരം { തിരികെ m_e == OUTPUT; }
    bool isReadOnly() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == നിവേശം || m_e == CONSTREF; }
    bool isWritable() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == REF; }
    bool isConstRef() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == CONSTREF; }
    string traceSigDirection() സ്ഥിരം {
        എങ്കിൽ (isInout()) {
            തിരികെ "VerilatedTraceSigDirection::INOUT";
        } അല്ലെങ്കിൽ എങ്കിൽ (isWritable()) {
            തിരികെ "VerilatedTraceSigDirection::OUTPUT";
        } അല്ലെങ്കിൽ എങ്കിൽ (isNonOutput()) {
            തിരികെ "VerilatedTraceSigDirection::INPUT";
        }
        തിരികെ "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() സ്ഥിരം {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ NONE: തിരികെ VAccess::NOCHANGE;
        അവസ്ഥ നിവേശം:
        അവസ്ഥ CONSTREF: തിരികെ VAccess::READ;
        അവസ്ഥ OUTPUT: തിരികെ VAccess::WRITE;
        അവസ്ഥ INOUT:
        അവസ്ഥ REF: തിരികെ VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(സ്ഥിരം VDirection& lhs, സ്ഥിരം VDirection& rhs) VL_MT_SAFE {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, സ്ഥിരം VDirection& rhs) VL_MT_SAFE {
    തിരികെ lhs == rhs.m_e;
}
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VDirection& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    എണ്ണൽ en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    bool addNewline() സ്ഥിരം { തിരികെ m_e != DT_WRITE; }
    bool needScopeTracking() സ്ഥിരം { തിരികെ m_e != DT_DISPLAY && m_e != DT_WRITE; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VDisplayType& lhs, സ്ഥിരം VDisplayType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VDisplayType& lhs, VDisplayType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, സ്ഥിരം VDisplayType& rhs) { തിരികെ lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
പൊതു:
    എണ്ണൽ en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    എണ്ണൽ en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VDumpCtlType& lhs, സ്ഥിരം VDumpCtlType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VDumpCtlType& lhs, VDumpCtlType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, സ്ഥിരം VDumpCtlType& rhs) { തിരികെ lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
പൊതു:
    // REMEMBER to edit the strings below too
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ en m_e;
    bool clockedStmt() സ്ഥിരം {
        സ്ഥിതം constexpr bool clocked[] = {
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
        തിരികെ clocked[m_e];
    }
    bool anEdge() സ്ഥിരം { തിരികെ m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() സ്ഥിരം {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ ET_CHANGED: തിരികെ ET_CHANGED;
        അവസ്ഥ ET_BOTHEDGE: തിരികെ ET_BOTHEDGE;
        അവസ്ഥ ET_POSEDGE: തിരികെ ET_NEGEDGE;
        അവസ്ഥ ET_NEGEDGE: തിരികെ ET_POSEDGE;
        സ്ഥിരസ്ഥിതി: UASSERT_STATIC(0, "Inverting bad edgeType()"); തിരികെ ET_NEGEDGE;
        }
    }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        തിരികെ names[m_e];
    }
    സ്ഥിരം അക്ഷരം* verilogKwd() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        തിരികെ names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(സ്ഥിരം VEdgeType& other) സ്ഥിരം {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ VEdgeType::ET_POSEDGE:
            എങ്കിൽ (other.m_e == VEdgeType::ET_NEGEDGE) തിരികെ true;
            നിർത്തുക;
        അവസ്ഥ VEdgeType::ET_NEGEDGE:
            എങ്കിൽ (other.m_e == VEdgeType::ET_POSEDGE) തിരികെ true;
            നിർത്തുക;
        സ്ഥിരസ്ഥിതി: നിർത്തുക;
        }
        തിരികെ false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
};
constexpr bool operator==(സ്ഥിരം VEdgeType& lhs, സ്ഥിരം VEdgeType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VEdgeType& lhs, VEdgeType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, സ്ഥിരം VEdgeType& rhs) { തിരികെ lhs == rhs.m_e; }

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
പൊതു:
    എണ്ണൽ en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    എണ്ണൽ en m_e;

    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം മൂല്യങ്ങൾ[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    bool isIterated() സ്ഥിരം {
        സ്ഥിതം constexpr bool മൂല്യങ്ങൾ[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    bool hasTriggers() സ്ഥിരം {
        സ്ഥിതം constexpr bool മൂല്യങ്ങൾ[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    bool firstIteration() സ്ഥിരം {
        സ്ഥിതം constexpr bool മൂല്യങ്ങൾ[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
    bool slow() സ്ഥിരം {
        സ്ഥിതം constexpr bool മൂല്യങ്ങൾ[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }

    // Short name
    std::string tag() സ്ഥിരം { തിരികെ VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() സ്ഥിരം { തിരികെ "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() സ്ഥിരം { തിരികെ "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() സ്ഥിരം { തിരികെ "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() സ്ഥിരം { തിരികെ "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() സ്ഥിരം { തിരികെ m_e; }

സ്വകാര്യ:
    std::string capitalizedTag() സ്ഥിരം {
        സ്ഥിരം std::string name = ascii();
        തിരികെ name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
പൊതു:
    എണ്ണൽ en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        തിരികെ names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    // Is a node type compatible with the declaration
    bool is_കണ്ണി_Compatible(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം;
};
constexpr bool operator==(സ്ഥിരം VFwdType& lhs, സ്ഥിരം VFwdType& rhs) { തിരികെ lhs.m_e == rhs.m_e; }
constexpr bool operator==(സ്ഥിരം VFwdType& lhs, VFwdType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, സ്ഥിരം VFwdType& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VFwdType& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    സ്ഥിതം uint64_t s_cachedCntGbl;  // Global computed count

പൊതു:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() സ്ഥിരം { തിരികെ m_cachedCnt == s_cachedCntGbl; }
    bool get() സ്ഥിരം { തിരികെ m_state; }
    ശൂന്യം set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    ശൂന്യം clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    സ്ഥിതം ശൂന്യം clear_Cache_വൃക്ഷം() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
പൊതു:
    എണ്ണൽ en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    എണ്ണൽ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        തിരികെ names[m_e];
    }
    സ്ഥിരം അക്ഷരം* verilogKwd() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"join", "join_any", "join_none"};
        തിരികെ names[m_e];
    }
    bool join() സ്ഥിരം { തിരികെ m_e == JOIN; }
    bool joinAny() സ്ഥിരം { തിരികെ m_e == JOIN_ANY; }
    bool joinNone() സ്ഥിരം { തിരികെ m_e == JOIN_NONE; }
};
constexpr bool operator==(സ്ഥിരം VJoinType& lhs, സ്ഥിരം VJoinType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VJoinType& lhs, VJoinType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, സ്ഥിരം VJoinType& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VJoinType& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        തിരികെ names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    bool isNone() സ്ഥിരം { തിരികെ m_e == NONE; }
    bool isAutomatic() സ്ഥിരം { തിരികെ m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() സ്ഥിരം { തിരികെ m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() സ്ഥിരം { തിരികെ m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() സ്ഥിരം {
        തിരഞ്ഞെടുപ്പ് (m_e) {
        അവസ്ഥ AUTOMATIC_EXPLICIT: തിരികെ AUTOMATIC_IMPLICIT;
        അവസ്ഥ STATIC_EXPLICIT: തിരികെ STATIC_IMPLICIT;
        സ്ഥിരസ്ഥിതി: തിരികെ m_e;
        }
    }
};
constexpr bool operator==(സ്ഥിരം VLifetime& lhs, സ്ഥിരം VLifetime& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VLifetime& lhs, VLifetime::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, സ്ഥിരം VLifetime& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VLifetime& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
പൊതു:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            പൂർണ്ണസംഖ്യ m_index;  // Current index
            സ്ഥിരം പൂർണ്ണസംഖ്യ m_inc;  // Step, +1 or -1
        പൊതു:
            Iterator(പൂർണ്ണസംഖ്യ index, പൂർണ്ണസംഖ്യ inc)
                : m_index{index}
                , m_inc{inc} {}
            പൂർണ്ണസംഖ്യ operator*() സ്ഥിരം { തിരികെ m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                തിരികെ *this;
            }
            bool operator!=(സ്ഥിരം Iterator& that) സ്ഥിരം { തിരികെ m_index != that.m_index; }
        };
        സ്ഥിരം പൂർണ്ണസംഖ്യ m_begin;  // First index
        സ്ഥിരം പൂർണ്ണസംഖ്യ m_end;  // One past the last index
        സ്ഥിരം പൂർണ്ണസംഖ്യ m_inc;  // Step, +1 or -1
    പൊതു:
        Sequence(പൂർണ്ണസംഖ്യ first, പൂർണ്ണസംഖ്യ last, പൂർണ്ണസംഖ്യ inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() സ്ഥിരം { തിരികെ Iterator{m_begin, m_inc}; }
        Iterator end() സ്ഥിരം { തിരികെ Iterator{m_end, m_inc}; }
    };

    പൂർണ്ണസംഖ്യ m_ഇടത് = 0;  // Left side of range (pre-':')
    പൂർണ്ണസംഖ്യ m_വലത് = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(സ്ഥിരം VNumRange& rhs) സ്ഥിരം {
        തിരികെ m_ഇടത് == rhs.m_ഇടത് && m_വലത് == rhs.m_വലത് && m_ranged == rhs.m_ranged;
    }
    bool operator<(സ്ഥിരം VNumRange& rhs) സ്ഥിരം {
        എങ്കിൽ ((m_ഇടത് < rhs.m_ഇടത്)) തിരികെ true;
        എങ്കിൽ (!(m_ഇടത് == rhs.m_ഇടത്)) തിരികെ false;  // lhs > rhs
        എങ്കിൽ ((m_വലത് < rhs.m_വലത്)) തിരികെ true;
        എങ്കിൽ (!(m_വലത് == rhs.m_വലത്)) തിരികെ false;  // lhs > rhs
        എങ്കിൽ ((m_ranged < rhs.m_ranged)) തിരികെ true;
        എങ്കിൽ (!(m_ranged == rhs.m_ranged)) തിരികെ false;  // lhs > rhs
        തിരികെ false;
    }
    //
    VNumRange() = സ്ഥിരസ്ഥിതി;
    VNumRange(പൂർണ്ണസംഖ്യ hi, പൂർണ്ണസംഖ്യ lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(പൂർണ്ണസംഖ്യ ഇടത്, പൂർണ്ണസംഖ്യ വലത്)
        : m_ഇടത്{ഇടത്}
        , m_വലത്{വലത്}
        , m_ranged{true} {}
    ~VNumRange() = സ്ഥിരസ്ഥിതി;
    // MEMBERS
    ശൂന്യം init(പൂർണ്ണസംഖ്യ hi, പൂർണ്ണസംഖ്യ lo, bool ascending) {
        എങ്കിൽ (lo > hi) std::swap(hi, lo);
        m_ഇടത് = ascending ? lo : hi;
        m_വലത് = ascending ? hi : lo;
        m_ranged = true;
    }
    പൂർണ്ണസംഖ്യ ഇടത്() സ്ഥിരം { തിരികെ m_ഇടത്; }
    പൂർണ്ണസംഖ്യ വലത്() സ്ഥിരം { തിരികെ m_വലത്; }
    പൂർണ്ണസംഖ്യ hi() സ്ഥിരം VL_MT_SAFE {
        തിരികെ m_ഇടത് > m_വലത് ? m_ഇടത് : m_വലത്;
    }  // How to show a declaration
    പൂർണ്ണസംഖ്യ lo() സ്ഥിരം VL_MT_SAFE {
        തിരികെ m_ഇടത് > m_വലത് ? m_വലത് : m_ഇടത്;
    }  // How to show a declaration
    പൂർണ്ണസംഖ്യ ഇടത്_To_വലത്_Inc() സ്ഥിരം { തിരികെ ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_ഇടത്_To_വലത്() സ്ഥിരം { തിരികെ {m_ഇടത്, m_വലത്, ഇടത്_To_വലത്_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_വലത്_To_ഇടത്() സ്ഥിരം { തിരികെ {m_വലത്, m_ഇടത്, -ഇടത്_To_വലത്_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() സ്ഥിരം { തിരികെ {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() സ്ഥിരം { തിരികെ {hi(), lo(), -1}; }
    പൂർണ്ണസംഖ്യ elements() സ്ഥിരം VL_MT_SAFE { തിരികെ hi() - lo() + 1; }
    bool ranged() സ്ഥിരം { തിരികെ m_ranged; }
    bool ascending() സ്ഥിരം { തിരികെ m_ഇടത് < m_വലത്; }
    പൂർണ്ണസംഖ്യ hiMaxSelect() സ്ഥിരം {
        തിരികെ (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() സ്ഥിരം {
        എങ്കിൽ (ranged()) {
            തിരികെ "["s + std::to_string(ഇടത്()) + ":" + std::to_string(വലത്()) + "]";
        } അല്ലെങ്കിൽ {
            തിരികെ "[norg]";
        }
    }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം { str << ascii(); }
};
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VNumRange& rhs) {
    rhs.dump(os);
    തിരികെ os;
}

// ######################################################################

class VPragmaType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {
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
        തിരികെ names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
};
constexpr bool operator==(സ്ഥിരം VPragmaType& lhs, സ്ഥിരം VPragmaType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VPragmaType& lhs, VPragmaType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, സ്ഥിരം VPragmaType& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VPragmaType& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        തിരികെ names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    bool isRandomizable() സ്ഥിരം { തിരികെ m_e != NONE; }
    bool isRand() സ്ഥിരം { തിരികെ m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() സ്ഥിരം { തിരികെ m_e == RAND_CYCLIC; }
};
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VRandAttr& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    സ്ഥിതം സ്ഥിരം std::shared_ptr<സ്ഥിരം string> s_emptyp;  // Holds ""
    സ്ഥിതം സ്ഥിരം std::shared_ptr<സ്ഥിരം string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<സ്ഥിരം string> m_strp;

പൊതു:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, സ്ഥിരം string& field)
        : m_strp{std::make_shared<സ്ഥിരം string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, സ്ഥിരം string& field)
        : m_strp{std::make_shared<സ്ഥിരം string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() സ്ഥിരം { തിരികെ m_strp == s_emptyp; }
    bool isVlSym() സ്ഥിരം { തിരികെ m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() സ്ഥിരം { തിരികെ m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) സ്ഥിരം;
    സ്ഥിതം string replaceThis(bool useSelfForThis, സ്ഥിരം string& text);
    സ്ഥിരം std::string& asString() സ്ഥിരം { തിരികെ *m_strp; }
    bool operator==(സ്ഥിരം VSelfPointerText& other) സ്ഥിരം { തിരികെ *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        തിരികെ names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    സ്ഥിതം VSigning fromBool(bool isSigned) {  // Factory method
        തിരികെ isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    bool isSigned() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == SIGNED; }
    bool isNosign() സ്ഥിരം VL_MT_SAFE { തിരികെ m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(സ്ഥിരം VSigning& lhs, സ്ഥിരം VSigning& rhs) { തിരികെ lhs.m_e == rhs.m_e; }
constexpr bool operator==(സ്ഥിരം VSigning& lhs, VSigning::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, സ്ഥിരം VSigning& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VSigning& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

class VStrength final {
പൊതു:
    എണ്ണൽ en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    എണ്ണൽ en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VStrength& lhs, സ്ഥിരം VStrength& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VStrength& lhs, VStrength::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, സ്ഥിരം VStrength& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VStrength& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    എണ്ണൽ en m_e;
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        തിരികെ names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
};

//######################################################################

class VTracePrefixType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    എണ്ണൽ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VTracePrefixType& lhs, സ്ഥിരം VTracePrefixType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, സ്ഥിരം VTracePrefixType& rhs) {
    തിരികെ lhs == rhs.m_e;
}
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VTracePrefixType& rhs) {
    തിരികെ os << rhs.ascii();
}

//######################################################################

class VTraceType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    എണ്ണൽ en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"CONSTANT", "FULL", "CHANGE"};
        തിരികെ names[m_e];
    }
    സ്ഥിരം അക്ഷരം* func_prefix() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"trace_const", "trace_full", "trace_chg"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VTraceType& lhs, സ്ഥിരം VTraceType& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VTraceType& lhs, VTraceType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, സ്ഥിരം VTraceType& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VTraceType& rhs) {
    തിരികെ os << rhs.ascii();
}

//######################################################################

class VUseType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    എണ്ണൽ en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    bool containsAny(VUseType other) സ്ഥിരം { തിരികെ m_e & other.m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        തിരികെ names[m_e - 1];
    }
};
constexpr bool operator==(സ്ഥിരം VUseType& lhs, സ്ഥിരം VUseType& rhs) { തിരികെ lhs.m_e == rhs.m_e; }
constexpr bool operator==(സ്ഥിരം VUseType& lhs, VUseType::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, സ്ഥിരം VUseType& rhs) { തിരികെ lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    തിരികെ VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    തിരികെ VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VUseType& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

class VVarType final {
പൊതു:
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        തിരികെ names[m_e];
    }
    bool isParam() സ്ഥിരം { തിരികെ m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() സ്ഥിരം {
        തിരികെ (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() സ്ഥിരം {
        തിരികെ (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() സ്ഥിരം { തിരികെ (m_e == TRIOR); }
    bool isWiredNet() സ്ഥിരം { തിരികെ (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() സ്ഥിരം {  // In Verilog, always ok in SystemVerilog
        തിരികെ (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() സ്ഥിരം {
        തിരികെ (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() സ്ഥിരം {
        തിരികെ (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() സ്ഥിരം {
        തിരികെ (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    സ്ഥിരം അക്ഷരം* traceSigKind() സ്ഥിരം {
        // VerilatedTraceSigKind to used in trace signal declaration
        സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം lut[] = {
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
        തിരികെ lut[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VVarType& lhs, സ്ഥിരം VVarType& rhs) VL_MT_SAFE {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, സ്ഥിരം VVarType& rhs) VL_MT_SAFE {
    തിരികെ lhs == rhs.m_e;
}
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VVarType& rhs) VL_MT_SAFE {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_താക്കോൽ final {
പൊതു:
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_width;  // From AstNodeDType: Bit width of operation
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    സ്ഥിരം VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    സ്ഥിരം VSigning m_numeric;  // From AstNodeDType: Node is signed
    സ്ഥിരം VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(സ്ഥിരം VBasic_Type_താക്കോൽ& rhs) സ്ഥിരം {
        തിരികെ m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(സ്ഥിരം VBasic_Type_താക്കോൽ& rhs) സ്ഥിരം {
        എങ്കിൽ ((m_width < rhs.m_width)) തിരികെ true;
        എങ്കിൽ (!(m_width == rhs.m_width)) തിരികെ false;  // lhs > rhs
        എങ്കിൽ ((m_widthMin < rhs.m_widthMin)) തിരികെ true;
        എങ്കിൽ (!(m_widthMin == rhs.m_widthMin)) തിരികെ false;  // lhs > rhs
        എങ്കിൽ ((m_numeric < rhs.m_numeric)) തിരികെ true;
        എങ്കിൽ (!(m_numeric == rhs.m_numeric)) തിരികെ false;  // lhs > rhs
        എങ്കിൽ ((m_keyword < rhs.m_keyword)) തിരികെ true;
        എങ്കിൽ (!(m_keyword == rhs.m_keyword)) തിരികെ false;  // lhs > rhs
        എങ്കിൽ ((m_nrange < rhs.m_nrange)) തിരികെ true;
        എങ്കിൽ (!(m_nrange == rhs.m_nrange)) തിരികെ false;  // lhs > rhs
        തിരികെ false;
    }
    VBasic_Type_താക്കോൽ(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  സ്ഥിരം VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_താക്കോൽ() = സ്ഥിരസ്ഥിതി;
};

#endif  // Guard
