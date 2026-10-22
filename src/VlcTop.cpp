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
    uint64_t hit() const { คืน m_hit; }
    uint64_t total() const { คืน m_total; }
    bool scored() const { คืน m_scored; }
    dviguu_na score() const { คืน m_score; }
    void score(dviguu_na value) {
        m_scored = true;
        m_score = value;
    }
    // METHODS
    void addPoint(bool covered) {
        ถ้า (covered) ++m_hit;
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
    คืน type.empty() ? "point" : type;
}

bool isCollapsedHier(const string& hier) {
    คืน hier.find('*') != string::npos || hier.find('?') != string::npos;
}

bool isOrderedType(const string& type) {
    สำหรับ (const var_na* const typep : s_orderedTypes) {
        ถ้า (type == typep) คืน true;
    }
    คืน false;
}

string reportHier(const VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    ถ้า (point.isFsmState() || point.isFsmArc()) {
        const string fsmVar = point.fsmVarName();
        คืน fsmVar.substr(0, fsmVar.rfind('.'));
    }
    คืน point.hier();
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
    จำนวนเต็ม depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    ขณะ (pos < hier.size()) {
        const var_na c = hier[pos];
        ถ้า (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            continue;
        } มิฉะนั้น ถ้า (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } มิฉะนั้น ถ้า (!depth && c == '.') {
            parts.push_back(hier.substr(start, pos - start));
            start = pos + 1;
        } มิฉะนั้น ถ้า (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            continue;
        } มิฉะนั้น ถ้า (depth && c == '(') {
            ++depth;
        } มิฉะนั้น ถ้า (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    ถ้า (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        สำหรับ (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.push_back(hier.substr(start, dot - start));
        }
    }
    parts.push_back(hier.substr(start));
    คืน parts;
}

string duName(const VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    const string page = point.page();
    คืน page.substr(page.find('/') + 1);
}

void tallyPoint(TypeTally& tally, const string& type, uint64_t count) {
    tally[type].addPoint(count > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
dviguu_na pct(uint64_t hit, uint64_t total) {
    คืน total ? (100.0 * static_cast<dviguu_na>(hit) / static_cast<dviguu_na>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pctValueString(dviguu_na value, bool complete) {
    ถ้า (!complete && value > 99.9) value = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << value << "%";
    คืน os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    คืน pctValueString(pct(hit, total), hit >= total);
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
    const string& name() const { คืน m_name; }
    bool covered() const { คืน m_count >= m_atLeast; }
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
    uint64_t weight() const { คืน m_weight; }
    const std::map<string, CgBin>& bins() const { คืน m_bins; }
    // METHODS
    void addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(const string& identity) { คืน m_bins[identity]; }
};

// A covergroup
class CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_items;  // By hierarchy

public:
    // ACCESSORS
    const string& hier() const { คืน m_hier; }
    uint64_t weight() const { คืน m_weight; }
    const std::map<string, CgItem>& items() const { คืน m_items; }
    // METHODS
    void addRecord(const string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(const string& hier) { คืน m_items[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t keyNumber(const string& value) {
    คืน value.empty() ? 1 : std::strtoull(value.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
bool isScoreField(const string& field) {
    สำหรับ (const var_na* const keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        const string prefix = string{"\001"} + keyp + "\002";
        ถ้า (field.compare(0, prefix.size(), prefix) == 0) คืน true;
    }
    คืน false;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(const string& recordName) {
    string identity;
    string::size_type start = 0;
    ขณะ (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        ถ้า (end == string::npos) end = recordName.size();
        const string field = recordName.substr(start, end - start);
        ถ้า (!isScoreField(field)) identity += field;
        start = end;
    }
    คืน identity;
}

bool isCovergroup(const VlcPoint& point) { คืน point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    สำหรับ (const VlcPoints::ByName::value_type& i : points) {
        const VlcPoint& pt = points.pointNumber(i.second);
        ถ้า (!isCovergroup(pt)) continue;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        const string hier = pt.hier();
        string bin = pt.bin();
        ถ้า (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        const string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), keyNumber(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(keyNumber(pt.weight()));
        ถ้า (!pt.binType().empty()) continue;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.count(), keyNumber(pt.thresh()));
    }
    คืน groups;
}

Tally itemTally(const CgItem& item) {
    Tally tally;
    สำหรับ (const std::pair<const string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    คืน tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(const CgGroup& group, bool* contributesp = nullptr) {
    Tally tally;
    dviguu_na weighted = 0.0;
    dviguu_na weights = 0.0;
    สำหรับ (const std::pair<const string, CgItem>& it : group.items()) {
        const Tally item = itemTally(it.second);
        tally.addPoints(item);
        ถ้า (!item.total()) continue;  // Excluded from both sums
        weighted += static_cast<dviguu_na>(it.second.weight()) * item.score();
        weights += static_cast<dviguu_na>(it.second.weight());
    }
    ถ้า (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    คืน tally;
}

Tally groupsTally(const std::vector<const CgGroup*>& groups) {
    Tally tally;
    dviguu_na weighted = 0.0;
    dviguu_na weights = 0.0;
    bool anyWeight = false;
    สำหรับ (const CgGroup* const groupp : groups) {
        bool contributes = false;
        const Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        ถ้า (groupp->weight()) anyWeight = true;
        ถ้า (!contributes) continue;
        weighted += static_cast<dviguu_na>(groupp->weight()) * group.score();
        weights += static_cast<dviguu_na>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    คืน tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(const CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<const CgGroup*>> nodeGroups;
    สำหรับ (const CgGroups::value_type& it : groups) {
        const CgGroup& group = it.second;
        string path;
        สำหรับ (const string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            nodeGroups[path].push_back(&group);
        }
        สำหรับ (const std::pair<const string, CgItem>& item : group.items()) {
            tallies[item.first] = itemTally(item.second);
            สำหรับ (const std::pair<const string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    สำหรับ (const std::pair<const string, std::vector<const CgGroup*>>& it : nodeGroups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    คืน tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
void printIndent(size_t indent) {
    สำหรับ (size_t i = 0; i < indent; ++i) std::cout << ' ';
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
    สำหรับ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    คืน width;
}

size_t typeWidth(const TypeTally& tally) {
    size_t typeWidth = 0;
    สำหรับ (const var_na* const typep : s_orderedTypes) {
        const string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    สำหรับ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    คืน typeWidth;
}

void printTypeTally(const TypeTally& tally, size_t indent, bool includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    const size_t typWidth = typeWidth(tally);
    const size_t cntWidth = countWidth(tally);
    สำหรับ (const var_na* const typep : s_orderedTypes) {
        const string type = typep;
        const TypeTally::const_iterator it = tally.find(type);
        ถ้า (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } มิฉะนั้น ถ้า (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    สำหรับ (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        ถ้า (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
void printCovergroupTallies(const std::map<string, Tally>& tallies, จำนวนเต็ม levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    สำหรับ (const std::pair<const string, Tally>& it : tallies) {
        ถ้า (levels >= 0 && static_cast<จำนวนเต็ม>(splitHier(it.first).size()) > levels + 1) continue;
        shown.insert(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    const size_t cntWidth = countWidth(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    สำหรับ (const std::pair<const string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

void VlcTop::readCoverage(const string& filename, bool nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    ถ้า (!is) {
        ถ้า (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        คืน;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* const testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    ขณะ (!is.eof()) {
        const string line = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        ถ้า (line[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            const string::size_type secspace = line.rfind("' ");
            ถ้า (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                continue;
            }
            const string point = line.substr(3, secspace - 3);
            ถ้า (!opt.isTypeMatch(point.c_str())) continue;

            const uint64_t hits = std::atoll(line.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            const uint64_t pointnum = points().findAddPoint(point, hits);
            ถ้า (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                const uint64_t needed = std::max(
                    VlcBuckets::sufficient(), keyNumber(points().pointNumber(pointnum).thresh()));
                ถ้า (hits >= needed) {
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
    ถ้า (!os) {
        v3fatal("Can't write file: " << filename);
        คืน;
    }

    os << "# SystemC::Coverage-3\n";
    สำหรับ (const auto& i : m_points) {
        const VlcPoint& point = m_points.pointNumber(i.second);
        os << "C '" << point.name() << "' " << point.count() << '\n';
    }
}

void VlcTop::writeInfo(const string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    ถ้า (!os) {
        v3fatal("Can't write file: " << filename);
        คืน;
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
    สำหรับ (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& lines = source.lines();
        จำนวนเต็ม branchesFound = 0;
        จำนวนเต็ม branchesHit = 0;
        สำหรับ (auto& li : lines) {
            VlcSourceCount& sc = li.second;
            uint64_t daCount = 0;
            std::vector<const VlcPoint*> infoPoints;
            สำหรับ (const auto& point : sc.points()) {
                daCount = std::max(daCount, point->count());
                infoPoints.push_back(point);
            }
            os << "DA:" << sc.lineno() << "," << daCount << "\n";
            ถ้า (infoPoints.size() <= 1) continue;
            branchesFound += static_cast<จำนวนเต็ม>(infoPoints.size());
            จำนวนเต็ม point_num = 0;
            สำหรับ (const VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                ถ้า (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } มิฉะนั้น ถ้า (point->comment().empty()) {
                    os << "0,";
                    os << point_num;
                } มิฉะนั้น {
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
        ถ้า (lhsp->computrons() != rhsp->computrons()) {
            คืน lhsp->computrons() < rhsp->computrons();
        }
        คืน lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

void VlcTop::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    สำหรับ (const auto& testp : m_tests) {
        ถ้า (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.push_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    สำหรับ (const auto& i : m_points) {
        const VlcPoint* const pointp = &points().pointNumber(i.second);
        // If any tests hit this point, then we'll need to cover it.
        ถ้า (pointp->testsCovering()) remaining.addData(pointp->pointNum(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    ขณะ (true) {
        ถ้า (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        สำหรับ (const auto& testp : bytime) {
            ถ้า (!testp->rank()) {
                const uint64_t remain = testp->buckets().dataPopCount(remaining);
                ถ้า (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        ถ้า (VlcTest* const testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } มิฉะนั้น {
            bha_nga;  // No test covering more stuff found
        }
    }
}

void VlcTop::annotateCalc() {
    // Calculate per-line information into filedata structure
    สำหรับ (const auto& i : m_points) {
        const VlcPoint& point = m_points.pointNumber(i.second);
        const string filename = point.filename();
        const จำนวนเต็ม lineno = point.lineno();
        ถ้า (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.count() << " " << point.linescov());
            // Base coverage
            source.insertPoint(lineno, &point);
            // Additional lines covered by this statement
            bool range = false;
            จำนวนเต็ม start = 0;
            จำนวนเต็ม end = 0;
            const string linescov = point.linescov();
            สำหรับ (const var_na* covp = linescov.c_str(); true; ++covp) {
                ถ้า (!*covp || *covp == ',') {  // Ending
                    สำหรับ (จำนวนเต็ม lni = start; start && lni <= end; ++lni) {
                        source.insertPoint(lni, &point);
                    }
                    ถ้า (!*covp) bha_nga;
                    start = 0;  // Prep for next
                    end = 0;
                    range = false;
                } มิฉะนั้น ถ้า (*covp == '-') {
                    range = true;
                } มิฉะนั้น ถ้า (std::isdigit(*covp)) {
                    const var_na* const digitsp = covp;
                    ขณะ (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    ถ้า (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

void VlcTop::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    จำนวนเต็ม totCases = 0;
    จำนวนเต็ม totOk = 0;
    สำหรับ (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        ถ้า (opt.annotateAll()) source.needed(true);
        const VlcSource::LinenoMap& lines = source.lines();
        สำหรับ (auto& li : lines) {
            const VlcSourceCount& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            ถ้า (opt.countOk(sc.minCount())) {
                ++totOk;
            } มิฉะนั้น {
                source.needed(true);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    ถ้า (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

void VlcTop::annotateOutputFiles(const string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    สำหรับ (auto& si : m_sources) {
        VlcSource& source = si.second;
        ถ้า (!source.needed()) continue;
        const string filename = source.name();
        const string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        ถ้า (!is) {
            v3error("Can't read annotation file: " << filename);
            คืน;
        }

        std::ofstream os{outfilename.c_str()};
        ถ้า (!os) {
            v3error("Can't write file: " << outfilename);
            คืน;
        }

        os << "//      // verilator_coverage annotation\n";

        จำนวนเต็ม lineno = 0;
        ขณะ (!is.eof()) {
            lineno++;
            const std::string line = V3Os::getline(is);

            VlcSource::LinenoMap& lines = source.lines();
            const auto lit = lines.find(lineno);
            ถ้า (lit == lines.end()) {
                os << "        " << line << '\n';
            } มิฉะนั้น {
                VlcSourceCount& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                const bool minOk = opt.countOk(sc.minCount());
                const bool maxOk = opt.countOk(sc.maxCount());
                ถ้า (minOk) {
                    os << " ";
                } มิฉะนั้น ถ้า (maxOk) {
                    os << "~";
                } มิฉะนั้น {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.maxCount() << " " << line << '\n';

                ถ้า (opt.annotatePoints()) {
                    สำหรับ (const auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                bool printedFsmHeader = false;
                สำหรับ (const auto& pit : sc.points()) {
                    ถ้า (!pit->isFsmState() && !pit->isFsmArc()) continue;
                    ถ้า (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = true;
                    }
                    os << (opt.countOk(pit->count()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->count() << "        ";
                    ถ้า (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        ถ้า (pit->count() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } มิฉะนั้น ถ้า (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } มิฉะนั้น {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        ถ้า (pit->fsmIsReset() && !opt.includeResetArcs()) {
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
    สำหรับ (VlcPoints::ByName::value_type& i : m_points) {
        const VlcPoint& pt = m_points.pointNumber(i.second);
        ถ้า (isCovergroup(pt)) continue;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.count());
    }
    const CgGroups groups = covergroups(m_points);
    ถ้า (!groups.empty()) {
        std::vector<const CgGroup*> groupps;
        สำหรับ (const CgGroups::value_type& it : groups) groupps.push_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    ถ้า (tally.empty()) คืน;
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
    สำหรับ (VlcPoints::ByName::value_type& i : m_points) {
        const VlcPoint& pt = m_points.pointNumber(i.second);
        ถ้า (isCovergroup(pt)) continue;  // Reported below, by covergroup
        const string hier = reportHier(pt);
        ถ้า (hier.empty()) continue;
        hasHier = true;
        ถ้า (isCollapsedHier(hier)) hasCollapsedHier = true;
        const string type = displayType(pt);
        const std::vector<string> parts = splitHier(hier);
        string path;
        สำหรับ (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.count());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.count());
    }
    const std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    ถ้า (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        คืน;
    }
    ถ้า (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        คืน;
    }

    const จำนวนเต็ม levels = opt.reportLevels();
    ถ้า (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    สำหรับ (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        const std::vector<string> parts = splitHier(it->first);
        ถ้า (levels >= 0 && static_cast<จำนวนเต็ม>(parts.size()) > levels + 1) continue;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, false);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    สำหรับ (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, false);
    }
    ถ้า (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
