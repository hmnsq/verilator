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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ASTATTR_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ASTATTR_H_

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3AST_H_
#ದೋಷ "Use V3Ast.h as the include"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ವ್ಯಾಖ್ಯಾನಿಸು VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ಆದರೆ_ಅಂತ್ಯ

// Hint class so we can choose constructors
class VFlag_ಬಿಟ್_Packed {};
class VFlag_ಮಗು_DType {};  // Used by parser.y to select constructor that sets childDType
class VFlagLogicPacked {};
class VFlagLogic2StatePacked {};

// ######################################################################

class VAbort_ಬಗೆ final {
ಸಾರ್ವಜನಿಕ:
    // IEEE 1800-2023 16.12.14 property abort operators.
    ಎಣಿಕೆ en : uint8_t {
        ACCEPT_ON,  // accept_on(cond) prop: async abort, property succeeds
        REJECT_ON,  // reject_on(cond) prop: async abort, property fails
        SYNC_ACCEPT_ON,  // sync_accept_on(cond) prop: sampled at matured clock, succeeds
        SYNC_REJECT_ON  // sync_reject_on(cond) prop: sampled at matured clock, fails
    };
    ಎಣಿಕೆ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VAbort_ಬಗೆ(en _e)
        : m_e{_e} {}
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"accept_on", "reject_on", "sync_accept_on", "sync_reject_on"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    bool isAccept() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == ACCEPT_ON || m_e == SYNC_ACCEPT_ON; }
};

//######################################################################

class VAccess final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        READ,  // Read/Consumed, variable not changed
        WRITE,  // Written/Updated, variable might be updated, but not consumed
        //      // so variable might be removable if not consumed elsewhere
        READWRITE,  // Read/Consumed and written/updated, variable both set and
        //          // also consumed, cannot remove usage of variable.
        //          // For non-simple data types only e.g. no tristates/delayed vars.
        NOCHANGE  // No change to previous state, used only in V3LinkLValue
    };
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"RD", "WR", "RW", "--"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* arrow() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"[RV] <-", "[LV] =>", "[LRV] <=>", "--"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    VAccess()
        : m_e{READ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAccess(en _e)
        : m_e{_e} {}
    explicit VAccess(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    VAccess invert() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (m_e == READWRITE) ? VAccess{m_e} : (m_e == WRITE ? VAccess{READ} : VAccess{WRITE});
    }
    bool isReadOnly() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == READ; }  // False with READWRITE
    bool isWriteOnly() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == WRITE; }  // False with READWRITE
    bool isReadOrRW() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == READ || m_e == READWRITE; }
    bool isWriteOrRW() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == WRITE || m_e == READWRITE; }
    bool isRW() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == READWRITE; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAccess& lhs, ಸ್ಥಿರಾಂಕ VAccess& rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAccess& lhs, VAccess::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VAccess::en lhs, ಸ್ಥಿರಾಂಕ VAccess& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VAccess& rhs) { ಹಿಂತಿರುಗಿ os << rhs.ascii(); }

//######################################################################

class VAlwaysKwd final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { ALWAYS, ALWAYS_FF, ALWAYS_LATCH, ALWAYS_COMB, CONT_ನಿಯೋಜಿಸು };
    ಎಣಿಕೆ en m_e;
    VAlwaysKwd()
        : m_e{ALWAYS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAlwaysKwd(en _e)
        : m_e{_e} {}
    explicit VAlwaysKwd(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"always", "always_ff", "always_latch", "always_comb", "cont_assign"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAlwaysKwd& lhs, ಸ್ಥಿರಾಂಕ VAlwaysKwd& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAlwaysKwd& lhs, VAlwaysKwd::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VAlwaysKwd::en lhs, ಸ್ಥಿರಾಂಕ VAlwaysKwd& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }

// ######################################################################

class VAssertCtlType final {
ಸಾರ್ವಜನಿಕ:
    // IEEE 1800-2023 Table 20-5
    ಎಣಿಕೆ en : uint8_t {
        ಗಮ್ಯ_BE_EVALUATED = 0,
        ಬೀಗ = 1,
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
    ಎಣಿಕೆ en m_e;
    VAssertCtlType()
        : m_e{ಗಮ್ಯ_BE_EVALUATED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertCtlType(en _e)
        : m_e{_e} {}
    explicit VAssertCtlType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        // IEEE 1800-2023 20.11
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"",
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
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAssertCtlType& lhs, ಸ್ಥಿರಾಂಕ VAssertCtlType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAssertCtlType& lhs, VAssertCtlType::en rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VAssertCtlType::en lhs, ಸ್ಥಿರಾಂಕ VAssertCtlType& rhs) {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
}

// ######################################################################

class VAssertDirectiveType final {
ಸಾರ್ವಜನಿಕ:
    // IEEE 1800-2023 Table 20-7
    ಎಣಿಕೆ en : uint8_t {
        ಆಂತರಿಕ = 0,  // Non IEEE type, for directives to be evaluated from expression.
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
    ಎಣಿಕೆ en m_e;
    VAssertDirectiveType()
        : m_e{ASSERT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertDirectiveType(en _e)
        : m_e{_e} {}
    explicit VAssertDirectiveType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    string ascii() ಸ್ಥಿರಾಂಕ {
        std::stringstream types;
        ಆದರೆ (m_e == ಆಂತರಿಕ)
            types << "INTERNAL ";
        ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_e & ASSERT) types << "ASSERT ";
            ಆದರೆ (m_e & COVER) types << "COVER ";
            ಆದರೆ (m_e & ASSUME) types << "ASSUME ";
            ಆದರೆ (m_e & VIOLATION_CASE) types << "VIOLATION_CASE ";
            ಆದರೆ (m_e & VIOLATION_IF) types << "VIOLATION_IF ";
            ಆದರೆ (m_e & INTRINSIC) types << "INTRINSIC ";
            ಆದರೆ (m_e & RESTRICT) types << "RESTRICT ";
        }
        ಸ್ಥಿರಾಂಕ string str = types.str();
        UASSERT(!str.ಖಾಲಿಗಳು(), "Assert should be of one of types");
        ಹಿಂತಿರುಗಿ str.substr(0, str.ಗಾತ್ರ_2() - 1);
    }
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAssertDirectiveType& lhs, ಸ್ಥಿರಾಂಕ VAssertDirectiveType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAssertDirectiveType& lhs, VAssertDirectiveType::en rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VAssertDirectiveType::en lhs, ಸ್ಥಿರಾಂಕ VAssertDirectiveType& rhs) {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
}
constexpr VAssertDirectiveType::en ಕ್ರಿಯಾಕಾರಕ|(VAssertDirectiveType::en lhs,
                                             VAssertDirectiveType::en rhs) {
    ಹಿಂತಿರುಗಿ VAssertDirectiveType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VAssertType final {
ಸಾರ್ವಜನಿಕ:
    // IEEE 1800-2023 Table 20-6
    ಎಣಿಕೆ en : uint8_t {
        ಆಂತರಿಕ = 0,  // Non IEEE type, for assertions that should not be controlled.
        CONCURRENT = (1 << 0),
        SIMPLE_ತಕ್ಷಣ = (1 << 1),
        OBSERVED_DEFERRED_ತಕ್ಷಣ = (1 << 2),
        FINAL_DEFERRED_ತಕ್ಷಣ = (1 << 3),
        EXPECT = (1 << 4),
        UNIQUE = (1 << 5),
        UNIQUE0 = (1 << 6),
        ಆದ್ಯತೆ = (1 << 7),
    };
    ಎಣಿಕೆ en m_e;
    VAssertType()
        : m_e{ಆಂತರಿಕ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAssertType(en _e)
        : m_e{_e} {}
    explicit VAssertType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    bool ಒಳಗೊಂಡಿದೆ_Any(VAssertType other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e & other.m_e; }
    string ascii() ಸ್ಥಿರಾಂಕ {
        std::stringstream types;
        ಆದರೆ (m_e == ಆಂತರಿಕ)
            types << "INTERNAL ";
        ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (m_e & CONCURRENT) types << "CONCURRENT ";
            ಆದರೆ (m_e & SIMPLE_ತಕ್ಷಣ) types << "SIMPLE_IMMEDIATE ";
            ಆದರೆ (m_e & OBSERVED_DEFERRED_ತಕ್ಷಣ) types << "OBSERVED_DEFERRED_IMMEDIATE ";
            ಆದರೆ (m_e & FINAL_DEFERRED_ತಕ್ಷಣ) types << "FINAL_DEFERRED_IMMEDIATE ";
            ಆದರೆ (m_e & EXPECT) types << "EXPECT ";
            ಆದರೆ (m_e & UNIQUE) types << "UNIQUE ";
            ಆದರೆ (m_e & UNIQUE0) types << "UNIQUE0 ";
            ಆದರೆ (m_e & ಆದ್ಯತೆ) types << "PRIORITY ";
        }
        ಸ್ಥಿರಾಂಕ string str = types.str();
        UASSERT(!str.ಖಾಲಿಗಳು(), "Assert should be of one of types");
        ಹಿಂತಿರುಗಿ str.substr(0, str.ಗಾತ್ರ_2() - 1);
    }
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAssertType& lhs, ಸ್ಥಿರಾಂಕ VAssertType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAssertType& lhs, VAssertType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VAssertType::en lhs, ಸ್ಥಿರಾಂಕ VAssertType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
constexpr VAssertType::en ಕ್ರಿಯಾಕಾರಕ|(VAssertType::en lhs, VAssertType::en rhs) {
    ಹಿಂತಿರುಗಿ VAssertType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

// ######################################################################

class VPropStrength final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        DEFAULT = 0,  // Resolve from assertion/coverage context
        WEAK,
        STRONG,
    };
    ಎಣಿಕೆ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VPropStrength(en _e)
        : m_e{_e} {}
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"default", "weak", "strong"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VPropStrength& lhs, ಸ್ಥಿರಾಂಕ VPropStrength& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VPropStrength& lhs, VPropStrength::en rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ!=(ಸ್ಥಿರಾಂಕ VPropStrength& lhs, VPropStrength::en rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e != rhs;
}

// ######################################################################

class VAttrType final {
ಸಾರ್ವಜನಿಕ:
    // clang-format off
    ಎಣಿಕೆ en: uint8_t  {
        ILLEGAL,
        //
        DIM_ಬಿಟ್ಟುಗಳು,                       // V3Const converts to constant
        DIM_ಬಿಟ್ಟುಗಳು_OR_ಸಂಖ್ಯೆ,             // V3Const converts to constant
        DIM_DIMENSIONS,                 // V3Width converts to constant
        DIM_ಮೇಲಿನ,                       // V3Width processes
        DIM_INCREMENT,                  // V3Width processes
        DIM_ಎಡ,                       // V3Width processes
        DIM_ಕೆಳಗಿನ,                        // V3Width processes
        DIM_ಬಲ,                      // V3Width processes
        DIM_ಗಾತ್ರ,                       // V3Width processes
        DIM_UNPK_DIMENSIONS,            // V3Width converts to constant
        //
        DT_ಬಹಿರಂಗ,                      // V3LinkParse moves to AstTypedef::attrPublic
        //
        ENUM_ಮೊದಲ,                     // V3Width processes
        ENUM_ಕೊನೆಯ,                      // V3Width processes
        ENUM_ಅಂಕೆ,                       // V3Width processes
        ENUM_ಮುಂದಿನ,                      // V3Width processes
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
        VAR_ತಳ,                       // V3LinkResolve creates for AstPreSel, V3LinkParam removes
        VAR_FORCEABLE,                  // V3LinkParse moves to AstVar::isForceable
        VAR_FSM_ARC_INCLUDE_COND,       // V3LinkParse moves to AstVar::attrFsmArcInclCond
        VAR_FSM_ARC_INCLUDE_COND_AUTO,  // Flavors of above option - setting expand mode per FSM
        VAR_FSM_ARC_INCLUDE_COND_AUTO_EXPAND,
        VAR_FSM_ARC_INCLUDE_COND_ತುಂಬಿದ,
        VAR_FSM_STATE,                  // V3LinkParse moves to AstVar::attrFsmState
        VAR_FSM_STATE_AUTO,             // Flavors of above option - setting expand mode per FSM
        VAR_FSM_STATE_AUTO_EXPAND,
        VAR_FSM_STATE_ತುಂಬಿದ,
        VAR_ಪೋರ್ಟ್_DTYPE,                 // V3LinkDot for V3Width to check port dtype
        VAR_ಬಹಿರಂಗ,                     // V3LinkParse moves to AstVar::sigPublic
        VAR_ಬಹಿರಂಗ_FLAT,                // V3LinkParse moves to AstVar::sigPublic
        VAR_ಬಹಿರಂಗ_FLAT_RD,             // V3LinkParse moves to AstVar::sigPublic
        VAR_ಬಹಿರಂಗ_FLAT_RW,             // V3LinkParse moves to AstVar::sigPublic
        VAR_SC_BIGUINT,                 // V3LinkParse moves to AstVar::attrScBigUint
        VAR_SC_BV,                      // V3LinkParse moves to AstVar::attrScBv
        VAR_SFORMAT,                    // V3LinkParse moves to AstVar::attrSFormat
        VAR_ವಿಭಜಿಸು_VAR                   // V3LinkParse moves to AstVar::attrSplitVar
    };
    // clang-format on
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        // clang-format off
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {
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
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    // True for attributes that read an operand's type rather than its value, such as $bits.
    bool isTypeQuery() ಸ್ಥಿರಾಂಕ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ DIM_ಬಿಟ್ಟುಗಳು:
        ಸ್ಥಿತಿ DIM_ಬಿಟ್ಟುಗಳು_OR_ಸಂಖ್ಯೆ:
        ಸ್ಥಿತಿ DIM_DIMENSIONS:
        ಸ್ಥಿತಿ DIM_ಮೇಲಿನ:
        ಸ್ಥಿತಿ DIM_INCREMENT:
        ಸ್ಥಿತಿ DIM_ಎಡ:
        ಸ್ಥಿತಿ DIM_ಕೆಳಗಿನ:
        ಸ್ಥಿತಿ DIM_ಬಲ:
        ಸ್ಥಿತಿ DIM_ಗಾತ್ರ:
        ಸ್ಥಿತಿ DIM_UNPK_DIMENSIONS:
        ಸ್ಥಿತಿ TYPEID:
        ಸ್ಥಿತಿ TYPENAME: ಹಿಂತಿರುಗಿ true;
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ false;
        }
    }
    VAttrType()
        : m_e{ILLEGAL} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VAttrType(en _e)
        : m_e{_e} {}
    explicit VAttrType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAttrType& lhs, ಸ್ಥಿರಾಂಕ VAttrType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VAttrType& lhs, VAttrType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VAttrType::en lhs, ಸ್ಥಿರಾಂಕ VAttrType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }

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
    ಶೂನ್ಯ combine(ಸ್ಥಿರಾಂಕ VBaseOverride& other) {
        m_extends |= other.m_extends;
        m_final |= other.m_final;
        m_initial |= other.m_initial;
    }
    bool isAny() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_extends | m_final | m_initial; }
    bool isExtends() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_extends; }
    bool isFinal() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_final; }
    bool isInitial() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_initial; }
    string ascii() ಸ್ಥಿರಾಂಕ {
        string ಹೊರಹೋಗು;
        ಆದರೆ (m_initial) ಹೊರಹೋಗು = VString::dot(ಹೊರಹೋಗು, " ", "initial");
        ಆದರೆ (m_extends) ಹೊರಹೋಗು = VString::dot(ಹೊರಹೋಗು, " ", "extends");
        ಆದರೆ (m_final) ಹೊರಹೋಗು = VString::dot(ಹೊರಹೋಗು, " ", "final");
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
    }
};

