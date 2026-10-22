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

#اگر_تعریف_نشده VERILATOR_VLCSOURCE_H_
#تعریف_کن VERILATOR_VLCSOURCE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "VlcPoint.h"

#ضمن <limits>
#ضمن <map>
#ضمن <set>
#ضمن <utility>
#ضمن <vector>

class Vlc_نقطه;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_منبع_شمار final {
    // TYPES
    using Points_مجموعه = std::مجموعه<ثابت Vlc_نقطه*>;

    // MEMBERS
    ثابت صحيح m_lineno;  ///< Line number
    uint64_t m_max_شمار = 0;  ///< Max count
    uint64_t m_min_شمار = std::numeric_limits<uint64_t>::max();  ///< Min count
    Points_مجموعه m_points;  // Points on this line

عام:
    // CONSTRUCTORS
    explicit Vlc_منبع_شمار(صحيح lineno)
        : m_lineno{lineno} {}
    ~Vlc_منبع_شمار() = افتراضي;

    // ACCESSORS
    صحيح lineno() ثابت { أرجع m_lineno; }
    uint64_t max_شمار() ثابت { أرجع m_max_شمار; }
    uint64_t min_شمار() ثابت { أرجع m_min_شمار; }

    // METHODS
    فراغ درج_نقطه(ثابت Vlc_نقطه* pointp) {
        m_max_شمار = std::max(m_max_شمار, pointp->شمار());
        m_min_شمار = std::min(m_min_شمار, pointp->شمار());
        m_points.emplace(pointp);
    }
    ثابت Points_مجموعه& points() { أرجع m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class Vlc_منبع final {
عام:
    // TYPES
    using LinenoMap = std::map<صحيح, Vlc_منبع_شمار>;  // Map of {column}

خاص:
    // MEMBERS
    ثابت string m_name;  //< Name of the source file
    LinenoMap m_سطرها;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

عام:
    // CONSTRUCTORS
    explicit Vlc_منبع(ثابت string& name)
        : m_name{name} {}
    ~Vlc_منبع() = افتراضي;

    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    bool needed() ثابت { أرجع m_needed; }
    فراغ needed(bool flag) { m_needed = flag; }
    LinenoMap& سطرها() { أرجع m_سطرها; }

    // METHODS
    فراغ درج_نقطه(صحيح lineno, ثابت Vlc_نقطه* pointp) {
        Vlc_منبع_شمار& sc = m_سطرها.emplace(lineno, lineno).اولی->دومی;
        sc.درج_نقطه(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
عام:
    // TYPES
    using NameMap = std::map<ثابت std::string, Vlc_منبع>;

خاص:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

عام:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { أرجع m_sources.begin(); }
    NameMap::iterator انتها() { أرجع m_sources.انتها(); }

    // CONSTRUCTORS
    VlcSources() = افتراضي;
    ~VlcSources() = افتراضي;

    // METHODS
    Vlc_منبع& پیدا_کن_New_منبع(ثابت string& name) {
        NameMap::iterator iter = m_sources.پیدا_کن(name);
        إذا (iter != m_sources.انتها()) أرجع iter->دومی;
        iter = m_sources.emplace(name, Vlc_منبع{name}).اولی;
        أرجع iter->دومی;
    }
};

//######################################################################

#پایان_اگر  // Guard
