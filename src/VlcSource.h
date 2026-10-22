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
    using PointsSet = std::set<ধ্রুবক VlcPoint*>;

    // MEMBERS
    ধ্রুবক পূর্ণসংখ্যা m_lineno;  ///< Line number
    uint64_t m_max_গুনতি = 0;  ///< Max count
    uint64_t m_min_গুনতি = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

সর্বজনীন:
    // CONSTRUCTORS
    explicit Vlc_Source_গুনতি(পূর্ণসংখ্যা lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_গুনতি() = পূর্বনির্ধারিত;

    // ACCESSORS
    পূর্ণসংখ্যা lineno() ধ্রুবক { ফেরত m_lineno; }
    uint64_t max_গুনতি() ধ্রুবক { ফেরত m_max_গুনতি; }
    uint64_t min_গুনতি() ধ্রুবক { ফেরত m_min_গুনতি; }

    // METHODS
    শূন্য ঢোকাও_Point(ধ্রুবক VlcPoint* pointp) {
        m_max_গুনতি = std::max(m_max_গুনতি, pointp->গুনতি());
        m_min_গুনতি = std::min(m_min_গুনতি, pointp->গুনতি());
        m_points.emplace(pointp);
    }
    ধ্রুবক PointsSet& points() { ফেরত m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
সর্বজনীন:
    // TYPES
    using LinenoMap = std::map<পূর্ণসংখ্যা, Vlc_Source_গুনতি>;  // Map of {column}

ব্যক্তিগত:
    // MEMBERS
    ধ্রুবক string m_name;  //< Name of the source file
    LinenoMap m_সারিগুলি;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

সর্বজনীন:
    // CONSTRUCTORS
    explicit VlcSource(ধ্রুবক string& name)
        : m_name{name} {}
    ~VlcSource() = পূর্বনির্ধারিত;

    // ACCESSORS
    ধ্রুবক string& name() ধ্রুবক { ফেরত m_name; }
    bool needed() ধ্রুবক { ফেরত m_needed; }
    শূন্য needed(bool flag) { m_needed = flag; }
    LinenoMap& সারিগুলি() { ফেরত m_সারিগুলি; }

    // METHODS
    শূন্য ঢোকাও_Point(পূর্ণসংখ্যা lineno, ধ্রুবক VlcPoint* pointp) {
        Vlc_Source_গুনতি& sc = m_সারিগুলি.emplace(lineno, lineno).first->second;
        sc.ঢোকাও_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
সর্বজনীন:
    // TYPES
    using NameMap = std::map<ধ্রুবক std::string, VlcSource>;

ব্যক্তিগত:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

সর্বজনীন:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { ফেরত m_sources.begin(); }
    NameMap::iterator end() { ফেরত m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = পূর্বনির্ধারিত;
    ~VlcSources() = পূর্বনির্ধারিত;

    // METHODS
    VlcSource& findNewSource(ধ্রুবক string& name) {
        NameMap::iterator iter = m_sources.find(name);
        যদি (iter != m_sources.end()) ফেরত iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        ফেরত iter->second;
    }
};

//######################################################################

#endif  // Guard
