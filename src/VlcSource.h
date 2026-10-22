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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "VlcPoint.h"

#அந்தர்பாவ <limits>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_மொத்தம் final {
    // TYPES
    using PointsSet = std::set<மாறிலி VlcPoint*>;

    // MEMBERS
    மாறிலி முழு m_lineno;  ///< Line number
    uint64_t m_max_மொத்தம் = 0;  ///< Max count
    uint64_t m_min_மொத்தம் = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

பொது:
    // CONSTRUCTORS
    explicit Vlc_Source_மொத்தம்(முழு lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_மொத்தம்() = இயல்பு;

    // ACCESSORS
    முழு lineno() மாறிலி { திரும்பு m_lineno; }
    uint64_t max_மொத்தம்() மாறிலி { திரும்பு m_max_மொத்தம்; }
    uint64_t min_மொத்தம்() மாறிலி { திரும்பு m_min_மொத்தம்; }

    // METHODS
    வெறுமை செருகு_Point(மாறிலி VlcPoint* pointp) {
        m_max_மொத்தம் = std::max(m_max_மொத்தம், pointp->மொத்தம்());
        m_min_மொத்தம் = std::min(m_min_மொத்தம், pointp->மொத்தம்());
        m_points.emplace(pointp);
    }
    மாறிலி PointsSet& points() { திரும்பு m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
பொது:
    // TYPES
    using LinenoMap = std::map<முழு, Vlc_Source_மொத்தம்>;  // Map of {column}

தனிப்பட்ட:
    // MEMBERS
    மாறிலி string m_name;  //< Name of the source file
    LinenoMap m_வரிகள்;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

பொது:
    // CONSTRUCTORS
    explicit VlcSource(மாறிலி string& name)
        : m_name{name} {}
    ~VlcSource() = இயல்பு;

    // ACCESSORS
    மாறிலி string& name() மாறிலி { திரும்பு m_name; }
    bool needed() மாறிலி { திரும்பு m_needed; }
    வெறுமை needed(bool flag) { m_needed = flag; }
    LinenoMap& வரிகள்() { திரும்பு m_வரிகள்; }

    // METHODS
    வெறுமை செருகு_Point(முழு lineno, மாறிலி VlcPoint* pointp) {
        Vlc_Source_மொத்தம்& sc = m_வரிகள்.emplace(lineno, lineno).first->second;
        sc.செருகு_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
பொது:
    // TYPES
    using NameMap = std::map<மாறிலி std::string, VlcSource>;

தனிப்பட்ட:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

பொது:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { திரும்பு m_sources.begin(); }
    NameMap::iterator end() { திரும்பு m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = இயல்பு;
    ~VlcSources() = இயல்பு;

    // METHODS
    VlcSource& findNewSource(மாறிலி string& name) {
        NameMap::iterator iter = m_sources.find(name);
        எனில் (iter != m_sources.end()) திரும்பு iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        திரும்பு iter->second;
    }
};

//######################################################################

#endif  // Guard
