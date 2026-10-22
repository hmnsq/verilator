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

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "VlcTop.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"

#ಅನ್ತರ್ಭಾವ "VlcOptions.h"

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <fstream>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <sstream>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

//######################################################################

ನಾಮಾಕಾಶ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
ವರ್ಗ Tally final {
    // MEMBERS
    uint64_t m_ದೊರಕಿತು = 0;  // Covered points
    uint64_t m_ಒಟ್ಟು = 0;  // Points
    ತಾರ್ಕಿಕ m_scored = ಸುಳ್ಳು;  // Coverage is m_score, not the ratio of the points
    ದ್ವಿಗುಣ m_score = 0.0;  // Percent coverage, if m_scored

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    uint64_t ದೊರಕಿತು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ದೊರಕಿತು; }
    uint64_t ಒಟ್ಟು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಒಟ್ಟು; }
    ತಾರ್ಕಿಕ scored() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_scored; }
    ದ್ವಿಗುಣ score() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_score; }
    ಶೂನ್ಯ score(ದ್ವಿಗುಣ ಮೌಲ್ಯ) {
        m_scored = ನಿಜ;
        m_score = ಮೌಲ್ಯ;
    }
    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_ಬಿಂದು(ತಾರ್ಕಿಕ covered) {
        ಆದರೆ (covered) ++m_ದೊರಕಿತು;
        ++m_ಒಟ್ಟು;
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Points(ಸ್ಥಿರಾಂಕ Tally& other) {
        m_ದೊರಕಿತು += other.m_ದೊರಕಿತು;
        m_ಒಟ್ಟು += other.m_ಒಟ್ಟು;
    }
};

// Map coverage type to its tally.
ಬಳಸಿ TypeTally = std::map<std::string, Tally>;

ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ s_covergroupType = "covergroup";
ಸ್ಥಿರ constexpr ಗಾತ್ರ_t s_summaryIndent = 2;
ಸ್ಥಿರ constexpr ಗಾತ್ರ_t s_report_ಅಡ್ಡಸಾಲು_Indent = 4;

string displayType(ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& ಬಿಂದು) {
    ಸ್ಥಿರಾಂಕ string type = ಬಿಂದು.type();
    ಹಿಂತಿರುಗಿ type.ಖಾಲಿಗಳು() ? "point" : type;
}

ತಾರ್ಕಿಕ isCollapsedHier(ಸ್ಥಿರಾಂಕ string& hier) {
    ಹಿಂತಿರುಗಿ hier.ಕಂಡುಹಿಡಿ('*') != string::npos || hier.ಕಂಡುಹಿಡಿ('?') != string::npos;
}