// ######################################################################

class VBasicDTypeKwd final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        UNKNOWN,
        ಬಿಟ್,
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
        ಸಮಯ,
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
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"%E-unk",
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
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* typeName() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ LOGIC2STATE: ಹಿಂತಿರುಗಿ VBasicDTypeKwd{LOGIC}.ascii();
        ಸ್ಥಿತಿ INTEGER2STATE: ಹಿಂತಿರುಗಿ VBasicDTypeKwd{INTEGER}.ascii();
        ಸ್ಥಿತಿ TIME2STATE: ಹಿಂತಿರುಗಿ VBasicDTypeKwd{ಸಮಯ}.ascii();
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ ascii();
        }
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* dpiType() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"%E-unk",
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
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() {
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.ascii(), " MAX"),
                "SelfTest: Enum mismatch");
        UASSERT(0 == std::strcmp(VBasicDTypeKwd{_ENUM_MAX}.dpiType(), " MAX"),
                "SelfTest: Enum mismatch");
    }
    VBasicDTypeKwd() VL_MT_ಸುರಕ್ಷಿತ : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBasicDTypeKwd(en _e) VL_MT_ಸುರಕ್ಷಿತ : m_e{_e} {}
    explicit VBasicDTypeKwd(ಪೂರ್ಣಾಂಕ _e) VL_MT_ಸುರಕ್ಷಿತ  //
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಪೂರ್ಣಾಂಕ ಅಗಲ() ಸ್ಥಿರಾಂಕ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ ಬಿಟ್: ಹಿಂತಿರುಗಿ 1;  // scalar, can't bit extract unless ranged
        ಸ್ಥಿತಿ BYTE: ಹಿಂತಿರುಗಿ 8;
        ಸ್ಥಿತಿ CHANDLE: ಹಿಂತಿರುಗಿ 64;
        ಸ್ಥಿತಿ NULLTYPE: ಹಿಂತಿರುಗಿ 1;  // Historical - bit was used instead of NULLTYPE
        ಸ್ಥಿತಿ EVENT: ಹಿಂತಿರುಗಿ 1;
        ಸ್ಥಿತಿ INT: ಹಿಂತಿರುಗಿ 32;
        ಸ್ಥಿತಿ INTEGER: ಹಿಂತಿರುಗಿ 32;
        ಸ್ಥಿತಿ INTEGER2STATE: ಹಿಂತಿರುಗಿ 32;
        ಸ್ಥಿತಿ LOGIC: ಹಿಂತಿರುಗಿ 1;  // scalar, can't bit extract unless ranged
        ಸ್ಥಿತಿ LOGIC2STATE: ಹಿಂತಿರುಗಿ 1;  // scalar, can't bit extract unless ranged
        ಸ್ಥಿತಿ LONGINT: ಹಿಂತಿರುಗಿ 64;
        ಸ್ಥಿತಿ DOUBLE: ಹಿಂತಿರುಗಿ 64;  // opaque
        ಸ್ಥಿತಿ SHORTINT: ಹಿಂತಿರುಗಿ 16;
        ಸ್ಥಿತಿ ಸಮಯ: ಹಿಂತಿರುಗಿ 64;
        ಸ್ಥಿತಿ TIME2STATE: ಹಿಂತಿರುಗಿ 64;
        ಸ್ಥಿತಿ STRING: ಹಿಂತಿರುಗಿ 64;  // opaque  // Just the pointer, for today
        ಸ್ಥಿತಿ SCOPEPTR: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ CHARPTR: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ MTASKSTATE: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ DELAY_SCHEDULER: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ TRIGGER_SCHEDULER: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ DYNAMIC_TRIGGER_SCHEDULER: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ FORK_SYNC: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ PROCESS_REFERENCE: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ RANDOM_GENERATOR: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ RANDOM_STDGENERATOR: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ COVERGROUP_INSTHANDLE: ಹಿಂತಿರುಗಿ 0;  // opaque
        ಸ್ಥಿತಿ UINT32: ಹಿಂತಿರುಗಿ 32;
        ಸ್ಥಿತಿ UINT64: ಹಿಂತಿರುಗಿ 64;
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ 0;
        }
    }
    bool isSigned() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
               || m_e == INTEGER2STATE || m_e == DOUBLE;
    }
    bool isUnsigned() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_e == CHANDLE || m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
               || m_e == UINT32 || m_e == UINT64 || m_e == ಬಿಟ್ || m_e == LOGIC
               || m_e == LOGIC2STATE || m_e == ಸಮಯ || m_e == TIME2STATE;
    }
    bool isFourstate() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_e == INTEGER || m_e == LOGIC || m_e == LOGIC_IMPLICIT || m_e == ಸಮಯ;
    }
    bool isZeroInit() ಸ್ಥಿರಾಂಕ {  // Otherwise initializes to X
        ಹಿಂತಿರುಗಿ (m_e == ಬಿಟ್ || m_e == BYTE || m_e == CHANDLE || m_e == NULLTYPE || m_e == EVENT
                || m_e == INT || m_e == LONGINT || m_e == SHORTINT || m_e == STRING
                || m_e == DOUBLE);
    }
    bool isIntNumeric() ಸ್ಥಿರಾಂಕ {  // Enum increment supported
        ಹಿಂತಿರುಗಿ (m_e == ಬಿಟ್ || m_e == BYTE || m_e == INT || m_e == INTEGER || m_e == INTEGER2STATE
                || m_e == LOGIC || m_e == LOGIC2STATE || m_e == LONGINT || m_e == SHORTINT
                || m_e == UINT32 || m_e == UINT64 || m_e == ಸಮಯ || m_e == TIME2STATE);
    }
    bool is_ಬಿಟ್() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == ಬಿಟ್; }
    bool is_ಬಿಟ್_Logic() ಸ್ಥಿರಾಂಕ {  // Bit/logic vector types; can form a packed array
        ಹಿಂತಿರುಗಿ (m_e == LOGIC || m_e == LOGIC2STATE || m_e == ಬಿಟ್);
    }
    bool isDpiUnsignable() ಸ್ಥಿರಾಂಕ {  // Can add "unsigned" to DPI
        ಹಿಂತಿರುಗಿ (m_e == BYTE || m_e == SHORTINT || m_e == INT || m_e == LONGINT || m_e == INTEGER
                || m_e == INTEGER2STATE);
    }
    bool isDpiCLayout() ಸ್ಥಿರಾಂಕ {  // Uses standard C layout, for DPI runtime access
        ಹಿಂತಿರುಗಿ (m_e == ಬಿಟ್ || m_e == BYTE || m_e == CHANDLE || m_e == INT || m_e == LONGINT
                || m_e == DOUBLE || m_e == SHORTINT || m_e == UINT32 || m_e == UINT64);
    }
    bool isOpaque() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {  // IE not a simple number we can bit optimize
        ಹಿಂತಿರುಗಿ (m_e == EVENT || m_e == STRING || m_e == SCOPEPTR || m_e == CHARPTR
                || m_e == MTASKSTATE || m_e == DELAY_SCHEDULER || m_e == TRIGGER_SCHEDULER
                || m_e == DYNAMIC_TRIGGER_SCHEDULER || m_e == FORK_SYNC || m_e == PROCESS_REFERENCE
                || m_e == RANDOM_GENERATOR || m_e == RANDOM_STDGENERATOR
                || m_e == COVERGROUP_INSTHANDLE || m_e == DOUBLE || m_e == UNTYPED);
    }
    bool isCHandle() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == CHANDLE; }
    bool isDouble() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == DOUBLE; }
    bool isEvent() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == EVENT; }
    bool isString() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == STRING; }
    bool isMTaskState() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == MTASKSTATE; }
    bool isSameish(ಸ್ಥಿರಾಂಕ VBasicDTypeKwd& other) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆಯ್ಕೆ (other.m_e) {
        ಸ್ಥಿತಿ LOGIC:
        ಸ್ಥಿತಿ LOGIC2STATE:
        ಸ್ಥಿತಿ LOGIC_IMPLICIT: ಹಿಂತಿರುಗಿ m_e == LOGIC || m_e == LOGIC2STATE || m_e == LOGIC_IMPLICIT;
        ಸ್ಥಿತಿ INTEGER:
        ಸ್ಥಿತಿ INTEGER2STATE: ಹಿಂತಿರುಗಿ m_e == INTEGER || m_e == INTEGER2STATE;
        ಸ್ಥಿತಿ ಸಮಯ:
        ಸ್ಥಿತಿ TIME2STATE: ಹಿಂತಿರುಗಿ m_e == ಸಮಯ || m_e == TIME2STATE;
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ m_e == other.m_e;
        }
    }
    // Does this represent a C++ LiteralType? (can be constexpr)
    bool isLiteralType() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ ಬಿಟ್:
        ಸ್ಥಿತಿ BYTE:
        ಸ್ಥಿತಿ CHANDLE:
        ಸ್ಥಿತಿ INT:
        ಸ್ಥಿತಿ INTEGER:
        ಸ್ಥಿತಿ INTEGER2STATE:
        ಸ್ಥಿತಿ LOGIC:
        ಸ್ಥಿತಿ LOGIC2STATE:
        ಸ್ಥಿತಿ LONGINT:
        ಸ್ಥಿತಿ DOUBLE:
        ಸ್ಥಿತಿ SHORTINT:
        ಸ್ಥಿತಿ SCOPEPTR:
        ಸ್ಥಿತಿ CHARPTR:
        ಸ್ಥಿತಿ UINT32:
        ಸ್ಥಿತಿ UINT64: ಹಿಂತಿರುಗಿ true;
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ false;
        }
    }

    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* traceSigType() ಸ್ಥಿರಾಂಕ {
        // VerilatedTraceSigType to used in trace signal declaration
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ lut[] = {
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
        static_assert(ಗಾತ್ರ(lut) / ಗಾತ್ರ(lut[0]) == _ENUM_MAX, "Missing trace signal type");
        ಹಿಂತಿರುಗಿ lut[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VBasicDTypeKwd& lhs, ಸ್ಥಿರಾಂಕ VBasicDTypeKwd& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VBasicDTypeKwd& lhs, VBasicDTypeKwd::en rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VBasicDTypeKwd::en lhs, ಸ್ಥಿರಾಂಕ VBasicDTypeKwd& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
}

// ######################################################################

/// Boolean or unknown
class VBoolOrUnknown final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { BU_FALSE = 0, BU_TRUE = 1, BU_UNKNOWN = 2, ENUM_ಕೊನೆ };
    ಎಣಿಕೆ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBoolOrUnknown()
        : m_e{BU_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBoolOrUnknown(en _e)
        : m_e{_e} {}
    explicit VBoolOrUnknown(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"FALSE", "TRUE", "UNK"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    bool is_ತಿಳಿದ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e != BU_UNKNOWN; }
    bool true_ತಿಳಿದ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == BU_TRUE; }
    ಶೂನ್ಯ ಗಣ_True_Or_False(bool flag) { m_e = flag ? BU_TRUE : BU_FALSE; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VBoolOrUnknown& lhs, ಸ್ಥಿರಾಂಕ VBoolOrUnknown& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VBoolOrUnknown& lhs, VBoolOrUnknown::en rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VBoolOrUnknown::en lhs, ಸ್ಥಿರಾಂಕ VBoolOrUnknown& rhs) {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
}
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VBoolOrUnknown& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

class VBranchPred final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { BP_UNKNOWN = 0, BP_LIKELY, BP_UNLIKELY, ENUM_ಕೊನೆ };
    ಎಣಿಕೆ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VBranchPred()
        : m_e{BP_UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VBranchPred(en _e)
        : m_e{_e} {}
    explicit VBranchPred(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    bool unknown() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == BP_UNKNOWN; }
    bool likely() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == BP_LIKELY; }
    bool unlikely() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == BP_UNLIKELY; }
    VBranchPred invert() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_e == BP_UNLIKELY) ಹಿಂತಿರುಗಿ BP_LIKELY;
        ಆದರೆ (m_e == BP_LIKELY) ಹಿಂತಿರುಗಿ BP_UNLIKELY;
        ಹಿಂತಿರುಗಿ m_e;
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"", "VL_LIKELY", "VL_UNLIKELY"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* asciiShort() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"", "L", "!L"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VBranchPred& lhs, ಸ್ಥಿರಾಂಕ VBranchPred& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VBranchPred& lhs, VBranchPred::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VBranchPred::en lhs, ಸ್ಥಿರಾಂಕ VBranchPred& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VBranchPred& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

