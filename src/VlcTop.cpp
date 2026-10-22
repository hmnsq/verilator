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

#ଅନ୍ତର୍ଭାଵ "VlcTop.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"

#ଅନ୍ତର୍ଭାଵ "VlcOptions.h"

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <fstream>
#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <sstream>
#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <vector>

//######################################################################

ନାମାକାଶ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
ଶ୍ରେଣୀ Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    ତାର୍କିକ m_scored = ମିଥ୍ୟା;  // Coverage is m_score, not the ratio of the points
    ଦ୍ୱିଗୁଣ m_score = 0.0;  // Percent coverage, if m_scored

ସର୍ବସାଧାରଣ:
    // ACCESSORS
    uint64_t hit() ଅଚଳ { ଫେରନ୍ତୁ m_hit; }
    uint64_t total() ଅଚଳ { ଫେରନ୍ତୁ m_total; }
    ତାର୍କିକ scored() ଅଚଳ { ଫେରନ୍ତୁ m_scored; }
    ଦ୍ୱିଗୁଣ score() ଅଚଳ { ଫେରନ୍ତୁ m_score; }
    ଶୂନ୍ୟ score(ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ) {
        m_scored = ସତ୍ୟ;
        m_score = ମୂଲ୍ୟ;
    }
    // METHODS
    ଶୂନ୍ୟ addPoint(ତାର୍କିକ covered) {
        ଯଦି (covered) ++m_hit;
        ++m_total;
    }
    ଶୂନ୍ୟ addPoints(ଅଚଳ Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ s_covergroupType = "covergroup";
ସ୍ଥିର constexpr size_t s_summaryIndent = 2;
ସ୍ଥିର constexpr size_t s_reportRowIndent = 4;

string displayType(ଅଚଳ VlcPoint& point) {
    ଅଚଳ string type = point.type();
    ଫେରନ୍ତୁ type.empty() ? "point" : type;
}

ତାର୍କିକ isCollapsedHier(ଅଚଳ string& hier) {
    ଫେରନ୍ତୁ hier.find('*') != string::npos || hier.find('?') != string::npos;
}

ତାର୍କିକ isOrderedType(ଅଚଳ string& type) {
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* ଅଚଳ typep : s_orderedTypes) {
        ଯଦି (type == typep) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

string reportHier(ଅଚଳ VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    ଯଦି (point.isFsmState() || point.isFsmArc()) {
        ଅଚଳ string fsmVar = point.fsmVarName();
        ଫେରନ୍ତୁ fsmVar.substr(0, fsmVar.rfind('.'));
    }
    ଫେରନ୍ତୁ point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(ଅଚଳ string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    ଯେତେବେଳେ (pos < hier.size()) {
        ଅଚଳ ଅକ୍ଷର c = hier[pos];
        ଯଦି (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            ଜାରି;
        } ଅନ୍ୟଥା ଯଦି (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } ଅନ୍ୟଥା ଯଦି (!depth && c == '.') {
            parts.ଠେଲ_back(hier.substr(start, pos - start));
            start = pos + 1;
        } ଅନ୍ୟଥା ଯଦି (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            ଜାରି;
        } ଅନ୍ୟଥା ଯଦି (depth && c == '(') {
            ++depth;
        } ଅନ୍ୟଥା ଯଦି (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    ଯଦି (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        ପାଇଁ (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.ଠେଲ_back(hier.substr(start, dot - start));
        }
    }
    parts.ଠେଲ_back(hier.substr(start));
    ଫେରନ୍ତୁ parts;
}

string duName(ଅଚଳ VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ଅଚଳ string page = point.page();
    ଫେରନ୍ତୁ page.substr(page.find('/') + 1);
}

ଶୂନ୍ୟ tallyPoint(TypeTally& tally, ଅଚଳ string& type, uint64_t ଗଣନା_2) {
    tally[type].addPoint(ଗଣନା_2 > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
ଦ୍ୱିଗୁଣ pct(uint64_t hit, uint64_t total) {
    ଫେରନ୍ତୁ total ? (100.0 * static_cast<ଦ୍ୱିଗୁଣ>(hit) / static_cast<ଦ୍ୱିଗୁଣ>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_ମୂଲ୍ୟ_String(ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ, ତାର୍କିକ complete) {
    ଯଦି (!complete && ମୂଲ୍ୟ > 99.9) ମୂଲ୍ୟ = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << ମୂଲ୍ୟ << "%";
    ଫେରନ୍ତୁ os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    ଫେରନ୍ତୁ pct_ମୂଲ୍ୟ_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
ଶ୍ରେଣୀ CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_ଗଣନା = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

ସର୍ବସାଧାରଣ:
    // ACCESSORS
    ଅଚଳ string& name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
    ତାର୍କିକ covered() ଅଚଳ { ଫେରନ୍ତୁ m_ଗଣନା >= m_atLeast; }
    // METHODS
    ଶୂନ୍ୟ addRecord(ଅଚଳ string& name, uint64_t ଗଣନା_2, uint64_t atLeast) {
        m_name = name;
        m_ଗଣନା += ଗଣନା_2;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
ଶ୍ରେଣୀ CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

ସର୍ବସାଧାରଣ:
    // ACCESSORS
    uint64_t weight() ଅଚଳ { ଫେରନ୍ତୁ m_weight; }
    ଅଚଳ std::map<string, CgBin>& bins() ଅଚଳ { ଫେରନ୍ତୁ m_bins; }
    // METHODS
    ଶୂନ୍ୟ addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ଅଚଳ string& identity) { ଫେରନ୍ତୁ m_bins[identity]; }
};

// A covergroup
ଶ୍ରେଣୀ CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_ବସ୍ତୁଗୁଡ଼ିକ;  // By hierarchy

ସର୍ବସାଧାରଣ:
    // ACCESSORS
    ଅଚଳ string& hier() ଅଚଳ { ଫେରନ୍ତୁ m_hier; }
    uint64_t weight() ଅଚଳ { ଫେରନ୍ତୁ m_weight; }
    ଅଚଳ std::map<string, CgItem>& ବସ୍ତୁଗୁଡ଼ିକ() ଅଚଳ { ଫେରନ୍ତୁ m_ବସ୍ତୁଗୁଡ଼ିକ; }
    // METHODS
    ଶୂନ୍ୟ addRecord(ଅଚଳ string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ଅଚଳ string& hier) { ଫେରନ୍ତୁ m_ବସ୍ତୁଗୁଡ଼ିକ[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t ଚାବି_ସଂଖ୍ୟା(ଅଚଳ string& ମୂଲ୍ୟ) {
    ଫେରନ୍ତୁ ମୂଲ୍ୟ.empty() ? 1 : std::strtoull(ମୂଲ୍ୟ.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
ତାର୍କିକ isScoreField(ଅଚଳ string& field) {
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* ଅଚଳ keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        ଅଚଳ string prefix = string{"\001"} + keyp + "\002";
        ଯଦି (field.compare(0, prefix.size(), prefix) == 0) ଫେରନ୍ତୁ ସତ୍ୟ;
    }
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(ଅଚଳ string& recordName) {
    string identity;
    string::size_type start = 0;
    ଯେତେବେଳେ (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        ଯଦି (end == string::npos) end = recordName.size();
        ଅଚଳ string field = recordName.substr(start, end - start);
        ଯଦି (!isScoreField(field)) identity += field;
        start = end;
    }
    ଫେରନ୍ତୁ identity;
}

ତାର୍କିକ isCovergroup(ଅଚଳ VlcPoint& point) { ଫେରନ୍ତୁ point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    ପାଇଁ (ଅଚଳ VlcPoints::ByName::ମୂଲ୍ୟ_type& i : points) {
        ଅଚଳ VlcPoint& pt = points.point_ସଂଖ୍ୟା(i.second);
        ଯଦି (!isCovergroup(pt)) ଜାରି;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ଅଚଳ string hier = pt.hier();
        string bin = pt.bin();
        ଯଦି (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        ଅଚଳ string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), ଚାବି_ସଂଖ୍ୟା(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(ଚାବି_ସଂଖ୍ୟା(pt.weight()));
        ଯଦି (!pt.binType().empty()) ଜାରି;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.ଗଣନା_2(), ଚାବି_ସଂଖ୍ୟା(pt.thresh()));
    }
    ଫେରନ୍ତୁ groups;
}

Tally itemTally(ଅଚଳ CgItem& item) {
    Tally tally;
    ପାଇଁ (ଅଚଳ std::pair<ଅଚଳ string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    ଫେରନ୍ତୁ tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(ଅଚଳ CgGroup& group, ତାର୍କିକ* contributesp = nullptr) {
    Tally tally;
    ଦ୍ୱିଗୁଣ weighted = 0.0;
    ଦ୍ୱିଗୁଣ weights = 0.0;
    ପାଇଁ (ଅଚଳ std::pair<ଅଚଳ string, CgItem>& it : group.ବସ୍ତୁଗୁଡ଼ିକ()) {
        ଅଚଳ Tally item = itemTally(it.second);
        tally.addPoints(item);
        ଯଦି (!item.total()) ଜାରି;  // Excluded from both sums
        weighted += static_cast<ଦ୍ୱିଗୁଣ>(it.second.weight()) * item.score();
        weights += static_cast<ଦ୍ୱିଗୁଣ>(it.second.weight());
    }
    ଯଦି (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    ଫେରନ୍ତୁ tally;
}

Tally groupsTally(ଅଚଳ std::vector<ଅଚଳ CgGroup*>& groups) {
    Tally tally;
    ଦ୍ୱିଗୁଣ weighted = 0.0;
    ଦ୍ୱିଗୁଣ weights = 0.0;
    ତାର୍କିକ anyWeight = ମିଥ୍ୟା;
    ପାଇଁ (ଅଚଳ CgGroup* ଅଚଳ groupp : groups) {
        ତାର୍କିକ contributes = ମିଥ୍ୟା;
        ଅଚଳ Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        ଯଦି (groupp->weight()) anyWeight = ସତ୍ୟ;
        ଯଦି (!contributes) ଜାରି;
        weighted += static_cast<ଦ୍ୱିଗୁଣ>(groupp->weight()) * group.score();
        weights += static_cast<ଦ୍ୱିଗୁଣ>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    ଫେରନ୍ତୁ tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ଅଚଳ CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<ଅଚଳ CgGroup*>> ଗଣ୍ଠି_Groups;
    ପାଇଁ (ଅଚଳ CgGroups::ମୂଲ୍ୟ_type& it : groups) {
        ଅଚଳ CgGroup& group = it.second;
        string path;
        ପାଇଁ (ଅଚଳ string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            ଗଣ୍ଠି_Groups[path].ଠେଲ_back(&group);
        }
        ପାଇଁ (ଅଚଳ std::pair<ଅଚଳ string, CgItem>& item : group.ବସ୍ତୁଗୁଡ଼ିକ()) {
            tallies[item.first] = itemTally(item.second);
            ପାଇଁ (ଅଚଳ std::pair<ଅଚଳ string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    ପାଇଁ (ଅଚଳ std::pair<ଅଚଳ string, std::vector<ଅଚଳ CgGroup*>>& it : ଗଣ୍ଠି_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    ଫେରନ୍ତୁ tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
ଶୂନ୍ୟ printIndent(size_t indent) {
    ପାଇଁ (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

ଶୂନ୍ୟ printTallyRow(ଅଚଳ string& type, ଅଚଳ Tally& tally, size_t indent, size_t typeWidth,
                   size_t ଗଣନା_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ଅଚଳ string percent = tally.scored() ? pct_ମୂଲ୍ୟ_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::ବାମ << std::setw(typeWidth) << type << " : " << std::ଡାହାଣ << std::fixed
              << std::setw(6) << percent << " (" << std::setw(ଗଣନା_Width) << tally.hit() << "/"
              << std::setw(ଗଣନା_Width) << tally.total() << ")\n";
}

size_t ଗଣନା_Width(ଅଚଳ TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    ପାଇଁ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    ଫେରନ୍ତୁ width;
}

size_t typeWidth(ଅଚଳ TypeTally& tally) {
    size_t typeWidth = 0;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* ଅଚଳ typep : s_orderedTypes) {
        ଅଚଳ string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    ପାଇଁ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    ଫେରନ୍ତୁ typeWidth;
}

ଶୂନ୍ୟ printTypeTally(ଅଚଳ TypeTally& tally, size_t indent, ତାର୍କିକ includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ଅଚଳ size_t typWidth = typeWidth(tally);
    ଅଚଳ size_t cntWidth = ଗଣନା_Width(tally);
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* ଅଚଳ typep : s_orderedTypes) {
        ଅଚଳ string type = typep;
        ଅଚଳ TypeTally::const_iterator it = tally.find(type);
        ଯଦି (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } ଅନ୍ୟଥା ଯଦି (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    ପାଇଁ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        ଯଦି (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
ଶୂନ୍ୟ printCovergroupTallies(ଅଚଳ std::map<string, Tally>& tallies, ପୂର୍ଣ୍ଣସଂଖ୍ୟା levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    ପାଇଁ (ଅଚଳ std::pair<ଅଚଳ string, Tally>& it : tallies) {
        ଯଦି (levels >= 0 && static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(splitHier(it.first).size()) > levels + 1) ଜାରି;
        shown.ଭର୍ତ୍ତି(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    ଅଚଳ size_t cntWidth = ଗଣନା_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    ପାଇଁ (ଅଚଳ std::pair<ଅଚଳ string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::readCoverage(ଅଚଳ string& filename, ତାର୍କିକ nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    ଯଦି (!is) {
        ଯଦି (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        ଫେରନ୍ତୁ;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ଅଚଳ testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    ଯେତେବେଳେ (!is.eof()) {
        ଅଚଳ string ଧାଡ଼ି = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        ଯଦି (ଧାଡ଼ି[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ଅଚଳ string::size_type secspace = ଧାଡ଼ି.rfind("' ");
            ଯଦି (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                ଜାରି;
            }
            ଅଚଳ string point = ଧାଡ଼ି.substr(3, secspace - 3);
            ଯଦି (!opt.isTypeMatch(point.c_str())) ଜାରି;

            ଅଚଳ uint64_t hits = std::atoll(ଧାଡ଼ି.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ଅଚଳ uint64_t pointnum = points().findAddPoint(point, hits);
            ଯଦି (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ଅଚଳ uint64_t needed = std::max(
                    VlcBuckets::sufficient(), ଚାବି_ସଂଖ୍ୟା(points().point_ସଂଖ୍ୟା(pointnum).thresh()));
                ଯଦି (hits >= needed) {
                    points().point_ସଂଖ୍ୟା(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::writeCoverage(ଅଚଳ string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    ଯଦି (!os) {
        v3fatal("Can't write file: " << filename);
        ଫେରନ୍ତୁ;
    }

    os << "# SystemC::Coverage-3\n";
    ପାଇଁ (ଅଚଳ auto& i : m_points) {
        ଅଚଳ VlcPoint& point = m_points.point_ସଂଖ୍ୟା(i.second);
        os << "C '" << point.name() << "' " << point.ଗଣନା_2() << '\n';
    }
}

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::writeInfo(ଅଚଳ string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    ଯଦି (!os) {
        v3fatal("Can't write file: " << filename);
        ଫେରନ୍ତୁ;
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
    ପାଇଁ (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& ଧାଡ଼ିଗୁଡ଼ିକ = source.ଧାଡ଼ିଗୁଡ଼ିକ();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା branchesFound = 0;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା branchesHit = 0;
        ପାଇଁ (auto& li : ଧାଡ଼ିଗୁଡ଼ିକ) {
            Vlc_Source_ଗଣନା& sc = li.second;
            uint64_t da_ଗଣନା = 0;
            std::vector<ଅଚଳ VlcPoint*> infoPoints;
            ପାଇଁ (ଅଚଳ auto& point : sc.points()) {
                da_ଗଣନା = std::max(da_ଗଣନା, point->ଗଣନା_2());
                infoPoints.ଠେଲ_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_ଗଣନା << "\n";
            ଯଦି (infoPoints.size() <= 1) ଜାରି;
            branchesFound += static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(infoPoints.size());
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା point_ଅଙ୍କ_2 = 0;
            ପାଇଁ (ଅଚଳ VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                ଯଦି (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } ଅନ୍ୟଥା ଯଦି (point->comment().empty()) {
                    os << "0,";
                    os << point_ଅଙ୍କ_2;
                } ଅନ୍ୟଥା {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->ଗଣନା_2() << "\n";

                branchesHit += opt.ଗଣନା_Ok(point->ଗଣନା_2());
                ++point_ଅଙ୍କ_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

ସଂରଚନା CmpComputrons final {
    ତାର୍କିକ operator()(ଅଚଳ VlcTest* lhsp, ଅଚଳ VlcTest* rhsp) ଅଚଳ {
        ଯଦି (lhsp->computrons() != rhsp->computrons()) {
            ଫେରନ୍ତୁ lhsp->computrons() < rhsp->computrons();
        }
        ଫେରନ୍ତୁ lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    ପାଇଁ (ଅଚଳ auto& testp : m_tests) {
        ଯଦି (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ଠେଲ_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    ପାଇଁ (ଅଚଳ auto& i : m_points) {
        ଅଚଳ VlcPoint* ଅଚଳ pointp = &points().point_ସଂଖ୍ୟା(i.second);
        // If any tests hit this point, then we'll need to cover it.
        ଯଦି (pointp->testsCovering()) remaining.addData(pointp->point_ଅଙ୍କ(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    ଯେତେବେଳେ (ସତ୍ୟ) {
        ଯଦି (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        ପାଇଁ (ଅଚଳ auto& testp : bytime) {
            ଯଦି (!testp->rank()) {
                ଅଚଳ uint64_t remain = testp->buckets().data_କାଢ଼_ଗଣନା(remaining);
                ଯଦି (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        ଯଦି (VlcTest* ଅଚଳ testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } ଅନ୍ୟଥା {
            ଭାଙ୍ଗ;  // No test covering more stuff found
        }
    }
}

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::annotateCalc() {
    // Calculate per-line information into filedata structure
    ପାଇଁ (ଅଚଳ auto& i : m_points) {
        ଅଚଳ VlcPoint& point = m_points.point_ସଂଖ୍ୟା(i.second);
        ଅଚଳ string filename = point.filename();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno = point.lineno();
        ଯଦି (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.ଗଣନା_2() << " " << point.linescov());
            // Base coverage
            source.ଭର୍ତ୍ତି_Point(lineno, &point);
            // Additional lines covered by this statement
            ତାର୍କିକ range = ମିଥ୍ୟା;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା start = 0;
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା end = 0;
            ଅଚଳ string linescov = point.linescov();
            ପାଇଁ (ଅଚଳ ଅକ୍ଷର* covp = linescov.c_str(); ସତ୍ୟ; ++covp) {
                ଯଦି (!*covp || *covp == ',') {  // Ending
                    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା lni = start; start && lni <= end; ++lni) {
                        source.ଭର୍ତ୍ତି_Point(lni, &point);
                    }
                    ଯଦି (!*covp) ଭାଙ୍ଗ;
                    start = 0;  // Prep for next
                    end = 0;
                    range = ମିଥ୍ୟା;
                } ଅନ୍ୟଥା ଯଦି (*covp == '-') {
                    range = ସତ୍ୟ;
                } ଅନ୍ୟଥା ଯଦି (std::isdigit(*covp)) {
                    ଅଚଳ ଅକ୍ଷର* ଅଚଳ digitsp = covp;
                    ଯେତେବେଳେ (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    ଯଦି (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା totCases = 0;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା totOk = 0;
    ପାଇଁ (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        ଯଦି (opt.annotateAll()) source.needed(ସତ୍ୟ);
        ଅଚଳ VlcSource::LinenoMap& ଧାଡ଼ିଗୁଡ଼ିକ = source.ଧାଡ଼ିଗୁଡ଼ିକ();
        ପାଇଁ (auto& li : ଧାଡ଼ିଗୁଡ଼ିକ) {
            ଅଚଳ Vlc_Source_ଗଣନା& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            ଯଦି (opt.ଗଣନା_Ok(sc.min_ଗଣନା())) {
                ++totOk;
            } ଅନ୍ୟଥା {
                source.needed(ସତ୍ୟ);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    ଯଦି (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::annotateOutputFiles(ଅଚଳ string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    ପାଇଁ (auto& si : m_sources) {
        VlcSource& source = si.second;
        ଯଦି (!source.needed()) ଜାରି;
        ଅଚଳ string filename = source.name();
        ଅଚଳ string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        ଯଦି (!is) {
            v3error("Can't read annotation file: " << filename);
            ଫେରନ୍ତୁ;
        }

        std::ofstream os{outfilename.c_str()};
        ଯଦି (!os) {
            v3error("Can't write file: " << outfilename);
            ଫେରନ୍ତୁ;
        }

        os << "//      // verilator_coverage annotation\n";

        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno = 0;
        ଯେତେବେଳେ (!is.eof()) {
            lineno++;
            ଅଚଳ std::string ଧାଡ଼ି = V3Os::getline(is);

            VlcSource::LinenoMap& ଧାଡ଼ିଗୁଡ଼ିକ = source.ଧାଡ଼ିଗୁଡ଼ିକ();
            ଅଚଳ auto lit = ଧାଡ଼ିଗୁଡ଼ିକ.find(lineno);
            ଯଦି (lit == ଧାଡ଼ିଗୁଡ଼ିକ.end()) {
                os << "        " << ଧାଡ଼ି << '\n';
            } ଅନ୍ୟଥା {
                Vlc_Source_ଗଣନା& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ଅଚଳ ତାର୍କିକ minOk = opt.ଗଣନା_Ok(sc.min_ଗଣନା());
                ଅଚଳ ତାର୍କିକ maxOk = opt.ଗଣନା_Ok(sc.max_ଗଣନା());
                ଯଦି (minOk) {
                    os << " ";
                } ଅନ୍ୟଥା ଯଦି (maxOk) {
                    os << "~";
                } ଅନ୍ୟଥା {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_ଗଣନା() << " " << ଧାଡ଼ି << '\n';

                ଯଦି (opt.annotatePoints()) {
                    ପାଇଁ (ଅଚଳ auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                ତାର୍କିକ printedFsmHeader = ମିଥ୍ୟା;
                ପାଇଁ (ଅଚଳ auto& pit : sc.points()) {
                    ଯଦି (!pit->isFsmState() && !pit->isFsmArc()) ଜାରି;
                    ଯଦି (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = ସତ୍ୟ;
                    }
                    os << (opt.ଗଣନା_Ok(pit->ଗଣନା_2()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->ଗଣନା_2() << "        ";
                    ଯଦି (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        ଯଦି (pit->ଗଣନା_2() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } ଅନ୍ୟଥା ଯଦି (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } ଅନ୍ୟଥା {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        ଯଦି (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::annotate(ଅଚଳ string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::printTypeSummary() {
    TypeTally tally;
    ପାଇଁ (VlcPoints::ByName::ମୂଲ୍ୟ_type& i : m_points) {
        ଅଚଳ VlcPoint& pt = m_points.point_ସଂଖ୍ୟା(i.second);
        ଯଦି (isCovergroup(pt)) ଜାରି;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.ଗଣନା_2());
    }
    ଅଚଳ CgGroups groups = covergroups(m_points);
    ଯଦି (!groups.empty()) {
        std::vector<ଅଚଳ CgGroup*> groupps;
        ପାଇଁ (ଅଚଳ CgGroups::ମୂଲ୍ୟ_type& it : groups) groupps.ଠେଲ_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    ଯଦି (tally.empty()) ଫେରନ୍ତୁ;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, ସତ୍ୟ);
}

ଶୂନ୍ୟ Vlc_ଶୀର୍ଷ::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    ତାର୍କିକ hasHier = ମିଥ୍ୟା;
    ତାର୍କିକ hasCollapsedHier = ମିଥ୍ୟା;
    ପାଇଁ (VlcPoints::ByName::ମୂଲ୍ୟ_type& i : m_points) {
        ଅଚଳ VlcPoint& pt = m_points.point_ସଂଖ୍ୟା(i.second);
        ଯଦି (isCovergroup(pt)) ଜାରି;  // Reported below, by covergroup
        ଅଚଳ string hier = reportHier(pt);
        ଯଦି (hier.empty()) ଜାରି;
        hasHier = ସତ୍ୟ;
        ଯଦି (isCollapsedHier(hier)) hasCollapsedHier = ସତ୍ୟ;
        ଅଚଳ string type = displayType(pt);
        ଅଚଳ std::vector<string> parts = splitHier(hier);
        string path;
        ପାଇଁ (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.ଗଣନା_2());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.ଗଣନା_2());
    }
    ଅଚଳ std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    ଯଦି (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        ଫେରନ୍ତୁ;
    }
    ଯଦି (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        ଫେରନ୍ତୁ;
    }

    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା levels = opt.reportLevels();
    ଯଦି (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    ପାଇଁ (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        ଅଚଳ std::vector<string> parts = splitHier(it->first);
        ଯଦି (levels >= 0 && static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(parts.size()) > levels + 1) ଜାରି;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, ମିଥ୍ୟା);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    ପାଇଁ (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, ମିଥ୍ୟା);
    }
    ଯଦି (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
