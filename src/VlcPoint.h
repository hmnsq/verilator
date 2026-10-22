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

#اگر_تعریف_نشده VERILATOR_VLCPOINT_H_
#تعریف_کن VERILATOR_VLCPOINT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <iomanip>
#ضمن <map>
#ضمن <unordered_map>
#ضمن <vector>

#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
#تعریف_کن V3ERROR_NO_GLOBAL_
#پایان_اگر
#ضمن "verilated_cov_key.h"

#ضمن "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class Vlc_نقطه final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_نقطه_رقم;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_شمار = 0;  //< Count of hits across all tests

عام:
    // CONSTRUCTORS
    Vlc_نقطه(ثابت string& name, uint64_t نقطه_رقم)
        : m_name{name}
        , m_نقطه_رقم{نقطه_رقم} {}
    ~Vlc_نقطه() = افتراضي;
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    uint64_t نقطه_رقم() ثابت { أرجع m_نقطه_رقم; }
    uint64_t testsCovering() ثابت { أرجع m_testsCovering; }
    فراغ شمار_Inc(uint64_t inc) { m_شمار += inc; }
    uint64_t شمار() ثابت { أرجع m_شمار; }
    فراغ testsCoveringInc() { m_testsCovering++; }
    bool ok(بیعلامت annotateMin) ثابت {
        ثابت std::string threshStr = thresh();
        ثابت بیعلامت threshi = !threshStr.خالی() ? std::atoi(threshStr.c_str()) : annotateMin;
        أرجع m_شمار >= threshi;
    }
    // KEY ACCESSORS
    string filename() ثابت { أرجع کلید_استخراج(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ثابت { أرجع کلید_استخراج(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ثابت { أرجع کلید_استخراج(VL_CIK_HIER, m_name.c_str()); }
    string صفحه() ثابت { أرجع کلید_استخراج("page", m_name.c_str()); }
    string type() ثابت { أرجع type_استخراج(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ثابت { أرجع کلید_استخراج(VL_CIK_BIN, m_name.c_str()); }
    string binType() ثابت { أرجع کلید_استخراج(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ثابت { أرجع !کلید_استخراج(VL_CIK_CROSS, m_name.c_str()).خالی(); }
    string crossBins() ثابت { أرجع کلید_استخراج(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string گروه_وزن() ثابت { أرجع کلید_استخراج(VL_CIK_گروه_وزن, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string وزن_2() ثابت { أرجع کلید_استخراج(VL_CIK_وزن, m_name.c_str()); }
    string thresh() ثابت {
        // string as maybe ""
        أرجع کلید_استخراج(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ثابت { أرجع کلید_استخراج(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ثابت { أرجع type() == "fsm_state"; }
    bool isFsmArc() ثابت { أرجع type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ثابت { أرجع کلید_استخراج(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ثابت { أرجع کلید_استخراج(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsm_مقصد_State() ثابت { أرجع کلید_استخراج(VL_CIK_FSM_مقصد, m_name.c_str()); }
    string fsm_برچسب() ثابت { أرجع کلید_استخراج(VL_CIK_FSM_برچسب, m_name.c_str()); }
    bool isFsmResetInclude() ثابت { أرجع fsm_برچسب() == "reset_include"; }
    bool isFsmResetArc() ثابت { أرجع fsm_برچسب() == "reset"; }
    bool isFsmDefaultArc() ثابت { أرجع fsm_برچسب() == "default"; }
    bool fsmIsReset() ثابت { أرجع isFsmResetArc() || isFsmResetInclude(); }
    صحيح lineno() ثابت {
        ثابت string سطر_Str = کلید_استخراج(VL_CIK_LINENO, m_name.c_str());
        أرجع std::atoi(سطر_Str.c_str());
    }
    صحيح ستونی_2() ثابت {
        ثابت string ستونی_Str = کلید_استخراج(VL_CIK_ستونی, m_name.c_str());
        أرجع std::atoi(ستونی_Str.c_str());
    }
    // METHODS
    ساكن string type_استخراج(ثابت حرف* name) { أرجع کلید_استخراج(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ساكن string کلید_استخراج(ثابت حرف* short_کلید, ثابت حرف* name) {
        // Hot function
        ثابت اندازه_t shortLen = std::طول_رشته(short_کلید);
        لكل (ثابت حرف* cp = name; *cp; ++cp) {
            إذا (*cp == '\001') {
                إذا (0 == std::مقایسه_رشته(cp + 1, short_کلید, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ثابت حرف* ep = cp;
                    طالما (*ep && *ep != '\001') ++ep;
                    أرجع Verilated_Cov_کلید::unescape(string(cp, ep - cp));
                }
            }
        }
        أرجع "";
    }
    ساكن فراغ dump_سرآیند(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    فراغ dump(std::ostream& os) ثابت {
        os << "  " << std::setw(8) << std::setfill('0') << نقطه_رقم();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << شمار();
        os << ",  \"" << name() << "\"\n";
    }
    فراغ dumpAnnotate(std::ostream& os, بیعلامت annotateMin) ثابت {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << شمار();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        إذا (isCross()) {
            ثابت string bins = crossBins();
            إذا (!bins.خالی()) {
                os << "        //  cross: [";
                bool اولی = true;
                اندازه_t شروع = 0;
                طالما (true) {
                    ثابت اندازه_t comma = bins.پیدا_کن(',', شروع);
                    إذا (!اولی) os << ", ";
                    os << bins.substr(شروع, comma == string::npos ? string::npos : comma - شروع);
                    اولی = false;
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
    std::وکتور<Vlc_نقطه> m_points;  //< List of all points
    uint64_t m_رقم_Points = 0;  //< Total unique points

    ساكن صحيح debug() { أرجع V3Error::debugDefault(); }

عام:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { أرجع m_nameMap.begin(); }
    ByName::iterator انتها() { أرجع m_nameMap.انتها(); }

    // CONSTRUCTORS
    VlcPoints() = افتراضي;
    ~VlcPoints() = افتراضي;

    // METHODS
    فراغ dump() {
        UINFO(2, "dumpPoints...");
        Vlc_نقطه::dump_سرآیند(std::cout);
        لكل (ثابت خودکار& i : *this) {
            ثابت Vlc_نقطه& نقطه = نقطه_عدد(i.دومی);
            نقطه.dump(std::cout);
        }
    }
    Vlc_نقطه& نقطه_عدد(uint64_t رقم) { أرجع m_points[رقم]; }
    uint64_t پیدا_کن_جمع_کن_نقطه(ثابت string& name, uint64_t شمار) {
        ثابت خودکار pair = m_nameMap.emplace(name, m_رقم_Points);
        إذا (pair.دومی) m_points.emplace_پشت(name, m_رقم_Points++);
        ثابت uint64_t pointnum = pair.اولی->دومی;
        m_points[pointnum].شمار_Inc(شمار);
        أرجع pointnum;
    }
};

//######################################################################

#پایان_اگر  // guard
