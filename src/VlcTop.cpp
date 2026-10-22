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
    uint64_t hit() स्थिरांक { परत m_hit; }
    uint64_t total() स्थिरांक { परत m_total; }
    तार्किक scored() स्थिरांक { परत m_scored; }
    द्विगुण score() स्थिरांक { परत m_score; }
    शून्य score(द्विगुण मूल्य) {
        m_scored = सत्य;
        m_score = मूल्य;
    }
    // METHODS
    शून्य addPoint(तार्किक covered) {
        जर (covered) ++m_hit;
        ++m_total;
    }
    शून्य addPoints(स्थिरांक Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

स्थिर constexpr स्थिरांक अक्षर* स्थिरांक s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
स्थिर constexpr स्थिरांक अक्षर* स्थिरांक s_covergroupType = "covergroup";
स्थिर constexpr माप_प्रकार s_summaryIndent = 2;
स्थिर constexpr माप_प्रकार s_reportRowIndent = 4;

string displayType(स्थिरांक VlcPoint& point) {
    स्थिरांक string type = point.type();
    परत type.empty() ? "point" : type;
}

तार्किक isCollapsedHier(स्थिरांक string& hier) {
    परत hier.find('*') != string::npos || hier.find('?') != string::npos;
}

तार्किक isOrderedType(स्थिरांक string& type) {
    साठी (स्थिरांक अक्षर* स्थिरांक typep : s_orderedTypes) {
        जर (type == typep) परत सत्य;
    }
    परत असत्य;
}

string reportHier(स्थिरांक VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    जर (point.isFsmState() || point.isFsmArc()) {
        स्थिरांक string fsmVar = point.fsmVarName();
        परत fsmVar.substr(0, fsmVar.rfind('.'));
    }
    परत point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(स्थिरांक string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    पूर्णांक depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    पर्यंत (pos < hier.size()) {
        स्थिरांक अक्षर c = hier[pos];
        जर (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            सुरूठेवा;
        } अन्यथा जर (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } अन्यथा जर (!depth && c == '.') {
            parts.ढकला_back(hier.substr(start, pos - start));
            start = pos + 1;
        } अन्यथा जर (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            सुरूठेवा;
        } अन्यथा जर (depth && c == '(') {
            ++depth;
        } अन्यथा जर (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    जर (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        साठी (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.ढकला_back(hier.substr(start, dot - start));
        }
    }
    parts.ढकला_back(hier.substr(start));
    परत parts;
}

string duName(स्थिरांक VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    स्थिरांक string page = point.page();
    परत page.substr(page.find('/') + 1);
}

शून्य tallyPoint(TypeTally& tally, स्थिरांक string& type, uint64_t मोजणी) {
    tally[type].addPoint(मोजणी > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
द्विगुण pct(uint64_t hit, uint64_t total) {
    परत total ? (100.0 * static_cast<द्विगुण>(hit) / static_cast<द्विगुण>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_मूल्य_String(द्विगुण मूल्य, तार्किक complete) {
    जर (!complete && मूल्य > 99.9) मूल्य = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << मूल्य << "%";
    परत os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    परत pct_मूल्य_String(pct(hit, total), hit >= total);
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
    uint64_t m_मोजणी = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

सार्वजनिक:
    // ACCESSORS
    स्थिरांक string& name() स्थिरांक { परत m_name; }
    तार्किक covered() स्थिरांक { परत m_मोजणी >= m_atLeast; }
    // METHODS
    शून्य addRecord(स्थिरांक string& name, uint64_t मोजणी, uint64_t atLeast) {
        m_name = name;
        m_मोजणी += मोजणी;
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
    uint64_t weight() स्थिरांक { परत m_weight; }
    स्थिरांक std::map<string, CgBin>& bins() स्थिरांक { परत m_bins; }
    // METHODS
    शून्य addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(स्थिरांक string& identity) { परत m_bins[identity]; }
};

// A covergroup
वर्ग CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_वस्तुसंच;  // By hierarchy

सार्वजनिक:
    // ACCESSORS
    स्थिरांक string& hier() स्थिरांक { परत m_hier; }
    uint64_t weight() स्थिरांक { परत m_weight; }
    स्थिरांक std::map<string, CgItem>& वस्तुसंच() स्थिरांक { परत m_वस्तुसंच; }
    // METHODS
    शून्य addRecord(स्थिरांक string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(स्थिरांक string& hier) { परत m_वस्तुसंच[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t किल्ली_संख्या(स्थिरांक string& मूल्य) {
    परत मूल्य.empty() ? 1 : std::strtoull(मूल्य.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
तार्किक isScoreField(स्थिरांक string& field) {
    साठी (स्थिरांक अक्षर* स्थिरांक keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        स्थिरांक string prefix = string{"\001"} + keyp + "\002";
        जर (field.compare(0, prefix.size(), prefix) == 0) परत सत्य;
    }
    परत असत्य;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(स्थिरांक string& recordName) {
    string identity;
    string::size_type start = 0;
    पर्यंत (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        जर (end == string::npos) end = recordName.size();
        स्थिरांक string field = recordName.substr(start, end - start);
        जर (!isScoreField(field)) identity += field;
        start = end;
    }
    परत identity;
}

तार्किक isCovergroup(स्थिरांक VlcPoint& point) { परत point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    साठी (स्थिरांक VlcPoints::ByName::मूल्य_type& i : points) {
        स्थिरांक VlcPoint& pt = points.point_संख्या(i.second);
        जर (!isCovergroup(pt)) सुरूठेवा;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        स्थिरांक string hier = pt.hier();
        string bin = pt.bin();
        जर (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        स्थिरांक string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), किल्ली_संख्या(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(किल्ली_संख्या(pt.weight()));
        जर (!pt.binType().empty()) सुरूठेवा;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.मोजणी(), किल्ली_संख्या(pt.thresh()));
    }
    परत groups;
}

Tally itemTally(स्थिरांक CgItem& item) {
    Tally tally;
    साठी (स्थिरांक std::pair<स्थिरांक string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    परत tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(स्थिरांक CgGroup& group, तार्किक* contributesp = nullptr) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    साठी (स्थिरांक std::pair<स्थिरांक string, CgItem>& it : group.वस्तुसंच()) {
        स्थिरांक Tally item = itemTally(it.second);
        tally.addPoints(item);
        जर (!item.total()) सुरूठेवा;  // Excluded from both sums
        weighted += static_cast<द्विगुण>(it.second.weight()) * item.score();
        weights += static_cast<द्विगुण>(it.second.weight());
    }
    जर (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    परत tally;
}

Tally groupsTally(स्थिरांक std::vector<स्थिरांक CgGroup*>& groups) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    तार्किक anyWeight = असत्य;
    साठी (स्थिरांक CgGroup* स्थिरांक groupp : groups) {
        तार्किक contributes = असत्य;
        स्थिरांक Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        जर (groupp->weight()) anyWeight = सत्य;
        जर (!contributes) सुरूठेवा;
        weighted += static_cast<द्विगुण>(groupp->weight()) * group.score();
        weights += static_cast<द्विगुण>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    परत tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(स्थिरांक CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<स्थिरांक CgGroup*>> गाठ_Groups;
    साठी (स्थिरांक CgGroups::मूल्य_type& it : groups) {
        स्थिरांक CgGroup& group = it.second;
        string path;
        साठी (स्थिरांक string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            गाठ_Groups[path].ढकला_back(&group);
        }
        साठी (स्थिरांक std::pair<स्थिरांक string, CgItem>& item : group.वस्तुसंच()) {
            tallies[item.first] = itemTally(item.second);
            साठी (स्थिरांक std::pair<स्थिरांक string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    साठी (स्थिरांक std::pair<स्थिरांक string, std::vector<स्थिरांक CgGroup*>>& it : गाठ_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    परत tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
शून्य printIndent(माप_प्रकार indent) {
    साठी (माप_प्रकार i = 0; i < indent; ++i) std::cout << ' ';
}

शून्य printTallyRow(स्थिरांक string& type, स्थिरांक Tally& tally, माप_प्रकार indent, माप_प्रकार typeWidth,
                   माप_प्रकार मोजणी_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    स्थिरांक string percent = tally.scored() ? pct_मूल्य_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::डावा << std::setw(typeWidth) << type << " : " << std::उजवा << std::fixed
              << std::setw(6) << percent << " (" << std::setw(मोजणी_Width) << tally.hit() << "/"
              << std::setw(मोजणी_Width) << tally.total() << ")\n";
}

माप_प्रकार मोजणी_Width(स्थिरांक TypeTally& tally) {
    माप_प्रकार width = cvtToStr(0).size();
    साठी (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    परत width;
}

माप_प्रकार typeWidth(स्थिरांक TypeTally& tally) {
    माप_प्रकार typeWidth = 0;
    साठी (स्थिरांक अक्षर* स्थिरांक typep : s_orderedTypes) {
        स्थिरांक string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    साठी (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    परत typeWidth;
}

शून्य printTypeTally(स्थिरांक TypeTally& tally, माप_प्रकार indent, तार्किक includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    स्थिरांक माप_प्रकार typWidth = typeWidth(tally);
    स्थिरांक माप_प्रकार cntWidth = मोजणी_Width(tally);
    साठी (स्थिरांक अक्षर* स्थिरांक typep : s_orderedTypes) {
        स्थिरांक string type = typep;
        स्थिरांक TypeTally::const_iterator it = tally.find(type);
        जर (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } अन्यथा जर (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    साठी (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        जर (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
शून्य printCovergroupTallies(स्थिरांक std::map<string, Tally>& tallies, पूर्णांक levels) {
    std::map<string, Tally> shown;
    माप_प्रकार nameWidth = 0;
    साठी (स्थिरांक std::pair<स्थिरांक string, Tally>& it : tallies) {
        जर (levels >= 0 && static_cast<पूर्णांक>(splitHier(it.first).size()) > levels + 1) सुरूठेवा;
        shown.घाला(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    स्थिरांक माप_प्रकार cntWidth = मोजणी_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    साठी (स्थिरांक std::pair<स्थिरांक string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

शून्य Vlc_शीर्ष::readCoverage(स्थिरांक string& filename, तार्किक nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    जर (!is) {
        जर (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        परत;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* स्थिरांक testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    पर्यंत (!is.eof()) {
        स्थिरांक string ओळ = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        जर (ओळ[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            स्थिरांक string::size_type secspace = ओळ.rfind("' ");
            जर (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                सुरूठेवा;
            }
            स्थिरांक string point = ओळ.substr(3, secspace - 3);
            जर (!opt.isTypeMatch(point.c_str())) सुरूठेवा;

            स्थिरांक uint64_t hits = std::atoll(ओळ.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            स्थिरांक uint64_t pointnum = points().findAddPoint(point, hits);
            जर (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                स्थिरांक uint64_t needed = std::max(
                    VlcBuckets::sufficient(), किल्ली_संख्या(points().point_संख्या(pointnum).thresh()));
                जर (hits >= needed) {
                    points().point_संख्या(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

शून्य Vlc_शीर्ष::writeCoverage(स्थिरांक string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    जर (!os) {
        v3fatal("Can't write file: " << filename);
        परत;
    }

    os << "# SystemC::Coverage-3\n";
    साठी (स्थिरांक स्वतः& i : m_points) {
        स्थिरांक VlcPoint& point = m_points.point_संख्या(i.second);
        os << "C '" << point.name() << "' " << point.मोजणी() << '\n';
    }
}

शून्य Vlc_शीर्ष::writeInfo(स्थिरांक string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    जर (!os) {
        v3fatal("Can't write file: " << filename);
        परत;
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
    साठी (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& ओळी = source.ओळी();
        पूर्णांक branchesFound = 0;
        पूर्णांक branchesHit = 0;
        साठी (स्वतः& li : ओळी) {
            Vlc_Source_मोजणी& sc = li.second;
            uint64_t da_मोजणी = 0;
            std::vector<स्थिरांक VlcPoint*> infoPoints;
            साठी (स्थिरांक स्वतः& point : sc.points()) {
                da_मोजणी = std::max(da_मोजणी, point->मोजणी());
                infoPoints.ढकला_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_मोजणी << "\n";
            जर (infoPoints.size() <= 1) सुरूठेवा;
            branchesFound += static_cast<पूर्णांक>(infoPoints.size());
            पूर्णांक point_अंक_2 = 0;
            साठी (स्थिरांक VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                जर (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } अन्यथा जर (point->comment().empty()) {
                    os << "0,";
                    os << point_अंक_2;
                } अन्यथा {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->मोजणी() << "\n";

                branchesHit += opt.मोजणी_Ok(point->मोजणी());
                ++point_अंक_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

संरचना CmpComputrons final {
    तार्किक चालक()(स्थिरांक VlcTest* lhsp, स्थिरांक VlcTest* rhsp) स्थिरांक {
        जर (lhsp->computrons() != rhsp->computrons()) {
            परत lhsp->computrons() < rhsp->computrons();
        }
        परत lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

शून्य Vlc_शीर्ष::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    साठी (स्थिरांक स्वतः& testp : m_tests) {
        जर (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ढकला_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    साठी (स्थिरांक स्वतः& i : m_points) {
        स्थिरांक VlcPoint* स्थिरांक pointp = &points().point_संख्या(i.second);
        // If any tests hit this point, then we'll need to cover it.
        जर (pointp->testsCovering()) remaining.addData(pointp->point_अंक(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    पर्यंत (सत्य) {
        जर (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        साठी (स्थिरांक स्वतः& testp : bytime) {
            जर (!testp->rank()) {
                स्थिरांक uint64_t remain = testp->buckets().data_काढा_मोजणी(remaining);
                जर (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        जर (VlcTest* स्थिरांक testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } अन्यथा {
            थांबा;  // No test covering more stuff found
        }
    }
}

शून्य Vlc_शीर्ष::annotateCalc() {
    // Calculate per-line information into filedata structure
    साठी (स्थिरांक स्वतः& i : m_points) {
        स्थिरांक VlcPoint& point = m_points.point_संख्या(i.second);
        स्थिरांक string filename = point.filename();
        स्थिरांक पूर्णांक lineno = point.lineno();
        जर (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.मोजणी() << " " << point.linescov());
            // Base coverage
            source.घाला_Point(lineno, &point);
            // Additional lines covered by this statement
            तार्किक range = असत्य;
            पूर्णांक start = 0;
            पूर्णांक end = 0;
            स्थिरांक string linescov = point.linescov();
            साठी (स्थिरांक अक्षर* covp = linescov.c_str(); सत्य; ++covp) {
                जर (!*covp || *covp == ',') {  // Ending
                    साठी (पूर्णांक lni = start; start && lni <= end; ++lni) {
                        source.घाला_Point(lni, &point);
                    }
                    जर (!*covp) थांबा;
                    start = 0;  // Prep for next
                    end = 0;
                    range = असत्य;
                } अन्यथा जर (*covp == '-') {
                    range = सत्य;
                } अन्यथा जर (std::है_अंक(*covp)) {
                    स्थिरांक अक्षर* स्थिरांक digitsp = covp;
                    पर्यंत (std::है_अंक(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    जर (!range) start = std::म_से_प(digitsp);
                    end = std::म_से_प(digitsp);
                }
            }
        }
    }
}

शून्य Vlc_शीर्ष::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    पूर्णांक totCases = 0;
    पूर्णांक totOk = 0;
    साठी (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        जर (opt.annotateAll()) source.needed(सत्य);
        स्थिरांक VlcSource::LinenoMap& ओळी = source.ओळी();
        साठी (स्वतः& li : ओळी) {
            स्थिरांक Vlc_Source_मोजणी& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            जर (opt.मोजणी_Ok(sc.min_मोजणी())) {
                ++totOk;
            } अन्यथा {
                source.needed(सत्य);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    जर (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

शून्य Vlc_शीर्ष::annotateOutputFiles(स्थिरांक string& स_नाम) {
    // Create if uncreated, ignore errors
    V3Os::createDir(स_नाम);
    साठी (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        जर (!source.needed()) सुरूठेवा;
        स्थिरांक string filename = source.name();
        स्थिरांक string outfilename = स_नाम + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        जर (!is) {
            v3error("Can't read annotation file: " << filename);
            परत;
        }

        std::ofstream os{outfilename.c_str()};
        जर (!os) {
            v3error("Can't write file: " << outfilename);
            परत;
        }

        os << "//      // verilator_coverage annotation\n";

        पूर्णांक lineno = 0;
        पर्यंत (!is.eof()) {
            lineno++;
            स्थिरांक std::string ओळ = V3Os::getline(is);

            VlcSource::LinenoMap& ओळी = source.ओळी();
            स्थिरांक स्वतः lit = ओळी.find(lineno);
            जर (lit == ओळी.end()) {
                os << "        " << ओळ << '\n';
            } अन्यथा {
                Vlc_Source_मोजणी& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                स्थिरांक तार्किक minOk = opt.मोजणी_Ok(sc.min_मोजणी());
                स्थिरांक तार्किक maxOk = opt.मोजणी_Ok(sc.max_मोजणी());
                जर (minOk) {
                    os << " ";
                } अन्यथा जर (maxOk) {
                    os << "~";
                } अन्यथा {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_मोजणी() << " " << ओळ << '\n';

                जर (opt.annotatePoints()) {
                    साठी (स्थिरांक स्वतः& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                तार्किक printedFsmHeader = असत्य;
                साठी (स्थिरांक स्वतः& pit : sc.points()) {
                    जर (!pit->isFsmState() && !pit->isFsmArc()) सुरूठेवा;
                    जर (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = सत्य;
                    }
                    os << (opt.मोजणी_Ok(pit->मोजणी()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->मोजणी() << "        ";
                    जर (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        जर (pit->मोजणी() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } अन्यथा जर (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } अन्यथा {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        जर (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

शून्य Vlc_शीर्ष::annotate(स्थिरांक string& स_नाम) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(स_नाम);
}

शून्य Vlc_शीर्ष::printTypeSummary() {
    TypeTally tally;
    साठी (VlcPoints::ByName::मूल्य_type& i : m_points) {
        स्थिरांक VlcPoint& pt = m_points.point_संख्या(i.second);
        जर (isCovergroup(pt)) सुरूठेवा;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.मोजणी());
    }
    स्थिरांक CgGroups groups = covergroups(m_points);
    जर (!groups.empty()) {
        std::vector<स्थिरांक CgGroup*> groupps;
        साठी (स्थिरांक CgGroups::मूल्य_type& it : groups) groupps.ढकला_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    जर (tally.empty()) परत;
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
    साठी (VlcPoints::ByName::मूल्य_type& i : m_points) {
        स्थिरांक VlcPoint& pt = m_points.point_संख्या(i.second);
        जर (isCovergroup(pt)) सुरूठेवा;  // Reported below, by covergroup
        स्थिरांक string hier = reportHier(pt);
        जर (hier.empty()) सुरूठेवा;
        hasHier = सत्य;
        जर (isCollapsedHier(hier)) hasCollapsedHier = सत्य;
        स्थिरांक string type = displayType(pt);
        स्थिरांक std::vector<string> parts = splitHier(hier);
        string path;
        साठी (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.मोजणी());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.मोजणी());
    }
    स्थिरांक std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    जर (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        परत;
    }
    जर (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        परत;
    }

    स्थिरांक पूर्णांक levels = opt.reportLevels();
    जर (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    साठी (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        स्थिरांक std::vector<string> parts = splitHier(it->first);
        जर (levels >= 0 && static_cast<पूर्णांक>(parts.size()) > levels + 1) सुरूठेवा;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, असत्य);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    साठी (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, असत्य);
    }
    जर (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
