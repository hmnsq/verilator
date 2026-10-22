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

#परिभाषय VL_MT_DISABLED_संहिता_UNIT 1

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
    uint64_t m_लब्धम् = 0;  // Covered points
    uint64_t m_योग = 0;  // Points
    तार्किक m_scored = असत्य;  // Coverage is m_score, not the ratio of the points
    द्विगुण m_score = 0.0;  // Percent coverage, if m_scored

सार्वजनिक:
    // ACCESSORS
    uint64_t लब्धम्() ध्रुव { प्रत्यागम m_लब्धम्; }
    uint64_t योग() ध्रुव { प्रत्यागम m_योग; }
    तार्किक scored() ध्रुव { प्रत्यागम m_scored; }
    द्विगुण score() ध्रुव { प्रत्यागम m_score; }
    शून्य score(द्विगुण मानम्) {
        m_scored = सत्य;
        m_score = मानम्;
    }
    // METHODS
    शून्य सङ्कलय_Point(तार्किक covered) {
        यदि (covered) ++m_लब्धम्;
        ++m_योग;
    }
    शून्य सङ्कलय_Points(ध्रुव Tally& other) {
        m_लब्धम् += other.m_लब्धम्;
        m_योग += other.m_योग;
    }
};

// Map coverage type to its tally.
उपयुज्य TypeTally = std::map<std::string, Tally>;

स्थिर constexpr ध्रुव वर्ण* ध्रुव s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
स्थिर constexpr ध्रुव वर्ण* ध्रुव s_covergroupType = "covergroup";
स्थिर constexpr आकार_t s_summaryIndent = 2;
स्थिर constexpr आकार_t s_reportRowIndent = 4;

string displayType(ध्रुव VlcPoint& point) {
    ध्रुव string type = point.type();
    प्रत्यागम type.रिक्तानि() ? "point" : type;
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
std::सदिशम्<string> विभज_Hier(ध्रुव string& hier) {
    std::सदिशम्<string> parts;
    string::आकार_type start = 0;
    पूर्णाङ्क गभीरता = 0;  // Of the parentheses of the values of parameters
    string::आकार_type pos = 0;
    यावत् (pos < hier.आकार()) {
        ध्रुव वर्ण c = hier[pos];
        यदि (c == '\\') {
            pos = hier.find(' ', pos);  // npos if not ended
            अनुवर्त;
        } अन्यथा यदि (!गभीरता && hier.compare(pos, 2, "#(") == 0) {
            गभीरता = 1;
            ++pos;
        } अन्यथा यदि (!गभीरता && c == '.') {
            parts.प्रक्षिप_पृष्ठभागः(hier.substr(start, pos - start));
            start = pos + 1;
        } अन्यथा यदि (गभीरता && c == '"') {
            pos = VString::quoted_अन्तः(hier, pos);  // npos if not terminated
            अनुवर्त;
        } अन्यथा यदि (गभीरता && c == '(') {
            ++गभीरता;
        } अन्यथा यदि (गभीरता && c == ')') {
            --गभीरता;
        }
        ++pos;
    }
    यदि (गभीरता || pos != hier.आकार()) {  // Unbalanced
        parts.clear();
        start = 0;
        प्रति (string::आकार_type dot; (dot = hier.find('.', start)) != string::npos;
             start = dot + 1) {
            parts.प्रक्षिप_पृष्ठभागः(hier.substr(start, dot - start));
        }
    }
    parts.प्रक्षिप_पृष्ठभागः(hier.substr(start));
    प्रत्यागम parts;
}

string duName(ध्रुव VlcPoint& point) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ध्रुव string page = point.page();
    प्रत्यागम page.substr(page.find('/') + 1);
}

