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

#അന്തര്ഭാവ "VlcTop.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Os.h"

#അന്തര്ഭാവ "VlcOptions.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <fstream>
#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <sstream>
#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <vector>

//######################################################################

നാമാകാശ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
വർഗ്ഗം Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    തർക്കം m_scored = അസത്യം;  // Coverage is m_score, not the ratio of the points
    ഇരട്ട m_score = 0.0;  // Percent coverage, if m_scored

പൊതു:
    // ACCESSORS
    uint64_t hit() സ്ഥിരം { തിരികെ m_hit; }
    uint64_t total() സ്ഥിരം { തിരികെ m_total; }
    തർക്കം scored() സ്ഥിരം { തിരികെ m_scored; }
    ഇരട്ട score() സ്ഥിരം { തിരികെ m_score; }
    ശൂന്യം score(ഇരട്ട മൂല്യം) {
        m_scored = സത്യം;
        m_score = മൂല്യം;
    }
    // METHODS
    ശൂന്യം addPoint(തർക്കം covered) {
        എങ്കിൽ (covered) ++m_hit;
        ++m_total;
    }
    ശൂന്യം addPoints(സ്ഥിരം Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം s_covergroupType = "covergroup";
സ്ഥിതം constexpr size_t s_summaryIndent = 2;
സ്ഥിതം constexpr size_t s_reportRowIndent = 4;

string displayType(സ്ഥിരം VlcPoint& point) {
    സ്ഥിരം string type = point.type();
    തിരികെ type.empty() ? "point" : type;
}

തർക്കം isCollapsedHier(സ്ഥിരം string& hier) {
    തിരികെ hier.find('*') != string::npos || hier.find('?') != string::npos;
}

തർക്കം isOrderedType(സ്ഥിരം string& type) {
    വേണ്ടി (സ്ഥിരം അക്ഷരം* സ്ഥിരം typep : s_orderedTypes) {
        എങ്കിൽ (type == typep) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}

string reportHier(സ്ഥിരം VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    എങ്കിൽ (point.isFsmState() || point.isFsmArc()) {
        സ്ഥിരം string fsmVar = point.fsmVarName();
        തിരികെ fsmVar.substr(0, fsmVar.rfind('.'));
    }
    തിരികെ point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(സ്ഥിരം string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    പൂർണ്ണസംഖ്യ depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    ആകുമ്പോൾ (pos < hier.size()) {
        സ്ഥിരം അക്ഷരം c = hier[pos];
        എങ്കിൽ (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            തുടരുക;
        } അല്ലെങ്കിൽ എങ്കിൽ (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } അല്ലെങ്കിൽ എങ്കിൽ (!depth && c == '.') {
            parts.തള്ളുക_back(hier.substr(start, pos - start));
            start = pos + 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            തുടരുക;
        } അല്ലെങ്കിൽ എങ്കിൽ (depth && c == '(') {
            ++depth;
        } അല്ലെങ്കിൽ എങ്കിൽ (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    എങ്കിൽ (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        വേണ്ടി (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.തള്ളുക_back(hier.substr(start, dot - start));
        }
    }
    parts.തള്ളുക_back(hier.substr(start));
    തിരികെ parts;
}

string duName(സ്ഥിരം VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    സ്ഥിരം string page = point.page();
    തിരികെ page.substr(page.find('/') + 1);
}

ശൂന്യം tallyPoint(TypeTally& tally, സ്ഥിരം string& type, uint64_t എണ്ണം) {
    tally[type].addPoint(എണ്ണം > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
ഇരട്ട pct(uint64_t hit, uint64_t total) {
    തിരികെ total ? (100.0 * static_cast<ഇരട്ട>(hit) / static_cast<ഇരട്ട>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_മൂല്യം_String(ഇരട്ട മൂല്യം, തർക്കം complete) {
    എങ്കിൽ (!complete && മൂല്യം > 99.9) മൂല്യം = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << മൂല്യം << "%";
    തിരികെ os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    തിരികെ pct_മൂല്യം_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
വർഗ്ഗം CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_എണ്ണം = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

പൊതു:
    // ACCESSORS
    സ്ഥിരം string& name() സ്ഥിരം { തിരികെ m_name; }
    തർക്കം covered() സ്ഥിരം { തിരികെ m_എണ്ണം >= m_atLeast; }
    // METHODS
    ശൂന്യം addRecord(സ്ഥിരം string& name, uint64_t എണ്ണം, uint64_t atLeast) {
        m_name = name;
        m_എണ്ണം += എണ്ണം;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
വർഗ്ഗം CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

പൊതു:
    // ACCESSORS
    uint64_t weight() സ്ഥിരം { തിരികെ m_weight; }
    സ്ഥിരം std::map<string, CgBin>& bins() സ്ഥിരം { തിരികെ m_bins; }
    // METHODS
    ശൂന്യം addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(സ്ഥിരം string& identity) { തിരികെ m_bins[identity]; }
};

// A covergroup
വർഗ്ഗം CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_ഇനങ്ങൾ;  // By hierarchy

പൊതു:
    // ACCESSORS
    സ്ഥിരം string& hier() സ്ഥിരം { തിരികെ m_hier; }
    uint64_t weight() സ്ഥിരം { തിരികെ m_weight; }
    സ്ഥിരം std::map<string, CgItem>& ഇനങ്ങൾ() സ്ഥിരം { തിരികെ m_ഇനങ്ങൾ; }
    // METHODS
    ശൂന്യം addRecord(സ്ഥിരം string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(സ്ഥിരം string& hier) { തിരികെ m_ഇനങ്ങൾ[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t താക്കോൽ_സംഖ്യ(സ്ഥിരം string& മൂല്യം) {
    തിരികെ മൂല്യം.empty() ? 1 : std::strtoull(മൂല്യം.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
തർക്കം isScoreField(സ്ഥിരം string& field) {
    വേണ്ടി (സ്ഥിരം അക്ഷരം* സ്ഥിരം keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        സ്ഥിരം string prefix = string{"\001"} + keyp + "\002";
        എങ്കിൽ (field.compare(0, prefix.size(), prefix) == 0) തിരികെ സത്യം;
    }
    തിരികെ അസത്യം;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(സ്ഥിരം string& recordName) {
    string identity;
    string::size_type start = 0;
    ആകുമ്പോൾ (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        എങ്കിൽ (end == string::npos) end = recordName.size();
        സ്ഥിരം string field = recordName.substr(start, end - start);
        എങ്കിൽ (!isScoreField(field)) identity += field;
        start = end;
    }
    തിരികെ identity;
}

തർക്കം isCovergroup(സ്ഥിരം VlcPoint& point) { തിരികെ point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    വേണ്ടി (സ്ഥിരം VlcPoints::ByName::മൂല്യം_type& i : points) {
        സ്ഥിരം VlcPoint& pt = points.point_സംഖ്യ(i.second);
        എങ്കിൽ (!isCovergroup(pt)) തുടരുക;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        സ്ഥിരം string hier = pt.hier();
        string bin = pt.bin();
        എങ്കിൽ (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        സ്ഥിരം string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), താക്കോൽ_സംഖ്യ(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(താക്കോൽ_സംഖ്യ(pt.weight()));
        എങ്കിൽ (!pt.binType().empty()) തുടരുക;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.എണ്ണം(), താക്കോൽ_സംഖ്യ(pt.thresh()));
    }
    തിരികെ groups;
}

Tally itemTally(സ്ഥിരം CgItem& item) {
    Tally tally;
    വേണ്ടി (സ്ഥിരം std::pair<സ്ഥിരം string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    തിരികെ tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(സ്ഥിരം CgGroup& group, തർക്കം* contributesp = nullptr) {
    Tally tally;
    ഇരട്ട weighted = 0.0;
    ഇരട്ട weights = 0.0;
    വേണ്ടി (സ്ഥിരം std::pair<സ്ഥിരം string, CgItem>& it : group.ഇനങ്ങൾ()) {
        സ്ഥിരം Tally item = itemTally(it.second);
        tally.addPoints(item);
        എങ്കിൽ (!item.total()) തുടരുക;  // Excluded from both sums
        weighted += static_cast<ഇരട്ട>(it.second.weight()) * item.score();
        weights += static_cast<ഇരട്ട>(it.second.weight());
    }
    എങ്കിൽ (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    തിരികെ tally;
}

Tally groupsTally(സ്ഥിരം std::vector<സ്ഥിരം CgGroup*>& groups) {
    Tally tally;
    ഇരട്ട weighted = 0.0;
    ഇരട്ട weights = 0.0;
    തർക്കം anyWeight = അസത്യം;
    വേണ്ടി (സ്ഥിരം CgGroup* സ്ഥിരം groupp : groups) {
        തർക്കം contributes = അസത്യം;
        സ്ഥിരം Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        എങ്കിൽ (groupp->weight()) anyWeight = സത്യം;
        എങ്കിൽ (!contributes) തുടരുക;
        weighted += static_cast<ഇരട്ട>(groupp->weight()) * group.score();
        weights += static_cast<ഇരട്ട>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    തിരികെ tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(സ്ഥിരം CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<സ്ഥിരം CgGroup*>> കണ്ണി_Groups;
    വേണ്ടി (സ്ഥിരം CgGroups::മൂല്യം_type& it : groups) {
        സ്ഥിരം CgGroup& group = it.second;
        string path;
        വേണ്ടി (സ്ഥിരം string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            കണ്ണി_Groups[path].തള്ളുക_back(&group);
        }
        വേണ്ടി (സ്ഥിരം std::pair<സ്ഥിരം string, CgItem>& item : group.ഇനങ്ങൾ()) {
            tallies[item.first] = itemTally(item.second);
            വേണ്ടി (സ്ഥിരം std::pair<സ്ഥിരം string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    വേണ്ടി (സ്ഥിരം std::pair<സ്ഥിരം string, std::vector<സ്ഥിരം CgGroup*>>& it : കണ്ണി_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    തിരികെ tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
ശൂന്യം printIndent(size_t indent) {
    വേണ്ടി (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

ശൂന്യം printTallyRow(സ്ഥിരം string& type, സ്ഥിരം Tally& tally, size_t indent, size_t typeWidth,
                   size_t എണ്ണം_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    സ്ഥിരം string percent = tally.scored() ? pct_മൂല്യം_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::ഇടത് << std::setw(typeWidth) << type << " : " << std::വലത് << std::fixed
              << std::setw(6) << percent << " (" << std::setw(എണ്ണം_Width) << tally.hit() << "/"
              << std::setw(എണ്ണം_Width) << tally.total() << ")\n";
}

size_t എണ്ണം_Width(സ്ഥിരം TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    വേണ്ടി (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    തിരികെ width;
}

size_t typeWidth(സ്ഥിരം TypeTally& tally) {
    size_t typeWidth = 0;
    വേണ്ടി (സ്ഥിരം അക്ഷരം* സ്ഥിരം typep : s_orderedTypes) {
        സ്ഥിരം string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    വേണ്ടി (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    തിരികെ typeWidth;
}

ശൂന്യം printTypeTally(സ്ഥിരം TypeTally& tally, size_t indent, തർക്കം includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    സ്ഥിരം size_t typWidth = typeWidth(tally);
    സ്ഥിരം size_t cntWidth = എണ്ണം_Width(tally);
    വേണ്ടി (സ്ഥിരം അക്ഷരം* സ്ഥിരം typep : s_orderedTypes) {
        സ്ഥിരം string type = typep;
        സ്ഥിരം TypeTally::const_iterator it = tally.find(type);
        എങ്കിൽ (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } അല്ലെങ്കിൽ എങ്കിൽ (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    വേണ്ടി (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        എങ്കിൽ (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
ശൂന്യം printCovergroupTallies(സ്ഥിരം std::map<string, Tally>& tallies, പൂർണ്ണസംഖ്യ levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    വേണ്ടി (സ്ഥിരം std::pair<സ്ഥിരം string, Tally>& it : tallies) {
        എങ്കിൽ (levels >= 0 && static_cast<പൂർണ്ണസംഖ്യ>(splitHier(it.first).size()) > levels + 1) തുടരുക;
        shown.തിരുകുക(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    സ്ഥിരം size_t cntWidth = എണ്ണം_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    വേണ്ടി (സ്ഥിരം std::pair<സ്ഥിരം string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

ശൂന്യം Vlc_മുകൾ::readCoverage(സ്ഥിരം string& filename, തർക്കം nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    എങ്കിൽ (!is) {
        എങ്കിൽ (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        തിരികെ;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* സ്ഥിരം testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    ആകുമ്പോൾ (!is.eof()) {
        സ്ഥിരം string വരി = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        എങ്കിൽ (വരി[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            സ്ഥിരം string::size_type secspace = വരി.rfind("' ");
            എങ്കിൽ (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                തുടരുക;
            }
            സ്ഥിരം string point = വരി.substr(3, secspace - 3);
            എങ്കിൽ (!opt.isTypeMatch(point.c_str())) തുടരുക;

            സ്ഥിരം uint64_t hits = std::atoll(വരി.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            സ്ഥിരം uint64_t pointnum = points().findAddPoint(point, hits);
            എങ്കിൽ (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                സ്ഥിരം uint64_t needed = std::max(
                    VlcBuckets::sufficient(), താക്കോൽ_സംഖ്യ(points().point_സംഖ്യ(pointnum).thresh()));
                എങ്കിൽ (hits >= needed) {
                    points().point_സംഖ്യ(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

ശൂന്യം Vlc_മുകൾ::writeCoverage(സ്ഥിരം string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    എങ്കിൽ (!os) {
        v3fatal("Can't write file: " << filename);
        തിരികെ;
    }

    os << "# SystemC::Coverage-3\n";
    വേണ്ടി (സ്ഥിരം auto& i : m_points) {
        സ്ഥിരം VlcPoint& point = m_points.point_സംഖ്യ(i.second);
        os << "C '" << point.name() << "' " << point.എണ്ണം() << '\n';
    }
}

ശൂന്യം Vlc_മുകൾ::writeInfo(സ്ഥിരം string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    എങ്കിൽ (!os) {
        v3fatal("Can't write file: " << filename);
        തിരികെ;
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
    വേണ്ടി (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& വരികൾ = source.വരികൾ();
        പൂർണ്ണസംഖ്യ branchesFound = 0;
        പൂർണ്ണസംഖ്യ branchesHit = 0;
        വേണ്ടി (auto& li : വരികൾ) {
            Vlc_Source_എണ്ണം& sc = li.second;
            uint64_t da_എണ്ണം = 0;
            std::vector<സ്ഥിരം VlcPoint*> infoPoints;
            വേണ്ടി (സ്ഥിരം auto& point : sc.points()) {
                da_എണ്ണം = std::max(da_എണ്ണം, point->എണ്ണം());
                infoPoints.തള്ളുക_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_എണ്ണം << "\n";
            എങ്കിൽ (infoPoints.size() <= 1) തുടരുക;
            branchesFound += static_cast<പൂർണ്ണസംഖ്യ>(infoPoints.size());
            പൂർണ്ണസംഖ്യ point_അക്കം_2 = 0;
            വേണ്ടി (സ്ഥിരം VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                എങ്കിൽ (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } അല്ലെങ്കിൽ എങ്കിൽ (point->comment().empty()) {
                    os << "0,";
                    os << point_അക്കം_2;
                } അല്ലെങ്കിൽ {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->എണ്ണം() << "\n";

                branchesHit += opt.എണ്ണം_Ok(point->എണ്ണം());
                ++point_അക്കം_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

ഘടന CmpComputrons final {
    തർക്കം operator()(സ്ഥിരം VlcTest* lhsp, സ്ഥിരം VlcTest* rhsp) സ്ഥിരം {
        എങ്കിൽ (lhsp->computrons() != rhsp->computrons()) {
            തിരികെ lhsp->computrons() < rhsp->computrons();
        }
        തിരികെ lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

ശൂന്യം Vlc_മുകൾ::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    വേണ്ടി (സ്ഥിരം auto& testp : m_tests) {
        എങ്കിൽ (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.തള്ളുക_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    വേണ്ടി (സ്ഥിരം auto& i : m_points) {
        സ്ഥിരം VlcPoint* സ്ഥിരം pointp = &points().point_സംഖ്യ(i.second);
        // If any tests hit this point, then we'll need to cover it.
        എങ്കിൽ (pointp->testsCovering()) remaining.addData(pointp->point_അക്കം(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    ആകുമ്പോൾ (സത്യം) {
        എങ്കിൽ (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        വേണ്ടി (സ്ഥിരം auto& testp : bytime) {
            എങ്കിൽ (!testp->rank()) {
                സ്ഥിരം uint64_t remain = testp->buckets().data_എടുക്കുക_എണ്ണം(remaining);
                എങ്കിൽ (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        എങ്കിൽ (VlcTest* സ്ഥിരം testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } അല്ലെങ്കിൽ {
            നിർത്തുക;  // No test covering more stuff found
        }
    }
}

ശൂന്യം Vlc_മുകൾ::annotateCalc() {
    // Calculate per-line information into filedata structure
    വേണ്ടി (സ്ഥിരം auto& i : m_points) {
        സ്ഥിരം VlcPoint& point = m_points.point_സംഖ്യ(i.second);
        സ്ഥിരം string filename = point.filename();
        സ്ഥിരം പൂർണ്ണസംഖ്യ lineno = point.lineno();
        എങ്കിൽ (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.എണ്ണം() << " " << point.linescov());
            // Base coverage
            source.തിരുകുക_Point(lineno, &point);
            // Additional lines covered by this statement
            തർക്കം range = അസത്യം;
            പൂർണ്ണസംഖ്യ start = 0;
            പൂർണ്ണസംഖ്യ end = 0;
            സ്ഥിരം string linescov = point.linescov();
            വേണ്ടി (സ്ഥിരം അക്ഷരം* covp = linescov.c_str(); സത്യം; ++covp) {
                എങ്കിൽ (!*covp || *covp == ',') {  // Ending
                    വേണ്ടി (പൂർണ്ണസംഖ്യ lni = start; start && lni <= end; ++lni) {
                        source.തിരുകുക_Point(lni, &point);
                    }
                    എങ്കിൽ (!*covp) നിർത്തുക;
                    start = 0;  // Prep for next
                    end = 0;
                    range = അസത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (*covp == '-') {
                    range = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (std::isdigit(*covp)) {
                    സ്ഥിരം അക്ഷരം* സ്ഥിരം digitsp = covp;
                    ആകുമ്പോൾ (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    എങ്കിൽ (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

ശൂന്യം Vlc_മുകൾ::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    പൂർണ്ണസംഖ്യ totCases = 0;
    പൂർണ്ണസംഖ്യ totOk = 0;
    വേണ്ടി (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        എങ്കിൽ (opt.annotateAll()) source.needed(സത്യം);
        സ്ഥിരം VlcSource::LinenoMap& വരികൾ = source.വരികൾ();
        വേണ്ടി (auto& li : വരികൾ) {
            സ്ഥിരം Vlc_Source_എണ്ണം& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            എങ്കിൽ (opt.എണ്ണം_Ok(sc.min_എണ്ണം())) {
                ++totOk;
            } അല്ലെങ്കിൽ {
                source.needed(സത്യം);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    എങ്കിൽ (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

ശൂന്യം Vlc_മുകൾ::annotateOutputFiles(സ്ഥിരം string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    വേണ്ടി (auto& si : m_sources) {
        VlcSource& source = si.second;
        എങ്കിൽ (!source.needed()) തുടരുക;
        സ്ഥിരം string filename = source.name();
        സ്ഥിരം string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        എങ്കിൽ (!is) {
            v3error("Can't read annotation file: " << filename);
            തിരികെ;
        }

        std::ofstream os{outfilename.c_str()};
        എങ്കിൽ (!os) {
            v3error("Can't write file: " << outfilename);
            തിരികെ;
        }

        os << "//      // verilator_coverage annotation\n";

        പൂർണ്ണസംഖ്യ lineno = 0;
        ആകുമ്പോൾ (!is.eof()) {
            lineno++;
            സ്ഥിരം std::string വരി = V3Os::getline(is);

            VlcSource::LinenoMap& വരികൾ = source.വരികൾ();
            സ്ഥിരം auto lit = വരികൾ.find(lineno);
            എങ്കിൽ (lit == വരികൾ.end()) {
                os << "        " << വരി << '\n';
            } അല്ലെങ്കിൽ {
                Vlc_Source_എണ്ണം& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                സ്ഥിരം തർക്കം minOk = opt.എണ്ണം_Ok(sc.min_എണ്ണം());
                സ്ഥിരം തർക്കം maxOk = opt.എണ്ണം_Ok(sc.max_എണ്ണം());
                എങ്കിൽ (minOk) {
                    os << " ";
                } അല്ലെങ്കിൽ എങ്കിൽ (maxOk) {
                    os << "~";
                } അല്ലെങ്കിൽ {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_എണ്ണം() << " " << വരി << '\n';

                എങ്കിൽ (opt.annotatePoints()) {
                    വേണ്ടി (സ്ഥിരം auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                തർക്കം printedFsmHeader = അസത്യം;
                വേണ്ടി (സ്ഥിരം auto& pit : sc.points()) {
                    എങ്കിൽ (!pit->isFsmState() && !pit->isFsmArc()) തുടരുക;
                    എങ്കിൽ (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = സത്യം;
                    }
                    os << (opt.എണ്ണം_Ok(pit->എണ്ണം()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->എണ്ണം() << "        ";
                    എങ്കിൽ (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        എങ്കിൽ (pit->എണ്ണം() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } അല്ലെങ്കിൽ എങ്കിൽ (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } അല്ലെങ്കിൽ {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        എങ്കിൽ (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

ശൂന്യം Vlc_മുകൾ::annotate(സ്ഥിരം string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

ശൂന്യം Vlc_മുകൾ::printTypeSummary() {
    TypeTally tally;
    വേണ്ടി (VlcPoints::ByName::മൂല്യം_type& i : m_points) {
        സ്ഥിരം VlcPoint& pt = m_points.point_സംഖ്യ(i.second);
        എങ്കിൽ (isCovergroup(pt)) തുടരുക;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.എണ്ണം());
    }
    സ്ഥിരം CgGroups groups = covergroups(m_points);
    എങ്കിൽ (!groups.empty()) {
        std::vector<സ്ഥിരം CgGroup*> groupps;
        വേണ്ടി (സ്ഥിരം CgGroups::മൂല്യം_type& it : groups) groupps.തള്ളുക_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    എങ്കിൽ (tally.empty()) തിരികെ;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, സത്യം);
}

ശൂന്യം Vlc_മുകൾ::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    തർക്കം hasHier = അസത്യം;
    തർക്കം hasCollapsedHier = അസത്യം;
    വേണ്ടി (VlcPoints::ByName::മൂല്യം_type& i : m_points) {
        സ്ഥിരം VlcPoint& pt = m_points.point_സംഖ്യ(i.second);
        എങ്കിൽ (isCovergroup(pt)) തുടരുക;  // Reported below, by covergroup
        സ്ഥിരം string hier = reportHier(pt);
        എങ്കിൽ (hier.empty()) തുടരുക;
        hasHier = സത്യം;
        എങ്കിൽ (isCollapsedHier(hier)) hasCollapsedHier = സത്യം;
        സ്ഥിരം string type = displayType(pt);
        സ്ഥിരം std::vector<string> parts = splitHier(hier);
        string path;
        വേണ്ടി (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.എണ്ണം());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.എണ്ണം());
    }
    സ്ഥിരം std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    എങ്കിൽ (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        തിരികെ;
    }
    എങ്കിൽ (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        തിരികെ;
    }

    സ്ഥിരം പൂർണ്ണസംഖ്യ levels = opt.reportLevels();
    എങ്കിൽ (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    വേണ്ടി (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        സ്ഥിരം std::vector<string> parts = splitHier(it->first);
        എങ്കിൽ (levels >= 0 && static_cast<പൂർണ്ണസംഖ്യ>(parts.size()) > levels + 1) തുടരുക;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, അസത്യം);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    വേണ്ടി (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, അസത്യം);
    }
    എങ്കിൽ (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
