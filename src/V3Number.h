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

#ifndef VERILATOR_V3_رقم_H
#define VERILATOR_V3_رقم_H

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
    constexpr operator en() ثابت { أرجع m_e; }
    حرف ascii() ثابت { أرجع m_e; }
    bool isComplex() ثابت { أرجع m_e == COMPLEX; }
    bool isDouble() ثابت { أرجع m_e == DOUBLE; }
    bool isEnum() ثابت { أرجع m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ثابت { أرجع m_e == SIGNED; }
    bool isString() ثابت { أرجع m_e == STRING; }
    bool isUnsigned() ثابت { أرجع m_e == UNSIGNED; }
};
constexpr bool operator==(ثابت VFormatAttr& lhs, ثابت VFormatAttr& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VFormatAttr& lhs, VFormatAttr::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, ثابت VFormatAttr& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& operator<<(std::ostream& os, ثابت VFormatAttr& rhs) {
    أرجع os << rhs.ascii();
}

//============================================================================

class V3_رقم_Data final {
عام:
    // TYPES
    بنية قيمة_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_قيمة;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_قيمة_X;
        bool operator==(ثابت قيمة_And_X& other) ثابت VL_MT_SAFE {
            أرجع m_قيمة == other.m_قيمة && m_قيمة_X == other.m_قيمة_X;
        }
    };

    تعداد class V3_رقم_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ثابت V3_رقم_Data_Type& rhs) VL_MT_SAFE {
        بدل (rhs) {
        حالة V3_رقم_Data_Type::UNINITIALIZED: أرجع os << "UNINITIALIZED";
        حالة V3_رقم_Data_Type::LOGIC: أرجع os << "LOGIC";
        حالة V3_رقم_Data_Type::DOUBLE: أرجع os << "DOUBLE";
        حالة V3_رقم_Data_Type::STRING: أرجع os << "STRING";
        }
        أرجع os;
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
        std::array<قيمة_And_X, INLINE_كلمات> m_inline_رقم;
        std::vector<قيمة_And_X> m_dynamic_رقم;
        std::string m_string;
    };

    صحيح m_width = 0;  // Width (in bits) as specified/calculated
    V3_رقم_Data_Type m_type;

