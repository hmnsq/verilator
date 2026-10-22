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

#که_نه_تعریف_شوی VERILATOR_V3OPTIONS_H_
#تعریف_کړه VERILATOR_V3OPTIONS_H_

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
class File_کرښه;

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
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت نومونه[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        أرجع نومونه[m_e];
    }
    bool isDefault() ثابت { أرجع m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ثابت { أرجع m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool is_سېټ_True() ثابت { أرجع m_e == OPT_TRUE; }
    bool is_سېټ_False() ثابت { أرجع m_e == OPT_FALSE; }
    فراغ سېټ_True_Or_False(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool چلوونکی==(ثابت VOptionBool& lhs, ثابت VOptionBool& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VOptionBool& lhs, VOptionBool::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VOptionBool::en lhs, ثابت VOptionBool& rhs) { أرجع lhs == rhs.m_e; }

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
    string filename() ثابت { أرجع m_filename; }
    string libname() ثابت { أرجع m_libname; }
    bool چلوونکی==(ثابت VFileLibName& rhs) ثابت {
        أرجع m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool چلوونکی<(ثابت VFileLibName& rhs) ثابت {
        إذا (m_filename < rhs.m_filename) أرجع true;
        إذا (m_filename > rhs.m_filename) أرجع false;
        أرجع m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::ویکټور<VFileLibName>;
using VFile_Lib_سېټ = std::سېټ<VFileLibName>;

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
        ENUM_پای_ټکی
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
    VTimescale(ثابت string& ارزښت, bool& badr);
    VTimescale(مزدوج ارزښت, bool& badr) {
        badr = false;
        لكل (صحيح i = TS_100S; i < ENUM_پای_ټکی; ++i) {
            m_e = static_cast<en>(i);
            إذا (multiplier() == ارزښت) اكسر;
        }
        إذا (multiplier() != ارزښت) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ثابت { أرجع m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ساكن فراغ parseSlashed(File_کرښه* fl, ثابت حرف* textp, VTimescale& unitr, VTimescale& precr,
                             bool allow_تشه = false);
    ثابت حرف* ascii() ثابت {
        ساكن ثابت حرف* ثابت نومونه[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        أرجع نومونه[m_e];
    }
    صحيح power_د_Ten() ثابت { أرجع 2 - static_cast<صحيح>(m_e); }
    مزدوج multiplier() ثابت {
        ساكن ثابت مزدوج ارزښتونه[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        أرجع ارزښتونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VTimescale& lhs, VTimescale::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(VTimescale::en lhs, ثابت VTimescale& rhs) { أرجع lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool چلوونکی<(ثابت VTimescale& lhs, ثابت VTimescale& rhs) {
    أرجع lhs.m_e > rhs.m_e;
}
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VTimescale& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
عام:
    تعداد en : uint8_t {
        DEFAULT = 0,  // Use the global default expansion type
        AUTO,  // Expands only reset and default
        AUTO_EXPAND,  // Expands transitions between all possible states
        ډک,  // Cover all possible transitions
    } m_e;
    VFsmExpandType()
        : m_e{DEFAULT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFsmExpandType(en _e)
        : m_e{_e} {}
    explicit VFsmExpandType(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت { أرجع m_e; }
    constexpr ثابت حرف* ascii() ثابت {
        constexpr ثابت حرف* ثابت نومونه[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        أرجع نومونه[m_e];
    }
};
constexpr bool چلوونکی==(ثابت VFsmExpandType& lhs, ثابت VFsmExpandType& rhs) VL_MT_خوندي {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_خوندي {
    أرجع lhs.m_e == rhs;
}
constexpr bool چلوونکی==(VFsmExpandType::en lhs, ثابت VFsmExpandType& rhs) VL_MT_خوندي {
    أرجع lhs == rhs.m_e;
}
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت VFsmExpandType& rhs) VL_MT_خوندي {
    أرجع os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3_پوړیز_بلاک_Option final {
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
    explicit V3_پوړیز_بلاک_Option(ثابت string& optstring);
    ثابت string& origName() ثابت { أرجع m_origName; }
    ثابت string& mangledName() ثابت { أرجع m_mangledName; }
    ثابت ParamStrMap params() ثابت { أرجع m_parameters; }
};

using V3_Hier_بلاک_Opt_سېټ = std::map<ثابت std::string, V3_پوړیز_بلاک_Option>;

//######################################################################
// V3Options - Command line options

class V3Options final {
عام:
خاص:
    // TYPES
    using DebugLevelMap = std::map<ثابت std::string, بینښه>;

    // MEMBERS (general options)
    V3OptionsImp* m_impp;  // Slow hidden options

    // clang-format off
    VString_سېټ m_cppFiles;     // argument: C++ files to link against
    VStringList m_cFlags;      // argument: user CFLAGS
    VStringList m_ldLibs;      // argument: user LDFLAGS
    VStringList m_ldLibFiles;  // argument: library files, subset of m_ldLibs
    VStringList m_makeFlags;   // argument: user MAKEFLAGS
    VString_سېټ m_compilerIncludes; // argument: user --compiler-include
    VString_سېټ m_futures;      // argument: -Wfuture- list
    VString_سېټ m_future0s;     // argument: -future list
    VString_سېټ m_future1s;     // argument: -future1 list
    VFile_Lib_سېټ m_libraryFiles; // argument: Verilog -v files
    VFileLibList m_vFiles;      // argument: Verilog files to read
    VFile_Lib_سېټ m_vltFiles;     // argument: Verilator config files to read
    VString_سېټ m_libmapFiles;  // argument: --libmap files to read
    VStringList m_forceIncs;   // argument: -FI
    DebugLevelMap m_debugLevel; // argument: --debugi-<srcfile/tag> <level>
    DebugLevelMap m_dumpLevel;  // argument: --dumpi-<srcfile/tag> <level>
    std::map<ثابت string, string> m_parameters;  // Parameters
    std::map<ثابت string, V3_پوړیز_بلاک_Option> m_hier_بلاکونه;  // main switch: --hierarchical-block
    VString_سېټ m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_کرښه = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_دوګونی = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_بیان = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_کرښه = false;    // main switch: --coverage-block
    bool m_coverage_یوځای_Instances = false;  // main switch: --coverage-merge-instances
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
    bool m_debug_برخه_کول = false;  // main switch: --debug-partition
    bool m_debugPreprocPassthru = false;  // main switch: --debug-preproc-passthru
    bool m_debugProtect = false;    // main switch: --debug-protect
    bool m_debugSelfTest = false;   // main switch: --debug-self-test
    bool m_debug_ډېری_Check = false;  // main switch: --debug-stack-check
    صحيح m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
    bool m_debug_پلنوالی = false;      // main switch: --debug-width
    bool m_decoration = true;       // main switch: --decoration
    bool m_decorationNodes = false;  // main switch: --decoration=nodes
    bool m_diagnosticsSarif = false;  // main switch: --diagnostics-sarif
    bool m_dpiHdrOnly = false;      // main switch: --dpi-hdr-only
    bool m_emitAccessors = false;   // main switch: --emit-accessors
    bool m_exe = false;             // main switch: --exe
    bool m_flatten = false;         // main switch: --flatten
    bool m_fourstate = false;       // main switch: --fourstate
    bool m_پوړیز = false;    // main switch: --hierarchical
    bool m_ignc = false;            // main switch: --ignc
    bool m_jsonOnly = false;        // main switch: --json-only
    bool m_lintOnly = false;        // main switch: --lint-only
    bool m_gmake = false;           // main switch: --make gmake
    bool m_makeJson = false;        // main switch: --make json
    bool m_main = false;            // main switch: --main
    bool m_بهر_Format_Ok = false;     // main switch: --cc, --sc or --sp was specified
    bool m_pedantic = false;        // main switch: --Wpedantic
    bool m_pinsInoutEnables = false;// main switch: --pins-inout-enables
    bool m_pinsScUint = false;      // main switch: --pins-sc-uint
    bool m_pinsScUintBool = false;  // main switch: --pins-sc-uint-bool
    bool m_pinsScBigUint = false;   // main switch: --pins-sc-biguint
    bool m_pinsUint8 = false;       // main switch: --pins-uint8
    bool m_preprocComments = false;  // main switch: --preproc-comments
    bool m_preproc_تعریفوي = false;  // main switch: --preproc-defines
    bool m_profC = false;           // main switch: --prof-c
    bool m_profCFuncs = false;      // main switch: --prof-cfuncs
    bool m_profExec = false;        // main switch: --prof-exec
    bool m_profPgo = false;         // main switch: --prof-pgo
    bool m_protectIds = false;      // main switch: --protect-ids
    bool m_عامه = false;          // main switch: --public
    bool m_عامه_Flat_RW = false;    // main switch: --public-flat-rw
    bool m_عامه_Ignore = false;    // main switch: --public-ignore
    bool m_عامه_Params = false;    // main switch: --public-params
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
    bool m_تارونه_Coarsen = true;   // main switch: --threads-coarsen
    bool m_تارونه_Dpi_خالص = true;   // main switch: --threads-dpi all/pure
    bool m_تارونه_Dpi_Unpure = false;  // main switch: --threads-dpi all
    VOptionBool m_timing;           // main switch: --timing
    bool m_trace = false;           // main switch: --trace
    bool m_traceCoverage = false;   // main switch: --trace-coverage
    bool m_traceEnabledFst = false;  // main switch: --trace-fst
    bool m_traceEnabledSaif = false;  // main switch: --trace-saif
    bool m_traceEnabledVcd = false;  // main switch: --trace-vcd
    bool m_traceParams = true;      // main switch: --trace-params
    bool m_traceStructs = false;    // main switch: --trace-structs
    bool m_no_Trace_سر = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_x_Initial_څنډه = false;    // main switch: --x-initial-edge

    صحيح         m_buildJobs = -1;    // main switch: --build-jobs, -j
    صحيح         m_coverage_بیان_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    صحيح         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    صحيح         m_converge_حد = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    صحيح         m_coverage_Max_پلنوالی = 256; // main switch: --coverage-max-width
    صحيح         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    صحيح         m_expand_حد = 256;  // main switch: --expand-limit
    صحيح         m_gateStmts = 100;    // main switch: --gate-stmts
    صحيح         m_hier_ماشوم = 0;      // main switch: --hierarchical-child
    صحيح         m_hier_تارونه = 0;      // main switch: --hierarchical-threads
    صحيح         m_if_ژوروالی = 0;      // main switch: --if-depth
    صحيح         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    صحيح         m_inline_CFuncs_حاصل_ضرب = 200;   // main switch: --inline-cfuncs-product
    صحيح         m_inlineMult = 2000;   // main switch: --inline-mult
    صحيح         m_instr_شمېر_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    صحيح         m_localize_Max_اندازه = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    صحيح         m_max_عدد_پلنوالی = 65536;  // main switch: --max-num-width
    صحيح         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    صحيح         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    صحيح         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_output_Groups_اندازه = 512 * 1024 * 1024;  // main switch: --output-groups-size
    صحيح         m_output_وېشل = 20000;  // main switch: --output-split
    صحيح         m_output_وېشل_CFuncs = -1;  // main switch: --output-split-cfuncs
    صحيح         m_output_وېشل_CTrace = -1;  // main switch: --output-split-ctrace
    صحيح         m_pinsBv = 65;       // main switch: --pins-bv
    صحيح         m_preproc_Token_حد = 40000; // main switch: --preproc-token-limit
    صحيح         m_عامه_ژوروالی = 0;   // main switch: --public-depth
    صحيح         m_reloop_حد = 40; // main switch: --reloop-limit
    صحيح         m_replication_حد = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    صحيح         m_تارونه = 1;      // main switch: --threads
    صحيح         m_تارونه_Max_MTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_وخت_Default_Prec;  // main switch: --timescale
    VTimescale  m_وخت_Default_Unit;  // main switch: --timescale
    VTimescale  m_وخت_Override_Prec;  // main switch: --timescale-override
    VTimescale  m_وخت_Override_Unit;  // main switch: --timescale-override
    صحيح         m_trace_ژوروالی = 0;   // main switch: --trace-depth
    صحيح         m_traceMaxArray = 32;  // main switch: --trace-max-array
    صحيح         m_trace_Max_پلنوالی = 4096; // main switch: --trace-max-width
    صحيح         m_unroll_شمېر = 64;  // main switch: --unroll-count
    صحيح         m_unroll_حد = 16384;  // main switch: --unroll-limit
    صحيح         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    صحيح         m_constraint_Array_حد = 64;  // main switch: --constraint-array-limit
    صحيح         m_verilateJobs = -1;  // main switch: --verilate-jobs

    صحيح         m_comp_حد_بلاکونه = 0;  // compiler selection; number of nested blocks
    صحيح         m_comp_حد_Members = 64;  // compiler selection; number of members in struct before make anon array
    صحيح         m_comp_حد_Parens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_سر_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_mod_مختاړی;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_مختاړی;       // main switch: --prefix
    string      m_protect_کلی;   // main switch: --protect-key
    string      m_سر_Module;    // main switch: --top-module
    string      m_ناکارول_شوی_Regexp; // main switch: --unused-regexp
    string      m_waiverOutput;  // main switch: --waiver-output {filename}
    string      m_work = "work";  // main switch: --work {libname}
    string      m_x_ټاکل;      // main switch: --x-assign
    string      m_xInitial;     // main switch: --x-initial

    // Language is now held in FileLine, on a per-node basis. However we still
    // have a concept of the default language at a global level.
    V3_Lang_کوډ  m_defaultLanguage;      // main switch: --language

    // MEMBERS (optimizations)
    bool m_fAcycSimp;    // main switch: -fno-acyc-simp: acyclic pre-optimizations
    bool m_fAssemble;    // main switch: -fno-assemble: assign assemble
    bool m_f_بټ_لټون_Loops;  // main switch: -fno-bit-scan-loops: convert bit scan loops to builtins
    bool m_fBitblast;    // main switch: -fno-bitblast: split packed variables into bit ranges
    bool m_fCaseDecoder; // main switch: -fno-case-decoder: case decoder conversion
    bool m_f_Case_جدول;   // main switch: -fno-case-table: case table conversion
    bool m_f_Case_ونه;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_بټ_Op_ونه;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ټېل_ښکته_Sels = true; // main switch: -fno-dfg-push-down-sels
    bool m_fDfg;         // main switch: -fno-dfg
    bool m_fDfgSynthesizeAll = false;  // main switch: -fdfg-synthesize-all
    bool m_fDeadAssigns;     // main switch: -fno-dead-assigns: remove dead assigns
    bool m_fDeadCells;   // main switch: -fno-dead-cells: remove dead cells
    bool m_fDeadMethods;   // main switch: -fno-dead-methods: remove dead methods
    bool m_fDecomposePacked;  // main switch: -fno-decompose-packed: packed decomposition
    bool m_fDecomposeUnpacked;  // main switch: -fno-decompose-unpacked: unpacked decomposition
    bool m_fExpand;      // main switch: -fno-expand: expansion of C macros
    bool m_fFuncBalanceCat = true;  // main switch: -fno-func-balance-cat: expansion of C macros
    bool m_f_Func_وېشل_Cat = true;  // main switch: -fno-func-split-cat: expansion of C macros
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
    bool m_f_یوځای_Cond;   // main switch: -fno-merge-cond: merge conditionals
    bool m_f_یوځای_Cond_Motion = true; // main switch: -fno-merge-cond-motion: perform code motion
    bool m_fReloop;      // main switch: -fno-reloop: reform loops
    bool m_fReorder;     // main switch: -fno-reorder: reorder assignments in blocks
    bool m_f_ټوټه = true;  // main switch: -fno-slice: array assignment slicing
    صحيح  m_f_ټوټه_Element_حد = 256;  // main switch: --fslice-element-limit
    bool m_f_وېشل;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_f_جدول;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_شته = false;  // Set to true at the end of option parsing

    // METHODS
    فراغ جمع_کړه_Arg(حرف** argv, اندازه_t شمېر, bool isForRerun);
    فراغ جمع_کړه_Arg(ثابت std::string& arg, bool isForRerun);
    فراغ جمع_کړه_کرښه_Arg(ثابت string& arg);
    فراغ جمع_کړه_Define(ثابت string& defline, bool allowPlus) VL_MT_DISABLED;
    فراغ جمع_کړه_Future(ثابت string& flag);
    فراغ جمع_کړه_Future0(ثابت string& flag);
    فراغ جمع_کړه_Future1(ثابت string& flag);
    فراغ جمع_کړه_Inc_Dir_User(ثابت string& incdir);  // User requested
    فراغ جمع_کړه_Inc_Dir_Fallback(ثابت string& incdir);  // Low priority if not found otherwise
    فراغ جمع_کړه_Parameter(ثابت string& paramline, bool allowPlus);
    فراغ جمع_کړه_Lang_Ext(ثابت string& langext, ثابت V3_Lang_کوډ& lc);
    فراغ جمع_کړه_Lib_Ext_V(ثابت string& libext);
    فراغ optimize(صحيح level);
    فراغ show_نسخه(bool verbose);
    فراغ coverage(bool flag) {
        m_coverage_کرښه = m_coverageBranch = m_coverageToggle = m_coverage_بیان = m_coverageFsm
            = m_coverageUser = flag;
    }
    ساكن bool suffixed(ثابت string& sw, ثابت حرف* arg);
    ساكن string parseFileArg(ثابت string& optdir, ثابت string& relfilename);
    string file_لار_Check_One_Dir(ثابت string& modname, ثابت string& dirname);
    ساكن صحيح strip_Options_For_ماشوم_پرلپسې(ثابت string& opt, bool for_سر);
    فراغ validateIdentifier(File_کرښه* fl, ثابت string& arg, ثابت string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

عام:
    V3Options();
    ~V3Options();
    فراغ سېټ_Debug_ډېرپېښ(صحيح level);
    بینښه debugLevel(ثابت string& نښان) ثابت VL_MT_خوندي;
    بینښه debugSrcLevel(ثابت string& srcfile_لار) ثابت VL_MT_خوندي;
    بینښه dumpLevel(ثابت string& نښان) ثابت VL_MT_خوندي;
    بینښه dumpSrcLevel(ثابت string& srcfile_لار) ثابت VL_MT_خوندي;

    // METHODS
    فراغ جمع_کړه_Cpp_File(ثابت string& filename);
    فراغ جمع_کړه_CFlags(ثابت string& filename);
    فراغ جمع_کړه_Compiler_Includes(ثابت string& filename);
    فراغ جمع_کړه_Ld_Lib_File(ثابت string& filename);
    فراغ جمع_کړه_Ld_Libs(ثابت string& filename);
    فراغ جمع_کړه_Make_Flags(ثابت string& filename);
    فراغ جمع_کړه_Lib_Map_File(ثابت string& filename);
    فراغ جمع_کړه_Library_File(ثابت string& filename, ثابت string& libname);
    فراغ جمع_کړه_VFile(ثابت string& filename, ثابت string& libname);
    فراغ جمع_کړه_Vlt_File(ثابت string& filename, ثابت string& libname);
    فراغ جمع_کړه_Force_Inc(ثابت string& filename);
    bool شته() ثابت VL_MT_خوندي { أرجع m_شته; }
    فراغ cc_سېټ();
    فراغ decorations(File_کرښه* fl, ثابت string& filename);
    فراغ notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ثابت { أرجع m_preprocOnly; }
    bool makePhony() ثابت { أرجع m_makePhony; }
    bool preproc_No_کرښه() ثابت { أرجع m_preproc_No_کرښه; }
    bool preprocResolve() ثابت { أرجع m_preprocResolve; }
    صحيح preproc_Token_حد() ثابت { أرجع m_preproc_Token_حد; }
    bool underlineZero() ثابت { أرجع m_underlineZero; }
    bool systemC() ثابت VL_MT_خوندي { أرجع m_systemC; }
    bool savable() ثابت VL_MT_خوندي { أرجع m_savable; }
    VOptionBool schedZeroDelay() ثابت { أرجع m_schedZeroDelay; }
    bool stats() ثابت { أرجع m_stats; }
    bool statsVars() ثابت { أرجع m_statsVars; }
    bool stdPackage() ثابت { أرجع m_stdPackage; }
    bool stdWaiver() ثابت { أرجع m_stdWaiver; }
    bool assertOn() ثابت { أرجع m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ثابت { أرجع m_assertCase; }
    bool autoflush() ثابت { أرجع m_autoflush; }
    bool bboxSys() ثابت { أرجع m_bboxSys; }
    bool bboxUnsup() ثابت { أرجع m_bboxUnsup; }
    bool build() ثابت { أرجع m_build; }
    string buildDepBin() ثابت { أرجع m_buildDepBin; }
    فراغ buildDepBin(ثابت string& flag) { m_buildDepBin = flag; }
    bool context() ثابت VL_MT_خوندي { أرجع m_context; }
    bool coverage() ثابت VL_MT_خوندي {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        أرجع m_coverage_کرښه || m_coverageBranch || m_coverageToggle || m_coverage_بیان
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ثابت VL_MT_خوندي {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        أرجع m_coverage_کرښه || m_coverageBranch || m_coverageToggle || m_coverage_بیان
               || m_coverageUser;
    }
    bool coverageBranch() ثابت { أرجع m_coverageBranch; }
    bool coverage_بیان() ثابت { أرجع m_coverage_بیان; }
    bool coverageFsm() ثابت { أرجع m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ثابت { أرجع m_coverageFsmExpand; }
    صحيح coverageFsmMaxArcs() ثابت { أرجع m_coverageFsmMaxArcs; }
    bool coverage_کرښه() ثابت { أرجع m_coverage_کرښه; }
    bool coverage_یوځای_Instances() ثابت { أرجع m_coverage_یوځای_Instances; }
    bool coverage_فی_Instance() ثابت { أرجع m_coverage_فی_Instance; }
    bool coverageToggle() ثابت { أرجع m_coverageToggle; }
    bool coverageUnderscore() ثابت { أرجع m_coverageUnderscore; }
    bool coverageUser() ثابت { أرجع m_coverageUser; }
    bool debugCheck() ثابت VL_MT_خوندي { أرجع m_debugCheck; }
    bool debugCollision() ثابت { أرجع m_debugCollision; }
    bool debugEmitV() ثابت VL_MT_خوندي { أرجع m_debugEmitV; }
    bool debugExitElab() ثابت { أرجع m_debugExitElab; }
    bool debugExitParse() ثابت { أرجع m_debugExitParse; }
    bool debugLeak() ثابت { أرجع m_debugLeak; }
    bool debugNondeterminism() ثابت { أرجع m_debugNondeterminism; }
    bool debug_برخه_کول() ثابت { أرجع m_debug_برخه_کول; }
    bool debugPreprocPassthru() ثابت VL_MT_خوندي { أرجع m_debugPreprocPassthru; }
    bool debugProtect() ثابت VL_MT_خوندي { أرجع m_debugProtect; }
    صحيح debugRuntimeTimeout() ثابت { أرجع m_debugRuntimeTimeout; }
    bool debugSelfTest() ثابت { أرجع m_debugSelfTest; }
    bool debug_ډېری_Check() ثابت { أرجع m_debug_ډېری_Check; }
    bool debug_پلنوالی() ثابت VL_خالص { أرجع m_debug_پلنوالی; }
    bool decoration() ثابت VL_MT_خوندي { أرجع m_decoration; }
    bool decorationNodes() ثابت VL_MT_خوندي { أرجع m_decorationNodes; }
    bool diagnosticsSarif() ثابت VL_MT_خوندي { أرجع m_diagnosticsSarif; }
    bool dpiHdrOnly() ثابت { أرجع m_dpiHdrOnly; }
    bool dumpAstPatterns() ثابت {
        أرجع m_dumpLevel.شمېر("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dump_تعریفوي() ثابت { أرجع m_dumpLevel.شمېر("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ثابت {
        أرجع m_dumpLevel.شمېر("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_ونه_Dot() ثابت {
        أرجع m_dumpLevel.شمېر("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ثابت { أرجع m_emitAccessors; }
    bool exe() ثابت { أرجع m_exe; }
    bool flatten() ثابت { أرجع m_flatten; }
    bool fourstate() ثابت { أرجع m_fourstate; }
    bool gmake() ثابت { أرجع m_gmake; }
    bool makeJson() ثابت { أرجع m_makeJson; }
    bool تارونه_Dpi_خالص() ثابت { أرجع m_تارونه_Dpi_خالص; }
    bool تارونه_Dpi_Unpure() ثابت { أرجع m_تارونه_Dpi_Unpure; }
    bool تارونه_Coarsen() ثابت { أرجع m_تارونه_Coarsen; }
    VOptionBool timing() ثابت { أرجع m_timing; }
    bool trace() ثابت { أرجع m_trace; }
    bool traceCoverage() ثابت { أرجع m_traceCoverage; }
    bool traceEnabledFst() ثابت { أرجع m_traceEnabledFst; }
    bool traceEnabledSaif() ثابت { أرجع m_traceEnabledSaif; }
    bool traceEnabledVcd() ثابت { أرجع m_traceEnabledVcd; }
    bool traceParams() ثابت { أرجع m_traceParams; }
    bool traceStructs() ثابت { أرجع m_traceStructs; }
    bool traceUnderscore() ثابت { أرجع m_traceUnderscore; }
    bool اصلي() ثابت { أرجع m_main; }
    bool بهر_Format_Ok() ثابت { أرجع m_بهر_Format_Ok; }
    bool jsonOnly() ثابت { أرجع m_jsonOnly; }
    bool keep_لنډمهاله_Files() ثابت { أرجع (V3Error::debugDefault() != 0); }
    bool pedantic() ثابت { أرجع m_pedantic; }
    bool pinsInoutEnables() ثابت { أرجع m_pinsInoutEnables; }
    bool pinsScUint() ثابت { أرجع m_pinsScUint; }
    bool pinsScUintBool() ثابت { أرجع m_pinsScUintBool; }
    bool pinsScBigUint() ثابت VL_MT_خوندي { أرجع m_pinsScBigUint; }
    bool pinsUint8() ثابت { أرجع m_pinsUint8; }
    bool preprocComments() ثابت { أرجع m_preprocComments; }
    bool preproc_تعریفوي() ثابت { أرجع m_preproc_تعریفوي; }
    bool profC() ثابت { أرجع m_profC; }
    bool profCFuncs() ثابت { أرجع m_profCFuncs; }
    bool profExec() ثابت { أرجع m_profExec; }
    bool profPgo() ثابت { أرجع m_profPgo; }
    bool کاروي_Profiler() ثابت { أرجع profExec() || profPgo(); }
    bool protectIds() ثابت VL_MT_خوندي { أرجع m_protectIds; }
    bool all_عامه() ثابت { أرجع m_عامه; }
    bool عامه_Params() ثابت { أرجع m_عامه_Params; }
    bool عامه_Off() ثابت { أرجع m_عامه_Ignore; }
    bool عامه_Flat_RW() ثابت { أرجع m_عامه_Flat_RW; }
    صحيح عامه_ژوروالی() ثابت { أرجع m_عامه_ژوروالی; }
    bool any_عامه_Flat() ثابت { أرجع m_عامه_Params || m_عامه_Flat_RW || m_عامه_ژوروالی; }
    bool lintOnly() ثابت VL_MT_خوندي { أرجع m_lintOnly; }
    bool ignc() ثابت { أرجع m_ignc; }
    bool quietBuild() ثابت VL_MT_خوندي { أرجع m_quietBuild; }
    bool quietExit() ثابت VL_MT_خوندي { أرجع m_quietExit; }
    bool quietStats() ثابت VL_MT_خوندي { أرجع m_quietStats; }
    bool reportUnoptflat() ثابت { أرجع m_reportUnoptflat; }
    bool verilate() ثابت { أرجع m_verilate; }
    bool vpi() ثابت { أرجع m_vpi.isTrue(); }
    bool waiverMultiline() ثابت { أرجع m_waiverMultiline; }
    bool x_Initial_څنډه() ثابت { أرجع m_x_Initial_څنډه; }
    bool serializeOnly() ثابت { أرجع m_jsonOnly; }
    bool سر_Ifaces_Supported() ثابت { أرجع lintOnly() && !پوړیز(); }

    صحيح buildJobs() ثابت VL_MT_خوندي { أرجع m_buildJobs; }
    صحيح converge_حد() ثابت { أرجع m_converge_حد; }
    صحيح coverage_بیان_Max() ثابت { أرجع m_coverage_بیان_Max; }
    uint32_t coverageMaxBins() ثابت { أرجع m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ثابت { أرجع m_coverageMaxRealBins; }
    صحيح coverage_Max_پلنوالی() ثابت { أرجع m_coverage_Max_پلنوالی; }
    صحيح debugAllocRandom() ثابت { أرجع m_debugAllocRandom; }
    bool dump_ونه_Addrids() ثابت VL_MT_خوندي;
    صحيح expand_حد() ثابت { أرجع m_expand_حد; }
    صحيح gateStmts() ثابت { أرجع m_gateStmts; }
    صحيح if_ژوروالی() ثابت { أرجع m_if_ژوروالی; }
    صحيح inlineCFuncs() ثابت { أرجع m_inlineCFuncs; }
    صحيح inline_CFuncs_حاصل_ضرب() ثابت { أرجع m_inline_CFuncs_حاصل_ضرب; }
    صحيح inlineMult() ثابت { أرجع m_inlineMult; }
    صحيح instr_شمېر_Dpi() ثابت { أرجع m_instr_شمېر_Dpi; }
    صحيح localize_Max_اندازه() ثابت { أرجع m_localize_Max_اندازه; }
    bool jsonEditNums() ثابت { أرجع m_jsonEditNums; }
    bool jsonIds() ثابت { أرجع m_jsonIds; }
    VOptionBool makeDepend() ثابت { أرجع m_makeDepend; }
    صحيح max_عدد_پلنوالی() ثابت { أرجع m_max_عدد_پلنوالی; }
    صحيح func_Recursion_ژوروالی() ثابت { أرجع m_funcRecursion; }
    صحيح module_Recursion_ژوروالی() ثابت { أرجع m_moduleRecursion; }
    صحيح output_وېشل() ثابت { أرجع m_output_وېشل; }
    صحيح output_وېشل_CFuncs() ثابت { أرجع m_output_وېشل_CFuncs; }
    صحيح output_وېشل_CTrace() ثابت { أرجع m_output_وېشل_CTrace; }
    صحيح outputGroups() ثابت { أرجع m_outputGroups; }
    int64_t output_Groups_اندازه() ثابت { أرجع m_output_Groups_اندازه; }
    صحيح pinsBv() ثابت VL_MT_خوندي { أرجع m_pinsBv; }
    صحيح reloop_حد() ثابت { أرجع m_reloop_حد; }
    صحيح replication_حد() ثابت { أرجع m_replication_حد; }
    VOptionBool skipIdentical() ثابت { أرجع m_skipIdentical; }
    bool stopFail() ثابت { أرجع m_stopFail; }
    صحيح تارونه() ثابت VL_MT_خوندي { أرجع m_تارونه; }
    صحيح تارونه_Max_MTasks() ثابت { أرجع m_تارونه_Max_MTasks; }
    bool mtasks() ثابت VL_MT_خوندي { أرجع (m_تارونه > 1); }
    VTimescale وخت_Default_Prec() ثابت { أرجع m_وخت_Default_Prec; }
    VTimescale وخت_Default_Unit() ثابت { أرجع m_وخت_Default_Unit; }
    VTimescale وخت_Override_Prec() ثابت { أرجع m_وخت_Override_Prec; }
    VTimescale وخت_Override_Unit() ثابت { أرجع m_وخت_Override_Unit; }
    VTimescale وخت_Compute_Prec(ثابت VTimescale& flag) ثابت;
    VTimescale وخت_Compute_Unit(ثابت VTimescale& flag) ثابت;
    صحيح trace_ژوروالی() ثابت { أرجع m_trace_ژوروالی; }
    صحيح traceMaxArray() ثابت { أرجع m_traceMaxArray; }
    صحيح trace_Max_پلنوالی() ثابت { أرجع m_trace_Max_پلنوالی; }
    bool useTraceParallel() ثابت {
        أرجع trace() && traceEnabledVcd() && (تارونه() > 1 || hier_ماشوم() > 1);
    }
    صحيح unroll_شمېر() ثابت { أرجع m_unroll_شمېر; }
    صحيح unroll_حد() ثابت { أرجع m_unroll_حد; }
    صحيح unrollStmts() ثابت { أرجع m_unrollStmts; }
    صحيح constraint_Array_حد() ثابت { أرجع m_constraint_Array_حد; }
    صحيح verilateJobs() ثابت { أرجع m_verilateJobs; }

    صحيح comp_حد_بلاکونه() ثابت { أرجع m_comp_حد_بلاکونه; }
    صحيح comp_حد_Members() ثابت VL_MT_خوندي { أرجع m_comp_حد_Members; }
    صحيح comp_حد_Parens() ثابت { أرجع m_comp_حد_Parens; }

    string diagnosticsSarifOutput() ثابت VL_MT_خوندي {
        أرجع m_diagnosticsSarifOutput.تشه() ? makeDir() + "/" + مختاړی() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ثابت { أرجع m_exeName != "" ? m_exeName : مختاړی(); }
    VFileLibList hierParamFile() ثابت { أرجع m_hierParamsFile; }
    string jsonOnlyOutput() ثابت { أرجع m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ثابت { أرجع m_jsonOnlyMetaOutput; }
    string l2Name() ثابت { أرجع m_l2Name; }
    string libCreate() ثابت { أرجع m_libCreate; }
    string libCreateName(bool shared) ثابت {
        string libName = "lib" + libCreate();
        إذا (shared) {
            libName += ".so";
        } وإلا {
            libName += ".a";
        }
        أرجع libName;
    }
    string main_سر_Name() ثابت { أرجع m_main_سر_Name; }
    string makeDir() ثابت VL_MT_خوندي { أرجع m_makeDir; }
    string mod_مختاړی() ثابت VL_MT_خوندي { أرجع m_mod_مختاړی; }
    string pipeFilter() ثابت { أرجع m_pipeFilter; }
    string مختاړی() ثابت VL_MT_خوندي { أرجع m_مختاړی; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_کلی_Provided() ثابت { أرجع !m_protect_کلی.تشه(); }
    string protect_کلی_Defaulted() VL_MT_خوندي;  // Set default key if not set by user
    string سر_Module() ثابت { أرجع m_سر_Module; }  // As AstNode::encodeName()
    bool no_Trace_سر() ثابت { أرجع m_no_Trace_سر; }
    string ناکارول_شوی_Regexp() ثابت { أرجع m_ناکارول_شوی_Regexp; }
    string waiverOutput() ثابت { أرجع m_waiverOutput; }
    string work() ثابت { أرجع m_work; }
    bool isWaiverOutput() ثابت { أرجع !m_waiverOutput.تشه(); }
    string x_ټاکل() ثابت { أرجع m_x_ټاکل; }
    string xInitial() ثابت { أرجع m_xInitial; }

    ثابت VString_سېټ& cppFiles() ثابت { أرجع m_cppFiles; }
    ثابت VStringList& cFlags() ثابت { أرجع m_cFlags; }
    ثابت VString_سېټ& compilerIncludes() ثابت { أرجع m_compilerIncludes; }
    ثابت VStringList& ldLibFiles() ثابت { أرجع m_ldLibFiles; }
    ثابت VStringList& ldLibs() ثابت { أرجع m_ldLibs; }
    ثابت VStringList& makeFlags() ثابت { أرجع m_makeFlags; }
    ثابت VFile_Lib_سېټ& libraryFiles() ثابت { أرجع m_libraryFiles; }
    ثابت VString_سېټ& libmapFiles() ثابت { أرجع m_libmapFiles; }
    ثابت VFileLibList& vFiles() ثابت { أرجع m_vFiles; }
    ثابت VFile_Lib_سېټ& vltFiles() ثابت { أرجع m_vltFiles; }
    ثابت VStringList& forceIncs() ثابت { أرجع m_forceIncs; }

    bool hasParameter(ثابت string& name);
    string parameter(ثابت string& name);
    فراغ checkParameters();

    bool isFuture(ثابت string& flag) ثابت;
    bool isFuture0(ثابت string& flag) ثابت;
    bool isFuture1(ثابت string& flag) ثابت;
    bool isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ثابت { أرجع m_fAcycSimp; }
    bool fAssemble() ثابت { أرجع m_fAssemble; }
    bool f_بټ_لټون_Loops() ثابت { أرجع m_f_بټ_لټون_Loops; }
    bool fBitblast() ثابت { أرجع m_fBitblast; }
    bool fCaseDecoder() ثابت { أرجع m_fCaseDecoder; }
    bool f_Case_جدول() ثابت { أرجع m_f_Case_جدول; }
    bool f_Case_ونه() ثابت { أرجع m_f_Case_ونه; }
    bool fCombine() ثابت { أرجع m_fCombine; }
    bool fConst() ثابت { أرجع m_fConst; }
    bool fConstBeforeDfg() ثابت { أرجع m_fConstBeforeDfg; }
    bool f_Const_بټ_Op_ونه() ثابت { أرجع m_f_Const_بټ_Op_ونه; }
    bool fConstEager() ثابت { أرجع m_fConstEager; }
    bool fDedupe() ثابت { أرجع m_fDedupe; }
    bool fDfg() ثابت { أرجع m_fDfg; }
    bool fDfgPeephole() ثابت { أرجع m_fDfgPeephole; }
    bool f_Dfg_ټېل_ښکته_Sels() ثابت { أرجع m_f_Dfg_ټېل_ښکته_Sels; }
    bool fDfgSynthesizeAll() ثابت { أرجع m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ثابت std::string& name) ثابت {
        أرجع !m_fDfgPeepholeDisabled.شمېر(name);
    }
    bool fDeadAssigns() ثابت { أرجع m_fDeadAssigns; }
    bool fDeadCells() ثابت { أرجع m_fDeadCells; }
    bool fDeadMethods() ثابت { أرجع m_fDeadMethods; }
    bool fDecompose() ثابت { أرجع m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ثابت { أرجع m_fDecomposePacked; }
    bool fDecomposeUnpacked() ثابت { أرجع m_fDecomposeUnpacked; }
    bool fExpand() ثابت { أرجع m_fExpand; }
    bool fFuncBalanceCat() ثابت { أرجع m_fFuncBalanceCat; }
    bool f_Func_وېشل_Cat() ثابت { أرجع m_f_Func_وېشل_Cat; }
    bool fFunc() ثابت { أرجع f_Func_وېشل_Cat() || fFuncBalanceCat(); }
    bool fGate() ثابت { أرجع m_fGate; }
    VOptionBool fIcoChangeDetect() ثابت { أرجع m_fIcoChangeDetect; }
    bool fInline() ثابت { أرجع m_fInline; }
    bool fInlineCFuncs() ثابت { أرجع m_fInlineCFuncs; }
    bool fInlineFuncs() ثابت { أرجع m_fInlineFuncs; }
    bool fInlineFuncsEager() ثابت { أرجع m_fInlineFuncsEager; }
    bool fLife() ثابت { أرجع m_fLife; }
    bool fLifePost() ثابت { أرجع m_fLifePost; }
    bool fLocalize() ثابت { أرجع m_fLocalize; }
    bool f_یوځای_Cond() ثابت { أرجع m_f_یوځای_Cond; }
    bool f_یوځای_Cond_Motion() ثابت { أرجع m_f_یوځای_Cond_Motion; }
    bool fReloop() ثابت { أرجع m_fReloop; }
    bool fReorder() ثابت { أرجع m_fReorder; }
    bool f_ټوټه() ثابت { أرجع m_f_ټوټه; }
    صحيح f_ټوټه_Element_حد() ثابت { أرجع m_f_ټوټه_Element_حد; }
    bool f_وېشل() ثابت { أرجع m_f_وېشل; }
    bool fSubst() ثابت { أرجع m_fSubst; }
    bool fSubstConst() ثابت { أرجع m_fSubstConst; }
    bool f_جدول() ثابت { أرجع m_f_جدول; }
    bool fTaskifyAll() ثابت { أرجع m_fTaskifyAll; }

    std::string trace_Class_بنسټ() ثابت VL_MT_خوندي;  // Deprecated
    std::string traceClassLang() ثابت VL_MT_خوندي;  // Deprecated
    std::ویکټور<std::string> traceClassBases() ثابت VL_MT_خوندي;
    std::ویکټور<std::string> traceClassLangs() ثابت VL_MT_خوندي;
    std::ویکټور<std::string> trace_سرچینه_Bases() ثابت VL_MT_خوندي;
    std::ویکټور<std::string> trace_سرچینه_Langs() ثابت VL_MT_خوندي;

    bool پوړیز() ثابت { أرجع m_پوړیز; }
    صحيح hier_ماشوم() ثابت VL_MT_خوندي { أرجع m_hier_ماشوم; }
    صحيح hier_تارونه() ثابت VL_MT_خوندي { أرجع m_hier_تارونه == 0 ? m_تارونه : m_hier_تارونه; }
    bool hier_سر() ثابت VL_MT_خوندي { أرجع !m_hier_ماشوم && !m_hier_بلاکونه.تشه(); }
    ثابت V3_Hier_بلاک_Opt_سېټ& hier_بلاکونه() ثابت { أرجع m_hier_بلاکونه; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_سر_معلومات_Dir() ثابت VL_MT_خوندي {
        أرجع hier_سر() ? (makeDir() + '/' + مختاړی() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ساكن string نسخه() VL_خالص;
    ساكن string argString(صحيح argc, حرف** argv);  ///< Return list of arguments as simple string
    string all_دلیلونه_String() ثابت VL_MT_خوندي;  ///< Return all passed arguments as simple string
    ثابت std::list<std::pair<std::list<std::string>, bool>>& all_دلیلونه() ثابت;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string all_دلیلونه_String_For_Hier_بلاک(bool for_سر) ثابت;
    فراغ parseOpts(File_کرښه* fl, صحيح argc, حرف** argv) VL_MT_DISABLED;
    فراغ parseOptsList(File_کرښه* fl, ثابت string& optdir, صحيح argc, حرف** argv) VL_MT_DISABLED;
    فراغ parseOptsFile(File_کرښه* fl, ثابت string& filename, bool rel) VL_MT_DISABLED;

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
    ساكن string getenv_VERILATOR_ریښه();
    ساكن string getenvVERILATOR_SOLVER();
    ساكن string get_Std_Package_لار();
    ساكن string get_Std_Waiver_لار();
    ساكن string getSupported(ثابت string& var);
    ساكن bool systemCSystemWide();
    ساكن bool systemCFound();  // SystemC installed, or environment points to it
    ساكن bool coroutineSupport();  // Configured C++ compiler supports coroutines
    ساكن bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    ساكن bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    ساكن bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ثابت string& filename);
    string file_لار(File_کرښه* fl, ثابت string& modname, ثابت string& lastpath,
                    ثابت string& errmsg);
    string file_لار_Looked_پیغام(File_کرښه* fl, ثابت string& modname);
    V3_Lang_کوډ fileLanguage(ثابت string& filename);
    ساكن bool file_Stat_نورمال(ثابت string& filename);

    // METHODS (other OS)
    ساكن فراغ throwSigsegv();
};

//######################################################################

#د_که_پای  // guard
