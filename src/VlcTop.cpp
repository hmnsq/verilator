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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "VlcTop.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "VlcOptions.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <fstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

//######################################################################

ਨਾਮਾਕਾਸ਼ {

// Report helpers only.  They keep the flat summary and hierarchy report using
// the same hit/total accounting and output formatting.  These helpers read the
// fields already exposed by VlcPoint; they do not affect coverage point
// identity, merging, or .dat writing.

// Coverage of a report row: its covered and total points, and for covergroups, their
// coverage, which weights coverpoints and crosses rather than counting bins
ਵਰਗ Tally final {
    // MEMBERS
    uint64_t m_ਪ੍ਰਾਪਤੀ = 0;  // Covered points
    uint64_t m_ਕੁੱਲ = 0;  // Points
    ਤਾਰਕਿਕ m_scored = ਝੂਠ;  // Coverage is m_score, not the ratio of the points
    ਦੁੱਗਣਾ m_score = 0.0;  // Percent coverage, if m_scored

ਜਨਤਕ:
    // ACCESSORS
    uint64_t ਪ੍ਰਾਪਤੀ() ਅਚਲ { ਵਾਪਸ m_ਪ੍ਰਾਪਤੀ; }
    uint64_t ਕੁੱਲ() ਅਚਲ { ਵਾਪਸ m_ਕੁੱਲ; }
    ਤਾਰਕਿਕ scored() ਅਚਲ { ਵਾਪਸ m_scored; }
    ਦੁੱਗਣਾ score() ਅਚਲ { ਵਾਪਸ m_score; }
    ਖਾਲੀ score(ਦੁੱਗਣਾ ਮੁੱਲ) {
        m_scored = ਸੱਚ;
        m_score = ਮੁੱਲ;
    }
    // METHODS
    ਖਾਲੀ ਜੋੜੋ_ਬਿੰਦੂ(ਤਾਰਕਿਕ covered) {
        ਜੇ (covered) ++m_ਪ੍ਰਾਪਤੀ;
        ++m_ਕੁੱਲ;
    }
    ਖਾਲੀ ਜੋੜੋ_Points(ਅਚਲ Tally& other) {
        m_ਪ੍ਰਾਪਤੀ += other.m_ਪ੍ਰਾਪਤੀ;
        m_ਕੁੱਲ += other.m_ਕੁੱਲ;
    }
};

// Map coverage type to its tally.
ਵਰਤੋਂ TypeTally = std::map<std::string, Tally>;

ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ s_orderedTypes[]
    = {"line", "toggle", "branch", "expr", "fsm_state", "fsm_arc"};
ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ s_covergroupType = "covergroup";
ਸਥਿਰ constexpr ਮਾਪ_t s_summaryIndent = 2;
ਸਥਿਰ constexpr ਮਾਪ_t s_report_ਪੰਕਤੀ_Indent = 4;

string displayType(ਅਚਲ Vlc_ਬਿੰਦੂ& ਬਿੰਦੂ) {
    ਅਚਲ string type = ਬਿੰਦੂ.type();
    ਵਾਪਸ type.ਖਾਲੀ_ਥਾਵਾਂ() ? "point" : type;
}

ਤਾਰਕਿਕ isCollapsedHier(ਅਚਲ string& hier) {
    ਵਾਪਸ hier.ਲੱਭੋ('*') != string::npos || hier.ਲੱਭੋ('?') != string::npos;
}

ਤਾਰਕਿਕ isOrderedType(ਅਚਲ string& type) {
    ਲਈ (ਅਚਲ ਅੱਖਰ* ਅਚਲ typep : s_orderedTypes) {
        ਜੇ (type == typep) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}

string reportHier(ਅਚਲ Vlc_ਬਿੰਦੂ& ਬਿੰਦੂ) {
    // FSM records currently have useful instance scope in Fv.
    ਜੇ (ਬਿੰਦੂ.isFsmState() || ਬਿੰਦੂ.isFsmArc()) {
        ਅਚਲ string fsmVar = ਬਿੰਦੂ.fsmVarName();
        ਵਾਪਸ fsmVar.substr(0, fsmVar.rfind('.'));
    }
    ਵਾਪਸ ਬਿੰਦੂ.hier();
}

// The components of a hierarchy, split at its dots.  A covergroup's is the name of its type, as
// $typename names it (dtypeName() in V3AstNodes.cpp), whose escaped identifiers, from a '\' to
// the white space ending each (IEEE 1800-2023 23.6), hold no separating dots; nor do the values
// of the parameters of a specialization, '#(...)', as of a real value, or the string values
// within, which V3OutFormatter::quoteNameControls() escapes, as VString::quotedEnd() finds the
// end of.  A name whose identifiers, parentheses, or quotes do not end is not as $typename writes
// a type, so splits at each dot.
std::ਸਦਿਸ਼<string> ਵੰਡੋ_Hier(ਅਚਲ string& hier) {
    std::ਸਦਿਸ਼<string> parts;
    string::ਮਾਪ_type ਸ਼ੁਰੂ = 0;
    ਪੂਰਨ ਡੂੰਘਾਈ = 0;  // Of the parentheses of the values of parameters
    string::ਮਾਪ_type pos = 0;
    ਜਦੋਂ (pos < hier.ਮਾਪ()) {
        ਅਚਲ ਅੱਖਰ c = hier[pos];
        ਜੇ (c == '\\') {
            pos = hier.ਲੱਭੋ(' ', pos);  // npos if not ended
            ਜਾਰੀ;
        } ਨਹੀਂਤਾਂ ਜੇ (!ਡੂੰਘਾਈ && hier.compare(pos, 2, "#(") == 0) {
            ਡੂੰਘਾਈ = 1;
            ++pos;
        } ਨਹੀਂਤਾਂ ਜੇ (!ਡੂੰਘਾਈ && c == '.') {
            parts.ਧੱਕੋ_ਪਿੱਠ(hier.substr(ਸ਼ੁਰੂ, pos - ਸ਼ੁਰੂ));
            ਸ਼ੁਰੂ = pos + 1;
        } ਨਹੀਂਤਾਂ ਜੇ (ਡੂੰਘਾਈ && c == '"') {
            pos = VString::quoted_ਸਿਰਾ(hier, pos);  // npos if not terminated
            ਜਾਰੀ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਡੂੰਘਾਈ && c == '(') {
            ++ਡੂੰਘਾਈ;
        } ਨਹੀਂਤਾਂ ਜੇ (ਡੂੰਘਾਈ && c == ')') {
            --ਡੂੰਘਾਈ;
        }
        ++pos;
    }
    ਜੇ (ਡੂੰਘਾਈ || pos != hier.ਮਾਪ()) {  // Unbalanced
        parts.clear();
        ਸ਼ੁਰੂ = 0;
        ਲਈ (string::ਮਾਪ_type dot; (dot = hier.ਲੱਭੋ('.', ਸ਼ੁਰੂ)) != string::npos;
             ਸ਼ੁਰੂ = dot + 1) {
            parts.ਧੱਕੋ_ਪਿੱਠ(hier.substr(ਸ਼ੁਰੂ, dot - ਸ਼ੁਰੂ));
        }
    }
    parts.ਧੱਕੋ_ਪਿੱਠ(hier.substr(ਸ਼ੁਰੂ));
    ਵਾਪਸ parts;
}

string duName(ਅਚਲ Vlc_ਬਿੰਦੂ& ਬਿੰਦੂ) {
    // Pages are emitted as v_<type>/<design-unit> for RTL coverage.  Use the
    // suffix as the design-unit summary key.
    ਅਚਲ string ਪੰਨਾ = ਬਿੰਦੂ.ਪੰਨਾ();
    ਵਾਪਸ ਪੰਨਾ.substr(ਪੰਨਾ.ਲੱਭੋ('/') + 1);
}

ਖਾਲੀ tally_ਬਿੰਦੂ(TypeTally& tally, ਅਚਲ string& type, uint64_t ਗਿਣਤੀ_2) {
    tally[type].ਜੋੜੋ_ਬਿੰਦੂ(ਗਿਣਤੀ_2 > 0);
}

// Keep the percentage calculation in one place so flat summaries and hierarchy
// reports cannot drift in zero-total handling.
ਦੁੱਗਣਾ pct(uint64_t ਪ੍ਰਾਪਤੀ, uint64_t ਕੁੱਲ) {
    ਵਾਪਸ ਕੁੱਲ ? (100.0 * static_cast<ਦੁੱਗਣਾ>(ਪ੍ਰਾਪਤੀ) / static_cast<ਦੁੱਗਣਾ>(ਕੁੱਲ)) : 0.0;
}

// Return a percentage as a string, handling 100%: coverage not complete shows below it.
string pct_ਮੁੱਲ_String(ਦੁੱਗਣਾ ਮੁੱਲ, ਤਾਰਕਿਕ complete) {
    ਜੇ (!complete && ਮੁੱਲ > 99.9) ਮੁੱਲ = 99.9;  // Presumes precision 1
    std::stringstream os;
    os << std::fixed << std::setprecision(1)  // 1 matters, see above
       << ਮੁੱਲ << "%";
    ਵਾਪਸ os.str();
}

// Return percentage as a string, handling 100%, and protecting from div zero.
string pctString(uint64_t ਪ੍ਰਾਪਤੀ, uint64_t ਕੁੱਲ) {
    ਵਾਪਸ pct_ਮੁੱਲ_String(pct(ਪ੍ਰਾਪਤੀ, ਕੁੱਲ), ਪ੍ਰਾਪਤੀ >= ਕੁੱਲ);
}

// Covergroup coverage (IEEE 1800-2023 19.11).  An item, a coverpoint or cross, covers the
// ratio of its bins that reached option.at_least, excluding ignore, illegal and default bins;
// a covergroup, the average of its items, weighted by their weight in the records (a constant
// option.weight, else type_option.weight); and the covergroups together, their average
// weighted by type_option.weight.  The coverage database merges the instances of a covergroup
// type, so this is the coverage of the merged instances, which is what get_coverage() returns
// for a single instance.

// A coverable bin of a coverpoint or cross, of all its records
ਵਰਗ CgBin final {
    // MEMBERS
    string m_name;  // Name of the bin
    uint64_t m_ਗਿਣਤੀ = 0;  // Hits, of all the records
    uint64_t m_at_ਘੱਟ_ਤੋਂ_ਘੱਟ = 0;  // option.at_least, the largest of the records

ਜਨਤਕ:
    // ACCESSORS
    ਅਚਲ string& name() ਅਚਲ { ਵਾਪਸ m_name; }
    ਤਾਰਕਿਕ covered() ਅਚਲ { ਵਾਪਸ m_ਗਿਣਤੀ >= m_at_ਘੱਟ_ਤੋਂ_ਘੱਟ; }
    // METHODS
    ਖਾਲੀ ਜੋੜੋ_ਰਿਕਾਰਡ(ਅਚਲ string& name, uint64_t ਗਿਣਤੀ_2, uint64_t at_ਘੱਟ_ਤੋਂ_ਘੱਟ) {
        m_name = name;
        m_ਗਿਣਤੀ += ਗਿਣਤੀ_2;
        m_at_ਘੱਟ_ਤੋਂ_ਘੱਟ = std::max(m_at_ਘੱਟ_ਤੋਂ_ਘੱਟ, at_ਘੱਟ_ਤੋਂ_ਘੱਟ);
    }
};

// A coverpoint or cross
ਵਰਗ Cg_ਵਸਤੂ final {
    // MEMBERS
    uint64_t m_ਭਾਰ = 0;  // The weight of the records, the largest
    std::map<string, CgBin> m_bins;  // Coverable bins, by binIdentity()

ਜਨਤਕ:
    // ACCESSORS
    uint64_t ਭਾਰ_2() ਅਚਲ { ਵਾਪਸ m_ਭਾਰ; }
    ਅਚਲ std::map<string, CgBin>& bins() ਅਚਲ { ਵਾਪਸ m_bins; }
    // METHODS
    ਖਾਲੀ ਜੋੜੋ_ਰਿਕਾਰਡ(uint64_t ਭਾਰ_2) { m_ਭਾਰ = std::max(m_ਭਾਰ, ਭਾਰ_2); }
    CgBin& ਲੱਭੋ_New_Bin(ਅਚਲ string& ਪਛਾਣ_ਤੱਤ) { ਵਾਪਸ m_bins[ਪਛਾਣ_ਤੱਤ]; }
};

// A covergroup
ਵਰਗ Cg_ਸਮੂਹ final {
    // MEMBERS
    string m_hier;  // Hierarchy above the items
    uint64_t m_ਭਾਰ = 0;  // type_option.weight, the largest of the records
    std::map<string, Cg_ਵਸਤੂ> m_ਚੀਜ਼ਾਂ;  // By hierarchy

ਜਨਤਕ:
    // ACCESSORS
    ਅਚਲ string& hier() ਅਚਲ { ਵਾਪਸ m_hier; }
    uint64_t ਭਾਰ_2() ਅਚਲ { ਵਾਪਸ m_ਭਾਰ; }
    ਅਚਲ std::map<string, Cg_ਵਸਤੂ>& ਚੀਜ਼ਾਂ() ਅਚਲ { ਵਾਪਸ m_ਚੀਜ਼ਾਂ; }
    // METHODS
    ਖਾਲੀ ਜੋੜੋ_ਰਿਕਾਰਡ(ਅਚਲ string& hier, uint64_t ਭਾਰ_2) {
        m_hier = hier;
        m_ਭਾਰ = std::max(m_ਭਾਰ, ਭਾਰ_2);
    }
    Cg_ਵਸਤੂ& ਲੱਭੋ_New_ਵਸਤੂ(ਅਚਲ string& hier) { ਵਾਪਸ m_ਚੀਜ਼ਾਂ[hier]; }
};
ਵਰਤੋਂ CgGroups = std::map<string, Cg_ਸਮੂਹ>;  // By page

// A number of a record, or 1 if the record leaves it out
uint64_t ਕੁੰਜੀ_ਸੰਖਿਆ(ਅਚਲ string& ਮੁੱਲ) {
    ਵਾਪਸ ਮੁੱਲ.ਖਾਲੀ_ਥਾਵਾਂ() ? 1 : std::strtoull(ਮੁੱਲ.c_str(), ਖਾਲੀ_ਸੂਚਕ, 10);
}

// If a field of a record's name, '\001<key>\002<value>', has a key of the coverage computation
ਤਾਰਕਿਕ is_Score_ਖੇਤਰ(ਅਚਲ string& ਖੇਤਰ) {
    ਲਈ (ਅਚਲ ਅੱਖਰ* ਅਚਲ keyp : {VL_CIK_THRESH, VL_CIK_ਭਾਰ, VL_CIK_ਸਮੂਹ_ਭਾਰ}) {
        ਅਚਲ string ਅਗੇਤਰ = string{"\001"} + keyp + "\002";
        ਜੇ (ਖੇਤਰ.compare(0, ਅਗੇਤਰ.ਮਾਪ(), ਅਗੇਤਰ) == 0) ਵਾਪਸ ਸੱਚ;
    }
    ਵਾਪਸ ਝੂਠ;
}

// The name of a bin's record without the keys of the coverage computation, which the records of
// a bin may differ in, so identifying the bin.  Its name does not: covergroups of distinct scopes
// may share a name.
string bin_ਪਛਾਣ_ਤੱਤ(ਅਚਲ string& ਰਿਕਾਰਡ_Name) {
    string ਪਛਾਣ_ਤੱਤ;
    string::ਮਾਪ_type ਸ਼ੁਰੂ = 0;
    ਜਦੋਂ (ਸ਼ੁਰੂ < ਰਿਕਾਰਡ_Name.ਮਾਪ()) {
        string::ਮਾਪ_type ਸਿਰਾ = ਰਿਕਾਰਡ_Name.ਲੱਭੋ('\001', ਸ਼ੁਰੂ + 1);
        ਜੇ (ਸਿਰਾ == string::npos) ਸਿਰਾ = ਰਿਕਾਰਡ_Name.ਮਾਪ();
        ਅਚਲ string ਖੇਤਰ = ਰਿਕਾਰਡ_Name.substr(ਸ਼ੁਰੂ, ਸਿਰਾ - ਸ਼ੁਰੂ);
        ਜੇ (!is_Score_ਖੇਤਰ(ਖੇਤਰ)) ਪਛਾਣ_ਤੱਤ += ਖੇਤਰ;
        ਸ਼ੁਰੂ = ਸਿਰਾ;
    }
    ਵਾਪਸ ਪਛਾਣ_ਤੱਤ;
}

ਤਾਰਕਿਕ isCovergroup(ਅਚਲ Vlc_ਬਿੰਦੂ& ਬਿੰਦੂ) { ਵਾਪਸ ਬਿੰਦੂ.type() == s_covergroupType; }

CgGroups covergroups(VlcPoints& points) {
    CgGroups groups;
    ਲਈ (ਅਚਲ VlcPoints::ByName::ਮੁੱਲ_type& i : points) {
        ਅਚਲ Vlc_ਬਿੰਦੂ& pt = points.ਬਿੰਦੂ_ਸੰਖਿਆ(i.ਦੂਜਾ);
        ਜੇ (!isCovergroup(pt)) ਜਾਰੀ;
        // Named '<covergroup>.<item>.<bin>', where only the bin, of its own key, may have dots
        ਅਚਲ string hier = pt.hier();
        string bin = pt.bin();
        ਜੇ (bin.ਖਾਲੀ_ਥਾਵਾਂ() || bin.ਮਾਪ() >= hier.ਮਾਪ()) bin = hier.substr(hier.rfind('.') + 1);
        ਅਚਲ string ਵਸਤੂ_Hier
            = hier.substr(0, hier.ਮਾਪ() - std::min(hier.ਮਾਪ(), bin.ਮਾਪ() + 1));
        Cg_ਸਮੂਹ& ਸਮੂਹ = groups[pt.ਪੰਨਾ()];
        ਸਮੂਹ.ਜੋੜੋ_ਰਿਕਾਰਡ(ਵਸਤੂ_Hier.substr(0, ਵਸਤੂ_Hier.rfind('.')), ਕੁੰਜੀ_ਸੰਖਿਆ(pt.ਸਮੂਹ_ਭਾਰ()));
        Cg_ਵਸਤੂ& ਵਸਤੂ_3 = ਸਮੂਹ.ਲੱਭੋ_New_ਵਸਤੂ(ਵਸਤੂ_Hier);
        ਵਸਤੂ_3.ਜੋੜੋ_ਰਿਕਾਰਡ(ਕੁੰਜੀ_ਸੰਖਿਆ(pt.ਭਾਰ_2()));
        ਜੇ (!pt.binType().ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;  // Not coverable
        ਵਸਤੂ_3.ਲੱਭੋ_New_Bin(bin_ਪਛਾਣ_ਤੱਤ(pt.name())).ਜੋੜੋ_ਰਿਕਾਰਡ(bin, pt.ਗਿਣਤੀ_2(), ਕੁੰਜੀ_ਸੰਖਿਆ(pt.thresh()));
    }
    ਵਾਪਸ groups;
}

Tally ਵਸਤੂ_Tally(ਅਚਲ Cg_ਵਸਤੂ& ਵਸਤੂ_3) {
    Tally tally;
    ਲਈ (ਅਚਲ std::pair<ਅਚਲ string, CgBin>& bin : ਵਸਤੂ_3.bins()) {
        tally.ਜੋੜੋ_ਬਿੰਦੂ(bin.ਦੂਜਾ.covered());
    }
    // Without bins, 0.0, or 100.0 if of zero weight (IEEE 1800-2023 19.11.1)
    tally.score(tally.ਕੁੱਲ() ? pct(tally.ਪ੍ਰਾਪਤੀ(), tally.ਕੁੱਲ()) : ਵਸਤੂ_3.ਭਾਰ_2() ? 0.0 : 100.0);
    ਵਾਪਸ tally;
}

// The tally of a covergroup; contributesp, if its items have weight and bins, so that it counts
// in the coverage of covergroups together
Tally ਸਮੂਹ_Tally(ਅਚਲ Cg_ਸਮੂਹ& ਸਮੂਹ, ਤਾਰਕਿਕ* contributesp = ਖਾਲੀ_ਸੂਚਕ) {
    Tally tally;
    ਦੁੱਗਣਾ weighted = 0.0;
    ਦੁੱਗਣਾ weights = 0.0;
    ਲਈ (ਅਚਲ std::pair<ਅਚਲ string, Cg_ਵਸਤੂ>& it : ਸਮੂਹ.ਚੀਜ਼ਾਂ()) {
        ਅਚਲ Tally ਵਸਤੂ_3 = ਵਸਤੂ_Tally(it.ਦੂਜਾ);
        tally.ਜੋੜੋ_Points(ਵਸਤੂ_3);
        ਜੇ (!ਵਸਤੂ_3.ਕੁੱਲ()) ਜਾਰੀ;  // Excluded from both sums
        weighted += static_cast<ਦੁੱਗਣਾ>(it.ਦੂਜਾ.ਭਾਰ_2()) * ਵਸਤੂ_3.score();
        weights += static_cast<ਦੁੱਗਣਾ>(it.ਦੂਜਾ.ਭਾਰ_2());
    }
    ਜੇ (contributesp) *contributesp = weights != 0.0;
    // Without items of weight and bins, 0.0, or 100.0 if of zero weight
    tally.score(weights != 0.0 ? weighted / weights : ਸਮੂਹ.ਭਾਰ_2() ? 0.0 : 100.0);
    ਵਾਪਸ tally;
}

Tally groupsTally(ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Cg_ਸਮੂਹ*>& groups) {
    Tally tally;
    ਦੁੱਗਣਾ weighted = 0.0;
    ਦੁੱਗਣਾ weights = 0.0;
    ਤਾਰਕਿਕ any_ਭਾਰ = ਝੂਠ;
    ਲਈ (ਅਚਲ Cg_ਸਮੂਹ* ਅਚਲ groupp : groups) {
        ਤਾਰਕਿਕ contributes = ਝੂਠ;
        ਅਚਲ Tally ਸਮੂਹ = ਸਮੂਹ_Tally(*groupp, &contributes);
        tally.ਜੋੜੋ_Points(ਸਮੂਹ);
        ਜੇ (groupp->ਭਾਰ_2()) any_ਭਾਰ = ਸੱਚ;
        ਜੇ (!contributes) ਜਾਰੀ;
        weighted += static_cast<ਦੁੱਗਣਾ>(groupp->ਭਾਰ_2()) * ਸਮੂਹ.score();
        weights += static_cast<ਦੁੱਗਣਾ>(groupp->ਭਾਰ_2());
    }
    // Without covergroups of weight that contribute, 0.0, or 100.0 if all have zero weight
    tally.score(weights != 0.0 ? weighted / weights : any_ਭਾਰ ? 0.0 : 100.0);
    ਵਾਪਸ tally;
}

// The tallies of covergroups, of their coverpoints and crosses, and of the coverable bins of
// those, by name; a covergroup of a dotted name also tallies in each node of its name
std::map<string, Tally> covergroupTallies(ਅਚਲ CgGroups& groups) {
    std::map<string, Tally> tallies;
    std::map<string, std::ਸਦਿਸ਼<ਅਚਲ Cg_ਸਮੂਹ*>> ਗੰਢ_Groups;
    ਲਈ (ਅਚਲ CgGroups::ਮੁੱਲ_type& it : groups) {
        ਅਚਲ Cg_ਸਮੂਹ& ਸਮੂਹ = it.ਦੂਜਾ;
        string ਰਸਤਾ;
        ਲਈ (ਅਚਲ string& part : ਵੰਡੋ_Hier(ਸਮੂਹ.hier())) {
            ਰਸਤਾ = ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ() ? part : ਰਸਤਾ + "." + part;
            ਗੰਢ_Groups[ਰਸਤਾ].ਧੱਕੋ_ਪਿੱਠ(&ਸਮੂਹ);
        }
        ਲਈ (ਅਚਲ std::pair<ਅਚਲ string, Cg_ਵਸਤੂ>& ਵਸਤੂ_3 : ਸਮੂਹ.ਚੀਜ਼ਾਂ()) {
            tallies[ਵਸਤੂ_3.ਪਹਿਲਾ] = ਵਸਤੂ_Tally(ਵਸਤੂ_3.ਦੂਜਾ);
            ਲਈ (ਅਚਲ std::pair<ਅਚਲ string, CgBin>& bin : ਵਸਤੂ_3.ਦੂਜਾ.bins()) {
                // Of the bins of the name
                tallies[ਵਸਤੂ_3.ਪਹਿਲਾ + "." + bin.ਦੂਜਾ.name()].ਜੋੜੋ_ਬਿੰਦੂ(bin.ਦੂਜਾ.covered());
            }
        }
    }
    ਲਈ (ਅਚਲ std::pair<ਅਚਲ string, std::ਸਦਿਸ਼<ਅਚਲ Cg_ਸਮੂਹ*>>& it : ਗੰਢ_Groups) {
        tallies[it.ਪਹਿਲਾ]
            = it.ਦੂਜਾ.ਮਾਪ() == 1 ? ਸਮੂਹ_Tally(*it.ਦੂਜਾ.front()) : groupsTally(it.ਦੂਜਾ);
    }
    ਵਾਪਸ tallies;
}

// Shared row formatter.  The callers choose which rows to print; this only keeps
// the text layout identical between the flat and hierarchy reports.
ਖਾਲੀ printIndent(ਮਾਪ_t indent) {
    ਲਈ (ਮਾਪ_t i = 0; i < indent; ++i) std::ਬਾਹਰ << ' ';
}

ਖਾਲੀ print_Tally_ਪੰਕਤੀ(ਅਚਲ string& type, ਅਚਲ Tally& tally, ਮਾਪ_t indent, ਮਾਪ_t type_ਚੌੜਾਈ,
                   ਮਾਪ_t ਗਿਣਤੀ_ਚੌੜਾਈ) {
    printIndent(indent);
    // A score is complete at 100%, which it may reach with bins of no weight uncovered
    ਅਚਲ string percent = tally.scored() ? pct_ਮੁੱਲ_String(tally.score(), tally.score() >= 100.0)
                                          : pctString(tally.ਪ੍ਰਾਪਤੀ(), tally.ਕੁੱਲ());
    // Right-align percentages to the width of "100.0%", so that rows line up
    std::ਬਾਹਰ << std::ਖੱਬੇ << std::setw(type_ਚੌੜਾਈ) << type << " : " << std::ਸੱਜੇ << std::fixed
              << std::setw(6) << percent << " (" << std::setw(ਗਿਣਤੀ_ਚੌੜਾਈ) << tally.ਪ੍ਰਾਪਤੀ() << "/"
              << std::setw(ਗਿਣਤੀ_ਚੌੜਾਈ) << tally.ਕੁੱਲ() << ")\n";
}

ਮਾਪ_t ਗਿਣਤੀ_ਚੌੜਾਈ(ਅਚਲ TypeTally& tally) {
    ਮਾਪ_t ਚੌੜਾਈ = cvt_ਮੰਜ਼ਿਲ_Str(0).ਮਾਪ();
    ਲਈ (TypeTally::const_iterator it = tally.begin(); it != tally.ਸਿਰਾ(); ++it) {
        ਚੌੜਾਈ = std::max(ਚੌੜਾਈ, cvt_ਮੰਜ਼ਿਲ_Str(it->ਦੂਜਾ.ਪ੍ਰਾਪਤੀ()).ਮਾਪ());
        ਚੌੜਾਈ = std::max(ਚੌੜਾਈ, cvt_ਮੰਜ਼ਿਲ_Str(it->ਦੂਜਾ.ਕੁੱਲ()).ਮਾਪ());
    }
    ਵਾਪਸ ਚੌੜਾਈ;
}

ਮਾਪ_t type_ਚੌੜਾਈ(ਅਚਲ TypeTally& tally) {
    ਮਾਪ_t type_ਚੌੜਾਈ = 0;
    ਲਈ (ਅਚਲ ਅੱਖਰ* ਅਚਲ typep : s_orderedTypes) {
        ਅਚਲ string type = typep;
        type_ਚੌੜਾਈ = std::max(type_ਚੌੜਾਈ, type.ਮਾਪ());
    }
    ਲਈ (TypeTally::const_iterator it = tally.begin(); it != tally.ਸਿਰਾ(); ++it) {
        type_ਚੌੜਾਈ = std::max(type_ਚੌੜਾਈ, it->ਪਹਿਲਾ.ਮਾਪ());
    }
    ਵਾਪਸ type_ਚੌੜਾਈ;
}

ਖਾਲੀ printTypeTally(ਅਚਲ TypeTally& tally, ਮਾਪ_t indent, ਤਾਰਕਿਕ includeMissingOrdered) {
    // Print standard coverage types first for stable output.  When requested,
    // missing standard rows are printed with zero counts for compatibility with
    // the historical flat summary output.
    ਅਚਲ ਮਾਪ_t typ_ਚੌੜਾਈ = type_ਚੌੜਾਈ(tally);
    ਅਚਲ ਮਾਪ_t cnt_ਚੌੜਾਈ = ਗਿਣਤੀ_ਚੌੜਾਈ(tally);
    ਲਈ (ਅਚਲ ਅੱਖਰ* ਅਚਲ typep : s_orderedTypes) {
        ਅਚਲ string type = typep;
        ਅਚਲ TypeTally::const_iterator it = tally.ਲੱਭੋ(type);
        ਜੇ (it != tally.ਸਿਰਾ()) {
            print_Tally_ਪੰਕਤੀ(type, it->ਦੂਜਾ, indent, typ_ਚੌੜਾਈ, cnt_ਚੌੜਾਈ);
        } ਨਹੀਂਤਾਂ ਜੇ (includeMissingOrdered) {
            print_Tally_ਪੰਕਤੀ(type, Tally{}, indent, typ_ਚੌੜਾਈ, cnt_ਚੌੜਾਈ);
        }
    }
    ਲਈ (TypeTally::const_iterator it = tally.begin(); it != tally.ਸਿਰਾ(); ++it) {
        ਜੇ (!isOrderedType(it->ਪਹਿਲਾ)) {
            print_Tally_ਪੰਕਤੀ(it->ਪਹਿਲਾ, it->ਦੂਜਾ, indent, typ_ਚੌੜਾਈ, cnt_ਚੌੜਾਈ);
        }
    }
}

// Print covergroups, coverpoints, crosses, and bins one per line, so that searching for a name
// shows its coverage
ਖਾਲੀ printCovergroupTallies(ਅਚਲ std::map<string, Tally>& tallies, ਪੂਰਨ ਪੱਧਰ) {
    std::map<string, Tally> shown;
    ਮਾਪ_t name_ਚੌੜਾਈ = 0;
    ਲਈ (ਅਚਲ std::pair<ਅਚਲ string, Tally>& it : tallies) {
        ਜੇ (ਪੱਧਰ >= 0 && static_cast<ਪੂਰਨ>(ਵੰਡੋ_Hier(it.ਪਹਿਲਾ).ਮਾਪ()) > ਪੱਧਰ + 1) ਜਾਰੀ;
        shown.ਪਾਓ(it);
        name_ਚੌੜਾਈ = std::max(name_ਚੌੜਾਈ, it.ਪਹਿਲਾ.ਮਾਪ());
    }
    ਅਚਲ ਮਾਪ_t cnt_ਚੌੜਾਈ = ਗਿਣਤੀ_ਚੌੜਾਈ(shown);
    std::ਬਾਹਰ << "Covergroup Coverage Summary:\n";
    ਲਈ (ਅਚਲ std::pair<ਅਚਲ string, Tally>& it : shown) {
        print_Tally_ਪੰਕਤੀ(it.ਪਹਿਲਾ, it.ਦੂਜਾ, s_summaryIndent, name_ਚੌੜਾਈ, cnt_ਚੌੜਾਈ);
    }
}

}  // namespace

ਖਾਲੀ Vlc_ਸਿਖਰ::readCoverage(ਅਚਲ string& filename, ਤਾਰਕਿਕ nonfatal) {
    UINFO(2, "readCoverage " << filename);

    std::ifstream is{filename.c_str()};
    ਜੇ (!is) {
        ਜੇ (!nonfatal) v3fatal("Can't read coverage file: " << filename);
        ਵਾਪਸ;
    }

    // Testrun and computrons argument unsupported as yet
    VlcTest* ਅਚਲ testp = tests().newTest(filename, 0, 0);

    uint64_t lineno = 0;
    ਜਦੋਂ (!is.eof()) {
        ਅਚਲ string ਸਤਰ = V3Os::getline(is);
        ++lineno;
        // UINFO(9, " got " << line);
        ਜੇ (ਸਤਰ[0] == 'C') {
            // The count follows the last "' ": a point may hold one too, as does a covergroup
            // type named with the value of a string parameter
            ਅਚਲ string::ਮਾਪ_type secspace = ਸਤਰ.rfind("' ");
            ਜੇ (secspace == string::npos || secspace < 3) {
                v3error("Malformed coverage point, without a count: " << filename << ":"
                                                                      << lineno);
                ਜਾਰੀ;
            }
            ਅਚਲ string ਬਿੰਦੂ = ਸਤਰ.substr(3, secspace - 3);
            ਜੇ (!opt.is_Type_ਮੇਲ(ਬਿੰਦੂ.c_str())) ਜਾਰੀ;

            ਅਚਲ uint64_t ਪ੍ਰਾਪਤੀਆਂ = std::atoll(ਸਤਰ.c_str() + secspace + 1);
            // UINFO(9, "   point '" << point << "'" << " " << hits);

            ਅਚਲ uint64_t pointnum = points().ਲੱਭੋ_ਜੋੜੋ_ਬਿੰਦੂ(ਬਿੰਦੂ, ਪ੍ਰਾਪਤੀਆਂ);
            ਜੇ (opt.rank()) {  // Only if ranking - uses a lot of memory
                // A test covers a point with a threshold (option.at_least) when its own hits
                // reach it
                ਅਚਲ uint64_t needed = std::max(
                    VlcBuckets::sufficient(), ਕੁੰਜੀ_ਸੰਖਿਆ(points().ਬਿੰਦੂ_ਸੰਖਿਆ(pointnum).thresh()));
                ਜੇ (ਪ੍ਰਾਪਤੀਆਂ >= needed) {
                    points().ਬਿੰਦੂ_ਸੰਖਿਆ(pointnum).testsCoveringInc();
                    testp->buckets().ਜੋੜੋ_ਅੰਕੜੇ(pointnum, ਪ੍ਰਾਪਤੀਆਂ);
                }
            }
        }
    }
}

ਖਾਲੀ Vlc_ਸਿਖਰ::writeCoverage(ਅਚਲ string& filename) {
    UINFO(2, "writeCoverage " << filename);

    std::ofstream os{filename.c_str()};
    ਜੇ (!os) {
        v3fatal("Can't write file: " << filename);
        ਵਾਪਸ;
    }

    os << "# SystemC::Coverage-3\n";
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : m_points) {
        ਅਚਲ Vlc_ਬਿੰਦੂ& ਬਿੰਦੂ = m_points.ਬਿੰਦੂ_ਸੰਖਿਆ(i.ਦੂਜਾ);
        os << "C '" << ਬਿੰਦੂ.name() << "' " << ਬਿੰਦੂ.ਗਿਣਤੀ_2() << '\n';
    }
}

ਖਾਲੀ Vlc_ਸਿਖਰ::writeInfo(ਅਚਲ string& filename) {
    UINFO(2, "writeInfo " << filename);

    std::ofstream os{filename.c_str()};
    ਜੇ (!os) {
        v3fatal("Can't write file: " << filename);
        ਵਾਪਸ;
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
    ਲਈ (ਸਵੈਚਾਲਿਤ& si : m_sources) {
        Vlc_ਸਰੋਤ& ਸਰੋਤ = si.ਦੂਜਾ;
        os << "SF:" << ਸਰੋਤ.name() << '\n';
        Vlc_ਸਰੋਤ::LinenoMap& ਸਤਰਾਂ = ਸਰੋਤ.ਸਤਰਾਂ();
        ਪੂਰਨ branches_ਮਿਲਿਆ = 0;
        ਪੂਰਨ branches_ਪ੍ਰਾਪਤੀ = 0;
        ਲਈ (ਸਵੈਚਾਲਿਤ& li : ਸਤਰਾਂ) {
            Vlc_ਸਰੋਤ_ਗਿਣਤੀ& sc = li.ਦੂਜਾ;
            uint64_t da_ਗਿਣਤੀ = 0;
            std::ਸਦਿਸ਼<ਅਚਲ Vlc_ਬਿੰਦੂ*> infoPoints;
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਬਿੰਦੂ : sc.points()) {
                da_ਗਿਣਤੀ = std::max(da_ਗਿਣਤੀ, ਬਿੰਦੂ->ਗਿਣਤੀ_2());
                infoPoints.ਧੱਕੋ_ਪਿੱਠ(ਬਿੰਦੂ);
            }
            os << "DA:" << sc.lineno() << "," << da_ਗਿਣਤੀ << "\n";
            ਜੇ (infoPoints.ਮਾਪ() <= 1) ਜਾਰੀ;
            branches_ਮਿਲਿਆ += static_cast<ਪੂਰਨ>(infoPoints.ਮਾਪ());
            ਪੂਰਨ ਬਿੰਦੂ_ਅੰਕ_2 = 0;
            ਲਈ (ਅਚਲ Vlc_ਬਿੰਦੂ* ਬਿੰਦੂ : infoPoints) {
                os << "BRDA:" << sc.lineno() << ",";
                ਜੇ (ਬਿੰਦੂ->isFsmArc()) {
                    os << "2,";
                    os << ਬਿੰਦੂ->fsmFromState() << "->" << ਬਿੰਦੂ->fsm_ਮੰਜ਼ਿਲ_State();
                } ਨਹੀਂਤਾਂ ਜੇ (ਬਿੰਦੂ->comment().ਖਾਲੀ_ਥਾਵਾਂ()) {
                    os << "0,";
                    os << ਬਿੰਦੂ_ਅੰਕ_2;
                } ਨਹੀਂਤਾਂ {
                    os << (ਬਿੰਦੂ->isFsmState() ? '1' : '0') << ',';
                    std::string comment(ਬਿੰਦੂ->comment());
                    std::replace(comment.begin(), comment.ਸਿਰਾ(), ',', '_');
                    os << comment;
                }
                os << "," << ਬਿੰਦੂ->ਗਿਣਤੀ_2() << "\n";

                branches_ਪ੍ਰਾਪਤੀ += opt.ਗਿਣਤੀ_Ok(ਬਿੰਦੂ->ਗਿਣਤੀ_2());
                ++ਬਿੰਦੂ_ਅੰਕ_2;
            }
        }
        os << "BRF:" << branches_ਮਿਲਿਆ << '\n';
        os << "BRH:" << branches_ਪ੍ਰਾਪਤੀ << '\n';

        os << "end_of_record\n";
    }
}

//********************************************************************

ਬਣਤਰ CmpComputrons final {
    ਤਾਰਕਿਕ ਸੰਚਾਲਕ()(ਅਚਲ VlcTest* lhsp, ਅਚਲ VlcTest* rhsp) ਅਚਲ {
        ਜੇ (lhsp->computrons() != rhsp->computrons()) {
            ਵਾਪਸ lhsp->computrons() < rhsp->computrons();
        }
        ਵਾਪਸ lhsp->bucketsCovered() > rhsp->bucketsCovered();
    }
};

ਖਾਲੀ Vlc_ਸਿਖਰ::rank() {
    UINFO(2, "rank...");
    uint64_t nextrank = 1;

    // Sort by computrons, so fast tests get selected first
    std::ਸਦਿਸ਼<VlcTest*> bytime;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& testp : m_tests) {
        ਜੇ (testp->bucketsCovered()) {  // else no points, so can't help us
            bytime.ਧੱਕੋ_ਪਿੱਠ(testp);
        }
    }
    ਛਾਂਟੋ(bytime.begin(), bytime.ਸਿਰਾ(), CmpComputrons());  // Sort the vector

    VlcBuckets ਬਚਿਆ;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : m_points) {
        ਅਚਲ Vlc_ਬਿੰਦੂ* ਅਚਲ pointp = &points().ਬਿੰਦੂ_ਸੰਖਿਆ(i.ਦੂਜਾ);
        // If any tests hit this point, then we'll need to cover it.
        ਜੇ (pointp->testsCovering()) ਬਚਿਆ.ਜੋੜੋ_ਅੰਕੜੇ(pointp->ਬਿੰਦੂ_ਅੰਕ(), 1);
    }

    // Additional Greedy algorithm
    // O(n^2) Ouch.  Probably the thing to do is randomize the order of data
    // then hierarchically solve a small subset of tests, and take resulting
    // solution and move up to larger subset of tests.  (Aka quick sort.)
    ਜਦੋਂ (ਸੱਚ) {
        ਜੇ (debug() >= 9) {
            UINFO_ਅਗੇਤਰ("Left on iter" << nextrank << ": ");  // LCOV_EXCL_LINE
            ਬਚਿਆ.dump();  // LCOV_EXCL_LINE
        }
        VlcTest* ਉੱਤਮ_Testp = ਖਾਲੀ_ਸੂਚਕ;
        uint64_t ਉੱਤਮ_Remain = 0;
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& testp : bytime) {
            ਜੇ (!testp->rank()) {
                ਅਚਲ uint64_t remain = testp->buckets().ਅੰਕੜੇ_ਕੱਢੋ_ਗਿਣਤੀ(ਬਚਿਆ);
                ਜੇ (remain > ਉੱਤਮ_Remain) {
                    ਉੱਤਮ_Testp = testp;
                    ਉੱਤਮ_Remain = remain;
                }
            }
        }
        ਜੇ (VlcTest* ਅਚਲ testp = ਉੱਤਮ_Testp) {
            testp->rank(nextrank++);
            testp->rankPoints(ਉੱਤਮ_Remain);
            ਬਚਿਆ.or_ਅੰਕੜੇ(ਉੱਤਮ_Testp->buckets());
        } ਨਹੀਂਤਾਂ {
            ਰੋਕੋ;  // No test covering more stuff found
        }
    }
}