namespace {
// Validate the arguments descriptor
// LCOV_EXCL_START
ಸ್ಥಿರ constexpr bool validate_ವಾದಗಳು_Descriptor(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descrp) {
    // Is "TODO"
    ಆದರೆ (descrp[0] == 'T' && descrp[1] == 'O' && descrp[2] == 'D' && descrp[3] == 'O'
        && !descrp[4]) {
        ಹಿಂತಿರುಗಿ true;
    }
    // Is a sequence of 'r'/'w'/'m' with an optional trailing '+'
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = descrp; *cp; ++cp) {
        ಆದರೆ (*cp == '+') ಹಿಂತಿರುಗಿ cp != descrp && !cp[1];
        ಆದರೆ (*cp != 'r' && *cp != 'w' && *cp != 'm') ಹಿಂತಿರುಗಿ false;
    }
    ಹಿಂತಿರುಗಿ true;
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
#ವ್ಯಾಖ್ಯಾನಿಸು FOR_EACH_CMETHOD(macro) \
    /*    id,                                 method,                   pure,   clean,  args */ \
    macro(_NONE,                              "_none",                  false,  CLEAN,  "") \
    macro(ARRAY_AT,                           "at",                     ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(ARRAY_AT_ಹಿಂಭಾಗ,                      "atBack",                 ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(ARRAY_AT_WRITE,                     "atWrite",                ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(ARRAY_ಕಂಡುಹಿಡಿ,                         "find",                   ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_ಕಂಡುಹಿಡಿ_ಮೊದಲ,                   "find_first",             ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_ಕಂಡುಹಿಡಿ_ಮೊದಲ_ಸೂಚ್ಯಂಕ,             "find_first_index",       ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_ಕಂಡುಹಿಡಿ_ಸೂಚ್ಯಂಕ,                   "find_index",             ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_ಕಂಡುಹಿಡಿ_ಕೊನೆಯ,                    "find_last",              ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_ಕಂಡುಹಿಡಿ_ಕೊನೆಯ_ಸೂಚ್ಯಂಕ,              "find_last_index",        ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_ಮೊದಲ,                        "first",                  false,  CLEAN,  "m") \
    macro(ARRAY_INSIDE,                       "inside",                 ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(ARRAY_ಕೊನೆಯ,                         "last",                   false,  CLEAN,  "m") \
    macro(ARRAY_MAP,                          "map",                    ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_MAX,                          "max",                    ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_MIN,                          "min",                    ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_ಮುಂದಿನ,                         "next",                   false,  CLEAN,  "m") \
    macro(ARRAY_ತೆಗೆ_ಹಿಂಭಾಗ,                     "pop_back",               false,  CLEAN,  "") \
    macro(ARRAY_ತೆಗೆ_FRONT,                    "pop_front",              false,  CLEAN,  "") \
    macro(ARRAY_PREV,                         "prev",                   false,  CLEAN,  "m") \
    macro(ARRAY_ತಳ್ಳು_ಹಿಂಭಾಗ,                    "push_back",              false,  CLEAN,  "r") \
    macro(ARRAY_ತಳ್ಳು_FRONT,                   "push_front",             false,  CLEAN,  "r") \
    macro(ARRAY_ಹಿಮ್ಮುಖ,                      "reverse",                false,  CLEAN,  "") \
    macro(ARRAY_RSORT,                        "rsort",                  false,  CLEAN,  "") \
    macro(ARRAY_R_AND,                        "r_and",                  ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_R_OR,                         "r_or",                   ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_R_ಗುಣಲಬ್ಧ,                    "r_product",              ಶುದ್ಧ_2,   false,  "") \
    macro(ARRAY_R_ಮೊತ್ತ,                        "r_sum",                  ಶುದ್ಧ_2,   false,  "") \
    macro(ARRAY_R_XOR,                        "r_xor",                  ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_SHUFFLE,                      "shuffle",                false,  CLEAN,  "") \
    macro(ARRAY_ತುಂಡು,                        "slice",                  ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(ARRAY_ವಿಂಗಡಿಸು,                         "sort",                   false,  CLEAN,  "") \
    macro(ARRAY_UNIQUE,                       "unique",                 ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ARRAY_UNIQUE_ಸೂಚ್ಯಂಕ,                 "unique_index",           ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(ASSOC_CLEAR,                        "clear",                  false,  CLEAN,  "") \
    macro(ASSOC_ERASE,                        "erase",                  false,  CLEAN,  "r") \
    macro(ASSOC_EXISTS,                       "exists",                 ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(ASSOC_ಮೊದಲ,                        "first",                  false,  CLEAN,  "m") \
    macro(ASSOC_ಮುಂದಿನ,                         "next",                   false,  CLEAN,  "m") \
    macro(ASSOC_ಗಾತ್ರ,                         "size",                   ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(CLASS_ಗಣ_RANDMODE,                 "set_randmode",           false,  CLEAN,  "r") \
    macro(COVERGROUP_ಕೂಡಿಸು_ARRAY_NAMER,         "addArrayNamer",          false,  CLEAN,  "r+") \
    macro(COVERGROUP_ಕೂಡಿಸು_BIN,                 "addBin",                 false,  CLEAN,  "r+") \
    macro(COVERGROUP_ಕೂಡಿಸು_COVERPOINT,          "addCoverpoint",          false,  CLEAN,  "") \
    macro(COVERGROUP_ಕೂಡಿಸು_CROSS,               "addCross",               false,  CLEAN,  "") \
    macro(COVERGROUP_ಕೂಡಿಸು_CROSS_DYN,           "addCrossDyn",            false,  CLEAN,  "") \
    macro(COVERGROUP_ಕೂಡಿಸು_NUMBERED_NAMER,      "addNumberedNamer",       false,  CLEAN,  "r+") \
    macro(COVERGROUP_ಕೂಡಿಸು_SINGLE_NAMER,        "addSingleNamer",         false,  CLEAN,  "r+") \
    macro(COVERGROUP_ATTACH,                  "attach",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_CLEAR_ದೊರಕಿತು_LIST,          "clearHitList",           false,  CLEAN,  "") \
    macro(COVERGROUP_COVERAGE,                "coverage",               false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_BEGIN,        "withBegin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_CROSS_WITH_ಕೊನೆ,          "withEnd",                false,  CLEAN,  "") \
    macro(COVERGROUP_CROSS_WITH_TEST,         "withTest",               false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_ಮೌಲ್ಯ,        "withValue",              false,  CLEAN,  "r") \
    macro(COVERGROUP_CROSS_WITH_ಮೌಲ್ಯ_W,      "withValueW",             false,  CLEAN,  "rw") \
    macro(COVERGROUP_FINALIZE_BINS,           "finalizeBins",           false,  CLEAN,  "") \
    macro(COVERGROUP_INCREMENT_BIN,           "incrementBin",           false,  CLEAN,  "r") \
    macro(COVERGROUP_INIT,                    "init",                   false,  CLEAN,  "r+") \
    macro(COVERGROUP_INST_P,                  "p",                      ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(COVERGROUP_LEND_ತೂಕ,             "lendWeight",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_ದಾಖಲೆ_ದೊರಕಿತು,              "recordHit",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ರಿಜಿಸ್ಟರ್_BINS,           "registerBins",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_SAMPLE,                  "sample",                 false,  CLEAN,  "") \
    macro(COVERGROUP_SAMPLE_IFFS,             "sample",                 false,  CLEAN,  "r") \
    macro(COVERGROUP_SELECT_ALL,              "selectAll",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_AND,              "selectAnd",              false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_BIN,              "selectBin",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM,              "selectDim",              false,  CLEAN,  "r+") \
    macro(COVERGROUP_SELECT_DIM_ಕೊನೆ,          "selectDimEnd",           false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_OR,               "selectOr",               false,  CLEAN,  "") \
    macro(COVERGROUP_SELECT_RANGE,            "selectRange",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SELECT_RANGE_W,          "selectRangeW",           false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_ಕೊನೆ,               "sizedEnd",               ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_ಮುಕ್ತಾಯ,            "sizedFinish",            false,  CLEAN,  "r+") \
    macro(COVERGROUP_SIZED_ಮೊದಲ,             "sizedFirst",             ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(COVERGROUP_SIZED_RANGE,             "sizedRange",             false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_RANGE_W,           "sizedRangeW",            false,  CLEAN,  "rr") \
    macro(COVERGROUP_SIZED_SAMPLE,            "sizedSample",            false,  CLEAN,  "rrr") \
    macro(COVERGROUP_SIZED_SAMPLE_W,          "sizedSampleW",           false,  CLEAN,  "rrr") \
    macro(COVERGROUP_TYPE_COVERAGE,           "typeCoverage",           false,  CLEAN,  "rrrr") \
    macro(COVERGROUP_TYPE_ತೂಕ,             "typeWeight",             false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_EXCLUDED,          "valueExcluded",          ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_EXCLUDED_W,        "valueExcludedW",         ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_FINALIZE,          "valueFinalize",          false,  CLEAN,  "") \
    macro(COVERGROUP_ಮೌಲ್ಯ_PATTERNS,          "valuePatterns",          false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_RANGES,            "valueRanges",            false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_RELEASE,           "valueRelease",           false,  CLEAN,  "") \
    macro(COVERGROUP_ಮೌಲ್ಯ_RUNS,              "valueRuns",              false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_TRANSITIONS,       "valueTransitions",       false,  CLEAN,  "r") \
    macro(COVERGROUP_ಮೌಲ್ಯ_TYPE,              "valueType",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_ತೂಕ,                  "weight",                 false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_BEGIN,              "withBegin",              false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_ಮುಕ್ತಾಯ,             "withFinish",             false,  CLEAN,  "r+") \
    macro(COVERGROUP_WITH_HI,                 "withHi",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_HI_W,               "withHiW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_LO,                 "withLo",                 false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_LO_W,               "withLoW",                false,  CLEAN,  "w") \
    macro(COVERGROUP_WITH_ಮುಂದಿನ,               "withNext",               false,  CLEAN,  "") \
    macro(COVERGROUP_WITH_ಸತತ,                "withRun",                false,  CLEAN,  "rr") \
    macro(COVERGROUP_WITH_ಸತತ_W,              "withRunW",               false,  CLEAN,  "rr") \
    macro(DYN_AT_WRITE_APPEND,                "atWriteAppend",          false,  CLEAN,  "r") \
    macro(DYN_AT_WRITE_APPEND_ಹಿಂಭಾಗ,           "atWriteAppendBack",      false,  CLEAN,  "r") \
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
    macro(DYN_ಗಾತ್ರ,                           "size",                   ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(DYN_ತುಂಡು,                          "slice",                  ಶುದ್ಧ_2,   CLEAN,  "rr") \
    macro(DYN_ತುಂಡು_ನಿಯೋಜಿಸು,                   "sliceAssign",            false,  CLEAN,  "rrr") \
    macro(DYN_ತುಂಡು_ನಿಯೋಜಿಸು_ಹಿಂಭಾಗ_ಹಿಂಭಾಗ,         "sliceAssignBackBack",    false,  CLEAN,  "rrr") \
    macro(DYN_ತುಂಡು_ನಿಯೋಜಿಸು_FRONT_ಹಿಂಭಾಗ,        "sliceAssignFrontBack",   false,  CLEAN,  "rrr") \
    macro(DYN_ತುಂಡು_ಹಿಂಭಾಗ_ಹಿಂಭಾಗ,                "sliceBackBack",          ಶುದ್ಧ_2,   CLEAN,  "rr") \
    macro(DYN_ತುಂಡು_FRONT_ಹಿಂಭಾಗ,               "sliceFrontBack",         ಶುದ್ಧ_2,   CLEAN,  "rr") \
    macro(EVENT_CLEAR_FIRED,                  "clearFired",             false,  CLEAN,  "") \
    macro(EVENT_CLEAR_TRIGGERED,              "clearTriggered",         false,  CLEAN,  "") \
    macro(EVENT_FIRE,                         "fire",                   false,  CLEAN,  "") \
    macro(EVENT_IS_FIRED,                     "isFired",                ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(EVENT_IS_TRIGGERED,                 "isTriggered",            ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(FORCE_ಕೂಡಿಸು,                          "addForce",               false,  CLEAN,  "rrrr+") \
    macro(FORCE_READ,                         "read",                   ಶುದ್ಧ_2,   CLEAN,  "r") \
    macro(FORCE_READ_ಸೂಚ್ಯಂಕ,                   "readIndex",              ಶುದ್ಧ_2,   CLEAN,  "rr") \
    macro(FORCE_READ_SEL,                     "readSel",                ಶುದ್ಧ_2,   CLEAN,  "TODO") \
    macro(FORCE_RELEASE,                      "release",                false,  CLEAN,  "rr+") \
    macro(FORCE_TOUCH,                        "touch",                  false,  CLEAN,  "") \
    macro(FORK_ಮುಗಿದ,                          "done",                   false,  CLEAN,  "rr") \
    macro(FORK_INIT,                          "init",                   false,  CLEAN,  "rr") \
    macro(FORK_JOIN,                          "join",                   false,  CLEAN,  "rrr") \
    macro(FORK_ON_KILL,                       "onKill",                 false,  CLEAN,  "r") \
    macro(NBA_COMMIT,                         "commit",                 false,  CLEAN,  "w") \
    macro(NBA_ಸರತಿಗೆ_ಸೇರಿಸು,                        "enqueue",                false,  CLEAN,  "r+") \
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
    macro(RANDOMIZER_ಗಣ_VAR_DISABLED,        "set_var_disabled",       false,  CLEAN,  "r") \
    macro(RANDOMIZER_CLEAR_VAR_DISABLED,      "clear_var_disabled",     false,  CLEAN,  "r") \
    macro(RANDOMIZER_MARK_VAR_STATIC,         "mark_var_static",        false,  CLEAN,  "r") \
    macro(RANDOMIZER_ಗಣ_STATIC_RANDMODE,     "set_static_randmode",    false,  CLEAN,  "r") \
    macro(RNG_GET_RANDSTATE,                  "__Vm_rng.get_randstate", ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(RNG_ಗಣ_RANDSTATE,                  "__Vm_rng.set_randstate", false,  CLEAN,  "r") \
    macro(SCHED_ANY_TRIGGERED,                "anyTriggered",           false,  CLEAN,  "r") \
    macro(SCHED_AWAITING_CURRENT_ಸಮಯ,        "awaitingCurrentTime",    ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(SCHED_AWAITING_ZERO_DELAY,          "awaitingZeroDelay",      ಶುದ್ಧ_2,   CLEAN,  "") \
    macro(SCHED_READY,                        "ready",                  false,  CLEAN,  "r") \
    macro(SCHED_MOVE_ಗಮ್ಯ_RESUME_ಸರತಿ,         "moveToResumeQueue",      false,  CLEAN,  "r") \
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
    macro(UNPACKED_ನಿಯೋಜಿಸು,                    "assign",                 false,  CLEAN,  "r") \
    macro(UNPACKED_FILL,                      "fill",                   false,  CLEAN,  "r") \
    macro(UNPACKED_NEQ,                       "neq",                    ಶುದ್ಧ_2,   CLEAN,  "r")
// clang-format on

class VCMethod final {
    ಸ್ಥಿರ constexpr bool ಶುದ್ಧ_2 = true;  // For macro expansion of 'pure' field only
    ಸ್ಥಿರ constexpr bool CLEAN = true;  // For macro expansion of 'clean' field only

ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CMETHOD_ID(id, ವಿಧಾನ, ಶುದ್ಧ_3, clean, ವಾದಗಳು) id,
        FOR_EACH_CMETHOD(VL_CMETHOD_ID)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CMETHOD_ID
            _ENUM_MAX  // Leave last
    };
    ಎಣಿಕೆ en m_e;
    VCMethod()
        : m_e{_NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCMethod(en _e)
        : m_e{_e} {}
    explicit VCMethod(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CMETHOD_NAME(id, ವಿಧಾನ, ಶುದ್ಧ_3, clean, ವಾದಗಳು) ವಿಧಾನ,
            FOR_EACH_CMETHOD(VL_CMETHOD_NAME)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CMETHOD_NAME
                "_ENUM_MAX"  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool is_ಶುದ್ಧ() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CMETHOD_ಶುದ್ಧ(id, ವಿಧಾನ, ಶುದ್ಧ_3, clean, ವಾದಗಳು) ಶುದ್ಧ_3,
            FOR_EACH_CMETHOD(VL_CMETHOD_ಶುದ್ಧ)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CMETHOD_ಶುದ್ಧ
                false  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool isClean() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CMETHOD_CLEAN(id, ವಿಧಾನ, ಶುದ್ಧ_3, clean, ವಾದಗಳು) clean,
            FOR_EACH_CMETHOD(VL_CMETHOD_CLEAN)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CMETHOD_CLEAN
                false  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ವಾದಗಳು() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CMETHOD_ವಾದಗಳು(id, ವಿಧಾನ, ಶುದ್ಧ_3, clean, ವಾದಗಳು) ವಾದಗಳು,
            FOR_EACH_CMETHOD(VL_CMETHOD_ವಾದಗಳು)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CMETHOD_ವಾದಗಳು
                ""  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    // Return array method for given name
    ಸ್ಥಿರ VCMethod array_ವಿಧಾನ(ಸ್ಥಿರಾಂಕ string& name);
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCMethod& lhs, ಸ್ಥಿರಾಂಕ VCMethod& rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCMethod& lhs, VCMethod::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VCMethod::en lhs, ಸ್ಥಿರಾಂಕ VCMethod& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VCMethod& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// Static assert all argument descriptors are well formed
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CMETHOD_ವಾದಗಳು_CHECK(id, ವಿಧಾನ, ಶುದ್ಧ_3, clean, ವಾದಗಳು) \
    static_assert(validate_ವಾದಗಳು_Descriptor(ವಾದಗಳು), "Malformed argument descriptor for " #id);
FOR_EACH_CMETHOD(VL_CMETHOD_ವಾದಗಳು_CHECK)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CMETHOD_ವಾದಗಳು_CHECK

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು FOR_EACH_CMETHOD

// clang-format off
#ವ್ಯಾಖ್ಯಾನಿಸು FOR_EACH_CFUNCTION(macro) \
    /*    id,                                 function,                 pure,   args */ \
    macro(_NONE,                              "_none",                  false,  "") \
    macro(RANDOMIZER_ಗಮ್ಯ_SOLVER_HEX,           "vlToSolverHex",          ಶುದ್ಧ_2,   "r")
// clang-format on

class VCFunction final {
    ಸ್ಥಿರ constexpr bool ಶುದ್ಧ_2 = true;  // For macro expansion of 'pure' field only

ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CFUNCTION_ID(id, ವಿಧಾನ, ಶುದ್ಧ_3, ವಾದಗಳು) id,
        FOR_EACH_CFUNCTION(VL_CFUNCTION_ID)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CFUNCTION_ID
            _ENUM_MAX  // Leave last
    };
    ಎಣಿಕೆ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCFunction(en _e)
        : m_e{_e} {}
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CFUNCTION_NAME(id, function, ಶುದ್ಧ_3, ವಾದಗಳು) function,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_NAME)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CFUNCTION_NAME
                "_ENUM_MAX"  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool is_ಶುದ್ಧ() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ bool ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CFUNCTION_ಶುದ್ಧ(id, ವಿಧಾನ, ಶುದ್ಧ_3, ವಾದಗಳು) ಶುದ್ಧ_3,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ಶುದ್ಧ)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CFUNCTION_ಶುದ್ಧ
                false  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ವಾದಗಳು() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CFUNCTION_ವಾದಗಳು(id, ವಿಧಾನ, ಶುದ್ಧ_3, ವಾದಗಳು) ವಾದಗಳು,
            FOR_EACH_CFUNCTION(VL_CFUNCTION_ವಾದಗಳು)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CFUNCTION_ವಾದಗಳು
                ""  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
};

// Static assert all argument descriptors are well formed
#ವ್ಯಾಖ್ಯಾನಿಸು VL_CFUNCTION_ವಾದಗಳು_CHECK(id, ವಿಧಾನ, ಶುದ್ಧ_3, ವಾದಗಳು) \
    static_assert(validate_ವಾದಗಳು_Descriptor(ವಾದಗಳು), "Malformed argument descriptor for " #id);
FOR_EACH_CFUNCTION(VL_CFUNCTION_ವಾದಗಳು_CHECK)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_CFUNCTION_ವಾದಗಳು_CHECK

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು FOR_EACH_CFUNCTION

// ######################################################################

class VCStmtType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        NONE,  // Unknown or not applicable
        CTOR_VAR_RESET_CALL,
        _ENUM_MAX  // Leave last
    };

ಖಾಸಗಿ:
    ರಚನೆ ಅಂಶ final {
        ಎಣಿಕೆ en m_e;  // Statement's enum mnemonic, for checking
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* m_name;  // Statements name, for debugging
    };
    ಸ್ಥಿರ ಅಂಶ s_ಅಂಶ_ದತ್ತಾಂಶ[];

ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en m_e;
    VCStmtType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCStmtType(en _e)
        : m_e{_e} {}
    explicit VCStmtType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"none", "ctor_var_reset_call"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    bool isNone() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == NONE; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCStmtType& lhs, ಸ್ಥಿರಾಂಕ VCStmtType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCStmtType& lhs, VCStmtType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VCStmtType::en lhs, ಸ್ಥಿರಾಂಕ VCStmtType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VCStmtType& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

class VCaseType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        CT_CASE,
        CT_CASEX,
        CT_CASEZ,
        CT_CASEINSIDE,
        CT_CASEMATCHES,
        CT_RANDSEQUENCE
    };
    ಎಣಿಕೆ en m_e;
    VCaseType()
        : m_e{CT_CASE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCaseType(en _e)
        : m_e{_e} {}
    explicit VCaseType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"CASE", "CASEX", "CASEZ", "CASEINSIDE", "CASEMATCHES", "RANDSEQUENCE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCaseType& lhs, ಸ್ಥಿರಾಂಕ VCaseType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCaseType& lhs, VCaseType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VCaseType::en lhs, ಸ್ಥಿರಾಂಕ VCaseType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }

// ######################################################################

class VCastable final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        UNSUPPORTED,
        SAMEISH,
        COMPATIBLE,
        ENUM_IMPLICIT,
        ENUM_EXPLICIT,
        DYNAMIC_CLASS,
        INCOMPATIBLE,
        _ENUM_MAX  // Leave last
    };
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"UNSUPPORTED",   "SAMEISH",       "COMPATIBLE",  "ENUM_IMPLICIT",
               "ENUM_EXPLICIT", "DYNAMIC_CLASS", "INCOMPATIBLE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    bool isAssignable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e != UNSUPPORTED && m_e != INCOMPATIBLE; }
    VCastable()
        : m_e{UNSUPPORTED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VCastable(en _e)
        : m_e{_e} {}
    explicit VCastable(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCastable& lhs, ಸ್ಥಿರಾಂಕ VCastable& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCastable& lhs, VCastable::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VCastable::en lhs, ಸ್ಥಿರಾಂಕ VCastable& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VCastable& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

//######################################################################

class VCoverBinsType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
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
    ಎಣಿಕೆ en m_e;
    VCoverBinsType()  // LCOV_EXCL_START
        : m_e{BINS_USER} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverBinsType(en _e)
        : m_e{_e} {}
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }  // LCOV_EXCL_LINE
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"array",        "auto",       "auto_implicit", "default", "ignore_bins",
               "illegal_bins", "transition", "bins",          "wildcard"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    // Keyword declaring the bins, without any 'wildcard' prefix
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* verilogKwd() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {
            "bins", "bins", "bins", "bins", "ignore_bins", "illegal_bins", "bins", "bins", "bins"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    // VlCovBinKind enumerator naming the bin's set
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* bin_ಗಣ_Enum() ಸ್ಥಿರಾಂಕ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ BINS_IGNORE: ಹಿಂತಿರುಗಿ "VlCovBinKind::KIND_IGNORE";
        ಸ್ಥಿತಿ BINS_ILLEGAL: ಹಿಂತಿರುಗಿ "VlCovBinKind::KIND_ILLEGAL";
        ಸ್ಥಿತಿ BINS_DEFAULT: ಹಿಂತಿರುಗಿ "VlCovBinKind::KIND_DEFAULT";
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "VlCovBinKind::KIND_NORMAL";
        }
    }
    // Normal bins (feed coverage) are anything but ignore/illegal/default
    bool bin_Is_ಸಾಮಾನ್ಯ() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_e != BINS_IGNORE && m_e != BINS_ILLEGAL && m_e != BINS_DEFAULT;
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCoverBinsType& lhs, VCoverBinsType::en rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}

//######################################################################

class VCoverOptionType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        // Shared by option.* and type_option.*
        ತೂಕ,
        GOAL,
        AT_ಕನಿಷ್ಠ,
        AUTO_BIN_MAX,
        ಪ್ರತಿ_INSTANCE,
        COMMENT,
        // option.* only (IEEE 1800-2023 Table 19-1)
        NAME,
        CROSS_ಅಂಕೆ_PRINT_MISSING,
        CROSS_RETAIN_AUTO_BINS,
        DETECT_OVERLAP,
        GET_INST_COVERAGE,
        // type_option.* only (IEEE 1800-2023 Table 19-3)
        STROBE,
        ವಿಲೀನ_INSTANCES,
        DISTRIBUTE_ಮೊದಲ,
        REAL_INTERVAL,
        // Legacy option.* accepted for compatibility
        CROSS_AUTO_BIN_MAX,
        // sentinel - should never appear after parse-time validation
        UNKNOWN
    };
    ಎಣಿಕೆ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VCoverOptionType(en _e)
        : m_e{_e} {}
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"weight",
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
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VCoverOptionType& lhs, VCoverOptionType::en rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}

//######################################################################

class VTransRepType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        NONE,  // No repetition
        CONSEC,  // Consecutive repetition [*]
        GOTO,  // Goto repetition [->]
        NONCONS  // Nonconsecutive repetition [=]
    };
    ಎಣಿಕೆ en m_e;
    VTransRepType()  // LCOV_EXCL_START
        : m_e{NONE} {}  // LCOV_EXCL_STOP
    // cppcheck-suppress noExplicitConstructor
    constexpr VTransRepType(en _e)
        : m_e{_e} {}
    explicit VTransRepType(ಪೂರ್ಣಾಂಕ _e)  // LCOV_EXCL_START
        : m_e(static_cast<en>(_e)) {}  // LCOV_EXCL_STOP  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }  // LCOV_EXCL_LINE
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"", "[*]", "[->]", "[=]"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* asciiJson() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"\"none\"", "\"consec\"", "\"goto\"", "\"noncons\""};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};

//######################################################################

class VDirection final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { NONE, ಒಳಹರಿವು, OUTPUT, INOUT, REF, CONSTREF };
    ಎಣಿಕೆ en m_e;
    VDirection()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDirection(en _e)
        : m_e{_e} {}
    explicit VDirection(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"NONE", "INPUT", "OUTPUT", "INOUT", "REF", "CONSTREF"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    string verilogKwd() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"", "input", "output", "inout", "ref", "const ref"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    string prettyName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ verilogKwd(); }
    bool isAny() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e != NONE; }
    bool isInout() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == INOUT; }
    bool isInoutOrRef() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == INOUT || m_e == REF || m_e == CONSTREF; }
    bool is_ಒಳಹರಿವು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == ಒಳಹರಿವು; }
    bool isNonOutput() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_e == ಒಳಹರಿವು || m_e == INOUT || m_e == REF || m_e == CONSTREF;
    }
    bool isOutput() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == OUTPUT; }
    bool isReadOnly() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == ಒಳಹರಿವು || m_e == CONSTREF; }
    bool isWritable() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == OUTPUT || m_e == INOUT || m_e == REF; }
    bool isRef() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == REF; }
    bool isConstRef() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == CONSTREF; }
    string traceSigDirection() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (isInout()) {
            ಹಿಂತಿರುಗಿ "VerilatedTraceSigDirection::INOUT";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isWritable()) {
            ಹಿಂತಿರುಗಿ "VerilatedTraceSigDirection::OUTPUT";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isNonOutput()) {
            ಹಿಂತಿರುಗಿ "VerilatedTraceSigDirection::INPUT";
        }
        ಹಿಂತಿರುಗಿ "VerilatedTraceSigDirection::NONE";
    }
    VAccess pinAccess() ಸ್ಥಿರಾಂಕ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ NONE: ಹಿಂತಿರುಗಿ VAccess::NOCHANGE;
        ಸ್ಥಿತಿ ಒಳಹರಿವು:
        ಸ್ಥಿತಿ CONSTREF: ಹಿಂತಿರುಗಿ VAccess::READ;
        ಸ್ಥಿತಿ OUTPUT: ಹಿಂತಿರುಗಿ VAccess::WRITE;
        ಸ್ಥಿತಿ INOUT:
        ಸ್ಥಿತಿ REF: ಹಿಂತಿರುಗಿ VAccess::READWRITE;
        }
        v3fatalSrc("Unhandled VDirection");
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VDirection& lhs, ಸ್ಥಿರಾಂಕ VDirection& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VDirection& lhs, VDirection::en rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VDirection::en lhs, ಸ್ಥಿರಾಂಕ VDirection& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
}
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VDirection& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

