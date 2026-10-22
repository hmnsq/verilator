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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3LangCode.h"

#अन्तर्भाव <list>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <string>
#अन्तर्भाव <vector>

class V3OptionsImp;
class File_ओळ;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
सार्वजनिक:
    गणना en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    गणना en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिरांक { परत m_e; }
    स्थिरांक अक्षर* ascii() स्थिरांक {
        स्थिर स्थिरांक अक्षर* स्थिरांक names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        परत names[m_e];
    }
    bool isDefault() स्थिरांक { परत m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() स्थिरांक { परत m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() स्थिरांक { परत m_e == OPT_TRUE; }
    bool isSetFalse() स्थिरांक { परत m_e == OPT_FALSE; }
    शून्य setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool चालक==(स्थिरांक VOptionBool& lhs, स्थिरांक VOptionBool& rhs) {
    परत lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिरांक VOptionBool& lhs, VOptionBool::en rhs) { परत lhs.m_e == rhs; }
constexpr bool चालक==(VOptionBool::en lhs, स्थिरांक VOptionBool& rhs) { परत lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    स्थिरांक string m_filename;  // Filename
    स्थिरांक string m_libname;  // Libname
सार्वजनिक:
    VFileLibName(स्थिरांक string& filename, स्थिरांक string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(स्थिरांक VFileLibName& rhs) = पूर्वनिर्धारित;
    string filename() स्थिरांक { परत m_filename; }
    string libname() स्थिरांक { परत m_libname; }
    bool चालक==(स्थिरांक VFileLibName& rhs) स्थिरांक {
        परत m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool चालक<(स्थिरांक VFileLibName& rhs) स्थिरांक {
        जर (m_filename < rhs.m_filename) परत true;
        जर (m_filename > rhs.m_filename) परत false;
        परत m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
सार्वजनिक:
    गणना en : uint8_t {
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
    गणना : uint8_t { TS_DEFAULT = TS_1PS };
    गणना en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(स्थिरांक string& मूल्य, bool& badr);
    VTimescale(द्विगुण मूल्य, bool& badr) {
        badr = false;
        साठी (पूर्णांक i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            जर (multiplier() == मूल्य) थांबा;
        }
        जर (multiplier() != मूल्य) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() स्थिरांक { परत m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    स्थिर शून्य parseSlashed(File_ओळ* fl, स्थिरांक अक्षर* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    स्थिरांक अक्षर* ascii() स्थिरांक {
        स्थिर स्थिरांक अक्षर* स्थिरांक names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        परत names[m_e];
    }
    पूर्णांक powerOfTen() स्थिरांक { परत 2 - static_cast<पूर्णांक>(m_e); }
    द्विगुण multiplier() स्थिरांक {
        स्थिर स्थिरांक द्विगुण मूल्ये[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        परत मूल्ये[m_e];
    }
};
constexpr bool चालक==(स्थिरांक VTimescale& lhs, स्थिरांक VTimescale& rhs) {
    परत lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिरांक VTimescale& lhs, VTimescale::en rhs) { परत lhs.m_e == rhs; }
constexpr bool चालक==(VTimescale::en lhs, स्थिरांक VTimescale& rhs) { परत lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool चालक<(स्थिरांक VTimescale& lhs, स्थिरांक VTimescale& rhs) {
    परत lhs.m_e > rhs.m_e;
}
ओळीत std::ostream& चालक<<(std::ostream& os, स्थिरांक VTimescale& rhs) {
    परत os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
सार्वजनिक:
    गणना en : uint8_t {
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
    explicit VFsmExpandType(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिरांक { परत m_e; }
    constexpr स्थिरांक अक्षर* ascii() स्थिरांक {
        constexpr स्थिरांक अक्षर* स्थिरांक names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        परत names[m_e];
    }
};
constexpr bool चालक==(स्थिरांक VFsmExpandType& lhs, स्थिरांक VFsmExpandType& rhs) VL_MT_SAFE {
    परत lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिरांक VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    परत lhs.m_e == rhs;
}
constexpr bool चालक==(VFsmExpandType::en lhs, स्थिरांक VFsmExpandType& rhs) VL_MT_SAFE {
    परत lhs == rhs.m_e;
}
ओळीत std::ostream& चालक<<(std::ostream& os, स्थिरांक VFsmExpandType& rhs) VL_MT_SAFE {
    परत os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
सार्वजनिक:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<स्थिरांक std::string, std::string>;

खाजगी:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

सार्वजनिक:
    explicit V3HierarchicalBlockOption(स्थिरांक string& optstring);
    स्थिरांक string& origName() स्थिरांक { परत m_origName; }
    स्थिरांक string& mangledName() स्थिरांक { परत m_mangledName; }
    स्थिरांक ParamStrMap params() स्थिरांक { परत m_parameters; }
};

using V3HierBlockOptSet = std::map<स्थिरांक std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
सार्वजनिक:
खाजगी:
    // TYPES
    using DebugLevelMap = std::map<स्थिरांक std::string, अचिन्हित>;

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
    std::map<स्थिरांक string, string> m_parameters;  // Parameters
    std::map<स्थिरांक string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_ओळ = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_द्विमान = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_अभिव्यक्ती = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_ओळ = false;    // main switch: --coverage-block
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
    bool m_debug_चळत_Check = false;  // main switch: --debug-stack-check
    पूर्णांक m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_no_Trace_शीर्ष = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    पूर्णांक         m_buildJobs = -1;    // main switch: --build-jobs, -j
    पूर्णांक         m_coverage_अभिव्यक्ती_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    पूर्णांक         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    पूर्णांक         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    पूर्णांक         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    पूर्णांक         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    पूर्णांक         m_expandLimit = 256;  // main switch: --expand-limit
    पूर्णांक         m_gateStmts = 100;    // main switch: --gate-stmts
    पूर्णांक         m_hierChild = 0;      // main switch: --hierarchical-child
    पूर्णांक         m_hierThreads = 0;      // main switch: --hierarchical-threads
    पूर्णांक         m_ifDepth = 0;      // main switch: --if-depth
    पूर्णांक         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    पूर्णांक         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    पूर्णांक         m_inlineMult = 2000;   // main switch: --inline-mult
    पूर्णांक         m_instr_मोजणी_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    पूर्णांक         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    पूर्णांक         m_max_अंक_Width = 65536;  // main switch: --max-num-width
    पूर्णांक         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    पूर्णांक         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    पूर्णांक         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    पूर्णांक         m_outputSplit = 20000;  // main switch: --output-split
    पूर्णांक         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    पूर्णांक         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    पूर्णांक         m_pinsBv = 65;       // main switch: --pins-bv
    पूर्णांक         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    पूर्णांक         m_publicDepth = 0;   // main switch: --public-depth
    पूर्णांक         m_reloopLimit = 40; // main switch: --reloop-limit
    पूर्णांक         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    पूर्णांक         m_threads = 1;      // main switch: --threads
    पूर्णांक         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    पूर्णांक         m_traceDepth = 0;   // main switch: --trace-depth
    पूर्णांक         m_traceMaxArray = 32;  // main switch: --trace-max-array
    पूर्णांक         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    पूर्णांक         m_unroll_मोजणी = 64;  // main switch: --unroll-count
    पूर्णांक         m_unrollLimit = 16384;  // main switch: --unroll-limit
    पूर्णांक         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    पूर्णांक         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    पूर्णांक         m_verilateJobs = -1;  // main switch: --verilate-jobs

    पूर्णांक         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    पूर्णांक         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    पूर्णांक         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_शीर्ष_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_किल्ली;   // main switch: --protect-key
    string      m_शीर्ष_Module;    // main switch: --top-module
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
    bool m_f_Case_वृक्ष;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_वृक्ष;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ढकला_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    पूर्णांक  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    शून्य addArg(अक्षर** argv, माप_प्रकार मोजणी, bool isForRerun);
    शून्य addArg(स्थिरांक std::string& arg, bool isForRerun);
    शून्य add_ओळ_Arg(स्थिरांक string& arg);
    शून्य addDefine(स्थिरांक string& defline, bool allowPlus) VL_MT_DISABLED;
    शून्य addFuture(स्थिरांक string& flag);
    शून्य addFuture0(स्थिरांक string& flag);
    शून्य addFuture1(स्थिरांक string& flag);
    शून्य addIncDirUser(स्थिरांक string& incdir);  // User requested
    शून्य addIncDirFallback(स्थिरांक string& incdir);  // Low priority if not found otherwise
    शून्य addParameter(स्थिरांक string& paramline, bool allowPlus);
    शून्य addLangExt(स्थिरांक string& langext, स्थिरांक V3LangCode& lc);
    शून्य addLibExtV(स्थिरांक string& libext);
    शून्य optimize(पूर्णांक level);
    शून्य showVersion(bool verbose);
    शून्य coverage(bool flag) {
        m_coverage_ओळ = m_coverageBranch = m_coverageToggle = m_coverage_अभिव्यक्ती = m_coverageFsm
            = m_coverageUser = flag;
    }
    स्थिर bool suffixed(स्थिरांक string& sw, स्थिरांक अक्षर* arg);
    स्थिर string parseFileArg(स्थिरांक string& optdir, स्थिरांक string& relfilename);
    string filePathCheckOneDir(स्थिरांक string& modname, स्थिरांक string& स_नाम);
    स्थिर पूर्णांक stripOptionsForChildRun(स्थिरांक string& opt, bool for_शीर्ष);
    शून्य validateIdentifier(File_ओळ* fl, स्थिरांक string& arg, स्थिरांक string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

सार्वजनिक:
    V3Options();
    ~V3Options();
    शून्य setDebugMode(पूर्णांक level);
    अचिन्हित debugLevel(स्थिरांक string& tag) स्थिरांक VL_MT_SAFE;
    अचिन्हित debugSrcLevel(स्थिरांक string& srcfile_path) स्थिरांक VL_MT_SAFE;
    अचिन्हित dumpLevel(स्थिरांक string& tag) स्थिरांक VL_MT_SAFE;
    अचिन्हित dumpSrcLevel(स्थिरांक string& srcfile_path) स्थिरांक VL_MT_SAFE;

    // METHODS
    शून्य addCppFile(स्थिरांक string& filename);
    शून्य addCFlags(स्थिरांक string& filename);
    शून्य addCompilerIncludes(स्थिरांक string& filename);
    शून्य addLdLibFile(स्थिरांक string& filename);
    शून्य addLdLibs(स्थिरांक string& filename);
    शून्य addMakeFlags(स्थिरांक string& filename);
    शून्य addLibMapFile(स्थिरांक string& filename);
    शून्य addLibraryFile(स्थिरांक string& filename, स्थिरांक string& libname);
    शून्य addVFile(स्थिरांक string& filename, स्थिरांक string& libname);
    शून्य addVltFile(स्थिरांक string& filename, स्थिरांक string& libname);
    शून्य addForceInc(स्थिरांक string& filename);
    bool available() स्थिरांक VL_MT_SAFE { परत m_available; }
    शून्य ccSet();
    शून्य decorations(File_ओळ* fl, स्थिरांक string& filename);
    शून्य notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() स्थिरांक { परत m_preprocOnly; }
    bool makePhony() स्थिरांक { परत m_makePhony; }
    bool preproc_No_ओळ() स्थिरांक { परत m_preproc_No_ओळ; }
    bool preprocResolve() स्थिरांक { परत m_preprocResolve; }
    पूर्णांक preprocTokenLimit() स्थिरांक { परत m_preprocTokenLimit; }
    bool underlineZero() स्थिरांक { परत m_underlineZero; }
    bool systemC() स्थिरांक VL_MT_SAFE { परत m_systemC; }
    bool savable() स्थिरांक VL_MT_SAFE { परत m_savable; }
    VOptionBool schedZeroDelay() स्थिरांक { परत m_schedZeroDelay; }
    bool stats() स्थिरांक { परत m_stats; }
    bool statsVars() स्थिरांक { परत m_statsVars; }
    bool stdPackage() स्थिरांक { परत m_stdPackage; }
    bool stdWaiver() स्थिरांक { परत m_stdWaiver; }
    bool assertOn() स्थिरांक { परत m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() स्थिरांक { परत m_assertCase; }
    bool autoflush() स्थिरांक { परत m_autoflush; }
    bool bboxSys() स्थिरांक { परत m_bboxSys; }
    bool bboxUnsup() स्थिरांक { परत m_bboxUnsup; }
    bool build() स्थिरांक { परत m_build; }
    string buildDepBin() स्थिरांक { परत m_buildDepBin; }
    शून्य buildDepBin(स्थिरांक string& flag) { m_buildDepBin = flag; }
    bool context() स्थिरांक VL_MT_SAFE { परत m_context; }
    bool coverage() स्थिरांक VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        परत m_coverage_ओळ || m_coverageBranch || m_coverageToggle || m_coverage_अभिव्यक्ती
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() स्थिरांक VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        परत m_coverage_ओळ || m_coverageBranch || m_coverageToggle || m_coverage_अभिव्यक्ती
               || m_coverageUser;
    }
    bool coverageBranch() स्थिरांक { परत m_coverageBranch; }
    bool coverage_अभिव्यक्ती() स्थिरांक { परत m_coverage_अभिव्यक्ती; }
    bool coverageFsm() स्थिरांक { परत m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() स्थिरांक { परत m_coverageFsmExpand; }
    पूर्णांक coverageFsmMaxArcs() स्थिरांक { परत m_coverageFsmMaxArcs; }
    bool coverage_ओळ() स्थिरांक { परत m_coverage_ओळ; }
    bool coverageMergeInstances() स्थिरांक { परत m_coverageMergeInstances; }
    bool coveragePerInstance() स्थिरांक { परत m_coveragePerInstance; }
    bool coverageToggle() स्थिरांक { परत m_coverageToggle; }
    bool coverageUnderscore() स्थिरांक { परत m_coverageUnderscore; }
    bool coverageUser() स्थिरांक { परत m_coverageUser; }
    bool debugCheck() स्थिरांक VL_MT_SAFE { परत m_debugCheck; }
    bool debugCollision() स्थिरांक { परत m_debugCollision; }
    bool debugEmitV() स्थिरांक VL_MT_SAFE { परत m_debugEmitV; }
    bool debugExitElab() स्थिरांक { परत m_debugExitElab; }
    bool debugExitParse() स्थिरांक { परत m_debugExitParse; }
    bool debugLeak() स्थिरांक { परत m_debugLeak; }
    bool debugNondeterminism() स्थिरांक { परत m_debugNondeterminism; }
    bool debugPartition() स्थिरांक { परत m_debugPartition; }
    bool debugPreprocPassthru() स्थिरांक VL_MT_SAFE { परत m_debugPreprocPassthru; }
    bool debugProtect() स्थिरांक VL_MT_SAFE { परत m_debugProtect; }
    पूर्णांक debugRuntimeTimeout() स्थिरांक { परत m_debugRuntimeTimeout; }
    bool debugSelfTest() स्थिरांक { परत m_debugSelfTest; }
    bool debug_चळत_Check() स्थिरांक { परत m_debug_चळत_Check; }
    bool debugWidth() स्थिरांक VL_PURE { परत m_debugWidth; }
    bool decoration() स्थिरांक VL_MT_SAFE { परत m_decoration; }
    bool decorationNodes() स्थिरांक VL_MT_SAFE { परत m_decorationNodes; }
    bool diagnosticsSarif() स्थिरांक VL_MT_SAFE { परत m_diagnosticsSarif; }
    bool dpiHdrOnly() स्थिरांक { परत m_dpiHdrOnly; }
    bool dumpAstPatterns() स्थिरांक {
        परत m_dumpLevel.मोजणी("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() स्थिरांक { परत m_dumpLevel.मोजणी("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() स्थिरांक {
        परत m_dumpLevel.मोजणी("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_वृक्ष_Dot() स्थिरांक {
        परत m_dumpLevel.मोजणी("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() स्थिरांक { परत m_emitAccessors; }
    bool exe() स्थिरांक { परत m_exe; }
    bool flatten() स्थिरांक { परत m_flatten; }
    bool fourstate() स्थिरांक { परत m_fourstate; }
    bool gmake() स्थिरांक { परत m_gmake; }
    bool makeJson() स्थिरांक { परत m_makeJson; }
    bool threadsDpiPure() स्थिरांक { परत m_threadsDpiPure; }
    bool threadsDpiUnpure() स्थिरांक { परत m_threadsDpiUnpure; }
    bool threadsCoarsen() स्थिरांक { परत m_threadsCoarsen; }
    VOptionBool timing() स्थिरांक { परत m_timing; }
    bool trace() स्थिरांक { परत m_trace; }
    bool traceCoverage() स्थिरांक { परत m_traceCoverage; }
    bool traceEnabledFst() स्थिरांक { परत m_traceEnabledFst; }
    bool traceEnabledSaif() स्थिरांक { परत m_traceEnabledSaif; }
    bool traceEnabledVcd() स्थिरांक { परत m_traceEnabledVcd; }
    bool traceParams() स्थिरांक { परत m_traceParams; }
    bool traceStructs() स्थिरांक { परत m_traceStructs; }
    bool traceUnderscore() स्थिरांक { परत m_traceUnderscore; }
    bool main() स्थिरांक { परत m_main; }
    bool outFormatOk() स्थिरांक { परत m_outFormatOk; }
    bool jsonOnly() स्थिरांक { परत m_jsonOnly; }
    bool keepTempFiles() स्थिरांक { परत (V3Error::debugDefault() != 0); }
    bool pedantic() स्थिरांक { परत m_pedantic; }
    bool pinsInoutEnables() स्थिरांक { परत m_pinsInoutEnables; }
    bool pinsScUint() स्थिरांक { परत m_pinsScUint; }
    bool pinsScUintBool() स्थिरांक { परत m_pinsScUintBool; }
    bool pinsScBigUint() स्थिरांक VL_MT_SAFE { परत m_pinsScBigUint; }
    bool pinsUint8() स्थिरांक { परत m_pinsUint8; }
    bool preprocComments() स्थिरांक { परत m_preprocComments; }
    bool preprocDefines() स्थिरांक { परत m_preprocDefines; }
    bool profC() स्थिरांक { परत m_profC; }
    bool profCFuncs() स्थिरांक { परत m_profCFuncs; }
    bool profExec() स्थिरांक { परत m_profExec; }
    bool profPgo() स्थिरांक { परत m_profPgo; }
    bool usesProfiler() स्थिरांक { परत profExec() || profPgo(); }
    bool protectIds() स्थिरांक VL_MT_SAFE { परत m_protectIds; }
    bool allPublic() स्थिरांक { परत m_public; }
    bool publicParams() स्थिरांक { परत m_publicParams; }
    bool publicOff() स्थिरांक { परत m_publicIgnore; }
    bool publicFlatRW() स्थिरांक { परत m_publicFlatRW; }
    पूर्णांक publicDepth() स्थिरांक { परत m_publicDepth; }
    bool anyPublicFlat() स्थिरांक { परत m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() स्थिरांक VL_MT_SAFE { परत m_lintOnly; }
    bool ignc() स्थिरांक { परत m_ignc; }
    bool quietBuild() स्थिरांक VL_MT_SAFE { परत m_quietBuild; }
    bool quietExit() स्थिरांक VL_MT_SAFE { परत m_quietExit; }
    bool quietStats() स्थिरांक VL_MT_SAFE { परत m_quietStats; }
    bool reportUnoptflat() स्थिरांक { परत m_reportUnoptflat; }
    bool verilate() स्थिरांक { परत m_verilate; }
    bool vpi() स्थिरांक { परत m_vpi.isTrue(); }
    bool waiverMultiline() स्थिरांक { परत m_waiverMultiline; }
    bool xInitialEdge() स्थिरांक { परत m_xInitialEdge; }
    bool serializeOnly() स्थिरांक { परत m_jsonOnly; }
    bool शीर्ष_Ifaces_Supported() स्थिरांक { परत lintOnly() && !hierarchical(); }

    पूर्णांक buildJobs() स्थिरांक VL_MT_SAFE { परत m_buildJobs; }
    पूर्णांक convergeLimit() स्थिरांक { परत m_convergeLimit; }
    पूर्णांक coverage_अभिव्यक्ती_Max() स्थिरांक { परत m_coverage_अभिव्यक्ती_Max; }
    uint32_t coverageMaxBins() स्थिरांक { परत m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() स्थिरांक { परत m_coverageMaxRealBins; }
    पूर्णांक coverageMaxWidth() स्थिरांक { परत m_coverageMaxWidth; }
    पूर्णांक debugAllocRandom() स्थिरांक { परत m_debugAllocRandom; }
    bool dump_वृक्ष_Addrids() स्थिरांक VL_MT_SAFE;
    पूर्णांक expandLimit() स्थिरांक { परत m_expandLimit; }
    पूर्णांक gateStmts() स्थिरांक { परत m_gateStmts; }
    पूर्णांक ifDepth() स्थिरांक { परत m_ifDepth; }
    पूर्णांक inlineCFuncs() स्थिरांक { परत m_inlineCFuncs; }
    पूर्णांक inlineCFuncsProduct() स्थिरांक { परत m_inlineCFuncsProduct; }
    पूर्णांक inlineMult() स्थिरांक { परत m_inlineMult; }
    पूर्णांक instr_मोजणी_Dpi() स्थिरांक { परत m_instr_मोजणी_Dpi; }
    पूर्णांक localizeMaxSize() स्थिरांक { परत m_localizeMaxSize; }
    bool jsonEditNums() स्थिरांक { परत m_jsonEditNums; }
    bool jsonIds() स्थिरांक { परत m_jsonIds; }
    VOptionBool makeDepend() स्थिरांक { परत m_makeDepend; }
    पूर्णांक max_अंक_Width() स्थिरांक { परत m_max_अंक_Width; }
    पूर्णांक funcRecursionDepth() स्थिरांक { परत m_funcRecursion; }
    पूर्णांक moduleRecursionDepth() स्थिरांक { परत m_moduleRecursion; }
    पूर्णांक outputSplit() स्थिरांक { परत m_outputSplit; }
    पूर्णांक outputSplitCFuncs() स्थिरांक { परत m_outputSplitCFuncs; }
    पूर्णांक outputSplitCTrace() स्थिरांक { परत m_outputSplitCTrace; }
    पूर्णांक outputGroups() स्थिरांक { परत m_outputGroups; }
    int64_t outputGroupsSize() स्थिरांक { परत m_outputGroupsSize; }
    पूर्णांक pinsBv() स्थिरांक VL_MT_SAFE { परत m_pinsBv; }
    पूर्णांक reloopLimit() स्थिरांक { परत m_reloopLimit; }
    पूर्णांक replicationLimit() स्थिरांक { परत m_replicationLimit; }
    VOptionBool skipIdentical() स्थिरांक { परत m_skipIdentical; }
    bool stopFail() स्थिरांक { परत m_stopFail; }
    पूर्णांक threads() स्थिरांक VL_MT_SAFE { परत m_threads; }
    पूर्णांक threadsMaxMTasks() स्थिरांक { परत m_threadsMaxMTasks; }
    bool mtasks() स्थिरांक VL_MT_SAFE { परत (m_threads > 1); }
    VTimescale timeDefaultPrec() स्थिरांक { परत m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() स्थिरांक { परत m_timeDefaultUnit; }
    VTimescale timeOverridePrec() स्थिरांक { परत m_timeOverridePrec; }
    VTimescale timeOverrideUnit() स्थिरांक { परत m_timeOverrideUnit; }
    VTimescale timeComputePrec(स्थिरांक VTimescale& flag) स्थिरांक;
    VTimescale timeComputeUnit(स्थिरांक VTimescale& flag) स्थिरांक;
    पूर्णांक traceDepth() स्थिरांक { परत m_traceDepth; }
    पूर्णांक traceMaxArray() स्थिरांक { परत m_traceMaxArray; }
    पूर्णांक traceMaxWidth() स्थिरांक { परत m_traceMaxWidth; }
    bool useTraceParallel() स्थिरांक {
        परत trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    पूर्णांक unroll_मोजणी() स्थिरांक { परत m_unroll_मोजणी; }
    पूर्णांक unrollLimit() स्थिरांक { परत m_unrollLimit; }
    पूर्णांक unrollStmts() स्थिरांक { परत m_unrollStmts; }
    पूर्णांक constraintArrayLimit() स्थिरांक { परत m_constraintArrayLimit; }
    पूर्णांक verilateJobs() स्थिरांक { परत m_verilateJobs; }

    पूर्णांक compLimitBlocks() स्थिरांक { परत m_compLimitBlocks; }
    पूर्णांक compLimitMembers() स्थिरांक VL_MT_SAFE { परत m_compLimitMembers; }
    पूर्णांक compLimitParens() स्थिरांक { परत m_compLimitParens; }

    string diagnosticsSarifOutput() स्थिरांक VL_MT_SAFE {
        परत m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() स्थिरांक { परत m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() स्थिरांक { परत m_hierParamsFile; }
    string jsonOnlyOutput() स्थिरांक { परत m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() स्थिरांक { परत m_jsonOnlyMetaOutput; }
    string l2Name() स्थिरांक { परत m_l2Name; }
    string libCreate() स्थिरांक { परत m_libCreate; }
    string libCreateName(bool shared) स्थिरांक {
        string libName = "lib" + libCreate();
        जर (shared) {
            libName += ".so";
        } अन्यथा {
            libName += ".a";
        }
        परत libName;
    }
    string main_शीर्ष_Name() स्थिरांक { परत m_main_शीर्ष_Name; }
    string makeDir() स्थिरांक VL_MT_SAFE { परत m_makeDir; }
    string modPrefix() स्थिरांक VL_MT_SAFE { परत m_modPrefix; }
    string pipeFilter() स्थिरांक { परत m_pipeFilter; }
    string prefix() स्थिरांक VL_MT_SAFE { परत m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_किल्ली_Provided() स्थिरांक { परत !m_protect_किल्ली.empty(); }
    string protect_किल्ली_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string शीर्ष_Module() स्थिरांक { परत m_शीर्ष_Module; }  // As AstNode::encodeName()
    bool no_Trace_शीर्ष() स्थिरांक { परत m_no_Trace_शीर्ष; }
    string unusedRegexp() स्थिरांक { परत m_unusedRegexp; }
    string waiverOutput() स्थिरांक { परत m_waiverOutput; }
    string work() स्थिरांक { परत m_work; }
    bool isWaiverOutput() स्थिरांक { परत !m_waiverOutput.empty(); }
    string xAssign() स्थिरांक { परत m_xAssign; }
    string xInitial() स्थिरांक { परत m_xInitial; }

    स्थिरांक VStringSet& cppFiles() स्थिरांक { परत m_cppFiles; }
    स्थिरांक VStringList& cFlags() स्थिरांक { परत m_cFlags; }
    स्थिरांक VStringSet& compilerIncludes() स्थिरांक { परत m_compilerIncludes; }
    स्थिरांक VStringList& ldLibFiles() स्थिरांक { परत m_ldLibFiles; }
    स्थिरांक VStringList& ldLibs() स्थिरांक { परत m_ldLibs; }
    स्थिरांक VStringList& makeFlags() स्थिरांक { परत m_makeFlags; }
    स्थिरांक VFileLibSet& libraryFiles() स्थिरांक { परत m_libraryFiles; }
    स्थिरांक VStringSet& libmapFiles() स्थिरांक { परत m_libmapFiles; }
    स्थिरांक VFileLibList& vFiles() स्थिरांक { परत m_vFiles; }
    स्थिरांक VFileLibSet& vltFiles() स्थिरांक { परत m_vltFiles; }
    स्थिरांक VStringList& forceIncs() स्थिरांक { परत m_forceIncs; }

    bool hasParameter(स्थिरांक string& name);
    string parameter(स्थिरांक string& name);
    शून्य checkParameters();

    bool isFuture(स्थिरांक string& flag) स्थिरांक;
    bool isFuture0(स्थिरांक string& flag) स्थिरांक;
    bool isFuture1(स्थिरांक string& flag) स्थिरांक;
    bool isLibraryFile(स्थिरांक string& filename, स्थिरांक string& libname) स्थिरांक;

    // ACCESSORS (optimization options)
    bool fAcycSimp() स्थिरांक { परत m_fAcycSimp; }
    bool fAssemble() स्थिरांक { परत m_fAssemble; }
    bool fBitScanLoops() स्थिरांक { परत m_fBitScanLoops; }
    bool fBitblast() स्थिरांक { परत m_fBitblast; }
    bool fCaseDecoder() स्थिरांक { परत m_fCaseDecoder; }
    bool fCaseTable() स्थिरांक { परत m_fCaseTable; }
    bool f_Case_वृक्ष() स्थिरांक { परत m_f_Case_वृक्ष; }
    bool fCombine() स्थिरांक { परत m_fCombine; }
    bool fConst() स्थिरांक { परत m_fConst; }
    bool fConstBeforeDfg() स्थिरांक { परत m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_वृक्ष() स्थिरांक { परत m_f_Const_Bit_Op_वृक्ष; }
    bool fConstEager() स्थिरांक { परत m_fConstEager; }
    bool fDedupe() स्थिरांक { परत m_fDedupe; }
    bool fDfg() स्थिरांक { परत m_fDfg; }
    bool fDfgPeephole() स्थिरांक { परत m_fDfgPeephole; }
    bool f_Dfg_ढकला_Down_Sels() स्थिरांक { परत m_f_Dfg_ढकला_Down_Sels; }
    bool fDfgSynthesizeAll() स्थिरांक { परत m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(स्थिरांक std::string& name) स्थिरांक {
        परत !m_fDfgPeepholeDisabled.मोजणी(name);
    }
    bool fDeadAssigns() स्थिरांक { परत m_fDeadAssigns; }
    bool fDeadCells() स्थिरांक { परत m_fDeadCells; }
    bool fDeadMethods() स्थिरांक { परत m_fDeadMethods; }
    bool fDecompose() स्थिरांक { परत m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() स्थिरांक { परत m_fDecomposePacked; }
    bool fDecomposeUnpacked() स्थिरांक { परत m_fDecomposeUnpacked; }
    bool fExpand() स्थिरांक { परत m_fExpand; }
    bool fFuncBalanceCat() स्थिरांक { परत m_fFuncBalanceCat; }
    bool fFuncSplitCat() स्थिरांक { परत m_fFuncSplitCat; }
    bool fFunc() स्थिरांक { परत fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() स्थिरांक { परत m_fGate; }
    VOptionBool fIcoChangeDetect() स्थिरांक { परत m_fIcoChangeDetect; }
    bool fInline() स्थिरांक { परत m_fInline; }
    bool fInlineCFuncs() स्थिरांक { परत m_fInlineCFuncs; }
    bool fInlineFuncs() स्थिरांक { परत m_fInlineFuncs; }
    bool fInlineFuncsEager() स्थिरांक { परत m_fInlineFuncsEager; }
    bool fLife() स्थिरांक { परत m_fLife; }
    bool fLifePost() स्थिरांक { परत m_fLifePost; }
    bool fLocalize() स्थिरांक { परत m_fLocalize; }
    bool fMergeCond() स्थिरांक { परत m_fMergeCond; }
    bool fMergeCondMotion() स्थिरांक { परत m_fMergeCondMotion; }
    bool fReloop() स्थिरांक { परत m_fReloop; }
    bool fReorder() स्थिरांक { परत m_fReorder; }
    bool fSlice() स्थिरांक { परत m_fSlice; }
    पूर्णांक fSliceElementLimit() स्थिरांक { परत m_fSliceElementLimit; }
    bool fSplit() स्थिरांक { परत m_fSplit; }
    bool fSubst() स्थिरांक { परत m_fSubst; }
    bool fSubstConst() स्थिरांक { परत m_fSubstConst; }
    bool fTable() स्थिरांक { परत m_fTable; }
    bool fTaskifyAll() स्थिरांक { परत m_fTaskifyAll; }

    std::string traceClassBase() स्थिरांक VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() स्थिरांक VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() स्थिरांक VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() स्थिरांक VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() स्थिरांक VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() स्थिरांक VL_MT_SAFE;

    bool hierarchical() स्थिरांक { परत m_hierarchical; }
    पूर्णांक hierChild() स्थिरांक VL_MT_SAFE { परत m_hierChild; }
    पूर्णांक hierThreads() स्थिरांक VL_MT_SAFE { परत m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_शीर्ष() स्थिरांक VL_MT_SAFE { परत !m_hierChild && !m_hierBlocks.empty(); }
    स्थिरांक V3HierBlockOptSet& hierBlocks() स्थिरांक { परत m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_शीर्ष_Data_Dir() स्थिरांक VL_MT_SAFE {
        परत hier_शीर्ष() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    स्थिर string version() VL_PURE;
    स्थिर string argString(पूर्णांक argc, अक्षर** argv);  ///< Return list of arguments as simple string
    string allArgsString() स्थिरांक VL_MT_SAFE;  ///< Return all passed arguments as simple string
    स्थिरांक std::list<std::pair<std::list<std::string>, bool>>& allArgs() स्थिरांक;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_शीर्ष) स्थिरांक;
    शून्य parseOpts(File_ओळ* fl, पूर्णांक argc, अक्षर** argv) VL_MT_DISABLED;
    शून्य parseOptsList(File_ओळ* fl, स्थिरांक string& optdir, पूर्णांक argc, अक्षर** argv) VL_MT_DISABLED;
    शून्य parseOptsFile(File_ओळ* fl, स्थिरांक string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    स्थिर string getenvBuiltins(स्थिरांक string& var);
    स्थिर string getenvMAKE();
    स्थिर string getenvMAKEFLAGS();
    स्थिर string getenvPERL();
    स्थिर string getenvPYTHON3();
    स्थिर string getenvSYSTEMC();
    स्थिर string getenvSYSTEMC_ARCH();
    स्थिर string getenvSYSTEMC_INCLUDE();
    स्थिर string getenvSYSTEMC_LIBDIR();
    स्थिर string getenv_VERILATOR_मूळ();
    स्थिर string getenvVERILATOR_SOLVER();
    स्थिर string getStdPackagePath();
    स्थिर string getStdWaiverPath();
    स्थिर string getSupported(स्थिरांक string& var);
    स्थिर bool systemCSystemWide();
    स्थिर bool systemCFound();  // SystemC installed, or environment points to it
    स्थिर bool coroutineSupport();  // Configured C++ compiler supports coroutines
    स्थिर bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    स्थिर bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    स्थिर bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(स्थिरांक string& filename);
    string filePath(File_ओळ* fl, स्थिरांक string& modname, स्थिरांक string& lastpath,
                    स्थिरांक string& errmsg);
    string file_Path_Looked_संदेश(File_ओळ* fl, स्थिरांक string& modname);
    V3LangCode fileLanguage(स्थिरांक string& filename);
    स्थिर bool fileStatNormal(स्थिरांक string& filename);

    // METHODS (other OS)
    स्थिर शून्य throwSigsegv();
};

//######################################################################

#endif  // guard
