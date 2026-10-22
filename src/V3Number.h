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

#که_نه_تعریف_شوی VERILATOR_V3_شمېره_H
#تعریف_کړه VERILATOR_V3_شمېره_H

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
class Ast_Enum_توکی;
class Ast_غوټه;
class Ast_غوټه_DType;
class AstSFormatArg;
class File_کرښه;

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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    حرف ascii() ثابت { أرجع m_e; }
    bool isComplex() ثابت { أرجع m_e == COMPLEX; }
    bool isDouble() ثابت { أرجع m_e == DOUBLE; }
    bool isEnum() ثابت { أرجع m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ثابت { أرجع m_e == SIGNED; }
    bool isString() ثابت { أرجع m_e == STRING; }
    bool isUnsigned() ثابت { أرجع m_e == UNSIGNED; }
};
constexpr bool چلوونکی==(ثابت VFormatAttr& lhs, ثابت VFormatAttr& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VFormatAttr& lhs, VFormatAttr::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VFormatAttr::en lhs, ثابت VFormatAttr& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VFormatAttr& rhs) {
    أرجع os << rhs.ascii();
}

//============================================================================

class V3_شمېره_معلومات final {
عام:
    // TYPES
    بنية ارزښت_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_ارزښت;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_ارزښت_X;
        bool چلوونکی==(ثابت ارزښت_And_X& other) ثابت VL_MT_خوندي {
            أرجع m_ارزښت == other.m_ارزښت && m_ارزښت_X == other.m_ارزښت_X;
        }
    };

    تعداد class V3_شمېره_معلومات_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    ملګری std::ostream& چلوونکی<<(std::ostream& os, ثابت V3_شمېره_معلومات_Type& rhs) VL_MT_خوندي {
        بدل (rhs) {
        حالة V3_شمېره_معلومات_Type::UNINITIALIZED: أرجع os << "UNINITIALIZED";
        حالة V3_شمېره_معلومات_Type::LOGIC: أرجع os << "LOGIC";
        حالة V3_شمېره_معلومات_Type::DOUBLE: أرجع os << "DOUBLE";
        حالة V3_شمېره_معلومات_Type::STRING: أرجع os << "STRING";
        }
        أرجع os;
    }

