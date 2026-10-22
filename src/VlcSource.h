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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "VlcPoint.h"

#ضمن <limits>
#ضمن <map>
#ضمن <set>
#ضمن <utility>
#ضمن <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_کيراءن final {
    // TYPES
    using PointsSet = std::set<ثابت VlcPoint*>;

    // MEMBERS
    ثابت صحيح m_lineno;  ///< Line number
    uint64_t m_max_کيراءن = 0;  ///< Max count
    uint64_t m_min_کيراءن = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

عام:
    // CONSTRUCTORS
    explicit Vlc_Source_کيراءن(صحيح lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_کيراءن() = افتراضي;

    // ACCESSORS
    صحيح lineno() ثابت { أرجع m_lineno; }
    uint64_t max_کيراءن() ثابت { أرجع m_max_کيراءن; }
    uint64_t min_کيراءن() ثابت { أرجع m_min_کيراءن; }

    // METHODS
    فراغ سيسيڤ_Point(ثابت VlcPoint* pointp) {
        m_max_کيراءن = std::max(m_max_کيراءن, pointp->کيراءن());
        m_min_کيراءن = std::min(m_min_کيراءن, pointp->کيراءن());
        m_points.emplace(pointp);
    }
    ثابت PointsSet& points() { أرجع m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
عام:
    // TYPES
    using LinenoMap = std::map<صحيح, Vlc_Source_کيراءن>;  // Map of {column}

خاص:
    // MEMBERS
    ثابت string m_name;  //< Name of the source file
    LinenoMap m_باريس_سموا;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

عام:
    // CONSTRUCTORS
    explicit VlcSource(ثابت string& name)
        : m_name{name} {}
    ~VlcSource() = افتراضي;

    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    bool needed() ثابت { أرجع m_needed; }
    فراغ needed(bool flag) { m_needed = flag; }
    LinenoMap& باريس_سموا() { أرجع m_باريس_سموا; }

    // METHODS
    فراغ سيسيڤ_Point(صحيح lineno, ثابت VlcPoint* pointp) {
        Vlc_Source_کيراءن& sc = m_باريس_سموا.emplace(lineno, lineno).first->second;
        sc.سيسيڤ_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
عام:
    // TYPES
    using NameMap = std::map<ثابت std::string, VlcSource>;

خاص:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

عام:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { أرجع m_sources.begin(); }
    NameMap::iterator end() { أرجع m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = افتراضي;
    ~VlcSources() = افتراضي;

    // METHODS
    VlcSource& findNewSource(ثابت string& name) {
        NameMap::iterator iter = m_sources.find(name);
        إذا (iter != m_sources.end()) أرجع iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        أرجع iter->second;
    }
};

//######################################################################

#endif  // Guard
