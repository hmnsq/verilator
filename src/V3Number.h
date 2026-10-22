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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3_ਸੰਖਿਆ_H
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3_ਸੰਖਿਆ_H

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hash.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3StdFuture.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <array>
#ਅਨ੍ਤਰ੍ਭਾਵ <cmath>
#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class AstEnumDType;
class Ast_Enum_ਵਸਤੂ;
class Ast_ਗੰਢ;
class Ast_ਗੰਢ_DType;
class AstSFormatArg;
class File_ਸਤਰ;

//============================================================================

class VFormatAttr final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : ਅੱਖਰ {
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
    ਗਿਣਤੀ en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅੱਖਰ ascii() ਅਚਲ { ਵਾਪਸ m_e; }
    bool isComplex() ਅਚਲ { ਵਾਪਸ m_e == COMPLEX; }
    bool isDouble() ਅਚਲ { ਵਾਪਸ m_e == DOUBLE; }
    bool isEnum() ਅਚਲ { ਵਾਪਸ m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ਅਚਲ { ਵਾਪਸ m_e == SIGNED; }
    bool isString() ਅਚਲ { ਵਾਪਸ m_e == STRING; }
    bool isUnsigned() ਅਚਲ { ਵਾਪਸ m_e == UNSIGNED; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VFormatAttr& lhs, ਅਚਲ VFormatAttr& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VFormatAttr& lhs, VFormatAttr::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VFormatAttr::en lhs, ਅਚਲ VFormatAttr& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VFormatAttr& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

//============================================================================

class V3_ਸੰਖਿਆ_ਅੰਕੜੇ final {
ਜਨਤਕ:
    // TYPES
    ਬਣਤਰ ਮੁੱਲ_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_ਮੁੱਲ;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_ਮੁੱਲ_X;
        bool ਸੰਚਾਲਕ==(ਅਚਲ ਮੁੱਲ_And_X& other) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
            ਵਾਪਸ m_ਮੁੱਲ == other.m_ਮੁੱਲ && m_ਮੁੱਲ_X == other.m_ਮੁੱਲ_X;
        }
    };

    ਗਿਣਤੀ class V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    ਮਿੱਤਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
        ਚੋਣ (rhs) {
        ਹਾਲਤ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::UNINITIALIZED: ਵਾਪਸ os << "UNINITIALIZED";
        ਹਾਲਤ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::LOGIC: ਵਾਪਸ os << "LOGIC";
        ਹਾਲਤ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::DOUBLE: ਵਾਪਸ os << "DOUBLE";
        ਹਾਲਤ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::STRING: ਵਾਪਸ os << "STRING";
        }
        ਵਾਪਸ os;
    }

ਨਿਜੀ:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ਸਥਿਰ constexpr ਪੂਰਨ INLINE_ਸ਼ਬਦ = std::max(
        static_cast<ਮਾਪ_t>(2), std::max(ਆਕਾਰ(std::string) / ਆਕਾਰ(ਮੁੱਲ_And_X),
                                         ਆਕਾਰ(std::ਸਦਿਸ਼<ਮੁੱਲ_And_X>) / ਆਕਾਰ(ਮੁੱਲ_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ਸਥਿਰ constexpr ਪੂਰਨ MAX_INLINE_ਚੌੜਾਈ = INLINE_ਸ਼ਬਦ * ਆਕਾਰ(ਮੁੱਲ_And_X) / 2 * 8;

    // MEMBERS
    ਸੰਘ {
        std::array<ਮੁੱਲ_And_X, INLINE_ਸ਼ਬਦ> m_inline_ਸੰਖਿਆ;
        std::ਸਦਿਸ਼<ਮੁੱਲ_And_X> m_dynamic_ਸੰਖਿਆ;
        std::string m_string;
    };

    ਪੂਰਨ m_ਚੌੜਾਈ = 0;  // Width (in bits) as specified/calculated
    V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type m_type;

ਜਨਤਕ:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_ਕਦਮ : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_ਸੰਖਿਆ_ਅੰਕੜੇ()
        : m_type{V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_ਕਦਮ{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_ਸੰਖਿਆ_ਅੰਕੜੇ() { destroy_Stored_ਮੁੱਲ(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_ਸੰਖਿਆ_ਅੰਕੜੇ(ਅਚਲ V3_ਸੰਖਿਆ_ਅੰਕੜੇ& other)
        : m_ਚੌੜਾਈ{other.m_ਚੌੜਾਈ}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_ਕਦਮ{other.m_is1_ਕਦਮ}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ਜੇ (other.is_Inline_ਸੰਖਿਆ()) {
            init_Inline_ਸੰਖਿਆ(other.m_inline_ਸੰਖਿਆ);
        } ਨਹੀਂਤਾਂ ਜੇ (other.is_Dynamic_ਸੰਖਿਆ()) {
            init_Dynamic_ਸੰਖਿਆ(other.m_dynamic_ਸੰਖਿਆ);
        } ਨਹੀਂਤਾਂ ਜੇ (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_ਸੰਖਿਆ_ਅੰਕੜੇ& ਸੰਚਾਲਕ=(ਅਚਲ V3_ਸੰਖਿਆ_ਅੰਕੜੇ& other) {
        ਜੇ (other.is_Inline_ਸੰਖਿਆ()) {
            destroy_Stored_ਮੁੱਲ();
            init_Inline_ਸੰਖਿਆ(other.m_inline_ਸੰਖਿਆ);
        } ਨਹੀਂਤਾਂ ਜੇ (other.is_Dynamic_ਸੰਖਿਆ()) {
            reinit_With_Or_ਸੌਂਪੋ_Dynamic_ਸੰਖਿਆ(other.m_dynamic_ਸੰਖਿਆ);
        } ਨਹੀਂਤਾਂ ਜੇ (other.isString()) {
            reinit_With_Or_ਸੌਂਪੋ_String(other.m_string);
        } ਨਹੀਂਤਾਂ {
            destroy_Stored_ਮੁੱਲ();
        }
        m_ਚੌੜਾਈ = other.m_ਚੌੜਾਈ;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_ਕਦਮ = other.m_is1_ਕਦਮ;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        ਵਾਪਸ *this;
    }

    V3_ਸੰਖਿਆ_ਅੰਕੜੇ(V3_ਸੰਖਿਆ_ਅੰਕੜੇ&& other)
        : m_ਚੌੜਾਈ{other.m_ਚੌੜਾਈ}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_ਕਦਮ{other.m_is1_ਕਦਮ}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ਜੇ (other.is_Inline_ਸੰਖਿਆ()) {
            init_Inline_ਸੰਖਿਆ(other.m_inline_ਸੰਖਿਆ);
        } ਨਹੀਂਤਾਂ ਜੇ (other.is_Dynamic_ਸੰਖਿਆ()) {
            init_Dynamic_ਸੰਖਿਆ(std::move(other.m_dynamic_ਸੰਖਿਆ));
        } ਨਹੀਂਤਾਂ ਜੇ (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::UNINITIALIZED;
    }

    V3_ਸੰਖਿਆ_ਅੰਕੜੇ& ਸੰਚਾਲਕ=(V3_ਸੰਖਿਆ_ਅੰਕੜੇ&& other) {
        ਜੇ (other.is_Inline_ਸੰਖਿਆ()) {
            destroy_Stored_ਮੁੱਲ();
            init_Inline_ਸੰਖਿਆ(other.m_inline_ਸੰਖਿਆ);
        } ਨਹੀਂਤਾਂ ਜੇ (other.is_Dynamic_ਸੰਖਿਆ()) {
            reinit_With_Or_ਸੌਂਪੋ_Dynamic_ਸੰਖਿਆ(std::move(other.m_dynamic_ਸੰਖਿਆ));
        } ਨਹੀਂਤਾਂ ਜੇ (other.isString()) {
            reinit_With_Or_ਸੌਂਪੋ_String(std::move(other.m_string));
        } ਨਹੀਂਤਾਂ {
            destroy_Stored_ਮੁੱਲ();
        }
        m_ਚੌੜਾਈ = other.m_ਚੌੜਾਈ;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_ਕਦਮ = other.m_is1_ਕਦਮ;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::UNINITIALIZED;
        ਵਾਪਸ *this;
    }

    // ACCESSORS
    ਮੁੱਲ_And_X* ਅੰਕ() {
        UASSERT(is_ਸੰਖਿਆ(), "`num` member accessed when data type is " << m_type);
        ਵਾਪਸ is_Inline_ਸੰਖਿਆ() ? m_inline_ਸੰਖਿਆ.ਅੰਕੜੇ() : m_dynamic_ਸੰਖਿਆ.ਅੰਕੜੇ();
    }
    ਅਚਲ ਮੁੱਲ_And_X* ਅੰਕ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        UASSERT(is_ਸੰਖਿਆ(), "`num` member accessed when data type is " << m_type);
        ਵਾਪਸ is_Inline_ਸੰਖਿਆ() ? m_inline_ਸੰਖਿਆ.ਅੰਕੜੇ() : m_dynamic_ਸੰਖਿਆ.ਅੰਕੜੇ();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ਵਾਪਸ m_string;
    }
    ਅਚਲ std::string& str() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ਵਾਪਸ m_string;
    }

    ਪੂਰਨ ਚੌੜਾਈ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਚੌੜਾਈ; }
    V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type type() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_type; }

    // METHODS
    ਖਾਲੀ resize(ਪੂਰਨ ਬਿੱਟ_ਗਿਣਤੀ) {
        ਜੇ (m_ਚੌੜਾਈ == ਬਿੱਟ_ਗਿਣਤੀ) ਵਾਪਸ;
        ਜੇ (ਬਿੱਟ_ਮੰਜ਼ਿਲ_ਸ਼ਬਦ(m_ਚੌੜਾਈ) == ਬਿੱਟ_ਮੰਜ਼ਿਲ_ਸ਼ਬਦ(ਬਿੱਟ_ਗਿਣਤੀ)) {
            m_ਚੌੜਾਈ = ਬਿੱਟ_ਗਿਣਤੀ;
            ਵਾਪਸ;
        }
        ਜੇ (is_Dynamic_ਸੰਖਿਆ()) {
            ਜੇ (ਬਿੱਟ_ਗਿਣਤੀ > MAX_INLINE_ਚੌੜਾਈ) {
                m_dynamic_ਸੰਖਿਆ.resize(ਬਿੱਟ_ਮੰਜ਼ਿਲ_ਸ਼ਬਦ(ਬਿੱਟ_ਗਿਣਤੀ));
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ ਸਵੈਚਾਲਿਤ dynamic_ਬਿੱਟ = std::move(m_dynamic_ਸੰਖਿਆ);
                destroy_Dynamic_ਸੰਖਿਆ();
                init_Inline_ਸੰਖਿਆ();
                std::ਸਮ੍ਰਿਤੀ_ਨਕਲ(m_inline_ਸੰਖਿਆ.ਅੰਕੜੇ(), dynamic_ਬਿੱਟ.ਅੰਕੜੇ(), ਆਕਾਰ(m_inline_ਸੰਖਿਆ));
            }
        } ਨਹੀਂਤਾਂ ਜੇ (is_Inline_ਸੰਖਿਆ()) {
            ਜੇ (ਬਿੱਟ_ਗਿਣਤੀ > MAX_INLINE_ਚੌੜਾਈ) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ ਬਿੱਟ = m_inline_ਸੰਖਿਆ;
                init_Dynamic_ਸੰਖਿਆ(ਬਿੱਟ_ਮੰਜ਼ਿਲ_ਸ਼ਬਦ(ਬਿੱਟ_ਗਿਣਤੀ));
                std::ਸਮ੍ਰਿਤੀ_ਨਕਲ(m_dynamic_ਸੰਖਿਆ.ਅੰਕੜੇ(), ਬਿੱਟ.ਅੰਕੜੇ(), ਆਕਾਰ(ਬਿੱਟ));
            }
        }
        m_ਚੌੜਾਈ = ਬਿੱਟ_ਗਿਣਤੀ;
    }

    ਖਾਲੀ ਸੈੱਟ_String() {
        // If there has been a string already it is kept intact.
        ਜੇ (isString()) ਵਾਪਸ;
        ਜੇ (is_Dynamic_ਸੰਖਿਆ()) destroy_Dynamic_ਸੰਖਿਆ();
        initString();
        m_type = V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::STRING;
    }
    ਖਾਲੀ ਸੈੱਟ_String(std::string&& s) {
        reinit_With_Or_ਸੌਂਪੋ_String(std::move(s));
        m_type = V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::STRING;
    }
    ਖਾਲੀ ਸੈੱਟ_String(ਅਚਲ std::string& s) {
        reinit_With_Or_ਸੌਂਪੋ_String(s);
        m_type = V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::STRING;
    }

    ਖਾਲੀ ਸੈੱਟ_Double() {
        destroy_Stored_ਮੁੱਲ();
        ਜੇ (!is_Inline_ਸੰਖਿਆ()) init_Inline_ਸੰਖਿਆ();
        m_type = V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::DOUBLE;
        resize(64);
    }

    ਖਾਲੀ ਸੈੱਟ_Logic() {
        ਜੇ (isString()) destroyString();
        ਜੇ (!is_ਸੰਖਿਆ()) {
            ਜੇ (m_ਚੌੜਾਈ <= MAX_INLINE_ਚੌੜਾਈ) {
                init_Inline_ਸੰਖਿਆ();
            } ਨਹੀਂਤਾਂ {
                init_Dynamic_ਸੰਖਿਆ(ਬਿੱਟ_ਮੰਜ਼ਿਲ_ਸ਼ਬਦ(m_ਚੌੜਾਈ));
            }
        }
        m_type = V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::LOGIC;
        resize(m_ਚੌੜਾਈ);
    }

ਨਿਜੀ:
    ਸਥਿਰ constexpr ਪੂਰਨ ਬਿੱਟ_ਮੰਜ਼ਿਲ_ਸ਼ਬਦ(ਪੂਰਨ ਬਿੱਟ_ਗਿਣਤੀ) VL_ਸ਼ੁੱਧ { ਵਾਪਸ (ਬਿੱਟ_ਗਿਣਤੀ + 31) / 32; }

    bool is_ਸੰਖਿਆ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_type == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::DOUBLE || m_type == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::LOGIC;
    }
    bool is_Inline_ਸੰਖਿਆ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (m_ਚੌੜਾਈ <= MAX_INLINE_ਚੌੜਾਈ)
               && (m_type == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::DOUBLE || m_type == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::LOGIC);
    }
    bool is_Dynamic_ਸੰਖਿਆ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (m_ਚੌੜਾਈ > MAX_INLINE_ਚੌੜਾਈ) && (m_type == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::LOGIC);
    }
    bool isString() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_type == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::STRING; }

    ਸਾਂਚਾ <typename... ਦਲੀਲਾਂ_2>
    ਖਾਲੀ init_Inline_ਸੰਖਿਆ(ਦਲੀਲਾਂ_2&&... ਦਲੀਲਾਂ) {
        new (&m_inline_ਸੰਖਿਆ) std::array<ਮੁੱਲ_And_X, INLINE_ਸ਼ਬਦ>(std::forward<ਦਲੀਲਾਂ_2>(ਦਲੀਲਾਂ)...);
    }
    ਸਾਂਚਾ <typename... ਦਲੀਲਾਂ_2>
    ਖਾਲੀ init_Dynamic_ਸੰਖਿਆ(ਦਲੀਲਾਂ_2&&... ਦਲੀਲਾਂ) {
        new (&m_dynamic_ਸੰਖਿਆ) std::ਸਦਿਸ਼<ਮੁੱਲ_And_X>(std::forward<ਦਲੀਲਾਂ_2>(ਦਲੀਲਾਂ)...);
    }
    ਸਾਂਚਾ <typename... ਦਲੀਲਾਂ_2>
    ਖਾਲੀ initString(ਦਲੀਲਾਂ_2&&... ਦਲੀਲਾਂ) {
        new (&m_string) std::string(std::forward<ਦਲੀਲਾਂ_2>(ਦਲੀਲਾਂ)...);
    }

    string formatDecimal();

    ਖਾਲੀ destroy_Dynamic_ਸੰਖਿਆ() { m_dynamic_ਸੰਖਿਆ.~ਸਦਿਸ਼(); }
    ਖਾਲੀ destroyString() { m_string.~string(); }
    ਖਾਲੀ destroy_Stored_ਮੁੱਲ() {
        ਜੇ (isString())
            destroyString();
        ਨਹੀਂਤਾਂ ਜੇ (is_Dynamic_ਸੰਖਿਆ())
            destroy_Dynamic_ਸੰਖਿਆ();
    }

    ਸਾਂਚਾ <typename T>
    ਖਾਲੀ reinit_With_Or_ਸੌਂਪੋ_Dynamic_ਸੰਖਿਆ(T&& s) {
        ਜੇ (is_Dynamic_ਸੰਖਿਆ()) {
            m_dynamic_ਸੰਖਿਆ = std::forward<T>(s);
            ਵਾਪਸ;
        }
        ਜੇ (isString()) destroyString();
        init_Dynamic_ਸੰਖਿਆ(std::forward<T>(s));
    }
    ਸਾਂਚਾ <typename T>
    ਖਾਲੀ reinit_With_Or_ਸੌਂਪੋ_String(T&& s) {
        ਜੇ (isString()) {
            m_string = std::forward<T>(s);
            ਵਾਪਸ;
        }
        ਜੇ (is_Dynamic_ਸੰਖਿਆ()) destroy_Dynamic_ਸੰਖਿਆ();
        initString(std::forward<T>(s));
    }
};

