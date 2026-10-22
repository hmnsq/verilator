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

#ifndef VERILATOR_V3_ಸಂಖ್ಯೆ_H
#define VERILATOR_V3_ಸಂಖ್ಯೆ_H

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
class AstEnumItem;
class Ast_ಗಂಟು;
class Ast_ಗಂಟು_DType;
class AstSFormatArg;
class File_ಸಾಲು;

//============================================================================

class VFormatAttr final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : ವರ್ಣ {
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
    ಗಣನಾ en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ವರ್ಣ ascii() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    bool isComplex() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == COMPLEX; }
    bool isDouble() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == DOUBLE; }
    bool isEnum() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == SIGNED; }
    bool isString() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == STRING; }
    bool isUnsigned() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == UNSIGNED; }
};
constexpr bool operator==(ಧ್ರುವ VFormatAttr& lhs, ಧ್ರುವ VFormatAttr& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VFormatAttr& lhs, VFormatAttr::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, ಧ್ರುವ VFormatAttr& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VFormatAttr& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

//============================================================================

class V3_ಸಂಖ್ಯೆ_Data final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    ಸಂರಚನಾ ಮೌಲ್ಯ_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_ಮೌಲ್ಯ;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_ಮೌಲ್ಯ_X;
        bool operator==(ಧ್ರುವ ಮೌಲ್ಯ_And_X& other) ಧ್ರುವ VL_MT_SAFE {
            ಪ್ರತ್ಯಾಗಮ m_ಮೌಲ್ಯ == other.m_ಮೌಲ್ಯ && m_ಮೌಲ್ಯ_X == other.m_ಮೌಲ್ಯ_X;
        }
    };

    ಗಣನಾ class V3_ಸಂಖ್ಯೆ_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ಧ್ರುವ V3_ಸಂಖ್ಯೆ_Data_Type& rhs) VL_MT_SAFE {
        ವಿಕಲ್ಪ (rhs) {
        ಪಕ್ಷ V3_ಸಂಖ್ಯೆ_Data_Type::UNINITIALIZED: ಪ್ರತ್ಯಾಗಮ os << "UNINITIALIZED";
        ಪಕ್ಷ V3_ಸಂಖ್ಯೆ_Data_Type::LOGIC: ಪ್ರತ್ಯಾಗಮ os << "LOGIC";
        ಪಕ್ಷ V3_ಸಂಖ್ಯೆ_Data_Type::DOUBLE: ಪ್ರತ್ಯಾಗಮ os << "DOUBLE";
        ಪಕ್ಷ V3_ಸಂಖ್ಯೆ_Data_Type::STRING: ಪ್ರತ್ಯಾಗಮ os << "STRING";
        }
        ಪ್ರತ್ಯಾಗಮ os;
    }

