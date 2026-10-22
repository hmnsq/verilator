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

#నిర్వచితం_కాకపోతే VERILATOR_V3_సంఖ్య_H
#నిర్వచించు VERILATOR_V3_సంఖ్య_H

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Hash.h"
#అన్తర్భావ "V3StdFuture.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <array>
#అన్తర్భావ <cmath>
#అన్తర్భావ <limits>
#అన్తర్భావ <vector>

class AstEnumDType;
class Ast_Enum_అంశం;
class Ast_కణుపు;
class Ast_కణుపు_DType;
class AstSFormatArg;
class File_పంక్తి;

//============================================================================

class VFormatAttr final {
బహిరంగ:
    గణన en : అక్షరం {
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
    గణన en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    అక్షరం ascii() స్థిరరాశి { తిరిగి m_e; }
    bool isComplex() స్థిరరాశి { తిరిగి m_e == COMPLEX; }
    bool isDouble() స్థిరరాశి { తిరిగి m_e == DOUBLE; }
    bool isEnum() స్థిరరాశి { తిరిగి m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() స్థిరరాశి { తిరిగి m_e == SIGNED; }
    bool isString() స్థిరరాశి { తిరిగి m_e == STRING; }
    bool isUnsigned() స్థిరరాశి { తిరిగి m_e == UNSIGNED; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VFormatAttr& lhs, స్థిరరాశి VFormatAttr& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VFormatAttr& lhs, VFormatAttr::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VFormatAttr::en lhs, స్థిరరాశి VFormatAttr& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VFormatAttr& rhs) {
    తిరిగి os << rhs.ascii();
}

//============================================================================

class V3_సంఖ్య_దత్తాంశం final {
బహిరంగ:
    // TYPES
    నిర్మాణం విలువ_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_విలువ;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_విలువ_X;
        bool క్రియాకారకం==(స్థిరరాశి విలువ_And_X& other) స్థిరరాశి VL_MT_సురక్షిత {
            తిరిగి m_విలువ == other.m_విలువ && m_విలువ_X == other.m_విలువ_X;
        }
    };

    గణన class V3_సంఖ్య_దత్తాంశం_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    మిత్ర std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి V3_సంఖ్య_దత్తాంశం_Type& rhs) VL_MT_సురక్షిత {
        ఎంపిక (rhs) {
        స్థితి V3_సంఖ్య_దత్తాంశం_Type::UNINITIALIZED: తిరిగి os << "UNINITIALIZED";
        స్థితి V3_సంఖ్య_దత్తాంశం_Type::LOGIC: తిరిగి os << "LOGIC";
        స్థితి V3_సంఖ్య_దత్తాంశం_Type::DOUBLE: తిరిగి os << "DOUBLE";
        స్థితి V3_సంఖ్య_దత్తాంశం_Type::STRING: తిరిగి os << "STRING";
        }
        తిరిగి os;
    }

అంతరంగ:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    స్థిర constexpr పూర్ణాంకం INLINE_పదాలు = std::max(
        static_cast<కొలత_t>(2), std::max(పరిమాణం(std::string) / పరిమాణం(విలువ_And_X),
                                         పరిమాణం(std::సదిశ<విలువ_And_X>) / పరిమాణం(విలువ_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    స్థిర constexpr పూర్ణాంకం MAX_INLINE_వెడల్పు = INLINE_పదాలు * పరిమాణం(విలువ_And_X) / 2 * 8;

    // MEMBERS
    సంఘం {
        std::array<విలువ_And_X, INLINE_పదాలు> m_inline_సంఖ్య;
        std::సదిశ<విలువ_And_X> m_dynamic_సంఖ్య;
        std::string m_string;
    };

    పూర్ణాంకం m_వెడల్పు = 0;  // Width (in bits) as specified/calculated
    V3_సంఖ్య_దత్తాంశం_Type m_type;

బహిరంగ:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1_అడుగు : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_సంఖ్య_దత్తాంశం()
        : m_type{V3_సంఖ్య_దత్తాంశం_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1_అడుగు{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_సంఖ్య_దత్తాంశం() { destroy_Stored_విలువ(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_సంఖ్య_దత్తాంశం(స్థిరరాశి V3_సంఖ్య_దత్తాంశం& other)
        : m_వెడల్పు{other.m_వెడల్పు}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_అడుగు{other.m_is1_అడుగు}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ఒకవేళ (other.is_Inline_సంఖ్య()) {
            init_Inline_సంఖ్య(other.m_inline_సంఖ్య);
        } లేకపోతే ఒకవేళ (other.is_Dynamic_సంఖ్య()) {
            init_Dynamic_సంఖ్య(other.m_dynamic_సంఖ్య);
        } లేకపోతే ఒకవేళ (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_సంఖ్య_దత్తాంశం& క్రియాకారకం=(స్థిరరాశి V3_సంఖ్య_దత్తాంశం& other) {
        ఒకవేళ (other.is_Inline_సంఖ్య()) {
            destroy_Stored_విలువ();
            init_Inline_సంఖ్య(other.m_inline_సంఖ్య);
        } లేకపోతే ఒకవేళ (other.is_Dynamic_సంఖ్య()) {
            reinit_With_Or_కేటాయించు_Dynamic_సంఖ్య(other.m_dynamic_సంఖ్య);
        } లేకపోతే ఒకవేళ (other.isString()) {
            reinit_With_Or_కేటాయించు_String(other.m_string);
        } లేకపోతే {
            destroy_Stored_విలువ();
        }
        m_వెడల్పు = other.m_వెడల్పు;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_అడుగు = other.m_is1_అడుగు;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        తిరిగి *this;
    }

    V3_సంఖ్య_దత్తాంశం(V3_సంఖ్య_దత్తాంశం&& other)
        : m_వెడల్పు{other.m_వెడల్పు}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1_అడుగు{other.m_is1_అడుగు}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        ఒకవేళ (other.is_Inline_సంఖ్య()) {
            init_Inline_సంఖ్య(other.m_inline_సంఖ్య);
        } లేకపోతే ఒకవేళ (other.is_Dynamic_సంఖ్య()) {
            init_Dynamic_సంఖ్య(std::move(other.m_dynamic_సంఖ్య));
        } లేకపోతే ఒకవేళ (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_సంఖ్య_దత్తాంశం_Type::UNINITIALIZED;
    }

    V3_సంఖ్య_దత్తాంశం& క్రియాకారకం=(V3_సంఖ్య_దత్తాంశం&& other) {
        ఒకవేళ (other.is_Inline_సంఖ్య()) {
            destroy_Stored_విలువ();
            init_Inline_సంఖ్య(other.m_inline_సంఖ్య);
        } లేకపోతే ఒకవేళ (other.is_Dynamic_సంఖ్య()) {
            reinit_With_Or_కేటాయించు_Dynamic_సంఖ్య(std::move(other.m_dynamic_సంఖ్య));
        } లేకపోతే ఒకవేళ (other.isString()) {
            reinit_With_Or_కేటాయించు_String(std::move(other.m_string));
        } లేకపోతే {
            destroy_Stored_విలువ();
        }
        m_వెడల్పు = other.m_వెడల్పు;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1_అడుగు = other.m_is1_అడుగు;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_సంఖ్య_దత్తాంశం_Type::UNINITIALIZED;
        తిరిగి *this;
    }

    // ACCESSORS
    విలువ_And_X* అంకె() {
        UASSERT(is_సంఖ్య(), "`num` member accessed when data type is " << m_type);
        తిరిగి is_Inline_సంఖ్య() ? m_inline_సంఖ్య.దత్తాంశం() : m_dynamic_సంఖ్య.దత్తాంశం();
    }
    స్థిరరాశి విలువ_And_X* అంకె() స్థిరరాశి VL_MT_సురక్షిత {
        UASSERT(is_సంఖ్య(), "`num` member accessed when data type is " << m_type);
        తిరిగి is_Inline_సంఖ్య() ? m_inline_సంఖ్య.దత్తాంశం() : m_dynamic_సంఖ్య.దత్తాంశం();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        తిరిగి m_string;
    }
    స్థిరరాశి std::string& str() స్థిరరాశి VL_MT_సురక్షిత {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        తిరిగి m_string;
    }

    పూర్ణాంకం వెడల్పు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_వెడల్పు; }
    V3_సంఖ్య_దత్తాంశం_Type type() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_type; }

    // METHODS
    శూన్య resize(పూర్ణాంకం బిట్లు_లెక్క) {
        ఒకవేళ (m_వెడల్పు == బిట్లు_లెక్క) తిరిగి;
        ఒకవేళ (బిట్లు_గమ్యం_పదాలు(m_వెడల్పు) == బిట్లు_గమ్యం_పదాలు(బిట్లు_లెక్క)) {
            m_వెడల్పు = బిట్లు_లెక్క;
            తిరిగి;
        }
        ఒకవేళ (is_Dynamic_సంఖ్య()) {
            ఒకవేళ (బిట్లు_లెక్క > MAX_INLINE_వెడల్పు) {
                m_dynamic_సంఖ్య.resize(బిట్లు_గమ్యం_పదాలు(బిట్లు_లెక్క));
            } లేకపోతే {
                స్థిరరాశి స్వయంచాలక dynamic_బిట్లు = std::move(m_dynamic_సంఖ్య);
                destroy_Dynamic_సంఖ్య();
                init_Inline_సంఖ్య();
                std::జ్ఞాపకం_నకలు(m_inline_సంఖ్య.దత్తాంశం(), dynamic_బిట్లు.దత్తాంశం(), పరిమాణం(m_inline_సంఖ్య));
            }
        } లేకపోతే ఒకవేళ (is_Inline_సంఖ్య()) {
            ఒకవేళ (బిట్లు_లెక్క > MAX_INLINE_వెడల్పు) {
                స్థిరరాశి స్వయంచాలక బిట్లు = m_inline_సంఖ్య;
                init_Dynamic_సంఖ్య(బిట్లు_గమ్యం_పదాలు(బిట్లు_లెక్క));
                std::జ్ఞాపకం_నకలు(m_dynamic_సంఖ్య.దత్తాంశం(), బిట్లు.దత్తాంశం(), పరిమాణం(బిట్లు));
            }
        }
        m_వెడల్పు = బిట్లు_లెక్క;
    }

    శూన్య సమితి_String() {
        // If there has been a string already it is kept intact.
        ఒకవేళ (isString()) తిరిగి;
        ఒకవేళ (is_Dynamic_సంఖ్య()) destroy_Dynamic_సంఖ్య();
        initString();
        m_type = V3_సంఖ్య_దత్తాంశం_Type::STRING;
    }
    శూన్య సమితి_String(std::string&& s) {
        reinit_With_Or_కేటాయించు_String(std::move(s));
        m_type = V3_సంఖ్య_దత్తాంశం_Type::STRING;
    }
    శూన్య సమితి_String(స్థిరరాశి std::string& s) {
        reinit_With_Or_కేటాయించు_String(s);
        m_type = V3_సంఖ్య_దత్తాంశం_Type::STRING;
    }

    శూన్య సమితి_Double() {
        destroy_Stored_విలువ();
        ఒకవేళ (!is_Inline_సంఖ్య()) init_Inline_సంఖ్య();
        m_type = V3_సంఖ్య_దత్తాంశం_Type::DOUBLE;
        resize(64);
    }

    శూన్య సమితి_Logic() {
        ఒకవేళ (isString()) destroyString();
        ఒకవేళ (!is_సంఖ్య()) {
            ఒకవేళ (m_వెడల్పు <= MAX_INLINE_వెడల్పు) {
                init_Inline_సంఖ్య();
            } లేకపోతే {
                init_Dynamic_సంఖ్య(బిట్లు_గమ్యం_పదాలు(m_వెడల్పు));
            }
        }
        m_type = V3_సంఖ్య_దత్తాంశం_Type::LOGIC;
        resize(m_వెడల్పు);
    }

అంతరంగ:
    స్థిర constexpr పూర్ణాంకం బిట్లు_గమ్యం_పదాలు(పూర్ణాంకం బిట్లు_లెక్క) VL_శుద్ధ { తిరిగి (బిట్లు_లెక్క + 31) / 32; }

    bool is_సంఖ్య() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_type == V3_సంఖ్య_దత్తాంశం_Type::DOUBLE || m_type == V3_సంఖ్య_దత్తాంశం_Type::LOGIC;
    }
    bool is_Inline_సంఖ్య() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (m_వెడల్పు <= MAX_INLINE_వెడల్పు)
               && (m_type == V3_సంఖ్య_దత్తాంశం_Type::DOUBLE || m_type == V3_సంఖ్య_దత్తాంశం_Type::LOGIC);
    }
    bool is_Dynamic_సంఖ్య() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (m_వెడల్పు > MAX_INLINE_వెడల్పు) && (m_type == V3_సంఖ్య_దత్తాంశం_Type::LOGIC);
    }
    bool isString() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_type == V3_సంఖ్య_దత్తాంశం_Type::STRING; }

    మూస <typename... వాదనలు_2>
    శూన్య init_Inline_సంఖ్య(వాదనలు_2&&... వాదనలు) {
        new (&m_inline_సంఖ్య) std::array<విలువ_And_X, INLINE_పదాలు>(std::forward<వాదనలు_2>(వాదనలు)...);
    }
    మూస <typename... వాదనలు_2>
    శూన్య init_Dynamic_సంఖ్య(వాదనలు_2&&... వాదనలు) {
        new (&m_dynamic_సంఖ్య) std::సదిశ<విలువ_And_X>(std::forward<వాదనలు_2>(వాదనలు)...);
    }
    మూస <typename... వాదనలు_2>
    శూన్య initString(వాదనలు_2&&... వాదనలు) {
        new (&m_string) std::string(std::forward<వాదనలు_2>(వాదనలు)...);
    }

    string formatDecimal();

    శూన్య destroy_Dynamic_సంఖ్య() { m_dynamic_సంఖ్య.~సదిశ(); }
    శూన్య destroyString() { m_string.~string(); }
    శూన్య destroy_Stored_విలువ() {
        ఒకవేళ (isString())
            destroyString();
        లేకపోతే ఒకవేళ (is_Dynamic_సంఖ్య())
            destroy_Dynamic_సంఖ్య();
    }

    మూస <typename T>
    శూన్య reinit_With_Or_కేటాయించు_Dynamic_సంఖ్య(T&& s) {
        ఒకవేళ (is_Dynamic_సంఖ్య()) {
            m_dynamic_సంఖ్య = std::forward<T>(s);
            తిరిగి;
        }
        ఒకవేళ (isString()) destroyString();
        init_Dynamic_సంఖ్య(std::forward<T>(s));
    }
    మూస <typename T>
    శూన్య reinit_With_Or_కేటాయించు_String(T&& s) {
        ఒకవేళ (isString()) {
            m_string = std::forward<T>(s);
            తిరిగి;
        }
        ఒకవేళ (is_Dynamic_సంఖ్య()) destroy_Dynamic_సంఖ్య();
        initString(std::forward<T>(s));
    }
};

class V3_సంఖ్య final {
    // TYPES
    using విలువ_And_X = V3_సంఖ్య_దత్తాంశం::విలువ_And_X;
    using V3_సంఖ్య_దత్తాంశం_Type = V3_సంఖ్య_దత్తాంశం::V3_సంఖ్య_దత్తాంశం_Type;

    // MEMBERS
    V3_సంఖ్య_దత్తాంశం m_దత్తాంశం;
    Ast_కణుపు* m_nodep = nullptr;  // Parent node - for error reporting only
    File_పంక్తి* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    శూన్య సమితి_Null() {
        m_దత్తాంశం.సమితి_Logic();
        m_దత్తాంశం.m_isNull = true;
        m_దత్తాంశం.m_autoExtend = true;
    }
    V3_సంఖ్య& సమితి_Single_బిట్లు(అక్షరం విలువ);
    శూన్య fill_బిట్లు(పూర్ణాంకం destLsb, పూర్ణాంకం వెడల్పు, అక్షరం విలువ);
    శూన్య copy_బిట్లు(పూర్ణాంకం destLsb, స్థిరరాశి V3_సంఖ్య& ఆధారం, పూర్ణాంకం ఆధారం_Lsb, పూర్ణాంకం వెడల్పు);
    V3_సంఖ్య& సమితి_String(స్థిరరాశి string& str) {
        m_దత్తాంశం.సమితి_String(str);
        తిరిగి *this;
    }
    శూన్య opCleanThis(bool warnOnTruncation = false);

బహిరంగ:
    శూన్య nodep(Ast_కణుపు* nodep) VL_MT_STABLE;
    Ast_కణుపు* nodep() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_nodep; }  // For debug only
    File_పంక్తి* fileline() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_fileline; }
    V3_సంఖ్య& సమితి_Zero();
    V3_సంఖ్య& సమితి_Quad(uint64_t విలువ);
    V3_సంఖ్య& సమితి_Long(uint32_t విలువ);
    V3_సంఖ్య& సమితి_Long_S(int32_t విలువ);
    V3_సంఖ్య& సమితి_Double(ద్విగుణ విలువ);
    శూన్య సమితి_బిట్టు_X0(పూర్ణాంకం బిట్టు_2);
    శూన్య సమితి_బిట్టు(పూర్ణాంకం బిట్టు_2, అక్షరం విలువ) {  // Note: must be initialized as number and pre-zeroed!
        ఒకవేళ (బిట్టు_2 >= m_దత్తాంశం.వెడల్పు()) తిరిగి;
        స్థిరరాశి uint32_t ముసుగు = (1UL << (బిట్టు_2 & 31));
        విలువ_And_X& v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        ఒకవేళ (విలువ == '0' || విలువ == 0) {
            v.m_విలువ &= ~ముసుగు;
            v.m_విలువ_X &= ~ముసుగు;
        } లేకపోతే ఒకవేళ (విలువ == '1' || విలువ == 1) {
            v.m_విలువ |= ముసుగు;
            v.m_విలువ_X &= ~ముసుగు;
        } లేకపోతే ఒకవేళ (విలువ == 'z' || విలువ == 2) {
            v.m_విలువ &= ~ముసుగు;
            v.m_విలువ_X |= ముసుగు;
        } లేకపోతే {  // X
            v.m_విలువ |= ముసుగు;
            v.m_విలువ_X |= ముసుగు;
        }
    }

అంతరంగ:
    అక్షరం బిట్టు_Is(పూర్ణాంకం బిట్టు_2) స్థిరరాశి {
        ఒకవేళ (బిట్టు_2 >= m_దత్తాంశం.వెడల్పు() || బిట్టు_2 < 0) {
            // We never sign extend
            తిరిగి '0';
        }
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        తిరిగి ("01zx"[(((v.m_విలువ & (1UL << (బిట్టు_2 & 31))) ? 1 : 0)
                        | ((v.m_విలువ_X & (1UL << (బిట్టు_2 & 31))) ? 2 : 0))]);
    }
    అక్షరం బిట్టు_Is_Extend(పూర్ణాంకం బిట్టు_2, పూర్ణాంకం lbits) స్థిరరాశి {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        ఒకవేళ (బిట్టు_2 < 0) తిరిగి '0';
        UASSERT(lbits <= m_దత్తాంశం.వెడల్పు(), "Extend of wrong size");
        ఒకవేళ (బిట్టు_2 >= lbits) {
            బిట్టు_2 = lbits ? lbits - 1 : 0;
            స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
            // We do sign extend
            తిరిగి ("01zx"[(((v.m_విలువ & (1UL << (బిట్టు_2 & 31))) ? 1 : 0)
                            | ((v.m_విలువ_X & (1UL << (బిట్టు_2 & 31))) ? 2 : 0))]);
        }
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        తిరిగి ("01zx"[(((v.m_విలువ & (1UL << (బిట్టు_2 & 31))) ? 1 : 0)
                        | ((v.m_విలువ_X & (1UL << (బిట్టు_2 & 31))) ? 2 : 0))]);
    }

బహిరంగ:
    bool బిట్టు_Is0(పూర్ణాంకం బిట్టు_2) స్థిరరాశి VL_MT_సురక్షిత {
        ఒకవేళ (!is_సంఖ్య()) తిరిగి false;
        ఒకవేళ (బిట్టు_2 < 0) తిరిగి false;
        ఒకవేళ (బిట్టు_2 >= m_దత్తాంశం.వెడల్పు()) తిరిగి !బిట్టు_Is_XZ(m_దత్తాంశం.వెడల్పు() - 1);
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        తిరిగి ((v.m_విలువ | v.m_విలువ_X) & (1UL << (బిట్టు_2 & 31))) == 0;
    }
    bool బిట్టు_Is1(పూర్ణాంకం బిట్టు_2) స్థిరరాశి VL_MT_సురక్షిత {
        ఒకవేళ (!is_సంఖ్య()) తిరిగి false;
        ఒకవేళ (బిట్టు_2 < 0) తిరిగి false;
        ఒకవేళ (బిట్టు_2 >= m_దత్తాంశం.వెడల్పు()) తిరిగి false;
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        తిరిగి ((v.m_విలువ & (1UL << (బిట్టు_2 & 31))) && !(v.m_విలువ_X & (1UL << (బిట్టు_2 & 31))));
    }
    bool బిట్టు_Is1_Extend(పూర్ణాంకం బిట్టు_2) స్థిరరాశి {
        // Correct number of zero bits/width matters
        ఒకవేళ (!is_సంఖ్య()) తిరిగి false;
        ఒకవేళ (బిట్టు_2 < 0) తిరిగి false;
        ఒకవేళ (బిట్టు_2 >= m_దత్తాంశం.వెడల్పు()) తిరిగి బిట్టు_Is1_Extend(m_దత్తాంశం.వెడల్పు() - 1);
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        తిరిగి ((v.m_విలువ & (1UL << (బిట్టు_2 & 31))) && !(v.m_విలువ_X & (1UL << (బిట్టు_2 & 31))));
    }
    bool బిట్టు_Is_X(పూర్ణాంకం బిట్టు_2) స్థిరరాశి VL_MT_సురక్షిత {
        ఒకవేళ (!is_సంఖ్య()) తిరిగి false;
        ఒకవేళ (బిట్టు_2 < 0) తిరిగి false;
        ఒకవేళ (బిట్టు_2 >= m_దత్తాంశం.వెడల్పు()) తిరిగి బిట్టు_Is_Z(m_దత్తాంశం.వెడల్పు() - 1);
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        తిరిగి ((v.m_విలువ & (1UL << (బిట్టు_2 & 31))) && (v.m_విలువ_X & (1UL << (బిట్టు_2 & 31))));
    }
    bool బిట్టు_Is_XZ(పూర్ణాంకం బిట్టు_2) స్థిరరాశి VL_MT_సురక్షిత {
        ఒకవేళ (!is_సంఖ్య()) తిరిగి false;
        ఒకవేళ (బిట్టు_2 < 0) తిరిగి false;
        ఒకవేళ (బిట్టు_2 >= m_దత్తాంశం.వెడల్పు()) తిరిగి బిట్టు_Is_XZ(m_దత్తాంశం.వెడల్పు() - 1);
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        తిరిగి ((v.m_విలువ_X & (1UL << (బిట్టు_2 & 31))));
    }
    bool బిట్టు_Is_Z(పూర్ణాంకం బిట్టు_2) స్థిరరాశి VL_MT_సురక్షిత {
        ఒకవేళ (!is_సంఖ్య()) తిరిగి false;
        ఒకవేళ (బిట్టు_2 < 0) తిరిగి false;
        ఒకవేళ (బిట్టు_2 >= m_దత్తాంశం.వెడల్పు()) తిరిగి బిట్టు_Is_Z(m_దత్తాంశం.వెడల్పు() - 1);
        స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[బిట్టు_2 / 32];
        తిరిగి ((~v.m_విలువ & (1UL << (బిట్టు_2 & 31))) && (v.m_విలువ_X & (1UL << (బిట్టు_2 & 31))));
    }

అంతరంగ:
    uint32_t బిట్లు_విలువ(పూర్ణాంకం lsb, పూర్ణాంకం nbits) స్థిరరాశి VL_MT_సురక్షిత {
        uint32_t v = 0;
        కొరకు (పూర్ణాంకం bitn = 0; bitn < nbits; bitn++) v |= (బిట్టు_Is1(lsb + bitn) << bitn);
        తిరిగి v;
    }

    పూర్ణాంకం లెక్క_X(పూర్ణాంకం lsb, పూర్ణాంకం nbits) స్థిరరాశి VL_MT_సురక్షిత;
    పూర్ణాంకం లెక్క_Z(పూర్ణాంకం lsb, పూర్ణాంకం nbits) స్థిరరాశి VL_MT_సురక్షిత;

    పూర్ణాంకం పదాలు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి ((వెడల్పు() + 31) / 32); }
    uint32_t hi_మాట_ముసుగు() స్థిరరాశి VL_MT_సురక్షిత {
        // Correct number of zero bits/width matters
        తిరిగి VL_ముసుగు_I(వెడల్పు());
    }