class V3_ਸੰਖਿਆ final {
    // TYPES
    using ਮੁੱਲ_And_X = V3_ਸੰਖਿਆ_ਅੰਕੜੇ::ਮੁੱਲ_And_X;
    using V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type = V3_ਸੰਖਿਆ_ਅੰਕੜੇ::V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type;

    // MEMBERS
    V3_ਸੰਖਿਆ_ਅੰਕੜੇ m_ਅੰਕੜੇ;
    Ast_ਗੰਢ* m_nodep = nullptr;  // Parent node - for error reporting only
    File_ਸਤਰ* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    ਖਾਲੀ ਸੈੱਟ_Null() {
        m_ਅੰਕੜੇ.ਸੈੱਟ_Logic();
        m_ਅੰਕੜੇ.m_isNull = true;
        m_ਅੰਕੜੇ.m_autoExtend = true;
    }
    V3_ਸੰਖਿਆ& ਸੈੱਟ_Single_ਬਿੱਟ(ਅੱਖਰ ਮੁੱਲ);
    ਖਾਲੀ fill_ਬਿੱਟ(ਪੂਰਨ destLsb, ਪੂਰਨ ਚੌੜਾਈ, ਅੱਖਰ ਮੁੱਲ);
    ਖਾਲੀ copy_ਬਿੱਟ(ਪੂਰਨ destLsb, ਅਚਲ V3_ਸੰਖਿਆ& ਸਰੋਤ, ਪੂਰਨ ਸਰੋਤ_Lsb, ਪੂਰਨ ਚੌੜਾਈ);
    V3_ਸੰਖਿਆ& ਸੈੱਟ_String(ਅਚਲ string& str) {
        m_ਅੰਕੜੇ.ਸੈੱਟ_String(str);
        ਵਾਪਸ *this;
    }
    ਖਾਲੀ opCleanThis(bool warnOnTruncation = false);

ਜਨਤਕ:
    ਖਾਲੀ nodep(Ast_ਗੰਢ* nodep) VL_MT_STABLE;
    Ast_ਗੰਢ* nodep() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_nodep; }  // For debug only
    File_ਸਤਰ* fileline() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_fileline; }
    V3_ਸੰਖਿਆ& ਸੈੱਟ_Zero();
    V3_ਸੰਖਿਆ& ਸੈੱਟ_Quad(uint64_t ਮੁੱਲ);
    V3_ਸੰਖਿਆ& ਸੈੱਟ_Long(uint32_t ਮੁੱਲ);
    V3_ਸੰਖਿਆ& ਸੈੱਟ_Long_S(int32_t ਮੁੱਲ);
    V3_ਸੰਖਿਆ& ਸੈੱਟ_Double(ਦੁੱਗਣਾ ਮੁੱਲ);
    ਖਾਲੀ ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_X0(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2);
    ਖਾਲੀ ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਅੱਖਰ ਮੁੱਲ) {  // Note: must be initialized as number and pre-zeroed!
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= m_ਅੰਕੜੇ.ਚੌੜਾਈ()) ਵਾਪਸ;
        ਅਚਲ uint32_t ਨਕਾਬ = (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31));
        ਮੁੱਲ_And_X& v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਜੇ (ਮੁੱਲ == '0' || ਮੁੱਲ == 0) {
            v.m_ਮੁੱਲ &= ~ਨਕਾਬ;
            v.m_ਮੁੱਲ_X &= ~ਨਕਾਬ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਮੁੱਲ == '1' || ਮੁੱਲ == 1) {
            v.m_ਮੁੱਲ |= ਨਕਾਬ;
            v.m_ਮੁੱਲ_X &= ~ਨਕਾਬ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਮੁੱਲ == 'z' || ਮੁੱਲ == 2) {
            v.m_ਮੁੱਲ &= ~ਨਕਾਬ;
            v.m_ਮੁੱਲ_X |= ਨਕਾਬ;
        } ਨਹੀਂਤਾਂ {  // X
            v.m_ਮੁੱਲ |= ਨਕਾਬ;
            v.m_ਮੁੱਲ_X |= ਨਕਾਬ;
        }
    }