ਖਾਲੀ Vlc_ਸਿਖਰ::annotateCalc() {
    // Calculate per-line information into filedata structure
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : m_points) {
        ਅਚਲ Vlc_ਬਿੰਦੂ& ਬਿੰਦੂ = m_points.ਬਿੰਦੂ_ਸੰਖਿਆ(i.ਦੂਜਾ);
        ਅਚਲ string filename = ਬਿੰਦੂ.filename();
        ਅਚਲ ਪੂਰਨ lineno = ਬਿੰਦੂ.lineno();
        ਜੇ (!filename.ਖਾਲੀ_ਥਾਵਾਂ() && lineno != 0) {
            Vlc_ਸਰੋਤ& ਸਰੋਤ = sources().ਲੱਭੋ_New_ਸਰੋਤ(filename);
            UINFO(9, "AnnoCalc count " << filename << ":" << lineno << ":" << ਬਿੰਦੂ.ਕਾਲਮ_2() << " "
                                       << ਬਿੰਦੂ.ਗਿਣਤੀ_2() << " " << ਬਿੰਦੂ.linescov());
            // Base coverage
            ਸਰੋਤ.ਪਾਓ_ਬਿੰਦੂ(lineno, &ਬਿੰਦੂ);
            // Additional lines covered by this statement
            ਤਾਰਕਿਕ range = ਝੂਠ;
            ਪੂਰਨ ਸ਼ੁਰੂ = 0;
            ਪੂਰਨ ਸਿਰਾ = 0;
            ਅਚਲ string linescov = ਬਿੰਦੂ.linescov();
            ਲਈ (ਅਚਲ ਅੱਖਰ* covp = linescov.c_str(); ਸੱਚ; ++covp) {
                ਜੇ (!*covp || *covp == ',') {  // Ending
                    ਲਈ (ਪੂਰਨ lni = ਸ਼ੁਰੂ; ਸ਼ੁਰੂ && lni <= ਸਿਰਾ; ++lni) {
                        ਸਰੋਤ.ਪਾਓ_ਬਿੰਦੂ(lni, &ਬਿੰਦੂ);
                    }
                    ਜੇ (!*covp) ਰੋਕੋ;
                    ਸ਼ੁਰੂ = 0;  // Prep for next
                    ਸਿਰਾ = 0;
                    range = ਝੂਠ;
                } ਨਹੀਂਤਾਂ ਜੇ (*covp == '-') {
                    range = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (std::ਅੰਕ_ਹੈ(*covp)) {
                    ਅਚਲ ਅੱਖਰ* ਅਚਲ digitsp = covp;
                    ਜਦੋਂ (std::ਅੰਕ_ਹੈ(*covp)) ++covp;
                    --covp;  // Will inc in for loop
                    ਜੇ (!range) ਸ਼ੁਰੂ = std::atoi(digitsp);
                    ਸਿਰਾ = std::atoi(digitsp);
                }
            }
        }
    }
}

