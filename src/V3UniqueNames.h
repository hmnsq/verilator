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

#నిర్వచితం_కాకపోతే VERILATOR_V3UNIQUENAMES_H_
#నిర్వచించు VERILATOR_V3UNIQUENAMES_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Hasher.h"

#అన్తర్భావ <map>
#అన్తర్భావ <string>

class V3_Unique_పేర్లు final {
    std::string m_ఉపసర్గ;  // Prefix to attach to all names

    std::map<std::string, గుర్తులేని> m_multiplicity;  // Suffix number for given key

    bool m_కూడు_Suffix = true;  // Ad suffix or not

బహిరంగ:
    V3_Unique_పేర్లు() = అప్రమేయ;
    explicit V3_Unique_పేర్లు(స్థిరరాశి std::string& ఉపసర్గ, bool కూడు_Suffix = true)
        : m_ఉపసర్గ{ఉపసర్గ}
        , m_కూడు_Suffix{కూడు_Suffix} {
        ఒకవేళ (!m_ఉపసర్గ.ఖాళీలు()) {
            UASSERT(VString::startsWith(m_ఉపసర్గ, "__V"), "Prefix must start with '__V'");
            UASSERT(!VString::చివర్లు_With(m_ఉపసర్గ, "_"), "Prefix must not end with '_'");
        }
    }

    // Return argument, prepended with the prefix if any, then appended with a unique suffix each
    // time we are called with the same argument.
    std::string get(స్థిరరాశి std::string& name) {
        ఒకవేళ (!m_కూడు_Suffix) {
            ఒకవేళ (m_multiplicity.లెక్క(name) == 0) {
                m_multiplicity[name] = 0;
                తిరిగి name;
            }
            తిరిగి get(name + "__" + cvt_గమ్యం_Str(m_multiplicity[name]++));
        }
        // NORMAL mode
        స్థిరరాశి గుర్తులేని అంకె = m_multiplicity[name]++;
        std::string ఫలితం;
        ఒకవేళ (!m_ఉపసర్గ.ఖాళీలు()) {
            ఫలితం += m_ఉపసర్గ;
            ఫలితం += "_";
        }
        ఫలితం += name;
        ఫలితం += "__";
        ఫలితం += cvt_గమ్యం_Str(అంకె);
        తిరిగి ఫలితం;
    }

    // Return hash of node as string, prepended with the prefix if any, appended with a unique
    // suffix each time we are called with a node that hashes to the same value.
    std::string get(స్థిరరాశి Ast_కణుపు* nodep) {
        ఒకవేళ (nodep == nullptr) { తిరిగి get(""); }
        తిరిగి get(V3Hasher::uncachedHash(nodep).గమ్యం_String());
    }

    // Reset to initial state (as if just constructed)
    శూన్య reset() { m_multiplicity.clear(); }
};

#అయితే_ముగింపు  // Guard
