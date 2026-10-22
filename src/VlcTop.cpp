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

#অন্তর্ভাৱ "VlcTop.h"

#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3Os.h"

#অন্তর্ভাৱ "VlcOptions.h"

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <fstream>
#অন্তর্ভাৱ <iomanip>
#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <sstream>
#অন্তর্ভাৱ <string>
#অন্তর্ভাৱ <vector>

//######################################################################

নামাকাশ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
শ্ৰেণী Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    তার্কিক m_scored = মিছা;  // Coverage is m_score, not the ratio of the points
    দ্ৱিগুণ m_score = 0.0;  // Percent coverage, if m_scored

সার্ৱজনিক:
    // ACCESSORS
    uint64_t hit() ধ্রুৱ { ঘূৰাই m_hit; }
    uint64_t total() ধ্রুৱ { ঘূৰাই m_total; }
    তার্কিক scored() ধ্রুৱ { ঘূৰাই m_scored; }
    দ্ৱিগুণ score() ধ্রুৱ { ঘূৰাই m_score; }
    শূন্য score(দ্ৱিগুণ মান) {
        m_scored = সঁচা;
        m_score = মান;
    }
    // METHODS
    শূন্য addPoint(তার্কিক covered) {
        যদি (covered) ++m_hit;
        ++m_total;
    }
    শূন্য addPoints(ধ্রুৱ Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
স্থির constexpr ধ্রুৱ ৱর্ণ* ধ্রুৱ s_covergroupType = "covergroup";
স্থির constexpr size_t s_summaryIndent = 2;
স্থির constexpr size_t s_reportRowIndent = 4;

string displayType(ধ্রুৱ VlcPoint& point) {
    ধ্রুৱ string type = point.type();
    ঘূৰাই type.empty() ? "point" : type;
}

তার্কিক isCollapsedHier(ধ্রুৱ string& hier) {
    ঘূৰাই hier.find('*') != string::npos || hier.find('?') != string::npos;
}

তার্কিক isOrderedType(ধ্রুৱ string& type) {
    বাবে (ধ্রুৱ ৱর্ণ* ধ্রুৱ typep : s_orderedTypes) {
        যদি (type == typep) ঘূৰাই সঁচা;
    }
    ঘূৰাই মিছা;
}

string reportHier(ধ্রুৱ VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    যদি (point.isFsmState() || point.isFsmArc()) {
        ধ্রুৱ string fsmVar = point.fsmVarName();
        ঘূৰাই fsmVar.substr(0, fsmVar.rfind('.'));
    }
    ঘূৰাই point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(ধ্রুৱ string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    পূর্ণাঙ্ক depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    যেতিয়ালৈ (pos < hier.size()) {
        ধ্রুৱ ৱর্ণ c = hier[pos];
        যদি (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            অনুৱর্ত;
        } অন্যথা যদি (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } অন্যথা যদি (!depth && c == '.') {
            parts.ঠেলো_back(hier.substr(start, pos - start));
            start = pos + 1;
        } অন্যথা যদি (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            অনুৱর্ত;
        } অন্যথা যদি (depth && c == '(') {
            ++depth;
        } অন্যথা যদি (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    যদি (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        বাবে (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.ঠেলো_back(hier.substr(start, dot - start));
        }
    }
    parts.ঠেলো_back(hier.substr(start));
    ঘূৰাই parts;
}

string duName(ধ্রুৱ VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ধ্রুৱ string page = point.page();
    ঘূৰাই page.substr(page.find('/') + 1);
}

শূন্য tallyPoint(TypeTally& tally, ধ্রুৱ string& type, uint64_t গুনতি) {
    tally[type].addPoint(গুনতি > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
দ্ৱিগুণ pct(uint64_t hit, uint64_t total) {
    ঘূৰাই total ? (100.0 * static_cast<দ্ৱিগুণ>(hit) / static_cast<দ্ৱিগুণ>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_মান_String(দ্ৱিগুণ মান, তার্কিক complete) {
    যদি (!complete && মান > 99.9) মান = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << মান << "%";
    ঘূৰাই os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    ঘূৰাই pct_মান_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
শ্ৰেণী CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_গুনতি = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

সার্ৱজনিক:
    // ACCESSORS
    ধ্রুৱ string& name() ধ্রুৱ { ঘূৰাই m_name; }
    তার্কিক covered() ধ্রুৱ { ঘূৰাই m_গুনতি >= m_atLeast; }
    // METHODS
    শূন্য addRecord(ধ্রুৱ string& name, uint64_t গুনতি, uint64_t atLeast) {
        m_name = name;
        m_গুনতি += গুনতি;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
শ্ৰেণী CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

সার্ৱজনিক:
    // ACCESSORS
    uint64_t weight() ধ্রুৱ { ঘূৰাই m_weight; }
    ধ্রুৱ std::map<string, CgBin>& bins() ধ্রুৱ { ঘূৰাই m_bins; }
    // METHODS
    শূন্য addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ধ্রুৱ string& identity) { ঘূৰাই m_bins[identity]; }
};

// A covergroup
শ্ৰেণী CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_বস্তুসমূহ;  // By hierarchy

সার্ৱজনিক:
    // ACCESSORS
    ধ্রুৱ string& hier() ধ্রুৱ { ঘূৰাই m_hier; }
    uint64_t weight() ধ্রুৱ { ঘূৰাই m_weight; }
    ধ্রুৱ std::map<string, CgItem>& বস্তুসমূহ() ধ্রুৱ { ঘূৰাই m_বস্তুসমূহ; }
    // METHODS
    শূন্য addRecord(ধ্রুৱ string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ধ্রুৱ string& hier) { ঘূৰাই m_বস্তুসমূহ[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t চাবি_সংখ্যা(ধ্রুৱ string& মান) {
    ঘূৰাই মান.empty() ? 1 : std::strtoull(মান.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
তার্কিক isScoreField(ধ্রুৱ string& field) {
    বাবে (ধ্রুৱ ৱর্ণ* ধ্রুৱ keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        ধ্রুৱ string prefix = string{"\001"} + keyp + "\002";
        যদি (field.compare(0, prefix.size(), prefix) == 0) ঘূৰাই সঁচা;
    }
    ঘূৰাই মিছা;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(ধ্রুৱ string& recordName) {
    string identity;
    string::size_type start = 0;
    যেতিয়ালৈ (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        যদি (end == string::npos) end = recordName.size();
        ধ্রুৱ string field = recordName.substr(start, end - start);
        যদি (!isScoreField(field)) identity += field;
        start = end;
    }
    ঘূৰাই identity;
}

তার্কিক isCovergroup(ধ্রুৱ VlcPoint& point) { ঘূৰাই point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    বাবে (ধ্রুৱ VlcPoints::ByName::মান_type& i : points) {
        ধ্রুৱ VlcPoint& pt = points.point_সংখ্যা(i.second);
        যদি (!isCovergroup(pt)) অনুৱর্ত;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ধ্রুৱ string hier = pt.hier();
        string bin = pt.bin();
        যদি (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        ধ্রুৱ string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), চাবি_সংখ্যা(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(চাবি_সংখ্যা(pt.weight()));
        যদি (!pt.binType().empty()) অনুৱর্ত;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.গুনতি(), চাবি_সংখ্যা(pt.thresh()));
    }
    ঘূৰাই groups;
}

Tally itemTally(ধ্রুৱ CgItem& item) {
    Tally tally;
    বাবে (ধ্রুৱ std::pair<ধ্রুৱ string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    ঘূৰাই tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(ধ্রুৱ CgGroup& group, তার্কিক* contributesp = nullptr) {
    Tally tally;
    দ্ৱিগুণ weighted = 0.0;
    দ্ৱিগুণ weights = 0.0;
    বাবে (ধ্রুৱ std::pair<ধ্রুৱ string, CgItem>& it : group.বস্তুসমূহ()) {
        ধ্রুৱ Tally item = itemTally(it.second);
        tally.addPoints(item);
        যদি (!item.total()) অনুৱর্ত;  // Excluded from both sums
        weighted += static_cast<দ্ৱিগুণ>(it.second.weight()) * item.score();
        weights += static_cast<দ্ৱিগুণ>(it.second.weight());
    }
    যদি (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    ঘূৰাই tally;
}

Tally groupsTally(ধ্রুৱ std::vector<ধ্রুৱ CgGroup*>& groups) {
    Tally tally;
    দ্ৱিগুণ weighted = 0.0;
    দ্ৱিগুণ weights = 0.0;
    তার্কিক anyWeight = মিছা;
    বাবে (ধ্রুৱ CgGroup* ধ্রুৱ groupp : groups) {
        তার্কিক contributes = মিছা;
        ধ্রুৱ Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        যদি (groupp->weight()) anyWeight = সঁচা;
        যদি (!contributes) অনুৱর্ত;
        weighted += static_cast<দ্ৱিগুণ>(groupp->weight()) * group.score();
        weights += static_cast<দ্ৱিগুণ>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    ঘূৰাই tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ধ্রুৱ CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<ধ্রুৱ CgGroup*>> গ্ৰন্থি_Groups;
    বাবে (ধ্রুৱ CgGroups::মান_type& it : groups) {
        ধ্রুৱ CgGroup& group = it.second;
        string path;
        বাবে (ধ্রুৱ string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            গ্ৰন্থি_Groups[path].ঠেলো_back(&group);
        }
        বাবে (ধ্রুৱ std::pair<ধ্রুৱ string, CgItem>& item : group.বস্তুসমূহ()) {
            tallies[item.first] = itemTally(item.second);
            বাবে (ধ্রুৱ std::pair<ধ্রুৱ string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    বাবে (ধ্রুৱ std::pair<ধ্রুৱ string, std::vector<ধ্রুৱ CgGroup*>>& it : গ্ৰন্থি_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    ঘূৰাই tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
শূন্য printIndent(size_t indent) {
    বাবে (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

শূন্য printTallyRow(ধ্রুৱ string& type, ধ্রুৱ Tally& tally, size_t indent, size_t typeWidth,
                   size_t গুনতি_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ধ্রুৱ string percent = tally.scored() ? pct_মান_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::বাম << std::setw(typeWidth) << type << " : " << std::ডান << std::fixed
              << std::setw(6) << percent << " (" << std::setw(গুনতি_Width) << tally.hit() << "/"
              << std::setw(গুনতি_Width) << tally.total() << ")\n";
}

size_t গুনতি_Width(ধ্রুৱ TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    বাবে (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    ঘূৰাই width;
}

size_t typeWidth(ধ্রুৱ TypeTally& tally) {
    size_t typeWidth = 0;
    বাবে (ধ্রুৱ ৱর্ণ* ধ্রুৱ typep : s_orderedTypes) {
        ধ্রুৱ string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    বাবে (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    ঘূৰাই typeWidth;
}

শূন্য printTypeTally(ধ্রুৱ TypeTally& tally, size_t indent, তার্কিক includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ধ্রুৱ size_t typWidth = typeWidth(tally);
    ধ্রুৱ size_t cntWidth = গুনতি_Width(tally);
    বাবে (ধ্রুৱ ৱর্ণ* ধ্রুৱ typep : s_orderedTypes) {
        ধ্রুৱ string type = typep;
        ধ্রুৱ TypeTally::const_iterator it = tally.find(type);
        যদি (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } অন্যথা যদি (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    বাবে (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        যদি (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
শূন্য printCovergroupTallies(ধ্রুৱ std::map<string, Tally>& tallies, পূর্ণাঙ্ক levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    বাবে (ধ্রুৱ std::pair<ধ্রুৱ string, Tally>& it : tallies) {
        যদি (levels >= 0 && static_cast<পূর্ণাঙ্ক>(splitHier(it.first).size()) > levels + 1) অনুৱর্ত;
        shown.ঢোকাও(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    ধ্রুৱ size_t cntWidth = গুনতি_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    বাবে (ধ্রুৱ std::pair<ধ্রুৱ string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

শূন্য Vlc_শীৰ্ষ::readCoverage(ধ্রুৱ string& filename, তার্কিক nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    যদি (!is) {
        যদি (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        ঘূৰাই;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ধ্রুৱ testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    যেতিয়ালৈ (!is.eof()) {
        ধ্রুৱ string সাৰি = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        যদি (সাৰি[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ধ্রুৱ string::size_type secspace = সাৰি.rfind("' ");
            যদি (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                অনুৱর্ত;
            }
            ধ্রুৱ string point = সাৰি.substr(3, secspace - 3);
            যদি (!opt.isTypeMatch(point.c_str())) অনুৱর্ত;

            ধ্রুৱ uint64_t hits = std::atoll(সাৰি.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ধ্রুৱ uint64_t pointnum = points().findAddPoint(point, hits);
            যদি (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ধ্রুৱ uint64_t needed = std::max(
                    VlcBuckets::sufficient(), চাবি_সংখ্যা(points().point_সংখ্যা(pointnum).thresh()));
                যদি (hits >= needed) {
                    points().point_সংখ্যা(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

শূন্য Vlc_শীৰ্ষ::writeCoverage(ধ্রুৱ string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    যদি (!os) {
        v3fatal("Can't write file: " << filename);
        ঘূৰাই;
    }

    os << "# SystemC::Coverage-3\n";
    বাবে (ধ্রুৱ auto& i : m_points) {
        ধ্রুৱ VlcPoint& point = m_points.point_সংখ্যা(i.second);
        os << "C '" << point.name() << "' " << point.গুনতি() << '\n';
    }
}

শূন্য Vlc_শীৰ্ষ::writeInfo(ধ্রুৱ string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    যদি (!os) {
        v3fatal("Can't write file: " << filename);
        ঘূৰাই;
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
    বাবে (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& সাৰিগুলি = source.সাৰিগুলি();
        পূর্ণাঙ্ক branchesFound = 0;
        পূর্ণাঙ্ক branchesHit = 0;
        বাবে (auto& li : সাৰিগুলি) {
            Vlc_Source_গুনতি& sc = li.second;
            uint64_t da_গুনতি = 0;
            std::vector<ধ্রুৱ VlcPoint*> infoPoints;
            বাবে (ধ্রুৱ auto& point : sc.points()) {
                da_গুনতি = std::max(da_গুনতি, point->গুনতি());
                infoPoints.ঠেলো_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_গুনতি << "\n";
            যদি (infoPoints.size() <= 1) অনুৱর্ত;
            branchesFound += static_cast<পূর্ণাঙ্ক>(infoPoints.size());
            পূর্ণাঙ্ক point_অঙ্ক_2 = 0;
            বাবে (ধ্রুৱ VlcPoint* point : infoPoints) {
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

সংরচনা CmpComputrons final {
    তার্কিক operator()(ধ্রুৱ VlcTest* lhsp, ধ্রুৱ VlcTest* rhsp) ধ্রুৱ {
        যদি (lhsp->computrons() != rhsp->computrons()) {
            ঘূৰাই lhsp->computrons() < rhsp->computrons();
        }
        ঘূৰাই lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

শূন্য Vlc_শীৰ্ষ::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    বাবে (ধ্রুৱ auto& testp : m_tests) {
        যদি (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ঠেলো_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    বাবে (ধ্রুৱ auto& i : m_points) {
        ধ্রুৱ VlcPoint* ধ্রুৱ pointp = &points().point_সংখ্যা(i.second);
        // If any tests hit this point, then we'll need to cover it.
        যদি (pointp->testsCovering()) remaining.addData(pointp->point_অঙ্ক(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    যেতিয়ালৈ (সঁচা) {
        যদি (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        বাবে (ধ্রুৱ auto& testp : bytime) {
            যদি (!testp->rank()) {
                ধ্রুৱ uint64_t remain = testp->buckets().data_বেৰ_কৰো_গুনতি(remaining);
                যদি (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        যদি (VlcTest* ধ্রুৱ testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } অন্যথা {
            ভঙ্গ;  // No test covering more stuff found
        }
    }
}

শূন্য Vlc_শীৰ্ষ::annotateCalc() {
    // Calculate per-line information into filedata structure
    বাবে (ধ্রুৱ auto& i : m_points) {
        ধ্রুৱ VlcPoint& point = m_points.point_সংখ্যা(i.second);
        ধ্রুৱ string filename = point.filename();
        ধ্রুৱ পূর্ণাঙ্ক lineno = point.lineno();
        যদি (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.গুনতি() << " " << point.linescov());
            // Base coverage
            source.ঢোকাও_Point(lineno, &point);
            // Additional lines covered by this statement
            তার্কিক range = মিছা;
            পূর্ণাঙ্ক start = 0;
            পূর্ণাঙ্ক end = 0;
            ধ্রুৱ string linescov = point.linescov();
            বাবে (ধ্রুৱ ৱর্ণ* covp = linescov.c_str(); সঁচা; ++covp) {
                যদি (!*covp || *covp == ',') {  // Ending
                    বাবে (পূর্ণাঙ্ক lni = start; start && lni <= end; ++lni) {
                        source.ঢোকাও_Point(lni, &point);
                    }
                    যদি (!*covp) ভঙ্গ;
                    start = 0;  // Prep for next
                    end = 0;
                    range = মিছা;
                } অন্যথা যদি (*covp == '-') {
                    range = সঁচা;
                } অন্যথা যদি (std::isdigit(*covp)) {
                    ধ্রুৱ ৱর্ণ* ধ্রুৱ digitsp = covp;
                    যেতিয়ালৈ (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    যদি (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

শূন্য Vlc_শীৰ্ষ::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    পূর্ণাঙ্ক totCases = 0;
    পূর্ণাঙ্ক totOk = 0;
    বাবে (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        যদি (opt.annotateAll()) source.needed(সঁচা);
        ধ্রুৱ VlcSource::LinenoMap& সাৰিগুলি = source.সাৰিগুলি();
        বাবে (auto& li : সাৰিগুলি) {
            ধ্রুৱ Vlc_Source_গুনতি& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            যদি (opt.গুনতি_Ok(sc.min_গুনতি())) {
                ++totOk;
            } অন্যথা {
                source.needed(সঁচা);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    যদি (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

শূন্য Vlc_শীৰ্ষ::annotateOutputFiles(ধ্রুৱ string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    বাবে (auto& si : m_sources) {
        VlcSource& source = si.second;
        যদি (!source.needed()) অনুৱর্ত;
        ধ্রুৱ string filename = source.name();
        ধ্রুৱ string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        যদি (!is) {
            v3error("Can't read annotation file: " << filename);
            ঘূৰাই;
        }

        std::ofstream os{outfilename.c_str()};
        যদি (!os) {
            v3error("Can't write file: " << outfilename);
            ঘূৰাই;
        }

        os << "//      // verilator_coverage annotation\n";

        পূর্ণাঙ্ক lineno = 0;
        যেতিয়ালৈ (!is.eof()) {
            lineno++;
            ধ্রুৱ std::string সাৰি = V3Os::getline(is);

            VlcSource::LinenoMap& সাৰিগুলি = source.সাৰিগুলি();
            ধ্রুৱ auto lit = সাৰিগুলি.find(lineno);
            যদি (lit == সাৰিগুলি.end()) {
                os << "        " << সাৰি << '\n';
            } অন্যথা {
                Vlc_Source_গুনতি& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ধ্রুৱ তার্কিক minOk = opt.গুনতি_Ok(sc.min_গুনতি());
                ধ্রুৱ তার্কিক maxOk = opt.গুনতি_Ok(sc.max_গুনতি());
                যদি (minOk) {
                    os << " ";
                } অন্যথা যদি (maxOk) {
                    os << "~";
                } অন্যথা {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_গুনতি() << " " << সাৰি << '\n';

                যদি (opt.annotatePoints()) {
                    বাবে (ধ্রুৱ auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                তার্কিক printedFsmHeader = মিছা;
                বাবে (ধ্রুৱ auto& pit : sc.points()) {
                    যদি (!pit->isFsmState() && !pit->isFsmArc()) অনুৱর্ত;
                    যদি (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = সঁচা;
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

শূন্য Vlc_শীৰ্ষ::annotate(ধ্রুৱ string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

শূন্য Vlc_শীৰ্ষ::printTypeSummary() {
    TypeTally tally;
    বাবে (VlcPoints::ByName::মান_type& i : m_points) {
        ধ্রুৱ VlcPoint& pt = m_points.point_সংখ্যা(i.second);
        যদি (isCovergroup(pt)) অনুৱর্ত;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.গুনতি());
    }
    ধ্রুৱ CgGroups groups = covergroups(m_points);
    যদি (!groups.empty()) {
        std::vector<ধ্রুৱ CgGroup*> groupps;
        বাবে (ধ্রুৱ CgGroups::মান_type& it : groups) groupps.ঠেলো_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    যদি (tally.empty()) ঘূৰাই;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, সঁচা);
}

শূন্য Vlc_শীৰ্ষ::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    তার্কিক hasHier = মিছা;
    তার্কিক hasCollapsedHier = মিছা;
    বাবে (VlcPoints::ByName::মান_type& i : m_points) {
        ধ্রুৱ VlcPoint& pt = m_points.point_সংখ্যা(i.second);
        যদি (isCovergroup(pt)) অনুৱর্ত;  // Reported below, by covergroup
        ধ্রুৱ string hier = reportHier(pt);
        যদি (hier.empty()) অনুৱর্ত;
        hasHier = সঁচা;
        যদি (isCollapsedHier(hier)) hasCollapsedHier = সঁচা;
        ধ্রুৱ string type = displayType(pt);
        ধ্রুৱ std::vector<string> parts = splitHier(hier);
        string path;
        বাবে (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.গুনতি());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.গুনতি());
    }
    ধ্রুৱ std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    যদি (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        ঘূৰাই;
    }
    যদি (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        ঘূৰাই;
    }

    ধ্রুৱ পূর্ণাঙ্ক levels = opt.reportLevels();
    যদি (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    বাবে (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        ধ্রুৱ std::vector<string> parts = splitHier(it->first);
        যদি (levels >= 0 && static_cast<পূর্ণাঙ্ক>(parts.size()) > levels + 1) অনুৱর্ত;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, মিছা);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    বাবে (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, মিছা);
    }
    যদি (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