ਖਾਲੀ Vlc_ਸਿਖਰ::annotateCalcNeeded() {
    // Compute which files are needed.  A file isn't needed if it has appropriate
    // coverage in all categories
    ਪੂਰਨ totCases = 0;
    ਪੂਰਨ totOk = 0;
    ਲਈ (ਸਵੈਚਾਲਿਤ& si : m_sources) {
        Vlc_ਸਰੋਤ& ਸਰੋਤ = si.ਦੂਜਾ;
        // UINFO(1, "Source " << source.name());
        ਜੇ (opt.annotateAll()) ਸਰੋਤ.needed(ਸੱਚ);
        ਅਚਲ Vlc_ਸਰੋਤ::LinenoMap& ਸਤਰਾਂ = ਸਰੋਤ.ਸਤਰਾਂ();
        ਲਈ (ਸਵੈਚਾਲਿਤ& li : ਸਤਰਾਂ) {
            ਅਚਲ Vlc_ਸਰੋਤ_ਗਿਣਤੀ& sc = li.ਦੂਜਾ;
            // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" << sc.column());
            ++totCases;
            ਜੇ (opt.ਗਿਣਤੀ_Ok(sc.min_ਗਿਣਤੀ())) {
                ++totOk;
            } ਨਹੀਂਤਾਂ {
                ਸਰੋਤ.needed(ਸੱਚ);
            }
        }
    }
    std::ਬਾਹਰ << "Annotation Summary:\n";
    std::ਬਾਹਰ << "  lines with all attached points covered : ";
    std::ਬਾਹਰ << pctString(totOk, totCases) << "  (" << totOk << "/" << totCases << ")\n";
    ਜੇ (totOk != totCases) ਬਾਹਰ << "See lines with '%00' in " << opt.annotate_ਬਾਹਰਲਾ() << '\n';
}

