// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Large 4-state numbers
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3_ಸಂಖ್ಯೆ_H
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3_ಸಂಖ್ಯೆ_H

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Hash.h"
#ಅನ್ತರ್ಭಾವ "V3StdFuture.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <cmath>
#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <vector>

class AstEnumDType;
class Ast_Enum_ಅಂಶ;
class Ast_ಗಂಟು;
class Ast_ಗಂಟು_DType;
class AstSFormatArg;
class File_ಸಾಲು;

//============================================================================

class VFormatAttr final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : ಅಕ್ಷರ {
        //               // AstSFormatArg is typically skipped for UNSIGNED, as is the default
        UNSIGNED = VL_VFORMATATTR_UNSIGNED,
        SIGNED = VL_VFORMATATTR_SIGNED,
        //
        COMPLEX = VL_VFORMATATTR_COMPLEX,
        DOUBLE = VL_VFORMATATTR_DOUBLE,
        ENUM = VL_VFORMATATTR_ENUM,
        ENUM_SIGNED = VL_VFORMATATTR_ENUM_SIGNED,
        SCOPE = VL_VFORMATATTR_SCOPE,
        STRING = VL_VFORMATATTR_STRING,
        TIMEUNIT = VL_VFORMATATTR_TIMEUNIT
    };
    ಎಣಿಕೆ en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಅಕ್ಷರ ascii() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    bool isComplex() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == COMPLEX; }
    bool isDouble() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == DOUBLE; }
    bool isEnum() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == SIGNED; }
    bool isString() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == STRING; }
    bool isUnsigned() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == UNSIGNED; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VFormatAttr& lhs, ಸ್ಥಿರಾಂಕ VFormatAttr& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VFormatAttr& lhs, VFormatAttr::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VFormatAttr::en lhs, ಸ್ಥಿರಾಂಕ VFormatAttr& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VFormatAttr& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

//============================================================================

class V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    ರಚನೆ ಮೌಲ್ಯ_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_ಮೌಲ್ಯ;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_ಮೌಲ್ಯ_X;
        bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X& other) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
            ಹಿಂತಿರುಗಿ m_ಮೌಲ್ಯ == other.m_ಮೌಲ್ಯ && m_ಮೌಲ್ಯ_X == other.m_ಮೌಲ್ಯ_X;
        }
    };

    ಎಣಿಕೆ class V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    ಮಿತ್ರ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type& rhs) VL_MT_ಸುರಕ್ಷಿತ {
        ಆಯ್ಕೆ (rhs) {
        ಸ್ಥಿತಿ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::UNINITIALIZED: ಹಿಂತಿರುಗಿ os << "UNINITIALIZED";
        ಸ್ಥಿತಿ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::LOGIC: ಹಿಂತಿರುಗಿ os << "LOGIC";
        ಸ್ಥಿತಿ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::DOUBLE: ಹಿಂತಿರುಗಿ os << "DOUBLE";
        ಸ್ಥಿತಿ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::STRING: ಹಿಂತಿರುಗಿ os << "STRING";
        }
        ಹಿಂತಿರುಗಿ os;
    }

