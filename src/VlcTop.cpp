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

#அந்தர்பாவ "VlcTop.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Os.h"

#அந்தர்பாவ "VlcOptions.h"

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <fstream>
#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <sstream>
#அந்தர்பாவ <string>
#அந்தர்பாவ <vector>

//######################################################################

நாமாகாஶ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
வகுப்பு Tally final {
    // MEMBERS
    uint64_t m_hit = 0;  // Covered points
    uint64_t m_total = 0;  // Points
    தருக்க m_scored = பொய்;  // Coverage is m_score, not the ratio of the points
    இரட்டை m_score = 0.0;  // Percent coverage, if m_scored

பொது:
    // ACCESSORS
    uint64_t hit() மாறிலி { திரும்பு m_hit; }
    uint64_t total() மாறிலி { திரும்பு m_total; }
    தருக்க scored() மாறிலி { திரும்பு m_scored; }
    இரட்டை score() மாறிலி { திரும்பு m_score; }
    வெறுமை score(இரட்டை மதிப்பு) {
        m_scored = உண்மை;
        m_score = மதிப்பு;
    }
    // METHODS
    வெறுமை addPoint(தருக்க covered) {
        எனில் (covered) ++m_hit;
        ++m_total;
    }
    வெறுமை addPoints(மாறிலி Tally& other) {
        m_hit += other.m_hit;
        m_total += other.m_total;
    }
};

// Map coverage type to its tally.
using TypeTally = std::map<std::string, Tally>;

நிலையான constexpr மாறிலி எழுத்து* மாறிலி s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
நிலையான constexpr மாறிலி எழுத்து* மாறிலி s_covergroupType = "covergroup";
நிலையான constexpr size_t s_summaryIndent = 2;
நிலையான constexpr size_t s_reportRowIndent = 4;

string displayType(மாறிலி VlcPoint& point) {
    மாறிலி string type = point.type();
    திரும்பு type.empty() ? "point" : type;
}

தருக்க isCollapsedHier(மாறிலி string& hier) {
    திரும்பு hier.find('*') != string::npos || hier.find('?') != string::npos;
}

