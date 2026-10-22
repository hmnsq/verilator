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

#અન્તર્ભાવ "VlcTop.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Os.h"

#અન્તર્ભાવ "VlcOptions.h"

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <fstream>
#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <sstream>
#અન્તર્ભાવ <string>
#અન્તર્ભાવ <vector>

//######################################################################

નામાકાશ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
વર્ગ Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    તાર્કિક m_scored = ખોટું;  // Coverage is m_score, not the ratio of the points
    દ્વિગુણ m_score = 0.0;  // Percent coverage, if m_scored

જાહેર:
    // ACCESSORS
    uint64_t hit() અચલ { પાછા m_hit; }
    uint64_t total() અચલ { પાછા m_total; }
    તાર્કિક scored() અચલ { પાછા m_scored; }
    દ્વિગુણ score() અચલ { પાછા m_score; }
    શૂન્ય score(દ્વિગુણ કિંમત) {
        m_scored = સાચું;
        m_score = કિંમત;
    }
    // METHODS
    શૂન્ય addPoint(તાર્કિક covered) {
        જો (covered) ++m_hit;
        ++m_total;
    }
    શૂન્ય addPoints(અચલ Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

સ્થિર constexpr અચલ અક્ષર* અચલ s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
સ્થિર constexpr અચલ અક્ષર* અચલ s_covergroupType = "covergroup";
સ્થિર constexpr size_t s_summaryIndent = 2;
સ્થિર constexpr size_t s_reportRowIndent = 4;

string displayType(અચલ VlcPoint& point) {
    અચલ string type = point.type();
    પાછા type.empty() ? "point" : type;
}

તાર્કિક isCollapsedHier(અચલ string& hier) {
    પાછા hier.find('*') != string::npos || hier.find('?') != string::npos;
}

તાર્કિક isOrderedType(અચલ string& type) {
    માટે (અચલ અક્ષર* અચલ typep : s_orderedTypes) {
        જો (type == typep) પાછા સાચું;
    }
    પાછા ખોટું;
}

string reportHier(અચલ VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    જો (point.isFsmState() || point.isFsmArc()) {
        અચલ string fsmVar = point.fsmVarName();
        પાછા fsmVar.substr(0, fsmVar.rfind('.'));
    }
    પાછા point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(અચલ string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    પૂર્ણાંક depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    જ્યારે (pos < hier.size()) {
        અચલ અક્ષર c = hier[pos];
        જો (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            ચાલુ;
        } નહીંતર જો (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } નહીંતર જો (!depth && c == '.') {
            parts.ધકેલો_back(hier.substr(start, pos - start));
            start = pos + 1;
        } નહીંતર જો (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            ચાલુ;
        } નહીંતર જો (depth && c == '(') {
            ++depth;
        } નહીંતર જો (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    જો (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        માટે (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.ધકેલો_back(hier.substr(start, dot - start));
        }
    }
    parts.ધકેલો_back(hier.substr(start));
    પાછા parts;
}

string duName(અચલ VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    અચલ string page = point.page();
    પાછા page.substr(page.find('/') + 1);
}

શૂન્ય tallyPoint(TypeTally& tally, અચલ string& type, uint64_t ગણતરી) {
    tally[type].addPoint(ગણતરી > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
દ્વિગુણ pct(uint64_t hit, uint64_t total) {
    પાછા total ? (100.0 * static_cast<દ્વિગુણ>(hit) / static_cast<દ્વિગુણ>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_કિંમત_String(દ્વિગુણ કિંમત, તાર્કિક complete) {
    જો (!complete && કિંમત > 99.9) કિંમત = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << કિંમત << "%";
    પાછા os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    પાછા pct_કિંમત_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
વર્ગ CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_ગણતરી = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

જાહેર:
    // ACCESSORS
    અચલ string& name() અચલ { પાછા m_name; }
    તાર્કિક covered() અચલ { પાછા m_ગણતરી >= m_atLeast; }
    // METHODS
    શૂન્ય addRecord(અચલ string& name, uint64_t ગણતરી, uint64_t atLeast) {
        m_name = name;
        m_ગણતરી += ગણતરી;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
વર્ગ CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

જાહેર:
    // ACCESSORS
    uint64_t weight() અચલ { પાછા m_weight; }
    અચલ std::map<string, CgBin>& bins() અચલ { પાછા m_bins; }
    // METHODS
    શૂન્ય addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(અચલ string& identity) { પાછા m_bins[identity]; }
};

// A covergroup
વર્ગ CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_વસ્તુઓ;  // By hierarchy

જાહેર:
    // ACCESSORS
    અચલ string& hier() અચલ { પાછા m_hier; }
    uint64_t weight() અચલ { પાછા m_weight; }
    અચલ std::map<string, CgItem>& વસ્તુઓ() અચલ { પાછા m_વસ્તુઓ; }
    // METHODS
    શૂન્ય addRecord(અચલ string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(અચલ string& hier) { પાછા m_વસ્તુઓ[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t ચાવી_સંખ્યા(અચલ string& કિંમત) {
    પાછા કિંમત.empty() ? 1 : std::strtoull(કિંમત.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
તાર્કિક isScoreField(અચલ string& field) {
    માટે (અચલ અક્ષર* અચલ keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        અચલ string prefix = string{"\001"} + keyp + "\002";
        જો (field.compare(0, prefix.size(), prefix) == 0) પાછા સાચું;
    }
    પાછા ખોટું;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(અચલ string& recordName) {
    string identity;
    string::size_type start = 0;
    જ્યારે (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        જો (end == string::npos) end = recordName.size();
        અચલ string field = recordName.substr(start, end - start);
        જો (!isScoreField(field)) identity += field;
        start = end;
    }
    પાછા identity;
}

તાર્કિક isCovergroup(અચલ VlcPoint& point) { પાછા point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    માટે (અચલ VlcPoints::ByName::કિંમત_type& i : points) {
        અચલ VlcPoint& pt = points.point_સંખ્યા(i.second);
        જો (!isCovergroup(pt)) ચાલુ;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        અચલ string hier = pt.hier();
        string bin = pt.bin();
        જો (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        અચલ string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), ચાવી_સંખ્યા(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(ચાવી_સંખ્યા(pt.weight()));
        જો (!pt.binType().empty()) ચાલુ;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.ગણતરી(), ચાવી_સંખ્યા(pt.thresh()));
    }
    પાછા groups;
}

Tally itemTally(અચલ CgItem& item) {
    Tally tally;
    માટે (અચલ std::pair<અચલ string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    પાછા tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(અચલ CgGroup& group, તાર્કિક* contributesp = nullptr) {
    Tally tally;
    દ્વિગુણ weighted = 0.0;
    દ્વિગુણ weights = 0.0;
    માટે (અચલ std::pair<અચલ string, CgItem>& it : group.વસ્તુઓ()) {
        અચલ Tally item = itemTally(it.second);
        tally.addPoints(item);
        જો (!item.total()) ચાલુ;  // Excluded from both sums
        weighted += static_cast<દ્વિગુણ>(it.second.weight()) * item.score();
        weights += static_cast<દ્વિગુણ>(it.second.weight());
    }
    જો (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    પાછા tally;
}

Tally groupsTally(અચલ std::vector<અચલ CgGroup*>& groups) {
    Tally tally;
    દ્વિગુણ weighted = 0.0;
    દ્વિગુણ weights = 0.0;
    તાર્કિક anyWeight = ખોટું;
    માટે (અચલ CgGroup* અચલ groupp : groups) {
        તાર્કિક contributes = ખોટું;
        અચલ Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        જો (groupp->weight()) anyWeight = સાચું;
        જો (!contributes) ચાલુ;
        weighted += static_cast<દ્વિગુણ>(groupp->weight()) * group.score();
        weights += static_cast<દ્વિગુણ>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    પાછા tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(અચલ CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<અચલ CgGroup*>> ગાંઠ_Groups;
    માટે (અચલ CgGroups::કિંમત_type& it : groups) {
        અચલ CgGroup& group = it.second;
        string path;
        માટે (અચલ string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            ગાંઠ_Groups[path].ધકેલો_back(&group);
        }
        માટે (અચલ std::pair<અચલ string, CgItem>& item : group.વસ્તુઓ()) {
            tallies[item.first] = itemTally(item.second);
            માટે (અચલ std::pair<અચલ string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    માટે (અચલ std::pair<અચલ string, std::vector<અચલ CgGroup*>>& it : ગાંઠ_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    પાછા tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
શૂન્ય printIndent(size_t indent) {
    માટે (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

શૂન્ય printTallyRow(અચલ string& type, અચલ Tally& tally, size_t indent, size_t typeWidth,
                   size_t ગણતરી_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    અચલ string percent = tally.scored() ? pct_કિંમત_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::ડાબું << std::setw(typeWidth) << type << " : " << std::જમણું << std::fixed
              << std::setw(6) << percent << " (" << std::setw(ગણતરી_Width) << tally.hit() << "/"
              << std::setw(ગણતરી_Width) << tally.total() << ")\n";
}

size_t ગણતરી_Width(અચલ TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    માટે (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    પાછા width;
}

size_t typeWidth(અચલ TypeTally& tally) {
    size_t typeWidth = 0;
    માટે (અચલ અક્ષર* અચલ typep : s_orderedTypes) {
        અચલ string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    માટે (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    પાછા typeWidth;
}

શૂન્ય printTypeTally(અચલ TypeTally& tally, size_t indent, તાર્કિક includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    અચલ size_t typWidth = typeWidth(tally);
    અચલ size_t cntWidth = ગણતરી_Width(tally);
    માટે (અચલ અક્ષર* અચલ typep : s_orderedTypes) {
        અચલ string type = typep;
        અચલ TypeTally::const_iterator it = tally.find(type);
        જો (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } નહીંતર જો (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    માટે (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        જો (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
શૂન્ય printCovergroupTallies(અચલ std::map<string, Tally>& tallies, પૂર્ણાંક levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    માટે (અચલ std::pair<અચલ string, Tally>& it : tallies) {
        જો (levels >= 0 && static_cast<પૂર્ણાંક>(splitHier(it.first).size()) > levels + 1) ચાલુ;
        shown.ઉમેરો(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    અચલ size_t cntWidth = ગણતરી_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    માટે (અચલ std::pair<અચલ string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

શૂન્ય Vlc_ટોચ::readCoverage(અચલ string& filename, તાર્કિક nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    જો (!is) {
        જો (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        પાછા;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* અચલ testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    જ્યારે (!is.eof()) {
        અચલ string લીટી = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        જો (લીટી[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            અચલ string::size_type secspace = લીટી.rfind("' ");
            જો (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                ચાલુ;
            }
            અચલ string point = લીટી.substr(3, secspace - 3);
            જો (!opt.isTypeMatch(point.c_str())) ચાલુ;

            અચલ uint64_t hits = std::atoll(લીટી.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            અચલ uint64_t pointnum = points().findAddPoint(point, hits);
            જો (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                અચલ uint64_t needed = std::max(
                    VlcBuckets::sufficient(), ચાવી_સંખ્યા(points().point_સંખ્યા(pointnum).thresh()));
                જો (hits >= needed) {
                    points().point_સંખ્યા(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

શૂન્ય Vlc_ટોચ::writeCoverage(અચલ string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    જો (!os) {
        v3fatal("Can't write file: " << filename);
        પાછા;
    }

    os << "# SystemC::Coverage-3\n";
    માટે (અચલ auto& i : m_points) {
        અચલ VlcPoint& point = m_points.point_સંખ્યા(i.second);
        os << "C '" << point.name() << "' " << point.ગણતરી() << '\n';
    }
}

શૂન્ય Vlc_ટોચ::writeInfo(અચલ string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    જો (!os) {
        v3fatal("Can't write file: " << filename);
        પાછા;
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
    માટે (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& લીટીઓ = source.લીટીઓ();
        પૂર્ણાંક branchesFound = 0;
        પૂર્ણાંક branchesHit = 0;
        માટે (auto& li : લીટીઓ) {
            Vlc_Source_ગણતરી& sc = li.second;
            uint64_t da_ગણતરી = 0;
            std::vector<અચલ VlcPoint*> infoPoints;
            માટે (અચલ auto& point : sc.points()) {
                da_ગણતરી = std::max(da_ગણતરી, point->ગણતરી());
                infoPoints.ધકેલો_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_ગણતરી << "\n";
            જો (infoPoints.size() <= 1) ચાલુ;
            branchesFound += static_cast<પૂર્ણાંક>(infoPoints.size());
            પૂર્ણાંક point_અંક_2 = 0;
            માટે (અચલ VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                જો (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } નહીંતર જો (point->comment().empty()) {
                    os << "0,";
                    os << point_અંક_2;
                } નહીંતર {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->ગણતરી() << "\n";

                branchesHit += opt.ગણતરી_Ok(point->ગણતરી());
                ++point_અંક_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

સંરચના CmpComputrons final {
    તાર્કિક operator()(અચલ VlcTest* lhsp, અચલ VlcTest* rhsp) અચલ {
        જો (lhsp->computrons() != rhsp->computrons()) {
            પાછા lhsp->computrons() < rhsp->computrons();
        }
        પાછા lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

શૂન્ય Vlc_ટોચ::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    માટે (અચલ auto& testp : m_tests) {
        જો (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ધકેલો_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    માટે (અચલ auto& i : m_points) {
        અચલ VlcPoint* અચલ pointp = &points().point_સંખ્યા(i.second);
        // If any tests hit this point, then we'll need to cover it.
        જો (pointp->testsCovering()) remaining.addData(pointp->point_અંક(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    જ્યારે (સાચું) {
        જો (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        માટે (અચલ auto& testp : bytime) {
            જો (!testp->rank()) {
                અચલ uint64_t remain = testp->buckets().data_કાઢો_ગણતરી(remaining);
                જો (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        જો (VlcTest* અચલ testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } નહીંતર {
            તોડો;  // No test covering more stuff found
        }
    }
}

શૂન્ય Vlc_ટોચ::annotateCalc() {
    // Calculate per-line information into filedata structure
    માટે (અચલ auto& i : m_points) {
        અચલ VlcPoint& point = m_points.point_સંખ્યા(i.second);
        અચલ string filename = point.filename();
        અચલ પૂર્ણાંક lineno = point.lineno();
        જો (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.ગણતરી() << " " << point.linescov());
            // Base coverage
            source.ઉમેરો_Point(lineno, &point);
            // Additional lines covered by this statement
            તાર્કિક range = ખોટું;
            પૂર્ણાંક start = 0;
            પૂર્ણાંક end = 0;
            અચલ string linescov = point.linescov();
            માટે (અચલ અક્ષર* covp = linescov.c_str(); સાચું; ++covp) {
                જો (!*covp || *covp == ',') {  // Ending
                    માટે (પૂર્ણાંક lni = start; start && lni <= end; ++lni) {
                        source.ઉમેરો_Point(lni, &point);
                    }
                    જો (!*covp) તોડો;
                    start = 0;  // Prep for next
                    end = 0;
                    range = ખોટું;
                } નહીંતર જો (*covp == '-') {
                    range = સાચું;
                } નહીંતર જો (std::isdigit(*covp)) {
                    અચલ અક્ષર* અચલ digitsp = covp;
                    જ્યારે (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    જો (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

શૂન્ય Vlc_ટોચ::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    પૂર્ણાંક totCases = 0;
    પૂર્ણાંક totOk = 0;
    માટે (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        જો (opt.annotateAll()) source.needed(સાચું);
        અચલ VlcSource::LinenoMap& લીટીઓ = source.લીટીઓ();
        માટે (auto& li : લીટીઓ) {
            અચલ Vlc_Source_ગણતરી& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            જો (opt.ગણતરી_Ok(sc.min_ગણતરી())) {
                ++totOk;
            } નહીંતર {
                source.needed(સાચું);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    જો (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

શૂન્ય Vlc_ટોચ::annotateOutputFiles(અચલ string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    માટે (auto& si : m_sources) {
        VlcSource& source = si.second;
        જો (!source.needed()) ચાલુ;
        અચલ string filename = source.name();
        અચલ string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        જો (!is) {
            v3error("Can't read annotation file: " << filename);
            પાછા;
        }

        std::ofstream os{outfilename.c_str()};
        જો (!os) {
            v3error("Can't write file: " << outfilename);
            પાછા;
        }

        os << "//      // verilator_coverage annotation\n";

        પૂર્ણાંક lineno = 0;
        જ્યારે (!is.eof()) {
            lineno++;
            અચલ std::string લીટી = V3Os::getline(is);

            VlcSource::LinenoMap& લીટીઓ = source.લીટીઓ();
            અચલ auto lit = લીટીઓ.find(lineno);
            જો (lit == લીટીઓ.end()) {
                os << "        " << લીટી << '\n';
            } નહીંતર {
                Vlc_Source_ગણતરી& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                અચલ તાર્કિક minOk = opt.ગણતરી_Ok(sc.min_ગણતરી());
                અચલ તાર્કિક maxOk = opt.ગણતરી_Ok(sc.max_ગણતરી());
                જો (minOk) {
                    os << " ";
                } નહીંતર જો (maxOk) {
                    os << "~";
                } નહીંતર {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_ગણતરી() << " " << લીટી << '\n';

                જો (opt.annotatePoints()) {
                    માટે (અચલ auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                તાર્કિક printedFsmHeader = ખોટું;
                માટે (અચલ auto& pit : sc.points()) {
                    જો (!pit->isFsmState() && !pit->isFsmArc()) ચાલુ;
                    જો (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = સાચું;
                    }
                    os << (opt.ગણતરી_Ok(pit->ગણતરી()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->ગણતરી() << "        ";
                    જો (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        જો (pit->ગણતરી() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } નહીંતર જો (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } નહીંતર {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        જો (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

શૂન્ય Vlc_ટોચ::annotate(અચલ string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

શૂન્ય Vlc_ટોચ::printTypeSummary() {
    TypeTally tally;
    માટે (VlcPoints::ByName::કિંમત_type& i : m_points) {
        અચલ VlcPoint& pt = m_points.point_સંખ્યા(i.second);
        જો (isCovergroup(pt)) ચાલુ;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.ગણતરી());
    }
    અચલ CgGroups groups = covergroups(m_points);
    જો (!groups.empty()) {
        std::vector<અચલ CgGroup*> groupps;
        માટે (અચલ CgGroups::કિંમત_type& it : groups) groupps.ધકેલો_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    જો (tally.empty()) પાછા;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, સાચું);
}

શૂન્ય Vlc_ટોચ::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    તાર્કિક hasHier = ખોટું;
    તાર્કિક hasCollapsedHier = ખોટું;
    માટે (VlcPoints::ByName::કિંમત_type& i : m_points) {
        અચલ VlcPoint& pt = m_points.point_સંખ્યા(i.second);
        જો (isCovergroup(pt)) ચાલુ;  // Reported below, by covergroup
        અચલ string hier = reportHier(pt);
        જો (hier.empty()) ચાલુ;
        hasHier = સાચું;
        જો (isCollapsedHier(hier)) hasCollapsedHier = સાચું;
        અચલ string type = displayType(pt);
        અચલ std::vector<string> parts = splitHier(hier);
        string path;
        માટે (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.ગણતરી());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.ગણતરી());
    }
    અચલ std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    જો (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        પાછા;
    }
    જો (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        પાછા;
    }

    અચલ પૂર્ણાંક levels = opt.reportLevels();
    જો (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    માટે (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        અચલ std::vector<string> parts = splitHier(it->first);
        જો (levels >= 0 && static_cast<પૂર્ણાંક>(parts.size()) > levels + 1) ચાલુ;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, ખોટું);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    માટે (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, ખોટું);
    }
    જો (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
