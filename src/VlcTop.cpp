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

فضاینام {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
رده Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    منطقی m_scored = نادرست;  // Coverage is m_score, not the ratio of the points
    دوگانه m_score = 0.0;  // Percent coverage, if m_scored

عمومی:
    // ACCESSORS
    uint64_t hit() ثابت { بازگشت m_hit; }
    uint64_t total() ثابت { بازگشت m_total; }
    منطقی scored() ثابت { بازگشت m_scored; }
    دوگانه score() ثابت { بازگشت m_score; }
    تهی score(دوگانه مقدار) {
        m_scored = درست;
        m_score = مقدار;
    }
    // METHODS
    تهی addPoint(منطقی covered) {
        اگر (covered) ++m_hit;
        ++m_total;
    }
    تهی addPoints(ثابت Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

ایستا constexpr ثابت نویسه* ثابت s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ایستا constexpr ثابت نویسه* ثابت s_covergroupType = "covergroup";
ایستا constexpr size_t s_summaryIndent = 2;
ایستا constexpr size_t s_reportRowIndent = 4;

string displayType(ثابت VlcPoint& point) {
    ثابت string type = point.type();
    بازگشت type.empty() ? "point" : type;
}

منطقی isCollapsedHier(ثابت string& hier) {
    بازگشت hier.find('*') != string::npos || hier.find('?') != string::npos;
}

منطقی isOrderedType(ثابت string& type) {
    برای (ثابت نویسه* ثابت typep : s_orderedTypes) {
        اگر (type == typep) بازگشت درست;
    }
    بازگشت نادرست;
}

string reportHier(ثابت VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    اگر (point.isFsmState() || point.isFsmArc()) {
        ثابت string fsmVar = point.fsmVarName();
        بازگشت fsmVar.substr(0, fsmVar.rfind('.'));
    }
    بازگشت point.hier();
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
    صحیح depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    وقتی (pos < hier.size()) {
        ثابت نویسه c = hier[pos];
        اگر (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            ادامه;
        } وگرنه اگر (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } وگرنه اگر (!depth && c == '.') {
            parts.افزودن_back(hier.substr(start, pos - start));
            start = pos + 1;
        } وگرنه اگر (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            ادامه;
        } وگرنه اگر (depth && c == '(') {
            ++depth;
        } وگرنه اگر (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    اگر (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        برای (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.افزودن_back(hier.substr(start, dot - start));
        }
    }
    parts.افزودن_back(hier.substr(start));
    بازگشت parts;
}

string duName(ثابت VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ثابت string page = point.page();
    بازگشت page.substr(page.find('/') + 1);
}

تهی tallyPoint(TypeTally& tally, ثابت string& type, uint64_t شمار) {
    tally[type].addPoint(شمار > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
دوگانه pct(uint64_t hit, uint64_t total) {
    بازگشت total ? (100.0 * static_cast<دوگانه>(hit) / static_cast<دوگانه>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_مقدار_String(دوگانه مقدار, منطقی complete) {
    اگر (!complete && مقدار > 99.9) مقدار = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << مقدار << "%";
    بازگشت os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    بازگشت pct_مقدار_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
رده CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_شمار = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

عمومی:
    // ACCESSORS
    ثابت string& name() ثابت { بازگشت m_name; }
    منطقی covered() ثابت { بازگشت m_شمار >= m_atLeast; }
    // METHODS
    تهی addRecord(ثابت string& name, uint64_t شمار, uint64_t atLeast) {
        m_name = name;
        m_شمار += شمار;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
رده CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

عمومی:
    // ACCESSORS
    uint64_t weight() ثابت { بازگشت m_weight; }
    ثابت std::map<string, CgBin>& bins() ثابت { بازگشت m_bins; }
    // METHODS
    تهی addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ثابت string& identity) { بازگشت m_bins[identity]; }
};

// A covergroup
رده CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_اقلام;  // By hierarchy

عمومی:
    // ACCESSORS
    ثابت string& hier() ثابت { بازگشت m_hier; }
    uint64_t weight() ثابت { بازگشت m_weight; }
    ثابت std::map<string, CgItem>& اقلام() ثابت { بازگشت m_اقلام; }
    // METHODS
    تهی addRecord(ثابت string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ثابت string& hier) { بازگشت m_اقلام[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t کلید_عدد(ثابت string& مقدار) {
    بازگشت مقدار.empty() ? 1 : std::strtoull(مقدار.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
منطقی isScoreField(ثابت string& field) {
    برای (ثابت نویسه* ثابت keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        ثابت string prefix = string{"\001"} + keyp + "\002";
        اگر (field.compare(0, prefix.size(), prefix) == 0) بازگشت درست;
    }
    بازگشت نادرست;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(ثابت string& recordName) {
    string identity;
    string::size_type start = 0;
    وقتی (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        اگر (end == string::npos) end = recordName.size();
        ثابت string field = recordName.substr(start, end - start);
        اگر (!isScoreField(field)) identity += field;
        start = end;
    }
    بازگشت identity;
}

منطقی isCovergroup(ثابت VlcPoint& point) { بازگشت point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    برای (ثابت VlcPoints::ByName::مقدار_type& i : points) {
        ثابت VlcPoint& pt = points.point_عدد(i.second);
        اگر (!isCovergroup(pt)) ادامه;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ثابت string hier = pt.hier();
        string bin = pt.bin();
        اگر (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        ثابت string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), کلید_عدد(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(کلید_عدد(pt.weight()));
        اگر (!pt.binType().empty()) ادامه;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.شمار(), کلید_عدد(pt.thresh()));
    }
    بازگشت groups;
}

Tally itemTally(ثابت CgItem& item) {
    Tally tally;
    برای (ثابت std::pair<ثابت string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    بازگشت tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(ثابت CgGroup& group, منطقی* contributesp = nullptr) {
    Tally tally;
    دوگانه weighted = 0.0;
    دوگانه weights = 0.0;
    برای (ثابت std::pair<ثابت string, CgItem>& it : group.اقلام()) {
        ثابت Tally item = itemTally(it.second);
        tally.addPoints(item);
        اگر (!item.total()) ادامه;  // Excluded from both sums
        weighted += static_cast<دوگانه>(it.second.weight()) * item.score();
        weights += static_cast<دوگانه>(it.second.weight());
    }
    اگر (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    بازگشت tally;
}

Tally groupsTally(ثابت std::vector<ثابت CgGroup*>& groups) {
    Tally tally;
    دوگانه weighted = 0.0;
    دوگانه weights = 0.0;
    منطقی anyWeight = نادرست;
    برای (ثابت CgGroup* ثابت groupp : groups) {
        منطقی contributes = نادرست;
        ثابت Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        اگر (groupp->weight()) anyWeight = درست;
        اگر (!contributes) ادامه;
        weighted += static_cast<دوگانه>(groupp->weight()) * group.score();
        weights += static_cast<دوگانه>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    بازگشت tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ثابت CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<ثابت CgGroup*>> گره_Groups;
    برای (ثابت CgGroups::مقدار_type& it : groups) {
        ثابت CgGroup& group = it.second;
        string path;
        برای (ثابت string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            گره_Groups[path].افزودن_back(&group);
        }
        برای (ثابت std::pair<ثابت string, CgItem>& item : group.اقلام()) {
            tallies[item.first] = itemTally(item.second);
            برای (ثابت std::pair<ثابت string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    برای (ثابت std::pair<ثابت string, std::vector<ثابت CgGroup*>>& it : گره_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    بازگشت tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
تهی printIndent(size_t indent) {
    برای (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

تهی printTallyRow(ثابت string& type, ثابت Tally& tally, size_t indent, size_t typeWidth,
                   size_t شمار_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ثابت string percent = tally.scored() ? pct_مقدار_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::چپ << std::setw(typeWidth) << type << " : " << std::راست << std::fixed
              << std::setw(6) << percent << " (" << std::setw(شمار_Width) << tally.hit() << "/"
              << std::setw(شمار_Width) << tally.total() << ")\n";
}

size_t شمار_Width(ثابت TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    برای (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    بازگشت width;
}

size_t typeWidth(ثابت TypeTally& tally) {
    size_t typeWidth = 0;
    برای (ثابت نویسه* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    برای (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    بازگشت typeWidth;
}

تهی printTypeTally(ثابت TypeTally& tally, size_t indent, منطقی includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ثابت size_t typWidth = typeWidth(tally);
    ثابت size_t cntWidth = شمار_Width(tally);
    برای (ثابت نویسه* ثابت typep : s_orderedTypes) {
        ثابت string type = typep;
        ثابت TypeTally::const_iterator it = tally.find(type);
        اگر (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } وگرنه اگر (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    برای (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        اگر (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
تهی printCovergroupTallies(ثابت std::map<string, Tally>& tallies, صحیح levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    برای (ثابت std::pair<ثابت string, Tally>& it : tallies) {
        اگر (levels >= 0 && static_cast<صحیح>(splitHier(it.first).size()) > levels + 1) ادامه;
        shown.درج(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    ثابت size_t cntWidth = شمار_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    برای (ثابت std::pair<ثابت string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

تهی Vlc_بالاترین::readCoverage(ثابت string& filename, منطقی nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    اگر (!is) {
        اگر (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        بازگشت;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ثابت testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    وقتی (!is.eof()) {
        ثابت string سطر = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        اگر (سطر[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ثابت string::size_type secspace = سطر.rfind("' ");
            اگر (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                ادامه;
            }
            ثابت string point = سطر.substr(3, secspace - 3);
            اگر (!opt.isTypeMatch(point.c_str())) ادامه;

            ثابت uint64_t hits = std::atoll(سطر.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ثابت uint64_t pointnum = points().findAddPoint(point, hits);
            اگر (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ثابت uint64_t needed = std::max(
                    VlcBuckets::sufficient(), کلید_عدد(points().point_عدد(pointnum).thresh()));
                اگر (hits >= needed) {
                    points().point_عدد(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

تهی Vlc_بالاترین::writeCoverage(ثابت string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    اگر (!os) {
        v3fatal("Can't write file: " << filename);
        بازگشت;
    }

    os << "# SystemC::Coverage-3\n";
    برای (ثابت auto& i : m_points) {
        ثابت VlcPoint& point = m_points.point_عدد(i.second);
        os << "C '" << point.name() << "' " << point.شمار() << '\n';
    }
}

تهی Vlc_بالاترین::writeInfo(ثابت string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    اگر (!os) {
        v3fatal("Can't write file: " << filename);
        بازگشت;
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
    برای (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& سطرها = source.سطرها();
        صحیح branchesFound = 0;
        صحیح branchesHit = 0;
        برای (auto& li : سطرها) {
            Vlc_Source_شمار& sc = li.second;
            uint64_t da_شمار = 0;
            std::vector<ثابت VlcPoint*> infoPoints;
            برای (ثابت auto& point : sc.points()) {
                da_شمار = std::max(da_شمار, point->شمار());
                infoPoints.افزودن_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_شمار << "\n";
            اگر (infoPoints.size() <= 1) ادامه;
            branchesFound += static_cast<صحیح>(infoPoints.size());
            صحیح point_رقم_2 = 0;
            برای (ثابت VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                اگر (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } وگرنه اگر (point->comment().empty()) {
                    os << "0,";
                    os << point_رقم_2;
                } وگرنه {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->شمار() << "\n";

                branchesHit += opt.شمار_Ok(point->شمار());
                ++point_رقم_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

ساختار CmpComputrons final {
    منطقی operator()(ثابت VlcTest* lhsp, ثابت VlcTest* rhsp) ثابت {
        اگر (lhsp->computrons() != rhsp->computrons()) {
            بازگشت lhsp->computrons() < rhsp->computrons();
        }
        بازگشت lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

تهی Vlc_بالاترین::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    برای (ثابت auto& testp : m_tests) {
        اگر (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.افزودن_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    برای (ثابت auto& i : m_points) {
        ثابت VlcPoint* ثابت pointp = &points().point_عدد(i.second);
        // If any tests hit this point, then we'll need to cover it.
        اگر (pointp->testsCovering()) remaining.addData(pointp->point_رقم(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    وقتی (درست) {
        اگر (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        برای (ثابت auto& testp : bytime) {
            اگر (!testp->rank()) {
                ثابت uint64_t remain = testp->buckets().data_برداشت_شمار(remaining);
                اگر (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        اگر (VlcTest* ثابت testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } وگرنه {
            بشکن;  // No test covering more stuff found
        }
    }
}

تهی Vlc_بالاترین::annotateCalc() {
    // Calculate per-line information into filedata structure
    برای (ثابت auto& i : m_points) {
        ثابت VlcPoint& point = m_points.point_عدد(i.second);
        ثابت string filename = point.filename();
        ثابت صحیح lineno = point.lineno();
        اگر (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.شمار() << " " << point.linescov());
            // Base coverage
            source.درج_Point(lineno, &point);
            // Additional lines covered by this statement
            منطقی range = نادرست;
            صحیح start = 0;
            صحیح end = 0;
            ثابت string linescov = point.linescov();
            برای (ثابت نویسه* covp = linescov.c_str(); درست; ++covp) {
                اگر (!*covp || *covp == ',') {  // Ending
                    برای (صحیح lni = start; start && lni <= end; ++lni) {
                        source.درج_Point(lni, &point);
                    }
                    اگر (!*covp) بشکن;
                    start = 0;  // Prep for next
                    end = 0;
                    range = نادرست;
                } وگرنه اگر (*covp == '-') {
                    range = درست;
                } وگرنه اگر (std::isdigit(*covp)) {
                    ثابت نویسه* ثابت digitsp = covp;
                    وقتی (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    اگر (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

تهی Vlc_بالاترین::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    صحیح totCases = 0;
    صحیح totOk = 0;
    برای (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        اگر (opt.annotateAll()) source.needed(درست);
        ثابت VlcSource::LinenoMap& سطرها = source.سطرها();
        برای (auto& li : سطرها) {
            ثابت Vlc_Source_شمار& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            اگر (opt.شمار_Ok(sc.min_شمار())) {
                ++totOk;
            } وگرنه {
                source.needed(درست);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    اگر (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

تهی Vlc_بالاترین::annotateOutputFiles(ثابت string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    برای (auto& si : m_sources) {
        VlcSource& source = si.second;
        اگر (!source.needed()) ادامه;
        ثابت string filename = source.name();
        ثابت string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        اگر (!is) {
            v3error("Can't read annotation file: " << filename);
            بازگشت;
        }

        std::ofstream os{outfilename.c_str()};
        اگر (!os) {
            v3error("Can't write file: " << outfilename);
            بازگشت;
        }

        os << "//      // verilator_coverage annotation\n";

        صحیح lineno = 0;
        وقتی (!is.eof()) {
            lineno++;
            ثابت std::string سطر = V3Os::getline(is);

            VlcSource::LinenoMap& سطرها = source.سطرها();
            ثابت auto lit = سطرها.find(lineno);
            اگر (lit == سطرها.end()) {
                os << "        " << سطر << '\n';
            } وگرنه {
                Vlc_Source_شمار& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ثابت منطقی minOk = opt.شمار_Ok(sc.min_شمار());
                ثابت منطقی maxOk = opt.شمار_Ok(sc.max_شمار());
                اگر (minOk) {
                    os << " ";
                } وگرنه اگر (maxOk) {
                    os << "~";
                } وگرنه {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_شمار() << " " << سطر << '\n';

                اگر (opt.annotatePoints()) {
                    برای (ثابت auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                منطقی printedFsmHeader = نادرست;
                برای (ثابت auto& pit : sc.points()) {
                    اگر (!pit->isFsmState() && !pit->isFsmArc()) ادامه;
                    اگر (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = درست;
                    }
                    os << (opt.شمار_Ok(pit->شمار()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->شمار() << "        ";
                    اگر (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        اگر (pit->شمار() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } وگرنه اگر (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } وگرنه {
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

تهی Vlc_بالاترین::annotate(ثابت string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

تهی Vlc_بالاترین::printTypeSummary() {
    TypeTally tally;
    برای (VlcPoints::ByName::مقدار_type& i : m_points) {
        ثابت VlcPoint& pt = m_points.point_عدد(i.second);
        اگر (isCovergroup(pt)) ادامه;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.شمار());
    }
    ثابت CgGroups groups = covergroups(m_points);
    اگر (!groups.empty()) {
        std::vector<ثابت CgGroup*> groupps;
        برای (ثابت CgGroups::مقدار_type& it : groups) groupps.افزودن_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    اگر (tally.empty()) بازگشت;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, درست);
}

تهی Vlc_بالاترین::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    منطقی hasHier = نادرست;
    منطقی hasCollapsedHier = نادرست;
    برای (VlcPoints::ByName::مقدار_type& i : m_points) {
        ثابت VlcPoint& pt = m_points.point_عدد(i.second);
        اگر (isCovergroup(pt)) ادامه;  // Reported below, by covergroup
        ثابت string hier = reportHier(pt);
        اگر (hier.empty()) ادامه;
        hasHier = درست;
        اگر (isCollapsedHier(hier)) hasCollapsedHier = درست;
        ثابت string type = displayType(pt);
        ثابت std::vector<string> parts = splitHier(hier);
        string path;
        برای (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.شمار());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.شمار());
    }
    ثابت std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    اگر (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        بازگشت;
    }
    اگر (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        بازگشت;
    }

    ثابت صحیح levels = opt.reportLevels();
    اگر (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    برای (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        ثابت std::vector<string> parts = splitHier(it->first);
        اگر (levels >= 0 && static_cast<صحیح>(parts.size()) > levels + 1) ادامه;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, نادرست);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    برای (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, نادرست);
    }
    اگر (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
