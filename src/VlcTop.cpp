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

#وصف_ڪريو VL_MT_DISABLED_ڪوڊ_UNIT 1

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
    uint64_t m_ڪاميابي = 0;  // Covered points
    uint64_t m_ڪل = 0;  // Points
    منطقي m_scored = خطأ;  // Coverage is m_score, not the ratio of the points
    مزدوج m_score = 0.0;  // Percent coverage, if m_scored

عام:
    // ACCESSORS
    uint64_t ڪاميابي() ثابت { أرجع m_ڪاميابي; }
    uint64_t ڪل() ثابت { أرجع m_ڪل; }
    منطقي scored() ثابت { أرجع m_scored; }
    مزدوج score() ثابت { أرجع m_score; }
    فراغ score(مزدوج قدر) {
        m_scored = سچ;
        m_score = قدر;
    }
    // METHODS
    فراغ جوڙيو_نقطو(منطقي covered) {
        إذا (covered) ++m_ڪاميابي;
        ++m_ڪل;
    }
    فراغ جوڙيو_Points(ثابت Tally& other) {
        m_ڪاميابي += other.m_ڪاميابي;
        m_ڪل += other.m_ڪل;
    }
};

// Map coverage type to its tally.
استعمال TypeTally = std::map<std::string, Tally>;

ساكن constexpr ثابت حرف* ثابت s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ساكن constexpr ثابت حرف* ثابت s_covergroupType = "covergroup";
ساكن constexpr سائيز_t s_summaryIndent = 2;
ساكن constexpr سائيز_t s_report_صف_Indent = 4;

string displayType(ثابت Vlc_نقطو& نقطو) {
    ثابت string type = نقطو.type();
    أرجع type.خالي_جايون() ? "point" : type;
}

منطقي isCollapsedHier(ثابت string& hier) {
    أرجع hier.ڳوليو('*') != string::npos || hier.ڳوليو('?') != string::npos;
}

منطقي isOrderedType(ثابت string& type) {
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        إذا (type == typep) أرجع سچ;
    }
    أرجع خطأ;
}

string reportHier(ثابت Vlc_نقطو& نقطو) {
    // FSM records currently have useful instance scope in Fv.
    إذا (نقطو.isFsmState() || نقطو.isFsmArc()) {
        ثابت string fsmVar = نقطو.fsmVarName();
        أرجع fsmVar.substr(0, fsmVar.rfind('.'));
    }
    أرجع نقطو.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::ويڪٽر<string> ورهايو_Hier(ثابت string& hier) {
    std::ويڪٽر<string> parts;
    string::سائيز_type شروعات = 0;
    صحيح اونهائي = 0;  // Of the parentheses of the values of parameters
    string::سائيز_type pos = 0;
    طالما (pos < hier.سائيز()) {
        ثابت حرف c = hier[pos];
        إذا (c == '\\') {
            pos = hier.ڳوليو(' ', pos);  // npos if not ended
            استمر;
        } وإلا إذا (!اونهائي && hier.compare(pos, 2, "#(") == 0) {
            اونهائي = 1;
            ++pos;
        } وإلا إذا (!اونهائي && c == '.') {
            parts.ڌڪو_پٺ(hier.substr(شروعات, pos - شروعات));
            شروعات = pos + 1;
        } وإلا إذا (اونهائي && c == '"') {
            pos = VString::quoted_پڇاڙي(hier, pos);  // npos if not terminated
            استمر;
        } وإلا إذا (اونهائي && c == '(') {
            ++اونهائي;
        } وإلا إذا (اونهائي && c == ')') {
            --اونهائي;
        }
        ++pos;
    }
    إذا (اونهائي || pos != hier.سائيز()) {  // Unbalanced
        parts.clear();
        شروعات = 0;
        لكل (string::سائيز_type dot; (dot = hier.ڳوليو('.', شروعات)) != string::npos;
             شروعات = dot + 1) {
            parts.ڌڪو_پٺ(hier.substr(شروعات, dot - شروعات));
        }
    }
    parts.ڌڪو_پٺ(hier.substr(شروعات));
    أرجع parts;
}

string duName(ثابت Vlc_نقطو& نقطو) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ثابت string صفحو = نقطو.صفحو();
    أرجع صفحو.substr(صفحو.ڳوليو('/') + 1);
}