ಖಾಸಗಿ:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ INLINE_ಪದಗಳು = std::max(
        static_cast<ಗಾತ್ರ_t>(2), std::max(ಗಾತ್ರ(std::string) / ಗಾತ್ರ(ಮೌಲ್ಯ_And_X),
                                         ಗಾತ್ರ(std::ಸದಿಶ<ಮೌಲ್ಯ_And_X>) / ಗಾತ್ರ(ಮೌಲ್ಯ_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ MAX_INLINE_ಅಗಲ = INLINE_ಪದಗಳು * ಗಾತ್ರ(ಮೌಲ್ಯ_And_X) / 2 * 8;

    // MEMBERS
    ಒಕ್ಕೂಟ {
        std::array<ಮೌಲ್ಯ_And_X, INLINE_ಪದಗಳು> m_inline_ಸಂಖ್ಯೆ;
        std::ಸದಿಶ<ಮೌಲ್ಯ_And_X> m_dynamic_ಸಂಖ್ಯೆ;
        std::string m_string;
    };

    ಪೂರ್ಣಾಂಕ m_ಅಗಲ = 0;  // Width (in bits) as specified/calculated
    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type m_type;

ಸಾರ್ವಜನಿಕ:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_ಹೆಜ್ಜೆ : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ()
        : m_type{V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_ಹೆಜ್ಜೆ{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ() { destroy_Stored_ಮೌಲ್ಯ(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ& other)
        : m_ಅಗಲ{other.m_ಅಗಲ}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_ಹೆಜ್ಜೆ{other.m_is1_ಹೆಜ್ಜೆ}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ಆದರೆ (other.is_Inline_ಸಂಖ್ಯೆ()) {
            init_Inline_ಸಂಖ್ಯೆ(other.m_inline_ಸಂಖ್ಯೆ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.is_Dynamic_ಸಂಖ್ಯೆ()) {
            init_Dynamic_ಸಂಖ್ಯೆ(other.m_dynamic_ಸಂಖ್ಯೆ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ& other) {
        ಆದರೆ (other.is_Inline_ಸಂಖ್ಯೆ()) {
            destroy_Stored_ಮೌಲ್ಯ();
            init_Inline_ಸಂಖ್ಯೆ(other.m_inline_ಸಂಖ್ಯೆ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.is_Dynamic_ಸಂಖ್ಯೆ()) {
            reinit_With_Or_ನಿಯೋಜಿಸು_Dynamic_ಸಂಖ್ಯೆ(other.m_dynamic_ಸಂಖ್ಯೆ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.isString()) {
            reinit_With_Or_ನಿಯೋಜಿಸು_String(other.m_string);
        } ಇಲ್ಲದಿದ್ದರೆ {
            destroy_Stored_ಮೌಲ್ಯ();
        }
        m_ಅಗಲ = other.m_ಅಗಲ;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_ಹೆಜ್ಜೆ = other.m_is1_ಹೆಜ್ಜೆ;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        ಹಿಂತಿರುಗಿ *this;
    }

    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ(V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ&& other)
        : m_ಅಗಲ{other.m_ಅಗಲ}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_ಹೆಜ್ಜೆ{other.m_is1_ಹೆಜ್ಜೆ}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ಆದರೆ (other.is_Inline_ಸಂಖ್ಯೆ()) {
            init_Inline_ಸಂಖ್ಯೆ(other.m_inline_ಸಂಖ್ಯೆ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.is_Dynamic_ಸಂಖ್ಯೆ()) {
            init_Dynamic_ಸಂಖ್ಯೆ(std::move(other.m_dynamic_ಸಂಖ್ಯೆ));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::UNINITIALIZED;
    }

    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ& ಕ್ರಿಯಾಕಾರಕ=(V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ&& other) {
        ಆದರೆ (other.is_Inline_ಸಂಖ್ಯೆ()) {
            destroy_Stored_ಮೌಲ್ಯ();
            init_Inline_ಸಂಖ್ಯೆ(other.m_inline_ಸಂಖ್ಯೆ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.is_Dynamic_ಸಂಖ್ಯೆ()) {
            reinit_With_Or_ನಿಯೋಜಿಸು_Dynamic_ಸಂಖ್ಯೆ(std::move(other.m_dynamic_ಸಂಖ್ಯೆ));
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (other.isString()) {
            reinit_With_Or_ನಿಯೋಜಿಸು_String(std::move(other.m_string));
        } ಇಲ್ಲದಿದ್ದರೆ {
            destroy_Stored_ಮೌಲ್ಯ();
        }
        m_ಅಗಲ = other.m_ಅಗಲ;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_ಹೆಜ್ಜೆ = other.m_is1_ಹೆಜ್ಜೆ;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::UNINITIALIZED;
        ಹಿಂತಿರುಗಿ *this;
    }

    // ACCESSORS
    ಮೌಲ್ಯ_And_X* ಅಂಕೆ() {
        UASSERT(is_ಸಂಖ್ಯೆ(), "`num` member accessed when data type is " << m_type);
        ಹಿಂತಿರುಗಿ is_Inline_ಸಂಖ್ಯೆ() ? m_inline_ಸಂಖ್ಯೆ.ದತ್ತಾಂಶ() : m_dynamic_ಸಂಖ್ಯೆ.ದತ್ತಾಂಶ();
    }
    ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X* ಅಂಕೆ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        UASSERT(is_ಸಂಖ್ಯೆ(), "`num` member accessed when data type is " << m_type);
        ಹಿಂತಿರುಗಿ is_Inline_ಸಂಖ್ಯೆ() ? m_inline_ಸಂಖ್ಯೆ.ದತ್ತಾಂಶ() : m_dynamic_ಸಂಖ್ಯೆ.ದತ್ತಾಂಶ();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ಹಿಂತಿರುಗಿ m_string;
    }
    ಸ್ಥಿರಾಂಕ std::string& str() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ಹಿಂತಿರುಗಿ m_string;
    }

    ಪೂರ್ಣಾಂಕ ಅಗಲ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಅಗಲ; }
    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type type() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_type; }

    // METHODS
    ಶೂನ್ಯ resize(ಪೂರ್ಣಾಂಕ ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ) {
        ಆದರೆ (m_ಅಗಲ == ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_ಪದಗಳು(m_ಅಗಲ) == ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_ಪದಗಳು(ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ)) {
            m_ಅಗಲ = ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ;
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (is_Dynamic_ಸಂಖ್ಯೆ()) {
            ಆದರೆ (ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ > MAX_INLINE_ಅಗಲ) {
                m_dynamic_ಸಂಖ್ಯೆ.resize(ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_ಪದಗಳು(ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ));
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ dynamic_ಬಿಟ್ಟುಗಳು = std::move(m_dynamic_ಸಂಖ್ಯೆ);
                destroy_Dynamic_ಸಂಖ್ಯೆ();
                init_Inline_ಸಂಖ್ಯೆ();
                std::ಸ್ಮರಣೆ_ನಕಲು(m_inline_ಸಂಖ್ಯೆ.ದತ್ತಾಂಶ(), dynamic_ಬಿಟ್ಟುಗಳು.ದತ್ತಾಂಶ(), ಗಾತ್ರ(m_inline_ಸಂಖ್ಯೆ));
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (is_Inline_ಸಂಖ್ಯೆ()) {
            ಆದರೆ (ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ > MAX_INLINE_ಅಗಲ) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಬಿಟ್ಟುಗಳು = m_inline_ಸಂಖ್ಯೆ;
                init_Dynamic_ಸಂಖ್ಯೆ(ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_ಪದಗಳು(ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ));
                std::ಸ್ಮರಣೆ_ನಕಲು(m_dynamic_ಸಂಖ್ಯೆ.ದತ್ತಾಂಶ(), ಬಿಟ್ಟುಗಳು.ದತ್ತಾಂಶ(), ಗಾತ್ರ(ಬಿಟ್ಟುಗಳು));
            }
        }
        m_ಅಗಲ = ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ;
    }

    ಶೂನ್ಯ ಗಣ_String() {
        // If there has been a string already it is kept intact.
        ಆದರೆ (isString()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (is_Dynamic_ಸಂಖ್ಯೆ()) destroy_Dynamic_ಸಂಖ್ಯೆ();
        initString();
        m_type = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::STRING;
    }
    ಶೂನ್ಯ ಗಣ_String(std::string&& s) {
        reinit_With_Or_ನಿಯೋಜಿಸು_String(std::move(s));
        m_type = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::STRING;
    }
    ಶೂನ್ಯ ಗಣ_String(ಸ್ಥಿರಾಂಕ std::string& s) {
        reinit_With_Or_ನಿಯೋಜಿಸು_String(s);
        m_type = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::STRING;
    }

    ಶೂನ್ಯ ಗಣ_Double() {
        destroy_Stored_ಮೌಲ್ಯ();
        ಆದರೆ (!is_Inline_ಸಂಖ್ಯೆ()) init_Inline_ಸಂಖ್ಯೆ();
        m_type = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::DOUBLE;
        resize(64);
    }

    ಶೂನ್ಯ ಗಣ_Logic() {
        ಆದರೆ (isString()) destroyString();
        ಆದರೆ (!is_ಸಂಖ್ಯೆ()) {
            ಆದರೆ (m_ಅಗಲ <= MAX_INLINE_ಅಗಲ) {
                init_Inline_ಸಂಖ್ಯೆ();
            } ಇಲ್ಲದಿದ್ದರೆ {
                init_Dynamic_ಸಂಖ್ಯೆ(ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_ಪದಗಳು(m_ಅಗಲ));
            }
        }
        m_type = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::LOGIC;
        resize(m_ಅಗಲ);
    }

ಖಾಸಗಿ:
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಂಕ ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_ಪದಗಳು(ಪೂರ್ಣಾಂಕ ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ) VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ (ಬಿಟ್ಟುಗಳು_ಎಣಿಕೆ + 31) / 32; }

    bool is_ಸಂಖ್ಯೆ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_type == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::DOUBLE || m_type == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::LOGIC;
    }
    bool is_Inline_ಸಂಖ್ಯೆ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (m_ಅಗಲ <= MAX_INLINE_ಅಗಲ)
               && (m_type == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::DOUBLE || m_type == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::LOGIC);
    }
    bool is_Dynamic_ಸಂಖ್ಯೆ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (m_ಅಗಲ > MAX_INLINE_ಅಗಲ) && (m_type == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::LOGIC);
    }
    bool isString() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_type == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::STRING; }

    ಮಾದರಿ <typename... ವಾದಗಳು_2>
    ಶೂನ್ಯ init_Inline_ಸಂಖ್ಯೆ(ವಾದಗಳು_2&&... ವಾದಗಳು) {
        new (&m_inline_ಸಂಖ್ಯೆ) std::array<ಮೌಲ್ಯ_And_X, INLINE_ಪದಗಳು>(std::forward<ವಾದಗಳು_2>(ವಾದಗಳು)...);
    }
    ಮಾದರಿ <typename... ವಾದಗಳು_2>
    ಶೂನ್ಯ init_Dynamic_ಸಂಖ್ಯೆ(ವಾದಗಳು_2&&... ವಾದಗಳು) {
        new (&m_dynamic_ಸಂಖ್ಯೆ) std::ಸದಿಶ<ಮೌಲ್ಯ_And_X>(std::forward<ವಾದಗಳು_2>(ವಾದಗಳು)...);
    }
    ಮಾದರಿ <typename... ವಾದಗಳು_2>
    ಶೂನ್ಯ initString(ವಾದಗಳು_2&&... ವಾದಗಳು) {
        new (&m_string) std::string(std::forward<ವಾದಗಳು_2>(ವಾದಗಳು)...);
    }

    string formatDecimal();

    ಶೂನ್ಯ destroy_Dynamic_ಸಂಖ್ಯೆ() { m_dynamic_ಸಂಖ್ಯೆ.~ಸದಿಶ(); }
    ಶೂನ್ಯ destroyString() { m_string.~string(); }
    ಶೂನ್ಯ destroy_Stored_ಮೌಲ್ಯ() {
        ಆದರೆ (isString())
            destroyString();
        ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (is_Dynamic_ಸಂಖ್ಯೆ())
            destroy_Dynamic_ಸಂಖ್ಯೆ();
    }

    ಮಾದರಿ <typename T>
    ಶೂನ್ಯ reinit_With_Or_ನಿಯೋಜಿಸು_Dynamic_ಸಂಖ್ಯೆ(T&& s) {
        ಆದರೆ (is_Dynamic_ಸಂಖ್ಯೆ()) {
            m_dynamic_ಸಂಖ್ಯೆ = std::forward<T>(s);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (isString()) destroyString();
        init_Dynamic_ಸಂಖ್ಯೆ(std::forward<T>(s));
    }
    ಮಾದರಿ <typename T>
    ಶೂನ್ಯ reinit_With_Or_ನಿಯೋಜಿಸು_String(T&& s) {
        ಆದರೆ (isString()) {
            m_string = std::forward<T>(s);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (is_Dynamic_ಸಂಖ್ಯೆ()) destroy_Dynamic_ಸಂಖ್ಯೆ();
        initString(std::forward<T>(s));
    }
};

class V3_ಸಂಖ್ಯೆ final {
    // TYPES
    using ಮೌಲ್ಯ_And_X = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ::ಮೌಲ್ಯ_And_X;
    using V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type = V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ::V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type;

    // MEMBERS
    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ m_ದತ್ತಾಂಶ;
    Ast_ಗಂಟು* m_nodep = nullptr;  // Parent node - for error reporting only
    File_ಸಾಲು* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    ಶೂನ್ಯ ಗಣ_Null() {
        m_ದತ್ತಾಂಶ.ಗಣ_Logic();
        m_ದತ್ತಾಂಶ.m_isNull = true;
        m_ದತ್ತಾಂಶ.m_autoExtend = true;
    }
    V3_ಸಂಖ್ಯೆ& ಗಣ_Single_ಬಿಟ್ಟುಗಳು(ಅಕ್ಷರ ಮೌಲ್ಯ);
    ಶೂನ್ಯ fill_ಬಿಟ್ಟುಗಳು(ಪೂರ್ಣಾಂಕ destLsb, ಪೂರ್ಣಾಂಕ ಅಗಲ, ಅಕ್ಷರ ಮೌಲ್ಯ);
    ಶೂನ್ಯ copy_ಬಿಟ್ಟುಗಳು(ಪೂರ್ಣಾಂಕ destLsb, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮೂಲ, ಪೂರ್ಣಾಂಕ ಮೂಲ_Lsb, ಪೂರ್ಣಾಂಕ ಅಗಲ);
    V3_ಸಂಖ್ಯೆ& ಗಣ_String(ಸ್ಥಿರಾಂಕ string& str) {
        m_ದತ್ತಾಂಶ.ಗಣ_String(str);
        ಹಿಂತಿರುಗಿ *this;
    }
    ಶೂನ್ಯ opCleanThis(bool warnOnTruncation = false);

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ nodep(Ast_ಗಂಟು* nodep) VL_MT_STABLE;
    Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_nodep; }  // For debug only
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_fileline; }
    V3_ಸಂಖ್ಯೆ& ಗಣ_Zero();
    V3_ಸಂಖ್ಯೆ& ಗಣ_Quad(uint64_t ಮೌಲ್ಯ);
    V3_ಸಂಖ್ಯೆ& ಗಣ_Long(uint32_t ಮೌಲ್ಯ);
    V3_ಸಂಖ್ಯೆ& ಗಣ_Long_S(int32_t ಮೌಲ್ಯ);
    V3_ಸಂಖ್ಯೆ& ಗಣ_Double(ದ್ವಿಗುಣ ಮೌಲ್ಯ);
    ಶೂನ್ಯ ಗಣ_ಬಿಟ್_X0(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2);
    ಶೂನ್ಯ ಗಣ_ಬಿಟ್(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2, ಅಕ್ಷರ ಮೌಲ್ಯ) {  // Note: must be initialized as number and pre-zeroed!
        ಆದರೆ (ಬಿಟ್_2 >= m_ದತ್ತಾಂಶ.ಅಗಲ()) ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿರಾಂಕ uint32_t ಮುಖವಾಡ = (1UL << (ಬಿಟ್_2 & 31));
        ಮೌಲ್ಯ_And_X& v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಆದರೆ (ಮೌಲ್ಯ == '0' || ಮೌಲ್ಯ == 0) {
            v.m_ಮೌಲ್ಯ &= ~ಮುಖವಾಡ;
            v.m_ಮೌಲ್ಯ_X &= ~ಮುಖವಾಡ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಮೌಲ್ಯ == '1' || ಮೌಲ್ಯ == 1) {
            v.m_ಮೌಲ್ಯ |= ಮುಖವಾಡ;
            v.m_ಮೌಲ್ಯ_X &= ~ಮುಖವಾಡ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಮೌಲ್ಯ == 'z' || ಮೌಲ್ಯ == 2) {
            v.m_ಮೌಲ್ಯ &= ~ಮುಖವಾಡ;
            v.m_ಮೌಲ್ಯ_X |= ಮುಖವಾಡ;
        } ಇಲ್ಲದಿದ್ದರೆ {  // X
            v.m_ಮೌಲ್ಯ |= ಮುಖವಾಡ;
            v.m_ಮೌಲ್ಯ_X |= ಮುಖವಾಡ;
        }
    }

ಖಾಸಗಿ:
    ಅಕ್ಷರ ಬಿಟ್_Is(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (ಬಿಟ್_2 >= m_ದತ್ತಾಂಶ.ಅಗಲ() || ಬಿಟ್_2 < 0) {
            // We never sign extend
            ಹಿಂತಿರುಗಿ '0';
        }
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಹಿಂತಿರುಗಿ ("01zx"[(((v.m_ಮೌಲ್ಯ & (1UL << (ಬಿಟ್_2 & 31))) ? 1 : 0)
                        | ((v.m_ಮೌಲ್ಯ_X & (1UL << (ಬಿಟ್_2 & 31))) ? 2 : 0))]);
    }
    ಅಕ್ಷರ ಬಿಟ್_Is_Extend(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2, ಪೂರ್ಣಾಂಕ lbits) ಸ್ಥಿರಾಂಕ {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        ಆದರೆ (ಬಿಟ್_2 < 0) ಹಿಂತಿರುಗಿ '0';
        UASSERT(lbits <= m_ದತ್ತಾಂಶ.ಅಗಲ(), "Extend of wrong size");
        ಆದರೆ (ಬಿಟ್_2 >= lbits) {
            ಬಿಟ್_2 = lbits ? lbits - 1 : 0;
            ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
            // We do sign extend
            ಹಿಂತಿರುಗಿ ("01zx"[(((v.m_ಮೌಲ್ಯ & (1UL << (ಬಿಟ್_2 & 31))) ? 1 : 0)
                            | ((v.m_ಮೌಲ್ಯ_X & (1UL << (ಬಿಟ್_2 & 31))) ? 2 : 0))]);
        }
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಹಿಂತಿರುಗಿ ("01zx"[(((v.m_ಮೌಲ್ಯ & (1UL << (ಬಿಟ್_2 & 31))) ? 1 : 0)
                        | ((v.m_ಮೌಲ್ಯ_X & (1UL << (ಬಿಟ್_2 & 31))) ? 2 : 0))]);
    }

ಸಾರ್ವಜನಿಕ:
    bool ಬಿಟ್_Is0(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆದರೆ (!is_ಸಂಖ್ಯೆ()) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 < 0) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 >= m_ದತ್ತಾಂಶ.ಅಗಲ()) ಹಿಂತಿರುಗಿ !ಬಿಟ್_Is_XZ(m_ದತ್ತಾಂಶ.ಅಗಲ() - 1);
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಹಿಂತಿರುಗಿ ((v.m_ಮೌಲ್ಯ | v.m_ಮೌಲ್ಯ_X) & (1UL << (ಬಿಟ್_2 & 31))) == 0;
    }
    bool ಬಿಟ್_Is1(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆದರೆ (!is_ಸಂಖ್ಯೆ()) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 < 0) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 >= m_ದತ್ತಾಂಶ.ಅಗಲ()) ಹಿಂತಿರುಗಿ false;
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಹಿಂತಿರುಗಿ ((v.m_ಮೌಲ್ಯ & (1UL << (ಬಿಟ್_2 & 31))) && !(v.m_ಮೌಲ್ಯ_X & (1UL << (ಬಿಟ್_2 & 31))));
    }
    bool ಬಿಟ್_Is1_Extend(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ {
        // Correct number of zero bits/width matters
        ಆದರೆ (!is_ಸಂಖ್ಯೆ()) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 < 0) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 >= m_ದತ್ತಾಂಶ.ಅಗಲ()) ಹಿಂತಿರುಗಿ ಬಿಟ್_Is1_Extend(m_ದತ್ತಾಂಶ.ಅಗಲ() - 1);
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಹಿಂತಿರುಗಿ ((v.m_ಮೌಲ್ಯ & (1UL << (ಬಿಟ್_2 & 31))) && !(v.m_ಮೌಲ್ಯ_X & (1UL << (ಬಿಟ್_2 & 31))));
    }
    bool ಬಿಟ್_Is_X(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆದರೆ (!is_ಸಂಖ್ಯೆ()) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 < 0) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 >= m_ದತ್ತಾಂಶ.ಅಗಲ()) ಹಿಂತಿರುಗಿ ಬಿಟ್_Is_Z(m_ದತ್ತಾಂಶ.ಅಗಲ() - 1);
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಹಿಂತಿರುಗಿ ((v.m_ಮೌಲ್ಯ & (1UL << (ಬಿಟ್_2 & 31))) && (v.m_ಮೌಲ್ಯ_X & (1UL << (ಬಿಟ್_2 & 31))));
    }
    bool ಬಿಟ್_Is_XZ(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆದರೆ (!is_ಸಂಖ್ಯೆ()) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 < 0) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 >= m_ದತ್ತಾಂಶ.ಅಗಲ()) ಹಿಂತಿರುಗಿ ಬಿಟ್_Is_XZ(m_ದತ್ತಾಂಶ.ಅಗಲ() - 1);
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಹಿಂತಿರುಗಿ ((v.m_ಮೌಲ್ಯ_X & (1UL << (ಬಿಟ್_2 & 31))));
    }
    bool ಬಿಟ್_Is_Z(ಪೂರ್ಣಾಂಕ ಬಿಟ್_2) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಆದರೆ (!is_ಸಂಖ್ಯೆ()) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 < 0) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (ಬಿಟ್_2 >= m_ದತ್ತಾಂಶ.ಅಗಲ()) ಹಿಂತಿರುಗಿ ಬಿಟ್_Is_Z(m_ದತ್ತಾಂಶ.ಅಗಲ() - 1);
        ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[ಬಿಟ್_2 / 32];
        ಹಿಂತಿರುಗಿ ((~v.m_ಮೌಲ್ಯ & (1UL << (ಬಿಟ್_2 & 31))) && (v.m_ಮೌಲ್ಯ_X & (1UL << (ಬಿಟ್_2 & 31))));
    }

