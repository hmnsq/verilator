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

#جے_بنا_تعریف VERILATOR_V3_نمبر_H
#تعریف_کرو VERILATOR_V3_نمبر_H

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3Hash.h"
#ضمن "V3StdFuture.h"

#ضمن <algorithm>
#ضمن <array>
#ضمن <cmath>
#ضمن <limits>
#ضمن <vector>

class AstEnumDType;
class Ast_Enum_شے;
class Ast_گرہ;
class Ast_گرہ_DType;
class AstSFormatArg;
class File_سطر;

//============================================================================

class VFormatAttr final {
عام:
    تعداد en : حرف {
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
    تعداد en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    حرف ascii() ثابت { أرجع m_e; }
    bool isComplex() ثابت { أرجع m_e == COMPLEX; }
    bool isDouble() ثابت { أرجع m_e == DOUBLE; }
    bool isEnum() ثابت { أرجع m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ثابت { أرجع m_e == SIGNED; }
    bool isString() ثابت { أرجع m_e == STRING; }
    bool isUnsigned() ثابت { أرجع m_e == UNSIGNED; }
};
constexpr bool عامل==(ثابت VFormatAttr& lhs, ثابت VFormatAttr& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VFormatAttr& lhs, VFormatAttr::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(VFormatAttr::en lhs, ثابت VFormatAttr& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت VFormatAttr& rhs) {
    أرجع os << rhs.ascii();
}

//============================================================================

class V3_نمبر_انکڑے final {
عام:
    // TYPES
    بنية قدر_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_قدر;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_قدر_X;
        bool عامل==(ثابت قدر_And_X& other) ثابت VL_MT_محفوظ {
            أرجع m_قدر == other.m_قدر && m_قدر_X == other.m_قدر_X;
        }
    };

    تعداد class V3_نمبر_انکڑے_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    یار std::ostream& عامل<<(std::ostream& os, ثابت V3_نمبر_انکڑے_Type& rhs) VL_MT_محفوظ {
        بدل (rhs) {
        حالة V3_نمبر_انکڑے_Type::UNINITIALIZED: أرجع os << "UNINITIALIZED";
        حالة V3_نمبر_انکڑے_Type::LOGIC: أرجع os << "LOGIC";
        حالة V3_نمبر_انکڑے_Type::DOUBLE: أرجع os << "DOUBLE";
        حالة V3_نمبر_انکڑے_Type::STRING: أرجع os << "STRING";
        }
        أرجع os;
    }

خاص:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ساكن constexpr صحيح INLINE_الفاظ = std::max(
        static_cast<ناپ_t>(2), std::max(حجم(std::string) / حجم(قدر_And_X),
                                         حجم(std::سمتیہ<قدر_And_X>) / حجم(قدر_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ساكن constexpr صحيح MAX_INLINE_چوڑائی = INLINE_الفاظ * حجم(قدر_And_X) / 2 * 8;

    // MEMBERS
    اتحاد {
        std::array<قدر_And_X, INLINE_الفاظ> m_inline_نمبر;
        std::سمتیہ<قدر_And_X> m_dynamic_نمبر;
        std::string m_string;
    };

    صحيح m_چوڑائی = 0;  // Width (in bits) as specified/calculated
    V3_نمبر_انکڑے_Type m_type;

عام:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_قدم : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_نمبر_انکڑے()
        : m_type{V3_نمبر_انکڑے_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_قدم{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_نمبر_انکڑے() { destroy_Stored_قدر(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_نمبر_انکڑے(ثابت V3_نمبر_انکڑے& other)
        : m_چوڑائی{other.m_چوڑائی}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_قدم{other.m_is1_قدم}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_نمبر()) {
            init_Inline_نمبر(other.m_inline_نمبر);
        } وإلا إذا (other.is_Dynamic_نمبر()) {
            init_Dynamic_نمبر(other.m_dynamic_نمبر);
        } وإلا إذا (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_نمبر_انکڑے& عامل=(ثابت V3_نمبر_انکڑے& other) {
        إذا (other.is_Inline_نمبر()) {
            destroy_Stored_قدر();
            init_Inline_نمبر(other.m_inline_نمبر);
        } وإلا إذا (other.is_Dynamic_نمبر()) {
            reinit_With_Or_سونپو_Dynamic_نمبر(other.m_dynamic_نمبر);
        } وإلا إذا (other.isString()) {
            reinit_With_Or_سونپو_String(other.m_string);
        } وإلا {
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
        أرجع *this;
    }

    V3_نمبر_انکڑے(V3_نمبر_انکڑے&& other)
        : m_چوڑائی{other.m_چوڑائی}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_قدم{other.m_is1_قدم}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_نمبر()) {
            init_Inline_نمبر(other.m_inline_نمبر);
        } وإلا إذا (other.is_Dynamic_نمبر()) {
            init_Dynamic_نمبر(std::move(other.m_dynamic_نمبر));
        } وإلا إذا (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_نمبر_انکڑے_Type::UNINITIALIZED;
    }

    V3_نمبر_انکڑے& عامل=(V3_نمبر_انکڑے&& other) {
        إذا (other.is_Inline_نمبر()) {
            destroy_Stored_قدر();
            init_Inline_نمبر(other.m_inline_نمبر);
        } وإلا إذا (other.is_Dynamic_نمبر()) {
            reinit_With_Or_سونپو_Dynamic_نمبر(std::move(other.m_dynamic_نمبر));
        } وإلا إذا (other.isString()) {
            reinit_With_Or_سونپو_String(std::move(other.m_string));
        } وإلا {
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
        other.m_type = V3_نمبر_انکڑے_Type::UNINITIALIZED;
        أرجع *this;
    }

    // ACCESSORS
    قدر_And_X* عدد() {
        UASSERT(is_نمبر(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_نمبر() ? m_inline_نمبر.انکڑے() : m_dynamic_نمبر.انکڑے();
    }
    ثابت قدر_And_X* عدد() ثابت VL_MT_محفوظ {
        UASSERT(is_نمبر(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_نمبر() ? m_inline_نمبر.انکڑے() : m_dynamic_نمبر.انکڑے();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }
    ثابت std::string& str() ثابت VL_MT_محفوظ {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }

    صحيح چوڑائی() ثابت VL_MT_محفوظ { أرجع m_چوڑائی; }
    V3_نمبر_انکڑے_Type type() ثابت VL_MT_محفوظ { أرجع m_type; }

    // METHODS
    فراغ resize(صحيح بٹ_گنتی) {
        إذا (m_چوڑائی == بٹ_گنتی) أرجع;
        إذا (بٹ_منزل_الفاظ(m_چوڑائی) == بٹ_منزل_الفاظ(بٹ_گنتی)) {
            m_چوڑائی = بٹ_گنتی;
            أرجع;
        }
        إذا (is_Dynamic_نمبر()) {
            إذا (بٹ_گنتی > MAX_INLINE_چوڑائی) {
                m_dynamic_نمبر.resize(بٹ_منزل_الفاظ(بٹ_گنتی));
            } وإلا {
                ثابت خودکار dynamic_بٹ = std::move(m_dynamic_نمبر);
                destroy_Dynamic_نمبر();
                init_Inline_نمبر();
                std::حافظہ_نقل(m_inline_نمبر.انکڑے(), dynamic_بٹ.انکڑے(), حجم(m_inline_نمبر));
            }
        } وإلا إذا (is_Inline_نمبر()) {
            إذا (بٹ_گنتی > MAX_INLINE_چوڑائی) {
                ثابت خودکار بٹ = m_inline_نمبر;
                init_Dynamic_نمبر(بٹ_منزل_الفاظ(بٹ_گنتی));
                std::حافظہ_نقل(m_dynamic_نمبر.انکڑے(), بٹ.انکڑے(), حجم(بٹ));
            }
        }
        m_چوڑائی = بٹ_گنتی;
    }

    فراغ سیٹ_String() {
        // If there has been a string already it is kept intact.
        إذا (isString()) أرجع;
        إذا (is_Dynamic_نمبر()) destroy_Dynamic_نمبر();
        initString();
        m_type = V3_نمبر_انکڑے_Type::STRING;
    }
    فراغ سیٹ_String(std::string&& s) {
        reinit_With_Or_سونپو_String(std::move(s));
        m_type = V3_نمبر_انکڑے_Type::STRING;
    }
    فراغ سیٹ_String(ثابت std::string& s) {
        reinit_With_Or_سونپو_String(s);
        m_type = V3_نمبر_انکڑے_Type::STRING;
    }

    فراغ سیٹ_Double() {
        destroy_Stored_قدر();
        إذا (!is_Inline_نمبر()) init_Inline_نمبر();
        m_type = V3_نمبر_انکڑے_Type::DOUBLE;
        resize(64);
    }

    فراغ سیٹ_Logic() {
        إذا (isString()) destroyString();
        إذا (!is_نمبر()) {
            إذا (m_چوڑائی <= MAX_INLINE_چوڑائی) {
                init_Inline_نمبر();
            } وإلا {
                init_Dynamic_نمبر(بٹ_منزل_الفاظ(m_چوڑائی));
            }
        }
        m_type = V3_نمبر_انکڑے_Type::LOGIC;
        resize(m_چوڑائی);
    }

خاص:
    ساكن constexpr صحيح بٹ_منزل_الفاظ(صحيح بٹ_گنتی) VL_خالص { أرجع (بٹ_گنتی + 31) / 32; }

    bool is_نمبر() ثابت VL_MT_محفوظ {
        أرجع m_type == V3_نمبر_انکڑے_Type::DOUBLE || m_type == V3_نمبر_انکڑے_Type::LOGIC;
    }
    bool is_Inline_نمبر() ثابت VL_MT_محفوظ {
        أرجع (m_چوڑائی <= MAX_INLINE_چوڑائی)
               && (m_type == V3_نمبر_انکڑے_Type::DOUBLE || m_type == V3_نمبر_انکڑے_Type::LOGIC);
    }
    bool is_Dynamic_نمبر() ثابت VL_MT_محفوظ {
        أرجع (m_چوڑائی > MAX_INLINE_چوڑائی) && (m_type == V3_نمبر_انکڑے_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_محفوظ { أرجع m_type == V3_نمبر_انکڑے_Type::STRING; }

    سانچا <typename... دلیلاں_2>
    فراغ init_Inline_نمبر(دلیلاں_2&&... دلیلاں) {
        new (&m_inline_نمبر) std::array<قدر_And_X, INLINE_الفاظ>(std::forward<دلیلاں_2>(دلیلاں)...);
    }
    سانچا <typename... دلیلاں_2>
    فراغ init_Dynamic_نمبر(دلیلاں_2&&... دلیلاں) {
        new (&m_dynamic_نمبر) std::سمتیہ<قدر_And_X>(std::forward<دلیلاں_2>(دلیلاں)...);
    }
    سانچا <typename... دلیلاں_2>
    فراغ initString(دلیلاں_2&&... دلیلاں) {
        new (&m_string) std::string(std::forward<دلیلاں_2>(دلیلاں)...);
    }

    string formatDecimal();

    فراغ destroy_Dynamic_نمبر() { m_dynamic_نمبر.~سمتیہ(); }
    فراغ destroyString() { m_string.~string(); }
    فراغ destroy_Stored_قدر() {
        إذا (isString())
            destroyString();
        وإلا إذا (is_Dynamic_نمبر())
            destroy_Dynamic_نمبر();
    }

    سانچا <typename T>
    فراغ reinit_With_Or_سونپو_Dynamic_نمبر(T&& s) {
        إذا (is_Dynamic_نمبر()) {
            m_dynamic_نمبر = std::forward<T>(s);
            أرجع;
        }
        إذا (isString()) destroyString();
        init_Dynamic_نمبر(std::forward<T>(s));
    }
    سانچا <typename T>
    فراغ reinit_With_Or_سونپو_String(T&& s) {
        إذا (isString()) {
            m_string = std::forward<T>(s);
            أرجع;
        }
        إذا (is_Dynamic_نمبر()) destroy_Dynamic_نمبر();
        initString(std::forward<T>(s));
    }
};

class V3_نمبر final {
    // TYPES
    using قدر_And_X = V3_نمبر_انکڑے::قدر_And_X;
    using V3_نمبر_انکڑے_Type = V3_نمبر_انکڑے::V3_نمبر_انکڑے_Type;

    // MEMBERS
    V3_نمبر_انکڑے m_انکڑے;
    Ast_گرہ* m_nodep = nullptr;  // Parent node - for error reporting only
    File_سطر* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    فراغ سیٹ_Null() {
        m_انکڑے.سیٹ_Logic();
        m_انکڑے.m_isNull = true;
        m_انکڑے.m_autoExtend = true;
    }
    V3_نمبر& سیٹ_Single_بٹ(حرف قدر);
    فراغ fill_بٹ(صحيح destLsb, صحيح چوڑائی, حرف قدر);
    فراغ copy_بٹ(صحيح destLsb, ثابت V3_نمبر& سروت, صحيح سروت_Lsb, صحيح چوڑائی);
    V3_نمبر& سیٹ_String(ثابت string& str) {
        m_انکڑے.سیٹ_String(str);
        أرجع *this;
    }
    فراغ opCleanThis(bool warnOnTruncation = false);

عام:
    فراغ nodep(Ast_گرہ* nodep) VL_MT_STABLE;
    Ast_گرہ* nodep() ثابت VL_MT_محفوظ { أرجع m_nodep; }  // For debug only
    File_سطر* fileline() ثابت VL_MT_محفوظ { أرجع m_fileline; }
    V3_نمبر& سیٹ_Zero();
    V3_نمبر& سیٹ_Quad(uint64_t قدر);
    V3_نمبر& سیٹ_Long(uint32_t قدر);
    V3_نمبر& سیٹ_Long_S(int32_t قدر);
    V3_نمبر& سیٹ_Double(مزدوج قدر);
    فراغ سیٹ_ثنائی_ہندسہ_X0(صحيح ثنائی_ہندسہ_2);
    فراغ سیٹ_ثنائی_ہندسہ(صحيح ثنائی_ہندسہ_2, حرف قدر) {  // Note: must be initialized as number and pre-zeroed!
        إذا (ثنائی_ہندسہ_2 >= m_انکڑے.چوڑائی()) أرجع;
        ثابت uint32_t نقاب = (1UL << (ثنائی_ہندسہ_2 & 31));
        قدر_And_X& v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        إذا (قدر == '0' || قدر == 0) {
            v.m_قدر &= ~نقاب;
            v.m_قدر_X &= ~نقاب;
        } وإلا إذا (قدر == '1' || قدر == 1) {
            v.m_قدر |= نقاب;
            v.m_قدر_X &= ~نقاب;
        } وإلا إذا (قدر == 'z' || قدر == 2) {
            v.m_قدر &= ~نقاب;
            v.m_قدر_X |= نقاب;
        } وإلا {  // X
            v.m_قدر |= نقاب;
            v.m_قدر_X |= نقاب;
        }
    }

خاص:
    حرف ثنائی_ہندسہ_Is(صحيح ثنائی_ہندسہ_2) ثابت {
        إذا (ثنائی_ہندسہ_2 >= m_انکڑے.چوڑائی() || ثنائی_ہندسہ_2 < 0) {
            // We never sign extend
            أرجع '0';
        }
        ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        أرجع ("01zx"[(((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 1 : 0)
                        | ((v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 2 : 0))]);
    }
    حرف ثنائی_ہندسہ_Is_Extend(صحيح ثنائی_ہندسہ_2, صحيح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        إذا (ثنائی_ہندسہ_2 < 0) أرجع '0';
        UASSERT(lbits <= m_انکڑے.چوڑائی(), "Extend of wrong size");
        إذا (ثنائی_ہندسہ_2 >= lbits) {
            ثنائی_ہندسہ_2 = lbits ? lbits - 1 : 0;
            ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
            // We do sign extend
            أرجع ("01zx"[(((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 1 : 0)
                            | ((v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 2 : 0))]);
        }
        ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        أرجع ("01zx"[(((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 1 : 0)
                        | ((v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))) ? 2 : 0))]);
    }

عام:
    bool ثنائی_ہندسہ_Is0(صحيح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        إذا (!is_نمبر()) أرجع false;
        إذا (ثنائی_ہندسہ_2 < 0) أرجع false;
        إذا (ثنائی_ہندسہ_2 >= m_انکڑے.چوڑائی()) أرجع !ثنائی_ہندسہ_Is_XZ(m_انکڑے.چوڑائی() - 1);
        ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        أرجع ((v.m_قدر | v.m_قدر_X) & (1UL << (ثنائی_ہندسہ_2 & 31))) == 0;
    }
    bool ثنائی_ہندسہ_Is1(صحيح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        إذا (!is_نمبر()) أرجع false;
        إذا (ثنائی_ہندسہ_2 < 0) أرجع false;
        إذا (ثنائی_ہندسہ_2 >= m_انکڑے.چوڑائی()) أرجع false;
        ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        أرجع ((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) && !(v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }
    bool ثنائی_ہندسہ_Is1_Extend(صحيح ثنائی_ہندسہ_2) ثابت {
        // Correct number of zero bits/width matters
        إذا (!is_نمبر()) أرجع false;
        إذا (ثنائی_ہندسہ_2 < 0) أرجع false;
        إذا (ثنائی_ہندسہ_2 >= m_انکڑے.چوڑائی()) أرجع ثنائی_ہندسہ_Is1_Extend(m_انکڑے.چوڑائی() - 1);
        ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        أرجع ((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) && !(v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }
    bool ثنائی_ہندسہ_Is_X(صحيح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        إذا (!is_نمبر()) أرجع false;
        إذا (ثنائی_ہندسہ_2 < 0) أرجع false;
        إذا (ثنائی_ہندسہ_2 >= m_انکڑے.چوڑائی()) أرجع ثنائی_ہندسہ_Is_Z(m_انکڑے.چوڑائی() - 1);
        ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        أرجع ((v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) && (v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }
    bool ثنائی_ہندسہ_Is_XZ(صحيح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        إذا (!is_نمبر()) أرجع false;
        إذا (ثنائی_ہندسہ_2 < 0) أرجع false;
        إذا (ثنائی_ہندسہ_2 >= m_انکڑے.چوڑائی()) أرجع ثنائی_ہندسہ_Is_XZ(m_انکڑے.چوڑائی() - 1);
        ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        أرجع ((v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }
    bool ثنائی_ہندسہ_Is_Z(صحيح ثنائی_ہندسہ_2) ثابت VL_MT_محفوظ {
        إذا (!is_نمبر()) أرجع false;
        إذا (ثنائی_ہندسہ_2 < 0) أرجع false;
        إذا (ثنائی_ہندسہ_2 >= m_انکڑے.چوڑائی()) أرجع ثنائی_ہندسہ_Is_Z(m_انکڑے.چوڑائی() - 1);
        ثابت قدر_And_X v = m_انکڑے.عدد()[ثنائی_ہندسہ_2 / 32];
        أرجع ((~v.m_قدر & (1UL << (ثنائی_ہندسہ_2 & 31))) && (v.m_قدر_X & (1UL << (ثنائی_ہندسہ_2 & 31))));
    }

خاص:
    uint32_t بٹ_قدر(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ {
        uint32_t v = 0;
        لكل (صحيح bitn = 0; bitn < nbits; bitn++) v |= (ثنائی_ہندسہ_Is1(lsb + bitn) << bitn);
        أرجع v;
    }

    صحيح گنتی_X(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ;
    صحيح گنتی_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ;

    صحيح الفاظ() ثابت VL_MT_محفوظ { أرجع ((چوڑائی() + 31) / 32); }
    uint32_t hi_لفظ_نقاب() ثابت VL_MT_محفوظ {
        // Correct number of zero bits/width matters
        أرجع VL_نقاب_I(چوڑائی());
    }

    V3_نمبر& opModDivGuts(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs, bool is_modulus);

عام:
    // CONSTRUCTORS
    explicit V3_نمبر(Ast_گرہ* nodep) { init(nodep, 1); }
    V3_نمبر(Ast_گرہ* nodep, صحيح چوڑائی) {  // 0=unsized
        init(nodep, چوڑائی, چوڑائی > 0);
    }
    // Construct with value, changing to new width
    V3_نمبر(Ast_گرہ* nodep, صحيح چوڑائی, ثابت V3_نمبر& قدر) {
        init(nodep, چوڑائی, چوڑائی > 0);
        op_سونپو(قدر);
    }
    V3_نمبر(Ast_گرہ* nodep, صحيح چوڑائی, uint32_t قدر, bool sized = true) {
        init(nodep, چوڑائی, sized);
        m_انکڑے.عدد()[0].m_قدر = قدر;
        opCleanThis();
    }
    V3_نمبر(File_سطر* flp, صحيح چوڑائی, uint32_t قدر, bool sized = true) {
        init(nullptr, چوڑائی, sized);
        m_fileline = flp;
        m_انکڑے.عدد()[0].m_قدر = قدر;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_نمبر_Literal {};  // For creator type-overload selection
    V3_نمبر(Ast_گرہ* nodep, Verilog_نمبر_Literal, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3_نمبر(File_سطر* flp, Verilog_نمبر_Literal, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_نمبر(Ast_گرہ* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_نمبر(Ast_گرہ* nodep, Double, مزدوج قدر) {
        init(nodep, 64);
        سیٹ_Double(قدر);
    }
    class String {};
    V3_نمبر(Ast_گرہ* nodep, String, ثابت string& قدر) {
        init(nodep);
        سیٹ_String(قدر);
        m_انکڑے.m_fromString = true;
    }
    class One_قدم {};
    V3_نمبر(Ast_گرہ* nodep, One_قدم) {
        init(nodep, 64);
        m_انکڑے.m_is1_قدم = true;
    }
    class Null {};
    V3_نمبر(Ast_گرہ* nodep, Null) {
        init(nodep);
        سیٹ_Null();
    }
    explicit V3_نمبر(ثابت V3_نمبر* nump, صحيح چوڑائی = 1) {
        init(nullptr, چوڑائی);
        m_fileline = nump->fileline();
    }
    V3_نمبر(ثابت V3_نمبر* nump, صحيح چوڑائی, uint32_t قدر) {
        init(nullptr, چوڑائی);
        m_انکڑے.عدد()[0].m_قدر = قدر;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_نمبر(Ast_گرہ* nodep, ثابت Ast_گرہ_DType* nodedtypep);

    V3_نمبر(ثابت V3_نمبر& other) = افتراضي;
    V3_نمبر& عامل=(ثابت V3_نمبر& other) = افتراضي;

    V3_نمبر(V3_نمبر&& other) = افتراضي;
    V3_نمبر& عامل=(V3_نمبر&& other) = افتراضي;

    ~V3_نمبر() = افتراضي;

خاص:
    فراغ selfTestThis();
    فراغ create(Ast_گرہ* nodep, ثابت حرف* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    فراغ create(File_سطر* flp, ثابت حرف* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    فراغ create(ثابت حرف* sourcep);
    فراغ init(Ast_گرہ* nodep, صحيح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        إذا (swidth >= 0) {
            إذا (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_انکڑے.سیٹ_Logic();
            m_انکڑے.resize(swidth);
            m_انکڑے.m_sized = sized;
            لكل (صحيح i = 0; i < الفاظ(); ++i) m_انکڑے.عدد()[i] = {0, 0};
        } وإلا {
            m_انکڑے.resize(1);
            m_انکڑے.m_sized = false;
        }
    }
    ساكن string displayPad(ناپ_t fmtsize, حرف pad, bool بایاں, ثابت string& وچ) VL_خالص;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        أرجع displayed(m_fileline, vformat);
    }
    فراغ warnTooMany(ثابت string& قدر);

عام:
    فراغ v3error_سرا(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_سرا_Fatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ چوڑائی(صحيح چوڑائی, bool sized = true) {
        m_انکڑے.m_sized = sized;
        m_انکڑے.resize(چوڑائی);
    }

    // SETTERS
    V3_نمبر& سیٹ_All_بٹ_XRemoved();
    V3_نمبر& سیٹ_All_بٹ_X();
    V3_نمبر& سیٹ_All_بٹ_Z();
    V3_نمبر& سیٹ_All_Bits0();
    V3_نمبر& سیٹ_All_Bits1();
    V3_نمبر& سیٹ_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_نمبر& سیٹ_نقاب(صحيح nbits, صحيح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت Ast_گرہ* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(File_سطر* fl, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayedEnum(ثابت AstSFormatArg* argp, ثابت string& vformat) ثابت VL_MT_STABLE;
    string displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE;
    ساكن string displayedEnumName(ثابت Ast_Enum_شے* itemp);
    ساكن bool displayedFmtHasArg(حرف format, bool is_جانچ);
    string emitC() ثابت VL_MT_STABLE;
    صحيح چوڑائی() ثابت VL_MT_محفوظ { أرجع m_انکڑے.چوڑائی(); }
    صحيح چوڑائی_منزل_Fit() ثابت;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ثابت VL_MT_محفوظ { أرجع m_انکڑے.m_sized; }
    bool autoExtend() ثابت VL_MT_محفوظ { أرجع m_انکڑے.m_autoExtend; }
    bool isFromString() ثابت { أرجع m_انکڑے.m_fromString; }
    bool hasOrigParamName() ثابت { أرجع m_انکڑے.m_hasOrigParamName; }
    فراغ hasOrigParamName(bool flag) { m_انکڑے.m_hasOrigParamName = flag; }
    V3_نمبر_انکڑے_Type انکڑے_Type() ثابت VL_MT_محفوظ { أرجع m_انکڑے.type(); }
    فراغ انکڑے_Type(V3_نمبر_انکڑے_Type newType) {
        إذا (انکڑے_Type() == newType) أرجع;
        UASSERT(newType != V3_نمبر_انکڑے_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        بدل (newType) {
        حالة V3_نمبر_انکڑے_Type::STRING: m_انکڑے.سیٹ_String(); اكسر;
        حالة V3_نمبر_انکڑے_Type::DOUBLE: m_انکڑے.سیٹ_Double(); اكسر;
        حالة V3_نمبر_انکڑے_Type::LOGIC: m_انکڑے.سیٹ_Logic(); اكسر;
        حالة V3_نمبر_انکڑے_Type::UNINITIALIZED: اكسر;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_محفوظ { أرجع m_انکڑے.m_signed; }
    فراغ isSigned(bool ssigned) { m_انکڑے.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_محفوظ { أرجع انکڑے_Type() == V3_نمبر_انکڑے_Type::DOUBLE; }
    bool isString() ثابت VL_MT_محفوظ { أرجع انکڑے_Type() == V3_نمبر_انکڑے_Type::STRING; }
    bool isOpaque() ثابت VL_MT_محفوظ { أرجع isDouble() || isString(); }
    bool is_نمبر() ثابت VL_MT_محفوظ {
        أرجع m_انکڑے.type() == V3_نمبر_انکڑے_Type::LOGIC
               || m_انکڑے.type() == V3_نمبر_انکڑے_Type::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_محفوظ {
        // Correct number of zero bits/width matters
        أرجع !isString() && ثنائی_ہندسہ_Is1(چوڑائی() - 1);
    }
    bool is1_قدم() ثابت VL_MT_محفوظ { أرجع m_انکڑے.m_is1_قدم; }
    bool isNull() ثابت VL_MT_محفوظ { أرجع m_انکڑے.m_isNull; }
    bool isFourState() ثابت VL_MT_محفوظ;
    bool hasZ() ثابت {
        إذا (isString()) أرجع false;
        لكل (صحيح i = 0; i < الفاظ(); ++i) {
            ثابت قدر_And_X v = m_انکڑے.عدد()[i];
            إذا ((~v.m_قدر) & v.m_قدر_X) أرجع true;
        }
        أرجع false;
    }
    bool isAllZ() ثابت VL_MT_محفوظ;
    bool isAllX() ثابت VL_MT_محفوظ;
    bool isEqZero() ثابت VL_MT_محفوظ;
    bool isNeqZero() ثابت;
    bool is_بٹ_Zero(صحيح msb, صحيح lsb) ثابت;
    bool isBroken(صحيح vwidth) ثابت;
    bool isEqOne() ثابت;
    bool isEqAllOnes(صحيح optwidth = 0) ثابت;
    bool isCaseEq(ثابت V3_نمبر& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_نمبر& rhs) ثابت;  // operator< with XZ compared
    bool isAnyX() ثابت VL_MT_محفوظ;
    bool isAnyXZ() ثابت;
    bool isAnyZ() ثابت VL_MT_محفوظ;
    bool isMsbXZ() ثابت {
        // Correct number of zero bits/width matters
        أرجع ثنائی_ہندسہ_Is_XZ(چوڑائی() - 1);
    }
    bool سماندا_وچ_UInt() ثابت VL_MT_محفوظ;
    uint32_t منزل_UInt() ثابت VL_MT_محفوظ;
    int32_t منزل_SInt() ثابت VL_MT_محفوظ;
    uint64_t منزل_UQuad() ثابت VL_MT_محفوظ;
    int64_t منزل_SQuad() ثابت VL_MT_محفوظ;
    string منزل_String() ثابت VL_MT_محفوظ;
    string منزل_Decimal_S() ثابت VL_MT_STABLE;  // return ASCII signed decimal number
    string منزل_Decimal_U() ثابت VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string منزل_Real_String() ثابت VL_MT_STABLE;
    مزدوج منزل_Double() ثابت VL_MT_محفوظ;
    V3Hash منزل_Hash() ثابت;
    uint32_t edata_لفظ(صحيح eword) ثابت;
    uint8_t انکڑے_Byte(صحيح byte) ثابت;
    uint32_t گنتی_بٹ(ثابت V3_نمبر& ctrl) ثابت;
    uint32_t گنتی_بٹ(ثابت V3_نمبر& ctrl1, ثابت V3_نمبر& ctrl2, ثابت V3_نمبر& ctrl3) ثابت;
    uint32_t گنتی_Ones() ثابت;
    uint32_t most_سیٹ_ثنائی_ہندسہ_P1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t گھٹ_توں_گھٹ_سیٹ_ثنائی_ہندسہ_P1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool عامل<(ثابت V3_نمبر& rhs) ثابت { أرجع isLtXZ(rhs); }

    // STATICS
    ساكن bool epsilonEqual(مزدوج a, مزدوج b);  // True if number within Epsilon of the other
    ساكن bool epsilonIntegral(مزدوج a);  // True if number rounds to integer within Epsilon
    ساكن صحيح log2b(uint32_t عدد);
    ساكن صحيح log2bQuad(uint64_t عدد);
    ساكن فراغ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_نمبر& op_بٹ_Non_XZ(ثابت V3_نمبر& lhs);  // 0/1->1, X/Z->0
    V3_نمبر& op_بٹ_One(ثابت V3_نمبر& lhs);  // 1->1, 0/X/Z->0
    V3_نمبر& op_بٹ_XZ(ثابت V3_نمبر& lhs);  // 0/1->0, X/Z->1
    V3_نمبر& op_بٹ_Z(ثابت V3_نمبر& lhs);  // Z->1, 0/1/X->0
    //
    V3_نمبر& op_سونپو(ثابت V3_نمبر& lhs);
    V3_نمبر& op_سونپو_Non_XZ(ثابت V3_نمبر& lhs, bool ignoreXZ = true);
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
    V3_نمبر& opSelInto(ثابت V3_نمبر& lhs, ثابت V3_نمبر& lsb, صحيح چوڑائی);
    V3_نمبر& opSelInto(ثابت V3_نمبر& lhs, صحيح lsbval, صحيح چوڑائی);
    V3_نمبر& opCvtPackString(ثابت V3_نمبر& lhs);
    V3_نمبر& op_منزل_ہیٹھلا_N(ثابت V3_نمبر& lhs);
    V3_نمبر& op_منزل_اتلا_N(ثابت V3_نمبر& lhs);
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
    V3_نمبر& op_کھسکاؤ_R(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
    V3_نمبر& op_کھسکاؤ_RS(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_نمبر& op_کھسکاؤ_L(ثابت V3_نمبر& lhs, ثابت V3_نمبر& rhs);
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
    V3_نمبر& opISToRD(ثابت V3_نمبر& lhs) { أرجع opIToRD(lhs, true); }
    V3_نمبر& opRToIS(ثابت V3_نمبر& lhs);
    V3_نمبر& opRToIRoundS(ثابت V3_نمبر& lhs);
    V3_نمبر& op_Real_منزل_بٹ(ثابت V3_نمبر& lhs);
    V3_نمبر& op_بٹ_منزل_Real_D(ثابت V3_نمبر& lhs);
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
    V3_نمبر& opAtoN(ثابت V3_نمبر& lhs, صحيح ادھار);
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
مضمن std::ostream& عامل<<(std::ostream& os, ثابت V3_نمبر& rhs) VL_MT_محفوظ {
    أرجع os << rhs.ascii();
}

#جے_انت  // Guard
