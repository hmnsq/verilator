// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Command line options
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3OPTIONS_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3OPTIONS_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3LangCode.h"

#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <vector>

class V3OptionsImp;
class File_ಸಾಲು;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    ಎಣಿಕೆ en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    bool isDefault() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool is_ಗಣ_True() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == OPT_TRUE; }
    bool is_ಗಣ_False() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == OPT_FALSE; }
    ಶೂನ್ಯ ಗಣ_True_Or_False(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VOptionBool& lhs, ಸ್ಥಿರಾಂಕ VOptionBool& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VOptionBool& lhs, VOptionBool::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VOptionBool::en lhs, ಸ್ಥಿರಾಂಕ VOptionBool& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ಸ್ಥಿರಾಂಕ string m_filename;  // Filename
    ಸ್ಥಿರಾಂಕ string m_libname;  // Libname
ಸಾರ್ವಜನಿಕ:
    VFileLibName(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ಸ್ಥಿರಾಂಕ VFileLibName& rhs) = ಪೂರ್ವನಿಯೋಜಿತ;
    string filename() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_filename; }
    string libname() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libname; }
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VFileLibName& rhs) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ VFileLibName& rhs) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_filename < rhs.m_filename) ಹಿಂತಿರುಗಿ true;
        ಆದರೆ (m_filename > rhs.m_filename) ಹಿಂತಿರುಗಿ false;
        ಹಿಂತಿರುಗಿ m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::ಸದಿಶ<VFileLibName>;
using VFile_Lib_ಗಣ = std::ಗಣ<VFileLibName>;

// ######################################################################

class VTimescale final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        // clang-format off
        TS_100S = 0, TS_10S = 1, TS_1S = 2,
        TS_100MS = 3, TS_10MS = 4, TS_1MS = 5,
        TS_100US = 6, TS_10US = 7, TS_1US = 8,
        TS_100NS = 9, TS_10NS = 10, TS_1NS = 11,
        TS_100PS = 12, TS_10PS = 13, TS_1PS = 14,
        TS_100FS = 15, TS_10FS = 16, TS_1FS = 17,
        // clang-format on
        NONE = 18,
        ENUM_ಕೊನೆ
    };
    ಎಣಿಕೆ : uint8_t { TS_DEFAULT = TS_1PS };
    ಎಣಿಕೆ en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ, bool& badr);
    VTimescale(ದ್ವಿಗುಣ ಮೌಲ್ಯ, bool& badr) {
        badr = false;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = TS_100S; i < ENUM_ಕೊನೆ; ++i) {
            m_e = static_cast<en>(i);
            ಆದರೆ (multiplier() == ಮೌಲ್ಯ) ನಿಲ್ಲಿಸು;
        }
        ಆದರೆ (multiplier() != ಮೌಲ್ಯ) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ಸ್ಥಿರ ಶೂನ್ಯ parseSlashed(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, VTimescale& unitr, VTimescale& precr,
                             bool allow_ಖಾಲಿಗಳು = false);
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    ಪೂರ್ಣಾಂಕ power_ದ_Ten() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ 2 - static_cast<ಪೂರ್ಣಾಂಕ>(m_e); }
    ದ್ವಿಗುಣ multiplier() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ದ್ವಿಗುಣ ಮೌಲ್ಯಗಳು[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        ಹಿಂತಿರುಗಿ ಮೌಲ್ಯಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VTimescale& lhs, ಸ್ಥಿರಾಂಕ VTimescale& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VTimescale& lhs, VTimescale::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VTimescale::en lhs, ಸ್ಥಿರಾಂಕ VTimescale& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ VTimescale& lhs, ಸ್ಥಿರಾಂಕ VTimescale& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e > rhs.m_e;
}
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VTimescale& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t {
        DEFAULT = 0,  // Use the global default expansion type
        AUTO,  // Expands only reset and default
        AUTO_EXPAND,  // Expands transitions between all possible states
        ತುಂಬಿದ,  // Cover all possible transitions
    } m_e;
    VFsmExpandType()
        : m_e{DEFAULT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFsmExpandType(en _e)
        : m_e{_e} {}
    explicit VFsmExpandType(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ {
        constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VFsmExpandType& lhs, ಸ್ಥಿರಾಂಕ VFsmExpandType& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VFsmExpandType::en lhs, ಸ್ಥಿರಾಂಕ VFsmExpandType& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
}
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ VFsmExpandType& rhs) VL_MT_ಸುರಕ್ಷಿತ {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option final {
ಸಾರ್ವಜನಿಕ:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ಸ್ಥಿರಾಂಕ std::string, std::string>;

ಖಾಸಗಿ:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

ಸಾರ್ವಜನಿಕ:
    explicit V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option(ಸ್ಥಿರಾಂಕ string& optstring);
    ಸ್ಥಿರಾಂಕ string& origName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_origName; }
    ಸ್ಥಿರಾಂಕ string& mangledName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_mangledName; }
    ಸ್ಥಿರಾಂಕ ParamStrMap params() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_parameters; }
};

using V3_Hier_ಖಂಡ_Opt_ಗಣ = std::map<ಸ್ಥಿರಾಂಕ std::string, V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option>;

//######################################################################
// V3Options - Command line options

class V3Options final {
ಸಾರ್ವಜನಿಕ:
ಖಾಸಗಿ:
    // TYPES
    using DebugLevelMap = std::map<ಸ್ಥಿರಾಂಕ std::string, ಚಿಹ್ನೆರಹಿತ>;

    // MEMBERS (general options)
    V3OptionsImp* m_impp;  // Slow hidden options

