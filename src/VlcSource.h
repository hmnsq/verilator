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

#اگر_غیر_تعریف_شدہ VERILATOR_VLCSOURCE_H_
#تعریف_کرو VERILATOR_VLCSOURCE_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "VlcPoint.h"

#شامل <limits>
#شامل <map>
#شامل <set>
#شامل <utility>
#شامل <vector>

class Vlc_نقطہ;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_ماخذ_گنتی final {
    // TYPES
    using Points_سیٹ = std::سیٹ<ثابت Vlc_نقطہ*>;

    // MEMBERS
    ثابت صحیح m_lineno;  ///< Line number
    uint64_t m_max_گنتی = 0;  ///< Max count
    uint64_t m_min_گنتی = std::numeric_limits<uint64_t>::max();  ///< Min count
    Points_سیٹ m_points;  // Points on this line

عوامی:
    // CONSTRUCTORS
    explicit Vlc_ماخذ_گنتی(صحیح lineno)
        : m_lineno{lineno} {}
    ~Vlc_ماخذ_گنتی() = طےشدہ;

    // ACCESSORS
    صحیح lineno() ثابت { واپس m_lineno; }
    uint64_t max_گنتی() ثابت { واپس m_max_گنتی; }
    uint64_t min_گنتی() ثابت { واپس m_min_گنتی; }

    // METHODS
    خالی داخل_کرو_نقطہ(ثابت Vlc_نقطہ* pointp) {
        m_max_گنتی = std::max(m_max_گنتی, pointp->گنتی());
        m_min_گنتی = std::min(m_min_گنتی, pointp->گنتی());
        m_points.emplace(pointp);
    }
    ثابت Points_سیٹ& points() { واپس m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class Vlc_ماخذ final {
عوامی:
    // TYPES
    using LinenoMap = std::map<صحیح, Vlc_ماخذ_گنتی>;  // Map of {column}

نجی:
    // MEMBERS
    ثابت string m_name;  //< Name of the source file
    LinenoMap m_سطریں;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

عوامی:
    // CONSTRUCTORS
    explicit Vlc_ماخذ(ثابت string& name)
        : m_name{name} {}
    ~Vlc_ماخذ() = طےشدہ;

    // ACCESSORS
    ثابت string& name() ثابت { واپس m_name; }
    bool needed() ثابت { واپس m_needed; }
    خالی needed(bool flag) { m_needed = flag; }
    LinenoMap& سطریں() { واپس m_سطریں; }

    // METHODS
    خالی داخل_کرو_نقطہ(صحیح lineno, ثابت Vlc_نقطہ* pointp) {
        Vlc_ماخذ_گنتی& sc = m_سطریں.emplace(lineno, lineno).پہلا->دوسرا;
        sc.داخل_کرو_نقطہ(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
عوامی:
    // TYPES
    using NameMap = std::map<ثابت std::string, Vlc_ماخذ>;

نجی:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

عوامی:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { واپس m_sources.begin(); }
    NameMap::iterator انتہا() { واپس m_sources.انتہا(); }

    // CONSTRUCTORS
    VlcSources() = طےشدہ;
    ~VlcSources() = طےشدہ;

    // METHODS
    Vlc_ماخذ& ڈھونڈو_New_ماخذ(ثابت string& name) {
        NameMap::iterator iter = m_sources.ڈھونڈو(name);
        اگر (iter != m_sources.انتہا()) واپس iter->دوسرا;
        iter = m_sources.emplace(name, Vlc_ماخذ{name}).پہلا;
        واپس iter->دوسرا;
    }
};

//######################################################################

#اگر_ختم  // Guard
