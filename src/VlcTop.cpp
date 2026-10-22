// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: top implementation
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

#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

#ضمن "VlcTop.h"

#ضمن "V3Error.h"
#ضمن "V3Os.h"

#ضمن "VlcOptions.h"

#ضمن <algorithm>
#ضمن <fstream>
#ضمن <iomanip>
#ضمن <map>
#ضمن <set>
#ضمن <sstream>
#ضمن <string>
#ضمن <vector>

//######################################################################

نطاق {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
صنف Tally final {
    // MEMBERS
    uint64_t m_موندنه = 0;  // Covered points
    uint64_t m_ټول = 0;  // Points
    منطقي m_scored = خطأ;  // Coverage is m_score, not the ratio of the points
    مزدوج m_score = 0.0;  // Percent coverage, if m_scored

عام:
    // ACCESSORS
    uint64_t موندنه() ثابت { أرجع m_موندنه; }
    uint64_t ټول() ثابت { أرجع m_ټول; }
    منطقي scored() ثابت { أرجع m_scored; }
    مزدوج score() ثابت { أرجع m_score; }
    فراغ score(مزدوج ارزښت) {
        m_scored = ریښتیا;
        m_score = ارزښت;
    }
    // METHODS
    فراغ جمع_کړه_ټکی(منطقي covered) {
        إذا (covered) ++m_موندنه;
        ++m_ټول;
    }
    فراغ جمع_کړه_Points(ثابت Tally& other) {
        m_موندنه += other.m_موندنه;
        m_ټول += other.m_ټول;
    }
};

// Map coverage type to its tally.
کارول TypeTally = std::map<std::string, Tally>;

ساكن constexpr ثابت حرف* ثابت s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ساكن constexpr ثابت حرف* ثابت s_covergroupType = "covergroup";
ساكن constexpr اندازه_t s_summaryIndent = 2;
ساكن constexpr اندازه_t s_report_لیکه_Indent = 4;

string displayType(ثابت Vlc_ټکی& ټکی) {
    ثابت string type = ټکی.type();
    أرجع type.تشه() ? "point" : type;
}

منطقي isCollapsedHier(ثابت string& hier) {
    أرجع hier.پیدا_کړه('*') != string::npos || hier.پیدا_کړه('?') != string::npos;
}

منطقي isOrderedType(ثابت string& type) {
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        إذا (type == typep) أرجع ریښتیا;
    }
    أرجع خطأ;
}

string reportHier(ثابت Vlc_ټکی& ټکی) {
    // FSM records currently have useful instance scope in Fv.
    إذا (ټکی.isFsmState() || ټکی.isFsmArc()) {
        ثابت string fsmVar = ټکی.fsmVarName();
        أرجع fsmVar.substr(0, fsmVar.rfind('.'));
    }
    أرجع ټکی.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::ویکټور<string> وېشل_Hier(ثابت string& hier) {
    std::ویکټور<string> parts;
    string::اندازه_type پیل = 0;
    صحيح ژوروالی = 0;  // Of the parentheses of the values of parameters
    string::اندازه_type pos = 0;
    طالما (pos < hier.اندازه()) {
        ثابت حرف c = hier[pos];
        إذا (c == '\\') {
            pos = hier.پیدا_کړه(' ', pos);  // npos if not ended
            استمر;
        } وإلا إذا (!ژوروالی && hier.compare(pos, 2, "#(") == 0) {
            ژوروالی = 1;
            ++pos;
        } وإلا إذا (!ژوروالی && c == '.') {
            parts.ټېل_شا(hier.substr(پیل, pos - پیل));
            پیل = pos + 1;
        } وإلا إذا (ژوروالی && c == '"') {
            pos = VString::quoted_پای_ټکی(hier, pos);  // npos if not terminated
            استمر;
        } وإلا إذا (ژوروالی && c == '(') {
            ++ژوروالی;
        } وإلا إذا (ژوروالی && c == ')') {
            --ژوروالی;
        }
        ++pos;
    }
    إذا (ژوروالی || pos != hier.اندازه()) {  // Unbalanced
        parts.clear();
        پیل = 0;
        لكل (string::اندازه_type dot; (dot = hier.پیدا_کړه('.', پیل)) != string::npos;
             پیل = dot + 1) {
            parts.ټېل_شا(hier.substr(پیل, dot - پیل));
        }
    }
    parts.ټېل_شا(hier.substr(پیل));
    أرجع parts;
}

string duName(ثابت Vlc_ټکی& ټکی) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ثابت string پاڼه = ټکی.پاڼه();
    أرجع پاڼه.substr(پاڼه.پیدا_کړه('/') + 1);
}