ਖਾਲੀ Vlc_ਸਿਖਰ::annotateOutputFiles(ਅਚਲ string& dirname) {
    // Create if uncreated, ignore errors
    V3Os::createDir(dirname);
    ਲਈ (ਸਵੈਚਾਲਿਤ& si : m_sources) {
        Vlc_ਸਰੋਤ& ਸਰੋਤ = si.ਦੂਜਾ;
        ਜੇ (!ਸਰੋਤ.needed()) ਜਾਰੀ;
        ਅਚਲ string filename = ਸਰੋਤ.name();
        ਅਚਲ string outfilename = dirname + "/" + V3Os::filenameNonDir(filename);

        UINFO(1, "annotateOutputFile " << filename << " -> " << outfilename);

        std::ifstream is{filename.c_str()};
        ਜੇ (!is) {
            v3error("Can't read annotation file: " << filename);
            ਵਾਪਸ;
        }

        std::ofstream os{outfilename.c_str()};
        ਜੇ (!os) {
            v3error("Can't write file: " << outfilename);
            ਵਾਪਸ;
        }

        os << "//      // verilator_coverage annotation\n";

        ਪੂਰਨ lineno = 0;
        ਜਦੋਂ (!is.eof()) {
            lineno++;
            ਅਚਲ std::string ਸਤਰ = V3Os::getline(is);

            Vlc_ਸਰੋਤ::LinenoMap& ਸਤਰਾਂ = ਸਰੋਤ.ਸਤਰਾਂ();
            ਅਚਲ ਸਵੈਚਾਲਿਤ lit = ਸਤਰਾਂ.ਲੱਭੋ(lineno);
            ਜੇ (lit == ਸਤਰਾਂ.ਸਿਰਾ()) {
                os << "        " << ਸਤਰ << '\n';
            } ਨਹੀਂਤਾਂ {
                Vlc_ਸਰੋਤ_ਗਿਣਤੀ& sc = lit->ਦੂਜਾ;
                // UINFO(0, "Source " << source.name() << ":" << sc.lineno() << ":" <<
                // sc.column());
                ਅਚਲ ਤਾਰਕਿਕ minOk = opt.ਗਿਣਤੀ_Ok(sc.min_ਗਿਣਤੀ());
                ਅਚਲ ਤਾਰਕਿਕ maxOk = opt.ਗਿਣਤੀ_Ok(sc.max_ਗਿਣਤੀ());
                ਜੇ (minOk) {
                    os << " ";
                } ਨਹੀਂਤਾਂ ਜੇ (maxOk) {
                    os << "~";
                } ਨਹੀਂਤਾਂ {
                    os << "%";
                }
                os << std::setfill('0') << std::setw(6) << sc.max_ਗਿਣਤੀ() << " " << ਸਤਰ << '\n';

                ਜੇ (opt.annotatePoints()) {
                    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pit : sc.points()) pit->dumpAnnotate(os, opt.annotateMin());
                }
                ਤਾਰਕਿਕ printed_Fsm_ਸਿਰਲੇਖ = ਝੂਠ;
                ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pit : sc.points()) {
                    ਜੇ (!pit->isFsmState() && !pit->isFsmArc()) ਜਾਰੀ;
                    ਜੇ (!printed_Fsm_ਸਿਰਲੇਖ) {
                        os << "        // [FSM coverage]\n";
                        printed_Fsm_ਸਿਰਲੇਖ = ਸੱਚ;
                    }
                    os << (opt.ਗਿਣਤੀ_Ok(pit->ਗਿਣਤੀ_2()) ? " " : "%");
                    os << std::setfill('0') << std::setw(6) << pit->ਗਿਣਤੀ_2() << "        ";
                    ਜੇ (pit->isFsmState()) {
                        os << "// [fsm_state " << pit->comment() << "]";
                        ਜੇ (pit->ਗਿਣਤੀ_2() == 0) os << " *** UNCOVERED ***";
                        os << "\n";
                    } ਨਹੀਂਤਾਂ ਜੇ (pit->isFsmDefaultArc()) {
                        os << "// [SYNTHETIC DEFAULT ARC: " << pit->comment() << "]\n";
                    } ਨਹੀਂਤਾਂ {
                        os << "// [fsm_arc " << pit->comment() << "]";
                        ਜੇ (pit->fsmIsReset() && !opt.includeResetArcs()) {
                            os << " [reset arc, excluded from %]";
                        }
                        os << "\n";
                    }
                }
            }
        }
    }
}

