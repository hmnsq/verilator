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

#که_نه_تعریف_شوی VERILATOR_VLCPOINT_H_
#تعریف_کړه VERILATOR_VLCPOINT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <iomanip>
#ضمن <map>
#ضمن <unordered_map>
#ضمن <vector>

#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
#تعریف_کړه V3ERROR_NO_GLOBAL_
#د_که_پای
#ضمن "verilated_cov_key.h"

#ضمن "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class Vlc_ټکی final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_ټکی_عدد;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_شمېر = 0;  //< Count of hits across all tests

عام:
    // CONSTRUCTORS
    Vlc_ټکی(ثابت string& name, uint64_t ټکی_عدد)
        : m_name{name}
        , m_ټکی_عدد{ټکی_عدد} {}
    ~Vlc_ټکی() = افتراضي;
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    uint64_t ټکی_عدد() ثابت { أرجع m_ټکی_عدد; }
    uint64_t testsCovering() ثابت { أرجع m_testsCovering; }
    فراغ شمېر_Inc(uint64_t inc) { m_شمېر += inc; }
    uint64_t شمېر() ثابت { أرجع m_شمېر; }
    فراغ testsCoveringInc() { m_testsCovering++; }
    bool ok(بینښه annotateMin) ثابت {
        ثابت std::string threshStr = thresh();
        ثابت بینښه threshi = !threshStr.تشه() ? std::atoi(threshStr.c_str()) : annotateMin;
        أرجع m_شمېر >= threshi;
    }
    // KEY ACCESSORS
    string filename() ثابت { أرجع کلی_ایستل(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ثابت { أرجع کلی_ایستل(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ثابت { أرجع کلی_ایستل(VL_CIK_HIER, m_name.c_str()); }
    string پاڼه() ثابت { أرجع کلی_ایستل("page", m_name.c_str()); }
    string type() ثابت { أرجع type_ایستل(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ثابت { أرجع کلی_ایستل(VL_CIK_BIN, m_name.c_str()); }
    string binType() ثابت { أرجع کلی_ایستل(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ثابت { أرجع !کلی_ایستل(VL_CIK_CROSS, m_name.c_str()).تشه(); }
    string crossBins() ثابت { أرجع کلی_ایستل(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string ډله_وزن() ثابت { أرجع کلی_ایستل(VL_CIK_ډله_وزن, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string وزن_2() ثابت { أرجع کلی_ایستل(VL_CIK_وزن, m_name.c_str()); }
    string thresh() ثابت {
        // string as maybe ""
        أرجع کلی_ایستل(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ثابت { أرجع کلی_ایستل(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ثابت { أرجع type() == "fsm_state"; }
    bool isFsmArc() ثابت { أرجع type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ثابت { أرجع کلی_ایستل(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ثابت { أرجع کلی_ایستل(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsm_منزل_State() ثابت { أرجع کلی_ایستل(VL_CIK_FSM_منزل, m_name.c_str()); }
    string fsm_نښان() ثابت { أرجع کلی_ایستل(VL_CIK_FSM_نښان, m_name.c_str()); }
    bool isFsmResetInclude() ثابت { أرجع fsm_نښان() == "reset_include"; }
    bool isFsmResetArc() ثابت { أرجع fsm_نښان() == "reset"; }
    bool isFsmDefaultArc() ثابت { أرجع fsm_نښان() == "default"; }
    bool fsmIsReset() ثابت { أرجع isFsmResetArc() || isFsmResetInclude(); }
    صحيح lineno() ثابت {
        ثابت string کرښه_Str = کلی_ایستل(VL_CIK_LINENO, m_name.c_str());
        أرجع std::atoi(کرښه_Str.c_str());
    }
    صحيح ستنه_2() ثابت {
        ثابت string ستنه_Str = کلی_ایستل(VL_CIK_ستنه, m_name.c_str());
        أرجع std::atoi(ستنه_Str.c_str());
    }
    // METHODS
    ساكن string type_ایستل(ثابت حرف* name) { أرجع کلی_ایستل(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ساكن string کلی_ایستل(ثابت حرف* short_کلی, ثابت حرف* name) {
        // Hot function
        ثابت اندازه_t shortLen = std::مزي_اوږدوالی(short_کلی);
        لكل (ثابت حرف* cp = name; *cp; ++cp) {
            إذا (*cp == '\001') {
                إذا (0 == std::مزي_پرتله(cp + 1, short_کلی, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ثابت حرف* ep = cp;
                    طالما (*ep && *ep != '\001') ++ep;
                    أرجع Verilated_Cov_کلی::unescape(string(cp, ep - cp));
                }
            }
        }
        أرجع "";
    }
    ساكن فراغ dump_سرلیک(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    فراغ dump(std::ostream& os) ثابت {
        os << "  " << std::setw(8) << std::setfill('0') << ټکی_عدد();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << شمېر();
        os << ",  \"" << name() << "\"\n";
    }
    فراغ dumpAnnotate(std::ostream& os, بینښه annotateMin) ثابت {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << شمېر();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        إذا (isCross()) {
            ثابت string bins = crossBins();
            إذا (!bins.تشه()) {
                os << "        //  cross: [";
                bool لومړی = true;
                اندازه_t پیل = 0;
                طالما (true) {
                    ثابت اندازه_t comma = bins.پیدا_کړه(',', پیل);
                    إذا (!لومړی) os << ", ";
                    os << bins.substr(پیل, comma == string::npos ? string::npos : comma - پیل);
                    لومړی = false;
                    إذا (comma == string::npos) اكسر;
                    پیل = comma + 1;
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
    std::ویکټور<Vlc_ټکی> m_points;  //< List of all points
    uint64_t m_عدد_Points = 0;  //< Total unique points

    ساكن صحيح debug() { أرجع V3Error::debugDefault(); }

عام:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { أرجع m_nameMap.begin(); }
    ByName::iterator پای_ټکی() { أرجع m_nameMap.پای_ټکی(); }

    // CONSTRUCTORS
    VlcPoints() = افتراضي;
    ~VlcPoints() = افتراضي;

    // METHODS
    فراغ dump() {
        UINFO(2, "dumpPoints...");
        Vlc_ټکی::dump_سرلیک(std::cout);
        لكل (ثابت خپلکاری& i : *this) {
            ثابت Vlc_ټکی& ټکی = ټکی_شمېره(i.دوهم);
            ټکی.dump(std::cout);
        }
    }
    Vlc_ټکی& ټکی_شمېره(uint64_t عدد) { أرجع m_points[عدد]; }
    uint64_t پیدا_کړه_جمع_کړه_ټکی(ثابت string& name, uint64_t شمېر) {
        ثابت خپلکاری pair = m_nameMap.emplace(name, m_عدد_Points);
        إذا (pair.دوهم) m_points.emplace_شا(name, m_عدد_Points++);
        ثابت uint64_t pointnum = pair.لومړی->دوهم;
        m_points[pointnum].شمېر_Inc(شمېر);
        أرجع pointnum;
    }
};

//######################################################################

#د_که_پای  // guard
