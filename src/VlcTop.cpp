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

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

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
    uint64_t m_کامیابی = 0;  // Covered points
    uint64_t m_کل = 0;  // Points
    منطقي m_scored = خطأ;  // Coverage is m_score, not the ratio of the points
    مزدوج m_score = 0.0;  // Percent coverage, if m_scored

عام:
    // ACCESSORS
    uint64_t کامیابی() ثابت { أرجع m_کامیابی; }
    uint64_t کل() ثابت { أرجع m_کل; }
    منطقي scored() ثابت { أرجع m_scored; }
    مزدوج score() ثابت { أرجع m_score; }
    فراغ score(مزدوج قدر) {
        m_scored = سچ;
        m_score = قدر;
    }
    // METHODS
    فراغ جوڑو_نقطہ(منطقي covered) {
        إذا (covered) ++m_کامیابی;
        ++m_کل;
    }
    فراغ جوڑو_Points(ثابت Tally& other) {
        m_کامیابی += other.m_کامیابی;
        m_کل += other.m_کل;
    }
};

// Map coverage type to its tally.
ورتوں TypeTally = std::map<std::string, Tally>;

ساكن constexpr ثابت حرف* ثابت s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ساكن constexpr ثابت حرف* ثابت s_covergroupType = "covergroup";
ساكن constexpr ناپ_t s_summaryIndent = 2;
ساكن constexpr ناپ_t s_report_صف_Indent = 4;

string displayType(ثابت Vlc_نقطہ& نقطہ) {
    ثابت string type = نقطہ.type();
    أرجع type.خالی_تھاواں() ? "point" : type;
}

منطقي isCollapsedHier(ثابت string& hier) {
    أرجع hier.لبھو('*') != string::npos || hier.لبھو('?') != string::npos;
}

منطقي isOrderedType(ثابت string& type) {
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        إذا (type == typep) أرجع سچ;
    }
    أرجع خطأ;
}

string reportHier(ثابت Vlc_نقطہ& نقطہ) {
    // FSM records currently have useful instance scope in Fv.
    إذا (نقطہ.isFsmState() || نقطہ.isFsmArc()) {
        ثابت string fsmVar = نقطہ.fsmVarName();
        أرجع fsmVar.substr(0, fsmVar.rfind('.'));
    }
    أرجع نقطہ.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::سمتیہ<string> ونڈو_Hier(ثابت string& hier) {
    std::سمتیہ<string> parts;
    string::ناپ_type شروع = 0;
    صحيح ڈونگھائی = 0;  // Of the parentheses of the values of parameters
    string::ناپ_type pos = 0;
    طالما (pos < hier.ناپ()) {
        ثابت حرف c = hier[pos];
        إذا (c == '\\') {
            pos = hier.لبھو(' ', pos);  // npos if not ended
            استمر;
        } وإلا إذا (!ڈونگھائی && hier.compare(pos, 2, "#(") == 0) {
            ڈونگھائی = 1;
            ++pos;
        } وإلا إذا (!ڈونگھائی && c == '.') {
            parts.ڈالو_پٹھ(hier.substr(شروع, pos - شروع));
            شروع = pos + 1;
        } وإلا إذا (ڈونگھائی && c == '"') {
            pos = VString::quoted_سرا(hier, pos);  // npos if not terminated
            استمر;
        } وإلا إذا (ڈونگھائی && c == '(') {
            ++ڈونگھائی;
        } وإلا إذا (ڈونگھائی && c == ')') {
            --ڈونگھائی;
        }
        ++pos;
    }
    إذا (ڈونگھائی || pos != hier.ناپ()) {  // Unbalanced
        parts.clear();
        شروع = 0;
        لكل (string::ناپ_type dot; (dot = hier.لبھو('.', شروع)) != string::npos;
             شروع = dot + 1) {
            parts.ڈالو_پٹھ(hier.substr(شروع, dot - شروع));
        }
    }
    parts.ڈالو_پٹھ(hier.substr(شروع));
    أرجع parts;
}

string duName(ثابت Vlc_نقطہ& نقطہ) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ثابت string ورقہ = نقطہ.ورقہ();
    أرجع ورقہ.substr(ورقہ.لبھو('/') + 1);
}

