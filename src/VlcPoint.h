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

#నిర్వచితం_కాకపోతే VERILATOR_VLCPOINT_H_
#నిర్వచించు VERILATOR_VLCPOINT_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <iomanip>
#అన్తర్భావ <map>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
#నిర్వచించు V3ERROR_NO_GLOBAL_
#అయితే_ముగింపు
#అన్తర్భావ "verilated_cov_key.h"

#అన్తర్భావ "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class Vlc_బిందువు final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_బిందువు_అంకె;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_లెక్క = 0;  //< Count of hits across all tests

బహిరంగ:
    // CONSTRUCTORS
    Vlc_బిందువు(స్థిరరాశి string& name, uint64_t బిందువు_అంకె)
        : m_name{name}
        , m_బిందువు_అంకె{బిందువు_అంకె} {}
    ~Vlc_బిందువు() = అప్రమేయ;
    // ACCESSORS
    స్థిరరాశి string& name() స్థిరరాశి { తిరిగి m_name; }
    uint64_t బిందువు_అంకె() స్థిరరాశి { తిరిగి m_బిందువు_అంకె; }
    uint64_t testsCovering() స్థిరరాశి { తిరిగి m_testsCovering; }
    శూన్య లెక్క_Inc(uint64_t inc) { m_లెక్క += inc; }
    uint64_t లెక్క() స్థిరరాశి { తిరిగి m_లెక్క; }
    శూన్య testsCoveringInc() { m_testsCovering++; }
    bool ok(గుర్తులేని annotateMin) స్థిరరాశి {
        స్థిరరాశి std::string threshStr = thresh();
        స్థిరరాశి గుర్తులేని threshi = !threshStr.ఖాళీలు() ? std::atoi(threshStr.c_str()) : annotateMin;
        తిరిగి m_లెక్క >= threshi;
    }
    // KEY ACCESSORS
    string filename() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_HIER, m_name.c_str()); }
    string పుట() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి("page", m_name.c_str()); }
    string type() స్థిరరాశి { తిరిగి type_వెలికితీయి(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_BIN, m_name.c_str()); }
    string binType() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() స్థిరరాశి { తిరిగి !కీలకం_వెలికితీయి(VL_CIK_CROSS, m_name.c_str()).ఖాళీలు(); }
    string crossBins() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string సమూహం_బరువు() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_సమూహం_బరువు, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string బరువు_2() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_బరువు, m_name.c_str()); }
    string thresh() స్థిరరాశి {
        // string as maybe ""
        తిరిగి కీలకం_వెలికితీయి(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() స్థిరరాశి { తిరిగి type() == "fsm_state"; }
    bool isFsmArc() స్థిరరాశి { తిరిగి type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsm_గమ్యం_State() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_FSM_గమ్యం, m_name.c_str()); }
    string fsm_ట్యాగ్() స్థిరరాశి { తిరిగి కీలకం_వెలికితీయి(VL_CIK_FSM_ట్యాగ్, m_name.c_str()); }
    bool isFsmResetInclude() స్థిరరాశి { తిరిగి fsm_ట్యాగ్() == "reset_include"; }
    bool isFsmResetArc() స్థిరరాశి { తిరిగి fsm_ట్యాగ్() == "reset"; }
    bool isFsmDefaultArc() స్థిరరాశి { తిరిగి fsm_ట్యాగ్() == "default"; }
    bool fsmIsReset() స్థిరరాశి { తిరిగి isFsmResetArc() || isFsmResetInclude(); }
    పూర్ణాంకం lineno() స్థిరరాశి {
        స్థిరరాశి string పంక్తి_Str = కీలకం_వెలికితీయి(VL_CIK_LINENO, m_name.c_str());
        తిరిగి std::atoi(పంక్తి_Str.c_str());
    }
    పూర్ణాంకం స్తంభం_2() స్థిరరాశి {
        స్థిరరాశి string స్తంభం_Str = కీలకం_వెలికితీయి(VL_CIK_స్తంభం, m_name.c_str());
        తిరిగి std::atoi(స్తంభం_Str.c_str());
    }
    // METHODS
    స్థిర string type_వెలికితీయి(స్థిరరాశి అక్షరం* name) { తిరిగి కీలకం_వెలికితీయి(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    స్థిర string కీలకం_వెలికితీయి(స్థిరరాశి అక్షరం* short_కీలకం, స్థిరరాశి అక్షరం* name) {
        // Hot function
        స్థిరరాశి కొలత_t shortLen = std::అక్షరశ్రేణి_పొడవు(short_కీలకం);
        కొరకు (స్థిరరాశి అక్షరం* cp = name; *cp; ++cp) {
            ఒకవేళ (*cp == '\001') {
                ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(cp + 1, short_కీలకం, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    స్థిరరాశి అక్షరం* ep = cp;
                    అయితే (*ep && *ep != '\001') ++ep;
                    తిరిగి Verilated_Cov_కీలకం::unescape(string(cp, ep - cp));
                }
            }
        }
        తిరిగి "";
    }
    స్థిర శూన్య dump_శీర్షిక(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    శూన్య dump(std::ostream& os) స్థిరరాశి {
        os << "  " << std::setw(8) << std::setfill('0') << బిందువు_అంకె();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << లెక్క();
        os << ",  \"" << name() << "\"\n";
    }
    శూన్య dumpAnnotate(std::ostream& os, గుర్తులేని annotateMin) స్థిరరాశి {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << లెక్క();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        ఒకవేళ (isCross()) {
            స్థిరరాశి string bins = crossBins();
            ఒకవేళ (!bins.ఖాళీలు()) {
                os << "        //  cross: [";
                bool మొదటి = true;
                కొలత_t ప్రారంభం = 0;
                అయితే (true) {
                    స్థిరరాశి కొలత_t comma = bins.కనుగొను(',', ప్రారంభం);
                    ఒకవేళ (!మొదటి) os << ", ";
                    os << bins.substr(ప్రారంభం, comma == string::npos ? string::npos : comma - ప్రారంభం);
                    మొదటి = false;
                    ఒకవేళ (comma == string::npos) ఆపు;
                    ప్రారంభం = comma + 1;
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
    using NameMap = std::map<స్థిరరాశి std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::సదిశ<Vlc_బిందువు> m_points;  //< List of all points
    uint64_t m_అంకె_Points = 0;  //< Total unique points

    స్థిర పూర్ణాంకం debug() { తిరిగి V3Error::debugDefault(); }

బహిరంగ:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { తిరిగి m_nameMap.begin(); }
    ByName::iterator చివర() { తిరిగి m_nameMap.చివర(); }

    // CONSTRUCTORS
    VlcPoints() = అప్రమేయ;
    ~VlcPoints() = అప్రమేయ;

    // METHODS
    శూన్య dump() {
        UINFO(2, "dumpPoints...");
        Vlc_బిందువు::dump_శీర్షిక(std::cout);
        కొరకు (స్థిరరాశి స్వయంచాలక& i : *this) {
            స్థిరరాశి Vlc_బిందువు& బిందువు = బిందువు_సంఖ్య(i.రెండవ);
            బిందువు.dump(std::cout);
        }
    }
    Vlc_బిందువు& బిందువు_సంఖ్య(uint64_t అంకె) { తిరిగి m_points[అంకె]; }
    uint64_t కనుగొను_కూడు_బిందువు(స్థిరరాశి string& name, uint64_t లెక్క) {
        స్థిరరాశి స్వయంచాలక pair = m_nameMap.emplace(name, m_అంకె_Points);
        ఒకవేళ (pair.రెండవ) m_points.emplace_వెనుక(name, m_అంకె_Points++);
        స్థిరరాశి uint64_t pointnum = pair.మొదటి->రెండవ;
        m_points[pointnum].లెక్క_Inc(లెక్క);
        తిరిగి pointnum;
    }
};

//######################################################################

#అయితే_ముగింపు  // guard
