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

#నిర్వచితం_కాకపోతే VERILATOR_V3ASTATTR_H_
#నిర్వచించు VERILATOR_V3ASTATTR_H_

#నిర్వచితం_కాకపోతే VERILATOR_V3AST_H_
#దోషం "Use V3Ast.h as the include"
#అన్తర్భావ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#నిర్వచించు VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#అయితే_ముగింపు

// Hint class so we can choose constructors
class VFlag_బిట్టు_Packed {};
class VFlag_బిడ్డ_DType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbort_రకం final {
బహిరంగ:
    // IEEE 1800-2023 16.12.14 property abort operators.
    గణన en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    గణన en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbort_రకం(en _e)
        : m_e{_e} {}
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        తిరిగి పేర్లు[m_e];
    }
    bool isAccept() స్థిరరాశి { తిరిగి m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
బహిరంగ:
    గణన en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"RD", "WR", "RW", "--"};
        తిరిగి పేర్లు[m_e];
    }
    స్థిరరాశి అక్షరం* arrow() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        తిరిగి పేర్లు[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    VAccess invert() స్థిరరాశి {
        తిరిగి (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() స్థిరరాశి { తిరిగి m_e == READ; }  // False with READWRITE
    bool isWriteOnly() స్థిరరాశి { తిరిగి m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() స్థిరరాశి { తిరిగి m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() స్థిరరాశి { తిరిగి m_e == WRITE || m_e == READWRITE; }
    bool isRW() స్థిరరాశి { తిరిగి m_e == READWRITE; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VAccess& lhs, స్థిరరాశి VAccess& rhs) { తిరిగి lhs.m_e == rhs.m_e; }
constexpr bool క్రియాకారకం==(స్థిరరాశి VAccess& lhs, VAccess::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VAccess::en lhs, స్థిరరాశి VAccess& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VAccess& rhs) { తిరిగి os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
బహిరంగ:
    గణన en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_కేటాయించు };
    గణన en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VAlwaysKwd& lhs, స్థిరరాశి VAlwaysKwd& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VAlwaysKwd::en lhs, స్థిరరాశి VAlwaysKwd& rhs) { తిరిగి lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
బహిరంగ:
    // IEEE 1800-2023 Table 20-5
    గణన en : uint8_t {
        గమ్యం_BE_EVALUATED = 0,
        తాళం = 1,
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
    గణన en m_e;
    VAssertCtlType()
        : m_e{గమ్యం_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        // IEEE 1800-2023 20.11
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"",
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
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VAssertCtlType& lhs, స్థిరరాశి VAssertCtlType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VAssertCtlType::en lhs, స్థిరరాశి VAssertCtlType& rhs) {
    తిరిగి lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
బహిరంగ:
    // IEEE 1800-2023 Table 20-7
    గణన en : uint8_t {
        అంతర్గత = 0,  // Non IEEE type, for directives to be evaluated from expression.
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
    గణన en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() స్థిరరాశి {
        std::stringstream types;
        ఒకవేళ (m_e == అంతర్గత)
            types << "INTERNAL ";
        లేకపోతే {
            ఒకవేళ (m_e & ASSERT) types << "ASSERT ";
            ఒకవేళ (m_e & COVER) types << "COVER ";
            ఒకవేళ (m_e & ASSUME) types << "ASSUME ";
            ఒకవేళ (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            ఒకవేళ (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            ఒకవేళ (m_e & INTRINSIC) types << "INTRINSIC ";
            ఒకవేళ (m_e & RESTRICT) types << "RESTRICT ";
        }
        స్థిరరాశి string str = types.str();
        UASSERT(!str.ఖాళీలు(), "Assert should be of one of types");
        తిరిగి str.substr(0, str.కొలత() - 1);
    }
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VAssertDirectiveType& lhs, స్థిరరాశి VAssertDirectiveType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VAssertDirectiveType::en lhs, స్థిరరాశి VAssertDirectiveType& rhs) {
    తిరిగి lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en క్రియాకారకం|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    తిరిగి VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
బహిరంగ:
    // IEEE 1800-2023 Table 20-6
    గణన en : uint8_t {
        అంతర్గత = 0,  // Non IEEE type, for assertions that should not be controlled.
        CONCURRENT = (1 << 0),
        SIMPLE_తక్షణ = (1 << 1),
        OBSERVED_DEFERRED_తక్షణ = (1 << 2),
        FINAL_DEFERRED_తక్షణ = (1 << 3),
        EXPECT = (1 << 4),
        UNIQUE = (1 << 5),
        UNIQUE0 = (1 << 6),
        ప్రాధాన్యత = (1 << 7),
    };
    గణన en m_e;
    VAssertType()
        : m_e{అంతర్గత} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool కలిగియుంది_Any(VAssertType other) స్థిరరాశి { తిరిగి m_e & other.m_e; }
    string ascii() స్థిరరాశి {
        std::stringstream types;
        ఒకవేళ (m_e == అంతర్గత)
            types << "INTERNAL ";
        లేకపోతే {
            ఒకవేళ (m_e & CONCURRENT) types << "CONCURRENT ";
            ఒకవేళ (m_e & SIMPLE_తక్షణ) types << "SIMPLE_IMMEDIATE ";
            ఒకవేళ (m_e & OBSERVED_DEFERRED_తక్షణ) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            ఒకవేళ (m_e & FINAL_DEFERRED_తక్షణ) types << "FINAL_DEFERRED_IMMEDIATE ";
            ఒకవేళ (m_e & EXPECT) types << "EXPECT ";
            ఒకవేళ (m_e & UNIQUE) types << "UNIQUE ";
            ఒకవేళ (m_e & UNIQUE0) types << "UNIQUE0 ";
            ఒకవేళ (m_e & ప్రాధాన్యత) types << "PRIORITY ";
        }
        స్థిరరాశి string str = types.str();
        UASSERT(!str.ఖాళీలు(), "Assert should be of one of types");
        తిరిగి str.substr(0, str.కొలత() - 1);
    }
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VAssertType& lhs, స్థిరరాశి VAssertType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VAssertType& lhs, VAssertType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VAssertType::en lhs, స్థిరరాశి VAssertType& rhs) { తిరిగి lhs == rhs.m_e; }
constexpr VAssertType::en క్రియాకారకం|(VAssertType::en lhs, VAssertType::en rhs) {
    తిరిగి VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
బహిరంగ:
    గణన en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    గణన en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"default", "weak", "strong"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VPropStrength& lhs, స్థిరరాశి VPropStrength& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VPropStrength& lhs, VPropStrength::en rhs) {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం!=(స్థిరరాశి VPropStrength& lhs, VPropStrength::en rhs) {
    తిరిగి lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
బహిరంగ:
    // clang-format off
    గణన en: uint8_t  {
        ILLEGAL,
        //
        DIM_బిట్లు,                       // V3Const converts to constant
        DIM_బిట్లు_OR_సంఖ్య,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_ఎగువ,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_ఎడమ,                       // V3Width processes
        DIM_దిగువ,                        // V3Width processes
        DIM_కుడి,                      // V3Width processes
        DIM_కొలత,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_ప్రజా,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_మొదటి,                     // V3Width processes
        ENUM_చివరి,                      // V3Width processes
        ENUM_అంకె,                       // V3Width processes
        ENUM_తదుపరి,                      // V3Width processes
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
        VAR_పునాది,                       // V3LinkResolve creates for AstPreSel, V3LinkParam removes
        VAR_FORCEABLE,                  // V3LinkParse moves to AstVar::isForceable
        VAR_FSM_ARC_INCLUDE_COND,       // V3LinkParse moves to AstVar::attrFsmArcInclCond
        VAR_FSM_ARC_INCLUDE_COND_AUTO,  // Flavors of above option - setting expand mode per FSM
        VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND,
        VAR_FSM_ARC_INCLUDE_COND_నిండిన,
        VAR_FSM_STATE,                  // V3LinkParse moves to AstVar::attrFsmState
        VAR_FSM_STATE_AUTO,             // Flavors of above option - setting expand mode per FSM
        VAR_FSM_STATE_AUTO_EXPAND,
        VAR_FSM_STATE_నిండిన,
        VAR_పోర్టు_DTYPE,                 // V3LinkDot for V3Width to check port dtype
        VAR_ప్రజా,                     // V3LinkParse moves to AstVar::sigPublic
        VAR_ప్రజా_FLAT,                // V3LinkParse moves to AstVar::sigPublic
        VAR_ప్రజా_FLAT_RD,             // V3LinkParse moves to AstVar::sigPublic
        VAR_ప్రజా_FLAT_RW,             // V3LinkParse moves to AstVar::sigPublic
        VAR_SC_BIGUINT,                 // V3LinkParse moves to AstVar::attrScBigUint
        VAR_SC_BV,                      // V3LinkParse moves to AstVar::attrScBv
        VAR_SFORMAT,                    // V3LinkParse moves to AstVar::attrSFormat
        VAR_విభజించు_VAR                   // V3LinkParse moves to AstVar::attrSplitVar
    };
    // clang-format on
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        // clang-format off
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {
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
        తిరిగి పేర్లు[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() స్థిరరాశి {
        ఎంపిక (m_e) {
        స్థితి DIM_బిట్లు:
        స్థితి DIM_బిట్లు_OR_సంఖ్య:
        స్థితి DIM_DIMENSIONS:
        స్థితి DIM_ఎగువ:
        స్థితి DIM_INCREMENT:
        స్థితి DIM_ఎడమ:
        స్థితి DIM_దిగువ:
        స్థితి DIM_కుడి:
        స్థితి DIM_కొలత:
        స్థితి DIM_UNPK_DIMENSIONS:
        స్థితి TYPEID:
        స్థితి TYPENAME: తిరిగి true;
        అప్రమేయ: తిరిగి false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VAttrType& lhs, స్థిరరాశి VAttrType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VAttrType& lhs, VAttrType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VAttrType::en lhs, స్థిరరాశి VAttrType& rhs) { తిరిగి lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

బహిరంగ:
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
    శూన్య combine(స్థిరరాశి VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() స్థిరరాశి { తిరిగి m_extends | m_final | m_initial; }
    bool isExtends() స్థిరరాశి { తిరిగి m_extends; }
    bool isFinal() స్థిరరాశి { తిరిగి m_final; }
    bool isInitial() స్థిరరాశి { తిరిగి m_initial; }
    string ascii() స్థిరరాశి {
        string బహిర్గతం;
        ఒకవేళ (m_initial) బహిర్గతం = VString::dot(బహిర్గతం, " ", "initial");
        ఒకవేళ (m_extends) బహిర్గతం = VString::dot(బహిర్గతం, " ", "extends");
        ఒకవేళ (m_final) బహిర్గతం = VString::dot(బహిర్గతం, " ", "final");
        తిరిగి బహిర్గతం;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
బహిరంగ:
    గణన en : uint8_t {
        UNKNOWN,
        బిట్టు,
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
        సమయం,
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
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి VL_MT_సురక్షిత {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"%E-unk",
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
        తిరిగి పేర్లు[m_e];
    }
    స్థిరరాశి అక్షరం* typeName() స్థిరరాశి VL_MT_సురక్షిత {
        ఎంపిక (m_e) {
        స్థితి LOGIC2STATE: తిరిగి VBasicDTypeKwd{LOGIC}.ascii();
        స్థితి INTEGER2STATE: తిరిగి VBasicDTypeKwd{INTEGER}.ascii();
        స్థితి TIME2STATE: తిరిగి VBasicDTypeKwd{సమయం}.ascii();
        అప్రమేయ: తిరిగి ascii();
        }
    }
    స్థిరరాశి అక్షరం* dpiType() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"%E-unk",
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
        తిరిగి పేర్లు[m_e];
    }
    స్థిర శూన్య selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_సురక్షిత : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_సురక్షిత : m_e{_e} {}
    explicit VBasicDTypeKwd(పూర్ణాంకం _e) VL_MT_సురక్షిత  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    పూర్ణాంకం వెడల్పు() స్థిరరాశి {
        ఎంపిక (m_e) {
        స్థితి బిట్టు: తిరిగి 1;  // scalar, can't bit extract unless ranged
        స్థితి BYTE: తిరిగి 8;
        స్థితి CHANDLE: తిరిగి 64;
        స్థితి NULLTYPE: తిరిగి 1;  // Historical - bit was used instead of NULLTYPE
        స్థితి EVENT: తిరిగి 1;
        స్థితి INT: తిరిగి 32;
        స్థితి INTEGER: తిరిగి 32;
        స్థితి INTEGER2STATE: తిరిగి 32;
        స్థితి LOGIC: తిరిగి 1;  // scalar, can't bit extract unless ranged
        స్థితి LOGIC2STATE: తిరిగి 1;  // scalar, can't bit extract unless ranged
        స్థితి LONGINT: తిరిగి 64;
        స్థితి DOUBLE: తిరిగి 64;  // opaque
        స్థితి SHORTINT: తిరిగి 16;
        స్థితి సమయం: తిరిగి 64;
        స్థితి TIME2STATE: తిరిగి 64;
        స్థితి STRING: తిరిగి 64;  // opaque  // Just the pointer, for today
        స్థితి SCOPEPTR: తిరిగి 0;  // opaque
        స్థితి CHARPTR: తిరిగి 0;  // opaque
        స్థితి MTASKSTATE: తిరిగి 0;  // opaque
        స్థితి DELAY_SCHEDULER: తిరిగి 0;  // opaque
        స్థితి TRIGGER_SCHEDULER: తిరిగి 0;  // opaque
        స్థితి DYNAMIC_TRIGGER_SCHEDULER: తిరిగి 0;  // opaque
        స్థితి FORK_SYNC: తిరిగి 0;  // opaque
        స్థితి PROCESS_REFERENCE: తిరిగి 0;  // opaque
        స్థితి RANDOM_GENERATOR: తిరిగి 0;  // opaque
        స్థితి RANDOM_STDGENERATOR: తిరిగి 0;  // opaque
        స్థితి COVERGROUP_INSTHANDLE: తిరిగి 0;  // opaque
        స్థితి UINT32: తిరిగి 32;
        స్థితి UINT64: తిరిగి 64;
        అప్రమేయ: తిరిగి 0;
        }
    }
    bool isSigned() స్థిరరాశి {
        తిరిగి m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() స్థిరరాశి {
        తిరిగి m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == బిట్టు || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == సమయం || m_e == TIME2STATE;
    }
    bool isFourstate() స్థిరరాశి {
        తిరిగి m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == సమయం;
    }
    bool isZeroInit() స్థిరరాశి {  // Otherwise initializes to X
        తిరిగి (m_e == బిట్టు || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() స్థిరరాశి {  // Enum increment supported
        తిరిగి (m_e == బిట్టు || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == సమయం || m_e == TIME2STATE);
    }
    bool is_బిట్టు() స్థిరరాశి { తిరిగి m_e == బిట్టు; }
    bool is_బిట్టు_Logic() స్థిరరాశి {  // Bit/logic vector types; can form a packed array
        తిరిగి (m_e == LOGIC || m_e == LOGIC2STATE || m_e == బిట్టు);
    }
    bool isDpiUnsignable() స్థిరరాశి {  // Can add "unsigned" to DPI
        తిరిగి (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() స్థిరరాశి {  // Uses standard C layout, for DPI runtime access
        తిరిగి (m_e == బిట్టు || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() స్థిరరాశి VL_MT_సురక్షిత {  // IE not a simple number we can bit optimize
        తిరిగి (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == CHANDLE; }
    bool isDouble() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == DOUBLE; }
    bool isEvent() స్థిరరాశి { తిరిగి m_e == EVENT; }
    bool isString() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == STRING; }
    bool isMTaskState() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == MTASKSTATE; }
    bool isSameish(స్థిరరాశి VBasicDTypeKwd& other) స్థిరరాశి VL_MT_సురక్షిత {
        ఎంపిక (other.m_e) {
        స్థితి LOGIC:
        స్థితి LOGIC2STATE:
        స్థితి LOGIC_IMPLICIT: తిరిగి m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        స్థితి INTEGER:
        స్థితి INTEGER2STATE: తిరిగి m_e == INTEGER || m_e == INTEGER2STATE;
        స్థితి సమయం:
        స్థితి TIME2STATE: తిరిగి m_e == సమయం || m_e == TIME2STATE;
        అప్రమేయ: తిరిగి m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() స్థిరరాశి VL_MT_సురక్షిత {
        ఎంపిక (m_e) {
        స్థితి బిట్టు:
        స్థితి BYTE:
        స్థితి CHANDLE:
        స్థితి INT:
        స్థితి INTEGER:
        స్థితి INTEGER2STATE:
        స్థితి LOGIC:
        స్థితి LOGIC2STATE:
        స్థితి LONGINT:
        స్థితి DOUBLE:
        స్థితి SHORTINT:
        స్థితి SCOPEPTR:
        స్థితి CHARPTR:
        స్థితి UINT32:
        స్థితి UINT64: తిరిగి true;
        అప్రమేయ: తిరిగి false;
        }
    }

    స్థిరరాశి అక్షరం* traceSigType() స్థిరరాశి {
        // VerilatedTraceSigType to used in trace signal declaration
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి lut[] = {
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
        static_assert(పరిమాణం(lut) / పరిమాణం(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        తిరిగి lut[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VBasicDTypeKwd& lhs, స్థిరరాశి VBasicDTypeKwd& rhs) VL_MT_సురక్షిత {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_సురక్షిత {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VBasicDTypeKwd::en lhs, స్థిరరాశి VBasicDTypeKwd& rhs) VL_MT_సురక్షిత {
    తిరిగి lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
బహిరంగ:
    గణన en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, ENUM_చివర };
    గణన en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"FALSE", "TRUE", "UNK"};
        తిరిగి పేర్లు[m_e];
    }
    bool is_తెలిసిన() స్థిరరాశి { తిరిగి m_e != BU_UNKNOWN; }
    bool true_తెలిసిన() స్థిరరాశి { తిరిగి m_e == BU_TRUE; }
    శూన్య సమితి_True_Or_False(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VBoolOrUnknown& lhs, స్థిరరాశి VBoolOrUnknown& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VBoolOrUnknown::en lhs, స్థిరరాశి VBoolOrUnknown& rhs) {
    తిరిగి lhs == rhs.m_e;
}
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VBoolOrUnknown& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
బహిరంగ:
    గణన en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, ENUM_చివర };
    గణన en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    bool unknown() స్థిరరాశి { తిరిగి m_e == BP_UNKNOWN; }
    bool likely() స్థిరరాశి { తిరిగి m_e == BP_LIKELY; }
    bool unlikely() స్థిరరాశి { తిరిగి m_e == BP_UNLIKELY; }
    VBranchPred invert() స్థిరరాశి {
        ఒకవేళ (m_e == BP_UNLIKELY) తిరిగి BP_LIKELY;
        ఒకవేళ (m_e == BP_LIKELY) తిరిగి BP_UNLIKELY;
        తిరిగి m_e;
    }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        తిరిగి పేర్లు[m_e];
    }
    స్థిరరాశి అక్షరం* asciiShort() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"", "L", "!L"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VBranchPred& lhs, స్థిరరాశి VBranchPred& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VBranchPred& lhs, VBranchPred::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VBranchPred::en lhs, స్థిరరాశి VBranchPred& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VBranchPred& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
స్థిర constexpr bool validate_వాదనలు_Descriptor(స్థిరరాశి అక్షరం* descrp) {
    // Is "TODO"
    ఒకవేళ (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        తిరిగి true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    కొరకు (స్థిరరాశి అక్షరం* cp = descrp; *cp; ++cp) {
        ఒకవేళ (*cp == '+') తిరిగి cp != descrp && !cp[1];
        ఒకవేళ (*cp != 'r' && *cp != 'w' && *cp != 'm') తిరిగి false;
    }
    తిరిగి true;
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
#నిర్వచించు FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     శుద్ధ_2,   CLEAN,  "r") \
    macro(ARRAY_AT_వెనుక,                      "atBack",                 శుద్ధ_2,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                శుద్ధ_2,   CLEAN,  "r") \
    macro(ARRAY_కనుగొను,                         "find",                   శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_కనుగొను_మొదటి,                   "find_first",             శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_కనుగొను_మొదటి_సూచీ,             "find_first_index",       శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_కనుగొను_సూచీ,                   "find_index",             శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_కనుగొను_చివరి,                    "find_last",              శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_కనుగొను_చివరి_సూచీ,              "find_last_index",        శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_మొదటి,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 శుద్ధ_2,   CLEAN,  "r") \
    macro(ARRAY_చివరి,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_తదుపరి,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_తీయి_వెనుక,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_తీయి_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_నెట్టు_వెనుక,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_నెట్టు_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_తిరగేయి,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_R_లబ్ధం,                    "r_product",              శుద్ధ_2,   false,  "") \
    macro(ARRAY_R_కూడిక,                        "r_sum",                  శుద్ధ_2,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_ముక్క,                        "slice",                  శుద్ధ_2,   CLEAN,  "r") \
    macro(ARRAY_క్రమీకరించు,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 శుద్ధ_2,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_సూచీ,                 "unique_index",           శుద్ధ_2,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 శుద్ధ_2,   CLEAN,  "r") \
    macro(ASSOC_మొదటి,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_తదుపరి,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_కొలత,                         "size",                   శుద్ధ_2,   CLEAN,  "") \
    macro(CLASS_సమితి_RANDMODE,                 "set_randmode",           false,  CLEAN,  "r") \
    macro(COVERGROUP_కూడు_ARRAY_NAMER,         "addArrayNamer",          false,  CLEAN,  "r+") \
    macro(COVERGROUP_కూడు_BIN,                 "addBin",                 false,  CLEAN,  "r+") \
    macro(COVERGROUP_కూడు_COVERPOINT,          "addCoverpoint",          false,  CLEAN,  "") \
    macro(COVERGROUP_కూడు_CROSS,               "addCross",               false,  CLEAN,  "") \
    macro(COVERGROUP_కూడు_CROSS_DYN,           "addCrossDyn",            false,  CLEAN,  "") \
    macro(COVERGROUP_కూడు_NUMBERED_NAMER,      "addNumberedNamer",       false,  CLEAN,  "r+") \
    macro(COVERGROUP_కూడు_SINGLE_NAMER,        "addSingleNamer",         false,  CLEAN,  "r+") \
    macro(COVERGROUP_ATTACH,                  "attach",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_CLEAR_తగిలింది_LIST,          "clearHitList",           false,  CLEAN,  "") \
    macro(COVERGROUP_COVERAGE,                "coverage",               false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_BEGIN,        "withBegin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_CROSS_WITH_చివర,          "withEnd",                false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_TEST,         "withTest",               false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_విలువ,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_విలువ_W,      "withValueW",             false,  CLEAN,  "rw") \
    macro(COVERGROUP_FINALIZE_BINS,           "finalizeBins",           false,  CLEAN,  "") \
    macro(COVERGROUP_INCREMENT_BIN,           "incrementBin",           false,  CLEAN,  "r") \
    macro(COVERGROUP_INIT,                    "init",                   false,  CLEAN,  "r+") \
    macro(COVERGROUP_INST_P,                  "p",                      శుద్ధ_2,   CLEAN,  "") \
    macro(COVERGROUP_LEND_బరువు,             "lendWeight",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_రికార్డు_తగిలింది,              "recordHit",              false,  CLEAN,  "r") \
    macro(COVERGROUP_రిజిస్టరు_BINS,           "registerBins",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_SAMPLE,                  "sample",                 false,  CLEAN,  "") \
    macro(COVERGROUP_SAMPLE_IFFS,             "sample",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_SELECT_ALL,              "selectAll",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_AND,              "selectAnd",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_BIN,              "selectBin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM,              "selectDim",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM_చివర,          "selectDimEnd",           false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_OR,               "selectOr",               false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_RANGE,            "selectRange",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SELECT_RANGE_W,          "selectRangeW",           false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_చివర,               "sizedEnd",               శుద్ధ_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_ముగింపు,            "sizedFinish",            false,  CLEAN,  "r+") \
    macro(COVERGROUP_SIZED_మొదటి,             "sizedFirst",             శుద్ధ_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_RANGE,             "sizedRange",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_RANGE_W,           "sizedRangeW",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_SAMPLE,            "sizedSample",            false,  CLEAN,  "rrr") \
    macro(COVERGROUP_SIZED_SAMPLE_W,          "sizedSampleW",           false,  CLEAN,  "rrr") \
    macro(COVERGROUP_TYPE_COVERAGE,           "typeCoverage",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_TYPE_బరువు,             "typeWeight",             false,  CLEAN,  "r") \
    macro(COVERGROUP_విలువ_EXCLUDED,          "valueExcluded",          శుద్ధ_2,   CLEAN,  "r") \
    macro(COVERGROUP_విలువ_EXCLUDED_W,        "valueExcludedW",         శుద్ధ_2,   CLEAN,  "r") \
    macro(COVERGROUP_విలువ_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_విలువ_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_విలువ_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_విలువ_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_విలువ_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_విలువ_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_విలువ_TYPE,              "valueType",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_బరువు,                  "weight",                 false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_BEGIN,              "withBegin",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_ముగింపు,             "withFinish",             false,  CLEAN,  "r+") \
    macro(COVERGROUP_WITH_HI,                 "withHi",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_HI_W,               "withHiW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_LO,                 "withLo",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_LO_W,               "withLoW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_తదుపరి,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_నిరంతరం,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_నిరంతరం_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_వెనుక,           "atWriteAppendBack",      false,  CLEAN,  "r") \
    macro(DYN_CLEAR,                          "clear",                  false,  CLEAN,  "") \
    macro(DYN_ERASE,                          "erase",                  false,  CLEAN,  "r") \
    macro(DYN_చొప్పించు,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_తీయి,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_తీయి_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_నెట్టు,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_నెట్టు_FRONT,                     "push_front",             false,  CLEAN,  "r") \
    macro(DYN_RENEW,                          "renew",                  false,  CLEAN,  "r") \
    macro(DYN_RENEW_COPY,                     "renew_copy",             false,  CLEAN,  "rr") \
    macro(DYN_RESIZE,                         "resize",                 false,  CLEAN,  "TODO") \
    macro(DYN_కొలత,                           "size",                   శుద్ధ_2,   CLEAN,  "") \
    macro(DYN_ముక్క,                          "slice",                  శుద్ధ_2,   CLEAN,  "rr") \
    macro(DYN_ముక్క_కేటాయించు,                   "sliceAssign",            false,  CLEAN,  "rrr") \
    macro(DYN_ముక్క_కేటాయించు_వెనుక_వెనుక,         "sliceAssignBackBack",    false,  CLEAN,  "rrr") \
    macro(DYN_ముక్క_కేటాయించు_FRONT_వెనుక,        "sliceAssignFrontBack",   false,  CLEAN,  "rrr") \
    macro(DYN_ముక్క_వెనుక_వెనుక,                "sliceBackBack",          శుద్ధ_2,   CLEAN,  "rr") \
    macro(DYN_ముక్క_FRONT_వెనుక,               "sliceFrontBack",         శుద్ధ_2,   CLEAN,  "rr") \
    macro(EVENT_CLEAR_FIRED,                  "clearFired",             false,  CLEAN,  "") \
    macro(EVENT_CLEAR_TRIGGERED,              "clearTriggered",         false,  CLEAN,  "") \
    macro(EVENT_FIRE,                         "fire",                   false,  CLEAN,  "") \
    macro(EVENT_IS_FIRED,                     "isFired",                శుద్ధ_2,   CLEAN,  "") \
    macro(EVENT_IS_TRIGGERED,                 "isTriggered",            శుద్ధ_2,   CLEAN,  "") \
    macro(FORCE_కూడు,                          "addForce",               false,  CLEAN,  "rrrr+") \
    macro(FORCE_READ,                         "read",                   శుద్ధ_2,   CLEAN,  "r") \
    macro(FORCE_READ_సూచీ,                   "readIndex",              శుద్ధ_2,   CLEAN,  "rr") \
    macro(FORCE_READ_SEL,                     "readSel",                శుద్ధ_2,   CLEAN,  "TODO") \
    macro(FORCE_RELEASE,                      "release",                false,  CLEAN,  "rr+") \
    macro(FORCE_TOUCH,                        "touch",                  false,  CLEAN,  "") \
    macro(FORK_పూర్తి,                          "done",                   false,  CLEAN,  "rr") \
    macro(FORK_INIT,                          "init",                   false,  CLEAN,  "rr") \
    macro(FORK_JOIN,                          "join",                   false,  CLEAN,  "rrr") \
    macro(FORK_ON_KILL,                       "onKill",                 false,  CLEAN,  "r") \
    macro(NBA_COMMIT,                         "commit",                 false,  CLEAN,  "w") \
    macro(NBA_వరుసలో_చేర్చు,                        "enqueue",                false,  CLEAN,  "r+") \
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
    macro(RANDOMIZER_సమితి_VAR_DISABLED,        "set_var_disabled",       false,  CLEAN,  "r") \
    macro(RANDOMIZER_CLEAR_VAR_DISABLED,      "clear_var_disabled",     false,  CLEAN,  "r") \
    macro(RANDOMIZER_MARK_VAR_STATIC,         "mark_var_static",        false,  CLEAN,  "r") \
    macro(RANDOMIZER_సమితి_STATIC_RANDMODE,     "set_static_randmode",    false,  CLEAN,  "r") \
    macro(RNG_GET_RANDSTATE,                  "__Vm_rng.get_randstate", శుద్ధ_2,   CLEAN,  "") \
    macro(RNG_సమితి_RANDSTATE,                  "__Vm_rng.set_randstate", false,  CLEAN,  "r") \
    macro(SCHED_ANY_TRIGGERED,                "anyTriggered",           false,  CLEAN,  "r") \
    macro(SCHED_AWAITING_CURRENT_సమయం,        "awaitingCurrentTime",    శుద్ధ_2,   CLEAN,  "") \
    macro(SCHED_AWAITING_ZERO_DELAY,          "awaitingZeroDelay",      శుద్ధ_2,   CLEAN,  "") \
    macro(SCHED_READY,                        "ready",                  false,  CLEAN,  "r") \
    macro(SCHED_MOVE_గమ్యం_RESUME_వరుస,         "moveToResumeQueue",      false,  CLEAN,  "r") \
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
    macro(UNPACKED_కేటాయించు,                    "assign",                 false,  CLEAN,  "r") \
    macro(UNPACKED_FILL,                      "fill",                   false,  CLEAN,  "r") \
    macro(UNPACKED_NEQ,                       "neq",                    శుద్ధ_2,   CLEAN,  "r")
// clang-format on

class VCMethod final {
    స్థిర constexpr bool శుద్ధ_2 = true;  // For macro expansion of 'pure' field only
    స్థిర constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

బహిరంగ:
    గణన en : uint8_t {
#నిర్వచించు VL_CMETHOD_ID(id, పద్ధతి, శుద్ధ_3, clean, వాదనలు) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#నిర్వచనం_రద్దు VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    గణన en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి VL_శుద్ధ {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి విలువలు[] = {
#నిర్వచించు VL_CMETHOD_NAME(id, పద్ధతి, శుద్ధ_3, clean, వాదనలు) పద్ధతి,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#నిర్వచనం_రద్దు VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        తిరిగి విలువలు[m_e];
    }
    bool is_శుద్ధ() స్థిరరాశి VL_శుద్ధ {
        స్థిర constexpr bool విలువలు[] = {
#నిర్వచించు VL_CMETHOD_శుద్ధ(id, పద్ధతి, శుద్ధ_3, clean, వాదనలు) శుద్ధ_3,
            FOR_EACH_CMETHOD(VL_CMETHOD_శుద్ధ)
#నిర్వచనం_రద్దు VL_CMETHOD_శుద్ధ
                false  //
        };
        తిరిగి విలువలు[m_e];
    }
    bool isClean() స్థిరరాశి VL_శుద్ధ {
        స్థిర constexpr bool విలువలు[] = {
#నిర్వచించు VL_CMETHOD_CLEAN(id, పద్ధతి, శుద్ధ_3, clean, వాదనలు) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#నిర్వచనం_రద్దు VL_CMETHOD_CLEAN
                false  //
        };
        తిరిగి విలువలు[m_e];
    }
    స్థిరరాశి అక్షరం* వాదనలు() స్థిరరాశి VL_శుద్ధ {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి విలువలు[] = {
#నిర్వచించు VL_CMETHOD_వాదనలు(id, పద్ధతి, శుద్ధ_3, clean, వాదనలు) వాదనలు,
            FOR_EACH_CMETHOD(VL_CMETHOD_వాదనలు)
#నిర్వచనం_రద్దు VL_CMETHOD_వాదనలు
                ""  //
        };
        తిరిగి విలువలు[m_e];
    }
    // Return array method for given name
    స్థిర VCMethod array_పద్ధతి(స్థిరరాశి string& name);
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VCMethod& lhs, స్థిరరాశి VCMethod& rhs) { తిరిగి lhs.m_e == rhs.m_e; }
constexpr bool క్రియాకారకం==(స్థిరరాశి VCMethod& lhs, VCMethod::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VCMethod::en lhs, స్థిరరాశి VCMethod& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VCMethod& rhs) {
    తిరిగి os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#నిర్వచించు VL_CMETHOD_వాదనలు_CHECK(id, పద్ధతి, శుద్ధ_3, clean, వాదనలు) \
    static_assert(validate_వాదనలు_Descriptor(వాదనలు), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_వాదనలు_CHECK)
#నిర్వచనం_రద్దు VL_CMETHOD_వాదనలు_CHECK

#నిర్వచనం_రద్దు FOR_EACH_CMETHOD

// clang-format off
#నిర్వచించు FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_గమ్యం_SOLVER_HEX,           "vlToSolverHex",          శుద్ధ_2,   "r")
// clang-format on

class VCFunction final {
    స్థిర constexpr bool శుద్ధ_2 = true;  // For macro expansion of 'pure' field only

బహిరంగ:
    గణన en : uint8_t {
#నిర్వచించు VL_CFUNCTION_ID(id, పద్ధతి, శుద్ధ_3, వాదనలు) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#నిర్వచనం_రద్దు VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    గణన en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి VL_శుద్ధ {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి విలువలు[] = {
#నిర్వచించు VL_CFUNCTION_NAME(id, function, శుద్ధ_3, వాదనలు) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#నిర్వచనం_రద్దు VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        తిరిగి విలువలు[m_e];
    }
    bool is_శుద్ధ() స్థిరరాశి VL_శుద్ధ {
        స్థిర స్థిరరాశి bool విలువలు[] = {
#నిర్వచించు VL_CFUNCTION_శుద్ధ(id, పద్ధతి, శుద్ధ_3, వాదనలు) శుద్ధ_3,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_శుద్ధ)
#నిర్వచనం_రద్దు VL_CFUNCTION_శుద్ధ
                false  //
        };
        తిరిగి విలువలు[m_e];
    }
    స్థిరరాశి అక్షరం* వాదనలు() స్థిరరాశి VL_శుద్ధ {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి విలువలు[] = {
#నిర్వచించు VL_CFUNCTION_వాదనలు(id, పద్ధతి, శుద్ధ_3, వాదనలు) వాదనలు,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_వాదనలు)
#నిర్వచనం_రద్దు VL_CFUNCTION_వాదనలు
                ""  //
        };
        తిరిగి విలువలు[m_e];
    }
};

// Static assert all argument descriptors are well formed
#నిర్వచించు VL_CFUNCTION_వాదనలు_CHECK(id, పద్ధతి, శుద్ధ_3, వాదనలు) \
    static_assert(validate_వాదనలు_Descriptor(వాదనలు), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_వాదనలు_CHECK)
#నిర్వచనం_రద్దు VL_CFUNCTION_వాదనలు_CHECK

#నిర్వచనం_రద్దు FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
బహిరంగ:
    గణన en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

అంతరంగ:
    నిర్మాణం అంశం final {
        గణన en m_e;  // Statement's enum mnemonic, for checking
        స్థిరరాశి అక్షరం* m_name;  // Statements name, for debugging
    };
    స్థిర అంశం s_అంశం_దత్తాంశం[];

బహిరంగ:
    గణన en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి VL_శుద్ధ {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"none", "ctor_var_reset_call"};
        తిరిగి పేర్లు[m_e];
    }
    bool isNone() స్థిరరాశి { తిరిగి m_e == NONE; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VCStmtType& lhs, స్థిరరాశి VCStmtType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VCStmtType& lhs, VCStmtType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VCStmtType::en lhs, స్థిరరాశి VCStmtType& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VCStmtType& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
బహిరంగ:
    గణన en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    గణన en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి VL_శుద్ధ {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VCaseType& lhs, స్థిరరాశి VCaseType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VCaseType& lhs, VCaseType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VCaseType::en lhs, స్థిరరాశి VCaseType& rhs) { తిరిగి lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
బహిరంగ:
    గణన en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        తిరిగి పేర్లు[m_e];
    }
    bool isAssignable() స్థిరరాశి { తిరిగి m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VCastable& lhs, స్థిరరాశి VCastable& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VCastable& lhs, VCastable::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VCastable::en lhs, స్థిరరాశి VCastable& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VCastable& rhs) {
    తిరిగి os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
బహిరంగ:
    గణన en : uint8_t {
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
    గణన en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }  // LCOV_EXCL_LINE
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        తిరిగి పేర్లు[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    స్థిరరాశి అక్షరం* verilogKwd() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        తిరిగి పేర్లు[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    స్థిరరాశి అక్షరం* bin_సమితి_Enum() స్థిరరాశి {
        ఎంపిక (m_e) {
        స్థితి BINS_IGNORE: తిరిగి "VlCovBinKind::KIND_IGNORE";
        స్థితి BINS_ILLEGAL: తిరిగి "VlCovBinKind::KIND_ILLEGAL";
        స్థితి BINS_DEFAULT: తిరిగి "VlCovBinKind::KIND_DEFAULT";
        అప్రమేయ: తిరిగి "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool bin_Is_సాధారణ() స్థిరరాశి {
        తిరిగి m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    తిరిగి lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
బహిరంగ:
    గణన en : uint8_t {
        // Shared by option.* and type_option.*
        బరువు,
        GOAL,
        AT_కనిష్ఠ,
        AUTO_BIN_MAX,
        చొప్పున_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_అంకె_PRINT_MISSING,
        CROSS_RETAIN_AUTO_BINS,
        DETECT_OVERLAP,
        GET_INST_COVERAGE,
        // type_option.* only (IEEE 1800-2023 Table 19-3)
        STROBE,
        విలీనం_INSTANCES,
        DISTRIBUTE_మొదటి,
        REAL_INTERVAL,
        // Legacy option.* accepted for compatibility
        CROSS_AUTO_BIN_MAX,
        // sentinel - should never appear after parse-time validation
        UNKNOWN
    };
    గణన en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"weight",
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
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    తిరిగి lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
బహిరంగ:
    గణన en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    గణన en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(పూర్ణాంకం _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }  // LCOV_EXCL_LINE
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"", "[*]", "[->]", "[=]"};
        తిరిగి పేర్లు[m_e];
    }
    స్థిరరాశి అక్షరం* asciiJson() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        తిరిగి పేర్లు[m_e];
    }
};

//######################################################################

class VDirection final {
బహిరంగ:
    గణన en : uint8_t { NONE, ఆదానం, OUTPUT, INOUT, REF, CONSTREF };
    గణన en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        తిరిగి పేర్లు[m_e];
    }
    string verilogKwd() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        తిరిగి పేర్లు[m_e];
    }
    string prettyName() స్థిరరాశి { తిరిగి verilogKwd(); }
    bool isAny() స్థిరరాశి { తిరిగి m_e != NONE; }
    bool isInout() స్థిరరాశి { తిరిగి m_e == INOUT; }
    bool isInoutOrRef() స్థిరరాశి { తిరిగి m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_ఆదానం() స్థిరరాశి { తిరిగి m_e == ఆదానం; }
    bool isNonOutput() స్థిరరాశి {
        తిరిగి m_e == ఆదానం || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() స్థిరరాశి { తిరిగి m_e == OUTPUT; }
    bool isReadOnly() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == ఆదానం || m_e == CONSTREF; }
    bool isWritable() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == REF; }
    bool isConstRef() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == CONSTREF; }
    string traceSigDirection() స్థిరరాశి {
        ఒకవేళ (isInout()) {
            తిరిగి "VerilatedTraceSigDirection::INOUT";
        } లేకపోతే ఒకవేళ (isWritable()) {
            తిరిగి "VerilatedTraceSigDirection::OUTPUT";
        } లేకపోతే ఒకవేళ (isNonOutput()) {
            తిరిగి "VerilatedTraceSigDirection::INPUT";
        }
        తిరిగి "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() స్థిరరాశి {
        ఎంపిక (m_e) {
        స్థితి NONE: తిరిగి VAccess::NOCHANGE;
        స్థితి ఆదానం:
        స్థితి CONSTREF: తిరిగి VAccess::READ;
        స్థితి OUTPUT: తిరిగి VAccess::WRITE;
        స్థితి INOUT:
        స్థితి REF: తిరిగి VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VDirection& lhs, స్థిరరాశి VDirection& rhs) VL_MT_సురక్షిత {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VDirection& lhs, VDirection::en rhs) VL_MT_సురక్షిత {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VDirection::en lhs, స్థిరరాశి VDirection& rhs) VL_MT_సురక్షిత {
    తిరిగి lhs == rhs.m_e;
}
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VDirection& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
బహిరంగ:
    గణన en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    గణన en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    bool కూడు_Newline() స్థిరరాశి { తిరిగి m_e != DT_WRITE; }
    bool needScopeTracking() స్థిరరాశి { తిరిగి m_e != DT_DISPLAY && m_e != DT_WRITE; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VDisplayType& lhs, స్థిరరాశి VDisplayType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VDisplayType& lhs, VDisplayType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VDisplayType::en lhs, స్థిరరాశి VDisplayType& rhs) { తిరిగి lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
బహిరంగ:
    గణన en : uint8_t { FILE, VARS, ALL, FLUSH, హద్దు_2, OFF, ON };
    గణన en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VDumpCtlType& lhs, స్థిరరాశి VDumpCtlType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VDumpCtlType& lhs, VDumpCtlType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VDumpCtlType::en lhs, స్థిరరాశి VDumpCtlType& rhs) { తిరిగి lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
బహిరంగ:
    // REMEMBER to edit the strings below too
    గణన en : uint8_t {
        // These must be in general -> most specific order, as we sort by it
        // in V3Const::visit AstSenTree
        // Involving a variable
        ET_మారింది,  // Value changed
        ET_BOTHEDGE,  // POSEDGE | NEGEDGE (i.e.: 'edge' in Verilog)
        ET_POSEDGE,
        ET_NEGEDGE,
        ET_EVENT,  // VlEventBase::isFired
        // Involving an expression
        ET_TRUE,
        ET_INITIAL_NBA,  // Event that is fired initially and never again
        //
        ET_COMBO,  // Sensitive to all combo inputs to this block
        ET_COMBO_నక్షత్రం,  // Sensitive to all combo inputs to this block (from .*)
        ET_HYBRID,  // This is like ET_COMB, but with explicit sensitivity to an expression
        ET_STATIC,  // static variable initializers (runs before 'initial')
        ET_INITIAL,  // 'initial' statements
        ET_FINAL,  // 'final' statements
        ET_NEVER  // Never occurs (optimized away)
    };
    గణన en m_e;
    bool clockedStmt() స్థిరరాశి {
        స్థిర constexpr bool clocked[] = {
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
        తిరిగి clocked[m_e];
    }
    bool an_అంచు() స్థిరరాశి { తిరిగి m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() స్థిరరాశి {
        ఎంపిక (m_e) {
        స్థితి ET_మారింది: తిరిగి ET_మారింది;
        స్థితి ET_BOTHEDGE: తిరిగి ET_BOTHEDGE;
        స్థితి ET_POSEDGE: తిరిగి ET_NEGEDGE;
        స్థితి ET_NEGEDGE: తిరిగి ET_POSEDGE;
        అప్రమేయ: UASSERT_STATIC(0, "Inverting bad edgeType()"); తిరిగి ET_NEGEDGE;
        }
    }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        తిరిగి పేర్లు[m_e];
    }
    స్థిరరాశి అక్షరం* verilogKwd() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        తిరిగి పేర్లు[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusive_అంచు(స్థిరరాశి VEdgeType& other) స్థిరరాశి {
        ఎంపిక (m_e) {
        స్థితి VEdgeType::ET_POSEDGE:
            ఒకవేళ (other.m_e == VEdgeType::ET_NEGEDGE) తిరిగి true;
            ఆపు;
        స్థితి VEdgeType::ET_NEGEDGE:
            ఒకవేళ (other.m_e == VEdgeType::ET_POSEDGE) తిరిగి true;
            ఆపు;
        అప్రమేయ: ఆపు;
        }
        తిరిగి false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VEdgeType& lhs, స్థిరరాశి VEdgeType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VEdgeType& lhs, VEdgeType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VEdgeType::en lhs, స్థిరరాశి VEdgeType& rhs) { తిరిగి lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#నిర్వచించు FOR_EACH_EVAL(macro) \
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
బహిరంగ:
    గణన en : uint8_t {
#నిర్వచించు VL_EVAL_ID(id, iterated, triggers, మొదటి, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#నిర్వచనం_రద్దు VL_EVAL_ID
            ENUM_చివర
    };
    గణన en m_e;

    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి విలువలు[] = {
#నిర్వచించు VL_EVAL_NAME(id, iterated, triggers, మొదటి, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#నిర్వచనం_రద్దు VL_EVAL_NAME
                "_ENUM_END"  //
        };
        తిరిగి విలువలు[m_e];
    }
    bool isIterated() స్థిరరాశి {
        స్థిర constexpr bool విలువలు[] = {
#నిర్వచించు VL_EVAL_IS_ITERATED(id, iterated, triggers, మొదటి, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#నిర్వచనం_రద్దు VL_EVAL_IS_ITERATED
                false  //
        };
        తిరిగి విలువలు[m_e];
    }
    bool hasTriggers() స్థిరరాశి {
        స్థిర constexpr bool విలువలు[] = {
#నిర్వచించు VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, మొదటి, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#నిర్వచనం_రద్దు VL_EVAL_HAS_TRIGGERS
                false  //
        };
        తిరిగి విలువలు[m_e];
    }
    bool మొదటి_Iteration() స్థిరరాశి {
        స్థిర constexpr bool విలువలు[] = {
#నిర్వచించు VL_EVAL_మొదటి(id, iterated, triggers, మొదటి, slow) మొదటి,
            FOR_EACH_EVAL(VL_EVAL_మొదటి)
#నిర్వచనం_రద్దు VL_EVAL_మొదటి
                false  //
        };
        తిరిగి విలువలు[m_e];
    }
    bool slow() స్థిరరాశి {
        స్థిర constexpr bool విలువలు[] = {
#నిర్వచించు VL_EVAL_SLOW(id, iterated, triggers, మొదటి, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#నిర్వచనం_రద్దు VL_EVAL_SLOW
                false  //
        };
        తిరిగి విలువలు[m_e];
    }

    // Short name
    std::string ట్యాగ్() స్థిరరాశి { తిరిగి VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() స్థిరరాశి { తిరిగి "_eval_" + ట్యాగ్(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string eval_పద్ధతి() స్థిరరాశి { తిరిగి "eval" + capitalized_ట్యాగ్(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() స్థిరరాశి { తిరిగి "_eval_dump_triggers__" + ట్యాగ్(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dump_Triggers_పద్ధతి() స్థిరరాశి { తిరిగి "dumpTriggers" + capitalized_ట్యాగ్(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}
    క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }

అంతరంగ:
    std::string capitalized_ట్యాగ్() స్థిరరాశి {
        స్థిరరాశి std::string name = ascii();
        తిరిగి name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#నిర్వచనం_రద్దు FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
బహిరంగ:
    గణన en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        తిరిగి పేర్లు[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    // Is a node type compatible with the declaration
    bool is_కణుపు_Compatible(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి;
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VFwdType& lhs, స్థిరరాశి VFwdType& rhs) { తిరిగి lhs.m_e == rhs.m_e; }
constexpr bool క్రియాకారకం==(స్థిరరాశి VFwdType& lhs, VFwdType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VFwdType::en lhs, స్థిరరాశి VFwdType& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VFwdType& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    స్థిర uint64_t s_cachedCntGbl;  // Global computed count

బహిరంగ:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() స్థిరరాశి { తిరిగి m_cachedCnt == s_cachedCntGbl; }
    bool get() స్థిరరాశి { తిరిగి m_state; }
    శూన్య సమితి(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    శూన్య clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    స్థిర శూన్య clear_Cache_వృక్షం() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
బహిరంగ:
    గణన en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    గణన en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        తిరిగి పేర్లు[m_e];
    }
    స్థిరరాశి అక్షరం* verilogKwd() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"join", "join_any", "join_none"};
        తిరిగి పేర్లు[m_e];
    }
    bool join() స్థిరరాశి { తిరిగి m_e == JOIN; }
    bool joinAny() స్థిరరాశి { తిరిగి m_e == JOIN_ANY; }
    bool joinNone() స్థిరరాశి { తిరిగి m_e == JOIN_NONE; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VJoinType& lhs, స్థిరరాశి VJoinType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VJoinType& lhs, VJoinType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VJoinType::en lhs, స్థిరరాశి VJoinType& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VJoinType& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
బహిరంగ:
    గణన en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        తిరిగి పేర్లు[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    bool isNone() స్థిరరాశి { తిరిగి m_e == NONE; }
    bool isAutomatic() స్థిరరాశి { తిరిగి m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() స్థిరరాశి { తిరిగి m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() స్థిరరాశి { తిరిగి m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() స్థిరరాశి {
        ఎంపిక (m_e) {
        స్థితి AUTOMATIC_EXPLICIT: తిరిగి AUTOMATIC_IMPLICIT;
        స్థితి STATIC_EXPLICIT: తిరిగి STATIC_IMPLICIT;
        అప్రమేయ: తిరిగి m_e;
        }
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VLifetime& lhs, స్థిరరాశి VLifetime& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VLifetime& lhs, VLifetime::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VLifetime::en lhs, స్థిరరాశి VLifetime& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VLifetime& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
బహిరంగ:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            పూర్ణాంకం m_సూచీ;  // Current index
            స్థిరరాశి పూర్ణాంకం m_inc;  // Step, +1 or -1
        బహిరంగ:
            Iterator(పూర్ణాంకం సూచీ, పూర్ణాంకం inc)
                : m_సూచీ{సూచీ}
                , m_inc{inc} {}
            పూర్ణాంకం క్రియాకారకం*() స్థిరరాశి { తిరిగి m_సూచీ; }
            Iterator& క్రియాకారకం++() {
                m_సూచీ += m_inc;
                తిరిగి *this;
            }
            bool క్రియాకారకం!=(స్థిరరాశి Iterator& that) స్థిరరాశి { తిరిగి m_సూచీ != that.m_సూచీ; }
        };
        స్థిరరాశి పూర్ణాంకం m_begin;  // First index
        స్థిరరాశి పూర్ణాంకం m_చివర;  // One past the last index
        స్థిరరాశి పూర్ణాంకం m_inc;  // Step, +1 or -1
    బహిరంగ:
        Sequence(పూర్ణాంకం మొదటి, పూర్ణాంకం చివరి, పూర్ణాంకం inc)
            : m_begin{మొదటి}
            , m_చివర{చివరి + inc}
            , m_inc{inc} {}
        Iterator begin() స్థిరరాశి { తిరిగి Iterator{m_begin, m_inc}; }
        Iterator చివర() స్థిరరాశి { తిరిగి Iterator{m_చివర, m_inc}; }
    };

    పూర్ణాంకం m_ఎడమ = 0;  // Left side of range (pre-':')
    పూర్ణాంకం m_కుడి = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool క్రియాకారకం==(స్థిరరాశి VNumRange& rhs) స్థిరరాశి {
        తిరిగి m_ఎడమ == rhs.m_ఎడమ && m_కుడి == rhs.m_కుడి && m_ranged == rhs.m_ranged;
    }
    bool క్రియాకారకం<(స్థిరరాశి VNumRange& rhs) స్థిరరాశి {
        ఒకవేళ ((m_ఎడమ < rhs.m_ఎడమ)) తిరిగి true;
        ఒకవేళ (!(m_ఎడమ == rhs.m_ఎడమ)) తిరిగి false;  // lhs > rhs
        ఒకవేళ ((m_కుడి < rhs.m_కుడి)) తిరిగి true;
        ఒకవేళ (!(m_కుడి == rhs.m_కుడి)) తిరిగి false;  // lhs > rhs
        ఒకవేళ ((m_ranged < rhs.m_ranged)) తిరిగి true;
        ఒకవేళ (!(m_ranged == rhs.m_ranged)) తిరిగి false;  // lhs > rhs
        తిరిగి false;
    }
    //
    VNumRange() = అప్రమేయ;
    VNumRange(పూర్ణాంకం hi, పూర్ణాంకం lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(పూర్ణాంకం ఎడమ_2, పూర్ణాంకం కుడి_2)
        : m_ఎడమ{ఎడమ_2}
        , m_కుడి{కుడి_2}
        , m_ranged{true} {}
    ~VNumRange() = అప్రమేయ;
    // MEMBERS
    శూన్య init(పూర్ణాంకం hi, పూర్ణాంకం lo, bool ascending) {
        ఒకవేళ (lo > hi) std::మార్పిడి(hi, lo);
        m_ఎడమ = ascending ? lo : hi;
        m_కుడి = ascending ? hi : lo;
        m_ranged = true;
    }
    పూర్ణాంకం ఎడమ_2() స్థిరరాశి { తిరిగి m_ఎడమ; }
    పూర్ణాంకం కుడి_2() స్థిరరాశి { తిరిగి m_కుడి; }
    పూర్ణాంకం hi() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_ఎడమ > m_కుడి ? m_ఎడమ : m_కుడి;
    }  // How to show a declaration
    పూర్ణాంకం lo() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_ఎడమ > m_కుడి ? m_కుడి : m_ఎడమ;
    }  // How to show a declaration
    పూర్ణాంకం ఎడమ_గమ్యం_కుడి_Inc() స్థిరరాశి { తిరిగి ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_ఎడమ_గమ్యం_కుడి() స్థిరరాశి { తిరిగి {m_ఎడమ, m_కుడి, ఎడమ_గమ్యం_కుడి_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_కుడి_గమ్యం_ఎడమ() స్థిరరాశి { తిరిగి {m_కుడి, m_ఎడమ, -ఎడమ_గమ్యం_కుడి_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seq_Lo_గమ్యం_Hi() స్థిరరాశి { తిరిగి {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seq_Hi_గమ్యం_Lo() స్థిరరాశి { తిరిగి {hi(), lo(), -1}; }
    పూర్ణాంకం elements() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి hi() - lo() + 1; }
    bool ranged() స్థిరరాశి { తిరిగి m_ranged; }
    bool ascending() స్థిరరాశి { తిరిగి m_ఎడమ < m_కుడి; }
    పూర్ణాంకం hiMaxSelect() స్థిరరాశి {
        తిరిగి (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() స్థిరరాశి {
        ఒకవేళ (ranged()) {
            తిరిగి "["s + std::గమ్యం_string(ఎడమ_2()) + ":" + std::గమ్యం_string(కుడి_2()) + "]";
        } లేకపోతే {
            తిరిగి "[norg]";
        }
    }
    శూన్య dump(std::ostream& str) స్థిరరాశి { str << ascii(); }
};
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VNumRange& rhs) {
    rhs.dump(os);
    తిరిగి os;
}

// ######################################################################

class VPragmaType final {
బహిరంగ:
    గణన en : uint8_t {
        COVERAGE_ఖండం_OFF,
        HIER_ఖండం,
        HIER_PARAMS,
        INLINE_MODULE,
        NO_INLINE_MODULE,
        NO_INLINE_TASK,
        ప్రజా_MODULE,
        ప్రజా_TASK,
        TIMEUNIT_సమితి,
        UNROLL_DISABLE,
        UNROLL_నిండిన,
        నిండిన_CASE,
        PARALLEL_CASE,
        VERILATOR_LIB,
        ENUM_కొలత
    };
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {
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
        తిరిగి పేర్లు[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VPragmaType& lhs, స్థిరరాశి VPragmaType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VPragmaType& lhs, VPragmaType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VPragmaType::en lhs, స్థిరరాశి VPragmaType& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VPragmaType& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
బహిరంగ:
    గణన en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        తిరిగి పేర్లు[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    bool isRandomizable() స్థిరరాశి { తిరిగి m_e != NONE; }
    bool isRand() స్థిరరాశి { తిరిగి m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() స్థిరరాశి { తిరిగి m_e == RAND_CYCLIC; }
};
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VRandAttr& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelf_సూచిక_పాఠ్యం final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    స్థిర స్థిరరాశి std::shared_ptr<స్థిరరాశి string> s_emptyp;  // Holds ""
    స్థిర స్థిరరాశి std::shared_ptr<స్థిరరాశి string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<స్థిరరాశి string> m_strp;

బహిరంగ:
    // CONSTRUCTORS
    class ఖాళీలు_2 {};  // for creator type-overload selection
    explicit VSelf_సూచిక_పాఠ్యం(ఖాళీలు_2)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelf_సూచిక_పాఠ్యం(This)
        : m_strp{s_thisp} {}
    VSelf_సూచిక_పాఠ్యం(This, స్థిరరాశి string& క్షేత్రం)
        : m_strp{std::make_shared<స్థిరరాశి string>("this->" + క్షేత్రం)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelf_సూచిక_పాఠ్యం(VlSyms, స్థిరరాశి string& క్షేత్రం)
        : m_strp{std::make_shared<స్థిరరాశి string>("(&vlSymsp->vlInst." + క్షేత్రం + ')')} {}

    // METHODS
    bool is_ఖాళీలు() స్థిరరాశి { తిరిగి m_strp == s_emptyp; }
    bool isVlSym() స్థిరరాశి { తిరిగి m_strp->కనుగొను("vlSymsp") != string::npos; }
    bool hasThis() స్థిరరాశి { తిరిగి m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) స్థిరరాశి;
    స్థిర string replaceThis(bool useSelfForThis, స్థిరరాశి string& పాఠ్యం);
    స్థిరరాశి std::string& asString() స్థిరరాశి { తిరిగి *m_strp; }
    bool క్రియాకారకం==(స్థిరరాశి VSelf_సూచిక_పాఠ్యం& other) స్థిరరాశి { తిరిగి *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
బహిరంగ:
    గణన en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        తిరిగి పేర్లు[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    స్థిర VSigning fromBool(bool isSigned) {  // Factory method
        తిరిగి isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    bool isSigned() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == SIGNED; }
    bool isNosign() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VSigning& lhs, స్థిరరాశి VSigning& rhs) { తిరిగి lhs.m_e == rhs.m_e; }
constexpr bool క్రియాకారకం==(స్థిరరాశి VSigning& lhs, VSigning::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VSigning::en lhs, స్థిరరాశి VSigning& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VSigning& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

class VStrength final {
బహిరంగ:
    గణన en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    గణన en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VStrength& lhs, స్థిరరాశి VStrength& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VStrength& lhs, VStrength::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VStrength::en lhs, స్థిరరాశి VStrength& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VStrength& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
బహిరంగ:
    గణన en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    గణన en m_e;
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        తిరిగి పేర్లు[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
};

//######################################################################

class VTrace_ఉపసర్గ_Type final {
బహిరంగ:
    గణన en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    గణన en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTrace_ఉపసర్గ_Type(en _e)
        : m_e{_e} {}
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VTrace_ఉపసర్గ_Type& lhs, స్థిరరాశి VTrace_ఉపసర్గ_Type& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VTrace_ఉపసర్గ_Type& lhs, VTrace_ఉపసర్గ_Type::en rhs) {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VTrace_ఉపసర్గ_Type::en lhs, స్థిరరాశి VTrace_ఉపసర్గ_Type& rhs) {
    తిరిగి lhs == rhs.m_e;
}
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VTrace_ఉపసర్గ_Type& rhs) {
    తిరిగి os << rhs.ascii();
}

//######################################################################

class VTraceType final {
బహిరంగ:
    గణన en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        నిండిన,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    గణన en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"CONSTANT", "FULL", "CHANGE"};
        తిరిగి పేర్లు[m_e];
    }
    స్థిరరాశి అక్షరం* func_ఉపసర్గ() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"trace_const", "trace_full", "trace_chg"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VTraceType& lhs, స్థిరరాశి VTraceType& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VTraceType& lhs, VTraceType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VTraceType::en lhs, స్థిరరాశి VTraceType& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VTraceType& rhs) {
    తిరిగి os << rhs.ascii();
}

//######################################################################

class VUseType final {
బహిరంగ:
    గణన en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    గణన en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    bool కలిగియుంది_Any(VUseType other) స్థిరరాశి { తిరిగి m_e & other.m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        తిరిగి పేర్లు[m_e - 1];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VUseType& lhs, స్థిరరాశి VUseType& rhs) { తిరిగి lhs.m_e == rhs.m_e; }
constexpr bool క్రియాకారకం==(స్థిరరాశి VUseType& lhs, VUseType::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VUseType::en lhs, స్థిరరాశి VUseType& rhs) { తిరిగి lhs == rhs.m_e; }
constexpr VUseType::en క్రియాకారకం|(VUseType::en lhs, VUseType::en rhs) {
    తిరిగి VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en క్రియాకారకం&(VUseType::en lhs, VUseType::en rhs) {
    తిరిగి VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VUseType& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

class VVarType final {
బహిరంగ:
    గణన en : uint8_t {
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
        పోర్టు,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    గణన en m_e;
    VVarType() VL_MT_సురక్షిత : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_సురక్షిత : m_e{_e} {}
    explicit VVarType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        తిరిగి పేర్లు[m_e];
    }
    bool isParam() స్థిరరాశి { తిరిగి m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() స్థిరరాశి {
        తిరిగి (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == పోర్టు || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() స్థిరరాశి {
        తిరిగి (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() స్థిరరాశి { తిరిగి (m_e == TRIOR); }
    bool isWiredNet() స్థిరరాశి { తిరిగి (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() స్థిరరాశి {  // In Verilog, always ok in SystemVerilog
        తిరిగి (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == పోర్టు || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() స్థిరరాశి {
        తిరిగి (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool is_తాత్కాలిక() స్థిరరాశి {
        తిరిగి (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() స్థిరరాశి {
        తిరిగి (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == పోర్టు
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    స్థిరరాశి అక్షరం* trace_Sig_రకం() స్థిరరాశి {
        // VerilatedTraceSigKind to used in trace signal declaration
        స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి lut[] = {
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
        తిరిగి lut[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VVarType& lhs, స్థిరరాశి VVarType& rhs) VL_MT_సురక్షిత {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VVarType& lhs, VVarType::en rhs) VL_MT_సురక్షిత {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VVarType::en lhs, స్థిరరాశి VVarType& rhs) VL_MT_సురక్షిత {
    తిరిగి lhs == rhs.m_e;
}
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VVarType& rhs) VL_MT_సురక్షిత {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_కీలకం final {
బహిరంగ:
    స్థిరరాశి పూర్ణాంకం m_వెడల్పు;  // From AstNodeDType: Bit width of operation
    స్థిరరాశి పూర్ణాంకం m_వెడల్పు_Min;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    స్థిరరాశి VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    స్థిరరాశి VSigning m_numeric;  // From AstNodeDType: Node is signed
    స్థిరరాశి VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool క్రియాకారకం==(స్థిరరాశి VBasic_Type_కీలకం& rhs) స్థిరరాశి {
        తిరిగి m_వెడల్పు == rhs.m_వెడల్పు && m_వెడల్పు_Min == rhs.m_వెడల్పు_Min && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool క్రియాకారకం<(స్థిరరాశి VBasic_Type_కీలకం& rhs) స్థిరరాశి {
        ఒకవేళ ((m_వెడల్పు < rhs.m_వెడల్పు)) తిరిగి true;
        ఒకవేళ (!(m_వెడల్పు == rhs.m_వెడల్పు)) తిరిగి false;  // lhs > rhs
        ఒకవేళ ((m_వెడల్పు_Min < rhs.m_వెడల్పు_Min)) తిరిగి true;
        ఒకవేళ (!(m_వెడల్పు_Min == rhs.m_వెడల్పు_Min)) తిరిగి false;  // lhs > rhs
        ఒకవేళ ((m_numeric < rhs.m_numeric)) తిరిగి true;
        ఒకవేళ (!(m_numeric == rhs.m_numeric)) తిరిగి false;  // lhs > rhs
        ఒకవేళ ((m_keyword < rhs.m_keyword)) తిరిగి true;
        ఒకవేళ (!(m_keyword == rhs.m_keyword)) తిరిగి false;  // lhs > rhs
        ఒకవేళ ((m_nrange < rhs.m_nrange)) తిరిగి true;
        ఒకవేళ (!(m_nrange == rhs.m_nrange)) తిరిగి false;  // lhs > rhs
        తిరిగి false;
    }
    VBasic_Type_కీలకం(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric, VBasicDTypeKwd kwd,
                  స్థిరరాశి VNumRange& nrange)
        : m_వెడల్పు{వెడల్పు}
        , m_వెడల్పు_Min{వెడల్పు_Min}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_కీలకం() = అప్రమేయ;
};

#అయితే_ముగింపు  // Guard
