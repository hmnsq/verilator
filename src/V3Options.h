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
class File_पङ्क्ति;

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
    explicit VOptionBool(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
    अचर वर्ण* ascii() अचर {
        स्थिर अचर वर्ण* अचर names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        फर्काऊ names[m_e];
    }
    bool isDefault() अचर { फर्काऊ m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() अचर { फर्काऊ m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() अचर { फर्काऊ m_e == OPT_TRUE; }
    bool isSetFalse() अचर { फर्काऊ m_e == OPT_FALSE; }
    शून्य setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool चालक==(अचर VOptionBool& lhs, अचर VOptionBool& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VOptionBool& lhs, VOptionBool::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VOptionBool::en lhs, अचर VOptionBool& rhs) { फर्काऊ lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    अचर string m_filename;  // Filename
    अचर string m_libname;  // Libname
सार्वजनिक:
    VFileLibName(अचर string& filename, अचर string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(अचर VFileLibName& rhs) = स्वभाव;
    string filename() अचर { फर्काऊ m_filename; }
    string libname() अचर { फर्काऊ m_libname; }
    bool चालक==(अचर VFileLibName& rhs) अचर {
        फर्काऊ m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool चालक<(अचर VFileLibName& rhs) अचर {
        यदि (m_filename < rhs.m_filename) फर्काऊ true;
        यदि (m_filename > rhs.m_filename) फर्काऊ false;
        फर्काऊ m_libname < rhs.m_libname;
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
    explicit VTimescale(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(अचर string& मान, bool& badr);
    VTimescale(द्विगुण मान, bool& badr) {
        badr = false;
        लागि (पूर्णाङ्क i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            यदि (multiplier() == मान) रोक;
        }
        यदि (multiplier() != मान) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() अचर { फर्काऊ m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    स्थिर शून्य parseSlashed(File_पङ्क्ति* fl, अचर वर्ण* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    अचर वर्ण* ascii() अचर {
        स्थिर अचर वर्ण* अचर names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        फर्काऊ names[m_e];
    }
    पूर्णाङ्क powerOfTen() अचर { फर्काऊ 2 - static_cast<पूर्णाङ्क>(m_e); }
    द्विगुण multiplier() अचर {
        स्थिर अचर द्विगुण मानहरू[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        फर्काऊ मानहरू[m_e];
    }
};
constexpr bool चालक==(अचर VTimescale& lhs, अचर VTimescale& rhs) {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VTimescale& lhs, VTimescale::en rhs) { फर्काऊ lhs.m_e == rhs; }
constexpr bool चालक==(VTimescale::en lhs, अचर VTimescale& rhs) { फर्काऊ lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool चालक<(अचर VTimescale& lhs, अचर VTimescale& rhs) {
    फर्काऊ lhs.m_e > rhs.m_e;
}
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VTimescale& rhs) {
    फर्काऊ os << rhs.ascii();
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
    explicit VFsmExpandType(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() अचर { फर्काऊ m_e; }
    constexpr अचर वर्ण* ascii() अचर {
        constexpr अचर वर्ण* अचर names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        फर्काऊ names[m_e];
    }
};
constexpr bool चालक==(अचर VFsmExpandType& lhs, अचर VFsmExpandType& rhs) VL_MT_SAFE {
    फर्काऊ lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(अचर VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    फर्काऊ lhs.m_e == rhs;
}
constexpr bool चालक==(VFsmExpandType::en lhs, अचर VFsmExpandType& rhs) VL_MT_SAFE {
    फर्काऊ lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& चालक<<(std::ostream& os, अचर VFsmExpandType& rhs) VL_MT_SAFE {
    फर्काऊ os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
सार्वजनिक:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<अचर std::string, std::string>;

गुप्त:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

सार्वजनिक:
    explicit V3HierarchicalBlockOption(अचर string& optstring);
    अचर string& origName() अचर { फर्काऊ m_origName; }
    अचर string& mangledName() अचर { फर्काऊ m_mangledName; }
    अचर ParamStrMap params() अचर { फर्काऊ m_parameters; }
};

using V3HierBlockOptSet = std::map<अचर std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
सार्वजनिक:
गुप्त:
    // TYPES
    using DebugLevelMap = std::map<अचर std::string, अचिन्हित>;

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
    std::map<अचर string, string> m_parameters;  // Parameters
    std::map<अचर string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_पङ्क्ति = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_द्विआधारी = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_अभिव्यक्ति = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_पङ्क्ति = false;    // main switch: --coverage-block
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
    bool m_debug_थाक_Check = false;  // main switch: --debug-stack-check
    पूर्णाङ्क m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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

    पूर्णाङ्क         m_buildJobs = -1;    // main switch: --build-jobs, -j
    पूर्णाङ्क         m_coverage_अभिव्यक्ति_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    पूर्णाङ्क         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    पूर्णाङ्क         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    पूर्णाङ्क         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    पूर्णाङ्क         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    पूर्णाङ्क         m_expandLimit = 256;  // main switch: --expand-limit
    पूर्णाङ्क         m_gateStmts = 100;    // main switch: --gate-stmts
    पूर्णाङ्क         m_hierChild = 0;      // main switch: --hierarchical-child
    पूर्णाङ्क         m_hierThreads = 0;      // main switch: --hierarchical-threads
    पूर्णाङ्क         m_ifDepth = 0;      // main switch: --if-depth
    पूर्णाङ्क         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    पूर्णाङ्क         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    पूर्णाङ्क         m_inlineMult = 2000;   // main switch: --inline-mult
    पूर्णाङ्क         m_instr_गन्ती_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    पूर्णाङ्क         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    पूर्णाङ्क         m_max_अङ्क_Width = 65536;  // main switch: --max-num-width
    पूर्णाङ्क         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    पूर्णाङ्क         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    पूर्णाङ्क         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    पूर्णाङ्क         m_outputSplit = 20000;  // main switch: --output-split
    पूर्णाङ्क         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    पूर्णाङ्क         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    पूर्णाङ्क         m_pinsBv = 65;       // main switch: --pins-bv
    पूर्णाङ्क         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    पूर्णाङ्क         m_publicDepth = 0;   // main switch: --public-depth
    पूर्णाङ्क         m_reloopLimit = 40; // main switch: --reloop-limit
    पूर्णाङ्क         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    पूर्णाङ्क         m_threads = 1;      // main switch: --threads
    पूर्णाङ्क         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    पूर्णाङ्क         m_traceDepth = 0;   // main switch: --trace-depth
    पूर्णाङ्क         m_traceMaxArray = 32;  // main switch: --trace-max-array
    पूर्णाङ्क         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    पूर्णाङ्क         m_unroll_गन्ती = 64;  // main switch: --unroll-count
    पूर्णाङ्क         m_unrollLimit = 16384;  // main switch: --unroll-limit
    पूर्णाङ्क         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    पूर्णाङ्क         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    पूर्णाङ्क         m_verilateJobs = -1;  // main switch: --verilate-jobs

    पूर्णाङ्क         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    पूर्णाङ्क         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    पूर्णाङ्क         m_compLimitParens = 240;  // compiler selection; number of nested parens

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
    string      m_protect_साँचो;   // main switch: --protect-key
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
    bool m_f_Case_रूख;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_रूख;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_थप_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    पूर्णाङ्क  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    शून्य addArg(वर्ण** argv, माप_प्रकार गन्ती, bool isForRerun);
    शून्य addArg(अचर std::string& arg, bool isForRerun);
    शून्य add_पङ्क्ति_Arg(अचर string& arg);
    शून्य addDefine(अचर string& defline, bool allowPlus) VL_MT_DISABLED;
    शून्य addFuture(अचर string& flag);
    शून्य addFuture0(अचर string& flag);
    शून्य addFuture1(अचर string& flag);
    शून्य addIncDirUser(अचर string& incdir);  // User requested
    शून्य addIncDirFallback(अचर string& incdir);  // Low priority if not found otherwise
    शून्य addParameter(अचर string& paramline, bool allowPlus);
    शून्य addLangExt(अचर string& langext, अचर V3LangCode& lc);
    शून्य addLibExtV(अचर string& libext);
    शून्य optimize(पूर्णाङ्क level);
    शून्य showVersion(bool verbose);
    शून्य coverage(bool flag) {
        m_coverage_पङ्क्ति = m_coverageBranch = m_coverageToggle = m_coverage_अभिव्यक्ति = m_coverageFsm
            = m_coverageUser = flag;
    }
    स्थिर bool suffixed(अचर string& sw, अचर वर्ण* arg);
    स्थिर string parseFileArg(अचर string& optdir, अचर string& relfilename);
    string filePathCheckOneDir(अचर string& modname, अचर string& स_नाम);
    स्थिर पूर्णाङ्क stripOptionsForChildRun(अचर string& opt, bool for_शीर्ष);
    शून्य validateIdentifier(File_पङ्क्ति* fl, अचर string& arg, अचर string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

सार्वजनिक:
    V3Options();
    ~V3Options();
    शून्य setDebugMode(पूर्णाङ्क level);
    अचिन्हित debugLevel(अचर string& tag) अचर VL_MT_SAFE;
    अचिन्हित debugSrcLevel(अचर string& srcfile_path) अचर VL_MT_SAFE;
    अचिन्हित dumpLevel(अचर string& tag) अचर VL_MT_SAFE;
    अचिन्हित dumpSrcLevel(अचर string& srcfile_path) अचर VL_MT_SAFE;

    // METHODS
    शून्य addCppFile(अचर string& filename);
    शून्य addCFlags(अचर string& filename);
    शून्य addCompilerIncludes(अचर string& filename);
    शून्य addLdLibFile(अचर string& filename);
    शून्य addLdLibs(अचर string& filename);
    शून्य addMakeFlags(अचर string& filename);
    शून्य addLibMapFile(अचर string& filename);
    शून्य addLibraryFile(अचर string& filename, अचर string& libname);
    शून्य addVFile(अचर string& filename, अचर string& libname);
    शून्य addVltFile(अचर string& filename, अचर string& libname);
    शून्य addForceInc(अचर string& filename);
    bool available() अचर VL_MT_SAFE { फर्काऊ m_available; }
    शून्य ccSet();
    शून्य decorations(File_पङ्क्ति* fl, अचर string& filename);
    शून्य notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() अचर { फर्काऊ m_preprocOnly; }
    bool makePhony() अचर { फर्काऊ m_makePhony; }
    bool preproc_No_पङ्क्ति() अचर { फर्काऊ m_preproc_No_पङ्क्ति; }
    bool preprocResolve() अचर { फर्काऊ m_preprocResolve; }
    पूर्णाङ्क preprocTokenLimit() अचर { फर्काऊ m_preprocTokenLimit; }
    bool underlineZero() अचर { फर्काऊ m_underlineZero; }
    bool systemC() अचर VL_MT_SAFE { फर्काऊ m_systemC; }
    bool savable() अचर VL_MT_SAFE { फर्काऊ m_savable; }
    VOptionBool schedZeroDelay() अचर { फर्काऊ m_schedZeroDelay; }
    bool stats() अचर { फर्काऊ m_stats; }
    bool statsVars() अचर { फर्काऊ m_statsVars; }
    bool stdPackage() अचर { फर्काऊ m_stdPackage; }
    bool stdWaiver() अचर { फर्काऊ m_stdWaiver; }
    bool assertOn() अचर { फर्काऊ m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() अचर { फर्काऊ m_assertCase; }
    bool autoflush() अचर { फर्काऊ m_autoflush; }
    bool bboxSys() अचर { फर्काऊ m_bboxSys; }
    bool bboxUnsup() अचर { फर्काऊ m_bboxUnsup; }
    bool build() अचर { फर्काऊ m_build; }
    string buildDepBin() अचर { फर्काऊ m_buildDepBin; }
    शून्य buildDepBin(अचर string& flag) { m_buildDepBin = flag; }
    bool context() अचर VL_MT_SAFE { फर्काऊ m_context; }
    bool coverage() अचर VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        फर्काऊ m_coverage_पङ्क्ति || m_coverageBranch || m_coverageToggle || m_coverage_अभिव्यक्ति
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() अचर VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        फर्काऊ m_coverage_पङ्क्ति || m_coverageBranch || m_coverageToggle || m_coverage_अभिव्यक्ति
               || m_coverageUser;
    }
    bool coverageBranch() अचर { फर्काऊ m_coverageBranch; }
    bool coverage_अभिव्यक्ति() अचर { फर्काऊ m_coverage_अभिव्यक्ति; }
    bool coverageFsm() अचर { फर्काऊ m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() अचर { फर्काऊ m_coverageFsmExpand; }
    पूर्णाङ्क coverageFsmMaxArcs() अचर { फर्काऊ m_coverageFsmMaxArcs; }
    bool coverage_पङ्क्ति() अचर { फर्काऊ m_coverage_पङ्क्ति; }
    bool coverageMergeInstances() अचर { फर्काऊ m_coverageMergeInstances; }
    bool coveragePerInstance() अचर { फर्काऊ m_coveragePerInstance; }
    bool coverageToggle() अचर { फर्काऊ m_coverageToggle; }
    bool coverageUnderscore() अचर { फर्काऊ m_coverageUnderscore; }
    bool coverageUser() अचर { फर्काऊ m_coverageUser; }
    bool debugCheck() अचर VL_MT_SAFE { फर्काऊ m_debugCheck; }
    bool debugCollision() अचर { फर्काऊ m_debugCollision; }
    bool debugEmitV() अचर VL_MT_SAFE { फर्काऊ m_debugEmitV; }
    bool debugExitElab() अचर { फर्काऊ m_debugExitElab; }
    bool debugExitParse() अचर { फर्काऊ m_debugExitParse; }
    bool debugLeak() अचर { फर्काऊ m_debugLeak; }
    bool debugNondeterminism() अचर { फर्काऊ m_debugNondeterminism; }
    bool debugPartition() अचर { फर्काऊ m_debugPartition; }
    bool debugPreprocPassthru() अचर VL_MT_SAFE { फर्काऊ m_debugPreprocPassthru; }
    bool debugProtect() अचर VL_MT_SAFE { फर्काऊ m_debugProtect; }
    पूर्णाङ्क debugRuntimeTimeout() अचर { फर्काऊ m_debugRuntimeTimeout; }
    bool debugSelfTest() अचर { फर्काऊ m_debugSelfTest; }
    bool debug_थाक_Check() अचर { फर्काऊ m_debug_थाक_Check; }
    bool debugWidth() अचर VL_PURE { फर्काऊ m_debugWidth; }
    bool decoration() अचर VL_MT_SAFE { फर्काऊ m_decoration; }
    bool decorationNodes() अचर VL_MT_SAFE { फर्काऊ m_decorationNodes; }
    bool diagnosticsSarif() अचर VL_MT_SAFE { फर्काऊ m_diagnosticsSarif; }
    bool dpiHdrOnly() अचर { फर्काऊ m_dpiHdrOnly; }
    bool dumpAstPatterns() अचर {
        फर्काऊ m_dumpLevel.गन्ती("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() अचर { फर्काऊ m_dumpLevel.गन्ती("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() अचर {
        फर्काऊ m_dumpLevel.गन्ती("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_रूख_Dot() अचर {
        फर्काऊ m_dumpLevel.गन्ती("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() अचर { फर्काऊ m_emitAccessors; }
    bool exe() अचर { फर्काऊ m_exe; }
    bool flatten() अचर { फर्काऊ m_flatten; }
    bool fourstate() अचर { फर्काऊ m_fourstate; }
    bool gmake() अचर { फर्काऊ m_gmake; }
    bool makeJson() अचर { फर्काऊ m_makeJson; }
    bool threadsDpiPure() अचर { फर्काऊ m_threadsDpiPure; }
    bool threadsDpiUnpure() अचर { फर्काऊ m_threadsDpiUnpure; }
    bool threadsCoarsen() अचर { फर्काऊ m_threadsCoarsen; }
    VOptionBool timing() अचर { फर्काऊ m_timing; }
    bool trace() अचर { फर्काऊ m_trace; }
    bool traceCoverage() अचर { फर्काऊ m_traceCoverage; }
    bool traceEnabledFst() अचर { फर्काऊ m_traceEnabledFst; }
    bool traceEnabledSaif() अचर { फर्काऊ m_traceEnabledSaif; }
    bool traceEnabledVcd() अचर { फर्काऊ m_traceEnabledVcd; }
    bool traceParams() अचर { फर्काऊ m_traceParams; }
    bool traceStructs() अचर { फर्काऊ m_traceStructs; }
    bool traceUnderscore() अचर { फर्काऊ m_traceUnderscore; }
    bool main() अचर { फर्काऊ m_main; }
    bool outFormatOk() अचर { फर्काऊ m_outFormatOk; }
    bool jsonOnly() अचर { फर्काऊ m_jsonOnly; }
    bool keepTempFiles() अचर { फर्काऊ (V3Error::debugDefault() != 0); }
    bool pedantic() अचर { फर्काऊ m_pedantic; }
    bool pinsInoutEnables() अचर { फर्काऊ m_pinsInoutEnables; }
    bool pinsScUint() अचर { फर्काऊ m_pinsScUint; }
    bool pinsScUintBool() अचर { फर्काऊ m_pinsScUintBool; }
    bool pinsScBigUint() अचर VL_MT_SAFE { फर्काऊ m_pinsScBigUint; }
    bool pinsUint8() अचर { फर्काऊ m_pinsUint8; }
    bool preprocComments() अचर { फर्काऊ m_preprocComments; }
    bool preprocDefines() अचर { फर्काऊ m_preprocDefines; }
    bool profC() अचर { फर्काऊ m_profC; }
    bool profCFuncs() अचर { फर्काऊ m_profCFuncs; }
    bool profExec() अचर { फर्काऊ m_profExec; }
    bool profPgo() अचर { फर्काऊ m_profPgo; }
    bool usesProfiler() अचर { फर्काऊ profExec() || profPgo(); }
    bool protectIds() अचर VL_MT_SAFE { फर्काऊ m_protectIds; }
    bool allPublic() अचर { फर्काऊ m_public; }
    bool publicParams() अचर { फर्काऊ m_publicParams; }
    bool publicOff() अचर { फर्काऊ m_publicIgnore; }
    bool publicFlatRW() अचर { फर्काऊ m_publicFlatRW; }
    पूर्णाङ्क publicDepth() अचर { फर्काऊ m_publicDepth; }
    bool anyPublicFlat() अचर { फर्काऊ m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() अचर VL_MT_SAFE { फर्काऊ m_lintOnly; }
    bool ignc() अचर { फर्काऊ m_ignc; }
    bool quietBuild() अचर VL_MT_SAFE { फर्काऊ m_quietBuild; }
    bool quietExit() अचर VL_MT_SAFE { फर्काऊ m_quietExit; }
    bool quietStats() अचर VL_MT_SAFE { फर्काऊ m_quietStats; }
    bool reportUnoptflat() अचर { फर्काऊ m_reportUnoptflat; }
    bool verilate() अचर { फर्काऊ m_verilate; }
    bool vpi() अचर { फर्काऊ m_vpi.isTrue(); }
    bool waiverMultiline() अचर { फर्काऊ m_waiverMultiline; }
    bool xInitialEdge() अचर { फर्काऊ m_xInitialEdge; }
    bool serializeOnly() अचर { फर्काऊ m_jsonOnly; }
    bool शीर्ष_Ifaces_Supported() अचर { फर्काऊ lintOnly() && !hierarchical(); }

    पूर्णाङ्क buildJobs() अचर VL_MT_SAFE { फर्काऊ m_buildJobs; }
    पूर्णाङ्क convergeLimit() अचर { फर्काऊ m_convergeLimit; }
    पूर्णाङ्क coverage_अभिव्यक्ति_Max() अचर { फर्काऊ m_coverage_अभिव्यक्ति_Max; }
    uint32_t coverageMaxBins() अचर { फर्काऊ m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() अचर { फर्काऊ m_coverageMaxRealBins; }
    पूर्णाङ्क coverageMaxWidth() अचर { फर्काऊ m_coverageMaxWidth; }
    पूर्णाङ्क debugAllocRandom() अचर { फर्काऊ m_debugAllocRandom; }
    bool dump_रूख_Addrids() अचर VL_MT_SAFE;
    पूर्णाङ्क expandLimit() अचर { फर्काऊ m_expandLimit; }
    पूर्णाङ्क gateStmts() अचर { फर्काऊ m_gateStmts; }
    पूर्णाङ्क ifDepth() अचर { फर्काऊ m_ifDepth; }
    पूर्णाङ्क inlineCFuncs() अचर { फर्काऊ m_inlineCFuncs; }
    पूर्णाङ्क inlineCFuncsProduct() अचर { फर्काऊ m_inlineCFuncsProduct; }
    पूर्णाङ्क inlineMult() अचर { फर्काऊ m_inlineMult; }
    पूर्णाङ्क instr_गन्ती_Dpi() अचर { फर्काऊ m_instr_गन्ती_Dpi; }
    पूर्णाङ्क localizeMaxSize() अचर { फर्काऊ m_localizeMaxSize; }
    bool jsonEditNums() अचर { फर्काऊ m_jsonEditNums; }
    bool jsonIds() अचर { फर्काऊ m_jsonIds; }
    VOptionBool makeDepend() अचर { फर्काऊ m_makeDepend; }
    पूर्णाङ्क max_अङ्क_Width() अचर { फर्काऊ m_max_अङ्क_Width; }
    पूर्णाङ्क funcRecursionDepth() अचर { फर्काऊ m_funcRecursion; }
    पूर्णाङ्क moduleRecursionDepth() अचर { फर्काऊ m_moduleRecursion; }
    पूर्णाङ्क outputSplit() अचर { फर्काऊ m_outputSplit; }
    पूर्णाङ्क outputSplitCFuncs() अचर { फर्काऊ m_outputSplitCFuncs; }
    पूर्णाङ्क outputSplitCTrace() अचर { फर्काऊ m_outputSplitCTrace; }
    पूर्णाङ्क outputGroups() अचर { फर्काऊ m_outputGroups; }
    int64_t outputGroupsSize() अचर { फर्काऊ m_outputGroupsSize; }
    पूर्णाङ्क pinsBv() अचर VL_MT_SAFE { फर्काऊ m_pinsBv; }
    पूर्णाङ्क reloopLimit() अचर { फर्काऊ m_reloopLimit; }
    पूर्णाङ्क replicationLimit() अचर { फर्काऊ m_replicationLimit; }
    VOptionBool skipIdentical() अचर { फर्काऊ m_skipIdentical; }
    bool stopFail() अचर { फर्काऊ m_stopFail; }
    पूर्णाङ्क threads() अचर VL_MT_SAFE { फर्काऊ m_threads; }
    पूर्णाङ्क threadsMaxMTasks() अचर { फर्काऊ m_threadsMaxMTasks; }
    bool mtasks() अचर VL_MT_SAFE { फर्काऊ (m_threads > 1); }
    VTimescale timeDefaultPrec() अचर { फर्काऊ m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() अचर { फर्काऊ m_timeDefaultUnit; }
    VTimescale timeOverridePrec() अचर { फर्काऊ m_timeOverridePrec; }
    VTimescale timeOverrideUnit() अचर { फर्काऊ m_timeOverrideUnit; }
    VTimescale timeComputePrec(अचर VTimescale& flag) अचर;
    VTimescale timeComputeUnit(अचर VTimescale& flag) अचर;
    पूर्णाङ्क traceDepth() अचर { फर्काऊ m_traceDepth; }
    पूर्णाङ्क traceMaxArray() अचर { फर्काऊ m_traceMaxArray; }
    पूर्णाङ्क traceMaxWidth() अचर { फर्काऊ m_traceMaxWidth; }
    bool useTraceParallel() अचर {
        फर्काऊ trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    पूर्णाङ्क unroll_गन्ती() अचर { फर्काऊ m_unroll_गन्ती; }
    पूर्णाङ्क unrollLimit() अचर { फर्काऊ m_unrollLimit; }
    पूर्णाङ्क unrollStmts() अचर { फर्काऊ m_unrollStmts; }
    पूर्णाङ्क constraintArrayLimit() अचर { फर्काऊ m_constraintArrayLimit; }
    पूर्णाङ्क verilateJobs() अचर { फर्काऊ m_verilateJobs; }

    पूर्णाङ्क compLimitBlocks() अचर { फर्काऊ m_compLimitBlocks; }
    पूर्णाङ्क compLimitMembers() अचर VL_MT_SAFE { फर्काऊ m_compLimitMembers; }
    पूर्णाङ्क compLimitParens() अचर { फर्काऊ m_compLimitParens; }

    string diagnosticsSarifOutput() अचर VL_MT_SAFE {
        फर्काऊ m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() अचर { फर्काऊ m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() अचर { फर्काऊ m_hierParamsFile; }
    string jsonOnlyOutput() अचर { फर्काऊ m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() अचर { फर्काऊ m_jsonOnlyMetaOutput; }
    string l2Name() अचर { फर्काऊ m_l2Name; }
    string libCreate() अचर { फर्काऊ m_libCreate; }
    string libCreateName(bool shared) अचर {
        string libName = "lib" + libCreate();
        यदि (shared) {
            libName += ".so";
        } अन्यथा {
            libName += ".a";
        }
        फर्काऊ libName;
    }
    string main_शीर्ष_Name() अचर { फर्काऊ m_main_शीर्ष_Name; }
    string makeDir() अचर VL_MT_SAFE { फर्काऊ m_makeDir; }
    string modPrefix() अचर VL_MT_SAFE { फर्काऊ m_modPrefix; }
    string pipeFilter() अचर { फर्काऊ m_pipeFilter; }
    string prefix() अचर VL_MT_SAFE { फर्काऊ m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_साँचो_Provided() अचर { फर्काऊ !m_protect_साँचो.empty(); }
    string protect_साँचो_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string शीर्ष_Module() अचर { फर्काऊ m_शीर्ष_Module; }  // As AstNode::encodeName()
    bool no_Trace_शीर्ष() अचर { फर्काऊ m_no_Trace_शीर्ष; }
    string unusedRegexp() अचर { फर्काऊ m_unusedRegexp; }
    string waiverOutput() अचर { फर्काऊ m_waiverOutput; }
    string work() अचर { फर्काऊ m_work; }
    bool isWaiverOutput() अचर { फर्काऊ !m_waiverOutput.empty(); }
    string xAssign() अचर { फर्काऊ m_xAssign; }
    string xInitial() अचर { फर्काऊ m_xInitial; }

    अचर VStringSet& cppFiles() अचर { फर्काऊ m_cppFiles; }
    अचर VStringList& cFlags() अचर { फर्काऊ m_cFlags; }
    अचर VStringSet& compilerIncludes() अचर { फर्काऊ m_compilerIncludes; }
    अचर VStringList& ldLibFiles() अचर { फर्काऊ m_ldLibFiles; }
    अचर VStringList& ldLibs() अचर { फर्काऊ m_ldLibs; }
    अचर VStringList& makeFlags() अचर { फर्काऊ m_makeFlags; }
    अचर VFileLibSet& libraryFiles() अचर { फर्काऊ m_libraryFiles; }
    अचर VStringSet& libmapFiles() अचर { फर्काऊ m_libmapFiles; }
    अचर VFileLibList& vFiles() अचर { फर्काऊ m_vFiles; }
    अचर VFileLibSet& vltFiles() अचर { फर्काऊ m_vltFiles; }
    अचर VStringList& forceIncs() अचर { फर्काऊ m_forceIncs; }

    bool hasParameter(अचर string& name);
    string parameter(अचर string& name);
    शून्य checkParameters();

    bool isFuture(अचर string& flag) अचर;
    bool isFuture0(अचर string& flag) अचर;
    bool isFuture1(अचर string& flag) अचर;
    bool isLibraryFile(अचर string& filename, अचर string& libname) अचर;

    // ACCESSORS (optimization options)
    bool fAcycSimp() अचर { फर्काऊ m_fAcycSimp; }
    bool fAssemble() अचर { फर्काऊ m_fAssemble; }
    bool fBitScanLoops() अचर { फर्काऊ m_fBitScanLoops; }
    bool fBitblast() अचर { फर्काऊ m_fBitblast; }
    bool fCaseDecoder() अचर { फर्काऊ m_fCaseDecoder; }
    bool fCaseTable() अचर { फर्काऊ m_fCaseTable; }
    bool f_Case_रूख() अचर { फर्काऊ m_f_Case_रूख; }
    bool fCombine() अचर { फर्काऊ m_fCombine; }
    bool fConst() अचर { फर्काऊ m_fConst; }
    bool fConstBeforeDfg() अचर { फर्काऊ m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_रूख() अचर { फर्काऊ m_f_Const_Bit_Op_रूख; }
    bool fConstEager() अचर { फर्काऊ m_fConstEager; }
    bool fDedupe() अचर { फर्काऊ m_fDedupe; }
    bool fDfg() अचर { फर्काऊ m_fDfg; }
    bool fDfgPeephole() अचर { फर्काऊ m_fDfgPeephole; }
    bool f_Dfg_थप_Down_Sels() अचर { फर्काऊ m_f_Dfg_थप_Down_Sels; }
    bool fDfgSynthesizeAll() अचर { फर्काऊ m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(अचर std::string& name) अचर {
        फर्काऊ !m_fDfgPeepholeDisabled.गन्ती(name);
    }
    bool fDeadAssigns() अचर { फर्काऊ m_fDeadAssigns; }
    bool fDeadCells() अचर { फर्काऊ m_fDeadCells; }
    bool fDeadMethods() अचर { फर्काऊ m_fDeadMethods; }
    bool fDecompose() अचर { फर्काऊ m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() अचर { फर्काऊ m_fDecomposePacked; }
    bool fDecomposeUnpacked() अचर { फर्काऊ m_fDecomposeUnpacked; }
    bool fExpand() अचर { फर्काऊ m_fExpand; }
    bool fFuncBalanceCat() अचर { फर्काऊ m_fFuncBalanceCat; }
    bool fFuncSplitCat() अचर { फर्काऊ m_fFuncSplitCat; }
    bool fFunc() अचर { फर्काऊ fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() अचर { फर्काऊ m_fGate; }
    VOptionBool fIcoChangeDetect() अचर { फर्काऊ m_fIcoChangeDetect; }
    bool fInline() अचर { फर्काऊ m_fInline; }
    bool fInlineCFuncs() अचर { फर्काऊ m_fInlineCFuncs; }
    bool fInlineFuncs() अचर { फर्काऊ m_fInlineFuncs; }
    bool fInlineFuncsEager() अचर { फर्काऊ m_fInlineFuncsEager; }
    bool fLife() अचर { फर्काऊ m_fLife; }
    bool fLifePost() अचर { फर्काऊ m_fLifePost; }
    bool fLocalize() अचर { फर्काऊ m_fLocalize; }
    bool fMergeCond() अचर { फर्काऊ m_fMergeCond; }
    bool fMergeCondMotion() अचर { फर्काऊ m_fMergeCondMotion; }
    bool fReloop() अचर { फर्काऊ m_fReloop; }
    bool fReorder() अचर { फर्काऊ m_fReorder; }
    bool fSlice() अचर { फर्काऊ m_fSlice; }
    पूर्णाङ्क fSliceElementLimit() अचर { फर्काऊ m_fSliceElementLimit; }
    bool fSplit() अचर { फर्काऊ m_fSplit; }
    bool fSubst() अचर { फर्काऊ m_fSubst; }
    bool fSubstConst() अचर { फर्काऊ m_fSubstConst; }
    bool fTable() अचर { फर्काऊ m_fTable; }
    bool fTaskifyAll() अचर { फर्काऊ m_fTaskifyAll; }

    std::string traceClassBase() अचर VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() अचर VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() अचर VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() अचर VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() अचर VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() अचर VL_MT_SAFE;

    bool hierarchical() अचर { फर्काऊ m_hierarchical; }
    पूर्णाङ्क hierChild() अचर VL_MT_SAFE { फर्काऊ m_hierChild; }
    पूर्णाङ्क hierThreads() अचर VL_MT_SAFE { फर्काऊ m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_शीर्ष() अचर VL_MT_SAFE { फर्काऊ !m_hierChild && !m_hierBlocks.empty(); }
    अचर V3HierBlockOptSet& hierBlocks() अचर { फर्काऊ m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_शीर्ष_Data_Dir() अचर VL_MT_SAFE {
        फर्काऊ hier_शीर्ष() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    स्थिर string version() VL_PURE;
    स्थिर string argString(पूर्णाङ्क argc, वर्ण** argv);  ///< Return list of arguments as simple string
    string allArgsString() अचर VL_MT_SAFE;  ///< Return all passed arguments as simple string
    अचर std::list<std::pair<std::list<std::string>, bool>>& allArgs() अचर;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_शीर्ष) अचर;
    शून्य parseOpts(File_पङ्क्ति* fl, पूर्णाङ्क argc, वर्ण** argv) VL_MT_DISABLED;
    शून्य parseOptsList(File_पङ्क्ति* fl, अचर string& optdir, पूर्णाङ्क argc, वर्ण** argv) VL_MT_DISABLED;
    शून्य parseOptsFile(File_पङ्क्ति* fl, अचर string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    स्थिर string getenvBuiltins(अचर string& var);
    स्थिर string getenvMAKE();
    स्थिर string getenvMAKEFLAGS();
    स्थिर string getenvPERL();
    स्थिर string getenvPYTHON3();
    स्थिर string getenvSYSTEMC();
    स्थिर string getenvSYSTEMC_ARCH();
    स्थिर string getenvSYSTEMC_INCLUDE();
    स्थिर string getenvSYSTEMC_LIBDIR();
    स्थिर string getenv_VERILATOR_जरा();
    स्थिर string getenvVERILATOR_SOLVER();
    स्थिर string getStdPackagePath();
    स्थिर string getStdWaiverPath();
    स्थिर string getSupported(अचर string& var);
    स्थिर bool systemCSystemWide();
    स्थिर bool systemCFound();  // SystemC installed, or environment points to it
    स्थिर bool coroutineSupport();  // Configured C++ compiler supports coroutines
    स्थिर bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    स्थिर bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    स्थिर bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(अचर string& filename);
    string filePath(File_पङ्क्ति* fl, अचर string& modname, अचर string& lastpath,
                    अचर string& errmsg);
    string file_Path_Looked_सन्देश(File_पङ्क्ति* fl, अचर string& modname);
    V3LangCode fileLanguage(अचर string& filename);
    स्थिर bool fileStatNormal(अचर string& filename);

    // METHODS (other OS)
    स्थिर शून्य throwSigsegv();
};

//######################################################################

#endif  // guard
