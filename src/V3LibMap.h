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

#که_نه_تعریف_شوی VERILATOR_V3LIBMAP_H_
#تعریف_کړه VERILATOR_V3LIBMAP_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ثابت string m_نمونه;  // Pattern to match
    ثابت string m_libname;  // Library name
    ثابت string m_بنسټ;  // Relative path of the libmap file

عام:
    LibMapping(ثابت string& نمونه_3, ثابت string& libname_, ثابت string& بنسټ_2)
        : m_نمونه{نمونه_3}
        , m_libname{libname_}
        , m_بنسټ{بنسټ_2} {}
    string نمونه_2() ثابت { أرجع m_نمونه; }
    string libname() ثابت { أرجع m_libname; }
    string بنسټ() ثابت { أرجع m_بنسټ; }
};

class V3LibMap final {
خاص:
    // STATE
    // 33.3.1.1 File path resolution
    //   If a file name potentially matches multiple file path specifications, the path
    //   specifications shall be resolved in the following order: a) File path specifications that
    //   end with an explicit file name b) File path specifications that end with a wildcarded file
    //   name c) File path specifications that end with a directory name
    std::ویکټور<LibMapping> m_explicitMappings;
    std::ویکټور<LibMapping> m_wildcardMappings;
    std::ویکټور<LibMapping> m_directoryMappings;

    // METHODS
عام:
    string سمون_Mapping(ثابت string& filename);
    فراغ جمع_کړه_نمونه(ثابت string& نمونه_2, ثابت string& libname, ثابت string& بنسټ);

    ساكن فراغ map(AstNetlist* nodep) VL_MT_DISABLED;
};

#د_که_پای  // Guard
