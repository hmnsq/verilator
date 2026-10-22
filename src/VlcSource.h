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

#నిర్వచితం_కాకపోతే VERILATOR_VLCSOURCE_H_
#నిర్వచించు VERILATOR_VLCSOURCE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "VlcPoint.h"

#అన్తర్భావ <limits>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

class Vlc_బిందువు;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_ఆధారం_లెక్క final {
    // TYPES
    using Points_సమితి = std::సమితి<స్థిరరాశి Vlc_బిందువు*>;

    // MEMBERS
    స్థిరరాశి పూర్ణాంకం m_lineno;  ///< Line number
    uint64_t m_max_లెక్క = 0;  ///< Max count
    uint64_t m_min_లెక్క = std::numeric_limits<uint64_t>::max();  ///< Min count
    Points_సమితి m_points;  // Points on this line

బహిరంగ:
    // CONSTRUCTORS
    explicit Vlc_ఆధారం_లెక్క(పూర్ణాంకం lineno)
        : m_lineno{lineno} {}
    ~Vlc_ఆధారం_లెక్క() = అప్రమేయ;

    // ACCESSORS
    పూర్ణాంకం lineno() స్థిరరాశి { తిరిగి m_lineno; }
    uint64_t max_లెక్క() స్థిరరాశి { తిరిగి m_max_లెక్క; }
    uint64_t min_లెక్క() స్థిరరాశి { తిరిగి m_min_లెక్క; }

    // METHODS
    శూన్య చొప్పించు_బిందువు(స్థిరరాశి Vlc_బిందువు* pointp) {
        m_max_లెక్క = std::max(m_max_లెక్క, pointp->లెక్క());
        m_min_లెక్క = std::min(m_min_లెక్క, pointp->లెక్క());
        m_points.emplace(pointp);
    }
    స్థిరరాశి Points_సమితి& points() { తిరిగి m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class Vlc_ఆధారం final {
బహిరంగ:
    // TYPES
    using LinenoMap = std::map<పూర్ణాంకం, Vlc_ఆధారం_లెక్క>;  // Map of {column}

అంతరంగ:
    // MEMBERS
    స్థిరరాశి string m_name;  //< Name of the source file
    LinenoMap m_పంక్తులు;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

బహిరంగ:
    // CONSTRUCTORS
    explicit Vlc_ఆధారం(స్థిరరాశి string& name)
        : m_name{name} {}
    ~Vlc_ఆధారం() = అప్రమేయ;

    // ACCESSORS
    స్థిరరాశి string& name() స్థిరరాశి { తిరిగి m_name; }
    bool needed() స్థిరరాశి { తిరిగి m_needed; }
    శూన్య needed(bool flag) { m_needed = flag; }
    LinenoMap& పంక్తులు() { తిరిగి m_పంక్తులు; }

    // METHODS
    శూన్య చొప్పించు_బిందువు(పూర్ణాంకం lineno, స్థిరరాశి Vlc_బిందువు* pointp) {
        Vlc_ఆధారం_లెక్క& sc = m_పంక్తులు.emplace(lineno, lineno).మొదటి->రెండవ;
        sc.చొప్పించు_బిందువు(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
బహిరంగ:
    // TYPES
    using NameMap = std::map<స్థిరరాశి std::string, Vlc_ఆధారం>;

అంతరంగ:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

బహిరంగ:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { తిరిగి m_sources.begin(); }
    NameMap::iterator చివర() { తిరిగి m_sources.చివర(); }

    // CONSTRUCTORS
    VlcSources() = అప్రమేయ;
    ~VlcSources() = అప్రమేయ;

    // METHODS
    Vlc_ఆధారం& కనుగొను_New_ఆధారం(స్థిరరాశి string& name) {
        NameMap::iterator iter = m_sources.కనుగొను(name);
        ఒకవేళ (iter != m_sources.చివర()) తిరిగి iter->రెండవ;
        iter = m_sources.emplace(name, Vlc_ఆధారం{name}).మొదటి;
        తిరిగి iter->రెండవ;
    }
};

//######################################################################

#అయితే_ముగింపు  // Guard
