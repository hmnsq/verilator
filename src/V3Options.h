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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3LangCode.h"

#شامل <list>
#شامل <map>
#شامل <set>
#شامل <string>
#شامل <vector>

class V3OptionsImp;
class File_سطر;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
عمومی:
    شمارش en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    شمارش en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت {
        ایستا ثابت نویسه* ثابت names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        بازگشت names[m_e];
    }
    bool isDefault() ثابت { بازگشت m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ثابت { بازگشت m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() ثابت { بازگشت m_e == OPT_TRUE; }
    bool isSetFalse() ثابت { بازگشت m_e == OPT_FALSE; }
    تهی setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(ثابت VOptionBool& lhs, ثابت VOptionBool& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VOptionBool& lhs, VOptionBool::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, ثابت VOptionBool& rhs) { بازگشت lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ثابت string m_filename;  // Filename
    ثابت string m_libname;  // Libname
عمومی:
    VFileLibName(ثابت string& filename, ثابت string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ثابت VFileLibName& rhs) = پیشفرض;
    string filename() ثابت { بازگشت m_filename; }
    string libname() ثابت { بازگشت m_libname; }
    bool operator==(ثابت VFileLibName& rhs) ثابت {
        بازگشت m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(ثابت VFileLibName& rhs) ثابت {
        اگر (m_filename < rhs.m_filename) بازگشت true;
        اگر (m_filename > rhs.m_filename) بازگشت false;
        بازگشت m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
عمومی:
    شمارش en : uint8_t {
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
    شمارش : uint8_t { TS_DEFAULT = TS_1PS };
    شمارش en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ثابت string& مقدار, bool& badr);
    VTimescale(دوگانه مقدار, bool& badr) {
        badr = false;
        برای (صحیح i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            اگر (multiplier() == مقدار) بشکن;
        }
        اگر (multiplier() != مقدار) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ثابت { بازگشت m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ایستا تهی parseSlashed(File_سطر* fl, ثابت نویسه* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    ثابت نویسه* ascii() ثابت {
        ایستا ثابت نویسه* ثابت names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        بازگشت names[m_e];
    }
    صحیح powerOfTen() ثابت { بازگشت 2 - static_cast<صحیح>(m_e); }
    دوگانه multiplier() ثابت {
        ایستا ثابت دوگانه مقادیر[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        بازگشت مقادیر[m_e];
    }
};
constexpr bool operator==(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VTimescale& lhs, VTimescale::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, ثابت VTimescale& rhs) { بازگشت lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    بازگشت lhs.m_e > rhs.m_e;
}
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VTimescale& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
عمومی:
    شمارش en : uint8_t {
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
    explicit VFsmExpandType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    constexpr ثابت نویسه* ascii() ثابت {
        constexpr ثابت نویسه* ثابت names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        بازگشت names[m_e];
    }
};
constexpr bool operator==(ثابت VFsmExpandType& lhs, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    بازگشت lhs == rhs.m_e;
}
درونخطی std::ostream& operator<<(std::ostream& os, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    بازگشت os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
عمومی:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ثابت std::string, std::string>;

خصوصی:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

عمومی:
    explicit V3HierarchicalBlockOption(ثابت string& optstring);
    ثابت string& origName() ثابت { بازگشت m_origName; }
    ثابت string& mangledName() ثابت { بازگشت m_mangledName; }
    ثابت ParamStrMap params() ثابت { بازگشت m_parameters; }
};

using V3HierBlockOptSet = std::map<ثابت std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
عمومی:
خصوصی:
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
    bool m_preproc_No_سطر = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_دودویی = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_عبارت = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_سطر = false;    // main switch: --coverage-block
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
    bool m_debug_پشته_Check = false;  // main switch: --debug-stack-check
    صحیح m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_no_Trace_بالاترین = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    صحیح         m_buildJobs = -1;    // main switch: --build-jobs, -j
    صحیح         m_coverage_عبارت_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    صحیح         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    صحیح         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    صحیح         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    صحیح         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    صحیح         m_expandLimit = 256;  // main switch: --expand-limit
    صحیح         m_gateStmts = 100;    // main switch: --gate-stmts
    صحیح         m_hierChild = 0;      // main switch: --hierarchical-child
    صحیح         m_hierThreads = 0;      // main switch: --hierarchical-threads
    صحیح         m_ifDepth = 0;      // main switch: --if-depth
    صحیح         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    صحیح         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    صحیح         m_inlineMult = 2000;   // main switch: --inline-mult
    صحیح         m_instr_شمار_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    صحیح         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    صحیح         m_max_رقم_Width = 65536;  // main switch: --max-num-width
    صحیح         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    صحیح         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    صحیح         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    صحیح         m_outputSplit = 20000;  // main switch: --output-split
    صحیح         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    صحیح         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    صحیح         m_pinsBv = 65;       // main switch: --pins-bv
    صحیح         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    صحیح         m_publicDepth = 0;   // main switch: --public-depth
    صحیح         m_reloopLimit = 40; // main switch: --reloop-limit
    صحیح         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    صحیح         m_threads = 1;      // main switch: --threads
    صحیح         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    صحیح         m_traceDepth = 0;   // main switch: --trace-depth
    صحیح         m_traceMaxArray = 32;  // main switch: --trace-max-array
    صحیح         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    صحیح         m_unroll_شمار = 64;  // main switch: --unroll-count
    صحیح         m_unrollLimit = 16384;  // main switch: --unroll-limit
    صحیح         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    صحیح         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    صحیح         m_verilateJobs = -1;  // main switch: --verilate-jobs

    صحیح         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    صحیح         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    صحیح         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_بالاترین_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_کلید;   // main switch: --protect-key
    string      m_بالاترین_Module;    // main switch: --top-module
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
    bool m_f_Case_درخت;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_درخت;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_افزودن_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    صحیح  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    تهی addArg(نویسه** argv, size_t شمار, bool isForRerun);
    تهی addArg(ثابت std::string& arg, bool isForRerun);
    تهی add_سطر_Arg(ثابت string& arg);
    تهی addDefine(ثابت string& defline, bool allowPlus) VL_MT_DISABLED;
    تهی addFuture(ثابت string& flag);
    تهی addFuture0(ثابت string& flag);
    تهی addFuture1(ثابت string& flag);
    تهی addIncDirUser(ثابت string& incdir);  // User requested
    تهی addIncDirFallback(ثابت string& incdir);  // Low priority if not found otherwise
    تهی addParameter(ثابت string& paramline, bool allowPlus);
    تهی addLangExt(ثابت string& langext, ثابت V3LangCode& lc);
    تهی addLibExtV(ثابت string& libext);
    تهی optimize(صحیح level);
    تهی showVersion(bool verbose);
    تهی coverage(bool flag) {
        m_coverage_سطر = m_coverageBranch = m_coverageToggle = m_coverage_عبارت = m_coverageFsm
            = m_coverageUser = flag;
    }
    ایستا bool suffixed(ثابت string& sw, ثابت نویسه* arg);
    ایستا string parseFileArg(ثابت string& optdir, ثابت string& relfilename);
    string filePathCheckOneDir(ثابت string& modname, ثابت string& dirname);
    ایستا صحیح stripOptionsForChildRun(ثابت string& opt, bool for_بالاترین);
    تهی validateIdentifier(File_سطر* fl, ثابت string& arg, ثابت string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

عمومی:
    V3Options();
    ~V3Options();
    تهی setDebugMode(صحیح level);
    unsigned debugLevel(ثابت string& tag) ثابت VL_MT_SAFE;
    unsigned debugSrcLevel(ثابت string& srcfile_path) ثابت VL_MT_SAFE;
    unsigned dumpLevel(ثابت string& tag) ثابت VL_MT_SAFE;
    unsigned dumpSrcLevel(ثابت string& srcfile_path) ثابت VL_MT_SAFE;

    // METHODS
    تهی addCppFile(ثابت string& filename);
    تهی addCFlags(ثابت string& filename);
    تهی addCompilerIncludes(ثابت string& filename);
    تهی addLdLibFile(ثابت string& filename);
    تهی addLdLibs(ثابت string& filename);
    تهی addMakeFlags(ثابت string& filename);
    تهی addLibMapFile(ثابت string& filename);
    تهی addLibraryFile(ثابت string& filename, ثابت string& libname);
    تهی addVFile(ثابت string& filename, ثابت string& libname);
    تهی addVltFile(ثابت string& filename, ثابت string& libname);
    تهی addForceInc(ثابت string& filename);
    bool available() ثابت VL_MT_SAFE { بازگشت m_available; }
    تهی ccSet();
    تهی decorations(File_سطر* fl, ثابت string& filename);
    تهی notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ثابت { بازگشت m_preprocOnly; }
    bool makePhony() ثابت { بازگشت m_makePhony; }
    bool preproc_No_سطر() ثابت { بازگشت m_preproc_No_سطر; }
    bool preprocResolve() ثابت { بازگشت m_preprocResolve; }
    صحیح preprocTokenLimit() ثابت { بازگشت m_preprocTokenLimit; }
    bool underlineZero() ثابت { بازگشت m_underlineZero; }
    bool systemC() ثابت VL_MT_SAFE { بازگشت m_systemC; }
    bool savable() ثابت VL_MT_SAFE { بازگشت m_savable; }
    VOptionBool schedZeroDelay() ثابت { بازگشت m_schedZeroDelay; }
    bool stats() ثابت { بازگشت m_stats; }
    bool statsVars() ثابت { بازگشت m_statsVars; }
    bool stdPackage() ثابت { بازگشت m_stdPackage; }
    bool stdWaiver() ثابت { بازگشت m_stdWaiver; }
    bool assertOn() ثابت { بازگشت m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ثابت { بازگشت m_assertCase; }
    bool autoflush() ثابت { بازگشت m_autoflush; }
    bool bboxSys() ثابت { بازگشت m_bboxSys; }
    bool bboxUnsup() ثابت { بازگشت m_bboxUnsup; }
    bool build() ثابت { بازگشت m_build; }
    string buildDepBin() ثابت { بازگشت m_buildDepBin; }
    تهی buildDepBin(ثابت string& flag) { m_buildDepBin = flag; }
    bool context() ثابت VL_MT_SAFE { بازگشت m_context; }
    bool coverage() ثابت VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        بازگشت m_coverage_سطر || m_coverageBranch || m_coverageToggle || m_coverage_عبارت
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ثابت VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        بازگشت m_coverage_سطر || m_coverageBranch || m_coverageToggle || m_coverage_عبارت
               || m_coverageUser;
    }
    bool coverageBranch() ثابت { بازگشت m_coverageBranch; }
    bool coverage_عبارت() ثابت { بازگشت m_coverage_عبارت; }
    bool coverageFsm() ثابت { بازگشت m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ثابت { بازگشت m_coverageFsmExpand; }
    صحیح coverageFsmMaxArcs() ثابت { بازگشت m_coverageFsmMaxArcs; }
    bool coverage_سطر() ثابت { بازگشت m_coverage_سطر; }
    bool coverageMergeInstances() ثابت { بازگشت m_coverageMergeInstances; }
    bool coveragePerInstance() ثابت { بازگشت m_coveragePerInstance; }
    bool coverageToggle() ثابت { بازگشت m_coverageToggle; }
    bool coverageUnderscore() ثابت { بازگشت m_coverageUnderscore; }
    bool coverageUser() ثابت { بازگشت m_coverageUser; }
    bool debugCheck() ثابت VL_MT_SAFE { بازگشت m_debugCheck; }
    bool debugCollision() ثابت { بازگشت m_debugCollision; }
    bool debugEmitV() ثابت VL_MT_SAFE { بازگشت m_debugEmitV; }
    bool debugExitElab() ثابت { بازگشت m_debugExitElab; }
    bool debugExitParse() ثابت { بازگشت m_debugExitParse; }
    bool debugLeak() ثابت { بازگشت m_debugLeak; }
    bool debugNondeterminism() ثابت { بازگشت m_debugNondeterminism; }
    bool debugPartition() ثابت { بازگشت m_debugPartition; }
    bool debugPreprocPassthru() ثابت VL_MT_SAFE { بازگشت m_debugPreprocPassthru; }
    bool debugProtect() ثابت VL_MT_SAFE { بازگشت m_debugProtect; }
    صحیح debugRuntimeTimeout() ثابت { بازگشت m_debugRuntimeTimeout; }
    bool debugSelfTest() ثابت { بازگشت m_debugSelfTest; }
    bool debug_پشته_Check() ثابت { بازگشت m_debug_پشته_Check; }
    bool debugWidth() ثابت VL_PURE { بازگشت m_debugWidth; }
    bool decoration() ثابت VL_MT_SAFE { بازگشت m_decoration; }
    bool decorationNodes() ثابت VL_MT_SAFE { بازگشت m_decorationNodes; }
    bool diagnosticsSarif() ثابت VL_MT_SAFE { بازگشت m_diagnosticsSarif; }
    bool dpiHdrOnly() ثابت { بازگشت m_dpiHdrOnly; }
    bool dumpAstPatterns() ثابت {
        بازگشت m_dumpLevel.شمار("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() ثابت { بازگشت m_dumpLevel.شمار("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ثابت {
        بازگشت m_dumpLevel.شمار("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_درخت_Dot() ثابت {
        بازگشت m_dumpLevel.شمار("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ثابت { بازگشت m_emitAccessors; }
    bool exe() ثابت { بازگشت m_exe; }
    bool flatten() ثابت { بازگشت m_flatten; }
    bool fourstate() ثابت { بازگشت m_fourstate; }
    bool gmake() ثابت { بازگشت m_gmake; }
    bool makeJson() ثابت { بازگشت m_makeJson; }
    bool threadsDpiPure() ثابت { بازگشت m_threadsDpiPure; }
    bool threadsDpiUnpure() ثابت { بازگشت m_threadsDpiUnpure; }
    bool threadsCoarsen() ثابت { بازگشت m_threadsCoarsen; }
    VOptionBool timing() ثابت { بازگشت m_timing; }
    bool trace() ثابت { بازگشت m_trace; }
    bool traceCoverage() ثابت { بازگشت m_traceCoverage; }
    bool traceEnabledFst() ثابت { بازگشت m_traceEnabledFst; }
    bool traceEnabledSaif() ثابت { بازگشت m_traceEnabledSaif; }
    bool traceEnabledVcd() ثابت { بازگشت m_traceEnabledVcd; }
    bool traceParams() ثابت { بازگشت m_traceParams; }
    bool traceStructs() ثابت { بازگشت m_traceStructs; }
    bool traceUnderscore() ثابت { بازگشت m_traceUnderscore; }
    bool اصلی() ثابت { بازگشت m_main; }
    bool outFormatOk() ثابت { بازگشت m_outFormatOk; }
    bool jsonOnly() ثابت { بازگشت m_jsonOnly; }
    bool keepTempFiles() ثابت { بازگشت (V3Error::debugDefault() != 0); }
    bool pedantic() ثابت { بازگشت m_pedantic; }
    bool pinsInoutEnables() ثابت { بازگشت m_pinsInoutEnables; }
    bool pinsScUint() ثابت { بازگشت m_pinsScUint; }
    bool pinsScUintBool() ثابت { بازگشت m_pinsScUintBool; }
    bool pinsScBigUint() ثابت VL_MT_SAFE { بازگشت m_pinsScBigUint; }
    bool pinsUint8() ثابت { بازگشت m_pinsUint8; }
    bool preprocComments() ثابت { بازگشت m_preprocComments; }
    bool preprocDefines() ثابت { بازگشت m_preprocDefines; }
    bool profC() ثابت { بازگشت m_profC; }
    bool profCFuncs() ثابت { بازگشت m_profCFuncs; }
    bool profExec() ثابت { بازگشت m_profExec; }
    bool profPgo() ثابت { بازگشت m_profPgo; }
    bool usesProfiler() ثابت { بازگشت profExec() || profPgo(); }
    bool protectIds() ثابت VL_MT_SAFE { بازگشت m_protectIds; }
    bool allPublic() ثابت { بازگشت m_public; }
    bool publicParams() ثابت { بازگشت m_publicParams; }
    bool publicOff() ثابت { بازگشت m_publicIgnore; }
    bool publicFlatRW() ثابت { بازگشت m_publicFlatRW; }
    صحیح publicDepth() ثابت { بازگشت m_publicDepth; }
    bool anyPublicFlat() ثابت { بازگشت m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() ثابت VL_MT_SAFE { بازگشت m_lintOnly; }
    bool ignc() ثابت { بازگشت m_ignc; }
    bool quietBuild() ثابت VL_MT_SAFE { بازگشت m_quietBuild; }
    bool quietExit() ثابت VL_MT_SAFE { بازگشت m_quietExit; }
    bool quietStats() ثابت VL_MT_SAFE { بازگشت m_quietStats; }
    bool reportUnoptflat() ثابت { بازگشت m_reportUnoptflat; }
    bool verilate() ثابت { بازگشت m_verilate; }
    bool vpi() ثابت { بازگشت m_vpi.isTrue(); }
    bool waiverMultiline() ثابت { بازگشت m_waiverMultiline; }
    bool xInitialEdge() ثابت { بازگشت m_xInitialEdge; }
    bool serializeOnly() ثابت { بازگشت m_jsonOnly; }
    bool بالاترین_Ifaces_Supported() ثابت { بازگشت lintOnly() && !hierarchical(); }

    صحیح buildJobs() ثابت VL_MT_SAFE { بازگشت m_buildJobs; }
    صحیح convergeLimit() ثابت { بازگشت m_convergeLimit; }
    صحیح coverage_عبارت_Max() ثابت { بازگشت m_coverage_عبارت_Max; }
    uint32_t coverageMaxBins() ثابت { بازگشت m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ثابت { بازگشت m_coverageMaxRealBins; }
    صحیح coverageMaxWidth() ثابت { بازگشت m_coverageMaxWidth; }
    صحیح debugAllocRandom() ثابت { بازگشت m_debugAllocRandom; }
    bool dump_درخت_Addrids() ثابت VL_MT_SAFE;
    صحیح expandLimit() ثابت { بازگشت m_expandLimit; }
    صحیح gateStmts() ثابت { بازگشت m_gateStmts; }
    صحیح ifDepth() ثابت { بازگشت m_ifDepth; }
    صحیح inlineCFuncs() ثابت { بازگشت m_inlineCFuncs; }
    صحیح inlineCFuncsProduct() ثابت { بازگشت m_inlineCFuncsProduct; }
    صحیح inlineMult() ثابت { بازگشت m_inlineMult; }
    صحیح instr_شمار_Dpi() ثابت { بازگشت m_instr_شمار_Dpi; }
    صحیح localizeMaxSize() ثابت { بازگشت m_localizeMaxSize; }
    bool jsonEditNums() ثابت { بازگشت m_jsonEditNums; }
    bool jsonIds() ثابت { بازگشت m_jsonIds; }
    VOptionBool makeDepend() ثابت { بازگشت m_makeDepend; }
    صحیح max_رقم_Width() ثابت { بازگشت m_max_رقم_Width; }
    صحیح funcRecursionDepth() ثابت { بازگشت m_funcRecursion; }
    صحیح moduleRecursionDepth() ثابت { بازگشت m_moduleRecursion; }
    صحیح outputSplit() ثابت { بازگشت m_outputSplit; }
    صحیح outputSplitCFuncs() ثابت { بازگشت m_outputSplitCFuncs; }
    صحیح outputSplitCTrace() ثابت { بازگشت m_outputSplitCTrace; }
    صحیح outputGroups() ثابت { بازگشت m_outputGroups; }
    int64_t outputGroupsSize() ثابت { بازگشت m_outputGroupsSize; }
    صحیح pinsBv() ثابت VL_MT_SAFE { بازگشت m_pinsBv; }
    صحیح reloopLimit() ثابت { بازگشت m_reloopLimit; }
    صحیح replicationLimit() ثابت { بازگشت m_replicationLimit; }
    VOptionBool skipIdentical() ثابت { بازگشت m_skipIdentical; }
    bool stopFail() ثابت { بازگشت m_stopFail; }
    صحیح threads() ثابت VL_MT_SAFE { بازگشت m_threads; }
    صحیح threadsMaxMTasks() ثابت { بازگشت m_threadsMaxMTasks; }
    bool mtasks() ثابت VL_MT_SAFE { بازگشت (m_threads > 1); }
    VTimescale timeDefaultPrec() ثابت { بازگشت m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() ثابت { بازگشت m_timeDefaultUnit; }
    VTimescale timeOverridePrec() ثابت { بازگشت m_timeOverridePrec; }
    VTimescale timeOverrideUnit() ثابت { بازگشت m_timeOverrideUnit; }
    VTimescale timeComputePrec(ثابت VTimescale& flag) ثابت;
    VTimescale timeComputeUnit(ثابت VTimescale& flag) ثابت;
    صحیح traceDepth() ثابت { بازگشت m_traceDepth; }
    صحیح traceMaxArray() ثابت { بازگشت m_traceMaxArray; }
    صحیح traceMaxWidth() ثابت { بازگشت m_traceMaxWidth; }
    bool useTraceParallel() ثابت {
        بازگشت trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    صحیح unroll_شمار() ثابت { بازگشت m_unroll_شمار; }
    صحیح unrollLimit() ثابت { بازگشت m_unrollLimit; }
    صحیح unrollStmts() ثابت { بازگشت m_unrollStmts; }
    صحیح constraintArrayLimit() ثابت { بازگشت m_constraintArrayLimit; }
    صحیح verilateJobs() ثابت { بازگشت m_verilateJobs; }

    صحیح compLimitBlocks() ثابت { بازگشت m_compLimitBlocks; }
    صحیح compLimitMembers() ثابت VL_MT_SAFE { بازگشت m_compLimitMembers; }
    صحیح compLimitParens() ثابت { بازگشت m_compLimitParens; }

    string diagnosticsSarifOutput() ثابت VL_MT_SAFE {
        بازگشت m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ثابت { بازگشت m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() ثابت { بازگشت m_hierParamsFile; }
    string jsonOnlyOutput() ثابت { بازگشت m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ثابت { بازگشت m_jsonOnlyMetaOutput; }
    string l2Name() ثابت { بازگشت m_l2Name; }
    string libCreate() ثابت { بازگشت m_libCreate; }
    string libCreateName(bool shared) ثابت {
        string libName = "lib" + libCreate();
        اگر (shared) {
            libName += ".so";
        } وگرنه {
            libName += ".a";
        }
        بازگشت libName;
    }
    string main_بالاترین_Name() ثابت { بازگشت m_main_بالاترین_Name; }
    string makeDir() ثابت VL_MT_SAFE { بازگشت m_makeDir; }
    string modPrefix() ثابت VL_MT_SAFE { بازگشت m_modPrefix; }
    string pipeFilter() ثابت { بازگشت m_pipeFilter; }
    string prefix() ثابت VL_MT_SAFE { بازگشت m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_کلید_Provided() ثابت { بازگشت !m_protect_کلید.empty(); }
    string protect_کلید_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string بالاترین_Module() ثابت { بازگشت m_بالاترین_Module; }  // As AstNode::encodeName()
    bool no_Trace_بالاترین() ثابت { بازگشت m_no_Trace_بالاترین; }
    string unusedRegexp() ثابت { بازگشت m_unusedRegexp; }
    string waiverOutput() ثابت { بازگشت m_waiverOutput; }
    string work() ثابت { بازگشت m_work; }
    bool isWaiverOutput() ثابت { بازگشت !m_waiverOutput.empty(); }
    string xAssign() ثابت { بازگشت m_xAssign; }
    string xInitial() ثابت { بازگشت m_xInitial; }

    ثابت VStringSet& cppFiles() ثابت { بازگشت m_cppFiles; }
    ثابت VStringList& cFlags() ثابت { بازگشت m_cFlags; }
    ثابت VStringSet& compilerIncludes() ثابت { بازگشت m_compilerIncludes; }
    ثابت VStringList& ldLibFiles() ثابت { بازگشت m_ldLibFiles; }
    ثابت VStringList& ldLibs() ثابت { بازگشت m_ldLibs; }
    ثابت VStringList& makeFlags() ثابت { بازگشت m_makeFlags; }
    ثابت VFileLibSet& libraryFiles() ثابت { بازگشت m_libraryFiles; }
    ثابت VStringSet& libmapFiles() ثابت { بازگشت m_libmapFiles; }
    ثابت VFileLibList& vFiles() ثابت { بازگشت m_vFiles; }
    ثابت VFileLibSet& vltFiles() ثابت { بازگشت m_vltFiles; }
    ثابت VStringList& forceIncs() ثابت { بازگشت m_forceIncs; }

    bool hasParameter(ثابت string& name);
    string parameter(ثابت string& name);
    تهی checkParameters();

    bool isFuture(ثابت string& flag) ثابت;
    bool isFuture0(ثابت string& flag) ثابت;
    bool isFuture1(ثابت string& flag) ثابت;
    bool isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ثابت { بازگشت m_fAcycSimp; }
    bool fAssemble() ثابت { بازگشت m_fAssemble; }
    bool fBitScanLoops() ثابت { بازگشت m_fBitScanLoops; }
    bool fBitblast() ثابت { بازگشت m_fBitblast; }
    bool fCaseDecoder() ثابت { بازگشت m_fCaseDecoder; }
    bool fCaseTable() ثابت { بازگشت m_fCaseTable; }
    bool f_Case_درخت() ثابت { بازگشت m_f_Case_درخت; }
    bool fCombine() ثابت { بازگشت m_fCombine; }
    bool fConst() ثابت { بازگشت m_fConst; }
    bool fConstBeforeDfg() ثابت { بازگشت m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_درخت() ثابت { بازگشت m_f_Const_Bit_Op_درخت; }
    bool fConstEager() ثابت { بازگشت m_fConstEager; }
    bool fDedupe() ثابت { بازگشت m_fDedupe; }
    bool fDfg() ثابت { بازگشت m_fDfg; }
    bool fDfgPeephole() ثابت { بازگشت m_fDfgPeephole; }
    bool f_Dfg_افزودن_Down_Sels() ثابت { بازگشت m_f_Dfg_افزودن_Down_Sels; }
    bool fDfgSynthesizeAll() ثابت { بازگشت m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ثابت std::string& name) ثابت {
        بازگشت !m_fDfgPeepholeDisabled.شمار(name);
    }
    bool fDeadAssigns() ثابت { بازگشت m_fDeadAssigns; }
    bool fDeadCells() ثابت { بازگشت m_fDeadCells; }
    bool fDeadMethods() ثابت { بازگشت m_fDeadMethods; }
    bool fDecompose() ثابت { بازگشت m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ثابت { بازگشت m_fDecomposePacked; }
    bool fDecomposeUnpacked() ثابت { بازگشت m_fDecomposeUnpacked; }
    bool fExpand() ثابت { بازگشت m_fExpand; }
    bool fFuncBalanceCat() ثابت { بازگشت m_fFuncBalanceCat; }
    bool fFuncSplitCat() ثابت { بازگشت m_fFuncSplitCat; }
    bool fFunc() ثابت { بازگشت fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() ثابت { بازگشت m_fGate; }
    VOptionBool fIcoChangeDetect() ثابت { بازگشت m_fIcoChangeDetect; }
    bool fInline() ثابت { بازگشت m_fInline; }
    bool fInlineCFuncs() ثابت { بازگشت m_fInlineCFuncs; }
    bool fInlineFuncs() ثابت { بازگشت m_fInlineFuncs; }
    bool fInlineFuncsEager() ثابت { بازگشت m_fInlineFuncsEager; }
    bool fLife() ثابت { بازگشت m_fLife; }
    bool fLifePost() ثابت { بازگشت m_fLifePost; }
    bool fLocalize() ثابت { بازگشت m_fLocalize; }
    bool fMergeCond() ثابت { بازگشت m_fMergeCond; }
    bool fMergeCondMotion() ثابت { بازگشت m_fMergeCondMotion; }
    bool fReloop() ثابت { بازگشت m_fReloop; }
    bool fReorder() ثابت { بازگشت m_fReorder; }
    bool fSlice() ثابت { بازگشت m_fSlice; }
    صحیح fSliceElementLimit() ثابت { بازگشت m_fSliceElementLimit; }
    bool fSplit() ثابت { بازگشت m_fSplit; }
    bool fSubst() ثابت { بازگشت m_fSubst; }
    bool fSubstConst() ثابت { بازگشت m_fSubstConst; }
    bool fTable() ثابت { بازگشت m_fTable; }
    bool fTaskifyAll() ثابت { بازگشت m_fTaskifyAll; }

    std::string traceClassBase() ثابت VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() ثابت VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() ثابت VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() ثابت VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() ثابت VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() ثابت VL_MT_SAFE;

    bool hierarchical() ثابت { بازگشت m_hierarchical; }
    صحیح hierChild() ثابت VL_MT_SAFE { بازگشت m_hierChild; }
    صحیح hierThreads() ثابت VL_MT_SAFE { بازگشت m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_بالاترین() ثابت VL_MT_SAFE { بازگشت !m_hierChild && !m_hierBlocks.empty(); }
    ثابت V3HierBlockOptSet& hierBlocks() ثابت { بازگشت m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_بالاترین_Data_Dir() ثابت VL_MT_SAFE {
        بازگشت hier_بالاترین() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ایستا string version() VL_PURE;
    ایستا string argString(صحیح argc, نویسه** argv);  ///< Return list of arguments as simple string
    string allArgsString() ثابت VL_MT_SAFE;  ///< Return all passed arguments as simple string
    ثابت std::list<std::pair<std::list<std::string>, bool>>& allArgs() ثابت;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_بالاترین) ثابت;
    تهی parseOpts(File_سطر* fl, صحیح argc, نویسه** argv) VL_MT_DISABLED;
    تهی parseOptsList(File_سطر* fl, ثابت string& optdir, صحیح argc, نویسه** argv) VL_MT_DISABLED;
    تهی parseOptsFile(File_سطر* fl, ثابت string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    ایستا string getenvBuiltins(ثابت string& var);
    ایستا string getenvMAKE();
    ایستا string getenvMAKEFLAGS();
    ایستا string getenvPERL();
    ایستا string getenvPYTHON3();
    ایستا string getenvSYSTEMC();
    ایستا string getenvSYSTEMC_ARCH();
    ایستا string getenvSYSTEMC_INCLUDE();
    ایستا string getenvSYSTEMC_LIBDIR();
    ایستا string getenv_VERILATOR_ریشه();
    ایستا string getenvVERILATOR_SOLVER();
    ایستا string getStdPackagePath();
    ایستا string getStdWaiverPath();
    ایستا string getSupported(ثابت string& var);
    ایستا bool systemCSystemWide();
    ایستا bool systemCFound();  // SystemC installed, or environment points to it
    ایستا bool coroutineSupport();  // Configured C++ compiler supports coroutines
    ایستا bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    ایستا bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    ایستا bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ثابت string& filename);
    string filePath(File_سطر* fl, ثابت string& modname, ثابت string& lastpath,
                    ثابت string& errmsg);
    string file_Path_Looked_پیام(File_سطر* fl, ثابت string& modname);
    V3LangCode fileLanguage(ثابت string& filename);
    ایستا bool fileStatNormal(ثابت string& filename);

    // METHODS (other OS)
    ایستا تهی throwSigsegv();
};

//######################################################################

#endif  // guard
