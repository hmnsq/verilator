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
    explicit VOptionBool(ཧྲིལ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ལོག m_e; }
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        ལོག names[m_e];
    }
    bool isDefault() const { ལོག m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() const { ལོག m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() const { ལོག m_e == OPT_TRUE; }
    bool isSetFalse() const { ལོག m_e == OPT_FALSE; }
    void setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(const VOptionBool& lhs, const VOptionBool& rhs) {
    ལོག lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VOptionBool& lhs, VOptionBool::en rhs) { ལོག lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, const VOptionBool& rhs) { ལོག lhs == rhs.m_e; }

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
    string filename() const { ལོག m_filename; }
    string libname() const { ལོག m_libname; }
    bool operator==(const VFileLibName& rhs) const {
        ལོག m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(const VFileLibName& rhs) const {
        གལ་སྲིད (m_filename < rhs.m_filename) ལོག true;
        གལ་སྲིད (m_filename > rhs.m_filename) ལོག false;
        ལོག m_libname < rhs.m_libname;
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
    explicit VTimescale(ཧྲིལ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(const string& value, bool& badr);
    VTimescale(dviguu_na value, bool& badr) {
        badr = false;
        དོན་ལུ (ཧྲིལ i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            གལ་སྲིད (multiplier() == value) bha_nga;
        }
        གལ་སྲིད (multiplier() != value) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() const { ལོག m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    static void parseSlashed(FileLine* fl, const var_na* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        ལོག names[m_e];
    }
    ཧྲིལ powerOfTen() const { ལོག 2 - static_cast<ཧྲིལ>(m_e); }
    dviguu_na multiplier() const {
        static const dviguu_na values[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        ལོག values[m_e];
    }
};
constexpr bool operator==(const VTimescale& lhs, const VTimescale& rhs) {
    ལོག lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTimescale& lhs, VTimescale::en rhs) { ལོག lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, const VTimescale& rhs) { ལོག lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(const VTimescale& lhs, const VTimescale& rhs) {
    ལོག lhs.m_e > rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VTimescale& rhs) {
    ལོག os << rhs.ascii();
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
    explicit VFsmExpandType(ཧྲིལ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ལོག m_e; }
    constexpr const var_na* ascii() const {
        constexpr const var_na* const names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        ལོག names[m_e];
    }
};
constexpr bool operator==(const VFsmExpandType& lhs, const VFsmExpandType& rhs) VL_MT_SAFE {
    ལོག lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    ལོག lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, const VFsmExpandType& rhs) VL_MT_SAFE {
    ལོག lhs == rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VFsmExpandType& rhs) VL_MT_SAFE {
    ལོག os << rhs.ascii();
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
    const string& origName() const { ལོག m_origName; }
    const string& mangledName() const { ལོག m_mangledName; }
    const ParamStrMap params() const { ལོག m_parameters; }
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
    ཧྲིལ m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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

    ཧྲིལ         m_buildJobs = -1;    // main switch: --build-jobs, -j
    ཧྲིལ         m_coverageExprMax = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    ཧྲིལ         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    ཧྲིལ         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    ཧྲིལ         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    ཧྲིལ         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    ཧྲིལ         m_expandLimit = 256;  // main switch: --expand-limit
    ཧྲིལ         m_gateStmts = 100;    // main switch: --gate-stmts
    ཧྲིལ         m_hierChild = 0;      // main switch: --hierarchical-child
    ཧྲིལ         m_hierThreads = 0;      // main switch: --hierarchical-threads
    ཧྲིལ         m_ifDepth = 0;      // main switch: --if-depth
    ཧྲིལ         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    ཧྲིལ         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    ཧྲིལ         m_inlineMult = 2000;   // main switch: --inline-mult
    ཧྲིལ         m_instrCountDpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    ཧྲིལ         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    ཧྲིལ         m_maxNumWidth = 65536;  // main switch: --max-num-width
    ཧྲིལ         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    ཧྲིལ         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    ཧྲིལ         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    ཧྲིལ         m_outputSplit = 20000;  // main switch: --output-split
    ཧྲིལ         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    ཧྲིལ         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    ཧྲིལ         m_pinsBv = 65;       // main switch: --pins-bv
    ཧྲིལ         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    ཧྲིལ         m_publicDepth = 0;   // main switch: --public-depth
    ཧྲིལ         m_reloopLimit = 40; // main switch: --reloop-limit
    ཧྲིལ         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    ཧྲིལ         m_threads = 1;      // main switch: --threads
    ཧྲིལ         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    ཧྲིལ         m_traceDepth = 0;   // main switch: --trace-depth
    ཧྲིལ         m_traceMaxArray = 32;  // main switch: --trace-max-array
    ཧྲིལ         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    ཧྲིལ         m_unrollCount = 64;  // main switch: --unroll-count
    ཧྲིལ         m_unrollLimit = 16384;  // main switch: --unroll-limit
    ཧྲིལ         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    ཧྲིལ         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    ཧྲིལ         m_verilateJobs = -1;  // main switch: --verilate-jobs

    ཧྲིལ         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    ཧྲིལ         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    ཧྲིལ         m_compLimitParens = 240;  // compiler selection; number of nested parens

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
    ཧྲིལ  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
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
    void optimize(ཧྲིལ level);
    void showVersion(bool verbose);
    void coverage(bool flag) {
        m_coverageLine = m_coverageBranch = m_coverageToggle = m_coverageExpr = m_coverageFsm
            = m_coverageUser = flag;
    }
    static bool suffixed(const string& sw, const var_na* arg);
    static string parseFileArg(const string& optdir, const string& relfilename);
    string filePathCheckOneDir(const string& modname, const string& dirname);
    static ཧྲིལ stripOptionsForChildRun(const string& opt, bool forTop);
    void validateIdentifier(FileLine* fl, const string& arg, const string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

public:
    V3Options();
    ~V3Options();
    void setDebugMode(ཧྲིལ level);
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
    bool available() const VL_MT_SAFE { ལོག m_available; }
    void ccSet();
    void decorations(FileLine* fl, const string& filename);
    void notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() const { ལོག m_preprocOnly; }
    bool makePhony() const { ལོག m_makePhony; }
    bool preprocNoLine() const { ལོག m_preprocNoLine; }
    bool preprocResolve() const { ལོག m_preprocResolve; }
    ཧྲིལ preprocTokenLimit() const { ལོག m_preprocTokenLimit; }
    bool underlineZero() const { ལོག m_underlineZero; }
    bool systemC() const VL_MT_SAFE { ལོག m_systemC; }
    bool savable() const VL_MT_SAFE { ལོག m_savable; }
    VOptionBool schedZeroDelay() const { ལོག m_schedZeroDelay; }
    bool stats() const { ལོག m_stats; }
    bool statsVars() const { ལོག m_statsVars; }
    bool stdPackage() const { ལོག m_stdPackage; }
    bool stdWaiver() const { ལོག m_stdWaiver; }
    bool assertOn() const { ལོག m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() const { ལོག m_assertCase; }
    bool autoflush() const { ལོག m_autoflush; }
    bool bboxSys() const { ལོག m_bboxSys; }
    bool bboxUnsup() const { ལོག m_bboxUnsup; }
    bool build() const { ལོག m_build; }
    string buildDepBin() const { ལོག m_buildDepBin; }
    void buildDepBin(const string& flag) { m_buildDepBin = flag; }
    bool context() const VL_MT_SAFE { ལོག m_context; }
    bool coverage() const VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        ལོག m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() const VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        ལོག m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser;
    }
    bool coverageBranch() const { ལོག m_coverageBranch; }
    bool coverageExpr() const { ལོག m_coverageExpr; }
    bool coverageFsm() const { ལོག m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() const { ལོག m_coverageFsmExpand; }
    ཧྲིལ coverageFsmMaxArcs() const { ལོག m_coverageFsmMaxArcs; }
    bool coverageLine() const { ལོག m_coverageLine; }
    bool coverageMergeInstances() const { ལོག m_coverageMergeInstances; }
    bool coveragePerInstance() const { ལོག m_coveragePerInstance; }
    bool coverageToggle() const { ལོག m_coverageToggle; }
    bool coverageUnderscore() const { ལོག m_coverageUnderscore; }
    bool coverageUser() const { ལོག m_coverageUser; }
    bool debugCheck() const VL_MT_SAFE { ལོག m_debugCheck; }
    bool debugCollision() const { ལོག m_debugCollision; }
    bool debugEmitV() const VL_MT_SAFE { ལོག m_debugEmitV; }
    bool debugExitElab() const { ལོག m_debugExitElab; }
    bool debugExitParse() const { ལོག m_debugExitParse; }
    bool debugLeak() const { ལོག m_debugLeak; }
    bool debugNondeterminism() const { ལོག m_debugNondeterminism; }
    bool debugPartition() const { ལོག m_debugPartition; }
    bool debugPreprocPassthru() const VL_MT_SAFE { ལོག m_debugPreprocPassthru; }
    bool debugProtect() const VL_MT_SAFE { ལོག m_debugProtect; }
    ཧྲིལ debugRuntimeTimeout() const { ལོག m_debugRuntimeTimeout; }
    bool debugSelfTest() const { ལོག m_debugSelfTest; }
    bool debugStackCheck() const { ལོག m_debugStackCheck; }
    bool debugWidth() const VL_PURE { ལོག m_debugWidth; }
    bool decoration() const VL_MT_SAFE { ལོག m_decoration; }
    bool decorationNodes() const VL_MT_SAFE { ལོག m_decorationNodes; }
    bool diagnosticsSarif() const VL_MT_SAFE { ལོག m_diagnosticsSarif; }
    bool dpiHdrOnly() const { ལོག m_dpiHdrOnly; }
    bool dumpAstPatterns() const {
        ལོག m_dumpLevel.count("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() const { ལོག m_dumpLevel.count("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() const {
        ལོག m_dumpLevel.count("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dumpTreeDot() const {
        ལོག m_dumpLevel.count("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() const { ལོག m_emitAccessors; }
    bool exe() const { ལོག m_exe; }
    bool flatten() const { ལོག m_flatten; }
    bool fourstate() const { ལོག m_fourstate; }
    bool gmake() const { ལོག m_gmake; }
    bool makeJson() const { ལོག m_makeJson; }
    bool threadsDpiPure() const { ལོག m_threadsDpiPure; }
    bool threadsDpiUnpure() const { ལོག m_threadsDpiUnpure; }
    bool threadsCoarsen() const { ལོག m_threadsCoarsen; }
    VOptionBool timing() const { ལོག m_timing; }
    bool trace() const { ལོག m_trace; }
    bool traceCoverage() const { ལོག m_traceCoverage; }
    bool traceEnabledFst() const { ལོག m_traceEnabledFst; }
    bool traceEnabledSaif() const { ལོག m_traceEnabledSaif; }
    bool traceEnabledVcd() const { ལོག m_traceEnabledVcd; }
    bool traceParams() const { ལོག m_traceParams; }
    bool traceStructs() const { ལོག m_traceStructs; }
    bool traceUnderscore() const { ལོག m_traceUnderscore; }
    bool main() const { ལོག m_main; }
    bool outFormatOk() const { ལོག m_outFormatOk; }
    bool jsonOnly() const { ལོག m_jsonOnly; }
    bool keepTempFiles() const { ལོག (V3Error::debugDefault() != 0); }
    bool pedantic() const { ལོག m_pedantic; }
    bool pinsInoutEnables() const { ལོག m_pinsInoutEnables; }
    bool pinsScUint() const { ལོག m_pinsScUint; }
    bool pinsScUintBool() const { ལོག m_pinsScUintBool; }
    bool pinsScBigUint() const VL_MT_SAFE { ལོག m_pinsScBigUint; }
    bool pinsUint8() const { ལོག m_pinsUint8; }
    bool preprocComments() const { ལོག m_preprocComments; }
    bool preprocDefines() const { ལོག m_preprocDefines; }
    bool profC() const { ལོག m_profC; }
    bool profCFuncs() const { ལོག m_profCFuncs; }
    bool profExec() const { ལོག m_profExec; }
    bool profPgo() const { ལོག m_profPgo; }
    bool usesProfiler() const { ལོག profExec() || profPgo(); }
    bool protectIds() const VL_MT_SAFE { ལོག m_protectIds; }
    bool allPublic() const { ལོག m_public; }
    bool publicParams() const { ལོག m_publicParams; }
    bool publicOff() const { ལོག m_publicIgnore; }
    bool publicFlatRW() const { ལོག m_publicFlatRW; }
    ཧྲིལ publicDepth() const { ལོག m_publicDepth; }
    bool anyPublicFlat() const { ལོག m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() const VL_MT_SAFE { ལོག m_lintOnly; }
    bool ignc() const { ལོག m_ignc; }
    bool quietBuild() const VL_MT_SAFE { ལོག m_quietBuild; }
    bool quietExit() const VL_MT_SAFE { ལོག m_quietExit; }
    bool quietStats() const VL_MT_SAFE { ལོག m_quietStats; }
    bool reportUnoptflat() const { ལོག m_reportUnoptflat; }
    bool verilate() const { ལོག m_verilate; }
    bool vpi() const { ལོག m_vpi.isTrue(); }
    bool waiverMultiline() const { ལོག m_waiverMultiline; }
    bool xInitialEdge() const { ལོག m_xInitialEdge; }
    bool serializeOnly() const { ལོག m_jsonOnly; }
    bool topIfacesSupported() const { ལོག lintOnly() && !hierarchical(); }

    ཧྲིལ buildJobs() const VL_MT_SAFE { ལོག m_buildJobs; }
    ཧྲིལ convergeLimit() const { ལོག m_convergeLimit; }
    ཧྲིལ coverageExprMax() const { ལོག m_coverageExprMax; }
    uint32_t coverageMaxBins() const { ལོག m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() const { ལོག m_coverageMaxRealBins; }
    ཧྲིལ coverageMaxWidth() const { ལོག m_coverageMaxWidth; }
    ཧྲིལ debugAllocRandom() const { ལོག m_debugAllocRandom; }
    bool dumpTreeAddrids() const VL_MT_SAFE;
    ཧྲིལ expandLimit() const { ལོག m_expandLimit; }
    ཧྲིལ gateStmts() const { ལོག m_gateStmts; }
    ཧྲིལ ifDepth() const { ལོག m_ifDepth; }
    ཧྲིལ inlineCFuncs() const { ལོག m_inlineCFuncs; }
    ཧྲིལ inlineCFuncsProduct() const { ལོག m_inlineCFuncsProduct; }
    ཧྲིལ inlineMult() const { ལོག m_inlineMult; }
    ཧྲིལ instrCountDpi() const { ལོག m_instrCountDpi; }
    ཧྲིལ localizeMaxSize() const { ལོག m_localizeMaxSize; }
    bool jsonEditNums() const { ལོག m_jsonEditNums; }
    bool jsonIds() const { ལོག m_jsonIds; }
    VOptionBool makeDepend() const { ལོག m_makeDepend; }
    ཧྲིལ maxNumWidth() const { ལོག m_maxNumWidth; }
    ཧྲིལ funcRecursionDepth() const { ལོག m_funcRecursion; }
    ཧྲིལ moduleRecursionDepth() const { ལོག m_moduleRecursion; }
    ཧྲིལ outputSplit() const { ལོག m_outputSplit; }
    ཧྲིལ outputSplitCFuncs() const { ལོག m_outputSplitCFuncs; }
    ཧྲིལ outputSplitCTrace() const { ལོག m_outputSplitCTrace; }
    ཧྲིལ outputGroups() const { ལོག m_outputGroups; }
    int64_t outputGroupsSize() const { ལོག m_outputGroupsSize; }
    ཧྲིལ pinsBv() const VL_MT_SAFE { ལོག m_pinsBv; }
    ཧྲིལ reloopLimit() const { ལོག m_reloopLimit; }
    ཧྲིལ replicationLimit() const { ལོག m_replicationLimit; }
    VOptionBool skipIdentical() const { ལོག m_skipIdentical; }
    bool stopFail() const { ལོག m_stopFail; }
    ཧྲིལ threads() const VL_MT_SAFE { ལོག m_threads; }
    ཧྲིལ threadsMaxMTasks() const { ལོག m_threadsMaxMTasks; }
    bool mtasks() const VL_MT_SAFE { ལོག (m_threads > 1); }
    VTimescale timeDefaultPrec() const { ལོག m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() const { ལོག m_timeDefaultUnit; }
    VTimescale timeOverridePrec() const { ལོག m_timeOverridePrec; }
    VTimescale timeOverrideUnit() const { ལོག m_timeOverrideUnit; }
    VTimescale timeComputePrec(const VTimescale& flag) const;
    VTimescale timeComputeUnit(const VTimescale& flag) const;
    ཧྲིལ traceDepth() const { ལོག m_traceDepth; }
    ཧྲིལ traceMaxArray() const { ལོག m_traceMaxArray; }
    ཧྲིལ traceMaxWidth() const { ལོག m_traceMaxWidth; }
    bool useTraceParallel() const {
        ལོག trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    ཧྲིལ unrollCount() const { ལོག m_unrollCount; }
    ཧྲིལ unrollLimit() const { ལོག m_unrollLimit; }
    ཧྲིལ unrollStmts() const { ལོག m_unrollStmts; }
    ཧྲིལ constraintArrayLimit() const { ལོག m_constraintArrayLimit; }
    ཧྲིལ verilateJobs() const { ལོག m_verilateJobs; }

    ཧྲིལ compLimitBlocks() const { ལོག m_compLimitBlocks; }
    ཧྲིལ compLimitMembers() const VL_MT_SAFE { ལོག m_compLimitMembers; }
    ཧྲིལ compLimitParens() const { ལོག m_compLimitParens; }

    string diagnosticsSarifOutput() const VL_MT_SAFE {
        ལོག m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() const { ལོག m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() const { ལོག m_hierParamsFile; }
    string jsonOnlyOutput() const { ལོག m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() const { ལོག m_jsonOnlyMetaOutput; }
    string l2Name() const { ལོག m_l2Name; }
    string libCreate() const { ལོག m_libCreate; }
    string libCreateName(bool shared) const {
        string libName = "lib" + libCreate();
        གལ་སྲིད (shared) {
            libName += ".so";
        } དེ་མིན {
            libName += ".a";
        }
        ལོག libName;
    }
    string mainTopName() const { ལོག m_mainTopName; }
    string makeDir() const VL_MT_SAFE { ལོག m_makeDir; }
    string modPrefix() const VL_MT_SAFE { ལོག m_modPrefix; }
    string pipeFilter() const { ལོག m_pipeFilter; }
    string prefix() const VL_MT_SAFE { ལོག m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protectKeyProvided() const { ལོག !m_protectKey.empty(); }
    string protectKeyDefaulted() VL_MT_SAFE;  // Set default key if not set by user
    string topModule() const { ལོག m_topModule; }  // As AstNode::encodeName()
    bool noTraceTop() const { ལོག m_noTraceTop; }
    string unusedRegexp() const { ལོག m_unusedRegexp; }
    string waiverOutput() const { ལོག m_waiverOutput; }
    string work() const { ལོག m_work; }
    bool isWaiverOutput() const { ལོག !m_waiverOutput.empty(); }
    string xAssign() const { ལོག m_xAssign; }
    string xInitial() const { ལོག m_xInitial; }

    const VStringSet& cppFiles() const { ལོག m_cppFiles; }
    const VStringList& cFlags() const { ལོག m_cFlags; }
    const VStringSet& compilerIncludes() const { ལོག m_compilerIncludes; }
    const VStringList& ldLibFiles() const { ལོག m_ldLibFiles; }
    const VStringList& ldLibs() const { ལོག m_ldLibs; }
    const VStringList& makeFlags() const { ལོག m_makeFlags; }
    const VFileLibSet& libraryFiles() const { ལོག m_libraryFiles; }
    const VStringSet& libmapFiles() const { ལོག m_libmapFiles; }
    const VFileLibList& vFiles() const { ལོག m_vFiles; }
    const VFileLibSet& vltFiles() const { ལོག m_vltFiles; }
    const VStringList& forceIncs() const { ལོག m_forceIncs; }

    bool hasParameter(const string& name);
    string parameter(const string& name);
    void checkParameters();

    bool isFuture(const string& flag) const;
    bool isFuture0(const string& flag) const;
    bool isFuture1(const string& flag) const;
    bool isLibraryFile(const string& filename, const string& libname) const;

    // ACCESSORS (optimization options)
    bool fAcycSimp() const { ལོག m_fAcycSimp; }
    bool fAssemble() const { ལོག m_fAssemble; }
    bool fBitScanLoops() const { ལོག m_fBitScanLoops; }
    bool fBitblast() const { ལོག m_fBitblast; }
    bool fCaseDecoder() const { ལོག m_fCaseDecoder; }
    bool fCaseTable() const { ལོག m_fCaseTable; }
    bool fCaseTree() const { ལོག m_fCaseTree; }
    bool fCombine() const { ལོག m_fCombine; }
    bool fConst() const { ལོག m_fConst; }
    bool fConstBeforeDfg() const { ལོག m_fConstBeforeDfg; }
    bool fConstBitOpTree() const { ལོག m_fConstBitOpTree; }
    bool fConstEager() const { ལོག m_fConstEager; }
    bool fDedupe() const { ལོག m_fDedupe; }
    bool fDfg() const { ལོག m_fDfg; }
    bool fDfgPeephole() const { ལོག m_fDfgPeephole; }
    bool fDfgPushDownSels() const { ལོག m_fDfgPushDownSels; }
    bool fDfgSynthesizeAll() const { ལོག m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(const std::string& name) const {
        ལོག !m_fDfgPeepholeDisabled.count(name);
    }
    bool fDeadAssigns() const { ལོག m_fDeadAssigns; }
    bool fDeadCells() const { ལོག m_fDeadCells; }
    bool fDeadMethods() const { ལོག m_fDeadMethods; }
    bool fDecompose() const { ལོག m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() const { ལོག m_fDecomposePacked; }
    bool fDecomposeUnpacked() const { ལོག m_fDecomposeUnpacked; }
    bool fExpand() const { ལོག m_fExpand; }
    bool fFuncBalanceCat() const { ལོག m_fFuncBalanceCat; }
    bool fFuncSplitCat() const { ལོག m_fFuncSplitCat; }
    bool fFunc() const { ལོག fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() const { ལོག m_fGate; }
    VOptionBool fIcoChangeDetect() const { ལོག m_fIcoChangeDetect; }
    bool fInline() const { ལོག m_fInline; }
    bool fInlineCFuncs() const { ལོག m_fInlineCFuncs; }
    bool fInlineFuncs() const { ལོག m_fInlineFuncs; }
    bool fInlineFuncsEager() const { ལོག m_fInlineFuncsEager; }
    bool fLife() const { ལོག m_fLife; }
    bool fLifePost() const { ལོག m_fLifePost; }
    bool fLocalize() const { ལོག m_fLocalize; }
    bool fMergeCond() const { ལོག m_fMergeCond; }
    bool fMergeCondMotion() const { ལོག m_fMergeCondMotion; }
    bool fReloop() const { ལོག m_fReloop; }
    bool fReorder() const { ལོག m_fReorder; }
    bool fSlice() const { ལོག m_fSlice; }
    ཧྲིལ fSliceElementLimit() const { ལོག m_fSliceElementLimit; }
    bool fSplit() const { ལོག m_fSplit; }
    bool fSubst() const { ལོག m_fSubst; }
    bool fSubstConst() const { ལོག m_fSubstConst; }
    bool fTable() const { ལོག m_fTable; }
    bool fTaskifyAll() const { ལོག m_fTaskifyAll; }

    std::string traceClassBase() const VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() const VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() const VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() const VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() const VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() const VL_MT_SAFE;

    bool hierarchical() const { ལོག m_hierarchical; }
    ཧྲིལ hierChild() const VL_MT_SAFE { ལོག m_hierChild; }
    ཧྲིལ hierThreads() const VL_MT_SAFE { ལོག m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hierTop() const VL_MT_SAFE { ལོག !m_hierChild && !m_hierBlocks.empty(); }
    const V3HierBlockOptSet& hierBlocks() const { ལོག m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hierTopDataDir() const VL_MT_SAFE {
        ལོག hierTop() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    static string version() VL_PURE;
    static string argString(ཧྲིལ argc, var_na** argv);  ///< Return list of arguments as simple string
    string allArgsString() const VL_MT_SAFE;  ///< Return all passed arguments as simple string
    const std::list<std::pair<std::list<std::string>, bool>>& allArgs() const;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool forTop) const;
    void parseOpts(FileLine* fl, ཧྲིལ argc, var_na** argv) VL_MT_DISABLED;
    void parseOptsList(FileLine* fl, const string& optdir, ཧྲིལ argc, var_na** argv) VL_MT_DISABLED;
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
