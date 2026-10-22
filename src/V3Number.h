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
সার্বজনিক:
    গণনা en : বর্ণ {
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
    explicit VFormatAttr(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুব { প্রত্যাগম m_e; }
    বর্ণ ascii() ধ্রুব { প্রত্যাগম m_e; }
    bool isComplex() ধ্রুব { প্রত্যাগম m_e == COMPLEX; }
    bool isDouble() ধ্রুব { প্রত্যাগম m_e == DOUBLE; }
    bool isEnum() ধ্রুব { প্রত্যাগম m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ধ্রুব { প্রত্যাগম m_e == SIGNED; }
    bool isString() ধ্রুব { প্রত্যাগম m_e == STRING; }
    bool isUnsigned() ধ্রুব { প্রত্যাগম m_e == UNSIGNED; }
};
constexpr bool operator==(ধ্রুব VFormatAttr& lhs, ধ্রুব VFormatAttr& rhs) {
    প্রত্যাগম lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুব VFormatAttr& lhs, VFormatAttr::en rhs) { প্রত্যাগম lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, ধ্রুব VFormatAttr& rhs) { প্রত্যাগম lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুব VFormatAttr& rhs) {
    প্রত্যাগম os << rhs.ascii();
}

//============================================================================

class V3_সংখ্যা_Data final {
সার্বজনিক:
    // TYPES
    সংরচনা মান_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_মান;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_মান_X;
        bool operator==(ধ্রুব মান_And_X& other) ধ্রুব VL_MT_SAFE {
            প্রত্যাগম m_মান == other.m_মান && m_মান_X == other.m_মান_X;
        }
    };

    গণনা class V3_সংখ্যা_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ধ্রুব V3_সংখ্যা_Data_Type& rhs) VL_MT_SAFE {
        বিকল্প (rhs) {
        পক্ষ V3_সংখ্যা_Data_Type::UNINITIALIZED: প্রত্যাগম os << "UNINITIALIZED";
        পক্ষ V3_সংখ্যা_Data_Type::LOGIC: প্রত্যাগম os << "LOGIC";
        পক্ষ V3_সংখ্যা_Data_Type::DOUBLE: প্রত্যাগম os << "DOUBLE";
        পক্ষ V3_সংখ্যা_Data_Type::STRING: প্রত্যাগম os << "STRING";
        }
        প্রত্যাগম os;
    }

গুপ্ত:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    স্থির constexpr পূর্ণাঙ্ক INLINE_শব্দগুলি = std::max(
        static_cast<size_t>(2), std::max(পরিমাণ(std::string) / পরিমাণ(মান_And_X),
                                         পরিমাণ(std::vector<মান_And_X>) / পরিমাণ(মান_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    স্থির constexpr পূর্ণাঙ্ক MAX_INLINE_WIDTH = INLINE_শব্দগুলি * পরিমাণ(মান_And_X) / 2 * 8;

    // MEMBERS
    সঙ্ঘ {
        std::array<মান_And_X, INLINE_শব্দগুলি> m_inline_সংখ্যা;
        std::vector<মান_And_X> m_dynamic_সংখ্যা;
        std::string m_string;
    };

    পূর্ণাঙ্ক m_width = 0;  // Width (in bits) as specified/calculated
    V3_সংখ্যা_Data_Type m_type;

সার্বজনিক:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_সংখ্যা_Data()
        : m_type{V3_সংখ্যা_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_সংখ্যা_Data() { destroy_Stored_মান(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_সংখ্যা_Data(ধ্রুব V3_সংখ্যা_Data& other)
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

    V3_সংখ্যা_Data& operator=(ধ্রুব V3_সংখ্যা_Data& other) {
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
        প্রত্যাগম *this;
    }

    V3_সংখ্যা_Data(V3_সংখ্যা_Data&& other)
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
        other.m_type = V3_সংখ্যা_Data_Type::UNINITIALIZED;
    }

    V3_সংখ্যা_Data& operator=(V3_সংখ্যা_Data&& other) {
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
        other.m_type = V3_সংখ্যা_Data_Type::UNINITIALIZED;
        প্রত্যাগম *this;
    }

    // ACCESSORS
    মান_And_X* অঙ্ক() {
        UASSERT(is_সংখ্যা(), "`num` member accessed when data type is " << m_type);
        প্রত্যাগম is_Inline_সংখ্যা() ? m_inline_সংখ্যা.data() : m_dynamic_সংখ্যা.data();
    }
    ধ্রুব মান_And_X* অঙ্ক() ধ্রুব VL_MT_SAFE {
        UASSERT(is_সংখ্যা(), "`num` member accessed when data type is " << m_type);
        প্রত্যাগম is_Inline_সংখ্যা() ? m_inline_সংখ্যা.data() : m_dynamic_সংখ্যা.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        প্রত্যাগম m_string;
    }
    ধ্রুব std::string& str() ধ্রুব VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        প্রত্যাগম m_string;
    }

    পূর্ণাঙ্ক width() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_width; }
    V3_সংখ্যা_Data_Type type() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_type; }

    // METHODS
    শূন্য resize(পূর্ণাঙ্ক bits_গুনতি) {
        যদি (m_width == bits_গুনতি) প্রত্যাগম;
        যদি (bits_To_শব্দগুলি(m_width) == bits_To_শব্দগুলি(bits_গুনতি)) {
            m_width = bits_গুনতি;
            প্রত্যাগম;
        }
        যদি (is_Dynamic_সংখ্যা()) {
            যদি (bits_গুনতি > MAX_INLINE_WIDTH) {
                m_dynamic_সংখ্যা.resize(bits_To_শব্দগুলি(bits_গুনতি));
            } অন্যথা {
                ধ্রুব auto dynamicBits = std::move(m_dynamic_সংখ্যা);
                destroy_Dynamic_সংখ্যা();
                init_Inline_সংখ্যা();
                std::memcpy(m_inline_সংখ্যা.data(), dynamicBits.data(), পরিমাণ(m_inline_সংখ্যা));
            }
        } অন্যথা যদি (is_Inline_সংখ্যা()) {
            যদি (bits_গুনতি > MAX_INLINE_WIDTH) {
                ধ্রুব auto bits = m_inline_সংখ্যা;
                init_Dynamic_সংখ্যা(bits_To_শব্দগুলি(bits_গুনতি));
                std::memcpy(m_dynamic_সংখ্যা.data(), bits.data(), পরিমাণ(bits));
            }
        }
        m_width = bits_গুনতি;
    }

    শূন্য setString() {
        // If there has been a string already it is kept intact.
        যদি (isString()) প্রত্যাগম;
        যদি (is_Dynamic_সংখ্যা()) destroy_Dynamic_সংখ্যা();
        initString();
        m_type = V3_সংখ্যা_Data_Type::STRING;
    }
    শূন্য setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_সংখ্যা_Data_Type::STRING;
    }
    শূন্য setString(ধ্রুব std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_সংখ্যা_Data_Type::STRING;
    }

    শূন্য setDouble() {
        destroy_Stored_মান();
        যদি (!is_Inline_সংখ্যা()) init_Inline_সংখ্যা();
        m_type = V3_সংখ্যা_Data_Type::DOUBLE;
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
        m_type = V3_সংখ্যা_Data_Type::LOGIC;
        resize(m_width);
    }

গুপ্ত:
    স্থির constexpr পূর্ণাঙ্ক bits_To_শব্দগুলি(পূর্ণাঙ্ক bits_গুনতি) VL_PURE { প্রত্যাগম (bits_গুনতি + 31) / 32; }

    bool is_সংখ্যা() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম m_type == V3_সংখ্যা_Data_Type::DOUBLE || m_type == V3_সংখ্যা_Data_Type::LOGIC;
    }
    bool is_Inline_সংখ্যা() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_সংখ্যা_Data_Type::DOUBLE || m_type == V3_সংখ্যা_Data_Type::LOGIC);
    }
    bool is_Dynamic_সংখ্যা() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম (m_width > MAX_INLINE_WIDTH) && (m_type == V3_সংখ্যা_Data_Type::LOGIC);
    }
    bool isString() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_type == V3_সংখ্যা_Data_Type::STRING; }

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
            প্রত্যাগম;
        }
        যদি (isString()) destroyString();
        init_Dynamic_সংখ্যা(std::forward<T>(s));
    }
    template <typename T>
    শূন্য reinitWithOrAssignString(T&& s) {
        যদি (isString()) {
            m_string = std::forward<T>(s);
            প্রত্যাগম;
        }
        যদি (is_Dynamic_সংখ্যা()) destroy_Dynamic_সংখ্যা();
        initString(std::forward<T>(s));
    }
};

