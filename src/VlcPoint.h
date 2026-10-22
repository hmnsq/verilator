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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_VLCPOINT_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_VLCPOINT_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
#ವ್ಯಾಖ್ಯಾನಿಸು V3ERROR_NO_GLOBAL_
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ "verilated_cov_key.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class Vlc_ಬಿಂದು final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_ಬಿಂದು_ಅಂಕೆ;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_ಎಣಿಕೆ = 0;  //< Count of hits across all tests

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    Vlc_ಬಿಂದು(ಸ್ಥಿರಾಂಕ string& name, uint64_t ಬಿಂದು_ಅಂಕೆ)
        : m_name{name}
        , m_ಬಿಂದು_ಅಂಕೆ{ಬಿಂದು_ಅಂಕೆ} {}
    ~Vlc_ಬಿಂದು() = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    ಸ್ಥಿರಾಂಕ string& name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
    uint64_t ಬಿಂದು_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಿಂದು_ಅಂಕೆ; }
    uint64_t testsCovering() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_testsCovering; }
    ಶೂನ್ಯ ಎಣಿಕೆ_Inc(uint64_t inc) { m_ಎಣಿಕೆ += inc; }
    uint64_t ಎಣಿಕೆ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಣಿಕೆ; }
    ಶೂನ್ಯ testsCoveringInc() { m_testsCovering++; }
    bool ok(ಚಿಹ್ನೆರಹಿತ annotateMin) ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ std::string threshStr = thresh();
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ threshi = !threshStr.ಖಾಲಿಗಳು() ? std::atoi(threshStr.c_str()) : annotateMin;
        ಹಿಂತಿರುಗಿ m_ಎಣಿಕೆ >= threshi;
    }
    // KEY ACCESSORS
    string filename() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_HIER, m_name.c_str()); }
    string ಪುಟ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ("page", m_name.c_str()); }
    string type() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ type_ಹೊರತೆಗೆ(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_BIN, m_name.c_str()); }
    string binType() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_CROSS, m_name.c_str()).ಖಾಲಿಗಳು(); }
    string crossBins() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string ಗುಂಪು_ತೂಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_ಗುಂಪು_ತೂಕ, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string ತೂಕ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_ತೂಕ, m_name.c_str()); }
    string thresh() ಸ್ಥಿರಾಂಕ {
        // string as maybe ""
        ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ type() == "fsm_state"; }
    bool isFsmArc() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsm_ಗಮ್ಯ_State() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_FSM_ಗಮ್ಯ, m_name.c_str()); }
    string fsm_ಟ್ಯಾಗ್() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_FSM_ಟ್ಯಾಗ್, m_name.c_str()); }
    bool isFsmResetInclude() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ fsm_ಟ್ಯಾಗ್() == "reset_include"; }
    bool isFsmResetArc() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ fsm_ಟ್ಯಾಗ್() == "reset"; }
    bool isFsmDefaultArc() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ fsm_ಟ್ಯಾಗ್() == "default"; }
    bool fsmIsReset() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ isFsmResetArc() || isFsmResetInclude(); }
    ಪೂರ್ಣಾಂಕ lineno() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ string ಸಾಲು_Str = ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_LINENO, m_name.c_str());
        ಹಿಂತಿರುಗಿ std::atoi(ಸಾಲು_Str.c_str());
    }
    ಪೂರ್ಣಾಂಕ ಸ್ತಂಭ_2() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ string ಸ್ತಂಭ_Str = ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_ಸ್ತಂಭ, m_name.c_str());
        ಹಿಂತಿರುಗಿ std::atoi(ಸ್ತಂಭ_Str.c_str());
    }
    // METHODS
    ಸ್ಥಿರ string type_ಹೊರತೆಗೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* name) { ಹಿಂತಿರುಗಿ ಕೀಲಿ_ಹೊರತೆಗೆ(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ಸ್ಥಿರ string ಕೀಲಿ_ಹೊರತೆಗೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* short_ಕೀಲಿ, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* name) {
        // Hot function
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t shortLen = std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(short_ಕೀಲಿ);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = name; *cp; ++cp) {
            ಆದರೆ (*cp == '\001') {
                ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(cp + 1, short_ಕೀಲಿ, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ep = cp;
                    ಆಗ (*ep && *ep != '\001') ++ep;
                    ಹಿಂತಿರುಗಿ Verilated_Cov_ಕೀಲಿ::unescape(string(cp, ep - cp));
                }
            }
        }
        ಹಿಂತಿರುಗಿ "";
    }
    ಸ್ಥಿರ ಶೂನ್ಯ dump_ಶೀರ್ಷಿಕೆ(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    ಶೂನ್ಯ dump(std::ostream& os) ಸ್ಥಿರಾಂಕ {
        os << "  " << std::setw(8) << std::setfill('0') << ಬಿಂದು_ಅಂಕೆ();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << ಎಣಿಕೆ_2();
        os << ",  \"" << name() << "\"\n";
    }
    ಶೂನ್ಯ dumpAnnotate(std::ostream& os, ಚಿಹ್ನೆರಹಿತ annotateMin) ಸ್ಥಿರಾಂಕ {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << ಎಣಿಕೆ_2();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        ಆದರೆ (isCross()) {
            ಸ್ಥಿರಾಂಕ string bins = crossBins();
            ಆದರೆ (!bins.ಖಾಲಿಗಳು()) {
                os << "        //  cross: [";
                bool ಮೊದಲ = true;
                ಗಾತ್ರ_t ಆರಂಭ = 0;
                ಆಗ (true) {
                    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t comma = bins.ಕಂಡುಹಿಡಿ(',', ಆರಂಭ);
                    ಆದರೆ (!ಮೊದಲ) os << ", ";
                    os << bins.substr(ಆರಂಭ, comma == string::npos ? string::npos : comma - ಆರಂಭ);
                    ಮೊದಲ = false;
                    ಆದರೆ (comma == string::npos) ನಿಲ್ಲಿಸು;
                    ಆರಂಭ = comma + 1;
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
    using NameMap = std::map<ಸ್ಥಿರಾಂಕ std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::ಸದಿಶ<Vlc_ಬಿಂದು> m_points;  //< List of all points
    uint64_t m_ಅಂಕೆ_Points = 0;  //< Total unique points

    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ debug() { ಹಿಂತಿರುಗಿ V3Error::debugDefault(); }

ಸಾರ್ವಜನಿಕ:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { ಹಿಂತಿರುಗಿ m_nameMap.begin(); }
    ByName::iterator ಕೊನೆ() { ಹಿಂತಿರುಗಿ m_nameMap.ಕೊನೆ(); }

    // CONSTRUCTORS
    VlcPoints() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~VlcPoints() = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    ಶೂನ್ಯ dump() {
        UINFO(2, "dumpPoints...");
        Vlc_ಬಿಂದು::dump_ಶೀರ್ಷಿಕೆ(std::cout);
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : *this) {
            ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& ಬಿಂದು = ಬಿಂದು_ಸಂಖ್ಯೆ(i.ಎರಡನೇ);
            ಬಿಂದು.dump(std::cout);
        }
    }
    Vlc_ಬಿಂದು& ಬಿಂದು_ಸಂಖ್ಯೆ(uint64_t ಅಂಕೆ) { ಹಿಂತಿರುಗಿ m_points[ಅಂಕೆ]; }
    uint64_t ಕಂಡುಹಿಡಿ_ಕೂಡಿಸು_ಬಿಂದು(ಸ್ಥಿರಾಂಕ string& name, uint64_t ಎಣಿಕೆ_2) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_nameMap.emplace(name, m_ಅಂಕೆ_Points);
        ಆದರೆ (pair.ಎರಡನೇ) m_points.emplace_ಹಿಂಭಾಗ(name, m_ಅಂಕೆ_Points++);
        ಸ್ಥಿರಾಂಕ uint64_t pointnum = pair.ಮೊದಲ->ಎರಡನೇ;
        m_points[pointnum].ಎಣಿಕೆ_Inc(ಎಣಿಕೆ_2);
        ಹಿಂತಿರುಗಿ pointnum;
    }
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