class VDisplayType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        DT_DISPLAY,
        DT_WRITE,
        DT_MONITOR,
        DT_STROBE,
        DT_INFO,
        DT_ERROR,
        DT_WARNING,
        DT_FATAL
    };
    ಎಣಿಕೆ en m_e;
    VDisplayType()
        : m_e{DT_DISPLAY} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDisplayType(en _e)
        : m_e{_e} {}
    explicit VDisplayType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    bool ಕೂಡಿಸು_Newline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e != DT_WRITE; }
    bool needScopeTracking() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e != DT_DISPLAY && m_e != DT_WRITE; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"display", "write", "monitor", "strobe", "info", "error", "warning", "fatal"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VDisplayType& lhs, ಸ್ಥಿರಾಂಕ VDisplayType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VDisplayType& lhs, VDisplayType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VDisplayType::en lhs, ಸ್ಥಿರಾಂಕ VDisplayType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }

// ######################################################################

class VDumpCtlType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { FILE, VARS, ALL, FLUSH, ಎಲ್ಲೆ_2, OFF, ON };
    ಎಣಿಕೆ en m_e;
    VDumpCtlType()
        : m_e{ON} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VDumpCtlType(en _e)
        : m_e{_e} {}
    explicit VDumpCtlType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"$dumpfile",  "$dumpvars", "$dumpall", "$dumpflush",
               "$dumplimit", "$dumpoff",  "$dumpon"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VDumpCtlType& lhs, ಸ್ಥಿರಾಂಕ VDumpCtlType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VDumpCtlType& lhs, VDumpCtlType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VDumpCtlType::en lhs, ಸ್ಥಿರಾಂಕ VDumpCtlType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }

