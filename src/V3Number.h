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

#ifndef VERILATOR_V3_नुमेरो_H
#define VERILATOR_V3_नुमेरो_H

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
class Ast_बुहोल;
class Ast_बुहोल_DType;
class AstSFormatArg;
class File_लिन्या;

//============================================================================

class VFormatAttr final {
public:
    enum en : char {
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
    enum en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(int _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { return m_e; }
    char ascii() const { return m_e; }
    bool isComplex() const { return m_e == COMPLEX; }
    bool isDouble() const { return m_e == DOUBLE; }
    bool isEnum() const { return m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() const { return m_e == SIGNED; }
    bool isString() const { return m_e == STRING; }
    bool isUnsigned() const { return m_e == UNSIGNED; }
};
constexpr bool operator==(const VFormatAttr& lhs, const VFormatAttr& rhs) {
    return lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VFormatAttr& lhs, VFormatAttr::en rhs) { return lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, const VFormatAttr& rhs) { return lhs == rhs.m_e; }
inline std::ostream& operator<<(std::ostream& os, const VFormatAttr& rhs) {
    return os << rhs.ascii();
}

//============================================================================

class V3_नुमेरो_Data final {
public:
    // TYPES
    struct हालागा_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_हालागा;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_हालागा_X;
        bool operator==(const हालागा_And_X& other) const VL_MT_SAFE {
            return m_हालागा == other.m_हालागा && m_हालागा_X == other.m_हालागा_X;
        }
    };

    enum class V3_नुमेरो_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, const V3_नुमेरो_Data_Type& rhs) VL_MT_SAFE {
        switch (rhs) {
        case V3_नुमेरो_Data_Type::UNINITIALIZED: return os << "UNINITIALIZED";
        case V3_नुमेरो_Data_Type::LOGIC: return os << "LOGIC";
        case V3_नुमेरो_Data_Type::DOUBLE: return os << "DOUBLE";
        case V3_नुमेरो_Data_Type::STRING: return os << "STRING";
        }
        return os;
    }

private:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    static constexpr int INLINE_मगा_सालिता = std::max(
        static_cast<size_t>(2), std::max(sizeof(std::string) / sizeof(हालागा_And_X),
                                         sizeof(std::vector<हालागा_And_X>) / sizeof(हालागा_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    static constexpr int MAX_INLINE_WIDTH = INLINE_मगा_सालिता * sizeof(हालागा_And_X) / 2 * 8;

    // MEMBERS
    union {
        std::array<हालागा_And_X, INLINE_मगा_सालिता> m_inline_नुमेरो;
        std::vector<हालागा_And_X> m_dynamic_नुमेरो;
        std::string m_string;
    };

    int m_width = 0;  // Width (in bits) as specified/calculated
    V3_नुमेरो_Data_Type m_type;

public:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_नुमेरो_Data()
        : m_type{V3_नुमेरो_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_नुमेरो_Data() { destroy_Stored_हालागा(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_नुमेरो_Data(const V3_नुमेरो_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        if (other.is_Inline_नुमेरो()) {
            init_Inline_नुमेरो(other.m_inline_नुमेरो);
        } else if (other.is_Dynamic_नुमेरो()) {
            init_Dynamic_नुमेरो(other.m_dynamic_नुमेरो);
        } else if (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_नुमेरो_Data& operator=(const V3_नुमेरो_Data& other) {
        if (other.is_Inline_नुमेरो()) {
            destroy_Stored_हालागा();
            init_Inline_नुमेरो(other.m_inline_नुमेरो);
        } else if (other.is_Dynamic_नुमेरो()) {
            reinit_With_Or_Assign_Dynamic_नुमेरो(other.m_dynamic_नुमेरो);
        } else if (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } else {
            destroy_Stored_हालागा();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        return *this;
    }

    V3_नुमेरो_Data(V3_नुमेरो_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        if (other.is_Inline_नुमेरो()) {
            init_Inline_नुमेरो(other.m_inline_नुमेरो);
        } else if (other.is_Dynamic_नुमेरो()) {
            init_Dynamic_नुमेरो(std::move(other.m_dynamic_नुमेरो));
        } else if (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_नुमेरो_Data_Type::UNINITIALIZED;
    }

    V3_नुमेरो_Data& operator=(V3_नुमेरो_Data&& other) {
        if (other.is_Inline_नुमेरो()) {
            destroy_Stored_हालागा();
            init_Inline_नुमेरो(other.m_inline_नुमेरो);
        } else if (other.is_Dynamic_नुमेरो()) {
            reinit_With_Or_Assign_Dynamic_नुमेरो(std::move(other.m_dynamic_नुमेरो));
        } else if (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } else {
            destroy_Stored_हालागा();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_नुमेरो_Data_Type::UNINITIALIZED;
        return *this;
    }

    // ACCESSORS
    हालागा_And_X* नुमेरो() {
        UASSERT(is_नुमेरो(), "`num` member accessed when data type is " << m_type);
        return is_Inline_नुमेरो() ? m_inline_नुमेरो.data() : m_dynamic_नुमेरो.data();
    }
    const हालागा_And_X* नुमेरो() const VL_MT_SAFE {
        UASSERT(is_नुमेरो(), "`num` member accessed when data type is " << m_type);
        return is_Inline_नुमेरो() ? m_inline_नुमेरो.data() : m_dynamic_नुमेरो.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        return m_string;
    }
    const std::string& str() const VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        return m_string;
    }

    int width() const VL_MT_SAFE { return m_width; }
    V3_नुमेरो_Data_Type type() const VL_MT_SAFE { return m_type; }

    // METHODS
    void resize(int bits_बिलांग) {
        if (m_width == bits_बिलांग) return;
        if (bits_To_मगा_सालिता(m_width) == bits_To_मगा_सालिता(bits_बिलांग)) {
            m_width = bits_बिलांग;
            return;
        }
        if (is_Dynamic_नुमेरो()) {
            if (bits_बिलांग > MAX_INLINE_WIDTH) {
                m_dynamic_नुमेरो.resize(bits_To_मगा_सालिता(bits_बिलांग));
            } else {
                const auto dynamicBits = std::move(m_dynamic_नुमेरो);
                destroy_Dynamic_नुमेरो();
                init_Inline_नुमेरो();
                std::memcpy(m_inline_नुमेरो.data(), dynamicBits.data(), sizeof(m_inline_नुमेरो));
            }
        } else if (is_Inline_नुमेरो()) {
            if (bits_बिलांग > MAX_INLINE_WIDTH) {
                const auto bits = m_inline_नुमेरो;
                init_Dynamic_नुमेरो(bits_To_मगा_सालिता(bits_बिलांग));
                std::memcpy(m_dynamic_नुमेरो.data(), bits.data(), sizeof(bits));
            }
        }
        m_width = bits_बिलांग;
    }

    void setString() {
        // If there has been a string already it is kept intact.
        if (isString()) return;
        if (is_Dynamic_नुमेरो()) destroy_Dynamic_नुमेरो();
        initString();
        m_type = V3_नुमेरो_Data_Type::STRING;
    }
    void setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_नुमेरो_Data_Type::STRING;
    }
    void setString(const std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_नुमेरो_Data_Type::STRING;
    }

    void setDouble() {
        destroy_Stored_हालागा();
        if (!is_Inline_नुमेरो()) init_Inline_नुमेरो();
        m_type = V3_नुमेरो_Data_Type::DOUBLE;
        resize(64);
    }

    void setLogic() {
        if (isString()) destroyString();
        if (!is_नुमेरो()) {
            if (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_नुमेरो();
            } else {
                init_Dynamic_नुमेरो(bits_To_मगा_सालिता(m_width));
            }
        }
        m_type = V3_नुमेरो_Data_Type::LOGIC;
        resize(m_width);
    }

private:
    static constexpr int bits_To_मगा_सालिता(int bits_बिलांग) VL_PURE { return (bits_बिलांग + 31) / 32; }

    bool is_नुमेरो() const VL_MT_SAFE {
        return m_type == V3_नुमेरो_Data_Type::DOUBLE || m_type == V3_नुमेरो_Data_Type::LOGIC;
    }
    bool is_Inline_नुमेरो() const VL_MT_SAFE {
        return (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_नुमेरो_Data_Type::DOUBLE || m_type == V3_नुमेरो_Data_Type::LOGIC);
    }
    bool is_Dynamic_नुमेरो() const VL_MT_SAFE {
        return (m_width > MAX_INLINE_WIDTH) && (m_type == V3_नुमेरो_Data_Type::LOGIC);
    }
    bool isString() const VL_MT_SAFE { return m_type == V3_नुमेरो_Data_Type::STRING; }

    template <typename... मगा_कातविरान_2>
    void init_Inline_नुमेरो(मगा_कातविरान_2&&... मगा_कातविरान) {
        new (&m_inline_नुमेरो) std::array<हालागा_And_X, INLINE_मगा_सालिता>(std::forward<मगा_कातविरान_2>(मगा_कातविरान)...);
    }
    template <typename... मगा_कातविरान_2>
    void init_Dynamic_नुमेरो(मगा_कातविरान_2&&... मगा_कातविरान) {
        new (&m_dynamic_नुमेरो) std::vector<हालागा_And_X>(std::forward<मगा_कातविरान_2>(मगा_कातविरान)...);
    }
    template <typename... मगा_कातविरान_2>
    void initString(मगा_कातविरान_2&&... मगा_कातविरान) {
        new (&m_string) std::string(std::forward<मगा_कातविरान_2>(मगा_कातविरान)...);
    }

    string formatDecimal();

    void destroy_Dynamic_नुमेरो() { m_dynamic_नुमेरो.~vector(); }
    void destroyString() { m_string.~string(); }
    void destroy_Stored_हालागा() {
        if (isString())
            destroyString();
        else if (is_Dynamic_नुमेरो())
            destroy_Dynamic_नुमेरो();
    }

    template <typename T>
    void reinit_With_Or_Assign_Dynamic_नुमेरो(T&& s) {
        if (is_Dynamic_नुमेरो()) {
            m_dynamic_नुमेरो = std::forward<T>(s);
            return;
        }
        if (isString()) destroyString();
        init_Dynamic_नुमेरो(std::forward<T>(s));
    }
    template <typename T>
    void reinitWithOrAssignString(T&& s) {
        if (isString()) {
            m_string = std::forward<T>(s);
            return;
        }
        if (is_Dynamic_नुमेरो()) destroy_Dynamic_नुमेरो();
        initString(std::forward<T>(s));
    }
};

class V3_नुमेरो final {
    // TYPES
    using हालागा_And_X = V3_नुमेरो_Data::हालागा_And_X;
    using V3_नुमेरो_Data_Type = V3_नुमेरो_Data::V3_नुमेरो_Data_Type;

    // MEMBERS
    V3_नुमेरो_Data m_data;
    Ast_बुहोल* m_nodep = nullptr;  // Parent node - for error reporting only
    File_लिन्या* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    void setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_नुमेरो& setSingleBits(char हालागा);
    void fillBits(int destLsb, int width, char हालागा);
    void copyBits(int destLsb, const V3_नुमेरो& source, int sourceLsb, int width);
    V3_नुमेरो& setString(const string& str) {
        m_data.setString(str);
        return *this;
    }
    void opCleanThis(bool warnOnTruncation = false);

public:
    void nodep(Ast_बुहोल* nodep) VL_MT_STABLE;
    Ast_बुहोल* nodep() const VL_MT_SAFE { return m_nodep; }  // For debug only
    File_लिन्या* fileline() const VL_MT_SAFE { return m_fileline; }
    V3_नुमेरो& setZero();
    V3_नुमेरो& setQuad(uint64_t हालागा);
    V3_नुमेरो& setLong(uint32_t हालागा);
    V3_नुमेरो& setLongS(int32_t हालागा);
    V3_नुमेरो& setDouble(double हालागा);
    void setBitX0(int bit);
    void setBit(int bit, char हालागा) {  // Note: must be initialized as number and pre-zeroed!
        if (bit >= m_data.width()) return;
        const uint32_t mask = (1UL << (bit & 31));
        हालागा_And_X& v = m_data.नुमेरो()[bit / 32];
        if (हालागा == '0' || हालागा == 0) {
            v.m_हालागा &= ~mask;
            v.m_हालागा_X &= ~mask;
        } else if (हालागा == '1' || हालागा == 1) {
            v.m_हालागा |= mask;
            v.m_हालागा_X &= ~mask;
        } else if (हालागा == 'z' || हालागा == 2) {
            v.m_हालागा &= ~mask;
            v.m_हालागा_X |= mask;
        } else {  // X
            v.m_हालागा |= mask;
            v.m_हालागा_X |= mask;
        }
    }

private:
    char bitIs(int bit) const {
        if (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            return '0';
        }
        const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
        return ("01zx"[(((v.m_हालागा & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_हालागा_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    char bitIsExtend(int bit, int lbits) const {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        if (bit < 0) return '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        if (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
            // We do sign extend
            return ("01zx"[(((v.m_हालागा & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_हालागा_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
        return ("01zx"[(((v.m_हालागा & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_हालागा_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

public:
    bool bitIs0(int bit) const VL_MT_SAFE {
        if (!is_नुमेरो()) return false;
        if (bit < 0) return false;
        if (bit >= m_data.width()) return !bitIsXZ(m_data.width() - 1);
        const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
        return ((v.m_हालागा | v.m_हालागा_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(int bit) const VL_MT_SAFE {
        if (!is_नुमेरो()) return false;
        if (bit < 0) return false;
        if (bit >= m_data.width()) return false;
        const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
        return ((v.m_हालागा & (1UL << (bit & 31))) && !(v.m_हालागा_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(int bit) const {
        // Correct number of zero bits/width matters
        if (!is_नुमेरो()) return false;
        if (bit < 0) return false;
        if (bit >= m_data.width()) return bitIs1Extend(m_data.width() - 1);
        const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
        return ((v.m_हालागा & (1UL << (bit & 31))) && !(v.m_हालागा_X & (1UL << (bit & 31))));
    }
    bool bitIsX(int bit) const VL_MT_SAFE {
        if (!is_नुमेरो()) return false;
        if (bit < 0) return false;
        if (bit >= m_data.width()) return bitIsZ(m_data.width() - 1);
        const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
        return ((v.m_हालागा & (1UL << (bit & 31))) && (v.m_हालागा_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(int bit) const VL_MT_SAFE {
        if (!is_नुमेरो()) return false;
        if (bit < 0) return false;
        if (bit >= m_data.width()) return bitIsXZ(m_data.width() - 1);
        const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
        return ((v.m_हालागा_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(int bit) const VL_MT_SAFE {
        if (!is_नुमेरो()) return false;
        if (bit < 0) return false;
        if (bit >= m_data.width()) return bitIsZ(m_data.width() - 1);
        const हालागा_And_X v = m_data.नुमेरो()[bit / 32];
        return ((~v.m_हालागा & (1UL << (bit & 31))) && (v.m_हालागा_X & (1UL << (bit & 31))));
    }

private:
    uint32_t bits_हालागा(int lsb, int nbits) const VL_MT_SAFE {
        uint32_t v = 0;
        for (int bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        return v;
    }

    int बिलांग_X(int lsb, int nbits) const VL_MT_SAFE;
    int बिलांग_Z(int lsb, int nbits) const VL_MT_SAFE;

    int मगा_सालिता() const VL_MT_SAFE { return ((width() + 31) / 32); }
    uint32_t hiWordMask() const VL_MT_SAFE {
        // Correct number of zero bits/width matters
        return VL_MASK_I(width());
    }

    V3_नुमेरो& opModDivGuts(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, bool is_modulus);

public:
    // CONSTRUCTORS
    explicit V3_नुमेरो(Ast_बुहोल* nodep) { init(nodep, 1); }
    V3_नुमेरो(Ast_बुहोल* nodep, int width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_नुमेरो(Ast_बुहोल* nodep, int width, const V3_नुमेरो& हालागा) {
        init(nodep, width, width > 0);
        opAssign(हालागा);
    }
    V3_नुमेरो(Ast_बुहोल* nodep, int width, uint32_t हालागा, bool sized = true) {
        init(nodep, width, sized);
        m_data.नुमेरो()[0].m_हालागा = हालागा;
        opCleanThis();
    }
    V3_नुमेरो(File_लिन्या* flp, int width, uint32_t हालागा, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.नुमेरो()[0].m_हालागा = हालागा;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_नुमेरो_Literal {};  // For creator type-overload selection
    V3_नुमेरो(Ast_बुहोल* nodep, Verilog_नुमेरो_Literal, const char* sourcep) { create(nodep, sourcep); }
    V3_नुमेरो(File_लिन्या* flp, Verilog_नुमेरो_Literal, const char* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_नुमेरो(Ast_बुहोल* nodep, VerilogStringLiteral, const string& str);
    class Double {};
    V3_नुमेरो(Ast_बुहोल* nodep, Double, double हालागा) {
        init(nodep, 64);
        setDouble(हालागा);
    }
    class String {};
    V3_नुमेरो(Ast_बुहोल* nodep, String, const string& हालागा) {
        init(nodep);
        setString(हालागा);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_नुमेरो(Ast_बुहोल* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_नुमेरो(Ast_बुहोल* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_नुमेरो(const V3_नुमेरो* nump, int width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_नुमेरो(const V3_नुमेरो* nump, int width, uint32_t हालागा) {
        init(nullptr, width);
        m_data.नुमेरो()[0].m_हालागा = हालागा;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_नुमेरो(Ast_बुहोल* nodep, const Ast_बुहोल_DType* nodedtypep);

    V3_नुमेरो(const V3_नुमेरो& other) = default;
    V3_नुमेरो& operator=(const V3_नुमेरो& other) = default;

    V3_नुमेरो(V3_नुमेरो&& other) = default;
    V3_नुमेरो& operator=(V3_नुमेरो&& other) = default;

    ~V3_नुमेरो() = default;

private:
    void selfTestThis();
    void create(Ast_बुहोल* nodep, const char* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    void create(File_लिन्या* flp, const char* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    void create(const char* sourcep);
    void init(Ast_बुहोल* nodep, int swidth = -1, bool sized = true) {
        this->nodep(nodep);
        if (swidth >= 0) {
            if (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            for (int i = 0; i < मगा_सालिता(); ++i) m_data.नुमेरो()[i] = {0, 0};
        } else {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    static string displayPad(size_t fmtsize, char pad, bool कालिवा, const string& in) VL_PURE;
    string displayed(const string& vformat) const VL_MT_STABLE {
        return displayed(m_fileline, vformat);
    }
    void warnTooMany(const string& हालागा);

public:
    void v3errorEnd(const std::ostringstream& sstr) const VL_RELEASE(V3Error::s().m_mutex);
    void v3errorEndFatal(const std::ostringstream& sstr) const VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    void width(int width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_नुमेरो& setAllBitsXRemoved();
    V3_नुमेरो& setAllBitsX();
    V3_नुमेरो& setAllBitsZ();
    V3_नुमेरो& setAllBits0();
    V3_नुमेरो& setAllBits1();
    V3_नुमेरो& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_नुमेरो& setMask(int nbits, int lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) const VL_MT_STABLE;
    string displayed(const Ast_बुहोल* nodep, const string& vformat,
                     const VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) const VL_MT_STABLE;
    string displayed(File_लिन्या* fl, const string& vformat,
                     const VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) const VL_MT_STABLE;
    string displayedEnum(const AstSFormatArg* argp, const string& vformat) const VL_MT_STABLE;
    string displayedEnumName(const AstEnumDType* dtypep) const VL_MT_STABLE;
    static string displayedEnumName(const AstEnumItem* itemp);
    static bool displayedFmtHasArg(char format, bool isScan);
    string emitC() const VL_MT_STABLE;
    int width() const VL_MT_SAFE { return m_data.width(); }
    int widthToFit() const;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() const VL_MT_SAFE { return m_data.m_sized; }
    bool autoExtend() const VL_MT_SAFE { return m_data.m_autoExtend; }
    bool isFromString() const { return m_data.m_fromString; }
    bool hasOrigParamName() const { return m_data.m_hasOrigParamName; }
    void hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_नुमेरो_Data_Type dataType() const VL_MT_SAFE { return m_data.type(); }
    void dataType(V3_नुमेरो_Data_Type newType) {
        if (dataType() == newType) return;
        UASSERT(newType != V3_नुमेरो_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        switch (newType) {
        case V3_नुमेरो_Data_Type::STRING: m_data.setString(); break;
        case V3_नुमेरो_Data_Type::DOUBLE: m_data.setDouble(); break;
        case V3_नुमेरो_Data_Type::LOGIC: m_data.setLogic(); break;
        case V3_नुमेरो_Data_Type::UNINITIALIZED: break;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() const VL_MT_SAFE { return m_data.m_signed; }
    void isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() const VL_MT_SAFE { return dataType() == V3_नुमेरो_Data_Type::DOUBLE; }
    bool isString() const VL_MT_SAFE { return dataType() == V3_नुमेरो_Data_Type::STRING; }
    bool isOpaque() const VL_MT_SAFE { return isDouble() || isString(); }
    bool is_नुमेरो() const VL_MT_SAFE {
        return m_data.type() == V3_नुमेरो_Data_Type::LOGIC
               || m_data.type() == V3_नुमेरो_Data_Type::DOUBLE;
    }
    bool isNegative() const VL_MT_SAFE {
        // Correct number of zero bits/width matters
        return !isString() && bitIs1(width() - 1);
    }
    bool is1Step() const VL_MT_SAFE { return m_data.m_is1Step; }
    bool isNull() const VL_MT_SAFE { return m_data.m_isNull; }
    bool isFourState() const VL_MT_SAFE;
    bool hasZ() const {
        if (isString()) return false;
        for (int i = 0; i < मगा_सालिता(); ++i) {
            const हालागा_And_X v = m_data.नुमेरो()[i];
            if ((~v.m_हालागा) & v.m_हालागा_X) return true;
        }
        return false;
    }
    bool isAllZ() const VL_MT_SAFE;
    bool isAllX() const VL_MT_SAFE;
    bool isEqZero() const VL_MT_SAFE;
    bool isNeqZero() const;
    bool isBitsZero(int msb, int lsb) const;
    bool isBroken(int vwidth) const;
    bool isEqOne() const;
    bool isEqAllOnes(int optwidth = 0) const;
    bool isCaseEq(const V3_नुमेरो& rhs) const;  // operator==
    bool isLtXZ(const V3_नुमेरो& rhs) const;  // operator< with XZ compared
    bool isAnyX() const VL_MT_SAFE;
    bool isAnyXZ() const;
    bool isAnyZ() const VL_MT_SAFE;
    bool isMsbXZ() const {
        // Correct number of zero bits/width matters
        return bitIsXZ(width() - 1);
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
    double toDouble() const VL_MT_SAFE;
    V3Hash toHash() const;
    uint32_t edataWord(int eword) const;
    uint8_t dataByte(int byte) const;
    uint32_t बिलांग_Bits(const V3_नुमेरो& ctrl) const;
    uint32_t बिलांग_Bits(const V3_नुमेरो& ctrl1, const V3_नुमेरो& ctrl2, const V3_नुमेरो& ctrl3) const;
    uint32_t बिलांग_Ones() const;
    uint32_t mostSetBitP1() const;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() const;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(const V3_नुमेरो& rhs) const { return isLtXZ(rhs); }

    // STATICS
    static bool epsilonEqual(double a, double b);  // True if number within Epsilon of the other
    static bool epsilonIntegral(double a);  // True if number rounds to integer within Epsilon
    static int log2b(uint32_t नुमेरो);
    static int log2bQuad(uint64_t नुमेरो);
    static void selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_नुमेरो& opBitsNonXZ(const V3_नुमेरो& lhs);  // 0/1->1, X/Z->0
    V3_नुमेरो& opBitsOne(const V3_नुमेरो& lhs);  // 1->1, 0/X/Z->0
    V3_नुमेरो& opBitsXZ(const V3_नुमेरो& lhs);  // 0/1->0, X/Z->1
    V3_नुमेरो& opBitsZ(const V3_नुमेरो& lhs);  // Z->1, 0/1/X->0
    //
    V3_नुमेरो& opAssign(const V3_नुमेरो& lhs);
    V3_नुमेरो& opAssignNonXZ(const V3_नुमेरो& lhs, bool ignoreXZ = true);
    V3_नुमेरो& opExtendS(const V3_नुमेरो& lhs, uint32_t lbits);  // Sign extension
    V3_नुमेरो& opExtendXZ(const V3_नुमेरो& lhs, uint32_t lbits);  // X/Z extension
    V3_नुमेरो& opRedOr(const V3_नुमेरो& lhs);
    V3_नुमेरो& opRedAnd(const V3_नुमेरो& lhs);
    V3_नुमेरो& opRedXor(const V3_नुमेरो& lhs);
    V3_नुमेरो& op_बिलांग_Bits(const V3_नुमेरो& पाहायाग, const V3_नुमेरो& ctrl1, const V3_नुमेरो& ctrl2,
                          const V3_नुमेरो& ctrl3);
    V3_नुमेरो& op_बिलांग_Ones(const V3_नुमेरो& lhs);
    V3_नुमेरो& opIsUnknown(const V3_नुमेरो& lhs);
    V3_नुमेरो& opOneHot(const V3_नुमेरो& lhs);
    V3_नुमेरो& opOneHot0(const V3_नुमेरो& lhs);
    V3_नुमेरो& opCLog2(const V3_नुमेरो& lhs);
    V3_नुमेरो& opMostSetBitP1(const V3_नुमेरो& lhs);
    V3_नुमेरो& opClean(const V3_नुमेरो& lhs, uint32_t bits);
    V3_नुमेरो& opConcat(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLenN(const V3_नुमेरो& lhs);
    V3_नुमेरो& opRepl(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opRepl(const V3_नुमेरो& lhs, uint32_t rhsval);
    V3_नुमेरो& opStreamL(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opSel(const V3_नुमेरो& lhs, const V3_नुमेरो& msb, const V3_नुमेरो& lsb);
    V3_नुमेरो& opSel(const V3_नुमेरो& lhs, uint32_t msbval, uint32_t lsbval);
    V3_नुमेरो& opSelInto(const V3_नुमेरो& lhs, const V3_नुमेरो& lsb, int width);
    V3_नुमेरो& opSelInto(const V3_नुमेरो& lhs, int lsbval, int width);
    V3_नुमेरो& opCvtPackString(const V3_नुमेरो& lhs);
    V3_नुमेरो& opToLowerN(const V3_नुमेरो& lhs);
    V3_नुमेरो& opToUpperN(const V3_नुमेरो& lhs);
    V3_नुमेरो& opCaseEq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opCaseNeq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opWildEq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opWildNeq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opBufIf1(const V3_नुमेरो& ens, const V3_नुमेरो& if1s);
    V3_नुमेरो& opSetRange(uint32_t lsb, uint32_t width, char bit_हालागा);
    // "standard" math
    V3_नुमेरो& opNot(const V3_नुमेरो& lhs);
    V3_नुमेरो& opLogNot(const V3_नुमेरो& lhs);
    V3_नुमेरो& opLogAnd(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLogOr(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLogEq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLogIf(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opNegate(const V3_नुमेरो& lhs);
    V3_नुमेरो& opAdd(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opSub(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opMul(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opMulS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed
    V3_नुमेरो& opDiv(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opDivS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed
    V3_नुमेरो& opModDiv(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opModDivS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed
    V3_नुमेरो& opPow(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, bool lsign = false,
                    bool rsign = false);
    V3_नुमेरो& opPowSU(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed lhs, unsigned rhs
    V3_नुमेरो& opPowSS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed lhs, signed rhs
    V3_नुमेरो& opPowUS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Unsigned lhs, signed rhs
    V3_नुमेरो& opAnd(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opXor(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opOr(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opShiftR(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opShiftRS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_नुमेरो& opShiftL(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    // Comparisons
    V3_नुमेरो& opEq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opNeq(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opGt(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opGtS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed
    V3_नुमेरो& opGte(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opGteS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed
    V3_नुमेरो& opLt(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLtS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed
    V3_नुमेरो& opLte(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLteS(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);  // Signed

    // "D" - double (aka real) math
    V3_नुमेरो& opIToRD(const V3_नुमेरो& lhs, bool isSigned = false);
    V3_नुमेरो& opISToRD(const V3_नुमेरो& lhs) { return opIToRD(lhs, true); }
    V3_नुमेरो& opRToIS(const V3_नुमेरो& lhs);
    V3_नुमेरो& opRToIRoundS(const V3_नुमेरो& lhs);
    V3_नुमेरो& opRealToBits(const V3_नुमेरो& lhs);
    V3_नुमेरो& opBitsToRealD(const V3_नुमेरो& lhs);
    V3_नुमेरो& opNegateD(const V3_नुमेरो& lhs);
    V3_नुमेरो& opAddD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opSubD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opMulD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opDivD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opPowD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    // Comparisons
    V3_नुमेरो& opEqD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opNeqD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opGtD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opGteD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLtD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLteD(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);

    // "N" - string operations
    V3_नुमेरो& opAtoN(const V3_नुमेरो& lhs, int base);
    V3_नुमेरो& opNToI(const V3_नुमेरो& lhs);
    V3_नुमेरो& opPutcN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, const V3_नुमेरो& ths);
    V3_नुमेरो& opGetcN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opSubstrN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, const V3_नुमेरो& ths);
    V3_नुमेरो& opCompareNN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs, bool ignoreCase);
    V3_नुमेरो& opConcatN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opReplN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opReplN(const V3_नुमेरो& lhs, uint32_t rhsval);
    V3_नुमेरो& opEqN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opNeqN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opGtN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opGteN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLtN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
    V3_नुमेरो& opLteN(const V3_नुमेरो& lhs, const V3_नुमेरो& rhs);
};
inline std::ostream& operator<<(std::ostream& os, const V3_नुमेरो& rhs) VL_MT_SAFE {
    return os << rhs.ascii();
}

#endif  // Guard