خاص:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ساكن constexpr صحيح INLINE_ټکي = std::max(
        static_cast<اندازه_t>(2), std::max(حجم(std::string) / حجم(ارزښت_And_X),
                                         حجم(std::ویکټور<ارزښت_And_X>) / حجم(ارزښت_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ساكن constexpr صحيح MAX_INLINE_پلنوالی = INLINE_ټکي * حجم(ارزښت_And_X) / 2 * 8;

    // MEMBERS
    اتحاد {
        std::array<ارزښت_And_X, INLINE_ټکي> m_inline_شمېره;
        std::ویکټور<ارزښت_And_X> m_dynamic_شمېره;
        std::string m_string;
    };

    صحيح m_پلنوالی = 0;  // Width (in bits) as specified/calculated
    V3_شمېره_معلومات_Type m_type;

عام:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_ګام : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_شمېره_معلومات()
        : m_type{V3_شمېره_معلومات_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_ګام{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_شمېره_معلومات() { destroy_Stored_ارزښت(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_شمېره_معلومات(ثابت V3_شمېره_معلومات& other)
        : m_پلنوالی{other.m_پلنوالی}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_ګام{other.m_is1_ګام}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_شمېره()) {
            init_Inline_شمېره(other.m_inline_شمېره);
        } وإلا إذا (other.is_Dynamic_شمېره()) {
            init_Dynamic_شمېره(other.m_dynamic_شمېره);
        } وإلا إذا (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_شمېره_معلومات& چلوونکی=(ثابت V3_شمېره_معلومات& other) {
        إذا (other.is_Inline_شمېره()) {
            destroy_Stored_ارزښت();
            init_Inline_شمېره(other.m_inline_شمېره);
        } وإلا إذا (other.is_Dynamic_شمېره()) {
            reinit_With_Or_ټاکل_Dynamic_شمېره(other.m_dynamic_شمېره);
        } وإلا إذا (other.isString()) {
            reinit_With_Or_ټاکل_String(other.m_string);
        } وإلا {
            destroy_Stored_ارزښت();
        }
        m_پلنوالی = other.m_پلنوالی;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_ګام = other.m_is1_ګام;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        أرجع *this;
    }

    V3_شمېره_معلومات(V3_شمېره_معلومات&& other)
        : m_پلنوالی{other.m_پلنوالی}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_ګام{other.m_is1_ګام}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_شمېره()) {
            init_Inline_شمېره(other.m_inline_شمېره);
        } وإلا إذا (other.is_Dynamic_شمېره()) {
            init_Dynamic_شمېره(std::move(other.m_dynamic_شمېره));
        } وإلا إذا (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_شمېره_معلومات_Type::UNINITIALIZED;
    }

    V3_شمېره_معلومات& چلوونکی=(V3_شمېره_معلومات&& other) {
        إذا (other.is_Inline_شمېره()) {
            destroy_Stored_ارزښت();
            init_Inline_شمېره(other.m_inline_شمېره);
        } وإلا إذا (other.is_Dynamic_شمېره()) {
            reinit_With_Or_ټاکل_Dynamic_شمېره(std::move(other.m_dynamic_شمېره));
        } وإلا إذا (other.isString()) {
            reinit_With_Or_ټاکل_String(std::move(other.m_string));
        } وإلا {
            destroy_Stored_ارزښت();
        }
        m_پلنوالی = other.m_پلنوالی;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_ګام = other.m_is1_ګام;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_شمېره_معلومات_Type::UNINITIALIZED;
        أرجع *this;
    }

    // ACCESSORS
    ارزښت_And_X* عدد() {
        UASSERT(is_شمېره(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_شمېره() ? m_inline_شمېره.معلومات() : m_dynamic_شمېره.معلومات();
    }
    ثابت ارزښت_And_X* عدد() ثابت VL_MT_خوندي {
        UASSERT(is_شمېره(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_شمېره() ? m_inline_شمېره.معلومات() : m_dynamic_شمېره.معلومات();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }
    ثابت std::string& str() ثابت VL_MT_خوندي {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }

    صحيح پلنوالی() ثابت VL_MT_خوندي { أرجع m_پلنوالی; }
    V3_شمېره_معلومات_Type type() ثابت VL_MT_خوندي { أرجع m_type; }

    // METHODS
    فراغ resize(صحيح بټونه_شمېر) {
        إذا (m_پلنوالی == بټونه_شمېر) أرجع;
        إذا (بټونه_منزل_ټکي(m_پلنوالی) == بټونه_منزل_ټکي(بټونه_شمېر)) {
            m_پلنوالی = بټونه_شمېر;
            أرجع;
        }
        إذا (is_Dynamic_شمېره()) {
            إذا (بټونه_شمېر > MAX_INLINE_پلنوالی) {
                m_dynamic_شمېره.resize(بټونه_منزل_ټکي(بټونه_شمېر));
            } وإلا {
                ثابت خپلکاری dynamic_بټونه = std::move(m_dynamic_شمېره);
                destroy_Dynamic_شمېره();
                init_Inline_شمېره();
                std::حافظه_نقل(m_inline_شمېره.معلومات(), dynamic_بټونه.معلومات(), حجم(m_inline_شمېره));
            }
        } وإلا إذا (is_Inline_شمېره()) {
            إذا (بټونه_شمېر > MAX_INLINE_پلنوالی) {
                ثابت خپلکاری بټونه = m_inline_شمېره;
                init_Dynamic_شمېره(بټونه_منزل_ټکي(بټونه_شمېر));
                std::حافظه_نقل(m_dynamic_شمېره.معلومات(), بټونه.معلومات(), حجم(بټونه));
            }
        }
        m_پلنوالی = بټونه_شمېر;
    }

    فراغ سېټ_String() {
        // If there has been a string already it is kept intact.
        إذا (isString()) أرجع;
        إذا (is_Dynamic_شمېره()) destroy_Dynamic_شمېره();
        initString();
        m_type = V3_شمېره_معلومات_Type::STRING;
    }
    فراغ سېټ_String(std::string&& s) {
        reinit_With_Or_ټاکل_String(std::move(s));
        m_type = V3_شمېره_معلومات_Type::STRING;
    }
    فراغ سېټ_String(ثابت std::string& s) {
        reinit_With_Or_ټاکل_String(s);
        m_type = V3_شمېره_معلومات_Type::STRING;
    }

    فراغ سېټ_Double() {
        destroy_Stored_ارزښت();
        إذا (!is_Inline_شمېره()) init_Inline_شمېره();
        m_type = V3_شمېره_معلومات_Type::DOUBLE;
        resize(64);
    }

    فراغ سېټ_Logic() {
        إذا (isString()) destroyString();
        إذا (!is_شمېره()) {
            إذا (m_پلنوالی <= MAX_INLINE_پلنوالی) {
                init_Inline_شمېره();
            } وإلا {
                init_Dynamic_شمېره(بټونه_منزل_ټکي(m_پلنوالی));
            }
        }
        m_type = V3_شمېره_معلومات_Type::LOGIC;
        resize(m_پلنوالی);
    }

خاص:
    ساكن constexpr صحيح بټونه_منزل_ټکي(صحيح بټونه_شمېر) VL_خالص { أرجع (بټونه_شمېر + 31) / 32; }

    bool is_شمېره() ثابت VL_MT_خوندي {
        أرجع m_type == V3_شمېره_معلومات_Type::DOUBLE || m_type == V3_شمېره_معلومات_Type::LOGIC;
    }
    bool is_Inline_شمېره() ثابت VL_MT_خوندي {
        أرجع (m_پلنوالی <= MAX_INLINE_پلنوالی)
               && (m_type == V3_شمېره_معلومات_Type::DOUBLE || m_type == V3_شمېره_معلومات_Type::LOGIC);
    }
    bool is_Dynamic_شمېره() ثابت VL_MT_خوندي {
        أرجع (m_پلنوالی > MAX_INLINE_پلنوالی) && (m_type == V3_شمېره_معلومات_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_خوندي { أرجع m_type == V3_شمېره_معلومات_Type::STRING; }

    کينډۍ <typename... دلیلونه_2>
    فراغ init_Inline_شمېره(دلیلونه_2&&... دلیلونه) {
        new (&m_inline_شمېره) std::array<ارزښت_And_X, INLINE_ټکي>(std::forward<دلیلونه_2>(دلیلونه)...);
    }
    کينډۍ <typename... دلیلونه_2>
    فراغ init_Dynamic_شمېره(دلیلونه_2&&... دلیلونه) {
        new (&m_dynamic_شمېره) std::ویکټور<ارزښت_And_X>(std::forward<دلیلونه_2>(دلیلونه)...);
    }
    کينډۍ <typename... دلیلونه_2>
    فراغ initString(دلیلونه_2&&... دلیلونه) {
        new (&m_string) std::string(std::forward<دلیلونه_2>(دلیلونه)...);
    }

    string formatDecimal();

    فراغ destroy_Dynamic_شمېره() { m_dynamic_شمېره.~ویکټور(); }
    فراغ destroyString() { m_string.~string(); }
    فراغ destroy_Stored_ارزښت() {
        إذا (isString())
            destroyString();
        وإلا إذا (is_Dynamic_شمېره())
            destroy_Dynamic_شمېره();
    }

    کينډۍ <typename T>
    فراغ reinit_With_Or_ټاکل_Dynamic_شمېره(T&& s) {
        إذا (is_Dynamic_شمېره()) {
            m_dynamic_شمېره = std::forward<T>(s);
            أرجع;
        }
        إذا (isString()) destroyString();
        init_Dynamic_شمېره(std::forward<T>(s));
    }
    کينډۍ <typename T>
    فراغ reinit_With_Or_ټاکل_String(T&& s) {
        إذا (isString()) {
            m_string = std::forward<T>(s);
            أرجع;
        }
        إذا (is_Dynamic_شمېره()) destroy_Dynamic_شمېره();
        initString(std::forward<T>(s));
    }
};

class V3_شمېره final {
    // TYPES
    using ارزښت_And_X = V3_شمېره_معلومات::ارزښت_And_X;
    using V3_شمېره_معلومات_Type = V3_شمېره_معلومات::V3_شمېره_معلومات_Type;

    // MEMBERS
    V3_شمېره_معلومات m_معلومات;
    Ast_غوټه* m_nodep = nullptr;  // Parent node - for error reporting only
    File_کرښه* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    فراغ سېټ_Null() {
        m_معلومات.سېټ_Logic();
        m_معلومات.m_isNull = true;
        m_معلومات.m_autoExtend = true;
    }
    V3_شمېره& سېټ_Single_بټونه(حرف ارزښت);
    فراغ fill_بټونه(صحيح destLsb, صحيح پلنوالی, حرف ارزښت);
    فراغ copy_بټونه(صحيح destLsb, ثابت V3_شمېره& سرچینه, صحيح سرچینه_Lsb, صحيح پلنوالی);
    V3_شمېره& سېټ_String(ثابت string& str) {
        m_معلومات.سېټ_String(str);
        أرجع *this;
    }
    فراغ opCleanThis(bool warnOnTruncation = false);

عام:
    فراغ nodep(Ast_غوټه* nodep) VL_MT_STABLE;
    Ast_غوټه* nodep() ثابت VL_MT_خوندي { أرجع m_nodep; }  // For debug only
    File_کرښه* fileline() ثابت VL_MT_خوندي { أرجع m_fileline; }
    V3_شمېره& سېټ_Zero();
    V3_شمېره& سېټ_Quad(uint64_t ارزښت);
    V3_شمېره& سېټ_Long(uint32_t ارزښت);
    V3_شمېره& سېټ_Long_S(int32_t ارزښت);
    V3_شمېره& سېټ_Double(مزدوج ارزښت);
    فراغ سېټ_بټ_X0(صحيح بټ_2);
    فراغ سېټ_بټ(صحيح بټ_2, حرف ارزښت) {  // Note: must be initialized as number and pre-zeroed!
        إذا (بټ_2 >= m_معلومات.پلنوالی()) أرجع;
        ثابت uint32_t ماسک = (1UL << (بټ_2 & 31));
        ارزښت_And_X& v = m_معلومات.عدد()[بټ_2 / 32];
        إذا (ارزښت == '0' || ارزښت == 0) {
            v.m_ارزښت &= ~ماسک;
            v.m_ارزښت_X &= ~ماسک;
        } وإلا إذا (ارزښت == '1' || ارزښت == 1) {
            v.m_ارزښت |= ماسک;
            v.m_ارزښت_X &= ~ماسک;
        } وإلا إذا (ارزښت == 'z' || ارزښت == 2) {
            v.m_ارزښت &= ~ماسک;
            v.m_ارزښت_X |= ماسک;
        } وإلا {  // X
            v.m_ارزښت |= ماسک;
            v.m_ارزښت_X |= ماسک;
        }
    }

خاص:
    حرف بټ_Is(صحيح بټ_2) ثابت {
        إذا (بټ_2 >= m_معلومات.پلنوالی() || بټ_2 < 0) {
            // We never sign extend
            أرجع '0';
        }
        ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
        أرجع ("01zx"[(((v.m_ارزښت & (1UL << (بټ_2 & 31))) ? 1 : 0)
                        | ((v.m_ارزښت_X & (1UL << (بټ_2 & 31))) ? 2 : 0))]);
    }
    حرف بټ_Is_Extend(صحيح بټ_2, صحيح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        إذا (بټ_2 < 0) أرجع '0';
        UASSERT(lbits <= m_معلومات.پلنوالی(), "Extend of wrong size");
        إذا (بټ_2 >= lbits) {
            بټ_2 = lbits ? lbits - 1 : 0;
            ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
            // We do sign extend
            أرجع ("01zx"[(((v.m_ارزښت & (1UL << (بټ_2 & 31))) ? 1 : 0)
                            | ((v.m_ارزښت_X & (1UL << (بټ_2 & 31))) ? 2 : 0))]);
        }
        ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
        أرجع ("01zx"[(((v.m_ارزښت & (1UL << (بټ_2 & 31))) ? 1 : 0)
                        | ((v.m_ارزښت_X & (1UL << (بټ_2 & 31))) ? 2 : 0))]);
    }

عام:
    bool بټ_Is0(صحيح بټ_2) ثابت VL_MT_خوندي {
        إذا (!is_شمېره()) أرجع false;
        إذا (بټ_2 < 0) أرجع false;
        إذا (بټ_2 >= m_معلومات.پلنوالی()) أرجع !بټ_Is_XZ(m_معلومات.پلنوالی() - 1);
        ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
        أرجع ((v.m_ارزښت | v.m_ارزښت_X) & (1UL << (بټ_2 & 31))) == 0;
    }
    bool بټ_Is1(صحيح بټ_2) ثابت VL_MT_خوندي {
        إذا (!is_شمېره()) أرجع false;
        إذا (بټ_2 < 0) أرجع false;
        إذا (بټ_2 >= m_معلومات.پلنوالی()) أرجع false;
        ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
        أرجع ((v.m_ارزښت & (1UL << (بټ_2 & 31))) && !(v.m_ارزښت_X & (1UL << (بټ_2 & 31))));
    }
    bool بټ_Is1_Extend(صحيح بټ_2) ثابت {
        // Correct number of zero bits/width matters
        إذا (!is_شمېره()) أرجع false;
        إذا (بټ_2 < 0) أرجع false;
        إذا (بټ_2 >= m_معلومات.پلنوالی()) أرجع بټ_Is1_Extend(m_معلومات.پلنوالی() - 1);
        ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
        أرجع ((v.m_ارزښت & (1UL << (بټ_2 & 31))) && !(v.m_ارزښت_X & (1UL << (بټ_2 & 31))));
    }
    bool بټ_Is_X(صحيح بټ_2) ثابت VL_MT_خوندي {
        إذا (!is_شمېره()) أرجع false;
        إذا (بټ_2 < 0) أرجع false;
        إذا (بټ_2 >= m_معلومات.پلنوالی()) أرجع بټ_Is_Z(m_معلومات.پلنوالی() - 1);
        ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
        أرجع ((v.m_ارزښت & (1UL << (بټ_2 & 31))) && (v.m_ارزښت_X & (1UL << (بټ_2 & 31))));
    }
    bool بټ_Is_XZ(صحيح بټ_2) ثابت VL_MT_خوندي {
        إذا (!is_شمېره()) أرجع false;
        إذا (بټ_2 < 0) أرجع false;
        إذا (بټ_2 >= m_معلومات.پلنوالی()) أرجع بټ_Is_XZ(m_معلومات.پلنوالی() - 1);
        ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
        أرجع ((v.m_ارزښت_X & (1UL << (بټ_2 & 31))));
    }
    bool بټ_Is_Z(صحيح بټ_2) ثابت VL_MT_خوندي {
        إذا (!is_شمېره()) أرجع false;
        إذا (بټ_2 < 0) أرجع false;
        إذا (بټ_2 >= m_معلومات.پلنوالی()) أرجع بټ_Is_Z(m_معلومات.پلنوالی() - 1);
        ثابت ارزښت_And_X v = m_معلومات.عدد()[بټ_2 / 32];
        أرجع ((~v.m_ارزښت & (1UL << (بټ_2 & 31))) && (v.m_ارزښت_X & (1UL << (بټ_2 & 31))));
    }

خاص:
    uint32_t بټونه_ارزښت(صحيح lsb, صحيح nbits) ثابت VL_MT_خوندي {
        uint32_t v = 0;
        لكل (صحيح bitn = 0; bitn < nbits; bitn++) v |= (بټ_Is1(lsb + bitn) << bitn);
        أرجع v;
    }

    صحيح شمېر_X(صحيح lsb, صحيح nbits) ثابت VL_MT_خوندي;
    صحيح شمېر_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_خوندي;

    صحيح ټکي() ثابت VL_MT_خوندي { أرجع ((پلنوالی() + 31) / 32); }
    uint32_t hi_کلمه_ماسک() ثابت VL_MT_خوندي {
        // Correct number of zero bits/width matters
        أرجع VL_ماسک_I(پلنوالی());
    }

    V3_شمېره& opModDivGuts(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, bool is_modulus);

عام:
    // CONSTRUCTORS
    explicit V3_شمېره(Ast_غوټه* nodep) { init(nodep, 1); }
    V3_شمېره(Ast_غوټه* nodep, صحيح پلنوالی) {  // 0=unsized
        init(nodep, پلنوالی, پلنوالی > 0);
    }
    // Construct with value, changing to new width
    V3_شمېره(Ast_غوټه* nodep, صحيح پلنوالی, ثابت V3_شمېره& ارزښت) {
        init(nodep, پلنوالی, پلنوالی > 0);
        op_ټاکل(ارزښت);
    }
    V3_شمېره(Ast_غوټه* nodep, صحيح پلنوالی, uint32_t ارزښت, bool sized = true) {
        init(nodep, پلنوالی, sized);
        m_معلومات.عدد()[0].m_ارزښت = ارزښت;
        opCleanThis();
    }
    V3_شمېره(File_کرښه* flp, صحيح پلنوالی, uint32_t ارزښت, bool sized = true) {
        init(nullptr, پلنوالی, sized);
        m_fileline = flp;
        m_معلومات.عدد()[0].m_ارزښت = ارزښت;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_شمېره_Literal {};  // For creator type-overload selection
    V3_شمېره(Ast_غوټه* nodep, Verilog_شمېره_Literal, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3_شمېره(File_کرښه* flp, Verilog_شمېره_Literal, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_شمېره(Ast_غوټه* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_شمېره(Ast_غوټه* nodep, Double, مزدوج ارزښت) {
        init(nodep, 64);
        سېټ_Double(ارزښت);
    }
    class String {};
    V3_شمېره(Ast_غوټه* nodep, String, ثابت string& ارزښت) {
        init(nodep);
        سېټ_String(ارزښت);
        m_معلومات.m_fromString = true;
    }
    class One_ګام {};
    V3_شمېره(Ast_غوټه* nodep, One_ګام) {
        init(nodep, 64);
        m_معلومات.m_is1_ګام = true;
    }
    class Null {};
    V3_شمېره(Ast_غوټه* nodep, Null) {
        init(nodep);
        سېټ_Null();
    }
    explicit V3_شمېره(ثابت V3_شمېره* nump, صحيح پلنوالی = 1) {
        init(nullptr, پلنوالی);
        m_fileline = nump->fileline();
    }
    V3_شمېره(ثابت V3_شمېره* nump, صحيح پلنوالی, uint32_t ارزښت) {
        init(nullptr, پلنوالی);
        m_معلومات.عدد()[0].m_ارزښت = ارزښت;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_شمېره(Ast_غوټه* nodep, ثابت Ast_غوټه_DType* nodedtypep);

    V3_شمېره(ثابت V3_شمېره& other) = افتراضي;
    V3_شمېره& چلوونکی=(ثابت V3_شمېره& other) = افتراضي;

    V3_شمېره(V3_شمېره&& other) = افتراضي;
    V3_شمېره& چلوونکی=(V3_شمېره&& other) = افتراضي;

    ~V3_شمېره() = افتراضي;

خاص:
    فراغ selfTestThis();
    فراغ create(Ast_غوټه* nodep, ثابت حرف* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    فراغ create(File_کرښه* flp, ثابت حرف* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    فراغ create(ثابت حرف* sourcep);
    فراغ init(Ast_غوټه* nodep, صحيح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        إذا (swidth >= 0) {
            إذا (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_معلومات.سېټ_Logic();
            m_معلومات.resize(swidth);
            m_معلومات.m_sized = sized;
            لكل (صحيح i = 0; i < ټکي(); ++i) m_معلومات.عدد()[i] = {0, 0};
        } وإلا {
            m_معلومات.resize(1);
            m_معلومات.m_sized = false;
        }
    }
    ساكن string displayPad(اندازه_t fmtsize, حرف pad, bool کیڼ_2, ثابت string& دننه) VL_خالص;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        أرجع displayed(m_fileline, vformat);
    }
    فراغ warnTooMany(ثابت string& ارزښت);

عام:
    فراغ v3error_پای_ټکی(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_پای_ټکی_Fatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ پلنوالی(صحيح پلنوالی, bool sized = true) {
        m_معلومات.m_sized = sized;
        m_معلومات.resize(پلنوالی);
    }

    // SETTERS
    V3_شمېره& سېټ_All_بټونه_XRemoved();
    V3_شمېره& سېټ_All_بټونه_X();
    V3_شمېره& سېټ_All_بټونه_Z();
    V3_شمېره& سېټ_All_Bits0();
    V3_شمېره& سېټ_All_Bits1();
    V3_شمېره& سېټ_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_شمېره& سېټ_ماسک(صحيح nbits, صحيح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت Ast_غوټه* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(File_کرښه* fl, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayedEnum(ثابت AstSFormatArg* argp, ثابت string& vformat) ثابت VL_MT_STABLE;
    string displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE;
    ساكن string displayedEnumName(ثابت Ast_Enum_توکی* itemp);
    ساكن bool displayedFmtHasArg(حرف format, bool is_لټون);
    string emitC() ثابت VL_MT_STABLE;
    صحيح پلنوالی() ثابت VL_MT_خوندي { أرجع m_معلومات.پلنوالی(); }
    صحيح پلنوالی_منزل_Fit() ثابت;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ثابت VL_MT_خوندي { أرجع m_معلومات.m_sized; }
    bool autoExtend() ثابت VL_MT_خوندي { أرجع m_معلومات.m_autoExtend; }
    bool isFromString() ثابت { أرجع m_معلومات.m_fromString; }
    bool hasOrigParamName() ثابت { أرجع m_معلومات.m_hasOrigParamName; }
    فراغ hasOrigParamName(bool flag) { m_معلومات.m_hasOrigParamName = flag; }
    V3_شمېره_معلومات_Type معلومات_Type() ثابت VL_MT_خوندي { أرجع m_معلومات.type(); }
    فراغ معلومات_Type(V3_شمېره_معلومات_Type newType) {
        إذا (معلومات_Type() == newType) أرجع;
        UASSERT(newType != V3_شمېره_معلومات_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        بدل (newType) {
        حالة V3_شمېره_معلومات_Type::STRING: m_معلومات.سېټ_String(); اكسر;
        حالة V3_شمېره_معلومات_Type::DOUBLE: m_معلومات.سېټ_Double(); اكسر;
        حالة V3_شمېره_معلومات_Type::LOGIC: m_معلومات.سېټ_Logic(); اكسر;
        حالة V3_شمېره_معلومات_Type::UNINITIALIZED: اكسر;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_خوندي { أرجع m_معلومات.m_signed; }
    فراغ isSigned(bool ssigned) { m_معلومات.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_خوندي { أرجع معلومات_Type() == V3_شمېره_معلومات_Type::DOUBLE; }
    bool isString() ثابت VL_MT_خوندي { أرجع معلومات_Type() == V3_شمېره_معلومات_Type::STRING; }
    bool isOpaque() ثابت VL_MT_خوندي { أرجع isDouble() || isString(); }
    bool is_شمېره() ثابت VL_MT_خوندي {
        أرجع m_معلومات.type() == V3_شمېره_معلومات_Type::LOGIC
               || m_معلومات.type() == V3_شمېره_معلومات_Type::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_خوندي {
        // Correct number of zero bits/width matters
        أرجع !isString() && بټ_Is1(پلنوالی() - 1);
    }
    bool is1_ګام() ثابت VL_MT_خوندي { أرجع m_معلومات.m_is1_ګام; }
    bool isNull() ثابت VL_MT_خوندي { أرجع m_معلومات.m_isNull; }
    bool isFourState() ثابت VL_MT_خوندي;
    bool hasZ() ثابت {
        إذا (isString()) أرجع false;
        لكل (صحيح i = 0; i < ټکي(); ++i) {
            ثابت ارزښت_And_X v = m_معلومات.عدد()[i];
            إذا ((~v.m_ارزښت) & v.m_ارزښت_X) أرجع true;
        }
        أرجع false;
    }
    bool isAllZ() ثابت VL_MT_خوندي;
    bool isAllX() ثابت VL_MT_خوندي;
    bool isEqZero() ثابت VL_MT_خوندي;
    bool isNeqZero() ثابت;
    bool is_بټونه_Zero(صحيح msb, صحيح lsb) ثابت;
    bool isBroken(صحيح vwidth) ثابت;
    bool isEqOne() ثابت;
    bool isEqAllOnes(صحيح optwidth = 0) ثابت;
    bool isCaseEq(ثابت V3_شمېره& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_شمېره& rhs) ثابت;  // operator< with XZ compared
    bool isAnyX() ثابت VL_MT_خوندي;
    bool isAnyXZ() ثابت;
    bool isAnyZ() ثابت VL_MT_خوندي;
    bool isMsbXZ() ثابت {
        // Correct number of zero bits/width matters
        أرجع بټ_Is_XZ(پلنوالی() - 1);
    }
    bool ځاییږي_دننه_UInt() ثابت VL_MT_خوندي;
    uint32_t منزل_UInt() ثابت VL_MT_خوندي;
    int32_t منزل_SInt() ثابت VL_MT_خوندي;
    uint64_t منزل_UQuad() ثابت VL_MT_خوندي;
    int64_t منزل_SQuad() ثابت VL_MT_خوندي;
    string منزل_String() ثابت VL_MT_خوندي;
    string منزل_Decimal_S() ثابت VL_MT_STABLE;  // return ASCII signed decimal number
    string منزل_Decimal_U() ثابت VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string منزل_Real_String() ثابت VL_MT_STABLE;
    مزدوج منزل_Double() ثابت VL_MT_خوندي;
    V3Hash منزل_Hash() ثابت;
    uint32_t edata_کلمه(صحيح eword) ثابت;
    uint8_t معلومات_Byte(صحيح byte) ثابت;
    uint32_t شمېر_بټونه(ثابت V3_شمېره& ctrl) ثابت;
    uint32_t شمېر_بټونه(ثابت V3_شمېره& ctrl1, ثابت V3_شمېره& ctrl2, ثابت V3_شمېره& ctrl3) ثابت;
    uint32_t شمېر_Ones() ثابت;
    uint32_t most_سېټ_بټ_P1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t لږترلږه_سېټ_بټ_P1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool چلوونکی<(ثابت V3_شمېره& rhs) ثابت { أرجع isLtXZ(rhs); }

    // STATICS
    ساكن bool epsilonEqual(مزدوج a, مزدوج b);  // True if number within Epsilon of the other
    ساكن bool epsilonIntegral(مزدوج a);  // True if number rounds to integer within Epsilon
    ساكن صحيح log2b(uint32_t عدد);
    ساكن صحيح log2bQuad(uint64_t عدد);
    ساكن فراغ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_شمېره& op_بټونه_Non_XZ(ثابت V3_شمېره& lhs);  // 0/1->1, X/Z->0
    V3_شمېره& op_بټونه_One(ثابت V3_شمېره& lhs);  // 1->1, 0/X/Z->0
    V3_شمېره& op_بټونه_XZ(ثابت V3_شمېره& lhs);  // 0/1->0, X/Z->1
    V3_شمېره& op_بټونه_Z(ثابت V3_شمېره& lhs);  // Z->1, 0/1/X->0
    //
    V3_شمېره& op_ټاکل(ثابت V3_شمېره& lhs);
    V3_شمېره& op_ټاکل_Non_XZ(ثابت V3_شمېره& lhs, bool ignoreXZ = true);
    V3_شمېره& opExtendS(ثابت V3_شمېره& lhs, uint32_t lbits);  // Sign extension
    V3_شمېره& opExtendXZ(ثابت V3_شمېره& lhs, uint32_t lbits);  // X/Z extension
    V3_شمېره& opRedOr(ثابت V3_شمېره& lhs);
    V3_شمېره& opRedAnd(ثابت V3_شمېره& lhs);
    V3_شمېره& opRedXor(ثابت V3_شمېره& lhs);
    V3_شمېره& op_شمېر_بټونه(ثابت V3_شمېره& بیان, ثابت V3_شمېره& ctrl1, ثابت V3_شمېره& ctrl2,
                          ثابت V3_شمېره& ctrl3);
    V3_شمېره& op_شمېر_Ones(ثابت V3_شمېره& lhs);
    V3_شمېره& opIsUnknown(ثابت V3_شمېره& lhs);
    V3_شمېره& opOneHot(ثابت V3_شمېره& lhs);
    V3_شمېره& opOneHot0(ثابت V3_شمېره& lhs);
    V3_شمېره& opCLog2(ثابت V3_شمېره& lhs);
    V3_شمېره& op_Most_سېټ_بټ_P1(ثابت V3_شمېره& lhs);
    V3_شمېره& opClean(ثابت V3_شمېره& lhs, uint32_t بټونه);
    V3_شمېره& opConcat(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLenN(ثابت V3_شمېره& lhs);
    V3_شمېره& opRepl(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opRepl(ثابت V3_شمېره& lhs, uint32_t rhsval);
    V3_شمېره& op_بهیر_L(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opSel(ثابت V3_شمېره& lhs, ثابت V3_شمېره& msb, ثابت V3_شمېره& lsb);
    V3_شمېره& opSel(ثابت V3_شمېره& lhs, uint32_t msbval, uint32_t lsbval);
    V3_شمېره& opSelInto(ثابت V3_شمېره& lhs, ثابت V3_شمېره& lsb, صحيح پلنوالی);
    V3_شمېره& opSelInto(ثابت V3_شمېره& lhs, صحيح lsbval, صحيح پلنوالی);
    V3_شمېره& opCvtPackString(ثابت V3_شمېره& lhs);
    V3_شمېره& op_منزل_ښکتنی_N(ثابت V3_شمېره& lhs);
    V3_شمېره& op_منزل_پاسنی_N(ثابت V3_شمېره& lhs);
    V3_شمېره& opCaseEq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opCaseNeq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opWildEq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opWildNeq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opBufIf1(ثابت V3_شمېره& ens, ثابت V3_شمېره& if1s);
    V3_شمېره& op_سېټ_Range(uint32_t lsb, uint32_t پلنوالی, حرف بټ_ارزښت);
    // "standard" math
    V3_شمېره& opNot(ثابت V3_شمېره& lhs);
    V3_شمېره& opLogNot(ثابت V3_شمېره& lhs);
    V3_شمېره& opLogAnd(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLogOr(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLogEq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLogIf(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opNegate(ثابت V3_شمېره& lhs);
    V3_شمېره& op_جمع_کړه(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opSub(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opMul(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opMulS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed
    V3_شمېره& opDiv(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opDivS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed
    V3_شمېره& opModDiv(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opModDivS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed
    V3_شمېره& opPow(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, bool lsign = false,
                    bool rsign = false);
    V3_شمېره& opPowSU(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed lhs, unsigned rhs
    V3_شمېره& opPowSS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed lhs, signed rhs
    V3_شمېره& opPowUS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Unsigned lhs, signed rhs
    V3_شمېره& opAnd(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opXor(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opOr(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& op_ښويدنه_R(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& op_ښويدنه_RS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_شمېره& op_ښويدنه_L(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    // Comparisons
    V3_شمېره& opEq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opNeq(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opGt(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opGtS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed
    V3_شمېره& opGte(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opGteS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed
    V3_شمېره& opLt(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLtS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed
    V3_شمېره& opLte(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLteS(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);  // Signed

    // "D" - double (aka real) math
    V3_شمېره& opIToRD(ثابت V3_شمېره& lhs, bool isSigned = false);
    V3_شمېره& opISToRD(ثابت V3_شمېره& lhs) { أرجع opIToRD(lhs, true); }
    V3_شمېره& opRToIS(ثابت V3_شمېره& lhs);
    V3_شمېره& opRToIRoundS(ثابت V3_شمېره& lhs);
    V3_شمېره& op_Real_منزل_بټونه(ثابت V3_شمېره& lhs);
    V3_شمېره& op_بټونه_منزل_Real_D(ثابت V3_شمېره& lhs);
    V3_شمېره& opNegateD(ثابت V3_شمېره& lhs);
    V3_شمېره& op_جمع_کړه_D(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opSubD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opMulD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opDivD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opPowD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    // Comparisons
    V3_شمېره& opEqD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opNeqD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opGtD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opGteD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLtD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLteD(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);

    // "N" - string operations
    V3_شمېره& opAtoN(ثابت V3_شمېره& lhs, صحيح بنسټ);
    V3_شمېره& opNToI(ثابت V3_شمېره& lhs);
    V3_شمېره& opPutcN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, ثابت V3_شمېره& ths);
    V3_شمېره& opGetcN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opSubstrN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, ثابت V3_شمېره& ths);
    V3_شمېره& opCompareNN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs, bool ignoreCase);
    V3_شمېره& opConcatN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opReplN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opReplN(ثابت V3_شمېره& lhs, uint32_t rhsval);
    V3_شمېره& opEqN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opNeqN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opGtN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opGteN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLtN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
    V3_شمېره& opLteN(ثابت V3_شمېره& lhs, ثابت V3_شمېره& rhs);
};
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت V3_شمېره& rhs) VL_MT_خوندي {
    أرجع os << rhs.ascii();
}

#د_که_پای  // Guard
