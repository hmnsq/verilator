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
    गणना en : अक्षर {
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
    explicit VFormatAttr(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिरांक { परत m_e; }
    अक्षर ascii() स्थिरांक { परत m_e; }
    bool isComplex() स्थिरांक { परत m_e == COMPLEX; }
    bool isDouble() स्थिरांक { परत m_e == DOUBLE; }
    bool isEnum() स्थिरांक { परत m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() स्थिरांक { परत m_e == SIGNED; }
    bool isString() स्थिरांक { परत m_e == STRING; }
    bool isUnsigned() स्थिरांक { परत m_e == UNSIGNED; }
};
constexpr bool चालक==(स्थिरांक VFormatAttr& lhs, स्थिरांक VFormatAttr& rhs) {
    परत lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिरांक VFormatAttr& lhs, VFormatAttr::en rhs) { परत lhs.m_e == rhs; }
constexpr bool चालक==(VFormatAttr::en lhs, स्थिरांक VFormatAttr& rhs) { परत lhs == rhs.m_e; }
ओळीत std::ostream& चालक<<(std::ostream& os, स्थिरांक VFormatAttr& rhs) {
    परत os << rhs.ascii();
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
        bool चालक==(स्थिरांक मूल्य_And_X& other) स्थिरांक VL_MT_SAFE {
            परत m_मूल्य == other.m_मूल्य && m_मूल्य_X == other.m_मूल्य_X;
        }
    };

    गणना class V3_संख्या_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    मित्र std::ostream& चालक<<(std::ostream& os, स्थिरांक V3_संख्या_Data_Type& rhs) VL_MT_SAFE {
        निवड (rhs) {
        स्थिती V3_संख्या_Data_Type::UNINITIALIZED: परत os << "UNINITIALIZED";
        स्थिती V3_संख्या_Data_Type::LOGIC: परत os << "LOGIC";
        स्थिती V3_संख्या_Data_Type::DOUBLE: परत os << "DOUBLE";
        स्थिती V3_संख्या_Data_Type::STRING: परत os << "STRING";
        }
        परत os;
    }

खाजगी:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    स्थिर constexpr पूर्णांक INLINE_शब्द = std::max(
        static_cast<माप_प्रकार>(2), std::max(आकार(std::string) / आकार(मूल्य_And_X),
                                         आकार(std::vector<मूल्य_And_X>) / आकार(मूल्य_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    स्थिर constexpr पूर्णांक MAX_INLINE_WIDTH = INLINE_शब्द * आकार(मूल्य_And_X) / 2 * 8;

    // MEMBERS
    संघ {
        std::array<मूल्य_And_X, INLINE_शब्द> m_inline_संख्या;
        std::vector<मूल्य_And_X> m_dynamic_संख्या;
        std::string m_string;
    };

    पूर्णांक m_width = 0;  // Width (in bits) as specified/calculated
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
    V3_संख्या_Data(स्थिरांक V3_संख्या_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        जर (other.is_Inline_संख्या()) {
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा जर (other.is_Dynamic_संख्या()) {
            init_Dynamic_संख्या(other.m_dynamic_संख्या);
        } अन्यथा जर (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_संख्या_Data& चालक=(स्थिरांक V3_संख्या_Data& other) {
        जर (other.is_Inline_संख्या()) {
            destroy_Stored_मूल्य();
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा जर (other.is_Dynamic_संख्या()) {
            reinit_With_Or_Assign_Dynamic_संख्या(other.m_dynamic_संख्या);
        } अन्यथा जर (other.isString()) {
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
        परत *this;
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
        जर (other.is_Inline_संख्या()) {
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा जर (other.is_Dynamic_संख्या()) {
            init_Dynamic_संख्या(std::move(other.m_dynamic_संख्या));
        } अन्यथा जर (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_संख्या_Data_Type::UNINITIALIZED;
    }

    V3_संख्या_Data& चालक=(V3_संख्या_Data&& other) {
        जर (other.is_Inline_संख्या()) {
            destroy_Stored_मूल्य();
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा जर (other.is_Dynamic_संख्या()) {
            reinit_With_Or_Assign_Dynamic_संख्या(std::move(other.m_dynamic_संख्या));
        } अन्यथा जर (other.isString()) {
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
        परत *this;
    }

    // ACCESSORS
    मूल्य_And_X* अंक() {
        UASSERT(is_संख्या(), "`num` member accessed when data type is " << m_type);
        परत is_Inline_संख्या() ? m_inline_संख्या.data() : m_dynamic_संख्या.data();
    }
    स्थिरांक मूल्य_And_X* अंक() स्थिरांक VL_MT_SAFE {
        UASSERT(is_संख्या(), "`num` member accessed when data type is " << m_type);
        परत is_Inline_संख्या() ? m_inline_संख्या.data() : m_dynamic_संख्या.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        परत m_string;
    }
    स्थिरांक std::string& str() स्थिरांक VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        परत m_string;
    }

    पूर्णांक width() स्थिरांक VL_MT_SAFE { परत m_width; }
    V3_संख्या_Data_Type type() स्थिरांक VL_MT_SAFE { परत m_type; }

    // METHODS
    शून्य resize(पूर्णांक bits_मोजणी) {
        जर (m_width == bits_मोजणी) परत;
        जर (bits_To_शब्द(m_width) == bits_To_शब्द(bits_मोजणी)) {
            m_width = bits_मोजणी;
            परत;
        }
        जर (is_Dynamic_संख्या()) {
            जर (bits_मोजणी > MAX_INLINE_WIDTH) {
                m_dynamic_संख्या.resize(bits_To_शब्द(bits_मोजणी));
            } अन्यथा {
                स्थिरांक स्वतः dynamicBits = std::move(m_dynamic_संख्या);
                destroy_Dynamic_संख्या();
                init_Inline_संख्या();
                std::स_नकल(m_inline_संख्या.data(), dynamicBits.data(), आकार(m_inline_संख्या));
            }
        } अन्यथा जर (is_Inline_संख्या()) {
            जर (bits_मोजणी > MAX_INLINE_WIDTH) {
                स्थिरांक स्वतः bits = m_inline_संख्या;
                init_Dynamic_संख्या(bits_To_शब्द(bits_मोजणी));
                std::स_नकल(m_dynamic_संख्या.data(), bits.data(), आकार(bits));
            }
        }
        m_width = bits_मोजणी;
    }

    शून्य setString() {
        // If there has been a string already it is kept intact.
        जर (isString()) परत;
        जर (is_Dynamic_संख्या()) destroy_Dynamic_संख्या();
        initString();
        m_type = V3_संख्या_Data_Type::STRING;
    }
    शून्य setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_संख्या_Data_Type::STRING;
    }
    शून्य setString(स्थिरांक std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_संख्या_Data_Type::STRING;
    }

    शून्य setDouble() {
        destroy_Stored_मूल्य();
        जर (!is_Inline_संख्या()) init_Inline_संख्या();
        m_type = V3_संख्या_Data_Type::DOUBLE;
        resize(64);
    }

    शून्य setLogic() {
        जर (isString()) destroyString();
        जर (!is_संख्या()) {
            जर (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_संख्या();
            } अन्यथा {
                init_Dynamic_संख्या(bits_To_शब्द(m_width));
            }
        }
        m_type = V3_संख्या_Data_Type::LOGIC;
        resize(m_width);
    }

खाजगी:
    स्थिर constexpr पूर्णांक bits_To_शब्द(पूर्णांक bits_मोजणी) VL_PURE { परत (bits_मोजणी + 31) / 32; }

    bool is_संख्या() स्थिरांक VL_MT_SAFE {
        परत m_type == V3_संख्या_Data_Type::DOUBLE || m_type == V3_संख्या_Data_Type::LOGIC;
    }
    bool is_Inline_संख्या() स्थिरांक VL_MT_SAFE {
        परत (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_संख्या_Data_Type::DOUBLE || m_type == V3_संख्या_Data_Type::LOGIC);
    }
    bool is_Dynamic_संख्या() स्थिरांक VL_MT_SAFE {
        परत (m_width > MAX_INLINE_WIDTH) && (m_type == V3_संख्या_Data_Type::LOGIC);
    }
    bool isString() स्थिरांक VL_MT_SAFE { परत m_type == V3_संख्या_Data_Type::STRING; }

    ढाँचा <typename... Args>
    शून्य init_Inline_संख्या(Args&&... args) {
        new (&m_inline_संख्या) std::array<मूल्य_And_X, INLINE_शब्द>(std::forward<Args>(args)...);
    }
    ढाँचा <typename... Args>
    शून्य init_Dynamic_संख्या(Args&&... args) {
        new (&m_dynamic_संख्या) std::vector<मूल्य_And_X>(std::forward<Args>(args)...);
    }
    ढाँचा <typename... Args>
    शून्य initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    शून्य destroy_Dynamic_संख्या() { m_dynamic_संख्या.~vector(); }
    शून्य destroyString() { m_string.~string(); }
    शून्य destroy_Stored_मूल्य() {
        जर (isString())
            destroyString();
        अन्यथा जर (is_Dynamic_संख्या())
            destroy_Dynamic_संख्या();
    }

    ढाँचा <typename T>
    शून्य reinit_With_Or_Assign_Dynamic_संख्या(T&& s) {
        जर (is_Dynamic_संख्या()) {
            m_dynamic_संख्या = std::forward<T>(s);
            परत;
        }
        जर (isString()) destroyString();
        init_Dynamic_संख्या(std::forward<T>(s));
    }
    ढाँचा <typename T>
    शून्य reinitWithOrAssignString(T&& s) {
        जर (isString()) {
            m_string = std::forward<T>(s);
            परत;
        }
        जर (is_Dynamic_संख्या()) destroy_Dynamic_संख्या();
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
    V3_संख्या& setSingleBits(अक्षर मूल्य);
    शून्य fillBits(पूर्णांक destLsb, पूर्णांक width, अक्षर मूल्य);
    शून्य copyBits(पूर्णांक destLsb, स्थिरांक V3_संख्या& source, पूर्णांक sourceLsb, पूर्णांक width);
    V3_संख्या& setString(स्थिरांक string& str) {
        m_data.setString(str);
        परत *this;
    }
    शून्य opCleanThis(bool warnOnTruncation = false);

सार्वजनिक:
    शून्य nodep(Ast_गाठ* nodep) VL_MT_STABLE;
    Ast_गाठ* nodep() स्थिरांक VL_MT_SAFE { परत m_nodep; }  // For debug only
    File_ओळ* fileline() स्थिरांक VL_MT_SAFE { परत m_fileline; }
    V3_संख्या& setZero();
    V3_संख्या& setQuad(uint64_t मूल्य);
    V3_संख्या& setLong(uint32_t मूल्य);
    V3_संख्या& setLongS(int32_t मूल्य);
    V3_संख्या& setDouble(द्विगुण मूल्य);
    शून्य setBitX0(पूर्णांक bit);
    शून्य setBit(पूर्णांक bit, अक्षर मूल्य) {  // Note: must be initialized as number and pre-zeroed!
        जर (bit >= m_data.width()) परत;
        स्थिरांक uint32_t mask = (1UL << (bit & 31));
        मूल्य_And_X& v = m_data.अंक()[bit / 32];
        जर (मूल्य == '0' || मूल्य == 0) {
            v.m_मूल्य &= ~mask;
            v.m_मूल्य_X &= ~mask;
        } अन्यथा जर (मूल्य == '1' || मूल्य == 1) {
            v.m_मूल्य |= mask;
            v.m_मूल्य_X &= ~mask;
        } अन्यथा जर (मूल्य == 'z' || मूल्य == 2) {
            v.m_मूल्य &= ~mask;
            v.m_मूल्य_X |= mask;
        } अन्यथा {  // X
            v.m_मूल्य |= mask;
            v.m_मूल्य_X |= mask;
        }
    }

खाजगी:
    अक्षर bitIs(पूर्णांक bit) स्थिरांक {
        जर (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            परत '0';
        }
        स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
        परत ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    अक्षर bitIsExtend(पूर्णांक bit, पूर्णांक lbits) स्थिरांक {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        जर (bit < 0) परत '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        जर (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
            // We do sign extend
            परत ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
        परत ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

सार्वजनिक:
    bool bitIs0(पूर्णांक bit) स्थिरांक VL_MT_SAFE {
        जर (!is_संख्या()) परत false;
        जर (bit < 0) परत false;
        जर (bit >= m_data.width()) परत !bitIsXZ(m_data.width() - 1);
        स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
        परत ((v.m_मूल्य | v.m_मूल्य_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(पूर्णांक bit) स्थिरांक VL_MT_SAFE {
        जर (!is_संख्या()) परत false;
        जर (bit < 0) परत false;
        जर (bit >= m_data.width()) परत false;
        स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
        परत ((v.m_मूल्य & (1UL << (bit & 31))) && !(v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(पूर्णांक bit) स्थिरांक {
        // Correct number of zero bits/width matters
        जर (!is_संख्या()) परत false;
        जर (bit < 0) परत false;
        जर (bit >= m_data.width()) परत bitIs1Extend(m_data.width() - 1);
        स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
        परत ((v.m_मूल्य & (1UL << (bit & 31))) && !(v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsX(पूर्णांक bit) स्थिरांक VL_MT_SAFE {
        जर (!is_संख्या()) परत false;
        जर (bit < 0) परत false;
        जर (bit >= m_data.width()) परत bitIsZ(m_data.width() - 1);
        स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
        परत ((v.m_मूल्य & (1UL << (bit & 31))) && (v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(पूर्णांक bit) स्थिरांक VL_MT_SAFE {
        जर (!is_संख्या()) परत false;
        जर (bit < 0) परत false;
        जर (bit >= m_data.width()) परत bitIsXZ(m_data.width() - 1);
        स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
        परत ((v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(पूर्णांक bit) स्थिरांक VL_MT_SAFE {
        जर (!is_संख्या()) परत false;
        जर (bit < 0) परत false;
        जर (bit >= m_data.width()) परत bitIsZ(m_data.width() - 1);
        स्थिरांक मूल्य_And_X v = m_data.अंक()[bit / 32];
        परत ((~v.m_मूल्य & (1UL << (bit & 31))) && (v.m_मूल्य_X & (1UL << (bit & 31))));
    }

खाजगी:
    uint32_t bits_मूल्य(पूर्णांक lsb, पूर्णांक nbits) स्थिरांक VL_MT_SAFE {
        uint32_t v = 0;
        साठी (पूर्णांक bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        परत v;
    }

    पूर्णांक मोजणी_X(पूर्णांक lsb, पूर्णांक nbits) स्थिरांक VL_MT_SAFE;
    पूर्णांक मोजणी_Z(पूर्णांक lsb, पूर्णांक nbits) स्थिरांक VL_MT_SAFE;

    पूर्णांक शब्द() स्थिरांक VL_MT_SAFE { परत ((width() + 31) / 32); }
    uint32_t hiWordMask() स्थिरांक VL_MT_SAFE {
        // Correct number of zero bits/width matters
        परत VL_MASK_I(width());
    }

    V3_संख्या& opModDivGuts(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, bool is_modulus);

सार्वजनिक:
    // CONSTRUCTORS
    explicit V3_संख्या(Ast_गाठ* nodep) { init(nodep, 1); }
    V3_संख्या(Ast_गाठ* nodep, पूर्णांक width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_संख्या(Ast_गाठ* nodep, पूर्णांक width, स्थिरांक V3_संख्या& मूल्य) {
        init(nodep, width, width > 0);
        opAssign(मूल्य);
    }
    V3_संख्या(Ast_गाठ* nodep, पूर्णांक width, uint32_t मूल्य, bool sized = true) {
        init(nodep, width, sized);
        m_data.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
    }
    V3_संख्या(File_ओळ* flp, पूर्णांक width, uint32_t मूल्य, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_संख्या_Literal {};  // For creator type-overload selection
    V3_संख्या(Ast_गाठ* nodep, Verilog_संख्या_Literal, स्थिरांक अक्षर* sourcep) { create(nodep, sourcep); }
    V3_संख्या(File_ओळ* flp, Verilog_संख्या_Literal, स्थिरांक अक्षर* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_संख्या(Ast_गाठ* nodep, VerilogStringLiteral, स्थिरांक string& str);
    class Double {};
    V3_संख्या(Ast_गाठ* nodep, Double, द्विगुण मूल्य) {
        init(nodep, 64);
        setDouble(मूल्य);
    }
    class String {};
    V3_संख्या(Ast_गाठ* nodep, String, स्थिरांक string& मूल्य) {
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
    explicit V3_संख्या(स्थिरांक V3_संख्या* nump, पूर्णांक width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_संख्या(स्थिरांक V3_संख्या* nump, पूर्णांक width, uint32_t मूल्य) {
        init(nullptr, width);
        m_data.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_संख्या(Ast_गाठ* nodep, स्थिरांक Ast_गाठ_DType* nodedtypep);

    V3_संख्या(स्थिरांक V3_संख्या& other) = पूर्वनिर्धारित;
    V3_संख्या& चालक=(स्थिरांक V3_संख्या& other) = पूर्वनिर्धारित;

    V3_संख्या(V3_संख्या&& other) = पूर्वनिर्धारित;
    V3_संख्या& चालक=(V3_संख्या&& other) = पूर्वनिर्धारित;

    ~V3_संख्या() = पूर्वनिर्धारित;

खाजगी:
    शून्य selfTestThis();
    शून्य create(Ast_गाठ* nodep, स्थिरांक अक्षर* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    शून्य create(File_ओळ* flp, स्थिरांक अक्षर* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    शून्य create(स्थिरांक अक्षर* sourcep);
    शून्य init(Ast_गाठ* nodep, पूर्णांक swidth = -1, bool sized = true) {
        this->nodep(nodep);
        जर (swidth >= 0) {
            जर (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            साठी (पूर्णांक i = 0; i < शब्द(); ++i) m_data.अंक()[i] = {0, 0};
        } अन्यथा {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    स्थिर string displayPad(माप_प्रकार fmtsize, अक्षर pad, bool डावा, स्थिरांक string& in) VL_PURE;
    string displayed(स्थिरांक string& vformat) स्थिरांक VL_MT_STABLE {
        परत displayed(m_fileline, vformat);
    }
    शून्य warnTooMany(स्थिरांक string& मूल्य);

सार्वजनिक:
    शून्य v3errorEnd(स्थिरांक std::ostringstream& sstr) स्थिरांक VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(स्थिरांक std::ostringstream& sstr) स्थिरांक VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    शून्य width(पूर्णांक width, bool sized = true) {
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
    V3_संख्या& setMask(पूर्णांक nbits, पूर्णांक lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) स्थिरांक VL_MT_STABLE;
    string displayed(स्थिरांक Ast_गाठ* nodep, स्थिरांक string& vformat,
                     स्थिरांक VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) स्थिरांक VL_MT_STABLE;
    string displayed(File_ओळ* fl, स्थिरांक string& vformat,
                     स्थिरांक VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) स्थिरांक VL_MT_STABLE;
    string displayedEnum(स्थिरांक AstSFormatArg* argp, स्थिरांक string& vformat) स्थिरांक VL_MT_STABLE;
    string displayedEnumName(स्थिरांक AstEnumDType* dtypep) स्थिरांक VL_MT_STABLE;
    स्थिर string displayedEnumName(स्थिरांक AstEnumItem* itemp);
    स्थिर bool displayedFmtHasArg(अक्षर format, bool isScan);
    string emitC() स्थिरांक VL_MT_STABLE;
    पूर्णांक width() स्थिरांक VL_MT_SAFE { परत m_data.width(); }
    पूर्णांक widthToFit() स्थिरांक;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() स्थिरांक VL_MT_SAFE { परत m_data.m_sized; }
    bool autoExtend() स्थिरांक VL_MT_SAFE { परत m_data.m_autoExtend; }
    bool isFromString() स्थिरांक { परत m_data.m_fromString; }
    bool hasOrigParamName() स्थिरांक { परत m_data.m_hasOrigParamName; }
    शून्य hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_संख्या_Data_Type dataType() स्थिरांक VL_MT_SAFE { परत m_data.type(); }
    शून्य dataType(V3_संख्या_Data_Type newType) {
        जर (dataType() == newType) परत;
        UASSERT(newType != V3_संख्या_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        निवड (newType) {
        स्थिती V3_संख्या_Data_Type::STRING: m_data.setString(); थांबा;
        स्थिती V3_संख्या_Data_Type::DOUBLE: m_data.setDouble(); थांबा;
        स्थिती V3_संख्या_Data_Type::LOGIC: m_data.setLogic(); थांबा;
        स्थिती V3_संख्या_Data_Type::UNINITIALIZED: थांबा;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() स्थिरांक VL_MT_SAFE { परत m_data.m_signed; }
    शून्य isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() स्थिरांक VL_MT_SAFE { परत dataType() == V3_संख्या_Data_Type::DOUBLE; }
    bool isString() स्थिरांक VL_MT_SAFE { परत dataType() == V3_संख्या_Data_Type::STRING; }
    bool isOpaque() स्थिरांक VL_MT_SAFE { परत isDouble() || isString(); }
    bool is_संख्या() स्थिरांक VL_MT_SAFE {
        परत m_data.type() == V3_संख्या_Data_Type::LOGIC
               || m_data.type() == V3_संख्या_Data_Type::DOUBLE;
    }
    bool isNegative() स्थिरांक VL_MT_SAFE {
        // Correct number of zero bits/width matters
        परत !isString() && bitIs1(width() - 1);
    }
    bool is1Step() स्थिरांक VL_MT_SAFE { परत m_data.m_is1Step; }
    bool isNull() स्थिरांक VL_MT_SAFE { परत m_data.m_isNull; }
    bool isFourState() स्थिरांक VL_MT_SAFE;
    bool hasZ() स्थिरांक {
        जर (isString()) परत false;
        साठी (पूर्णांक i = 0; i < शब्द(); ++i) {
            स्थिरांक मूल्य_And_X v = m_data.अंक()[i];
            जर ((~v.m_मूल्य) & v.m_मूल्य_X) परत true;
        }
        परत false;
    }
    bool isAllZ() स्थिरांक VL_MT_SAFE;
    bool isAllX() स्थिरांक VL_MT_SAFE;
    bool isEqZero() स्थिरांक VL_MT_SAFE;
    bool isNeqZero() स्थिरांक;
    bool isBitsZero(पूर्णांक msb, पूर्णांक lsb) स्थिरांक;
    bool isBroken(पूर्णांक vwidth) स्थिरांक;
    bool isEqOne() स्थिरांक;
    bool isEqAllOnes(पूर्णांक optwidth = 0) स्थिरांक;
    bool isCaseEq(स्थिरांक V3_संख्या& rhs) स्थिरांक;  // operator==
    bool isLtXZ(स्थिरांक V3_संख्या& rhs) स्थिरांक;  // operator< with XZ compared
    bool isAnyX() स्थिरांक VL_MT_SAFE;
    bool isAnyXZ() स्थिरांक;
    bool isAnyZ() स्थिरांक VL_MT_SAFE;
    bool isMsbXZ() स्थिरांक {
        // Correct number of zero bits/width matters
        परत bitIsXZ(width() - 1);
    }
    bool fitsInUInt() स्थिरांक VL_MT_SAFE;
    uint32_t toUInt() स्थिरांक VL_MT_SAFE;
    int32_t toSInt() स्थिरांक VL_MT_SAFE;
    uint64_t toUQuad() स्थिरांक VL_MT_SAFE;
    int64_t toSQuad() स्थिरांक VL_MT_SAFE;
    string toString() स्थिरांक VL_MT_SAFE;
    string toDecimalS() स्थिरांक VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() स्थिरांक VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() स्थिरांक VL_MT_STABLE;
    द्विगुण toDouble() स्थिरांक VL_MT_SAFE;
    V3Hash toHash() स्थिरांक;
    uint32_t edataWord(पूर्णांक eword) स्थिरांक;
    uint8_t dataByte(पूर्णांक byte) स्थिरांक;
    uint32_t मोजणी_Bits(स्थिरांक V3_संख्या& ctrl) स्थिरांक;
    uint32_t मोजणी_Bits(स्थिरांक V3_संख्या& ctrl1, स्थिरांक V3_संख्या& ctrl2, स्थिरांक V3_संख्या& ctrl3) स्थिरांक;
    uint32_t मोजणी_Ones() स्थिरांक;
    uint32_t mostSetBitP1() स्थिरांक;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() स्थिरांक;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool चालक<(स्थिरांक V3_संख्या& rhs) स्थिरांक { परत isLtXZ(rhs); }

    // STATICS
    स्थिर bool epsilonEqual(द्विगुण a, द्विगुण b);  // True if number within Epsilon of the other
    स्थिर bool epsilonIntegral(द्विगुण a);  // True if number rounds to integer within Epsilon
    स्थिर पूर्णांक log2b(uint32_t अंक);
    स्थिर पूर्णांक log2bQuad(uint64_t अंक);
    स्थिर शून्य selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_संख्या& opBitsNonXZ(स्थिरांक V3_संख्या& lhs);  // 0/1->1, X/Z->0
    V3_संख्या& opBitsOne(स्थिरांक V3_संख्या& lhs);  // 1->1, 0/X/Z->0
    V3_संख्या& opBitsXZ(स्थिरांक V3_संख्या& lhs);  // 0/1->0, X/Z->1
    V3_संख्या& opBitsZ(स्थिरांक V3_संख्या& lhs);  // Z->1, 0/1/X->0
    //
    V3_संख्या& opAssign(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opAssignNonXZ(स्थिरांक V3_संख्या& lhs, bool ignoreXZ = true);
    V3_संख्या& opExtendS(स्थिरांक V3_संख्या& lhs, uint32_t lbits);  // Sign extension
    V3_संख्या& opExtendXZ(स्थिरांक V3_संख्या& lhs, uint32_t lbits);  // X/Z extension
    V3_संख्या& opRedOr(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opRedAnd(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opRedXor(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& op_मोजणी_Bits(स्थिरांक V3_संख्या& अभिव्यक्ती, स्थिरांक V3_संख्या& ctrl1, स्थिरांक V3_संख्या& ctrl2,
                          स्थिरांक V3_संख्या& ctrl3);
    V3_संख्या& op_मोजणी_Ones(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opIsUnknown(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opOneHot(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opOneHot0(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opCLog2(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opMostSetBitP1(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opClean(स्थिरांक V3_संख्या& lhs, uint32_t bits);
    V3_संख्या& opConcat(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLenN(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opRepl(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opRepl(स्थिरांक V3_संख्या& lhs, uint32_t rhsval);
    V3_संख्या& opStreamL(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opSel(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& msb, स्थिरांक V3_संख्या& lsb);
    V3_संख्या& opSel(स्थिरांक V3_संख्या& lhs, uint32_t msbval, uint32_t lsbval);
    V3_संख्या& opSelInto(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& lsb, पूर्णांक width);
    V3_संख्या& opSelInto(स्थिरांक V3_संख्या& lhs, पूर्णांक lsbval, पूर्णांक width);
    V3_संख्या& opCvtPackString(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opToLowerN(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opToUpperN(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opCaseEq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opCaseNeq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opWildEq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opWildNeq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opBufIf1(स्थिरांक V3_संख्या& ens, स्थिरांक V3_संख्या& if1s);
    V3_संख्या& opSetRange(uint32_t lsb, uint32_t width, अक्षर bit_मूल्य);
    // "standard" math
    V3_संख्या& opNot(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opLogNot(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opLogAnd(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLogOr(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLogEq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLogIf(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opNegate(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opAdd(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opSub(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opMul(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opMulS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed
    V3_संख्या& opDiv(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opDivS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed
    V3_संख्या& opModDiv(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opModDivS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed
    V3_संख्या& opPow(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, bool lsign = false,
                    bool rsign = false);
    V3_संख्या& opPowSU(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed lhs, unsigned rhs
    V3_संख्या& opPowSS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed lhs, signed rhs
    V3_संख्या& opPowUS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Unsigned lhs, signed rhs
    V3_संख्या& opAnd(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opXor(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opOr(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opShiftR(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opShiftRS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_संख्या& opShiftL(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    // Comparisons
    V3_संख्या& opEq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opNeq(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opGt(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opGtS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed
    V3_संख्या& opGte(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opGteS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed
    V3_संख्या& opLt(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLtS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed
    V3_संख्या& opLte(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLteS(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);  // Signed

    // "D" - double (aka real) math
    V3_संख्या& opIToRD(स्थिरांक V3_संख्या& lhs, bool isSigned = false);
    V3_संख्या& opISToRD(स्थिरांक V3_संख्या& lhs) { परत opIToRD(lhs, true); }
    V3_संख्या& opRToIS(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opRToIRoundS(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opRealToBits(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opBitsToRealD(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opNegateD(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opAddD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opSubD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opMulD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opDivD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opPowD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    // Comparisons
    V3_संख्या& opEqD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opNeqD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opGtD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opGteD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLtD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLteD(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);

    // "N" - string operations
    V3_संख्या& opAtoN(स्थिरांक V3_संख्या& lhs, पूर्णांक base);
    V3_संख्या& opNToI(स्थिरांक V3_संख्या& lhs);
    V3_संख्या& opPutcN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, स्थिरांक V3_संख्या& ths);
    V3_संख्या& opGetcN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opSubstrN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, स्थिरांक V3_संख्या& ths);
    V3_संख्या& opCompareNN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs, bool ignoreCase);
    V3_संख्या& opConcatN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opReplN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opReplN(स्थिरांक V3_संख्या& lhs, uint32_t rhsval);
    V3_संख्या& opEqN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opNeqN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opGtN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opGteN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLtN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
    V3_संख्या& opLteN(स्थिरांक V3_संख्या& lhs, स्थिरांक V3_संख्या& rhs);
};
ओळीत std::ostream& चालक<<(std::ostream& os, स्थिरांक V3_संख्या& rhs) VL_MT_SAFE {
    परत os << rhs.ascii();
}

#endif  // Guard
