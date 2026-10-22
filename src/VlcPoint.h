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

#اگر_غیر_تعریف_شدہ VERILATOR_VLCPOINT_H_
#تعریف_کرو VERILATOR_VLCPOINT_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <iomanip>
#شامل <map>
#شامل <unordered_map>
#شامل <vector>

#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
#تعریف_کرو V3ERROR_NO_GLOBAL_
#اگر_ختم
#شامل "verilated_cov_key.h"

#شامل "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class Vlc_نقطہ final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_نقطہ_عدد;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_گنتی = 0;  //< Count of hits across all tests

عوامی:
    // CONSTRUCTORS
    Vlc_نقطہ(ثابت string& name, uint64_t نقطہ_عدد)
        : m_name{name}
        , m_نقطہ_عدد{نقطہ_عدد} {}
    ~Vlc_نقطہ() = طےشدہ;
    // ACCESSORS
    ثابت string& name() ثابت { واپس m_name; }
    uint64_t نقطہ_عدد() ثابت { واپس m_نقطہ_عدد; }
    uint64_t testsCovering() ثابت { واپس m_testsCovering; }
    خالی گنتی_Inc(uint64_t inc) { m_گنتی += inc; }
    uint64_t گنتی() ثابت { واپس m_گنتی; }
    خالی testsCoveringInc() { m_testsCovering++; }
    bool ok(غیرعلامتی annotateMin) ثابت {
        ثابت std::string threshStr = thresh();
        ثابت غیرعلامتی threshi = !threshStr.خالی_خانے() ? std::atoi(threshStr.c_str()) : annotateMin;
        واپس m_گنتی >= threshi;
    }
    // KEY ACCESSORS
    string filename() ثابت { واپس کلید_نکالو(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ثابت { واپس کلید_نکالو(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ثابت { واپس کلید_نکالو(VL_CIK_HIER, m_name.c_str()); }
    string صفحہ() ثابت { واپس کلید_نکالو("page", m_name.c_str()); }
    string type() ثابت { واپس type_نکالو(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ثابت { واپس کلید_نکالو(VL_CIK_BIN, m_name.c_str()); }
    string binType() ثابت { واپس کلید_نکالو(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ثابت { واپس !کلید_نکالو(VL_CIK_CROSS, m_name.c_str()).خالی_خانے(); }
    string crossBins() ثابت { واپس کلید_نکالو(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string گروہ_وزن() ثابت { واپس کلید_نکالو(VL_CIK_گروہ_وزن, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string وزن_2() ثابت { واپس کلید_نکالو(VL_CIK_وزن, m_name.c_str()); }
    string thresh() ثابت {
        // string as maybe ""
        واپس کلید_نکالو(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ثابت { واپس کلید_نکالو(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ثابت { واپس type() == "fsm_state"; }
    bool isFsmArc() ثابت { واپس type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ثابت { واپس کلید_نکالو(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ثابت { واپس کلید_نکالو(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsm_مقصد_State() ثابت { واپس کلید_نکالو(VL_CIK_FSM_مقصد, m_name.c_str()); }
    string fsm_لیبل() ثابت { واپس کلید_نکالو(VL_CIK_FSM_لیبل, m_name.c_str()); }
    bool isFsmResetInclude() ثابت { واپس fsm_لیبل() == "reset_include"; }
    bool isFsmResetArc() ثابت { واپس fsm_لیبل() == "reset"; }
    bool isFsmDefaultArc() ثابت { واپس fsm_لیبل() == "default"; }
    bool fsmIsReset() ثابت { واپس isFsmResetArc() || isFsmResetInclude(); }
    صحیح lineno() ثابت {
        ثابت string سطر_Str = کلید_نکالو(VL_CIK_LINENO, m_name.c_str());
        واپس std::atoi(سطر_Str.c_str());
    }
    صحیح عمود_2() ثابت {
        ثابت string عمود_Str = کلید_نکالو(VL_CIK_عمود, m_name.c_str());
        واپس std::atoi(عمود_Str.c_str());
    }
    // METHODS
    ساکن string type_نکالو(ثابت حرف* name) { واپس کلید_نکالو(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ساکن string کلید_نکالو(ثابت حرف* short_کلید, ثابت حرف* name) {
        // Hot function
        ثابت سائز_t shortLen = std::لڑی_لمبائی(short_کلید);
        برائے (ثابت حرف* cp = name; *cp; ++cp) {
            اگر (*cp == '\001') {
                اگر (0 == std::لڑی_موازنہ(cp + 1, short_کلید, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ثابت حرف* ep = cp;
                    جب (*ep && *ep != '\001') ++ep;
                    واپس Verilated_Cov_کلید::unescape(string(cp, ep - cp));
                }
            }
        }
        واپس "";
    }
    ساکن خالی dump_سرخی(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    خالی dump(std::ostream& os) ثابت {
        os << "  " << std::setw(8) << std::setfill('0') << نقطہ_عدد();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << گنتی();
        os << ",  \"" << name() << "\"\n";
    }
    خالی dumpAnnotate(std::ostream& os, غیرعلامتی annotateMin) ثابت {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << گنتی();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        اگر (isCross()) {
            ثابت string bins = crossBins();
            اگر (!bins.خالی_خانے()) {
                os << "        //  cross: [";
                bool پہلا = true;
                سائز_t آغاز = 0;
                جب (true) {
                    ثابت سائز_t comma = bins.ڈھونڈو(',', آغاز);
                    اگر (!پہلا) os << ", ";
                    os << bins.substr(آغاز, comma == string::npos ? string::npos : comma - آغاز);
                    پہلا = false;
                    اگر (comma == string::npos) توڑو;
                    آغاز = comma + 1;
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
    using NameMap = std::map<ثابت std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::سمتیہ<Vlc_نقطہ> m_points;  //< List of all points
    uint64_t m_عدد_Points = 0;  //< Total unique points

    ساکن صحیح debug() { واپس V3Error::debugDefault(); }

عوامی:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { واپس m_nameMap.begin(); }
    ByName::iterator انتہا() { واپس m_nameMap.انتہا(); }

    // CONSTRUCTORS
    VlcPoints() = طےشدہ;
    ~VlcPoints() = طےشدہ;

    // METHODS
    خالی dump() {
        UINFO(2, "dumpPoints...");
        Vlc_نقطہ::dump_سرخی(std::cout);
        برائے (ثابت خودکار& i : *this) {
            ثابت Vlc_نقطہ& نقطہ = نقطہ_نمبر(i.دوسرا);
            نقطہ.dump(std::cout);
        }
    }
    Vlc_نقطہ& نقطہ_نمبر(uint64_t عدد) { واپس m_points[عدد]; }
    uint64_t ڈھونڈو_جوڑو_نقطہ(ثابت string& name, uint64_t گنتی) {
        ثابت خودکار pair = m_nameMap.emplace(name, m_عدد_Points);
        اگر (pair.دوسرا) m_points.emplace_عقب(name, m_عدد_Points++);
        ثابت uint64_t pointnum = pair.پہلا->دوسرا;
        m_points[pointnum].گنتی_Inc(گنتی);
        واپس pointnum;
    }
};

//######################################################################

#اگر_ختم  // guard
