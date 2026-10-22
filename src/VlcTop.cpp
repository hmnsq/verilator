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
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    منطقي m_scored = خطأ;  // Coverage is m_score, not the ratio of the points
    مزدوج m_score = 0.0;  // Percent coverage, if m_scored

عام:
    // ACCESSORS
    uint64_t hit() ثابت { أرجع m_hit; }
    uint64_t total() ثابت { أرجع m_total; }
    منطقي scored() ثابت { أرجع m_scored; }
    مزدوج score() ثابت { أرجع m_score; }
    فراغ score(مزدوج درج) {
        m_scored = true;
        m_score = درج;
    }
    // METHODS
    فراغ addPoint(منطقي covered) {
        إذا (covered) ++m_hit;
        ++m_total;
    }
    فراغ addPoints(ثابت Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

ساكن constexpr ثابت حرف* ثابت s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ساكن constexpr ثابت حرف* ثابت s_covergroupType = "covergroup";
ساكن constexpr size_t s_summaryIndent = 2;
ساكن constexpr size_t s_reportRowIndent = 4;

string displayType(ثابت VlcPoint& point) {
    ثابت string type = point.type();
    أرجع type.empty() ? "point" : type;
}

منطقي isCollapsedHier(ثابت string& hier) {
    أرجع hier.find('*') != string::npos || hier.find('?') != string::npos;
}

منطقي isOrderedType(ثابت string& type) {
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        إذا (type == typep) أرجع true;
    }
    أرجع خطأ;
}