فراغ tally_نقطو(TypeTally& tally, ثابت string& type, uint64_t ڳڻپ) {
    tally[type].جوڙيو_نقطو(ڳڻپ > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
مزدوج pct(uint64_t ڪاميابي, uint64_t ڪل) {
    أرجع ڪل ? (100.0 * static_cast<مزدوج>(ڪاميابي) / static_cast<مزدوج>(ڪل)) : 0.0;
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
string pctString(uint64_t ڪاميابي, uint64_t ڪل) {
    أرجع pct_قدر_String(pct(ڪاميابي, ڪل), ڪاميابي >= ڪل);
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
    uint64_t m_ڳڻپ = 0;  // Hits, of all the records
    uint64_t m_at_گهٽ_کان_گهٽ = 0;  // option.at_least, the largest of the records

عام:
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    منطقي covered() ثابت { أرجع m_ڳڻپ >= m_at_گهٽ_کان_گهٽ; }
    // METHODS
    فراغ جوڙيو_رڪارڊ(ثابت string& name, uint64_t ڳڻپ, uint64_t at_گهٽ_کان_گهٽ) {
        m_name = name;
        m_ڳڻپ += ڳڻپ;
        m_at_گهٽ_کان_گهٽ = std::max(m_at_گهٽ_کان_گهٽ, at_گهٽ_کان_گهٽ);
    }
};

// A coverpoint or cross
صنف Cg_شي final {
    // MEMBERS
    uint64_t m_وزن = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

عام:
    // ACCESSORS
    uint64_t وزن_2() ثابت { أرجع m_وزن; }
    ثابت std::map<string, CgBin>& bins() ثابت { أرجع m_bins; }
    // METHODS
    فراغ جوڙيو_رڪارڊ(uint64_t وزن_2) { m_وزن = std::max(m_وزن, وزن_2); }
    CgBin& ڳوليو_New_Bin(ثابت string& سڃاڻپ_عنصر) { أرجع m_bins[سڃاڻپ_عنصر]; }
};

// A covergroup
صنف Cg_گروهه final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_وزن = 0;  // type_option.weight, the largest of the records
    std::map<string, Cg_شي> m_شيون;  // By hierarchy

عام:
    // ACCESSORS
    ثابت string& hier() ثابت { أرجع m_hier; }
    uint64_t وزن_2() ثابت { أرجع m_وزن; }
    ثابت std::map<string, Cg_شي>& شيون() ثابت { أرجع m_شيون; }
    // METHODS
    فراغ جوڙيو_رڪارڊ(ثابت string& hier, uint64_t وزن_2) {
        m_hier = hier;
        m_وزن = std::max(m_وزن, وزن_2);
    }
    Cg_شي& ڳوليو_New_شي(ثابت string& hier) { أرجع m_شيون[hier]; }
};
استعمال CgGroups = std::map<string, Cg_گروهه>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t ڪنجي_عدد(ثابت string& قدر) {
    أرجع قدر.خالي_جايون() ? 1 : std::strtoull(قدر.c_str(), خالي_اشارو, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
منطقي is_Score_ميدان(ثابت string& ميدان) {
    لكل (ثابت حرف* ثابت keyp : {VL_CIK_THRESH, VL_CIK_وزن, VL_CIK_گروهه_وزن}) {
        ثابت string اڳياڙي = string{"\001"} + keyp + "\002";
        إذا (ميدان.compare(0, اڳياڙي.سائيز(), اڳياڙي) == 0) أرجع سچ;
    }
    أرجع خطأ;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_سڃاڻپ_عنصر(ثابت string& رڪارڊ_Name) {
    string سڃاڻپ_عنصر;
    string::سائيز_type شروعات = 0;
    طالما (شروعات < رڪارڊ_Name.سائيز()) {
        string::سائيز_type پڇاڙي = رڪارڊ_Name.ڳوليو('\001', شروعات + 1);
        إذا (پڇاڙي == string::npos) پڇاڙي = رڪارڊ_Name.سائيز();
        ثابت string ميدان = رڪارڊ_Name.substr(شروعات, پڇاڙي - شروعات);
        إذا (!is_Score_ميدان(ميدان)) سڃاڻپ_عنصر += ميدان;
        شروعات = پڇاڙي;
    }
    أرجع سڃاڻپ_عنصر;
}

منطقي isCovergroup(ثابت Vlc_نقطو& نقطو) { أرجع نقطو.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    لكل (ثابت VlcPoints::ByName::قدر_type& i : points) {
        ثابت Vlc_نقطو& pt = points.نقطو_عدد(i.ٻيو);
        إذا (!isCovergroup(pt)) استمر;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ثابت string hier = pt.hier();
        string bin = pt.bin();
        إذا (bin.خالي_جايون() || bin.سائيز() >= hier.سائيز()) bin = hier.substr(hier.rfind('.') + 1);
        ثابت string شي_Hier
            = hier.substr(0, hier.سائيز() - std::min(hier.سائيز(), bin.سائيز() + 1));
        Cg_گروهه& گروهه = groups[pt.صفحو()];
        گروهه.جوڙيو_رڪارڊ(شي_Hier.substr(0, شي_Hier.rfind('.')), ڪنجي_عدد(pt.گروهه_وزن()));
        Cg_شي& شي_2 = گروهه.ڳوليو_New_شي(شي_Hier);
        شي_2.جوڙيو_رڪارڊ(ڪنجي_عدد(pt.وزن_2()));
        إذا (!pt.binType().خالي_جايون()) استمر;  // Not coverable
        شي_2.ڳوليو_New_Bin(bin_سڃاڻپ_عنصر(pt.name())).جوڙيو_رڪارڊ(bin, pt.ڳڻپ(), ڪنجي_عدد(pt.thresh()));
    }
    أرجع groups;
}

Tally شي_Tally(ثابت Cg_شي& شي_2) {
    Tally tally;
    لكل (ثابت std::pair<ثابت string, CgBin>& bin : شي_2.bins()) {
        tally.جوڙيو_نقطو(bin.ٻيو.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.ڪل() ? pct(tally.ڪاميابي(), tally.ڪل()) : شي_2.وزن_2() ? 0.0 : 100.0);
    أرجع tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally گروهه_Tally(ثابت Cg_گروهه& گروهه, منطقي* contributesp = خالي_اشارو) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    لكل (ثابت std::pair<ثابت string, Cg_شي>& it : گروهه.شيون()) {
        ثابت Tally شي_2 = شي_Tally(it.ٻيو);
        tally.جوڙيو_Points(شي_2);
        إذا (!شي_2.ڪل()) استمر;  // Excluded from both sums
        weighted += static_cast<مزدوج>(it.ٻيو.وزن_2()) * شي_2.score();
        weights += static_cast<مزدوج>(it.ٻيو.وزن_2());
    }
    إذا (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : گروهه.وزن_2() ? 0.0 : 100.0);
    أرجع tally;
}

Tally groupsTally(ثابت std::ويڪٽر<ثابت Cg_گروهه*>& groups) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    منطقي any_وزن = خطأ;
    لكل (ثابت Cg_گروهه* ثابت groupp : groups) {
        منطقي contributes = خطأ;
        ثابت Tally گروهه = گروهه_Tally(*groupp, &contributes);
        tally.جوڙيو_Points(گروهه);
        إذا (groupp->وزن_2()) any_وزن = سچ;
        إذا (!contributes) استمر;
        weighted += static_cast<مزدوج>(groupp->وزن_2()) * گروهه.score();
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
    std::map<string, std::ويڪٽر<ثابت Cg_گروهه*>> ڳنڍ_Groups;
    لكل (ثابت CgGroups::قدر_type& it : groups) {
        ثابت Cg_گروهه& گروهه = it.ٻيو;
        string رستو;
        لكل (ثابت string& part : ورهايو_Hier(گروهه.hier())) {
            رستو = رستو.خالي_جايون() ? part : رستو + "." + part;
            ڳنڍ_Groups[رستو].ڌڪو_پٺ(&گروهه);
        }
        لكل (ثابت std::pair<ثابت string, Cg_شي>& شي_2 : گروهه.شيون()) {
            tallies[شي_2.پهريون] = شي_Tally(شي_2.ٻيو);
            لكل (ثابت std::pair<ثابت string, CgBin>& bin : شي_2.ٻيو.bins()) {
                // Of the bins of the name
                tallies[شي_2.پهريون + "." + bin.ٻيو.name()].جوڙيو_نقطو(bin.ٻيو.covered());
            }
        }
    }
    لكل (ثابت std::pair<ثابت string, std::ويڪٽر<ثابت Cg_گروهه*>>& it : ڳنڍ_Groups) {
        tallies[it.پهريون]
            = it.ٻيو.سائيز() == 1 ? گروهه_Tally(*it.ٻيو.front()) : groupsTally(it.ٻيو);
    }
    أرجع tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
فراغ printIndent(سائيز_t indent) {
    لكل (سائيز_t i = 0; i < indent; ++i) std::ٻاهر << ' ';
}

فراغ print_Tally_صف(ثابت string& type, ثابت Tally& tally, سائيز_t indent, سائيز_t type_ويڪر,
                   سائيز_t ڳڻپ_ويڪر) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ثابت string percent = tally.scored() ? pct_قدر_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.ڪاميابي(), tally.ڪل());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::ٻاهر << std::کاٻي << std::setw(type_ويڪر) << type << " : " << std::ساڄي << std::fixed
              << std::setw(6) << percent << " (" << std::setw(ڳڻپ_ويڪر) << tally.ڪاميابي() << "/"
              << std::setw(ڳڻپ_ويڪر) << tally.ڪل() << ")\n";
}

سائيز_t ڳڻپ_ويڪر(ثابت TypeTally& tally) {
    سائيز_t ويڪر = cvt_منزل_Str(0).سائيز();
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.پڇاڙي(); ++it) {
        ويڪر = std::max(ويڪر, cvt_منزل_Str(it->ٻيو.ڪاميابي()).سائيز());
        ويڪر = std::max(ويڪر, cvt_منزل_Str(it->ٻيو.ڪل()).سائيز());
    }
    أرجع ويڪر;
}

سائيز_t type_ويڪر(ثابت TypeTally& tally) {
    سائيز_t type_ويڪر = 0;
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        type_ويڪر = std::max(type_ويڪر, type.سائيز());
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.پڇاڙي(); ++it) {
        type_ويڪر = std::max(type_ويڪر, it->پهريون.سائيز());
    }
    أرجع type_ويڪر;
}

فراغ printTypeTally(ثابت TypeTally& tally, سائيز_t indent, منطقي includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ثابت سائيز_t typ_ويڪر = type_ويڪر(tally);
    ثابت سائيز_t cnt_ويڪر = ڳڻپ_ويڪر(tally);
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        ثابت TypeTally::const_iterator it = tally.ڳوليو(type);
        إذا (it != tally.پڇاڙي()) {
            print_Tally_صف(type, it->ٻيو, indent, typ_ويڪر, cnt_ويڪر);
        } وإلا إذا (includeMissingOrdered) {
            print_Tally_صف(type, Tally{}, indent, typ_ويڪر, cnt_ويڪر);
        }
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.پڇاڙي(); ++it) {
        إذا (!isOrderedType(it->پهريون)) {
            print_Tally_صف(it->پهريون, it->ٻيو, indent, typ_ويڪر, cnt_ويڪر);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
فراغ printCovergroupTallies(ثابت std::map<string, Tally>& tallies, صحيح سطحون) {
    std::map<string, Tally> shown;
    سائيز_t name_ويڪر = 0;
    لكل (ثابت std::pair<ثابت string, Tally>& it : tallies) {
        إذا (سطحون >= 0 && static_cast<صحيح>(ورهايو_Hier(it.پهريون).سائيز()) > سطحون + 1) استمر;
        shown.وجهو(it);
        name_ويڪر = std::max(name_ويڪر, it.پهريون.سائيز());
    }
    ثابت سائيز_t cnt_ويڪر = ڳڻپ_ويڪر(shown);
    std::ٻاهر << "Covergroup Coverage Summary:\n";
    لكل (ثابت std::pair<ثابت string, Tally>& it : shown) {
        print_Tally_صف(it.پهريون, it.ٻيو, s_summaryIndent, name_ويڪر, cnt_ويڪر);
    }
}

}  // namespace

فراغ Vlc_چوٽي::readCoverage(ثابت string& filename, منطقي nonfatal) {
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
        ثابت string سٽ = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        إذا (سٽ[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ثابت string::سائيز_type secspace = سٽ.rfind("' ");
            إذا (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                استمر;
            }
            ثابت string نقطو = سٽ.substr(3, secspace - 3);
            إذا (!opt.is_Type_ميلاپ(نقطو.c_str())) استمر;

            ثابت uint64_t ڪاميابيون = std::atoll(سٽ.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ثابت uint64_t pointnum = points().ڳوليو_جوڙيو_نقطو(نقطو, ڪاميابيون);
            إذا (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ثابت uint64_t needed = std::max(
                    VlcBuckets::sufficient(), ڪنجي_عدد(points().نقطو_عدد(pointnum).thresh()));
                إذا (ڪاميابيون >= needed) {
                    points().نقطو_عدد(pointnum).testsCoveringInc();
                    testp->buckets().جوڙيو_معلومات(pointnum, ڪاميابيون);
                }
            }
        }
    }
}

فراغ Vlc_چوٽي::writeCoverage(ثابت string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    إذا (!os) {
        v3fatal("Can't write file: " << filename);
        أرجع;
    }

    os << "# SystemC::Coverage-3\n";
    لكل (ثابت خودڪار& i : m_points) {
        ثابت Vlc_نقطو& نقطو = m_points.نقطو_عدد(i.ٻيو);
        os << "C '" << نقطو.name() << "' " << نقطو.ڳڻپ() << '\n';
    }
}

فراغ Vlc_چوٽي::writeInfo(ثابت string& filename) {
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
    لكل (خودڪار& si : m_sources) {
        Vlc_ذريعو& ذريعو = si.ٻيو;
        os << "SF:" << ذريعو.name() << '\n';
        Vlc_ذريعو::LinenoMap& سٽون = ذريعو.سٽون();
        صحيح branches_مليو = 0;
        صحيح branches_ڪاميابي = 0;
        لكل (خودڪار& li : سٽون) {
            Vlc_ذريعو_ڳڻپ& sc = li.ٻيو;
            uint64_t da_ڳڻپ = 0;
            std::ويڪٽر<ثابت Vlc_نقطو*> infoPoints;
            لكل (ثابت خودڪار& نقطو : sc.points()) {
                da_ڳڻپ = std::max(da_ڳڻپ, نقطو->ڳڻپ());
                infoPoints.ڌڪو_پٺ(نقطو);
            }
            os << "DA:" << sc.lineno() << "," << da_ڳڻپ << "\n";
            إذا (infoPoints.سائيز() <= 1) استمر;
            branches_مليو += static_cast<صحيح>(infoPoints.سائيز());
            صحيح نقطو_انگ_2 = 0;
            لكل (ثابت Vlc_نقطو* نقطو : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                إذا (نقطو->isFsmArc()) {
                    os << "2,";
                    os << نقطو->fsmFromState() << "->" << نقطو->fsm_منزل_State();
                } وإلا إذا (نقطو->comment().خالي_جايون()) {
                    os << "0,";
                    os << نقطو_انگ_2;
                } وإلا {
                    os << (نقطو->isFsmState() ? '1' : '0') << ',';
                    std::string comment(نقطو->comment());
                    std::replace(comment.begin(), comment.پڇاڙي(), ',', '_');
                    os << comment;
                }
                os << "," << نقطو->ڳڻپ() << "\n";

                branches_ڪاميابي += opt.ڳڻپ_Ok(نقطو->ڳڻپ());
                ++نقطو_انگ_2;
            }
        }
        os << "BRF:" << branches_مليو << '\n';
        os << "BRH:" << branches_ڪاميابي << '\n';

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

فراغ Vlc_چوٽي::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::ويڪٽر<VlcTest*> bytime;
    لكل (ثابت خودڪار& testp : m_tests) {
        إذا (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ڌڪو_پٺ(testp);
        }
    }
    ترتيب_ڏيو(bytime.begin(), bytime.پڇاڙي(), CmpComputrons());  // Sort the vector

    VlcBuckets بچيل;
    لكل (ثابت خودڪار& i : m_points) {
        ثابت Vlc_نقطو* ثابت pointp = &points().نقطو_عدد(i.ٻيو);
        // If any tests hit this point, then we'll need to cover it.
        إذا (pointp->testsCovering()) بچيل.جوڙيو_معلومات(pointp->نقطو_انگ(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    طالما (سچ) {
        إذا (debug() >= 9) {
            UINFO_اڳياڙي("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            بچيل.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* بهترين_Testp = خالي_اشارو;
        uint64_t بهترين_Remain = 0;
        لكل (ثابت خودڪار& testp : bytime) {
            إذا (!testp->rank()) {
                ثابت uint64_t remain = testp->buckets().معلومات_ڪڍو_ڳڻپ(بچيل);
                إذا (remain > بهترين_Remain) {
                    بهترين_Testp = testp;
                    بهترين_Remain = remain;
                }
            }
        }
        إذا (VlcTest* ثابت testp = بهترين_Testp) {
            testp->rank(nextrank++);
            testp->rankPoints(بهترين_Remain);
            بچيل.or_معلومات(بهترين_Testp->buckets());
        } وإلا {
            اكسر;  // No test covering more stuff found
        }
    }
}

فراغ Vlc_چوٽي::annotateCalc() {
    // Calculate per-line information into filedata structure
    لكل (ثابت خودڪار& i : m_points) {
        ثابت Vlc_نقطو& نقطو = m_points.نقطو_عدد(i.ٻيو);
        ثابت string filename = نقطو.filename();
        ثابت صحيح lineno = نقطو.lineno();
        إذا (!filename.خالي_جايون() && lineno != 0) {
            Vlc_ذريعو& ذريعو = sources().ڳوليو_New_ذريعو(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << نقطو.ڪالم_2() << " "
                                       << نقطو.ڳڻپ() << " " << نقطو.linescov());
            // Base coverage
            ذريعو.وجهو_نقطو(lineno, &نقطو);
            // Additional lines covered by this statement
            منطقي range = خطأ;
            صحيح شروعات = 0;
            صحيح پڇاڙي = 0;
            ثابت string linescov = نقطو.linescov();
            لكل (ثابت حرف* covp = linescov.c_str(); سچ; ++covp) {
                إذا (!*covp || *covp == ',') {  // Ending
                    لكل (صحيح lni = شروعات; شروعات && lni <= پڇاڙي; ++lni) {
                        ذريعو.وجهو_نقطو(lni, &نقطو);
                    }
                    إذا (!*covp) اكسر;
                    شروعات = 0;  // Prep for next
                    پڇاڙي = 0;
                    range = خطأ;
                } وإلا إذا (*covp == '-') {
                    range = سچ;
                } وإلا إذا (std::انگ_آهي(*covp)) {
                    ثابت حرف* ثابت digitsp = covp;
                    طالما (std::انگ_آهي(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    إذا (!range) شروعات = std::atoi(digitsp);
                    پڇاڙي = std::atoi(digitsp);
                }
            }
        }
    }
}

فراغ Vlc_چوٽي::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    صحيح totCases = 0;
    صحيح totOk = 0;
    لكل (خودڪار& si : m_sources) {
        Vlc_ذريعو& ذريعو = si.ٻيو;
        // UINFO(1, "Source " << source.name());
        إذا (opt.annotateAll()) ذريعو.needed(سچ);
        ثابت Vlc_ذريعو::LinenoMap& سٽون = ذريعو.سٽون();
        لكل (خودڪار& li : سٽون) {
            ثابت Vlc_ذريعو_ڳڻپ& sc = li.ٻيو;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            إذا (opt.ڳڻپ_Ok(sc.min_ڳڻپ())) {
                ++totOk;
            } وإلا {
                ذريعو.needed(سچ);
            }
        }
    }
    std::ٻاهر << "Annotation Summary:\n";
    std::ٻاهر << "  lines with all attached points covered : ";
    std::ٻاهر << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    إذا (totOk != totCases) ٻاهر << "See lines with '%00' in " << opt.annotate_ٻاهريون() << '\n';
}

فراغ Vlc_چوٽي::annotateOutputFiles(ثابت string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    لكل (خودڪار& si : m_sources) {
        Vlc_ذريعو& ذريعو = si.ٻيو;
        إذا (!ذريعو.needed()) استمر;
        ثابت string filename = ذريعو.name();
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
            ثابت std::string سٽ = V3Os::getline(is);

            Vlc_ذريعو::LinenoMap& سٽون = ذريعو.سٽون();
            ثابت خودڪار lit = سٽون.ڳوليو(lineno);
            إذا (lit == سٽون.پڇاڙي()) {
                os << "        " << سٽ << '\n';
            } وإلا {
                Vlc_ذريعو_ڳڻپ& sc = lit->ٻيو;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ثابت منطقي minOk = opt.ڳڻپ_Ok(sc.min_ڳڻپ());
                ثابت منطقي maxOk = opt.ڳڻپ_Ok(sc.max_ڳڻپ());
                إذا (minOk) {
                    os << " ";
                } وإلا إذا (maxOk) {
                    os << "~";
                } وإلا {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_ڳڻپ() << " " << سٽ << '\n';

                إذا (opt.annotatePoints()) {
                    لكل (ثابت خودڪار& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                منطقي printed_Fsm_سرو = خطأ;
                لكل (ثابت خودڪار& pit : sc.points()) {
                    إذا (!pit->isFsmState() && !pit->isFsmArc()) استمر;
                    إذا (!printed_Fsm_سرو) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_سرو = سچ;
                    }
                    os << (opt.ڳڻپ_Ok(pit->ڳڻپ()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->ڳڻپ() << "        ";
                    إذا (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        إذا (pit->ڳڻپ() == 0) os << " *** UNCOVERED ***";
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

فراغ Vlc_چوٽي::annotate(ثابت string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

فراغ Vlc_چوٽي::printTypeSummary() {
    TypeTally tally;
    لكل (VlcPoints::ByName::قدر_type& i : m_points) {
        ثابت Vlc_نقطو& pt = m_points.نقطو_عدد(i.ٻيو);
        إذا (isCovergroup(pt)) استمر;  // Tallied below, by covergroup
        tally_نقطو(tally, displayType(pt), pt.ڳڻپ());
    }
    ثابت CgGroups groups = covergroups(m_points);
    إذا (!groups.خالي_جايون()) {
        std::ويڪٽر<ثابت Cg_گروهه*> groupps;
        لكل (ثابت CgGroups::قدر_type& it : groups) groupps.ڌڪو_پٺ(&it.ٻيو);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    إذا (tally.خالي_جايون()) أرجع;
    std::ٻاهر << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, سچ);
}

فراغ Vlc_چوٽي::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    منطقي hasHier = خطأ;
    منطقي hasCollapsedHier = خطأ;
    لكل (VlcPoints::ByName::قدر_type& i : m_points) {
        ثابت Vlc_نقطو& pt = m_points.نقطو_عدد(i.ٻيو);
        إذا (isCovergroup(pt)) استمر;  // Reported below, by covergroup
        ثابت string hier = reportHier(pt);
        إذا (hier.خالي_جايون()) استمر;
        hasHier = سچ;
        إذا (isCollapsedHier(hier)) hasCollapsedHier = سچ;
        ثابت string type = displayType(pt);
        ثابت std::ويڪٽر<string> parts = ورهايو_Hier(hier);
        string رستو;
        لكل (std::ويڪٽر<string>::const_iterator it = parts.begin(); it != parts.پڇاڙي(); ++it) {
            رستو = رستو.خالي_جايون() ? *it : رستو + "." + *it;
            tally_نقطو(hierTallies[رستو], type, pt.ڳڻپ());
        }
        tally_نقطو(duTallies[duName(pt)], type, pt.ڳڻپ());
    }
    ثابت std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    إذا (!hasHier && !cgTallies.خالي_جايون()) {
        printCovergroupTallies(cgTallies, opt.report_سطحون());
        أرجع;
    }
    إذا (!hasHier) {
        std::ٻاهر << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        أرجع;
    }

    ثابت صحيح سطحون = opt.report_سطحون();
    إذا (hasCollapsedHier) {
        std::ٻاهر << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::ٻاهر << "Hierarchy Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.پڇاڙي(); ++it) {
        ثابت std::ويڪٽر<string> parts = ورهايو_Hier(it->پهريون);
        إذا (سطحون >= 0 && static_cast<صحيح>(parts.سائيز()) > سطحون + 1) استمر;
        printIndent(s_summaryIndent);
        std::ٻاهر << it->پهريون << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->ٻيو, s_report_صف_Indent, خطأ);
    }
    std::ٻاهر << "Design Unit Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.پڇاڙي();
         ++it) {
        printIndent(s_summaryIndent);
        std::ٻاهر << it->پهريون << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->ٻيو, s_report_صف_Indent, خطأ);
    }
    إذا (!cgTallies.خالي_جايون()) printCovergroupTallies(cgTallies, سطحون);
}
