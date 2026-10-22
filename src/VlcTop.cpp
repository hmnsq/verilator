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

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "VlcTop.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Os.h"

#అన్తర్భావ "VlcOptions.h"

#అన్తర్భావ <algorithm>
#అన్తర్భావ <fstream>
#అన్తర్భావ <iomanip>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <sstream>
#అన్తర్భావ <string>
#అన్తర్భావ <vector>

//######################################################################

నామాకాశ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
తరగతి Tally final {
    // MEMBERS
    uint64_t m_తగిలింది = 0;  // Covered points
    uint64_t m_మొత్తం = 0;  // Points
    తార్కిక m_scored = అబద్ధం;  // Coverage is m_score, not the ratio of the points
    ద్విగుణ m_score = 0.0;  // Percent coverage, if m_scored

బహిరంగ:
    // ACCESSORS
    uint64_t తగిలింది() స్థిరరాశి { తిరిగి m_తగిలింది; }
    uint64_t మొత్తం() స్థిరరాశి { తిరిగి m_మొత్తం; }
    తార్కిక scored() స్థిరరాశి { తిరిగి m_scored; }
    ద్విగుణ score() స్థిరరాశి { తిరిగి m_score; }
    శూన్య score(ద్విగుణ విలువ) {
        m_scored = నిజం;
        m_score = విలువ;
    }
    // METHODS
    శూన్య కూడు_బిందువు(తార్కిక covered) {
        ఒకవేళ (covered) ++m_తగిలింది;
        ++m_మొత్తం;
    }
    శూన్య కూడు_Points(స్థిరరాశి Tally& other) {
        m_తగిలింది += other.m_తగిలింది;
        m_మొత్తం += other.m_మొత్తం;
    }
};

// Map coverage type to its tally.
ఉపయోగించి TypeTally = std::map<std::string, Tally>;

స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి s_covergroupType = "covergroup";
స్థిర constexpr కొలత_t s_summaryIndent = 2;
స్థిర constexpr కొలత_t s_report_అడ్డువరుస_Indent = 4;

string displayType(స్థిరరాశి Vlc_బిందువు& బిందువు) {
    స్థిరరాశి string type = బిందువు.type();
    తిరిగి type.ఖాళీలు() ? "point" : type;
}

తార్కిక isCollapsedHier(స్థిరరాశి string& hier) {
    తిరిగి hier.కనుగొను('*') != string::npos || hier.కనుగొను('?') != string::npos;
}