    V3_సంఖ్య& opModDivGuts(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, bool is_modulus);

బహిరంగ:
    // CONSTRUCTORS
    explicit V3_సంఖ్య(Ast_కణుపు* nodep) { init(nodep, 1); }
    V3_సంఖ్య(Ast_కణుపు* nodep, పూర్ణాంకం వెడల్పు) {  // 0=unsized
        init(nodep, వెడల్పు, వెడల్పు > 0);
    }
    // Construct with value, changing to new width
    V3_సంఖ్య(Ast_కణుపు* nodep, పూర్ణాంకం వెడల్పు, స్థిరరాశి V3_సంఖ్య& విలువ) {
        init(nodep, వెడల్పు, వెడల్పు > 0);
        op_కేటాయించు(విలువ);
    }
    V3_సంఖ్య(Ast_కణుపు* nodep, పూర్ణాంకం వెడల్పు, uint32_t విలువ, bool sized = true) {
        init(nodep, వెడల్పు, sized);
        m_దత్తాంశం.అంకె()[0].m_విలువ = విలువ;
        opCleanThis();
    }
    V3_సంఖ్య(File_పంక్తి* flp, పూర్ణాంకం వెడల్పు, uint32_t విలువ, bool sized = true) {
        init(nullptr, వెడల్పు, sized);
        m_fileline = flp;
        m_దత్తాంశం.అంకె()[0].m_విలువ = విలువ;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_సంఖ్య_Literal {};  // For creator type-overload selection
    V3_సంఖ్య(Ast_కణుపు* nodep, Verilog_సంఖ్య_Literal, స్థిరరాశి అక్షరం* sourcep) { create(nodep, sourcep); }
    V3_సంఖ్య(File_పంక్తి* flp, Verilog_సంఖ్య_Literal, స్థిరరాశి అక్షరం* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_సంఖ్య(Ast_కణుపు* nodep, VerilogStringLiteral, స్థిరరాశి string& str);
    class Double {};
    V3_సంఖ్య(Ast_కణుపు* nodep, Double, ద్విగుణ విలువ) {
        init(nodep, 64);
        సమితి_Double(విలువ);
    }
    class String {};
    V3_సంఖ్య(Ast_కణుపు* nodep, String, స్థిరరాశి string& విలువ) {
        init(nodep);
        సమితి_String(విలువ);
        m_దత్తాంశం.m_fromString = true;
    }
    class One_అడుగు {};
    V3_సంఖ్య(Ast_కణుపు* nodep, One_అడుగు) {
        init(nodep, 64);
        m_దత్తాంశం.m_is1_అడుగు = true;
    }
    class Null {};
    V3_సంఖ్య(Ast_కణుపు* nodep, Null) {
        init(nodep);
        సమితి_Null();
    }
    explicit V3_సంఖ్య(స్థిరరాశి V3_సంఖ్య* nump, పూర్ణాంకం వెడల్పు = 1) {
        init(nullptr, వెడల్పు);
        m_fileline = nump->fileline();
    }
    V3_సంఖ్య(స్థిరరాశి V3_సంఖ్య* nump, పూర్ణాంకం వెడల్పు, uint32_t విలువ) {
        init(nullptr, వెడల్పు);
        m_దత్తాంశం.అంకె()[0].m_విలువ = విలువ;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_సంఖ్య(Ast_కణుపు* nodep, స్థిరరాశి Ast_కణుపు_DType* nodedtypep);

    V3_సంఖ్య(స్థిరరాశి V3_సంఖ్య& other) = అప్రమేయ;
    V3_సంఖ్య& క్రియాకారకం=(స్థిరరాశి V3_సంఖ్య& other) = అప్రమేయ;

    V3_సంఖ్య(V3_సంఖ్య&& other) = అప్రమేయ;
    V3_సంఖ్య& క్రియాకారకం=(V3_సంఖ్య&& other) = అప్రమేయ;

    ~V3_సంఖ్య() = అప్రమేయ;

అంతరంగ:
    శూన్య selfTestThis();
    శూన్య create(Ast_కణుపు* nodep, స్థిరరాశి అక్షరం* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    శూన్య create(File_పంక్తి* flp, స్థిరరాశి అక్షరం* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    శూన్య create(స్థిరరాశి అక్షరం* sourcep);
    శూన్య init(Ast_కణుపు* nodep, పూర్ణాంకం swidth = -1, bool sized = true) {
        this->nodep(nodep);
        ఒకవేళ (swidth >= 0) {
            ఒకవేళ (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_దత్తాంశం.సమితి_Logic();
            m_దత్తాంశం.resize(swidth);
            m_దత్తాంశం.m_sized = sized;
            కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) m_దత్తాంశం.అంకె()[i] = {0, 0};
        } లేకపోతే {
            m_దత్తాంశం.resize(1);
            m_దత్తాంశం.m_sized = false;
        }
    }
    స్థిర string displayPad(కొలత_t fmtsize, అక్షరం pad, bool ఎడమ_2, స్థిరరాశి string& లోపల) VL_శుద్ధ;
    string displayed(స్థిరరాశి string& vformat) స్థిరరాశి VL_MT_STABLE {
        తిరిగి displayed(m_fileline, vformat);
    }
    శూన్య warnTooMany(స్థిరరాశి string& విలువ);

బహిరంగ:
    శూన్య v3error_చివర(స్థిరరాశి std::ostringstream& sstr) స్థిరరాశి VL_RELEASE(V3Error::s().m_mutex);
    శూన్య v3error_చివర_Fatal(స్థిరరాశి std::ostringstream& sstr) స్థిరరాశి VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    శూన్య వెడల్పు(పూర్ణాంకం వెడల్పు, bool sized = true) {
        m_దత్తాంశం.m_sized = sized;
        m_దత్తాంశం.resize(వెడల్పు);
    }

    // SETTERS
    V3_సంఖ్య& సమితి_All_బిట్లు_XRemoved();
    V3_సంఖ్య& సమితి_All_బిట్లు_X();
    V3_సంఖ్య& సమితి_All_బిట్లు_Z();
    V3_సంఖ్య& సమితి_All_Bits0();
    V3_సంఖ్య& సమితి_All_Bits1();
    V3_సంఖ్య& సమితి_Value1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_సంఖ్య& సమితి_ముసుగు(పూర్ణాంకం nbits, పూర్ణాంకం lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) స్థిరరాశి VL_MT_STABLE;
    string displayed(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string& vformat,
                     స్థిరరాశి VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) స్థిరరాశి VL_MT_STABLE;
    string displayed(File_పంక్తి* fl, స్థిరరాశి string& vformat,
                     స్థిరరాశి VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) స్థిరరాశి VL_MT_STABLE;
    string displayedEnum(స్థిరరాశి AstSFormatArg* argp, స్థిరరాశి string& vformat) స్థిరరాశి VL_MT_STABLE;
    string displayedEnumName(స్థిరరాశి AstEnumDType* dtypep) స్థిరరాశి VL_MT_STABLE;
    స్థిర string displayedEnumName(స్థిరరాశి Ast_Enum_అంశం* itemp);
    స్థిర bool displayedFmtHasArg(అక్షరం format, bool is_పరిశీలన);
    string emitC() స్థిరరాశి VL_MT_STABLE;
    పూర్ణాంకం వెడల్పు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_దత్తాంశం.వెడల్పు(); }
    పూర్ణాంకం వెడల్పు_గమ్యం_Fit() స్థిరరాశి;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_దత్తాంశం.m_sized; }
    bool autoExtend() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_దత్తాంశం.m_autoExtend; }
    bool isFromString() స్థిరరాశి { తిరిగి m_దత్తాంశం.m_fromString; }
    bool hasOrigParamName() స్థిరరాశి { తిరిగి m_దత్తాంశం.m_hasOrigParamName; }
    శూన్య hasOrigParamName(bool flag) { m_దత్తాంశం.m_hasOrigParamName = flag; }
    V3_సంఖ్య_దత్తాంశం_Type దత్తాంశం_Type() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_దత్తాంశం.type(); }
    శూన్య దత్తాంశం_Type(V3_సంఖ్య_దత్తాంశం_Type newType) {
        ఒకవేళ (దత్తాంశం_Type() == newType) తిరిగి;
        UASSERT(newType != V3_సంఖ్య_దత్తాంశం_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        ఎంపిక (newType) {
        స్థితి V3_సంఖ్య_దత్తాంశం_Type::STRING: m_దత్తాంశం.సమితి_String(); ఆపు;
        స్థితి V3_సంఖ్య_దత్తాంశం_Type::DOUBLE: m_దత్తాంశం.సమితి_Double(); ఆపు;
        స్థితి V3_సంఖ్య_దత్తాంశం_Type::LOGIC: m_దత్తాంశం.సమితి_Logic(); ఆపు;
        స్థితి V3_సంఖ్య_దత్తాంశం_Type::UNINITIALIZED: ఆపు;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_దత్తాంశం.m_signed; }
    శూన్య isSigned(bool ssigned) { m_దత్తాంశం.m_signed = ssigned; }
    bool isDouble() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి దత్తాంశం_Type() == V3_సంఖ్య_దత్తాంశం_Type::DOUBLE; }
    bool isString() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి దత్తాంశం_Type() == V3_సంఖ్య_దత్తాంశం_Type::STRING; }
    bool isOpaque() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి isDouble() || isString(); }
    bool is_సంఖ్య() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_దత్తాంశం.type() == V3_సంఖ్య_దత్తాంశం_Type::LOGIC
               || m_దత్తాంశం.type() == V3_సంఖ్య_దత్తాంశం_Type::DOUBLE;
    }
    bool isNegative() స్థిరరాశి VL_MT_సురక్షిత {
        // Correct number of zero bits/width matters
        తిరిగి !isString() && బిట్టు_Is1(వెడల్పు() - 1);
    }
    bool is1_అడుగు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_దత్తాంశం.m_is1_అడుగు; }
    bool isNull() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_దత్తాంశం.m_isNull; }
    bool isFourState() స్థిరరాశి VL_MT_సురక్షిత;
    bool hasZ() స్థిరరాశి {
        ఒకవేళ (isString()) తిరిగి false;
        కొరకు (పూర్ణాంకం i = 0; i < పదాలు(); ++i) {
            స్థిరరాశి విలువ_And_X v = m_దత్తాంశం.అంకె()[i];
            ఒకవేళ ((~v.m_విలువ) & v.m_విలువ_X) తిరిగి true;
        }
        తిరిగి false;
    }
    bool isAllZ() స్థిరరాశి VL_MT_సురక్షిత;
    bool isAllX() స్థిరరాశి VL_MT_సురక్షిత;
    bool isEqZero() స్థిరరాశి VL_MT_సురక్షిత;
    bool isNeqZero() స్థిరరాశి;
    bool is_బిట్లు_Zero(పూర్ణాంకం msb, పూర్ణాంకం lsb) స్థిరరాశి;
    bool isBroken(పూర్ణాంకం vwidth) స్థిరరాశి;
    bool isEqOne() స్థిరరాశి;
    bool isEqAllOnes(పూర్ణాంకం optwidth = 0) స్థిరరాశి;
    bool isCaseEq(స్థిరరాశి V3_సంఖ్య& rhs) స్థిరరాశి;  // operator==
    bool isLtXZ(స్థిరరాశి V3_సంఖ్య& rhs) స్థిరరాశి;  // operator< with XZ compared
    bool isAnyX() స్థిరరాశి VL_MT_సురక్షిత;
    bool isAnyXZ() స్థిరరాశి;
    bool isAnyZ() స్థిరరాశి VL_MT_సురక్షిత;
    bool isMsbXZ() స్థిరరాశి {
        // Correct number of zero bits/width matters
        తిరిగి బిట్టు_Is_XZ(వెడల్పు() - 1);
    }
    bool సరిపోతుంది_లోపల_UInt() స్థిరరాశి VL_MT_సురక్షిత;
    uint32_t గమ్యం_UInt() స్థిరరాశి VL_MT_సురక్షిత;
    int32_t గమ్యం_SInt() స్థిరరాశి VL_MT_సురక్షిత;
    uint64_t గమ్యం_UQuad() స్థిరరాశి VL_MT_సురక్షిత;
    int64_t గమ్యం_SQuad() స్థిరరాశి VL_MT_సురక్షిత;
    string గమ్యం_String() స్థిరరాశి VL_MT_సురక్షిత;
    string గమ్యం_Decimal_S() స్థిరరాశి VL_MT_STABLE;  // return ASCII signed decimal number
    string గమ్యం_Decimal_U() స్థిరరాశి VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string గమ్యం_Real_String() స్థిరరాశి VL_MT_STABLE;
    ద్విగుణ గమ్యం_Double() స్థిరరాశి VL_MT_సురక్షిత;
    V3Hash గమ్యం_Hash() స్థిరరాశి;
    uint32_t edata_మాట(పూర్ణాంకం eword) స్థిరరాశి;
    uint8_t దత్తాంశం_Byte(పూర్ణాంకం byte) స్థిరరాశి;
    uint32_t లెక్క_బిట్లు(స్థిరరాశి V3_సంఖ్య& ctrl) స్థిరరాశి;
    uint32_t లెక్క_బిట్లు(స్థిరరాశి V3_సంఖ్య& ctrl1, స్థిరరాశి V3_సంఖ్య& ctrl2, స్థిరరాశి V3_సంఖ్య& ctrl3) స్థిరరాశి;
    uint32_t లెక్క_Ones() స్థిరరాశి;
    uint32_t most_సమితి_బిట్టు_P1() స్థిరరాశి;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t కనిష్ఠ_సమితి_బిట్టు_P1() స్థిరరాశి;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool క్రియాకారకం<(స్థిరరాశి V3_సంఖ్య& rhs) స్థిరరాశి { తిరిగి isLtXZ(rhs); }

    // STATICS
    స్థిర bool epsilonEqual(ద్విగుణ a, ద్విగుణ b);  // True if number within Epsilon of the other
    స్థిర bool epsilonIntegral(ద్విగుణ a);  // True if number rounds to integer within Epsilon
    స్థిర పూర్ణాంకం log2b(uint32_t అంకె);
    స్థిర పూర్ణాంకం log2bQuad(uint64_t అంకె);
    స్థిర శూన్య selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_సంఖ్య& op_బిట్లు_Non_XZ(స్థిరరాశి V3_సంఖ్య& lhs);  // 0/1->1, X/Z->0
    V3_సంఖ్య& op_బిట్లు_One(స్థిరరాశి V3_సంఖ్య& lhs);  // 1->1, 0/X/Z->0
    V3_సంఖ్య& op_బిట్లు_XZ(స్థిరరాశి V3_సంఖ్య& lhs);  // 0/1->0, X/Z->1
    V3_సంఖ్య& op_బిట్లు_Z(స్థిరరాశి V3_సంఖ్య& lhs);  // Z->1, 0/1/X->0
    //
    V3_సంఖ్య& op_కేటాయించు(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_కేటాయించు_Non_XZ(స్థిరరాశి V3_సంఖ్య& lhs, bool ignoreXZ = true);
    V3_సంఖ్య& opExtendS(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t lbits);  // Sign extension
    V3_సంఖ్య& opExtendXZ(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t lbits);  // X/Z extension
    V3_సంఖ్య& opRedOr(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opRedAnd(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opRedXor(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_లెక్క_బిట్లు(స్థిరరాశి V3_సంఖ్య& సమాసం, స్థిరరాశి V3_సంఖ్య& ctrl1, స్థిరరాశి V3_సంఖ్య& ctrl2,
                          స్థిరరాశి V3_సంఖ్య& ctrl3);
    V3_సంఖ్య& op_లెక్క_Ones(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opIsUnknown(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opOneHot(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opOneHot0(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opCLog2(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_Most_సమితి_బిట్టు_P1(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opClean(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t బిట్లు);
    V3_సంఖ్య& opConcat(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLenN(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opRepl(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opRepl(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t rhsval);
    V3_సంఖ్య& op_ప్రవాహం_L(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opSel(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& msb, స్థిరరాశి V3_సంఖ్య& lsb);
    V3_సంఖ్య& opSel(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t msbval, uint32_t lsbval);
    V3_సంఖ్య& opSelInto(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& lsb, పూర్ణాంకం వెడల్పు);
    V3_సంఖ్య& opSelInto(స్థిరరాశి V3_సంఖ్య& lhs, పూర్ణాంకం lsbval, పూర్ణాంకం వెడల్పు);
    V3_సంఖ్య& opCvtPackString(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_గమ్యం_క్రింది_N(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_గమ్యం_పై_N(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opCaseEq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opCaseNeq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opWildEq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opWildNeq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opBufIf1(స్థిరరాశి V3_సంఖ్య& ens, స్థిరరాశి V3_సంఖ్య& if1s);
    V3_సంఖ్య& op_సమితి_Range(uint32_t lsb, uint32_t వెడల్పు, అక్షరం బిట్టు_విలువ);
    // "standard" math
    V3_సంఖ్య& opNot(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opLogNot(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opLogAnd(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLogOr(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLogEq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLogIf(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opNegate(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_కూడు(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opSub(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opMul(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opMulS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed
    V3_సంఖ్య& opDiv(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opDivS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed
    V3_సంఖ్య& opModDiv(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opModDivS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed
    V3_సంఖ్య& opPow(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, bool lsign = false,
                    bool rsign = false);
    V3_సంఖ్య& opPowSU(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed lhs, unsigned rhs
    V3_సంఖ్య& opPowSS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed lhs, signed rhs
    V3_సంఖ్య& opPowUS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Unsigned lhs, signed rhs
    V3_సంఖ్య& opAnd(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opXor(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opOr(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& op_జరుపు_R(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& op_జరుపు_RS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_సంఖ్య& op_జరుపు_L(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    // Comparisons
    V3_సంఖ్య& opEq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opNeq(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opGt(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opGtS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed
    V3_సంఖ్య& opGte(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opGteS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed
    V3_సంఖ్య& opLt(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLtS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed
    V3_సంఖ్య& opLte(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLteS(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);  // Signed

    // "D" - double (aka real) math
    V3_సంఖ్య& opIToRD(స్థిరరాశి V3_సంఖ్య& lhs, bool isSigned = false);
    V3_సంఖ్య& opISToRD(స్థిరరాశి V3_సంఖ్య& lhs) { తిరిగి opIToRD(lhs, true); }
    V3_సంఖ్య& opRToIS(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opRToIRoundS(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_Real_గమ్యం_బిట్లు(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_బిట్లు_గమ్యం_Real_D(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opNegateD(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& op_కూడు_D(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opSubD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opMulD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opDivD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opPowD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    // Comparisons
    V3_సంఖ్య& opEqD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opNeqD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opGtD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opGteD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLtD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLteD(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);

    // "N" - string operations
    V3_సంఖ్య& opAtoN(స్థిరరాశి V3_సంఖ్య& lhs, పూర్ణాంకం పునాది);
    V3_సంఖ్య& opNToI(స్థిరరాశి V3_సంఖ్య& lhs);
    V3_సంఖ్య& opPutcN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, స్థిరరాశి V3_సంఖ్య& ths);
    V3_సంఖ్య& opGetcN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opSubstrN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, స్థిరరాశి V3_సంఖ్య& ths);
    V3_సంఖ్య& opCompareNN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs, bool ignoreCase);
    V3_సంఖ్య& opConcatN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opReplN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opReplN(స్థిరరాశి V3_సంఖ్య& lhs, uint32_t rhsval);
    V3_సంఖ్య& opEqN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opNeqN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opGtN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opGteN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLtN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
    V3_సంఖ్య& opLteN(స్థిరరాశి V3_సంఖ్య& lhs, స్థిరరాశి V3_సంఖ్య& rhs);
};
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి V3_సంఖ్య& rhs) VL_MT_సురక్షిత {
    తిరిగి os << rhs.ascii();
}

#అయితే_ముగింపు  // Guard
