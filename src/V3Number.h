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

#اگر_تعریف_نشده VERILATOR_V3_عدد_H
#تعریف_کن VERILATOR_V3_عدد_H

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
class Ast_Enum_قلم;
class Ast_گره;
class Ast_گره_DType;
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
    constexpr عملگر en() ثابت { أرجع m_e; }
    حرف ascii() ثابت { أرجع m_e; }
    bool isComplex() ثابت { أرجع m_e == COMPLEX; }
    bool isDouble() ثابت { أرجع m_e == DOUBLE; }
    bool isEnum() ثابت { أرجع m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ثابت { أرجع m_e == SIGNED; }
    bool isString() ثابت { أرجع m_e == STRING; }
    bool isUnsigned() ثابت { أرجع m_e == UNSIGNED; }
};
constexpr bool عملگر==(ثابت VFormatAttr& lhs, ثابت VFormatAttr& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عملگر==(ثابت VFormatAttr& lhs, VFormatAttr::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عملگر==(VFormatAttr::en lhs, ثابت VFormatAttr& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت VFormatAttr& rhs) {
    أرجع os << rhs.ascii();
}

//============================================================================

class V3_عدد_داده final {
عام:
    // TYPES
    بنية مقدار_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_مقدار;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_مقدار_X;
        bool عملگر==(ثابت مقدار_And_X& other) ثابت VL_MT_امن {
            أرجع m_مقدار == other.m_مقدار && m_مقدار_X == other.m_مقدار_X;
        }
    };

    تعداد class V3_عدد_داده_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    دوست std::ostream& عملگر<<(std::ostream& os, ثابت V3_عدد_داده_Type& rhs) VL_MT_امن {
        بدل (rhs) {
        حالة V3_عدد_داده_Type::UNINITIALIZED: أرجع os << "UNINITIALIZED";
        حالة V3_عدد_داده_Type::LOGIC: أرجع os << "LOGIC";
        حالة V3_عدد_داده_Type::DOUBLE: أرجع os << "DOUBLE";
        حالة V3_عدد_داده_Type::STRING: أرجع os << "STRING";
        }
        أرجع os;
    }

خاص:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ساكن constexpr صحيح INLINE_واژگان = std::max(
        static_cast<اندازه_t>(2), std::max(حجم(std::string) / حجم(مقدار_And_X),
                                         حجم(std::وکتور<مقدار_And_X>) / حجم(مقدار_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ساكن constexpr صحيح MAX_INLINE_عرض = INLINE_واژگان * حجم(مقدار_And_X) / 2 * 8;

    // MEMBERS
    اتحاد {
        std::array<مقدار_And_X, INLINE_واژگان> m_inline_عدد;
        std::وکتور<مقدار_And_X> m_dynamic_عدد;
        std::string m_string;
    };

    صحيح m_عرض = 0;  // Width (in bits) as specified/calculated
    V3_عدد_داده_Type m_type;

عام:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_گام : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_عدد_داده()
        : m_type{V3_عدد_داده_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_گام{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_عدد_داده() { destroy_Stored_مقدار(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_عدد_داده(ثابت V3_عدد_داده& other)
        : m_عرض{other.m_عرض}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_گام{other.m_is1_گام}
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

    V3_عدد_داده& عملگر=(ثابت V3_عدد_داده& other) {
        إذا (other.is_Inline_عدد()) {
            destroy_Stored_مقدار();
            init_Inline_عدد(other.m_inline_عدد);
        } وإلا إذا (other.is_Dynamic_عدد()) {
            reinit_With_Or_تخصیص_بده_Dynamic_عدد(other.m_dynamic_عدد);
        } وإلا إذا (other.isString()) {
            reinit_With_Or_تخصیص_بده_String(other.m_string);
        } وإلا {
            destroy_Stored_مقدار();
        }
        m_عرض = other.m_عرض;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_گام = other.m_is1_گام;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        أرجع *this;
    }

    V3_عدد_داده(V3_عدد_داده&& other)
        : m_عرض{other.m_عرض}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_گام{other.m_is1_گام}
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
        other.m_type = V3_عدد_داده_Type::UNINITIALIZED;
    }

    V3_عدد_داده& عملگر=(V3_عدد_داده&& other) {
        إذا (other.is_Inline_عدد()) {
            destroy_Stored_مقدار();
            init_Inline_عدد(other.m_inline_عدد);
        } وإلا إذا (other.is_Dynamic_عدد()) {
            reinit_With_Or_تخصیص_بده_Dynamic_عدد(std::move(other.m_dynamic_عدد));
        } وإلا إذا (other.isString()) {
            reinit_With_Or_تخصیص_بده_String(std::move(other.m_string));
        } وإلا {
            destroy_Stored_مقدار();
        }
        m_عرض = other.m_عرض;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_گام = other.m_is1_گام;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_عدد_داده_Type::UNINITIALIZED;
        أرجع *this;
    }

    // ACCESSORS
    مقدار_And_X* رقم() {
        UASSERT(is_عدد(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_عدد() ? m_inline_عدد.داده() : m_dynamic_عدد.داده();
    }
    ثابت مقدار_And_X* رقم() ثابت VL_MT_امن {
        UASSERT(is_عدد(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_عدد() ? m_inline_عدد.داده() : m_dynamic_عدد.داده();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }
    ثابت std::string& str() ثابت VL_MT_امن {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }

    صحيح عرض() ثابت VL_MT_امن { أرجع m_عرض; }
    V3_عدد_داده_Type type() ثابت VL_MT_امن { أرجع m_type; }

    // METHODS
    فراغ resize(صحيح بیتها_شمار) {
        إذا (m_عرض == بیتها_شمار) أرجع;
        إذا (بیتها_مقصد_واژگان(m_عرض) == بیتها_مقصد_واژگان(بیتها_شمار)) {
            m_عرض = بیتها_شمار;
            أرجع;
        }
        إذا (is_Dynamic_عدد()) {
            إذا (بیتها_شمار > MAX_INLINE_عرض) {
                m_dynamic_عدد.resize(بیتها_مقصد_واژگان(بیتها_شمار));
            } وإلا {
                ثابت خودکار dynamic_بیتها = std::move(m_dynamic_عدد);
                destroy_Dynamic_عدد();
                init_Inline_عدد();
                std::کپی_حافظه(m_inline_عدد.داده(), dynamic_بیتها.داده(), حجم(m_inline_عدد));
            }
        } وإلا إذا (is_Inline_عدد()) {
            إذا (بیتها_شمار > MAX_INLINE_عرض) {
                ثابت خودکار بیتها = m_inline_عدد;
                init_Dynamic_عدد(بیتها_مقصد_واژگان(بیتها_شمار));
                std::کپی_حافظه(m_dynamic_عدد.داده(), بیتها.داده(), حجم(بیتها));
            }
        }
        m_عرض = بیتها_شمار;
    }

    فراغ مجموعه_String() {
        // If there has been a string already it is kept intact.
        إذا (isString()) أرجع;
        إذا (is_Dynamic_عدد()) destroy_Dynamic_عدد();
        initString();
        m_type = V3_عدد_داده_Type::STRING;
    }
    فراغ مجموعه_String(std::string&& s) {
        reinit_With_Or_تخصیص_بده_String(std::move(s));
        m_type = V3_عدد_داده_Type::STRING;
    }
    فراغ مجموعه_String(ثابت std::string& s) {
        reinit_With_Or_تخصیص_بده_String(s);
        m_type = V3_عدد_داده_Type::STRING;
    }

    فراغ مجموعه_Double() {
        destroy_Stored_مقدار();
        إذا (!is_Inline_عدد()) init_Inline_عدد();
        m_type = V3_عدد_داده_Type::DOUBLE;
        resize(64);
    }

    فراغ مجموعه_Logic() {
        إذا (isString()) destroyString();
        إذا (!is_عدد()) {
            إذا (m_عرض <= MAX_INLINE_عرض) {
                init_Inline_عدد();
            } وإلا {
                init_Dynamic_عدد(بیتها_مقصد_واژگان(m_عرض));
            }
        }
        m_type = V3_عدد_داده_Type::LOGIC;
        resize(m_عرض);
    }

خاص:
    ساكن constexpr صحيح بیتها_مقصد_واژگان(صحيح بیتها_شمار) VL_خالص { أرجع (بیتها_شمار + 31) / 32; }

    bool is_عدد() ثابت VL_MT_امن {
        أرجع m_type == V3_عدد_داده_Type::DOUBLE || m_type == V3_عدد_داده_Type::LOGIC;
    }
    bool is_Inline_عدد() ثابت VL_MT_امن {
        أرجع (m_عرض <= MAX_INLINE_عرض)
               && (m_type == V3_عدد_داده_Type::DOUBLE || m_type == V3_عدد_داده_Type::LOGIC);
    }
    bool is_Dynamic_عدد() ثابت VL_MT_امن {
        أرجع (m_عرض > MAX_INLINE_عرض) && (m_type == V3_عدد_داده_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_امن { أرجع m_type == V3_عدد_داده_Type::STRING; }

    قالب <typename... آرگومانها_2>
    فراغ init_Inline_عدد(آرگومانها_2&&... آرگومانها) {
        new (&m_inline_عدد) std::array<مقدار_And_X, INLINE_واژگان>(std::forward<آرگومانها_2>(آرگومانها)...);
    }
    قالب <typename... آرگومانها_2>
    فراغ init_Dynamic_عدد(آرگومانها_2&&... آرگومانها) {
        new (&m_dynamic_عدد) std::وکتور<مقدار_And_X>(std::forward<آرگومانها_2>(آرگومانها)...);
    }
    قالب <typename... آرگومانها_2>
    فراغ initString(آرگومانها_2&&... آرگومانها) {
        new (&m_string) std::string(std::forward<آرگومانها_2>(آرگومانها)...);
    }

    string formatDecimal();

    فراغ destroy_Dynamic_عدد() { m_dynamic_عدد.~وکتور(); }
    فراغ destroyString() { m_string.~string(); }
    فراغ destroy_Stored_مقدار() {
        إذا (isString())
            destroyString();
        وإلا إذا (is_Dynamic_عدد())
            destroy_Dynamic_عدد();
    }

    قالب <typename T>
    فراغ reinit_With_Or_تخصیص_بده_Dynamic_عدد(T&& s) {
        إذا (is_Dynamic_عدد()) {
            m_dynamic_عدد = std::forward<T>(s);
            أرجع;
        }
        إذا (isString()) destroyString();
        init_Dynamic_عدد(std::forward<T>(s));
    }
    قالب <typename T>
    فراغ reinit_With_Or_تخصیص_بده_String(T&& s) {
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
    using مقدار_And_X = V3_عدد_داده::مقدار_And_X;
    using V3_عدد_داده_Type = V3_عدد_داده::V3_عدد_داده_Type;

    // MEMBERS
    V3_عدد_داده m_داده;
    Ast_گره* m_nodep = nullptr;  // Parent node - for error reporting only
    File_سطر* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    فراغ مجموعه_Null() {
        m_داده.مجموعه_Logic();
        m_داده.m_isNull = true;
        m_داده.m_autoExtend = true;
    }
    V3_عدد& مجموعه_Single_بیتها(حرف مقدار);
    فراغ fill_بیتها(صحيح destLsb, صحيح عرض, حرف مقدار);
    فراغ copy_بیتها(صحيح destLsb, ثابت V3_عدد& منبع, صحيح منبع_Lsb, صحيح عرض);
    V3_عدد& مجموعه_String(ثابت string& str) {
        m_داده.مجموعه_String(str);
        أرجع *this;
    }
    فراغ opCleanThis(bool warnOnTruncation = false);

عام:
    فراغ nodep(Ast_گره* nodep) VL_MT_STABLE;
    Ast_گره* nodep() ثابت VL_MT_امن { أرجع m_nodep; }  // For debug only
    File_سطر* fileline() ثابت VL_MT_امن { أرجع m_fileline; }
    V3_عدد& مجموعه_Zero();
    V3_عدد& مجموعه_Quad(uint64_t مقدار);
    V3_عدد& مجموعه_Long(uint32_t مقدار);
    V3_عدد& مجموعه_Long_S(int32_t مقدار);
    V3_عدد& مجموعه_Double(مزدوج مقدار);
    فراغ مجموعه_بیت_X0(صحيح بیت_2);
    فراغ مجموعه_بیت(صحيح بیت_2, حرف مقدار) {  // Note: must be initialized as number and pre-zeroed!
        إذا (بیت_2 >= m_داده.عرض()) أرجع;
        ثابت uint32_t ماسک = (1UL << (بیت_2 & 31));
        مقدار_And_X& v = m_داده.رقم()[بیت_2 / 32];
        إذا (مقدار == '0' || مقدار == 0) {
            v.m_مقدار &= ~ماسک;
            v.m_مقدار_X &= ~ماسک;
        } وإلا إذا (مقدار == '1' || مقدار == 1) {
            v.m_مقدار |= ماسک;
            v.m_مقدار_X &= ~ماسک;
        } وإلا إذا (مقدار == 'z' || مقدار == 2) {
            v.m_مقدار &= ~ماسک;
            v.m_مقدار_X |= ماسک;
        } وإلا {  // X
            v.m_مقدار |= ماسک;
            v.m_مقدار_X |= ماسک;
        }
    }

خاص:
    حرف بیت_Is(صحيح بیت_2) ثابت {
        إذا (بیت_2 >= m_داده.عرض() || بیت_2 < 0) {
            // We never sign extend
            أرجع '0';
        }
        ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
        أرجع ("01zx"[(((v.m_مقدار & (1UL << (بیت_2 & 31))) ? 1 : 0)
                        | ((v.m_مقدار_X & (1UL << (بیت_2 & 31))) ? 2 : 0))]);
    }
    حرف بیت_Is_Extend(صحيح بیت_2, صحيح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        إذا (بیت_2 < 0) أرجع '0';
        UASSERT(lbits <= m_داده.عرض(), "Extend of wrong size");
        إذا (بیت_2 >= lbits) {
            بیت_2 = lbits ? lbits - 1 : 0;
            ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
            // We do sign extend
            أرجع ("01zx"[(((v.m_مقدار & (1UL << (بیت_2 & 31))) ? 1 : 0)
                            | ((v.m_مقدار_X & (1UL << (بیت_2 & 31))) ? 2 : 0))]);
        }
        ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
        أرجع ("01zx"[(((v.m_مقدار & (1UL << (بیت_2 & 31))) ? 1 : 0)
                        | ((v.m_مقدار_X & (1UL << (بیت_2 & 31))) ? 2 : 0))]);
    }

عام:
    bool بیت_Is0(صحيح بیت_2) ثابت VL_MT_امن {
        إذا (!is_عدد()) أرجع false;
        إذا (بیت_2 < 0) أرجع false;
        إذا (بیت_2 >= m_داده.عرض()) أرجع !بیت_Is_XZ(m_داده.عرض() - 1);
        ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
        أرجع ((v.m_مقدار | v.m_مقدار_X) & (1UL << (بیت_2 & 31))) == 0;
    }
    bool بیت_Is1(صحيح بیت_2) ثابت VL_MT_امن {
        إذا (!is_عدد()) أرجع false;
        إذا (بیت_2 < 0) أرجع false;
        إذا (بیت_2 >= m_داده.عرض()) أرجع false;
        ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
        أرجع ((v.m_مقدار & (1UL << (بیت_2 & 31))) && !(v.m_مقدار_X & (1UL << (بیت_2 & 31))));
    }
    bool بیت_Is1_Extend(صحيح بیت_2) ثابت {
        // Correct number of zero bits/width matters
        إذا (!is_عدد()) أرجع false;
        إذا (بیت_2 < 0) أرجع false;
        إذا (بیت_2 >= m_داده.عرض()) أرجع بیت_Is1_Extend(m_داده.عرض() - 1);
        ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
        أرجع ((v.m_مقدار & (1UL << (بیت_2 & 31))) && !(v.m_مقدار_X & (1UL << (بیت_2 & 31))));
    }
    bool بیت_Is_X(صحيح بیت_2) ثابت VL_MT_امن {
        إذا (!is_عدد()) أرجع false;
        إذا (بیت_2 < 0) أرجع false;
        إذا (بیت_2 >= m_داده.عرض()) أرجع بیت_Is_Z(m_داده.عرض() - 1);
        ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
        أرجع ((v.m_مقدار & (1UL << (بیت_2 & 31))) && (v.m_مقدار_X & (1UL << (بیت_2 & 31))));
    }
    bool بیت_Is_XZ(صحيح بیت_2) ثابت VL_MT_امن {
        إذا (!is_عدد()) أرجع false;
        إذا (بیت_2 < 0) أرجع false;
        إذا (بیت_2 >= m_داده.عرض()) أرجع بیت_Is_XZ(m_داده.عرض() - 1);
        ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
        أرجع ((v.m_مقدار_X & (1UL << (بیت_2 & 31))));
    }
    bool بیت_Is_Z(صحيح بیت_2) ثابت VL_MT_امن {
        إذا (!is_عدد()) أرجع false;
        إذا (بیت_2 < 0) أرجع false;
        إذا (بیت_2 >= m_داده.عرض()) أرجع بیت_Is_Z(m_داده.عرض() - 1);
        ثابت مقدار_And_X v = m_داده.رقم()[بیت_2 / 32];
        أرجع ((~v.m_مقدار & (1UL << (بیت_2 & 31))) && (v.m_مقدار_X & (1UL << (بیت_2 & 31))));
    }

خاص:
    uint32_t بیتها_مقدار(صحيح lsb, صحيح nbits) ثابت VL_MT_امن {
        uint32_t v = 0;
        لكل (صحيح bitn = 0; bitn < nbits; bitn++) v |= (بیت_Is1(lsb + bitn) << bitn);
        أرجع v;
    }

    صحيح شمار_X(صحيح lsb, صحيح nbits) ثابت VL_MT_امن;
    صحيح شمار_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_امن;

    صحيح واژگان() ثابت VL_MT_امن { أرجع ((عرض() + 31) / 32); }
    uint32_t hi_کلمه_ماسک() ثابت VL_MT_امن {
        // Correct number of zero bits/width matters
        أرجع VL_ماسک_I(عرض());
    }

    V3_عدد& opModDivGuts(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, bool is_modulus);

عام:
    // CONSTRUCTORS
    explicit V3_عدد(Ast_گره* nodep) { init(nodep, 1); }
    V3_عدد(Ast_گره* nodep, صحيح عرض) {  // 0=unsized
        init(nodep, عرض, عرض > 0);
    }
    // Construct with value, changing to new width
    V3_عدد(Ast_گره* nodep, صحيح عرض, ثابت V3_عدد& مقدار) {
        init(nodep, عرض, عرض > 0);
        op_تخصیص_بده(مقدار);
    }
    V3_عدد(Ast_گره* nodep, صحيح عرض, uint32_t مقدار, bool sized = true) {
        init(nodep, عرض, sized);
        m_داده.رقم()[0].m_مقدار = مقدار;
        opCleanThis();
    }
    V3_عدد(File_سطر* flp, صحيح عرض, uint32_t مقدار, bool sized = true) {
        init(nullptr, عرض, sized);
        m_fileline = flp;
        m_داده.رقم()[0].m_مقدار = مقدار;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_عدد_Literal {};  // For creator type-overload selection
    V3_عدد(Ast_گره* nodep, Verilog_عدد_Literal, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3_عدد(File_سطر* flp, Verilog_عدد_Literal, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_عدد(Ast_گره* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_عدد(Ast_گره* nodep, Double, مزدوج مقدار) {
        init(nodep, 64);
        مجموعه_Double(مقدار);
    }
    class String {};
    V3_عدد(Ast_گره* nodep, String, ثابت string& مقدار) {
        init(nodep);
        مجموعه_String(مقدار);
        m_داده.m_fromString = true;
    }
    class One_گام {};
    V3_عدد(Ast_گره* nodep, One_گام) {
        init(nodep, 64);
        m_داده.m_is1_گام = true;
    }
    class Null {};
    V3_عدد(Ast_گره* nodep, Null) {
        init(nodep);
        مجموعه_Null();
    }
    explicit V3_عدد(ثابت V3_عدد* nump, صحيح عرض = 1) {
        init(nullptr, عرض);
        m_fileline = nump->fileline();
    }
    V3_عدد(ثابت V3_عدد* nump, صحيح عرض, uint32_t مقدار) {
        init(nullptr, عرض);
        m_داده.رقم()[0].m_مقدار = مقدار;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_عدد(Ast_گره* nodep, ثابت Ast_گره_DType* nodedtypep);

    V3_عدد(ثابت V3_عدد& other) = افتراضي;
    V3_عدد& عملگر=(ثابت V3_عدد& other) = افتراضي;

    V3_عدد(V3_عدد&& other) = افتراضي;
    V3_عدد& عملگر=(V3_عدد&& other) = افتراضي;

    ~V3_عدد() = افتراضي;

خاص:
    فراغ selfTestThis();
    فراغ create(Ast_گره* nodep, ثابت حرف* sourcep) {
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
    فراغ init(Ast_گره* nodep, صحيح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        إذا (swidth >= 0) {
            إذا (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_داده.مجموعه_Logic();
            m_داده.resize(swidth);
            m_داده.m_sized = sized;
            لكل (صحيح i = 0; i < واژگان(); ++i) m_داده.رقم()[i] = {0, 0};
        } وإلا {
            m_داده.resize(1);
            m_داده.m_sized = false;
        }
    }
    ساكن string displayPad(اندازه_t fmtsize, حرف pad, bool چپ_2, ثابت string& درون) VL_خالص;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        أرجع displayed(m_fileline, vformat);
    }
    فراغ warnTooMany(ثابت string& مقدار);

عام:
    فراغ v3error_انتها(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_انتها_Fatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ عرض(صحيح عرض, bool sized = true) {
        m_داده.m_sized = sized;
        m_داده.resize(عرض);
    }

    // SETTERS
    V3_عدد& مجموعه_All_بیتها_XRemoved();
    V3_عدد& مجموعه_All_بیتها_X();
    V3_عدد& مجموعه_All_بیتها_Z();
    V3_عدد& مجموعه_All_Bits0();
    V3_عدد& مجموعه_All_Bits1();
    V3_عدد& مجموعه_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_عدد& مجموعه_ماسک(صحيح nbits, صحيح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت Ast_گره* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(File_سطر* fl, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayedEnum(ثابت AstSFormatArg* argp, ثابت string& vformat) ثابت VL_MT_STABLE;
    string displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE;
    ساكن string displayedEnumName(ثابت Ast_Enum_قلم* itemp);
    ساكن bool displayedFmtHasArg(حرف format, bool is_پیمایش);
    string emitC() ثابت VL_MT_STABLE;
    صحيح عرض() ثابت VL_MT_امن { أرجع m_داده.عرض(); }
    صحيح عرض_مقصد_Fit() ثابت;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ثابت VL_MT_امن { أرجع m_داده.m_sized; }
    bool autoExtend() ثابت VL_MT_امن { أرجع m_داده.m_autoExtend; }
    bool isFromString() ثابت { أرجع m_داده.m_fromString; }
    bool hasOrigParamName() ثابت { أرجع m_داده.m_hasOrigParamName; }
    فراغ hasOrigParamName(bool flag) { m_داده.m_hasOrigParamName = flag; }
    V3_عدد_داده_Type داده_Type() ثابت VL_MT_امن { أرجع m_داده.type(); }
    فراغ داده_Type(V3_عدد_داده_Type newType) {
        إذا (داده_Type() == newType) أرجع;
        UASSERT(newType != V3_عدد_داده_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        بدل (newType) {
        حالة V3_عدد_داده_Type::STRING: m_داده.مجموعه_String(); اكسر;
        حالة V3_عدد_داده_Type::DOUBLE: m_داده.مجموعه_Double(); اكسر;
        حالة V3_عدد_داده_Type::LOGIC: m_داده.مجموعه_Logic(); اكسر;
        حالة V3_عدد_داده_Type::UNINITIALIZED: اكسر;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_امن { أرجع m_داده.m_signed; }
    فراغ isSigned(bool ssigned) { m_داده.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_امن { أرجع داده_Type() == V3_عدد_داده_Type::DOUBLE; }
    bool isString() ثابت VL_MT_امن { أرجع داده_Type() == V3_عدد_داده_Type::STRING; }
    bool isOpaque() ثابت VL_MT_امن { أرجع isDouble() || isString(); }
    bool is_عدد() ثابت VL_MT_امن {
        أرجع m_داده.type() == V3_عدد_داده_Type::LOGIC
               || m_داده.type() == V3_عدد_داده_Type::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_امن {
        // Correct number of zero bits/width matters
        أرجع !isString() && بیت_Is1(عرض() - 1);
    }
    bool is1_گام() ثابت VL_MT_امن { أرجع m_داده.m_is1_گام; }
    bool isNull() ثابت VL_MT_امن { أرجع m_داده.m_isNull; }
    bool isFourState() ثابت VL_MT_امن;
    bool hasZ() ثابت {
        إذا (isString()) أرجع false;
        لكل (صحيح i = 0; i < واژگان(); ++i) {
            ثابت مقدار_And_X v = m_داده.رقم()[i];
            إذا ((~v.m_مقدار) & v.m_مقدار_X) أرجع true;
        }
        أرجع false;
    }
    bool isAllZ() ثابت VL_MT_امن;
    bool isAllX() ثابت VL_MT_امن;
    bool isEqZero() ثابت VL_MT_امن;
    bool isNeqZero() ثابت;
    bool is_بیتها_Zero(صحيح msb, صحيح lsb) ثابت;
    bool isBroken(صحيح vwidth) ثابت;
    bool isEqOne() ثابت;
    bool isEqAllOnes(صحيح optwidth = 0) ثابت;
    bool isCaseEq(ثابت V3_عدد& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_عدد& rhs) ثابت;  // operator< with XZ compared
    bool isAnyX() ثابت VL_MT_امن;
    bool isAnyXZ() ثابت;
    bool isAnyZ() ثابت VL_MT_امن;
    bool isMsbXZ() ثابت {
        // Correct number of zero bits/width matters
        أرجع بیت_Is_XZ(عرض() - 1);
    }
    bool میگنجد_درون_UInt() ثابت VL_MT_امن;
    uint32_t مقصد_UInt() ثابت VL_MT_امن;
    int32_t مقصد_SInt() ثابت VL_MT_امن;
    uint64_t مقصد_UQuad() ثابت VL_MT_امن;
    int64_t مقصد_SQuad() ثابت VL_MT_امن;
    string مقصد_String() ثابت VL_MT_امن;
    string مقصد_Decimal_S() ثابت VL_MT_STABLE;  // return ASCII signed decimal number
    string مقصد_Decimal_U() ثابت VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string مقصد_Real_String() ثابت VL_MT_STABLE;
    مزدوج مقصد_Double() ثابت VL_MT_امن;
    V3Hash مقصد_Hash() ثابت;
    uint32_t edata_کلمه(صحيح eword) ثابت;
    uint8_t داده_Byte(صحيح byte) ثابت;
    uint32_t شمار_بیتها(ثابت V3_عدد& ctrl) ثابت;
    uint32_t شمار_بیتها(ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2, ثابت V3_عدد& ctrl3) ثابت;
    uint32_t شمار_Ones() ثابت;
    uint32_t most_مجموعه_بیت_P1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t کمترین_مجموعه_بیت_P1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool عملگر<(ثابت V3_عدد& rhs) ثابت { أرجع isLtXZ(rhs); }

    // STATICS
    ساكن bool epsilonEqual(مزدوج a, مزدوج b);  // True if number within Epsilon of the other
    ساكن bool epsilonIntegral(مزدوج a);  // True if number rounds to integer within Epsilon
    ساكن صحيح log2b(uint32_t رقم);
    ساكن صحيح log2bQuad(uint64_t رقم);
    ساكن فراغ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_عدد& op_بیتها_Non_XZ(ثابت V3_عدد& lhs);  // 0/1->1, X/Z->0
    V3_عدد& op_بیتها_One(ثابت V3_عدد& lhs);  // 1->1, 0/X/Z->0
    V3_عدد& op_بیتها_XZ(ثابت V3_عدد& lhs);  // 0/1->0, X/Z->1
    V3_عدد& op_بیتها_Z(ثابت V3_عدد& lhs);  // Z->1, 0/1/X->0
    //
    V3_عدد& op_تخصیص_بده(ثابت V3_عدد& lhs);
    V3_عدد& op_تخصیص_بده_Non_XZ(ثابت V3_عدد& lhs, bool ignoreXZ = true);
    V3_عدد& opExtendS(ثابت V3_عدد& lhs, uint32_t lbits);  // Sign extension
    V3_عدد& opExtendXZ(ثابت V3_عدد& lhs, uint32_t lbits);  // X/Z extension
    V3_عدد& opRedOr(ثابت V3_عدد& lhs);
    V3_عدد& opRedAnd(ثابت V3_عدد& lhs);
    V3_عدد& opRedXor(ثابت V3_عدد& lhs);
    V3_عدد& op_شمار_بیتها(ثابت V3_عدد& عبارت, ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2,
                          ثابت V3_عدد& ctrl3);
    V3_عدد& op_شمار_Ones(ثابت V3_عدد& lhs);
    V3_عدد& opIsUnknown(ثابت V3_عدد& lhs);
    V3_عدد& opOneHot(ثابت V3_عدد& lhs);
    V3_عدد& opOneHot0(ثابت V3_عدد& lhs);
    V3_عدد& opCLog2(ثابت V3_عدد& lhs);
    V3_عدد& op_Most_مجموعه_بیت_P1(ثابت V3_عدد& lhs);
    V3_عدد& opClean(ثابت V3_عدد& lhs, uint32_t بیتها);
    V3_عدد& opConcat(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLenN(ثابت V3_عدد& lhs);
    V3_عدد& opRepl(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opRepl(ثابت V3_عدد& lhs, uint32_t rhsval);
    V3_عدد& op_جریان_L(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSel(ثابت V3_عدد& lhs, ثابت V3_عدد& msb, ثابت V3_عدد& lsb);
    V3_عدد& opSel(ثابت V3_عدد& lhs, uint32_t msbval, uint32_t lsbval);
    V3_عدد& opSelInto(ثابت V3_عدد& lhs, ثابت V3_عدد& lsb, صحيح عرض);
    V3_عدد& opSelInto(ثابت V3_عدد& lhs, صحيح lsbval, صحيح عرض);
    V3_عدد& opCvtPackString(ثابت V3_عدد& lhs);
    V3_عدد& op_مقصد_پایینی_N(ثابت V3_عدد& lhs);
    V3_عدد& op_مقصد_فوقانی_N(ثابت V3_عدد& lhs);
    V3_عدد& opCaseEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opCaseNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opWildEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opWildNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opBufIf1(ثابت V3_عدد& ens, ثابت V3_عدد& if1s);
    V3_عدد& op_مجموعه_Range(uint32_t lsb, uint32_t عرض, حرف بیت_مقدار);
    // "standard" math
    V3_عدد& opNot(ثابت V3_عدد& lhs);
    V3_عدد& opLogNot(ثابت V3_عدد& lhs);
    V3_عدد& opLogAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogIf(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNegate(ثابت V3_عدد& lhs);
    V3_عدد& op_جمع_کن(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
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
    V3_عدد& op_لغزش_R(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& op_لغزش_RS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_عدد& op_لغزش_L(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
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
    V3_عدد& op_Real_مقصد_بیتها(ثابت V3_عدد& lhs);
    V3_عدد& op_بیتها_مقصد_Real_D(ثابت V3_عدد& lhs);
    V3_عدد& opNegateD(ثابت V3_عدد& lhs);
    V3_عدد& op_جمع_کن_D(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
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
    V3_عدد& opAtoN(ثابت V3_عدد& lhs, صحيح پایه);
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
مضمن std::ostream& عملگر<<(std::ostream& os, ثابت V3_عدد& rhs) VL_MT_امن {
    أرجع os << rhs.ascii();
}

#پایان_اگر  // Guard
