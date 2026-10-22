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
    explicit VOptionBool(จำนวนเต็ม _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { คืน m_e; }
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        คืน names[m_e];
    }
    bool isDefault() const { คืน m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() const { คืน m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() const { คืน m_e == OPT_TRUE; }
    bool isSetFalse() const { คืน m_e == OPT_FALSE; }
    void setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(const VOptionBool& lhs, const VOptionBool& rhs) {
    คืน lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VOptionBool& lhs, VOptionBool::en rhs) { คืน lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, const VOptionBool& rhs) { คืน lhs == rhs.m_e; }

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
    string filename() const { คืน m_filename; }
    string libname() const { คืน m_libname; }
    bool operator==(const VFileLibName& rhs) const {
        คืน m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(const VFileLibName& rhs) const {
        ถ้า (m_filename < rhs.m_filename) คืน true;
        ถ้า (m_filename > rhs.m_filename) คืน false;
        คืน m_libname < rhs.m_libname;
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
    explicit VTimescale(จำนวนเต็ม _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(const string& value, bool& badr);
    VTimescale(dviguu_na value, bool& badr) {
        badr = false;
        สำหรับ (จำนวนเต็ม i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            ถ้า (multiplier() == value) bha_nga;
        }
        ถ้า (multiplier() != value) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() const { คืน m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    static void parseSlashed(FileLine* fl, const var_na* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        คืน names[m_e];
    }
    จำนวนเต็ม powerOfTen() const { คืน 2 - static_cast<จำนวนเต็ม>(m_e); }
    dviguu_na multiplier() const {
        static const dviguu_na values[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        คืน values[m_e];
    }
};
constexpr bool operator==(const VTimescale& lhs, const VTimescale& rhs) {
    คืน lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTimescale& lhs, VTimescale::en rhs) { คืน lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, const VTimescale& rhs) { คืน lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(const VTimescale& lhs, const VTimescale& rhs) {
    คืน lhs.m_e > rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VTimescale& rhs) {
    คืน os << rhs.ascii();
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
    explicit VFsmExpandType(จำนวนเต็ม _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { คืน m_e; }
    constexpr const var_na* ascii() const {
        constexpr const var_na* const names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        คืน names[m_e];
    }
};
constexpr bool operator==(const VFsmExpandType& lhs, const VFsmExpandType& rhs) VL_MT_SAFE {
    คืน lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    คืน lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, const VFsmExpandType& rhs) VL_MT_SAFE {
    คืน lhs == rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VFsmExpandType& rhs) VL_MT_SAFE {
    คืน os << rhs.ascii();
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
    const string& origName() const { คืน m_origName; }
    const string& mangledName() const { คืน m_mangledName; }
    const ParamStrMap params() const { คืน m_parameters; }
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
    จำนวนเต็ม m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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

    จำนวนเต็ม         m_buildJobs = -1;    // main switch: --build-jobs, -j
    จำนวนเต็ม         m_coverageExprMax = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    จำนวนเต็ม         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    จำนวนเต็ม         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    จำนวนเต็ม         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    จำนวนเต็ม         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    จำนวนเต็ม         m_expandLimit = 256;  // main switch: --expand-limit
    จำนวนเต็ม         m_gateStmts = 100;    // main switch: --gate-stmts
    จำนวนเต็ม         m_hierChild = 0;      // main switch: --hierarchical-child
    จำนวนเต็ม         m_hierThreads = 0;      // main switch: --hierarchical-threads
    จำนวนเต็ม         m_ifDepth = 0;      // main switch: --if-depth
    จำนวนเต็ม         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    จำนวนเต็ม         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    จำนวนเต็ม         m_inlineMult = 2000;   // main switch: --inline-mult
    จำนวนเต็ม         m_instrCountDpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    จำนวนเต็ม         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    จำนวนเต็ม         m_maxNumWidth = 65536;  // main switch: --max-num-width
    จำนวนเต็ม         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    จำนวนเต็ม         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    จำนวนเต็ม         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    จำนวนเต็ม         m_outputSplit = 20000;  // main switch: --output-split
    จำนวนเต็ม         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    จำนวนเต็ม         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    จำนวนเต็ม         m_pinsBv = 65;       // main switch: --pins-bv
    จำนวนเต็ม         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    จำนวนเต็ม         m_publicDepth = 0;   // main switch: --public-depth
    จำนวนเต็ม         m_reloopLimit = 40; // main switch: --reloop-limit
    จำนวนเต็ม         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    จำนวนเต็ม         m_threads = 1;      // main switch: --threads
    จำนวนเต็ม         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    จำนวนเต็ม         m_traceDepth = 0;   // main switch: --trace-depth
    จำนวนเต็ม         m_traceMaxArray = 32;  // main switch: --trace-max-array
    จำนวนเต็ม         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    จำนวนเต็ม         m_unrollCount = 64;  // main switch: --unroll-count
    จำนวนเต็ม         m_unrollLimit = 16384;  // main switch: --unroll-limit
    จำนวนเต็ม         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    จำนวนเต็ม         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    จำนวนเต็ม         m_verilateJobs = -1;  // main switch: --verilate-jobs

    จำนวนเต็ม         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    จำนวนเต็ม         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    จำนวนเต็ม         m_compLimitParens = 240;  // compiler selection; number of nested parens

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
    จำนวนเต็ม  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
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
    void optimize(จำนวนเต็ม level);
    void showVersion(bool verbose);
    void coverage(bool flag) {
        m_coverageLine = m_coverageBranch = m_coverageToggle = m_coverageExpr = m_coverageFsm
            = m_coverageUser = flag;
    }
    static bool suffixed(const string& sw, const var_na* arg);
    static string parseFileArg(const string& optdir, const string& relfilename);
    string filePathCheckOneDir(const string& modname, const string& dirname);
    static จำนวนเต็ม stripOptionsForChildRun(const string& opt, bool forTop);
    void validateIdentifier(FileLine* fl, const string& arg, const string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

public:
    V3Options();
    ~V3Options();
    void setDebugMode(จำนวนเต็ม level);
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
    bool available() const VL_MT_SAFE { คืน m_available; }
    void ccSet();
    void decorations(FileLine* fl, const string& filename);
    void notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() const { คืน m_preprocOnly; }
    bool makePhony() const { คืน m_makePhony; }
    bool preprocNoLine() const { คืน m_preprocNoLine; }
    bool preprocResolve() const { คืน m_preprocResolve; }
    จำนวนเต็ม preprocTokenLimit() const { คืน m_preprocTokenLimit; }
    bool underlineZero() const { คืน m_underlineZero; }
    bool systemC() const VL_MT_SAFE { คืน m_systemC; }
    bool savable() const VL_MT_SAFE { คืน m_savable; }
    VOptionBool schedZeroDelay() const { คืน m_schedZeroDelay; }
    bool stats() const { คืน m_stats; }
    bool statsVars() const { คืน m_statsVars; }
    bool stdPackage() const { คืน m_stdPackage; }
    bool stdWaiver() const { คืน m_stdWaiver; }
    bool assertOn() const { คืน m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() const { คืน m_assertCase; }
    bool autoflush() const { คืน m_autoflush; }
    bool bboxSys() const { คืน m_bboxSys; }
    bool bboxUnsup() const { คืน m_bboxUnsup; }
    bool build() const { คืน m_build; }
    string buildDepBin() const { คืน m_buildDepBin; }
    void buildDepBin(const string& flag) { m_buildDepBin = flag; }
    bool context() const VL_MT_SAFE { คืน m_context; }
    bool coverage() const VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        คืน m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() const VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        คืน m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser;
    }
    bool coverageBranch() const { คืน m_coverageBranch; }
    bool coverageExpr() const { คืน m_coverageExpr; }
    bool coverageFsm() const { คืน m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() const { คืน m_coverageFsmExpand; }
    จำนวนเต็ม coverageFsmMaxArcs() const { คืน m_coverageFsmMaxArcs; }
    bool coverageLine() const { คืน m_coverageLine; }
    bool coverageMergeInstances() const { คืน m_coverageMergeInstances; }
    bool coveragePerInstance() const { คืน m_coveragePerInstance; }
    bool coverageToggle() const { คืน m_coverageToggle; }
    bool coverageUnderscore() const { คืน m_coverageUnderscore; }
    bool coverageUser() const { คืน m_coverageUser; }
    bool debugCheck() const VL_MT_SAFE { คืน m_debugCheck; }
    bool debugCollision() const { คืน m_debugCollision; }
    bool debugEmitV() const VL_MT_SAFE { คืน m_debugEmitV; }
    bool debugExitElab() const { คืน m_debugExitElab; }
    bool debugExitParse() const { คืน m_debugExitParse; }
    bool debugLeak() const { คืน m_debugLeak; }
    bool debugNondeterminism() const { คืน m_debugNondeterminism; }
    bool debugPartition() const { คืน m_debugPartition; }
    bool debugPreprocPassthru() const VL_MT_SAFE { คืน m_debugPreprocPassthru; }
    bool debugProtect() const VL_MT_SAFE { คืน m_debugProtect; }
    จำนวนเต็ม debugRuntimeTimeout() const { คืน m_debugRuntimeTimeout; }
    bool debugSelfTest() const { คืน m_debugSelfTest; }
    bool debugStackCheck() const { คืน m_debugStackCheck; }
    bool debugWidth() const VL_PURE { คืน m_debugWidth; }
    bool decoration() const VL_MT_SAFE { คืน m_decoration; }
    bool decorationNodes() const VL_MT_SAFE { คืน m_decorationNodes; }
    bool diagnosticsSarif() const VL_MT_SAFE { คืน m_diagnosticsSarif; }
    bool dpiHdrOnly() const { คืน m_dpiHdrOnly; }
    bool dumpAstPatterns() const {
        คืน m_dumpLevel.count("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() const { คืน m_dumpLevel.count("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() const {
        คืน m_dumpLevel.count("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dumpTreeDot() const {
        คืน m_dumpLevel.count("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() const { คืน m_emitAccessors; }
    bool exe() const { คืน m_exe; }
    bool flatten() const { คืน m_flatten; }
    bool fourstate() const { คืน m_fourstate; }
    bool gmake() const { คืน m_gmake; }
    bool makeJson() const { คืน m_makeJson; }
    bool threadsDpiPure() const { คืน m_threadsDpiPure; }
    bool threadsDpiUnpure() const { คืน m_threadsDpiUnpure; }
    bool threadsCoarsen() const { คืน m_threadsCoarsen; }
    VOptionBool timing() const { คืน m_timing; }
    bool trace() const { คืน m_trace; }
    bool traceCoverage() const { คืน m_traceCoverage; }
    bool traceEnabledFst() const { คืน m_traceEnabledFst; }
    bool traceEnabledSaif() const { คืน m_traceEnabledSaif; }
    bool traceEnabledVcd() const { คืน m_traceEnabledVcd; }
    bool traceParams() const { คืน m_traceParams; }
    bool traceStructs() const { คืน m_traceStructs; }
    bool traceUnderscore() const { คืน m_traceUnderscore; }
    bool หลัก() const { คืน m_main; }
    bool outFormatOk() const { คืน m_outFormatOk; }
    bool jsonOnly() const { คืน m_jsonOnly; }
    bool keepTempFiles() const { คืน (V3Error::debugDefault() != 0); }
    bool pedantic() const { คืน m_pedantic; }
    bool pinsInoutEnables() const { คืน m_pinsInoutEnables; }
    bool pinsScUint() const { คืน m_pinsScUint; }
    bool pinsScUintBool() const { คืน m_pinsScUintBool; }
    bool pinsScBigUint() const VL_MT_SAFE { คืน m_pinsScBigUint; }
    bool pinsUint8() const { คืน m_pinsUint8; }
    bool preprocComments() const { คืน m_preprocComments; }
    bool preprocDefines() const { คืน m_preprocDefines; }
    bool profC() const { คืน m_profC; }
    bool profCFuncs() const { คืน m_profCFuncs; }
    bool profExec() const { คืน m_profExec; }
    bool profPgo() const { คืน m_profPgo; }
    bool usesProfiler() const { คืน profExec() || profPgo(); }
    bool protectIds() const VL_MT_SAFE { คืน m_protectIds; }
    bool allPublic() const { คืน m_public; }
    bool publicParams() const { คืน m_publicParams; }
    bool publicOff() const { คืน m_publicIgnore; }
    bool publicFlatRW() const { คืน m_publicFlatRW; }
    จำนวนเต็ม publicDepth() const { คืน m_publicDepth; }
    bool anyPublicFlat() const { คืน m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() const VL_MT_SAFE { คืน m_lintOnly; }
    bool ignc() const { คืน m_ignc; }
    bool quietBuild() const VL_MT_SAFE { คืน m_quietBuild; }
    bool quietExit() const VL_MT_SAFE { คืน m_quietExit; }
    bool quietStats() const VL_MT_SAFE { คืน m_quietStats; }
    bool reportUnoptflat() const { คืน m_reportUnoptflat; }
    bool verilate() const { คืน m_verilate; }
    bool vpi() const { คืน m_vpi.isTrue(); }
    bool waiverMultiline() const { คืน m_waiverMultiline; }
    bool xInitialEdge() const { คืน m_xInitialEdge; }
    bool serializeOnly() const { คืน m_jsonOnly; }
    bool topIfacesSupported() const { คืน lintOnly() && !hierarchical(); }

    จำนวนเต็ม buildJobs() const VL_MT_SAFE { คืน m_buildJobs; }
    จำนวนเต็ม convergeLimit() const { คืน m_convergeLimit; }
    จำนวนเต็ม coverageExprMax() const { คืน m_coverageExprMax; }
    uint32_t coverageMaxBins() const { คืน m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() const { คืน m_coverageMaxRealBins; }
    จำนวนเต็ม coverageMaxWidth() const { คืน m_coverageMaxWidth; }
    จำนวนเต็ม debugAllocRandom() const { คืน m_debugAllocRandom; }
    bool dumpTreeAddrids() const VL_MT_SAFE;
    จำนวนเต็ม expandLimit() const { คืน m_expandLimit; }
    จำนวนเต็ม gateStmts() const { คืน m_gateStmts; }
    จำนวนเต็ม ifDepth() const { คืน m_ifDepth; }
    จำนวนเต็ม inlineCFuncs() const { คืน m_inlineCFuncs; }
    จำนวนเต็ม inlineCFuncsProduct() const { คืน m_inlineCFuncsProduct; }
    จำนวนเต็ม inlineMult() const { คืน m_inlineMult; }
    จำนวนเต็ม instrCountDpi() const { คืน m_instrCountDpi; }
    จำนวนเต็ม localizeMaxSize() const { คืน m_localizeMaxSize; }
    bool jsonEditNums() const { คืน m_jsonEditNums; }
    bool jsonIds() const { คืน m_jsonIds; }
    VOptionBool makeDepend() const { คืน m_makeDepend; }
    จำนวนเต็ม maxNumWidth() const { คืน m_maxNumWidth; }
    จำนวนเต็ม funcRecursionDepth() const { คืน m_funcRecursion; }
    จำนวนเต็ม moduleRecursionDepth() const { คืน m_moduleRecursion; }
    จำนวนเต็ม outputSplit() const { คืน m_outputSplit; }
    จำนวนเต็ม outputSplitCFuncs() const { คืน m_outputSplitCFuncs; }
    จำนวนเต็ม outputSplitCTrace() const { คืน m_outputSplitCTrace; }
    จำนวนเต็ม outputGroups() const { คืน m_outputGroups; }
    int64_t outputGroupsSize() const { คืน m_outputGroupsSize; }
    จำนวนเต็ม pinsBv() const VL_MT_SAFE { คืน m_pinsBv; }
    จำนวนเต็ม reloopLimit() const { คืน m_reloopLimit; }
    จำนวนเต็ม replicationLimit() const { คืน m_replicationLimit; }
    VOptionBool skipIdentical() const { คืน m_skipIdentical; }
    bool stopFail() const { คืน m_stopFail; }
    จำนวนเต็ม threads() const VL_MT_SAFE { คืน m_threads; }
    จำนวนเต็ม threadsMaxMTasks() const { คืน m_threadsMaxMTasks; }
    bool mtasks() const VL_MT_SAFE { คืน (m_threads > 1); }
    VTimescale timeDefaultPrec() const { คืน m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() const { คืน m_timeDefaultUnit; }
    VTimescale timeOverridePrec() const { คืน m_timeOverridePrec; }
    VTimescale timeOverrideUnit() const { คืน m_timeOverrideUnit; }
    VTimescale timeComputePrec(const VTimescale& flag) const;
    VTimescale timeComputeUnit(const VTimescale& flag) const;
    จำนวนเต็ม traceDepth() const { คืน m_traceDepth; }
    จำนวนเต็ม traceMaxArray() const { คืน m_traceMaxArray; }
    จำนวนเต็ม traceMaxWidth() const { คืน m_traceMaxWidth; }
    bool useTraceParallel() const {
        คืน trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    จำนวนเต็ม unrollCount() const { คืน m_unrollCount; }
    จำนวนเต็ม unrollLimit() const { คืน m_unrollLimit; }
    จำนวนเต็ม unrollStmts() const { คืน m_unrollStmts; }
    จำนวนเต็ม constraintArrayLimit() const { คืน m_constraintArrayLimit; }
    จำนวนเต็ม verilateJobs() const { คืน m_verilateJobs; }

    จำนวนเต็ม compLimitBlocks() const { คืน m_compLimitBlocks; }
    จำนวนเต็ม compLimitMembers() const VL_MT_SAFE { คืน m_compLimitMembers; }
    จำนวนเต็ม compLimitParens() const { คืน m_compLimitParens; }

    string diagnosticsSarifOutput() const VL_MT_SAFE {
        คืน m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() const { คืน m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() const { คืน m_hierParamsFile; }
    string jsonOnlyOutput() const { คืน m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() const { คืน m_jsonOnlyMetaOutput; }
    string l2Name() const { คืน m_l2Name; }
    string libCreate() const { คืน m_libCreate; }
    string libCreateName(bool shared) const {
        string libName = "lib" + libCreate();
        ถ้า (shared) {
            libName += ".so";
        } มิฉะนั้น {
            libName += ".a";
        }
        คืน libName;
    }
    string mainTopName() const { คืน m_mainTopName; }
    string makeDir() const VL_MT_SAFE { คืน m_makeDir; }
    string modPrefix() const VL_MT_SAFE { คืน m_modPrefix; }
    string pipeFilter() const { คืน m_pipeFilter; }
    string prefix() const VL_MT_SAFE { คืน m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protectKeyProvided() const { คืน !m_protectKey.empty(); }
    string protectKeyDefaulted() VL_MT_SAFE;  // Set default key if not set by user
    string topModule() const { คืน m_topModule; }  // As AstNode::encodeName()
    bool noTraceTop() const { คืน m_noTraceTop; }
    string unusedRegexp() const { คืน m_unusedRegexp; }
    string waiverOutput() const { คืน m_waiverOutput; }
    string work() const { คืน m_work; }
    bool isWaiverOutput() const { คืน !m_waiverOutput.empty(); }
    string xAssign() const { คืน m_xAssign; }
    string xInitial() const { คืน m_xInitial; }

    const VStringSet& cppFiles() const { คืน m_cppFiles; }
    const VStringList& cFlags() const { คืน m_cFlags; }
    const VStringSet& compilerIncludes() const { คืน m_compilerIncludes; }
    const VStringList& ldLibFiles() const { คืน m_ldLibFiles; }
    const VStringList& ldLibs() const { คืน m_ldLibs; }
    const VStringList& makeFlags() const { คืน m_makeFlags; }
    const VFileLibSet& libraryFiles() const { คืน m_libraryFiles; }
    const VStringSet& libmapFiles() const { คืน m_libmapFiles; }
    const VFileLibList& vFiles() const { คืน m_vFiles; }
    const VFileLibSet& vltFiles() const { คืน m_vltFiles; }
    const VStringList& forceIncs() const { คืน m_forceIncs; }

    bool hasParameter(const string& name);
    string parameter(const string& name);
    void checkParameters();

    bool isFuture(const string& flag) const;
    bool isFuture0(const string& flag) const;
    bool isFuture1(const string& flag) const;
    bool isLibraryFile(const string& filename, const string& libname) const;

    // ACCESSORS (optimization options)
    bool fAcycSimp() const { คืน m_fAcycSimp; }
    bool fAssemble() const { คืน m_fAssemble; }
    bool fBitScanLoops() const { คืน m_fBitScanLoops; }
    bool fBitblast() const { คืน m_fBitblast; }
    bool fCaseDecoder() const { คืน m_fCaseDecoder; }
    bool fCaseTable() const { คืน m_fCaseTable; }
    bool fCaseTree() const { คืน m_fCaseTree; }
    bool fCombine() const { คืน m_fCombine; }
    bool fConst() const { คืน m_fConst; }
    bool fConstBeforeDfg() const { คืน m_fConstBeforeDfg; }
    bool fConstBitOpTree() const { คืน m_fConstBitOpTree; }
    bool fConstEager() const { คืน m_fConstEager; }
    bool fDedupe() const { คืน m_fDedupe; }
    bool fDfg() const { คืน m_fDfg; }
    bool fDfgPeephole() const { คืน m_fDfgPeephole; }
    bool fDfgPushDownSels() const { คืน m_fDfgPushDownSels; }
    bool fDfgSynthesizeAll() const { คืน m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(const std::string& name) const {
        คืน !m_fDfgPeepholeDisabled.count(name);
    }
    bool fDeadAssigns() const { คืน m_fDeadAssigns; }
    bool fDeadCells() const { คืน m_fDeadCells; }
    bool fDeadMethods() const { คืน m_fDeadMethods; }
    bool fDecompose() const { คืน m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() const { คืน m_fDecomposePacked; }
    bool fDecomposeUnpacked() const { คืน m_fDecomposeUnpacked; }
    bool fExpand() const { คืน m_fExpand; }
    bool fFuncBalanceCat() const { คืน m_fFuncBalanceCat; }
    bool fFuncSplitCat() const { คืน m_fFuncSplitCat; }
    bool fFunc() const { คืน fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() const { คืน m_fGate; }
    VOptionBool fIcoChangeDetect() const { คืน m_fIcoChangeDetect; }
    bool fInline() const { คืน m_fInline; }
    bool fInlineCFuncs() const { คืน m_fInlineCFuncs; }
    bool fInlineFuncs() const { คืน m_fInlineFuncs; }
    bool fInlineFuncsEager() const { คืน m_fInlineFuncsEager; }
    bool fLife() const { คืน m_fLife; }
    bool fLifePost() const { คืน m_fLifePost; }
    bool fLocalize() const { คืน m_fLocalize; }
    bool fMergeCond() const { คืน m_fMergeCond; }
    bool fMergeCondMotion() const { คืน m_fMergeCondMotion; }
    bool fReloop() const { คืน m_fReloop; }
    bool fReorder() const { คืน m_fReorder; }
    bool fSlice() const { คืน m_fSlice; }
    จำนวนเต็ม fSliceElementLimit() const { คืน m_fSliceElementLimit; }
    bool fSplit() const { คืน m_fSplit; }
    bool fSubst() const { คืน m_fSubst; }
    bool fSubstConst() const { คืน m_fSubstConst; }
    bool fTable() const { คืน m_fTable; }
    bool fTaskifyAll() const { คืน m_fTaskifyAll; }

    std::string traceClassBase() const VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() const VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() const VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() const VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() const VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() const VL_MT_SAFE;

    bool hierarchical() const { คืน m_hierarchical; }
    จำนวนเต็ม hierChild() const VL_MT_SAFE { คืน m_hierChild; }
    จำนวนเต็ม hierThreads() const VL_MT_SAFE { คืน m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hierTop() const VL_MT_SAFE { คืน !m_hierChild && !m_hierBlocks.empty(); }
    const V3HierBlockOptSet& hierBlocks() const { คืน m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hierTopDataDir() const VL_MT_SAFE {
        คืน hierTop() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    static string version() VL_PURE;
    static string argString(จำนวนเต็ม argc, var_na** argv);  ///< Return list of arguments as simple string
    string allArgsString() const VL_MT_SAFE;  ///< Return all passed arguments as simple string
    const std::list<std::pair<std::list<std::string>, bool>>& allArgs() const;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool forTop) const;
    void parseOpts(FileLine* fl, จำนวนเต็ม argc, var_na** argv) VL_MT_DISABLED;
    void parseOptsList(FileLine* fl, const string& optdir, จำนวนเต็ม argc, var_na** argv) VL_MT_DISABLED;
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
