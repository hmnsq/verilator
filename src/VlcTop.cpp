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

#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

#شامل "VlcTop.h"

#شامل "V3Error.h"
#شامل "V3Os.h"

#شامل "VlcOptions.h"

#شامل <algorithm>
#شامل <fstream>
#شامل <iomanip>
#شامل <map>
#شامل <set>
#شامل <sstream>
#شامل <string>
#شامل <vector>

//######################################################################

نامخلا {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
جماعت Tally final {
    // MEMBERS
    uint64_t m_کامیابی = 0;  // Covered points
    uint64_t m_کل = 0;  // Points
    منطقی m_scored = جھوٹ;  // Coverage is m_score, not the ratio of the points
    دوہرا m_score = 0.0;  // Percent coverage, if m_scored

عوامی:
    // ACCESSORS
    uint64_t کامیابی() ثابت { واپس m_کامیابی; }
    uint64_t کل() ثابت { واپس m_کل; }
    منطقی scored() ثابت { واپس m_scored; }
    دوہرا score() ثابت { واپس m_score; }
    خالی score(دوہرا قدر) {
        m_scored = سچ;
        m_score = قدر;
    }
    // METHODS
    خالی جوڑو_نقطہ(منطقی covered) {
        اگر (covered) ++m_کامیابی;
        ++m_کل;
    }
    خالی جوڑو_Points(ثابت Tally& other) {
        m_کامیابی += other.m_کامیابی;
        m_کل += other.m_کل;
    }
};

// Map coverage type to its tally.
استعمال TypeTally = std::map<std::string, Tally>;

ساکن constexpr ثابت حرف* ثابت s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ساکن constexpr ثابت حرف* ثابت s_covergroupType = "covergroup";
ساکن constexpr سائز_t s_summaryIndent = 2;
ساکن constexpr سائز_t s_report_صف_Indent = 4;

string displayType(ثابت Vlc_نقطہ& نقطہ) {
    ثابت string type = نقطہ.type();
    واپس type.خالی_خانے() ? "point" : type;
}

منطقی isCollapsedHier(ثابت string& hier) {
    واپس hier.ڈھونڈو('*') != string::npos || hier.ڈھونڈو('?') != string::npos;
}

منطقی isOrderedType(ثابت string& type) {
    برائے (ثابت حرف* ثابت typep : s_orderedTypes) {
        اگر (type == typep) واپس سچ;
    }
    واپس جھوٹ;
}

string reportHier(ثابت Vlc_نقطہ& نقطہ) {
    // FSM records currently have useful instance scope in Fv.
    اگر (نقطہ.isFsmState() || نقطہ.isFsmArc()) {
        ثابت string fsmVar = نقطہ.fsmVarName();
        واپس fsmVar.substr(0, fsmVar.rfind('.'));
    }
    واپس نقطہ.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::سمتیہ<string> بانٹو_Hier(ثابت string& hier) {
    std::سمتیہ<string> parts;
    string::سائز_type آغاز = 0;
    صحیح گہرائی = 0;  // Of the parentheses of the values of parameters
    string::سائز_type pos = 0;
    جب (pos < hier.سائز()) {
        ثابت حرف c = hier[pos];
        اگر (c == '\\') {
            pos = hier.ڈھونڈو(' ', pos);  // npos if not ended
            جاری;
        } ورنہ اگر (!گہرائی && hier.compare(pos, 2, "#(") == 0) {
            گہرائی = 1;
            ++pos;
        } ورنہ اگر (!گہرائی && c == '.') {
            parts.ڈالو_عقب(hier.substr(آغاز, pos - آغاز));
            آغاز = pos + 1;
        } ورنہ اگر (گہرائی && c == '"') {
            pos = VString::quoted_انتہا(hier, pos);  // npos if not terminated
            جاری;
        } ورنہ اگر (گہرائی && c == '(') {
            ++گہرائی;
        } ورنہ اگر (گہرائی && c == ')') {
            --گہرائی;
        }
        ++pos;
    }
    اگر (گہرائی || pos != hier.سائز()) {  // Unbalanced
        parts.clear();
        آغاز = 0;
        برائے (string::سائز_type dot; (dot = hier.ڈھونڈو('.', آغاز)) != string::npos;
             آغاز = dot + 1) {
            parts.ڈالو_عقب(hier.substr(آغاز, dot - آغاز));
        }
    }
    parts.ڈالو_عقب(hier.substr(آغاز));
    واپس parts;
}

string duName(ثابت Vlc_نقطہ& نقطہ) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ثابت string صفحہ = نقطہ.صفحہ();
    واپس صفحہ.substr(صفحہ.ڈھونڈو('/') + 1);
}

خالی tally_نقطہ(TypeTally& tally, ثابت string& type, uint64_t گنتی) {
    tally[type].جوڑو_نقطہ(گنتی > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
دوہرا pct(uint64_t کامیابی, uint64_t کل) {
    واپس کل ? (100.0 * static_cast<دوہرا>(کامیابی) / static_cast<دوہرا>(کل)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_قدر_String(دوہرا قدر, منطقی complete) {
    اگر (!complete && قدر > 99.9) قدر = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << قدر << "%";
    واپس os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t کامیابی, uint64_t کل) {
    واپس pct_قدر_String(pct(کامیابی, کل), کامیابی >= کل);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
جماعت CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_گنتی = 0;  // Hits, of all the records
    uint64_t m_at_کم_سے_کم = 0;  // option.at_least, the largest of the records

عوامی:
    // ACCESSORS
    ثابت string& name() ثابت { واپس m_name; }
    منطقی covered() ثابت { واپس m_گنتی >= m_at_کم_سے_کم; }
    // METHODS
    خالی جوڑو_ریکارڈ(ثابت string& name, uint64_t گنتی, uint64_t at_کم_سے_کم) {
        m_name = name;
        m_گنتی += گنتی;
        m_at_کم_سے_کم = std::max(m_at_کم_سے_کم, at_کم_سے_کم);
    }
};

// A coverpoint or cross
جماعت Cg_شے final {
    // MEMBERS
    uint64_t m_وزن = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

عوامی:
    // ACCESSORS
    uint64_t وزن_2() ثابت { واپس m_وزن; }
    ثابت std::map<string, CgBin>& bins() ثابت { واپس m_bins; }
    // METHODS
    خالی جوڑو_ریکارڈ(uint64_t وزن_2) { m_وزن = std::max(m_وزن, وزن_2); }
    CgBin& ڈھونڈو_New_Bin(ثابت string& شناختی_عنصر) { واپس m_bins[شناختی_عنصر]; }
};

// A covergroup
جماعت Cg_گروہ final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_وزن = 0;  // type_option.weight, the largest of the records
    std::map<string, Cg_شے> m_اشیاء;  // By hierarchy

عوامی:
    // ACCESSORS
    ثابت string& hier() ثابت { واپس m_hier; }
    uint64_t وزن_2() ثابت { واپس m_وزن; }
    ثابت std::map<string, Cg_شے>& اشیاء() ثابت { واپس m_اشیاء; }
    // METHODS
    خالی جوڑو_ریکارڈ(ثابت string& hier, uint64_t وزن_2) {
        m_hier = hier;
        m_وزن = std::max(m_وزن, وزن_2);
    }
    Cg_شے& ڈھونڈو_New_شے(ثابت string& hier) { واپس m_اشیاء[hier]; }
};
استعمال CgGroups = std::map<string, Cg_گروہ>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t کلید_نمبر(ثابت string& قدر) {
    واپس قدر.خالی_خانے() ? 1 : std::strtoull(قدر.c_str(), خالی_اشاریہ, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
منطقی is_Score_میدان(ثابت string& میدان) {
    برائے (ثابت حرف* ثابت keyp : {VL_CIK_THRESH, VL_CIK_وزن, VL_CIK_گروہ_وزن}) {
        ثابت string سابقہ = string{"\001"} + keyp + "\002";
        اگر (میدان.compare(0, سابقہ.سائز(), سابقہ) == 0) واپس سچ;
    }
    واپس جھوٹ;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_شناختی_عنصر(ثابت string& ریکارڈ_Name) {
    string شناختی_عنصر;
    string::سائز_type آغاز = 0;
    جب (آغاز < ریکارڈ_Name.سائز()) {
        string::سائز_type انتہا = ریکارڈ_Name.ڈھونڈو('\001', آغاز + 1);
        اگر (انتہا == string::npos) انتہا = ریکارڈ_Name.سائز();
        ثابت string میدان = ریکارڈ_Name.substr(آغاز, انتہا - آغاز);
        اگر (!is_Score_میدان(میدان)) شناختی_عنصر += میدان;
        آغاز = انتہا;
    }
    واپس شناختی_عنصر;
}

منطقی isCovergroup(ثابت Vlc_نقطہ& نقطہ) { واپس نقطہ.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    برائے (ثابت VlcPoints::ByName::قدر_type& i : points) {
        ثابت Vlc_نقطہ& pt = points.نقطہ_نمبر(i.دوسرا);
        اگر (!isCovergroup(pt)) جاری;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ثابت string hier = pt.hier();
        string bin = pt.bin();
        اگر (bin.خالی_خانے() || bin.سائز() >= hier.سائز()) bin = hier.substr(hier.rfind('.') + 1);
        ثابت string شے_Hier
            = hier.substr(0, hier.سائز() - std::min(hier.سائز(), bin.سائز() + 1));
        Cg_گروہ& گروہ = groups[pt.صفحہ()];
        گروہ.جوڑو_ریکارڈ(شے_Hier.substr(0, شے_Hier.rfind('.')), کلید_نمبر(pt.گروہ_وزن()));
        Cg_شے& شے_3 = گروہ.ڈھونڈو_New_شے(شے_Hier);
        شے_3.جوڑو_ریکارڈ(کلید_نمبر(pt.وزن_2()));
        اگر (!pt.binType().خالی_خانے()) جاری;  // Not coverable
        شے_3.ڈھونڈو_New_Bin(bin_شناختی_عنصر(pt.name())).جوڑو_ریکارڈ(bin, pt.گنتی(), کلید_نمبر(pt.thresh()));
    }
    واپس groups;
}

Tally شے_Tally(ثابت Cg_شے& شے_3) {
    Tally tally;
    برائے (ثابت std::pair<ثابت string, CgBin>& bin : شے_3.bins()) {
        tally.جوڑو_نقطہ(bin.دوسرا.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.کل() ? pct(tally.کامیابی(), tally.کل()) : شے_3.وزن_2() ? 0.0 : 100.0);
    واپس tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally گروہ_Tally(ثابت Cg_گروہ& گروہ, منطقی* contributesp = خالی_اشاریہ) {
    Tally tally;
    دوہرا weighted = 0.0;
    دوہرا weights = 0.0;
    برائے (ثابت std::pair<ثابت string, Cg_شے>& it : گروہ.اشیاء()) {
        ثابت Tally شے_3 = شے_Tally(it.دوسرا);
        tally.جوڑو_Points(شے_3);
        اگر (!شے_3.کل()) جاری;  // Excluded from both sums
        weighted += static_cast<دوہرا>(it.دوسرا.وزن_2()) * شے_3.score();
        weights += static_cast<دوہرا>(it.دوسرا.وزن_2());
    }
    اگر (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : گروہ.وزن_2() ? 0.0 : 100.0);
    واپس tally;
}

Tally groupsTally(ثابت std::سمتیہ<ثابت Cg_گروہ*>& groups) {
    Tally tally;
    دوہرا weighted = 0.0;
    دوہرا weights = 0.0;
    منطقی any_وزن = جھوٹ;
    برائے (ثابت Cg_گروہ* ثابت groupp : groups) {
        منطقی contributes = جھوٹ;
        ثابت Tally گروہ = گروہ_Tally(*groupp, &contributes);
        tally.جوڑو_Points(گروہ);
        اگر (groupp->وزن_2()) any_وزن = سچ;
        اگر (!contributes) جاری;
        weighted += static_cast<دوہرا>(groupp->وزن_2()) * گروہ.score();
        weights += static_cast<دوہرا>(groupp->وزن_2());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : any_وزن ? 0.0 : 100.0);
    واپس tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ثابت CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::سمتیہ<ثابت Cg_گروہ*>> گرہ_Groups;
    برائے (ثابت CgGroups::قدر_type& it : groups) {
        ثابت Cg_گروہ& گروہ = it.دوسرا;
        string راستہ;
        برائے (ثابت string& part : بانٹو_Hier(گروہ.hier())) {
            راستہ = راستہ.خالی_خانے() ? part : راستہ + "." + part;
            گرہ_Groups[راستہ].ڈالو_عقب(&گروہ);
        }
        برائے (ثابت std::pair<ثابت string, Cg_شے>& شے_3 : گروہ.اشیاء()) {
            tallies[شے_3.پہلا] = شے_Tally(شے_3.دوسرا);
            برائے (ثابت std::pair<ثابت string, CgBin>& bin : شے_3.دوسرا.bins()) {
                // Of the bins of the name
                tallies[شے_3.پہلا + "." + bin.دوسرا.name()].جوڑو_نقطہ(bin.دوسرا.covered());
            }
        }
    }
    برائے (ثابت std::pair<ثابت string, std::سمتیہ<ثابت Cg_گروہ*>>& it : گرہ_Groups) {
        tallies[it.پہلا]
            = it.دوسرا.سائز() == 1 ? گروہ_Tally(*it.دوسرا.front()) : groupsTally(it.دوسرا);
    }
    واپس tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
خالی printIndent(سائز_t indent) {
    برائے (سائز_t i = 0; i < indent; ++i) std::خارج << ' ';
}

خالی print_Tally_صف(ثابت string& type, ثابت Tally& tally, سائز_t indent, سائز_t type_چوڑائی,
                   سائز_t گنتی_چوڑائی) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ثابت string percent = tally.scored() ? pct_قدر_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.کامیابی(), tally.کل());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::خارج << std::بائیں << std::setw(type_چوڑائی) << type << " : " << std::دائیں << std::fixed
              << std::setw(6) << percent << " (" << std::setw(گنتی_چوڑائی) << tally.کامیابی() << "/"
              << std::setw(گنتی_چوڑائی) << tally.کل() << ")\n";
}

سائز_t گنتی_چوڑائی(ثابت TypeTally& tally) {
    سائز_t چوڑائی = cvt_مقصد_Str(0).سائز();
    برائے (TypeTally::const_iterator it = tally.begin(); it != tally.انتہا(); ++it) {
        چوڑائی = std::max(چوڑائی, cvt_مقصد_Str(it->دوسرا.کامیابی()).سائز());
        چوڑائی = std::max(چوڑائی, cvt_مقصد_Str(it->دوسرا.کل()).سائز());
    }
    واپس چوڑائی;
}

سائز_t type_چوڑائی(ثابت TypeTally& tally) {
    سائز_t type_چوڑائی = 0;
    برائے (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        type_چوڑائی = std::max(type_چوڑائی, type.سائز());
    }
    برائے (TypeTally::const_iterator it = tally.begin(); it != tally.انتہا(); ++it) {
        type_چوڑائی = std::max(type_چوڑائی, it->پہلا.سائز());
    }
    واپس type_چوڑائی;
}

خالی printTypeTally(ثابت TypeTally& tally, سائز_t indent, منطقی includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ثابت سائز_t typ_چوڑائی = type_چوڑائی(tally);
    ثابت سائز_t cnt_چوڑائی = گنتی_چوڑائی(tally);
    برائے (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        ثابت TypeTally::const_iterator it = tally.ڈھونڈو(type);
        اگر (it != tally.انتہا()) {
            print_Tally_صف(type, it->دوسرا, indent, typ_چوڑائی, cnt_چوڑائی);
        } ورنہ اگر (includeMissingOrdered) {
            print_Tally_صف(type, Tally{}, indent, typ_چوڑائی, cnt_چوڑائی);
        }
    }
    برائے (TypeTally::const_iterator it = tally.begin(); it != tally.انتہا(); ++it) {
        اگر (!isOrderedType(it->پہلا)) {
            print_Tally_صف(it->پہلا, it->دوسرا, indent, typ_چوڑائی, cnt_چوڑائی);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
خالی printCovergroupTallies(ثابت std::map<string, Tally>& tallies, صحیح سطحیں) {
    std::map<string, Tally> shown;
    سائز_t name_چوڑائی = 0;
    برائے (ثابت std::pair<ثابت string, Tally>& it : tallies) {
        اگر (سطحیں >= 0 && static_cast<صحیح>(بانٹو_Hier(it.پہلا).سائز()) > سطحیں + 1) جاری;
        shown.داخل_کرو(it);
        name_چوڑائی = std::max(name_چوڑائی, it.پہلا.سائز());
    }
    ثابت سائز_t cnt_چوڑائی = گنتی_چوڑائی(shown);
    std::خارج << "Covergroup Coverage Summary:\n";
    برائے (ثابت std::pair<ثابت string, Tally>& it : shown) {
        print_Tally_صف(it.پہلا, it.دوسرا, s_summaryIndent, name_چوڑائی, cnt_چوڑائی);
    }
}

}  // namespace

خالی Vlc_چوٹی::readCoverage(ثابت string& filename, منطقی nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    اگر (!is) {
        اگر (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        واپس;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ثابت testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    جب (!is.eof()) {
        ثابت string سطر = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        اگر (سطر[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ثابت string::سائز_type secspace = سطر.rfind("' ");
            اگر (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                جاری;
            }
            ثابت string نقطہ = سطر.substr(3, secspace - 3);
            اگر (!opt.is_Type_مطابقت(نقطہ.c_str())) جاری;

            ثابت uint64_t کامیابیاں = std::atoll(سطر.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ثابت uint64_t pointnum = points().ڈھونڈو_جوڑو_نقطہ(نقطہ, کامیابیاں);
            اگر (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ثابت uint64_t needed = std::max(
                    VlcBuckets::sufficient(), کلید_نمبر(points().نقطہ_نمبر(pointnum).thresh()));
                اگر (کامیابیاں >= needed) {
                    points().نقطہ_نمبر(pointnum).testsCoveringInc();
                    testp->buckets().جوڑو_ڈیٹا(pointnum, کامیابیاں);
                }
            }
        }
    }
}

خالی Vlc_چوٹی::writeCoverage(ثابت string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    اگر (!os) {
        v3fatal("Can't write file: " << filename);
        واپس;
    }

    os << "# SystemC::Coverage-3\n";
    برائے (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطہ& نقطہ = m_points.نقطہ_نمبر(i.دوسرا);
        os << "C '" << نقطہ.name() << "' " << نقطہ.گنتی() << '\n';
    }
}

خالی Vlc_چوٹی::writeInfo(ثابت string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    اگر (!os) {
        v3fatal("Can't write file: " << filename);
        واپس;
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
    برائے (خودکار& si : m_sources) {
        Vlc_ماخذ& ماخذ = si.دوسرا;
        os << "SF:" << ماخذ.name() << '\n';
        Vlc_ماخذ::LinenoMap& سطریں = ماخذ.سطریں();
        صحیح branches_ملا = 0;
        صحیح branches_کامیابی = 0;
        برائے (خودکار& li : سطریں) {
            Vlc_ماخذ_گنتی& sc = li.دوسرا;
            uint64_t da_گنتی = 0;
            std::سمتیہ<ثابت Vlc_نقطہ*> infoPoints;
            برائے (ثابت خودکار& نقطہ : sc.points()) {
                da_گنتی = std::max(da_گنتی, نقطہ->گنتی());
                infoPoints.ڈالو_عقب(نقطہ);
            }
            os << "DA:" << sc.lineno() << "," << da_گنتی << "\n";
            اگر (infoPoints.سائز() <= 1) جاری;
            branches_ملا += static_cast<صحیح>(infoPoints.سائز());
            صحیح نقطہ_عدد_2 = 0;
            برائے (ثابت Vlc_نقطہ* نقطہ : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                اگر (نقطہ->isFsmArc()) {
                    os << "2,";
                    os << نقطہ->fsmFromState() << "->" << نقطہ->fsm_مقصد_State();
                } ورنہ اگر (نقطہ->comment().خالی_خانے()) {
                    os << "0,";
                    os << نقطہ_عدد_2;
                } ورنہ {
                    os << (نقطہ->isFsmState() ? '1' : '0') << ',';
                    std::string comment(نقطہ->comment());
                    std::replace(comment.begin(), comment.انتہا(), ',', '_');
                    os << comment;
                }
                os << "," << نقطہ->گنتی() << "\n";

                branches_کامیابی += opt.گنتی_Ok(نقطہ->گنتی());
                ++نقطہ_عدد_2;
            }
        }
        os << "BRF:" << branches_ملا << '\n';
        os << "BRH:" << branches_کامیابی << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

ساخت CmpComputrons final {
    منطقی عامل()(ثابت VlcTest* lhsp, ثابت VlcTest* rhsp) ثابت {
        اگر (lhsp->computrons() != rhsp->computrons()) {
            واپس lhsp->computrons() < rhsp->computrons();
        }
        واپس lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

خالی Vlc_چوٹی::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::سمتیہ<VlcTest*> bytime;
    برائے (ثابت خودکار& testp : m_tests) {
        اگر (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ڈالو_عقب(testp);
        }
    }
    ترتیب_دو(bytime.begin(), bytime.انتہا(), CmpComputrons());  // Sort the vector

    VlcBuckets بقایا;
    برائے (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطہ* ثابت pointp = &points().نقطہ_نمبر(i.دوسرا);
        // If any tests hit this point, then we'll need to cover it.
        اگر (pointp->testsCovering()) بقایا.جوڑو_ڈیٹا(pointp->نقطہ_عدد(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    جب (سچ) {
        اگر (debug() >= 9) {
            UINFO_سابقہ("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            بقایا.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* بہترین_Testp = خالی_اشاریہ;
        uint64_t بہترین_Remain = 0;
        برائے (ثابت خودکار& testp : bytime) {
            اگر (!testp->rank()) {
                ثابت uint64_t remain = testp->buckets().ڈیٹا_نکالو_گنتی(بقایا);
                اگر (remain > بہترین_Remain) {
                    بہترین_Testp = testp;
                    بہترین_Remain = remain;
                }
            }
        }
        اگر (VlcTest* ثابت testp = بہترین_Testp) {
            testp->rank(nextrank++);
            testp->rankPoints(بہترین_Remain);
            بقایا.or_ڈیٹا(بہترین_Testp->buckets());
        } ورنہ {
            توڑو;  // No test covering more stuff found
        }
    }
}

خالی Vlc_چوٹی::annotateCalc() {
    // Calculate per-line information into filedata structure
    برائے (ثابت خودکار& i : m_points) {
        ثابت Vlc_نقطہ& نقطہ = m_points.نقطہ_نمبر(i.دوسرا);
        ثابت string filename = نقطہ.filename();
        ثابت صحیح lineno = نقطہ.lineno();
        اگر (!filename.خالی_خانے() && lineno != 0) {
            Vlc_ماخذ& ماخذ = sources().ڈھونڈو_New_ماخذ(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << نقطہ.عمود_2() << " "
                                       << نقطہ.گنتی() << " " << نقطہ.linescov());
            // Base coverage
            ماخذ.داخل_کرو_نقطہ(lineno, &نقطہ);
            // Additional lines covered by this statement
            منطقی range = جھوٹ;
            صحیح آغاز = 0;
            صحیح انتہا = 0;
            ثابت string linescov = نقطہ.linescov();
            برائے (ثابت حرف* covp = linescov.c_str(); سچ; ++covp) {
                اگر (!*covp || *covp == ',') {  // Ending
                    برائے (صحیح lni = آغاز; آغاز && lni <= انتہا; ++lni) {
                        ماخذ.داخل_کرو_نقطہ(lni, &نقطہ);
                    }
                    اگر (!*covp) توڑو;
                    آغاز = 0;  // Prep for next
                    انتہا = 0;
                    range = جھوٹ;
                } ورنہ اگر (*covp == '-') {
                    range = سچ;
                } ورنہ اگر (std::ہندسہ_ہے(*covp)) {
                    ثابت حرف* ثابت digitsp = covp;
                    جب (std::ہندسہ_ہے(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    اگر (!range) آغاز = std::atoi(digitsp);
                    انتہا = std::atoi(digitsp);
                }
            }
        }
    }
}

خالی Vlc_چوٹی::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    صحیح totCases = 0;
    صحیح totOk = 0;
    برائے (خودکار& si : m_sources) {
        Vlc_ماخذ& ماخذ = si.دوسرا;
        // UINFO(1, "Source " << source.name());
        اگر (opt.annotateAll()) ماخذ.needed(سچ);
        ثابت Vlc_ماخذ::LinenoMap& سطریں = ماخذ.سطریں();
        برائے (خودکار& li : سطریں) {
            ثابت Vlc_ماخذ_گنتی& sc = li.دوسرا;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            اگر (opt.گنتی_Ok(sc.min_گنتی())) {
                ++totOk;
            } ورنہ {
                ماخذ.needed(سچ);
            }
        }
    }
    std::خارج << "Annotation Summary:\n";
    std::خارج << "  lines with all attached points covered : ";
    std::خارج << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    اگر (totOk != totCases) خارج << "See lines with '%00' in " << opt.annotate_برآمد() << '\n';
}

خالی Vlc_چوٹی::annotateOutputFiles(ثابت string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    برائے (خودکار& si : m_sources) {
        Vlc_ماخذ& ماخذ = si.دوسرا;
        اگر (!ماخذ.needed()) جاری;
        ثابت string filename = ماخذ.name();
        ثابت string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        اگر (!is) {
            v3error("Can't read annotation file: " << filename);
            واپس;
        }

        std::ofstream os{outfilename.c_str()};
        اگر (!os) {
            v3error("Can't write file: " << outfilename);
            واپس;
        }

        os << "//      // verilator_coverage annotation\n";

        صحیح lineno = 0;
        جب (!is.eof()) {
            lineno++;
            ثابت std::string سطر = V3Os::getline(is);

            Vlc_ماخذ::LinenoMap& سطریں = ماخذ.سطریں();
            ثابت خودکار lit = سطریں.ڈھونڈو(lineno);
            اگر (lit == سطریں.انتہا()) {
                os << "        " << سطر << '\n';
            } ورنہ {
                Vlc_ماخذ_گنتی& sc = lit->دوسرا;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ثابت منطقی minOk = opt.گنتی_Ok(sc.min_گنتی());
                ثابت منطقی maxOk = opt.گنتی_Ok(sc.max_گنتی());
                اگر (minOk) {
                    os << " ";
                } ورنہ اگر (maxOk) {
                    os << "~";
                } ورنہ {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_گنتی() << " " << سطر << '\n';

                اگر (opt.annotatePoints()) {
                    برائے (ثابت خودکار& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                منطقی printed_Fsm_سرخی = جھوٹ;
                برائے (ثابت خودکار& pit : sc.points()) {
                    اگر (!pit->isFsmState() && !pit->isFsmArc()) جاری;
                    اگر (!printed_Fsm_سرخی) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_سرخی = سچ;
                    }
                    os << (opt.گنتی_Ok(pit->گنتی()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->گنتی() << "        ";
                    اگر (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        اگر (pit->گنتی() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } ورنہ اگر (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } ورنہ {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        اگر (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

خالی Vlc_چوٹی::annotate(ثابت string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

خالی Vlc_چوٹی::printTypeSummary() {
    TypeTally tally;
    برائے (VlcPoints::ByName::قدر_type& i : m_points) {
        ثابت Vlc_نقطہ& pt = m_points.نقطہ_نمبر(i.دوسرا);
        اگر (isCovergroup(pt)) جاری;  // Tallied below, by covergroup
        tally_نقطہ(tally, displayType(pt), pt.گنتی());
    }
    ثابت CgGroups groups = covergroups(m_points);
    اگر (!groups.خالی_خانے()) {
        std::سمتیہ<ثابت Cg_گروہ*> groupps;
        برائے (ثابت CgGroups::قدر_type& it : groups) groupps.ڈالو_عقب(&it.دوسرا);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    اگر (tally.خالی_خانے()) واپس;
    std::خارج << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, سچ);
}

خالی Vlc_چوٹی::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    منطقی hasHier = جھوٹ;
    منطقی hasCollapsedHier = جھوٹ;
    برائے (VlcPoints::ByName::قدر_type& i : m_points) {
        ثابت Vlc_نقطہ& pt = m_points.نقطہ_نمبر(i.دوسرا);
        اگر (isCovergroup(pt)) جاری;  // Reported below, by covergroup
        ثابت string hier = reportHier(pt);
        اگر (hier.خالی_خانے()) جاری;
        hasHier = سچ;
        اگر (isCollapsedHier(hier)) hasCollapsedHier = سچ;
        ثابت string type = displayType(pt);
        ثابت std::سمتیہ<string> parts = بانٹو_Hier(hier);
        string راستہ;
        برائے (std::سمتیہ<string>::const_iterator it = parts.begin(); it != parts.انتہا(); ++it) {
            راستہ = راستہ.خالی_خانے() ? *it : راستہ + "." + *it;
            tally_نقطہ(hierTallies[راستہ], type, pt.گنتی());
        }
        tally_نقطہ(duTallies[duName(pt)], type, pt.گنتی());
    }
    ثابت std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    اگر (!hasHier && !cgTallies.خالی_خانے()) {
        printCovergroupTallies(cgTallies, opt.report_سطحیں());
        واپس;
    }
    اگر (!hasHier) {
        std::خارج << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        واپس;
    }

    ثابت صحیح سطحیں = opt.report_سطحیں();
    اگر (hasCollapsedHier) {
        std::خارج << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::خارج << "Hierarchy Coverage Summary:\n";
    برائے (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.انتہا(); ++it) {
        ثابت std::سمتیہ<string> parts = بانٹو_Hier(it->پہلا);
        اگر (سطحیں >= 0 && static_cast<صحیح>(parts.سائز()) > سطحیں + 1) جاری;
        printIndent(s_summaryIndent);
        std::خارج << it->پہلا << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->دوسرا, s_report_صف_Indent, جھوٹ);
    }
    std::خارج << "Design Unit Coverage Summary:\n";
    برائے (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.انتہا();
         ++it) {
        printIndent(s_summaryIndent);
        std::خارج << it->پہلا << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->دوسرا, s_report_صف_Indent, جھوٹ);
    }
    اگر (!cgTallies.خالی_خانے()) printCovergroupTallies(cgTallies, سطحیں);
}
