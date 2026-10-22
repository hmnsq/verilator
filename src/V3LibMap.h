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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3LIBMAP_H_
#وصف_ڪريو VERILATOR_V3LIBMAP_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ثابت string m_نمونو;  // Pattern to match
    ثابت string m_libname;  // Library name
    ثابت string m_بنياد;  // Relative path of the libmap file

عام:
    LibMapping(ثابت string& نمونو_3, ثابت string& libname_, ثابت string& بنياد_2)
        : m_نمونو{نمونو_3}
        , m_libname{libname_}
        , m_بنياد{بنياد_2} {}
    string نمونو_2() ثابت { أرجع m_نمونو; }
    string libname() ثابت { أرجع m_libname; }
    string بنياد() ثابت { أرجع m_بنياد; }
};

class V3LibMap final {
خاص:
    // STATE
    // 33.3.1.1 File path resolution
    //   If a file name potentially matches multiple file path specifications, the path
    //   specifications shall be resolved in the following order: a) File path specifications that
    //   end with an explicit file name b) File path specifications that end with a wildcarded file
    //   name c) File path specifications that end with a directory name
    std::ويڪٽر<LibMapping> m_explicitMappings;
    std::ويڪٽر<LibMapping> m_wildcardMappings;
    std::ويڪٽر<LibMapping> m_directoryMappings;

    // METHODS
عام:
    string ميلاپ_Mapping(ثابت string& filename);
    فراغ جوڙيو_نمونو(ثابت string& نمونو_2, ثابت string& libname, ثابت string& بنياد);

    ساكن فراغ map(AstNetlist* nodep) VL_MT_DISABLED;
};

#جيڪڏهن_پورو  // Guard
