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

#ifndef VERILATOR_V3_सङ्ख्या_H
#define VERILATOR_V3_सङ्ख्या_H

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
class Ast_गाँठो;
class Ast_गाँठो_DType;
class AstSFormatArg;
class File_पङ्क्ति;

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
    constexpr चालक en() अचर { फर्काऊ m_e; }
    वर्ण ascii() अचर { फर्काऊ m_e; }
    bool isComplex() अचर { फर्काऊ m_e == COMPLEX; }
    bool isDouble() अचर { फर्काऊ m_e == DOUBLE; }
    bool isEnum() अचर { फर्काऊ m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() अचर { फर्काऊ m_e == SIGNED; }
    bool isString() अचर { फर्काऊ m_e == STRING; }
    bool isUnsigned() अचर { फर्काऊ m_e == UNSIGNED; }
};
constexpr bool चालक==(अचर VFormatAttr& lhs, अचर VFormatAttr& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VFormatAttr& lhs, VFormatAttr::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VFormatAttr::en lhs, अचर VFormatAttr& rhs) { फर्काऊ lhs == rhs.m_e; }
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VFormatAttr& rhs) {
    फर्काऊ os << rhs.ascii();
}

//============================================================================

class V3_सङ्ख्या_Data final {
सार्वजनिक:
    // TYPES
    संरचना मान_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_मान;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_मान_X;
        bool चालक==(अचर मान_And_X& other) अचर VL_MT_SAFE {
            फर्काऊ m_मान == other.m_मान && m_मान_X == other.m_मान_X;
        }
    };

    गणना class V3_सङ्ख्या_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    मित्र std::ostream& चालक<<(std::ostream& os, अचर V3_सङ्ख्या_Data_Type& rhs) VL_MT_SAFE {
        विकल्प (rhs) {
        पक्ष V3_सङ्ख्या_Data_Type::UNINITIALIZED: फर्काऊ os << "UNINITIALIZED";
        पक्ष V3_सङ्ख्या_Data_Type::LOGIC: फर्काऊ os << "LOGIC";
        पक्ष V3_सङ्ख्या_Data_Type::DOUBLE: फर्काऊ os << "DOUBLE";
        पक्ष V3_सङ्ख्या_Data_Type::STRING: फर्काऊ os << "STRING";
        }
        फर्काऊ os;
    }