// ######################################################################

class VEdgeType final {
ಸಾರ್ವಜನಿಕ:
    // REMEMBER to edit the strings below too
    ಎಣಿಕೆ en : uint8_t {
        // These must be in general -> most specific order, as we sort by it
        // in V3Const::visit AstSenTree
        // Involving a variable
        ET_ಬದಲಾಯಿತು,  // Value changed
        ET_BOTHEDGE,  // POSEDGE | NEGEDGE (i.e.: 'edge' in Verilog)
        ET_POSEDGE,
        ET_NEGEDGE,
        ET_EVENT,  // VlEventBase::isFired
        // Involving an expression
        ET_TRUE,
        ET_INITIAL_NBA,  // Event that is fired initially and never again
        //
        ET_COMBO,  // Sensitive to all combo inputs to this block
        ET_COMBO_ನಕ್ಷತ್ರ,  // Sensitive to all combo inputs to this block (from .*)
        ET_HYBRID,  // This is like ET_COMB, but with explicit sensitivity to an expression
        ET_STATIC,  // static variable initializers (runs before 'initial')
        ET_INITIAL,  // 'initial' statements
        ET_FINAL,  // 'final' statements
        ET_NEVER  // Never occurs (optimized away)
    };
    ಎಣಿಕೆ en m_e;
    bool clockedStmt() ಸ್ಥಿರಾಂಕ {
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
        ಹಿಂತಿರುಗಿ clocked[m_e];
    }
    bool an_ಅಂಚು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == ET_BOTHEDGE || m_e == ET_POSEDGE || m_e == ET_NEGEDGE; }
    VEdgeType invert() ಸ್ಥಿರಾಂಕ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ ET_ಬದಲಾಯಿತು: ಹಿಂತಿರುಗಿ ET_ಬದಲಾಯಿತು;
        ಸ್ಥಿತಿ ET_BOTHEDGE: ಹಿಂತಿರುಗಿ ET_BOTHEDGE;
        ಸ್ಥಿತಿ ET_POSEDGE: ಹಿಂತಿರುಗಿ ET_NEGEDGE;
        ಸ್ಥಿತಿ ET_NEGEDGE: ಹಿಂತಿರುಗಿ ET_POSEDGE;
        ಪೂರ್ವನಿಯೋಜಿತ: UASSERT_STATIC(0, "Inverting bad edgeType()"); ಹಿಂತಿರುಗಿ ET_NEGEDGE;
        }
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"CHANGED", "BOTH",       "POS",    "NEG",    "EVENT",   "TRUE",  "ET_INITIAL_NBA",
               "COMBO",   "COMBO_STAR", "HYBRID", "STATIC", "INITIAL", "FINAL", "NEVER"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* verilogKwd() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {
            "[changed]", "edge", "posedge",  "negedge",  "[event]",   "[true]",  "[initial_nba]",
            "*",         "*",    "[hybrid]", "[static]", "[initial]", "[final]", "[never]"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    // Return true iff this and the other have mutually exclusive transitions
    bool exclusive_ಅಂಚು(ಸ್ಥಿರಾಂಕ VEdgeType& other) ಸ್ಥಿರಾಂಕ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ VEdgeType::ET_POSEDGE:
            ಆದರೆ (other.m_e == VEdgeType::ET_NEGEDGE) ಹಿಂತಿರುಗಿ true;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VEdgeType::ET_NEGEDGE:
            ಆದರೆ (other.m_e == VEdgeType::ET_POSEDGE) ಹಿಂತಿರುಗಿ true;
            ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: ನಿಲ್ಲಿಸು;
        }
        ಹಿಂತಿರುಗಿ false;
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VEdgeType(en _e)
        : m_e{_e} {}
    explicit VEdgeType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VEdgeType& lhs, ಸ್ಥಿರಾಂಕ VEdgeType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VEdgeType& lhs, VEdgeType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VEdgeType::en lhs, ಸ್ಥಿರಾಂಕ VEdgeType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }

