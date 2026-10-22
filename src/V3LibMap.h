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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3LIBMAP_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3LIBMAP_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <vector>

class AstNetlist;
class VInFilter;
class V3ParseSym;

//============================================================================

// State to pass between config parsing and cell linking visitors.
class LibMapping final {
    ಸ್ಥಿರಾಂಕ string m_ನಮೂನೆ;  // Pattern to match
    ಸ್ಥಿರಾಂಕ string m_libname;  // Library name
    ಸ್ಥಿರಾಂಕ string m_ತಳ;  // Relative path of the libmap file

ಸಾರ್ವಜನಿಕ:
    LibMapping(ಸ್ಥಿರಾಂಕ string& ನಮೂನೆ_3, ಸ್ಥಿರಾಂಕ string& libname_, ಸ್ಥಿರಾಂಕ string& ತಳ_2)
        : m_ನಮೂನೆ{ನಮೂನೆ_3}
        , m_libname{libname_}
        , m_ತಳ{ತಳ_2} {}
    string ನಮೂನೆ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ನಮೂನೆ; }
    string libname() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libname; }
    string ತಳ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತಳ; }
};

class V3LibMap final {
ಖಾಸಗಿ:
    // STATE
    // 33.3.1.1 File path resolution
    //   If a file name potentially matches multiple file path specifications, the path
    //   specifications shall be resolved in the following order: a) File path specifications that
    //   end with an explicit file name b) File path specifications that end with a wildcarded file
    //   name c) File path specifications that end with a directory name
    std::ಸದಿಶ<LibMapping> m_explicitMappings;
    std::ಸದಿಶ<LibMapping> m_wildcardMappings;
    std::ಸದಿಶ<LibMapping> m_directoryMappings;

    // METHODS
ಸಾರ್ವಜನಿಕ:
    string ಹೊಂದಿಕೆ_Mapping(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ ಕೂಡಿಸು_ನಮೂನೆ(ಸ್ಥಿರಾಂಕ string& ನಮೂನೆ_2, ಸ್ಥಿರಾಂಕ string& libname, ಸ್ಥಿರಾಂಕ string& ತಳ);

    ಸ್ಥಿರ ಶೂನ್ಯ map(AstNetlist* nodep) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
