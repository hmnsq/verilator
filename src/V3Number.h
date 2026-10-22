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

#ifndef VERILATOR_V3_संख्या_H
#define VERILATOR_V3_संख्या_H

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Hash.h"
#अन्तर्भाव "V3StdFuture.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <array>
#अन्तर्भाव <cmath>
#अन्तर्भाव <limits>
#अन्तर्भाव <vector>

class AstEnumDType;
class AstEnumItem;
class Ast_गाठ;
class Ast_गाठ_DType;
class AstSFormatArg;
class File_ओळ;

//============================================================================

class VFormatAttr final {
सार्वजनिक:
    गणना en : वर्ण {
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
    गणना en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    वर्ण ascii() ध्रुव { प्रत्यागम m_e; }
    bool isComplex() ध्रुव { प्रत्यागम m_e == COMPLEX; }
    bool isDouble() ध्रुव { प्रत्यागम m_e == DOUBLE; }
    bool isEnum() ध्रुव { प्रत्यागम m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ध्रुव { प्रत्यागम m_e == SIGNED; }
    bool isString() ध्रुव { प्रत्यागम m_e == STRING; }
    bool isUnsigned() ध्रुव { प्रत्यागम m_e == UNSIGNED; }
};
constexpr bool operator==(ध्रुव VFormatAttr& lhs, ध्रुव VFormatAttr& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VFormatAttr& lhs, VFormatAttr::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, ध्रुव VFormatAttr& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VFormatAttr& rhs) {
    प्रत्यागम os << rhs.ascii();
}

//============================================================================

class V3_संख्या_Data final {
सार्वजनिक:
    // TYPES
    संरचना मूल्य_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_मूल्य;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_मूल्य_X;
        bool operator==(ध्रुव मूल्य_And_X& other) ध्रुव VL_MT_SAFE {
            प्रत्यागम m_मूल्य == other.m_मूल्य && m_मूल्य_X == other.m_मूल्य_X;
        }
    };

    गणना class V3_संख्या_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, ध्रुव V3_संख्या_Data_Type& rhs) VL_MT_SAFE {
        विकल्प (rhs) {
        पक्ष V3_संख्या_Data_Type::UNINITIALIZED: प्रत्यागम os << "UNINITIALIZED";
        पक्ष V3_संख्या_Data_Type::LOGIC: प्रत्यागम os << "LOGIC";
        पक्ष V3_संख्या_Data_Type::DOUBLE: प्रत्यागम os << "DOUBLE";
        पक्ष V3_संख्या_Data_Type::STRING: प्रत्यागम os << "STRING";
        }
        प्रत्यागम os;
    }

गुप्त:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    स्थिर constexpr पूर्णाङ्क INLINE_शब्द = std::max(
        static_cast<size_t>(2), std::max(परिमाण(std::string) / परिमाण(मूल्य_And_X),
                                         परिमाण(std::vector<मूल्य_And_X>) / परिमाण(मूल्य_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    स्थिर constexpr पूर्णाङ्क MAX_INLINE_WIDTH = INLINE_शब्द * परिमाण(मूल्य_And_X) / 2 * 8;

    // MEMBERS
    सङ्घ {
        std::array<मूल्य_And_X, INLINE_शब्द> m_inline_संख्या;
        std::vector<मूल्य_And_X> m_dynamic_संख्या;
        std::string m_string;
    };

    पूर्णाङ्क m_width = 0;  // Width (in bits) as specified/calculated
    V3_संख्या_Data_Type m_type;

सार्वजनिक:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_संख्या_Data()
        : m_type{V3_संख्या_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_संख्या_Data() { destroy_Stored_मूल्य(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_संख्या_Data(ध्रुव V3_संख्या_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        यदि (other.is_Inline_संख्या()) {
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा यदि (other.is_Dynamic_संख्या()) {
            init_Dynamic_संख्या(other.m_dynamic_संख्या);
        } अन्यथा यदि (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_संख्या_Data& operator=(ध्रुव V3_संख्या_Data& other) {
        यदि (other.is_Inline_संख्या()) {
            destroy_Stored_मूल्य();
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा यदि (other.is_Dynamic_संख्या()) {
            reinit_With_Or_Assign_Dynamic_संख्या(other.m_dynamic_संख्या);
        } अन्यथा यदि (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } अन्यथा {
            destroy_Stored_मूल्य();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        प्रत्यागम *this;
    }

    V3_संख्या_Data(V3_संख्या_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        यदि (other.is_Inline_संख्या()) {
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा यदि (other.is_Dynamic_संख्या()) {
            init_Dynamic_संख्या(std::move(other.m_dynamic_संख्या));
        } अन्यथा यदि (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_संख्या_Data_Type::UNINITIALIZED;
    }

    V3_संख्या_Data& operator=(V3_संख्या_Data&& other) {
        यदि (other.is_Inline_संख्या()) {
            destroy_Stored_मूल्य();
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा यदि (other.is_Dynamic_संख्या()) {
            reinit_With_Or_Assign_Dynamic_संख्या(std::move(other.m_dynamic_संख्या));
        } अन्यथा यदि (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } अन्यथा {
            destroy_Stored_मूल्य();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_संख्या_Data_Type::UNINITIALIZED;
        प्रत्यागम *this;
    }

    // ACCESSORS
    मूल्य_And_X* अंक() {
        UASSERT(is_संख्या(), "`num` member accessed when data type is " << m_type);
        प्रत्यागम is_Inline_संख्या() ? m_inline_संख्या.data() : m_dynamic_संख्या.data();
    }
    ध्रुव मूल्य_And_X* अंक() ध्रुव VL_MT_SAFE {
        UASSERT(is_संख्या(), "`num` member accessed when data type is " << m_type);
        प्रत्यागम is_Inline_संख्या() ? m_inline_संख्या.data() : m_dynamic_संख्या.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        प्रत्यागम m_string;
    }
    ध्रुव std::string& str() ध्रुव VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        प्रत्यागम m_string;
    }

    पूर्णाङ्क width() ध्रुव VL_MT_SAFE { प्रत्यागम m_width; }
    V3_संख्या_Data_Type type() ध्रुव VL_MT_SAFE { प्रत्यागम m_type; }

    // METHODS
    शून्य resize(पूर्णाङ्क bits_मोजणी) {
        यदि (m_width == bits_मोजणी) प्रत्यागम;
        यदि (bits_To_शब्द(m_width) == bits_To_शब्द(bits_मोजणी)) {
            m_width = bits_मोजणी;
            प्रत्यागम;
        }
        यदि (is_Dynamic_संख्या()) {
            यदि (bits_मोजणी > MAX_INLINE_WIDTH) {
                m_dynamic_संख्या.resize(bits_To_शब्द(bits_मोजणी));
            } अन्यथा {
                ध्रुव auto dynamicBits = std::move(m_dynamic_संख्या);
                destroy_Dynamic_संख्या();
                init_Inline_संख्या();
                std::memcpy(m_inline_संख्या.data(), dynamicBits.data(), परिमाण(m_inline_संख्या));
            }
        } अन्यथा यदि (is_Inline_संख्या()) {
            यदि (bits_मोजणी > MAX_INLINE_WIDTH) {
                ध्रुव auto bits = m_inline_संख्या;
                init_Dynamic_संख्या(bits_To_शब्द(bits_मोजणी));
                std::memcpy(m_dynamic_संख्या.data(), bits.data(), परिमाण(bits));
            }
        }
        m_width = bits_मोजणी;
    }

    शून्य setString() {
        // If there has been a string already it is kept intact.
        यदि (isString()) प्रत्यागम;
        यदि (is_Dynamic_संख्या()) destroy_Dynamic_संख्या();
        initString();
        m_type = V3_संख्या_Data_Type::STRING;
    }
    शून्य setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_संख्या_Data_Type::STRING;
    }
    शून्य setString(ध्रुव std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_संख्या_Data_Type::STRING;
    }

    शून्य setDouble() {
        destroy_Stored_मूल्य();
        यदि (!is_Inline_संख्या()) init_Inline_संख्या();
        m_type = V3_संख्या_Data_Type::DOUBLE;
        resize(64);
    }

    शून्य setLogic() {
        यदि (isString()) destroyString();
        यदि (!is_संख्या()) {
            यदि (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_संख्या();
            } अन्यथा {
                init_Dynamic_संख्या(bits_To_शब्द(m_width));
            }
        }
        m_type = V3_संख्या_Data_Type::LOGIC;
        resize(m_width);
    }

गुप्त:
    स्थिर constexpr पूर्णाङ्क bits_To_शब्द(पूर्णाङ्क bits_मोजणी) VL_PURE { प्रत्यागम (bits_मोजणी + 31) / 32; }

    bool is_संख्या() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_type == V3_संख्या_Data_Type::DOUBLE || m_type == V3_संख्या_Data_Type::LOGIC;
    }
    bool is_Inline_संख्या() ध्रुव VL_MT_SAFE {
        प्रत्यागम (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_संख्या_Data_Type::DOUBLE || m_type == V3_संख्या_Data_Type::LOGIC);
    }
    bool is_Dynamic_संख्या() ध्रुव VL_MT_SAFE {
        प्रत्यागम (m_width > MAX_INLINE_WIDTH) && (m_type == V3_संख्या_Data_Type::LOGIC);
    }
    bool isString() ध्रुव VL_MT_SAFE { प्रत्यागम m_type == V3_संख्या_Data_Type::STRING; }

    template <typename... Args>
    शून्य init_Inline_संख्या(Args&&... args) {
        new (&m_inline_संख्या) std::array<मूल्य_And_X, INLINE_शब्द>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    शून्य init_Dynamic_संख्या(Args&&... args) {
        new (&m_dynamic_संख्या) std::vector<मूल्य_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    शून्य initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    शून्य destroy_Dynamic_संख्या() { m_dynamic_संख्या.~vector(); }
    शून्य destroyString() { m_string.~string(); }
    शून्य destroy_Stored_मूल्य() {
        यदि (isString())
            destroyString();
        अन्यथा यदि (is_Dynamic_संख्या())
            destroy_Dynamic_संख्या();
    }

    template <typename T>
    शून्य reinit_With_Or_Assign_Dynamic_संख्या(T&& s) {
        यदि (is_Dynamic_संख्या()) {
            m_dynamic_संख्या = std::forward<T>(s);
            प्रत्यागम;
        }
        यदि (isString()) destroyString();
        init_Dynamic_संख्या(std::forward<T>(s));
    }
    template <typename T>
    शून्य reinitWithOrAssignString(T&& s) {
        यदि (isString()) {
            m_string = std::forward<T>(s);
            प्रत्यागम;
        }
        यदि (is_Dynamic_संख्या()) destroy_Dynamic_संख्या();
        initString(std::forward<T>(s));
    }
};

class V3_संख्या final {
    // TYPES
    using मूल्य_And_X = V3_संख्या_Data::मूल्य_And_X;
    using V3_संख्या_Data_Type = V3_संख्या_Data::V3_संख्या_Data_Type;

    // MEMBERS
    V3_संख्या_Data m_data;
    Ast_गाठ* m_nodep = nullptr;  // Parent node - for error reporting only
    File_ओळ* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    शून्य setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_संख्या& setSingleBits(वर्ण मूल्य);
    शून्य fillBits(पूर्णाङ्क destLsb, पूर्णाङ्क width, वर्ण मूल्य);
    शून्य copyBits(पूर्णाङ्क destLsb, ध्रुव V3_संख्या& source, पूर्णाङ्क sourceLsb, पूर्णाङ्क width);
    V3_संख्या& setString(ध्रुव string& str) {
        m_data.setString(str);
        प्रत्यागम *this;
    }
    शून्य opCleanThis(bool warnOnTruncation = false);

सार्वजनिक:
    शून्य nodep(Ast_गाठ* nodep) VL_MT_STABLE;
    Ast_गाठ* nodep() ध्रुव VL_MT_SAFE { प्रत्यागम m_nodep; }  // For debug only
    File_ओळ* fileline() ध्रुव VL_MT_SAFE { प्रत्यागम m_fileline; }
    V3_संख्या& setZero();
    V3_संख्या& setQuad(uint64_t मूल्य);
    V3_संख्या& setLong(uint32_t मूल्य);
    V3_संख्या& setLongS(int32_t मूल्य);
    V3_संख्या& setDouble(द्विगुण मूल्य);
    शून्य setBitX0(पूर्णाङ्क bit);
    शून्य setBit(पूर्णाङ्क bit, वर्ण मूल्य) {  // Note: must be initialized as number and pre-zeroed!
        यदि (bit >= m_data.width()) प्रत्यागम;
        ध्रुव uint32_t mask = (1UL << (bit & 31));
        मूल्य_And_X& v = m_data.अंक()[bit / 32];
        यदि (मूल्य == '0' || मूल्य == 0) {
            v.m_मूल्य &= ~mask;
            v.m_मूल्य_X &= ~mask;
        } अन्यथा यदि (मूल्य == '1' || मूल्य == 1) {
            v.m_मूल्य |= mask;
            v.m_मूल्य_X &= ~mask;
        } अन्यथा यदि (मूल्य == 'z' || मूल्य == 2) {
            v.m_मूल्य &= ~mask;
            v.m_मूल्य_X |= mask;
        } अन्यथा {  // X
            v.m_मूल्य |= mask;
            v.m_मूल्य_X |= mask;
        }
    }

गुप्त:
    वर्ण bitIs(पूर्णाङ्क bit) ध्रुव {
        यदि (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            प्रत्यागम '0';
        }
        ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
        प्रत्यागम ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    वर्ण bitIsExtend(पूर्णाङ्क bit, पूर्णाङ्क lbits) ध्रुव {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        यदि (bit < 0) प्रत्यागम '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        यदि (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
            // We do sign extend
            प्रत्यागम ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
        प्रत्यागम ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

सार्वजनिक:
    bool bitIs0(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_संख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_data.width()) प्रत्यागम !bitIsXZ(m_data.width() - 1);
        ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
        प्रत्यागम ((v.m_मूल्य | v.m_मूल्य_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_संख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_data.width()) प्रत्यागम false;
        ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
        प्रत्यागम ((v.m_मूल्य & (1UL << (bit & 31))) && !(v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(पूर्णाङ्क bit) ध्रुव {
        // Correct number of zero bits/width matters
        यदि (!is_संख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_data.width()) प्रत्यागम bitIs1Extend(m_data.width() - 1);
        ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
        प्रत्यागम ((v.m_मूल्य & (1UL << (bit & 31))) && !(v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsX(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_संख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_data.width()) प्रत्यागम bitIsZ(m_data.width() - 1);
        ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
        प्रत्यागम ((v.m_मूल्य & (1UL << (bit & 31))) && (v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_संख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_data.width()) प्रत्यागम bitIsXZ(m_data.width() - 1);
        ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
        प्रत्यागम ((v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_संख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_data.width()) प्रत्यागम bitIsZ(m_data.width() - 1);
        ध्रुव मूल्य_And_X v = m_data.अंक()[bit / 32];
        प्रत्यागम ((~v.m_मूल्य & (1UL << (bit & 31))) && (v.m_मूल्य_X & (1UL << (bit & 31))));
    }

गुप्त:
    uint32_t bits_मूल्य(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE {
        uint32_t v = 0;
        प्रति (पूर्णाङ्क bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        प्रत्यागम v;
    }

    पूर्णाङ्क मोजणी_X(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE;
    पूर्णाङ्क मोजणी_Z(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE;

    पूर्णाङ्क शब्द() ध्रुव VL_MT_SAFE { प्रत्यागम ((width() + 31) / 32); }
    uint32_t hiWordMask() ध्रुव VL_MT_SAFE {
        // Correct number of zero bits/width matters
        प्रत्यागम VL_MASK_I(width());
    }

    V3_संख्या& opModDivGuts(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, bool is_modulus);

सार्वजनिक:
    // CONSTRUCTORS
    explicit V3_संख्या(Ast_गाठ* nodep) { init(nodep, 1); }
    V3_संख्या(Ast_गाठ* nodep, पूर्णाङ्क width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_संख्या(Ast_गाठ* nodep, पूर्णाङ्क width, ध्रुव V3_संख्या& मूल्य) {
        init(nodep, width, width > 0);
        opAssign(मूल्य);
    }
    V3_संख्या(Ast_गाठ* nodep, पूर्णाङ्क width, uint32_t मूल्य, bool sized = true) {
        init(nodep, width, sized);
        m_data.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
    }
    V3_संख्या(File_ओळ* flp, पूर्णाङ्क width, uint32_t मूल्य, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_संख्या_Literal {};  // For creator type-overload selection
    V3_संख्या(Ast_गाठ* nodep, Verilog_संख्या_Literal, ध्रुव वर्ण* sourcep) { create(nodep, sourcep); }
    V3_संख्या(File_ओळ* flp, Verilog_संख्या_Literal, ध्रुव वर्ण* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_संख्या(Ast_गाठ* nodep, VerilogStringLiteral, ध्रुव string& str);
    class Double {};
    V3_संख्या(Ast_गाठ* nodep, Double, द्विगुण मूल्य) {
        init(nodep, 64);
        setDouble(मूल्य);
    }
    class String {};
    V3_संख्या(Ast_गाठ* nodep, String, ध्रुव string& मूल्य) {
        init(nodep);
        setString(मूल्य);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_संख्या(Ast_गाठ* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_संख्या(Ast_गाठ* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_संख्या(ध्रुव V3_संख्या* nump, पूर्णाङ्क width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_संख्या(ध्रुव V3_संख्या* nump, पूर्णाङ्क width, uint32_t मूल्य) {
        init(nullptr, width);
        m_data.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_संख्या(Ast_गाठ* nodep, ध्रुव Ast_गाठ_DType* nodedtypep);

    V3_संख्या(ध्रुव V3_संख्या& other) = स्वभाव;
    V3_संख्या& operator=(ध्रुव V3_संख्या& other) = स्वभाव;

    V3_संख्या(V3_संख्या&& other) = स्वभाव;
    V3_संख्या& operator=(V3_संख्या&& other) = स्वभाव;

    ~V3_संख्या() = स्वभाव;

गुप्त:
    शून्य selfTestThis();
    शून्य create(Ast_गाठ* nodep, ध्रुव वर्ण* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    शून्य create(File_ओळ* flp, ध्रुव वर्ण* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    शून्य create(ध्रुव वर्ण* sourcep);
    शून्य init(Ast_गाठ* nodep, पूर्णाङ्क swidth = -1, bool sized = true) {
        this->nodep(nodep);
        यदि (swidth >= 0) {
            यदि (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            प्रति (पूर्णाङ्क i = 0; i < शब्द(); ++i) m_data.अंक()[i] = {0, 0};
        } अन्यथा {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    स्थिर string displayPad(size_t fmtsize, वर्ण pad, bool डावा, ध्रुव string& in) VL_PURE;
    string displayed(ध्रुव string& vformat) ध्रुव VL_MT_STABLE {
        प्रत्यागम displayed(m_fileline, vformat);
    }
    शून्य warnTooMany(ध्रुव string& मूल्य);

सार्वजनिक:
    शून्य v3errorEnd(ध्रुव std::ostringstream& sstr) ध्रुव VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(ध्रुव std::ostringstream& sstr) ध्रुव VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    शून्य width(पूर्णाङ्क width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_संख्या& setAllBitsXRemoved();
    V3_संख्या& setAllBitsX();
    V3_संख्या& setAllBitsZ();
    V3_संख्या& setAllBits0();
    V3_संख्या& setAllBits1();
    V3_संख्या& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_संख्या& setMask(पूर्णाङ्क nbits, पूर्णाङ्क lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ध्रुव VL_MT_STABLE;
    string displayed(ध्रुव Ast_गाठ* nodep, ध्रुव string& vformat,
                     ध्रुव VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ध्रुव VL_MT_STABLE;
    string displayed(File_ओळ* fl, ध्रुव string& vformat,
                     ध्रुव VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ध्रुव VL_MT_STABLE;
    string displayedEnum(ध्रुव AstSFormatArg* argp, ध्रुव string& vformat) ध्रुव VL_MT_STABLE;
    string displayedEnumName(ध्रुव AstEnumDType* dtypep) ध्रुव VL_MT_STABLE;
    स्थिर string displayedEnumName(ध्रुव AstEnumItem* itemp);
    स्थिर bool displayedFmtHasArg(वर्ण format, bool isScan);
    string emitC() ध्रुव VL_MT_STABLE;
    पूर्णाङ्क width() ध्रुव VL_MT_SAFE { प्रत्यागम m_data.width(); }
    पूर्णाङ्क widthToFit() ध्रुव;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ध्रुव VL_MT_SAFE { प्रत्यागम m_data.m_sized; }
    bool autoExtend() ध्रुव VL_MT_SAFE { प्रत्यागम m_data.m_autoExtend; }
    bool isFromString() ध्रुव { प्रत्यागम m_data.m_fromString; }
    bool hasOrigParamName() ध्रुव { प्रत्यागम m_data.m_hasOrigParamName; }
    शून्य hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_संख्या_Data_Type dataType() ध्रुव VL_MT_SAFE { प्रत्यागम m_data.type(); }
    शून्य dataType(V3_संख्या_Data_Type newType) {
        यदि (dataType() == newType) प्रत्यागम;
        UASSERT(newType != V3_संख्या_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        विकल्प (newType) {
        पक्ष V3_संख्या_Data_Type::STRING: m_data.setString(); भङ्ग;
        पक्ष V3_संख्या_Data_Type::DOUBLE: m_data.setDouble(); भङ्ग;
        पक्ष V3_संख्या_Data_Type::LOGIC: m_data.setLogic(); भङ्ग;
        पक्ष V3_संख्या_Data_Type::UNINITIALIZED: भङ्ग;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ध्रुव VL_MT_SAFE { प्रत्यागम m_data.m_signed; }
    शून्य isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() ध्रुव VL_MT_SAFE { प्रत्यागम dataType() == V3_संख्या_Data_Type::DOUBLE; }
    bool isString() ध्रुव VL_MT_SAFE { प्रत्यागम dataType() == V3_संख्या_Data_Type::STRING; }
    bool isOpaque() ध्रुव VL_MT_SAFE { प्रत्यागम isDouble() || isString(); }
    bool is_संख्या() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_data.type() == V3_संख्या_Data_Type::LOGIC
               || m_data.type() == V3_संख्या_Data_Type::DOUBLE;
    }
    bool isNegative() ध्रुव VL_MT_SAFE {
        // Correct number of zero bits/width matters
        प्रत्यागम !isString() && bitIs1(width() - 1);
    }
    bool is1Step() ध्रुव VL_MT_SAFE { प्रत्यागम m_data.m_is1Step; }
    bool isNull() ध्रुव VL_MT_SAFE { प्रत्यागम m_data.m_isNull; }
    bool isFourState() ध्रुव VL_MT_SAFE;
    bool hasZ() ध्रुव {
        यदि (isString()) प्रत्यागम false;
        प्रति (पूर्णाङ्क i = 0; i < शब्द(); ++i) {
            ध्रुव मूल्य_And_X v = m_data.अंक()[i];
            यदि ((~v.m_मूल्य) & v.m_मूल्य_X) प्रत्यागम true;
        }
        प्रत्यागम false;
    }
    bool isAllZ() ध्रुव VL_MT_SAFE;
    bool isAllX() ध्रुव VL_MT_SAFE;
    bool isEqZero() ध्रुव VL_MT_SAFE;
    bool isNeqZero() ध्रुव;
    bool isBitsZero(पूर्णाङ्क msb, पूर्णाङ्क lsb) ध्रुव;
    bool isBroken(पूर्णाङ्क vwidth) ध्रुव;
    bool isEqOne() ध्रुव;
    bool isEqAllOnes(पूर्णाङ्क optwidth = 0) ध्रुव;
    bool isCaseEq(ध्रुव V3_संख्या& rhs) ध्रुव;  // operator==
    bool isLtXZ(ध्रुव V3_संख्या& rhs) ध्रुव;  // operator< with XZ compared
    bool isAnyX() ध्रुव VL_MT_SAFE;
    bool isAnyXZ() ध्रुव;
    bool isAnyZ() ध्रुव VL_MT_SAFE;
    bool isMsbXZ() ध्रुव {
        // Correct number of zero bits/width matters
        प्रत्यागम bitIsXZ(width() - 1);
    }
    bool fitsInUInt() ध्रुव VL_MT_SAFE;
    uint32_t toUInt() ध्रुव VL_MT_SAFE;
    int32_t toSInt() ध्रुव VL_MT_SAFE;
    uint64_t toUQuad() ध्रुव VL_MT_SAFE;
    int64_t toSQuad() ध्रुव VL_MT_SAFE;
    string toString() ध्रुव VL_MT_SAFE;
    string toDecimalS() ध्रुव VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() ध्रुव VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() ध्रुव VL_MT_STABLE;
    द्विगुण toDouble() ध्रुव VL_MT_SAFE;
    V3Hash toHash() ध्रुव;
    uint32_t edataWord(पूर्णाङ्क eword) ध्रुव;
    uint8_t dataByte(पूर्णाङ्क byte) ध्रुव;
    uint32_t मोजणी_Bits(ध्रुव V3_संख्या& ctrl) ध्रुव;
    uint32_t मोजणी_Bits(ध्रुव V3_संख्या& ctrl1, ध्रुव V3_संख्या& ctrl2, ध्रुव V3_संख्या& ctrl3) ध्रुव;
    uint32_t मोजणी_Ones() ध्रुव;
    uint32_t mostSetBitP1() ध्रुव;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() ध्रुव;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(ध्रुव V3_संख्या& rhs) ध्रुव { प्रत्यागम isLtXZ(rhs); }

    // STATICS
    स्थिर bool epsilonEqual(द्विगुण a, द्विगुण b);  // True if number within Epsilon of the other
    स्थिर bool epsilonIntegral(द्विगुण a);  // True if number rounds to integer within Epsilon
    स्थिर पूर्णाङ्क log2b(uint32_t अंक);
    स्थिर पूर्णाङ्क log2bQuad(uint64_t अंक);
    स्थिर शून्य selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_संख्या& opBitsNonXZ(ध्रुव V3_संख्या& lhs);  // 0/1->1, X/Z->0
    V3_संख्या& opBitsOne(ध्रुव V3_संख्या& lhs);  // 1->1, 0/X/Z->0
    V3_संख्या& opBitsXZ(ध्रुव V3_संख्या& lhs);  // 0/1->0, X/Z->1
    V3_संख्या& opBitsZ(ध्रुव V3_संख्या& lhs);  // Z->1, 0/1/X->0
    //
    V3_संख्या& opAssign(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opAssignNonXZ(ध्रुव V3_संख्या& lhs, bool ignoreXZ = true);
    V3_संख्या& opExtendS(ध्रुव V3_संख्या& lhs, uint32_t lbits);  // Sign extension
    V3_संख्या& opExtendXZ(ध्रुव V3_संख्या& lhs, uint32_t lbits);  // X/Z extension
    V3_संख्या& opRedOr(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opRedAnd(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opRedXor(ध्रुव V3_संख्या& lhs);
    V3_संख्या& op_मोजणी_Bits(ध्रुव V3_संख्या& अभिव्यक्ती, ध्रुव V3_संख्या& ctrl1, ध्रुव V3_संख्या& ctrl2,
                          ध्रुव V3_संख्या& ctrl3);
    V3_संख्या& op_मोजणी_Ones(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opIsUnknown(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opOneHot(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opOneHot0(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opCLog2(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opMostSetBitP1(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opClean(ध्रुव V3_संख्या& lhs, uint32_t bits);
    V3_संख्या& opConcat(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLenN(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opRepl(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opRepl(ध्रुव V3_संख्या& lhs, uint32_t rhsval);
    V3_संख्या& opStreamL(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opSel(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& msb, ध्रुव V3_संख्या& lsb);
    V3_संख्या& opSel(ध्रुव V3_संख्या& lhs, uint32_t msbval, uint32_t lsbval);
    V3_संख्या& opSelInto(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& lsb, पूर्णाङ्क width);
    V3_संख्या& opSelInto(ध्रुव V3_संख्या& lhs, पूर्णाङ्क lsbval, पूर्णाङ्क width);
    V3_संख्या& opCvtPackString(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opToLowerN(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opToUpperN(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opCaseEq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opCaseNeq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opWildEq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opWildNeq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opBufIf1(ध्रुव V3_संख्या& ens, ध्रुव V3_संख्या& if1s);
    V3_संख्या& opSetRange(uint32_t lsb, uint32_t width, वर्ण bit_मूल्य);
    // "standard" math
    V3_संख्या& opNot(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opLogNot(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opLogAnd(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLogOr(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLogEq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLogIf(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opNegate(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opAdd(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opSub(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opMul(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opMulS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed
    V3_संख्या& opDiv(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opDivS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed
    V3_संख्या& opModDiv(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opModDivS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed
    V3_संख्या& opPow(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, bool lsign = false,
                    bool rsign = false);
    V3_संख्या& opPowSU(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed lhs, unsigned rhs
    V3_संख्या& opPowSS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed lhs, signed rhs
    V3_संख्या& opPowUS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Unsigned lhs, signed rhs
    V3_संख्या& opAnd(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opXor(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opOr(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opShiftR(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opShiftRS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_संख्या& opShiftL(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    // Comparisons
    V3_संख्या& opEq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opNeq(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opGt(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opGtS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed
    V3_संख्या& opGte(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opGteS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed
    V3_संख्या& opLt(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLtS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed
    V3_संख्या& opLte(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLteS(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);  // Signed

    // "D" - double (aka real) math
    V3_संख्या& opIToRD(ध्रुव V3_संख्या& lhs, bool isSigned = false);
    V3_संख्या& opISToRD(ध्रुव V3_संख्या& lhs) { प्रत्यागम opIToRD(lhs, true); }
    V3_संख्या& opRToIS(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opRToIRoundS(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opRealToBits(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opBitsToRealD(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opNegateD(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opAddD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opSubD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opMulD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opDivD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opPowD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    // Comparisons
    V3_संख्या& opEqD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opNeqD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opGtD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opGteD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLtD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLteD(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);

    // "N" - string operations
    V3_संख्या& opAtoN(ध्रुव V3_संख्या& lhs, पूर्णाङ्क base);
    V3_संख्या& opNToI(ध्रुव V3_संख्या& lhs);
    V3_संख्या& opPutcN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, ध्रुव V3_संख्या& ths);
    V3_संख्या& opGetcN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opSubstrN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, ध्रुव V3_संख्या& ths);
    V3_संख्या& opCompareNN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs, bool ignoreCase);
    V3_संख्या& opConcatN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opReplN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opReplN(ध्रुव V3_संख्या& lhs, uint32_t rhsval);
    V3_संख्या& opEqN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opNeqN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opGtN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opGteN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLtN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
    V3_संख्या& opLteN(ध्रुव V3_संख्या& lhs, ध्रुव V3_संख्या& rhs);
};
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव V3_संख्या& rhs) VL_MT_SAFE {
    प्रत्यागम os << rhs.ascii();
}

#endif  // Guard
