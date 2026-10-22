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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "VlcPoint.h"

#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <utility>
#അന്തര്ഭാവ <vector>

class VlcPoint;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_Source_എണ്ണം final {
    // TYPES
    using PointsSet = std::set<സ്ഥിരം VlcPoint*>;

    // MEMBERS
    സ്ഥിരം പൂർണ്ണസംഖ്യ m_lineno;  ///< Line number
    uint64_t m_max_എണ്ണം = 0;  ///< Max count
    uint64_t m_min_എണ്ണം = std::numeric_limits<uint64_t>::max();  ///< Min count
    PointsSet m_points;  // Points on this line

പൊതു:
    // CONSTRUCTORS
    explicit Vlc_Source_എണ്ണം(പൂർണ്ണസംഖ്യ lineno)
        : m_lineno{lineno} {}
    ~Vlc_Source_എണ്ണം() = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    പൂർണ്ണസംഖ്യ lineno() സ്ഥിരം { തിരികെ m_lineno; }
    uint64_t max_എണ്ണം() സ്ഥിരം { തിരികെ m_max_എണ്ണം; }
    uint64_t min_എണ്ണം() സ്ഥിരം { തിരികെ m_min_എണ്ണം; }

    // METHODS
    ശൂന്യം തിരുകുക_Point(സ്ഥിരം VlcPoint* pointp) {
        m_max_എണ്ണം = std::max(m_max_എണ്ണം, pointp->എണ്ണം());
        m_min_എണ്ണം = std::min(m_min_എണ്ണം, pointp->എണ്ണം());
        m_points.emplace(pointp);
    }
    സ്ഥിരം PointsSet& points() { തിരികെ m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class VlcSource final {
പൊതു:
    // TYPES
    using LinenoMap = std::map<പൂർണ്ണസംഖ്യ, Vlc_Source_എണ്ണം>;  // Map of {column}

സ്വകാര്യ:
    // MEMBERS
    സ്ഥിരം string m_name;  //< Name of the source file
    LinenoMap m_വരികൾ;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

പൊതു:
    // CONSTRUCTORS
    explicit VlcSource(സ്ഥിരം string& name)
        : m_name{name} {}
    ~VlcSource() = സ്ഥിരസ്ഥിതി;

    // ACCESSORS
    സ്ഥിരം string& name() സ്ഥിരം { തിരികെ m_name; }
    bool needed() സ്ഥിരം { തിരികെ m_needed; }
    ശൂന്യം needed(bool flag) { m_needed = flag; }
    LinenoMap& വരികൾ() { തിരികെ m_വരികൾ; }

    // METHODS
    ശൂന്യം തിരുകുക_Point(പൂർണ്ണസംഖ്യ lineno, സ്ഥിരം VlcPoint* pointp) {
        Vlc_Source_എണ്ണം& sc = m_വരികൾ.emplace(lineno, lineno).first->second;
        sc.തിരുകുക_Point(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
പൊതു:
    // TYPES
    using NameMap = std::map<സ്ഥിരം std::string, VlcSource>;

സ്വകാര്യ:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

പൊതു:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { തിരികെ m_sources.begin(); }
    NameMap::iterator end() { തിരികെ m_sources.end(); }

    // CONSTRUCTORS
    VlcSources() = സ്ഥിരസ്ഥിതി;
    ~VlcSources() = സ്ഥിരസ്ഥിതി;

    // METHODS
    VlcSource& findNewSource(സ്ഥിരം string& name) {
        NameMap::iterator iter = m_sources.find(name);
        എങ്കിൽ (iter != m_sources.end()) തിരികെ iter->second;
        iter = m_sources.emplace(name, VlcSource{name}).first;
        തിരികെ iter->second;
    }
};

//######################################################################

#endif  // Guard
