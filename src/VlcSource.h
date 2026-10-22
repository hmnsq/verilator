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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "VlcPoint.h"

#অন্তর্ভাব <limits>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <utility>
#অন্তর্ভাব <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_গুনতি final {
    // TYPES
    using PointsSet = std::set<ধ্রুব VlcPoint*>;

    // MEMBERS
    ধ্রুব পূর্ণাঙ্ক m_lineno;  ///< Line number
    uint64_t m_max_গুনতি = 0;  ///< Max count
    uint64_t m_min_গুনতি = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

সার্বজনিক:
    // CONSTRUCTORS
    explicit Vlc_Source_গুনতি(পূর্ণাঙ্ক lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_গুনতি() = স্বভাব;

    // ACCESSORS
    পূর্ণাঙ্ক lineno() ধ্রুব { প্রত্যাগম m_lineno; }
    uint64_t max_গুনতি() ধ্রুব { প্রত্যাগম m_max_গুনতি; }
    uint64_t min_গুনতি() ধ্রুব { প্রত্যাগম m_min_গুনতি; }

    // METHODS
    শূন্য ঢোকাও_Point(ধ্রুব VlcPoint* pointp) {
        m_max_গুনতি = std::max(m_max_গুনতি, pointp->গুনতি());
        m_min_গুনতি = std::min(m_min_গুনতি, pointp->গুনতি());
        m_points.emplace(pointp);
    }
    ধ্রুব PointsSet& points() { প্রত্যাগম m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
সার্বজনিক:
    // TYPES
    using LinenoMap = std::map<পূর্ণাঙ্ক, Vlc_Source_গুনতি>;  // Map of {column}

গুপ্ত:
    // MEMBERS
    ধ্রুব string m_name;  //< Name of the source file
    LinenoMap m_সারিগুলি;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

সার্বজনিক:
    // CONSTRUCTORS
    explicit VlcSource(ধ্রুব string& name)
        : m_name{name} {}
    ~VlcSource() = স্বভাব;

    // ACCESSORS
    ধ্রুব string& name() ধ্রুব { প্রত্যাগম m_name; }
    bool needed() ধ্রুব { প্রত্যাগম m_needed; }
    শূন্য needed(bool flag) { m_needed = flag; }
    LinenoMap& সারিগুলি() { প্রত্যাগম m_সারিগুলি; }

    // METHODS
    শূন্য ঢোকাও_Point(পূর্ণাঙ্ক lineno, ধ্রুব VlcPoint* pointp) {
        Vlc_Source_গুনতি& sc = m_সারিগুলি.emplace(lineno, lineno).first->second;
        sc.ঢোকাও_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
সার্বজনিক:
    // TYPES
    using NameMap = std::map<ধ্রুব std::string, VlcSource>;

গুপ্ত:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

সার্বজনিক:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { প্রত্যাগম m_sources.begin(); }
    NameMap::iterator end() { প্রত্যাগম m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = স্বভাব;
    ~VlcSources() = স্বভাব;

    // METHODS
    VlcSource& findNewSource(ধ্রুব string& name) {
        NameMap::iterator iter = m_sources.find(name);
        যদি (iter != m_sources.end()) প্রত্যাগম iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        প্রত্যাগম iter->second;
    }
};

//######################################################################

#endif  // Guard
