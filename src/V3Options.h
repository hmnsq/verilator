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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3LangCode.h"

#ضمن <list>
#ضمن <map>
#ضمن <set>
#ضمن <string>
#ضمن <vector>

class V3OptionsImp;
class FileLine;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
عام:
    تعداد en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    تعداد en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { החזר m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        החזר names[m_e];
    }
    bool isDefault() ثابت { החזר m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ثابت { החזר m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() ثابت { החזר m_e == OPT_TRUE; }
    bool isSetFalse() ثابت { החזר m_e == OPT_FALSE; }
    فراغ setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(ثابت VOptionBool& lhs, ثابت VOptionBool& rhs) {
    החזר lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VOptionBool& lhs, VOptionBool::en rhs) { החזר lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, ثابت VOptionBool& rhs) { החזר lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ثابت string m_filename;  // Filename
    ثابت string m_libname;  // Libname
عام:
    VFileLibName(ثابت string& filename, ثابت string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ثابت VFileLibName& rhs) = افتراضي;
    string filename() ثابت { החזר m_filename; }
    string libname() ثابت { החזר m_libname; }
    bool operator==(ثابت VFileLibName& rhs) ثابت {
        החזר m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(ثابت VFileLibName& rhs) ثابت {
        אם (m_filename < rhs.m_filename) החזר true;
        אם (m_filename > rhs.m_filename) החזר false;
        החזר m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
عام:
    تعداد en : uint8_t {
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
    تعداد : uint8_t { TS_DEFAULT = TS_1PS };
    تعداد en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ثابت string& value, bool& badr);
    VTimescale(مزدوج value, bool& badr) {
        badr = false;
        עבור (صحيح i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            אם (multiplier() == value) اكسر;
        }
        אם (multiplier() != value) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ثابت { החזר m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ساكن فراغ parseSlashed(FileLine* fl, ثابت حرف* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        החזר names[m_e];
    }
    صحيح powerOfTen() ثابت { החזר 2 - static_cast<صحيح>(m_e); }
    مزدوج multiplier() ثابت {
        ساكن ثابت مزدوج values[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        החזר values[m_e];
    }
};
constexpr bool operator==(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    החזר lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VTimescale& lhs, VTimescale::en rhs) { החזר lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, ثابت VTimescale& rhs) { החזר lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    החזר lhs.m_e > rhs.m_e;
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VTimescale& rhs) {
    החזר os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
عام:
    تعداد en : uint8_t {
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
    explicit VFsmExpandType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { החזר m_e; }
    constexpr ثابت حرف* ascii() ثابت {
        constexpr ثابت حرف* ثابت names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        החזר names[m_e];
    }
};
constexpr bool operator==(ثابت VFsmExpandType& lhs, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    החזר lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    החזר lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    החזר lhs == rhs.m_e;
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    החזר os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
عام:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ثابت std::string, std::string>;

خاص:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

عام:
    explicit V3HierarchicalBlockOption(ثابت string& optstring);
    ثابت string& origName() ثابت { החזר m_origName; }
    ثابت string& mangledName() ثابت { החזר m_mangledName; }
    ثابت ParamStrMap params() ثابت { החזר m_parameters; }
};

using V3HierBlockOptSet = std::map<ثابت std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
عام:
خاص:
    // TYPES
    using DebugLevelMap = std::map<ثابت std::string, unsigned>;

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
    std::map<ثابت string, string> m_parameters;  // Parameters
    std::map<ثابت string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
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
    صحيح m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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

    صحيح         m_buildJobs = -1;    // main switch: --build-jobs, -j
    صحيح         m_coverageExprMax = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    صحيح         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    صحيح         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    صحيح         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    صحيح         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    صحيح         m_expandLimit = 256;  // main switch: --expand-limit
    صحيح         m_gateStmts = 100;    // main switch: --gate-stmts
    صحيح         m_hierChild = 0;      // main switch: --hierarchical-child
    صحيح         m_hierThreads = 0;      // main switch: --hierarchical-threads
    صحيح         m_ifDepth = 0;      // main switch: --if-depth
    صحيح         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    صحيح         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    صحيح         m_inlineMult = 2000;   // main switch: --inline-mult
    صحيح         m_instrCountDpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    صحيح         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    صحيح         m_maxNumWidth = 65536;  // main switch: --max-num-width
    صحيح         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    صحيح         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    صحيح         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    صحيح         m_outputSplit = 20000;  // main switch: --output-split
    صحيح         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    صحيح         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    صحيح         m_pinsBv = 65;       // main switch: --pins-bv
    صحيح         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    صحيح         m_publicDepth = 0;   // main switch: --public-depth
    صحيح         m_reloopLimit = 40; // main switch: --reloop-limit
    صحيح         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    صحيح         m_threads = 1;      // main switch: --threads
    صحيح         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    صحيح         m_traceDepth = 0;   // main switch: --trace-depth
    صحيح         m_traceMaxArray = 32;  // main switch: --trace-max-array
    صحيح         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    صحيح         m_unrollCount = 64;  // main switch: --unroll-count
    صحيح         m_unrollLimit = 16384;  // main switch: --unroll-limit
    صحيح         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    صحيح         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    صحيح         m_verilateJobs = -1;  // main switch: --verilate-jobs

    صحيح         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    صحيح         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    صحيح         m_compLimitParens = 240;  // compiler selection; number of nested parens

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
    صحيح  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    فراغ addArg(حرف** argv, size_t count, bool isForRerun);
    فراغ addArg(ثابت std::string& arg, bool isForRerun);
    فراغ addLineArg(ثابت string& arg);
    فراغ addDefine(ثابت string& defline, bool allowPlus) VL_MT_DISABLED;
    فراغ addFuture(ثابت string& flag);
    فراغ addFuture0(ثابت string& flag);
    فراغ addFuture1(ثابت string& flag);
    فراغ addIncDirUser(ثابت string& incdir);  // User requested
    فراغ addIncDirFallback(ثابت string& incdir);  // Low priority if not found otherwise
    فراغ addParameter(ثابت string& paramline, bool allowPlus);
    فراغ addLangExt(ثابت string& langext, ثابت V3LangCode& lc);
    فراغ addLibExtV(ثابت string& libext);
    فراغ optimize(صحيح level);
    فراغ showVersion(bool verbose);
    فراغ coverage(bool flag) {
        m_coverageLine = m_coverageBranch = m_coverageToggle = m_coverageExpr = m_coverageFsm
            = m_coverageUser = flag;
    }
    ساكن bool suffixed(ثابت string& sw, ثابت حرف* arg);
    ساكن string parseFileArg(ثابت string& optdir, ثابت string& relfilename);
    string filePathCheckOneDir(ثابت string& modname, ثابت string& dirname);
    ساكن صحيح stripOptionsForChildRun(ثابت string& opt, bool forTop);
    فراغ validateIdentifier(FileLine* fl, ثابت string& arg, ثابت string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

عام:
    V3Options();
    ~V3Options();
    فراغ setDebugMode(صحيح level);
    unsigned debugLevel(ثابت string& tag) ثابت VL_MT_SAFE;
    unsigned debugSrcLevel(ثابت string& srcfile_path) ثابت VL_MT_SAFE;
    unsigned dumpLevel(ثابت string& tag) ثابت VL_MT_SAFE;
    unsigned dumpSrcLevel(ثابت string& srcfile_path) ثابت VL_MT_SAFE;

    // METHODS
    فراغ addCppFile(ثابت string& filename);
    فراغ addCFlags(ثابت string& filename);
    فراغ addCompilerIncludes(ثابت string& filename);
    فراغ addLdLibFile(ثابت string& filename);
    فراغ addLdLibs(ثابت string& filename);
    فراغ addMakeFlags(ثابت string& filename);
    فراغ addLibMapFile(ثابت string& filename);
    فراغ addLibraryFile(ثابت string& filename, ثابت string& libname);
    فراغ addVFile(ثابت string& filename, ثابت string& libname);
    فراغ addVltFile(ثابت string& filename, ثابت string& libname);
    فراغ addForceInc(ثابت string& filename);
    bool available() ثابت VL_MT_SAFE { החזר m_available; }
    فراغ ccSet();
    فراغ decorations(FileLine* fl, ثابت string& filename);
    فراغ notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ثابت { החזר m_preprocOnly; }
    bool makePhony() ثابت { החזר m_makePhony; }
    bool preprocNoLine() ثابت { החזר m_preprocNoLine; }
    bool preprocResolve() ثابت { החזר m_preprocResolve; }
    صحيح preprocTokenLimit() ثابت { החזר m_preprocTokenLimit; }
    bool underlineZero() ثابت { החזר m_underlineZero; }
    bool systemC() ثابت VL_MT_SAFE { החזר m_systemC; }
    bool savable() ثابت VL_MT_SAFE { החזר m_savable; }
    VOptionBool schedZeroDelay() ثابت { החזר m_schedZeroDelay; }
    bool stats() ثابت { החזר m_stats; }
    bool statsVars() ثابت { החזר m_statsVars; }
    bool stdPackage() ثابت { החזר m_stdPackage; }
    bool stdWaiver() ثابت { החזר m_stdWaiver; }
    bool assertOn() ثابت { החזר m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ثابت { החזר m_assertCase; }
    bool autoflush() ثابت { החזר m_autoflush; }
    bool bboxSys() ثابت { החזר m_bboxSys; }
    bool bboxUnsup() ثابت { החזר m_bboxUnsup; }
    bool build() ثابت { החזר m_build; }
    string buildDepBin() ثابت { החזר m_buildDepBin; }
    فراغ buildDepBin(ثابت string& flag) { m_buildDepBin = flag; }
    bool context() ثابت VL_MT_SAFE { החזר m_context; }
    bool coverage() ثابت VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        החזר m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ثابت VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        החזר m_coverageLine || m_coverageBranch || m_coverageToggle || m_coverageExpr
               || m_coverageUser;
    }
    bool coverageBranch() ثابت { החזר m_coverageBranch; }
    bool coverageExpr() ثابت { החזר m_coverageExpr; }
    bool coverageFsm() ثابت { החזר m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ثابت { החזר m_coverageFsmExpand; }
    صحيح coverageFsmMaxArcs() ثابت { החזר m_coverageFsmMaxArcs; }
    bool coverageLine() ثابت { החזר m_coverageLine; }
    bool coverageMergeInstances() ثابت { החזר m_coverageMergeInstances; }
    bool coveragePerInstance() ثابت { החזר m_coveragePerInstance; }
    bool coverageToggle() ثابت { החזר m_coverageToggle; }
    bool coverageUnderscore() ثابت { החזר m_coverageUnderscore; }
    bool coverageUser() ثابت { החזר m_coverageUser; }
    bool debugCheck() ثابت VL_MT_SAFE { החזר m_debugCheck; }
    bool debugCollision() ثابت { החזר m_debugCollision; }
    bool debugEmitV() ثابت VL_MT_SAFE { החזר m_debugEmitV; }
    bool debugExitElab() ثابت { החזר m_debugExitElab; }
    bool debugExitParse() ثابت { החזר m_debugExitParse; }
    bool debugLeak() ثابت { החזר m_debugLeak; }
    bool debugNondeterminism() ثابت { החזר m_debugNondeterminism; }
    bool debugPartition() ثابت { החזר m_debugPartition; }
    bool debugPreprocPassthru() ثابت VL_MT_SAFE { החזר m_debugPreprocPassthru; }
    bool debugProtect() ثابت VL_MT_SAFE { החזר m_debugProtect; }
    صحيح debugRuntimeTimeout() ثابت { החזר m_debugRuntimeTimeout; }
    bool debugSelfTest() ثابت { החזר m_debugSelfTest; }
    bool debugStackCheck() ثابت { החזר m_debugStackCheck; }
    bool debugWidth() ثابت VL_PURE { החזר m_debugWidth; }
    bool decoration() ثابت VL_MT_SAFE { החזר m_decoration; }
    bool decorationNodes() ثابت VL_MT_SAFE { החזר m_decorationNodes; }
    bool diagnosticsSarif() ثابت VL_MT_SAFE { החזר m_diagnosticsSarif; }
    bool dpiHdrOnly() ثابت { החזר m_dpiHdrOnly; }
    bool dumpAstPatterns() ثابت {
        החזר m_dumpLevel.count("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() ثابت { החזר m_dumpLevel.count("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ثابت {
        החזר m_dumpLevel.count("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dumpTreeDot() ثابت {
        החזר m_dumpLevel.count("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ثابت { החזר m_emitAccessors; }
    bool exe() ثابت { החזר m_exe; }
    bool flatten() ثابت { החזר m_flatten; }
    bool fourstate() ثابت { החזר m_fourstate; }
    bool gmake() ثابت { החזר m_gmake; }
    bool makeJson() ثابت { החזר m_makeJson; }
    bool threadsDpiPure() ثابت { החזר m_threadsDpiPure; }
    bool threadsDpiUnpure() ثابت { החזר m_threadsDpiUnpure; }
    bool threadsCoarsen() ثابت { החזר m_threadsCoarsen; }
    VOptionBool timing() ثابت { החזר m_timing; }
    bool trace() ثابت { החזר m_trace; }
    bool traceCoverage() ثابت { החזר m_traceCoverage; }
    bool traceEnabledFst() ثابت { החזר m_traceEnabledFst; }
    bool traceEnabledSaif() ثابت { החזר m_traceEnabledSaif; }
    bool traceEnabledVcd() ثابت { החזר m_traceEnabledVcd; }
    bool traceParams() ثابت { החזר m_traceParams; }
    bool traceStructs() ثابت { החזר m_traceStructs; }
    bool traceUnderscore() ثابت { החזר m_traceUnderscore; }
    bool main() ثابت { החזר m_main; }
    bool outFormatOk() ثابت { החזר m_outFormatOk; }
    bool jsonOnly() ثابت { החזר m_jsonOnly; }
    bool keepTempFiles() ثابت { החזר (V3Error::debugDefault() != 0); }
    bool pedantic() ثابت { החזר m_pedantic; }
    bool pinsInoutEnables() ثابت { החזר m_pinsInoutEnables; }
    bool pinsScUint() ثابت { החזר m_pinsScUint; }
    bool pinsScUintBool() ثابت { החזר m_pinsScUintBool; }
    bool pinsScBigUint() ثابت VL_MT_SAFE { החזר m_pinsScBigUint; }
    bool pinsUint8() ثابت { החזר m_pinsUint8; }
    bool preprocComments() ثابت { החזר m_preprocComments; }
    bool preprocDefines() ثابت { החזר m_preprocDefines; }
    bool profC() ثابت { החזר m_profC; }
    bool profCFuncs() ثابت { החזר m_profCFuncs; }
    bool profExec() ثابت { החזר m_profExec; }
    bool profPgo() ثابت { החזר m_profPgo; }
    bool usesProfiler() ثابت { החזר profExec() || profPgo(); }
    bool protectIds() ثابت VL_MT_SAFE { החזר m_protectIds; }
    bool allPublic() ثابت { החזר m_public; }
    bool publicParams() ثابت { החזר m_publicParams; }
    bool publicOff() ثابت { החזר m_publicIgnore; }
    bool publicFlatRW() ثابت { החזר m_publicFlatRW; }
    صحيح publicDepth() ثابت { החזר m_publicDepth; }
    bool anyPublicFlat() ثابت { החזר m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() ثابت VL_MT_SAFE { החזר m_lintOnly; }
    bool ignc() ثابت { החזר m_ignc; }
    bool quietBuild() ثابت VL_MT_SAFE { החזר m_quietBuild; }
    bool quietExit() ثابت VL_MT_SAFE { החזר m_quietExit; }
    bool quietStats() ثابت VL_MT_SAFE { החזר m_quietStats; }
    bool reportUnoptflat() ثابت { החזר m_reportUnoptflat; }
    bool verilate() ثابت { החזר m_verilate; }
    bool vpi() ثابت { החזר m_vpi.isTrue(); }
    bool waiverMultiline() ثابت { החזר m_waiverMultiline; }
    bool xInitialEdge() ثابت { החזר m_xInitialEdge; }
    bool serializeOnly() ثابت { החזר m_jsonOnly; }
    bool topIfacesSupported() ثابت { החזר lintOnly() && !hierarchical(); }

    صحيح buildJobs() ثابت VL_MT_SAFE { החזר m_buildJobs; }
    صحيح convergeLimit() ثابت { החזר m_convergeLimit; }
    صحيح coverageExprMax() ثابت { החזר m_coverageExprMax; }
    uint32_t coverageMaxBins() ثابت { החזר m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ثابت { החזר m_coverageMaxRealBins; }
    صحيح coverageMaxWidth() ثابت { החזר m_coverageMaxWidth; }
    صحيح debugAllocRandom() ثابت { החזר m_debugAllocRandom; }
    bool dumpTreeAddrids() ثابت VL_MT_SAFE;
    صحيح expandLimit() ثابت { החזר m_expandLimit; }
    صحيح gateStmts() ثابت { החזר m_gateStmts; }
    صحيح ifDepth() ثابت { החזר m_ifDepth; }
    صحيح inlineCFuncs() ثابت { החזר m_inlineCFuncs; }
    صحيح inlineCFuncsProduct() ثابت { החזר m_inlineCFuncsProduct; }
    صحيح inlineMult() ثابت { החזר m_inlineMult; }
    صحيح instrCountDpi() ثابت { החזר m_instrCountDpi; }
    صحيح localizeMaxSize() ثابت { החזר m_localizeMaxSize; }
    bool jsonEditNums() ثابت { החזר m_jsonEditNums; }
    bool jsonIds() ثابت { החזר m_jsonIds; }
    VOptionBool makeDepend() ثابت { החזר m_makeDepend; }
    صحيح maxNumWidth() ثابت { החזר m_maxNumWidth; }
    صحيح funcRecursionDepth() ثابت { החזר m_funcRecursion; }
    صحيح moduleRecursionDepth() ثابت { החזר m_moduleRecursion; }
    صحيح outputSplit() ثابت { החזר m_outputSplit; }
    صحيح outputSplitCFuncs() ثابت { החזר m_outputSplitCFuncs; }
    صحيح outputSplitCTrace() ثابت { החזר m_outputSplitCTrace; }
    صحيح outputGroups() ثابت { החזר m_outputGroups; }
    int64_t outputGroupsSize() ثابت { החזר m_outputGroupsSize; }
    صحيح pinsBv() ثابت VL_MT_SAFE { החזר m_pinsBv; }
    صحيح reloopLimit() ثابت { החזר m_reloopLimit; }
    صحيح replicationLimit() ثابت { החזר m_replicationLimit; }
    VOptionBool skipIdentical() ثابت { החזר m_skipIdentical; }
    bool stopFail() ثابت { החזר m_stopFail; }
    صحيح threads() ثابت VL_MT_SAFE { החזר m_threads; }
    صحيح threadsMaxMTasks() ثابت { החזר m_threadsMaxMTasks; }
    bool mtasks() ثابت VL_MT_SAFE { החזר (m_threads > 1); }
    VTimescale timeDefaultPrec() ثابت { החזר m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() ثابت { החזר m_timeDefaultUnit; }
    VTimescale timeOverridePrec() ثابت { החזר m_timeOverridePrec; }
    VTimescale timeOverrideUnit() ثابت { החזר m_timeOverrideUnit; }
    VTimescale timeComputePrec(ثابت VTimescale& flag) ثابت;
    VTimescale timeComputeUnit(ثابت VTimescale& flag) ثابت;
    صحيح traceDepth() ثابت { החזר m_traceDepth; }
    صحيح traceMaxArray() ثابت { החזר m_traceMaxArray; }
    صحيح traceMaxWidth() ثابت { החזר m_traceMaxWidth; }
    bool useTraceParallel() ثابت {
        החזר trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    صحيح unrollCount() ثابت { החזר m_unrollCount; }
    صحيح unrollLimit() ثابت { החזר m_unrollLimit; }
    صحيح unrollStmts() ثابت { החזר m_unrollStmts; }
    صحيح constraintArrayLimit() ثابت { החזר m_constraintArrayLimit; }
    صحيح verilateJobs() ثابت { החזר m_verilateJobs; }

    صحيح compLimitBlocks() ثابت { החזר m_compLimitBlocks; }
    صحيح compLimitMembers() ثابت VL_MT_SAFE { החזר m_compLimitMembers; }
    صحيح compLimitParens() ثابت { החזר m_compLimitParens; }

    string diagnosticsSarifOutput() ثابت VL_MT_SAFE {
        החזר m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ثابت { החזר m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() ثابت { החזר m_hierParamsFile; }
    string jsonOnlyOutput() ثابت { החזר m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ثابت { החזר m_jsonOnlyMetaOutput; }
    string l2Name() ثابت { החזר m_l2Name; }
    string libCreate() ثابت { החזר m_libCreate; }
    string libCreateName(bool shared) ثابت {
        string libName = "lib" + libCreate();
        אם (shared) {
            libName += ".so";
        } אחרת {
            libName += ".a";
        }
        החזר libName;
    }
    string mainTopName() ثابت { החזר m_mainTopName; }
    string makeDir() ثابت VL_MT_SAFE { החזר m_makeDir; }
    string modPrefix() ثابت VL_MT_SAFE { החזר m_modPrefix; }
    string pipeFilter() ثابت { החזר m_pipeFilter; }
    string prefix() ثابت VL_MT_SAFE { החזר m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protectKeyProvided() ثابت { החזר !m_protectKey.empty(); }
    string protectKeyDefaulted() VL_MT_SAFE;  // Set default key if not set by user
    string topModule() ثابت { החזר m_topModule; }  // As AstNode::encodeName()
    bool noTraceTop() ثابت { החזר m_noTraceTop; }
    string unusedRegexp() ثابت { החזר m_unusedRegexp; }
    string waiverOutput() ثابت { החזר m_waiverOutput; }
    string work() ثابت { החזר m_work; }
    bool isWaiverOutput() ثابت { החזר !m_waiverOutput.empty(); }
    string xAssign() ثابت { החזר m_xAssign; }
    string xInitial() ثابت { החזר m_xInitial; }

    ثابت VStringSet& cppFiles() ثابت { החזר m_cppFiles; }
    ثابت VStringList& cFlags() ثابت { החזר m_cFlags; }
    ثابت VStringSet& compilerIncludes() ثابت { החזר m_compilerIncludes; }
    ثابت VStringList& ldLibFiles() ثابت { החזר m_ldLibFiles; }
    ثابت VStringList& ldLibs() ثابت { החזר m_ldLibs; }
    ثابت VStringList& makeFlags() ثابت { החזר m_makeFlags; }
    ثابت VFileLibSet& libraryFiles() ثابت { החזר m_libraryFiles; }
    ثابت VStringSet& libmapFiles() ثابت { החזר m_libmapFiles; }
    ثابت VFileLibList& vFiles() ثابت { החזר m_vFiles; }
    ثابت VFileLibSet& vltFiles() ثابت { החזר m_vltFiles; }
    ثابت VStringList& forceIncs() ثابت { החזר m_forceIncs; }

    bool hasParameter(ثابت string& name);
    string parameter(ثابت string& name);
    فراغ checkParameters();

    bool isFuture(ثابت string& flag) ثابت;
    bool isFuture0(ثابت string& flag) ثابت;
    bool isFuture1(ثابت string& flag) ثابت;
    bool isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ثابت { החזר m_fAcycSimp; }
    bool fAssemble() ثابت { החזר m_fAssemble; }
    bool fBitScanLoops() ثابت { החזר m_fBitScanLoops; }
    bool fBitblast() ثابت { החזר m_fBitblast; }
    bool fCaseDecoder() ثابت { החזר m_fCaseDecoder; }
    bool fCaseTable() ثابت { החזר m_fCaseTable; }
    bool fCaseTree() ثابت { החזר m_fCaseTree; }
    bool fCombine() ثابت { החזר m_fCombine; }
    bool fConst() ثابت { החזר m_fConst; }
    bool fConstBeforeDfg() ثابت { החזר m_fConstBeforeDfg; }
    bool fConstBitOpTree() ثابت { החזר m_fConstBitOpTree; }
    bool fConstEager() ثابت { החזר m_fConstEager; }
    bool fDedupe() ثابت { החזר m_fDedupe; }
    bool fDfg() ثابت { החזר m_fDfg; }
    bool fDfgPeephole() ثابت { החזר m_fDfgPeephole; }
    bool fDfgPushDownSels() ثابت { החזר m_fDfgPushDownSels; }
    bool fDfgSynthesizeAll() ثابت { החזר m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ثابت std::string& name) ثابت {
        החזר !m_fDfgPeepholeDisabled.count(name);
    }
    bool fDeadAssigns() ثابت { החזר m_fDeadAssigns; }
    bool fDeadCells() ثابت { החזר m_fDeadCells; }
    bool fDeadMethods() ثابت { החזר m_fDeadMethods; }
    bool fDecompose() ثابت { החזר m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ثابت { החזר m_fDecomposePacked; }
    bool fDecomposeUnpacked() ثابت { החזר m_fDecomposeUnpacked; }
    bool fExpand() ثابت { החזר m_fExpand; }
    bool fFuncBalanceCat() ثابت { החזר m_fFuncBalanceCat; }
    bool fFuncSplitCat() ثابت { החזר m_fFuncSplitCat; }
    bool fFunc() ثابت { החזר fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() ثابت { החזר m_fGate; }
    VOptionBool fIcoChangeDetect() ثابت { החזר m_fIcoChangeDetect; }
    bool fInline() ثابت { החזר m_fInline; }
    bool fInlineCFuncs() ثابت { החזר m_fInlineCFuncs; }
    bool fInlineFuncs() ثابت { החזר m_fInlineFuncs; }
    bool fInlineFuncsEager() ثابت { החזר m_fInlineFuncsEager; }
    bool fLife() ثابت { החזר m_fLife; }
    bool fLifePost() ثابت { החזר m_fLifePost; }
    bool fLocalize() ثابت { החזר m_fLocalize; }
    bool fMergeCond() ثابت { החזר m_fMergeCond; }
    bool fMergeCondMotion() ثابت { החזר m_fMergeCondMotion; }
    bool fReloop() ثابت { החזר m_fReloop; }
    bool fReorder() ثابت { החזר m_fReorder; }
    bool fSlice() ثابت { החזר m_fSlice; }
    صحيح fSliceElementLimit() ثابت { החזר m_fSliceElementLimit; }
    bool fSplit() ثابت { החזר m_fSplit; }
    bool fSubst() ثابت { החזר m_fSubst; }
    bool fSubstConst() ثابت { החזר m_fSubstConst; }
    bool fTable() ثابت { החזר m_fTable; }
    bool fTaskifyAll() ثابت { החזר m_fTaskifyAll; }

    std::string traceClassBase() ثابت VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() ثابت VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() ثابت VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() ثابت VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() ثابت VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() ثابت VL_MT_SAFE;

    bool hierarchical() ثابت { החזר m_hierarchical; }
    صحيح hierChild() ثابت VL_MT_SAFE { החזר m_hierChild; }
    صحيح hierThreads() ثابت VL_MT_SAFE { החזר m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hierTop() ثابت VL_MT_SAFE { החזר !m_hierChild && !m_hierBlocks.empty(); }
    ثابت V3HierBlockOptSet& hierBlocks() ثابت { החזר m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hierTopDataDir() ثابت VL_MT_SAFE {
        החזר hierTop() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ساكن string version() VL_PURE;
    ساكن string argString(صحيح argc, حرف** argv);  ///< Return list of arguments as simple string
    string allArgsString() ثابت VL_MT_SAFE;  ///< Return all passed arguments as simple string
    ثابت std::list<std::pair<std::list<std::string>, bool>>& allArgs() ثابت;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool forTop) ثابت;
    فراغ parseOpts(FileLine* fl, صحيح argc, حرف** argv) VL_MT_DISABLED;
    فراغ parseOptsList(FileLine* fl, ثابت string& optdir, صحيح argc, حرف** argv) VL_MT_DISABLED;
    فراغ parseOptsFile(FileLine* fl, ثابت string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    ساكن string getenvBuiltins(ثابت string& var);
    ساكن string getenvMAKE();
    ساكن string getenvMAKEFLAGS();
    ساكن string getenvPERL();
    ساكن string getenvPYTHON3();
    ساكن string getenvSYSTEMC();
    ساكن string getenvSYSTEMC_ARCH();
    ساكن string getenvSYSTEMC_INCLUDE();
    ساكن string getenvSYSTEMC_LIBDIR();
    ساكن string getenvVERILATOR_ROOT();
    ساكن string getenvVERILATOR_SOLVER();
    ساكن string getStdPackagePath();
    ساكن string getStdWaiverPath();
    ساكن string getSupported(ثابت string& var);
    ساكن bool systemCSystemWide();
    ساكن bool systemCFound();  // SystemC installed, or environment points to it
    ساكن bool coroutineSupport();  // Configured C++ compiler supports coroutines
    ساكن bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    ساكن bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    ساكن bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ثابت string& filename);
    string filePath(FileLine* fl, ثابت string& modname, ثابت string& lastpath,
                    ثابت string& errmsg);
    string filePathLookedMsg(FileLine* fl, ثابت string& modname);
    V3LangCode fileLanguage(ثابت string& filename);
    ساكن bool fileStatNormal(ثابت string& filename);

    // METHODS (other OS)
    ساكن فراغ throwSigsegv();
};

//######################################################################

#endif  // guard
