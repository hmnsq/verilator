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

#अगर_अघोषित VERILATOR_V3LIBMAP_H_
#घोषणा VERILATOR_V3LIBMAP_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    स्थिर string m_pattern;  // Pattern to match
    स्थिर string m_libname;  // Library name
    स्थिर string m_base;  // Relative path of the libmap file

खुला:
    LibMapping(स्थिर string& pattern_, स्थिर string& libname_, स्थिर string& base_)
        : m_pattern{pattern_}
        , m_libname{libname_}
        , m_base{base_} {}
    string pattern() स्थिर { वापस m_pattern; }
    string libname() स्थिर { वापस m_libname; }
    string base() स्थिर { वापस m_base; }
};

class V3LibMap final {
निजी:
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
खुला:
    string matchMapping(स्थिर string& filename);
    व्योम addPattern(स्थिर string& pattern, स्थिर string& libname, स्थिर string& base);

    अटल व्योम map(AstNetlist* nodep) VL_MT_DISABLED;
};

#पूर्ण_अगर  // Guard
