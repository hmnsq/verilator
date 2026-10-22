// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: Coverage points
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

#ifndef VERILATOR_VLCPOINT_H_
#define VERILATOR_VLCPOINT_H_

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <map>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif
#அந்தர்பாவ "verilated_cov_key.h"

#அந்தர்பாவ "V3Error.h"

//********************************************************************
// VlcPoint - A coverage point (across all tests)

class VlcPoint final {
    // MEMBERS
    string m_name;  //< Name of the point
    uint64_t m_point_இலக்கம்;  //< Point number
    uint64_t m_testsCovering = 0;  //< Number tests with non-zero coverage of this point
    uint64_t m_மொத்தம் = 0;  //< Count of hits across all tests

பொது:
    // CONSTRUCTORS
    VlcPoint(மாறிலி string& name, uint64_t point_இலக்கம்)
        : m_name{name}
        , m_point_இலக்கம்{point_இலக்கம்} {}
    ~VlcPoint() = இயல்பு;
    // ACCESSORS
    மாறிலி string& name() மாறிலி { திரும்பு m_name; }
    uint64_t point_இலக்கம்() மாறிலி { திரும்பு m_point_இலக்கம்; }
    uint64_t testsCovering() மாறிலி { திரும்பு m_testsCovering; }
    வெறுமை மொத்தம்_Inc(uint64_t inc) { m_மொத்தம் += inc; }
    uint64_t மொத்தம்() மாறிலி { திரும்பு m_மொத்தம்; }
    வெறுமை testsCoveringInc() { m_testsCovering++; }
    bool ok(unsigned annotateMin) மாறிலி {
        மாறிலி std::string threshStr = thresh();
        மாறிலி unsigned threshi = !threshStr.empty() ? std::atoi(threshStr.c_str()) : annotateMin;
        திரும்பு m_மொத்தம் >= threshi;
    }
    // KEY ACCESSORS
    string filename() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_FILENAME, m_name.c_str()); }
    string comment() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_COMMENT, m_name.c_str()); }
    string hier() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_HIER, m_name.c_str()); }
    string page() மாறிலி { திரும்பு சாவி_Extract("page", m_name.c_str()); }
    string type() மாறிலி { திரும்பு typeExtract(m_name.c_str()); }
    // Covergroup-specific key accessors
    string bin() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_BIN, m_name.c_str()); }
    string binType() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_BIN_TYPE, m_name.c_str()); }
    bool isCross() மாறிலி { திரும்பு !சாவி_Extract(VL_CIK_CROSS, m_name.c_str()).empty(); }
    string crossBins() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_CROSS_BINS, m_name.c_str()); }
    // The covergroup's type_option.weight; "" for the default 1
    string groupWeight() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_GROUP_WEIGHT, m_name.c_str()); }
    // The coverpoint's or cross's option.weight; "" for the default 1
    string weight() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_WEIGHT, m_name.c_str()); }
    string thresh() மாறிலி {
        // string as maybe ""
        திரும்பு சாவி_Extract(VL_CIK_THRESH, m_name.c_str());
    }
    string linescov() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_LINESCOV, m_name.c_str()); }
    bool isFsmState() மாறிலி { திரும்பு type() == "fsm_state"; }
    bool isFsmArc() மாறிலி { திரும்பு type() == "fsm_arc"; }
    // Arc-specific helpers are used after callers have already filtered to
    // FSM arc points, so they do not repeat the type check here.
    string fsmVarName() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_FSM_VAR, m_name.c_str()); }
    string fsmFromState() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_FSM_FROM, m_name.c_str()); }
    string fsmToState() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_FSM_TO, m_name.c_str()); }
    string fsmTag() மாறிலி { திரும்பு சாவி_Extract(VL_CIK_FSM_TAG, m_name.c_str()); }
    bool isFsmResetInclude() மாறிலி { திரும்பு fsmTag() == "reset_include"; }
    bool isFsmResetArc() மாறிலி { திரும்பு fsmTag() == "reset"; }
    bool isFsmDefaultArc() மாறிலி { திரும்பு fsmTag() == "default"; }
    bool fsmIsReset() மாறிலி { திரும்பு isFsmResetArc() || isFsmResetInclude(); }
    முழு lineno() மாறிலி {
        மாறிலி string வரி_Str = சாவி_Extract(VL_CIK_LINENO, m_name.c_str());
        திரும்பு std::atoi(வரி_Str.c_str());
    }
    முழு column() மாறிலி {
        மாறிலி string columnStr = சாவி_Extract(VL_CIK_COLUMN, m_name.c_str());
        திரும்பு std::atoi(columnStr.c_str());
    }
    // METHODS
    நிலையான string typeExtract(மாறிலி எழுத்து* name) { திரும்பு சாவி_Extract(VL_CIK_TYPE, name); }
    // The value of a key of a record, as the design writes it, so without the escapes of the
    // coverage file, VerilatedCovKey::escape(), of characters that print, see unescape(); only
    // the record's name() keeps them all, so that --write writes it back as read
    நிலையான string சாவி_Extract(மாறிலி எழுத்து* short_சாவி, மாறிலி எழுத்து* name) {
        // Hot function
        மாறிலி size_t shortLen = std::strlen(short_சாவி);
        க்கு (மாறிலி எழுத்து* cp = name; *cp; ++cp) {
            எனில் (*cp == '\001') {
                எனில் (0 == std::strncmp(cp + 1, short_சாவி, shortLen) && cp[shortLen + 1] == '\002') {
                    cp += shortLen + 2;  // Skip \001+short+\002
                    மாறிலி எழுத்து* ep = cp;
                    போது (*ep && *ep != '\001') ++ep;
                    திரும்பு Verilated_Cov_சாவி::unescape(string(cp, ep - cp));
                }
            }
        }
        திரும்பு "";
    }
    நிலையான வெறுமை dumpHeader(std::ostream& os) {
        os << "Points:\n";
        os << "  Num,    TestsCover,    Count,  Name\n";
    }
    வெறுமை dump(std::ostream& os) மாறிலி {
        os << "  " << std::setw(8) << std::setfill('0') << point_இலக்கம்();
        os << ",  " << std::setw(7) << std::setfill(' ') << testsCovering();
        os << ",  " << std::setw(7) << std::setfill(' ') << மொத்தம்();
        os << ",  \"" << name() << "\"\n";
    }
    வெறுமை dumpAnnotate(std::ostream& os, unsigned annotateMin) மாறிலி {
        os << (ok(annotateMin) ? "+" : "-");
        os << std::setw(6) << std::setfill('0') << மொத்தம்();
        os << "  point: type=" << type() << " comment=" << comment() << " hier=" << hier();
        os << "\n";
        எனில் (isCross()) {
            மாறிலி string bins = crossBins();
            எனில் (!bins.empty()) {
                os << "        //  cross: [";
                bool first = true;
                size_t start = 0;
                போது (true) {
                    மாறிலி size_t comma = bins.find(',', start);
                    எனில் (!first) os << ", ";
                    os << bins.substr(start, comma == string::npos ? string::npos : comma - start);
                    first = false;
                    எனில் (comma == string::npos) நிறுத்து;
                    start = comma + 1;
                }
                os << "]\n";
            }
        }
    }
};