शून्य tallyPoint(TypeTally& tally, ध्रुव string& type, uint64_t गणनम्) {
    tally[type].सङ्कलय_Point(गणनम् > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
द्विगुण pct(uint64_t लब्धम्, uint64_t योग) {
    प्रत्यागम योग ? (100.0 * static_cast<द्विगुण>(लब्धम्) / static_cast<द्विगुण>(योग)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_मानम्_String(द्विगुण मानम्, तार्किक complete) {
    यदि (!complete && मानम् > 99.9) मानम् = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << मानम् << "%";
    प्रत्यागम os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t लब्धम्, uint64_t योग) {
    प्रत्यागम pct_मानम्_String(pct(लब्धम्, योग), लब्धम् >= योग);
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
    uint64_t m_गणनम् = 0;  // Hits, of all the records
    uint64_t m_at_न्यूनतमम् = 0;  // option.at_least, the largest of the records

सार्वजनिक:
    // ACCESSORS
    ध्रुव string& name() ध्रुव { प्रत्यागम m_name; }
    तार्किक covered() ध्रुव { प्रत्यागम m_गणनम् >= m_at_न्यूनतमम्; }
    // METHODS
    शून्य सङ्कलय_लेखः(ध्रुव string& name, uint64_t गणनम्, uint64_t at_न्यूनतमम्) {
        m_name = name;
        m_गणनम् += गणनम्;
        m_at_न्यूनतमम् = std::max(m_at_न्यूनतमम्, at_न्यूनतमम्);
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
    शून्य सङ्कलय_लेखः(uint64_t weight) { m_weight = std::max(m_weight, weight); }
    CgBin& findNewBin(ध्रुव string& तटस्थः) { प्रत्यागम m_bins[तटस्थः]; }
};

// A covergroup
वर्ग Cg_गणः final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_weight = 0;  // type_option.weight, the largest of the records
    std::map<string, CgItem> m_वस्तूनि;  // By hierarchy

सार्वजनिक:
    // ACCESSORS
    ध्रुव string& hier() ध्रुव { प्रत्यागम m_hier; }
    uint64_t weight() ध्रुव { प्रत्यागम m_weight; }
    ध्रुव std::map<string, CgItem>& वस्तूनि() ध्रुव { प्रत्यागम m_वस्तूनि; }
    // METHODS
    शून्य सङ्कलय_लेखः(ध्रुव string& hier, uint64_t weight) {
        m_hier = hier;
        m_weight = std::max(m_weight, weight);
    }
    CgItem& findNewItem(ध्रुव string& hier) { प्रत्यागम m_वस्तूनि[hier]; }
};
उपयुज्य CgGroups = std::map<string, Cg_गणः>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t कुञ्चिका_सङ्ख्या(ध्रुव string& मानम्) {
    प्रत्यागम मानम्.रिक्तानि() ? 1 : std::strtoull(मानम्.c_str(), शून्यसूचकः, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
तार्किक is_Score_क्षेत्रम्(ध्रुव string& क्षेत्रम्) {
    प्रति (ध्रुव वर्ण* ध्रुव keyp : {VL_CIK_THRESH, VL_CIK_WEIGHT, VL_CIK_गणः_WEIGHT}) {
        ध्रुव string उपसर्गः = string{"\001"} + keyp + "\002";
        यदि (क्षेत्रम्.compare(0, उपसर्गः.आकार(), उपसर्गः) == 0) प्रत्यागम सत्य;
    }
    प्रत्यागम असत्य;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_तटस्थः(ध्रुव string& लेखः_Name) {
    string तटस्थः;
    string::आकार_type start = 0;
    यावत् (start < लेखः_Name.आकार()) {
        string::आकार_type अन्तः_2 = लेखः_Name.find('\001', start + 1);
        यदि (अन्तः_2 == string::npos) अन्तः_2 = लेखः_Name.आकार();
        ध्रुव string क्षेत्रम् = लेखः_Name.substr(start, अन्तः_2 - start);
        यदि (!is_Score_क्षेत्रम्(क्षेत्रम्)) तटस्थः += क्षेत्रम्;
        start = अन्तः_2;
    }
    प्रत्यागम तटस्थः;
}

तार्किक isCovergroup(ध्रुव VlcPoint& point) { प्रत्यागम point.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    प्रति (ध्रुव VlcPoints::ByName::मानम्_type& i : points) {
        ध्रुव VlcPoint& pt = points.point_सङ्ख्या(i.second);
        यदि (!isCovergroup(pt)) अनुवर्त;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ध्रुव string hier = pt.hier();
        string bin = pt.bin();
        यदि (bin.रिक्तानि() || bin.आकार() >= hier.आकार()) bin = hier.substr(hier.rfind('.') + 1);
        ध्रुव string itemHier
            = hier.substr(0, hier.आकार() - std::min(hier.आकार(), bin.आकार() + 1));
        Cg_गणः& गणः = groups[pt.page()];
        गणः.सङ्कलय_लेखः(itemHier.substr(0, itemHier.rfind('.')), कुञ्चिका_सङ्ख्या(pt.गणः_Weight()));
        CgItem& item = गणः.findNewItem(itemHier);
        item.सङ्कलय_लेखः(कुञ्चिका_सङ्ख्या(pt.weight()));
        यदि (!pt.binType().रिक्तानि()) अनुवर्त;  // Not coverable
        item.findNewBin(bin_तटस्थः(pt.name())).सङ्कलय_लेखः(bin, pt.गणनम्(), कुञ्चिका_सङ्ख्या(pt.thresh()));
    }
    प्रत्यागम groups;
}

Tally itemTally(ध्रुव CgItem& item) {
    Tally tally;
    प्रति (ध्रुव std::pair<ध्रुव string, CgBin>& bin : item.bins()) {
        tally.सङ्कलय_Point(bin.second.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.योग() ? pct(tally.लब्धम्(), tally.योग()) : item.weight() ? 0.0 : 100.0);
    प्रत्यागम tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally गणः_Tally(ध्रुव Cg_गणः& गणः, तार्किक* contributesp = शून्यसूचकः) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    प्रति (ध्रुव std::pair<ध्रुव string, CgItem>& it : गणः.वस्तूनि()) {
        ध्रुव Tally item = itemTally(it.second);
        tally.सङ्कलय_Points(item);
        यदि (!item.योग()) अनुवर्त;  // Excluded from both sums
        weighted += static_cast<द्विगुण>(it.second.weight()) * item.score();
        weights += static_cast<द्विगुण>(it.second.weight());
    }
    यदि (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : गणः.weight() ? 0.0 : 100.0);
    प्रत्यागम tally;
}

Tally groupsTally(ध्रुव std::सदिशम्<ध्रुव Cg_गणः*>& groups) {
    Tally tally;
    द्विगुण weighted = 0.0;
    द्विगुण weights = 0.0;
    तार्किक anyWeight = असत्य;
    प्रति (ध्रुव Cg_गणः* ध्रुव groupp : groups) {
        तार्किक contributes = असत्य;
        ध्रुव Tally गणः = गणः_Tally(*groupp, &contributes);
        tally.सङ्कलय_Points(गणः);
        यदि (groupp->weight()) anyWeight = सत्य;
        यदि (!contributes) अनुवर्त;
        weighted += static_cast<द्विगुण>(groupp->weight()) * गणः.score();
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
    std::map<string, std::सदिशम्<ध्रुव Cg_गणः*>> ग्रन्थिः_Groups;
    प्रति (ध्रुव CgGroups::मानम्_type& it : groups) {
        ध्रुव Cg_गणः& गणः = it.second;
        string मार्गः;
        प्रति (ध्रुव string& part : विभज_Hier(गणः.hier())) {
            मार्गः = मार्गः.रिक्तानि() ? part : मार्गः + "." + part;
            ग्रन्थिः_Groups[मार्गः].प्रक्षिप_पृष्ठभागः(&गणः);
        }
        प्रति (ध्रुव std::pair<ध्रुव string, CgItem>& item : गणः.वस्तूनि()) {
            tallies[item.first] = itemTally(item.second);
            प्रति (ध्रुव std::pair<ध्रुव string, CgBin>& bin : item.second.bins()) {
                // Of the bins of the name
                tallies[item.first + "." + bin.second.name()].सङ्कलय_Point(bin.second.covered());
            }
        }
    }
    प्रति (ध्रुव std::pair<ध्रुव string, std::सदिशम्<ध्रुव Cg_गणः*>>& it : ग्रन्थिः_Groups) {
        tallies[it.first]
            = it.second.आकार() == 1 ? गणः_Tally(*it.second.front()) : groupsTally(it.second);
    }
    प्रत्यागम tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
शून्य printIndent(आकार_t indent) {
    प्रति (आकार_t i = 0; i < indent; ++i) std::निर्गमः << ' ';
}

शून्य printTallyRow(ध्रुव string& type, ध्रुव Tally& tally, आकार_t indent, आकार_t type_विस्तारः,
                   आकार_t गणनम्_विस्तारः) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ध्रुव string percent = tally.scored() ? pct_मानम्_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.लब्धम्(), tally.योग());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::निर्गमः << std::वामतः << std::setw(type_विस्तारः) << type << " : " << std::दक्षिणतः << std::fixed
              << std::setw(6) << percent << " (" << std::setw(गणनम्_विस्तारः) << tally.लब्धम्() << "/"
              << std::setw(गणनम्_विस्तारः) << tally.योग() << ")\n";
}

आकार_t गणनम्_विस्तारः(ध्रुव TypeTally& tally) {
    आकार_t विस्तारः = cvt_गन्तव्यम्_Str(0).आकार();
    प्रति (TypeTally::const_iterator it = tally.begin(); it != tally.अन्तः_2(); ++it) {
        विस्तारः = std::max(विस्तारः, cvt_गन्तव्यम्_Str(it->second.लब्धम्()).आकार());
        विस्तारः = std::max(विस्तारः, cvt_गन्तव्यम्_Str(it->second.योग()).आकार());
    }
    प्रत्यागम विस्तारः;
}

आकार_t type_विस्तारः(ध्रुव TypeTally& tally) {
    आकार_t type_विस्तारः = 0;
    प्रति (ध्रुव वर्ण* ध्रुव typep : s_orderedTypes) {
        ध्रुव string type = typep;
        type_विस्तारः = std::max(type_विस्तारः, type.आकार());
    }
    प्रति (TypeTally::const_iterator it = tally.begin(); it != tally.अन्तः_2(); ++it) {
        type_विस्तारः = std::max(type_विस्तारः, it->first.आकार());
    }
    प्रत्यागम type_विस्तारः;
}

शून्य printTypeTally(ध्रुव TypeTally& tally, आकार_t indent, तार्किक includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ध्रुव आकार_t typ_विस्तारः = type_विस्तारः(tally);
    ध्रुव आकार_t cnt_विस्तारः = गणनम्_विस्तारः(tally);
    प्रति (ध्रुव वर्ण* ध्रुव typep : s_orderedTypes) {
        ध्रुव string type = typep;
        ध्रुव TypeTally::const_iterator it = tally.find(type);
        यदि (it != tally.अन्तः_2()) {
            printTallyRow(type, it->second, indent, typ_विस्तारः, cnt_विस्तारः);
        } अन्यथा यदि (includeMissingOrdered) {
            printTallyRow(type, Tally{}, indent, typ_विस्तारः, cnt_विस्तारः);
        }
    }
    प्रति (TypeTally::const_iterator it = tally.begin(); it != tally.अन्तः_2(); ++it) {
        यदि (!isOrderedType(it->first)) {
            printTallyRow(it->first, it->second, indent, typ_विस्तारः, cnt_विस्तारः);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
शून्य printCovergroupTallies(ध्रुव std::map<string, Tally>& tallies, पूर्णाङ्क स्तराः) {
    std::map<string, Tally> shown;
    आकार_t name_विस्तारः = 0;
    प्रति (ध्रुव std::pair<ध्रुव string, Tally>& it : tallies) {
        यदि (स्तराः >= 0 && static_cast<पूर्णाङ्क>(विभज_Hier(it.first).आकार()) > स्तराः + 1) अनुवर्त;
        shown.निवेशय(it);
        name_विस्तारः = std::max(name_विस्तारः, it.first.आकार());
    }
    ध्रुव आकार_t cnt_विस्तारः = गणनम्_विस्तारः(shown);
    std::निर्गमः << "Covergroup Coverage Summary:\n";
    प्रति (ध्रुव std::pair<ध्रुव string, Tally>& it : shown) {
        printTallyRow(it.first, it.second, s_summaryIndent, name_विस्तारः, cnt_विस्तारः);
    }
}

}  // namespace

शून्य Vlc_शिखरम्::readCoverage(ध्रुव string& filename, तार्किक nonfatal) {
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
        ध्रुव string पङ्क्तिः = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        यदि (पङ्क्तिः[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ध्रुव string::आकार_type secspace = पङ्क्तिः.rfind("' ");
            यदि (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                अनुवर्त;
            }
            ध्रुव string point = पङ्क्तिः.substr(3, secspace - 3);
            यदि (!opt.is_Type_मेलनम्(point.c_str())) अनुवर्त;

            ध्रुव uint64_t लब्धानि = std::atoll(पङ्क्तिः.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ध्रुव uint64_t pointnum = points().find_सङ्कलय_Point(point, लब्धानि);
            यदि (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ध्रुव uint64_t needed = std::max(
                    VlcBuckets::sufficient(), कुञ्चिका_सङ्ख्या(points().point_सङ्ख्या(pointnum).thresh()));
                यदि (लब्धानि >= needed) {
                    points().point_सङ्ख्या(pointnum).testsCoveringInc();
                    testp->buckets().सङ्कलय_दत्त(pointnum, लब्धानि);
                }
            }
        }
    }
}

शून्य Vlc_शिखरम्::writeCoverage(ध्रुव string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    यदि (!os) {
        v3fatal("Can't write file: " << filename);
        प्रत्यागम;
    }

    os << "# SystemC::Coverage-3\n";
    प्रति (ध्रुव स्वचालित& i : m_points) {
        ध्रुव VlcPoint& point = m_points.point_सङ्ख्या(i.second);
        os << "C '" << point.name() << "' " << point.गणनम्() << '\n';
    }
}

शून्य Vlc_शिखरम्::writeInfo(ध्रुव string& filename) {
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
    प्रति (स्वचालित& si : m_sources) {
        VlcSource& source = si.second;
        os << "SF:" << source.name() << '\n';
        VlcSource::LinenoMap& पङ्क्तयः = source.पङ्क्तयः();
        पूर्णाङ्क branches_प्राप्तम् = 0;
        पूर्णाङ्क branches_लब्धम् = 0;
        प्रति (स्वचालित& li : पङ्क्तयः) {
            Vlc_Source_गणनम्& sc = li.second;
            uint64_t da_गणनम् = 0;
            std::सदिशम्<ध्रुव VlcPoint*> infoPoints;
            प्रति (ध्रुव स्वचालित& point : sc.points()) {
                da_गणनम् = std::max(da_गणनम्, point->गणनम्());
                infoPoints.प्रक्षिप_पृष्ठभागः(point);
            }
            os << "DA:" << sc.lineno() << "," << da_गणनम् << "\n";
            यदि (infoPoints.आकार() <= 1) अनुवर्त;
            branches_प्राप्तम् += static_cast<पूर्णाङ्क>(infoPoints.आकार());
            पूर्णाङ्क point_अङ्कः_2 = 0;
            प्रति (ध्रुव VlcPoint* point : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                यदि (point->isFsmArc()) {
                    os << "2,";
                    os << point->fsmFromState() << "->" << point->fsm_गन्तव्यम्_State();
                } अन्यथा यदि (point->comment().रिक्तानि()) {
                    os << "0,";
                    os << point_अङ्कः_2;
                } अन्यथा {
                    os << (point->isFsmState() ? '1' : '0') << ',';
                    std::string comment(point->comment());
                    std::replace(comment.begin(), comment.अन्तः_2(), ',', '_');
                    os << comment;
                }
                os << "," << point->गणनम्() << "\n";

                branches_लब्धम् += opt.गणनम्_Ok(point->गणनम्());
                ++point_अङ्कः_2;
            }
        }
        os << "BRF:" << branches_प्राप्तम् << '\n';
        os << "BRH:" << branches_लब्धम् << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

संरचना CmpComputrons final {
    तार्किक संक्रियकः()(ध्रुव VlcTest* lhsp, ध्रुव VlcTest* rhsp) ध्रुव {
        यदि (lhsp->computrons() != rhsp->computrons()) {
            प्रत्यागम lhsp->computrons() < rhsp->computrons();
        }
        प्रत्यागम lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

शून्य Vlc_शिखरम्::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::सदिशम्<VlcTest*> bytime;
    प्रति (ध्रुव स्वचालित& testp : m_tests) {
        यदि (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.प्रक्षिप_पृष्ठभागः(testp);
        }
    }
    क्रमय(bytime.begin(), bytime.अन्तः_2(), CmpComputrons());  // Sort the vector

    VlcBuckets remaining;
    प्रति (ध्रुव स्वचालित& i : m_points) {
        ध्रुव VlcPoint* ध्रुव pointp = &points().point_सङ्ख्या(i.second);
        // If any tests hit this point, then we'll need to cover it.
        यदि (pointp->testsCovering()) remaining.सङ्कलय_दत्त(pointp->point_अङ्कः(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    यावत् (सत्य) {
        यदि (debug() >= 9) {
            UINFO_उपसर्गः("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            remaining.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* bestTestp = शून्यसूचकः;
        uint64_t bestRemain = 0;
        प्रति (ध्रुव स्वचालित& testp : bytime) {
            यदि (!testp->rank()) {
                ध्रुव uint64_t remain = testp->buckets().दत्त_निष्कासय_गणनम्(remaining);
                यदि (remain > bestRemain) {
                    bestTestp = testp;
                    bestRemain = remain;
                }
            }
        }
        यदि (VlcTest* ध्रुव testp = bestTestp) {
            testp->rank(nextrank++);
            testp->rankPoints(bestRemain);
            remaining.or_दत्त(bestTestp->buckets());
        } अन्यथा {
            भङ्ग;  // No test covering more stuff found
        }
    }
}

शून्य Vlc_शिखरम्::annotateCalc() {
    // Calculate per-line information into filedata structure
    प्रति (ध्रुव स्वचालित& i : m_points) {
        ध्रुव VlcPoint& point = m_points.point_सङ्ख्या(i.second);
        ध्रुव string filename = point.filename();
        ध्रुव पूर्णाङ्क lineno = point.lineno();
        यदि (!filename.रिक्तानि() && lineno != 0) {
            VlcSource& source = sources().findNewSource(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << point.column() << " "
                                       << point.गणनम्() << " " << point.linescov());
            // Base coverage
            source.निवेशय_Point(lineno, &point);
            // Additional lines covered by this statement
            तार्किक range = असत्य;
            पूर्णाङ्क start = 0;
            पूर्णाङ्क अन्तः_2 = 0;
            ध्रुव string linescov = point.linescov();
            प्रति (ध्रुव वर्ण* covp = linescov.c_str(); सत्य; ++covp) {
                यदि (!*covp || *covp == ',') {  // Ending
                    प्रति (पूर्णाङ्क lni = start; start && lni <= अन्तः_2; ++lni) {
                        source.निवेशय_Point(lni, &point);
                    }
                    यदि (!*covp) भङ्ग;
                    start = 0;  // Prep for next
                    अन्तः_2 = 0;
                    range = असत्य;
                } अन्यथा यदि (*covp == '-') {
                    range = सत्य;
                } अन्यथा यदि (std::अङ्कः_वा(*covp)) {
                    ध्रुव वर्ण* ध्रुव digitsp = covp;
                    यावत् (std::अङ्कः_वा(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    यदि (!range) start = std::atoi(digitsp);
                    अन्तः_2 = std::atoi(digitsp);
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
    प्रति (स्वचालित& si : m_sources) {
        VlcSource& source = si.second;
        // UINFO(1, "Source " << source.name());
        यदि (opt.annotateAll()) source.needed(सत्य);
        ध्रुव VlcSource::LinenoMap& पङ्क्तयः = source.पङ्क्तयः();
        प्रति (स्वचालित& li : पङ्क्तयः) {
            ध्रुव Vlc_Source_गणनम्& sc = li.second;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            यदि (opt.गणनम्_Ok(sc.min_गणनम्())) {
                ++totOk;
            } अन्यथा {
                source.needed(सत्य);
            }
        }
    }
    std::निर्गमः << "Annotation Summary:\n";
    std::निर्गमः << "  lines with all attached points covered : ";
    std::निर्गमः << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    यदि (totOk != totCases) निर्गमः << "See lines with '%00' in " << opt.annotate_बहिर्गमनम्() << '\n';
}

शून्य Vlc_शिखरम्::annotateOutputFiles(ध्रुव string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    प्रति (स्वचालित& si : m_sources) {
        VlcSource& source = si.second;
        यदि (!source.needed()) अनुवर्त;
        ध्रुव string filename = source.name();
        ध्रुव string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

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
            ध्रुव std::string पङ्क्तिः = V3Os::getline(is);

            VlcSource::LinenoMap& पङ्क्तयः = source.पङ्क्तयः();
            ध्रुव स्वचालित lit = पङ्क्तयः.find(lineno);
            यदि (lit == पङ्क्तयः.अन्तः_2()) {
                os << "        " << पङ्क्तिः << '\n';
            } अन्यथा {
                Vlc_Source_गणनम्& sc = lit->second;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ध्रुव तार्किक minOk = opt.गणनम्_Ok(sc.min_गणनम्());
                ध्रुव तार्किक maxOk = opt.गणनम्_Ok(sc.max_गणनम्());
                यदि (minOk) {
                    os << " ";
                } अन्यथा यदि (maxOk) {
                    os << "~";
                } अन्यथा {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_गणनम्() << " " << पङ्क्तिः << '\n';

                यदि (opt.annotatePoints()) {
                    प्रति (ध्रुव स्वचालित& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                तार्किक printed_Fsm_शीर्षकम् = असत्य;
                प्रति (ध्रुव स्वचालित& pit : sc.points()) {
                    यदि (!pit->isFsmState() && !pit->isFsmArc()) अनुवर्त;
                    यदि (!printed_Fsm_शीर्षकम्) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_शीर्षकम् = सत्य;
                    }
                    os << (opt.गणनम्_Ok(pit->गणनम्()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->गणनम्() << "        ";
                    यदि (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        यदि (pit->गणनम्() == 0) os << " *** UNCOVERED ***";
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

शून्य Vlc_शिखरम्::annotate(ध्रुव string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

शून्य Vlc_शिखरम्::printTypeSummary() {
    TypeTally tally;
    प्रति (VlcPoints::ByName::मानम्_type& i : m_points) {
        ध्रुव VlcPoint& pt = m_points.point_सङ्ख्या(i.second);
        यदि (isCovergroup(pt)) अनुवर्त;  // Tallied below, by covergroup
        tallyPoint(tally, displayType(pt), pt.गणनम्());
    }
    ध्रुव CgGroups groups = covergroups(m_points);
    यदि (!groups.रिक्तानि()) {
        std::सदिशम्<ध्रुव Cg_गणः*> groupps;
        प्रति (ध्रुव CgGroups::मानम्_type& it : groups) groupps.प्रक्षिप_पृष्ठभागः(&it.second);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    यदि (tally.रिक्तानि()) प्रत्यागम;
    std::निर्गमः << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, सत्य);
}

शून्य Vlc_शिखरम्::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    तार्किक hasHier = असत्य;
    तार्किक hasCollapsedHier = असत्य;
    प्रति (VlcPoints::ByName::मानम्_type& i : m_points) {
        ध्रुव VlcPoint& pt = m_points.point_सङ्ख्या(i.second);
        यदि (isCovergroup(pt)) अनुवर्त;  // Reported below, by covergroup
        ध्रुव string hier = reportHier(pt);
        यदि (hier.रिक्तानि()) अनुवर्त;
        hasHier = सत्य;
        यदि (isCollapsedHier(hier)) hasCollapsedHier = सत्य;
        ध्रुव string type = displayType(pt);
        ध्रुव std::सदिशम्<string> parts = विभज_Hier(hier);
        string मार्गः;
        प्रति (std::सदिशम्<string>::const_iterator it = parts.begin(); it != parts.अन्तः_2(); ++it) {
            मार्गः = मार्गः.रिक्तानि() ? *it : मार्गः + "." + *it;
            tallyPoint(hierTallies[मार्गः], type, pt.गणनम्());
        }
        tallyPoint(duTallies[duName(pt)], type, pt.गणनम्());
    }
    ध्रुव std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    यदि (!hasHier && !cgTallies.रिक्तानि()) {
        printCovergroupTallies(cgTallies, opt.report_स्तराः());
        प्रत्यागम;
    }
    यदि (!hasHier) {
        std::निर्गमः << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        प्रत्यागम;
    }

    ध्रुव पूर्णाङ्क स्तराः = opt.report_स्तराः();
    यदि (hasCollapsedHier) {
        std::निर्गमः << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::निर्गमः << "Hierarchy Coverage Summary:\n";
    प्रति (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.अन्तः_2(); ++it) {
        ध्रुव std::सदिशम्<string> parts = विभज_Hier(it->first);
        यदि (स्तराः >= 0 && static_cast<पूर्णाङ्क>(parts.आकार()) > स्तराः + 1) अनुवर्त;
        printIndent(s_summaryIndent);
        std::निर्गमः << it->first << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->second, s_reportRowIndent, असत्य);
    }
    std::निर्गमः << "Design Unit Coverage Summary:\n";
    प्रति (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.अन्तः_2();
         ++it) {
        printIndent(s_summaryIndent);
        std::निर्गमः << it->first << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->second, s_reportRowIndent, असत्य);
    }
    यदि (!cgTallies.रिक्तानि()) printCovergroupTallies(cgTallies, स्तराः);
}
