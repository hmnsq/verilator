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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "VlcPoint.h"

#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <utility>
#અન્તર્ભાવ <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_ગણતરી final {
    // TYPES
    using PointsSet = std::set<અચલ VlcPoint*>;

    // MEMBERS
    અચલ પૂર્ણાંક m_lineno;  ///< Line number
    uint64_t m_max_ગણતરી = 0;  ///< Max count
    uint64_t m_min_ગણતરી = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

જાહેર:
    // CONSTRUCTORS
    explicit Vlc_Source_ગણતરી(પૂર્ણાંક lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_ગણતરી() = મૂળભૂત;

    // ACCESSORS
    પૂર્ણાંક lineno() અચલ { પાછા m_lineno; }
    uint64_t max_ગણતરી() અચલ { પાછા m_max_ગણતરી; }
    uint64_t min_ગણતરી() અચલ { પાછા m_min_ગણતરી; }

    // METHODS
    શૂન્ય ઉમેરો_Point(અચલ VlcPoint* pointp) {
        m_max_ગણતરી = std::max(m_max_ગણતરી, pointp->ગણતરી());
        m_min_ગણતરી = std::min(m_min_ગણતરી, pointp->ગણતરી());
        m_points.emplace(pointp);
    }
    અચલ PointsSet& points() { પાછા m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
જાહેર:
    // TYPES
    using LinenoMap = std::map<પૂર્ણાંક, Vlc_Source_ગણતરી>;  // Map of {column}

ખાનગી:
    // MEMBERS
    અચલ string m_name;  //< Name of the source file
    LinenoMap m_લીટીઓ;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

જાહેર:
    // CONSTRUCTORS
    explicit VlcSource(અચલ string& name)
        : m_name{name} {}
    ~VlcSource() = મૂળભૂત;

    // ACCESSORS
    અચલ string& name() અચલ { પાછા m_name; }
    bool needed() અચલ { પાછા m_needed; }
    શૂન્ય needed(bool flag) { m_needed = flag; }
    LinenoMap& લીટીઓ() { પાછા m_લીટીઓ; }

    // METHODS
    શૂન્ય ઉમેરો_Point(પૂર્ણાંક lineno, અચલ VlcPoint* pointp) {
        Vlc_Source_ગણતરી& sc = m_લીટીઓ.emplace(lineno, lineno).first->second;
        sc.ઉમેરો_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
જાહેર:
    // TYPES
    using NameMap = std::map<અચલ std::string, VlcSource>;

ખાનગી:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

જાહેર:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { પાછા m_sources.begin(); }
    NameMap::iterator end() { પાછા m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = મૂળભૂત;
    ~VlcSources() = મૂળભૂત;

    // METHODS
    VlcSource& findNewSource(અચલ string& name) {
        NameMap::iterator iter = m_sources.find(name);
        જો (iter != m_sources.end()) પાછા iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        પાછા iter->second;
    }
};

//######################################################################

#endif  // Guard