ತಾರ್ಕಿಕ isOrderedType(ಸ್ಥಿರಾಂಕ string& type) {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ typep : s_orderedTypes) {
        ಆದರೆ (type == typep) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

string reportHier(ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& ಬಿಂದು) {
    // FSM records currently have useful instance scope in Fv.
    ಆದರೆ (ಬಿಂದು.isFsmState() || ಬಿಂದು.isFsmArc()) {
        ಸ್ಥಿರಾಂಕ string fsmVar = ಬಿಂದು.fsmVarName();
        ಹಿಂತಿರುಗಿ fsmVar.substr(0, fsmVar.rfind('.'));
    }
    ಹಿಂತಿರುಗಿ ಬಿಂದು.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::ಸದಿಶ<string> ವಿಭಜಿಸು_Hier(ಸ್ಥಿರಾಂಕ string& hier) {
    std::ಸದಿಶ<string> parts;
    string::ಗಾತ್ರ_type ಆರಂಭ = 0;
    ಪೂರ್ಣಾಂಕ ಆಳ = 0;  // Of the parentheses of the values of parameters
    string::ಗಾತ್ರ_type pos = 0;
    ಆಗ (pos < hier.ಗಾತ್ರ_2()) {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ c = hier[pos];
        ಆದರೆ (c == '\\') {
            pos = hier.ಕಂಡುಹಿಡಿ(' ', pos);  // npos if not ended
            ಮುಂದುವರಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಆಳ && hier.compare(pos, 2, "#(") == 0) {
            ಆಳ = 1;
            ++pos;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಆಳ && c == '.') {
            parts.ತಳ್ಳು_ಹಿಂಭಾಗ(hier.substr(ಆರಂಭ, pos - ಆರಂಭ));
            ಆರಂಭ = pos + 1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಆಳ && c == '"') {
            pos = VString::quoted_ಕೊನೆ(hier, pos);  // npos if not terminated
            ಮುಂದುವರಿ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಆಳ && c == '(') {
            ++ಆಳ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಆಳ && c == ')') {
            --ಆಳ;
        }
        ++pos;
    }
    ಆದರೆ (ಆಳ || pos != hier.ಗಾತ್ರ_2()) {  // Unbalanced
        parts.clear();
        ಆರಂಭ = 0;
        ಗಾಗಿ (string::ಗಾತ್ರ_type dot; (dot = hier.ಕಂಡುಹಿಡಿ('.', ಆರಂಭ)) != string::npos;
             ಆರಂಭ = dot + 1) {
            parts.ತಳ್ಳು_ಹಿಂಭಾಗ(hier.substr(ಆರಂಭ, dot - ಆರಂಭ));
        }
    }
    parts.ತಳ್ಳು_ಹಿಂಭಾಗ(hier.substr(ಆರಂಭ));
    ಹಿಂತಿರುಗಿ parts;
}

string duName(ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& ಬಿಂದು) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ಸ್ಥಿರಾಂಕ string ಪುಟ = ಬಿಂದು.ಪುಟ();
    ಹಿಂತಿರುಗಿ ಪುಟ.substr(ಪುಟ.ಕಂಡುಹಿಡಿ('/') + 1);
}

ಶೂನ್ಯ tally_ಬಿಂದು(TypeTally& tally, ಸ್ಥಿರಾಂಕ string& type, uint64_t ಎಣಿಕೆ_2) {
    tally[type].ಕೂಡಿಸು_ಬಿಂದು(ಎಣಿಕೆ_2 > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
ದ್ವಿಗುಣ pct(uint64_t ದೊರಕಿತು, uint64_t ಒಟ್ಟು) {
    ಹಿಂತಿರುಗಿ ಒಟ್ಟು ? (100.0 * static_cast<ದ್ವಿಗುಣ>(ದೊರಕಿತು) / static_cast<ದ್ವಿಗುಣ>(ಒಟ್ಟು)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_ಮೌಲ್ಯ_String(ದ್ವಿಗುಣ ಮೌಲ್ಯ, ತಾರ್ಕಿಕ complete) {
    ಆದರೆ (!complete && ಮೌಲ್ಯ > 99.9) ಮೌಲ್ಯ = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << ಮೌಲ್ಯ << "%";
    ಹಿಂತಿರುಗಿ os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t ದೊರಕಿತು, uint64_t ಒಟ್ಟು) {
    ಹಿಂತಿರುಗಿ pct_ಮೌಲ್ಯ_String(pct(ದೊರಕಿತು, ಒಟ್ಟು), ದೊರಕಿತು >= ಒಟ್ಟು);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
ವರ್ಗ CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_ಎಣಿಕೆ = 0;  // Hits, of all the records
    uint64_t m_at_ಕನಿಷ್ಠ = 0;  // option.at_least, the largest of the records

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    ಸ್ಥಿರಾಂಕ string& name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
    ತಾರ್ಕಿಕ covered() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಣಿಕೆ >= m_at_ಕನಿಷ್ಠ; }
    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_ದಾಖಲೆ(ಸ್ಥಿರಾಂಕ string& name, uint64_t ಎಣಿಕೆ_2, uint64_t at_ಕನಿಷ್ಠ) {
        m_name = name;
        m_ಎಣಿಕೆ += ಎಣಿಕೆ_2;
        m_at_ಕನಿಷ್ಠ = std::max(m_at_ಕನಿಷ್ಠ, at_ಕನಿಷ್ಠ);
    }
};

// A coverpoint or cross
ವರ್ಗ Cg_ಅಂಶ final {
    // MEMBERS
    uint64_t m_ತೂಕ = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    uint64_t ತೂಕ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತೂಕ; }
    ಸ್ಥಿರಾಂಕ std::map<string, CgBin>& bins() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bins; }
    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_ದಾಖಲೆ(uint64_t ತೂಕ_2) { m_ತೂಕ = std::max(m_ತೂಕ, ತೂಕ_2); }
    CgBin& ಕಂಡುಹಿಡಿ_New_Bin(ಸ್ಥಿರಾಂಕ string& ತಟಸ್ಥ) { ಹಿಂತಿರುಗಿ m_bins[ತಟಸ್ಥ]; }
};

// A covergroup
ವರ್ಗ Cg_ಗುಂಪು final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_ತೂಕ = 0;  // type_option.weight, the largest of the records
    std::map<string, Cg_ಅಂಶ> m_ವಸ್ತುಗಳು;  // By hierarchy

ಸಾರ್ವಜನಿಕ:
    // ACCESSORS
    ಸ್ಥಿರಾಂಕ string& hier() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hier; }
    uint64_t ತೂಕ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತೂಕ; }
    ಸ್ಥಿರಾಂಕ std::map<string, Cg_ಅಂಶ>& ವಸ್ತುಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ವಸ್ತುಗಳು; }
    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_ದಾಖಲೆ(ಸ್ಥಿರಾಂಕ string& hier, uint64_t ತೂಕ_2) {
        m_hier = hier;
        m_ತೂಕ = std::max(m_ತೂಕ, ತೂಕ_2);
    }
    Cg_ಅಂಶ& ಕಂಡುಹಿಡಿ_New_ಅಂಶ(ಸ್ಥಿರಾಂಕ string& hier) { ಹಿಂತಿರುಗಿ m_ವಸ್ತುಗಳು[hier]; }
};
ಬಳಸಿ CgGroups = std::map<string, Cg_ಗುಂಪು>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t ಕೀಲಿ_ಸಂಖ್ಯೆ(ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) {
    ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ.ಖಾಲಿಗಳು() ? 1 : std::strtoull(ಮೌಲ್ಯ.c_str(), ಶೂನ್ಯಸೂಚಕ, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
ತಾರ್ಕಿಕ is_Score_ಕ್ಷೇತ್ರ(ಸ್ಥಿರಾಂಕ string& ಕ್ಷೇತ್ರ) {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ keyp : {VL_CIK_THRESH, VL_CIK_ತೂಕ, VL_CIK_ಗುಂಪು_ತೂಕ}) {
        ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ = string{"\001"} + keyp + "\002";
        ಆದರೆ (ಕ್ಷೇತ್ರ.compare(0, ಪೂರ್ವಪ್ರತ್ಯಯ.ಗಾತ್ರ_2(), ಪೂರ್ವಪ್ರತ್ಯಯ) == 0) ಹಿಂತಿರುಗಿ ನಿಜ;
    }
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_ತಟಸ್ಥ(ಸ್ಥಿರಾಂಕ string& ದಾಖಲೆ_Name) {
    string ತಟಸ್ಥ;
    string::ಗಾತ್ರ_type ಆರಂಭ = 0;
    ಆಗ (ಆರಂಭ < ದಾಖಲೆ_Name.ಗಾತ್ರ_2()) {
        string::ಗಾತ್ರ_type ಕೊನೆ = ದಾಖಲೆ_Name.ಕಂಡುಹಿಡಿ('\001', ಆರಂಭ + 1);
        ಆದರೆ (ಕೊನೆ == string::npos) ಕೊನೆ = ದಾಖಲೆ_Name.ಗಾತ್ರ_2();
        ಸ್ಥಿರಾಂಕ string ಕ್ಷೇತ್ರ = ದಾಖಲೆ_Name.substr(ಆರಂಭ, ಕೊನೆ - ಆರಂಭ);
        ಆದರೆ (!is_Score_ಕ್ಷೇತ್ರ(ಕ್ಷೇತ್ರ)) ತಟಸ್ಥ += ಕ್ಷೇತ್ರ;
        ಆರಂಭ = ಕೊನೆ;
    }
    ಹಿಂತಿರುಗಿ ತಟಸ್ಥ;
}

ತಾರ್ಕಿಕ isCovergroup(ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& ಬಿಂದು) { ಹಿಂತಿರುಗಿ ಬಿಂದು.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VlcPoints::ByName::ಮೌಲ್ಯ_type& i : points) {
        ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& pt = points.ಬಿಂದು_ಸಂಖ್ಯೆ(i.ಎರಡನೇ);
        ಆದರೆ (!isCovergroup(pt)) ಮುಂದುವರಿ;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ಸ್ಥಿರಾಂಕ string hier = pt.hier();
        string bin = pt.bin();
        ಆದರೆ (bin.ಖಾಲಿಗಳು() || bin.ಗಾತ್ರ_2() >= hier.ಗಾತ್ರ_2()) bin = hier.substr(hier.rfind('.') + 1);
        ಸ್ಥಿರಾಂಕ string ಅಂಶ_Hier
            = hier.substr(0, hier.ಗಾತ್ರ_2() - std::min(hier.ಗಾತ್ರ_2(), bin.ಗಾತ್ರ_2() + 1));
        Cg_ಗುಂಪು& ಗುಂಪು = groups[pt.ಪುಟ()];
        ಗುಂಪು.ಕೂಡಿಸು_ದಾಖಲೆ(ಅಂಶ_Hier.substr(0, ಅಂಶ_Hier.rfind('.')), ಕೀಲಿ_ಸಂಖ್ಯೆ(pt.ಗುಂಪು_ತೂಕ()));
        Cg_ಅಂಶ& ಅಂಶ_2 = ಗುಂಪು.ಕಂಡುಹಿಡಿ_New_ಅಂಶ(ಅಂಶ_Hier);
        ಅಂಶ_2.ಕೂಡಿಸು_ದಾಖಲೆ(ಕೀಲಿ_ಸಂಖ್ಯೆ(pt.ತೂಕ_2()));
        ಆದರೆ (!pt.binType().ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;  // Not coverable
        ಅಂಶ_2.ಕಂಡುಹಿಡಿ_New_Bin(bin_ತಟಸ್ಥ(pt.name())).ಕೂಡಿಸು_ದಾಖಲೆ(bin, pt.ಎಣಿಕೆ_2(), ಕೀಲಿ_ಸಂಖ್ಯೆ(pt.thresh()));
    }
    ಹಿಂತಿರುಗಿ groups;
}

Tally ಅಂಶ_Tally(ಸ್ಥಿರಾಂಕ Cg_ಅಂಶ& ಅಂಶ_2) {
    Tally tally;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಸ್ಥಿರಾಂಕ string, CgBin>& bin : ಅಂಶ_2.bins()) {
        tally.ಕೂಡಿಸು_ಬಿಂದು(bin.ಎರಡನೇ.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.ಒಟ್ಟು() ? pct(tally.ದೊರಕಿತು(), tally.ಒಟ್ಟು()) : ಅಂಶ_2.ತೂಕ_2() ? 0.0 : 100.0);
    ಹಿಂತಿರುಗಿ tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally ಗುಂಪು_Tally(ಸ್ಥಿರಾಂಕ Cg_ಗುಂಪು& ಗುಂಪು, ತಾರ್ಕಿಕ* contributesp = ಶೂನ್ಯಸೂಚಕ) {
    Tally tally;
    ದ್ವಿಗುಣ weighted = 0.0;
    ದ್ವಿಗುಣ weights = 0.0;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಸ್ಥಿರಾಂಕ string, Cg_ಅಂಶ>& it : ಗುಂಪು.ವಸ್ತುಗಳು()) {
        ಸ್ಥಿರಾಂಕ Tally ಅಂಶ_2 = ಅಂಶ_Tally(it.ಎರಡನೇ);
        tally.ಕೂಡಿಸು_Points(ಅಂಶ_2);
        ಆದರೆ (!ಅಂಶ_2.ಒಟ್ಟು()) ಮುಂದುವರಿ;  // Excluded from both sums
        weighted += static_cast<ದ್ವಿಗುಣ>(it.ಎರಡನೇ.ತೂಕ_2()) * ಅಂಶ_2.score();
        weights += static_cast<ದ್ವಿಗುಣ>(it.ಎರಡನೇ.ತೂಕ_2());
    }
    ಆದರೆ (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : ಗುಂಪು.ತೂಕ_2() ? 0.0 : 100.0);
    ಹಿಂತಿರುಗಿ tally;
}

Tally groupsTally(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Cg_ಗುಂಪು*>& groups) {
    Tally tally;
    ದ್ವಿಗುಣ weighted = 0.0;
    ದ್ವಿಗುಣ weights = 0.0;
    ತಾರ್ಕಿಕ any_ತೂಕ = ಸುಳ್ಳು;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Cg_ಗುಂಪು* ಸ್ಥಿರಾಂಕ groupp : groups) {
        ತಾರ್ಕಿಕ contributes = ಸುಳ್ಳು;
        ಸ್ಥಿರಾಂಕ Tally ಗುಂಪು = ಗುಂಪು_Tally(*groupp, &contributes);
        tally.ಕೂಡಿಸು_Points(ಗುಂಪು);
        ಆದರೆ (groupp->ತೂಕ_2()) any_ತೂಕ = ನಿಜ;
        ಆದರೆ (!contributes) ಮುಂದುವರಿ;
        weighted += static_cast<ದ್ವಿಗುಣ>(groupp->ತೂಕ_2()) * ಗುಂಪು.score();
        weights += static_cast<ದ್ವಿಗುಣ>(groupp->ತೂಕ_2());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : any_ತೂಕ ? 0.0 : 100.0);
    ಹಿಂತಿರುಗಿ tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ಸ್ಥಿರಾಂಕ CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Cg_ಗುಂಪು*>> ಗಂಟು_Groups;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ CgGroups::ಮೌಲ್ಯ_type& it : groups) {
        ಸ್ಥಿರಾಂಕ Cg_ಗುಂಪು& ಗುಂಪು = it.ಎರಡನೇ;
        string ಮಾರ್ಗ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& part : ವಿಭಜಿಸು_Hier(ಗುಂಪು.hier())) {
            ಮಾರ್ಗ = ಮಾರ್ಗ.ಖಾಲಿಗಳು() ? part : ಮಾರ್ಗ + "." + part;
            ಗಂಟು_Groups[ಮಾರ್ಗ].ತಳ್ಳು_ಹಿಂಭಾಗ(&ಗುಂಪು);
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಸ್ಥಿರಾಂಕ string, Cg_ಅಂಶ>& ಅಂಶ_2 : ಗುಂಪು.ವಸ್ತುಗಳು()) {
            tallies[ಅಂಶ_2.ಮೊದಲ] = ಅಂಶ_Tally(ಅಂಶ_2.ಎರಡನೇ);
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಸ್ಥಿರಾಂಕ string, CgBin>& bin : ಅಂಶ_2.ಎರಡನೇ.bins()) {
                // Of the bins of the name
                tallies[ಅಂಶ_2.ಮೊದಲ + "." + bin.ಎರಡನೇ.name()].ಕೂಡಿಸು_ಬಿಂದು(bin.ಎರಡನೇ.covered());
            }
        }
    }
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಸ್ಥಿರಾಂಕ string, std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Cg_ಗುಂಪು*>>& it : ಗಂಟು_Groups) {
        tallies[it.ಮೊದಲ]
            = it.ಎರಡನೇ.ಗಾತ್ರ_2() == 1 ? ಗುಂಪು_Tally(*it.ಎರಡನೇ.front()) : groupsTally(it.ಎರಡನೇ);
    }
    ಹಿಂತಿರುಗಿ tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