ಖಾಸಗಿ:
    uint32_t ಬಿಟ್ಟುಗಳು_ಮೌಲ್ಯ(ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ nbits) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        uint32_t v = 0;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ bitn = 0; bitn < nbits; bitn++) v |= (ಬಿಟ್_Is1(lsb + bitn) << bitn);
        ಹಿಂತಿರುಗಿ v;
    }

    ಪೂರ್ಣಾಂಕ ಎಣಿಕೆ_X(ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ nbits) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    ಪೂರ್ಣಾಂಕ ಎಣಿಕೆ_Z(ಪೂರ್ಣಾಂಕ lsb, ಪೂರ್ಣಾಂಕ nbits) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;

    ಪೂರ್ಣಾಂಕ ಪದಗಳು() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ ((ಅಗಲ() + 31) / 32); }
    uint32_t hi_ಶಬ್ದ_ಮುಖವಾಡ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        // Correct number of zero bits/width matters
        ಹಿಂತಿರುಗಿ VL_ಮುಖವಾಡ_I(ಅಗಲ());
    }

    V3_ಸಂಖ್ಯೆ& opModDivGuts(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, bool is_modulus);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep) { init(nodep, 1); }
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಅಗಲ) {  // 0=unsized
        init(nodep, ಅಗಲ, ಅಗಲ > 0);
    }
    // Construct with value, changing to new width
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಅಗಲ, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ) {
        init(nodep, ಅಗಲ, ಅಗಲ > 0);
        op_ನಿಯೋಜಿಸು(ಮೌಲ್ಯ);
    }
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ ಅಗಲ, uint32_t ಮೌಲ್ಯ, bool sized = true) {
        init(nodep, ಅಗಲ, sized);
        m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ;
        opCleanThis();
    }
    V3_ಸಂಖ್ಯೆ(File_ಸಾಲು* flp, ಪೂರ್ಣಾಂಕ ಅಗಲ, uint32_t ಮೌಲ್ಯ, bool sized = true) {
        init(nullptr, ಅಗಲ, sized);
        m_fileline = flp;
        m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_ಸಂಖ್ಯೆ_Literal {};  // For creator type-overload selection
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, Verilog_ಸಂಖ್ಯೆ_Literal, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sourcep) { create(nodep, sourcep); }
    V3_ಸಂಖ್ಯೆ(File_ಸಾಲು* flp, Verilog_ಸಂಖ್ಯೆ_Literal, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, VerilogStringLiteral, ಸ್ಥಿರಾಂಕ string& str);
    class Double {};
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, Double, ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
        init(nodep, 64);
        ಗಣ_Double(ಮೌಲ್ಯ);
    }
    class String {};
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, String, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) {
        init(nodep);
        ಗಣ_String(ಮೌಲ್ಯ);
        m_ದತ್ತಾಂಶ.m_fromString = true;
    }
    class One_ಹೆಜ್ಜೆ {};
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, One_ಹೆಜ್ಜೆ) {
        init(nodep, 64);
        m_ದತ್ತಾಂಶ.m_is1_ಹೆಜ್ಜೆ = true;
    }
    class Null {};
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, Null) {
        init(nodep);
        ಗಣ_Null();
    }
    explicit V3_ಸಂಖ್ಯೆ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ* nump, ಪೂರ್ಣಾಂಕ ಅಗಲ = 1) {
        init(nullptr, ಅಗಲ);
        m_fileline = nump->fileline();
    }
    V3_ಸಂಖ್ಯೆ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ* nump, ಪೂರ್ಣಾಂಕ ಅಗಲ, uint32_t ಮೌಲ್ಯ) {
        init(nullptr, ಅಗಲ);
        m_ದತ್ತಾಂಶ.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* nodedtypep);

    V3_ಸಂಖ್ಯೆ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& other) = ಪೂರ್ವನಿಯೋಜಿತ;
    V3_ಸಂಖ್ಯೆ& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& other) = ಪೂರ್ವನಿಯೋಜಿತ;

    V3_ಸಂಖ್ಯೆ(V3_ಸಂಖ್ಯೆ&& other) = ಪೂರ್ವನಿಯೋಜಿತ;
    V3_ಸಂಖ್ಯೆ& ಕ್ರಿಯಾಕಾರಕ=(V3_ಸಂಖ್ಯೆ&& other) = ಪೂರ್ವನಿಯೋಜಿತ;

    ~V3_ಸಂಖ್ಯೆ() = ಪೂರ್ವನಿಯೋಜಿತ;