class V3_সংখ্যা final {
    // TYPES
    using মান_And_X = V3_সংখ্যা_Data::মান_And_X;
    using V3_সংখ্যা_Data_Type = V3_সংখ্যা_Data::V3_সংখ্যা_Data_Type;

    // MEMBERS
    V3_সংখ্যা_Data m_data;
    Ast_গ্রন্থি* m_nodep = nullptr;  // Parent node - for error reporting only
    File_সারি* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    শূন্য setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_সংখ্যা& setSingleBits(বর্ণ মান);
    শূন্য fillBits(পূর্ণাঙ্ক destLsb, পূর্ণাঙ্ক width, বর্ণ মান);
    শূন্য copyBits(পূর্ণাঙ্ক destLsb, ধ্রুব V3_সংখ্যা& source, পূর্ণাঙ্ক sourceLsb, পূর্ণাঙ্ক width);
    V3_সংখ্যা& setString(ধ্রুব string& str) {
        m_data.setString(str);
        প্রত্যাগম *this;
    }
    শূন্য opCleanThis(bool warnOnTruncation = false);

সার্বজনিক:
    শূন্য nodep(Ast_গ্রন্থি* nodep) VL_MT_STABLE;
    Ast_গ্রন্থি* nodep() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_nodep; }  // For debug only
    File_সারি* fileline() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_fileline; }
    V3_সংখ্যা& setZero();
    V3_সংখ্যা& setQuad(uint64_t মান);
    V3_সংখ্যা& setLong(uint32_t মান);
    V3_সংখ্যা& setLongS(int32_t মান);
    V3_সংখ্যা& setDouble(দ্বিগুণ মান);
    শূন্য setBitX0(পূর্ণাঙ্ক bit);
    শূন্য setBit(পূর্ণাঙ্ক bit, বর্ণ মান) {  // Note: must be initialized as number and pre-zeroed!
        যদি (bit >= m_data.width()) প্রত্যাগম;
        ধ্রুব uint32_t mask = (1UL << (bit & 31));
        মান_And_X& v = m_data.অঙ্ক()[bit / 32];
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

গুপ্ত:
    বর্ণ bitIs(পূর্ণাঙ্ক bit) ধ্রুব {
        যদি (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            প্রত্যাগম '0';
        }
        ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
        প্রত্যাগম ("01zx"[(((v.m_মান & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_মান_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    বর্ণ bitIsExtend(পূর্ণাঙ্ক bit, পূর্ণাঙ্ক lbits) ধ্রুব {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        যদি (bit < 0) প্রত্যাগম '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        যদি (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
            // We do sign extend
            প্রত্যাগম ("01zx"[(((v.m_মান & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_মান_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
        প্রত্যাগম ("01zx"[(((v.m_মান & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_মান_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

সার্বজনিক:
    bool bitIs0(পূর্ণাঙ্ক bit) ধ্রুব VL_MT_SAFE {
        যদি (!is_সংখ্যা()) প্রত্যাগম false;
        যদি (bit < 0) প্রত্যাগম false;
        যদি (bit >= m_data.width()) প্রত্যাগম !bitIsXZ(m_data.width() - 1);
        ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
        প্রত্যাগম ((v.m_মান | v.m_মান_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(পূর্ণাঙ্ক bit) ধ্রুব VL_MT_SAFE {
        যদি (!is_সংখ্যা()) প্রত্যাগম false;
        যদি (bit < 0) প্রত্যাগম false;
        যদি (bit >= m_data.width()) প্রত্যাগম false;
        ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
        প্রত্যাগম ((v.m_মান & (1UL << (bit & 31))) && !(v.m_মান_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(পূর্ণাঙ্ক bit) ধ্রুব {
        // Correct number of zero bits/width matters
        যদি (!is_সংখ্যা()) প্রত্যাগম false;
        যদি (bit < 0) প্রত্যাগম false;
        যদি (bit >= m_data.width()) প্রত্যাগম bitIs1Extend(m_data.width() - 1);
        ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
        প্রত্যাগম ((v.m_মান & (1UL << (bit & 31))) && !(v.m_মান_X & (1UL << (bit & 31))));
    }
    bool bitIsX(পূর্ণাঙ্ক bit) ধ্রুব VL_MT_SAFE {
        যদি (!is_সংখ্যা()) প্রত্যাগম false;
        যদি (bit < 0) প্রত্যাগম false;
        যদি (bit >= m_data.width()) প্রত্যাগম bitIsZ(m_data.width() - 1);
        ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
        প্রত্যাগম ((v.m_মান & (1UL << (bit & 31))) && (v.m_মান_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(পূর্ণাঙ্ক bit) ধ্রুব VL_MT_SAFE {
        যদি (!is_সংখ্যা()) প্রত্যাগম false;
        যদি (bit < 0) প্রত্যাগম false;
        যদি (bit >= m_data.width()) প্রত্যাগম bitIsXZ(m_data.width() - 1);
        ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
        প্রত্যাগম ((v.m_মান_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(পূর্ণাঙ্ক bit) ধ্রুব VL_MT_SAFE {
        যদি (!is_সংখ্যা()) প্রত্যাগম false;
        যদি (bit < 0) প্রত্যাগম false;
        যদি (bit >= m_data.width()) প্রত্যাগম bitIsZ(m_data.width() - 1);
        ধ্রুব মান_And_X v = m_data.অঙ্ক()[bit / 32];
        প্রত্যাগম ((~v.m_মান & (1UL << (bit & 31))) && (v.m_মান_X & (1UL << (bit & 31))));
    }

গুপ্ত:
    uint32_t bits_মান(পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক nbits) ধ্রুব VL_MT_SAFE {
        uint32_t v = 0;
        প্রতি (পূর্ণাঙ্ক bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        প্রত্যাগম v;
    }

    পূর্ণাঙ্ক গুনতি_X(পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক nbits) ধ্রুব VL_MT_SAFE;
    পূর্ণাঙ্ক গুনতি_Z(পূর্ণাঙ্ক lsb, পূর্ণাঙ্ক nbits) ধ্রুব VL_MT_SAFE;

    পূর্ণাঙ্ক শব্দগুলি() ধ্রুব VL_MT_SAFE { প্রত্যাগম ((width() + 31) / 32); }
    uint32_t hiWordMask() ধ্রুব VL_MT_SAFE {
        // Correct number of zero bits/width matters
        প্রত্যাগম VL_MASK_I(width());
    }

    V3_সংখ্যা& opModDivGuts(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs, bool is_modulus);

সার্বজনিক:
    // CONSTRUCTORS
    explicit V3_সংখ্যা(Ast_গ্রন্থি* nodep) { init(nodep, 1); }
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক width, ধ্রুব V3_সংখ্যা& মান) {
        init(nodep, width, width > 0);
        opAssign(মান);
    }
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক width, uint32_t মান, bool sized = true) {
        init(nodep, width, sized);
        m_data.অঙ্ক()[0].m_মান = মান;
        opCleanThis();
    }
    V3_সংখ্যা(File_সারি* flp, পূর্ণাঙ্ক width, uint32_t মান, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.অঙ্ক()[0].m_মান = মান;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_সংখ্যা_Literal {};  // For creator type-overload selection
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, Verilog_সংখ্যা_Literal, ধ্রুব বর্ণ* sourcep) { create(nodep, sourcep); }
    V3_সংখ্যা(File_সারি* flp, Verilog_সংখ্যা_Literal, ধ্রুব বর্ণ* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, VerilogStringLiteral, ধ্রুব string& str);
    class Double {};
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, Double, দ্বিগুণ মান) {
        init(nodep, 64);
        setDouble(মান);
    }
    class String {};
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, String, ধ্রুব string& মান) {
        init(nodep);
        setString(মান);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_সংখ্যা(ধ্রুব V3_সংখ্যা* nump, পূর্ণাঙ্ক width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_সংখ্যা(ধ্রুব V3_সংখ্যা* nump, পূর্ণাঙ্ক width, uint32_t মান) {
        init(nullptr, width);
        m_data.অঙ্ক()[0].m_মান = মান;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_সংখ্যা(Ast_গ্রন্থি* nodep, ধ্রুব Ast_গ্রন্থি_DType* nodedtypep);

    V3_সংখ্যা(ধ্রুব V3_সংখ্যা& other) = স্বভাব;
    V3_সংখ্যা& operator=(ধ্রুব V3_সংখ্যা& other) = স্বভাব;

    V3_সংখ্যা(V3_সংখ্যা&& other) = স্বভাব;
    V3_সংখ্যা& operator=(V3_সংখ্যা&& other) = স্বভাব;

    ~V3_সংখ্যা() = স্বভাব;

গুপ্ত:
    শূন্য selfTestThis();
    শূন্য create(Ast_গ্রন্থি* nodep, ধ্রুব বর্ণ* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    শূন্য create(File_সারি* flp, ধ্রুব বর্ণ* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    শূন্য create(ধ্রুব বর্ণ* sourcep);
    শূন্য init(Ast_গ্রন্থি* nodep, পূর্ণাঙ্ক swidth = -1, bool sized = true) {
        this->nodep(nodep);
        যদি (swidth >= 0) {
            যদি (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            প্রতি (পূর্ণাঙ্ক i = 0; i < শব্দগুলি(); ++i) m_data.অঙ্ক()[i] = {0, 0};
        } অন্যথা {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    স্থির string displayPad(size_t fmtsize, বর্ণ pad, bool বাম, ধ্রুব string& in) VL_PURE;
    string displayed(ধ্রুব string& vformat) ধ্রুব VL_MT_STABLE {
        প্রত্যাগম displayed(m_fileline, vformat);
    }
    শূন্য warnTooMany(ধ্রুব string& মান);

সার্বজনিক:
    শূন্য v3errorEnd(ধ্রুব std::ostringstream& sstr) ধ্রুব VL_RELEASE(V3Error::s().m_mutex);
    শূন্য v3errorEndFatal(ধ্রুব std::ostringstream& sstr) ধ্রুব VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    শূন্য width(পূর্ণাঙ্ক width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_সংখ্যা& setAllBitsXRemoved();
    V3_সংখ্যা& setAllBitsX();
    V3_সংখ্যা& setAllBitsZ();
    V3_সংখ্যা& setAllBits0();
    V3_সংখ্যা& setAllBits1();
    V3_সংখ্যা& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_সংখ্যা& setMask(পূর্ণাঙ্ক nbits, পূর্ণাঙ্ক lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ধ্রুব VL_MT_STABLE;
    string displayed(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব string& vformat,
                     ধ্রুব VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ধ্রুব VL_MT_STABLE;
    string displayed(File_সারি* fl, ধ্রুব string& vformat,
                     ধ্রুব VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ধ্রুব VL_MT_STABLE;
    string displayedEnum(ধ্রুব AstSFormatArg* argp, ধ্রুব string& vformat) ধ্রুব VL_MT_STABLE;
    string displayedEnumName(ধ্রুব AstEnumDType* dtypep) ধ্রুব VL_MT_STABLE;
    স্থির string displayedEnumName(ধ্রুব AstEnumItem* itemp);
    স্থির bool displayedFmtHasArg(বর্ণ format, bool isScan);
    string emitC() ধ্রুব VL_MT_STABLE;
    পূর্ণাঙ্ক width() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_data.width(); }
    পূর্ণাঙ্ক widthToFit() ধ্রুব;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_data.m_sized; }
    bool autoExtend() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_data.m_autoExtend; }
    bool isFromString() ধ্রুব { প্রত্যাগম m_data.m_fromString; }
    bool hasOrigParamName() ধ্রুব { প্রত্যাগম m_data.m_hasOrigParamName; }
    শূন্য hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_সংখ্যা_Data_Type dataType() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_data.type(); }
    শূন্য dataType(V3_সংখ্যা_Data_Type newType) {
        যদি (dataType() == newType) প্রত্যাগম;
        UASSERT(newType != V3_সংখ্যা_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        বিকল্প (newType) {
        পক্ষ V3_সংখ্যা_Data_Type::STRING: m_data.setString(); ভঙ্গ;
        পক্ষ V3_সংখ্যা_Data_Type::DOUBLE: m_data.setDouble(); ভঙ্গ;
        পক্ষ V3_সংখ্যা_Data_Type::LOGIC: m_data.setLogic(); ভঙ্গ;
        পক্ষ V3_সংখ্যা_Data_Type::UNINITIALIZED: ভঙ্গ;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_data.m_signed; }
    শূন্য isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ধ্রুব VL_MT_SAFE { প্রত্যাগম dataType() == V3_সংখ্যা_Data_Type::DOUBLE; }
    bool isString() ধ্রুব VL_MT_SAFE { প্রত্যাগম dataType() == V3_সংখ্যা_Data_Type::STRING; }
    bool isOpaque() ধ্রুব VL_MT_SAFE { প্রত্যাগম isDouble() || isString(); }
    bool is_সংখ্যা() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম m_data.type() == V3_সংখ্যা_Data_Type::LOGIC
               || m_data.type() == V3_সংখ্যা_Data_Type::DOUBLE;
    }
    bool isNegative() ধ্রুব VL_MT_SAFE {
        // Correct number of zero bits/width matters
        প্রত্যাগম !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_data.m_is1Step; }
    bool isNull() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_data.m_isNull; }
    bool isFourState() ধ্রুব VL_MT_SAFE;
    bool hasZ() ধ্রুব {
        যদি (isString()) প্রত্যাগম false;
        প্রতি (পূর্ণাঙ্ক i = 0; i < শব্দগুলি(); ++i) {
            ধ্রুব মান_And_X v = m_data.অঙ্ক()[i];
            যদি ((~v.m_মান) & v.m_মান_X) প্রত্যাগম true;
        }
        প্রত্যাগম false;
    }
    bool isAllZ() ধ্রুব VL_MT_SAFE;
    bool isAllX() ধ্রুব VL_MT_SAFE;
    bool isEqZero() ধ্রুব VL_MT_SAFE;
    bool isNeqZero() ধ্রুব;
    bool isBitsZero(পূর্ণাঙ্ক msb, পূর্ণাঙ্ক lsb) ধ্রুব;
    bool isBroken(পূর্ণাঙ্ক vwidth) ধ্রুব;
    bool isEqOne() ধ্রুব;
    bool isEqAllOnes(পূর্ণাঙ্ক optwidth = 0) ধ্রুব;
    bool isCaseEq(ধ্রুব V3_সংখ্যা& rhs) ধ্রুব;  // operator==
    bool isLtXZ(ধ্রুব V3_সংখ্যা& rhs) ধ্রুব;  // operator< with XZ compared
    bool isAnyX() ধ্রুব VL_MT_SAFE;
    bool isAnyXZ() ধ্রুব;
    bool isAnyZ() ধ্রুব VL_MT_SAFE;
    bool isMsbXZ() ধ্রুব {
        // Correct number of zero bits/width matters
        প্রত্যাগম bitIsXZ(width() - 1);
    }
    bool fitsInUInt() ধ্রুব VL_MT_SAFE;
    uint32_t toUInt() ধ্রুব VL_MT_SAFE;
    int32_t toSInt() ধ্রুব VL_MT_SAFE;
    uint64_t toUQuad() ধ্রুব VL_MT_SAFE;
    int64_t toSQuad() ধ্রুব VL_MT_SAFE;
    string toString() ধ্রুব VL_MT_SAFE;
    string toDecimalS() ধ্রুব VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() ধ্রুব VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() ধ্রুব VL_MT_STABLE;
    দ্বিগুণ toDouble() ধ্রুব VL_MT_SAFE;
    V3Hash toHash() ধ্রুব;
    uint32_t edataWord(পূর্ণাঙ্ক eword) ধ্রুব;
    uint8_t dataByte(পূর্ণাঙ্ক byte) ধ্রুব;
    uint32_t গুনতি_Bits(ধ্রুব V3_সংখ্যা& ctrl) ধ্রুব;
    uint32_t গুনতি_Bits(ধ্রুব V3_সংখ্যা& ctrl1, ধ্রুব V3_সংখ্যা& ctrl2, ধ্রুব V3_সংখ্যা& ctrl3) ধ্রুব;
    uint32_t গুনতি_Ones() ধ্রুব;
    uint32_t mostSetBitP1() ধ্রুব;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ধ্রুব;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ধ্রুব V3_সংখ্যা& rhs) ধ্রুব { প্রত্যাগম isLtXZ(rhs); }

    // STATICS
    স্থির bool epsilonEqual(দ্বিগুণ a, দ্বিগুণ b);  // True if number within Epsilon of the other
    স্থির bool epsilonIntegral(দ্বিগুণ a);  // True if number rounds to integer within Epsilon
    স্থির পূর্ণাঙ্ক log2b(uint32_t অঙ্ক);
    স্থির পূর্ণাঙ্ক log2bQuad(uint64_t অঙ্ক);
    স্থির শূন্য selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_সংখ্যা& opBitsNonXZ(ধ্রুব V3_সংখ্যা& lhs);  // 0/1->1, X/Z->0
    V3_সংখ্যা& opBitsOne(ধ্রুব V3_সংখ্যা& lhs);  // 1->1, 0/X/Z->0
    V3_সংখ্যা& opBitsXZ(ধ্রুব V3_সংখ্যা& lhs);  // 0/1->0, X/Z->1
    V3_সংখ্যা& opBitsZ(ধ্রুব V3_সংখ্যা& lhs);  // Z->1, 0/1/X->0
    //
    V3_সংখ্যা& opAssign(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opAssignNonXZ(ধ্রুব V3_সংখ্যা& lhs, bool ignoreXZ = true);
    V3_সংখ্যা& opExtendS(ধ্রুব V3_সংখ্যা& lhs, uint32_t lbits);  // Sign extension
    V3_সংখ্যা& opExtendXZ(ধ্রুব V3_সংখ্যা& lhs, uint32_t lbits);  // X/Z extension
    V3_সংখ্যা& opRedOr(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRedAnd(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRedXor(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& op_গুনতি_Bits(ধ্রুব V3_সংখ্যা& রাশিমালা, ধ্রুব V3_সংখ্যা& ctrl1, ধ্রুব V3_সংখ্যা& ctrl2,
                          ধ্রুব V3_সংখ্যা& ctrl3);
    V3_সংখ্যা& op_গুনতি_Ones(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opIsUnknown(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opOneHot(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opOneHot0(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opCLog2(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opMostSetBitP1(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opClean(ধ্রুব V3_সংখ্যা& lhs, uint32_t bits);
    V3_সংখ্যা& opConcat(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLenN(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRepl(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opRepl(ধ্রুব V3_সংখ্যা& lhs, uint32_t rhsval);
    V3_সংখ্যা& opStreamL(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opSel(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& msb, ধ্রুব V3_সংখ্যা& lsb);
    V3_সংখ্যা& opSel(ধ্রুব V3_সংখ্যা& lhs, uint32_t msbval, uint32_t lsbval);
    V3_সংখ্যা& opSelInto(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& lsb, পূর্ণাঙ্ক width);
    V3_সংখ্যা& opSelInto(ধ্রুব V3_সংখ্যা& lhs, পূর্ণাঙ্ক lsbval, পূর্ণাঙ্ক width);
    V3_সংখ্যা& opCvtPackString(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opToLowerN(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opToUpperN(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opCaseEq(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opCaseNeq(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opWildEq(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opWildNeq(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opBufIf1(ধ্রুব V3_সংখ্যা& ens, ধ্রুব V3_সংখ্যা& if1s);
    V3_সংখ্যা& opSetRange(uint32_t lsb, uint32_t width, বর্ণ bit_মান);
    // "standard" math
    V3_সংখ্যা& opNot(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opLogNot(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opLogAnd(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLogOr(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLogEq(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLogIf(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opNegate(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opAdd(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opSub(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opMul(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opMulS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opDiv(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opDivS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opModDiv(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opModDivS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opPow(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs, bool lsign = false,
                    bool rsign = false);
    V3_সংখ্যা& opPowSU(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed lhs, unsigned rhs
    V3_সংখ্যা& opPowSS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed lhs, signed rhs
    V3_সংখ্যা& opPowUS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Unsigned lhs, signed rhs
    V3_সংখ্যা& opAnd(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opXor(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opOr(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opShiftR(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opShiftRS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_সংখ্যা& opShiftL(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    // Comparisons
    V3_সংখ্যা& opEq(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opNeq(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGt(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGtS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opGte(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGteS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opLt(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLtS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed
    V3_সংখ্যা& opLte(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLteS(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);  // Signed

    // "D" - double (aka real) math
    V3_সংখ্যা& opIToRD(ধ্রুব V3_সংখ্যা& lhs, bool isSigned = false);
    V3_সংখ্যা& opISToRD(ধ্রুব V3_সংখ্যা& lhs) { প্রত্যাগম opIToRD(lhs, true); }
    V3_সংখ্যা& opRToIS(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRToIRoundS(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opRealToBits(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opBitsToRealD(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opNegateD(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opAddD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opSubD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opMulD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opDivD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opPowD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    // Comparisons
    V3_সংখ্যা& opEqD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opNeqD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGtD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGteD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLtD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLteD(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);

    // "N" - string operations
    V3_সংখ্যা& opAtoN(ধ্রুব V3_সংখ্যা& lhs, পূর্ণাঙ্ক base);
    V3_সংখ্যা& opNToI(ধ্রুব V3_সংখ্যা& lhs);
    V3_সংখ্যা& opPutcN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs, ধ্রুব V3_সংখ্যা& ths);
    V3_সংখ্যা& opGetcN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opSubstrN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs, ধ্রুব V3_সংখ্যা& ths);
    V3_সংখ্যা& opCompareNN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs, bool ignoreCase);
    V3_সংখ্যা& opConcatN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opReplN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opReplN(ধ্রুব V3_সংখ্যা& lhs, uint32_t rhsval);
    V3_সংখ্যা& opEqN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opNeqN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGtN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opGteN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLtN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
    V3_সংখ্যা& opLteN(ধ্রুব V3_সংখ্যা& lhs, ধ্রুব V3_সংখ্যা& rhs);
};
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুব V3_সংখ্যা& rhs) VL_MT_SAFE {
    প্রত্যাগম os << rhs.ascii();
}

#endif  // Guard
