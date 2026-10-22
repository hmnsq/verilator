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

#ifndef VERILATOR_V3_સંખ્યા_H
#define VERILATOR_V3_સંખ્યા_H

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Hash.h"
#અન્તર્ભાવ "V3StdFuture.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <array>
#અન્તર્ભાવ <cmath>
#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <vector>

class AstEnumDType;
class AstEnumItem;
class Ast_ગાંઠ;
class Ast_ગાંઠ_DType;
class AstSFormatArg;
class File_લીટી;

//============================================================================

class VFormatAttr final {
જાહેર:
    ગણના en : અક્ષર {
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
    ગણના en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અક્ષર ascii() અચલ { પાછા m_e; }
    bool isComplex() અચલ { પાછા m_e == COMPLEX; }
    bool isDouble() અચલ { પાછા m_e == DOUBLE; }
    bool isEnum() અચલ { પાછા m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() અચલ { પાછા m_e == SIGNED; }
    bool isString() અચલ { પાછા m_e == STRING; }
    bool isUnsigned() અચલ { પાછા m_e == UNSIGNED; }
};
constexpr bool operator==(અચલ VFormatAttr& lhs, અચલ VFormatAttr& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VFormatAttr& lhs, VFormatAttr::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, અચલ VFormatAttr& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VFormatAttr& rhs) {
    પાછા os << rhs.ascii();
}

//============================================================================

class V3_સંખ્યા_Data final {
જાહેર:
    // TYPES
    સંરચના કિંમત_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_કિંમત;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_કિંમત_X;
        bool operator==(અચલ કિંમત_And_X& other) અચલ VL_MT_SAFE {
            પાછા m_કિંમત == other.m_કિંમત && m_કિંમત_X == other.m_કિંમત_X;
        }
    };

    ગણના class V3_સંખ્યા_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, અચલ V3_સંખ્યા_Data_Type& rhs) VL_MT_SAFE {
        પસંદગી (rhs) {
        સ્થિતિ V3_સંખ્યા_Data_Type::UNINITIALIZED: પાછા os << "UNINITIALIZED";
        સ્થિતિ V3_સંખ્યા_Data_Type::LOGIC: પાછા os << "LOGIC";
        સ્થિતિ V3_સંખ્યા_Data_Type::DOUBLE: પાછા os << "DOUBLE";
        સ્થિતિ V3_સંખ્યા_Data_Type::STRING: પાછા os << "STRING";
        }
        પાછા os;
    }

ખાનગી:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    સ્થિર constexpr પૂર્ણાંક INLINE_શબ્દો = std::max(
        static_cast<size_t>(2), std::max(કદ(std::string) / કદ(કિંમત_And_X),
                                         કદ(std::vector<કિંમત_And_X>) / કદ(કિંમત_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    સ્થિર constexpr પૂર્ણાંક MAX_INLINE_WIDTH = INLINE_શબ્દો * કદ(કિંમત_And_X) / 2 * 8;

    // MEMBERS
    સંઘ {
        std::array<કિંમત_And_X, INLINE_શબ્દો> m_inline_સંખ્યા;
        std::vector<કિંમત_And_X> m_dynamic_સંખ્યા;
        std::string m_string;
    };

    પૂર્ણાંક m_width = 0;  // Width (in bits) as specified/calculated
    V3_સંખ્યા_Data_Type m_type;

જાહેર:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_સંખ્યા_Data()
        : m_type{V3_સંખ્યા_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_સંખ્યા_Data() { destroy_Stored_કિંમત(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_સંખ્યા_Data(અચલ V3_સંખ્યા_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        જો (other.is_Inline_સંખ્યા()) {
            init_Inline_સંખ્યા(other.m_inline_સંખ્યા);
        } નહીંતર જો (other.is_Dynamic_સંખ્યા()) {
            init_Dynamic_સંખ્યા(other.m_dynamic_સંખ્યા);
        } નહીંતર જો (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_સંખ્યા_Data& operator=(અચલ V3_સંખ્યા_Data& other) {
        જો (other.is_Inline_સંખ્યા()) {
            destroy_Stored_કિંમત();
            init_Inline_સંખ્યા(other.m_inline_સંખ્યા);
        } નહીંતર જો (other.is_Dynamic_સંખ્યા()) {
            reinit_With_Or_Assign_Dynamic_સંખ્યા(other.m_dynamic_સંખ્યા);
        } નહીંતર જો (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } નહીંતર {
            destroy_Stored_કિંમત();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        પાછા *this;
    }

    V3_સંખ્યા_Data(V3_સંખ્યા_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        જો (other.is_Inline_સંખ્યા()) {
            init_Inline_સંખ્યા(other.m_inline_સંખ્યા);
        } નહીંતર જો (other.is_Dynamic_સંખ્યા()) {
            init_Dynamic_સંખ્યા(std::move(other.m_dynamic_સંખ્યા));
        } નહીંતર જો (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_સંખ્યા_Data_Type::UNINITIALIZED;
    }

    V3_સંખ્યા_Data& operator=(V3_સંખ્યા_Data&& other) {
        જો (other.is_Inline_સંખ્યા()) {
            destroy_Stored_કિંમત();
            init_Inline_સંખ્યા(other.m_inline_સંખ્યા);
        } નહીંતર જો (other.is_Dynamic_સંખ્યા()) {
            reinit_With_Or_Assign_Dynamic_સંખ્યા(std::move(other.m_dynamic_સંખ્યા));
        } નહીંતર જો (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } નહીંતર {
            destroy_Stored_કિંમત();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_સંખ્યા_Data_Type::UNINITIALIZED;
        પાછા *this;
    }

    // ACCESSORS
    કિંમત_And_X* અંક() {
        UASSERT(is_સંખ્યા(), "`num` member accessed when data type is " << m_type);
        પાછા is_Inline_સંખ્યા() ? m_inline_સંખ્યા.data() : m_dynamic_સંખ્યા.data();
    }
    અચલ કિંમત_And_X* અંક() અચલ VL_MT_SAFE {
        UASSERT(is_સંખ્યા(), "`num` member accessed when data type is " << m_type);
        પાછા is_Inline_સંખ્યા() ? m_inline_સંખ્યા.data() : m_dynamic_સંખ્યા.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        પાછા m_string;
    }
    અચલ std::string& str() અચલ VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        પાછા m_string;
    }

    પૂર્ણાંક width() અચલ VL_MT_SAFE { પાછા m_width; }
    V3_સંખ્યા_Data_Type type() અચલ VL_MT_SAFE { પાછા m_type; }

    // METHODS
    શૂન્ય resize(પૂર્ણાંક bits_ગણતરી) {
        જો (m_width == bits_ગણતરી) પાછા;
        જો (bits_To_શબ્દો(m_width) == bits_To_શબ્દો(bits_ગણતરી)) {
            m_width = bits_ગણતરી;
            પાછા;
        }
        જો (is_Dynamic_સંખ્યા()) {
            જો (bits_ગણતરી > MAX_INLINE_WIDTH) {
                m_dynamic_સંખ્યા.resize(bits_To_શબ્દો(bits_ગણતરી));
            } નહીંતર {
                અચલ auto dynamicBits = std::move(m_dynamic_સંખ્યા);
                destroy_Dynamic_સંખ્યા();
                init_Inline_સંખ્યા();
                std::memcpy(m_inline_સંખ્યા.data(), dynamicBits.data(), કદ(m_inline_સંખ્યા));
            }
        } નહીંતર જો (is_Inline_સંખ્યા()) {
            જો (bits_ગણતરી > MAX_INLINE_WIDTH) {
                અચલ auto bits = m_inline_સંખ્યા;
                init_Dynamic_સંખ્યા(bits_To_શબ્દો(bits_ગણતરી));
                std::memcpy(m_dynamic_સંખ્યા.data(), bits.data(), કદ(bits));
            }
        }
        m_width = bits_ગણતરી;
    }

    શૂન્ય setString() {
        // If there has been a string already it is kept intact.
        જો (isString()) પાછા;
        જો (is_Dynamic_સંખ્યા()) destroy_Dynamic_સંખ્યા();
        initString();
        m_type = V3_સંખ્યા_Data_Type::STRING;
    }
    શૂન્ય setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_સંખ્યા_Data_Type::STRING;
    }
    શૂન્ય setString(અચલ std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_સંખ્યા_Data_Type::STRING;
    }

    શૂન્ય setDouble() {
        destroy_Stored_કિંમત();
        જો (!is_Inline_સંખ્યા()) init_Inline_સંખ્યા();
        m_type = V3_સંખ્યા_Data_Type::DOUBLE;
        resize(64);
    }

    શૂન્ય setLogic() {
        જો (isString()) destroyString();
        જો (!is_સંખ્યા()) {
            જો (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_સંખ્યા();
            } નહીંતર {
                init_Dynamic_સંખ્યા(bits_To_શબ્દો(m_width));
            }
        }
        m_type = V3_સંખ્યા_Data_Type::LOGIC;
        resize(m_width);
    }

ખાનગી:
    સ્થિર constexpr પૂર્ણાંક bits_To_શબ્દો(પૂર્ણાંક bits_ગણતરી) VL_PURE { પાછા (bits_ગણતરી + 31) / 32; }

    bool is_સંખ્યા() અચલ VL_MT_SAFE {
        પાછા m_type == V3_સંખ્યા_Data_Type::DOUBLE || m_type == V3_સંખ્યા_Data_Type::LOGIC;
    }
    bool is_Inline_સંખ્યા() અચલ VL_MT_SAFE {
        પાછા (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_સંખ્યા_Data_Type::DOUBLE || m_type == V3_સંખ્યા_Data_Type::LOGIC);
    }
    bool is_Dynamic_સંખ્યા() અચલ VL_MT_SAFE {
        પાછા (m_width > MAX_INLINE_WIDTH) && (m_type == V3_સંખ્યા_Data_Type::LOGIC);
    }
    bool isString() અચલ VL_MT_SAFE { પાછા m_type == V3_સંખ્યા_Data_Type::STRING; }

    template <typename... Args>
    શૂન્ય init_Inline_સંખ્યા(Args&&... args) {
        new (&m_inline_સંખ્યા) std::array<કિંમત_And_X, INLINE_શબ્દો>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    શૂન્ય init_Dynamic_સંખ્યા(Args&&... args) {
        new (&m_dynamic_સંખ્યા) std::vector<કિંમત_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    શૂન્ય initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    શૂન્ય destroy_Dynamic_સંખ્યા() { m_dynamic_સંખ્યા.~vector(); }
    શૂન્ય destroyString() { m_string.~string(); }
    શૂન્ય destroy_Stored_કિંમત() {
        જો (isString())
            destroyString();
        નહીંતર જો (is_Dynamic_સંખ્યા())
            destroy_Dynamic_સંખ્યા();
    }

    template <typename T>
    શૂન્ય reinit_With_Or_Assign_Dynamic_સંખ્યા(T&& s) {
        જો (is_Dynamic_સંખ્યા()) {
            m_dynamic_સંખ્યા = std::forward<T>(s);
            પાછા;
        }
        જો (isString()) destroyString();
        init_Dynamic_સંખ્યા(std::forward<T>(s));
    }
    template <typename T>
    શૂન્ય reinitWithOrAssignString(T&& s) {
        જો (isString()) {
            m_string = std::forward<T>(s);
            પાછા;
        }
        જો (is_Dynamic_સંખ્યા()) destroy_Dynamic_સંખ્યા();
        initString(std::forward<T>(s));
    }
};

class V3_સંખ્યા final {
    // TYPES
    using કિંમત_And_X = V3_સંખ્યા_Data::કિંમત_And_X;
    using V3_સંખ્યા_Data_Type = V3_સંખ્યા_Data::V3_સંખ્યા_Data_Type;

    // MEMBERS
    V3_સંખ્યા_Data m_data;
    Ast_ગાંઠ* m_nodep = nullptr;  // Parent node - for error reporting only
    File_લીટી* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    શૂન્ય setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_સંખ્યા& setSingleBits(અક્ષર કિંમત);
    શૂન્ય fillBits(પૂર્ણાંક destLsb, પૂર્ણાંક width, અક્ષર કિંમત);
    શૂન્ય copyBits(પૂર્ણાંક destLsb, અચલ V3_સંખ્યા& source, પૂર્ણાંક sourceLsb, પૂર્ણાંક width);
    V3_સંખ્યા& setString(અચલ string& str) {
        m_data.setString(str);
        પાછા *this;
    }
    શૂન્ય opCleanThis(bool warnOnTruncation = false);

જાહેર:
    શૂન્ય nodep(Ast_ગાંઠ* nodep) VL_MT_STABLE;
    Ast_ગાંઠ* nodep() અચલ VL_MT_SAFE { પાછા m_nodep; }  // For debug only
    File_લીટી* fileline() અચલ VL_MT_SAFE { પાછા m_fileline; }
    V3_સંખ્યા& setZero();
    V3_સંખ્યા& setQuad(uint64_t કિંમત);
    V3_સંખ્યા& setLong(uint32_t કિંમત);
    V3_સંખ્યા& setLongS(int32_t કિંમત);
    V3_સંખ્યા& setDouble(દ્વિગુણ કિંમત);
    શૂન્ય setBitX0(પૂર્ણાંક bit);
    શૂન્ય setBit(પૂર્ણાંક bit, અક્ષર કિંમત) {  // Note: must be initialized as number and pre-zeroed!
        જો (bit >= m_data.width()) પાછા;
        અચલ uint32_t mask = (1UL << (bit & 31));
        કિંમત_And_X& v = m_data.અંક()[bit / 32];
        જો (કિંમત == '0' || કિંમત == 0) {
            v.m_કિંમત &= ~mask;
            v.m_કિંમત_X &= ~mask;
        } નહીંતર જો (કિંમત == '1' || કિંમત == 1) {
            v.m_કિંમત |= mask;
            v.m_કિંમત_X &= ~mask;
        } નહીંતર જો (કિંમત == 'z' || કિંમત == 2) {
            v.m_કિંમત &= ~mask;
            v.m_કિંમત_X |= mask;
        } નહીંતર {  // X
            v.m_કિંમત |= mask;
            v.m_કિંમત_X |= mask;
        }
    }

ખાનગી:
    અક્ષર bitIs(પૂર્ણાંક bit) અચલ {
        જો (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            પાછા '0';
        }
        અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
        પાછા ("01zx"[(((v.m_કિંમત & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_કિંમત_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    અક્ષર bitIsExtend(પૂર્ણાંક bit, પૂર્ણાંક lbits) અચલ {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        જો (bit < 0) પાછા '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        જો (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
            // We do sign extend
            પાછા ("01zx"[(((v.m_કિંમત & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_કિંમત_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
        પાછા ("01zx"[(((v.m_કિંમત & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_કિંમત_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

જાહેર:
    bool bitIs0(પૂર્ણાંક bit) અચલ VL_MT_SAFE {
        જો (!is_સંખ્યા()) પાછા false;
        જો (bit < 0) પાછા false;
        જો (bit >= m_data.width()) પાછા !bitIsXZ(m_data.width() - 1);
        અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
        પાછા ((v.m_કિંમત | v.m_કિંમત_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(પૂર્ણાંક bit) અચલ VL_MT_SAFE {
        જો (!is_સંખ્યા()) પાછા false;
        જો (bit < 0) પાછા false;
        જો (bit >= m_data.width()) પાછા false;
        અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
        પાછા ((v.m_કિંમત & (1UL << (bit & 31))) && !(v.m_કિંમત_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(પૂર્ણાંક bit) અચલ {
        // Correct number of zero bits/width matters
        જો (!is_સંખ્યા()) પાછા false;
        જો (bit < 0) પાછા false;
        જો (bit >= m_data.width()) પાછા bitIs1Extend(m_data.width() - 1);
        અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
        પાછા ((v.m_કિંમત & (1UL << (bit & 31))) && !(v.m_કિંમત_X & (1UL << (bit & 31))));
    }
    bool bitIsX(પૂર્ણાંક bit) અચલ VL_MT_SAFE {
        જો (!is_સંખ્યા()) પાછા false;
        જો (bit < 0) પાછા false;
        જો (bit >= m_data.width()) પાછા bitIsZ(m_data.width() - 1);
        અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
        પાછા ((v.m_કિંમત & (1UL << (bit & 31))) && (v.m_કિંમત_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(પૂર્ણાંક bit) અચલ VL_MT_SAFE {
        જો (!is_સંખ્યા()) પાછા false;
        જો (bit < 0) પાછા false;
        જો (bit >= m_data.width()) પાછા bitIsXZ(m_data.width() - 1);
        અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
        પાછા ((v.m_કિંમત_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(પૂર્ણાંક bit) અચલ VL_MT_SAFE {
        જો (!is_સંખ્યા()) પાછા false;
        જો (bit < 0) પાછા false;
        જો (bit >= m_data.width()) પાછા bitIsZ(m_data.width() - 1);
        અચલ કિંમત_And_X v = m_data.અંક()[bit / 32];
        પાછા ((~v.m_કિંમત & (1UL << (bit & 31))) && (v.m_કિંમત_X & (1UL << (bit & 31))));
    }

ખાનગી:
    uint32_t bits_કિંમત(પૂર્ણાંક lsb, પૂર્ણાંક nbits) અચલ VL_MT_SAFE {
        uint32_t v = 0;
        માટે (પૂર્ણાંક bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        પાછા v;
    }

    પૂર્ણાંક ગણતરી_X(પૂર્ણાંક lsb, પૂર્ણાંક nbits) અચલ VL_MT_SAFE;
    પૂર્ણાંક ગણતરી_Z(પૂર્ણાંક lsb, પૂર્ણાંક nbits) અચલ VL_MT_SAFE;

    પૂર્ણાંક શબ્દો() અચલ VL_MT_SAFE { પાછા ((width() + 31) / 32); }
    uint32_t hiWordMask() અચલ VL_MT_SAFE {
        // Correct number of zero bits/width matters
        પાછા VL_MASK_I(width());
    }

    V3_સંખ્યા& opModDivGuts(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, bool is_modulus);

જાહેર:
    // CONSTRUCTORS
    explicit V3_સંખ્યા(Ast_ગાંઠ* nodep) { init(nodep, 1); }
    V3_સંખ્યા(Ast_ગાંઠ* nodep, પૂર્ણાંક width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_સંખ્યા(Ast_ગાંઠ* nodep, પૂર્ણાંક width, અચલ V3_સંખ્યા& કિંમત) {
        init(nodep, width, width > 0);
        opAssign(કિંમત);
    }
    V3_સંખ્યા(Ast_ગાંઠ* nodep, પૂર્ણાંક width, uint32_t કિંમત, bool sized = true) {
        init(nodep, width, sized);
        m_data.અંક()[0].m_કિંમત = કિંમત;
        opCleanThis();
    }
    V3_સંખ્યા(File_લીટી* flp, પૂર્ણાંક width, uint32_t કિંમત, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.અંક()[0].m_કિંમત = કિંમત;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_સંખ્યા_Literal {};  // For creator type-overload selection
    V3_સંખ્યા(Ast_ગાંઠ* nodep, Verilog_સંખ્યા_Literal, અચલ અક્ષર* sourcep) { create(nodep, sourcep); }
    V3_સંખ્યા(File_લીટી* flp, Verilog_સંખ્યા_Literal, અચલ અક્ષર* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_સંખ્યા(Ast_ગાંઠ* nodep, VerilogStringLiteral, અચલ string& str);
    class Double {};
    V3_સંખ્યા(Ast_ગાંઠ* nodep, Double, દ્વિગુણ કિંમત) {
        init(nodep, 64);
        setDouble(કિંમત);
    }
    class String {};
    V3_સંખ્યા(Ast_ગાંઠ* nodep, String, અચલ string& કિંમત) {
        init(nodep);
        setString(કિંમત);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_સંખ્યા(Ast_ગાંઠ* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_સંખ્યા(Ast_ગાંઠ* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_સંખ્યા(અચલ V3_સંખ્યા* nump, પૂર્ણાંક width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_સંખ્યા(અચલ V3_સંખ્યા* nump, પૂર્ણાંક width, uint32_t કિંમત) {
        init(nullptr, width);
        m_data.અંક()[0].m_કિંમત = કિંમત;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_સંખ્યા(Ast_ગાંઠ* nodep, અચલ Ast_ગાંઠ_DType* nodedtypep);

    V3_સંખ્યા(અચલ V3_સંખ્યા& other) = મૂળભૂત;
    V3_સંખ્યા& operator=(અચલ V3_સંખ્યા& other) = મૂળભૂત;

    V3_સંખ્યા(V3_સંખ્યા&& other) = મૂળભૂત;
    V3_સંખ્યા& operator=(V3_સંખ્યા&& other) = મૂળભૂત;

    ~V3_સંખ્યા() = મૂળભૂત;

ખાનગી:
    શૂન્ય selfTestThis();
    શૂન્ય create(Ast_ગાંઠ* nodep, અચલ અક્ષર* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    શૂન્ય create(File_લીટી* flp, અચલ અક્ષર* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    શૂન્ય create(અચલ અક્ષર* sourcep);
    શૂન્ય init(Ast_ગાંઠ* nodep, પૂર્ણાંક swidth = -1, bool sized = true) {
        this->nodep(nodep);
        જો (swidth >= 0) {
            જો (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) m_data.અંક()[i] = {0, 0};
        } નહીંતર {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    સ્થિર string displayPad(size_t fmtsize, અક્ષર pad, bool ડાબું, અચલ string& in) VL_PURE;
    string displayed(અચલ string& vformat) અચલ VL_MT_STABLE {
        પાછા displayed(m_fileline, vformat);
    }
    શૂન્ય warnTooMany(અચલ string& કિંમત);

જાહેર:
    શૂન્ય v3errorEnd(અચલ std::ostringstream& sstr) અચલ VL_RELEASE(V3Error::s().m_mutex);
    શૂન્ય v3errorEndFatal(અચલ std::ostringstream& sstr) અચલ VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    શૂન્ય width(પૂર્ણાંક width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_સંખ્યા& setAllBitsXRemoved();
    V3_સંખ્યા& setAllBitsX();
    V3_સંખ્યા& setAllBitsZ();
    V3_સંખ્યા& setAllBits0();
    V3_સંખ્યા& setAllBits1();
    V3_સંખ્યા& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_સંખ્યા& setMask(પૂર્ણાંક nbits, પૂર્ણાંક lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) અચલ VL_MT_STABLE;
    string displayed(અચલ Ast_ગાંઠ* nodep, અચલ string& vformat,
                     અચલ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) અચલ VL_MT_STABLE;
    string displayed(File_લીટી* fl, અચલ string& vformat,
                     અચલ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) અચલ VL_MT_STABLE;
    string displayedEnum(અચલ AstSFormatArg* argp, અચલ string& vformat) અચલ VL_MT_STABLE;
    string displayedEnumName(અચલ AstEnumDType* dtypep) અચલ VL_MT_STABLE;
    સ્થિર string displayedEnumName(અચલ AstEnumItem* itemp);
    સ્થિર bool displayedFmtHasArg(અક્ષર format, bool isScan);
    string emitC() અચલ VL_MT_STABLE;
    પૂર્ણાંક width() અચલ VL_MT_SAFE { પાછા m_data.width(); }
    પૂર્ણાંક widthToFit() અચલ;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() અચલ VL_MT_SAFE { પાછા m_data.m_sized; }
    bool autoExtend() અચલ VL_MT_SAFE { પાછા m_data.m_autoExtend; }
    bool isFromString() અચલ { પાછા m_data.m_fromString; }
    bool hasOrigParamName() અચલ { પાછા m_data.m_hasOrigParamName; }
    શૂન્ય hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_સંખ્યા_Data_Type dataType() અચલ VL_MT_SAFE { પાછા m_data.type(); }
    શૂન્ય dataType(V3_સંખ્યા_Data_Type newType) {
        જો (dataType() == newType) પાછા;
        UASSERT(newType != V3_સંખ્યા_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        પસંદગી (newType) {
        સ્થિતિ V3_સંખ્યા_Data_Type::STRING: m_data.setString(); તોડો;
        સ્થિતિ V3_સંખ્યા_Data_Type::DOUBLE: m_data.setDouble(); તોડો;
        સ્થિતિ V3_સંખ્યા_Data_Type::LOGIC: m_data.setLogic(); તોડો;
        સ્થિતિ V3_સંખ્યા_Data_Type::UNINITIALIZED: તોડો;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() અચલ VL_MT_SAFE { પાછા m_data.m_signed; }
    શૂન્ય isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() અચલ VL_MT_SAFE { પાછા dataType() == V3_સંખ્યા_Data_Type::DOUBLE; }
    bool isString() અચલ VL_MT_SAFE { પાછા dataType() == V3_સંખ્યા_Data_Type::STRING; }
    bool isOpaque() અચલ VL_MT_SAFE { પાછા isDouble() || isString(); }
    bool is_સંખ્યા() અચલ VL_MT_SAFE {
        પાછા m_data.type() == V3_સંખ્યા_Data_Type::LOGIC
               || m_data.type() == V3_સંખ્યા_Data_Type::DOUBLE;
    }
    bool isNegative() અચલ VL_MT_SAFE {
        // Correct number of zero bits/width matters
        પાછા !isString() && bitIs1(width() - 1);
    }
    bool is1Step() અચલ VL_MT_SAFE { પાછા m_data.m_is1Step; }
    bool isNull() અચલ VL_MT_SAFE { પાછા m_data.m_isNull; }
    bool isFourState() અચલ VL_MT_SAFE;
    bool hasZ() અચલ {
        જો (isString()) પાછા false;
        માટે (પૂર્ણાંક i = 0; i < શબ્દો(); ++i) {
            અચલ કિંમત_And_X v = m_data.અંક()[i];
            જો ((~v.m_કિંમત) & v.m_કિંમત_X) પાછા true;
        }
        પાછા false;
    }
    bool isAllZ() અચલ VL_MT_SAFE;
    bool isAllX() અચલ VL_MT_SAFE;
    bool isEqZero() અચલ VL_MT_SAFE;
    bool isNeqZero() અચલ;
    bool isBitsZero(પૂર્ણાંક msb, પૂર્ણાંક lsb) અચલ;
    bool isBroken(પૂર્ણાંક vwidth) અચલ;
    bool isEqOne() અચલ;
    bool isEqAllOnes(પૂર્ણાંક optwidth = 0) અચલ;
    bool isCaseEq(અચલ V3_સંખ્યા& rhs) અચલ;  // operator==
    bool isLtXZ(અચલ V3_સંખ્યા& rhs) અચલ;  // operator< with XZ compared
    bool isAnyX() અચલ VL_MT_SAFE;
    bool isAnyXZ() અચલ;
    bool isAnyZ() અચલ VL_MT_SAFE;
    bool isMsbXZ() અચલ {
        // Correct number of zero bits/width matters
        પાછા bitIsXZ(width() - 1);
    }
    bool fitsInUInt() અચલ VL_MT_SAFE;
    uint32_t toUInt() અચલ VL_MT_SAFE;
    int32_t toSInt() અચલ VL_MT_SAFE;
    uint64_t toUQuad() અચલ VL_MT_SAFE;
    int64_t toSQuad() અચલ VL_MT_SAFE;
    string toString() અચલ VL_MT_SAFE;
    string toDecimalS() અચલ VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() અચલ VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() અચલ VL_MT_STABLE;
    દ્વિગુણ toDouble() અચલ VL_MT_SAFE;
    V3Hash toHash() અચલ;
    uint32_t edataWord(પૂર્ણાંક eword) અચલ;
    uint8_t dataByte(પૂર્ણાંક byte) અચલ;
    uint32_t ગણતરી_Bits(અચલ V3_સંખ્યા& ctrl) અચલ;
    uint32_t ગણતરી_Bits(અચલ V3_સંખ્યા& ctrl1, અચલ V3_સંખ્યા& ctrl2, અચલ V3_સંખ્યા& ctrl3) અચલ;
    uint32_t ગણતરી_Ones() અચલ;
    uint32_t mostSetBitP1() અચલ;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() અચલ;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(અચલ V3_સંખ્યા& rhs) અચલ { પાછા isLtXZ(rhs); }

    // STATICS
    સ્થિર bool epsilonEqual(દ્વિગુણ a, દ્વિગુણ b);  // True if number within Epsilon of the other
    સ્થિર bool epsilonIntegral(દ્વિગુણ a);  // True if number rounds to integer within Epsilon
    સ્થિર પૂર્ણાંક log2b(uint32_t અંક);
    સ્થિર પૂર્ણાંક log2bQuad(uint64_t અંક);
    સ્થિર શૂન્ય selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_સંખ્યા& opBitsNonXZ(અચલ V3_સંખ્યા& lhs);  // 0/1->1, X/Z->0
    V3_સંખ્યા& opBitsOne(અચલ V3_સંખ્યા& lhs);  // 1->1, 0/X/Z->0
    V3_સંખ્યા& opBitsXZ(અચલ V3_સંખ્યા& lhs);  // 0/1->0, X/Z->1
    V3_સંખ્યા& opBitsZ(અચલ V3_સંખ્યા& lhs);  // Z->1, 0/1/X->0
    //
    V3_સંખ્યા& opAssign(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opAssignNonXZ(અચલ V3_સંખ્યા& lhs, bool ignoreXZ = true);
    V3_સંખ્યા& opExtendS(અચલ V3_સંખ્યા& lhs, uint32_t lbits);  // Sign extension
    V3_સંખ્યા& opExtendXZ(અચલ V3_સંખ્યા& lhs, uint32_t lbits);  // X/Z extension
    V3_સંખ્યા& opRedOr(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opRedAnd(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opRedXor(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& op_ગણતરી_Bits(અચલ V3_સંખ્યા& પદાવલિ, અચલ V3_સંખ્યા& ctrl1, અચલ V3_સંખ્યા& ctrl2,
                          અચલ V3_સંખ્યા& ctrl3);
    V3_સંખ્યા& op_ગણતરી_Ones(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opIsUnknown(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opOneHot(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opOneHot0(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opCLog2(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opMostSetBitP1(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opClean(અચલ V3_સંખ્યા& lhs, uint32_t bits);
    V3_સંખ્યા& opConcat(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLenN(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opRepl(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opRepl(અચલ V3_સંખ્યા& lhs, uint32_t rhsval);
    V3_સંખ્યા& opStreamL(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opSel(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& msb, અચલ V3_સંખ્યા& lsb);
    V3_સંખ્યા& opSel(અચલ V3_સંખ્યા& lhs, uint32_t msbval, uint32_t lsbval);
    V3_સંખ્યા& opSelInto(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& lsb, પૂર્ણાંક width);
    V3_સંખ્યા& opSelInto(અચલ V3_સંખ્યા& lhs, પૂર્ણાંક lsbval, પૂર્ણાંક width);
    V3_સંખ્યા& opCvtPackString(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opToLowerN(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opToUpperN(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opCaseEq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opCaseNeq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opWildEq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opWildNeq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opBufIf1(અચલ V3_સંખ્યા& ens, અચલ V3_સંખ્યા& if1s);
    V3_સંખ્યા& opSetRange(uint32_t lsb, uint32_t width, અક્ષર bit_કિંમત);
    // "standard" math
    V3_સંખ્યા& opNot(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opLogNot(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opLogAnd(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLogOr(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLogEq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLogIf(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opNegate(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opAdd(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opSub(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opMul(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opMulS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed
    V3_સંખ્યા& opDiv(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opDivS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed
    V3_સંખ્યા& opModDiv(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opModDivS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed
    V3_સંખ્યા& opPow(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, bool lsign = false,
                    bool rsign = false);
    V3_સંખ્યા& opPowSU(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed lhs, unsigned rhs
    V3_સંખ્યા& opPowSS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed lhs, signed rhs
    V3_સંખ્યા& opPowUS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Unsigned lhs, signed rhs
    V3_સંખ્યા& opAnd(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opXor(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opOr(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opShiftR(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opShiftRS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_સંખ્યા& opShiftL(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    // Comparisons
    V3_સંખ્યા& opEq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opNeq(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opGt(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opGtS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed
    V3_સંખ્યા& opGte(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opGteS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed
    V3_સંખ્યા& opLt(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLtS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed
    V3_સંખ્યા& opLte(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLteS(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);  // Signed

    // "D" - double (aka real) math
    V3_સંખ્યા& opIToRD(અચલ V3_સંખ્યા& lhs, bool isSigned = false);
    V3_સંખ્યા& opISToRD(અચલ V3_સંખ્યા& lhs) { પાછા opIToRD(lhs, true); }
    V3_સંખ્યા& opRToIS(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opRToIRoundS(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opRealToBits(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opBitsToRealD(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opNegateD(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opAddD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opSubD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opMulD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opDivD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opPowD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    // Comparisons
    V3_સંખ્યા& opEqD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opNeqD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opGtD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opGteD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLtD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLteD(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);

    // "N" - string operations
    V3_સંખ્યા& opAtoN(અચલ V3_સંખ્યા& lhs, પૂર્ણાંક base);
    V3_સંખ્યા& opNToI(અચલ V3_સંખ્યા& lhs);
    V3_સંખ્યા& opPutcN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, અચલ V3_સંખ્યા& ths);
    V3_સંખ્યા& opGetcN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opSubstrN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, અચલ V3_સંખ્યા& ths);
    V3_સંખ્યા& opCompareNN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs, bool ignoreCase);
    V3_સંખ્યા& opConcatN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opReplN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opReplN(અચલ V3_સંખ્યા& lhs, uint32_t rhsval);
    V3_સંખ્યા& opEqN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opNeqN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opGtN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opGteN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLtN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
    V3_સંખ્યા& opLteN(અચલ V3_સંખ્યા& lhs, અચલ V3_સંખ્યા& rhs);
};
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ V3_સંખ્યા& rhs) VL_MT_SAFE {
    પાછા os << rhs.ascii();
}

#endif  // Guard