ಗುಪ್ತ:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ INLINE_ಪದಗಳು = std::max(
        static_cast<size_t>(2), std::max(ಪರಿಮಾಣ(std::string) / ಪರಿಮಾಣ(ಮೌಲ್ಯ_And_X),
                                         ಪರಿಮಾಣ(std::vector<ಮೌಲ್ಯ_And_X>) / ಪರಿಮಾಣ(ಮೌಲ್ಯ_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ MAX_INLINE_WIDTH = INLINE_ಪದಗಳು * ಪರಿಮಾಣ(ಮೌಲ್ಯ_And_X) / 2 * 8;

    // MEMBERS
    ಸಙ್ಘ {
        std::array<ಮೌಲ್ಯ_And_X, INLINE_ಪದಗಳು> m_inline_ಸಂಖ್ಯೆ;
        std::vector<ಮೌಲ್ಯ_And_X> m_dynamic_ಸಂಖ್ಯೆ;
        std::string m_string;
    };

    ಪೂರ್ಣಾಙ್ಕ m_width = 0;  // Width (in bits) as specified/calculated
    V3_ಸಂಖ್ಯೆ_Data_Type m_type;

ಸಾರ್ವಜನಿಕ:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_ಸಂಖ್ಯೆ_Data()
        : m_type{V3_ಸಂಖ್ಯೆ_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_ಸಂಖ್ಯೆ_Data() { destroy_Stored_ಮೌಲ್ಯ(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_ಸಂಖ್ಯೆ_Data(ಧ್ರುವ V3_ಸಂಖ್ಯೆ_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ಯದಿ (other.is_Inline_ಸಂಖ್ಯೆ()) {
            init_Inline_ಸಂಖ್ಯೆ(other.m_inline_ಸಂಖ್ಯೆ);
        } ಅನ್ಯಥಾ ಯದಿ (other.is_Dynamic_ಸಂಖ್ಯೆ()) {
            init_Dynamic_ಸಂಖ್ಯೆ(other.m_dynamic_ಸಂಖ್ಯೆ);
        } ಅನ್ಯಥಾ ಯದಿ (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_ಸಂಖ್ಯೆ_Data& operator=(ಧ್ರುವ V3_ಸಂಖ್ಯೆ_Data& other) {
        ಯದಿ (other.is_Inline_ಸಂಖ್ಯೆ()) {
            destroy_Stored_ಮೌಲ್ಯ();
            init_Inline_ಸಂಖ್ಯೆ(other.m_inline_ಸಂಖ್ಯೆ);
        } ಅನ್ಯಥಾ ಯದಿ (other.is_Dynamic_ಸಂಖ್ಯೆ()) {
            reinit_With_Or_Assign_Dynamic_ಸಂಖ್ಯೆ(other.m_dynamic_ಸಂಖ್ಯೆ);
        } ಅನ್ಯಥಾ ಯದಿ (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } ಅನ್ಯಥಾ {
            destroy_Stored_ಮೌಲ್ಯ();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        ಪ್ರತ್ಯಾಗಮ *this;
    }

    V3_ಸಂಖ್ಯೆ_Data(V3_ಸಂಖ್ಯೆ_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ಯದಿ (other.is_Inline_ಸಂಖ್ಯೆ()) {
            init_Inline_ಸಂಖ್ಯೆ(other.m_inline_ಸಂಖ್ಯೆ);
        } ಅನ್ಯಥಾ ಯದಿ (other.is_Dynamic_ಸಂಖ್ಯೆ()) {
            init_Dynamic_ಸಂಖ್ಯೆ(std::move(other.m_dynamic_ಸಂಖ್ಯೆ));
        } ಅನ್ಯಥಾ ಯದಿ (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_ಸಂಖ್ಯೆ_Data_Type::UNINITIALIZED;
    }

    V3_ಸಂಖ್ಯೆ_Data& operator=(V3_ಸಂಖ್ಯೆ_Data&& other) {
        ಯದಿ (other.is_Inline_ಸಂಖ್ಯೆ()) {
            destroy_Stored_ಮೌಲ್ಯ();
            init_Inline_ಸಂಖ್ಯೆ(other.m_inline_ಸಂಖ್ಯೆ);
        } ಅನ್ಯಥಾ ಯದಿ (other.is_Dynamic_ಸಂಖ್ಯೆ()) {
            reinit_With_Or_Assign_Dynamic_ಸಂಖ್ಯೆ(std::move(other.m_dynamic_ಸಂಖ್ಯೆ));
        } ಅನ್ಯಥಾ ಯದಿ (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } ಅನ್ಯಥಾ {
            destroy_Stored_ಮೌಲ್ಯ();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_ಸಂಖ್ಯೆ_Data_Type::UNINITIALIZED;
        ಪ್ರತ್ಯಾಗಮ *this;
    }

    // ACCESSORS
    ಮೌಲ್ಯ_And_X* ಅಂಕೆ() {
        UASSERT(is_ಸಂಖ್ಯೆ(), "`num` member accessed when data type is " << m_type);
        ಪ್ರತ್ಯಾಗಮ is_Inline_ಸಂಖ್ಯೆ() ? m_inline_ಸಂಖ್ಯೆ.data() : m_dynamic_ಸಂಖ್ಯೆ.data();
    }
    ಧ್ರುವ ಮೌಲ್ಯ_And_X* ಅಂಕೆ() ಧ್ರುವ VL_MT_SAFE {
        UASSERT(is_ಸಂಖ್ಯೆ(), "`num` member accessed when data type is " << m_type);
        ಪ್ರತ್ಯಾಗಮ is_Inline_ಸಂಖ್ಯೆ() ? m_inline_ಸಂಖ್ಯೆ.data() : m_dynamic_ಸಂಖ್ಯೆ.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ಪ್ರತ್ಯಾಗಮ m_string;
    }
    ಧ್ರುವ std::string& str() ಧ್ರುವ VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ಪ್ರತ್ಯಾಗಮ m_string;
    }

    ಪೂರ್ಣಾಙ್ಕ width() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_width; }
    V3_ಸಂಖ್ಯೆ_Data_Type type() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_type; }

    // METHODS
    ಶೂನ್ಯ resize(ಪೂರ್ಣಾಙ್ಕ bits_ಎಣಿಕೆ) {
        ಯದಿ (m_width == bits_ಎಣಿಕೆ) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (bits_To_ಪದಗಳು(m_width) == bits_To_ಪದಗಳು(bits_ಎಣಿಕೆ)) {
            m_width = bits_ಎಣಿಕೆ;
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (is_Dynamic_ಸಂಖ್ಯೆ()) {
            ಯದಿ (bits_ಎಣಿಕೆ > MAX_INLINE_WIDTH) {
                m_dynamic_ಸಂಖ್ಯೆ.resize(bits_To_ಪದಗಳು(bits_ಎಣಿಕೆ));
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ auto dynamicBits = std::move(m_dynamic_ಸಂಖ್ಯೆ);
                destroy_Dynamic_ಸಂಖ್ಯೆ();
                init_Inline_ಸಂಖ್ಯೆ();
                std::memcpy(m_inline_ಸಂಖ್ಯೆ.data(), dynamicBits.data(), ಪರಿಮಾಣ(m_inline_ಸಂಖ್ಯೆ));
            }
        } ಅನ್ಯಥಾ ಯದಿ (is_Inline_ಸಂಖ್ಯೆ()) {
            ಯದಿ (bits_ಎಣಿಕೆ > MAX_INLINE_WIDTH) {
                ಧ್ರುವ auto bits = m_inline_ಸಂಖ್ಯೆ;
                init_Dynamic_ಸಂಖ್ಯೆ(bits_To_ಪದಗಳು(bits_ಎಣಿಕೆ));
                std::memcpy(m_dynamic_ಸಂಖ್ಯೆ.data(), bits.data(), ಪರಿಮಾಣ(bits));
            }
        }
        m_width = bits_ಎಣಿಕೆ;
    }

    ಶೂನ್ಯ setString() {
        // If there has been a string already it is kept intact.
        ಯದಿ (isString()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (is_Dynamic_ಸಂಖ್ಯೆ()) destroy_Dynamic_ಸಂಖ್ಯೆ();
        initString();
        m_type = V3_ಸಂಖ್ಯೆ_Data_Type::STRING;
    }
    ಶೂನ್ಯ setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_ಸಂಖ್ಯೆ_Data_Type::STRING;
    }
    ಶೂನ್ಯ setString(ಧ್ರುವ std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_ಸಂಖ್ಯೆ_Data_Type::STRING;
    }

    ಶೂನ್ಯ setDouble() {
        destroy_Stored_ಮೌಲ್ಯ();
        ಯದಿ (!is_Inline_ಸಂಖ್ಯೆ()) init_Inline_ಸಂಖ್ಯೆ();
        m_type = V3_ಸಂಖ್ಯೆ_Data_Type::DOUBLE;
        resize(64);
    }

    ಶೂನ್ಯ setLogic() {
        ಯದಿ (isString()) destroyString();
        ಯದಿ (!is_ಸಂಖ್ಯೆ()) {
            ಯದಿ (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_ಸಂಖ್ಯೆ();
            } ಅನ್ಯಥಾ {
                init_Dynamic_ಸಂಖ್ಯೆ(bits_To_ಪದಗಳು(m_width));
            }
        }
        m_type = V3_ಸಂಖ್ಯೆ_Data_Type::LOGIC;
        resize(m_width);
    }

ಗುಪ್ತ:
    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ bits_To_ಪದಗಳು(ಪೂರ್ಣಾಙ್ಕ bits_ಎಣಿಕೆ) VL_PURE { ಪ್ರತ್ಯಾಗಮ (bits_ಎಣಿಕೆ + 31) / 32; }

    bool is_ಸಂಖ್ಯೆ() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_type == V3_ಸಂಖ್ಯೆ_Data_Type::DOUBLE || m_type == V3_ಸಂಖ್ಯೆ_Data_Type::LOGIC;
    }
    bool is_Inline_ಸಂಖ್ಯೆ() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_ಸಂಖ್ಯೆ_Data_Type::DOUBLE || m_type == V3_ಸಂಖ್ಯೆ_Data_Type::LOGIC);
    }
    bool is_Dynamic_ಸಂಖ್ಯೆ() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (m_width > MAX_INLINE_WIDTH) && (m_type == V3_ಸಂಖ್ಯೆ_Data_Type::LOGIC);
    }
    bool isString() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_type == V3_ಸಂಖ್ಯೆ_Data_Type::STRING; }

    template <typename... Args>
    ಶೂನ್ಯ init_Inline_ಸಂಖ್ಯೆ(Args&&... args) {
        new (&m_inline_ಸಂಖ್ಯೆ) std::array<ಮೌಲ್ಯ_And_X, INLINE_ಪದಗಳು>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    ಶೂನ್ಯ init_Dynamic_ಸಂಖ್ಯೆ(Args&&... args) {
        new (&m_dynamic_ಸಂಖ್ಯೆ) std::vector<ಮೌಲ್ಯ_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    ಶೂನ್ಯ initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    ಶೂನ್ಯ destroy_Dynamic_ಸಂಖ್ಯೆ() { m_dynamic_ಸಂಖ್ಯೆ.~vector(); }
    ಶೂನ್ಯ destroyString() { m_string.~string(); }
    ಶೂನ್ಯ destroy_Stored_ಮೌಲ್ಯ() {
        ಯದಿ (isString())
            destroyString();
        ಅನ್ಯಥಾ ಯದಿ (is_Dynamic_ಸಂಖ್ಯೆ())
            destroy_Dynamic_ಸಂಖ್ಯೆ();
    }

    template <typename T>
    ಶೂನ್ಯ reinit_With_Or_Assign_Dynamic_ಸಂಖ್ಯೆ(T&& s) {
        ಯದಿ (is_Dynamic_ಸಂಖ್ಯೆ()) {
            m_dynamic_ಸಂಖ್ಯೆ = std::forward<T>(s);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (isString()) destroyString();
        init_Dynamic_ಸಂಖ್ಯೆ(std::forward<T>(s));
    }
    template <typename T>
    ಶೂನ್ಯ reinitWithOrAssignString(T&& s) {
        ಯದಿ (isString()) {
            m_string = std::forward<T>(s);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (is_Dynamic_ಸಂಖ್ಯೆ()) destroy_Dynamic_ಸಂಖ್ಯೆ();
        initString(std::forward<T>(s));
    }
};

class V3_ಸಂಖ್ಯೆ final {
    // TYPES
    using ಮೌಲ್ಯ_And_X = V3_ಸಂಖ್ಯೆ_Data::ಮೌಲ್ಯ_And_X;
    using V3_ಸಂಖ್ಯೆ_Data_Type = V3_ಸಂಖ್ಯೆ_Data::V3_ಸಂಖ್ಯೆ_Data_Type;

    // MEMBERS
    V3_ಸಂಖ್ಯೆ_Data m_data;
    Ast_ಗಂಟು* m_nodep = nullptr;  // Parent node - for error reporting only
    File_ಸಾಲು* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    ಶೂನ್ಯ setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_ಸಂಖ್ಯೆ& setSingleBits(ವರ್ಣ ಮೌಲ್ಯ);
    ಶೂನ್ಯ fillBits(ಪೂರ್ಣಾಙ್ಕ destLsb, ಪೂರ್ಣಾಙ್ಕ width, ವರ್ಣ ಮೌಲ್ಯ);
    ಶೂನ್ಯ copyBits(ಪೂರ್ಣಾಙ್ಕ destLsb, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& source, ಪೂರ್ಣಾಙ್ಕ sourceLsb, ಪೂರ್ಣಾಙ್ಕ width);
    V3_ಸಂಖ್ಯೆ& setString(ಧ್ರುವ string& str) {
        m_data.setString(str);
        ಪ್ರತ್ಯಾಗಮ *this;
    }
    ಶೂನ್ಯ opCleanThis(bool warnOnTruncation = false);

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ nodep(Ast_ಗಂಟು* nodep) VL_MT_STABLE;
    Ast_ಗಂಟು* nodep() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_nodep; }  // For debug only
    File_ಸಾಲು* fileline() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_fileline; }
    V3_ಸಂಖ್ಯೆ& setZero();
    V3_ಸಂಖ್ಯೆ& setQuad(uint64_t ಮೌಲ್ಯ);
    V3_ಸಂಖ್ಯೆ& setLong(uint32_t ಮೌಲ್ಯ);
    V3_ಸಂಖ್ಯೆ& setLongS(int32_t ಮೌಲ್ಯ);
    V3_ಸಂಖ್ಯೆ& setDouble(ದ್ವಿಗುಣ ಮೌಲ್ಯ);
    ಶೂನ್ಯ setBitX0(ಪೂರ್ಣಾಙ್ಕ bit);
    ಶೂನ್ಯ setBit(ಪೂರ್ಣಾಙ್ಕ bit, ವರ್ಣ ಮೌಲ್ಯ) {  // Note: must be initialized as number and pre-zeroed!
        ಯದಿ (bit >= m_data.width()) ಪ್ರತ್ಯಾಗಮ;
        ಧ್ರುವ uint32_t mask = (1UL << (bit & 31));
        ಮೌಲ್ಯ_And_X& v = m_data.ಅಂಕೆ()[bit / 32];
        ಯದಿ (ಮೌಲ್ಯ == '0' || ಮೌಲ್ಯ == 0) {
            v.m_ಮೌಲ್ಯ &= ~mask;
            v.m_ಮೌಲ್ಯ_X &= ~mask;
        } ಅನ್ಯಥಾ ಯದಿ (ಮೌಲ್ಯ == '1' || ಮೌಲ್ಯ == 1) {
            v.m_ಮೌಲ್ಯ |= mask;
            v.m_ಮೌಲ್ಯ_X &= ~mask;
        } ಅನ್ಯಥಾ ಯದಿ (ಮೌಲ್ಯ == 'z' || ಮೌಲ್ಯ == 2) {
            v.m_ಮೌಲ್ಯ &= ~mask;
            v.m_ಮೌಲ್ಯ_X |= mask;
        } ಅನ್ಯಥಾ {  // X
            v.m_ಮೌಲ್ಯ |= mask;
            v.m_ಮೌಲ್ಯ_X |= mask;
        }
    }

ಗುಪ್ತ:
    ವರ್ಣ bitIs(ಪೂರ್ಣಾಙ್ಕ bit) ಧ್ರುವ {
        ಯದಿ (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            ಪ್ರತ್ಯಾಗಮ '0';
        }
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
        ಪ್ರತ್ಯಾಗಮ ("01zx"[(((v.m_ಮೌಲ್ಯ & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_ಮೌಲ್ಯ_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    ವರ್ಣ bitIsExtend(ಪೂರ್ಣಾಙ್ಕ bit, ಪೂರ್ಣಾಙ್ಕ lbits) ಧ್ರುವ {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        ಯದಿ (bit < 0) ಪ್ರತ್ಯಾಗಮ '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        ಯದಿ (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
            // We do sign extend
            ಪ್ರತ್ಯಾಗಮ ("01zx"[(((v.m_ಮೌಲ್ಯ & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_ಮೌಲ್ಯ_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
        ಪ್ರತ್ಯಾಗಮ ("01zx"[(((v.m_ಮೌಲ್ಯ & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_ಮೌಲ್ಯ_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

ಸಾರ್ವಜನಿಕ:
    bool bitIs0(ಪೂರ್ಣಾಙ್ಕ bit) ಧ್ರುವ VL_MT_SAFE {
        ಯದಿ (!is_ಸಂಖ್ಯೆ()) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit < 0) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit >= m_data.width()) ಪ್ರತ್ಯಾಗಮ !bitIsXZ(m_data.width() - 1);
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
        ಪ್ರತ್ಯಾಗಮ ((v.m_ಮೌಲ್ಯ | v.m_ಮೌಲ್ಯ_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(ಪೂರ್ಣಾಙ್ಕ bit) ಧ್ರುವ VL_MT_SAFE {
        ಯದಿ (!is_ಸಂಖ್ಯೆ()) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit < 0) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit >= m_data.width()) ಪ್ರತ್ಯಾಗಮ false;
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
        ಪ್ರತ್ಯಾಗಮ ((v.m_ಮೌಲ್ಯ & (1UL << (bit & 31))) && !(v.m_ಮೌಲ್ಯ_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(ಪೂರ್ಣಾಙ್ಕ bit) ಧ್ರುವ {
        // Correct number of zero bits/width matters
        ಯದಿ (!is_ಸಂಖ್ಯೆ()) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit < 0) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit >= m_data.width()) ಪ್ರತ್ಯಾಗಮ bitIs1Extend(m_data.width() - 1);
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
        ಪ್ರತ್ಯಾಗಮ ((v.m_ಮೌಲ್ಯ & (1UL << (bit & 31))) && !(v.m_ಮೌಲ್ಯ_X & (1UL << (bit & 31))));
    }
    bool bitIsX(ಪೂರ್ಣಾಙ್ಕ bit) ಧ್ರುವ VL_MT_SAFE {
        ಯದಿ (!is_ಸಂಖ್ಯೆ()) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit < 0) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit >= m_data.width()) ಪ್ರತ್ಯಾಗಮ bitIsZ(m_data.width() - 1);
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
        ಪ್ರತ್ಯಾಗಮ ((v.m_ಮೌಲ್ಯ & (1UL << (bit & 31))) && (v.m_ಮೌಲ್ಯ_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(ಪೂರ್ಣಾಙ್ಕ bit) ಧ್ರುವ VL_MT_SAFE {
        ಯದಿ (!is_ಸಂಖ್ಯೆ()) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit < 0) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit >= m_data.width()) ಪ್ರತ್ಯಾಗಮ bitIsXZ(m_data.width() - 1);
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
        ಪ್ರತ್ಯಾಗಮ ((v.m_ಮೌಲ್ಯ_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(ಪೂರ್ಣಾಙ್ಕ bit) ಧ್ರುವ VL_MT_SAFE {
        ಯದಿ (!is_ಸಂಖ್ಯೆ()) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit < 0) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (bit >= m_data.width()) ಪ್ರತ್ಯಾಗಮ bitIsZ(m_data.width() - 1);
        ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[bit / 32];
        ಪ್ರತ್ಯಾಗಮ ((~v.m_ಮೌಲ್ಯ & (1UL << (bit & 31))) && (v.m_ಮೌಲ್ಯ_X & (1UL << (bit & 31))));
    }

ಗುಪ್ತ:
    uint32_t bits_ಮೌಲ್ಯ(ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ nbits) ಧ್ರುವ VL_MT_SAFE {
        uint32_t v = 0;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        ಪ್ರತ್ಯಾಗಮ v;
    }

    ಪೂರ್ಣಾಙ್ಕ ಎಣಿಕೆ_X(ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ nbits) ಧ್ರುವ VL_MT_SAFE;
    ಪೂರ್ಣಾಙ್ಕ ಎಣಿಕೆ_Z(ಪೂರ್ಣಾಙ್ಕ lsb, ಪೂರ್ಣಾಙ್ಕ nbits) ಧ್ರುವ VL_MT_SAFE;

    ಪೂರ್ಣಾಙ್ಕ ಪದಗಳು() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ ((width() + 31) / 32); }
    uint32_t hiWordMask() ಧ್ರುವ VL_MT_SAFE {
        // Correct number of zero bits/width matters
        ಪ್ರತ್ಯಾಗಮ VL_MASK_I(width());
    }

    V3_ಸಂಖ್ಯೆ& opModDivGuts(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, bool is_modulus);

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep) { init(nodep, 1); }
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ width, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಮೌಲ್ಯ) {
        init(nodep, width, width > 0);
        opAssign(ಮೌಲ್ಯ);
    }
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ width, uint32_t ಮೌಲ್ಯ, bool sized = true) {
        init(nodep, width, sized);
        m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ;
        opCleanThis();
    }
    V3_ಸಂಖ್ಯೆ(File_ಸಾಲು* flp, ಪೂರ್ಣಾಙ್ಕ width, uint32_t ಮೌಲ್ಯ, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_ಸಂಖ್ಯೆ_Literal {};  // For creator type-overload selection
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, Verilog_ಸಂಖ್ಯೆ_Literal, ಧ್ರುವ ವರ್ಣ* sourcep) { create(nodep, sourcep); }
    V3_ಸಂಖ್ಯೆ(File_ಸಾಲು* flp, Verilog_ಸಂಖ್ಯೆ_Literal, ಧ್ರುವ ವರ್ಣ* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, VerilogStringLiteral, ಧ್ರುವ string& str);
    class Double {};
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, Double, ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
        init(nodep, 64);
        setDouble(ಮೌಲ್ಯ);
    }
    class String {};
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, String, ಧ್ರುವ string& ಮೌಲ್ಯ) {
        init(nodep);
        setString(ಮೌಲ್ಯ);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_ಸಂಖ್ಯೆ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ* nump, ಪೂರ್ಣಾಙ್ಕ width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_ಸಂಖ್ಯೆ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ* nump, ಪೂರ್ಣಾಙ್ಕ width, uint32_t ಮೌಲ್ಯ) {
        init(nullptr, width);
        m_data.ಅಂಕೆ()[0].m_ಮೌಲ್ಯ = ಮೌಲ್ಯ;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_ಸಂಖ್ಯೆ(Ast_ಗಂಟು* nodep, ಧ್ರುವ Ast_ಗಂಟು_DType* nodedtypep);

    V3_ಸಂಖ್ಯೆ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& other) = ಸ್ವಭಾವ;
    V3_ಸಂಖ್ಯೆ& operator=(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& other) = ಸ್ವಭಾವ;

    V3_ಸಂಖ್ಯೆ(V3_ಸಂಖ್ಯೆ&& other) = ಸ್ವಭಾವ;
    V3_ಸಂಖ್ಯೆ& operator=(V3_ಸಂಖ್ಯೆ&& other) = ಸ್ವಭಾವ;

    ~V3_ಸಂಖ್ಯೆ() = ಸ್ವಭಾವ;

ಗುಪ್ತ:
    ಶೂನ್ಯ selfTestThis();
    ಶೂನ್ಯ create(Ast_ಗಂಟು* nodep, ಧ್ರುವ ವರ್ಣ* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    ಶೂನ್ಯ create(File_ಸಾಲು* flp, ಧ್ರುವ ವರ್ಣ* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    ಶೂನ್ಯ create(ಧ್ರುವ ವರ್ಣ* sourcep);
    ಶೂನ್ಯ init(Ast_ಗಂಟು* nodep, ಪೂರ್ಣಾಙ್ಕ swidth = -1, bool sized = true) {
        this->nodep(nodep);
        ಯದಿ (swidth >= 0) {
            ಯದಿ (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) m_data.ಅಂಕೆ()[i] = {0, 0};
        } ಅನ್ಯಥಾ {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    ಸ್ಥಿರ string displayPad(size_t fmtsize, ವರ್ಣ pad, bool ಎಡ, ಧ್ರುವ string& in) VL_PURE;
    string displayed(ಧ್ರುವ string& vformat) ಧ್ರುವ VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ displayed(m_fileline, vformat);
    }
    ಶೂನ್ಯ warnTooMany(ಧ್ರುವ string& ಮೌಲ್ಯ);

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ v3errorEnd(ಧ್ರುವ std::ostringstream& sstr) ಧ್ರುವ VL_RELEASE(V3Error::s().m_mutex);
    ಶೂನ್ಯ v3errorEndFatal(ಧ್ರುವ std::ostringstream& sstr) ಧ್ರುವ VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    ಶೂನ್ಯ width(ಪೂರ್ಣಾಙ್ಕ width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_ಸಂಖ್ಯೆ& setAllBitsXRemoved();
    V3_ಸಂಖ್ಯೆ& setAllBitsX();
    V3_ಸಂಖ್ಯೆ& setAllBitsZ();
    V3_ಸಂಖ್ಯೆ& setAllBits0();
    V3_ಸಂಖ್ಯೆ& setAllBits1();
    V3_ಸಂಖ್ಯೆ& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_ಸಂಖ್ಯೆ& setMask(ಪೂರ್ಣಾಙ್ಕ nbits, ಪೂರ್ಣಾಙ್ಕ lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ಧ್ರುವ VL_MT_STABLE;
    string displayed(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& vformat,
                     ಧ್ರುವ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ಧ್ರುವ VL_MT_STABLE;
    string displayed(File_ಸಾಲು* fl, ಧ್ರುವ string& vformat,
                     ಧ್ರುವ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ಧ್ರುವ VL_MT_STABLE;
    string displayedEnum(ಧ್ರುವ AstSFormatArg* argp, ಧ್ರುವ string& vformat) ಧ್ರುವ VL_MT_STABLE;
    string displayedEnumName(ಧ್ರುವ AstEnumDType* dtypep) ಧ್ರುವ VL_MT_STABLE;
    ಸ್ಥಿರ string displayedEnumName(ಧ್ರುವ AstEnumItem* itemp);
    ಸ್ಥಿರ bool displayedFmtHasArg(ವರ್ಣ format, bool isScan);
    string emitC() ಧ್ರುವ VL_MT_STABLE;
    ಪೂರ್ಣಾಙ್ಕ width() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_data.width(); }
    ಪೂರ್ಣಾಙ್ಕ widthToFit() ಧ್ರುವ;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_data.m_sized; }
    bool autoExtend() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_data.m_autoExtend; }
    bool isFromString() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_data.m_fromString; }
    bool hasOrigParamName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_data.m_hasOrigParamName; }
    ಶೂನ್ಯ hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_ಸಂಖ್ಯೆ_Data_Type dataType() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_data.type(); }
    ಶೂನ್ಯ dataType(V3_ಸಂಖ್ಯೆ_Data_Type newType) {
        ಯದಿ (dataType() == newType) ಪ್ರತ್ಯಾಗಮ;
        UASSERT(newType != V3_ಸಂಖ್ಯೆ_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        ವಿಕಲ್ಪ (newType) {
        ಪಕ್ಷ V3_ಸಂಖ್ಯೆ_Data_Type::STRING: m_data.setString(); ಭಙ್ಗ;
        ಪಕ್ಷ V3_ಸಂಖ್ಯೆ_Data_Type::DOUBLE: m_data.setDouble(); ಭಙ್ಗ;
        ಪಕ್ಷ V3_ಸಂಖ್ಯೆ_Data_Type::LOGIC: m_data.setLogic(); ಭಙ್ಗ;
        ಪಕ್ಷ V3_ಸಂಖ್ಯೆ_Data_Type::UNINITIALIZED: ಭಙ್ಗ;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_data.m_signed; }
    ಶೂನ್ಯ isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ dataType() == V3_ಸಂಖ್ಯೆ_Data_Type::DOUBLE; }
    bool isString() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ dataType() == V3_ಸಂಖ್ಯೆ_Data_Type::STRING; }
    bool isOpaque() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ isDouble() || isString(); }
    bool is_ಸಂಖ್ಯೆ() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_data.type() == V3_ಸಂಖ್ಯೆ_Data_Type::LOGIC
               || m_data.type() == V3_ಸಂಖ್ಯೆ_Data_Type::DOUBLE;
    }
    bool isNegative() ಧ್ರುವ VL_MT_SAFE {
        // Correct number of zero bits/width matters
        ಪ್ರತ್ಯಾಗಮ !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_data.m_is1Step; }
    bool isNull() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_data.m_isNull; }
    bool isFourState() ಧ್ರುವ VL_MT_SAFE;
    bool hasZ() ಧ್ರುವ {
        ಯದಿ (isString()) ಪ್ರತ್ಯಾಗಮ false;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < ಪದಗಳು(); ++i) {
            ಧ್ರುವ ಮೌಲ್ಯ_And_X v = m_data.ಅಂಕೆ()[i];
            ಯದಿ ((~v.m_ಮೌಲ್ಯ) & v.m_ಮೌಲ್ಯ_X) ಪ್ರತ್ಯಾಗಮ true;
        }
        ಪ್ರತ್ಯಾಗಮ false;
    }
    bool isAllZ() ಧ್ರುವ VL_MT_SAFE;
    bool isAllX() ಧ್ರುವ VL_MT_SAFE;
    bool isEqZero() ಧ್ರುವ VL_MT_SAFE;
    bool isNeqZero() ಧ್ರುವ;
    bool isBitsZero(ಪೂರ್ಣಾಙ್ಕ msb, ಪೂರ್ಣಾಙ್ಕ lsb) ಧ್ರುವ;
    bool isBroken(ಪೂರ್ಣಾಙ್ಕ vwidth) ಧ್ರುವ;
    bool isEqOne() ಧ್ರುವ;
    bool isEqAllOnes(ಪೂರ್ಣಾಙ್ಕ optwidth = 0) ಧ್ರುವ;
    bool isCaseEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) ಧ್ರುವ;  // operator==
    bool isLtXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) ಧ್ರುವ;  // operator< with XZ compared
    bool isAnyX() ಧ್ರುವ VL_MT_SAFE;
    bool isAnyXZ() ಧ್ರುವ;
    bool isAnyZ() ಧ್ರುವ VL_MT_SAFE;
    bool isMsbXZ() ಧ್ರುವ {
        // Correct number of zero bits/width matters
        ಪ್ರತ್ಯಾಗಮ bitIsXZ(width() - 1);
    }
    bool fitsInUInt() ಧ್ರುವ VL_MT_SAFE;
    uint32_t toUInt() ಧ್ರುವ VL_MT_SAFE;
    int32_t toSInt() ಧ್ರುವ VL_MT_SAFE;
    uint64_t toUQuad() ಧ್ರುವ VL_MT_SAFE;
    int64_t toSQuad() ಧ್ರುವ VL_MT_SAFE;
    string toString() ಧ್ರುವ VL_MT_SAFE;
    string toDecimalS() ಧ್ರುವ VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() ಧ್ರುವ VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() ಧ್ರುವ VL_MT_STABLE;
    ದ್ವಿಗುಣ toDouble() ಧ್ರುವ VL_MT_SAFE;
    V3Hash toHash() ಧ್ರುವ;
    uint32_t edataWord(ಪೂರ್ಣಾಙ್ಕ eword) ಧ್ರುವ;
    uint8_t dataByte(ಪೂರ್ಣಾಙ್ಕ byte) ಧ್ರುವ;
    uint32_t ಎಣಿಕೆ_Bits(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl) ಧ್ರುವ;
    uint32_t ಎಣಿಕೆ_Bits(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl1, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl2, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl3) ಧ್ರುವ;
    uint32_t ಎಣಿಕೆ_Ones() ಧ್ರುವ;
    uint32_t mostSetBitP1() ಧ್ರುವ;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ಧ್ರುವ;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ isLtXZ(rhs); }

    // STATICS
    ಸ್ಥಿರ bool epsilonEqual(ದ್ವಿಗುಣ a, ದ್ವಿಗುಣ b);  // True if number within Epsilon of the other
    ಸ್ಥಿರ bool epsilonIntegral(ದ್ವಿಗುಣ a);  // True if number rounds to integer within Epsilon
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ log2b(uint32_t ಅಂಕೆ);
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ log2bQuad(uint64_t ಅಂಕೆ);
    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_ಸಂಖ್ಯೆ& opBitsNonXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);  // 0/1->1, X/Z->0
    V3_ಸಂಖ್ಯೆ& opBitsOne(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);  // 1->1, 0/X/Z->0
    V3_ಸಂಖ್ಯೆ& opBitsXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);  // 0/1->0, X/Z->1
    V3_ಸಂಖ್ಯೆ& opBitsZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);  // Z->1, 0/1/X->0
    //
    V3_ಸಂಖ್ಯೆ& opAssign(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opAssignNonXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, bool ignoreXZ = true);
    V3_ಸಂಖ್ಯೆ& opExtendS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t lbits);  // Sign extension
    V3_ಸಂಖ್ಯೆ& opExtendXZ(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t lbits);  // X/Z extension
    V3_ಸಂಖ್ಯೆ& opRedOr(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRedAnd(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRedXor(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& op_ಎಣಿಕೆ_Bits(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ಅಭಿವ್ಯಕ್ತಿ, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl1, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl2,
                          ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ctrl3);
    V3_ಸಂಖ್ಯೆ& op_ಎಣಿಕೆ_Ones(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opIsUnknown(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opOneHot(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opOneHot0(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opCLog2(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opMostSetBitP1(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opClean(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t bits);
    V3_ಸಂಖ್ಯೆ& opConcat(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLenN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRepl(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opRepl(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t rhsval);
    V3_ಸಂಖ್ಯೆ& opStreamL(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opSel(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& msb, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lsb);
    V3_ಸಂಖ್ಯೆ& opSel(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t msbval, uint32_t lsbval);
    V3_ಸಂಖ್ಯೆ& opSelInto(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lsb, ಪೂರ್ಣಾಙ್ಕ width);
    V3_ಸಂಖ್ಯೆ& opSelInto(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಪೂರ್ಣಾಙ್ಕ lsbval, ಪೂರ್ಣಾಙ್ಕ width);
    V3_ಸಂಖ್ಯೆ& opCvtPackString(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opToLowerN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opToUpperN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opCaseEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opCaseNeq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opWildEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opWildNeq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opBufIf1(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ens, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& if1s);
    V3_ಸಂಖ್ಯೆ& opSetRange(uint32_t lsb, uint32_t width, ವರ್ಣ bit_ಮೌಲ್ಯ);
    // "standard" math
    V3_ಸಂಖ್ಯೆ& opNot(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opLogNot(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opLogAnd(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLogOr(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLogEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLogIf(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opNegate(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opAdd(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opSub(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opMul(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opMulS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opDiv(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opDivS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opModDiv(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opModDivS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opPow(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, bool lsign = false,
                    bool rsign = false);
    V3_ಸಂಖ್ಯೆ& opPowSU(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed lhs, unsigned rhs
    V3_ಸಂಖ್ಯೆ& opPowSS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed lhs, signed rhs
    V3_ಸಂಖ್ಯೆ& opPowUS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Unsigned lhs, signed rhs
    V3_ಸಂಖ್ಯೆ& opAnd(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opXor(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opOr(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opShiftR(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opShiftRS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_ಸಂಖ್ಯೆ& opShiftL(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    // Comparisons
    V3_ಸಂಖ್ಯೆ& opEq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opNeq(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGt(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGtS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opGte(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGteS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opLt(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLtS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed
    V3_ಸಂಖ್ಯೆ& opLte(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLteS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);  // Signed

    // "D" - double (aka real) math
    V3_ಸಂಖ್ಯೆ& opIToRD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, bool isSigned = false);
    V3_ಸಂಖ್ಯೆ& opISToRD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs) { ಪ್ರತ್ಯಾಗಮ opIToRD(lhs, true); }
    V3_ಸಂಖ್ಯೆ& opRToIS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRToIRoundS(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opRealToBits(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opBitsToRealD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opNegateD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opAddD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opSubD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opMulD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opDivD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opPowD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    // Comparisons
    V3_ಸಂಖ್ಯೆ& opEqD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opNeqD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGtD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGteD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLtD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLteD(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);

    // "N" - string operations
    V3_ಸಂಖ್ಯೆ& opAtoN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಪೂರ್ಣಾಙ್ಕ base);
    V3_ಸಂಖ್ಯೆ& opNToI(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs);
    V3_ಸಂಖ್ಯೆ& opPutcN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths);
    V3_ಸಂಖ್ಯೆ& opGetcN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opSubstrN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& ths);
    V3_ಸಂಖ್ಯೆ& opCompareNN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs, bool ignoreCase);
    V3_ಸಂಖ್ಯೆ& opConcatN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opReplN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opReplN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, uint32_t rhsval);
    V3_ಸಂಖ್ಯೆ& opEqN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opNeqN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGtN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opGteN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLtN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
    V3_ಸಂಖ್ಯೆ& opLteN(ಧ್ರುವ V3_ಸಂಖ್ಯೆ& lhs, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs);
};
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ V3_ಸಂಖ್ಯೆ& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

#endif  // Guard