    // clang-format off
    VString_ಗಣ m_cppFiles;     // argument: C++ files to link against
    VStringList m_cFlags;      // argument: user CFLAGS
    VStringList m_ldLibs;      // argument: user LDFLAGS
    VStringList m_ldLibFiles;  // argument: library files, subset of m_ldLibs
    VStringList m_makeFlags;   // argument: user MAKEFLAGS
    VString_ಗಣ m_compilerIncludes; // argument: user --compiler-include
    VString_ಗಣ m_futures;      // argument: -Wfuture- list
    VString_ಗಣ m_future0s;     // argument: -future list
    VString_ಗಣ m_future1s;     // argument: -future1 list
    VFile_Lib_ಗಣ m_libraryFiles; // argument: Verilog -v files
    VFileLibList m_vFiles;      // argument: Verilog files to read
    VFile_Lib_ಗಣ m_vltFiles;     // argument: Verilator config files to read
    VString_ಗಣ m_libmapFiles;  // argument: --libmap files to read
    VStringList m_forceIncs;   // argument: -FI
    DebugLevelMap m_debugLevel; // argument: --debugi-<srcfile/tag> <level>
    DebugLevelMap m_dumpLevel;  // argument: --dumpi-<srcfile/tag> <level>
    std::map<ಸ್ಥಿರಾಂಕ string, string> m_parameters;  // Parameters
    std::map<ಸ್ಥಿರಾಂಕ string, V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option> m_hier_ಖಂಡಗಳು;  // main switch: --hierarchical-block
    VString_ಗಣ m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_ಸಾಲು = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_ದ್ವಿಮಾನ = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_ಅಭಿವ್ಯಕ್ತಿ = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_ಸಾಲು = false;    // main switch: --coverage-block
    bool m_coverage_ವಿಲೀನ_Instances = false;  // main switch: --coverage-merge-instances
    bool m_coverage_ಪ್ರತಿ_Instance = false;  // main switch: --coverage-per-instance
    bool m_coverageToggle = false;  // main switch: --coverage-toggle
    bool m_coverageUnderscore = false;  // main switch: --coverage-underscore
    bool m_coverageUser = false;    // main switch: --coverage-func
    bool m_debugCheck = false;      // main switch: --debug-check
    bool m_debugCollision = false;  // main switch: --debug-collision
    bool m_debugEmitV = false;      // main switch: --debug-emitv
    bool m_debugExitElab = false;   // main switch: --debug-exit-elab
    bool m_debugExitParse = false;  // main switch: --debug-exit-parse
    bool m_debugLeak = true;        // main switch: --debug-leak
    bool m_debugNondeterminism = false;  // main switch: --debug-nondeterminism
    bool m_debugOptions = false;    // main switch: --debug-options
    bool m_debug_ವಿಭಜನೆ = false;  // main switch: --debug-partition
    bool m_debugPreprocPassthru = false;  // main switch: --debug-preproc-passthru
    bool m_debugProtect = false;    // main switch: --debug-protect
    bool m_debugSelfTest = false;   // main switch: --debug-self-test
    bool m_debug_ರಾಶಿ_Check = false;  // main switch: --debug-stack-check
    ಪೂರ್ಣಾಂಕ m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
    bool m_debug_ಅಗಲ = false;      // main switch: --debug-width
    bool m_decoration = true;       // main switch: --decoration
    bool m_decorationNodes = false;  // main switch: --decoration=nodes
    bool m_diagnosticsSarif = false;  // main switch: --diagnostics-sarif
    bool m_dpiHdrOnly = false;      // main switch: --dpi-hdr-only
    bool m_emitAccessors = false;   // main switch: --emit-accessors
    bool m_exe = false;             // main switch: --exe
    bool m_flatten = false;         // main switch: --flatten
    bool m_fourstate = false;       // main switch: --fourstate
    bool m_ಶ್ರೇಣೀಕೃತ = false;    // main switch: --hierarchical
    bool m_ignc = false;            // main switch: --ignc
    bool m_jsonOnly = false;        // main switch: --json-only
    bool m_lintOnly = false;        // main switch: --lint-only
    bool m_gmake = false;           // main switch: --make gmake
    bool m_makeJson = false;        // main switch: --make json
    bool m_main = false;            // main switch: --main
    bool m_ಹೊರಹೋಗು_Format_Ok = false;     // main switch: --cc, --sc or --sp was specified
    bool m_pedantic = false;        // main switch: --Wpedantic
    bool m_pinsInoutEnables = false;// main switch: --pins-inout-enables
    bool m_pinsScUint = false;      // main switch: --pins-sc-uint
    bool m_pinsScUintBool = false;  // main switch: --pins-sc-uint-bool
    bool m_pinsScBigUint = false;   // main switch: --pins-sc-biguint
    bool m_pinsUint8 = false;       // main switch: --pins-uint8
    bool m_preprocComments = false;  // main switch: --preproc-comments
    bool m_preproc_ನಿರೂಪಿಸುತ್ತದೆ = false;  // main switch: --preproc-defines
    bool m_profC = false;           // main switch: --prof-c
    bool m_profCFuncs = false;      // main switch: --prof-cfuncs
    bool m_profExec = false;        // main switch: --prof-exec
    bool m_profPgo = false;         // main switch: --prof-pgo
    bool m_protectIds = false;      // main switch: --protect-ids
    bool m_ಬಹಿರಂಗ = false;          // main switch: --public
    bool m_ಬಹಿರಂಗ_Flat_RW = false;    // main switch: --public-flat-rw
    bool m_ಬಹಿರಂಗ_Ignore = false;    // main switch: --public-ignore
    bool m_ಬಹಿರಂಗ_Params = false;    // main switch: --public-params
    bool m_quietBuild = false;      // main switch: --quiet-build
    bool m_quietExit = false;       // main switch: --quiet-exit
    bool m_quietStats = false;      // main switch: --quiet-stats
    bool m_relativeIncludes = false;  // main switch: --relative-includes
    bool m_reportUnoptflat = false;  // main switch: --report-unoptflat
    bool m_savable = false;         // main switch: --savable
    VOptionBool m_schedZeroDelay;  // main switch: --sched-zero-delay
    bool m_stdPackage = true;       // main switch: --std-package
    bool m_stdWaiver = true;        // main switch: --std-waiver
    bool m_systemC = false;         // main switch: --sc: System C instead of simple C++
    bool m_stats = false;           // main switch: --stats
    bool m_statsVars = false;       // main switch: --stats-vars
    bool m_ಎಳೆಗಳು_Coarsen = true;   // main switch: --threads-coarsen
    bool m_ಎಳೆಗಳು_Dpi_ಶುದ್ಧ = true;   // main switch: --threads-dpi all/pure
    bool m_ಎಳೆಗಳು_Dpi_Unpure = false;  // main switch: --threads-dpi all
    VOptionBool m_timing;           // main switch: --timing
    bool m_trace = false;           // main switch: --trace
    bool m_traceCoverage = false;   // main switch: --trace-coverage
    bool m_traceEnabledFst = false;  // main switch: --trace-fst
    bool m_traceEnabledSaif = false;  // main switch: --trace-saif
    bool m_traceEnabledVcd = false;  // main switch: --trace-vcd
    bool m_traceParams = true;      // main switch: --trace-params
    bool m_traceStructs = false;    // main switch: --trace-structs
    bool m_no_Trace_ತುದಿ = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_x_Initial_ಅಂಚು = false;    // main switch: --x-initial-edge

