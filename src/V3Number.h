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

#ifndef VERILATOR_V3_ସଂଖ୍ୟା_H
#define VERILATOR_V3_ସଂଖ୍ୟା_H

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Hash.h"
#ଅନ୍ତର୍ଭାଵ "V3StdFuture.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <array>
#ଅନ୍ତର୍ଭାଵ <cmath>
#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <vector>

class AstEnumDType;
class AstEnumItem;
class Ast_ଗଣ୍ଠି;
class Ast_ଗଣ୍ଠି_DType;
class AstSFormatArg;
class File_ଧାଡ଼ି;

//============================================================================

class VFormatAttr final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : ଅକ୍ଷର {
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
    ଗଣନା en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅକ୍ଷର ascii() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    bool isComplex() ଅଚଳ { ଫେରନ୍ତୁ m_e == COMPLEX; }
    bool isDouble() ଅଚଳ { ଫେରନ୍ତୁ m_e == DOUBLE; }
    bool isEnum() ଅଚଳ { ଫେରନ୍ତୁ m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ଅଚଳ { ଫେରନ୍ତୁ m_e == SIGNED; }
    bool isString() ଅଚଳ { ଫେରନ୍ତୁ m_e == STRING; }
    bool isUnsigned() ଅଚଳ { ଫେରନ୍ତୁ m_e == UNSIGNED; }
};
constexpr bool operator==(ଅଚଳ VFormatAttr& lhs, ଅଚଳ VFormatAttr& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VFormatAttr& lhs, VFormatAttr::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, ଅଚଳ VFormatAttr& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VFormatAttr& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

//============================================================================

class V3_ସଂଖ୍ୟା_Data final {
ସର୍ବସାଧାରଣ:
    // TYPES
    ସଂରଚନା ମୂଲ୍ୟ_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_ମୂଲ୍ୟ;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_ମୂଲ୍ୟ_X;
        bool operator==(ଅଚଳ ମୂଲ୍ୟ_And_X& other) ଅଚଳ VL_MT_SAFE {
            ଫେରନ୍ତୁ m_ମୂଲ୍ୟ == other.m_ମୂଲ୍ୟ && m_ମୂଲ୍ୟ_X == other.m_ମୂଲ୍ୟ_X;
        }
    };

    ଗଣନା class V3_ସଂଖ୍ୟା_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ଅଚଳ V3_ସଂଖ୍ୟା_Data_Type& rhs) VL_MT_SAFE {
        ବାଛ (rhs) {
        ସ୍ଥିତି V3_ସଂଖ୍ୟା_Data_Type::UNINITIALIZED: ଫେରନ୍ତୁ os << "UNINITIALIZED";
        ସ୍ଥିତି V3_ସଂଖ୍ୟା_Data_Type::LOGIC: ଫେରନ୍ତୁ os << "LOGIC";
        ସ୍ଥିତି V3_ସଂଖ୍ୟା_Data_Type::DOUBLE: ଫେରନ୍ତୁ os << "DOUBLE";
        ସ୍ଥିତି V3_ସଂଖ୍ୟା_Data_Type::STRING: ଫେରନ୍ତୁ os << "STRING";
        }
        ଫେରନ୍ତୁ os;
    }

ବ୍ୟକ୍ତିଗତ:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା INLINE_ଶବ୍ଦଗୁଡ଼ିକ = std::max(
        static_cast<size_t>(2), std::max(ଆକାର(std::string) / ଆକାର(ମୂଲ୍ୟ_And_X),
                                         ଆକାର(std::vector<ମୂଲ୍ୟ_And_X>) / ଆକାର(ମୂଲ୍ୟ_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା MAX_INLINE_WIDTH = INLINE_ଶବ୍ଦଗୁଡ଼ିକ * ଆକାର(ମୂଲ୍ୟ_And_X) / 2 * 8;

    // MEMBERS
    ସଂଘ {
        std::array<ମୂଲ୍ୟ_And_X, INLINE_ଶବ୍ଦଗୁଡ଼ିକ> m_inline_ସଂଖ୍ୟା;
        std::vector<ମୂଲ୍ୟ_And_X> m_dynamic_ସଂଖ୍ୟା;
        std::string m_string;
    };

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_width = 0;  // Width (in bits) as specified/calculated
    V3_ସଂଖ୍ୟା_Data_Type m_type;

ସର୍ବସାଧାରଣ:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_ସଂଖ୍ୟା_Data()
        : m_type{V3_ସଂଖ୍ୟା_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_ସଂଖ୍ୟା_Data() { destroy_Stored_ମୂଲ୍ୟ(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_ସଂଖ୍ୟା_Data(ଅଚଳ V3_ସଂଖ୍ୟା_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ଯଦି (other.is_Inline_ସଂଖ୍ୟା()) {
            init_Inline_ସଂଖ୍ୟା(other.m_inline_ସଂଖ୍ୟା);
        } ଅନ୍ୟଥା ଯଦି (other.is_Dynamic_ସଂଖ୍ୟା()) {
            init_Dynamic_ସଂଖ୍ୟା(other.m_dynamic_ସଂଖ୍ୟା);
        } ଅନ୍ୟଥା ଯଦି (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_ସଂଖ୍ୟା_Data& operator=(ଅଚଳ V3_ସଂଖ୍ୟା_Data& other) {
        ଯଦି (other.is_Inline_ସଂଖ୍ୟା()) {
            destroy_Stored_ମୂଲ୍ୟ();
            init_Inline_ସଂଖ୍ୟା(other.m_inline_ସଂଖ୍ୟା);
        } ଅନ୍ୟଥା ଯଦି (other.is_Dynamic_ସଂଖ୍ୟା()) {
            reinit_With_Or_Assign_Dynamic_ସଂଖ୍ୟା(other.m_dynamic_ସଂଖ୍ୟା);
        } ଅନ୍ୟଥା ଯଦି (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } ଅନ୍ୟଥା {
            destroy_Stored_ମୂଲ୍ୟ();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        ଫେରନ୍ତୁ *this;
    }

    V3_ସଂଖ୍ୟା_Data(V3_ସଂଖ୍ୟା_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ଯଦି (other.is_Inline_ସଂଖ୍ୟା()) {
            init_Inline_ସଂଖ୍ୟା(other.m_inline_ସଂଖ୍ୟା);
        } ଅନ୍ୟଥା ଯଦି (other.is_Dynamic_ସଂଖ୍ୟା()) {
            init_Dynamic_ସଂଖ୍ୟା(std::move(other.m_dynamic_ସଂଖ୍ୟା));
        } ଅନ୍ୟଥା ଯଦି (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_ସଂଖ୍ୟା_Data_Type::UNINITIALIZED;
    }

    V3_ସଂଖ୍ୟା_Data& operator=(V3_ସଂଖ୍ୟା_Data&& other) {
        ଯଦି (other.is_Inline_ସଂଖ୍ୟା()) {
            destroy_Stored_ମୂଲ୍ୟ();
            init_Inline_ସଂଖ୍ୟା(other.m_inline_ସଂଖ୍ୟା);
        } ଅନ୍ୟଥା ଯଦି (other.is_Dynamic_ସଂଖ୍ୟା()) {
            reinit_With_Or_Assign_Dynamic_ସଂଖ୍ୟା(std::move(other.m_dynamic_ସଂଖ୍ୟା));
        } ଅନ୍ୟଥା ଯଦି (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } ଅନ୍ୟଥା {
            destroy_Stored_ମୂଲ୍ୟ();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_ସଂଖ୍ୟା_Data_Type::UNINITIALIZED;
        ଫେରନ୍ତୁ *this;
    }

    // ACCESSORS
    ମୂଲ୍ୟ_And_X* ଅଙ୍କ() {
        UASSERT(is_ସଂଖ୍ୟା(), "`num` member accessed when data type is " << m_type);
        ଫେରନ୍ତୁ is_Inline_ସଂଖ୍ୟା() ? m_inline_ସଂଖ୍ୟା.data() : m_dynamic_ସଂଖ୍ୟା.data();
    }
    ଅଚଳ ମୂଲ୍ୟ_And_X* ଅଙ୍କ() ଅଚଳ VL_MT_SAFE {
        UASSERT(is_ସଂଖ୍ୟା(), "`num` member accessed when data type is " << m_type);
        ଫେରନ୍ତୁ is_Inline_ସଂଖ୍ୟା() ? m_inline_ସଂଖ୍ୟା.data() : m_dynamic_ସଂଖ୍ୟା.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ଫେରନ୍ତୁ m_string;
    }
    ଅଚଳ std::string& str() ଅଚଳ VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        ଫେରନ୍ତୁ m_string;
    }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା width() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_width; }
    V3_ସଂଖ୍ୟା_Data_Type type() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_type; }

    // METHODS
    ଶୂନ୍ୟ resize(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bits_ଗଣନା) {
        ଯଦି (m_width == bits_ଗଣନା) ଫେରନ୍ତୁ;
        ଯଦି (bits_To_ଶବ୍ଦଗୁଡ଼ିକ(m_width) == bits_To_ଶବ୍ଦଗୁଡ଼ିକ(bits_ଗଣନା)) {
            m_width = bits_ଗଣନା;
            ଫେରନ୍ତୁ;
        }
        ଯଦି (is_Dynamic_ସଂଖ୍ୟା()) {
            ଯଦି (bits_ଗଣନା > MAX_INLINE_WIDTH) {
                m_dynamic_ସଂଖ୍ୟା.resize(bits_To_ଶବ୍ଦଗୁଡ଼ିକ(bits_ଗଣନା));
            } ଅନ୍ୟଥା {
                ଅଚଳ auto dynamicBits = std::move(m_dynamic_ସଂଖ୍ୟା);
                destroy_Dynamic_ସଂଖ୍ୟା();
                init_Inline_ସଂଖ୍ୟା();
                std::memcpy(m_inline_ସଂଖ୍ୟା.data(), dynamicBits.data(), ଆକାର(m_inline_ସଂଖ୍ୟା));
            }
        } ଅନ୍ୟଥା ଯଦି (is_Inline_ସଂଖ୍ୟା()) {
            ଯଦି (bits_ଗଣନା > MAX_INLINE_WIDTH) {
                ଅଚଳ auto bits = m_inline_ସଂଖ୍ୟା;
                init_Dynamic_ସଂଖ୍ୟା(bits_To_ଶବ୍ଦଗୁଡ଼ିକ(bits_ଗଣନା));
                std::memcpy(m_dynamic_ସଂଖ୍ୟା.data(), bits.data(), ଆକାର(bits));
            }
        }
        m_width = bits_ଗଣନା;
    }

    ଶୂନ୍ୟ setString() {
        // If there has been a string already it is kept intact.
        ଯଦି (isString()) ଫେରନ୍ତୁ;
        ଯଦି (is_Dynamic_ସଂଖ୍ୟା()) destroy_Dynamic_ସଂଖ୍ୟା();
        initString();
        m_type = V3_ସଂଖ୍ୟା_Data_Type::STRING;
    }
    ଶୂନ୍ୟ setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_ସଂଖ୍ୟା_Data_Type::STRING;
    }
    ଶୂନ୍ୟ setString(ଅଚଳ std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_ସଂଖ୍ୟା_Data_Type::STRING;
    }

    ଶୂନ୍ୟ setDouble() {
        destroy_Stored_ମୂଲ୍ୟ();
        ଯଦି (!is_Inline_ସଂଖ୍ୟା()) init_Inline_ସଂଖ୍ୟା();
        m_type = V3_ସଂଖ୍ୟା_Data_Type::DOUBLE;
        resize(64);
    }

    ଶୂନ୍ୟ setLogic() {
        ଯଦି (isString()) destroyString();
        ଯଦି (!is_ସଂଖ୍ୟା()) {
            ଯଦି (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_ସଂଖ୍ୟା();
            } ଅନ୍ୟଥା {
                init_Dynamic_ସଂଖ୍ୟା(bits_To_ଶବ୍ଦଗୁଡ଼ିକ(m_width));
            }
        }
        m_type = V3_ସଂଖ୍ୟା_Data_Type::LOGIC;
        resize(m_width);
    }

ବ୍ୟକ୍ତିଗତ:
    ସ୍ଥିର constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା bits_To_ଶବ୍ଦଗୁଡ଼ିକ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bits_ଗଣନା) VL_PURE { ଫେରନ୍ତୁ (bits_ଗଣନା + 31) / 32; }

    bool is_ସଂଖ୍ୟା() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_type == V3_ସଂଖ୍ୟା_Data_Type::DOUBLE || m_type == V3_ସଂଖ୍ୟା_Data_Type::LOGIC;
    }
    bool is_Inline_ସଂଖ୍ୟା() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_ସଂଖ୍ୟା_Data_Type::DOUBLE || m_type == V3_ସଂଖ୍ୟା_Data_Type::LOGIC);
    }
    bool is_Dynamic_ସଂଖ୍ୟା() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (m_width > MAX_INLINE_WIDTH) && (m_type == V3_ସଂଖ୍ୟା_Data_Type::LOGIC);
    }
    bool isString() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_type == V3_ସଂଖ୍ୟା_Data_Type::STRING; }

    template <typename... Args>
    ଶୂନ୍ୟ init_Inline_ସଂଖ୍ୟା(Args&&... args) {
        new (&m_inline_ସଂଖ୍ୟା) std::array<ମୂଲ୍ୟ_And_X, INLINE_ଶବ୍ଦଗୁଡ଼ିକ>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    ଶୂନ୍ୟ init_Dynamic_ସଂଖ୍ୟା(Args&&... args) {
        new (&m_dynamic_ସଂଖ୍ୟା) std::vector<ମୂଲ୍ୟ_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    ଶୂନ୍ୟ initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    ଶୂନ୍ୟ destroy_Dynamic_ସଂଖ୍ୟା() { m_dynamic_ସଂଖ୍ୟା.~vector(); }
    ଶୂନ୍ୟ destroyString() { m_string.~string(); }
    ଶୂନ୍ୟ destroy_Stored_ମୂଲ୍ୟ() {
        ଯଦି (isString())
            destroyString();
        ଅନ୍ୟଥା ଯଦି (is_Dynamic_ସଂଖ୍ୟା())
            destroy_Dynamic_ସଂଖ୍ୟା();
    }

    template <typename T>
    ଶୂନ୍ୟ reinit_With_Or_Assign_Dynamic_ସଂଖ୍ୟା(T&& s) {
        ଯଦି (is_Dynamic_ସଂଖ୍ୟା()) {
            m_dynamic_ସଂଖ୍ୟା = std::forward<T>(s);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (isString()) destroyString();
        init_Dynamic_ସଂଖ୍ୟା(std::forward<T>(s));
    }
    template <typename T>
    ଶୂନ୍ୟ reinitWithOrAssignString(T&& s) {
        ଯଦି (isString()) {
            m_string = std::forward<T>(s);
            ଫେରନ୍ତୁ;
        }
        ଯଦି (is_Dynamic_ସଂଖ୍ୟା()) destroy_Dynamic_ସଂଖ୍ୟା();
        initString(std::forward<T>(s));
    }
};

class V3_ସଂଖ୍ୟା final {
    // TYPES
    using ମୂଲ୍ୟ_And_X = V3_ସଂଖ୍ୟା_Data::ମୂଲ୍ୟ_And_X;
    using V3_ସଂଖ୍ୟା_Data_Type = V3_ସଂଖ୍ୟା_Data::V3_ସଂଖ୍ୟା_Data_Type;

    // MEMBERS
    V3_ସଂଖ୍ୟା_Data m_data;
    Ast_ଗଣ୍ଠି* m_nodep = nullptr;  // Parent node - for error reporting only
    File_ଧାଡ଼ି* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    ଶୂନ୍ୟ setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_ସଂଖ୍ୟା& setSingleBits(ଅକ୍ଷର ମୂଲ୍ୟ);
    ଶୂନ୍ୟ fillBits(ପୂର୍ଣ୍ଣସଂଖ୍ୟା destLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ଅକ୍ଷର ମୂଲ୍ୟ);
    ଶୂନ୍ୟ copyBits(ପୂର୍ଣ୍ଣସଂଖ୍ୟା destLsb, ଅଚଳ V3_ସଂଖ୍ୟା& source, ପୂର୍ଣ୍ଣସଂଖ୍ୟା sourceLsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width);
    V3_ସଂଖ୍ୟା& setString(ଅଚଳ string& str) {
        m_data.setString(str);
        ଫେରନ୍ତୁ *this;
    }
    ଶୂନ୍ୟ opCleanThis(bool warnOnTruncation = false);

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ nodep(Ast_ଗଣ୍ଠି* nodep) VL_MT_STABLE;
    Ast_ଗଣ୍ଠି* nodep() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_nodep; }  // For debug only
    File_ଧାଡ଼ି* fileline() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_fileline; }
    V3_ସଂଖ୍ୟା& setZero();
    V3_ସଂଖ୍ୟା& setQuad(uint64_t ମୂଲ୍ୟ);
    V3_ସଂଖ୍ୟା& setLong(uint32_t ମୂଲ୍ୟ);
    V3_ସଂଖ୍ୟା& setLongS(int32_t ମୂଲ୍ୟ);
    V3_ସଂଖ୍ୟା& setDouble(ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ);
    ଶୂନ୍ୟ setBitX0(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit);
    ଶୂନ୍ୟ setBit(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit, ଅକ୍ଷର ମୂଲ୍ୟ) {  // Note: must be initialized as number and pre-zeroed!
        ଯଦି (bit >= m_data.width()) ଫେରନ୍ତୁ;
        ଅଚଳ uint32_t mask = (1UL << (bit & 31));
        ମୂଲ୍ୟ_And_X& v = m_data.ଅଙ୍କ()[bit / 32];
        ଯଦି (ମୂଲ୍ୟ == '0' || ମୂଲ୍ୟ == 0) {
            v.m_ମୂଲ୍ୟ &= ~mask;
            v.m_ମୂଲ୍ୟ_X &= ~mask;
        } ଅନ୍ୟଥା ଯଦି (ମୂଲ୍ୟ == '1' || ମୂଲ୍ୟ == 1) {
            v.m_ମୂଲ୍ୟ |= mask;
            v.m_ମୂଲ୍ୟ_X &= ~mask;
        } ଅନ୍ୟଥା ଯଦି (ମୂଲ୍ୟ == 'z' || ମୂଲ୍ୟ == 2) {
            v.m_ମୂଲ୍ୟ &= ~mask;
            v.m_ମୂଲ୍ୟ_X |= mask;
        } ଅନ୍ୟଥା {  // X
            v.m_ମୂଲ୍ୟ |= mask;
            v.m_ମୂଲ୍ୟ_X |= mask;
        }
    }

ବ୍ୟକ୍ତିଗତ:
    ଅକ୍ଷର bitIs(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ {
        ଯଦି (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            ଫେରନ୍ତୁ '0';
        }
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
        ଫେରନ୍ତୁ ("01zx"[(((v.m_ମୂଲ୍ୟ & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_ମୂଲ୍ୟ_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    ଅକ୍ଷର bitIsExtend(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lbits) ଅଚଳ {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        ଯଦି (bit < 0) ଫେରନ୍ତୁ '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        ଯଦି (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
            // We do sign extend
            ଫେରନ୍ତୁ ("01zx"[(((v.m_ମୂଲ୍ୟ & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_ମୂଲ୍ୟ_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
        ଫେରନ୍ତୁ ("01zx"[(((v.m_ମୂଲ୍ୟ & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_ମୂଲ୍ୟ_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

ସର୍ବସାଧାରଣ:
    bool bitIs0(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ VL_MT_SAFE {
        ଯଦି (!is_ସଂଖ୍ୟା()) ଫେରନ୍ତୁ false;
        ଯଦି (bit < 0) ଫେରନ୍ତୁ false;
        ଯଦି (bit >= m_data.width()) ଫେରନ୍ତୁ !bitIsXZ(m_data.width() - 1);
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
        ଫେରନ୍ତୁ ((v.m_ମୂଲ୍ୟ | v.m_ମୂଲ୍ୟ_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ VL_MT_SAFE {
        ଯଦି (!is_ସଂଖ୍ୟା()) ଫେରନ୍ତୁ false;
        ଯଦି (bit < 0) ଫେରନ୍ତୁ false;
        ଯଦି (bit >= m_data.width()) ଫେରନ୍ତୁ false;
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
        ଫେରନ୍ତୁ ((v.m_ମୂଲ୍ୟ & (1UL << (bit & 31))) && !(v.m_ମୂଲ୍ୟ_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ {
        // Correct number of zero bits/width matters
        ଯଦି (!is_ସଂଖ୍ୟା()) ଫେରନ୍ତୁ false;
        ଯଦି (bit < 0) ଫେରନ୍ତୁ false;
        ଯଦି (bit >= m_data.width()) ଫେରନ୍ତୁ bitIs1Extend(m_data.width() - 1);
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
        ଫେରନ୍ତୁ ((v.m_ମୂଲ୍ୟ & (1UL << (bit & 31))) && !(v.m_ମୂଲ୍ୟ_X & (1UL << (bit & 31))));
    }
    bool bitIsX(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ VL_MT_SAFE {
        ଯଦି (!is_ସଂଖ୍ୟା()) ଫେରନ୍ତୁ false;
        ଯଦି (bit < 0) ଫେରନ୍ତୁ false;
        ଯଦି (bit >= m_data.width()) ଫେରନ୍ତୁ bitIsZ(m_data.width() - 1);
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
        ଫେରନ୍ତୁ ((v.m_ମୂଲ୍ୟ & (1UL << (bit & 31))) && (v.m_ମୂଲ୍ୟ_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ VL_MT_SAFE {
        ଯଦି (!is_ସଂଖ୍ୟା()) ଫେରନ୍ତୁ false;
        ଯଦି (bit < 0) ଫେରନ୍ତୁ false;
        ଯଦି (bit >= m_data.width()) ଫେରନ୍ତୁ bitIsXZ(m_data.width() - 1);
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
        ଫେରନ୍ତୁ ((v.m_ମୂଲ୍ୟ_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା bit) ଅଚଳ VL_MT_SAFE {
        ଯଦି (!is_ସଂଖ୍ୟା()) ଫେରନ୍ତୁ false;
        ଯଦି (bit < 0) ଫେରନ୍ତୁ false;
        ଯଦି (bit >= m_data.width()) ଫେରନ୍ତୁ bitIsZ(m_data.width() - 1);
        ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[bit / 32];
        ଫେରନ୍ତୁ ((~v.m_ମୂଲ୍ୟ & (1UL << (bit & 31))) && (v.m_ମୂଲ୍ୟ_X & (1UL << (bit & 31))));
    }

ବ୍ୟକ୍ତିଗତ:
    uint32_t bits_ମୂଲ୍ୟ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା nbits) ଅଚଳ VL_MT_SAFE {
        uint32_t v = 0;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        ଫେରନ୍ତୁ v;
    }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣନା_X(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା nbits) ଅଚଳ VL_MT_SAFE;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଗଣନା_Z(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା nbits) ଅଚଳ VL_MT_SAFE;

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଶବ୍ଦଗୁଡ଼ିକ() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ ((width() + 31) / 32); }
    uint32_t hiWordMask() ଅଚଳ VL_MT_SAFE {
        // Correct number of zero bits/width matters
        ଫେରନ୍ତୁ VL_MASK_I(width());
    }

    V3_ସଂଖ୍ୟା& opModDivGuts(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, bool is_modulus);

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep) { init(nodep, 1); }
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ଅଚଳ V3_ସଂଖ୍ୟା& ମୂଲ୍ୟ) {
        init(nodep, width, width > 0);
        opAssign(ମୂଲ୍ୟ);
    }
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, uint32_t ମୂଲ୍ୟ, bool sized = true) {
        init(nodep, width, sized);
        m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = ମୂଲ୍ୟ;
        opCleanThis();
    }
    V3_ସଂଖ୍ୟା(File_ଧାଡ଼ି* flp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, uint32_t ମୂଲ୍ୟ, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = ମୂଲ୍ୟ;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_ସଂଖ୍ୟା_Literal {};  // For creator type-overload selection
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, Verilog_ସଂଖ୍ୟା_Literal, ଅଚଳ ଅକ୍ଷର* sourcep) { create(nodep, sourcep); }
    V3_ସଂଖ୍ୟା(File_ଧାଡ଼ି* flp, Verilog_ସଂଖ୍ୟା_Literal, ଅଚଳ ଅକ୍ଷର* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, VerilogStringLiteral, ଅଚଳ string& str);
    class Double {};
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, Double, ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ) {
        init(nodep, 64);
        setDouble(ମୂଲ୍ୟ);
    }
    class String {};
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, String, ଅଚଳ string& ମୂଲ୍ୟ) {
        init(nodep);
        setString(ମୂଲ୍ୟ);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_ସଂଖ୍ୟା(ଅଚଳ V3_ସଂଖ୍ୟା* nump, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_ସଂଖ୍ୟା(ଅଚଳ V3_ସଂଖ୍ୟା* nump, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, uint32_t ମୂଲ୍ୟ) {
        init(nullptr, width);
        m_data.ଅଙ୍କ()[0].m_ମୂଲ୍ୟ = ମୂଲ୍ୟ;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_ସଂଖ୍ୟା(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_DType* nodedtypep);

    V3_ସଂଖ୍ୟା(ଅଚଳ V3_ସଂଖ୍ୟା& other) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    V3_ସଂଖ୍ୟା& operator=(ଅଚଳ V3_ସଂଖ୍ୟା& other) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    V3_ସଂଖ୍ୟା(V3_ସଂଖ୍ୟା&& other) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    V3_ସଂଖ୍ୟା& operator=(V3_ସଂଖ୍ୟା&& other) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ~V3_ସଂଖ୍ୟା() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ selfTestThis();
    ଶୂନ୍ୟ create(Ast_ଗଣ୍ଠି* nodep, ଅଚଳ ଅକ୍ଷର* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    ଶୂନ୍ୟ create(File_ଧାଡ଼ି* flp, ଅଚଳ ଅକ୍ଷର* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    ଶୂନ୍ୟ create(ଅଚଳ ଅକ୍ଷର* sourcep);
    ଶୂନ୍ୟ init(Ast_ଗଣ୍ଠି* nodep, ପୂର୍ଣ୍ଣସଂଖ୍ୟା swidth = -1, bool sized = true) {
        this->nodep(nodep);
        ଯଦି (swidth >= 0) {
            ଯଦି (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) m_data.ଅଙ୍କ()[i] = {0, 0};
        } ଅନ୍ୟଥା {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    ସ୍ଥିର string displayPad(size_t fmtsize, ଅକ୍ଷର pad, bool ବାମ, ଅଚଳ string& in) VL_PURE;
    string displayed(ଅଚଳ string& vformat) ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ displayed(m_fileline, vformat);
    }
    ଶୂନ୍ୟ warnTooMany(ଅଚଳ string& ମୂଲ୍ୟ);

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ v3errorEnd(ଅଚଳ std::ostringstream& sstr) ଅଚଳ VL_RELEASE(V3Error::s().m_mutex);
    ଶୂନ୍ୟ v3errorEndFatal(ଅଚଳ std::ostringstream& sstr) ଅଚଳ VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    ଶୂନ୍ୟ width(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_ସଂଖ୍ୟା& setAllBitsXRemoved();
    V3_ସଂଖ୍ୟା& setAllBitsX();
    V3_ସଂଖ୍ୟା& setAllBitsZ();
    V3_ସଂଖ୍ୟା& setAllBits0();
    V3_ସଂଖ୍ୟା& setAllBits1();
    V3_ସଂଖ୍ୟା& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_ସଂଖ୍ୟା& setMask(ପୂର୍ଣ୍ଣସଂଖ୍ୟା nbits, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ଅଚଳ VL_MT_STABLE;
    string displayed(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& vformat,
                     ଅଚଳ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ଅଚଳ VL_MT_STABLE;
    string displayed(File_ଧାଡ଼ି* fl, ଅଚଳ string& vformat,
                     ଅଚଳ VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ଅଚଳ VL_MT_STABLE;
    string displayedEnum(ଅଚଳ AstSFormatArg* argp, ଅଚଳ string& vformat) ଅଚଳ VL_MT_STABLE;
    string displayedEnumName(ଅଚଳ AstEnumDType* dtypep) ଅଚଳ VL_MT_STABLE;
    ସ୍ଥିର string displayedEnumName(ଅଚଳ AstEnumItem* itemp);
    ସ୍ଥିର bool displayedFmtHasArg(ଅକ୍ଷର format, bool isScan);
    string emitC() ଅଚଳ VL_MT_STABLE;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା width() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_data.width(); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthToFit() ଅଚଳ;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_data.m_sized; }
    bool autoExtend() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_data.m_autoExtend; }
    bool isFromString() ଅଚଳ { ଫେରନ୍ତୁ m_data.m_fromString; }
    bool hasOrigParamName() ଅଚଳ { ଫେରନ୍ତୁ m_data.m_hasOrigParamName; }
    ଶୂନ୍ୟ hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_ସଂଖ୍ୟା_Data_Type dataType() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_data.type(); }
    ଶୂନ୍ୟ dataType(V3_ସଂଖ୍ୟା_Data_Type newType) {
        ଯଦି (dataType() == newType) ଫେରନ୍ତୁ;
        UASSERT(newType != V3_ସଂଖ୍ୟା_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        ବାଛ (newType) {
        ସ୍ଥିତି V3_ସଂଖ୍ୟା_Data_Type::STRING: m_data.setString(); ଭାଙ୍ଗ;
        ସ୍ଥିତି V3_ସଂଖ୍ୟା_Data_Type::DOUBLE: m_data.setDouble(); ଭାଙ୍ଗ;
        ସ୍ଥିତି V3_ସଂଖ୍ୟା_Data_Type::LOGIC: m_data.setLogic(); ଭାଙ୍ଗ;
        ସ୍ଥିତି V3_ସଂଖ୍ୟା_Data_Type::UNINITIALIZED: ଭାଙ୍ଗ;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_data.m_signed; }
    ଶୂନ୍ୟ isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ dataType() == V3_ସଂଖ୍ୟା_Data_Type::DOUBLE; }
    bool isString() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ dataType() == V3_ସଂଖ୍ୟା_Data_Type::STRING; }
    bool isOpaque() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ isDouble() || isString(); }
    bool is_ସଂଖ୍ୟା() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_data.type() == V3_ସଂଖ୍ୟା_Data_Type::LOGIC
               || m_data.type() == V3_ସଂଖ୍ୟା_Data_Type::DOUBLE;
    }
    bool isNegative() ଅଚଳ VL_MT_SAFE {
        // Correct number of zero bits/width matters
        ଫେରନ୍ତୁ !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_data.m_is1Step; }
    bool isNull() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_data.m_isNull; }
    bool isFourState() ଅଚଳ VL_MT_SAFE;
    bool hasZ() ଅଚଳ {
        ଯଦି (isString()) ଫେରନ୍ତୁ false;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < ଶବ୍ଦଗୁଡ଼ିକ(); ++i) {
            ଅଚଳ ମୂଲ୍ୟ_And_X v = m_data.ଅଙ୍କ()[i];
            ଯଦି ((~v.m_ମୂଲ୍ୟ) & v.m_ମୂଲ୍ୟ_X) ଫେରନ୍ତୁ true;
        }
        ଫେରନ୍ତୁ false;
    }
    bool isAllZ() ଅଚଳ VL_MT_SAFE;
    bool isAllX() ଅଚଳ VL_MT_SAFE;
    bool isEqZero() ଅଚଳ VL_MT_SAFE;
    bool isNeqZero() ଅଚଳ;
    bool isBitsZero(ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb) ଅଚଳ;
    bool isBroken(ପୂର୍ଣ୍ଣସଂଖ୍ୟା vwidth) ଅଚଳ;
    bool isEqOne() ଅଚଳ;
    bool isEqAllOnes(ପୂର୍ଣ୍ଣସଂଖ୍ୟା optwidth = 0) ଅଚଳ;
    bool isCaseEq(ଅଚଳ V3_ସଂଖ୍ୟା& rhs) ଅଚଳ;  // operator==
    bool isLtXZ(ଅଚଳ V3_ସଂଖ୍ୟା& rhs) ଅଚଳ;  // operator< with XZ compared
    bool isAnyX() ଅଚଳ VL_MT_SAFE;
    bool isAnyXZ() ଅଚଳ;
    bool isAnyZ() ଅଚଳ VL_MT_SAFE;
    bool isMsbXZ() ଅଚଳ {
        // Correct number of zero bits/width matters
        ଫେରନ୍ତୁ bitIsXZ(width() - 1);
    }
    bool fitsInUInt() ଅଚଳ VL_MT_SAFE;
    uint32_t toUInt() ଅଚଳ VL_MT_SAFE;
    int32_t toSInt() ଅଚଳ VL_MT_SAFE;
    uint64_t toUQuad() ଅଚଳ VL_MT_SAFE;
    int64_t toSQuad() ଅଚଳ VL_MT_SAFE;
    string toString() ଅଚଳ VL_MT_SAFE;
    string toDecimalS() ଅଚଳ VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() ଅଚଳ VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() ଅଚଳ VL_MT_STABLE;
    ଦ୍ୱିଗୁଣ toDouble() ଅଚଳ VL_MT_SAFE;
    V3Hash toHash() ଅଚଳ;
    uint32_t edataWord(ପୂର୍ଣ୍ଣସଂଖ୍ୟା eword) ଅଚଳ;
    uint8_t dataByte(ପୂର୍ଣ୍ଣସଂଖ୍ୟା byte) ଅଚଳ;
    uint32_t ଗଣନା_Bits(ଅଚଳ V3_ସଂଖ୍ୟା& ctrl) ଅଚଳ;
    uint32_t ଗଣନା_Bits(ଅଚଳ V3_ସଂଖ୍ୟା& ctrl1, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl2, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl3) ଅଚଳ;
    uint32_t ଗଣନା_Ones() ଅଚଳ;
    uint32_t mostSetBitP1() ଅଚଳ;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ଅଚଳ;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ଅଚଳ V3_ସଂଖ୍ୟା& rhs) ଅଚଳ { ଫେରନ୍ତୁ isLtXZ(rhs); }

    // STATICS
    ସ୍ଥିର bool epsilonEqual(ଦ୍ୱିଗୁଣ a, ଦ୍ୱିଗୁଣ b);  // True if number within Epsilon of the other
    ସ୍ଥିର bool epsilonIntegral(ଦ୍ୱିଗୁଣ a);  // True if number rounds to integer within Epsilon
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା log2b(uint32_t ଅଙ୍କ);
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା log2bQuad(uint64_t ଅଙ୍କ);
    ସ୍ଥିର ଶୂନ୍ୟ selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_ସଂଖ୍ୟା& opBitsNonXZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);  // 0/1->1, X/Z->0
    V3_ସଂଖ୍ୟା& opBitsOne(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);  // 1->1, 0/X/Z->0
    V3_ସଂଖ୍ୟା& opBitsXZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);  // 0/1->0, X/Z->1
    V3_ସଂଖ୍ୟା& opBitsZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);  // Z->1, 0/1/X->0
    //
    V3_ସଂଖ୍ୟା& opAssign(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opAssignNonXZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, bool ignoreXZ = true);
    V3_ସଂଖ୍ୟା& opExtendS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t lbits);  // Sign extension
    V3_ସଂଖ୍ୟା& opExtendXZ(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t lbits);  // X/Z extension
    V3_ସଂଖ୍ୟା& opRedOr(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opRedAnd(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opRedXor(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& op_ଗଣନା_Bits(ଅଚଳ V3_ସଂଖ୍ୟା& ପଦସମଷ୍ଟି, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl1, ଅଚଳ V3_ସଂଖ୍ୟା& ctrl2,
                          ଅଚଳ V3_ସଂଖ୍ୟା& ctrl3);
    V3_ସଂଖ୍ୟା& op_ଗଣନା_Ones(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opIsUnknown(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opOneHot(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opOneHot0(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opCLog2(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opMostSetBitP1(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opClean(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t bits);
    V3_ସଂଖ୍ୟା& opConcat(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLenN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opRepl(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opRepl(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t rhsval);
    V3_ସଂଖ୍ୟା& opStreamL(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opSel(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& msb, ଅଚଳ V3_ସଂଖ୍ୟା& lsb);
    V3_ସଂଖ୍ୟା& opSel(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t msbval, uint32_t lsbval);
    V3_ସଂଖ୍ୟା& opSelInto(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& lsb, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width);
    V3_ସଂଖ୍ୟା& opSelInto(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsbval, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width);
    V3_ସଂଖ୍ୟା& opCvtPackString(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opToLowerN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opToUpperN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opCaseEq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opCaseNeq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opWildEq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opWildNeq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opBufIf1(ଅଚଳ V3_ସଂଖ୍ୟା& ens, ଅଚଳ V3_ସଂଖ୍ୟା& if1s);
    V3_ସଂଖ୍ୟା& opSetRange(uint32_t lsb, uint32_t width, ଅକ୍ଷର bit_ମୂଲ୍ୟ);
    // "standard" math
    V3_ସଂଖ୍ୟା& opNot(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opLogNot(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opLogAnd(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLogOr(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLogEq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLogIf(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opNegate(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opAdd(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opSub(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opMul(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opMulS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed
    V3_ସଂଖ୍ୟା& opDiv(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opDivS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed
    V3_ସଂଖ୍ୟା& opModDiv(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opModDivS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed
    V3_ସଂଖ୍ୟା& opPow(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, bool lsign = false,
                    bool rsign = false);
    V3_ସଂଖ୍ୟା& opPowSU(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed lhs, unsigned rhs
    V3_ସଂଖ୍ୟା& opPowSS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed lhs, signed rhs
    V3_ସଂଖ୍ୟା& opPowUS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Unsigned lhs, signed rhs
    V3_ସଂଖ୍ୟା& opAnd(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opXor(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opOr(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opShiftR(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opShiftRS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_ସଂଖ୍ୟା& opShiftL(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    // Comparisons
    V3_ସଂଖ୍ୟା& opEq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opNeq(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opGt(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opGtS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed
    V3_ସଂଖ୍ୟା& opGte(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opGteS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed
    V3_ସଂଖ୍ୟା& opLt(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLtS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed
    V3_ସଂଖ୍ୟା& opLte(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLteS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);  // Signed

    // "D" - double (aka real) math
    V3_ସଂଖ୍ୟା& opIToRD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, bool isSigned = false);
    V3_ସଂଖ୍ୟା& opISToRD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs) { ଫେରନ୍ତୁ opIToRD(lhs, true); }
    V3_ସଂଖ୍ୟା& opRToIS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opRToIRoundS(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opRealToBits(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opBitsToRealD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opNegateD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opAddD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opSubD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opMulD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opDivD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opPowD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    // Comparisons
    V3_ସଂଖ୍ୟା& opEqD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opNeqD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opGtD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opGteD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLtD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLteD(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);

    // "N" - string operations
    V3_ସଂଖ୍ୟା& opAtoN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ପୂର୍ଣ୍ଣସଂଖ୍ୟା base);
    V3_ସଂଖ୍ୟା& opNToI(ଅଚଳ V3_ସଂଖ୍ୟା& lhs);
    V3_ସଂଖ୍ୟା& opPutcN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, ଅଚଳ V3_ସଂଖ୍ୟା& ths);
    V3_ସଂଖ୍ୟା& opGetcN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opSubstrN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, ଅଚଳ V3_ସଂଖ୍ୟା& ths);
    V3_ସଂଖ୍ୟା& opCompareNN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs, bool ignoreCase);
    V3_ସଂଖ୍ୟା& opConcatN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opReplN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opReplN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, uint32_t rhsval);
    V3_ସଂଖ୍ୟା& opEqN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opNeqN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opGtN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opGteN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLtN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
    V3_ସଂଖ୍ୟା& opLteN(ଅଚଳ V3_ସଂଖ୍ୟା& lhs, ଅଚଳ V3_ସଂଖ୍ୟା& rhs);
};
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ V3_ସଂଖ୍ୟା& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

#endif  // Guard
