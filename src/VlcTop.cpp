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
    uint64_t hit() ध्रुव { प्रत्यागम m_hit; }
    uint64_t total() ध्रुव { प्रत्यागम m_total; }
    तार्किक scored() ध्रुव { प्रत्यागम m_scored; }
    द्विगुण score() ध्रुव { प्रत्यागम m_score; }
    शून्य score(द्विगुण मूल्य) {
        m_scored = सत्य;
        m_score = मूल्य;
    }
    // METHODS
    शून्य addPoint(तार्किक covered) {
        यदि (covered) ++m_hit;
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
स्थिर constexpr माप_प्रकार s_summaryIndent = 2;
स्थिर constexpr माप_प्रकार s_reportRowIndent = 4;

string displayType(ध्रुव VlcPoint& point) {
    ध्रुव string type = point.type();
    प्रत्यागम type.empty() ? "point" : type;
}

तार्किक isCollapsedHier(ध्रुव string& hier) {
    प्रत्यागम hier.find('*') != string::npos || hier.find('?') != string::npos;
}

तार्किक isOrderedType(ध्रुव string& type) {
    प्रति (ध्रुव वर्ण* ध्रुव typep : s_orderedTypes) {
        यदि (type == typep) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}

string reportHier(ध्रुव VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    यदि (point.isFsmState() || point.isFsmArc()) {
        ध्रुव string fsmVar = point.fsmVarName();
        प्रत्यागम fsmVar.substr(0, fsmVar.rfind('.'));
    }
    प्रत्यागम point.hier();
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
    यावत् (pos < hier.size()) {
        ध्रुव वर्ण c = hier[pos];
        यदि (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            अनुवर्त;
        } अन्यथा यदि (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } अन्यथा यदि (!depth && c == '.') {
            parts.धकेलो_back(hier.substr(start, pos - start));
            start = pos + 1;
        } अन्यथा यदि (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            अनुवर्त;
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
        प्रति (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.धकेलो_back(hier.substr(start, dot - start));
        }
    }
    parts.धकेलो_back(hier.substr(start));
    प्रत्यागम parts;
}

string duName(ध्रुव VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ध्रुव string page = point.page();
    प्रत्यागम page.substr(page.find('/') + 1);
}

शून्य tallyPoint(TypeTally& tally, ध्रुव string& type, uint64_t गिनती) {
    tally[type].addPoint(गिनती > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
द्विगुण pct(uint64_t hit, uint64_t total) {
    प्रत्यागम total ? (100.0 * static_cast<द्विगुण>(hit) / static_cast<द्विगुण>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_मूल्य_String(द्विगुण मूल्य, तार्किक complete) {
    यदि (!complete && मूल्य > 99.9) मूल्य = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << मूल्य << "%";
    प्रत्यागम os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    प्रत्यागम pct_मूल्य_String(pct(hit, total), hit >= total);
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
    uint64_t m_गिनती = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

सार्वजनिक:
    // ACCESSORS
    ध्रुव string& name() ध्रुव { प्रत्यागम m_name; }
    तार्किक covered() ध्रुव { प्रत्यागम m_गिनती >= m_atLeast; }
    // METHODS
    शून्य addRecord(ध्रुव string& name, uint64_t गिनती, uint64_t atLeast) {
        m_name = name;
        m_गिनती += गिनती;
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
    uint64_t weight() ध्रुव { प्रत्यागम m_weight; }
    ध्रुव std::map<string, CgBin>& bins() ध्रुव { प्रत्यागम m_bins; }
    // METHODS
    शून्य addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ध्रुव string& identity) { प्रत्यागम m_bins[identity]; }
};

// A covergroup
वर्ग CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_वस्तुएँ;  // By hierarchy

सार्वजनिक:
    // ACCESSORS
    ध्रुव string& hier() ध्रुव { प्रत्यागम m_hier; }
    uint64_t weight() ध्रुव { प्रत्यागम m_weight; }
    ध्रुव std::map<string, CgItem>& वस्तुएँ() ध्रुव { प्रत्यागम m_वस्तुएँ; }
    // METHODS
    शून्य addRecord(ध्रुव string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ध्रुव string& hier) { प्रत्यागम m_वस्तुएँ[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t कुंजी_संख्या(ध्रुव string& मूल्य) {
    प्रत्यागम मूल्य.empty() ? 1 : std::strtoull(मूल्य.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
तार्किक isScoreField(ध्रुव string& field) {
    प्रति (ध्रुव वर्ण* ध्रुव keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        ध्रुव string prefix = string{"\001"} + keyp + "\002";
        यदि (field.compare(0, prefix.size(), prefix) == 0) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(ध्रुव string& recordName) {
    string identity;
    string::size_type start = 0;
    यावत् (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        यदि (end == string::npos) end = recordName.size();
        ध्रुव string field = recordName.substr(start, end - start);
        यदि (!isScoreField(field)) identity += field;
        start = end;
    }
    प्रत्यागम identity;
}

तार्किक isCovergroup(ध्रुव VlcPoint& point) { प्रत्यागम point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    प्रति (ध्रुव VlcPoints::ByName::मूल्य_type& i : points) {
        ध्रुव VlcPoint& pt = points.point_संख्या(i.second);
        यदि (!isCovergroup(pt)) अनुवर्त;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ध्रुव string hier = pt.hier();
        string bin = pt.bin();
        यदि (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        ध्रुव string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), कुंजी_संख्या(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(कुंजी_संख्या(pt.weight()));
        यदि (!pt.binType().empty()) अनुवर्त;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.गिनती(), कुंजी_संख्या(pt.thresh()));
    }
    प्रत्यागम groups;
}

Tally itemTally(ध्रुव CgItem& item) {
    Tally tally;
    प्रति (ध्रुव std::pair<ध्रुव string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    प्रत्यागम tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(ध्रुव CgGroup& group, तार्किक* contributesp = nullptr) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    प्रति (ध्रुव std::pair<ध्रुव string, CgItem>& it : group.वस्तुएँ()) {
        ध्रुव Tally item = itemTally(it.second);
        tally.addPoints(item);
        यदि (!item.total()) अनुवर्त;  // Excluded from both sums
        weighted += static_cast<द्विगुण>(it.second.weight()) * item.score();
        weights += static_cast<द्विगुण>(it.second.weight());
    }
    यदि (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    प्रत्यागम tally;
}

Tally groupsTally(ध्रुव std::vector<ध्रुव CgGroup*>& groups) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    तार्किक anyWeight = असत्य;
    प्रति (ध्रुव CgGroup* ध्रुव groupp : groups) {
        तार्किक contributes = असत्य;
        ध्रुव Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        यदि (groupp->weight()) anyWeight = सत्य;
        यदि (!contributes) अनुवर्त;
        weighted += static_cast<द्विगुण>(groupp->weight()) * group.score();
        weights += static_cast<द्विगुण>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    प्रत्यागम tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ध्रुव CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<ध्रुव CgGroup*>> गाँठ_Groups;
    प्रति (ध्रुव CgGroups::मूल्य_type& it : groups) {
        ध्रुव CgGroup& group = it.second;
        string path;
        प्रति (ध्रुव string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            गाँठ_Groups[path].धकेलो_back(&group);
        }
        प्रति (ध्रुव std::pair<ध्रुव string, CgItem>& item : group.वस्तुएँ()) {
            tallies[item.first] = itemTally(item.second);
            प्रति (ध्रुव std::pair<ध्रुव string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    प्रति (ध्रुव std::pair<ध्रुव string, std::vector<ध्रुव CgGroup*>>& it : गाँठ_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    प्रत्यागम tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
शून्य printIndent(माप_प्रकार indent) {
    प्रति (माप_प्रकार i = 0; i < indent; ++i) std::cout << ' ';
}

शून्य printTallyRow(ध्रुव string& type, ध्रुव Tally& tally, माप_प्रकार indent, माप_प्रकार typeWidth,
                   माप_प्रकार गिनती_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ध्रुव string percent = tally.scored() ? pct_मूल्य_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::बायाँ << std::setw(typeWidth) << type << " : " << std::दायाँ << std::fixed
              << std::setw(6) << percent << " (" << std::setw(गिनती_Width) << tally.hit() << "/"
              << std::setw(गिनती_Width) << tally.total() << ")\n";
}

माप_प्रकार गिनती_Width(ध्रुव TypeTally& tally) {
    माप_प्रकार width = cvtToStr(0).size();
    प्रति (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    प्रत्यागम width;
}

माप_प्रकार typeWidth(ध्रुव TypeTally& tally) {
    माप_प्रकार typeWidth = 0;
    प्रति (ध्रुव वर्ण* ध्रुव typep : s_orderedTypes) {
        ध्रुव string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    प्रति (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    प्रत्यागम typeWidth;
}

शून्य printTypeTally(ध्रुव TypeTally& tally, माप_प्रकार indent, तार्किक includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ध्रुव माप_प्रकार typWidth = typeWidth(tally);
    ध्रुव माप_प्रकार cntWidth = गिनती_Width(tally);
    प्रति (ध्रुव वर्ण* ध्रुव typep : s_orderedTypes) {
        ध्रुव string type = typep;
        ध्रुव TypeTally::const_iterator it = tally.find(type);
        यदि (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } अन्यथा यदि (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    प्रति (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        यदि (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
शून्य printCovergroupTallies(ध्रुव std::map<string, Tally>& tallies, पूर्णाङ्क levels) {
    std::map<string, Tally> shown;
    माप_प्रकार nameWidth = 0;
    प्रति (ध्रुव std::pair<ध्रुव string, Tally>& it : tallies) {
        यदि (levels >= 0 && static_cast<पूर्णाङ्क>(splitHier(it.first).size()) > levels + 1) अनुवर्त;
        shown.जोड़ो(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    ध्रुव माप_प्रकार cntWidth = गिनती_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    प्रति (ध्रुव std::pair<ध्रुव string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

शून्य Vlc_शीर्ष::readCoverage(ध्रुव string& filename, तार्किक nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    यदि (!is) {
        यदि (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        प्रत्यागम;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ध्रुव testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    यावत् (!is.eof()) {
        ध्रुव string पंक्ति = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        यदि (पंक्ति[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ध्रुव string::size_type secspace = पंक्ति.rfind("' ");
            यदि (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                अनुवर्त;
            }
            ध्रुव string point = पंक्ति.substr(3, secspace - 3);
            यदि (!opt.isTypeMatch(point.c_str())) अनुवर्त;

            ध्रुव uint64_t hits = std::atoll(पंक्ति.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ध्रुव uint64_t pointnum = points().findAddPoint(point, hits);
            यदि (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ध्रुव uint64_t needed = std::max(
                    VlcBuckets::sufficient(), कुंजी_संख्या(points().point_संख्या(pointnum).thresh()));
                यदि (hits >= needed) {
                    points().point_संख्या(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

शून्य Vlc_शीर्ष::writeCoverage(ध्रुव string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    यदि (!os) {
        v3fatal("Can't write file: " << filename);
        प्रत्यागम;
    }

    os << "# SystemC::Coverage-3\n";
    प्रति (ध्रुव स्वतः& i : m_points) {
        ध्रुव VlcPoint& point = m_points.point_संख्या(i.second);
        os << "C '" << point.name() << "' " << point.गिनती() << '\n';
    }
}

शून्य Vlc_शीर्ष::writeInfo(ध्रुव string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    यदि (!os) {
        v3fatal("Can't write file: " << filename);
        प्रत्यागम;
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
    प्रति (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& पंक्तियाँ = source.पंक्तियाँ();
        पूर्णाङ्क branchesFound = 0;
        पूर्णाङ्क branchesHit = 0;
        प्रति (स्वतः& li : पंक्तियाँ) {
            Vlc_Source_गिनती& sc = li.second;
            uint64_t da_गिनती = 0;
            std::vector<ध्रुव VlcPoint*> infoPoints;
            प्रति (ध्रुव स्वतः& point : sc.points()) {
                da_गिनती = std::max(da_गिनती, point->गिनती());
                infoPoints.धकेलो_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_गिनती << "\n";
            यदि (infoPoints.size() <= 1) अनुवर्त;
            branchesFound += static_cast<पूर्णाङ्क>(infoPoints.size());
            पूर्णाङ्क point_अंक_2 = 0;
            प्रति (ध्रुव VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                यदि (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } अन्यथा यदि (point->comment().empty()) {
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

संरचना CmpComputrons final {
    तार्किक चालक()(ध्रुव VlcTest* lhsp, ध्रुव VlcTest* rhsp) ध्रुव {
        यदि (lhsp->computrons() != rhsp->computrons()) {
            प्रत्यागम lhsp->computrons() < rhsp->computrons();
        }
        प्रत्यागम lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

शून्य Vlc_शीर्ष::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    प्रति (ध्रुव स्वतः& testp : m_tests) {
        यदि (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.धकेलो_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    प्रति (ध्रुव स्वतः& i : m_points) {
        ध्रुव VlcPoint* ध्रुव pointp = &points().point_संख्या(i.second);
        // If any tests hit this point, then we'll need to cover it.
        यदि (pointp->testsCovering()) remaining.addData(pointp->point_अंक(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    यावत् (सत्य) {
        यदि (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        प्रति (ध्रुव स्वतः& testp : bytime) {
            यदि (!testp->rank()) {
                ध्रुव uint64_t remain = testp->buckets().data_निकालो_गिनती(remaining);
                यदि (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        यदि (VlcTest* ध्रुव testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } अन्यथा {
            भङ्ग;  // No test covering more stuff found
        }
    }
}

शून्य Vlc_शीर्ष::annotateCalc() {
    // Calculate per-line information into filedata structure
    प्रति (ध्रुव स्वतः& i : m_points) {
        ध्रुव VlcPoint& point = m_points.point_संख्या(i.second);
        ध्रुव string filename = point.filename();
        ध्रुव पूर्णाङ्क lineno = point.lineno();
        यदि (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.गिनती() << " " << point.linescov());
            // Base coverage
            source.जोड़ो_Point(lineno, &point);
            // Additional lines covered by this statement
            तार्किक range = असत्य;
            पूर्णाङ्क start = 0;
            पूर्णाङ्क end = 0;
            ध्रुव string linescov = point.linescov();
            प्रति (ध्रुव वर्ण* covp = linescov.c_str(); सत्य; ++covp) {
                यदि (!*covp || *covp == ',') {  // Ending
                    प्रति (पूर्णाङ्क lni = start; start && lni <= end; ++lni) {
                        source.जोड़ो_Point(lni, &point);
                    }
                    यदि (!*covp) भङ्ग;
                    start = 0;  // Prep for next
                    end = 0;
                    range = असत्य;
                } अन्यथा यदि (*covp == '-') {
                    range = सत्य;
                } अन्यथा यदि (std::है_अंक(*covp)) {
                    ध्रुव वर्ण* ध्रुव digitsp = covp;
                    यावत् (std::है_अंक(*covp)) ++covp;
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
    प्रति (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        यदि (opt.annotateAll()) source.needed(सत्य);
        ध्रुव VlcSource::LinenoMap& पंक्तियाँ = source.पंक्तियाँ();
        प्रति (स्वतः& li : पंक्तियाँ) {
            ध्रुव Vlc_Source_गिनती& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            यदि (opt.गिनती_Ok(sc.min_गिनती())) {
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

शून्य Vlc_शीर्ष::annotateOutputFiles(ध्रुव string& स_नाम) {
    // Create if uncreated, ignore errors
    V3Os::createDir(स_नाम);
    प्रति (स्वतः& si : m_sources) {
        VlcSource& source = si.second;
        यदि (!source.needed()) अनुवर्त;
        ध्रुव string filename = source.name();
        ध्रुव string outfilename = स_नाम + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        यदि (!is) {
            v3error("Can't read annotation file: " << filename);
            प्रत्यागम;
        }

        std::ofstream os{outfilename.c_str()};
        यदि (!os) {
            v3error("Can't write file: " << outfilename);
            प्रत्यागम;
        }

        os << "//      // verilator_coverage annotation\n";

        पूर्णाङ्क lineno = 0;
        यावत् (!is.eof()) {
            lineno++;
            ध्रुव std::string पंक्ति = V3Os::getline(is);

            VlcSource::LinenoMap& पंक्तियाँ = source.पंक्तियाँ();
            ध्रुव स्वतः lit = पंक्तियाँ.find(lineno);
            यदि (lit == पंक्तियाँ.end()) {
                os << "        " << पंक्ति << '\n';
            } अन्यथा {
                Vlc_Source_गिनती& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ध्रुव तार्किक minOk = opt.गिनती_Ok(sc.min_गिनती());
                ध्रुव तार्किक maxOk = opt.गिनती_Ok(sc.max_गिनती());
                यदि (minOk) {
                    os << " ";
                } अन्यथा यदि (maxOk) {
                    os << "~";
                } अन्यथा {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_गिनती() << " " << पंक्ति << '\n';

                यदि (opt.annotatePoints()) {
                    प्रति (ध्रुव स्वतः& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                तार्किक printedFsmHeader = असत्य;
                प्रति (ध्रुव स्वतः& pit : sc.points()) {
                    यदि (!pit->isFsmState() && !pit->isFsmArc()) अनुवर्त;
                    यदि (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = सत्य;
                    }
                    os << (opt.गिनती_Ok(pit->गिनती()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->गिनती() << "        ";
                    यदि (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        यदि (pit->गिनती() == 0) os << " *** UNCOVERED ***";
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

शून्य Vlc_शीर्ष::annotate(ध्रुव string& स_नाम) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(स_नाम);
}

शून्य Vlc_शीर्ष::printTypeSummary() {
    TypeTally tally;
    प्रति (VlcPoints::ByName::मूल्य_type& i : m_points) {
        ध्रुव VlcPoint& pt = m_points.point_संख्या(i.second);
        यदि (isCovergroup(pt)) अनुवर्त;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.गिनती());
    }
    ध्रुव CgGroups groups = covergroups(m_points);
    यदि (!groups.empty()) {
        std::vector<ध्रुव CgGroup*> groupps;
        प्रति (ध्रुव CgGroups::मूल्य_type& it : groups) groupps.धकेलो_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    यदि (tally.empty()) प्रत्यागम;
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
    प्रति (VlcPoints::ByName::मूल्य_type& i : m_points) {
        ध्रुव VlcPoint& pt = m_points.point_संख्या(i.second);
        यदि (isCovergroup(pt)) अनुवर्त;  // Reported below, by covergroup
        ध्रुव string hier = reportHier(pt);
        यदि (hier.empty()) अनुवर्त;
        hasHier = सत्य;
        यदि (isCollapsedHier(hier)) hasCollapsedHier = सत्य;
        ध्रुव string type = displayType(pt);
        ध्रुव std::vector<string> parts = splitHier(hier);
        string path;
        प्रति (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.गिनती());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.गिनती());
    }
    ध्रुव std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    यदि (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        प्रत्यागम;
    }
    यदि (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        प्रत्यागम;
    }

    ध्रुव पूर्णाङ्क levels = opt.reportLevels();
    यदि (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    प्रति (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        ध्रुव std::vector<string> parts = splitHier(it->first);
        यदि (levels >= 0 && static_cast<पूर्णाङ्क>(parts.size()) > levels + 1) अनुवर्त;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, असत्य);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    प्रति (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, असत्य);
    }
    यदि (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
