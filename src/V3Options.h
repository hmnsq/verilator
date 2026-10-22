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
class File_سطر;

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
    constexpr operator en() ثابت { إرجاع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        إرجاع names[m_e];
    }
    bool isDefault() ثابت { إرجاع m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ثابت { إرجاع m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() ثابت { إرجاع m_e == OPT_TRUE; }
    bool isSetFalse() ثابت { إرجاع m_e == OPT_FALSE; }
    فراغ setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(ثابت VOptionBool& lhs, ثابت VOptionBool& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VOptionBool& lhs, VOptionBool::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, ثابت VOptionBool& rhs) { إرجاع lhs == rhs.m_e; }

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
    string filename() ثابت { إرجاع m_filename; }
    string libname() ثابت { إرجاع m_libname; }
    bool operator==(ثابت VFileLibName& rhs) ثابت {
        إرجاع m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(ثابت VFileLibName& rhs) ثابت {
        إذا (m_filename < rhs.m_filename) إرجاع true;
        إذا (m_filename > rhs.m_filename) إرجاع false;
        إرجاع m_libname < rhs.m_libname;
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
    VTimescale(ثابت string& قيمة, bool& badr);
    VTimescale(مزدوج قيمة, bool& badr) {
        badr = false;
        لكل (صحيح i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            إذا (multiplier() == قيمة) اكسر;
        }
        إذا (multiplier() != قيمة) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ثابت { إرجاع m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ساكن فراغ parseSlashed(File_سطر* fl, ثابت حرف* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        إرجاع names[m_e];
    }
    صحيح powerOfTen() ثابت { إرجاع 2 - static_cast<صحيح>(m_e); }
    مزدوج multiplier() ثابت {
        ساكن ثابت مزدوج قيم[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        إرجاع قيم[m_e];
    }
};
constexpr bool operator==(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VTimescale& lhs, VTimescale::en rhs) { إرجاع lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, ثابت VTimescale& rhs) { إرجاع lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    إرجاع lhs.m_e > rhs.m_e;
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VTimescale& rhs) {
    إرجاع os << rhs.ascii();
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
    constexpr operator en() ثابت { إرجاع m_e; }
    constexpr ثابت حرف* ascii() ثابت {
        constexpr ثابت حرف* ثابت names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        إرجاع names[m_e];
    }
};
constexpr bool operator==(ثابت VFsmExpandType& lhs, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    إرجاع lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    إرجاع lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    إرجاع lhs == rhs.m_e;
}
مضمن std::ostream& operator<<(std::ostream& os, ثابت VFsmExpandType& rhs) VL_MT_SAFE {
    إرجاع os << rhs.ascii();
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
    ثابت string& origName() ثابت { إرجاع m_origName; }
    ثابت string& mangledName() ثابت { إرجاع m_mangledName; }
    ثابت ParamStrMap params() ثابت { إرجاع m_parameters; }
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
    bool m_preproc_No_سطر = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_ثنائي = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_تعبير = false;    // main switch: --coverage-expr
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
    bool m_debug_مكدس_Check = false;  // main switch: --debug-stack-check
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
    bool m_no_Trace_قمة = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    صحيح         m_buildJobs = -1;    // main switch: --build-jobs, -j
    صحيح         m_coverage_تعبير_Max = 32;    // main switch: --coverage-expr-max
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
    صحيح         m_instr_تعداد_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    صحيح         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    صحيح         m_max_رقم_Width = 65536;  // main switch: --max-num-width
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
    صحيح         m_unroll_تعداد = 64;  // main switch: --unroll-count
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
    string      m_main_قمة_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_مفتاح;   // main switch: --protect-key
    string      m_قمة_Module;    // main switch: --top-module
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
    bool m_f_Case_شجرة;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_شجرة;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ادفع_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    فراغ addArg(حرف** argv, size_t تعداد_2, bool isForRerun);
    فراغ addArg(ثابت std::string& arg, bool isForRerun);
    فراغ add_سطر_Arg(ثابت string& arg);
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
        m_coverage_سطر = m_coverageBranch = m_coverageToggle = m_coverage_تعبير = m_coverageFsm
            = m_coverageUser = flag;
    }
    ساكن bool suffixed(ثابت string& sw, ثابت حرف* arg);
    ساكن string parseFileArg(ثابت string& optdir, ثابت string& relfilename);
    string filePathCheckOneDir(ثابت string& modname, ثابت string& dirname);
    ساكن صحيح stripOptionsForChildRun(ثابت string& opt, bool for_قمة);
    فراغ validateIdentifier(File_سطر* fl, ثابت string& arg, ثابت string& opt);

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
    bool available() ثابت VL_MT_SAFE { إرجاع m_available; }
    فراغ ccSet();
    فراغ decorations(File_سطر* fl, ثابت string& filename);
    فراغ notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ثابت { إرجاع m_preprocOnly; }
    bool makePhony() ثابت { إرجاع m_makePhony; }
    bool preproc_No_سطر() ثابت { إرجاع m_preproc_No_سطر; }
    bool preprocResolve() ثابت { إرجاع m_preprocResolve; }
    صحيح preprocTokenLimit() ثابت { إرجاع m_preprocTokenLimit; }
    bool underlineZero() ثابت { إرجاع m_underlineZero; }
    bool systemC() ثابت VL_MT_SAFE { إرجاع m_systemC; }
    bool savable() ثابت VL_MT_SAFE { إرجاع m_savable; }
    VOptionBool schedZeroDelay() ثابت { إرجاع m_schedZeroDelay; }
    bool stats() ثابت { إرجاع m_stats; }
    bool statsVars() ثابت { إرجاع m_statsVars; }
    bool stdPackage() ثابت { إرجاع m_stdPackage; }
    bool stdWaiver() ثابت { إرجاع m_stdWaiver; }
    bool assertOn() ثابت { إرجاع m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ثابت { إرجاع m_assertCase; }
    bool autoflush() ثابت { إرجاع m_autoflush; }
    bool bboxSys() ثابت { إرجاع m_bboxSys; }
    bool bboxUnsup() ثابت { إرجاع m_bboxUnsup; }
    bool build() ثابت { إرجاع m_build; }
    string buildDepBin() ثابت { إرجاع m_buildDepBin; }
    فراغ buildDepBin(ثابت string& flag) { m_buildDepBin = flag; }
    bool context() ثابت VL_MT_SAFE { إرجاع m_context; }
    bool coverage() ثابت VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        إرجاع m_coverage_سطر || m_coverageBranch || m_coverageToggle || m_coverage_تعبير
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ثابت VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        إرجاع m_coverage_سطر || m_coverageBranch || m_coverageToggle || m_coverage_تعبير
               || m_coverageUser;
    }
    bool coverageBranch() ثابت { إرجاع m_coverageBranch; }
    bool coverage_تعبير() ثابت { إرجاع m_coverage_تعبير; }
    bool coverageFsm() ثابت { إرجاع m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ثابت { إرجاع m_coverageFsmExpand; }
    صحيح coverageFsmMaxArcs() ثابت { إرجاع m_coverageFsmMaxArcs; }
    bool coverage_سطر() ثابت { إرجاع m_coverage_سطر; }
    bool coverageMergeInstances() ثابت { إرجاع m_coverageMergeInstances; }
    bool coveragePerInstance() ثابت { إرجاع m_coveragePerInstance; }
    bool coverageToggle() ثابت { إرجاع m_coverageToggle; }
    bool coverageUnderscore() ثابت { إرجاع m_coverageUnderscore; }
    bool coverageUser() ثابت { إرجاع m_coverageUser; }
    bool debugCheck() ثابت VL_MT_SAFE { إرجاع m_debugCheck; }
    bool debugCollision() ثابت { إرجاع m_debugCollision; }
    bool debugEmitV() ثابت VL_MT_SAFE { إرجاع m_debugEmitV; }
    bool debugExitElab() ثابت { إرجاع m_debugExitElab; }
    bool debugExitParse() ثابت { إرجاع m_debugExitParse; }
    bool debugLeak() ثابت { إرجاع m_debugLeak; }
    bool debugNondeterminism() ثابت { إرجاع m_debugNondeterminism; }
    bool debugPartition() ثابت { إرجاع m_debugPartition; }
    bool debugPreprocPassthru() ثابت VL_MT_SAFE { إرجاع m_debugPreprocPassthru; }
    bool debugProtect() ثابت VL_MT_SAFE { إرجاع m_debugProtect; }
    صحيح debugRuntimeTimeout() ثابت { إرجاع m_debugRuntimeTimeout; }
    bool debugSelfTest() ثابت { إرجاع m_debugSelfTest; }
    bool debug_مكدس_Check() ثابت { إرجاع m_debug_مكدس_Check; }
    bool debugWidth() ثابت VL_PURE { إرجاع m_debugWidth; }
    bool decoration() ثابت VL_MT_SAFE { إرجاع m_decoration; }
    bool decorationNodes() ثابت VL_MT_SAFE { إرجاع m_decorationNodes; }
    bool diagnosticsSarif() ثابت VL_MT_SAFE { إرجاع m_diagnosticsSarif; }
    bool dpiHdrOnly() ثابت { إرجاع m_dpiHdrOnly; }
    bool dumpAstPatterns() ثابت {
        إرجاع m_dumpLevel.تعداد_2("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() ثابت { إرجاع m_dumpLevel.تعداد_2("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ثابت {
        إرجاع m_dumpLevel.تعداد_2("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_شجرة_Dot() ثابت {
        إرجاع m_dumpLevel.تعداد_2("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ثابت { إرجاع m_emitAccessors; }
    bool exe() ثابت { إرجاع m_exe; }
    bool flatten() ثابت { إرجاع m_flatten; }
    bool fourstate() ثابت { إرجاع m_fourstate; }
    bool gmake() ثابت { إرجاع m_gmake; }
    bool makeJson() ثابت { إرجاع m_makeJson; }
    bool threadsDpiPure() ثابت { إرجاع m_threadsDpiPure; }
    bool threadsDpiUnpure() ثابت { إرجاع m_threadsDpiUnpure; }
    bool threadsCoarsen() ثابت { إرجاع m_threadsCoarsen; }
    VOptionBool timing() ثابت { إرجاع m_timing; }
    bool trace() ثابت { إرجاع m_trace; }
    bool traceCoverage() ثابت { إرجاع m_traceCoverage; }
    bool traceEnabledFst() ثابت { إرجاع m_traceEnabledFst; }
    bool traceEnabledSaif() ثابت { إرجاع m_traceEnabledSaif; }
    bool traceEnabledVcd() ثابت { إرجاع m_traceEnabledVcd; }
    bool traceParams() ثابت { إرجاع m_traceParams; }
    bool traceStructs() ثابت { إرجاع m_traceStructs; }
    bool traceUnderscore() ثابت { إرجاع m_traceUnderscore; }
    bool الرئيسية() ثابت { إرجاع m_main; }
    bool outFormatOk() ثابت { إرجاع m_outFormatOk; }
    bool jsonOnly() ثابت { إرجاع m_jsonOnly; }
    bool keepTempFiles() ثابت { إرجاع (V3Error::debugDefault() != 0); }
    bool pedantic() ثابت { إرجاع m_pedantic; }
    bool pinsInoutEnables() ثابت { إرجاع m_pinsInoutEnables; }
    bool pinsScUint() ثابت { إرجاع m_pinsScUint; }
    bool pinsScUintBool() ثابت { إرجاع m_pinsScUintBool; }
    bool pinsScBigUint() ثابت VL_MT_SAFE { إرجاع m_pinsScBigUint; }
    bool pinsUint8() ثابت { إرجاع m_pinsUint8; }
    bool preprocComments() ثابت { إرجاع m_preprocComments; }
    bool preprocDefines() ثابت { إرجاع m_preprocDefines; }
    bool profC() ثابت { إرجاع m_profC; }
    bool profCFuncs() ثابت { إرجاع m_profCFuncs; }
    bool profExec() ثابت { إرجاع m_profExec; }
    bool profPgo() ثابت { إرجاع m_profPgo; }
    bool usesProfiler() ثابت { إرجاع profExec() || profPgo(); }
    bool protectIds() ثابت VL_MT_SAFE { إرجاع m_protectIds; }
    bool allPublic() ثابت { إرجاع m_public; }
    bool publicParams() ثابت { إرجاع m_publicParams; }
    bool publicOff() ثابت { إرجاع m_publicIgnore; }
    bool publicFlatRW() ثابت { إرجاع m_publicFlatRW; }
    صحيح publicDepth() ثابت { إرجاع m_publicDepth; }
    bool anyPublicFlat() ثابت { إرجاع m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() ثابت VL_MT_SAFE { إرجاع m_lintOnly; }
    bool ignc() ثابت { إرجاع m_ignc; }
    bool quietBuild() ثابت VL_MT_SAFE { إرجاع m_quietBuild; }
    bool quietExit() ثابت VL_MT_SAFE { إرجاع m_quietExit; }
    bool quietStats() ثابت VL_MT_SAFE { إرجاع m_quietStats; }
    bool reportUnoptflat() ثابت { إرجاع m_reportUnoptflat; }
    bool verilate() ثابت { إرجاع m_verilate; }
    bool vpi() ثابت { إرجاع m_vpi.isTrue(); }
    bool waiverMultiline() ثابت { إرجاع m_waiverMultiline; }
    bool xInitialEdge() ثابت { إرجاع m_xInitialEdge; }
    bool serializeOnly() ثابت { إرجاع m_jsonOnly; }
    bool قمة_Ifaces_Supported() ثابت { إرجاع lintOnly() && !hierarchical(); }

    صحيح buildJobs() ثابت VL_MT_SAFE { إرجاع m_buildJobs; }
    صحيح convergeLimit() ثابت { إرجاع m_convergeLimit; }
    صحيح coverage_تعبير_Max() ثابت { إرجاع m_coverage_تعبير_Max; }
    uint32_t coverageMaxBins() ثابت { إرجاع m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ثابت { إرجاع m_coverageMaxRealBins; }
    صحيح coverageMaxWidth() ثابت { إرجاع m_coverageMaxWidth; }
    صحيح debugAllocRandom() ثابت { إرجاع m_debugAllocRandom; }
    bool dump_شجرة_Addrids() ثابت VL_MT_SAFE;
    صحيح expandLimit() ثابت { إرجاع m_expandLimit; }
    صحيح gateStmts() ثابت { إرجاع m_gateStmts; }
    صحيح ifDepth() ثابت { إرجاع m_ifDepth; }
    صحيح inlineCFuncs() ثابت { إرجاع m_inlineCFuncs; }
    صحيح inlineCFuncsProduct() ثابت { إرجاع m_inlineCFuncsProduct; }
    صحيح inlineMult() ثابت { إرجاع m_inlineMult; }
    صحيح instr_تعداد_Dpi() ثابت { إرجاع m_instr_تعداد_Dpi; }
    صحيح localizeMaxSize() ثابت { إرجاع m_localizeMaxSize; }
    bool jsonEditNums() ثابت { إرجاع m_jsonEditNums; }
    bool jsonIds() ثابت { إرجاع m_jsonIds; }
    VOptionBool makeDepend() ثابت { إرجاع m_makeDepend; }
    صحيح max_رقم_Width() ثابت { إرجاع m_max_رقم_Width; }
    صحيح funcRecursionDepth() ثابت { إرجاع m_funcRecursion; }
    صحيح moduleRecursionDepth() ثابت { إرجاع m_moduleRecursion; }
    صحيح outputSplit() ثابت { إرجاع m_outputSplit; }
    صحيح outputSplitCFuncs() ثابت { إرجاع m_outputSplitCFuncs; }
    صحيح outputSplitCTrace() ثابت { إرجاع m_outputSplitCTrace; }
    صحيح outputGroups() ثابت { إرجاع m_outputGroups; }
    int64_t outputGroupsSize() ثابت { إرجاع m_outputGroupsSize; }
    صحيح pinsBv() ثابت VL_MT_SAFE { إرجاع m_pinsBv; }
    صحيح reloopLimit() ثابت { إرجاع m_reloopLimit; }
    صحيح replicationLimit() ثابت { إرجاع m_replicationLimit; }
    VOptionBool skipIdentical() ثابت { إرجاع m_skipIdentical; }
    bool stopFail() ثابت { إرجاع m_stopFail; }
    صحيح threads() ثابت VL_MT_SAFE { إرجاع m_threads; }
    صحيح threadsMaxMTasks() ثابت { إرجاع m_threadsMaxMTasks; }
    bool mtasks() ثابت VL_MT_SAFE { إرجاع (m_threads > 1); }
    VTimescale timeDefaultPrec() ثابت { إرجاع m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() ثابت { إرجاع m_timeDefaultUnit; }
    VTimescale timeOverridePrec() ثابت { إرجاع m_timeOverridePrec; }
    VTimescale timeOverrideUnit() ثابت { إرجاع m_timeOverrideUnit; }
    VTimescale timeComputePrec(ثابت VTimescale& flag) ثابت;
    VTimescale timeComputeUnit(ثابت VTimescale& flag) ثابت;
    صحيح traceDepth() ثابت { إرجاع m_traceDepth; }
    صحيح traceMaxArray() ثابت { إرجاع m_traceMaxArray; }
    صحيح traceMaxWidth() ثابت { إرجاع m_traceMaxWidth; }
    bool useTraceParallel() ثابت {
        إرجاع trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    صحيح unroll_تعداد() ثابت { إرجاع m_unroll_تعداد; }
    صحيح unrollLimit() ثابت { إرجاع m_unrollLimit; }
    صحيح unrollStmts() ثابت { إرجاع m_unrollStmts; }
    صحيح constraintArrayLimit() ثابت { إرجاع m_constraintArrayLimit; }
    صحيح verilateJobs() ثابت { إرجاع m_verilateJobs; }

    صحيح compLimitBlocks() ثابت { إرجاع m_compLimitBlocks; }
    صحيح compLimitMembers() ثابت VL_MT_SAFE { إرجاع m_compLimitMembers; }
    صحيح compLimitParens() ثابت { إرجاع m_compLimitParens; }

    string diagnosticsSarifOutput() ثابت VL_MT_SAFE {
        إرجاع m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ثابت { إرجاع m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() ثابت { إرجاع m_hierParamsFile; }
    string jsonOnlyOutput() ثابت { إرجاع m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ثابت { إرجاع m_jsonOnlyMetaOutput; }
    string l2Name() ثابت { إرجاع m_l2Name; }
    string libCreate() ثابت { إرجاع m_libCreate; }
    string libCreateName(bool shared) ثابت {
        string libName = "lib" + libCreate();
        إذا (shared) {
            libName += ".so";
        } وإلا {
            libName += ".a";
        }
        إرجاع libName;
    }
    string main_قمة_Name() ثابت { إرجاع m_main_قمة_Name; }
    string makeDir() ثابت VL_MT_SAFE { إرجاع m_makeDir; }
    string modPrefix() ثابت VL_MT_SAFE { إرجاع m_modPrefix; }
    string pipeFilter() ثابت { إرجاع m_pipeFilter; }
    string prefix() ثابت VL_MT_SAFE { إرجاع m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_مفتاح_Provided() ثابت { إرجاع !m_protect_مفتاح.empty(); }
    string protect_مفتاح_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string قمة_Module() ثابت { إرجاع m_قمة_Module; }  // As AstNode::encodeName()
    bool no_Trace_قمة() ثابت { إرجاع m_no_Trace_قمة; }
    string unusedRegexp() ثابت { إرجاع m_unusedRegexp; }
    string waiverOutput() ثابت { إرجاع m_waiverOutput; }
    string work() ثابت { إرجاع m_work; }
    bool isWaiverOutput() ثابت { إرجاع !m_waiverOutput.empty(); }
    string xAssign() ثابت { إرجاع m_xAssign; }
    string xInitial() ثابت { إرجاع m_xInitial; }

    ثابت VStringSet& cppFiles() ثابت { إرجاع m_cppFiles; }
    ثابت VStringList& cFlags() ثابت { إرجاع m_cFlags; }
    ثابت VStringSet& compilerIncludes() ثابت { إرجاع m_compilerIncludes; }
    ثابت VStringList& ldLibFiles() ثابت { إرجاع m_ldLibFiles; }
    ثابت VStringList& ldLibs() ثابت { إرجاع m_ldLibs; }
    ثابت VStringList& makeFlags() ثابت { إرجاع m_makeFlags; }
    ثابت VFileLibSet& libraryFiles() ثابت { إرجاع m_libraryFiles; }
    ثابت VStringSet& libmapFiles() ثابت { إرجاع m_libmapFiles; }
    ثابت VFileLibList& vFiles() ثابت { إرجاع m_vFiles; }
    ثابت VFileLibSet& vltFiles() ثابت { إرجاع m_vltFiles; }
    ثابت VStringList& forceIncs() ثابت { إرجاع m_forceIncs; }

    bool hasParameter(ثابت string& name);
    string parameter(ثابت string& name);
    فراغ checkParameters();

    bool isFuture(ثابت string& flag) ثابت;
    bool isFuture0(ثابت string& flag) ثابت;
    bool isFuture1(ثابت string& flag) ثابت;
    bool isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ثابت { إرجاع m_fAcycSimp; }
    bool fAssemble() ثابت { إرجاع m_fAssemble; }
    bool fBitScanLoops() ثابت { إرجاع m_fBitScanLoops; }
    bool fBitblast() ثابت { إرجاع m_fBitblast; }
    bool fCaseDecoder() ثابت { إرجاع m_fCaseDecoder; }
    bool fCaseTable() ثابت { إرجاع m_fCaseTable; }
    bool f_Case_شجرة() ثابت { إرجاع m_f_Case_شجرة; }
    bool fCombine() ثابت { إرجاع m_fCombine; }
    bool fConst() ثابت { إرجاع m_fConst; }
    bool fConstBeforeDfg() ثابت { إرجاع m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_شجرة() ثابت { إرجاع m_f_Const_Bit_Op_شجرة; }
    bool fConstEager() ثابت { إرجاع m_fConstEager; }
    bool fDedupe() ثابت { إرجاع m_fDedupe; }
    bool fDfg() ثابت { إرجاع m_fDfg; }
    bool fDfgPeephole() ثابت { إرجاع m_fDfgPeephole; }
    bool f_Dfg_ادفع_Down_Sels() ثابت { إرجاع m_f_Dfg_ادفع_Down_Sels; }
    bool fDfgSynthesizeAll() ثابت { إرجاع m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ثابت std::string& name) ثابت {
        إرجاع !m_fDfgPeepholeDisabled.تعداد_2(name);
    }
    bool fDeadAssigns() ثابت { إرجاع m_fDeadAssigns; }
    bool fDeadCells() ثابت { إرجاع m_fDeadCells; }
    bool fDeadMethods() ثابت { إرجاع m_fDeadMethods; }
    bool fDecompose() ثابت { إرجاع m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ثابت { إرجاع m_fDecomposePacked; }
    bool fDecomposeUnpacked() ثابت { إرجاع m_fDecomposeUnpacked; }
    bool fExpand() ثابت { إرجاع m_fExpand; }
    bool fFuncBalanceCat() ثابت { إرجاع m_fFuncBalanceCat; }
    bool fFuncSplitCat() ثابت { إرجاع m_fFuncSplitCat; }
    bool fFunc() ثابت { إرجاع fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() ثابت { إرجاع m_fGate; }
    VOptionBool fIcoChangeDetect() ثابت { إرجاع m_fIcoChangeDetect; }
    bool fInline() ثابت { إرجاع m_fInline; }
    bool fInlineCFuncs() ثابت { إرجاع m_fInlineCFuncs; }
    bool fInlineFuncs() ثابت { إرجاع m_fInlineFuncs; }
    bool fInlineFuncsEager() ثابت { إرجاع m_fInlineFuncsEager; }
    bool fLife() ثابت { إرجاع m_fLife; }
    bool fLifePost() ثابت { إرجاع m_fLifePost; }
    bool fLocalize() ثابت { إرجاع m_fLocalize; }
    bool fMergeCond() ثابت { إرجاع m_fMergeCond; }
    bool fMergeCondMotion() ثابت { إرجاع m_fMergeCondMotion; }
    bool fReloop() ثابت { إرجاع m_fReloop; }
    bool fReorder() ثابت { إرجاع m_fReorder; }
    bool fSlice() ثابت { إرجاع m_fSlice; }
    صحيح fSliceElementLimit() ثابت { إرجاع m_fSliceElementLimit; }
    bool fSplit() ثابت { إرجاع m_fSplit; }
    bool fSubst() ثابت { إرجاع m_fSubst; }
    bool fSubstConst() ثابت { إرجاع m_fSubstConst; }
    bool fTable() ثابت { إرجاع m_fTable; }
    bool fTaskifyAll() ثابت { إرجاع m_fTaskifyAll; }

    std::string traceClassBase() ثابت VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() ثابت VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() ثابت VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() ثابت VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() ثابت VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() ثابت VL_MT_SAFE;

    bool hierarchical() ثابت { إرجاع m_hierarchical; }
    صحيح hierChild() ثابت VL_MT_SAFE { إرجاع m_hierChild; }
    صحيح hierThreads() ثابت VL_MT_SAFE { إرجاع m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_قمة() ثابت VL_MT_SAFE { إرجاع !m_hierChild && !m_hierBlocks.empty(); }
    ثابت V3HierBlockOptSet& hierBlocks() ثابت { إرجاع m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_قمة_Data_Dir() ثابت VL_MT_SAFE {
        إرجاع hier_قمة() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ساكن string version() VL_PURE;
    ساكن string argString(صحيح argc, حرف** argv);  ///< Return list of arguments as simple string
    string allArgsString() ثابت VL_MT_SAFE;  ///< Return all passed arguments as simple string
    ثابت std::list<std::pair<std::list<std::string>, bool>>& allArgs() ثابت;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_قمة) ثابت;
    فراغ parseOpts(File_سطر* fl, صحيح argc, حرف** argv) VL_MT_DISABLED;
    فراغ parseOptsList(File_سطر* fl, ثابت string& optdir, صحيح argc, حرف** argv) VL_MT_DISABLED;
    فراغ parseOptsFile(File_سطر* fl, ثابت string& filename, bool rel) VL_MT_DISABLED;

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
    ساكن string getenv_VERILATOR_جذر();
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
    string filePath(File_سطر* fl, ثابت string& modname, ثابت string& lastpath,
                    ثابت string& errmsg);
    string file_Path_Looked_رسالة(File_سطر* fl, ثابت string& modname);
    V3LangCode fileLanguage(ثابت string& filename);
    ساكن bool fileStatNormal(ثابت string& filename);

    // METHODS (other OS)
    ساكن فراغ throwSigsegv();
};

//######################################################################

#endif  // guard
