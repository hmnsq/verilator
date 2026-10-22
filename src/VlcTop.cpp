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

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "VlcTop.h"

#समावेश "V3Error.h"
#समावेश "V3Os.h"

#समावेश "VlcOptions.h"

#समावेश <algorithm>
#समावेश <fstream>
#समावेश <iomanip>
#समावेश <map>
#समावेश <set>
#समावेश <sstream>
#समावेश <string>
#समावेश <vector>

//######################################################################

namespace {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
श्रेणी Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_योग = 0;  // Points
    bool m_scored = false;  // Coverage is m_score, not the ratio of the points
    द्विगुन m_score = 0.0;  // Percent coverage, if m_scored

खुला:
    // ACCESSORS
    uint64_t hit() स्थिर { वापस m_hit; }
    uint64_t योग() स्थिर { वापस m_योग; }
    bool scored() स्थिर { वापस m_scored; }
    द्विगुन score() स्थिर { वापस m_score; }
    व्योम score(द्विगुन मूल्य) {
        m_scored = true;
        m_score = मूल्य;
    }
    // METHODS
    व्योम addPoint(bool covered) {
        अगर (covered) ++m_hit;
        ++m_योग;
    }
    व्योम addPoints(स्थिर Tally& other) {
        m_hit += other.m_hit;
        m_योग += other.m_योग;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

अटल constexpr स्थिर अक्षर* स्थिर s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
अटल constexpr स्थिर अक्षर* स्थिर s_covergroupType = "covergroup";
अटल constexpr माप_प्रकार s_summaryIndent = 2;
अटल constexpr माप_प्रकार s_reportRowIndent = 4;

string displayType(स्थिर VlcPoint& point) {
    स्थिर string type = point.type();
    वापस type.empty() ? "point" : type;
}

bool isCollapsedHier(स्थिर string& hier) {
    वापस hier.find('*') != string::npos || hier.find('?') != string::npos;
}

bool isOrderedType(स्थिर string& type) {
    क्रम (स्थिर अक्षर* स्थिर typep : s_orderedTypes) {
        अगर (type == typep) वापस true;
    }
    वापस false;
}

string reportHier(स्थिर VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    अगर (point.isFsmState() || point.isFsmArc()) {
        स्थिर string fsmVar = point.fsmVarName();
        वापस fsmVar.substr(0, fsmVar.rfind('.'));
    }
    वापस point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(स्थिर string& hier) {
    std::vector<string> parts;
    string::आकार_type start = 0;
    पूर्णांक depth = 0;  // Of the parentheses of the values of parameters
    string::आकार_type pos = 0;
    जबतक (pos < hier.आकार()) {
        स्थिर अक्षर c = hier[pos];
        अगर (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            जारी;
        } अन्यथा अगर (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } अन्यथा अगर (!depth && c == '.') {
            parts.धकेलो_back(hier.substr(start, pos - start));
            start = pos + 1;
        } अन्यथा अगर (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            जारी;
        } अन्यथा अगर (depth && c == '(') {
            ++depth;
        } अन्यथा अगर (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    अगर (depth || pos != hier.आकार()) {  // Unbalanced
        parts.सफा();
        start = 0;
        क्रम (string::आकार_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.धकेलो_back(hier.substr(start, dot - start));
        }
    }
    parts.धकेलो_back(hier.substr(start));
    वापस parts;
}

string duName(स्थिर VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    स्थिर string page = point.page();
    वापस page.substr(page.find('/') + 1);
}

व्योम tallyPoint(TypeTally& tally, स्थिर string& type, uint64_t गिनती) {
    tally[type].addPoint(गिनती > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
द्विगुन pct(uint64_t hit, uint64_t योग) {
    वापस योग ? (100.0 * static_cast<द्विगुन>(hit) / static_cast<द्विगुन>(योग)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_मूल्य_String(द्विगुन मूल्य, bool complete) {
    अगर (!complete && मूल्य > 99.9) मूल्य = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::जड़ << std::रखो_सटीक(1)  // 1 matters, see above
       << मूल्य << "%";
    वापस os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t योग) {
    वापस pct_मूल्य_String(pct(hit, योग), hit >= योग);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
श्रेणी CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_गिनती = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

खुला:
    // ACCESSORS
    स्थिर string& name() स्थिर { वापस m_name; }
    bool covered() स्थिर { वापस m_गिनती >= m_atLeast; }
    // METHODS
    व्योम addRecord(स्थिर string& name, uint64_t गिनती, uint64_t atLeast) {
        m_name = name;
        m_गिनती += गिनती;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
श्रेणी CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

खुला:
    // ACCESSORS
    uint64_t weight() स्थिर { वापस m_weight; }
    स्थिर std::map<string, CgBin>& bins() स्थिर { वापस m_bins; }
    // METHODS
    व्योम addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(स्थिर string& identity) { वापस m_bins[identity]; }
};

// A covergroup
श्रेणी CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_वस्तुएँ;  // By hierarchy

खुला:
    // ACCESSORS
    स्थिर string& hier() स्थिर { वापस m_hier; }
    uint64_t weight() स्थिर { वापस m_weight; }
    स्थिर std::map<string, CgItem>& वस्तुएँ() स्थिर { वापस m_वस्तुएँ; }
    // METHODS
    व्योम addRecord(स्थिर string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(स्थिर string& hier) { वापस m_वस्तुएँ[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t कुंजी_संख्या(स्थिर string& मूल्य) {
    वापस मूल्य.empty() ? 1 : std::strtoull(मूल्य.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
bool isScoreField(स्थिर string& field) {
    क्रम (स्थिर अक्षर* स्थिर keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        स्थिर string prefix = string{"\001"} + keyp + "\002";
        अगर (field.compare(0, prefix.आकार(), prefix) == 0) वापस true;
    }
    वापस false;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(स्थिर string& recordName) {
    string identity;
    string::आकार_type start = 0;
    जबतक (start < recordName.आकार()) {
        string::आकार_type end = recordName.find('\001', start + 1);
        अगर (end == string::npos) end = recordName.आकार();
        स्थिर string field = recordName.substr(start, end - start);
        अगर (!isScoreField(field)) identity += field;
        start = end;
    }
    वापस identity;
}

bool isCovergroup(स्थिर VlcPoint& point) { वापस point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    क्रम (स्थिर VlcPoints::ByName::मूल्य_type& i : points) {
        स्थिर VlcPoint& pt = points.point_संख्या(i.second);
        अगर (!isCovergroup(pt)) जारी;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        स्थिर string hier = pt.hier();
        string bin = pt.bin();
        अगर (bin.empty() || bin.आकार() >= hier.आकार()) bin = hier.substr(hier.rfind('.') + 1);
        स्थिर string itemHier
            = hier.substr(0, hier.आकार() - std::min(hier.आकार(), bin.आकार() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), कुंजी_संख्या(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(कुंजी_संख्या(pt.weight()));
        अगर (!pt.binType().empty()) जारी;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.गिनती(), कुंजी_संख्या(pt.thresh()));
    }
    वापस groups;
}

Tally itemTally(स्थिर CgItem& item) {
    Tally tally;
    क्रम (स्थिर std::pair<स्थिर string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.योग() ? pct(tally.hit(), tally.योग()) : item.weight() ? 0.0 : 100.0);
    वापस tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(स्थिर CgGroup& group, bool* contributesp = nullptr) {
    Tally tally;
    द्विगुन weighted = 0.0;
    द्विगुन weights = 0.0;
    क्रम (स्थिर std::pair<स्थिर string, CgItem>& it : group.वस्तुएँ()) {
        स्थिर Tally item = itemTally(it.second);
        tally.addPoints(item);
        अगर (!item.योग()) जारी;  // Excluded from both sums
        weighted += static_cast<द्विगुन>(it.second.weight()) * item.score();
        weights += static_cast<द्विगुन>(it.second.weight());
    }
    अगर (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    वापस tally;
}

Tally groupsTally(स्थिर std::vector<स्थिर CgGroup*>& groups) {
    Tally tally;
    द्विगुन weighted = 0.0;
    द्विगुन weights = 0.0;
    bool anyWeight = false;
    क्रम (स्थिर CgGroup* स्थिर groupp : groups) {
        bool contributes = false;
        स्थिर Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        अगर (groupp->weight()) anyWeight = true;
        अगर (!contributes) जारी;
        weighted += static_cast<द्विगुन>(groupp->weight()) * group.score();
        weights += static_cast<द्विगुन>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    वापस tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(स्थिर CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<स्थिर CgGroup*>> गाँठ_Groups;
    क्रम (स्थिर CgGroups::मूल्य_type& it : groups) {
        स्थिर CgGroup& group = it.second;
        string path;
        क्रम (स्थिर string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            गाँठ_Groups[path].धकेलो_back(&group);
        }
        क्रम (स्थिर std::pair<स्थिर string, CgItem>& item : group.वस्तुएँ()) {
            tallies[item.first] = itemTally(item.second);
            क्रम (स्थिर std::pair<स्थिर string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    क्रम (स्थिर std::pair<स्थिर string, std::vector<स्थिर CgGroup*>>& it : गाँठ_Groups) {
        tallies[it.first]
            = it.second.आकार() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    वापस tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
व्योम printIndent(माप_प्रकार indent) {
    क्रम (माप_प्रकार i = 0; i < indent; ++i) std::अ_बाहर << ' ';
}

व्योम printTallyRow(स्थिर string& type, स्थिर Tally& tally, माप_प्रकार indent, माप_प्रकार typeWidth,
                   माप_प्रकार गिनती_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    स्थिर string percent = tally.scored() ? pct_मूल्य_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.योग());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::अ_बाहर << std::बाँए << std::रखो_चौड़ाई(typeWidth) << type << " : " << std::दाँए << std::जड़
              << std::रखो_चौड़ाई(6) << percent << " (" << std::रखो_चौड़ाई(गिनती_Width) << tally.hit() << "/"
              << std::रखो_चौड़ाई(गिनती_Width) << tally.योग() << ")\n";
}

माप_प्रकार गिनती_Width(स्थिर TypeTally& tally) {
    माप_प्रकार width = cvtToStr(0).आकार();
    क्रम (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).आकार());
        width = std::max(width, cvtToStr(it->second.योग()).आकार());
    }
    वापस width;
}

माप_प्रकार typeWidth(स्थिर TypeTally& tally) {
    माप_प्रकार typeWidth = 0;
    क्रम (स्थिर अक्षर* स्थिर typep : s_orderedTypes) {
        स्थिर string type = typep;
        typeWidth = std::max(typeWidth, type.आकार());
    }
    क्रम (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.आकार());
    }
    वापस typeWidth;
}

व्योम printTypeTally(स्थिर TypeTally& tally, माप_प्रकार indent, bool includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    स्थिर माप_प्रकार typWidth = typeWidth(tally);
    स्थिर माप_प्रकार cntWidth = गिनती_Width(tally);
    क्रम (स्थिर अक्षर* स्थिर typep : s_orderedTypes) {
        स्थिर string type = typep;
        स्थिर TypeTally::const_iterator it = tally.find(type);
        अगर (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } अन्यथा अगर (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    क्रम (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        अगर (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
व्योम printCovergroupTallies(स्थिर std::map<string, Tally>& tallies, पूर्णांक levels) {
    std::map<string, Tally> shown;
    माप_प्रकार nameWidth = 0;
    क्रम (स्थिर std::pair<स्थिर string, Tally>& it : tallies) {
        अगर (levels >= 0 && static_cast<पूर्णांक>(splitHier(it.first).आकार()) > levels + 1) जारी;
        shown.जोड़ो(it);
        nameWidth = std::max(nameWidth, it.first.आकार());
    }
    स्थिर माप_प्रकार cntWidth = गिनती_Width(shown);
    std::अ_बाहर << "Covergroup Coverage Summary:\n";
    क्रम (स्थिर std::pair<स्थिर string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

व्योम Vlc_शीर्ष::readCoverage(स्थिर string& filename, bool nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::अख_प्रवाह is{filename.c_str()};
    अगर (!is) {
        अगर (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        वापस;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* स्थिर testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    जबतक (!is.खप()) {
        स्थिर string पंक्ति = V3Os::लो_पंक्ति(is);
        ++lineno;
        // UINFO(9, " got " << line);
        अगर (पंक्ति[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            स्थिर string::आकार_type secspace = पंक्ति.rfind("' ");
            अगर (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                जारी;
            }
            स्थिर string point = पंक्ति.substr(3, secspace - 3);
            अगर (!opt.isTypeMatch(point.c_str())) जारी;

            स्थिर uint64_t hits = std::atoll(पंक्ति.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            स्थिर uint64_t pointnum = points().findAddPoint(point, hits);
            अगर (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                स्थिर uint64_t needed = std::max(
                    VlcBuckets::sufficient(), कुंजी_संख्या(points().point_संख्या(pointnum).thresh()));
                अगर (hits >= needed) {
                    points().point_संख्या(pointnum).testsCoveringInc();
                    testp->buckets().add_तथ्य(pointnum, hits);
                }
            }
        }
    }
}

व्योम Vlc_शीर्ष::writeCoverage(स्थिर string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::बख_प्रवाह os{filename.c_str()};
    अगर (!os) {
        v3fatal("Can't write file: " << filename);
        वापस;
    }

    os << "# SystemC::Coverage-3\n";
    क्रम (स्थिर स्वतः& i : m_points) {
        स्थिर VlcPoint& point = m_points.point_संख्या(i.second);
        os << "C '" << point.name() << "' " << point.गिनती() << '\n';
    }
}

व्योम Vlc_शीर्ष::writeInfo(स्थिर string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::बख_प्रवाह os{filename.c_str()};
    अगर (!os) {
        v3fatal("Can't write file: " << filename);
        वापस;
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
    क्रम (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& पंक्तियाँ = source.पंक्तियाँ();
        पूर्णांक branchesFound = 0;
        पूर्णांक branchesHit = 0;
        क्रम (स्वतः& li : पंक्तियाँ) {
            Vlc_Source_गिनती& sc = li.second;
            uint64_t da_गिनती = 0;
            std::vector<स्थिर VlcPoint*> infoPoints;
            क्रम (स्थिर स्वतः& point : sc.points()) {
                da_गिनती = std::max(da_गिनती, point->गिनती());
                infoPoints.धकेलो_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_गिनती << "\n";
            अगर (infoPoints.आकार() <= 1) जारी;
            branchesFound += static_cast<पूर्णांक>(infoPoints.आकार());
            पूर्णांक point_अंक_2 = 0;
            क्रम (स्थिर VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                अगर (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } अन्यथा अगर (point->comment().empty()) {
                    os << "0,";
                    os << point_अंक_2;
                } अन्यथा {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->गिनती() << "\n";

                branchesHit += opt.गिनती_Ok(point->गिनती());
                ++point_अंक_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

काष्ठा CmpComputrons final {
    bool चालक()(स्थिर VlcTest* lhsp, स्थिर VlcTest* rhsp) स्थिर {
        अगर (lhsp->computrons() != rhsp->computrons()) {
            वापस lhsp->computrons() < rhsp->computrons();
        }
        वापस lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

व्योम Vlc_शीर्ष::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    क्रम (स्थिर स्वतः& testp : m_tests) {
        अगर (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.धकेलो_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    क्रम (स्थिर स्वतः& i : m_points) {
        स्थिर VlcPoint* स्थिर pointp = &points().point_संख्या(i.second);
        // If any tests hit this point, then we'll need to cover it.
        अगर (pointp->testsCovering()) remaining.add_तथ्य(pointp->point_अंक(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    जबतक (true) {
        अगर (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        क्रम (स्थिर स्वतः& testp : bytime) {
            अगर (!testp->rank()) {
                स्थिर uint64_t remain = testp->buckets().तथ्य_निकालो_गिनती(remaining);
                अगर (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        अगर (VlcTest* स्थिर testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.or_तथ्य(bestTestp->buckets());
        } अन्यथा {
            अवरोध;  // No test covering more stuff found
        }
    }
}

व्योम Vlc_शीर्ष::annotateCalc() {
    // Calculate per-line information into filedata structure
    क्रम (स्थिर स्वतः& i : m_points) {
        स्थिर VlcPoint& point = m_points.point_संख्या(i.second);
        स्थिर string filename = point.filename();
        स्थिर पूर्णांक lineno = point.lineno();
        अगर (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.गिनती() << " " << point.linescov());
            // Base coverage
            source.जोड़ो_Point(lineno, &point);
            // Additional lines covered by this statement
            bool range = false;
            पूर्णांक start = 0;
            पूर्णांक end = 0;
            स्थिर string linescov = point.linescov();
            क्रम (स्थिर अक्षर* covp = linescov.c_str(); true; ++covp) {
                अगर (!*covp || *covp == ',') {  // Ending
                    क्रम (पूर्णांक lni = start; start && lni <= end; ++lni) {
                        source.जोड़ो_Point(lni, &point);
                    }
                    अगर (!*covp) अवरोध;
                    start = 0;  // Prep for next
                    end = 0;
                    range = false;
                } अन्यथा अगर (*covp == '-') {
                    range = true;
                } अन्यथा अगर (std::है_अंक(*covp)) {
                    स्थिर अक्षर* स्थिर digitsp = covp;
                    जबतक (std::है_अंक(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    अगर (!range) start = std::म_से_प(digitsp);
                    end = std::म_से_प(digitsp);
                }
            }
        }
    }
}

व्योम Vlc_शीर्ष::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    पूर्णांक totCases = 0;
    पूर्णांक totOk = 0;
    क्रम (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        अगर (opt.annotateAll()) source.needed(true);
        स्थिर VlcSource::LinenoMap& पंक्तियाँ = source.पंक्तियाँ();
        क्रम (स्वतः& li : पंक्तियाँ) {
            स्थिर Vlc_Source_गिनती& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            अगर (opt.गिनती_Ok(sc.min_गिनती())) {
                ++totOk;
            } अन्यथा {
                source.needed(true);
            }
        }
    }
    std::अ_बाहर << "Annotation Summary:\n";
    std::अ_बाहर << "  lines with all attached points covered : ";
    std::अ_बाहर << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    अगर (totOk != totCases) अ_बाहर << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

व्योम Vlc_शीर्ष::annotateOutputFiles(स्थिर string& स_नाम) {
    // Create if uncreated, ignore errors
    V3Os::createDir(स_नाम);
    क्रम (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        अगर (!source.needed()) जारी;
        स्थिर string filename = source.name();
        स्थिर string outfilename = स_नाम + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::अख_प्रवाह is{filename.c_str()};
        अगर (!is) {
            v3error("Can't read annotation file: " << filename);
            वापस;
        }

        std::बख_प्रवाह os{outfilename.c_str()};
        अगर (!os) {
            v3error("Can't write file: " << outfilename);
            वापस;
        }

        os << "//      // verilator_coverage annotation\n";

        पूर्णांक lineno = 0;
        जबतक (!is.खप()) {
            lineno++;
            स्थिर std::string पंक्ति = V3Os::लो_पंक्ति(is);

            VlcSource::LinenoMap& पंक्तियाँ = source.पंक्तियाँ();
            स्थिर स्वतः lit = पंक्तियाँ.find(lineno);
            अगर (lit == पंक्तियाँ.end()) {
                os << "        " << पंक्ति << '\n';
            } अन्यथा {
                Vlc_Source_गिनती& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                स्थिर bool minOk = opt.गिनती_Ok(sc.min_गिनती());
                स्थिर bool maxOk = opt.गिनती_Ok(sc.max_गिनती());
                अगर (minOk) {
                    os << " ";
                } अन्यथा अगर (maxOk) {
                    os << "~";
                } अन्यथा {
                    os << "%";
                }
                os << std::रखो_भरो('0') << std::रखो_चौड़ाई(6) << sc.max_गिनती() << " " << पंक्ति << '\n';

                अगर (opt.annotatePoints()) {
                    क्रम (स्थिर स्वतः& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                bool printedFsmHeader = false;
                क्रम (स्थिर स्वतः& pit : sc.points()) {
                    अगर (!pit->isFsmState() && !pit->isFsmArc()) जारी;
                    अगर (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = true;
                    }
                    os << (opt.गिनती_Ok(pit->गिनती()) ? " " : "%");
                    os << std::रखो_भरो('0') << std::रखो_चौड़ाई(6) << pit->गिनती() << "        ";
                    अगर (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        अगर (pit->गिनती() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } अन्यथा अगर (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } अन्यथा {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        अगर (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

व्योम Vlc_शीर्ष::annotate(स्थिर string& स_नाम) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(स_नाम);
}

व्योम Vlc_शीर्ष::printTypeSummary() {
    TypeTally tally;
    क्रम (VlcPoints::ByName::मूल्य_type& i : m_points) {
        स्थिर VlcPoint& pt = m_points.point_संख्या(i.second);
        अगर (isCovergroup(pt)) जारी;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.गिनती());
    }
    स्थिर CgGroups groups = covergroups(m_points);
    अगर (!groups.empty()) {
        std::vector<स्थिर CgGroup*> groupps;
        क्रम (स्थिर CgGroups::मूल्य_type& it : groups) groupps.धकेलो_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    अगर (tally.empty()) वापस;
    std::अ_बाहर << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, true);
}

व्योम Vlc_शीर्ष::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    bool hasHier = false;
    bool hasCollapsedHier = false;
    क्रम (VlcPoints::ByName::मूल्य_type& i : m_points) {
        स्थिर VlcPoint& pt = m_points.point_संख्या(i.second);
        अगर (isCovergroup(pt)) जारी;  // Reported below, by covergroup
        स्थिर string hier = reportHier(pt);
        अगर (hier.empty()) जारी;
        hasHier = true;
        अगर (isCollapsedHier(hier)) hasCollapsedHier = true;
        स्थिर string type = displayType(pt);
        स्थिर std::vector<string> parts = splitHier(hier);
        string path;
        क्रम (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.गिनती());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.गिनती());
    }
    स्थिर std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    अगर (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        वापस;
    }
    अगर (!hasHier) {
        std::अ_बाहर << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        वापस;
    }

    स्थिर पूर्णांक levels = opt.reportLevels();
    अगर (hasCollapsedHier) {
        std::अ_बाहर << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::अ_बाहर << "Hierarchy Coverage Summary:\n";
    क्रम (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        स्थिर std::vector<string> parts = splitHier(it->first);
        अगर (levels >= 0 && static_cast<पूर्णांक>(parts.आकार()) > levels + 1) जारी;
        printIndent(s_summaryIndent);
        std::अ_बाहर << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, false);
    }
    std::अ_बाहर << "Design Unit Coverage Summary:\n";
    क्रम (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::अ_बाहर << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, false);
    }
    अगर (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
