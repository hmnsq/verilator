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

#ಅನ್ತರ್ಭಾವ "VlcTop.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"

#ಅನ್ತರ್ಭಾವ "VlcOptions.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <fstream>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <sstream>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

//######################################################################

ನಾಮಾಕಾಶ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
ವರ್ಗ Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    ತಾರ್ಕಿಕ m_scored = ಅಸತ್ಯ;  // Coverage is m_score, not the ratio of the points
    ದ್ವಿಗುಣ m_score = 0.0;  // Percent coverage, if m_scored

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    uint64_t hit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hit; }
    uint64_t total() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_total; }
    ತಾರ್ಕಿಕ scored() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_scored; }
    ದ್ವಿಗುಣ score() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_score; }
    ಶೂನ್ಯ score(ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
        m_scored = ಸತ್ಯ;
        m_score = ಮೌಲ್ಯ;
    }
    // METHODS
    ಶೂನ್ಯ addPoint(ತಾರ್ಕಿಕ covered) {
        ಯದಿ (covered) ++m_hit;
        ++m_total;
    }
    ಶೂನ್ಯ addPoints(ಧ್ರುವ Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ s_covergroupType = "covergroup";
ಸ್ಥಿರ constexpr size_t s_summaryIndent = 2;
ಸ್ಥಿರ constexpr size_t s_reportRowIndent = 4;

string displayType(ಧ್ರುವ VlcPoint& point) {
    ಧ್ರುವ string type = point.type();
    ಪ್ರತ್ಯಾಗಮ type.empty() ? "point" : type;
}

ತಾರ್ಕಿಕ isCollapsedHier(ಧ್ರುವ string& hier) {
    ಪ್ರತ್ಯಾಗಮ hier.find('*') != string::npos || hier.find('?') != string::npos;
}

ತಾರ್ಕಿಕ isOrderedType(ಧ್ರುವ string& type) {
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ typep : s_orderedTypes) {
        ಯದಿ (type == typep) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

string reportHier(ಧ್ರುವ VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    ಯದಿ (point.isFsmState() || point.isFsmArc()) {
        ಧ್ರುವ string fsmVar = point.fsmVarName();
        ಪ್ರತ್ಯಾಗಮ fsmVar.substr(0, fsmVar.rfind('.'));
    }
    ಪ್ರತ್ಯಾಗಮ point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(ಧ್ರುವ string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    ಪೂರ್ಣಾಙ್ಕ depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    ಯಾವತ್ (pos < hier.size()) {
        ಧ್ರುವ ವರ್ಣ c = hier[pos];
        ಯದಿ (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            ಅನುವರ್ತ;
        } ಅನ್ಯಥಾ ಯದಿ (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } ಅನ್ಯಥಾ ಯದಿ (!depth && c == '.') {
            parts.ತಳ್ಳು_back(hier.substr(start, pos - start));
            start = pos + 1;
        } ಅನ್ಯಥಾ ಯದಿ (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            ಅನುವರ್ತ;
        } ಅನ್ಯಥಾ ಯದಿ (depth && c == '(') {
            ++depth;
        } ಅನ್ಯಥಾ ಯದಿ (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    ಯದಿ (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        ಪ್ರತಿ (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.ತಳ್ಳು_back(hier.substr(start, dot - start));
        }
    }
    parts.ತಳ್ಳು_back(hier.substr(start));
    ಪ್ರತ್ಯಾಗಮ parts;
}

string duName(ಧ್ರುವ VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ಧ್ರುವ string page = point.page();
    ಪ್ರತ್ಯಾಗಮ page.substr(page.find('/') + 1);
}

ಶೂನ್ಯ tallyPoint(TypeTally& tally, ಧ್ರುವ string& type, uint64_t ಎಣಿಕೆ) {
    tally[type].addPoint(ಎಣಿಕೆ > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
ದ್ವಿಗುಣ pct(uint64_t hit, uint64_t total) {
    ಪ್ರತ್ಯಾಗಮ total ? (100.0 * static_cast<ದ್ವಿಗುಣ>(hit) / static_cast<ದ್ವಿಗುಣ>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_ಮೌಲ್ಯ_String(ದ್ವಿಗುಣ ಮೌಲ್ಯ, ತಾರ್ಕಿಕ complete) {
    ಯದಿ (!complete && ಮೌಲ್ಯ > 99.9) ಮೌಲ್ಯ = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << ಮೌಲ್ಯ << "%";
    ಪ್ರತ್ಯಾಗಮ os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    ಪ್ರತ್ಯಾಗಮ pct_ಮೌಲ್ಯ_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
ವರ್ಗ CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_ಎಣಿಕೆ = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    ಧ್ರುವ string& name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }
    ತಾರ್ಕಿಕ covered() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಎಣಿಕೆ >= m_atLeast; }
    // METHODS
    ಶೂನ್ಯ addRecord(ಧ್ರುವ string& name, uint64_t ಎಣಿಕೆ, uint64_t atLeast) {
        m_name = name;
        m_ಎಣಿಕೆ += ಎಣಿಕೆ;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
ವರ್ಗ CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    uint64_t weight() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_weight; }
    ಧ್ರುವ std::map<string, CgBin>& bins() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bins; }
    // METHODS
    ಶೂನ್ಯ addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ಧ್ರುವ string& identity) { ಪ್ರತ್ಯಾಗಮ m_bins[identity]; }
};

// A covergroup
ವರ್ಗ CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_ವಸ್ತುಗಳು;  // By hierarchy

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    ಧ್ರುವ string& hier() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hier; }
    uint64_t weight() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_weight; }
    ಧ್ರುವ std::map<string, CgItem>& ವಸ್ತುಗಳು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ವಸ್ತುಗಳು; }
    // METHODS
    ಶೂನ್ಯ addRecord(ಧ್ರುವ string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ಧ್ರುವ string& hier) { ಪ್ರತ್ಯಾಗಮ m_ವಸ್ತುಗಳು[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t ಕೀಲಿ_ಸಂಖ್ಯೆ(ಧ್ರುವ string& ಮೌಲ್ಯ) {
    ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ.empty() ? 1 : std::strtoull(ಮೌಲ್ಯ.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
ತಾರ್ಕಿಕ isScoreField(ಧ್ರುವ string& field) {
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        ಧ್ರುವ string prefix = string{"\001"} + keyp + "\002";
        ಯದಿ (field.compare(0, prefix.size(), prefix) == 0) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(ಧ್ರುವ string& recordName) {
    string identity;
    string::size_type start = 0;
    ಯಾವತ್ (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        ಯದಿ (end == string::npos) end = recordName.size();
        ಧ್ರುವ string field = recordName.substr(start, end - start);
        ಯದಿ (!isScoreField(field)) identity += field;
        start = end;
    }
    ಪ್ರತ್ಯಾಗಮ identity;
}

ತಾರ್ಕಿಕ isCovergroup(ಧ್ರುವ VlcPoint& point) { ಪ್ರತ್ಯಾಗಮ point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    ಪ್ರತಿ (ಧ್ರುವ VlcPoints::ByName::ಮೌಲ್ಯ_type& i : points) {
        ಧ್ರುವ VlcPoint& pt = points.point_ಸಂಖ್ಯೆ(i.second);
        ಯದಿ (!isCovergroup(pt)) ಅನುವರ್ತ;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ಧ್ರುವ string hier = pt.hier();
        string bin = pt.bin();
        ಯದಿ (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        ಧ್ರುವ string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), ಕೀಲಿ_ಸಂಖ್ಯೆ(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(ಕೀಲಿ_ಸಂಖ್ಯೆ(pt.weight()));
        ಯದಿ (!pt.binType().empty()) ಅನುವರ್ತ;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.ಎಣಿಕೆ(), ಕೀಲಿ_ಸಂಖ್ಯೆ(pt.thresh()));
    }
    ಪ್ರತ್ಯಾಗಮ groups;
}

Tally itemTally(ಧ್ರುವ CgItem& item) {
    Tally tally;
    ಪ್ರತಿ (ಧ್ರುವ std::pair<ಧ್ರುವ string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    ಪ್ರತ್ಯಾಗಮ tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(ಧ್ರುವ CgGroup& group, ತಾರ್ಕಿಕ* contributesp = nullptr) {
    Tally tally;
    ದ್ವಿಗುಣ weighted = 0.0;
    ದ್ವಿಗುಣ weights = 0.0;
    ಪ್ರತಿ (ಧ್ರುವ std::pair<ಧ್ರುವ string, CgItem>& it : group.ವಸ್ತುಗಳು()) {
        ಧ್ರುವ Tally item = itemTally(it.second);
        tally.addPoints(item);
        ಯದಿ (!item.total()) ಅನುವರ್ತ;  // Excluded from both sums
        weighted += static_cast<ದ್ವಿಗುಣ>(it.second.weight()) * item.score();
        weights += static_cast<ದ್ವಿಗುಣ>(it.second.weight());
    }
    ಯದಿ (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    ಪ್ರತ್ಯಾಗಮ tally;
}

Tally groupsTally(ಧ್ರುವ std::vector<ಧ್ರುವ CgGroup*>& groups) {
    Tally tally;
    ದ್ವಿಗುಣ weighted = 0.0;
    ದ್ವಿಗುಣ weights = 0.0;
    ತಾರ್ಕಿಕ anyWeight = ಅಸತ್ಯ;
    ಪ್ರತಿ (ಧ್ರುವ CgGroup* ಧ್ರುವ groupp : groups) {
        ತಾರ್ಕಿಕ contributes = ಅಸತ್ಯ;
        ಧ್ರುವ Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        ಯದಿ (groupp->weight()) anyWeight = ಸತ್ಯ;
        ಯದಿ (!contributes) ಅನುವರ್ತ;
        weighted += static_cast<ದ್ವಿಗುಣ>(groupp->weight()) * group.score();
        weights += static_cast<ದ್ವಿಗುಣ>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    ಪ್ರತ್ಯಾಗಮ tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ಧ್ರುವ CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<ಧ್ರುವ CgGroup*>> ಗಂಟು_Groups;
    ಪ್ರತಿ (ಧ್ರುವ CgGroups::ಮೌಲ್ಯ_type& it : groups) {
        ಧ್ರುವ CgGroup& group = it.second;
        string path;
        ಪ್ರತಿ (ಧ್ರುವ string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            ಗಂಟು_Groups[path].ತಳ್ಳು_back(&group);
        }
        ಪ್ರತಿ (ಧ್ರುವ std::pair<ಧ್ರುವ string, CgItem>& item : group.ವಸ್ತುಗಳು()) {
            tallies[item.first] = itemTally(item.second);
            ಪ್ರತಿ (ಧ್ರುವ std::pair<ಧ್ರುವ string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    ಪ್ರತಿ (ಧ್ರುವ std::pair<ಧ್ರುವ string, std::vector<ಧ್ರುವ CgGroup*>>& it : ಗಂಟು_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    ಪ್ರತ್ಯಾಗಮ tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
ಶೂನ್ಯ printIndent(size_t indent) {
    ಪ್ರತಿ (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

ಶೂನ್ಯ printTallyRow(ಧ್ರುವ string& type, ಧ್ರುವ Tally& tally, size_t indent, size_t typeWidth,
                   size_t ಎಣಿಕೆ_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ಧ್ರುವ string percent = tally.scored() ? pct_ಮೌಲ್ಯ_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::ಎಡ << std::setw(typeWidth) << type << " : " << std::ಬಲ << std::fixed
              << std::setw(6) << percent << " (" << std::setw(ಎಣಿಕೆ_Width) << tally.hit() << "/"
              << std::setw(ಎಣಿಕೆ_Width) << tally.total() << ")\n";
}

size_t ಎಣಿಕೆ_Width(ಧ್ರುವ TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    ಪ್ರತಿ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    ಪ್ರತ್ಯಾಗಮ width;
}

size_t typeWidth(ಧ್ರುವ TypeTally& tally) {
    size_t typeWidth = 0;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ typep : s_orderedTypes) {
        ಧ್ರುವ string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    ಪ್ರತಿ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    ಪ್ರತ್ಯಾಗಮ typeWidth;
}

ಶೂನ್ಯ printTypeTally(ಧ್ರುವ TypeTally& tally, size_t indent, ತಾರ್ಕಿಕ includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ಧ್ರುವ size_t typWidth = typeWidth(tally);
    ಧ್ರುವ size_t cntWidth = ಎಣಿಕೆ_Width(tally);
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ typep : s_orderedTypes) {
        ಧ್ರುವ string type = typep;
        ಧ್ರುವ TypeTally::const_iterator it = tally.find(type);
        ಯದಿ (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } ಅನ್ಯಥಾ ಯದಿ (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    ಪ್ರತಿ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        ಯದಿ (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
ಶೂನ್ಯ printCovergroupTallies(ಧ್ರುವ std::map<string, Tally>& tallies, ಪೂರ್ಣಾಙ್ಕ levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    ಪ್ರತಿ (ಧ್ರುವ std::pair<ಧ್ರುವ string, Tally>& it : tallies) {
        ಯದಿ (levels >= 0 && static_cast<ಪೂರ್ಣಾಙ್ಕ>(splitHier(it.first).size()) > levels + 1) ಅನುವರ್ತ;
        shown.ಸೇರಿಸು(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    ಧ್ರುವ size_t cntWidth = ಎಣಿಕೆ_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    ಪ್ರತಿ (ಧ್ರುವ std::pair<ಧ್ರುವ string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

ಶೂನ್ಯ Vlc_ತುದಿ::readCoverage(ಧ್ರುವ string& filename, ತಾರ್ಕಿಕ nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    ಯದಿ (!is) {
        ಯದಿ (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        ಪ್ರತ್ಯಾಗಮ;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ಧ್ರುವ testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    ಯಾವತ್ (!is.eof()) {
        ಧ್ರುವ string ಸಾಲು = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        ಯದಿ (ಸಾಲು[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ಧ್ರುವ string::size_type secspace = ಸಾಲು.rfind("' ");
            ಯದಿ (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                ಅನುವರ್ತ;
            }
            ಧ್ರುವ string point = ಸಾಲು.substr(3, secspace - 3);
            ಯದಿ (!opt.isTypeMatch(point.c_str())) ಅನುವರ್ತ;

            ಧ್ರುವ uint64_t hits = std::atoll(ಸಾಲು.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ಧ್ರುವ uint64_t pointnum = points().findAddPoint(point, hits);
            ಯದಿ (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ಧ್ರುವ uint64_t needed = std::max(
                    VlcBuckets::sufficient(), ಕೀಲಿ_ಸಂಖ್ಯೆ(points().point_ಸಂಖ್ಯೆ(pointnum).thresh()));
                ಯದಿ (hits >= needed) {
                    points().point_ಸಂಖ್ಯೆ(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::writeCoverage(ಧ್ರುವ string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    ಯದಿ (!os) {
        v3fatal("Can't write file: " << filename);
        ಪ್ರತ್ಯಾಗಮ;
    }

    os << "# SystemC::Coverage-3\n";
    ಪ್ರತಿ (ಧ್ರುವ auto& i : m_points) {
        ಧ್ರುವ VlcPoint& point = m_points.point_ಸಂಖ್ಯೆ(i.second);
        os << "C '" << point.name() << "' " << point.ಎಣಿಕೆ() << '\n';
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::writeInfo(ಧ್ರುವ string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    ಯದಿ (!os) {
        v3fatal("Can't write file: " << filename);
        ಪ್ರತ್ಯಾಗಮ;
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
    ಪ್ರತಿ (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& ಸಾಲುಗಳು = source.ಸಾಲುಗಳು();
        ಪೂರ್ಣಾಙ್ಕ branchesFound = 0;
        ಪೂರ್ಣಾಙ್ಕ branchesHit = 0;
        ಪ್ರತಿ (auto& li : ಸಾಲುಗಳು) {
            Vlc_Source_ಎಣಿಕೆ& sc = li.second;
            uint64_t da_ಎಣಿಕೆ = 0;
            std::vector<ಧ್ರುವ VlcPoint*> infoPoints;
            ಪ್ರತಿ (ಧ್ರುವ auto& point : sc.points()) {
                da_ಎಣಿಕೆ = std::max(da_ಎಣಿಕೆ, point->ಎಣಿಕೆ());
                infoPoints.ತಳ್ಳು_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_ಎಣಿಕೆ << "\n";
            ಯದಿ (infoPoints.size() <= 1) ಅನುವರ್ತ;
            branchesFound += static_cast<ಪೂರ್ಣಾಙ್ಕ>(infoPoints.size());
            ಪೂರ್ಣಾಙ್ಕ point_ಅಂಕೆ_2 = 0;
            ಪ್ರತಿ (ಧ್ರುವ VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                ಯದಿ (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } ಅನ್ಯಥಾ ಯದಿ (point->comment().empty()) {
                    os << "0,";
                    os << point_ಅಂಕೆ_2;
                } ಅನ್ಯಥಾ {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->ಎಣಿಕೆ() << "\n";

                branchesHit += opt.ಎಣಿಕೆ_Ok(point->ಎಣಿಕೆ());
                ++point_ಅಂಕೆ_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

ಸಂರಚನಾ CmpComputrons final {
    ತಾರ್ಕಿಕ operator()(ಧ್ರುವ VlcTest* lhsp, ಧ್ರುವ VlcTest* rhsp) ಧ್ರುವ {
        ಯದಿ (lhsp->computrons() != rhsp->computrons()) {
            ಪ್ರತ್ಯಾಗಮ lhsp->computrons() < rhsp->computrons();
        }
        ಪ್ರತ್ಯಾಗಮ lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

ಶೂನ್ಯ Vlc_ತುದಿ::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    ಪ್ರತಿ (ಧ್ರುವ auto& testp : m_tests) {
        ಯದಿ (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ತಳ್ಳು_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    ಪ್ರತಿ (ಧ್ರುವ auto& i : m_points) {
        ಧ್ರುವ VlcPoint* ಧ್ರುವ pointp = &points().point_ಸಂಖ್ಯೆ(i.second);
        // If any tests hit this point, then we'll need to cover it.
        ಯದಿ (pointp->testsCovering()) remaining.addData(pointp->point_ಅಂಕೆ(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    ಯಾವತ್ (ಸತ್ಯ) {
        ಯದಿ (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        ಪ್ರತಿ (ಧ್ರುವ auto& testp : bytime) {
            ಯದಿ (!testp->rank()) {
                ಧ್ರುವ uint64_t remain = testp->buckets().data_ತೆಗೆ_ಎಣಿಕೆ(remaining);
                ಯದಿ (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        ಯದಿ (VlcTest* ಧ್ರುವ testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } ಅನ್ಯಥಾ {
            ಭಙ್ಗ;  // No test covering more stuff found
        }
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::annotateCalc() {
    // Calculate per-line information into filedata structure
    ಪ್ರತಿ (ಧ್ರುವ auto& i : m_points) {
        ಧ್ರುವ VlcPoint& point = m_points.point_ಸಂಖ್ಯೆ(i.second);
        ಧ್ರುವ string filename = point.filename();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lineno = point.lineno();
        ಯದಿ (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.ಎಣಿಕೆ() << " " << point.linescov());
            // Base coverage
            source.ಸೇರಿಸು_Point(lineno, &point);
            // Additional lines covered by this statement
            ತಾರ್ಕಿಕ range = ಅಸತ್ಯ;
            ಪೂರ್ಣಾಙ್ಕ start = 0;
            ಪೂರ್ಣಾಙ್ಕ end = 0;
            ಧ್ರುವ string linescov = point.linescov();
            ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* covp = linescov.c_str(); ಸತ್ಯ; ++covp) {
                ಯದಿ (!*covp || *covp == ',') {  // Ending
                    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ lni = start; start && lni <= end; ++lni) {
                        source.ಸೇರಿಸು_Point(lni, &point);
                    }
                    ಯದಿ (!*covp) ಭಙ್ಗ;
                    start = 0;  // Prep for next
                    end = 0;
                    range = ಅಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (*covp == '-') {
                    range = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (std::isdigit(*covp)) {
                    ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ digitsp = covp;
                    ಯಾವತ್ (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    ಯದಿ (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    ಪೂರ್ಣಾಙ್ಕ totCases = 0;
    ಪೂರ್ಣಾಙ್ಕ totOk = 0;
    ಪ್ರತಿ (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        ಯದಿ (opt.annotateAll()) source.needed(ಸತ್ಯ);
        ಧ್ರುವ VlcSource::LinenoMap& ಸಾಲುಗಳು = source.ಸಾಲುಗಳು();
        ಪ್ರತಿ (auto& li : ಸಾಲುಗಳು) {
            ಧ್ರುವ Vlc_Source_ಎಣಿಕೆ& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            ಯದಿ (opt.ಎಣಿಕೆ_Ok(sc.min_ಎಣಿಕೆ())) {
                ++totOk;
            } ಅನ್ಯಥಾ {
                source.needed(ಸತ್ಯ);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    ಯದಿ (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

ಶೂನ್ಯ Vlc_ತುದಿ::annotateOutputFiles(ಧ್ರುವ string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    ಪ್ರತಿ (auto& si : m_sources) {
        VlcSource& source = si.second;
        ಯದಿ (!source.needed()) ಅನುವರ್ತ;
        ಧ್ರುವ string filename = source.name();
        ಧ್ರುವ string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        ಯದಿ (!is) {
            v3error("Can't read annotation file: " << filename);
            ಪ್ರತ್ಯಾಗಮ;
        }

        std::ofstream os{outfilename.c_str()};
        ಯದಿ (!os) {
            v3error("Can't write file: " << outfilename);
            ಪ್ರತ್ಯಾಗಮ;
        }

        os << "//      // verilator_coverage annotation\n";

        ಪೂರ್ಣಾಙ್ಕ lineno = 0;
        ಯಾವತ್ (!is.eof()) {
            lineno++;
            ಧ್ರುವ std::string ಸಾಲು = V3Os::getline(is);

            VlcSource::LinenoMap& ಸಾಲುಗಳು = source.ಸಾಲುಗಳು();
            ಧ್ರುವ auto lit = ಸಾಲುಗಳು.find(lineno);
            ಯದಿ (lit == ಸಾಲುಗಳು.end()) {
                os << "        " << ಸಾಲು << '\n';
            } ಅನ್ಯಥಾ {
                Vlc_Source_ಎಣಿಕೆ& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ಧ್ರುವ ತಾರ್ಕಿಕ minOk = opt.ಎಣಿಕೆ_Ok(sc.min_ಎಣಿಕೆ());
                ಧ್ರುವ ತಾರ್ಕಿಕ maxOk = opt.ಎಣಿಕೆ_Ok(sc.max_ಎಣಿಕೆ());
                ಯದಿ (minOk) {
                    os << " ";
                } ಅನ್ಯಥಾ ಯದಿ (maxOk) {
                    os << "~";
                } ಅನ್ಯಥಾ {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_ಎಣಿಕೆ() << " " << ಸಾಲು << '\n';

                ಯದಿ (opt.annotatePoints()) {
                    ಪ್ರತಿ (ಧ್ರುವ auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                ತಾರ್ಕಿಕ printedFsmHeader = ಅಸತ್ಯ;
                ಪ್ರತಿ (ಧ್ರುವ auto& pit : sc.points()) {
                    ಯದಿ (!pit->isFsmState() && !pit->isFsmArc()) ಅನುವರ್ತ;
                    ಯದಿ (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = ಸತ್ಯ;
                    }
                    os << (opt.ಎಣಿಕೆ_Ok(pit->ಎಣಿಕೆ()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->ಎಣಿಕೆ() << "        ";
                    ಯದಿ (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        ಯದಿ (pit->ಎಣಿಕೆ() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } ಅನ್ಯಥಾ ಯದಿ (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } ಅನ್ಯಥಾ {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        ಯದಿ (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::annotate(ಧ್ರುವ string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

ಶೂನ್ಯ Vlc_ತುದಿ::printTypeSummary() {
    TypeTally tally;
    ಪ್ರತಿ (VlcPoints::ByName::ಮೌಲ್ಯ_type& i : m_points) {
        ಧ್ರುವ VlcPoint& pt = m_points.point_ಸಂಖ್ಯೆ(i.second);
        ಯದಿ (isCovergroup(pt)) ಅನುವರ್ತ;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.ಎಣಿಕೆ());
    }
    ಧ್ರುವ CgGroups groups = covergroups(m_points);
    ಯದಿ (!groups.empty()) {
        std::vector<ಧ್ರುವ CgGroup*> groupps;
        ಪ್ರತಿ (ಧ್ರುವ CgGroups::ಮೌಲ್ಯ_type& it : groups) groupps.ತಳ್ಳು_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    ಯದಿ (tally.empty()) ಪ್ರತ್ಯಾಗಮ;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, ಸತ್ಯ);
}

ಶೂನ್ಯ Vlc_ತುದಿ::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    ತಾರ್ಕಿಕ hasHier = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ hasCollapsedHier = ಅಸತ್ಯ;
    ಪ್ರತಿ (VlcPoints::ByName::ಮೌಲ್ಯ_type& i : m_points) {
        ಧ್ರುವ VlcPoint& pt = m_points.point_ಸಂಖ್ಯೆ(i.second);
        ಯದಿ (isCovergroup(pt)) ಅನುವರ್ತ;  // Reported below, by covergroup
        ಧ್ರುವ string hier = reportHier(pt);
        ಯದಿ (hier.empty()) ಅನುವರ್ತ;
        hasHier = ಸತ್ಯ;
        ಯದಿ (isCollapsedHier(hier)) hasCollapsedHier = ಸತ್ಯ;
        ಧ್ರುವ string type = displayType(pt);
        ಧ್ರುವ std::vector<string> parts = splitHier(hier);
        string path;
        ಪ್ರತಿ (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.ಎಣಿಕೆ());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.ಎಣಿಕೆ());
    }
    ಧ್ರುವ std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    ಯದಿ (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಯದಿ (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ levels = opt.reportLevels();
    ಯದಿ (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    ಪ್ರತಿ (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        ಧ್ರುವ std::vector<string> parts = splitHier(it->first);
        ಯದಿ (levels >= 0 && static_cast<ಪೂರ್ಣಾಙ್ಕ>(parts.size()) > levels + 1) ಅನುವರ್ತ;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, ಅಸತ್ಯ);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    ಪ್ರತಿ (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, ಅಸತ್ಯ);
    }
    ಯದಿ (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
