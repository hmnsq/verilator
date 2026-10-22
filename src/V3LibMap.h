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

#यदि_अपरिभाषितम् VERILATOR_V3LIBMAP_H_
#परिभाषय VERILATOR_V3LIBMAP_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ध्रुव string m_आदर्शः;  // Pattern to match
    ध्रुव string m_libname;  // Library name
    ध्रुव string m_आधारः;  // Relative path of the libmap file

सार्वजनिक:
    LibMapping(ध्रुव string& आदर्शः_3, ध्रुव string& libname_, ध्रुव string& आधारः_2)
        : m_आदर्शः{आदर्शः_3}
        , m_libname{libname_}
        , m_आधारः{आधारः_2} {}
    string आदर्शः_2() ध्रुव { प्रत्यागम m_आदर्शः; }
    string libname() ध्रुव { प्रत्यागम m_libname; }
    string आधारः() ध्रुव { प्रत्यागम m_आधारः; }
};

class V3LibMap final {
गुप्त:
    // STATE
    // 33.3.1.1 File path resolution
    //   If a file name potentially matches multiple file path specifications, the path
    //   specifications shall be resolved in the following order: a) File path specifications that
    //   end with an explicit file name b) File path specifications that end with a wildcarded file
    //   name c) File path specifications that end with a directory name
    std::सदिशम्<LibMapping> m_explicitMappings;
    std::सदिशम्<LibMapping> m_wildcardMappings;
    std::सदिशम्<LibMapping> m_directoryMappings;

    // METHODS
सार्वजनिक:
    string मेलनम्_Mapping(ध्रुव string& filename);
    शून्य सङ्कलय_आदर्शः(ध्रुव string& आदर्शः_2, ध्रुव string& libname, ध्रुव string& आधारः);

    स्थिर शून्य map(AstNetlist* nodep) VL_MT_DISABLED;
};

#यदि_अन्तः  // Guard
