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

#جے_بنا_تعریف VERILATOR_V3LIBMAP_H_
#تعریف_کرو VERILATOR_V3LIBMAP_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ثابت string m_نمونہ;  // Pattern to match
    ثابت string m_libname;  // Library name
    ثابت string m_ادھار;  // Relative path of the libmap file

عام:
    LibMapping(ثابت string& نمونہ_3, ثابت string& libname_, ثابت string& ادھار_2)
        : m_نمونہ{نمونہ_3}
        , m_libname{libname_}
        , m_ادھار{ادھار_2} {}
    string نمونہ_2() ثابت { أرجع m_نمونہ; }
    string libname() ثابت { أرجع m_libname; }
    string ادھار() ثابت { أرجع m_ادھار; }
};

class V3LibMap final {
خاص:
    // STATE
    // 33.3.1.1 File path resolution
    //   If a file name potentially matches multiple file path specifications, the path
    //   specifications shall be resolved in the following order: a) File path specifications that
    //   end with an explicit file name b) File path specifications that end with a wildcarded file
    //   name c) File path specifications that end with a directory name
    std::سمتیہ<LibMapping> m_explicitMappings;
    std::سمتیہ<LibMapping> m_wildcardMappings;
    std::سمتیہ<LibMapping> m_directoryMappings;

    // METHODS
عام:
    string میل_Mapping(ثابت string& filename);
    فراغ جوڑو_نمونہ(ثابت string& نمونہ_2, ثابت string& libname, ثابت string& ادھار);

    ساكن فراغ map(AstNetlist* nodep) VL_MT_DISABLED;
};

#جے_انت  // Guard