ಖಾಸಗಿ:
    ಶೂನ್ಯ selfTestThis();
    ಶೂನ್ಯ create(Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    ಶೂನ್ಯ create(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    ಶೂನ್ಯ create(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sourcep);
    ಶೂನ್ಯ init(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಂಕ swidth = -1, bool sized = true) {
        this->nodep(nodep);
        ಆದರೆ (swidth >= 0) {
            ಆದರೆ (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_ದತ್ತಾಂಶ.ಗಣ_Logic();
            m_ದತ್ತಾಂಶ.resize(swidth);
            m_ದತ್ತಾಂಶ.m_sized = sized;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i] = {0, 0};
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_ದತ್ತಾಂಶ.resize(1);
            m_ದತ್ತಾಂಶ.m_sized = false;
        }
    }
    ಸ್ಥಿರ string displayPad(ಗಾತ್ರ_t fmtsize, ಅಕ್ಷರ pad, bool ಎಡ_2, ಸ್ಥಿರಾಂಕ string& ಒಳ) VL_ಶುದ್ಧ;
    string displayed(ಸ್ಥಿರಾಂಕ string& vformat) ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ displayed(m_fileline, vformat);
    }
    ಶೂನ್ಯ warnTooMany(ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ);

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& sstr) ಸ್ಥಿರಾಂಕ VL_RELEASE(V3Error::s().m_mutex);
    ಶೂನ್ಯ v3error_ಕೊನೆ_Fatal(ಸ್ಥಿರಾಂಕ std::ostringstream& sstr) ಸ್ಥಿರಾಂಕ VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    ಶೂನ್ಯ ಅಗಲ(ಪೂರ್ಣಾಂಕ ಅಗಲ, bool sized = true) {
        m_ದತ್ತಾಂಶ.m_sized = sized;
        m_ದತ್ತಾಂಶ.resize(ಅಗಲ);
    }

    // SETTERS
    V3_ಸಂಖ್ಯೆ& ಗಣ_All_ಬಿಟ್ಟುಗಳು_XRemoved();
    V3_ಸಂಖ್ಯೆ& ಗಣ_All_ಬಿಟ್ಟುಗಳು_X();
    V3_ಸಂಖ್ಯೆ& ಗಣ_All_ಬಿಟ್ಟುಗಳು_Z();
    V3_ಸಂಖ್ಯೆ& ಗಣ_All_Bits0();
    V3_ಸಂಖ್ಯೆ& ಗಣ_All_Bits1();
    V3_ಸಂಖ್ಯೆ& ಗಣ_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_ಸಂಖ್ಯೆ& ಗಣ_ಮುಖವಾಡ(ಪೂರ್ಣಾಂಕ nbits, ಪೂರ್ಣಾಂಕ lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    string displayed(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& vformat,
                     ಸ್ಥಿರಾಂಕ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    string displayed(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& vformat,
                     ಸ್ಥಿರಾಂಕ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    string displayedEnum(ಸ್ಥಿರಾಂಕ AstSFormatArg* argp, ಸ್ಥಿರಾಂಕ string& vformat) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    string displayedEnumName(ಸ್ಥಿರಾಂಕ AstEnumDType* dtypep) ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಸ್ಥಿರ string displayedEnumName(ಸ್ಥಿರಾಂಕ Ast_Enum_ಅಂಶ* itemp);
    ಸ್ಥಿರ bool displayedFmtHasArg(ಅಕ್ಷರ format, bool is_ಪರಿಶೀಲನೆ);
    string emitC() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ಪೂರ್ಣಾಂಕ ಅಗಲ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.ಅಗಲ(); }
    ಪೂರ್ಣಾಂಕ ಅಗಲ_ಗಮ್ಯ_Fit() ಸ್ಥಿರಾಂಕ;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.m_sized; }
    bool autoExtend() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.m_autoExtend; }
    bool isFromString() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.m_fromString; }
    bool hasOrigParamName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.m_hasOrigParamName; }
    ಶೂನ್ಯ hasOrigParamName(bool flag) { m_ದತ್ತಾಂಶ.m_hasOrigParamName = flag; }
    V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type ದತ್ತಾಂಶ_Type() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.type(); }
    ಶೂನ್ಯ ದತ್ತಾಂಶ_Type(V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type newType) {
        ಆದರೆ (ದತ್ತಾಂಶ_Type() == newType) ಹಿಂತಿರುಗಿ;
        UASSERT(newType != V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        ಆಯ್ಕೆ (newType) {
        ಸ್ಥಿತಿ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::STRING: m_ದತ್ತಾಂಶ.ಗಣ_String(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::DOUBLE: m_ದತ್ತಾಂಶ.ಗಣ_Double(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::LOGIC: m_ದತ್ತಾಂಶ.ಗಣ_Logic(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::UNINITIALIZED: ನಿಲ್ಲಿಸು;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.m_signed; }
    ಶೂನ್ಯ isSigned(bool ssigned) { m_ದತ್ತಾಂಶ.m_signed = ssigned; }
    bool isDouble() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ ದತ್ತಾಂಶ_Type() == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::DOUBLE; }
    bool isString() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ ದತ್ತಾಂಶ_Type() == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::STRING; }
    bool isOpaque() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ isDouble() || isString(); }
    bool is_ಸಂಖ್ಯೆ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.type() == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::LOGIC
               || m_ದತ್ತಾಂಶ.type() == V3_ಸಂಖ್ಯೆ_ದತ್ತಾಂಶ_Type::DOUBLE;
    }
    bool isNegative() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        // Correct number of zero bits/width matters
        ಹಿಂತಿರುಗಿ !isString() && ಬಿಟ್_Is1(ಅಗಲ() - 1);
    }
    bool is1_ಹೆಜ್ಜೆ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.m_is1_ಹೆಜ್ಜೆ; }
    bool isNull() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ದತ್ತಾಂಶ.m_isNull; }
    bool isFourState() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    bool hasZ() ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (isString()) ಹಿಂತಿರುಗಿ false;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < ಪದಗಳು(); ++i) {
            ಸ್ಥಿರಾಂಕ ಮೌಲ್ಯ_And_X v = m_ದತ್ತಾಂಶ.ಅಂಕೆ()[i];
            ಆದರೆ ((~v.m_ಮೌಲ್ಯ) & v.m_ಮೌಲ್ಯ_X) ಹಿಂತಿರುಗಿ true;
        }
        ಹಿಂತಿರುಗಿ false;
    }
    bool isAllZ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    bool isAllX() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    bool isEqZero() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    bool isNeqZero() ಸ್ಥಿರಾಂಕ;
    bool is_ಬಿಟ್ಟುಗಳು_Zero(ಪೂರ್ಣಾಂಕ msb, ಪೂರ್ಣಾಂಕ lsb) ಸ್ಥಿರಾಂಕ;
    bool isBroken(ಪೂರ್ಣಾಂಕ vwidth) ಸ್ಥಿರಾಂಕ;
    bool isEqOne() ಸ್ಥಿರಾಂಕ;
    bool isEqAllOnes(ಪೂರ್ಣಾಂಕ optwidth = 0) ಸ್ಥಿರಾಂಕ;
    bool isCaseEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) ಸ್ಥಿರಾಂಕ;  // operator==
    bool isLtXZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) ಸ್ಥಿರಾಂಕ;  // operator< with XZ compared
    bool isAnyX() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    bool isAnyXZ() ಸ್ಥಿರಾಂಕ;
    bool isAnyZ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    bool isMsbXZ() ಸ್ಥಿರಾಂಕ {
        // Correct number of zero bits/width matters
        ಹಿಂತಿರುಗಿ ಬಿಟ್_Is_XZ(ಅಗಲ() - 1);
    }
    bool ಹೊಂದುತ್ತದೆ_ಒಳ_UInt() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    uint32_t ಗಮ್ಯ_UInt() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    int32_t ಗಮ್ಯ_SInt() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    uint64_t ಗಮ್ಯ_UQuad() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    int64_t ಗಮ್ಯ_SQuad() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    string ಗಮ್ಯ_String() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    string ಗಮ್ಯ_Decimal_S() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;  // return ASCII signed decimal number
    string ಗಮ್ಯ_Decimal_U() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string ಗಮ್ಯ_Real_String() ಸ್ಥಿರಾಂಕ VL_MT_STABLE;
    ದ್ವಿಗುಣ ಗಮ್ಯ_Double() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    V3Hash ಗಮ್ಯ_Hash() ಸ್ಥಿರಾಂಕ;
    uint32_t edata_ಶಬ್ದ(ಪೂರ್ಣಾಂಕ eword) ಸ್ಥಿರಾಂಕ;
    uint8_t ದತ್ತಾಂಶ_Byte(ಪೂರ್ಣಾಂಕ byte) ಸ್ಥಿರಾಂಕ;
    uint32_t ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl) ಸ್ಥಿರಾಂಕ;
    uint32_t ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl1, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl2, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl3) ಸ್ಥಿರಾಂಕ;
    uint32_t ಎಣಿಕೆ_Ones() ಸ್ಥಿರಾಂಕ;
    uint32_t most_ಗಣ_ಬಿಟ್_P1() ಸ್ಥಿರಾಂಕ;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t ಕನಿಷ್ಠ_ಗಣ_ಬಿಟ್_P1() ಸ್ಥಿರಾಂಕ;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ isLtXZ(rhs); }

    // STATICS
    ಸ್ಥಿರ bool epsilonEqual(ದ್ವಿಗುಣ a, ದ್ವಿಗುಣ b);  // True if number within Epsilon of the other
    ಸ್ಥಿರ bool epsilonIntegral(ದ್ವಿಗುಣ a);  // True if number rounds to integer within Epsilon
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ log2b(uint32_t ಅಂಕೆ);
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ log2bQuad(uint64_t ಅಂಕೆ);
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_ಸಂಖ್ಯೆ& op_ಬಿಟ್ಟುಗಳು_Non_XZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);  // 0/1->1, X/Z->0
    V3_ಸಂಖ್ಯೆ& op_ಬಿಟ್ಟುಗಳು_One(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);  // 1->1, 0/X/Z->0
    V3_ಸಂಖ್ಯೆ& op_ಬಿಟ್ಟುಗಳು_XZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);  // 0/1->0, X/Z->1
    V3_ಸಂಖ್ಯೆ& op_ಬಿಟ್ಟುಗಳು_Z(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);  // Z->1, 0/1/X->0
    //
    V3_ಸಂಖ್ಯೆ& op_ನಿಯೋಜಿಸು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_ನಿಯೋಜಿಸು_Non_XZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, bool ignoreXZ = true);
    V3_ಸಂಖ್ಯೆ& opExtendS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t lbits);  // Sign extension
    V3_ಸಂಖ್ಯೆ& opExtendXZ(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t lbits);  // X/Z extension
    V3_ಸಂಖ್ಯೆ& opRedOr(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRedAnd(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRedXor(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_ಎಣಿಕೆ_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಅಭಿವ್ಯಕ್ತಿ, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl1, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl2,
                          ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ctrl3);
    V3_ಸಂಖ್ಯೆ& op_ಎಣಿಕೆ_Ones(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opIsUnknown(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opOneHot(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opOneHot0(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opCLog2(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_Most_ಗಣ_ಬಿಟ್_P1(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opClean(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t ಬಿಟ್ಟುಗಳು);
    V3_ಸಂಖ್ಯೆ& opConcat(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLenN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRepl(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opRepl(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t rhsval);
    V3_ಸಂಖ್ಯೆ& op_ಹರಿವು_L(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opSel(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& msb, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lsb);
    V3_ಸಂಖ್ಯೆ& opSel(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t msbval, uint32_t lsbval);
    V3_ಸಂಖ್ಯೆ& opSelInto(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lsb, ಪೂರ್ಣಾಂಕ ಅಗಲ);
    V3_ಸಂಖ್ಯೆ& opSelInto(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಪೂರ್ಣಾಂಕ lsbval, ಪೂರ್ಣಾಂಕ ಅಗಲ);
    V3_ಸಂಖ್ಯೆ& opCvtPackString(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_ಗಮ್ಯ_ಅಧೋ_N(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_ಗಮ್ಯ_ಊರ್ಧ್ವ_N(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opCaseEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opCaseNeq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opWildEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opWildNeq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opBufIf1(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ens, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& if1s);
    V3_ಸಂಖ್ಯೆ& op_ಗಣ_Range(uint32_t lsb, uint32_t ಅಗಲ, ಅಕ್ಷರ ಬಿಟ್_ಮೌಲ್ಯ);
    // "standard" math
    V3_ಸಂಖ್ಯೆ& opNot(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opLogNot(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opLogAnd(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLogOr(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLogEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLogIf(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opNegate(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opSub(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opMul(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opMulS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opDiv(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opDivS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opModDiv(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opModDivS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opPow(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, bool lsign = false,
                    bool rsign = false);
    V3_ಸಂಖ್ಯೆ& opPowSU(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed lhs, unsigned rhs
    V3_ಸಂಖ್ಯೆ& opPowSS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed lhs, signed rhs
    V3_ಸಂಖ್ಯೆ& opPowUS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Unsigned lhs, signed rhs
    V3_ಸಂಖ್ಯೆ& opAnd(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opXor(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opOr(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& op_ಸರಿಸು_R(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& op_ಸರಿಸು_RS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_ಸಂಖ್ಯೆ& op_ಸರಿಸು_L(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    // Comparisons
    V3_ಸಂಖ್ಯೆ& opEq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opNeq(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGt(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGtS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opGte(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGteS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opLt(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLtS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opLte(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLteS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);  // Signed

    // "D" - double (aka real) math
    V3_ಸಂಖ್ಯೆ& opIToRD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, bool isSigned = false);
    V3_ಸಂಖ್ಯೆ& opISToRD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs) { ಹಿಂತಿರುಗಿ opIToRD(lhs, true); }
    V3_ಸಂಖ್ಯೆ& opRToIS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRToIRoundS(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_Real_ಗಮ್ಯ_ಬಿಟ್ಟುಗಳು(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_ಬಿಟ್ಟುಗಳು_ಗಮ್ಯ_Real_D(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opNegateD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_ಕೂಡಿಸು_D(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opSubD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opMulD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opDivD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opPowD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    // Comparisons
    V3_ಸಂಖ್ಯೆ& opEqD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opNeqD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGtD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGteD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLtD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLteD(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);

    // "N" - string operations
    V3_ಸಂಖ್ಯೆ& opAtoN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಪೂರ್ಣಾಂಕ ತಳ);
    V3_ಸಂಖ್ಯೆ& opNToI(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opPutcN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths);
    V3_ಸಂಖ್ಯೆ& opGetcN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opSubstrN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ths);
    V3_ಸಂಖ್ಯೆ& opCompareNN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs, bool ignoreCase);
    V3_ಸಂಖ್ಯೆ& opConcatN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opReplN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opReplN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, uint32_t rhsval);
    V3_ಸಂಖ್ಯೆ& opEqN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opNeqN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGtN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGteN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLtN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLteN(ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& lhs, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs);
};
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

#ಆದರೆ_ಅಂತ್ಯ  // Guard
