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

#تعریف_کن VL_MT_DISABLED_کد_UNIT 1

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
    uint64_t m_اصابت = 0;  // Covered points
    uint64_t m_مجموع = 0;  // Points
    منطقي m_scored = خطأ;  // Coverage is m_score, not the ratio of the points
    مزدوج m_score = 0.0;  // Percent coverage, if m_scored

عام:
    // ACCESSORS
    uint64_t اصابت() ثابت { أرجع m_اصابت; }
    uint64_t مجموع() ثابت { أرجع m_مجموع; }
    منطقي scored() ثابت { أرجع m_scored; }
    مزدوج score() ثابت { أرجع m_score; }
    فراغ score(مزدوج مقدار) {
        m_scored = درست;
        m_score = مقدار;
    }
    // METHODS
    فراغ جمع_کن_نقطه(منطقي covered) {
        إذا (covered) ++m_اصابت;
        ++m_مجموع;
    }
    فراغ جمع_کن_Points(ثابت Tally& other) {
        m_اصابت += other.m_اصابت;
        m_مجموع += other.m_مجموع;
    }
};

// Map coverage type to its tally.
استفاده TypeTally = std::map<std::string, Tally>;

ساكن constexpr ثابت حرف* ثابت s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ساكن constexpr ثابت حرف* ثابت s_covergroupType = "covergroup";
ساكن constexpr اندازه_t s_summaryIndent = 2;
ساكن constexpr اندازه_t s_report_ردیف_Indent = 4;

string displayType(ثابت Vlc_نقطه& نقطه) {
    ثابت string type = نقطه.type();
    أرجع type.خالی() ? "point" : type;
}

منطقي isCollapsedHier(ثابت string& hier) {
    أرجع hier.پیدا_کن('*') != string::npos || hier.پیدا_کن('?') != string::npos;
}

منطقي isOrderedType(ثابت string& type) {
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        إذا (type == typep) أرجع درست;
    }
    أرجع خطأ;
}

string reportHier(ثابت Vlc_نقطه& نقطه) {
    // FSM records currently have useful instance scope in Fv.
    إذا (نقطه.isFsmState() || نقطه.isFsmArc()) {
        ثابت string fsmVar = نقطه.fsmVarName();
        أرجع fsmVar.substr(0, fsmVar.rfind('.'));
    }
    أرجع نقطه.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::وکتور<string> تقسیم_Hier(ثابت string& hier) {
    std::وکتور<string> parts;
    string::اندازه_type شروع = 0;
    صحيح عمق = 0;  // Of the parentheses of the values of parameters
    string::اندازه_type pos = 0;
    طالما (pos < hier.اندازه()) {
        ثابت حرف c = hier[pos];
        إذا (c == '\\') {
            pos = hier.پیدا_کن(' ', pos);  // npos if not ended
            استمر;
        } وإلا إذا (!عمق && hier.compare(pos, 2, "#(") == 0) {
            عمق = 1;
            ++pos;
        } وإلا إذا (!عمق && c == '.') {
            parts.افزودن_پشت(hier.substr(شروع, pos - شروع));
            شروع = pos + 1;
        } وإلا إذا (عمق && c == '"') {
            pos = VString::quoted_انتها(hier, pos);  // npos if not terminated
            استمر;
        } وإلا إذا (عمق && c == '(') {
            ++عمق;
        } وإلا إذا (عمق && c == ')') {
            --عمق;
        }
        ++pos;
    }
    إذا (عمق || pos != hier.اندازه()) {  // Unbalanced
        parts.clear();
        شروع = 0;
        لكل (string::اندازه_type dot; (dot = hier.پیدا_کن('.', شروع)) != string::npos;
             شروع = dot + 1) {
            parts.افزودن_پشت(hier.substr(شروع, dot - شروع));
        }
    }
    parts.افزودن_پشت(hier.substr(شروع));
    أرجع parts;
}

string duName(ثابت Vlc_نقطه& نقطه) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ثابت string صفحه = نقطه.صفحه();
    أرجع صفحه.substr(صفحه.پیدا_کن('/') + 1);
}

