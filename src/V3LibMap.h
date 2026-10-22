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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3LIBMAP_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3LIBMAP_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ਅਚਲ string m_ਨਮੂਨਾ;  // Pattern to match
    ਅਚਲ string m_libname;  // Library name
    ਅਚਲ string m_ਅਧਾਰ;  // Relative path of the libmap file

ਜਨਤਕ:
    LibMapping(ਅਚਲ string& ਨਮੂਨਾ_3, ਅਚਲ string& libname_, ਅਚਲ string& ਅਧਾਰ_2)
        : m_ਨਮੂਨਾ{ਨਮੂਨਾ_3}
        , m_libname{libname_}
        , m_ਅਧਾਰ{ਅਧਾਰ_2} {}
    string ਨਮੂਨਾ_2() ਅਚਲ { ਵਾਪਸ m_ਨਮੂਨਾ; }
    string libname() ਅਚਲ { ਵਾਪਸ m_libname; }
    string ਅਧਾਰ() ਅਚਲ { ਵਾਪਸ m_ਅਧਾਰ; }
};

class V3LibMap final {
ਨਿਜੀ:
    // STATE
    // 33.3.1.1 File path resolution
    //   If a file name potentially matches multiple file path specifications, the path
    //   specifications shall be resolved in the following order: a) File path specifications that
    //   end with an explicit file name b) File path specifications that end with a wildcarded file
    //   name c) File path specifications that end with a directory name
    std::ਸਦਿਸ਼<LibMapping> m_explicitMappings;
    std::ਸਦਿਸ਼<LibMapping> m_wildcardMappings;
    std::ਸਦਿਸ਼<LibMapping> m_directoryMappings;

    // METHODS
ਜਨਤਕ:
    string ਮੇਲ_Mapping(ਅਚਲ string& filename);
    ਖਾਲੀ ਜੋੜੋ_ਨਮੂਨਾ(ਅਚਲ string& ਨਮੂਨਾ_2, ਅਚਲ string& libname, ਅਚਲ string& ਅਧਾਰ);

    ਸਥਿਰ ਖਾਲੀ map(AstNetlist* nodep) VL_MT_DISABLED;
};

#ਜੇ_ਅੰਤ  // Guard
