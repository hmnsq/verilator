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

#ifndef VERILATOR_V3OPTIONS_H_
#define VERILATOR_V3OPTIONS_H_

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
    ಗಣನಾ en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    ಗಣನಾ en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    bool isDefault() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == OPT_TRUE; }
    bool isSetFalse() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == OPT_FALSE; }
    ಶೂನ್ಯ setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(ಧ್ರುವ VOptionBool& lhs, ಧ್ರುವ VOptionBool& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VOptionBool& lhs, VOptionBool::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, ಧ್ರುವ VOptionBool& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ಧ್ರುವ string m_filename;  // Filename
    ಧ್ರುವ string m_libname;  // Libname
ಸಾರ್ವಜನಿಕ:
    VFileLibName(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ಧ್ರುವ VFileLibName& rhs) = ಸ್ವಭಾವ;
    string filename() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_filename; }
    string libname() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libname; }
    bool operator==(ಧ್ರುವ VFileLibName& rhs) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(ಧ್ರುವ VFileLibName& rhs) ಧ್ರುವ {
        ಯದಿ (m_filename < rhs.m_filename) ಪ್ರತ್ಯಾಗಮ true;
        ಯದಿ (m_filename > rhs.m_filename) ಪ್ರತ್ಯಾಗಮ false;
        ಪ್ರತ್ಯಾಗಮ m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        // clang-format off
        TS_100S = 0, TS_10S = 1, TS_1S = 2,
        TS_100MS = 3, TS_10MS = 4, TS_1MS = 5,
        TS_100US = 6, TS_10US = 7, TS_1US = 8,
        TS_100NS = 9, TS_10NS = 10, TS_1NS = 11,
        TS_100PS = 12, TS_10PS = 13, TS_1PS = 14,
        TS_100FS = 15, TS_10FS = 16, TS_1FS = 17,
        // clang-format on
        NONE = 18,
        _ENUM_END
    };
    ಗಣನಾ : uint8_t { TS_DEFAULT = TS_1PS };
    ಗಣನಾ en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ಧ್ರುವ string& ಮೌಲ್ಯ, bool& badr);
    VTimescale(ದ್ವಿಗುಣ ಮೌಲ್ಯ, bool& badr) {
        badr = false;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            ಯದಿ (multiplier() == ಮೌಲ್ಯ) ಭಙ್ಗ;
        }
        ಯದಿ (multiplier() != ಮೌಲ್ಯ) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ಸ್ಥಿರ ಶೂನ್ಯ parseSlashed(File_ಸಾಲು* fl, ಧ್ರುವ ವರ್ಣ* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    ಪೂರ್ಣಾಙ್ಕ powerOfTen() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ 2 - static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_e); }
    ದ್ವಿಗುಣ multiplier() ಧ್ರುವ {
        ಸ್ಥಿರ ಧ್ರುವ ದ್ವಿಗುಣ ಮೌಲ್ಯಗಳು[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯಗಳು[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VTimescale& lhs, ಧ್ರುವ VTimescale& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VTimescale& lhs, VTimescale::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, ಧ್ರುವ VTimescale& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(ಧ್ರುವ VTimescale& lhs, ಧ್ರುವ VTimescale& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e > rhs.m_e;
}
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VTimescale& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t {
        DEFAULT = 0,  // Use the global default expansion type
        AUTO,  // Expands only reset and default
        AUTO_EXPAND,  // Expands transitions between all possible states
        FULL,  // Cover all possible transitions
    } m_e;
    VFsmExpandType()
        : m_e{DEFAULT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFsmExpandType(en _e)
        : m_e{_e} {}
    explicit VFsmExpandType(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    constexpr ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ {
        constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
};
constexpr bool operator==(ಧ್ರುವ VFsmExpandType& lhs, ಧ್ರುವ VFsmExpandType& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, ಧ್ರುವ VFsmExpandType& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
}
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ VFsmExpandType& rhs) VL_MT_SAFE {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
ಸಾರ್ವಜನಿಕ:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ಧ್ರುವ std::string, std::string>;

ಗುಪ್ತ:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

ಸಾರ್ವಜನಿಕ:
    explicit V3HierarchicalBlockOption(ಧ್ರುವ string& optstring);
    ಧ್ರುವ string& origName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_origName; }
    ಧ್ರುವ string& mangledName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_mangledName; }
    ಧ್ರುವ ParamStrMap params() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_parameters; }
};

using V3HierBlockOptSet = std::map<ಧ್ರುವ std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
ಸಾರ್ವಜನಿಕ:
ಗುಪ್ತ:
    // TYPES
    using DebugLevelMap = std::map<ಧ್ರುವ std::string, unsigned>;

    // MEMBERS (general options)
    V3OptionsImp* m_impp;  // Slow hidden options

    // clang-format off
    VStringSet m_cppFiles;     // argument: C++ files to link against
    VStringList m_cFlags;      // argument: user CFLAGS
    VStringList m_ldLibs;      // argument: user LDFLAGS
    VStringList m_ldLibFiles;  // argument: library files, subset of m_ldLibs
    VStringList m_makeFlags;   // argument: user MAKEFLAGS
    VStringSet m_compilerIncludes; // argument: user --compiler-include
    VStringSet m_futures;      // argument: -Wfuture- list
    VStringSet m_future0s;     // argument: -future list
    VStringSet m_future1s;     // argument: -future1 list
    VFileLibSet m_libraryFiles; // argument: Verilog -v files
    VFileLibList m_vFiles;      // argument: Verilog files to read
    VFileLibSet m_vltFiles;     // argument: Verilator config files to read
    VStringSet m_libmapFiles;  // argument: --libmap files to read
    VStringList m_forceIncs;   // argument: -FI
    DebugLevelMap m_debugLevel; // argument: --debugi-<srcfile/tag> <level>
    DebugLevelMap m_dumpLevel;  // argument: --dumpi-<srcfile/tag> <level>
    std::map<ಧ್ರುವ string, string> m_parameters;  // Parameters
    std::map<ಧ್ರುವ string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

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
    bool m_coverageMergeInstances = false;  // main switch: --coverage-merge-instances
    bool m_coveragePerInstance = false;  // main switch: --coverage-per-instance
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
    bool m_debugPartition = false;  // main switch: --debug-partition
    bool m_debugPreprocPassthru = false;  // main switch: --debug-preproc-passthru
    bool m_debugProtect = false;    // main switch: --debug-protect
    bool m_debugSelfTest = false;   // main switch: --debug-self-test
    bool m_debug_ರಾಶಿ_Check = false;  // main switch: --debug-stack-check
    ಪೂರ್ಣಾಙ್ಕ m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
    bool m_debugWidth = false;      // main switch: --debug-width
    bool m_decoration = true;       // main switch: --decoration
    bool m_decorationNodes = false;  // main switch: --decoration=nodes
    bool m_diagnosticsSarif = false;  // main switch: --diagnostics-sarif
    bool m_dpiHdrOnly = false;      // main switch: --dpi-hdr-only
    bool m_emitAccessors = false;   // main switch: --emit-accessors
    bool m_exe = false;             // main switch: --exe
    bool m_flatten = false;         // main switch: --flatten
    bool m_fourstate = false;       // main switch: --fourstate
    bool m_hierarchical = false;    // main switch: --hierarchical
    bool m_ignc = false;            // main switch: --ignc
    bool m_jsonOnly = false;        // main switch: --json-only
    bool m_lintOnly = false;        // main switch: --lint-only
    bool m_gmake = false;           // main switch: --make gmake
    bool m_makeJson = false;        // main switch: --make json
    bool m_main = false;            // main switch: --main
    bool m_outFormatOk = false;     // main switch: --cc, --sc or --sp was specified
    bool m_pedantic = false;        // main switch: --Wpedantic
    bool m_pinsInoutEnables = false;// main switch: --pins-inout-enables
    bool m_pinsScUint = false;      // main switch: --pins-sc-uint
    bool m_pinsScUintBool = false;  // main switch: --pins-sc-uint-bool
    bool m_pinsScBigUint = false;   // main switch: --pins-sc-biguint
    bool m_pinsUint8 = false;       // main switch: --pins-uint8
    bool m_preprocComments = false;  // main switch: --preproc-comments
    bool m_preprocDefines = false;  // main switch: --preproc-defines
    bool m_profC = false;           // main switch: --prof-c
    bool m_profCFuncs = false;      // main switch: --prof-cfuncs
    bool m_profExec = false;        // main switch: --prof-exec
    bool m_profPgo = false;         // main switch: --prof-pgo
    bool m_protectIds = false;      // main switch: --protect-ids
    bool m_public = false;          // main switch: --public
    bool m_publicFlatRW = false;    // main switch: --public-flat-rw
    bool m_publicIgnore = false;    // main switch: --public-ignore
    bool m_publicParams = false;    // main switch: --public-params
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
    bool m_threadsCoarsen = true;   // main switch: --threads-coarsen
    bool m_threadsDpiPure = true;   // main switch: --threads-dpi all/pure
    bool m_threadsDpiUnpure = false;  // main switch: --threads-dpi all
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
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    ಪೂರ್ಣಾಙ್ಕ         m_buildJobs = -1;    // main switch: --build-jobs, -j
    ಪೂರ್ಣಾಙ್ಕ         m_coverage_ಅಭಿವ್ಯಕ್ತಿ_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    ಪೂರ್ಣಾಙ್ಕ         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    ಪೂರ್ಣಾಙ್ಕ         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    ಪೂರ್ಣಾಙ್ಕ         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    ಪೂರ್ಣಾಙ್ಕ         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    ಪೂರ್ಣಾಙ್ಕ         m_expandLimit = 256;  // main switch: --expand-limit
    ಪೂರ್ಣಾಙ್ಕ         m_gateStmts = 100;    // main switch: --gate-stmts
    ಪೂರ್ಣಾಙ್ಕ         m_hierChild = 0;      // main switch: --hierarchical-child
    ಪೂರ್ಣಾಙ್ಕ         m_hierThreads = 0;      // main switch: --hierarchical-threads
    ಪೂರ್ಣಾಙ್ಕ         m_ifDepth = 0;      // main switch: --if-depth
    ಪೂರ್ಣಾಙ್ಕ         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    ಪೂರ್ಣಾಙ್ಕ         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    ಪೂರ್ಣಾಙ್ಕ         m_inlineMult = 2000;   // main switch: --inline-mult
    ಪೂರ್ಣಾಙ್ಕ         m_instr_ಎಣಿಕೆ_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    ಪೂರ್ಣಾಙ್ಕ         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    ಪೂರ್ಣಾಙ್ಕ         m_max_ಅಂಕೆ_Width = 65536;  // main switch: --max-num-width
    ಪೂರ್ಣಾಙ್ಕ         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    ಪೂರ್ಣಾಙ್ಕ         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    ಪೂರ್ಣಾಙ್ಕ         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    ಪೂರ್ಣಾಙ್ಕ         m_outputSplit = 20000;  // main switch: --output-split
    ಪೂರ್ಣಾಙ್ಕ         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    ಪೂರ್ಣಾಙ್ಕ         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    ಪೂರ್ಣಾಙ್ಕ         m_pinsBv = 65;       // main switch: --pins-bv
    ಪೂರ್ಣಾಙ್ಕ         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    ಪೂರ್ಣಾಙ್ಕ         m_publicDepth = 0;   // main switch: --public-depth
    ಪೂರ್ಣಾಙ್ಕ         m_reloopLimit = 40; // main switch: --reloop-limit
    ಪೂರ್ಣಾಙ್ಕ         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    ಪೂರ್ಣಾಙ್ಕ         m_threads = 1;      // main switch: --threads
    ಪೂರ್ಣಾಙ್ಕ         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    ಪೂರ್ಣಾಙ್ಕ         m_traceDepth = 0;   // main switch: --trace-depth
    ಪೂರ್ಣಾಙ್ಕ         m_traceMaxArray = 32;  // main switch: --trace-max-array
    ಪೂರ್ಣಾಙ್ಕ         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    ಪೂರ್ಣಾಙ್ಕ         m_unroll_ಎಣಿಕೆ = 64;  // main switch: --unroll-count
    ಪೂರ್ಣಾಙ್ಕ         m_unrollLimit = 16384;  // main switch: --unroll-limit
    ಪೂರ್ಣಾಙ್ಕ         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    ಪೂರ್ಣಾಙ್ಕ         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    ಪೂರ್ಣಾಙ್ಕ         m_verilateJobs = -1;  // main switch: --verilate-jobs

    ಪೂರ್ಣಾಙ್ಕ         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    ಪೂರ್ಣಾಙ್ಕ         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    ಪೂರ್ಣಾಙ್ಕ         m_compLimitParens = 240;  // compiler selection; number of nested parens

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
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_ಕೀಲಿ;   // main switch: --protect-key
    string      m_ತುದಿ_Module;    // main switch: --top-module
    string      m_unusedRegexp; // main switch: --unused-regexp
    string      m_waiverOutput;  // main switch: --waiver-output {filename}
    string      m_work = "work";  // main switch: --work {libname}
    string      m_xAssign;      // main switch: --x-assign
    string      m_xInitial;     // main switch: --x-initial

    // Language is now held in FileLine, on a per-node basis. However we still
    // have a concept of the default language at a global level.
    V3LangCode  m_defaultLanguage;      // main switch: --language

    // MEMBERS (optimizations)
    bool m_fAcycSimp;    // main switch: -fno-acyc-simp: acyclic pre-optimizations
    bool m_fAssemble;    // main switch: -fno-assemble: assign assemble
    bool m_fBitScanLoops;  // main switch: -fno-bit-scan-loops: convert bit scan loops to builtins
    bool m_fBitblast;    // main switch: -fno-bitblast: split packed variables into bit ranges
    bool m_fCaseDecoder; // main switch: -fno-case-decoder: case decoder conversion
    bool m_fCaseTable;   // main switch: -fno-case-table: case table conversion
    bool m_f_Case_ವೃಕ್ಷ;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_ವೃಕ್ಷ;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ತಳ್ಳು_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
    bool m_fDfg;         // main switch: -fno-dfg
    bool m_fDfgSynthesizeAll = false;  // main switch: -fdfg-synthesize-all
    bool m_fDeadAssigns;     // main switch: -fno-dead-assigns: remove dead assigns
    bool m_fDeadCells;   // main switch: -fno-dead-cells: remove dead cells
    bool m_fDeadMethods;   // main switch: -fno-dead-methods: remove dead methods
    bool m_fDecomposePacked;  // main switch: -fno-decompose-packed: packed decomposition
    bool m_fDecomposeUnpacked;  // main switch: -fno-decompose-unpacked: unpacked decomposition
    bool m_fExpand;      // main switch: -fno-expand: expansion of C macros
    bool m_fFuncBalanceCat = true;  // main switch: -fno-func-balance-cat: expansion of C macros
    bool m_fFuncSplitCat = true;  // main switch: -fno-func-split-cat: expansion of C macros
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
    bool m_fMergeCond;   // main switch: -fno-merge-cond: merge conditionals
    bool m_fMergeCondMotion = true; // main switch: -fno-merge-cond-motion: perform code motion
    bool m_fReloop;      // main switch: -fno-reloop: reform loops
    bool m_fReorder;     // main switch: -fno-reorder: reorder assignments in blocks
    bool m_fSlice = true;  // main switch: -fno-slice: array assignment slicing
    ಪೂರ್ಣಾಙ್ಕ  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    ಶೂನ್ಯ addArg(ವರ್ಣ** argv, size_t ಎಣಿಕೆ, bool isForRerun);
    ಶೂನ್ಯ addArg(ಧ್ರುವ std::string& arg, bool isForRerun);
    ಶೂನ್ಯ add_ಸಾಲು_Arg(ಧ್ರುವ string& arg);
    ಶೂನ್ಯ addDefine(ಧ್ರುವ string& defline, bool allowPlus) VL_MT_DISABLED;
    ಶೂನ್ಯ addFuture(ಧ್ರುವ string& flag);
    ಶೂನ್ಯ addFuture0(ಧ್ರುವ string& flag);
    ಶೂನ್ಯ addFuture1(ಧ್ರುವ string& flag);
    ಶೂನ್ಯ addIncDirUser(ಧ್ರುವ string& incdir);  // User requested
    ಶೂನ್ಯ addIncDirFallback(ಧ್ರುವ string& incdir);  // Low priority if not found otherwise
    ಶೂನ್ಯ addParameter(ಧ್ರುವ string& paramline, bool allowPlus);
    ಶೂನ್ಯ addLangExt(ಧ್ರುವ string& langext, ಧ್ರುವ V3LangCode& lc);
    ಶೂನ್ಯ addLibExtV(ಧ್ರುವ string& libext);
    ಶೂನ್ಯ optimize(ಪೂರ್ಣಾಙ್ಕ level);
    ಶೂನ್ಯ showVersion(bool verbose);
    ಶೂನ್ಯ coverage(bool flag) {
        m_coverage_ಸಾಲು = m_coverageBranch = m_coverageToggle = m_coverage_ಅಭಿವ್ಯಕ್ತಿ = m_coverageFsm
            = m_coverageUser = flag;
    }
    ಸ್ಥಿರ bool suffixed(ಧ್ರುವ string& sw, ಧ್ರುವ ವರ್ಣ* arg);
    ಸ್ಥಿರ string parseFileArg(ಧ್ರುವ string& optdir, ಧ್ರುವ string& relfilename);
    string filePathCheckOneDir(ಧ್ರುವ string& modname, ಧ್ರುವ string& dirname);
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ stripOptionsForChildRun(ಧ್ರುವ string& opt, bool for_ತುದಿ);
    ಶೂನ್ಯ validateIdentifier(File_ಸಾಲು* fl, ಧ್ರುವ string& arg, ಧ್ರುವ string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

ಸಾರ್ವಜನಿಕ:
    V3Options();
    ~V3Options();
    ಶೂನ್ಯ setDebugMode(ಪೂರ್ಣಾಙ್ಕ level);
    unsigned debugLevel(ಧ್ರುವ string& tag) ಧ್ರುವ VL_MT_SAFE;
    unsigned debugSrcLevel(ಧ್ರುವ string& srcfile_path) ಧ್ರುವ VL_MT_SAFE;
    unsigned dumpLevel(ಧ್ರುವ string& tag) ಧ್ರುವ VL_MT_SAFE;
    unsigned dumpSrcLevel(ಧ್ರುವ string& srcfile_path) ಧ್ರುವ VL_MT_SAFE;

    // METHODS
    ಶೂನ್ಯ addCppFile(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ addCFlags(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ addCompilerIncludes(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ addLdLibFile(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ addLdLibs(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ addMakeFlags(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ addLibMapFile(ಧ್ರುವ string& filename);
    ಶೂನ್ಯ addLibraryFile(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname);
    ಶೂನ್ಯ addVFile(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname);
    ಶೂನ್ಯ addVltFile(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname);
    ಶೂನ್ಯ addForceInc(ಧ್ರುವ string& filename);
    bool available() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_available; }
    ಶೂನ್ಯ ccSet();
    ಶೂನ್ಯ decorations(File_ಸಾಲು* fl, ಧ್ರುವ string& filename);
    ಶೂನ್ಯ notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_preprocOnly; }
    bool makePhony() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_makePhony; }
    bool preproc_No_ಸಾಲು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_preproc_No_ಸಾಲು; }
    bool preprocResolve() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_preprocResolve; }
    ಪೂರ್ಣಾಙ್ಕ preprocTokenLimit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_preprocTokenLimit; }
    bool underlineZero() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_underlineZero; }
    bool systemC() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_systemC; }
    bool savable() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_savable; }
    VOptionBool schedZeroDelay() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_schedZeroDelay; }
    bool stats() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stats; }
    bool statsVars() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_statsVars; }
    bool stdPackage() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stdPackage; }
    bool stdWaiver() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stdWaiver; }
    bool assertOn() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_assertCase; }
    bool autoflush() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_autoflush; }
    bool bboxSys() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bboxSys; }
    bool bboxUnsup() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bboxUnsup; }
    bool build() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_build; }
    string buildDepBin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_buildDepBin; }
    ಶೂನ್ಯ buildDepBin(ಧ್ರುವ string& flag) { m_buildDepBin = flag; }
    bool context() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_context; }
    bool coverage() ಧ್ರುವ VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        ಪ್ರತ್ಯಾಗಮ m_coverage_ಸಾಲು || m_coverageBranch || m_coverageToggle || m_coverage_ಅಭಿವ್ಯಕ್ತಿ
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ಧ್ರುವ VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        ಪ್ರತ್ಯಾಗಮ m_coverage_ಸಾಲು || m_coverageBranch || m_coverageToggle || m_coverage_ಅಭಿವ್ಯಕ್ತಿ
               || m_coverageUser;
    }
    bool coverageBranch() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageBranch; }
    bool coverage_ಅಭಿವ್ಯಕ್ತಿ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverage_ಅಭಿವ್ಯಕ್ತಿ; }
    bool coverageFsm() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageFsmExpand; }
    ಪೂರ್ಣಾಙ್ಕ coverageFsmMaxArcs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageFsmMaxArcs; }
    bool coverage_ಸಾಲು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverage_ಸಾಲು; }
    bool coverageMergeInstances() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageMergeInstances; }
    bool coveragePerInstance() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coveragePerInstance; }
    bool coverageToggle() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageToggle; }
    bool coverageUnderscore() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageUnderscore; }
    bool coverageUser() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageUser; }
    bool debugCheck() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_debugCheck; }
    bool debugCollision() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugCollision; }
    bool debugEmitV() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_debugEmitV; }
    bool debugExitElab() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugExitElab; }
    bool debugExitParse() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugExitParse; }
    bool debugLeak() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugLeak; }
    bool debugNondeterminism() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugNondeterminism; }
    bool debugPartition() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugPartition; }
    bool debugPreprocPassthru() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_debugPreprocPassthru; }
    bool debugProtect() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_debugProtect; }
    ಪೂರ್ಣಾಙ್ಕ debugRuntimeTimeout() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugRuntimeTimeout; }
    bool debugSelfTest() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugSelfTest; }
    bool debug_ರಾಶಿ_Check() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debug_ರಾಶಿ_Check; }
    bool debugWidth() ಧ್ರುವ VL_PURE { ಪ್ರತ್ಯಾಗಮ m_debugWidth; }
    bool decoration() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_decoration; }
    bool decorationNodes() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_decorationNodes; }
    bool diagnosticsSarif() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_diagnosticsSarif; }
    bool dpiHdrOnly() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dpiHdrOnly; }
    bool dumpAstPatterns() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_dumpLevel.ಎಣಿಕೆ("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dumpLevel.ಎಣಿಕೆ("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_dumpLevel.ಎಣಿಕೆ("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_ವೃಕ್ಷ_Dot() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_dumpLevel.ಎಣಿಕೆ("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_emitAccessors; }
    bool exe() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_exe; }
    bool flatten() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_flatten; }
    bool fourstate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fourstate; }
    bool gmake() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_gmake; }
    bool makeJson() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_makeJson; }
    bool threadsDpiPure() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_threadsDpiPure; }
    bool threadsDpiUnpure() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_threadsDpiUnpure; }
    bool threadsCoarsen() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_threadsCoarsen; }
    VOptionBool timing() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timing; }
    bool trace() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_trace; }
    bool traceCoverage() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceCoverage; }
    bool traceEnabledFst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceEnabledFst; }
    bool traceEnabledSaif() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceEnabledSaif; }
    bool traceEnabledVcd() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceEnabledVcd; }
    bool traceParams() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceParams; }
    bool traceStructs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceStructs; }
    bool traceUnderscore() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceUnderscore; }
    bool main() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_main; }
    bool outFormatOk() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outFormatOk; }
    bool jsonOnly() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_jsonOnly; }
    bool keepTempFiles() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ (V3Error::debugDefault() != 0); }
    bool pedantic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pedantic; }
    bool pinsInoutEnables() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pinsInoutEnables; }
    bool pinsScUint() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pinsScUint; }
    bool pinsScUintBool() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pinsScUintBool; }
    bool pinsScBigUint() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_pinsScBigUint; }
    bool pinsUint8() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pinsUint8; }
    bool preprocComments() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_preprocComments; }
    bool preprocDefines() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_preprocDefines; }
    bool profC() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_profC; }
    bool profCFuncs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_profCFuncs; }
    bool profExec() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_profExec; }
    bool profPgo() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_profPgo; }
    bool usesProfiler() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ profExec() || profPgo(); }
    bool protectIds() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_protectIds; }
    bool allPublic() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_public; }
    bool publicParams() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_publicParams; }
    bool publicOff() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_publicIgnore; }
    bool publicFlatRW() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_publicFlatRW; }
    ಪೂರ್ಣಾಙ್ಕ publicDepth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_publicDepth; }
    bool anyPublicFlat() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_lintOnly; }
    bool ignc() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ignc; }
    bool quietBuild() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_quietBuild; }
    bool quietExit() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_quietExit; }
    bool quietStats() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_quietStats; }
    bool reportUnoptflat() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_reportUnoptflat; }
    bool verilate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_verilate; }
    bool vpi() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vpi.isTrue(); }
    bool waiverMultiline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_waiverMultiline; }
    bool xInitialEdge() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_xInitialEdge; }
    bool serializeOnly() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_jsonOnly; }
    bool ತುದಿ_Ifaces_Supported() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ lintOnly() && !hierarchical(); }

    ಪೂರ್ಣಾಙ್ಕ buildJobs() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_buildJobs; }
    ಪೂರ್ಣಾಙ್ಕ convergeLimit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_convergeLimit; }
    ಪೂರ್ಣಾಙ್ಕ coverage_ಅಭಿವ್ಯಕ್ತಿ_Max() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverage_ಅಭಿವ್ಯಕ್ತಿ_Max; }
    uint32_t coverageMaxBins() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageMaxRealBins; }
    ಪೂರ್ಣಾಙ್ಕ coverageMaxWidth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_coverageMaxWidth; }
    ಪೂರ್ಣಾಙ್ಕ debugAllocRandom() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_debugAllocRandom; }
    bool dump_ವೃಕ್ಷ_Addrids() ಧ್ರುವ VL_MT_SAFE;
    ಪೂರ್ಣಾಙ್ಕ expandLimit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_expandLimit; }
    ಪೂರ್ಣಾಙ್ಕ gateStmts() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_gateStmts; }
    ಪೂರ್ಣಾಙ್ಕ ifDepth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ifDepth; }
    ಪೂರ್ಣಾಙ್ಕ inlineCFuncs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inlineCFuncs; }
    ಪೂರ್ಣಾಙ್ಕ inlineCFuncsProduct() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inlineCFuncsProduct; }
    ಪೂರ್ಣಾಙ್ಕ inlineMult() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_inlineMult; }
    ಪೂರ್ಣಾಙ್ಕ instr_ಎಣಿಕೆ_Dpi() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_instr_ಎಣಿಕೆ_Dpi; }
    ಪೂರ್ಣಾಙ್ಕ localizeMaxSize() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_localizeMaxSize; }
    bool jsonEditNums() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_jsonEditNums; }
    bool jsonIds() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_jsonIds; }
    VOptionBool makeDepend() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_makeDepend; }
    ಪೂರ್ಣಾಙ್ಕ max_ಅಂಕೆ_Width() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_max_ಅಂಕೆ_Width; }
    ಪೂರ್ಣಾಙ್ಕ funcRecursionDepth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_funcRecursion; }
    ಪೂರ್ಣಾಙ್ಕ moduleRecursionDepth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_moduleRecursion; }
    ಪೂರ್ಣಾಙ್ಕ outputSplit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outputSplit; }
    ಪೂರ್ಣಾಙ್ಕ outputSplitCFuncs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outputSplitCFuncs; }
    ಪೂರ್ಣಾಙ್ಕ outputSplitCTrace() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outputSplitCTrace; }
    ಪೂರ್ಣಾಙ್ಕ outputGroups() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outputGroups; }
    int64_t outputGroupsSize() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_outputGroupsSize; }
    ಪೂರ್ಣಾಙ್ಕ pinsBv() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_pinsBv; }
    ಪೂರ್ಣಾಙ್ಕ reloopLimit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_reloopLimit; }
    ಪೂರ್ಣಾಙ್ಕ replicationLimit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_replicationLimit; }
    VOptionBool skipIdentical() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_skipIdentical; }
    bool stopFail() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stopFail; }
    ಪೂರ್ಣಾಙ್ಕ threads() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_threads; }
    ಪೂರ್ಣಾಙ್ಕ threadsMaxMTasks() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_threadsMaxMTasks; }
    bool mtasks() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ (m_threads > 1); }
    VTimescale timeDefaultPrec() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeDefaultUnit; }
    VTimescale timeOverridePrec() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeOverridePrec; }
    VTimescale timeOverrideUnit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_timeOverrideUnit; }
    VTimescale timeComputePrec(ಧ್ರುವ VTimescale& flag) ಧ್ರುವ;
    VTimescale timeComputeUnit(ಧ್ರುವ VTimescale& flag) ಧ್ರುವ;
    ಪೂರ್ಣಾಙ್ಕ traceDepth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceDepth; }
    ಪೂರ್ಣಾಙ್ಕ traceMaxArray() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceMaxArray; }
    ಪೂರ್ಣಾಙ್ಕ traceMaxWidth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_traceMaxWidth; }
    bool useTraceParallel() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    ಪೂರ್ಣಾಙ್ಕ unroll_ಎಣಿಕೆ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unroll_ಎಣಿಕೆ; }
    ಪೂರ್ಣಾಙ್ಕ unrollLimit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unrollLimit; }
    ಪೂರ್ಣಾಙ್ಕ unrollStmts() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unrollStmts; }
    ಪೂರ್ಣಾಙ್ಕ constraintArrayLimit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constraintArrayLimit; }
    ಪೂರ್ಣಾಙ್ಕ verilateJobs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_verilateJobs; }

    ಪೂರ್ಣಾಙ್ಕ compLimitBlocks() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_compLimitBlocks; }
    ಪೂರ್ಣಾಙ್ಕ compLimitMembers() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_compLimitMembers; }
    ಪೂರ್ಣಾಙ್ಕ compLimitParens() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_compLimitParens; }

    string diagnosticsSarifOutput() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hierParamsFile; }
    string jsonOnlyOutput() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_jsonOnlyMetaOutput; }
    string l2Name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_l2Name; }
    string libCreate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libCreate; }
    string libCreateName(bool shared) ಧ್ರುವ {
        string libName = "lib" + libCreate();
        ಯದಿ (shared) {
            libName += ".so";
        } ಅನ್ಯಥಾ {
            libName += ".a";
        }
        ಪ್ರತ್ಯಾಗಮ libName;
    }
    string main_ತುದಿ_Name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_main_ತುದಿ_Name; }
    string makeDir() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_makeDir; }
    string modPrefix() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_modPrefix; }
    string pipeFilter() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_pipeFilter; }
    string prefix() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_ಕೀಲಿ_Provided() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !m_protect_ಕೀಲಿ.empty(); }
    string protect_ಕೀಲಿ_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string ತುದಿ_Module() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ತುದಿ_Module; }  // As AstNode::encodeName()
    bool no_Trace_ತುದಿ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_no_Trace_ತುದಿ; }
    string unusedRegexp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_unusedRegexp; }
    string waiverOutput() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_waiverOutput; }
    string work() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_work; }
    bool isWaiverOutput() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ !m_waiverOutput.empty(); }
    string xAssign() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_xAssign; }
    string xInitial() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_xInitial; }

    ಧ್ರುವ VStringSet& cppFiles() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cppFiles; }
    ಧ್ರುವ VStringList& cFlags() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_cFlags; }
    ಧ್ರುವ VStringSet& compilerIncludes() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_compilerIncludes; }
    ಧ್ರುವ VStringList& ldLibFiles() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ldLibFiles; }
    ಧ್ರುವ VStringList& ldLibs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ldLibs; }
    ಧ್ರುವ VStringList& makeFlags() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_makeFlags; }
    ಧ್ರುವ VFileLibSet& libraryFiles() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libraryFiles; }
    ಧ್ರುವ VStringSet& libmapFiles() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_libmapFiles; }
    ಧ್ರುವ VFileLibList& vFiles() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vFiles; }
    ಧ್ರುವ VFileLibSet& vltFiles() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vltFiles; }
    ಧ್ರುವ VStringList& forceIncs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_forceIncs; }

    bool hasParameter(ಧ್ರುವ string& name);
    string parameter(ಧ್ರುವ string& name);
    ಶೂನ್ಯ checkParameters();

    bool isFuture(ಧ್ರುವ string& flag) ಧ್ರುವ;
    bool isFuture0(ಧ್ರುವ string& flag) ಧ್ರುವ;
    bool isFuture1(ಧ್ರುವ string& flag) ಧ್ರುವ;
    bool isLibraryFile(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname) ಧ್ರುವ;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fAcycSimp; }
    bool fAssemble() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fAssemble; }
    bool fBitScanLoops() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fBitScanLoops; }
    bool fBitblast() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fBitblast; }
    bool fCaseDecoder() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fCaseDecoder; }
    bool fCaseTable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fCaseTable; }
    bool f_Case_ವೃಕ್ಷ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_f_Case_ವೃಕ್ಷ; }
    bool fCombine() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fCombine; }
    bool fConst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fConst; }
    bool fConstBeforeDfg() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_ವೃಕ್ಷ() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_f_Const_Bit_Op_ವೃಕ್ಷ; }
    bool fConstEager() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fConstEager; }
    bool fDedupe() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDedupe; }
    bool fDfg() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDfg; }
    bool fDfgPeephole() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDfgPeephole; }
    bool f_Dfg_ತಳ್ಳು_Down_Sels() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_f_Dfg_ತಳ್ಳು_Down_Sels; }
    bool fDfgSynthesizeAll() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ಧ್ರುವ std::string& name) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ !m_fDfgPeepholeDisabled.ಎಣಿಕೆ(name);
    }
    bool fDeadAssigns() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDeadAssigns; }
    bool fDeadCells() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDeadCells; }
    bool fDeadMethods() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDeadMethods; }
    bool fDecompose() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDecomposePacked; }
    bool fDecomposeUnpacked() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fDecomposeUnpacked; }
    bool fExpand() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fExpand; }
    bool fFuncBalanceCat() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fFuncBalanceCat; }
    bool fFuncSplitCat() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fFuncSplitCat; }
    bool fFunc() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fGate; }
    VOptionBool fIcoChangeDetect() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fIcoChangeDetect; }
    bool fInline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fInline; }
    bool fInlineCFuncs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fInlineCFuncs; }
    bool fInlineFuncs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fInlineFuncs; }
    bool fInlineFuncsEager() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fInlineFuncsEager; }
    bool fLife() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fLife; }
    bool fLifePost() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fLifePost; }
    bool fLocalize() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fLocalize; }
    bool fMergeCond() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fMergeCond; }
    bool fMergeCondMotion() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fMergeCondMotion; }
    bool fReloop() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fReloop; }
    bool fReorder() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fReorder; }
    bool fSlice() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fSlice; }
    ಪೂರ್ಣಾಙ್ಕ fSliceElementLimit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fSliceElementLimit; }
    bool fSplit() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fSplit; }
    bool fSubst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fSubst; }
    bool fSubstConst() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fSubstConst; }
    bool fTable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fTable; }
    bool fTaskifyAll() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fTaskifyAll; }

    std::string traceClassBase() ಧ್ರುವ VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() ಧ್ರುವ VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() ಧ್ರುವ VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() ಧ್ರುವ VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() ಧ್ರುವ VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() ಧ್ರುವ VL_MT_SAFE;

    bool hierarchical() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hierarchical; }
    ಪೂರ್ಣಾಙ್ಕ hierChild() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_hierChild; }
    ಪೂರ್ಣಾಙ್ಕ hierThreads() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_ತುದಿ() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ !m_hierChild && !m_hierBlocks.empty(); }
    ಧ್ರುವ V3HierBlockOptSet& hierBlocks() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_ತುದಿ_Data_Dir() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ hier_ತುದಿ() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ಸ್ಥಿರ string version() VL_PURE;
    ಸ್ಥಿರ string argString(ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ** argv);  ///< Return list of arguments as simple string
    string allArgsString() ಧ್ರುವ VL_MT_SAFE;  ///< Return all passed arguments as simple string
    ಧ್ರುವ std::list<std::pair<std::list<std::string>, bool>>& allArgs() ಧ್ರುವ;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_ತುದಿ) ಧ್ರುವ;
    ಶೂನ್ಯ parseOpts(File_ಸಾಲು* fl, ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ** argv) VL_MT_DISABLED;
    ಶೂನ್ಯ parseOptsList(File_ಸಾಲು* fl, ಧ್ರುವ string& optdir, ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ** argv) VL_MT_DISABLED;
    ಶೂನ್ಯ parseOptsFile(File_ಸಾಲು* fl, ಧ್ರುವ string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    ಸ್ಥಿರ string getenvBuiltins(ಧ್ರುವ string& var);
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
    ಸ್ಥಿರ string getStdPackagePath();
    ಸ್ಥಿರ string getStdWaiverPath();
    ಸ್ಥಿರ string getSupported(ಧ್ರುವ string& var);
    ಸ್ಥಿರ bool systemCSystemWide();
    ಸ್ಥಿರ bool systemCFound();  // SystemC installed, or environment points to it
    ಸ್ಥಿರ bool coroutineSupport();  // Configured C++ compiler supports coroutines
    ಸ್ಥಿರ bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    ಸ್ಥಿರ bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    ಸ್ಥಿರ bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ಧ್ರುವ string& filename);
    string filePath(File_ಸಾಲು* fl, ಧ್ರುವ string& modname, ಧ್ರುವ string& lastpath,
                    ಧ್ರುವ string& errmsg);
    string file_Path_Looked_ಸಂದೇಶ(File_ಸಾಲು* fl, ಧ್ರುವ string& modname);
    V3LangCode fileLanguage(ಧ್ರುವ string& filename);
    ಸ್ಥಿರ bool fileStatNormal(ಧ್ರುವ string& filename);

    // METHODS (other OS)
    ಸ್ಥಿರ ಶೂನ್ಯ throwSigsegv();
};

//######################################################################

#endif  // guard
