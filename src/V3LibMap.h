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

#اگر_غیر_تعریف_شدہ VERILATOR_V3LIBMAP_H_
#تعریف_کرو VERILATOR_V3LIBMAP_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ثابت string m_نمونہ;  // Pattern to match
    ثابت string m_libname;  // Library name
    ثابت string m_بنیاد;  // Relative path of the libmap file

عوامی:
    LibMapping(ثابت string& نمونہ_3, ثابت string& libname_, ثابت string& بنیاد_2)
        : m_نمونہ{نمونہ_3}
        , m_libname{libname_}
        , m_بنیاد{بنیاد_2} {}
    string نمونہ_2() ثابت { واپس m_نمونہ; }
    string libname() ثابت { واپس m_libname; }
    string بنیاد() ثابت { واپس m_بنیاد; }
};

class V3LibMap final {
نجی:
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
عوامی:
    string مطابقت_Mapping(ثابت string& filename);
    خالی جوڑو_نمونہ(ثابت string& نمونہ_2, ثابت string& libname, ثابت string& بنیاد);

    ساکن خالی map(AstNetlist* nodep) VL_MT_DISABLED;
};

#اگر_ختم  // Guard
