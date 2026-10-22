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

#include "VlcTop.h"

#include "V3Error.h"
#include "V3Os.h"

#include "VlcOptions.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

//######################################################################

namespace {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
class Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    bool m_scored = false;  // Coverage is m_score, not the ratio of the points
    dviguu_na m_score = 0.0;  // Percent coverage, if m_scored

public:
    // ACCESSORS
    uint64_t hit() const { ꦮꦭꦶ m_hit; }
    uint64_t total() const { ꦮꦭꦶ m_total; }
    bool scored() const { ꦮꦭꦶ m_scored; }
    dviguu_na score() const { ꦮꦭꦶ m_score; }
    void score(dviguu_na value) {
        m_scored = true;
        m_score = value;
    }
    // METHODS
    void addPoint(bool covered) {
        ꦪꦼꦤ꧀ (covered) ++m_hit;
        ++m_total;
    }
    void addPoints(const Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

static constexpr const var_na* const s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
static constexpr const var_na* const s_covergroupType = "covergroup";
static constexpr size_t s_summaryIndent = 2;
static constexpr size_t s_reportRowIndent = 4;

string displayType(const VlcPoint& point) {
    const string type = point.type();
    ꦮꦭꦶ type.empty() ? "point" : type;
}

bool isCollapsedHier(const string& hier) {
    ꦮꦭꦶ hier.find('*') != string::npos || hier.find('?') != string::npos;
}

bool isOrderedType(const string& type) {
    ꦏꦁꦒꦺꦴ (const var_na* const typep : s_orderedTypes) {
        ꦪꦼꦤ꧀ (type == typep) ꦮꦭꦶ true;
    }
    ꦮꦭꦶ false;
}

string reportHier(const VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    ꦪꦼꦤ꧀ (point.isFsmState() || point.isFsmArc()) {
        const string fsmVar = point.fsmVarName();
        ꦮꦭꦶ fsmVar.substr(0, fsmVar.rfind('.'));
    }
    ꦮꦭꦶ point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(const string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    puur_naa_nka depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    ꦱꦼꦢꦼꦭ (pos < hier.size()) {
        const var_na c = hier[pos];
        ꦪꦼꦤ꧀ (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            continue;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (!depth && c == '.') {
            parts.push_back(hier.substr(start, pos - start));
            start = pos + 1;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            continue;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (depth && c == '(') {
            ++depth;
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    ꦪꦼꦤ꧀ (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        ꦏꦁꦒꦺꦴ (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.push_back(hier.substr(start, dot - start));
        }
    }
    parts.push_back(hier.substr(start));
    ꦮꦭꦶ parts;
}

string duName(const VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    const string page = point.page();
    ꦮꦭꦶ page.substr(page.find('/') + 1);
}

void tallyPoint(TypeTally& tally, const string& type, uint64_t count) {
    tally[type].addPoint(count > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
dviguu_na pct(uint64_t hit, uint64_t total) {
    ꦮꦭꦶ total ? (100.0 * static_cast<dviguu_na>(hit) / static_cast<dviguu_na>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pctValueString(dviguu_na value, bool complete) {
    ꦪꦼꦤ꧀ (!complete && value > 99.9) value = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << value << "%";
    ꦮꦭꦶ os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    ꦮꦭꦶ pctValueString(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
class CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_count = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

public:
    // ACCESSORS
    const string& name() const { ꦮꦭꦶ m_name; }
    bool covered() const { ꦮꦭꦶ m_count >= m_atLeast; }
    // METHODS
    void addRecord(const string& name, uint64_t count, uint64_t atLeast) {
        m_name = name;
        m_count += count;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
class CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

public:
    // ACCESSORS
    uint64_t weight() const { ꦮꦭꦶ m_weight; }
    const std::map<string, CgBin>& bins() const { ꦮꦭꦶ m_bins; }
    // METHODS
    void addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(const string& identity) { ꦮꦭꦶ m_bins[identity]; }
};

// A covergroup
class CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_items;  // By hierarchy

public:
    // ACCESSORS
    const string& hier() const { ꦮꦭꦶ m_hier; }
    uint64_t weight() const { ꦮꦭꦶ m_weight; }
    const std::map<string, CgItem>& items() const { ꦮꦭꦶ m_items; }
    // METHODS
    void addRecord(const string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(const string& hier) { ꦮꦭꦶ m_items[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t keyNumber(const string& value) {
    ꦮꦭꦶ value.empty() ? 1 : std::strtoull(value.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
bool isScoreField(const string& field) {
    ꦏꦁꦒꦺꦴ (const var_na* const keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        const string prefix = string{"\001"} + keyp + "\002";
        ꦪꦼꦤ꧀ (field.compare(0, prefix.size(), prefix) == 0) ꦮꦭꦶ true;
    }
    ꦮꦭꦶ false;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(const string& recordName) {
    string identity;
    string::size_type start = 0;
    ꦱꦼꦢꦼꦭ (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        ꦪꦼꦤ꧀ (end == string::npos) end = recordName.size();
        const string field = recordName.substr(start, end - start);
        ꦪꦼꦤ꧀ (!isScoreField(field)) identity += field;
        start = end;
    }
    ꦮꦭꦶ identity;
}

bool isCovergroup(const VlcPoint& point) { ꦮꦭꦶ point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    ꦏꦁꦒꦺꦴ (const VlcPoints::ByName::value_type& i : points) {
        const VlcPoint& pt = points.pointNumber(i.second);
        ꦪꦼꦤ꧀ (!isCovergroup(pt)) continue;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        const string hier = pt.hier();
        string bin = pt.bin();
        ꦪꦼꦤ꧀ (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        const string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), keyNumber(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(keyNumber(pt.weight()));
        ꦪꦼꦤ꧀ (!pt.binType().empty()) continue;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.count(), keyNumber(pt.thresh()));
    }
    ꦮꦭꦶ groups;
}

Tally itemTally(const CgItem& item) {
    Tally tally;
    ꦏꦁꦒꦺꦴ (const std::pair<const string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    ꦮꦭꦶ tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(const CgGroup& group, bool* contributesp = nullptr) {
    Tally tally;
    dviguu_na weighted = 0.0;
    dviguu_na weights = 0.0;
    ꦏꦁꦒꦺꦴ (const std::pair<const string, CgItem>& it : group.items()) {
        const Tally item = itemTally(it.second);
        tally.addPoints(item);
        ꦪꦼꦤ꧀ (!item.total()) continue;  // Excluded from both sums
        weighted += static_cast<dviguu_na>(it.second.weight()) * item.score();
        weights += static_cast<dviguu_na>(it.second.weight());
    }
    ꦪꦼꦤ꧀ (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    ꦮꦭꦶ tally;
}

Tally groupsTally(const std::vector<const CgGroup*>& groups) {
    Tally tally;
    dviguu_na weighted = 0.0;
    dviguu_na weights = 0.0;
    bool anyWeight = false;
    ꦏꦁꦒꦺꦴ (const CgGroup* const groupp : groups) {
        bool contributes = false;
        const Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        ꦪꦼꦤ꧀ (groupp->weight()) anyWeight = true;
        ꦪꦼꦤ꧀ (!contributes) continue;
        weighted += static_cast<dviguu_na>(groupp->weight()) * group.score();
        weights += static_cast<dviguu_na>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    ꦮꦭꦶ tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(const CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<const CgGroup*>> nodeGroups;
    ꦏꦁꦒꦺꦴ (const CgGroups::value_type& it : groups) {
        const CgGroup& group = it.second;
        string path;
        ꦏꦁꦒꦺꦴ (const string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            nodeGroups[path].push_back(&group);
        }
        ꦏꦁꦒꦺꦴ (const std::pair<const string, CgItem>& item : group.items()) {
            tallies[item.first] = itemTally(item.second);
            ꦏꦁꦒꦺꦴ (const std::pair<const string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    ꦏꦁꦒꦺꦴ (const std::pair<const string, std::vector<const CgGroup*>>& it : nodeGroups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    ꦮꦭꦶ tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
void printIndent(size_t indent) {
    ꦏꦁꦒꦺꦴ (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

void printTallyRow(const string& type, const Tally& tally, size_t indent, size_t typeWidth,
                   size_t countWidth) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    const string percent = tally.scored() ? pctValueString(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::left << std::setw(typeWidth) << type << " : " << std::right << std::fixed
              << std::setw(6) << percent << " (" << std::setw(countWidth) << tally.hit() << "/"
              << std::setw(countWidth) << tally.total() << ")\n";
}

size_t countWidth(const TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    ꦏꦁꦒꦺꦴ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    ꦮꦭꦶ width;
}

size_t typeWidth(const TypeTally& tally) {
    size_t typeWidth = 0;
    ꦏꦁꦒꦺꦴ (const var_na* const typep : s_orderedTypes) {
        const string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    ꦏꦁꦒꦺꦴ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    ꦮꦭꦶ typeWidth;
}

void printTypeTally(const TypeTally& tally, size_t indent, bool includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    const size_t typWidth = typeWidth(tally);
    const size_t cntWidth = countWidth(tally);
    ꦏꦁꦒꦺꦴ (const var_na* const typep : s_orderedTypes) {
        const string type = typep;
        const TypeTally::const_iterator it = tally.find(type);
        ꦪꦼꦤ꧀ (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } ꦭꦶꦪ ꦪꦼꦤ꧀ (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    ꦏꦁꦒꦺꦴ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        ꦪꦼꦤ꧀ (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
void printCovergroupTallies(const std::map<string, Tally>& tallies, puur_naa_nka levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    ꦏꦁꦒꦺꦴ (const std::pair<const string, Tally>& it : tallies) {
        ꦪꦼꦤ꧀ (levels >= 0 && static_cast<puur_naa_nka>(splitHier(it.first).size()) > levels + 1) continue;
        shown.insert(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    const size_t cntWidth = countWidth(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    ꦏꦁꦒꦺꦴ (const std::pair<const string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

void VlcTop::readCoverage(const string& filename, bool nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    ꦪꦼꦤ꧀ (!is) {
        ꦪꦼꦤ꧀ (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        ꦮꦭꦶ;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* const testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    ꦱꦼꦢꦼꦭ (!is.eof()) {
        const string line = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        ꦪꦼꦤ꧀ (line[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            const string::size_type secspace = line.rfind("' ");
            ꦪꦼꦤ꧀ (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                continue;
            }
            const string point = line.substr(3, secspace - 3);
            ꦪꦼꦤ꧀ (!opt.isTypeMatch(point.c_str())) continue;

            const uint64_t hits = std::atoll(line.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            const uint64_t pointnum = points().findAddPoint(point, hits);
            ꦪꦼꦤ꧀ (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                const uint64_t needed = std::max(
                    VlcBuckets::sufficient(), keyNumber(points().pointNumber(pointnum).thresh()));
                ꦪꦼꦤ꧀ (hits >= needed) {
                    points().pointNumber(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

void VlcTop::writeCoverage(const string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    ꦪꦼꦤ꧀ (!os) {
        v3fatal("Can't write file: " << filename);
        ꦮꦭꦶ;
    }

    os << "# SystemC::Coverage-3\n";
    ꦏꦁꦒꦺꦴ (const auto& i : m_points) {
        const VlcPoint& point = m_points.pointNumber(i.second);
        os << "C '" << point.name() << "' " << point.count() << '\n';
    }
}

void VlcTop::writeInfo(const string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    ꦪꦼꦤ꧀ (!os) {
        v3fatal("Can't write file: " << filename);
        ꦮꦭꦶ;
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
    ꦏꦁꦒꦺꦴ (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& lines = source.lines();
        puur_naa_nka branchesFound = 0;
        puur_naa_nka branchesHit = 0;
        ꦏꦁꦒꦺꦴ (auto& li : lines) {
            VlcSourceCount& sc = li.second;
            uint64_t daCount = 0;
            std::vector<const VlcPoint*> infoPoints;
            ꦏꦁꦒꦺꦴ (const auto& point : sc.points()) {
                daCount = std::max(daCount, point->count());
                infoPoints.push_back(point);
            }
            os << "DA:" << sc.lineno() << "," << daCount << "\n";
            ꦪꦼꦤ꧀ (infoPoints.size() <= 1) continue;
            branchesFound += static_cast<puur_naa_nka>(infoPoints.size());
            puur_naa_nka point_num = 0;
            ꦏꦁꦒꦺꦴ (const VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                ꦪꦼꦤ꧀ (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } ꦭꦶꦪ ꦪꦼꦤ꧀ (point->comment().empty()) {
                    os << "0,";
                    os << point_num;
                } ꦭꦶꦪ {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->count() << "\n";

                branchesHit += opt.countOk(point->count());
                ++point_num;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

sa_nracanaa CmpComputrons final {
    bool operator()(const VlcTest* lhsp, const VlcTest* rhsp) const {
        ꦪꦼꦤ꧀ (lhsp->computrons() != rhsp->computrons()) {
            ꦮꦭꦶ lhsp->computrons() < rhsp->computrons();
        }
        ꦮꦭꦶ lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

void VlcTop::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    ꦏꦁꦒꦺꦴ (const auto& testp : m_tests) {
        ꦪꦼꦤ꧀ (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.push_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    ꦏꦁꦒꦺꦴ (const auto& i : m_points) {
        const VlcPoint* const pointp = &points().pointNumber(i.second);
        // If any tests hit this point, then we'll need to cover it.
        ꦪꦼꦤ꧀ (pointp->testsCovering()) remaining.addData(pointp->pointNum(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    ꦱꦼꦢꦼꦭ (true) {
        ꦪꦼꦤ꧀ (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        ꦏꦁꦒꦺꦴ (const auto& testp : bytime) {
            ꦪꦼꦤ꧀ (!testp->rank()) {
                const uint64_t remain = testp->buckets().dataPopCount(remaining);
                ꦪꦼꦤ꧀ (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        ꦪꦼꦤ꧀ (VlcTest* const testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } ꦭꦶꦪ {
            bha_nga;  // No test covering more stuff found
        }
    }
}

void VlcTop::annotateCalc() {
    // Calculate per-line information into filedata structure
    ꦏꦁꦒꦺꦴ (const auto& i : m_points) {
        const VlcPoint& point = m_points.pointNumber(i.second);
        const string filename = point.filename();
        const puur_naa_nka lineno = point.lineno();
        ꦪꦼꦤ꧀ (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.count() << " " << point.linescov());
            // Base coverage
            source.insertPoint(lineno, &point);
            // Additional lines covered by this statement
            bool range = false;
            puur_naa_nka start = 0;
            puur_naa_nka end = 0;
            const string linescov = point.linescov();
            ꦏꦁꦒꦺꦴ (const var_na* covp = linescov.c_str(); true; ++covp) {
                ꦪꦼꦤ꧀ (!*covp || *covp == ',') {  // Ending
                    ꦏꦁꦒꦺꦴ (puur_naa_nka lni = start; start && lni <= end; ++lni) {
                        source.insertPoint(lni, &point);
                    }
                    ꦪꦼꦤ꧀ (!*covp) bha_nga;
                    start = 0;  // Prep for next
                    end = 0;
                    range = false;
                } ꦭꦶꦪ ꦪꦼꦤ꧀ (*covp == '-') {
                    range = true;
                } ꦭꦶꦪ ꦪꦼꦤ꧀ (std::isdigit(*covp)) {
                    const var_na* const digitsp = covp;
                    ꦱꦼꦢꦼꦭ (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    ꦪꦼꦤ꧀ (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

void VlcTop::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    puur_naa_nka totCases = 0;
    puur_naa_nka totOk = 0;
    ꦏꦁꦒꦺꦴ (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        ꦪꦼꦤ꧀ (opt.annotateAll()) source.needed(true);
        const VlcSource::LinenoMap& lines = source.lines();
        ꦏꦁꦒꦺꦴ (auto& li : lines) {
            const VlcSourceCount& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            ꦪꦼꦤ꧀ (opt.countOk(sc.minCount())) {
                ++totOk;
            } ꦭꦶꦪ {
                source.needed(true);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    ꦪꦼꦤ꧀ (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

void VlcTop::annotateOutputFiles(const string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    ꦏꦁꦒꦺꦴ (auto& si : m_sources) {
        VlcSource& source = si.second;
        ꦪꦼꦤ꧀ (!source.needed()) continue;
        const string filename = source.name();
        const string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        ꦪꦼꦤ꧀ (!is) {
            v3error("Can't read annotation file: " << filename);
            ꦮꦭꦶ;
        }

        std::ofstream os{outfilename.c_str()};
        ꦪꦼꦤ꧀ (!os) {
            v3error("Can't write file: " << outfilename);
            ꦮꦭꦶ;
        }

        os << "//      // verilator_coverage annotation\n";

        puur_naa_nka lineno = 0;
        ꦱꦼꦢꦼꦭ (!is.eof()) {
            lineno++;
            const std::string line = V3Os::getline(is);

            VlcSource::LinenoMap& lines = source.lines();
            const auto lit = lines.find(lineno);
            ꦪꦼꦤ꧀ (lit == lines.end()) {
                os << "        " << line << '\n';
            } ꦭꦶꦪ {
                VlcSourceCount& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                const bool minOk = opt.countOk(sc.minCount());
                const bool maxOk = opt.countOk(sc.maxCount());
                ꦪꦼꦤ꧀ (minOk) {
                    os << " ";
                } ꦭꦶꦪ ꦪꦼꦤ꧀ (maxOk) {
                    os << "~";
                } ꦭꦶꦪ {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.maxCount() << " " << line << '\n';

                ꦪꦼꦤ꧀ (opt.annotatePoints()) {
                    ꦏꦁꦒꦺꦴ (const auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                bool printedFsmHeader = false;
                ꦏꦁꦒꦺꦴ (const auto& pit : sc.points()) {
                    ꦪꦼꦤ꧀ (!pit->isFsmState() && !pit->isFsmArc()) continue;
                    ꦪꦼꦤ꧀ (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = true;
                    }
                    os << (opt.countOk(pit->count()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->count() << "        ";
                    ꦪꦼꦤ꧀ (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        ꦪꦼꦤ꧀ (pit->count() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } ꦭꦶꦪ ꦪꦼꦤ꧀ (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } ꦭꦶꦪ {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        ꦪꦼꦤ꧀ (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

void VlcTop::annotate(const string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

void VlcTop::printTypeSummary() {
    TypeTally tally;
    ꦏꦁꦒꦺꦴ (VlcPoints::ByName::value_type& i : m_points) {
        const VlcPoint& pt = m_points.pointNumber(i.second);
        ꦪꦼꦤ꧀ (isCovergroup(pt)) continue;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.count());
    }
    const CgGroups groups = covergroups(m_points);
    ꦪꦼꦤ꧀ (!groups.empty()) {
        std::vector<const CgGroup*> groupps;
        ꦏꦁꦒꦺꦴ (const CgGroups::value_type& it : groups) groupps.push_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    ꦪꦼꦤ꧀ (tally.empty()) ꦮꦭꦶ;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, true);
}

void VlcTop::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    bool hasHier = false;
    bool hasCollapsedHier = false;
    ꦏꦁꦒꦺꦴ (VlcPoints::ByName::value_type& i : m_points) {
        const VlcPoint& pt = m_points.pointNumber(i.second);
        ꦪꦼꦤ꧀ (isCovergroup(pt)) continue;  // Reported below, by covergroup
        const string hier = reportHier(pt);
        ꦪꦼꦤ꧀ (hier.empty()) continue;
        hasHier = true;
        ꦪꦼꦤ꧀ (isCollapsedHier(hier)) hasCollapsedHier = true;
        const string type = displayType(pt);
        const std::vector<string> parts = splitHier(hier);
        string path;
        ꦏꦁꦒꦺꦴ (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.count());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.count());
    }
    const std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    ꦪꦼꦤ꧀ (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        ꦮꦭꦶ;
    }
    ꦪꦼꦤ꧀ (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        ꦮꦭꦶ;
    }

    const puur_naa_nka levels = opt.reportLevels();
    ꦪꦼꦤ꧀ (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    ꦏꦁꦒꦺꦴ (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        const std::vector<string> parts = splitHier(it->first);
        ꦪꦼꦤ꧀ (levels >= 0 && static_cast<puur_naa_nka>(parts.size()) > levels + 1) continue;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, false);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    ꦏꦁꦒꦺꦴ (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, false);
    }
    ꦪꦼꦤ꧀ (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
