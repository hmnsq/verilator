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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3LangCode.h"

#અન્તર્ભાવ <list>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <string>
#અન્તર્ભાવ <vector>

class V3OptionsImp;
class File_લીટી;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
જાહેર:
    ગણના en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    ગણના en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર અચલ અક્ષર* અચલ names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        પાછા names[m_e];
    }
    bool isDefault() અચલ { પાછા m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() અચલ { પાછા m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() અચલ { પાછા m_e == OPT_TRUE; }
    bool isSetFalse() અચલ { પાછા m_e == OPT_FALSE; }
    શૂન્ય setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(અચલ VOptionBool& lhs, અચલ VOptionBool& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VOptionBool& lhs, VOptionBool::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, અચલ VOptionBool& rhs) { પાછા lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    અચલ string m_filename;  // Filename
    અચલ string m_libname;  // Libname
જાહેર:
    VFileLibName(અચલ string& filename, અચલ string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(અચલ VFileLibName& rhs) = મૂળભૂત;
    string filename() અચલ { પાછા m_filename; }
    string libname() અચલ { પાછા m_libname; }
    bool operator==(અચલ VFileLibName& rhs) અચલ {
        પાછા m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(અચલ VFileLibName& rhs) અચલ {
        જો (m_filename < rhs.m_filename) પાછા true;
        જો (m_filename > rhs.m_filename) પાછા false;
        પાછા m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
જાહેર:
    ગણના en : uint8_t {
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
    ગણના : uint8_t { TS_DEFAULT = TS_1PS };
    ગણના en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(અચલ string& કિંમત, bool& badr);
    VTimescale(દ્વિગુણ કિંમત, bool& badr) {
        badr = false;
        માટે (પૂર્ણાંક i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            જો (multiplier() == કિંમત) તોડો;
        }
        જો (multiplier() != કિંમત) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() અચલ { પાછા m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    સ્થિર શૂન્ય parseSlashed(File_લીટી* fl, અચલ અક્ષર* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    અચલ અક્ષર* ascii() અચલ {
        સ્થિર અચલ અક્ષર* અચલ names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        પાછા names[m_e];
    }
    પૂર્ણાંક powerOfTen() અચલ { પાછા 2 - static_cast<પૂર્ણાંક>(m_e); }
    દ્વિગુણ multiplier() અચલ {
        સ્થિર અચલ દ્વિગુણ કિંમતો[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        પાછા કિંમતો[m_e];
    }
};
constexpr bool operator==(અચલ VTimescale& lhs, અચલ VTimescale& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VTimescale& lhs, VTimescale::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, અચલ VTimescale& rhs) { પાછા lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(અચલ VTimescale& lhs, અચલ VTimescale& rhs) {
    પાછા lhs.m_e > rhs.m_e;
}
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VTimescale& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
જાહેર:
    ગણના en : uint8_t {
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
    explicit VFsmExpandType(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    constexpr અચલ અક્ષર* ascii() અચલ {
        constexpr અચલ અક્ષર* અચલ names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        પાછા names[m_e];
    }
};
constexpr bool operator==(અચલ VFsmExpandType& lhs, અચલ VFsmExpandType& rhs) VL_MT_SAFE {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, અચલ VFsmExpandType& rhs) VL_MT_SAFE {
    પાછા lhs == rhs.m_e;
}
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ VFsmExpandType& rhs) VL_MT_SAFE {
    પાછા os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
જાહેર:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<અચલ std::string, std::string>;

ખાનગી:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

જાહેર:
    explicit V3HierarchicalBlockOption(અચલ string& optstring);
    અચલ string& origName() અચલ { પાછા m_origName; }
    અચલ string& mangledName() અચલ { પાછા m_mangledName; }
    અચલ ParamStrMap params() અચલ { પાછા m_parameters; }
};

using V3HierBlockOptSet = std::map<અચલ std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
જાહેર:
ખાનગી:
    // TYPES
    using DebugLevelMap = std::map<અચલ std::string, unsigned>;

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
    std::map<અચલ string, string> m_parameters;  // Parameters
    std::map<અચલ string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_લીટી = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_દ્વિઅંકી = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_પદાવલિ = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_લીટી = false;    // main switch: --coverage-block
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
    bool m_debug_ઢગલો_Check = false;  // main switch: --debug-stack-check
    પૂર્ણાંક m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_no_Trace_ટોચ = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    પૂર્ણાંક         m_buildJobs = -1;    // main switch: --build-jobs, -j
    પૂર્ણાંક         m_coverage_પદાવલિ_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    પૂર્ણાંક         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    પૂર્ણાંક         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    પૂર્ણાંક         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    પૂર્ણાંક         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    પૂર્ણાંક         m_expandLimit = 256;  // main switch: --expand-limit
    પૂર્ણાંક         m_gateStmts = 100;    // main switch: --gate-stmts
    પૂર્ણાંક         m_hierChild = 0;      // main switch: --hierarchical-child
    પૂર્ણાંક         m_hierThreads = 0;      // main switch: --hierarchical-threads
    પૂર્ણાંક         m_ifDepth = 0;      // main switch: --if-depth
    પૂર્ણાંક         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    પૂર્ણાંક         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    પૂર્ણાંક         m_inlineMult = 2000;   // main switch: --inline-mult
    પૂર્ણાંક         m_instr_ગણતરી_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    પૂર્ણાંક         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    પૂર્ણાંક         m_max_અંક_Width = 65536;  // main switch: --max-num-width
    પૂર્ણાંક         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    પૂર્ણાંક         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    પૂર્ણાંક         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    પૂર્ણાંક         m_outputSplit = 20000;  // main switch: --output-split
    પૂર્ણાંક         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    પૂર્ણાંક         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    પૂર્ણાંક         m_pinsBv = 65;       // main switch: --pins-bv
    પૂર્ણાંક         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    પૂર્ણાંક         m_publicDepth = 0;   // main switch: --public-depth
    પૂર્ણાંક         m_reloopLimit = 40; // main switch: --reloop-limit
    પૂર્ણાંક         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    પૂર્ણાંક         m_threads = 1;      // main switch: --threads
    પૂર્ણાંક         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    પૂર્ણાંક         m_traceDepth = 0;   // main switch: --trace-depth
    પૂર્ણાંક         m_traceMaxArray = 32;  // main switch: --trace-max-array
    પૂર્ણાંક         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    પૂર્ણાંક         m_unroll_ગણતરી = 64;  // main switch: --unroll-count
    પૂર્ણાંક         m_unrollLimit = 16384;  // main switch: --unroll-limit
    પૂર્ણાંક         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    પૂર્ણાંક         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    પૂર્ણાંક         m_verilateJobs = -1;  // main switch: --verilate-jobs

    પૂર્ણાંક         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    પૂર્ણાંક         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    પૂર્ણાંક         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_ટોચ_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_ચાવી;   // main switch: --protect-key
    string      m_ટોચ_Module;    // main switch: --top-module
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
    bool m_f_Case_વૃક્ષ;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_વૃક્ષ;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ધકેલો_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    પૂર્ણાંક  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    શૂન્ય addArg(અક્ષર** argv, size_t ગણતરી, bool isForRerun);
    શૂન્ય addArg(અચલ std::string& arg, bool isForRerun);
    શૂન્ય add_લીટી_Arg(અચલ string& arg);
    શૂન્ય addDefine(અચલ string& defline, bool allowPlus) VL_MT_DISABLED;
    શૂન્ય addFuture(અચલ string& flag);
    શૂન્ય addFuture0(અચલ string& flag);
    શૂન્ય addFuture1(અચલ string& flag);
    શૂન્ય addIncDirUser(અચલ string& incdir);  // User requested
    શૂન્ય addIncDirFallback(અચલ string& incdir);  // Low priority if not found otherwise
    શૂન્ય addParameter(અચલ string& paramline, bool allowPlus);
    શૂન્ય addLangExt(અચલ string& langext, અચલ V3LangCode& lc);
    શૂન્ય addLibExtV(અચલ string& libext);
    શૂન્ય optimize(પૂર્ણાંક level);
    શૂન્ય showVersion(bool verbose);
    શૂન્ય coverage(bool flag) {
        m_coverage_લીટી = m_coverageBranch = m_coverageToggle = m_coverage_પદાવલિ = m_coverageFsm
            = m_coverageUser = flag;
    }
    સ્થિર bool suffixed(અચલ string& sw, અચલ અક્ષર* arg);
    સ્થિર string parseFileArg(અચલ string& optdir, અચલ string& relfilename);
    string filePathCheckOneDir(અચલ string& modname, અચલ string& dirname);
    સ્થિર પૂર્ણાંક stripOptionsForChildRun(અચલ string& opt, bool for_ટોચ);
    શૂન્ય validateIdentifier(File_લીટી* fl, અચલ string& arg, અચલ string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

જાહેર:
    V3Options();
    ~V3Options();
    શૂન્ય setDebugMode(પૂર્ણાંક level);
    unsigned debugLevel(અચલ string& tag) અચલ VL_MT_SAFE;
    unsigned debugSrcLevel(અચલ string& srcfile_path) અચલ VL_MT_SAFE;
    unsigned dumpLevel(અચલ string& tag) અચલ VL_MT_SAFE;
    unsigned dumpSrcLevel(અચલ string& srcfile_path) અચલ VL_MT_SAFE;

    // METHODS
    શૂન્ય addCppFile(અચલ string& filename);
    શૂન્ય addCFlags(અચલ string& filename);
    શૂન્ય addCompilerIncludes(અચલ string& filename);
    શૂન્ય addLdLibFile(અચલ string& filename);
    શૂન્ય addLdLibs(અચલ string& filename);
    શૂન્ય addMakeFlags(અચલ string& filename);
    શૂન્ય addLibMapFile(અચલ string& filename);
    શૂન્ય addLibraryFile(અચલ string& filename, અચલ string& libname);
    શૂન્ય addVFile(અચલ string& filename, અચલ string& libname);
    શૂન્ય addVltFile(અચલ string& filename, અચલ string& libname);
    શૂન્ય addForceInc(અચલ string& filename);
    bool available() અચલ VL_MT_SAFE { પાછા m_available; }
    શૂન્ય ccSet();
    શૂન્ય decorations(File_લીટી* fl, અચલ string& filename);
    શૂન્ય notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() અચલ { પાછા m_preprocOnly; }
    bool makePhony() અચલ { પાછા m_makePhony; }
    bool preproc_No_લીટી() અચલ { પાછા m_preproc_No_લીટી; }
    bool preprocResolve() અચલ { પાછા m_preprocResolve; }
    પૂર્ણાંક preprocTokenLimit() અચલ { પાછા m_preprocTokenLimit; }
    bool underlineZero() અચલ { પાછા m_underlineZero; }
    bool systemC() અચલ VL_MT_SAFE { પાછા m_systemC; }
    bool savable() અચલ VL_MT_SAFE { પાછા m_savable; }
    VOptionBool schedZeroDelay() અચલ { પાછા m_schedZeroDelay; }
    bool stats() અચલ { પાછા m_stats; }
    bool statsVars() અચલ { પાછા m_statsVars; }
    bool stdPackage() અચલ { પાછા m_stdPackage; }
    bool stdWaiver() અચલ { પાછા m_stdWaiver; }
    bool assertOn() અચલ { પાછા m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() અચલ { પાછા m_assertCase; }
    bool autoflush() અચલ { પાછા m_autoflush; }
    bool bboxSys() અચલ { પાછા m_bboxSys; }
    bool bboxUnsup() અચલ { પાછા m_bboxUnsup; }
    bool build() અચલ { પાછા m_build; }
    string buildDepBin() અચલ { પાછા m_buildDepBin; }
    શૂન્ય buildDepBin(અચલ string& flag) { m_buildDepBin = flag; }
    bool context() અચલ VL_MT_SAFE { પાછા m_context; }
    bool coverage() અચલ VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        પાછા m_coverage_લીટી || m_coverageBranch || m_coverageToggle || m_coverage_પદાવલિ
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() અચલ VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        પાછા m_coverage_લીટી || m_coverageBranch || m_coverageToggle || m_coverage_પદાવલિ
               || m_coverageUser;
    }
    bool coverageBranch() અચલ { પાછા m_coverageBranch; }
    bool coverage_પદાવલિ() અચલ { પાછા m_coverage_પદાવલિ; }
    bool coverageFsm() અચલ { પાછા m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() અચલ { પાછા m_coverageFsmExpand; }
    પૂર્ણાંક coverageFsmMaxArcs() અચલ { પાછા m_coverageFsmMaxArcs; }
    bool coverage_લીટી() અચલ { પાછા m_coverage_લીટી; }
    bool coverageMergeInstances() અચલ { પાછા m_coverageMergeInstances; }
    bool coveragePerInstance() અચલ { પાછા m_coveragePerInstance; }
    bool coverageToggle() અચલ { પાછા m_coverageToggle; }
    bool coverageUnderscore() અચલ { પાછા m_coverageUnderscore; }
    bool coverageUser() અચલ { પાછા m_coverageUser; }
    bool debugCheck() અચલ VL_MT_SAFE { પાછા m_debugCheck; }
    bool debugCollision() અચલ { પાછા m_debugCollision; }
    bool debugEmitV() અચલ VL_MT_SAFE { પાછા m_debugEmitV; }
    bool debugExitElab() અચલ { પાછા m_debugExitElab; }
    bool debugExitParse() અચલ { પાછા m_debugExitParse; }
    bool debugLeak() અચલ { પાછા m_debugLeak; }
    bool debugNondeterminism() અચલ { પાછા m_debugNondeterminism; }
    bool debugPartition() અચલ { પાછા m_debugPartition; }
    bool debugPreprocPassthru() અચલ VL_MT_SAFE { પાછા m_debugPreprocPassthru; }
    bool debugProtect() અચલ VL_MT_SAFE { પાછા m_debugProtect; }
    પૂર્ણાંક debugRuntimeTimeout() અચલ { પાછા m_debugRuntimeTimeout; }
    bool debugSelfTest() અચલ { પાછા m_debugSelfTest; }
    bool debug_ઢગલો_Check() અચલ { પાછા m_debug_ઢગલો_Check; }
    bool debugWidth() અચલ VL_PURE { પાછા m_debugWidth; }
    bool decoration() અચલ VL_MT_SAFE { પાછા m_decoration; }
    bool decorationNodes() અચલ VL_MT_SAFE { પાછા m_decorationNodes; }
    bool diagnosticsSarif() અચલ VL_MT_SAFE { પાછા m_diagnosticsSarif; }
    bool dpiHdrOnly() અચલ { પાછા m_dpiHdrOnly; }
    bool dumpAstPatterns() અચલ {
        પાછા m_dumpLevel.ગણતરી("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() અચલ { પાછા m_dumpLevel.ગણતરી("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() અચલ {
        પાછા m_dumpLevel.ગણતરી("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_વૃક્ષ_Dot() અચલ {
        પાછા m_dumpLevel.ગણતરી("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() અચલ { પાછા m_emitAccessors; }
    bool exe() અચલ { પાછા m_exe; }
    bool flatten() અચલ { પાછા m_flatten; }
    bool fourstate() અચલ { પાછા m_fourstate; }
    bool gmake() અચલ { પાછા m_gmake; }
    bool makeJson() અચલ { પાછા m_makeJson; }
    bool threadsDpiPure() અચલ { પાછા m_threadsDpiPure; }
    bool threadsDpiUnpure() અચલ { પાછા m_threadsDpiUnpure; }
    bool threadsCoarsen() અચલ { પાછા m_threadsCoarsen; }
    VOptionBool timing() અચલ { પાછા m_timing; }
    bool trace() અચલ { પાછા m_trace; }
    bool traceCoverage() અચલ { પાછા m_traceCoverage; }
    bool traceEnabledFst() અચલ { પાછા m_traceEnabledFst; }
    bool traceEnabledSaif() અચલ { પાછા m_traceEnabledSaif; }
    bool traceEnabledVcd() અચલ { પાછા m_traceEnabledVcd; }
    bool traceParams() અચલ { પાછા m_traceParams; }
    bool traceStructs() અચલ { પાછા m_traceStructs; }
    bool traceUnderscore() અચલ { પાછા m_traceUnderscore; }
    bool મુખ્ય() અચલ { પાછા m_main; }
    bool outFormatOk() અચલ { પાછા m_outFormatOk; }
    bool jsonOnly() અચલ { પાછા m_jsonOnly; }
    bool keepTempFiles() અચલ { પાછા (V3Error::debugDefault() != 0); }
    bool pedantic() અચલ { પાછા m_pedantic; }
    bool pinsInoutEnables() અચલ { પાછા m_pinsInoutEnables; }
    bool pinsScUint() અચલ { પાછા m_pinsScUint; }
    bool pinsScUintBool() અચલ { પાછા m_pinsScUintBool; }
    bool pinsScBigUint() અચલ VL_MT_SAFE { પાછા m_pinsScBigUint; }
    bool pinsUint8() અચલ { પાછા m_pinsUint8; }
    bool preprocComments() અચલ { પાછા m_preprocComments; }
    bool preprocDefines() અચલ { પાછા m_preprocDefines; }
    bool profC() અચલ { પાછા m_profC; }
    bool profCFuncs() અચલ { પાછા m_profCFuncs; }
    bool profExec() અચલ { પાછા m_profExec; }
    bool profPgo() અચલ { પાછા m_profPgo; }
    bool usesProfiler() અચલ { પાછા profExec() || profPgo(); }
    bool protectIds() અચલ VL_MT_SAFE { પાછા m_protectIds; }
    bool allPublic() અચલ { પાછા m_public; }
    bool publicParams() અચલ { પાછા m_publicParams; }
    bool publicOff() અચલ { પાછા m_publicIgnore; }
    bool publicFlatRW() અચલ { પાછા m_publicFlatRW; }
    પૂર્ણાંક publicDepth() અચલ { પાછા m_publicDepth; }
    bool anyPublicFlat() અચલ { પાછા m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() અચલ VL_MT_SAFE { પાછા m_lintOnly; }
    bool ignc() અચલ { પાછા m_ignc; }
    bool quietBuild() અચલ VL_MT_SAFE { પાછા m_quietBuild; }
    bool quietExit() અચલ VL_MT_SAFE { પાછા m_quietExit; }
    bool quietStats() અચલ VL_MT_SAFE { પાછા m_quietStats; }
    bool reportUnoptflat() અચલ { પાછા m_reportUnoptflat; }
    bool verilate() અચલ { પાછા m_verilate; }
    bool vpi() અચલ { પાછા m_vpi.isTrue(); }
    bool waiverMultiline() અચલ { પાછા m_waiverMultiline; }
    bool xInitialEdge() અચલ { પાછા m_xInitialEdge; }
    bool serializeOnly() અચલ { પાછા m_jsonOnly; }
    bool ટોચ_Ifaces_Supported() અચલ { પાછા lintOnly() && !hierarchical(); }

    પૂર્ણાંક buildJobs() અચલ VL_MT_SAFE { પાછા m_buildJobs; }
    પૂર્ણાંક convergeLimit() અચલ { પાછા m_convergeLimit; }
    પૂર્ણાંક coverage_પદાવલિ_Max() અચલ { પાછા m_coverage_પદાવલિ_Max; }
    uint32_t coverageMaxBins() અચલ { પાછા m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() અચલ { પાછા m_coverageMaxRealBins; }
    પૂર્ણાંક coverageMaxWidth() અચલ { પાછા m_coverageMaxWidth; }
    પૂર્ણાંક debugAllocRandom() અચલ { પાછા m_debugAllocRandom; }
    bool dump_વૃક્ષ_Addrids() અચલ VL_MT_SAFE;
    પૂર્ણાંક expandLimit() અચલ { પાછા m_expandLimit; }
    પૂર્ણાંક gateStmts() અચલ { પાછા m_gateStmts; }
    પૂર્ણાંક ifDepth() અચલ { પાછા m_ifDepth; }
    પૂર્ણાંક inlineCFuncs() અચલ { પાછા m_inlineCFuncs; }
    પૂર્ણાંક inlineCFuncsProduct() અચલ { પાછા m_inlineCFuncsProduct; }
    પૂર્ણાંક inlineMult() અચલ { પાછા m_inlineMult; }
    પૂર્ણાંક instr_ગણતરી_Dpi() અચલ { પાછા m_instr_ગણતરી_Dpi; }
    પૂર્ણાંક localizeMaxSize() અચલ { પાછા m_localizeMaxSize; }
    bool jsonEditNums() અચલ { પાછા m_jsonEditNums; }
    bool jsonIds() અચલ { પાછા m_jsonIds; }
    VOptionBool makeDepend() અચલ { પાછા m_makeDepend; }
    પૂર્ણાંક max_અંક_Width() અચલ { પાછા m_max_અંક_Width; }
    પૂર્ણાંક funcRecursionDepth() અચલ { પાછા m_funcRecursion; }
    પૂર્ણાંક moduleRecursionDepth() અચલ { પાછા m_moduleRecursion; }
    પૂર્ણાંક outputSplit() અચલ { પાછા m_outputSplit; }
    પૂર્ણાંક outputSplitCFuncs() અચલ { પાછા m_outputSplitCFuncs; }
    પૂર્ણાંક outputSplitCTrace() અચલ { પાછા m_outputSplitCTrace; }
    પૂર્ણાંક outputGroups() અચલ { પાછા m_outputGroups; }
    int64_t outputGroupsSize() અચલ { પાછા m_outputGroupsSize; }
    પૂર્ણાંક pinsBv() અચલ VL_MT_SAFE { પાછા m_pinsBv; }
    પૂર્ણાંક reloopLimit() અચલ { પાછા m_reloopLimit; }
    પૂર્ણાંક replicationLimit() અચલ { પાછા m_replicationLimit; }
    VOptionBool skipIdentical() અચલ { પાછા m_skipIdentical; }
    bool stopFail() અચલ { પાછા m_stopFail; }
    પૂર્ણાંક threads() અચલ VL_MT_SAFE { પાછા m_threads; }
    પૂર્ણાંક threadsMaxMTasks() અચલ { પાછા m_threadsMaxMTasks; }
    bool mtasks() અચલ VL_MT_SAFE { પાછા (m_threads > 1); }
    VTimescale timeDefaultPrec() અચલ { પાછા m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() અચલ { પાછા m_timeDefaultUnit; }
    VTimescale timeOverridePrec() અચલ { પાછા m_timeOverridePrec; }
    VTimescale timeOverrideUnit() અચલ { પાછા m_timeOverrideUnit; }
    VTimescale timeComputePrec(અચલ VTimescale& flag) અચલ;
    VTimescale timeComputeUnit(અચલ VTimescale& flag) અચલ;
    પૂર્ણાંક traceDepth() અચલ { પાછા m_traceDepth; }
    પૂર્ણાંક traceMaxArray() અચલ { પાછા m_traceMaxArray; }
    પૂર્ણાંક traceMaxWidth() અચલ { પાછા m_traceMaxWidth; }
    bool useTraceParallel() અચલ {
        પાછા trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    પૂર્ણાંક unroll_ગણતરી() અચલ { પાછા m_unroll_ગણતરી; }
    પૂર્ણાંક unrollLimit() અચલ { પાછા m_unrollLimit; }
    પૂર્ણાંક unrollStmts() અચલ { પાછા m_unrollStmts; }
    પૂર્ણાંક constraintArrayLimit() અચલ { પાછા m_constraintArrayLimit; }
    પૂર્ણાંક verilateJobs() અચલ { પાછા m_verilateJobs; }

    પૂર્ણાંક compLimitBlocks() અચલ { પાછા m_compLimitBlocks; }
    પૂર્ણાંક compLimitMembers() અચલ VL_MT_SAFE { પાછા m_compLimitMembers; }
    પૂર્ણાંક compLimitParens() અચલ { પાછા m_compLimitParens; }

    string diagnosticsSarifOutput() અચલ VL_MT_SAFE {
        પાછા m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() અચલ { પાછા m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() અચલ { પાછા m_hierParamsFile; }
    string jsonOnlyOutput() અચલ { પાછા m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() અચલ { પાછા m_jsonOnlyMetaOutput; }
    string l2Name() અચલ { પાછા m_l2Name; }
    string libCreate() અચલ { પાછા m_libCreate; }
    string libCreateName(bool shared) અચલ {
        string libName = "lib" + libCreate();
        જો (shared) {
            libName += ".so";
        } નહીંતર {
            libName += ".a";
        }
        પાછા libName;
    }
    string main_ટોચ_Name() અચલ { પાછા m_main_ટોચ_Name; }
    string makeDir() અચલ VL_MT_SAFE { પાછા m_makeDir; }
    string modPrefix() અચલ VL_MT_SAFE { પાછા m_modPrefix; }
    string pipeFilter() અચલ { પાછા m_pipeFilter; }
    string prefix() અચલ VL_MT_SAFE { પાછા m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_ચાવી_Provided() અચલ { પાછા !m_protect_ચાવી.empty(); }
    string protect_ચાવી_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string ટોચ_Module() અચલ { પાછા m_ટોચ_Module; }  // As AstNode::encodeName()
    bool no_Trace_ટોચ() અચલ { પાછા m_no_Trace_ટોચ; }
    string unusedRegexp() અચલ { પાછા m_unusedRegexp; }
    string waiverOutput() અચલ { પાછા m_waiverOutput; }
    string work() અચલ { પાછા m_work; }
    bool isWaiverOutput() અચલ { પાછા !m_waiverOutput.empty(); }
    string xAssign() અચલ { પાછા m_xAssign; }
    string xInitial() અચલ { પાછા m_xInitial; }

    અચલ VStringSet& cppFiles() અચલ { પાછા m_cppFiles; }
    અચલ VStringList& cFlags() અચલ { પાછા m_cFlags; }
    અચલ VStringSet& compilerIncludes() અચલ { પાછા m_compilerIncludes; }
    અચલ VStringList& ldLibFiles() અચલ { પાછા m_ldLibFiles; }
    અચલ VStringList& ldLibs() અચલ { પાછા m_ldLibs; }
    અચલ VStringList& makeFlags() અચલ { પાછા m_makeFlags; }
    અચલ VFileLibSet& libraryFiles() અચલ { પાછા m_libraryFiles; }
    અચલ VStringSet& libmapFiles() અચલ { પાછા m_libmapFiles; }
    અચલ VFileLibList& vFiles() અચલ { પાછા m_vFiles; }
    અચલ VFileLibSet& vltFiles() અચલ { પાછા m_vltFiles; }
    અચલ VStringList& forceIncs() અચલ { પાછા m_forceIncs; }

    bool hasParameter(અચલ string& name);
    string parameter(અચલ string& name);
    શૂન્ય checkParameters();

    bool isFuture(અચલ string& flag) અચલ;
    bool isFuture0(અચલ string& flag) અચલ;
    bool isFuture1(અચલ string& flag) અચલ;
    bool isLibraryFile(અચલ string& filename, અચલ string& libname) અચલ;

    // ACCESSORS (optimization options)
    bool fAcycSimp() અચલ { પાછા m_fAcycSimp; }
    bool fAssemble() અચલ { પાછા m_fAssemble; }
    bool fBitScanLoops() અચલ { પાછા m_fBitScanLoops; }
    bool fBitblast() અચલ { પાછા m_fBitblast; }
    bool fCaseDecoder() અચલ { પાછા m_fCaseDecoder; }
    bool fCaseTable() અચલ { પાછા m_fCaseTable; }
    bool f_Case_વૃક્ષ() અચલ { પાછા m_f_Case_વૃક્ષ; }
    bool fCombine() અચલ { પાછા m_fCombine; }
    bool fConst() અચલ { પાછા m_fConst; }
    bool fConstBeforeDfg() અચલ { પાછા m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_વૃક્ષ() અચલ { પાછા m_f_Const_Bit_Op_વૃક્ષ; }
    bool fConstEager() અચલ { પાછા m_fConstEager; }
    bool fDedupe() અચલ { પાછા m_fDedupe; }
    bool fDfg() અચલ { પાછા m_fDfg; }
    bool fDfgPeephole() અચલ { પાછા m_fDfgPeephole; }
    bool f_Dfg_ધકેલો_Down_Sels() અચલ { પાછા m_f_Dfg_ધકેલો_Down_Sels; }
    bool fDfgSynthesizeAll() અચલ { પાછા m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(અચલ std::string& name) અચલ {
        પાછા !m_fDfgPeepholeDisabled.ગણતરી(name);
    }
    bool fDeadAssigns() અચલ { પાછા m_fDeadAssigns; }
    bool fDeadCells() અચલ { પાછા m_fDeadCells; }
    bool fDeadMethods() અચલ { પાછા m_fDeadMethods; }
    bool fDecompose() અચલ { પાછા m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() અચલ { પાછા m_fDecomposePacked; }
    bool fDecomposeUnpacked() અચલ { પાછા m_fDecomposeUnpacked; }
    bool fExpand() અચલ { પાછા m_fExpand; }
    bool fFuncBalanceCat() અચલ { પાછા m_fFuncBalanceCat; }
    bool fFuncSplitCat() અચલ { પાછા m_fFuncSplitCat; }
    bool fFunc() અચલ { પાછા fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() અચલ { પાછા m_fGate; }
    VOptionBool fIcoChangeDetect() અચલ { પાછા m_fIcoChangeDetect; }
    bool fInline() અચલ { પાછા m_fInline; }
    bool fInlineCFuncs() અચલ { પાછા m_fInlineCFuncs; }
    bool fInlineFuncs() અચલ { પાછા m_fInlineFuncs; }
    bool fInlineFuncsEager() અચલ { પાછા m_fInlineFuncsEager; }
    bool fLife() અચલ { પાછા m_fLife; }
    bool fLifePost() અચલ { પાછા m_fLifePost; }
    bool fLocalize() અચલ { પાછા m_fLocalize; }
    bool fMergeCond() અચલ { પાછા m_fMergeCond; }
    bool fMergeCondMotion() અચલ { પાછા m_fMergeCondMotion; }
    bool fReloop() અચલ { પાછા m_fReloop; }
    bool fReorder() અચલ { પાછા m_fReorder; }
    bool fSlice() અચલ { પાછા m_fSlice; }
    પૂર્ણાંક fSliceElementLimit() અચલ { પાછા m_fSliceElementLimit; }
    bool fSplit() અચલ { પાછા m_fSplit; }
    bool fSubst() અચલ { પાછા m_fSubst; }
    bool fSubstConst() અચલ { પાછા m_fSubstConst; }
    bool fTable() અચલ { પાછા m_fTable; }
    bool fTaskifyAll() અચલ { પાછા m_fTaskifyAll; }

    std::string traceClassBase() અચલ VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() અચલ VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() અચલ VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() અચલ VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() અચલ VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() અચલ VL_MT_SAFE;

    bool hierarchical() અચલ { પાછા m_hierarchical; }
    પૂર્ણાંક hierChild() અચલ VL_MT_SAFE { પાછા m_hierChild; }
    પૂર્ણાંક hierThreads() અચલ VL_MT_SAFE { પાછા m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_ટોચ() અચલ VL_MT_SAFE { પાછા !m_hierChild && !m_hierBlocks.empty(); }
    અચલ V3HierBlockOptSet& hierBlocks() અચલ { પાછા m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_ટોચ_Data_Dir() અચલ VL_MT_SAFE {
        પાછા hier_ટોચ() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    સ્થિર string version() VL_PURE;
    સ્થિર string argString(પૂર્ણાંક argc, અક્ષર** argv);  ///< Return list of arguments as simple string
    string allArgsString() અચલ VL_MT_SAFE;  ///< Return all passed arguments as simple string
    અચલ std::list<std::pair<std::list<std::string>, bool>>& allArgs() અચલ;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_ટોચ) અચલ;
    શૂન્ય parseOpts(File_લીટી* fl, પૂર્ણાંક argc, અક્ષર** argv) VL_MT_DISABLED;
    શૂન્ય parseOptsList(File_લીટી* fl, અચલ string& optdir, પૂર્ણાંક argc, અક્ષર** argv) VL_MT_DISABLED;
    શૂન્ય parseOptsFile(File_લીટી* fl, અચલ string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    સ્થિર string getenvBuiltins(અચલ string& var);
    સ્થિર string getenvMAKE();
    સ્થિર string getenvMAKEFLAGS();
    સ્થિર string getenvPERL();
    સ્થિર string getenvPYTHON3();
    સ્થિર string getenvSYSTEMC();
    સ્થિર string getenvSYSTEMC_ARCH();
    સ્થિર string getenvSYSTEMC_INCLUDE();
    સ્થિર string getenvSYSTEMC_LIBDIR();
    સ્થિર string getenv_VERILATOR_મૂળ();
    સ્થિર string getenvVERILATOR_SOLVER();
    સ્થિર string getStdPackagePath();
    સ્થિર string getStdWaiverPath();
    સ્થિર string getSupported(અચલ string& var);
    સ્થિર bool systemCSystemWide();
    સ્થિર bool systemCFound();  // SystemC installed, or environment points to it
    સ્થિર bool coroutineSupport();  // Configured C++ compiler supports coroutines
    સ્થિર bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    સ્થિર bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    સ્થિર bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(અચલ string& filename);
    string filePath(File_લીટી* fl, અચલ string& modname, અચલ string& lastpath,
                    અચલ string& errmsg);
    string file_Path_Looked_સંદેશ(File_લીટી* fl, અચલ string& modname);
    V3LangCode fileLanguage(અચલ string& filename);
    સ્થિર bool fileStatNormal(અચલ string& filename);

    // METHODS (other OS)
    સ્થિર શૂન્ય throwSigsegv();
};

//######################################################################

#endif  // guard
