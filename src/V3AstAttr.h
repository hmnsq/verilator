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

#अगर_अघोषित VERILATOR_V3ASTATTR_H_
#घोषणा VERILATOR_V3ASTATTR_H_

#अगर_अघोषित VERILATOR_V3AST_H_
#त्रुटि "Use V3Ast.h as the include"
#समावेश "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#घोषणा VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#पूर्ण_अगर

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
खुला:
    // IEEE 1800-2023 16.12.14 property abort operators.
    क्रमागत en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    क्रमागत en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    स्थिर अक्षर* ascii() स्थिर {
        अटल स्थिर अक्षर* स्थिर names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        वापस names[m_e];
    }
    bool isAccept() स्थिर { वापस m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
खुला:
    क्रमागत en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"RD", "WR", "RW", "--"};
        वापस names[m_e];
    }
    स्थिर अक्षर* arrow() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        वापस names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    VAccess invert() स्थिर {
        वापस (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() स्थिर { वापस m_e == READ; }  // False with READWRITE
    bool isWriteOnly() स्थिर { वापस m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() स्थिर { वापस m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() स्थिर { वापस m_e == WRITE || m_e == READWRITE; }
    bool isRW() स्थिर { वापस m_e == READWRITE; }
};
constexpr bool चालक==(स्थिर VAccess& lhs, स्थिर VAccess& rhs) { वापस lhs.m_e == rhs.m_e; }
constexpr bool चालक==(स्थिर VAccess& lhs, VAccess::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VAccess::en lhs, स्थिर VAccess& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VAccess& rhs) { वापस os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
खुला:
    क्रमागत en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    क्रमागत en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VAlwaysKwd& lhs, स्थिर VAlwaysKwd& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VAlwaysKwd::en lhs, स्थिर VAlwaysKwd& rhs) { वापस lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
खुला:
    // IEEE 1800-2023 Table 20-5
    क्रमागत en : uint8_t {
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
    क्रमागत en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        // IEEE 1800-2023 20.11
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"",
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
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VAssertCtlType& lhs, स्थिर VAssertCtlType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VAssertCtlType::en lhs, स्थिर VAssertCtlType& rhs) {
    वापस lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
खुला:
    // IEEE 1800-2023 Table 20-7
    क्रमागत en : uint8_t {
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
    क्रमागत en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() स्थिर {
        std::stringstream types;
        अगर (m_e == INTERNAL)
            types << "INTERNAL ";
        अन्यथा {
            अगर (m_e & ASSERT) types << "ASSERT ";
            अगर (m_e & COVER) types << "COVER ";
            अगर (m_e & ASSUME) types << "ASSUME ";
            अगर (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            अगर (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            अगर (m_e & INTRINSIC) types << "INTRINSIC ";
            अगर (m_e & RESTRICT) types << "RESTRICT ";
        }
        स्थिर string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        वापस str.substr(0, str.आकार() - 1);
    }
    constexpr चालक en() स्थिर { वापस m_e; }
};
constexpr bool चालक==(स्थिर VAssertDirectiveType& lhs, स्थिर VAssertDirectiveType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VAssertDirectiveType::en lhs, स्थिर VAssertDirectiveType& rhs) {
    वापस lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en चालक|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    वापस VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
खुला:
    // IEEE 1800-2023 Table 20-6
    क्रमागत en : uint8_t {
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
    क्रमागत en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) स्थिर { वापस m_e & other.m_e; }
    string ascii() स्थिर {
        std::stringstream types;
        अगर (m_e == INTERNAL)
            types << "INTERNAL ";
        अन्यथा {
            अगर (m_e & CONCURRENT) types << "CONCURRENT ";
            अगर (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            अगर (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            अगर (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            अगर (m_e & EXPECT) types << "EXPECT ";
            अगर (m_e & UNIQUE) types << "UNIQUE ";
            अगर (m_e & UNIQUE0) types << "UNIQUE0 ";
            अगर (m_e & PRIORITY) types << "PRIORITY ";
        }
        स्थिर string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        वापस str.substr(0, str.आकार() - 1);
    }
    constexpr चालक en() स्थिर { वापस m_e; }
};
constexpr bool चालक==(स्थिर VAssertType& lhs, स्थिर VAssertType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VAssertType& lhs, VAssertType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VAssertType::en lhs, स्थिर VAssertType& rhs) { वापस lhs == rhs.m_e; }
constexpr VAssertType::en चालक|(VAssertType::en lhs, VAssertType::en rhs) {
    वापस VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
खुला:
    क्रमागत en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    क्रमागत en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"default", "weak", "strong"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VPropStrength& lhs, स्थिर VPropStrength& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VPropStrength& lhs, VPropStrength::en rhs) {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक!=(स्थिर VPropStrength& lhs, VPropStrength::en rhs) {
    वापस lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
खुला:
    // clang-format off
    क्रमागत en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_संख्या,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_ऊपरी,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_बायाँ,                       // V3Width processes
        DIM_निचला,                        // V3Width processes
        DIM_दायाँ,                      // V3Width processes
        DIM_आकार,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_अंक,                       // V3Width processes
        ENUM_अग्र,                      // V3Width processes
        ENUM_पूर्व,                      // V3Width processes
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
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        // clang-format off
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {
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
        वापस names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() स्थिर {
        चयन (m_e) {
        हाल DIM_BITS:
        हाल DIM_BITS_OR_संख्या:
        हाल DIM_DIMENSIONS:
        हाल DIM_ऊपरी:
        हाल DIM_INCREMENT:
        हाल DIM_बायाँ:
        हाल DIM_निचला:
        हाल DIM_दायाँ:
        हाल DIM_आकार:
        हाल DIM_UNPK_DIMENSIONS:
        हाल TYPEID:
        हाल TYPENAME: वापस true;
        शेष: वापस false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
};
constexpr bool चालक==(स्थिर VAttrType& lhs, स्थिर VAttrType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VAttrType& lhs, VAttrType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VAttrType::en lhs, स्थिर VAttrType& rhs) { वापस lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

खुला:
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
    व्योम combine(स्थिर VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() स्थिर { वापस m_extends | m_final | m_initial; }
    bool isExtends() स्थिर { वापस m_extends; }
    bool isFinal() स्थिर { वापस m_final; }
    bool isInitial() स्थिर { वापस m_initial; }
    string ascii() स्थिर {
        string out;
        अगर (m_initial) out = VString::dot(out, " ", "initial");
        अगर (m_extends) out = VString::dot(out, " ", "extends");
        अगर (m_final) out = VString::dot(out, " ", "final");
        वापस out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
खुला:
    क्रमागत en : uint8_t {
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
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर VL_MT_SAFE {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"%E-unk",
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
        वापस names[m_e];
    }
    स्थिर अक्षर* typeName() स्थिर VL_MT_SAFE {
        चयन (m_e) {
        हाल LOGIC2STATE: वापस VBasicDTypeKwd{LOGIC}.ascii();
        हाल INTEGER2STATE: वापस VBasicDTypeKwd{INTEGER}.ascii();
        हाल TIME2STATE: वापस VBasicDTypeKwd{TIME}.ascii();
        शेष: वापस ascii();
        }
    }
    स्थिर अक्षर* dpiType() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"%E-unk",
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
        वापस names[m_e];
    }
    अटल व्योम selfTest() {
        UASSERT(0 == std::म_भेद(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::म_भेद(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(पूर्णांक _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    पूर्णांक width() स्थिर {
        चयन (m_e) {
        हाल BIT: वापस 1;  // scalar, can't bit extract unless ranged
        हाल BYTE: वापस 8;
        हाल CHANDLE: वापस 64;
        हाल NULLTYPE: वापस 1;  // Historical - bit was used instead of NULLTYPE
        हाल EVENT: वापस 1;
        हाल INT: वापस 32;
        हाल INTEGER: वापस 32;
        हाल INTEGER2STATE: वापस 32;
        हाल LOGIC: वापस 1;  // scalar, can't bit extract unless ranged
        हाल LOGIC2STATE: वापस 1;  // scalar, can't bit extract unless ranged
        हाल LONGINT: वापस 64;
        हाल DOUBLE: वापस 64;  // opaque
        हाल SHORTINT: वापस 16;
        हाल TIME: वापस 64;
        हाल TIME2STATE: वापस 64;
        हाल STRING: वापस 64;  // opaque  // Just the pointer, for today
        हाल SCOPEPTR: वापस 0;  // opaque
        हाल CHARPTR: वापस 0;  // opaque
        हाल MTASKSTATE: वापस 0;  // opaque
        हाल DELAY_SCHEDULER: वापस 0;  // opaque
        हाल TRIGGER_SCHEDULER: वापस 0;  // opaque
        हाल DYNAMIC_TRIGGER_SCHEDULER: वापस 0;  // opaque
        हाल FORK_SYNC: वापस 0;  // opaque
        हाल PROCESS_REFERENCE: वापस 0;  // opaque
        हाल RANDOM_GENERATOR: वापस 0;  // opaque
        हाल RANDOM_STDGENERATOR: वापस 0;  // opaque
        हाल COVERGROUP_INSTHANDLE: वापस 0;  // opaque
        हाल UINT32: वापस 32;
        हाल UINT64: वापस 64;
        शेष: वापस 0;
        }
    }
    bool isSigned() स्थिर {
        वापस m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() स्थिर {
        वापस m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() स्थिर {
        वापस m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() स्थिर {  // Otherwise initializes to X
        वापस (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() स्थिर {  // Enum increment supported
        वापस (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() स्थिर { वापस m_e == BIT; }
    bool isBitLogic() स्थिर {  // Bit/logic vector types; can form a packed array
        वापस (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() स्थिर {  // Can add "unsigned" to DPI
        वापस (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() स्थिर {  // Uses standard C layout, for DPI runtime access
        वापस (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() स्थिर VL_MT_SAFE {  // IE not a simple number we can bit optimize
        वापस (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() स्थिर VL_MT_SAFE { वापस m_e == CHANDLE; }
    bool isDouble() स्थिर VL_MT_SAFE { वापस m_e == DOUBLE; }
    bool isEvent() स्थिर { वापस m_e == EVENT; }
    bool isString() स्थिर VL_MT_SAFE { वापस m_e == STRING; }
    bool isMTaskState() स्थिर VL_MT_SAFE { वापस m_e == MTASKSTATE; }
    bool isSameish(स्थिर VBasicDTypeKwd& other) स्थिर VL_MT_SAFE {
        चयन (other.m_e) {
        हाल LOGIC:
        हाल LOGIC2STATE:
        हाल LOGIC_IMPLICIT: वापस m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        हाल INTEGER:
        हाल INTEGER2STATE: वापस m_e == INTEGER || m_e == INTEGER2STATE;
        हाल TIME:
        हाल TIME2STATE: वापस m_e == TIME || m_e == TIME2STATE;
        शेष: वापस m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() स्थिर VL_MT_SAFE {
        चयन (m_e) {
        हाल BIT:
        हाल BYTE:
        हाल CHANDLE:
        हाल INT:
        हाल INTEGER:
        हाल INTEGER2STATE:
        हाल LOGIC:
        हाल LOGIC2STATE:
        हाल LONGINT:
        हाल DOUBLE:
        हाल SHORTINT:
        हाल SCOPEPTR:
        हाल CHARPTR:
        हाल UINT32:
        हाल UINT64: वापस true;
        शेष: वापस false;
        }
    }

    स्थिर अक्षर* traceSigType() स्थिर {
        // VerilatedTraceSigType to used in trace signal declaration
        अटल constexpr स्थिर अक्षर* स्थिर lut[] = {
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
        static_assert(माप(lut) / माप(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        वापस lut[m_e];
    }
};
constexpr bool चालक==(स्थिर VBasicDTypeKwd& lhs, स्थिर VBasicDTypeKwd& rhs) VL_MT_SAFE {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VBasicDTypeKwd::en lhs, स्थिर VBasicDTypeKwd& rhs) VL_MT_SAFE {
    वापस lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
खुला:
    क्रमागत en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    क्रमागत en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"FALSE", "TRUE", "UNK"};
        वापस names[m_e];
    }
    bool isKnown() स्थिर { वापस m_e != BU_UNKNOWN; }
    bool trueKnown() स्थिर { वापस m_e == BU_TRUE; }
    व्योम setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool चालक==(स्थिर VBoolOrUnknown& lhs, स्थिर VBoolOrUnknown& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VBoolOrUnknown::en lhs, स्थिर VBoolOrUnknown& rhs) {
    वापस lhs == rhs.m_e;
}
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VBoolOrUnknown& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
खुला:
    क्रमागत en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    क्रमागत en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    bool unknown() स्थिर { वापस m_e == BP_UNKNOWN; }
    bool likely() स्थिर { वापस m_e == BP_LIKELY; }
    bool unlikely() स्थिर { वापस m_e == BP_UNLIKELY; }
    VBranchPred invert() स्थिर {
        अगर (m_e == BP_UNLIKELY) वापस BP_LIKELY;
        अगर (m_e == BP_LIKELY) वापस BP_UNLIKELY;
        वापस m_e;
    }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        वापस names[m_e];
    }
    स्थिर अक्षर* asciiShort() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"", "L", "!L"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VBranchPred& lhs, स्थिर VBranchPred& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VBranchPred& lhs, VBranchPred::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VBranchPred::en lhs, स्थिर VBranchPred& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VBranchPred& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
अटल constexpr bool validateArgsDescriptor(स्थिर अक्षर* descrp) {
    // Is "TODO"
    अगर (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        वापस true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    क्रम (स्थिर अक्षर* cp = descrp; *cp; ++cp) {
        अगर (*cp == '+') वापस cp != descrp && !cp[1];
        अगर (*cp != 'r' && *cp != 'w' && *cp != 'm') वापस false;
    }
    वापस true;
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
#घोषणा FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     PURE,   CLEAN,  "r") \
    macro(ARRAY_AT_BACK,                      "atBack",                 PURE,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                PURE,   CLEAN,  "r") \
    macro(ARRAY_FIND,                         "find",                   PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_FIRST,                   "find_first",             PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_FIRST_सूचकांक,             "find_first_index",       PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_सूचकांक,                   "find_index",             PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_LAST,                    "find_last",              PURE,   CLEAN,  "") \
    macro(ARRAY_FIND_LAST_सूचकांक,              "find_last_index",        PURE,   CLEAN,  "") \
    macro(ARRAY_FIRST,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 PURE,   CLEAN,  "r") \
    macro(ARRAY_LAST,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    PURE,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    PURE,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    PURE,   CLEAN,  "") \
    macro(ARRAY_अग्र,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_निकालो_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_निकालो_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_पूर्व,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_धकेलो_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_धकेलो_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_REVERSE,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  PURE,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   PURE,   CLEAN,  "") \
    macro(ARRAY_R_PRODUCT,                    "r_product",              PURE,   false,  "") \
    macro(ARRAY_R_योग,                        "r_sum",                  PURE,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  PURE,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_SLICE,                        "slice",                  PURE,   CLEAN,  "r") \
    macro(ARRAY_SORT,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 PURE,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_सूचकांक,                 "unique_index",           PURE,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 PURE,   CLEAN,  "r") \
    macro(ASSOC_FIRST,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_अग्र,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_आकार,                         "size",                   PURE,   CLEAN,  "") \
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
    macro(COVERGROUP_WITH_अग्र,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_RUN,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_RUN_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_BACK,           "atWriteAppendBack",      false,  CLEAN,  "r") \
    macro(DYN_CLEAR,                          "clear",                  false,  CLEAN,  "") \
    macro(DYN_ERASE,                          "erase",                  false,  CLEAN,  "r") \
    macro(DYN_जोड़ो,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_निकालो,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_निकालो_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_धकेलो,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_धकेलो_FRONT,                     "push_front",             false,  CLEAN,  "r") \
    macro(DYN_RENEW,                          "renew",                  false,  CLEAN,  "r") \
    macro(DYN_RENEW_COPY,                     "renew_copy",             false,  CLEAN,  "rr") \
    macro(DYN_RESIZE,                         "resize",                 false,  CLEAN,  "TODO") \
    macro(DYN_आकार,                           "size",                   PURE,   CLEAN,  "") \
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
    macro(FORCE_READ_सूचकांक,                   "readIndex",              PURE,   CLEAN,  "rr") \
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
    अटल constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    अटल constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

खुला:
    क्रमागत en : uint8_t {
#घोषणा VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#अघोषित VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    क्रमागत en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर VL_PURE {
        अटल constexpr स्थिर अक्षर* स्थिर मूल्यावली[] = {
#घोषणा VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#अघोषित VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        वापस मूल्यावली[m_e];
    }
    bool isPure() स्थिर VL_PURE {
        अटल constexpr bool मूल्यावली[] = {
#घोषणा VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#अघोषित VL_CMETHOD_PURE
                false  //
        };
        वापस मूल्यावली[m_e];
    }
    bool isClean() स्थिर VL_PURE {
        अटल constexpr bool मूल्यावली[] = {
#घोषणा VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#अघोषित VL_CMETHOD_CLEAN
                false  //
        };
        वापस मूल्यावली[m_e];
    }
    स्थिर अक्षर* args() स्थिर VL_PURE {
        अटल constexpr स्थिर अक्षर* स्थिर मूल्यावली[] = {
#घोषणा VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#अघोषित VL_CMETHOD_ARGS
                ""  //
        };
        वापस मूल्यावली[m_e];
    }
    // Return array method for given name
    अटल VCMethod arrayMethod(स्थिर string& name);
};
constexpr bool चालक==(स्थिर VCMethod& lhs, स्थिर VCMethod& rhs) { वापस lhs.m_e == rhs.m_e; }
constexpr bool चालक==(स्थिर VCMethod& lhs, VCMethod::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VCMethod::en lhs, स्थिर VCMethod& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VCMethod& rhs) {
    वापस os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#घोषणा VL_CMETHOD_ARGS_CHECK(id, method, pure, clean, args) \
    static_assert(validateArgsDescriptor(args), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_ARGS_CHECK)
#अघोषित VL_CMETHOD_ARGS_CHECK

#अघोषित FOR_EACH_CMETHOD

// clang-format off
#घोषणा FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_TO_SOLVER_HEX,           "vlToSolverHex",          PURE,   "r")
// clang-format on

class VCFunction final {
    अटल constexpr bool PURE = true;  // For macro expansion of 'pure' field only

खुला:
    क्रमागत en : uint8_t {
#घोषणा VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#अघोषित VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    क्रमागत en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    स्थिर अक्षर* ascii() स्थिर VL_PURE {
        अटल स्थिर अक्षर* स्थिर मूल्यावली[] = {
#घोषणा VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#अघोषित VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        वापस मूल्यावली[m_e];
    }
    bool isPure() स्थिर VL_PURE {
        अटल स्थिर bool मूल्यावली[] = {
#घोषणा VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#अघोषित VL_CFUNCTION_PURE
                false  //
        };
        वापस मूल्यावली[m_e];
    }
    स्थिर अक्षर* args() स्थिर VL_PURE {
        अटल स्थिर अक्षर* स्थिर मूल्यावली[] = {
#घोषणा VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#अघोषित VL_CFUNCTION_ARGS
                ""  //
        };
        वापस मूल्यावली[m_e];
    }
};

// Static assert all argument descriptors are well formed
#घोषणा VL_CFUNCTION_ARGS_CHECK(id, method, pure, args) \
    static_assert(validateArgsDescriptor(args), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS_CHECK)
#अघोषित VL_CFUNCTION_ARGS_CHECK

#अघोषित FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
खुला:
    क्रमागत en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

निजी:
    काष्ठा Item final {
        क्रमागत en m_e;  // Statement's enum mnemonic, for checking
        स्थिर अक्षर* m_name;  // Statements name, for debugging
    };
    अटल Item s_item_तथ्य[];

खुला:
    क्रमागत en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर VL_PURE {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"none", "ctor_var_reset_call"};
        वापस names[m_e];
    }
    bool isNone() स्थिर { वापस m_e == NONE; }
};
constexpr bool चालक==(स्थिर VCStmtType& lhs, स्थिर VCStmtType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VCStmtType& lhs, VCStmtType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VCStmtType::en lhs, स्थिर VCStmtType& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VCStmtType& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
खुला:
    क्रमागत en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    क्रमागत en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर VL_PURE {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VCaseType& lhs, स्थिर VCaseType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VCaseType& lhs, VCaseType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VCaseType::en lhs, स्थिर VCaseType& rhs) { वापस lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
खुला:
    क्रमागत en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        वापस names[m_e];
    }
    bool isAssignable() स्थिर { वापस m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
};
constexpr bool चालक==(स्थिर VCastable& lhs, स्थिर VCastable& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VCastable& lhs, VCastable::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VCastable::en lhs, स्थिर VCastable& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VCastable& rhs) {
    वापस os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
खुला:
    क्रमागत en : uint8_t {
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
    क्रमागत en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr चालक en() स्थिर { वापस m_e; }  // LCOV_EXCL_LINE
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        वापस names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    स्थिर अक्षर* verilogKwd() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        वापस names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    स्थिर अक्षर* binSetEnum() स्थिर {
        चयन (m_e) {
        हाल BINS_IGNORE: वापस "VlCovBinKind::KIND_IGNORE";
        हाल BINS_ILLEGAL: वापस "VlCovBinKind::KIND_ILLEGAL";
        हाल BINS_DEFAULT: वापस "VlCovBinKind::KIND_DEFAULT";
        शेष: वापस "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() स्थिर {
        वापस m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool चालक==(स्थिर VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    वापस lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
खुला:
    क्रमागत en : uint8_t {
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
    क्रमागत en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"weight",
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
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    वापस lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
खुला:
    क्रमागत en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    क्रमागत en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(पूर्णांक _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }  // LCOV_EXCL_LINE
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"", "[*]", "[->]", "[=]"};
        वापस names[m_e];
    }
    स्थिर अक्षर* asciiJson() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        वापस names[m_e];
    }
};

//######################################################################

class VDirection final {
खुला:
    क्रमागत en : uint8_t { NONE, निवेश, OUTPUT, INOUT, REF, CONSTREF };
    क्रमागत en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर VL_MT_SAFE { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        वापस names[m_e];
    }
    string verilogKwd() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        वापस names[m_e];
    }
    string prettyName() स्थिर { वापस verilogKwd(); }
    bool isAny() स्थिर { वापस m_e != NONE; }
    bool isInout() स्थिर { वापस m_e == INOUT; }
    bool isInoutOrRef() स्थिर { वापस m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_निवेश() स्थिर { वापस m_e == निवेश; }
    bool isNonOutput() स्थिर {
        वापस m_e == निवेश || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() स्थिर { वापस m_e == OUTPUT; }
    bool isReadOnly() स्थिर VL_MT_SAFE { वापस m_e == निवेश || m_e == CONSTREF; }
    bool isWritable() स्थिर VL_MT_SAFE { वापस m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() स्थिर VL_MT_SAFE { वापस m_e == REF; }
    bool isConstRef() स्थिर VL_MT_SAFE { वापस m_e == CONSTREF; }
    string traceSigDirection() स्थिर {
        अगर (isInout()) {
            वापस "VerilatedTraceSigDirection::INOUT";
        } अन्यथा अगर (isWritable()) {
            वापस "VerilatedTraceSigDirection::OUTPUT";
        } अन्यथा अगर (isNonOutput()) {
            वापस "VerilatedTraceSigDirection::INPUT";
        }
        वापस "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() स्थिर {
        चयन (m_e) {
        हाल NONE: वापस VAccess::NOCHANGE;
        हाल निवेश:
        हाल CONSTREF: वापस VAccess::READ;
        हाल OUTPUT: वापस VAccess::WRITE;
        हाल INOUT:
        हाल REF: वापस VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool चालक==(स्थिर VDirection& lhs, स्थिर VDirection& rhs) VL_MT_SAFE {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VDirection::en lhs, स्थिर VDirection& rhs) VL_MT_SAFE {
    वापस lhs == rhs.m_e;
}
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VDirection& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
खुला:
    क्रमागत en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    क्रमागत en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    bool addNewline() स्थिर { वापस m_e != DT_WRITE; }
    bool needScopeTracking() स्थिर { वापस m_e != DT_DISPLAY && m_e != DT_WRITE; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VDisplayType& lhs, स्थिर VDisplayType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VDisplayType& lhs, VDisplayType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VDisplayType::en lhs, स्थिर VDisplayType& rhs) { वापस lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
खुला:
    क्रमागत en : uint8_t { खाता, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    क्रमागत en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VDumpCtlType& lhs, स्थिर VDumpCtlType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VDumpCtlType& lhs, VDumpCtlType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VDumpCtlType::en lhs, स्थिर VDumpCtlType& rhs) { वापस lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
खुला:
    // REMEMBER to edit the strings below too
    क्रमागत en : uint8_t {
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
    क्रमागत en m_e;
    bool clockedStmt() स्थिर {
        अटल constexpr bool clocked[] = {
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
        वापस clocked[m_e];
    }
    bool anEdge() स्थिर { वापस m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() स्थिर {
        चयन (m_e) {
        हाल ET_CHANGED: वापस ET_CHANGED;
        हाल ET_BOTHEDGE: वापस ET_BOTHEDGE;
        हाल ET_POSEDGE: वापस ET_NEGEDGE;
        हाल ET_NEGEDGE: वापस ET_POSEDGE;
        शेष: UASSERT_STATIC(0, "Inverting bad edgeType()"); वापस ET_NEGEDGE;
        }
    }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        वापस names[m_e];
    }
    स्थिर अक्षर* verilogKwd() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        वापस names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(स्थिर VEdgeType& other) स्थिर {
        चयन (m_e) {
        हाल VEdgeType::ET_POSEDGE:
            अगर (other.m_e == VEdgeType::ET_NEGEDGE) वापस true;
            अवरोध;
        हाल VEdgeType::ET_NEGEDGE:
            अगर (other.m_e == VEdgeType::ET_POSEDGE) वापस true;
            अवरोध;
        शेष: अवरोध;
        }
        वापस false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
};
constexpr bool चालक==(स्थिर VEdgeType& lhs, स्थिर VEdgeType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VEdgeType& lhs, VEdgeType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VEdgeType::en lhs, स्थिर VEdgeType& rhs) { वापस lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#घोषणा FOR_EACH_EVAL(macro) \
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
खुला:
    क्रमागत en : uint8_t {
#घोषणा VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#अघोषित VL_EVAL_ID
            _ENUM_END
    };
    क्रमागत en m_e;

    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर मूल्यावली[] = {
#घोषणा VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#अघोषित VL_EVAL_NAME
                "_ENUM_END"  //
        };
        वापस मूल्यावली[m_e];
    }
    bool isIterated() स्थिर {
        अटल constexpr bool मूल्यावली[] = {
#घोषणा VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#अघोषित VL_EVAL_IS_ITERATED
                false  //
        };
        वापस मूल्यावली[m_e];
    }
    bool hasTriggers() स्थिर {
        अटल constexpr bool मूल्यावली[] = {
#घोषणा VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#अघोषित VL_EVAL_HAS_TRIGGERS
                false  //
        };
        वापस मूल्यावली[m_e];
    }
    bool firstIteration() स्थिर {
        अटल constexpr bool मूल्यावली[] = {
#घोषणा VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#अघोषित VL_EVAL_FIRST
                false  //
        };
        वापस मूल्यावली[m_e];
    }
    bool slow() स्थिर {
        अटल constexpr bool मूल्यावली[] = {
#घोषणा VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#अघोषित VL_EVAL_SLOW
                false  //
        };
        वापस मूल्यावली[m_e];
    }

    // Short name
    std::string tag() स्थिर { वापस VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() स्थिर { वापस "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() स्थिर { वापस "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() स्थिर { वापस "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() स्थिर { वापस "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}
    चालक en() स्थिर { वापस m_e; }

निजी:
    std::string capitalizedTag() स्थिर {
        स्थिर std::string name = ascii();
        वापस name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#अघोषित FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
खुला:
    क्रमागत en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        वापस names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    // Is a node type compatible with the declaration
    bool is_गाँठ_Compatible(स्थिर Ast_गाँठ* nodep) स्थिर;
};
constexpr bool चालक==(स्थिर VFwdType& lhs, स्थिर VFwdType& rhs) { वापस lhs.m_e == rhs.m_e; }
constexpr bool चालक==(स्थिर VFwdType& lhs, VFwdType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VFwdType::en lhs, स्थिर VFwdType& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VFwdType& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    अटल uint64_t s_cachedCntGbl;  // Global computed count

खुला:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() स्थिर { वापस m_cachedCnt == s_cachedCntGbl; }
    bool get() स्थिर { वापस m_state; }
    व्योम set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    व्योम clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    अटल व्योम clear_Cache_वृक्ष() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
खुला:
    क्रमागत en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    क्रमागत en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        वापस names[m_e];
    }
    स्थिर अक्षर* verilogKwd() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"join", "join_any", "join_none"};
        वापस names[m_e];
    }
    bool join() स्थिर { वापस m_e == JOIN; }
    bool joinAny() स्थिर { वापस m_e == JOIN_ANY; }
    bool joinNone() स्थिर { वापस m_e == JOIN_NONE; }
};
constexpr bool चालक==(स्थिर VJoinType& lhs, स्थिर VJoinType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VJoinType& lhs, VJoinType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VJoinType::en lhs, स्थिर VJoinType& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VJoinType& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
खुला:
    क्रमागत en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        वापस names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    bool isNone() स्थिर { वापस m_e == NONE; }
    bool isAutomatic() स्थिर { वापस m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() स्थिर { वापस m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() स्थिर { वापस m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() स्थिर {
        चयन (m_e) {
        हाल AUTOMATIC_EXPLICIT: वापस AUTOMATIC_IMPLICIT;
        हाल STATIC_EXPLICIT: वापस STATIC_IMPLICIT;
        शेष: वापस m_e;
        }
    }
};
constexpr bool चालक==(स्थिर VLifetime& lhs, स्थिर VLifetime& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VLifetime& lhs, VLifetime::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VLifetime::en lhs, स्थिर VLifetime& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VLifetime& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
खुला:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            पूर्णांक m_सूचकांक;  // Current index
            स्थिर पूर्णांक m_inc;  // Step, +1 or -1
        खुला:
            Iterator(पूर्णांक सूचकांक_2, पूर्णांक inc)
                : m_सूचकांक{सूचकांक_2}
                , m_inc{inc} {}
            पूर्णांक चालक*() स्थिर { वापस m_सूचकांक; }
            Iterator& चालक++() {
                m_सूचकांक += m_inc;
                वापस *this;
            }
            bool चालक!=(स्थिर Iterator& that) स्थिर { वापस m_सूचकांक != that.m_सूचकांक; }
        };
        स्थिर पूर्णांक m_begin;  // First index
        स्थिर पूर्णांक m_end;  // One past the last index
        स्थिर पूर्णांक m_inc;  // Step, +1 or -1
    खुला:
        Sequence(पूर्णांक first, पूर्णांक last, पूर्णांक inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() स्थिर { वापस Iterator{m_begin, m_inc}; }
        Iterator end() स्थिर { वापस Iterator{m_end, m_inc}; }
    };

    पूर्णांक m_बायाँ = 0;  // Left side of range (pre-':')
    पूर्णांक m_दायाँ = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool चालक==(स्थिर VNumRange& rhs) स्थिर {
        वापस m_बायाँ == rhs.m_बायाँ && m_दायाँ == rhs.m_दायाँ && m_ranged == rhs.m_ranged;
    }
    bool चालक<(स्थिर VNumRange& rhs) स्थिर {
        अगर ((m_बायाँ < rhs.m_बायाँ)) वापस true;
        अगर (!(m_बायाँ == rhs.m_बायाँ)) वापस false;  // lhs > rhs
        अगर ((m_दायाँ < rhs.m_दायाँ)) वापस true;
        अगर (!(m_दायाँ == rhs.m_दायाँ)) वापस false;  // lhs > rhs
        अगर ((m_ranged < rhs.m_ranged)) वापस true;
        अगर (!(m_ranged == rhs.m_ranged)) वापस false;  // lhs > rhs
        वापस false;
    }
    //
    VNumRange() = शेष;
    VNumRange(पूर्णांक hi, पूर्णांक lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(पूर्णांक बायाँ, पूर्णांक दायाँ)
        : m_बायाँ{बायाँ}
        , m_दायाँ{दायाँ}
        , m_ranged{true} {}
    ~VNumRange() = शेष;
    // MEMBERS
    व्योम init(पूर्णांक hi, पूर्णांक lo, bool ascending) {
        अगर (lo > hi) std::swap(hi, lo);
        m_बायाँ = ascending ? lo : hi;
        m_दायाँ = ascending ? hi : lo;
        m_ranged = true;
    }
    पूर्णांक बायाँ() स्थिर { वापस m_बायाँ; }
    पूर्णांक दायाँ() स्थिर { वापस m_दायाँ; }
    पूर्णांक hi() स्थिर VL_MT_SAFE {
        वापस m_बायाँ > m_दायाँ ? m_बायाँ : m_दायाँ;
    }  // How to show a declaration
    पूर्णांक lo() स्थिर VL_MT_SAFE {
        वापस m_बायाँ > m_दायाँ ? m_दायाँ : m_बायाँ;
    }  // How to show a declaration
    पूर्णांक बायाँ_To_दायाँ_Inc() स्थिर { वापस ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_बायाँ_To_दायाँ() स्थिर { वापस {m_बायाँ, m_दायाँ, बायाँ_To_दायाँ_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_दायाँ_To_बायाँ() स्थिर { वापस {m_दायाँ, m_बायाँ, -बायाँ_To_दायाँ_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() स्थिर { वापस {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() स्थिर { वापस {hi(), lo(), -1}; }
    पूर्णांक elements() स्थिर VL_MT_SAFE { वापस hi() - lo() + 1; }
    bool ranged() स्थिर { वापस m_ranged; }
    bool ascending() स्थिर { वापस m_बायाँ < m_दायाँ; }
    पूर्णांक hiMaxSelect() स्थिर {
        वापस (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() स्थिर {
        अगर (ranged()) {
            वापस "["s + std::to_string(बायाँ()) + ":" + std::to_string(दायाँ()) + "]";
        } अन्यथा {
            वापस "[norg]";
        }
    }
    व्योम dump(std::ostream& str) स्थिर { str << ascii(); }
};
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VNumRange& rhs) {
    rhs.dump(os);
    वापस os;
}

// ######################################################################

class VPragmaType final {
खुला:
    क्रमागत en : uint8_t {
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
        ENUM_आकार
    };
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {
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
        वापस names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
};
constexpr bool चालक==(स्थिर VPragmaType& lhs, स्थिर VPragmaType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VPragmaType& lhs, VPragmaType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VPragmaType::en lhs, स्थिर VPragmaType& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VPragmaType& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
खुला:
    क्रमागत en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        वापस names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr चालक en() स्थिर { वापस m_e; }
    bool isRandomizable() स्थिर { वापस m_e != NONE; }
    bool isRand() स्थिर { वापस m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() स्थिर { वापस m_e == RAND_CYCLIC; }
};
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VRandAttr& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    अटल स्थिर std::shared_ptr<स्थिर string> s_emptyp;  // Holds ""
    अटल स्थिर std::shared_ptr<स्थिर string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<स्थिर string> m_strp;

खुला:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, स्थिर string& field)
        : m_strp{std::make_shared<स्थिर string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, स्थिर string& field)
        : m_strp{std::make_shared<स्थिर string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() स्थिर { वापस m_strp == s_emptyp; }
    bool isVlSym() स्थिर { वापस m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() स्थिर { वापस m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) स्थिर;
    अटल string replaceThis(bool useSelfForThis, स्थिर string& text);
    स्थिर std::string& asString() स्थिर { वापस *m_strp; }
    bool चालक==(स्थिर VSelfPointerText& other) स्थिर { वापस *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
खुला:
    क्रमागत en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        वापस names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    अटल VSigning fromBool(bool isSigned) {  // Factory method
        वापस isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    bool isSigned() स्थिर VL_MT_SAFE { वापस m_e == SIGNED; }
    bool isNosign() स्थिर VL_MT_SAFE { वापस m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool चालक==(स्थिर VSigning& lhs, स्थिर VSigning& rhs) { वापस lhs.m_e == rhs.m_e; }
constexpr bool चालक==(स्थिर VSigning& lhs, VSigning::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VSigning::en lhs, स्थिर VSigning& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VSigning& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

class VStrength final {
खुला:
    क्रमागत en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    क्रमागत en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VStrength& lhs, स्थिर VStrength& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VStrength& lhs, VStrength::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VStrength::en lhs, स्थिर VStrength& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VStrength& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
खुला:
    क्रमागत en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    क्रमागत en m_e;
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        वापस names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr चालक en() स्थिर { वापस m_e; }
};

//######################################################################

class VTracePrefixType final {
खुला:
    क्रमागत en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    क्रमागत en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VTracePrefixType& lhs, स्थिर VTracePrefixType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VTracePrefixType::en lhs, स्थिर VTracePrefixType& rhs) {
    वापस lhs == rhs.m_e;
}
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VTracePrefixType& rhs) {
    वापस os << rhs.ascii();
}

//######################################################################

class VTraceType final {
खुला:
    क्रमागत en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    क्रमागत en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"CONSTANT", "FULL", "CHANGE"};
        वापस names[m_e];
    }
    स्थिर अक्षर* func_prefix() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"trace_const", "trace_full", "trace_chg"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VTraceType& lhs, स्थिर VTraceType& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VTraceType& lhs, VTraceType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VTraceType::en lhs, स्थिर VTraceType& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VTraceType& rhs) {
    वापस os << rhs.ascii();
}

//######################################################################

class VUseType final {
खुला:
    क्रमागत en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    क्रमागत en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    bool containsAny(VUseType other) स्थिर { वापस m_e & other.m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        वापस names[m_e - 1];
    }
};
constexpr bool चालक==(स्थिर VUseType& lhs, स्थिर VUseType& rhs) { वापस lhs.m_e == rhs.m_e; }
constexpr bool चालक==(स्थिर VUseType& lhs, VUseType::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VUseType::en lhs, स्थिर VUseType& rhs) { वापस lhs == rhs.m_e; }
constexpr VUseType::en चालक|(VUseType::en lhs, VUseType::en rhs) {
    वापस VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en चालक&(VUseType::en lhs, VUseType::en rhs) {
    वापस VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VUseType& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

class VVarType final {
खुला:
    क्रमागत en : uint8_t {
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
    क्रमागत en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल constexpr स्थिर अक्षर* स्थिर names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        वापस names[m_e];
    }
    bool isParam() स्थिर { वापस m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() स्थिर {
        वापस (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() स्थिर {
        वापस (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() स्थिर { वापस (m_e == TRIOR); }
    bool isWiredNet() स्थिर { वापस (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() स्थिर {  // In Verilog, always ok in SystemVerilog
        वापस (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() स्थिर {
        वापस (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool is_अस्थायी() स्थिर {
        वापस (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() स्थिर {
        वापस (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    स्थिर अक्षर* traceSigKind() स्थिर {
        // VerilatedTraceSigKind to used in trace signal declaration
        अटल constexpr स्थिर अक्षर* स्थिर lut[] = {
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
        वापस lut[m_e];
    }
};
constexpr bool चालक==(स्थिर VVarType& lhs, स्थिर VVarType& rhs) VL_MT_SAFE {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VVarType::en lhs, स्थिर VVarType& rhs) VL_MT_SAFE {
    वापस lhs == rhs.m_e;
}
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VVarType& rhs) VL_MT_SAFE {
    वापस os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_कुंजी final {
खुला:
    स्थिर पूर्णांक m_width;  // From AstNodeDType: Bit width of operation
    स्थिर पूर्णांक m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    स्थिर VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    स्थिर VSigning m_numeric;  // From AstNodeDType: Node is signed
    स्थिर VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool चालक==(स्थिर VBasic_Type_कुंजी& rhs) स्थिर {
        वापस m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool चालक<(स्थिर VBasic_Type_कुंजी& rhs) स्थिर {
        अगर ((m_width < rhs.m_width)) वापस true;
        अगर (!(m_width == rhs.m_width)) वापस false;  // lhs > rhs
        अगर ((m_widthMin < rhs.m_widthMin)) वापस true;
        अगर (!(m_widthMin == rhs.m_widthMin)) वापस false;  // lhs > rhs
        अगर ((m_numeric < rhs.m_numeric)) वापस true;
        अगर (!(m_numeric == rhs.m_numeric)) वापस false;  // lhs > rhs
        अगर ((m_keyword < rhs.m_keyword)) वापस true;
        अगर (!(m_keyword == rhs.m_keyword)) वापस false;  // lhs > rhs
        अगर ((m_nrange < rhs.m_nrange)) वापस true;
        अगर (!(m_nrange == rhs.m_nrange)) वापस false;  // lhs > rhs
        वापस false;
    }
    VBasic_Type_कुंजी(पूर्णांक width, पूर्णांक widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  स्थिर VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_कुंजी() = शेष;
};

#पूर्ण_अगर  // Guard