ಶೂನ್ಯ printIndent(ಗಾತ್ರ_t indent) {
    ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < indent; ++i) std::ಹೊರಗೆ << ' ';
}

ಶೂನ್ಯ print_Tally_ಅಡ್ಡಸಾಲು(ಸ್ಥಿರಾಂಕ string& type, ಸ್ಥಿರಾಂಕ Tally& tally, ಗಾತ್ರ_t indent, ಗಾತ್ರ_t type_ಅಗಲ,
                   ಗಾತ್ರ_t ಎಣಿಕೆ_ಅಗಲ) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ಸ್ಥಿರಾಂಕ string percent = tally.scored() ? pct_ಮೌಲ್ಯ_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.ದೊರಕಿತು(), tally.ಒಟ್ಟು());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::ಹೊರಗೆ << std::ಎಡ << std::setw(type_ಅಗಲ) << type << " : " << std::ಬಲ << std::fixed
              << std::setw(6) << percent << " (" << std::setw(ಎಣಿಕೆ_ಅಗಲ) << tally.ದೊರಕಿತು() << "/"
              << std::setw(ಎಣಿಕೆ_ಅಗಲ) << tally.ಒಟ್ಟು() << ")\n";
}

ಗಾತ್ರ_t ಎಣಿಕೆ_ಅಗಲ(ಸ್ಥಿರಾಂಕ TypeTally& tally) {
    ಗಾತ್ರ_t ಅಗಲ = cvt_ಗಮ್ಯ_Str(0).ಗಾತ್ರ_2();
    ಗಾಗಿ (TypeTally::const_iterator it = tally.begin(); it != tally.ಕೊನೆ(); ++it) {
        ಅಗಲ = std::max(ಅಗಲ, cvt_ಗಮ್ಯ_Str(it->ಎರಡನೇ.ದೊರಕಿತು()).ಗಾತ್ರ_2());
        ಅಗಲ = std::max(ಅಗಲ, cvt_ಗಮ್ಯ_Str(it->ಎರಡನೇ.ಒಟ್ಟು()).ಗಾತ್ರ_2());
    }
    ಹಿಂತಿರುಗಿ ಅಗಲ;
}