// ######################################################################

// Enumeration of the model's evaluation entry points. Fields:
// is iterated, has triggers, takes 'firstIteration' flag, is slow
// clang-format off
#ವ್ಯಾಖ್ಯಾನಿಸು FOR_EACH_EVAL(macro) \
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
    ಎಣಿಕೆ en : uint8_t {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_EVAL_ID(id, iterated, triggers, ಮೊದಲ, slow) id,
        FOR_EACH_EVAL(VL_EVAL_ID)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_EVAL_ID
            ENUM_ಕೊನೆ
    };
    ಎಣಿಕೆ en m_e;

    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_EVAL_NAME(id, iterated, triggers, ಮೊದಲ, slow) #id,
            FOR_EACH_EVAL(VL_EVAL_NAME)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_EVAL_NAME
                "_ENUM_END"  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool isIterated() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_EVAL_IS_ITERATED(id, iterated, triggers, ಮೊದಲ, slow) iterated,
            FOR_EACH_EVAL(VL_EVAL_IS_ITERATED)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_EVAL_IS_ITERATED
                false  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool hasTriggers() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_EVAL_HAS_TRIGGERS(id, iterated, triggers, ಮೊದಲ, slow) triggers,
            FOR_EACH_EVAL(VL_EVAL_HAS_TRIGGERS)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_EVAL_HAS_TRIGGERS
                false  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool ಮೊದಲ_Iteration() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_EVAL_ಮೊದಲ(id, iterated, triggers, ಮೊದಲ, slow) ಮೊದಲ,
            FOR_EACH_EVAL(VL_EVAL_ಮೊದಲ)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_EVAL_ಮೊದಲ
                false  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
    bool slow() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr bool ಮೌಲ್ಯಗಳು[] = {
#ವ್ಯಾಖ್ಯಾನಿಸು VL_EVAL_SLOW(id, iterated, triggers, ಮೊದಲ, slow) slow,
            FOR_EACH_EVAL(VL_EVAL_SLOW)
#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು VL_EVAL_SLOW
                false  //
        };
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }

    // Short name
    std::string ಟ್ಯಾಗ್() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VString::downcase(ascii()); }
    // Name of the generated entry point function
    std::string funcName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "_eval_" + ಟ್ಯಾಗ್(); }
    // Name of the VerilatedModel virtual method invoking the entry point
    std::string eval_ವಿಧಾನ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "eval" + capitalized_ಟ್ಯಾಗ್(); }
    // Name of the function dumping the triggers of this region
    std::string dumpTriggersFuncName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "_eval_dump_triggers__" + ಟ್ಯಾಗ್(); }
    // Name of the VerilatedModel virtual method dumping this region's triggers
    std::string dump_Triggers_ವಿಧಾನ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "dumpTriggers" + capitalized_ಟ್ಯಾಗ್(); }

    // cppcheck-suppress noExplicitConstructor
    VEval(en _e)
        : m_e{_e} {}
    explicit VEval(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}
    ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }

ಖಾಸಗಿ:
    std::string capitalized_ಟ್ಯಾಗ್() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ std::string name = ascii();
        ಹಿಂತಿರುಗಿ name.substr(0, 1) + VString::downcase(name.substr(1));
    }
};

#ವ್ಯಾಖ್ಯಾನ_ರದ್ದು FOR_EACH_EVAL

// ######################################################################

class VFwdType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { NONE, ENUM, STRUCT, UNION, CLASS, INTERFACE_CLASS, GENERIC_INTERFACE };
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"none", "enum", "struct", "union", "class", "interface class", "generic interface"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    VFwdType()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFwdType(en _e)
        : m_e{_e} {}
    explicit VFwdType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    // Is a node type compatible with the declaration
    bool is_ಗಂಟು_Compatible(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ;
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VFwdType& lhs, ಸ್ಥಿರಾಂಕ VFwdType& rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VFwdType& lhs, VFwdType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VFwdType::en lhs, ಸ್ಥಿರಾಂಕ VFwdType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VFwdType& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
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
    bool isCached() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cachedCnt == s_cachedCntGbl; }
    bool get() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_state; }
    ಶೂನ್ಯ ಗಣ(bool flag) {
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
    ಎಣಿಕೆ en : uint8_t { JOIN = 0, JOIN_ANY = 1, JOIN_NONE = 2 };
    ಎಣಿಕೆ en m_e;
    // CONSTRUCTOR - note defaults to *UNKNOWN*
    VJoinType()
        : m_e{JOIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VJoinType(en _e)
        : m_e{_e} {}
    explicit VJoinType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"JOIN", "JOIN_ANY", "JOIN_NONE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* verilogKwd() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"join", "join_any", "join_none"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    bool join() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == JOIN; }
    bool joinAny() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == JOIN_ANY; }
    bool joinNone() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == JOIN_NONE; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VJoinType& lhs, ಸ್ಥಿರಾಂಕ VJoinType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VJoinType& lhs, VJoinType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VJoinType::en lhs, ಸ್ಥಿರಾಂಕ VJoinType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VJoinType& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

class VLifetime final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        NONE,
        AUTOMATIC_EXPLICIT,  // Automatic assigned by user
        AUTOMATIC_IMPLICIT,  // AUtomatic propagated from above
        STATIC_EXPLICIT,  // Static assigned by user
        STATIC_IMPLICIT
    };  // Static propagated from above
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"NONE", "VAUTOM", "VAUTOMI", "VSTATIC", "VSTATICI"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    VLifetime()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VLifetime(en _e)
        : m_e{_e} {}
    explicit VLifetime(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    bool isNone() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == NONE; }
    bool isAutomatic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == AUTOMATIC_EXPLICIT || m_e == AUTOMATIC_IMPLICIT; }
    bool isStatic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == STATIC_EXPLICIT || m_e == STATIC_IMPLICIT; }
    bool isStaticExplicit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == STATIC_EXPLICIT; }
    VLifetime makeImplicit() ಸ್ಥಿರಾಂಕ {
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ AUTOMATIC_EXPLICIT: ಹಿಂತಿರುಗಿ AUTOMATIC_IMPLICIT;
        ಸ್ಥಿತಿ STATIC_EXPLICIT: ಹಿಂತಿರುಗಿ STATIC_IMPLICIT;
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ m_e;
        }
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VLifetime& lhs, ಸ್ಥಿರಾಂಕ VLifetime& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VLifetime& lhs, VLifetime::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VLifetime::en lhs, ಸ್ಥಿರಾಂಕ VLifetime& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VLifetime& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################
//  VNumRange - Structure containing numeric range information
//  See also AstRange, which is a symbolic version of this

