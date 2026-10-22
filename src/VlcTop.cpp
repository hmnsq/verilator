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

#अन्तर्भाव "VlcTop.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Os.h"

#अन्तर्भाव "VlcOptions.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <fstream>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <sstream>
#अन्तर्भाव <string>
#अन्तर्भाव <vector>

//######################################################################

नामाकाश {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
वर्ग Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    तार्किक m_scored = असत्य;  // Coverage is m_score, not the ratio of the points
    द्विगुण m_score = 0.0;  // Percent coverage, if m_scored

सार्वजनिक:
    // ACCESSORS
    uint64_t hit() अचर { फर्काऊ m_hit; }
    uint64_t total() अचर { फर्काऊ m_total; }
    तार्किक scored() अचर { फर्काऊ m_scored; }
    द्विगुण score() अचर { फर्काऊ m_score; }
    शून्य score(द्विगुण मान) {
        m_scored = सत्य;
        m_score = मान;
    }
    // METHODS
    शून्य addPoint(तार्किक covered) {
        यदि (covered) ++m_hit;
        ++m_total;
    }
    शून्य addPoints(अचर Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

स्थिर constexpr अचर वर्ण* अचर s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
स्थिर constexpr अचर वर्ण* अचर s_covergroupType = "covergroup";
स्थिर constexpr माप_प्रकार s_summaryIndent = 2;
स्थिर constexpr माप_प्रकार s_reportRowIndent = 4;

string displayType(अचर VlcPoint& point) {
    अचर string type = point.type();
    फर्काऊ type.empty() ? "point" : type;
}

तार्किक isCollapsedHier(अचर string& hier) {
    फर्काऊ hier.find('*') != string::npos || hier.find('?') != string::npos;
}

तार्किक isOrderedType(अचर string& type) {
    लागि (अचर वर्ण* अचर typep : s_orderedTypes) {
        यदि (type == typep) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}

string reportHier(अचर VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    यदि (point.isFsmState() || point.isFsmArc()) {
        अचर string fsmVar = point.fsmVarName();
        फर्काऊ fsmVar.substr(0, fsmVar.rfind('.'));
    }
    फर्काऊ point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(अचर string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    पूर्णाङ्क depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    जबसम्म (pos < hier.size()) {
        अचर वर्ण c = hier[pos];
        यदि (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            जारी;
        } अन्यथा यदि (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } अन्यथा यदि (!depth && c == '.') {
            parts.थप_back(hier.substr(start, pos - start));
            start = pos + 1;
        } अन्यथा यदि (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            जारी;
        } अन्यथा यदि (depth && c == '(') {
            ++depth;
        } अन्यथा यदि (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    यदि (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        लागि (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.थप_back(hier.substr(start, dot - start));
        }
    }
    parts.थप_back(hier.substr(start));
    फर्काऊ parts;
}

string duName(अचर VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    अचर string page = point.page();
    फर्काऊ page.substr(page.find('/') + 1);
}

शून्य tallyPoint(TypeTally& tally, अचर string& type, uint64_t गन्ती) {
    tally[type].addPoint(गन्ती > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
द्विगुण pct(uint64_t hit, uint64_t total) {
    फर्काऊ total ? (100.0 * static_cast<द्विगुण>(hit) / static_cast<द्विगुण>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_मान_String(द्विगुण मान, तार्किक complete) {
    यदि (!complete && मान > 99.9) मान = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << मान << "%";
    फर्काऊ os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    फर्काऊ pct_मान_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
वर्ग CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_गन्ती = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

सार्वजनिक:
    // ACCESSORS
    अचर string& name() अचर { फर्काऊ m_name; }
    तार्किक covered() अचर { फर्काऊ m_गन्ती >= m_atLeast; }
    // METHODS
    शून्य addRecord(अचर string& name, uint64_t गन्ती, uint64_t atLeast) {
        m_name = name;
        m_गन्ती += गन्ती;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
वर्ग CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

सार्वजनिक:
    // ACCESSORS
    uint64_t weight() अचर { फर्काऊ m_weight; }
    अचर std::map<string, CgBin>& bins() अचर { फर्काऊ m_bins; }
    // METHODS
    शून्य addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(अचर string& identity) { फर्काऊ m_bins[identity]; }
};

// A covergroup
वर्ग CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_वस्तुहरू;  // By hierarchy

सार्वजनिक:
    // ACCESSORS
    अचर string& hier() अचर { फर्काऊ m_hier; }
    uint64_t weight() अचर { फर्काऊ m_weight; }
    अचर std::map<string, CgItem>& वस्तुहरू() अचर { फर्काऊ m_वस्तुहरू; }
    // METHODS
    शून्य addRecord(अचर string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(अचर string& hier) { फर्काऊ m_वस्तुहरू[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t साँचो_सङ्ख्या(अचर string& मान) {
    फर्काऊ मान.empty() ? 1 : std::strtoull(मान.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
तार्किक isScoreField(अचर string& field) {
    लागि (अचर वर्ण* अचर keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        अचर string prefix = string{"\001"} + keyp + "\002";
        यदि (field.compare(0, prefix.size(), prefix) == 0) फर्काऊ सत्य;
    }
    फर्काऊ असत्य;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(अचर string& recordName) {
    string identity;
    string::size_type start = 0;
    जबसम्म (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        यदि (end == string::npos) end = recordName.size();
        अचर string field = recordName.substr(start, end - start);
        यदि (!isScoreField(field)) identity += field;
        start = end;
    }
    फर्काऊ identity;
}

तार्किक isCovergroup(अचर VlcPoint& point) { फर्काऊ point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    लागि (अचर VlcPoints::ByName::मान_type& i : points) {
        अचर VlcPoint& pt = points.point_सङ्ख्या(i.second);
        यदि (!isCovergroup(pt)) जारी;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        अचर string hier = pt.hier();
        string bin = pt.bin();
        यदि (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        अचर string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), साँचो_सङ्ख्या(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(साँचो_सङ्ख्या(pt.weight()));
        यदि (!pt.binType().empty()) जारी;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.गन्ती(), साँचो_सङ्ख्या(pt.thresh()));
    }
    फर्काऊ groups;
}

Tally itemTally(अचर CgItem& item) {
    Tally tally;
    लागि (अचर std::pair<अचर string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    फर्काऊ tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(अचर CgGroup& group, तार्किक* contributesp = nullptr) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    लागि (अचर std::pair<अचर string, CgItem>& it : group.वस्तुहरू()) {
        अचर Tally item = itemTally(it.second);
        tally.addPoints(item);
        यदि (!item.total()) जारी;  // Excluded from both sums
        weighted += static_cast<द्विगुण>(it.second.weight()) * item.score();
        weights += static_cast<द्विगुण>(it.second.weight());
    }
    यदि (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    फर्काऊ tally;
}

Tally groupsTally(अचर std::vector<अचर CgGroup*>& groups) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    तार्किक anyWeight = असत्य;
    लागि (अचर CgGroup* अचर groupp : groups) {
        तार्किक contributes = असत्य;
        अचर Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        यदि (groupp->weight()) anyWeight = सत्य;
        यदि (!contributes) जारी;
        weighted += static_cast<द्विगुण>(groupp->weight()) * group.score();
        weights += static_cast<द्विगुण>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    फर्काऊ tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(अचर CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<अचर CgGroup*>> गाँठो_Groups;
    लागि (अचर CgGroups::मान_type& it : groups) {
        अचर CgGroup& group = it.second;
        string path;
        लागि (अचर string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            गाँठो_Groups[path].थप_back(&group);
        }
        लागि (अचर std::pair<अचर string, CgItem>& item : group.वस्तुहरू()) {
            tallies[item.first] = itemTally(item.second);
            लागि (अचर std::pair<अचर string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    लागि (अचर std::pair<अचर string, std::vector<अचर CgGroup*>>& it : गाँठो_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    फर्काऊ tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
शून्य printIndent(माप_प्रकार indent) {
    लागि (माप_प्रकार i = 0; i < indent; ++i) std::cout << ' ';
}

शून्य printTallyRow(अचर string& type, अचर Tally& tally, माप_प्रकार indent, माप_प्रकार typeWidth,
                   माप_प्रकार गन्ती_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    अचर string percent = tally.scored() ? pct_मान_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::बायाँ << std::setw(typeWidth) << type << " : " << std::दायाँ << std::fixed
              << std::setw(6) << percent << " (" << std::setw(गन्ती_Width) << tally.hit() << "/"
              << std::setw(गन्ती_Width) << tally.total() << ")\n";
}

माप_प्रकार गन्ती_Width(अचर TypeTally& tally) {
    माप_प्रकार width = cvtToStr(0).size();
    लागि (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    फर्काऊ width;
}

माप_प्रकार typeWidth(अचर TypeTally& tally) {
    माप_प्रकार typeWidth = 0;
    लागि (अचर वर्ण* अचर typep : s_orderedTypes) {
        अचर string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    लागि (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    फर्काऊ typeWidth;
}

शून्य printTypeTally(अचर TypeTally& tally, माप_प्रकार indent, तार्किक includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    अचर माप_प्रकार typWidth = typeWidth(tally);
    अचर माप_प्रकार cntWidth = गन्ती_Width(tally);
    लागि (अचर वर्ण* अचर typep : s_orderedTypes) {
        अचर string type = typep;
        अचर TypeTally::const_iterator it = tally.find(type);
        यदि (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } अन्यथा यदि (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    लागि (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        यदि (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
शून्य printCovergroupTallies(अचर std::map<string, Tally>& tallies, पूर्णाङ्क levels) {
    std::map<string, Tally> shown;
    माप_प्रकार nameWidth = 0;
    लागि (अचर std::pair<अचर string, Tally>& it : tallies) {
        यदि (levels >= 0 && static_cast<पूर्णाङ्क>(splitHier(it.first).size()) > levels + 1) जारी;
        shown.घुसाउ(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    अचर माप_प्रकार cntWidth = गन्ती_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    लागि (अचर std::pair<अचर string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

शून्य Vlc_शीर्ष::readCoverage(अचर string& filename, तार्किक nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    यदि (!is) {
        यदि (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        फर्काऊ;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* अचर testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    जबसम्म (!is.eof()) {
        अचर string पङ्क्ति = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        यदि (पङ्क्ति[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            अचर string::size_type secspace = पङ्क्ति.rfind("' ");
            यदि (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                जारी;
            }
            अचर string point = पङ्क्ति.substr(3, secspace - 3);
            यदि (!opt.isTypeMatch(point.c_str())) जारी;

            अचर uint64_t hits = std::atoll(पङ्क्ति.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            अचर uint64_t pointnum = points().findAddPoint(point, hits);
            यदि (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                अचर uint64_t needed = std::max(
                    VlcBuckets::sufficient(), साँचो_सङ्ख्या(points().point_सङ्ख्या(pointnum).thresh()));
                यदि (hits >= needed) {
                    points().point_सङ्ख्या(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

शून्य Vlc_शीर्ष::writeCoverage(अचर string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    यदि (!os) {
        v3fatal("Can't write file: " << filename);
        फर्काऊ;
    }

    os << "# SystemC::Coverage-3\n";
    लागि (अचर स्वतः& i : m_points) {
        अचर VlcPoint& point = m_points.point_सङ्ख्या(i.second);
        os << "C '" << point.name() << "' " << point.गन्ती() << '\n';
    }
}

शून्य Vlc_शीर्ष::writeInfo(अचर string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    यदि (!os) {
        v3fatal("Can't write file: " << filename);
        फर्काऊ;
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
    लागि (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& पङ्क्तिहरू = source.पङ्क्तिहरू();
        पूर्णाङ्क branchesFound = 0;
        पूर्णाङ्क branchesHit = 0;
        लागि (स्वतः& li : पङ्क्तिहरू) {
            Vlc_Source_गन्ती& sc = li.second;
            uint64_t da_गन्ती = 0;
            std::vector<अचर VlcPoint*> infoPoints;
            लागि (अचर स्वतः& point : sc.points()) {
                da_गन्ती = std::max(da_गन्ती, point->गन्ती());
                infoPoints.थप_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_गन्ती << "\n";
            यदि (infoPoints.size() <= 1) जारी;
            branchesFound += static_cast<पूर्णाङ्क>(infoPoints.size());
            पूर्णाङ्क point_अङ्क_2 = 0;
            लागि (अचर VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                यदि (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } अन्यथा यदि (point->comment().empty()) {
                    os << "0,";
                    os << point_अङ्क_2;
                } अन्यथा {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->गन्ती() << "\n";

                branchesHit += opt.गन्ती_Ok(point->गन्ती());
                ++point_अङ्क_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

संरचना CmpComputrons final {
    तार्किक चालक()(अचर VlcTest* lhsp, अचर VlcTest* rhsp) अचर {
        यदि (lhsp->computrons() != rhsp->computrons()) {
            फर्काऊ lhsp->computrons() < rhsp->computrons();
        }
        फर्काऊ lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

शून्य Vlc_शीर्ष::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    लागि (अचर स्वतः& testp : m_tests) {
        यदि (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.थप_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    लागि (अचर स्वतः& i : m_points) {
        अचर VlcPoint* अचर pointp = &points().point_सङ्ख्या(i.second);
        // If any tests hit this point, then we'll need to cover it.
        यदि (pointp->testsCovering()) remaining.addData(pointp->point_अङ्क(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    जबसम्म (सत्य) {
        यदि (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        लागि (अचर स्वतः& testp : bytime) {
            यदि (!testp->rank()) {
                अचर uint64_t remain = testp->buckets().data_झिक_गन्ती(remaining);
                यदि (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        यदि (VlcTest* अचर testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } अन्यथा {
            रोक;  // No test covering more stuff found
        }
    }
}

शून्य Vlc_शीर्ष::annotateCalc() {
    // Calculate per-line information into filedata structure
    लागि (अचर स्वतः& i : m_points) {
        अचर VlcPoint& point = m_points.point_सङ्ख्या(i.second);
        अचर string filename = point.filename();
        अचर पूर्णाङ्क lineno = point.lineno();
        यदि (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.गन्ती() << " " << point.linescov());
            // Base coverage
            source.घुसाउ_Point(lineno, &point);
            // Additional lines covered by this statement
            तार्किक range = असत्य;
            पूर्णाङ्क start = 0;
            पूर्णाङ्क end = 0;
            अचर string linescov = point.linescov();
            लागि (अचर वर्ण* covp = linescov.c_str(); सत्य; ++covp) {
                यदि (!*covp || *covp == ',') {  // Ending
                    लागि (पूर्णाङ्क lni = start; start && lni <= end; ++lni) {
                        source.घुसाउ_Point(lni, &point);
                    }
                    यदि (!*covp) रोक;
                    start = 0;  // Prep for next
                    end = 0;
                    range = असत्य;
                } अन्यथा यदि (*covp == '-') {
                    range = सत्य;
                } अन्यथा यदि (std::है_अंक(*covp)) {
                    अचर वर्ण* अचर digitsp = covp;
                    जबसम्म (std::है_अंक(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    यदि (!range) start = std::म_से_प(digitsp);
                    end = std::म_से_प(digitsp);
                }
            }
        }
    }
}

शून्य Vlc_शीर्ष::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    पूर्णाङ्क totCases = 0;
    पूर्णाङ्क totOk = 0;
    लागि (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        यदि (opt.annotateAll()) source.needed(सत्य);
        अचर VlcSource::LinenoMap& पङ्क्तिहरू = source.पङ्क्तिहरू();
        लागि (स्वतः& li : पङ्क्तिहरू) {
            अचर Vlc_Source_गन्ती& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            यदि (opt.गन्ती_Ok(sc.min_गन्ती())) {
                ++totOk;
            } अन्यथा {
                source.needed(सत्य);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    यदि (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

शून्य Vlc_शीर्ष::annotateOutputFiles(अचर string& स_नाम) {
    // Create if uncreated, ignore errors
    V3Os::createDir(स_नाम);
    लागि (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        यदि (!source.needed()) जारी;
        अचर string filename = source.name();
        अचर string outfilename = स_नाम + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        यदि (!is) {
            v3error("Can't read annotation file: " << filename);
            फर्काऊ;
        }

        std::ofstream os{outfilename.c_str()};
        यदि (!os) {
            v3error("Can't write file: " << outfilename);
            फर्काऊ;
        }

        os << "//      // verilator_coverage annotation\n";

        पूर्णाङ्क lineno = 0;
        जबसम्म (!is.eof()) {
            lineno++;
            अचर std::string पङ्क्ति = V3Os::getline(is);

            VlcSource::LinenoMap& पङ्क्तिहरू = source.पङ्क्तिहरू();
            अचर स्वतः lit = पङ्क्तिहरू.find(lineno);
            यदि (lit == पङ्क्तिहरू.end()) {
                os << "        " << पङ्क्ति << '\n';
            } अन्यथा {
                Vlc_Source_गन्ती& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                अचर तार्किक minOk = opt.गन्ती_Ok(sc.min_गन्ती());
                अचर तार्किक maxOk = opt.गन्ती_Ok(sc.max_गन्ती());
                यदि (minOk) {
                    os << " ";
                } अन्यथा यदि (maxOk) {
                    os << "~";
                } अन्यथा {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_गन्ती() << " " << पङ्क्ति << '\n';

                यदि (opt.annotatePoints()) {
                    लागि (अचर स्वतः& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                तार्किक printedFsmHeader = असत्य;
                लागि (अचर स्वतः& pit : sc.points()) {
                    यदि (!pit->isFsmState() && !pit->isFsmArc()) जारी;
                    यदि (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = सत्य;
                    }
                    os << (opt.गन्ती_Ok(pit->गन्ती()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->गन्ती() << "        ";
                    यदि (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        यदि (pit->गन्ती() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } अन्यथा यदि (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } अन्यथा {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        यदि (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

शून्य Vlc_शीर्ष::annotate(अचर string& स_नाम) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(स_नाम);
}

शून्य Vlc_शीर्ष::printTypeSummary() {
    TypeTally tally;
    लागि (VlcPoints::ByName::मान_type& i : m_points) {
        अचर VlcPoint& pt = m_points.point_सङ्ख्या(i.second);
        यदि (isCovergroup(pt)) जारी;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.गन्ती());
    }
    अचर CgGroups groups = covergroups(m_points);
    यदि (!groups.empty()) {
        std::vector<अचर CgGroup*> groupps;
        लागि (अचर CgGroups::मान_type& it : groups) groupps.थप_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    यदि (tally.empty()) फर्काऊ;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, सत्य);
}

शून्य Vlc_शीर्ष::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    तार्किक hasHier = असत्य;
    तार्किक hasCollapsedHier = असत्य;
    लागि (VlcPoints::ByName::मान_type& i : m_points) {
        अचर VlcPoint& pt = m_points.point_सङ्ख्या(i.second);
        यदि (isCovergroup(pt)) जारी;  // Reported below, by covergroup
        अचर string hier = reportHier(pt);
        यदि (hier.empty()) जारी;
        hasHier = सत्य;
        यदि (isCollapsedHier(hier)) hasCollapsedHier = सत्य;
        अचर string type = displayType(pt);
        अचर std::vector<string> parts = splitHier(hier);
        string path;
        लागि (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.गन्ती());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.गन्ती());
    }
    अचर std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    यदि (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        फर्काऊ;
    }
    यदि (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        फर्काऊ;
    }

    अचर पूर्णाङ्क levels = opt.reportLevels();
    यदि (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    लागि (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        अचर std::vector<string> parts = splitHier(it->first);
        यदि (levels >= 0 && static_cast<पूर्णाङ्क>(parts.size()) > levels + 1) जारी;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, असत्य);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    लागि (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, असत्य);
    }
    यदि (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