ਨਿਜੀ:
    ਅੱਖਰ ਦੋਆਧਾਰੀ_ਅੰਕ_Is(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ {
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= m_ਅੰਕੜੇ.ਚੌੜਾਈ() || ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0) {
            // We never sign extend
            ਵਾਪਸ '0';
        }
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਵਾਪਸ ("01zx"[(((v.m_ਮੁੱਲ & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) ? 1 : 0)
                        | ((v.m_ਮੁੱਲ_X & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) ? 2 : 0))]);
    }
    ਅੱਖਰ ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Extend(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2, ਪੂਰਨ lbits) ਅਚਲ {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0) ਵਾਪਸ '0';
        UASSERT(lbits <= m_ਅੰਕੜੇ.ਚੌੜਾਈ(), "Extend of wrong size");
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= lbits) {
            ਦੋਆਧਾਰੀ_ਅੰਕ_2 = lbits ? lbits - 1 : 0;
            ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
            // We do sign extend
            ਵਾਪਸ ("01zx"[(((v.m_ਮੁੱਲ & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) ? 1 : 0)
                            | ((v.m_ਮੁੱਲ_X & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) ? 2 : 0))]);
        }
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਵਾਪਸ ("01zx"[(((v.m_ਮੁੱਲ & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) ? 1 : 0)
                        | ((v.m_ਮੁੱਲ_X & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) ? 2 : 0))]);
    }