class VNumRange final {
ಸಾರ್ವಜನಿಕ:
    // Iterable sequence of integers
    class Sequence final {
        class Iterator final {
            ಪೂರ್ಣಾಂಕ m_ಸೂಚ್ಯಂಕ;  // Current index
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_inc;  // Step, +1 or -1
        ಸಾರ್ವಜನಿಕ:
            Iterator(ಪೂರ್ಣಾಂಕ ಸೂಚ್ಯಂಕ, ಪೂರ್ಣಾಂಕ inc)
                : m_ಸೂಚ್ಯಂಕ{ಸೂಚ್ಯಂಕ}
                , m_inc{inc} {}
            ಪೂರ್ಣಾಂಕ ಕ್ರಿಯಾಕಾರಕ*() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸೂಚ್ಯಂಕ; }
            Iterator& ಕ್ರಿಯಾಕಾರಕ++() {
                m_ಸೂಚ್ಯಂಕ += m_inc;
                ಹಿಂತಿರುಗಿ *this;
            }
            bool ಕ್ರಿಯಾಕಾರಕ!=(ಸ್ಥಿರಾಂಕ Iterator& that) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸೂಚ್ಯಂಕ != that.m_ಸೂಚ್ಯಂಕ; }
        };
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_begin;  // First index
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಕೊನೆ;  // One past the last index
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_inc;  // Step, +1 or -1
    ಸಾರ್ವಜನಿಕ:
        Sequence(ಪೂರ್ಣಾಂಕ ಮೊದಲ, ಪೂರ್ಣಾಂಕ ಕೊನೆಯ, ಪೂರ್ಣಾಂಕ inc)
            : m_begin{ಮೊದಲ}
            , m_ಕೊನೆ{ಕೊನೆಯ + inc}
            , m_inc{inc} {}
        Iterator begin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ Iterator{m_begin, m_inc}; }
        Iterator ಕೊನೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ Iterator{m_ಕೊನೆ, m_inc}; }
    };

    ಪೂರ್ಣಾಂಕ m_ಎಡ = 0;  // Left side of range (pre-':')
    ಪೂರ್ಣಾಂಕ m_ಬಲ = 0;  // Right side of range (post-':')
    bool m_ranged = false;  // Has a range
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VNumRange& rhs) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_ಎಡ == rhs.m_ಎಡ && m_ಬಲ == rhs.m_ಬಲ && m_ranged == rhs.m_ranged;
    }
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ VNumRange& rhs) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ ((m_ಎಡ < rhs.m_ಎಡ)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (!(m_ಎಡ == rhs.m_ಎಡ)) ಹಿಂತಿರುಗಿ false;  // lhs > rhs
        ಆದರೆ ((m_ಬಲ < rhs.m_ಬಲ)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (!(m_ಬಲ == rhs.m_ಬಲ)) ಹಿಂತಿರುಗಿ false;  // lhs > rhs
        ಆದರೆ ((m_ranged < rhs.m_ranged)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (!(m_ranged == rhs.m_ranged)) ಹಿಂತಿರುಗಿ false;  // lhs > rhs
        ಹಿಂತಿರುಗಿ false;
    }
    //
    VNumRange() = ಪೂರ್ವನಿಯೋಜಿತ;
    VNumRange(ಪೂರ್ಣಾಂಕ hi, ಪೂರ್ಣಾಂಕ lo, bool ascending) { init(hi, lo, ascending); }
    VNumRange(ಪೂರ್ಣಾಂಕ ಎಡ_2, ಪೂರ್ಣಾಂಕ ಬಲ_2)
        : m_ಎಡ{ಎಡ_2}
        , m_ಬಲ{ಬಲ_2}
        , m_ranged{true} {}
    ~VNumRange() = ಪೂರ್ವನಿಯೋಜಿತ;
    // MEMBERS
    ಶೂನ್ಯ init(ಪೂರ್ಣಾಂಕ hi, ಪೂರ್ಣಾಂಕ lo, bool ascending) {
        ಆದರೆ (lo > hi) std::ಅದಲುಬದಲು(hi, lo);
        m_ಎಡ = ascending ? lo : hi;
        m_ಬಲ = ascending ? hi : lo;
        m_ranged = true;
    }
    ಪೂರ್ಣಾಂಕ ಎಡ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಡ; }
    ಪೂರ್ಣಾಂಕ ಬಲ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಲ; }
    ಪೂರ್ಣಾಂಕ hi() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_ಎಡ > m_ಬಲ ? m_ಎಡ : m_ಬಲ;
    }  // How to show a declaration
    ಪೂರ್ಣಾಂಕ lo() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_ಎಡ > m_ಬಲ ? m_ಬಲ : m_ಎಡ;
    }  // How to show a declaration
    ಪೂರ್ಣಾಂಕ ಎಡ_ಗಮ್ಯ_ಬಲ_Inc() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ascending() ? 1 : -1; }
    // Iterate indices from left() to right() inclusive: 'for (int i : range.seqLeftToRight())'
    Sequence seq_ಎಡ_ಗಮ್ಯ_ಬಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ {m_ಎಡ, m_ಬಲ, ಎಡ_ಗಮ್ಯ_ಬಲ_Inc()}; }
    // Iterate indices from right() to left() inclusive: 'for (int i : range.seqRightToLeft())'
    Sequence seq_ಬಲ_ಗಮ್ಯ_ಎಡ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ {m_ಬಲ, m_ಎಡ, -ಎಡ_ಗಮ್ಯ_ಬಲ_Inc()}; }
    // Iterate indices from lo() to hi() inclusive: 'for (int i : range.seqLoToHi())'
    Sequence seq_Lo_ಗಮ್ಯ_Hi() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ {lo(), hi(), 1}; }
    // Iterate indices from hi() to lo() inclusive: 'for (int i : range.seqHiToLo())'
    Sequence seq_Hi_ಗಮ್ಯ_Lo() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ {hi(), lo(), -1}; }
    ಪೂರ್ಣಾಂಕ elements() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ hi() - lo() + 1; }
    bool ranged() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ranged; }
    bool ascending() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಡ < m_ಬಲ; }
    ಪೂರ್ಣಾಂಕ hiMaxSelect() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (lo() < 0 ? hi() - lo() : hi());
    }  // Maximum value a [] select may index
    string ascii() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (ranged()) {
            ಹಿಂತಿರುಗಿ "["s + std::ಗಮ್ಯ_string(ಎಡ_2()) + ":" + std::ಗಮ್ಯ_string(ಬಲ_2()) + "]";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ "[norg]";
        }
    }
    ಶೂನ್ಯ dump(std::ostream& str) ಸ್ಥಿರಾಂಕ { str << ascii(); }
};
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VNumRange& rhs) {
    rhs.dump(os);
    ಹಿಂತಿರುಗಿ os;
}

// ######################################################################

class VPragmaType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        COVERAGE_ಖಂಡ_OFF,
        HIER_ಖಂಡ,
        HIER_PARAMS,
        INLINE_MODULE,
        NO_INLINE_MODULE,
        NO_INLINE_TASK,
        ಬಹಿರಂಗ_MODULE,
        ಬಹಿರಂಗ_TASK,
        TIMEUNIT_ಗಣ,
        UNROLL_DISABLE,
        UNROLL_ತುಂಬಿದ,
        ತುಂಬಿದ_CASE,
        PARALLEL_CASE,
        VERILATOR_LIB,
        ENUM_ಗಾತ್ರ
    };
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {
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
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VPragmaType(en _e)
        : m_e{_e} {}
    explicit VPragmaType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VPragmaType& lhs, ಸ್ಥಿರಾಂಕ VPragmaType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VPragmaType& lhs, VPragmaType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VPragmaType::en lhs, ಸ್ಥಿರಾಂಕ VPragmaType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VPragmaType& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################
// Defines what kind of randomization is done on a variable

class VRandAttr final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        NONE,  // Not randomizable
        RAND,  // Has a rand modifier
        RAND_CYCLIC,  // Has a randc modifier
        RAND_INLINE  // Not rand/randc, but used in inline random variable control
    };
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"NONE", "RAND", "RANDC", "RAND_INLINE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    VRandAttr()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VRandAttr(en _e)
        : m_e{_e} {}
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    bool isRandomizable() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e != NONE; }
    bool isRand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == RAND || m_e == RAND_CYCLIC; }
    bool isRandC() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == RAND_CYCLIC; }
};
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VRandAttr& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################
//  VSelfPointerText - Represents text to be emitted before a given var reference, call, etc. to
//  serve as a pointer to a 'self' object. For example, it could be empty (no self pointer), or the
//  string 'this', or 'vlSymsp->...'

class VSelf_ಸೂಚಕ_ಪಠ್ಯ final {
    // STATIC MEMBERS
    // Keep these in shared pointers to avoid branching for special cases
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ std::shared_ptr<ಸ್ಥಿರಾಂಕ string> s_emptyp;  // Holds ""
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ std::shared_ptr<ಸ್ಥಿರಾಂಕ string> s_thisp;  // Holds "this"

    // MEMBERS
    std::shared_ptr<ಸ್ಥಿರಾಂಕ string> m_strp;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    class ಖಾಲಿಗಳು_2 {};  // for creator type-overload selection
    explicit VSelf_ಸೂಚಕ_ಪಠ್ಯ(ಖಾಲಿಗಳು_2)
        : m_strp{s_emptyp} {}
    class This {};  // for creator type-overload selection
    explicit VSelf_ಸೂಚಕ_ಪಠ್ಯ(This)
        : m_strp{s_thisp} {}
    VSelf_ಸೂಚಕ_ಪಠ್ಯ(This, ಸ್ಥಿರಾಂಕ string& ಕ್ಷೇತ್ರ)
        : m_strp{std::make_shared<ಸ್ಥಿರಾಂಕ string>("this->" + ಕ್ಷೇತ್ರ)} {}
    class VlSyms {};  // for creator type-overload selection
    VSelf_ಸೂಚಕ_ಪಠ್ಯ(VlSyms, ಸ್ಥಿರಾಂಕ string& ಕ್ಷೇತ್ರ)
        : m_strp{std::make_shared<ಸ್ಥಿರಾಂಕ string>("(&vlSymsp->vlInst." + ಕ್ಷೇತ್ರ + ')')} {}

    // METHODS
    bool is_ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_strp == s_emptyp; }
    bool isVlSym() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_strp->ಕಂಡುಹಿಡಿ("vlSymsp") != string::npos; }
    bool hasThis() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_strp == s_thisp || VString::startsWith(*m_strp, "this"); }
    string protect(bool useSelfForThis, bool protect) ಸ್ಥಿರಾಂಕ;
    ಸ್ಥಿರ string replaceThis(bool useSelfForThis, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ);
    ಸ್ಥಿರಾಂಕ std::string& asString() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ *m_strp; }
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VSelf_ಸೂಚಕ_ಪಠ್ಯ& other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ *m_strp == *other.m_strp; }
};

