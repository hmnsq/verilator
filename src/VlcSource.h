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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "VlcPoint.h"

#شامل <limits>
#شامل <map>
#شامل <set>
#شامل <utility>
#شامل <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_شمار final {
    // TYPES
    using PointsSet = std::set<ثابت VlcPoint*>;

    // MEMBERS
    ثابت صحیح m_lineno;  ///< Line number
    uint64_t m_max_شمار = 0;  ///< Max count
    uint64_t m_min_شمار = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

عمومی:
    // CONSTRUCTORS
    explicit Vlc_Source_شمار(صحیح lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_شمار() = پیشفرض;

    // ACCESSORS
    صحیح lineno() ثابت { بازگشت m_lineno; }
    uint64_t max_شمار() ثابت { بازگشت m_max_شمار; }
    uint64_t min_شمار() ثابت { بازگشت m_min_شمار; }

    // METHODS
    تهی درج_Point(ثابت VlcPoint* pointp) {
        m_max_شمار = std::max(m_max_شمار, pointp->شمار());
        m_min_شمار = std::min(m_min_شمار, pointp->شمار());
        m_points.emplace(pointp);
    }
    ثابت PointsSet& points() { بازگشت m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
عمومی:
    // TYPES
    using LinenoMap = std::map<صحیح, Vlc_Source_شمار>;  // Map of {column}

خصوصی:
    // MEMBERS
    ثابت string m_name;  //< Name of the source file
    LinenoMap m_سطرها;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

عمومی:
    // CONSTRUCTORS
    explicit VlcSource(ثابت string& name)
        : m_name{name} {}
    ~VlcSource() = پیشفرض;

    // ACCESSORS
    ثابت string& name() ثابت { بازگشت m_name; }
    bool needed() ثابت { بازگشت m_needed; }
    تهی needed(bool flag) { m_needed = flag; }
    LinenoMap& سطرها() { بازگشت m_سطرها; }

    // METHODS
    تهی درج_Point(صحیح lineno, ثابت VlcPoint* pointp) {
        Vlc_Source_شمار& sc = m_سطرها.emplace(lineno, lineno).first->second;
        sc.درج_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
عمومی:
    // TYPES
    using NameMap = std::map<ثابت std::string, VlcSource>;

خصوصی:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

عمومی:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { بازگشت m_sources.begin(); }
    NameMap::iterator end() { بازگشت m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = پیشفرض;
    ~VlcSources() = پیشفرض;

    // METHODS
    VlcSource& findNewSource(ثابت string& name) {
        NameMap::iterator iter = m_sources.find(name);
        اگر (iter != m_sources.end()) بازگشت iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        بازگشت iter->second;
    }
};

//######################################################################

#endif  // Guard