فراغ tally_نقطه(TypeTally& tally, ثابت string& type, uint64_t شمار) {
    tally[type].جمع_کن_نقطه(شمار > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
مزدوج pct(uint64_t اصابت, uint64_t مجموع) {
    أرجع مجموع ? (100.0 * static_cast<مزدوج>(اصابت) / static_cast<مزدوج>(مجموع)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_مقدار_String(مزدوج مقدار, منطقي complete) {
    إذا (!complete && مقدار > 99.9) مقدار = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << مقدار << "%";
    أرجع os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t اصابت, uint64_t مجموع) {
    أرجع pct_مقدار_String(pct(اصابت, مجموع), اصابت >= مجموع);
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
    uint64_t m_شمار = 0;  // Hits, of all the records
    uint64_t m_at_کمترین = 0;  // option.at_least, the largest of the records

عام:
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    منطقي covered() ثابت { أرجع m_شمار >= m_at_کمترین; }
    // METHODS
    فراغ جمع_کن_رکورد(ثابت string& name, uint64_t شمار, uint64_t at_کمترین) {
        m_name = name;
        m_شمار += شمار;
        m_at_کمترین = std::max(m_at_کمترین, at_کمترین);
    }
};

// A coverpoint or cross
صنف Cg_قلم final {
    // MEMBERS
    uint64_t m_وزن = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

عام:
    // ACCESSORS
    uint64_t وزن_2() ثابت { أرجع m_وزن; }
    ثابت std::map<string, CgBin>& bins() ثابت { أرجع m_bins; }
    // METHODS
    فراغ جمع_کن_رکورد(uint64_t وزن_2) { m_وزن = std::max(m_وزن, وزن_2); }
    CgBin& پیدا_کن_New_Bin(ثابت string& عنصر_خنثی) { أرجع m_bins[عنصر_خنثی]; }
};

// A covergroup
صنف Cg_گروه final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_وزن = 0;  // type_option.weight, the largest of the records
    std::map<string, Cg_قلم> m_اقلام;  // By hierarchy

عام:
    // ACCESSORS
    ثابت string& hier() ثابت { أرجع m_hier; }
    uint64_t وزن_2() ثابت { أرجع m_وزن; }
    ثابت std::map<string, Cg_قلم>& اقلام() ثابت { أرجع m_اقلام; }
    // METHODS
    فراغ جمع_کن_رکورد(ثابت string& hier, uint64_t وزن_2) {
        m_hier = hier;
        m_وزن = std::max(m_وزن, وزن_2);
    }
    Cg_قلم& پیدا_کن_New_قلم(ثابت string& hier) { أرجع m_اقلام[hier]; }
};
استفاده CgGroups = std::map<string, Cg_گروه>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t کلید_عدد(ثابت string& مقدار) {
    أرجع مقدار.خالی() ? 1 : std::strtoull(مقدار.c_str(), اشاره_تهی, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
منطقي is_Score_فیلد(ثابت string& فیلد) {
    لكل (ثابت حرف* ثابت keyp : {VL_CIK_THRESH, VL_CIK_وزن, VL_CIK_گروه_وزن}) {
        ثابت string پیشوند = string{"\001"} + keyp + "\002";
        إذا (فیلد.compare(0, پیشوند.اندازه(), پیشوند) == 0) أرجع درست;
    }
    أرجع خطأ;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_عنصر_خنثی(ثابت string& رکورد_Name) {
    string عنصر_خنثی;
    string::اندازه_type شروع = 0;
    طالما (شروع < رکورد_Name.اندازه()) {
        string::اندازه_type انتها = رکورد_Name.پیدا_کن('\001', شروع + 1);
        إذا (انتها == string::npos) انتها = رکورد_Name.اندازه();
        ثابت string فیلد = رکورد_Name.substr(شروع, انتها - شروع);
        إذا (!is_Score_فیلد(فیلد)) عنصر_خنثی += فیلد;
        شروع = انتها;
    }
    أرجع عنصر_خنثی;
}

منطقي isCovergroup(ثابت Vlc_نقطه& نقطه) { أرجع نقطه.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    لكل (ثابت VlcPoints::ByName::مقدار_type& i : points) {
        ثابت Vlc_نقطه& pt = points.نقطه_عدد(i.دومی);
        إذا (!isCovergroup(pt)) استمر;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ثابت string hier = pt.hier();
        string bin = pt.bin();
        إذا (bin.خالی() || bin.اندازه() >= hier.اندازه()) bin = hier.substr(hier.rfind('.') + 1);
        ثابت string قلم_Hier
            = hier.substr(0, hier.اندازه() - std::min(hier.اندازه(), bin.اندازه() + 1));
        Cg_گروه& گروه = groups[pt.صفحه()];
        گروه.جمع_کن_رکورد(قلم_Hier.substr(0, قلم_Hier.rfind('.')), کلید_عدد(pt.گروه_وزن()));
        Cg_قلم& قلم_2 = گروه.پیدا_کن_New_قلم(قلم_Hier);
        قلم_2.جمع_کن_رکورد(کلید_عدد(pt.وزن_2()));
        إذا (!pt.binType().خالی()) استمر;  // Not coverable
        قلم_2.پیدا_کن_New_Bin(bin_عنصر_خنثی(pt.name())).جمع_کن_رکورد(bin, pt.شمار(), کلید_عدد(pt.thresh()));
    }
    أرجع groups;
}

Tally قلم_Tally(ثابت Cg_قلم& قلم_2) {
    Tally tally;
    لكل (ثابت std::pair<ثابت string, CgBin>& bin : قلم_2.bins()) {
        tally.جمع_کن_نقطه(bin.دومی.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.مجموع() ? pct(tally.اصابت(), tally.مجموع()) : قلم_2.وزن_2() ? 0.0 : 100.0);
    أرجع tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally گروه_Tally(ثابت Cg_گروه& گروه, منطقي* contributesp = اشاره_تهی) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    لكل (ثابت std::pair<ثابت string, Cg_قلم>& it : گروه.اقلام()) {
        ثابت Tally قلم_2 = قلم_Tally(it.دومی);
        tally.جمع_کن_Points(قلم_2);
        إذا (!قلم_2.مجموع()) استمر;  // Excluded from both sums
        weighted += static_cast<مزدوج>(it.دومی.وزن_2()) * قلم_2.score();
        weights += static_cast<مزدوج>(it.دومی.وزن_2());
    }
    إذا (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : گروه.وزن_2() ? 0.0 : 100.0);
    أرجع tally;
}

Tally groupsTally(ثابت std::وکتور<ثابت Cg_گروه*>& groups) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    منطقي any_وزن = خطأ;
    لكل (ثابت Cg_گروه* ثابت groupp : groups) {
        منطقي contributes = خطأ;
        ثابت Tally گروه = گروه_Tally(*groupp, &contributes);
        tally.جمع_کن_Points(گروه);
        إذا (groupp->وزن_2()) any_وزن = درست;
        إذا (!contributes) استمر;
        weighted += static_cast<مزدوج>(groupp->وزن_2()) * گروه.score();
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
    std::map<string, std::وکتور<ثابت Cg_گروه*>> گره_Groups;
    لكل (ثابت CgGroups::مقدار_type& it : groups) {
        ثابت Cg_گروه& گروه = it.دومی;
        string مسیر;
        لكل (ثابت string& part : تقسیم_Hier(گروه.hier())) {
            مسیر = مسیر.خالی() ? part : مسیر + "." + part;
            گره_Groups[مسیر].افزودن_پشت(&گروه);
        }
        لكل (ثابت std::pair<ثابت string, Cg_قلم>& قلم_2 : گروه.اقلام()) {
            tallies[قلم_2.اولی] = قلم_Tally(قلم_2.دومی);
            لكل (ثابت std::pair<ثابت string, CgBin>& bin : قلم_2.دومی.bins()) {
                // Of the bins of the name
                tallies[قلم_2.اولی + "." + bin.دومی.name()].جمع_کن_نقطه(bin.دومی.covered());
            }
        }
    }
    لكل (ثابت std::pair<ثابت string, std::وکتور<ثابت Cg_گروه*>>& it : گره_Groups) {
        tallies[it.اولی]
            = it.دومی.اندازه() == 1 ? گروه_Tally(*it.دومی.front()) : groupsTally(it.دومی);
    }
    أرجع tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
فراغ printIndent(اندازه_t indent) {
    لكل (اندازه_t i = 0; i < indent; ++i) std::خروجی << ' ';
}

فراغ print_Tally_ردیف(ثابت string& type, ثابت Tally& tally, اندازه_t indent, اندازه_t type_عرض,
                   اندازه_t شمار_عرض) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ثابت string percent = tally.scored() ? pct_مقدار_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.اصابت(), tally.مجموع());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::خروجی << std::چپ << std::setw(type_عرض) << type << " : " << std::راست << std::fixed
              << std::setw(6) << percent << " (" << std::setw(شمار_عرض) << tally.اصابت() << "/"
              << std::setw(شمار_عرض) << tally.مجموع() << ")\n";
}

اندازه_t شمار_عرض(ثابت TypeTally& tally) {
    اندازه_t عرض = cvt_مقصد_Str(0).اندازه();
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.انتها(); ++it) {
        عرض = std::max(عرض, cvt_مقصد_Str(it->دومی.اصابت()).اندازه());
        عرض = std::max(عرض, cvt_مقصد_Str(it->دومی.مجموع()).اندازه());
    }
    أرجع عرض;
}

اندازه_t type_عرض(ثابت TypeTally& tally) {
    اندازه_t type_عرض = 0;
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        type_عرض = std::max(type_عرض, type.اندازه());
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.انتها(); ++it) {
        type_عرض = std::max(type_عرض, it->اولی.اندازه());
    }
    أرجع type_عرض;
}

فراغ printTypeTally(ثابت TypeTally& tally, اندازه_t indent, منطقي includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ثابت اندازه_t typ_عرض = type_عرض(tally);
    ثابت اندازه_t cnt_عرض = شمار_عرض(tally);
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        ثابت TypeTally::const_iterator it = tally.پیدا_کن(type);
        إذا (it != tally.انتها()) {
            print_Tally_ردیف(type, it->دومی, indent, typ_عرض, cnt_عرض);
        } وإلا إذا (includeMissingOrdered) {
            print_Tally_ردیف(type, Tally{}, indent, typ_عرض, cnt_عرض);
        }
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.انتها(); ++it) {
        إذا (!isOrderedType(it->اولی)) {
            print_Tally_ردیف(it->اولی, it->دومی, indent, typ_عرض, cnt_عرض);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
فراغ printCovergroupTallies(ثابت std::map<string, Tally>& tallies, صحيح سطوح) {
    std::map<string, Tally> shown;
    اندازه_t name_عرض = 0;
    لكل (ثابت std::pair<ثابت string, Tally>& it : tallies) {
        إذا (سطوح >= 0 && static_cast<صحيح>(تقسیم_Hier(it.اولی).اندازه()) > سطوح + 1) استمر;
        shown.درج(it);
        name_عرض = std::max(name_عرض, it.اولی.اندازه());
    }
    ثابت اندازه_t cnt_عرض = شمار_عرض(shown);
    std::خروجی << "Covergroup Coverage Summary:\n";
    لكل (ثابت std::pair<ثابت string, Tally>& it : shown) {
        print_Tally_ردیف(it.اولی, it.دومی, s_summaryIndent, name_عرض, cnt_عرض);
    }
}

}  // namespace

فراغ Vlc_بالاترین::readCoverage(ثابت string& filename, منطقي nonfatal) {
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
        ثابت string سطر = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        إذا (سطر[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ثابت string::اندازه_type secspace = سطر.rfind("' ");
            إذا (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                استمر;
            }
            ثابت string نقطه = سطر.substr(3, secspace - 3);
            إذا (!opt.is_Type_تطابق(نقطه.c_str())) استمر;

            ثابت uint64_t اصابتها = std::atoll(سطر.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ثابت uint64_t pointnum = points().پیدا_کن_جمع_کن_نقطه(نقطه, اصابتها);
            إذا (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ثابت uint64_t needed = std::max(
                    VlcBuckets::sufficient(), کلید_عدد(points().نقطه_عدد(pointnum).thresh()));
                إذا (اصابتها >= needed) {
                    points().نقطه_عدد(pointnum).testsCoveringInc();
                    testp->buckets().جمع_کن_داده(pointnum, اصابتها);
                }
            }
        }
    }
}

فراغ Vlc_بالاترین::writeCoverage(ثابت string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    إذا (!os) {
        v3fatal("Can't write file: " << filename);
        أرجع;
    }

    os << "# SystemC::Coverage-3\n";
    لكل (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطه& نقطه = m_points.نقطه_عدد(i.دومی);
        os << "C '" << نقطه.name() << "' " << نقطه.شمار() << '\n';
    }
}

فراغ Vlc_بالاترین::writeInfo(ثابت string& filename) {
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
    لكل (خودکار& si : m_sources) {
        Vlc_منبع& منبع = si.دومی;
        os << "SF:" << منبع.name() << '\n';
        Vlc_منبع::LinenoMap& سطرها = منبع.سطرها();
        صحيح branches_یافت = 0;
        صحيح branches_اصابت = 0;
        لكل (خودکار& li : سطرها) {
            Vlc_منبع_شمار& sc = li.دومی;
            uint64_t da_شمار = 0;
            std::وکتور<ثابت Vlc_نقطه*> infoPoints;
            لكل (ثابت خودکار& نقطه : sc.points()) {
                da_شمار = std::max(da_شمار, نقطه->شمار());
                infoPoints.افزودن_پشت(نقطه);
            }
            os << "DA:" << sc.lineno() << "," << da_شمار << "\n";
            إذا (infoPoints.اندازه() <= 1) استمر;
            branches_یافت += static_cast<صحيح>(infoPoints.اندازه());
            صحيح نقطه_رقم_2 = 0;
            لكل (ثابت Vlc_نقطه* نقطه : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                إذا (نقطه->isFsmArc()) {
                    os << "2,";
                    os << نقطه->fsmFromState() << "->" << نقطه->fsm_مقصد_State();
                } وإلا إذا (نقطه->comment().خالی()) {
                    os << "0,";
                    os << نقطه_رقم_2;
                } وإلا {
                    os << (نقطه->isFsmState() ? '1' : '0') << ',';
                    std::string comment(نقطه->comment());
                    std::replace(comment.begin(), comment.انتها(), ',', '_');
                    os << comment;
                }
                os << "," << نقطه->شمار() << "\n";

                branches_اصابت += opt.شمار_Ok(نقطه->شمار());
                ++نقطه_رقم_2;
            }
        }
        os << "BRF:" << branches_یافت << '\n';
        os << "BRH:" << branches_اصابت << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

بنية CmpComputrons final {
    منطقي عملگر()(ثابت VlcTest* lhsp, ثابت VlcTest* rhsp) ثابت {
        إذا (lhsp->computrons() != rhsp->computrons()) {
            أرجع lhsp->computrons() < rhsp->computrons();
        }
        أرجع lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

فراغ Vlc_بالاترین::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::وکتور<VlcTest*> bytime;
    لكل (ثابت خودکار& testp : m_tests) {
        إذا (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.افزودن_پشت(testp);
        }
    }
    مرتب_سازی(bytime.begin(), bytime.انتها(), CmpComputrons());  // Sort the vector

    VlcBuckets مانده;
    لكل (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطه* ثابت pointp = &points().نقطه_عدد(i.دومی);
        // If any tests hit this point, then we'll need to cover it.
        إذا (pointp->testsCovering()) مانده.جمع_کن_داده(pointp->نقطه_رقم(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    طالما (درست) {
        إذا (debug() >= 9) {
            UINFO_پیشوند("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            مانده.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* بهترین_Testp = اشاره_تهی;
        uint64_t بهترین_Remain = 0;
        لكل (ثابت خودکار& testp : bytime) {
            إذا (!testp->rank()) {
                ثابت uint64_t remain = testp->buckets().داده_برداشت_شمار(مانده);
                إذا (remain > بهترین_Remain) {
                    بهترین_Testp = testp;
                    بهترین_Remain = remain;
                }
            }
        }
        إذا (VlcTest* ثابت testp = بهترین_Testp) {
            testp->rank(nextrank++);
            testp->rankPoints(بهترین_Remain);
            مانده.or_داده(بهترین_Testp->buckets());
        } وإلا {
            اكسر;  // No test covering more stuff found
        }
    }
}

فراغ Vlc_بالاترین::annotateCalc() {
    // Calculate per-line information into filedata structure
    لكل (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطه& نقطه = m_points.نقطه_عدد(i.دومی);
        ثابت string filename = نقطه.filename();
        ثابت صحيح lineno = نقطه.lineno();
        إذا (!filename.خالی() && lineno != 0) {
            Vlc_منبع& منبع = sources().پیدا_کن_New_منبع(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << نقطه.ستونی_2() << " "
                                       << نقطه.شمار() << " " << نقطه.linescov());
            // Base coverage
            منبع.درج_نقطه(lineno, &نقطه);
            // Additional lines covered by this statement
            منطقي range = خطأ;
            صحيح شروع = 0;
            صحيح انتها = 0;
            ثابت string linescov = نقطه.linescov();
            لكل (ثابت حرف* covp = linescov.c_str(); درست; ++covp) {
                إذا (!*covp || *covp == ',') {  // Ending
                    لكل (صحيح lni = شروع; شروع && lni <= انتها; ++lni) {
                        منبع.درج_نقطه(lni, &نقطه);
                    }
                    إذا (!*covp) اكسر;
                    شروع = 0;  // Prep for next
                    انتها = 0;
                    range = خطأ;
                } وإلا إذا (*covp == '-') {
                    range = درست;
                } وإلا إذا (std::آیا_رقم(*covp)) {
                    ثابت حرف* ثابت digitsp = covp;
                    طالما (std::آیا_رقم(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    إذا (!range) شروع = std::atoi(digitsp);
                    انتها = std::atoi(digitsp);
                }
            }
        }
    }
}

فراغ Vlc_بالاترین::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    صحيح totCases = 0;
    صحيح totOk = 0;
    لكل (خودکار& si : m_sources) {
        Vlc_منبع& منبع = si.دومی;
        // UINFO(1, "Source " << source.name());
        إذا (opt.annotateAll()) منبع.needed(درست);
        ثابت Vlc_منبع::LinenoMap& سطرها = منبع.سطرها();
        لكل (خودکار& li : سطرها) {
            ثابت Vlc_منبع_شمار& sc = li.دومی;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            إذا (opt.شمار_Ok(sc.min_شمار())) {
                ++totOk;
            } وإلا {
                منبع.needed(درست);
            }
        }
    }
    std::خروجی << "Annotation Summary:\n";
    std::خروجی << "  lines with all attached points covered : ";
    std::خروجی << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    إذا (totOk != totCases) خروجی << "See lines with '%00' in " << opt.annotate_بیرون() << '\n';
}

فراغ Vlc_بالاترین::annotateOutputFiles(ثابت string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    لكل (خودکار& si : m_sources) {
        Vlc_منبع& منبع = si.دومی;
        إذا (!منبع.needed()) استمر;
        ثابت string filename = منبع.name();
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
            ثابت std::string سطر = V3Os::getline(is);

            Vlc_منبع::LinenoMap& سطرها = منبع.سطرها();
            ثابت خودکار lit = سطرها.پیدا_کن(lineno);
            إذا (lit == سطرها.انتها()) {
                os << "        " << سطر << '\n';
            } وإلا {
                Vlc_منبع_شمار& sc = lit->دومی;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ثابت منطقي minOk = opt.شمار_Ok(sc.min_شمار());
                ثابت منطقي maxOk = opt.شمار_Ok(sc.max_شمار());
                إذا (minOk) {
                    os << " ";
                } وإلا إذا (maxOk) {
                    os << "~";
                } وإلا {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_شمار() << " " << سطر << '\n';

                إذا (opt.annotatePoints()) {
                    لكل (ثابت خودکار& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                منطقي printed_Fsm_سرآیند = خطأ;
                لكل (ثابت خودکار& pit : sc.points()) {
                    إذا (!pit->isFsmState() && !pit->isFsmArc()) استمر;
                    إذا (!printed_Fsm_سرآیند) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_سرآیند = درست;
                    }
                    os << (opt.شمار_Ok(pit->شمار()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->شمار() << "        ";
                    إذا (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        إذا (pit->شمار() == 0) os << " *** UNCOVERED ***";
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

فراغ Vlc_بالاترین::annotate(ثابت string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

فراغ Vlc_بالاترین::printTypeSummary() {
    TypeTally tally;
    لكل (VlcPoints::ByName::مقدار_type& i : m_points) {
        ثابت Vlc_نقطه& pt = m_points.نقطه_عدد(i.دومی);
        إذا (isCovergroup(pt)) استمر;  // Tallied below, by covergroup
        tally_نقطه(tally, displayType(pt), pt.شمار());
    }
    ثابت CgGroups groups = covergroups(m_points);
    إذا (!groups.خالی()) {
        std::وکتور<ثابت Cg_گروه*> groupps;
        لكل (ثابت CgGroups::مقدار_type& it : groups) groupps.افزودن_پشت(&it.دومی);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    إذا (tally.خالی()) أرجع;
    std::خروجی << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, درست);
}

فراغ Vlc_بالاترین::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    منطقي hasHier = خطأ;
    منطقي hasCollapsedHier = خطأ;
    لكل (VlcPoints::ByName::مقدار_type& i : m_points) {
        ثابت Vlc_نقطه& pt = m_points.نقطه_عدد(i.دومی);
        إذا (isCovergroup(pt)) استمر;  // Reported below, by covergroup
        ثابت string hier = reportHier(pt);
        إذا (hier.خالی()) استمر;
        hasHier = درست;
        إذا (isCollapsedHier(hier)) hasCollapsedHier = درست;
        ثابت string type = displayType(pt);
        ثابت std::وکتور<string> parts = تقسیم_Hier(hier);
        string مسیر;
        لكل (std::وکتور<string>::const_iterator it = parts.begin(); it != parts.انتها(); ++it) {
            مسیر = مسیر.خالی() ? *it : مسیر + "." + *it;
            tally_نقطه(hierTallies[مسیر], type, pt.شمار());
        }
        tally_نقطه(duTallies[duName(pt)], type, pt.شمار());
    }
    ثابت std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    إذا (!hasHier && !cgTallies.خالی()) {
        printCovergroupTallies(cgTallies, opt.report_سطوح());
        أرجع;
    }
    إذا (!hasHier) {
        std::خروجی << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        أرجع;
    }

    ثابت صحيح سطوح = opt.report_سطوح();
    إذا (hasCollapsedHier) {
        std::خروجی << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::خروجی << "Hierarchy Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.انتها(); ++it) {
        ثابت std::وکتور<string> parts = تقسیم_Hier(it->اولی);
        إذا (سطوح >= 0 && static_cast<صحيح>(parts.اندازه()) > سطوح + 1) استمر;
        printIndent(s_summaryIndent);
        std::خروجی << it->اولی << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->دومی, s_report_ردیف_Indent, خطأ);
    }
    std::خروجی << "Design Unit Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.انتها();
         ++it) {
        printIndent(s_summaryIndent);
        std::خروجی << it->اولی << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->دومی, s_report_ردیف_Indent, خطأ);
    }
    إذا (!cgTallies.خالی()) printCovergroupTallies(cgTallies, سطوح);
}