తార్కిక isOrderedType(స్థిరరాశి string& type) {
    కొరకు (స్థిరరాశి అక్షరం* స్థిరరాశి typep : s_orderedTypes) {
        ఒకవేళ (type == typep) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}

string reportHier(స్థిరరాశి Vlc_బిందువు& బిందువు) {
    // FSM records currently have useful instance scope in Fv.
    ఒకవేళ (బిందువు.isFsmState() || బిందువు.isFsmArc()) {
        స్థిరరాశి string fsmVar = బిందువు.fsmVarName();
        తిరిగి fsmVar.substr(0, fsmVar.rfind('.'));
    }
    తిరిగి బిందువు.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::సదిశ<string> విభజించు_Hier(స్థిరరాశి string& hier) {
    std::సదిశ<string> parts;
    string::కొలత_type ప్రారంభం = 0;
    పూర్ణాంకం లోతు = 0;  // Of the parentheses of the values of parameters
    string::కొలత_type pos = 0;
    అయితే (pos < hier.కొలత()) {
        స్థిరరాశి అక్షరం c = hier[pos];
        ఒకవేళ (c == '\\') {
            pos = hier.కనుగొను(' ', pos);  // npos if not ended
            కొనసాగు;
        } లేకపోతే ఒకవేళ (!లోతు && hier.compare(pos, 2, "#(") == 0) {
            లోతు = 1;
            ++pos;
        } లేకపోతే ఒకవేళ (!లోతు && c == '.') {
            parts.నెట్టు_వెనుక(hier.substr(ప్రారంభం, pos - ప్రారంభం));
            ప్రారంభం = pos + 1;
        } లేకపోతే ఒకవేళ (లోతు && c == '"') {
            pos = VString::quoted_చివర(hier, pos);  // npos if not terminated
            కొనసాగు;
        } లేకపోతే ఒకవేళ (లోతు && c == '(') {
            ++లోతు;
        } లేకపోతే ఒకవేళ (లోతు && c == ')') {
            --లోతు;
        }
        ++pos;
    }
    ఒకవేళ (లోతు || pos != hier.కొలత()) {  // Unbalanced
        parts.clear();
        ప్రారంభం = 0;
        కొరకు (string::కొలత_type dot; (dot = hier.కనుగొను('.', ప్రారంభం)) != string::npos;
             ప్రారంభం = dot + 1) {
            parts.నెట్టు_వెనుక(hier.substr(ప్రారంభం, dot - ప్రారంభం));
        }
    }
    parts.నెట్టు_వెనుక(hier.substr(ప్రారంభం));
    తిరిగి parts;
}

string duName(స్థిరరాశి Vlc_బిందువు& బిందువు) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    స్థిరరాశి string పుట = బిందువు.పుట();
    తిరిగి పుట.substr(పుట.కనుగొను('/') + 1);
}

శూన్య tally_బిందువు(TypeTally& tally, స్థిరరాశి string& type, uint64_t లెక్క) {
    tally[type].కూడు_బిందువు(లెక్క > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
ద్విగుణ pct(uint64_t తగిలింది, uint64_t మొత్తం) {
    తిరిగి మొత్తం ? (100.0 * static_cast<ద్విగుణ>(తగిలింది) / static_cast<ద్విగుణ>(మొత్తం)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_విలువ_String(ద్విగుణ విలువ, తార్కిక complete) {
    ఒకవేళ (!complete && విలువ > 99.9) విలువ = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << విలువ << "%";
    తిరిగి os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t తగిలింది, uint64_t మొత్తం) {
    తిరిగి pct_విలువ_String(pct(తగిలింది, మొత్తం), తగిలింది >= మొత్తం);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
తరగతి CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_లెక్క = 0;  // Hits, of all the records
    uint64_t m_at_కనిష్ఠ = 0;  // option.at_least, the largest of the records

బహిరంగ:
    // ACCESSORS
    స్థిరరాశి string& name() స్థిరరాశి { తిరిగి m_name; }
    తార్కిక covered() స్థిరరాశి { తిరిగి m_లెక్క >= m_at_కనిష్ఠ; }
    // METHODS
    శూన్య కూడు_రికార్డు(స్థిరరాశి string& name, uint64_t లెక్క, uint64_t at_కనిష్ఠ) {
        m_name = name;
        m_లెక్క += లెక్క;
        m_at_కనిష్ఠ = std::max(m_at_కనిష్ఠ, at_కనిష్ఠ);
    }
};

// A coverpoint or cross
తరగతి Cg_అంశం final {
    // MEMBERS
    uint64_t m_బరువు = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

బహిరంగ:
    // ACCESSORS
    uint64_t బరువు_2() స్థిరరాశి { తిరిగి m_బరువు; }
    స్థిరరాశి std::map<string, CgBin>& bins() స్థిరరాశి { తిరిగి m_bins; }
    // METHODS
    శూన్య కూడు_రికార్డు(uint64_t బరువు_2) { m_బరువు = std::max(m_బరువు, బరువు_2); }
    CgBin& కనుగొను_New_Bin(స్థిరరాశి string& తటస్థం) { తిరిగి m_bins[తటస్థం]; }
};

// A covergroup
తరగతి Cg_సమూహం final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_బరువు = 0;  // type_option.weight, the largest of the records
    std::map<string, Cg_అంశం> m_వస్తువులు;  // By hierarchy

బహిరంగ:
    // ACCESSORS
    స్థిరరాశి string& hier() స్థిరరాశి { తిరిగి m_hier; }
    uint64_t బరువు_2() స్థిరరాశి { తిరిగి m_బరువు; }
    స్థిరరాశి std::map<string, Cg_అంశం>& వస్తువులు() స్థిరరాశి { తిరిగి m_వస్తువులు; }
    // METHODS
    శూన్య కూడు_రికార్డు(స్థిరరాశి string& hier, uint64_t బరువు_2) {
        m_hier = hier;
        m_బరువు = std::max(m_బరువు, బరువు_2);
    }
    Cg_అంశం& కనుగొను_New_అంశం(స్థిరరాశి string& hier) { తిరిగి m_వస్తువులు[hier]; }
};
ఉపయోగించి CgGroups = std::map<string, Cg_సమూహం>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t కీలకం_సంఖ్య(స్థిరరాశి string& విలువ) {
    తిరిగి విలువ.ఖాళీలు() ? 1 : std::strtoull(విలువ.c_str(), శూన్యసూచిక, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
తార్కిక is_Score_క్షేత్రం(స్థిరరాశి string& క్షేత్రం) {
    కొరకు (స్థిరరాశి అక్షరం* స్థిరరాశి keyp : {VL_CIK_THRESH, VL_CIK_బరువు, VL_CIK_సమూహం_బరువు}) {
        స్థిరరాశి string ఉపసర్గ = string{"\001"} + keyp + "\002";
        ఒకవేళ (క్షేత్రం.compare(0, ఉపసర్గ.కొలత(), ఉపసర్గ) == 0) తిరిగి నిజం;
    }
    తిరిగి అబద్ధం;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_తటస్థం(స్థిరరాశి string& రికార్డు_Name) {
    string తటస్థం;
    string::కొలత_type ప్రారంభం = 0;
    అయితే (ప్రారంభం < రికార్డు_Name.కొలత()) {
        string::కొలత_type చివర = రికార్డు_Name.కనుగొను('\001', ప్రారంభం + 1);
        ఒకవేళ (చివర == string::npos) చివర = రికార్డు_Name.కొలత();
        స్థిరరాశి string క్షేత్రం = రికార్డు_Name.substr(ప్రారంభం, చివర - ప్రారంభం);
        ఒకవేళ (!is_Score_క్షేత్రం(క్షేత్రం)) తటస్థం += క్షేత్రం;
        ప్రారంభం = చివర;
    }
    తిరిగి తటస్థం;
}

తార్కిక isCovergroup(స్థిరరాశి Vlc_బిందువు& బిందువు) { తిరిగి బిందువు.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    కొరకు (స్థిరరాశి VlcPoints::ByName::విలువ_type& i : points) {
        స్థిరరాశి Vlc_బిందువు& pt = points.బిందువు_సంఖ్య(i.రెండవ);
        ఒకవేళ (!isCovergroup(pt)) కొనసాగు;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        స్థిరరాశి string hier = pt.hier();
        string bin = pt.bin();
        ఒకవేళ (bin.ఖాళీలు() || bin.కొలత() >= hier.కొలత()) bin = hier.substr(hier.rfind('.') + 1);
        స్థిరరాశి string అంశం_Hier
            = hier.substr(0, hier.కొలత() - std::min(hier.కొలత(), bin.కొలత() + 1));
        Cg_సమూహం& సమూహం = groups[pt.పుట()];
        సమూహం.కూడు_రికార్డు(అంశం_Hier.substr(0, అంశం_Hier.rfind('.')), కీలకం_సంఖ్య(pt.సమూహం_బరువు()));
        Cg_అంశం& అంశం_2 = సమూహం.కనుగొను_New_అంశం(అంశం_Hier);
        అంశం_2.కూడు_రికార్డు(కీలకం_సంఖ్య(pt.బరువు_2()));
        ఒకవేళ (!pt.binType().ఖాళీలు()) కొనసాగు;  // Not coverable
        అంశం_2.కనుగొను_New_Bin(bin_తటస్థం(pt.name())).కూడు_రికార్డు(bin, pt.లెక్క(), కీలకం_సంఖ్య(pt.thresh()));
    }
    తిరిగి groups;
}

Tally అంశం_Tally(స్థిరరాశి Cg_అంశం& అంశం_2) {
    Tally tally;
    కొరకు (స్థిరరాశి std::pair<స్థిరరాశి string, CgBin>& bin : అంశం_2.bins()) {
        tally.కూడు_బిందువు(bin.రెండవ.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.మొత్తం() ? pct(tally.తగిలింది(), tally.మొత్తం()) : అంశం_2.బరువు_2() ? 0.0 : 100.0);
    తిరిగి tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally సమూహం_Tally(స్థిరరాశి Cg_సమూహం& సమూహం, తార్కిక* contributesp = శూన్యసూచిక) {
    Tally tally;
    ద్విగుణ weighted = 0.0;
    ద్విగుణ weights = 0.0;
    కొరకు (స్థిరరాశి std::pair<స్థిరరాశి string, Cg_అంశం>& it : సమూహం.వస్తువులు()) {
        స్థిరరాశి Tally అంశం_2 = అంశం_Tally(it.రెండవ);
        tally.కూడు_Points(అంశం_2);
        ఒకవేళ (!అంశం_2.మొత్తం()) కొనసాగు;  // Excluded from both sums
        weighted += static_cast<ద్విగుణ>(it.రెండవ.బరువు_2()) * అంశం_2.score();
        weights += static_cast<ద్విగుణ>(it.రెండవ.బరువు_2());
    }
    ఒకవేళ (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : సమూహం.బరువు_2() ? 0.0 : 100.0);
    తిరిగి tally;
}

Tally groupsTally(స్థిరరాశి std::సదిశ<స్థిరరాశి Cg_సమూహం*>& groups) {
    Tally tally;
    ద్విగుణ weighted = 0.0;
    ద్విగుణ weights = 0.0;
    తార్కిక any_బరువు = అబద్ధం;
    కొరకు (స్థిరరాశి Cg_సమూహం* స్థిరరాశి groupp : groups) {
        తార్కిక contributes = అబద్ధం;
        స్థిరరాశి Tally సమూహం = సమూహం_Tally(*groupp, &contributes);
        tally.కూడు_Points(సమూహం);
        ఒకవేళ (groupp->బరువు_2()) any_బరువు = నిజం;
        ఒకవేళ (!contributes) కొనసాగు;
        weighted += static_cast<ద్విగుణ>(groupp->బరువు_2()) * సమూహం.score();
        weights += static_cast<ద్విగుణ>(groupp->బరువు_2());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : any_బరువు ? 0.0 : 100.0);
    తిరిగి tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(స్థిరరాశి CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::సదిశ<స్థిరరాశి Cg_సమూహం*>> కణుపు_Groups;
    కొరకు (స్థిరరాశి CgGroups::విలువ_type& it : groups) {
        స్థిరరాశి Cg_సమూహం& సమూహం = it.రెండవ;
        string మార్గం;
        కొరకు (స్థిరరాశి string& part : విభజించు_Hier(సమూహం.hier())) {
            మార్గం = మార్గం.ఖాళీలు() ? part : మార్గం + "." + part;
            కణుపు_Groups[మార్గం].నెట్టు_వెనుక(&సమూహం);
        }
        కొరకు (స్థిరరాశి std::pair<స్థిరరాశి string, Cg_అంశం>& అంశం_2 : సమూహం.వస్తువులు()) {
            tallies[అంశం_2.మొదటి] = అంశం_Tally(అంశం_2.రెండవ);
            కొరకు (స్థిరరాశి std::pair<స్థిరరాశి string, CgBin>& bin : అంశం_2.రెండవ.bins()) {
                // Of the bins of the name
                tallies[అంశం_2.మొదటి + "." + bin.రెండవ.name()].కూడు_బిందువు(bin.రెండవ.covered());
            }
        }
    }
    కొరకు (స్థిరరాశి std::pair<స్థిరరాశి string, std::సదిశ<స్థిరరాశి Cg_సమూహం*>>& it : కణుపు_Groups) {
        tallies[it.మొదటి]
            = it.రెండవ.కొలత() == 1 ? సమూహం_Tally(*it.రెండవ.front()) : groupsTally(it.రెండవ);
    }
    తిరిగి tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
శూన్య printIndent(కొలత_t indent) {
    కొరకు (కొలత_t i = 0; i < indent; ++i) std::బయటకు << ' ';
}

శూన్య print_Tally_అడ్డువరుస(స్థిరరాశి string& type, స్థిరరాశి Tally& tally, కొలత_t indent, కొలత_t type_వెడల్పు,
                   కొలత_t లెక్క_వెడల్పు) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    స్థిరరాశి string percent = tally.scored() ? pct_విలువ_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.తగిలింది(), tally.మొత్తం());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::బయటకు << std::ఎడమ << std::setw(type_వెడల్పు) << type << " : " << std::కుడి << std::fixed
              << std::setw(6) << percent << " (" << std::setw(లెక్క_వెడల్పు) << tally.తగిలింది() << "/"
              << std::setw(లెక్క_వెడల్పు) << tally.మొత్తం() << ")\n";
}

కొలత_t లెక్క_వెడల్పు(స్థిరరాశి TypeTally& tally) {
    కొలత_t వెడల్పు = cvt_గమ్యం_Str(0).కొలత();
    కొరకు (TypeTally::const_iterator it = tally.begin(); it != tally.చివర(); ++it) {
        వెడల్పు = std::max(వెడల్పు, cvt_గమ్యం_Str(it->రెండవ.తగిలింది()).కొలత());
        వెడల్పు = std::max(వెడల్పు, cvt_గమ్యం_Str(it->రెండవ.మొత్తం()).కొలత());
    }
    తిరిగి వెడల్పు;
}

కొలత_t type_వెడల్పు(స్థిరరాశి TypeTally& tally) {
    కొలత_t type_వెడల్పు = 0;
    కొరకు (స్థిరరాశి అక్షరం* స్థిరరాశి typep : s_orderedTypes) {
        స్థిరరాశి string type = typep;
        type_వెడల్పు = std::max(type_వెడల్పు, type.కొలత());
    }
    కొరకు (TypeTally::const_iterator it = tally.begin(); it != tally.చివర(); ++it) {
        type_వెడల్పు = std::max(type_వెడల్పు, it->మొదటి.కొలత());
    }
    తిరిగి type_వెడల్పు;
}

శూన్య printTypeTally(స్థిరరాశి TypeTally& tally, కొలత_t indent, తార్కిక includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    స్థిరరాశి కొలత_t typ_వెడల్పు = type_వెడల్పు(tally);
    స్థిరరాశి కొలత_t cnt_వెడల్పు = లెక్క_వెడల్పు(tally);
    కొరకు (స్థిరరాశి అక్షరం* స్థిరరాశి typep : s_orderedTypes) {
        స్థిరరాశి string type = typep;
        స్థిరరాశి TypeTally::const_iterator it = tally.కనుగొను(type);
        ఒకవేళ (it != tally.చివర()) {
            print_Tally_అడ్డువరుస(type, it->రెండవ, indent, typ_వెడల్పు, cnt_వెడల్పు);
        } లేకపోతే ఒకవేళ (includeMissingOrdered) {
            print_Tally_అడ్డువరుస(type, Tally{}, indent, typ_వెడల్పు, cnt_వెడల్పు);
        }
    }
    కొరకు (TypeTally::const_iterator it = tally.begin(); it != tally.చివర(); ++it) {
        ఒకవేళ (!isOrderedType(it->మొదటి)) {
            print_Tally_అడ్డువరుస(it->మొదటి, it->రెండవ, indent, typ_వెడల్పు, cnt_వెడల్పు);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
శూన్య printCovergroupTallies(స్థిరరాశి std::map<string, Tally>& tallies, పూర్ణాంకం స్థాయిలు) {
    std::map<string, Tally> shown;
    కొలత_t name_వెడల్పు = 0;
    కొరకు (స్థిరరాశి std::pair<స్థిరరాశి string, Tally>& it : tallies) {
        ఒకవేళ (స్థాయిలు >= 0 && static_cast<పూర్ణాంకం>(విభజించు_Hier(it.మొదటి).కొలత()) > స్థాయిలు + 1) కొనసాగు;
        shown.చొప్పించు(it);
        name_వెడల్పు = std::max(name_వెడల్పు, it.మొదటి.కొలత());
    }
    స్థిరరాశి కొలత_t cnt_వెడల్పు = లెక్క_వెడల్పు(shown);
    std::బయటకు << "Covergroup Coverage Summary:\n";
    కొరకు (స్థిరరాశి std::pair<స్థిరరాశి string, Tally>& it : shown) {
        print_Tally_అడ్డువరుస(it.మొదటి, it.రెండవ, s_summaryIndent, name_వెడల్పు, cnt_వెడల్పు);
    }
}

}  // namespace

శూన్య Vlc_శిఖరం::readCoverage(స్థిరరాశి string& filename, తార్కిక nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    ఒకవేళ (!is) {
        ఒకవేళ (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        తిరిగి;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* స్థిరరాశి testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    అయితే (!is.eof()) {
        స్థిరరాశి string పంక్తి = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        ఒకవేళ (పంక్తి[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            స్థిరరాశి string::కొలత_type secspace = పంక్తి.rfind("' ");
            ఒకవేళ (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                కొనసాగు;
            }
            స్థిరరాశి string బిందువు = పంక్తి.substr(3, secspace - 3);
            ఒకవేళ (!opt.is_Type_సరిపోలిక(బిందువు.c_str())) కొనసాగు;

            స్థిరరాశి uint64_t తగిలినవి = std::atoll(పంక్తి.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            స్థిరరాశి uint64_t pointnum = points().కనుగొను_కూడు_బిందువు(బిందువు, తగిలినవి);
            ఒకవేళ (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                స్థిరరాశి uint64_t needed = std::max(
                    VlcBuckets::sufficient(), కీలకం_సంఖ్య(points().బిందువు_సంఖ్య(pointnum).thresh()));
                ఒకవేళ (తగిలినవి >= needed) {
                    points().బిందువు_సంఖ్య(pointnum).testsCoveringInc();
                    testp->buckets().కూడు_దత్తాంశం(pointnum, తగిలినవి);
                }
            }
        }
    }
}

శూన్య Vlc_శిఖరం::writeCoverage(స్థిరరాశి string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    ఒకవేళ (!os) {
        v3fatal("Can't write file: " << filename);
        తిరిగి;
    }

    os << "# SystemC::Coverage-3\n";
    కొరకు (స్థిరరాశి స్వయంచాలక& i : m_points) {
        స్థిరరాశి Vlc_బిందువు& బిందువు = m_points.బిందువు_సంఖ్య(i.రెండవ);
        os << "C '" << బిందువు.name() << "' " << బిందువు.లెక్క() << '\n';
    }
}

శూన్య Vlc_శిఖరం::writeInfo(స్థిరరాశి string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    ఒకవేళ (!os) {
        v3fatal("Can't write file: " << filename);
        తిరిగి;
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
    కొరకు (స్వయంచాలక& si : m_sources) {
        Vlc_ఆధారం& ఆధారం = si.రెండవ;
        os << "SF:" << ఆధారం.name() << '\n';
        Vlc_ఆధారం::LinenoMap& పంక్తులు = ఆధారం.పంక్తులు();
        పూర్ణాంకం branches_దొరికింది = 0;
        పూర్ణాంకం branches_తగిలింది = 0;
        కొరకు (స్వయంచాలక& li : పంక్తులు) {
            Vlc_ఆధారం_లెక్క& sc = li.రెండవ;
            uint64_t da_లెక్క = 0;
            std::సదిశ<స్థిరరాశి Vlc_బిందువు*> infoPoints;
            కొరకు (స్థిరరాశి స్వయంచాలక& బిందువు : sc.points()) {
                da_లెక్క = std::max(da_లెక్క, బిందువు->లెక్క());
                infoPoints.నెట్టు_వెనుక(బిందువు);
            }
            os << "DA:" << sc.lineno() << "," << da_లెక్క << "\n";
            ఒకవేళ (infoPoints.కొలత() <= 1) కొనసాగు;
            branches_దొరికింది += static_cast<పూర్ణాంకం>(infoPoints.కొలత());
            పూర్ణాంకం బిందువు_అంకె_2 = 0;
            కొరకు (స్థిరరాశి Vlc_బిందువు* బిందువు : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                ఒకవేళ (బిందువు->isFsmArc()) {
                    os << "2,";
                    os << బిందువు->fsmFromState() << "->" << బిందువు->fsm_గమ్యం_State();
                } లేకపోతే ఒకవేళ (బిందువు->comment().ఖాళీలు()) {
                    os << "0,";
                    os << బిందువు_అంకె_2;
                } లేకపోతే {
                    os << (బిందువు->isFsmState() ? '1' : '0') << ',';
                    std::string comment(బిందువు->comment());
                    std::replace(comment.begin(), comment.చివర(), ',', '_');
                    os << comment;
                }
                os << "," << బిందువు->లెక్క() << "\n";

                branches_తగిలింది += opt.లెక్క_Ok(బిందువు->లెక్క());
                ++బిందువు_అంకె_2;
            }
        }
        os << "BRF:" << branches_దొరికింది << '\n';
        os << "BRH:" << branches_తగిలింది << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

నిర్మాణం CmpComputrons final {
    తార్కిక క్రియాకారకం()(స్థిరరాశి VlcTest* lhsp, స్థిరరాశి VlcTest* rhsp) స్థిరరాశి {
        ఒకవేళ (lhsp->computrons() != rhsp->computrons()) {
            తిరిగి lhsp->computrons() < rhsp->computrons();
        }
        తిరిగి lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

శూన్య Vlc_శిఖరం::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::సదిశ<VlcTest*> bytime;
    కొరకు (స్థిరరాశి స్వయంచాలక& testp : m_tests) {
        ఒకవేళ (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.నెట్టు_వెనుక(testp);
        }
    }
    క్రమీకరించు(bytime.begin(), bytime.చివర(), CmpComputrons());  // Sort the vector

    VlcBuckets మిగిలిన;
    కొరకు (స్థిరరాశి స్వయంచాలక& i : m_points) {
        స్థిరరాశి Vlc_బిందువు* స్థిరరాశి pointp = &points().బిందువు_సంఖ్య(i.రెండవ);
        // If any tests hit this point, then we'll need to cover it.
        ఒకవేళ (pointp->testsCovering()) మిగిలిన.కూడు_దత్తాంశం(pointp->బిందువు_అంకె(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    అయితే (నిజం) {
        ఒకవేళ (debug() >= 9) {
            UINFO_ఉపసర్గ("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            మిగిలిన.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* ఉత్తమ_Testp = శూన్యసూచిక;
        uint64_t ఉత్తమ_Remain = 0;
        కొరకు (స్థిరరాశి స్వయంచాలక& testp : bytime) {
            ఒకవేళ (!testp->rank()) {
                స్థిరరాశి uint64_t remain = testp->buckets().దత్తాంశం_తీయి_లెక్క(మిగిలిన);
                ఒకవేళ (remain > ఉత్తమ_Remain) {
                    ఉత్తమ_Testp = testp;
                    ఉత్తమ_Remain = remain;
                }
            }
        }
        ఒకవేళ (VlcTest* స్థిరరాశి testp = ఉత్తమ_Testp) {
            testp->rank(nextrank++);
            testp->rankPoints(ఉత్తమ_Remain);
            మిగిలిన.or_దత్తాంశం(ఉత్తమ_Testp->buckets());
        } లేకపోతే {
            ఆపు;  // No test covering more stuff found
        }
    }
}

శూన్య Vlc_శిఖరం::annotateCalc() {
    // Calculate per-line information into filedata structure
    కొరకు (స్థిరరాశి స్వయంచాలక& i : m_points) {
        స్థిరరాశి Vlc_బిందువు& బిందువు = m_points.బిందువు_సంఖ్య(i.రెండవ);
        స్థిరరాశి string filename = బిందువు.filename();
        స్థిరరాశి పూర్ణాంకం lineno = బిందువు.lineno();
        ఒకవేళ (!filename.ఖాళీలు() && lineno != 0) {
            Vlc_ఆధారం& ఆధారం = sources().కనుగొను_New_ఆధారం(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << బిందువు.స్తంభం_2() << " "
                                       << బిందువు.లెక్క() << " " << బిందువు.linescov());
            // Base coverage
            ఆధారం.చొప్పించు_బిందువు(lineno, &బిందువు);
            // Additional lines covered by this statement
            తార్కిక range = అబద్ధం;
            పూర్ణాంకం ప్రారంభం = 0;
            పూర్ణాంకం చివర = 0;
            స్థిరరాశి string linescov = బిందువు.linescov();
            కొరకు (స్థిరరాశి అక్షరం* covp = linescov.c_str(); నిజం; ++covp) {
                ఒకవేళ (!*covp || *covp == ',') {  // Ending
                    కొరకు (పూర్ణాంకం lni = ప్రారంభం; ప్రారంభం && lni <= చివర; ++lni) {
                        ఆధారం.చొప్పించు_బిందువు(lni, &బిందువు);
                    }
                    ఒకవేళ (!*covp) ఆపు;
                    ప్రారంభం = 0;  // Prep for next
                    చివర = 0;
                    range = అబద్ధం;
                } లేకపోతే ఒకవేళ (*covp == '-') {
                    range = నిజం;
                } లేకపోతే ఒకవేళ (std::అంకెనా(*covp)) {
                    స్థిరరాశి అక్షరం* స్థిరరాశి digitsp = covp;
                    అయితే (std::అంకెనా(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    ఒకవేళ (!range) ప్రారంభం = std::atoi(digitsp);
                    చివర = std::atoi(digitsp);
                }
            }
        }
    }
}

శూన్య Vlc_శిఖరం::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    పూర్ణాంకం totCases = 0;
    పూర్ణాంకం totOk = 0;
    కొరకు (స్వయంచాలక& si : m_sources) {
        Vlc_ఆధారం& ఆధారం = si.రెండవ;
        // UINFO(1, "Source " << source.name());
        ఒకవేళ (opt.annotateAll()) ఆధారం.needed(నిజం);
        స్థిరరాశి Vlc_ఆధారం::LinenoMap& పంక్తులు = ఆధారం.పంక్తులు();
        కొరకు (స్వయంచాలక& li : పంక్తులు) {
            స్థిరరాశి Vlc_ఆధారం_లెక్క& sc = li.రెండవ;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            ఒకవేళ (opt.లెక్క_Ok(sc.min_లెక్క())) {
                ++totOk;
            } లేకపోతే {
                ఆధారం.needed(నిజం);
            }
        }
    }
    std::బయటకు << "Annotation Summary:\n";
    std::బయటకు << "  lines with all attached points covered : ";
    std::బయటకు << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    ఒకవేళ (totOk != totCases) బయటకు << "See lines with '%00' in " << opt.annotate_బహిర్గతం() << '\n';
}

శూన్య Vlc_శిఖరం::annotateOutputFiles(స్థిరరాశి string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    కొరకు (స్వయంచాలక& si : m_sources) {
        Vlc_ఆధారం& ఆధారం = si.రెండవ;
        ఒకవేళ (!ఆధారం.needed()) కొనసాగు;
        స్థిరరాశి string filename = ఆధారం.name();
        స్థిరరాశి string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        ఒకవేళ (!is) {
            v3error("Can't read annotation file: " << filename);
            తిరిగి;
        }

        std::ofstream os{outfilename.c_str()};
        ఒకవేళ (!os) {
            v3error("Can't write file: " << outfilename);
            తిరిగి;
        }

        os << "//      // verilator_coverage annotation\n";

        పూర్ణాంకం lineno = 0;
        అయితే (!is.eof()) {
            lineno++;
            స్థిరరాశి std::string పంక్తి = V3Os::getline(is);

            Vlc_ఆధారం::LinenoMap& పంక్తులు = ఆధారం.పంక్తులు();
            స్థిరరాశి స్వయంచాలక lit = పంక్తులు.కనుగొను(lineno);
            ఒకవేళ (lit == పంక్తులు.చివర()) {
                os << "        " << పంక్తి << '\n';
            } లేకపోతే {
                Vlc_ఆధారం_లెక్క& sc = lit->రెండవ;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                స్థిరరాశి తార్కిక minOk = opt.లెక్క_Ok(sc.min_లెక్క());
                స్థిరరాశి తార్కిక maxOk = opt.లెక్క_Ok(sc.max_లెక్క());
                ఒకవేళ (minOk) {
                    os << " ";
                } లేకపోతే ఒకవేళ (maxOk) {
                    os << "~";
                } లేకపోతే {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_లెక్క() << " " << పంక్తి << '\n';

                ఒకవేళ (opt.annotatePoints()) {
                    కొరకు (స్థిరరాశి స్వయంచాలక& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                తార్కిక printed_Fsm_శీర్షిక = అబద్ధం;
                కొరకు (స్థిరరాశి స్వయంచాలక& pit : sc.points()) {
                    ఒకవేళ (!pit->isFsmState() && !pit->isFsmArc()) కొనసాగు;
                    ఒకవేళ (!printed_Fsm_శీర్షిక) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_శీర్షిక = నిజం;
                    }
                    os << (opt.లెక్క_Ok(pit->లెక్క()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->లెక్క() << "        ";
                    ఒకవేళ (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        ఒకవేళ (pit->లెక్క() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } లేకపోతే ఒకవేళ (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } లేకపోతే {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        ఒకవేళ (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

శూన్య Vlc_శిఖరం::annotate(స్థిరరాశి string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

శూన్య Vlc_శిఖరం::printTypeSummary() {
    TypeTally tally;
    కొరకు (VlcPoints::ByName::విలువ_type& i : m_points) {
        స్థిరరాశి Vlc_బిందువు& pt = m_points.బిందువు_సంఖ్య(i.రెండవ);
        ఒకవేళ (isCovergroup(pt)) కొనసాగు;  // Tallied below, by covergroup
        tally_బిందువు(tally, displayType(pt), pt.లెక్క());
    }
    స్థిరరాశి CgGroups groups = covergroups(m_points);
    ఒకవేళ (!groups.ఖాళీలు()) {
        std::సదిశ<స్థిరరాశి Cg_సమూహం*> groupps;
        కొరకు (స్థిరరాశి CgGroups::విలువ_type& it : groups) groupps.నెట్టు_వెనుక(&it.రెండవ);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    ఒకవేళ (tally.ఖాళీలు()) తిరిగి;
    std::బయటకు << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, నిజం);
}

శూన్య Vlc_శిఖరం::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    తార్కిక hasHier = అబద్ధం;
    తార్కిక hasCollapsedHier = అబద్ధం;
    కొరకు (VlcPoints::ByName::విలువ_type& i : m_points) {
        స్థిరరాశి Vlc_బిందువు& pt = m_points.బిందువు_సంఖ్య(i.రెండవ);
        ఒకవేళ (isCovergroup(pt)) కొనసాగు;  // Reported below, by covergroup
        స్థిరరాశి string hier = reportHier(pt);
        ఒకవేళ (hier.ఖాళీలు()) కొనసాగు;
        hasHier = నిజం;
        ఒకవేళ (isCollapsedHier(hier)) hasCollapsedHier = నిజం;
        స్థిరరాశి string type = displayType(pt);
        స్థిరరాశి std::సదిశ<string> parts = విభజించు_Hier(hier);
        string మార్గం;
        కొరకు (std::సదిశ<string>::const_iterator it = parts.begin(); it != parts.చివర(); ++it) {
            మార్గం = మార్గం.ఖాళీలు() ? *it : మార్గం + "." + *it;
            tally_బిందువు(hierTallies[మార్గం], type, pt.లెక్క());
        }
        tally_బిందువు(duTallies[duName(pt)], type, pt.లెక్క());
    }
    స్థిరరాశి std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    ఒకవేళ (!hasHier && !cgTallies.ఖాళీలు()) {
        printCovergroupTallies(cgTallies, opt.report_స్థాయిలు());
        తిరిగి;
    }
    ఒకవేళ (!hasHier) {
        std::బయటకు << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        తిరిగి;
    }

    స్థిరరాశి పూర్ణాంకం స్థాయిలు = opt.report_స్థాయిలు();
    ఒకవేళ (hasCollapsedHier) {
        std::బయటకు << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::బయటకు << "Hierarchy Coverage Summary:\n";
    కొరకు (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.చివర(); ++it) {
        స్థిరరాశి std::సదిశ<string> parts = విభజించు_Hier(it->మొదటి);
        ఒకవేళ (స్థాయిలు >= 0 && static_cast<పూర్ణాంకం>(parts.కొలత()) > స్థాయిలు + 1) కొనసాగు;
        printIndent(s_summaryIndent);
        std::బయటకు << it->మొదటి << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->రెండవ, s_report_అడ్డువరుస_Indent, అబద్ధం);
    }
    std::బయటకు << "Design Unit Coverage Summary:\n";
    కొరకు (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.చివర();
         ++it) {
        printIndent(s_summaryIndent);
        std::బయటకు << it->మొదటి << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->రెండవ, s_report_అడ్డువరుస_Indent, అబద్ధం);
    }
    ఒకవేళ (!cgTallies.ఖాళీలు()) printCovergroupTallies(cgTallies, స్థాయిలు);
}
