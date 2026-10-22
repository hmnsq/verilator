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

#ifndef VERILATOR_V3_സംഖ്യ_H
#define VERILATOR_V3_സംഖ്യ_H

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Hash.h"
#അന്തര്ഭാവ "V3StdFuture.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <array>
#അന്തര്ഭാവ <cmath>
#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <vector>

class AstEnumDType;
class AstEnumItem;
class Ast_കണ്ണി;
class Ast_കണ്ണി_DType;
class AstSFormatArg;
class File_വരി;

//============================================================================

class VFormatAttr final {
പൊതു:
    എണ്ണൽ en : അക്ഷരം {
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
    എണ്ണൽ en m_e;
    VFormatAttr()
        : m_e{UNSIGNED} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFormatAttr(en _e)
        : m_e{_e} {}
    explicit VFormatAttr(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    അക്ഷരം ascii() സ്ഥിരം { തിരികെ m_e; }
    bool isComplex() സ്ഥിരം { തിരികെ m_e == COMPLEX; }
    bool isDouble() സ്ഥിരം { തിരികെ m_e == DOUBLE; }
    bool isEnum() സ്ഥിരം { തിരികെ m_e == ENUM || m_e == ENUM_SIGNED; }
    bool isSigned() സ്ഥിരം { തിരികെ m_e == SIGNED; }
    bool isString() സ്ഥിരം { തിരികെ m_e == STRING; }
    bool isUnsigned() സ്ഥിരം { തിരികെ m_e == UNSIGNED; }
};
constexpr bool operator==(സ്ഥിരം VFormatAttr& lhs, സ്ഥിരം VFormatAttr& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VFormatAttr& lhs, VFormatAttr::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VFormatAttr::en lhs, സ്ഥിരം VFormatAttr& rhs) { തിരികെ lhs == rhs.m_e; }
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VFormatAttr& rhs) {
    തിരികെ os << rhs.ascii();
}

//============================================================================

class V3_സംഖ്യ_Data final {
പൊതു:
    // TYPES
    ഘടന മൂല്യം_And_X final {
        // Value, with bit 0 in bit 0 of this vector (unless X/Z)
        uint32_t m_മൂല്യം;
        // Each bit is true if it's X or Z, 10=z, 11=x
        uint32_t m_മൂല്യം_X;
        bool operator==(സ്ഥിരം മൂല്യം_And_X& other) സ്ഥിരം VL_MT_SAFE {
            തിരികെ m_മൂല്യം == other.m_മൂല്യം && m_മൂല്യം_X == other.m_മൂല്യം_X;
        }
    };

    എണ്ണൽ class V3_സംഖ്യ_Data_Type : uint8_t {
        UNINITIALIZED = 0,
        LOGIC = 1,
        DOUBLE = 2,
        STRING = 3,
    };
    friend std::ostream& operator<<(std::ostream& os, സ്ഥിരം V3_സംഖ്യ_Data_Type& rhs) VL_MT_SAFE {
        തിരഞ്ഞെടുപ്പ് (rhs) {
        അവസ്ഥ V3_സംഖ്യ_Data_Type::UNINITIALIZED: തിരികെ os << "UNINITIALIZED";
        അവസ്ഥ V3_സംഖ്യ_Data_Type::LOGIC: തിരികെ os << "LOGIC";
        അവസ്ഥ V3_സംഖ്യ_Data_Type::DOUBLE: തിരികെ os << "DOUBLE";
        അവസ്ഥ V3_സംഖ്യ_Data_Type::STRING: തിരികെ os << "STRING";
        }
        തിരികെ os;
    }

സ്വകാര്യ:
    // CONSTANTS
    // At least 2 words (64 fourstate bits). 4 words (128 fourstate bits) in most cases,
    // i.e. when std::string has 32 bytes.
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ INLINE_വാക്കുകൾ = std::max(
        static_cast<size_t>(2), std::max(വലുപ്പം(std::string) / വലുപ്പം(മൂല്യം_And_X),
                                         വലുപ്പം(std::vector<മൂല്യം_And_X>) / വലുപ്പം(മൂല്യം_And_X)));
    // When m_width > MAX_INLINE_WIDTH number is stored in m_dynamicNumber.
    // Otherwise number is stored in m_inlineNumber.
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ MAX_INLINE_WIDTH = INLINE_വാക്കുകൾ * വലുപ്പം(മൂല്യം_And_X) / 2 * 8;

    // MEMBERS
    സംഘം {
        std::array<മൂല്യം_And_X, INLINE_വാക്കുകൾ> m_inline_സംഖ്യ;
        std::vector<മൂല്യം_And_X> m_dynamic_സംഖ്യ;
        std::string m_string;
    };

    പൂർണ്ണസംഖ്യ m_width = 0;  // Width (in bits) as specified/calculated
    V3_സംഖ്യ_Data_Type m_type;

പൊതു:
    bool m_sized : 1;  // True if the user specified the width, else we track it.
    bool m_signed : 1;  // True if signed value
    bool m_is1Step : 1;  // True if 1step
    bool m_isNull : 1;  // True if "null" versus normal 0
    bool m_fromString : 1;  // True if from string literal
    bool m_autoExtend : 1;  // True if SystemVerilog extend-to-any-width
    bool m_hasOrigParamName : 1;  // Owning AstConst has originating parameter-name metadata

    // CONSTRUCTORS
    V3_സംഖ്യ_Data()
        : m_type{V3_സംഖ്യ_Data_Type::UNINITIALIZED}
        , m_sized{false}
        , m_signed{false}
        , m_is1Step{false}
        , m_isNull{false}
        , m_fromString{false}
        , m_autoExtend{false}
        , m_hasOrigParamName{false} {}

    ~V3_സംഖ്യ_Data() { destroy_Stored_മൂല്യം(); }

