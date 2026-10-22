// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Basic data structure to keep names unique
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
//*************************************************************************

#अगर_अघोषित VERILATOR_V3UNIQUENAMES_H_
#घोषणा VERILATOR_V3UNIQUENAMES_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Hasher.h"

#समावेश <map>
#समावेश <string>

class V3UniqueNames final {
    std::string m_prefix;  // Prefix to attach to all names

    std::map<std::string, अचिन्हित> m_multiplicity;  // Suffix number for given key

    bool m_addSuffix = true;  // Ad suffix or not

खुला:
    V3UniqueNames() = शेष;
    explicit V3UniqueNames(स्थिर std::string& prefix, bool addSuffix = true)
        : m_prefix{prefix}
        , m_addSuffix{addSuffix} {
        अगर (!m_prefix.empty()) {
            UASSERT(VString::startsWith(m_prefix, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::endsWith(m_prefix, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(स्थिर std::string& name) {
        अगर (!m_addSuffix) {
            अगर (m_multiplicity.गिनती(name) == 0) {
                m_multiplicity[name] = 0;
                वापस name;
            }
            वापस get(name + "__" + cvtToStr(m_multiplicity[name]++));
        }
        // NORMAL mode
        स्थिर अचिन्हित अंक = m_multiplicity[name]++;
        std::string परिणाम;
        अगर (!m_prefix.empty()) {
            परिणाम += m_prefix;
            परिणाम += "_";
        }
        परिणाम += name;
        परिणाम += "__";
        परिणाम += cvtToStr(अंक);
        वापस परिणाम;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(स्थिर Ast_गाँठ* nodep) {
        अगर (nodep == nullptr) { वापस get(""); }
        वापस get(V3Hasher::uncachedHash(nodep).toString());
    }

    // Reset to initial state (as if just constructed)
    व्योम reset() { m_multiplicity.clear(); }
};

#पूर्ण_अगर  // Guard
