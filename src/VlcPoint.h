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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_VLCPOINT_H_
#وصف_ڪريو VERILATOR_VLCPOINT_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <iomanip>
#ضمن <map>
#ضمن <unordered_map>
#ضمن <vector>

#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
#وصف_ڪريو V3ERROR_NO_GLOBAL_
#جيڪڏهن_پورو
#ضمن "verilated_cov_key.h"

#ضمن "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class Vlc_نقطو final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_نقطو_انگ;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_ڳڻپ = 0;  //< Count of hits across all tests

عام:
    // CONSTRUCTORS
    Vlc_نقطو(ثابت string& name, uint64_t نقطو_انگ)
        : m_name{name}
        , m_نقطو_انگ{نقطو_انگ} {}
    ~Vlc_نقطو() = افتراضي;
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    uint64_t نقطو_انگ() ثابت { أرجع m_نقطو_انگ; }
    uint64_t testsCovering() ثابت { أرجع m_testsCovering; }
    فراغ ڳڻپ_Inc(uint64_t inc) { m_ڳڻپ += inc; }
    uint64_t ڳڻپ() ثابت { أرجع m_ڳڻپ; }
    فراغ testsCoveringInc() { m_testsCovering++; }
    bool ok(بنانشان annotateMin) ثابت {
        ثابت std::string threshStr = thresh();
        ثابت بنانشان threshi = !threshStr.خالي_جايون() ? std::atoi(threshStr.c_str()) : annotateMin;
        أرجع m_ڳڻپ >= threshi;
    }
    // KEY ACCESSORS
    string filename() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_HIER, m_name.c_str()); }
    string صفحو() ثابت { أرجع ڪنجي_ڪڍو("page", m_name.c_str()); }
    string type() ثابت { أرجع type_ڪڍو(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_BIN, m_name.c_str()); }
    string binType() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() ثابت { أرجع !ڪنجي_ڪڍو(VL_CIK_CROSS, m_name.c_str()).خالي_جايون(); }
    string crossBins() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string گروهه_وزن() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_گروهه_وزن, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string وزن_2() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_وزن, m_name.c_str()); }
    string thresh() ثابت {
        // string as maybe ""
        أرجع ڪنجي_ڪڍو(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() ثابت { أرجع type() == "fsm_state"; }
    bool isFsmArc() ثابت { أرجع type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsm_منزل_State() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_FSM_منزل, m_name.c_str()); }
    string fsm_ٽيگ() ثابت { أرجع ڪنجي_ڪڍو(VL_CIK_FSM_ٽيگ, m_name.c_str()); }
    bool isFsmResetInclude() ثابت { أرجع fsm_ٽيگ() == "reset_include"; }
    bool isFsmResetArc() ثابت { أرجع fsm_ٽيگ() == "reset"; }
    bool isFsmDefaultArc() ثابت { أرجع fsm_ٽيگ() == "default"; }
    bool fsmIsReset() ثابت { أرجع isFsmResetArc() || isFsmResetInclude(); }
    صحيح lineno() ثابت {
        ثابت string سٽ_Str = ڪنجي_ڪڍو(VL_CIK_LINENO, m_name.c_str());
        أرجع std::atoi(سٽ_Str.c_str());
    }
    صحيح ڪالم_2() ثابت {
        ثابت string ڪالم_Str = ڪنجي_ڪڍو(VL_CIK_ڪالم, m_name.c_str());
        أرجع std::atoi(ڪالم_Str.c_str());
    }
    // METHODS
    ساكن string type_ڪڍو(ثابت حرف* name) { أرجع ڪنجي_ڪڍو(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    ساكن string ڪنجي_ڪڍو(ثابت حرف* short_ڪنجي, ثابت حرف* name) {
        // Hot function
        ثابت سائيز_t shortLen = std::لڙي_ڊيگهه(short_ڪنجي);
        لكل (ثابت حرف* cp = name; *cp; ++cp) {
            إذا (*cp == '\001') {
                إذا (0 == std::لڙي_ڀيٽ(cp + 1, short_ڪنجي, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    ثابت حرف* ep = cp;
                    طالما (*ep && *ep != '\001') ++ep;
                    أرجع Verilated_Cov_ڪنجي::unescape(string(cp, ep - cp));
                }
            }
        }
        أرجع "";
    }
    ساكن فراغ dump_سرو(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    فراغ dump(std::ostream& os) ثابت {
        os << "  " << std::setw(8) << std::setfill('0') << نقطو_انگ();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << ڳڻپ();
        os << ",  \"" << name() << "\"\n";
    }
    فراغ dumpAnnotate(std::ostream& os, بنانشان annotateMin) ثابت {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << ڳڻپ();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        إذا (isCross()) {
            ثابت string bins = crossBins();
            إذا (!bins.خالي_جايون()) {
                os << "        //  cross: [";
                bool پهريون = true;
                سائيز_t شروعات = 0;
                طالما (true) {
                    ثابت سائيز_t comma = bins.ڳوليو(',', شروعات);
                    إذا (!پهريون) os << ", ";
                    os << bins.substr(شروعات, comma == string::npos ? string::npos : comma - شروعات);
                    پهريون = false;
                    إذا (comma == string::npos) اكسر;
                    شروعات = comma + 1;
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
    std::ويڪٽر<Vlc_نقطو> m_points;  //< List of all points
    uint64_t m_انگ_Points = 0;  //< Total unique points

    ساكن صحيح debug() { أرجع V3Error::debugDefault(); }

عام:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { أرجع m_nameMap.begin(); }
    ByName::iterator پڇاڙي() { أرجع m_nameMap.پڇاڙي(); }

    // CONSTRUCTORS
    VlcPoints() = افتراضي;
    ~VlcPoints() = افتراضي;

    // METHODS
    فراغ dump() {
        UINFO(2, "dumpPoints...");
        Vlc_نقطو::dump_سرو(std::cout);
        لكل (ثابت خودڪار& i : *this) {
            ثابت Vlc_نقطو& نقطو = نقطو_عدد(i.ٻيو);
            نقطو.dump(std::cout);
        }
    }
    Vlc_نقطو& نقطو_عدد(uint64_t انگ) { أرجع m_points[انگ]; }
    uint64_t ڳوليو_جوڙيو_نقطو(ثابت string& name, uint64_t ڳڻپ) {
        ثابت خودڪار pair = m_nameMap.emplace(name, m_انگ_Points);
        إذا (pair.ٻيو) m_points.emplace_پٺ(name, m_انگ_Points++);
        ثابت uint64_t pointnum = pair.پهريون->ٻيو;
        m_points[pointnum].ڳڻپ_Inc(ڳڻپ);
        أرجع pointnum;
    }
};

//######################################################################

#جيڪڏهن_پورو  // guard
