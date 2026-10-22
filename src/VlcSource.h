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

class Vlc_Source_मोजणी final {
    // TYPES
    using PointsSet = std::set<स्थिरांक VlcPoint*>;

    // MEMBERS
    स्थिरांक पूर्णांक m_lineno;  ///< Line number
    uint64_t m_max_मोजणी = 0;  ///< Max count
    uint64_t m_min_मोजणी = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

सार्वजनिक:
    // CONSTRUCTORS
    explicit Vlc_Source_मोजणी(पूर्णांक lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_मोजणी() = पूर्वनिर्धारित;

    // ACCESSORS
    पूर्णांक lineno() स्थिरांक { परत m_lineno; }
    uint64_t max_मोजणी() स्थिरांक { परत m_max_मोजणी; }
    uint64_t min_मोजणी() स्थिरांक { परत m_min_मोजणी; }

    // METHODS
    शून्य घाला_Point(स्थिरांक VlcPoint* pointp) {
        m_max_मोजणी = std::max(m_max_मोजणी, pointp->मोजणी());
        m_min_मोजणी = std::min(m_min_मोजणी, pointp->मोजणी());
        m_points.emplace(pointp);
    }
    स्थिरांक PointsSet& points() { परत m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
सार्वजनिक:
    // TYPES
    using LinenoMap = std::map<पूर्णांक, Vlc_Source_मोजणी>;  // Map of {column}

खाजगी:
    // MEMBERS
    स्थिरांक string m_name;  //< Name of the source file
    LinenoMap m_ओळी;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

सार्वजनिक:
    // CONSTRUCTORS
    explicit VlcSource(स्थिरांक string& name)
        : m_name{name} {}
    ~VlcSource() = पूर्वनिर्धारित;

    // ACCESSORS
    स्थिरांक string& name() स्थिरांक { परत m_name; }
    bool needed() स्थिरांक { परत m_needed; }
    शून्य needed(bool flag) { m_needed = flag; }
    LinenoMap& ओळी() { परत m_ओळी; }

    // METHODS
    शून्य घाला_Point(पूर्णांक lineno, स्थिरांक VlcPoint* pointp) {
        Vlc_Source_मोजणी& sc = m_ओळी.emplace(lineno, lineno).first->second;
        sc.घाला_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
सार्वजनिक:
    // TYPES
    using NameMap = std::map<स्थिरांक std::string, VlcSource>;

खाजगी:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

सार्वजनिक:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { परत m_sources.begin(); }
    NameMap::iterator end() { परत m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = पूर्वनिर्धारित;
    ~VlcSources() = पूर्वनिर्धारित;

    // METHODS
    VlcSource& findNewSource(स्थिरांक string& name) {
        NameMap::iterator iter = m_sources.find(name);
        जर (iter != m_sources.end()) परत iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        परत iter->second;
    }
};

//######################################################################

#endif  // Guard
