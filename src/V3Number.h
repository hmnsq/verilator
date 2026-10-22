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

#ifndef VERILATOR_V3_العدد_H
#define VERILATOR_V3_العدد_H

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
class AstEnumItem;
class Ast_عقدة;
class Ast_عقدة_DType;
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
    constexpr operator en() ثابت { إرجاع m_e; }
    حرف ascii() ثابت { إرجاع m_e; }
    bool isComplex() ثابت { إرجاع m_e == COMPLEX; }
    bool isDouble() ثابت { إرجاع m_e == DOUBLE; }
    bool isEnum() ثابت { إرجاع m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ثابت { إرجاع m_e == SIGNED; }
    bool isString() ثابت { إرجاع m_e == STRING; }
    bool isUnsigned() ثابت { إرجاع m_e == UNSIGNED; }
};
constexpr bool operator==(ثابت VFormatAttr& lhs, ثابت VFormatAttr& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VFormatAttr& lhs, VFormatAttr::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, ثابت VFormatAttr& rhs) { إرجاع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VFormatAttr& rhs) {
    إرجاع os << rhs.ascii();
}

//============================================================================

class V3_العدد_Data final {
عام:
    // TYPES
    بنية قيمة_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_قيمة;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_قيمة_X;
        bool operator==(ثابت قيمة_And_X& other) ثابت VL_MT_SAFE {
            إرجاع m_قيمة == other.m_قيمة && m_قيمة_X == other.m_قيمة_X;
        }
    };

    تعداد class V3_العدد_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ثابت V3_العدد_Data_Type& rhs) VL_MT_SAFE {
        بدل (rhs) {
        حالة V3_العدد_Data_Type::UNINITIALIZED: إرجاع os << "UNINITIALIZED";
        حالة V3_العدد_Data_Type::LOGIC: إرجاع os << "LOGIC";
        حالة V3_العدد_Data_Type::DOUBLE: إرجاع os << "DOUBLE";
        حالة V3_العدد_Data_Type::STRING: إرجاع os << "STRING";
        }
        إرجاع os;
    }

