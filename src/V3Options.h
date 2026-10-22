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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3LangCode.h"

#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <vector>

class V3OptionsImp;
class File_ଧାଡ଼ି;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    ଗଣନା en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        ଫେରନ୍ତୁ names[m_e];
    }
    bool isDefault() ଅଚଳ { ଫେରନ୍ତୁ m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ଅଚଳ { ଫେରନ୍ତୁ m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() ଅଚଳ { ଫେରନ୍ତୁ m_e == OPT_TRUE; }
    bool isSetFalse() ଅଚଳ { ଫେରନ୍ତୁ m_e == OPT_FALSE; }
    ଶୂନ୍ୟ setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(ଅଚଳ VOptionBool& lhs, ଅଚଳ VOptionBool& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VOptionBool& lhs, VOptionBool::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, ଅଚଳ VOptionBool& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ଅଚଳ string m_filename;  // Filename
    ଅଚଳ string m_libname;  // Libname
ସର୍ବସାଧାରଣ:
    VFileLibName(ଅଚଳ string& filename, ଅଚଳ string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ଅଚଳ VFileLibName& rhs) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    string filename() ଅଚଳ { ଫେରନ୍ତୁ m_filename; }
    string libname() ଅଚଳ { ଫେରନ୍ତୁ m_libname; }
    bool operator==(ଅଚଳ VFileLibName& rhs) ଅଚଳ {
        ଫେରନ୍ତୁ m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(ଅଚଳ VFileLibName& rhs) ଅଚଳ {
        ଯଦି (m_filename < rhs.m_filename) ଫେରନ୍ତୁ true;
        ଯଦି (m_filename > rhs.m_filename) ଫେରନ୍ତୁ false;
        ଫେରନ୍ତୁ m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
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
    ଗଣନା : uint8_t { TS_DEFAULT = TS_1PS };
    ଗଣନା en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ଅଚଳ string& ମୂଲ୍ୟ, bool& badr);
    VTimescale(ଦ୍ୱିଗୁଣ ମୂଲ୍ୟ, bool& badr) {
        badr = false;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            ଯଦି (multiplier() == ମୂଲ୍ୟ) ଭାଙ୍ଗ;
        }
        ଯଦି (multiplier() != ମୂଲ୍ୟ) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ଅଚଳ { ଫେରନ୍ତୁ m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ସ୍ଥିର ଶୂନ୍ୟ parseSlashed(File_ଧାଡ଼ି* fl, ଅଚଳ ଅକ୍ଷର* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        ଫେରନ୍ତୁ names[m_e];
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା powerOfTen() ଅଚଳ { ଫେରନ୍ତୁ 2 - static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_e); }
    ଦ୍ୱିଗୁଣ multiplier() ଅଚଳ {
        ସ୍ଥିର ଅଚଳ ଦ୍ୱିଗୁଣ ମୂଲ୍ୟଗୁଡ଼ିକ[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        ଫେରନ୍ତୁ ମୂଲ୍ୟଗୁଡ଼ିକ[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VTimescale& lhs, ଅଚଳ VTimescale& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VTimescale& lhs, VTimescale::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, ଅଚଳ VTimescale& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(ଅଚଳ VTimescale& lhs, ଅଚଳ VTimescale& rhs) {
    ଫେରନ୍ତୁ lhs.m_e > rhs.m_e;
}
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VTimescale& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t {
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
    explicit VFsmExpandType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    constexpr ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ {
        constexpr ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        ଫେରନ୍ତୁ names[m_e];
    }
};
constexpr bool operator==(ଅଚଳ VFsmExpandType& lhs, ଅଚଳ VFsmExpandType& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, ଅଚଳ VFsmExpandType& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
}
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ VFsmExpandType& rhs) VL_MT_SAFE {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
ସର୍ବସାଧାରଣ:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ଅଚଳ std::string, std::string>;

ବ୍ୟକ୍ତିଗତ:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

ସର୍ବସାଧାରଣ:
    explicit V3HierarchicalBlockOption(ଅଚଳ string& optstring);
    ଅଚଳ string& origName() ଅଚଳ { ଫେରନ୍ତୁ m_origName; }
    ଅଚଳ string& mangledName() ଅଚଳ { ଫେରନ୍ତୁ m_mangledName; }
    ଅଚଳ ParamStrMap params() ଅଚଳ { ଫେରନ୍ତୁ m_parameters; }
};

using V3HierBlockOptSet = std::map<ଅଚଳ std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
ସର୍ବସାଧାରଣ:
ବ୍ୟକ୍ତିଗତ:
    // TYPES
    using DebugLevelMap = std::map<ଅଚଳ std::string, unsigned>;

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
    std::map<ଅଚଳ string, string> m_parameters;  // Parameters
    std::map<ଅଚଳ string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_ଧାଡ଼ି = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_ଦ୍ୱିମିକ = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_ପଦସମଷ୍ଟି = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_ଧାଡ଼ି = false;    // main switch: --coverage-block
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
    bool m_debug_ଗଦା_Check = false;  // main switch: --debug-stack-check
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_no_Trace_ଶୀର୍ଷ = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_buildJobs = -1;    // main switch: --build-jobs, -j
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_coverage_ପଦସମଷ୍ଟି_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_expandLimit = 256;  // main switch: --expand-limit
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_gateStmts = 100;    // main switch: --gate-stmts
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_hierChild = 0;      // main switch: --hierarchical-child
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_hierThreads = 0;      // main switch: --hierarchical-threads
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_ifDepth = 0;      // main switch: --if-depth
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_inlineMult = 2000;   // main switch: --inline-mult
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_instr_ଗଣନା_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_max_ଅଙ୍କ_Width = 65536;  // main switch: --max-num-width
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_outputSplit = 20000;  // main switch: --output-split
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_pinsBv = 65;       // main switch: --pins-bv
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_publicDepth = 0;   // main switch: --public-depth
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_reloopLimit = 40; // main switch: --reloop-limit
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_threads = 1;      // main switch: --threads
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_traceDepth = 0;   // main switch: --trace-depth
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_traceMaxArray = 32;  // main switch: --trace-max-array
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_unroll_ଗଣନା = 64;  // main switch: --unroll-count
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_unrollLimit = 16384;  // main switch: --unroll-limit
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_verilateJobs = -1;  // main switch: --verilate-jobs

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_ଶୀର୍ଷ_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_ଚାବି;   // main switch: --protect-key
    string      m_ଶୀର୍ଷ_Module;    // main switch: --top-module
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
    bool m_f_Case_ବୃକ୍ଷ;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_ବୃକ୍ଷ;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ଠେଲ_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    ଶୂନ୍ୟ addArg(ଅକ୍ଷର** argv, size_t ଗଣନା_2, bool isForRerun);
    ଶୂନ୍ୟ addArg(ଅଚଳ std::string& arg, bool isForRerun);
    ଶୂନ୍ୟ add_ଧାଡ଼ି_Arg(ଅଚଳ string& arg);
    ଶୂନ୍ୟ addDefine(ଅଚଳ string& defline, bool allowPlus) VL_MT_DISABLED;
    ଶୂନ୍ୟ addFuture(ଅଚଳ string& flag);
    ଶୂନ୍ୟ addFuture0(ଅଚଳ string& flag);
    ଶୂନ୍ୟ addFuture1(ଅଚଳ string& flag);
    ଶୂନ୍ୟ addIncDirUser(ଅଚଳ string& incdir);  // User requested
    ଶୂନ୍ୟ addIncDirFallback(ଅଚଳ string& incdir);  // Low priority if not found otherwise
    ଶୂନ୍ୟ addParameter(ଅଚଳ string& paramline, bool allowPlus);
    ଶୂନ୍ୟ addLangExt(ଅଚଳ string& langext, ଅଚଳ V3LangCode& lc);
    ଶୂନ୍ୟ addLibExtV(ଅଚଳ string& libext);
    ଶୂନ୍ୟ optimize(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level);
    ଶୂନ୍ୟ showVersion(bool verbose);
    ଶୂନ୍ୟ coverage(bool flag) {
        m_coverage_ଧାଡ଼ି = m_coverageBranch = m_coverageToggle = m_coverage_ପଦସମଷ୍ଟି = m_coverageFsm
            = m_coverageUser = flag;
    }
    ସ୍ଥିର bool suffixed(ଅଚଳ string& sw, ଅଚଳ ଅକ୍ଷର* arg);
    ସ୍ଥିର string parseFileArg(ଅଚଳ string& optdir, ଅଚଳ string& relfilename);
    string filePathCheckOneDir(ଅଚଳ string& modname, ଅଚଳ string& dirname);
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା stripOptionsForChildRun(ଅଚଳ string& opt, bool for_ଶୀର୍ଷ);
    ଶୂନ୍ୟ validateIdentifier(File_ଧାଡ଼ି* fl, ଅଚଳ string& arg, ଅଚଳ string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

ସର୍ବସାଧାରଣ:
    V3Options();
    ~V3Options();
    ଶୂନ୍ୟ setDebugMode(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level);
    unsigned debugLevel(ଅଚଳ string& tag) ଅଚଳ VL_MT_SAFE;
    unsigned debugSrcLevel(ଅଚଳ string& srcfile_path) ଅଚଳ VL_MT_SAFE;
    unsigned dumpLevel(ଅଚଳ string& tag) ଅଚଳ VL_MT_SAFE;
    unsigned dumpSrcLevel(ଅଚଳ string& srcfile_path) ଅଚଳ VL_MT_SAFE;

    // METHODS
    ଶୂନ୍ୟ addCppFile(ଅଚଳ string& filename);
    ଶୂନ୍ୟ addCFlags(ଅଚଳ string& filename);
    ଶୂନ୍ୟ addCompilerIncludes(ଅଚଳ string& filename);
    ଶୂନ୍ୟ addLdLibFile(ଅଚଳ string& filename);
    ଶୂନ୍ୟ addLdLibs(ଅଚଳ string& filename);
    ଶୂନ୍ୟ addMakeFlags(ଅଚଳ string& filename);
    ଶୂନ୍ୟ addLibMapFile(ଅଚଳ string& filename);
    ଶୂନ୍ୟ addLibraryFile(ଅଚଳ string& filename, ଅଚଳ string& libname);
    ଶୂନ୍ୟ addVFile(ଅଚଳ string& filename, ଅଚଳ string& libname);
    ଶୂନ୍ୟ addVltFile(ଅଚଳ string& filename, ଅଚଳ string& libname);
    ଶୂନ୍ୟ addForceInc(ଅଚଳ string& filename);
    bool available() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_available; }
    ଶୂନ୍ୟ ccSet();
    ଶୂନ୍ୟ decorations(File_ଧାଡ଼ି* fl, ଅଚଳ string& filename);
    ଶୂନ୍ୟ notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ଅଚଳ { ଫେରନ୍ତୁ m_preprocOnly; }
    bool makePhony() ଅଚଳ { ଫେରନ୍ତୁ m_makePhony; }
    bool preproc_No_ଧାଡ଼ି() ଅଚଳ { ଫେରନ୍ତୁ m_preproc_No_ଧାଡ଼ି; }
    bool preprocResolve() ଅଚଳ { ଫେରନ୍ତୁ m_preprocResolve; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା preprocTokenLimit() ଅଚଳ { ଫେରନ୍ତୁ m_preprocTokenLimit; }
    bool underlineZero() ଅଚଳ { ଫେରନ୍ତୁ m_underlineZero; }
    bool systemC() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_systemC; }
    bool savable() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_savable; }
    VOptionBool schedZeroDelay() ଅଚଳ { ଫେରନ୍ତୁ m_schedZeroDelay; }
    bool stats() ଅଚଳ { ଫେରନ୍ତୁ m_stats; }
    bool statsVars() ଅଚଳ { ଫେରନ୍ତୁ m_statsVars; }
    bool stdPackage() ଅଚଳ { ଫେରନ୍ତୁ m_stdPackage; }
    bool stdWaiver() ଅଚଳ { ଫେରନ୍ତୁ m_stdWaiver; }
    bool assertOn() ଅଚଳ { ଫେରନ୍ତୁ m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ଅଚଳ { ଫେରନ୍ତୁ m_assertCase; }
    bool autoflush() ଅଚଳ { ଫେରନ୍ତୁ m_autoflush; }
    bool bboxSys() ଅଚଳ { ଫେରନ୍ତୁ m_bboxSys; }
    bool bboxUnsup() ଅଚଳ { ଫେରନ୍ତୁ m_bboxUnsup; }
    bool build() ଅଚଳ { ଫେରନ୍ତୁ m_build; }
    string buildDepBin() ଅଚଳ { ଫେରନ୍ତୁ m_buildDepBin; }
    ଶୂନ୍ୟ buildDepBin(ଅଚଳ string& flag) { m_buildDepBin = flag; }
    bool context() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_context; }
    bool coverage() ଅଚଳ VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        ଫେରନ୍ତୁ m_coverage_ଧାଡ଼ି || m_coverageBranch || m_coverageToggle || m_coverage_ପଦସମଷ୍ଟି
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ଅଚଳ VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        ଫେରନ୍ତୁ m_coverage_ଧାଡ଼ି || m_coverageBranch || m_coverageToggle || m_coverage_ପଦସମଷ୍ଟି
               || m_coverageUser;
    }
    bool coverageBranch() ଅଚଳ { ଫେରନ୍ତୁ m_coverageBranch; }
    bool coverage_ପଦସମଷ୍ଟି() ଅଚଳ { ଫେରନ୍ତୁ m_coverage_ପଦସମଷ୍ଟି; }
    bool coverageFsm() ଅଚଳ { ଫେରନ୍ତୁ m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ଅଚଳ { ଫେରନ୍ତୁ m_coverageFsmExpand; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା coverageFsmMaxArcs() ଅଚଳ { ଫେରନ୍ତୁ m_coverageFsmMaxArcs; }
    bool coverage_ଧାଡ଼ି() ଅଚଳ { ଫେରନ୍ତୁ m_coverage_ଧାଡ଼ି; }
    bool coverageMergeInstances() ଅଚଳ { ଫେରନ୍ତୁ m_coverageMergeInstances; }
    bool coveragePerInstance() ଅଚଳ { ଫେରନ୍ତୁ m_coveragePerInstance; }
    bool coverageToggle() ଅଚଳ { ଫେରନ୍ତୁ m_coverageToggle; }
    bool coverageUnderscore() ଅଚଳ { ଫେରନ୍ତୁ m_coverageUnderscore; }
    bool coverageUser() ଅଚଳ { ଫେରନ୍ତୁ m_coverageUser; }
    bool debugCheck() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_debugCheck; }
    bool debugCollision() ଅଚଳ { ଫେରନ୍ତୁ m_debugCollision; }
    bool debugEmitV() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_debugEmitV; }
    bool debugExitElab() ଅଚଳ { ଫେରନ୍ତୁ m_debugExitElab; }
    bool debugExitParse() ଅଚଳ { ଫେରନ୍ତୁ m_debugExitParse; }
    bool debugLeak() ଅଚଳ { ଫେରନ୍ତୁ m_debugLeak; }
    bool debugNondeterminism() ଅଚଳ { ଫେରନ୍ତୁ m_debugNondeterminism; }
    bool debugPartition() ଅଚଳ { ଫେରନ୍ତୁ m_debugPartition; }
    bool debugPreprocPassthru() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_debugPreprocPassthru; }
    bool debugProtect() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_debugProtect; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା debugRuntimeTimeout() ଅଚଳ { ଫେରନ୍ତୁ m_debugRuntimeTimeout; }
    bool debugSelfTest() ଅଚଳ { ଫେରନ୍ତୁ m_debugSelfTest; }
    bool debug_ଗଦା_Check() ଅଚଳ { ଫେରନ୍ତୁ m_debug_ଗଦା_Check; }
    bool debugWidth() ଅଚଳ VL_PURE { ଫେରନ୍ତୁ m_debugWidth; }
    bool decoration() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_decoration; }
    bool decorationNodes() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_decorationNodes; }
    bool diagnosticsSarif() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_diagnosticsSarif; }
    bool dpiHdrOnly() ଅଚଳ { ଫେରନ୍ତୁ m_dpiHdrOnly; }
    bool dumpAstPatterns() ଅଚଳ {
        ଫେରନ୍ତୁ m_dumpLevel.ଗଣନା_2("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() ଅଚଳ { ଫେରନ୍ତୁ m_dumpLevel.ଗଣନା_2("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ଅଚଳ {
        ଫେରନ୍ତୁ m_dumpLevel.ଗଣନା_2("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_ବୃକ୍ଷ_Dot() ଅଚଳ {
        ଫେରନ୍ତୁ m_dumpLevel.ଗଣନା_2("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ଅଚଳ { ଫେରନ୍ତୁ m_emitAccessors; }
    bool exe() ଅଚଳ { ଫେରନ୍ତୁ m_exe; }
    bool flatten() ଅଚଳ { ଫେରନ୍ତୁ m_flatten; }
    bool fourstate() ଅଚଳ { ଫେରନ୍ତୁ m_fourstate; }
    bool gmake() ଅଚଳ { ଫେରନ୍ତୁ m_gmake; }
    bool makeJson() ଅଚଳ { ଫେରନ୍ତୁ m_makeJson; }
    bool threadsDpiPure() ଅଚଳ { ଫେରନ୍ତୁ m_threadsDpiPure; }
    bool threadsDpiUnpure() ଅଚଳ { ଫେରନ୍ତୁ m_threadsDpiUnpure; }
    bool threadsCoarsen() ଅଚଳ { ଫେରନ୍ତୁ m_threadsCoarsen; }
    VOptionBool timing() ଅଚଳ { ଫେରନ୍ତୁ m_timing; }
    bool trace() ଅଚଳ { ଫେରନ୍ତୁ m_trace; }
    bool traceCoverage() ଅଚଳ { ଫେରନ୍ତୁ m_traceCoverage; }
    bool traceEnabledFst() ଅଚଳ { ଫେରନ୍ତୁ m_traceEnabledFst; }
    bool traceEnabledSaif() ଅଚଳ { ଫେରନ୍ତୁ m_traceEnabledSaif; }
    bool traceEnabledVcd() ଅଚଳ { ଫେରନ୍ତୁ m_traceEnabledVcd; }
    bool traceParams() ଅଚଳ { ଫେରନ୍ତୁ m_traceParams; }
    bool traceStructs() ଅଚଳ { ଫେରନ୍ତୁ m_traceStructs; }
    bool traceUnderscore() ଅଚଳ { ଫେରନ୍ତୁ m_traceUnderscore; }
    bool ମୁଖ୍ୟ() ଅଚଳ { ଫେରନ୍ତୁ m_main; }
    bool outFormatOk() ଅଚଳ { ଫେରନ୍ତୁ m_outFormatOk; }
    bool jsonOnly() ଅଚଳ { ଫେରନ୍ତୁ m_jsonOnly; }
    bool keepTempFiles() ଅଚଳ { ଫେରନ୍ତୁ (V3Error::debugDefault() != 0); }
    bool pedantic() ଅଚଳ { ଫେରନ୍ତୁ m_pedantic; }
    bool pinsInoutEnables() ଅଚଳ { ଫେରନ୍ତୁ m_pinsInoutEnables; }
    bool pinsScUint() ଅଚଳ { ଫେରନ୍ତୁ m_pinsScUint; }
    bool pinsScUintBool() ଅଚଳ { ଫେରନ୍ତୁ m_pinsScUintBool; }
    bool pinsScBigUint() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_pinsScBigUint; }
    bool pinsUint8() ଅଚଳ { ଫେରନ୍ତୁ m_pinsUint8; }
    bool preprocComments() ଅଚଳ { ଫେରନ୍ତୁ m_preprocComments; }
    bool preprocDefines() ଅଚଳ { ଫେରନ୍ତୁ m_preprocDefines; }
    bool profC() ଅଚଳ { ଫେରନ୍ତୁ m_profC; }
    bool profCFuncs() ଅଚଳ { ଫେରନ୍ତୁ m_profCFuncs; }
    bool profExec() ଅଚଳ { ଫେରନ୍ତୁ m_profExec; }
    bool profPgo() ଅଚଳ { ଫେରନ୍ତୁ m_profPgo; }
    bool usesProfiler() ଅଚଳ { ଫେରନ୍ତୁ profExec() || profPgo(); }
    bool protectIds() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_protectIds; }
    bool allPublic() ଅଚଳ { ଫେରନ୍ତୁ m_public; }
    bool publicParams() ଅଚଳ { ଫେରନ୍ତୁ m_publicParams; }
    bool publicOff() ଅଚଳ { ଫେରନ୍ତୁ m_publicIgnore; }
    bool publicFlatRW() ଅଚଳ { ଫେରନ୍ତୁ m_publicFlatRW; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା publicDepth() ଅଚଳ { ଫେରନ୍ତୁ m_publicDepth; }
    bool anyPublicFlat() ଅଚଳ { ଫେରନ୍ତୁ m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_lintOnly; }
    bool ignc() ଅଚଳ { ଫେରନ୍ତୁ m_ignc; }
    bool quietBuild() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_quietBuild; }
    bool quietExit() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_quietExit; }
    bool quietStats() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_quietStats; }
    bool reportUnoptflat() ଅଚଳ { ଫେରନ୍ତୁ m_reportUnoptflat; }
    bool verilate() ଅଚଳ { ଫେରନ୍ତୁ m_verilate; }
    bool vpi() ଅଚଳ { ଫେରନ୍ତୁ m_vpi.isTrue(); }
    bool waiverMultiline() ଅଚଳ { ଫେରନ୍ତୁ m_waiverMultiline; }
    bool xInitialEdge() ଅଚଳ { ଫେରନ୍ତୁ m_xInitialEdge; }
    bool serializeOnly() ଅଚଳ { ଫେରନ୍ତୁ m_jsonOnly; }
    bool ଶୀର୍ଷ_Ifaces_Supported() ଅଚଳ { ଫେରନ୍ତୁ lintOnly() && !hierarchical(); }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା buildJobs() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_buildJobs; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା convergeLimit() ଅଚଳ { ଫେରନ୍ତୁ m_convergeLimit; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା coverage_ପଦସମଷ୍ଟି_Max() ଅଚଳ { ଫେରନ୍ତୁ m_coverage_ପଦସମଷ୍ଟି_Max; }
    uint32_t coverageMaxBins() ଅଚଳ { ଫେରନ୍ତୁ m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ଅଚଳ { ଫେରନ୍ତୁ m_coverageMaxRealBins; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା coverageMaxWidth() ଅଚଳ { ଫେରନ୍ତୁ m_coverageMaxWidth; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା debugAllocRandom() ଅଚଳ { ଫେରନ୍ତୁ m_debugAllocRandom; }
    bool dump_ବୃକ୍ଷ_Addrids() ଅଚଳ VL_MT_SAFE;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା expandLimit() ଅଚଳ { ଫେରନ୍ତୁ m_expandLimit; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା gateStmts() ଅଚଳ { ଫେରନ୍ତୁ m_gateStmts; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ifDepth() ଅଚଳ { ଫେରନ୍ତୁ m_ifDepth; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା inlineCFuncs() ଅଚଳ { ଫେରନ୍ତୁ m_inlineCFuncs; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା inlineCFuncsProduct() ଅଚଳ { ଫେରନ୍ତୁ m_inlineCFuncsProduct; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା inlineMult() ଅଚଳ { ଫେରନ୍ତୁ m_inlineMult; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା_Dpi() ଅଚଳ { ଫେରନ୍ତୁ m_instr_ଗଣନା_Dpi; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା localizeMaxSize() ଅଚଳ { ଫେରନ୍ତୁ m_localizeMaxSize; }
    bool jsonEditNums() ଅଚଳ { ଫେରନ୍ତୁ m_jsonEditNums; }
    bool jsonIds() ଅଚଳ { ଫେରନ୍ତୁ m_jsonIds; }
    VOptionBool makeDepend() ଅଚଳ { ଫେରନ୍ତୁ m_makeDepend; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା max_ଅଙ୍କ_Width() ଅଚଳ { ଫେରନ୍ତୁ m_max_ଅଙ୍କ_Width; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା funcRecursionDepth() ଅଚଳ { ଫେରନ୍ତୁ m_funcRecursion; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା moduleRecursionDepth() ଅଚଳ { ଫେରନ୍ତୁ m_moduleRecursion; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା outputSplit() ଅଚଳ { ଫେରନ୍ତୁ m_outputSplit; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା outputSplitCFuncs() ଅଚଳ { ଫେରନ୍ତୁ m_outputSplitCFuncs; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା outputSplitCTrace() ଅଚଳ { ଫେରନ୍ତୁ m_outputSplitCTrace; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା outputGroups() ଅଚଳ { ଫେରନ୍ତୁ m_outputGroups; }
    int64_t outputGroupsSize() ଅଚଳ { ଫେରନ୍ତୁ m_outputGroupsSize; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା pinsBv() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_pinsBv; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା reloopLimit() ଅଚଳ { ଫେରନ୍ତୁ m_reloopLimit; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା replicationLimit() ଅଚଳ { ଫେରନ୍ତୁ m_replicationLimit; }
    VOptionBool skipIdentical() ଅଚଳ { ଫେରନ୍ତୁ m_skipIdentical; }
    bool stopFail() ଅଚଳ { ଫେରନ୍ତୁ m_stopFail; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା threads() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_threads; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା threadsMaxMTasks() ଅଚଳ { ଫେରନ୍ତୁ m_threadsMaxMTasks; }
    bool mtasks() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ (m_threads > 1); }
    VTimescale timeDefaultPrec() ଅଚଳ { ଫେରନ୍ତୁ m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() ଅଚଳ { ଫେରନ୍ତୁ m_timeDefaultUnit; }
    VTimescale timeOverridePrec() ଅଚଳ { ଫେରନ୍ତୁ m_timeOverridePrec; }
    VTimescale timeOverrideUnit() ଅଚଳ { ଫେରନ୍ତୁ m_timeOverrideUnit; }
    VTimescale timeComputePrec(ଅଚଳ VTimescale& flag) ଅଚଳ;
    VTimescale timeComputeUnit(ଅଚଳ VTimescale& flag) ଅଚଳ;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା traceDepth() ଅଚଳ { ଫେରନ୍ତୁ m_traceDepth; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା traceMaxArray() ଅଚଳ { ଫେରନ୍ତୁ m_traceMaxArray; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା traceMaxWidth() ଅଚଳ { ଫେରନ୍ତୁ m_traceMaxWidth; }
    bool useTraceParallel() ଅଚଳ {
        ଫେରନ୍ତୁ trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା unroll_ଗଣନା() ଅଚଳ { ଫେରନ୍ତୁ m_unroll_ଗଣନା; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା unrollLimit() ଅଚଳ { ଫେରନ୍ତୁ m_unrollLimit; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା unrollStmts() ଅଚଳ { ଫେରନ୍ତୁ m_unrollStmts; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା constraintArrayLimit() ଅଚଳ { ଫେରନ୍ତୁ m_constraintArrayLimit; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା verilateJobs() ଅଚଳ { ଫେରନ୍ତୁ m_verilateJobs; }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା compLimitBlocks() ଅଚଳ { ଫେରନ୍ତୁ m_compLimitBlocks; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା compLimitMembers() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_compLimitMembers; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା compLimitParens() ଅଚଳ { ଫେରନ୍ତୁ m_compLimitParens; }

    string diagnosticsSarifOutput() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ଅଚଳ { ଫେରନ୍ତୁ m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() ଅଚଳ { ଫେରନ୍ତୁ m_hierParamsFile; }
    string jsonOnlyOutput() ଅଚଳ { ଫେରନ୍ତୁ m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ଅଚଳ { ଫେରନ୍ତୁ m_jsonOnlyMetaOutput; }
    string l2Name() ଅଚଳ { ଫେରନ୍ତୁ m_l2Name; }
    string libCreate() ଅଚଳ { ଫେରନ୍ତୁ m_libCreate; }
    string libCreateName(bool shared) ଅଚଳ {
        string libName = "lib" + libCreate();
        ଯଦି (shared) {
            libName += ".so";
        } ଅନ୍ୟଥା {
            libName += ".a";
        }
        ଫେରନ୍ତୁ libName;
    }
    string main_ଶୀର୍ଷ_Name() ଅଚଳ { ଫେରନ୍ତୁ m_main_ଶୀର୍ଷ_Name; }
    string makeDir() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_makeDir; }
    string modPrefix() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_modPrefix; }
    string pipeFilter() ଅଚଳ { ଫେରନ୍ତୁ m_pipeFilter; }
    string prefix() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_ଚାବି_Provided() ଅଚଳ { ଫେରନ୍ତୁ !m_protect_ଚାବି.empty(); }
    string protect_ଚାବି_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string ଶୀର୍ଷ_Module() ଅଚଳ { ଫେରନ୍ତୁ m_ଶୀର୍ଷ_Module; }  // As AstNode::encodeName()
    bool no_Trace_ଶୀର୍ଷ() ଅଚଳ { ଫେରନ୍ତୁ m_no_Trace_ଶୀର୍ଷ; }
    string unusedRegexp() ଅଚଳ { ଫେରନ୍ତୁ m_unusedRegexp; }
    string waiverOutput() ଅଚଳ { ଫେରନ୍ତୁ m_waiverOutput; }
    string work() ଅଚଳ { ଫେରନ୍ତୁ m_work; }
    bool isWaiverOutput() ଅଚଳ { ଫେରନ୍ତୁ !m_waiverOutput.empty(); }
    string xAssign() ଅଚଳ { ଫେରନ୍ତୁ m_xAssign; }
    string xInitial() ଅଚଳ { ଫେରନ୍ତୁ m_xInitial; }

    ଅଚଳ VStringSet& cppFiles() ଅଚଳ { ଫେରନ୍ତୁ m_cppFiles; }
    ଅଚଳ VStringList& cFlags() ଅଚଳ { ଫେରନ୍ତୁ m_cFlags; }
    ଅଚଳ VStringSet& compilerIncludes() ଅଚଳ { ଫେରନ୍ତୁ m_compilerIncludes; }
    ଅଚଳ VStringList& ldLibFiles() ଅଚଳ { ଫେରନ୍ତୁ m_ldLibFiles; }
    ଅଚଳ VStringList& ldLibs() ଅଚଳ { ଫେରନ୍ତୁ m_ldLibs; }
    ଅଚଳ VStringList& makeFlags() ଅଚଳ { ଫେରନ୍ତୁ m_makeFlags; }
    ଅଚଳ VFileLibSet& libraryFiles() ଅଚଳ { ଫେରନ୍ତୁ m_libraryFiles; }
    ଅଚଳ VStringSet& libmapFiles() ଅଚଳ { ଫେରନ୍ତୁ m_libmapFiles; }
    ଅଚଳ VFileLibList& vFiles() ଅଚଳ { ଫେରନ୍ତୁ m_vFiles; }
    ଅଚଳ VFileLibSet& vltFiles() ଅଚଳ { ଫେରନ୍ତୁ m_vltFiles; }
    ଅଚଳ VStringList& forceIncs() ଅଚଳ { ଫେରନ୍ତୁ m_forceIncs; }

    bool hasParameter(ଅଚଳ string& name);
    string parameter(ଅଚଳ string& name);
    ଶୂନ୍ୟ checkParameters();

    bool isFuture(ଅଚଳ string& flag) ଅଚଳ;
    bool isFuture0(ଅଚଳ string& flag) ଅଚଳ;
    bool isFuture1(ଅଚଳ string& flag) ଅଚଳ;
    bool isLibraryFile(ଅଚଳ string& filename, ଅଚଳ string& libname) ଅଚଳ;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ଅଚଳ { ଫେରନ୍ତୁ m_fAcycSimp; }
    bool fAssemble() ଅଚଳ { ଫେରନ୍ତୁ m_fAssemble; }
    bool fBitScanLoops() ଅଚଳ { ଫେରନ୍ତୁ m_fBitScanLoops; }
    bool fBitblast() ଅଚଳ { ଫେରନ୍ତୁ m_fBitblast; }
    bool fCaseDecoder() ଅଚଳ { ଫେରନ୍ତୁ m_fCaseDecoder; }
    bool fCaseTable() ଅଚଳ { ଫେରନ୍ତୁ m_fCaseTable; }
    bool f_Case_ବୃକ୍ଷ() ଅଚଳ { ଫେରନ୍ତୁ m_f_Case_ବୃକ୍ଷ; }
    bool fCombine() ଅଚଳ { ଫେରନ୍ତୁ m_fCombine; }
    bool fConst() ଅଚଳ { ଫେରନ୍ତୁ m_fConst; }
    bool fConstBeforeDfg() ଅଚଳ { ଫେରନ୍ତୁ m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_ବୃକ୍ଷ() ଅଚଳ { ଫେରନ୍ତୁ m_f_Const_Bit_Op_ବୃକ୍ଷ; }
    bool fConstEager() ଅଚଳ { ଫେରନ୍ତୁ m_fConstEager; }
    bool fDedupe() ଅଚଳ { ଫେରନ୍ତୁ m_fDedupe; }
    bool fDfg() ଅଚଳ { ଫେରନ୍ତୁ m_fDfg; }
    bool fDfgPeephole() ଅଚଳ { ଫେରନ୍ତୁ m_fDfgPeephole; }
    bool f_Dfg_ଠେଲ_Down_Sels() ଅଚଳ { ଫେରନ୍ତୁ m_f_Dfg_ଠେଲ_Down_Sels; }
    bool fDfgSynthesizeAll() ଅଚଳ { ଫେରନ୍ତୁ m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ଅଚଳ std::string& name) ଅଚଳ {
        ଫେରନ୍ତୁ !m_fDfgPeepholeDisabled.ଗଣନା_2(name);
    }
    bool fDeadAssigns() ଅଚଳ { ଫେରନ୍ତୁ m_fDeadAssigns; }
    bool fDeadCells() ଅଚଳ { ଫେରନ୍ତୁ m_fDeadCells; }
    bool fDeadMethods() ଅଚଳ { ଫେରନ୍ତୁ m_fDeadMethods; }
    bool fDecompose() ଅଚଳ { ଫେରନ୍ତୁ m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ଅଚଳ { ଫେରନ୍ତୁ m_fDecomposePacked; }
    bool fDecomposeUnpacked() ଅଚଳ { ଫେରନ୍ତୁ m_fDecomposeUnpacked; }
    bool fExpand() ଅଚଳ { ଫେରନ୍ତୁ m_fExpand; }
    bool fFuncBalanceCat() ଅଚଳ { ଫେରନ୍ତୁ m_fFuncBalanceCat; }
    bool fFuncSplitCat() ଅଚଳ { ଫେରନ୍ତୁ m_fFuncSplitCat; }
    bool fFunc() ଅଚଳ { ଫେରନ୍ତୁ fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() ଅଚଳ { ଫେରନ୍ତୁ m_fGate; }
    VOptionBool fIcoChangeDetect() ଅଚଳ { ଫେରନ୍ତୁ m_fIcoChangeDetect; }
    bool fInline() ଅଚଳ { ଫେରନ୍ତୁ m_fInline; }
    bool fInlineCFuncs() ଅଚଳ { ଫେରନ୍ତୁ m_fInlineCFuncs; }
    bool fInlineFuncs() ଅଚଳ { ଫେରନ୍ତୁ m_fInlineFuncs; }
    bool fInlineFuncsEager() ଅଚଳ { ଫେରନ୍ତୁ m_fInlineFuncsEager; }
    bool fLife() ଅଚଳ { ଫେରନ୍ତୁ m_fLife; }
    bool fLifePost() ଅଚଳ { ଫେରନ୍ତୁ m_fLifePost; }
    bool fLocalize() ଅଚଳ { ଫେରନ୍ତୁ m_fLocalize; }
    bool fMergeCond() ଅଚଳ { ଫେରନ୍ତୁ m_fMergeCond; }
    bool fMergeCondMotion() ଅଚଳ { ଫେରନ୍ତୁ m_fMergeCondMotion; }
    bool fReloop() ଅଚଳ { ଫେରନ୍ତୁ m_fReloop; }
    bool fReorder() ଅଚଳ { ଫେରନ୍ତୁ m_fReorder; }
    bool fSlice() ଅଚଳ { ଫେରନ୍ତୁ m_fSlice; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା fSliceElementLimit() ଅଚଳ { ଫେରନ୍ତୁ m_fSliceElementLimit; }
    bool fSplit() ଅଚଳ { ଫେରନ୍ତୁ m_fSplit; }
    bool fSubst() ଅଚଳ { ଫେରନ୍ତୁ m_fSubst; }
    bool fSubstConst() ଅଚଳ { ଫେରନ୍ତୁ m_fSubstConst; }
    bool fTable() ଅଚଳ { ଫେରନ୍ତୁ m_fTable; }
    bool fTaskifyAll() ଅଚଳ { ଫେରନ୍ତୁ m_fTaskifyAll; }

    std::string traceClassBase() ଅଚଳ VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() ଅଚଳ VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() ଅଚଳ VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() ଅଚଳ VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() ଅଚଳ VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() ଅଚଳ VL_MT_SAFE;

    bool hierarchical() ଅଚଳ { ଫେରନ୍ତୁ m_hierarchical; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା hierChild() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_hierChild; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା hierThreads() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_ଶୀର୍ଷ() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ !m_hierChild && !m_hierBlocks.empty(); }
    ଅଚଳ V3HierBlockOptSet& hierBlocks() ଅଚଳ { ଫେରନ୍ତୁ m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_ଶୀର୍ଷ_Data_Dir() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ hier_ଶୀର୍ଷ() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ସ୍ଥିର string version() VL_PURE;
    ସ୍ଥିର string argString(ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv);  ///< Return list of arguments as simple string
    string allArgsString() ଅଚଳ VL_MT_SAFE;  ///< Return all passed arguments as simple string
    ଅଚଳ std::list<std::pair<std::list<std::string>, bool>>& allArgs() ଅଚଳ;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_ଶୀର୍ଷ) ଅଚଳ;
    ଶୂନ୍ୟ parseOpts(File_ଧାଡ଼ି* fl, ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv) VL_MT_DISABLED;
    ଶୂନ୍ୟ parseOptsList(File_ଧାଡ଼ି* fl, ଅଚଳ string& optdir, ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv) VL_MT_DISABLED;
    ଶୂନ୍ୟ parseOptsFile(File_ଧାଡ଼ି* fl, ଅଚଳ string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    ସ୍ଥିର string getenvBuiltins(ଅଚଳ string& var);
    ସ୍ଥିର string getenvMAKE();
    ସ୍ଥିର string getenvMAKEFLAGS();
    ସ୍ଥିର string getenvPERL();
    ସ୍ଥିର string getenvPYTHON3();
    ସ୍ଥିର string getenvSYSTEMC();
    ସ୍ଥିର string getenvSYSTEMC_ARCH();
    ସ୍ଥିର string getenvSYSTEMC_INCLUDE();
    ସ୍ଥିର string getenvSYSTEMC_LIBDIR();
    ସ୍ଥିର string getenv_VERILATOR_ମୂଳ();
    ସ୍ଥିର string getenvVERILATOR_SOLVER();
    ସ୍ଥିର string getStdPackagePath();
    ସ୍ଥିର string getStdWaiverPath();
    ସ୍ଥିର string getSupported(ଅଚଳ string& var);
    ସ୍ଥିର bool systemCSystemWide();
    ସ୍ଥିର bool systemCFound();  // SystemC installed, or environment points to it
    ସ୍ଥିର bool coroutineSupport();  // Configured C++ compiler supports coroutines
    ସ୍ଥିର bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    ସ୍ଥିର bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    ସ୍ଥିର bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ଅଚଳ string& filename);
    string filePath(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname, ଅଚଳ string& lastpath,
                    ଅଚଳ string& errmsg);
    string file_Path_Looked_ସନ୍ଦେଶ(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname);
    V3LangCode fileLanguage(ଅଚଳ string& filename);
    ସ୍ଥିର bool fileStatNormal(ଅଚଳ string& filename);

    // METHODS (other OS)
    ସ୍ଥିର ଶୂନ୍ୟ throwSigsegv();
};

//######################################################################

#endif  // guard
