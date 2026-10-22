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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3LangCode.h"

#অন্তর্ভাব <list>
#অন্তর্ভাব <map>
#অন্তর্ভাব <set>
#অন্তর্ভাব <string>
#অন্তর্ভাব <vector>

class V3OptionsImp;
class File_সারি;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
সার্বজনিক:
    গণনা en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    গণনা en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুব { প্রত্যাগম m_e; }
    ধ্রুব বর্ণ* ascii() ধ্রুব {
        স্থির ধ্রুব বর্ণ* ধ্রুব names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        প্রত্যাগম names[m_e];
    }
    bool isDefault() ধ্রুব { প্রত্যাগম m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ধ্রুব { প্রত্যাগম m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() ধ্রুব { প্রত্যাগম m_e == OPT_TRUE; }
    bool isSetFalse() ধ্রুব { প্রত্যাগম m_e == OPT_FALSE; }
    শূন্য setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(ধ্রুব VOptionBool& lhs, ধ্রুব VOptionBool& rhs) {
    প্রত্যাগম lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুব VOptionBool& lhs, VOptionBool::en rhs) { প্রত্যাগম lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, ধ্রুব VOptionBool& rhs) { প্রত্যাগম lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ধ্রুব string m_filename;  // Filename
    ধ্রুব string m_libname;  // Libname
সার্বজনিক:
    VFileLibName(ধ্রুব string& filename, ধ্রুব string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ধ্রুব VFileLibName& rhs) = স্বভাব;
    string filename() ধ্রুব { প্রত্যাগম m_filename; }
    string libname() ধ্রুব { প্রত্যাগম m_libname; }
    bool operator==(ধ্রুব VFileLibName& rhs) ধ্রুব {
        প্রত্যাগম m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(ধ্রুব VFileLibName& rhs) ধ্রুব {
        যদি (m_filename < rhs.m_filename) প্রত্যাগম true;
        যদি (m_filename > rhs.m_filename) প্রত্যাগম false;
        প্রত্যাগম m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
সার্বজনিক:
    গণনা en : uint8_t {
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
    গণনা : uint8_t { TS_DEFAULT = TS_1PS };
    গণনা en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ধ্রুব string& মান, bool& badr);
    VTimescale(দ্বিগুণ মান, bool& badr) {
        badr = false;
        প্রতি (পূর্ণাঙ্ক i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            যদি (multiplier() == মান) ভঙ্গ;
        }
        যদি (multiplier() != মান) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ধ্রুব { প্রত্যাগম m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    স্থির শূন্য parseSlashed(File_সারি* fl, ধ্রুব বর্ণ* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    ধ্রুব বর্ণ* ascii() ধ্রুব {
        স্থির ধ্রুব বর্ণ* ধ্রুব names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        প্রত্যাগম names[m_e];
    }
    পূর্ণাঙ্ক powerOfTen() ধ্রুব { প্রত্যাগম 2 - static_cast<পূর্ণাঙ্ক>(m_e); }
    দ্বিগুণ multiplier() ধ্রুব {
        স্থির ধ্রুব দ্বিগুণ মানসমূহ[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        প্রত্যাগম মানসমূহ[m_e];
    }
};
constexpr bool operator==(ধ্রুব VTimescale& lhs, ধ্রুব VTimescale& rhs) {
    প্রত্যাগম lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুব VTimescale& lhs, VTimescale::en rhs) { প্রত্যাগম lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, ধ্রুব VTimescale& rhs) { প্রত্যাগম lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(ধ্রুব VTimescale& lhs, ধ্রুব VTimescale& rhs) {
    প্রত্যাগম lhs.m_e > rhs.m_e;
}
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুব VTimescale& rhs) {
    প্রত্যাগম os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
সার্বজনিক:
    গণনা en : uint8_t {
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
    explicit VFsmExpandType(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুব { প্রত্যাগম m_e; }
    constexpr ধ্রুব বর্ণ* ascii() ধ্রুব {
        constexpr ধ্রুব বর্ণ* ধ্রুব names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        প্রত্যাগম names[m_e];
    }
};
constexpr bool operator==(ধ্রুব VFsmExpandType& lhs, ধ্রুব VFsmExpandType& rhs) VL_MT_SAFE {
    প্রত্যাগম lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুব VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    প্রত্যাগম lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, ধ্রুব VFsmExpandType& rhs) VL_MT_SAFE {
    প্রত্যাগম lhs == rhs.m_e;
}
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুব VFsmExpandType& rhs) VL_MT_SAFE {
    প্রত্যাগম os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
সার্বজনিক:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ধ্রুব std::string, std::string>;

গুপ্ত:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

সার্বজনিক:
    explicit V3HierarchicalBlockOption(ধ্রুব string& optstring);
    ধ্রুব string& origName() ধ্রুব { প্রত্যাগম m_origName; }
    ধ্রুব string& mangledName() ধ্রুব { প্রত্যাগম m_mangledName; }
    ধ্রুব ParamStrMap params() ধ্রুব { প্রত্যাগম m_parameters; }
};

using V3HierBlockOptSet = std::map<ধ্রুব std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
সার্বজনিক:
গুপ্ত:
    // TYPES
    using DebugLevelMap = std::map<ধ্রুব std::string, unsigned>;

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
    std::map<ধ্রুব string, string> m_parameters;  // Parameters
    std::map<ধ্রুব string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_সারি = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_দ্বিমিক = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_রাশিমালা = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_সারি = false;    // main switch: --coverage-block
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
    bool m_debug_স্তূপ_Check = false;  // main switch: --debug-stack-check
    পূর্ণাঙ্ক m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_no_Trace_শীর্ষ = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    পূর্ণাঙ্ক         m_buildJobs = -1;    // main switch: --build-jobs, -j
    পূর্ণাঙ্ক         m_coverage_রাশিমালা_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    পূর্ণাঙ্ক         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    পূর্ণাঙ্ক         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    পূর্ণাঙ্ক         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    পূর্ণাঙ্ক         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    পূর্ণাঙ্ক         m_expandLimit = 256;  // main switch: --expand-limit
    পূর্ণাঙ্ক         m_gateStmts = 100;    // main switch: --gate-stmts
    পূর্ণাঙ্ক         m_hierChild = 0;      // main switch: --hierarchical-child
    পূর্ণাঙ্ক         m_hierThreads = 0;      // main switch: --hierarchical-threads
    পূর্ণাঙ্ক         m_ifDepth = 0;      // main switch: --if-depth
    পূর্ণাঙ্ক         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    পূর্ণাঙ্ক         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    পূর্ণাঙ্ক         m_inlineMult = 2000;   // main switch: --inline-mult
    পূর্ণাঙ্ক         m_instr_গুনতি_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    পূর্ণাঙ্ক         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    পূর্ণাঙ্ক         m_max_অঙ্ক_Width = 65536;  // main switch: --max-num-width
    পূর্ণাঙ্ক         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    পূর্ণাঙ্ক         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    পূর্ণাঙ্ক         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    পূর্ণাঙ্ক         m_outputSplit = 20000;  // main switch: --output-split
    পূর্ণাঙ্ক         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    পূর্ণাঙ্ক         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    পূর্ণাঙ্ক         m_pinsBv = 65;       // main switch: --pins-bv
    পূর্ণাঙ্ক         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    পূর্ণাঙ্ক         m_publicDepth = 0;   // main switch: --public-depth
    পূর্ণাঙ্ক         m_reloopLimit = 40; // main switch: --reloop-limit
    পূর্ণাঙ্ক         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    পূর্ণাঙ্ক         m_threads = 1;      // main switch: --threads
    পূর্ণাঙ্ক         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    পূর্ণাঙ্ক         m_traceDepth = 0;   // main switch: --trace-depth
    পূর্ণাঙ্ক         m_traceMaxArray = 32;  // main switch: --trace-max-array
    পূর্ণাঙ্ক         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    পূর্ণাঙ্ক         m_unroll_গুনতি = 64;  // main switch: --unroll-count
    পূর্ণাঙ্ক         m_unrollLimit = 16384;  // main switch: --unroll-limit
    পূর্ণাঙ্ক         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    পূর্ণাঙ্ক         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    পূর্ণাঙ্ক         m_verilateJobs = -1;  // main switch: --verilate-jobs

    পূর্ণাঙ্ক         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    পূর্ণাঙ্ক         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    পূর্ণাঙ্ক         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_শীর্ষ_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_চাবি;   // main switch: --protect-key
    string      m_শীর্ষ_Module;    // main switch: --top-module
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
    bool m_f_Case_বৃক্ষ;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_বৃক্ষ;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ঠেলো_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    পূর্ণাঙ্ক  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    শূন্য addArg(বর্ণ** argv, size_t গুনতি, bool isForRerun);
    শূন্য addArg(ধ্রুব std::string& arg, bool isForRerun);
    শূন্য add_সারি_Arg(ধ্রুব string& arg);
    শূন্য addDefine(ধ্রুব string& defline, bool allowPlus) VL_MT_DISABLED;
    শূন্য addFuture(ধ্রুব string& flag);
    শূন্য addFuture0(ধ্রুব string& flag);
    শূন্য addFuture1(ধ্রুব string& flag);
    শূন্য addIncDirUser(ধ্রুব string& incdir);  // User requested
    শূন্য addIncDirFallback(ধ্রুব string& incdir);  // Low priority if not found otherwise
    শূন্য addParameter(ধ্রুব string& paramline, bool allowPlus);
    শূন্য addLangExt(ধ্রুব string& langext, ধ্রুব V3LangCode& lc);
    শূন্য addLibExtV(ধ্রুব string& libext);
    শূন্য optimize(পূর্ণাঙ্ক level);
    শূন্য showVersion(bool verbose);
    শূন্য coverage(bool flag) {
        m_coverage_সারি = m_coverageBranch = m_coverageToggle = m_coverage_রাশিমালা = m_coverageFsm
            = m_coverageUser = flag;
    }
    স্থির bool suffixed(ধ্রুব string& sw, ধ্রুব বর্ণ* arg);
    স্থির string parseFileArg(ধ্রুব string& optdir, ধ্রুব string& relfilename);
    string filePathCheckOneDir(ধ্রুব string& modname, ধ্রুব string& dirname);
    স্থির পূর্ণাঙ্ক stripOptionsForChildRun(ধ্রুব string& opt, bool for_শীর্ষ);
    শূন্য validateIdentifier(File_সারি* fl, ধ্রুব string& arg, ধ্রুব string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

সার্বজনিক:
    V3Options();
    ~V3Options();
    শূন্য setDebugMode(পূর্ণাঙ্ক level);
    unsigned debugLevel(ধ্রুব string& tag) ধ্রুব VL_MT_SAFE;
    unsigned debugSrcLevel(ধ্রুব string& srcfile_path) ধ্রুব VL_MT_SAFE;
    unsigned dumpLevel(ধ্রুব string& tag) ধ্রুব VL_MT_SAFE;
    unsigned dumpSrcLevel(ধ্রুব string& srcfile_path) ধ্রুব VL_MT_SAFE;

    // METHODS
    শূন্য addCppFile(ধ্রুব string& filename);
    শূন্য addCFlags(ধ্রুব string& filename);
    শূন্য addCompilerIncludes(ধ্রুব string& filename);
    শূন্য addLdLibFile(ধ্রুব string& filename);
    শূন্য addLdLibs(ধ্রুব string& filename);
    শূন্য addMakeFlags(ধ্রুব string& filename);
    শূন্য addLibMapFile(ধ্রুব string& filename);
    শূন্য addLibraryFile(ধ্রুব string& filename, ধ্রুব string& libname);
    শূন্য addVFile(ধ্রুব string& filename, ধ্রুব string& libname);
    শূন্য addVltFile(ধ্রুব string& filename, ধ্রুব string& libname);
    শূন্য addForceInc(ধ্রুব string& filename);
    bool available() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_available; }
    শূন্য ccSet();
    শূন্য decorations(File_সারি* fl, ধ্রুব string& filename);
    শূন্য notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ধ্রুব { প্রত্যাগম m_preprocOnly; }
    bool makePhony() ধ্রুব { প্রত্যাগম m_makePhony; }
    bool preproc_No_সারি() ধ্রুব { প্রত্যাগম m_preproc_No_সারি; }
    bool preprocResolve() ধ্রুব { প্রত্যাগম m_preprocResolve; }
    পূর্ণাঙ্ক preprocTokenLimit() ধ্রুব { প্রত্যাগম m_preprocTokenLimit; }
    bool underlineZero() ধ্রুব { প্রত্যাগম m_underlineZero; }
    bool systemC() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_systemC; }
    bool savable() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_savable; }
    VOptionBool schedZeroDelay() ধ্রুব { প্রত্যাগম m_schedZeroDelay; }
    bool stats() ধ্রুব { প্রত্যাগম m_stats; }
    bool statsVars() ধ্রুব { প্রত্যাগম m_statsVars; }
    bool stdPackage() ধ্রুব { প্রত্যাগম m_stdPackage; }
    bool stdWaiver() ধ্রুব { প্রত্যাগম m_stdWaiver; }
    bool assertOn() ধ্রুব { প্রত্যাগম m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ধ্রুব { প্রত্যাগম m_assertCase; }
    bool autoflush() ধ্রুব { প্রত্যাগম m_autoflush; }
    bool bboxSys() ধ্রুব { প্রত্যাগম m_bboxSys; }
    bool bboxUnsup() ধ্রুব { প্রত্যাগম m_bboxUnsup; }
    bool build() ধ্রুব { প্রত্যাগম m_build; }
    string buildDepBin() ধ্রুব { প্রত্যাগম m_buildDepBin; }
    শূন্য buildDepBin(ধ্রুব string& flag) { m_buildDepBin = flag; }
    bool context() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_context; }
    bool coverage() ধ্রুব VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        প্রত্যাগম m_coverage_সারি || m_coverageBranch || m_coverageToggle || m_coverage_রাশিমালা
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ধ্রুব VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        প্রত্যাগম m_coverage_সারি || m_coverageBranch || m_coverageToggle || m_coverage_রাশিমালা
               || m_coverageUser;
    }
    bool coverageBranch() ধ্রুব { প্রত্যাগম m_coverageBranch; }
    bool coverage_রাশিমালা() ধ্রুব { প্রত্যাগম m_coverage_রাশিমালা; }
    bool coverageFsm() ধ্রুব { প্রত্যাগম m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ধ্রুব { প্রত্যাগম m_coverageFsmExpand; }
    পূর্ণাঙ্ক coverageFsmMaxArcs() ধ্রুব { প্রত্যাগম m_coverageFsmMaxArcs; }
    bool coverage_সারি() ধ্রুব { প্রত্যাগম m_coverage_সারি; }
    bool coverageMergeInstances() ধ্রুব { প্রত্যাগম m_coverageMergeInstances; }
    bool coveragePerInstance() ধ্রুব { প্রত্যাগম m_coveragePerInstance; }
    bool coverageToggle() ধ্রুব { প্রত্যাগম m_coverageToggle; }
    bool coverageUnderscore() ধ্রুব { প্রত্যাগম m_coverageUnderscore; }
    bool coverageUser() ধ্রুব { প্রত্যাগম m_coverageUser; }
    bool debugCheck() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_debugCheck; }
    bool debugCollision() ধ্রুব { প্রত্যাগম m_debugCollision; }
    bool debugEmitV() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_debugEmitV; }
    bool debugExitElab() ধ্রুব { প্রত্যাগম m_debugExitElab; }
    bool debugExitParse() ধ্রুব { প্রত্যাগম m_debugExitParse; }
    bool debugLeak() ধ্রুব { প্রত্যাগম m_debugLeak; }
    bool debugNondeterminism() ধ্রুব { প্রত্যাগম m_debugNondeterminism; }
    bool debugPartition() ধ্রুব { প্রত্যাগম m_debugPartition; }
    bool debugPreprocPassthru() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_debugPreprocPassthru; }
    bool debugProtect() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_debugProtect; }
    পূর্ণাঙ্ক debugRuntimeTimeout() ধ্রুব { প্রত্যাগম m_debugRuntimeTimeout; }
    bool debugSelfTest() ধ্রুব { প্রত্যাগম m_debugSelfTest; }
    bool debug_স্তূপ_Check() ধ্রুব { প্রত্যাগম m_debug_স্তূপ_Check; }
    bool debugWidth() ধ্রুব VL_PURE { প্রত্যাগম m_debugWidth; }
    bool decoration() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_decoration; }
    bool decorationNodes() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_decorationNodes; }
    bool diagnosticsSarif() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_diagnosticsSarif; }
    bool dpiHdrOnly() ধ্রুব { প্রত্যাগম m_dpiHdrOnly; }
    bool dumpAstPatterns() ধ্রুব {
        প্রত্যাগম m_dumpLevel.গুনতি("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() ধ্রুব { প্রত্যাগম m_dumpLevel.গুনতি("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ধ্রুব {
        প্রত্যাগম m_dumpLevel.গুনতি("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_বৃক্ষ_Dot() ধ্রুব {
        প্রত্যাগম m_dumpLevel.গুনতি("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ধ্রুব { প্রত্যাগম m_emitAccessors; }
    bool exe() ধ্রুব { প্রত্যাগম m_exe; }
    bool flatten() ধ্রুব { প্রত্যাগম m_flatten; }
    bool fourstate() ধ্রুব { প্রত্যাগম m_fourstate; }
    bool gmake() ধ্রুব { প্রত্যাগম m_gmake; }
    bool makeJson() ধ্রুব { প্রত্যাগম m_makeJson; }
    bool threadsDpiPure() ধ্রুব { প্রত্যাগম m_threadsDpiPure; }
    bool threadsDpiUnpure() ধ্রুব { প্রত্যাগম m_threadsDpiUnpure; }
    bool threadsCoarsen() ধ্রুব { প্রত্যাগম m_threadsCoarsen; }
    VOptionBool timing() ধ্রুব { প্রত্যাগম m_timing; }
    bool trace() ধ্রুব { প্রত্যাগম m_trace; }
    bool traceCoverage() ধ্রুব { প্রত্যাগম m_traceCoverage; }
    bool traceEnabledFst() ধ্রুব { প্রত্যাগম m_traceEnabledFst; }
    bool traceEnabledSaif() ধ্রুব { প্রত্যাগম m_traceEnabledSaif; }
    bool traceEnabledVcd() ধ্রুব { প্রত্যাগম m_traceEnabledVcd; }
    bool traceParams() ধ্রুব { প্রত্যাগম m_traceParams; }
    bool traceStructs() ধ্রুব { প্রত্যাগম m_traceStructs; }
    bool traceUnderscore() ধ্রুব { প্রত্যাগম m_traceUnderscore; }
    bool main() ধ্রুব { প্রত্যাগম m_main; }
    bool outFormatOk() ধ্রুব { প্রত্যাগম m_outFormatOk; }
    bool jsonOnly() ধ্রুব { প্রত্যাগম m_jsonOnly; }
    bool keepTempFiles() ধ্রুব { প্রত্যাগম (V3Error::debugDefault() != 0); }
    bool pedantic() ধ্রুব { প্রত্যাগম m_pedantic; }
    bool pinsInoutEnables() ধ্রুব { প্রত্যাগম m_pinsInoutEnables; }
    bool pinsScUint() ধ্রুব { প্রত্যাগম m_pinsScUint; }
    bool pinsScUintBool() ধ্রুব { প্রত্যাগম m_pinsScUintBool; }
    bool pinsScBigUint() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_pinsScBigUint; }
    bool pinsUint8() ধ্রুব { প্রত্যাগম m_pinsUint8; }
    bool preprocComments() ধ্রুব { প্রত্যাগম m_preprocComments; }
    bool preprocDefines() ধ্রুব { প্রত্যাগম m_preprocDefines; }
    bool profC() ধ্রুব { প্রত্যাগম m_profC; }
    bool profCFuncs() ধ্রুব { প্রত্যাগম m_profCFuncs; }
    bool profExec() ধ্রুব { প্রত্যাগম m_profExec; }
    bool profPgo() ধ্রুব { প্রত্যাগম m_profPgo; }
    bool usesProfiler() ধ্রুব { প্রত্যাগম profExec() || profPgo(); }
    bool protectIds() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_protectIds; }
    bool allPublic() ধ্রুব { প্রত্যাগম m_public; }
    bool publicParams() ধ্রুব { প্রত্যাগম m_publicParams; }
    bool publicOff() ধ্রুব { প্রত্যাগম m_publicIgnore; }
    bool publicFlatRW() ধ্রুব { প্রত্যাগম m_publicFlatRW; }
    পূর্ণাঙ্ক publicDepth() ধ্রুব { প্রত্যাগম m_publicDepth; }
    bool anyPublicFlat() ধ্রুব { প্রত্যাগম m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_lintOnly; }
    bool ignc() ধ্রুব { প্রত্যাগম m_ignc; }
    bool quietBuild() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_quietBuild; }
    bool quietExit() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_quietExit; }
    bool quietStats() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_quietStats; }
    bool reportUnoptflat() ধ্রুব { প্রত্যাগম m_reportUnoptflat; }
    bool verilate() ধ্রুব { প্রত্যাগম m_verilate; }
    bool vpi() ধ্রুব { প্রত্যাগম m_vpi.isTrue(); }
    bool waiverMultiline() ধ্রুব { প্রত্যাগম m_waiverMultiline; }
    bool xInitialEdge() ধ্রুব { প্রত্যাগম m_xInitialEdge; }
    bool serializeOnly() ধ্রুব { প্রত্যাগম m_jsonOnly; }
    bool শীর্ষ_Ifaces_Supported() ধ্রুব { প্রত্যাগম lintOnly() && !hierarchical(); }

    পূর্ণাঙ্ক buildJobs() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_buildJobs; }
    পূর্ণাঙ্ক convergeLimit() ধ্রুব { প্রত্যাগম m_convergeLimit; }
    পূর্ণাঙ্ক coverage_রাশিমালা_Max() ধ্রুব { প্রত্যাগম m_coverage_রাশিমালা_Max; }
    uint32_t coverageMaxBins() ধ্রুব { প্রত্যাগম m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ধ্রুব { প্রত্যাগম m_coverageMaxRealBins; }
    পূর্ণাঙ্ক coverageMaxWidth() ধ্রুব { প্রত্যাগম m_coverageMaxWidth; }
    পূর্ণাঙ্ক debugAllocRandom() ধ্রুব { প্রত্যাগম m_debugAllocRandom; }
    bool dump_বৃক্ষ_Addrids() ধ্রুব VL_MT_SAFE;
    পূর্ণাঙ্ক expandLimit() ধ্রুব { প্রত্যাগম m_expandLimit; }
    পূর্ণাঙ্ক gateStmts() ধ্রুব { প্রত্যাগম m_gateStmts; }
    পূর্ণাঙ্ক ifDepth() ধ্রুব { প্রত্যাগম m_ifDepth; }
    পূর্ণাঙ্ক inlineCFuncs() ধ্রুব { প্রত্যাগম m_inlineCFuncs; }
    পূর্ণাঙ্ক inlineCFuncsProduct() ধ্রুব { প্রত্যাগম m_inlineCFuncsProduct; }
    পূর্ণাঙ্ক inlineMult() ধ্রুব { প্রত্যাগম m_inlineMult; }
    পূর্ণাঙ্ক instr_গুনতি_Dpi() ধ্রুব { প্রত্যাগম m_instr_গুনতি_Dpi; }
    পূর্ণাঙ্ক localizeMaxSize() ধ্রুব { প্রত্যাগম m_localizeMaxSize; }
    bool jsonEditNums() ধ্রুব { প্রত্যাগম m_jsonEditNums; }
    bool jsonIds() ধ্রুব { প্রত্যাগম m_jsonIds; }
    VOptionBool makeDepend() ধ্রুব { প্রত্যাগম m_makeDepend; }
    পূর্ণাঙ্ক max_অঙ্ক_Width() ধ্রুব { প্রত্যাগম m_max_অঙ্ক_Width; }
    পূর্ণাঙ্ক funcRecursionDepth() ধ্রুব { প্রত্যাগম m_funcRecursion; }
    পূর্ণাঙ্ক moduleRecursionDepth() ধ্রুব { প্রত্যাগম m_moduleRecursion; }
    পূর্ণাঙ্ক outputSplit() ধ্রুব { প্রত্যাগম m_outputSplit; }
    পূর্ণাঙ্ক outputSplitCFuncs() ধ্রুব { প্রত্যাগম m_outputSplitCFuncs; }
    পূর্ণাঙ্ক outputSplitCTrace() ধ্রুব { প্রত্যাগম m_outputSplitCTrace; }
    পূর্ণাঙ্ক outputGroups() ধ্রুব { প্রত্যাগম m_outputGroups; }
    int64_t outputGroupsSize() ধ্রুব { প্রত্যাগম m_outputGroupsSize; }
    পূর্ণাঙ্ক pinsBv() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_pinsBv; }
    পূর্ণাঙ্ক reloopLimit() ধ্রুব { প্রত্যাগম m_reloopLimit; }
    পূর্ণাঙ্ক replicationLimit() ধ্রুব { প্রত্যাগম m_replicationLimit; }
    VOptionBool skipIdentical() ধ্রুব { প্রত্যাগম m_skipIdentical; }
    bool stopFail() ধ্রুব { প্রত্যাগম m_stopFail; }
    পূর্ণাঙ্ক threads() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_threads; }
    পূর্ণাঙ্ক threadsMaxMTasks() ধ্রুব { প্রত্যাগম m_threadsMaxMTasks; }
    bool mtasks() ধ্রুব VL_MT_SAFE { প্রত্যাগম (m_threads > 1); }
    VTimescale timeDefaultPrec() ধ্রুব { প্রত্যাগম m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() ধ্রুব { প্রত্যাগম m_timeDefaultUnit; }
    VTimescale timeOverridePrec() ধ্রুব { প্রত্যাগম m_timeOverridePrec; }
    VTimescale timeOverrideUnit() ধ্রুব { প্রত্যাগম m_timeOverrideUnit; }
    VTimescale timeComputePrec(ধ্রুব VTimescale& flag) ধ্রুব;
    VTimescale timeComputeUnit(ধ্রুব VTimescale& flag) ধ্রুব;
    পূর্ণাঙ্ক traceDepth() ধ্রুব { প্রত্যাগম m_traceDepth; }
    পূর্ণাঙ্ক traceMaxArray() ধ্রুব { প্রত্যাগম m_traceMaxArray; }
    পূর্ণাঙ্ক traceMaxWidth() ধ্রুব { প্রত্যাগম m_traceMaxWidth; }
    bool useTraceParallel() ধ্রুব {
        প্রত্যাগম trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    পূর্ণাঙ্ক unroll_গুনতি() ধ্রুব { প্রত্যাগম m_unroll_গুনতি; }
    পূর্ণাঙ্ক unrollLimit() ধ্রুব { প্রত্যাগম m_unrollLimit; }
    পূর্ণাঙ্ক unrollStmts() ধ্রুব { প্রত্যাগম m_unrollStmts; }
    পূর্ণাঙ্ক constraintArrayLimit() ধ্রুব { প্রত্যাগম m_constraintArrayLimit; }
    পূর্ণাঙ্ক verilateJobs() ধ্রুব { প্রত্যাগম m_verilateJobs; }

    পূর্ণাঙ্ক compLimitBlocks() ধ্রুব { প্রত্যাগম m_compLimitBlocks; }
    পূর্ণাঙ্ক compLimitMembers() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_compLimitMembers; }
    পূর্ণাঙ্ক compLimitParens() ধ্রুব { প্রত্যাগম m_compLimitParens; }

    string diagnosticsSarifOutput() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ধ্রুব { প্রত্যাগম m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() ধ্রুব { প্রত্যাগম m_hierParamsFile; }
    string jsonOnlyOutput() ধ্রুব { প্রত্যাগম m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ধ্রুব { প্রত্যাগম m_jsonOnlyMetaOutput; }
    string l2Name() ধ্রুব { প্রত্যাগম m_l2Name; }
    string libCreate() ধ্রুব { প্রত্যাগম m_libCreate; }
    string libCreateName(bool shared) ধ্রুব {
        string libName = "lib" + libCreate();
        যদি (shared) {
            libName += ".so";
        } অন্যথা {
            libName += ".a";
        }
        প্রত্যাগম libName;
    }
    string main_শীর্ষ_Name() ধ্রুব { প্রত্যাগম m_main_শীর্ষ_Name; }
    string makeDir() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_makeDir; }
    string modPrefix() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_modPrefix; }
    string pipeFilter() ধ্রুব { প্রত্যাগম m_pipeFilter; }
    string prefix() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_চাবি_Provided() ধ্রুব { প্রত্যাগম !m_protect_চাবি.empty(); }
    string protect_চাবি_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string শীর্ষ_Module() ধ্রুব { প্রত্যাগম m_শীর্ষ_Module; }  // As AstNode::encodeName()
    bool no_Trace_শীর্ষ() ধ্রুব { প্রত্যাগম m_no_Trace_শীর্ষ; }
    string unusedRegexp() ধ্রুব { প্রত্যাগম m_unusedRegexp; }
    string waiverOutput() ধ্রুব { প্রত্যাগম m_waiverOutput; }
    string work() ধ্রুব { প্রত্যাগম m_work; }
    bool isWaiverOutput() ধ্রুব { প্রত্যাগম !m_waiverOutput.empty(); }
    string xAssign() ধ্রুব { প্রত্যাগম m_xAssign; }
    string xInitial() ধ্রুব { প্রত্যাগম m_xInitial; }

    ধ্রুব VStringSet& cppFiles() ধ্রুব { প্রত্যাগম m_cppFiles; }
    ধ্রুব VStringList& cFlags() ধ্রুব { প্রত্যাগম m_cFlags; }
    ধ্রুব VStringSet& compilerIncludes() ধ্রুব { প্রত্যাগম m_compilerIncludes; }
    ধ্রুব VStringList& ldLibFiles() ধ্রুব { প্রত্যাগম m_ldLibFiles; }
    ধ্রুব VStringList& ldLibs() ধ্রুব { প্রত্যাগম m_ldLibs; }
    ধ্রুব VStringList& makeFlags() ধ্রুব { প্রত্যাগম m_makeFlags; }
    ধ্রুব VFileLibSet& libraryFiles() ধ্রুব { প্রত্যাগম m_libraryFiles; }
    ধ্রুব VStringSet& libmapFiles() ধ্রুব { প্রত্যাগম m_libmapFiles; }
    ধ্রুব VFileLibList& vFiles() ধ্রুব { প্রত্যাগম m_vFiles; }
    ধ্রুব VFileLibSet& vltFiles() ধ্রুব { প্রত্যাগম m_vltFiles; }
    ধ্রুব VStringList& forceIncs() ধ্রুব { প্রত্যাগম m_forceIncs; }

    bool hasParameter(ধ্রুব string& name);
    string parameter(ধ্রুব string& name);
    শূন্য checkParameters();

    bool isFuture(ধ্রুব string& flag) ধ্রুব;
    bool isFuture0(ধ্রুব string& flag) ধ্রুব;
    bool isFuture1(ধ্রুব string& flag) ধ্রুব;
    bool isLibraryFile(ধ্রুব string& filename, ধ্রুব string& libname) ধ্রুব;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ধ্রুব { প্রত্যাগম m_fAcycSimp; }
    bool fAssemble() ধ্রুব { প্রত্যাগম m_fAssemble; }
    bool fBitScanLoops() ধ্রুব { প্রত্যাগম m_fBitScanLoops; }
    bool fBitblast() ধ্রুব { প্রত্যাগম m_fBitblast; }
    bool fCaseDecoder() ধ্রুব { প্রত্যাগম m_fCaseDecoder; }
    bool fCaseTable() ধ্রুব { প্রত্যাগম m_fCaseTable; }
    bool f_Case_বৃক্ষ() ধ্রুব { প্রত্যাগম m_f_Case_বৃক্ষ; }
    bool fCombine() ধ্রুব { প্রত্যাগম m_fCombine; }
    bool fConst() ধ্রুব { প্রত্যাগম m_fConst; }
    bool fConstBeforeDfg() ধ্রুব { প্রত্যাগম m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_বৃক্ষ() ধ্রুব { প্রত্যাগম m_f_Const_Bit_Op_বৃক্ষ; }
    bool fConstEager() ধ্রুব { প্রত্যাগম m_fConstEager; }
    bool fDedupe() ধ্রুব { প্রত্যাগম m_fDedupe; }
    bool fDfg() ধ্রুব { প্রত্যাগম m_fDfg; }
    bool fDfgPeephole() ধ্রুব { প্রত্যাগম m_fDfgPeephole; }
    bool f_Dfg_ঠেলো_Down_Sels() ধ্রুব { প্রত্যাগম m_f_Dfg_ঠেলো_Down_Sels; }
    bool fDfgSynthesizeAll() ধ্রুব { প্রত্যাগম m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ধ্রুব std::string& name) ধ্রুব {
        প্রত্যাগম !m_fDfgPeepholeDisabled.গুনতি(name);
    }
    bool fDeadAssigns() ধ্রুব { প্রত্যাগম m_fDeadAssigns; }
    bool fDeadCells() ধ্রুব { প্রত্যাগম m_fDeadCells; }
    bool fDeadMethods() ধ্রুব { প্রত্যাগম m_fDeadMethods; }
    bool fDecompose() ধ্রুব { প্রত্যাগম m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ধ্রুব { প্রত্যাগম m_fDecomposePacked; }
    bool fDecomposeUnpacked() ধ্রুব { প্রত্যাগম m_fDecomposeUnpacked; }
    bool fExpand() ধ্রুব { প্রত্যাগম m_fExpand; }
    bool fFuncBalanceCat() ধ্রুব { প্রত্যাগম m_fFuncBalanceCat; }
    bool fFuncSplitCat() ধ্রুব { প্রত্যাগম m_fFuncSplitCat; }
    bool fFunc() ধ্রুব { প্রত্যাগম fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() ধ্রুব { প্রত্যাগম m_fGate; }
    VOptionBool fIcoChangeDetect() ধ্রুব { প্রত্যাগম m_fIcoChangeDetect; }
    bool fInline() ধ্রুব { প্রত্যাগম m_fInline; }
    bool fInlineCFuncs() ধ্রুব { প্রত্যাগম m_fInlineCFuncs; }
    bool fInlineFuncs() ধ্রুব { প্রত্যাগম m_fInlineFuncs; }
    bool fInlineFuncsEager() ধ্রুব { প্রত্যাগম m_fInlineFuncsEager; }
    bool fLife() ধ্রুব { প্রত্যাগম m_fLife; }
    bool fLifePost() ধ্রুব { প্রত্যাগম m_fLifePost; }
    bool fLocalize() ধ্রুব { প্রত্যাগম m_fLocalize; }
    bool fMergeCond() ধ্রুব { প্রত্যাগম m_fMergeCond; }
    bool fMergeCondMotion() ধ্রুব { প্রত্যাগম m_fMergeCondMotion; }
    bool fReloop() ধ্রুব { প্রত্যাগম m_fReloop; }
    bool fReorder() ধ্রুব { প্রত্যাগম m_fReorder; }
    bool fSlice() ধ্রুব { প্রত্যাগম m_fSlice; }
    পূর্ণাঙ্ক fSliceElementLimit() ধ্রুব { প্রত্যাগম m_fSliceElementLimit; }
    bool fSplit() ধ্রুব { প্রত্যাগম m_fSplit; }
    bool fSubst() ধ্রুব { প্রত্যাগম m_fSubst; }
    bool fSubstConst() ধ্রুব { প্রত্যাগম m_fSubstConst; }
    bool fTable() ধ্রুব { প্রত্যাগম m_fTable; }
    bool fTaskifyAll() ধ্রুব { প্রত্যাগম m_fTaskifyAll; }

    std::string traceClassBase() ধ্রুব VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() ধ্রুব VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() ধ্রুব VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() ধ্রুব VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() ধ্রুব VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() ধ্রুব VL_MT_SAFE;

    bool hierarchical() ধ্রুব { প্রত্যাগম m_hierarchical; }
    পূর্ণাঙ্ক hierChild() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_hierChild; }
    পূর্ণাঙ্ক hierThreads() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_শীর্ষ() ধ্রুব VL_MT_SAFE { প্রত্যাগম !m_hierChild && !m_hierBlocks.empty(); }
    ধ্রুব V3HierBlockOptSet& hierBlocks() ধ্রুব { প্রত্যাগম m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_শীর্ষ_Data_Dir() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম hier_শীর্ষ() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    স্থির string version() VL_PURE;
    স্থির string argString(পূর্ণাঙ্ক argc, বর্ণ** argv);  ///< Return list of arguments as simple string
    string allArgsString() ধ্রুব VL_MT_SAFE;  ///< Return all passed arguments as simple string
    ধ্রুব std::list<std::pair<std::list<std::string>, bool>>& allArgs() ধ্রুব;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_শীর্ষ) ধ্রুব;
    শূন্য parseOpts(File_সারি* fl, পূর্ণাঙ্ক argc, বর্ণ** argv) VL_MT_DISABLED;
    শূন্য parseOptsList(File_সারি* fl, ধ্রুব string& optdir, পূর্ণাঙ্ক argc, বর্ণ** argv) VL_MT_DISABLED;
    শূন্য parseOptsFile(File_সারি* fl, ধ্রুব string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    স্থির string getenvBuiltins(ধ্রুব string& var);
    স্থির string getenvMAKE();
    স্থির string getenvMAKEFLAGS();
    স্থির string getenvPERL();
    স্থির string getenvPYTHON3();
    স্থির string getenvSYSTEMC();
    স্থির string getenvSYSTEMC_ARCH();
    স্থির string getenvSYSTEMC_INCLUDE();
    স্থির string getenvSYSTEMC_LIBDIR();
    স্থির string getenv_VERILATOR_মূল();
    স্থির string getenvVERILATOR_SOLVER();
    স্থির string getStdPackagePath();
    স্থির string getStdWaiverPath();
    স্থির string getSupported(ধ্রুব string& var);
    স্থির bool systemCSystemWide();
    স্থির bool systemCFound();  // SystemC installed, or environment points to it
    স্থির bool coroutineSupport();  // Configured C++ compiler supports coroutines
    স্থির bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    স্থির bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    স্থির bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ধ্রুব string& filename);
    string filePath(File_সারি* fl, ধ্রুব string& modname, ধ্রুব string& lastpath,
                    ধ্রুব string& errmsg);
    string file_Path_Looked_বার্তা(File_সারি* fl, ধ্রুব string& modname);
    V3LangCode fileLanguage(ধ্রুব string& filename);
    স্থির bool fileStatNormal(ধ্রুব string& filename);

    // METHODS (other OS)
    স্থির শূন্য throwSigsegv();
};

//######################################################################

#endif  // guard
