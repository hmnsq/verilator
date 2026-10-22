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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_VLCSOURCE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_VLCSOURCE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "VlcPoint.h"

#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

class Vlc_ಬಿಂದು;

//********************************************************************
// VlcColumnCount - count at specific source file, line and column

class Vlc_ಮೂಲ_ಎಣಿಕೆ final {
    // TYPES
    using Points_ಗಣ = std::ಗಣ<ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು*>;

    // MEMBERS
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ m_lineno;  ///< Line number
    uint64_t m_max_ಎಣಿಕೆ = 0;  ///< Max count
    uint64_t m_min_ಎಣಿಕೆ = std::numeric_limits<uint64_t>::max();  ///< Min count
    Points_ಗಣ m_points;  // Points on this line

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit Vlc_ಮೂಲ_ಎಣಿಕೆ(ಪೂರ್ಣಾಂಕ lineno)
        : m_lineno{lineno} {}
    ~Vlc_ಮೂಲ_ಎಣಿಕೆ() = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    ಪೂರ್ಣಾಂಕ lineno() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_lineno; }
    uint64_t max_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_max_ಎಣಿಕೆ; }
    uint64_t min_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_min_ಎಣಿಕೆ; }

    // METHODS
    ಶೂನ್ಯ ಸೇರಿಸು_ಬಿಂದು(ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು* pointp) {
        m_max_ಎಣಿಕೆ = std::max(m_max_ಎಣಿಕೆ, pointp->ಎಣಿಕೆ_2());
        m_min_ಎಣಿಕೆ = std::min(m_min_ಎಣಿಕೆ, pointp->ಎಣಿಕೆ_2());
        m_points.emplace(pointp);
    }
    ಸ್ಥಿರಾಂಕ Points_ಗಣ& points() { ಹಿಂತಿರುಗಿ m_points; }
};

//********************************************************************
// VlcSource - source file to annotate

class Vlc_ಮೂಲ final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using LinenoMap = std::map<ಪೂರ್ಣಾಂಕ, Vlc_ಮೂಲ_ಎಣಿಕೆ>;  // Map of {column}

ಖಾಸಗಿ:
    // MEMBERS
    ಸ್ಥಿರಾಂಕ string m_name;  //< Name of the source file
    LinenoMap m_ಸಾಲುಗಳು;  //< Map of each annotated line
    bool m_needed = false;  //< Need to annotate; has low coverage

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit Vlc_ಮೂಲ(ಸ್ಥಿರಾಂಕ string& name)
        : m_name{name} {}
    ~Vlc_ಮೂಲ() = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    ಸ್ಥಿರಾಂಕ string& name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }
    bool needed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needed; }
    ಶೂನ್ಯ needed(bool flag) { m_needed = flag; }
    LinenoMap& ಸಾಲುಗಳು() { ಹಿಂತಿರುಗಿ m_ಸಾಲುಗಳು; }

    // METHODS
    ಶೂನ್ಯ ಸೇರಿಸು_ಬಿಂದು(ಪೂರ್ಣಾಂಕ lineno, ಸ್ಥಿರಾಂಕ Vlc_ಬಿಂದು* pointp) {
        Vlc_ಮೂಲ_ಎಣಿಕೆ& sc = m_ಸಾಲುಗಳು.emplace(lineno, lineno).ಮೊದಲ->ಎರಡನೇ;
        sc.ಸೇರಿಸು_ಬಿಂದು(pointp);
    }
};

//********************************************************************
// VlcSources - Container of all source files

class VlcSources final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using NameMap = std::map<ಸ್ಥಿರಾಂಕ std::string, Vlc_ಮೂಲ>;

ಖಾಸಗಿ:
    // MEMBERS
    NameMap m_sources;  //< List of all sources

ಸಾರ್ವಜನಿಕ:
    // ITERATORS
    using iterator = NameMap::iterator;
    NameMap::iterator begin() { ಹಿಂತಿರುಗಿ m_sources.begin(); }
    NameMap::iterator ಕೊನೆ() { ಹಿಂತಿರುಗಿ m_sources.ಕೊನೆ(); }

    // CONSTRUCTORS
    VlcSources() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~VlcSources() = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    Vlc_ಮೂಲ& ಕಂಡುಹಿಡಿ_New_ಮೂಲ(ಸ್ಥಿರಾಂಕ string& name) {
        NameMap::iterator iter = m_sources.ಕಂಡುಹಿಡಿ(name);
        ಆದರೆ (iter != m_sources.ಕೊನೆ()) ಹಿಂತಿರುಗಿ iter->ಎರಡನೇ;
        iter = m_sources.emplace(name, Vlc_ಮೂಲ{name}).ಮೊದಲ;
        ಹಿಂತಿರುಗಿ iter->ಎರಡನೇ;
    }
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // Guard
