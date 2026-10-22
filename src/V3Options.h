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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3LangCode.h"

#അന്തര്ഭാവ <list>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <vector>

class V3OptionsImp;
class File_വരി;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
പൊതു:
    എണ്ണൽ en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    എണ്ണൽ en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        തിരികെ names[m_e];
    }
    bool isDefault() സ്ഥിരം { തിരികെ m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() സ്ഥിരം { തിരികെ m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() സ്ഥിരം { തിരികെ m_e == OPT_TRUE; }
    bool isSetFalse() സ്ഥിരം { തിരികെ m_e == OPT_FALSE; }
    ശൂന്യം setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(സ്ഥിരം VOptionBool& lhs, സ്ഥിരം VOptionBool& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VOptionBool& lhs, VOptionBool::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, സ്ഥിരം VOptionBool& rhs) { തിരികെ lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    സ്ഥിരം string m_filename;  // Filename
    സ്ഥിരം string m_libname;  // Libname
പൊതു:
    VFileLibName(സ്ഥിരം string& filename, സ്ഥിരം string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(സ്ഥിരം VFileLibName& rhs) = സ്ഥിരസ്ഥിതി;
    string filename() സ്ഥിരം { തിരികെ m_filename; }
    string libname() സ്ഥിരം { തിരികെ m_libname; }
    bool operator==(സ്ഥിരം VFileLibName& rhs) സ്ഥിരം {
        തിരികെ m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(സ്ഥിരം VFileLibName& rhs) സ്ഥിരം {
        എങ്കിൽ (m_filename < rhs.m_filename) തിരികെ true;
        എങ്കിൽ (m_filename > rhs.m_filename) തിരികെ false;
        തിരികെ m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::vector<VFileLibName>;
using VFileLibSet = std::set<VFileLibName>;

// ######################################################################

class VTimescale final {
പൊതു:
    എണ്ണൽ en : uint8_t {
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
    എണ്ണൽ : uint8_t { TS_DEFAULT = TS_1PS };
    എണ്ണൽ en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(സ്ഥിരം string& മൂല്യം, bool& badr);
    VTimescale(ഇരട്ട മൂല്യം, bool& badr) {
        badr = false;
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            എങ്കിൽ (multiplier() == മൂല്യം) നിർത്തുക;
        }
        എങ്കിൽ (multiplier() != മൂല്യം) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() സ്ഥിരം { തിരികെ m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    സ്ഥിതം ശൂന്യം parseSlashed(File_വരി* fl, സ്ഥിരം അക്ഷരം* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        സ്ഥിതം സ്ഥിരം അക്ഷരം* സ്ഥിരം names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        തിരികെ names[m_e];
    }
    പൂർണ്ണസംഖ്യ powerOfTen() സ്ഥിരം { തിരികെ 2 - static_cast<പൂർണ്ണസംഖ്യ>(m_e); }
    ഇരട്ട multiplier() സ്ഥിരം {
        സ്ഥിതം സ്ഥിരം ഇരട്ട മൂല്യങ്ങൾ[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        തിരികെ മൂല്യങ്ങൾ[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VTimescale& lhs, സ്ഥിരം VTimescale& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VTimescale& lhs, VTimescale::en rhs) { തിരികെ lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, സ്ഥിരം VTimescale& rhs) { തിരികെ lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(സ്ഥിരം VTimescale& lhs, സ്ഥിരം VTimescale& rhs) {
    തിരികെ lhs.m_e > rhs.m_e;
}
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VTimescale& rhs) {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
പൊതു:
    എണ്ണൽ en : uint8_t {
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
    explicit VFsmExpandType(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    constexpr സ്ഥിരം അക്ഷരം* ascii() സ്ഥിരം {
        constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        തിരികെ names[m_e];
    }
};
constexpr bool operator==(സ്ഥിരം VFsmExpandType& lhs, സ്ഥിരം VFsmExpandType& rhs) VL_MT_SAFE {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, സ്ഥിരം VFsmExpandType& rhs) VL_MT_SAFE {
    തിരികെ lhs == rhs.m_e;
}
വരിയിൽ std::ostream& operator<<(std::ostream& os, സ്ഥിരം VFsmExpandType& rhs) VL_MT_SAFE {
    തിരികെ os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
പൊതു:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<സ്ഥിരം std::string, std::string>;

സ്വകാര്യ:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

പൊതു:
    explicit V3HierarchicalBlockOption(സ്ഥിരം string& optstring);
    സ്ഥിരം string& origName() സ്ഥിരം { തിരികെ m_origName; }
    സ്ഥിരം string& mangledName() സ്ഥിരം { തിരികെ m_mangledName; }
    സ്ഥിരം ParamStrMap params() സ്ഥിരം { തിരികെ m_parameters; }
};

using V3HierBlockOptSet = std::map<സ്ഥിരം std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
പൊതു:
സ്വകാര്യ:
    // TYPES
    using DebugLevelMap = std::map<സ്ഥിരം std::string, unsigned>;

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
    std::map<സ്ഥിരം string, string> m_parameters;  // Parameters
    std::map<സ്ഥിരം string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
    VStringSet m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_വരി = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_ദ്വിമാന = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_വ്യഞ്ജകം = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_വരി = false;    // main switch: --coverage-block
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
    bool m_debug_അട്ടി_Check = false;  // main switch: --debug-stack-check
    പൂർണ്ണസംഖ്യ m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
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
    bool m_no_Trace_മുകൾ = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_xInitialEdge = false;    // main switch: --x-initial-edge

    പൂർണ്ണസംഖ്യ         m_buildJobs = -1;    // main switch: --build-jobs, -j
    പൂർണ്ണസംഖ്യ         m_coverage_വ്യഞ്ജകം_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    പൂർണ്ണസംഖ്യ         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    പൂർണ്ണസംഖ്യ         m_convergeLimit = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    പൂർണ്ണസംഖ്യ         m_coverageMaxWidth = 256; // main switch: --coverage-max-width
    പൂർണ്ണസംഖ്യ         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    പൂർണ്ണസംഖ്യ         m_expandLimit = 256;  // main switch: --expand-limit
    പൂർണ്ണസംഖ്യ         m_gateStmts = 100;    // main switch: --gate-stmts
    പൂർണ്ണസംഖ്യ         m_hierChild = 0;      // main switch: --hierarchical-child
    പൂർണ്ണസംഖ്യ         m_hierThreads = 0;      // main switch: --hierarchical-threads
    പൂർണ്ണസംഖ്യ         m_ifDepth = 0;      // main switch: --if-depth
    പൂർണ്ണസംഖ്യ         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    പൂർണ്ണസംഖ്യ         m_inlineCFuncsProduct = 200;   // main switch: --inline-cfuncs-product
    പൂർണ്ണസംഖ്യ         m_inlineMult = 2000;   // main switch: --inline-mult
    പൂർണ്ണസംഖ്യ         m_instr_എണ്ണം_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    പൂർണ്ണസംഖ്യ         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    പൂർണ്ണസംഖ്യ         m_max_അക്കം_Width = 65536;  // main switch: --max-num-width
    പൂർണ്ണസംഖ്യ         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    പൂർണ്ണസംഖ്യ         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    പൂർണ്ണസംഖ്യ         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_outputGroupsSize = 512 * 1024 * 1024;  // main switch: --output-groups-size
    പൂർണ്ണസംഖ്യ         m_outputSplit = 20000;  // main switch: --output-split
    പൂർണ്ണസംഖ്യ         m_outputSplitCFuncs = -1;  // main switch: --output-split-cfuncs
    പൂർണ്ണസംഖ്യ         m_outputSplitCTrace = -1;  // main switch: --output-split-ctrace
    പൂർണ്ണസംഖ്യ         m_pinsBv = 65;       // main switch: --pins-bv
    പൂർണ്ണസംഖ്യ         m_preprocTokenLimit = 40000; // main switch: --preproc-token-limit
    പൂർണ്ണസംഖ്യ         m_publicDepth = 0;   // main switch: --public-depth
    പൂർണ്ണസംഖ്യ         m_reloopLimit = 40; // main switch: --reloop-limit
    പൂർണ്ണസംഖ്യ         m_replicationLimit = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    പൂർണ്ണസംഖ്യ         m_threads = 1;      // main switch: --threads
    പൂർണ്ണസംഖ്യ         m_threadsMaxMTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_timeDefaultPrec;  // main switch: --timescale
    VTimescale  m_timeDefaultUnit;  // main switch: --timescale
    VTimescale  m_timeOverridePrec;  // main switch: --timescale-override
    VTimescale  m_timeOverrideUnit;  // main switch: --timescale-override
    പൂർണ്ണസംഖ്യ         m_traceDepth = 0;   // main switch: --trace-depth
    പൂർണ്ണസംഖ്യ         m_traceMaxArray = 32;  // main switch: --trace-max-array
    പൂർണ്ണസംഖ്യ         m_traceMaxWidth = 4096; // main switch: --trace-max-width
    പൂർണ്ണസംഖ്യ         m_unroll_എണ്ണം = 64;  // main switch: --unroll-count
    പൂർണ്ണസംഖ്യ         m_unrollLimit = 16384;  // main switch: --unroll-limit
    പൂർണ്ണസംഖ്യ         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    പൂർണ്ണസംഖ്യ         m_constraintArrayLimit = 64;  // main switch: --constraint-array-limit
    പൂർണ്ണസംഖ്യ         m_verilateJobs = -1;  // main switch: --verilate-jobs

    പൂർണ്ണസംഖ്യ         m_compLimitBlocks = 0;  // compiler selection; number of nested blocks
    പൂർണ്ണസംഖ്യ         m_compLimitMembers = 64;  // compiler selection; number of members in struct before make anon array
    പൂർണ്ണസംഖ്യ         m_compLimitParens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_മുകൾ_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_modPrefix;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_prefix;       // main switch: --prefix
    string      m_protect_താക്കോൽ;   // main switch: --protect-key
    string      m_മുകൾ_Module;    // main switch: --top-module
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
    bool m_f_Case_വൃക്ഷം;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_Bit_Op_വൃക്ഷം;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_തള്ളുക_Down_Sels = true; // main switch: -fno-dfg-push-down-sels
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
    പൂർണ്ണസംഖ്യ  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    ശൂന്യം addArg(അക്ഷരം** argv, size_t എണ്ണം, bool isForRerun);
    ശൂന്യം addArg(സ്ഥിരം std::string& arg, bool isForRerun);
    ശൂന്യം add_വരി_Arg(സ്ഥിരം string& arg);
    ശൂന്യം addDefine(സ്ഥിരം string& defline, bool allowPlus) VL_MT_DISABLED;
    ശൂന്യം addFuture(സ്ഥിരം string& flag);
    ശൂന്യം addFuture0(സ്ഥിരം string& flag);
    ശൂന്യം addFuture1(സ്ഥിരം string& flag);
    ശൂന്യം addIncDirUser(സ്ഥിരം string& incdir);  // User requested
    ശൂന്യം addIncDirFallback(സ്ഥിരം string& incdir);  // Low priority if not found otherwise
    ശൂന്യം addParameter(സ്ഥിരം string& paramline, bool allowPlus);
    ശൂന്യം addLangExt(സ്ഥിരം string& langext, സ്ഥിരം V3LangCode& lc);
    ശൂന്യം addLibExtV(സ്ഥിരം string& libext);
    ശൂന്യം optimize(പൂർണ്ണസംഖ്യ level);
    ശൂന്യം showVersion(bool verbose);
    ശൂന്യം coverage(bool flag) {
        m_coverage_വരി = m_coverageBranch = m_coverageToggle = m_coverage_വ്യഞ്ജകം = m_coverageFsm
            = m_coverageUser = flag;
    }
    സ്ഥിതം bool suffixed(സ്ഥിരം string& sw, സ്ഥിരം അക്ഷരം* arg);
    സ്ഥിതം string parseFileArg(സ്ഥിരം string& optdir, സ്ഥിരം string& relfilename);
    string filePathCheckOneDir(സ്ഥിരം string& modname, സ്ഥിരം string& dirname);
    സ്ഥിതം പൂർണ്ണസംഖ്യ stripOptionsForChildRun(സ്ഥിരം string& opt, bool for_മുകൾ);
    ശൂന്യം validateIdentifier(File_വരി* fl, സ്ഥിരം string& arg, സ്ഥിരം string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

പൊതു:
    V3Options();
    ~V3Options();
    ശൂന്യം setDebugMode(പൂർണ്ണസംഖ്യ level);
    unsigned debugLevel(സ്ഥിരം string& tag) സ്ഥിരം VL_MT_SAFE;
    unsigned debugSrcLevel(സ്ഥിരം string& srcfile_path) സ്ഥിരം VL_MT_SAFE;
    unsigned dumpLevel(സ്ഥിരം string& tag) സ്ഥിരം VL_MT_SAFE;
    unsigned dumpSrcLevel(സ്ഥിരം string& srcfile_path) സ്ഥിരം VL_MT_SAFE;

    // METHODS
    ശൂന്യം addCppFile(സ്ഥിരം string& filename);
    ശൂന്യം addCFlags(സ്ഥിരം string& filename);
    ശൂന്യം addCompilerIncludes(സ്ഥിരം string& filename);
    ശൂന്യം addLdLibFile(സ്ഥിരം string& filename);
    ശൂന്യം addLdLibs(സ്ഥിരം string& filename);
    ശൂന്യം addMakeFlags(സ്ഥിരം string& filename);
    ശൂന്യം addLibMapFile(സ്ഥിരം string& filename);
    ശൂന്യം addLibraryFile(സ്ഥിരം string& filename, സ്ഥിരം string& libname);
    ശൂന്യം addVFile(സ്ഥിരം string& filename, സ്ഥിരം string& libname);
    ശൂന്യം addVltFile(സ്ഥിരം string& filename, സ്ഥിരം string& libname);
    ശൂന്യം addForceInc(സ്ഥിരം string& filename);
    bool available() സ്ഥിരം VL_MT_SAFE { തിരികെ m_available; }
    ശൂന്യം ccSet();
    ശൂന്യം decorations(File_വരി* fl, സ്ഥിരം string& filename);
    ശൂന്യം notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() സ്ഥിരം { തിരികെ m_preprocOnly; }
    bool makePhony() സ്ഥിരം { തിരികെ m_makePhony; }
    bool preproc_No_വരി() സ്ഥിരം { തിരികെ m_preproc_No_വരി; }
    bool preprocResolve() സ്ഥിരം { തിരികെ m_preprocResolve; }
    പൂർണ്ണസംഖ്യ preprocTokenLimit() സ്ഥിരം { തിരികെ m_preprocTokenLimit; }
    bool underlineZero() സ്ഥിരം { തിരികെ m_underlineZero; }
    bool systemC() സ്ഥിരം VL_MT_SAFE { തിരികെ m_systemC; }
    bool savable() സ്ഥിരം VL_MT_SAFE { തിരികെ m_savable; }
    VOptionBool schedZeroDelay() സ്ഥിരം { തിരികെ m_schedZeroDelay; }
    bool stats() സ്ഥിരം { തിരികെ m_stats; }
    bool statsVars() സ്ഥിരം { തിരികെ m_statsVars; }
    bool stdPackage() സ്ഥിരം { തിരികെ m_stdPackage; }
    bool stdWaiver() സ്ഥിരം { തിരികെ m_stdWaiver; }
    bool assertOn() സ്ഥിരം { തിരികെ m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() സ്ഥിരം { തിരികെ m_assertCase; }
    bool autoflush() സ്ഥിരം { തിരികെ m_autoflush; }
    bool bboxSys() സ്ഥിരം { തിരികെ m_bboxSys; }
    bool bboxUnsup() സ്ഥിരം { തിരികെ m_bboxUnsup; }
    bool build() സ്ഥിരം { തിരികെ m_build; }
    string buildDepBin() സ്ഥിരം { തിരികെ m_buildDepBin; }
    ശൂന്യം buildDepBin(സ്ഥിരം string& flag) { m_buildDepBin = flag; }
    bool context() സ്ഥിരം VL_MT_SAFE { തിരികെ m_context; }
    bool coverage() സ്ഥിരം VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        തിരികെ m_coverage_വരി || m_coverageBranch || m_coverageToggle || m_coverage_വ്യഞ്ജകം
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() സ്ഥിരം VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        തിരികെ m_coverage_വരി || m_coverageBranch || m_coverageToggle || m_coverage_വ്യഞ്ജകം
               || m_coverageUser;
    }
    bool coverageBranch() സ്ഥിരം { തിരികെ m_coverageBranch; }
    bool coverage_വ്യഞ്ജകം() സ്ഥിരം { തിരികെ m_coverage_വ്യഞ്ജകം; }
    bool coverageFsm() സ്ഥിരം { തിരികെ m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() സ്ഥിരം { തിരികെ m_coverageFsmExpand; }
    പൂർണ്ണസംഖ്യ coverageFsmMaxArcs() സ്ഥിരം { തിരികെ m_coverageFsmMaxArcs; }
    bool coverage_വരി() സ്ഥിരം { തിരികെ m_coverage_വരി; }
    bool coverageMergeInstances() സ്ഥിരം { തിരികെ m_coverageMergeInstances; }
    bool coveragePerInstance() സ്ഥിരം { തിരികെ m_coveragePerInstance; }
    bool coverageToggle() സ്ഥിരം { തിരികെ m_coverageToggle; }
    bool coverageUnderscore() സ്ഥിരം { തിരികെ m_coverageUnderscore; }
    bool coverageUser() സ്ഥിരം { തിരികെ m_coverageUser; }
    bool debugCheck() സ്ഥിരം VL_MT_SAFE { തിരികെ m_debugCheck; }
    bool debugCollision() സ്ഥിരം { തിരികെ m_debugCollision; }
    bool debugEmitV() സ്ഥിരം VL_MT_SAFE { തിരികെ m_debugEmitV; }
    bool debugExitElab() സ്ഥിരം { തിരികെ m_debugExitElab; }
    bool debugExitParse() സ്ഥിരം { തിരികെ m_debugExitParse; }
    bool debugLeak() സ്ഥിരം { തിരികെ m_debugLeak; }
    bool debugNondeterminism() സ്ഥിരം { തിരികെ m_debugNondeterminism; }
    bool debugPartition() സ്ഥിരം { തിരികെ m_debugPartition; }
    bool debugPreprocPassthru() സ്ഥിരം VL_MT_SAFE { തിരികെ m_debugPreprocPassthru; }
    bool debugProtect() സ്ഥിരം VL_MT_SAFE { തിരികെ m_debugProtect; }
    പൂർണ്ണസംഖ്യ debugRuntimeTimeout() സ്ഥിരം { തിരികെ m_debugRuntimeTimeout; }
    bool debugSelfTest() സ്ഥിരം { തിരികെ m_debugSelfTest; }
    bool debug_അട്ടി_Check() സ്ഥിരം { തിരികെ m_debug_അട്ടി_Check; }
    bool debugWidth() സ്ഥിരം VL_PURE { തിരികെ m_debugWidth; }
    bool decoration() സ്ഥിരം VL_MT_SAFE { തിരികെ m_decoration; }
    bool decorationNodes() സ്ഥിരം VL_MT_SAFE { തിരികെ m_decorationNodes; }
    bool diagnosticsSarif() സ്ഥിരം VL_MT_SAFE { തിരികെ m_diagnosticsSarif; }
    bool dpiHdrOnly() സ്ഥിരം { തിരികെ m_dpiHdrOnly; }
    bool dumpAstPatterns() സ്ഥിരം {
        തിരികെ m_dumpLevel.എണ്ണം("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() സ്ഥിരം { തിരികെ m_dumpLevel.എണ്ണം("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() സ്ഥിരം {
        തിരികെ m_dumpLevel.എണ്ണം("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_വൃക്ഷം_Dot() സ്ഥിരം {
        തിരികെ m_dumpLevel.എണ്ണം("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() സ്ഥിരം { തിരികെ m_emitAccessors; }
    bool exe() സ്ഥിരം { തിരികെ m_exe; }
    bool flatten() സ്ഥിരം { തിരികെ m_flatten; }
    bool fourstate() സ്ഥിരം { തിരികെ m_fourstate; }
    bool gmake() സ്ഥിരം { തിരികെ m_gmake; }
    bool makeJson() സ്ഥിരം { തിരികെ m_makeJson; }
    bool threadsDpiPure() സ്ഥിരം { തിരികെ m_threadsDpiPure; }
    bool threadsDpiUnpure() സ്ഥിരം { തിരികെ m_threadsDpiUnpure; }
    bool threadsCoarsen() സ്ഥിരം { തിരികെ m_threadsCoarsen; }
    VOptionBool timing() സ്ഥിരം { തിരികെ m_timing; }
    bool trace() സ്ഥിരം { തിരികെ m_trace; }
    bool traceCoverage() സ്ഥിരം { തിരികെ m_traceCoverage; }
    bool traceEnabledFst() സ്ഥിരം { തിരികെ m_traceEnabledFst; }
    bool traceEnabledSaif() സ്ഥിരം { തിരികെ m_traceEnabledSaif; }
    bool traceEnabledVcd() സ്ഥിരം { തിരികെ m_traceEnabledVcd; }
    bool traceParams() സ്ഥിരം { തിരികെ m_traceParams; }
    bool traceStructs() സ്ഥിരം { തിരികെ m_traceStructs; }
    bool traceUnderscore() സ്ഥിരം { തിരികെ m_traceUnderscore; }
    bool പ്രധാന() സ്ഥിരം { തിരികെ m_main; }
    bool outFormatOk() സ്ഥിരം { തിരികെ m_outFormatOk; }
    bool jsonOnly() സ്ഥിരം { തിരികെ m_jsonOnly; }
    bool keepTempFiles() സ്ഥിരം { തിരികെ (V3Error::debugDefault() != 0); }
    bool pedantic() സ്ഥിരം { തിരികെ m_pedantic; }
    bool pinsInoutEnables() സ്ഥിരം { തിരികെ m_pinsInoutEnables; }
    bool pinsScUint() സ്ഥിരം { തിരികെ m_pinsScUint; }
    bool pinsScUintBool() സ്ഥിരം { തിരികെ m_pinsScUintBool; }
    bool pinsScBigUint() സ്ഥിരം VL_MT_SAFE { തിരികെ m_pinsScBigUint; }
    bool pinsUint8() സ്ഥിരം { തിരികെ m_pinsUint8; }
    bool preprocComments() സ്ഥിരം { തിരികെ m_preprocComments; }
    bool preprocDefines() സ്ഥിരം { തിരികെ m_preprocDefines; }
    bool profC() സ്ഥിരം { തിരികെ m_profC; }
    bool profCFuncs() സ്ഥിരം { തിരികെ m_profCFuncs; }
    bool profExec() സ്ഥിരം { തിരികെ m_profExec; }
    bool profPgo() സ്ഥിരം { തിരികെ m_profPgo; }
    bool usesProfiler() സ്ഥിരം { തിരികെ profExec() || profPgo(); }
    bool protectIds() സ്ഥിരം VL_MT_SAFE { തിരികെ m_protectIds; }
    bool allPublic() സ്ഥിരം { തിരികെ m_public; }
    bool publicParams() സ്ഥിരം { തിരികെ m_publicParams; }
    bool publicOff() സ്ഥിരം { തിരികെ m_publicIgnore; }
    bool publicFlatRW() സ്ഥിരം { തിരികെ m_publicFlatRW; }
    പൂർണ്ണസംഖ്യ publicDepth() സ്ഥിരം { തിരികെ m_publicDepth; }
    bool anyPublicFlat() സ്ഥിരം { തിരികെ m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() സ്ഥിരം VL_MT_SAFE { തിരികെ m_lintOnly; }
    bool ignc() സ്ഥിരം { തിരികെ m_ignc; }
    bool quietBuild() സ്ഥിരം VL_MT_SAFE { തിരികെ m_quietBuild; }
    bool quietExit() സ്ഥിരം VL_MT_SAFE { തിരികെ m_quietExit; }
    bool quietStats() സ്ഥിരം VL_MT_SAFE { തിരികെ m_quietStats; }
    bool reportUnoptflat() സ്ഥിരം { തിരികെ m_reportUnoptflat; }
    bool verilate() സ്ഥിരം { തിരികെ m_verilate; }
    bool vpi() സ്ഥിരം { തിരികെ m_vpi.isTrue(); }
    bool waiverMultiline() സ്ഥിരം { തിരികെ m_waiverMultiline; }
    bool xInitialEdge() സ്ഥിരം { തിരികെ m_xInitialEdge; }
    bool serializeOnly() സ്ഥിരം { തിരികെ m_jsonOnly; }
    bool മുകൾ_Ifaces_Supported() സ്ഥിരം { തിരികെ lintOnly() && !hierarchical(); }

    പൂർണ്ണസംഖ്യ buildJobs() സ്ഥിരം VL_MT_SAFE { തിരികെ m_buildJobs; }
    പൂർണ്ണസംഖ്യ convergeLimit() സ്ഥിരം { തിരികെ m_convergeLimit; }
    പൂർണ്ണസംഖ്യ coverage_വ്യഞ്ജകം_Max() സ്ഥിരം { തിരികെ m_coverage_വ്യഞ്ജകം_Max; }
    uint32_t coverageMaxBins() സ്ഥിരം { തിരികെ m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() സ്ഥിരം { തിരികെ m_coverageMaxRealBins; }
    പൂർണ്ണസംഖ്യ coverageMaxWidth() സ്ഥിരം { തിരികെ m_coverageMaxWidth; }
    പൂർണ്ണസംഖ്യ debugAllocRandom() സ്ഥിരം { തിരികെ m_debugAllocRandom; }
    bool dump_വൃക്ഷം_Addrids() സ്ഥിരം VL_MT_SAFE;
    പൂർണ്ണസംഖ്യ expandLimit() സ്ഥിരം { തിരികെ m_expandLimit; }
    പൂർണ്ണസംഖ്യ gateStmts() സ്ഥിരം { തിരികെ m_gateStmts; }
    പൂർണ്ണസംഖ്യ ifDepth() സ്ഥിരം { തിരികെ m_ifDepth; }
    പൂർണ്ണസംഖ്യ inlineCFuncs() സ്ഥിരം { തിരികെ m_inlineCFuncs; }
    പൂർണ്ണസംഖ്യ inlineCFuncsProduct() സ്ഥിരം { തിരികെ m_inlineCFuncsProduct; }
    പൂർണ്ണസംഖ്യ inlineMult() സ്ഥിരം { തിരികെ m_inlineMult; }
    പൂർണ്ണസംഖ്യ instr_എണ്ണം_Dpi() സ്ഥിരം { തിരികെ m_instr_എണ്ണം_Dpi; }
    പൂർണ്ണസംഖ്യ localizeMaxSize() സ്ഥിരം { തിരികെ m_localizeMaxSize; }
    bool jsonEditNums() സ്ഥിരം { തിരികെ m_jsonEditNums; }
    bool jsonIds() സ്ഥിരം { തിരികെ m_jsonIds; }
    VOptionBool makeDepend() സ്ഥിരം { തിരികെ m_makeDepend; }
    പൂർണ്ണസംഖ്യ max_അക്കം_Width() സ്ഥിരം { തിരികെ m_max_അക്കം_Width; }
    പൂർണ്ണസംഖ്യ funcRecursionDepth() സ്ഥിരം { തിരികെ m_funcRecursion; }
    പൂർണ്ണസംഖ്യ moduleRecursionDepth() സ്ഥിരം { തിരികെ m_moduleRecursion; }
    പൂർണ്ണസംഖ്യ outputSplit() സ്ഥിരം { തിരികെ m_outputSplit; }
    പൂർണ്ണസംഖ്യ outputSplitCFuncs() സ്ഥിരം { തിരികെ m_outputSplitCFuncs; }
    പൂർണ്ണസംഖ്യ outputSplitCTrace() സ്ഥിരം { തിരികെ m_outputSplitCTrace; }
    പൂർണ്ണസംഖ്യ outputGroups() സ്ഥിരം { തിരികെ m_outputGroups; }
    int64_t outputGroupsSize() സ്ഥിരം { തിരികെ m_outputGroupsSize; }
    പൂർണ്ണസംഖ്യ pinsBv() സ്ഥിരം VL_MT_SAFE { തിരികെ m_pinsBv; }
    പൂർണ്ണസംഖ്യ reloopLimit() സ്ഥിരം { തിരികെ m_reloopLimit; }
    പൂർണ്ണസംഖ്യ replicationLimit() സ്ഥിരം { തിരികെ m_replicationLimit; }
    VOptionBool skipIdentical() സ്ഥിരം { തിരികെ m_skipIdentical; }
    bool stopFail() സ്ഥിരം { തിരികെ m_stopFail; }
    പൂർണ്ണസംഖ്യ threads() സ്ഥിരം VL_MT_SAFE { തിരികെ m_threads; }
    പൂർണ്ണസംഖ്യ threadsMaxMTasks() സ്ഥിരം { തിരികെ m_threadsMaxMTasks; }
    bool mtasks() സ്ഥിരം VL_MT_SAFE { തിരികെ (m_threads > 1); }
    VTimescale timeDefaultPrec() സ്ഥിരം { തിരികെ m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() സ്ഥിരം { തിരികെ m_timeDefaultUnit; }
    VTimescale timeOverridePrec() സ്ഥിരം { തിരികെ m_timeOverridePrec; }
    VTimescale timeOverrideUnit() സ്ഥിരം { തിരികെ m_timeOverrideUnit; }
    VTimescale timeComputePrec(സ്ഥിരം VTimescale& flag) സ്ഥിരം;
    VTimescale timeComputeUnit(സ്ഥിരം VTimescale& flag) സ്ഥിരം;
    പൂർണ്ണസംഖ്യ traceDepth() സ്ഥിരം { തിരികെ m_traceDepth; }
    പൂർണ്ണസംഖ്യ traceMaxArray() സ്ഥിരം { തിരികെ m_traceMaxArray; }
    പൂർണ്ണസംഖ്യ traceMaxWidth() സ്ഥിരം { തിരികെ m_traceMaxWidth; }
    bool useTraceParallel() സ്ഥിരം {
        തിരികെ trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    പൂർണ്ണസംഖ്യ unroll_എണ്ണം() സ്ഥിരം { തിരികെ m_unroll_എണ്ണം; }
    പൂർണ്ണസംഖ്യ unrollLimit() സ്ഥിരം { തിരികെ m_unrollLimit; }
    പൂർണ്ണസംഖ്യ unrollStmts() സ്ഥിരം { തിരികെ m_unrollStmts; }
    പൂർണ്ണസംഖ്യ constraintArrayLimit() സ്ഥിരം { തിരികെ m_constraintArrayLimit; }
    പൂർണ്ണസംഖ്യ verilateJobs() സ്ഥിരം { തിരികെ m_verilateJobs; }

    പൂർണ്ണസംഖ്യ compLimitBlocks() സ്ഥിരം { തിരികെ m_compLimitBlocks; }
    പൂർണ്ണസംഖ്യ compLimitMembers() സ്ഥിരം VL_MT_SAFE { തിരികെ m_compLimitMembers; }
    പൂർണ്ണസംഖ്യ compLimitParens() സ്ഥിരം { തിരികെ m_compLimitParens; }

    string diagnosticsSarifOutput() സ്ഥിരം VL_MT_SAFE {
        തിരികെ m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() സ്ഥിരം { തിരികെ m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() സ്ഥിരം { തിരികെ m_hierParamsFile; }
    string jsonOnlyOutput() സ്ഥിരം { തിരികെ m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() സ്ഥിരം { തിരികെ m_jsonOnlyMetaOutput; }
    string l2Name() സ്ഥിരം { തിരികെ m_l2Name; }
    string libCreate() സ്ഥിരം { തിരികെ m_libCreate; }
    string libCreateName(bool shared) സ്ഥിരം {
        string libName = "lib" + libCreate();
        എങ്കിൽ (shared) {
            libName += ".so";
        } അല്ലെങ്കിൽ {
            libName += ".a";
        }
        തിരികെ libName;
    }
    string main_മുകൾ_Name() സ്ഥിരം { തിരികെ m_main_മുകൾ_Name; }
    string makeDir() സ്ഥിരം VL_MT_SAFE { തിരികെ m_makeDir; }
    string modPrefix() സ്ഥിരം VL_MT_SAFE { തിരികെ m_modPrefix; }
    string pipeFilter() സ്ഥിരം { തിരികെ m_pipeFilter; }
    string prefix() സ്ഥിരം VL_MT_SAFE { തിരികെ m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_താക്കോൽ_Provided() സ്ഥിരം { തിരികെ !m_protect_താക്കോൽ.empty(); }
    string protect_താക്കോൽ_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string മുകൾ_Module() സ്ഥിരം { തിരികെ m_മുകൾ_Module; }  // As AstNode::encodeName()
    bool no_Trace_മുകൾ() സ്ഥിരം { തിരികെ m_no_Trace_മുകൾ; }
    string unusedRegexp() സ്ഥിരം { തിരികെ m_unusedRegexp; }
    string waiverOutput() സ്ഥിരം { തിരികെ m_waiverOutput; }
    string work() സ്ഥിരം { തിരികെ m_work; }
    bool isWaiverOutput() സ്ഥിരം { തിരികെ !m_waiverOutput.empty(); }
    string xAssign() സ്ഥിരം { തിരികെ m_xAssign; }
    string xInitial() സ്ഥിരം { തിരികെ m_xInitial; }

    സ്ഥിരം VStringSet& cppFiles() സ്ഥിരം { തിരികെ m_cppFiles; }
    സ്ഥിരം VStringList& cFlags() സ്ഥിരം { തിരികെ m_cFlags; }
    സ്ഥിരം VStringSet& compilerIncludes() സ്ഥിരം { തിരികെ m_compilerIncludes; }
    സ്ഥിരം VStringList& ldLibFiles() സ്ഥിരം { തിരികെ m_ldLibFiles; }
    സ്ഥിരം VStringList& ldLibs() സ്ഥിരം { തിരികെ m_ldLibs; }
    സ്ഥിരം VStringList& makeFlags() സ്ഥിരം { തിരികെ m_makeFlags; }
    സ്ഥിരം VFileLibSet& libraryFiles() സ്ഥിരം { തിരികെ m_libraryFiles; }
    സ്ഥിരം VStringSet& libmapFiles() സ്ഥിരം { തിരികെ m_libmapFiles; }
    സ്ഥിരം VFileLibList& vFiles() സ്ഥിരം { തിരികെ m_vFiles; }
    സ്ഥിരം VFileLibSet& vltFiles() സ്ഥിരം { തിരികെ m_vltFiles; }
    സ്ഥിരം VStringList& forceIncs() സ്ഥിരം { തിരികെ m_forceIncs; }

    bool hasParameter(സ്ഥിരം string& name);
    string parameter(സ്ഥിരം string& name);
    ശൂന്യം checkParameters();

    bool isFuture(സ്ഥിരം string& flag) സ്ഥിരം;
    bool isFuture0(സ്ഥിരം string& flag) സ്ഥിരം;
    bool isFuture1(സ്ഥിരം string& flag) സ്ഥിരം;
    bool isLibraryFile(സ്ഥിരം string& filename, സ്ഥിരം string& libname) സ്ഥിരം;

    // ACCESSORS (optimization options)
    bool fAcycSimp() സ്ഥിരം { തിരികെ m_fAcycSimp; }
    bool fAssemble() സ്ഥിരം { തിരികെ m_fAssemble; }
    bool fBitScanLoops() സ്ഥിരം { തിരികെ m_fBitScanLoops; }
    bool fBitblast() സ്ഥിരം { തിരികെ m_fBitblast; }
    bool fCaseDecoder() സ്ഥിരം { തിരികെ m_fCaseDecoder; }
    bool fCaseTable() സ്ഥിരം { തിരികെ m_fCaseTable; }
    bool f_Case_വൃക്ഷം() സ്ഥിരം { തിരികെ m_f_Case_വൃക്ഷം; }
    bool fCombine() സ്ഥിരം { തിരികെ m_fCombine; }
    bool fConst() സ്ഥിരം { തിരികെ m_fConst; }
    bool fConstBeforeDfg() സ്ഥിരം { തിരികെ m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_വൃക്ഷം() സ്ഥിരം { തിരികെ m_f_Const_Bit_Op_വൃക്ഷം; }
    bool fConstEager() സ്ഥിരം { തിരികെ m_fConstEager; }
    bool fDedupe() സ്ഥിരം { തിരികെ m_fDedupe; }
    bool fDfg() സ്ഥിരം { തിരികെ m_fDfg; }
    bool fDfgPeephole() സ്ഥിരം { തിരികെ m_fDfgPeephole; }
    bool f_Dfg_തള്ളുക_Down_Sels() സ്ഥിരം { തിരികെ m_f_Dfg_തള്ളുക_Down_Sels; }
    bool fDfgSynthesizeAll() സ്ഥിരം { തിരികെ m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(സ്ഥിരം std::string& name) സ്ഥിരം {
        തിരികെ !m_fDfgPeepholeDisabled.എണ്ണം(name);
    }
    bool fDeadAssigns() സ്ഥിരം { തിരികെ m_fDeadAssigns; }
    bool fDeadCells() സ്ഥിരം { തിരികെ m_fDeadCells; }
    bool fDeadMethods() സ്ഥിരം { തിരികെ m_fDeadMethods; }
    bool fDecompose() സ്ഥിരം { തിരികെ m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() സ്ഥിരം { തിരികെ m_fDecomposePacked; }
    bool fDecomposeUnpacked() സ്ഥിരം { തിരികെ m_fDecomposeUnpacked; }
    bool fExpand() സ്ഥിരം { തിരികെ m_fExpand; }
    bool fFuncBalanceCat() സ്ഥിരം { തിരികെ m_fFuncBalanceCat; }
    bool fFuncSplitCat() സ്ഥിരം { തിരികെ m_fFuncSplitCat; }
    bool fFunc() സ്ഥിരം { തിരികെ fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() സ്ഥിരം { തിരികെ m_fGate; }
    VOptionBool fIcoChangeDetect() സ്ഥിരം { തിരികെ m_fIcoChangeDetect; }
    bool fInline() സ്ഥിരം { തിരികെ m_fInline; }
    bool fInlineCFuncs() സ്ഥിരം { തിരികെ m_fInlineCFuncs; }
    bool fInlineFuncs() സ്ഥിരം { തിരികെ m_fInlineFuncs; }
    bool fInlineFuncsEager() സ്ഥിരം { തിരികെ m_fInlineFuncsEager; }
    bool fLife() സ്ഥിരം { തിരികെ m_fLife; }
    bool fLifePost() സ്ഥിരം { തിരികെ m_fLifePost; }
    bool fLocalize() സ്ഥിരം { തിരികെ m_fLocalize; }
    bool fMergeCond() സ്ഥിരം { തിരികെ m_fMergeCond; }
    bool fMergeCondMotion() സ്ഥിരം { തിരികെ m_fMergeCondMotion; }
    bool fReloop() സ്ഥിരം { തിരികെ m_fReloop; }
    bool fReorder() സ്ഥിരം { തിരികെ m_fReorder; }
    bool fSlice() സ്ഥിരം { തിരികെ m_fSlice; }
    പൂർണ്ണസംഖ്യ fSliceElementLimit() സ്ഥിരം { തിരികെ m_fSliceElementLimit; }
    bool fSplit() സ്ഥിരം { തിരികെ m_fSplit; }
    bool fSubst() സ്ഥിരം { തിരികെ m_fSubst; }
    bool fSubstConst() സ്ഥിരം { തിരികെ m_fSubstConst; }
    bool fTable() സ്ഥിരം { തിരികെ m_fTable; }
    bool fTaskifyAll() സ്ഥിരം { തിരികെ m_fTaskifyAll; }

    std::string traceClassBase() സ്ഥിരം VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() സ്ഥിരം VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() സ്ഥിരം VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() സ്ഥിരം VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() സ്ഥിരം VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() സ്ഥിരം VL_MT_SAFE;

    bool hierarchical() സ്ഥിരം { തിരികെ m_hierarchical; }
    പൂർണ്ണസംഖ്യ hierChild() സ്ഥിരം VL_MT_SAFE { തിരികെ m_hierChild; }
    പൂർണ്ണസംഖ്യ hierThreads() സ്ഥിരം VL_MT_SAFE { തിരികെ m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_മുകൾ() സ്ഥിരം VL_MT_SAFE { തിരികെ !m_hierChild && !m_hierBlocks.empty(); }
    സ്ഥിരം V3HierBlockOptSet& hierBlocks() സ്ഥിരം { തിരികെ m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_മുകൾ_Data_Dir() സ്ഥിരം VL_MT_SAFE {
        തിരികെ hier_മുകൾ() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    സ്ഥിതം string version() VL_PURE;
    സ്ഥിതം string argString(പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv);  ///< Return list of arguments as simple string
    string allArgsString() സ്ഥിരം VL_MT_SAFE;  ///< Return all passed arguments as simple string
    സ്ഥിരം std::list<std::pair<std::list<std::string>, bool>>& allArgs() സ്ഥിരം;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_മുകൾ) സ്ഥിരം;
    ശൂന്യം parseOpts(File_വരി* fl, പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv) VL_MT_DISABLED;
    ശൂന്യം parseOptsList(File_വരി* fl, സ്ഥിരം string& optdir, പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv) VL_MT_DISABLED;
    ശൂന്യം parseOptsFile(File_വരി* fl, സ്ഥിരം string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    സ്ഥിതം string getenvBuiltins(സ്ഥിരം string& var);
    സ്ഥിതം string getenvMAKE();
    സ്ഥിതം string getenvMAKEFLAGS();
    സ്ഥിതം string getenvPERL();
    സ്ഥിതം string getenvPYTHON3();
    സ്ഥിതം string getenvSYSTEMC();
    സ്ഥിതം string getenvSYSTEMC_ARCH();
    സ്ഥിതം string getenvSYSTEMC_INCLUDE();
    സ്ഥിതം string getenvSYSTEMC_LIBDIR();
    സ്ഥിതം string getenv_VERILATOR_വേര്();
    സ്ഥിതം string getenvVERILATOR_SOLVER();
    സ്ഥിതം string getStdPackagePath();
    സ്ഥിതം string getStdWaiverPath();
    സ്ഥിതം string getSupported(സ്ഥിരം string& var);
    സ്ഥിതം bool systemCSystemWide();
    സ്ഥിതം bool systemCFound();  // SystemC installed, or environment points to it
    സ്ഥിതം bool coroutineSupport();  // Configured C++ compiler supports coroutines
    സ്ഥിതം bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    സ്ഥിതം bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    സ്ഥിതം bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(സ്ഥിരം string& filename);
    string filePath(File_വരി* fl, സ്ഥിരം string& modname, സ്ഥിരം string& lastpath,
                    സ്ഥിരം string& errmsg);
    string file_Path_Looked_സന്ദേശം(File_വരി* fl, സ്ഥിരം string& modname);
    V3LangCode fileLanguage(സ്ഥിരം string& filename);
    സ്ഥിതം bool fileStatNormal(സ്ഥിരം string& filename);

    // METHODS (other OS)
    സ്ഥിതം ശൂന്യം throwSigsegv();
};

//######################################################################

#endif  // guard
