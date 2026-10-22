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
    explicit VOptionBool(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर ध्रुव वर्ण* ध्रुव names[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        प्रत्यागम names[m_e];
    }
    bool isDefault() ध्रुव { प्रत्यागम m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ध्रुव { प्रत्यागम m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool isSetTrue() ध्रुव { प्रत्यागम m_e == OPT_TRUE; }
    bool isSetFalse() ध्रुव { प्रत्यागम m_e == OPT_FALSE; }
    शून्य setTrueOrFalse(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool operator==(ध्रुव VOptionBool& lhs, ध्रुव VOptionBool& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VOptionBool& lhs, VOptionBool::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VOptionBool::en lhs, ध्रुव VOptionBool& rhs) { प्रत्यागम lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ध्रुव string m_filename;  // Filename
    ध्रुव string m_libname;  // Libname
सार्वजनिक:
    VFileLibName(ध्रुव string& filename, ध्रुव string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ध्रुव VFileLibName& rhs) = स्वभाव;
    string filename() ध्रुव { प्रत्यागम m_filename; }
    string libname() ध्रुव { प्रत्यागम m_libname; }
    bool operator==(ध्रुव VFileLibName& rhs) ध्रुव {
        प्रत्यागम m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool operator<(ध्रुव VFileLibName& rhs) ध्रुव {
        यदि (m_filename < rhs.m_filename) प्रत्यागम true;
        यदि (m_filename > rhs.m_filename) प्रत्यागम false;
        प्रत्यागम m_libname < rhs.m_libname;
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
    VTimescale(ध्रुव string& मूल्य, bool& badr);
    VTimescale(द्विगुण मूल्य, bool& badr) {
        badr = false;
        प्रति (पूर्णाङ्क i = TS_100S; i < _ENUM_END; ++i) {
            m_e = static_cast<en>(i);
            यदि (multiplier() == मूल्य) भङ्ग;
        }
        यदि (multiplier() != मूल्य) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ध्रुव { प्रत्यागम m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    स्थिर शून्य parseSlashed(File_ओळ* fl, ध्रुव वर्ण* textp, VTimescale& unitr, VTimescale& precr,
                             bool allowEmpty = false);
    ध्रुव वर्ण* ascii() ध्रुव {
        स्थिर ध्रुव वर्ण* ध्रुव names[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        प्रत्यागम names[m_e];
    }
    पूर्णाङ्क powerOfTen() ध्रुव { प्रत्यागम 2 - static_cast<पूर्णाङ्क>(m_e); }
    द्विगुण multiplier() ध्रुव {
        स्थिर ध्रुव द्विगुण मूल्ये[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        प्रत्यागम मूल्ये[m_e];
    }
};
constexpr bool operator==(ध्रुव VTimescale& lhs, ध्रुव VTimescale& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VTimescale& lhs, VTimescale::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool operator==(VTimescale::en lhs, ध्रुव VTimescale& rhs) { प्रत्यागम lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool operator<(ध्रुव VTimescale& lhs, ध्रुव VTimescale& rhs) {
    प्रत्यागम lhs.m_e > rhs.m_e;
}
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VTimescale& rhs) {
    प्रत्यागम os << rhs.ascii();
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
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    constexpr ध्रुव वर्ण* ascii() ध्रुव {
        constexpr ध्रुव वर्ण* ध्रुव names[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        प्रत्यागम names[m_e];
    }
};
constexpr bool operator==(ध्रुव VFsmExpandType& lhs, ध्रुव VFsmExpandType& rhs) VL_MT_SAFE {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_SAFE {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VFsmExpandType::en lhs, ध्रुव VFsmExpandType& rhs) VL_MT_SAFE {
    प्रत्यागम lhs == rhs.m_e;
}
अन्तःस्थ std::ostream& operator<<(std::ostream& os, ध्रुव VFsmExpandType& rhs) VL_MT_SAFE {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3HierarchicalBlockOption final {
सार्वजनिक:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ध्रुव std::string, std::string>;

गुप्त:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

सार्वजनिक:
    explicit V3HierarchicalBlockOption(ध्रुव string& optstring);
    ध्रुव string& origName() ध्रुव { प्रत्यागम m_origName; }
    ध्रुव string& mangledName() ध्रुव { प्रत्यागम m_mangledName; }
    ध्रुव ParamStrMap params() ध्रुव { प्रत्यागम m_parameters; }
};

using V3HierBlockOptSet = std::map<ध्रुव std::string, V3HierarchicalBlockOption>;

//######################################################################
// V3Options - Command line options

class V3Options final {
सार्वजनिक:
गुप्त:
    // TYPES
    using DebugLevelMap = std::map<ध्रुव std::string, unsigned>;

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
    std::map<ध्रुव string, string> m_parameters;  // Parameters
    std::map<ध्रुव string, V3HierarchicalBlockOption> m_hierBlocks;  // main switch: --hierarchical-block
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
    पूर्णाङ्क         m_coverage_अभिव्यक्ती_Max = 32;    // main switch: --coverage-expr-max
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
    पूर्णाङ्क         m_instr_मोजणी_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    पूर्णाङ्क         m_localizeMaxSize = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    पूर्णाङ्क         m_max_अंक_Width = 65536;  // main switch: --max-num-width
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
    पूर्णाङ्क         m_unroll_मोजणी = 64;  // main switch: --unroll-count
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
    पूर्णाङ्क  m_fSliceElementLimit = 256;  // main switch: --fslice-element-limit
    bool m_fSplit;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_fTable;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_available = false;  // Set to true at the end of option parsing

    // METHODS
    शून्य addArg(वर्ण** argv, size_t मोजणी, bool isForRerun);
    शून्य addArg(ध्रुव std::string& arg, bool isForRerun);
    शून्य add_ओळ_Arg(ध्रुव string& arg);
    शून्य addDefine(ध्रुव string& defline, bool allowPlus) VL_MT_DISABLED;
    शून्य addFuture(ध्रुव string& flag);
    शून्य addFuture0(ध्रुव string& flag);
    शून्य addFuture1(ध्रुव string& flag);
    शून्य addIncDirUser(ध्रुव string& incdir);  // User requested
    शून्य addIncDirFallback(ध्रुव string& incdir);  // Low priority if not found otherwise
    शून्य addParameter(ध्रुव string& paramline, bool allowPlus);
    शून्य addLangExt(ध्रुव string& langext, ध्रुव V3LangCode& lc);
    शून्य addLibExtV(ध्रुव string& libext);
    शून्य optimize(पूर्णाङ्क level);
    शून्य showVersion(bool verbose);
    शून्य coverage(bool flag) {
        m_coverage_ओळ = m_coverageBranch = m_coverageToggle = m_coverage_अभिव्यक्ती = m_coverageFsm
            = m_coverageUser = flag;
    }
    स्थिर bool suffixed(ध्रुव string& sw, ध्रुव वर्ण* arg);
    स्थिर string parseFileArg(ध्रुव string& optdir, ध्रुव string& relfilename);
    string filePathCheckOneDir(ध्रुव string& modname, ध्रुव string& dirname);
    स्थिर पूर्णाङ्क stripOptionsForChildRun(ध्रुव string& opt, bool for_शीर्ष);
    शून्य validateIdentifier(File_ओळ* fl, ध्रुव string& arg, ध्रुव string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

सार्वजनिक:
    V3Options();
    ~V3Options();
    शून्य setDebugMode(पूर्णाङ्क level);
    unsigned debugLevel(ध्रुव string& tag) ध्रुव VL_MT_SAFE;
    unsigned debugSrcLevel(ध्रुव string& srcfile_path) ध्रुव VL_MT_SAFE;
    unsigned dumpLevel(ध्रुव string& tag) ध्रुव VL_MT_SAFE;
    unsigned dumpSrcLevel(ध्रुव string& srcfile_path) ध्रुव VL_MT_SAFE;

    // METHODS
    शून्य addCppFile(ध्रुव string& filename);
    शून्य addCFlags(ध्रुव string& filename);
    शून्य addCompilerIncludes(ध्रुव string& filename);
    शून्य addLdLibFile(ध्रुव string& filename);
    शून्य addLdLibs(ध्रुव string& filename);
    शून्य addMakeFlags(ध्रुव string& filename);
    शून्य addLibMapFile(ध्रुव string& filename);
    शून्य addLibraryFile(ध्रुव string& filename, ध्रुव string& libname);
    शून्य addVFile(ध्रुव string& filename, ध्रुव string& libname);
    शून्य addVltFile(ध्रुव string& filename, ध्रुव string& libname);
    शून्य addForceInc(ध्रुव string& filename);
    bool available() ध्रुव VL_MT_SAFE { प्रत्यागम m_available; }
    शून्य ccSet();
    शून्य decorations(File_ओळ* fl, ध्रुव string& filename);
    शून्य notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ध्रुव { प्रत्यागम m_preprocOnly; }
    bool makePhony() ध्रुव { प्रत्यागम m_makePhony; }
    bool preproc_No_ओळ() ध्रुव { प्रत्यागम m_preproc_No_ओळ; }
    bool preprocResolve() ध्रुव { प्रत्यागम m_preprocResolve; }
    पूर्णाङ्क preprocTokenLimit() ध्रुव { प्रत्यागम m_preprocTokenLimit; }
    bool underlineZero() ध्रुव { प्रत्यागम m_underlineZero; }
    bool systemC() ध्रुव VL_MT_SAFE { प्रत्यागम m_systemC; }
    bool savable() ध्रुव VL_MT_SAFE { प्रत्यागम m_savable; }
    VOptionBool schedZeroDelay() ध्रुव { प्रत्यागम m_schedZeroDelay; }
    bool stats() ध्रुव { प्रत्यागम m_stats; }
    bool statsVars() ध्रुव { प्रत्यागम m_statsVars; }
    bool stdPackage() ध्रुव { प्रत्यागम m_stdPackage; }
    bool stdWaiver() ध्रुव { प्रत्यागम m_stdWaiver; }
    bool assertOn() ध्रुव { प्रत्यागम m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ध्रुव { प्रत्यागम m_assertCase; }
    bool autoflush() ध्रुव { प्रत्यागम m_autoflush; }
    bool bboxSys() ध्रुव { प्रत्यागम m_bboxSys; }
    bool bboxUnsup() ध्रुव { प्रत्यागम m_bboxUnsup; }
    bool build() ध्रुव { प्रत्यागम m_build; }
    string buildDepBin() ध्रुव { प्रत्यागम m_buildDepBin; }
    शून्य buildDepBin(ध्रुव string& flag) { m_buildDepBin = flag; }
    bool context() ध्रुव VL_MT_SAFE { प्रत्यागम m_context; }
    bool coverage() ध्रुव VL_MT_SAFE {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        प्रत्यागम m_coverage_ओळ || m_coverageBranch || m_coverageToggle || m_coverage_अभिव्यक्ती
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ध्रुव VL_MT_SAFE {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        प्रत्यागम m_coverage_ओळ || m_coverageBranch || m_coverageToggle || m_coverage_अभिव्यक्ती
               || m_coverageUser;
    }
    bool coverageBranch() ध्रुव { प्रत्यागम m_coverageBranch; }
    bool coverage_अभिव्यक्ती() ध्रुव { प्रत्यागम m_coverage_अभिव्यक्ती; }
    bool coverageFsm() ध्रुव { प्रत्यागम m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ध्रुव { प्रत्यागम m_coverageFsmExpand; }
    पूर्णाङ्क coverageFsmMaxArcs() ध्रुव { प्रत्यागम m_coverageFsmMaxArcs; }
    bool coverage_ओळ() ध्रुव { प्रत्यागम m_coverage_ओळ; }
    bool coverageMergeInstances() ध्रुव { प्रत्यागम m_coverageMergeInstances; }
    bool coveragePerInstance() ध्रुव { प्रत्यागम m_coveragePerInstance; }
    bool coverageToggle() ध्रुव { प्रत्यागम m_coverageToggle; }
    bool coverageUnderscore() ध्रुव { प्रत्यागम m_coverageUnderscore; }
    bool coverageUser() ध्रुव { प्रत्यागम m_coverageUser; }
    bool debugCheck() ध्रुव VL_MT_SAFE { प्रत्यागम m_debugCheck; }
    bool debugCollision() ध्रुव { प्रत्यागम m_debugCollision; }
    bool debugEmitV() ध्रुव VL_MT_SAFE { प्रत्यागम m_debugEmitV; }
    bool debugExitElab() ध्रुव { प्रत्यागम m_debugExitElab; }
    bool debugExitParse() ध्रुव { प्रत्यागम m_debugExitParse; }
    bool debugLeak() ध्रुव { प्रत्यागम m_debugLeak; }
    bool debugNondeterminism() ध्रुव { प्रत्यागम m_debugNondeterminism; }
    bool debugPartition() ध्रुव { प्रत्यागम m_debugPartition; }
    bool debugPreprocPassthru() ध्रुव VL_MT_SAFE { प्रत्यागम m_debugPreprocPassthru; }
    bool debugProtect() ध्रुव VL_MT_SAFE { प्रत्यागम m_debugProtect; }
    पूर्णाङ्क debugRuntimeTimeout() ध्रुव { प्रत्यागम m_debugRuntimeTimeout; }
    bool debugSelfTest() ध्रुव { प्रत्यागम m_debugSelfTest; }
    bool debug_चळत_Check() ध्रुव { प्रत्यागम m_debug_चळत_Check; }
    bool debugWidth() ध्रुव VL_PURE { प्रत्यागम m_debugWidth; }
    bool decoration() ध्रुव VL_MT_SAFE { प्रत्यागम m_decoration; }
    bool decorationNodes() ध्रुव VL_MT_SAFE { प्रत्यागम m_decorationNodes; }
    bool diagnosticsSarif() ध्रुव VL_MT_SAFE { प्रत्यागम m_diagnosticsSarif; }
    bool dpiHdrOnly() ध्रुव { प्रत्यागम m_dpiHdrOnly; }
    bool dumpAstPatterns() ध्रुव {
        प्रत्यागम m_dumpLevel.मोजणी("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dumpDefines() ध्रुव { प्रत्यागम m_dumpLevel.मोजणी("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ध्रुव {
        प्रत्यागम m_dumpLevel.मोजणी("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_वृक्ष_Dot() ध्रुव {
        प्रत्यागम m_dumpLevel.मोजणी("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ध्रुव { प्रत्यागम m_emitAccessors; }
    bool exe() ध्रुव { प्रत्यागम m_exe; }
    bool flatten() ध्रुव { प्रत्यागम m_flatten; }
    bool fourstate() ध्रुव { प्रत्यागम m_fourstate; }
    bool gmake() ध्रुव { प्रत्यागम m_gmake; }
    bool makeJson() ध्रुव { प्रत्यागम m_makeJson; }
    bool threadsDpiPure() ध्रुव { प्रत्यागम m_threadsDpiPure; }
    bool threadsDpiUnpure() ध्रुव { प्रत्यागम m_threadsDpiUnpure; }
    bool threadsCoarsen() ध्रुव { प्रत्यागम m_threadsCoarsen; }
    VOptionBool timing() ध्रुव { प्रत्यागम m_timing; }
    bool trace() ध्रुव { प्रत्यागम m_trace; }
    bool traceCoverage() ध्रुव { प्रत्यागम m_traceCoverage; }
    bool traceEnabledFst() ध्रुव { प्रत्यागम m_traceEnabledFst; }
    bool traceEnabledSaif() ध्रुव { प्रत्यागम m_traceEnabledSaif; }
    bool traceEnabledVcd() ध्रुव { प्रत्यागम m_traceEnabledVcd; }
    bool traceParams() ध्रुव { प्रत्यागम m_traceParams; }
    bool traceStructs() ध्रुव { प्रत्यागम m_traceStructs; }
    bool traceUnderscore() ध्रुव { प्रत्यागम m_traceUnderscore; }
    bool main() ध्रुव { प्रत्यागम m_main; }
    bool outFormatOk() ध्रुव { प्रत्यागम m_outFormatOk; }
    bool jsonOnly() ध्रुव { प्रत्यागम m_jsonOnly; }
    bool keepTempFiles() ध्रुव { प्रत्यागम (V3Error::debugDefault() != 0); }
    bool pedantic() ध्रुव { प्रत्यागम m_pedantic; }
    bool pinsInoutEnables() ध्रुव { प्रत्यागम m_pinsInoutEnables; }
    bool pinsScUint() ध्रुव { प्रत्यागम m_pinsScUint; }
    bool pinsScUintBool() ध्रुव { प्रत्यागम m_pinsScUintBool; }
    bool pinsScBigUint() ध्रुव VL_MT_SAFE { प्रत्यागम m_pinsScBigUint; }
    bool pinsUint8() ध्रुव { प्रत्यागम m_pinsUint8; }
    bool preprocComments() ध्रुव { प्रत्यागम m_preprocComments; }
    bool preprocDefines() ध्रुव { प्रत्यागम m_preprocDefines; }
    bool profC() ध्रुव { प्रत्यागम m_profC; }
    bool profCFuncs() ध्रुव { प्रत्यागम m_profCFuncs; }
    bool profExec() ध्रुव { प्रत्यागम m_profExec; }
    bool profPgo() ध्रुव { प्रत्यागम m_profPgo; }
    bool usesProfiler() ध्रुव { प्रत्यागम profExec() || profPgo(); }
    bool protectIds() ध्रुव VL_MT_SAFE { प्रत्यागम m_protectIds; }
    bool allPublic() ध्रुव { प्रत्यागम m_public; }
    bool publicParams() ध्रुव { प्रत्यागम m_publicParams; }
    bool publicOff() ध्रुव { प्रत्यागम m_publicIgnore; }
    bool publicFlatRW() ध्रुव { प्रत्यागम m_publicFlatRW; }
    पूर्णाङ्क publicDepth() ध्रुव { प्रत्यागम m_publicDepth; }
    bool anyPublicFlat() ध्रुव { प्रत्यागम m_publicParams || m_publicFlatRW || m_publicDepth; }
    bool lintOnly() ध्रुव VL_MT_SAFE { प्रत्यागम m_lintOnly; }
    bool ignc() ध्रुव { प्रत्यागम m_ignc; }
    bool quietBuild() ध्रुव VL_MT_SAFE { प्रत्यागम m_quietBuild; }
    bool quietExit() ध्रुव VL_MT_SAFE { प्रत्यागम m_quietExit; }
    bool quietStats() ध्रुव VL_MT_SAFE { प्रत्यागम m_quietStats; }
    bool reportUnoptflat() ध्रुव { प्रत्यागम m_reportUnoptflat; }
    bool verilate() ध्रुव { प्रत्यागम m_verilate; }
    bool vpi() ध्रुव { प्रत्यागम m_vpi.isTrue(); }
    bool waiverMultiline() ध्रुव { प्रत्यागम m_waiverMultiline; }
    bool xInitialEdge() ध्रुव { प्रत्यागम m_xInitialEdge; }
    bool serializeOnly() ध्रुव { प्रत्यागम m_jsonOnly; }
    bool शीर्ष_Ifaces_Supported() ध्रुव { प्रत्यागम lintOnly() && !hierarchical(); }

    पूर्णाङ्क buildJobs() ध्रुव VL_MT_SAFE { प्रत्यागम m_buildJobs; }
    पूर्णाङ्क convergeLimit() ध्रुव { प्रत्यागम m_convergeLimit; }
    पूर्णाङ्क coverage_अभिव्यक्ती_Max() ध्रुव { प्रत्यागम m_coverage_अभिव्यक्ती_Max; }
    uint32_t coverageMaxBins() ध्रुव { प्रत्यागम m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ध्रुव { प्रत्यागम m_coverageMaxRealBins; }
    पूर्णाङ्क coverageMaxWidth() ध्रुव { प्रत्यागम m_coverageMaxWidth; }
    पूर्णाङ्क debugAllocRandom() ध्रुव { प्रत्यागम m_debugAllocRandom; }
    bool dump_वृक्ष_Addrids() ध्रुव VL_MT_SAFE;
    पूर्णाङ्क expandLimit() ध्रुव { प्रत्यागम m_expandLimit; }
    पूर्णाङ्क gateStmts() ध्रुव { प्रत्यागम m_gateStmts; }
    पूर्णाङ्क ifDepth() ध्रुव { प्रत्यागम m_ifDepth; }
    पूर्णाङ्क inlineCFuncs() ध्रुव { प्रत्यागम m_inlineCFuncs; }
    पूर्णाङ्क inlineCFuncsProduct() ध्रुव { प्रत्यागम m_inlineCFuncsProduct; }
    पूर्णाङ्क inlineMult() ध्रुव { प्रत्यागम m_inlineMult; }
    पूर्णाङ्क instr_मोजणी_Dpi() ध्रुव { प्रत्यागम m_instr_मोजणी_Dpi; }
    पूर्णाङ्क localizeMaxSize() ध्रुव { प्रत्यागम m_localizeMaxSize; }
    bool jsonEditNums() ध्रुव { प्रत्यागम m_jsonEditNums; }
    bool jsonIds() ध्रुव { प्रत्यागम m_jsonIds; }
    VOptionBool makeDepend() ध्रुव { प्रत्यागम m_makeDepend; }
    पूर्णाङ्क max_अंक_Width() ध्रुव { प्रत्यागम m_max_अंक_Width; }
    पूर्णाङ्क funcRecursionDepth() ध्रुव { प्रत्यागम m_funcRecursion; }
    पूर्णाङ्क moduleRecursionDepth() ध्रुव { प्रत्यागम m_moduleRecursion; }
    पूर्णाङ्क outputSplit() ध्रुव { प्रत्यागम m_outputSplit; }
    पूर्णाङ्क outputSplitCFuncs() ध्रुव { प्रत्यागम m_outputSplitCFuncs; }
    पूर्णाङ्क outputSplitCTrace() ध्रुव { प्रत्यागम m_outputSplitCTrace; }
    पूर्णाङ्क outputGroups() ध्रुव { प्रत्यागम m_outputGroups; }
    int64_t outputGroupsSize() ध्रुव { प्रत्यागम m_outputGroupsSize; }
    पूर्णाङ्क pinsBv() ध्रुव VL_MT_SAFE { प्रत्यागम m_pinsBv; }
    पूर्णाङ्क reloopLimit() ध्रुव { प्रत्यागम m_reloopLimit; }
    पूर्णाङ्क replicationLimit() ध्रुव { प्रत्यागम m_replicationLimit; }
    VOptionBool skipIdentical() ध्रुव { प्रत्यागम m_skipIdentical; }
    bool stopFail() ध्रुव { प्रत्यागम m_stopFail; }
    पूर्णाङ्क threads() ध्रुव VL_MT_SAFE { प्रत्यागम m_threads; }
    पूर्णाङ्क threadsMaxMTasks() ध्रुव { प्रत्यागम m_threadsMaxMTasks; }
    bool mtasks() ध्रुव VL_MT_SAFE { प्रत्यागम (m_threads > 1); }
    VTimescale timeDefaultPrec() ध्रुव { प्रत्यागम m_timeDefaultPrec; }
    VTimescale timeDefaultUnit() ध्रुव { प्रत्यागम m_timeDefaultUnit; }
    VTimescale timeOverridePrec() ध्रुव { प्रत्यागम m_timeOverridePrec; }
    VTimescale timeOverrideUnit() ध्रुव { प्रत्यागम m_timeOverrideUnit; }
    VTimescale timeComputePrec(ध्रुव VTimescale& flag) ध्रुव;
    VTimescale timeComputeUnit(ध्रुव VTimescale& flag) ध्रुव;
    पूर्णाङ्क traceDepth() ध्रुव { प्रत्यागम m_traceDepth; }
    पूर्णाङ्क traceMaxArray() ध्रुव { प्रत्यागम m_traceMaxArray; }
    पूर्णाङ्क traceMaxWidth() ध्रुव { प्रत्यागम m_traceMaxWidth; }
    bool useTraceParallel() ध्रुव {
        प्रत्यागम trace() && traceEnabledVcd() && (threads() > 1 || hierChild() > 1);
    }
    पूर्णाङ्क unroll_मोजणी() ध्रुव { प्रत्यागम m_unroll_मोजणी; }
    पूर्णाङ्क unrollLimit() ध्रुव { प्रत्यागम m_unrollLimit; }
    पूर्णाङ्क unrollStmts() ध्रुव { प्रत्यागम m_unrollStmts; }
    पूर्णाङ्क constraintArrayLimit() ध्रुव { प्रत्यागम m_constraintArrayLimit; }
    पूर्णाङ्क verilateJobs() ध्रुव { प्रत्यागम m_verilateJobs; }

    पूर्णाङ्क compLimitBlocks() ध्रुव { प्रत्यागम m_compLimitBlocks; }
    पूर्णाङ्क compLimitMembers() ध्रुव VL_MT_SAFE { प्रत्यागम m_compLimitMembers; }
    पूर्णाङ्क compLimitParens() ध्रुव { प्रत्यागम m_compLimitParens; }

    string diagnosticsSarifOutput() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_diagnosticsSarifOutput.empty() ? makeDir() + "/" + prefix() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ध्रुव { प्रत्यागम m_exeName != "" ? m_exeName : prefix(); }
    VFileLibList hierParamFile() ध्रुव { प्रत्यागम m_hierParamsFile; }
    string jsonOnlyOutput() ध्रुव { प्रत्यागम m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ध्रुव { प्रत्यागम m_jsonOnlyMetaOutput; }
    string l2Name() ध्रुव { प्रत्यागम m_l2Name; }
    string libCreate() ध्रुव { प्रत्यागम m_libCreate; }
    string libCreateName(bool shared) ध्रुव {
        string libName = "lib" + libCreate();
        यदि (shared) {
            libName += ".so";
        } अन्यथा {
            libName += ".a";
        }
        प्रत्यागम libName;
    }
    string main_शीर्ष_Name() ध्रुव { प्रत्यागम m_main_शीर्ष_Name; }
    string makeDir() ध्रुव VL_MT_SAFE { प्रत्यागम m_makeDir; }
    string modPrefix() ध्रुव VL_MT_SAFE { प्रत्यागम m_modPrefix; }
    string pipeFilter() ध्रुव { प्रत्यागम m_pipeFilter; }
    string prefix() ध्रुव VL_MT_SAFE { प्रत्यागम m_prefix; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_किल्ली_Provided() ध्रुव { प्रत्यागम !m_protect_किल्ली.empty(); }
    string protect_किल्ली_Defaulted() VL_MT_SAFE;  // Set default key if not set by user
    string शीर्ष_Module() ध्रुव { प्रत्यागम m_शीर्ष_Module; }  // As AstNode::encodeName()
    bool no_Trace_शीर्ष() ध्रुव { प्रत्यागम m_no_Trace_शीर्ष; }
    string unusedRegexp() ध्रुव { प्रत्यागम m_unusedRegexp; }
    string waiverOutput() ध्रुव { प्रत्यागम m_waiverOutput; }
    string work() ध्रुव { प्रत्यागम m_work; }
    bool isWaiverOutput() ध्रुव { प्रत्यागम !m_waiverOutput.empty(); }
    string xAssign() ध्रुव { प्रत्यागम m_xAssign; }
    string xInitial() ध्रुव { प्रत्यागम m_xInitial; }

    ध्रुव VStringSet& cppFiles() ध्रुव { प्रत्यागम m_cppFiles; }
    ध्रुव VStringList& cFlags() ध्रुव { प्रत्यागम m_cFlags; }
    ध्रुव VStringSet& compilerIncludes() ध्रुव { प्रत्यागम m_compilerIncludes; }
    ध्रुव VStringList& ldLibFiles() ध्रुव { प्रत्यागम m_ldLibFiles; }
    ध्रुव VStringList& ldLibs() ध्रुव { प्रत्यागम m_ldLibs; }
    ध्रुव VStringList& makeFlags() ध्रुव { प्रत्यागम m_makeFlags; }
    ध्रुव VFileLibSet& libraryFiles() ध्रुव { प्रत्यागम m_libraryFiles; }
    ध्रुव VStringSet& libmapFiles() ध्रुव { प्रत्यागम m_libmapFiles; }
    ध्रुव VFileLibList& vFiles() ध्रुव { प्रत्यागम m_vFiles; }
    ध्रुव VFileLibSet& vltFiles() ध्रुव { प्रत्यागम m_vltFiles; }
    ध्रुव VStringList& forceIncs() ध्रुव { प्रत्यागम m_forceIncs; }

    bool hasParameter(ध्रुव string& name);
    string parameter(ध्रुव string& name);
    शून्य checkParameters();

    bool isFuture(ध्रुव string& flag) ध्रुव;
    bool isFuture0(ध्रुव string& flag) ध्रुव;
    bool isFuture1(ध्रुव string& flag) ध्रुव;
    bool isLibraryFile(ध्रुव string& filename, ध्रुव string& libname) ध्रुव;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ध्रुव { प्रत्यागम m_fAcycSimp; }
    bool fAssemble() ध्रुव { प्रत्यागम m_fAssemble; }
    bool fBitScanLoops() ध्रुव { प्रत्यागम m_fBitScanLoops; }
    bool fBitblast() ध्रुव { प्रत्यागम m_fBitblast; }
    bool fCaseDecoder() ध्रुव { प्रत्यागम m_fCaseDecoder; }
    bool fCaseTable() ध्रुव { प्रत्यागम m_fCaseTable; }
    bool f_Case_वृक्ष() ध्रुव { प्रत्यागम m_f_Case_वृक्ष; }
    bool fCombine() ध्रुव { प्रत्यागम m_fCombine; }
    bool fConst() ध्रुव { प्रत्यागम m_fConst; }
    bool fConstBeforeDfg() ध्रुव { प्रत्यागम m_fConstBeforeDfg; }
    bool f_Const_Bit_Op_वृक्ष() ध्रुव { प्रत्यागम m_f_Const_Bit_Op_वृक्ष; }
    bool fConstEager() ध्रुव { प्रत्यागम m_fConstEager; }
    bool fDedupe() ध्रुव { प्रत्यागम m_fDedupe; }
    bool fDfg() ध्रुव { प्रत्यागम m_fDfg; }
    bool fDfgPeephole() ध्रुव { प्रत्यागम m_fDfgPeephole; }
    bool f_Dfg_ढकला_Down_Sels() ध्रुव { प्रत्यागम m_f_Dfg_ढकला_Down_Sels; }
    bool fDfgSynthesizeAll() ध्रुव { प्रत्यागम m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ध्रुव std::string& name) ध्रुव {
        प्रत्यागम !m_fDfgPeepholeDisabled.मोजणी(name);
    }
    bool fDeadAssigns() ध्रुव { प्रत्यागम m_fDeadAssigns; }
    bool fDeadCells() ध्रुव { प्रत्यागम m_fDeadCells; }
    bool fDeadMethods() ध्रुव { प्रत्यागम m_fDeadMethods; }
    bool fDecompose() ध्रुव { प्रत्यागम m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ध्रुव { प्रत्यागम m_fDecomposePacked; }
    bool fDecomposeUnpacked() ध्रुव { प्रत्यागम m_fDecomposeUnpacked; }
    bool fExpand() ध्रुव { प्रत्यागम m_fExpand; }
    bool fFuncBalanceCat() ध्रुव { प्रत्यागम m_fFuncBalanceCat; }
    bool fFuncSplitCat() ध्रुव { प्रत्यागम m_fFuncSplitCat; }
    bool fFunc() ध्रुव { प्रत्यागम fFuncSplitCat() || fFuncBalanceCat(); }
    bool fGate() ध्रुव { प्रत्यागम m_fGate; }
    VOptionBool fIcoChangeDetect() ध्रुव { प्रत्यागम m_fIcoChangeDetect; }
    bool fInline() ध्रुव { प्रत्यागम m_fInline; }
    bool fInlineCFuncs() ध्रुव { प्रत्यागम m_fInlineCFuncs; }
    bool fInlineFuncs() ध्रुव { प्रत्यागम m_fInlineFuncs; }
    bool fInlineFuncsEager() ध्रुव { प्रत्यागम m_fInlineFuncsEager; }
    bool fLife() ध्रुव { प्रत्यागम m_fLife; }
    bool fLifePost() ध्रुव { प्रत्यागम m_fLifePost; }
    bool fLocalize() ध्रुव { प्रत्यागम m_fLocalize; }
    bool fMergeCond() ध्रुव { प्रत्यागम m_fMergeCond; }
    bool fMergeCondMotion() ध्रुव { प्रत्यागम m_fMergeCondMotion; }
    bool fReloop() ध्रुव { प्रत्यागम m_fReloop; }
    bool fReorder() ध्रुव { प्रत्यागम m_fReorder; }
    bool fSlice() ध्रुव { प्रत्यागम m_fSlice; }
    पूर्णाङ्क fSliceElementLimit() ध्रुव { प्रत्यागम m_fSliceElementLimit; }
    bool fSplit() ध्रुव { प्रत्यागम m_fSplit; }
    bool fSubst() ध्रुव { प्रत्यागम m_fSubst; }
    bool fSubstConst() ध्रुव { प्रत्यागम m_fSubstConst; }
    bool fTable() ध्रुव { प्रत्यागम m_fTable; }
    bool fTaskifyAll() ध्रुव { प्रत्यागम m_fTaskifyAll; }

    std::string traceClassBase() ध्रुव VL_MT_SAFE;  // Deprecated
    std::string traceClassLang() ध्रुव VL_MT_SAFE;  // Deprecated
    std::vector<std::string> traceClassBases() ध्रुव VL_MT_SAFE;
    std::vector<std::string> traceClassLangs() ध्रुव VL_MT_SAFE;
    std::vector<std::string> traceSourceBases() ध्रुव VL_MT_SAFE;
    std::vector<std::string> traceSourceLangs() ध्रुव VL_MT_SAFE;

    bool hierarchical() ध्रुव { प्रत्यागम m_hierarchical; }
    पूर्णाङ्क hierChild() ध्रुव VL_MT_SAFE { प्रत्यागम m_hierChild; }
    पूर्णाङ्क hierThreads() ध्रुव VL_MT_SAFE { प्रत्यागम m_hierThreads == 0 ? m_threads : m_hierThreads; }
    bool hier_शीर्ष() ध्रुव VL_MT_SAFE { प्रत्यागम !m_hierChild && !m_hierBlocks.empty(); }
    ध्रुव V3HierBlockOptSet& hierBlocks() ध्रुव { प्रत्यागम m_hierBlocks; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_शीर्ष_Data_Dir() ध्रुव VL_MT_SAFE {
        प्रत्यागम hier_शीर्ष() ? (makeDir() + '/' + prefix() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    स्थिर string version() VL_PURE;
    स्थिर string argString(पूर्णाङ्क argc, वर्ण** argv);  ///< Return list of arguments as simple string
    string allArgsString() ध्रुव VL_MT_SAFE;  ///< Return all passed arguments as simple string
    ध्रुव std::list<std::pair<std::list<std::string>, bool>>& allArgs() ध्रुव;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string allArgsStringForHierBlock(bool for_शीर्ष) ध्रुव;
    शून्य parseOpts(File_ओळ* fl, पूर्णाङ्क argc, वर्ण** argv) VL_MT_DISABLED;
    शून्य parseOptsList(File_ओळ* fl, ध्रुव string& optdir, पूर्णाङ्क argc, वर्ण** argv) VL_MT_DISABLED;
    शून्य parseOptsFile(File_ओळ* fl, ध्रुव string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    स्थिर string getenvBuiltins(ध्रुव string& var);
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
    स्थिर string getSupported(ध्रुव string& var);
    स्थिर bool systemCSystemWide();
    स्थिर bool systemCFound();  // SystemC installed, or environment points to it
    स्थिर bool coroutineSupport();  // Configured C++ compiler supports coroutines
    स्थिर bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    स्थिर bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    स्थिर bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ध्रुव string& filename);
    string filePath(File_ओळ* fl, ध्रुव string& modname, ध्रुव string& lastpath,
                    ध्रुव string& errmsg);
    string file_Path_Looked_संदेश(File_ओळ* fl, ध्रुव string& modname);
    V3LangCode fileLanguage(ध्रुव string& filename);
    स्थिर bool fileStatNormal(ध्रुव string& filename);

    // METHODS (other OS)
    स्थिर शून्य throwSigsegv();
};

//######################################################################

#endif  // guard
