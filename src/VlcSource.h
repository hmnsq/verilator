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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_VLCSOURCE_H_
#وصف_ڪريو VERILATOR_VLCSOURCE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "VlcPoint.h"

#ضمن <limits>
#ضمن <map>
#ضمن <set>
#ضمن <utility>
#ضمن <vector>

class Vlc_نقطو;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_ذريعو_ڳڻپ final {
    // TYPES
    using Points_سيٽ = std::سيٽ<ثابت Vlc_نقطو*>;

    // MEMBERS
    ثابت صحيح m_lineno;  ///< Line number
    uint64_t m_max_ڳڻپ = 0;  ///< Max count
    uint64_t m_min_ڳڻپ = std::numeric_limits<uint64_t>::max();  ///< Min count
    Points_سيٽ m_points;  // Points on this line

عام:
    // CONSTRUCTORS
    explicit Vlc_ذريعو_ڳڻپ(صحيح lineno)
        : m_lineno{lineno} {}
    ~Vlc_ذريعو_ڳڻپ() = افتراضي;

    // ACCESSORS
    صحيح lineno() ثابت { أرجع m_lineno; }
    uint64_t max_ڳڻپ() ثابت { أرجع m_max_ڳڻپ; }
    uint64_t min_ڳڻپ() ثابت { أرجع m_min_ڳڻپ; }

    // METHODS
    فراغ وجهو_نقطو(ثابت Vlc_نقطو* pointp) {
        m_max_ڳڻپ = std::max(m_max_ڳڻپ, pointp->ڳڻپ());
        m_min_ڳڻپ = std::min(m_min_ڳڻپ, pointp->ڳڻپ());
        m_points.emplace(pointp);
    }
    ثابت Points_سيٽ& points() { أرجع m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class Vlc_ذريعو final {
عام:
    // TYPES
    using LinenoMap = std::map<صحيح, Vlc_ذريعو_ڳڻپ>;  // Map of {column}

خاص:
    // MEMBERS
    ثابت string m_name;  //< Name of the source file
    LinenoMap m_سٽون;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

عام:
    // CONSTRUCTORS
    explicit Vlc_ذريعو(ثابت string& name)
        : m_name{name} {}
    ~Vlc_ذريعو() = افتراضي;

    // ACCESSORS
    ثابت string& name() ثابت { أرجع m_name; }
    bool needed() ثابت { أرجع m_needed; }
    فراغ needed(bool flag) { m_needed = flag; }
    LinenoMap& سٽون() { أرجع m_سٽون; }

    // METHODS
    فراغ وجهو_نقطو(صحيح lineno, ثابت Vlc_نقطو* pointp) {
        Vlc_ذريعو_ڳڻپ& sc = m_سٽون.emplace(lineno, lineno).پهريون->ٻيو;
        sc.وجهو_نقطو(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
عام:
    // TYPES
    using NameMap = std::map<ثابت std::string, Vlc_ذريعو>;

خاص:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

عام:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { أرجع m_sources.begin(); }
    NameMap::iterator پڇاڙي() { أرجع m_sources.پڇاڙي(); }

    // CONSTRUCTORS
    VlcSources() = افتراضي;
    ~VlcSources() = افتراضي;

    // METHODS
    Vlc_ذريعو& ڳوليو_New_ذريعو(ثابت string& name) {
        NameMap::iterator iter = m_sources.ڳوليو(name);
        إذا (iter != m_sources.پڇاڙي()) أرجع iter->ٻيو;
        iter = m_sources.emplace(name, Vlc_ذريعو{name}).پهريون;
        أرجع iter->ٻيو;
    }
};

//######################################################################

#جيڪڏهن_پورو  // Guard
