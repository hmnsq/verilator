// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: Coverage points
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_VLCPOINT_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_VLCPOINT_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3ERROR_NO_GLOBAL_
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ "verilated_cov_key.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class Vlc_ਬਿੰਦੂ final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_ਬਿੰਦੂ_ਅੰਕ;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_ਗਿਣਤੀ = 0;  //< Count of hits across all tests

ਜਨਤਕ:
    // CONSTRUCTORS
    Vlc_ਬਿੰਦੂ(ਅਚਲ string& name, uint64_t ਬਿੰਦੂ_ਅੰਕ)
        : m_name{name}
        , m_ਬਿੰਦੂ_ਅੰਕ{ਬਿੰਦੂ_ਅੰਕ} {}
    ~Vlc_ਬਿੰਦੂ() = ਮੂਲ;
    // ACCESSORS
    ਅਚਲ string& name() ਅਚਲ { ਵਾਪਸ m_name; }
    uint64_t ਬਿੰਦੂ_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_ਬਿੰਦੂ_ਅੰਕ; }
    uint64_t testsCovering() ਅਚਲ { ਵਾਪਸ m_testsCovering; }
    ਖਾਲੀ ਗਿਣਤੀ_Inc(uint64_t inc) { m_ਗਿਣਤੀ += inc; }
    uint64_t ਗਿਣਤੀ_2() ਅਚਲ { ਵਾਪਸ m_ਗਿਣਤੀ; }
    ਖਾਲੀ testsCoveringInc() { m_testsCovering++; }
    bool ok(ਬਿਨਾਨਿਸ਼ਾਨੀ annotateMin) ਅਚਲ {
        ਅਚਲ std::string threshStr = thresh();
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ threshi = !threshStr.ਖਾਲੀ_ਥਾਵਾਂ() ? std::atoi(threshStr.c_str()) : annotateMin;
        ਵਾਪਸ m_ਗਿਣਤੀ >= threshi;
    }
    // KEY ACCESSORS
    string filename() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_HIER, m_name.c_str()); }
    string ਪੰਨਾ() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ("page", m_name.c_str()); }
    string type() ਅਚਲ { ਵਾਪਸ type_ਕੱਢੋ(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_BIN, m_name.c_str()); }
    string binType() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ਅਚਲ { ਵਾਪਸ !ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_CROSS, m_name.c_str()).ਖਾਲੀ_ਥਾਵਾਂ(); }
    string crossBins() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string ਸਮੂਹ_ਭਾਰ() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_ਸਮੂਹ_ਭਾਰ, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string ਭਾਰ_2() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_ਭਾਰ, m_name.c_str()); }
    string thresh() ਅਚਲ {
        // string as maybe ""
        ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ਅਚਲ { ਵਾਪਸ type() == "fsm_state"; }
    bool isFsmArc() ਅਚਲ { ਵਾਪਸ type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsm_ਮੰਜ਼ਿਲ_State() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_FSM_ਮੰਜ਼ਿਲ, m_name.c_str()); }
    string fsm_ਟੈਗ() ਅਚਲ { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_FSM_ਟੈਗ, m_name.c_str()); }
    bool isFsmResetInclude() ਅਚਲ { ਵਾਪਸ fsm_ਟੈਗ() == "reset_include"; }
    bool isFsmResetArc() ਅਚਲ { ਵਾਪਸ fsm_ਟੈਗ() == "reset"; }
    bool isFsmDefaultArc() ਅਚਲ { ਵਾਪਸ fsm_ਟੈਗ() == "default"; }
    bool fsmIsReset() ਅਚਲ { ਵਾਪਸ isFsmResetArc() || isFsmResetInclude(); }
    ਪੂਰਨ lineno() ਅਚਲ {
        ਅਚਲ string ਸਤਰ_Str = ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_LINENO, m_name.c_str());
        ਵਾਪਸ std::atoi(ਸਤਰ_Str.c_str());
    }
    ਪੂਰਨ ਕਾਲਮ_2() ਅਚਲ {
        ਅਚਲ string ਕਾਲਮ_Str = ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_ਕਾਲਮ, m_name.c_str());
        ਵਾਪਸ std::atoi(ਕਾਲਮ_Str.c_str());
    }
    // METHODS
    ਸਥਿਰ string type_ਕੱਢੋ(ਅਚਲ ਅੱਖਰ* name) { ਵਾਪਸ ਕੁੰਜੀ_ਕੱਢੋ(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ਸਥਿਰ string ਕੁੰਜੀ_ਕੱਢੋ(ਅਚਲ ਅੱਖਰ* short_ਕੁੰਜੀ, ਅਚਲ ਅੱਖਰ* name) {
        // Hot function
        ਅਚਲ ਮਾਪ_t shortLen = std::ਲੜੀ_ਲੰਬਾਈ(short_ਕੁੰਜੀ);
        ਲਈ (ਅਚਲ ਅੱਖਰ* cp = name; *cp; ++cp) {
            ਜੇ (*cp == '\001') {
                ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(cp + 1, short_ਕੁੰਜੀ, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ਅਚਲ ਅੱਖਰ* ep = cp;
                    ਜਦੋਂ (*ep && *ep != '\001') ++ep;
                    ਵਾਪਸ Verilated_Cov_ਕੁੰਜੀ::unescape(string(cp, ep - cp));
                }
            }
        }
        ਵਾਪਸ "";
    }
    ਸਥਿਰ ਖਾਲੀ dump_ਸਿਰਲੇਖ(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    ਖਾਲੀ dump(std::ostream& os) ਅਚਲ {
        os << "  " << std::setw(8) << std::setfill('0') << ਬਿੰਦੂ_ਅੰਕ();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << ਗਿਣਤੀ_2();
        os << ",  \"" << name() << "\"\n";
    }
    ਖਾਲੀ dumpAnnotate(std::ostream& os, ਬਿਨਾਨਿਸ਼ਾਨੀ annotateMin) ਅਚਲ {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << ਗਿਣਤੀ_2();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        ਜੇ (isCross()) {
            ਅਚਲ string bins = crossBins();
            ਜੇ (!bins.ਖਾਲੀ_ਥਾਵਾਂ()) {
                os << "        //  cross: [";
                bool ਪਹਿਲਾ = true;
                ਮਾਪ_t ਸ਼ੁਰੂ = 0;
                ਜਦੋਂ (true) {
                    ਅਚਲ ਮਾਪ_t comma = bins.ਲੱਭੋ(',', ਸ਼ੁਰੂ);
                    ਜੇ (!ਪਹਿਲਾ) os << ", ";
                    os << bins.substr(ਸ਼ੁਰੂ, comma == string::npos ? string::npos : comma - ਸ਼ੁਰੂ);
                    ਪਹਿਲਾ = false;
                    ਜੇ (comma == string::npos) ਰੋਕੋ;
                    ਸ਼ੁਰੂ = comma + 1;
                }
                os << "]\n";
            }
        }
    }
};

//********************************************************************
// VlcPoints - Container of all points

class VlcPoints final {
    // MEMBERS
    using NameMap = std::map<ਅਚਲ std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::ਸਦਿਸ਼<Vlc_ਬਿੰਦੂ> m_points;  //< List of all points
    uint64_t m_ਅੰਕ_Points = 0;  //< Total unique points

    ਸਥਿਰ ਪੂਰਨ debug() { ਵਾਪਸ V3Error::debugDefault(); }

ਜਨਤਕ:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { ਵਾਪਸ m_nameMap.begin(); }
    ByName::iterator ਸਿਰਾ() { ਵਾਪਸ m_nameMap.ਸਿਰਾ(); }

    // CONSTRUCTORS
    VlcPoints() = ਮੂਲ;
    ~VlcPoints() = ਮੂਲ;

    // METHODS
    ਖਾਲੀ dump() {
        UINFO(2, "dumpPoints...");
        Vlc_ਬਿੰਦੂ::dump_ਸਿਰਲੇਖ(std::cout);
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : *this) {
            ਅਚਲ Vlc_ਬਿੰਦੂ& ਬਿੰਦੂ = ਬਿੰਦੂ_ਸੰਖਿਆ(i.ਦੂਜਾ);
            ਬਿੰਦੂ.dump(std::cout);
        }
    }
    Vlc_ਬਿੰਦੂ& ਬਿੰਦੂ_ਸੰਖਿਆ(uint64_t ਅੰਕ) { ਵਾਪਸ m_points[ਅੰਕ]; }
    uint64_t ਲੱਭੋ_ਜੋੜੋ_ਬਿੰਦੂ(ਅਚਲ string& name, uint64_t ਗਿਣਤੀ_2) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_nameMap.emplace(name, m_ਅੰਕ_Points);
        ਜੇ (pair.ਦੂਜਾ) m_points.emplace_ਪਿੱਠ(name, m_ਅੰਕ_Points++);
        ਅਚਲ uint64_t pointnum = pair.ਪਹਿਲਾ->ਦੂਜਾ;
        m_points[pointnum].ਗਿਣਤੀ_Inc(ਗਿਣਤੀ_2);
        ਵਾਪਸ pointnum;
    }
};

//######################################################################

#ਜੇ_ਅੰਤ  // guard
