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

#که_نه_تعریف_شوی VERILATOR_VLCSOURCE_H_
#تعریف_کړه VERILATOR_VLCSOURCE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "VlcPoint.h"

#ضمن <limits>
#ضمن <map>
#ضمن <set>
#ضمن <utility>
#ضمن <vector>

class Vlc_ټکی;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_سرچینه_شمېر final {
    // TYPES
    using Points_سېټ = std::سېټ<ثابت Vlc_ټکی*>;

    // MEMBERS
    ثابت صحيح m_lineno;  ///< Line number
    uint64_t m_max_شمېر = 0;  ///< Max count
    uint64_t m_min_شمېر = std::numeric_limits<uint64_t>::max();  ///< Min count
    Points_سېټ m_points;  // Points on this line

عام:
    // CONSTRUCTORS
    explicit Vlc_سرچینه_شمېر(صحيح lineno)
        : m_lineno{lineno} {}
    ~Vlc_سرچینه_شمېر() = افتراضي;

    // ACCESSORS
    صحيح lineno() ثابت { أرجع m_lineno; }
    uint64_t max_شمېر() ثابت { أرجع m_max_شمېر; }
    uint64_t min_شمېر() ثابت { أرجع m_min_شمېر; }

    // METHODS
    فراغ ورننباسه_ټکی(ثابت Vlc_ټکی* pointp) {
        m_max_شمېر = std::max(m_max_شمېر, pointp->شمېر());
        m_min_شمېر = std::min(m_min_شمېر, pointp->شمېر());
        m_points.emplace(pointp);
    }
    ثابت Points_سېټ& points() { أرجع m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class Vlc_سرچینه final {
عام:
    // TYPES
    using LinenoMap = std::map<صحيح, Vlc_سرچینه_شمېر>;  // Map of {column}

خاص:
    // MEMBERS
    ثابت string m_name;  //< Name of the source file
    LinenoMap m_کرښې;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

عام:
    // CONSTRUCTORS
    explicit Vlc_سرچینه(ثابت string& name)
        : m_name{name} {}
    ~Vlc_سرچینه() = افتراضي;

    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    bool needed() ثابت { أرجع m_needed; }
    فراغ needed(bool flag) { m_needed = flag; }
    LinenoMap& کرښې() { أرجع m_کرښې; }

    // METHODS
    فراغ ورننباسه_ټکی(صحيح lineno, ثابت Vlc_ټکی* pointp) {
        Vlc_سرچینه_شمېر& sc = m_کرښې.emplace(lineno, lineno).لومړی->دوهم;
        sc.ورننباسه_ټکی(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
عام:
    // TYPES
    using NameMap = std::map<ثابت std::string, Vlc_سرچینه>;

خاص:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

عام:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { أرجع m_sources.begin(); }
    NameMap::iterator پای_ټکی() { أرجع m_sources.پای_ټکی(); }

    // CONSTRUCTORS
    VlcSources() = افتراضي;
    ~VlcSources() = افتراضي;

    // METHODS
    Vlc_سرچینه& پیدا_کړه_New_سرچینه(ثابت string& name) {
        NameMap::iterator iter = m_sources.پیدا_کړه(name);
        إذا (iter != m_sources.پای_ټکی()) أرجع iter->دوهم;
        iter = m_sources.emplace(name, Vlc_سرچینه{name}).لومړی;
        أرجع iter->دوهم;
    }
};

//######################################################################

#د_که_پای  // Guard
