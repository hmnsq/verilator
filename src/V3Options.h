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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3LangCode.h"

#அந்தர்பாவ <list>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <string>
#அந்தர்பாவ <vector>

class V3OptionsImp;
class File_வரி;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
பொது:
    எண்ணிக்கை en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    எண்ணிக்கை en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(முழு _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() மாறிலி { திரும்பு m_e; }
    மாறிலி எழுத்து* ascii() மாறிலி {
        நிலையான மாறிலி எழுத்து* மாறிலி names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        திரும்பு names[m_e];
    }
    bool isDefault() மாறிலி { திரும்பு m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() மாறிலி { திரும்பு m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() மாறிலி { திரும்பு m_e == OPT_TRUE; }
    bool isSetFalse() மாறிலி { திரும்பு m_e == OPT_FALSE; }
    வெறுமை setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(மாறிலி VOptionBool& lhs, மாறிலி VOptionBool& rhs) {
    திரும்பு lhs.m_e == rhs.m_e;
}
constexpr bool operator==(மாறிலி VOptionBool& lhs, VOptionBool::en rhs) { திரும்பு lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, மாறிலி VOptionBool& rhs) { திரும்பு lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    மாறிலி string m_filename;  // Filename
    மாறிலி string m_libname;  // Libname
பொது:
    VFileLibName(மாறிலி string& filename, மாறிலி string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(மாறிலி VFileLibName& rhs) = இயல்பு;
    string filename() மாறிலி { திரும்பு m_filename; }
    string libname() மாறிலி { திரும்பு m_libname; }
    bool operator==(மாறிலி VFileLibName& rhs) மாறிலி {
        திரும்பு m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(மாறிலி VFileLibName& rhs) மாறிலி {
        எனில் (m_filename < rhs.m_filename) திரும்பு true;
        எனில் (m_filename > rhs.m_filename) திரும்பு false;
        திரும்பு m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
பொது:
    எண்ணிக்கை en : uint8_t {
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
    எண்ணிக்கை : uint8_t { TS_DEFAULT = TS_1PS };
    எண்ணிக்கை en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(முழு _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(மாறிலி string& மதிப்பு, bool& badr);
    VTimescale(இரட்டை மதிப்பு, bool& badr) {
        badr = false;
        க்கு (முழு i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            எனில் (multiplier() == மதிப்பு) நிறுத்து;
        }
        எனில் (multiplier() != மதிப்பு) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() மாறிலி { திரும்பு m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    நிலையான வெறுமை parseSlashed(File_வரி* fl, மாறிலி எழுத்து* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    மாறிலி எழுத்து* ascii() மாறிலி {
        நிலையான மாறிலி எழுத்து* மாறிலி names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        திரும்பு names[m_e];
    }
    முழு powerOfTen() மாறிலி { திரும்பு 2 - static_cast<முழு>(m_e); }
    இரட்டை multiplier() மாறிலி {
        நிலையான மாறிலி இரட்டை மதிப்புகள்[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        திரும்பு மதிப்புகள்[m_e];
    }
};
constexpr bool operator==(மாறிலி VTimescale& lhs, மாறிலி VTimescale& rhs) {
    திரும்பு lhs.m_e == rhs.m_e;
}
constexpr bool operator==(மாறிலி VTimescale& lhs, VTimescale::en rhs) { திரும்பு lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, மாறிலி VTimescale& rhs) { திரும்பு lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(மாறிலி VTimescale& lhs, மாறிலி VTimescale& rhs) {
    திரும்பு lhs.m_e > rhs.m_e;
}
உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி VTimescale& rhs) {
    திரும்பு os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
பொது:
    எண்ணிக்கை en : uint8_t {
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
    explicit VFsmExpandType(முழு _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() மாறிலி { திரும்பு m_e; }
    constexpr மாறிலி எழுத்து* ascii() மாறிலி {
        constexpr மாறிலி எழுத்து* மாறிலி names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        திரும்பு names[m_e];
    }
};
constexpr bool operator==(மாறிலி VFsmExpandType& lhs, மாறிலி VFsmExpandType& rhs) VL_MT_SAFE {
    திரும்பு lhs.m_e == rhs.m_e;
}
constexpr bool operator==(மாறிலி VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    திரும்பு lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, மாறிலி VFsmExpandType& rhs) VL_MT_SAFE {
    திரும்பு lhs == rhs.m_e;
}
உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி VFsmExpandType& rhs) VL_MT_SAFE {
    திரும்பு os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
பொது:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<மாறிலி std::string, std::string>;

தனிப்பட்ட:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

பொது:
    explicit V3HierarchicalBlockOption(மாறிலி string& optstring);
    மாறிலி string& origName() மாறிலி { திரும்பு m_origName; }
    மாறிலி string& mangledName() மாறிலி { திரும்பு m_mangledName; }
    மாறிலி ParamStrMap params() மாறிலி { திரும்பு m_parameters; }
};

using V3HierBlockOptSet = std::map<மாறிலி std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
பொது:
தனிப்பட்ட:
    // TYPES
    using DebugLevelMap = std::map<மாறிலி std::string, unsigned>;

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
    std::map<மாறிலி string, string> m_parameters;  // Parameters
    std::map<மாறிலி string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_வரி = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_இருமம் = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_கோவை = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_வரி = false;    // main switch: --coverage-block
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
    bool m_debug_அடுக்கு_Check = false;  // main switch: --debug-stack-check
    முழு m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_no_Trace_உச்சி = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    முழு         m_buildJobs = -1;    // main switch: --build-jobs, -j
    முழு         m_coverage_கோவை_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    முழு         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    முழு         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    முழு         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    முழு         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    முழு         m_expandLimit = 256;  // main switch: --expand-limit
    முழு         m_gateStmts = 100;    // main switch: --gate-stmts
    முழு         m_hierChild = 0;      // main switch: --hierarchical-child
    முழு         m_hierThreads = 0;      // main switch: --hierarchical-threads
    முழு         m_ifDepth = 0;      // main switch: --if-depth
    முழு         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    முழு         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    முழு         m_inlineMult = 2000;   // main switch: --inline-mult
    முழு         m_instr_மொத்தம்_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    முழு         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    முழு         m_max_இலக்கம்_Width = 65536;  // main switch: --max-num-width
    முழு         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    முழு         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    முழு         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    முழு         m_outputSplit = 20000;  // main switch: --output-split
    முழு         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    முழு         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    முழு         m_pinsBv = 65;       // main switch: --pins-bv
    முழு         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    முழு         m_publicDepth = 0;   // main switch: --public-depth
    முழு         m_reloopLimit = 40; // main switch: --reloop-limit
    முழு         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    முழு         m_threads = 1;      // main switch: --threads
    முழு         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    முழு         m_traceDepth = 0;   // main switch: --trace-depth
    முழு         m_traceMaxArray = 32;  // main switch: --trace-max-array
    முழு         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    முழு         m_unroll_மொத்தம் = 64;  // main switch: --unroll-count
    முழு         m_unrollLimit = 16384;  // main switch: --unroll-limit
    முழு         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    முழு         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    முழு         m_verilateJobs = -1;  // main switch: --verilate-jobs

    முழு         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    முழு         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    முழு         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_உச்சி_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_சாவி;   // main switch: --protect-key
    string      m_உச்சி_Module;    // main switch: --top-module
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
    bool m_f_Case_மரம்;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_மரம்;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_தள்ளு_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    முழு  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    வெறுமை addArg(எழுத்து** argv, size_t மொத்தம், bool isForRerun);
    வெறுமை addArg(மாறிலி std::string& arg, bool isForRerun);
    வெறுமை add_வரி_Arg(மாறிலி string& arg);
    வெறுமை addDefine(மாறிலி string& defline, bool allowPlus) VL_MT_DISABLED;
    வெறுமை addFuture(மாறிலி string& flag);
    வெறுமை addFuture0(மாறிலி string& flag);
    வெறுமை addFuture1(மாறிலி string& flag);
    வெறுமை addIncDirUser(மாறிலி string& incdir);  // User requested
    வெறுமை addIncDirFallback(மாறிலி string& incdir);  // Low priority if not found otherwise
    வெறுமை addParameter(மாறிலி string& paramline, bool allowPlus);
    வெறுமை addLangExt(மாறிலி string& langext, மாறிலி V3LangCode& lc);
    வெறுமை addLibExtV(மாறிலி string& libext);
    வெறுமை optimize(முழு level);
    வெறுமை showVersion(bool verbose);
    வெறுமை coverage(bool flag) {
        m_coverage_வரி = m_coverageBranch = m_coverageToggle = m_coverage_கோவை = m_coverageFsm
            = m_coverageUser = flag;
    }
    நிலையான bool suffixed(மாறிலி string& sw, மாறிலி எழுத்து* arg);
    நிலையான string parseFileArg(மாறிலி string& optdir, மாறிலி string& relfilename);
    string filePathCheckOneDir(மாறிலி string& modname, மாறிலி string& dirname);
    நிலையான முழு stripOptionsForChildRun(மாறிலி string& opt, bool for_உச்சி);
    வெறுமை validateIdentifier(File_வரி* fl, மாறிலி string& arg, மாறிலி string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

பொது:
    V3Options();
    ~V3Options();
    வெறுமை setDebugMode(முழு level);
    unsigned debugLevel(மாறிலி string& tag) மாறிலி VL_MT_SAFE;
    unsigned debugSrcLevel(மாறிலி string& srcfile_path) மாறிலி VL_MT_SAFE;
    unsigned dumpLevel(மாறிலி string& tag) மாறிலி VL_MT_SAFE;
    unsigned dumpSrcLevel(மாறிலி string& srcfile_path) மாறிலி VL_MT_SAFE;

    // METHODS
    வெறுமை addCppFile(மாறிலி string& filename);
    வெறுமை addCFlags(மாறிலி string& filename);
    வெறுமை addCompilerIncludes(மாறிலி string& filename);
    வெறுமை addLdLibFile(மாறிலி string& filename);
    வெறுமை addLdLibs(மாறிலி string& filename);
    வெறுமை addMakeFlags(மாறிலி string& filename);
    வெறுமை addLibMapFile(மாறிலி string& filename);
    வெறுமை addLibraryFile(மாறிலி string& filename, மாறிலி string& libname);
    வெறுமை addVFile(மாறிலி string& filename, மாறிலி string& libname);
    வெறுமை addVltFile(மாறிலி string& filename, மாறிலி string& libname);
    வெறுமை addForceInc(மாறிலி string& filename);
    bool available() மாறிலி VL_MT_SAFE { திரும்பு m_available; }
    வெறுமை ccSet();
    வெறுமை decorations(File_வரி* fl, மாறிலி string& filename);
    வெறுமை notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() மாறிலி { திரும்பு m_preprocOnly; }
    bool makePhony() மாறிலி { திரும்பு m_makePhony; }
    bool preproc_No_வரி() மாறிலி { திரும்பு m_preproc_No_வரி; }
    bool preprocResolve() மாறிலி { திரும்பு m_preprocResolve; }
    முழு preprocTokenLimit() மாறிலி { திரும்பு m_preprocTokenLimit; }
    bool underlineZero() மாறிலி { திரும்பு m_underlineZero; }
    bool systemC() மாறிலி VL_MT_SAFE { திரும்பு m_systemC; }
    bool savable() மாறிலி VL_MT_SAFE { திரும்பு m_savable; }
    VOptionBool schedZeroDelay() மாறிலி { திரும்பு m_schedZeroDelay; }
    bool stats() மாறிலி { திரும்பு m_stats; }
    bool statsVars() மாறிலி { திரும்பு m_statsVars; }
    bool stdPackage() மாறிலி { திரும்பு m_stdPackage; }
    bool stdWaiver() மாறிலி { திரும்பு m_stdWaiver; }
    bool assertOn() மாறிலி { திரும்பு m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() மாறிலி { திரும்பு m_assertCase; }
    bool autoflush() மாறிலி { திரும்பு m_autoflush; }
    bool bboxSys() மாறிலி { திரும்பு m_bboxSys; }
    bool bboxUnsup() மாறிலி { திரும்பு m_bboxUnsup; }
    bool build() மாறிலி { திரும்பு m_build; }
    string buildDepBin() மாறிலி { திரும்பு m_buildDepBin; }
    வெறுமை buildDepBin(மாறிலி string& flag) { m_buildDepBin = flag; }
    bool context() மாறிலி VL_MT_SAFE { திரும்பு m_context; }
    bool coverage() மாறிலி VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        திரும்பு m_coverage_வரி || m_coverageBranch || m_coverageToggle || m_coverage_கோவை
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() மாறிலி VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        திரும்பு m_coverage_வரி || m_coverageBranch || m_coverageToggle || m_coverage_கோவை
               || m_coverageUser;
    }
    bool coverageBranch() மாறிலி { திரும்பு m_coverageBranch; }
    bool coverage_கோவை() மாறிலி { திரும்பு m_coverage_கோவை; }
    bool coverageFsm() மாறிலி { திரும்பு m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() மாறிலி { திரும்பு m_coverageFsmExpand; }
    முழு coverageFsmMaxArcs() மாறிலி { திரும்பு m_coverageFsmMaxArcs; }
    bool coverage_வரி() மாறிலி { திரும்பு m_coverage_வரி; }
    bool coverageMergeInstances() மாறிலி { திரும்பு m_coverageMergeInstances; }
    bool coveragePerInstance() மாறிலி { திரும்பு m_coveragePerInstance; }
    bool coverageToggle() மாறிலி { திரும்பு m_coverageToggle; }
    bool coverageUnderscore() மாறிலி { திரும்பு m_coverageUnderscore; }
    bool coverageUser() மாறிலி { திரும்பு m_coverageUser; }
    bool debugCheck() மாறிலி VL_MT_SAFE { திரும்பு m_debugCheck; }
    bool debugCollision() மாறிலி { திரும்பு m_debugCollision; }
    bool debugEmitV() மாறிலி VL_MT_SAFE { திரும்பு m_debugEmitV; }
    bool debugExitElab() மாறிலி { திரும்பு m_debugExitElab; }
    bool debugExitParse() மாறிலி { திரும்பு m_debugExitParse; }
    bool debugLeak() மாறிலி { திரும்பு m_debugLeak; }
    bool debugNondeterminism() மாறிலி { திரும்பு m_debugNondeterminism; }
    bool debugPartition() மாறிலி { திரும்பு m_debugPartition; }
    bool debugPreprocPassthru() மாறிலி VL_MT_SAFE { திரும்பு m_debugPreprocPassthru; }
    bool debugProtect() மாறிலி VL_MT_SAFE { திரும்பு m_debugProtect; }
    முழு debugRuntimeTimeout() மாறிலி { திரும்பு m_debugRuntimeTimeout; }
    bool debugSelfTest() மாறிலி { திரும்பு m_debugSelfTest; }
    bool debug_அடுக்கு_Check() மாறிலி { திரும்பு m_debug_அடுக்கு_Check; }
    bool debugWidth() மாறிலி VL_PURE { திரும்பு m_debugWidth; }
    bool decoration() மாறிலி VL_MT_SAFE { திரும்பு m_decoration; }
    bool decorationNodes() மாறிலி VL_MT_SAFE { திரும்பு m_decorationNodes; }
    bool diagnosticsSarif() மாறிலி VL_MT_SAFE { திரும்பு m_diagnosticsSarif; }
    bool dpiHdrOnly() மாறிலி { திரும்பு m_dpiHdrOnly; }
    bool dumpAstPatterns() மாறிலி {
        திரும்பு m_dumpLevel.மொத்தம்("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() மாறிலி { திரும்பு m_dumpLevel.மொத்தம்("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() மாறிலி {
        திரும்பு m_dumpLevel.மொத்தம்("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_மரம்_Dot() மாறிலி {
        திரும்பு m_dumpLevel.மொத்தம்("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() மாறிலி { திரும்பு m_emitAccessors; }
    bool exe() மாறிலி { திரும்பு m_exe; }
    bool flatten() மாறிலி { திரும்பு m_flatten; }
    bool fourstate() மாறிலி { திரும்பு m_fourstate; }
    bool gmake() மாறிலி { திரும்பு m_gmake; }
    bool makeJson() மாறிலி { திரும்பு m_makeJson; }
    bool threadsDpiPure() மாறிலி { திரும்பு m_threadsDpiPure; }
    bool threadsDpiUnpure() மாறிலி { திரும்பு m_threadsDpiUnpure; }
    bool threadsCoarsen() மாறிலி { திரும்பு m_threadsCoarsen; }
    VOptionBool timing() மாறிலி { திரும்பு m_timing; }
    bool trace() மாறிலி { திரும்பு m_trace; }
    bool traceCoverage() மாறிலி { திரும்பு m_traceCoverage; }
    bool traceEnabledFst() மாறிலி { திரும்பு m_traceEnabledFst; }
    bool traceEnabledSaif() மாறிலி { திரும்பு m_traceEnabledSaif; }
    bool traceEnabledVcd() மாறிலி { திரும்பு m_traceEnabledVcd; }
    bool traceParams() மாறிலி { திரும்பு m_traceParams; }
    bool traceStructs() மாறிலி { திரும்பு m_traceStructs; }
    bool traceUnderscore() மாறிலி { திரும்பு m_traceUnderscore; }
    bool முதன்மை() மாறிலி { திரும்பு m_main; }
    bool outFormatOk() மாறிலி { திரும்பு m_outFormatOk; }
    bool jsonOnly() மாறிலி { திரும்பு m_jsonOnly; }
    bool keepTempFiles() மாறிலி { திரும்பு (V3Error::debugDefault() != 0); }
    bool pedantic() மாறிலி { திரும்பு m_pedantic; }
    bool pinsInoutEnables() மாறிலி { திரும்பு m_pinsInoutEnables; }
    bool pinsScUint() மாறிலி { திரும்பு m_pinsScUint; }
    bool pinsScUintBool() மாறிலி { திரும்பு m_pinsScUintBool; }
    bool pinsScBigUint() மாறிலி VL_MT_SAFE { திரும்பு m_pinsScBigUint; }
    bool pinsUint8() மாறிலி { திரும்பு m_pinsUint8; }
    bool preprocComments() மாறிலி { திரும்பு m_preprocComments; }
    bool preprocDefines() மாறிலி { திரும்பு m_preprocDefines; }
    bool profC() மாறிலி { திரும்பு m_profC; }
    bool profCFuncs() மாறிலி { திரும்பு m_profCFuncs; }
    bool profExec() மாறிலி { திரும்பு m_profExec; }
    bool profPgo() மாறிலி { திரும்பு m_profPgo; }
    bool usesProfiler() மாறிலி { திரும்பு profExec() || profPgo(); }
    bool protectIds() மாறிலி VL_MT_SAFE { திரும்பு m_protectIds; }
    bool allPublic() மாறிலி { திரும்பு m_public; }
    bool publicParams() மாறிலி { திரும்பு m_publicParams; }
    bool publicOff() மாறிலி { திரும்பு m_publicIgnore; }
    bool publicFlatRW() மாறிலி { திரும்பு m_publicFlatRW; }
    முழு publicDepth() மாறிலி { திரும்பு m_publicDepth; }
    bool anyPublicFlat() மாறிலி { திரும்பு m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() மாறிலி VL_MT_SAFE { திரும்பு m_lintOnly; }
    bool ignc() மாறிலி { திரும்பு m_ignc; }
    bool quietBuild() மாறிலி VL_MT_SAFE { திரும்பு m_quietBuild; }
    bool quietExit() மாறிலி VL_MT_SAFE { திரும்பு m_quietExit; }
    bool quietStats() மாறிலி VL_MT_SAFE { திரும்பு m_quietStats; }
    bool reportUnoptflat() மாறிலி { திரும்பு m_reportUnoptflat; }
    bool verilate() மாறிலி { திரும்பு m_verilate; }
    bool vpi() மாறிலி { திரும்பு m_vpi.isTrue(); }
    bool waiverMultiline() மாறிலி { திரும்பு m_waiverMultiline; }
    bool xInitialEdge() மாறிலி { திரும்பு m_xInitialEdge; }
    bool serializeOnly() மாறிலி { திரும்பு m_jsonOnly; }
    bool உச்சி_Ifaces_Supported() மாறிலி { திரும்பு lintOnly() && !hierarchical(); }

    முழு buildJobs() மாறிலி VL_MT_SAFE { திரும்பு m_buildJobs; }
    முழு convergeLimit() மாறிலி { திரும்பு m_convergeLimit; }
    முழு coverage_கோவை_Max() மாறிலி { திரும்பு m_coverage_கோவை_Max; }
    uint32_t coverageMaxBins() மாறிலி { திரும்பு m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() மாறிலி { திரும்பு m_coverageMaxRealBins; }
    முழு coverageMaxWidth() மாறிலி { திரும்பு m_coverageMaxWidth; }
    முழு debugAllocRandom() மாறிலி { திரும்பு m_debugAllocRandom; }
    bool dump_மரம்_Addrids() மாறிலி VL_MT_SAFE;
    முழு expandLimit() மாறிலி { திரும்பு m_expandLimit; }
    முழு gateStmts() மாறிலி { திரும்பு m_gateStmts; }
    முழு ifDepth() மாறிலி { திரும்பு m_ifDepth; }
    முழு inlineCFuncs() மாறிலி { திரும்பு m_inlineCFuncs; }
    முழு inlineCFuncsProduct() மாறிலி { திரும்பு m_inlineCFuncsProduct; }
    முழு inlineMult() மாறிலி { திரும்பு m_inlineMult; }
    முழு instr_மொத்தம்_Dpi() மாறிலி { திரும்பு m_instr_மொத்தம்_Dpi; }
    முழு localizeMaxSize() மாறிலி { திரும்பு m_localizeMaxSize; }
    bool jsonEditNums() மாறிலி { திரும்பு m_jsonEditNums; }
    bool jsonIds() மாறிலி { திரும்பு m_jsonIds; }
    VOptionBool makeDepend() மாறிலி { திரும்பு m_makeDepend; }
    முழு max_இலக்கம்_Width() மாறிலி { திரும்பு m_max_இலக்கம்_Width; }
    முழு funcRecursionDepth() மாறிலி { திரும்பு m_funcRecursion; }
    முழு moduleRecursionDepth() மாறிலி { திரும்பு m_moduleRecursion; }
    முழு outputSplit() மாறிலி { திரும்பு m_outputSplit; }
    முழு outputSplitCFuncs() மாறிலி { திரும்பு m_outputSplitCFuncs; }
    முழு outputSplitCTrace() மாறிலி { திரும்பு m_outputSplitCTrace; }
    முழு outputGroups() மாறிலி { திரும்பு m_outputGroups; }
    int64_t outputGroupsSize() மாறிலி { திரும்பு m_outputGroupsSize; }
    முழு pinsBv() மாறிலி VL_MT_SAFE { திரும்பு m_pinsBv; }
    முழு reloopLimit() மாறிலி { திரும்பு m_reloopLimit; }
    முழு replicationLimit() மாறிலி { திரும்பு m_replicationLimit; }
    VOptionBool skipIdentical() மாறிலி { திரும்பு m_skipIdentical; }
    bool stopFail() மாறிலி { திரும்பு m_stopFail; }
    முழு threads() மாறிலி VL_MT_SAFE { திரும்பு m_threads; }
    முழு threadsMaxMTasks() மாறிலி { திரும்பு m_threadsMaxMTasks; }
    bool mtasks() மாறிலி VL_MT_SAFE { திரும்பு (m_threads > 1); }
    VTimescale timeDefaultPrec() மாறிலி { திரும்பு m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() மாறிலி { திரும்பு m_timeDefaultUnit; }
    VTimescale timeOverridePrec() மாறிலி { திரும்பு m_timeOverridePrec; }
    VTimescale timeOverrideUnit() மாறிலி { திரும்பு m_timeOverrideUnit; }
    VTimescale timeComputePrec(மாறிலி VTimescale& flag) மாறிலி;
    VTimescale timeComputeUnit(மாறிலி VTimescale& flag) மாறிலி;
    முழு traceDepth() மாறிலி { திரும்பு m_traceDepth; }
    முழு traceMaxArray() மாறிலி { திரும்பு m_traceMaxArray; }
    முழு traceMaxWidth() மாறிலி { திரும்பு m_traceMaxWidth; }
    bool useTraceParallel() மாறிலி {
        திரும்பு trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    முழு unroll_மொத்தம்() மாறிலி { திரும்பு m_unroll_மொத்தம்; }
    முழு unrollLimit() மாறிலி { திரும்பு m_unrollLimit; }
    முழு unrollStmts() மாறிலி { திரும்பு m_unrollStmts; }
    முழு constraintArrayLimit() மாறிலி { திரும்பு m_constraintArrayLimit; }
    முழு verilateJobs() மாறிலி { திரும்பு m_verilateJobs; }

    முழு compLimitBlocks() மாறிலி { திரும்பு m_compLimitBlocks; }
    முழு compLimitMembers() மாறிலி VL_MT_SAFE { திரும்பு m_compLimitMembers; }
    முழு compLimitParens() மாறிலி { திரும்பு m_compLimitParens; }

    string diagnosticsSarifOutput() மாறிலி VL_MT_SAFE {
        திரும்பு m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() மாறிலி { திரும்பு m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() மாறிலி { திரும்பு m_hierParamsFile; }
    string jsonOnlyOutput() மாறிலி { திரும்பு m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() மாறிலி { திரும்பு m_jsonOnlyMetaOutput; }
    string l2Name() மாறிலி { திரும்பு m_l2Name; }
    string libCreate() மாறிலி { திரும்பு m_libCreate; }
    string libCreateName(bool shared) மாறிலி {
        string libName = "lib" + libCreate();
        எனில் (shared) {
            libName += ".so";
        } இல்லையெனில் {
            libName += ".a";
        }
        திரும்பு libName;
    }
    string main_உச்சி_Name() மாறிலி { திரும்பு m_main_உச்சி_Name; }
    string makeDir() மாறிலி VL_MT_SAFE { திரும்பு m_makeDir; }
    string modPrefix() மாறிலி VL_MT_SAFE { திரும்பு m_modPrefix; }
    string pipeFilter() மாறிலி { திரும்பு m_pipeFilter; }
    string prefix() மாறிலி VL_MT_SAFE { திரும்பு m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_சாவி_Provided() மாறிலி { திரும்பு !m_protect_சாவி.empty(); }
    string protect_சாவி_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string உச்சி_Module() மாறிலி { திரும்பு m_உச்சி_Module; }  // As AstNode::encodeName()
    bool no_Trace_உச்சி() மாறிலி { திரும்பு m_no_Trace_உச்சி; }
    string unusedRegexp() மாறிலி { திரும்பு m_unusedRegexp; }
    string waiverOutput() மாறிலி { திரும்பு m_waiverOutput; }
    string work() மாறிலி { திரும்பு m_work; }
    bool isWaiverOutput() மாறிலி { திரும்பு !m_waiverOutput.empty(); }
    string xAssign() மாறிலி { திரும்பு m_xAssign; }
    string xInitial() மாறிலி { திரும்பு m_xInitial; }

    மாறிலி VStringSet& cppFiles() மாறிலி { திரும்பு m_cppFiles; }
    மாறிலி VStringList& cFlags() மாறிலி { திரும்பு m_cFlags; }
    மாறிலி VStringSet& compilerIncludes() மாறிலி { திரும்பு m_compilerIncludes; }
    மாறிலி VStringList& ldLibFiles() மாறிலி { திரும்பு m_ldLibFiles; }
    மாறிலி VStringList& ldLibs() மாறிலி { திரும்பு m_ldLibs; }
    மாறிலி VStringList& makeFlags() மாறிலி { திரும்பு m_makeFlags; }
    மாறிலி VFileLibSet& libraryFiles() மாறிலி { திரும்பு m_libraryFiles; }
    மாறிலி VStringSet& libmapFiles() மாறிலி { திரும்பு m_libmapFiles; }
    மாறிலி VFileLibList& vFiles() மாறிலி { திரும்பு m_vFiles; }
    மாறிலி VFileLibSet& vltFiles() மாறிலி { திரும்பு m_vltFiles; }
    மாறிலி VStringList& forceIncs() மாறிலி { திரும்பு m_forceIncs; }

    bool hasParameter(மாறிலி string& name);
    string parameter(மாறிலி string& name);
    வெறுமை checkParameters();

    bool isFuture(மாறிலி string& flag) மாறிலி;
    bool isFuture0(மாறிலி string& flag) மாறிலி;
    bool isFuture1(மாறிலி string& flag) மாறிலி;
    bool isLibraryFile(மாறிலி string& filename, மாறிலி string& libname) மாறிலி;

    // ACCESSORS (optimization options)
    bool fAcycSimp() மாறிலி { திரும்பு m_fAcycSimp; }
    bool fAssemble() மாறிலி { திரும்பு m_fAssemble; }
    bool fBitScanLoops() மாறிலி { திரும்பு m_fBitScanLoops; }
    bool fBitblast() மாறிலி { திரும்பு m_fBitblast; }
    bool fCaseDecoder() மாறிலி { திரும்பு m_fCaseDecoder; }
    bool fCaseTable() மாறிலி { திரும்பு m_fCaseTable; }
    bool f_Case_மரம்() மாறிலி { திரும்பு m_f_Case_மரம்; }
    bool fCombine() மாறிலி { திரும்பு m_fCombine; }
    bool fConst() மாறிலி { திரும்பு m_fConst; }
    bool fConstBeforeDfg() மாறிலி { திரும்பு m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_மரம்() மாறிலி { திரும்பு m_f_Const_Bit_Op_மரம்; }
    bool fConstEager() மாறிலி { திரும்பு m_fConstEager; }
    bool fDedupe() மாறிலி { திரும்பு m_fDedupe; }
    bool fDfg() மாறிலி { திரும்பு m_fDfg; }
    bool fDfgPeephole() மாறிலி { திரும்பு m_fDfgPeephole; }
    bool f_Dfg_தள்ளு_Down_Sels() மாறிலி { திரும்பு m_f_Dfg_தள்ளு_Down_Sels; }
    bool fDfgSynthesizeAll() மாறிலி { திரும்பு m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(மாறிலி std::string& name) மாறிலி {
        திரும்பு !m_fDfgPeepholeDisabled.மொத்தம்(name);
    }
    bool fDeadAssigns() மாறிலி { திரும்பு m_fDeadAssigns; }
    bool fDeadCells() மாறிலி { திரும்பு m_fDeadCells; }
    bool fDeadMethods() மாறிலி { திரும்பு m_fDeadMethods; }
    bool fDecompose() மாறிலி { திரும்பு m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() மாறிலி { திரும்பு m_fDecomposePacked; }
    bool fDecomposeUnpacked() மாறிலி { திரும்பு m_fDecomposeUnpacked; }
    bool fExpand() மாறிலி { திரும்பு m_fExpand; }
    bool fFuncBalanceCat() மாறிலி { திரும்பு m_fFuncBalanceCat; }
    bool fFuncSplitCat() மாறிலி { திரும்பு m_fFuncSplitCat; }
    bool fFunc() மாறிலி { திரும்பு fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() மாறிலி { திரும்பு m_fGate; }
    VOptionBool fIcoChangeDetect() மாறிலி { திரும்பு m_fIcoChangeDetect; }
    bool fInline() மாறிலி { திரும்பு m_fInline; }
    bool fInlineCFuncs() மாறிலி { திரும்பு m_fInlineCFuncs; }
    bool fInlineFuncs() மாறிலி { திரும்பு m_fInlineFuncs; }
    bool fInlineFuncsEager() மாறிலி { திரும்பு m_fInlineFuncsEager; }
    bool fLife() மாறிலி { திரும்பு m_fLife; }
    bool fLifePost() மாறிலி { திரும்பு m_fLifePost; }
    bool fLocalize() மாறிலி { திரும்பு m_fLocalize; }
    bool fMergeCond() மாறிலி { திரும்பு m_fMergeCond; }
    bool fMergeCondMotion() மாறிலி { திரும்பு m_fMergeCondMotion; }
    bool fReloop() மாறிலி { திரும்பு m_fReloop; }
    bool fReorder() மாறிலி { திரும்பு m_fReorder; }
    bool fSlice() மாறிலி { திரும்பு m_fSlice; }
    முழு fSliceElementLimit() மாறிலி { திரும்பு m_fSliceElementLimit; }
    bool fSplit() மாறிலி { திரும்பு m_fSplit; }
    bool fSubst() மாறிலி { திரும்பு m_fSubst; }
    bool fSubstConst() மாறிலி { திரும்பு m_fSubstConst; }
    bool fTable() மாறிலி { திரும்பு m_fTable; }
    bool fTaskifyAll() மாறிலி { திரும்பு m_fTaskifyAll; }

    std::string traceClassBase() மாறிலி VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() மாறிலி VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() மாறிலி VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() மாறிலி VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() மாறிலி VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() மாறிலி VL_MT_SAFE;

    bool hierarchical() மாறிலி { திரும்பு m_hierarchical; }
    முழு hierChild() மாறிலி VL_MT_SAFE { திரும்பு m_hierChild; }
    முழு hierThreads() மாறிலி VL_MT_SAFE { திரும்பு m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_உச்சி() மாறிலி VL_MT_SAFE { திரும்பு !m_hierChild && !m_hierBlocks.empty(); }
    மாறிலி V3HierBlockOptSet& hierBlocks() மாறிலி { திரும்பு m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_உச்சி_Data_Dir() மாறிலி VL_MT_SAFE {
        திரும்பு hier_உச்சி() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    நிலையான string version() VL_PURE;
    நிலையான string argString(முழு argc, எழுத்து** argv);  ///< Return list of arguments as simple string
    string allArgsString() மாறிலி VL_MT_SAFE;  ///< Return all passed arguments as simple string
    மாறிலி std::list<std::pair<std::list<std::string>, bool>>& allArgs() மாறிலி;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_உச்சி) மாறிலி;
    வெறுமை parseOpts(File_வரி* fl, முழு argc, எழுத்து** argv) VL_MT_DISABLED;
    வெறுமை parseOptsList(File_வரி* fl, மாறிலி string& optdir, முழு argc, எழுத்து** argv) VL_MT_DISABLED;
    வெறுமை parseOptsFile(File_வரி* fl, மாறிலி string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    நிலையான string getenvBuiltins(மாறிலி string& var);
    நிலையான string getenvMAKE();
    நிலையான string getenvMAKEFLAGS();
    நிலையான string getenvPERL();
    நிலையான string getenvPYTHON3();
    நிலையான string getenvSYSTEMC();
    நிலையான string getenvSYSTEMC_ARCH();
    நிலையான string getenvSYSTEMC_INCLUDE();
    நிலையான string getenvSYSTEMC_LIBDIR();
    நிலையான string getenv_VERILATOR_வேர்();
    நிலையான string getenvVERILATOR_SOLVER();
    நிலையான string getStdPackagePath();
    நிலையான string getStdWaiverPath();
    நிலையான string getSupported(மாறிலி string& var);
    நிலையான bool systemCSystemWide();
    நிலையான bool systemCFound();  // SystemC installed, or environment points to it
    நிலையான bool coroutineSupport();  // Configured C++ compiler supports coroutines
    நிலையான bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    நிலையான bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    நிலையான bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(மாறிலி string& filename);
    string filePath(File_வரி* fl, மாறிலி string& modname, மாறிலி string& lastpath,
                    மாறிலி string& errmsg);
    string file_Path_Looked_செய்தி(File_வரி* fl, மாறிலி string& modname);
    V3LangCode fileLanguage(மாறிலி string& filename);
    நிலையான bool fileStatNormal(மாறிலி string& filename);

    // METHODS (other OS)
    நிலையான வெறுமை throwSigsegv();
};

//######################################################################

#endif  // guard
