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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3LangCode.h"

#অন্তর্ভাব <list>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <string>
#অন্তর্ভাব <vector>

class V3OptionsImp;
class File_সারি;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
সর্বজনীন:
    গণনা en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    গণনা en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(পূর্ণসংখ্যা _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুবক { ফেরত m_e; }
    ধ্রুবক অক্ষর* ascii() ধ্রুবক {
        স্থির ধ্রুবক অক্ষর* ধ্রুবক names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        ফেরত names[m_e];
    }
    bool isDefault() ধ্রুবক { ফেরত m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ধ্রুবক { ফেরত m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() ধ্রুবক { ফেরত m_e == OPT_TRUE; }
    bool isSetFalse() ধ্রুবক { ফেরত m_e == OPT_FALSE; }
    শূন্য setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(ধ্রুবক VOptionBool& lhs, ধ্রুবক VOptionBool& rhs) {
    ফেরত lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুবক VOptionBool& lhs, VOptionBool::en rhs) { ফেরত lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, ধ্রুবক VOptionBool& rhs) { ফেরত lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ধ্রুবক string m_filename;  // Filename
    ধ্রুবক string m_libname;  // Libname
সর্বজনীন:
    VFileLibName(ধ্রুবক string& filename, ধ্রুবক string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ধ্রুবক VFileLibName& rhs) = পূর্বনির্ধারিত;
    string filename() ধ্রুবক { ফেরত m_filename; }
    string libname() ধ্রুবক { ফেরত m_libname; }
    bool operator==(ধ্রুবক VFileLibName& rhs) ধ্রুবক {
        ফেরত m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(ধ্রুবক VFileLibName& rhs) ধ্রুবক {
        যদি (m_filename < rhs.m_filename) ফেরত true;
        যদি (m_filename > rhs.m_filename) ফেরত false;
        ফেরত m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
সর্বজনীন:
    গণনা en : uint8_t {
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
    গণনা : uint8_t { TS_DEFAULT = TS_1PS };
    গণনা en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(পূর্ণসংখ্যা _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ধ্রুবক string& মান, bool& badr);
    VTimescale(দ্বিগুণ মান, bool& badr) {
        badr = false;
        জন্য (পূর্ণসংখ্যা i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            যদি (multiplier() == মান) বিরতি;
        }
        যদি (multiplier() != মান) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ধ্রুবক { ফেরত m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    স্থির শূন্য parseSlashed(File_সারি* fl, ধ্রুবক অক্ষর* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    ধ্রুবক অক্ষর* ascii() ধ্রুবক {
        স্থির ধ্রুবক অক্ষর* ধ্রুবক names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        ফেরত names[m_e];
    }
    পূর্ণসংখ্যা powerOfTen() ধ্রুবক { ফেরত 2 - static_cast<পূর্ণসংখ্যা>(m_e); }
    দ্বিগুণ multiplier() ধ্রুবক {
        স্থির ধ্রুবক দ্বিগুণ মানসমূহ[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        ফেরত মানসমূহ[m_e];
    }
};
constexpr bool operator==(ধ্রুবক VTimescale& lhs, ধ্রুবক VTimescale& rhs) {
    ফেরত lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুবক VTimescale& lhs, VTimescale::en rhs) { ফেরত lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, ধ্রুবক VTimescale& rhs) { ফেরত lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(ধ্রুবক VTimescale& lhs, ধ্রুবক VTimescale& rhs) {
    ফেরত lhs.m_e > rhs.m_e;
}
অন্তঃরেখ std::ostream& operator<<(std::ostream& os, ধ্রুবক VTimescale& rhs) {
    ফেরত os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
সর্বজনীন:
    গণনা en : uint8_t {
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
    explicit VFsmExpandType(পূর্ণসংখ্যা _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুবক { ফেরত m_e; }
    constexpr ধ্রুবক অক্ষর* ascii() ধ্রুবক {
        constexpr ধ্রুবক অক্ষর* ধ্রুবক names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        ফেরত names[m_e];
    }
};
constexpr bool operator==(ধ্রুবক VFsmExpandType& lhs, ধ্রুবক VFsmExpandType& rhs) VL_MT_SAFE {
    ফেরত lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুবক VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    ফেরত lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, ধ্রুবক VFsmExpandType& rhs) VL_MT_SAFE {
    ফেরত lhs == rhs.m_e;
}
অন্তঃরেখ std::ostream& operator<<(std::ostream& os, ধ্রুবক VFsmExpandType& rhs) VL_MT_SAFE {
    ফেরত os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
সর্বজনীন:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ধ্রুবক std::string, std::string>;

ব্যক্তিগত:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

সর্বজনীন:
    explicit V3HierarchicalBlockOption(ধ্রুবক string& optstring);
    ধ্রুবক string& origName() ধ্রুবক { ফেরত m_origName; }
    ধ্রুবক string& mangledName() ধ্রুবক { ফেরত m_mangledName; }
    ধ্রুবক ParamStrMap params() ধ্রুবক { ফেরত m_parameters; }
};

using V3HierBlockOptSet = std::map<ধ্রুবক std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
সর্বজনীন:
ব্যক্তিগত:
    // TYPES
    using DebugLevelMap = std::map<ধ্রুবক std::string, unsigned>;

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
    std::map<ধ্রুবক string, string> m_parameters;  // Parameters
    std::map<ধ্রুবক string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_সারি = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_দ্বিমিক = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_রাশিমালা = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_সারি = false;    // main switch: --coverage-block
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
    bool m_debug_স্তূপ_Check = false;  // main switch: --debug-stack-check
    পূর্ণসংখ্যা m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_no_Trace_শীর্ষ = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    পূর্ণসংখ্যা         m_buildJobs = -1;    // main switch: --build-jobs, -j
    পূর্ণসংখ্যা         m_coverage_রাশিমালা_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    পূর্ণসংখ্যা         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    পূর্ণসংখ্যা         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    পূর্ণসংখ্যা         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    পূর্ণসংখ্যা         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    পূর্ণসংখ্যা         m_expandLimit = 256;  // main switch: --expand-limit
    পূর্ণসংখ্যা         m_gateStmts = 100;    // main switch: --gate-stmts
    পূর্ণসংখ্যা         m_hierChild = 0;      // main switch: --hierarchical-child
    পূর্ণসংখ্যা         m_hierThreads = 0;      // main switch: --hierarchical-threads
    পূর্ণসংখ্যা         m_ifDepth = 0;      // main switch: --if-depth
    পূর্ণসংখ্যা         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    পূর্ণসংখ্যা         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    পূর্ণসংখ্যা         m_inlineMult = 2000;   // main switch: --inline-mult
    পূর্ণসংখ্যা         m_instr_গুনতি_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    পূর্ণসংখ্যা         m_localize_Max_আকার = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    পূর্ণসংখ্যা         m_max_অঙ্ক_Width = 65536;  // main switch: --max-num-width
    পূর্ণসংখ্যা         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    পূর্ণসংখ্যা         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    পূর্ণসংখ্যা         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_output_Groups_আকার = 512 * 1024 * 1024;  // main switch: --output-groups-size
    পূর্ণসংখ্যা         m_outputSplit = 20000;  // main switch: --output-split
    পূর্ণসংখ্যা         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    পূর্ণসংখ্যা         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    পূর্ণসংখ্যা         m_pinsBv = 65;       // main switch: --pins-bv
    পূর্ণসংখ্যা         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    পূর্ণসংখ্যা         m_publicDepth = 0;   // main switch: --public-depth
    পূর্ণসংখ্যা         m_reloopLimit = 40; // main switch: --reloop-limit
    পূর্ণসংখ্যা         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    পূর্ণসংখ্যা         m_threads = 1;      // main switch: --threads
    পূর্ণসংখ্যা         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    পূর্ণসংখ্যা         m_traceDepth = 0;   // main switch: --trace-depth
    পূর্ণসংখ্যা         m_traceMaxArray = 32;  // main switch: --trace-max-array
    পূর্ণসংখ্যা         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    পূর্ণসংখ্যা         m_unroll_গুনতি = 64;  // main switch: --unroll-count
    পূর্ণসংখ্যা         m_unrollLimit = 16384;  // main switch: --unroll-limit
    পূর্ণসংখ্যা         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    পূর্ণসংখ্যা         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    পূর্ণসংখ্যা         m_verilateJobs = -1;  // main switch: --verilate-jobs

    পূর্ণসংখ্যা         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    পূর্ণসংখ্যা         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    পূর্ণসংখ্যা         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_শীর্ষ_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_চাবি;   // main switch: --protect-key
    string      m_শীর্ষ_Module;    // main switch: --top-module
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
    bool m_f_Case_বৃক্ষ;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_বৃক্ষ;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ঠেলো_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    পূর্ণসংখ্যা  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    শূন্য addArg(অক্ষর** argv, আকার_t গুনতি, bool isForRerun);
    শূন্য addArg(ধ্রুবক std::string& arg, bool isForRerun);
    শূন্য add_সারি_Arg(ধ্রুবক string& arg);
    শূন্য addDefine(ধ্রুবক string& defline, bool allowPlus) VL_MT_DISABLED;
    শূন্য addFuture(ধ্রুবক string& flag);
    শূন্য addFuture0(ধ্রুবক string& flag);
    শূন্য addFuture1(ধ্রুবক string& flag);
    শূন্য addIncDirUser(ধ্রুবক string& incdir);  // User requested
    শূন্য addIncDirFallback(ধ্রুবক string& incdir);  // Low priority if not found otherwise
    শূন্য addParameter(ধ্রুবক string& paramline, bool allowPlus);
    শূন্য addLangExt(ধ্রুবক string& langext, ধ্রুবক V3LangCode& lc);
    শূন্য addLibExtV(ধ্রুবক string& libext);
    শূন্য optimize(পূর্ণসংখ্যা level);
    শূন্য showVersion(bool verbose);
    শূন্য coverage(bool flag) {
        m_coverage_সারি = m_coverageBranch = m_coverageToggle = m_coverage_রাশিমালা = m_coverageFsm
            = m_coverageUser = flag;
    }
    স্থির bool suffixed(ধ্রুবক string& sw, ধ্রুবক অক্ষর* arg);
    স্থির string parseFileArg(ধ্রুবক string& optdir, ধ্রুবক string& relfilename);
    string filePathCheckOneDir(ধ্রুবক string& modname, ধ্রুবক string& dirname);
    স্থির পূর্ণসংখ্যা stripOptionsForChildRun(ধ্রুবক string& opt, bool for_শীর্ষ);
    শূন্য validateIdentifier(File_সারি* fl, ধ্রুবক string& arg, ধ্রুবক string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

সর্বজনীন:
    V3Options();
    ~V3Options();
    শূন্য setDebugMode(পূর্ণসংখ্যা level);
    unsigned debugLevel(ধ্রুবক string& tag) ধ্রুবক VL_MT_SAFE;
    unsigned debugSrcLevel(ধ্রুবক string& srcfile_path) ধ্রুবক VL_MT_SAFE;
    unsigned dumpLevel(ধ্রুবক string& tag) ধ্রুবক VL_MT_SAFE;
    unsigned dumpSrcLevel(ধ্রুবক string& srcfile_path) ধ্রুবক VL_MT_SAFE;

    // METHODS
    শূন্য addCppFile(ধ্রুবক string& filename);
    শূন্য addCFlags(ধ্রুবক string& filename);
    শূন্য addCompilerIncludes(ধ্রুবক string& filename);
    শূন্য addLdLibFile(ধ্রুবক string& filename);
    শূন্য addLdLibs(ধ্রুবক string& filename);
    শূন্য addMakeFlags(ধ্রুবক string& filename);
    শূন্য addLibMapFile(ধ্রুবক string& filename);
    শূন্য addLibraryFile(ধ্রুবক string& filename, ধ্রুবক string& libname);
    শূন্য addVFile(ধ্রুবক string& filename, ধ্রুবক string& libname);
    শূন্য addVltFile(ধ্রুবক string& filename, ধ্রুবক string& libname);
    শূন্য addForceInc(ধ্রুবক string& filename);
    bool available() ধ্রুবক VL_MT_SAFE { ফেরত m_available; }
    শূন্য ccSet();
    শূন্য decorations(File_সারি* fl, ধ্রুবক string& filename);
    শূন্য notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ধ্রুবক { ফেরত m_preprocOnly; }
    bool makePhony() ধ্রুবক { ফেরত m_makePhony; }
    bool preproc_No_সারি() ধ্রুবক { ফেরত m_preproc_No_সারি; }
    bool preprocResolve() ধ্রুবক { ফেরত m_preprocResolve; }
    পূর্ণসংখ্যা preprocTokenLimit() ধ্রুবক { ফেরত m_preprocTokenLimit; }
    bool underlineZero() ধ্রুবক { ফেরত m_underlineZero; }
    bool systemC() ধ্রুবক VL_MT_SAFE { ফেরত m_systemC; }
    bool savable() ধ্রুবক VL_MT_SAFE { ফেরত m_savable; }
    VOptionBool schedZeroDelay() ধ্রুবক { ফেরত m_schedZeroDelay; }
    bool stats() ধ্রুবক { ফেরত m_stats; }
    bool statsVars() ধ্রুবক { ফেরত m_statsVars; }
    bool stdPackage() ধ্রুবক { ফেরত m_stdPackage; }
    bool stdWaiver() ধ্রুবক { ফেরত m_stdWaiver; }
    bool assertOn() ধ্রুবক { ফেরত m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ধ্রুবক { ফেরত m_assertCase; }
    bool autoflush() ধ্রুবক { ফেরত m_autoflush; }
    bool bboxSys() ধ্রুবক { ফেরত m_bboxSys; }
    bool bboxUnsup() ধ্রুবক { ফেরত m_bboxUnsup; }
    bool build() ধ্রুবক { ফেরত m_build; }
    string buildDepBin() ধ্রুবক { ফেরত m_buildDepBin; }
    শূন্য buildDepBin(ধ্রুবক string& flag) { m_buildDepBin = flag; }
    bool context() ধ্রুবক VL_MT_SAFE { ফেরত m_context; }
    bool coverage() ধ্রুবক VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        ফেরত m_coverage_সারি || m_coverageBranch || m_coverageToggle || m_coverage_রাশিমালা
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ধ্রুবক VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        ফেরত m_coverage_সারি || m_coverageBranch || m_coverageToggle || m_coverage_রাশিমালা
               || m_coverageUser;
    }
    bool coverageBranch() ধ্রুবক { ফেরত m_coverageBranch; }
    bool coverage_রাশিমালা() ধ্রুবক { ফেরত m_coverage_রাশিমালা; }
    bool coverageFsm() ধ্রুবক { ফেরত m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ধ্রুবক { ফেরত m_coverageFsmExpand; }
    পূর্ণসংখ্যা coverageFsmMaxArcs() ধ্রুবক { ফেরত m_coverageFsmMaxArcs; }
    bool coverage_সারি() ধ্রুবক { ফেরত m_coverage_সারি; }
    bool coverageMergeInstances() ধ্রুবক { ফেরত m_coverageMergeInstances; }
    bool coveragePerInstance() ধ্রুবক { ফেরত m_coveragePerInstance; }
    bool coverageToggle() ধ্রুবক { ফেরত m_coverageToggle; }
    bool coverageUnderscore() ধ্রুবক { ফেরত m_coverageUnderscore; }
    bool coverageUser() ধ্রুবক { ফেরত m_coverageUser; }
    bool debugCheck() ধ্রুবক VL_MT_SAFE { ফেরত m_debugCheck; }
    bool debugCollision() ধ্রুবক { ফেরত m_debugCollision; }
    bool debugEmitV() ধ্রুবক VL_MT_SAFE { ফেরত m_debugEmitV; }
    bool debugExitElab() ধ্রুবক { ফেরত m_debugExitElab; }
    bool debugExitParse() ধ্রুবক { ফেরত m_debugExitParse; }
    bool debugLeak() ধ্রুবক { ফেরত m_debugLeak; }
    bool debugNondeterminism() ধ্রুবক { ফেরত m_debugNondeterminism; }
    bool debugPartition() ধ্রুবক { ফেরত m_debugPartition; }
    bool debugPreprocPassthru() ধ্রুবক VL_MT_SAFE { ফেরত m_debugPreprocPassthru; }
    bool debugProtect() ধ্রুবক VL_MT_SAFE { ফেরত m_debugProtect; }
    পূর্ণসংখ্যা debugRuntimeTimeout() ধ্রুবক { ফেরত m_debugRuntimeTimeout; }
    bool debugSelfTest() ধ্রুবক { ফেরত m_debugSelfTest; }
    bool debug_স্তূপ_Check() ধ্রুবক { ফেরত m_debug_স্তূপ_Check; }
    bool debugWidth() ধ্রুবক VL_PURE { ফেরত m_debugWidth; }
    bool decoration() ধ্রুবক VL_MT_SAFE { ফেরত m_decoration; }
    bool decorationNodes() ধ্রুবক VL_MT_SAFE { ফেরত m_decorationNodes; }
    bool diagnosticsSarif() ধ্রুবক VL_MT_SAFE { ফেরত m_diagnosticsSarif; }
    bool dpiHdrOnly() ধ্রুবক { ফেরত m_dpiHdrOnly; }
    bool dumpAstPatterns() ধ্রুবক {
        ফেরত m_dumpLevel.গুনতি("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() ধ্রুবক { ফেরত m_dumpLevel.গুনতি("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ধ্রুবক {
        ফেরত m_dumpLevel.গুনতি("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_বৃক্ষ_Dot() ধ্রুবক {
        ফেরত m_dumpLevel.গুনতি("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ধ্রুবক { ফেরত m_emitAccessors; }
    bool exe() ধ্রুবক { ফেরত m_exe; }
    bool flatten() ধ্রুবক { ফেরত m_flatten; }
    bool fourstate() ধ্রুবক { ফেরত m_fourstate; }
    bool gmake() ধ্রুবক { ফেরত m_gmake; }
    bool makeJson() ধ্রুবক { ফেরত m_makeJson; }
    bool threadsDpiPure() ধ্রুবক { ফেরত m_threadsDpiPure; }
    bool threadsDpiUnpure() ধ্রুবক { ফেরত m_threadsDpiUnpure; }
    bool threadsCoarsen() ধ্রুবক { ফেরত m_threadsCoarsen; }
    VOptionBool timing() ধ্রুবক { ফেরত m_timing; }
    bool trace() ধ্রুবক { ফেরত m_trace; }
    bool traceCoverage() ধ্রুবক { ফেরত m_traceCoverage; }
    bool traceEnabledFst() ধ্রুবক { ফেরত m_traceEnabledFst; }
    bool traceEnabledSaif() ধ্রুবক { ফেরত m_traceEnabledSaif; }
    bool traceEnabledVcd() ধ্রুবক { ফেরত m_traceEnabledVcd; }
    bool traceParams() ধ্রুবক { ফেরত m_traceParams; }
    bool traceStructs() ধ্রুবক { ফেরত m_traceStructs; }
    bool traceUnderscore() ধ্রুবক { ফেরত m_traceUnderscore; }
    bool প্রধান() ধ্রুবক { ফেরত m_main; }
    bool outFormatOk() ধ্রুবক { ফেরত m_outFormatOk; }
    bool jsonOnly() ধ্রুবক { ফেরত m_jsonOnly; }
    bool keep_অস্থায়ী_Files() ধ্রুবক { ফেরত (V3Error::debugDefault() != 0); }
    bool pedantic() ধ্রুবক { ফেরত m_pedantic; }
    bool pinsInoutEnables() ধ্রুবক { ফেরত m_pinsInoutEnables; }
    bool pinsScUint() ধ্রুবক { ফেরত m_pinsScUint; }
    bool pinsScUintBool() ধ্রুবক { ফেরত m_pinsScUintBool; }
    bool pinsScBigUint() ধ্রুবক VL_MT_SAFE { ফেরত m_pinsScBigUint; }
    bool pinsUint8() ধ্রুবক { ফেরত m_pinsUint8; }
    bool preprocComments() ধ্রুবক { ফেরত m_preprocComments; }
    bool preprocDefines() ধ্রুবক { ফেরত m_preprocDefines; }
    bool profC() ধ্রুবক { ফেরত m_profC; }
    bool profCFuncs() ধ্রুবক { ফেরত m_profCFuncs; }
    bool profExec() ধ্রুবক { ফেরত m_profExec; }
    bool profPgo() ধ্রুবক { ফেরত m_profPgo; }
    bool usesProfiler() ধ্রুবক { ফেরত profExec() || profPgo(); }
    bool protectIds() ধ্রুবক VL_MT_SAFE { ফেরত m_protectIds; }
    bool allPublic() ধ্রুবক { ফেরত m_public; }
    bool publicParams() ধ্রুবক { ফেরত m_publicParams; }
    bool publicOff() ধ্রুবক { ফেরত m_publicIgnore; }
    bool publicFlatRW() ধ্রুবক { ফেরত m_publicFlatRW; }
    পূর্ণসংখ্যা publicDepth() ধ্রুবক { ফেরত m_publicDepth; }
    bool anyPublicFlat() ধ্রুবক { ফেরত m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() ধ্রুবক VL_MT_SAFE { ফেরত m_lintOnly; }
    bool ignc() ধ্রুবক { ফেরত m_ignc; }
    bool quietBuild() ধ্রুবক VL_MT_SAFE { ফেরত m_quietBuild; }
    bool quietExit() ধ্রুবক VL_MT_SAFE { ফেরত m_quietExit; }
    bool quietStats() ধ্রুবক VL_MT_SAFE { ফেরত m_quietStats; }
    bool reportUnoptflat() ধ্রুবক { ফেরত m_reportUnoptflat; }
    bool verilate() ধ্রুবক { ফেরত m_verilate; }
    bool vpi() ধ্রুবক { ফেরত m_vpi.isTrue(); }
    bool waiverMultiline() ধ্রুবক { ফেরত m_waiverMultiline; }
    bool xInitialEdge() ধ্রুবক { ফেরত m_xInitialEdge; }
    bool serializeOnly() ধ্রুবক { ফেরত m_jsonOnly; }
    bool শীর্ষ_Ifaces_Supported() ধ্রুবক { ফেরত lintOnly() && !hierarchical(); }

    পূর্ণসংখ্যা buildJobs() ধ্রুবক VL_MT_SAFE { ফেরত m_buildJobs; }
    পূর্ণসংখ্যা convergeLimit() ধ্রুবক { ফেরত m_convergeLimit; }
    পূর্ণসংখ্যা coverage_রাশিমালা_Max() ধ্রুবক { ফেরত m_coverage_রাশিমালা_Max; }
    uint32_t coverageMaxBins() ধ্রুবক { ফেরত m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ধ্রুবক { ফেরত m_coverageMaxRealBins; }
    পূর্ণসংখ্যা coverageMaxWidth() ধ্রুবক { ফেরত m_coverageMaxWidth; }
    পূর্ণসংখ্যা debugAllocRandom() ধ্রুবক { ফেরত m_debugAllocRandom; }
    bool dump_বৃক্ষ_Addrids() ধ্রুবক VL_MT_SAFE;
    পূর্ণসংখ্যা expandLimit() ধ্রুবক { ফেরত m_expandLimit; }
    পূর্ণসংখ্যা gateStmts() ধ্রুবক { ফেরত m_gateStmts; }
    পূর্ণসংখ্যা ifDepth() ধ্রুবক { ফেরত m_ifDepth; }
    পূর্ণসংখ্যা inlineCFuncs() ধ্রুবক { ফেরত m_inlineCFuncs; }
    পূর্ণসংখ্যা inlineCFuncsProduct() ধ্রুবক { ফেরত m_inlineCFuncsProduct; }
    পূর্ণসংখ্যা inlineMult() ধ্রুবক { ফেরত m_inlineMult; }
    পূর্ণসংখ্যা instr_গুনতি_Dpi() ধ্রুবক { ফেরত m_instr_গুনতি_Dpi; }
    পূর্ণসংখ্যা localize_Max_আকার() ধ্রুবক { ফেরত m_localize_Max_আকার; }
    bool jsonEditNums() ধ্রুবক { ফেরত m_jsonEditNums; }
    bool jsonIds() ধ্রুবক { ফেরত m_jsonIds; }
    VOptionBool makeDepend() ধ্রুবক { ফেরত m_makeDepend; }
    পূর্ণসংখ্যা max_অঙ্ক_Width() ধ্রুবক { ফেরত m_max_অঙ্ক_Width; }
    পূর্ণসংখ্যা funcRecursionDepth() ধ্রুবক { ফেরত m_funcRecursion; }
    পূর্ণসংখ্যা moduleRecursionDepth() ধ্রুবক { ফেরত m_moduleRecursion; }
    পূর্ণসংখ্যা outputSplit() ধ্রুবক { ফেরত m_outputSplit; }
    পূর্ণসংখ্যা outputSplitCFuncs() ধ্রুবক { ফেরত m_outputSplitCFuncs; }
    পূর্ণসংখ্যা outputSplitCTrace() ধ্রুবক { ফেরত m_outputSplitCTrace; }
    পূর্ণসংখ্যা outputGroups() ধ্রুবক { ফেরত m_outputGroups; }
    int64_t output_Groups_আকার() ধ্রুবক { ফেরত m_output_Groups_আকার; }
    পূর্ণসংখ্যা pinsBv() ধ্রুবক VL_MT_SAFE { ফেরত m_pinsBv; }
    পূর্ণসংখ্যা reloopLimit() ধ্রুবক { ফেরত m_reloopLimit; }
    পূর্ণসংখ্যা replicationLimit() ধ্রুবক { ফেরত m_replicationLimit; }
    VOptionBool skipIdentical() ধ্রুবক { ফেরত m_skipIdentical; }
    bool stopFail() ধ্রুবক { ফেরত m_stopFail; }
    পূর্ণসংখ্যা threads() ধ্রুবক VL_MT_SAFE { ফেরত m_threads; }
    পূর্ণসংখ্যা threadsMaxMTasks() ধ্রুবক { ফেরত m_threadsMaxMTasks; }
    bool mtasks() ধ্রুবক VL_MT_SAFE { ফেরত (m_threads > 1); }
    VTimescale timeDefaultPrec() ধ্রুবক { ফেরত m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() ধ্রুবক { ফেরত m_timeDefaultUnit; }
    VTimescale timeOverridePrec() ধ্রুবক { ফেরত m_timeOverridePrec; }
    VTimescale timeOverrideUnit() ধ্রুবক { ফেরত m_timeOverrideUnit; }
    VTimescale timeComputePrec(ধ্রুবক VTimescale& flag) ধ্রুবক;
    VTimescale timeComputeUnit(ধ্রুবক VTimescale& flag) ধ্রুবক;
    পূর্ণসংখ্যা traceDepth() ধ্রুবক { ফেরত m_traceDepth; }
    পূর্ণসংখ্যা traceMaxArray() ধ্রুবক { ফেরত m_traceMaxArray; }
    পূর্ণসংখ্যা traceMaxWidth() ধ্রুবক { ফেরত m_traceMaxWidth; }
    bool useTraceParallel() ধ্রুবক {
        ফেরত trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    পূর্ণসংখ্যা unroll_গুনতি() ধ্রুবক { ফেরত m_unroll_গুনতি; }
    পূর্ণসংখ্যা unrollLimit() ধ্রুবক { ফেরত m_unrollLimit; }
    পূর্ণসংখ্যা unrollStmts() ধ্রুবক { ফেরত m_unrollStmts; }
    পূর্ণসংখ্যা constraintArrayLimit() ধ্রুবক { ফেরত m_constraintArrayLimit; }
    পূর্ণসংখ্যা verilateJobs() ধ্রুবক { ফেরত m_verilateJobs; }

    পূর্ণসংখ্যা compLimitBlocks() ধ্রুবক { ফেরত m_compLimitBlocks; }
    পূর্ণসংখ্যা compLimitMembers() ধ্রুবক VL_MT_SAFE { ফেরত m_compLimitMembers; }
    পূর্ণসংখ্যা compLimitParens() ধ্রুবক { ফেরত m_compLimitParens; }

    string diagnosticsSarifOutput() ধ্রুবক VL_MT_SAFE {
        ফেরত m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ধ্রুবক { ফেরত m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() ধ্রুবক { ফেরত m_hierParamsFile; }
    string jsonOnlyOutput() ধ্রুবক { ফেরত m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ধ্রুবক { ফেরত m_jsonOnlyMetaOutput; }
    string l2Name() ধ্রুবক { ফেরত m_l2Name; }
    string libCreate() ধ্রুবক { ফেরত m_libCreate; }
    string libCreateName(bool shared) ধ্রুবক {
        string libName = "lib" + libCreate();
        যদি (shared) {
            libName += ".so";
        } অন্যথা {
            libName += ".a";
        }
        ফেরত libName;
    }
    string main_শীর্ষ_Name() ধ্রুবক { ফেরত m_main_শীর্ষ_Name; }
    string makeDir() ধ্রুবক VL_MT_SAFE { ফেরত m_makeDir; }
    string modPrefix() ধ্রুবক VL_MT_SAFE { ফেরত m_modPrefix; }
    string pipeFilter() ধ্রুবক { ফেরত m_pipeFilter; }
    string prefix() ধ্রুবক VL_MT_SAFE { ফেরত m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_চাবি_Provided() ধ্রুবক { ফেরত !m_protect_চাবি.empty(); }
    string protect_চাবি_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string শীর্ষ_Module() ধ্রুবক { ফেরত m_শীর্ষ_Module; }  // As AstNode::encodeName()
    bool no_Trace_শীর্ষ() ধ্রুবক { ফেরত m_no_Trace_শীর্ষ; }
    string unusedRegexp() ধ্রুবক { ফেরত m_unusedRegexp; }
    string waiverOutput() ধ্রুবক { ফেরত m_waiverOutput; }
    string work() ধ্রুবক { ফেরত m_work; }
    bool isWaiverOutput() ধ্রুবক { ফেরত !m_waiverOutput.empty(); }
    string xAssign() ধ্রুবক { ফেরত m_xAssign; }
    string xInitial() ধ্রুবক { ফেরত m_xInitial; }

    ধ্রুবক VStringSet& cppFiles() ধ্রুবক { ফেরত m_cppFiles; }
    ধ্রুবক VStringList& cFlags() ধ্রুবক { ফেরত m_cFlags; }
    ধ্রুবক VStringSet& compilerIncludes() ধ্রুবক { ফেরত m_compilerIncludes; }
    ধ্রুবক VStringList& ldLibFiles() ধ্রুবক { ফেরত m_ldLibFiles; }
    ধ্রুবক VStringList& ldLibs() ধ্রুবক { ফেরত m_ldLibs; }
    ধ্রুবক VStringList& makeFlags() ধ্রুবক { ফেরত m_makeFlags; }
    ধ্রুবক VFileLibSet& libraryFiles() ধ্রুবক { ফেরত m_libraryFiles; }
    ধ্রুবক VStringSet& libmapFiles() ধ্রুবক { ফেরত m_libmapFiles; }
    ধ্রুবক VFileLibList& vFiles() ধ্রুবক { ফেরত m_vFiles; }
    ধ্রুবক VFileLibSet& vltFiles() ধ্রুবক { ফেরত m_vltFiles; }
    ধ্রুবক VStringList& forceIncs() ধ্রুবক { ফেরত m_forceIncs; }

    bool hasParameter(ধ্রুবক string& name);
    string parameter(ধ্রুবক string& name);
    শূন্য checkParameters();

    bool isFuture(ধ্রুবক string& flag) ধ্রুবক;
    bool isFuture0(ধ্রুবক string& flag) ধ্রুবক;
    bool isFuture1(ধ্রুবক string& flag) ধ্রুবক;
    bool isLibraryFile(ধ্রুবক string& filename, ধ্রুবক string& libname) ধ্রুবক;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ধ্রুবক { ফেরত m_fAcycSimp; }
    bool fAssemble() ধ্রুবক { ফেরত m_fAssemble; }
    bool fBitScanLoops() ধ্রুবক { ফেরত m_fBitScanLoops; }
    bool fBitblast() ধ্রুবক { ফেরত m_fBitblast; }
    bool fCaseDecoder() ধ্রুবক { ফেরত m_fCaseDecoder; }
    bool fCaseTable() ধ্রুবক { ফেরত m_fCaseTable; }
    bool f_Case_বৃক্ষ() ধ্রুবক { ফেরত m_f_Case_বৃক্ষ; }
    bool fCombine() ধ্রুবক { ফেরত m_fCombine; }
    bool fConst() ধ্রুবক { ফেরত m_fConst; }
    bool fConstBeforeDfg() ধ্রুবক { ফেরত m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_বৃক্ষ() ধ্রুবক { ফেরত m_f_Const_Bit_Op_বৃক্ষ; }
    bool fConstEager() ধ্রুবক { ফেরত m_fConstEager; }
    bool fDedupe() ধ্রুবক { ফেরত m_fDedupe; }
    bool fDfg() ধ্রুবক { ফেরত m_fDfg; }
    bool fDfgPeephole() ধ্রুবক { ফেরত m_fDfgPeephole; }
    bool f_Dfg_ঠেলো_Down_Sels() ধ্রুবক { ফেরত m_f_Dfg_ঠেলো_Down_Sels; }
    bool fDfgSynthesizeAll() ধ্রুবক { ফেরত m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ধ্রুবক std::string& name) ধ্রুবক {
        ফেরত !m_fDfgPeepholeDisabled.গুনতি(name);
    }
    bool fDeadAssigns() ধ্রুবক { ফেরত m_fDeadAssigns; }
    bool fDeadCells() ধ্রুবক { ফেরত m_fDeadCells; }
    bool fDeadMethods() ধ্রুবক { ফেরত m_fDeadMethods; }
    bool fDecompose() ধ্রুবক { ফেরত m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ধ্রুবক { ফেরত m_fDecomposePacked; }
    bool fDecomposeUnpacked() ধ্রুবক { ফেরত m_fDecomposeUnpacked; }
    bool fExpand() ধ্রুবক { ফেরত m_fExpand; }
    bool fFuncBalanceCat() ধ্রুবক { ফেরত m_fFuncBalanceCat; }
    bool fFuncSplitCat() ধ্রুবক { ফেরত m_fFuncSplitCat; }
    bool fFunc() ধ্রুবক { ফেরত fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() ধ্রুবক { ফেরত m_fGate; }
    VOptionBool fIcoChangeDetect() ধ্রুবক { ফেরত m_fIcoChangeDetect; }
    bool fInline() ধ্রুবক { ফেরত m_fInline; }
    bool fInlineCFuncs() ধ্রুবক { ফেরত m_fInlineCFuncs; }
    bool fInlineFuncs() ধ্রুবক { ফেরত m_fInlineFuncs; }
    bool fInlineFuncsEager() ধ্রুবক { ফেরত m_fInlineFuncsEager; }
    bool fLife() ধ্রুবক { ফেরত m_fLife; }
    bool fLifePost() ধ্রুবক { ফেরত m_fLifePost; }
    bool fLocalize() ধ্রুবক { ফেরত m_fLocalize; }
    bool fMergeCond() ধ্রুবক { ফেরত m_fMergeCond; }
    bool fMergeCondMotion() ধ্রুবক { ফেরত m_fMergeCondMotion; }
    bool fReloop() ধ্রুবক { ফেরত m_fReloop; }
    bool fReorder() ধ্রুবক { ফেরত m_fReorder; }
    bool fSlice() ধ্রুবক { ফেরত m_fSlice; }
    পূর্ণসংখ্যা fSliceElementLimit() ধ্রুবক { ফেরত m_fSliceElementLimit; }
    bool fSplit() ধ্রুবক { ফেরত m_fSplit; }
    bool fSubst() ধ্রুবক { ফেরত m_fSubst; }
    bool fSubstConst() ধ্রুবক { ফেরত m_fSubstConst; }
    bool fTable() ধ্রুবক { ফেরত m_fTable; }
    bool fTaskifyAll() ধ্রুবক { ফেরত m_fTaskifyAll; }

    std::string traceClassBase() ধ্রুবক VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() ধ্রুবক VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() ধ্রুবক VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() ধ্রুবক VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() ধ্রুবক VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() ধ্রুবক VL_MT_SAFE;

    bool hierarchical() ধ্রুবক { ফেরত m_hierarchical; }
    পূর্ণসংখ্যা hierChild() ধ্রুবক VL_MT_SAFE { ফেরত m_hierChild; }
    পূর্ণসংখ্যা hierThreads() ধ্রুবক VL_MT_SAFE { ফেরত m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_শীর্ষ() ধ্রুবক VL_MT_SAFE { ফেরত !m_hierChild && !m_hierBlocks.empty(); }
    ধ্রুবক V3HierBlockOptSet& hierBlocks() ধ্রুবক { ফেরত m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_শীর্ষ_তথ্য_Dir() ধ্রুবক VL_MT_SAFE {
        ফেরত hier_শীর্ষ() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    স্থির string version() VL_PURE;
    স্থির string argString(পূর্ণসংখ্যা argc, অক্ষর** argv);  ///< Return list of arguments as simple string
    string allArgsString() ধ্রুবক VL_MT_SAFE;  ///< Return all passed arguments as simple string
    ধ্রুবক std::list<std::pair<std::list<std::string>, bool>>& allArgs() ধ্রুবক;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_শীর্ষ) ধ্রুবক;
    শূন্য parseOpts(File_সারি* fl, পূর্ণসংখ্যা argc, অক্ষর** argv) VL_MT_DISABLED;
    শূন্য parseOptsList(File_সারি* fl, ধ্রুবক string& optdir, পূর্ণসংখ্যা argc, অক্ষর** argv) VL_MT_DISABLED;
    শূন্য parseOptsFile(File_সারি* fl, ধ্রুবক string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    স্থির string getenvBuiltins(ধ্রুবক string& var);
    স্থির string getenvMAKE();
    স্থির string getenvMAKEFLAGS();
    স্থির string getenvPERL();
    স্থির string getenvPYTHON3();
    স্থির string getenvSYSTEMC();
    স্থির string getenvSYSTEMC_ARCH();
    স্থির string getenvSYSTEMC_INCLUDE();
    স্থির string getenvSYSTEMC_LIBDIR();
    স্থির string getenv_VERILATOR_মূল();
    স্থির string getenvVERILATOR_SOLVER();
    স্থির string getStdPackagePath();
    স্থির string getStdWaiverPath();
    স্থির string getSupported(ধ্রুবক string& var);
    স্থির bool systemCSystemWide();
    স্থির bool systemCFound();  // SystemC installed, or environment points to it
    স্থির bool coroutineSupport();  // Configured C++ compiler supports coroutines
    স্থির bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    স্থির bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    স্থির bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ধ্রুবক string& filename);
    string filePath(File_সারি* fl, ধ্রুবক string& modname, ধ্রুবক string& lastpath,
                    ধ্রুবক string& errmsg);
    string file_Path_Looked_বার্তা(File_সারি* fl, ধ্রুবক string& modname);
    V3LangCode fileLanguage(ধ্রুবক string& filename);
    স্থির bool fileStatNormal(ধ্রুবক string& filename);

    // METHODS (other OS)
    স্থির শূন্য throwSigsegv();
};

//######################################################################

#endif  // guard