गुप्त:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    स्थिर constexpr पूर्णाङ्क INLINE_शब्दहरू = std::max(
        static_cast<माप_प्रकार>(2), std::max(परिमाण(std::string) / परिमाण(मान_And_X),
                                         परिमाण(std::vector<मान_And_X>) / परिमाण(मान_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    स्थिर constexpr पूर्णाङ्क MAX_INLINE_WIDTH = INLINE_शब्दहरू * परिमाण(मान_And_X) / 2 * 8;

    // MEMBERS
    सङ्घ {
        std::array<मान_And_X, INLINE_शब्दहरू> m_inline_सङ्ख्या;
        std::vector<मान_And_X> m_dynamic_सङ्ख्या;
        std::string m_string;
    };

    पूर्णाङ्क m_width = 0;  // Width (in bits) as specified/calculated
    V3_सङ्ख्या_Data_Type m_type;

सार्वजनिक:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_सङ्ख्या_Data()
        : m_type{V3_सङ्ख्या_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_सङ्ख्या_Data() { destroy_Stored_मान(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_सङ्ख्या_Data(अचर V3_सङ्ख्या_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        यदि (other.is_Inline_सङ्ख्या()) {
            init_Inline_सङ्ख्या(other.m_inline_सङ्ख्या);
        } अन्यथा यदि (other.is_Dynamic_सङ्ख्या()) {
            init_Dynamic_सङ्ख्या(other.m_dynamic_सङ्ख्या);
        } अन्यथा यदि (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_सङ्ख्या_Data& चालक=(अचर V3_सङ्ख्या_Data& other) {
        यदि (other.is_Inline_सङ्ख्या()) {
            destroy_Stored_मान();
            init_Inline_सङ्ख्या(other.m_inline_सङ्ख्या);
        } अन्यथा यदि (other.is_Dynamic_सङ्ख्या()) {
            reinit_With_Or_Assign_Dynamic_सङ्ख्या(other.m_dynamic_सङ्ख्या);
        } अन्यथा यदि (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } अन्यथा {
            destroy_Stored_मान();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        फर्काऊ *this;
    }

    V3_सङ्ख्या_Data(V3_सङ्ख्या_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        यदि (other.is_Inline_सङ्ख्या()) {
            init_Inline_सङ्ख्या(other.m_inline_सङ्ख्या);
        } अन्यथा यदि (other.is_Dynamic_सङ्ख्या()) {
            init_Dynamic_सङ्ख्या(std::move(other.m_dynamic_सङ्ख्या));
        } अन्यथा यदि (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_सङ्ख्या_Data_Type::UNINITIALIZED;
    }

    V3_सङ्ख्या_Data& चालक=(V3_सङ्ख्या_Data&& other) {
        यदि (other.is_Inline_सङ्ख्या()) {
            destroy_Stored_मान();
            init_Inline_सङ्ख्या(other.m_inline_सङ्ख्या);
        } अन्यथा यदि (other.is_Dynamic_सङ्ख्या()) {
            reinit_With_Or_Assign_Dynamic_सङ्ख्या(std::move(other.m_dynamic_सङ्ख्या));
        } अन्यथा यदि (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } अन्यथा {
            destroy_Stored_मान();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_सङ्ख्या_Data_Type::UNINITIALIZED;
        फर्काऊ *this;
    }

    // ACCESSORS
    मान_And_X* अङ्क() {
        UASSERT(is_सङ्ख्या(), "`num` member accessed when data type is " << m_type);
        फर्काऊ is_Inline_सङ्ख्या() ? m_inline_सङ्ख्या.data() : m_dynamic_सङ्ख्या.data();
    }
    अचर मान_And_X* अङ्क() अचर VL_MT_SAFE {
        UASSERT(is_सङ्ख्या(), "`num` member accessed when data type is " << m_type);
        फर्काऊ is_Inline_सङ्ख्या() ? m_inline_सङ्ख्या.data() : m_dynamic_सङ्ख्या.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        फर्काऊ m_string;
    }
    अचर std::string& str() अचर VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        फर्काऊ m_string;
    }

    पूर्णाङ्क width() अचर VL_MT_SAFE { फर्काऊ m_width; }
    V3_सङ्ख्या_Data_Type type() अचर VL_MT_SAFE { फर्काऊ m_type; }

    // METHODS
    शून्य resize(पूर्णाङ्क bits_गन्ती) {
        यदि (m_width == bits_गन्ती) फर्काऊ;
        यदि (bits_To_शब्दहरू(m_width) == bits_To_शब्दहरू(bits_गन्ती)) {
            m_width = bits_गन्ती;
            फर्काऊ;
        }
        यदि (is_Dynamic_सङ्ख्या()) {
            यदि (bits_गन्ती > MAX_INLINE_WIDTH) {
                m_dynamic_सङ्ख्या.resize(bits_To_शब्दहरू(bits_गन्ती));
            } अन्यथा {
                अचर स्वतः dynamicBits = std::move(m_dynamic_सङ्ख्या);
                destroy_Dynamic_सङ्ख्या();
                init_Inline_सङ्ख्या();
                std::स_नकल(m_inline_सङ्ख्या.data(), dynamicBits.data(), परिमाण(m_inline_सङ्ख्या));
            }
        } अन्यथा यदि (is_Inline_सङ्ख्या()) {
            यदि (bits_गन्ती > MAX_INLINE_WIDTH) {
                अचर स्वतः bits = m_inline_सङ्ख्या;
                init_Dynamic_सङ्ख्या(bits_To_शब्दहरू(bits_गन्ती));
                std::स_नकल(m_dynamic_सङ्ख्या.data(), bits.data(), परिमाण(bits));
            }
        }
        m_width = bits_गन्ती;
    }

    शून्य setString() {
        // If there has been a string already it is kept intact.
        यदि (isString()) फर्काऊ;
        यदि (is_Dynamic_सङ्ख्या()) destroy_Dynamic_सङ्ख्या();
        initString();
        m_type = V3_सङ्ख्या_Data_Type::STRING;
    }
    शून्य setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_सङ्ख्या_Data_Type::STRING;
    }
    शून्य setString(अचर std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_सङ्ख्या_Data_Type::STRING;
    }

    शून्य setDouble() {
        destroy_Stored_मान();
        यदि (!is_Inline_सङ्ख्या()) init_Inline_सङ्ख्या();
        m_type = V3_सङ्ख्या_Data_Type::DOUBLE;
        resize(64);
    }

    शून्य setLogic() {
        यदि (isString()) destroyString();
        यदि (!is_सङ्ख्या()) {
            यदि (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_सङ्ख्या();
            } अन्यथा {
                init_Dynamic_सङ्ख्या(bits_To_शब्दहरू(m_width));
            }
        }
        m_type = V3_सङ्ख्या_Data_Type::LOGIC;
        resize(m_width);
    }

गुप्त:
    स्थिर constexpr पूर्णाङ्क bits_To_शब्दहरू(पूर्णाङ्क bits_गन्ती) VL_PURE { फर्काऊ (bits_गन्ती + 31) / 32; }

    bool is_सङ्ख्या() अचर VL_MT_SAFE {
        फर्काऊ m_type == V3_सङ्ख्या_Data_Type::DOUBLE || m_type == V3_सङ्ख्या_Data_Type::LOGIC;
    }
    bool is_Inline_सङ्ख्या() अचर VL_MT_SAFE {
        फर्काऊ (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_सङ्ख्या_Data_Type::DOUBLE || m_type == V3_सङ्ख्या_Data_Type::LOGIC);
    }
    bool is_Dynamic_सङ्ख्या() अचर VL_MT_SAFE {
        फर्काऊ (m_width > MAX_INLINE_WIDTH) && (m_type == V3_सङ्ख्या_Data_Type::LOGIC);
    }
    bool isString() अचर VL_MT_SAFE { फर्काऊ m_type == V3_सङ्ख्या_Data_Type::STRING; }

    ढाँचा <typename... Args>
    शून्य init_Inline_सङ्ख्या(Args&&... args) {
        new (&m_inline_सङ्ख्या) std::array<मान_And_X, INLINE_शब्दहरू>(std::forward<Args>(args)...);
    }
    ढाँचा <typename... Args>
    शून्य init_Dynamic_सङ्ख्या(Args&&... args) {
        new (&m_dynamic_सङ्ख्या) std::vector<मान_And_X>(std::forward<Args>(args)...);
    }
    ढाँचा <typename... Args>
    शून्य initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    शून्य destroy_Dynamic_सङ्ख्या() { m_dynamic_सङ्ख्या.~vector(); }
    शून्य destroyString() { m_string.~string(); }
    शून्य destroy_Stored_मान() {
        यदि (isString())
            destroyString();
        अन्यथा यदि (is_Dynamic_सङ्ख्या())
            destroy_Dynamic_सङ्ख्या();
    }

    ढाँचा <typename T>
    शून्य reinit_With_Or_Assign_Dynamic_सङ्ख्या(T&& s) {
        यदि (is_Dynamic_सङ्ख्या()) {
            m_dynamic_सङ्ख्या = std::forward<T>(s);
            फर्काऊ;
        }
        यदि (isString()) destroyString();
        init_Dynamic_सङ्ख्या(std::forward<T>(s));
    }
    ढाँचा <typename T>
    शून्य reinitWithOrAssignString(T&& s) {
        यदि (isString()) {
            m_string = std::forward<T>(s);
            फर्काऊ;
        }
        यदि (is_Dynamic_सङ्ख्या()) destroy_Dynamic_सङ्ख्या();
        initString(std::forward<T>(s));
    }
};

class V3_सङ्ख्या final {
    // TYPES
    using मान_And_X = V3_सङ्ख्या_Data::मान_And_X;
    using V3_सङ्ख्या_Data_Type = V3_सङ्ख्या_Data::V3_सङ्ख्या_Data_Type;

    // MEMBERS
    V3_सङ्ख्या_Data m_data;
    Ast_गाँठो* m_nodep = nullptr;  // Parent node - for error reporting only
    File_पङ्क्ति* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    शून्य setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_सङ्ख्या& setSingleBits(वर्ण मान);
    शून्य fillBits(पूर्णाङ्क destLsb, पूर्णाङ्क width, वर्ण मान);
    शून्य copyBits(पूर्णाङ्क destLsb, अचर V3_सङ्ख्या& source, पूर्णाङ्क sourceLsb, पूर्णाङ्क width);
    V3_सङ्ख्या& setString(अचर string& str) {
        m_data.setString(str);
        फर्काऊ *this;
    }
    शून्य opCleanThis(bool warnOnTruncation = false);

सार्वजनिक:
    शून्य nodep(Ast_गाँठो* nodep) VL_MT_STABLE;
    Ast_गाँठो* nodep() अचर VL_MT_SAFE { फर्काऊ m_nodep; }  // For debug only
    File_पङ्क्ति* fileline() अचर VL_MT_SAFE { फर्काऊ m_fileline; }
    V3_सङ्ख्या& setZero();
    V3_सङ्ख्या& setQuad(uint64_t मान);
    V3_सङ्ख्या& setLong(uint32_t मान);
    V3_सङ्ख्या& setLongS(int32_t मान);
    V3_सङ्ख्या& setDouble(द्विगुण मान);
    शून्य setBitX0(पूर्णाङ्क bit);
    शून्य setBit(पूर्णाङ्क bit, वर्ण मान) {  // Note: must be initialized as number and pre-zeroed!
        यदि (bit >= m_data.width()) फर्काऊ;
        अचर uint32_t mask = (1UL << (bit & 31));
        मान_And_X& v = m_data.अङ्क()[bit / 32];
        यदि (मान == '0' || मान == 0) {
            v.m_मान &= ~mask;
            v.m_मान_X &= ~mask;
        } अन्यथा यदि (मान == '1' || मान == 1) {
            v.m_मान |= mask;
            v.m_मान_X &= ~mask;
        } अन्यथा यदि (मान == 'z' || मान == 2) {
            v.m_मान &= ~mask;
            v.m_मान_X |= mask;
        } अन्यथा {  // X
            v.m_मान |= mask;
            v.m_मान_X |= mask;
        }
    }

गुप्त:
    वर्ण bitIs(पूर्णाङ्क bit) अचर {
        यदि (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            फर्काऊ '0';
        }
        अचर मान_And_X v = m_data.अङ्क()[bit / 32];
        फर्काऊ ("01zx"[(((v.m_मान & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मान_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    वर्ण bitIsExtend(पूर्णाङ्क bit, पूर्णाङ्क lbits) अचर {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        यदि (bit < 0) फर्काऊ '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        यदि (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            अचर मान_And_X v = m_data.अङ्क()[bit / 32];
            // We do sign extend
            फर्काऊ ("01zx"[(((v.m_मान & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_मान_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        अचर मान_And_X v = m_data.अङ्क()[bit / 32];
        फर्काऊ ("01zx"[(((v.m_मान & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मान_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

सार्वजनिक:
    bool bitIs0(पूर्णाङ्क bit) अचर VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) फर्काऊ false;
        यदि (bit < 0) फर्काऊ false;
        यदि (bit >= m_data.width()) फर्काऊ !bitIsXZ(m_data.width() - 1);
        अचर मान_And_X v = m_data.अङ्क()[bit / 32];
        फर्काऊ ((v.m_मान | v.m_मान_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(पूर्णाङ्क bit) अचर VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) फर्काऊ false;
        यदि (bit < 0) फर्काऊ false;
        यदि (bit >= m_data.width()) फर्काऊ false;
        अचर मान_And_X v = m_data.अङ्क()[bit / 32];
        फर्काऊ ((v.m_मान & (1UL << (bit & 31))) && !(v.m_मान_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(पूर्णाङ्क bit) अचर {
        // Correct number of zero bits/width matters
        यदि (!is_सङ्ख्या()) फर्काऊ false;
        यदि (bit < 0) फर्काऊ false;
        यदि (bit >= m_data.width()) फर्काऊ bitIs1Extend(m_data.width() - 1);
        अचर मान_And_X v = m_data.अङ्क()[bit / 32];
        फर्काऊ ((v.m_मान & (1UL << (bit & 31))) && !(v.m_मान_X & (1UL << (bit & 31))));
    }
    bool bitIsX(पूर्णाङ्क bit) अचर VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) फर्काऊ false;
        यदि (bit < 0) फर्काऊ false;
        यदि (bit >= m_data.width()) फर्काऊ bitIsZ(m_data.width() - 1);
        अचर मान_And_X v = m_data.अङ्क()[bit / 32];
        फर्काऊ ((v.m_मान & (1UL << (bit & 31))) && (v.m_मान_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(पूर्णाङ्क bit) अचर VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) फर्काऊ false;
        यदि (bit < 0) फर्काऊ false;
        यदि (bit >= m_data.width()) फर्काऊ bitIsXZ(m_data.width() - 1);
        अचर मान_And_X v = m_data.अङ्क()[bit / 32];
        फर्काऊ ((v.m_मान_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(पूर्णाङ्क bit) अचर VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) फर्काऊ false;
        यदि (bit < 0) फर्काऊ false;
        यदि (bit >= m_data.width()) फर्काऊ bitIsZ(m_data.width() - 1);
        अचर मान_And_X v = m_data.अङ्क()[bit / 32];
        फर्काऊ ((~v.m_मान & (1UL << (bit & 31))) && (v.m_मान_X & (1UL << (bit & 31))));
    }

गुप्त:
    uint32_t bits_मान(पूर्णाङ्क lsb, पूर्णाङ्क nbits) अचर VL_MT_SAFE {
        uint32_t v = 0;
        लागि (पूर्णाङ्क bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        फर्काऊ v;
    }

    पूर्णाङ्क गन्ती_X(पूर्णाङ्क lsb, पूर्णाङ्क nbits) अचर VL_MT_SAFE;
    पूर्णाङ्क गन्ती_Z(पूर्णाङ्क lsb, पूर्णाङ्क nbits) अचर VL_MT_SAFE;

    पूर्णाङ्क शब्दहरू() अचर VL_MT_SAFE { फर्काऊ ((width() + 31) / 32); }
    uint32_t hiWordMask() अचर VL_MT_SAFE {
        // Correct number of zero bits/width matters
        फर्काऊ VL_MASK_I(width());
    }

    V3_सङ्ख्या& opModDivGuts(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, bool is_modulus);

सार्वजनिक:
    // CONSTRUCTORS
    explicit V3_सङ्ख्या(Ast_गाँठो* nodep) { init(nodep, 1); }
    V3_सङ्ख्या(Ast_गाँठो* nodep, पूर्णाङ्क width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_सङ्ख्या(Ast_गाँठो* nodep, पूर्णाङ्क width, अचर V3_सङ्ख्या& मान) {
        init(nodep, width, width > 0);
        opAssign(मान);
    }
    V3_सङ्ख्या(Ast_गाँठो* nodep, पूर्णाङ्क width, uint32_t मान, bool sized = true) {
        init(nodep, width, sized);
        m_data.अङ्क()[0].m_मान = मान;
        opCleanThis();
    }
    V3_सङ्ख्या(File_पङ्क्ति* flp, पूर्णाङ्क width, uint32_t मान, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.अङ्क()[0].m_मान = मान;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_सङ्ख्या_Literal {};  // For creator type-overload selection
    V3_सङ्ख्या(Ast_गाँठो* nodep, Verilog_सङ्ख्या_Literal, अचर वर्ण* sourcep) { create(nodep, sourcep); }
    V3_सङ्ख्या(File_पङ्क्ति* flp, Verilog_सङ्ख्या_Literal, अचर वर्ण* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_सङ्ख्या(Ast_गाँठो* nodep, VerilogStringLiteral, अचर string& str);
    class Double {};
    V3_सङ्ख्या(Ast_गाँठो* nodep, Double, द्विगुण मान) {
        init(nodep, 64);
        setDouble(मान);
    }
    class String {};
    V3_सङ्ख्या(Ast_गाँठो* nodep, String, अचर string& मान) {
        init(nodep);
        setString(मान);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_सङ्ख्या(Ast_गाँठो* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_सङ्ख्या(Ast_गाँठो* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_सङ्ख्या(अचर V3_सङ्ख्या* nump, पूर्णाङ्क width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_सङ्ख्या(अचर V3_सङ्ख्या* nump, पूर्णाङ्क width, uint32_t मान) {
        init(nullptr, width);
        m_data.अङ्क()[0].m_मान = मान;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_सङ्ख्या(Ast_गाँठो* nodep, अचर Ast_गाँठो_DType* nodedtypep);

    V3_सङ्ख्या(अचर V3_सङ्ख्या& other) = स्वभाव;
    V3_सङ्ख्या& चालक=(अचर V3_सङ्ख्या& other) = स्वभाव;

    V3_सङ्ख्या(V3_सङ्ख्या&& other) = स्वभाव;
    V3_सङ्ख्या& चालक=(V3_सङ्ख्या&& other) = स्वभाव;

    ~V3_सङ्ख्या() = स्वभाव;

गुप्त:
    शून्य selfTestThis();
    शून्य create(Ast_गाँठो* nodep, अचर वर्ण* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    शून्य create(File_पङ्क्ति* flp, अचर वर्ण* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    शून्य create(अचर वर्ण* sourcep);
    शून्य init(Ast_गाँठो* nodep, पूर्णाङ्क swidth = -1, bool sized = true) {
        this->nodep(nodep);
        यदि (swidth >= 0) {
            यदि (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) m_data.अङ्क()[i] = {0, 0};
        } अन्यथा {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    स्थिर string displayPad(माप_प्रकार fmtsize, वर्ण pad, bool बायाँ, अचर string& in) VL_PURE;
    string displayed(अचर string& vformat) अचर VL_MT_STABLE {
        फर्काऊ displayed(m_fileline, vformat);
    }
    शून्य warnTooMany(अचर string& मान);

सार्वजनिक:
    शून्य v3errorEnd(अचर std::ostringstream& sstr) अचर VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(अचर std::ostringstream& sstr) अचर VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    शून्य width(पूर्णाङ्क width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_सङ्ख्या& setAllBitsXRemoved();
    V3_सङ्ख्या& setAllBitsX();
    V3_सङ्ख्या& setAllBitsZ();
    V3_सङ्ख्या& setAllBits0();
    V3_सङ्ख्या& setAllBits1();
    V3_सङ्ख्या& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_सङ्ख्या& setMask(पूर्णाङ्क nbits, पूर्णाङ्क lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) अचर VL_MT_STABLE;
    string displayed(अचर Ast_गाँठो* nodep, अचर string& vformat,
                     अचर VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) अचर VL_MT_STABLE;
    string displayed(File_पङ्क्ति* fl, अचर string& vformat,
                     अचर VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) अचर VL_MT_STABLE;
    string displayedEnum(अचर AstSFormatArg* argp, अचर string& vformat) अचर VL_MT_STABLE;
    string displayedEnumName(अचर AstEnumDType* dtypep) अचर VL_MT_STABLE;
    स्थिर string displayedEnumName(अचर AstEnumItem* itemp);
    स्थिर bool displayedFmtHasArg(वर्ण format, bool isScan);
    string emitC() अचर VL_MT_STABLE;
    पूर्णाङ्क width() अचर VL_MT_SAFE { फर्काऊ m_data.width(); }
    पूर्णाङ्क widthToFit() अचर;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() अचर VL_MT_SAFE { फर्काऊ m_data.m_sized; }
    bool autoExtend() अचर VL_MT_SAFE { फर्काऊ m_data.m_autoExtend; }
    bool isFromString() अचर { फर्काऊ m_data.m_fromString; }
    bool hasOrigParamName() अचर { फर्काऊ m_data.m_hasOrigParamName; }
    शून्य hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_सङ्ख्या_Data_Type dataType() अचर VL_MT_SAFE { फर्काऊ m_data.type(); }
    शून्य dataType(V3_सङ्ख्या_Data_Type newType) {
        यदि (dataType() == newType) फर्काऊ;
        UASSERT(newType != V3_सङ्ख्या_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        विकल्प (newType) {
        पक्ष V3_सङ्ख्या_Data_Type::STRING: m_data.setString(); रोक;
        पक्ष V3_सङ्ख्या_Data_Type::DOUBLE: m_data.setDouble(); रोक;
        पक्ष V3_सङ्ख्या_Data_Type::LOGIC: m_data.setLogic(); रोक;
        पक्ष V3_सङ्ख्या_Data_Type::UNINITIALIZED: रोक;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() अचर VL_MT_SAFE { फर्काऊ m_data.m_signed; }
    शून्य isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() अचर VL_MT_SAFE { फर्काऊ dataType() == V3_सङ्ख्या_Data_Type::DOUBLE; }
    bool isString() अचर VL_MT_SAFE { फर्काऊ dataType() == V3_सङ्ख्या_Data_Type::STRING; }
    bool isOpaque() अचर VL_MT_SAFE { फर्काऊ isDouble() || isString(); }
    bool is_सङ्ख्या() अचर VL_MT_SAFE {
        फर्काऊ m_data.type() == V3_सङ्ख्या_Data_Type::LOGIC
               || m_data.type() == V3_सङ्ख्या_Data_Type::DOUBLE;
    }
    bool isNegative() अचर VL_MT_SAFE {
        // Correct number of zero bits/width matters
        फर्काऊ !isString() && bitIs1(width() - 1);
    }
    bool is1Step() अचर VL_MT_SAFE { फर्काऊ m_data.m_is1Step; }
    bool isNull() अचर VL_MT_SAFE { फर्काऊ m_data.m_isNull; }
    bool isFourState() अचर VL_MT_SAFE;
    bool hasZ() अचर {
        यदि (isString()) फर्काऊ false;
        लागि (पूर्णाङ्क i = 0; i < शब्दहरू(); ++i) {
            अचर मान_And_X v = m_data.अङ्क()[i];
            यदि ((~v.m_मान) & v.m_मान_X) फर्काऊ true;
        }
        फर्काऊ false;
    }
    bool isAllZ() अचर VL_MT_SAFE;
    bool isAllX() अचर VL_MT_SAFE;
    bool isEqZero() अचर VL_MT_SAFE;
    bool isNeqZero() अचर;
    bool isBitsZero(पूर्णाङ्क msb, पूर्णाङ्क lsb) अचर;
    bool isBroken(पूर्णाङ्क vwidth) अचर;
    bool isEqOne() अचर;
    bool isEqAllOnes(पूर्णाङ्क optwidth = 0) अचर;
    bool isCaseEq(अचर V3_सङ्ख्या& rhs) अचर;  // operator==
    bool isLtXZ(अचर V3_सङ्ख्या& rhs) अचर;  // operator< with XZ compared
    bool isAnyX() अचर VL_MT_SAFE;
    bool isAnyXZ() अचर;
    bool isAnyZ() अचर VL_MT_SAFE;
    bool isMsbXZ() अचर {
        // Correct number of zero bits/width matters
        फर्काऊ bitIsXZ(width() - 1);
    }
    bool fitsInUInt() अचर VL_MT_SAFE;
    uint32_t toUInt() अचर VL_MT_SAFE;
    int32_t toSInt() अचर VL_MT_SAFE;
    uint64_t toUQuad() अचर VL_MT_SAFE;
    int64_t toSQuad() अचर VL_MT_SAFE;
    string toString() अचर VL_MT_SAFE;
    string toDecimalS() अचर VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() अचर VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() अचर VL_MT_STABLE;
    द्विगुण toDouble() अचर VL_MT_SAFE;
    V3Hash toHash() अचर;
    uint32_t edataWord(पूर्णाङ्क eword) अचर;
    uint8_t dataByte(पूर्णाङ्क byte) अचर;
    uint32_t गन्ती_Bits(अचर V3_सङ्ख्या& ctrl) अचर;
    uint32_t गन्ती_Bits(अचर V3_सङ्ख्या& ctrl1, अचर V3_सङ्ख्या& ctrl2, अचर V3_सङ्ख्या& ctrl3) अचर;
    uint32_t गन्ती_Ones() अचर;
    uint32_t mostSetBitP1() अचर;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() अचर;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool चालक<(अचर V3_सङ्ख्या& rhs) अचर { फर्काऊ isLtXZ(rhs); }

    // STATICS
    स्थिर bool epsilonEqual(द्विगुण a, द्विगुण b);  // True if number within Epsilon of the other
    स्थिर bool epsilonIntegral(द्विगुण a);  // True if number rounds to integer within Epsilon
    स्थिर पूर्णाङ्क log2b(uint32_t अङ्क);
    स्थिर पूर्णाङ्क log2bQuad(uint64_t अङ्क);
    स्थिर शून्य selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_सङ्ख्या& opBitsNonXZ(अचर V3_सङ्ख्या& lhs);  // 0/1->1, X/Z->0
    V3_सङ्ख्या& opBitsOne(अचर V3_सङ्ख्या& lhs);  // 1->1, 0/X/Z->0
    V3_सङ्ख्या& opBitsXZ(अचर V3_सङ्ख्या& lhs);  // 0/1->0, X/Z->1
    V3_सङ्ख्या& opBitsZ(अचर V3_सङ्ख्या& lhs);  // Z->1, 0/1/X->0
    //
    V3_सङ्ख्या& opAssign(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opAssignNonXZ(अचर V3_सङ्ख्या& lhs, bool ignoreXZ = true);
    V3_सङ्ख्या& opExtendS(अचर V3_सङ्ख्या& lhs, uint32_t lbits);  // Sign extension
    V3_सङ्ख्या& opExtendXZ(अचर V3_सङ्ख्या& lhs, uint32_t lbits);  // X/Z extension
    V3_सङ्ख्या& opRedOr(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRedAnd(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRedXor(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_गन्ती_Bits(अचर V3_सङ्ख्या& अभिव्यक्ति, अचर V3_सङ्ख्या& ctrl1, अचर V3_सङ्ख्या& ctrl2,
                          अचर V3_सङ्ख्या& ctrl3);
    V3_सङ्ख्या& op_गन्ती_Ones(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opIsUnknown(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opOneHot(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opOneHot0(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opCLog2(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opMostSetBitP1(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opClean(अचर V3_सङ्ख्या& lhs, uint32_t bits);
    V3_सङ्ख्या& opConcat(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLenN(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRepl(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opRepl(अचर V3_सङ्ख्या& lhs, uint32_t rhsval);
    V3_सङ्ख्या& opStreamL(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opSel(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& msb, अचर V3_सङ्ख्या& lsb);
    V3_सङ्ख्या& opSel(अचर V3_सङ्ख्या& lhs, uint32_t msbval, uint32_t lsbval);
    V3_सङ्ख्या& opSelInto(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& lsb, पूर्णाङ्क width);
    V3_सङ्ख्या& opSelInto(अचर V3_सङ्ख्या& lhs, पूर्णाङ्क lsbval, पूर्णाङ्क width);
    V3_सङ्ख्या& opCvtPackString(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opToLowerN(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opToUpperN(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opCaseEq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opCaseNeq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opWildEq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opWildNeq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opBufIf1(अचर V3_सङ्ख्या& ens, अचर V3_सङ्ख्या& if1s);
    V3_सङ्ख्या& opSetRange(uint32_t lsb, uint32_t width, वर्ण bit_मान);
    // "standard" math
    V3_सङ्ख्या& opNot(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opLogNot(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opLogAnd(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLogOr(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLogEq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLogIf(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opNegate(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opAdd(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opSub(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opMul(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opMulS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opDiv(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opDivS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opModDiv(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opModDivS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opPow(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, bool lsign = false,
                    bool rsign = false);
    V3_सङ्ख्या& opPowSU(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed lhs, unsigned rhs
    V3_सङ्ख्या& opPowSS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed lhs, signed rhs
    V3_सङ्ख्या& opPowUS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Unsigned lhs, signed rhs
    V3_सङ्ख्या& opAnd(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opXor(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opOr(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opShiftR(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opShiftRS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_सङ्ख्या& opShiftL(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    // Comparisons
    V3_सङ्ख्या& opEq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opNeq(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGt(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGtS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opGte(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGteS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opLt(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLtS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opLte(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLteS(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);  // Signed

    // "D" - double (aka real) math
    V3_सङ्ख्या& opIToRD(अचर V3_सङ्ख्या& lhs, bool isSigned = false);
    V3_सङ्ख्या& opISToRD(अचर V3_सङ्ख्या& lhs) { फर्काऊ opIToRD(lhs, true); }
    V3_सङ्ख्या& opRToIS(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRToIRoundS(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRealToBits(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opBitsToRealD(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opNegateD(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opAddD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opSubD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opMulD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opDivD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opPowD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    // Comparisons
    V3_सङ्ख्या& opEqD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opNeqD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGtD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGteD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLtD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLteD(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);

    // "N" - string operations
    V3_सङ्ख्या& opAtoN(अचर V3_सङ्ख्या& lhs, पूर्णाङ्क base);
    V3_सङ्ख्या& opNToI(अचर V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opPutcN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, अचर V3_सङ्ख्या& ths);
    V3_सङ्ख्या& opGetcN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opSubstrN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, अचर V3_सङ्ख्या& ths);
    V3_सङ्ख्या& opCompareNN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs, bool ignoreCase);
    V3_सङ्ख्या& opConcatN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opReplN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opReplN(अचर V3_सङ्ख्या& lhs, uint32_t rhsval);
    V3_सङ्ख्या& opEqN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opNeqN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGtN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGteN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLtN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLteN(अचर V3_सङ्ख्या& lhs, अचर V3_सङ्ख्या& rhs);
};
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर V3_सङ्ख्या& rhs) VL_MT_SAFE {
    फर्काऊ os << rhs.ascii();
}

#endif  // Guard
