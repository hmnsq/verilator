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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ধ্রুৱ string m_pattern;  // Pattern to match
    ধ্রুৱ string m_libname;  // Library name
    ধ্রুৱ string m_base;  // Relative path of the libmap file

সার্ৱজনিক:
    LibMapping(ধ্রুৱ string& pattern_, ধ্রুৱ string& libname_, ধ্রুৱ string& base_)
        : m_pattern{pattern_}
        , m_libname{libname_}
        , m_base{base_} {}
    string pattern() ধ্রুৱ { ঘূৰাই m_pattern; }
    string libname() ধ্রুৱ { ঘূৰাই m_libname; }
    string base() ধ্রুৱ { ঘূৰাই m_base; }
};

class V3LibMap final {
গুপ্ত:
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
সার্ৱজনিক:
    string matchMapping(ধ্রুৱ string& filename);
    শূন্য addPattern(ধ্রুৱ string& pattern, ধ্রুৱ string& libname, ধ্রুৱ string& base);

    স্থির শূন্য map(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
