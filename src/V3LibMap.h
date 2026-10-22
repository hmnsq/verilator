// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Link modules/signals together
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

#ifndef VERILATOR_V3LIBMAP_H_
#define VERILATOR_V3LIBMAP_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    અચલ string m_pattern;  // Pattern to match
    અચલ string m_libname;  // Library name
    અચલ string m_base;  // Relative path of the libmap file

જાહેર:
    LibMapping(અચલ string& pattern_, અચલ string& libname_, અચલ string& base_)
        : m_pattern{pattern_}
        , m_libname{libname_}
        , m_base{base_} {}
    string pattern() અચલ { પાછા m_pattern; }
    string libname() અચલ { પાછા m_libname; }
    string base() અચલ { પાછા m_base; }
};

class V3LibMap final {
ખાનગી:
    // STATE
    // 33.3.1.1 File path resolution
    //   If a file name potentially matches multiple file path specifications, the path
    //   specifications shall be resolved in the following order: a) File path specifications that
    //   end with an explicit file name b) File path specifications that end with a wildcarded file
    //   name c) File path specifications that end with a directory name
    std::vector<LibMapping> m_explicitMappings;
    std::vector<LibMapping> m_wildcardMappings;
    std::vector<LibMapping> m_directoryMappings;

    // METHODS
જાહેર:
    string matchMapping(અચલ string& filename);
    શૂન્ય addPattern(અચલ string& pattern, અચલ string& libname, અચલ string& base);

    સ્થિર શૂન્ય map(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
