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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3_عدد_H
#وصف_ڪريو VERILATOR_V3_عدد_H

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
class Ast_Enum_شي;
class Ast_ڳنڍ;
class Ast_ڳنڍ_DType;
class AstSFormatArg;
class File_سٽ;

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

class V3_عدد_معلومات final {
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

    تعداد class V3_عدد_معلومات_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    دوست std::ostream& عامل<<(std::ostream& os, ثابت V3_عدد_معلومات_Type& rhs) VL_MT_محفوظ {
        بدل (rhs) {
        حالة V3_عدد_معلومات_Type::UNINITIALIZED: أرجع os << "UNINITIALIZED";
        حالة V3_عدد_معلومات_Type::LOGIC: أرجع os << "LOGIC";
        حالة V3_عدد_معلومات_Type::DOUBLE: أرجع os << "DOUBLE";
        حالة V3_عدد_معلومات_Type::STRING: أرجع os << "STRING";
        }
        أرجع os;
    }

خاص:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ساكن constexpr صحيح INLINE_لفظ = std::max(
        static_cast<سائيز_t>(2), std::max(حجم(std::string) / حجم(قدر_And_X),
                                         حجم(std::ويڪٽر<قدر_And_X>) / حجم(قدر_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ساكن constexpr صحيح MAX_INLINE_ويڪر = INLINE_لفظ * حجم(قدر_And_X) / 2 * 8;

    // MEMBERS
    اتحاد {
        std::array<قدر_And_X, INLINE_لفظ> m_inline_عدد;
        std::ويڪٽر<قدر_And_X> m_dynamic_عدد;
        std::string m_string;
    };

    صحيح m_ويڪر = 0;  // Width (in bits) as specified/calculated
    V3_عدد_معلومات_Type m_type;

عام:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_قدم : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_عدد_معلومات()
        : m_type{V3_عدد_معلومات_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_قدم{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_عدد_معلومات() { destroy_Stored_قدر(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_عدد_معلومات(ثابت V3_عدد_معلومات& other)
        : m_ويڪر{other.m_ويڪر}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_قدم{other.m_is1_قدم}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_عدد()) {
            init_Inline_عدد(other.m_inline_عدد);
        } وإلا إذا (other.is_Dynamic_عدد()) {
            init_Dynamic_عدد(other.m_dynamic_عدد);
        } وإلا إذا (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_عدد_معلومات& عامل=(ثابت V3_عدد_معلومات& other) {
        إذا (other.is_Inline_عدد()) {
            destroy_Stored_قدر();
            init_Inline_عدد(other.m_inline_عدد);
        } وإلا إذا (other.is_Dynamic_عدد()) {
            reinit_With_Or_سونپيو_Dynamic_عدد(other.m_dynamic_عدد);
        } وإلا إذا (other.isString()) {
            reinit_With_Or_سونپيو_String(other.m_string);
        } وإلا {
            destroy_Stored_قدر();
        }
        m_ويڪر = other.m_ويڪر;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_قدم = other.m_is1_قدم;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        أرجع *this;
    }

    V3_عدد_معلومات(V3_عدد_معلومات&& other)
        : m_ويڪر{other.m_ويڪر}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_قدم{other.m_is1_قدم}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_عدد()) {
            init_Inline_عدد(other.m_inline_عدد);
        } وإلا إذا (other.is_Dynamic_عدد()) {
            init_Dynamic_عدد(std::move(other.m_dynamic_عدد));
        } وإلا إذا (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_عدد_معلومات_Type::UNINITIALIZED;
    }

    V3_عدد_معلومات& عامل=(V3_عدد_معلومات&& other) {
        إذا (other.is_Inline_عدد()) {
            destroy_Stored_قدر();
            init_Inline_عدد(other.m_inline_عدد);
        } وإلا إذا (other.is_Dynamic_عدد()) {
            reinit_With_Or_سونپيو_Dynamic_عدد(std::move(other.m_dynamic_عدد));
        } وإلا إذا (other.isString()) {
            reinit_With_Or_سونپيو_String(std::move(other.m_string));
        } وإلا {
            destroy_Stored_قدر();
        }
        m_ويڪر = other.m_ويڪر;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_قدم = other.m_is1_قدم;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_عدد_معلومات_Type::UNINITIALIZED;
        أرجع *this;
    }

    // ACCESSORS
    قدر_And_X* انگ() {
        UASSERT(is_عدد(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_عدد() ? m_inline_عدد.معلومات() : m_dynamic_عدد.معلومات();
    }
    ثابت قدر_And_X* انگ() ثابت VL_MT_محفوظ {
        UASSERT(is_عدد(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_عدد() ? m_inline_عدد.معلومات() : m_dynamic_عدد.معلومات();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }
    ثابت std::string& str() ثابت VL_MT_محفوظ {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }

    صحيح ويڪر() ثابت VL_MT_محفوظ { أرجع m_ويڪر; }
    V3_عدد_معلومات_Type type() ثابت VL_MT_محفوظ { أرجع m_type; }

    // METHODS
    فراغ resize(صحيح بٽ_ڳڻپ) {
        إذا (m_ويڪر == بٽ_ڳڻپ) أرجع;
        إذا (بٽ_منزل_لفظ(m_ويڪر) == بٽ_منزل_لفظ(بٽ_ڳڻپ)) {
            m_ويڪر = بٽ_ڳڻپ;
            أرجع;
        }
        إذا (is_Dynamic_عدد()) {
            إذا (بٽ_ڳڻپ > MAX_INLINE_ويڪر) {
                m_dynamic_عدد.resize(بٽ_منزل_لفظ(بٽ_ڳڻپ));
            } وإلا {
                ثابت خودڪار dynamic_بٽ = std::move(m_dynamic_عدد);
                destroy_Dynamic_عدد();
                init_Inline_عدد();
                std::يادگيري_نقل(m_inline_عدد.معلومات(), dynamic_بٽ.معلومات(), حجم(m_inline_عدد));
            }
        } وإلا إذا (is_Inline_عدد()) {
            إذا (بٽ_ڳڻپ > MAX_INLINE_ويڪر) {
                ثابت خودڪار بٽ = m_inline_عدد;
                init_Dynamic_عدد(بٽ_منزل_لفظ(بٽ_ڳڻپ));
                std::يادگيري_نقل(m_dynamic_عدد.معلومات(), بٽ.معلومات(), حجم(بٽ));
            }
        }
        m_ويڪر = بٽ_ڳڻپ;
    }

    فراغ سيٽ_String() {
        // If there has been a string already it is kept intact.
        إذا (isString()) أرجع;
        إذا (is_Dynamic_عدد()) destroy_Dynamic_عدد();
        initString();
        m_type = V3_عدد_معلومات_Type::STRING;
    }
    فراغ سيٽ_String(std::string&& s) {
        reinit_With_Or_سونپيو_String(std::move(s));
        m_type = V3_عدد_معلومات_Type::STRING;
    }
    فراغ سيٽ_String(ثابت std::string& s) {
        reinit_With_Or_سونپيو_String(s);
        m_type = V3_عدد_معلومات_Type::STRING;
    }

    فراغ سيٽ_Double() {
        destroy_Stored_قدر();
        إذا (!is_Inline_عدد()) init_Inline_عدد();
        m_type = V3_عدد_معلومات_Type::DOUBLE;
        resize(64);
    }

    فراغ سيٽ_Logic() {
        إذا (isString()) destroyString();
        إذا (!is_عدد()) {
            إذا (m_ويڪر <= MAX_INLINE_ويڪر) {
                init_Inline_عدد();
            } وإلا {
                init_Dynamic_عدد(بٽ_منزل_لفظ(m_ويڪر));
            }
        }
        m_type = V3_عدد_معلومات_Type::LOGIC;
        resize(m_ويڪر);
    }

خاص:
    ساكن constexpr صحيح بٽ_منزل_لفظ(صحيح بٽ_ڳڻپ) VL_خالص { أرجع (بٽ_ڳڻپ + 31) / 32; }

    bool is_عدد() ثابت VL_MT_محفوظ {
        أرجع m_type == V3_عدد_معلومات_Type::DOUBLE || m_type == V3_عدد_معلومات_Type::LOGIC;
    }
    bool is_Inline_عدد() ثابت VL_MT_محفوظ {
        أرجع (m_ويڪر <= MAX_INLINE_ويڪر)
               && (m_type == V3_عدد_معلومات_Type::DOUBLE || m_type == V3_عدد_معلومات_Type::LOGIC);
    }
    bool is_Dynamic_عدد() ثابت VL_MT_محفوظ {
        أرجع (m_ويڪر > MAX_INLINE_ويڪر) && (m_type == V3_عدد_معلومات_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_محفوظ { أرجع m_type == V3_عدد_معلومات_Type::STRING; }

    سانچو <typename... دليلون_2>
    فراغ init_Inline_عدد(دليلون_2&&... دليلون) {
        new (&m_inline_عدد) std::array<قدر_And_X, INLINE_لفظ>(std::forward<دليلون_2>(دليلون)...);
    }
    سانچو <typename... دليلون_2>
    فراغ init_Dynamic_عدد(دليلون_2&&... دليلون) {
        new (&m_dynamic_عدد) std::ويڪٽر<قدر_And_X>(std::forward<دليلون_2>(دليلون)...);
    }
    سانچو <typename... دليلون_2>
    فراغ initString(دليلون_2&&... دليلون) {
        new (&m_string) std::string(std::forward<دليلون_2>(دليلون)...);
    }

    string formatDecimal();

    فراغ destroy_Dynamic_عدد() { m_dynamic_عدد.~ويڪٽر(); }
    فراغ destroyString() { m_string.~string(); }
    فراغ destroy_Stored_قدر() {
        إذا (isString())
            destroyString();
        وإلا إذا (is_Dynamic_عدد())
            destroy_Dynamic_عدد();
    }

    سانچو <typename T>
    فراغ reinit_With_Or_سونپيو_Dynamic_عدد(T&& s) {
        إذا (is_Dynamic_عدد()) {
            m_dynamic_عدد = std::forward<T>(s);
            أرجع;
        }
        إذا (isString()) destroyString();
        init_Dynamic_عدد(std::forward<T>(s));
    }
    سانچو <typename T>
    فراغ reinit_With_Or_سونپيو_String(T&& s) {
        إذا (isString()) {
            m_string = std::forward<T>(s);
            أرجع;
        }
        إذا (is_Dynamic_عدد()) destroy_Dynamic_عدد();
        initString(std::forward<T>(s));
    }
};

class V3_عدد final {
    // TYPES
    using قدر_And_X = V3_عدد_معلومات::قدر_And_X;
    using V3_عدد_معلومات_Type = V3_عدد_معلومات::V3_عدد_معلومات_Type;

    // MEMBERS
    V3_عدد_معلومات m_معلومات;
    Ast_ڳنڍ* m_nodep = nullptr;  // Parent node - for error reporting only
    File_سٽ* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    فراغ سيٽ_Null() {
        m_معلومات.سيٽ_Logic();
        m_معلومات.m_isNull = true;
        m_معلومات.m_autoExtend = true;
    }
    V3_عدد& سيٽ_Single_بٽ(حرف قدر);
    فراغ fill_بٽ(صحيح destLsb, صحيح ويڪر, حرف قدر);
    فراغ copy_بٽ(صحيح destLsb, ثابت V3_عدد& ذريعو, صحيح ذريعو_Lsb, صحيح ويڪر);
    V3_عدد& سيٽ_String(ثابت string& str) {
        m_معلومات.سيٽ_String(str);
        أرجع *this;
    }
    فراغ opCleanThis(bool warnOnTruncation = false);

عام:
    فراغ nodep(Ast_ڳنڍ* nodep) VL_MT_STABLE;
    Ast_ڳنڍ* nodep() ثابت VL_MT_محفوظ { أرجع m_nodep; }  // For debug only
    File_سٽ* fileline() ثابت VL_MT_محفوظ { أرجع m_fileline; }
    V3_عدد& سيٽ_Zero();
    V3_عدد& سيٽ_Quad(uint64_t قدر);
    V3_عدد& سيٽ_Long(uint32_t قدر);
    V3_عدد& سيٽ_Long_S(int32_t قدر);
    V3_عدد& سيٽ_Double(مزدوج قدر);
    فراغ سيٽ_ٻٽو_انگ_X0(صحيح ٻٽو_انگ_2);
    فراغ سيٽ_ٻٽو_انگ(صحيح ٻٽو_انگ_2, حرف قدر) {  // Note: must be initialized as number and pre-zeroed!
        إذا (ٻٽو_انگ_2 >= m_معلومات.ويڪر()) أرجع;
        ثابت uint32_t نقاب = (1UL << (ٻٽو_انگ_2 & 31));
        قدر_And_X& v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
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
    حرف ٻٽو_انگ_Is(صحيح ٻٽو_انگ_2) ثابت {
        إذا (ٻٽو_انگ_2 >= m_معلومات.ويڪر() || ٻٽو_انگ_2 < 0) {
            // We never sign extend
            أرجع '0';
        }
        ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
        أرجع ("01zx"[(((v.m_قدر & (1UL << (ٻٽو_انگ_2 & 31))) ? 1 : 0)
                        | ((v.m_قدر_X & (1UL << (ٻٽو_انگ_2 & 31))) ? 2 : 0))]);
    }
    حرف ٻٽو_انگ_Is_Extend(صحيح ٻٽو_انگ_2, صحيح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        إذا (ٻٽو_انگ_2 < 0) أرجع '0';
        UASSERT(lbits <= m_معلومات.ويڪر(), "Extend of wrong size");
        إذا (ٻٽو_انگ_2 >= lbits) {
            ٻٽو_انگ_2 = lbits ? lbits - 1 : 0;
            ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
            // We do sign extend
            أرجع ("01zx"[(((v.m_قدر & (1UL << (ٻٽو_انگ_2 & 31))) ? 1 : 0)
                            | ((v.m_قدر_X & (1UL << (ٻٽو_انگ_2 & 31))) ? 2 : 0))]);
        }
        ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
        أرجع ("01zx"[(((v.m_قدر & (1UL << (ٻٽو_انگ_2 & 31))) ? 1 : 0)
                        | ((v.m_قدر_X & (1UL << (ٻٽو_انگ_2 & 31))) ? 2 : 0))]);
    }

عام:
    bool ٻٽو_انگ_Is0(صحيح ٻٽو_انگ_2) ثابت VL_MT_محفوظ {
        إذا (!is_عدد()) أرجع false;
        إذا (ٻٽو_انگ_2 < 0) أرجع false;
        إذا (ٻٽو_انگ_2 >= m_معلومات.ويڪر()) أرجع !ٻٽو_انگ_Is_XZ(m_معلومات.ويڪر() - 1);
        ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
        أرجع ((v.m_قدر | v.m_قدر_X) & (1UL << (ٻٽو_انگ_2 & 31))) == 0;
    }
    bool ٻٽو_انگ_Is1(صحيح ٻٽو_انگ_2) ثابت VL_MT_محفوظ {
        إذا (!is_عدد()) أرجع false;
        إذا (ٻٽو_انگ_2 < 0) أرجع false;
        إذا (ٻٽو_انگ_2 >= m_معلومات.ويڪر()) أرجع false;
        ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
        أرجع ((v.m_قدر & (1UL << (ٻٽو_انگ_2 & 31))) && !(v.m_قدر_X & (1UL << (ٻٽو_انگ_2 & 31))));
    }
    bool ٻٽو_انگ_Is1_Extend(صحيح ٻٽو_انگ_2) ثابت {
        // Correct number of zero bits/width matters
        إذا (!is_عدد()) أرجع false;
        إذا (ٻٽو_انگ_2 < 0) أرجع false;
        إذا (ٻٽو_انگ_2 >= m_معلومات.ويڪر()) أرجع ٻٽو_انگ_Is1_Extend(m_معلومات.ويڪر() - 1);
        ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
        أرجع ((v.m_قدر & (1UL << (ٻٽو_انگ_2 & 31))) && !(v.m_قدر_X & (1UL << (ٻٽو_انگ_2 & 31))));
    }
    bool ٻٽو_انگ_Is_X(صحيح ٻٽو_انگ_2) ثابت VL_MT_محفوظ {
        إذا (!is_عدد()) أرجع false;
        إذا (ٻٽو_انگ_2 < 0) أرجع false;
        إذا (ٻٽو_انگ_2 >= m_معلومات.ويڪر()) أرجع ٻٽو_انگ_Is_Z(m_معلومات.ويڪر() - 1);
        ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
        أرجع ((v.m_قدر & (1UL << (ٻٽو_انگ_2 & 31))) && (v.m_قدر_X & (1UL << (ٻٽو_انگ_2 & 31))));
    }
    bool ٻٽو_انگ_Is_XZ(صحيح ٻٽو_انگ_2) ثابت VL_MT_محفوظ {
        إذا (!is_عدد()) أرجع false;
        إذا (ٻٽو_انگ_2 < 0) أرجع false;
        إذا (ٻٽو_انگ_2 >= m_معلومات.ويڪر()) أرجع ٻٽو_انگ_Is_XZ(m_معلومات.ويڪر() - 1);
        ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
        أرجع ((v.m_قدر_X & (1UL << (ٻٽو_انگ_2 & 31))));
    }
    bool ٻٽو_انگ_Is_Z(صحيح ٻٽو_انگ_2) ثابت VL_MT_محفوظ {
        إذا (!is_عدد()) أرجع false;
        إذا (ٻٽو_انگ_2 < 0) أرجع false;
        إذا (ٻٽو_انگ_2 >= m_معلومات.ويڪر()) أرجع ٻٽو_انگ_Is_Z(m_معلومات.ويڪر() - 1);
        ثابت قدر_And_X v = m_معلومات.انگ()[ٻٽو_انگ_2 / 32];
        أرجع ((~v.m_قدر & (1UL << (ٻٽو_انگ_2 & 31))) && (v.m_قدر_X & (1UL << (ٻٽو_انگ_2 & 31))));
    }

خاص:
    uint32_t بٽ_قدر(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ {
        uint32_t v = 0;
        لكل (صحيح bitn = 0; bitn < nbits; bitn++) v |= (ٻٽو_انگ_Is1(lsb + bitn) << bitn);
        أرجع v;
    }

    صحيح ڳڻپ_X(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ;
    صحيح ڳڻپ_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_محفوظ;

    صحيح لفظ_2() ثابت VL_MT_محفوظ { أرجع ((ويڪر() + 31) / 32); }
    uint32_t hi_لفظ_نقاب() ثابت VL_MT_محفوظ {
        // Correct number of zero bits/width matters
        أرجع VL_نقاب_I(ويڪر());
    }

    V3_عدد& opModDivGuts(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, bool is_modulus);

عام:
    // CONSTRUCTORS
    explicit V3_عدد(Ast_ڳنڍ* nodep) { init(nodep, 1); }
    V3_عدد(Ast_ڳنڍ* nodep, صحيح ويڪر) {  // 0=unsized
        init(nodep, ويڪر, ويڪر > 0);
    }
    // Construct with value, changing to new width
    V3_عدد(Ast_ڳنڍ* nodep, صحيح ويڪر, ثابت V3_عدد& قدر) {
        init(nodep, ويڪر, ويڪر > 0);
        op_سونپيو(قدر);
    }
    V3_عدد(Ast_ڳنڍ* nodep, صحيح ويڪر, uint32_t قدر, bool sized = true) {
        init(nodep, ويڪر, sized);
        m_معلومات.انگ()[0].m_قدر = قدر;
        opCleanThis();
    }
    V3_عدد(File_سٽ* flp, صحيح ويڪر, uint32_t قدر, bool sized = true) {
        init(nullptr, ويڪر, sized);
        m_fileline = flp;
        m_معلومات.انگ()[0].m_قدر = قدر;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_عدد_Literal {};  // For creator type-overload selection
    V3_عدد(Ast_ڳنڍ* nodep, Verilog_عدد_Literal, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3_عدد(File_سٽ* flp, Verilog_عدد_Literal, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_عدد(Ast_ڳنڍ* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_عدد(Ast_ڳنڍ* nodep, Double, مزدوج قدر) {
        init(nodep, 64);
        سيٽ_Double(قدر);
    }
    class String {};
    V3_عدد(Ast_ڳنڍ* nodep, String, ثابت string& قدر) {
        init(nodep);
        سيٽ_String(قدر);
        m_معلومات.m_fromString = true;
    }
    class One_قدم {};
    V3_عدد(Ast_ڳنڍ* nodep, One_قدم) {
        init(nodep, 64);
        m_معلومات.m_is1_قدم = true;
    }
    class Null {};
    V3_عدد(Ast_ڳنڍ* nodep, Null) {
        init(nodep);
        سيٽ_Null();
    }
    explicit V3_عدد(ثابت V3_عدد* nump, صحيح ويڪر = 1) {
        init(nullptr, ويڪر);
        m_fileline = nump->fileline();
    }
    V3_عدد(ثابت V3_عدد* nump, صحيح ويڪر, uint32_t قدر) {
        init(nullptr, ويڪر);
        m_معلومات.انگ()[0].m_قدر = قدر;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_عدد(Ast_ڳنڍ* nodep, ثابت Ast_ڳنڍ_DType* nodedtypep);

    V3_عدد(ثابت V3_عدد& other) = افتراضي;
    V3_عدد& عامل=(ثابت V3_عدد& other) = افتراضي;

    V3_عدد(V3_عدد&& other) = افتراضي;
    V3_عدد& عامل=(V3_عدد&& other) = افتراضي;

    ~V3_عدد() = افتراضي;

خاص:
    فراغ selfTestThis();
    فراغ create(Ast_ڳنڍ* nodep, ثابت حرف* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    فراغ create(File_سٽ* flp, ثابت حرف* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    فراغ create(ثابت حرف* sourcep);
    فراغ init(Ast_ڳنڍ* nodep, صحيح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        إذا (swidth >= 0) {
            إذا (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_معلومات.سيٽ_Logic();
            m_معلومات.resize(swidth);
            m_معلومات.m_sized = sized;
            لكل (صحيح i = 0; i < لفظ_2(); ++i) m_معلومات.انگ()[i] = {0, 0};
        } وإلا {
            m_معلومات.resize(1);
            m_معلومات.m_sized = false;
        }
    }
    ساكن string displayPad(سائيز_t fmtsize, حرف pad, bool کاٻو, ثابت string& اندريون) VL_خالص;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        أرجع displayed(m_fileline, vformat);
    }
    فراغ warnTooMany(ثابت string& قدر);

عام:
    فراغ v3error_پڇاڙي(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_پڇاڙي_Fatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ ويڪر(صحيح ويڪر, bool sized = true) {
        m_معلومات.m_sized = sized;
        m_معلومات.resize(ويڪر);
    }

    // SETTERS
    V3_عدد& سيٽ_All_بٽ_XRemoved();
    V3_عدد& سيٽ_All_بٽ_X();
    V3_عدد& سيٽ_All_بٽ_Z();
    V3_عدد& سيٽ_All_Bits0();
    V3_عدد& سيٽ_All_Bits1();
    V3_عدد& سيٽ_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_عدد& سيٽ_نقاب(صحيح nbits, صحيح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت Ast_ڳنڍ* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(File_سٽ* fl, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayedEnum(ثابت AstSFormatArg* argp, ثابت string& vformat) ثابت VL_MT_STABLE;
    string displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE;
    ساكن string displayedEnumName(ثابت Ast_Enum_شي* itemp);
    ساكن bool displayedFmtHasArg(حرف format, bool is_جاچ);
    string emitC() ثابت VL_MT_STABLE;
    صحيح ويڪر() ثابت VL_MT_محفوظ { أرجع m_معلومات.ويڪر(); }
    صحيح ويڪر_منزل_Fit() ثابت;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ثابت VL_MT_محفوظ { أرجع m_معلومات.m_sized; }
    bool autoExtend() ثابت VL_MT_محفوظ { أرجع m_معلومات.m_autoExtend; }
    bool isFromString() ثابت { أرجع m_معلومات.m_fromString; }
    bool hasOrigParamName() ثابت { أرجع m_معلومات.m_hasOrigParamName; }
    فراغ hasOrigParamName(bool flag) { m_معلومات.m_hasOrigParamName = flag; }
    V3_عدد_معلومات_Type معلومات_Type() ثابت VL_MT_محفوظ { أرجع m_معلومات.type(); }
    فراغ معلومات_Type(V3_عدد_معلومات_Type newType) {
        إذا (معلومات_Type() == newType) أرجع;
        UASSERT(newType != V3_عدد_معلومات_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        بدل (newType) {
        حالة V3_عدد_معلومات_Type::STRING: m_معلومات.سيٽ_String(); اكسر;
        حالة V3_عدد_معلومات_Type::DOUBLE: m_معلومات.سيٽ_Double(); اكسر;
        حالة V3_عدد_معلومات_Type::LOGIC: m_معلومات.سيٽ_Logic(); اكسر;
        حالة V3_عدد_معلومات_Type::UNINITIALIZED: اكسر;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_محفوظ { أرجع m_معلومات.m_signed; }
    فراغ isSigned(bool ssigned) { m_معلومات.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_محفوظ { أرجع معلومات_Type() == V3_عدد_معلومات_Type::DOUBLE; }
    bool isString() ثابت VL_MT_محفوظ { أرجع معلومات_Type() == V3_عدد_معلومات_Type::STRING; }
    bool isOpaque() ثابت VL_MT_محفوظ { أرجع isDouble() || isString(); }
    bool is_عدد() ثابت VL_MT_محفوظ {
        أرجع m_معلومات.type() == V3_عدد_معلومات_Type::LOGIC
               || m_معلومات.type() == V3_عدد_معلومات_Type::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_محفوظ {
        // Correct number of zero bits/width matters
        أرجع !isString() && ٻٽو_انگ_Is1(ويڪر() - 1);
    }
    bool is1_قدم() ثابت VL_MT_محفوظ { أرجع m_معلومات.m_is1_قدم; }
    bool isNull() ثابت VL_MT_محفوظ { أرجع m_معلومات.m_isNull; }
    bool isFourState() ثابت VL_MT_محفوظ;
    bool hasZ() ثابت {
        إذا (isString()) أرجع false;
        لكل (صحيح i = 0; i < لفظ_2(); ++i) {
            ثابت قدر_And_X v = m_معلومات.انگ()[i];
            إذا ((~v.m_قدر) & v.m_قدر_X) أرجع true;
        }
        أرجع false;
    }
    bool isAllZ() ثابت VL_MT_محفوظ;
    bool isAllX() ثابت VL_MT_محفوظ;
    bool isEqZero() ثابت VL_MT_محفوظ;
    bool isNeqZero() ثابت;
    bool is_بٽ_Zero(صحيح msb, صحيح lsb) ثابت;
    bool isBroken(صحيح vwidth) ثابت;
    bool isEqOne() ثابت;
    bool isEqAllOnes(صحيح optwidth = 0) ثابت;
    bool isCaseEq(ثابت V3_عدد& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_عدد& rhs) ثابت;  // operator< with XZ compared
    bool isAnyX() ثابت VL_MT_محفوظ;
    bool isAnyXZ() ثابت;
    bool isAnyZ() ثابت VL_MT_محفوظ;
    bool isMsbXZ() ثابت {
        // Correct number of zero bits/width matters
        أرجع ٻٽو_انگ_Is_XZ(ويڪر() - 1);
    }
    bool سمائي_اندريون_UInt() ثابت VL_MT_محفوظ;
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
    uint8_t معلومات_Byte(صحيح byte) ثابت;
    uint32_t ڳڻپ_بٽ(ثابت V3_عدد& ctrl) ثابت;
    uint32_t ڳڻپ_بٽ(ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2, ثابت V3_عدد& ctrl3) ثابت;
    uint32_t ڳڻپ_Ones() ثابت;
    uint32_t most_سيٽ_ٻٽو_انگ_P1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t گهٽ_کان_گهٽ_سيٽ_ٻٽو_انگ_P1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool عامل<(ثابت V3_عدد& rhs) ثابت { أرجع isLtXZ(rhs); }

    // STATICS
    ساكن bool epsilonEqual(مزدوج a, مزدوج b);  // True if number within Epsilon of the other
    ساكن bool epsilonIntegral(مزدوج a);  // True if number rounds to integer within Epsilon
    ساكن صحيح log2b(uint32_t انگ);
    ساكن صحيح log2bQuad(uint64_t انگ);
    ساكن فراغ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_عدد& op_بٽ_Non_XZ(ثابت V3_عدد& lhs);  // 0/1->1, X/Z->0
    V3_عدد& op_بٽ_One(ثابت V3_عدد& lhs);  // 1->1, 0/X/Z->0
    V3_عدد& op_بٽ_XZ(ثابت V3_عدد& lhs);  // 0/1->0, X/Z->1
    V3_عدد& op_بٽ_Z(ثابت V3_عدد& lhs);  // Z->1, 0/1/X->0
    //
    V3_عدد& op_سونپيو(ثابت V3_عدد& lhs);
    V3_عدد& op_سونپيو_Non_XZ(ثابت V3_عدد& lhs, bool ignoreXZ = true);
    V3_عدد& opExtendS(ثابت V3_عدد& lhs, uint32_t lbits);  // Sign extension
    V3_عدد& opExtendXZ(ثابت V3_عدد& lhs, uint32_t lbits);  // X/Z extension
    V3_عدد& opRedOr(ثابت V3_عدد& lhs);
    V3_عدد& opRedAnd(ثابت V3_عدد& lhs);
    V3_عدد& opRedXor(ثابت V3_عدد& lhs);
    V3_عدد& op_ڳڻپ_بٽ(ثابت V3_عدد& اظهار, ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2,
                          ثابت V3_عدد& ctrl3);
    V3_عدد& op_ڳڻپ_Ones(ثابت V3_عدد& lhs);
    V3_عدد& opIsUnknown(ثابت V3_عدد& lhs);
    V3_عدد& opOneHot(ثابت V3_عدد& lhs);
    V3_عدد& opOneHot0(ثابت V3_عدد& lhs);
    V3_عدد& opCLog2(ثابت V3_عدد& lhs);
    V3_عدد& op_Most_سيٽ_ٻٽو_انگ_P1(ثابت V3_عدد& lhs);
    V3_عدد& opClean(ثابت V3_عدد& lhs, uint32_t بٽ);
    V3_عدد& opConcat(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLenN(ثابت V3_عدد& lhs);
    V3_عدد& opRepl(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opRepl(ثابت V3_عدد& lhs, uint32_t rhsval);
    V3_عدد& op_وهڪرو_L(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSel(ثابت V3_عدد& lhs, ثابت V3_عدد& msb, ثابت V3_عدد& lsb);
    V3_عدد& opSel(ثابت V3_عدد& lhs, uint32_t msbval, uint32_t lsbval);
    V3_عدد& opSelInto(ثابت V3_عدد& lhs, ثابت V3_عدد& lsb, صحيح ويڪر);
    V3_عدد& opSelInto(ثابت V3_عدد& lhs, صحيح lsbval, صحيح ويڪر);
    V3_عدد& opCvtPackString(ثابت V3_عدد& lhs);
    V3_عدد& op_منزل_هيٺاهون_N(ثابت V3_عدد& lhs);
    V3_عدد& op_منزل_مٿانهون_N(ثابت V3_عدد& lhs);
    V3_عدد& opCaseEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opCaseNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opWildEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opWildNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opBufIf1(ثابت V3_عدد& ens, ثابت V3_عدد& if1s);
    V3_عدد& op_سيٽ_Range(uint32_t lsb, uint32_t ويڪر, حرف ٻٽو_انگ_قدر);
    // "standard" math
    V3_عدد& opNot(ثابت V3_عدد& lhs);
    V3_عدد& opLogNot(ثابت V3_عدد& lhs);
    V3_عدد& opLogAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogIf(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNegate(ثابت V3_عدد& lhs);
    V3_عدد& op_جوڙيو(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSub(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opMul(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opMulS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opDiv(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opDivS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opModDiv(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opModDivS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opPow(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, bool lsign = false,
                    bool rsign = false);
    V3_عدد& opPowSU(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed lhs, unsigned rhs
    V3_عدد& opPowSS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed lhs, signed rhs
    V3_عدد& opPowUS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Unsigned lhs, signed rhs
    V3_عدد& opAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opXor(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& op_سرڪايو_R(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& op_سرڪايو_RS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_عدد& op_سرڪايو_L(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    // Comparisons
    V3_عدد& opEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGt(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGtS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opGte(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGteS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opLt(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLtS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opLte(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLteS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed

    // "D" - double (aka real) math
    V3_عدد& opIToRD(ثابت V3_عدد& lhs, bool isSigned = false);
    V3_عدد& opISToRD(ثابت V3_عدد& lhs) { أرجع opIToRD(lhs, true); }
    V3_عدد& opRToIS(ثابت V3_عدد& lhs);
    V3_عدد& opRToIRoundS(ثابت V3_عدد& lhs);
    V3_عدد& op_Real_منزل_بٽ(ثابت V3_عدد& lhs);
    V3_عدد& op_بٽ_منزل_Real_D(ثابت V3_عدد& lhs);
    V3_عدد& opNegateD(ثابت V3_عدد& lhs);
    V3_عدد& op_جوڙيو_D(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSubD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opMulD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opDivD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opPowD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    // Comparisons
    V3_عدد& opEqD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNeqD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGtD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGteD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLtD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLteD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);

    // "N" - string operations
    V3_عدد& opAtoN(ثابت V3_عدد& lhs, صحيح بنياد);
    V3_عدد& opNToI(ثابت V3_عدد& lhs);
    V3_عدد& opPutcN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, ثابت V3_عدد& ths);
    V3_عدد& opGetcN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSubstrN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, ثابت V3_عدد& ths);
    V3_عدد& opCompareNN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, bool ignoreCase);
    V3_عدد& opConcatN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opReplN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opReplN(ثابت V3_عدد& lhs, uint32_t rhsval);
    V3_عدد& opEqN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNeqN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGtN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGteN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLtN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLteN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
};
مضمن std::ostream& عامل<<(std::ostream& os, ثابت V3_عدد& rhs) VL_MT_محفوظ {
    أرجع os << rhs.ascii();
}

#جيڪڏهن_پورو  // Guard
