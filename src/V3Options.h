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

#अगर_अघोषित VERILATOR_V3OPTIONS_H_
#घोषणा VERILATOR_V3OPTIONS_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3LangCode.h"

#समावेश <list>
#समावेश <map>
#समावेश <set>
#समावेश <string>
#समावेश <vector>

class V3OptionsImp;
class File_पंक्ति;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
खुला:
    क्रमागत en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    क्रमागत en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    स्थिर अक्षर* ascii() स्थिर {
        अटल स्थिर अक्षर* स्थिर names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        वापस names[m_e];
    }
    bool isDefault() स्थिर { वापस m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() स्थिर { वापस m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() स्थिर { वापस m_e == OPT_TRUE; }
    bool isSetFalse() स्थिर { वापस m_e == OPT_FALSE; }
    व्योम setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool चालक==(स्थिर VOptionBool& lhs, स्थिर VOptionBool& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VOptionBool& lhs, VOptionBool::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VOptionBool::en lhs, स्थिर VOptionBool& rhs) { वापस lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    स्थिर string m_filename;  // Filename
    स्थिर string m_libname;  // Libname
खुला:
    VFileLibName(स्थिर string& filename, स्थिर string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(स्थिर VFileLibName& rhs) = शेष;
    string filename() स्थिर { वापस m_filename; }
    string libname() स्थिर { वापस m_libname; }
    bool चालक==(स्थिर VFileLibName& rhs) स्थिर {
        वापस m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool चालक<(स्थिर VFileLibName& rhs) स्थिर {
        अगर (m_filename < rhs.m_filename) वापस true;
        अगर (m_filename > rhs.m_filename) वापस false;
        वापस m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
खुला:
    क्रमागत en : uint8_t {
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
    क्रमागत : uint8_t { TS_DEFAULT = TS_1PS };
    क्रमागत en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(स्थिर string& मूल्य, bool& badr);
    VTimescale(द्विगुन मूल्य, bool& badr) {
        badr = false;
        क्रम (पूर्णांक i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            अगर (multiplier() == मूल्य) अवरोध;
        }
        अगर (multiplier() != मूल्य) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() स्थिर { वापस m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    अटल व्योम parseSlashed(File_पंक्ति* fl, स्थिर अक्षर* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    स्थिर अक्षर* ascii() स्थिर {
        अटल स्थिर अक्षर* स्थिर names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        वापस names[m_e];
    }
    पूर्णांक powerOfTen() स्थिर { वापस 2 - static_cast<पूर्णांक>(m_e); }
    द्विगुन multiplier() स्थिर {
        अटल स्थिर द्विगुन मूल्यावली[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        वापस मूल्यावली[m_e];
    }
};
constexpr bool चालक==(स्थिर VTimescale& lhs, स्थिर VTimescale& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VTimescale& lhs, VTimescale::en rhs) { वापस lhs.m_e == rhs; }
constexpr bool चालक==(VTimescale::en lhs, स्थिर VTimescale& rhs) { वापस lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool चालक<(स्थिर VTimescale& lhs, स्थिर VTimescale& rhs) {
    वापस lhs.m_e > rhs.m_e;
}
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VTimescale& rhs) {
    वापस os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
खुला:
    क्रमागत en : uint8_t {
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
    constexpr चालक en() स्थिर { वापस m_e; }
    constexpr स्थिर अक्षर* ascii() स्थिर {
        constexpr स्थिर अक्षर* स्थिर names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        वापस names[m_e];
    }
};
constexpr bool चालक==(स्थिर VFsmExpandType& lhs, स्थिर VFsmExpandType& rhs) VL_MT_SAFE {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VFsmExpandType::en lhs, स्थिर VFsmExpandType& rhs) VL_MT_SAFE {
    वापस lhs == rhs.m_e;
}
अंतरभूत std::ostream& चालक<<(std::ostream& os, स्थिर VFsmExpandType& rhs) VL_MT_SAFE {
    वापस os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
खुला:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<स्थिर std::string, std::string>;

निजी:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

खुला:
    explicit V3HierarchicalBlockOption(स्थिर string& optstring);
    स्थिर string& origName() स्थिर { वापस m_origName; }
    स्थिर string& mangledName() स्थिर { वापस m_mangledName; }
    स्थिर ParamStrMap params() स्थिर { वापस m_parameters; }
};

using V3HierBlockOptSet = std::map<स्थिर std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
खुला:
निजी:
    // TYPES
    using DebugLevelMap = std::map<स्थिर std::string, अचिन्हित>;

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
    std::map<स्थिर string, string> m_parameters;  // Parameters
    std::map<स्थिर string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_पंक्ति = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_द्विआधारी = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_व्यंजक = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_पंक्ति = false;    // main switch: --coverage-block
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
    bool m_debug_ढेर_Check = false;  // main switch: --debug-stack-check
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
    पूर्णांक         m_coverage_व्यंजक_Max = 32;    // main switch: --coverage-expr-max
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
    पूर्णांक         m_instr_गिनती_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    पूर्णांक         m_localize_Max_आकार = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    पूर्णांक         m_max_अंक_Width = 65536;  // main switch: --max-num-width
    पूर्णांक         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    पूर्णांक         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    पूर्णांक         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_output_Groups_आकार = 512 * 1024 * 1024;  // main switch: --output-groups-size
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
    पूर्णांक         m_unroll_गिनती = 64;  // main switch: --unroll-count
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
    string      m_protect_कुंजी;   // main switch: --protect-key
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
    bool m_f_Dfg_धकेलो_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    व्योम addArg(अक्षर** argv, माप_प्रकार गिनती, bool isForRerun);
    व्योम addArg(स्थिर std::string& arg, bool isForRerun);
    व्योम add_पंक्ति_Arg(स्थिर string& arg);
    व्योम addDefine(स्थिर string& defline, bool allowPlus) VL_MT_DISABLED;
    व्योम addFuture(स्थिर string& flag);
    व्योम addFuture0(स्थिर string& flag);
    व्योम addFuture1(स्थिर string& flag);
    व्योम addIncDirUser(स्थिर string& incdir);  // User requested
    व्योम addIncDirFallback(स्थिर string& incdir);  // Low priority if not found otherwise
    व्योम addParameter(स्थिर string& paramline, bool allowPlus);
    व्योम addLangExt(स्थिर string& langext, स्थिर V3LangCode& lc);
    व्योम addLibExtV(स्थिर string& libext);
    व्योम optimize(पूर्णांक level);
    व्योम showVersion(bool verbose);
    व्योम coverage(bool flag) {
        m_coverage_पंक्ति = m_coverageBranch = m_coverageToggle = m_coverage_व्यंजक = m_coverageFsm
            = m_coverageUser = flag;
    }
    अटल bool suffixed(स्थिर string& sw, स्थिर अक्षर* arg);
    अटल string parseFileArg(स्थिर string& optdir, स्थिर string& relfilename);
    string filePathCheckOneDir(स्थिर string& modname, स्थिर string& स_नाम);
    अटल पूर्णांक stripOptionsForChildRun(स्थिर string& opt, bool for_शीर्ष);
    व्योम validateIdentifier(File_पंक्ति* fl, स्थिर string& arg, स्थिर string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

खुला:
    V3Options();
    ~V3Options();
    व्योम setDebugMode(पूर्णांक level);
    अचिन्हित debugLevel(स्थिर string& tag) स्थिर VL_MT_SAFE;
    अचिन्हित debugSrcLevel(स्थिर string& srcfile_path) स्थिर VL_MT_SAFE;
    अचिन्हित dumpLevel(स्थिर string& tag) स्थिर VL_MT_SAFE;
    अचिन्हित dumpSrcLevel(स्थिर string& srcfile_path) स्थिर VL_MT_SAFE;

    // METHODS
    व्योम addCppFile(स्थिर string& filename);
    व्योम addCFlags(स्थिर string& filename);
    व्योम addCompilerIncludes(स्थिर string& filename);
    व्योम addLdLibFile(स्थिर string& filename);
    व्योम addLdLibs(स्थिर string& filename);
    व्योम addMakeFlags(स्थिर string& filename);
    व्योम addLibMapFile(स्थिर string& filename);
    व्योम addLibraryFile(स्थिर string& filename, स्थिर string& libname);
    व्योम addVFile(स्थिर string& filename, स्थिर string& libname);
    व्योम addVltFile(स्थिर string& filename, स्थिर string& libname);
    व्योम addForceInc(स्थिर string& filename);
    bool available() स्थिर VL_MT_SAFE { वापस m_available; }
    व्योम ccSet();
    व्योम decorations(File_पंक्ति* fl, स्थिर string& filename);
    व्योम notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() स्थिर { वापस m_preprocOnly; }
    bool makePhony() स्थिर { वापस m_makePhony; }
    bool preproc_No_पंक्ति() स्थिर { वापस m_preproc_No_पंक्ति; }
    bool preprocResolve() स्थिर { वापस m_preprocResolve; }
    पूर्णांक preprocTokenLimit() स्थिर { वापस m_preprocTokenLimit; }
    bool underlineZero() स्थिर { वापस m_underlineZero; }
    bool systemC() स्थिर VL_MT_SAFE { वापस m_systemC; }
    bool savable() स्थिर VL_MT_SAFE { वापस m_savable; }
    VOptionBool schedZeroDelay() स्थिर { वापस m_schedZeroDelay; }
    bool stats() स्थिर { वापस m_stats; }
    bool statsVars() स्थिर { वापस m_statsVars; }
    bool stdPackage() स्थिर { वापस m_stdPackage; }
    bool stdWaiver() स्थिर { वापस m_stdWaiver; }
    bool assertOn() स्थिर { वापस m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() स्थिर { वापस m_assertCase; }
    bool autoflush() स्थिर { वापस m_autoflush; }
    bool bboxSys() स्थिर { वापस m_bboxSys; }
    bool bboxUnsup() स्थिर { वापस m_bboxUnsup; }
    bool build() स्थिर { वापस m_build; }
    string buildDepBin() स्थिर { वापस m_buildDepBin; }
    व्योम buildDepBin(स्थिर string& flag) { m_buildDepBin = flag; }
    bool context() स्थिर VL_MT_SAFE { वापस m_context; }
    bool coverage() स्थिर VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        वापस m_coverage_पंक्ति || m_coverageBranch || m_coverageToggle || m_coverage_व्यंजक
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() स्थिर VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        वापस m_coverage_पंक्ति || m_coverageBranch || m_coverageToggle || m_coverage_व्यंजक
               || m_coverageUser;
    }
    bool coverageBranch() स्थिर { वापस m_coverageBranch; }
    bool coverage_व्यंजक() स्थिर { वापस m_coverage_व्यंजक; }
    bool coverageFsm() स्थिर { वापस m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() स्थिर { वापस m_coverageFsmExpand; }
    पूर्णांक coverageFsmMaxArcs() स्थिर { वापस m_coverageFsmMaxArcs; }
    bool coverage_पंक्ति() स्थिर { वापस m_coverage_पंक्ति; }
    bool coverageMergeInstances() स्थिर { वापस m_coverageMergeInstances; }
    bool coveragePerInstance() स्थिर { वापस m_coveragePerInstance; }
    bool coverageToggle() स्थिर { वापस m_coverageToggle; }
    bool coverageUnderscore() स्थिर { वापस m_coverageUnderscore; }
    bool coverageUser() स्थिर { वापस m_coverageUser; }
    bool debugCheck() स्थिर VL_MT_SAFE { वापस m_debugCheck; }
    bool debugCollision() स्थिर { वापस m_debugCollision; }
    bool debugEmitV() स्थिर VL_MT_SAFE { वापस m_debugEmitV; }
    bool debugExitElab() स्थिर { वापस m_debugExitElab; }
    bool debugExitParse() स्थिर { वापस m_debugExitParse; }
    bool debugLeak() स्थिर { वापस m_debugLeak; }
    bool debugNondeterminism() स्थिर { वापस m_debugNondeterminism; }
    bool debugPartition() स्थिर { वापस m_debugPartition; }
    bool debugPreprocPassthru() स्थिर VL_MT_SAFE { वापस m_debugPreprocPassthru; }
    bool debugProtect() स्थिर VL_MT_SAFE { वापस m_debugProtect; }
    पूर्णांक debugRuntimeTimeout() स्थिर { वापस m_debugRuntimeTimeout; }
    bool debugSelfTest() स्थिर { वापस m_debugSelfTest; }
    bool debug_ढेर_Check() स्थिर { वापस m_debug_ढेर_Check; }
    bool debugWidth() स्थिर VL_PURE { वापस m_debugWidth; }
    bool decoration() स्थिर VL_MT_SAFE { वापस m_decoration; }
    bool decorationNodes() स्थिर VL_MT_SAFE { वापस m_decorationNodes; }
    bool diagnosticsSarif() स्थिर VL_MT_SAFE { वापस m_diagnosticsSarif; }
    bool dpiHdrOnly() स्थिर { वापस m_dpiHdrOnly; }
    bool dumpAstPatterns() स्थिर {
        वापस m_dumpLevel.गिनती("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() स्थिर { वापस m_dumpLevel.गिनती("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() स्थिर {
        वापस m_dumpLevel.गिनती("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_वृक्ष_Dot() स्थिर {
        वापस m_dumpLevel.गिनती("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() स्थिर { वापस m_emitAccessors; }
    bool exe() स्थिर { वापस m_exe; }
    bool flatten() स्थिर { वापस m_flatten; }
    bool fourstate() स्थिर { वापस m_fourstate; }
    bool gmake() स्थिर { वापस m_gmake; }
    bool makeJson() स्थिर { वापस m_makeJson; }
    bool threadsDpiPure() स्थिर { वापस m_threadsDpiPure; }
    bool threadsDpiUnpure() स्थिर { वापस m_threadsDpiUnpure; }
    bool threadsCoarsen() स्थिर { वापस m_threadsCoarsen; }
    VOptionBool timing() स्थिर { वापस m_timing; }
    bool trace() स्थिर { वापस m_trace; }
    bool traceCoverage() स्थिर { वापस m_traceCoverage; }
    bool traceEnabledFst() स्थिर { वापस m_traceEnabledFst; }
    bool traceEnabledSaif() स्थिर { वापस m_traceEnabledSaif; }
    bool traceEnabledVcd() स्थिर { वापस m_traceEnabledVcd; }
    bool traceParams() स्थिर { वापस m_traceParams; }
    bool traceStructs() स्थिर { वापस m_traceStructs; }
    bool traceUnderscore() स्थिर { वापस m_traceUnderscore; }
    bool मुख्य() स्थिर { वापस m_main; }
    bool outFormatOk() स्थिर { वापस m_outFormatOk; }
    bool jsonOnly() स्थिर { वापस m_jsonOnly; }
    bool keep_अस्थायी_Files() स्थिर { वापस (V3Error::debugDefault() != 0); }
    bool pedantic() स्थिर { वापस m_pedantic; }
    bool pinsInoutEnables() स्थिर { वापस m_pinsInoutEnables; }
    bool pinsScUint() स्थिर { वापस m_pinsScUint; }
    bool pinsScUintBool() स्थिर { वापस m_pinsScUintBool; }
    bool pinsScBigUint() स्थिर VL_MT_SAFE { वापस m_pinsScBigUint; }
    bool pinsUint8() स्थिर { वापस m_pinsUint8; }
    bool preprocComments() स्थिर { वापस m_preprocComments; }
    bool preprocDefines() स्थिर { वापस m_preprocDefines; }
    bool profC() स्थिर { वापस m_profC; }
    bool profCFuncs() स्थिर { वापस m_profCFuncs; }
    bool profExec() स्थिर { वापस m_profExec; }
    bool profPgo() स्थिर { वापस m_profPgo; }
    bool usesProfiler() स्थिर { वापस profExec() || profPgo(); }
    bool protectIds() स्थिर VL_MT_SAFE { वापस m_protectIds; }
    bool allPublic() स्थिर { वापस m_public; }
    bool publicParams() स्थिर { वापस m_publicParams; }
    bool publicOff() स्थिर { वापस m_publicIgnore; }
    bool publicFlatRW() स्थिर { वापस m_publicFlatRW; }
    पूर्णांक publicDepth() स्थिर { वापस m_publicDepth; }
    bool anyPublicFlat() स्थिर { वापस m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() स्थिर VL_MT_SAFE { वापस m_lintOnly; }
    bool ignc() स्थिर { वापस m_ignc; }
    bool quietBuild() स्थिर VL_MT_SAFE { वापस m_quietBuild; }
    bool quietExit() स्थिर VL_MT_SAFE { वापस m_quietExit; }
    bool quietStats() स्थिर VL_MT_SAFE { वापस m_quietStats; }
    bool reportUnoptflat() स्थिर { वापस m_reportUnoptflat; }
    bool verilate() स्थिर { वापस m_verilate; }
    bool vpi() स्थिर { वापस m_vpi.isTrue(); }
    bool waiverMultiline() स्थिर { वापस m_waiverMultiline; }
    bool xInitialEdge() स्थिर { वापस m_xInitialEdge; }
    bool serializeOnly() स्थिर { वापस m_jsonOnly; }
    bool शीर्ष_Ifaces_Supported() स्थिर { वापस lintOnly() && !hierarchical(); }

    पूर्णांक buildJobs() स्थिर VL_MT_SAFE { वापस m_buildJobs; }
    पूर्णांक convergeLimit() स्थिर { वापस m_convergeLimit; }
    पूर्णांक coverage_व्यंजक_Max() स्थिर { वापस m_coverage_व्यंजक_Max; }
    uint32_t coverageMaxBins() स्थिर { वापस m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() स्थिर { वापस m_coverageMaxRealBins; }
    पूर्णांक coverageMaxWidth() स्थिर { वापस m_coverageMaxWidth; }
    पूर्णांक debugAllocRandom() स्थिर { वापस m_debugAllocRandom; }
    bool dump_वृक्ष_Addrids() स्थिर VL_MT_SAFE;
    पूर्णांक expandLimit() स्थिर { वापस m_expandLimit; }
    पूर्णांक gateStmts() स्थिर { वापस m_gateStmts; }
    पूर्णांक ifDepth() स्थिर { वापस m_ifDepth; }
    पूर्णांक inlineCFuncs() स्थिर { वापस m_inlineCFuncs; }
    पूर्णांक inlineCFuncsProduct() स्थिर { वापस m_inlineCFuncsProduct; }
    पूर्णांक inlineMult() स्थिर { वापस m_inlineMult; }
    पूर्णांक instr_गिनती_Dpi() स्थिर { वापस m_instr_गिनती_Dpi; }
    पूर्णांक localize_Max_आकार() स्थिर { वापस m_localize_Max_आकार; }
    bool jsonEditNums() स्थिर { वापस m_jsonEditNums; }
    bool jsonIds() स्थिर { वापस m_jsonIds; }
    VOptionBool makeDepend() स्थिर { वापस m_makeDepend; }
    पूर्णांक max_अंक_Width() स्थिर { वापस m_max_अंक_Width; }
    पूर्णांक funcRecursionDepth() स्थिर { वापस m_funcRecursion; }
    पूर्णांक moduleRecursionDepth() स्थिर { वापस m_moduleRecursion; }
    पूर्णांक outputSplit() स्थिर { वापस m_outputSplit; }
    पूर्णांक outputSplitCFuncs() स्थिर { वापस m_outputSplitCFuncs; }
    पूर्णांक outputSplitCTrace() स्थिर { वापस m_outputSplitCTrace; }
    पूर्णांक outputGroups() स्थिर { वापस m_outputGroups; }
    int64_t output_Groups_आकार() स्थिर { वापस m_output_Groups_आकार; }
    पूर्णांक pinsBv() स्थिर VL_MT_SAFE { वापस m_pinsBv; }
    पूर्णांक reloopLimit() स्थिर { वापस m_reloopLimit; }
    पूर्णांक replicationLimit() स्थिर { वापस m_replicationLimit; }
    VOptionBool skipIdentical() स्थिर { वापस m_skipIdentical; }
    bool stopFail() स्थिर { वापस m_stopFail; }
    पूर्णांक threads() स्थिर VL_MT_SAFE { वापस m_threads; }
    पूर्णांक threadsMaxMTasks() स्थिर { वापस m_threadsMaxMTasks; }
    bool mtasks() स्थिर VL_MT_SAFE { वापस (m_threads > 1); }
    VTimescale timeDefaultPrec() स्थिर { वापस m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() स्थिर { वापस m_timeDefaultUnit; }
    VTimescale timeOverridePrec() स्थिर { वापस m_timeOverridePrec; }
    VTimescale timeOverrideUnit() स्थिर { वापस m_timeOverrideUnit; }
    VTimescale timeComputePrec(स्थिर VTimescale& flag) स्थिर;
    VTimescale timeComputeUnit(स्थिर VTimescale& flag) स्थिर;
    पूर्णांक traceDepth() स्थिर { वापस m_traceDepth; }
    पूर्णांक traceMaxArray() स्थिर { वापस m_traceMaxArray; }
    पूर्णांक traceMaxWidth() स्थिर { वापस m_traceMaxWidth; }
    bool useTraceParallel() स्थिर {
        वापस trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    पूर्णांक unroll_गिनती() स्थिर { वापस m_unroll_गिनती; }
    पूर्णांक unrollLimit() स्थिर { वापस m_unrollLimit; }
    पूर्णांक unrollStmts() स्थिर { वापस m_unrollStmts; }
    पूर्णांक constraintArrayLimit() स्थिर { वापस m_constraintArrayLimit; }
    पूर्णांक verilateJobs() स्थिर { वापस m_verilateJobs; }

    पूर्णांक compLimitBlocks() स्थिर { वापस m_compLimitBlocks; }
    पूर्णांक compLimitMembers() स्थिर VL_MT_SAFE { वापस m_compLimitMembers; }
    पूर्णांक compLimitParens() स्थिर { वापस m_compLimitParens; }

    string diagnosticsSarifOutput() स्थिर VL_MT_SAFE {
        वापस m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() स्थिर { वापस m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() स्थिर { वापस m_hierParamsFile; }
    string jsonOnlyOutput() स्थिर { वापस m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() स्थिर { वापस m_jsonOnlyMetaOutput; }
    string l2Name() स्थिर { वापस m_l2Name; }
    string libCreate() स्थिर { वापस m_libCreate; }
    string libCreateName(bool shared) स्थिर {
        string libName = "lib" + libCreate();
        अगर (shared) {
            libName += ".so";
        } अन्यथा {
            libName += ".a";
        }
        वापस libName;
    }
    string main_शीर्ष_Name() स्थिर { वापस m_main_शीर्ष_Name; }
    string makeDir() स्थिर VL_MT_SAFE { वापस m_makeDir; }
    string modPrefix() स्थिर VL_MT_SAFE { वापस m_modPrefix; }
    string pipeFilter() स्थिर { वापस m_pipeFilter; }
    string prefix() स्थिर VL_MT_SAFE { वापस m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_कुंजी_Provided() स्थिर { वापस !m_protect_कुंजी.empty(); }
    string protect_कुंजी_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string शीर्ष_Module() स्थिर { वापस m_शीर्ष_Module; }  // As AstNode::encodeName()
    bool no_Trace_शीर्ष() स्थिर { वापस m_no_Trace_शीर्ष; }
    string unusedRegexp() स्थिर { वापस m_unusedRegexp; }
    string waiverOutput() स्थिर { वापस m_waiverOutput; }
    string work() स्थिर { वापस m_work; }
    bool isWaiverOutput() स्थिर { वापस !m_waiverOutput.empty(); }
    string xAssign() स्थिर { वापस m_xAssign; }
    string xInitial() स्थिर { वापस m_xInitial; }

    स्थिर VStringSet& cppFiles() स्थिर { वापस m_cppFiles; }
    स्थिर VStringList& cFlags() स्थिर { वापस m_cFlags; }
    स्थिर VStringSet& compilerIncludes() स्थिर { वापस m_compilerIncludes; }
    स्थिर VStringList& ldLibFiles() स्थिर { वापस m_ldLibFiles; }
    स्थिर VStringList& ldLibs() स्थिर { वापस m_ldLibs; }
    स्थिर VStringList& makeFlags() स्थिर { वापस m_makeFlags; }
    स्थिर VFileLibSet& libraryFiles() स्थिर { वापस m_libraryFiles; }
    स्थिर VStringSet& libmapFiles() स्थिर { वापस m_libmapFiles; }
    स्थिर VFileLibList& vFiles() स्थिर { वापस m_vFiles; }
    स्थिर VFileLibSet& vltFiles() स्थिर { वापस m_vltFiles; }
    स्थिर VStringList& forceIncs() स्थिर { वापस m_forceIncs; }

    bool hasParameter(स्थिर string& name);
    string parameter(स्थिर string& name);
    व्योम checkParameters();

    bool isFuture(स्थिर string& flag) स्थिर;
    bool isFuture0(स्थिर string& flag) स्थिर;
    bool isFuture1(स्थिर string& flag) स्थिर;
    bool isLibraryFile(स्थिर string& filename, स्थिर string& libname) स्थिर;

    // ACCESSORS (optimization options)
    bool fAcycSimp() स्थिर { वापस m_fAcycSimp; }
    bool fAssemble() स्थिर { वापस m_fAssemble; }
    bool fBitScanLoops() स्थिर { वापस m_fBitScanLoops; }
    bool fBitblast() स्थिर { वापस m_fBitblast; }
    bool fCaseDecoder() स्थिर { वापस m_fCaseDecoder; }
    bool fCaseTable() स्थिर { वापस m_fCaseTable; }
    bool f_Case_वृक्ष() स्थिर { वापस m_f_Case_वृक्ष; }
    bool fCombine() स्थिर { वापस m_fCombine; }
    bool fConst() स्थिर { वापस m_fConst; }
    bool fConstBeforeDfg() स्थिर { वापस m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_वृक्ष() स्थिर { वापस m_f_Const_Bit_Op_वृक्ष; }
    bool fConstEager() स्थिर { वापस m_fConstEager; }
    bool fDedupe() स्थिर { वापस m_fDedupe; }
    bool fDfg() स्थिर { वापस m_fDfg; }
    bool fDfgPeephole() स्थिर { वापस m_fDfgPeephole; }
    bool f_Dfg_धकेलो_Down_Sels() स्थिर { वापस m_f_Dfg_धकेलो_Down_Sels; }
    bool fDfgSynthesizeAll() स्थिर { वापस m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(स्थिर std::string& name) स्थिर {
        वापस !m_fDfgPeepholeDisabled.गिनती(name);
    }
    bool fDeadAssigns() स्थिर { वापस m_fDeadAssigns; }
    bool fDeadCells() स्थिर { वापस m_fDeadCells; }
    bool fDeadMethods() स्थिर { वापस m_fDeadMethods; }
    bool fDecompose() स्थिर { वापस m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() स्थिर { वापस m_fDecomposePacked; }
    bool fDecomposeUnpacked() स्थिर { वापस m_fDecomposeUnpacked; }
    bool fExpand() स्थिर { वापस m_fExpand; }
    bool fFuncBalanceCat() स्थिर { वापस m_fFuncBalanceCat; }
    bool fFuncSplitCat() स्थिर { वापस m_fFuncSplitCat; }
    bool fFunc() स्थिर { वापस fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() स्थिर { वापस m_fGate; }
    VOptionBool fIcoChangeDetect() स्थिर { वापस m_fIcoChangeDetect; }
    bool fInline() स्थिर { वापस m_fInline; }
    bool fInlineCFuncs() स्थिर { वापस m_fInlineCFuncs; }
    bool fInlineFuncs() स्थिर { वापस m_fInlineFuncs; }
    bool fInlineFuncsEager() स्थिर { वापस m_fInlineFuncsEager; }
    bool fLife() स्थिर { वापस m_fLife; }
    bool fLifePost() स्थिर { वापस m_fLifePost; }
    bool fLocalize() स्थिर { वापस m_fLocalize; }
    bool fMergeCond() स्थिर { वापस m_fMergeCond; }
    bool fMergeCondMotion() स्थिर { वापस m_fMergeCondMotion; }
    bool fReloop() स्थिर { वापस m_fReloop; }
    bool fReorder() स्थिर { वापस m_fReorder; }
    bool fSlice() स्थिर { वापस m_fSlice; }
    पूर्णांक fSliceElementLimit() स्थिर { वापस m_fSliceElementLimit; }
    bool fSplit() स्थिर { वापस m_fSplit; }
    bool fSubst() स्थिर { वापस m_fSubst; }
    bool fSubstConst() स्थिर { वापस m_fSubstConst; }
    bool fTable() स्थिर { वापस m_fTable; }
    bool fTaskifyAll() स्थिर { वापस m_fTaskifyAll; }

    std::string traceClassBase() स्थिर VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() स्थिर VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() स्थिर VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() स्थिर VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() स्थिर VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() स्थिर VL_MT_SAFE;

    bool hierarchical() स्थिर { वापस m_hierarchical; }
    पूर्णांक hierChild() स्थिर VL_MT_SAFE { वापस m_hierChild; }
    पूर्णांक hierThreads() स्थिर VL_MT_SAFE { वापस m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_शीर्ष() स्थिर VL_MT_SAFE { वापस !m_hierChild && !m_hierBlocks.empty(); }
    स्थिर V3HierBlockOptSet& hierBlocks() स्थिर { वापस m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_शीर्ष_तथ्य_Dir() स्थिर VL_MT_SAFE {
        वापस hier_शीर्ष() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    अटल string version() VL_PURE;
    अटल string argString(पूर्णांक argc, अक्षर** argv);  ///< Return list of arguments as simple string
    string allArgsString() स्थिर VL_MT_SAFE;  ///< Return all passed arguments as simple string
    स्थिर std::list<std::pair<std::list<std::string>, bool>>& allArgs() स्थिर;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_शीर्ष) स्थिर;
    व्योम parseOpts(File_पंक्ति* fl, पूर्णांक argc, अक्षर** argv) VL_MT_DISABLED;
    व्योम parseOptsList(File_पंक्ति* fl, स्थिर string& optdir, पूर्णांक argc, अक्षर** argv) VL_MT_DISABLED;
    व्योम parseOptsFile(File_पंक्ति* fl, स्थिर string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    अटल string getenvBuiltins(स्थिर string& var);
    अटल string getenvMAKE();
    अटल string getenvMAKEFLAGS();
    अटल string getenvPERL();
    अटल string getenvPYTHON3();
    अटल string getenvSYSTEMC();
    अटल string getenvSYSTEMC_ARCH();
    अटल string getenvSYSTEMC_INCLUDE();
    अटल string getenvSYSTEMC_LIBDIR();
    अटल string getenv_VERILATOR_मूल();
    अटल string getenvVERILATOR_SOLVER();
    अटल string getStdPackagePath();
    अटल string getStdWaiverPath();
    अटल string getSupported(स्थिर string& var);
    अटल bool systemCSystemWide();
    अटल bool systemCFound();  // SystemC installed, or environment points to it
    अटल bool coroutineSupport();  // Configured C++ compiler supports coroutines
    अटल bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    अटल bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    अटल bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(स्थिर string& filename);
    string filePath(File_पंक्ति* fl, स्थिर string& modname, स्थिर string& lastpath,
                    स्थिर string& errmsg);
    string file_Path_Looked_संदेश(File_पंक्ति* fl, स्थिर string& modname);
    V3LangCode fileLanguage(स्थिर string& filename);
    अटल bool fileStatNormal(स्थिर string& filename);

    // METHODS (other OS)
    अटल व्योम throwSigsegv();
};

//######################################################################

#पूर्ण_अगर  // guard
