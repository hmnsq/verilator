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

#اگر_غیر_تعریف_شدہ VERILATOR_V3OPTIONS_H_
#تعریف_کرو VERILATOR_V3OPTIONS_H_

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
عوامی:
    شمار en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    شمار en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    ثابت حرف* ascii() ثابت {
        ساکن ثابت حرف* ثابت نام[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        واپس نام[m_e];
    }
    bool isDefault() ثابت { واپس m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ثابت { واپس m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool is_سیٹ_True() ثابت { واپس m_e == OPT_TRUE; }
    bool is_سیٹ_False() ثابت { واپس m_e == OPT_FALSE; }
    خالی سیٹ_True_Or_False(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool عامل==(ثابت VOptionBool& lhs, ثابت VOptionBool& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VOptionBool& lhs, VOptionBool::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VOptionBool::en lhs, ثابت VOptionBool& rhs) { واپس lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ثابت string m_filename;  // Filename
    ثابت string m_libname;  // Libname
عوامی:
    VFileLibName(ثابت string& filename, ثابت string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ثابت VFileLibName& rhs) = طےشدہ;
    string filename() ثابت { واپس m_filename; }
    string libname() ثابت { واپس m_libname; }
    bool عامل==(ثابت VFileLibName& rhs) ثابت {
        واپس m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool عامل<(ثابت VFileLibName& rhs) ثابت {
        اگر (m_filename < rhs.m_filename) واپس true;
        اگر (m_filename > rhs.m_filename) واپس false;
        واپس m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::سمتیہ<VFileLibName>;
using VFile_Lib_سیٹ = std::سیٹ<VFileLibName>;

// ######################################################################

class VTimescale final {
عوامی:
    شمار en : uint8_t {
        // clang-format off
        TS_100S = 0, TS_10S = 1, TS_1S = 2,
        TS_100MS = 3, TS_10MS = 4, TS_1MS = 5,
        TS_100US = 6, TS_10US = 7, TS_1US = 8,
        TS_100NS = 9, TS_10NS = 10, TS_1NS = 11,
        TS_100PS = 12, TS_10PS = 13, TS_1PS = 14,
        TS_100FS = 15, TS_10FS = 16, TS_1FS = 17,
        // clang-format on
        NONE = 18,
        ENUM_انتہا
    };
    شمار : uint8_t { TS_DEFAULT = TS_1PS };
    شمار en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ثابت string& قدر, bool& badr);
    VTimescale(دوہرا قدر, bool& badr) {
        badr = false;
        برائے (صحیح i = TS_100S; i < ENUM_انتہا; ++i) {
            m_e = static_cast<en>(i);
            اگر (multiplier() == قدر) توڑو;
        }
        اگر (multiplier() != قدر) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ثابت { واپس m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ساکن خالی parseSlashed(File_سطر* fl, ثابت حرف* textp, VTimescale& unitr, VTimescale& precr,
                             bool allow_خالی_خانے = false);
    ثابت حرف* ascii() ثابت {
        ساکن ثابت حرف* ثابت نام[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        واپس نام[m_e];
    }
    صحیح power_کا_Ten() ثابت { واپس 2 - static_cast<صحیح>(m_e); }
    دوہرا multiplier() ثابت {
        ساکن ثابت دوہرا اقدار[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        واپس اقدار[m_e];
    }
};
constexpr bool عامل==(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VTimescale& lhs, VTimescale::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(VTimescale::en lhs, ثابت VTimescale& rhs) { واپس lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool عامل<(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    واپس lhs.m_e > rhs.m_e;
}
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VTimescale& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
عوامی:
    شمار en : uint8_t {
        DEFAULT = 0,  // Use the global default expansion type
        AUTO,  // Expands only reset and default
        AUTO_EXPAND,  // Expands transitions between all possible states
        بھرا,  // Cover all possible transitions
    } m_e;
    VFsmExpandType()
        : m_e{DEFAULT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFsmExpandType(en _e)
        : m_e{_e} {}
    explicit VFsmExpandType(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    constexpr ثابت حرف* ascii() ثابت {
        constexpr ثابت حرف* ثابت نام[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        واپس نام[m_e];
    }
};
constexpr bool عامل==(ثابت VFsmExpandType& lhs, ثابت VFsmExpandType& rhs) VL_MT_محفوظ {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_محفوظ {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VFsmExpandType::en lhs, ثابت VFsmExpandType& rhs) VL_MT_محفوظ {
    واپس lhs == rhs.m_e;
}
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت VFsmExpandType& rhs) VL_MT_محفوظ {
    واپس os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3_درجہ_وار_بلاک_Option final {
عوامی:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ثابت std::string, std::string>;

نجی:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

عوامی:
    explicit V3_درجہ_وار_بلاک_Option(ثابت string& optstring);
    ثابت string& origName() ثابت { واپس m_origName; }
    ثابت string& mangledName() ثابت { واپس m_mangledName; }
    ثابت ParamStrMap params() ثابت { واپس m_parameters; }
};

using V3_Hier_بلاک_Opt_سیٹ = std::map<ثابت std::string, V3_درجہ_وار_بلاک_Option>;

//######################################################################
// V3Options - Command line options

class V3Options final {
عوامی:
نجی:
    // TYPES
    using DebugLevelMap = std::map<ثابت std::string, غیرعلامتی>;

    // MEMBERS (general options)
    V3OptionsImp* m_impp;  // Slow hidden options

    // clang-format off
    VString_سیٹ m_cppFiles;     // argument: C++ files to link against
    VStringList m_cFlags;      // argument: user CFLAGS
    VStringList m_ldLibs;      // argument: user LDFLAGS
    VStringList m_ldLibFiles;  // argument: library files, subset of m_ldLibs
    VStringList m_makeFlags;   // argument: user MAKEFLAGS
    VString_سیٹ m_compilerIncludes; // argument: user --compiler-include
    VString_سیٹ m_futures;      // argument: -Wfuture- list
    VString_سیٹ m_future0s;     // argument: -future list
    VString_سیٹ m_future1s;     // argument: -future1 list
    VFile_Lib_سیٹ m_libraryFiles; // argument: Verilog -v files
    VFileLibList m_vFiles;      // argument: Verilog files to read
    VFile_Lib_سیٹ m_vltFiles;     // argument: Verilator config files to read
    VString_سیٹ m_libmapFiles;  // argument: --libmap files to read
    VStringList m_forceIncs;   // argument: -FI
    DebugLevelMap m_debugLevel; // argument: --debugi-<srcfile/tag> <level>
    DebugLevelMap m_dumpLevel;  // argument: --dumpi-<srcfile/tag> <level>
    std::map<ثابت string, string> m_parameters;  // Parameters
    std::map<ثابت string, V3_درجہ_وار_بلاک_Option> m_hier_بلاکیں;  // main switch: --hierarchical-block
    VString_سیٹ m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_سطر = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_ثنائی = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_اظہاریہ = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_سطر = false;    // main switch: --coverage-block
    bool m_coverage_ملاؤ_Instances = false;  // main switch: --coverage-merge-instances
    bool m_coverage_فی_Instance = false;  // main switch: --coverage-per-instance
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
    bool m_debug_تقسیم = false;  // main switch: --debug-partition
    bool m_debugPreprocPassthru = false;  // main switch: --debug-preproc-passthru
    bool m_debugProtect = false;    // main switch: --debug-protect
    bool m_debugSelfTest = false;   // main switch: --debug-self-test
    bool m_debug_ڈھیر_Check = false;  // main switch: --debug-stack-check
    صحیح m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
    bool m_debug_چوڑائی = false;      // main switch: --debug-width
    bool m_decoration = true;       // main switch: --decoration
    bool m_decorationNodes = false;  // main switch: --decoration=nodes
    bool m_diagnosticsSarif = false;  // main switch: --diagnostics-sarif
    bool m_dpiHdrOnly = false;      // main switch: --dpi-hdr-only
    bool m_emitAccessors = false;   // main switch: --emit-accessors
    bool m_exe = false;             // main switch: --exe
    bool m_flatten = false;         // main switch: --flatten
    bool m_fourstate = false;       // main switch: --fourstate
    bool m_درجہ_وار = false;    // main switch: --hierarchical
    bool m_ignc = false;            // main switch: --ignc
    bool m_jsonOnly = false;        // main switch: --json-only
    bool m_lintOnly = false;        // main switch: --lint-only
    bool m_gmake = false;           // main switch: --make gmake
    bool m_makeJson = false;        // main switch: --make json
    bool m_main = false;            // main switch: --main
    bool m_برآمد_Format_Ok = false;     // main switch: --cc, --sc or --sp was specified
    bool m_pedantic = false;        // main switch: --Wpedantic
    bool m_pinsInoutEnables = false;// main switch: --pins-inout-enables
    bool m_pinsScUint = false;      // main switch: --pins-sc-uint
    bool m_pinsScUintBool = false;  // main switch: --pins-sc-uint-bool
    bool m_pinsScBigUint = false;   // main switch: --pins-sc-biguint
    bool m_pinsUint8 = false;       // main switch: --pins-uint8
    bool m_preprocComments = false;  // main switch: --preproc-comments
    bool m_preproc_متعین_کرتا = false;  // main switch: --preproc-defines
    bool m_profC = false;           // main switch: --prof-c
    bool m_profCFuncs = false;      // main switch: --prof-cfuncs
    bool m_profExec = false;        // main switch: --prof-exec
    bool m_profPgo = false;         // main switch: --prof-pgo
    bool m_protectIds = false;      // main switch: --protect-ids
    bool m_عوامی = false;          // main switch: --public
    bool m_عوامی_Flat_RW = false;    // main switch: --public-flat-rw
    bool m_عوامی_Ignore = false;    // main switch: --public-ignore
    bool m_عوامی_Params = false;    // main switch: --public-params
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
    bool m_دھاگے_Coarsen = true;   // main switch: --threads-coarsen
    bool m_دھاگے_Dpi_خالص = true;   // main switch: --threads-dpi all/pure
    bool m_دھاگے_Dpi_Unpure = false;  // main switch: --threads-dpi all
    VOptionBool m_timing;           // main switch: --timing
    bool m_trace = false;           // main switch: --trace
    bool m_traceCoverage = false;   // main switch: --trace-coverage
    bool m_traceEnabledFst = false;  // main switch: --trace-fst
    bool m_traceEnabledSaif = false;  // main switch: --trace-saif
    bool m_traceEnabledVcd = false;  // main switch: --trace-vcd
    bool m_traceParams = true;      // main switch: --trace-params
    bool m_traceStructs = false;    // main switch: --trace-structs
    bool m_no_Trace_چوٹی = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_x_Initial_کنارہ = false;    // main switch: --x-initial-edge

    صحیح         m_buildJobs = -1;    // main switch: --build-jobs, -j
    صحیح         m_coverage_اظہاریہ_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    صحیح         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    صحیح         m_converge_حد_بندی = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    صحیح         m_coverage_Max_چوڑائی = 256; // main switch: --coverage-max-width
    صحیح         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    صحیح         m_expand_حد_بندی = 256;  // main switch: --expand-limit
    صحیح         m_gateStmts = 100;    // main switch: --gate-stmts
    صحیح         m_hier_بچہ = 0;      // main switch: --hierarchical-child
    صحیح         m_hier_دھاگے = 0;      // main switch: --hierarchical-threads
    صحیح         m_if_گہرائی = 0;      // main switch: --if-depth
    صحیح         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    صحیح         m_inline_CFuncs_حاصل_ضرب = 200;   // main switch: --inline-cfuncs-product
    صحیح         m_inlineMult = 2000;   // main switch: --inline-mult
    صحیح         m_instr_گنتی_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    صحیح         m_localize_Max_سائز = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    صحیح         m_max_عدد_چوڑائی = 65536;  // main switch: --max-num-width
    صحیح         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    صحیح         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    صحیح         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_output_Groups_سائز = 512 * 1024 * 1024;  // main switch: --output-groups-size
    صحیح         m_output_بانٹو = 20000;  // main switch: --output-split
    صحیح         m_output_بانٹو_CFuncs = -1;  // main switch: --output-split-cfuncs
    صحیح         m_output_بانٹو_CTrace = -1;  // main switch: --output-split-ctrace
    صحیح         m_pinsBv = 65;       // main switch: --pins-bv
    صحیح         m_preproc_Token_حد_بندی = 40000; // main switch: --preproc-token-limit
    صحیح         m_عوامی_گہرائی = 0;   // main switch: --public-depth
    صحیح         m_reloop_حد_بندی = 40; // main switch: --reloop-limit
    صحیح         m_replication_حد_بندی = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    صحیح         m_دھاگے = 1;      // main switch: --threads
    صحیح         m_دھاگے_Max_MTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_وقت_Default_Prec;  // main switch: --timescale
    VTimescale  m_وقت_Default_Unit;  // main switch: --timescale
    VTimescale  m_وقت_Override_Prec;  // main switch: --timescale-override
    VTimescale  m_وقت_Override_Unit;  // main switch: --timescale-override
    صحیح         m_trace_گہرائی = 0;   // main switch: --trace-depth
    صحیح         m_traceMaxArray = 32;  // main switch: --trace-max-array
    صحیح         m_trace_Max_چوڑائی = 4096; // main switch: --trace-max-width
    صحیح         m_unroll_گنتی = 64;  // main switch: --unroll-count
    صحیح         m_unroll_حد_بندی = 16384;  // main switch: --unroll-limit
    صحیح         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    صحیح         m_constraint_Array_حد_بندی = 64;  // main switch: --constraint-array-limit
    صحیح         m_verilateJobs = -1;  // main switch: --verilate-jobs

    صحیح         m_comp_حد_بندی_بلاکیں = 0;  // compiler selection; number of nested blocks
    صحیح         m_comp_حد_بندی_Members = 64;  // compiler selection; number of members in struct before make anon array
    صحیح         m_comp_حد_بندی_Parens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_چوٹی_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_mod_سابقہ;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_سابقہ;       // main switch: --prefix
    string      m_protect_کلید;   // main switch: --protect-key
    string      m_چوٹی_Module;    // main switch: --top-module
    string      m_غیر_مستعمل_Regexp; // main switch: --unused-regexp
    string      m_waiverOutput;  // main switch: --waiver-output {filename}
    string      m_work = "work";  // main switch: --work {libname}
    string      m_x_تفویض;      // main switch: --x-assign
    string      m_xInitial;     // main switch: --x-initial

    // Language is now held in FileLine, on a per-node basis. However we still
    // have a concept of the default language at a global level.
    V3_Lang_رمز  m_defaultLanguage;      // main switch: --language

    // MEMBERS (optimizations)
    bool m_fAcycSimp;    // main switch: -fno-acyc-simp: acyclic pre-optimizations
    bool m_fAssemble;    // main switch: -fno-assemble: assign assemble
    bool m_f_ثنائی_ہندسہ_جائزہ_Loops;  // main switch: -fno-bit-scan-loops: convert bit scan loops to builtins
    bool m_fBitblast;    // main switch: -fno-bitblast: split packed variables into bit ranges
    bool m_fCaseDecoder; // main switch: -fno-case-decoder: case decoder conversion
    bool m_f_Case_جدول;   // main switch: -fno-case-table: case table conversion
    bool m_f_Case_درخت;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_ثنائی_ہندسہ_Op_درخت;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ڈالو_نیچے_کو_Sels = true; // main switch: -fno-dfg-push-down-sels
    bool m_fDfg;         // main switch: -fno-dfg
    bool m_fDfgSynthesizeAll = false;  // main switch: -fdfg-synthesize-all
    bool m_fDeadAssigns;     // main switch: -fno-dead-assigns: remove dead assigns
    bool m_fDeadCells;   // main switch: -fno-dead-cells: remove dead cells
    bool m_fDeadMethods;   // main switch: -fno-dead-methods: remove dead methods
    bool m_fDecomposePacked;  // main switch: -fno-decompose-packed: packed decomposition
    bool m_fDecomposeUnpacked;  // main switch: -fno-decompose-unpacked: unpacked decomposition
    bool m_fExpand;      // main switch: -fno-expand: expansion of C macros
    bool m_fFuncBalanceCat = true;  // main switch: -fno-func-balance-cat: expansion of C macros
    bool m_f_Func_بانٹو_Cat = true;  // main switch: -fno-func-split-cat: expansion of C macros
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
    bool m_f_ملاؤ_Cond;   // main switch: -fno-merge-cond: merge conditionals
    bool m_f_ملاؤ_Cond_Motion = true; // main switch: -fno-merge-cond-motion: perform code motion
    bool m_fReloop;      // main switch: -fno-reloop: reform loops
    bool m_fReorder;     // main switch: -fno-reorder: reorder assignments in blocks
    bool m_f_قاش = true;  // main switch: -fno-slice: array assignment slicing
    صحیح  m_f_قاش_Element_حد_بندی = 256;  // main switch: --fslice-element-limit
    bool m_f_بانٹو;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_f_جدول;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_دستیاب = false;  // Set to true at the end of option parsing

    // METHODS
    خالی جوڑو_Arg(حرف** argv, سائز_t گنتی, bool isForRerun);
    خالی جوڑو_Arg(ثابت std::string& arg, bool isForRerun);
    خالی جوڑو_سطر_Arg(ثابت string& arg);
    خالی جوڑو_Define(ثابت string& defline, bool allowPlus) VL_MT_DISABLED;
    خالی جوڑو_Future(ثابت string& flag);
    خالی جوڑو_Future0(ثابت string& flag);
    خالی جوڑو_Future1(ثابت string& flag);
    خالی جوڑو_Inc_Dir_User(ثابت string& incdir);  // User requested
    خالی جوڑو_Inc_Dir_Fallback(ثابت string& incdir);  // Low priority if not found otherwise
    خالی جوڑو_Parameter(ثابت string& paramline, bool allowPlus);
    خالی جوڑو_Lang_Ext(ثابت string& langext, ثابت V3_Lang_رمز& lc);
    خالی جوڑو_Lib_Ext_V(ثابت string& libext);
    خالی optimize(صحیح level);
    خالی show_نسخہ(bool verbose);
    خالی coverage(bool flag) {
        m_coverage_سطر = m_coverageBranch = m_coverageToggle = m_coverage_اظہاریہ = m_coverageFsm
            = m_coverageUser = flag;
    }
    ساکن bool suffixed(ثابت string& sw, ثابت حرف* arg);
    ساکن string parseFileArg(ثابت string& optdir, ثابت string& relfilename);
    string file_راستہ_Check_One_Dir(ثابت string& modname, ثابت string& dirname);
    ساکن صحیح strip_Options_For_بچہ_تسلسل(ثابت string& opt, bool for_چوٹی);
    خالی validateIdentifier(File_سطر* fl, ثابت string& arg, ثابت string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

عوامی:
    V3Options();
    ~V3Options();
    خالی سیٹ_Debug_منوال(صحیح level);
    غیرعلامتی debugLevel(ثابت string& لیبل) ثابت VL_MT_محفوظ;
    غیرعلامتی debugSrcLevel(ثابت string& srcfile_راستہ) ثابت VL_MT_محفوظ;
    غیرعلامتی dumpLevel(ثابت string& لیبل) ثابت VL_MT_محفوظ;
    غیرعلامتی dumpSrcLevel(ثابت string& srcfile_راستہ) ثابت VL_MT_محفوظ;

    // METHODS
    خالی جوڑو_Cpp_File(ثابت string& filename);
    خالی جوڑو_CFlags(ثابت string& filename);
    خالی جوڑو_Compiler_Includes(ثابت string& filename);
    خالی جوڑو_Ld_Lib_File(ثابت string& filename);
    خالی جوڑو_Ld_Libs(ثابت string& filename);
    خالی جوڑو_Make_Flags(ثابت string& filename);
    خالی جوڑو_Lib_Map_File(ثابت string& filename);
    خالی جوڑو_Library_File(ثابت string& filename, ثابت string& libname);
    خالی جوڑو_VFile(ثابت string& filename, ثابت string& libname);
    خالی جوڑو_Vlt_File(ثابت string& filename, ثابت string& libname);
    خالی جوڑو_Force_Inc(ثابت string& filename);
    bool دستیاب() ثابت VL_MT_محفوظ { واپس m_دستیاب; }
    خالی cc_سیٹ();
    خالی decorations(File_سطر* fl, ثابت string& filename);
    خالی notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ثابت { واپس m_preprocOnly; }
    bool makePhony() ثابت { واپس m_makePhony; }
    bool preproc_No_سطر() ثابت { واپس m_preproc_No_سطر; }
    bool preprocResolve() ثابت { واپس m_preprocResolve; }
    صحیح preproc_Token_حد_بندی() ثابت { واپس m_preproc_Token_حد_بندی; }
    bool underlineZero() ثابت { واپس m_underlineZero; }
    bool systemC() ثابت VL_MT_محفوظ { واپس m_systemC; }
    bool savable() ثابت VL_MT_محفوظ { واپس m_savable; }
    VOptionBool schedZeroDelay() ثابت { واپس m_schedZeroDelay; }
    bool stats() ثابت { واپس m_stats; }
    bool statsVars() ثابت { واپس m_statsVars; }
    bool stdPackage() ثابت { واپس m_stdPackage; }
    bool stdWaiver() ثابت { واپس m_stdWaiver; }
    bool assertOn() ثابت { واپس m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ثابت { واپس m_assertCase; }
    bool autoflush() ثابت { واپس m_autoflush; }
    bool bboxSys() ثابت { واپس m_bboxSys; }
    bool bboxUnsup() ثابت { واپس m_bboxUnsup; }
    bool build() ثابت { واپس m_build; }
    string buildDepBin() ثابت { واپس m_buildDepBin; }
    خالی buildDepBin(ثابت string& flag) { m_buildDepBin = flag; }
    bool context() ثابت VL_MT_محفوظ { واپس m_context; }
    bool coverage() ثابت VL_MT_محفوظ {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        واپس m_coverage_سطر || m_coverageBranch || m_coverageToggle || m_coverage_اظہاریہ
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ثابت VL_MT_محفوظ {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        واپس m_coverage_سطر || m_coverageBranch || m_coverageToggle || m_coverage_اظہاریہ
               || m_coverageUser;
    }
    bool coverageBranch() ثابت { واپس m_coverageBranch; }
    bool coverage_اظہاریہ() ثابت { واپس m_coverage_اظہاریہ; }
    bool coverageFsm() ثابت { واپس m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ثابت { واپس m_coverageFsmExpand; }
    صحیح coverageFsmMaxArcs() ثابت { واپس m_coverageFsmMaxArcs; }
    bool coverage_سطر() ثابت { واپس m_coverage_سطر; }
    bool coverage_ملاؤ_Instances() ثابت { واپس m_coverage_ملاؤ_Instances; }
    bool coverage_فی_Instance() ثابت { واپس m_coverage_فی_Instance; }
    bool coverageToggle() ثابت { واپس m_coverageToggle; }
    bool coverageUnderscore() ثابت { واپس m_coverageUnderscore; }
    bool coverageUser() ثابت { واپس m_coverageUser; }
    bool debugCheck() ثابت VL_MT_محفوظ { واپس m_debugCheck; }
    bool debugCollision() ثابت { واپس m_debugCollision; }
    bool debugEmitV() ثابت VL_MT_محفوظ { واپس m_debugEmitV; }
    bool debugExitElab() ثابت { واپس m_debugExitElab; }
    bool debugExitParse() ثابت { واپس m_debugExitParse; }
    bool debugLeak() ثابت { واپس m_debugLeak; }
    bool debugNondeterminism() ثابت { واپس m_debugNondeterminism; }
    bool debug_تقسیم() ثابت { واپس m_debug_تقسیم; }
    bool debugPreprocPassthru() ثابت VL_MT_محفوظ { واپس m_debugPreprocPassthru; }
    bool debugProtect() ثابت VL_MT_محفوظ { واپس m_debugProtect; }
    صحیح debugRuntimeTimeout() ثابت { واپس m_debugRuntimeTimeout; }
    bool debugSelfTest() ثابت { واپس m_debugSelfTest; }
    bool debug_ڈھیر_Check() ثابت { واپس m_debug_ڈھیر_Check; }
    bool debug_چوڑائی() ثابت VL_خالص { واپس m_debug_چوڑائی; }
    bool decoration() ثابت VL_MT_محفوظ { واپس m_decoration; }
    bool decorationNodes() ثابت VL_MT_محفوظ { واپس m_decorationNodes; }
    bool diagnosticsSarif() ثابت VL_MT_محفوظ { واپس m_diagnosticsSarif; }
    bool dpiHdrOnly() ثابت { واپس m_dpiHdrOnly; }
    bool dumpAstPatterns() ثابت {
        واپس m_dumpLevel.گنتی("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dump_متعین_کرتا() ثابت { واپس m_dumpLevel.گنتی("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ثابت {
        واپس m_dumpLevel.گنتی("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_درخت_Dot() ثابت {
        واپس m_dumpLevel.گنتی("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ثابت { واپس m_emitAccessors; }
    bool exe() ثابت { واپس m_exe; }
    bool flatten() ثابت { واپس m_flatten; }
    bool fourstate() ثابت { واپس m_fourstate; }
    bool gmake() ثابت { واپس m_gmake; }
    bool makeJson() ثابت { واپس m_makeJson; }
    bool دھاگے_Dpi_خالص() ثابت { واپس m_دھاگے_Dpi_خالص; }
    bool دھاگے_Dpi_Unpure() ثابت { واپس m_دھاگے_Dpi_Unpure; }
    bool دھاگے_Coarsen() ثابت { واپس m_دھاگے_Coarsen; }
    VOptionBool timing() ثابت { واپس m_timing; }
    bool trace() ثابت { واپس m_trace; }
    bool traceCoverage() ثابت { واپس m_traceCoverage; }
    bool traceEnabledFst() ثابت { واپس m_traceEnabledFst; }
    bool traceEnabledSaif() ثابت { واپس m_traceEnabledSaif; }
    bool traceEnabledVcd() ثابت { واپس m_traceEnabledVcd; }
    bool traceParams() ثابت { واپس m_traceParams; }
    bool traceStructs() ثابت { واپس m_traceStructs; }
    bool traceUnderscore() ثابت { واپس m_traceUnderscore; }
    bool مین() ثابت { واپس m_main; }
    bool برآمد_Format_Ok() ثابت { واپس m_برآمد_Format_Ok; }
    bool jsonOnly() ثابت { واپس m_jsonOnly; }
    bool keep_عارضی_Files() ثابت { واپس (V3Error::debugDefault() != 0); }
    bool pedantic() ثابت { واپس m_pedantic; }
    bool pinsInoutEnables() ثابت { واپس m_pinsInoutEnables; }
    bool pinsScUint() ثابت { واپس m_pinsScUint; }
    bool pinsScUintBool() ثابت { واپس m_pinsScUintBool; }
    bool pinsScBigUint() ثابت VL_MT_محفوظ { واپس m_pinsScBigUint; }
    bool pinsUint8() ثابت { واپس m_pinsUint8; }
    bool preprocComments() ثابت { واپس m_preprocComments; }
    bool preproc_متعین_کرتا() ثابت { واپس m_preproc_متعین_کرتا; }
    bool profC() ثابت { واپس m_profC; }
    bool profCFuncs() ثابت { واپس m_profCFuncs; }
    bool profExec() ثابت { واپس m_profExec; }
    bool profPgo() ثابت { واپس m_profPgo; }
    bool استعمال_کرتا_Profiler() ثابت { واپس profExec() || profPgo(); }
    bool protectIds() ثابت VL_MT_محفوظ { واپس m_protectIds; }
    bool all_عوامی() ثابت { واپس m_عوامی; }
    bool عوامی_Params() ثابت { واپس m_عوامی_Params; }
    bool عوامی_Off() ثابت { واپس m_عوامی_Ignore; }
    bool عوامی_Flat_RW() ثابت { واپس m_عوامی_Flat_RW; }
    صحیح عوامی_گہرائی() ثابت { واپس m_عوامی_گہرائی; }
    bool any_عوامی_Flat() ثابت { واپس m_عوامی_Params || m_عوامی_Flat_RW || m_عوامی_گہرائی; }
    bool lintOnly() ثابت VL_MT_محفوظ { واپس m_lintOnly; }
    bool ignc() ثابت { واپس m_ignc; }
    bool quietBuild() ثابت VL_MT_محفوظ { واپس m_quietBuild; }
    bool quietExit() ثابت VL_MT_محفوظ { واپس m_quietExit; }
    bool quietStats() ثابت VL_MT_محفوظ { واپس m_quietStats; }
    bool reportUnoptflat() ثابت { واپس m_reportUnoptflat; }
    bool verilate() ثابت { واپس m_verilate; }
    bool vpi() ثابت { واپس m_vpi.isTrue(); }
    bool waiverMultiline() ثابت { واپس m_waiverMultiline; }
    bool x_Initial_کنارہ() ثابت { واپس m_x_Initial_کنارہ; }
    bool serializeOnly() ثابت { واپس m_jsonOnly; }
    bool چوٹی_Ifaces_Supported() ثابت { واپس lintOnly() && !درجہ_وار(); }

    صحیح buildJobs() ثابت VL_MT_محفوظ { واپس m_buildJobs; }
    صحیح converge_حد_بندی() ثابت { واپس m_converge_حد_بندی; }
    صحیح coverage_اظہاریہ_Max() ثابت { واپس m_coverage_اظہاریہ_Max; }
    uint32_t coverageMaxBins() ثابت { واپس m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ثابت { واپس m_coverageMaxRealBins; }
    صحیح coverage_Max_چوڑائی() ثابت { واپس m_coverage_Max_چوڑائی; }
    صحیح debugAllocRandom() ثابت { واپس m_debugAllocRandom; }
    bool dump_درخت_Addrids() ثابت VL_MT_محفوظ;
    صحیح expand_حد_بندی() ثابت { واپس m_expand_حد_بندی; }
    صحیح gateStmts() ثابت { واپس m_gateStmts; }
    صحیح if_گہرائی() ثابت { واپس m_if_گہرائی; }
    صحیح inlineCFuncs() ثابت { واپس m_inlineCFuncs; }
    صحیح inline_CFuncs_حاصل_ضرب() ثابت { واپس m_inline_CFuncs_حاصل_ضرب; }
    صحیح inlineMult() ثابت { واپس m_inlineMult; }
    صحیح instr_گنتی_Dpi() ثابت { واپس m_instr_گنتی_Dpi; }
    صحیح localize_Max_سائز() ثابت { واپس m_localize_Max_سائز; }
    bool jsonEditNums() ثابت { واپس m_jsonEditNums; }
    bool jsonIds() ثابت { واپس m_jsonIds; }
    VOptionBool makeDepend() ثابت { واپس m_makeDepend; }
    صحیح max_عدد_چوڑائی() ثابت { واپس m_max_عدد_چوڑائی; }
    صحیح func_Recursion_گہرائی() ثابت { واپس m_funcRecursion; }
    صحیح module_Recursion_گہرائی() ثابت { واپس m_moduleRecursion; }
    صحیح output_بانٹو() ثابت { واپس m_output_بانٹو; }
    صحیح output_بانٹو_CFuncs() ثابت { واپس m_output_بانٹو_CFuncs; }
    صحیح output_بانٹو_CTrace() ثابت { واپس m_output_بانٹو_CTrace; }
    صحیح outputGroups() ثابت { واپس m_outputGroups; }
    int64_t output_Groups_سائز() ثابت { واپس m_output_Groups_سائز; }
    صحیح pinsBv() ثابت VL_MT_محفوظ { واپس m_pinsBv; }
    صحیح reloop_حد_بندی() ثابت { واپس m_reloop_حد_بندی; }
    صحیح replication_حد_بندی() ثابت { واپس m_replication_حد_بندی; }
    VOptionBool skipIdentical() ثابت { واپس m_skipIdentical; }
    bool stopFail() ثابت { واپس m_stopFail; }
    صحیح دھاگے() ثابت VL_MT_محفوظ { واپس m_دھاگے; }
    صحیح دھاگے_Max_MTasks() ثابت { واپس m_دھاگے_Max_MTasks; }
    bool mtasks() ثابت VL_MT_محفوظ { واپس (m_دھاگے > 1); }
    VTimescale وقت_Default_Prec() ثابت { واپس m_وقت_Default_Prec; }
    VTimescale وقت_Default_Unit() ثابت { واپس m_وقت_Default_Unit; }
    VTimescale وقت_Override_Prec() ثابت { واپس m_وقت_Override_Prec; }
    VTimescale وقت_Override_Unit() ثابت { واپس m_وقت_Override_Unit; }
    VTimescale وقت_Compute_Prec(ثابت VTimescale& flag) ثابت;
    VTimescale وقت_Compute_Unit(ثابت VTimescale& flag) ثابت;
    صحیح trace_گہرائی() ثابت { واپس m_trace_گہرائی; }
    صحیح traceMaxArray() ثابت { واپس m_traceMaxArray; }
    صحیح trace_Max_چوڑائی() ثابت { واپس m_trace_Max_چوڑائی; }
    bool useTraceParallel() ثابت {
        واپس trace() && traceEnabledVcd() && (دھاگے() > 1 || hier_بچہ() > 1);
    }
    صحیح unroll_گنتی() ثابت { واپس m_unroll_گنتی; }
    صحیح unroll_حد_بندی() ثابت { واپس m_unroll_حد_بندی; }
    صحیح unrollStmts() ثابت { واپس m_unrollStmts; }
    صحیح constraint_Array_حد_بندی() ثابت { واپس m_constraint_Array_حد_بندی; }
    صحیح verilateJobs() ثابت { واپس m_verilateJobs; }

    صحیح comp_حد_بندی_بلاکیں() ثابت { واپس m_comp_حد_بندی_بلاکیں; }
    صحیح comp_حد_بندی_Members() ثابت VL_MT_محفوظ { واپس m_comp_حد_بندی_Members; }
    صحیح comp_حد_بندی_Parens() ثابت { واپس m_comp_حد_بندی_Parens; }

    string diagnosticsSarifOutput() ثابت VL_MT_محفوظ {
        واپس m_diagnosticsSarifOutput.خالی_خانے() ? makeDir() + "/" + سابقہ() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ثابت { واپس m_exeName != "" ? m_exeName : سابقہ(); }
    VFileLibList hierParamFile() ثابت { واپس m_hierParamsFile; }
    string jsonOnlyOutput() ثابت { واپس m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ثابت { واپس m_jsonOnlyMetaOutput; }
    string l2Name() ثابت { واپس m_l2Name; }
    string libCreate() ثابت { واپس m_libCreate; }
    string libCreateName(bool shared) ثابت {
        string libName = "lib" + libCreate();
        اگر (shared) {
            libName += ".so";
        } ورنہ {
            libName += ".a";
        }
        واپس libName;
    }
    string main_چوٹی_Name() ثابت { واپس m_main_چوٹی_Name; }
    string makeDir() ثابت VL_MT_محفوظ { واپس m_makeDir; }
    string mod_سابقہ() ثابت VL_MT_محفوظ { واپس m_mod_سابقہ; }
    string pipeFilter() ثابت { واپس m_pipeFilter; }
    string سابقہ() ثابت VL_MT_محفوظ { واپس m_سابقہ; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_کلید_Provided() ثابت { واپس !m_protect_کلید.خالی_خانے(); }
    string protect_کلید_Defaulted() VL_MT_محفوظ;  // Set default key if not set by user
    string چوٹی_Module() ثابت { واپس m_چوٹی_Module; }  // As AstNode::encodeName()
    bool no_Trace_چوٹی() ثابت { واپس m_no_Trace_چوٹی; }
    string غیر_مستعمل_Regexp() ثابت { واپس m_غیر_مستعمل_Regexp; }
    string waiverOutput() ثابت { واپس m_waiverOutput; }
    string work() ثابت { واپس m_work; }
    bool isWaiverOutput() ثابت { واپس !m_waiverOutput.خالی_خانے(); }
    string x_تفویض() ثابت { واپس m_x_تفویض; }
    string xInitial() ثابت { واپس m_xInitial; }

    ثابت VString_سیٹ& cppFiles() ثابت { واپس m_cppFiles; }
    ثابت VStringList& cFlags() ثابت { واپس m_cFlags; }
    ثابت VString_سیٹ& compilerIncludes() ثابت { واپس m_compilerIncludes; }
    ثابت VStringList& ldLibFiles() ثابت { واپس m_ldLibFiles; }
    ثابت VStringList& ldLibs() ثابت { واپس m_ldLibs; }
    ثابت VStringList& makeFlags() ثابت { واپس m_makeFlags; }
    ثابت VFile_Lib_سیٹ& libraryFiles() ثابت { واپس m_libraryFiles; }
    ثابت VString_سیٹ& libmapFiles() ثابت { واپس m_libmapFiles; }
    ثابت VFileLibList& vFiles() ثابت { واپس m_vFiles; }
    ثابت VFile_Lib_سیٹ& vltFiles() ثابت { واپس m_vltFiles; }
    ثابت VStringList& forceIncs() ثابت { واپس m_forceIncs; }

    bool hasParameter(ثابت string& name);
    string parameter(ثابت string& name);
    خالی checkParameters();

    bool isFuture(ثابت string& flag) ثابت;
    bool isFuture0(ثابت string& flag) ثابت;
    bool isFuture1(ثابت string& flag) ثابت;
    bool isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ثابت { واپس m_fAcycSimp; }
    bool fAssemble() ثابت { واپس m_fAssemble; }
    bool f_ثنائی_ہندسہ_جائزہ_Loops() ثابت { واپس m_f_ثنائی_ہندسہ_جائزہ_Loops; }
    bool fBitblast() ثابت { واپس m_fBitblast; }
    bool fCaseDecoder() ثابت { واپس m_fCaseDecoder; }
    bool f_Case_جدول() ثابت { واپس m_f_Case_جدول; }
    bool f_Case_درخت() ثابت { واپس m_f_Case_درخت; }
    bool fCombine() ثابت { واپس m_fCombine; }
    bool fConst() ثابت { واپس m_fConst; }
    bool fConstBeforeDfg() ثابت { واپس m_fConstBeforeDfg; }
    bool f_Const_ثنائی_ہندسہ_Op_درخت() ثابت { واپس m_f_Const_ثنائی_ہندسہ_Op_درخت; }
    bool fConstEager() ثابت { واپس m_fConstEager; }
    bool fDedupe() ثابت { واپس m_fDedupe; }
    bool fDfg() ثابت { واپس m_fDfg; }
    bool fDfgPeephole() ثابت { واپس m_fDfgPeephole; }
    bool f_Dfg_ڈالو_نیچے_کو_Sels() ثابت { واپس m_f_Dfg_ڈالو_نیچے_کو_Sels; }
    bool fDfgSynthesizeAll() ثابت { واپس m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ثابت std::string& name) ثابت {
        واپس !m_fDfgPeepholeDisabled.گنتی(name);
    }
    bool fDeadAssigns() ثابت { واپس m_fDeadAssigns; }
    bool fDeadCells() ثابت { واپس m_fDeadCells; }
    bool fDeadMethods() ثابت { واپس m_fDeadMethods; }
    bool fDecompose() ثابت { واپس m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ثابت { واپس m_fDecomposePacked; }
    bool fDecomposeUnpacked() ثابت { واپس m_fDecomposeUnpacked; }
    bool fExpand() ثابت { واپس m_fExpand; }
    bool fFuncBalanceCat() ثابت { واپس m_fFuncBalanceCat; }
    bool f_Func_بانٹو_Cat() ثابت { واپس m_f_Func_بانٹو_Cat; }
    bool fFunc() ثابت { واپس f_Func_بانٹو_Cat() || fFuncBalanceCat(); }
    bool fGate() ثابت { واپس m_fGate; }
    VOptionBool fIcoChangeDetect() ثابت { واپس m_fIcoChangeDetect; }
    bool fInline() ثابت { واپس m_fInline; }
    bool fInlineCFuncs() ثابت { واپس m_fInlineCFuncs; }
    bool fInlineFuncs() ثابت { واپس m_fInlineFuncs; }
    bool fInlineFuncsEager() ثابت { واپس m_fInlineFuncsEager; }
    bool fLife() ثابت { واپس m_fLife; }
    bool fLifePost() ثابت { واپس m_fLifePost; }
    bool fLocalize() ثابت { واپس m_fLocalize; }
    bool f_ملاؤ_Cond() ثابت { واپس m_f_ملاؤ_Cond; }
    bool f_ملاؤ_Cond_Motion() ثابت { واپس m_f_ملاؤ_Cond_Motion; }
    bool fReloop() ثابت { واپس m_fReloop; }
    bool fReorder() ثابت { واپس m_fReorder; }
    bool f_قاش() ثابت { واپس m_f_قاش; }
    صحیح f_قاش_Element_حد_بندی() ثابت { واپس m_f_قاش_Element_حد_بندی; }
    bool f_بانٹو() ثابت { واپس m_f_بانٹو; }
    bool fSubst() ثابت { واپس m_fSubst; }
    bool fSubstConst() ثابت { واپس m_fSubstConst; }
    bool f_جدول() ثابت { واپس m_f_جدول; }
    bool fTaskifyAll() ثابت { واپس m_fTaskifyAll; }

    std::string trace_Class_بنیاد() ثابت VL_MT_محفوظ;  // Deprecated
    std::string traceClassLang() ثابت VL_MT_محفوظ;  // Deprecated
    std::سمتیہ<std::string> traceClassBases() ثابت VL_MT_محفوظ;
    std::سمتیہ<std::string> traceClassLangs() ثابت VL_MT_محفوظ;
    std::سمتیہ<std::string> trace_ماخذ_Bases() ثابت VL_MT_محفوظ;
    std::سمتیہ<std::string> trace_ماخذ_Langs() ثابت VL_MT_محفوظ;

    bool درجہ_وار() ثابت { واپس m_درجہ_وار; }
    صحیح hier_بچہ() ثابت VL_MT_محفوظ { واپس m_hier_بچہ; }
    صحیح hier_دھاگے() ثابت VL_MT_محفوظ { واپس m_hier_دھاگے == 0 ? m_دھاگے : m_hier_دھاگے; }
    bool hier_چوٹی() ثابت VL_MT_محفوظ { واپس !m_hier_بچہ && !m_hier_بلاکیں.خالی_خانے(); }
    ثابت V3_Hier_بلاک_Opt_سیٹ& hier_بلاکیں() ثابت { واپس m_hier_بلاکیں; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_چوٹی_ڈیٹا_Dir() ثابت VL_MT_محفوظ {
        واپس hier_چوٹی() ? (makeDir() + '/' + سابقہ() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ساکن string نسخہ() VL_خالص;
    ساکن string argString(صحیح argc, حرف** argv);  ///< Return list of arguments as simple string
    string all_دلائل_String() ثابت VL_MT_محفوظ;  ///< Return all passed arguments as simple string
    ثابت std::list<std::pair<std::list<std::string>, bool>>& all_دلائل() ثابت;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string all_دلائل_String_For_Hier_بلاک(bool for_چوٹی) ثابت;
    خالی parseOpts(File_سطر* fl, صحیح argc, حرف** argv) VL_MT_DISABLED;
    خالی parseOptsList(File_سطر* fl, ثابت string& optdir, صحیح argc, حرف** argv) VL_MT_DISABLED;
    خالی parseOptsFile(File_سطر* fl, ثابت string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    ساکن string getenvBuiltins(ثابت string& var);
    ساکن string getenvMAKE();
    ساکن string getenvMAKEFLAGS();
    ساکن string getenvPERL();
    ساکن string getenvPYTHON3();
    ساکن string getenvSYSTEMC();
    ساکن string getenvSYSTEMC_ARCH();
    ساکن string getenvSYSTEMC_INCLUDE();
    ساکن string getenvSYSTEMC_LIBDIR();
    ساکن string getenv_VERILATOR_جڑ();
    ساکن string getenvVERILATOR_SOLVER();
    ساکن string get_Std_Package_راستہ();
    ساکن string get_Std_Waiver_راستہ();
    ساکن string getSupported(ثابت string& var);
    ساکن bool systemCSystemWide();
    ساکن bool systemCFound();  // SystemC installed, or environment points to it
    ساکن bool coroutineSupport();  // Configured C++ compiler supports coroutines
    ساکن bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    ساکن bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    ساکن bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ثابت string& filename);
    string file_راستہ(File_سطر* fl, ثابت string& modname, ثابت string& lastpath,
                    ثابت string& errmsg);
    string file_راستہ_Looked_پیغام(File_سطر* fl, ثابت string& modname);
    V3_Lang_رمز fileLanguage(ثابت string& filename);
    ساکن bool file_Stat_معمول(ثابت string& filename);

    // METHODS (other OS)
    ساکن خالی throwSigsegv();
};

//######################################################################

#اگر_ختم  // guard