ਜਨਤਕ:
    bool ਦੋਆਧਾਰੀ_ਅੰਕ_Is0(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਜੇ (!is_ਸੰਖਿਆ()) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= m_ਅੰਕੜੇ.ਚੌੜਾਈ()) ਵਾਪਸ !ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(m_ਅੰਕੜੇ.ਚੌੜਾਈ() - 1);
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਵਾਪਸ ((v.m_ਮੁੱਲ | v.m_ਮੁੱਲ_X) & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) == 0;
    }
    bool ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਜੇ (!is_ਸੰਖਿਆ()) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= m_ਅੰਕੜੇ.ਚੌੜਾਈ()) ਵਾਪਸ false;
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਵਾਪਸ ((v.m_ਮੁੱਲ & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) && !(v.m_ਮੁੱਲ_X & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))));
    }
    bool ਦੋਆਧਾਰੀ_ਅੰਕ_Is1_Extend(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ {
        // Correct number of zero bits/width matters
        ਜੇ (!is_ਸੰਖਿਆ()) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= m_ਅੰਕੜੇ.ਚੌੜਾਈ()) ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_Is1_Extend(m_ਅੰਕੜੇ.ਚੌੜਾਈ() - 1);
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਵਾਪਸ ((v.m_ਮੁੱਲ & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) && !(v.m_ਮੁੱਲ_X & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))));
    }
    bool ਦੋਆਧਾਰੀ_ਅੰਕ_Is_X(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਜੇ (!is_ਸੰਖਿਆ()) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= m_ਅੰਕੜੇ.ਚੌੜਾਈ()) ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(m_ਅੰਕੜੇ.ਚੌੜਾਈ() - 1);
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਵਾਪਸ ((v.m_ਮੁੱਲ & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) && (v.m_ਮੁੱਲ_X & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))));
    }
    bool ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਜੇ (!is_ਸੰਖਿਆ()) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= m_ਅੰਕੜੇ.ਚੌੜਾਈ()) ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(m_ਅੰਕੜੇ.ਚੌੜਾਈ() - 1);
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਵਾਪਸ ((v.m_ਮੁੱਲ_X & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))));
    }
    bool ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਜੇ (!is_ਸੰਖਿਆ()) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0) ਵਾਪਸ false;
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= m_ਅੰਕੜੇ.ਚੌੜਾਈ()) ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_Is_Z(m_ਅੰਕੜੇ.ਚੌੜਾਈ() - 1);
        ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[ਦੋਆਧਾਰੀ_ਅੰਕ_2 / 32];
        ਵਾਪਸ ((~v.m_ਮੁੱਲ & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))) && (v.m_ਮੁੱਲ_X & (1UL << (ਦੋਆਧਾਰੀ_ਅੰਕ_2 & 31))));
    }

