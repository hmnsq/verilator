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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    மாறிலி string m_pattern;  // Pattern to match
    மாறிலி string m_libname;  // Library name
    மாறிலி string m_base;  // Relative path of the libmap file

பொது:
    LibMapping(மாறிலி string& pattern_, மாறிலி string& libname_, மாறிலி string& base_)
        : m_pattern{pattern_}
        , m_libname{libname_}
        , m_base{base_} {}
    string pattern() மாறிலி { திரும்பு m_pattern; }
    string libname() மாறிலி { திரும்பு m_libname; }
    string base() மாறிலி { திரும்பு m_base; }
};

class V3LibMap final {
தனிப்பட்ட:
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
பொது:
    string matchMapping(மாறிலி string& filename);
    வெறுமை addPattern(மாறிலி string& pattern, மாறிலி string& libname, மாறிலி string& base);

    நிலையான வெறுமை map(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