    // m_hasOrigParamName is ownership bookkeeping for an AstConst side-table entry, not part of
    // the numeric value. Copies/moves of V3NumberData must not claim the destination has metadata.
    V3_സംഖ്യ_Data(സ്ഥിരം V3_സംഖ്യ_Data& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        എങ്കിൽ (other.is_Inline_സംഖ്യ()) {
            init_Inline_സംഖ്യ(other.m_inline_സംഖ്യ);
        } അല്ലെങ്കിൽ എങ്കിൽ (other.is_Dynamic_സംഖ്യ()) {
            init_Dynamic_സംഖ്യ(other.m_dynamic_സംഖ്യ);
        } അല്ലെങ്കിൽ എങ്കിൽ (other.isString()) {
            initString(other.m_string);
        }
    }

    V3_സംഖ്യ_Data& operator=(സ്ഥിരം V3_സംഖ്യ_Data& other) {
        എങ്കിൽ (other.is_Inline_സംഖ്യ()) {
            destroy_Stored_മൂല്യം();
            init_Inline_സംഖ്യ(other.m_inline_സംഖ്യ);
        } അല്ലെങ്കിൽ എങ്കിൽ (other.is_Dynamic_സംഖ്യ()) {
            reinit_With_Or_Assign_Dynamic_സംഖ്യ(other.m_dynamic_സംഖ്യ);
        } അല്ലെങ്കിൽ എങ്കിൽ (other.isString()) {
            reinitWithOrAssignString(other.m_string);
        } അല്ലെങ്കിൽ {
            destroy_Stored_മൂല്യം();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        തിരികെ *this;
    }

    V3_സംഖ്യ_Data(V3_സംഖ്യ_Data&& other)
        : m_width{other.m_width}
        , m_type{other.m_type}
        , m_sized{other.m_sized}
        , m_signed{other.m_signed}
        , m_is1Step{other.m_is1Step}
        , m_isNull{other.m_isNull}
        , m_fromString{other.m_fromString}
        , m_autoExtend{other.m_autoExtend}
        , m_hasOrigParamName{false} {
        എങ്കിൽ (other.is_Inline_സംഖ്യ()) {
            init_Inline_സംഖ്യ(other.m_inline_സംഖ്യ);
        } അല്ലെങ്കിൽ എങ്കിൽ (other.is_Dynamic_സംഖ്യ()) {
            init_Dynamic_സംഖ്യ(std::move(other.m_dynamic_സംഖ്യ));
        } അല്ലെങ്കിൽ എങ്കിൽ (other.isString()) {
            initString(std::move(other.m_string));
        }
        other.m_type = V3_സംഖ്യ_Data_Type::UNINITIALIZED;
    }

    V3_സംഖ്യ_Data& operator=(V3_സംഖ്യ_Data&& other) {
        എങ്കിൽ (other.is_Inline_സംഖ്യ()) {
            destroy_Stored_മൂല്യം();
            init_Inline_സംഖ്യ(other.m_inline_സംഖ്യ);
        } അല്ലെങ്കിൽ എങ്കിൽ (other.is_Dynamic_സംഖ്യ()) {
            reinit_With_Or_Assign_Dynamic_സംഖ്യ(std::move(other.m_dynamic_സംഖ്യ));
        } അല്ലെങ്കിൽ എങ്കിൽ (other.isString()) {
            reinitWithOrAssignString(std::move(other.m_string));
        } അല്ലെങ്കിൽ {
            destroy_Stored_മൂല്യം();
        }
        m_width = other.m_width;
        m_type = other.m_type;
        m_sized = other.m_sized;
        m_signed = other.m_signed;
        m_is1Step = other.m_is1Step;
        m_isNull = other.m_isNull;
        m_fromString = other.m_fromString;
        m_autoExtend = other.m_autoExtend;
        other.m_type = V3_സംഖ്യ_Data_Type::UNINITIALIZED;
        തിരികെ *this;
    }

    // ACCESSORS
    മൂല്യം_And_X* അക്കം() {
        UASSERT(is_സംഖ്യ(), "`num` member accessed when data type is " << m_type);
        തിരികെ is_Inline_സംഖ്യ() ? m_inline_സംഖ്യ.data() : m_dynamic_സംഖ്യ.data();
    }
    സ്ഥിരം മൂല്യം_And_X* അക്കം() സ്ഥിരം VL_MT_SAFE {
        UASSERT(is_സംഖ്യ(), "`num` member accessed when data type is " << m_type);
        തിരികെ is_Inline_സംഖ്യ() ? m_inline_സംഖ്യ.data() : m_dynamic_സംഖ്യ.data();
    }
    std::string& str() {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        തിരികെ m_string;
    }
    സ്ഥിരം std::string& str() സ്ഥിരം VL_MT_SAFE {
        UASSERT(isString(), "`str` member accessed when data type is " << m_type);
        തിരികെ m_string;
    }

    പൂർണ്ണസംഖ്യ width() സ്ഥിരം VL_MT_SAFE { തിരികെ m_width; }
    V3_സംഖ്യ_Data_Type type() സ്ഥിരം VL_MT_SAFE { തിരികെ m_type; }

    // METHODS
    ശൂന്യം resize(പൂർണ്ണസംഖ്യ bits_എണ്ണം) {
        എങ്കിൽ (m_width == bits_എണ്ണം) തിരികെ;
        എങ്കിൽ (bits_To_വാക്കുകൾ(m_width) == bits_To_വാക്കുകൾ(bits_എണ്ണം)) {
            m_width = bits_എണ്ണം;
            തിരികെ;
        }
        എങ്കിൽ (is_Dynamic_സംഖ്യ()) {
            എങ്കിൽ (bits_എണ്ണം > MAX_INLINE_WIDTH) {
                m_dynamic_സംഖ്യ.resize(bits_To_വാക്കുകൾ(bits_എണ്ണം));
            } അല്ലെങ്കിൽ {
                സ്ഥിരം auto dynamicBits = std::move(m_dynamic_സംഖ്യ);
                destroy_Dynamic_സംഖ്യ();
                init_Inline_സംഖ്യ();
                std::memcpy(m_inline_സംഖ്യ.data(), dynamicBits.data(), വലുപ്പം(m_inline_സംഖ്യ));
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (is_Inline_സംഖ്യ()) {
            എങ്കിൽ (bits_എണ്ണം > MAX_INLINE_WIDTH) {
                സ്ഥിരം auto bits = m_inline_സംഖ്യ;
                init_Dynamic_സംഖ്യ(bits_To_വാക്കുകൾ(bits_എണ്ണം));
                std::memcpy(m_dynamic_സംഖ്യ.data(), bits.data(), വലുപ്പം(bits));
            }
        }
        m_width = bits_എണ്ണം;
    }

    ശൂന്യം setString() {
        // If there has been a string already it is kept intact.
        എങ്കിൽ (isString()) തിരികെ;
        എങ്കിൽ (is_Dynamic_സംഖ്യ()) destroy_Dynamic_സംഖ്യ();
        initString();
        m_type = V3_സംഖ്യ_Data_Type::STRING;
    }
    ശൂന്യം setString(std::string&& s) {
        reinitWithOrAssignString(std::move(s));
        m_type = V3_സംഖ്യ_Data_Type::STRING;
    }
    ശൂന്യം setString(സ്ഥിരം std::string& s) {
        reinitWithOrAssignString(s);
        m_type = V3_സംഖ്യ_Data_Type::STRING;
    }

    ശൂന്യം setDouble() {
        destroy_Stored_മൂല്യം();
        എങ്കിൽ (!is_Inline_സംഖ്യ()) init_Inline_സംഖ്യ();
        m_type = V3_സംഖ്യ_Data_Type::DOUBLE;
        resize(64);
    }

    ശൂന്യം setLogic() {
        എങ്കിൽ (isString()) destroyString();
        എങ്കിൽ (!is_സംഖ്യ()) {
            എങ്കിൽ (m_width <= MAX_INLINE_WIDTH) {
                init_Inline_സംഖ്യ();
            } അല്ലെങ്കിൽ {
                init_Dynamic_സംഖ്യ(bits_To_വാക്കുകൾ(m_width));
            }
        }
        m_type = V3_സംഖ്യ_Data_Type::LOGIC;
        resize(m_width);
    }

സ്വകാര്യ:
    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ bits_To_വാക്കുകൾ(പൂർണ്ണസംഖ്യ bits_എണ്ണം) VL_PURE { തിരികെ (bits_എണ്ണം + 31) / 32; }

    bool is_സംഖ്യ() സ്ഥിരം VL_MT_SAFE {
        തിരികെ m_type == V3_സംഖ്യ_Data_Type::DOUBLE || m_type == V3_സംഖ്യ_Data_Type::LOGIC;
    }
    bool is_Inline_സംഖ്യ() സ്ഥിരം VL_MT_SAFE {
        തിരികെ (m_width <= MAX_INLINE_WIDTH)
               && (m_type == V3_സംഖ്യ_Data_Type::DOUBLE || m_type == V3_സംഖ്യ_Data_Type::LOGIC);
    }
    bool is_Dynamic_സംഖ്യ() സ്ഥിരം VL_MT_SAFE {
        തിരികെ (m_width > MAX_INLINE_WIDTH) && (m_type == V3_സംഖ്യ_Data_Type::LOGIC);
    }
    bool isString() സ്ഥിരം VL_MT_SAFE { തിരികെ m_type == V3_സംഖ്യ_Data_Type::STRING; }

    template <typename... Args>
    ശൂന്യം init_Inline_സംഖ്യ(Args&&... args) {
        new (&m_inline_സംഖ്യ) std::array<മൂല്യം_And_X, INLINE_വാക്കുകൾ>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    ശൂന്യം init_Dynamic_സംഖ്യ(Args&&... args) {
        new (&m_dynamic_സംഖ്യ) std::vector<മൂല്യം_And_X>(std::forward<Args>(args)...);
    }
    template <typename... Args>
    ശൂന്യം initString(Args&&... args) {
        new (&m_string) std::string(std::forward<Args>(args)...);
    }

    string formatDecimal();

    ശൂന്യം destroy_Dynamic_സംഖ്യ() { m_dynamic_സംഖ്യ.~vector(); }
    ശൂന്യം destroyString() { m_string.~string(); }
    ശൂന്യം destroy_Stored_മൂല്യം() {
        എങ്കിൽ (isString())
            destroyString();
        അല്ലെങ്കിൽ എങ്കിൽ (is_Dynamic_സംഖ്യ())
            destroy_Dynamic_സംഖ്യ();
    }

    template <typename T>
    ശൂന്യം reinit_With_Or_Assign_Dynamic_സംഖ്യ(T&& s) {
        എങ്കിൽ (is_Dynamic_സംഖ്യ()) {
            m_dynamic_സംഖ്യ = std::forward<T>(s);
            തിരികെ;
        }
        എങ്കിൽ (isString()) destroyString();
        init_Dynamic_സംഖ്യ(std::forward<T>(s));
    }
    template <typename T>
    ശൂന്യം reinitWithOrAssignString(T&& s) {
        എങ്കിൽ (isString()) {
            m_string = std::forward<T>(s);
            തിരികെ;
        }
        എങ്കിൽ (is_Dynamic_സംഖ്യ()) destroy_Dynamic_സംഖ്യ();
        initString(std::forward<T>(s));
    }
};

class V3_സംഖ്യ final {
    // TYPES
    using മൂല്യം_And_X = V3_സംഖ്യ_Data::മൂല്യം_And_X;
    using V3_സംഖ്യ_Data_Type = V3_സംഖ്യ_Data::V3_സംഖ്യ_Data_Type;

    // MEMBERS
    V3_സംഖ്യ_Data m_data;
    Ast_കണ്ണി* m_nodep = nullptr;  // Parent node - for error reporting only
    File_വരി* m_fileline = nullptr;  // Source location - if no parent node is reasonable

    // METHODS
    ശൂന്യം setNull() {
        m_data.setLogic();
        m_data.m_isNull = true;
        m_data.m_autoExtend = true;
    }
    V3_സംഖ്യ& setSingleBits(അക്ഷരം മൂല്യം);
    ശൂന്യം fillBits(പൂർണ്ണസംഖ്യ destLsb, പൂർണ്ണസംഖ്യ width, അക്ഷരം മൂല്യം);
    ശൂന്യം copyBits(പൂർണ്ണസംഖ്യ destLsb, സ്ഥിരം V3_സംഖ്യ& source, പൂർണ്ണസംഖ്യ sourceLsb, പൂർണ്ണസംഖ്യ width);
    V3_സംഖ്യ& setString(സ്ഥിരം string& str) {
        m_data.setString(str);
        തിരികെ *this;
    }
    ശൂന്യം opCleanThis(bool warnOnTruncation = false);

പൊതു:
    ശൂന്യം nodep(Ast_കണ്ണി* nodep) VL_MT_STABLE;
    Ast_കണ്ണി* nodep() സ്ഥിരം VL_MT_SAFE { തിരികെ m_nodep; }  // For debug only
    File_വരി* fileline() സ്ഥിരം VL_MT_SAFE { തിരികെ m_fileline; }
    V3_സംഖ്യ& setZero();
    V3_സംഖ്യ& setQuad(uint64_t മൂല്യം);
    V3_സംഖ്യ& setLong(uint32_t മൂല്യം);
    V3_സംഖ്യ& setLongS(int32_t മൂല്യം);
    V3_സംഖ്യ& setDouble(ഇരട്ട മൂല്യം);
    ശൂന്യം setBitX0(പൂർണ്ണസംഖ്യ bit);
    ശൂന്യം setBit(പൂർണ്ണസംഖ്യ bit, അക്ഷരം മൂല്യം) {  // Note: must be initialized as number and pre-zeroed!
        എങ്കിൽ (bit >= m_data.width()) തിരികെ;
        സ്ഥിരം uint32_t mask = (1UL << (bit & 31));
        മൂല്യം_And_X& v = m_data.അക്കം()[bit / 32];
        എങ്കിൽ (മൂല്യം == '0' || മൂല്യം == 0) {
            v.m_മൂല്യം &= ~mask;
            v.m_മൂല്യം_X &= ~mask;
        } അല്ലെങ്കിൽ എങ്കിൽ (മൂല്യം == '1' || മൂല്യം == 1) {
            v.m_മൂല്യം |= mask;
            v.m_മൂല്യം_X &= ~mask;
        } അല്ലെങ്കിൽ എങ്കിൽ (മൂല്യം == 'z' || മൂല്യം == 2) {
            v.m_മൂല്യം &= ~mask;
            v.m_മൂല്യം_X |= mask;
        } അല്ലെങ്കിൽ {  // X
            v.m_മൂല്യം |= mask;
            v.m_മൂല്യം_X |= mask;
        }
    }

സ്വകാര്യ:
    അക്ഷരം bitIs(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം {
        എങ്കിൽ (bit >= m_data.width() || bit < 0) {
            // We never sign extend
            തിരികെ '0';
        }
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
        തിരികെ ("01zx"[(((v.m_മൂല്യം & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_മൂല്യം_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }
    അക്ഷരം bitIsExtend(പൂർണ്ണസംഖ്യ bit, പൂർണ്ണസംഖ്യ lbits) സ്ഥിരം {
        // lbits usually = width, but for C optimizations width=32_bits, lbits = 32_or_less
        എങ്കിൽ (bit < 0) തിരികെ '0';
        UASSERT(lbits <= m_data.width(), "Extend of wrong size");
        എങ്കിൽ (bit >= lbits) {
            bit = lbits ? lbits - 1 : 0;
            സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
            // We do sign extend
            തിരികെ ("01zx"[(((v.m_മൂല്യം & (1UL << (bit & 31))) ? 1 : 0)
                            | ((v.m_മൂല്യം_X & (1UL << (bit & 31))) ? 2 : 0))]);
        }
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
        തിരികെ ("01zx"[(((v.m_മൂല്യം & (1UL << (bit & 31))) ? 1 : 0)
                        | ((v.m_മൂല്യം_X & (1UL << (bit & 31))) ? 2 : 0))]);
    }

പൊതു:
    bool bitIs0(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം VL_MT_SAFE {
        എങ്കിൽ (!is_സംഖ്യ()) തിരികെ false;
        എങ്കിൽ (bit < 0) തിരികെ false;
        എങ്കിൽ (bit >= m_data.width()) തിരികെ !bitIsXZ(m_data.width() - 1);
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
        തിരികെ ((v.m_മൂല്യം | v.m_മൂല്യം_X) & (1UL << (bit & 31))) == 0;
    }
    bool bitIs1(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം VL_MT_SAFE {
        എങ്കിൽ (!is_സംഖ്യ()) തിരികെ false;
        എങ്കിൽ (bit < 0) തിരികെ false;
        എങ്കിൽ (bit >= m_data.width()) തിരികെ false;
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
        തിരികെ ((v.m_മൂല്യം & (1UL << (bit & 31))) && !(v.m_മൂല്യം_X & (1UL << (bit & 31))));
    }
    bool bitIs1Extend(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം {
        // Correct number of zero bits/width matters
        എങ്കിൽ (!is_സംഖ്യ()) തിരികെ false;
        എങ്കിൽ (bit < 0) തിരികെ false;
        എങ്കിൽ (bit >= m_data.width()) തിരികെ bitIs1Extend(m_data.width() - 1);
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
        തിരികെ ((v.m_മൂല്യം & (1UL << (bit & 31))) && !(v.m_മൂല്യം_X & (1UL << (bit & 31))));
    }
    bool bitIsX(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം VL_MT_SAFE {
        എങ്കിൽ (!is_സംഖ്യ()) തിരികെ false;
        എങ്കിൽ (bit < 0) തിരികെ false;
        എങ്കിൽ (bit >= m_data.width()) തിരികെ bitIsZ(m_data.width() - 1);
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
        തിരികെ ((v.m_മൂല്യം & (1UL << (bit & 31))) && (v.m_മൂല്യം_X & (1UL << (bit & 31))));
    }
    bool bitIsXZ(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം VL_MT_SAFE {
        എങ്കിൽ (!is_സംഖ്യ()) തിരികെ false;
        എങ്കിൽ (bit < 0) തിരികെ false;
        എങ്കിൽ (bit >= m_data.width()) തിരികെ bitIsXZ(m_data.width() - 1);
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
        തിരികെ ((v.m_മൂല്യം_X & (1UL << (bit & 31))));
    }
    bool bitIsZ(പൂർണ്ണസംഖ്യ bit) സ്ഥിരം VL_MT_SAFE {
        എങ്കിൽ (!is_സംഖ്യ()) തിരികെ false;
        എങ്കിൽ (bit < 0) തിരികെ false;
        എങ്കിൽ (bit >= m_data.width()) തിരികെ bitIsZ(m_data.width() - 1);
        സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[bit / 32];
        തിരികെ ((~v.m_മൂല്യം & (1UL << (bit & 31))) && (v.m_മൂല്യം_X & (1UL << (bit & 31))));
    }

സ്വകാര്യ:
    uint32_t bits_മൂല്യം(പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ nbits) സ്ഥിരം VL_MT_SAFE {
        uint32_t v = 0;
        വേണ്ടി (പൂർണ്ണസംഖ്യ bitn = 0; bitn < nbits; bitn++) v |= (bitIs1(lsb + bitn) << bitn);
        തിരികെ v;
    }

    പൂർണ്ണസംഖ്യ എണ്ണം_X(പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ nbits) സ്ഥിരം VL_MT_SAFE;
    പൂർണ്ണസംഖ്യ എണ്ണം_Z(പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ nbits) സ്ഥിരം VL_MT_SAFE;

    പൂർണ്ണസംഖ്യ വാക്കുകൾ() സ്ഥിരം VL_MT_SAFE { തിരികെ ((width() + 31) / 32); }
    uint32_t hiWordMask() സ്ഥിരം VL_MT_SAFE {
        // Correct number of zero bits/width matters
        തിരികെ VL_MASK_I(width());
    }

    V3_സംഖ്യ& opModDivGuts(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, bool is_modulus);

പൊതു:
    // CONSTRUCTORS
    explicit V3_സംഖ്യ(Ast_കണ്ണി* nodep) { init(nodep, 1); }
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ width) {  // 0=unsized
        init(nodep, width, width > 0);
    }
    // Construct with value, changing to new width
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ width, സ്ഥിരം V3_സംഖ്യ& മൂല്യം) {
        init(nodep, width, width > 0);
        opAssign(മൂല്യം);
    }
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ width, uint32_t മൂല്യം, bool sized = true) {
        init(nodep, width, sized);
        m_data.അക്കം()[0].m_മൂല്യം = മൂല്യം;
        opCleanThis();
    }
    V3_സംഖ്യ(File_വരി* flp, പൂർണ്ണസംഖ്യ width, uint32_t മൂല്യം, bool sized = true) {
        init(nullptr, width, sized);
        m_fileline = flp;
        m_data.അക്കം()[0].m_മൂല്യം = മൂല്യം;
        opCleanThis();
    }
    // Create from a verilog 32'hxxxx number.
    class Verilog_സംഖ്യ_Literal {};  // For creator type-overload selection
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, Verilog_സംഖ്യ_Literal, സ്ഥിരം അക്ഷരം* sourcep) { create(nodep, sourcep); }
    V3_സംഖ്യ(File_വരി* flp, Verilog_സംഖ്യ_Literal, സ്ഥിരം അക്ഷരം* sourcep) { create(flp, sourcep); }
    class VerilogStringLiteral {};  // For creator type-overload selection
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, VerilogStringLiteral, സ്ഥിരം string& str);
    class Double {};
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, Double, ഇരട്ട മൂല്യം) {
        init(nodep, 64);
        setDouble(മൂല്യം);
    }
    class String {};
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, String, സ്ഥിരം string& മൂല്യം) {
        init(nodep);
        setString(മൂല്യം);
        m_data.m_fromString = true;
    }
    class OneStep {};
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, OneStep) {
        init(nodep, 64);
        m_data.m_is1Step = true;
    }
    class Null {};
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, Null) {
        init(nodep);
        setNull();
    }
    explicit V3_സംഖ്യ(സ്ഥിരം V3_സംഖ്യ* nump, പൂർണ്ണസംഖ്യ width = 1) {
        init(nullptr, width);
        m_fileline = nump->fileline();
    }
    V3_സംഖ്യ(സ്ഥിരം V3_സംഖ്യ* nump, പൂർണ്ണസംഖ്യ width, uint32_t മൂല്യം) {
        init(nullptr, width);
        m_data.അക്കം()[0].m_മൂല്യം = മൂല്യം;
        opCleanThis();
        m_fileline = nump->fileline();
    }
    V3_സംഖ്യ(Ast_കണ്ണി* nodep, സ്ഥിരം Ast_കണ്ണി_DType* nodedtypep);

    V3_സംഖ്യ(സ്ഥിരം V3_സംഖ്യ& other) = സ്ഥിരസ്ഥിതി;
    V3_സംഖ്യ& operator=(സ്ഥിരം V3_സംഖ്യ& other) = സ്ഥിരസ്ഥിതി;

    V3_സംഖ്യ(V3_സംഖ്യ&& other) = സ്ഥിരസ്ഥിതി;
    V3_സംഖ്യ& operator=(V3_സംഖ്യ&& other) = സ്ഥിരസ്ഥിതി;

    ~V3_സംഖ്യ() = സ്ഥിരസ്ഥിതി;

സ്വകാര്യ:
    ശൂന്യം selfTestThis();
    ശൂന്യം create(Ast_കണ്ണി* nodep, സ്ഥിരം അക്ഷരം* sourcep) {
        init(nodep, 0);
        m_fileline = nullptr;
        create(sourcep);
    }
    ശൂന്യം create(File_വരി* flp, സ്ഥിരം അക്ഷരം* sourcep) {
        init(nullptr, 0);
        m_fileline = flp;
        create(sourcep);
    }
    ശൂന്യം create(സ്ഥിരം അക്ഷരം* sourcep);
    ശൂന്യം init(Ast_കണ്ണി* nodep, പൂർണ്ണസംഖ്യ swidth = -1, bool sized = true) {
        this->nodep(nodep);
        എങ്കിൽ (swidth >= 0) {
            എങ്കിൽ (swidth == 0) {
                swidth = 1;
                sized = false;
            }
            m_data.setLogic();
            m_data.resize(swidth);
            m_data.m_sized = sized;
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) m_data.അക്കം()[i] = {0, 0};
        } അല്ലെങ്കിൽ {
            m_data.resize(1);
            m_data.m_sized = false;
        }
    }
    സ്ഥിതം string displayPad(size_t fmtsize, അക്ഷരം pad, bool ഇടത്, സ്ഥിരം string& in) VL_PURE;
    string displayed(സ്ഥിരം string& vformat) സ്ഥിരം VL_MT_STABLE {
        തിരികെ displayed(m_fileline, vformat);
    }
    ശൂന്യം warnTooMany(സ്ഥിരം string& മൂല്യം);

പൊതു:
    ശൂന്യം v3errorEnd(സ്ഥിരം std::ostringstream& sstr) സ്ഥിരം VL_RELEASE(V3Error::s().m_mutex);
    ശൂന്യം v3errorEndFatal(സ്ഥിരം std::ostringstream& sstr) സ്ഥിരം VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex);
    ശൂന്യം width(പൂർണ്ണസംഖ്യ width, bool sized = true) {
        m_data.m_sized = sized;
        m_data.resize(width);
    }

    // SETTERS
    V3_സംഖ്യ& setAllBitsXRemoved();
    V3_സംഖ്യ& setAllBitsX();
    V3_സംഖ്യ& setAllBitsZ();
    V3_സംഖ്യ& setAllBits0();
    V3_സംഖ്യ& setAllBits1();
    V3_സംഖ്യ& setValue1();
    // IE if nbits=1, then 0b1, if 2->0b11, if 3->0b111 etc
    V3_സംഖ്യ& setMask(പൂർണ്ണസംഖ്യ nbits, പൂർണ്ണസംഖ്യ lsb = 0);

    // ACCESSORS
    string ascii(bool prefixed = true, bool cleanVerilog = false) സ്ഥിരം VL_MT_STABLE;
    string displayed(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& vformat,
                     സ്ഥിരം VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) സ്ഥിരം VL_MT_STABLE;
    string displayed(File_വരി* fl, സ്ഥിരം string& vformat,
                     സ്ഥിരം VFormatAttr& formatAttr = VFormatAttr::UNSIGNED) സ്ഥിരം VL_MT_STABLE;
    string displayedEnum(സ്ഥിരം AstSFormatArg* argp, സ്ഥിരം string& vformat) സ്ഥിരം VL_MT_STABLE;
    string displayedEnumName(സ്ഥിരം AstEnumDType* dtypep) സ്ഥിരം VL_MT_STABLE;
    സ്ഥിതം string displayedEnumName(സ്ഥിരം AstEnumItem* itemp);
    സ്ഥിതം bool displayedFmtHasArg(അക്ഷരം format, bool isScan);
    string emitC() സ്ഥിരം VL_MT_STABLE;
    പൂർണ്ണസംഖ്യ width() സ്ഥിരം VL_MT_SAFE { തിരികെ m_data.width(); }
    പൂർണ്ണസംഖ്യ widthToFit() സ്ഥിരം;  // Minimum width that can represent this number (~== log2(num)+1)
    bool sized() സ്ഥിരം VL_MT_SAFE { തിരികെ m_data.m_sized; }
    bool autoExtend() സ്ഥിരം VL_MT_SAFE { തിരികെ m_data.m_autoExtend; }
    bool isFromString() സ്ഥിരം { തിരികെ m_data.m_fromString; }
    bool hasOrigParamName() സ്ഥിരം { തിരികെ m_data.m_hasOrigParamName; }
    ശൂന്യം hasOrigParamName(bool flag) { m_data.m_hasOrigParamName = flag; }
    V3_സംഖ്യ_Data_Type dataType() സ്ഥിരം VL_MT_SAFE { തിരികെ m_data.type(); }
    ശൂന്യം dataType(V3_സംഖ്യ_Data_Type newType) {
        എങ്കിൽ (dataType() == newType) തിരികെ;
        UASSERT(newType != V3_സംഖ്യ_Data_Type::UNINITIALIZED, "Can't set type to UNINITIALIZED.");
        തിരഞ്ഞെടുപ്പ് (newType) {
        അവസ്ഥ V3_സംഖ്യ_Data_Type::STRING: m_data.setString(); നിർത്തുക;
        അവസ്ഥ V3_സംഖ്യ_Data_Type::DOUBLE: m_data.setDouble(); നിർത്തുക;
        അവസ്ഥ V3_സംഖ്യ_Data_Type::LOGIC: m_data.setLogic(); നിർത്തുക;
        അവസ്ഥ V3_സംഖ്യ_Data_Type::UNINITIALIZED: നിർത്തുക;
        }
    }
    // Only correct for parsing of numbers from strings, otherwise not used
    // (use AstConst::isSigned())
    bool isSigned() സ്ഥിരം VL_MT_SAFE { തിരികെ m_data.m_signed; }
    ശൂന്യം isSigned(bool ssigned) { m_data.m_signed = ssigned; }
    bool isDouble() സ്ഥിരം VL_MT_SAFE { തിരികെ dataType() == V3_സംഖ്യ_Data_Type::DOUBLE; }
    bool isString() സ്ഥിരം VL_MT_SAFE { തിരികെ dataType() == V3_സംഖ്യ_Data_Type::STRING; }
    bool isOpaque() സ്ഥിരം VL_MT_SAFE { തിരികെ isDouble() || isString(); }
    bool is_സംഖ്യ() സ്ഥിരം VL_MT_SAFE {
        തിരികെ m_data.type() == V3_സംഖ്യ_Data_Type::LOGIC
               || m_data.type() == V3_സംഖ്യ_Data_Type::DOUBLE;
    }
    bool isNegative() സ്ഥിരം VL_MT_SAFE {
        // Correct number of zero bits/width matters
        തിരികെ !isString() && bitIs1(width() - 1);
    }
    bool is1Step() സ്ഥിരം VL_MT_SAFE { തിരികെ m_data.m_is1Step; }
    bool isNull() സ്ഥിരം VL_MT_SAFE { തിരികെ m_data.m_isNull; }
    bool isFourState() സ്ഥിരം VL_MT_SAFE;
    bool hasZ() സ്ഥിരം {
        എങ്കിൽ (isString()) തിരികെ false;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < വാക്കുകൾ(); ++i) {
            സ്ഥിരം മൂല്യം_And_X v = m_data.അക്കം()[i];
            എങ്കിൽ ((~v.m_മൂല്യം) & v.m_മൂല്യം_X) തിരികെ true;
        }
        തിരികെ false;
    }
    bool isAllZ() സ്ഥിരം VL_MT_SAFE;
    bool isAllX() സ്ഥിരം VL_MT_SAFE;
    bool isEqZero() സ്ഥിരം VL_MT_SAFE;
    bool isNeqZero() സ്ഥിരം;
    bool isBitsZero(പൂർണ്ണസംഖ്യ msb, പൂർണ്ണസംഖ്യ lsb) സ്ഥിരം;
    bool isBroken(പൂർണ്ണസംഖ്യ vwidth) സ്ഥിരം;
    bool isEqOne() സ്ഥിരം;
    bool isEqAllOnes(പൂർണ്ണസംഖ്യ optwidth = 0) സ്ഥിരം;
    bool isCaseEq(സ്ഥിരം V3_സംഖ്യ& rhs) സ്ഥിരം;  // operator==
    bool isLtXZ(സ്ഥിരം V3_സംഖ്യ& rhs) സ്ഥിരം;  // operator< with XZ compared
    bool isAnyX() സ്ഥിരം VL_MT_SAFE;
    bool isAnyXZ() സ്ഥിരം;
    bool isAnyZ() സ്ഥിരം VL_MT_SAFE;
    bool isMsbXZ() സ്ഥിരം {
        // Correct number of zero bits/width matters
        തിരികെ bitIsXZ(width() - 1);
    }
    bool fitsInUInt() സ്ഥിരം VL_MT_SAFE;
    uint32_t toUInt() സ്ഥിരം VL_MT_SAFE;
    int32_t toSInt() സ്ഥിരം VL_MT_SAFE;
    uint64_t toUQuad() സ്ഥിരം VL_MT_SAFE;
    int64_t toSQuad() സ്ഥിരം VL_MT_SAFE;
    string toString() സ്ഥിരം VL_MT_SAFE;
    string toDecimalS() സ്ഥിരം VL_MT_STABLE;  // return ASCII signed decimal number
    string toDecimalU() സ്ഥിരം VL_MT_STABLE;  // return ASCII unsigned decimal number
    // return ASCII real number, with the fewest of 15 or 17 digits that is exact
    string toRealString() സ്ഥിരം VL_MT_STABLE;
    ഇരട്ട toDouble() സ്ഥിരം VL_MT_SAFE;
    V3Hash toHash() സ്ഥിരം;
    uint32_t edataWord(പൂർണ്ണസംഖ്യ eword) സ്ഥിരം;
    uint8_t dataByte(പൂർണ്ണസംഖ്യ byte) സ്ഥിരം;
    uint32_t എണ്ണം_Bits(സ്ഥിരം V3_സംഖ്യ& ctrl) സ്ഥിരം;
    uint32_t എണ്ണം_Bits(സ്ഥിരം V3_സംഖ്യ& ctrl1, സ്ഥിരം V3_സംഖ്യ& ctrl2, സ്ഥിരം V3_സംഖ്യ& ctrl3) സ്ഥിരം;
    uint32_t എണ്ണം_Ones() സ്ഥിരം;
    uint32_t mostSetBitP1() സ്ഥിരം;  // Highest bit set + 1, e.g. for 16 return 5, for 0 return 0
    uint32_t leastSetBitP1() സ്ഥിരം;  // Lowest bit set + 1, e.g. for 14 return 2, for 0 return 0

    // Operators
    bool operator<(സ്ഥിരം V3_സംഖ്യ& rhs) സ്ഥിരം { തിരികെ isLtXZ(rhs); }

    // STATICS
    സ്ഥിതം bool epsilonEqual(ഇരട്ട a, ഇരട്ട b);  // True if number within Epsilon of the other
    സ്ഥിതം bool epsilonIntegral(ഇരട്ട a);  // True if number rounds to integer within Epsilon
    സ്ഥിതം പൂർണ്ണസംഖ്യ log2b(uint32_t അക്കം);
    സ്ഥിതം പൂർണ്ണസംഖ്യ log2bQuad(uint64_t അക്കം);
    സ്ഥിതം ശൂന്യം selfTest();

    // MATH
    // "this" is the output, as we need the output width before some computations
    V3_സംഖ്യ& opBitsNonXZ(സ്ഥിരം V3_സംഖ്യ& lhs);  // 0/1->1, X/Z->0
    V3_സംഖ്യ& opBitsOne(സ്ഥിരം V3_സംഖ്യ& lhs);  // 1->1, 0/X/Z->0
    V3_സംഖ്യ& opBitsXZ(സ്ഥിരം V3_സംഖ്യ& lhs);  // 0/1->0, X/Z->1
    V3_സംഖ്യ& opBitsZ(സ്ഥിരം V3_സംഖ്യ& lhs);  // Z->1, 0/1/X->0
    //
    V3_സംഖ്യ& opAssign(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opAssignNonXZ(സ്ഥിരം V3_സംഖ്യ& lhs, bool ignoreXZ = true);
    V3_സംഖ്യ& opExtendS(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t lbits);  // Sign extension
    V3_സംഖ്യ& opExtendXZ(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t lbits);  // X/Z extension
    V3_സംഖ്യ& opRedOr(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opRedAnd(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opRedXor(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& op_എണ്ണം_Bits(സ്ഥിരം V3_സംഖ്യ& വ്യഞ്ജകം, സ്ഥിരം V3_സംഖ്യ& ctrl1, സ്ഥിരം V3_സംഖ്യ& ctrl2,
                          സ്ഥിരം V3_സംഖ്യ& ctrl3);
    V3_സംഖ്യ& op_എണ്ണം_Ones(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opIsUnknown(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opOneHot(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opOneHot0(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opCLog2(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opMostSetBitP1(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opClean(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t bits);
    V3_സംഖ്യ& opConcat(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLenN(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opRepl(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opRepl(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t rhsval);
    V3_സംഖ്യ& opStreamL(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opSel(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& msb, സ്ഥിരം V3_സംഖ്യ& lsb);
    V3_സംഖ്യ& opSel(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t msbval, uint32_t lsbval);
    V3_സംഖ്യ& opSelInto(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& lsb, പൂർണ്ണസംഖ്യ width);
    V3_സംഖ്യ& opSelInto(സ്ഥിരം V3_സംഖ്യ& lhs, പൂർണ്ണസംഖ്യ lsbval, പൂർണ്ണസംഖ്യ width);
    V3_സംഖ്യ& opCvtPackString(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opToLowerN(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opToUpperN(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opCaseEq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opCaseNeq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opWildEq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opWildNeq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opBufIf1(സ്ഥിരം V3_സംഖ്യ& ens, സ്ഥിരം V3_സംഖ്യ& if1s);
    V3_സംഖ്യ& opSetRange(uint32_t lsb, uint32_t width, അക്ഷരം bit_മൂല്യം);
    // "standard" math
    V3_സംഖ്യ& opNot(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opLogNot(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opLogAnd(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLogOr(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLogEq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLogIf(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opNegate(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opAdd(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opSub(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opMul(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opMulS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed
    V3_സംഖ്യ& opDiv(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opDivS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed
    V3_സംഖ്യ& opModDiv(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opModDivS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed
    V3_സംഖ്യ& opPow(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, bool lsign = false,
                    bool rsign = false);
    V3_സംഖ്യ& opPowSU(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed lhs, unsigned rhs
    V3_സംഖ്യ& opPowSS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed lhs, signed rhs
    V3_സംഖ്യ& opPowUS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Unsigned lhs, signed rhs
    V3_സംഖ്യ& opAnd(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opXor(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opOr(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opShiftR(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opShiftRS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs,  // Arithmetic w/carry
                        uint32_t lbits);
    V3_സംഖ്യ& opShiftL(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    // Comparisons
    V3_സംഖ്യ& opEq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opNeq(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opGt(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opGtS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed
    V3_സംഖ്യ& opGte(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opGteS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed
    V3_സംഖ്യ& opLt(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLtS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed
    V3_സംഖ്യ& opLte(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLteS(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);  // Signed

    // "D" - double (aka real) math
    V3_സംഖ്യ& opIToRD(സ്ഥിരം V3_സംഖ്യ& lhs, bool isSigned = false);
    V3_സംഖ്യ& opISToRD(സ്ഥിരം V3_സംഖ്യ& lhs) { തിരികെ opIToRD(lhs, true); }
    V3_സംഖ്യ& opRToIS(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opRToIRoundS(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opRealToBits(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opBitsToRealD(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opNegateD(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opAddD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opSubD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opMulD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opDivD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opPowD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    // Comparisons
    V3_സംഖ്യ& opEqD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opNeqD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opGtD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opGteD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLtD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLteD(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);

    // "N" - string operations
    V3_സംഖ്യ& opAtoN(സ്ഥിരം V3_സംഖ്യ& lhs, പൂർണ്ണസംഖ്യ base);
    V3_സംഖ്യ& opNToI(സ്ഥിരം V3_സംഖ്യ& lhs);
    V3_സംഖ്യ& opPutcN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, സ്ഥിരം V3_സംഖ്യ& ths);
    V3_സംഖ്യ& opGetcN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opSubstrN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, സ്ഥിരം V3_സംഖ്യ& ths);
    V3_സംഖ്യ& opCompareNN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs, bool ignoreCase);
    V3_സംഖ്യ& opConcatN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opReplN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opReplN(സ്ഥിരം V3_സംഖ്യ& lhs, uint32_t rhsval);
    V3_സംഖ്യ& opEqN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opNeqN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opGtN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opGteN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLtN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
    V3_സംഖ്യ& opLteN(സ്ഥിരം V3_സംഖ്യ& lhs, സ്ഥിരം V3_സംഖ്യ& rhs);
};
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം V3_സംഖ്യ& rhs) VL_MT_SAFE {
    തിരികെ os << rhs.ascii();
}

#endif  // Guard