ಗಾತ್ರ_t type_ಅಗಲ(ಸ್ಥಿರಾಂಕ TypeTally& tally) {
    ಗಾತ್ರ_t type_ಅಗಲ = 0;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ typep : s_orderedTypes) {
        ಸ್ಥಿರಾಂಕ string type = typep;
        type_ಅಗಲ = std::max(type_ಅಗಲ, type.ಗಾತ್ರ_2());
    }
    ಗಾಗಿ (TypeTally::const_iterator it = tally.begin(); it != tally.ಕೊನೆ(); ++it) {
        type_ಅಗಲ = std::max(type_ಅಗಲ, it->ಮೊದಲ.ಗಾತ್ರ_2());
    }
    ಹಿಂತಿರುಗಿ type_ಅಗಲ;
}

ಶೂನ್ಯ printTypeTally(ಸ್ಥಿರಾಂಕ TypeTally& tally, ಗಾತ್ರ_t indent, ತಾರ್ಕಿಕ includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t typ_ಅಗಲ = type_ಅಗಲ(tally);
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t cnt_ಅಗಲ = ಎಣಿಕೆ_ಅಗಲ(tally);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ typep : s_orderedTypes) {
        ಸ್ಥಿರಾಂಕ string type = typep;
        ಸ್ಥಿರಾಂಕ TypeTally::const_iterator it = tally.ಕಂಡುಹಿಡಿ(type);
        ಆದರೆ (it != tally.ಕೊನೆ()) {
            print_Tally_ಅಡ್ಡಸಾಲು(type, it->ಎರಡನೇ, indent, typ_ಅಗಲ, cnt_ಅಗಲ);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (includeMissingOrdered) {
            print_Tally_ಅಡ್ಡಸಾಲು(type, Tally{}, indent, typ_ಅಗಲ, cnt_ಅಗಲ);
        }
    }
    ಗಾಗಿ (TypeTally::const_iterator it = tally.begin(); it != tally.ಕೊನೆ(); ++it) {
        ಆದರೆ (!isOrderedType(it->ಮೊದಲ)) {
            print_Tally_ಅಡ್ಡಸಾಲು(it->ಮೊದಲ, it->ಎರಡನೇ, indent, typ_ಅಗಲ, cnt_ಅಗಲ);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
ಶೂನ್ಯ printCovergroupTallies(ಸ್ಥಿರಾಂಕ std::map<string, Tally>& tallies, ಪೂರ್ಣಾಂಕ ಮಟ್ಟಗಳು) {
    std::map<string, Tally> shown;
    ಗಾತ್ರ_t name_ಅಗಲ = 0;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಸ್ಥಿರಾಂಕ string, Tally>& it : tallies) {
        ಆದರೆ (ಮಟ್ಟಗಳು >= 0 && static_cast<ಪೂರ್ಣಾಂಕ>(ವಿಭಜಿಸು_Hier(it.ಮೊದಲ).ಗಾತ್ರ_2()) > ಮಟ್ಟಗಳು + 1) ಮುಂದುವರಿ;
        shown.ಸೇರಿಸು(it);
        name_ಅಗಲ = std::max(name_ಅಗಲ, it.ಮೊದಲ.ಗಾತ್ರ_2());
    }
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t cnt_ಅಗಲ = ಎಣಿಕೆ_ಅಗಲ(shown);
    std::ಹೊರಗೆ << "Covergroup Coverage Summary:\n";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::pair<ಸ್ಥಿರಾಂಕ string, Tally>& it : shown) {
        print_Tally_ಅಡ್ಡಸಾಲು(it.ಮೊದಲ, it.ಎರಡನೇ, s_summaryIndent, name_ಅಗಲ, cnt_ಅಗಲ);
    }
}

}  // namespace