فراغ tally_نقطہ(TypeTally& tally, ثابت string& type, uint64_t گنتی) {
    tally[type].جوڑو_نقطہ(گنتی > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
مزدوج pct(uint64_t کامیابی, uint64_t کل) {
    أرجع کل ? (100.0 * static_cast<مزدوج>(کامیابی) / static_cast<مزدوج>(کل)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_قدر_String(مزدوج قدر, منطقي complete) {
    إذا (!complete && قدر > 99.9) قدر = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << قدر << "%";
    أرجع os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t کامیابی, uint64_t کل) {
    أرجع pct_قدر_String(pct(کامیابی, کل), کامیابی >= کل);
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
    uint64_t m_گنتی = 0;  // Hits, of all the records
    uint64_t m_at_گھٹ_توں_گھٹ = 0;  // option.at_least, the largest of the records

عام:
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    منطقي covered() ثابت { أرجع m_گنتی >= m_at_گھٹ_توں_گھٹ; }
    // METHODS
    فراغ جوڑو_ریکارڈ(ثابت string& name, uint64_t گنتی, uint64_t at_گھٹ_توں_گھٹ) {
        m_name = name;
        m_گنتی += گنتی;
        m_at_گھٹ_توں_گھٹ = std::max(m_at_گھٹ_توں_گھٹ, at_گھٹ_توں_گھٹ);
    }
};

// A coverpoint or cross
صنف Cg_شے final {
    // MEMBERS
    uint64_t m_بھار = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

عام:
    // ACCESSORS
    uint64_t بھار_2() ثابت { أرجع m_بھار; }
    ثابت std::map<string, CgBin>& bins() ثابت { أرجع m_bins; }
    // METHODS
    فراغ جوڑو_ریکارڈ(uint64_t بھار_2) { m_بھار = std::max(m_بھار, بھار_2); }
    CgBin& لبھو_New_Bin(ثابت string& پچھان_تت) { أرجع m_bins[پچھان_تت]; }
};

// A covergroup
صنف Cg_گروہ final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_بھار = 0;  // type_option.weight, the largest of the records
    std::map<string, Cg_شے> m_اشیاء;  // By hierarchy

عام:
    // ACCESSORS
    ثابت string& hier() ثابت { أرجع m_hier; }
    uint64_t بھار_2() ثابت { أرجع m_بھار; }
    ثابت std::map<string, Cg_شے>& اشیاء() ثابت { أرجع m_اشیاء; }
    // METHODS
    فراغ جوڑو_ریکارڈ(ثابت string& hier, uint64_t بھار_2) {
        m_hier = hier;
        m_بھار = std::max(m_بھار, بھار_2);
    }
    Cg_شے& لبھو_New_شے(ثابت string& hier) { أرجع m_اشیاء[hier]; }
};
ورتوں CgGroups = std::map<string, Cg_گروہ>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t کلید_نمبر(ثابت string& قدر) {
    أرجع قدر.خالی_تھاواں() ? 1 : std::strtoull(قدر.c_str(), خالی_سوچک, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
منطقي is_Score_میدان(ثابت string& میدان) {
    لكل (ثابت حرف* ثابت keyp : {VL_CIK_THRESH, VL_CIK_بھار, VL_CIK_گروہ_بھار}) {
        ثابت string اگیتر = string{"\001"} + keyp + "\002";
        إذا (میدان.compare(0, اگیتر.ناپ(), اگیتر) == 0) أرجع سچ;
    }
    أرجع خطأ;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_پچھان_تت(ثابت string& ریکارڈ_Name) {
    string پچھان_تت;
    string::ناپ_type شروع = 0;
    طالما (شروع < ریکارڈ_Name.ناپ()) {
        string::ناپ_type سرا = ریکارڈ_Name.لبھو('\001', شروع + 1);
        إذا (سرا == string::npos) سرا = ریکارڈ_Name.ناپ();
        ثابت string میدان = ریکارڈ_Name.substr(شروع, سرا - شروع);
        إذا (!is_Score_میدان(میدان)) پچھان_تت += میدان;
        شروع = سرا;
    }
    أرجع پچھان_تت;
}

منطقي isCovergroup(ثابت Vlc_نقطہ& نقطہ) { أرجع نقطہ.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    لكل (ثابت VlcPoints::ByName::قدر_type& i : points) {
        ثابت Vlc_نقطہ& pt = points.نقطہ_نمبر(i.دوجا);
        إذا (!isCovergroup(pt)) استمر;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ثابت string hier = pt.hier();
        string bin = pt.bin();
        إذا (bin.خالی_تھاواں() || bin.ناپ() >= hier.ناپ()) bin = hier.substr(hier.rfind('.') + 1);
        ثابت string شے_Hier
            = hier.substr(0, hier.ناپ() - std::min(hier.ناپ(), bin.ناپ() + 1));
        Cg_گروہ& گروہ = groups[pt.ورقہ()];
        گروہ.جوڑو_ریکارڈ(شے_Hier.substr(0, شے_Hier.rfind('.')), کلید_نمبر(pt.گروہ_بھار()));
        Cg_شے& شے_2 = گروہ.لبھو_New_شے(شے_Hier);
        شے_2.جوڑو_ریکارڈ(کلید_نمبر(pt.بھار_2()));
        إذا (!pt.binType().خالی_تھاواں()) استمر;  // Not coverable
        شے_2.لبھو_New_Bin(bin_پچھان_تت(pt.name())).جوڑو_ریکارڈ(bin, pt.گنتی(), کلید_نمبر(pt.thresh()));
    }
    أرجع groups;
}

Tally شے_Tally(ثابت Cg_شے& شے_2) {
    Tally tally;
    لكل (ثابت std::pair<ثابت string, CgBin>& bin : شے_2.bins()) {
        tally.جوڑو_نقطہ(bin.دوجا.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.کل() ? pct(tally.کامیابی(), tally.کل()) : شے_2.بھار_2() ? 0.0 : 100.0);
    أرجع tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally گروہ_Tally(ثابت Cg_گروہ& گروہ, منطقي* contributesp = خالی_سوچک) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    لكل (ثابت std::pair<ثابت string, Cg_شے>& it : گروہ.اشیاء()) {
        ثابت Tally شے_2 = شے_Tally(it.دوجا);
        tally.جوڑو_Points(شے_2);
        إذا (!شے_2.کل()) استمر;  // Excluded from both sums
        weighted += static_cast<مزدوج>(it.دوجا.بھار_2()) * شے_2.score();
        weights += static_cast<مزدوج>(it.دوجا.بھار_2());
    }
    إذا (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : گروہ.بھار_2() ? 0.0 : 100.0);
    أرجع tally;
}

Tally groupsTally(ثابت std::سمتیہ<ثابت Cg_گروہ*>& groups) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    منطقي any_بھار = خطأ;
    لكل (ثابت Cg_گروہ* ثابت groupp : groups) {
        منطقي contributes = خطأ;
        ثابت Tally گروہ = گروہ_Tally(*groupp, &contributes);
        tally.جوڑو_Points(گروہ);
        إذا (groupp->بھار_2()) any_بھار = سچ;
        إذا (!contributes) استمر;
        weighted += static_cast<مزدوج>(groupp->بھار_2()) * گروہ.score();
        weights += static_cast<مزدوج>(groupp->بھار_2());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : any_بھار ? 0.0 : 100.0);
    أرجع tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ثابت CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::سمتیہ<ثابت Cg_گروہ*>> گرہ_Groups;
    لكل (ثابت CgGroups::قدر_type& it : groups) {
        ثابت Cg_گروہ& گروہ = it.دوجا;
        string رستہ;
        لكل (ثابت string& part : ونڈو_Hier(گروہ.hier())) {
            رستہ = رستہ.خالی_تھاواں() ? part : رستہ + "." + part;
            گرہ_Groups[رستہ].ڈالو_پٹھ(&گروہ);
        }
        لكل (ثابت std::pair<ثابت string, Cg_شے>& شے_2 : گروہ.اشیاء()) {
            tallies[شے_2.پہلا] = شے_Tally(شے_2.دوجا);
            لكل (ثابت std::pair<ثابت string, CgBin>& bin : شے_2.دوجا.bins()) {
                // Of the bins of the name
                tallies[شے_2.پہلا + "." + bin.دوجا.name()].جوڑو_نقطہ(bin.دوجا.covered());
            }
        }
    }
    لكل (ثابت std::pair<ثابت string, std::سمتیہ<ثابت Cg_گروہ*>>& it : گرہ_Groups) {
        tallies[it.پہلا]
            = it.دوجا.ناپ() == 1 ? گروہ_Tally(*it.دوجا.front()) : groupsTally(it.دوجا);
    }
    أرجع tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
فراغ printIndent(ناپ_t indent) {
    لكل (ناپ_t i = 0; i < indent; ++i) std::باہر << ' ';
}

فراغ print_Tally_صف(ثابت string& type, ثابت Tally& tally, ناپ_t indent, ناپ_t type_چوڑائی,
                   ناپ_t گنتی_چوڑائی) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ثابت string percent = tally.scored() ? pct_قدر_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.کامیابی(), tally.کل());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::باہر << std::کھبے << std::setw(type_چوڑائی) << type << " : " << std::سجے << std::fixed
              << std::setw(6) << percent << " (" << std::setw(گنتی_چوڑائی) << tally.کامیابی() << "/"
              << std::setw(گنتی_چوڑائی) << tally.کل() << ")\n";
}

ناپ_t گنتی_چوڑائی(ثابت TypeTally& tally) {
    ناپ_t چوڑائی = cvt_منزل_Str(0).ناپ();
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.سرا(); ++it) {
        چوڑائی = std::max(چوڑائی, cvt_منزل_Str(it->دوجا.کامیابی()).ناپ());
        چوڑائی = std::max(چوڑائی, cvt_منزل_Str(it->دوجا.کل()).ناپ());
    }
    أرجع چوڑائی;
}

ناپ_t type_چوڑائی(ثابت TypeTally& tally) {
    ناپ_t type_چوڑائی = 0;
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        type_چوڑائی = std::max(type_چوڑائی, type.ناپ());
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.سرا(); ++it) {
        type_چوڑائی = std::max(type_چوڑائی, it->پہلا.ناپ());
    }
    أرجع type_چوڑائی;
}

فراغ printTypeTally(ثابت TypeTally& tally, ناپ_t indent, منطقي includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ثابت ناپ_t typ_چوڑائی = type_چوڑائی(tally);
    ثابت ناپ_t cnt_چوڑائی = گنتی_چوڑائی(tally);
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        ثابت TypeTally::const_iterator it = tally.لبھو(type);
        إذا (it != tally.سرا()) {
            print_Tally_صف(type, it->دوجا, indent, typ_چوڑائی, cnt_چوڑائی);
        } وإلا إذا (includeMissingOrdered) {
            print_Tally_صف(type, Tally{}, indent, typ_چوڑائی, cnt_چوڑائی);
        }
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.سرا(); ++it) {
        إذا (!isOrderedType(it->پہلا)) {
            print_Tally_صف(it->پہلا, it->دوجا, indent, typ_چوڑائی, cnt_چوڑائی);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
فراغ printCovergroupTallies(ثابت std::map<string, Tally>& tallies, صحيح پدھراں) {
    std::map<string, Tally> shown;
    ناپ_t name_چوڑائی = 0;
    لكل (ثابت std::pair<ثابت string, Tally>& it : tallies) {
        إذا (پدھراں >= 0 && static_cast<صحيح>(ونڈو_Hier(it.پہلا).ناپ()) > پدھراں + 1) استمر;
        shown.داخل_کرو(it);
        name_چوڑائی = std::max(name_چوڑائی, it.پہلا.ناپ());
    }
    ثابت ناپ_t cnt_چوڑائی = گنتی_چوڑائی(shown);
    std::باہر << "Covergroup Coverage Summary:\n";
    لكل (ثابت std::pair<ثابت string, Tally>& it : shown) {
        print_Tally_صف(it.پہلا, it.دوجا, s_summaryIndent, name_چوڑائی, cnt_چوڑائی);
    }
}

}  // namespace

فراغ Vlc_چوٹی::readCoverage(ثابت string& filename, منطقي nonfatal) {
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
            ثابت string::ناپ_type secspace = سطر.rfind("' ");
            إذا (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                استمر;
            }
            ثابت string نقطہ = سطر.substr(3, secspace - 3);
            إذا (!opt.is_Type_میل(نقطہ.c_str())) استمر;

            ثابت uint64_t کامیابیاں = std::atoll(سطر.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ثابت uint64_t pointnum = points().لبھو_جوڑو_نقطہ(نقطہ, کامیابیاں);
            إذا (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ثابت uint64_t needed = std::max(
                    VlcBuckets::sufficient(), کلید_نمبر(points().نقطہ_نمبر(pointnum).thresh()));
                إذا (کامیابیاں >= needed) {
                    points().نقطہ_نمبر(pointnum).testsCoveringInc();
                    testp->buckets().جوڑو_انکڑے(pointnum, کامیابیاں);
                }
            }
        }
    }
}

فراغ Vlc_چوٹی::writeCoverage(ثابت string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    إذا (!os) {
        v3fatal("Can't write file: " << filename);
        أرجع;
    }

    os << "# SystemC::Coverage-3\n";
    لكل (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطہ& نقطہ = m_points.نقطہ_نمبر(i.دوجا);
        os << "C '" << نقطہ.name() << "' " << نقطہ.گنتی() << '\n';
    }
}

فراغ Vlc_چوٹی::writeInfo(ثابت string& filename) {
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
        Vlc_سروت& سروت = si.دوجا;
        os << "SF:" << سروت.name() << '\n';
        Vlc_سروت::LinenoMap& سطریں = سروت.سطریں();
        صحيح branches_لبھیا = 0;
        صحيح branches_کامیابی = 0;
        لكل (خودکار& li : سطریں) {
            Vlc_سروت_گنتی& sc = li.دوجا;
            uint64_t da_گنتی = 0;
            std::سمتیہ<ثابت Vlc_نقطہ*> infoPoints;
            لكل (ثابت خودکار& نقطہ : sc.points()) {
                da_گنتی = std::max(da_گنتی, نقطہ->گنتی());
                infoPoints.ڈالو_پٹھ(نقطہ);
            }
            os << "DA:" << sc.lineno() << "," << da_گنتی << "\n";
            إذا (infoPoints.ناپ() <= 1) استمر;
            branches_لبھیا += static_cast<صحيح>(infoPoints.ناپ());
            صحيح نقطہ_عدد_2 = 0;
            لكل (ثابت Vlc_نقطہ* نقطہ : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                إذا (نقطہ->isFsmArc()) {
                    os << "2,";
                    os << نقطہ->fsmFromState() << "->" << نقطہ->fsm_منزل_State();
                } وإلا إذا (نقطہ->comment().خالی_تھاواں()) {
                    os << "0,";
                    os << نقطہ_عدد_2;
                } وإلا {
                    os << (نقطہ->isFsmState() ? '1' : '0') << ',';
                    std::string comment(نقطہ->comment());
                    std::replace(comment.begin(), comment.سرا(), ',', '_');
                    os << comment;
                }
                os << "," << نقطہ->گنتی() << "\n";

                branches_کامیابی += opt.گنتی_Ok(نقطہ->گنتی());
                ++نقطہ_عدد_2;
            }
        }
        os << "BRF:" << branches_لبھیا << '\n';
        os << "BRH:" << branches_کامیابی << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

بنية CmpComputrons final {
    منطقي عامل()(ثابت VlcTest* lhsp, ثابت VlcTest* rhsp) ثابت {
        إذا (lhsp->computrons() != rhsp->computrons()) {
            أرجع lhsp->computrons() < rhsp->computrons();
        }
        أرجع lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

فراغ Vlc_چوٹی::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::سمتیہ<VlcTest*> bytime;
    لكل (ثابت خودکار& testp : m_tests) {
        إذا (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ڈالو_پٹھ(testp);
        }
    }
    ترتیب_دیو(bytime.begin(), bytime.سرا(), CmpComputrons());  // Sort the vector

    VlcBuckets بچیا;
    لكل (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطہ* ثابت pointp = &points().نقطہ_نمبر(i.دوجا);
        // If any tests hit this point, then we'll need to cover it.
        إذا (pointp->testsCovering()) بچیا.جوڑو_انکڑے(pointp->نقطہ_عدد(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    طالما (سچ) {
        إذا (debug() >= 9) {
            UINFO_اگیتر("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            بچیا.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* بہترین_Testp = خالی_سوچک;
        uint64_t بہترین_Remain = 0;
        لكل (ثابت خودکار& testp : bytime) {
            إذا (!testp->rank()) {
                ثابت uint64_t remain = testp->buckets().انکڑے_نکالو_گنتی(بچیا);
                إذا (remain > بہترین_Remain) {
                    بہترین_Testp = testp;
                    بہترین_Remain = remain;
                }
            }
        }
        إذا (VlcTest* ثابت testp = بہترین_Testp) {
            testp->rank(nextrank++);
            testp->rankPoints(بہترین_Remain);
            بچیا.or_انکڑے(بہترین_Testp->buckets());
        } وإلا {
            اكسر;  // No test covering more stuff found
        }
    }
}

فراغ Vlc_چوٹی::annotateCalc() {
    // Calculate per-line information into filedata structure
    لكل (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطہ& نقطہ = m_points.نقطہ_نمبر(i.دوجا);
        ثابت string filename = نقطہ.filename();
        ثابت صحيح lineno = نقطہ.lineno();
        إذا (!filename.خالی_تھاواں() && lineno != 0) {
            Vlc_سروت& سروت = sources().لبھو_New_سروت(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << نقطہ.کالم_2() << " "
                                       << نقطہ.گنتی() << " " << نقطہ.linescov());
            // Base coverage
            سروت.داخل_کرو_نقطہ(lineno, &نقطہ);
            // Additional lines covered by this statement
            منطقي range = خطأ;
            صحيح شروع = 0;
            صحيح سرا = 0;
            ثابت string linescov = نقطہ.linescov();
            لكل (ثابت حرف* covp = linescov.c_str(); سچ; ++covp) {
                إذا (!*covp || *covp == ',') {  // Ending
                    لكل (صحيح lni = شروع; شروع && lni <= سرا; ++lni) {
                        سروت.داخل_کرو_نقطہ(lni, &نقطہ);
                    }
                    إذا (!*covp) اكسر;
                    شروع = 0;  // Prep for next
                    سرا = 0;
                    range = خطأ;
                } وإلا إذا (*covp == '-') {
                    range = سچ;
                } وإلا إذا (std::ہندسہ_اے(*covp)) {
                    ثابت حرف* ثابت digitsp = covp;
                    طالما (std::ہندسہ_اے(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    إذا (!range) شروع = std::atoi(digitsp);
                    سرا = std::atoi(digitsp);
                }
            }
        }
    }
}

فراغ Vlc_چوٹی::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    صحيح totCases = 0;
    صحيح totOk = 0;
    لكل (خودکار& si : m_sources) {
        Vlc_سروت& سروت = si.دوجا;
        // UINFO(1, "Source " << source.name());
        إذا (opt.annotateAll()) سروت.needed(سچ);
        ثابت Vlc_سروت::LinenoMap& سطریں = سروت.سطریں();
        لكل (خودکار& li : سطریں) {
            ثابت Vlc_سروت_گنتی& sc = li.دوجا;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            إذا (opt.گنتی_Ok(sc.min_گنتی())) {
                ++totOk;
            } وإلا {
                سروت.needed(سچ);
            }
        }
    }
    std::باہر << "Annotation Summary:\n";
    std::باہر << "  lines with all attached points covered : ";
    std::باہر << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    إذا (totOk != totCases) باہر << "See lines with '%00' in " << opt.annotate_باہرلا() << '\n';
}

فراغ Vlc_چوٹی::annotateOutputFiles(ثابت string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    لكل (خودکار& si : m_sources) {
        Vlc_سروت& سروت = si.دوجا;
        إذا (!سروت.needed()) استمر;
        ثابت string filename = سروت.name();
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

            Vlc_سروت::LinenoMap& سطریں = سروت.سطریں();
            ثابت خودکار lit = سطریں.لبھو(lineno);
            إذا (lit == سطریں.سرا()) {
                os << "        " << سطر << '\n';
            } وإلا {
                Vlc_سروت_گنتی& sc = lit->دوجا;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ثابت منطقي minOk = opt.گنتی_Ok(sc.min_گنتی());
                ثابت منطقي maxOk = opt.گنتی_Ok(sc.max_گنتی());
                إذا (minOk) {
                    os << " ";
                } وإلا إذا (maxOk) {
                    os << "~";
                } وإلا {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_گنتی() << " " << سطر << '\n';

                إذا (opt.annotatePoints()) {
                    لكل (ثابت خودکار& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                منطقي printed_Fsm_سرخی = خطأ;
                لكل (ثابت خودکار& pit : sc.points()) {
                    إذا (!pit->isFsmState() && !pit->isFsmArc()) استمر;
                    إذا (!printed_Fsm_سرخی) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_سرخی = سچ;
                    }
                    os << (opt.گنتی_Ok(pit->گنتی()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->گنتی() << "        ";
                    إذا (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        إذا (pit->گنتی() == 0) os << " *** UNCOVERED ***";
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

فراغ Vlc_چوٹی::annotate(ثابت string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

فراغ Vlc_چوٹی::printTypeSummary() {
    TypeTally tally;
    لكل (VlcPoints::ByName::قدر_type& i : m_points) {
        ثابت Vlc_نقطہ& pt = m_points.نقطہ_نمبر(i.دوجا);
        إذا (isCovergroup(pt)) استمر;  // Tallied below, by covergroup
        tally_نقطہ(tally, displayType(pt), pt.گنتی());
    }
    ثابت CgGroups groups = covergroups(m_points);
    إذا (!groups.خالی_تھاواں()) {
        std::سمتیہ<ثابت Cg_گروہ*> groupps;
        لكل (ثابت CgGroups::قدر_type& it : groups) groupps.ڈالو_پٹھ(&it.دوجا);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    إذا (tally.خالی_تھاواں()) أرجع;
    std::باہر << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, سچ);
}

فراغ Vlc_چوٹی::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    منطقي hasHier = خطأ;
    منطقي hasCollapsedHier = خطأ;
    لكل (VlcPoints::ByName::قدر_type& i : m_points) {
        ثابت Vlc_نقطہ& pt = m_points.نقطہ_نمبر(i.دوجا);
        إذا (isCovergroup(pt)) استمر;  // Reported below, by covergroup
        ثابت string hier = reportHier(pt);
        إذا (hier.خالی_تھاواں()) استمر;
        hasHier = سچ;
        إذا (isCollapsedHier(hier)) hasCollapsedHier = سچ;
        ثابت string type = displayType(pt);
        ثابت std::سمتیہ<string> parts = ونڈو_Hier(hier);
        string رستہ;
        لكل (std::سمتیہ<string>::const_iterator it = parts.begin(); it != parts.سرا(); ++it) {
            رستہ = رستہ.خالی_تھاواں() ? *it : رستہ + "." + *it;
            tally_نقطہ(hierTallies[رستہ], type, pt.گنتی());
        }
        tally_نقطہ(duTallies[duName(pt)], type, pt.گنتی());
    }
    ثابت std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    إذا (!hasHier && !cgTallies.خالی_تھاواں()) {
        printCovergroupTallies(cgTallies, opt.report_پدھراں());
        أرجع;
    }
    إذا (!hasHier) {
        std::باہر << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        أرجع;
    }

    ثابت صحيح پدھراں = opt.report_پدھراں();
    إذا (hasCollapsedHier) {
        std::باہر << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::باہر << "Hierarchy Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.سرا(); ++it) {
        ثابت std::سمتیہ<string> parts = ونڈو_Hier(it->پہلا);
        إذا (پدھراں >= 0 && static_cast<صحيح>(parts.ناپ()) > پدھراں + 1) استمر;
        printIndent(s_summaryIndent);
        std::باہر << it->پہلا << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->دوجا, s_report_صف_Indent, خطأ);
    }
    std::باہر << "Design Unit Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.سرا();
         ++it) {
        printIndent(s_summaryIndent);
        std::باہر << it->پہلا << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->دوجا, s_report_صف_Indent, خطأ);
    }
    إذا (!cgTallies.خالی_تھاواں()) printCovergroupTallies(cgTallies, پدھراں);
}
