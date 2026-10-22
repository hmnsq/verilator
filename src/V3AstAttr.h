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
    अचर वर्ण* ascii() अचर {
        स्थिर अचर वर्ण* अचर names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        फर्काऊ names[m_e];
    }
    bool isAccept() अचर { फर्काऊ m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"RD", "WR", "RW", "--"};
        फर्काऊ names[m_e];
    }
    अचर वर्ण* arrow() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        फर्काऊ names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
    VAccess invert() अचर {
        फर्काऊ (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() अचर { फर्काऊ m_e == READ; }  // False with READWRITE
    bool isWriteOnly() अचर { फर्काऊ m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() अचर { फर्काऊ m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() अचर { फर्काऊ m_e == WRITE || m_e == READWRITE; }
    bool isRW() अचर { फर्काऊ m_e == READWRITE; }
};
constexpr bool चालक==(अचर VAccess& lhs, अचर VAccess& rhs) { फर्काऊ lhs.m_e == rhs.m_e; }
constexpr bool चालक==(अचर VAccess& lhs, VAccess::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VAccess::en lhs, अचर VAccess& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VAccess& rhs) { फर्काऊ os << rhs.ascii(); }

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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VAlwaysKwd& lhs, अचर VAlwaysKwd& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VAlwaysKwd::en lhs, अचर VAlwaysKwd& rhs) { फर्काऊ lhs == rhs.m_e; }

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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        // IEEE 1800-2023 20.11
        स्थिर constexpr अचर वर्ण* अचर names[] = {"",
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
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VAssertCtlType& lhs, अचर VAssertCtlType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक==(VAssertCtlType::en lhs, अचर VAssertCtlType& rhs) {
    फर्काऊ lhs == rhs.m_e;
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
    string ascii() अचर {
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
        अचर string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        फर्काऊ str.substr(0, str.size() - 1);
    }
    constexpr चालक en() अचर { फर्काऊ m_e; }
};
constexpr bool चालक==(अचर VAssertDirectiveType& lhs, अचर VAssertDirectiveType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक==(VAssertDirectiveType::en lhs, अचर VAssertDirectiveType& rhs) {
    फर्काऊ lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en चालक|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    फर्काऊ VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
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
    bool containsAny(VAssertType other) अचर { फर्काऊ m_e & other.m_e; }
    string ascii() अचर {
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
        अचर string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        फर्काऊ str.substr(0, str.size() - 1);
    }
    constexpr चालक en() अचर { फर्काऊ m_e; }
};
constexpr bool चालक==(अचर VAssertType& lhs, अचर VAssertType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VAssertType& lhs, VAssertType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VAssertType::en lhs, अचर VAssertType& rhs) { फर्काऊ lhs == rhs.m_e; }
constexpr VAssertType::en चालक|(VAssertType::en lhs, VAssertType::en rhs) {
    फर्काऊ VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"default", "weak", "strong"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VPropStrength& lhs, अचर VPropStrength& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VPropStrength& lhs, VPropStrength::en rhs) {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक!=(अचर VPropStrength& lhs, VPropStrength::en rhs) {
    फर्काऊ lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
सार्वजनिक:
    // clang-format off
    गणना en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_सङ्ख्या,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_माथिल्लो,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_बायाँ,                       // V3Width processes
        DIM_तल्लो,                        // V3Width processes
        DIM_दायाँ,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_अङ्क,                       // V3Width processes
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
    अचर वर्ण* ascii() अचर {
        // clang-format off
        स्थिर constexpr अचर वर्ण* अचर names[] = {
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
        फर्काऊ names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() अचर {
        विकल्प (m_e) {
        पक्ष DIM_BITS:
        पक्ष DIM_BITS_OR_सङ्ख्या:
        पक्ष DIM_DIMENSIONS:
        पक्ष DIM_माथिल्लो:
        पक्ष DIM_INCREMENT:
        पक्ष DIM_बायाँ:
        पक्ष DIM_तल्लो:
        पक्ष DIM_दायाँ:
        पक्ष DIM_SIZE:
        पक्ष DIM_UNPK_DIMENSIONS:
        पक्ष TYPEID:
        पक्ष TYPENAME: फर्काऊ true;
        स्वभाव: फर्काऊ false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
};
constexpr bool चालक==(अचर VAttrType& lhs, अचर VAttrType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VAttrType& lhs, VAttrType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VAttrType::en lhs, अचर VAttrType& rhs) { फर्काऊ lhs == rhs.m_e; }

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
    शून्य combine(अचर VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() अचर { फर्काऊ m_extends | m_final | m_initial; }
    bool isExtends() अचर { फर्काऊ m_extends; }
    bool isFinal() अचर { फर्काऊ m_final; }
    bool isInitial() अचर { फर्काऊ m_initial; }
    string ascii() अचर {
        string out;
        यदि (m_initial) out = VString::dot(out, " ", "initial");
        यदि (m_extends) out = VString::dot(out, " ", "extends");
        यदि (m_final) out = VString::dot(out, " ", "final");
        फर्काऊ out;
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
    अचर वर्ण* ascii() अचर VL_MT_SAFE {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"%E-unk",
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
        फर्काऊ names[m_e];
    }
    अचर वर्ण* typeName() अचर VL_MT_SAFE {
        विकल्प (m_e) {
        पक्ष LOGIC2STATE: फर्काऊ VBasicDTypeKwd{LOGIC}.ascii();
        पक्ष INTEGER2STATE: फर्काऊ VBasicDTypeKwd{INTEGER}.ascii();
        पक्ष TIME2STATE: फर्काऊ VBasicDTypeKwd{TIME}.ascii();
        स्वभाव: फर्काऊ ascii();
        }
    }
    अचर वर्ण* dpiType() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"%E-unk",
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
        फर्काऊ names[m_e];
    }
    स्थिर शून्य selfTest() {
        UASSERT(0 == std::म_भेद(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::म_भेद(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(पूर्णाङ्क _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
    पूर्णाङ्क width() अचर {
        विकल्प (m_e) {
        पक्ष BIT: फर्काऊ 1;  // scalar, can't bit extract unless ranged
        पक्ष BYTE: फर्काऊ 8;
        पक्ष CHANDLE: फर्काऊ 64;
        पक्ष NULLTYPE: फर्काऊ 1;  // Historical - bit was used instead of NULLTYPE
        पक्ष EVENT: फर्काऊ 1;
        पक्ष INT: फर्काऊ 32;
        पक्ष INTEGER: फर्काऊ 32;
        पक्ष INTEGER2STATE: फर्काऊ 32;
        पक्ष LOGIC: फर्काऊ 1;  // scalar, can't bit extract unless ranged
        पक्ष LOGIC2STATE: फर्काऊ 1;  // scalar, can't bit extract unless ranged
        पक्ष LONGINT: फर्काऊ 64;
        पक्ष DOUBLE: फर्काऊ 64;  // opaque
        पक्ष SHORTINT: फर्काऊ 16;
        पक्ष TIME: फर्काऊ 64;
        पक्ष TIME2STATE: फर्काऊ 64;
        पक्ष STRING: फर्काऊ 64;  // opaque  // Just the pointer, for today
        पक्ष SCOPEPTR: फर्काऊ 0;  // opaque
        पक्ष CHARPTR: फर्काऊ 0;  // opaque
        पक्ष MTASKSTATE: फर्काऊ 0;  // opaque
        पक्ष DELAY_SCHEDULER: फर्काऊ 0;  // opaque
        पक्ष TRIGGER_SCHEDULER: फर्काऊ 0;  // opaque
        पक्ष DYNAMIC_TRIGGER_SCHEDULER: फर्काऊ 0;  // opaque
        पक्ष FORK_SYNC: फर्काऊ 0;  // opaque
        पक्ष PROCESS_REFERENCE: फर्काऊ 0;  // opaque
        पक्ष RANDOM_GENERATOR: फर्काऊ 0;  // opaque
        पक्ष RANDOM_STDGENERATOR: फर्काऊ 0;  // opaque
        पक्ष COVERGROUP_INSTHANDLE: फर्काऊ 0;  // opaque
        पक्ष UINT32: फर्काऊ 32;
        पक्ष UINT64: फर्काऊ 64;
        स्वभाव: फर्काऊ 0;
        }
    }
    bool isSigned() अचर {
        फर्काऊ m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() अचर {
        फर्काऊ m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() अचर {
        फर्काऊ m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() अचर {  // Otherwise initializes to X
        फर्काऊ (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() अचर {  // Enum increment supported
        फर्काऊ (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() अचर { फर्काऊ m_e == BIT; }
    bool isBitLogic() अचर {  // Bit/logic vector types; can form a packed array
        फर्काऊ (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() अचर {  // Can add "unsigned" to DPI
        फर्काऊ (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() अचर {  // Uses standard C layout, for DPI runtime access
        फर्काऊ (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() अचर VL_MT_SAFE {  // IE not a simple number we can bit optimize
        फर्काऊ (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() अचर VL_MT_SAFE { फर्काऊ m_e == CHANDLE; }
    bool isDouble() अचर VL_MT_SAFE { फर्काऊ m_e == DOUBLE; }
    bool isEvent() अचर { फर्काऊ m_e == EVENT; }
    bool isString() अचर VL_MT_SAFE { फर्काऊ m_e == STRING; }
    bool isMTaskState() अचर VL_MT_SAFE { फर्काऊ m_e == MTASKSTATE; }
    bool isSameish(अचर VBasicDTypeKwd& other) अचर VL_MT_SAFE {
        विकल्प (other.m_e) {
        पक्ष LOGIC:
        पक्ष LOGIC2STATE:
        पक्ष LOGIC_IMPLICIT: फर्काऊ m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        पक्ष INTEGER:
        पक्ष INTEGER2STATE: फर्काऊ m_e == INTEGER || m_e == INTEGER2STATE;
        पक्ष TIME:
        पक्ष TIME2STATE: फर्काऊ m_e == TIME || m_e == TIME2STATE;
        स्वभाव: फर्काऊ m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() अचर VL_MT_SAFE {
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
        पक्ष UINT64: फर्काऊ true;
        स्वभाव: फर्काऊ false;
        }
    }

    अचर वर्ण* traceSigType() अचर {
        // VerilatedTraceSigType to used in trace signal declaration
        स्थिर constexpr अचर वर्ण* अचर lut[] = {
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
        फर्काऊ lut[m_e];
    }
};
constexpr bool चालक==(अचर VBasicDTypeKwd& lhs, अचर VBasicDTypeKwd& rhs) VL_MT_SAFE {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक==(VBasicDTypeKwd::en lhs, अचर VBasicDTypeKwd& rhs) VL_MT_SAFE {
    फर्काऊ lhs == rhs.m_e;
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"FALSE", "TRUE", "UNK"};
        फर्काऊ names[m_e];
    }
    bool isKnown() अचर { फर्काऊ m_e != BU_UNKNOWN; }
    bool trueKnown() अचर { फर्काऊ m_e == BU_TRUE; }
    शून्य setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool चालक==(अचर VBoolOrUnknown& lhs, अचर VBoolOrUnknown& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक==(VBoolOrUnknown::en lhs, अचर VBoolOrUnknown& rhs) {
    फर्काऊ lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VBoolOrUnknown& rhs) {
    फर्काऊ os << rhs.ascii();
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    bool unknown() अचर { फर्काऊ m_e == BP_UNKNOWN; }
    bool likely() अचर { फर्काऊ m_e == BP_LIKELY; }
    bool unlikely() अचर { फर्काऊ m_e == BP_UNLIKELY; }
    VBranchPred invert() अचर {
        यदि (m_e == BP_UNLIKELY) फर्काऊ BP_LIKELY;
        यदि (m_e == BP_LIKELY) फर्काऊ BP_UNLIKELY;
        फर्काऊ m_e;
    }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        फर्काऊ names[m_e];
    }
    अचर वर्ण* asciiShort() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"", "L", "!L"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VBranchPred& lhs, अचर VBranchPred& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VBranchPred& lhs, VBranchPred::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VBranchPred::en lhs, अचर VBranchPred& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VBranchPred& rhs) {
    फर्काऊ os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
स्थिर constexpr bool validateArgsDescriptor(अचर वर्ण* descrp) {
    // Is "TODO"
    यदि (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        फर्काऊ true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    लागि (अचर वर्ण* cp = descrp; *cp; ++cp) {
        यदि (*cp == '+') फर्काऊ cp != descrp && !cp[1];
        यदि (*cp != 'r' && *cp != 'w' && *cp != 'm') फर्काऊ false;
    }
    फर्काऊ true;
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
    macro(ARRAY_झिक_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_झिक_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_थप_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_थप_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_मान,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_मान_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_मान_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_मान_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_मान_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_मान_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_मान_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_मान_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_मान_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_मान_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_मान_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_घुसाउ,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_झिक,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_झिक_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_थप,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_थप_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर VL_PURE {
        स्थिर constexpr अचर वर्ण* अचर मानहरू[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        फर्काऊ मानहरू[m_e];
    }
    bool isPure() अचर VL_PURE {
        स्थिर constexpr bool मानहरू[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        फर्काऊ मानहरू[m_e];
    }
    bool isClean() अचर VL_PURE {
        स्थिर constexpr bool मानहरू[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        फर्काऊ मानहरू[m_e];
    }
    अचर वर्ण* args() अचर VL_PURE {
        स्थिर constexpr अचर वर्ण* अचर मानहरू[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        फर्काऊ मानहरू[m_e];
    }
    // Return array method for given name
    स्थिर VCMethod arrayMethod(अचर string& name);
};
constexpr bool चालक==(अचर VCMethod& lhs, अचर VCMethod& rhs) { फर्काऊ lhs.m_e == rhs.m_e; }
constexpr bool चालक==(अचर VCMethod& lhs, VCMethod::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VCMethod::en lhs, अचर VCMethod& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VCMethod& rhs) {
    फर्काऊ os << rhs.ascii();
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
    अचर वर्ण* ascii() अचर VL_PURE {
        स्थिर अचर वर्ण* अचर मानहरू[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        फर्काऊ मानहरू[m_e];
    }
    bool isPure() अचर VL_PURE {
        स्थिर अचर bool मानहरू[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        फर्काऊ मानहरू[m_e];
    }
    अचर वर्ण* args() अचर VL_PURE {
        स्थिर अचर वर्ण* अचर मानहरू[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        फर्काऊ मानहरू[m_e];
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
        अचर वर्ण* m_name;  // Statements name, for debugging
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर VL_PURE {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"none", "ctor_var_reset_call"};
        फर्काऊ names[m_e];
    }
    bool isNone() अचर { फर्काऊ m_e == NONE; }
};
constexpr bool चालक==(अचर VCStmtType& lhs, अचर VCStmtType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VCStmtType& lhs, VCStmtType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VCStmtType::en lhs, अचर VCStmtType& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VCStmtType& rhs) {
    फर्काऊ os << rhs.ascii();
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर VL_PURE {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VCaseType& lhs, अचर VCaseType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VCaseType& lhs, VCaseType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VCaseType::en lhs, अचर VCaseType& rhs) { फर्काऊ lhs == rhs.m_e; }

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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        फर्काऊ names[m_e];
    }
    bool isAssignable() अचर { फर्काऊ m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
};
constexpr bool चालक==(अचर VCastable& lhs, अचर VCastable& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VCastable& lhs, VCastable::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VCastable::en lhs, अचर VCastable& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VCastable& rhs) {
    फर्काऊ os << rhs.ascii();
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
    constexpr चालक en() अचर { फर्काऊ m_e; }  // LCOV_EXCL_LINE
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        फर्काऊ names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    अचर वर्ण* verilogKwd() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        फर्काऊ names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    अचर वर्ण* binSetEnum() अचर {
        विकल्प (m_e) {
        पक्ष BINS_IGNORE: फर्काऊ "VlCovBinKind::KIND_IGNORE";
        पक्ष BINS_ILLEGAL: फर्काऊ "VlCovBinKind::KIND_ILLEGAL";
        पक्ष BINS_DEFAULT: फर्काऊ "VlCovBinKind::KIND_DEFAULT";
        स्वभाव: फर्काऊ "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() अचर {
        फर्काऊ m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool चालक==(अचर VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    फर्काऊ lhs.m_e == rhs;
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
        CROSS_अङ्क_PRINT_MISSING,
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"weight",
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
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    फर्काऊ lhs.m_e == rhs;
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
    constexpr चालक en() अचर { फर्काऊ m_e; }  // LCOV_EXCL_LINE
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"", "[*]", "[->]", "[=]"};
        फर्काऊ names[m_e];
    }
    अचर वर्ण* asciiJson() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        फर्काऊ names[m_e];
    }
};

//######################################################################

class VDirection final {
सार्वजनिक:
    गणना en : uint8_t { NONE, आगत, OUTPUT, INOUT, REF, CONSTREF };
    गणना en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर VL_MT_SAFE { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        फर्काऊ names[m_e];
    }
    string verilogKwd() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        फर्काऊ names[m_e];
    }
    string prettyName() अचर { फर्काऊ verilogKwd(); }
    bool isAny() अचर { फर्काऊ m_e != NONE; }
    bool isInout() अचर { फर्काऊ m_e == INOUT; }
    bool isInoutOrRef() अचर { फर्काऊ m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_आगत() अचर { फर्काऊ m_e == आगत; }
    bool isNonOutput() अचर {
        फर्काऊ m_e == आगत || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() अचर { फर्काऊ m_e == OUTPUT; }
    bool isReadOnly() अचर VL_MT_SAFE { फर्काऊ m_e == आगत || m_e == CONSTREF; }
    bool isWritable() अचर VL_MT_SAFE { फर्काऊ m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() अचर VL_MT_SAFE { फर्काऊ m_e == REF; }
    bool isConstRef() अचर VL_MT_SAFE { फर्काऊ m_e == CONSTREF; }
    string traceSigDirection() अचर {
        यदि (isInout()) {
            फर्काऊ "VerilatedTraceSigDirection::INOUT";
        } अन्यथा यदि (isWritable()) {
            फर्काऊ "VerilatedTraceSigDirection::OUTPUT";
        } अन्यथा यदि (isNonOutput()) {
            फर्काऊ "VerilatedTraceSigDirection::INPUT";
        }
        फर्काऊ "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() अचर {
        विकल्प (m_e) {
        पक्ष NONE: फर्काऊ VAccess::NOCHANGE;
        पक्ष आगत:
        पक्ष CONSTREF: फर्काऊ VAccess::READ;
        पक्ष OUTPUT: फर्काऊ VAccess::WRITE;
        पक्ष INOUT:
        पक्ष REF: फर्काऊ VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool चालक==(अचर VDirection& lhs, अचर VDirection& rhs) VL_MT_SAFE {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक==(VDirection::en lhs, अचर VDirection& rhs) VL_MT_SAFE {
    फर्काऊ lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VDirection& rhs) {
    फर्काऊ os << rhs.ascii();
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    bool addNewline() अचर { फर्काऊ m_e != DT_WRITE; }
    bool needScopeTracking() अचर { फर्काऊ m_e != DT_DISPLAY && m_e != DT_WRITE; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VDisplayType& lhs, अचर VDisplayType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VDisplayType& lhs, VDisplayType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VDisplayType::en lhs, अचर VDisplayType& rhs) { फर्काऊ lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
सार्वजनिक:
    गणना en : uint8_t { खाता, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    गणना en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VDumpCtlType& lhs, अचर VDumpCtlType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VDumpCtlType& lhs, VDumpCtlType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VDumpCtlType::en lhs, अचर VDumpCtlType& rhs) { फर्काऊ lhs == rhs.m_e; }

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
    bool clockedStmt() अचर {
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
        फर्काऊ clocked[m_e];
    }
    bool anEdge() अचर { फर्काऊ m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() अचर {
        विकल्प (m_e) {
        पक्ष ET_CHANGED: फर्काऊ ET_CHANGED;
        पक्ष ET_BOTHEDGE: फर्काऊ ET_BOTHEDGE;
        पक्ष ET_POSEDGE: फर्काऊ ET_NEGEDGE;
        पक्ष ET_NEGEDGE: फर्काऊ ET_POSEDGE;
        स्वभाव: UASSERT_STATIC(0, "Inverting bad edgeType()"); फर्काऊ ET_NEGEDGE;
        }
    }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        फर्काऊ names[m_e];
    }
    अचर वर्ण* verilogKwd() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        फर्काऊ names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(अचर VEdgeType& other) अचर {
        विकल्प (m_e) {
        पक्ष VEdgeType::ET_POSEDGE:
            यदि (other.m_e == VEdgeType::ET_NEGEDGE) फर्काऊ true;
            रोक;
        पक्ष VEdgeType::ET_NEGEDGE:
            यदि (other.m_e == VEdgeType::ET_POSEDGE) फर्काऊ true;
            रोक;
        स्वभाव: रोक;
        }
        फर्काऊ false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
};
constexpr bool चालक==(अचर VEdgeType& lhs, अचर VEdgeType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VEdgeType& lhs, VEdgeType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VEdgeType::en lhs, अचर VEdgeType& rhs) { फर्काऊ lhs == rhs.m_e; }

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

    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर मानहरू[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        फर्काऊ मानहरू[m_e];
    }
    bool isIterated() अचर {
        स्थिर constexpr bool मानहरू[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        फर्काऊ मानहरू[m_e];
    }
    bool hasTriggers() अचर {
        स्थिर constexpr bool मानहरू[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        फर्काऊ मानहरू[m_e];
    }
    bool firstIteration() अचर {
        स्थिर constexpr bool मानहरू[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        फर्काऊ मानहरू[m_e];
    }
    bool slow() अचर {
        स्थिर constexpr bool मानहरू[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        फर्काऊ मानहरू[m_e];
    }

    // Short name
    std::string tag() अचर { फर्काऊ VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() अचर { फर्काऊ "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() अचर { फर्काऊ "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() अचर { फर्काऊ "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() अचर { फर्काऊ "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}
    चालक en() अचर { फर्काऊ m_e; }

गुप्त:
    std::string capitalizedTag() अचर {
        अचर std::string name = ascii();
        फर्काऊ name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
सार्वजनिक:
    गणना en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    गणना en m_e;
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        फर्काऊ names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
    // Is a node type compatible with the declaration
    bool is_गाँठो_Compatible(अचर Ast_गाँठो* nodep) अचर;
};
constexpr bool चालक==(अचर VFwdType& lhs, अचर VFwdType& rhs) { फर्काऊ lhs.m_e == rhs.m_e; }
constexpr bool चालक==(अचर VFwdType& lhs, VFwdType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VFwdType::en lhs, अचर VFwdType& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VFwdType& rhs) {
    फर्काऊ os << rhs.ascii();
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
    bool isCached() अचर { फर्काऊ m_cachedCnt == s_cachedCntGbl; }
    bool get() अचर { फर्काऊ m_state; }
    शून्य set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    शून्य clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    स्थिर शून्य clear_Cache_रूख() {
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        फर्काऊ names[m_e];
    }
    अचर वर्ण* verilogKwd() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"join", "join_any", "join_none"};
        फर्काऊ names[m_e];
    }
    bool join() अचर { फर्काऊ m_e == JOIN; }
    bool joinAny() अचर { फर्काऊ m_e == JOIN_ANY; }
    bool joinNone() अचर { फर्काऊ m_e == JOIN_NONE; }
};
constexpr bool चालक==(अचर VJoinType& lhs, अचर VJoinType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VJoinType& lhs, VJoinType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VJoinType::en lhs, अचर VJoinType& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VJoinType& rhs) {
    फर्काऊ os << rhs.ascii();
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        फर्काऊ names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
    bool isNone() अचर { फर्काऊ m_e == NONE; }
    bool isAutomatic() अचर { फर्काऊ m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() अचर { फर्काऊ m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() अचर { फर्काऊ m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() अचर {
        विकल्प (m_e) {
        पक्ष AUTOMATIC_EXPLICIT: फर्काऊ AUTOMATIC_IMPLICIT;
        पक्ष STATIC_EXPLICIT: फर्काऊ STATIC_IMPLICIT;
        स्वभाव: फर्काऊ m_e;
        }
    }
};
constexpr bool चालक==(अचर VLifetime& lhs, अचर VLifetime& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VLifetime& lhs, VLifetime::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VLifetime::en lhs, अचर VLifetime& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VLifetime& rhs) {
    फर्काऊ os << rhs.ascii();
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
            अचर पूर्णाङ्क m_inc;  // Step, +1 or -1
        सार्वजनिक:
            Iterator(पूर्णाङ्क index, पूर्णाङ्क inc)
                : m_index{index}
                , m_inc{inc} {}
            पूर्णाङ्क चालक*() अचर { फर्काऊ m_index; }
            Iterator& चालक++() {
                m_index += m_inc;
                फर्काऊ *this;
            }
            bool चालक!=(अचर Iterator& that) अचर { फर्काऊ m_index != that.m_index; }
        };
        अचर पूर्णाङ्क m_begin;  // First index
        अचर पूर्णाङ्क m_end;  // One past the last index
        अचर पूर्णाङ्क m_inc;  // Step, +1 or -1
    सार्वजनिक:
        Sequence(पूर्णाङ्क first, पूर्णाङ्क last, पूर्णाङ्क inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() अचर { फर्काऊ Iterator{m_begin, m_inc}; }
        Iterator end() अचर { फर्काऊ Iterator{m_end, m_inc}; }
    };

    पूर्णाङ्क m_बायाँ = 0;  // Left side of range (pre-':')
    पूर्णाङ्क m_दायाँ = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool चालक==(अचर VNumRange& rhs) अचर {
        फर्काऊ m_बायाँ == rhs.m_बायाँ && m_दायाँ == rhs.m_दायाँ && m_ranged == rhs.m_ranged;
    }
    bool चालक<(अचर VNumRange& rhs) अचर {
        यदि ((m_बायाँ < rhs.m_बायाँ)) फर्काऊ true;
        यदि (!(m_बायाँ == rhs.m_बायाँ)) फर्काऊ false;  // lhs > rhs
        यदि ((m_दायाँ < rhs.m_दायाँ)) फर्काऊ true;
        यदि (!(m_दायाँ == rhs.m_दायाँ)) फर्काऊ false;  // lhs > rhs
        यदि ((m_ranged < rhs.m_ranged)) फर्काऊ true;
        यदि (!(m_ranged == rhs.m_ranged)) फर्काऊ false;  // lhs > rhs
        फर्काऊ false;
    }
    //
    VNumRange() = स्वभाव;
    VNumRange(पूर्णाङ्क hi, पूर्णाङ्क lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(पूर्णाङ्क बायाँ, पूर्णाङ्क दायाँ)
        : m_बायाँ{बायाँ}
        , m_दायाँ{दायाँ}
        , m_ranged{true} {}
    ~VNumRange() = स्वभाव;
    // MEMBERS
    शून्य init(पूर्णाङ्क hi, पूर्णाङ्क lo, bool ascending) {
        यदि (lo > hi) std::swap(hi, lo);
        m_बायाँ = ascending ? lo : hi;
        m_दायाँ = ascending ? hi : lo;
        m_ranged = true;
    }
    पूर्णाङ्क बायाँ() अचर { फर्काऊ m_बायाँ; }
    पूर्णाङ्क दायाँ() अचर { फर्काऊ m_दायाँ; }
    पूर्णाङ्क hi() अचर VL_MT_SAFE {
        फर्काऊ m_बायाँ > m_दायाँ ? m_बायाँ : m_दायाँ;
    }  // How to show a declaration
    पूर्णाङ्क lo() अचर VL_MT_SAFE {
        फर्काऊ m_बायाँ > m_दायाँ ? m_दायाँ : m_बायाँ;
    }  // How to show a declaration
    पूर्णाङ्क बायाँ_To_दायाँ_Inc() अचर { फर्काऊ ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_बायाँ_To_दायाँ() अचर { फर्काऊ {m_बायाँ, m_दायाँ, बायाँ_To_दायाँ_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_दायाँ_To_बायाँ() अचर { फर्काऊ {m_दायाँ, m_बायाँ, -बायाँ_To_दायाँ_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() अचर { फर्काऊ {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() अचर { फर्काऊ {hi(), lo(), -1}; }
    पूर्णाङ्क elements() अचर VL_MT_SAFE { फर्काऊ hi() - lo() + 1; }
    bool ranged() अचर { फर्काऊ m_ranged; }
    bool ascending() अचर { फर्काऊ m_बायाँ < m_दायाँ; }
    पूर्णाङ्क hiMaxSelect() अचर {
        फर्काऊ (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() अचर {
        यदि (ranged()) {
            फर्काऊ "["s + std::to_string(बायाँ()) + ":" + std::to_string(दायाँ()) + "]";
        } अन्यथा {
            फर्काऊ "[norg]";
        }
    }
    शून्य dump(std::ostream& str) अचर { str << ascii(); }
};
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VNumRange& rhs) {
    rhs.dump(os);
    फर्काऊ os;
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {
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
        फर्काऊ names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
};
constexpr bool चालक==(अचर VPragmaType& lhs, अचर VPragmaType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VPragmaType& lhs, VPragmaType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VPragmaType::en lhs, अचर VPragmaType& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VPragmaType& rhs) {
    फर्काऊ os << rhs.ascii();
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        फर्काऊ names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr चालक en() अचर { फर्काऊ m_e; }
    bool isRandomizable() अचर { फर्काऊ m_e != NONE; }
    bool isRand() अचर { फर्काऊ m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() अचर { फर्काऊ m_e == RAND_CYCLIC; }
};
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VRandAttr& rhs) {
    फर्काऊ os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    स्थिर अचर std::shared_ptr<अचर string> s_emptyp;  // Holds ""
    स्थिर अचर std::shared_ptr<अचर string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<अचर string> m_strp;

सार्वजनिक:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, अचर string& field)
        : m_strp{std::make_shared<अचर string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, अचर string& field)
        : m_strp{std::make_shared<अचर string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() अचर { फर्काऊ m_strp == s_emptyp; }
    bool isVlSym() अचर { फर्काऊ m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() अचर { फर्काऊ m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) अचर;
    स्थिर string replaceThis(bool useSelfForThis, अचर string& text);
    अचर std::string& asString() अचर { फर्काऊ *m_strp; }
    bool चालक==(अचर VSelfPointerText& other) अचर { फर्काऊ *m_strp == *other.m_strp; }
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        फर्काऊ names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    स्थिर VSigning fromBool(bool isSigned) {  // Factory method
        फर्काऊ isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
    bool isSigned() अचर VL_MT_SAFE { फर्काऊ m_e == SIGNED; }
    bool isNosign() अचर VL_MT_SAFE { फर्काऊ m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool चालक==(अचर VSigning& lhs, अचर VSigning& rhs) { फर्काऊ lhs.m_e == rhs.m_e; }
constexpr bool चालक==(अचर VSigning& lhs, VSigning::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VSigning::en lhs, अचर VSigning& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VSigning& rhs) {
    फर्काऊ os << rhs.ascii();
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VStrength& lhs, अचर VStrength& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VStrength& lhs, VStrength::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VStrength::en lhs, अचर VStrength& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VStrength& rhs) {
    फर्काऊ os << rhs.ascii();
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
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        फर्काऊ names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr चालक en() अचर { फर्काऊ m_e; }
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VTracePrefixType& lhs, अचर VTracePrefixType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक==(VTracePrefixType::en lhs, अचर VTracePrefixType& rhs) {
    फर्काऊ lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VTracePrefixType& rhs) {
    फर्काऊ os << rhs.ascii();
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"CONSTANT", "FULL", "CHANGE"};
        फर्काऊ names[m_e];
    }
    अचर वर्ण* func_prefix() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"trace_const", "trace_full", "trace_chg"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VTraceType& lhs, अचर VTraceType& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VTraceType& lhs, VTraceType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VTraceType::en lhs, अचर VTraceType& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VTraceType& rhs) {
    फर्काऊ os << rhs.ascii();
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    bool containsAny(VUseType other) अचर { फर्काऊ m_e & other.m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        फर्काऊ names[m_e - 1];
    }
};
constexpr bool चालक==(अचर VUseType& lhs, अचर VUseType& rhs) { फर्काऊ lhs.m_e == rhs.m_e; }
constexpr bool चालक==(अचर VUseType& lhs, VUseType::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VUseType::en lhs, अचर VUseType& rhs) { फर्काऊ lhs == rhs.m_e; }
constexpr VUseType::en चालक|(VUseType::en lhs, VUseType::en rhs) {
    फर्काऊ VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en चालक&(VUseType::en lhs, VUseType::en rhs) {
    फर्काऊ VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VUseType& rhs) {
    फर्काऊ os << rhs.ascii();
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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर constexpr अचर वर्ण* अचर names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        फर्काऊ names[m_e];
    }
    bool isParam() अचर { फर्काऊ m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() अचर {
        फर्काऊ (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() अचर {
        फर्काऊ (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() अचर { फर्काऊ (m_e == TRIOR); }
    bool isWiredNet() अचर { फर्काऊ (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() अचर {  // In Verilog, always ok in SystemVerilog
        फर्काऊ (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() अचर {
        फर्काऊ (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() अचर {
        फर्काऊ (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() अचर {
        फर्काऊ (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    अचर वर्ण* traceSigKind() अचर {
        // VerilatedTraceSigKind to used in trace signal declaration
        स्थिर constexpr अचर वर्ण* अचर lut[] = {
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
        फर्काऊ lut[m_e];
    }
};
constexpr bool चालक==(अचर VVarType& lhs, अचर VVarType& rhs) VL_MT_SAFE {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक==(VVarType::en lhs, अचर VVarType& rhs) VL_MT_SAFE {
    फर्काऊ lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VVarType& rhs) VL_MT_SAFE {
    फर्काऊ os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_साँचो final {
सार्वजनिक:
    अचर पूर्णाङ्क m_width;  // From AstNodeDType: Bit width of operation
    अचर पूर्णाङ्क m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    अचर VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    अचर VSigning m_numeric;  // From AstNodeDType: Node is signed
    अचर VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool चालक==(अचर VBasic_Type_साँचो& rhs) अचर {
        फर्काऊ m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool चालक<(अचर VBasic_Type_साँचो& rhs) अचर {
        यदि ((m_width < rhs.m_width)) फर्काऊ true;
        यदि (!(m_width == rhs.m_width)) फर्काऊ false;  // lhs > rhs
        यदि ((m_widthMin < rhs.m_widthMin)) फर्काऊ true;
        यदि (!(m_widthMin == rhs.m_widthMin)) फर्काऊ false;  // lhs > rhs
        यदि ((m_numeric < rhs.m_numeric)) फर्काऊ true;
        यदि (!(m_numeric == rhs.m_numeric)) फर्काऊ false;  // lhs > rhs
        यदि ((m_keyword < rhs.m_keyword)) फर्काऊ true;
        यदि (!(m_keyword == rhs.m_keyword)) फर्काऊ false;  // lhs > rhs
        यदि ((m_nrange < rhs.m_nrange)) फर्काऊ true;
        यदि (!(m_nrange == rhs.m_nrange)) फर्काऊ false;  // lhs > rhs
        फर्काऊ false;
    }
    VBasic_Type_साँचो(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  अचर VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_साँचो() = स्वभाव;
};

#endif  // Guard
