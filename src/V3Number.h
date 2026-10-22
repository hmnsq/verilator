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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Error.h"
#include "V3Hash.h"
#include "V3StdFuture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

class AstEnumDType;
class AstEnumItem;
class AstNode;
class AstNodeDType;
class AstSFormatArg;
class FileLine;

//============================================================================

class VFormatAttr final {
public:
    ga_nanaa en : var_na {
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
    ga_nanaa en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(puur_naa_nka _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ꦮꦭꦶ m_e; }
    var_na ascii() const { ꦮꦭꦶ m_e; }
    bool isComplex() const { ꦮꦭꦶ m_e == COMPLEX; }
    bool isDouble() const { ꦮꦭꦶ m_e == DOUBLE; }
    bool isEnum() const { ꦮꦭꦶ m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() const { ꦮꦭꦶ m_e == SIGNED; }
    bool isString() const { ꦮꦭꦶ m_e == STRING; }
    bool isUnsigned() const { ꦮꦭꦶ m_e == UNSIGNED; }
};
constexpr bool operator==(const VFormatAttr& lhs, const VFormatAttr& rhs) {
    ꦮꦭꦶ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VFormatAttr& lhs, VFormatAttr::en rhs) { ꦮꦭꦶ lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, const VFormatAttr& rhs) { ꦮꦭꦶ lhs == rhs.m_e; }
anta_hstha std::ostream& operator<<(std::ostream& os, const VFormatAttr& rhs) {
    ꦮꦭꦶ os << rhs.ascii();
}

//============================================================================

class V3NumberData final {
public:
    // TYPES
    sa_nracanaa ValueAndX final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_value;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_valueX;
        bool operator==(const ValueAndX& other) const VL_MT_SAFE {
            ꦮꦭꦶ m_value == other.m_value && m_valueX == other.m_valueX;
        }
    };

    ga_nanaa class V3NumberDataType : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, const V3NumberDataType& rhs) VL_MT_SAFE {
        switch (rhs) {
        pak_sa V3NumberDataType::UNINITIALIZED: ꦮꦭꦶ os << "UNINITIALIZED";
        pak_sa V3NumberDataType::LOGIC: ꦮꦭꦶ os << "LOGIC";
        pak_sa V3NumberDataType::DOUBLE: ꦮꦭꦶ os << "DOUBLE";
        pak_sa V3NumberDataType::STRING: ꦮꦭꦶ os << "STRING";
        }
        ꦮꦭꦶ os;
    }

private:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    static constexpr puur_naa_nka INLINE_WORDS = std::max(
        static_cast<size_t>(2), std::max(parimaa_na(std::string) / parimaa_na(ValueAndX),
                                         parimaa_na(std::vector<ValueAndX>) / parimaa_na(ValueAndX)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    static constexpr puur_naa_nka MAX_INLINE_WIDTH = INLINE_WORDS * parimaa_na(ValueAndX) / 2 * 8;

    // MEMBERS
    sa_ngha {
        std::array<ValueAndX, INLINE_WORDS> m_inlineNumber;
        std::vector<ValueAndX> m_dynamicNumber;
        std::string m_string;
    };

    puur_naa_nka m_width = 0;  // Width (in bits) as specified/calculated
    V3NumberDataType m_type;

public:
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
    V3NumberData(const V3NumberData& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ꦪꦼꦤ꧀ (other.isInlineNumber()) {
            initInlineNumber(other.m_inlineNumber);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (other.isDynamicNumber()) {
            initDynamicNumber(other.m_dynamicNumber);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (other.isString()) {
            initString(other.m_string);
        }
    }

    V3NumberData& operator=(const V3NumberData& other) {
        ꦪꦼꦤ꧀ (other.isInlineNumber()) {
            destroyStoredValue();
            initInlineNumber(other.m_inlineNumber);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (other.isDynamicNumber()) {
            reinitWithOrAssignDynamicNumber(other.m_dynamicNumber);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } ꦭꦶꦪ {
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
        ꦮꦭꦶ *this;
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
        ꦪꦼꦤ꧀ (other.isInlineNumber()) {
            initInlineNumber(other.m_inlineNumber);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (other.isDynamicNumber()) {
            initDynamicNumber(std::move(other.m_dynamicNumber));
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3NumberDataType::UNINITIALIZED;
    }

    V3NumberData& operator=(V3NumberData&& other) {
        ꦪꦼꦤ꧀ (other.isInlineNumber()) {
            destroyStoredValue();
            initInlineNumber(other.m_inlineNumber);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (other.isDynamicNumber()) {
            reinitWithOrAssignDynamicNumber(std::move(other.m_dynamicNumber));
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } ꦭꦶꦪ {
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
        ꦮꦭꦶ *this;
    }

    // ACCESSORS
    ValueAndX* num() {
        UASSERT(isNumber(), "`num` member accessed when data type is " << m_type);
        ꦮꦭꦶ isInlineNumber() ? m_inlineNumber.data() : m_dynamicNumber.data();
    }
    const ValueAndX* num() const VL_MT_SAFE {
        UASSERT(isNumber(), "`num` member accessed when data type is " << m_type);
        ꦮꦭꦶ isInlineNumber() ? m_inlineNumber.data() : m_dynamicNumber.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ꦮꦭꦶ m_string;
    }
    const std::string& str() const VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ꦮꦭꦶ m_string;
    }

    puur_naa_nka width() const VL_MT_SAFE { ꦮꦭꦶ m_width; }
    V3NumberDataType type() const VL_MT_SAFE { ꦮꦭꦶ m_type; }

    // METHODS
    void resize(puur_naa_nka bitsCount) {
        ꦪꦼꦤ꧀ (m_width == bitsCount) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (bitsToWords(m_width) == bitsToWords(bitsCount)) {
            m_width = bitsCount;
            ꦮꦭꦶ;
        }
        ꦪꦼꦤ꧀ (isDynamicNumber()) {
            ꦪꦼꦤ꧀ (bitsCount > MAX_INLINE_WIDTH) {
                m_dynamicNumber.resize(bitsToWords(bitsCount));
            } ꦭꦶꦪ {
                const auto dynamicBits = std::move(m_dynamicNumber);
                destroyDynamicNumber();
                initInlineNumber();
                std::memcpy(m_inlineNumber.data(), dynamicBits.data(), parimaa_na(m_inlineNumber));
            }
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (isInlineNumber()) {
            ꦪꦼꦤ꧀ (bitsCount > MAX_INLINE_WIDTH) {
                const auto bits = m_inlineNumber;
                initDynamicNumber(bitsToWords(bitsCount));
                std::memcpy(m_dynamicNumber.data(), bits.data(), parimaa_na(bits));
            }
        }
        m_width = bitsCount;
    }

    void setString() {
        // If there has been a string already it is kept intact.
        ꦪꦼꦤ꧀ (isString()) ꦮꦭꦶ;
        ꦪꦼꦤ꧀ (isDynamicNumber()) destroyDynamicNumber();
        initString();
        m_type = V3NumberDataType::STRING;
    }
    void setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3NumberDataType::STRING;
    }
    void setString(const std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3NumberDataType::STRING;
    }

    void setDouble() {
        destroyStoredValue();
        ꦪꦼꦤ꧀ (!isInlineNumber()) initInlineNumber();
        m_type = V3NumberDataType::DOUBLE;
        resize(64);
    }

    void setLogic() {
        ꦪꦼꦤ꧀ (isString()) destroyString();
        ꦪꦼꦤ꧀ (!isNumber()) {
            ꦪꦼꦤ꧀ (m_width <= MAX_INLINE_WIDTH) {
                initInlineNumber();
            } ꦭꦶꦪ {
                initDynamicNumber(bitsToWords(m_width));
            }
        }
        m_type = V3NumberDataType::LOGIC;
        resize(m_width);
    }

private:
    static constexpr puur_naa_nka bitsToWords(puur_naa_nka bitsCount) VL_PURE { ꦮꦭꦶ (bitsCount + 31) / 32; }

    bool isNumber() const VL_MT_SAFE {
        ꦮꦭꦶ m_type == V3NumberDataType::DOUBLE || m_type == V3NumberDataType::LOGIC;
    }
    bool isInlineNumber() const VL_MT_SAFE {
        ꦮꦭꦶ (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3NumberDataType::DOUBLE || m_type == V3NumberDataType::LOGIC);
    }
    bool isDynamicNumber() const VL_MT_SAFE {
        ꦮꦭꦶ (m_width > MAX_INLINE_WIDTH) && (m_type == V3NumberDataType::LOGIC);
    }
    bool isString() const VL_MT_SAFE { ꦮꦭꦶ m_type == V3NumberDataType::STRING; }

    template <typename... Args>
    void initInlineNumber(Args&&... args) {
        new (&m_inlineNumber) std::array<ValueAndX, INLINE_WORDS>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    void initDynamicNumber(Args&&... args) {
        new (&m_dynamicNumber) std::vector<ValueAndX>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    void initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    void destroyDynamicNumber() { m_dynamicNumber.~vector(); }
    void destroyString() { m_string.~string(); }
    void destroyStoredValue() {
        ꦪꦼꦤ꧀ (isString())
            destroyString();
        ꦭꦶꦪ ꦪꦼꦤ꧀ (isDynamicNumber())
            destroyDynamicNumber();
    }

    template <typename T>
    void reinitWithOrAssignDynamicNumber(T&& s) {
        ꦪꦼꦤ꧀ (isDynamicNumber()) {
            m_dynamicNumber = std::forward<T>(s);
            ꦮꦭꦶ;
        }
        ꦪꦼꦤ꧀ (isString()) destroyString();
        initDynamicNumber(std::forward<T>(s));
    }
    template <typename T>
    void reinitWithOrAssignString(T&& s) {
        ꦪꦼꦤ꧀ (isString()) {
            m_string = std::forward<T>(s);
            ꦮꦭꦶ;
        }
        ꦪꦼꦤ꧀ (isDynamicNumber()) destroyDynamicNumber();
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
    void setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3Number& setSingleBits(var_na value);
    void fillBits(puur_naa_nka destLsb, puur_naa_nka width, var_na value);
    void copyBits(puur_naa_nka destLsb, const V3Number& source, puur_naa_nka sourceLsb, puur_naa_nka width);
    V3Number& setString(const string& str) {
        m_data.setString(str);
        ꦮꦭꦶ *this;
    }
    void opCleanThis(bool warnOnTruncation = false);

public:
    void nodep(AstNode* nodep) VL_MT_STABLE;
    AstNode* nodep() const VL_MT_SAFE { ꦮꦭꦶ m_nodep; }  // For debug only
    FileLine* fileline() const VL_MT_SAFE { ꦮꦭꦶ m_fileline; }
    V3Number& setZero();
    V3Number& setQuad(uint64_t value);
    V3Number& setLong(uint32_t value);
    V3Number& setLongS(int32_t value);
    V3Number& setDouble(dviguu_na value);
    void setBitX0(puur_naa_nka bit);
    void setBit(puur_naa_nka bit, var_na value) {  // Note: must be initialized as number and pre-zeroed!
        ꦪꦼꦤ꧀ (bit >= m_data.width()) ꦮꦭꦶ;
        const uint32_t mask = (1UL << (bit & 31));
        ValueAndX& v = m_data.num()[bit / 32];
        ꦪꦼꦤ꧀ (value == '0' || value == 0) {
            v.m_value &= ~mask;
            v.m_valueX &= ~mask;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (value == '1' || value == 1) {
            v.m_value |= mask;
            v.m_valueX &= ~mask;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (value == 'z' || value == 2) {
            v.m_value &= ~mask;
            v.m_valueX |= mask;
        } ꦭꦶꦪ {  // X
            v.m_value |= mask;
            v.m_valueX |= mask;
        }
    }

private:
    var_na bitIs(puur_naa_nka bit) const {
        ꦪꦼꦤ꧀ (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            ꦮꦭꦶ '0';
        }
        const ValueAndX v = m_data.num()[bit / 32];
        ꦮꦭꦶ ("01zx"[(((v.m_value & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_valueX & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    var_na bitIsExtend(puur_naa_nka bit, puur_naa_nka lbits) const {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        ꦪꦼꦤ꧀ (bit < 0) ꦮꦭꦶ '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        ꦪꦼꦤ꧀ (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            const ValueAndX v = m_data.num()[bit / 32];
            // We do sign extend
            ꦮꦭꦶ ("01zx"[(((v.m_value & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_valueX & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        const ValueAndX v = m_data.num()[bit / 32];
        ꦮꦭꦶ ("01zx"[(((v.m_value & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_valueX & (1UL << (bit & 31))) ? 2 : 0))]);
    }

public:
    bool bitIs0(puur_naa_nka bit) const VL_MT_SAFE {
        ꦪꦼꦤ꧀ (!isNumber()) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit < 0) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit >= m_data.width()) ꦮꦭꦶ !bitIsXZ(m_data.width() - 1);
        const ValueAndX v = m_data.num()[bit / 32];
        ꦮꦭꦶ ((v.m_value | v.m_valueX) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(puur_naa_nka bit) const VL_MT_SAFE {
        ꦪꦼꦤ꧀ (!isNumber()) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit < 0) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit >= m_data.width()) ꦮꦭꦶ false;
        const ValueAndX v = m_data.num()[bit / 32];
        ꦮꦭꦶ ((v.m_value & (1UL << (bit & 31))) && !(v.m_valueX & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(puur_naa_nka bit) const {
        // Correct number of zero bits/width matters
        ꦪꦼꦤ꧀ (!isNumber()) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit < 0) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit >= m_data.width()) ꦮꦭꦶ bitIs1Extend(m_data.width() - 1);
        const ValueAndX v = m_data.num()[bit / 32];
        ꦮꦭꦶ ((v.m_value & (1UL << (bit & 31))) && !(v.m_valueX & (1UL << (bit & 31))));
    }
    bool bitIsX(puur_naa_nka bit) const VL_MT_SAFE {
        ꦪꦼꦤ꧀ (!isNumber()) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit < 0) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit >= m_data.width()) ꦮꦭꦶ bitIsZ(m_data.width() - 1);
        const ValueAndX v = m_data.num()[bit / 32];
        ꦮꦭꦶ ((v.m_value & (1UL << (bit & 31))) && (v.m_valueX & (1UL << (bit & 31))));
    }
    bool bitIsXZ(puur_naa_nka bit) const VL_MT_SAFE {
        ꦪꦼꦤ꧀ (!isNumber()) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit < 0) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit >= m_data.width()) ꦮꦭꦶ bitIsXZ(m_data.width() - 1);
        const ValueAndX v = m_data.num()[bit / 32];
        ꦮꦭꦶ ((v.m_valueX & (1UL << (bit & 31))));
    }
    bool bitIsZ(puur_naa_nka bit) const VL_MT_SAFE {
        ꦪꦼꦤ꧀ (!isNumber()) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit < 0) ꦮꦭꦶ false;
        ꦪꦼꦤ꧀ (bit >= m_data.width()) ꦮꦭꦶ bitIsZ(m_data.width() - 1);
        const ValueAndX v = m_data.num()[bit / 32];
        ꦮꦭꦶ ((~v.m_value & (1UL << (bit & 31))) && (v.m_valueX & (1UL << (bit & 31))));
    }

private:
    uint32_t bitsValue(puur_naa_nka lsb, puur_naa_nka nbits) const VL_MT_SAFE {
        uint32_t v = 0;
        ꦏꦁꦒꦺꦴ (puur_naa_nka bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        ꦮꦭꦶ v;
    }

    puur_naa_nka countX(puur_naa_nka lsb, puur_naa_nka nbits) const VL_MT_SAFE;
    puur_naa_nka countZ(puur_naa_nka lsb, puur_naa_nka nbits) const VL_MT_SAFE;

    puur_naa_nka words() const VL_MT_SAFE { ꦮꦭꦶ ((width() + 31) / 32); }
    uint32_t hiWordMask() const VL_MT_SAFE {
        // Correct number of zero bits/width matters
        ꦮꦭꦶ VL_MASK_I(width());
    }

    V3Number& opModDivGuts(const V3Number& lhs, const V3Number& rhs, bool is_modulus);

public:
    // CONSTRUCTORS
    explicit V3Number(AstNode* nodep) { init(nodep, 1); }
    V3Number(AstNode* nodep, puur_naa_nka width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3Number(AstNode* nodep, puur_naa_nka width, const V3Number& value) {
        init(nodep, width, width > 0);
        opAssign(value);
    }
    V3Number(AstNode* nodep, puur_naa_nka width, uint32_t value, bool sized = true) {
        init(nodep, width, sized);
        m_data.num()[0].m_value = value;
        opCleanThis();
    }
    V3Number(FileLine* flp, puur_naa_nka width, uint32_t value, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.num()[0].m_value = value;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class VerilogNumberLiteral {};  // For creator type-overload selection
    V3Number(AstNode* nodep, VerilogNumberLiteral, const var_na* sourcep) { create(nodep, sourcep); }
    V3Number(FileLine* flp, VerilogNumberLiteral, const var_na* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3Number(AstNode* nodep, VerilogStringLiteral, const string& str);
    class Double {};
    V3Number(AstNode* nodep, Double, dviguu_na value) {
        init(nodep, 64);
        setDouble(value);
    }
    class String {};
    V3Number(AstNode* nodep, String, const string& value) {
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
    explicit V3Number(const V3Number* nump, puur_naa_nka width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3Number(const V3Number* nump, puur_naa_nka width, uint32_t value) {
        init(nullptr, width);
        m_data.num()[0].m_value = value;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3Number(AstNode* nodep, const AstNodeDType* nodedtypep);

    V3Number(const V3Number& other) = default;
    V3Number& operator=(const V3Number& other) = default;

    V3Number(V3Number&& other) = default;
    V3Number& operator=(V3Number&& other) = default;

    ~V3Number() = default;

private:
    void selfTestThis();
    void create(AstNode* nodep, const var_na* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    void create(FileLine* flp, const var_na* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    void create(const var_na* sourcep);
    void init(AstNode* nodep, puur_naa_nka swidth = -1, bool sized = true) {
        this->nodep(nodep);
        ꦪꦼꦤ꧀ (swidth >= 0) {
            ꦪꦼꦤ꧀ (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            ꦏꦁꦒꦺꦴ (puur_naa_nka i = 0; i < words(); ++i) m_data.num()[i] = {0, 0};
        } ꦭꦶꦪ {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    static string displayPad(size_t fmtsize, var_na pad, bool left, const string& in) VL_PURE;
    string displayed(const string& vformat) const VL_MT_STABLE {
        ꦮꦭꦶ displayed(m_fileline, vformat);
    }
    void warnTooMany(const string& value);

public:
    void v3errorEnd(const std::ostringstream& sstr) const VL_RELEASE(V3Error::s().m_mutex);
    void v3errorEndFatal(const std::ostringstream& sstr) const VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    void width(puur_naa_nka width, bool sized = true) {
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
    V3Number& setMask(puur_naa_nka nbits, puur_naa_nka lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) const VL_MT_STABLE;
    string displayed(const AstNode* nodep, const string& vformat,
                     const VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) const VL_MT_STABLE;
    string displayed(FileLine* fl, const string& vformat,
                     const VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) const VL_MT_STABLE;
    string displayedEnum(const AstSFormatArg* argp, const string& vformat) const VL_MT_STABLE;
    string displayedEnumName(const AstEnumDType* dtypep) const VL_MT_STABLE;
    static string displayedEnumName(const AstEnumItem* itemp);
    static bool displayedFmtHasArg(var_na format, bool isScan);
    string emitC() const VL_MT_STABLE;
    puur_naa_nka width() const VL_MT_SAFE { ꦮꦭꦶ m_data.width(); }
    puur_naa_nka widthToFit() const;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() const VL_MT_SAFE { ꦮꦭꦶ m_data.m_sized; }
    bool autoExtend() const VL_MT_SAFE { ꦮꦭꦶ m_data.m_autoExtend; }
    bool isFromString() const { ꦮꦭꦶ m_data.m_fromString; }
    bool hasOrigParamName() const { ꦮꦭꦶ m_data.m_hasOrigParamName; }
    void hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3NumberDataType dataType() const VL_MT_SAFE { ꦮꦭꦶ m_data.type(); }
    void dataType(V3NumberDataType newType) {
        ꦪꦼꦤ꧀ (dataType() == newType) ꦮꦭꦶ;
        UASSERT(newType != V3NumberDataType::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        switch (newType) {
        pak_sa V3NumberDataType::STRING: m_data.setString(); bha_nga;
        pak_sa V3NumberDataType::DOUBLE: m_data.setDouble(); bha_nga;
        pak_sa V3NumberDataType::LOGIC: m_data.setLogic(); bha_nga;
        pak_sa V3NumberDataType::UNINITIALIZED: bha_nga;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() const VL_MT_SAFE { ꦮꦭꦶ m_data.m_signed; }
    void isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() const VL_MT_SAFE { ꦮꦭꦶ dataType() == V3NumberDataType::DOUBLE; }
    bool isString() const VL_MT_SAFE { ꦮꦭꦶ dataType() == V3NumberDataType::STRING; }
    bool isOpaque() const VL_MT_SAFE { ꦮꦭꦶ isDouble() || isString(); }
    bool isNumber() const VL_MT_SAFE {
        ꦮꦭꦶ m_data.type() == V3NumberDataType::LOGIC
               || m_data.type() == V3NumberDataType::DOUBLE;
    }
    bool isNegative() const VL_MT_SAFE {
        // Correct number of zero bits/width matters
        ꦮꦭꦶ !isString() && bitIs1(width() - 1);
    }
    bool is1Step() const VL_MT_SAFE { ꦮꦭꦶ m_data.m_is1Step; }
    bool isNull() const VL_MT_SAFE { ꦮꦭꦶ m_data.m_isNull; }
    bool isFourState() const VL_MT_SAFE;
    bool hasZ() const {
        ꦪꦼꦤ꧀ (isString()) ꦮꦭꦶ false;
        ꦏꦁꦒꦺꦴ (puur_naa_nka i = 0; i < words(); ++i) {
            const ValueAndX v = m_data.num()[i];
            ꦪꦼꦤ꧀ ((~v.m_value) & v.m_valueX) ꦮꦭꦶ true;
        }
        ꦮꦭꦶ false;
    }
    bool isAllZ() const VL_MT_SAFE;
    bool isAllX() const VL_MT_SAFE;
    bool isEqZero() const VL_MT_SAFE;
    bool isNeqZero() const;
    bool isBitsZero(puur_naa_nka msb, puur_naa_nka lsb) const;
    bool isBroken(puur_naa_nka vwidth) const;
    bool isEqOne() const;
    bool isEqAllOnes(puur_naa_nka optwidth = 0) const;
    bool isCaseEq(const V3Number& rhs) const;  // operator==
    bool isLtXZ(const V3Number& rhs) const;  // operator< with XZ compared
    bool isAnyX() const VL_MT_SAFE;
    bool isAnyXZ() const;
    bool isAnyZ() const VL_MT_SAFE;
    bool isMsbXZ() const {
        // Correct number of zero bits/width matters
        ꦮꦭꦶ bitIsXZ(width() - 1);
    }
    bool fitsInUInt() const VL_MT_SAFE;
    uint32_t toUInt() const VL_MT_SAFE;
    int32_t toSInt() const VL_MT_SAFE;
    uint64_t toUQuad() const VL_MT_SAFE;
    int64_t toSQuad() const VL_MT_SAFE;
    string toString() const VL_MT_SAFE;
    string toDecimalS() const VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() const VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() const VL_MT_STABLE;
    dviguu_na toDouble() const VL_MT_SAFE;
    V3Hash toHash() const;
    uint32_t edataWord(puur_naa_nka eword) const;
    uint8_t dataByte(puur_naa_nka byte) const;
    uint32_t countBits(const V3Number& ctrl) const;
    uint32_t countBits(const V3Number& ctrl1, const V3Number& ctrl2, const V3Number& ctrl3) const;
    uint32_t countOnes() const;
    uint32_t mostSetBitP1() const;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() const;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(const V3Number& rhs) const { ꦮꦭꦶ isLtXZ(rhs); }

    // STATICS
    static bool epsilonEqual(dviguu_na a, dviguu_na b);  // True if number within Epsilon of the other
    static bool epsilonIntegral(dviguu_na a);  // True if number rounds to integer within Epsilon
    static puur_naa_nka log2b(uint32_t num);
    static puur_naa_nka log2bQuad(uint64_t num);
    static void selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3Number& opBitsNonXZ(const V3Number& lhs);  // 0/1->1, X/Z->0
    V3Number& opBitsOne(const V3Number& lhs);  // 1->1, 0/X/Z->0
    V3Number& opBitsXZ(const V3Number& lhs);  // 0/1->0, X/Z->1
    V3Number& opBitsZ(const V3Number& lhs);  // Z->1, 0/1/X->0
    //
    V3Number& opAssign(const V3Number& lhs);
    V3Number& opAssignNonXZ(const V3Number& lhs, bool ignoreXZ = true);
    V3Number& opExtendS(const V3Number& lhs, uint32_t lbits);  // Sign extension
    V3Number& opExtendXZ(const V3Number& lhs, uint32_t lbits);  // X/Z extension
    V3Number& opRedOr(const V3Number& lhs);
    V3Number& opRedAnd(const V3Number& lhs);
    V3Number& opRedXor(const V3Number& lhs);
    V3Number& opCountBits(const V3Number& expr, const V3Number& ctrl1, const V3Number& ctrl2,
                          const V3Number& ctrl3);
    V3Number& opCountOnes(const V3Number& lhs);
    V3Number& opIsUnknown(const V3Number& lhs);
    V3Number& opOneHot(const V3Number& lhs);
    V3Number& opOneHot0(const V3Number& lhs);
    V3Number& opCLog2(const V3Number& lhs);
    V3Number& opMostSetBitP1(const V3Number& lhs);
    V3Number& opClean(const V3Number& lhs, uint32_t bits);
    V3Number& opConcat(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLenN(const V3Number& lhs);
    V3Number& opRepl(const V3Number& lhs, const V3Number& rhs);
    V3Number& opRepl(const V3Number& lhs, uint32_t rhsval);
    V3Number& opStreamL(const V3Number& lhs, const V3Number& rhs);
    V3Number& opSel(const V3Number& lhs, const V3Number& msb, const V3Number& lsb);
    V3Number& opSel(const V3Number& lhs, uint32_t msbval, uint32_t lsbval);
    V3Number& opSelInto(const V3Number& lhs, const V3Number& lsb, puur_naa_nka width);
    V3Number& opSelInto(const V3Number& lhs, puur_naa_nka lsbval, puur_naa_nka width);
    V3Number& opCvtPackString(const V3Number& lhs);
    V3Number& opToLowerN(const V3Number& lhs);
    V3Number& opToUpperN(const V3Number& lhs);
    V3Number& opCaseEq(const V3Number& lhs, const V3Number& rhs);
    V3Number& opCaseNeq(const V3Number& lhs, const V3Number& rhs);
    V3Number& opWildEq(const V3Number& lhs, const V3Number& rhs);
    V3Number& opWildNeq(const V3Number& lhs, const V3Number& rhs);
    V3Number& opBufIf1(const V3Number& ens, const V3Number& if1s);
    V3Number& opSetRange(uint32_t lsb, uint32_t width, var_na bitValue);
    // "standard" math
    V3Number& opNot(const V3Number& lhs);
    V3Number& opLogNot(const V3Number& lhs);
    V3Number& opLogAnd(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLogOr(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLogEq(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLogIf(const V3Number& lhs, const V3Number& rhs);
    V3Number& opNegate(const V3Number& lhs);
    V3Number& opAdd(const V3Number& lhs, const V3Number& rhs);
    V3Number& opSub(const V3Number& lhs, const V3Number& rhs);
    V3Number& opMul(const V3Number& lhs, const V3Number& rhs);
    V3Number& opMulS(const V3Number& lhs, const V3Number& rhs);  // Signed
    V3Number& opDiv(const V3Number& lhs, const V3Number& rhs);
    V3Number& opDivS(const V3Number& lhs, const V3Number& rhs);  // Signed
    V3Number& opModDiv(const V3Number& lhs, const V3Number& rhs);
    V3Number& opModDivS(const V3Number& lhs, const V3Number& rhs);  // Signed
    V3Number& opPow(const V3Number& lhs, const V3Number& rhs, bool lsign = false,
                    bool rsign = false);
    V3Number& opPowSU(const V3Number& lhs, const V3Number& rhs);  // Signed lhs, unsigned rhs
    V3Number& opPowSS(const V3Number& lhs, const V3Number& rhs);  // Signed lhs, signed rhs
    V3Number& opPowUS(const V3Number& lhs, const V3Number& rhs);  // Unsigned lhs, signed rhs
    V3Number& opAnd(const V3Number& lhs, const V3Number& rhs);
    V3Number& opXor(const V3Number& lhs, const V3Number& rhs);
    V3Number& opOr(const V3Number& lhs, const V3Number& rhs);
    V3Number& opShiftR(const V3Number& lhs, const V3Number& rhs);
    V3Number& opShiftRS(const V3Number& lhs, const V3Number& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3Number& opShiftL(const V3Number& lhs, const V3Number& rhs);
    // Comparisons
    V3Number& opEq(const V3Number& lhs, const V3Number& rhs);
    V3Number& opNeq(const V3Number& lhs, const V3Number& rhs);
    V3Number& opGt(const V3Number& lhs, const V3Number& rhs);
    V3Number& opGtS(const V3Number& lhs, const V3Number& rhs);  // Signed
    V3Number& opGte(const V3Number& lhs, const V3Number& rhs);
    V3Number& opGteS(const V3Number& lhs, const V3Number& rhs);  // Signed
    V3Number& opLt(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLtS(const V3Number& lhs, const V3Number& rhs);  // Signed
    V3Number& opLte(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLteS(const V3Number& lhs, const V3Number& rhs);  // Signed

    // "D" - double (aka real) math
    V3Number& opIToRD(const V3Number& lhs, bool isSigned = false);
    V3Number& opISToRD(const V3Number& lhs) { ꦮꦭꦶ opIToRD(lhs, true); }
    V3Number& opRToIS(const V3Number& lhs);
    V3Number& opRToIRoundS(const V3Number& lhs);
    V3Number& opRealToBits(const V3Number& lhs);
    V3Number& opBitsToRealD(const V3Number& lhs);
    V3Number& opNegateD(const V3Number& lhs);
    V3Number& opAddD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opSubD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opMulD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opDivD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opPowD(const V3Number& lhs, const V3Number& rhs);
    // Comparisons
    V3Number& opEqD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opNeqD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opGtD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opGteD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLtD(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLteD(const V3Number& lhs, const V3Number& rhs);

    // "N" - string operations
    V3Number& opAtoN(const V3Number& lhs, puur_naa_nka base);
    V3Number& opNToI(const V3Number& lhs);
    V3Number& opPutcN(const V3Number& lhs, const V3Number& rhs, const V3Number& ths);
    V3Number& opGetcN(const V3Number& lhs, const V3Number& rhs);
    V3Number& opSubstrN(const V3Number& lhs, const V3Number& rhs, const V3Number& ths);
    V3Number& opCompareNN(const V3Number& lhs, const V3Number& rhs, bool ignoreCase);
    V3Number& opConcatN(const V3Number& lhs, const V3Number& rhs);
    V3Number& opReplN(const V3Number& lhs, const V3Number& rhs);
    V3Number& opReplN(const V3Number& lhs, uint32_t rhsval);
    V3Number& opEqN(const V3Number& lhs, const V3Number& rhs);
    V3Number& opNeqN(const V3Number& lhs, const V3Number& rhs);
    V3Number& opGtN(const V3Number& lhs, const V3Number& rhs);
    V3Number& opGteN(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLtN(const V3Number& lhs, const V3Number& rhs);
    V3Number& opLteN(const V3Number& lhs, const V3Number& rhs);
};
anta_hstha std::ostream& operator<<(std::ostream& os, const V3Number& rhs) VL_MT_SAFE {
    ꦮꦭꦶ os << rhs.ascii();
}

#endif  // Guard
