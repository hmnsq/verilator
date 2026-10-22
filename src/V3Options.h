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

#నిర్వచితం_కాకపోతే VERILATOR_V3OPTIONS_H_
#నిర్వచించు VERILATOR_V3OPTIONS_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3LangCode.h"

#అన్తర్భావ <list>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <string>
#అన్తర్భావ <vector>

class V3OptionsImp;
class File_పంక్తి;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
బహిరంగ:
    గణన en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    గణన en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        తిరిగి పేర్లు[m_e];
    }
    bool isDefault() స్థిరరాశి { తిరిగి m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() స్థిరరాశి { తిరిగి m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool is_సమితి_True() స్థిరరాశి { తిరిగి m_e == OPT_TRUE; }
    bool is_సమితి_False() స్థిరరాశి { తిరిగి m_e == OPT_FALSE; }
    శూన్య సమితి_True_Or_False(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VOptionBool& lhs, స్థిరరాశి VOptionBool& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VOptionBool& lhs, VOptionBool::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VOptionBool::en lhs, స్థిరరాశి VOptionBool& rhs) { తిరిగి lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    స్థిరరాశి string m_filename;  // Filename
    స్థిరరాశి string m_libname;  // Libname
బహిరంగ:
    VFileLibName(స్థిరరాశి string& filename, స్థిరరాశి string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(స్థిరరాశి VFileLibName& rhs) = అప్రమేయ;
    string filename() స్థిరరాశి { తిరిగి m_filename; }
    string libname() స్థిరరాశి { తిరిగి m_libname; }
    bool క్రియాకారకం==(స్థిరరాశి VFileLibName& rhs) స్థిరరాశి {
        తిరిగి m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool క్రియాకారకం<(స్థిరరాశి VFileLibName& rhs) స్థిరరాశి {
        ఒకవేళ (m_filename < rhs.m_filename) తిరిగి true;
        ఒకవేళ (m_filename > rhs.m_filename) తిరిగి false;
        తిరిగి m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::సదిశ<VFileLibName>;
using VFile_Lib_సమితి = std::సమితి<VFileLibName>;

// ######################################################################

class VTimescale final {
బహిరంగ:
    గణన en : uint8_t {
        // clang-format off
        TS_100S = 0, TS_10S = 1, TS_1S = 2,
        TS_100MS = 3, TS_10MS = 4, TS_1MS = 5,
        TS_100US = 6, TS_10US = 7, TS_1US = 8,
        TS_100NS = 9, TS_10NS = 10, TS_1NS = 11,
        TS_100PS = 12, TS_10PS = 13, TS_1PS = 14,
        TS_100FS = 15, TS_10FS = 16, TS_1FS = 17,
        // clang-format on
        NONE = 18,
        ENUM_చివర
    };
    గణన : uint8_t { TS_DEFAULT = TS_1PS };
    గణన en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(స్థిరరాశి string& విలువ, bool& badr);
    VTimescale(ద్విగుణ విలువ, bool& badr) {
        badr = false;
        కొరకు (పూర్ణాంకం i = TS_100S; i < ENUM_చివర; ++i) {
            m_e = static_cast<en>(i);
            ఒకవేళ (multiplier() == విలువ) ఆపు;
        }
        ఒకవేళ (multiplier() != విలువ) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() స్థిరరాశి { తిరిగి m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    స్థిర శూన్య parseSlashed(File_పంక్తి* fl, స్థిరరాశి అక్షరం* textp, VTimescale& unitr, VTimescale& precr,
                             bool allow_ఖాళీలు = false);
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        తిరిగి పేర్లు[m_e];
    }
    పూర్ణాంకం power_యొక్క_Ten() స్థిరరాశి { తిరిగి 2 - static_cast<పూర్ణాంకం>(m_e); }
    ద్విగుణ multiplier() స్థిరరాశి {
        స్థిర స్థిరరాశి ద్విగుణ విలువలు[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        తిరిగి విలువలు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VTimescale& lhs, స్థిరరాశి VTimescale& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VTimescale& lhs, VTimescale::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(VTimescale::en lhs, స్థిరరాశి VTimescale& rhs) { తిరిగి lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool క్రియాకారకం<(స్థిరరాశి VTimescale& lhs, స్థిరరాశి VTimescale& rhs) {
    తిరిగి lhs.m_e > rhs.m_e;
}
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VTimescale& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
బహిరంగ:
    గణన en : uint8_t {
        DEFAULT = 0,  // Use the global default expansion type
        AUTO,  // Expands only reset and default
        AUTO_EXPAND,  // Expands transitions between all possible states
        నిండిన,  // Cover all possible transitions
    } m_e;
    VFsmExpandType()
        : m_e{DEFAULT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFsmExpandType(en _e)
        : m_e{_e} {}
    explicit VFsmExpandType(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    constexpr స్థిరరాశి అక్షరం* ascii() స్థిరరాశి {
        constexpr స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        తిరిగి పేర్లు[m_e];
    }
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VFsmExpandType& lhs, స్థిరరాశి VFsmExpandType& rhs) VL_MT_సురక్షిత {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_సురక్షిత {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VFsmExpandType::en lhs, స్థిరరాశి VFsmExpandType& rhs) VL_MT_సురక్షిత {
    తిరిగి lhs == rhs.m_e;
}
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి VFsmExpandType& rhs) VL_MT_సురక్షిత {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3_క్రమానుగత_ఖండం_Option final {
బహిరంగ:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<స్థిరరాశి std::string, std::string>;

అంతరంగ:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

బహిరంగ:
    explicit V3_క్రమానుగత_ఖండం_Option(స్థిరరాశి string& optstring);
    స్థిరరాశి string& origName() స్థిరరాశి { తిరిగి m_origName; }
    స్థిరరాశి string& mangledName() స్థిరరాశి { తిరిగి m_mangledName; }
    స్థిరరాశి ParamStrMap params() స్థిరరాశి { తిరిగి m_parameters; }
};

using V3_Hier_ఖండం_Opt_సమితి = std::map<స్థిరరాశి std::string, V3_క్రమానుగత_ఖండం_Option>;

//######################################################################
// V3Options - Command line options

class V3Options final {
బహిరంగ:
అంతరంగ:
    // TYPES
    using DebugLevelMap = std::map<స్థిరరాశి std::string, గుర్తులేని>;

    // MEMBERS (general options)
    V3OptionsImp* m_impp;  // Slow hidden options

    // clang-format off
    VString_సమితి m_cppFiles;     // argument: C++ files to link against
    VStringList m_cFlags;      // argument: user CFLAGS
    VStringList m_ldLibs;      // argument: user LDFLAGS
    VStringList m_ldLibFiles;  // argument: library files, subset of m_ldLibs
    VStringList m_makeFlags;   // argument: user MAKEFLAGS
    VString_సమితి m_compilerIncludes; // argument: user --compiler-include
    VString_సమితి m_futures;      // argument: -Wfuture- list
    VString_సమితి m_future0s;     // argument: -future list
    VString_సమితి m_future1s;     // argument: -future1 list
    VFile_Lib_సమితి m_libraryFiles; // argument: Verilog -v files
    VFileLibList m_vFiles;      // argument: Verilog files to read
    VFile_Lib_సమితి m_vltFiles;     // argument: Verilator config files to read
    VString_సమితి m_libmapFiles;  // argument: --libmap files to read
    VStringList m_forceIncs;   // argument: -FI
    DebugLevelMap m_debugLevel; // argument: --debugi-<srcfile/tag> <level>
    DebugLevelMap m_dumpLevel;  // argument: --dumpi-<srcfile/tag> <level>
    std::map<స్థిరరాశి string, string> m_parameters;  // Parameters
    std::map<స్థిరరాశి string, V3_క్రమానుగత_ఖండం_Option> m_hier_ఖండాలు;  // main switch: --hierarchical-block
    VString_సమితి m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_పంక్తి = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_ద్విమాన = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_సమాసం = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_పంక్తి = false;    // main switch: --coverage-block
    bool m_coverage_విలీనం_Instances = false;  // main switch: --coverage-merge-instances
    bool m_coverage_చొప్పున_Instance = false;  // main switch: --coverage-per-instance
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
    bool m_debug_విభజన = false;  // main switch: --debug-partition
    bool m_debugPreprocPassthru = false;  // main switch: --debug-preproc-passthru
    bool m_debugProtect = false;    // main switch: --debug-protect
    bool m_debugSelfTest = false;   // main switch: --debug-self-test
    bool m_debug_దొంతర_Check = false;  // main switch: --debug-stack-check
    పూర్ణాంకం m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
    bool m_debug_వెడల్పు = false;      // main switch: --debug-width
    bool m_decoration = true;       // main switch: --decoration
    bool m_decorationNodes = false;  // main switch: --decoration=nodes
    bool m_diagnosticsSarif = false;  // main switch: --diagnostics-sarif
    bool m_dpiHdrOnly = false;      // main switch: --dpi-hdr-only
    bool m_emitAccessors = false;   // main switch: --emit-accessors
    bool m_exe = false;             // main switch: --exe
    bool m_flatten = false;         // main switch: --flatten
    bool m_fourstate = false;       // main switch: --fourstate
    bool m_క్రమానుగత = false;    // main switch: --hierarchical
    bool m_ignc = false;            // main switch: --ignc
    bool m_jsonOnly = false;        // main switch: --json-only
    bool m_lintOnly = false;        // main switch: --lint-only
    bool m_gmake = false;           // main switch: --make gmake
    bool m_makeJson = false;        // main switch: --make json
    bool m_main = false;            // main switch: --main
    bool m_బహిర్గతం_Format_Ok = false;     // main switch: --cc, --sc or --sp was specified
    bool m_pedantic = false;        // main switch: --Wpedantic
    bool m_pinsInoutEnables = false;// main switch: --pins-inout-enables
    bool m_pinsScUint = false;      // main switch: --pins-sc-uint
    bool m_pinsScUintBool = false;  // main switch: --pins-sc-uint-bool
    bool m_pinsScBigUint = false;   // main switch: --pins-sc-biguint
    bool m_pinsUint8 = false;       // main switch: --pins-uint8
    bool m_preprocComments = false;  // main switch: --preproc-comments
    bool m_preproc_నిర్వచిస్తుంది = false;  // main switch: --preproc-defines
    bool m_profC = false;           // main switch: --prof-c
    bool m_profCFuncs = false;      // main switch: --prof-cfuncs
    bool m_profExec = false;        // main switch: --prof-exec
    bool m_profPgo = false;         // main switch: --prof-pgo
    bool m_protectIds = false;      // main switch: --protect-ids
    bool m_ప్రజా = false;          // main switch: --public
    bool m_ప్రజా_Flat_RW = false;    // main switch: --public-flat-rw
    bool m_ప్రజా_Ignore = false;    // main switch: --public-ignore
    bool m_ప్రజా_Params = false;    // main switch: --public-params
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
    bool m_దారాలు_Coarsen = true;   // main switch: --threads-coarsen
    bool m_దారాలు_Dpi_శుద్ధ = true;   // main switch: --threads-dpi all/pure
    bool m_దారాలు_Dpi_Unpure = false;  // main switch: --threads-dpi all
    VOptionBool m_timing;           // main switch: --timing
    bool m_trace = false;           // main switch: --trace
    bool m_traceCoverage = false;   // main switch: --trace-coverage
    bool m_traceEnabledFst = false;  // main switch: --trace-fst
    bool m_traceEnabledSaif = false;  // main switch: --trace-saif
    bool m_traceEnabledVcd = false;  // main switch: --trace-vcd
    bool m_traceParams = true;      // main switch: --trace-params
    bool m_traceStructs = false;    // main switch: --trace-structs
    bool m_no_Trace_శిఖరం = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_x_Initial_అంచు = false;    // main switch: --x-initial-edge

    పూర్ణాంకం         m_buildJobs = -1;    // main switch: --build-jobs, -j
    పూర్ణాంకం         m_coverage_సమాసం_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    పూర్ణాంకం         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    పూర్ణాంకం         m_converge_హద్దు = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    పూర్ణాంకం         m_coverage_Max_వెడల్పు = 256; // main switch: --coverage-max-width
    పూర్ణాంకం         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    పూర్ణాంకం         m_expand_హద్దు = 256;  // main switch: --expand-limit
    పూర్ణాంకం         m_gateStmts = 100;    // main switch: --gate-stmts
    పూర్ణాంకం         m_hier_బిడ్డ = 0;      // main switch: --hierarchical-child
    పూర్ణాంకం         m_hier_దారాలు = 0;      // main switch: --hierarchical-threads
    పూర్ణాంకం         m_if_లోతు = 0;      // main switch: --if-depth
    పూర్ణాంకం         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    పూర్ణాంకం         m_inline_CFuncs_లబ్ధం = 200;   // main switch: --inline-cfuncs-product
    పూర్ణాంకం         m_inlineMult = 2000;   // main switch: --inline-mult
    పూర్ణాంకం         m_instr_లెక్క_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    పూర్ణాంకం         m_localize_Max_కొలత = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    పూర్ణాంకం         m_max_అంకె_వెడల్పు = 65536;  // main switch: --max-num-width
    పూర్ణాంకం         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    పూర్ణాంకం         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    పూర్ణాంకం         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_output_Groups_కొలత = 512 * 1024 * 1024;  // main switch: --output-groups-size
    పూర్ణాంకం         m_output_విభజించు = 20000;  // main switch: --output-split
    పూర్ణాంకం         m_output_విభజించు_CFuncs = -1;  // main switch: --output-split-cfuncs
    పూర్ణాంకం         m_output_విభజించు_CTrace = -1;  // main switch: --output-split-ctrace
    పూర్ణాంకం         m_pinsBv = 65;       // main switch: --pins-bv
    పూర్ణాంకం         m_preproc_Token_హద్దు = 40000; // main switch: --preproc-token-limit
    పూర్ణాంకం         m_ప్రజా_లోతు = 0;   // main switch: --public-depth
    పూర్ణాంకం         m_reloop_హద్దు = 40; // main switch: --reloop-limit
    పూర్ణాంకం         m_replication_హద్దు = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    పూర్ణాంకం         m_దారాలు = 1;      // main switch: --threads
    పూర్ణాంకం         m_దారాలు_Max_MTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_సమయం_Default_Prec;  // main switch: --timescale
    VTimescale  m_సమయం_Default_Unit;  // main switch: --timescale
    VTimescale  m_సమయం_Override_Prec;  // main switch: --timescale-override
    VTimescale  m_సమయం_Override_Unit;  // main switch: --timescale-override
    పూర్ణాంకం         m_trace_లోతు = 0;   // main switch: --trace-depth
    పూర్ణాంకం         m_traceMaxArray = 32;  // main switch: --trace-max-array
    పూర్ణాంకం         m_trace_Max_వెడల్పు = 4096; // main switch: --trace-max-width
    పూర్ణాంకం         m_unroll_లెక్క = 64;  // main switch: --unroll-count
    పూర్ణాంకం         m_unroll_హద్దు = 16384;  // main switch: --unroll-limit
    పూర్ణాంకం         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    పూర్ణాంకం         m_constraint_Array_హద్దు = 64;  // main switch: --constraint-array-limit
    పూర్ణాంకం         m_verilateJobs = -1;  // main switch: --verilate-jobs

    పూర్ణాంకం         m_comp_హద్దు_ఖండాలు = 0;  // compiler selection; number of nested blocks
    పూర్ణాంకం         m_comp_హద్దు_Members = 64;  // compiler selection; number of members in struct before make anon array
    పూర్ణాంకం         m_comp_హద్దు_Parens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_శిఖరం_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_mod_ఉపసర్గ;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_ఉపసర్గ;       // main switch: --prefix
    string      m_protect_కీలకం;   // main switch: --protect-key
    string      m_శిఖరం_Module;    // main switch: --top-module
    string      m_వాడని_Regexp; // main switch: --unused-regexp
    string      m_waiverOutput;  // main switch: --waiver-output {filename}
    string      m_work = "work";  // main switch: --work {libname}
    string      m_x_కేటాయించు;      // main switch: --x-assign
    string      m_xInitial;     // main switch: --x-initial

    // Language is now held in FileLine, on a per-node basis. However we still
    // have a concept of the default language at a global level.
    V3_Lang_సంకేతం  m_defaultLanguage;      // main switch: --language

    // MEMBERS (optimizations)
    bool m_fAcycSimp;    // main switch: -fno-acyc-simp: acyclic pre-optimizations
    bool m_fAssemble;    // main switch: -fno-assemble: assign assemble
    bool m_f_బిట్టు_పరిశీలన_Loops;  // main switch: -fno-bit-scan-loops: convert bit scan loops to builtins
    bool m_fBitblast;    // main switch: -fno-bitblast: split packed variables into bit ranges
    bool m_fCaseDecoder; // main switch: -fno-case-decoder: case decoder conversion
    bool m_f_Case_పట్టిక;   // main switch: -fno-case-table: case table conversion
    bool m_f_Case_వృక్షం;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_బిట్టు_Op_వృక్షం;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_నెట్టు_కిందికి_Sels = true; // main switch: -fno-dfg-push-down-sels
    bool m_fDfg;         // main switch: -fno-dfg
    bool m_fDfgSynthesizeAll = false;  // main switch: -fdfg-synthesize-all
    bool m_fDeadAssigns;     // main switch: -fno-dead-assigns: remove dead assigns
    bool m_fDeadCells;   // main switch: -fno-dead-cells: remove dead cells
    bool m_fDeadMethods;   // main switch: -fno-dead-methods: remove dead methods
    bool m_fDecomposePacked;  // main switch: -fno-decompose-packed: packed decomposition
    bool m_fDecomposeUnpacked;  // main switch: -fno-decompose-unpacked: unpacked decomposition
    bool m_fExpand;      // main switch: -fno-expand: expansion of C macros
    bool m_fFuncBalanceCat = true;  // main switch: -fno-func-balance-cat: expansion of C macros
    bool m_f_Func_విభజించు_Cat = true;  // main switch: -fno-func-split-cat: expansion of C macros
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
    bool m_f_విలీనం_Cond;   // main switch: -fno-merge-cond: merge conditionals
    bool m_f_విలీనం_Cond_Motion = true; // main switch: -fno-merge-cond-motion: perform code motion
    bool m_fReloop;      // main switch: -fno-reloop: reform loops
    bool m_fReorder;     // main switch: -fno-reorder: reorder assignments in blocks
    bool m_f_ముక్క = true;  // main switch: -fno-slice: array assignment slicing
    పూర్ణాంకం  m_f_ముక్క_Element_హద్దు = 256;  // main switch: --fslice-element-limit
    bool m_f_విభజించు;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_f_పట్టిక;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_అందుబాటు = false;  // Set to true at the end of option parsing

    // METHODS
    శూన్య కూడు_Arg(అక్షరం** argv, కొలత_t లెక్క, bool isForRerun);
    శూన్య కూడు_Arg(స్థిరరాశి std::string& arg, bool isForRerun);
    శూన్య కూడు_పంక్తి_Arg(స్థిరరాశి string& arg);
    శూన్య కూడు_Define(స్థిరరాశి string& defline, bool allowPlus) VL_MT_DISABLED;
    శూన్య కూడు_Future(స్థిరరాశి string& flag);
    శూన్య కూడు_Future0(స్థిరరాశి string& flag);
    శూన్య కూడు_Future1(స్థిరరాశి string& flag);
    శూన్య కూడు_Inc_Dir_User(స్థిరరాశి string& incdir);  // User requested
    శూన్య కూడు_Inc_Dir_Fallback(స్థిరరాశి string& incdir);  // Low priority if not found otherwise
    శూన్య కూడు_Parameter(స్థిరరాశి string& paramline, bool allowPlus);
    శూన్య కూడు_Lang_Ext(స్థిరరాశి string& langext, స్థిరరాశి V3_Lang_సంకేతం& lc);
    శూన్య కూడు_Lib_Ext_V(స్థిరరాశి string& libext);
    శూన్య optimize(పూర్ణాంకం level);
    శూన్య show_సంస్కరణ(bool verbose);
    శూన్య coverage(bool flag) {
        m_coverage_పంక్తి = m_coverageBranch = m_coverageToggle = m_coverage_సమాసం = m_coverageFsm
            = m_coverageUser = flag;
    }
    స్థిర bool suffixed(స్థిరరాశి string& sw, స్థిరరాశి అక్షరం* arg);
    స్థిర string parseFileArg(స్థిరరాశి string& optdir, స్థిరరాశి string& relfilename);
    string file_మార్గం_Check_One_Dir(స్థిరరాశి string& modname, స్థిరరాశి string& dirname);
    స్థిర పూర్ణాంకం strip_Options_For_బిడ్డ_నిరంతరం(స్థిరరాశి string& opt, bool for_శిఖరం);
    శూన్య validateIdentifier(File_పంక్తి* fl, స్థిరరాశి string& arg, స్థిరరాశి string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

బహిరంగ:
    V3Options();
    ~V3Options();
    శూన్య సమితి_Debug_బహుళకం(పూర్ణాంకం level);
    గుర్తులేని debugLevel(స్థిరరాశి string& ట్యాగ్) స్థిరరాశి VL_MT_సురక్షిత;
    గుర్తులేని debugSrcLevel(స్థిరరాశి string& srcfile_మార్గం) స్థిరరాశి VL_MT_సురక్షిత;
    గుర్తులేని dumpLevel(స్థిరరాశి string& ట్యాగ్) స్థిరరాశి VL_MT_సురక్షిత;
    గుర్తులేని dumpSrcLevel(స్థిరరాశి string& srcfile_మార్గం) స్థిరరాశి VL_MT_సురక్షిత;

    // METHODS
    శూన్య కూడు_Cpp_File(స్థిరరాశి string& filename);
    శూన్య కూడు_CFlags(స్థిరరాశి string& filename);
    శూన్య కూడు_Compiler_Includes(స్థిరరాశి string& filename);
    శూన్య కూడు_Ld_Lib_File(స్థిరరాశి string& filename);
    శూన్య కూడు_Ld_Libs(స్థిరరాశి string& filename);
    శూన్య కూడు_Make_Flags(స్థిరరాశి string& filename);
    శూన్య కూడు_Lib_Map_File(స్థిరరాశి string& filename);
    శూన్య కూడు_Library_File(స్థిరరాశి string& filename, స్థిరరాశి string& libname);
    శూన్య కూడు_VFile(స్థిరరాశి string& filename, స్థిరరాశి string& libname);
    శూన్య కూడు_Vlt_File(స్థిరరాశి string& filename, స్థిరరాశి string& libname);
    శూన్య కూడు_Force_Inc(స్థిరరాశి string& filename);
    bool అందుబాటు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_అందుబాటు; }
    శూన్య cc_సమితి();
    శూన్య decorations(File_పంక్తి* fl, స్థిరరాశి string& filename);
    శూన్య notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() స్థిరరాశి { తిరిగి m_preprocOnly; }
    bool makePhony() స్థిరరాశి { తిరిగి m_makePhony; }
    bool preproc_No_పంక్తి() స్థిరరాశి { తిరిగి m_preproc_No_పంక్తి; }
    bool preprocResolve() స్థిరరాశి { తిరిగి m_preprocResolve; }
    పూర్ణాంకం preproc_Token_హద్దు() స్థిరరాశి { తిరిగి m_preproc_Token_హద్దు; }
    bool underlineZero() స్థిరరాశి { తిరిగి m_underlineZero; }
    bool systemC() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_systemC; }
    bool savable() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_savable; }
    VOptionBool schedZeroDelay() స్థిరరాశి { తిరిగి m_schedZeroDelay; }
    bool stats() స్థిరరాశి { తిరిగి m_stats; }
    bool statsVars() స్థిరరాశి { తిరిగి m_statsVars; }
    bool stdPackage() స్థిరరాశి { తిరిగి m_stdPackage; }
    bool stdWaiver() స్థిరరాశి { తిరిగి m_stdWaiver; }
    bool assertOn() స్థిరరాశి { తిరిగి m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() స్థిరరాశి { తిరిగి m_assertCase; }
    bool autoflush() స్థిరరాశి { తిరిగి m_autoflush; }
    bool bboxSys() స్థిరరాశి { తిరిగి m_bboxSys; }
    bool bboxUnsup() స్థిరరాశి { తిరిగి m_bboxUnsup; }
    bool build() స్థిరరాశి { తిరిగి m_build; }
    string buildDepBin() స్థిరరాశి { తిరిగి m_buildDepBin; }
    శూన్య buildDepBin(స్థిరరాశి string& flag) { m_buildDepBin = flag; }
    bool context() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_context; }
    bool coverage() స్థిరరాశి VL_MT_సురక్షిత {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        తిరిగి m_coverage_పంక్తి || m_coverageBranch || m_coverageToggle || m_coverage_సమాసం
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() స్థిరరాశి VL_MT_సురక్షిత {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        తిరిగి m_coverage_పంక్తి || m_coverageBranch || m_coverageToggle || m_coverage_సమాసం
               || m_coverageUser;
    }
    bool coverageBranch() స్థిరరాశి { తిరిగి m_coverageBranch; }
    bool coverage_సమాసం() స్థిరరాశి { తిరిగి m_coverage_సమాసం; }
    bool coverageFsm() స్థిరరాశి { తిరిగి m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() స్థిరరాశి { తిరిగి m_coverageFsmExpand; }
    పూర్ణాంకం coverageFsmMaxArcs() స్థిరరాశి { తిరిగి m_coverageFsmMaxArcs; }
    bool coverage_పంక్తి() స్థిరరాశి { తిరిగి m_coverage_పంక్తి; }
    bool coverage_విలీనం_Instances() స్థిరరాశి { తిరిగి m_coverage_విలీనం_Instances; }
    bool coverage_చొప్పున_Instance() స్థిరరాశి { తిరిగి m_coverage_చొప్పున_Instance; }
    bool coverageToggle() స్థిరరాశి { తిరిగి m_coverageToggle; }
    bool coverageUnderscore() స్థిరరాశి { తిరిగి m_coverageUnderscore; }
    bool coverageUser() స్థిరరాశి { తిరిగి m_coverageUser; }
    bool debugCheck() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_debugCheck; }
    bool debugCollision() స్థిరరాశి { తిరిగి m_debugCollision; }
    bool debugEmitV() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_debugEmitV; }
    bool debugExitElab() స్థిరరాశి { తిరిగి m_debugExitElab; }
    bool debugExitParse() స్థిరరాశి { తిరిగి m_debugExitParse; }
    bool debugLeak() స్థిరరాశి { తిరిగి m_debugLeak; }
    bool debugNondeterminism() స్థిరరాశి { తిరిగి m_debugNondeterminism; }
    bool debug_విభజన() స్థిరరాశి { తిరిగి m_debug_విభజన; }
    bool debugPreprocPassthru() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_debugPreprocPassthru; }
    bool debugProtect() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_debugProtect; }
    పూర్ణాంకం debugRuntimeTimeout() స్థిరరాశి { తిరిగి m_debugRuntimeTimeout; }
    bool debugSelfTest() స్థిరరాశి { తిరిగి m_debugSelfTest; }
    bool debug_దొంతర_Check() స్థిరరాశి { తిరిగి m_debug_దొంతర_Check; }
    bool debug_వెడల్పు() స్థిరరాశి VL_శుద్ధ { తిరిగి m_debug_వెడల్పు; }
    bool decoration() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_decoration; }
    bool decorationNodes() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_decorationNodes; }
    bool diagnosticsSarif() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_diagnosticsSarif; }
    bool dpiHdrOnly() స్థిరరాశి { తిరిగి m_dpiHdrOnly; }
    bool dumpAstPatterns() స్థిరరాశి {
        తిరిగి m_dumpLevel.లెక్క("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dump_నిర్వచిస్తుంది() స్థిరరాశి { తిరిగి m_dumpLevel.లెక్క("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() స్థిరరాశి {
        తిరిగి m_dumpLevel.లెక్క("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_వృక్షం_Dot() స్థిరరాశి {
        తిరిగి m_dumpLevel.లెక్క("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() స్థిరరాశి { తిరిగి m_emitAccessors; }
    bool exe() స్థిరరాశి { తిరిగి m_exe; }
    bool flatten() స్థిరరాశి { తిరిగి m_flatten; }
    bool fourstate() స్థిరరాశి { తిరిగి m_fourstate; }
    bool gmake() స్థిరరాశి { తిరిగి m_gmake; }
    bool makeJson() స్థిరరాశి { తిరిగి m_makeJson; }
    bool దారాలు_Dpi_శుద్ధ() స్థిరరాశి { తిరిగి m_దారాలు_Dpi_శుద్ధ; }
    bool దారాలు_Dpi_Unpure() స్థిరరాశి { తిరిగి m_దారాలు_Dpi_Unpure; }
    bool దారాలు_Coarsen() స్థిరరాశి { తిరిగి m_దారాలు_Coarsen; }
    VOptionBool timing() స్థిరరాశి { తిరిగి m_timing; }
    bool trace() స్థిరరాశి { తిరిగి m_trace; }
    bool traceCoverage() స్థిరరాశి { తిరిగి m_traceCoverage; }
    bool traceEnabledFst() స్థిరరాశి { తిరిగి m_traceEnabledFst; }
    bool traceEnabledSaif() స్థిరరాశి { తిరిగి m_traceEnabledSaif; }
    bool traceEnabledVcd() స్థిరరాశి { తిరిగి m_traceEnabledVcd; }
    bool traceParams() స్థిరరాశి { తిరిగి m_traceParams; }
    bool traceStructs() స్థిరరాశి { తిరిగి m_traceStructs; }
    bool traceUnderscore() స్థిరరాశి { తిరిగి m_traceUnderscore; }
    bool ప్రధాన() స్థిరరాశి { తిరిగి m_main; }
    bool బహిర్గతం_Format_Ok() స్థిరరాశి { తిరిగి m_బహిర్గతం_Format_Ok; }
    bool jsonOnly() స్థిరరాశి { తిరిగి m_jsonOnly; }
    bool keep_తాత్కాలిక_Files() స్థిరరాశి { తిరిగి (V3Error::debugDefault() != 0); }
    bool pedantic() స్థిరరాశి { తిరిగి m_pedantic; }
    bool pinsInoutEnables() స్థిరరాశి { తిరిగి m_pinsInoutEnables; }
    bool pinsScUint() స్థిరరాశి { తిరిగి m_pinsScUint; }
    bool pinsScUintBool() స్థిరరాశి { తిరిగి m_pinsScUintBool; }
    bool pinsScBigUint() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_pinsScBigUint; }
    bool pinsUint8() స్థిరరాశి { తిరిగి m_pinsUint8; }
    bool preprocComments() స్థిరరాశి { తిరిగి m_preprocComments; }
    bool preproc_నిర్వచిస్తుంది() స్థిరరాశి { తిరిగి m_preproc_నిర్వచిస్తుంది; }
    bool profC() స్థిరరాశి { తిరిగి m_profC; }
    bool profCFuncs() స్థిరరాశి { తిరిగి m_profCFuncs; }
    bool profExec() స్థిరరాశి { తిరిగి m_profExec; }
    bool profPgo() స్థిరరాశి { తిరిగి m_profPgo; }
    bool ఉపయోగిస్తుంది_Profiler() స్థిరరాశి { తిరిగి profExec() || profPgo(); }
    bool protectIds() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_protectIds; }
    bool all_ప్రజా() స్థిరరాశి { తిరిగి m_ప్రజా; }
    bool ప్రజా_Params() స్థిరరాశి { తిరిగి m_ప్రజా_Params; }
    bool ప్రజా_Off() స్థిరరాశి { తిరిగి m_ప్రజా_Ignore; }
    bool ప్రజా_Flat_RW() స్థిరరాశి { తిరిగి m_ప్రజా_Flat_RW; }
    పూర్ణాంకం ప్రజా_లోతు() స్థిరరాశి { తిరిగి m_ప్రజా_లోతు; }
    bool any_ప్రజా_Flat() స్థిరరాశి { తిరిగి m_ప్రజా_Params || m_ప్రజా_Flat_RW || m_ప్రజా_లోతు; }
    bool lintOnly() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_lintOnly; }
    bool ignc() స్థిరరాశి { తిరిగి m_ignc; }
    bool quietBuild() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_quietBuild; }
    bool quietExit() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_quietExit; }
    bool quietStats() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_quietStats; }
    bool reportUnoptflat() స్థిరరాశి { తిరిగి m_reportUnoptflat; }
    bool verilate() స్థిరరాశి { తిరిగి m_verilate; }
    bool vpi() స్థిరరాశి { తిరిగి m_vpi.isTrue(); }
    bool waiverMultiline() స్థిరరాశి { తిరిగి m_waiverMultiline; }
    bool x_Initial_అంచు() స్థిరరాశి { తిరిగి m_x_Initial_అంచు; }
    bool serializeOnly() స్థిరరాశి { తిరిగి m_jsonOnly; }
    bool శిఖరం_Ifaces_Supported() స్థిరరాశి { తిరిగి lintOnly() && !క్రమానుగత(); }

    పూర్ణాంకం buildJobs() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_buildJobs; }
    పూర్ణాంకం converge_హద్దు() స్థిరరాశి { తిరిగి m_converge_హద్దు; }
    పూర్ణాంకం coverage_సమాసం_Max() స్థిరరాశి { తిరిగి m_coverage_సమాసం_Max; }
    uint32_t coverageMaxBins() స్థిరరాశి { తిరిగి m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() స్థిరరాశి { తిరిగి m_coverageMaxRealBins; }
    పూర్ణాంకం coverage_Max_వెడల్పు() స్థిరరాశి { తిరిగి m_coverage_Max_వెడల్పు; }
    పూర్ణాంకం debugAllocRandom() స్థిరరాశి { తిరిగి m_debugAllocRandom; }
    bool dump_వృక్షం_Addrids() స్థిరరాశి VL_MT_సురక్షిత;
    పూర్ణాంకం expand_హద్దు() స్థిరరాశి { తిరిగి m_expand_హద్దు; }
    పూర్ణాంకం gateStmts() స్థిరరాశి { తిరిగి m_gateStmts; }
    పూర్ణాంకం if_లోతు() స్థిరరాశి { తిరిగి m_if_లోతు; }
    పూర్ణాంకం inlineCFuncs() స్థిరరాశి { తిరిగి m_inlineCFuncs; }
    పూర్ణాంకం inline_CFuncs_లబ్ధం() స్థిరరాశి { తిరిగి m_inline_CFuncs_లబ్ధం; }
    పూర్ణాంకం inlineMult() స్థిరరాశి { తిరిగి m_inlineMult; }
    పూర్ణాంకం instr_లెక్క_Dpi() స్థిరరాశి { తిరిగి m_instr_లెక్క_Dpi; }
    పూర్ణాంకం localize_Max_కొలత() స్థిరరాశి { తిరిగి m_localize_Max_కొలత; }
    bool jsonEditNums() స్థిరరాశి { తిరిగి m_jsonEditNums; }
    bool jsonIds() స్థిరరాశి { తిరిగి m_jsonIds; }
    VOptionBool makeDepend() స్థిరరాశి { తిరిగి m_makeDepend; }
    పూర్ణాంకం max_అంకె_వెడల్పు() స్థిరరాశి { తిరిగి m_max_అంకె_వెడల్పు; }
    పూర్ణాంకం func_Recursion_లోతు() స్థిరరాశి { తిరిగి m_funcRecursion; }
    పూర్ణాంకం module_Recursion_లోతు() స్థిరరాశి { తిరిగి m_moduleRecursion; }
    పూర్ణాంకం output_విభజించు() స్థిరరాశి { తిరిగి m_output_విభజించు; }
    పూర్ణాంకం output_విభజించు_CFuncs() స్థిరరాశి { తిరిగి m_output_విభజించు_CFuncs; }
    పూర్ణాంకం output_విభజించు_CTrace() స్థిరరాశి { తిరిగి m_output_విభజించు_CTrace; }
    పూర్ణాంకం outputGroups() స్థిరరాశి { తిరిగి m_outputGroups; }
    int64_t output_Groups_కొలత() స్థిరరాశి { తిరిగి m_output_Groups_కొలత; }
    పూర్ణాంకం pinsBv() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_pinsBv; }
    పూర్ణాంకం reloop_హద్దు() స్థిరరాశి { తిరిగి m_reloop_హద్దు; }
    పూర్ణాంకం replication_హద్దు() స్థిరరాశి { తిరిగి m_replication_హద్దు; }
    VOptionBool skipIdentical() స్థిరరాశి { తిరిగి m_skipIdentical; }
    bool stopFail() స్థిరరాశి { తిరిగి m_stopFail; }
    పూర్ణాంకం దారాలు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_దారాలు; }
    పూర్ణాంకం దారాలు_Max_MTasks() స్థిరరాశి { తిరిగి m_దారాలు_Max_MTasks; }
    bool mtasks() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి (m_దారాలు > 1); }
    VTimescale సమయం_Default_Prec() స్థిరరాశి { తిరిగి m_సమయం_Default_Prec; }
    VTimescale సమయం_Default_Unit() స్థిరరాశి { తిరిగి m_సమయం_Default_Unit; }
    VTimescale సమయం_Override_Prec() స్థిరరాశి { తిరిగి m_సమయం_Override_Prec; }
    VTimescale సమయం_Override_Unit() స్థిరరాశి { తిరిగి m_సమయం_Override_Unit; }
    VTimescale సమయం_Compute_Prec(స్థిరరాశి VTimescale& flag) స్థిరరాశి;
    VTimescale సమయం_Compute_Unit(స్థిరరాశి VTimescale& flag) స్థిరరాశి;
    పూర్ణాంకం trace_లోతు() స్థిరరాశి { తిరిగి m_trace_లోతు; }
    పూర్ణాంకం traceMaxArray() స్థిరరాశి { తిరిగి m_traceMaxArray; }
    పూర్ణాంకం trace_Max_వెడల్పు() స్థిరరాశి { తిరిగి m_trace_Max_వెడల్పు; }
    bool useTraceParallel() స్థిరరాశి {
        తిరిగి trace() && traceEnabledVcd() && (దారాలు() > 1 || hier_బిడ్డ() > 1);
    }
    పూర్ణాంకం unroll_లెక్క() స్థిరరాశి { తిరిగి m_unroll_లెక్క; }
    పూర్ణాంకం unroll_హద్దు() స్థిరరాశి { తిరిగి m_unroll_హద్దు; }
    పూర్ణాంకం unrollStmts() స్థిరరాశి { తిరిగి m_unrollStmts; }
    పూర్ణాంకం constraint_Array_హద్దు() స్థిరరాశి { తిరిగి m_constraint_Array_హద్దు; }
    పూర్ణాంకం verilateJobs() స్థిరరాశి { తిరిగి m_verilateJobs; }

    పూర్ణాంకం comp_హద్దు_ఖండాలు() స్థిరరాశి { తిరిగి m_comp_హద్దు_ఖండాలు; }
    పూర్ణాంకం comp_హద్దు_Members() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_comp_హద్దు_Members; }
    పూర్ణాంకం comp_హద్దు_Parens() స్థిరరాశి { తిరిగి m_comp_హద్దు_Parens; }

    string diagnosticsSarifOutput() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_diagnosticsSarifOutput.ఖాళీలు() ? makeDir() + "/" + ఉపసర్గ() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() స్థిరరాశి { తిరిగి m_exeName != "" ? m_exeName : ఉపసర్గ(); }
    VFileLibList hierParamFile() స్థిరరాశి { తిరిగి m_hierParamsFile; }
    string jsonOnlyOutput() స్థిరరాశి { తిరిగి m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() స్థిరరాశి { తిరిగి m_jsonOnlyMetaOutput; }
    string l2Name() స్థిరరాశి { తిరిగి m_l2Name; }
    string libCreate() స్థిరరాశి { తిరిగి m_libCreate; }
    string libCreateName(bool shared) స్థిరరాశి {
        string libName = "lib" + libCreate();
        ఒకవేళ (shared) {
            libName += ".so";
        } లేకపోతే {
            libName += ".a";
        }
        తిరిగి libName;
    }
    string main_శిఖరం_Name() స్థిరరాశి { తిరిగి m_main_శిఖరం_Name; }
    string makeDir() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_makeDir; }
    string mod_ఉపసర్గ() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_mod_ఉపసర్గ; }
    string pipeFilter() స్థిరరాశి { తిరిగి m_pipeFilter; }
    string ఉపసర్గ() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_ఉపసర్గ; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_కీలకం_Provided() స్థిరరాశి { తిరిగి !m_protect_కీలకం.ఖాళీలు(); }
    string protect_కీలకం_Defaulted() VL_MT_సురక్షిత;  // Set default key if not set by user
    string శిఖరం_Module() స్థిరరాశి { తిరిగి m_శిఖరం_Module; }  // As AstNode::encodeName()
    bool no_Trace_శిఖరం() స్థిరరాశి { తిరిగి m_no_Trace_శిఖరం; }
    string వాడని_Regexp() స్థిరరాశి { తిరిగి m_వాడని_Regexp; }
    string waiverOutput() స్థిరరాశి { తిరిగి m_waiverOutput; }
    string work() స్థిరరాశి { తిరిగి m_work; }
    bool isWaiverOutput() స్థిరరాశి { తిరిగి !m_waiverOutput.ఖాళీలు(); }
    string x_కేటాయించు() స్థిరరాశి { తిరిగి m_x_కేటాయించు; }
    string xInitial() స్థిరరాశి { తిరిగి m_xInitial; }

    స్థిరరాశి VString_సమితి& cppFiles() స్థిరరాశి { తిరిగి m_cppFiles; }
    స్థిరరాశి VStringList& cFlags() స్థిరరాశి { తిరిగి m_cFlags; }
    స్థిరరాశి VString_సమితి& compilerIncludes() స్థిరరాశి { తిరిగి m_compilerIncludes; }
    స్థిరరాశి VStringList& ldLibFiles() స్థిరరాశి { తిరిగి m_ldLibFiles; }
    స్థిరరాశి VStringList& ldLibs() స్థిరరాశి { తిరిగి m_ldLibs; }
    స్థిరరాశి VStringList& makeFlags() స్థిరరాశి { తిరిగి m_makeFlags; }
    స్థిరరాశి VFile_Lib_సమితి& libraryFiles() స్థిరరాశి { తిరిగి m_libraryFiles; }
    స్థిరరాశి VString_సమితి& libmapFiles() స్థిరరాశి { తిరిగి m_libmapFiles; }
    స్థిరరాశి VFileLibList& vFiles() స్థిరరాశి { తిరిగి m_vFiles; }
    స్థిరరాశి VFile_Lib_సమితి& vltFiles() స్థిరరాశి { తిరిగి m_vltFiles; }
    స్థిరరాశి VStringList& forceIncs() స్థిరరాశి { తిరిగి m_forceIncs; }

    bool hasParameter(స్థిరరాశి string& name);
    string parameter(స్థిరరాశి string& name);
    శూన్య checkParameters();

    bool isFuture(స్థిరరాశి string& flag) స్థిరరాశి;
    bool isFuture0(స్థిరరాశి string& flag) స్థిరరాశి;
    bool isFuture1(స్థిరరాశి string& flag) స్థిరరాశి;
    bool isLibraryFile(స్థిరరాశి string& filename, స్థిరరాశి string& libname) స్థిరరాశి;

    // ACCESSORS (optimization options)
    bool fAcycSimp() స్థిరరాశి { తిరిగి m_fAcycSimp; }
    bool fAssemble() స్థిరరాశి { తిరిగి m_fAssemble; }
    bool f_బిట్టు_పరిశీలన_Loops() స్థిరరాశి { తిరిగి m_f_బిట్టు_పరిశీలన_Loops; }
    bool fBitblast() స్థిరరాశి { తిరిగి m_fBitblast; }
    bool fCaseDecoder() స్థిరరాశి { తిరిగి m_fCaseDecoder; }
    bool f_Case_పట్టిక() స్థిరరాశి { తిరిగి m_f_Case_పట్టిక; }
    bool f_Case_వృక్షం() స్థిరరాశి { తిరిగి m_f_Case_వృక్షం; }
    bool fCombine() స్థిరరాశి { తిరిగి m_fCombine; }
    bool fConst() స్థిరరాశి { తిరిగి m_fConst; }
    bool fConstBeforeDfg() స్థిరరాశి { తిరిగి m_fConstBeforeDfg; }
    bool f_Const_బిట్టు_Op_వృక్షం() స్థిరరాశి { తిరిగి m_f_Const_బిట్టు_Op_వృక్షం; }
    bool fConstEager() స్థిరరాశి { తిరిగి m_fConstEager; }
    bool fDedupe() స్థిరరాశి { తిరిగి m_fDedupe; }
    bool fDfg() స్థిరరాశి { తిరిగి m_fDfg; }
    bool fDfgPeephole() స్థిరరాశి { తిరిగి m_fDfgPeephole; }
    bool f_Dfg_నెట్టు_కిందికి_Sels() స్థిరరాశి { తిరిగి m_f_Dfg_నెట్టు_కిందికి_Sels; }
    bool fDfgSynthesizeAll() స్థిరరాశి { తిరిగి m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(స్థిరరాశి std::string& name) స్థిరరాశి {
        తిరిగి !m_fDfgPeepholeDisabled.లెక్క(name);
    }
    bool fDeadAssigns() స్థిరరాశి { తిరిగి m_fDeadAssigns; }
    bool fDeadCells() స్థిరరాశి { తిరిగి m_fDeadCells; }
    bool fDeadMethods() స్థిరరాశి { తిరిగి m_fDeadMethods; }
    bool fDecompose() స్థిరరాశి { తిరిగి m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() స్థిరరాశి { తిరిగి m_fDecomposePacked; }
    bool fDecomposeUnpacked() స్థిరరాశి { తిరిగి m_fDecomposeUnpacked; }
    bool fExpand() స్థిరరాశి { తిరిగి m_fExpand; }
    bool fFuncBalanceCat() స్థిరరాశి { తిరిగి m_fFuncBalanceCat; }
    bool f_Func_విభజించు_Cat() స్థిరరాశి { తిరిగి m_f_Func_విభజించు_Cat; }
    bool fFunc() స్థిరరాశి { తిరిగి f_Func_విభజించు_Cat() || fFuncBalanceCat(); }
    bool fGate() స్థిరరాశి { తిరిగి m_fGate; }
    VOptionBool fIcoChangeDetect() స్థిరరాశి { తిరిగి m_fIcoChangeDetect; }
    bool fInline() స్థిరరాశి { తిరిగి m_fInline; }
    bool fInlineCFuncs() స్థిరరాశి { తిరిగి m_fInlineCFuncs; }
    bool fInlineFuncs() స్థిరరాశి { తిరిగి m_fInlineFuncs; }
    bool fInlineFuncsEager() స్థిరరాశి { తిరిగి m_fInlineFuncsEager; }
    bool fLife() స్థిరరాశి { తిరిగి m_fLife; }
    bool fLifePost() స్థిరరాశి { తిరిగి m_fLifePost; }
    bool fLocalize() స్థిరరాశి { తిరిగి m_fLocalize; }
    bool f_విలీనం_Cond() స్థిరరాశి { తిరిగి m_f_విలీనం_Cond; }
    bool f_విలీనం_Cond_Motion() స్థిరరాశి { తిరిగి m_f_విలీనం_Cond_Motion; }
    bool fReloop() స్థిరరాశి { తిరిగి m_fReloop; }
    bool fReorder() స్థిరరాశి { తిరిగి m_fReorder; }
    bool f_ముక్క() స్థిరరాశి { తిరిగి m_f_ముక్క; }
    పూర్ణాంకం f_ముక్క_Element_హద్దు() స్థిరరాశి { తిరిగి m_f_ముక్క_Element_హద్దు; }
    bool f_విభజించు() స్థిరరాశి { తిరిగి m_f_విభజించు; }
    bool fSubst() స్థిరరాశి { తిరిగి m_fSubst; }
    bool fSubstConst() స్థిరరాశి { తిరిగి m_fSubstConst; }
    bool f_పట్టిక() స్థిరరాశి { తిరిగి m_f_పట్టిక; }
    bool fTaskifyAll() స్థిరరాశి { తిరిగి m_fTaskifyAll; }

    std::string trace_Class_పునాది() స్థిరరాశి VL_MT_సురక్షిత;  // Deprecated
    std::string traceClassLang() స్థిరరాశి VL_MT_సురక్షిత;  // Deprecated
    std::సదిశ<std::string> traceClassBases() స్థిరరాశి VL_MT_సురక్షిత;
    std::సదిశ<std::string> traceClassLangs() స్థిరరాశి VL_MT_సురక్షిత;
    std::సదిశ<std::string> trace_ఆధారం_Bases() స్థిరరాశి VL_MT_సురక్షిత;
    std::సదిశ<std::string> trace_ఆధారం_Langs() స్థిరరాశి VL_MT_సురక్షిత;

    bool క్రమానుగత() స్థిరరాశి { తిరిగి m_క్రమానుగత; }
    పూర్ణాంకం hier_బిడ్డ() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_hier_బిడ్డ; }
    పూర్ణాంకం hier_దారాలు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_hier_దారాలు == 0 ? m_దారాలు : m_hier_దారాలు; }
    bool hier_శిఖరం() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి !m_hier_బిడ్డ && !m_hier_ఖండాలు.ఖాళీలు(); }
    స్థిరరాశి V3_Hier_ఖండం_Opt_సమితి& hier_ఖండాలు() స్థిరరాశి { తిరిగి m_hier_ఖండాలు; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_శిఖరం_దత్తాంశం_Dir() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి hier_శిఖరం() ? (makeDir() + '/' + ఉపసర్గ() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    స్థిర string సంస్కరణ() VL_శుద్ధ;
    స్థిర string argString(పూర్ణాంకం argc, అక్షరం** argv);  ///< Return list of arguments as simple string
    string all_వాదనలు_String() స్థిరరాశి VL_MT_సురక్షిత;  ///< Return all passed arguments as simple string
    స్థిరరాశి std::list<std::pair<std::list<std::string>, bool>>& all_వాదనలు() స్థిరరాశి;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string all_వాదనలు_String_For_Hier_ఖండం(bool for_శిఖరం) స్థిరరాశి;
    శూన్య parseOpts(File_పంక్తి* fl, పూర్ణాంకం argc, అక్షరం** argv) VL_MT_DISABLED;
    శూన్య parseOptsList(File_పంక్తి* fl, స్థిరరాశి string& optdir, పూర్ణాంకం argc, అక్షరం** argv) VL_MT_DISABLED;
    శూన్య parseOptsFile(File_పంక్తి* fl, స్థిరరాశి string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    స్థిర string getenvBuiltins(స్థిరరాశి string& var);
    స్థిర string getenvMAKE();
    స్థిర string getenvMAKEFLAGS();
    స్థిర string getenvPERL();
    స్థిర string getenvPYTHON3();
    స్థిర string getenvSYSTEMC();
    స్థిర string getenvSYSTEMC_ARCH();
    స్థిర string getenvSYSTEMC_INCLUDE();
    స్థిర string getenvSYSTEMC_LIBDIR();
    స్థిర string getenv_VERILATOR_మూలం();
    స్థిర string getenvVERILATOR_SOLVER();
    స్థిర string get_Std_Package_మార్గం();
    స్థిర string get_Std_Waiver_మార్గం();
    స్థిర string getSupported(స్థిరరాశి string& var);
    స్థిర bool systemCSystemWide();
    స్థిర bool systemCFound();  // SystemC installed, or environment points to it
    స్థిర bool coroutineSupport();  // Configured C++ compiler supports coroutines
    స్థిర bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    స్థిర bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    స్థిర bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(స్థిరరాశి string& filename);
    string file_మార్గం(File_పంక్తి* fl, స్థిరరాశి string& modname, స్థిరరాశి string& lastpath,
                    స్థిరరాశి string& errmsg);
    string file_మార్గం_Looked_సందేశం(File_పంక్తి* fl, స్థిరరాశి string& modname);
    V3_Lang_సంకేతం fileLanguage(స్థిరరాశి string& filename);
    స్థిర bool file_Stat_సాధారణ(స్థిరరాశి string& filename);

    // METHODS (other OS)
    స్థిర శూన్య throwSigsegv();
};

//######################################################################

#అయితే_ముగింపు  // guard
