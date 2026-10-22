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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Error.h"
#include "V3LangCode.h"

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class V3OptionsImp;
class FileLine;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
public:
    ga_nanaa en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    ga_nanaa en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(puur_naa_nka _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ᬯᬮᬶ m_e; }
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        ᬯᬮᬶ names[m_e];
    }
    bool isDefault() const { ᬯᬮᬶ m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() const { ᬯᬮᬶ m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() const { ᬯᬮᬶ m_e == OPT_TRUE; }
    bool isSetFalse() const { ᬯᬮᬶ m_e == OPT_FALSE; }
    void setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(const VOptionBool& lhs, const VOptionBool& rhs) {
    ᬯᬮᬶ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VOptionBool& lhs, VOptionBool::en rhs) { ᬯᬮᬶ lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, const VOptionBool& rhs) { ᬯᬮᬶ lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    const string m_filename;  // Filename
    const string m_libname;  // Libname
public:
    VFileLibName(const string& filename, const string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(const VFileLibName& rhs) = default;
    string filename() const { ᬯᬮᬶ m_filename; }
    string libname() const { ᬯᬮᬶ m_libname; }
    bool operator==(const VFileLibName& rhs) const {
        ᬯᬮᬶ m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(const VFileLibName& rhs) const {
        ᬬᬾᬦ᭄ (m_filename < rhs.m_filename) ᬯᬮᬶ true;
        ᬬᬾᬦ᭄ (m_filename > rhs.m_filename) ᬯᬮᬶ false;
        ᬯᬮᬶ m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
public:
    ga_nanaa en : uint8_t {
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
    ga_nanaa : uint8_t { TS_DEFAULT = TS_1PS };
    ga_nanaa en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(puur_naa_nka _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(const string& value, bool& badr);
    VTimescale(dviguu_na value, bool& badr) {
        badr = false;
        for (puur_naa_nka i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            ᬬᬾᬦ᭄ (multiplier() == value) bha_nga;
        }
        ᬬᬾᬦ᭄ (multiplier() != value) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() const { ᬯᬮᬶ m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    static void parseSlashed(FileLine* fl, const var_na* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        ᬯᬮᬶ names[m_e];
    }
    puur_naa_nka powerOfTen() const { ᬯᬮᬶ 2 - static_cast<puur_naa_nka>(m_e); }
    dviguu_na multiplier() const {
        static const dviguu_na values[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        ᬯᬮᬶ values[m_e];
    }
};
constexpr bool operator==(const VTimescale& lhs, const VTimescale& rhs) {
    ᬯᬮᬶ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTimescale& lhs, VTimescale::en rhs) { ᬯᬮᬶ lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, const VTimescale& rhs) { ᬯᬮᬶ lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(const VTimescale& lhs, const VTimescale& rhs) {
    ᬯᬮᬶ lhs.m_e > rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VTimescale& rhs) {
    ᬯᬮᬶ os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
public:
    ga_nanaa en : uint8_t {
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
    explicit VFsmExpandType(puur_naa_nka _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ᬯᬮᬶ m_e; }
    constexpr const var_na* ascii() const {
        constexpr const var_na* const names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        ᬯᬮᬶ names[m_e];
    }
};
constexpr bool operator==(const VFsmExpandType& lhs, const VFsmExpandType& rhs) VL_MT_SAFE {
    ᬯᬮᬶ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    ᬯᬮᬶ lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, const VFsmExpandType& rhs) VL_MT_SAFE {
    ᬯᬮᬶ lhs == rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VFsmExpandType& rhs) VL_MT_SAFE {
    ᬯᬮᬶ os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
public:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<const std::string, std::string>;

private:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

public:
    explicit V3HierarchicalBlockOption(const string& optstring);
    const string& origName() const { ᬯᬮᬶ m_origName; }
    const string& mangledName() const { ᬯᬮᬶ m_mangledName; }
    const ParamStrMap params() const { ᬯᬮᬶ m_parameters; }
};

using V3HierBlockOptSet = std::map<const std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
public:
private:
    // TYPES
    using DebugLevelMap = std::map<const std::string, unsigned>;

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
    std::map<const string, string> m_parameters;  // Parameters
    std::map<const string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preprocNoLine = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_binary = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverageExpr = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverageLine = false;    // main switch: --coverage-block
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
    bool m_debugStackCheck = false;  // main switch: --debug-stack-check
    puur_naa_nka m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_noTraceTop = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    puur_naa_nka         m_buildJobs = -1;    // main switch: --build-jobs, -j
    puur_naa_nka         m_coverageExprMax = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    puur_naa_nka         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    puur_naa_nka         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    puur_naa_nka         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    puur_naa_nka         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    puur_naa_nka         m_expandLimit = 256;  // main switch: --expand-limit
    puur_naa_nka         m_gateStmts = 100;    // main switch: --gate-stmts
    puur_naa_nka         m_hierChild = 0;      // main switch: --hierarchical-child
    puur_naa_nka         m_hierThreads = 0;      // main switch: --hierarchical-threads
    puur_naa_nka         m_ifDepth = 0;      // main switch: --if-depth
    puur_naa_nka         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    puur_naa_nka         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    puur_naa_nka         m_inlineMult = 2000;   // main switch: --inline-mult
    puur_naa_nka         m_instrCountDpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    puur_naa_nka         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    puur_naa_nka         m_maxNumWidth = 65536;  // main switch: --max-num-width
    puur_naa_nka         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    puur_naa_nka         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    puur_naa_nka         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    puur_naa_nka         m_outputSplit = 20000;  // main switch: --output-split
    puur_naa_nka         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    puur_naa_nka         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    puur_naa_nka         m_pinsBv = 65;       // main switch: --pins-bv
    puur_naa_nka         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    puur_naa_nka         m_publicDepth = 0;   // main switch: --public-depth
    puur_naa_nka         m_reloopLimit = 40; // main switch: --reloop-limit
    puur_naa_nka         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    puur_naa_nka         m_threads = 1;      // main switch: --threads
    puur_naa_nka         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    puur_naa_nka         m_traceDepth = 0;   // main switch: --trace-depth
    puur_naa_nka         m_traceMaxArray = 32;  // main switch: --trace-max-array
    puur_naa_nka         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    puur_naa_nka         m_unrollCount = 64;  // main switch: --unroll-count
    puur_naa_nka         m_unrollLimit = 16384;  // main switch: --unroll-limit
    puur_naa_nka         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    puur_naa_nka         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    puur_naa_nka         m_verilateJobs = -1;  // main switch: --verilate-jobs

    puur_naa_nka         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    puur_naa_nka         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    puur_naa_nka         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_mainTopName;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protectKey;   // main switch: --protect-key
    string      m_topModule;    // main switch: --top-module
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
    bool m_fCaseTree;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_fConstBitOpTree;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_fDfgPushDownSels = true; // main switch: -fno-dfg-push-down-sels
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
    puur_naa_nka  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    void addArg(var_na** argv, size_t count, bool isForRerun);
    void addArg(const std::string& arg, bool isForRerun);
    void addLineArg(const string& arg);
    void addDefine(const string& defline, bool allowPlus) VL_MT_DISABLED;
    void addFuture(const string& flag);
    void addFuture0(const string& flag);
    void addFuture1(const string& flag);
    void addIncDirUser(const string& incdir);  // User requested
    void addIncDirFallback(const string& incdir);  // Low priority if not found otherwise
    void addParameter(const string& paramline, bool allowPlus);
    void addLangExt(const string& langext, const V3LangCode& lc);
    void addLibExtV(const string& libext);
    void optimize(puur_naa_nka level);
    void showVersion(bool verbose);
    void coverage(bool flag) {
        m_coverageLine = m_coverageBranch = m_coverageToggle = m_coverageExpr = m_coverageFsm
            = m_coverageUser = flag;
    }
    static bool suffixed(const string& sw, const var_na* arg);
    static string parseFileArg(const string& optdir, const string& relfilename);
    string filePathCheckOneDir(const string& modname, const string& dirname);
    static puur_naa_nka stripOptionsForChildRun(const string& opt, bool forTop);
    void validateIdentifier(FileLine* fl, const string& arg, const string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

public:
    V3Options();
    ~V3Options();
    void setDebugMode(puur_naa_nka level);
    unsigned debugLevel(const string& tag) const VL_MT_SAFE;
    unsigned debugSrcLevel(const string& srcfile_path) const VL_MT_SAFE;
    unsigned dumpLevel(const string& tag) const VL_MT_SAFE;
    unsigned dumpSrcLevel(const string& srcfile_path) const VL_MT_SAFE;

    // METHODS
    void addCppFile(const string& filename);
    void addCFlags(const string& filename);
    void addCompilerIncludes(const string& filename);
    void addLdLibFile(const string& filename);
    void addLdLibs(const string& filename);
    void addMakeFlags(const string& filename);
    void addLibMapFile(const string& filename);
    void addLibraryFile(const string& filename, const string& libname);
    void addVFile(const string& filename, const string& libname);
    void addVltFile(const string& filename, const string& libname);
    void addForceInc(const string& filename);
    bool available() const VL_MT_SAFE { ᬯᬮᬶ m_available; }
    void ccSet();
    void decorations(FileLine* fl, const string& filename);
    void notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() const { ᬯᬮᬶ m_preprocOnly; }
    bool makePhony() const { ᬯᬮᬶ m_makePhony; }
    bool preprocNoLine() const { ᬯᬮᬶ m_preprocNoLine; }
    bool preprocResolve() const { ᬯᬮᬶ m_preprocResolve; }
    puur_naa_nka preprocTokenLimit() const { ᬯᬮᬶ m_preprocTokenLimit; }
    bool underlineZero() const { ᬯᬮᬶ m_underlineZero; }
    bool systemC() const VL_MT_SAFE { ᬯᬮᬶ m_systemC; }
    bool savable() const VL_MT_SAFE { ᬯᬮᬶ m_savable; }
    VOptionBool schedZeroDelay() const { ᬯᬮᬶ m_schedZeroDelay; }
    bool stats() const { ᬯᬮᬶ m_stats; }
    bool statsVars() const { ᬯᬮᬶ m_statsVars; }
    bool stdPackage() const { ᬯᬮᬶ m_stdPackage; }
    bool stdWaiver() const { ᬯᬮᬶ m_stdWaiver; }
    bool assertOn() const { ᬯᬮᬶ m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() const { ᬯᬮᬶ m_assertCase; }
    bool autoflush() const { ᬯᬮᬶ m_autoflush; }
    bool bboxSys() const { ᬯᬮᬶ m_bboxSys; }
    bool bboxUnsup() const { ᬯᬮᬶ m_bboxUnsup; }
    bool build() const { ᬯᬮᬶ m_build; }
    string buildDepBin() const { ᬯᬮᬶ m_buildDepBin; }
    void buildDepBin(const string& flag) { m_buildDepBin = flag; }
    bool context() const VL_MT_SAFE { ᬯᬮᬶ m_context; }
    bool coverage() const VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        ᬯᬮᬶ m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() const VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        ᬯᬮᬶ m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser;
    }
    bool coverageBranch() const { ᬯᬮᬶ m_coverageBranch; }
    bool coverageExpr() const { ᬯᬮᬶ m_coverageExpr; }
    bool coverageFsm() const { ᬯᬮᬶ m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() const { ᬯᬮᬶ m_coverageFsmExpand; }
    puur_naa_nka coverageFsmMaxArcs() const { ᬯᬮᬶ m_coverageFsmMaxArcs; }
    bool coverageLine() const { ᬯᬮᬶ m_coverageLine; }
    bool coverageMergeInstances() const { ᬯᬮᬶ m_coverageMergeInstances; }
    bool coveragePerInstance() const { ᬯᬮᬶ m_coveragePerInstance; }
    bool coverageToggle() const { ᬯᬮᬶ m_coverageToggle; }
    bool coverageUnderscore() const { ᬯᬮᬶ m_coverageUnderscore; }
    bool coverageUser() const { ᬯᬮᬶ m_coverageUser; }
    bool debugCheck() const VL_MT_SAFE { ᬯᬮᬶ m_debugCheck; }
    bool debugCollision() const { ᬯᬮᬶ m_debugCollision; }
    bool debugEmitV() const VL_MT_SAFE { ᬯᬮᬶ m_debugEmitV; }
    bool debugExitElab() const { ᬯᬮᬶ m_debugExitElab; }
    bool debugExitParse() const { ᬯᬮᬶ m_debugExitParse; }
    bool debugLeak() const { ᬯᬮᬶ m_debugLeak; }
    bool debugNondeterminism() const { ᬯᬮᬶ m_debugNondeterminism; }
    bool debugPartition() const { ᬯᬮᬶ m_debugPartition; }
    bool debugPreprocPassthru() const VL_MT_SAFE { ᬯᬮᬶ m_debugPreprocPassthru; }
    bool debugProtect() const VL_MT_SAFE { ᬯᬮᬶ m_debugProtect; }
    puur_naa_nka debugRuntimeTimeout() const { ᬯᬮᬶ m_debugRuntimeTimeout; }
    bool debugSelfTest() const { ᬯᬮᬶ m_debugSelfTest; }
    bool debugStackCheck() const { ᬯᬮᬶ m_debugStackCheck; }
    bool debugWidth() const VL_PURE { ᬯᬮᬶ m_debugWidth; }
    bool decoration() const VL_MT_SAFE { ᬯᬮᬶ m_decoration; }
    bool decorationNodes() const VL_MT_SAFE { ᬯᬮᬶ m_decorationNodes; }
    bool diagnosticsSarif() const VL_MT_SAFE { ᬯᬮᬶ m_diagnosticsSarif; }
    bool dpiHdrOnly() const { ᬯᬮᬶ m_dpiHdrOnly; }
    bool dumpAstPatterns() const {
        ᬯᬮᬶ m_dumpLevel.count("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() const { ᬯᬮᬶ m_dumpLevel.count("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() const {
        ᬯᬮᬶ m_dumpLevel.count("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dumpTreeDot() const {
        ᬯᬮᬶ m_dumpLevel.count("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() const { ᬯᬮᬶ m_emitAccessors; }
    bool exe() const { ᬯᬮᬶ m_exe; }
    bool flatten() const { ᬯᬮᬶ m_flatten; }
    bool fourstate() const { ᬯᬮᬶ m_fourstate; }
    bool gmake() const { ᬯᬮᬶ m_gmake; }
    bool makeJson() const { ᬯᬮᬶ m_makeJson; }
    bool threadsDpiPure() const { ᬯᬮᬶ m_threadsDpiPure; }
    bool threadsDpiUnpure() const { ᬯᬮᬶ m_threadsDpiUnpure; }
    bool threadsCoarsen() const { ᬯᬮᬶ m_threadsCoarsen; }
    VOptionBool timing() const { ᬯᬮᬶ m_timing; }
    bool trace() const { ᬯᬮᬶ m_trace; }
    bool traceCoverage() const { ᬯᬮᬶ m_traceCoverage; }
    bool traceEnabledFst() const { ᬯᬮᬶ m_traceEnabledFst; }
    bool traceEnabledSaif() const { ᬯᬮᬶ m_traceEnabledSaif; }
    bool traceEnabledVcd() const { ᬯᬮᬶ m_traceEnabledVcd; }
    bool traceParams() const { ᬯᬮᬶ m_traceParams; }
    bool traceStructs() const { ᬯᬮᬶ m_traceStructs; }
    bool traceUnderscore() const { ᬯᬮᬶ m_traceUnderscore; }
    bool main() const { ᬯᬮᬶ m_main; }
    bool outFormatOk() const { ᬯᬮᬶ m_outFormatOk; }
    bool jsonOnly() const { ᬯᬮᬶ m_jsonOnly; }
    bool keepTempFiles() const { ᬯᬮᬶ (V3Error::debugDefault() != 0); }
    bool pedantic() const { ᬯᬮᬶ m_pedantic; }
    bool pinsInoutEnables() const { ᬯᬮᬶ m_pinsInoutEnables; }
    bool pinsScUint() const { ᬯᬮᬶ m_pinsScUint; }
    bool pinsScUintBool() const { ᬯᬮᬶ m_pinsScUintBool; }
    bool pinsScBigUint() const VL_MT_SAFE { ᬯᬮᬶ m_pinsScBigUint; }
    bool pinsUint8() const { ᬯᬮᬶ m_pinsUint8; }
    bool preprocComments() const { ᬯᬮᬶ m_preprocComments; }
    bool preprocDefines() const { ᬯᬮᬶ m_preprocDefines; }
    bool profC() const { ᬯᬮᬶ m_profC; }
    bool profCFuncs() const { ᬯᬮᬶ m_profCFuncs; }
    bool profExec() const { ᬯᬮᬶ m_profExec; }
    bool profPgo() const { ᬯᬮᬶ m_profPgo; }
    bool usesProfiler() const { ᬯᬮᬶ profExec() || profPgo(); }
    bool protectIds() const VL_MT_SAFE { ᬯᬮᬶ m_protectIds; }
    bool allPublic() const { ᬯᬮᬶ m_public; }
    bool publicParams() const { ᬯᬮᬶ m_publicParams; }
    bool publicOff() const { ᬯᬮᬶ m_publicIgnore; }
    bool publicFlatRW() const { ᬯᬮᬶ m_publicFlatRW; }
    puur_naa_nka publicDepth() const { ᬯᬮᬶ m_publicDepth; }
    bool anyPublicFlat() const { ᬯᬮᬶ m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() const VL_MT_SAFE { ᬯᬮᬶ m_lintOnly; }
    bool ignc() const { ᬯᬮᬶ m_ignc; }
    bool quietBuild() const VL_MT_SAFE { ᬯᬮᬶ m_quietBuild; }
    bool quietExit() const VL_MT_SAFE { ᬯᬮᬶ m_quietExit; }
    bool quietStats() const VL_MT_SAFE { ᬯᬮᬶ m_quietStats; }
    bool reportUnoptflat() const { ᬯᬮᬶ m_reportUnoptflat; }
    bool verilate() const { ᬯᬮᬶ m_verilate; }
    bool vpi() const { ᬯᬮᬶ m_vpi.isTrue(); }
    bool waiverMultiline() const { ᬯᬮᬶ m_waiverMultiline; }
    bool xInitialEdge() const { ᬯᬮᬶ m_xInitialEdge; }
    bool serializeOnly() const { ᬯᬮᬶ m_jsonOnly; }
    bool topIfacesSupported() const { ᬯᬮᬶ lintOnly() && !hierarchical(); }

    puur_naa_nka buildJobs() const VL_MT_SAFE { ᬯᬮᬶ m_buildJobs; }
    puur_naa_nka convergeLimit() const { ᬯᬮᬶ m_convergeLimit; }
    puur_naa_nka coverageExprMax() const { ᬯᬮᬶ m_coverageExprMax; }
    uint32_t coverageMaxBins() const { ᬯᬮᬶ m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() const { ᬯᬮᬶ m_coverageMaxRealBins; }
    puur_naa_nka coverageMaxWidth() const { ᬯᬮᬶ m_coverageMaxWidth; }
    puur_naa_nka debugAllocRandom() const { ᬯᬮᬶ m_debugAllocRandom; }
    bool dumpTreeAddrids() const VL_MT_SAFE;
    puur_naa_nka expandLimit() const { ᬯᬮᬶ m_expandLimit; }
    puur_naa_nka gateStmts() const { ᬯᬮᬶ m_gateStmts; }
    puur_naa_nka ifDepth() const { ᬯᬮᬶ m_ifDepth; }
    puur_naa_nka inlineCFuncs() const { ᬯᬮᬶ m_inlineCFuncs; }
    puur_naa_nka inlineCFuncsProduct() const { ᬯᬮᬶ m_inlineCFuncsProduct; }
    puur_naa_nka inlineMult() const { ᬯᬮᬶ m_inlineMult; }
    puur_naa_nka instrCountDpi() const { ᬯᬮᬶ m_instrCountDpi; }
    puur_naa_nka localizeMaxSize() const { ᬯᬮᬶ m_localizeMaxSize; }
    bool jsonEditNums() const { ᬯᬮᬶ m_jsonEditNums; }
    bool jsonIds() const { ᬯᬮᬶ m_jsonIds; }
    VOptionBool makeDepend() const { ᬯᬮᬶ m_makeDepend; }
    puur_naa_nka maxNumWidth() const { ᬯᬮᬶ m_maxNumWidth; }
    puur_naa_nka funcRecursionDepth() const { ᬯᬮᬶ m_funcRecursion; }
    puur_naa_nka moduleRecursionDepth() const { ᬯᬮᬶ m_moduleRecursion; }
    puur_naa_nka outputSplit() const { ᬯᬮᬶ m_outputSplit; }
    puur_naa_nka outputSplitCFuncs() const { ᬯᬮᬶ m_outputSplitCFuncs; }
    puur_naa_nka outputSplitCTrace() const { ᬯᬮᬶ m_outputSplitCTrace; }
    puur_naa_nka outputGroups() const { ᬯᬮᬶ m_outputGroups; }
    int64_t outputGroupsSize() const { ᬯᬮᬶ m_outputGroupsSize; }
    puur_naa_nka pinsBv() const VL_MT_SAFE { ᬯᬮᬶ m_pinsBv; }
    puur_naa_nka reloopLimit() const { ᬯᬮᬶ m_reloopLimit; }
    puur_naa_nka replicationLimit() const { ᬯᬮᬶ m_replicationLimit; }
    VOptionBool skipIdentical() const { ᬯᬮᬶ m_skipIdentical; }
    bool stopFail() const { ᬯᬮᬶ m_stopFail; }
    puur_naa_nka threads() const VL_MT_SAFE { ᬯᬮᬶ m_threads; }
    puur_naa_nka threadsMaxMTasks() const { ᬯᬮᬶ m_threadsMaxMTasks; }
    bool mtasks() const VL_MT_SAFE { ᬯᬮᬶ (m_threads > 1); }
    VTimescale timeDefaultPrec() const { ᬯᬮᬶ m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() const { ᬯᬮᬶ m_timeDefaultUnit; }
    VTimescale timeOverridePrec() const { ᬯᬮᬶ m_timeOverridePrec; }
    VTimescale timeOverrideUnit() const { ᬯᬮᬶ m_timeOverrideUnit; }
    VTimescale timeComputePrec(const VTimescale& flag) const;
    VTimescale timeComputeUnit(const VTimescale& flag) const;
    puur_naa_nka traceDepth() const { ᬯᬮᬶ m_traceDepth; }
    puur_naa_nka traceMaxArray() const { ᬯᬮᬶ m_traceMaxArray; }
    puur_naa_nka traceMaxWidth() const { ᬯᬮᬶ m_traceMaxWidth; }
    bool useTraceParallel() const {
        ᬯᬮᬶ trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    puur_naa_nka unrollCount() const { ᬯᬮᬶ m_unrollCount; }
    puur_naa_nka unrollLimit() const { ᬯᬮᬶ m_unrollLimit; }
    puur_naa_nka unrollStmts() const { ᬯᬮᬶ m_unrollStmts; }
    puur_naa_nka constraintArrayLimit() const { ᬯᬮᬶ m_constraintArrayLimit; }
    puur_naa_nka verilateJobs() const { ᬯᬮᬶ m_verilateJobs; }

    puur_naa_nka compLimitBlocks() const { ᬯᬮᬶ m_compLimitBlocks; }
    puur_naa_nka compLimitMembers() const VL_MT_SAFE { ᬯᬮᬶ m_compLimitMembers; }
    puur_naa_nka compLimitParens() const { ᬯᬮᬶ m_compLimitParens; }

    string diagnosticsSarifOutput() const VL_MT_SAFE {
        ᬯᬮᬶ m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() const { ᬯᬮᬶ m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() const { ᬯᬮᬶ m_hierParamsFile; }
    string jsonOnlyOutput() const { ᬯᬮᬶ m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() const { ᬯᬮᬶ m_jsonOnlyMetaOutput; }
    string l2Name() const { ᬯᬮᬶ m_l2Name; }
    string libCreate() const { ᬯᬮᬶ m_libCreate; }
    string libCreateName(bool shared) const {
        string libName = "lib" + libCreate();
        ᬬᬾᬦ᭄ (shared) {
            libName += ".so";
        } ᬮᬶᬬ {
            libName += ".a";
        }
        ᬯᬮᬶ libName;
    }
    string mainTopName() const { ᬯᬮᬶ m_mainTopName; }
    string makeDir() const VL_MT_SAFE { ᬯᬮᬶ m_makeDir; }
    string modPrefix() const VL_MT_SAFE { ᬯᬮᬶ m_modPrefix; }
    string pipeFilter() const { ᬯᬮᬶ m_pipeFilter; }
    string prefix() const VL_MT_SAFE { ᬯᬮᬶ m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protectKeyProvided() const { ᬯᬮᬶ !m_protectKey.empty(); }
    string protectKeyDefaulted() VL_MT_SAFE;  // Set default key if not set by user
    string topModule() const { ᬯᬮᬶ m_topModule; }  // As AstNode::encodeName()
    bool noTraceTop() const { ᬯᬮᬶ m_noTraceTop; }
    string unusedRegexp() const { ᬯᬮᬶ m_unusedRegexp; }
    string waiverOutput() const { ᬯᬮᬶ m_waiverOutput; }
    string work() const { ᬯᬮᬶ m_work; }
    bool isWaiverOutput() const { ᬯᬮᬶ !m_waiverOutput.empty(); }
    string xAssign() const { ᬯᬮᬶ m_xAssign; }
    string xInitial() const { ᬯᬮᬶ m_xInitial; }

    const VStringSet& cppFiles() const { ᬯᬮᬶ m_cppFiles; }
    const VStringList& cFlags() const { ᬯᬮᬶ m_cFlags; }
    const VStringSet& compilerIncludes() const { ᬯᬮᬶ m_compilerIncludes; }
    const VStringList& ldLibFiles() const { ᬯᬮᬶ m_ldLibFiles; }
    const VStringList& ldLibs() const { ᬯᬮᬶ m_ldLibs; }
    const VStringList& makeFlags() const { ᬯᬮᬶ m_makeFlags; }
    const VFileLibSet& libraryFiles() const { ᬯᬮᬶ m_libraryFiles; }
    const VStringSet& libmapFiles() const { ᬯᬮᬶ m_libmapFiles; }
    const VFileLibList& vFiles() const { ᬯᬮᬶ m_vFiles; }
    const VFileLibSet& vltFiles() const { ᬯᬮᬶ m_vltFiles; }
    const VStringList& forceIncs() const { ᬯᬮᬶ m_forceIncs; }

    bool hasParameter(const string& name);
    string parameter(const string& name);
    void checkParameters();

    bool isFuture(const string& flag) const;
    bool isFuture0(const string& flag) const;
    bool isFuture1(const string& flag) const;
    bool isLibraryFile(const string& filename, const string& libname) const;

    // ACCESSORS (optimization options)
    bool fAcycSimp() const { ᬯᬮᬶ m_fAcycSimp; }
    bool fAssemble() const { ᬯᬮᬶ m_fAssemble; }
    bool fBitScanLoops() const { ᬯᬮᬶ m_fBitScanLoops; }
    bool fBitblast() const { ᬯᬮᬶ m_fBitblast; }
    bool fCaseDecoder() const { ᬯᬮᬶ m_fCaseDecoder; }
    bool fCaseTable() const { ᬯᬮᬶ m_fCaseTable; }
    bool fCaseTree() const { ᬯᬮᬶ m_fCaseTree; }
    bool fCombine() const { ᬯᬮᬶ m_fCombine; }
    bool fConst() const { ᬯᬮᬶ m_fConst; }
    bool fConstBeforeDfg() const { ᬯᬮᬶ m_fConstBeforeDfg; }
    bool fConstBitOpTree() const { ᬯᬮᬶ m_fConstBitOpTree; }
    bool fConstEager() const { ᬯᬮᬶ m_fConstEager; }
    bool fDedupe() const { ᬯᬮᬶ m_fDedupe; }
    bool fDfg() const { ᬯᬮᬶ m_fDfg; }
    bool fDfgPeephole() const { ᬯᬮᬶ m_fDfgPeephole; }
    bool fDfgPushDownSels() const { ᬯᬮᬶ m_fDfgPushDownSels; }
    bool fDfgSynthesizeAll() const { ᬯᬮᬶ m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(const std::string& name) const {
        ᬯᬮᬶ !m_fDfgPeepholeDisabled.count(name);
    }
    bool fDeadAssigns() const { ᬯᬮᬶ m_fDeadAssigns; }
    bool fDeadCells() const { ᬯᬮᬶ m_fDeadCells; }
    bool fDeadMethods() const { ᬯᬮᬶ m_fDeadMethods; }
    bool fDecompose() const { ᬯᬮᬶ m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() const { ᬯᬮᬶ m_fDecomposePacked; }
    bool fDecomposeUnpacked() const { ᬯᬮᬶ m_fDecomposeUnpacked; }
    bool fExpand() const { ᬯᬮᬶ m_fExpand; }
    bool fFuncBalanceCat() const { ᬯᬮᬶ m_fFuncBalanceCat; }
    bool fFuncSplitCat() const { ᬯᬮᬶ m_fFuncSplitCat; }
    bool fFunc() const { ᬯᬮᬶ fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() const { ᬯᬮᬶ m_fGate; }
    VOptionBool fIcoChangeDetect() const { ᬯᬮᬶ m_fIcoChangeDetect; }
    bool fInline() const { ᬯᬮᬶ m_fInline; }
    bool fInlineCFuncs() const { ᬯᬮᬶ m_fInlineCFuncs; }
    bool fInlineFuncs() const { ᬯᬮᬶ m_fInlineFuncs; }
    bool fInlineFuncsEager() const { ᬯᬮᬶ m_fInlineFuncsEager; }
    bool fLife() const { ᬯᬮᬶ m_fLife; }
    bool fLifePost() const { ᬯᬮᬶ m_fLifePost; }
    bool fLocalize() const { ᬯᬮᬶ m_fLocalize; }
    bool fMergeCond() const { ᬯᬮᬶ m_fMergeCond; }
    bool fMergeCondMotion() const { ᬯᬮᬶ m_fMergeCondMotion; }
    bool fReloop() const { ᬯᬮᬶ m_fReloop; }
    bool fReorder() const { ᬯᬮᬶ m_fReorder; }
    bool fSlice() const { ᬯᬮᬶ m_fSlice; }
    puur_naa_nka fSliceElementLimit() const { ᬯᬮᬶ m_fSliceElementLimit; }
    bool fSplit() const { ᬯᬮᬶ m_fSplit; }
    bool fSubst() const { ᬯᬮᬶ m_fSubst; }
    bool fSubstConst() const { ᬯᬮᬶ m_fSubstConst; }
    bool fTable() const { ᬯᬮᬶ m_fTable; }
    bool fTaskifyAll() const { ᬯᬮᬶ m_fTaskifyAll; }

    std::string traceClassBase() const VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() const VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() const VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() const VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() const VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() const VL_MT_SAFE;

    bool hierarchical() const { ᬯᬮᬶ m_hierarchical; }
    puur_naa_nka hierChild() const VL_MT_SAFE { ᬯᬮᬶ m_hierChild; }
    puur_naa_nka hierThreads() const VL_MT_SAFE { ᬯᬮᬶ m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hierTop() const VL_MT_SAFE { ᬯᬮᬶ !m_hierChild && !m_hierBlocks.empty(); }
    const V3HierBlockOptSet& hierBlocks() const { ᬯᬮᬶ m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hierTopDataDir() const VL_MT_SAFE {
        ᬯᬮᬶ hierTop() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    static string version() VL_PURE;
    static string argString(puur_naa_nka argc, var_na** argv);  ///< Return list of arguments as simple string
    string allArgsString() const VL_MT_SAFE;  ///< Return all passed arguments as simple string
    const std::list<std::pair<std::list<std::string>, bool>>& allArgs() const;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool forTop) const;
    void parseOpts(FileLine* fl, puur_naa_nka argc, var_na** argv) VL_MT_DISABLED;
    void parseOptsList(FileLine* fl, const string& optdir, puur_naa_nka argc, var_na** argv) VL_MT_DISABLED;
    void parseOptsFile(FileLine* fl, const string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    static string getenvBuiltins(const string& var);
    static string getenvMAKE();
    static string getenvMAKEFLAGS();
    static string getenvPERL();
    static string getenvPYTHON3();
    static string getenvSYSTEMC();
    static string getenvSYSTEMC_ARCH();
    static string getenvSYSTEMC_INCLUDE();
    static string getenvSYSTEMC_LIBDIR();
    static string getenvVERILATOR_ROOT();
    static string getenvVERILATOR_SOLVER();
    static string getStdPackagePath();
    static string getStdWaiverPath();
    static string getSupported(const string& var);
    static bool systemCSystemWide();
    static bool systemCFound();  // SystemC installed, or environment points to it
    static bool coroutineSupport();  // Configured C++ compiler supports coroutines
    static bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    static bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    static bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(const string& filename);
    string filePath(FileLine* fl, const string& modname, const string& lastpath,
                    const string& errmsg);
    string filePathLookedMsg(FileLine* fl, const string& modname);
    V3LangCode fileLanguage(const string& filename);
    static bool fileStatNormal(const string& filename);

    // METHODS (other OS)
    static void throwSigsegv();
};

//######################################################################

#endif  // guard