//********************************************************************
// VlcPoints - Container of all points

class VlcPoints final {
    // MEMBERS
    using NameMap = std::map<மாறிலி std::string, uint64_t>;  // Sorted by name (ordered)
    NameMap m_nameMap;  //< Name to point-number
    std::vector<VlcPoint> m_points;  //< List of all points
    uint64_t m_இலக்கம்_Points = 0;  //< Total unique points

    நிலையான முழு debug() { திரும்பு V3Error::debugDefault(); }

பொது:
    // ITERATORS
    using ByName = NameMap;
    using iterator = ByName::iterator;
    ByName::iterator begin() { திரும்பு m_nameMap.begin(); }
    ByName::iterator end() { திரும்பு m_nameMap.end(); }

    // CONSTRUCTORS
    VlcPoints() = இயல்பு;
    ~VlcPoints() = இயல்பு;

    // METHODS
    வெறுமை dump() {
        UINFO(2, "dumpPoints...");
        VlcPoint::dumpHeader(std::cout);
        க்கு (மாறிலி auto& i : *this) {
            மாறிலி VlcPoint& point = point_எண்(i.second);
            point.dump(std::cout);
        }
    }
    VlcPoint& point_எண்(uint64_t இலக்கம்) { திரும்பு m_points[இலக்கம்]; }
    uint64_t findAddPoint(மாறிலி string& name, uint64_t மொத்தம்) {
        மாறிலி auto pair = m_nameMap.emplace(name, m_இலக்கம்_Points);
        எனில் (pair.second) m_points.emplace_back(name, m_இலக்கம்_Points++);
        மாறிலி uint64_t pointnum = pair.first->second;
        m_points[pointnum].மொத்தம்_Inc(மொத்தம்);
        திரும்பு pointnum;
    }
};

//######################################################################

#endif  // guard
