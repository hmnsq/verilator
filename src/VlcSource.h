// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: verilator_coverage: Source file to annotate with line coverage
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

#ifndef VERILATOR_VLCSOURCE_H_
#define VERILATOR_VLCSOURCE_H_

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "VlcPoint.h"

#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_ଗଣନା final {
    // TYPES
    using PointsSet = std::set<ଅଚଳ VlcPoint*>;

    // MEMBERS
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lineno;  ///< Line number
    uint64_t m_max_ଗଣନା = 0;  ///< Max count
    uint64_t m_min_ଗଣନା = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit Vlc_Source_ଗଣନା(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_ଗଣନା() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno() ଅଚଳ { ଫେରନ୍ତୁ m_lineno; }
    uint64_t max_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_max_ଗଣନା; }
    uint64_t min_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_min_ଗଣନା; }

    // METHODS
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Point(ଅଚଳ VlcPoint* pointp) {
        m_max_ଗଣନା = std::max(m_max_ଗଣନା, pointp->ଗଣନା_2());
        m_min_ଗଣନା = std::min(m_min_ଗଣନା, pointp->ଗଣନା_2());
        m_points.emplace(pointp);
    }
    ଅଚଳ PointsSet& points() { ଫେରନ୍ତୁ m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
ସର୍ବସାଧାରଣ:
    // TYPES
    using LinenoMap = std::map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, Vlc_Source_ଗଣନା>;  // Map of {column}

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    ଅଚଳ string m_name;  //< Name of the source file
    LinenoMap m_ଧାଡ଼ିଗୁଡ଼ିକ;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit VlcSource(ଅଚଳ string& name)
        : m_name{name} {}
    ~VlcSource() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // ACCESSORS
    ଅଚଳ string& name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
    bool needed() ଅଚଳ { ଫେରନ୍ତୁ m_needed; }
    ଶୂନ୍ୟ needed(bool flag) { m_needed = flag; }
    LinenoMap& ଧାଡ଼ିଗୁଡ଼ିକ() { ଫେରନ୍ତୁ m_ଧାଡ଼ିଗୁଡ଼ିକ; }

    // METHODS
    ଶୂନ୍ୟ ଭର୍ତ୍ତି_Point(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno, ଅଚଳ VlcPoint* pointp) {
        Vlc_Source_ଗଣନା& sc = m_ଧାଡ଼ିଗୁଡ଼ିକ.emplace(lineno, lineno).first->second;
        sc.ଭର୍ତ୍ତି_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
ସର୍ବସାଧାରଣ:
    // TYPES
    using NameMap = std::map<ଅଚଳ std::string, VlcSource>;

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

ସର୍ବସାଧାରଣ:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { ଫେରନ୍ତୁ m_sources.begin(); }
    NameMap::iterator end() { ଫେରନ୍ତୁ m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~VlcSources() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // METHODS
    VlcSource& findNewSource(ଅଚଳ string& name) {
        NameMap::iterator iter = m_sources.find(name);
        ଯଦି (iter != m_sources.end()) ଫେରନ୍ତୁ iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        ଫେରନ୍ତୁ iter->second;
    }
};

//######################################################################

#endif  // Guard
