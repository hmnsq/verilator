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

#جے_بنا_تعریف VERILATOR_VLCPOINT_H_
#تعریف_کرو VERILATOR_VLCPOINT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <iomanip>
#ضمن <map>
#ضمن <unordered_map>
#ضمن <vector>

#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
#تعریف_کرو V3ERROR_NO_GLOBAL_
#جے_انت
#ضمن "verilated_cov_key.h"

#ضمن "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class Vlc_نقطہ final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_نقطہ_عدد;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_گنتی = 0;  //< Count of hits across all tests

عام:
    // CONSTRUCTORS
    Vlc_نقطہ(ثابت string& name, uint64_t نقطہ_عدد)
        : m_name{name}
        , m_نقطہ_عدد{نقطہ_عدد} {}
    ~Vlc_نقطہ() = افتراضي;
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    uint64_t نقطہ_عدد() ثابت { أرجع m_نقطہ_عدد; }
    uint64_t testsCovering() ثابت { أرجع m_testsCovering; }
    فراغ گنتی_Inc(uint64_t inc) { m_گنتی += inc; }
    uint64_t گنتی() ثابت { أرجع m_گنتی; }
    فراغ testsCoveringInc() { m_testsCovering++; }
    bool ok(بنانشانی annotateMin) ثابت {
        ثابت std::string threshStr = thresh();
        ثابت بنانشانی threshi = !threshStr.خالی_تھاواں() ? std::atoi(threshStr.c_str()) : annotateMin;
        أرجع m_گنتی >= threshi;
    }
    // KEY ACCESSORS
    string filename() ثابت { أرجع کلید_کڈھو(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ثابت { أرجع کلید_کڈھو(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ثابت { أرجع کلید_کڈھو(VL_CIK_HIER, m_name.c_str()); }
    string ورقہ() ثابت { أرجع کلید_کڈھو("page", m_name.c_str()); }
    string type() ثابت { أرجع type_کڈھو(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ثابت { أرجع کلید_کڈھو(VL_CIK_BIN, m_name.c_str()); }
    string binType() ثابت { أرجع کلید_کڈھو(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ثابت { أرجع !کلید_کڈھو(VL_CIK_CROSS, m_name.c_str()).خالی_تھاواں(); }
    string crossBins() ثابت { أرجع کلید_کڈھو(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string گروہ_بھار() ثابت { أرجع کلید_کڈھو(VL_CIK_گروہ_بھار, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string بھار_2() ثابت { أرجع کلید_کڈھو(VL_CIK_بھار, m_name.c_str()); }
    string thresh() ثابت {
        // string as maybe ""
        أرجع کلید_کڈھو(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ثابت { أرجع کلید_کڈھو(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ثابت { أرجع type() == "fsm_state"; }
    bool isFsmArc() ثابت { أرجع type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ثابت { أرجع کلید_کڈھو(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ثابت { أرجع کلید_کڈھو(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsm_منزل_State() ثابت { أرجع کلید_کڈھو(VL_CIK_FSM_منزل, m_name.c_str()); }
    string fsm_ٹیگ() ثابت { أرجع کلید_کڈھو(VL_CIK_FSM_ٹیگ, m_name.c_str()); }
    bool isFsmResetInclude() ثابت { أرجع fsm_ٹیگ() == "reset_include"; }
    bool isFsmResetArc() ثابت { أرجع fsm_ٹیگ() == "reset"; }
    bool isFsmDefaultArc() ثابت { أرجع fsm_ٹیگ() == "default"; }
    bool fsmIsReset() ثابت { أرجع isFsmResetArc() || isFsmResetInclude(); }
    صحيح lineno() ثابت {
        ثابت string سطر_Str = کلید_کڈھو(VL_CIK_LINENO, m_name.c_str());
        أرجع std::atoi(سطر_Str.c_str());
    }
    صحيح کالم_2() ثابت {
        ثابت string کالم_Str = کلید_کڈھو(VL_CIK_کالم, m_name.c_str());
        أرجع std::atoi(کالم_Str.c_str());
    }
    // METHODS
    ساكن string type_کڈھو(ثابت حرف* name) { أرجع کلید_کڈھو(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ساكن string کلید_کڈھو(ثابت حرف* short_کلید, ثابت حرف* name) {
        // Hot function
        ثابت ناپ_t shortLen = std::لڑی_لمبائی(short_کلید);
        لكل (ثابت حرف* cp = name; *cp; ++cp) {
            إذا (*cp == '\001') {
                إذا (0 == std::لڑی_مقابلہ(cp + 1, short_کلید, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ثابت حرف* ep = cp;
                    طالما (*ep && *ep != '\001') ++ep;
                    أرجع Verilated_Cov_کلید::unescape(string(cp, ep - cp));
                }
            }
        }
        أرجع "";
    }
    ساكن فراغ dump_سرخی(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    فراغ dump(std::ostream& os) ثابت {
        os << "  " << std::setw(8) << std::setfill('0') << نقطہ_عدد();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << گنتی();
        os << ",  \"" << name() << "\"\n";
    }
    فراغ dumpAnnotate(std::ostream& os, بنانشانی annotateMin) ثابت {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << گنتی();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        إذا (isCross()) {
            ثابت string bins = crossBins();
            إذا (!bins.خالی_تھاواں()) {
                os << "        //  cross: [";
                bool پہلا = true;
                ناپ_t شروع = 0;
                طالما (true) {
                    ثابت ناپ_t comma = bins.لبھو(',', شروع);
                    إذا (!پہلا) os << ", ";
                    os << bins.substr(شروع, comma == string::npos ? string::npos : comma - شروع);
                    پہلا = false;
                    إذا (comma == string::npos) اكسر;
                    شروع = comma + 1;
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

    ساكن صحيح debug() { أرجع V3Error::debugDefault(); }

عام:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { أرجع m_nameMap.begin(); }
    ByName::iterator سرا() { أرجع m_nameMap.سرا(); }

    // CONSTRUCTORS
    VlcPoints() = افتراضي;
    ~VlcPoints() = افتراضي;

    // METHODS
    فراغ dump() {
        UINFO(2, "dumpPoints...");
        Vlc_نقطہ::dump_سرخی(std::cout);
        لكل (ثابت خودکار& i : *this) {
            ثابت Vlc_نقطہ& نقطہ = نقطہ_نمبر(i.دوجا);
            نقطہ.dump(std::cout);
        }
    }
    Vlc_نقطہ& نقطہ_نمبر(uint64_t عدد) { أرجع m_points[عدد]; }
    uint64_t لبھو_جوڑو_نقطہ(ثابت string& name, uint64_t گنتی) {
        ثابت خودکار pair = m_nameMap.emplace(name, m_عدد_Points);
        إذا (pair.دوجا) m_points.emplace_پٹھ(name, m_عدد_Points++);
        ثابت uint64_t pointnum = pair.پہلا->دوجا;
        m_points[pointnum].گنتی_Inc(گنتی);
        أرجع pointnum;
    }
};

//######################################################################

#جے_انت  // guard
