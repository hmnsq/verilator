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

#यदि_अपरिभाषितम् VERILATOR_V3_सङ्ख्या_H
#परिभाषय VERILATOR_V3_सङ्ख्या_H

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
class Ast_ग्रन्थिः;
class Ast_ग्रन्थिः_DType;
class AstSFormatArg;
class File_पङ्क्तिः;

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
    constexpr संक्रियकः en() ध्रुव { प्रत्यागम m_e; }
    वर्ण ascii() ध्रुव { प्रत्यागम m_e; }
    bool isComplex() ध्रुव { प्रत्यागम m_e == COMPLEX; }
    bool isDouble() ध्रुव { प्रत्यागम m_e == DOUBLE; }
    bool isEnum() ध्रुव { प्रत्यागम m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() ध्रुव { प्रत्यागम m_e == SIGNED; }
    bool isString() ध्रुव { प्रत्यागम m_e == STRING; }
    bool isUnsigned() ध्रुव { प्रत्यागम m_e == UNSIGNED; }
};
constexpr bool संक्रियकः==(ध्रुव VFormatAttr& lhs, ध्रुव VFormatAttr& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool संक्रियकः==(ध्रुव VFormatAttr& lhs, VFormatAttr::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool संक्रियकः==(VFormatAttr::en lhs, ध्रुव VFormatAttr& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& संक्रियकः<<(std::ostream& os, ध्रुव VFormatAttr& rhs) {
    प्रत्यागम os << rhs.ascii();
}

//============================================================================

class V3_सङ्ख्या_दत्त final {
सार्वजनिक:
    // TYPES
    संरचना मानम्_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_मानम्;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_मानम्_X;
        bool संक्रियकः==(ध्रुव मानम्_And_X& other) ध्रुव VL_MT_SAFE {
            प्रत्यागम m_मानम् == other.m_मानम् && m_मानम्_X == other.m_मानम्_X;
        }
    };

    गणना class V3_सङ्ख्या_दत्त_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    मित्रम् std::ostream& संक्रियकः<<(std::ostream& os, ध्रुव V3_सङ्ख्या_दत्त_Type& rhs) VL_MT_SAFE {
        विकल्प (rhs) {
        पक्ष V3_सङ्ख्या_दत्त_Type::UNINITIALIZED: प्रत्यागम os << "UNINITIALIZED";
        पक्ष V3_सङ्ख्या_दत्त_Type::LOGIC: प्रत्यागम os << "LOGIC";
        पक्ष V3_सङ्ख्या_दत्त_Type::DOUBLE: प्रत्यागम os << "DOUBLE";
        पक्ष V3_सङ्ख्या_दत्त_Type::STRING: प्रत्यागम os << "STRING";
        }
        प्रत्यागम os;
    }

गुप्त:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    स्थिर constexpr पूर्णाङ्क INLINE_शब्दाः = std::max(
        static_cast<आकार_t>(2), std::max(परिमाण(std::string) / परिमाण(मानम्_And_X),
                                         परिमाण(std::सदिशम्<मानम्_And_X>) / परिमाण(मानम्_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    स्थिर constexpr पूर्णाङ्क MAX_INLINE_विस्तारः = INLINE_शब्दाः * परिमाण(मानम्_And_X) / 2 * 8;

    // MEMBERS
    सङ्घ {
        std::array<मानम्_And_X, INLINE_शब्दाः> m_inline_सङ्ख्या;
        std::सदिशम्<मानम्_And_X> m_dynamic_सङ्ख्या;
        std::string m_string;
    };

    पूर्णाङ्क m_विस्तारः = 0;  // Width (in bits) as specified/calculated
    V3_सङ्ख्या_दत्त_Type m_type;

सार्वजनिक:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_चरणः : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_सङ्ख्या_दत्त()
        : m_type{V3_सङ्ख्या_दत्त_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_चरणः{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_सङ्ख्या_दत्त() { destroy_Stored_मानम्(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_सङ्ख्या_दत्त(ध्रुव V3_सङ्ख्या_दत्त& other)
        : m_विस्तारः{other.m_विस्तारः}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_चरणः{other.m_is1_चरणः}
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

    V3_सङ्ख्या_दत्त& संक्रियकः=(ध्रुव V3_सङ्ख्या_दत्त& other) {
        यदि (other.is_Inline_सङ्ख्या()) {
            destroy_Stored_मानम्();
            init_Inline_सङ्ख्या(other.m_inline_सङ्ख्या);
        } अन्यथा यदि (other.is_Dynamic_सङ्ख्या()) {
            reinit_With_Or_नियोजय_Dynamic_सङ्ख्या(other.m_dynamic_सङ्ख्या);
        } अन्यथा यदि (other.isString()) {
            reinit_With_Or_नियोजय_String(other.m_string);
        } अन्यथा {
            destroy_Stored_मानम्();
        }
        m_विस्तारः = other.m_विस्तारः;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_चरणः = other.m_is1_चरणः;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        प्रत्यागम *this;
    }

    V3_सङ्ख्या_दत्त(V3_सङ्ख्या_दत्त&& other)
        : m_विस्तारः{other.m_विस्तारः}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_चरणः{other.m_is1_चरणः}
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
        other.m_type = V3_सङ्ख्या_दत्त_Type::UNINITIALIZED;
    }

    V3_सङ्ख्या_दत्त& संक्रियकः=(V3_सङ्ख्या_दत्त&& other) {
        यदि (other.is_Inline_सङ्ख्या()) {
            destroy_Stored_मानम्();
            init_Inline_सङ्ख्या(other.m_inline_सङ्ख्या);
        } अन्यथा यदि (other.is_Dynamic_सङ्ख्या()) {
            reinit_With_Or_नियोजय_Dynamic_सङ्ख्या(std::move(other.m_dynamic_सङ्ख्या));
        } अन्यथा यदि (other.isString()) {
            reinit_With_Or_नियोजय_String(std::move(other.m_string));
        } अन्यथा {
            destroy_Stored_मानम्();
        }
        m_विस्तारः = other.m_विस्तारः;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_चरणः = other.m_is1_चरणः;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_सङ्ख्या_दत्त_Type::UNINITIALIZED;
        प्रत्यागम *this;
    }

    // ACCESSORS
    मानम्_And_X* अङ्कः() {
        UASSERT(is_सङ्ख्या(), "`num` member accessed when data type is " << m_type);
        प्रत्यागम is_Inline_सङ्ख्या() ? m_inline_सङ्ख्या.दत्त() : m_dynamic_सङ्ख्या.दत्त();
    }
    ध्रुव मानम्_And_X* अङ्कः() ध्रुव VL_MT_SAFE {
        UASSERT(is_सङ्ख्या(), "`num` member accessed when data type is " << m_type);
        प्रत्यागम is_Inline_सङ्ख्या() ? m_inline_सङ्ख्या.दत्त() : m_dynamic_सङ्ख्या.दत्त();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        प्रत्यागम m_string;
    }
    ध्रुव std::string& str() ध्रुव VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        प्रत्यागम m_string;
    }

    पूर्णाङ्क विस्तारः() ध्रुव VL_MT_SAFE { प्रत्यागम m_विस्तारः; }
    V3_सङ्ख्या_दत्त_Type type() ध्रुव VL_MT_SAFE { प्रत्यागम m_type; }

    // METHODS
    शून्य resize(पूर्णाङ्क द्वयङ्काः_गणनम्) {
        यदि (m_विस्तारः == द्वयङ्काः_गणनम्) प्रत्यागम;
        यदि (द्वयङ्काः_गन्तव्यम्_शब्दाः(m_विस्तारः) == द्वयङ्काः_गन्तव्यम्_शब्दाः(द्वयङ्काः_गणनम्)) {
            m_विस्तारः = द्वयङ्काः_गणनम्;
            प्रत्यागम;
        }
        यदि (is_Dynamic_सङ्ख्या()) {
            यदि (द्वयङ्काः_गणनम् > MAX_INLINE_विस्तारः) {
                m_dynamic_सङ्ख्या.resize(द्वयङ्काः_गन्तव्यम्_शब्दाः(द्वयङ्काः_गणनम्));
            } अन्यथा {
                ध्रुव स्वचालित dynamic_द्वयङ्काः = std::move(m_dynamic_सङ्ख्या);
                destroy_Dynamic_सङ्ख्या();
                init_Inline_सङ्ख्या();
                std::स्मृतिं_प्रतिलिख(m_inline_सङ्ख्या.दत्त(), dynamic_द्वयङ्काः.दत्त(), परिमाण(m_inline_सङ्ख्या));
            }
        } अन्यथा यदि (is_Inline_सङ्ख्या()) {
            यदि (द्वयङ्काः_गणनम् > MAX_INLINE_विस्तारः) {
                ध्रुव स्वचालित द्वयङ्काः = m_inline_सङ्ख्या;
                init_Dynamic_सङ्ख्या(द्वयङ्काः_गन्तव्यम्_शब्दाः(द्वयङ्काः_गणनम्));
                std::स्मृतिं_प्रतिलिख(m_dynamic_सङ्ख्या.दत्त(), द्वयङ्काः.दत्त(), परिमाण(द्वयङ्काः));
            }
        }
        m_विस्तारः = द्वयङ्काः_गणनम्;
    }

    शून्य समुच्चयः_String() {
        // If there has been a string already it is kept intact.
        यदि (isString()) प्रत्यागम;
        यदि (is_Dynamic_सङ्ख्या()) destroy_Dynamic_सङ्ख्या();
        initString();
        m_type = V3_सङ्ख्या_दत्त_Type::STRING;
    }
    शून्य समुच्चयः_String(std::string&& s) {
        reinit_With_Or_नियोजय_String(std::move(s));
        m_type = V3_सङ्ख्या_दत्त_Type::STRING;
    }
    शून्य समुच्चयः_String(ध्रुव std::string& s) {
        reinit_With_Or_नियोजय_String(s);
        m_type = V3_सङ्ख्या_दत्त_Type::STRING;
    }

    शून्य समुच्चयः_Double() {
        destroy_Stored_मानम्();
        यदि (!is_Inline_सङ्ख्या()) init_Inline_सङ्ख्या();
        m_type = V3_सङ्ख्या_दत्त_Type::DOUBLE;
        resize(64);
    }

    शून्य समुच्चयः_Logic() {
        यदि (isString()) destroyString();
        यदि (!is_सङ्ख्या()) {
            यदि (m_विस्तारः <= MAX_INLINE_विस्तारः) {
                init_Inline_सङ्ख्या();
            } अन्यथा {
                init_Dynamic_सङ्ख्या(द्वयङ्काः_गन्तव्यम्_शब्दाः(m_विस्तारः));
            }
        }
        m_type = V3_सङ्ख्या_दत्त_Type::LOGIC;
        resize(m_विस्तारः);
    }

गुप्त:
    स्थिर constexpr पूर्णाङ्क द्वयङ्काः_गन्तव्यम्_शब्दाः(पूर्णाङ्क द्वयङ्काः_गणनम्) VL_विशुद्धम् { प्रत्यागम (द्वयङ्काः_गणनम् + 31) / 32; }

    bool is_सङ्ख्या() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_type == V3_सङ्ख्या_दत्त_Type::DOUBLE || m_type == V3_सङ्ख्या_दत्त_Type::LOGIC;
    }
    bool is_Inline_सङ्ख्या() ध्रुव VL_MT_SAFE {
        प्रत्यागम (m_विस्तारः <= MAX_INLINE_विस्तारः)
               && (m_type == V3_सङ्ख्या_दत्त_Type::DOUBLE || m_type == V3_सङ्ख्या_दत्त_Type::LOGIC);
    }
    bool is_Dynamic_सङ्ख्या() ध्रुव VL_MT_SAFE {
        प्रत्यागम (m_विस्तारः > MAX_INLINE_विस्तारः) && (m_type == V3_सङ्ख्या_दत्त_Type::LOGIC);
    }
    bool isString() ध्रुव VL_MT_SAFE { प्रत्यागम m_type == V3_सङ्ख्या_दत्त_Type::STRING; }

    प्रतिरूपम् <typename... तर्काः_2>
    शून्य init_Inline_सङ्ख्या(तर्काः_2&&... तर्काः) {
        new (&m_inline_सङ्ख्या) std::array<मानम्_And_X, INLINE_शब्दाः>(std::forward<तर्काः_2>(तर्काः)...);
    }
    प्रतिरूपम् <typename... तर्काः_2>
    शून्य init_Dynamic_सङ्ख्या(तर्काः_2&&... तर्काः) {
        new (&m_dynamic_सङ्ख्या) std::सदिशम्<मानम्_And_X>(std::forward<तर्काः_2>(तर्काः)...);
    }
    प्रतिरूपम् <typename... तर्काः_2>
    शून्य initString(तर्काः_2&&... तर्काः) {
        new (&m_string) std::string(std::forward<तर्काः_2>(तर्काः)...);
    }

    string formatDecimal();

    शून्य destroy_Dynamic_सङ्ख्या() { m_dynamic_सङ्ख्या.~सदिशम्(); }
    शून्य destroyString() { m_string.~string(); }
    शून्य destroy_Stored_मानम्() {
        यदि (isString())
            destroyString();
        अन्यथा यदि (is_Dynamic_सङ्ख्या())
            destroy_Dynamic_सङ्ख्या();
    }

    प्रतिरूपम् <typename T>
    शून्य reinit_With_Or_नियोजय_Dynamic_सङ्ख्या(T&& s) {
        यदि (is_Dynamic_सङ्ख्या()) {
            m_dynamic_सङ्ख्या = std::forward<T>(s);
            प्रत्यागम;
        }
        यदि (isString()) destroyString();
        init_Dynamic_सङ्ख्या(std::forward<T>(s));
    }
    प्रतिरूपम् <typename T>
    शून्य reinit_With_Or_नियोजय_String(T&& s) {
        यदि (isString()) {
            m_string = std::forward<T>(s);
            प्रत्यागम;
        }
        यदि (is_Dynamic_सङ्ख्या()) destroy_Dynamic_सङ्ख्या();
        initString(std::forward<T>(s));
    }
};

class V3_सङ्ख्या final {
    // TYPES
    using मानम्_And_X = V3_सङ्ख्या_दत्त::मानम्_And_X;
    using V3_सङ्ख्या_दत्त_Type = V3_सङ्ख्या_दत्त::V3_सङ्ख्या_दत्त_Type;

    // MEMBERS
    V3_सङ्ख्या_दत्त m_दत्त;
    Ast_ग्रन्थिः* m_nodep = nullptr;  // Parent node - for error reporting only
    File_पङ्क्तिः* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    शून्य समुच्चयः_Null() {
        m_दत्त.समुच्चयः_Logic();
        m_दत्त.m_isNull = true;
        m_दत्त.m_autoExtend = true;
    }
    V3_सङ्ख्या& समुच्चयः_Single_द्वयङ्काः(वर्ण मानम्);
    शून्य fill_द्वयङ्काः(पूर्णाङ्क destLsb, पूर्णाङ्क विस्तारः, वर्ण मानम्);
    शून्य copy_द्वयङ्काः(पूर्णाङ्क destLsb, ध्रुव V3_सङ्ख्या& source, पूर्णाङ्क sourceLsb, पूर्णाङ्क विस्तारः);
    V3_सङ्ख्या& समुच्चयः_String(ध्रुव string& str) {
        m_दत्त.समुच्चयः_String(str);
        प्रत्यागम *this;
    }
    शून्य opCleanThis(bool warnOnTruncation = false);

सार्वजनिक:
    शून्य nodep(Ast_ग्रन्थिः* nodep) VL_MT_STABLE;
    Ast_ग्रन्थिः* nodep() ध्रुव VL_MT_SAFE { प्रत्यागम m_nodep; }  // For debug only
    File_पङ्क्तिः* fileline() ध्रुव VL_MT_SAFE { प्रत्यागम m_fileline; }
    V3_सङ्ख्या& समुच्चयः_Zero();
    V3_सङ्ख्या& समुच्चयः_Quad(uint64_t मानम्);
    V3_सङ्ख्या& समुच्चयः_Long(uint32_t मानम्);
    V3_सङ्ख्या& समुच्चयः_Long_S(int32_t मानम्);
    V3_सङ्ख्या& समुच्चयः_Double(द्विगुण मानम्);
    शून्य समुच्चयः_Bit_X0(पूर्णाङ्क bit);
    शून्य समुच्चयः_Bit(पूर्णाङ्क bit, वर्ण मानम्) {  // Note: must be initialized as number and pre-zeroed!
        यदि (bit >= m_दत्त.विस्तारः()) प्रत्यागम;
        ध्रुव uint32_t आवरणम् = (1UL << (bit & 31));
        मानम्_And_X& v = m_दत्त.अङ्कः()[bit / 32];
        यदि (मानम् == '0' || मानम् == 0) {
            v.m_मानम् &= ~आवरणम्;
            v.m_मानम्_X &= ~आवरणम्;
        } अन्यथा यदि (मानम् == '1' || मानम् == 1) {
            v.m_मानम् |= आवरणम्;
            v.m_मानम्_X &= ~आवरणम्;
        } अन्यथा यदि (मानम् == 'z' || मानम् == 2) {
            v.m_मानम् &= ~आवरणम्;
            v.m_मानम्_X |= आवरणम्;
        } अन्यथा {  // X
            v.m_मानम् |= आवरणम्;
            v.m_मानम्_X |= आवरणम्;
        }
    }

गुप्त:
    वर्ण bitIs(पूर्णाङ्क bit) ध्रुव {
        यदि (bit >= m_दत्त.विस्तारः() || bit < 0) {
            // We never sign extend
            प्रत्यागम '0';
        }
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
        प्रत्यागम ("01zx"[(((v.m_मानम् & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मानम्_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    वर्ण bitIsExtend(पूर्णाङ्क bit, पूर्णाङ्क lbits) ध्रुव {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        यदि (bit < 0) प्रत्यागम '0';
        UASSERT(lbits <= m_दत्त.विस्तारः(), "Extend of wrong size");
        यदि (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
            // We do sign extend
            प्रत्यागम ("01zx"[(((v.m_मानम् & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_मानम्_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
        प्रत्यागम ("01zx"[(((v.m_मानम् & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_मानम्_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

सार्वजनिक:
    bool bitIs0(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_दत्त.विस्तारः()) प्रत्यागम !bitIsXZ(m_दत्त.विस्तारः() - 1);
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
        प्रत्यागम ((v.m_मानम् | v.m_मानम्_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_दत्त.विस्तारः()) प्रत्यागम false;
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
        प्रत्यागम ((v.m_मानम् & (1UL << (bit & 31))) && !(v.m_मानम्_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(पूर्णाङ्क bit) ध्रुव {
        // Correct number of zero bits/width matters
        यदि (!is_सङ्ख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_दत्त.विस्तारः()) प्रत्यागम bitIs1Extend(m_दत्त.विस्तारः() - 1);
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
        प्रत्यागम ((v.m_मानम् & (1UL << (bit & 31))) && !(v.m_मानम्_X & (1UL << (bit & 31))));
    }
    bool bitIsX(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_दत्त.विस्तारः()) प्रत्यागम bitIsZ(m_दत्त.विस्तारः() - 1);
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
        प्रत्यागम ((v.m_मानम् & (1UL << (bit & 31))) && (v.m_मानम्_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_दत्त.विस्तारः()) प्रत्यागम bitIsXZ(m_दत्त.विस्तारः() - 1);
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
        प्रत्यागम ((v.m_मानम्_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(पूर्णाङ्क bit) ध्रुव VL_MT_SAFE {
        यदि (!is_सङ्ख्या()) प्रत्यागम false;
        यदि (bit < 0) प्रत्यागम false;
        यदि (bit >= m_दत्त.विस्तारः()) प्रत्यागम bitIsZ(m_दत्त.विस्तारः() - 1);
        ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[bit / 32];
        प्रत्यागम ((~v.m_मानम् & (1UL << (bit & 31))) && (v.m_मानम्_X & (1UL << (bit & 31))));
    }

गुप्त:
    uint32_t द्वयङ्काः_मानम्(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE {
        uint32_t v = 0;
        प्रति (पूर्णाङ्क bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        प्रत्यागम v;
    }

    पूर्णाङ्क गणनम्_X(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE;
    पूर्णाङ्क गणनम्_Z(पूर्णाङ्क lsb, पूर्णाङ्क nbits) ध्रुव VL_MT_SAFE;

    पूर्णाङ्क शब्दाः() ध्रुव VL_MT_SAFE { प्रत्यागम ((विस्तारः() + 31) / 32); }
    uint32_t hi_Word_आवरणम्() ध्रुव VL_MT_SAFE {
        // Correct number of zero bits/width matters
        प्रत्यागम VL_आवरणम्_I(विस्तारः());
    }

    V3_सङ्ख्या& opModDivGuts(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, bool is_modulus);

सार्वजनिक:
    // CONSTRUCTORS
    explicit V3_सङ्ख्या(Ast_ग्रन्थिः* nodep) { init(nodep, 1); }
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, पूर्णाङ्क विस्तारः) {  // 0=unsized
        init(nodep, विस्तारः, विस्तारः > 0);
    }
    // Construct with value, changing to new width
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, पूर्णाङ्क विस्तारः, ध्रुव V3_सङ्ख्या& मानम्) {
        init(nodep, विस्तारः, विस्तारः > 0);
        op_नियोजय(मानम्);
    }
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, पूर्णाङ्क विस्तारः, uint32_t मानम्, bool sized = true) {
        init(nodep, विस्तारः, sized);
        m_दत्त.अङ्कः()[0].m_मानम् = मानम्;
        opCleanThis();
    }
    V3_सङ्ख्या(File_पङ्क्तिः* flp, पूर्णाङ्क विस्तारः, uint32_t मानम्, bool sized = true) {
        init(nullptr, विस्तारः, sized);
        m_fileline = flp;
        m_दत्त.अङ्कः()[0].m_मानम् = मानम्;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_सङ्ख्या_Literal {};  // For creator type-overload selection
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, Verilog_सङ्ख्या_Literal, ध्रुव वर्ण* sourcep) { create(nodep, sourcep); }
    V3_सङ्ख्या(File_पङ्क्तिः* flp, Verilog_सङ्ख्या_Literal, ध्रुव वर्ण* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, VerilogStringLiteral, ध्रुव string& str);
    class Double {};
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, Double, द्विगुण मानम्) {
        init(nodep, 64);
        समुच्चयः_Double(मानम्);
    }
    class String {};
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, String, ध्रुव string& मानम्) {
        init(nodep);
        समुच्चयः_String(मानम्);
        m_दत्त.m_fromString = true;
    }
    class One_चरणः {};
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, One_चरणः) {
        init(nodep, 64);
        m_दत्त.m_is1_चरणः = true;
    }
    class Null {};
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, Null) {
        init(nodep);
        समुच्चयः_Null();
    }
    explicit V3_सङ्ख्या(ध्रुव V3_सङ्ख्या* nump, पूर्णाङ्क विस्तारः = 1) {
        init(nullptr, विस्तारः);
        m_fileline = nump->fileline();
    }
    V3_सङ्ख्या(ध्रुव V3_सङ्ख्या* nump, पूर्णाङ्क विस्तारः, uint32_t मानम्) {
        init(nullptr, विस्तारः);
        m_दत्त.अङ्कः()[0].m_मानम् = मानम्;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_सङ्ख्या(Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः_DType* nodedtypep);

    V3_सङ्ख्या(ध्रुव V3_सङ्ख्या& other) = स्वभाव;
    V3_सङ्ख्या& संक्रियकः=(ध्रुव V3_सङ्ख्या& other) = स्वभाव;

    V3_सङ्ख्या(V3_सङ्ख्या&& other) = स्वभाव;
    V3_सङ्ख्या& संक्रियकः=(V3_सङ्ख्या&& other) = स्वभाव;

    ~V3_सङ्ख्या() = स्वभाव;

गुप्त:
    शून्य selfTestThis();
    शून्य create(Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    शून्य create(File_पङ्क्तिः* flp, ध्रुव वर्ण* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    शून्य create(ध्रुव वर्ण* sourcep);
    शून्य init(Ast_ग्रन्थिः* nodep, पूर्णाङ्क swidth = -1, bool sized = true) {
        this->nodep(nodep);
        यदि (swidth >= 0) {
            यदि (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_दत्त.समुच्चयः_Logic();
            m_दत्त.resize(swidth);
            m_दत्त.m_sized = sized;
            प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) m_दत्त.अङ्कः()[i] = {0, 0};
        } अन्यथा {
            m_दत्त.resize(1);
            m_दत्त.m_sized = false;
        }
    }
    स्थिर string displayPad(आकार_t fmtsize, वर्ण pad, bool वामः, ध्रुव string& अन्तर्) VL_विशुद्धम्;
    string displayed(ध्रुव string& vformat) ध्रुव VL_MT_STABLE {
        प्रत्यागम displayed(m_fileline, vformat);
    }
    शून्य warnTooMany(ध्रुव string& मानम्);

सार्वजनिक:
    शून्य v3error_अन्तः(ध्रुव std::ostringstream& sstr) ध्रुव VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3error_अन्तः_Fatal(ध्रुव std::ostringstream& sstr) ध्रुव VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    शून्य विस्तारः(पूर्णाङ्क विस्तारः, bool sized = true) {
        m_दत्त.m_sized = sized;
        m_दत्त.resize(विस्तारः);
    }

    // SETTERS
    V3_सङ्ख्या& समुच्चयः_All_द्वयङ्काः_XRemoved();
    V3_सङ्ख्या& समुच्चयः_All_द्वयङ्काः_X();
    V3_सङ्ख्या& समुच्चयः_All_द्वयङ्काः_Z();
    V3_सङ्ख्या& समुच्चयः_All_Bits0();
    V3_सङ्ख्या& समुच्चयः_All_Bits1();
    V3_सङ्ख्या& समुच्चयः_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_सङ्ख्या& समुच्चयः_आवरणम्(पूर्णाङ्क nbits, पूर्णाङ्क lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) ध्रुव VL_MT_STABLE;
    string displayed(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& vformat,
                     ध्रुव VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ध्रुव VL_MT_STABLE;
    string displayed(File_पङ्क्तिः* fl, ध्रुव string& vformat,
                     ध्रुव VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) ध्रुव VL_MT_STABLE;
    string displayedEnum(ध्रुव AstSFormatArg* argp, ध्रुव string& vformat) ध्रुव VL_MT_STABLE;
    string displayedEnumName(ध्रुव AstEnumDType* dtypep) ध्रुव VL_MT_STABLE;
    स्थिर string displayedEnumName(ध्रुव AstEnumItem* itemp);
    स्थिर bool displayedFmtHasArg(वर्ण format, bool is_अवलोकनम्);
    string emitC() ध्रुव VL_MT_STABLE;
    पूर्णाङ्क विस्तारः() ध्रुव VL_MT_SAFE { प्रत्यागम m_दत्त.विस्तारः(); }
    पूर्णाङ्क विस्तारः_गन्तव्यम्_Fit() ध्रुव;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() ध्रुव VL_MT_SAFE { प्रत्यागम m_दत्त.m_sized; }
    bool autoExtend() ध्रुव VL_MT_SAFE { प्रत्यागम m_दत्त.m_autoExtend; }
    bool isFromString() ध्रुव { प्रत्यागम m_दत्त.m_fromString; }
    bool hasOrigParamName() ध्रुव { प्रत्यागम m_दत्त.m_hasOrigParamName; }
    शून्य hasOrigParamName(bool flag) { m_दत्त.m_hasOrigParamName = flag; }
    V3_सङ्ख्या_दत्त_Type दत्त_Type() ध्रुव VL_MT_SAFE { प्रत्यागम m_दत्त.type(); }
    शून्य दत्त_Type(V3_सङ्ख्या_दत्त_Type newType) {
        यदि (दत्त_Type() == newType) प्रत्यागम;
        UASSERT(newType != V3_सङ्ख्या_दत्त_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        विकल्प (newType) {
        पक्ष V3_सङ्ख्या_दत्त_Type::STRING: m_दत्त.समुच्चयः_String(); भङ्ग;
        पक्ष V3_सङ्ख्या_दत्त_Type::DOUBLE: m_दत्त.समुच्चयः_Double(); भङ्ग;
        पक्ष V3_सङ्ख्या_दत्त_Type::LOGIC: m_दत्त.समुच्चयः_Logic(); भङ्ग;
        पक्ष V3_सङ्ख्या_दत्त_Type::UNINITIALIZED: भङ्ग;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() ध्रुव VL_MT_SAFE { प्रत्यागम m_दत्त.m_signed; }
    शून्य isSigned(bool ssigned) { m_दत्त.m_signed = ssigned; }
    bool isDouble() ध्रुव VL_MT_SAFE { प्रत्यागम दत्त_Type() == V3_सङ्ख्या_दत्त_Type::DOUBLE; }
    bool isString() ध्रुव VL_MT_SAFE { प्रत्यागम दत्त_Type() == V3_सङ्ख्या_दत्त_Type::STRING; }
    bool isOpaque() ध्रुव VL_MT_SAFE { प्रत्यागम isDouble() || isString(); }
    bool is_सङ्ख्या() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_दत्त.type() == V3_सङ्ख्या_दत्त_Type::LOGIC
               || m_दत्त.type() == V3_सङ्ख्या_दत्त_Type::DOUBLE;
    }
    bool isNegative() ध्रुव VL_MT_SAFE {
        // Correct number of zero bits/width matters
        प्रत्यागम !isString() && bitIs1(विस्तारः() - 1);
    }
    bool is1_चरणः() ध्रुव VL_MT_SAFE { प्रत्यागम m_दत्त.m_is1_चरणः; }
    bool isNull() ध्रुव VL_MT_SAFE { प्रत्यागम m_दत्त.m_isNull; }
    bool isFourState() ध्रुव VL_MT_SAFE;
    bool hasZ() ध्रुव {
        यदि (isString()) प्रत्यागम false;
        प्रति (पूर्णाङ्क i = 0; i < शब्दाः(); ++i) {
            ध्रुव मानम्_And_X v = m_दत्त.अङ्कः()[i];
            यदि ((~v.m_मानम्) & v.m_मानम्_X) प्रत्यागम true;
        }
        प्रत्यागम false;
    }
    bool isAllZ() ध्रुव VL_MT_SAFE;
    bool isAllX() ध्रुव VL_MT_SAFE;
    bool isEqZero() ध्रुव VL_MT_SAFE;
    bool isNeqZero() ध्रुव;
    bool is_द्वयङ्काः_Zero(पूर्णाङ्क msb, पूर्णाङ्क lsb) ध्रुव;
    bool isBroken(पूर्णाङ्क vwidth) ध्रुव;
    bool isEqOne() ध्रुव;
    bool isEqAllOnes(पूर्णाङ्क optwidth = 0) ध्रुव;
    bool isCaseEq(ध्रुव V3_सङ्ख्या& rhs) ध्रुव;  // operator==
    bool isLtXZ(ध्रुव V3_सङ्ख्या& rhs) ध्रुव;  // operator< with XZ compared
    bool isAnyX() ध्रुव VL_MT_SAFE;
    bool isAnyXZ() ध्रुव;
    bool isAnyZ() ध्रुव VL_MT_SAFE;
    bool isMsbXZ() ध्रुव {
        // Correct number of zero bits/width matters
        प्रत्यागम bitIsXZ(विस्तारः() - 1);
    }
    bool माति_अन्तर्_UInt() ध्रुव VL_MT_SAFE;
    uint32_t गन्तव्यम्_UInt() ध्रुव VL_MT_SAFE;
    int32_t गन्तव्यम्_SInt() ध्रुव VL_MT_SAFE;
    uint64_t गन्तव्यम्_UQuad() ध्रुव VL_MT_SAFE;
    int64_t गन्तव्यम्_SQuad() ध्रुव VL_MT_SAFE;
    string गन्तव्यम्_String() ध्रुव VL_MT_SAFE;
    string गन्तव्यम्_Decimal_S() ध्रुव VL_MT_STABLE;  // return ASCII signed decimal number
    string गन्तव्यम्_Decimal_U() ध्रुव VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string गन्तव्यम्_Real_String() ध्रुव VL_MT_STABLE;
    द्विगुण गन्तव्यम्_Double() ध्रुव VL_MT_SAFE;
    V3Hash गन्तव्यम्_Hash() ध्रुव;
    uint32_t edataWord(पूर्णाङ्क eword) ध्रुव;
    uint8_t दत्त_Byte(पूर्णाङ्क byte) ध्रुव;
    uint32_t गणनम्_द्वयङ्काः(ध्रुव V3_सङ्ख्या& ctrl) ध्रुव;
    uint32_t गणनम्_द्वयङ्काः(ध्रुव V3_सङ्ख्या& ctrl1, ध्रुव V3_सङ्ख्या& ctrl2, ध्रुव V3_सङ्ख्या& ctrl3) ध्रुव;
    uint32_t गणनम्_Ones() ध्रुव;
    uint32_t most_समुच्चयः_Bit_P1() ध्रुव;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t न्यूनतमम्_समुच्चयः_Bit_P1() ध्रुव;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool संक्रियकः<(ध्रुव V3_सङ्ख्या& rhs) ध्रुव { प्रत्यागम isLtXZ(rhs); }

    // STATICS
    स्थिर bool epsilonEqual(द्विगुण a, द्विगुण b);  // True if number within Epsilon of the other
    स्थिर bool epsilonIntegral(द्विगुण a);  // True if number rounds to integer within Epsilon
    स्थिर पूर्णाङ्क log2b(uint32_t अङ्कः);
    स्थिर पूर्णाङ्क log2bQuad(uint64_t अङ्कः);
    स्थिर शून्य selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_सङ्ख्या& op_द्वयङ्काः_Non_XZ(ध्रुव V3_सङ्ख्या& lhs);  // 0/1->1, X/Z->0
    V3_सङ्ख्या& op_द्वयङ्काः_One(ध्रुव V3_सङ्ख्या& lhs);  // 1->1, 0/X/Z->0
    V3_सङ्ख्या& op_द्वयङ्काः_XZ(ध्रुव V3_सङ्ख्या& lhs);  // 0/1->0, X/Z->1
    V3_सङ्ख्या& op_द्वयङ्काः_Z(ध्रुव V3_सङ्ख्या& lhs);  // Z->1, 0/1/X->0
    //
    V3_सङ्ख्या& op_नियोजय(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_नियोजय_Non_XZ(ध्रुव V3_सङ्ख्या& lhs, bool ignoreXZ = true);
    V3_सङ्ख्या& opExtendS(ध्रुव V3_सङ्ख्या& lhs, uint32_t lbits);  // Sign extension
    V3_सङ्ख्या& opExtendXZ(ध्रुव V3_सङ्ख्या& lhs, uint32_t lbits);  // X/Z extension
    V3_सङ्ख्या& opRedOr(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRedAnd(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRedXor(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_गणनम्_द्वयङ्काः(ध्रुव V3_सङ्ख्या& व्यञ्जकम्, ध्रुव V3_सङ्ख्या& ctrl1, ध्रुव V3_सङ्ख्या& ctrl2,
                          ध्रुव V3_सङ्ख्या& ctrl3);
    V3_सङ्ख्या& op_गणनम्_Ones(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opIsUnknown(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opOneHot(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opOneHot0(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opCLog2(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_Most_समुच्चयः_Bit_P1(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opClean(ध्रुव V3_सङ्ख्या& lhs, uint32_t द्वयङ्काः);
    V3_सङ्ख्या& opConcat(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLenN(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRepl(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opRepl(ध्रुव V3_सङ्ख्या& lhs, uint32_t rhsval);
    V3_सङ्ख्या& op_प्रवाहः_L(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opSel(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& msb, ध्रुव V3_सङ्ख्या& lsb);
    V3_सङ्ख्या& opSel(ध्रुव V3_सङ्ख्या& lhs, uint32_t msbval, uint32_t lsbval);
    V3_सङ्ख्या& opSelInto(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& lsb, पूर्णाङ्क विस्तारः);
    V3_सङ्ख्या& opSelInto(ध्रुव V3_सङ्ख्या& lhs, पूर्णाङ्क lsbval, पूर्णाङ्क विस्तारः);
    V3_सङ्ख्या& opCvtPackString(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_गन्तव्यम्_अधस्तनम्_N(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_गन्तव्यम्_ऊर्ध्वस्थम्_N(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opCaseEq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opCaseNeq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opWildEq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opWildNeq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opBufIf1(ध्रुव V3_सङ्ख्या& ens, ध्रुव V3_सङ्ख्या& if1s);
    V3_सङ्ख्या& op_समुच्चयः_Range(uint32_t lsb, uint32_t विस्तारः, वर्ण bit_मानम्);
    // "standard" math
    V3_सङ्ख्या& opNot(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opLogNot(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opLogAnd(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLogOr(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLogEq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLogIf(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opNegate(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_सङ्कलय(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opSub(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opMul(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opMulS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opDiv(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opDivS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opModDiv(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opModDivS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opPow(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, bool lsign = false,
                    bool rsign = false);
    V3_सङ्ख्या& opPowSU(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed lhs, unsigned rhs
    V3_सङ्ख्या& opPowSS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed lhs, signed rhs
    V3_सङ्ख्या& opPowUS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Unsigned lhs, signed rhs
    V3_सङ्ख्या& opAnd(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opXor(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opOr(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& op_सरणम्_R(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& op_सरणम्_RS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_सङ्ख्या& op_सरणम्_L(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    // Comparisons
    V3_सङ्ख्या& opEq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opNeq(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGt(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGtS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opGte(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGteS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opLt(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLtS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed
    V3_सङ्ख्या& opLte(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLteS(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);  // Signed

    // "D" - double (aka real) math
    V3_सङ्ख्या& opIToRD(ध्रुव V3_सङ्ख्या& lhs, bool isSigned = false);
    V3_सङ्ख्या& opISToRD(ध्रुव V3_सङ्ख्या& lhs) { प्रत्यागम opIToRD(lhs, true); }
    V3_सङ्ख्या& opRToIS(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opRToIRoundS(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_Real_गन्तव्यम्_द्वयङ्काः(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_द्वयङ्काः_गन्तव्यम्_Real_D(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opNegateD(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& op_सङ्कलय_D(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opSubD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opMulD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opDivD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opPowD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    // Comparisons
    V3_सङ्ख्या& opEqD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opNeqD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGtD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGteD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLtD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLteD(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);

    // "N" - string operations
    V3_सङ्ख्या& opAtoN(ध्रुव V3_सङ्ख्या& lhs, पूर्णाङ्क आधारः);
    V3_सङ्ख्या& opNToI(ध्रुव V3_सङ्ख्या& lhs);
    V3_सङ्ख्या& opPutcN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, ध्रुव V3_सङ्ख्या& ths);
    V3_सङ्ख्या& opGetcN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opSubstrN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, ध्रुव V3_सङ्ख्या& ths);
    V3_सङ्ख्या& opCompareNN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs, bool ignoreCase);
    V3_सङ्ख्या& opConcatN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opReplN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opReplN(ध्रुव V3_सङ्ख्या& lhs, uint32_t rhsval);
    V3_सङ्ख्या& opEqN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opNeqN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGtN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opGteN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLtN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
    V3_सङ्ख्या& opLteN(ध्रुव V3_सङ्ख्या& lhs, ध्रुव V3_सङ्ख्या& rhs);
};
अन्तःस्थ std::ostream& संक्रियकः<<(std::ostream& os, ध्रुव V3_सङ्ख्या& rhs) VL_MT_SAFE {
    प्रत्यागम os << rhs.ascii();
}

#यदि_अन्तः  // Guard
