// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Language code class
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

#अगर_अघोषित VERILATOR_V3LANGCODE_H_
#घोषणा VERILATOR_V3LANGCODE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश <map>
#समावेश <set>
#समावेश <vector>

//######################################################################
//! Class for the different languages supported.
//! A separate file, since used both in V3Options (globally) and FileLine 9per
//! file).
class V3LangCode final {
खुला:
    क्रमागत en : uint8_t {
        L_ERROR,  // Must be first.
        L1364_1995,
        L1364_2001,
        L1364_2005,
        L1800_2005,
        L1800_2009,
        L1800_2012,
        L1800_2017,
        L1800_2023,
        // ***Add new elements below also***
        _ENUM_END
    };
    स्थिर अक्षर* ascii() स्थिर {
        स्थिर अक्षर* स्थिर names[]
            = {// These must match the `begin_keywords values
               " ERROR",    "1364-1995", "1364-2001", "1364-2005", "1800-2005",
               "1800-2009", "1800-2012", "1800-2017", "1800-2023"};
        वापस names[m_e];
    }
    अटल V3LangCode mostRecent() VL_MT_SAFE { वापस V3LangCode{L1800_2023}; }
    bool systemVerilog() स्थिर {
        वापस m_e == L1800_2005 || m_e == L1800_2009 || m_e == L1800_2012 || m_e == L1800_2017
               || m_e == L1800_2023;
    }
    bool legal() स्थिर { वापस m_e != L_ERROR; }
    //
    क्रमागत en m_e;
    V3LangCode()
        : m_e{L_ERROR} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3LangCode(en _e)
        : m_e{_e} {}
    explicit V3LangCode(स्थिर अक्षर* textp);
    explicit V3LangCode(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
};

//######################################################################

#पूर्ण_अगर  // guard