ਨਿਜੀ:
    uint32_t ਬਿੱਟ_ਮੁੱਲ(ਪੂਰਨ lsb, ਪੂਰਨ nbits) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        uint32_t v = 0;
        ਲਈ (ਪੂਰਨ bitn = 0; bitn < nbits; bitn++) v |= (ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(lsb + bitn) << bitn);
        ਵਾਪਸ v;
    }

    ਪੂਰਨ ਗਿਣਤੀ_X(ਪੂਰਨ lsb, ਪੂਰਨ nbits) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    ਪੂਰਨ ਗਿਣਤੀ_Z(ਪੂਰਨ lsb, ਪੂਰਨ nbits) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;

    ਪੂਰਨ ਸ਼ਬਦ_2() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ ((ਚੌੜਾਈ() + 31) / 32); }
    uint32_t hi_ਸ਼ਬਦ_ਨਕਾਬ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        // Correct number of zero bits/width matters
        ਵਾਪਸ VL_ਨਕਾਬ_I(ਚੌੜਾਈ());
    }

    V3_ਸੰਖਿਆ& opModDivGuts(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, bool is_modulus);

ਜਨਤਕ:
    // CONSTRUCTORS
    explicit V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep) { init(nodep, 1); }
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, ਪੂਰਨ ਚੌੜਾਈ) {  // 0=unsized
        init(nodep, ਚੌੜਾਈ, ਚੌੜਾਈ > 0);
    }
    // Construct with value, changing to new width
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, ਪੂਰਨ ਚੌੜਾਈ, ਅਚਲ V3_ਸੰਖਿਆ& ਮੁੱਲ) {
        init(nodep, ਚੌੜਾਈ, ਚੌੜਾਈ > 0);
        op_ਸੌਂਪੋ(ਮੁੱਲ);
    }
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, ਪੂਰਨ ਚੌੜਾਈ, uint32_t ਮੁੱਲ, bool sized = true) {
        init(nodep, ਚੌੜਾਈ, sized);
        m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = ਮੁੱਲ;
        opCleanThis();
    }
    V3_ਸੰਖਿਆ(File_ਸਤਰ* flp, ਪੂਰਨ ਚੌੜਾਈ, uint32_t ਮੁੱਲ, bool sized = true) {
        init(nullptr, ਚੌੜਾਈ, sized);
        m_fileline = flp;
        m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = ਮੁੱਲ;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_ਸੰਖਿਆ_Literal {};  // For creator type-overload selection
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, Verilog_ਸੰਖਿਆ_Literal, ਅਚਲ ਅੱਖਰ* sourcep) { create(nodep, sourcep); }
    V3_ਸੰਖਿਆ(File_ਸਤਰ* flp, Verilog_ਸੰਖਿਆ_Literal, ਅਚਲ ਅੱਖਰ* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, VerilogStringLiteral, ਅਚਲ string& str);
    class Double {};
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, Double, ਦੁੱਗਣਾ ਮੁੱਲ) {
        init(nodep, 64);
        ਸੈੱਟ_Double(ਮੁੱਲ);
    }
    class String {};
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, String, ਅਚਲ string& ਮੁੱਲ) {
        init(nodep);
        ਸੈੱਟ_String(ਮੁੱਲ);
        m_ਅੰਕੜੇ.m_fromString = true;
    }
    class One_ਕਦਮ {};
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, One_ਕਦਮ) {
        init(nodep, 64);
        m_ਅੰਕੜੇ.m_is1_ਕਦਮ = true;
    }
    class Null {};
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, Null) {
        init(nodep);
        ਸੈੱਟ_Null();
    }
    explicit V3_ਸੰਖਿਆ(ਅਚਲ V3_ਸੰਖਿਆ* nump, ਪੂਰਨ ਚੌੜਾਈ = 1) {
        init(nullptr, ਚੌੜਾਈ);
        m_fileline = nump->fileline();
    }
    V3_ਸੰਖਿਆ(ਅਚਲ V3_ਸੰਖਿਆ* nump, ਪੂਰਨ ਚੌੜਾਈ, uint32_t ਮੁੱਲ) {
        init(nullptr, ਚੌੜਾਈ);
        m_ਅੰਕੜੇ.ਅੰਕ()[0].m_ਮੁੱਲ = ਮੁੱਲ;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_ਸੰਖਿਆ(Ast_ਗੰਢ* nodep, ਅਚਲ Ast_ਗੰਢ_DType* nodedtypep);

    V3_ਸੰਖਿਆ(ਅਚਲ V3_ਸੰਖਿਆ& other) = ਮੂਲ;
    V3_ਸੰਖਿਆ& ਸੰਚਾਲਕ=(ਅਚਲ V3_ਸੰਖਿਆ& other) = ਮੂਲ;

    V3_ਸੰਖਿਆ(V3_ਸੰਖਿਆ&& other) = ਮੂਲ;
    V3_ਸੰਖਿਆ& ਸੰਚਾਲਕ=(V3_ਸੰਖਿਆ&& other) = ਮੂਲ;

    ~V3_ਸੰਖਿਆ() = ਮੂਲ;

ਨਿਜੀ:
    ਖਾਲੀ selfTestThis();
    ਖਾਲੀ create(Ast_ਗੰਢ* nodep, ਅਚਲ ਅੱਖਰ* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    ਖਾਲੀ create(File_ਸਤਰ* flp, ਅਚਲ ਅੱਖਰ* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    ਖਾਲੀ create(ਅਚਲ ਅੱਖਰ* sourcep);
    ਖਾਲੀ init(Ast_ਗੰਢ* nodep, ਪੂਰਨ swidth = -1, bool sized = true) {
        this->nodep(nodep);
        ਜੇ (swidth >= 0) {
            ਜੇ (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_ਅੰਕੜੇ.ਸੈੱਟ_Logic();
            m_ਅੰਕੜੇ.resize(swidth);
            m_ਅੰਕੜੇ.m_sized = sized;
            ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) m_ਅੰਕੜੇ.ਅੰਕ()[i] = {0, 0};
        } ਨਹੀਂਤਾਂ {
            m_ਅੰਕੜੇ.resize(1);
            m_ਅੰਕੜੇ.m_sized = false;
        }
    }
    ਸਥਿਰ string displayPad(ਮਾਪ_t fmtsize, ਅੱਖਰ pad, bool ਖੱਬਾ, ਅਚਲ string& ਵਿੱਚ_2) VL_ਸ਼ੁੱਧ;
    string displayed(ਅਚਲ string& vformat) ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ displayed(m_fileline, vformat);
    }
    ਖਾਲੀ warnTooMany(ਅਚਲ string& ਮੁੱਲ);

ਜਨਤਕ:
    ਖਾਲੀ v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& sstr) ਅਚਲ VL_RELEASE(V3Error::s().m_mutex);
    ਖਾਲੀ v3error_ਸਿਰਾ_Fatal(ਅਚਲ std::ostringstream& sstr) ਅਚਲ VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    ਖਾਲੀ ਚੌੜਾਈ(ਪੂਰਨ ਚੌੜਾਈ, bool sized = true) {
        m_ਅੰਕੜੇ.m_sized = sized;
        m_ਅੰਕੜੇ.resize(ਚੌੜਾਈ);
    }

    // SETTERS
    V3_ਸੰਖਿਆ& ਸੈੱਟ_All_ਬਿੱਟ_XRemoved();
    V3_ਸੰਖਿਆ& ਸੈੱਟ_All_ਬਿੱਟ_X();
    V3_ਸੰਖਿਆ& ਸੈੱਟ_All_ਬਿੱਟ_Z();
    V3_ਸੰਖਿਆ& ਸੈੱਟ_All_Bits0();
    V3_ਸੰਖਿਆ& ਸੈੱਟ_All_Bits1();
    V3_ਸੰਖਿਆ& ਸੈੱਟ_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_ਸੰਖਿਆ& ਸੈੱਟ_ਨਕਾਬ(ਪੂਰਨ nbits, ਪੂਰਨ lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ਅਚਲ VL_MT_STABLE;
    string displayed(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& vformat,
                     ਅਚਲ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ਅਚਲ VL_MT_STABLE;
    string displayed(File_ਸਤਰ* fl, ਅਚਲ string& vformat,
                     ਅਚਲ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ਅਚਲ VL_MT_STABLE;
    string displayedEnum(ਅਚਲ AstSFormatArg* argp, ਅਚਲ string& vformat) ਅਚਲ VL_MT_STABLE;
    string displayedEnumName(ਅਚਲ AstEnumDType* dtypep) ਅਚਲ VL_MT_STABLE;
    ਸਥਿਰ string displayedEnumName(ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp);
    ਸਥਿਰ bool displayedFmtHasArg(ਅੱਖਰ format, bool is_ਜਾਂਚ);
    string emitC() ਅਚਲ VL_MT_STABLE;
    ਪੂਰਨ ਚੌੜਾਈ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅੰਕੜੇ.ਚੌੜਾਈ(); }
    ਪੂਰਨ ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit() ਅਚਲ;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅੰਕੜੇ.m_sized; }
    bool autoExtend() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅੰਕੜੇ.m_autoExtend; }
    bool isFromString() ਅਚਲ { ਵਾਪਸ m_ਅੰਕੜੇ.m_fromString; }
    bool hasOrigParamName() ਅਚਲ { ਵਾਪਸ m_ਅੰਕੜੇ.m_hasOrigParamName; }
    ਖਾਲੀ hasOrigParamName(bool flag) { m_ਅੰਕੜੇ.m_hasOrigParamName = flag; }
    V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type ਅੰਕੜੇ_Type() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅੰਕੜੇ.type(); }
    ਖਾਲੀ ਅੰਕੜੇ_Type(V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type newType) {
        ਜੇ (ਅੰਕੜੇ_Type() == newType) ਵਾਪਸ;
        UASSERT(newType != V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        ਚੋਣ (newType) {
        ਹਾਲਤ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::STRING: m_ਅੰਕੜੇ.ਸੈੱਟ_String(); ਰੋਕੋ;
        ਹਾਲਤ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::DOUBLE: m_ਅੰਕੜੇ.ਸੈੱਟ_Double(); ਰੋਕੋ;
        ਹਾਲਤ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::LOGIC: m_ਅੰਕੜੇ.ਸੈੱਟ_Logic(); ਰੋਕੋ;
        ਹਾਲਤ V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::UNINITIALIZED: ਰੋਕੋ;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅੰਕੜੇ.m_signed; }
    ਖਾਲੀ isSigned(bool ssigned) { m_ਅੰਕੜੇ.m_signed = ssigned; }
    bool isDouble() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ ਅੰਕੜੇ_Type() == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::DOUBLE; }
    bool isString() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ ਅੰਕੜੇ_Type() == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::STRING; }
    bool isOpaque() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ isDouble() || isString(); }
    bool is_ਸੰਖਿਆ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_ਅੰਕੜੇ.type() == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::LOGIC
               || m_ਅੰਕੜੇ.type() == V3_ਸੰਖਿਆ_ਅੰਕੜੇ_Type::DOUBLE;
    }
    bool isNegative() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        // Correct number of zero bits/width matters
        ਵਾਪਸ !isString() && ਦੋਆਧਾਰੀ_ਅੰਕ_Is1(ਚੌੜਾਈ() - 1);
    }
    bool is1_ਕਦਮ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅੰਕੜੇ.m_is1_ਕਦਮ; }
    bool isNull() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅੰਕੜੇ.m_isNull; }
    bool isFourState() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    bool hasZ() ਅਚਲ {
        ਜੇ (isString()) ਵਾਪਸ false;
        ਲਈ (ਪੂਰਨ i = 0; i < ਸ਼ਬਦ_2(); ++i) {
            ਅਚਲ ਮੁੱਲ_And_X v = m_ਅੰਕੜੇ.ਅੰਕ()[i];
            ਜੇ ((~v.m_ਮੁੱਲ) & v.m_ਮੁੱਲ_X) ਵਾਪਸ true;
        }
        ਵਾਪਸ false;
    }
    bool isAllZ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    bool isAllX() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    bool isEqZero() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    bool isNeqZero() ਅਚਲ;
    bool is_ਬਿੱਟ_Zero(ਪੂਰਨ msb, ਪੂਰਨ lsb) ਅਚਲ;
    bool isBroken(ਪੂਰਨ vwidth) ਅਚਲ;
    bool isEqOne() ਅਚਲ;
    bool isEqAllOnes(ਪੂਰਨ optwidth = 0) ਅਚਲ;
    bool isCaseEq(ਅਚਲ V3_ਸੰਖਿਆ& rhs) ਅਚਲ;  // operator==
    bool isLtXZ(ਅਚਲ V3_ਸੰਖਿਆ& rhs) ਅਚਲ;  // operator< with XZ compared
    bool isAnyX() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    bool isAnyXZ() ਅਚਲ;
    bool isAnyZ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    bool isMsbXZ() ਅਚਲ {
        // Correct number of zero bits/width matters
        ਵਾਪਸ ਦੋਆਧਾਰੀ_ਅੰਕ_Is_XZ(ਚੌੜਾਈ() - 1);
    }
    bool ਸਮਾਉਂਦਾ_ਵਿੱਚ_UInt() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    uint32_t ਮੰਜ਼ਿਲ_UInt() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    int32_t ਮੰਜ਼ਿਲ_SInt() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    uint64_t ਮੰਜ਼ਿਲ_UQuad() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    int64_t ਮੰਜ਼ਿਲ_SQuad() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    string ਮੰਜ਼ਿਲ_String() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    string ਮੰਜ਼ਿਲ_Decimal_S() ਅਚਲ VL_MT_STABLE;  // return ASCII signed decimal number
    string ਮੰਜ਼ਿਲ_Decimal_U() ਅਚਲ VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string ਮੰਜ਼ਿਲ_Real_String() ਅਚਲ VL_MT_STABLE;
    ਦੁੱਗਣਾ ਮੰਜ਼ਿਲ_Double() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    V3Hash ਮੰਜ਼ਿਲ_Hash() ਅਚਲ;
    uint32_t edata_ਸ਼ਬਦ(ਪੂਰਨ eword) ਅਚਲ;
    uint8_t ਅੰਕੜੇ_Byte(ਪੂਰਨ byte) ਅਚਲ;
    uint32_t ਗਿਣਤੀ_ਬਿੱਟ(ਅਚਲ V3_ਸੰਖਿਆ& ctrl) ਅਚਲ;
    uint32_t ਗਿਣਤੀ_ਬਿੱਟ(ਅਚਲ V3_ਸੰਖਿਆ& ctrl1, ਅਚਲ V3_ਸੰਖਿਆ& ctrl2, ਅਚਲ V3_ਸੰਖਿਆ& ctrl3) ਅਚਲ;
    uint32_t ਗਿਣਤੀ_Ones() ਅਚਲ;
    uint32_t most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() ਅਚਲ;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t ਘੱਟ_ਤੋਂ_ਘੱਟ_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() ਅਚਲ;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool ਸੰਚਾਲਕ<(ਅਚਲ V3_ਸੰਖਿਆ& rhs) ਅਚਲ { ਵਾਪਸ isLtXZ(rhs); }

    // STATICS
    ਸਥਿਰ bool epsilonEqual(ਦੁੱਗਣਾ a, ਦੁੱਗਣਾ b);  // True if number within Epsilon of the other
    ਸਥਿਰ bool epsilonIntegral(ਦੁੱਗਣਾ a);  // True if number rounds to integer within Epsilon
    ਸਥਿਰ ਪੂਰਨ log2b(uint32_t ਅੰਕ);
    ਸਥਿਰ ਪੂਰਨ log2bQuad(uint64_t ਅੰਕ);
    ਸਥਿਰ ਖਾਲੀ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_ਸੰਖਿਆ& op_ਬਿੱਟ_Non_XZ(ਅਚਲ V3_ਸੰਖਿਆ& lhs);  // 0/1->1, X/Z->0
    V3_ਸੰਖਿਆ& op_ਬਿੱਟ_One(ਅਚਲ V3_ਸੰਖਿਆ& lhs);  // 1->1, 0/X/Z->0
    V3_ਸੰਖਿਆ& op_ਬਿੱਟ_XZ(ਅਚਲ V3_ਸੰਖਿਆ& lhs);  // 0/1->0, X/Z->1
    V3_ਸੰਖਿਆ& op_ਬਿੱਟ_Z(ਅਚਲ V3_ਸੰਖਿਆ& lhs);  // Z->1, 0/1/X->0
    //
    V3_ਸੰਖਿਆ& op_ਸੌਂਪੋ(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_ਸੌਂਪੋ_Non_XZ(ਅਚਲ V3_ਸੰਖਿਆ& lhs, bool ignoreXZ = true);
    V3_ਸੰਖਿਆ& opExtendS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t lbits);  // Sign extension
    V3_ਸੰਖਿਆ& opExtendXZ(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t lbits);  // X/Z extension
    V3_ਸੰਖਿਆ& opRedOr(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opRedAnd(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opRedXor(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_ਗਿਣਤੀ_ਬਿੱਟ(ਅਚਲ V3_ਸੰਖਿਆ& ਵਿਅੰਜਕ, ਅਚਲ V3_ਸੰਖਿਆ& ctrl1, ਅਚਲ V3_ਸੰਖਿਆ& ctrl2,
                          ਅਚਲ V3_ਸੰਖਿਆ& ctrl3);
    V3_ਸੰਖਿਆ& op_ਗਿਣਤੀ_Ones(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opIsUnknown(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opOneHot(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opOneHot0(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opCLog2(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_Most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opClean(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t ਬਿੱਟ);
    V3_ਸੰਖਿਆ& opConcat(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLenN(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opRepl(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opRepl(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t rhsval);
    V3_ਸੰਖਿਆ& op_ਧਾਰਾ_L(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opSel(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& msb, ਅਚਲ V3_ਸੰਖਿਆ& lsb);
    V3_ਸੰਖਿਆ& opSel(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t msbval, uint32_t lsbval);
    V3_ਸੰਖਿਆ& opSelInto(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& lsb, ਪੂਰਨ ਚੌੜਾਈ);
    V3_ਸੰਖਿਆ& opSelInto(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਪੂਰਨ lsbval, ਪੂਰਨ ਚੌੜਾਈ);
    V3_ਸੰਖਿਆ& opCvtPackString(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opCaseEq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opCaseNeq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opWildEq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opWildNeq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opBufIf1(ਅਚਲ V3_ਸੰਖਿਆ& ens, ਅਚਲ V3_ਸੰਖਿਆ& if1s);
    V3_ਸੰਖਿਆ& op_ਸੈੱਟ_Range(uint32_t lsb, uint32_t ਚੌੜਾਈ, ਅੱਖਰ ਦੋਆਧਾਰੀ_ਅੰਕ_ਮੁੱਲ);
    // "standard" math
    V3_ਸੰਖਿਆ& opNot(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opLogNot(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opLogAnd(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLogOr(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLogEq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLogIf(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opNegate(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_ਜੋੜੋ(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opSub(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opMul(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opMulS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed
    V3_ਸੰਖਿਆ& opDiv(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opDivS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed
    V3_ਸੰਖਿਆ& opModDiv(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opModDivS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed
    V3_ਸੰਖਿਆ& opPow(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, bool lsign = false,
                    bool rsign = false);
    V3_ਸੰਖਿਆ& opPowSU(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed lhs, unsigned rhs
    V3_ਸੰਖਿਆ& opPowSS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed lhs, signed rhs
    V3_ਸੰਖਿਆ& opPowUS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Unsigned lhs, signed rhs
    V3_ਸੰਖਿਆ& opAnd(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opXor(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opOr(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& op_ਖਿਸਕਾਓ_R(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& op_ਖਿਸਕਾਓ_RS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_ਸੰਖਿਆ& op_ਖਿਸਕਾਓ_L(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    // Comparisons
    V3_ਸੰਖਿਆ& opEq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opNeq(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opGt(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opGtS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed
    V3_ਸੰਖਿਆ& opGte(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opGteS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed
    V3_ਸੰਖਿਆ& opLt(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLtS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed
    V3_ਸੰਖਿਆ& opLte(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLteS(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);  // Signed

    // "D" - double (aka real) math
    V3_ਸੰਖਿਆ& opIToRD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, bool isSigned = false);
    V3_ਸੰਖਿਆ& opISToRD(ਅਚਲ V3_ਸੰਖਿਆ& lhs) { ਵਾਪਸ opIToRD(lhs, true); }
    V3_ਸੰਖਿਆ& opRToIS(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opRToIRoundS(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opNegateD(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& op_ਜੋੜੋ_D(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opSubD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opMulD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opDivD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opPowD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    // Comparisons
    V3_ਸੰਖਿਆ& opEqD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opNeqD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opGtD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opGteD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLtD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLteD(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);

    // "N" - string operations
    V3_ਸੰਖਿਆ& opAtoN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਪੂਰਨ ਅਧਾਰ);
    V3_ਸੰਖਿਆ& opNToI(ਅਚਲ V3_ਸੰਖਿਆ& lhs);
    V3_ਸੰਖਿਆ& opPutcN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, ਅਚਲ V3_ਸੰਖਿਆ& ths);
    V3_ਸੰਖਿਆ& opGetcN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opSubstrN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, ਅਚਲ V3_ਸੰਖਿਆ& ths);
    V3_ਸੰਖਿਆ& opCompareNN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs, bool ignoreCase);
    V3_ਸੰਖਿਆ& opConcatN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opReplN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opReplN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, uint32_t rhsval);
    V3_ਸੰਖਿਆ& opEqN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opNeqN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opGtN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opGteN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLtN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
    V3_ਸੰਖਿਆ& opLteN(ਅਚਲ V3_ਸੰਖਿਆ& lhs, ਅਚਲ V3_ਸੰਖਿਆ& rhs);
};
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ V3_ਸੰਖਿਆ& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ os << rhs.ascii();
}

#ਜੇ_ਅੰਤ  // Guard