// ######################################################################

class VSigning final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        UNSIGNED,
        SIGNED,
        NOSIGN,
        _ENUM_MAX  // Leave last
    };
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"UNSIGNED", "SIGNED", "NOSIGN"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    VSigning()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VSigning(en _e)
        : m_e{_e} {}
    ಸ್ಥಿರ VSigning fromBool(bool isSigned) {  // Factory method
        ಹಿಂತಿರುಗಿ isSigned ? VSigning{SIGNED} : VSigning{UNSIGNED};
    }
    explicit VSigning(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    bool isSigned() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == SIGNED; }
    bool isNosign() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e == NOSIGN; }
    // No isUnsigned() as it's ambiguous if NOSIGN should be included or not.
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VSigning& lhs, ಸ್ಥಿರಾಂಕ VSigning& rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VSigning& lhs, VSigning::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VSigning::en lhs, ಸ್ಥಿರಾಂಕ VSigning& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VSigning& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

class VStrength final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { HIGHZ, SMALL, MEDIUM, WEAK, LARGE, PULL, STRONG, SUPPLY };
    ಎಣಿಕೆ en m_e;

    // cppcheck-suppress noExplicitConstructor
    constexpr VStrength(en strengthLevel)
        : m_e{strengthLevel} {}
    explicit VStrength(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"highz", "small", "medium", "weak", "large", "pull", "strong", "supply"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VStrength& lhs, ಸ್ಥಿರಾಂಕ VStrength& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VStrength& lhs, VStrength::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VStrength::en lhs, ಸ್ಥಿರಾಂಕ VStrength& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VStrength& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################
//  VSystemCSectionType - Represents the type of a `systemc_* block (Verilator specific extension)

class VSystemCSectionType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        CTOR,  // `systemc_ctor
        DTOR,  // `systemc_dtor
        HDR,  // `systemc_header
        HDR_POST,  // `systemc_header_post
        IMP,  // `systemc_implementation
        IMP_HDR,  // `systemc_imp_header
        INT  // `systemc_interface
    };
    ಎಣಿಕೆ en m_e;
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"`systemc_ctor",  //
                                                      "`systemc_dtor",  //
                                                      "`systemc_header",  //
                                                      "`systemc_header_post",  //
                                                      "`systemc_implementation",  //
                                                      "`systemc_imp_header",  //
                                                      "`systemc_interface"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    // cppcheck-suppress noExplicitConstructor
    constexpr VSystemCSectionType(en _e)
        : m_e{_e} {}
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
};

//######################################################################

class VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        // Note: Entries must match VerilatedTracePrefixType
        ARRAY_PACKED,
        ARRAY_UNPACKED,
        SCOPE_MODULE,
        SCOPE_INTERFACE,
        STRUCT_PACKED,
        STRUCT_UNPACKED,
        UNION_PACKED,
    };
    ಎಣಿಕೆ en m_e;
    // cppcheck-suppress noExplicitConstructor
    constexpr VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type(en _e)
        : m_e{_e} {}
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"ARRAY_PACKED",  "ARRAY_UNPACKED",  "SCOPE_MODULE", "SCOPE_INTERFACE",
               "STRUCT_PACKED", "STRUCT_UNPACKED", "UNION_PACKED"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type& lhs, ಸ್ಥಿರಾಂಕ VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type& lhs, VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::en rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type::en lhs, ಸ್ಥಿರಾಂಕ VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type& rhs) {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
}
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VTrace_ಪೂರ್ವಪ್ರತ್ಯಯ_Type& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

//######################################################################

class VTraceType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        CONSTANT,  // Constant value dump (once at the beginning)
        ತುಂಬಿದ,  // Full value dump (always emitted)
        CHANGE  // Incremental value dump (emitted only if the value changed)
    };
    ಎಣಿಕೆ en m_e;
    VTraceType()
        : m_e{CONSTANT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTraceType(en _e)
        : m_e{_e} {}
    explicit VTraceType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"CONSTANT", "FULL", "CHANGE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* func_ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"trace_const", "trace_full", "trace_chg"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VTraceType& lhs, ಸ್ಥಿರಾಂಕ VTraceType& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VTraceType& lhs, VTraceType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VTraceType::en lhs, ಸ್ಥಿರಾಂಕ VTraceType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VTraceType& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

//######################################################################

class VUseType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        // Enum values are compared with <, so order matters
        INT_FWD_CLASS = 1 << 0,  // Interface (.h) needs a forward class declaration
        INT_INCLUDE = 1 << 1,  // Interface (.h) needs an include
    };
    ಎಣಿಕೆ en m_e;
    VUseType()
        : m_e{INT_FWD_CLASS} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VUseType(en _e)
        : m_e{_e} {}
    explicit VUseType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    bool ಒಳಗೊಂಡಿದೆ_Any(VUseType other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e & other.m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"INT_FWD", "INT_INC", "INT_FWD_INC"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e - 1];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VUseType& lhs, ಸ್ಥಿರಾಂಕ VUseType& rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VUseType& lhs, VUseType::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VUseType::en lhs, ಸ್ಥಿರಾಂಕ VUseType& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
constexpr VUseType::en ಕ್ರಿಯಾಕಾರಕ|(VUseType::en lhs, VUseType::en rhs) {
    ಹಿಂತಿರುಗಿ VUseType::en(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr VUseType::en ಕ್ರಿಯಾಕಾರಕ&(VUseType::en lhs, VUseType::en rhs) {
    ಹಿಂತಿರುಗಿ VUseType::en(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VUseType& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

class VVarType final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
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
        ಪೋರ್ಟ್,  // Used in parser to recognize ports
        BLOCKTEMP,
        MODULETEMP,
        STMTTEMP,
        XTEMP,
        IFACEREF,  // Used to link Interfaces between modules
        MEMBER
    };
    ಎಣಿಕೆ en m_e;
    VVarType() VL_MT_ಸುರಕ್ಷಿತ : m_e{UNKNOWN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VVarType(en _e) VL_MT_ಸುರಕ್ಷಿತ : m_e{_e} {}
    explicit VVarType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"?",        "GPARAM",  "LPARAM",   "SPECPARAM", "GENVAR",    "VAR",
               "SUPPLY0",  "SUPPLY1", "WIRE",     "WREAL",     "TRIAND",    "TRIOR",
               "TRIWIRE",  "TRI0",    "TRI1",     "PORT",      "BLOCKTEMP", "MODULETEMP",
               "STMTTEMP", "XTEMP",   "IFACEREF", "MEMBER"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    bool isParam() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == GPARAM || m_e == LPARAM; }
    bool isSignal() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (m_e == WIRE || m_e == WREAL || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1
                || m_e == ಪೋರ್ಟ್ || m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == VAR || m_e == TRIOR
                || m_e == TRIAND);
    }
    bool isNet() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (m_e == WIRE || m_e == TRIWIRE || m_e == TRI0 || m_e == TRI1 || m_e == SUPPLY0
                || m_e == SUPPLY1 || m_e == TRIOR || m_e == TRIAND);
    }
    bool isWor() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ (m_e == TRIOR); }
    bool isWiredNet() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ (m_e == TRIOR || m_e == TRIAND); }
    bool isContAssignable() ಸ್ಥಿರಾಂಕ {  // In Verilog, always ok in SystemVerilog
        ಹಿಂತಿರುಗಿ (m_e == SUPPLY0 || m_e == SUPPLY1 || m_e == WIRE || m_e == WREAL || m_e == TRIWIRE
                || m_e == TRI0 || m_e == TRI1 || m_e == ಪೋರ್ಟ್ || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF);
    }
    bool isProcAssignable() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (m_e == GPARAM || m_e == LPARAM || m_e == GENVAR || m_e == VAR || m_e == BLOCKTEMP
                || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP || m_e == IFACEREF
                || m_e == MEMBER);
    }
    bool is_ತಾತ್ಕಾಲಿಕ() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (m_e == BLOCKTEMP || m_e == MODULETEMP || m_e == STMTTEMP || m_e == XTEMP);
    }
    bool isVPIAccessible() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (m_e == VAR || m_e == GPARAM || m_e == LPARAM || m_e == SPECPARAM || m_e == ಪೋರ್ಟ್
                || m_e == WIRE || m_e == TRI0 || m_e == TRI1);
    }

    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* trace_Sig_ಬಗೆ() ಸ್ಥಿರಾಂಕ {
        // VerilatedTraceSigKind to used in trace signal declaration
        ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ lut[] = {
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
        ಹಿಂತಿರುಗಿ lut[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VVarType& lhs, ಸ್ಥಿರಾಂಕ VVarType& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VVarType& lhs, VVarType::en rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VVarType::en lhs, ಸ್ಥಿರಾಂಕ VVarType& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
}
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VVarType& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

// Not in sorted order, as depends on above classes
class VBasic_Type_ಕೀಲಿ final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಅಗಲ;  // From AstNodeDType: Bit width of operation
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_ಅಗಲ_Min;  // From AstNodeDType: If unsized, bitwidth of minimum implementation
    ಸ್ಥಿರಾಂಕ VNumRange m_nrange;  // From AstBasicDType: Numeric msb/lsb (if non-opaque keyword)
    ಸ್ಥಿರಾಂಕ VSigning m_numeric;  // From AstNodeDType: Node is signed
    ಸ್ಥಿರಾಂಕ VBasicDTypeKwd m_keyword;  // From AstBasicDType: What keyword created basic type
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VBasic_Type_ಕೀಲಿ& rhs) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_ಅಗಲ == rhs.m_ಅಗಲ && m_ಅಗಲ_Min == rhs.m_ಅಗಲ_Min && m_numeric == rhs.m_numeric
               && m_keyword == rhs.m_keyword && m_nrange == rhs.m_nrange;
    }
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ VBasic_Type_ಕೀಲಿ& rhs) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ ((m_ಅಗಲ < rhs.m_ಅಗಲ)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (!(m_ಅಗಲ == rhs.m_ಅಗಲ)) ಹಿಂತಿರುಗಿ false;  // lhs > rhs
        ಆದರೆ ((m_ಅಗಲ_Min < rhs.m_ಅಗಲ_Min)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (!(m_ಅಗಲ_Min == rhs.m_ಅಗಲ_Min)) ಹಿಂತಿರುಗಿ false;  // lhs > rhs
        ಆದರೆ ((m_numeric < rhs.m_numeric)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (!(m_numeric == rhs.m_numeric)) ಹಿಂತಿರುಗಿ false;  // lhs > rhs
        ಆದರೆ ((m_keyword < rhs.m_keyword)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (!(m_keyword == rhs.m_keyword)) ಹಿಂತಿರುಗಿ false;  // lhs > rhs
        ಆದರೆ ((m_nrange < rhs.m_nrange)) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (!(m_nrange == rhs.m_nrange)) ಹಿಂತಿರುಗಿ false;  // lhs > rhs
        ಹಿಂತಿರುಗಿ false;
    }
    VBasic_Type_ಕೀಲಿ(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric, VBasicDTypeKwd kwd,
                  ಸ್ಥಿರಾಂಕ VNumRange& nrange)
        : m_ಅಗಲ{ಅಗಲ}
        , m_ಅಗಲ_Min{ಅಗಲ_Min}
        , m_nrange{nrange}
        , m_numeric{numeric}
        , m_keyword{kwd} {}
    ~VBasic_Type_ಕೀಲಿ() = ಪೂರ್ವನಿಯೋಜಿತ;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
