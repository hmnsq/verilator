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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_VLCSOURCE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_VLCSOURCE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "VlcPoint.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class Vlc_ਬਿੰਦੂ;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_ਸਰੋਤ_ਗਿਣਤੀ final {
    // TYPES
    using Points_ਸੈੱਟ = std::ਸੈੱਟ<ਅਚਲ Vlc_ਬਿੰਦੂ*>;

    // MEMBERS
    ਅਚਲ ਪੂਰਨ m_lineno;  ///< Line number
    uint64_t m_max_ਗਿਣਤੀ = 0;  ///< Max count
    uint64_t m_min_ਗਿਣਤੀ = std::numeric_limits<uint64_t>::max();  ///< Min count
    Points_ਸੈੱਟ m_points;  // Points on this line

ਜਨਤਕ:
    // CONSTRUCTORS
    explicit Vlc_ਸਰੋਤ_ਗਿਣਤੀ(ਪੂਰਨ lineno)
        : m_lineno{lineno} {}
    ~Vlc_ਸਰੋਤ_ਗਿਣਤੀ() = ਮੂਲ;

    // ACCESSORS
    ਪੂਰਨ lineno() ਅਚਲ { ਵਾਪਸ m_lineno; }
    uint64_t max_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_max_ਗਿਣਤੀ; }
    uint64_t min_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_min_ਗਿਣਤੀ; }

    // METHODS
    ਖਾਲੀ ਪਾਓ_ਬਿੰਦੂ(ਅਚਲ Vlc_ਬਿੰਦੂ* pointp) {
        m_max_ਗਿਣਤੀ = std::max(m_max_ਗਿਣਤੀ, pointp->ਗਿਣਤੀ_2());
        m_min_ਗਿਣਤੀ = std::min(m_min_ਗਿਣਤੀ, pointp->ਗਿਣਤੀ_2());
        m_points.emplace(pointp);
    }
    ਅਚਲ Points_ਸੈੱਟ& points() { ਵਾਪਸ m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class Vlc_ਸਰੋਤ final {
ਜਨਤਕ:
    // TYPES
    using LinenoMap = std::map<ਪੂਰਨ, Vlc_ਸਰੋਤ_ਗਿਣਤੀ>;  // Map of {column}

ਨਿਜੀ:
    // MEMBERS
    ਅਚਲ string m_name;  //< Name of the source file
    LinenoMap m_ਸਤਰਾਂ;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

ਜਨਤਕ:
    // CONSTRUCTORS
    explicit Vlc_ਸਰੋਤ(ਅਚਲ string& name)
        : m_name{name} {}
    ~Vlc_ਸਰੋਤ() = ਮੂਲ;

    // ACCESSORS
    ਅਚਲ string& name() ਅਚਲ { ਵਾਪਸ m_name; }
    bool needed() ਅਚਲ { ਵਾਪਸ m_needed; }
    ਖਾਲੀ needed(bool flag) { m_needed = flag; }
    LinenoMap& ਸਤਰਾਂ() { ਵਾਪਸ m_ਸਤਰਾਂ; }

    // METHODS
    ਖਾਲੀ ਪਾਓ_ਬਿੰਦੂ(ਪੂਰਨ lineno, ਅਚਲ Vlc_ਬਿੰਦੂ* pointp) {
        Vlc_ਸਰੋਤ_ਗਿਣਤੀ& sc = m_ਸਤਰਾਂ.emplace(lineno, lineno).ਪਹਿਲਾ->ਦੂਜਾ;
        sc.ਪਾਓ_ਬਿੰਦੂ(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
ਜਨਤਕ:
    // TYPES
    using NameMap = std::map<ਅਚਲ std::string, Vlc_ਸਰੋਤ>;

ਨਿਜੀ:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

ਜਨਤਕ:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { ਵਾਪਸ m_sources.begin(); }
    NameMap::iterator ਸਿਰਾ() { ਵਾਪਸ m_sources.ਸਿਰਾ(); }

    // CONSTRUCTORS
    VlcSources() = ਮੂਲ;
    ~VlcSources() = ਮੂਲ;

    // METHODS
    Vlc_ਸਰੋਤ& ਲੱਭੋ_New_ਸਰੋਤ(ਅਚਲ string& name) {
        NameMap::iterator iter = m_sources.ਲੱਭੋ(name);
        ਜੇ (iter != m_sources.ਸਿਰਾ()) ਵਾਪਸ iter->ਦੂਜਾ;
        iter = m_sources.emplace(name, Vlc_ਸਰੋਤ{name}).ਪਹਿਲਾ;
        ਵਾਪਸ iter->ਦੂਜਾ;
    }
};

//######################################################################

#ਜੇ_ਅੰਤ  // Guard
