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

#अगर_अघोषित VERILATOR_V3_संख्या_H
#घोषणा VERILATOR_V3_संख्या_H

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3Hash.h"
#समावेश "V3StdFuture.h"

#समावेश <algorithm>
#समावेश <array>
#समावेश <cmath>
#समावेश <limits>
#समावेश <vector>

class AstEnumDType;
class AstEnumItem;
class Ast_गाँठ;
class Ast_गाँठ_DType;
class AstSFormatArg;
class File_पंक्ति;

//============================================================================

class VFormatAttr final {
खुला:
    क्रमागत en : अक्षर {
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
    क्रमागत en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    अक्षर ascii() स्थिर { वापस m_e; }
    bool isComplex() स्थिर { वापस m_e == COMPLEX; }
    bool isDouble() स्थिर { वापस m_e == DOUBLE; }
    bool isEnum() स्थिर { वापस m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() स्थिर { वापस m_e == SIGNED; }
    bool isString() स्थिर { वापस m_e == STRING; }
    bool isUnsigned() स्थिर { वापस m_e == UNSIGNED; }
};
constexpr bool चालक==(स्थिर VFormatAttr& lhs, स्थिर VFormatAttr& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VFormatAttr& lhs, VFormatAttr::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VFormatAttr::en lhs, स्थिर VFormatAttr& rhs) { वापस lhs == rhs.m_e; }
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VFormatAttr& rhs) {
    वापस os << rhs.ascii();
}

//============================================================================

class V3_संख्या_तथ्य final {
खुला:
    // TYPES
    काष्ठा मूल्य_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_मूल्य;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_मूल्य_X;
        bool चालक==(स्थिर मूल्य_And_X& other) स्थिर VL_MT_SAFE {
            वापस m_मूल्य == other.m_मूल्य && m_मूल्य_X == other.m_मूल्य_X;
        }
    };