    ಪೂರ್ಣಾಂಕ         m_buildJobs = -1;    // main switch: --build-jobs, -j
    ಪೂರ್ಣಾಂಕ         m_coverage_ಅಭಿವ್ಯಕ್ತಿ_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    ಪೂರ್ಣಾಂಕ         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    ಪೂರ್ಣಾಂಕ         m_converge_ಎಲ್ಲೆ = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    ಪೂರ್ಣಾಂಕ         m_coverage_Max_ಅಗಲ = 256; // main switch: --coverage-max-width
    ಪೂರ್ಣಾಂಕ         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    ಪೂರ್ಣಾಂಕ         m_expand_ಎಲ್ಲೆ = 256;  // main switch: --expand-limit
    ಪೂರ್ಣಾಂಕ         m_gateStmts = 100;    // main switch: --gate-stmts
    ಪೂರ್ಣಾಂಕ         m_hier_ಮಗು = 0;      // main switch: --hierarchical-child
    ಪೂರ್ಣಾಂಕ         m_hier_ಎಳೆಗಳು = 0;      // main switch: --hierarchical-threads
    ಪೂರ್ಣಾಂಕ         m_if_ಆಳ = 0;      // main switch: --if-depth
    ಪೂರ್ಣಾಂಕ         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    ಪೂರ್ಣಾಂಕ         m_inline_CFuncs_ಗುಣಲಬ್ಧ = 200;   // main switch: --inline-cfuncs-product
    ಪೂರ್ಣಾಂಕ         m_inlineMult = 2000;   // main switch: --inline-mult
    ಪೂರ್ಣಾಂಕ         m_instr_ಎಣಿಕೆ_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    ಪೂರ್ಣಾಂಕ         m_localize_Max_ಗಾತ್ರ = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    ಪೂರ್ಣಾಂಕ         m_max_ಅಂಕೆ_ಅಗಲ = 65536;  // main switch: --max-num-width
    ಪೂರ್ಣಾಂಕ         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    ಪೂರ್ಣಾಂಕ         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    ಪೂರ್ಣಾಂಕ         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_output_Groups_ಗಾತ್ರ = 512 * 1024 * 1024;  // main switch: --output-groups-size
    ಪೂರ್ಣಾಂಕ         m_output_ವಿಭಜಿಸು = 20000;  // main switch: --output-split
    ಪೂರ್ಣಾಂಕ         m_output_ವಿಭಜಿಸು_CFuncs = -1;  // main switch: --output-split-cfuncs
    ಪೂರ್ಣಾಂಕ         m_output_ವಿಭಜಿಸು_CTrace = -1;  // main switch: --output-split-ctrace
    ಪೂರ್ಣಾಂಕ         m_pinsBv = 65;       // main switch: --pins-bv
    ಪೂರ್ಣಾಂಕ         m_preproc_Token_ಎಲ್ಲೆ = 40000; // main switch: --preproc-token-limit
    ಪೂರ್ಣಾಂಕ         m_ಬಹಿರಂಗ_ಆಳ = 0;   // main switch: --public-depth
    ಪೂರ್ಣಾಂಕ         m_reloop_ಎಲ್ಲೆ = 40; // main switch: --reloop-limit
    ಪೂರ್ಣಾಂಕ         m_replication_ಎಲ್ಲೆ = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    ಪೂರ್ಣಾಂಕ         m_ಎಳೆಗಳು = 1;      // main switch: --threads
    ಪೂರ್ಣಾಂಕ         m_ಎಳೆಗಳು_Max_MTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_ಸಮಯ_Default_Prec;  // main switch: --timescale
    VTimescale  m_ಸಮಯ_Default_Unit;  // main switch: --timescale
    VTimescale  m_ಸಮಯ_Override_Prec;  // main switch: --timescale-override
    VTimescale  m_ಸಮಯ_Override_Unit;  // main switch: --timescale-override
    ಪೂರ್ಣಾಂಕ         m_trace_ಆಳ = 0;   // main switch: --trace-depth
    ಪೂರ್ಣಾಂಕ         m_traceMaxArray = 32;  // main switch: --trace-max-array
    ಪೂರ್ಣಾಂಕ         m_trace_Max_ಅಗಲ = 4096; // main switch: --trace-max-width
    ಪೂರ್ಣಾಂಕ         m_unroll_ಎಣಿಕೆ = 64;  // main switch: --unroll-count
    ಪೂರ್ಣಾಂಕ         m_unroll_ಎಲ್ಲೆ = 16384;  // main switch: --unroll-limit
    ಪೂರ್ಣಾಂಕ         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    ಪೂರ್ಣಾಂಕ         m_constraint_Array_ಎಲ್ಲೆ = 64;  // main switch: --constraint-array-limit
    ಪೂರ್ಣಾಂಕ         m_verilateJobs = -1;  // main switch: --verilate-jobs

    ಪೂರ್ಣಾಂಕ         m_comp_ಎಲ್ಲೆ_ಖಂಡಗಳು = 0;  // compiler selection; number of nested blocks
    ಪೂರ್ಣಾಂಕ         m_comp_ಎಲ್ಲೆ_Members = 64;  // compiler selection; number of members in struct before make anon array
    ಪೂರ್ಣಾಂಕ         m_comp_ಎಲ್ಲೆ_Parens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_ತುದಿ_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_mod_ಪೂರ್ವಪ್ರತ್ಯಯ;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_ಪೂರ್ವಪ್ರತ್ಯಯ;       // main switch: --prefix
    string      m_protect_ಕೀಲಿ;   // main switch: --protect-key
    string      m_ತುದಿ_Module;    // main switch: --top-module
    string      m_ಬಳಸದ_Regexp; // main switch: --unused-regexp
    string      m_waiverOutput;  // main switch: --waiver-output {filename}
    string      m_work = "work";  // main switch: --work {libname}
    string      m_x_ನಿಯೋಜಿಸು;      // main switch: --x-assign
    string      m_xInitial;     // main switch: --x-initial

    // Language is now held in FileLine, on a per-node basis. However we still
    // have a concept of the default language at a global level.
    V3_Lang_ಸಂಕೇತ  m_defaultLanguage;      // main switch: --language

