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

#अगर_अघोषित VERILATOR_VLCSOURCE_H_
#घोषणा VERILATOR_VLCSOURCE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "VlcPoint.h"

#समावेश <limits>
#समावेश <map>
#समावेश <set>
#समावेश <utility>
#समावेश <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_गिनती final {
    // TYPES
    using PointsSet = std::set<स्थिर VlcPoint*>;

    // MEMBERS
    स्थिर पूर्णांक m_lineno;  ///< Line number
    uint64_t m_max_गिनती = 0;  ///< Max count
    uint64_t m_min_गिनती = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

खुला:
    // CONSTRUCTORS
    explicit Vlc_Source_गिनती(पूर्णांक lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_गिनती() = शेष;

    // ACCESSORS
    पूर्णांक lineno() स्थिर { वापस m_lineno; }
    uint64_t max_गिनती() स्थिर { वापस m_max_गिनती; }
    uint64_t min_गिनती() स्थिर { वापस m_min_गिनती; }

    // METHODS
    व्योम जोड़ो_Point(स्थिर VlcPoint* pointp) {
        m_max_गिनती = std::max(m_max_गिनती, pointp->गिनती());
        m_min_गिनती = std::min(m_min_गिनती, pointp->गिनती());
        m_points.emplace(pointp);
    }
    स्थिर PointsSet& points() { वापस m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
खुला:
    // TYPES
    using LinenoMap = std::map<पूर्णांक, Vlc_Source_गिनती>;  // Map of {column}

निजी:
    // MEMBERS
    स्थिर string m_name;  //< Name of the source file
    LinenoMap m_पंक्तियाँ;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

खुला:
    // CONSTRUCTORS
    explicit VlcSource(स्थिर string& name)
        : m_name{name} {}
    ~VlcSource() = शेष;

    // ACCESSORS
    स्थिर string& name() स्थिर { वापस m_name; }
    bool needed() स्थिर { वापस m_needed; }
    व्योम needed(bool flag) { m_needed = flag; }
    LinenoMap& पंक्तियाँ() { वापस m_पंक्तियाँ; }

    // METHODS
    व्योम जोड़ो_Point(पूर्णांक lineno, स्थिर VlcPoint* pointp) {
        Vlc_Source_गिनती& sc = m_पंक्तियाँ.emplace(lineno, lineno).first->second;
        sc.जोड़ो_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
खुला:
    // TYPES
    using NameMap = std::map<स्थिर std::string, VlcSource>;

निजी:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

खुला:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { वापस m_sources.begin(); }
    NameMap::iterator end() { वापस m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = शेष;
    ~VlcSources() = शेष;

    // METHODS
    VlcSource& findNewSource(स्थिर string& name) {
        NameMap::iterator iter = m_sources.find(name);
        अगर (iter != m_sources.end()) वापस iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        वापस iter->second;
    }
};

//######################################################################

#पूर्ण_अगर  // Guard
