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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "VlcPoint.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_ಎಣಿಕೆ final {
    // TYPES
    using PointsSet = std::set<ಧ್ರುವ VlcPoint*>;

    // MEMBERS
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_lineno;  ///< Line number
    uint64_t m_max_ಎಣಿಕೆ = 0;  ///< Max count
    uint64_t m_min_ಎಣಿಕೆ = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit Vlc_Source_ಎಣಿಕೆ(ಪೂರ್ಣಾಙ್ಕ lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_ಎಣಿಕೆ() = ಸ್ವಭಾವ;

    // ACCESSORS
    ಪೂರ್ಣಾಙ್ಕ lineno() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_lineno; }
    uint64_t max_ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_max_ಎಣಿಕೆ; }
    uint64_t min_ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_min_ಎಣಿಕೆ; }

    // METHODS
    ಶೂನ್ಯ ಸೇರಿಸು_Point(ಧ್ರುವ VlcPoint* pointp) {
        m_max_ಎಣಿಕೆ = std::max(m_max_ಎಣಿಕೆ, pointp->ಎಣಿಕೆ());
        m_min_ಎಣಿಕೆ = std::min(m_min_ಎಣಿಕೆ, pointp->ಎಣಿಕೆ());
        m_points.emplace(pointp);
    }
    ಧ್ರುವ PointsSet& points() { ಪ್ರತ್ಯಾಗಮ m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using LinenoMap = std::map<ಪೂರ್ಣಾಙ್ಕ, Vlc_Source_ಎಣಿಕೆ>;  // Map of {column}

ಗುಪ್ತ:
    // MEMBERS
    ಧ್ರುವ string m_name;  //< Name of the source file
    LinenoMap m_ಸಾಲುಗಳು;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit VlcSource(ಧ್ರುವ string& name)
        : m_name{name} {}
    ~VlcSource() = ಸ್ವಭಾವ;

    // ACCESSORS
    ಧ್ರುವ string& name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }
    bool needed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needed; }
    ಶೂನ್ಯ needed(bool flag) { m_needed = flag; }
    LinenoMap& ಸಾಲುಗಳು() { ಪ್ರತ್ಯಾಗಮ m_ಸಾಲುಗಳು; }

    // METHODS
    ಶೂನ್ಯ ಸೇರಿಸು_Point(ಪೂರ್ಣಾಙ್ಕ lineno, ಧ್ರುವ VlcPoint* pointp) {
        Vlc_Source_ಎಣಿಕೆ& sc = m_ಸಾಲುಗಳು.emplace(lineno, lineno).first->second;
        sc.ಸೇರಿಸು_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using NameMap = std::map<ಧ್ರುವ std::string, VlcSource>;

ಗುಪ್ತ:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

ಸಾರ್ವಜನಿಕ:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { ಪ್ರತ್ಯಾಗಮ m_sources.begin(); }
    NameMap::iterator end() { ಪ್ರತ್ಯಾಗಮ m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = ಸ್ವಭಾವ;
    ~VlcSources() = ಸ್ವಭಾವ;

    // METHODS
    VlcSource& findNewSource(ಧ್ರುವ string& name) {
        NameMap::iterator iter = m_sources.find(name);
        ಯದಿ (iter != m_sources.end()) ಪ್ರತ್ಯಾಗಮ iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        ಪ್ರತ್ಯಾಗಮ iter->second;
    }
};

//######################################################################

#endif  // Guard
