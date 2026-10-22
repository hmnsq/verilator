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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "VlcPoint.h"

#অন্তর্ভাৱ <limits>
#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <utility>
#অন্তর্ভাৱ <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_গুনতি final {
    // TYPES
    using PointsSet = std::set<ধ্রুৱ VlcPoint*>;

    // MEMBERS
    ধ্রুৱ পূর্ণাঙ্ক m_lineno;  ///< Line number
    uint64_t m_max_গুনতি = 0;  ///< Max count
    uint64_t m_min_গুনতি = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit Vlc_Source_গুনতি(পূর্ণাঙ্ক lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_গুনতি() = স্ৱভাৱ;

    // ACCESSORS
    পূর্ণাঙ্ক lineno() ধ্রুৱ { ঘূৰাই m_lineno; }
    uint64_t max_গুনতি() ধ্রুৱ { ঘূৰাই m_max_গুনতি; }
    uint64_t min_গুনতি() ধ্রুৱ { ঘূৰাই m_min_গুনতি; }

    // METHODS
    শূন্য ঢোকাও_Point(ধ্রুৱ VlcPoint* pointp) {
        m_max_গুনতি = std::max(m_max_গুনতি, pointp->গুনতি());
        m_min_গুনতি = std::min(m_min_গুনতি, pointp->গুনতি());
        m_points.emplace(pointp);
    }
    ধ্রুৱ PointsSet& points() { ঘূৰাই m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
সার্ৱজনিক:
    // TYPES
    using LinenoMap = std::map<পূর্ণাঙ্ক, Vlc_Source_গুনতি>;  // Map of {column}

গুপ্ত:
    // MEMBERS
    ধ্রুৱ string m_name;  //< Name of the source file
    LinenoMap m_সাৰিগুলি;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit VlcSource(ধ্রুৱ string& name)
        : m_name{name} {}
    ~VlcSource() = স্ৱভাৱ;

    // ACCESSORS
    ধ্রুৱ string& name() ধ্রুৱ { ঘূৰাই m_name; }
    bool needed() ধ্রুৱ { ঘূৰাই m_needed; }
    শূন্য needed(bool flag) { m_needed = flag; }
    LinenoMap& সাৰিগুলি() { ঘূৰাই m_সাৰিগুলি; }

    // METHODS
    শূন্য ঢোকাও_Point(পূর্ণাঙ্ক lineno, ধ্রুৱ VlcPoint* pointp) {
        Vlc_Source_গুনতি& sc = m_সাৰিগুলি.emplace(lineno, lineno).first->second;
        sc.ঢোকাও_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
সার্ৱজনিক:
    // TYPES
    using NameMap = std::map<ধ্রুৱ std::string, VlcSource>;

গুপ্ত:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

সার্ৱজনিক:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { ঘূৰাই m_sources.begin(); }
    NameMap::iterator end() { ঘূৰাই m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = স্ৱভাৱ;
    ~VlcSources() = স্ৱভাৱ;

    // METHODS
    VlcSource& findNewSource(ধ্রুৱ string& name) {
        NameMap::iterator iter = m_sources.find(name);
        যদি (iter != m_sources.end()) ঘূৰাই iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        ঘূৰাই iter->second;
    }
};

//######################################################################

#endif  // Guard
