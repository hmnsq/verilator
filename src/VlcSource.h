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

class Vlc_Source_गिनती final {
    // TYPES
    using PointsSet = std::set<ध्रुव VlcPoint*>;

    // MEMBERS
    ध्रुव पूर्णाङ्क m_lineno;  ///< Line number
    uint64_t m_max_गिनती = 0;  ///< Max count
    uint64_t m_min_गिनती = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

सार्वजनिक:
    // CONSTRUCTORS
    explicit Vlc_Source_गिनती(पूर्णाङ्क lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_गिनती() = स्वभाव;

    // ACCESSORS
    पूर्णाङ्क lineno() ध्रुव { प्रत्यागम m_lineno; }
    uint64_t max_गिनती() ध्रुव { प्रत्यागम m_max_गिनती; }
    uint64_t min_गिनती() ध्रुव { प्रत्यागम m_min_गिनती; }

    // METHODS
    शून्य जोड़ो_Point(ध्रुव VlcPoint* pointp) {
        m_max_गिनती = std::max(m_max_गिनती, pointp->गिनती());
        m_min_गिनती = std::min(m_min_गिनती, pointp->गिनती());
        m_points.emplace(pointp);
    }
    ध्रुव PointsSet& points() { प्रत्यागम m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
सार्वजनिक:
    // TYPES
    using LinenoMap = std::map<पूर्णाङ्क, Vlc_Source_गिनती>;  // Map of {column}

गुप्त:
    // MEMBERS
    ध्रुव string m_name;  //< Name of the source file
    LinenoMap m_पंक्तियाँ;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

सार्वजनिक:
    // CONSTRUCTORS
    explicit VlcSource(ध्रुव string& name)
        : m_name{name} {}
    ~VlcSource() = स्वभाव;

    // ACCESSORS
    ध्रुव string& name() ध्रुव { प्रत्यागम m_name; }
    bool needed() ध्रुव { प्रत्यागम m_needed; }
    शून्य needed(bool flag) { m_needed = flag; }
    LinenoMap& पंक्तियाँ() { प्रत्यागम m_पंक्तियाँ; }

    // METHODS
    शून्य जोड़ो_Point(पूर्णाङ्क lineno, ध्रुव VlcPoint* pointp) {
        Vlc_Source_गिनती& sc = m_पंक्तियाँ.emplace(lineno, lineno).first->second;
        sc.जोड़ो_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
सार्वजनिक:
    // TYPES
    using NameMap = std::map<ध्रुव std::string, VlcSource>;

गुप्त:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

सार्वजनिक:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { प्रत्यागम m_sources.begin(); }
    NameMap::iterator end() { प्रत्यागम m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = स्वभाव;
    ~VlcSources() = स्वभाव;

    // METHODS
    VlcSource& findNewSource(ध्रुव string& name) {
        NameMap::iterator iter = m_sources.find(name);
        यदि (iter != m_sources.end()) प्रत्यागम iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        प्रत्यागम iter->second;
    }
};

//######################################################################

#endif  // Guard