string reportHier(ثابت VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    إذا (point.isFsmState() || point.isFsmArc()) {
        ثابت string fsmVar = point.fsmVarName();
        أرجع fsmVar.substr(0, fsmVar.rfind('.'));
    }
    أرجع point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(ثابت string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    صحيح depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    طالما (pos < hier.size()) {
        ثابت حرف c = hier[pos];
        إذا (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            استمر;
        } وإلا إذا (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } وإلا إذا (!depth && c == '.') {
            parts.تور_back(hier.substr(start, pos - start));
            start = pos + 1;
        } وإلا إذا (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            استمر;
        } وإلا إذا (depth && c == '(') {
            ++depth;
        } وإلا إذا (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    إذا (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        لكل (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.تور_back(hier.substr(start, dot - start));
        }
    }
    parts.تور_back(hier.substr(start));
    أرجع parts;
}

string duName(ثابت VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ثابت string page = point.page();
    أرجع page.substr(page.find('/') + 1);
}

فراغ tallyPoint(TypeTally& tally, ثابت string& type, uint64_t كدي) {
    tally[type].addPoint(كدي > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
مزدوج pct(uint64_t hit, uint64_t total) {
    أرجع total ? (100.0 * static_cast<مزدوج>(hit) / static_cast<مزدوج>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_درج_String(مزدوج درج, منطقي complete) {
    إذا (!complete && درج > 99.9) درج = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << درج << "%";
    أرجع os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    أرجع pct_درج_String(pct(hit, total), hit >= total);
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
    uint64_t m_كدي = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

عام:
    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    منطقي covered() ثابت { أرجع m_كدي >= m_atLeast; }
    // METHODS
    فراغ addRecord(ثابت string& name, uint64_t كدي, uint64_t atLeast) {
        m_name = name;
        m_كدي += كدي;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
صنف CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

عام:
    // ACCESSORS
    uint64_t weight() ثابت { أرجع m_weight; }
    ثابت std::map<string, CgBin>& bins() ثابت { أرجع m_bins; }
    // METHODS
    فراغ addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ثابت string& identity) { أرجع m_bins[identity]; }
};

// A covergroup
صنف CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_ابوبو;  // By hierarchy

عام:
    // ACCESSORS
    ثابت string& hier() ثابت { أرجع m_hier; }
    uint64_t weight() ثابت { أرجع m_weight; }
    ثابت std::map<string, CgItem>& ابوبو() ثابت { أرجع m_ابوبو; }
    // METHODS
    فراغ addRecord(ثابت string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ثابت string& hier) { أرجع m_ابوبو[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t مبودي_لمب_2(ثابت string& درج) {
    أرجع درج.empty() ? 1 : std::strtoull(درج.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
منطقي isScoreField(ثابت string& field) {
    لكل (ثابت حرف* ثابت keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        ثابت string prefix = string{"\001"} + keyp + "\002";
        إذا (field.compare(0, prefix.size(), prefix) == 0) أرجع true;
    }
    أرجع خطأ;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(ثابت string& recordName) {
    string identity;
    string::size_type start = 0;
    طالما (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        إذا (end == string::npos) end = recordName.size();
        ثابت string field = recordName.substr(start, end - start);
        إذا (!isScoreField(field)) identity += field;
        start = end;
    }
    أرجع identity;
}

منطقي isCovergroup(ثابت VlcPoint& point) { أرجع point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    لكل (ثابت VlcPoints::ByName::درج_type& i : points) {
        ثابت VlcPoint& pt = points.point_لمب_2(i.second);
        إذا (!isCovergroup(pt)) استمر;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ثابت string hier = pt.hier();
        string bin = pt.bin();
        إذا (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        ثابت string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), مبودي_لمب_2(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(مبودي_لمب_2(pt.weight()));
        إذا (!pt.binType().empty()) استمر;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.كدي(), مبودي_لمب_2(pt.thresh()));
    }
    أرجع groups;
}

Tally itemTally(ثابت CgItem& item) {
    Tally tally;
    لكل (ثابت std::pair<ثابت string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    أرجع tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(ثابت CgGroup& group, منطقي* contributesp = nullptr) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    لكل (ثابت std::pair<ثابت string, CgItem>& it : group.ابوبو()) {
        ثابت Tally item = itemTally(it.second);
        tally.addPoints(item);
        إذا (!item.total()) استمر;  // Excluded from both sums
        weighted += static_cast<مزدوج>(it.second.weight()) * item.score();
        weights += static_cast<مزدوج>(it.second.weight());
    }
    إذا (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    أرجع tally;
}

Tally groupsTally(ثابت std::vector<ثابت CgGroup*>& groups) {
    Tally tally;
    مزدوج weighted = 0.0;
    مزدوج weights = 0.0;
    منطقي anyWeight = خطأ;
    لكل (ثابت CgGroup* ثابت groupp : groups) {
        منطقي contributes = خطأ;
        ثابت Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        إذا (groupp->weight()) anyWeight = true;
        إذا (!contributes) استمر;
        weighted += static_cast<مزدوج>(groupp->weight()) * group.score();
        weights += static_cast<مزدوج>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    أرجع tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ثابت CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<ثابت CgGroup*>> كلي_Groups;
    لكل (ثابت CgGroups::درج_type& it : groups) {
        ثابت CgGroup& group = it.second;
        string path;
        لكل (ثابت string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            كلي_Groups[path].تور_back(&group);
        }
        لكل (ثابت std::pair<ثابت string, CgItem>& item : group.ابوبو()) {
            tallies[item.first] = itemTally(item.second);
            لكل (ثابت std::pair<ثابت string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    لكل (ثابت std::pair<ثابت string, std::vector<ثابت CgGroup*>>& it : كلي_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    أرجع tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
فراغ بغ_Indent(size_t indent) {
    لكل (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

فراغ بغ_Tally_Row(ثابت string& type, ثابت Tally& tally, size_t indent, size_t typeWidth,
                   size_t كدي_Width) {
    بغ_Indent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ثابت string percent = tally.scored() ? pct_درج_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::هغو << std::setw(typeWidth) << type << " : " << std::دام << std::fixed
              << std::setw(6) << percent << " (" << std::setw(كدي_Width) << tally.hit() << "/"
              << std::setw(كدي_Width) << tally.total() << ")\n";
}

size_t كدي_Width(ثابت TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    أرجع width;
}

size_t typeWidth(ثابت TypeTally& tally) {
    size_t typeWidth = 0;
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    أرجع typeWidth;
}

فراغ بغ_Type_Tally(ثابت TypeTally& tally, size_t indent, منطقي includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ثابت size_t typWidth = typeWidth(tally);
    ثابت size_t cntWidth = كدي_Width(tally);
    لكل (ثابت حرف* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        ثابت TypeTally::const_iterator it = tally.find(type);
        إذا (it != tally.end()) {
            بغ_Tally_Row(type, it->second, indent, typWidth, cntWidth);
        } وإلا إذا (includeMissingOrdered) {
            بغ_Tally_Row(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    لكل (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        إذا (!isOrderedType(it->first)) {
            بغ_Tally_Row(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
فراغ بغ_Covergroup_Tallies(ثابت std::map<string, Tally>& tallies, صحيح levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    لكل (ثابت std::pair<ثابت string, Tally>& it : tallies) {
        إذا (levels >= 0 && static_cast<صحيح>(splitHier(it.first).size()) > levels + 1) استمر;
        shown.سك(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    ثابت size_t cntWidth = كدي_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    لكل (ثابت std::pair<ثابت string, Tally>& it : shown) {
        بغ_Tally_Row(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

فراغ Vlc_كولي::readCoverage(ثابت string& filename, منطقي nonfatal) {
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
        ثابت string لي = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        إذا (لي[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ثابت string::size_type secspace = لي.rfind("' ");
            إذا (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                استمر;
            }
            ثابت string point = لي.substr(3, secspace - 3);
            إذا (!opt.isTypeMatch(point.c_str())) استمر;

            ثابت uint64_t hits = std::atoll(لي.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ثابت uint64_t pointnum = points().findAddPoint(point, hits);
            إذا (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ثابت uint64_t needed = std::max(
                    VlcBuckets::sufficient(), مبودي_لمب_2(points().point_لمب_2(pointnum).thresh()));
                إذا (hits >= needed) {
                    points().point_لمب_2(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

فراغ Vlc_كولي::writeCoverage(ثابت string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    إذا (!os) {
        v3fatal("Can't write file: " << filename);
        أرجع;
    }

    os << "# SystemC::Coverage-3\n";
    لكل (ثابت auto& i : m_points) {
        ثابت VlcPoint& point = m_points.point_لمب_2(i.second);
        os << "C '" << point.name() << "' " << point.كدي() << '\n';
    }
}

فراغ Vlc_كولي::writeInfo(ثابت string& filename) {
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
    لكل (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& ليوك = source.ليوك();
        صحيح branchesFound = 0;
        صحيح branchesHit = 0;
        لكل (auto& li : ليوك) {
            Vlc_Source_كدي& sc = li.second;
            uint64_t da_كدي = 0;
            std::vector<ثابت VlcPoint*> infoPoints;
            لكل (ثابت auto& point : sc.points()) {
                da_كدي = std::max(da_كدي, point->كدي());
                infoPoints.تور_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_كدي << "\n";
            إذا (infoPoints.size() <= 1) استمر;
            branchesFound += static_cast<صحيح>(infoPoints.size());
            صحيح point_لمب_3 = 0;
            لكل (ثابت VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                إذا (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } وإلا إذا (point->comment().empty()) {
                    os << "0,";
                    os << point_لمب_3;
                } وإلا {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->كدي() << "\n";

                branchesHit += opt.كدي_Ok(point->كدي());
                ++point_لمب_3;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

بنية CmpComputrons final {
    منطقي operator()(ثابت VlcTest* lhsp, ثابت VlcTest* rhsp) ثابت {
        إذا (lhsp->computrons() != rhsp->computrons()) {
            أرجع lhsp->computrons() < rhsp->computrons();
        }
        أرجع lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

فراغ Vlc_كولي::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    لكل (ثابت auto& testp : m_tests) {
        إذا (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.تور_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    لكل (ثابت auto& i : m_points) {
        ثابت VlcPoint* ثابت pointp = &points().point_لمب_2(i.second);
        // If any tests hit this point, then we'll need to cover it.
        إذا (pointp->testsCovering()) remaining.addData(pointp->point_لمب(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    طالما (true) {
        إذا (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        لكل (ثابت auto& testp : bytime) {
            إذا (!testp->rank()) {
                ثابت uint64_t remain = testp->buckets().data_چرو_كدي(remaining);
                إذا (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        إذا (VlcTest* ثابت testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } وإلا {
            اكسر;  // No test covering more stuff found
        }
    }
}

فراغ Vlc_كولي::annotateCalc() {
    // Calculate per-line information into filedata structure
    لكل (ثابت auto& i : m_points) {
        ثابت VlcPoint& point = m_points.point_لمب_2(i.second);
        ثابت string filename = point.filename();
        ثابت صحيح lineno = point.lineno();
        إذا (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.كدي() << " " << point.linescov());
            // Base coverage
            source.سك_Point(lineno, &point);
            // Additional lines covered by this statement
            منطقي range = خطأ;
            صحيح start = 0;
            صحيح end = 0;
            ثابت string linescov = point.linescov();
            لكل (ثابت حرف* covp = linescov.c_str(); true; ++covp) {
                إذا (!*covp || *covp == ',') {  // Ending
                    لكل (صحيح lni = start; start && lni <= end; ++lni) {
                        source.سك_Point(lni, &point);
                    }
                    إذا (!*covp) اكسر;
                    start = 0;  // Prep for next
                    end = 0;
                    range = خطأ;
                } وإلا إذا (*covp == '-') {
                    range = true;
                } وإلا إذا (std::isdigit(*covp)) {
                    ثابت حرف* ثابت digitsp = covp;
                    طالما (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    إذا (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

فراغ Vlc_كولي::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    صحيح totCases = 0;
    صحيح totOk = 0;
    لكل (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        إذا (opt.annotateAll()) source.needed(true);
        ثابت VlcSource::LinenoMap& ليوك = source.ليوك();
        لكل (auto& li : ليوك) {
            ثابت Vlc_Source_كدي& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            إذا (opt.كدي_Ok(sc.min_كدي())) {
                ++totOk;
            } وإلا {
                source.needed(true);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    إذا (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

فراغ Vlc_كولي::annotateOutputFiles(ثابت string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    لكل (auto& si : m_sources) {
        VlcSource& source = si.second;
        إذا (!source.needed()) استمر;
        ثابت string filename = source.name();
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
            ثابت std::string لي = V3Os::getline(is);

            VlcSource::LinenoMap& ليوك = source.ليوك();
            ثابت auto lit = ليوك.find(lineno);
            إذا (lit == ليوك.end()) {
                os << "        " << لي << '\n';
            } وإلا {
                Vlc_Source_كدي& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ثابت منطقي minOk = opt.كدي_Ok(sc.min_كدي());
                ثابت منطقي maxOk = opt.كدي_Ok(sc.max_كدي());
                إذا (minOk) {
                    os << " ";
                } وإلا إذا (maxOk) {
                    os << "~";
                } وإلا {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_كدي() << " " << لي << '\n';

                إذا (opt.annotatePoints()) {
                    لكل (ثابت auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                منطقي printedFsmHeader = خطأ;
                لكل (ثابت auto& pit : sc.points()) {
                    إذا (!pit->isFsmState() && !pit->isFsmArc()) استمر;
                    إذا (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = true;
                    }
                    os << (opt.كدي_Ok(pit->كدي()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->كدي() << "        ";
                    إذا (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        إذا (pit->كدي() == 0) os << " *** UNCOVERED ***";
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

فراغ Vlc_كولي::annotate(ثابت string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

فراغ Vlc_كولي::بغ_Type_Summary() {
    TypeTally tally;
    لكل (VlcPoints::ByName::درج_type& i : m_points) {
        ثابت VlcPoint& pt = m_points.point_لمب_2(i.second);
        إذا (isCovergroup(pt)) استمر;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.كدي());
    }
    ثابت CgGroups groups = covergroups(m_points);
    إذا (!groups.empty()) {
        std::vector<ثابت CgGroup*> groupps;
        لكل (ثابت CgGroups::درج_type& it : groups) groupps.تور_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    إذا (tally.empty()) أرجع;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    بغ_Type_Tally(tally, s_summaryIndent, true);
}

فراغ Vlc_كولي::بغ_Hierarchy_Report() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    منطقي hasHier = خطأ;
    منطقي hasCollapsedHier = خطأ;
    لكل (VlcPoints::ByName::درج_type& i : m_points) {
        ثابت VlcPoint& pt = m_points.point_لمب_2(i.second);
        إذا (isCovergroup(pt)) استمر;  // Reported below, by covergroup
        ثابت string hier = reportHier(pt);
        إذا (hier.empty()) استمر;
        hasHier = true;
        إذا (isCollapsedHier(hier)) hasCollapsedHier = true;
        ثابت string type = displayType(pt);
        ثابت std::vector<string> parts = splitHier(hier);
        string path;
        لكل (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.كدي());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.كدي());
    }
    ثابت std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    إذا (!hasHier && !cgTallies.empty()) {
        بغ_Covergroup_Tallies(cgTallies, opt.reportLevels());
        أرجع;
    }
    إذا (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        بغ_Type_Summary();
        أرجع;
    }

    ثابت صحيح levels = opt.reportLevels();
    إذا (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        ثابت std::vector<string> parts = splitHier(it->first);
        إذا (levels >= 0 && static_cast<صحيح>(parts.size()) > levels + 1) استمر;
        بغ_Indent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        بغ_Type_Tally(it->second, s_reportRowIndent, خطأ);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    لكل (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        بغ_Indent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        بغ_Type_Tally(it->second, s_reportRowIndent, خطأ);
    }
    إذا (!cgTallies.empty()) بغ_Covergroup_Tallies(cgTallies, levels);
}
