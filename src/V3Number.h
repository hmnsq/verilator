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

#ifndef VERILATOR_V3_عدد_H
#define VERILATOR_V3_عدد_H

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3Hash.h"
#شامل "V3StdFuture.h"

#شامل <algorithm>
#شامل <array>
#شامل <cmath>
#شامل <limits>
#شامل <vector>

class AstEnumDType;
class AstEnumItem;
class Ast_گره;
class Ast_گره_DType;
class AstSFormatArg;
class File_سطر;

//============================================================================

class VFormatAttr final {
عمومی:
    شمارش en : نویسه {
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
    شمارش en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    نویسه ascii() ثابت { بازگشت m_e; }
    bool isComplex() ثابت { بازگشت m_e == COMPLEX; }
    bool isDouble() ثابت { بازگشت m_e == DOUBLE; }
    bool isEnum() ثابت { بازگشت m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ثابت { بازگشت m_e == SIGNED; }
    bool isString() ثابت { بازگشت m_e == STRING; }
    bool isUnsigned() ثابت { بازگشت m_e == UNSIGNED; }
};
constexpr bool operator==(ثابت VFormatAttr& lhs, ثابت VFormatAttr& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VFormatAttr& lhs, VFormatAttr::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, ثابت VFormatAttr& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VFormatAttr& rhs) {
    بازگشت os << rhs.ascii();
}

//============================================================================

class V3_عدد_Data final {
عمومی:
    // TYPES
    ساختار مقدار_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_مقدار;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_مقدار_X;
        bool operator==(ثابت مقدار_And_X& other) ثابت VL_MT_SAFE {
            بازگشت m_مقدار == other.m_مقدار && m_مقدار_X == other.m_مقدار_X;
        }
    };

    شمارش class V3_عدد_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ثابت V3_عدد_Data_Type& rhs) VL_MT_SAFE {
        انتخاب (rhs) {
        حالت V3_عدد_Data_Type::UNINITIALIZED: بازگشت os << "UNINITIALIZED";
        حالت V3_عدد_Data_Type::LOGIC: بازگشت os << "LOGIC";
        حالت V3_عدد_Data_Type::DOUBLE: بازگشت os << "DOUBLE";
        حالت V3_عدد_Data_Type::STRING: بازگشت os << "STRING";
        }
        بازگشت os;
    }

خصوصی:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ایستا constexpr صحیح INLINE_واژگان = std::max(
        static_cast<size_t>(2), std::max(اندازه(std::string) / اندازه(مقدار_And_X),
                                         اندازه(std::vector<مقدار_And_X>) / اندازه(مقدار_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ایستا constexpr صحیح MAX_INLINE_WIDTH = INLINE_واژگان * اندازه(مقدار_And_X) / 2 * 8;

    // MEMBERS
    اجتماع {
        std::array<مقدار_And_X, INLINE_واژگان> m_inline_عدد;
        std::vector<مقدار_And_X> m_dynamic_عدد;
        std::string m_string;
    };

    صحیح m_width = 0;  // Width (in bits) as specified/calculated
    V3_عدد_Data_Type m_type;

عمومی:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_عدد_Data()
        : m_type{V3_عدد_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_عدد_Data() { destroy_Stored_مقدار(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_عدد_Data(ثابت V3_عدد_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        اگر (other.is_Inline_عدد()) {
            init_Inline_عدد(other.m_inline_عدد);
        } وگرنه اگر (other.is_Dynamic_عدد()) {
            init_Dynamic_عدد(other.m_dynamic_عدد);
        } وگرنه اگر (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_عدد_Data& operator=(ثابت V3_عدد_Data& other) {
        اگر (other.is_Inline_عدد()) {
            destroy_Stored_مقدار();
            init_Inline_عدد(other.m_inline_عدد);
        } وگرنه اگر (other.is_Dynamic_عدد()) {
            reinit_With_Or_Assign_Dynamic_عدد(other.m_dynamic_عدد);
        } وگرنه اگر (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } وگرنه {
            destroy_Stored_مقدار();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        بازگشت *this;
    }

    V3_عدد_Data(V3_عدد_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        اگر (other.is_Inline_عدد()) {
            init_Inline_عدد(other.m_inline_عدد);
        } وگرنه اگر (other.is_Dynamic_عدد()) {
            init_Dynamic_عدد(std::move(other.m_dynamic_عدد));
        } وگرنه اگر (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_عدد_Data_Type::UNINITIALIZED;
    }

    V3_عدد_Data& operator=(V3_عدد_Data&& other) {
        اگر (other.is_Inline_عدد()) {
            destroy_Stored_مقدار();
            init_Inline_عدد(other.m_inline_عدد);
        } وگرنه اگر (other.is_Dynamic_عدد()) {
            reinit_With_Or_Assign_Dynamic_عدد(std::move(other.m_dynamic_عدد));
        } وگرنه اگر (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } وگرنه {
            destroy_Stored_مقدار();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_عدد_Data_Type::UNINITIALIZED;
        بازگشت *this;
    }

    // ACCESSORS
    مقدار_And_X* رقم() {
        UASSERT(is_عدد(), "`num` member accessed when data type is " << m_type);
        بازگشت is_Inline_عدد() ? m_inline_عدد.data() : m_dynamic_عدد.data();
    }
    ثابت مقدار_And_X* رقم() ثابت VL_MT_SAFE {
        UASSERT(is_عدد(), "`num` member accessed when data type is " << m_type);
        بازگشت is_Inline_عدد() ? m_inline_عدد.data() : m_dynamic_عدد.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        بازگشت m_string;
    }
    ثابت std::string& str() ثابت VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        بازگشت m_string;
    }

    صحیح width() ثابت VL_MT_SAFE { بازگشت m_width; }
    V3_عدد_Data_Type type() ثابت VL_MT_SAFE { بازگشت m_type; }

    // METHODS
    تهی resize(صحیح bits_شمار) {
        اگر (m_width == bits_شمار) بازگشت;
        اگر (bits_To_واژگان(m_width) == bits_To_واژگان(bits_شمار)) {
            m_width = bits_شمار;
            بازگشت;
        }
        اگر (is_Dynamic_عدد()) {
            اگر (bits_شمار > MAX_INLINE_WIDTH) {
                m_dynamic_عدد.resize(bits_To_واژگان(bits_شمار));
            } وگرنه {
                ثابت auto dynamicBits = std::move(m_dynamic_عدد);
                destroy_Dynamic_عدد();
                init_Inline_عدد();
                std::memcpy(m_inline_عدد.data(), dynamicBits.data(), اندازه(m_inline_عدد));
            }
        } وگرنه اگر (is_Inline_عدد()) {
            اگر (bits_شمار > MAX_INLINE_WIDTH) {
                ثابت auto bits = m_inline_عدد;
                init_Dynamic_عدد(bits_To_واژگان(bits_شمار));
                std::memcpy(m_dynamic_عدد.data(), bits.data(), اندازه(bits));
            }
        }
        m_width = bits_شمار;
    }

    تهی setString() {
        // If there has been a string already it is kept intact.
        اگر (isString()) بازگشت;
        اگر (is_Dynamic_عدد()) destroy_Dynamic_عدد();
        initString();
        m_type = V3_عدد_Data_Type::STRING;
    }
    تهی setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_عدد_Data_Type::STRING;
    }
    تهی setString(ثابت std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_عدد_Data_Type::STRING;
    }

    تهی setDouble() {
        destroy_Stored_مقدار();
        اگر (!is_Inline_عدد()) init_Inline_عدد();
        m_type = V3_عدد_Data_Type::DOUBLE;
        resize(64);
    }

    تهی setLogic() {
        اگر (isString()) destroyString();
        اگر (!is_عدد()) {
            اگر (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_عدد();
            } وگرنه {
                init_Dynamic_عدد(bits_To_واژگان(m_width));
            }
        }
        m_type = V3_عدد_Data_Type::LOGIC;
        resize(m_width);
    }

خصوصی:
    ایستا constexpr صحیح bits_To_واژگان(صحیح bits_شمار) VL_PURE { بازگشت (bits_شمار + 31) / 32; }

    bool is_عدد() ثابت VL_MT_SAFE {
        بازگشت m_type == V3_عدد_Data_Type::DOUBLE || m_type == V3_عدد_Data_Type::LOGIC;
    }
    bool is_Inline_عدد() ثابت VL_MT_SAFE {
        بازگشت (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_عدد_Data_Type::DOUBLE || m_type == V3_عدد_Data_Type::LOGIC);
    }
    bool is_Dynamic_عدد() ثابت VL_MT_SAFE {
        بازگشت (m_width > MAX_INLINE_WIDTH) && (m_type == V3_عدد_Data_Type::LOGIC);
    }
    bool isString() ثابت VL_MT_SAFE { بازگشت m_type == V3_عدد_Data_Type::STRING; }

    template <typename... Args>
    تهی init_Inline_عدد(Args&&... args) {
        new (&m_inline_عدد) std::array<مقدار_And_X, INLINE_واژگان>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    تهی init_Dynamic_عدد(Args&&... args) {
        new (&m_dynamic_عدد) std::vector<مقدار_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    تهی initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    تهی destroy_Dynamic_عدد() { m_dynamic_عدد.~vector(); }
    تهی destroyString() { m_string.~string(); }
    تهی destroy_Stored_مقدار() {
        اگر (isString())
            destroyString();
        وگرنه اگر (is_Dynamic_عدد())
            destroy_Dynamic_عدد();
    }

    template <typename T>
    تهی reinit_With_Or_Assign_Dynamic_عدد(T&& s) {
        اگر (is_Dynamic_عدد()) {
            m_dynamic_عدد = std::forward<T>(s);
            بازگشت;
        }
        اگر (isString()) destroyString();
        init_Dynamic_عدد(std::forward<T>(s));
    }
    template <typename T>
    تهی reinitWithOrAssignString(T&& s) {
        اگر (isString()) {
            m_string = std::forward<T>(s);
            بازگشت;
        }
        اگر (is_Dynamic_عدد()) destroy_Dynamic_عدد();
        initString(std::forward<T>(s));
    }
};

class V3_عدد final {
    // TYPES
    using مقدار_And_X = V3_عدد_Data::مقدار_And_X;
    using V3_عدد_Data_Type = V3_عدد_Data::V3_عدد_Data_Type;

    // MEMBERS
    V3_عدد_Data m_data;
    Ast_گره* m_nodep = nullptr;  // Parent node - for error reporting only
    File_سطر* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    تهی setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_عدد& setSingleBits(نویسه مقدار);
    تهی fillBits(صحیح destLsb, صحیح width, نویسه مقدار);
    تهی copyBits(صحیح destLsb, ثابت V3_عدد& source, صحیح sourceLsb, صحیح width);
    V3_عدد& setString(ثابت string& str) {
        m_data.setString(str);
        بازگشت *this;
    }
    تهی opCleanThis(bool warnOnTruncation = false);

عمومی:
    تهی nodep(Ast_گره* nodep) VL_MT_STABLE;
    Ast_گره* nodep() ثابت VL_MT_SAFE { بازگشت m_nodep; }  // For debug only
    File_سطر* fileline() ثابت VL_MT_SAFE { بازگشت m_fileline; }
    V3_عدد& setZero();
    V3_عدد& setQuad(uint64_t مقدار);
    V3_عدد& setLong(uint32_t مقدار);
    V3_عدد& setLongS(int32_t مقدار);
    V3_عدد& setDouble(دوگانه مقدار);
    تهی setBitX0(صحیح bit);
    تهی setBit(صحیح bit, نویسه مقدار) {  // Note: must be initialized as number and pre-zeroed!
        اگر (bit >= m_data.width()) بازگشت;
        ثابت uint32_t mask = (1UL << (bit & 31));
        مقدار_And_X& v = m_data.رقم()[bit / 32];
        اگر (مقدار == '0' || مقدار == 0) {
            v.m_مقدار &= ~mask;
            v.m_مقدار_X &= ~mask;
        } وگرنه اگر (مقدار == '1' || مقدار == 1) {
            v.m_مقدار |= mask;
            v.m_مقدار_X &= ~mask;
        } وگرنه اگر (مقدار == 'z' || مقدار == 2) {
            v.m_مقدار &= ~mask;
            v.m_مقدار_X |= mask;
        } وگرنه {  // X
            v.m_مقدار |= mask;
            v.m_مقدار_X |= mask;
        }
    }

خصوصی:
    نویسه bitIs(صحیح bit) ثابت {
        اگر (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            بازگشت '0';
        }
        ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
        بازگشت ("01zx"[(((v.m_مقدار & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_مقدار_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    نویسه bitIsExtend(صحیح bit, صحیح lbits) ثابت {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        اگر (bit < 0) بازگشت '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        اگر (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
            // We do sign extend
            بازگشت ("01zx"[(((v.m_مقدار & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_مقدار_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
        بازگشت ("01zx"[(((v.m_مقدار & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_مقدار_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

عمومی:
    bool bitIs0(صحیح bit) ثابت VL_MT_SAFE {
        اگر (!is_عدد()) بازگشت false;
        اگر (bit < 0) بازگشت false;
        اگر (bit >= m_data.width()) بازگشت !bitIsXZ(m_data.width() - 1);
        ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
        بازگشت ((v.m_مقدار | v.m_مقدار_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(صحیح bit) ثابت VL_MT_SAFE {
        اگر (!is_عدد()) بازگشت false;
        اگر (bit < 0) بازگشت false;
        اگر (bit >= m_data.width()) بازگشت false;
        ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
        بازگشت ((v.m_مقدار & (1UL << (bit & 31))) && !(v.m_مقدار_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(صحیح bit) ثابت {
        // Correct number of zero bits/width matters
        اگر (!is_عدد()) بازگشت false;
        اگر (bit < 0) بازگشت false;
        اگر (bit >= m_data.width()) بازگشت bitIs1Extend(m_data.width() - 1);
        ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
        بازگشت ((v.m_مقدار & (1UL << (bit & 31))) && !(v.m_مقدار_X & (1UL << (bit & 31))));
    }
    bool bitIsX(صحیح bit) ثابت VL_MT_SAFE {
        اگر (!is_عدد()) بازگشت false;
        اگر (bit < 0) بازگشت false;
        اگر (bit >= m_data.width()) بازگشت bitIsZ(m_data.width() - 1);
        ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
        بازگشت ((v.m_مقدار & (1UL << (bit & 31))) && (v.m_مقدار_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(صحیح bit) ثابت VL_MT_SAFE {
        اگر (!is_عدد()) بازگشت false;
        اگر (bit < 0) بازگشت false;
        اگر (bit >= m_data.width()) بازگشت bitIsXZ(m_data.width() - 1);
        ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
        بازگشت ((v.m_مقدار_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(صحیح bit) ثابت VL_MT_SAFE {
        اگر (!is_عدد()) بازگشت false;
        اگر (bit < 0) بازگشت false;
        اگر (bit >= m_data.width()) بازگشت bitIsZ(m_data.width() - 1);
        ثابت مقدار_And_X v = m_data.رقم()[bit / 32];
        بازگشت ((~v.m_مقدار & (1UL << (bit & 31))) && (v.m_مقدار_X & (1UL << (bit & 31))));
    }

خصوصی:
    uint32_t bits_مقدار(صحیح lsb, صحیح nbits) ثابت VL_MT_SAFE {
        uint32_t v = 0;
        برای (صحیح bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        بازگشت v;
    }

    صحیح شمار_X(صحیح lsb, صحیح nbits) ثابت VL_MT_SAFE;
    صحیح شمار_Z(صحیح lsb, صحیح nbits) ثابت VL_MT_SAFE;

    صحیح واژگان() ثابت VL_MT_SAFE { بازگشت ((width() + 31) / 32); }
    uint32_t hiWordMask() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        بازگشت VL_MASK_I(width());
    }

    V3_عدد& opModDivGuts(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, bool is_modulus);

عمومی:
    // CONSTRUCTORS
    explicit V3_عدد(Ast_گره* nodep) { init(nodep, 1); }
    V3_عدد(Ast_گره* nodep, صحیح width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_عدد(Ast_گره* nodep, صحیح width, ثابت V3_عدد& مقدار) {
        init(nodep, width, width > 0);
        opAssign(مقدار);
    }
    V3_عدد(Ast_گره* nodep, صحیح width, uint32_t مقدار, bool sized = true) {
        init(nodep, width, sized);
        m_data.رقم()[0].m_مقدار = مقدار;
        opCleanThis();
    }
    V3_عدد(File_سطر* flp, صحیح width, uint32_t مقدار, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.رقم()[0].m_مقدار = مقدار;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_عدد_Literal {};  // For creator type-overload selection
    V3_عدد(Ast_گره* nodep, Verilog_عدد_Literal, ثابت نویسه* sourcep) { create(nodep, sourcep); }
    V3_عدد(File_سطر* flp, Verilog_عدد_Literal, ثابت نویسه* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_عدد(Ast_گره* nodep, VerilogStringLiteral, ثابت string& str);
    class Double {};
    V3_عدد(Ast_گره* nodep, Double, دوگانه مقدار) {
        init(nodep, 64);
        setDouble(مقدار);
    }
    class String {};
    V3_عدد(Ast_گره* nodep, String, ثابت string& مقدار) {
        init(nodep);
        setString(مقدار);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_عدد(Ast_گره* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_عدد(Ast_گره* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_عدد(ثابت V3_عدد* nump, صحیح width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_عدد(ثابت V3_عدد* nump, صحیح width, uint32_t مقدار) {
        init(nullptr, width);
        m_data.رقم()[0].m_مقدار = مقدار;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_عدد(Ast_گره* nodep, ثابت Ast_گره_DType* nodedtypep);

    V3_عدد(ثابت V3_عدد& other) = پیشفرض;
    V3_عدد& operator=(ثابت V3_عدد& other) = پیشفرض;

    V3_عدد(V3_عدد&& other) = پیشفرض;
    V3_عدد& operator=(V3_عدد&& other) = پیشفرض;

    ~V3_عدد() = پیشفرض;

خصوصی:
    تهی selfTestThis();
    تهی create(Ast_گره* nodep, ثابت نویسه* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    تهی create(File_سطر* flp, ثابت نویسه* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    تهی create(ثابت نویسه* sourcep);
    تهی init(Ast_گره* nodep, صحیح swidth = -1, bool sized = true) {
        this->nodep(nodep);
        اگر (swidth >= 0) {
            اگر (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            برای (صحیح i = 0; i < واژگان(); ++i) m_data.رقم()[i] = {0, 0};
        } وگرنه {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    ایستا string displayPad(size_t fmtsize, نویسه pad, bool چپ, ثابت string& in) VL_PURE;
    string displayed(ثابت string& vformat) ثابت VL_MT_STABLE {
        بازگشت displayed(m_fileline, vformat);
    }
    تهی warnTooMany(ثابت string& مقدار);

عمومی:
    تهی v3errorEnd(ثابت std::ostringstream& sstr) ثابت VL_RELEASE(V3Error::s().m_mutex);
    تهی v3errorEndFatal(ثابت std::ostringstream& sstr) ثابت VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    تهی width(صحیح width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_عدد& setAllBitsXRemoved();
    V3_عدد& setAllBitsX();
    V3_عدد& setAllBitsZ();
    V3_عدد& setAllBits0();
    V3_عدد& setAllBits1();
    V3_عدد& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_عدد& setMask(صحیح nbits, صحیح lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ثابت VL_MT_STABLE;
    string displayed(ثابت Ast_گره* nodep, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayed(File_سطر* fl, ثابت string& vformat,
                     ثابت VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ثابت VL_MT_STABLE;
    string displayedEnum(ثابت AstSFormatArg* argp, ثابت string& vformat) ثابت VL_MT_STABLE;
    string displayedEnumName(ثابت AstEnumDType* dtypep) ثابت VL_MT_STABLE;
    ایستا string displayedEnumName(ثابت AstEnumItem* itemp);
    ایستا bool displayedFmtHasArg(نویسه format, bool isScan);
    string emitC() ثابت VL_MT_STABLE;
    صحیح width() ثابت VL_MT_SAFE { بازگشت m_data.width(); }
    صحیح widthToFit() ثابت;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ثابت VL_MT_SAFE { بازگشت m_data.m_sized; }
    bool autoExtend() ثابت VL_MT_SAFE { بازگشت m_data.m_autoExtend; }
    bool isFromString() ثابت { بازگشت m_data.m_fromString; }
    bool hasOrigParamName() ثابت { بازگشت m_data.m_hasOrigParamName; }
    تهی hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_عدد_Data_Type dataType() ثابت VL_MT_SAFE { بازگشت m_data.type(); }
    تهی dataType(V3_عدد_Data_Type newType) {
        اگر (dataType() == newType) بازگشت;
        UASSERT(newType != V3_عدد_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        انتخاب (newType) {
        حالت V3_عدد_Data_Type::STRING: m_data.setString(); بشکن;
        حالت V3_عدد_Data_Type::DOUBLE: m_data.setDouble(); بشکن;
        حالت V3_عدد_Data_Type::LOGIC: m_data.setLogic(); بشکن;
        حالت V3_عدد_Data_Type::UNINITIALIZED: بشکن;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ثابت VL_MT_SAFE { بازگشت m_data.m_signed; }
    تهی isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ثابت VL_MT_SAFE { بازگشت dataType() == V3_عدد_Data_Type::DOUBLE; }
    bool isString() ثابت VL_MT_SAFE { بازگشت dataType() == V3_عدد_Data_Type::STRING; }
    bool isOpaque() ثابت VL_MT_SAFE { بازگشت isDouble() || isString(); }
    bool is_عدد() ثابت VL_MT_SAFE {
        بازگشت m_data.type() == V3_عدد_Data_Type::LOGIC
               || m_data.type() == V3_عدد_Data_Type::DOUBLE;
    }
    bool isNegative() ثابت VL_MT_SAFE {
        // Correct number of zero bits/width matters
        بازگشت !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ثابت VL_MT_SAFE { بازگشت m_data.m_is1Step; }
    bool isNull() ثابت VL_MT_SAFE { بازگشت m_data.m_isNull; }
    bool isFourState() ثابت VL_MT_SAFE;
    bool hasZ() ثابت {
        اگر (isString()) بازگشت false;
        برای (صحیح i = 0; i < واژگان(); ++i) {
            ثابت مقدار_And_X v = m_data.رقم()[i];
            اگر ((~v.m_مقدار) & v.m_مقدار_X) بازگشت true;
        }
        بازگشت false;
    }
    bool isAllZ() ثابت VL_MT_SAFE;
    bool isAllX() ثابت VL_MT_SAFE;
    bool isEqZero() ثابت VL_MT_SAFE;
    bool isNeqZero() ثابت;
    bool isBitsZero(صحیح msb, صحیح lsb) ثابت;
    bool isBroken(صحیح vwidth) ثابت;
    bool isEqOne() ثابت;
    bool isEqAllOnes(صحیح optwidth = 0) ثابت;
    bool isCaseEq(ثابت V3_عدد& rhs) ثابت;  // operator==
    bool isLtXZ(ثابت V3_عدد& rhs) ثابت;  // operator< with XZ compared
    bool isAnyX() ثابت VL_MT_SAFE;
    bool isAnyXZ() ثابت;
    bool isAnyZ() ثابت VL_MT_SAFE;
    bool isMsbXZ() ثابت {
        // Correct number of zero bits/width matters
        بازگشت bitIsXZ(width() - 1);
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
    دوگانه toDouble() ثابت VL_MT_SAFE;
    V3Hash toHash() ثابت;
    uint32_t edataWord(صحیح eword) ثابت;
    uint8_t dataByte(صحیح byte) ثابت;
    uint32_t شمار_Bits(ثابت V3_عدد& ctrl) ثابت;
    uint32_t شمار_Bits(ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2, ثابت V3_عدد& ctrl3) ثابت;
    uint32_t شمار_Ones() ثابت;
    uint32_t mostSetBitP1() ثابت;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ثابت;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ثابت V3_عدد& rhs) ثابت { بازگشت isLtXZ(rhs); }

    // STATICS
    ایستا bool epsilonEqual(دوگانه a, دوگانه b);  // True if number within Epsilon of the other
    ایستا bool epsilonIntegral(دوگانه a);  // True if number rounds to integer within Epsilon
    ایستا صحیح log2b(uint32_t رقم);
    ایستا صحیح log2bQuad(uint64_t رقم);
    ایستا تهی selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_عدد& opBitsNonXZ(ثابت V3_عدد& lhs);  // 0/1->1, X/Z->0
    V3_عدد& opBitsOne(ثابت V3_عدد& lhs);  // 1->1, 0/X/Z->0
    V3_عدد& opBitsXZ(ثابت V3_عدد& lhs);  // 0/1->0, X/Z->1
    V3_عدد& opBitsZ(ثابت V3_عدد& lhs);  // Z->1, 0/1/X->0
    //
    V3_عدد& opAssign(ثابت V3_عدد& lhs);
    V3_عدد& opAssignNonXZ(ثابت V3_عدد& lhs, bool ignoreXZ = true);
    V3_عدد& opExtendS(ثابت V3_عدد& lhs, uint32_t lbits);  // Sign extension
    V3_عدد& opExtendXZ(ثابت V3_عدد& lhs, uint32_t lbits);  // X/Z extension
    V3_عدد& opRedOr(ثابت V3_عدد& lhs);
    V3_عدد& opRedAnd(ثابت V3_عدد& lhs);
    V3_عدد& opRedXor(ثابت V3_عدد& lhs);
    V3_عدد& op_شمار_Bits(ثابت V3_عدد& عبارت, ثابت V3_عدد& ctrl1, ثابت V3_عدد& ctrl2,
                          ثابت V3_عدد& ctrl3);
    V3_عدد& op_شمار_Ones(ثابت V3_عدد& lhs);
    V3_عدد& opIsUnknown(ثابت V3_عدد& lhs);
    V3_عدد& opOneHot(ثابت V3_عدد& lhs);
    V3_عدد& opOneHot0(ثابت V3_عدد& lhs);
    V3_عدد& opCLog2(ثابت V3_عدد& lhs);
    V3_عدد& opMostSetBitP1(ثابت V3_عدد& lhs);
    V3_عدد& opClean(ثابت V3_عدد& lhs, uint32_t bits);
    V3_عدد& opConcat(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLenN(ثابت V3_عدد& lhs);
    V3_عدد& opRepl(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opRepl(ثابت V3_عدد& lhs, uint32_t rhsval);
    V3_عدد& opStreamL(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSel(ثابت V3_عدد& lhs, ثابت V3_عدد& msb, ثابت V3_عدد& lsb);
    V3_عدد& opSel(ثابت V3_عدد& lhs, uint32_t msbval, uint32_t lsbval);
    V3_عدد& opSelInto(ثابت V3_عدد& lhs, ثابت V3_عدد& lsb, صحیح width);
    V3_عدد& opSelInto(ثابت V3_عدد& lhs, صحیح lsbval, صحیح width);
    V3_عدد& opCvtPackString(ثابت V3_عدد& lhs);
    V3_عدد& opToLowerN(ثابت V3_عدد& lhs);
    V3_عدد& opToUpperN(ثابت V3_عدد& lhs);
    V3_عدد& opCaseEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opCaseNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opWildEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opWildNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opBufIf1(ثابت V3_عدد& ens, ثابت V3_عدد& if1s);
    V3_عدد& opSetRange(uint32_t lsb, uint32_t width, نویسه bit_مقدار);
    // "standard" math
    V3_عدد& opNot(ثابت V3_عدد& lhs);
    V3_عدد& opLogNot(ثابت V3_عدد& lhs);
    V3_عدد& opLogAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLogIf(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNegate(ثابت V3_عدد& lhs);
    V3_عدد& opAdd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSub(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opMul(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opMulS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opDiv(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opDivS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opModDiv(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opModDivS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opPow(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, bool lsign = false,
                    bool rsign = false);
    V3_عدد& opPowSU(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed lhs, unsigned rhs
    V3_عدد& opPowSS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed lhs, signed rhs
    V3_عدد& opPowUS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Unsigned lhs, signed rhs
    V3_عدد& opAnd(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opXor(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opOr(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opShiftR(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opShiftRS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_عدد& opShiftL(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    // Comparisons
    V3_عدد& opEq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNeq(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGt(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGtS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opGte(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGteS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opLt(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLtS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed
    V3_عدد& opLte(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLteS(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);  // Signed

    // "D" - double (aka real) math
    V3_عدد& opIToRD(ثابت V3_عدد& lhs, bool isSigned = false);
    V3_عدد& opISToRD(ثابت V3_عدد& lhs) { بازگشت opIToRD(lhs, true); }
    V3_عدد& opRToIS(ثابت V3_عدد& lhs);
    V3_عدد& opRToIRoundS(ثابت V3_عدد& lhs);
    V3_عدد& opRealToBits(ثابت V3_عدد& lhs);
    V3_عدد& opBitsToRealD(ثابت V3_عدد& lhs);
    V3_عدد& opNegateD(ثابت V3_عدد& lhs);
    V3_عدد& opAddD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSubD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opMulD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opDivD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opPowD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    // Comparisons
    V3_عدد& opEqD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNeqD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGtD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGteD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLtD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLteD(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);

    // "N" - string operations
    V3_عدد& opAtoN(ثابت V3_عدد& lhs, صحیح base);
    V3_عدد& opNToI(ثابت V3_عدد& lhs);
    V3_عدد& opPutcN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, ثابت V3_عدد& ths);
    V3_عدد& opGetcN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opSubstrN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, ثابت V3_عدد& ths);
    V3_عدد& opCompareNN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs, bool ignoreCase);
    V3_عدد& opConcatN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opReplN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opReplN(ثابت V3_عدد& lhs, uint32_t rhsval);
    V3_عدد& opEqN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opNeqN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGtN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opGteN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLtN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
    V3_عدد& opLteN(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs);
};
درونخطی std::ostream& operator<<(std::ostream& os, ثابت V3_عدد& rhs) VL_MT_SAFE {
    بازگشت os << rhs.ascii();
}

#endif  // Guard
