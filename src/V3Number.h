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

#ifndef VERILATOR_V3_எண்_H
#define VERILATOR_V3_எண்_H

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Hash.h"
#அந்தர்பாவ "V3StdFuture.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <array>
#அந்தர்பாவ <cmath>
#அந்தர்பாவ <limits>
#அந்தர்பாவ <vector>

class AstEnumDType;
class AstEnumItem;
class Ast_முனை;
class Ast_முனை_DType;
class AstSFormatArg;
class File_வரி;

//============================================================================

class VFormatAttr final {
பொது:
    எண்ணிக்கை en : எழுத்து {
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
    எண்ணிக்கை en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(முழு _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() மாறிலி { திரும்பு m_e; }
    எழுத்து ascii() மாறிலி { திரும்பு m_e; }
    bool isComplex() மாறிலி { திரும்பு m_e == COMPLEX; }
    bool isDouble() மாறிலி { திரும்பு m_e == DOUBLE; }
    bool isEnum() மாறிலி { திரும்பு m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() மாறிலி { திரும்பு m_e == SIGNED; }
    bool isString() மாறிலி { திரும்பு m_e == STRING; }
    bool isUnsigned() மாறிலி { திரும்பு m_e == UNSIGNED; }
};
constexpr bool operator==(மாறிலி VFormatAttr& lhs, மாறிலி VFormatAttr& rhs) {
    திரும்பு lhs.m_e == rhs.m_e;
}
constexpr bool operator==(மாறிலி VFormatAttr& lhs, VFormatAttr::en rhs) { திரும்பு lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, மாறிலி VFormatAttr& rhs) { திரும்பு lhs == rhs.m_e; }
உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி VFormatAttr& rhs) {
    திரும்பு os << rhs.ascii();
}

//============================================================================

class V3_எண்_Data final {
பொது:
    // TYPES
    கட்டமைப்பு மதிப்பு_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_மதிப்பு;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_மதிப்பு_X;
        bool operator==(மாறிலி மதிப்பு_And_X& other) மாறிலி VL_MT_SAFE {
            திரும்பு m_மதிப்பு == other.m_மதிப்பு && m_மதிப்பு_X == other.m_மதிப்பு_X;
        }
    };

    எண்ணிக்கை class V3_எண்_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, மாறிலி V3_எண்_Data_Type& rhs) VL_MT_SAFE {
        தேர்வு (rhs) {
        நிலை V3_எண்_Data_Type::UNINITIALIZED: திரும்பு os << "UNINITIALIZED";
        நிலை V3_எண்_Data_Type::LOGIC: திரும்பு os << "LOGIC";
        நிலை V3_எண்_Data_Type::DOUBLE: திரும்பு os << "DOUBLE";
        நிலை V3_எண்_Data_Type::STRING: திரும்பு os << "STRING";
        }
        திரும்பு os;
    }

தனிப்பட்ட:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    நிலையான constexpr முழு INLINE_சொற்கள் = std::max(
        static_cast<size_t>(2), std::max(அளவு(std::string) / அளவு(மதிப்பு_And_X),
                                         அளவு(std::vector<மதிப்பு_And_X>) / அளவு(மதிப்பு_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    நிலையான constexpr முழு MAX_INLINE_WIDTH = INLINE_சொற்கள் * அளவு(மதிப்பு_And_X) / 2 * 8;

    // MEMBERS
    ஒன்றிப்பு {
        std::array<மதிப்பு_And_X, INLINE_சொற்கள்> m_inline_எண்;
        std::vector<மதிப்பு_And_X> m_dynamic_எண்;
        std::string m_string;
    };

    முழு m_width = 0;  // Width (in bits) as specified/calculated
    V3_எண்_Data_Type m_type;

பொது:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_எண்_Data()
        : m_type{V3_எண்_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_எண்_Data() { destroy_Stored_மதிப்பு(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_எண்_Data(மாறிலி V3_எண்_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        எனில் (other.is_Inline_எண்()) {
            init_Inline_எண்(other.m_inline_எண்);
        } இல்லையெனில் எனில் (other.is_Dynamic_எண்()) {
            init_Dynamic_எண்(other.m_dynamic_எண்);
        } இல்லையெனில் எனில் (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_எண்_Data& operator=(மாறிலி V3_எண்_Data& other) {
        எனில் (other.is_Inline_எண்()) {
            destroy_Stored_மதிப்பு();
            init_Inline_எண்(other.m_inline_எண்);
        } இல்லையெனில் எனில் (other.is_Dynamic_எண்()) {
            reinit_With_Or_Assign_Dynamic_எண்(other.m_dynamic_எண்);
        } இல்லையெனில் எனில் (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } இல்லையெனில் {
            destroy_Stored_மதிப்பு();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        திரும்பு *this;
    }

    V3_எண்_Data(V3_எண்_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        எனில் (other.is_Inline_எண்()) {
            init_Inline_எண்(other.m_inline_எண்);
        } இல்லையெனில் எனில் (other.is_Dynamic_எண்()) {
            init_Dynamic_எண்(std::move(other.m_dynamic_எண்));
        } இல்லையெனில் எனில் (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_எண்_Data_Type::UNINITIALIZED;
    }

    V3_எண்_Data& operator=(V3_எண்_Data&& other) {
        எனில் (other.is_Inline_எண்()) {
            destroy_Stored_மதிப்பு();
            init_Inline_எண்(other.m_inline_எண்);
        } இல்லையெனில் எனில் (other.is_Dynamic_எண்()) {
            reinit_With_Or_Assign_Dynamic_எண்(std::move(other.m_dynamic_எண்));
        } இல்லையெனில் எனில் (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } இல்லையெனில் {
            destroy_Stored_மதிப்பு();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_எண்_Data_Type::UNINITIALIZED;
        திரும்பு *this;
    }

    // ACCESSORS
    மதிப்பு_And_X* இலக்கம்() {
        UASSERT(is_எண்(), "`num` member accessed when data type is " << m_type);
        திரும்பு is_Inline_எண்() ? m_inline_எண்.data() : m_dynamic_எண்.data();
    }
    மாறிலி மதிப்பு_And_X* இலக்கம்() மாறிலி VL_MT_SAFE {
        UASSERT(is_எண்(), "`num` member accessed when data type is " << m_type);
        திரும்பு is_Inline_எண்() ? m_inline_எண்.data() : m_dynamic_எண்.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        திரும்பு m_string;
    }
    மாறிலி std::string& str() மாறிலி VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        திரும்பு m_string;
    }

    முழு width() மாறிலி VL_MT_SAFE { திரும்பு m_width; }
    V3_எண்_Data_Type type() மாறிலி VL_MT_SAFE { திரும்பு m_type; }

    // METHODS
    வெறுமை resize(முழு bits_மொத்தம்) {
        எனில் (m_width == bits_மொத்தம்) திரும்பு;
        எனில் (bits_To_சொற்கள்(m_width) == bits_To_சொற்கள்(bits_மொத்தம்)) {
            m_width = bits_மொத்தம்;
            திரும்பு;
        }
        எனில் (is_Dynamic_எண்()) {
            எனில் (bits_மொத்தம் > MAX_INLINE_WIDTH) {
                m_dynamic_எண்.resize(bits_To_சொற்கள்(bits_மொத்தம்));
            } இல்லையெனில் {
                மாறிலி auto dynamicBits = std::move(m_dynamic_எண்);
                destroy_Dynamic_எண்();
                init_Inline_எண்();
                std::memcpy(m_inline_எண்.data(), dynamicBits.data(), அளவு(m_inline_எண்));
            }
        } இல்லையெனில் எனில் (is_Inline_எண்()) {
            எனில் (bits_மொத்தம் > MAX_INLINE_WIDTH) {
                மாறிலி auto bits = m_inline_எண்;
                init_Dynamic_எண்(bits_To_சொற்கள்(bits_மொத்தம்));
                std::memcpy(m_dynamic_எண்.data(), bits.data(), அளவு(bits));
            }
        }
        m_width = bits_மொத்தம்;
    }

    வெறுமை setString() {
        // If there has been a string already it is kept intact.
        எனில் (isString()) திரும்பு;
        எனில் (is_Dynamic_எண்()) destroy_Dynamic_எண்();
        initString();
        m_type = V3_எண்_Data_Type::STRING;
    }
    வெறுமை setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_எண்_Data_Type::STRING;
    }
    வெறுமை setString(மாறிலி std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_எண்_Data_Type::STRING;
    }

    வெறுமை setDouble() {
        destroy_Stored_மதிப்பு();
        எனில் (!is_Inline_எண்()) init_Inline_எண்();
        m_type = V3_எண்_Data_Type::DOUBLE;
        resize(64);
    }

    வெறுமை setLogic() {
        எனில் (isString()) destroyString();
        எனில் (!is_எண்()) {
            எனில் (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_எண்();
            } இல்லையெனில் {
                init_Dynamic_எண்(bits_To_சொற்கள்(m_width));
            }
        }
        m_type = V3_எண்_Data_Type::LOGIC;
        resize(m_width);
    }

தனிப்பட்ட:
    நிலையான constexpr முழு bits_To_சொற்கள்(முழு bits_மொத்தம்) VL_PURE { திரும்பு (bits_மொத்தம் + 31) / 32; }

    bool is_எண்() மாறிலி VL_MT_SAFE {
        திரும்பு m_type == V3_எண்_Data_Type::DOUBLE || m_type == V3_எண்_Data_Type::LOGIC;
    }
    bool is_Inline_எண்() மாறிலி VL_MT_SAFE {
        திரும்பு (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_எண்_Data_Type::DOUBLE || m_type == V3_எண்_Data_Type::LOGIC);
    }
    bool is_Dynamic_எண்() மாறிலி VL_MT_SAFE {
        திரும்பு (m_width > MAX_INLINE_WIDTH) && (m_type == V3_எண்_Data_Type::LOGIC);
    }
    bool isString() மாறிலி VL_MT_SAFE { திரும்பு m_type == V3_எண்_Data_Type::STRING; }

    template <typename... Args>
    வெறுமை init_Inline_எண்(Args&&... args) {
        new (&m_inline_எண்) std::array<மதிப்பு_And_X, INLINE_சொற்கள்>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    வெறுமை init_Dynamic_எண்(Args&&... args) {
        new (&m_dynamic_எண்) std::vector<மதிப்பு_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    வெறுமை initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    வெறுமை destroy_Dynamic_எண்() { m_dynamic_எண்.~vector(); }
    வெறுமை destroyString() { m_string.~string(); }
    வெறுமை destroy_Stored_மதிப்பு() {
        எனில் (isString())
            destroyString();
        இல்லையெனில் எனில் (is_Dynamic_எண்())
            destroy_Dynamic_எண்();
    }

    template <typename T>
    வெறுமை reinit_With_Or_Assign_Dynamic_எண்(T&& s) {
        எனில் (is_Dynamic_எண்()) {
            m_dynamic_எண் = std::forward<T>(s);
            திரும்பு;
        }
        எனில் (isString()) destroyString();
        init_Dynamic_எண்(std::forward<T>(s));
    }
    template <typename T>
    வெறுமை reinitWithOrAssignString(T&& s) {
        எனில் (isString()) {
            m_string = std::forward<T>(s);
            திரும்பு;
        }
        எனில் (is_Dynamic_எண்()) destroy_Dynamic_எண்();
        initString(std::forward<T>(s));
    }
};

class V3_எண் final {
    // TYPES
    using மதிப்பு_And_X = V3_எண்_Data::மதிப்பு_And_X;
    using V3_எண்_Data_Type = V3_எண்_Data::V3_எண்_Data_Type;

    // MEMBERS
    V3_எண்_Data m_data;
    Ast_முனை* m_nodep = nullptr;  // Parent node - for error reporting only
    File_வரி* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    வெறுமை setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_எண்& setSingleBits(எழுத்து மதிப்பு);
    வெறுமை fillBits(முழு destLsb, முழு width, எழுத்து மதிப்பு);
    வெறுமை copyBits(முழு destLsb, மாறிலி V3_எண்& source, முழு sourceLsb, முழு width);
    V3_எண்& setString(மாறிலி string& str) {
        m_data.setString(str);
        திரும்பு *this;
    }
    வெறுமை opCleanThis(bool warnOnTruncation = false);

பொது:
    வெறுமை nodep(Ast_முனை* nodep) VL_MT_STABLE;
    Ast_முனை* nodep() மாறிலி VL_MT_SAFE { திரும்பு m_nodep; }  // For debug only
    File_வரி* fileline() மாறிலி VL_MT_SAFE { திரும்பு m_fileline; }
    V3_எண்& setZero();
    V3_எண்& setQuad(uint64_t மதிப்பு);
    V3_எண்& setLong(uint32_t மதிப்பு);
    V3_எண்& setLongS(int32_t மதிப்பு);
    V3_எண்& setDouble(இரட்டை மதிப்பு);
    வெறுமை setBitX0(முழு bit);
    வெறுமை setBit(முழு bit, எழுத்து மதிப்பு) {  // Note: must be initialized as number and pre-zeroed!
        எனில் (bit >= m_data.width()) திரும்பு;
        மாறிலி uint32_t mask = (1UL << (bit & 31));
        மதிப்பு_And_X& v = m_data.இலக்கம்()[bit / 32];
        எனில் (மதிப்பு == '0' || மதிப்பு == 0) {
            v.m_மதிப்பு &= ~mask;
            v.m_மதிப்பு_X &= ~mask;
        } இல்லையெனில் எனில் (மதிப்பு == '1' || மதிப்பு == 1) {
            v.m_மதிப்பு |= mask;
            v.m_மதிப்பு_X &= ~mask;
        } இல்லையெனில் எனில் (மதிப்பு == 'z' || மதிப்பு == 2) {
            v.m_மதிப்பு &= ~mask;
            v.m_மதிப்பு_X |= mask;
        } இல்லையெனில் {  // X
            v.m_மதிப்பு |= mask;
            v.m_மதிப்பு_X |= mask;
        }
    }

தனிப்பட்ட:
    எழுத்து bitIs(முழு bit) மாறிலி {
        எனில் (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            திரும்பு '0';
        }
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
        திரும்பு ("01zx"[(((v.m_மதிப்பு & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_மதிப்பு_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    எழுத்து bitIsExtend(முழு bit, முழு lbits) மாறிலி {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        எனில் (bit < 0) திரும்பு '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        எனில் (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
            // We do sign extend
            திரும்பு ("01zx"[(((v.m_மதிப்பு & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_மதிப்பு_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
        திரும்பு ("01zx"[(((v.m_மதிப்பு & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_மதிப்பு_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

பொது:
    bool bitIs0(முழு bit) மாறிலி VL_MT_SAFE {
        எனில் (!is_எண்()) திரும்பு false;
        எனில் (bit < 0) திரும்பு false;
        எனில் (bit >= m_data.width()) திரும்பு !bitIsXZ(m_data.width() - 1);
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
        திரும்பு ((v.m_மதிப்பு | v.m_மதிப்பு_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(முழு bit) மாறிலி VL_MT_SAFE {
        எனில் (!is_எண்()) திரும்பு false;
        எனில் (bit < 0) திரும்பு false;
        எனில் (bit >= m_data.width()) திரும்பு false;
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
        திரும்பு ((v.m_மதிப்பு & (1UL << (bit & 31))) && !(v.m_மதிப்பு_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(முழு bit) மாறிலி {
        // Correct number of zero bits/width matters
        எனில் (!is_எண்()) திரும்பு false;
        எனில் (bit < 0) திரும்பு false;
        எனில் (bit >= m_data.width()) திரும்பு bitIs1Extend(m_data.width() - 1);
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
        திரும்பு ((v.m_மதிப்பு & (1UL << (bit & 31))) && !(v.m_மதிப்பு_X & (1UL << (bit & 31))));
    }
    bool bitIsX(முழு bit) மாறிலி VL_MT_SAFE {
        எனில் (!is_எண்()) திரும்பு false;
        எனில் (bit < 0) திரும்பு false;
        எனில் (bit >= m_data.width()) திரும்பு bitIsZ(m_data.width() - 1);
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
        திரும்பு ((v.m_மதிப்பு & (1UL << (bit & 31))) && (v.m_மதிப்பு_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(முழு bit) மாறிலி VL_MT_SAFE {
        எனில் (!is_எண்()) திரும்பு false;
        எனில் (bit < 0) திரும்பு false;
        எனில் (bit >= m_data.width()) திரும்பு bitIsXZ(m_data.width() - 1);
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
        திரும்பு ((v.m_மதிப்பு_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(முழு bit) மாறிலி VL_MT_SAFE {
        எனில் (!is_எண்()) திரும்பு false;
        எனில் (bit < 0) திரும்பு false;
        எனில் (bit >= m_data.width()) திரும்பு bitIsZ(m_data.width() - 1);
        மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[bit / 32];
        திரும்பு ((~v.m_மதிப்பு & (1UL << (bit & 31))) && (v.m_மதிப்பு_X & (1UL << (bit & 31))));
    }

தனிப்பட்ட:
    uint32_t bits_மதிப்பு(முழு lsb, முழு nbits) மாறிலி VL_MT_SAFE {
        uint32_t v = 0;
        க்கு (முழு bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        திரும்பு v;
    }

    முழு மொத்தம்_X(முழு lsb, முழு nbits) மாறிலி VL_MT_SAFE;
    முழு மொத்தம்_Z(முழு lsb, முழு nbits) மாறிலி VL_MT_SAFE;

    முழு சொற்கள்() மாறிலி VL_MT_SAFE { திரும்பு ((width() + 31) / 32); }
    uint32_t hiWordMask() மாறிலி VL_MT_SAFE {
        // Correct number of zero bits/width matters
        திரும்பு VL_MASK_I(width());
    }

    V3_எண்& opModDivGuts(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, bool is_modulus);

பொது:
    // CONSTRUCTORS
    explicit V3_எண்(Ast_முனை* nodep) { init(nodep, 1); }
    V3_எண்(Ast_முனை* nodep, முழு width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_எண்(Ast_முனை* nodep, முழு width, மாறிலி V3_எண்& மதிப்பு) {
        init(nodep, width, width > 0);
        opAssign(மதிப்பு);
    }
    V3_எண்(Ast_முனை* nodep, முழு width, uint32_t மதிப்பு, bool sized = true) {
        init(nodep, width, sized);
        m_data.இலக்கம்()[0].m_மதிப்பு = மதிப்பு;
        opCleanThis();
    }
    V3_எண்(File_வரி* flp, முழு width, uint32_t மதிப்பு, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.இலக்கம்()[0].m_மதிப்பு = மதிப்பு;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_எண்_Literal {};  // For creator type-overload selection
    V3_எண்(Ast_முனை* nodep, Verilog_எண்_Literal, மாறிலி எழுத்து* sourcep) { create(nodep, sourcep); }
    V3_எண்(File_வரி* flp, Verilog_எண்_Literal, மாறிலி எழுத்து* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_எண்(Ast_முனை* nodep, VerilogStringLiteral, மாறிலி string& str);
    class Double {};
    V3_எண்(Ast_முனை* nodep, Double, இரட்டை மதிப்பு) {
        init(nodep, 64);
        setDouble(மதிப்பு);
    }
    class String {};
    V3_எண்(Ast_முனை* nodep, String, மாறிலி string& மதிப்பு) {
        init(nodep);
        setString(மதிப்பு);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_எண்(Ast_முனை* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_எண்(Ast_முனை* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_எண்(மாறிலி V3_எண்* nump, முழு width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_எண்(மாறிலி V3_எண்* nump, முழு width, uint32_t மதிப்பு) {
        init(nullptr, width);
        m_data.இலக்கம்()[0].m_மதிப்பு = மதிப்பு;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_எண்(Ast_முனை* nodep, மாறிலி Ast_முனை_DType* nodedtypep);

    V3_எண்(மாறிலி V3_எண்& other) = இயல்பு;
    V3_எண்& operator=(மாறிலி V3_எண்& other) = இயல்பு;

    V3_எண்(V3_எண்&& other) = இயல்பு;
    V3_எண்& operator=(V3_எண்&& other) = இயல்பு;

    ~V3_எண்() = இயல்பு;

தனிப்பட்ட:
    வெறுமை selfTestThis();
    வெறுமை create(Ast_முனை* nodep, மாறிலி எழுத்து* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    வெறுமை create(File_வரி* flp, மாறிலி எழுத்து* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    வெறுமை create(மாறிலி எழுத்து* sourcep);
    வெறுமை init(Ast_முனை* nodep, முழு swidth = -1, bool sized = true) {
        this->nodep(nodep);
        எனில் (swidth >= 0) {
            எனில் (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            க்கு (முழு i = 0; i < சொற்கள்(); ++i) m_data.இலக்கம்()[i] = {0, 0};
        } இல்லையெனில் {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    நிலையான string displayPad(size_t fmtsize, எழுத்து pad, bool இடது, மாறிலி string& in) VL_PURE;
    string displayed(மாறிலி string& vformat) மாறிலி VL_MT_STABLE {
        திரும்பு displayed(m_fileline, vformat);
    }
    வெறுமை warnTooMany(மாறிலி string& மதிப்பு);

பொது:
    வெறுமை v3errorEnd(மாறிலி std::ostringstream& sstr) மாறிலி VL_RELEASE(V3Error::s().m_mutex);
    வெறுமை v3errorEndFatal(மாறிலி std::ostringstream& sstr) மாறிலி VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    வெறுமை width(முழு width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_எண்& setAllBitsXRemoved();
    V3_எண்& setAllBitsX();
    V3_எண்& setAllBitsZ();
    V3_எண்& setAllBits0();
    V3_எண்& setAllBits1();
    V3_எண்& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_எண்& setMask(முழு nbits, முழு lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) மாறிலி VL_MT_STABLE;
    string displayed(மாறிலி Ast_முனை* nodep, மாறிலி string& vformat,
                     மாறிலி VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) மாறிலி VL_MT_STABLE;
    string displayed(File_வரி* fl, மாறிலி string& vformat,
                     மாறிலி VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) மாறிலி VL_MT_STABLE;
    string displayedEnum(மாறிலி AstSFormatArg* argp, மாறிலி string& vformat) மாறிலி VL_MT_STABLE;
    string displayedEnumName(மாறிலி AstEnumDType* dtypep) மாறிலி VL_MT_STABLE;
    நிலையான string displayedEnumName(மாறிலி AstEnumItem* itemp);
    நிலையான bool displayedFmtHasArg(எழுத்து format, bool isScan);
    string emitC() மாறிலி VL_MT_STABLE;
    முழு width() மாறிலி VL_MT_SAFE { திரும்பு m_data.width(); }
    முழு widthToFit() மாறிலி;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() மாறிலி VL_MT_SAFE { திரும்பு m_data.m_sized; }
    bool autoExtend() மாறிலி VL_MT_SAFE { திரும்பு m_data.m_autoExtend; }
    bool isFromString() மாறிலி { திரும்பு m_data.m_fromString; }
    bool hasOrigParamName() மாறிலி { திரும்பு m_data.m_hasOrigParamName; }
    வெறுமை hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_எண்_Data_Type dataType() மாறிலி VL_MT_SAFE { திரும்பு m_data.type(); }
    வெறுமை dataType(V3_எண்_Data_Type newType) {
        எனில் (dataType() == newType) திரும்பு;
        UASSERT(newType != V3_எண்_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        தேர்வு (newType) {
        நிலை V3_எண்_Data_Type::STRING: m_data.setString(); நிறுத்து;
        நிலை V3_எண்_Data_Type::DOUBLE: m_data.setDouble(); நிறுத்து;
        நிலை V3_எண்_Data_Type::LOGIC: m_data.setLogic(); நிறுத்து;
        நிலை V3_எண்_Data_Type::UNINITIALIZED: நிறுத்து;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() மாறிலி VL_MT_SAFE { திரும்பு m_data.m_signed; }
    வெறுமை isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() மாறிலி VL_MT_SAFE { திரும்பு dataType() == V3_எண்_Data_Type::DOUBLE; }
    bool isString() மாறிலி VL_MT_SAFE { திரும்பு dataType() == V3_எண்_Data_Type::STRING; }
    bool isOpaque() மாறிலி VL_MT_SAFE { திரும்பு isDouble() || isString(); }
    bool is_எண்() மாறிலி VL_MT_SAFE {
        திரும்பு m_data.type() == V3_எண்_Data_Type::LOGIC
               || m_data.type() == V3_எண்_Data_Type::DOUBLE;
    }
    bool isNegative() மாறிலி VL_MT_SAFE {
        // Correct number of zero bits/width matters
        திரும்பு !isString() && bitIs1(width() - 1);
    }
    bool is1Step() மாறிலி VL_MT_SAFE { திரும்பு m_data.m_is1Step; }
    bool isNull() மாறிலி VL_MT_SAFE { திரும்பு m_data.m_isNull; }
    bool isFourState() மாறிலி VL_MT_SAFE;
    bool hasZ() மாறிலி {
        எனில் (isString()) திரும்பு false;
        க்கு (முழு i = 0; i < சொற்கள்(); ++i) {
            மாறிலி மதிப்பு_And_X v = m_data.இலக்கம்()[i];
            எனில் ((~v.m_மதிப்பு) & v.m_மதிப்பு_X) திரும்பு true;
        }
        திரும்பு false;
    }
    bool isAllZ() மாறிலி VL_MT_SAFE;
    bool isAllX() மாறிலி VL_MT_SAFE;
    bool isEqZero() மாறிலி VL_MT_SAFE;
    bool isNeqZero() மாறிலி;
    bool isBitsZero(முழு msb, முழு lsb) மாறிலி;
    bool isBroken(முழு vwidth) மாறிலி;
    bool isEqOne() மாறிலி;
    bool isEqAllOnes(முழு optwidth = 0) மாறிலி;
    bool isCaseEq(மாறிலி V3_எண்& rhs) மாறிலி;  // operator==
    bool isLtXZ(மாறிலி V3_எண்& rhs) மாறிலி;  // operator< with XZ compared
    bool isAnyX() மாறிலி VL_MT_SAFE;
    bool isAnyXZ() மாறிலி;
    bool isAnyZ() மாறிலி VL_MT_SAFE;
    bool isMsbXZ() மாறிலி {
        // Correct number of zero bits/width matters
        திரும்பு bitIsXZ(width() - 1);
    }
    bool fitsInUInt() மாறிலி VL_MT_SAFE;
    uint32_t toUInt() மாறிலி VL_MT_SAFE;
    int32_t toSInt() மாறிலி VL_MT_SAFE;
    uint64_t toUQuad() மாறிலி VL_MT_SAFE;
    int64_t toSQuad() மாறிலி VL_MT_SAFE;
    string toString() மாறிலி VL_MT_SAFE;
    string toDecimalS() மாறிலி VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() மாறிலி VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() மாறிலி VL_MT_STABLE;
    இரட்டை toDouble() மாறிலி VL_MT_SAFE;
    V3Hash toHash() மாறிலி;
    uint32_t edataWord(முழு eword) மாறிலி;
    uint8_t dataByte(முழு byte) மாறிலி;
    uint32_t மொத்தம்_Bits(மாறிலி V3_எண்& ctrl) மாறிலி;
    uint32_t மொத்தம்_Bits(மாறிலி V3_எண்& ctrl1, மாறிலி V3_எண்& ctrl2, மாறிலி V3_எண்& ctrl3) மாறிலி;
    uint32_t மொத்தம்_Ones() மாறிலி;
    uint32_t mostSetBitP1() மாறிலி;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() மாறிலி;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(மாறிலி V3_எண்& rhs) மாறிலி { திரும்பு isLtXZ(rhs); }

    // STATICS
    நிலையான bool epsilonEqual(இரட்டை a, இரட்டை b);  // True if number within Epsilon of the other
    நிலையான bool epsilonIntegral(இரட்டை a);  // True if number rounds to integer within Epsilon
    நிலையான முழு log2b(uint32_t இலக்கம்);
    நிலையான முழு log2bQuad(uint64_t இலக்கம்);
    நிலையான வெறுமை selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_எண்& opBitsNonXZ(மாறிலி V3_எண்& lhs);  // 0/1->1, X/Z->0
    V3_எண்& opBitsOne(மாறிலி V3_எண்& lhs);  // 1->1, 0/X/Z->0
    V3_எண்& opBitsXZ(மாறிலி V3_எண்& lhs);  // 0/1->0, X/Z->1
    V3_எண்& opBitsZ(மாறிலி V3_எண்& lhs);  // Z->1, 0/1/X->0
    //
    V3_எண்& opAssign(மாறிலி V3_எண்& lhs);
    V3_எண்& opAssignNonXZ(மாறிலி V3_எண்& lhs, bool ignoreXZ = true);
    V3_எண்& opExtendS(மாறிலி V3_எண்& lhs, uint32_t lbits);  // Sign extension
    V3_எண்& opExtendXZ(மாறிலி V3_எண்& lhs, uint32_t lbits);  // X/Z extension
    V3_எண்& opRedOr(மாறிலி V3_எண்& lhs);
    V3_எண்& opRedAnd(மாறிலி V3_எண்& lhs);
    V3_எண்& opRedXor(மாறிலி V3_எண்& lhs);
    V3_எண்& op_மொத்தம்_Bits(மாறிலி V3_எண்& கோவை, மாறிலி V3_எண்& ctrl1, மாறிலி V3_எண்& ctrl2,
                          மாறிலி V3_எண்& ctrl3);
    V3_எண்& op_மொத்தம்_Ones(மாறிலி V3_எண்& lhs);
    V3_எண்& opIsUnknown(மாறிலி V3_எண்& lhs);
    V3_எண்& opOneHot(மாறிலி V3_எண்& lhs);
    V3_எண்& opOneHot0(மாறிலி V3_எண்& lhs);
    V3_எண்& opCLog2(மாறிலி V3_எண்& lhs);
    V3_எண்& opMostSetBitP1(மாறிலி V3_எண்& lhs);
    V3_எண்& opClean(மாறிலி V3_எண்& lhs, uint32_t bits);
    V3_எண்& opConcat(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLenN(மாறிலி V3_எண்& lhs);
    V3_எண்& opRepl(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opRepl(மாறிலி V3_எண்& lhs, uint32_t rhsval);
    V3_எண்& opStreamL(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opSel(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& msb, மாறிலி V3_எண்& lsb);
    V3_எண்& opSel(மாறிலி V3_எண்& lhs, uint32_t msbval, uint32_t lsbval);
    V3_எண்& opSelInto(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& lsb, முழு width);
    V3_எண்& opSelInto(மாறிலி V3_எண்& lhs, முழு lsbval, முழு width);
    V3_எண்& opCvtPackString(மாறிலி V3_எண்& lhs);
    V3_எண்& opToLowerN(மாறிலி V3_எண்& lhs);
    V3_எண்& opToUpperN(மாறிலி V3_எண்& lhs);
    V3_எண்& opCaseEq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opCaseNeq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opWildEq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opWildNeq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opBufIf1(மாறிலி V3_எண்& ens, மாறிலி V3_எண்& if1s);
    V3_எண்& opSetRange(uint32_t lsb, uint32_t width, எழுத்து bit_மதிப்பு);
    // "standard" math
    V3_எண்& opNot(மாறிலி V3_எண்& lhs);
    V3_எண்& opLogNot(மாறிலி V3_எண்& lhs);
    V3_எண்& opLogAnd(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLogOr(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLogEq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLogIf(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opNegate(மாறிலி V3_எண்& lhs);
    V3_எண்& opAdd(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opSub(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opMul(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opMulS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed
    V3_எண்& opDiv(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opDivS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed
    V3_எண்& opModDiv(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opModDivS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed
    V3_எண்& opPow(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, bool lsign = false,
                    bool rsign = false);
    V3_எண்& opPowSU(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed lhs, unsigned rhs
    V3_எண்& opPowSS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed lhs, signed rhs
    V3_எண்& opPowUS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Unsigned lhs, signed rhs
    V3_எண்& opAnd(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opXor(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opOr(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opShiftR(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opShiftRS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_எண்& opShiftL(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    // Comparisons
    V3_எண்& opEq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opNeq(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opGt(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opGtS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed
    V3_எண்& opGte(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opGteS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed
    V3_எண்& opLt(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLtS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed
    V3_எண்& opLte(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLteS(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);  // Signed

    // "D" - double (aka real) math
    V3_எண்& opIToRD(மாறிலி V3_எண்& lhs, bool isSigned = false);
    V3_எண்& opISToRD(மாறிலி V3_எண்& lhs) { திரும்பு opIToRD(lhs, true); }
    V3_எண்& opRToIS(மாறிலி V3_எண்& lhs);
    V3_எண்& opRToIRoundS(மாறிலி V3_எண்& lhs);
    V3_எண்& opRealToBits(மாறிலி V3_எண்& lhs);
    V3_எண்& opBitsToRealD(மாறிலி V3_எண்& lhs);
    V3_எண்& opNegateD(மாறிலி V3_எண்& lhs);
    V3_எண்& opAddD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opSubD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opMulD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opDivD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opPowD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    // Comparisons
    V3_எண்& opEqD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opNeqD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opGtD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opGteD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLtD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLteD(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);

    // "N" - string operations
    V3_எண்& opAtoN(மாறிலி V3_எண்& lhs, முழு base);
    V3_எண்& opNToI(மாறிலி V3_எண்& lhs);
    V3_எண்& opPutcN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, மாறிலி V3_எண்& ths);
    V3_எண்& opGetcN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opSubstrN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, மாறிலி V3_எண்& ths);
    V3_எண்& opCompareNN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs, bool ignoreCase);
    V3_எண்& opConcatN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opReplN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opReplN(மாறிலி V3_எண்& lhs, uint32_t rhsval);
    V3_எண்& opEqN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opNeqN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opGtN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opGteN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLtN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
    V3_எண்& opLteN(மாறிலி V3_எண்& lhs, மாறிலி V3_எண்& rhs);
};
உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி V3_எண்& rhs) VL_MT_SAFE {
    திரும்பு os << rhs.ascii();
}

#endif  // Guard