    // MEMBERS (optimizations)
    bool m_fAcycSimp;    // main switch: -fno-acyc-simp: acyclic pre-optimizations
    bool m_fAssemble;    // main switch: -fno-assemble: assign assemble
    bool m_f_ಬಿಟ್_ಪರಿಶೀಲನೆ_Loops;  // main switch: -fno-bit-scan-loops: convert bit scan loops to builtins
    bool m_fBitblast;    // main switch: -fno-bitblast: split packed variables into bit ranges
    bool m_fCaseDecoder; // main switch: -fno-case-decoder: case decoder conversion
    bool m_f_Case_ಕೋಷ್ಟಕ;   // main switch: -fno-case-table: case table conversion
    bool m_f_Case_ವೃಕ್ಷ;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_ಬಿಟ್_Op_ವೃಕ್ಷ;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels = true; // main switch: -fno-dfg-push-down-sels
    bool m_fDfg;         // main switch: -fno-dfg
    bool m_fDfgSynthesizeAll = false;  // main switch: -fdfg-synthesize-all
    bool m_fDeadAssigns;     // main switch: -fno-dead-assigns: remove dead assigns
    bool m_fDeadCells;   // main switch: -fno-dead-cells: remove dead cells
    bool m_fDeadMethods;   // main switch: -fno-dead-methods: remove dead methods
    bool m_fDecomposePacked;  // main switch: -fno-decompose-packed: packed decomposition
    bool m_fDecomposeUnpacked;  // main switch: -fno-decompose-unpacked: unpacked decomposition
    bool m_fExpand;      // main switch: -fno-expand: expansion of C macros
    bool m_fFuncBalanceCat = true;  // main switch: -fno-func-balance-cat: expansion of C macros
    bool m_f_Func_ವಿಭಜಿಸು_Cat = true;  // main switch: -fno-func-split-cat: expansion of C macros
    bool m_fGate;        // main switch: -fno-gate: gate wire elimination
    // main switch: -fno-ico-change-detect: input change detection optimization
    VOptionBool m_fIcoChangeDetect{VOptionBool::OPT_DEFAULT_TRUE};
    bool m_fInline;      // main switch: -fno-inline: module inlining
    bool m_fInlineCFuncs;  // main switch: -fno-inline-cfuncs: inline small C functions
    bool m_fInlineFuncs = true;  // main switch: -fno-inline-funcs: function inlining
    bool m_fInlineFuncsEager = true;  // main switch: -fno-inline-funcs-eager: don't inline eagerly
    bool m_fLife;        // main switch: -fno-life: variable lifetime
    bool m_fLifePost;    // main switch: -fno-life-post: delayed assignment elimination
    bool m_fLocalize;    // main switch: -fno-localize: convert temps to local variables
    bool m_f_ವಿಲೀನ_Cond;   // main switch: -fno-merge-cond: merge conditionals
    bool m_f_ವಿಲೀನ_Cond_Motion = true; // main switch: -fno-merge-cond-motion: perform code motion
    bool m_fReloop;      // main switch: -fno-reloop: reform loops
    bool m_fReorder;     // main switch: -fno-reorder: reorder assignments in blocks
    bool m_f_ತುಂಡು = true;  // main switch: -fno-slice: array assignment slicing
    ಪೂರ್ಣಾಂಕ  m_f_ತುಂಡು_Element_ಎಲ್ಲೆ = 256;  // main switch: --fslice-element-limit
    bool m_f_ವಿಭಜಿಸು;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_f_ಕೋಷ್ಟಕ;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_ಲಭ್ಯ = false;  // Set to true at the end of option parsing

    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_Arg(ಅಕ್ಷರ** argv, ಗಾತ್ರ_t ಎಣಿಕೆ_2, bool isForRerun);
    ಶೂನ್ಯ ಕೂಡಿಸು_Arg(ಸ್ಥಿರಾಂಕ std::string& arg, bool isForRerun);
    ಶೂನ್ಯ ಕೂಡಿಸು_ಸಾಲು_Arg(ಸ್ಥಿರಾಂಕ string& arg);
    ಶೂನ್ಯ ಕೂಡಿಸು_Define(ಸ್ಥಿರಾಂಕ string& defline, bool allowPlus) VL_MT_DISABLED;
    ಶೂನ್ಯ ಕೂಡಿಸು_Future(ಸ್ಥಿರಾಂಕ string& flag);
    ಶೂನ್ಯ ಕೂಡಿಸು_Future0(ಸ್ಥಿರಾಂಕ string& flag);
    ಶೂನ್ಯ ಕೂಡಿಸು_Future1(ಸ್ಥಿರಾಂಕ string& flag);
    ಶೂನ್ಯ ಕೂಡಿಸು_Inc_Dir_User(ಸ್ಥಿರಾಂಕ string& incdir);  // User requested
    ಶೂನ್ಯ ಕೂಡಿಸು_Inc_Dir_Fallback(ಸ್ಥಿರಾಂಕ string& incdir);  // Low priority if not found otherwise
    ಶೂನ್ಯ ಕೂಡಿಸು_Parameter(ಸ್ಥಿರಾಂಕ string& paramline, bool allowPlus);
    ಶೂನ್ಯ ಕೂಡಿಸು_Lang_Ext(ಸ್ಥಿರಾಂಕ string& langext, ಸ್ಥಿರಾಂಕ V3_Lang_ಸಂಕೇತ& lc);
    ಶೂನ್ಯ ಕೂಡಿಸು_Lib_Ext_V(ಸ್ಥಿರಾಂಕ string& libext);
    ಶೂನ್ಯ optimize(ಪೂರ್ಣಾಂಕ level);
    ಶೂನ್ಯ show_ಆವೃತ್ತಿ(bool verbose);
    ಶೂನ್ಯ coverage(bool flag) {
        m_coverage_ಸಾಲು = m_coverageBranch = m_coverageToggle = m_coverage_ಅಭಿವ್ಯಕ್ತಿ = m_coverageFsm
            = m_coverageUser = flag;
    }
    ಸ್ಥಿರ bool suffixed(ಸ್ಥಿರಾಂಕ string& sw, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* arg);
    ಸ್ಥಿರ string parseFileArg(ಸ್ಥಿರಾಂಕ string& optdir, ಸ್ಥಿರಾಂಕ string& relfilename);
    string file_ಮಾರ್ಗ_Check_One_Dir(ಸ್ಥಿರಾಂಕ string& modname, ಸ್ಥಿರಾಂಕ string& dirname);
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ strip_Options_For_ಮಗು_ಸತತ(ಸ್ಥಿರಾಂಕ string& opt, bool for_ತುದಿ);
    ಶೂನ್ಯ validateIdentifier(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& arg, ಸ್ಥಿರಾಂಕ string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

ಸಾರ್ವಜನಿಕ:
    V3Options();
    ~V3Options();
    ಶೂನ್ಯ ಗಣ_Debug_ಬಹುಲಕ(ಪೂರ್ಣಾಂಕ level);
    ಚಿಹ್ನೆರಹಿತ debugLevel(ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    ಚಿಹ್ನೆರಹಿತ debugSrcLevel(ಸ್ಥಿರಾಂಕ string& srcfile_ಮಾರ್ಗ) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    ಚಿಹ್ನೆರಹಿತ dumpLevel(ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    ಚಿಹ್ನೆರಹಿತ dumpSrcLevel(ಸ್ಥಿರಾಂಕ string& srcfile_ಮಾರ್ಗ) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;

    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_Cpp_File(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ ಕೂಡಿಸು_CFlags(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ ಕೂಡಿಸು_Compiler_Includes(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ ಕೂಡಿಸು_Ld_Lib_File(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ ಕೂಡಿಸು_Ld_Libs(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ ಕೂಡಿಸು_Make_Flags(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ ಕೂಡಿಸು_Lib_Map_File(ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ ಕೂಡಿಸು_Library_File(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname);
    ಶೂನ್ಯ ಕೂಡಿಸು_VFile(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname);
    ಶೂನ್ಯ ಕೂಡಿಸು_Vlt_File(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname);
    ಶೂನ್ಯ ಕೂಡಿಸು_Force_Inc(ಸ್ಥಿರಾಂಕ string& filename);
    bool ಲಭ್ಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಲಭ್ಯ; }
    ಶೂನ್ಯ cc_ಗಣ();
    ಶೂನ್ಯ decorations(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& filename);
    ಶೂನ್ಯ notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_preprocOnly; }
    bool makePhony() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_makePhony; }
    bool preproc_No_ಸಾಲು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_preproc_No_ಸಾಲು; }
    bool preprocResolve() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_preprocResolve; }
    ಪೂರ್ಣಾಂಕ preproc_Token_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_preproc_Token_ಎಲ್ಲೆ; }
    bool underlineZero() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_underlineZero; }
    bool systemC() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_systemC; }
    bool savable() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_savable; }
    VOptionBool schedZeroDelay() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_schedZeroDelay; }
    bool stats() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stats; }
    bool statsVars() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_statsVars; }
    bool stdPackage() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stdPackage; }
    bool stdWaiver() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stdWaiver; }
    bool assertOn() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_assertCase; }
    bool autoflush() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_autoflush; }
    bool bboxSys() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bboxSys; }
    bool bboxUnsup() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bboxUnsup; }
    bool build() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_build; }
    string buildDepBin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_buildDepBin; }
    ಶೂನ್ಯ buildDepBin(ಸ್ಥಿರಾಂಕ string& flag) { m_buildDepBin = flag; }
    bool context() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_context; }
    bool coverage() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        ಹಿಂತಿರುಗಿ m_coverage_ಸಾಲು || m_coverageBranch || m_coverageToggle || m_coverage_ಅಭಿವ್ಯಕ್ತಿ
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        ಹಿಂತಿರುಗಿ m_coverage_ಸಾಲು || m_coverageBranch || m_coverageToggle || m_coverage_ಅಭಿವ್ಯಕ್ತಿ
               || m_coverageUser;
    }
    bool coverageBranch() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageBranch; }
    bool coverage_ಅಭಿವ್ಯಕ್ತಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverage_ಅಭಿವ್ಯಕ್ತಿ; }
    bool coverageFsm() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageFsmExpand; }
    ಪೂರ್ಣಾಂಕ coverageFsmMaxArcs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageFsmMaxArcs; }
    bool coverage_ಸಾಲು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverage_ಸಾಲು; }
    bool coverage_ವಿಲೀನ_Instances() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverage_ವಿಲೀನ_Instances; }
    bool coverage_ಪ್ರತಿ_Instance() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverage_ಪ್ರತಿ_Instance; }
    bool coverageToggle() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageToggle; }
    bool coverageUnderscore() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageUnderscore; }
    bool coverageUser() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageUser; }
    bool debugCheck() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_debugCheck; }
    bool debugCollision() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debugCollision; }
    bool debugEmitV() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_debugEmitV; }
    bool debugExitElab() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debugExitElab; }
    bool debugExitParse() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debugExitParse; }
    bool debugLeak() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debugLeak; }
    bool debugNondeterminism() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debugNondeterminism; }
    bool debug_ವಿಭಜನೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debug_ವಿಭಜನೆ; }
    bool debugPreprocPassthru() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_debugPreprocPassthru; }
    bool debugProtect() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_debugProtect; }
    ಪೂರ್ಣಾಂಕ debugRuntimeTimeout() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debugRuntimeTimeout; }
    bool debugSelfTest() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debugSelfTest; }
    bool debug_ರಾಶಿ_Check() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debug_ರಾಶಿ_Check; }
    bool debug_ಅಗಲ() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ m_debug_ಅಗಲ; }
    bool decoration() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_decoration; }
    bool decorationNodes() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_decorationNodes; }
    bool diagnosticsSarif() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_diagnosticsSarif; }
    bool dpiHdrOnly() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dpiHdrOnly; }
    bool dumpAstPatterns() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_dumpLevel.ಎಣಿಕೆ_2("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dump_ನಿರೂಪಿಸುತ್ತದೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_dumpLevel.ಎಣಿಕೆ_2("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_dumpLevel.ಎಣಿಕೆ_2("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_ವೃಕ್ಷ_Dot() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_dumpLevel.ಎಣಿಕೆ_2("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_emitAccessors; }
    bool exe() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_exe; }
    bool flatten() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_flatten; }
    bool fourstate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fourstate; }
    bool gmake() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_gmake; }
    bool makeJson() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_makeJson; }
    bool ಎಳೆಗಳು_Dpi_ಶುದ್ಧ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಳೆಗಳು_Dpi_ಶುದ್ಧ; }
    bool ಎಳೆಗಳು_Dpi_Unpure() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಳೆಗಳು_Dpi_Unpure; }
    bool ಎಳೆಗಳು_Coarsen() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಳೆಗಳು_Coarsen; }
    VOptionBool timing() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_timing; }
    bool trace() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_trace; }
    bool traceCoverage() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceCoverage; }
    bool traceEnabledFst() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceEnabledFst; }
    bool traceEnabledSaif() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceEnabledSaif; }
    bool traceEnabledVcd() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceEnabledVcd; }
    bool traceParams() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceParams; }
    bool traceStructs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceStructs; }
    bool traceUnderscore() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceUnderscore; }
    bool ಮುಖ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_main; }
    bool ಹೊರಹೋಗು_Format_Ok() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಹೊರಹೋಗು_Format_Ok; }
    bool jsonOnly() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_jsonOnly; }
    bool keep_ತಾತ್ಕಾಲಿಕ_Files() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ (V3Error::debugDefault() != 0); }
    bool pedantic() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pedantic; }
    bool pinsInoutEnables() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pinsInoutEnables; }
    bool pinsScUint() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pinsScUint; }
    bool pinsScUintBool() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pinsScUintBool; }
    bool pinsScBigUint() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_pinsScBigUint; }
    bool pinsUint8() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pinsUint8; }
    bool preprocComments() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_preprocComments; }
    bool preproc_ನಿರೂಪಿಸುತ್ತದೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_preproc_ನಿರೂಪಿಸುತ್ತದೆ; }
    bool profC() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_profC; }
    bool profCFuncs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_profCFuncs; }
    bool profExec() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_profExec; }
    bool profPgo() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_profPgo; }
    bool ಬಳಸುತ್ತದೆ_Profiler() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ profExec() || profPgo(); }
    bool protectIds() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_protectIds; }
    bool all_ಬಹಿರಂಗ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಹಿರಂಗ; }
    bool ಬಹಿರಂಗ_Params() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಹಿರಂಗ_Params; }
    bool ಬಹಿರಂಗ_Off() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಹಿರಂಗ_Ignore; }
    bool ಬಹಿರಂಗ_Flat_RW() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಹಿರಂಗ_Flat_RW; }
    ಪೂರ್ಣಾಂಕ ಬಹಿರಂಗ_ಆಳ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಹಿರಂಗ_ಆಳ; }
    bool any_ಬಹಿರಂಗ_Flat() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಹಿರಂಗ_Params || m_ಬಹಿರಂಗ_Flat_RW || m_ಬಹಿರಂಗ_ಆಳ; }
    bool lintOnly() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_lintOnly; }
    bool ignc() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ignc; }
    bool quietBuild() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_quietBuild; }
    bool quietExit() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_quietExit; }
    bool quietStats() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_quietStats; }
    bool reportUnoptflat() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_reportUnoptflat; }
    bool verilate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_verilate; }
    bool vpi() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vpi.isTrue(); }
    bool waiverMultiline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_waiverMultiline; }
    bool x_Initial_ಅಂಚು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_x_Initial_ಅಂಚು; }
    bool serializeOnly() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_jsonOnly; }
    bool ತುದಿ_Ifaces_Supported() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ lintOnly() && !ಶ್ರೇಣೀಕೃತ(); }

    ಪೂರ್ಣಾಂಕ buildJobs() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_buildJobs; }
    ಪೂರ್ಣಾಂಕ converge_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_converge_ಎಲ್ಲೆ; }
    ಪೂರ್ಣಾಂಕ coverage_ಅಭಿವ್ಯಕ್ತಿ_Max() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverage_ಅಭಿವ್ಯಕ್ತಿ_Max; }
    uint32_t coverageMaxBins() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverageMaxRealBins; }
    ಪೂರ್ಣಾಂಕ coverage_Max_ಅಗಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_coverage_Max_ಅಗಲ; }
    ಪೂರ್ಣಾಂಕ debugAllocRandom() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_debugAllocRandom; }
    bool dump_ವೃಕ್ಷ_Addrids() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    ಪೂರ್ಣಾಂಕ expand_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_expand_ಎಲ್ಲೆ; }
    ಪೂರ್ಣಾಂಕ gateStmts() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_gateStmts; }
    ಪೂರ್ಣಾಂಕ if_ಆಳ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_if_ಆಳ; }
    ಪೂರ್ಣಾಂಕ inlineCFuncs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_inlineCFuncs; }
    ಪೂರ್ಣಾಂಕ inline_CFuncs_ಗುಣಲಬ್ಧ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_inline_CFuncs_ಗುಣಲಬ್ಧ; }
    ಪೂರ್ಣಾಂಕ inlineMult() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_inlineMult; }
    ಪೂರ್ಣಾಂಕ instr_ಎಣಿಕೆ_Dpi() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_instr_ಎಣಿಕೆ_Dpi; }
    ಪೂರ್ಣಾಂಕ localize_Max_ಗಾತ್ರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_localize_Max_ಗಾತ್ರ; }
    bool jsonEditNums() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_jsonEditNums; }
    bool jsonIds() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_jsonIds; }
    VOptionBool makeDepend() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_makeDepend; }
    ಪೂರ್ಣಾಂಕ max_ಅಂಕೆ_ಅಗಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_max_ಅಂಕೆ_ಅಗಲ; }
    ಪೂರ್ಣಾಂಕ func_Recursion_ಆಳ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_funcRecursion; }
    ಪೂರ್ಣಾಂಕ module_Recursion_ಆಳ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_moduleRecursion; }
    ಪೂರ್ಣಾಂಕ output_ವಿಭಜಿಸು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_output_ವಿಭಜಿಸು; }
    ಪೂರ್ಣಾಂಕ output_ವಿಭಜಿಸು_CFuncs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_output_ವಿಭಜಿಸು_CFuncs; }
    ಪೂರ್ಣಾಂಕ output_ವಿಭಜಿಸು_CTrace() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_output_ವಿಭಜಿಸು_CTrace; }
    ಪೂರ್ಣಾಂಕ outputGroups() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_outputGroups; }
    int64_t output_Groups_ಗಾತ್ರ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_output_Groups_ಗಾತ್ರ; }
    ಪೂರ್ಣಾಂಕ pinsBv() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_pinsBv; }
    ಪೂರ್ಣಾಂಕ reloop_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_reloop_ಎಲ್ಲೆ; }
    ಪೂರ್ಣಾಂಕ replication_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_replication_ಎಲ್ಲೆ; }
    VOptionBool skipIdentical() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_skipIdentical; }
    bool stopFail() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stopFail; }
    ಪೂರ್ಣಾಂಕ ಎಳೆಗಳು() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಎಳೆಗಳು; }
    ಪೂರ್ಣಾಂಕ ಎಳೆಗಳು_Max_MTasks() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಎಳೆಗಳು_Max_MTasks; }
    bool mtasks() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ (m_ಎಳೆಗಳು > 1); }
    VTimescale ಸಮಯ_Default_Prec() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಮಯ_Default_Prec; }
    VTimescale ಸಮಯ_Default_Unit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಮಯ_Default_Unit; }
    VTimescale ಸಮಯ_Override_Prec() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಮಯ_Override_Prec; }
    VTimescale ಸಮಯ_Override_Unit() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಮಯ_Override_Unit; }
    VTimescale ಸಮಯ_Compute_Prec(ಸ್ಥಿರಾಂಕ VTimescale& flag) ಸ್ಥಿರಾಂಕ;
    VTimescale ಸಮಯ_Compute_Unit(ಸ್ಥಿರಾಂಕ VTimescale& flag) ಸ್ಥಿರಾಂಕ;
    ಪೂರ್ಣಾಂಕ trace_ಆಳ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_trace_ಆಳ; }
    ಪೂರ್ಣಾಂಕ traceMaxArray() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_traceMaxArray; }
    ಪೂರ್ಣಾಂಕ trace_Max_ಅಗಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_trace_Max_ಅಗಲ; }
    bool useTraceParallel() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ trace() && traceEnabledVcd() && (ಎಳೆಗಳು() > 1 || hier_ಮಗು() > 1);
    }
    ಪೂರ್ಣಾಂಕ unroll_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unroll_ಎಣಿಕೆ; }
    ಪೂರ್ಣಾಂಕ unroll_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unroll_ಎಲ್ಲೆ; }
    ಪೂರ್ಣಾಂಕ unrollStmts() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_unrollStmts; }
    ಪೂರ್ಣಾಂಕ constraint_Array_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_constraint_Array_ಎಲ್ಲೆ; }
    ಪೂರ್ಣಾಂಕ verilateJobs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_verilateJobs; }

    ಪೂರ್ಣಾಂಕ comp_ಎಲ್ಲೆ_ಖಂಡಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_comp_ಎಲ್ಲೆ_ಖಂಡಗಳು; }
    ಪೂರ್ಣಾಂಕ comp_ಎಲ್ಲೆ_Members() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_comp_ಎಲ್ಲೆ_Members; }
    ಪೂರ್ಣಾಂಕ comp_ಎಲ್ಲೆ_Parens() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_comp_ಎಲ್ಲೆ_Parens; }

    string diagnosticsSarifOutput() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_diagnosticsSarifOutput.ಖಾಲಿಗಳು() ? makeDir() + "/" + ಪೂರ್ವಪ್ರತ್ಯಯ() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_exeName != "" ? m_exeName : ಪೂರ್ವಪ್ರತ್ಯಯ(); }
    VFileLibList hierParamFile() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hierParamsFile; }
    string jsonOnlyOutput() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_jsonOnlyMetaOutput; }
    string l2Name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_l2Name; }
    string libCreate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libCreate; }
    string libCreateName(bool shared) ಸ್ಥಿರಾಂಕ {
        string libName = "lib" + libCreate();
        ಆದರೆ (shared) {
            libName += ".so";
        } ಇಲ್ಲದಿದ್ದರೆ {
            libName += ".a";
        }
        ಹಿಂತಿರುಗಿ libName;
    }
    string main_ತುದಿ_Name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_main_ತುದಿ_Name; }
    string makeDir() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_makeDir; }
    string mod_ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_mod_ಪೂರ್ವಪ್ರತ್ಯಯ; }
    string pipeFilter() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_pipeFilter; }
    string ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಪೂರ್ವಪ್ರತ್ಯಯ; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_ಕೀಲಿ_Provided() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_protect_ಕೀಲಿ.ಖಾಲಿಗಳು(); }
    string protect_ಕೀಲಿ_Defaulted() VL_MT_ಸುರಕ್ಷಿತ;  // Set default key if not set by user
    string ತುದಿ_Module() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ತುದಿ_Module; }  // As AstNode::encodeName()
    bool no_Trace_ತುದಿ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_no_Trace_ತುದಿ; }
    string ಬಳಸದ_Regexp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸದ_Regexp; }
    string waiverOutput() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_waiverOutput; }
    string work() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_work; }
    bool isWaiverOutput() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ !m_waiverOutput.ಖಾಲಿಗಳು(); }
    string x_ನಿಯೋಜಿಸು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_x_ನಿಯೋಜಿಸು; }
    string xInitial() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_xInitial; }

    ಸ್ಥಿರಾಂಕ VString_ಗಣ& cppFiles() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cppFiles; }
    ಸ್ಥಿರಾಂಕ VStringList& cFlags() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_cFlags; }
    ಸ್ಥಿರಾಂಕ VString_ಗಣ& compilerIncludes() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_compilerIncludes; }
    ಸ್ಥಿರಾಂಕ VStringList& ldLibFiles() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ldLibFiles; }
    ಸ್ಥಿರಾಂಕ VStringList& ldLibs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ldLibs; }
    ಸ್ಥಿರಾಂಕ VStringList& makeFlags() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_makeFlags; }
    ಸ್ಥಿರಾಂಕ VFile_Lib_ಗಣ& libraryFiles() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libraryFiles; }
    ಸ್ಥಿರಾಂಕ VString_ಗಣ& libmapFiles() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_libmapFiles; }
    ಸ್ಥಿರಾಂಕ VFileLibList& vFiles() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vFiles; }
    ಸ್ಥಿರಾಂಕ VFile_Lib_ಗಣ& vltFiles() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vltFiles; }
    ಸ್ಥಿರಾಂಕ VStringList& forceIncs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_forceIncs; }

    bool hasParameter(ಸ್ಥಿರಾಂಕ string& name);
    string parameter(ಸ್ಥಿರಾಂಕ string& name);
    ಶೂನ್ಯ checkParameters();

    bool isFuture(ಸ್ಥಿರಾಂಕ string& flag) ಸ್ಥಿರಾಂಕ;
    bool isFuture0(ಸ್ಥಿರಾಂಕ string& flag) ಸ್ಥಿರಾಂಕ;
    bool isFuture1(ಸ್ಥಿರಾಂಕ string& flag) ಸ್ಥಿರಾಂಕ;
    bool isLibraryFile(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname) ಸ್ಥಿರಾಂಕ;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fAcycSimp; }
    bool fAssemble() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fAssemble; }
    bool f_ಬಿಟ್_ಪರಿಶೀಲನೆ_Loops() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_ಬಿಟ್_ಪರಿಶೀಲನೆ_Loops; }
    bool fBitblast() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fBitblast; }
    bool fCaseDecoder() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fCaseDecoder; }
    bool f_Case_ಕೋಷ್ಟಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_Case_ಕೋಷ್ಟಕ; }
    bool f_Case_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_Case_ವೃಕ್ಷ; }
    bool fCombine() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fCombine; }
    bool fConst() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fConst; }
    bool fConstBeforeDfg() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fConstBeforeDfg; }
    bool f_Const_ಬಿಟ್_Op_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_Const_ಬಿಟ್_Op_ವೃಕ್ಷ; }
    bool fConstEager() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fConstEager; }
    bool fDedupe() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDedupe; }
    bool fDfg() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDfg; }
    bool fDfgPeephole() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDfgPeephole; }
    bool f_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels; }
    bool fDfgSynthesizeAll() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ಸ್ಥಿರಾಂಕ std::string& name) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ !m_fDfgPeepholeDisabled.ಎಣಿಕೆ_2(name);
    }
    bool fDeadAssigns() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDeadAssigns; }
    bool fDeadCells() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDeadCells; }
    bool fDeadMethods() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDeadMethods; }
    bool fDecompose() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDecomposePacked; }
    bool fDecomposeUnpacked() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fDecomposeUnpacked; }
    bool fExpand() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fExpand; }
    bool fFuncBalanceCat() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fFuncBalanceCat; }
    bool f_Func_ವಿಭಜಿಸು_Cat() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_Func_ವಿಭಜಿಸು_Cat; }
    bool fFunc() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ f_Func_ವಿಭಜಿಸು_Cat() || fFuncBalanceCat(); }
    bool fGate() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fGate; }
    VOptionBool fIcoChangeDetect() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fIcoChangeDetect; }
    bool fInline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fInline; }
    bool fInlineCFuncs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fInlineCFuncs; }
    bool fInlineFuncs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fInlineFuncs; }
    bool fInlineFuncsEager() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fInlineFuncsEager; }
    bool fLife() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fLife; }
    bool fLifePost() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fLifePost; }
    bool fLocalize() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fLocalize; }
    bool f_ವಿಲೀನ_Cond() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_ವಿಲೀನ_Cond; }
    bool f_ವಿಲೀನ_Cond_Motion() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_ವಿಲೀನ_Cond_Motion; }
    bool fReloop() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fReloop; }
    bool fReorder() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fReorder; }
    bool f_ತುಂಡು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_ತುಂಡು; }
    ಪೂರ್ಣಾಂಕ f_ತುಂಡು_Element_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_ತುಂಡು_Element_ಎಲ್ಲೆ; }
    bool f_ವಿಭಜಿಸು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_ವಿಭಜಿಸು; }
    bool fSubst() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fSubst; }
    bool fSubstConst() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fSubstConst; }
    bool f_ಕೋಷ್ಟಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_f_ಕೋಷ್ಟಕ; }
    bool fTaskifyAll() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fTaskifyAll; }

    std::string trace_Class_ತಳ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;  // Deprecated
    std::string traceClassLang() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;  // Deprecated
    std::ಸದಿಶ<std::string> traceClassBases() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    std::ಸದಿಶ<std::string> traceClassLangs() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    std::ಸದಿಶ<std::string> trace_ಮೂಲ_Bases() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    std::ಸದಿಶ<std::string> trace_ಮೂಲ_Langs() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;

    bool ಶ್ರೇಣೀಕೃತ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಶ್ರೇಣೀಕೃತ; }
    ಪೂರ್ಣಾಂಕ hier_ಮಗು() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_hier_ಮಗು; }
    ಪೂರ್ಣಾಂಕ hier_ಎಳೆಗಳು() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_hier_ಎಳೆಗಳು == 0 ? m_ಎಳೆಗಳು : m_hier_ಎಳೆಗಳು; }
    bool hier_ತುದಿ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ !m_hier_ಮಗು && !m_hier_ಖಂಡಗಳು.ಖಾಲಿಗಳು(); }
    ಸ್ಥಿರಾಂಕ V3_Hier_ಖಂಡ_Opt_ಗಣ& hier_ಖಂಡಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hier_ಖಂಡಗಳು; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_ತುದಿ_ದತ್ತಾಂಶ_Dir() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ hier_ತುದಿ() ? (makeDir() + '/' + ಪೂರ್ವಪ್ರತ್ಯಯ() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ಸ್ಥಿರ string ಆವೃತ್ತಿ() VL_ಶುದ್ಧ;
    ಸ್ಥಿರ string argString(ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ** argv);  ///< Return list of arguments as simple string
    string all_ವಾದಗಳು_String() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;  ///< Return all passed arguments as simple string
    ಸ್ಥಿರಾಂಕ std::list<std::pair<std::list<std::string>, bool>>& all_ವಾದಗಳು() ಸ್ಥಿರಾಂಕ;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string all_ವಾದಗಳು_String_For_Hier_ಖಂಡ(bool for_ತುದಿ) ಸ್ಥಿರಾಂಕ;
    ಶೂನ್ಯ parseOpts(File_ಸಾಲು* fl, ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ** argv) VL_MT_DISABLED;
    ಶೂನ್ಯ parseOptsList(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& optdir, ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ** argv) VL_MT_DISABLED;
    ಶೂನ್ಯ parseOptsFile(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    ಸ್ಥಿರ string getenvBuiltins(ಸ್ಥಿರಾಂಕ string& var);
    ಸ್ಥಿರ string getenvMAKE();
    ಸ್ಥಿರ string getenvMAKEFLAGS();
    ಸ್ಥಿರ string getenvPERL();
    ಸ್ಥಿರ string getenvPYTHON3();
    ಸ್ಥಿರ string getenvSYSTEMC();
    ಸ್ಥಿರ string getenvSYSTEMC_ARCH();
    ಸ್ಥಿರ string getenvSYSTEMC_INCLUDE();
    ಸ್ಥಿರ string getenvSYSTEMC_LIBDIR();
    ಸ್ಥಿರ string getenv_VERILATOR_ಬೇರು();
    ಸ್ಥಿರ string getenvVERILATOR_SOLVER();
    ಸ್ಥಿರ string get_Std_Package_ಮಾರ್ಗ();
    ಸ್ಥಿರ string get_Std_Waiver_ಮಾರ್ಗ();
    ಸ್ಥಿರ string getSupported(ಸ್ಥಿರಾಂಕ string& var);
    ಸ್ಥಿರ bool systemCSystemWide();
    ಸ್ಥಿರ bool systemCFound();  // SystemC installed, or environment points to it
    ಸ್ಥಿರ bool coroutineSupport();  // Configured C++ compiler supports coroutines
    ಸ್ಥಿರ bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    ಸ್ಥಿರ bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    ಸ್ಥಿರ bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ಸ್ಥಿರಾಂಕ string& filename);
    string file_ಮಾರ್ಗ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& modname, ಸ್ಥಿರಾಂಕ string& lastpath,
                    ಸ್ಥಿರಾಂಕ string& errmsg);
    string file_ಮಾರ್ಗ_Looked_ಸಂದೇಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& modname);
    V3_Lang_ಸಂಕೇತ fileLanguage(ಸ್ಥಿರಾಂಕ string& filename);
    ಸ್ಥಿರ bool file_Stat_ಸಾಮಾನ್ಯ(ಸ್ಥಿರಾಂಕ string& filename);

    // METHODS (other OS)
    ಸ್ಥಿರ ಶೂನ್ಯ throwSigsegv();
};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