عام:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_رقم_Data()
        : m_type{V3_رقم_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_رقم_Data() { destroy_Stored_قيمة(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_رقم_Data(ثابت V3_رقم_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_رقم()) {
            init_Inline_رقم(other.m_inline_رقم);
        } وإلا إذا (other.is_Dynamic_رقم()) {
            init_Dynamic_رقم(other.m_dynamic_رقم);
        } وإلا إذا (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_رقم_Data& operator=(ثابت V3_رقم_Data& other) {
        إذا (other.is_Inline_رقم()) {
            destroy_Stored_قيمة();
            init_Inline_رقم(other.m_inline_رقم);
        } وإلا إذا (other.is_Dynamic_رقم()) {
            reinit_With_Or_Assign_Dynamic_رقم(other.m_dynamic_رقم);
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
        أرجع *this;
    }

    V3_رقم_Data(V3_رقم_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_رقم()) {
            init_Inline_رقم(other.m_inline_رقم);
        } وإلا إذا (other.is_Dynamic_رقم()) {
            init_Dynamic_رقم(std::move(other.m_dynamic_رقم));
        } وإلا إذا (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_رقم_Data_Type::UNINITIALIZED;
    }

    V3_رقم_Data& operator=(V3_رقم_Data&& other) {
        إذا (other.is_Inline_رقم()) {
            destroy_Stored_قيمة();
            init_Inline_رقم(other.m_inline_رقم);
        } وإلا إذا (other.is_Dynamic_رقم()) {
            reinit_With_Or_Assign_Dynamic_رقم(std::move(other.m_dynamic_رقم));
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
        other.m_type = V3_رقم_Data_Type::UNINITIALIZED;
        أرجع *this;
    }

    // ACCESSORS
    قيمة_And_X* رقم() {
        UASSERT(is_رقم(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_رقم() ? m_inline_رقم.data() : m_dynamic_رقم.data();
    }
    ثابت قيمة_And_X* رقم() ثابت VL_MT_SAFE {
        UASSERT(is_رقم(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_رقم() ? m_inline_رقم.data() : m_dynamic_رقم.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }
    ثابت std::string& str() ثابت VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        أرجع m_string;
    }

    صحيح width() ثابت VL_MT_SAFE { أرجع m_width; }
    V3_رقم_Data_Type type() ثابت VL_MT_SAFE { أرجع m_type; }

    // METHODS
    فراغ resize(صحيح bits_عدد) {
        إذا (m_width == bits_عدد) أرجع;
        إذا (bits_To_كلمات(m_width) == bits_To_كلمات(bits_عدد)) {
            m_width = bits_عدد;
            أرجع;
        }
        إذا (is_Dynamic_رقم()) {
            إذا (bits_عدد > MAX_INLINE_WIDTH) {
                m_dynamic_رقم.resize(bits_To_كلمات(bits_عدد));
            } وإلا {
                ثابت auto dynamicBits = std::move(m_dynamic_رقم);
                destroy_Dynamic_رقم();
                init_Inline_رقم();
                std::memcpy(m_inline_رقم.data(), dynamicBits.data(), حجم(m_inline_رقم));
            }
        } وإلا إذا (is_Inline_رقم()) {
            إذا (bits_عدد > MAX_INLINE_WIDTH) {
                ثابت auto bits = m_inline_رقم;
                init_Dynamic_رقم(bits_To_كلمات(bits_عدد));
                std::memcpy(m_dynamic_رقم.data(), bits.data(), حجم(bits));
            }
        }
        m_width = bits_عدد;
    }

    فراغ setString() {
        // If there has been a string already it is kept intact.
        إذا (isString()) أرجع;
        إذا (is_Dynamic_رقم()) destroy_Dynamic_رقم();
        initString();
        m_type = V3_رقم_Data_Type::STRING;
    }
    فراغ setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_رقم_Data_Type::STRING;
    }
    فراغ setString(ثابت std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_رقم_Data_Type::STRING;
    }

    فراغ setDouble() {
        destroy_Stored_قيمة();
        إذا (!is_Inline_رقم()) init_Inline_رقم();
        m_type = V3_رقم_Data_Type::DOUBLE;
        resize(64);
    }

    فراغ setLogic() {
        إذا (isString()) destroyString();
        إذا (!is_رقم()) {
            إذا (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_رقم();
            } وإلا {
                init_Dynamic_رقم(bits_To_كلمات(m_width));
            }
        }
        m_type = V3_رقم_Data_Type::LOGIC;
        resize(m_width);
    }

خاص:
    ساكن constexpr صحيح bits_To_كلمات(صحيح bits_عدد) VL_PURE { أرجع (bits_عدد + 31) / 32; }

    bool is_رقم() ثابت VL_MT_SAFE {
        أرجع m_type == V3_رقم_Data_Type::DOUBLE || m_type == V3_رقم_Data_Type::LOGIC;
    }
    bool is_Inline_رقم() ثابت VL_MT_SAFE {
        أرجع (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_رقم_Data_Type::DOUBLE || m_type == V3_رقم_Data_Type::LOGIC);
    }
    bool is_Dynamic_رقم() ثابت VL_MT_SAFE {
        أرجع (m_width > MAX_INLINE_WIDTH) && (m_type == V3_رقم_Data_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_SAFE { أرجع m_type == V3_رقم_Data_Type::STRING; }

    template <typename... وسائط_2>
    فراغ init_Inline_رقم(وسائط_2&&... وسائط) {
        new (&m_inline_رقم) std::array<قيمة_And_X, INLINE_كلمات>(std::forward<وسائط_2>(وسائط)...);
    }
    template <typename... وسائط_2>
    فراغ init_Dynamic_رقم(وسائط_2&&... وسائط) {
        new (&m_dynamic_رقم) std::vector<قيمة_And_X>(std::forward<وسائط_2>(وسائط)...);
    }
    template <typename... وسائط_2>
    فراغ initString(وسائط_2&&... وسائط) {
        new (&m_string) std::string(std::forward<وسائط_2>(وسائط)...);
    }

    string formatDecimal();

    فراغ destroy_Dynamic_رقم() { m_dynamic_رقم.~vector(); }
    فراغ destroyString() { m_string.~string(); }
    فراغ destroy_Stored_قيمة() {
        إذا (isString())
            destroyString();
        وإلا إذا (is_Dynamic_رقم())
            destroy_Dynamic_رقم();
    }

    template <typename T>
    فراغ reinit_With_Or_Assign_Dynamic_رقم(T&& s) {
        إذا (is_Dynamic_رقم()) {
            m_dynamic_رقم = std::forward<T>(s);
            أرجع;
        }
        إذا (isString()) destroyString();
        init_Dynamic_رقم(std::forward<T>(s));
    }
    template <typename T>
    فراغ reinitWithOrAssignString(T&& s) {
        إذا (isString()) {
            m_string = std::forward<T>(s);
            أرجع;
        }
        إذا (is_Dynamic_رقم()) destroy_Dynamic_رقم();
        initString(std::forward<T>(s));
    }
};

class V3_رقم final {
    // TYPES
    using قيمة_And_X = V3_رقم_Data::قيمة_And_X;
    using V3_رقم_Data_Type = V3_رقم_Data::V3_رقم_Data_Type;

    // MEMBERS
    V3_رقم_Data m_data;
    Ast_عقدة* m_nodep = nullptr;  // Parent node - for error reporting only
    File_سطر* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    فراغ setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_رقم& setSingleBits(حرف قيمة);
    فراغ fillBits(صحيح destLsb, صحيح width, حرف قيمة);
    فراغ copyBits(صحيح destLsb, ثابت V3_رقم& source, صحيح sourceLsb, صحيح width);
    V3_رقم& setString(ثابت string& str) {
        m_data.setString(str);
        أرجع *this;
    }
    فراغ opCleanThis(bool warnOnTruncation = false);

عام:
    فراغ nodep(Ast_عقدة* nodep) VL_MT_STABLE;
    Ast_عقدة* nodep() ثابت VL_MT_SAFE { أرجع m_nodep; }  // For debug only
    File_سطر* fileline() ثابت VL_MT_SAFE { أرجع m_fileline; }
    V3_رقم& setZero();
    V3_رقم& setQuad(uint64_t قيمة);
    V3_رقم& setLong(uint32_t قيمة);
    V3_رقم& setLongS(int32_t قيمة);
    V3_رقم& setDouble(مزدوج قيمة);
    فراغ setBitX0(صحيح bit);
    فراغ setBit(صحيح bit, حرف قيمة) {  // Note: must be initialized as number and pre-zeroed!
        إذا (bit >= m_data.width()) أرجع;
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
            أرجع '0';
        }
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        أرجع ("01zx"[(((v.m_قيمة & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_قيمة_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    حرف bitIsExtend(صحيح bit, صحيح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        إذا (bit < 0) أرجع '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        إذا (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
            // We do sign extend
            أرجع ("01zx"[(((v.m_قيمة & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_قيمة_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        أرجع ("01zx"[(((v.m_قيمة & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_قيمة_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

عام:
    bool bitIs0(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_رقم()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع !bitIsXZ(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        أرجع ((v.m_قيمة | v.m_قيمة_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_رقم()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع false;
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        أرجع ((v.m_قيمة & (1UL << (bit & 31))) && !(v.m_قيمة_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(صحيح bit) ثابت {
        // Correct number of zero bits/width matters
        إذا (!is_رقم()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع bitIs1Extend(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        أرجع ((v.m_قيمة & (1UL << (bit & 31))) && !(v.m_قيمة_X & (1UL << (bit & 31))));
    }
    bool bitIsX(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_رقم()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع bitIsZ(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        أرجع ((v.m_قيمة & (1UL << (bit & 31))) && (v.m_قيمة_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_رقم()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع bitIsXZ(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        أرجع ((v.m_قيمة_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_رقم()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع bitIsZ(m_data.width() - 1);
        ثابت قيمة_And_X v = m_data.رقم()[bit / 32];
        أرجع ((~v.m_قيمة & (1UL << (bit & 31))) && (v.m_قيمة_X & (1UL << (bit & 31))));
    }

خاص:
    uint32_t bits_قيمة(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE {
        uint32_t v = 0;
        لكل (صحيح bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        أرجع v;
    }

    صحيح عدد_X(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE;
    صحيح عدد_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE;

    صحيح كلمات() ثابت VL_MT_SAFE { أرجع ((width() + 31) / 32); }
    uint32_t hiWordMask() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        أرجع VL_MASK_I(width());
    }

    V3_رقم& opModDivGuts(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs, bool is_modulus);

عام:
    // CONSTRUCTORS
    explicit V3_رقم(Ast_عقدة* nodep) { init(nodep, 1); }
    V3_رقم(Ast_عقدة* nodep, صحيح width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_رقم(Ast_عقدة* nodep, صحيح width, ثابت V3_رقم& قيمة) {
        init(nodep, width, width > 0);
        opAssign(قيمة);
    }
    V3_رقم(Ast_عقدة* nodep, صحيح width, uint32_t قيمة, bool sized = true) {
        init(nodep, width, sized);
        m_data.رقم()[0].m_قيمة = قيمة;
        opCleanThis();
    }
    V3_رقم(File_سطر* flp, صحيح width, uint32_t قيمة, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.رقم()[0].m_قيمة = قيمة;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_رقم_Literal {};  // For creator type-overload selection
    V3_رقم(Ast_عقدة* nodep, Verilog_رقم_Literal, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3_رقم(File_سطر* flp, Verilog_رقم_Literal, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_رقم(Ast_عقدة* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_رقم(Ast_عقدة* nodep, Double, مزدوج قيمة) {
        init(nodep, 64);
        setDouble(قيمة);
    }
    class String {};
    V3_رقم(Ast_عقدة* nodep, String, ثابت string& قيمة) {
        init(nodep);
        setString(قيمة);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_رقم(Ast_عقدة* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_رقم(Ast_عقدة* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_رقم(ثابت V3_رقم* nump, صحيح width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_رقم(ثابت V3_رقم* nump, صحيح width, uint32_t قيمة) {
        init(nullptr, width);
        m_data.رقم()[0].m_قيمة = قيمة;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_رقم(Ast_عقدة* nodep, ثابت Ast_عقدة_DType* nodedtypep);

    V3_رقم(ثابت V3_رقم& other) = افتراضي;
    V3_رقم& operator=(ثابت V3_رقم& other) = افتراضي;

    V3_رقم(V3_رقم&& other) = افتراضي;
    V3_رقم& operator=(V3_رقم&& other) = افتراضي;

    ~V3_رقم() = افتراضي;

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
        أرجع displayed(m_fileline, vformat);
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
    V3_رقم& setAllBitsXRemoved();
    V3_رقم& setAllBitsX();
    V3_رقم& setAllBitsZ();
    V3_رقم& setAllBits0();
    V3_رقم& setAllBits1();
    V3_رقم& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_رقم& setMask(صحيح nbits, صحيح lsb = 0);

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
    صحيح width() ثابت VL_MT_SAFE { أرجع m_data.width(); }
    صحيح widthToFit() ثابت;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ثابت VL_MT_SAFE { أرجع m_data.m_sized; }
    bool autoExtend() ثابت VL_MT_SAFE { أرجع m_data.m_autoExtend; }
    bool isFromString() ثابت { أرجع m_data.m_fromString; }
    bool hasOrigParamName() ثابت { أرجع m_data.m_hasOrigParamName; }
    فراغ hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_رقم_Data_Type dataType() ثابت VL_MT_SAFE { أرجع m_data.type(); }
    فراغ dataType(V3_رقم_Data_Type newType) {
        إذا (dataType() == newType) أرجع;
        UASSERT(newType != V3_رقم_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        بدل (newType) {
        حالة V3_رقم_Data_Type::STRING: m_data.setString(); اكسر;
        حالة V3_رقم_Data_Type::DOUBLE: m_data.setDouble(); اكسر;
        حالة V3_رقم_Data_Type::LOGIC: m_data.setLogic(); اكسر;
        حالة V3_رقم_Data_Type::UNINITIALIZED: اكسر;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_SAFE { أرجع m_data.m_signed; }
    فراغ isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_SAFE { أرجع dataType() == V3_رقم_Data_Type::DOUBLE; }
    bool isString() ثابت VL_MT_SAFE { أرجع dataType() == V3_رقم_Data_Type::STRING; }
    bool isOpaque() ثابت VL_MT_SAFE { أرجع isDouble() || isString(); }
    bool is_رقم() ثابت VL_MT_SAFE {
        أرجع m_data.type() == V3_رقم_Data_Type::LOGIC
               || m_data.type() == V3_رقم_Data_Type::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        أرجع !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ثابت VL_MT_SAFE { أرجع m_data.m_is1Step; }
    bool isNull() ثابت VL_MT_SAFE { أرجع m_data.m_isNull; }
    bool isFourState() ثابت VL_MT_SAFE;
    bool hasZ() ثابت {
        إذا (isString()) أرجع false;
        لكل (صحيح i = 0; i < كلمات(); ++i) {
            ثابت قيمة_And_X v = m_data.رقم()[i];
            إذا ((~v.m_قيمة) & v.m_قيمة_X) أرجع true;
        }
        أرجع false;
    }
    bool isAllZ() ثابت VL_MT_SAFE;
    bool isAllX() ثابت VL_MT_SAFE;
    bool isEqZero() ثابت VL_MT_SAFE;
    bool isNeqZero() ثابت;
    bool isBitsZero(صحيح msb, صحيح lsb) ثابت;
    bool isBroken(صحيح vwidth) ثابت;
    bool isEqOne() ثابت;
    bool isEqAllOnes(صحيح optwidth = 0) ثابت;
    bool isCaseEq(ثابت V3_رقم& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_رقم& rhs) ثابت;  // operator< with XZ compared
    bool isAnyX() ثابت VL_MT_SAFE;
    bool isAnyXZ() ثابت;
    bool isAnyZ() ثابت VL_MT_SAFE;
    bool isMsbXZ() ثابت {
        // Correct number of zero bits/width matters
        أرجع bitIsXZ(width() - 1);
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
    uint32_t عدد_Bits(ثابت V3_رقم& ctrl) ثابت;
    uint32_t عدد_Bits(ثابت V3_رقم& ctrl1, ثابت V3_رقم& ctrl2, ثابت V3_رقم& ctrl3) ثابت;
    uint32_t عدد_Ones() ثابت;
    uint32_t mostSetBitP1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ثابت V3_رقم& rhs) ثابت { أرجع isLtXZ(rhs); }

    // STATICS
    ساكن bool epsilonEqual(مزدوج a, مزدوج b);  // True if number within Epsilon of the other
    ساكن bool epsilonIntegral(مزدوج a);  // True if number rounds to integer within Epsilon
    ساكن صحيح log2b(uint32_t رقم);
    ساكن صحيح log2bQuad(uint64_t رقم);
    ساكن فراغ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_رقم& opBitsNonXZ(ثابت V3_رقم& lhs);  // 0/1->1, X/Z->0
    V3_رقم& opBitsOne(ثابت V3_رقم& lhs);  // 1->1, 0/X/Z->0
    V3_رقم& opBitsXZ(ثابت V3_رقم& lhs);  // 0/1->0, X/Z->1
    V3_رقم& opBitsZ(ثابت V3_رقم& lhs);  // Z->1, 0/1/X->0
    //
    V3_رقم& opAssign(ثابت V3_رقم& lhs);
    V3_رقم& opAssignNonXZ(ثابت V3_رقم& lhs, bool ignoreXZ = true);
    V3_رقم& opExtendS(ثابت V3_رقم& lhs, uint32_t lbits);  // Sign extension
    V3_رقم& opExtendXZ(ثابت V3_رقم& lhs, uint32_t lbits);  // X/Z extension
    V3_رقم& opRedOr(ثابت V3_رقم& lhs);
    V3_رقم& opRedAnd(ثابت V3_رقم& lhs);
    V3_رقم& opRedXor(ثابت V3_رقم& lhs);
    V3_رقم& op_عدد_Bits(ثابت V3_رقم& تعبير, ثابت V3_رقم& ctrl1, ثابت V3_رقم& ctrl2,
                          ثابت V3_رقم& ctrl3);
    V3_رقم& op_عدد_Ones(ثابت V3_رقم& lhs);
    V3_رقم& opIsUnknown(ثابت V3_رقم& lhs);
    V3_رقم& opOneHot(ثابت V3_رقم& lhs);
    V3_رقم& opOneHot0(ثابت V3_رقم& lhs);
    V3_رقم& opCLog2(ثابت V3_رقم& lhs);
    V3_رقم& opMostSetBitP1(ثابت V3_رقم& lhs);
    V3_رقم& opClean(ثابت V3_رقم& lhs, uint32_t bits);
    V3_رقم& opConcat(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLenN(ثابت V3_رقم& lhs);
    V3_رقم& opRepl(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opRepl(ثابت V3_رقم& lhs, uint32_t rhsval);
    V3_رقم& opStreamL(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opSel(ثابت V3_رقم& lhs, ثابت V3_رقم& msb, ثابت V3_رقم& lsb);
    V3_رقم& opSel(ثابت V3_رقم& lhs, uint32_t msbval, uint32_t lsbval);
    V3_رقم& opSelInto(ثابت V3_رقم& lhs, ثابت V3_رقم& lsb, صحيح width);
    V3_رقم& opSelInto(ثابت V3_رقم& lhs, صحيح lsbval, صحيح width);
    V3_رقم& opCvtPackString(ثابت V3_رقم& lhs);
    V3_رقم& opToLowerN(ثابت V3_رقم& lhs);
    V3_رقم& opToUpperN(ثابت V3_رقم& lhs);
    V3_رقم& opCaseEq(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opCaseNeq(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opWildEq(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opWildNeq(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opBufIf1(ثابت V3_رقم& ens, ثابت V3_رقم& if1s);
    V3_رقم& opSetRange(uint32_t lsb, uint32_t width, حرف bit_قيمة);
    // "standard" math
    V3_رقم& opNot(ثابت V3_رقم& lhs);
    V3_رقم& opLogNot(ثابت V3_رقم& lhs);
    V3_رقم& opLogAnd(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLogOr(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLogEq(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLogIf(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opNegate(ثابت V3_رقم& lhs);
    V3_رقم& opAdd(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opSub(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opMul(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opMulS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed
    V3_رقم& opDiv(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opDivS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed
    V3_رقم& opModDiv(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opModDivS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed
    V3_رقم& opPow(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs, bool lsign = false,
                    bool rsign = false);
    V3_رقم& opPowSU(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed lhs, unsigned rhs
    V3_رقم& opPowSS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed lhs, signed rhs
    V3_رقم& opPowUS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Unsigned lhs, signed rhs
    V3_رقم& opAnd(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opXor(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opOr(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opShiftR(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opShiftRS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_رقم& opShiftL(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    // Comparisons
    V3_رقم& opEq(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opNeq(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opGt(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opGtS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed
    V3_رقم& opGte(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opGteS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed
    V3_رقم& opLt(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLtS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed
    V3_رقم& opLte(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLteS(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);  // Signed

    // "D" - double (aka real) math
    V3_رقم& opIToRD(ثابت V3_رقم& lhs, bool isSigned = false);
    V3_رقم& opISToRD(ثابت V3_رقم& lhs) { أرجع opIToRD(lhs, true); }
    V3_رقم& opRToIS(ثابت V3_رقم& lhs);
    V3_رقم& opRToIRoundS(ثابت V3_رقم& lhs);
    V3_رقم& opRealToBits(ثابت V3_رقم& lhs);
    V3_رقم& opBitsToRealD(ثابت V3_رقم& lhs);
    V3_رقم& opNegateD(ثابت V3_رقم& lhs);
    V3_رقم& opAddD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opSubD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opMulD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opDivD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opPowD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    // Comparisons
    V3_رقم& opEqD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opNeqD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opGtD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opGteD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLtD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLteD(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);

    // "N" - string operations
    V3_رقم& opAtoN(ثابت V3_رقم& lhs, صحيح base);
    V3_رقم& opNToI(ثابت V3_رقم& lhs);
    V3_رقم& opPutcN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs, ثابت V3_رقم& ths);
    V3_رقم& opGetcN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opSubstrN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs, ثابت V3_رقم& ths);
    V3_رقم& opCompareNN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs, bool ignoreCase);
    V3_رقم& opConcatN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opReplN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opReplN(ثابت V3_رقم& lhs, uint32_t rhsval);
    V3_رقم& opEqN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opNeqN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opGtN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opGteN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLtN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
    V3_رقم& opLteN(ثابت V3_رقم& lhs, ثابت V3_رقم& rhs);
};
مضمن std::ostream& operator<<(std::ostream& os, ثابت V3_رقم& rhs) VL_MT_SAFE {
    أرجع os << rhs.ascii();
}

#endif  // Guard
