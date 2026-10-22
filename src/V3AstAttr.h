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
#ଅନ୍ତର୍ଭାଵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

// Hint class so we can choose constructors
class VFlagBitPacked {};
class VFlagChildDType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbortKind final {
ସର୍ବସାଧାରଣ:
    // IEEE 1800-2023 16.12.14 property abort operators.
    ଗଣନା en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    ଗଣନା en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbortKind(en _e)
        : m_e{_e} {}
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        ଫେରନ୍ତୁ names[m_e];
    }
    bool isAccept() ଅଚଳ { ଫେରନ୍ତୁ m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"RD", "WR", "RW", "--"};
        ଫେରନ୍ତୁ names[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* arrow() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        ଫେରନ୍ତୁ names[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    VAccess invert() ଅଚଳ {
        ଫେରନ୍ତୁ (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ଅଚଳ { ଫେରନ୍ତୁ m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ଅଚଳ { ଫେରନ୍ତୁ m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ଅଚଳ { ଫେରନ୍ତୁ m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ଅଚଳ { ଫେରନ୍ତୁ m_e == WRITE || m_e == READWRITE; }
    bool isRW() ଅଚଳ { ଫେରନ୍ତୁ m_e == READWRITE; }
};
constexpr bool operator==(ଅଚଳ VAccess& lhs, ଅଚଳ VAccess& rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ଅଚଳ VAccess& lhs, VAccess::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VAccess::en lhs, ଅଚଳ VAccess& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VAccess& rhs) { ଫେରନ୍ତୁ os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ASSIGN };
    ଗଣନା en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VAlwaysKwd& lhs, ଅଚଳ VAlwaysKwd& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VAlwaysKwd::en lhs, ଅଚଳ VAlwaysKwd& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
ସର୍ବସାଧାରଣ:
    // IEEE 1800-2023 Table 20-5
    ଗଣନା en : uint8_t {
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
    ଗଣନା en m_e;
    VAssertCtlType()
        : m_e{_TO_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        // IEEE 1800-2023 20.11
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"",
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
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VAssertCtlType& lhs, ଅଚଳ VAssertCtlType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VAssertCtlType::en lhs, ଅଚଳ VAssertCtlType& rhs) {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
ସର୍ବସାଧାରଣ:
    // IEEE 1800-2023 Table 20-7
    ଗଣନା en : uint8_t {
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
    ଗଣନା en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ଅଚଳ {
        std::stringstream types;
        ଯଦି (m_e == INTERNAL)
            types << "INTERNAL ";
        ଅନ୍ୟଥା {
            ଯଦି (m_e & ASSERT) types << "ASSERT ";
            ଯଦି (m_e & COVER) types << "COVER ";
            ଯଦି (m_e & ASSUME) types << "ASSUME ";
            ଯଦି (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            ଯଦି (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            ଯଦି (m_e & INTRINSIC) types << "INTRINSIC ";
            ଯଦି (m_e & RESTRICT) types << "RESTRICT ";
        }
        ଅଚଳ string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        ଫେରନ୍ତୁ str.substr(0, str.size() - 1);
    }
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
};
constexpr bool operator==(ଅଚଳ VAssertDirectiveType& lhs, ଅଚଳ VAssertDirectiveType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VAssertDirectiveType::en lhs, ଅଚଳ VAssertDirectiveType& rhs) {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en operator|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    ଫେରନ୍ତୁ VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
ସର୍ବସାଧାରଣ:
    // IEEE 1800-2023 Table 20-6
    ଗଣନା en : uint8_t {
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
    ଗଣନା en m_e;
    VAssertType()
        : m_e{INTERNAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool containsAny(VAssertType other) ଅଚଳ { ଫେରନ୍ତୁ m_e & other.m_e; }
    string ascii() ଅଚଳ {
        std::stringstream types;
        ଯଦି (m_e == INTERNAL)
            types << "INTERNAL ";
        ଅନ୍ୟଥା {
            ଯଦି (m_e & CONCURRENT) types << "CONCURRENT ";
            ଯଦି (m_e & SIMPLE_IMMEDIATE) types << "SIMPLE_IMMEDIATE ";
            ଯଦି (m_e & OBSERVED_DEFERRED_IMMEDIATE) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            ଯଦି (m_e & FINAL_DEFERRED_IMMEDIATE) types << "FINAL_DEFERRED_IMMEDIATE ";
            ଯଦି (m_e & EXPECT) types << "EXPECT ";
            ଯଦି (m_e & UNIQUE) types << "UNIQUE ";
            ଯଦି (m_e & UNIQUE0) types << "UNIQUE0 ";
            ଯଦି (m_e & PRIORITY) types << "PRIORITY ";
        }
        ଅଚଳ string str = types.str();
        UASSERT(!str.empty(), "Assert should be of one of types");
        ଫେରନ୍ତୁ str.substr(0, str.size() - 1);
    }
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
};
constexpr bool operator==(ଅଚଳ VAssertType& lhs, ଅଚଳ VAssertType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VAssertType& lhs, VAssertType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VAssertType::en lhs, ଅଚଳ VAssertType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
constexpr VAssertType::en operator|(VAssertType::en lhs, VAssertType::en rhs) {
    ଫେରନ୍ତୁ VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    ଗଣନା en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"default", "weak", "strong"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VPropStrength& lhs, ଅଚଳ VPropStrength& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VPropStrength& lhs, VPropStrength::en rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator!=(ଅଚଳ VPropStrength& lhs, VPropStrength::en rhs) {
    ଫେରନ୍ତୁ lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
ସର୍ବସାଧାରଣ:
    // clang-format off
    ଗଣନା en: uint8_t  {
        ILLEGAL,
        //
        DIM_BITS,                       // V3Const converts to constant
        DIM_BITS_OR_ସଂଖ୍ୟା,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_ଉପର,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_ବାମ,                       // V3Width processes
        DIM_ତଳ,                        // V3Width processes
        DIM_ଡାହାଣ,                      // V3Width processes
        DIM_SIZE,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_PUBLIC,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_FIRST,                     // V3Width processes
        ENUM_LAST,                      // V3Width processes
        ENUM_ଅଙ୍କ,                       // V3Width processes
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
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        // clang-format off
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {
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
        ଫେରନ୍ତୁ names[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ଅଚଳ {
        ବାଛ (m_e) {
        ସ୍ଥିତି DIM_BITS:
        ସ୍ଥିତି DIM_BITS_OR_ସଂଖ୍ୟା:
        ସ୍ଥିତି DIM_DIMENSIONS:
        ସ୍ଥିତି DIM_ଉପର:
        ସ୍ଥିତି DIM_INCREMENT:
        ସ୍ଥିତି DIM_ବାମ:
        ସ୍ଥିତି DIM_ତଳ:
        ସ୍ଥିତି DIM_ଡାହାଣ:
        ସ୍ଥିତି DIM_SIZE:
        ସ୍ଥିତି DIM_UNPK_DIMENSIONS:
        ସ୍ଥିତି TYPEID:
        ସ୍ଥିତି TYPENAME: ଫେରନ୍ତୁ true;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
};
constexpr bool operator==(ଅଚଳ VAttrType& lhs, ଅଚଳ VAttrType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VAttrType& lhs, VAttrType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VAttrType::en lhs, ଅଚଳ VAttrType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }

// ######################################################################

class VBaseOverride final {
    bool m_extends : 1;
    bool m_final : 1;
    bool m_initial : 1;

ସର୍ବସାଧାରଣ:
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
    ଶୂନ୍ୟ combine(ଅଚଳ VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ଅଚଳ { ଫେରନ୍ତୁ m_extends | m_final | m_initial; }
    bool isExtends() ଅଚଳ { ଫେରନ୍ତୁ m_extends; }
    bool isFinal() ଅଚଳ { ଫେରନ୍ତୁ m_final; }
    bool isInitial() ଅଚଳ { ଫେରନ୍ତୁ m_initial; }
    string ascii() ଅଚଳ {
        string out;
        ଯଦି (m_initial) out = VString::dot(out, " ", "initial");
        ଯଦି (m_extends) out = VString::dot(out, " ", "extends");
        ଯଦି (m_final) out = VString::dot(out, " ", "final");
        ଫେରନ୍ତୁ out;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
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
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ VL_MT_SAFE {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"%E-unk",
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
        ଫେରନ୍ତୁ names[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* typeName() ଅଚଳ VL_MT_SAFE {
        ବାଛ (m_e) {
        ସ୍ଥିତି LOGIC2STATE: ଫେରନ୍ତୁ VBasicDTypeKwd{LOGIC}.ascii();
        ସ୍ଥିତି INTEGER2STATE: ଫେରନ୍ତୁ VBasicDTypeKwd{INTEGER}.ascii();
        ସ୍ଥିତି TIME2STATE: ଫେରନ୍ତୁ VBasicDTypeKwd{TIME}.ascii();
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ ascii();
        }
    }
    ଅଚଳ ଅକ୍ଷର* dpiType() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"%E-unk",
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
        ଫେରନ୍ତୁ names[m_e];
    }
    ସ୍ଥିର ଶୂନ୍ୟ selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VBasicDTypeKwd(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e) VL_MT_SAFE  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା width() ଅଚଳ {
        ବାଛ (m_e) {
        ସ୍ଥିତି BIT: ଫେରନ୍ତୁ 1;  // scalar, can't bit extract unless ranged
        ସ୍ଥିତି BYTE: ଫେରନ୍ତୁ 8;
        ସ୍ଥିତି CHANDLE: ଫେରନ୍ତୁ 64;
        ସ୍ଥିତି NULLTYPE: ଫେରନ୍ତୁ 1;  // Historical - bit was used instead of NULLTYPE
        ସ୍ଥିତି EVENT: ଫେରନ୍ତୁ 1;
        ସ୍ଥିତି INT: ଫେରନ୍ତୁ 32;
        ସ୍ଥିତି INTEGER: ଫେରନ୍ତୁ 32;
        ସ୍ଥିତି INTEGER2STATE: ଫେରନ୍ତୁ 32;
        ସ୍ଥିତି LOGIC: ଫେରନ୍ତୁ 1;  // scalar, can't bit extract unless ranged
        ସ୍ଥିତି LOGIC2STATE: ଫେରନ୍ତୁ 1;  // scalar, can't bit extract unless ranged
        ସ୍ଥିତି LONGINT: ଫେରନ୍ତୁ 64;
        ସ୍ଥିତି DOUBLE: ଫେରନ୍ତୁ 64;  // opaque
        ସ୍ଥିତି SHORTINT: ଫେରନ୍ତୁ 16;
        ସ୍ଥିତି TIME: ଫେରନ୍ତୁ 64;
        ସ୍ଥିତି TIME2STATE: ଫେରନ୍ତୁ 64;
        ସ୍ଥିତି STRING: ଫେରନ୍ତୁ 64;  // opaque  // Just the pointer, for today
        ସ୍ଥିତି SCOPEPTR: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି CHARPTR: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି MTASKSTATE: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି DELAY_SCHEDULER: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି TRIGGER_SCHEDULER: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି DYNAMIC_TRIGGER_SCHEDULER: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି FORK_SYNC: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି PROCESS_REFERENCE: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି RANDOM_GENERATOR: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି RANDOM_STDGENERATOR: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି COVERGROUP_INSTHANDLE: ଫେରନ୍ତୁ 0;  // opaque
        ସ୍ଥିତି UINT32: ଫେରନ୍ତୁ 32;
        ସ୍ଥିତି UINT64: ଫେରନ୍ତୁ 64;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ 0;
        }
    }
    bool isSigned() ଅଚଳ {
        ଫେରନ୍ତୁ m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ଅଚଳ {
        ଫେରନ୍ତୁ m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == BIT || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == TIME || m_e == TIME2STATE;
    }
    bool isFourstate() ଅଚଳ {
        ଫେରନ୍ତୁ m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == TIME;
    }
    bool isZeroInit() ଅଚଳ {  // Otherwise initializes to X
        ଫେରନ୍ତୁ (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ଅଚଳ {  // Enum increment supported
        ଫେରନ୍ତୁ (m_e == BIT || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == TIME || m_e == TIME2STATE);
    }
    bool isBit() ଅଚଳ { ଫେରନ୍ତୁ m_e == BIT; }
    bool isBitLogic() ଅଚଳ {  // Bit/logic vector types; can form a packed array
        ଫେରନ୍ତୁ (m_e == LOGIC || m_e == LOGIC2STATE || m_e == BIT);
    }
    bool isDpiUnsignable() ଅଚଳ {  // Can add "unsigned" to DPI
        ଫେରନ୍ତୁ (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ଅଚଳ {  // Uses standard C layout, for DPI runtime access
        ଫେରନ୍ତୁ (m_e == BIT || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ଅଚଳ VL_MT_SAFE {  // IE not a simple number we can bit optimize
        ଫେରନ୍ତୁ (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == CHANDLE; }
    bool isDouble() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == DOUBLE; }
    bool isEvent() ଅଚଳ { ଫେରନ୍ତୁ m_e == EVENT; }
    bool isString() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == STRING; }
    bool isMTaskState() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == MTASKSTATE; }
    bool isSameish(ଅଚଳ VBasicDTypeKwd& other) ଅଚଳ VL_MT_SAFE {
        ବାଛ (other.m_e) {
        ସ୍ଥିତି LOGIC:
        ସ୍ଥିତି LOGIC2STATE:
        ସ୍ଥିତି LOGIC_IMPLICIT: ଫେରନ୍ତୁ m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        ସ୍ଥିତି INTEGER:
        ସ୍ଥିତି INTEGER2STATE: ଫେରନ୍ତୁ m_e == INTEGER || m_e == INTEGER2STATE;
        ସ୍ଥିତି TIME:
        ସ୍ଥିତି TIME2STATE: ଫେରନ୍ତୁ m_e == TIME || m_e == TIME2STATE;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ଅଚଳ VL_MT_SAFE {
        ବାଛ (m_e) {
        ସ୍ଥିତି BIT:
        ସ୍ଥିତି BYTE:
        ସ୍ଥିତି CHANDLE:
        ସ୍ଥିତି INT:
        ସ୍ଥିତି INTEGER:
        ସ୍ଥିତି INTEGER2STATE:
        ସ୍ଥିତି LOGIC:
        ସ୍ଥିତି LOGIC2STATE:
        ସ୍ଥିତି LONGINT:
        ସ୍ଥିତି DOUBLE:
        ସ୍ଥିତି SHORTINT:
        ସ୍ଥିତି SCOPEPTR:
        ସ୍ଥିତି CHARPTR:
        ସ୍ଥିତି UINT32:
        ସ୍ଥିତି UINT64: ଫେରନ୍ତୁ true;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ false;
        }
    }

    ଅଚଳ ଅକ୍ଷର* traceSigType() ଅଚଳ {
        // VerilatedTraceSigType to used in trace signal declaration
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ lut[] = {
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
        static_assert(ଆକାର(lut) / ଆକାର(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        ଫେରନ୍ତୁ lut[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VBasicDTypeKwd& lhs, ଅଚଳ VBasicDTypeKwd& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VBasicDTypeKwd::en lhs, ଅଚଳ VBasicDTypeKwd& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, _ENUM_END };
    ଗଣନା en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"FALSE", "TRUE", "UNK"};
        ଫେରନ୍ତୁ names[m_e];
    }
    bool isKnown() ଅଚଳ { ଫେରନ୍ତୁ m_e != BU_UNKNOWN; }
    bool trueKnown() ଅଚଳ { ଫେରନ୍ତୁ m_e == BU_TRUE; }
    ଶୂନ୍ୟ setTrueOrFalse(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool operator==(ଅଚଳ VBoolOrUnknown& lhs, ଅଚଳ VBoolOrUnknown& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VBoolOrUnknown::en lhs, ଅଚଳ VBoolOrUnknown& rhs) {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
}
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VBoolOrUnknown& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, _ENUM_END };
    ଗଣନା en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    bool unknown() ଅଚଳ { ଫେରନ୍ତୁ m_e == BP_UNKNOWN; }
    bool likely() ଅଚଳ { ଫେରନ୍ତୁ m_e == BP_LIKELY; }
    bool unlikely() ଅଚଳ { ଫେରନ୍ତୁ m_e == BP_UNLIKELY; }
    VBranchPred invert() ଅଚଳ {
        ଯଦି (m_e == BP_UNLIKELY) ଫେରନ୍ତୁ BP_LIKELY;
        ଯଦି (m_e == BP_LIKELY) ଫେରନ୍ତୁ BP_UNLIKELY;
        ଫେରନ୍ତୁ m_e;
    }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        ଫେରନ୍ତୁ names[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* asciiShort() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"", "L", "!L"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VBranchPred& lhs, ଅଚଳ VBranchPred& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VBranchPred& lhs, VBranchPred::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VBranchPred::en lhs, ଅଚଳ VBranchPred& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VBranchPred& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ସ୍ଥିର constexpr bool validateArgsDescriptor(ଅଚଳ ଅକ୍ଷର* descrp) {
    // Is "TODO"
    ଯଦି (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        ଫେରନ୍ତୁ true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* cp = descrp; *cp; ++cp) {
        ଯଦି (*cp == '+') ଫେରନ୍ତୁ cp != descrp && !cp[1];
        ଯଦି (*cp != 'r' && *cp != 'w' && *cp != 'm') ଫେରନ୍ତୁ false;
    }
    ଫେରନ୍ତୁ true;
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
    macro(ARRAY_କାଢ଼_BACK,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_କାଢ଼_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ଠେଲ_BACK,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ଠେଲ_FRONT,                   "push_front",             false,  CLEAN,  "r") \
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
    macro(COVERGROUP_CROSS_WITH_ମୂଲ୍ୟ,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_ମୂଲ୍ୟ_W,      "withValueW",             false,  CLEAN,  "rw") \
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
    macro(COVERGROUP_ମୂଲ୍ୟ_EXCLUDED,          "valueExcluded",          PURE,   CLEAN,  "r") \
    macro(COVERGROUP_ମୂଲ୍ୟ_EXCLUDED_W,        "valueExcludedW",         PURE,   CLEAN,  "r") \
    macro(COVERGROUP_ମୂଲ୍ୟ_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_ମୂଲ୍ୟ_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_ମୂଲ୍ୟ_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_ମୂଲ୍ୟ_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_ମୂଲ୍ୟ_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ମୂଲ୍ୟ_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_ମୂଲ୍ୟ_TYPE,              "valueType",              false,  CLEAN,  "rr") \
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
    macro(DYN_ଭର୍ତ୍ତି,                         "insert",                 false,  CLEAN,  "rr") \
    macro(DYN_କାଢ଼,                            "pop",                    false,  CLEAN,  "rrm") \
    macro(DYN_କାଢ଼_FRONT,                      "pop_front",              false,  CLEAN,  "") \
    macro(DYN_ଠେଲ,                           "push",                   false,  CLEAN,  "rr") \
    macro(DYN_ଠେଲ_FRONT,                     "push_front",             false,  CLEAN,  "r") \
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
    ସ୍ଥିର constexpr bool PURE = true;  // For macro expansion of 'pure' field only
    ସ୍ଥିର constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
#define VL_CMETHOD_ID(id, method, pure, clean, args) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#undef VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    ଗଣନା en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ VL_PURE {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_CMETHOD_NAME(id, method, pure, clean, args) method,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#undef VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    bool isPure() ଅଚଳ VL_PURE {
        ସ୍ଥିର constexpr bool ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_CMETHOD_PURE(id, method, pure, clean, args) pure,
            FOR_EACH_CMETHOD(VL_CMETHOD_PURE)
#undef VL_CMETHOD_PURE
                false  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    bool isClean() ଅଚଳ VL_PURE {
        ସ୍ଥିର constexpr bool ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_CMETHOD_CLEAN(id, method, pure, clean, args) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#undef VL_CMETHOD_CLEAN
                false  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* args() ଅଚଳ VL_PURE {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_CMETHOD_ARGS(id, method, pure, clean, args) args,
            FOR_EACH_CMETHOD(VL_CMETHOD_ARGS)
#undef VL_CMETHOD_ARGS
                ""  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    // Return array method for given name
    ସ୍ଥିର VCMethod arrayMethod(ଅଚଳ string& name);
};
constexpr bool operator==(ଅଚଳ VCMethod& lhs, ଅଚଳ VCMethod& rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ଅଚଳ VCMethod& lhs, VCMethod::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VCMethod::en lhs, ଅଚଳ VCMethod& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VCMethod& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
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
    ସ୍ଥିର constexpr bool PURE = true;  // For macro expansion of 'pure' field only

ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
#define VL_CFUNCTION_ID(id, method, pure, args) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#undef VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    ଗଣନା en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ VL_PURE {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_CFUNCTION_NAME(id, function, pure, args) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#undef VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    bool isPure() ଅଚଳ VL_PURE {
        ସ୍ଥିର ଅଚଳ bool ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_CFUNCTION_PURE(id, method, pure, args) pure,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_PURE)
#undef VL_CFUNCTION_PURE
                false  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* args() ଅଚଳ VL_PURE {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_CFUNCTION_ARGS(id, method, pure, args) args,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ARGS)
#undef VL_CFUNCTION_ARGS
                ""  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
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
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

ବ୍ୟକ୍ତିଗତ:
    ସଂରଚନା Item final {
        ଗଣନା en m_e;  // Statement's enum mnemonic, for checking
        ଅଚଳ ଅକ୍ଷର* m_name;  // Statements name, for debugging
    };
    ସ୍ଥିର Item s_itemData[];

ସର୍ବସାଧାରଣ:
    ଗଣନା en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ VL_PURE {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"none", "ctor_var_reset_call"};
        ଫେରନ୍ତୁ names[m_e];
    }
    bool isNone() ଅଚଳ { ଫେରନ୍ତୁ m_e == NONE; }
};
constexpr bool operator==(ଅଚଳ VCStmtType& lhs, ଅଚଳ VCStmtType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VCStmtType& lhs, VCStmtType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VCStmtType::en lhs, ଅଚଳ VCStmtType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VCStmtType& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    ଗଣନା en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ VL_PURE {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VCaseType& lhs, ଅଚଳ VCaseType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VCaseType& lhs, VCaseType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VCaseType::en lhs, ଅଚଳ VCaseType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        ଫେରନ୍ତୁ names[m_e];
    }
    bool isAssignable() ଅଚଳ { ଫେରନ୍ତୁ m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
};
constexpr bool operator==(ଅଚଳ VCastable& lhs, ଅଚଳ VCastable& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VCastable& lhs, VCastable::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VCastable::en lhs, ଅଚଳ VCastable& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VCastable& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
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
    ଗଣନା en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }  // LCOV_EXCL_LINE
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        ଫେରନ୍ତୁ names[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ଅଚଳ ଅକ୍ଷର* verilogKwd() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        ଫେରନ୍ତୁ names[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ଅଚଳ ଅକ୍ଷର* binSetEnum() ଅଚଳ {
        ବାଛ (m_e) {
        ସ୍ଥିତି BINS_IGNORE: ଫେରନ୍ତୁ "VlCovBinKind::KIND_IGNORE";
        ସ୍ଥିତି BINS_ILLEGAL: ଫେରନ୍ତୁ "VlCovBinKind::KIND_ILLEGAL";
        ସ୍ଥିତି BINS_DEFAULT: ଫେରନ୍ତୁ "VlCovBinKind::KIND_DEFAULT";
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool binIsNormal() ଅଚଳ {
        ଫେରନ୍ତୁ m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool operator==(ଅଚଳ VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        // Shared by option.* and type_option.*
        WEIGHT,
        GOAL,
        AT_LEAST,
        AUTO_BIN_MAX,
        PER_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_ଅଙ୍କ_PRINT_MISSING,
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
    ଗଣନା en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"weight",
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
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    ଗଣନା en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }  // LCOV_EXCL_LINE
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"", "[*]", "[->]", "[=]"};
        ଫେରନ୍ତୁ names[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* asciiJson() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        ଫେରନ୍ତୁ names[m_e];
    }
};

//######################################################################

class VDirection final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { NONE, ନିବେଶ, OUTPUT, INOUT, REF, CONSTREF };
    ଗଣନା en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        ଫେରନ୍ତୁ names[m_e];
    }
    string verilogKwd() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        ଫେରନ୍ତୁ names[m_e];
    }
    string prettyName() ଅଚଳ { ଫେରନ୍ତୁ verilogKwd(); }
    bool isAny() ଅଚଳ { ଫେରନ୍ତୁ m_e != NONE; }
    bool isInout() ଅଚଳ { ଫେରନ୍ତୁ m_e == INOUT; }
    bool isInoutOrRef() ଅଚଳ { ଫେରନ୍ତୁ m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_ନିବେଶ() ଅଚଳ { ଫେରନ୍ତୁ m_e == ନିବେଶ; }
    bool isNonOutput() ଅଚଳ {
        ଫେରନ୍ତୁ m_e == ନିବେଶ || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ଅଚଳ { ଫେରନ୍ତୁ m_e == OUTPUT; }
    bool isReadOnly() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == ନିବେଶ || m_e == CONSTREF; }
    bool isWritable() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == REF; }
    bool isConstRef() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == CONSTREF; }
    string traceSigDirection() ଅଚଳ {
        ଯଦି (isInout()) {
            ଫେରନ୍ତୁ "VerilatedTraceSigDirection::INOUT";
        } ଅନ୍ୟଥା ଯଦି (isWritable()) {
            ଫେରନ୍ତୁ "VerilatedTraceSigDirection::OUTPUT";
        } ଅନ୍ୟଥା ଯଦି (isNonOutput()) {
            ଫେରନ୍ତୁ "VerilatedTraceSigDirection::INPUT";
        }
        ଫେରନ୍ତୁ "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ଅଚଳ {
        ବାଛ (m_e) {
        ସ୍ଥିତି NONE: ଫେରନ୍ତୁ VAccess::NOCHANGE;
        ସ୍ଥିତି ନିବେଶ:
        ସ୍ଥିତି CONSTREF: ଫେରନ୍ତୁ VAccess::READ;
        ସ୍ଥିତି OUTPUT: ଫେରନ୍ତୁ VAccess::WRITE;
        ସ୍ଥିତି INOUT:
        ସ୍ଥିତି REF: ଫେରନ୍ତୁ VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool operator==(ଅଚଳ VDirection& lhs, ଅଚଳ VDirection& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VDirection& lhs, VDirection::en rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VDirection::en lhs, ଅଚଳ VDirection& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
}
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VDirection& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    ଗଣନା en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    bool addNewline() ଅଚଳ { ଫେରନ୍ତୁ m_e != DT_WRITE; }
    bool needScopeTracking() ଅଚଳ { ଫେରନ୍ତୁ m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VDisplayType& lhs, ଅଚଳ VDisplayType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VDisplayType& lhs, VDisplayType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VDisplayType::en lhs, ଅଚଳ VDisplayType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { FILE, VARS, ALL, FLUSH, LIMIT, OFF, ON };
    ଗଣନା en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VDumpCtlType& lhs, ଅଚଳ VDumpCtlType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VDumpCtlType& lhs, VDumpCtlType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VDumpCtlType::en lhs, ଅଚଳ VDumpCtlType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
ସର୍ବସାଧାରଣ:
    // REMEMBER to edit the strings below too
    ଗଣନା en : uint8_t {
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
    ଗଣନା en m_e;
    bool clockedStmt() ଅଚଳ {
        ସ୍ଥିର constexpr bool clocked[] = {
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
        ଫେରନ୍ତୁ clocked[m_e];
    }
    bool anEdge() ଅଚଳ { ଫେରନ୍ତୁ m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ଅଚଳ {
        ବାଛ (m_e) {
        ସ୍ଥିତି ET_CHANGED: ଫେରନ୍ତୁ ET_CHANGED;
        ସ୍ଥିତି ET_BOTHEDGE: ଫେରନ୍ତୁ ET_BOTHEDGE;
        ସ୍ଥିତି ET_POSEDGE: ଫେରନ୍ତୁ ET_NEGEDGE;
        ସ୍ଥିତି ET_NEGEDGE: ଫେରନ୍ତୁ ET_POSEDGE;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: UASSERT_STATIC(0, "Inverting bad edgeType()"); ଫେରନ୍ତୁ ET_NEGEDGE;
        }
    }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        ଫେରନ୍ତୁ names[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* verilogKwd() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        ଫେରନ୍ତୁ names[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusiveEdge(ଅଚଳ VEdgeType& other) ଅଚଳ {
        ବାଛ (m_e) {
        ସ୍ଥିତି VEdgeType::ET_POSEDGE:
            ଯଦି (other.m_e == VEdgeType::ET_NEGEDGE) ଫେରନ୍ତୁ true;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VEdgeType::ET_NEGEDGE:
            ଯଦି (other.m_e == VEdgeType::ET_POSEDGE) ଫେରନ୍ତୁ true;
            ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଭାଙ୍ଗ;
        }
        ଫେରନ୍ତୁ false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
};
constexpr bool operator==(ଅଚଳ VEdgeType& lhs, ଅଚଳ VEdgeType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VEdgeType& lhs, VEdgeType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VEdgeType::en lhs, ଅଚଳ VEdgeType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }

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
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
#define VL_EVAL_ID(id, iterated, triggers, first, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#undef VL_EVAL_ID
            _ENUM_END
    };
    ଗଣନା en m_e;

    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_EVAL_NAME(id, iterated, triggers, first, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#undef VL_EVAL_NAME
                "_ENUM_END"  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    bool isIterated() ଅଚଳ {
        ସ୍ଥିର constexpr bool ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_EVAL_IS_ITERATED(id, iterated, triggers, first, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#undef VL_EVAL_IS_ITERATED
                false  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    bool hasTriggers() ଅଚଳ {
        ସ୍ଥିର constexpr bool ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, first, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#undef VL_EVAL_HAS_TRIGGERS
                false  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    bool firstIteration() ଅଚଳ {
        ସ୍ଥିର constexpr bool ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_EVAL_FIRST(id, iterated, triggers, first, slow) first,
            FOR_EACH_EVAL(VL_EVAL_FIRST)
#undef VL_EVAL_FIRST
                false  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
    bool slow() ଅଚଳ {
        ସ୍ଥିର constexpr bool ମୂଲ୍ୟଗୁଡ଼ିକ[] = {
#define VL_EVAL_SLOW(id, iterated, triggers, first, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#undef VL_EVAL_SLOW
                false  //
        };
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }

    // Short name
    std::string tag() ଅଚଳ { ଫେରନ୍ତୁ VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ଅଚଳ { ଫେରନ୍ତୁ "_eval_" + tag(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string evalMethod() ଅଚଳ { ଫେରନ୍ତୁ "eval" + capitalizedTag(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ଅଚଳ { ଫେରନ୍ତୁ "_eval_dump_triggers__" + tag(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dumpTriggersMethod() ଅଚଳ { ଫେରନ୍ତୁ "dumpTriggers" + capitalizedTag(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}
    operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }

ବ୍ୟକ୍ତିଗତ:
    std::string capitalizedTag() ଅଚଳ {
        ଅଚଳ std::string name = ascii();
        ଫେରନ୍ତୁ name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#undef FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        ଫେରନ୍ତୁ names[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    // Is a node type compatible with the declaration
    bool is_ଗଣ୍ଠି_Compatible(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ;
};
constexpr bool operator==(ଅଚଳ VFwdType& lhs, ଅଚଳ VFwdType& rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ଅଚଳ VFwdType& lhs, VFwdType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VFwdType::en lhs, ଅଚଳ VFwdType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VFwdType& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

class VIsCached final {
    // Used in some nodes to cache results of boolean methods
    // If cachedCnt == 0, not cached
    // else if cachedCnt == s_cachedCntGbl, then m_state is if cached
    uint64_t m_cachedCnt : 63;  // Mark of when cache was computed
    uint64_t m_state : 1;
    ସ୍ଥିର uint64_t s_cachedCntGbl;  // Global computed count

ସର୍ବସାଧାରଣ:
    VIsCached()
        : m_cachedCnt{0}
        , m_state{0} {}
    bool isCached() ଅଚଳ { ଫେରନ୍ତୁ m_cachedCnt == s_cachedCntGbl; }
    bool get() ଅଚଳ { ଫେରନ୍ତୁ m_state; }
    ଶୂନ୍ୟ set(bool flag) {
        m_cachedCnt = s_cachedCntGbl;
        m_state = flag;
    }
    ଶୂନ୍ୟ clearCache() {
        m_cachedCnt = 0;
        m_state = 0;
    }
    ସ୍ଥିର ଶୂନ୍ୟ clear_Cache_ବୃକ୍ଷ() {
        ++s_cachedCntGbl;
        // 64 bits so won't overflow
        // UASSERT_STATIC(s_cachedCntGbl < MAX_CNT, "Overflow of cache counting");
    }
};

//######################################################################

/// Join type
class VJoinType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    ଗଣନା en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        ଫେରନ୍ତୁ names[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* verilogKwd() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"join", "join_any", "join_none"};
        ଫେରନ୍ତୁ names[m_e];
    }
    bool join() ଅଚଳ { ଫେରନ୍ତୁ m_e == JOIN; }
    bool joinAny() ଅଚଳ { ଫେରନ୍ତୁ m_e == JOIN_ANY; }
    bool joinNone() ଅଚଳ { ଫେରନ୍ତୁ m_e == JOIN_NONE; }
};
constexpr bool operator==(ଅଚଳ VJoinType& lhs, ଅଚଳ VJoinType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VJoinType& lhs, VJoinType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VJoinType::en lhs, ଅଚଳ VJoinType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VJoinType& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        ଫେରନ୍ତୁ names[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    bool isNone() ଅଚଳ { ଫେରନ୍ତୁ m_e == NONE; }
    bool isAutomatic() ଅଚଳ { ଫେରନ୍ତୁ m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ଅଚଳ { ଫେରନ୍ତୁ m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ଅଚଳ { ଫେରନ୍ତୁ m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ଅଚଳ {
        ବାଛ (m_e) {
        ସ୍ଥିତି AUTOMATIC_EXPLICIT: ଫେରନ୍ତୁ AUTOMATIC_IMPLICIT;
        ସ୍ଥିତି STATIC_EXPLICIT: ଫେରନ୍ତୁ STATIC_IMPLICIT;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ m_e;
        }
    }
};
constexpr bool operator==(ଅଚଳ VLifetime& lhs, ଅଚଳ VLifetime& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VLifetime& lhs, VLifetime::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VLifetime::en lhs, ଅଚଳ VLifetime& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VLifetime& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
ସର୍ବସାଧାରଣ:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_index;  // Current index
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_inc;  // Step, +1 or -1
        ସର୍ବସାଧାରଣ:
            Iterator(ପୂର୍ଣ୍ଣସଂଖ୍ୟା index, ପୂର୍ଣ୍ଣସଂଖ୍ୟା inc)
                : m_index{index}
                , m_inc{inc} {}
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା operator*() ଅଚଳ { ଫେରନ୍ତୁ m_index; }
            Iterator& operator++() {
                m_index += m_inc;
                ଫେରନ୍ତୁ *this;
            }
            bool operator!=(ଅଚଳ Iterator& that) ଅଚଳ { ଫେରନ୍ତୁ m_index != that.m_index; }
        };
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_begin;  // First index
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_end;  // One past the last index
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_inc;  // Step, +1 or -1
    ସର୍ବସାଧାରଣ:
        Sequence(ପୂର୍ଣ୍ଣସଂଖ୍ୟା first, ପୂର୍ଣ୍ଣସଂଖ୍ୟା last, ପୂର୍ଣ୍ଣସଂଖ୍ୟା inc)
            : m_begin{first}
            , m_end{last + inc}
            , m_inc{inc} {}
        Iterator begin() ଅଚଳ { ଫେରନ୍ତୁ Iterator{m_begin, m_inc}; }
        Iterator end() ଅଚଳ { ଫେରନ୍ତୁ Iterator{m_end, m_inc}; }
    };

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ବାମ = 0;  // Left side of range (pre-':')
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ଡାହାଣ = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool operator==(ଅଚଳ VNumRange& rhs) ଅଚଳ {
        ଫେରନ୍ତୁ m_ବାମ == rhs.m_ବାମ && m_ଡାହାଣ == rhs.m_ଡାହାଣ && m_ranged == rhs.m_ranged;
    }
    bool operator<(ଅଚଳ VNumRange& rhs) ଅଚଳ {
        ଯଦି ((m_ବାମ < rhs.m_ବାମ)) ଫେରନ୍ତୁ true;
        ଯଦି (!(m_ବାମ == rhs.m_ବାମ)) ଫେରନ୍ତୁ false;  // lhs > rhs
        ଯଦି ((m_ଡାହାଣ < rhs.m_ଡାହାଣ)) ଫେରନ୍ତୁ true;
        ଯଦି (!(m_ଡାହାଣ == rhs.m_ଡାହାଣ)) ଫେରନ୍ତୁ false;  // lhs > rhs
        ଯଦି ((m_ranged < rhs.m_ranged)) ଫେରନ୍ତୁ true;
        ଯଦି (!(m_ranged == rhs.m_ranged)) ଫେରନ୍ତୁ false;  // lhs > rhs
        ଫେରନ୍ତୁ false;
    }
    //
    VNumRange() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VNumRange(ପୂର୍ଣ୍ଣସଂଖ୍ୟା hi, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ)
        : m_ବାମ{ବାମ}
        , m_ଡାହାଣ{ଡାହାଣ}
        , m_ranged{true} {}
    ~VNumRange() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // MEMBERS
    ଶୂନ୍ୟ init(ପୂର୍ଣ୍ଣସଂଖ୍ୟା hi, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lo, bool ascending) {
        ଯଦି (lo > hi) std::swap(hi, lo);
        m_ବାମ = ascending ? lo : hi;
        m_ଡାହାଣ = ascending ? hi : lo;
        m_ranged = true;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ() ଅଚଳ { ଫେରନ୍ତୁ m_ବାମ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ() ଅଚଳ { ଫେରନ୍ତୁ m_ଡାହାଣ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା hi() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_ବାମ > m_ଡାହାଣ ? m_ବାମ : m_ଡାହାଣ;
    }  // How to show a declaration
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lo() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_ବାମ > m_ଡାହାଣ ? m_ଡାହାଣ : m_ବାମ;
    }  // How to show a declaration
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ_To_ଡାହାଣ_Inc() ଅଚଳ { ଫେରନ୍ତୁ ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_ବାମ_To_ଡାହାଣ() ଅଚଳ { ଫେରନ୍ତୁ {m_ବାମ, m_ଡାହାଣ, ବାମ_To_ଡାହାଣ_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_ଡାହାଣ_To_ବାମ() ଅଚଳ { ଫେରନ୍ତୁ {m_ଡାହାଣ, m_ବାମ, -ବାମ_To_ଡାହାଣ_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seqLoToHi() ଅଚଳ { ଫେରନ୍ତୁ {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seqHiToLo() ଅଚଳ { ଫେରନ୍ତୁ {hi(), lo(), -1}; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା elements() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ hi() - lo() + 1; }
    bool ranged() ଅଚଳ { ଫେରନ୍ତୁ m_ranged; }
    bool ascending() ଅଚଳ { ଫେରନ୍ତୁ m_ବାମ < m_ଡାହାଣ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା hiMaxSelect() ଅଚଳ {
        ଫେରନ୍ତୁ (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ଅଚଳ {
        ଯଦି (ranged()) {
            ଫେରନ୍ତୁ "["s + std::to_string(ବାମ()) + ":" + std::to_string(ଡାହାଣ()) + "]";
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ "[norg]";
        }
    }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ { str << ascii(); }
};
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VNumRange& rhs) {
    rhs.dump(os);
    ଫେରନ୍ତୁ os;
}

// ######################################################################

class VPragmaType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
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
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {
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
        ଫେରନ୍ତୁ names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
};
constexpr bool operator==(ଅଚଳ VPragmaType& lhs, ଅଚଳ VPragmaType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VPragmaType& lhs, VPragmaType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VPragmaType::en lhs, ଅଚଳ VPragmaType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VPragmaType& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        ଫେରନ୍ତୁ names[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    bool isRandomizable() ଅଚଳ { ଫେରନ୍ତୁ m_e != NONE; }
    bool isRand() ଅଚଳ { ଫେରନ୍ତୁ m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ଅଚଳ { ଫେରନ୍ତୁ m_e == RAND_CYCLIC; }
};
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VRandAttr& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelfPointerText final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ସ୍ଥିର ଅଚଳ std::shared_ptr<ଅଚଳ string> s_emptyp;  // Holds ""
    ସ୍ଥିର ଅଚଳ std::shared_ptr<ଅଚଳ string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ଅଚଳ string> m_strp;

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    class Empty {};  // for creator type-overload selection
    explicit VSelfPointerText(Empty)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelfPointerText(This)
        : m_strp{s_thisp} {}
    VSelfPointerText(This, ଅଚଳ string& field)
        : m_strp{std::make_shared<ଅଚଳ string>("this->" + field)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelfPointerText(VlSyms, ଅଚଳ string& field)
        : m_strp{std::make_shared<ଅଚଳ string>("(&vlSymsp->vlInst." + field + ')')} {}

    // METHODS
    bool isEmpty() ଅଚଳ { ଫେରନ୍ତୁ m_strp == s_emptyp; }
    bool isVlSym() ଅଚଳ { ଫେରନ୍ତୁ m_strp->find("vlSymsp") != string::npos; }
    bool hasThis() ଅଚଳ { ଫେରନ୍ତୁ m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ଅଚଳ;
    ସ୍ଥିର string replaceThis(bool useSelfForThis, ଅଚଳ string& text);
    ଅଚଳ std::string& asString() ଅଚଳ { ଫେରନ୍ତୁ *m_strp; }
    bool operator==(ଅଚଳ VSelfPointerText& other) ଅଚଳ { ଫେରନ୍ତୁ *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        ଫେରନ୍ତୁ names[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    ସ୍ଥିର VSigning fromBool(bool isSigned) {  // Factory method
        ଫେରନ୍ତୁ isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    bool isSigned() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == SIGNED; }
    bool isNosign() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool operator==(ଅଚଳ VSigning& lhs, ଅଚଳ VSigning& rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ଅଚଳ VSigning& lhs, VSigning::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VSigning::en lhs, ଅଚଳ VSigning& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VSigning& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

class VStrength final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    ଗଣନା en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VStrength& lhs, ଅଚଳ VStrength& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VStrength& lhs, VStrength::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VStrength::en lhs, ଅଚଳ VStrength& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VStrength& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    ଗଣନା en m_e;
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        ଫେରନ୍ତୁ names[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
};

//######################################################################

class VTracePrefixType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    ଗଣନା en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTracePrefixType(en _e)
        : m_e{_e} {}
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VTracePrefixType& lhs, ଅଚଳ VTracePrefixType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VTracePrefixType& lhs, VTracePrefixType::en rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VTracePrefixType::en lhs, ଅଚଳ VTracePrefixType& rhs) {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
}
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VTracePrefixType& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

//######################################################################

class VTraceType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        FULL,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    ଗଣନା en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"CONSTANT", "FULL", "CHANGE"};
        ଫେରନ୍ତୁ names[m_e];
    }
    ଅଚଳ ଅକ୍ଷର* func_prefix() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"trace_const", "trace_full", "trace_chg"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VTraceType& lhs, ଅଚଳ VTraceType& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VTraceType& lhs, VTraceType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VTraceType::en lhs, ଅଚଳ VTraceType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VTraceType& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

//######################################################################

class VUseType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    ଗଣନା en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    bool containsAny(VUseType other) ଅଚଳ { ଫେରନ୍ତୁ m_e & other.m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        ଫେରନ୍ତୁ names[m_e - 1];
    }
};
constexpr bool operator==(ଅଚଳ VUseType& lhs, ଅଚଳ VUseType& rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs.m_e; }
constexpr bool operator==(ଅଚଳ VUseType& lhs, VUseType::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VUseType::en lhs, ଅଚଳ VUseType& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
constexpr VUseType::en operator|(VUseType::en lhs, VUseType::en rhs) {
    ଫେରନ୍ତୁ VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en operator&(VUseType::en lhs, VUseType::en rhs) {
    ଫେରନ୍ତୁ VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VUseType& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

class VVarType final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
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
    ଗଣନା en m_e;
    VVarType() VL_MT_SAFE : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_SAFE : m_e{_e} {}
    explicit VVarType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        ଫେରନ୍ତୁ names[m_e];
    }
    bool isParam() ଅଚଳ { ଫେରନ୍ତୁ m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ଅଚଳ {
        ଫେରନ୍ତୁ (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == PORT || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ଅଚଳ {
        ଫେରନ୍ତୁ (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ଅଚଳ { ଫେରନ୍ତୁ (m_e == TRIOR); }
    bool isWiredNet() ଅଚଳ { ଫେରନ୍ତୁ (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ଅଚଳ {  // In Verilog, always ok in SystemVerilog
        ଫେରନ୍ତୁ (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == PORT || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ଅଚଳ {
        ଫେରନ୍ତୁ (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool isTemp() ଅଚଳ {
        ଫେରନ୍ତୁ (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ଅଚଳ {
        ଫେରନ୍ତୁ (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == PORT
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ଅଚଳ ଅକ୍ଷର* traceSigKind() ଅଚଳ {
        // VerilatedTraceSigKind to used in trace signal declaration
        ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ lut[] = {
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
        ଫେରନ୍ତୁ lut[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VVarType& lhs, ଅଚଳ VVarType& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VVarType& lhs, VVarType::en rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VVarType::en lhs, ଅଚଳ VVarType& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
}
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VVarType& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_ଚାବି final {
ସର୍ବସାଧାରଣ:
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_width;  // From AstNodeDType: Bit width of operation
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_widthMin;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ଅଚଳ VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ଅଚଳ VSigning m_numeric;  // From AstNodeDType: Node is signed
    ଅଚଳ VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool operator==(ଅଚଳ VBasic_Type_ଚାବି& rhs) ଅଚଳ {
        ଫେରନ୍ତୁ m_width == rhs.m_width && m_widthMin == rhs.m_widthMin && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool operator<(ଅଚଳ VBasic_Type_ଚାବି& rhs) ଅଚଳ {
        ଯଦି ((m_width < rhs.m_width)) ଫେରନ୍ତୁ true;
        ଯଦି (!(m_width == rhs.m_width)) ଫେରନ୍ତୁ false;  // lhs > rhs
        ଯଦି ((m_widthMin < rhs.m_widthMin)) ଫେରନ୍ତୁ true;
        ଯଦି (!(m_widthMin == rhs.m_widthMin)) ଫେରନ୍ତୁ false;  // lhs > rhs
        ଯଦି ((m_numeric < rhs.m_numeric)) ଫେରନ୍ତୁ true;
        ଯଦି (!(m_numeric == rhs.m_numeric)) ଫେରନ୍ତୁ false;  // lhs > rhs
        ଯଦି ((m_keyword < rhs.m_keyword)) ଫେରନ୍ତୁ true;
        ଯଦି (!(m_keyword == rhs.m_keyword)) ଫେରନ୍ତୁ false;  // lhs > rhs
        ଯଦି ((m_nrange < rhs.m_nrange)) ଫେରନ୍ତୁ true;
        ଯଦି (!(m_nrange == rhs.m_nrange)) ଫେରନ୍ତୁ false;  // lhs > rhs
        ଫେରନ୍ତୁ false;
    }
    VBasic_Type_ଚାବି(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric, VBasicDTypeKwd kwd,
                  ଅଚଳ VNumRange& nrange)
        : m_width{width}
        , m_widthMin{widthMin}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_ଚାବି() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

#endif  // Guard
