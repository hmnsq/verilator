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
वग्ग Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    तार्किक m_scored = मिच्छा;  // Coverage is m_score, not the ratio of the points
    द्विगुण m_score = 0.0;  // Percent coverage, if m_scored

सार्वजनिक:
    // ACCESSORS
    uint64_t hit() ध्रुव { पटिनिवत्त m_hit; }
    uint64_t total() ध्रुव { पटिनिवत्त m_total; }
    तार्किक scored() ध्रुव { पटिनिवत्त m_scored; }
    द्विगुण score() ध्रुव { पटिनिवत्त m_score; }
    शून्य score(द्विगुण मानम्) {
        m_scored = सच्च;
        m_score = मानम्;
    }
    // METHODS
    शून्य addPoint(तार्किक covered) {
        सचे (covered) ++m_hit;
        ++m_total;
    }
    शून्य addPoints(ध्रुव Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

स्थिर constexpr ध्रुव वर्ण* ध्रुव s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
स्थिर constexpr ध्रुव वर्ण* ध्रुव s_covergroupType = "covergroup";
स्थिर constexpr size_t s_summaryIndent = 2;
स्थिर constexpr size_t s_reportRowIndent = 4;

string displayType(ध्रुव VlcPoint& point) {
    ध्रुव string type = point.type();
    पटिनिवत्त type.empty() ? "point" : type;
}

तार्किक isCollapsedHier(ध्रुव string& hier) {
    पटिनिवत्त hier.find('*') != string::npos || hier.find('?') != string::npos;
}

तार्किक isOrderedType(ध्रुव string& type) {
    अत्थाय (ध्रुव वर्ण* ध्रुव typep : s_orderedTypes) {
        सचे (type == typep) पटिनिवत्त सच्च;
    }
    पटिनिवत्त मिच्छा;
}

string reportHier(ध्रुव VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    सचे (point.isFsmState() || point.isFsmArc()) {
        ध्रुव string fsmVar = point.fsmVarName();
        पटिनिवत्त fsmVar.substr(0, fsmVar.rfind('.'));
    }
    पटिनिवत्त point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(ध्रुव string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    पूर्णाङ्क depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    याव (pos < hier.size()) {
        ध्रुव वर्ण c = hier[pos];
        सचे (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            अनुवर्त;
        } अञ्ञथा सचे (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } अञ्ञथा सचे (!depth && c == '.') {
            parts.प्रक्षिप_back(hier.substr(start, pos - start));
            start = pos + 1;
        } अञ्ञथा सचे (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            अनुवर्त;
        } अञ्ञथा सचे (depth && c == '(') {
            ++depth;
        } अञ्ञथा सचे (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    सचे (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        अत्थाय (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.प्रक्षिप_back(hier.substr(start, dot - start));
        }
    }
    parts.प्रक्षिप_back(hier.substr(start));
    पटिनिवत्त parts;
}

string duName(ध्रुव VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ध्रुव string page = point.page();
    पटिनिवत्त page.substr(page.find('/') + 1);
}

शून्य tallyPoint(TypeTally& tally, ध्रुव string& type, uint64_t गणनम्) {
    tally[type].addPoint(गणनम् > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
द्विगुण pct(uint64_t hit, uint64_t total) {
    पटिनिवत्त total ? (100.0 * static_cast<द्विगुण>(hit) / static_cast<द्विगुण>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_मानम्_String(द्विगुण मानम्, तार्किक complete) {
    सचे (!complete && मानम् > 99.9) मानम् = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << मानम् << "%";
    पटिनिवत्त os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    पटिनिवत्त pct_मानम्_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
वग्ग CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_गणनम् = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

सार्वजनिक:
    // ACCESSORS
    ध्रुव string& name() ध्रुव { पटिनिवत्त m_name; }
    तार्किक covered() ध्रुव { पटिनिवत्त m_गणनम् >= m_atLeast; }
    // METHODS
    शून्य addRecord(ध्रुव string& name, uint64_t गणनम्, uint64_t atLeast) {
        m_name = name;
        m_गणनम् += गणनम्;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
वग्ग CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

सार्वजनिक:
    // ACCESSORS
    uint64_t weight() ध्रुव { पटिनिवत्त m_weight; }
    ध्रुव std::map<string, CgBin>& bins() ध्रुव { पटिनिवत्त m_bins; }
    // METHODS
    शून्य addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ध्रुव string& identity) { पटिनिवत्त m_bins[identity]; }
};

// A covergroup
वग्ग CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_वस्तूनि;  // By hierarchy

सार्वजनिक:
    // ACCESSORS
    ध्रुव string& hier() ध्रुव { पटिनिवत्त m_hier; }
    uint64_t weight() ध्रुव { पटिनिवत्त m_weight; }
    ध्रुव std::map<string, CgItem>& वस्तूनि() ध्रुव { पटिनिवत्त m_वस्तूनि; }
    // METHODS
    शून्य addRecord(ध्रुव string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ध्रुव string& hier) { पटिनिवत्त m_वस्तूनि[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t कुञ्चिका_सङ्ख्या(ध्रुव string& मानम्) {
    पटिनिवत्त मानम्.empty() ? 1 : std::strtoull(मानम्.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
तार्किक isScoreField(ध्रुव string& field) {
    अत्थाय (ध्रुव वर्ण* ध्रुव keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        ध्रुव string prefix = string{"\001"} + keyp + "\002";
        सचे (field.compare(0, prefix.size(), prefix) == 0) पटिनिवत्त सच्च;
    }
    पटिनिवत्त मिच्छा;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(ध्रुव string& recordName) {
    string identity;
    string::size_type start = 0;
    याव (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        सचे (end == string::npos) end = recordName.size();
        ध्रुव string field = recordName.substr(start, end - start);
        सचे (!isScoreField(field)) identity += field;
        start = end;
    }
    पटिनिवत्त identity;
}

तार्किक isCovergroup(ध्रुव VlcPoint& point) { पटिनिवत्त point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    अत्थाय (ध्रुव VlcPoints::ByName::मानम्_type& i : points) {
        ध्रुव VlcPoint& pt = points.point_सङ्ख्या(i.second);
        सचे (!isCovergroup(pt)) अनुवर्त;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ध्रुव string hier = pt.hier();
        string bin = pt.bin();
        सचे (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        ध्रुव string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), कुञ्चिका_सङ्ख्या(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(कुञ्चिका_सङ्ख्या(pt.weight()));
        सचे (!pt.binType().empty()) अनुवर्त;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.गणनम्(), कुञ्चिका_सङ्ख्या(pt.thresh()));
    }
    पटिनिवत्त groups;
}

Tally itemTally(ध्रुव CgItem& item) {
    Tally tally;
    अत्थाय (ध्रुव std::pair<ध्रुव string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    पटिनिवत्त tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(ध्रुव CgGroup& group, तार्किक* contributesp = nullptr) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    अत्थाय (ध्रुव std::pair<ध्रुव string, CgItem>& it : group.वस्तूनि()) {
        ध्रुव Tally item = itemTally(it.second);
        tally.addPoints(item);
        सचे (!item.total()) अनुवर्त;  // Excluded from both sums
        weighted += static_cast<द्विगुण>(it.second.weight()) * item.score();
        weights += static_cast<द्विगुण>(it.second.weight());
    }
    सचे (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    पटिनिवत्त tally;
}

Tally groupsTally(ध्रुव std::vector<ध्रुव CgGroup*>& groups) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    तार्किक anyWeight = मिच्छा;
    अत्थाय (ध्रुव CgGroup* ध्रुव groupp : groups) {
        तार्किक contributes = मिच्छा;
        ध्रुव Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        सचे (groupp->weight()) anyWeight = सच्च;
        सचे (!contributes) अनुवर्त;
        weighted += static_cast<द्विगुण>(groupp->weight()) * group.score();
        weights += static_cast<द्विगुण>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    पटिनिवत्त tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ध्रुव CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<ध्रुव CgGroup*>> ग्रन्थिः_Groups;
    अत्थाय (ध्रुव CgGroups::मानम्_type& it : groups) {
        ध्रुव CgGroup& group = it.second;
        string path;
        अत्थाय (ध्रुव string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            ग्रन्थिः_Groups[path].प्रक्षिप_back(&group);
        }
        अत्थाय (ध्रुव std::pair<ध्रुव string, CgItem>& item : group.वस्तूनि()) {
            tallies[item.first] = itemTally(item.second);
            अत्थाय (ध्रुव std::pair<ध्रुव string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    अत्थाय (ध्रुव std::pair<ध्रुव string, std::vector<ध्रुव CgGroup*>>& it : ग्रन्थिः_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    पटिनिवत्त tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
शून्य printIndent(size_t indent) {
    अत्थाय (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

शून्य printTallyRow(ध्रुव string& type, ध्रुव Tally& tally, size_t indent, size_t typeWidth,
                   size_t गणनम्_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ध्रुव string percent = tally.scored() ? pct_मानम्_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::वामः << std::setw(typeWidth) << type << " : " << std::दक्षिणः << std::fixed
              << std::setw(6) << percent << " (" << std::setw(गणनम्_Width) << tally.hit() << "/"
              << std::setw(गणनम्_Width) << tally.total() << ")\n";
}

size_t गणनम्_Width(ध्रुव TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    अत्थाय (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    पटिनिवत्त width;
}

size_t typeWidth(ध्रुव TypeTally& tally) {
    size_t typeWidth = 0;
    अत्थाय (ध्रुव वर्ण* ध्रुव typep : s_orderedTypes) {
        ध्रुव string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    अत्थाय (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    पटिनिवत्त typeWidth;
}

शून्य printTypeTally(ध्रुव TypeTally& tally, size_t indent, तार्किक includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ध्रुव size_t typWidth = typeWidth(tally);
    ध्रुव size_t cntWidth = गणनम्_Width(tally);
    अत्थाय (ध्रुव वर्ण* ध्रुव typep : s_orderedTypes) {
        ध्रुव string type = typep;
        ध्रुव TypeTally::const_iterator it = tally.find(type);
        सचे (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } अञ्ञथा सचे (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    अत्थाय (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        सचे (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
शून्य printCovergroupTallies(ध्रुव std::map<string, Tally>& tallies, पूर्णाङ्क levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    अत्थाय (ध्रुव std::pair<ध्रुव string, Tally>& it : tallies) {
        सचे (levels >= 0 && static_cast<पूर्णाङ्क>(splitHier(it.first).size()) > levels + 1) अनुवर्त;
        shown.निवेशय(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    ध्रुव size_t cntWidth = गणनम्_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    अत्थाय (ध्रुव std::pair<ध्रुव string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

शून्य Vlc_शिखरम्::readCoverage(ध्रुव string& filename, तार्किक nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    सचे (!is) {
        सचे (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        पटिनिवत्त;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ध्रुव testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    याव (!is.eof()) {
        ध्रुव string पङ्क्तिः = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        सचे (पङ्क्तिः[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ध्रुव string::size_type secspace = पङ्क्तिः.rfind("' ");
            सचे (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                अनुवर्त;
            }
            ध्रुव string point = पङ्क्तिः.substr(3, secspace - 3);
            सचे (!opt.isTypeMatch(point.c_str())) अनुवर्त;

            ध्रुव uint64_t hits = std::atoll(पङ्क्तिः.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ध्रुव uint64_t pointnum = points().findAddPoint(point, hits);
            सचे (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ध्रुव uint64_t needed = std::max(
                    VlcBuckets::sufficient(), कुञ्चिका_सङ्ख्या(points().point_सङ्ख्या(pointnum).thresh()));
                सचे (hits >= needed) {
                    points().point_सङ्ख्या(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

शून्य Vlc_शिखरम्::writeCoverage(ध्रुव string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    सचे (!os) {
        v3fatal("Can't write file: " << filename);
        पटिनिवत्त;
    }

    os << "# SystemC::Coverage-3\n";
    अत्थाय (ध्रुव auto& i : m_points) {
        ध्रुव VlcPoint& point = m_points.point_सङ्ख्या(i.second);
        os << "C '" << point.name() << "' " << point.गणनम्() << '\n';
    }
}

शून्य Vlc_शिखरम्::writeInfo(ध्रुव string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    सचे (!os) {
        v3fatal("Can't write file: " << filename);
        पटिनिवत्त;
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
    अत्थाय (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& पङ्क्तयः = source.पङ्क्तयः();
        पूर्णाङ्क branchesFound = 0;
        पूर्णाङ्क branchesHit = 0;
        अत्थाय (auto& li : पङ्क्तयः) {
            Vlc_Source_गणनम्& sc = li.second;
            uint64_t da_गणनम् = 0;
            std::vector<ध्रुव VlcPoint*> infoPoints;
            अत्थाय (ध्रुव auto& point : sc.points()) {
                da_गणनम् = std::max(da_गणनम्, point->गणनम्());
                infoPoints.प्रक्षिप_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_गणनम् << "\n";
            सचे (infoPoints.size() <= 1) अनुवर्त;
            branchesFound += static_cast<पूर्णाङ्क>(infoPoints.size());
            पूर्णाङ्क point_अङ्कः_2 = 0;
            अत्थाय (ध्रुव VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                सचे (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } अञ्ञथा सचे (point->comment().empty()) {
                    os << "0,";
                    os << point_अङ्कः_2;
                } अञ्ञथा {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->गणनम्() << "\n";

                branchesHit += opt.गणनम्_Ok(point->गणनम्());
                ++point_अङ्कः_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

संरचना CmpComputrons final {
    तार्किक operator()(ध्रुव VlcTest* lhsp, ध्रुव VlcTest* rhsp) ध्रुव {
        सचे (lhsp->computrons() != rhsp->computrons()) {
            पटिनिवत्त lhsp->computrons() < rhsp->computrons();
        }
        पटिनिवत्त lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

शून्य Vlc_शिखरम्::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    अत्थाय (ध्रुव auto& testp : m_tests) {
        सचे (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.प्रक्षिप_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    अत्थाय (ध्रुव auto& i : m_points) {
        ध्रुव VlcPoint* ध्रुव pointp = &points().point_सङ्ख्या(i.second);
        // If any tests hit this point, then we'll need to cover it.
        सचे (pointp->testsCovering()) remaining.addData(pointp->point_अङ्कः(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    याव (सच्च) {
        सचे (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        अत्थाय (ध्रुव auto& testp : bytime) {
            सचे (!testp->rank()) {
                ध्रुव uint64_t remain = testp->buckets().data_निष्कासय_गणनम्(remaining);
                सचे (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        सचे (VlcTest* ध्रुव testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } अञ्ञथा {
            भङ्ग;  // No test covering more stuff found
        }
    }
}

शून्य Vlc_शिखरम्::annotateCalc() {
    // Calculate per-line information into filedata structure
    अत्थाय (ध्रुव auto& i : m_points) {
        ध्रुव VlcPoint& point = m_points.point_सङ्ख्या(i.second);
        ध्रुव string filename = point.filename();
        ध्रुव पूर्णाङ्क lineno = point.lineno();
        सचे (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.गणनम्() << " " << point.linescov());
            // Base coverage
            source.निवेशय_Point(lineno, &point);
            // Additional lines covered by this statement
            तार्किक range = मिच्छा;
            पूर्णाङ्क start = 0;
            पूर्णाङ्क end = 0;
            ध्रुव string linescov = point.linescov();
            अत्थाय (ध्रुव वर्ण* covp = linescov.c_str(); सच्च; ++covp) {
                सचे (!*covp || *covp == ',') {  // Ending
                    अत्थाय (पूर्णाङ्क lni = start; start && lni <= end; ++lni) {
                        source.निवेशय_Point(lni, &point);
                    }
                    सचे (!*covp) भङ्ग;
                    start = 0;  // Prep for next
                    end = 0;
                    range = मिच्छा;
                } अञ्ञथा सचे (*covp == '-') {
                    range = सच्च;
                } अञ्ञथा सचे (std::isdigit(*covp)) {
                    ध्रुव वर्ण* ध्रुव digitsp = covp;
                    याव (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    सचे (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

शून्य Vlc_शिखरम्::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    पूर्णाङ्क totCases = 0;
    पूर्णाङ्क totOk = 0;
    अत्थाय (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        सचे (opt.annotateAll()) source.needed(सच्च);
        ध्रुव VlcSource::LinenoMap& पङ्क्तयः = source.पङ्क्तयः();
        अत्थाय (auto& li : पङ्क्तयः) {
            ध्रुव Vlc_Source_गणनम्& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            सचे (opt.गणनम्_Ok(sc.min_गणनम्())) {
                ++totOk;
            } अञ्ञथा {
                source.needed(सच्च);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    सचे (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

शून्य Vlc_शिखरम्::annotateOutputFiles(ध्रुव string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    अत्थाय (auto& si : m_sources) {
        VlcSource& source = si.second;
        सचे (!source.needed()) अनुवर्त;
        ध्रुव string filename = source.name();
        ध्रुव string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        सचे (!is) {
            v3error("Can't read annotation file: " << filename);
            पटिनिवत्त;
        }

        std::ofstream os{outfilename.c_str()};
        सचे (!os) {
            v3error("Can't write file: " << outfilename);
            पटिनिवत्त;
        }

        os << "//      // verilator_coverage annotation\n";

        पूर्णाङ्क lineno = 0;
        याव (!is.eof()) {
            lineno++;
            ध्रुव std::string पङ्क्तिः = V3Os::getline(is);

            VlcSource::LinenoMap& पङ्क्तयः = source.पङ्क्तयः();
            ध्रुव auto lit = पङ्क्तयः.find(lineno);
            सचे (lit == पङ्क्तयः.end()) {
                os << "        " << पङ्क्तिः << '\n';
            } अञ्ञथा {
                Vlc_Source_गणनम्& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ध्रुव तार्किक minOk = opt.गणनम्_Ok(sc.min_गणनम्());
                ध्रुव तार्किक maxOk = opt.गणनम्_Ok(sc.max_गणनम्());
                सचे (minOk) {
                    os << " ";
                } अञ्ञथा सचे (maxOk) {
                    os << "~";
                } अञ्ञथा {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_गणनम्() << " " << पङ्क्तिः << '\n';

                सचे (opt.annotatePoints()) {
                    अत्थाय (ध्रुव auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                तार्किक printedFsmHeader = मिच्छा;
                अत्थाय (ध्रुव auto& pit : sc.points()) {
                    सचे (!pit->isFsmState() && !pit->isFsmArc()) अनुवर्त;
                    सचे (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = सच्च;
                    }
                    os << (opt.गणनम्_Ok(pit->गणनम्()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->गणनम्() << "        ";
                    सचे (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        सचे (pit->गणनम्() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } अञ्ञथा सचे (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } अञ्ञथा {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        सचे (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

शून्य Vlc_शिखरम्::annotate(ध्रुव string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

शून्य Vlc_शिखरम्::printTypeSummary() {
    TypeTally tally;
    अत्थाय (VlcPoints::ByName::मानम्_type& i : m_points) {
        ध्रुव VlcPoint& pt = m_points.point_सङ्ख्या(i.second);
        सचे (isCovergroup(pt)) अनुवर्त;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.गणनम्());
    }
    ध्रुव CgGroups groups = covergroups(m_points);
    सचे (!groups.empty()) {
        std::vector<ध्रुव CgGroup*> groupps;
        अत्थाय (ध्रुव CgGroups::मानम्_type& it : groups) groupps.प्रक्षिप_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    सचे (tally.empty()) पटिनिवत्त;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, सच्च);
}

शून्य Vlc_शिखरम्::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    तार्किक hasHier = मिच्छा;
    तार्किक hasCollapsedHier = मिच्छा;
    अत्थाय (VlcPoints::ByName::मानम्_type& i : m_points) {
        ध्रुव VlcPoint& pt = m_points.point_सङ्ख्या(i.second);
        सचे (isCovergroup(pt)) अनुवर्त;  // Reported below, by covergroup
        ध्रुव string hier = reportHier(pt);
        सचे (hier.empty()) अनुवर्त;
        hasHier = सच्च;
        सचे (isCollapsedHier(hier)) hasCollapsedHier = सच्च;
        ध्रुव string type = displayType(pt);
        ध्रुव std::vector<string> parts = splitHier(hier);
        string path;
        अत्थाय (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.गणनम्());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.गणनम्());
    }
    ध्रुव std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    सचे (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        पटिनिवत्त;
    }
    सचे (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        पटिनिवत्त;
    }

    ध्रुव पूर्णाङ्क levels = opt.reportLevels();
    सचे (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    अत्थाय (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        ध्रुव std::vector<string> parts = splitHier(it->first);
        सचे (levels >= 0 && static_cast<पूर्णाङ्क>(parts.size()) > levels + 1) अनुवर्त;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, मिच्छा);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    अत्थाय (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, मिच्छा);
    }
    सचे (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