    क्रमागत class V3_संख्या_तथ्य_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    मित्र std::ostream& चालक<<(std::ostream& os, स्थिर V3_संख्या_तथ्य_Type& rhs) VL_MT_SAFE {
        चयन (rhs) {
        हाल V3_संख्या_तथ्य_Type::UNINITIALIZED: वापस os << "UNINITIALIZED";
        हाल V3_संख्या_तथ्य_Type::LOGIC: वापस os << "LOGIC";
        हाल V3_संख्या_तथ्य_Type::DOUBLE: वापस os << "DOUBLE";
        हाल V3_संख्या_तथ्य_Type::STRING: वापस os << "STRING";
        }
        वापस os;
    }

निजी:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    अटल constexpr पूर्णांक INLINE_शब्दगण = std::max(
        static_cast<माप_प्रकार>(2), std::max(माप(std::string) / माप(मूल्य_And_X),
                                         माप(std::vector<मूल्य_And_X>) / माप(मूल्य_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    अटल constexpr पूर्णांक MAX_INLINE_WIDTH = INLINE_शब्दगण * माप(मूल्य_And_X) / 2 * 8;

    // MEMBERS
    जोड़ {
        std::array<मूल्य_And_X, INLINE_शब्दगण> m_inline_संख्या;
        std::vector<मूल्य_And_X> m_dynamic_संख्या;
        std::string m_string;
    };

    पूर्णांक m_width = 0;  // Width (in bits) as specified/calculated
    V3_संख्या_तथ्य_Type m_type;

खुला:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_संख्या_तथ्य()
        : m_type{V3_संख्या_तथ्य_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_संख्या_तथ्य() { destroy_Stored_मूल्य(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_संख्या_तथ्य(स्थिर V3_संख्या_तथ्य& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        अगर (other.is_Inline_संख्या()) {
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा अगर (other.is_Dynamic_संख्या()) {
            init_Dynamic_संख्या(other.m_dynamic_संख्या);
        } अन्यथा अगर (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_संख्या_तथ्य& चालक=(स्थिर V3_संख्या_तथ्य& other) {
        अगर (other.is_Inline_संख्या()) {
            destroy_Stored_मूल्य();
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा अगर (other.is_Dynamic_संख्या()) {
            reinit_With_Or_Assign_Dynamic_संख्या(other.m_dynamic_संख्या);
        } अन्यथा अगर (other.isString()) {
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
        वापस *this;
    }

    V3_संख्या_तथ्य(V3_संख्या_तथ्य&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        अगर (other.is_Inline_संख्या()) {
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा अगर (other.is_Dynamic_संख्या()) {
            init_Dynamic_संख्या(std::move(other.m_dynamic_संख्या));
        } अन्यथा अगर (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_संख्या_तथ्य_Type::UNINITIALIZED;
    }

    V3_संख्या_तथ्य& चालक=(V3_संख्या_तथ्य&& other) {
        अगर (other.is_Inline_संख्या()) {
            destroy_Stored_मूल्य();
            init_Inline_संख्या(other.m_inline_संख्या);
        } अन्यथा अगर (other.is_Dynamic_संख्या()) {
            reinit_With_Or_Assign_Dynamic_संख्या(std::move(other.m_dynamic_संख्या));
        } अन्यथा अगर (other.isString()) {
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
        other.m_type = V3_संख्या_तथ्य_Type::UNINITIALIZED;
        वापस *this;
    }

    // ACCESSORS
    मूल्य_And_X* अंक() {
        UASSERT(is_संख्या(), "`num` member accessed when data type is " << m_type);
        वापस is_Inline_संख्या() ? m_inline_संख्या.तथ्य() : m_dynamic_संख्या.तथ्य();
    }
    स्थिर मूल्य_And_X* अंक() स्थिर VL_MT_SAFE {
        UASSERT(is_संख्या(), "`num` member accessed when data type is " << m_type);
        वापस is_Inline_संख्या() ? m_inline_संख्या.तथ्य() : m_dynamic_संख्या.तथ्य();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        वापस m_string;
    }
    स्थिर std::string& str() स्थिर VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        वापस m_string;
    }

    पूर्णांक width() स्थिर VL_MT_SAFE { वापस m_width; }
    V3_संख्या_तथ्य_Type type() स्थिर VL_MT_SAFE { वापस m_type; }

    // METHODS
    व्योम resize(पूर्णांक bits_गिनती) {
        अगर (m_width == bits_गिनती) वापस;
        अगर (bits_To_शब्दगण(m_width) == bits_To_शब्दगण(bits_गिनती)) {
            m_width = bits_गिनती;
            वापस;
        }
        अगर (is_Dynamic_संख्या()) {
            अगर (bits_गिनती > MAX_INLINE_WIDTH) {
                m_dynamic_संख्या.resize(bits_To_शब्दगण(bits_गिनती));
            } अन्यथा {
                स्थिर स्वतः dynamicBits = std::move(m_dynamic_संख्या);
                destroy_Dynamic_संख्या();
                init_Inline_संख्या();
                std::स_नकल(m_inline_संख्या.तथ्य(), dynamicBits.तथ्य(), माप(m_inline_संख्या));
            }
        } अन्यथा अगर (is_Inline_संख्या()) {
            अगर (bits_गिनती > MAX_INLINE_WIDTH) {
                स्थिर स्वतः bits = m_inline_संख्या;
                init_Dynamic_संख्या(bits_To_शब्दगण(bits_गिनती));
                std::स_नकल(m_dynamic_संख्या.तथ्य(), bits.तथ्य(), माप(bits));
            }
        }
        m_width = bits_गिनती;
    }

    व्योम setString() {
        // If there has been a string already it is kept intact.
        अगर (isString()) वापस;
        अगर (is_Dynamic_संख्या()) destroy_Dynamic_संख्या();
        initString();
        m_type = V3_संख्या_तथ्य_Type::STRING;
    }
    व्योम setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_संख्या_तथ्य_Type::STRING;
    }
    व्योम setString(स्थिर std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_संख्या_तथ्य_Type::STRING;
    }

    व्योम setDouble() {
        destroy_Stored_मूल्य();
        अगर (!is_Inline_संख्या()) init_Inline_संख्या();
        m_type = V3_संख्या_तथ्य_Type::DOUBLE;
        resize(64);
    }

    व्योम setLogic() {
        अगर (isString()) destroyString();
        अगर (!is_संख्या()) {
            अगर (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_संख्या();
            } अन्यथा {
                init_Dynamic_संख्या(bits_To_शब्दगण(m_width));
            }
        }
        m_type = V3_संख्या_तथ्य_Type::LOGIC;
        resize(m_width);
    }

निजी:
    अटल constexpr पूर्णांक bits_To_शब्दगण(पूर्णांक bits_गिनती) VL_PURE { वापस (bits_गिनती + 31) / 32; }

    bool is_संख्या() स्थिर VL_MT_SAFE {
        वापस m_type == V3_संख्या_तथ्य_Type::DOUBLE || m_type == V3_संख्या_तथ्य_Type::LOGIC;
    }
    bool is_Inline_संख्या() स्थिर VL_MT_SAFE {
        वापस (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_संख्या_तथ्य_Type::DOUBLE || m_type == V3_संख्या_तथ्य_Type::LOGIC);
    }
    bool is_Dynamic_संख्या() स्थिर VL_MT_SAFE {
        वापस (m_width > MAX_INLINE_WIDTH) && (m_type == V3_संख्या_तथ्य_Type::LOGIC);
    }
    bool isString() स्थिर VL_MT_SAFE { वापस m_type == V3_संख्या_तथ्य_Type::STRING; }

    ढाँचा <typename... Args>
    व्योम init_Inline_संख्या(Args&&... args) {
        new (&m_inline_संख्या) std::array<मूल्य_And_X, INLINE_शब्दगण>(std::forward<Args>(args)...);
    }
    ढाँचा <typename... Args>
    व्योम init_Dynamic_संख्या(Args&&... args) {
        new (&m_dynamic_संख्या) std::vector<मूल्य_And_X>(std::forward<Args>(args)...);
    }
    ढाँचा <typename... Args>
    व्योम initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    व्योम destroy_Dynamic_संख्या() { m_dynamic_संख्या.~vector(); }
    व्योम destroyString() { m_string.~string(); }
    व्योम destroy_Stored_मूल्य() {
        अगर (isString())
            destroyString();
        अन्यथा अगर (is_Dynamic_संख्या())
            destroy_Dynamic_संख्या();
    }

    ढाँचा <typename T>
    व्योम reinit_With_Or_Assign_Dynamic_संख्या(T&& s) {
        अगर (is_Dynamic_संख्या()) {
            m_dynamic_संख्या = std::forward<T>(s);
            वापस;
        }
        अगर (isString()) destroyString();
        init_Dynamic_संख्या(std::forward<T>(s));
    }
    ढाँचा <typename T>
    व्योम reinitWithOrAssignString(T&& s) {
        अगर (isString()) {
            m_string = std::forward<T>(s);
            वापस;
        }
        अगर (is_Dynamic_संख्या()) destroy_Dynamic_संख्या();
        initString(std::forward<T>(s));
    }
};

class V3_संख्या final {
    // TYPES
    using मूल्य_And_X = V3_संख्या_तथ्य::मूल्य_And_X;
    using V3_संख्या_तथ्य_Type = V3_संख्या_तथ्य::V3_संख्या_तथ्य_Type;

    // MEMBERS
    V3_संख्या_तथ्य m_तथ्य;
    Ast_गाँठ* m_nodep = nullptr;  // Parent node - for error reporting only
    File_पंक्ति* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    व्योम setNull() {
        m_तथ्य.setLogic();
        m_तथ्य.m_isNull = true;
        m_तथ्य.m_autoExtend = true;
    }
    V3_संख्या& setSingleBits(अक्षर मूल्य);
    व्योम fillBits(पूर्णांक destLsb, पूर्णांक width, अक्षर मूल्य);
    व्योम copyBits(पूर्णांक destLsb, स्थिर V3_संख्या& source, पूर्णांक sourceLsb, पूर्णांक width);
    V3_संख्या& setString(स्थिर string& str) {
        m_तथ्य.setString(str);
        वापस *this;
    }
    व्योम opCleanThis(bool warnOnTruncation = false);

खुला:
    व्योम nodep(Ast_गाँठ* nodep) VL_MT_STABLE;
    Ast_गाँठ* nodep() स्थिर VL_MT_SAFE { वापस m_nodep; }  // For debug only
    File_पंक्ति* fileline() स्थिर VL_MT_SAFE { वापस m_fileline; }
    V3_संख्या& setZero();
    V3_संख्या& setQuad(uint64_t मूल्य);
    V3_संख्या& setLong(uint32_t मूल्य);
    V3_संख्या& setLongS(int32_t मूल्य);
    V3_संख्या& setDouble(द्विगुन मूल्य);
    व्योम setBitX0(पूर्णांक bit);
    व्योम setBit(पूर्णांक bit, अक्षर मूल्य) {  // Note: must be initialized as number and pre-zeroed!
        अगर (bit >= m_तथ्य.width()) वापस;
        स्थिर uint32_t mask = (1UL << (bit & 31));
        मूल्य_And_X& v = m_तथ्य.अंक()[bit / 32];
        अगर (मूल्य == '0' || मूल्य == 0) {
            v.m_मूल्य &= ~mask;
            v.m_मूल्य_X &= ~mask;
        } अन्यथा अगर (मूल्य == '1' || मूल्य == 1) {
            v.m_मूल्य |= mask;
            v.m_मूल्य_X &= ~mask;
        } अन्यथा अगर (मूल्य == 'z' || मूल्य == 2) {
            v.m_मूल्य &= ~mask;
            v.m_मूल्य_X |= mask;
        } अन्यथा {  // X
            v.m_मूल्य |= mask;
            v.m_मूल्य_X |= mask;
        }
    }

निजी:
    अक्षर bitIs(पूर्णांक bit) स्थिर {
        अगर (bit >= m_तथ्य.width() || bit < 0) {
            // We never sign extend
            वापस '0';
        }
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
        वापस ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    अक्षर bitIsExtend(पूर्णांक bit, पूर्णांक lbits) स्थिर {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        अगर (bit < 0) वापस '0';
        UASSERT(lbits <= m_तथ्य.width(), "Extend of wrong size");
        अगर (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
            // We do sign extend
            वापस ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
        वापस ("01zx"[(((v.m_मूल्य & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मूल्य_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

खुला:
    bool bitIs0(पूर्णांक bit) स्थिर VL_MT_SAFE {
        अगर (!is_संख्या()) वापस false;
        अगर (bit < 0) वापस false;
        अगर (bit >= m_तथ्य.width()) वापस !bitIsXZ(m_तथ्य.width() - 1);
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
        वापस ((v.m_मूल्य | v.m_मूल्य_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(पूर्णांक bit) स्थिर VL_MT_SAFE {
        अगर (!is_संख्या()) वापस false;
        अगर (bit < 0) वापस false;
        अगर (bit >= m_तथ्य.width()) वापस false;
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
        वापस ((v.m_मूल्य & (1UL << (bit & 31))) && !(v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(पूर्णांक bit) स्थिर {
        // Correct number of zero bits/width matters
        अगर (!is_संख्या()) वापस false;
        अगर (bit < 0) वापस false;
        अगर (bit >= m_तथ्य.width()) वापस bitIs1Extend(m_तथ्य.width() - 1);
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
        वापस ((v.m_मूल्य & (1UL << (bit & 31))) && !(v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsX(पूर्णांक bit) स्थिर VL_MT_SAFE {
        अगर (!is_संख्या()) वापस false;
        अगर (bit < 0) वापस false;
        अगर (bit >= m_तथ्य.width()) वापस bitIsZ(m_तथ्य.width() - 1);
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
        वापस ((v.m_मूल्य & (1UL << (bit & 31))) && (v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(पूर्णांक bit) स्थिर VL_MT_SAFE {
        अगर (!is_संख्या()) वापस false;
        अगर (bit < 0) वापस false;
        अगर (bit >= m_तथ्य.width()) वापस bitIsXZ(m_तथ्य.width() - 1);
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
        वापस ((v.m_मूल्य_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(पूर्णांक bit) स्थिर VL_MT_SAFE {
        अगर (!is_संख्या()) वापस false;
        अगर (bit < 0) वापस false;
        अगर (bit >= m_तथ्य.width()) वापस bitIsZ(m_तथ्य.width() - 1);
        स्थिर मूल्य_And_X v = m_तथ्य.अंक()[bit / 32];
        वापस ((~v.m_मूल्य & (1UL << (bit & 31))) && (v.m_मूल्य_X & (1UL << (bit & 31))));
    }

निजी:
    uint32_t bits_मूल्य(पूर्णांक lsb, पूर्णांक nbits) स्थिर VL_MT_SAFE {
        uint32_t v = 0;
        क्रम (पूर्णांक bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        वापस v;
    }

    पूर्णांक गिनती_X(पूर्णांक lsb, पूर्णांक nbits) स्थिर VL_MT_SAFE;
    पूर्णांक गिनती_Z(पूर्णांक lsb, पूर्णांक nbits) स्थिर VL_MT_SAFE;

    पूर्णांक शब्दगण() स्थिर VL_MT_SAFE { वापस ((width() + 31) / 32); }
    uint32_t hiWordMask() स्थिर VL_MT_SAFE {
        // Correct number of zero bits/width matters
        वापस VL_MASK_I(width());
    }

    V3_संख्या& opModDivGuts(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, bool is_modulus);

खुला:
    // CONSTRUCTORS
    explicit V3_संख्या(Ast_गाँठ* nodep) { init(nodep, 1); }
    V3_संख्या(Ast_गाँठ* nodep, पूर्णांक width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_संख्या(Ast_गाँठ* nodep, पूर्णांक width, स्थिर V3_संख्या& मूल्य) {
        init(nodep, width, width > 0);
        opAssign(मूल्य);
    }
    V3_संख्या(Ast_गाँठ* nodep, पूर्णांक width, uint32_t मूल्य, bool sized = true) {
        init(nodep, width, sized);
        m_तथ्य.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
    }
    V3_संख्या(File_पंक्ति* flp, पूर्णांक width, uint32_t मूल्य, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_तथ्य.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_संख्या_Literal {};  // For creator type-overload selection
    V3_संख्या(Ast_गाँठ* nodep, Verilog_संख्या_Literal, स्थिर अक्षर* sourcep) { create(nodep, sourcep); }
    V3_संख्या(File_पंक्ति* flp, Verilog_संख्या_Literal, स्थिर अक्षर* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_संख्या(Ast_गाँठ* nodep, VerilogStringLiteral, स्थिर string& str);
    class Double {};
    V3_संख्या(Ast_गाँठ* nodep, Double, द्विगुन मूल्य) {
        init(nodep, 64);
        setDouble(मूल्य);
    }
    class String {};
    V3_संख्या(Ast_गाँठ* nodep, String, स्थिर string& मूल्य) {
        init(nodep);
        setString(मूल्य);
        m_तथ्य.m_fromString = true;
    }
    class OneStep {};
    V3_संख्या(Ast_गाँठ* nodep, OneStep) {
        init(nodep, 64);
        m_तथ्य.m_is1Step = true;
    }
    class Null {};
    V3_संख्या(Ast_गाँठ* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_संख्या(स्थिर V3_संख्या* nump, पूर्णांक width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_संख्या(स्थिर V3_संख्या* nump, पूर्णांक width, uint32_t मूल्य) {
        init(nullptr, width);
        m_तथ्य.अंक()[0].m_मूल्य = मूल्य;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_संख्या(Ast_गाँठ* nodep, स्थिर Ast_गाँठ_DType* nodedtypep);

    V3_संख्या(स्थिर V3_संख्या& other) = शेष;
    V3_संख्या& चालक=(स्थिर V3_संख्या& other) = शेष;

    V3_संख्या(V3_संख्या&& other) = शेष;
    V3_संख्या& चालक=(V3_संख्या&& other) = शेष;

    ~V3_संख्या() = शेष;

निजी:
    व्योम selfTestThis();
    व्योम create(Ast_गाँठ* nodep, स्थिर अक्षर* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    व्योम create(File_पंक्ति* flp, स्थिर अक्षर* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    व्योम create(स्थिर अक्षर* sourcep);
    व्योम init(Ast_गाँठ* nodep, पूर्णांक swidth = -1, bool sized = true) {
        this->nodep(nodep);
        अगर (swidth >= 0) {
            अगर (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_तथ्य.setLogic();
            m_तथ्य.resize(swidth);
            m_तथ्य.m_sized = sized;
            क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) m_तथ्य.अंक()[i] = {0, 0};
        } अन्यथा {
            m_तथ्य.resize(1);
            m_तथ्य.m_sized = false;
        }
    }
    अटल string displayPad(माप_प्रकार fmtsize, अक्षर pad, bool बायाँ, स्थिर string& in) VL_PURE;
    string displayed(स्थिर string& vformat) स्थिर VL_MT_STABLE {
        वापस displayed(m_fileline, vformat);
    }
    व्योम warnTooMany(स्थिर string& मूल्य);

खुला:
    व्योम v3errorEnd(स्थिर std::ostringstream& sstr) स्थिर VL_RELEASE(V3Error::s().m_mutex);
    व्योम v3errorEndFatal(स्थिर std::ostringstream& sstr) स्थिर VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    व्योम width(पूर्णांक width, bool sized = true) {
        m_तथ्य.m_sized = sized;
        m_तथ्य.resize(width);
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
    string ascii(bool prefixed = true, bool cleanVerilog = false) स्थिर VL_MT_STABLE;
    string displayed(स्थिर Ast_गाँठ* nodep, स्थिर string& vformat,
                     स्थिर VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) स्थिर VL_MT_STABLE;
    string displayed(File_पंक्ति* fl, स्थिर string& vformat,
                     स्थिर VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) स्थिर VL_MT_STABLE;
    string displayedEnum(स्थिर AstSFormatArg* argp, स्थिर string& vformat) स्थिर VL_MT_STABLE;
    string displayedEnumName(स्थिर AstEnumDType* dtypep) स्थिर VL_MT_STABLE;
    अटल string displayedEnumName(स्थिर AstEnumItem* itemp);
    अटल bool displayedFmtHasArg(अक्षर format, bool isScan);
    string emitC() स्थिर VL_MT_STABLE;
    पूर्णांक width() स्थिर VL_MT_SAFE { वापस m_तथ्य.width(); }
    पूर्णांक widthToFit() स्थिर;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() स्थिर VL_MT_SAFE { वापस m_तथ्य.m_sized; }
    bool autoExtend() स्थिर VL_MT_SAFE { वापस m_तथ्य.m_autoExtend; }
    bool isFromString() स्थिर { वापस m_तथ्य.m_fromString; }
    bool hasOrigParamName() स्थिर { वापस m_तथ्य.m_hasOrigParamName; }
    व्योम hasOrigParamName(bool flag) { m_तथ्य.m_hasOrigParamName = flag; }
    V3_संख्या_तथ्य_Type तथ्य_Type() स्थिर VL_MT_SAFE { वापस m_तथ्य.type(); }
    व्योम तथ्य_Type(V3_संख्या_तथ्य_Type newType) {
        अगर (तथ्य_Type() == newType) वापस;
        UASSERT(newType != V3_संख्या_तथ्य_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        चयन (newType) {
        हाल V3_संख्या_तथ्य_Type::STRING: m_तथ्य.setString(); अवरोध;
        हाल V3_संख्या_तथ्य_Type::DOUBLE: m_तथ्य.setDouble(); अवरोध;
        हाल V3_संख्या_तथ्य_Type::LOGIC: m_तथ्य.setLogic(); अवरोध;
        हाल V3_संख्या_तथ्य_Type::UNINITIALIZED: अवरोध;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() स्थिर VL_MT_SAFE { वापस m_तथ्य.m_signed; }
    व्योम isSigned(bool ssigned) { m_तथ्य.m_signed = ssigned; }
    bool isDouble() स्थिर VL_MT_SAFE { वापस तथ्य_Type() == V3_संख्या_तथ्य_Type::DOUBLE; }
    bool isString() स्थिर VL_MT_SAFE { वापस तथ्य_Type() == V3_संख्या_तथ्य_Type::STRING; }
    bool isOpaque() स्थिर VL_MT_SAFE { वापस isDouble() || isString(); }
    bool is_संख्या() स्थिर VL_MT_SAFE {
        वापस m_तथ्य.type() == V3_संख्या_तथ्य_Type::LOGIC
               || m_तथ्य.type() == V3_संख्या_तथ्य_Type::DOUBLE;
    }
    bool isNegative() स्थिर VL_MT_SAFE {
        // Correct number of zero bits/width matters
        वापस !isString() && bitIs1(width() - 1);
    }
    bool is1Step() स्थिर VL_MT_SAFE { वापस m_तथ्य.m_is1Step; }
    bool isNull() स्थिर VL_MT_SAFE { वापस m_तथ्य.m_isNull; }
    bool isFourState() स्थिर VL_MT_SAFE;
    bool hasZ() स्थिर {
        अगर (isString()) वापस false;
        क्रम (पूर्णांक i = 0; i < शब्दगण(); ++i) {
            स्थिर मूल्य_And_X v = m_तथ्य.अंक()[i];
            अगर ((~v.m_मूल्य) & v.m_मूल्य_X) वापस true;
        }
        वापस false;
    }
    bool isAllZ() स्थिर VL_MT_SAFE;
    bool isAllX() स्थिर VL_MT_SAFE;
    bool isEqZero() स्थिर VL_MT_SAFE;
    bool isNeqZero() स्थिर;
    bool isBitsZero(पूर्णांक msb, पूर्णांक lsb) स्थिर;
    bool isBroken(पूर्णांक vwidth) स्थिर;
    bool isEqOne() स्थिर;
    bool isEqAllOnes(पूर्णांक optwidth = 0) स्थिर;
    bool isCaseEq(स्थिर V3_संख्या& rhs) स्थिर;  // operator==
    bool isLtXZ(स्थिर V3_संख्या& rhs) स्थिर;  // operator< with XZ compared
    bool isAnyX() स्थिर VL_MT_SAFE;
    bool isAnyXZ() स्थिर;
    bool isAnyZ() स्थिर VL_MT_SAFE;
    bool isMsbXZ() स्थिर {
        // Correct number of zero bits/width matters
        वापस bitIsXZ(width() - 1);
    }
    bool fitsInUInt() स्थिर VL_MT_SAFE;
    uint32_t toUInt() स्थिर VL_MT_SAFE;
    int32_t toSInt() स्थिर VL_MT_SAFE;
    uint64_t toUQuad() स्थिर VL_MT_SAFE;
    int64_t toSQuad() स्थिर VL_MT_SAFE;
    string toString() स्थिर VL_MT_SAFE;
    string toDecimalS() स्थिर VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() स्थिर VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() स्थिर VL_MT_STABLE;
    द्विगुन toDouble() स्थिर VL_MT_SAFE;
    V3Hash toHash() स्थिर;
    uint32_t edataWord(पूर्णांक eword) स्थिर;
    uint8_t तथ्य_Byte(पूर्णांक byte) स्थिर;
    uint32_t गिनती_Bits(स्थिर V3_संख्या& ctrl) स्थिर;
    uint32_t गिनती_Bits(स्थिर V3_संख्या& ctrl1, स्थिर V3_संख्या& ctrl2, स्थिर V3_संख्या& ctrl3) स्थिर;
    uint32_t गिनती_Ones() स्थिर;
    uint32_t mostSetBitP1() स्थिर;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() स्थिर;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool चालक<(स्थिर V3_संख्या& rhs) स्थिर { वापस isLtXZ(rhs); }

    // STATICS
    अटल bool epsilonEqual(द्विगुन a, द्विगुन b);  // True if number within Epsilon of the other
    अटल bool epsilonIntegral(द्विगुन a);  // True if number rounds to integer within Epsilon
    अटल पूर्णांक log2b(uint32_t अंक);
    अटल पूर्णांक log2bQuad(uint64_t अंक);
    अटल व्योम selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_संख्या& opBitsNonXZ(स्थिर V3_संख्या& lhs);  // 0/1->1, X/Z->0
    V3_संख्या& opBitsOne(स्थिर V3_संख्या& lhs);  // 1->1, 0/X/Z->0
    V3_संख्या& opBitsXZ(स्थिर V3_संख्या& lhs);  // 0/1->0, X/Z->1
    V3_संख्या& opBitsZ(स्थिर V3_संख्या& lhs);  // Z->1, 0/1/X->0
    //
    V3_संख्या& opAssign(स्थिर V3_संख्या& lhs);
    V3_संख्या& opAssignNonXZ(स्थिर V3_संख्या& lhs, bool ignoreXZ = true);
    V3_संख्या& opExtendS(स्थिर V3_संख्या& lhs, uint32_t lbits);  // Sign extension
    V3_संख्या& opExtendXZ(स्थिर V3_संख्या& lhs, uint32_t lbits);  // X/Z extension
    V3_संख्या& opRedOr(स्थिर V3_संख्या& lhs);
    V3_संख्या& opRedAnd(स्थिर V3_संख्या& lhs);
    V3_संख्या& opRedXor(स्थिर V3_संख्या& lhs);
    V3_संख्या& op_गिनती_Bits(स्थिर V3_संख्या& व्यंजक, स्थिर V3_संख्या& ctrl1, स्थिर V3_संख्या& ctrl2,
                          स्थिर V3_संख्या& ctrl3);
    V3_संख्या& op_गिनती_Ones(स्थिर V3_संख्या& lhs);
    V3_संख्या& opIsUnknown(स्थिर V3_संख्या& lhs);
    V3_संख्या& opOneHot(स्थिर V3_संख्या& lhs);
    V3_संख्या& opOneHot0(स्थिर V3_संख्या& lhs);
    V3_संख्या& opCLog2(स्थिर V3_संख्या& lhs);
    V3_संख्या& opMostSetBitP1(स्थिर V3_संख्या& lhs);
    V3_संख्या& opClean(स्थिर V3_संख्या& lhs, uint32_t bits);
    V3_संख्या& opConcat(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLenN(स्थिर V3_संख्या& lhs);
    V3_संख्या& opRepl(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opRepl(स्थिर V3_संख्या& lhs, uint32_t rhsval);
    V3_संख्या& opStreamL(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opSel(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& msb, स्थिर V3_संख्या& lsb);
    V3_संख्या& opSel(स्थिर V3_संख्या& lhs, uint32_t msbval, uint32_t lsbval);
    V3_संख्या& opSelInto(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& lsb, पूर्णांक width);
    V3_संख्या& opSelInto(स्थिर V3_संख्या& lhs, पूर्णांक lsbval, पूर्णांक width);
    V3_संख्या& opCvtPackString(स्थिर V3_संख्या& lhs);
    V3_संख्या& opToLowerN(स्थिर V3_संख्या& lhs);
    V3_संख्या& opToUpperN(स्थिर V3_संख्या& lhs);
    V3_संख्या& opCaseEq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opCaseNeq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opWildEq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opWildNeq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opBufIf1(स्थिर V3_संख्या& ens, स्थिर V3_संख्या& if1s);
    V3_संख्या& opSetRange(uint32_t lsb, uint32_t width, अक्षर bit_मूल्य);
    // "standard" math
    V3_संख्या& opNot(स्थिर V3_संख्या& lhs);
    V3_संख्या& opLogNot(स्थिर V3_संख्या& lhs);
    V3_संख्या& opLogAnd(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLogOr(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLogEq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLogIf(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opNegate(स्थिर V3_संख्या& lhs);
    V3_संख्या& opAdd(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opSub(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opMul(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opMulS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed
    V3_संख्या& opDiv(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opDivS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed
    V3_संख्या& opModDiv(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opModDivS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed
    V3_संख्या& opPow(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, bool lsign = false,
                    bool rsign = false);
    V3_संख्या& opPowSU(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed lhs, unsigned rhs
    V3_संख्या& opPowSS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed lhs, signed rhs
    V3_संख्या& opPowUS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Unsigned lhs, signed rhs
    V3_संख्या& opAnd(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opXor(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opOr(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opShiftR(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opShiftRS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_संख्या& opShiftL(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    // Comparisons
    V3_संख्या& opEq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opNeq(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opGt(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opGtS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed
    V3_संख्या& opGte(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opGteS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed
    V3_संख्या& opLt(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLtS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed
    V3_संख्या& opLte(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLteS(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);  // Signed

    // "D" - double (aka real) math
    V3_संख्या& opIToRD(स्थिर V3_संख्या& lhs, bool isSigned = false);
    V3_संख्या& opISToRD(स्थिर V3_संख्या& lhs) { वापस opIToRD(lhs, true); }
    V3_संख्या& opRToIS(स्थिर V3_संख्या& lhs);
    V3_संख्या& opRToIRoundS(स्थिर V3_संख्या& lhs);
    V3_संख्या& opRealToBits(स्थिर V3_संख्या& lhs);
    V3_संख्या& opBitsToRealD(स्थिर V3_संख्या& lhs);
    V3_संख्या& opNegateD(स्थिर V3_संख्या& lhs);
    V3_संख्या& opAddD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opSubD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opMulD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opDivD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opPowD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    // Comparisons
    V3_संख्या& opEqD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opNeqD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opGtD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opGteD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLtD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLteD(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);

    // "N" - string operations
    V3_संख्या& opAtoN(स्थिर V3_संख्या& lhs, पूर्णांक base);
    V3_संख्या& opNToI(स्थिर V3_संख्या& lhs);
    V3_संख्या& opPutcN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, स्थिर V3_संख्या& ths);
    V3_संख्या& opGetcN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opSubstrN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, स्थिर V3_संख्या& ths);
    V3_संख्या& opCompareNN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs, bool ignoreCase);
    V3_संख्या& opConcatN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opReplN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opReplN(स्थिर V3_संख्या& lhs, uint32_t rhsval);
    V3_संख्या& opEqN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opNeqN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opGtN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opGteN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLtN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
    V3_संख्या& opLteN(स्थिर V3_संख्या& lhs, स्थिर V3_संख्या& rhs);
};
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर V3_संख्या& rhs) VL_MT_SAFE {
    वापस os << rhs.ascii();
}

#पूर्ण_अगर  // Guard
