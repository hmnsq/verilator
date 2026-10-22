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
    explicit VOptionBool(පූර්ණ_සංඛ්‍යා _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ආපසු m_e; }
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        ආපසු names[m_e];
    }
    bool isDefault() const { ආපසු m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() const { ආපසු m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() const { ආපසු m_e == OPT_TRUE; }
    bool isSetFalse() const { ආපසු m_e == OPT_FALSE; }
    void setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(const VOptionBool& lhs, const VOptionBool& rhs) {
    ආපසු lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VOptionBool& lhs, VOptionBool::en rhs) { ආපසු lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, const VOptionBool& rhs) { ආපසු lhs == rhs.m_e; }

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
    string filename() const { ආපසු m_filename; }
    string libname() const { ආපසු m_libname; }
    bool operator==(const VFileLibName& rhs) const {
        ආපසු m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(const VFileLibName& rhs) const {
        නම් (m_filename < rhs.m_filename) ආපසු true;
        නම් (m_filename > rhs.m_filename) ආපසු false;
        ආපසු m_libname < rhs.m_libname;
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
    explicit VTimescale(පූර්ණ_සංඛ්‍යා _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(const string& value, bool& badr);
    VTimescale(dviguu_na value, bool& badr) {
        badr = false;
        සඳහා (පූර්ණ_සංඛ්‍යා i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            නම් (multiplier() == value) bha_nga;
        }
        නම් (multiplier() != value) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() const { ආපසු m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    static void parseSlashed(FileLine* fl, const var_na* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    const var_na* ascii() const {
        static const var_na* const names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        ආපසු names[m_e];
    }
    පූර්ණ_සංඛ්‍යා powerOfTen() const { ආපසු 2 - static_cast<පූර්ණ_සංඛ්‍යා>(m_e); }
    dviguu_na multiplier() const {
        static const dviguu_na values[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        ආපසු values[m_e];
    }
};
constexpr bool operator==(const VTimescale& lhs, const VTimescale& rhs) {
    ආපසු lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VTimescale& lhs, VTimescale::en rhs) { ආපසු lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, const VTimescale& rhs) { ආපසු lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(const VTimescale& lhs, const VTimescale& rhs) {
    ආපසු lhs.m_e > rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VTimescale& rhs) {
    ආපසු os << rhs.ascii();
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
    explicit VFsmExpandType(පූර්ණ_සංඛ්‍යා _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() const { ආපසු m_e; }
    constexpr const var_na* ascii() const {
        constexpr const var_na* const names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        ආපසු names[m_e];
    }
};
constexpr bool operator==(const VFsmExpandType& lhs, const VFsmExpandType& rhs) VL_MT_SAFE {
    ආපසු lhs.m_e == rhs.m_e;
}
constexpr bool operator==(const VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    ආපසු lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, const VFsmExpandType& rhs) VL_MT_SAFE {
    ආපසු lhs == rhs.m_e;
}
anta_hstha std::ostream& operator<<(std::ostream& os, const VFsmExpandType& rhs) VL_MT_SAFE {
    ආපසු os << rhs.ascii();
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
    const string& origName() const { ආපසු m_origName; }
    const string& mangledName() const { ආපසු m_mangledName; }
    const ParamStrMap params() const { ආපසු m_parameters; }
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
    පූර්ණ_සංඛ්‍යා m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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

    පූර්ණ_සංඛ්‍යා         m_buildJobs = -1;    // main switch: --build-jobs, -j
    පූර්ණ_සංඛ්‍යා         m_coverageExprMax = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    පූර්ණ_සංඛ්‍යා         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    පූර්ණ_සංඛ්‍යා         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    පූර්ණ_සංඛ්‍යා         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    පූර්ණ_සංඛ්‍යා         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    පූර්ණ_සංඛ්‍යා         m_expandLimit = 256;  // main switch: --expand-limit
    පූර්ණ_සංඛ්‍යා         m_gateStmts = 100;    // main switch: --gate-stmts
    පූර්ණ_සංඛ්‍යා         m_hierChild = 0;      // main switch: --hierarchical-child
    පූර්ණ_සංඛ්‍යා         m_hierThreads = 0;      // main switch: --hierarchical-threads
    පූර්ණ_සංඛ්‍යා         m_ifDepth = 0;      // main switch: --if-depth
    පූර්ණ_සංඛ්‍යා         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    පූර්ණ_සංඛ්‍යා         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    පූර්ණ_සංඛ්‍යා         m_inlineMult = 2000;   // main switch: --inline-mult
    පූර්ණ_සංඛ්‍යා         m_instrCountDpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    පූර්ණ_සංඛ්‍යා         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    පූර්ණ_සංඛ්‍යා         m_maxNumWidth = 65536;  // main switch: --max-num-width
    පූර්ණ_සංඛ්‍යා         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    පූර්ණ_සංඛ්‍යා         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    පූර්ණ_සංඛ්‍යා         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    පූර්ණ_සංඛ්‍යා         m_outputSplit = 20000;  // main switch: --output-split
    පූර්ණ_සංඛ්‍යා         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    පූර්ණ_සංඛ්‍යා         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    පූර්ණ_සංඛ්‍යා         m_pinsBv = 65;       // main switch: --pins-bv
    පූර්ණ_සංඛ්‍යා         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    පූර්ණ_සංඛ්‍යා         m_publicDepth = 0;   // main switch: --public-depth
    පූර්ණ_සංඛ්‍යා         m_reloopLimit = 40; // main switch: --reloop-limit
    පූර්ණ_සංඛ්‍යා         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    පූර්ණ_සංඛ්‍යා         m_threads = 1;      // main switch: --threads
    පූර්ණ_සංඛ්‍යා         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    පූර්ණ_සංඛ්‍යා         m_traceDepth = 0;   // main switch: --trace-depth
    පූර්ණ_සංඛ්‍යා         m_traceMaxArray = 32;  // main switch: --trace-max-array
    පූර්ණ_සංඛ්‍යා         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    පූර්ණ_සංඛ්‍යා         m_unrollCount = 64;  // main switch: --unroll-count
    පූර්ණ_සංඛ්‍යා         m_unrollLimit = 16384;  // main switch: --unroll-limit
    පූර්ණ_සංඛ්‍යා         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    පූර්ණ_සංඛ්‍යා         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    පූර්ණ_සංඛ්‍යා         m_verilateJobs = -1;  // main switch: --verilate-jobs

    පූර්ණ_සංඛ්‍යා         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    පූර්ණ_සංඛ්‍යා         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    පූර්ණ_සංඛ්‍යා         m_compLimitParens = 240;  // compiler selection; number of nested parens

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
    පූර්ණ_සංඛ්‍යා  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
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
    void optimize(පූර්ණ_සංඛ්‍යා level);
    void showVersion(bool verbose);
    void coverage(bool flag) {
        m_coverageLine = m_coverageBranch = m_coverageToggle = m_coverageExpr = m_coverageFsm
            = m_coverageUser = flag;
    }
    static bool suffixed(const string& sw, const var_na* arg);
    static string parseFileArg(const string& optdir, const string& relfilename);
    string filePathCheckOneDir(const string& modname, const string& dirname);
    static පූර්ණ_සංඛ්‍යා stripOptionsForChildRun(const string& opt, bool forTop);
    void validateIdentifier(FileLine* fl, const string& arg, const string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

public:
    V3Options();
    ~V3Options();
    void setDebugMode(පූර්ණ_සංඛ්‍යා level);
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
    bool available() const VL_MT_SAFE { ආපසු m_available; }
    void ccSet();
    void decorations(FileLine* fl, const string& filename);
    void notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() const { ආපසු m_preprocOnly; }
    bool makePhony() const { ආපසු m_makePhony; }
    bool preprocNoLine() const { ආපසු m_preprocNoLine; }
    bool preprocResolve() const { ආපසු m_preprocResolve; }
    පූර්ණ_සංඛ්‍යා preprocTokenLimit() const { ආපසු m_preprocTokenLimit; }
    bool underlineZero() const { ආපසු m_underlineZero; }
    bool systemC() const VL_MT_SAFE { ආපසු m_systemC; }
    bool savable() const VL_MT_SAFE { ආපසු m_savable; }
    VOptionBool schedZeroDelay() const { ආපසු m_schedZeroDelay; }
    bool stats() const { ආපසු m_stats; }
    bool statsVars() const { ආපසු m_statsVars; }
    bool stdPackage() const { ආපසු m_stdPackage; }
    bool stdWaiver() const { ආපසු m_stdWaiver; }
    bool assertOn() const { ආපසු m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() const { ආපසු m_assertCase; }
    bool autoflush() const { ආපසු m_autoflush; }
    bool bboxSys() const { ආපසු m_bboxSys; }
    bool bboxUnsup() const { ආපසු m_bboxUnsup; }
    bool build() const { ආපසු m_build; }
    string buildDepBin() const { ආපසු m_buildDepBin; }
    void buildDepBin(const string& flag) { m_buildDepBin = flag; }
    bool context() const VL_MT_SAFE { ආපසු m_context; }
    bool coverage() const VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        ආපසු m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() const VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        ආපසු m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser;
    }
    bool coverageBranch() const { ආපසු m_coverageBranch; }
    bool coverageExpr() const { ආපසු m_coverageExpr; }
    bool coverageFsm() const { ආපසු m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() const { ආපසු m_coverageFsmExpand; }
    පූර්ණ_සංඛ්‍යා coverageFsmMaxArcs() const { ආපසු m_coverageFsmMaxArcs; }
    bool coverageLine() const { ආපසු m_coverageLine; }
    bool coverageMergeInstances() const { ආපසු m_coverageMergeInstances; }
    bool coveragePerInstance() const { ආපසු m_coveragePerInstance; }
    bool coverageToggle() const { ආපසු m_coverageToggle; }
    bool coverageUnderscore() const { ආපසු m_coverageUnderscore; }
    bool coverageUser() const { ආපසු m_coverageUser; }
    bool debugCheck() const VL_MT_SAFE { ආපසු m_debugCheck; }
    bool debugCollision() const { ආපසු m_debugCollision; }
    bool debugEmitV() const VL_MT_SAFE { ආපසු m_debugEmitV; }
    bool debugExitElab() const { ආපසු m_debugExitElab; }
    bool debugExitParse() const { ආපසු m_debugExitParse; }
    bool debugLeak() const { ආපසු m_debugLeak; }
    bool debugNondeterminism() const { ආපසු m_debugNondeterminism; }
    bool debugPartition() const { ආපසු m_debugPartition; }
    bool debugPreprocPassthru() const VL_MT_SAFE { ආපසු m_debugPreprocPassthru; }
    bool debugProtect() const VL_MT_SAFE { ආපසු m_debugProtect; }
    පූර්ණ_සංඛ්‍යා debugRuntimeTimeout() const { ආපසු m_debugRuntimeTimeout; }
    bool debugSelfTest() const { ආපසු m_debugSelfTest; }
    bool debugStackCheck() const { ආපසු m_debugStackCheck; }
    bool debugWidth() const VL_PURE { ආපසු m_debugWidth; }
    bool decoration() const VL_MT_SAFE { ආපසු m_decoration; }
    bool decorationNodes() const VL_MT_SAFE { ආපසු m_decorationNodes; }
    bool diagnosticsSarif() const VL_MT_SAFE { ආපසු m_diagnosticsSarif; }
    bool dpiHdrOnly() const { ආපසු m_dpiHdrOnly; }
    bool dumpAstPatterns() const {
        ආපසු m_dumpLevel.count("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() const { ආපසු m_dumpLevel.count("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() const {
        ආපසු m_dumpLevel.count("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dumpTreeDot() const {
        ආපසු m_dumpLevel.count("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() const { ආපසු m_emitAccessors; }
    bool exe() const { ආපසු m_exe; }
    bool flatten() const { ආපසු m_flatten; }
    bool fourstate() const { ආපසු m_fourstate; }
    bool gmake() const { ආපසු m_gmake; }
    bool makeJson() const { ආපසු m_makeJson; }
    bool threadsDpiPure() const { ආපසු m_threadsDpiPure; }
    bool threadsDpiUnpure() const { ආපසු m_threadsDpiUnpure; }
    bool threadsCoarsen() const { ආපසු m_threadsCoarsen; }
    VOptionBool timing() const { ආපසු m_timing; }
    bool trace() const { ආපසු m_trace; }
    bool traceCoverage() const { ආපසු m_traceCoverage; }
    bool traceEnabledFst() const { ආපසු m_traceEnabledFst; }
    bool traceEnabledSaif() const { ආපසු m_traceEnabledSaif; }
    bool traceEnabledVcd() const { ආපසු m_traceEnabledVcd; }
    bool traceParams() const { ආපසු m_traceParams; }
    bool traceStructs() const { ආපසු m_traceStructs; }
    bool traceUnderscore() const { ආපසු m_traceUnderscore; }
    bool main() const { ආපසු m_main; }
    bool outFormatOk() const { ආපසු m_outFormatOk; }
    bool jsonOnly() const { ආපසු m_jsonOnly; }
    bool keepTempFiles() const { ආපසු (V3Error::debugDefault() != 0); }
    bool pedantic() const { ආපසු m_pedantic; }
    bool pinsInoutEnables() const { ආපසු m_pinsInoutEnables; }
    bool pinsScUint() const { ආපසු m_pinsScUint; }
    bool pinsScUintBool() const { ආපසු m_pinsScUintBool; }
    bool pinsScBigUint() const VL_MT_SAFE { ආපසු m_pinsScBigUint; }
    bool pinsUint8() const { ආපසු m_pinsUint8; }
    bool preprocComments() const { ආපසු m_preprocComments; }
    bool preprocDefines() const { ආපසු m_preprocDefines; }
    bool profC() const { ආපසු m_profC; }
    bool profCFuncs() const { ආපසු m_profCFuncs; }
    bool profExec() const { ආපසු m_profExec; }
    bool profPgo() const { ආපසු m_profPgo; }
    bool usesProfiler() const { ආපසු profExec() || profPgo(); }
    bool protectIds() const VL_MT_SAFE { ආපසු m_protectIds; }
    bool allPublic() const { ආපසු m_public; }
    bool publicParams() const { ආපසු m_publicParams; }
    bool publicOff() const { ආපසු m_publicIgnore; }
    bool publicFlatRW() const { ආපසු m_publicFlatRW; }
    පූර්ණ_සංඛ්‍යා publicDepth() const { ආපසු m_publicDepth; }
    bool anyPublicFlat() const { ආපසු m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() const VL_MT_SAFE { ආපසු m_lintOnly; }
    bool ignc() const { ආපසු m_ignc; }
    bool quietBuild() const VL_MT_SAFE { ආපසු m_quietBuild; }
    bool quietExit() const VL_MT_SAFE { ආපසු m_quietExit; }
    bool quietStats() const VL_MT_SAFE { ආපසු m_quietStats; }
    bool reportUnoptflat() const { ආපසු m_reportUnoptflat; }
    bool verilate() const { ආපසු m_verilate; }
    bool vpi() const { ආපසු m_vpi.isTrue(); }
    bool waiverMultiline() const { ආපසු m_waiverMultiline; }
    bool xInitialEdge() const { ආපසු m_xInitialEdge; }
    bool serializeOnly() const { ආපසු m_jsonOnly; }
    bool topIfacesSupported() const { ආපසු lintOnly() && !hierarchical(); }

    පූර්ණ_සංඛ්‍යා buildJobs() const VL_MT_SAFE { ආපසු m_buildJobs; }
    පූර්ණ_සංඛ්‍යා convergeLimit() const { ආපසු m_convergeLimit; }
    පූර්ණ_සංඛ්‍යා coverageExprMax() const { ආපසු m_coverageExprMax; }
    uint32_t coverageMaxBins() const { ආපසු m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() const { ආපසු m_coverageMaxRealBins; }
    පූර්ණ_සංඛ්‍යා coverageMaxWidth() const { ආපසු m_coverageMaxWidth; }
    පූර්ණ_සංඛ්‍යා debugAllocRandom() const { ආපසු m_debugAllocRandom; }
    bool dumpTreeAddrids() const VL_MT_SAFE;
    පූර්ණ_සංඛ්‍යා expandLimit() const { ආපසු m_expandLimit; }
    පූර්ණ_සංඛ්‍යා gateStmts() const { ආපසු m_gateStmts; }
    පූර්ණ_සංඛ්‍යා ifDepth() const { ආපසු m_ifDepth; }
    පූර්ණ_සංඛ්‍යා inlineCFuncs() const { ආපසු m_inlineCFuncs; }
    පූර්ණ_සංඛ්‍යා inlineCFuncsProduct() const { ආපසු m_inlineCFuncsProduct; }
    පූර්ණ_සංඛ්‍යා inlineMult() const { ආපසු m_inlineMult; }
    පූර්ණ_සංඛ්‍යා instrCountDpi() const { ආපසු m_instrCountDpi; }
    පූර්ණ_සංඛ්‍යා localizeMaxSize() const { ආපසු m_localizeMaxSize; }
    bool jsonEditNums() const { ආපසු m_jsonEditNums; }
    bool jsonIds() const { ආපසු m_jsonIds; }
    VOptionBool makeDepend() const { ආපසු m_makeDepend; }
    පූර්ණ_සංඛ්‍යා maxNumWidth() const { ආපසු m_maxNumWidth; }
    පූර්ණ_සංඛ්‍යා funcRecursionDepth() const { ආපසු m_funcRecursion; }
    පූර්ණ_සංඛ්‍යා moduleRecursionDepth() const { ආපසු m_moduleRecursion; }
    පූර්ණ_සංඛ්‍යා outputSplit() const { ආපසු m_outputSplit; }
    පූර්ණ_සංඛ්‍යා outputSplitCFuncs() const { ආපසු m_outputSplitCFuncs; }
    පූර්ණ_සංඛ්‍යා outputSplitCTrace() const { ආපසු m_outputSplitCTrace; }
    පූර්ණ_සංඛ්‍යා outputGroups() const { ආපසු m_outputGroups; }
    int64_t outputGroupsSize() const { ආපසු m_outputGroupsSize; }
    පූර්ණ_සංඛ්‍යා pinsBv() const VL_MT_SAFE { ආපසු m_pinsBv; }
    පූර්ණ_සංඛ්‍යා reloopLimit() const { ආපසු m_reloopLimit; }
    පූර්ණ_සංඛ්‍යා replicationLimit() const { ආපසු m_replicationLimit; }
    VOptionBool skipIdentical() const { ආපසු m_skipIdentical; }
    bool stopFail() const { ආපසු m_stopFail; }
    පූර්ණ_සංඛ්‍යා threads() const VL_MT_SAFE { ආපසු m_threads; }
    පූර්ණ_සංඛ්‍යා threadsMaxMTasks() const { ආපසු m_threadsMaxMTasks; }
    bool mtasks() const VL_MT_SAFE { ආපසු (m_threads > 1); }
    VTimescale timeDefaultPrec() const { ආපසු m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() const { ආපසු m_timeDefaultUnit; }
    VTimescale timeOverridePrec() const { ආපසු m_timeOverridePrec; }
    VTimescale timeOverrideUnit() const { ආපසු m_timeOverrideUnit; }
    VTimescale timeComputePrec(const VTimescale& flag) const;
    VTimescale timeComputeUnit(const VTimescale& flag) const;
    පූර්ණ_සංඛ්‍යා traceDepth() const { ආපසු m_traceDepth; }
    පූර්ණ_සංඛ්‍යා traceMaxArray() const { ආපසු m_traceMaxArray; }
    පූර්ණ_සංඛ්‍යා traceMaxWidth() const { ආපසු m_traceMaxWidth; }
    bool useTraceParallel() const {
        ආපසු trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    පූර්ණ_සංඛ්‍යා unrollCount() const { ආපසු m_unrollCount; }
    පූර්ණ_සංඛ්‍යා unrollLimit() const { ආපසු m_unrollLimit; }
    පූර්ණ_සංඛ්‍යා unrollStmts() const { ආපසු m_unrollStmts; }
    පූර්ණ_සංඛ්‍යා constraintArrayLimit() const { ආපසු m_constraintArrayLimit; }
    පූර්ණ_සංඛ්‍යා verilateJobs() const { ආපසු m_verilateJobs; }

    පූර්ණ_සංඛ්‍යා compLimitBlocks() const { ආපසු m_compLimitBlocks; }
    පූර්ණ_සංඛ්‍යා compLimitMembers() const VL_MT_SAFE { ආපසු m_compLimitMembers; }
    පූර්ණ_සංඛ්‍යා compLimitParens() const { ආපසු m_compLimitParens; }

    string diagnosticsSarifOutput() const VL_MT_SAFE {
        ආපසු m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() const { ආපසු m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() const { ආපසු m_hierParamsFile; }
    string jsonOnlyOutput() const { ආපසු m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() const { ආපසු m_jsonOnlyMetaOutput; }
    string l2Name() const { ආපසු m_l2Name; }
    string libCreate() const { ආපසු m_libCreate; }
    string libCreateName(bool shared) const {
        string libName = "lib" + libCreate();
        නම් (shared) {
            libName += ".so";
        } නැතහොත් {
            libName += ".a";
        }
        ආපසු libName;
    }
    string mainTopName() const { ආපසු m_mainTopName; }
    string makeDir() const VL_MT_SAFE { ආපසු m_makeDir; }
    string modPrefix() const VL_MT_SAFE { ආපසු m_modPrefix; }
    string pipeFilter() const { ආපසු m_pipeFilter; }
    string prefix() const VL_MT_SAFE { ආපසු m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protectKeyProvided() const { ආපසු !m_protectKey.empty(); }
    string protectKeyDefaulted() VL_MT_SAFE;  // Set default key if not set by user
    string topModule() const { ආපසු m_topModule; }  // As AstNode::encodeName()
    bool noTraceTop() const { ආපසු m_noTraceTop; }
    string unusedRegexp() const { ආපසු m_unusedRegexp; }
    string waiverOutput() const { ආපසු m_waiverOutput; }
    string work() const { ආපසු m_work; }
    bool isWaiverOutput() const { ආපසු !m_waiverOutput.empty(); }
    string xAssign() const { ආපසු m_xAssign; }
    string xInitial() const { ආපසු m_xInitial; }

    const VStringSet& cppFiles() const { ආපසු m_cppFiles; }
    const VStringList& cFlags() const { ආපසු m_cFlags; }
    const VStringSet& compilerIncludes() const { ආපසු m_compilerIncludes; }
    const VStringList& ldLibFiles() const { ආපසු m_ldLibFiles; }
    const VStringList& ldLibs() const { ආපසු m_ldLibs; }
    const VStringList& makeFlags() const { ආපසු m_makeFlags; }
    const VFileLibSet& libraryFiles() const { ආපසු m_libraryFiles; }
    const VStringSet& libmapFiles() const { ආපසු m_libmapFiles; }
    const VFileLibList& vFiles() const { ආපසු m_vFiles; }
    const VFileLibSet& vltFiles() const { ආපසු m_vltFiles; }
    const VStringList& forceIncs() const { ආපසු m_forceIncs; }

    bool hasParameter(const string& name);
    string parameter(const string& name);
    void checkParameters();

    bool isFuture(const string& flag) const;
    bool isFuture0(const string& flag) const;
    bool isFuture1(const string& flag) const;
    bool isLibraryFile(const string& filename, const string& libname) const;

    // ACCESSORS (optimization options)
    bool fAcycSimp() const { ආපසු m_fAcycSimp; }
    bool fAssemble() const { ආපසු m_fAssemble; }
    bool fBitScanLoops() const { ආපසු m_fBitScanLoops; }
    bool fBitblast() const { ආපසු m_fBitblast; }
    bool fCaseDecoder() const { ආපසු m_fCaseDecoder; }
    bool fCaseTable() const { ආපසු m_fCaseTable; }
    bool fCaseTree() const { ආපසු m_fCaseTree; }
    bool fCombine() const { ආපසු m_fCombine; }
    bool fConst() const { ආපසු m_fConst; }
    bool fConstBeforeDfg() const { ආපසු m_fConstBeforeDfg; }
    bool fConstBitOpTree() const { ආපසු m_fConstBitOpTree; }
    bool fConstEager() const { ආපසු m_fConstEager; }
    bool fDedupe() const { ආපසු m_fDedupe; }
    bool fDfg() const { ආපසු m_fDfg; }
    bool fDfgPeephole() const { ආපසු m_fDfgPeephole; }
    bool fDfgPushDownSels() const { ආපසු m_fDfgPushDownSels; }
    bool fDfgSynthesizeAll() const { ආපසු m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(const std::string& name) const {
        ආපසු !m_fDfgPeepholeDisabled.count(name);
    }
    bool fDeadAssigns() const { ආපසු m_fDeadAssigns; }
    bool fDeadCells() const { ආපසු m_fDeadCells; }
    bool fDeadMethods() const { ආපසු m_fDeadMethods; }
    bool fDecompose() const { ආපසු m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() const { ආපසු m_fDecomposePacked; }
    bool fDecomposeUnpacked() const { ආපසු m_fDecomposeUnpacked; }
    bool fExpand() const { ආපසු m_fExpand; }
    bool fFuncBalanceCat() const { ආපසු m_fFuncBalanceCat; }
    bool fFuncSplitCat() const { ආපසු m_fFuncSplitCat; }
    bool fFunc() const { ආපසු fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() const { ආපසු m_fGate; }
    VOptionBool fIcoChangeDetect() const { ආපසු m_fIcoChangeDetect; }
    bool fInline() const { ආපසු m_fInline; }
    bool fInlineCFuncs() const { ආපසු m_fInlineCFuncs; }
    bool fInlineFuncs() const { ආපසු m_fInlineFuncs; }
    bool fInlineFuncsEager() const { ආපසු m_fInlineFuncsEager; }
    bool fLife() const { ආපසු m_fLife; }
    bool fLifePost() const { ආපසු m_fLifePost; }
    bool fLocalize() const { ආපසු m_fLocalize; }
    bool fMergeCond() const { ආපසු m_fMergeCond; }
    bool fMergeCondMotion() const { ආපසු m_fMergeCondMotion; }
    bool fReloop() const { ආපසු m_fReloop; }
    bool fReorder() const { ආපසු m_fReorder; }
    bool fSlice() const { ආපසු m_fSlice; }
    පූර්ණ_සංඛ්‍යා fSliceElementLimit() const { ආපසු m_fSliceElementLimit; }
    bool fSplit() const { ආපසු m_fSplit; }
    bool fSubst() const { ආපසු m_fSubst; }
    bool fSubstConst() const { ආපසු m_fSubstConst; }
    bool fTable() const { ආපසු m_fTable; }
    bool fTaskifyAll() const { ආපසු m_fTaskifyAll; }

    std::string traceClassBase() const VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() const VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() const VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() const VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() const VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() const VL_MT_SAFE;

    bool hierarchical() const { ආපසු m_hierarchical; }
    පූර්ණ_සංඛ්‍යා hierChild() const VL_MT_SAFE { ආපසු m_hierChild; }
    පූර්ණ_සංඛ්‍යා hierThreads() const VL_MT_SAFE { ආපසු m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hierTop() const VL_MT_SAFE { ආපසු !m_hierChild && !m_hierBlocks.empty(); }
    const V3HierBlockOptSet& hierBlocks() const { ආපසු m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hierTopDataDir() const VL_MT_SAFE {
        ආපසු hierTop() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    static string version() VL_PURE;
    static string argString(පූර්ණ_සංඛ්‍යා argc, var_na** argv);  ///< Return list of arguments as simple string
    string allArgsString() const VL_MT_SAFE;  ///< Return all passed arguments as simple string
    const std::list<std::pair<std::list<std::string>, bool>>& allArgs() const;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool forTop) const;
    void parseOpts(FileLine* fl, පූර්ණ_සංඛ්‍යා argc, var_na** argv) VL_MT_DISABLED;
    void parseOptsList(FileLine* fl, const string& optdir, පූර්ණ_සංඛ්‍යා argc, var_na** argv) VL_MT_DISABLED;
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
