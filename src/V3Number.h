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

#ifndef VERILATOR_V3NUMBER_H_
#define VERILATOR_V3NUMBER_H_

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
class AstNode;
class AstNodeDType;
class AstSFormatArg;
class FileLine;

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

class V3NumberData final {
عام:
    // TYPES
    بنية ValueAndX final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_value;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_valueX;
        bool operator==(ثابت ValueAndX& other) ثابت VL_MT_SAFE {
            أرجع m_value == other.m_value && m_valueX == other.m_valueX;
        }
    };

    تعداد class V3NumberDataType : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ثابت V3NumberDataType& rhs) VL_MT_SAFE {
        بدل (rhs) {
        حالة V3NumberDataType::UNINITIALIZED: أرجع os << "UNINITIALIZED";
        حالة V3NumberDataType::LOGIC: أرجع os << "LOGIC";
        حالة V3NumberDataType::DOUBLE: أرجع os << "DOUBLE";
        حالة V3NumberDataType::STRING: أرجع os << "STRING";
        }
        أرجع os;
    }

خاص:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ساكن constexpr صحيح INLINE_WORDS = std::max(
        static_cast<size_t>(2), std::max(حجم(std::string) / حجم(ValueAndX),
                                         حجم(std::vector<ValueAndX>) / حجم(ValueAndX)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ساكن constexpr صحيح MAX_INLINE_WIDTH = INLINE_WORDS * حجم(ValueAndX) / 2 * 8;

    // MEMBERS
    اتحاد {
        std::array<ValueAndX, INLINE_WORDS> m_inlineNumber;
        std::vector<ValueAndX> m_dynamicNumber;
        std::string m_string;
    };

    صحيح m_width = 0;  // Width (in bits) as specified/calculated
    V3NumberDataType m_type;

عام:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3NumberData()
        : m_type{V3NumberDataType::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3NumberData() { destroyStoredValue(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3NumberData(ثابت V3NumberData& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        𐡄𐡍 (other.isInlineNumber()) {
            initInlineNumber(other.m_inlineNumber);
        } وإلا 𐡄𐡍 (other.isDynamicNumber()) {
            initDynamicNumber(other.m_dynamicNumber);
        } وإلا 𐡄𐡍 (other.isString()) {
            initString(other.m_string);
        }
    }

    V3NumberData& operator=(ثابت V3NumberData& other) {
        𐡄𐡍 (other.isInlineNumber()) {
            destroyStoredValue();
            initInlineNumber(other.m_inlineNumber);
        } وإلا 𐡄𐡍 (other.isDynamicNumber()) {
            reinitWithOrAssignDynamicNumber(other.m_dynamicNumber);
        } وإلا 𐡄𐡍 (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } وإلا {
            destroyStoredValue();
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

    V3NumberData(V3NumberData&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        𐡄𐡍 (other.isInlineNumber()) {
            initInlineNumber(other.m_inlineNumber);
        } وإلا 𐡄𐡍 (other.isDynamicNumber()) {
            initDynamicNumber(std::move(other.m_dynamicNumber));
        } وإلا 𐡄𐡍 (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3NumberDataType::UNINITIALIZED;
    }

    V3NumberData& operator=(V3NumberData&& other) {
        𐡄𐡍 (other.isInlineNumber()) {
            destroyStoredValue();
            initInlineNumber(other.m_inlineNumber);
        } وإلا 𐡄𐡍 (other.isDynamicNumber()) {
            reinitWithOrAssignDynamicNumber(std::move(other.m_dynamicNumber));
        } وإلا 𐡄𐡍 (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } وإلا {
            destroyStoredValue();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3NumberDataType::UNINITIALIZED;
        أرجع *this;
    }

    // ACCESSORS
    ValueAndX* num() {
        UASSERT(isNumber(), "`num` member accessed when data type is " << m_type);
        أرجع isInlineNumber() ? m_inlineNumber.data() : m_dynamicNumber.data();
    }
    ثابت ValueAndX* num() ثابت VL_MT_SAFE {
        UASSERT(isNumber(), "`num` member accessed when data type is " << m_type);
        أرجع isInlineNumber() ? m_inlineNumber.data() : m_dynamicNumber.data();
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
    V3NumberDataType type() ثابت VL_MT_SAFE { أرجع m_type; }

    // METHODS
    فراغ resize(صحيح bitsCount) {
        𐡄𐡍 (m_width == bitsCount) أرجع;
        𐡄𐡍 (bitsToWords(m_width) == bitsToWords(bitsCount)) {
            m_width = bitsCount;
            أرجع;
        }
        𐡄𐡍 (isDynamicNumber()) {
            𐡄𐡍 (bitsCount > MAX_INLINE_WIDTH) {
                m_dynamicNumber.resize(bitsToWords(bitsCount));
            } وإلا {
                ثابت auto dynamicBits = std::move(m_dynamicNumber);
                destroyDynamicNumber();
                initInlineNumber();
                std::memcpy(m_inlineNumber.data(), dynamicBits.data(), حجم(m_inlineNumber));
            }
        } وإلا 𐡄𐡍 (isInlineNumber()) {
            𐡄𐡍 (bitsCount > MAX_INLINE_WIDTH) {
                ثابت auto bits = m_inlineNumber;
                initDynamicNumber(bitsToWords(bitsCount));
                std::memcpy(m_dynamicNumber.data(), bits.data(), حجم(bits));
            }
        }
        m_width = bitsCount;
    }

    فراغ setString() {
        // If there has been a string already it is kept intact.
        𐡄𐡍 (isString()) أرجع;
        𐡄𐡍 (isDynamicNumber()) destroyDynamicNumber();
        initString();
        m_type = V3NumberDataType::STRING;
    }
    فراغ setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3NumberDataType::STRING;
    }
    فراغ setString(ثابت std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3NumberDataType::STRING;
    }

    فراغ setDouble() {
        destroyStoredValue();
        𐡄𐡍 (!isInlineNumber()) initInlineNumber();
        m_type = V3NumberDataType::DOUBLE;
        resize(64);
    }

    فراغ setLogic() {
        𐡄𐡍 (isString()) destroyString();
        𐡄𐡍 (!isNumber()) {
            𐡄𐡍 (m_width <= MAX_INLINE_WIDTH) {
                initInlineNumber();
            } وإلا {
                initDynamicNumber(bitsToWords(m_width));
            }
        }
        m_type = V3NumberDataType::LOGIC;
        resize(m_width);
    }

خاص:
    ساكن constexpr صحيح bitsToWords(صحيح bitsCount) VL_PURE { أرجع (bitsCount + 31) / 32; }

    bool isNumber() ثابت VL_MT_SAFE {
        أرجع m_type == V3NumberDataType::DOUBLE || m_type == V3NumberDataType::LOGIC;
    }
    bool isInlineNumber() ثابت VL_MT_SAFE {
        أرجع (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3NumberDataType::DOUBLE || m_type == V3NumberDataType::LOGIC);
    }
    bool isDynamicNumber() ثابت VL_MT_SAFE {
        أرجع (m_width > MAX_INLINE_WIDTH) && (m_type == V3NumberDataType::LOGIC);
    }
    bool isString() ثابت VL_MT_SAFE { أرجع m_type == V3NumberDataType::STRING; }

    template <typename... Args>
    فراغ initInlineNumber(Args&&... args) {
        new (&m_inlineNumber) std::array<ValueAndX, INLINE_WORDS>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    فراغ initDynamicNumber(Args&&... args) {
        new (&m_dynamicNumber) std::vector<ValueAndX>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    فراغ initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    فراغ destroyDynamicNumber() { m_dynamicNumber.~vector(); }
    فراغ destroyString() { m_string.~string(); }
    فراغ destroyStoredValue() {
        𐡄𐡍 (isString())
            destroyString();
        وإلا 𐡄𐡍 (isDynamicNumber())
            destroyDynamicNumber();
    }

    template <typename T>
    فراغ reinitWithOrAssignDynamicNumber(T&& s) {
        𐡄𐡍 (isDynamicNumber()) {
            m_dynamicNumber = std::forward<T>(s);
            أرجع;
        }
        𐡄𐡍 (isString()) destroyString();
        initDynamicNumber(std::forward<T>(s));
    }
    template <typename T>
    فراغ reinitWithOrAssignString(T&& s) {
        𐡄𐡍 (isString()) {
            m_string = std::forward<T>(s);
            أرجع;
        }
        𐡄𐡍 (isDynamicNumber()) destroyDynamicNumber();
        initString(std::forward<T>(s));
    }
};

class V3Number final {
    // TYPES
    using ValueAndX = V3NumberData::ValueAndX;
    using V3NumberDataType = V3NumberData::V3NumberDataType;

    // MEMBERS
    V3NumberData m_data;
    AstNode* m_nodep = nullptr;  // Parent node - for error reporting only
    FileLine* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    فراغ setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3Number& setSingleBits(حرف value);
    فراغ fillBits(صحيح destLsb, صحيح width, حرف value);
    فراغ copyBits(صحيح destLsb, ثابت V3Number& source, صحيح sourceLsb, صحيح width);
    V3Number& setString(ثابت string& str) {
        m_data.setString(str);
        أرجع *this;
    }
    فراغ opCleanThis(bool warnOnTruncation = false);

عام:
    فراغ nodep(AstNode* nodep) VL_MT_STABLE;
    AstNode* nodep() ثابت VL_MT_SAFE { أرجع m_nodep; }  // For debug only
    FileLine* fileline() ثابت VL_MT_SAFE { أرجع m_fileline; }
    V3Number& setZero();
    V3Number& setQuad(uint64_t value);
    V3Number& setLong(uint32_t value);
    V3Number& setLongS(int32_t value);
    V3Number& setDouble(مزدوج value);
    فراغ setBitX0(صحيح bit);
    فراغ setBit(صحيح bit, حرف value) {  // Note: must be initialized as number and pre-zeroed!
        𐡄𐡍 (bit >= m_data.width()) أرجع;
        ثابت uint32_t mask = (1UL << (bit & 31));
        ValueAndX& v = m_data.num()[bit / 32];
        𐡄𐡍 (value == '0' || value == 0) {
            v.m_value &= ~mask;
            v.m_valueX &= ~mask;
        } وإلا 𐡄𐡍 (value == '1' || value == 1) {
            v.m_value |= mask;
            v.m_valueX &= ~mask;
        } وإلا 𐡄𐡍 (value == 'z' || value == 2) {
            v.m_value &= ~mask;
            v.m_valueX |= mask;
        } وإلا {  // X
            v.m_value |= mask;
            v.m_valueX |= mask;
        }
    }

خاص:
    حرف bitIs(صحيح bit) ثابت {
        𐡄𐡍 (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            أرجع '0';
        }
        ثابت ValueAndX v = m_data.num()[bit / 32];
        أرجع ("01zx"[(((v.m_value & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_valueX & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    حرف bitIsExtend(صحيح bit, صحيح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        𐡄𐡍 (bit < 0) أرجع '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        𐡄𐡍 (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ثابت ValueAndX v = m_data.num()[bit / 32];
            // We do sign extend
            أرجع ("01zx"[(((v.m_value & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_valueX & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ثابت ValueAndX v = m_data.num()[bit / 32];
        أرجع ("01zx"[(((v.m_value & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_valueX & (1UL << (bit & 31))) ? 2 : 0))]);
    }

عام:
    bool bitIs0(صحيح bit) ثابت VL_MT_SAFE {
        𐡄𐡍 (!isNumber()) أرجع false;
        𐡄𐡍 (bit < 0) أرجع false;
        𐡄𐡍 (bit >= m_data.width()) أرجع !bitIsXZ(m_data.width() - 1);
        ثابت ValueAndX v = m_data.num()[bit / 32];
        أرجع ((v.m_value | v.m_valueX) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(صحيح bit) ثابت VL_MT_SAFE {
        𐡄𐡍 (!isNumber()) أرجع false;
        𐡄𐡍 (bit < 0) أرجع false;
        𐡄𐡍 (bit >= m_data.width()) أرجع false;
        ثابت ValueAndX v = m_data.num()[bit / 32];
        أرجع ((v.m_value & (1UL << (bit & 31))) && !(v.m_valueX & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(صحيح bit) ثابت {
        // Correct number of zero bits/width matters
        𐡄𐡍 (!isNumber()) أرجع false;
        𐡄𐡍 (bit < 0) أرجع false;
        𐡄𐡍 (bit >= m_data.width()) أرجع bitIs1Extend(m_data.width() - 1);
        ثابت ValueAndX v = m_data.num()[bit / 32];
        أرجع ((v.m_value & (1UL << (bit & 31))) && !(v.m_valueX & (1UL << (bit & 31))));
    }
    bool bitIsX(صحيح bit) ثابت VL_MT_SAFE {
        𐡄𐡍 (!isNumber()) أرجع false;
        𐡄𐡍 (bit < 0) أرجع false;
        𐡄𐡍 (bit >= m_data.width()) أرجع bitIsZ(m_data.width() - 1);
        ثابت ValueAndX v = m_data.num()[bit / 32];
        أرجع ((v.m_value & (1UL << (bit & 31))) && (v.m_valueX & (1UL << (bit & 31))));
    }
    bool bitIsXZ(صحيح bit) ثابت VL_MT_SAFE {
        𐡄𐡍 (!isNumber()) أرجع false;
        𐡄𐡍 (bit < 0) أرجع false;
        𐡄𐡍 (bit >= m_data.width()) أرجع bitIsXZ(m_data.width() - 1);
        ثابت ValueAndX v = m_data.num()[bit / 32];
        أرجع ((v.m_valueX & (1UL << (bit & 31))));
    }
    bool bitIsZ(صحيح bit) ثابت VL_MT_SAFE {
        𐡄𐡍 (!isNumber()) أرجع false;
        𐡄𐡍 (bit < 0) أرجع false;
        𐡄𐡍 (bit >= m_data.width()) أرجع bitIsZ(m_data.width() - 1);
        ثابت ValueAndX v = m_data.num()[bit / 32];
        أرجع ((~v.m_value & (1UL << (bit & 31))) && (v.m_valueX & (1UL << (bit & 31))));
    }

خاص:
    uint32_t bitsValue(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE {
        uint32_t v = 0;
        لكل (صحيح bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        أرجع v;
    }

    صحيح countX(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE;
    صحيح countZ(صحيح lsb, صحيح nbits) ثابت VL_MT_SAFE;

    صحيح words() ثابت VL_MT_SAFE { أرجع ((width() + 31) / 32); }
    uint32_t hiWordMask() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        أرجع VL_MASK_I(width());
    }

    V3Number& opModDivGuts(ثابت V3Number& lhs, ثابت V3Number& rhs, bool is_modulus);

عام:
    // CONSTRUCTORS
    explicit V3Number(AstNode* nodep) { init(nodep, 1); }
    V3Number(AstNode* nodep, صحيح width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3Number(AstNode* nodep, صحيح width, ثابت V3Number& value) {
        init(nodep, width, width > 0);
        opAssign(value);
    }
    V3Number(AstNode* nodep, صحيح width, uint32_t value, bool sized = true) {
        init(nodep, width, sized);
        m_data.num()[0].m_value = value;
        opCleanThis();
    }
    V3Number(FileLine* flp, صحيح width, uint32_t value, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.num()[0].m_value = value;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class VerilogNumberLiteral {};  // For creator type-overload selection
    V3Number(AstNode* nodep, VerilogNumberLiteral, ثابت حرف* sourcep) { create(nodep, sourcep); }
    V3Number(FileLine* flp, VerilogNumberLiteral, ثابت حرف* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3Number(AstNode* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3Number(AstNode* nodep, Double, مزدوج value) {
        init(nodep, 64);
        setDouble(value);
    }
    class String {};
    V3Number(AstNode* nodep, String, ثابت string& value) {
        init(nodep);
        setString(value);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3Number(AstNode* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3Number(AstNode* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3Number(ثابت V3Number* nump, صحيح width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3Number(ثابت V3Number* nump, صحيح width, uint32_t value) {
        init(nullptr, width);
        m_data.num()[0].m_value = value;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3Number(AstNode* nodep, ثابت AstNodeDType* nodedtypep);

    V3Number(ثابت V3Number& other) = افتراضي;
    V3Number& operator=(ثابت V3Number& other) = افتراضي;

    V3Number(V3Number&& other) = افتراضي;
    V3Number& operator=(V3Number&& other) = افتراضي;

    ~V3Number() = افتراضي;

خاص:
    فراغ selfTestThis();
    فراغ create(AstNode* nodep, ثابت حرف* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    فراغ create(FileLine* flp, ثابت حرف* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    فراغ create(ثابت حرف* sourcep);
    فراغ init(AstNode* nodep, صحيح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        𐡄𐡍 (swidth >= 0) {
            𐡄𐡍 (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            لكل (صحيح i = 0; i < words(); ++i) m_data.num()[i] = {0, 0};
        } وإلا {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    ساكن string displayPad(size_t fmtsize, حرف pad, bool left, ثابت string& in) VL_PURE;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        أرجع displayed(m_fileline, vformat);
    }
    فراغ warnTooMany(ثابت string& value);

عام:
    فراغ v3errorEnd(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3errorEndFatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ width(صحيح width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3Number& setAllBitsXRemoved();
    V3Number& setAllBitsX();
    V3Number& setAllBitsZ();
    V3Number& setAllBits0();
    V3Number& setAllBits1();
    V3Number& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3Number& setMask(صحيح nbits, صحيح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت AstNode* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(FileLine* fl, ثابت string& vformat,
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
    V3NumberDataType dataType() ثابت VL_MT_SAFE { أرجع m_data.type(); }
    فراغ dataType(V3NumberDataType newType) {
        𐡄𐡍 (dataType() == newType) أرجع;
        UASSERT(newType != V3NumberDataType::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        بدل (newType) {
        حالة V3NumberDataType::STRING: m_data.setString(); اكسر;
        حالة V3NumberDataType::DOUBLE: m_data.setDouble(); اكسر;
        حالة V3NumberDataType::LOGIC: m_data.setLogic(); اكسر;
        حالة V3NumberDataType::UNINITIALIZED: اكسر;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_SAFE { أرجع m_data.m_signed; }
    فراغ isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_SAFE { أرجع dataType() == V3NumberDataType::DOUBLE; }
    bool isString() ثابت VL_MT_SAFE { أرجع dataType() == V3NumberDataType::STRING; }
    bool isOpaque() ثابت VL_MT_SAFE { أرجع isDouble() || isString(); }
    bool isNumber() ثابت VL_MT_SAFE {
        أرجع m_data.type() == V3NumberDataType::LOGIC
               || m_data.type() == V3NumberDataType::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        أرجع !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ثابت VL_MT_SAFE { أرجع m_data.m_is1Step; }
    bool isNull() ثابت VL_MT_SAFE { أرجع m_data.m_isNull; }
    bool isFourState() ثابت VL_MT_SAFE;
    bool hasZ() ثابت {
        𐡄𐡍 (isString()) أرجع false;
        لكل (صحيح i = 0; i < words(); ++i) {
            ثابت ValueAndX v = m_data.num()[i];
            𐡄𐡍 ((~v.m_value) & v.m_valueX) أرجع true;
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
    bool isCaseEq(ثابت V3Number& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3Number& rhs) ثابت;  // operator< with XZ compared
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
    uint32_t countBits(ثابت V3Number& ctrl) ثابت;
    uint32_t countBits(ثابت V3Number& ctrl1, ثابت V3Number& ctrl2, ثابت V3Number& ctrl3) ثابت;
    uint32_t countOnes() ثابت;
    uint32_t mostSetBitP1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ثابت V3Number& rhs) ثابت { أرجع isLtXZ(rhs); }

    // STATICS
    ساكن bool epsilonEqual(مزدوج a, مزدوج b);  // True if number within Epsilon of the other
    ساكن bool epsilonIntegral(مزدوج a);  // True if number rounds to integer within Epsilon
    ساكن صحيح log2b(uint32_t num);
    ساكن صحيح log2bQuad(uint64_t num);
    ساكن فراغ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3Number& opBitsNonXZ(ثابت V3Number& lhs);  // 0/1->1, X/Z->0
    V3Number& opBitsOne(ثابت V3Number& lhs);  // 1->1, 0/X/Z->0
    V3Number& opBitsXZ(ثابت V3Number& lhs);  // 0/1->0, X/Z->1
    V3Number& opBitsZ(ثابت V3Number& lhs);  // Z->1, 0/1/X->0
    //
    V3Number& opAssign(ثابت V3Number& lhs);
    V3Number& opAssignNonXZ(ثابت V3Number& lhs, bool ignoreXZ = true);
    V3Number& opExtendS(ثابت V3Number& lhs, uint32_t lbits);  // Sign extension
    V3Number& opExtendXZ(ثابت V3Number& lhs, uint32_t lbits);  // X/Z extension
    V3Number& opRedOr(ثابت V3Number& lhs);
    V3Number& opRedAnd(ثابت V3Number& lhs);
    V3Number& opRedXor(ثابت V3Number& lhs);
    V3Number& opCountBits(ثابت V3Number& expr, ثابت V3Number& ctrl1, ثابت V3Number& ctrl2,
                          ثابت V3Number& ctrl3);
    V3Number& opCountOnes(ثابت V3Number& lhs);
    V3Number& opIsUnknown(ثابت V3Number& lhs);
    V3Number& opOneHot(ثابت V3Number& lhs);
    V3Number& opOneHot0(ثابت V3Number& lhs);
    V3Number& opCLog2(ثابت V3Number& lhs);
    V3Number& opMostSetBitP1(ثابت V3Number& lhs);
    V3Number& opClean(ثابت V3Number& lhs, uint32_t bits);
    V3Number& opConcat(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLenN(ثابت V3Number& lhs);
    V3Number& opRepl(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opRepl(ثابت V3Number& lhs, uint32_t rhsval);
    V3Number& opStreamL(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opSel(ثابت V3Number& lhs, ثابت V3Number& msb, ثابت V3Number& lsb);
    V3Number& opSel(ثابت V3Number& lhs, uint32_t msbval, uint32_t lsbval);
    V3Number& opSelInto(ثابت V3Number& lhs, ثابت V3Number& lsb, صحيح width);
    V3Number& opSelInto(ثابت V3Number& lhs, صحيح lsbval, صحيح width);
    V3Number& opCvtPackString(ثابت V3Number& lhs);
    V3Number& opToLowerN(ثابت V3Number& lhs);
    V3Number& opToUpperN(ثابت V3Number& lhs);
    V3Number& opCaseEq(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opCaseNeq(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opWildEq(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opWildNeq(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opBufIf1(ثابت V3Number& ens, ثابت V3Number& if1s);
    V3Number& opSetRange(uint32_t lsb, uint32_t width, حرف bitValue);
    // "standard" math
    V3Number& opNot(ثابت V3Number& lhs);
    V3Number& opLogNot(ثابت V3Number& lhs);
    V3Number& opLogAnd(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLogOr(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLogEq(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLogIf(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opNegate(ثابت V3Number& lhs);
    V3Number& opAdd(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opSub(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opMul(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opMulS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed
    V3Number& opDiv(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opDivS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed
    V3Number& opModDiv(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opModDivS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed
    V3Number& opPow(ثابت V3Number& lhs, ثابت V3Number& rhs, bool lsign = false,
                    bool rsign = false);
    V3Number& opPowSU(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed lhs, unsigned rhs
    V3Number& opPowSS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed lhs, signed rhs
    V3Number& opPowUS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Unsigned lhs, signed rhs
    V3Number& opAnd(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opXor(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opOr(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opShiftR(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opShiftRS(ثابت V3Number& lhs, ثابت V3Number& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3Number& opShiftL(ثابت V3Number& lhs, ثابت V3Number& rhs);
    // Comparisons
    V3Number& opEq(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opNeq(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opGt(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opGtS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed
    V3Number& opGte(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opGteS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed
    V3Number& opLt(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLtS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed
    V3Number& opLte(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLteS(ثابت V3Number& lhs, ثابت V3Number& rhs);  // Signed

    // "D" - double (aka real) math
    V3Number& opIToRD(ثابت V3Number& lhs, bool isSigned = false);
    V3Number& opISToRD(ثابت V3Number& lhs) { أرجع opIToRD(lhs, true); }
    V3Number& opRToIS(ثابت V3Number& lhs);
    V3Number& opRToIRoundS(ثابت V3Number& lhs);
    V3Number& opRealToBits(ثابت V3Number& lhs);
    V3Number& opBitsToRealD(ثابت V3Number& lhs);
    V3Number& opNegateD(ثابت V3Number& lhs);
    V3Number& opAddD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opSubD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opMulD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opDivD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opPowD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    // Comparisons
    V3Number& opEqD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opNeqD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opGtD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opGteD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLtD(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLteD(ثابت V3Number& lhs, ثابت V3Number& rhs);

    // "N" - string operations
    V3Number& opAtoN(ثابت V3Number& lhs, صحيح base);
    V3Number& opNToI(ثابت V3Number& lhs);
    V3Number& opPutcN(ثابت V3Number& lhs, ثابت V3Number& rhs, ثابت V3Number& ths);
    V3Number& opGetcN(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opSubstrN(ثابت V3Number& lhs, ثابت V3Number& rhs, ثابت V3Number& ths);
    V3Number& opCompareNN(ثابت V3Number& lhs, ثابت V3Number& rhs, bool ignoreCase);
    V3Number& opConcatN(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opReplN(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opReplN(ثابت V3Number& lhs, uint32_t rhsval);
    V3Number& opEqN(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opNeqN(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opGtN(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opGteN(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLtN(ثابت V3Number& lhs, ثابت V3Number& rhs);
    V3Number& opLteN(ثابت V3Number& lhs, ثابت V3Number& rhs);
};
مضمن std::ostream& operator<<(std::ostream& os, ثابت V3Number& rhs) VL_MT_SAFE {
    أرجع os << rhs.ascii();
}

#endif  // Guard
