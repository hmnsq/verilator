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

#ifndef VERILATOR_V3_ژمار_H
#define VERILATOR_V3_ژمار_H

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
class Ast_گرێ;
class Ast_گرێ_DType;
class AstSFormatArg;
class File_هێڵ;

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

class V3_ژمار_Data final {
عام:
    // TYPES
    بنية بەها_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_بەها;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_بەها_X;
        bool operator==(ثابت بەها_And_X& other) ثابت VL_MT_SAFE {
            أرجع m_بەها == other.m_بەها && m_بەها_X == other.m_بەها_X;
        }
    };

    تعداد class V3_ژمار_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ثابت V3_ژمار_Data_Type& rhs) VL_MT_SAFE {
        بدل (rhs) {
        حالة V3_ژمار_Data_Type::UNINITIALIZED: أرجع os << "UNINITIALIZED";
        حالة V3_ژمار_Data_Type::LOGIC: أرجع os << "LOGIC";
        حالة V3_ژمار_Data_Type::DOUBLE: أرجع os << "DOUBLE";
        حالة V3_ژمار_Data_Type::STRING: أرجع os << "STRING";
        }
        أرجع os;
    }

خاص:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ساكن constexpr صحيح INLINE_وشەکان = std::max(
        static_cast<size_t>(2), std::max(حجم(std::string) / حجم(بەها_And_X),
                                         حجم(std::vector<بەها_And_X>) / حجم(بەها_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ساكن constexpr صحيح MAX_INLINE_WIDTH = INLINE_وشەکان * حجم(بەها_And_X) / 2 * 8;

    // MEMBERS
    اتحاد {
        std::array<بەها_And_X, INLINE_وشەکان> m_inline_ژمار;
        std::vector<بەها_And_X> m_dynamic_ژمار;
        std::string m_string;
    };

    صحيح m_width = 0;  // Width (in bits) as specified/calculated
    V3_ژمار_Data_Type m_type;

عام:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_ژمار_Data()
        : m_type{V3_ژمار_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_ژمار_Data() { destroy_Stored_بەها(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_ژمار_Data(ثابت V3_ژمار_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_ژمار()) {
            init_Inline_ژمار(other.m_inline_ژمار);
        } وإلا إذا (other.is_Dynamic_ژمار()) {
            init_Dynamic_ژمار(other.m_dynamic_ژمار);
        } وإلا إذا (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_ژمار_Data& operator=(ثابت V3_ژمار_Data& other) {
        إذا (other.is_Inline_ژمار()) {
            destroy_Stored_بەها();
            init_Inline_ژمار(other.m_inline_ژمار);
        } وإلا إذا (other.is_Dynamic_ژمار()) {
            reinit_With_Or_Assign_Dynamic_ژمار(other.m_dynamic_ژمار);
        } وإلا إذا (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } وإلا {
            destroy_Stored_بەها();
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

    V3_ژمار_Data(V3_ژمار_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        إذا (other.is_Inline_ژمار()) {
            init_Inline_ژمار(other.m_inline_ژمار);
        } وإلا إذا (other.is_Dynamic_ژمار()) {
            init_Dynamic_ژمار(std::move(other.m_dynamic_ژمار));
        } وإلا إذا (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_ژمار_Data_Type::UNINITIALIZED;
    }

    V3_ژمار_Data& operator=(V3_ژمار_Data&& other) {
        إذا (other.is_Inline_ژمار()) {
            destroy_Stored_بەها();
            init_Inline_ژمار(other.m_inline_ژمار);
        } وإلا إذا (other.is_Dynamic_ژمار()) {
            reinit_With_Or_Assign_Dynamic_ژمار(std::move(other.m_dynamic_ژمار));
        } وإلا إذا (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } وإلا {
            destroy_Stored_بەها();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_ژمار_Data_Type::UNINITIALIZED;
        أرجع *this;
    }

    // ACCESSORS
    بەها_And_X* ژمارە() {
        UASSERT(is_ژمار(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_ژمار() ? m_inline_ژمار.data() : m_dynamic_ژمار.data();
    }
    ثابت بەها_And_X* ژمارە() ثابت VL_MT_SAFE {
        UASSERT(is_ژمار(), "`num` member accessed when data type is " << m_type);
        أرجع is_Inline_ژمار() ? m_inline_ژمار.data() : m_dynamic_ژمار.data();
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
    V3_ژمار_Data_Type type() ثابت VL_MT_SAFE { أرجع m_type; }

    // METHODS
    فراغ resize(صحيح bits_ژماردن) {
        إذا (m_width == bits_ژماردن) أرجع;
        إذا (bits_To_وشەکان(m_width) == bits_To_وشەکان(bits_ژماردن)) {
            m_width = bits_ژماردن;
            أرجع;
        }
        إذا (is_Dynamic_ژمار()) {
            إذا (bits_ژماردن > MAX_INLINE_WIDTH) {
                m_dynamic_ژمار.resize(bits_To_وشەکان(bits_ژماردن));
            } وإلا {
                ثابت auto dynamicBits = std::move(m_dynamic_ژمار);
                destroy_Dynamic_ژمار();
                init_Inline_ژمار();
                std::memcpy(m_inline_ژمار.data(), dynamicBits.data(), حجم(m_inline_ژمار));
            }
        } وإلا إذا (is_Inline_ژمار()) {
            إذا (bits_ژماردن > MAX_INLINE_WIDTH) {
                ثابت auto bits = m_inline_ژمار;
                init_Dynamic_ژمار(bits_To_وشەکان(bits_ژماردن));
                std::memcpy(m_dynamic_ژمار.data(), bits.data(), حجم(bits));
            }
        }
        m_width = bits_ژماردن;
    }

    فراغ setString() {
        // If there has been a string already it is kept intact.
        إذا (isString()) أرجع;
        إذا (is_Dynamic_ژمار()) destroy_Dynamic_ژمار();
        initString();
        m_type = V3_ژمار_Data_Type::STRING;
    }
    فراغ setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_ژمار_Data_Type::STRING;
    }
    فراغ setString(ثابت std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_ژمار_Data_Type::STRING;
    }

    فراغ setDouble() {
        destroy_Stored_بەها();
        إذا (!is_Inline_ژمار()) init_Inline_ژمار();
        m_type = V3_ژمار_Data_Type::DOUBLE;
        resize(64);
    }

    فراغ setLogic() {
        إذا (isString()) destroyString();
        إذا (!is_ژمار()) {
            إذا (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_ژمار();
            } وإلا {
                init_Dynamic_ژمار(bits_To_وشەکان(m_width));
            }
        }
        m_type = V3_ژمار_Data_Type::LOGIC;
        resize(m_width);
    }

خاص:
    ساكن constexpr صحيح bits_To_وشەکان(صحيح bits_ژماردن) VL_PURE { أرجع (bits_ژماردن + 31) / 32; }

    bool is_ژمار() ثابت VL_MT_SAFE {
        أرجع m_type == V3_ژمار_Data_Type::DOUBLE || m_type == V3_ژمار_Data_Type::LOGIC;
    }
    bool is_Inline_ژمار() ثابت VL_MT_SAFE {
        أرجع (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_ژمار_Data_Type::DOUBLE || m_type == V3_ژمار_Data_Type::LOGIC);
    }
    bool is_Dynamic_ژمار() ثابت VL_MT_SAFE {
        أرجع (m_width > MAX_INLINE_WIDTH) && (m_type == V3_ژمار_Data_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_SAFE { أرجع m_type == V3_ژمار_Data_Type::STRING; }

    template <typename... Args>
    فراغ init_Inline_ژمار(Args&&... args) {
        new (&m_inline_ژمار) std::array<بەها_And_X, INLINE_وشەکان>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    فراغ init_Dynamic_ژمار(Args&&... args) {
        new (&m_dynamic_ژمار) std::vector<بەها_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    فراغ initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    فراغ destroy_Dynamic_ژمار() { m_dynamic_ژمار.~vector(); }
    فراغ destroyString() { m_string.~string(); }
    فراغ destroy_Stored_بەها() {
        إذا (isString())
            destroyString();
        وإلا إذا (is_Dynamic_ژمار())
            destroy_Dynamic_ژمار();
    }

    template <typename T>
    فراغ reinit_With_Or_Assign_Dynamic_ژمار(T&& s) {
        إذا (is_Dynamic_ژمار()) {
            m_dynamic_ژمار = std::forward<T>(s);
            أرجع;
        }
        إذا (isString()) destroyString();
        init_Dynamic_ژمار(std::forward<T>(s));
    }
    template <typename T>
    فراغ reinitWithOrAssignString(T&& s) {
        إذا (isString()) {
            m_string = std::forward<T>(s);
            أرجع;
        }
        إذا (is_Dynamic_ژمار()) destroy_Dynamic_ژمار();
        initString(std::forward<T>(s));
    }
};

class V3_ژمار final {
    // TYPES
    using بەها_And_X = V3_ژمار_Data::بەها_And_X;
    using V3_ژمار_Data_Type = V3_ژمار_Data::V3_ژمار_Data_Type;

    // MEMBERS
    V3_ژمار_Data m_data;
    Ast_گرێ* m_nodep = nullptr;  // Parent node - for error reporting only
    File_هێڵ* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    فراغ setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_ژمار& setSingleBits(حرف بەها);
    فراغ fillBits(صحيح destLsb, صحيح width, حرف بەها);
    فراغ copyBits(صحيح destLsb, ثابت V3_ژمار& source, صحيح sourceLsb, صحيح width);
    V3_ژمار& setString(ثابت string& str) {
        m_data.setString(str);
        أرجع *this;
    }
    فراغ opCleanThis(bool warnOnTruncation = false);

عام:
    فراغ nodep(Ast_گرێ* nodep) VL_MT_STABLE;
    Ast_گرێ* nodep() ثابت VL_MT_SAFE { أرجع m_nodep; }  // For debug only
    File_هێڵ* fileline() ثابت VL_MT_SAFE { أرجع m_fileline; }
    V3_ژمار& setZero();
    V3_ژمار& setQuad(uint64_t بەها);
    V3_ژمار& setLong(uint32_t بەها);
    V3_ژمار& setLongS(int32_t بەها);
    V3_ژمار& setDouble(مزدوج بەها);
    فراغ setBitX0(صحيح bit);
    فراغ setBit(صحيح bit, حرف بەها) {  // Note: must be initialized as number and pre-zeroed!
        إذا (bit >= m_data.width()) أرجع;
        ثابت uint32_t mask = (1UL << (bit & 31));
        بەها_And_X& v = m_data.ژمارە()[bit / 32];
        إذا (بەها == '0' || بەها == 0) {
            v.m_بەها &= ~mask;
            v.m_بەها_X &= ~mask;
        } وإلا إذا (بەها == '1' || بەها == 1) {
            v.m_بەها |= mask;
            v.m_بەها_X &= ~mask;
        } وإلا إذا (بەها == 'z' || بەها == 2) {
            v.m_بەها &= ~mask;
            v.m_بەها_X |= mask;
        } وإلا {  // X
            v.m_بەها |= mask;
            v.m_بەها_X |= mask;
        }
    }

خاص:
    حرف bitIs(صحيح bit) ثابت {
        إذا (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            أرجع '0';
        }
        ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
        أرجع ("01zx"[(((v.m_بەها & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_بەها_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    حرف bitIsExtend(صحيح bit, صحيح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        إذا (bit < 0) أرجع '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        إذا (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
            // We do sign extend
            أرجع ("01zx"[(((v.m_بەها & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_بەها_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
        أرجع ("01zx"[(((v.m_بەها & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_بەها_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

عام:
    bool bitIs0(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_ژمار()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع !bitIsXZ(m_data.width() - 1);
        ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
        أرجع ((v.m_بەها | v.m_بەها_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_ژمار()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع false;
        ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
        أرجع ((v.m_بەها & (1UL << (bit & 31))) && !(v.m_بەها_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(صحيح bit) ثابت {
        // Correct number of zero bits/width matters
        إذا (!is_ژمار()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع bitIs1Extend(m_data.width() - 1);
        ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
        أرجع ((v.m_بەها & (1UL << (bit & 31))) && !(v.m_بەها_X & (1UL << (bit & 31))));
    }
    bool bitIsX(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_ژمار()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع bitIsZ(m_data.width() - 1);
        ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
        أرجع ((v.m_بەها & (1UL << (bit & 31))) && (v.m_بەها_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_ژمار()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع bitIsXZ(m_data.width() - 1);
        ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
        أرجع ((v.m_بەها_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(صحيح bit) ثابت VL_MT_SAFE {
        إذا (!is_ژمار()) أرجع false;
        إذا (bit < 0) أرجع false;
        إذا (bit >= m_data.width()) أرجع bitIsZ(m_data.width() - 1);
        ثابت بەها_And_X v = m_data.ژمارە()[bit / 32];
        أرجع ((~v.m_بەها & (1UL << (bit & 31))) && (v.m_بەها_X & (1UL << (bit & 31))));
    }

خاص:
    uint32_t bits_بەها(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE {
        uint32_t v = 0;
        لكل (صحيح bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        أرجع v;
    }

    صحيح ژماردن_X(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE;
    صحيح ژماردن_Z(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE;

    صحيح وشەکان() ثابت VL_MT_SAFE { أرجع ((width() + 31) / 32); }
    uint32_t hiWordMask() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        أرجع VL_MASK_I(width());
    }

    V3_ژمار& opModDivGuts(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs, bool is_modulus);

عام:
    // CONSTRUCTORS
    explicit V3_ژمار(Ast_گرێ* nodep) { init(nodep, 1); }
    V3_ژمار(Ast_گرێ* nodep, صحيح width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_ژمار(Ast_گرێ* nodep, صحيح width, ثابت V3_ژمار& بەها) {
        init(nodep, width, width > 0);
        opAssign(بەها);
    }
    V3_ژمار(Ast_گرێ* nodep, صحيح width, uint32_t بەها, bool sized = true) {
        init(nodep, width, sized);
        m_data.ژمارە()[0].m_بەها = بەها;
        opCleanThis();
    }
    V3_ژمار(File_هێڵ* flp, صحيح width, uint32_t بەها, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.ژمارە()[0].m_بەها = بەها;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_ژمار_Literal {};  // For creator type-overload selection
    V3_ژمار(Ast_گرێ* nodep, Verilog_ژمار_Literal, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3_ژمار(File_هێڵ* flp, Verilog_ژمار_Literal, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_ژمار(Ast_گرێ* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_ژمار(Ast_گرێ* nodep, Double, مزدوج بەها) {
        init(nodep, 64);
        setDouble(بەها);
    }
    class String {};
    V3_ژمار(Ast_گرێ* nodep, String, ثابت string& بەها) {
        init(nodep);
        setString(بەها);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_ژمار(Ast_گرێ* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_ژمار(Ast_گرێ* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_ژمار(ثابت V3_ژمار* nump, صحيح width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_ژمار(ثابت V3_ژمار* nump, صحيح width, uint32_t بەها) {
        init(nullptr, width);
        m_data.ژمارە()[0].m_بەها = بەها;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_ژمار(Ast_گرێ* nodep, ثابت Ast_گرێ_DType* nodedtypep);

    V3_ژمار(ثابت V3_ژمار& other) = افتراضي;
    V3_ژمار& operator=(ثابت V3_ژمار& other) = افتراضي;

    V3_ژمار(V3_ژمار&& other) = افتراضي;
    V3_ژمار& operator=(V3_ژمار&& other) = افتراضي;

    ~V3_ژمار() = افتراضي;

خاص:
    فراغ selfTestThis();
    فراغ create(Ast_گرێ* nodep, ثابت حرف* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    فراغ create(File_هێڵ* flp, ثابت حرف* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    فراغ create(ثابت حرف* sourcep);
    فراغ init(Ast_گرێ* nodep, صحيح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        إذا (swidth >= 0) {
            إذا (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            لكل (صحيح i = 0; i < وشەکان(); ++i) m_data.ژمارە()[i] = {0, 0};
        } وإلا {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    ساكن string displayPad(size_t fmtsize, حرف pad, bool چەپ, ثابت string& in) VL_PURE;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        أرجع displayed(m_fileline, vformat);
    }
    فراغ warnTooMany(ثابت string& بەها);

عام:
    فراغ v3errorEnd(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3errorEndFatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ width(صحيح width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_ژمار& setAllBitsXRemoved();
    V3_ژمار& setAllBitsX();
    V3_ژمار& setAllBitsZ();
    V3_ژمار& setAllBits0();
    V3_ژمار& setAllBits1();
    V3_ژمار& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_ژمار& setMask(صحيح nbits, صحيح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت Ast_گرێ* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(File_هێڵ* fl, ثابت string& vformat,
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
    V3_ژمار_Data_Type dataType() ثابت VL_MT_SAFE { أرجع m_data.type(); }
    فراغ dataType(V3_ژمار_Data_Type newType) {
        إذا (dataType() == newType) أرجع;
        UASSERT(newType != V3_ژمار_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        بدل (newType) {
        حالة V3_ژمار_Data_Type::STRING: m_data.setString(); اكسر;
        حالة V3_ژمار_Data_Type::DOUBLE: m_data.setDouble(); اكسر;
        حالة V3_ژمار_Data_Type::LOGIC: m_data.setLogic(); اكسر;
        حالة V3_ژمار_Data_Type::UNINITIALIZED: اكسر;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_SAFE { أرجع m_data.m_signed; }
    فراغ isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_SAFE { أرجع dataType() == V3_ژمار_Data_Type::DOUBLE; }
    bool isString() ثابت VL_MT_SAFE { أرجع dataType() == V3_ژمار_Data_Type::STRING; }
    bool isOpaque() ثابت VL_MT_SAFE { أرجع isDouble() || isString(); }
    bool is_ژمار() ثابت VL_MT_SAFE {
        أرجع m_data.type() == V3_ژمار_Data_Type::LOGIC
               || m_data.type() == V3_ژمار_Data_Type::DOUBLE;
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
        لكل (صحيح i = 0; i < وشەکان(); ++i) {
            ثابت بەها_And_X v = m_data.ژمارە()[i];
            إذا ((~v.m_بەها) & v.m_بەها_X) أرجع true;
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
    bool isCaseEq(ثابت V3_ژمار& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_ژمار& rhs) ثابت;  // operator< with XZ compared
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
    uint32_t ژماردن_Bits(ثابت V3_ژمار& ctrl) ثابت;
    uint32_t ژماردن_Bits(ثابت V3_ژمار& ctrl1, ثابت V3_ژمار& ctrl2, ثابت V3_ژمار& ctrl3) ثابت;
    uint32_t ژماردن_Ones() ثابت;
    uint32_t mostSetBitP1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ثابت V3_ژمار& rhs) ثابت { أرجع isLtXZ(rhs); }

    // STATICS
    ساكن bool epsilonEqual(مزدوج a, مزدوج b);  // True if number within Epsilon of the other
    ساكن bool epsilonIntegral(مزدوج a);  // True if number rounds to integer within Epsilon
    ساكن صحيح log2b(uint32_t ژمارە);
    ساكن صحيح log2bQuad(uint64_t ژمارە);
    ساكن فراغ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_ژمار& opBitsNonXZ(ثابت V3_ژمار& lhs);  // 0/1->1, X/Z->0
    V3_ژمار& opBitsOne(ثابت V3_ژمار& lhs);  // 1->1, 0/X/Z->0
    V3_ژمار& opBitsXZ(ثابت V3_ژمار& lhs);  // 0/1->0, X/Z->1
    V3_ژمار& opBitsZ(ثابت V3_ژمار& lhs);  // Z->1, 0/1/X->0
    //
    V3_ژمار& opAssign(ثابت V3_ژمار& lhs);
    V3_ژمار& opAssignNonXZ(ثابت V3_ژمار& lhs, bool ignoreXZ = true);
    V3_ژمار& opExtendS(ثابت V3_ژمار& lhs, uint32_t lbits);  // Sign extension
    V3_ژمار& opExtendXZ(ثابت V3_ژمار& lhs, uint32_t lbits);  // X/Z extension
    V3_ژمار& opRedOr(ثابت V3_ژمار& lhs);
    V3_ژمار& opRedAnd(ثابت V3_ژمار& lhs);
    V3_ژمار& opRedXor(ثابت V3_ژمار& lhs);
    V3_ژمار& op_ژماردن_Bits(ثابت V3_ژمار& دەربڕین, ثابت V3_ژمار& ctrl1, ثابت V3_ژمار& ctrl2,
                          ثابت V3_ژمار& ctrl3);
    V3_ژمار& op_ژماردن_Ones(ثابت V3_ژمار& lhs);
    V3_ژمار& opIsUnknown(ثابت V3_ژمار& lhs);
    V3_ژمار& opOneHot(ثابت V3_ژمار& lhs);
    V3_ژمار& opOneHot0(ثابت V3_ژمار& lhs);
    V3_ژمار& opCLog2(ثابت V3_ژمار& lhs);
    V3_ژمار& opMostSetBitP1(ثابت V3_ژمار& lhs);
    V3_ژمار& opClean(ثابت V3_ژمار& lhs, uint32_t bits);
    V3_ژمار& opConcat(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLenN(ثابت V3_ژمار& lhs);
    V3_ژمار& opRepl(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opRepl(ثابت V3_ژمار& lhs, uint32_t rhsval);
    V3_ژمار& opStreamL(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opSel(ثابت V3_ژمار& lhs, ثابت V3_ژمار& msb, ثابت V3_ژمار& lsb);
    V3_ژمار& opSel(ثابت V3_ژمار& lhs, uint32_t msbval, uint32_t lsbval);
    V3_ژمار& opSelInto(ثابت V3_ژمار& lhs, ثابت V3_ژمار& lsb, صحيح width);
    V3_ژمار& opSelInto(ثابت V3_ژمار& lhs, صحيح lsbval, صحيح width);
    V3_ژمار& opCvtPackString(ثابت V3_ژمار& lhs);
    V3_ژمار& opToLowerN(ثابت V3_ژمار& lhs);
    V3_ژمار& opToUpperN(ثابت V3_ژمار& lhs);
    V3_ژمار& opCaseEq(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opCaseNeq(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opWildEq(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opWildNeq(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opBufIf1(ثابت V3_ژمار& ens, ثابت V3_ژمار& if1s);
    V3_ژمار& opSetRange(uint32_t lsb, uint32_t width, حرف bit_بەها);
    // "standard" math
    V3_ژمار& opNot(ثابت V3_ژمار& lhs);
    V3_ژمار& opLogNot(ثابت V3_ژمار& lhs);
    V3_ژمار& opLogAnd(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLogOr(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLogEq(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLogIf(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opNegate(ثابت V3_ژمار& lhs);
    V3_ژمار& opAdd(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opSub(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opMul(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opMulS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed
    V3_ژمار& opDiv(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opDivS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed
    V3_ژمار& opModDiv(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opModDivS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed
    V3_ژمار& opPow(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs, bool lsign = false,
                    bool rsign = false);
    V3_ژمار& opPowSU(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed lhs, unsigned rhs
    V3_ژمار& opPowSS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed lhs, signed rhs
    V3_ژمار& opPowUS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Unsigned lhs, signed rhs
    V3_ژمار& opAnd(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opXor(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opOr(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opShiftR(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opShiftRS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_ژمار& opShiftL(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    // Comparisons
    V3_ژمار& opEq(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opNeq(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opGt(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opGtS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed
    V3_ژمار& opGte(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opGteS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed
    V3_ژمار& opLt(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLtS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed
    V3_ژمار& opLte(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLteS(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);  // Signed

    // "D" - double (aka real) math
    V3_ژمار& opIToRD(ثابت V3_ژمار& lhs, bool isSigned = false);
    V3_ژمار& opISToRD(ثابت V3_ژمار& lhs) { أرجع opIToRD(lhs, true); }
    V3_ژمار& opRToIS(ثابت V3_ژمار& lhs);
    V3_ژمار& opRToIRoundS(ثابت V3_ژمار& lhs);
    V3_ژمار& opRealToBits(ثابت V3_ژمار& lhs);
    V3_ژمار& opBitsToRealD(ثابت V3_ژمار& lhs);
    V3_ژمار& opNegateD(ثابت V3_ژمار& lhs);
    V3_ژمار& opAddD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opSubD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opMulD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opDivD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opPowD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    // Comparisons
    V3_ژمار& opEqD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opNeqD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opGtD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opGteD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLtD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLteD(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);

    // "N" - string operations
    V3_ژمار& opAtoN(ثابت V3_ژمار& lhs, صحيح base);
    V3_ژمار& opNToI(ثابت V3_ژمار& lhs);
    V3_ژمار& opPutcN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs, ثابت V3_ژمار& ths);
    V3_ژمار& opGetcN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opSubstrN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs, ثابت V3_ژمار& ths);
    V3_ژمار& opCompareNN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs, bool ignoreCase);
    V3_ژمار& opConcatN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opReplN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opReplN(ثابت V3_ژمار& lhs, uint32_t rhsval);
    V3_ژمار& opEqN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opNeqN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opGtN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opGteN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLtN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
    V3_ژمار& opLteN(ثابت V3_ژمار& lhs, ثابت V3_ژمار& rhs);
};
مضمن std::ostream& operator<<(std::ostream& os, ثابت V3_ژمار& rhs) VL_MT_SAFE {
    أرجع os << rhs.ascii();
}

#endif  // Guard
