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

#ifndef VERILATOR_V3_সংখ্যা_H
#define VERILATOR_V3_সংখ্যা_H

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Hash.h"
#অন্তর্ভাব "V3StdFuture.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <array>
#অন্তর্ভাব <cmath>
#অন্তর্ভাব <limits>
#অন্তর্ভাব <vector>

class AstEnumDType;
class AstEnumItem;
class Ast_গ্রন্থি;
class Ast_গ্রন্থি_DType;
class AstSFormatArg;
class File_সারি;

//============================================================================

class VFormatAttr final {
সর্বজনীন:
    গণনা en : অক্ষর {
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
    গণনা en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(পূর্ণসংখ্যা _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুবক { ফেরত m_e; }
    অক্ষর ascii() ধ্রুবক { ফেরত m_e; }
    bool isComplex() ধ্রুবক { ফেরত m_e == COMPLEX; }
    bool isDouble() ধ্রুবক { ফেরত m_e == DOUBLE; }
    bool isEnum() ধ্রুবক { ফেরত m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ধ্রুবক { ফেরত m_e == SIGNED; }
    bool isString() ধ্রুবক { ফেরত m_e == STRING; }
    bool isUnsigned() ধ্রুবক { ফেরত m_e == UNSIGNED; }
};
constexpr bool operator==(ধ্রুবক VFormatAttr& lhs, ধ্রুবক VFormatAttr& rhs) {
    ফেরত lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুবক VFormatAttr& lhs, VFormatAttr::en rhs) { ফেরত lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, ধ্রুবক VFormatAttr& rhs) { ফেরত lhs == rhs.m_e; }
অন্তঃরেখ std::ostream& operator<<(std::ostream& os, ধ্রুবক VFormatAttr& rhs) {
    ফেরত os << rhs.ascii();
}

//============================================================================

class V3_সংখ্যা_তথ্য final {
সর্বজনীন:
    // TYPES
    কাঠামো মান_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_মান;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_মান_X;
        bool operator==(ধ্রুবক মান_And_X& other) ধ্রুবক VL_MT_SAFE {
            ফেরত m_মান == other.m_মান && m_মান_X == other.m_মান_X;
        }
    };

    গণনা class V3_সংখ্যা_তথ্য_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ধ্রুবক V3_সংখ্যা_তথ্য_Type& rhs) VL_MT_SAFE {
        নির্বাচন (rhs) {
        ক্ষেত্র V3_সংখ্যা_তথ্য_Type::UNINITIALIZED: ফেরত os << "UNINITIALIZED";
        ক্ষেত্র V3_সংখ্যা_তথ্য_Type::LOGIC: ফেরত os << "LOGIC";
        ক্ষেত্র V3_সংখ্যা_তথ্য_Type::DOUBLE: ফেরত os << "DOUBLE";
        ক্ষেত্র V3_সংখ্যা_তথ্য_Type::STRING: ফেরত os << "STRING";
        }
        ফেরত os;
    }

ব্যক্তিগত:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    স্থির constexpr পূর্ণসংখ্যা INLINE_শব্দগুলি = std::max(
        static_cast<আকার_t>(2), std::max(আকার(std::string) / আকার(মান_And_X),
                                         আকার(std::vector<মান_And_X>) / আকার(মান_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    স্থির constexpr পূর্ণসংখ্যা MAX_INLINE_WIDTH = INLINE_শব্দগুলি * আকার(মান_And_X) / 2 * 8;

    // MEMBERS
    সংঘ {
        std::array<মান_And_X, INLINE_শব্দগুলি> m_inline_সংখ্যা;
        std::vector<মান_And_X> m_dynamic_সংখ্যা;
        std::string m_string;
    };

    পূর্ণসংখ্যা m_width = 0;  // Width (in bits) as specified/calculated
    V3_সংখ্যা_তথ্য_Type m_type;

সর্বজনীন:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_সংখ্যা_তথ্য()
        : m_type{V3_সংখ্যা_তথ্য_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_সংখ্যা_তথ্য() { destroy_Stored_মান(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_সংখ্যা_তথ্য(ধ্রুবক V3_সংখ্যা_তথ্য& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        যদি (other.is_Inline_সংখ্যা()) {
            init_Inline_সংখ্যা(other.m_inline_সংখ্যা);
        } অন্যথা যদি (other.is_Dynamic_সংখ্যা()) {
            init_Dynamic_সংখ্যা(other.m_dynamic_সংখ্যা);
        } অন্যথা যদি (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_সংখ্যা_তথ্য& operator=(ধ্রুবক V3_সংখ্যা_তথ্য& other) {
        যদি (other.is_Inline_সংখ্যা()) {
            destroy_Stored_মান();
            init_Inline_সংখ্যা(other.m_inline_সংখ্যা);
        } অন্যথা যদি (other.is_Dynamic_সংখ্যা()) {
            reinit_With_Or_Assign_Dynamic_সংখ্যা(other.m_dynamic_সংখ্যা);
        } অন্যথা যদি (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } অন্যথা {
            destroy_Stored_মান();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        ফেরত *this;
    }

    V3_সংখ্যা_তথ্য(V3_সংখ্যা_তথ্য&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        যদি (other.is_Inline_সংখ্যা()) {
            init_Inline_সংখ্যা(other.m_inline_সংখ্যা);
        } অন্যথা যদি (other.is_Dynamic_সংখ্যা()) {
            init_Dynamic_সংখ্যা(std::move(other.m_dynamic_সংখ্যা));
        } অন্যথা যদি (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_সংখ্যা_তথ্য_Type::UNINITIALIZED;
    }

    V3_সংখ্যা_তথ্য& operator=(V3_সংখ্যা_তথ্য&& other) {
        যদি (other.is_Inline_সংখ্যা()) {
            destroy_Stored_মান();
            init_Inline_সংখ্যা(other.m_inline_সংখ্যা);
        } অন্যথা যদি (other.is_Dynamic_সংখ্যা()) {
            reinit_With_Or_Assign_Dynamic_সংখ্যা(std::move(other.m_dynamic_সংখ্যা));
        } অন্যথা যদি (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } অন্যথা {
            destroy_Stored_মান();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_সংখ্যা_তথ্য_Type::UNINITIALIZED;
        ফেরত *this;
    }

    // ACCESSORS
    মান_And_X* অঙ্ক() {
        UASSERT(is_সংখ্যা(), "`num` member accessed when data type is " << m_type);
        ফেরত is_Inline_সংখ্যা() ? m_inline_সংখ্যা.তথ্য() : m_dynamic_সংখ্যা.তথ্য();
    }
    ধ্রুবক মান_And_X* অঙ্ক() ধ্রুবক VL_MT_SAFE {
        UASSERT(is_সংখ্যা(), "`num` member accessed when data type is " << m_type);
        ফেরত is_Inline_সংখ্যা() ? m_inline_সংখ্যা.তথ্য() : m_dynamic_সংখ্যা.তথ্য();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ফেরত m_string;
    }
    ধ্রুবক std::string& str() ধ্রুবক VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ফেরত m_string;
    }

    পূর্ণসংখ্যা width() ধ্রুবক VL_MT_SAFE { ফেরত m_width; }
    V3_সংখ্যা_তথ্য_Type type() ধ্রুবক VL_MT_SAFE { ফেরত m_type; }

    // METHODS
    শূন্য resize(পূর্ণসংখ্যা bits_গুনতি) {
        যদি (m_width == bits_গুনতি) ফেরত;
        যদি (bits_To_শব্দগুলি(m_width) == bits_To_শব্দগুলি(bits_গুনতি)) {
            m_width = bits_গুনতি;
            ফেরত;
        }
        যদি (is_Dynamic_সংখ্যা()) {
            যদি (bits_গুনতি > MAX_INLINE_WIDTH) {
                m_dynamic_সংখ্যা.resize(bits_To_শব্দগুলি(bits_গুনতি));
            } অন্যথা {
                ধ্রুবক auto dynamicBits = std::move(m_dynamic_সংখ্যা);
                destroy_Dynamic_সংখ্যা();
                init_Inline_সংখ্যা();
                std::memcpy(m_inline_সংখ্যা.তথ্য(), dynamicBits.তথ্য(), আকার(m_inline_সংখ্যা));
            }
        } অন্যথা যদি (is_Inline_সংখ্যা()) {
            যদি (bits_গুনতি > MAX_INLINE_WIDTH) {
                ধ্রুবক auto bits = m_inline_সংখ্যা;
                init_Dynamic_সংখ্যা(bits_To_শব্দগুলি(bits_গুনতি));
                std::memcpy(m_dynamic_সংখ্যা.তথ্য(), bits.তথ্য(), আকার(bits));
            }
        }
        m_width = bits_গুনতি;
    }

    শূন্য setString() {
        // If there has been a string already it is kept intact.
        যদি (isString()) ফেরত;
        যদি (is_Dynamic_সংখ্যা()) destroy_Dynamic_সংখ্যা();
        initString();
        m_type = V3_সংখ্যা_তথ্য_Type::STRING;
    }
    শূন্য setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_সংখ্যা_তথ্য_Type::STRING;
    }
    শূন্য setString(ধ্রুবক std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_সংখ্যা_তথ্য_Type::STRING;
    }

    শূন্য setDouble() {
        destroy_Stored_মান();
        যদি (!is_Inline_সংখ্যা()) init_Inline_সংখ্যা();
        m_type = V3_সংখ্যা_তথ্য_Type::DOUBLE;
        resize(64);
    }

    শূন্য setLogic() {
        যদি (isString()) destroyString();
        যদি (!is_সংখ্যা()) {
            যদি (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_সংখ্যা();
            } অন্যথা {
                init_Dynamic_সংখ্যা(bits_To_শব্দগুলি(m_width));
            }
        }
        m_type = V3_সংখ্যা_তথ্য_Type::LOGIC;
        resize(m_width);
    }

ব্যক্তিগত:
    স্থির constexpr পূর্ণসংখ্যা bits_To_শব্দগুলি(পূর্ণসংখ্যা bits_গুনতি) VL_PURE { ফেরত (bits_গুনতি + 31) / 32; }

    bool is_সংখ্যা() ধ্রুবক VL_MT_SAFE {
        ফেরত m_type == V3_সংখ্যা_তথ্য_Type::DOUBLE || m_type == V3_সংখ্যা_তথ্য_Type::LOGIC;
    }
    bool is_Inline_সংখ্যা() ধ্রুবক VL_MT_SAFE {
        ফেরত (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_সংখ্যা_তথ্য_Type::DOUBLE || m_type == V3_সংখ্যা_তথ্য_Type::LOGIC);
    }
    bool is_Dynamic_সংখ্যা() ধ্রুবক VL_MT_SAFE {
        ফেরত (m_width > MAX_INLINE_WIDTH) && (m_type == V3_সংখ্যা_তথ্য_Type::LOGIC);
    }
    bool isString() ধ্রুবক VL_MT_SAFE { ফেরত m_type == V3_সংখ্যা_তথ্য_Type::STRING; }

    template <typename... Args>
    শূন্য init_Inline_সংখ্যা(Args&&... args) {
        new (&m_inline_সংখ্যা) std::array<মান_And_X, INLINE_শব্দগুলি>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    শূন্য init_Dynamic_সংখ্যা(Args&&... args) {
        new (&m_dynamic_সংখ্যা) std::vector<মান_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    শূন্য initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    শূন্য destroy_Dynamic_সংখ্যা() { m_dynamic_সংখ্যা.~vector(); }
    শূন্য destroyString() { m_string.~string(); }
    শূন্য destroy_Stored_মান() {
        যদি (isString())
            destroyString();
        অন্যথা যদি (is_Dynamic_সংখ্যা())
            destroy_Dynamic_সংখ্যা();
    }

    template <typename T>
    শূন্য reinit_With_Or_Assign_Dynamic_সংখ্যা(T&& s) {
        যদি (is_Dynamic_সংখ্যা()) {
            m_dynamic_সংখ্যা = std::forward<T>(s);
            ফেরত;
        }
        যদি (isString()) destroyString();
        init_Dynamic_সংখ্যা(std::forward<T>(s));
    }
    template <typename T>
    শূন্য reinitWithOrAssignString(T&& s) {
        যদি (isString()) {
            m_string = std::forward<T>(s);
            ফেরত;
        }
        যদি (is_Dynamic_সংখ্যা()) destroy_Dynamic_সংখ্যা();
        initString(std::forward<T>(s));
    }
};

class V3_সংখ্যা final {
    // TYPES
    using মান_And_X = V3_সংখ্যা_তথ্য::মান_And_X;
    using V3_সংখ্যা_তথ্য_Type = V3_সংখ্যা_তথ্য::V3_সংখ্যা_তথ্য_Type;

    // MEMBERS
    V3_সংখ্যা_তথ্য m_তথ্য;
    Ast_গ্রন্থি* m_nodep = nullptr;  // Parent node - for error reporting only
    File_সারি* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    শূন্য setNull() {
        m_তথ্য.setLogic();
        m_তথ্য.m_isNull = true;
        m_তথ্য.m_autoExtend = true;
    }
    V3_সংখ্যা& setSingleBits(অক্ষর মান);
    শূন্য fillBits(পূর্ণসংখ্যা destLsb, পূর্ণসংখ্যা width, অক্ষর মান);
    শূন্য copyBits(পূর্ণসংখ্যা destLsb, ধ্রুবক V3_সংখ্যা& source, পূর্ণসংখ্যা sourceLsb, পূর্ণসংখ্যা width);
    V3_সংখ্যা& setString(ধ্রুবক string& str) {
        m_তথ্য.setString(str);
        ফেরত *this;
    }
    শূন্য opCleanThis(bool warnOnTruncation = false);

সর্বজনীন:
    শূন্য nodep(Ast_গ্রন্থি* nodep) VL_MT_STABLE;
    Ast_গ্রন্থি* nodep() ধ্রুবক VL_MT_SAFE { ফেরত m_nodep; }  // For debug only
    File_সারি* fileline() ধ্রুবক VL_MT_SAFE { ফেরত m_fileline; }
    V3_সংখ্যা& setZero();
    V3_সংখ্যা& setQuad(uint64_t মান);
    V3_সংখ্যা& setLong(uint32_t মান);
    V3_সংখ্যা& setLongS(int32_t মান);
    V3_সংখ্যা& setDouble(দ্বিগুণ মান);
    শূন্য setBitX0(পূর্ণসংখ্যা bit);
    শূন্য setBit(পূর্ণসংখ্যা bit, অক্ষর মান) {  // Note: must be initialized as number and pre-zeroed!
        যদি (bit >= m_তথ্য.width()) ফেরত;
        ধ্রুবক uint32_t mask = (1UL << (bit & 31));
        মান_And_X& v = m_তথ্য.অঙ্ক()[bit / 32];
        যদি (মান == '0' || মান == 0) {
            v.m_মান &= ~mask;
            v.m_মান_X &= ~mask;
        } অন্যথা যদি (মান == '1' || মান == 1) {
            v.m_মান |= mask;
            v.m_মান_X &= ~mask;
        } অন্যথা যদি (মান == 'z' || মান == 2) {
            v.m_মান &= ~mask;
            v.m_মান_X |= mask;
        } অন্যথা {  // X
            v.m_মান |= mask;
            v.m_মান_X |= mask;
        }
    }

ব্যক্তিগত:
    অক্ষর bitIs(পূর্ণসংখ্যা bit) ধ্রুবক {
        যদি (bit >= m_তথ্য.width() || bit < 0) {
            // We never sign extend
            ফেরত '0';
        }
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
        ফেরত ("01zx"[(((v.m_মান & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_মান_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    অক্ষর bitIsExtend(পূর্ণসংখ্যা bit, পূর্ণসংখ্যা lbits) ধ্রুবক {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        যদি (bit < 0) ফেরত '0';
        UASSERT(lbits <= m_তথ্য.width(), "Extend of wrong size");
        যদি (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
            // We do sign extend
            ফেরত ("01zx"[(((v.m_মান & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_মান_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
        ফেরত ("01zx"[(((v.m_মান & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_মান_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

সর্বজনীন:
    bool bitIs0(পূর্ণসংখ্যা bit) ধ্রুবক VL_MT_SAFE {
        যদি (!is_সংখ্যা()) ফেরত false;
        যদি (bit < 0) ফেরত false;
        যদি (bit >= m_তথ্য.width()) ফেরত !bitIsXZ(m_তথ্য.width() - 1);
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
        ফেরত ((v.m_মান | v.m_মান_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(পূর্ণসংখ্যা bit) ধ্রুবক VL_MT_SAFE {
        যদি (!is_সংখ্যা()) ফেরত false;
        যদি (bit < 0) ফেরত false;
        যদি (bit >= m_তথ্য.width()) ফেরত false;
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
        ফেরত ((v.m_মান & (1UL << (bit & 31))) && !(v.m_মান_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(পূর্ণসংখ্যা bit) ধ্রুবক {
        // Correct number of zero bits/width matters
        যদি (!is_সংখ্যা()) ফেরত false;
        যদি (bit < 0) ফেরত false;
        যদি (bit >= m_তথ্য.width()) ফেরত bitIs1Extend(m_তথ্য.width() - 1);
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
        ফেরত ((v.m_মান & (1UL << (bit & 31))) && !(v.m_মান_X & (1UL << (bit & 31))));
    }
    bool bitIsX(পূর্ণসংখ্যা bit) ধ্রুবক VL_MT_SAFE {
        যদি (!is_সংখ্যা()) ফেরত false;
        যদি (bit < 0) ফেরত false;
        যদি (bit >= m_তথ্য.width()) ফেরত bitIsZ(m_তথ্য.width() - 1);
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
        ফেরত ((v.m_মান & (1UL << (bit & 31))) && (v.m_মান_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(পূর্ণসংখ্যা bit) ধ্রুবক VL_MT_SAFE {
        যদি (!is_সংখ্যা()) ফেরত false;
        যদি (bit < 0) ফেরত false;
        যদি (bit >= m_তথ্য.width()) ফেরত bitIsXZ(m_তথ্য.width() - 1);
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
        ফেরত ((v.m_মান_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(পূর্ণসংখ্যা bit) ধ্রুবক VL_MT_SAFE {
        যদি (!is_সংখ্যা()) ফেরত false;
        যদি (bit < 0) ফেরত false;
        যদি (bit >= m_তথ্য.width()) ফেরত bitIsZ(m_তথ্য.width() - 1);
        ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[bit / 32];
        ফেরত ((~v.m_মান & (1UL << (bit & 31))) && (v.m_মান_X & (1UL << (bit & 31))));
    }

ব্যক্তিগত:
    uint32_t bits_মান(পূর্ণসংখ্যা lsb, পূর্ণসংখ্যা nbits) ধ্রুবক VL_MT_SAFE {
        uint32_t v = 0;
        জন্য (পূর্ণসংখ্যা bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        ফেরত v;
    }

    পূর্ণসংখ্যা গুনতি_X(পূর্ণসংখ্যা lsb, পূর্ণসংখ্যা nbits) ধ্রুবক VL_MT_SAFE;
    পূর্ণসংখ্যা গুনতি_Z(পূর্ণসংখ্যা lsb, পূর্ণসংখ্যা nbits) ধ্রুবক VL_MT_SAFE;

    পূর্ণসংখ্যা শব্দগুলি() ধ্রুবক VL_MT_SAFE { ফেরত ((width() + 31) / 32); }
    uint32_t hiWordMask() ধ্রুবক VL_MT_SAFE {
        // Correct number of zero bits/width matters
        ফেরত VL_MASK_I(width());
    }

    V3_সংখ্যা& opModDivGuts(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, bool is_modulus);

সর্বজনীন:
    // CONSTRUCTORS
    explicit V3_সংখ্যা(Ast_গ্রন্থি* nodep) { init(nodep, 1); }
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, পূর্ণসংখ্যা width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, পূর্ণসংখ্যা width, ধ্রুবক V3_সংখ্যা& মান) {
        init(nodep, width, width > 0);
        opAssign(মান);
    }
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, পূর্ণসংখ্যা width, uint32_t মান, bool sized = true) {
        init(nodep, width, sized);
        m_তথ্য.অঙ্ক()[0].m_মান = মান;
        opCleanThis();
    }
    V3_সংখ্যা(File_সারি* flp, পূর্ণসংখ্যা width, uint32_t মান, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_তথ্য.অঙ্ক()[0].m_মান = মান;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_সংখ্যা_Literal {};  // For creator type-overload selection
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, Verilog_সংখ্যা_Literal, ধ্রুবক অক্ষর* sourcep) { create(nodep, sourcep); }
    V3_সংখ্যা(File_সারি* flp, Verilog_সংখ্যা_Literal, ধ্রুবক অক্ষর* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, VerilogStringLiteral, ধ্রুবক string& str);
    class Double {};
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, Double, দ্বিগুণ মান) {
        init(nodep, 64);
        setDouble(মান);
    }
    class String {};
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, String, ধ্রুবক string& মান) {
        init(nodep);
        setString(মান);
        m_তথ্য.m_fromString = true;
    }
    class OneStep {};
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, OneStep) {
        init(nodep, 64);
        m_তথ্য.m_is1Step = true;
    }
    class Null {};
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_সংখ্যা(ধ্রুবক V3_সংখ্যা* nump, পূর্ণসংখ্যা width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_সংখ্যা(ধ্রুবক V3_সংখ্যা* nump, পূর্ণসংখ্যা width, uint32_t মান) {
        init(nullptr, width);
        m_তথ্য.অঙ্ক()[0].m_মান = মান;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, ধ্রুবক Ast_গ্রন্থি_DType* nodedtypep);

    V3_সংখ্যা(ধ্রুবক V3_সংখ্যা& other) = পূর্বনির্ধারিত;
    V3_সংখ্যা& operator=(ধ্রুবক V3_সংখ্যা& other) = পূর্বনির্ধারিত;

    V3_সংখ্যা(V3_সংখ্যা&& other) = পূর্বনির্ধারিত;
    V3_সংখ্যা& operator=(V3_সংখ্যা&& other) = পূর্বনির্ধারিত;

    ~V3_সংখ্যা() = পূর্বনির্ধারিত;

ব্যক্তিগত:
    শূন্য selfTestThis();
    শূন্য create(Ast_গ্রন্থি* nodep, ধ্রুবক অক্ষর* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    শূন্য create(File_সারি* flp, ধ্রুবক অক্ষর* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    শূন্য create(ধ্রুবক অক্ষর* sourcep);
    শূন্য init(Ast_গ্রন্থি* nodep, পূর্ণসংখ্যা swidth = -1, bool sized = true) {
        this->nodep(nodep);
        যদি (swidth >= 0) {
            যদি (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_তথ্য.setLogic();
            m_তথ্য.resize(swidth);
            m_তথ্য.m_sized = sized;
            জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) m_তথ্য.অঙ্ক()[i] = {0, 0};
        } অন্যথা {
            m_তথ্য.resize(1);
            m_তথ্য.m_sized = false;
        }
    }
    স্থির string displayPad(আকার_t fmtsize, অক্ষর pad, bool বাম, ধ্রুবক string& in) VL_PURE;
    string displayed(ধ্রুবক string& vformat) ধ্রুবক VL_MT_STABLE {
        ফেরত displayed(m_fileline, vformat);
    }
    শূন্য warnTooMany(ধ্রুবক string& মান);

সর্বজনীন:
    শূন্য v3errorEnd(ধ্রুবক std::ostringstream& sstr) ধ্রুবক VL_RELEASE(V3Error::s().m_mutex);
    শূন্য v3errorEndFatal(ধ্রুবক std::ostringstream& sstr) ধ্রুবক VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    শূন্য width(পূর্ণসংখ্যা width, bool sized = true) {
        m_তথ্য.m_sized = sized;
        m_তথ্য.resize(width);
    }

    // SETTERS
    V3_সংখ্যা& setAllBitsXRemoved();
    V3_সংখ্যা& setAllBitsX();
    V3_সংখ্যা& setAllBitsZ();
    V3_সংখ্যা& setAllBits0();
    V3_সংখ্যা& setAllBits1();
    V3_সংখ্যা& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_সংখ্যা& setMask(পূর্ণসংখ্যা nbits, পূর্ণসংখ্যা lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ধ্রুবক VL_MT_STABLE;
    string displayed(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক string& vformat,
                     ধ্রুবক VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ধ্রুবক VL_MT_STABLE;
    string displayed(File_সারি* fl, ধ্রুবক string& vformat,
                     ধ্রুবক VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ধ্রুবক VL_MT_STABLE;
    string displayedEnum(ধ্রুবক AstSFormatArg* argp, ধ্রুবক string& vformat) ধ্রুবক VL_MT_STABLE;
    string displayedEnumName(ধ্রুবক AstEnumDType* dtypep) ধ্রুবক VL_MT_STABLE;
    স্থির string displayedEnumName(ধ্রুবক AstEnumItem* itemp);
    স্থির bool displayedFmtHasArg(অক্ষর format, bool isScan);
    string emitC() ধ্রুবক VL_MT_STABLE;
    পূর্ণসংখ্যা width() ধ্রুবক VL_MT_SAFE { ফেরত m_তথ্য.width(); }
    পূর্ণসংখ্যা widthToFit() ধ্রুবক;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ধ্রুবক VL_MT_SAFE { ফেরত m_তথ্য.m_sized; }
    bool autoExtend() ধ্রুবক VL_MT_SAFE { ফেরত m_তথ্য.m_autoExtend; }
    bool isFromString() ধ্রুবক { ফেরত m_তথ্য.m_fromString; }
    bool hasOrigParamName() ধ্রুবক { ফেরত m_তথ্য.m_hasOrigParamName; }
    শূন্য hasOrigParamName(bool flag) { m_তথ্য.m_hasOrigParamName = flag; }
    V3_সংখ্যা_তথ্য_Type তথ্য_Type() ধ্রুবক VL_MT_SAFE { ফেরত m_তথ্য.type(); }
    শূন্য তথ্য_Type(V3_সংখ্যা_তথ্য_Type newType) {
        যদি (তথ্য_Type() == newType) ফেরত;
        UASSERT(newType != V3_সংখ্যা_তথ্য_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        নির্বাচন (newType) {
        ক্ষেত্র V3_সংখ্যা_তথ্য_Type::STRING: m_তথ্য.setString(); বিরতি;
        ক্ষেত্র V3_সংখ্যা_তথ্য_Type::DOUBLE: m_তথ্য.setDouble(); বিরতি;
        ক্ষেত্র V3_সংখ্যা_তথ্য_Type::LOGIC: m_তথ্য.setLogic(); বিরতি;
        ক্ষেত্র V3_সংখ্যা_তথ্য_Type::UNINITIALIZED: বিরতি;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ধ্রুবক VL_MT_SAFE { ফেরত m_তথ্য.m_signed; }
    শূন্য isSigned(bool ssigned) { m_তথ্য.m_signed = ssigned; }
    bool isDouble() ধ্রুবক VL_MT_SAFE { ফেরত তথ্য_Type() == V3_সংখ্যা_তথ্য_Type::DOUBLE; }
    bool isString() ধ্রুবক VL_MT_SAFE { ফেরত তথ্য_Type() == V3_সংখ্যা_তথ্য_Type::STRING; }
    bool isOpaque() ধ্রুবক VL_MT_SAFE { ফেরত isDouble() || isString(); }
    bool is_সংখ্যা() ধ্রুবক VL_MT_SAFE {
        ফেরত m_তথ্য.type() == V3_সংখ্যা_তথ্য_Type::LOGIC
               || m_তথ্য.type() == V3_সংখ্যা_তথ্য_Type::DOUBLE;
    }
    bool isNegative() ধ্রুবক VL_MT_SAFE {
        // Correct number of zero bits/width matters
        ফেরত !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ধ্রুবক VL_MT_SAFE { ফেরত m_তথ্য.m_is1Step; }
    bool isNull() ধ্রুবক VL_MT_SAFE { ফেরত m_তথ্য.m_isNull; }
    bool isFourState() ধ্রুবক VL_MT_SAFE;
    bool hasZ() ধ্রুবক {
        যদি (isString()) ফেরত false;
        জন্য (পূর্ণসংখ্যা i = 0; i < শব্দগুলি(); ++i) {
            ধ্রুবক মান_And_X v = m_তথ্য.অঙ্ক()[i];
            যদি ((~v.m_মান) & v.m_মান_X) ফেরত true;
        }
        ফেরত false;
    }
    bool isAllZ() ধ্রুবক VL_MT_SAFE;
    bool isAllX() ধ্রুবক VL_MT_SAFE;
    bool isEqZero() ধ্রুবক VL_MT_SAFE;
    bool isNeqZero() ধ্রুবক;
    bool isBitsZero(পূর্ণসংখ্যা msb, পূর্ণসংখ্যা lsb) ধ্রুবক;
    bool isBroken(পূর্ণসংখ্যা vwidth) ধ্রুবক;
    bool isEqOne() ধ্রুবক;
    bool isEqAllOnes(পূর্ণসংখ্যা optwidth = 0) ধ্রুবক;
    bool isCaseEq(ধ্রুবক V3_সংখ্যা& rhs) ধ্রুবক;  // operator==
    bool isLtXZ(ধ্রুবক V3_সংখ্যা& rhs) ধ্রুবক;  // operator< with XZ compared
    bool isAnyX() ধ্রুবক VL_MT_SAFE;
    bool isAnyXZ() ধ্রুবক;
    bool isAnyZ() ধ্রুবক VL_MT_SAFE;
    bool isMsbXZ() ধ্রুবক {
        // Correct number of zero bits/width matters
        ফেরত bitIsXZ(width() - 1);
    }
    bool fitsInUInt() ধ্রুবক VL_MT_SAFE;
    uint32_t toUInt() ধ্রুবক VL_MT_SAFE;
    int32_t toSInt() ধ্রুবক VL_MT_SAFE;
    uint64_t toUQuad() ধ্রুবক VL_MT_SAFE;
    int64_t toSQuad() ধ্রুবক VL_MT_SAFE;
    string toString() ধ্রুবক VL_MT_SAFE;
    string toDecimalS() ধ্রুবক VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() ধ্রুবক VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() ধ্রুবক VL_MT_STABLE;
    দ্বিগুণ toDouble() ধ্রুবক VL_MT_SAFE;
    V3Hash toHash() ধ্রুবক;
    uint32_t edataWord(পূর্ণসংখ্যা eword) ধ্রুবক;
    uint8_t তথ্য_Byte(পূর্ণসংখ্যা byte) ধ্রুবক;
    uint32_t গুনতি_Bits(ধ্রুবক V3_সংখ্যা& ctrl) ধ্রুবক;
    uint32_t গুনতি_Bits(ধ্রুবক V3_সংখ্যা& ctrl1, ধ্রুবক V3_সংখ্যা& ctrl2, ধ্রুবক V3_সংখ্যা& ctrl3) ধ্রুবক;
    uint32_t গুনতি_Ones() ধ্রুবক;
    uint32_t mostSetBitP1() ধ্রুবক;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ধ্রুবক;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ধ্রুবক V3_সংখ্যা& rhs) ধ্রুবক { ফেরত isLtXZ(rhs); }

    // STATICS
    স্থির bool epsilonEqual(দ্বিগুণ a, দ্বিগুণ b);  // True if number within Epsilon of the other
    স্থির bool epsilonIntegral(দ্বিগুণ a);  // True if number rounds to integer within Epsilon
    স্থির পূর্ণসংখ্যা log2b(uint32_t অঙ্ক);
    স্থির পূর্ণসংখ্যা log2bQuad(uint64_t অঙ্ক);
    স্থির শূন্য selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_সংখ্যা& opBitsNonXZ(ধ্রুবক V3_সংখ্যা& lhs);  // 0/1->1, X/Z->0
    V3_সংখ্যা& opBitsOne(ধ্রুবক V3_সংখ্যা& lhs);  // 1->1, 0/X/Z->0
    V3_সংখ্যা& opBitsXZ(ধ্রুবক V3_সংখ্যা& lhs);  // 0/1->0, X/Z->1
    V3_সংখ্যা& opBitsZ(ধ্রুবক V3_সংখ্যা& lhs);  // Z->1, 0/1/X->0
    //
    V3_সংখ্যা& opAssign(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opAssignNonXZ(ধ্রুবক V3_সংখ্যা& lhs, bool ignoreXZ = true);
    V3_সংখ্যা& opExtendS(ধ্রুবক V3_সংখ্যা& lhs, uint32_t lbits);  // Sign extension
    V3_সংখ্যা& opExtendXZ(ধ্রুবক V3_সংখ্যা& lhs, uint32_t lbits);  // X/Z extension
    V3_সংখ্যা& opRedOr(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRedAnd(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRedXor(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& op_গুনতি_Bits(ধ্রুবক V3_সংখ্যা& রাশিমালা, ধ্রুবক V3_সংখ্যা& ctrl1, ধ্রুবক V3_সংখ্যা& ctrl2,
                          ধ্রুবক V3_সংখ্যা& ctrl3);
    V3_সংখ্যা& op_গুনতি_Ones(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opIsUnknown(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opOneHot(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opOneHot0(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opCLog2(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opMostSetBitP1(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opClean(ধ্রুবক V3_সংখ্যা& lhs, uint32_t bits);
    V3_সংখ্যা& opConcat(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLenN(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRepl(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opRepl(ধ্রুবক V3_সংখ্যা& lhs, uint32_t rhsval);
    V3_সংখ্যা& opStreamL(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opSel(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& msb, ধ্রুবক V3_সংখ্যা& lsb);
    V3_সংখ্যা& opSel(ধ্রুবক V3_সংখ্যা& lhs, uint32_t msbval, uint32_t lsbval);
    V3_সংখ্যা& opSelInto(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& lsb, পূর্ণসংখ্যা width);
    V3_সংখ্যা& opSelInto(ধ্রুবক V3_সংখ্যা& lhs, পূর্ণসংখ্যা lsbval, পূর্ণসংখ্যা width);
    V3_সংখ্যা& opCvtPackString(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opToLowerN(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opToUpperN(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opCaseEq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opCaseNeq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opWildEq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opWildNeq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opBufIf1(ধ্রুবক V3_সংখ্যা& ens, ধ্রুবক V3_সংখ্যা& if1s);
    V3_সংখ্যা& opSetRange(uint32_t lsb, uint32_t width, অক্ষর bit_মান);
    // "standard" math
    V3_সংখ্যা& opNot(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opLogNot(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opLogAnd(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLogOr(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLogEq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLogIf(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opNegate(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opAdd(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opSub(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opMul(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opMulS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opDiv(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opDivS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opModDiv(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opModDivS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opPow(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, bool lsign = false,
                    bool rsign = false);
    V3_সংখ্যা& opPowSU(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed lhs, unsigned rhs
    V3_সংখ্যা& opPowSS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed lhs, signed rhs
    V3_সংখ্যা& opPowUS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Unsigned lhs, signed rhs
    V3_সংখ্যা& opAnd(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opXor(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opOr(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opShiftR(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opShiftRS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_সংখ্যা& opShiftL(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    // Comparisons
    V3_সংখ্যা& opEq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opNeq(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGt(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGtS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opGte(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGteS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opLt(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLtS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opLte(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLteS(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);  // Signed

    // "D" - double (aka real) math
    V3_সংখ্যা& opIToRD(ধ্রুবক V3_সংখ্যা& lhs, bool isSigned = false);
    V3_সংখ্যা& opISToRD(ধ্রুবক V3_সংখ্যা& lhs) { ফেরত opIToRD(lhs, true); }
    V3_সংখ্যা& opRToIS(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRToIRoundS(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRealToBits(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opBitsToRealD(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opNegateD(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opAddD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opSubD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opMulD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opDivD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opPowD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    // Comparisons
    V3_সংখ্যা& opEqD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opNeqD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGtD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGteD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLtD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLteD(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);

    // "N" - string operations
    V3_সংখ্যা& opAtoN(ধ্রুবক V3_সংখ্যা& lhs, পূর্ণসংখ্যা base);
    V3_সংখ্যা& opNToI(ধ্রুবক V3_সংখ্যা& lhs);
    V3_সংখ্যা& opPutcN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, ধ্রুবক V3_সংখ্যা& ths);
    V3_সংখ্যা& opGetcN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opSubstrN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, ধ্রুবক V3_সংখ্যা& ths);
    V3_সংখ্যা& opCompareNN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs, bool ignoreCase);
    V3_সংখ্যা& opConcatN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opReplN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opReplN(ধ্রুবক V3_সংখ্যা& lhs, uint32_t rhsval);
    V3_সংখ্যা& opEqN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opNeqN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGtN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGteN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLtN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLteN(ধ্রুবক V3_সংখ্যা& lhs, ধ্রুবক V3_সংখ্যা& rhs);
};
অন্তঃরেখ std::ostream& operator<<(std::ostream& os, ধ্রুবক V3_সংখ্যা& rhs) VL_MT_SAFE {
    ফেরত os << rhs.ascii();
}

#endif  // Guard