خاص:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ساكن constexpr صحيح INLINE_كلمات = std::max(
        static_cast<size_t>(2), std::max(حجم(std::string) / حجم(قيمة_And_X),
                                         حجم(std::vector<قيمة_And_X>) / حجم(قيمة_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ساكن constexpr صحيح MAX_INLINE_WIDTH = INLINE_كلمات * حجم(قيمة_And_X) / 2 * 8;

    // MEMBERS
    اتحاد {
        std::array<قيمة_And_X, INLINE_كلمات> m_inline_العدد;
        std::vector<قيمة_And_X> m_dynamic_العدد;
        std::string m_string;
    };

    صحيح m_width = 0;  // Width (in bits) as specified/calculated
    V3_العدد_Data_Type m_type;

عام:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_العدد_Data()
        : m_type{V3_العدد_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_العدد_Data() { destroy_Stored_قيمة(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_العدد_Data(ثابت V3_العدد_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_العدد()) {
            init_Inline_العدد(other.m_inline_العدد);
        } وإلا إذا (other.is_Dynamic_العدد()) {
            init_Dynamic_العدد(other.m_dynamic_العدد);
        } وإلا إذا (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_العدد_Data& operator=(ثابت V3_العدد_Data& other) {
        إذا (other.is_Inline_العدد()) {
            destroy_Stored_قيمة();
            init_Inline_العدد(other.m_inline_العدد);
        } وإلا إذا (other.is_Dynamic_العدد()) {
            reinit_With_Or_Assign_Dynamic_العدد(other.m_dynamic_العدد);
        } وإلا إذا (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } وإلا {
            destroy_Stored_قيمة();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        إرجاع *this;
    }

    V3_العدد_Data(V3_العدد_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_العدد()) {
            init_Inline_العدد(other.m_inline_العدد);
        } وإلا إذا (other.is_Dynamic_العدد()) {
            init_Dynamic_العدد(std::move(other.m_dynamic_العدد));
        } وإلا إذا (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_العدد_Data_Type::UNINITIALIZED;
    }

    V3_العدد_Data& operator=(V3_العدد_Data&& other) {
        إذا (other.is_Inline_العدد()) {
            destroy_Stored_قيمة();
            init_Inline_العدد(other.m_inline_العدد);
        } وإلا إذا (other.is_Dynamic_العدد()) {
            reinit_With_Or_Assign_Dynamic_العدد(std::move(other.m_dynamic_العدد));
        } وإلا إذا (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } وإلا {
            destroy_Stored_قيمة();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_العدد_Data_Type::UNINITIALIZED;
        إرجاع *this;
    }

    // ACCESSORS
    قيمة_And_X* رقم() {
        UASSERT(is_العدد(), "`num` member accessed when data type is " << m_type);
        إرجاع is_Inline_العدد() ? m_inline_العدد.data() : m_dynamic_العدد.data();
    }
    ثابت قيمة_And_X* رقم() ثابت VL_MT_SAFE {
        UASSERT(is_العدد(), "`num` member accessed when data type is " << m_type);
        إرجاع is_Inline_العدد() ? m_inline_العدد.data() : m_dynamic_العدد.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        إرجاع m_string;
    }
    ثابت std::string& str() ثابت VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        إرجاع m_string;
    }

    صحيح width() ثابت VL_MT_SAFE { إرجاع m_width; }
    V3_العدد_Data_Type type() ثابت VL_MT_SAFE { إرجاع m_type; }

    // METHODS
    فراغ resize(صحيح bits_تعداد) {
        إذا (m_width == bits_تعداد) إرجاع;
        إذا (bits_To_كلمات(m_width) == bits_To_كلمات(bits_تعداد)) {
            m_width = bits_تعداد;
            إرجاع;
        }
        إذا (is_Dynamic_العدد()) {
            إذا (bits_تعداد > MAX_INLINE_WIDTH) {
                m_dynamic_العدد.resize(bits_To_كلمات(bits_تعداد));
            } وإلا {
                ثابت auto dynamicBits = std::move(m_dynamic_العدد);
                destroy_Dynamic_العدد();
                init_Inline_العدد();
                std::memcpy(m_inline_العدد.data(), dynamicBits.data(), حجم(m_inline_العدد));
            }
        } وإلا إذا (is_Inline_العدد()) {
            إذا (bits_تعداد > MAX_INLINE_WIDTH) {
                ثابت auto bits = m_inline_العدد;
                init_Dynamic_العدد(bits_To_كلمات(bits_تعداد));
                std::memcpy(m_dynamic_العدد.data(), bits.data(), حجم(bits));
            }
        }
        m_width = bits_تعداد;
    }

    فراغ setString() {
        // If there has been a string already it is kept intact.
        إذا (isString()) إرجاع;
        إذا (is_Dynamic_العدد()) destroy_Dynamic_العدد();
        initString();
        m_type = V3_العدد_Data_Type::STRING;
    }
    فراغ setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_العدد_Data_Type::STRING;
    }
    فراغ setString(ثابت std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_العدد_Data_Type::STRING;
    }

    فراغ setDouble() {
        destroy_Stored_قيمة();
        إذا (!is_Inline_العدد()) init_Inline_العدد();
        m_type = V3_العدد_Data_Type::DOUBLE;
        resize(64);
    }

    فراغ setLogic() {
        إذا (isString()) destroyString();
        إذا (!is_العدد()) {
            إذا (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_العدد();
            } وإلا {
                init_Dynamic_العدد(bits_To_كلمات(m_width));
            }
        }
        m_type = V3_العدد_Data_Type::LOGIC;
        resize(m_width);
    }

خاص:
    ساكن constexpr صحيح bits_To_كلمات(صحيح bits_تعداد) VL_PURE { إرجاع (bits_تعداد + 31) / 32; }

    bool is_العدد() ثابت VL_MT_SAFE {
        إرجاع m_type == V3_العدد_Data_Type::DOUBLE || m_type == V3_العدد_Data_Type::LOGIC;
    }
    bool is_Inline_العدد() ثابت VL_MT_SAFE {
        إرجاع (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_العدد_Data_Type::DOUBLE || m_type == V3_العدد_Data_Type::LOGIC);
    }
    bool is_Dynamic_العدد() ثابت VL_MT_SAFE {
        إرجاع (m_width > MAX_INLINE_WIDTH) && (m_type == V3_العدد_Data_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_SAFE { إرجاع m_type == V3_العدد_Data_Type::STRING; }

    template <typename... Args>
    فراغ init_Inline_العدد(Args&&... args) {
        new (&m_inline_العدد) std::array<قيمة_And_X, INLINE_كلمات>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    فراغ init_Dynamic_العدد(Args&&... args) {
        new (&m_dynamic_العدد) std::vector<قيمة_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    فراغ initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    فراغ destroy_Dynamic_العدد() { m_dynamic_العدد.~vector(); }
    فراغ destroyString() { m_string.~string(); }
    فراغ destroy_Stored_قيمة() {
        إذا (isString())
            destroyString();
        وإلا إذا (is_Dynamic_العدد())
            destroy_Dynamic_العدد();
    }

    template <typename T>
    فراغ reinit_With_Or_Assign_Dynamic_العدد(T&& s) {
        إذا (is_Dynamic_العدد()) {
            m_dynamic_العدد = std::forward<T>(s);
            إرجاع;
        }
        إذا (isString()) destroyString();
        init_Dynamic_العدد(std::forward<T>(s));
    }
    template <typename T>
    فراغ reinitWithOrAssignString(T&& s) {
        إذا (isString()) {
            m_string = std::forward<T>(s);
            إرجاع;
        }
        إذا (is_Dynamic_العدد()) destroy_Dynamic_العدد();
        initString(std::forward<T>(s));
    }
};

class V3_العدد final {
    // TYPES
    using قيمة_And_X = V3_العدد_Data::قيمة_And_X;
    using V3_العدد_Data_Type = V3_العدد_Data::V3_العدد_Data_Type;

    // MEMBERS
    V3_العدد_Data m_data;
    Ast_عقدة* m_nodep = nullptr;  // Parent node - for error reporting only
    File_سطر* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    فراغ setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_العدد& setSingleBits(حرف قيمة);
    فراغ fillBits(صحيح destLsb, صحيح width, حرف قيمة);
    فراغ copyBits(صحيح destLsb, ثابت V3_العدد& source, صحيح sourceLsb, صحيح width);
    V3_العدد& setString(ثابت string& str) {
        m_data.setString(str);
        إرجاع *this;
    }
    فراغ opCleanThis(bool warnOnTruncation = false);

عام:
    فراغ nodep(Ast_عقدة* nodep) VL_MT_STABLE;
    Ast_عقدة* nodep() ثابت VL_MT_SAFE { إرجاع m_nodep; }  // For debug only
    File_سطر* fileline() ثابت VL_MT_SAFE { إرجاع m_fileline; }
    V3_العدد& setZero();
    V3_العدد& setQuad(uint64_t قيمة);
    V3_العدد& setLong(uint32_t قيمة);
    V3_العدد& setLongS(int32_t قيمة);
    V3_العدد& setDouble(مزدوج قيمة);
    فراغ setBitX0(صحيح bit);
    فراغ setBit(صحيح bit, حرف قيمة) {  // Note: must be initialized as number and pre-zeroed!
        إذا (bit >= m_data.width()) إرجاع;
        ثابت uint32_t mask = (1UL << (bit & 31));
        قيمة_And_X& v = m_data.رقم()[bit / 32];
        إذا (قيمة == '0' || قيمة == 0) {
            v.m_قيمة &= ~mask;
            v.m_قيمة_X &= ~mask;
        } وإلا إذا (قيمة == '1' || قيمة == 1) {
            v.m_قيمة |= mask;
            v.m_قيمة_X &= ~mask;
        } وإلا إذا (قيمة == 'z' || قيمة == 2) {
            v.m_قيمة &= ~mask;
            v.m_قيمة_X |= mask;
        } وإلا {  // X
            v.m_قيمة |= mask;
            v.m_قيمة_X |= mask;
        }
    }

خاص:
    حرف bitIs(صحيح bit) ثابت {
        إذا (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            إرجاع '0';
        }
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        إرجاع ("01zx"[(((v.m_قيمة & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_قيمة_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    حرف bitIsExtend(صحيح bit, صحيح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        إذا (bit < 0) إرجاع '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        إذا (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
            // We do sign extend
            إرجاع ("01zx"[(((v.m_قيمة & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_قيمة_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        إرجاع ("01zx"[(((v.m_قيمة & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_قيمة_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

عام:
    bool bitIs0(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_العدد()) إرجاع false;
        إذا (bit < 0) إرجاع false;
        إذا (bit >= m_data.width()) إرجاع !bitIsXZ(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        إرجاع ((v.m_قيمة | v.m_قيمة_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_العدد()) إرجاع false;
        إذا (bit < 0) إرجاع false;
        إذا (bit >= m_data.width()) إرجاع false;
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        إرجاع ((v.m_قيمة & (1UL << (bit & 31))) && !(v.m_قيمة_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(صحيح bit) ثابت {
        // Correct number of zero bits/width matters
        إذا (!is_العدد()) إرجاع false;
        إذا (bit < 0) إرجاع false;
        إذا (bit >= m_data.width()) إرجاع bitIs1Extend(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        إرجاع ((v.m_قيمة & (1UL << (bit & 31))) && !(v.m_قيمة_X & (1UL << (bit & 31))));
    }
    bool bitIsX(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_العدد()) إرجاع false;
        إذا (bit < 0) إرجاع false;
        إذا (bit >= m_data.width()) إرجاع bitIsZ(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        إرجاع ((v.m_قيمة & (1UL << (bit & 31))) && (v.m_قيمة_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_العدد()) إرجاع false;
        إذا (bit < 0) إرجاع false;
        إذا (bit >= m_data.width()) إرجاع bitIsXZ(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        إرجاع ((v.m_قيمة_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_العدد()) إرجاع false;
        إذا (bit < 0) إرجاع false;
        إذا (bit >= m_data.width()) إرجاع bitIsZ(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        إرجاع ((~v.m_قيمة & (1UL << (bit & 31))) && (v.m_قيمة_X & (1UL << (bit & 31))));
    }

خاص:
    uint32_t bits_قيمة(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE {
        uint32_t v = 0;
        لكل (صحيح bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        إرجاع v;
    }

    صحيح تعداد_X(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE;
    صحيح تعداد_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE;

    صحيح كلمات() ثابت VL_MT_SAFE { إرجاع ((width() + 31) / 32); }
    uint32_t hiWordMask() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        إرجاع VL_MASK_I(width());
    }

    V3_العدد& opModDivGuts(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, bool is_modulus);

عام:
    // CONSTRUCTORS
    explicit V3_العدد(Ast_عقدة* nodep) { init(nodep, 1); }
    V3_العدد(Ast_عقدة* nodep, صحيح width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_العدد(Ast_عقدة* nodep, صحيح width, ثابت V3_العدد& قيمة) {
        init(nodep, width, width > 0);
        opAssign(قيمة);
    }
    V3_العدد(Ast_عقدة* nodep, صحيح width, uint32_t قيمة, bool sized = true) {
        init(nodep, width, sized);
        m_data.رقم()[0].m_قيمة = قيمة;
        opCleanThis();
    }
    V3_العدد(File_سطر* flp, صحيح width, uint32_t قيمة, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.رقم()[0].m_قيمة = قيمة;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_العدد_Literal {};  // For creator type-overload selection
    V3_العدد(Ast_عقدة* nodep, Verilog_العدد_Literal, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3_العدد(File_سطر* flp, Verilog_العدد_Literal, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_العدد(Ast_عقدة* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_العدد(Ast_عقدة* nodep, Double, مزدوج قيمة) {
        init(nodep, 64);
        setDouble(قيمة);
    }
    class String {};
    V3_العدد(Ast_عقدة* nodep, String, ثابت string& قيمة) {
        init(nodep);
        setString(قيمة);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_العدد(Ast_عقدة* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_العدد(Ast_عقدة* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_العدد(ثابت V3_العدد* nump, صحيح width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_العدد(ثابت V3_العدد* nump, صحيح width, uint32_t قيمة) {
        init(nullptr, width);
        m_data.رقم()[0].m_قيمة = قيمة;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_العدد(Ast_عقدة* nodep, ثابت Ast_عقدة_DType* nodedtypep);

    V3_العدد(ثابت V3_العدد& other) = افتراضي;
    V3_العدد& operator=(ثابت V3_العدد& other) = افتراضي;

    V3_العدد(V3_العدد&& other) = افتراضي;
    V3_العدد& operator=(V3_العدد&& other) = افتراضي;

    ~V3_العدد() = افتراضي;

خاص:
    فراغ selfTestThis();
    فراغ create(Ast_عقدة* nodep, ثابت حرف* sourcep) {
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
    فراغ init(Ast_عقدة* nodep, صحيح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        إذا (swidth >= 0) {
            إذا (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            لكل (صحيح i = 0; i < كلمات(); ++i) m_data.رقم()[i] = {0, 0};
        } وإلا {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    ساكن string displayPad(size_t fmtsize, حرف pad, bool يسار, ثابت string& in) VL_PURE;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        إرجاع displayed(m_fileline, vformat);
    }
    فراغ warnTooMany(ثابت string& قيمة);

عام:
    فراغ v3errorEnd(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3errorEndFatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ width(صحيح width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_العدد& setAllBitsXRemoved();
    V3_العدد& setAllBitsX();
    V3_العدد& setAllBitsZ();
    V3_العدد& setAllBits0();
    V3_العدد& setAllBits1();
    V3_العدد& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_العدد& setMask(صحيح nbits, صحيح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت Ast_عقدة* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(File_سطر* fl, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayedEnum(ثابت AstSFormatArg* argp, ثابت string& vformat) ثابت VL_MT_STABLE;
    string displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE;
    ساكن string displayedEnumName(ثابت AstEnumItem* itemp);
    ساكن bool displayedFmtHasArg(حرف format, bool isScan);
    string emitC() ثابت VL_MT_STABLE;
    صحيح width() ثابت VL_MT_SAFE { إرجاع m_data.width(); }
    صحيح widthToFit() ثابت;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ثابت VL_MT_SAFE { إرجاع m_data.m_sized; }
    bool autoExtend() ثابت VL_MT_SAFE { إرجاع m_data.m_autoExtend; }
    bool isFromString() ثابت { إرجاع m_data.m_fromString; }
    bool hasOrigParamName() ثابت { إرجاع m_data.m_hasOrigParamName; }
    فراغ hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_العدد_Data_Type dataType() ثابت VL_MT_SAFE { إرجاع m_data.type(); }
    فراغ dataType(V3_العدد_Data_Type newType) {
        إذا (dataType() == newType) إرجاع;
        UASSERT(newType != V3_العدد_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        بدل (newType) {
        حالة V3_العدد_Data_Type::STRING: m_data.setString(); اكسر;
        حالة V3_العدد_Data_Type::DOUBLE: m_data.setDouble(); اكسر;
        حالة V3_العدد_Data_Type::LOGIC: m_data.setLogic(); اكسر;
        حالة V3_العدد_Data_Type::UNINITIALIZED: اكسر;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_SAFE { إرجاع m_data.m_signed; }
    فراغ isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_SAFE { إرجاع dataType() == V3_العدد_Data_Type::DOUBLE; }
    bool isString() ثابت VL_MT_SAFE { إرجاع dataType() == V3_العدد_Data_Type::STRING; }
    bool isOpaque() ثابت VL_MT_SAFE { إرجاع isDouble() || isString(); }
    bool is_العدد() ثابت VL_MT_SAFE {
        إرجاع m_data.type() == V3_العدد_Data_Type::LOGIC
               || m_data.type() == V3_العدد_Data_Type::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        إرجاع !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ثابت VL_MT_SAFE { إرجاع m_data.m_is1Step; }
    bool isNull() ثابت VL_MT_SAFE { إرجاع m_data.m_isNull; }
    bool isFourState() ثابت VL_MT_SAFE;
    bool hasZ() ثابت {
        إذا (isString()) إرجاع false;
        لكل (صحيح i = 0; i < كلمات(); ++i) {
            ثابت قيمة_And_X v = m_data.رقم()[i];
            إذا ((~v.m_قيمة) & v.m_قيمة_X) إرجاع true;
        }
        إرجاع false;
    }
    bool isAllZ() ثابت VL_MT_SAFE;
    bool isAllX() ثابت VL_MT_SAFE;
    bool isEqZero() ثابت VL_MT_SAFE;
    bool isNeqZero() ثابت;
    bool isBitsZero(صحيح msb, صحيح lsb) ثابت;
    bool isBroken(صحيح vwidth) ثابت;
    bool isEqOne() ثابت;
    bool isEqAllOnes(صحيح optwidth = 0) ثابت;
    bool isCaseEq(ثابت V3_العدد& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_العدد& rhs) ثابت;  // operator< with XZ compared
    bool isAnyX() ثابت VL_MT_SAFE;
    bool isAnyXZ() ثابت;
    bool isAnyZ() ثابت VL_MT_SAFE;
    bool isMsbXZ() ثابت {
        // Correct number of zero bits/width matters
        إرجاع bitIsXZ(width() - 1);
    }
    bool fitsInUInt() ثابت VL_MT_SAFE;
    uint32_t toUInt() ثابت VL_MT_SAFE;
    int32_t toSInt() ثابت VL_MT_SAFE;
    uint64_t toUQuad() ثابت VL_MT_SAFE;
    int64_t toSQuad() ثابت VL_MT_SAFE;
    string toString() ثابت VL_MT_SAFE;
    string toDecimalS() ثابت VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() ثابت VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() ثابت VL_MT_STABLE;
    مزدوج toDouble() ثابت VL_MT_SAFE;
    V3Hash toHash() ثابت;
    uint32_t edataWord(صحيح eword) ثابت;
    uint8_t dataByte(صحيح byte) ثابت;
    uint32_t تعداد_Bits(ثابت V3_العدد& ctrl) ثابت;
    uint32_t تعداد_Bits(ثابت V3_العدد& ctrl1, ثابت V3_العدد& ctrl2, ثابت V3_العدد& ctrl3) ثابت;
    uint32_t تعداد_Ones() ثابت;
    uint32_t mostSetBitP1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ثابت V3_العدد& rhs) ثابت { إرجاع isLtXZ(rhs); }

    // STATICS
    ساكن bool epsilonEqual(مزدوج a, مزدوج b);  // True if number within Epsilon of the other
    ساكن bool epsilonIntegral(مزدوج a);  // True if number rounds to integer within Epsilon
    ساكن صحيح log2b(uint32_t رقم);
    ساكن صحيح log2bQuad(uint64_t رقم);
    ساكن فراغ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_العدد& opBitsNonXZ(ثابت V3_العدد& lhs);  // 0/1->1, X/Z->0
    V3_العدد& opBitsOne(ثابت V3_العدد& lhs);  // 1->1, 0/X/Z->0
    V3_العدد& opBitsXZ(ثابت V3_العدد& lhs);  // 0/1->0, X/Z->1
    V3_العدد& opBitsZ(ثابت V3_العدد& lhs);  // Z->1, 0/1/X->0
    //
    V3_العدد& opAssign(ثابت V3_العدد& lhs);
    V3_العدد& opAssignNonXZ(ثابت V3_العدد& lhs, bool ignoreXZ = true);
    V3_العدد& opExtendS(ثابت V3_العدد& lhs, uint32_t lbits);  // Sign extension
    V3_العدد& opExtendXZ(ثابت V3_العدد& lhs, uint32_t lbits);  // X/Z extension
    V3_العدد& opRedOr(ثابت V3_العدد& lhs);
    V3_العدد& opRedAnd(ثابت V3_العدد& lhs);
    V3_العدد& opRedXor(ثابت V3_العدد& lhs);
    V3_العدد& op_تعداد_Bits(ثابت V3_العدد& تعبير, ثابت V3_العدد& ctrl1, ثابت V3_العدد& ctrl2,
                          ثابت V3_العدد& ctrl3);
    V3_العدد& op_تعداد_Ones(ثابت V3_العدد& lhs);
    V3_العدد& opIsUnknown(ثابت V3_العدد& lhs);
    V3_العدد& opOneHot(ثابت V3_العدد& lhs);
    V3_العدد& opOneHot0(ثابت V3_العدد& lhs);
    V3_العدد& opCLog2(ثابت V3_العدد& lhs);
    V3_العدد& opMostSetBitP1(ثابت V3_العدد& lhs);
    V3_العدد& opClean(ثابت V3_العدد& lhs, uint32_t bits);
    V3_العدد& opConcat(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLenN(ثابت V3_العدد& lhs);
    V3_العدد& opRepl(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opRepl(ثابت V3_العدد& lhs, uint32_t rhsval);
    V3_العدد& opStreamL(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opSel(ثابت V3_العدد& lhs, ثابت V3_العدد& msb, ثابت V3_العدد& lsb);
    V3_العدد& opSel(ثابت V3_العدد& lhs, uint32_t msbval, uint32_t lsbval);
    V3_العدد& opSelInto(ثابت V3_العدد& lhs, ثابت V3_العدد& lsb, صحيح width);
    V3_العدد& opSelInto(ثابت V3_العدد& lhs, صحيح lsbval, صحيح width);
    V3_العدد& opCvtPackString(ثابت V3_العدد& lhs);
    V3_العدد& opToLowerN(ثابت V3_العدد& lhs);
    V3_العدد& opToUpperN(ثابت V3_العدد& lhs);
    V3_العدد& opCaseEq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opCaseNeq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opWildEq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opWildNeq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opBufIf1(ثابت V3_العدد& ens, ثابت V3_العدد& if1s);
    V3_العدد& opSetRange(uint32_t lsb, uint32_t width, حرف bit_قيمة);
    // "standard" math
    V3_العدد& opNot(ثابت V3_العدد& lhs);
    V3_العدد& opLogNot(ثابت V3_العدد& lhs);
    V3_العدد& opLogAnd(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLogOr(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLogEq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLogIf(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opNegate(ثابت V3_العدد& lhs);
    V3_العدد& opAdd(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opSub(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opMul(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opMulS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed
    V3_العدد& opDiv(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opDivS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed
    V3_العدد& opModDiv(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opModDivS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed
    V3_العدد& opPow(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, bool lsign = false,
                    bool rsign = false);
    V3_العدد& opPowSU(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed lhs, unsigned rhs
    V3_العدد& opPowSS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed lhs, signed rhs
    V3_العدد& opPowUS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Unsigned lhs, signed rhs
    V3_العدد& opAnd(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opXor(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opOr(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opShiftR(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opShiftRS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_العدد& opShiftL(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    // Comparisons
    V3_العدد& opEq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opNeq(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opGt(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opGtS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed
    V3_العدد& opGte(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opGteS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed
    V3_العدد& opLt(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLtS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed
    V3_العدد& opLte(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLteS(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);  // Signed

    // "D" - double (aka real) math
    V3_العدد& opIToRD(ثابت V3_العدد& lhs, bool isSigned = false);
    V3_العدد& opISToRD(ثابت V3_العدد& lhs) { إرجاع opIToRD(lhs, true); }
    V3_العدد& opRToIS(ثابت V3_العدد& lhs);
    V3_العدد& opRToIRoundS(ثابت V3_العدد& lhs);
    V3_العدد& opRealToBits(ثابت V3_العدد& lhs);
    V3_العدد& opBitsToRealD(ثابت V3_العدد& lhs);
    V3_العدد& opNegateD(ثابت V3_العدد& lhs);
    V3_العدد& opAddD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opSubD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opMulD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opDivD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opPowD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    // Comparisons
    V3_العدد& opEqD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opNeqD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opGtD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opGteD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLtD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLteD(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);

    // "N" - string operations
    V3_العدد& opAtoN(ثابت V3_العدد& lhs, صحيح base);
    V3_العدد& opNToI(ثابت V3_العدد& lhs);
    V3_العدد& opPutcN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, ثابت V3_العدد& ths);
    V3_العدد& opGetcN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opSubstrN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, ثابت V3_العدد& ths);
    V3_العدد& opCompareNN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs, bool ignoreCase);
    V3_العدد& opConcatN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opReplN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opReplN(ثابت V3_العدد& lhs, uint32_t rhsval);
    V3_العدد& opEqN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opNeqN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opGtN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opGteN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLtN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
    V3_العدد& opLteN(ثابت V3_العدد& lhs, ثابت V3_العدد& rhs);
};
مضمن std::ostream& operator<<(std::ostream& os, ثابت V3_العدد& rhs) VL_MT_SAFE {
    إرجاع os << rhs.ascii();
}

#endif  // Guard
