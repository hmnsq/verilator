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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ಧ್ರುವ string m_pattern;  // Pattern to match
    ಧ್ರುವ string m_libname;  // Library name
    ಧ್ರುವ string m_base;  // Relative path of the libmap file

ಸಾರ್ವಜನಿಕ:
    LibMapping(ಧ್ರುವ string& pattern_, ಧ್ರುವ string& libname_, ಧ್ರುವ string& base_)
        : m_pattern{pattern_}
        , m_libname{libname_}
        , m_base{base_} {}
    string pattern() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pattern; }
    string libname() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libname; }
    string base() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_base; }
};

class V3LibMap final {
ಗುಪ್ತ:
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
ಸಾರ್ವಜನಿಕ:
    string matchMapping(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ addPattern(ಧ್ರುವ string& pattern, ಧ್ರುವ string& libname, ಧ್ರುವ string& base);

    ಸ್ಥಿರ ಶೂನ್ಯ map(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