ਖਾਲੀ Vlc_ਸਿਖਰ::annotate(ਅਚਲ string& dirname) {
    // Calculate per-line information into filedata structure
    annotateCalc();
    annotateCalcNeeded();
    annotateOutputFiles(dirname);
}

ਖਾਲੀ Vlc_ਸਿਖਰ::printTypeSummary() {
    TypeTally tally;
    ਲਈ (VlcPoints::ByName::ਮੁੱਲ_type& i : m_points) {
        ਅਚਲ Vlc_ਬਿੰਦੂ& pt = m_points.ਬਿੰਦੂ_ਸੰਖਿਆ(i.ਦੂਜਾ);
        ਜੇ (isCovergroup(pt)) ਜਾਰੀ;  // Tallied below, by covergroup
        tally_ਬਿੰਦੂ(tally, displayType(pt), pt.ਗਿਣਤੀ_2());
    }
    ਅਚਲ CgGroups groups = covergroups(m_points);
    ਜੇ (!groups.ਖਾਲੀ_ਥਾਵਾਂ()) {
        std::ਸਦਿਸ਼<ਅਚਲ Cg_ਸਮੂਹ*> groupps;
        ਲਈ (ਅਚਲ CgGroups::ਮੁੱਲ_type& it : groups) groupps.ਧੱਕੋ_ਪਿੱਠ(&it.ਦੂਜਾ);
        tally[s_covergroupType] = groupsTally(groupps);
    }
    ਜੇ (tally.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
    std::ਬਾਹਰ << "Coverage Summary:\n";
    // Keep the legacy summary behavior of showing standard coverage types even
    // when the input has no points of that type.
    printTypeTally(tally, s_summaryIndent, ਸੱਚ);
}

ਖਾਲੀ Vlc_ਸਿਖਰ::printHierarchyReport() {
    std::map<string, TypeTally> hierTallies;
    std::map<string, TypeTally> duTallies;
    ਤਾਰਕਿਕ hasHier = ਝੂਠ;
    ਤਾਰਕਿਕ hasCollapsedHier = ਝੂਠ;
    ਲਈ (VlcPoints::ByName::ਮੁੱਲ_type& i : m_points) {
        ਅਚਲ Vlc_ਬਿੰਦੂ& pt = m_points.ਬਿੰਦੂ_ਸੰਖਿਆ(i.ਦੂਜਾ);
        ਜੇ (isCovergroup(pt)) ਜਾਰੀ;  // Reported below, by covergroup
        ਅਚਲ string hier = reportHier(pt);
        ਜੇ (hier.ਖਾਲੀ_ਥਾਵਾਂ()) ਜਾਰੀ;
        hasHier = ਸੱਚ;
        ਜੇ (isCollapsedHier(hier)) hasCollapsedHier = ਸੱਚ;
        ਅਚਲ string type = displayType(pt);
        ਅਚਲ std::ਸਦਿਸ਼<string> parts = ਵੰਡੋ_Hier(hier);
        string ਰਸਤਾ;
        ਲਈ (std::ਸਦਿਸ਼<string>::const_iterator it = parts.begin(); it != parts.ਸਿਰਾ(); ++it) {
            ਰਸਤਾ = ਰਸਤਾ.ਖਾਲੀ_ਥਾਵਾਂ() ? *it : ਰਸਤਾ + "." + *it;
            tally_ਬਿੰਦੂ(hierTallies[ਰਸਤਾ], type, pt.ਗਿਣਤੀ_2());
        }
        tally_ਬਿੰਦੂ(duTallies[duName(pt)], type, pt.ਗਿਣਤੀ_2());
    }
    ਅਚਲ std::map<string, Tally> cgTallies = covergroupTallies(covergroups(m_points));

    ਜੇ (!hasHier && !cgTallies.ਖਾਲੀ_ਥਾਵਾਂ()) {
        printCovergroupTallies(cgTallies, opt.report_ਪੱਧਰ());
        ਵਾਪਸ;
    }
    ਜੇ (!hasHier) {
        std::ਬਾਹਰ << "%Warning: --report hierarchy input has no hierarchy fields; "
                  << "printing flat summary instead.\n";
        printTypeSummary();
        ਵਾਪਸ;
    }

    ਅਚਲ ਪੂਰਨ ਪੱਧਰ = opt.report_ਪੱਧਰ();
    ਜੇ (hasCollapsedHier) {
        std::ਬਾਹਰ << "Note: hierarchy report contains collapsed hierarchy paths; "
                  << "it is not precise per-instance coverage.\n";
    }
    std::ਬਾਹਰ << "Hierarchy Coverage Summary:\n";
    ਲਈ (std::map<string, TypeTally>::const_iterator it = hierTallies.begin();
         it != hierTallies.ਸਿਰਾ(); ++it) {
        ਅਚਲ std::ਸਦਿਸ਼<string> parts = ਵੰਡੋ_Hier(it->ਪਹਿਲਾ);
        ਜੇ (ਪੱਧਰ >= 0 && static_cast<ਪੂਰਨ>(parts.ਮਾਪ()) > ਪੱਧਰ + 1) ਜਾਰੀ;
        printIndent(s_summaryIndent);
        std::ਬਾਹਰ << it->ਪਹਿਲਾ << "\n";
        // Hierarchy nodes can be numerous, so only print coverage types present
        // under this node instead of repeating absent zero-count rows.
        printTypeTally(it->ਦੂਜਾ, s_report_ਪੰਕਤੀ_Indent, ਝੂਠ);
    }
    std::ਬਾਹਰ << "Design Unit Coverage Summary:\n";
    ਲਈ (std::map<string, TypeTally>::const_iterator it = duTallies.begin(); it != duTallies.ਸਿਰਾ();
         ++it) {
        printIndent(s_summaryIndent);
        std::ਬਾਹਰ << it->ਪਹਿਲਾ << "\n";
        // Design-unit summaries follow the hierarchy report style: present
        // types only, but in the same stable order as the flat summary.
        printTypeTally(it->ਦੂਜਾ, s_report_ਪੰਕਤੀ_Indent, ਝੂਠ);
    }
    ਜੇ (!cgTallies.ਖਾਲੀ_ਥਾਵਾਂ()) printCovergroupTallies(cgTallies, ਪੱਧਰ);
}
