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

#اگر_غیر_تعریف_شدہ VERILATOR_V3_نمبر_H
#تعریف_کرو VERILATOR_V3_نمبر_H

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3Hash.h"
#شامل "V3StdFuture.h"

#شامل <algorithm>
#شامل <array>
#شامل <cmath>
#شامل <limits>
#شامل <vector>

class AstEnumDType;
class Ast_Enum_شے;
class Ast_گرہ;
class Ast_گرہ_DType;
class AstSFormatArg;
class File_سطر;

//============================================================================

class VFormatAttr final {
عوامی:
    شمار en : حرف {
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
    شمار en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    حرف ascii() ثابت { واپس m_e; }
    bool isComplex() ثابت { واپس m_e == COMPLEX; }
    bool isDouble() ثابت { واپس m_e == DOUBLE; }
    bool isEnum() ثابت { واپس m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ثابت { واپس m_e == SIGNED; }
    bool isString() ثابت { واپس m_e == STRING; }
    bool isUnsigned() ثابت { واپس m_e == UNSIGNED; }
};
constexpr bool عامل==(ثابت VFormatAttr& lhs, ثابت VFormatAttr& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VFormatAttr& lhs, VFormatAttr::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VFormatAttr::en lhs, ثابت VFormatAttr& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VFormatAttr& rhs) {
    واپس os << rhs.ascii();
}

//============================================================================

class V3_نمبر_ڈیٹا final {
عوامی:
    // TYPES
    ساخت قدر_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_قدر;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_قدر_X;
        bool عامل==(ثابت قدر_And_X& other) ثابت VL_MT_محفوظ {
            واپس m_قدر == other.m_قدر && m_قدر_X == other.m_قدر_X;
        }
    };

    شمار class V3_نمبر_ڈیٹا_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    دوست std::ostream& عامل<<(std::ostream& os, ثابت V3_نمبر_ڈیٹا_Type& rhs) VL_MT_محفوظ {
        انتخاب (rhs) {
        حالت V3_نمبر_ڈیٹا_Type::UNINITIALIZED: واپس os << "UNINITIALIZED";
        حالت V3_نمبر_ڈیٹا_Type::LOGIC: واپس os << "LOGIC";
        حالت V3_نمبر_ڈیٹا_Type::DOUBLE: واپس os << "DOUBLE";
        حالت V3_نمبر_ڈیٹا_Type::STRING: واپس os << "STRING";
        }
        واپس os;
    }

نجی:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ساکن constexpr صحیح INLINE_الفاظ = std::max(
        static_cast<سائز_t>(2), std::max(حجم(std::string) / حجم(قدر_And_X),
                                         حجم(std::سمتیہ<قدر_And_X>) / حجم(قدر_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ساکن constexpr صحیح MAX_INLINE_چوڑائی = INLINE_الفاظ * حجم(قدر_And_X) / 2 * 8;

    // MEMBERS
    اتحاد {
        std::array<قدر_And_X, INLINE_الفاظ> m_inline_نمبر;
        std::سمتیہ<قدر_And_X> m_dynamic_نمبر;
        std::string m_string;
    };

    صحیح m_چوڑائی = 0;  // Width (in bits) as specified/calculated
    V3_نمبر_ڈیٹا_Type m_type;

عوامی:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_قدم : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_نمبر_ڈیٹا()
        : m_type{V3_نمبر_ڈیٹا_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_قدم{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_نمبر_ڈیٹا() { destroy_Stored_قدر(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_نمبر_ڈیٹا(ثابت V3_نمبر_ڈیٹا& other)
        : m_چوڑائی{other.m_چوڑائی}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_قدم{other.m_is1_قدم}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        اگر (other.is_Inline_نمبر()) {
            init_Inline_نمبر(other.m_inline_نمبر);
        } ورنہ اگر (other.is_Dynamic_نمبر()) {
            init_Dynamic_نمبر(other.m_dynamic_نمبر);
        } ورنہ اگر (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_نمبر_ڈیٹا& عامل=(ثابت V3_نمبر_ڈیٹا& other) {
        اگر (other.is_Inline_نمبر()) {
            destroy_Stored_قدر();
            init_Inline_نمبر(other.m_inline_نمبر);
        } ورنہ اگر (other.is_Dynamic_نمبر()) {
            reinit_With_Or_تفویض_Dynamic_نمبر(other.m_dynamic_نمبر);
        } ورنہ اگر (other.isString()) {
            reinit_With_Or_تفویض_String(other.m_string);
        } ورنہ {
            destroy_Stored_قدر();
        }
        m_چوڑائی = other.m_چوڑائی;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_قدم = other.m_is1_قدم;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        واپس *this;
    }

    V3_نمبر_ڈیٹا(V3_نمبر_ڈیٹا&& other)
        : m_چوڑائی{other.m_چوڑائی}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_قدم{other.m_is1_قدم}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        اگر (other.is_Inline_نمبر()) {
            init_Inline_نمبر(other.m_inline_نمبر);
        } ورنہ اگر (other.is_Dynamic_نمبر()) {
            init_Dynamic_نمبر(std::move(other.m_dynamic_نمبر));
        } ورنہ اگر (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_نمبر_ڈیٹا_Type::UNINITIALIZED;
    }

    V3_نمبر_ڈیٹا& عامل=(V3_نمبر_ڈیٹا&& other) {
        اگر (other.is_Inline_نمبر()) {
            destroy_Stored_قدر();
            init_Inline_نمبر(other.m_inline_نمبر);
        } ورنہ اگر (other.is_Dynamic_نمبر()) {
            reinit_With_Or_تفویض_Dynamic_نمبر(std::move(other.m_dynamic_نمبر));
        } ورنہ اگر (other.isString()) {
            reinit_With_Or_تفویض_String(std::move(other.m_string));
        } ورنہ {
            destroy_Stored_قدر();
        }
        m_چوڑائی = other.m_چوڑائی;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_قدم = other.m_is1_قدم;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_نمبر_ڈیٹا_Type::UNINITIALIZED;
        واپس *this;
    }

    // ACCESSORS
    قدر_And_X* عدد() {
        UASSERT(is_نمبر(), "`num` member accessed when data type is " << m_type);
        واپس is_Inline_نمبر() ? m_inline_نمبر.ڈیٹا() : m_dynamic_نمبر.ڈیٹا();
    }
    ثابت قدر_And_X* عدد() ثابت VL_MT_محفوظ {
        UASSERT(is_نمبر(), "`num` member accessed when data type is " << m_type);
        واپس is_Inline_نمبر() ? m_inline_نمبر.ڈیٹا() : m_dynamic_نمبر.ڈیٹا();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        واپس m_string;
    }
    ثابت std::string& str() ثابت VL_MT_محفوظ {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        واپس m_string;
    }

    صحیح چوڑائی() ثابت VL_MT_محفوظ { واپس m_چوڑائی; }
    V3_نمبر_ڈیٹا_Type type() ثابت VL_MT_محفوظ { واپس m_type; }

    // METHODS
    خالی resize(صحیح بٹ_گنتی) {
        اگر (m_چوڑائی == بٹ_گنتی) واپس;
        اگر (بٹ_مقصد_الفاظ(m_چوڑائی) == بٹ_مقصد_الفاظ(بٹ_گنتی)) {
            m_چوڑائی = بٹ_گنتی;
            واپس;
        }
        اگر (is_Dynamic_نمبر()) {
            اگر (بٹ_گنتی > MAX_INLINE_چوڑائی) {
                m_dynamic_نمبر.resize(بٹ_مقصد_الفاظ(بٹ_گنتی));
            } ورنہ {
                ثابت خودکار dynamic_بٹ = std::move(m_dynamic_نمبر);
                destroy_Dynamic_نمبر();
                init_Inline_نمبر();
                std::حافظہ_نقل(m_inline_نمبر.ڈیٹا(), dynamic_بٹ.ڈیٹا(), حجم(m_inline_نمبر));
            }
        } ورنہ اگر (is_Inline_نمبر()) {
            اگر (بٹ_گنتی > MAX_INLINE_چوڑائی) {
                ثابت خودکار بٹ = m_inline_نمبر;
                init_Dynamic_نمبر(بٹ_مقصد_الفاظ(بٹ_گنتی));
                std::حافظہ_نقل(m_dynamic_نمبر.ڈیٹا(), بٹ.ڈیٹا(), حجم(بٹ));
            }
        }
        m_چوڑائی = بٹ_گنتی;
    }

    خالی سیٹ_String() {
        // If there has been a string already it is kept intact.
        اگر (isString()) واپس;
        اگر (is_Dynamic_نمبر()) destroy_Dynamic_نمبر();
        initString();
        m_type = V3_نمبر_ڈیٹا_Type::STRING;
    }
    خالی سیٹ_String(std::string&& s) {
        reinit_With_Or_تفویض_String(std::move(s));
        m_type = V3_نمبر_ڈیٹا_Type::STRING;
    }
    خالی سیٹ_String(ثابت std::string& s) {
        reinit_With_Or_تفویض_String(s);
        m_type = V3_نمبر_ڈیٹا_Type::STRING;
    }

    خالی سیٹ_Double() {
        destroy_Stored_قدر();
        اگر (!is_Inline_نمبر()) init_Inline_نمبر();
        m_type = V3_نمبر_ڈیٹا_Type::DOUBLE;
        resize(64);
    }

    خالی سیٹ_Logic() {
        اگر (isString()) destroyString();
        اگر (!is_نمبر()) {
            اگر (m_چوڑائی <= MAX_INLINE_چوڑائی) {
                init_Inline_نمبر();
            } ورنہ {
                init_Dynamic_نمبر(بٹ_مقصد_الفاظ(m_چوڑائی));
            }
        }
        m_type = V3_نمبر_ڈیٹا_Type::LOGIC;
        resize(m_چوڑائی);
    }

نجی:
    ساکن constexpr صحیح بٹ_مقصد_الفاظ(صحیح بٹ_گنتی) VL_خالص { واپس (بٹ_گنتی + 31) / 32; }

    bool is_نمبر() ثابت VL_MT_محفوظ {
        واپس m_type == V3_نمبر_ڈیٹا_Type::DOUBLE || m_type == V3_نمبر_ڈیٹا_Type::LOGIC;
    }
    bool is_Inline_نمبر() ثابت VL_MT_محفوظ {
        واپس (m_چوڑائی <= MAX_INLINE_چوڑائی)
               && (m_type == V3_نمبر_ڈیٹا_Type::DOUBLE || m_type == V3_نمبر_ڈیٹا_Type::LOGIC);
    }
    bool is_Dynamic_نمبر() ثابت VL_MT_محفوظ {
        واپس (m_چوڑائی > MAX_INLINE_چوڑائی) && (m_type == V3_نمبر_ڈیٹا_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_محفوظ { واپس m_type == V3_نمبر_ڈیٹا_Type::STRING; }

    سانچہ <typename... دلائل_2>
    خالی init_Inline_نمبر(دلائل_2&&... دلائل) {
        new (&m_inline_نمبر) std::array<قدر_And_X, INLINE_الفاظ>(std::forward<دلائل_2>(دلائل)...);
    }
    سانچہ <typename... دلائل_2>
    خالی init_Dynamic_نمبر(دلائل_2&&... دلائل) {
        new (&m_dynamic_نمبر) std::سمتیہ<قدر_And_X>(std::forward<دلائل_2>(دلائل)...);
    }
    سانچہ <typename... دلائل_2>
    خالی initString(دلائل_2&&... دلائل) {
        new (&m_string) std::string(std::forward<دلائل_2>(دلائل)...);
    }

    string formatDecimal();

    خالی destroy_Dynamic_نمبر() { m_dynamic_نمبر.~سمتیہ(); }
    خالی destroyString() { m_string.~string(); }
    خالی destroy_Stored_قدر() {
        اگر (isString())
            destroyString();
        ورنہ اگر (is_Dynamic_نمبر())
            destroy_Dynamic_نمبر();
    }

    سانچہ <typename T>
    خالی reinit_With_Or_تفویض_Dynamic_نمبر(T&& s) {
        اگر (is_Dynamic_نمبر()) {
            m_dynamic_نمبر = std::forward<T>(s);
            واپس;
        }
        اگر (isString()) destroyString();
        init_Dynamic_نمبر(std::forward<T>(s));
    }
    سانچہ <typename T>
    خالی reinit_With_Or_تفویض_String(T&& s) {
        اگر (isString()) {
            m_string = std::forward<T>(s);
            واپس;
        }
        اگر (is_Dynamic_نمبر()) destroy_Dynamic_نمبر();
        initString(std::forward<T>(s));
    }
};

class V3_نمبر final {
    // TYPES
    using قدر_And_X = V3_نمبر_ڈیٹا::قدر_And_X;
    using V3_نمبر_ڈیٹا_Type = V3_نمبر_ڈیٹا::V3_نمبر_ڈیٹا_Type;

    // MEMBERS
    V3_نمبر_ڈیٹا m_ڈیٹا;
    Ast_گرہ* m_nodep = nullptr;  // Parent node - for error reporting only
    File_سطر* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    خالی سیٹ_Null() {
        m_ڈیٹا.سیٹ_Logic();
        m_ڈیٹا.m_isNull = true;
        m_ڈیٹا.m_autoExtend = true;
    }
    V3_نمبر& سیٹ_Single_بٹ(حرف قدر);
    خالی fill_بٹ(صحیح destLsb, صحیح چوڑائی, حرف قدر);
    خالی copy_بٹ(صحیح destLsb, ثابت V3_نمبر& ماخذ, صحیح ماخذ_Lsb, صحیح چوڑائی);
    V3_نمبر& سیٹ_String(ثابت string& str) {
        m_ڈیٹا.سیٹ_String(str);
        واپس *this;
    }
    خالی opCleanThis(bool warnOnTruncation = false);

عوامی:
    خالی nodep(Ast_گرہ* nodep) VL_MT_STABLE;
    Ast_گرہ* nodep() ثابت VL_MT_محفوظ { واپس m_nodep; }  // For debug only
    File_سطر* fileline() ثابت VL_MT_محفوظ { واپس m_fileline; }
    V3_نمبر& سیٹ_Zero();
    V3_نمبر& سیٹ_Quad(uint64_t قدر);
    V3_نمبر& سیٹ_Long(uint32_t قدر);
    V3_نمبر& سیٹ_Long_S(int32_t قدر);
    V3_نمبر& سیٹ_Double(دوہرا قدر);
    خالی سیٹ_ثنائی_ہندسہ_X0(صحیح ثنائی_ہندسہ_2);
    خالی سیٹ_ثنائی_ہندسہ(صحیح ثنائی_ہندسہ_2, حرف قدر) {  // Note: must be initialized as number and pre-zeroed!
        اگر (ثنائی_ہندسہ_2 >= m_ڈیٹا.چوڑائی()) واپس;
        ثابت uint32_t نقاب = (1UL << (ثنائی_ہندسہ_2 & 31));
        قدر_And_X& v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        اگر (قدر == '0' || قدر == 0) {
            v.m_قدر &= ~نقاب;
            v.m_قدر_X &= ~نقاب;
        } ورنہ اگر (قدر == '1' || قدر == 1) {
            v.m_قدر |= نقاب;
            v.m_قدر_X &= ~نقاب;
        } ورنہ اگر (قدر == 'z' || قدر == 2) {
            v.m_قدر &= ~نقاب;
            v.m_قدر_X |= نقاب;
        } ورنہ {  // X
            v.m_قدر |= نقاب;
            v.m_قدر_X |= نقاب;
        }
    }

نجی:
    حرف ثنائی_ہندسہ_Is(صحیح ثنائی_ہندسہ_2) ثابت {
        اگر (ثنائی_ہندسہ_2 >= m_ڈیٹا.چوڑائی() || ثنائی_ہندسہ_2 < 0) {
            // We never sign extend
            واپس '0';
        }
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        واپس ("01zx"[(((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 1 : 0)
                        | ((v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 2 : 0))]);
    }
    حرف ثنائی_ہندسہ_Is_Extend(صحیح ثنائی_ہندسہ_2, صحیح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        اگر (ثنائی_ہندسہ_2 < 0) واپس '0';
        UASSERT(lbits <= m_ڈیٹا.چوڑائی(), "Extend of wrong size");
        اگر (ثنائی_ہندسہ_2 >= lbits) {
            ثنائی_ہندسہ_2 = lbits ? lbits - 1 : 0;
            ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
            // We do sign extend
            واپس ("01zx"[(((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 1 : 0)
                            | ((v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 2 : 0))]);
        }
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        واپس ("01zx"[(((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 1 : 0)
                        | ((v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 2 : 0))]);
    }

عوامی:
    bool ثنائی_ہندسہ_Is0(صحیح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        اگر (!is_نمبر()) واپس false;
        اگر (ثنائی_ہندسہ_2 < 0) واپس false;
        اگر (ثنائی_ہندسہ_2 >= m_ڈیٹا.چوڑائی()) واپس !ثنائی_ہندسہ_Is_XZ(m_ڈیٹا.چوڑائی() - 1);
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        واپس ((v.m_قدر | v.m_قدر_X) & (1UL << (ثنائی_ہندسہ_2 & 31))) == 0;
    }
    bool ثنائی_ہندسہ_Is1(صحیح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        اگر (!is_نمبر()) واپس false;
        اگر (ثنائی_ہندسہ_2 < 0) واپس false;
        اگر (ثنائی_ہندسہ_2 >= m_ڈیٹا.چوڑائی()) واپس false;
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        واپس ((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) && !(v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }
    bool ثنائی_ہندسہ_Is1_Extend(صحیح ثنائی_ہندسہ_2) ثابت {
        // Correct number of zero bits/width matters
        اگر (!is_نمبر()) واپس false;
        اگر (ثنائی_ہندسہ_2 < 0) واپس false;
        اگر (ثنائی_ہندسہ_2 >= m_ڈیٹا.چوڑائی()) واپس ثنائی_ہندسہ_Is1_Extend(m_ڈیٹا.چوڑائی() - 1);
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        واپس ((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) && !(v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }
    bool ثنائی_ہندسہ_Is_X(صحیح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        اگر (!is_نمبر()) واپس false;
        اگر (ثنائی_ہندسہ_2 < 0) واپس false;
        اگر (ثنائی_ہندسہ_2 >= m_ڈیٹا.چوڑائی()) واپس ثنائی_ہندسہ_Is_Z(m_ڈیٹا.چوڑائی() - 1);
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        واپس ((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) && (v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }
    bool ثنائی_ہندسہ_Is_XZ(صحیح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        اگر (!is_نمبر()) واپس false;
        اگر (ثنائی_ہندسہ_2 < 0) واپس false;
        اگر (ثنائی_ہندسہ_2 >= m_ڈیٹا.چوڑائی()) واپس ثنائی_ہندسہ_Is_XZ(m_ڈیٹا.چوڑائی() - 1);
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        واپس ((v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }
    bool ثنائی_ہندسہ_Is_Z(صحیح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        اگر (!is_نمبر()) واپس false;
        اگر (ثنائی_ہندسہ_2 < 0) واپس false;
        اگر (ثنائی_ہندسہ_2 >= m_ڈیٹا.چوڑائی()) واپس ثنائی_ہندسہ_Is_Z(m_ڈیٹا.چوڑائی() - 1);
        ثابت قدر_And_X v = m_ڈیٹا.عدد()[ثنائی_ہندسہ_2 / 32];
        واپس ((~v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) && (v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }

نجی:
    uint32_t بٹ_قدر(صحیح lsb, صحیح nbits) ثابت VL_MT_محفوظ {
        uint32_t v = 0;
        برائے (صحیح bitn = 0; bitn < nbits; bitn++) v |= (ثنائی_ہندسہ_Is1(lsb + bitn) << bitn);
        واپس v;
    }

    صحیح گنتی_X(صحیح lsb, صحیح nbits) ثابت VL_MT_محفوظ;
    صحیح گنتی_Z(صحیح lsb, صحیح nbits) ثابت VL_MT_محفوظ;

    صحیح الفاظ() ثابت VL_MT_محفوظ { واپس ((چوڑائی() + 31) / 32); }
    uint32_t hi_لفظ_نقاب() ثابت VL_MT_محفوظ {
        // Correct number of zero bits/width matters
        واپس VL_نقاب_I(چوڑائی());
    }

    V3_نمبر& opModDivGuts(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, bool is_modulus);

عوامی:
    // CONSTRUCTORS
    explicit V3_نمبر(Ast_گرہ* nodep) { init(nodep, 1); }
    V3_نمبر(Ast_گرہ* nodep, صحیح چوڑائی) {  // 0=unsized
        init(nodep, چوڑائی, چوڑائی > 0);
    }
    // Construct with value, changing to new width
    V3_نمبر(Ast_گرہ* nodep, صحیح چوڑائی, ثابت V3_نمبر& قدر) {
        init(nodep, چوڑائی, چوڑائی > 0);
        op_تفویض(قدر);
    }
    V3_نمبر(Ast_گرہ* nodep, صحیح چوڑائی, uint32_t قدر, bool sized = true) {
        init(nodep, چوڑائی, sized);
        m_ڈیٹا.عدد()[0].m_قدر = قدر;
        opCleanThis();
    }
    V3_نمبر(File_سطر* flp, صحیح چوڑائی, uint32_t قدر, bool sized = true) {
        init(nullptr, چوڑائی, sized);
        m_fileline = flp;
        m_ڈیٹا.عدد()[0].m_قدر = قدر;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_نمبر_Literal {};  // For creator type-overload selection
    V3_نمبر(Ast_گرہ* nodep, Verilog_نمبر_Literal, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3_نمبر(File_سطر* flp, Verilog_نمبر_Literal, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_نمبر(Ast_گرہ* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_نمبر(Ast_گرہ* nodep, Double, دوہرا قدر) {
        init(nodep, 64);
        سیٹ_Double(قدر);
    }
    class String {};
    V3_نمبر(Ast_گرہ* nodep, String, ثابت string& قدر) {
        init(nodep);
        سیٹ_String(قدر);
        m_ڈیٹا.m_fromString = true;
    }
    class One_قدم {};
    V3_نمبر(Ast_گرہ* nodep, One_قدم) {
        init(nodep, 64);
        m_ڈیٹا.m_is1_قدم = true;
    }
    class Null {};
    V3_نمبر(Ast_گرہ* nodep, Null) {
        init(nodep);
        سیٹ_Null();
    }
    explicit V3_نمبر(ثابت V3_نمبر* nump, صحیح چوڑائی = 1) {
        init(nullptr, چوڑائی);
        m_fileline = nump->fileline();
    }
    V3_نمبر(ثابت V3_نمبر* nump, صحیح چوڑائی, uint32_t قدر) {
        init(nullptr, چوڑائی);
        m_ڈیٹا.عدد()[0].m_قدر = قدر;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_نمبر(Ast_گرہ* nodep, ثابت Ast_گرہ_DType* nodedtypep);

    V3_نمبر(ثابت V3_نمبر& other) = طےشدہ;
    V3_نمبر& عامل=(ثابت V3_نمبر& other) = طےشدہ;

    V3_نمبر(V3_نمبر&& other) = طےشدہ;
    V3_نمبر& عامل=(V3_نمبر&& other) = طےشدہ;

    ~V3_نمبر() = طےشدہ;

نجی:
    خالی selfTestThis();
    خالی create(Ast_گرہ* nodep, ثابت حرف* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    خالی create(File_سطر* flp, ثابت حرف* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    خالی create(ثابت حرف* sourcep);
    خالی init(Ast_گرہ* nodep, صحیح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        اگر (swidth >= 0) {
            اگر (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_ڈیٹا.سیٹ_Logic();
            m_ڈیٹا.resize(swidth);
            m_ڈیٹا.m_sized = sized;
            برائے (صحیح i = 0; i < الفاظ(); ++i) m_ڈیٹا.عدد()[i] = {0, 0};
        } ورنہ {
            m_ڈیٹا.resize(1);
            m_ڈیٹا.m_sized = false;
        }
    }
    ساکن string displayPad(سائز_t fmtsize, حرف pad, bool بایاں, ثابت string& اندر) VL_خالص;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        واپس displayed(m_fileline, vformat);
    }
    خالی warnTooMany(ثابت string& قدر);

عوامی:
    خالی v3error_انتہا(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    خالی v3error_انتہا_Fatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    خالی چوڑائی(صحیح چوڑائی, bool sized = true) {
        m_ڈیٹا.m_sized = sized;
        m_ڈیٹا.resize(چوڑائی);
    }

    // SETTERS
    V3_نمبر& سیٹ_All_بٹ_XRemoved();
    V3_نمبر& سیٹ_All_بٹ_X();
    V3_نمبر& سیٹ_All_بٹ_Z();
    V3_نمبر& سیٹ_All_Bits0();
    V3_نمبر& سیٹ_All_Bits1();
    V3_نمبر& سیٹ_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_نمبر& سیٹ_نقاب(صحیح nbits, صحیح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت Ast_گرہ* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(File_سطر* fl, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayedEnum(ثابت AstSFormatArg* argp, ثابت string& vformat) ثابت VL_MT_STABLE;
    string displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE;
    ساکن string displayedEnumName(ثابت Ast_Enum_شے* itemp);
    ساکن bool displayedFmtHasArg(حرف format, bool is_جائزہ);
    string emitC() ثابت VL_MT_STABLE;
    صحیح چوڑائی() ثابت VL_MT_محفوظ { واپس m_ڈیٹا.چوڑائی(); }
    صحیح چوڑائی_مقصد_Fit() ثابت;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ثابت VL_MT_محفوظ { واپس m_ڈیٹا.m_sized; }
    bool autoExtend() ثابت VL_MT_محفوظ { واپس m_ڈیٹا.m_autoExtend; }
    bool isFromString() ثابت { واپس m_ڈیٹا.m_fromString; }
    bool hasOrigParamName() ثابت { واپس m_ڈیٹا.m_hasOrigParamName; }
    خالی hasOrigParamName(bool flag) { m_ڈیٹا.m_hasOrigParamName = flag; }
    V3_نمبر_ڈیٹا_Type ڈیٹا_Type() ثابت VL_MT_محفوظ { واپس m_ڈیٹا.type(); }
    خالی ڈیٹا_Type(V3_نمبر_ڈیٹا_Type newType) {
        اگر (ڈیٹا_Type() == newType) واپس;
        UASSERT(newType != V3_نمبر_ڈیٹا_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        انتخاب (newType) {
        حالت V3_نمبر_ڈیٹا_Type::STRING: m_ڈیٹا.سیٹ_String(); توڑو;
        حالت V3_نمبر_ڈیٹا_Type::DOUBLE: m_ڈیٹا.سیٹ_Double(); توڑو;
        حالت V3_نمبر_ڈیٹا_Type::LOGIC: m_ڈیٹا.سیٹ_Logic(); توڑو;
        حالت V3_نمبر_ڈیٹا_Type::UNINITIALIZED: توڑو;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_محفوظ { واپس m_ڈیٹا.m_signed; }
    خالی isSigned(bool ssigned) { m_ڈیٹا.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_محفوظ { واپس ڈیٹا_Type() == V3_نمبر_ڈیٹا_Type::DOUBLE; }
    bool isString() ثابت VL_MT_محفوظ { واپس ڈیٹا_Type() == V3_نمبر_ڈیٹا_Type::STRING; }
    bool isOpaque() ثابت VL_MT_محفوظ { واپس isDouble() || isString(); }
    bool is_نمبر() ثابت VL_MT_محفوظ {
        واپس m_ڈیٹا.type() == V3_نمبر_ڈیٹا_Type::LOGIC
               || m_ڈیٹا.type() == V3_نمبر_ڈیٹا_Type::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_محفوظ {
        // Correct number of zero bits/width matters
        واپس !isString() && ثنائی_ہندسہ_Is1(چوڑائی() - 1);
    }
    bool is1_قدم() ثابت VL_MT_محفوظ { واپس m_ڈیٹا.m_is1_قدم; }
    bool isNull() ثابت VL_MT_محفوظ { واپس m_ڈیٹا.m_isNull; }
    bool isFourState() ثابت VL_MT_محفوظ;
    bool hasZ() ثابت {
        اگر (isString()) واپس false;
        برائے (صحیح i = 0; i < الفاظ(); ++i) {
            ثابت قدر_And_X v = m_ڈیٹا.عدد()[i];
            اگر ((~v.m_قدر) & v.m_قدر_X) واپس true;
        }
        واپس false;
    }
    bool isAllZ() ثابت VL_MT_محفوظ;
    bool isAllX() ثابت VL_MT_محفوظ;
    bool isEqZero() ثابت VL_MT_محفوظ;
    bool isNeqZero() ثابت;
    bool is_بٹ_Zero(صحیح msb, صحیح lsb) ثابت;
    bool isBroken(صحیح vwidth) ثابت;
    bool isEqOne() ثابت;
    bool isEqAllOnes(صحیح optwidth = 0) ثابت;
    bool isCaseEq(ثابت V3_نمبر& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_نمبر& rhs) ثابت;  // operator< with XZ compared
    bool isAnyX() ثابت VL_MT_محفوظ;
    bool isAnyXZ() ثابت;
    bool isAnyZ() ثابت VL_MT_محفوظ;
    bool isMsbXZ() ثابت {
        // Correct number of zero bits/width matters
        واپس ثنائی_ہندسہ_Is_XZ(چوڑائی() - 1);
    }
    bool سماتا_اندر_UInt() ثابت VL_MT_محفوظ;
    uint32_t مقصد_UInt() ثابت VL_MT_محفوظ;
    int32_t مقصد_SInt() ثابت VL_MT_محفوظ;
    uint64_t مقصد_UQuad() ثابت VL_MT_محفوظ;
    int64_t مقصد_SQuad() ثابت VL_MT_محفوظ;
    string مقصد_String() ثابت VL_MT_محفوظ;
    string مقصد_Decimal_S() ثابت VL_MT_STABLE;  // return ASCII signed decimal number
    string مقصد_Decimal_U() ثابت VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string مقصد_Real_String() ثابت VL_MT_STABLE;
    دوہرا مقصد_Double() ثابت VL_MT_محفوظ;
    V3Hash مقصد_Hash() ثابت;
    uint32_t edata_لفظ(صحیح eword) ثابت;
    uint8_t ڈیٹا_Byte(صحیح byte) ثابت;
    uint32_t گنتی_بٹ(ثابت V3_نمبر& ctrl) ثابت;
    uint32_t گنتی_بٹ(ثابت V3_نمبر& ctrl1, ثابت V3_نمبر& ctrl2, ثابت V3_نمبر& ctrl3) ثابت;
    uint32_t گنتی_Ones() ثابت;
    uint32_t most_سیٹ_ثنائی_ہندسہ_P1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t کم_سے_کم_سیٹ_ثنائی_ہندسہ_P1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool عامل<(ثابت V3_نمبر& rhs) ثابت { واپس isLtXZ(rhs); }

    // STATICS
    ساکن bool epsilonEqual(دوہرا a, دوہرا b);  // True if number within Epsilon of the other
    ساکن bool epsilonIntegral(دوہرا a);  // True if number rounds to integer within Epsilon
    ساکن صحیح log2b(uint32_t عدد);
    ساکن صحیح log2bQuad(uint64_t عدد);
    ساکن خالی selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_نمبر& op_بٹ_Non_XZ(ثابت V3_نمبر& lhs);  // 0/1->1, X/Z->0
    V3_نمبر& op_بٹ_One(ثابت V3_نمبر& lhs);  // 1->1, 0/X/Z->0
    V3_نمبر& op_بٹ_XZ(ثابت V3_نمبر& lhs);  // 0/1->0, X/Z->1
    V3_نمبر& op_بٹ_Z(ثابت V3_نمبر& lhs);  // Z->1, 0/1/X->0
    //
    V3_نمبر& op_تفویض(ثابت V3_نمبر& lhs);
    V3_نمبر& op_تفویض_Non_XZ(ثابت V3_نمبر& lhs, bool ignoreXZ = true);
    V3_نمبر& opExtendS(ثابت V3_نمبر& lhs, uint32_t lbits);  // Sign extension
    V3_نمبر& opExtendXZ(ثابت V3_نمبر& lhs, uint32_t lbits);  // X/Z extension
    V3_نمبر& opRedOr(ثابت V3_نمبر& lhs);
    V3_نمبر& opRedAnd(ثابت V3_نمبر& lhs);
    V3_نمبر& opRedXor(ثابت V3_نمبر& lhs);
    V3_نمبر& op_گنتی_بٹ(ثابت V3_نمبر& اظہاریہ, ثابت V3_نمبر& ctrl1, ثابت V3_نمبر& ctrl2,
                          ثابت V3_نمبر& ctrl3);
    V3_نمبر& op_گنتی_Ones(ثابت V3_نمبر& lhs);
    V3_نمبر& opIsUnknown(ثابت V3_نمبر& lhs);
    V3_نمبر& opOneHot(ثابت V3_نمبر& lhs);
    V3_نمبر& opOneHot0(ثابت V3_نمبر& lhs);
    V3_نمبر& opCLog2(ثابت V3_نمبر& lhs);
    V3_نمبر& op_Most_سیٹ_ثنائی_ہندسہ_P1(ثابت V3_نمبر& lhs);
    V3_نمبر& opClean(ثابت V3_نمبر& lhs, uint32_t بٹ);
    V3_نمبر& opConcat(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLenN(ثابت V3_نمبر& lhs);
    V3_نمبر& opRepl(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opRepl(ثابت V3_نمبر& lhs, uint32_t rhsval);
    V3_نمبر& op_دھارا_L(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opSel(ثابت V3_نمبر& lhs, ثابت V3_نمبر& msb, ثابت V3_نمبر& lsb);
    V3_نمبر& opSel(ثابت V3_نمبر& lhs, uint32_t msbval, uint32_t lsbval);
    V3_نمبر& opSelInto(ثابت V3_نمبر& lhs, ثابت V3_نمبر& lsb, صحیح چوڑائی);
    V3_نمبر& opSelInto(ثابت V3_نمبر& lhs, صحیح lsbval, صحیح چوڑائی);
    V3_نمبر& opCvtPackString(ثابت V3_نمبر& lhs);
    V3_نمبر& op_مقصد_زیریں_N(ثابت V3_نمبر& lhs);
    V3_نمبر& op_مقصد_بالائی_N(ثابت V3_نمبر& lhs);
    V3_نمبر& opCaseEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opCaseNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opWildEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opWildNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opBufIf1(ثابت V3_نمبر& ens, ثابت V3_نمبر& if1s);
    V3_نمبر& op_سیٹ_Range(uint32_t lsb, uint32_t چوڑائی, حرف ثنائی_ہندسہ_قدر);
    // "standard" math
    V3_نمبر& opNot(ثابت V3_نمبر& lhs);
    V3_نمبر& opLogNot(ثابت V3_نمبر& lhs);
    V3_نمبر& opLogAnd(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLogOr(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLogEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLogIf(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opNegate(ثابت V3_نمبر& lhs);
    V3_نمبر& op_جوڑو(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opSub(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opMul(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opMulS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed
    V3_نمبر& opDiv(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opDivS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed
    V3_نمبر& opModDiv(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opModDivS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed
    V3_نمبر& opPow(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, bool lsign = false,
                    bool rsign = false);
    V3_نمبر& opPowSU(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed lhs, unsigned rhs
    V3_نمبر& opPowSS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed lhs, signed rhs
    V3_نمبر& opPowUS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Unsigned lhs, signed rhs
    V3_نمبر& opAnd(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opXor(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opOr(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& op_سرکاؤ_R(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& op_سرکاؤ_RS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_نمبر& op_سرکاؤ_L(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    // Comparisons
    V3_نمبر& opEq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opNeq(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opGt(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opGtS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed
    V3_نمبر& opGte(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opGteS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed
    V3_نمبر& opLt(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLtS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed
    V3_نمبر& opLte(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLteS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);  // Signed

    // "D" - double (aka real) math
    V3_نمبر& opIToRD(ثابت V3_نمبر& lhs, bool isSigned = false);
    V3_نمبر& opISToRD(ثابت V3_نمبر& lhs) { واپس opIToRD(lhs, true); }
    V3_نمبر& opRToIS(ثابت V3_نمبر& lhs);
    V3_نمبر& opRToIRoundS(ثابت V3_نمبر& lhs);
    V3_نمبر& op_Real_مقصد_بٹ(ثابت V3_نمبر& lhs);
    V3_نمبر& op_بٹ_مقصد_Real_D(ثابت V3_نمبر& lhs);
    V3_نمبر& opNegateD(ثابت V3_نمبر& lhs);
    V3_نمبر& op_جوڑو_D(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opSubD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opMulD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opDivD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opPowD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    // Comparisons
    V3_نمبر& opEqD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opNeqD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opGtD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opGteD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLtD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLteD(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);

    // "N" - string operations
    V3_نمبر& opAtoN(ثابت V3_نمبر& lhs, صحیح بنیاد);
    V3_نمبر& opNToI(ثابت V3_نمبر& lhs);
    V3_نمبر& opPutcN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, ثابت V3_نمبر& ths);
    V3_نمبر& opGetcN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opSubstrN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, ثابت V3_نمبر& ths);
    V3_نمبر& opCompareNN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, bool ignoreCase);
    V3_نمبر& opConcatN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opReplN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opReplN(ثابت V3_نمبر& lhs, uint32_t rhsval);
    V3_نمبر& opEqN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opNeqN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opGtN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opGteN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLtN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& opLteN(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
};
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت V3_نمبر& rhs) VL_MT_محفوظ {
    واپس os << rhs.ascii();
}

#اگر_ختم  // Guard
