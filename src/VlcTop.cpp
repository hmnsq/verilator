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

#define VL_MT_DISABLED_CODE_UNIT 1

#অন্তর্ভাব "VlcTop.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Os.h"

#অন্তর্ভাব "VlcOptions.h"

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <fstream>
#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <sstream>
#অন্তর্ভাব <string>
#অন্তর্ভাব <vector>

//######################################################################

নামাকাশ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
শ্রেণী Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_যোগ = 0;  // Points
    যৌক্তিক m_scored = মিথ্যা;  // Coverage is m_score, not the ratio of the points
    দ্বিগুণ m_score = 0.0;  // Percent coverage, if m_scored

সর্বজনীন:
    // ACCESSORS
    uint64_t hit() ধ্রুবক { ফেরত m_hit; }
    uint64_t যোগ() ধ্রুবক { ফেরত m_যোগ; }
    যৌক্তিক scored() ধ্রুবক { ফেরত m_scored; }
    দ্বিগুণ score() ধ্রুবক { ফেরত m_score; }
    শূন্য score(দ্বিগুণ মান) {
        m_scored = সত্য;
        m_score = মান;
    }
    // METHODS
    শূন্য addPoint(যৌক্তিক covered) {
        যদি (covered) ++m_hit;
        ++m_যোগ;
    }
    শূন্য addPoints(ধ্রুবক Tally& other) {
        m_hit += other.m_hit;
        m_যোগ += other.m_যোগ;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

স্থির constexpr ধ্রুবক অক্ষর* ধ্রুবক s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
স্থির constexpr ধ্রুবক অক্ষর* ধ্রুবক s_covergroupType = "covergroup";
স্থির constexpr আকার_t s_summaryIndent = 2;
স্থির constexpr আকার_t s_reportRowIndent = 4;

string displayType(ধ্রুবক VlcPoint& point) {
    ধ্রুবক string type = point.type();
    ফেরত type.empty() ? "point" : type;
}

যৌক্তিক isCollapsedHier(ধ্রুবক string& hier) {
    ফেরত hier.find('*') != string::npos || hier.find('?') != string::npos;
}

যৌক্তিক isOrderedType(ধ্রুবক string& type) {
    জন্য (ধ্রুবক অক্ষর* ধ্রুবক typep : s_orderedTypes) {
        যদি (type == typep) ফেরত সত্য;
    }
    ফেরত মিথ্যা;
}

string reportHier(ধ্রুবক VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    যদি (point.isFsmState() || point.isFsmArc()) {
        ধ্রুবক string fsmVar = point.fsmVarName();
        ফেরত fsmVar.substr(0, fsmVar.rfind('.'));
    }
    ফেরত point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(ধ্রুবক string& hier) {
    std::vector<string> parts;
    string::আকার_type start = 0;
    পূর্ণসংখ্যা depth = 0;  // Of the parentheses of the values of parameters
    string::আকার_type pos = 0;
    যখন (pos < hier.আকার_2()) {
        ধ্রুবক অক্ষর c = hier[pos];
        যদি (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            চালু;
        } অন্যথা যদি (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } অন্যথা যদি (!depth && c == '.') {
            parts.ঠেলো_back(hier.substr(start, pos - start));
            start = pos + 1;
        } অন্যথা যদি (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            চালু;
        } অন্যথা যদি (depth && c == '(') {
            ++depth;
        } অন্যথা যদি (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    যদি (depth || pos != hier.আকার_2()) {  // Unbalanced
        parts.clear();
        start = 0;
        জন্য (string::আকার_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.ঠেলো_back(hier.substr(start, dot - start));
        }
    }
    parts.ঠেলো_back(hier.substr(start));
    ফেরত parts;
}

string duName(ধ্রুবক VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ধ্রুবক string page = point.page();
    ফেরত page.substr(page.find('/') + 1);
}

শূন্য tallyPoint(TypeTally& tally, ধ্রুবক string& type, uint64_t গুনতি) {
    tally[type].addPoint(গুনতি > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
দ্বিগুণ pct(uint64_t hit, uint64_t যোগ) {
    ফেরত যোগ ? (100.0 * static_cast<দ্বিগুণ>(hit) / static_cast<দ্বিগুণ>(যোগ)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_মান_String(দ্বিগুণ মান, যৌক্তিক complete) {
    যদি (!complete && মান > 99.9) মান = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << মান << "%";
    ফেরত os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t যোগ) {
    ফেরত pct_মান_String(pct(hit, যোগ), hit >= যোগ);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
শ্রেণী CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_গুনতি = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

সর্বজনীন:
    // ACCESSORS
    ধ্রুবক string& name() ধ্রুবক { ফেরত m_name; }
    যৌক্তিক covered() ধ্রুবক { ফেরত m_গুনতি >= m_atLeast; }
    // METHODS
    শূন্য addRecord(ধ্রুবক string& name, uint64_t গুনতি, uint64_t atLeast) {
        m_name = name;
        m_গুনতি += গুনতি;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
শ্রেণী CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

সর্বজনীন:
    // ACCESSORS
    uint64_t weight() ধ্রুবক { ফেরত m_weight; }
    ধ্রুবক std::map<string, CgBin>& bins() ধ্রুবক { ফেরত m_bins; }
    // METHODS
    শূন্য addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ধ্রুবক string& identity) { ফেরত m_bins[identity]; }
};

// A covergroup
শ্রেণী CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_বস্তুসমূহ;  // By hierarchy

সর্বজনীন:
    // ACCESSORS
    ধ্রুবক string& hier() ধ্রুবক { ফেরত m_hier; }
    uint64_t weight() ধ্রুবক { ফেরত m_weight; }
    ধ্রুবক std::map<string, CgItem>& বস্তুসমূহ() ধ্রুবক { ফেরত m_বস্তুসমূহ; }
    // METHODS
    শূন্য addRecord(ধ্রুবক string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ধ্রুবক string& hier) { ফেরত m_বস্তুসমূহ[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t চাবি_সংখ্যা(ধ্রুবক string& মান) {
    ফেরত মান.empty() ? 1 : std::strtoull(মান.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
যৌক্তিক isScoreField(ধ্রুবক string& field) {
    জন্য (ধ্রুবক অক্ষর* ধ্রুবক keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        ধ্রুবক string prefix = string{"\001"} + keyp + "\002";
        যদি (field.compare(0, prefix.আকার_2(), prefix) == 0) ফেরত সত্য;
    }
    ফেরত মিথ্যা;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(ধ্রুবক string& recordName) {
    string identity;
    string::আকার_type start = 0;
    যখন (start < recordName.আকার_2()) {
        string::আকার_type end = recordName.find('\001', start + 1);
        যদি (end == string::npos) end = recordName.আকার_2();
        ধ্রুবক string field = recordName.substr(start, end - start);
        যদি (!isScoreField(field)) identity += field;
        start = end;
    }
    ফেরত identity;
}

যৌক্তিক isCovergroup(ধ্রুবক VlcPoint& point) { ফেরত point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    জন্য (ধ্রুবক VlcPoints::ByName::মান_type& i : points) {
        ধ্রুবক VlcPoint& pt = points.point_সংখ্যা(i.second);
        যদি (!isCovergroup(pt)) চালু;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ধ্রুবক string hier = pt.hier();
        string bin = pt.bin();
        যদি (bin.empty() || bin.আকার_2() >= hier.আকার_2()) bin = hier.substr(hier.rfind('.') + 1);
        ধ্রুবক string itemHier
            = hier.substr(0, hier.আকার_2() - std::min(hier.আকার_2(), bin.আকার_2() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), চাবি_সংখ্যা(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(চাবি_সংখ্যা(pt.weight()));
        যদি (!pt.binType().empty()) চালু;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.গুনতি(), চাবি_সংখ্যা(pt.thresh()));
    }
    ফেরত groups;
}

Tally itemTally(ধ্রুবক CgItem& item) {
    Tally tally;
    জন্য (ধ্রুবক std::pair<ধ্রুবক string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.যোগ() ? pct(tally.hit(), tally.যোগ()) : item.weight() ? 0.0 : 100.0);
    ফেরত tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(ধ্রুবক CgGroup& group, যৌক্তিক* contributesp = nullptr) {
    Tally tally;
    দ্বিগুণ weighted = 0.0;
    দ্বিগুণ weights = 0.0;
    জন্য (ধ্রুবক std::pair<ধ্রুবক string, CgItem>& it : group.বস্তুসমূহ()) {
        ধ্রুবক Tally item = itemTally(it.second);
        tally.addPoints(item);
        যদি (!item.যোগ()) চালু;  // Excluded from both sums
        weighted += static_cast<দ্বিগুণ>(it.second.weight()) * item.score();
        weights += static_cast<দ্বিগুণ>(it.second.weight());
    }
    যদি (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    ফেরত tally;
}

Tally groupsTally(ধ্রুবক std::vector<ধ্রুবক CgGroup*>& groups) {
    Tally tally;
    দ্বিগুণ weighted = 0.0;
    দ্বিগুণ weights = 0.0;
    যৌক্তিক anyWeight = মিথ্যা;
    জন্য (ধ্রুবক CgGroup* ধ্রুবক groupp : groups) {
        যৌক্তিক contributes = মিথ্যা;
        ধ্রুবক Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        যদি (groupp->weight()) anyWeight = সত্য;
        যদি (!contributes) চালু;
        weighted += static_cast<দ্বিগুণ>(groupp->weight()) * group.score();
        weights += static_cast<দ্বিগুণ>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    ফেরত tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ধ্রুবক CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<ধ্রুবক CgGroup*>> গ্রন্থি_Groups;
    জন্য (ধ্রুবক CgGroups::মান_type& it : groups) {
        ধ্রুবক CgGroup& group = it.second;
        string path;
        জন্য (ধ্রুবক string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            গ্রন্থি_Groups[path].ঠেলো_back(&group);
        }
        জন্য (ধ্রুবক std::pair<ধ্রুবক string, CgItem>& item : group.বস্তুসমূহ()) {
            tallies[item.first] = itemTally(item.second);
            জন্য (ধ্রুবক std::pair<ধ্রুবক string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    জন্য (ধ্রুবক std::pair<ধ্রুবক string, std::vector<ধ্রুবক CgGroup*>>& it : গ্রন্থি_Groups) {
        tallies[it.first]
            = it.second.আকার_2() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    ফেরত tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
শূন্য printIndent(আকার_t indent) {
    জন্য (আকার_t i = 0; i < indent; ++i) std::cout << ' ';
}

শূন্য printTallyRow(ধ্রুবক string& type, ধ্রুবক Tally& tally, আকার_t indent, আকার_t typeWidth,
                   আকার_t গুনতি_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ধ্রুবক string percent = tally.scored() ? pct_মান_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.যোগ());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::বাম << std::setw(typeWidth) << type << " : " << std::ডান << std::fixed
              << std::setw(6) << percent << " (" << std::setw(গুনতি_Width) << tally.hit() << "/"
              << std::setw(গুনতি_Width) << tally.যোগ() << ")\n";
}

আকার_t গুনতি_Width(ধ্রুবক TypeTally& tally) {
    আকার_t width = cvtToStr(0).আকার_2();
    জন্য (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).আকার_2());
        width = std::max(width, cvtToStr(it->second.যোগ()).আকার_2());
    }
    ফেরত width;
}

আকার_t typeWidth(ধ্রুবক TypeTally& tally) {
    আকার_t typeWidth = 0;
    জন্য (ধ্রুবক অক্ষর* ধ্রুবক typep : s_orderedTypes) {
        ধ্রুবক string type = typep;
        typeWidth = std::max(typeWidth, type.আকার_2());
    }
    জন্য (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.আকার_2());
    }
    ফেরত typeWidth;
}

শূন্য printTypeTally(ধ্রুবক TypeTally& tally, আকার_t indent, যৌক্তিক includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ধ্রুবক আকার_t typWidth = typeWidth(tally);
    ধ্রুবক আকার_t cntWidth = গুনতি_Width(tally);
    জন্য (ধ্রুবক অক্ষর* ধ্রুবক typep : s_orderedTypes) {
        ধ্রুবক string type = typep;
        ধ্রুবক TypeTally::const_iterator it = tally.find(type);
        যদি (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } অন্যথা যদি (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    জন্য (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        যদি (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
শূন্য printCovergroupTallies(ধ্রুবক std::map<string, Tally>& tallies, পূর্ণসংখ্যা levels) {
    std::map<string, Tally> shown;
    আকার_t nameWidth = 0;
    জন্য (ধ্রুবক std::pair<ধ্রুবক string, Tally>& it : tallies) {
        যদি (levels >= 0 && static_cast<পূর্ণসংখ্যা>(splitHier(it.first).আকার_2()) > levels + 1) চালু;
        shown.ঢোকাও(it);
        nameWidth = std::max(nameWidth, it.first.আকার_2());
    }
    ধ্রুবক আকার_t cntWidth = গুনতি_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    জন্য (ধ্রুবক std::pair<ধ্রুবক string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

শূন্য Vlc_শীর্ষ::readCoverage(ধ্রুবক string& filename, যৌক্তিক nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    যদি (!is) {
        যদি (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        ফেরত;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ধ্রুবক testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    যখন (!is.eof()) {
        ধ্রুবক string সারি = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        যদি (সারি[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ধ্রুবক string::আকার_type secspace = সারি.rfind("' ");
            যদি (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                চালু;
            }
            ধ্রুবক string point = সারি.substr(3, secspace - 3);
            যদি (!opt.isTypeMatch(point.c_str())) চালু;

            ধ্রুবক uint64_t hits = std::atoll(সারি.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ধ্রুবক uint64_t pointnum = points().findAddPoint(point, hits);
            যদি (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ধ্রুবক uint64_t needed = std::max(
                    VlcBuckets::sufficient(), চাবি_সংখ্যা(points().point_সংখ্যা(pointnum).thresh()));
                যদি (hits >= needed) {
                    points().point_সংখ্যা(pointnum).testsCoveringInc();
                    testp->buckets().add_তথ্য(pointnum, hits);
                }
            }
        }
    }
}

শূন্য Vlc_শীর্ষ::writeCoverage(ধ্রুবক string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    যদি (!os) {
        v3fatal("Can't write file: " << filename);
        ফেরত;
    }

    os << "# SystemC::Coverage-3\n";
    জন্য (ধ্রুবক auto& i : m_points) {
        ধ্রুবক VlcPoint& point = m_points.point_সংখ্যা(i.second);
        os << "C '" << point.name() << "' " << point.গুনতি() << '\n';
    }
}

শূন্য Vlc_শীর্ষ::writeInfo(ধ্রুবক string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    যদি (!os) {
        v3fatal("Can't write file: " << filename);
        ফেরত;
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
    জন্য (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& সারিগুলি = source.সারিগুলি();
        পূর্ণসংখ্যা branchesFound = 0;
        পূর্ণসংখ্যা branchesHit = 0;
        জন্য (auto& li : সারিগুলি) {
            Vlc_Source_গুনতি& sc = li.second;
            uint64_t da_গুনতি = 0;
            std::vector<ধ্রুবক VlcPoint*> infoPoints;
            জন্য (ধ্রুবক auto& point : sc.points()) {
                da_গুনতি = std::max(da_গুনতি, point->গুনতি());
                infoPoints.ঠেলো_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_গুনতি << "\n";
            যদি (infoPoints.আকার_2() <= 1) চালু;
            branchesFound += static_cast<পূর্ণসংখ্যা>(infoPoints.আকার_2());
            পূর্ণসংখ্যা point_অঙ্ক_2 = 0;
            জন্য (ধ্রুবক VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                যদি (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } অন্যথা যদি (point->comment().empty()) {
                    os << "0,";
                    os << point_অঙ্ক_2;
                } অন্যথা {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->গুনতি() << "\n";

                branchesHit += opt.গুনতি_Ok(point->গুনতি());
                ++point_অঙ্ক_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

কাঠামো CmpComputrons final {
    যৌক্তিক operator()(ধ্রুবক VlcTest* lhsp, ধ্রুবক VlcTest* rhsp) ধ্রুবক {
        যদি (lhsp->computrons() != rhsp->computrons()) {
            ফেরত lhsp->computrons() < rhsp->computrons();
        }
        ফেরত lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

শূন্য Vlc_শীর্ষ::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    জন্য (ধ্রুবক auto& testp : m_tests) {
        যদি (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ঠেলো_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    জন্য (ধ্রুবক auto& i : m_points) {
        ধ্রুবক VlcPoint* ধ্রুবক pointp = &points().point_সংখ্যা(i.second);
        // If any tests hit this point, then we'll need to cover it.
        যদি (pointp->testsCovering()) remaining.add_তথ্য(pointp->point_অঙ্ক(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    যখন (সত্য) {
        যদি (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        জন্য (ধ্রুবক auto& testp : bytime) {
            যদি (!testp->rank()) {
                ধ্রুবক uint64_t remain = testp->buckets().তথ্য_বের_করো_গুনতি(remaining);
                যদি (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        যদি (VlcTest* ধ্রুবক testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.or_তথ্য(bestTestp->buckets());
        } অন্যথা {
            বিরতি;  // No test covering more stuff found
        }
    }
}

শূন্য Vlc_শীর্ষ::annotateCalc() {
    // Calculate per-line information into filedata structure
    জন্য (ধ্রুবক auto& i : m_points) {
        ধ্রুবক VlcPoint& point = m_points.point_সংখ্যা(i.second);
        ধ্রুবক string filename = point.filename();
        ধ্রুবক পূর্ণসংখ্যা lineno = point.lineno();
        যদি (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.গুনতি() << " " << point.linescov());
            // Base coverage
            source.ঢোকাও_Point(lineno, &point);
            // Additional lines covered by this statement
            যৌক্তিক range = মিথ্যা;
            পূর্ণসংখ্যা start = 0;
            পূর্ণসংখ্যা end = 0;
            ধ্রুবক string linescov = point.linescov();
            জন্য (ধ্রুবক অক্ষর* covp = linescov.c_str(); সত্য; ++covp) {
                যদি (!*covp || *covp == ',') {  // Ending
                    জন্য (পূর্ণসংখ্যা lni = start; start && lni <= end; ++lni) {
                        source.ঢোকাও_Point(lni, &point);
                    }
                    যদি (!*covp) বিরতি;
                    start = 0;  // Prep for next
                    end = 0;
                    range = মিথ্যা;
                } অন্যথা যদি (*covp == '-') {
                    range = সত্য;
                } অন্যথা যদি (std::isdigit(*covp)) {
                    ধ্রুবক অক্ষর* ধ্রুবক digitsp = covp;
                    যখন (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    যদি (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

শূন্য Vlc_শীর্ষ::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    পূর্ণসংখ্যা totCases = 0;
    পূর্ণসংখ্যা totOk = 0;
    জন্য (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        যদি (opt.annotateAll()) source.needed(সত্য);
        ধ্রুবক VlcSource::LinenoMap& সারিগুলি = source.সারিগুলি();
        জন্য (auto& li : সারিগুলি) {
            ধ্রুবক Vlc_Source_গুনতি& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            যদি (opt.গুনতি_Ok(sc.min_গুনতি())) {
                ++totOk;
            } অন্যথা {
                source.needed(সত্য);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    যদি (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

শূন্য Vlc_শীর্ষ::annotateOutputFiles(ধ্রুবক string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    জন্য (auto& si : m_sources) {
        VlcSource& source = si.second;
        যদি (!source.needed()) চালু;
        ধ্রুবক string filename = source.name();
        ধ্রুবক string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        যদি (!is) {
            v3error("Can't read annotation file: " << filename);
            ফেরত;
        }

        std::ofstream os{outfilename.c_str()};
        যদি (!os) {
            v3error("Can't write file: " << outfilename);
            ফেরত;
        }

        os << "//      // verilator_coverage annotation\n";

        পূর্ণসংখ্যা lineno = 0;
        যখন (!is.eof()) {
            lineno++;
            ধ্রুবক std::string সারি = V3Os::getline(is);

            VlcSource::LinenoMap& সারিগুলি = source.সারিগুলি();
            ধ্রুবক auto lit = সারিগুলি.find(lineno);
            যদি (lit == সারিগুলি.end()) {
                os << "        " << সারি << '\n';
            } অন্যথা {
                Vlc_Source_গুনতি& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ধ্রুবক যৌক্তিক minOk = opt.গুনতি_Ok(sc.min_গুনতি());
                ধ্রুবক যৌক্তিক maxOk = opt.গুনতি_Ok(sc.max_গুনতি());
                যদি (minOk) {
                    os << " ";
                } অন্যথা যদি (maxOk) {
                    os << "~";
                } অন্যথা {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_গুনতি() << " " << সারি << '\n';

                যদি (opt.annotatePoints()) {
                    জন্য (ধ্রুবক auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                যৌক্তিক printedFsmHeader = মিথ্যা;
                জন্য (ধ্রুবক auto& pit : sc.points()) {
                    যদি (!pit->isFsmState() && !pit->isFsmArc()) চালু;
                    যদি (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = সত্য;
                    }
                    os << (opt.গুনতি_Ok(pit->গুনতি()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->গুনতি() << "        ";
                    যদি (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        যদি (pit->গুনতি() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } অন্যথা যদি (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } অন্যথা {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        যদি (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

শূন্য Vlc_শীর্ষ::annotate(ধ্রুবক string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

শূন্য Vlc_শীর্ষ::printTypeSummary() {
    TypeTally tally;
    জন্য (VlcPoints::ByName::মান_type& i : m_points) {
        ধ্রুবক VlcPoint& pt = m_points.point_সংখ্যা(i.second);
        যদি (isCovergroup(pt)) চালু;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.গুনতি());
    }
    ধ্রুবক CgGroups groups = covergroups(m_points);
    যদি (!groups.empty()) {
        std::vector<ধ্রুবক CgGroup*> groupps;
        জন্য (ধ্রুবক CgGroups::মান_type& it : groups) groupps.ঠেলো_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    যদি (tally.empty()) ফেরত;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, সত্য);
}

শূন্য Vlc_শীর্ষ::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    যৌক্তিক hasHier = মিথ্যা;
    যৌক্তিক hasCollapsedHier = মিথ্যা;
    জন্য (VlcPoints::ByName::মান_type& i : m_points) {
        ধ্রুবক VlcPoint& pt = m_points.point_সংখ্যা(i.second);
        যদি (isCovergroup(pt)) চালু;  // Reported below, by covergroup
        ধ্রুবক string hier = reportHier(pt);
        যদি (hier.empty()) চালু;
        hasHier = সত্য;
        যদি (isCollapsedHier(hier)) hasCollapsedHier = সত্য;
        ধ্রুবক string type = displayType(pt);
        ধ্রুবক std::vector<string> parts = splitHier(hier);
        string path;
        জন্য (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.গুনতি());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.গুনতি());
    }
    ধ্রুবক std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    যদি (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        ফেরত;
    }
    যদি (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        ফেরত;
    }

    ধ্রুবক পূর্ণসংখ্যা levels = opt.reportLevels();
    যদি (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    জন্য (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        ধ্রুবক std::vector<string> parts = splitHier(it->first);
        যদি (levels >= 0 && static_cast<পূর্ণসংখ্যা>(parts.আকার_2()) > levels + 1) চালু;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, মিথ্যা);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    জন্য (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, মিথ্যা);
    }
    যদি (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