தருக்க isOrderedType(மாறிலி string& type) {
    க்கு (மாறிலி எழுத்து* மாறிலி typep : s_orderedTypes) {
        எனில் (type == typep) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}

string reportHier(மாறிலி VlcPoint& point) {
    // FSM records currently have useful instance scope in Fv.
    எனில் (point.isFsmState() || point.isFsmArc()) {
        மாறிலி string fsmVar = point.fsmVarName();
        திரும்பு fsmVar.substr(0, fsmVar.rfind('.'));
    }
    திரும்பு point.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::vector<string> splitHier(மாறிலி string& hier) {
    std::vector<string> parts;
    string::size_type start = 0;
    முழு depth = 0;  // Of the parentheses of the values of parameters
    string::size_type pos = 0;
    போது (pos < hier.size()) {
        மாறிலி எழுத்து c = hier[pos];
        எனில் (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            தொடர்;
        } இல்லையெனில் எனில் (!depth && hier.compare(pos, 2, "#(") == 0) {
            depth = 1;
            ++pos;
        } இல்லையெனில் எனில் (!depth && c == '.') {
            parts.தள்ளு_back(hier.substr(start, pos - start));
            start = pos + 1;
        } இல்லையெனில் எனில் (depth && c == '"') {
            pos = VString::quotedEnd(hier, pos);  // npos if not terminated
            தொடர்;
        } இல்லையெனில் எனில் (depth && c == '(') {
            ++depth;
        } இல்லையெனில் எனில் (depth && c == ')') {
            --depth;
        }
        ++pos;
    }
    எனில் (depth || pos != hier.size()) {  // Unbalanced
        parts.clear();
        start = 0;
        க்கு (string::size_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.தள்ளு_back(hier.substr(start, dot - start));
        }
    }
    parts.தள்ளு_back(hier.substr(start));
    திரும்பு parts;
}

string duName(மாறிலி VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    மாறிலி string page = point.page();
    திரும்பு page.substr(page.find('/') + 1);
}

வெறுமை tallyPoint(TypeTally& tally, மாறிலி string& type, uint64_t மொத்தம்) {
    tally[type].addPoint(மொத்தம் > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
இரட்டை pct(uint64_t hit, uint64_t total) {
    திரும்பு total ? (100.0 * static_cast<இரட்டை>(hit) / static_cast<இரட்டை>(total)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_மதிப்பு_String(இரட்டை மதிப்பு, தருக்க complete) {
    எனில் (!complete && மதிப்பு > 99.9) மதிப்பு = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << மதிப்பு << "%";
    திரும்பு os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t hit, uint64_t total) {
    திரும்பு pct_மதிப்பு_String(pct(hit, total), hit >= total);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
வகுப்பு CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_மொத்தம் = 0;  // Hits, of all the records
    uint64_t m_atLeast = 0;  // option.at_least, the largest of the records

பொது:
    // ACCESSORS
    மாறிலி string& name() மாறிலி { திரும்பு m_name; }
    தருக்க covered() மாறிலி { திரும்பு m_மொத்தம் >= m_atLeast; }
    // METHODS
    வெறுமை addRecord(மாறிலி string& name, uint64_t மொத்தம், uint64_t atLeast) {
        m_name = name;
        m_மொத்தம் += மொத்தம்;
        m_atLeast = std::max(m_atLeast, atLeast);
    }
};

// A coverpoint or cross
வகுப்பு CgItem final {
    // MEMBERS
    uint64_t m_weight = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

பொது:
    // ACCESSORS
    uint64_t weight() மாறிலி { திரும்பு m_weight; }
    மாறிலி std::map<string, CgBin>& bins() மாறிலி { திரும்பு m_bins; }
    // METHODS
    வெறுமை addRecord(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(மாறிலி string& identity) { திரும்பு m_bins[identity]; }
};

// A covergroup
வகுப்பு CgGroup final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_பொருட்கள்;  // By hierarchy

பொது:
    // ACCESSORS
    மாறிலி string& hier() மாறிலி { திரும்பு m_hier; }
    uint64_t weight() மாறிலி { திரும்பு m_weight; }
    மாறிலி std::map<string, CgItem>& பொருட்கள்() மாறிலி { திரும்பு m_பொருட்கள்; }
    // METHODS
    வெறுமை addRecord(மாறிலி string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(மாறிலி string& hier) { திரும்பு m_பொருட்கள்[hier]; }
};
using CgGroups = std::map<string, CgGroup>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t சாவி_எண்(மாறிலி string& மதிப்பு) {
    திரும்பு மதிப்பு.empty() ? 1 : std::strtoull(மதிப்பு.c_str(), nullptr, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
தருக்க isScoreField(மாறிலி string& field) {
    க்கு (மாறிலி எழுத்து* மாறிலி keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_GROUP_WEIGHT}) {
        மாறிலி string prefix = string{"\001"} + keyp + "\002";
        எனில் (field.compare(0, prefix.size(), prefix) == 0) திரும்பு உண்மை;
    }
    திரும்பு பொய்;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string binIdentity(மாறிலி string& recordName) {
    string identity;
    string::size_type start = 0;
    போது (start < recordName.size()) {
        string::size_type end = recordName.find('\001', start + 1);
        எனில் (end == string::npos) end = recordName.size();
        மாறிலி string field = recordName.substr(start, end - start);
        எனில் (!isScoreField(field)) identity += field;
        start = end;
    }
    திரும்பு identity;
}

தருக்க isCovergroup(மாறிலி VlcPoint& point) { திரும்பு point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    க்கு (மாறிலி VlcPoints::ByName::மதிப்பு_type& i : points) {
        மாறிலி VlcPoint& pt = points.point_எண்(i.second);
        எனில் (!isCovergroup(pt)) தொடர்;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        மாறிலி string hier = pt.hier();
        string bin = pt.bin();
        எனில் (bin.empty() || bin.size() >= hier.size()) bin = hier.substr(hier.rfind('.') + 1);
        மாறிலி string itemHier
            = hier.substr(0, hier.size() - std::min(hier.size(), bin.size() + 1));
        CgGroup& group = groups[pt.page()];
        group.addRecord(itemHier.substr(0, itemHier.rfind('.')), சாவி_எண்(pt.groupWeight()));
        CgItem& item = group.findNewItem(itemHier);
        item.addRecord(சாவி_எண்(pt.weight()));
        எனில் (!pt.binType().empty()) தொடர்;  // Not coverable
        item.findNewBin(binIdentity(pt.name())).addRecord(bin, pt.மொத்தம்(), சாவி_எண்(pt.thresh()));
    }
    திரும்பு groups;
}

Tally itemTally(மாறிலி CgItem& item) {
    Tally tally;
    க்கு (மாறிலி std::pair<மாறிலி string, CgBin>& bin : item.bins()) {
        tally.addPoint(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.total() ? pct(tally.hit(), tally.total()) : item.weight() ? 0.0 : 100.0);
    திரும்பு tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally groupTally(மாறிலி CgGroup& group, தருக்க* contributesp = nullptr) {
    Tally tally;
    இரட்டை weighted = 0.0;
    இரட்டை weights = 0.0;
    க்கு (மாறிலி std::pair<மாறிலி string, CgItem>& it : group.பொருட்கள்()) {
        மாறிலி Tally item = itemTally(it.second);
        tally.addPoints(item);
        எனில் (!item.total()) தொடர்;  // Excluded from both sums
        weighted += static_cast<இரட்டை>(it.second.weight()) * item.score();
        weights += static_cast<இரட்டை>(it.second.weight());
    }
    எனில் (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : group.weight() ? 0.0 : 100.0);
    திரும்பு tally;
}

Tally groupsTally(மாறிலி std::vector<மாறிலி CgGroup*>& groups) {
    Tally tally;
    இரட்டை weighted = 0.0;
    இரட்டை weights = 0.0;
    தருக்க anyWeight = பொய்;
    க்கு (மாறிலி CgGroup* மாறிலி groupp : groups) {
        தருக்க contributes = பொய்;
        மாறிலி Tally group = groupTally(*groupp, &contributes);
        tally.addPoints(group);
        எனில் (groupp->weight()) anyWeight = உண்மை;
        எனில் (!contributes) தொடர்;
        weighted += static_cast<இரட்டை>(groupp->weight()) * group.score();
        weights += static_cast<இரட்டை>(groupp->weight());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : anyWeight ? 0.0 : 100.0);
    திரும்பு tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(மாறிலி CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::vector<மாறிலி CgGroup*>> முனை_Groups;
    க்கு (மாறிலி CgGroups::மதிப்பு_type& it : groups) {
        மாறிலி CgGroup& group = it.second;
        string path;
        க்கு (மாறிலி string& part : splitHier(group.hier())) {
            path = path.empty() ? part : path + "." + part;
            முனை_Groups[path].தள்ளு_back(&group);
        }
        க்கு (மாறிலி std::pair<மாறிலி string, CgItem>& item : group.பொருட்கள்()) {
            tallies[item.first] = itemTally(item.second);
            க்கு (மாறிலி std::pair<மாறிலி string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].addPoint(bin.second.covered());
            }
        }
    }
    க்கு (மாறிலி std::pair<மாறிலி string, std::vector<மாறிலி CgGroup*>>& it : முனை_Groups) {
        tallies[it.first]
            = it.second.size() == 1 ? groupTally(*it.second.front()) : groupsTally(it.second);
    }
    திரும்பு tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
வெறுமை printIndent(size_t indent) {
    க்கு (size_t i = 0; i < indent; ++i) std::cout << ' ';
}

வெறுமை printTallyRow(மாறிலி string& type, மாறிலி Tally& tally, size_t indent, size_t typeWidth,
                   size_t மொத்தம்_Width) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    மாறிலி string percent = tally.scored() ? pct_மதிப்பு_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.hit(), tally.total());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::cout << std::இடது << std::setw(typeWidth) << type << " : " << std::வலது << std::fixed
              << std::setw(6) << percent << " (" << std::setw(மொத்தம்_Width) << tally.hit() << "/"
              << std::setw(மொத்தம்_Width) << tally.total() << ")\n";
}

size_t மொத்தம்_Width(மாறிலி TypeTally& tally) {
    size_t width = cvtToStr(0).size();
    க்கு (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        width = std::max(width, cvtToStr(it->second.hit()).size());
        width = std::max(width, cvtToStr(it->second.total()).size());
    }
    திரும்பு width;
}

size_t typeWidth(மாறிலி TypeTally& tally) {
    size_t typeWidth = 0;
    க்கு (மாறிலி எழுத்து* மாறிலி typep : s_orderedTypes) {
        மாறிலி string type = typep;
        typeWidth = std::max(typeWidth, type.size());
    }
    க்கு (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        typeWidth = std::max(typeWidth, it->first.size());
    }
    திரும்பு typeWidth;
}

வெறுமை printTypeTally(மாறிலி TypeTally& tally, size_t indent, தருக்க includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    மாறிலி size_t typWidth = typeWidth(tally);
    மாறிலி size_t cntWidth = மொத்தம்_Width(tally);
    க்கு (மாறிலி எழுத்து* மாறிலி typep : s_orderedTypes) {
        மாறிலி string type = typep;
        மாறிலி TypeTally::const_iterator it = tally.find(type);
        எனில் (it != tally.end()) {
            printTallyRow(type, it->second, indent, typWidth, cntWidth);
        } இல்லையெனில் எனில் (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typWidth, cntWidth);
        }
    }
    க்கு (TypeTally::const_iterator it = tally.begin(); it != tally.end(); ++it) {
        எனில் (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typWidth, cntWidth);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
வெறுமை printCovergroupTallies(மாறிலி std::map<string, Tally>& tallies, முழு levels) {
    std::map<string, Tally> shown;
    size_t nameWidth = 0;
    க்கு (மாறிலி std::pair<மாறிலி string, Tally>& it : tallies) {
        எனில் (levels >= 0 && static_cast<முழு>(splitHier(it.first).size()) > levels + 1) தொடர்;
        shown.செருகு(it);
        nameWidth = std::max(nameWidth, it.first.size());
    }
    மாறிலி size_t cntWidth = மொத்தம்_Width(shown);
    std::cout << "Covergroup Coverage Summary:\n";
    க்கு (மாறிலி std::pair<மாறிலி string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, nameWidth, cntWidth);
    }
}

}  // namespace

வெறுமை Vlc_உச்சி::readCoverage(மாறிலி string& filename, தருக்க nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    எனில் (!is) {
        எனில் (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        திரும்பு;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* மாறிலி testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    போது (!is.eof()) {
        மாறிலி string வரி = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        எனில் (வரி[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            மாறிலி string::size_type secspace = வரி.rfind("' ");
            எனில் (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                தொடர்;
            }
            மாறிலி string point = வரி.substr(3, secspace - 3);
            எனில் (!opt.isTypeMatch(point.c_str())) தொடர்;

            மாறிலி uint64_t hits = std::atoll(வரி.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            மாறிலி uint64_t pointnum = points().findAddPoint(point, hits);
            எனில் (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                மாறிலி uint64_t needed = std::max(
                    VlcBuckets::sufficient(), சாவி_எண்(points().point_எண்(pointnum).thresh()));
                எனில் (hits >= needed) {
                    points().point_எண்(pointnum).testsCoveringInc();
                    testp->buckets().addData(pointnum, hits);
                }
            }
        }
    }
}

வெறுமை Vlc_உச்சி::writeCoverage(மாறிலி string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    எனில் (!os) {
        v3fatal("Can't write file: " << filename);
        திரும்பு;
    }

    os << "# SystemC::Coverage-3\n";
    க்கு (மாறிலி auto& i : m_points) {
        மாறிலி VlcPoint& point = m_points.point_எண்(i.second);
        os << "C '" << point.name() << "' " << point.மொத்தம்() << '\n';
    }
}

வெறுமை Vlc_உச்சி::writeInfo(மாறிலி string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    எனில் (!os) {
        v3fatal("Can't write file: " << filename);
        திரும்பு;
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
    க்கு (auto& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& வரிகள் = source.வரிகள்();
        முழு branchesFound = 0;
        முழு branchesHit = 0;
        க்கு (auto& li : வரிகள்) {
            Vlc_Source_மொத்தம்& sc = li.second;
            uint64_t da_மொத்தம் = 0;
            std::vector<மாறிலி VlcPoint*> infoPoints;
            க்கு (மாறிலி auto& point : sc.points()) {
                da_மொத்தம் = std::max(da_மொத்தம், point->மொத்தம்());
                infoPoints.தள்ளு_back(point);
            }
            os << "DA:" << sc.lineno() << "," << da_மொத்தம் << "\n";
            எனில் (infoPoints.size() <= 1) தொடர்;
            branchesFound += static_cast<முழு>(infoPoints.size());
            முழு point_இலக்கம்_2 = 0;
            க்கு (மாறிலி VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                எனில் (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsmToState();
                } இல்லையெனில் எனில் (point->comment().empty()) {
                    os << "0,";
                    os << point_இலக்கம்_2;
                } இல்லையெனில் {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.end(), ',', '_');
                    os << comment;
                }
                os << "," << point->மொத்தம்() << "\n";

                branchesHit += opt.மொத்தம்_Ok(point->மொத்தம்());
                ++point_இலக்கம்_2;
            }
        }
        os << "BRF:" << branchesFound << '\n';
        os << "BRH:" << branchesHit << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

கட்டமைப்பு CmpComputrons final {
    தருக்க operator()(மாறிலி VlcTest* lhsp, மாறிலி VlcTest* rhsp) மாறிலி {
        எனில் (lhsp->computrons() != rhsp->computrons()) {
            திரும்பு lhsp->computrons() < rhsp->computrons();
        }
        திரும்பு lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

வெறுமை Vlc_உச்சி::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::vector<VlcTest*> bytime;
    க்கு (மாறிலி auto& testp : m_tests) {
        எனில் (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.தள்ளு_back(testp);
        }
    }
    sort(bytime.begin(), bytime.end(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    க்கு (மாறிலி auto& i : m_points) {
        மாறிலி VlcPoint* மாறிலி pointp = &points().point_எண்(i.second);
        // If any tests hit this point, then we'll need to cover it.
        எனில் (pointp->testsCovering()) remaining.addData(pointp->point_இலக்கம்(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    போது (உண்மை) {
        எனில் (debug() >= 9) {
            UINFO_PREFIX("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = nullptr;
        uint64_t bestRemain = 0;
        க்கு (மாறிலி auto& testp : bytime) {
            எனில் (!testp->rank()) {
                மாறிலி uint64_t remain = testp->buckets().data_எடு_மொத்தம்(remaining);
                எனில் (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        எனில் (VlcTest* மாறிலி testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.orData(bestTestp->buckets());
        } இல்லையெனில் {
            நிறுத்து;  // No test covering more stuff found
        }
    }
}

வெறுமை Vlc_உச்சி::annotateCalc() {
    // Calculate per-line information into filedata structure
    க்கு (மாறிலி auto& i : m_points) {
        மாறிலி VlcPoint& point = m_points.point_எண்(i.second);
        மாறிலி string filename = point.filename();
        மாறிலி முழு lineno = point.lineno();
        எனில் (!filename.empty() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.மொத்தம்() << " " << point.linescov());
            // Base coverage
            source.செருகு_Point(lineno, &point);
            // Additional lines covered by this statement
            தருக்க range = பொய்;
            முழு start = 0;
            முழு end = 0;
            மாறிலி string linescov = point.linescov();
            க்கு (மாறிலி எழுத்து* covp = linescov.c_str(); உண்மை; ++covp) {
                எனில் (!*covp || *covp == ',') {  // Ending
                    க்கு (முழு lni = start; start && lni <= end; ++lni) {
                        source.செருகு_Point(lni, &point);
                    }
                    எனில் (!*covp) நிறுத்து;
                    start = 0;  // Prep for next
                    end = 0;
                    range = பொய்;
                } இல்லையெனில் எனில் (*covp == '-') {
                    range = உண்மை;
                } இல்லையெனில் எனில் (std::isdigit(*covp)) {
                    மாறிலி எழுத்து* மாறிலி digitsp = covp;
                    போது (std::isdigit(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    எனில் (!range) start = std::atoi(digitsp);
                    end = std::atoi(digitsp);
                }
            }
        }
    }
}

வெறுமை Vlc_உச்சி::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    முழு totCases = 0;
    முழு totOk = 0;
    க்கு (auto& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        எனில் (opt.annotateAll()) source.needed(உண்மை);
        மாறிலி VlcSource::LinenoMap& வரிகள் = source.வரிகள்();
        க்கு (auto& li : வரிகள்) {
            மாறிலி Vlc_Source_மொத்தம்& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            எனில் (opt.மொத்தம்_Ok(sc.min_மொத்தம்())) {
                ++totOk;
            } இல்லையெனில் {
                source.needed(உண்மை);
            }
        }
    }
    std::cout << "Annotation Summary:\n";
    std::cout << "  lines with all attached points covered : ";
    std::cout << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    எனில் (totOk != totCases) cout << "See lines with '%00' in " << opt.annotateOut() << '\n';
}

வெறுமை Vlc_உச்சி::annotateOutputFiles(மாறிலி string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    க்கு (auto& si : m_sources) {
        VlcSource& source = si.second;
        எனில் (!source.needed()) தொடர்;
        மாறிலி string filename = source.name();
        மாறிலி string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        எனில் (!is) {
            v3error("Can't read annotation file: " << filename);
            திரும்பு;
        }

        std::ofstream os{outfilename.c_str()};
        எனில் (!os) {
            v3error("Can't write file: " << outfilename);
            திரும்பு;
        }

        os << "//      // verilator_coverage annotation\n";

        முழு lineno = 0;
        போது (!is.eof()) {
            lineno++;
            மாறிலி std::string வரி = V3Os::getline(is);

            VlcSource::LinenoMap& வரிகள் = source.வரிகள்();
            மாறிலி auto lit = வரிகள்.find(lineno);
            எனில் (lit == வரிகள்.end()) {
                os << "        " << வரி << '\n';
            } இல்லையெனில் {
                Vlc_Source_மொத்தம்& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                மாறிலி தருக்க minOk = opt.மொத்தம்_Ok(sc.min_மொத்தம்());
                மாறிலி தருக்க maxOk = opt.மொத்தம்_Ok(sc.max_மொத்தம்());
                எனில் (minOk) {
                    os << " ";
                } இல்லையெனில் எனில் (maxOk) {
                    os << "~";
                } இல்லையெனில் {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_மொத்தம்() << " " << வரி << '\n';

                எனில் (opt.annotatePoints()) {
                    க்கு (மாறிலி auto& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                தருக்க printedFsmHeader = பொய்;
                க்கு (மாறிலி auto& pit : sc.points()) {
                    எனில் (!pit->isFsmState() && !pit->isFsmArc()) தொடர்;
                    எனில் (!printedFsmHeader) {
                        os << "        // [FSM coverage]\n";
                        printedFsmHeader = உண்மை;
                    }
                    os << (opt.மொத்தம்_Ok(pit->மொத்தம்()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->மொத்தம்() << "        ";
                    எனில் (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        எனில் (pit->மொத்தம்() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } இல்லையெனில் எனில் (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } இல்லையெனில் {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        எனில் (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

வெறுமை Vlc_உச்சி::annotate(மாறிலி string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

வெறுமை Vlc_உச்சி::printTypeSummary() {
    TypeTally tally;
    க்கு (VlcPoints::ByName::மதிப்பு_type& i : m_points) {
        மாறிலி VlcPoint& pt = m_points.point_எண்(i.second);
        எனில் (isCovergroup(pt)) தொடர்;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.மொத்தம்());
    }
    மாறிலி CgGroups groups = covergroups(m_points);
    எனில் (!groups.empty()) {
        std::vector<மாறிலி CgGroup*> groupps;
        க்கு (மாறிலி CgGroups::மதிப்பு_type& it : groups) groupps.தள்ளு_back(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    எனில் (tally.empty()) திரும்பு;
    std::cout << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, உண்மை);
}

வெறுமை Vlc_உச்சி::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    தருக்க hasHier = பொய்;
    தருக்க hasCollapsedHier = பொய்;
    க்கு (VlcPoints::ByName::மதிப்பு_type& i : m_points) {
        மாறிலி VlcPoint& pt = m_points.point_எண்(i.second);
        எனில் (isCovergroup(pt)) தொடர்;  // Reported below, by covergroup
        மாறிலி string hier = reportHier(pt);
        எனில் (hier.empty()) தொடர்;
        hasHier = உண்மை;
        எனில் (isCollapsedHier(hier)) hasCollapsedHier = உண்மை;
        மாறிலி string type = displayType(pt);
        மாறிலி std::vector<string> parts = splitHier(hier);
        string path;
        க்கு (std::vector<string>::const_iterator it = parts.begin(); it != parts.end(); ++it) {
            path = path.empty() ? *it : path + "." + *it;
            tallyPoint(hierTallies[path], type, pt.மொத்தம்());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.மொத்தம்());
    }
    மாறிலி std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    எனில் (!hasHier && !cgTallies.empty()) {
        printCovergroupTallies(cgTallies, opt.reportLevels());
        திரும்பு;
    }
    எனில் (!hasHier) {
        std::cout << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        திரும்பு;
    }

    மாறிலி முழு levels = opt.reportLevels();
    எனில் (hasCollapsedHier) {
        std::cout << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::cout << "Hierarchy Coverage Summary:\n";
    க்கு (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.end(); ++it) {
        மாறிலி std::vector<string> parts = splitHier(it->first);
        எனில் (levels >= 0 && static_cast<முழு>(parts.size()) > levels + 1) தொடர்;
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, பொய்);
    }
    std::cout << "Design Unit Coverage Summary:\n";
    க்கு (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.end();
         ++it) {
        printIndent(s_summaryIndent);
        std::cout << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, பொய்);
    }
    எனில் (!cgTallies.empty()) printCovergroupTallies(cgTallies, levels);
}
