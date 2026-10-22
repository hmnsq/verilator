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
#अन्तर्भाव "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
सार्वजनिक:
    // IEEE 1800-2023 16.12.14 property abort operators.
    गणना en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    गणना en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर ध्रुव वर्ण* ध्रुव names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        प्रत्यागम names[m_e];
    }
    bool isAccept() ध्रुव { प्रत्यागम m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
सार्वजनिक:
    गणना en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"RD", "WR", "RW", "--"};
        प्रत्यागम names[m_e];
    }
    ध्रुव वर्ण* arrow() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        प्रत्यागम names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    VAccess invert() ध्रुव {
        प्रत्यागम (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ध्रुव { प्रत्यागम m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ध्रुव { प्रत्यागम m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ध्रुव { प्रत्यागम m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ध्रुव { प्रत्यागम m_e == WRITE || m_e == READWRITE; }
    bool isRW() ध्रुव { प्रत्यागम m_e == READWRITE; }
};
constexpr bool operator==(ध्रुव VAccess& lhs, ध्रुव VAccess& rhs) { प्रत्यागम lhs.m_e == rhs.m_e; }
constexpr bool operator==(ध्रुव VAccess& lhs, VAccess::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, ध्रुव VAccess& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VAccess& rhs) { प्रत्यागम os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
सार्वजनिक:
    गणना en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    गणना en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VAlwaysKwd& lhs, ध्रुव VAlwaysKwd& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, ध्रुव VAlwaysKwd& rhs) { प्रत्यागम lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
सार्वजनिक:
    // IEEE 1800-2023 Table 20-5
    गणना en : uint8_t {
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
    गणना en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        // IEEE 1800-2023 20.11
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"",
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
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VAssertCtlType& lhs, ध्रुव VAssertCtlType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, ध्रुव VAssertCtlType& rhs) {
    प्रत्यागम lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
सार्वजनिक:
    // IEEE 1800-2023 Table 20-7
    गणना en : uint8_t {
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
    गणना en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ध्रुव {
        std::stringstream types;
        यदि (m_e == INTERNAL)
            types << "INTERNAL ";
        अन्यथा {
            यदि (m_e & ASSERT) types << "ASSERT ";
            यदि (m_e & COVER) types << "COVER ";
            यदि (m_e & ASSUME) types << "ASSUME ";
            यदि (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            यदि (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            यदि (m_e & INTRINSIC) types << "INTRINSIC ";
            यदि (m_e & RESTRICT) types << "RESTRICT ";
        }
        ध्रुव string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        प्रत्यागम str.substr(0, str.size() - 1);
    }
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
};
constexpr bool operator==(ध्रुव VAssertDirectiveType& lhs, ध्रुव VAssertDirectiveType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, ध्रुव VAssertDirectiveType& rhs) {
    प्रत्यागम lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    प्रत्यागम VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
सार्वजनिक:
    // IEEE 1800-2023 Table 20-6
    गणना en : uint8_t {
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
    गणना en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) ध्रुव { प्रत्यागम m_e & other.m_e; }
    string ascii() ध्रुव {
        std::stringstream types;
        यदि (m_e == INTERNAL)
            types << "INTERNAL ";
        अन्यथा {
            यदि (m_e & CONCURRENT) types << "CONCURRENT ";
            यदि (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            यदि (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            यदि (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            यदि (m_e & EXPECT) types << "EXPECT ";
            यदि (m_e & UNIQUE) types << "UNIQUE ";
            यदि (m_e & UNIQUE0) types << "UNIQUE0 ";
            यदि (m_e & PRIORITY) types << "PRIORITY ";
        }
        ध्रुव string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        प्रत्यागम str.substr(0, str.size() - 1);
    }
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
};
constexpr bool operator==(ध्रुव VAssertType& lhs, ध्रुव VAssertType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VAssertType& lhs, VAssertType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, ध्रुव VAssertType& rhs) { प्रत्यागम lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    प्रत्यागम VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
सार्वजनिक:
    गणना en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    गणना en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"default", "weak", "strong"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VPropStrength& lhs, ध्रुव VPropStrength& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VPropStrength& lhs, VPropStrength::en rhs) {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator!=(ध्रुव VPropStrength& lhs, VPropStrength::en rhs) {
    प्रत्यागम lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
सार्वजनिक:
    // clang-format off
    गणना en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_संख्या,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_वरचा,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_डावा,                       // V3Width processes
        DIM_खालचा,                        // V3Width processes
        DIM_उजवा,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_अंक,                       // V3Width processes
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
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        // clang-format off
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {
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
        प्रत्यागम names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ध्रुव {
        विकल्प (m_e) {
        पक्ष DIM_BITS:
        पक्ष DIM_BITS_OR_संख्या:
        पक्ष DIM_DIMENSIONS:
        पक्ष DIM_वरचा:
        पक्ष DIM_INCREMENT:
        पक्ष DIM_डावा:
        पक्ष DIM_खालचा:
        पक्ष DIM_उजवा:
        पक्ष DIM_SIZE:
        पक्ष DIM_UNPK_DIMENSIONS:
        पक्ष TYPEID:
        पक्ष TYPENAME: प्रत्यागम true;
        स्वभाव: प्रत्यागम false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
};
constexpr bool operator==(ध्रुव VAttrType& lhs, ध्रुव VAttrType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VAttrType& lhs, VAttrType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, ध्रुव VAttrType& rhs) { प्रत्यागम lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

सार्वजनिक:
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
    शून्य combine(ध्रुव VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ध्रुव { प्रत्यागम m_extends | m_final | m_initial; }
    bool isExtends() ध्रुव { प्रत्यागम m_extends; }
    bool isFinal() ध्रुव { प्रत्यागम m_final; }
    bool isInitial() ध्रुव { प्रत्यागम m_initial; }
    string ascii() ध्रुव {
        string out;
        यदि (m_initial) out = VString::dot(out, " ", "initial");
        यदि (m_extends) out = VString::dot(out, " ", "extends");
        यदि (m_final) out = VString::dot(out, " ", "final");
        प्रत्यागम out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
सार्वजनिक:
    गणना en : uint8_t {
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
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव VL_MT_SAFE {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"%E-unk",
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
        प्रत्यागम names[m_e];
    }
    ध्रुव वर्ण* typeName() ध्रुव VL_MT_SAFE {
        विकल्प (m_e) {
        पक्ष LOGIC2STATE: प्रत्यागम VBasicDTypeKwd{LOGIC}.ascii();
        पक्ष INTEGER2STATE: प्रत्यागम VBasicDTypeKwd{INTEGER}.ascii();
        पक्ष TIME2STATE: प्रत्यागम VBasicDTypeKwd{TIME}.ascii();
        स्वभाव: प्रत्यागम ascii();
        }
    }
    ध्रुव वर्ण* dpiType() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"%E-unk",
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
        प्रत्यागम names[m_e];
    }
    स्थिर शून्य selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(पूर्णाङ्क _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    पूर्णाङ्क width() ध्रुव {
        विकल्प (m_e) {
        पक्ष BIT: प्रत्यागम 1;  // scalar, can't bit extract unless ranged
        पक्ष BYTE: प्रत्यागम 8;
        पक्ष CHANDLE: प्रत्यागम 64;
        पक्ष NULLTYPE: प्रत्यागम 1;  // Historical - bit was used instead of NULLTYPE
        पक्ष EVENT: प्रत्यागम 1;
        पक्ष INT: प्रत्यागम 32;
        पक्ष INTEGER: प्रत्यागम 32;
        पक्ष INTEGER2STATE: प्रत्यागम 32;
        पक्ष LOGIC: प्रत्यागम 1;  // scalar, can't bit extract unless ranged
        पक्ष LOGIC2STATE: प्रत्यागम 1;  // scalar, can't bit extract unless ranged
        पक्ष LONGINT: प्रत्यागम 64;
        पक्ष DOUBLE: प्रत्यागम 64;  // opaque
        पक्ष SHORTINT: प्रत्यागम 16;
        पक्ष TIME: प्रत्यागम 64;
        पक्ष TIME2STATE: प्रत्यागम 64;
        पक्ष STRING: प्रत्यागम 64;  // opaque  // Just the pointer, for today
        पक्ष SCOPEPTR: प्रत्यागम 0;  // opaque
        पक्ष CHARPTR: प्रत्यागम 0;  // opaque
        पक्ष MTASKSTATE: प्रत्यागम 0;  // opaque
        पक्ष DELAY_SCHEDULER: प्रत्यागम 0;  // opaque
        पक्ष TRIGGER_SCHEDULER: प्रत्यागम 0;  // opaque
        पक्ष DYNAMIC_TRIGGER_SCHEDULER: प्रत्यागम 0;  // opaque
        पक्ष FORK_SYNC: प्रत्यागम 0;  // opaque
        पक्ष PROCESS_REFERENCE: प्रत्यागम 0;  // opaque
        पक्ष RANDOM_GENERATOR: प्रत्यागम 0;  // opaque
        पक्ष RANDOM_STDGENERATOR: प्रत्यागम 0;  // opaque
        पक्ष COVERGROUP_INSTHANDLE: प्रत्यागम 0;  // opaque
        पक्ष UINT32: प्रत्यागम 32;
        पक्ष UINT64: प्रत्यागम 64;
        स्वभाव: प्रत्यागम 0;
        }
    }
    bool isSigned() ध्रुव {
        प्रत्यागम m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ध्रुव {
        प्रत्यागम m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() ध्रुव {
        प्रत्यागम m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() ध्रुव {  // Otherwise initializes to X
        प्रत्यागम (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ध्रुव {  // Enum increment supported
        प्रत्यागम (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() ध्रुव { प्रत्यागम m_e == BIT; }
    bool isBitLogic() ध्रुव {  // Bit/logic vector types; can form a packed array
        प्रत्यागम (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() ध्रुव {  // Can add "unsigned" to DPI
        प्रत्यागम (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ध्रुव {  // Uses standard C layout, for DPI runtime access
        प्रत्यागम (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ध्रुव VL_MT_SAFE {  // IE not a simple number we can bit optimize
        प्रत्यागम (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == CHANDLE; }
    bool isDouble() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == DOUBLE; }
    bool isEvent() ध्रुव { प्रत्यागम m_e == EVENT; }
    bool isString() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == STRING; }
    bool isMTaskState() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == MTASKSTATE; }
    bool isSameish(ध्रुव VBasicDTypeKwd& other) ध्रुव VL_MT_SAFE {
        विकल्प (other.m_e) {
        पक्ष LOGIC:
        पक्ष LOGIC2STATE:
        पक्ष LOGIC_IMPLICIT: प्रत्यागम m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        पक्ष INTEGER:
        पक्ष INTEGER2STATE: प्रत्यागम m_e == INTEGER || m_e == INTEGER2STATE;
        पक्ष TIME:
        पक्ष TIME2STATE: प्रत्यागम m_e == TIME || m_e == TIME2STATE;
        स्वभाव: प्रत्यागम m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ध्रुव VL_MT_SAFE {
        विकल्प (m_e) {
        पक्ष BIT:
        पक्ष BYTE:
        पक्ष CHANDLE:
        पक्ष INT:
        पक्ष INTEGER:
        पक्ष INTEGER2STATE:
        पक्ष LOGIC:
        पक्ष LOGIC2STATE:
        पक्ष LONGINT:
        पक्ष DOUBLE:
        पक्ष SHORTINT:
        पक्ष SCOPEPTR:
        पक्ष CHARPTR:
        पक्ष UINT32:
        पक्ष UINT64: प्रत्यागम true;
        स्वभाव: प्रत्यागम false;
        }
    }

    ध्रुव वर्ण* traceSigType() ध्रुव {
        // VerilatedTraceSigType to used in trace signal declaration
        स्थिर constexpr ध्रुव वर्ण* ध्रुव lut[] = {
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
        static_assert(परिमाण(lut) / परिमाण(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        प्रत्यागम lut[m_e];
    }
};
constexpr bool operator==(ध्रुव VBasicDTypeKwd& lhs, ध्रुव VBasicDTypeKwd& rhs) VL_MT_SAFE {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, ध्रुव VBasicDTypeKwd& rhs) VL_MT_SAFE {
    प्रत्यागम lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
सार्वजनिक:
    गणना en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    गणना en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"FALSE", "TRUE", "UNK"};
        प्रत्यागम names[m_e];
    }
    bool isKnown() ध्रुव { प्रत्यागम m_e != BU_UNKNOWN; }
    bool trueKnown() ध्रुव { प्रत्यागम m_e == BU_TRUE; }
    शून्य setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(ध्रुव VBoolOrUnknown& lhs, ध्रुव VBoolOrUnknown& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, ध्रुव VBoolOrUnknown& rhs) {
    प्रत्यागम lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VBoolOrUnknown& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
सार्वजनिक:
    गणना en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    गणना en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    bool unknown() ध्रुव { प्रत्यागम m_e == BP_UNKNOWN; }
    bool likely() ध्रुव { प्रत्यागम m_e == BP_LIKELY; }
    bool unlikely() ध्रुव { प्रत्यागम m_e == BP_UNLIKELY; }
    VBranchPred invert() ध्रुव {
        यदि (m_e == BP_UNLIKELY) प्रत्यागम BP_LIKELY;
        यदि (m_e == BP_LIKELY) प्रत्यागम BP_UNLIKELY;
        प्रत्यागम m_e;
    }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        प्रत्यागम names[m_e];
    }
    ध्रुव वर्ण* asciiShort() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"", "L", "!L"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VBranchPred& lhs, ध्रुव VBranchPred& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VBranchPred& lhs, VBranchPred::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, ध्रुव VBranchPred& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VBranchPred& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
स्थिर constexpr bool validateArgsDescriptor(ध्रुव वर्ण* descrp) {
    // Is "TODO"
    यदि (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        प्रत्यागम true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    प्रति (ध्रुव वर्ण* cp = descrp; *cp; ++cp) {
        यदि (*cp == '+') प्रत्यागम cp != descrp && !cp[1];
        यदि (*cp != 'r' && *cp != 'w' && *cp != 'm') प्रत्यागम false;
    }
    प्रत्यागम true;
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
    macro(ARRAY_काढा_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_काढा_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ढकला_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ढकला_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_मूल्य,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_मूल्य_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_मूल्य_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_मूल्य_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_मूल्य_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_मूल्य_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_मूल्य_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_मूल्य_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_मूल्य_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_मूल्य_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_मूल्य_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_घाला,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_काढा,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_काढा_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ढकला,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ढकला_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    स्थिर constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    स्थिर constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

सार्वजनिक:
    गणना en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    गणना en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव VL_PURE {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव मूल्ये[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    bool isPure() ध्रुव VL_PURE {
        स्थिर constexpr bool मूल्ये[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    bool isClean() ध्रुव VL_PURE {
        स्थिर constexpr bool मूल्ये[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    ध्रुव वर्ण* args() ध्रुव VL_PURE {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव मूल्ये[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    // Return array method for given name
    स्थिर VCMethod arrayMethod(ध्रुव string& name);
};
constexpr bool operator==(ध्रुव VCMethod& lhs, ध्रुव VCMethod& rhs) { प्रत्यागम lhs.m_e == rhs.m_e; }
constexpr bool operator==(ध्रुव VCMethod& lhs, VCMethod::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, ध्रुव VCMethod& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VCMethod& rhs) {
    प्रत्यागम os << rhs.ascii();
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
    स्थिर constexpr bool PURE = true;  // For macro expansion of 'pure' field only

सार्वजनिक:
    गणना en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    गणना en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ध्रुव वर्ण* ascii() ध्रुव VL_PURE {
        स्थिर ध्रुव वर्ण* ध्रुव मूल्ये[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    bool isPure() ध्रुव VL_PURE {
        स्थिर ध्रुव bool मूल्ये[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    ध्रुव वर्ण* args() ध्रुव VL_PURE {
        स्थिर ध्रुव वर्ण* ध्रुव मूल्ये[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        प्रत्यागम मूल्ये[m_e];
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
सार्वजनिक:
    गणना en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

गुप्त:
    संरचना Item final {
        गणना en m_e;  // Statement's enum mnemonic, for checking
        ध्रुव वर्ण* m_name;  // Statements name, for debugging
    };
    स्थिर Item s_itemData[];

सार्वजनिक:
    गणना en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव VL_PURE {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"none", "ctor_var_reset_call"};
        प्रत्यागम names[m_e];
    }
    bool isNone() ध्रुव { प्रत्यागम m_e == NONE; }
};
constexpr bool operator==(ध्रुव VCStmtType& lhs, ध्रुव VCStmtType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VCStmtType& lhs, VCStmtType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, ध्रुव VCStmtType& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VCStmtType& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
सार्वजनिक:
    गणना en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    गणना en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव VL_PURE {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VCaseType& lhs, ध्रुव VCaseType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VCaseType& lhs, VCaseType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, ध्रुव VCaseType& rhs) { प्रत्यागम lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
सार्वजनिक:
    गणना en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        प्रत्यागम names[m_e];
    }
    bool isAssignable() ध्रुव { प्रत्यागम m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
};
constexpr bool operator==(ध्रुव VCastable& lhs, ध्रुव VCastable& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VCastable& lhs, VCastable::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, ध्रुव VCastable& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VCastable& rhs) {
    प्रत्यागम os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
सार्वजनिक:
    गणना en : uint8_t {
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
    गणना en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }  // LCOV_EXCL_LINE
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        प्रत्यागम names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ध्रुव वर्ण* verilogKwd() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        प्रत्यागम names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ध्रुव वर्ण* binSetEnum() ध्रुव {
        विकल्प (m_e) {
        पक्ष BINS_IGNORE: प्रत्यागम "VlCovBinKind::KIND_IGNORE";
        पक्ष BINS_ILLEGAL: प्रत्यागम "VlCovBinKind::KIND_ILLEGAL";
        पक्ष BINS_DEFAULT: प्रत्यागम "VlCovBinKind::KIND_DEFAULT";
        स्वभाव: प्रत्यागम "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() ध्रुव {
        प्रत्यागम m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(ध्रुव VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    प्रत्यागम lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
सार्वजनिक:
    गणना en : uint8_t {
        // Shared by option.* and type_option.*
        WEIGHT,
        GOAL,
        AT_LEAST,
        AUTO_BIN_MAX,
        PER_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_अंक_PRINT_MISSING,
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
    गणना en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"weight",
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
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    प्रत्यागम lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
सार्वजनिक:
    गणना en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    गणना en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(पूर्णाङ्क _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }  // LCOV_EXCL_LINE
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"", "[*]", "[->]", "[=]"};
        प्रत्यागम names[m_e];
    }
    ध्रुव वर्ण* asciiJson() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        प्रत्यागम names[m_e];
    }
};

//######################################################################

class VDirection final {
सार्वजनिक:
    गणना en : uint8_t { NONE, आदान, OUTPUT, INOUT, REF, CONSTREF };
    गणना en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव VL_MT_SAFE { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        प्रत्यागम names[m_e];
    }
    string verilogKwd() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        प्रत्यागम names[m_e];
    }
    string prettyName() ध्रुव { प्रत्यागम verilogKwd(); }
    bool isAny() ध्रुव { प्रत्यागम m_e != NONE; }
    bool isInout() ध्रुव { प्रत्यागम m_e == INOUT; }
    bool isInoutOrRef() ध्रुव { प्रत्यागम m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_आदान() ध्रुव { प्रत्यागम m_e == आदान; }
    bool isNonOutput() ध्रुव {
        प्रत्यागम m_e == आदान || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ध्रुव { प्रत्यागम m_e == OUTPUT; }
    bool isReadOnly() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == आदान || m_e == CONSTREF; }
    bool isWritable() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == REF; }
    bool isConstRef() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == CONSTREF; }
    string traceSigDirection() ध्रुव {
        यदि (isInout()) {
            प्रत्यागम "VerilatedTraceSigDirection::INOUT";
        } अन्यथा यदि (isWritable()) {
            प्रत्यागम "VerilatedTraceSigDirection::OUTPUT";
        } अन्यथा यदि (isNonOutput()) {
            प्रत्यागम "VerilatedTraceSigDirection::INPUT";
        }
        प्रत्यागम "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ध्रुव {
        विकल्प (m_e) {
        पक्ष NONE: प्रत्यागम VAccess::NOCHANGE;
        पक्ष आदान:
        पक्ष CONSTREF: प्रत्यागम VAccess::READ;
        पक्ष OUTPUT: प्रत्यागम VAccess::WRITE;
        पक्ष INOUT:
        पक्ष REF: प्रत्यागम VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(ध्रुव VDirection& lhs, ध्रुव VDirection& rhs) VL_MT_SAFE {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, ध्रुव VDirection& rhs) VL_MT_SAFE {
    प्रत्यागम lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VDirection& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
सार्वजनिक:
    गणना en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    गणना en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    bool addNewline() ध्रुव { प्रत्यागम m_e != DT_WRITE; }
    bool needScopeTracking() ध्रुव { प्रत्यागम m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VDisplayType& lhs, ध्रुव VDisplayType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VDisplayType& lhs, VDisplayType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, ध्रुव VDisplayType& rhs) { प्रत्यागम lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
सार्वजनिक:
    गणना en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    गणना en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VDumpCtlType& lhs, ध्रुव VDumpCtlType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VDumpCtlType& lhs, VDumpCtlType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, ध्रुव VDumpCtlType& rhs) { प्रत्यागम lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
सार्वजनिक:
    // REMEMBER to edit the strings below too
    गणना en : uint8_t {
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
    गणना en m_e;
    bool clockedStmt() ध्रुव {
        स्थिर constexpr bool clocked[] = {
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
        प्रत्यागम clocked[m_e];
    }
    bool anEdge() ध्रुव { प्रत्यागम m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ध्रुव {
        विकल्प (m_e) {
        पक्ष ET_CHANGED: प्रत्यागम ET_CHANGED;
        पक्ष ET_BOTHEDGE: प्रत्यागम ET_BOTHEDGE;
        पक्ष ET_POSEDGE: प्रत्यागम ET_NEGEDGE;
        पक्ष ET_NEGEDGE: प्रत्यागम ET_POSEDGE;
        स्वभाव: UASSERT_STATIC(0, "Inverting bad edgeType()"); प्रत्यागम ET_NEGEDGE;
        }
    }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        प्रत्यागम names[m_e];
    }
    ध्रुव वर्ण* verilogKwd() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        प्रत्यागम names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(ध्रुव VEdgeType& other) ध्रुव {
        विकल्प (m_e) {
        पक्ष VEdgeType::ET_POSEDGE:
            यदि (other.m_e == VEdgeType::ET_NEGEDGE) प्रत्यागम true;
            भङ्ग;
        पक्ष VEdgeType::ET_NEGEDGE:
            यदि (other.m_e == VEdgeType::ET_POSEDGE) प्रत्यागम true;
            भङ्ग;
        स्वभाव: भङ्ग;
        }
        प्रत्यागम false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
};
constexpr bool operator==(ध्रुव VEdgeType& lhs, ध्रुव VEdgeType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VEdgeType& lhs, VEdgeType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, ध्रुव VEdgeType& rhs) { प्रत्यागम lhs == rhs.m_e; }

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
सार्वजनिक:
    गणना en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    गणना en m_e;

    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव मूल्ये[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    bool isIterated() ध्रुव {
        स्थिर constexpr bool मूल्ये[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    bool hasTriggers() ध्रुव {
        स्थिर constexpr bool मूल्ये[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    bool firstIteration() ध्रुव {
        स्थिर constexpr bool मूल्ये[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        प्रत्यागम मूल्ये[m_e];
    }
    bool slow() ध्रुव {
        स्थिर constexpr bool मूल्ये[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        प्रत्यागम मूल्ये[m_e];
    }

    // Short name
    std::string tag() ध्रुव { प्रत्यागम VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ध्रुव { प्रत्यागम "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() ध्रुव { प्रत्यागम "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ध्रुव { प्रत्यागम "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() ध्रुव { प्रत्यागम "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() ध्रुव { प्रत्यागम m_e; }

गुप्त:
    std::string capitalizedTag() ध्रुव {
        ध्रुव std::string name = ascii();
        प्रत्यागम name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
सार्वजनिक:
    गणना en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        प्रत्यागम names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    // Is a node type compatible with the declaration
    bool is_गाठ_Compatible(ध्रुव Ast_गाठ* nodep) ध्रुव;
};
constexpr bool operator==(ध्रुव VFwdType& lhs, ध्रुव VFwdType& rhs) { प्रत्यागम lhs.m_e == rhs.m_e; }
constexpr bool operator==(ध्रुव VFwdType& lhs, VFwdType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, ध्रुव VFwdType& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VFwdType& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    स्थिर uint64_t s_cachedCntGbl;  // Global computed count

सार्वजनिक:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() ध्रुव { प्रत्यागम m_cachedCnt == s_cachedCntGbl; }
    bool get() ध्रुव { प्रत्यागम m_state; }
    शून्य set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    शून्य clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    स्थिर शून्य clear_Cache_वृक्ष() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
सार्वजनिक:
    गणना en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    गणना en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        प्रत्यागम names[m_e];
    }
    ध्रुव वर्ण* verilogKwd() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"join", "join_any", "join_none"};
        प्रत्यागम names[m_e];
    }
    bool join() ध्रुव { प्रत्यागम m_e == JOIN; }
    bool joinAny() ध्रुव { प्रत्यागम m_e == JOIN_ANY; }
    bool joinNone() ध्रुव { प्रत्यागम m_e == JOIN_NONE; }
};
constexpr bool operator==(ध्रुव VJoinType& lhs, ध्रुव VJoinType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VJoinType& lhs, VJoinType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, ध्रुव VJoinType& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VJoinType& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
सार्वजनिक:
    गणना en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        प्रत्यागम names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    bool isNone() ध्रुव { प्रत्यागम m_e == NONE; }
    bool isAutomatic() ध्रुव { प्रत्यागम m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ध्रुव { प्रत्यागम m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ध्रुव { प्रत्यागम m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ध्रुव {
        विकल्प (m_e) {
        पक्ष AUTOMATIC_EXPLICIT: प्रत्यागम AUTOMATIC_IMPLICIT;
        पक्ष STATIC_EXPLICIT: प्रत्यागम STATIC_IMPLICIT;
        स्वभाव: प्रत्यागम m_e;
        }
    }
};
constexpr bool operator==(ध्रुव VLifetime& lhs, ध्रुव VLifetime& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VLifetime& lhs, VLifetime::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, ध्रुव VLifetime& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VLifetime& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
सार्वजनिक:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            पूर्णाङ्क m_index;  // Current index
            ध्रुव पूर्णाङ्क m_inc;  // Step, +1 or -1
        सार्वजनिक:
            Iterator(पूर्णाङ्क index, पूर्णाङ्क inc)
                : m_index{index}
                , m_inc{inc} {}
            पूर्णाङ्क operator*() ध्रुव { प्रत्यागम m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                प्रत्यागम *this;
            }
            bool operator!=(ध्रुव Iterator& that) ध्रुव { प्रत्यागम m_index != that.m_index; }
        };
        ध्रुव पूर्णाङ्क m_begin;  // First index
        ध्रुव पूर्णाङ्क m_end;  // One past the last index
        ध्रुव पूर्णाङ्क m_inc;  // Step, +1 or -1
    सार्वजनिक:
        Sequence(पूर्णाङ्क first, पूर्णाङ्क last, पूर्णाङ्क inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() ध्रुव { प्रत्यागम Iterator{m_begin, m_inc}; }
        Iterator end() ध्रुव { प्रत्यागम Iterator{m_end, m_inc}; }
    };

    पूर्णाङ्क m_डावा = 0;  // Left side of range (pre-':')
    पूर्णाङ्क m_उजवा = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(ध्रुव VNumRange& rhs) ध्रुव {
        प्रत्यागम m_डावा == rhs.m_डावा && m_उजवा == rhs.m_उजवा && m_ranged == rhs.m_ranged;
    }
    bool operator<(ध्रुव VNumRange& rhs) ध्रुव {
        यदि ((m_डावा < rhs.m_डावा)) प्रत्यागम true;
        यदि (!(m_डावा == rhs.m_डावा)) प्रत्यागम false;  // lhs > rhs
        यदि ((m_उजवा < rhs.m_उजवा)) प्रत्यागम true;
        यदि (!(m_उजवा == rhs.m_उजवा)) प्रत्यागम false;  // lhs > rhs
        यदि ((m_ranged < rhs.m_ranged)) प्रत्यागम true;
        यदि (!(m_ranged == rhs.m_ranged)) प्रत्यागम false;  // lhs > rhs
        प्रत्यागम false;
    }
    //
    VNumRange() = स्वभाव;
    VNumRange(पूर्णाङ्क hi, पूर्णाङ्क lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(पूर्णाङ्क डावा, पूर्णाङ्क उजवा)
        : m_डावा{डावा}
        , m_उजवा{उजवा}
        , m_ranged{true} {}
    ~VNumRange() = स्वभाव;
    // MEMBERS
    शून्य init(पूर्णाङ्क hi, पूर्णाङ्क lo, bool ascending) {
        यदि (lo > hi) std::swap(hi, lo);
        m_डावा = ascending ? lo : hi;
        m_उजवा = ascending ? hi : lo;
        m_ranged = true;
    }
    पूर्णाङ्क डावा() ध्रुव { प्रत्यागम m_डावा; }
    पूर्णाङ्क उजवा() ध्रुव { प्रत्यागम m_उजवा; }
    पूर्णाङ्क hi() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_डावा > m_उजवा ? m_डावा : m_उजवा;
    }  // How to show a declaration
    पूर्णाङ्क lo() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_डावा > m_उजवा ? m_उजवा : m_डावा;
    }  // How to show a declaration
    पूर्णाङ्क डावा_To_उजवा_Inc() ध्रुव { प्रत्यागम ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_डावा_To_उजवा() ध्रुव { प्रत्यागम {m_डावा, m_उजवा, डावा_To_उजवा_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_उजवा_To_डावा() ध्रुव { प्रत्यागम {m_उजवा, m_डावा, -डावा_To_उजवा_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() ध्रुव { प्रत्यागम {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() ध्रुव { प्रत्यागम {hi(), lo(), -1}; }
    पूर्णाङ्क elements() ध्रुव VL_MT_SAFE { प्रत्यागम hi() - lo() + 1; }
    bool ranged() ध्रुव { प्रत्यागम m_ranged; }
    bool ascending() ध्रुव { प्रत्यागम m_डावा < m_उजवा; }
    पूर्णाङ्क hiMaxSelect() ध्रुव {
        प्रत्यागम (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ध्रुव {
        यदि (ranged()) {
            प्रत्यागम "["s + std::to_string(डावा()) + ":" + std::to_string(उजवा()) + "]";
        } अन्यथा {
            प्रत्यागम "[norg]";
        }
    }
    शून्य dump(std::ostream& str) ध्रुव { str << ascii(); }
};
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VNumRange& rhs) {
    rhs.dump(os);
    प्रत्यागम os;
}

// ######################################################################

class VPragmaType final {
सार्वजनिक:
    गणना en : uint8_t {
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
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {
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
        प्रत्यागम names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
};
constexpr bool operator==(ध्रुव VPragmaType& lhs, ध्रुव VPragmaType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VPragmaType& lhs, VPragmaType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, ध्रुव VPragmaType& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VPragmaType& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
सार्वजनिक:
    गणना en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        प्रत्यागम names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    bool isRandomizable() ध्रुव { प्रत्यागम m_e != NONE; }
    bool isRand() ध्रुव { प्रत्यागम m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ध्रुव { प्रत्यागम m_e == RAND_CYCLIC; }
};
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VRandAttr& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    स्थिर ध्रुव std::shared_ptr<ध्रुव string> s_emptyp;  // Holds ""
    स्थिर ध्रुव std::shared_ptr<ध्रुव string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ध्रुव string> m_strp;

सार्वजनिक:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, ध्रुव string& field)
        : m_strp{std::make_shared<ध्रुव string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, ध्रुव string& field)
        : m_strp{std::make_shared<ध्रुव string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() ध्रुव { प्रत्यागम m_strp == s_emptyp; }
    bool isVlSym() ध्रुव { प्रत्यागम m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() ध्रुव { प्रत्यागम m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ध्रुव;
    स्थिर string replaceThis(bool useSelfForThis, ध्रुव string& text);
    ध्रुव std::string& asString() ध्रुव { प्रत्यागम *m_strp; }
    bool operator==(ध्रुव VSelfPointerText& other) ध्रुव { प्रत्यागम *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
सार्वजनिक:
    गणना en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        प्रत्यागम names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    स्थिर VSigning fromBool(bool isSigned) {  // Factory method
        प्रत्यागम isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    bool isSigned() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == SIGNED; }
    bool isNosign() ध्रुव VL_MT_SAFE { प्रत्यागम m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(ध्रुव VSigning& lhs, ध्रुव VSigning& rhs) { प्रत्यागम lhs.m_e == rhs.m_e; }
constexpr bool operator==(ध्रुव VSigning& lhs, VSigning::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, ध्रुव VSigning& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VSigning& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

class VStrength final {
सार्वजनिक:
    गणना en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    गणना en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VStrength& lhs, ध्रुव VStrength& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VStrength& lhs, VStrength::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, ध्रुव VStrength& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VStrength& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
सार्वजनिक:
    गणना en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    गणना en m_e;
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        प्रत्यागम names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
};

//######################################################################

class VTracePrefixType final {
सार्वजनिक:
    गणना en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    गणना en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VTracePrefixType& lhs, ध्रुव VTracePrefixType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, ध्रुव VTracePrefixType& rhs) {
    प्रत्यागम lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VTracePrefixType& rhs) {
    प्रत्यागम os << rhs.ascii();
}

//######################################################################

class VTraceType final {
सार्वजनिक:
    गणना en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    गणना en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"CONSTANT", "FULL", "CHANGE"};
        प्रत्यागम names[m_e];
    }
    ध्रुव वर्ण* func_prefix() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"trace_const", "trace_full", "trace_chg"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VTraceType& lhs, ध्रुव VTraceType& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VTraceType& lhs, VTraceType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, ध्रुव VTraceType& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VTraceType& rhs) {
    प्रत्यागम os << rhs.ascii();
}

//######################################################################

class VUseType final {
सार्वजनिक:
    गणना en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    गणना en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    bool containsAny(VUseType other) ध्रुव { प्रत्यागम m_e & other.m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        प्रत्यागम names[m_e - 1];
    }
};
constexpr bool operator==(ध्रुव VUseType& lhs, ध्रुव VUseType& rhs) { प्रत्यागम lhs.m_e == rhs.m_e; }
constexpr bool operator==(ध्रुव VUseType& lhs, VUseType::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, ध्रुव VUseType& rhs) { प्रत्यागम lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    प्रत्यागम VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    प्रत्यागम VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VUseType& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

class VVarType final {
सार्वजनिक:
    गणना en : uint8_t {
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
    गणना en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर constexpr ध्रुव वर्ण* ध्रुव names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        प्रत्यागम names[m_e];
    }
    bool isParam() ध्रुव { प्रत्यागम m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ध्रुव {
        प्रत्यागम (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ध्रुव {
        प्रत्यागम (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ध्रुव { प्रत्यागम (m_e == TRIOR); }
    bool isWiredNet() ध्रुव { प्रत्यागम (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ध्रुव {  // In Verilog, always ok in SystemVerilog
        प्रत्यागम (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ध्रुव {
        प्रत्यागम (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() ध्रुव {
        प्रत्यागम (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ध्रुव {
        प्रत्यागम (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ध्रुव वर्ण* traceSigKind() ध्रुव {
        // VerilatedTraceSigKind to used in trace signal declaration
        स्थिर constexpr ध्रुव वर्ण* ध्रुव lut[] = {
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
        प्रत्यागम lut[m_e];
    }
};
constexpr bool operator==(ध्रुव VVarType& lhs, ध्रुव VVarType& rhs) VL_MT_SAFE {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, ध्रुव VVarType& rhs) VL_MT_SAFE {
    प्रत्यागम lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VVarType& rhs) VL_MT_SAFE {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_किल्ली final {
सार्वजनिक:
    ध्रुव पूर्णाङ्क m_width;  // From AstNodeDType: Bit width of operation
    ध्रुव पूर्णाङ्क m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ध्रुव VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ध्रुव VSigning m_numeric;  // From AstNodeDType: Node is signed
    ध्रुव VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(ध्रुव VBasic_Type_किल्ली& rhs) ध्रुव {
        प्रत्यागम m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(ध्रुव VBasic_Type_किल्ली& rhs) ध्रुव {
        यदि ((m_width < rhs.m_width)) प्रत्यागम true;
        यदि (!(m_width == rhs.m_width)) प्रत्यागम false;  // lhs > rhs
        यदि ((m_widthMin < rhs.m_widthMin)) प्रत्यागम true;
        यदि (!(m_widthMin == rhs.m_widthMin)) प्रत्यागम false;  // lhs > rhs
        यदि ((m_numeric < rhs.m_numeric)) प्रत्यागम true;
        यदि (!(m_numeric == rhs.m_numeric)) प्रत्यागम false;  // lhs > rhs
        यदि ((m_keyword < rhs.m_keyword)) प्रत्यागम true;
        यदि (!(m_keyword == rhs.m_keyword)) प्रत्यागम false;  // lhs > rhs
        यदि ((m_nrange < rhs.m_nrange)) प्रत्यागम true;
        यदि (!(m_nrange == rhs.m_nrange)) प्रत्यागम false;  // lhs > rhs
        प्रत्यागम false;
    }
    VBasic_Type_किल्ली(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  ध्रुव VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_किल्ली() = स्वभाव;
};

#endif  // Guard
