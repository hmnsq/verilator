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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ধ্রুবক string m_pattern;  // Pattern to match
    ধ্রুবক string m_libname;  // Library name
    ধ্রুবক string m_base;  // Relative path of the libmap file

সর্বজনীন:
    LibMapping(ধ্রুবক string& pattern_, ধ্রুবক string& libname_, ধ্রুবক string& base_)
        : m_pattern{pattern_}
        , m_libname{libname_}
        , m_base{base_} {}
    string pattern() ধ্রুবক { ফেরত m_pattern; }
    string libname() ধ্রুবক { ফেরত m_libname; }
    string base() ধ্রুবক { ফেরত m_base; }
};

class V3LibMap final {
ব্যক্তিগত:
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
সর্বজনীন:
    string matchMapping(ধ্রুবক string& filename);
    শূন্য addPattern(ধ্রুবক string& pattern, ধ্রুবক string& libname, ধ্রুবক string& base);

    স্থির শূন্য map(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