ಶೂನ್ಯ Vlc_ತುದಿ::readCoverage(ಸ್ಥಿರಾಂಕ string& filename, ತಾರ್ಕಿಕ nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    ಆದರೆ (!is) {
        ಆದರೆ (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        ಹಿಂತಿರುಗಿ;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ಸ್ಥಿರಾಂಕ testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    ಆಗ (!is.eof()) {
        ಸ್ಥಿರಾಂಕ string ಸಾಲು = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        ಆದರೆ (ಸಾಲು[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type secspace = ಸಾಲು.rfind("' ");
            ಆದರೆ (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                ಮುಂದುವರಿ;
            }
            ಸ್ಥಿರಾಂಕ string ಬಿಂದು = ಸಾಲು.substr(3, secspace - 3);
            ಆದರೆ (!opt.is_Type_ಹೊಂದಿಕೆ(ಬಿಂದು.c_str())) ಮುಂದುವರಿ;

            ಸ್ಥಿರಾಂಕ uint64_t ದೊರಕಿದವು = std::atoll(ಸಾಲು.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ಸ್ಥಿರಾಂಕ uint64_t pointnum = points().ಕಂಡುಹಿಡಿ_ಕೂಡಿಸು_ಬಿಂದು(ಬಿಂದು, ದೊರಕಿದವು);
            ಆದರೆ (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ಸ್ಥಿರಾಂಕ uint64_t needed = std::max(
                    VlcBuckets::sufficient(), ಕೀಲಿ_ಸಂಖ್ಯೆ(points().ಬಿಂದು_ಸಂಖ್ಯೆ(pointnum).thresh()));
                ಆದರೆ (ದೊರಕಿದವು >= needed) {
                    points().ಬಿಂದು_ಸಂಖ್ಯೆ(pointnum).testsCoveringInc();
                    testp->buckets().ಕೂಡಿಸು_ದತ್ತಾಂಶ(pointnum, ದೊರಕಿದವು);
                }
            }
        }
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::writeCoverage(ಸ್ಥಿರಾಂಕ string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    ಆದರೆ (!os) {
        v3fatal("Can't write file: " << filename);
        ಹಿಂತಿರುಗಿ;
    }

    os << "# SystemC::Coverage-3\n";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : m_points) {
        ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& ಬಿಂದು = m_points.ಬಿಂದು_ಸಂಖ್ಯೆ(i.ಎರಡನೇ);
        os << "C '" << ಬಿಂದು.name() << "' " << ಬಿಂದು.ಎಣಿಕೆ_2() << '\n';
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::writeInfo(ಸ್ಥಿರಾಂಕ string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    ಆದರೆ (!os) {
        v3fatal("Can't write file: " << filename);
        ಹಿಂತಿರುಗಿ;
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
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& si : m_sources) {
        Vlc_ಮೂಲ& ಮೂಲ = si.ಎರಡನೇ;
        os << "SF:" << ಮೂಲ.name() << '\n';
        Vlc_ಮೂಲ::LinenoMap& ಸಾಲುಗಳು = ಮೂಲ.ಸಾಲುಗಳು();
        ಪೂರ್ಣಾಂಕ branches_ಸಿಕ್ಕಿತು = 0;
        ಪೂರ್ಣಾಂಕ branches_ದೊರಕಿತು = 0;
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& li : ಸಾಲುಗಳು) {
            Vlc_ಮೂಲ_ಎಣಿಕೆ& sc = li.ಎರಡನೇ;
            uint64_t da_ಎಣಿಕೆ = 0;
            std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು*> infoPoints;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಬಿಂದು : sc.points()) {
                da_ಎಣಿಕೆ = std::max(da_ಎಣಿಕೆ, ಬಿಂದು->ಎಣಿಕೆ_2());
                infoPoints.ತಳ್ಳು_ಹಿಂಭಾಗ(ಬಿಂದು);
            }
            os << "DA:" << sc.lineno() << "," << da_ಎಣಿಕೆ << "\n";
            ಆದರೆ (infoPoints.ಗಾತ್ರ_2() <= 1) ಮುಂದುವರಿ;
            branches_ಸಿಕ್ಕಿತು += static_cast<ಪೂರ್ಣಾಂಕ>(infoPoints.ಗಾತ್ರ_2());
            ಪೂರ್ಣಾಂಕ ಬಿಂದು_ಅಂಕೆ_2 = 0;
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು* ಬಿಂದು : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                ಆದರೆ (ಬಿಂದು->isFsmArc()) {
                    os << "2,";
                    os << ಬಿಂದು->fsmFromState() << "->" << ಬಿಂದು->fsm_ಗಮ್ಯ_State();
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಬಿಂದು->comment().ಖಾಲಿಗಳು()) {
                    os << "0,";
                    os << ಬಿಂದು_ಅಂಕೆ_2;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    os << (ಬಿಂದು->isFsmState() ? '1' : '0') << ',';
                    std::string comment(ಬಿಂದು->comment());
                    std::replace(comment.begin(), comment.ಕೊನೆ(), ',', '_');
                    os << comment;
                }
                os << "," << ಬಿಂದು->ಎಣಿಕೆ_2() << "\n";

                branches_ದೊರಕಿತು += opt.ಎಣಿಕೆ_Ok(ಬಿಂದು->ಎಣಿಕೆ_2());
                ++ಬಿಂದು_ಅಂಕೆ_2;
            }
        }
        os << "BRF:" << branches_ಸಿಕ್ಕಿತು << '\n';
        os << "BRH:" << branches_ದೊರಕಿತು << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

ರಚನೆ CmpComputrons final {
    ತಾರ್ಕಿಕ ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ VlcTest* lhsp, ಸ್ಥಿರಾಂಕ VlcTest* rhsp) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (lhsp->computrons() != rhsp->computrons()) {
            ಹಿಂತಿರುಗಿ lhsp->computrons() < rhsp->computrons();
        }
        ಹಿಂತಿರುಗಿ lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

ಶೂನ್ಯ Vlc_ತುದಿ::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::ಸದಿಶ<VlcTest*> bytime;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& testp : m_tests) {
        ಆದರೆ (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ತಳ್ಳು_ಹಿಂಭಾಗ(testp);
        }
    }
    ವಿಂಗಡಿಸು(bytime.begin(), bytime.ಕೊನೆ(), CmpComputrons());  // Sort the vector

    VlcBuckets ಉಳಿದ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : m_points) {
        ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು* ಸ್ಥಿರಾಂಕ pointp = &points().ಬಿಂದು_ಸಂಖ್ಯೆ(i.ಎರಡನೇ);
        // If any tests hit this point, then we'll need to cover it.
        ಆದರೆ (pointp->testsCovering()) ಉಳಿದ.ಕೂಡಿಸು_ದತ್ತಾಂಶ(pointp->ಬಿಂದು_ಅಂಕೆ(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    ಆಗ (ನಿಜ) {
        ಆದರೆ (debug() >= 9) {
            UINFO_ಪೂರ್ವಪ್ರತ್ಯಯ("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            ಉಳಿದ.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* ಉತ್ತಮ_Testp = ಶೂನ್ಯಸೂಚಕ;
        uint64_t ಉತ್ತಮ_Remain = 0;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& testp : bytime) {
            ಆದರೆ (!testp->rank()) {
                ಸ್ಥಿರಾಂಕ uint64_t remain = testp->buckets().ದತ್ತಾಂಶ_ತೆಗೆ_ಎಣಿಕೆ(ಉಳಿದ);
                ಆದರೆ (remain > ಉತ್ತಮ_Remain) {
                    ಉತ್ತಮ_Testp = testp;
                    ಉತ್ತಮ_Remain = remain;
                }
            }
        }
        ಆದರೆ (VlcTest* ಸ್ಥಿರಾಂಕ testp = ಉತ್ತಮ_Testp) {
            testp->rank(nextrank++);
            testp->rankPoints(ಉತ್ತಮ_Remain);
            ಉಳಿದ.or_ದತ್ತಾಂಶ(ಉತ್ತಮ_Testp->buckets());
        } ಇಲ್ಲದಿದ್ದರೆ {
            ನಿಲ್ಲಿಸು;  // No test covering more stuff found
        }
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::annotateCalc() {
    // Calculate per-line information into filedata structure
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : m_points) {
        ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& ಬಿಂದು = m_points.ಬಿಂದು_ಸಂಖ್ಯೆ(i.ಎರಡನೇ);
        ಸ್ಥಿರಾಂಕ string filename = ಬಿಂದು.filename();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lineno = ಬಿಂದು.lineno();
        ಆದರೆ (!filename.ಖಾಲಿಗಳು() && lineno != 0) {
            Vlc_ಮೂಲ& ಮೂಲ = sources().ಕಂಡುಹಿಡಿ_New_ಮೂಲ(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << ಬಿಂದು.ಸ್ತಂಭ_2() << " "
                                       << ಬಿಂದು.ಎಣಿಕೆ_2() << " " << ಬಿಂದು.linescov());
            // Base coverage
            ಮೂಲ.ಸೇರಿಸು_ಬಿಂದು(lineno, &ಬಿಂದು);
            // Additional lines covered by this statement
            ತಾರ್ಕಿಕ range = ಸುಳ್ಳು;
            ಪೂರ್ಣಾಂಕ ಆರಂಭ = 0;
            ಪೂರ್ಣಾಂಕ ಕೊನೆ = 0;
            ಸ್ಥಿರಾಂಕ string linescov = ಬಿಂದು.linescov();
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* covp = linescov.c_str(); ನಿಜ; ++covp) {
                ಆದರೆ (!*covp || *covp == ',') {  // Ending
                    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ lni = ಆರಂಭ; ಆರಂಭ && lni <= ಕೊನೆ; ++lni) {
                        ಮೂಲ.ಸೇರಿಸು_ಬಿಂದು(lni, &ಬಿಂದು);
                    }
                    ಆದರೆ (!*covp) ನಿಲ್ಲಿಸು;
                    ಆರಂಭ = 0;  // Prep for next
                    ಕೊನೆ = 0;
                    range = ಸುಳ್ಳು;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*covp == '-') {
                    range = ನಿಜ;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::ಅಂಕಿಯೇ(*covp)) {
                    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ digitsp = covp;
                    ಆಗ (std::ಅಂಕಿಯೇ(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    ಆದರೆ (!range) ಆರಂಭ = std::atoi(digitsp);
                    ಕೊನೆ = std::atoi(digitsp);
                }
            }
        }
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    ಪೂರ್ಣಾಂಕ totCases = 0;
    ಪೂರ್ಣಾಂಕ totOk = 0;
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& si : m_sources) {
        Vlc_ಮೂಲ& ಮೂಲ = si.ಎರಡನೇ;
        // UINFO(1, "Source " << source.name());
        ಆದರೆ (opt.annotateAll()) ಮೂಲ.needed(ನಿಜ);
        ಸ್ಥಿರಾಂಕ Vlc_ಮೂಲ::LinenoMap& ಸಾಲುಗಳು = ಮೂಲ.ಸಾಲುಗಳು();
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& li : ಸಾಲುಗಳು) {
            ಸ್ಥಿರಾಂಕ Vlc_ಮೂಲ_ಎಣಿಕೆ& sc = li.ಎರಡನೇ;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            ಆದರೆ (opt.ಎಣಿಕೆ_Ok(sc.min_ಎಣಿಕೆ())) {
                ++totOk;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಮೂಲ.needed(ನಿಜ);
            }
        }
    }
    std::ಹೊರಗೆ << "Annotation Summary:\n";
    std::ಹೊರಗೆ << "  lines with all attached points covered : ";
    std::ಹೊರಗೆ << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    ಆದರೆ (totOk != totCases) ಹೊರಗೆ << "See lines with '%00' in " << opt.annotate_ಹೊರಹೋಗು() << '\n';
}

ಶೂನ್ಯ Vlc_ತುದಿ::annotateOutputFiles(ಸ್ಥಿರಾಂಕ string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ& si : m_sources) {
        Vlc_ಮೂಲ& ಮೂಲ = si.ಎರಡನೇ;
        ಆದರೆ (!ಮೂಲ.needed()) ಮುಂದುವರಿ;
        ಸ್ಥಿರಾಂಕ string filename = ಮೂಲ.name();
        ಸ್ಥಿರಾಂಕ string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        ಆದರೆ (!is) {
            v3error("Can't read annotation file: " << filename);
            ಹಿಂತಿರುಗಿ;
        }

        std::ofstream os{outfilename.c_str()};
        ಆದರೆ (!os) {
            v3error("Can't write file: " << outfilename);
            ಹಿಂತಿರುಗಿ;
        }

        os << "//      // verilator_coverage annotation\n";

        ಪೂರ್ಣಾಂಕ lineno = 0;
        ಆಗ (!is.eof()) {
            lineno++;
            ಸ್ಥಿರಾಂಕ std::string ಸಾಲು = V3Os::getline(is);

            Vlc_ಮೂಲ::LinenoMap& ಸಾಲುಗಳು = ಮೂಲ.ಸಾಲುಗಳು();
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ lit = ಸಾಲುಗಳು.ಕಂಡುಹಿಡಿ(lineno);
            ಆದರೆ (lit == ಸಾಲುಗಳು.ಕೊನೆ()) {
                os << "        " << ಸಾಲು << '\n';
            } ಇಲ್ಲದಿದ್ದರೆ {
                Vlc_ಮೂಲ_ಎಣಿಕೆ& sc = lit->ಎರಡನೇ;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ minOk = opt.ಎಣಿಕೆ_Ok(sc.min_ಎಣಿಕೆ());
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ maxOk = opt.ಎಣಿಕೆ_Ok(sc.max_ಎಣಿಕೆ());
                ಆದರೆ (minOk) {
                    os << " ";
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (maxOk) {
                    os << "~";
                } ಇಲ್ಲದಿದ್ದರೆ {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_ಎಣಿಕೆ() << " " << ಸಾಲು << '\n';

                ಆದರೆ (opt.annotatePoints()) {
                    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                ತಾರ್ಕಿಕ printed_Fsm_ಶೀರ್ಷಿಕೆ = ಸುಳ್ಳು;
                ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pit : sc.points()) {
                    ಆದರೆ (!pit->isFsmState() && !pit->isFsmArc()) ಮುಂದುವರಿ;
                    ಆದರೆ (!printed_Fsm_ಶೀರ್ಷಿಕೆ) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_ಶೀರ್ಷಿಕೆ = ನಿಜ;
                    }
                    os << (opt.ಎಣಿಕೆ_Ok(pit->ಎಣಿಕೆ_2()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->ಎಣಿಕೆ_2() << "        ";
                    ಆದರೆ (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        ಆದರೆ (pit->ಎಣಿಕೆ_2() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        ಆದರೆ (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

ಶೂನ್ಯ Vlc_ತುದಿ::annotate(ಸ್ಥಿರಾಂಕ string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

ಶೂನ್ಯ Vlc_ತುದಿ::printTypeSummary() {
    TypeTally tally;
    ಗಾಗಿ (VlcPoints::ByName::ಮೌಲ್ಯ_type& i : m_points) {
        ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& pt = m_points.ಬಿಂದು_ಸಂಖ್ಯೆ(i.ಎರಡನೇ);
        ಆದರೆ (isCovergroup(pt)) ಮುಂದುವರಿ;  // Tallied below, by covergroup
        tally_ಬಿಂದು(tally, displayType(pt), pt.ಎಣಿಕೆ_2());
    }
    ಸ್ಥಿರಾಂಕ CgGroups groups = covergroups(m_points);
    ಆದರೆ (!groups.ಖಾಲಿಗಳು()) {
        std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Cg_ಗುಂಪು*> groupps;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ CgGroups::ಮೌಲ್ಯ_type& it : groups) groupps.ತಳ್ಳು_ಹಿಂಭಾಗ(&it.ಎರಡನೇ);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    ಆದರೆ (tally.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
    std::ಹೊರಗೆ << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, ನಿಜ);
}

ಶೂನ್ಯ Vlc_ತುದಿ::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    ತಾರ್ಕಿಕ hasHier = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ hasCollapsedHier = ಸುಳ್ಳು;
    ಗಾಗಿ (VlcPoints::ByName::ಮೌಲ್ಯ_type& i : m_points) {
        ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು& pt = m_points.ಬಿಂದು_ಸಂಖ್ಯೆ(i.ಎರಡನೇ);
        ಆದರೆ (isCovergroup(pt)) ಮುಂದುವರಿ;  // Reported below, by covergroup
        ಸ್ಥಿರಾಂಕ string hier = reportHier(pt);
        ಆದರೆ (hier.ಖಾಲಿಗಳು()) ಮುಂದುವರಿ;
        hasHier = ನಿಜ;
        ಆದರೆ (isCollapsedHier(hier)) hasCollapsedHier = ನಿಜ;
        ಸ್ಥಿರಾಂಕ string type = displayType(pt);
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<string> parts = ವಿಭಜಿಸು_Hier(hier);
        string ಮಾರ್ಗ;
        ಗಾಗಿ (std::ಸದಿಶ<string>::const_iterator it = parts.begin(); it != parts.ಕೊನೆ(); ++it) {
            ಮಾರ್ಗ = ಮಾರ್ಗ.ಖಾಲಿಗಳು() ? *it : ಮಾರ್ಗ + "." + *it;
            tally_ಬಿಂದು(hierTallies[ಮಾರ್ಗ], type, pt.ಎಣಿಕೆ_2());
        }
        tally_ಬಿಂದು(duTallies[duName(pt)], type, pt.ಎಣಿಕೆ_2());
    }
    ಸ್ಥಿರಾಂಕ std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    ಆದರೆ (!hasHier && !cgTallies.ಖಾಲಿಗಳು()) {
        printCovergroupTallies(cgTallies, opt.report_ಮಟ್ಟಗಳು());
        ಹಿಂತಿರುಗಿ;
    }
    ಆದರೆ (!hasHier) {
        std::ಹೊರಗೆ << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        ಹಿಂತಿರುಗಿ;
    }

    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮಟ್ಟಗಳು = opt.report_ಮಟ್ಟಗಳು();
    ಆದರೆ (hasCollapsedHier) {
        std::ಹೊರಗೆ << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::ಹೊರಗೆ << "Hierarchy Coverage Summary:\n";
    ಗಾಗಿ (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.ಕೊನೆ(); ++it) {
        ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<string> parts = ವಿಭಜಿಸು_Hier(it->ಮೊದಲ);
        ಆದರೆ (ಮಟ್ಟಗಳು >= 0 && static_cast<ಪೂರ್ಣಾಂಕ>(parts.ಗಾತ್ರ_2()) > ಮಟ್ಟಗಳು + 1) ಮುಂದುವರಿ;
        printIndent(s_summaryIndent);
        std::ಹೊರಗೆ << it->ಮೊದಲ << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->ಎರಡನೇ, s_report_ಅಡ್ಡಸಾಲು_Indent, ಸುಳ್ಳು);
    }
    std::ಹೊರಗೆ << "Design Unit Coverage Summary:\n";
    ಗಾಗಿ (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.ಕೊನೆ();
         ++it) {
        printIndent(s_summaryIndent);
        std::ಹೊರಗೆ << it->ಮೊದಲ << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->ಎರಡನೇ, s_report_ಅಡ್ಡಸಾಲು_Indent, ಸುಳ್ಳು);
    }
    ಆದರೆ (!cgTallies.ಖಾಲಿಗಳು()) printCovergroupTallies(cgTallies, ಮಟ್ಟಗಳು);
}