فراغ tally_ټکی(TypeTally& tally, ثابت string& type, uint64_t شمېر) {
    tally[type].جمع_کړه_ټکی(شمېر > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
مزدوج pct(uint64_t موندنه, uint64_t ټول) {
    أرجع ټول ? (100.0 * static_cast<مزدوج>(موندنه) / static_cast<مزدوج>(ټول)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_ارزښت_String(مزدوج ارزښت, منطقي complete) {
    إذا (!complete && ارزښت > 99.9) ارزښت = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << ارزښت << "%";
    أرجع os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t موندنه, uint64_t ټول) {
    أرجع pct_ارزښت_String(pct(موندنه, ټول), موندنه >= ټول);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
صنف CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_شمېر = 0;  // Hits, of all the records
    uint64_t m_at_لږترلږه = 0;  // option.at_least, the largest of the records

عام:
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    منطقي covered() ثابت { أرجع m_شمېر >= m_at_لږترلږه; }
    // METHODS
    فراغ جمع_کړه_ریکارډ(ثابت string& name, uint64_t شمېر, uint64_t at_لږترلږه) {
        m_name = name;
        m_شمېر += شمېر;
        m_at_لږترلږه = std::max(m_at_لږترلږه, at_لږترلږه);
    }
};

// A coverpoint or cross
صنف Cg_توکی final {
    // MEMBERS
    uint64_t m_وزن = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

عام:
    // ACCESSORS
    uint64_t وزن_2() ثابت { أرجع m_وزن; }
    ثابت std::map<string, CgBin>& bins() ثابت { أرجع m_bins; }
    // METHODS
    فراغ جمع_کړه_ریکارډ(uint64_t وزن_2) { m_وزن = std::max(m_وزن, وزن_2); }
    CgBin& پیدا_کړه_New_Bin(ثابت string& بې_اغېزه_عنصر) { أرجع m_bins[بې_اغېزه_عنصر]; }
};

// A covergroup
صنف Cg_ډله final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_وزن = 0;  // type_option.weight, the largest of the records
    std::map<string, Cg_توکی> m_توکي;  // By hierarchy

عام:
    // ACCESSORS
    ثابت string& hier() ثابت { أرجع m_hier; }
    uint64_t وزن_2() ثابت { أرجع m_وزن; }
    ثابت std::map<string, Cg_توکی>& توکي() ثابت { أرجع m_توکي; }
    // METHODS
    فراغ جمع_کړه_ریکارډ(ثابت string& hier, uint64_t وزن_2) {
        m_hier = hier;
        m_وزن = std::max(m_وزن, وزن_2);
    }
    Cg_توکی& پیدا_کړه_New_توکی(ثابت string& hier) { أرجع m_توکي[hier]; }
};
کارول CgGroups = std::map<string, Cg_ډله>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t کلی_شمېره(ثابت string& ارزښت) {
    أرجع ارزښت.تشه() ? 1 : std::strtoull(ارزښت.c_str(), تش_اشاره, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
منطقي is_Score_ډګر(ثابت string& ډګر) {
    لكل (ثابت حرف* ثابت keyp : {VL_CIK_THRESH, VL_CIK_وزن, VL_CIK_ډله_وزن}) {
        ثابت string مختاړی = string{"\001"} + keyp + "\002";
        إذا (ډګر.compare(0, مختاړی.اندازه(), مختاړی) == 0) أرجع ریښتیا;
    }
    أرجع خطأ;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_بې_اغېزه_عنصر(ثابت string& ریکارډ_Name) {
    string بې_اغېزه_عنصر;
    string::اندازه_type پیل = 0;
    طالما (پیل < ریکارډ_Name.اندازه()) {
        string::اندازه_type پای_ټکی = ریکارډ_Name.پیدا_کړه('\001', پیل + 1);
        إذا (پای_ټکی == string::npos) پای_ټکی = ریکارډ_Name.اندازه();
        ثابت string ډګر = ریکارډ_Name.substr(پیل, پای_ټکی - پیل);
        إذا (!is_Score_ډګر(ډګر)) بې_اغېزه_عنصر += ډګر;
        پیل = پای_ټکی;
    }
    أرجع بې_اغېزه_عنصر;
}

منطقي isCovergroup(ثابت Vlc_ټکی& ټکی) { أرجع ټکی.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    لكل (ثابت VlcPoints::ByName::ارزښت_type& i : points) {
        ثابت Vlc_ټکی& pt = points.ټکی_شمېره(i.دوهم);
        إذا (!isCovergroup(pt)) استمر;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ثابت string hier = pt.hier();
        string bin = pt.bin();
        إذا (bin.تشه() || bin.اندازه() >= hier.اندازه()) bin = hier.substr(hier.rfind('.') + 1);
        ثابت string توکی_Hier
            = hier.substr(0, hier.اندازه() - std::min(hier.اندازه(), bin.اندازه() + 1));
        Cg_ډله& ډله = groups[pt.پاڼه()];
        ډله.جمع_کړه_ریکارډ(توکی_Hier.substr(0, توکی_Hier.rfind('.')), کلی_شمېره(pt.ډله_وزن()));
        Cg_توکی& توکی_2 = ډله.پیدا_کړه_New_توکی(توکی_Hier);
        توکی_2.جمع_کړه_ریکارډ(کلی_شمېره(pt.وزن_2()));
        إذا (!pt.binType().تشه()) استمر;  // Not coverable
        توکی_2.پیدا_کړه_New_Bin(bin_بې_اغېزه_عنصر(pt.name())).جمع_کړه_ریکارډ(bin, pt.شمېر(), کلی_شمېره(pt.thresh()));
    }
    أرجع groups;
}

Tally توکی_Tally(ثابت Cg_توکی& توکی_2) {
    Tally tally;
    لكل (ثابت std::pair<ثابت string, CgBin>& bin : توکی_2.bins()) {
        tally.جمع_کړه_ټکی(bin.دوهم.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.ټول() ? pct(tally.موندنه(), tally.ټول()) : توکی_2.وزن_2() ? 0.0 : 100.0);
    أرجع tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally ډله_Tally(ثابت Cg_ډله& ډله, منطقي* contributesp = تش_اشاره) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    لكل (ثابت std::pair<ثابت string, Cg_توکی>& it : ډله.توکي()) {
        ثابت Tally توکی_2 = توکی_Tally(it.دوهم);
        tally.جمع_کړه_Points(توکی_2);
        إذا (!توکی_2.ټول()) استمر;  // Excluded from both sums
        weighted += static_cast<مزدوج>(it.دوهم.وزن_2()) * توکی_2.score();
        weights += static_cast<مزدوج>(it.دوهم.وزن_2());
    }
    إذا (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : ډله.وزن_2() ? 0.0 : 100.0);
    أرجع tally;
}

Tally groupsTally(ثابت std::ویکټور<ثابت Cg_ډله*>& groups) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    منطقي any_وزن = خطأ;
    لكل (ثابت Cg_ډله* ثابت groupp : groups) {
        منطقي contributes = خطأ;
        ثابت Tally ډله = ډله_Tally(*groupp, &contributes);
        tally.جمع_کړه_Points(ډله);
        إذا (groupp->وزن_2()) any_وزن = ریښتیا;
        إذا (!contributes) استمر;
        weighted += static_cast<مزدوج>(groupp->وزن_2()) * ډله.score();
        weights += static_cast<مزدوج>(groupp->وزن_2());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : any_وزن ? 0.0 : 100.0);
    أرجع tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ثابت CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::ویکټور<ثابت Cg_ډله*>> غوټه_Groups;
    لكل (ثابت CgGroups::ارزښت_type& it : groups) {
        ثابت Cg_ډله& ډله = it.دوهم;
        string لار;
        لكل (ثابت string& part : وېشل_Hier(ډله.hier())) {
            لار = لار.تشه() ? part : لار + "." + part;
            غوټه_Groups[لار].ټېل_شا(&ډله);
        }
        لكل (ثابت std::pair<ثابت string, Cg_توکی>& توکی_2 : ډله.توکي()) {
            tallies[توکی_2.لومړی] = توکی_Tally(توکی_2.دوهم);
            لكل (ثابت std::pair<ثابت string, CgBin>& bin : توکی_2.دوهم.bins()) {
                // Of the bins of the name
                tallies[توکی_2.لومړی + "." + bin.دوهم.name()].جمع_کړه_ټکی(bin.دوهم.covered());
            }
        }
    }
    لكل (ثابت std::pair<ثابت string, std::ویکټور<ثابت Cg_ډله*>>& it : غوټه_Groups) {
        tallies[it.لومړی]
            = it.دوهم.اندازه() == 1 ? ډله_Tally(*it.دوهم.front()) : groupsTally(it.دوهم);
    }
    أرجع tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
فراغ printIndent(اندازه_t indent) {
    لكل (اندازه_t i = 0; i < indent; ++i) std::وتنه << ' ';
}

فراغ print_Tally_لیکه(ثابت string& type, ثابت Tally& tally, اندازه_t indent, اندازه_t type_پلنوالی,
                   اندازه_t شمېر_پلنوالی) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ثابت string percent = tally.scored() ? pct_ارزښت_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.موندنه(), tally.ټول());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::وتنه << std::کیڼ << std::setw(type_پلنوالی) << type << " : " << std::ښی << std::fixed
              << std::setw(6) << percent << " (" << std::setw(شمېر_پلنوالی) << tally.موندنه() << "/"
              << std::setw(شمېر_پلنوالی) << tally.ټول() << ")\n";
}

اندازه_t شمېر_پلنوالی(ثابت TypeTally& tally) {
    اندازه_t پلنوالی = cvt_منزل_Str(0).اندازه();
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.پای_ټکی(); ++it) {
        پلنوالی = std::max(پلنوالی, cvt_منزل_Str(it->دوهم.موندنه()).اندازه());
        پلنوالی = std::max(پلنوالی, cvt_منزل_Str(it->دوهم.ټول()).اندازه());
    }
    أرجع پلنوالی;
}

اندازه_t type_پلنوالی(ثابت TypeTally& tally) {
    اندازه_t type_پلنوالی = 0;
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        type_پلنوالی = std::max(type_پلنوالی, type.اندازه());
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.پای_ټکی(); ++it) {
        type_پلنوالی = std::max(type_پلنوالی, it->لومړی.اندازه());
    }
    أرجع type_پلنوالی;
}

فراغ printTypeTally(ثابت TypeTally& tally, اندازه_t indent, منطقي includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ثابت اندازه_t typ_پلنوالی = type_پلنوالی(tally);
    ثابت اندازه_t cnt_پلنوالی = شمېر_پلنوالی(tally);
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        ثابت TypeTally::const_iterator it = tally.پیدا_کړه(type);
        إذا (it != tally.پای_ټکی()) {
            print_Tally_لیکه(type, it->دوهم, indent, typ_پلنوالی, cnt_پلنوالی);
        } وإلا إذا (includeMissingOrdered) {
            print_Tally_لیکه(type, Tally{}, indent, typ_پلنوالی, cnt_پلنوالی);
        }
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.پای_ټکی(); ++it) {
        إذا (!isOrderedType(it->لومړی)) {
            print_Tally_لیکه(it->لومړی, it->دوهم, indent, typ_پلنوالی, cnt_پلنوالی);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
فراغ printCovergroupTallies(ثابت std::map<string, Tally>& tallies, صحيح پوړونه) {
    std::map<string, Tally> shown;
    اندازه_t name_پلنوالی = 0;
    لكل (ثابت std::pair<ثابت string, Tally>& it : tallies) {
        إذا (پوړونه >= 0 && static_cast<صحيح>(وېشل_Hier(it.لومړی).اندازه()) > پوړونه + 1) استمر;
        shown.ورننباسه(it);
        name_پلنوالی = std::max(name_پلنوالی, it.لومړی.اندازه());
    }
    ثابت اندازه_t cnt_پلنوالی = شمېر_پلنوالی(shown);
    std::وتنه << "Covergroup Coverage Summary:\n";
    لكل (ثابت std::pair<ثابت string, Tally>& it : shown) {
        print_Tally_لیکه(it.لومړی, it.دوهم, s_summaryIndent, name_پلنوالی, cnt_پلنوالی);
    }
}

}  // namespace

فراغ Vlc_سر::readCoverage(ثابت string& filename, منطقي nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    إذا (!is) {
        إذا (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        أرجع;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ثابت testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    طالما (!is.eof()) {
        ثابت string کرښه = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        إذا (کرښه[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ثابت string::اندازه_type secspace = کرښه.rfind("' ");
            إذا (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                استمر;
            }
            ثابت string ټکی = کرښه.substr(3, secspace - 3);
            إذا (!opt.is_Type_سمون(ټکی.c_str())) استمر;

            ثابت uint64_t موندنې = std::atoll(کرښه.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ثابت uint64_t pointnum = points().پیدا_کړه_جمع_کړه_ټکی(ټکی, موندنې);
            إذا (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ثابت uint64_t needed = std::max(
                    VlcBuckets::sufficient(), کلی_شمېره(points().ټکی_شمېره(pointnum).thresh()));
                إذا (موندنې >= needed) {
                    points().ټکی_شمېره(pointnum).testsCoveringInc();
                    testp->buckets().جمع_کړه_معلومات(pointnum, موندنې);
                }
            }
        }
    }
}

فراغ Vlc_سر::writeCoverage(ثابت string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    إذا (!os) {
        v3fatal("Can't write file: " << filename);
        أرجع;
    }

    os << "# SystemC::Coverage-3\n";
    لكل (ثابت خپلکاری& i : m_points) {
        ثابت Vlc_ټکی& ټکی = m_points.ټکی_شمېره(i.دوهم);
        os << "C '" << ټکی.name() << "' " << ټکی.شمېر() << '\n';
    }
}

فراغ Vlc_سر::writeInfo(ثابت string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    إذا (!os) {
        v3fatal("Can't write file: " << filename);
        أرجع;
    }

    annotateCalc();

    // See 'man lcov' for format details
    // TN:<trace_file_name>
    // Source file:
    //   SF:<absolute_path_to_source_file>
    //   FN:<line_number_of_function_start>,<function_name>
    //   FNDA:<execution_count>,<function_name>
    //   FNF:<number_functions_found>
    //   FNH:<number_functions_hit>
    // Branches:
    //   BRDA:<line_number>,<block_number>,<branch>,<taken_count_or_-_for_zero>
    //   BRF:<number_of_branches_found>
    //   BRH:<number_of_branches_hit>
    // Line counts:
    //   DA:<line_number>,<execution_count>
    //   LF:<number_of_lines_found>
    //   LH:<number_of_lines_hit>
    // Section ending:
    //   end_of_record

    os << "TN:verilator_coverage\n";
    لكل (خپلکاری& si : m_sources) {
        Vlc_سرچینه& سرچینه = si.دوهم;
        os << "SF:" << سرچینه.name() << '\n';
        Vlc_سرچینه::LinenoMap& کرښې = سرچینه.کرښې();
        صحيح branches_وموندل = 0;
        صحيح branches_موندنه = 0;
        لكل (خپلکاری& li : کرښې) {
            Vlc_سرچینه_شمېر& sc = li.دوهم;
            uint64_t da_شمېر = 0;
            std::ویکټور<ثابت Vlc_ټکی*> infoPoints;
            لكل (ثابت خپلکاری& ټکی : sc.points()) {
                da_شمېر = std::max(da_شمېر, ټکی->شمېر());
                infoPoints.ټېل_شا(ټکی);
            }
            os << "DA:" << sc.lineno() << "," << da_شمېر << "\n";
            إذا (infoPoints.اندازه() <= 1) استمر;
            branches_وموندل += static_cast<صحيح>(infoPoints.اندازه());
            صحيح ټکی_عدد_2 = 0;
            لكل (ثابت Vlc_ټکی* ټکی : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                إذا (ټکی->isFsmArc()) {
                    os << "2,";
                    os << ټکی->fsmFromState() << "->" << ټکی->fsm_منزل_State();
                } وإلا إذا (ټکی->comment().تشه()) {
                    os << "0,";
                    os << ټکی_عدد_2;
                } وإلا {
                    os << (ټکی->isFsmState() ? '1' : '0') << ',';
                    std::string comment(ټکی->comment());
                    std::replace(comment.begin(), comment.پای_ټکی(), ',', '_');
                    os << comment;
                }
                os << "," << ټکی->شمېر() << "\n";

                branches_موندنه += opt.شمېر_Ok(ټکی->شمېر());
                ++ټکی_عدد_2;
            }
        }
        os << "BRF:" << branches_وموندل << '\n';
        os << "BRH:" << branches_موندنه << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

بنية CmpComputrons final {
    منطقي چلوونکی()(ثابت VlcTest* lhsp, ثابت VlcTest* rhsp) ثابت {
        إذا (lhsp->computrons() != rhsp->computrons()) {
            أرجع lhsp->computrons() < rhsp->computrons();
        }
        أرجع lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

فراغ Vlc_سر::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::ویکټور<VlcTest*> bytime;
    لكل (ثابت خپلکاری& testp : m_tests) {
        إذا (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ټېل_شا(testp);
        }
    }
    ترتیبول(bytime.begin(), bytime.پای_ټکی(), CmpComputrons());  // Sort the vector

    VlcBuckets پاتې_شوی;
    لكل (ثابت خپلکاری& i : m_points) {
        ثابت Vlc_ټکی* ثابت pointp = &points().ټکی_شمېره(i.دوهم);
        // If any tests hit this point, then we'll need to cover it.
        إذا (pointp->testsCovering()) پاتې_شوی.جمع_کړه_معلومات(pointp->ټکی_عدد(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    طالما (ریښتیا) {
        إذا (debug() >= 9) {
            UINFO_مختاړی("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            پاتې_شوی.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* غوره_Testp = تش_اشاره;
        uint64_t غوره_Remain = 0;
        لكل (ثابت خپلکاری& testp : bytime) {
            إذا (!testp->rank()) {
                ثابت uint64_t remain = testp->buckets().معلومات_راوباسه_شمېر(پاتې_شوی);
                إذا (remain > غوره_Remain) {
                    غوره_Testp = testp;
                    غوره_Remain = remain;
                }
            }
        }
        إذا (VlcTest* ثابت testp = غوره_Testp) {
            testp->rank(nextrank++);
            testp->rankPoints(غوره_Remain);
            پاتې_شوی.or_معلومات(غوره_Testp->buckets());
        } وإلا {
            اكسر;  // No test covering more stuff found
        }
    }
}

فراغ Vlc_سر::annotateCalc() {
    // Calculate per-line information into filedata structure
    لكل (ثابت خپلکاری& i : m_points) {
        ثابت Vlc_ټکی& ټکی = m_points.ټکی_شمېره(i.دوهم);
        ثابت string filename = ټکی.filename();
        ثابت صحيح lineno = ټکی.lineno();
        إذا (!filename.تشه() && lineno != 0) {
            Vlc_سرچینه& سرچینه = sources().پیدا_کړه_New_سرچینه(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << ټکی.ستنه_2() << " "
                                       << ټکی.شمېر() << " " << ټکی.linescov());
            // Base coverage
            سرچینه.ورننباسه_ټکی(lineno, &ټکی);
            // Additional lines covered by this statement
            منطقي range = خطأ;
            صحيح پیل = 0;
            صحيح پای_ټکی = 0;
            ثابت string linescov = ټکی.linescov();
            لكل (ثابت حرف* covp = linescov.c_str(); ریښتیا; ++covp) {
                إذا (!*covp || *covp == ',') {  // Ending
                    لكل (صحيح lni = پیل; پیل && lni <= پای_ټکی; ++lni) {
                        سرچینه.ورننباسه_ټکی(lni, &ټکی);
                    }
                    إذا (!*covp) اكسر;
                    پیل = 0;  // Prep for next
                    پای_ټکی = 0;
                    range = خطأ;
                } وإلا إذا (*covp == '-') {
                    range = ریښتیا;
                } وإلا إذا (std::که_عدد(*covp)) {
                    ثابت حرف* ثابت digitsp = covp;
                    طالما (std::که_عدد(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    إذا (!range) پیل = std::atoi(digitsp);
                    پای_ټکی = std::atoi(digitsp);
                }
            }
        }
    }
}

فراغ Vlc_سر::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    صحيح totCases = 0;
    صحيح totOk = 0;
    لكل (خپلکاری& si : m_sources) {
        Vlc_سرچینه& سرچینه = si.دوهم;
        // UINFO(1, "Source " << source.name());
        إذا (opt.annotateAll()) سرچینه.needed(ریښتیا);
        ثابت Vlc_سرچینه::LinenoMap& کرښې = سرچینه.کرښې();
        لكل (خپلکاری& li : کرښې) {
            ثابت Vlc_سرچینه_شمېر& sc = li.دوهم;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            إذا (opt.شمېر_Ok(sc.min_شمېر())) {
                ++totOk;
            } وإلا {
                سرچینه.needed(ریښتیا);
            }
        }
    }
    std::وتنه << "Annotation Summary:\n";
    std::وتنه << "  lines with all attached points covered : ";
    std::وتنه << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    إذا (totOk != totCases) وتنه << "See lines with '%00' in " << opt.annotate_بهر() << '\n';
}

فراغ Vlc_سر::annotateOutputFiles(ثابت string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    لكل (خپلکاری& si : m_sources) {
        Vlc_سرچینه& سرچینه = si.دوهم;
        إذا (!سرچینه.needed()) استمر;
        ثابت string filename = سرچینه.name();
        ثابت string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        إذا (!is) {
            v3error("Can't read annotation file: " << filename);
            أرجع;
        }

        std::ofstream os{outfilename.c_str()};
        إذا (!os) {
            v3error("Can't write file: " << outfilename);
            أرجع;
        }

        os << "//      // verilator_coverage annotation\n";

        صحيح lineno = 0;
        طالما (!is.eof()) {
            lineno++;
            ثابت std::string کرښه = V3Os::getline(is);

            Vlc_سرچینه::LinenoMap& کرښې = سرچینه.کرښې();
            ثابت خپلکاری lit = کرښې.پیدا_کړه(lineno);
            إذا (lit == کرښې.پای_ټکی()) {
                os << "        " << کرښه << '\n';
            } وإلا {
                Vlc_سرچینه_شمېر& sc = lit->دوهم;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ثابت منطقي minOk = opt.شمېر_Ok(sc.min_شمېر());
                ثابت منطقي maxOk = opt.شمېر_Ok(sc.max_شمېر());
                إذا (minOk) {
                    os << " ";
                } وإلا إذا (maxOk) {
                    os << "~";
                } وإلا {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_شمېر() << " " << کرښه << '\n';

                إذا (opt.annotatePoints()) {
                    لكل (ثابت خپلکاری& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                منطقي printed_Fsm_سرلیک = خطأ;
                لكل (ثابت خپلکاری& pit : sc.points()) {
                    إذا (!pit->isFsmState() && !pit->isFsmArc()) استمر;
                    إذا (!printed_Fsm_سرلیک) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_سرلیک = ریښتیا;
                    }
                    os << (opt.شمېر_Ok(pit->شمېر()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->شمېر() << "        ";
                    إذا (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        إذا (pit->شمېر() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } وإلا إذا (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } وإلا {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        إذا (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

فراغ Vlc_سر::annotate(ثابت string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

فراغ Vlc_سر::printTypeSummary() {
    TypeTally tally;
    لكل (VlcPoints::ByName::ارزښت_type& i : m_points) {
        ثابت Vlc_ټکی& pt = m_points.ټکی_شمېره(i.دوهم);
        إذا (isCovergroup(pt)) استمر;  // Tallied below, by covergroup
        tally_ټکی(tally, displayType(pt), pt.شمېر());
    }
    ثابت CgGroups groups = covergroups(m_points);
    إذا (!groups.تشه()) {
        std::ویکټور<ثابت Cg_ډله*> groupps;
        لكل (ثابت CgGroups::ارزښت_type& it : groups) groupps.ټېل_شا(&it.دوهم);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    إذا (tally.تشه()) أرجع;
    std::وتنه << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, ریښتیا);
}

فراغ Vlc_سر::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    منطقي hasHier = خطأ;
    منطقي hasCollapsedHier = خطأ;
    لكل (VlcPoints::ByName::ارزښت_type& i : m_points) {
        ثابت Vlc_ټکی& pt = m_points.ټکی_شمېره(i.دوهم);
        إذا (isCovergroup(pt)) استمر;  // Reported below, by covergroup
        ثابت string hier = reportHier(pt);
        إذا (hier.تشه()) استمر;
        hasHier = ریښتیا;
        إذا (isCollapsedHier(hier)) hasCollapsedHier = ریښتیا;
        ثابت string type = displayType(pt);
        ثابت std::ویکټور<string> parts = وېشل_Hier(hier);
        string لار;
        لكل (std::ویکټور<string>::const_iterator it = parts.begin(); it != parts.پای_ټکی(); ++it) {
            لار = لار.تشه() ? *it : لار + "." + *it;
            tally_ټکی(hierTallies[لار], type, pt.شمېر());
        }
        tally_ټکی(duTallies[duName(pt)], type, pt.شمېر());
    }
    ثابت std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    إذا (!hasHier && !cgTallies.تشه()) {
        printCovergroupTallies(cgTallies, opt.report_پوړونه());
        أرجع;
    }
    إذا (!hasHier) {
        std::وتنه << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        أرجع;
    }

    ثابت صحيح پوړونه = opt.report_پوړونه();
    إذا (hasCollapsedHier) {
        std::وتنه << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::وتنه << "Hierarchy Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.پای_ټکی(); ++it) {
        ثابت std::ویکټور<string> parts = وېشل_Hier(it->لومړی);
        إذا (پوړونه >= 0 && static_cast<صحيح>(parts.اندازه()) > پوړونه + 1) استمر;
        printIndent(s_summaryIndent);
        std::وتنه << it->لومړی << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->دوهم, s_report_لیکه_Indent, خطأ);
    }
    std::وتنه << "Design Unit Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.پای_ټکی();
         ++it) {
        printIndent(s_summaryIndent);
        std::وتنه << it->لومړی << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->دوهم, s_report_لیکه_Indent, خطأ);
    }
    إذا (!cgTallies.تشه()) printCovergroupTallies(cgTallies, پوړونه);
}
