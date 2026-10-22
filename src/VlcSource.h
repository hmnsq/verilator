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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "VlcPoint.h"

#अन्तर्भाव <limits>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_गन्ती final {
    // TYPES
    using PointsSet = std::set<अचर VlcPoint*>;

    // MEMBERS
    अचर पूर्णाङ्क m_lineno;  ///< Line number
    uint64_t m_max_गन्ती = 0;  ///< Max count
    uint64_t m_min_गन्ती = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

सार्वजनिक:
    // CONSTRUCTORS
    explicit Vlc_Source_गन्ती(पूर्णाङ्क lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_गन्ती() = स्वभाव;

    // ACCESSORS
    पूर्णाङ्क lineno() अचर { फर्काऊ m_lineno; }
    uint64_t max_गन्ती() अचर { फर्काऊ m_max_गन्ती; }
    uint64_t min_गन्ती() अचर { फर्काऊ m_min_गन्ती; }

    // METHODS
    शून्य घुसाउ_Point(अचर VlcPoint* pointp) {
        m_max_गन्ती = std::max(m_max_गन्ती, pointp->गन्ती());
        m_min_गन्ती = std::min(m_min_गन्ती, pointp->गन्ती());
        m_points.emplace(pointp);
    }
    अचर PointsSet& points() { फर्काऊ m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
सार्वजनिक:
    // TYPES
    using LinenoMap = std::map<पूर्णाङ्क, Vlc_Source_गन्ती>;  // Map of {column}

गुप्त:
    // MEMBERS
    अचर string m_name;  //< Name of the source file
    LinenoMap m_पङ्क्तिहरू;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

सार्वजनिक:
    // CONSTRUCTORS
    explicit VlcSource(अचर string& name)
        : m_name{name} {}
    ~VlcSource() = स्वभाव;

    // ACCESSORS
    अचर string& name() अचर { फर्काऊ m_name; }
    bool needed() अचर { फर्काऊ m_needed; }
    शून्य needed(bool flag) { m_needed = flag; }
    LinenoMap& पङ्क्तिहरू() { फर्काऊ m_पङ्क्तिहरू; }

    // METHODS
    शून्य घुसाउ_Point(पूर्णाङ्क lineno, अचर VlcPoint* pointp) {
        Vlc_Source_गन्ती& sc = m_पङ्क्तिहरू.emplace(lineno, lineno).first->second;
        sc.घुसाउ_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
सार्वजनिक:
    // TYPES
    using NameMap = std::map<अचर std::string, VlcSource>;

गुप्त:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

सार्वजनिक:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { फर्काऊ m_sources.begin(); }
    NameMap::iterator end() { फर्काऊ m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = स्वभाव;
    ~VlcSources() = स्वभाव;

    // METHODS
    VlcSource& findNewSource(अचर string& name) {
        NameMap::iterator iter = m_sources.find(name);
        यदि (iter != m_sources.end()) फर्काऊ iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        फर्काऊ iter->second;
    }
};

//######################################################################

#endif  // Guard
