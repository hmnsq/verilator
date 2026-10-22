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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3OPTIONS_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3OPTIONS_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LangCode.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class V3OptionsImp;
class File_ਸਤਰ;

//######################################################################

class VOptionBool final {
    // Class to track options that are either not specified (and default
    // true/false), versus user setting the option to true or false
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { OPT_DEFAULT_FALSE = 0, OPT_DEFAULT_TRUE, OPT_TRUE, OPT_FALSE };
    ਗਿਣਤੀ en m_e;
    VOptionBool()
        : m_e{OPT_DEFAULT_FALSE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VOptionBool(en _e)
        : m_e{_e} {}
    explicit VOptionBool(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"DEFAULT_FALSE", "DEFAULT_TRUE", "OPT_TRUE", "OPT_FALSE"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    bool isDefault() ਅਚਲ { ਵਾਪਸ m_e == OPT_DEFAULT_FALSE || m_e == OPT_DEFAULT_TRUE; }
    bool isTrue() ਅਚਲ { ਵਾਪਸ m_e == OPT_TRUE || m_e == OPT_DEFAULT_TRUE; }
    bool is_ਸੈੱਟ_True() ਅਚਲ { ਵਾਪਸ m_e == OPT_TRUE; }
    bool is_ਸੈੱਟ_False() ਅਚਲ { ਵਾਪਸ m_e == OPT_FALSE; }
    ਖਾਲੀ ਸੈੱਟ_True_Or_False(bool flag) { m_e = flag ? OPT_TRUE : OPT_FALSE; }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VOptionBool& lhs, ਅਚਲ VOptionBool& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VOptionBool& lhs, VOptionBool::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VOptionBool::en lhs, ਅਚਲ VOptionBool& rhs) { ਵਾਪਸ lhs == rhs.m_e; }

//######################################################################

class VFileLibName final {
    // Filename and libname pair
    ਅਚਲ string m_filename;  // Filename
    ਅਚਲ string m_libname;  // Libname
ਜਨਤਕ:
    VFileLibName(ਅਚਲ string& filename, ਅਚਲ string& libname)
        : m_filename{filename}
        , m_libname{libname} {}
    VFileLibName(ਅਚਲ VFileLibName& rhs) = ਮੂਲ;
    string filename() ਅਚਲ { ਵਾਪਸ m_filename; }
    string libname() ਅਚਲ { ਵਾਪਸ m_libname; }
    bool ਸੰਚਾਲਕ==(ਅਚਲ VFileLibName& rhs) ਅਚਲ {
        ਵਾਪਸ m_filename == rhs.m_filename && m_libname == rhs.m_libname;
    }
    bool ਸੰਚਾਲਕ<(ਅਚਲ VFileLibName& rhs) ਅਚਲ {
        ਜੇ (m_filename < rhs.m_filename) ਵਾਪਸ true;
        ਜੇ (m_filename > rhs.m_filename) ਵਾਪਸ false;
        ਵਾਪਸ m_libname < rhs.m_libname;
    }
};

using VFileLibList = std::ਸਦਿਸ਼<VFileLibName>;
using VFile_Lib_ਸੈੱਟ = std::ਸੈੱਟ<VFileLibName>;

// ######################################################################

class VTimescale final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        // clang-format off
        TS_100S = 0, TS_10S = 1, TS_1S = 2,
        TS_100MS = 3, TS_10MS = 4, TS_1MS = 5,
        TS_100US = 6, TS_10US = 7, TS_1US = 8,
        TS_100NS = 9, TS_10NS = 10, TS_1NS = 11,
        TS_100PS = 12, TS_10PS = 13, TS_1PS = 14,
        TS_100FS = 15, TS_10FS = 16, TS_1FS = 17,
        // clang-format on
        NONE = 18,
        ENUM_ਸਿਰਾ
    };
    ਗਿਣਤੀ : uint8_t { TS_DEFAULT = TS_1PS };
    ਗਿਣਤੀ en m_e;
    // CONSTRUCTOR
    VTimescale()
        : m_e{NONE} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VTimescale(en _e)
        : m_e{_e} {}
    explicit VTimescale(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    // Construct from string
    VTimescale(ਅਚਲ string& ਮੁੱਲ, bool& badr);
    VTimescale(ਦੁੱਗਣਾ ਮੁੱਲ, bool& badr) {
        badr = false;
        ਲਈ (ਪੂਰਨ i = TS_100S; i < ENUM_ਸਿਰਾ; ++i) {
            m_e = static_cast<en>(i);
            ਜੇ (multiplier() == ਮੁੱਲ) ਰੋਕੋ;
        }
        ਜੇ (multiplier() != ਮੁੱਲ) {
            m_e = NONE;
            badr = true;
        }
    }
    bool isNone() ਅਚਲ { ਵਾਪਸ m_e == NONE; }
    // Parse a "unit/precision" string into two VTimescales, with error checking
    ਸਥਿਰ ਖਾਲੀ parseSlashed(File_ਸਤਰ* fl, ਅਚਲ ਅੱਖਰ* textp, VTimescale& unitr, VTimescale& precr,
                             bool allow_ਖਾਲੀ_ਥਾਵਾਂ = false);
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[]
            = {"100s", "10s", "1s",    "100ms", "10ms", "1ms",   "100us", "10us", "1us", "100ns",
               "10ns", "1ns", "100ps", "10ps",  "1ps",  "100fs", "10fs",  "1fs",  "NONE"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    ਪੂਰਨ power_ਦਾ_Ten() ਅਚਲ { ਵਾਪਸ 2 - static_cast<ਪੂਰਨ>(m_e); }
    ਦੁੱਗਣਾ multiplier() ਅਚਲ {
        ਸਥਿਰ ਅਚਲ ਦੁੱਗਣਾ ਮੁੱਲਾਂ[]
            = {100,  10,   1,     1e-1,  1e-2,  1e-3,  1e-4,  1e-5,  1e-6, 1e-7,
               1e-8, 1e-9, 1e-10, 1e-11, 1e-12, 1e-13, 1e-14, 1e-15, 0};
        ਵਾਪਸ ਮੁੱਲਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VTimescale& lhs, ਅਚਲ VTimescale& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VTimescale& lhs, VTimescale::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(VTimescale::en lhs, ਅਚਲ VTimescale& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
// Comparisons are based on time, not enum values, so seconds > milliseconds
constexpr bool ਸੰਚਾਲਕ<(ਅਚਲ VTimescale& lhs, ਅਚਲ VTimescale& rhs) {
    ਵਾਪਸ lhs.m_e > rhs.m_e;
}
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VTimescale& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

class VFsmExpandType final {
    // Defined here since it is used in V3Options which does not include V3Ast.h
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t {
        DEFAULT = 0,  // Use the global default expansion type
        AUTO,  // Expands only reset and default
        AUTO_EXPAND,  // Expands transitions between all possible states
        ਭਰਿਆ,  // Cover all possible transitions
    } m_e;
    VFsmExpandType()
        : m_e{DEFAULT} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VFsmExpandType(en _e)
        : m_e{_e} {}
    explicit VFsmExpandType(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    constexpr ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ {
        constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {"DEFAULT", "AUTO", "AUTO_EXPAND", "FULL"};
        ਵਾਪਸ ਨਾਂ[m_e];
    }
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VFsmExpandType& lhs, ਅਚਲ VFsmExpandType& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VFsmExpandType& lhs, VFsmExpandType::en rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VFsmExpandType::en lhs, ਅਚਲ VFsmExpandType& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ lhs == rhs.m_e;
}
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ VFsmExpandType& rhs) VL_MT_ਸੁਰੱਖਿਅਤ {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

// Information given by --hierarchical-block option
class V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option final {
ਜਨਤਕ:
    // key:parameter name, value:value (as string)
    using ParamStrMap = std::map<ਅਚਲ std::string, std::string>;

ਨਿਜੀ:
    string m_origName;  // module name
    // module name after uniquified
    // same as m_origName for non-parameterized module
    string m_mangledName;
    // overriding parameter values specified by -G option
    ParamStrMap m_parameters;

ਜਨਤਕ:
    explicit V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option(ਅਚਲ string& optstring);
    ਅਚਲ string& origName() ਅਚਲ { ਵਾਪਸ m_origName; }
    ਅਚਲ string& mangledName() ਅਚਲ { ਵਾਪਸ m_mangledName; }
    ਅਚਲ ParamStrMap params() ਅਚਲ { ਵਾਪਸ m_parameters; }
};

using V3_Hier_ਖੰਡ_Opt_ਸੈੱਟ = std::map<ਅਚਲ std::string, V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option>;

//######################################################################
// V3Options - Command line options

class V3Options final {
ਜਨਤਕ:
ਨਿਜੀ:
    // TYPES
    using DebugLevelMap = std::map<ਅਚਲ std::string, ਬਿਨਾਨਿਸ਼ਾਨੀ>;

    // MEMBERS (general options)
    V3OptionsImp* m_impp;  // Slow hidden options

    // clang-format off
    VString_ਸੈੱਟ m_cppFiles;     // argument: C++ files to link against
    VStringList m_cFlags;      // argument: user CFLAGS
    VStringList m_ldLibs;      // argument: user LDFLAGS
    VStringList m_ldLibFiles;  // argument: library files, subset of m_ldLibs
    VStringList m_makeFlags;   // argument: user MAKEFLAGS
    VString_ਸੈੱਟ m_compilerIncludes; // argument: user --compiler-include
    VString_ਸੈੱਟ m_futures;      // argument: -Wfuture- list
    VString_ਸੈੱਟ m_future0s;     // argument: -future list
    VString_ਸੈੱਟ m_future1s;     // argument: -future1 list
    VFile_Lib_ਸੈੱਟ m_libraryFiles; // argument: Verilog -v files
    VFileLibList m_vFiles;      // argument: Verilog files to read
    VFile_Lib_ਸੈੱਟ m_vltFiles;     // argument: Verilator config files to read
    VString_ਸੈੱਟ m_libmapFiles;  // argument: --libmap files to read
    VStringList m_forceIncs;   // argument: -FI
    DebugLevelMap m_debugLevel; // argument: --debugi-<srcfile/tag> <level>
    DebugLevelMap m_dumpLevel;  // argument: --dumpi-<srcfile/tag> <level>
    std::map<ਅਚਲ string, string> m_parameters;  // Parameters
    std::map<ਅਚਲ string, V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option> m_hier_ਖੰਡਾਂ;  // main switch: --hierarchical-block
    VString_ਸੈੱਟ m_fDfgPeepholeDisabled; // argument: -f[no-]dfg-peephole-<name>

    bool m_preprocOnly = false;     // main switch: -E
    bool m_preprocResolve = false;  // main switch: --preproc-resolve
    bool m_makePhony = false;       // main switch: -MP
    bool m_preproc_No_ਸਤਰ = false;   // main switch: -P
    bool m_assert = true;           // main switch: --assert
    bool m_assertCase = true;       // main switch: --assert-case
    bool m_autoflush = false;       // main switch: --autoflush
    bool m_bboxSys = false;         // main switch: --bbox-sys
    bool m_bboxUnsup = false;       // main switch: --bbox-unsup
    bool m_ਦੁਆਧਾਰੀ = false;          // main switch: --binary
    bool m_build = false;           // main switch: --build
    bool m_context = true;          // main switch: --Wcontext
    bool m_coverageBranch = false;  // main switch: --coverage-branch
    bool m_coverage_ਵਿਅੰਜਕ = false;    // main switch: --coverage-expr
    bool m_coverageFsm = false;     // main switch: --coverage-fsm
    bool m_coverage_ਸਤਰ = false;    // main switch: --coverage-block
    bool m_coverage_ਮਿਲਾਓ_Instances = false;  // main switch: --coverage-merge-instances
    bool m_coverage_ਪ੍ਰਤੀ_Instance = false;  // main switch: --coverage-per-instance
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
    bool m_debug_ਹਿੱਸੇ_ਕਰੋ = false;  // main switch: --debug-partition
    bool m_debugPreprocPassthru = false;  // main switch: --debug-preproc-passthru
    bool m_debugProtect = false;    // main switch: --debug-protect
    bool m_debugSelfTest = false;   // main switch: --debug-self-test
    bool m_debug_ਢੇਰ_Check = false;  // main switch: --debug-stack-check
    ਪੂਰਨ m_debugRuntimeTimeout = 0;  // main switch: --debug-runtime-timeout <n>
    bool m_debug_ਚੌੜਾਈ = false;      // main switch: --debug-width
    bool m_decoration = true;       // main switch: --decoration
    bool m_decorationNodes = false;  // main switch: --decoration=nodes
    bool m_diagnosticsSarif = false;  // main switch: --diagnostics-sarif
    bool m_dpiHdrOnly = false;      // main switch: --dpi-hdr-only
    bool m_emitAccessors = false;   // main switch: --emit-accessors
    bool m_exe = false;             // main switch: --exe
    bool m_flatten = false;         // main switch: --flatten
    bool m_fourstate = false;       // main switch: --fourstate
    bool m_ਦਰਜਾਵਾਰ = false;    // main switch: --hierarchical
    bool m_ignc = false;            // main switch: --ignc
    bool m_jsonOnly = false;        // main switch: --json-only
    bool m_lintOnly = false;        // main switch: --lint-only
    bool m_gmake = false;           // main switch: --make gmake
    bool m_makeJson = false;        // main switch: --make json
    bool m_main = false;            // main switch: --main
    bool m_ਬਾਹਰਲਾ_Format_Ok = false;     // main switch: --cc, --sc or --sp was specified
    bool m_pedantic = false;        // main switch: --Wpedantic
    bool m_pinsInoutEnables = false;// main switch: --pins-inout-enables
    bool m_pinsScUint = false;      // main switch: --pins-sc-uint
    bool m_pinsScUintBool = false;  // main switch: --pins-sc-uint-bool
    bool m_pinsScBigUint = false;   // main switch: --pins-sc-biguint
    bool m_pinsUint8 = false;       // main switch: --pins-uint8
    bool m_preprocComments = false;  // main switch: --preproc-comments
    bool m_preproc_ਨਿਰਧਾਰਤ_ਕਰਦਾ = false;  // main switch: --preproc-defines
    bool m_profC = false;           // main switch: --prof-c
    bool m_profCFuncs = false;      // main switch: --prof-cfuncs
    bool m_profExec = false;        // main switch: --prof-exec
    bool m_profPgo = false;         // main switch: --prof-pgo
    bool m_protectIds = false;      // main switch: --protect-ids
    bool m_ਜਨਤਕ = false;          // main switch: --public
    bool m_ਜਨਤਕ_Flat_RW = false;    // main switch: --public-flat-rw
    bool m_ਜਨਤਕ_Ignore = false;    // main switch: --public-ignore
    bool m_ਜਨਤਕ_Params = false;    // main switch: --public-params
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
    bool m_ਧਾਗੇ_Coarsen = true;   // main switch: --threads-coarsen
    bool m_ਧਾਗੇ_Dpi_ਸ਼ੁੱਧ = true;   // main switch: --threads-dpi all/pure
    bool m_ਧਾਗੇ_Dpi_Unpure = false;  // main switch: --threads-dpi all
    VOptionBool m_timing;           // main switch: --timing
    bool m_trace = false;           // main switch: --trace
    bool m_traceCoverage = false;   // main switch: --trace-coverage
    bool m_traceEnabledFst = false;  // main switch: --trace-fst
    bool m_traceEnabledSaif = false;  // main switch: --trace-saif
    bool m_traceEnabledVcd = false;  // main switch: --trace-vcd
    bool m_traceParams = true;      // main switch: --trace-params
    bool m_traceStructs = false;    // main switch: --trace-structs
    bool m_no_Trace_ਸਿਖਰ = false;      // main switch: --no-trace-top
    bool m_traceUnderscore = false; // main switch: --trace-underscore
    bool m_underlineZero = false;   // main switch: --underline-zero; undocumented old Verilator 2
    bool m_verilate = true;         // main switch: --verilate
    VOptionBool m_vpi;              // main switch: --vpi
    bool m_waiverMultiline = false;  // main switch: --waiver-multiline
    bool m_x_Initial_ਕਿਨਾਰਾ = false;    // main switch: --x-initial-edge

    ਪੂਰਨ         m_buildJobs = -1;    // main switch: --build-jobs, -j
    ਪੂਰਨ         m_coverage_ਵਿਅੰਜਕ_Max = 32;    // main switch: --coverage-expr-max
    VFsmExpandType m_coverageFsmExpand;  // main switch: --coverage-fsm-expand <mode>
    ਪੂਰਨ         m_coverageFsmMaxArcs = 4096;  // main switch: --coverage-fsm-max-arcs
    ਪੂਰਨ         m_converge_ਹੱਦਬੰਦੀ = 10000;  // main switch: --converge-limit
    uint32_t    m_coverageMaxBins = 1024;  // main switch: --coverage-max-bins
    uint32_t    m_coverageMaxRealBins = 1024;  // main switch: --coverage-max-real-bins
    ਪੂਰਨ         m_coverage_Max_ਚੌੜਾਈ = 256; // main switch: --coverage-max-width
    ਪੂਰਨ         m_debugAllocRandom = 0;  // main switch: --debug-alloc-random <seed>
    ਪੂਰਨ         m_expand_ਹੱਦਬੰਦੀ = 256;  // main switch: --expand-limit
    ਪੂਰਨ         m_gateStmts = 100;    // main switch: --gate-stmts
    ਪੂਰਨ         m_hier_ਬੱਚਾ = 0;      // main switch: --hierarchical-child
    ਪੂਰਨ         m_hier_ਧਾਗੇ = 0;      // main switch: --hierarchical-threads
    ਪੂਰਨ         m_if_ਡੂੰਘਾਈ = 0;      // main switch: --if-depth
    ਪੂਰਨ         m_inlineCFuncs = 20;   // main switch: --inline-cfuncs
    ਪੂਰਨ         m_inline_CFuncs_ਗੁਣਨਫਲ = 200;   // main switch: --inline-cfuncs-product
    ਪੂਰਨ         m_inlineMult = 2000;   // main switch: --inline-mult
    ਪੂਰਨ         m_instr_ਗਿਣਤੀ_Dpi = 200;   // main switch: --instr-count-dpi
    bool        m_jsonEditNums = true; // main switch: --no-json-edit-nums
    bool        m_jsonIds = true; // main switch: --no-json-ids
    ਪੂਰਨ         m_localize_Max_ਮਾਪ = 1024;  // main switch: --localize-max-size
    VOptionBool m_makeDepend;  // main switch: -MMD
    ਪੂਰਨ         m_max_ਅੰਕ_ਚੌੜਾਈ = 65536;  // main switch: --max-num-width
    ਪੂਰਨ         m_funcRecursion = 1000;  // main switch: --func-recursion-depth
    ਪੂਰਨ         m_moduleRecursion = 100;  // main switch: --module-recursion-depth
    ਪੂਰਨ         m_outputGroups = -1;  // main switch: --output-groups
    int64_t     m_output_Groups_ਮਾਪ = 512 * 1024 * 1024;  // main switch: --output-groups-size
    ਪੂਰਨ         m_output_ਵੰਡੋ = 20000;  // main switch: --output-split
    ਪੂਰਨ         m_output_ਵੰਡੋ_CFuncs = -1;  // main switch: --output-split-cfuncs
    ਪੂਰਨ         m_output_ਵੰਡੋ_CTrace = -1;  // main switch: --output-split-ctrace
    ਪੂਰਨ         m_pinsBv = 65;       // main switch: --pins-bv
    ਪੂਰਨ         m_preproc_Token_ਹੱਦਬੰਦੀ = 40000; // main switch: --preproc-token-limit
    ਪੂਰਨ         m_ਜਨਤਕ_ਡੂੰਘਾਈ = 0;   // main switch: --public-depth
    ਪੂਰਨ         m_reloop_ਹੱਦਬੰਦੀ = 40; // main switch: --reloop-limit
    ਪੂਰਨ         m_replication_ਹੱਦਬੰਦੀ = 8192; // main switch: --replication-limit
    VOptionBool m_skipIdentical;  // main switch: --skip-identical
    bool        m_stopFail = true;  // main switch: --stop-fail
    ਪੂਰਨ         m_ਧਾਗੇ = 1;      // main switch: --threads
    ਪੂਰਨ         m_ਧਾਗੇ_Max_MTasks = 0;  // main switch: --threads-max-mtasks
    VTimescale  m_ਸਮਾਂ_Default_Prec;  // main switch: --timescale
    VTimescale  m_ਸਮਾਂ_Default_Unit;  // main switch: --timescale
    VTimescale  m_ਸਮਾਂ_Override_Prec;  // main switch: --timescale-override
    VTimescale  m_ਸਮਾਂ_Override_Unit;  // main switch: --timescale-override
    ਪੂਰਨ         m_trace_ਡੂੰਘਾਈ = 0;   // main switch: --trace-depth
    ਪੂਰਨ         m_traceMaxArray = 32;  // main switch: --trace-max-array
    ਪੂਰਨ         m_trace_Max_ਚੌੜਾਈ = 4096; // main switch: --trace-max-width
    ਪੂਰਨ         m_unroll_ਗਿਣਤੀ = 64;  // main switch: --unroll-count
    ਪੂਰਨ         m_unroll_ਹੱਦਬੰਦੀ = 16384;  // main switch: --unroll-limit
    ਪੂਰਨ         m_unrollStmts = 30000;  // main switch: --unroll-stmts
    ਪੂਰਨ         m_constraint_Array_ਹੱਦਬੰਦੀ = 64;  // main switch: --constraint-array-limit
    ਪੂਰਨ         m_verilateJobs = -1;  // main switch: --verilate-jobs

    ਪੂਰਨ         m_comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ = 0;  // compiler selection; number of nested blocks
    ਪੂਰਨ         m_comp_ਹੱਦਬੰਦੀ_Members = 64;  // compiler selection; number of members in struct before make anon array
    ਪੂਰਨ         m_comp_ਹੱਦਬੰਦੀ_Parens = 240;  // compiler selection; number of nested parens

    string      m_buildDepBin;  // main switch: --build-dep-bin {filename}
    string      m_diagnosticsSarifOutput;  // main switch: --diagnostics-sarif-output
    string      m_exeName;      // main switch: -o {name}
    VFileLibList m_hierParamsFile; // main switch: --hierarchical-params-file
    string      m_jsonOnlyOutput;    // main switch: --json-only-output
    string      m_jsonOnlyMetaOutput;    // main switch: --json-only-meta-output
    string      m_l2Name;       // main switch: --l2name; "" for top-module's name
    string      m_libCreate;    // main switch: --lib-create {lib_name}
    string      m_main_ਸਿਖਰ_Name;  // main switch: --main-top-name
    string      m_makeDir;      // main switch: -Mdir
    string      m_mod_ਅਗੇਤਰ;    // main switch: --mod-prefix
    string      m_pipeFilter;   // main switch: --pipe-filter
    string      m_ਅਗੇਤਰ;       // main switch: --prefix
    string      m_protect_ਕੁੰਜੀ;   // main switch: --protect-key
    string      m_ਸਿਖਰ_Module;    // main switch: --top-module
    string      m_ਅਣਵਰਤਿਆ_Regexp; // main switch: --unused-regexp
    string      m_waiverOutput;  // main switch: --waiver-output {filename}
    string      m_work = "work";  // main switch: --work {libname}
    string      m_x_ਸੌਂਪੋ;      // main switch: --x-assign
    string      m_xInitial;     // main switch: --x-initial

    // Language is now held in FileLine, on a per-node basis. However we still
    // have a concept of the default language at a global level.
    V3_Lang_ਕੋਡ  m_defaultLanguage;      // main switch: --language

    // MEMBERS (optimizations)
    bool m_fAcycSimp;    // main switch: -fno-acyc-simp: acyclic pre-optimizations
    bool m_fAssemble;    // main switch: -fno-assemble: assign assemble
    bool m_f_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Loops;  // main switch: -fno-bit-scan-loops: convert bit scan loops to builtins
    bool m_fBitblast;    // main switch: -fno-bitblast: split packed variables into bit ranges
    bool m_fCaseDecoder; // main switch: -fno-case-decoder: case decoder conversion
    bool m_f_Case_ਸਾਰਣੀ;   // main switch: -fno-case-table: case table conversion
    bool m_f_Case_ਰੁੱਖ;    // main switch: -fno-case-tree: case tree conversion
    bool m_fCombine;     // main switch: -fno-combine: common icode packing
    bool m_fConst;       // main switch: -fno-const: constant folding
    bool m_fConstBeforeDfg = true;  // main switch: -fno-const-before-dfg for testing only!
    bool m_f_Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ;  // main switch: -fno-const-bit-op-tree constant bit op tree
    bool m_fConstEager = true;  // main switch: -fno-const-eagerly run V3Const during passes
    bool m_fDedupe;      // main switch: -fno-dedupe: logic deduplication
    bool m_fDfgPeephole = true; // main switch: -fno-dfg-peephole
    bool m_f_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels = true; // main switch: -fno-dfg-push-down-sels
    bool m_fDfg;         // main switch: -fno-dfg
    bool m_fDfgSynthesizeAll = false;  // main switch: -fdfg-synthesize-all
    bool m_fDeadAssigns;     // main switch: -fno-dead-assigns: remove dead assigns
    bool m_fDeadCells;   // main switch: -fno-dead-cells: remove dead cells
    bool m_fDeadMethods;   // main switch: -fno-dead-methods: remove dead methods
    bool m_fDecomposePacked;  // main switch: -fno-decompose-packed: packed decomposition
    bool m_fDecomposeUnpacked;  // main switch: -fno-decompose-unpacked: unpacked decomposition
    bool m_fExpand;      // main switch: -fno-expand: expansion of C macros
    bool m_fFuncBalanceCat = true;  // main switch: -fno-func-balance-cat: expansion of C macros
    bool m_f_Func_ਵੰਡੋ_Cat = true;  // main switch: -fno-func-split-cat: expansion of C macros
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
    bool m_f_ਮਿਲਾਓ_Cond;   // main switch: -fno-merge-cond: merge conditionals
    bool m_f_ਮਿਲਾਓ_Cond_Motion = true; // main switch: -fno-merge-cond-motion: perform code motion
    bool m_fReloop;      // main switch: -fno-reloop: reform loops
    bool m_fReorder;     // main switch: -fno-reorder: reorder assignments in blocks
    bool m_f_ਟੁਕੜਾ = true;  // main switch: -fno-slice: array assignment slicing
    ਪੂਰਨ  m_f_ਟੁਕੜਾ_Element_ਹੱਦਬੰਦੀ = 256;  // main switch: --fslice-element-limit
    bool m_f_ਵੰਡੋ;       // main switch: -fno-split: always assignment splitting
    bool m_fSubst;       // main switch: -fno-subst: substitute expression temp values
    bool m_fSubstConst;  // main switch: -fno-subst-const: final constant substitution
    bool m_f_ਸਾਰਣੀ;       // main switch: -fno-table: lookup table creation
    bool m_fTaskifyAll = false;  // main switch: --ftaskify-all-forked
    // clang-format on

    bool m_ਉਪਲਬਧ = false;  // Set to true at the end of option parsing

    // METHODS
    ਖਾਲੀ ਜੋੜੋ_Arg(ਅੱਖਰ** argv, ਮਾਪ_t ਗਿਣਤੀ_2, bool isForRerun);
    ਖਾਲੀ ਜੋੜੋ_Arg(ਅਚਲ std::string& arg, bool isForRerun);
    ਖਾਲੀ ਜੋੜੋ_ਸਤਰ_Arg(ਅਚਲ string& arg);
    ਖਾਲੀ ਜੋੜੋ_Define(ਅਚਲ string& defline, bool allowPlus) VL_MT_DISABLED;
    ਖਾਲੀ ਜੋੜੋ_Future(ਅਚਲ string& flag);
    ਖਾਲੀ ਜੋੜੋ_Future0(ਅਚਲ string& flag);
    ਖਾਲੀ ਜੋੜੋ_Future1(ਅਚਲ string& flag);
    ਖਾਲੀ ਜੋੜੋ_Inc_Dir_User(ਅਚਲ string& incdir);  // User requested
    ਖਾਲੀ ਜੋੜੋ_Inc_Dir_Fallback(ਅਚਲ string& incdir);  // Low priority if not found otherwise
    ਖਾਲੀ ਜੋੜੋ_Parameter(ਅਚਲ string& paramline, bool allowPlus);
    ਖਾਲੀ ਜੋੜੋ_Lang_Ext(ਅਚਲ string& langext, ਅਚਲ V3_Lang_ਕੋਡ& lc);
    ਖਾਲੀ ਜੋੜੋ_Lib_Ext_V(ਅਚਲ string& libext);
    ਖਾਲੀ optimize(ਪੂਰਨ level);
    ਖਾਲੀ show_ਸੰਸਕਰਣ(bool verbose);
    ਖਾਲੀ coverage(bool flag) {
        m_coverage_ਸਤਰ = m_coverageBranch = m_coverageToggle = m_coverage_ਵਿਅੰਜਕ = m_coverageFsm
            = m_coverageUser = flag;
    }
    ਸਥਿਰ bool suffixed(ਅਚਲ string& sw, ਅਚਲ ਅੱਖਰ* arg);
    ਸਥਿਰ string parseFileArg(ਅਚਲ string& optdir, ਅਚਲ string& relfilename);
    string file_ਰਸਤਾ_Check_One_Dir(ਅਚਲ string& modname, ਅਚਲ string& dirname);
    ਸਥਿਰ ਪੂਰਨ strip_Options_For_ਬੱਚਾ_ਲੜੀਵਾਰ(ਅਚਲ string& opt, bool for_ਸਿਖਰ);
    ਖਾਲੀ validateIdentifier(File_ਸਤਰ* fl, ਅਚਲ string& arg, ਅਚਲ string& opt);

    // CONSTRUCTORS
    VL_UNCOPYABLE(V3Options);

ਜਨਤਕ:
    V3Options();
    ~V3Options();
    ਖਾਲੀ ਸੈੱਟ_Debug_ਬਹੁਲਕ(ਪੂਰਨ level);
    ਬਿਨਾਨਿਸ਼ਾਨੀ debugLevel(ਅਚਲ string& ਟੈਗ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    ਬਿਨਾਨਿਸ਼ਾਨੀ debugSrcLevel(ਅਚਲ string& srcfile_ਰਸਤਾ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    ਬਿਨਾਨਿਸ਼ਾਨੀ dumpLevel(ਅਚਲ string& ਟੈਗ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    ਬਿਨਾਨਿਸ਼ਾਨੀ dumpSrcLevel(ਅਚਲ string& srcfile_ਰਸਤਾ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;

    // METHODS
    ਖਾਲੀ ਜੋੜੋ_Cpp_File(ਅਚਲ string& filename);
    ਖਾਲੀ ਜੋੜੋ_CFlags(ਅਚਲ string& filename);
    ਖਾਲੀ ਜੋੜੋ_Compiler_Includes(ਅਚਲ string& filename);
    ਖਾਲੀ ਜੋੜੋ_Ld_Lib_File(ਅਚਲ string& filename);
    ਖਾਲੀ ਜੋੜੋ_Ld_Libs(ਅਚਲ string& filename);
    ਖਾਲੀ ਜੋੜੋ_Make_Flags(ਅਚਲ string& filename);
    ਖਾਲੀ ਜੋੜੋ_Lib_Map_File(ਅਚਲ string& filename);
    ਖਾਲੀ ਜੋੜੋ_Library_File(ਅਚਲ string& filename, ਅਚਲ string& libname);
    ਖਾਲੀ ਜੋੜੋ_VFile(ਅਚਲ string& filename, ਅਚਲ string& libname);
    ਖਾਲੀ ਜੋੜੋ_Vlt_File(ਅਚਲ string& filename, ਅਚਲ string& libname);
    ਖਾਲੀ ਜੋੜੋ_Force_Inc(ਅਚਲ string& filename);
    bool ਉਪਲਬਧ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਉਪਲਬਧ; }
    ਖਾਲੀ cc_ਸੈੱਟ();
    ਖਾਲੀ decorations(File_ਸਤਰ* fl, ਅਚਲ string& filename);
    ਖਾਲੀ notify() VL_MT_DISABLED;

    // ACCESSORS (options)
    bool preprocOnly() ਅਚਲ { ਵਾਪਸ m_preprocOnly; }
    bool makePhony() ਅਚਲ { ਵਾਪਸ m_makePhony; }
    bool preproc_No_ਸਤਰ() ਅਚਲ { ਵਾਪਸ m_preproc_No_ਸਤਰ; }
    bool preprocResolve() ਅਚਲ { ਵਾਪਸ m_preprocResolve; }
    ਪੂਰਨ preproc_Token_ਹੱਦਬੰਦੀ() ਅਚਲ { ਵਾਪਸ m_preproc_Token_ਹੱਦਬੰਦੀ; }
    bool underlineZero() ਅਚਲ { ਵਾਪਸ m_underlineZero; }
    bool systemC() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_systemC; }
    bool savable() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_savable; }
    VOptionBool schedZeroDelay() ਅਚਲ { ਵਾਪਸ m_schedZeroDelay; }
    bool stats() ਅਚਲ { ਵਾਪਸ m_stats; }
    bool statsVars() ਅਚਲ { ਵਾਪਸ m_statsVars; }
    bool stdPackage() ਅਚਲ { ਵਾਪਸ m_stdPackage; }
    bool stdWaiver() ਅਚਲ { ਵਾਪਸ m_stdWaiver; }
    bool assertOn() ਅਚਲ { ਵਾਪਸ m_assert; }  // assertOn as __FILE__ may be defined
    bool assertCase() ਅਚਲ { ਵਾਪਸ m_assertCase; }
    bool autoflush() ਅਚਲ { ਵਾਪਸ m_autoflush; }
    bool bboxSys() ਅਚਲ { ਵਾਪਸ m_bboxSys; }
    bool bboxUnsup() ਅਚਲ { ਵਾਪਸ m_bboxUnsup; }
    bool build() ਅਚਲ { ਵਾਪਸ m_build; }
    string buildDepBin() ਅਚਲ { ਵਾਪਸ m_buildDepBin; }
    ਖਾਲੀ buildDepBin(ਅਚਲ string& flag) { m_buildDepBin = flag; }
    bool context() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_context; }
    bool coverage() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        // Any enabled coverage kind, including FSM coverage. Code generation
        // and runtime support should generally query this accessor.
        ਵਾਪਸ m_coverage_ਸਤਰ || m_coverageBranch || m_coverageToggle || m_coverage_ਵਿਅੰਜਕ
               || m_coverageUser || m_coverageFsm;
    }
    bool coverageNonFsm() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        // The broad line/toggle/expr/user coverage transforms use this
        // accessor. FSM coverage shares the overall coverage umbrella, but its
        // extraction still happens through a separate early-recognition path.
        ਵਾਪਸ m_coverage_ਸਤਰ || m_coverageBranch || m_coverageToggle || m_coverage_ਵਿਅੰਜਕ
               || m_coverageUser;
    }
    bool coverageBranch() ਅਚਲ { ਵਾਪਸ m_coverageBranch; }
    bool coverage_ਵਿਅੰਜਕ() ਅਚਲ { ਵਾਪਸ m_coverage_ਵਿਅੰਜਕ; }
    bool coverageFsm() ਅਚਲ { ਵਾਪਸ m_coverageFsm; }
    VFsmExpandType coverageFsmExpand() ਅਚਲ { ਵਾਪਸ m_coverageFsmExpand; }
    ਪੂਰਨ coverageFsmMaxArcs() ਅਚਲ { ਵਾਪਸ m_coverageFsmMaxArcs; }
    bool coverage_ਸਤਰ() ਅਚਲ { ਵਾਪਸ m_coverage_ਸਤਰ; }
    bool coverage_ਮਿਲਾਓ_Instances() ਅਚਲ { ਵਾਪਸ m_coverage_ਮਿਲਾਓ_Instances; }
    bool coverage_ਪ੍ਰਤੀ_Instance() ਅਚਲ { ਵਾਪਸ m_coverage_ਪ੍ਰਤੀ_Instance; }
    bool coverageToggle() ਅਚਲ { ਵਾਪਸ m_coverageToggle; }
    bool coverageUnderscore() ਅਚਲ { ਵਾਪਸ m_coverageUnderscore; }
    bool coverageUser() ਅਚਲ { ਵਾਪਸ m_coverageUser; }
    bool debugCheck() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_debugCheck; }
    bool debugCollision() ਅਚਲ { ਵਾਪਸ m_debugCollision; }
    bool debugEmitV() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_debugEmitV; }
    bool debugExitElab() ਅਚਲ { ਵਾਪਸ m_debugExitElab; }
    bool debugExitParse() ਅਚਲ { ਵਾਪਸ m_debugExitParse; }
    bool debugLeak() ਅਚਲ { ਵਾਪਸ m_debugLeak; }
    bool debugNondeterminism() ਅਚਲ { ਵਾਪਸ m_debugNondeterminism; }
    bool debug_ਹਿੱਸੇ_ਕਰੋ() ਅਚਲ { ਵਾਪਸ m_debug_ਹਿੱਸੇ_ਕਰੋ; }
    bool debugPreprocPassthru() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_debugPreprocPassthru; }
    bool debugProtect() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_debugProtect; }
    ਪੂਰਨ debugRuntimeTimeout() ਅਚਲ { ਵਾਪਸ m_debugRuntimeTimeout; }
    bool debugSelfTest() ਅਚਲ { ਵਾਪਸ m_debugSelfTest; }
    bool debug_ਢੇਰ_Check() ਅਚਲ { ਵਾਪਸ m_debug_ਢੇਰ_Check; }
    bool debug_ਚੌੜਾਈ() ਅਚਲ VL_ਸ਼ੁੱਧ { ਵਾਪਸ m_debug_ਚੌੜਾਈ; }
    bool decoration() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_decoration; }
    bool decorationNodes() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_decorationNodes; }
    bool diagnosticsSarif() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_diagnosticsSarif; }
    bool dpiHdrOnly() ਅਚਲ { ਵਾਪਸ m_dpiHdrOnly; }
    bool dumpAstPatterns() ਅਚਲ {
        ਵਾਪਸ m_dumpLevel.ਗਿਣਤੀ_2("ast-patterns") && m_dumpLevel.at("ast-patterns");
    }
    bool dump_ਨਿਰਧਾਰਤ_ਕਰਦਾ() ਅਚਲ { ਵਾਪਸ m_dumpLevel.ਗਿਣਤੀ_2("defines") && m_dumpLevel.at("defines"); }
    bool dumpDfgPatterns() ਅਚਲ {
        ਵਾਪਸ m_dumpLevel.ਗਿਣਤੀ_2("dfg-patterns") && m_dumpLevel.at("dfg-patterns");
    }
    bool dump_ਰੁੱਖ_Dot() ਅਚਲ {
        ਵਾਪਸ m_dumpLevel.ਗਿਣਤੀ_2("tree-dot") && m_dumpLevel.at("tree-dot");
    }
    bool emitAccessors() ਅਚਲ { ਵਾਪਸ m_emitAccessors; }
    bool exe() ਅਚਲ { ਵਾਪਸ m_exe; }
    bool flatten() ਅਚਲ { ਵਾਪਸ m_flatten; }
    bool fourstate() ਅਚਲ { ਵਾਪਸ m_fourstate; }
    bool gmake() ਅਚਲ { ਵਾਪਸ m_gmake; }
    bool makeJson() ਅਚਲ { ਵਾਪਸ m_makeJson; }
    bool ਧਾਗੇ_Dpi_ਸ਼ੁੱਧ() ਅਚਲ { ਵਾਪਸ m_ਧਾਗੇ_Dpi_ਸ਼ੁੱਧ; }
    bool ਧਾਗੇ_Dpi_Unpure() ਅਚਲ { ਵਾਪਸ m_ਧਾਗੇ_Dpi_Unpure; }
    bool ਧਾਗੇ_Coarsen() ਅਚਲ { ਵਾਪਸ m_ਧਾਗੇ_Coarsen; }
    VOptionBool timing() ਅਚਲ { ਵਾਪਸ m_timing; }
    bool trace() ਅਚਲ { ਵਾਪਸ m_trace; }
    bool traceCoverage() ਅਚਲ { ਵਾਪਸ m_traceCoverage; }
    bool traceEnabledFst() ਅਚਲ { ਵਾਪਸ m_traceEnabledFst; }
    bool traceEnabledSaif() ਅਚਲ { ਵਾਪਸ m_traceEnabledSaif; }
    bool traceEnabledVcd() ਅਚਲ { ਵਾਪਸ m_traceEnabledVcd; }
    bool traceParams() ਅਚਲ { ਵਾਪਸ m_traceParams; }
    bool traceStructs() ਅਚਲ { ਵਾਪਸ m_traceStructs; }
    bool traceUnderscore() ਅਚਲ { ਵਾਪਸ m_traceUnderscore; }
    bool ਮੁੱਖ() ਅਚਲ { ਵਾਪਸ m_main; }
    bool ਬਾਹਰਲਾ_Format_Ok() ਅਚਲ { ਵਾਪਸ m_ਬਾਹਰਲਾ_Format_Ok; }
    bool jsonOnly() ਅਚਲ { ਵਾਪਸ m_jsonOnly; }
    bool keep_ਆਰਜ਼ੀ_Files() ਅਚਲ { ਵਾਪਸ (V3Error::debugDefault() != 0); }
    bool pedantic() ਅਚਲ { ਵਾਪਸ m_pedantic; }
    bool pinsInoutEnables() ਅਚਲ { ਵਾਪਸ m_pinsInoutEnables; }
    bool pinsScUint() ਅਚਲ { ਵਾਪਸ m_pinsScUint; }
    bool pinsScUintBool() ਅਚਲ { ਵਾਪਸ m_pinsScUintBool; }
    bool pinsScBigUint() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_pinsScBigUint; }
    bool pinsUint8() ਅਚਲ { ਵਾਪਸ m_pinsUint8; }
    bool preprocComments() ਅਚਲ { ਵਾਪਸ m_preprocComments; }
    bool preproc_ਨਿਰਧਾਰਤ_ਕਰਦਾ() ਅਚਲ { ਵਾਪਸ m_preproc_ਨਿਰਧਾਰਤ_ਕਰਦਾ; }
    bool profC() ਅਚਲ { ਵਾਪਸ m_profC; }
    bool profCFuncs() ਅਚਲ { ਵਾਪਸ m_profCFuncs; }
    bool profExec() ਅਚਲ { ਵਾਪਸ m_profExec; }
    bool profPgo() ਅਚਲ { ਵਾਪਸ m_profPgo; }
    bool ਵਰਤਦਾ_Profiler() ਅਚਲ { ਵਾਪਸ profExec() || profPgo(); }
    bool protectIds() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_protectIds; }
    bool all_ਜਨਤਕ() ਅਚਲ { ਵਾਪਸ m_ਜਨਤਕ; }
    bool ਜਨਤਕ_Params() ਅਚਲ { ਵਾਪਸ m_ਜਨਤਕ_Params; }
    bool ਜਨਤਕ_Off() ਅਚਲ { ਵਾਪਸ m_ਜਨਤਕ_Ignore; }
    bool ਜਨਤਕ_Flat_RW() ਅਚਲ { ਵਾਪਸ m_ਜਨਤਕ_Flat_RW; }
    ਪੂਰਨ ਜਨਤਕ_ਡੂੰਘਾਈ() ਅਚਲ { ਵਾਪਸ m_ਜਨਤਕ_ਡੂੰਘਾਈ; }
    bool any_ਜਨਤਕ_Flat() ਅਚਲ { ਵਾਪਸ m_ਜਨਤਕ_Params || m_ਜਨਤਕ_Flat_RW || m_ਜਨਤਕ_ਡੂੰਘਾਈ; }
    bool lintOnly() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_lintOnly; }
    bool ignc() ਅਚਲ { ਵਾਪਸ m_ignc; }
    bool quietBuild() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_quietBuild; }
    bool quietExit() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_quietExit; }
    bool quietStats() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_quietStats; }
    bool reportUnoptflat() ਅਚਲ { ਵਾਪਸ m_reportUnoptflat; }
    bool verilate() ਅਚਲ { ਵਾਪਸ m_verilate; }
    bool vpi() ਅਚਲ { ਵਾਪਸ m_vpi.isTrue(); }
    bool waiverMultiline() ਅਚਲ { ਵਾਪਸ m_waiverMultiline; }
    bool x_Initial_ਕਿਨਾਰਾ() ਅਚਲ { ਵਾਪਸ m_x_Initial_ਕਿਨਾਰਾ; }
    bool serializeOnly() ਅਚਲ { ਵਾਪਸ m_jsonOnly; }
    bool ਸਿਖਰ_Ifaces_Supported() ਅਚਲ { ਵਾਪਸ lintOnly() && !ਦਰਜਾਵਾਰ(); }

    ਪੂਰਨ buildJobs() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_buildJobs; }
    ਪੂਰਨ converge_ਹੱਦਬੰਦੀ() ਅਚਲ { ਵਾਪਸ m_converge_ਹੱਦਬੰਦੀ; }
    ਪੂਰਨ coverage_ਵਿਅੰਜਕ_Max() ਅਚਲ { ਵਾਪਸ m_coverage_ਵਿਅੰਜਕ_Max; }
    uint32_t coverageMaxBins() ਅਚਲ { ਵਾਪਸ m_coverageMaxBins; }
    uint32_t coverageMaxRealBins() ਅਚਲ { ਵਾਪਸ m_coverageMaxRealBins; }
    ਪੂਰਨ coverage_Max_ਚੌੜਾਈ() ਅਚਲ { ਵਾਪਸ m_coverage_Max_ਚੌੜਾਈ; }
    ਪੂਰਨ debugAllocRandom() ਅਚਲ { ਵਾਪਸ m_debugAllocRandom; }
    bool dump_ਰੁੱਖ_Addrids() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    ਪੂਰਨ expand_ਹੱਦਬੰਦੀ() ਅਚਲ { ਵਾਪਸ m_expand_ਹੱਦਬੰਦੀ; }
    ਪੂਰਨ gateStmts() ਅਚਲ { ਵਾਪਸ m_gateStmts; }
    ਪੂਰਨ if_ਡੂੰਘਾਈ() ਅਚਲ { ਵਾਪਸ m_if_ਡੂੰਘਾਈ; }
    ਪੂਰਨ inlineCFuncs() ਅਚਲ { ਵਾਪਸ m_inlineCFuncs; }
    ਪੂਰਨ inline_CFuncs_ਗੁਣਨਫਲ() ਅਚਲ { ਵਾਪਸ m_inline_CFuncs_ਗੁਣਨਫਲ; }
    ਪੂਰਨ inlineMult() ਅਚਲ { ਵਾਪਸ m_inlineMult; }
    ਪੂਰਨ instr_ਗਿਣਤੀ_Dpi() ਅਚਲ { ਵਾਪਸ m_instr_ਗਿਣਤੀ_Dpi; }
    ਪੂਰਨ localize_Max_ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_localize_Max_ਮਾਪ; }
    bool jsonEditNums() ਅਚਲ { ਵਾਪਸ m_jsonEditNums; }
    bool jsonIds() ਅਚਲ { ਵਾਪਸ m_jsonIds; }
    VOptionBool makeDepend() ਅਚਲ { ਵਾਪਸ m_makeDepend; }
    ਪੂਰਨ max_ਅੰਕ_ਚੌੜਾਈ() ਅਚਲ { ਵਾਪਸ m_max_ਅੰਕ_ਚੌੜਾਈ; }
    ਪੂਰਨ func_Recursion_ਡੂੰਘਾਈ() ਅਚਲ { ਵਾਪਸ m_funcRecursion; }
    ਪੂਰਨ module_Recursion_ਡੂੰਘਾਈ() ਅਚਲ { ਵਾਪਸ m_moduleRecursion; }
    ਪੂਰਨ output_ਵੰਡੋ() ਅਚਲ { ਵਾਪਸ m_output_ਵੰਡੋ; }
    ਪੂਰਨ output_ਵੰਡੋ_CFuncs() ਅਚਲ { ਵਾਪਸ m_output_ਵੰਡੋ_CFuncs; }
    ਪੂਰਨ output_ਵੰਡੋ_CTrace() ਅਚਲ { ਵਾਪਸ m_output_ਵੰਡੋ_CTrace; }
    ਪੂਰਨ outputGroups() ਅਚਲ { ਵਾਪਸ m_outputGroups; }
    int64_t output_Groups_ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_output_Groups_ਮਾਪ; }
    ਪੂਰਨ pinsBv() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_pinsBv; }
    ਪੂਰਨ reloop_ਹੱਦਬੰਦੀ() ਅਚਲ { ਵਾਪਸ m_reloop_ਹੱਦਬੰਦੀ; }
    ਪੂਰਨ replication_ਹੱਦਬੰਦੀ() ਅਚਲ { ਵਾਪਸ m_replication_ਹੱਦਬੰਦੀ; }
    VOptionBool skipIdentical() ਅਚਲ { ਵਾਪਸ m_skipIdentical; }
    bool stopFail() ਅਚਲ { ਵਾਪਸ m_stopFail; }
    ਪੂਰਨ ਧਾਗੇ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਧਾਗੇ; }
    ਪੂਰਨ ਧਾਗੇ_Max_MTasks() ਅਚਲ { ਵਾਪਸ m_ਧਾਗੇ_Max_MTasks; }
    bool mtasks() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ (m_ਧਾਗੇ > 1); }
    VTimescale ਸਮਾਂ_Default_Prec() ਅਚਲ { ਵਾਪਸ m_ਸਮਾਂ_Default_Prec; }
    VTimescale ਸਮਾਂ_Default_Unit() ਅਚਲ { ਵਾਪਸ m_ਸਮਾਂ_Default_Unit; }
    VTimescale ਸਮਾਂ_Override_Prec() ਅਚਲ { ਵਾਪਸ m_ਸਮਾਂ_Override_Prec; }
    VTimescale ਸਮਾਂ_Override_Unit() ਅਚਲ { ਵਾਪਸ m_ਸਮਾਂ_Override_Unit; }
    VTimescale ਸਮਾਂ_Compute_Prec(ਅਚਲ VTimescale& flag) ਅਚਲ;
    VTimescale ਸਮਾਂ_Compute_Unit(ਅਚਲ VTimescale& flag) ਅਚਲ;
    ਪੂਰਨ trace_ਡੂੰਘਾਈ() ਅਚਲ { ਵਾਪਸ m_trace_ਡੂੰਘਾਈ; }
    ਪੂਰਨ traceMaxArray() ਅਚਲ { ਵਾਪਸ m_traceMaxArray; }
    ਪੂਰਨ trace_Max_ਚੌੜਾਈ() ਅਚਲ { ਵਾਪਸ m_trace_Max_ਚੌੜਾਈ; }
    bool useTraceParallel() ਅਚਲ {
        ਵਾਪਸ trace() && traceEnabledVcd() && (ਧਾਗੇ() > 1 || hier_ਬੱਚਾ() > 1);
    }
    ਪੂਰਨ unroll_ਗਿਣਤੀ() ਅਚਲ { ਵਾਪਸ m_unroll_ਗਿਣਤੀ; }
    ਪੂਰਨ unroll_ਹੱਦਬੰਦੀ() ਅਚਲ { ਵਾਪਸ m_unroll_ਹੱਦਬੰਦੀ; }
    ਪੂਰਨ unrollStmts() ਅਚਲ { ਵਾਪਸ m_unrollStmts; }
    ਪੂਰਨ constraint_Array_ਹੱਦਬੰਦੀ() ਅਚਲ { ਵਾਪਸ m_constraint_Array_ਹੱਦਬੰਦੀ; }
    ਪੂਰਨ verilateJobs() ਅਚਲ { ਵਾਪਸ m_verilateJobs; }

    ਪੂਰਨ comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ() ਅਚਲ { ਵਾਪਸ m_comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ; }
    ਪੂਰਨ comp_ਹੱਦਬੰਦੀ_Members() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_comp_ਹੱਦਬੰਦੀ_Members; }
    ਪੂਰਨ comp_ਹੱਦਬੰਦੀ_Parens() ਅਚਲ { ਵਾਪਸ m_comp_ਹੱਦਬੰਦੀ_Parens; }

    string diagnosticsSarifOutput() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_diagnosticsSarifOutput.ਖਾਲੀ_ਥਾਵਾਂ() ? makeDir() + "/" + ਅਗੇਤਰ() + ".sarif"
                                                : m_diagnosticsSarifOutput;
    }
    string exeName() ਅਚਲ { ਵਾਪਸ m_exeName != "" ? m_exeName : ਅਗੇਤਰ(); }
    VFileLibList hierParamFile() ਅਚਲ { ਵਾਪਸ m_hierParamsFile; }
    string jsonOnlyOutput() ਅਚਲ { ਵਾਪਸ m_jsonOnlyOutput; }
    string jsonOnlyMetaOutput() ਅਚਲ { ਵਾਪਸ m_jsonOnlyMetaOutput; }
    string l2Name() ਅਚਲ { ਵਾਪਸ m_l2Name; }
    string libCreate() ਅਚਲ { ਵਾਪਸ m_libCreate; }
    string libCreateName(bool shared) ਅਚਲ {
        string libName = "lib" + libCreate();
        ਜੇ (shared) {
            libName += ".so";
        } ਨਹੀਂਤਾਂ {
            libName += ".a";
        }
        ਵਾਪਸ libName;
    }
    string main_ਸਿਖਰ_Name() ਅਚਲ { ਵਾਪਸ m_main_ਸਿਖਰ_Name; }
    string makeDir() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_makeDir; }
    string mod_ਅਗੇਤਰ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_mod_ਅਗੇਤਰ; }
    string pipeFilter() ਅਚਲ { ਵਾਪਸ m_pipeFilter; }
    string ਅਗੇਤਰ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਅਗੇਤਰ; }
    // Not just called protectKey() to avoid bugs of not using protectKeyDefaulted()
    bool protect_ਕੁੰਜੀ_Provided() ਅਚਲ { ਵਾਪਸ !m_protect_ਕੁੰਜੀ.ਖਾਲੀ_ਥਾਵਾਂ(); }
    string protect_ਕੁੰਜੀ_Defaulted() VL_MT_ਸੁਰੱਖਿਅਤ;  // Set default key if not set by user
    string ਸਿਖਰ_Module() ਅਚਲ { ਵਾਪਸ m_ਸਿਖਰ_Module; }  // As AstNode::encodeName()
    bool no_Trace_ਸਿਖਰ() ਅਚਲ { ਵਾਪਸ m_no_Trace_ਸਿਖਰ; }
    string ਅਣਵਰਤਿਆ_Regexp() ਅਚਲ { ਵਾਪਸ m_ਅਣਵਰਤਿਆ_Regexp; }
    string waiverOutput() ਅਚਲ { ਵਾਪਸ m_waiverOutput; }
    string work() ਅਚਲ { ਵਾਪਸ m_work; }
    bool isWaiverOutput() ਅਚਲ { ਵਾਪਸ !m_waiverOutput.ਖਾਲੀ_ਥਾਵਾਂ(); }
    string x_ਸੌਂਪੋ() ਅਚਲ { ਵਾਪਸ m_x_ਸੌਂਪੋ; }
    string xInitial() ਅਚਲ { ਵਾਪਸ m_xInitial; }

    ਅਚਲ VString_ਸੈੱਟ& cppFiles() ਅਚਲ { ਵਾਪਸ m_cppFiles; }
    ਅਚਲ VStringList& cFlags() ਅਚਲ { ਵਾਪਸ m_cFlags; }
    ਅਚਲ VString_ਸੈੱਟ& compilerIncludes() ਅਚਲ { ਵਾਪਸ m_compilerIncludes; }
    ਅਚਲ VStringList& ldLibFiles() ਅਚਲ { ਵਾਪਸ m_ldLibFiles; }
    ਅਚਲ VStringList& ldLibs() ਅਚਲ { ਵਾਪਸ m_ldLibs; }
    ਅਚਲ VStringList& makeFlags() ਅਚਲ { ਵਾਪਸ m_makeFlags; }
    ਅਚਲ VFile_Lib_ਸੈੱਟ& libraryFiles() ਅਚਲ { ਵਾਪਸ m_libraryFiles; }
    ਅਚਲ VString_ਸੈੱਟ& libmapFiles() ਅਚਲ { ਵਾਪਸ m_libmapFiles; }
    ਅਚਲ VFileLibList& vFiles() ਅਚਲ { ਵਾਪਸ m_vFiles; }
    ਅਚਲ VFile_Lib_ਸੈੱਟ& vltFiles() ਅਚਲ { ਵਾਪਸ m_vltFiles; }
    ਅਚਲ VStringList& forceIncs() ਅਚਲ { ਵਾਪਸ m_forceIncs; }

    bool hasParameter(ਅਚਲ string& name);
    string parameter(ਅਚਲ string& name);
    ਖਾਲੀ checkParameters();

    bool isFuture(ਅਚਲ string& flag) ਅਚਲ;
    bool isFuture0(ਅਚਲ string& flag) ਅਚਲ;
    bool isFuture1(ਅਚਲ string& flag) ਅਚਲ;
    bool isLibraryFile(ਅਚਲ string& filename, ਅਚਲ string& libname) ਅਚਲ;

    // ACCESSORS (optimization options)
    bool fAcycSimp() ਅਚਲ { ਵਾਪਸ m_fAcycSimp; }
    bool fAssemble() ਅਚਲ { ਵਾਪਸ m_fAssemble; }
    bool f_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Loops() ਅਚਲ { ਵਾਪਸ m_f_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Loops; }
    bool fBitblast() ਅਚਲ { ਵਾਪਸ m_fBitblast; }
    bool fCaseDecoder() ਅਚਲ { ਵਾਪਸ m_fCaseDecoder; }
    bool f_Case_ਸਾਰਣੀ() ਅਚਲ { ਵਾਪਸ m_f_Case_ਸਾਰਣੀ; }
    bool f_Case_ਰੁੱਖ() ਅਚਲ { ਵਾਪਸ m_f_Case_ਰੁੱਖ; }
    bool fCombine() ਅਚਲ { ਵਾਪਸ m_fCombine; }
    bool fConst() ਅਚਲ { ਵਾਪਸ m_fConst; }
    bool fConstBeforeDfg() ਅਚਲ { ਵਾਪਸ m_fConstBeforeDfg; }
    bool f_Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ() ਅਚਲ { ਵਾਪਸ m_f_Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ; }
    bool fConstEager() ਅਚਲ { ਵਾਪਸ m_fConstEager; }
    bool fDedupe() ਅਚਲ { ਵਾਪਸ m_fDedupe; }
    bool fDfg() ਅਚਲ { ਵਾਪਸ m_fDfg; }
    bool fDfgPeephole() ਅਚਲ { ਵਾਪਸ m_fDfgPeephole; }
    bool f_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels() ਅਚਲ { ਵਾਪਸ m_f_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels; }
    bool fDfgSynthesizeAll() ਅਚਲ { ਵਾਪਸ m_fDfgSynthesizeAll; }
    bool fDfgPeepholeEnabled(ਅਚਲ std::string& name) ਅਚਲ {
        ਵਾਪਸ !m_fDfgPeepholeDisabled.ਗਿਣਤੀ_2(name);
    }
    bool fDeadAssigns() ਅਚਲ { ਵਾਪਸ m_fDeadAssigns; }
    bool fDeadCells() ਅਚਲ { ਵਾਪਸ m_fDeadCells; }
    bool fDeadMethods() ਅਚਲ { ਵਾਪਸ m_fDeadMethods; }
    bool fDecompose() ਅਚਲ { ਵਾਪਸ m_fDecomposePacked || m_fDecomposeUnpacked; }
    bool fDecomposePacked() ਅਚਲ { ਵਾਪਸ m_fDecomposePacked; }
    bool fDecomposeUnpacked() ਅਚਲ { ਵਾਪਸ m_fDecomposeUnpacked; }
    bool fExpand() ਅਚਲ { ਵਾਪਸ m_fExpand; }
    bool fFuncBalanceCat() ਅਚਲ { ਵਾਪਸ m_fFuncBalanceCat; }
    bool f_Func_ਵੰਡੋ_Cat() ਅਚਲ { ਵਾਪਸ m_f_Func_ਵੰਡੋ_Cat; }
    bool fFunc() ਅਚਲ { ਵਾਪਸ f_Func_ਵੰਡੋ_Cat() || fFuncBalanceCat(); }
    bool fGate() ਅਚਲ { ਵਾਪਸ m_fGate; }
    VOptionBool fIcoChangeDetect() ਅਚਲ { ਵਾਪਸ m_fIcoChangeDetect; }
    bool fInline() ਅਚਲ { ਵਾਪਸ m_fInline; }
    bool fInlineCFuncs() ਅਚਲ { ਵਾਪਸ m_fInlineCFuncs; }
    bool fInlineFuncs() ਅਚਲ { ਵਾਪਸ m_fInlineFuncs; }
    bool fInlineFuncsEager() ਅਚਲ { ਵਾਪਸ m_fInlineFuncsEager; }
    bool fLife() ਅਚਲ { ਵਾਪਸ m_fLife; }
    bool fLifePost() ਅਚਲ { ਵਾਪਸ m_fLifePost; }
    bool fLocalize() ਅਚਲ { ਵਾਪਸ m_fLocalize; }
    bool f_ਮਿਲਾਓ_Cond() ਅਚਲ { ਵਾਪਸ m_f_ਮਿਲਾਓ_Cond; }
    bool f_ਮਿਲਾਓ_Cond_Motion() ਅਚਲ { ਵਾਪਸ m_f_ਮਿਲਾਓ_Cond_Motion; }
    bool fReloop() ਅਚਲ { ਵਾਪਸ m_fReloop; }
    bool fReorder() ਅਚਲ { ਵਾਪਸ m_fReorder; }
    bool f_ਟੁਕੜਾ() ਅਚਲ { ਵਾਪਸ m_f_ਟੁਕੜਾ; }
    ਪੂਰਨ f_ਟੁਕੜਾ_Element_ਹੱਦਬੰਦੀ() ਅਚਲ { ਵਾਪਸ m_f_ਟੁਕੜਾ_Element_ਹੱਦਬੰਦੀ; }
    bool f_ਵੰਡੋ() ਅਚਲ { ਵਾਪਸ m_f_ਵੰਡੋ; }
    bool fSubst() ਅਚਲ { ਵਾਪਸ m_fSubst; }
    bool fSubstConst() ਅਚਲ { ਵਾਪਸ m_fSubstConst; }
    bool f_ਸਾਰਣੀ() ਅਚਲ { ਵਾਪਸ m_f_ਸਾਰਣੀ; }
    bool fTaskifyAll() ਅਚਲ { ਵਾਪਸ m_fTaskifyAll; }

    std::string trace_Class_ਅਧਾਰ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;  // Deprecated
    std::string traceClassLang() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;  // Deprecated
    std::ਸਦਿਸ਼<std::string> traceClassBases() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    std::ਸਦਿਸ਼<std::string> traceClassLangs() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    std::ਸਦਿਸ਼<std::string> trace_ਸਰੋਤ_Bases() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    std::ਸਦਿਸ਼<std::string> trace_ਸਰੋਤ_Langs() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;

    bool ਦਰਜਾਵਾਰ() ਅਚਲ { ਵਾਪਸ m_ਦਰਜਾਵਾਰ; }
    ਪੂਰਨ hier_ਬੱਚਾ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_hier_ਬੱਚਾ; }
    ਪੂਰਨ hier_ਧਾਗੇ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_hier_ਧਾਗੇ == 0 ? m_ਧਾਗੇ : m_hier_ਧਾਗੇ; }
    bool hier_ਸਿਖਰ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ !m_hier_ਬੱਚਾ && !m_hier_ਖੰਡਾਂ.ਖਾਲੀ_ਥਾਵਾਂ(); }
    ਅਚਲ V3_Hier_ਖੰਡ_Opt_ਸੈੱਟ& hier_ਖੰਡਾਂ() ਅਚਲ { ਵਾਪਸ m_hier_ਖੰਡਾਂ; }
    // Directory to save .tree, .dot, .dat, .vpp for hierarchical block top
    // Returns makeDir() unless top module of hierarchical Verilation.
    string hier_ਸਿਖਰ_ਅੰਕੜੇ_Dir() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ hier_ਸਿਖਰ() ? (makeDir() + '/' + ਅਗੇਤਰ() + "__hier.dir") : makeDir();
    }

    // METHODS (from main)
    ਸਥਿਰ string ਸੰਸਕਰਣ() VL_ਸ਼ੁੱਧ;
    ਸਥਿਰ string argString(ਪੂਰਨ argc, ਅੱਖਰ** argv);  ///< Return list of arguments as simple string
    string all_ਦਲੀਲਾਂ_String() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;  ///< Return all passed arguments as simple string
    ਅਚਲ std::list<std::pair<std::list<std::string>, bool>>& all_ਦਲੀਲਾਂ() ਅਚਲ;
    // Return options for child hierarchical blocks when forTop==false, otherwise returns args for
    // the top module.
    string all_ਦਲੀਲਾਂ_String_For_Hier_ਖੰਡ(bool for_ਸਿਖਰ) ਅਚਲ;
    ਖਾਲੀ parseOpts(File_ਸਤਰ* fl, ਪੂਰਨ argc, ਅੱਖਰ** argv) VL_MT_DISABLED;
    ਖਾਲੀ parseOptsList(File_ਸਤਰ* fl, ਅਚਲ string& optdir, ਪੂਰਨ argc, ਅੱਖਰ** argv) VL_MT_DISABLED;
    ਖਾਲੀ parseOptsFile(File_ਸਤਰ* fl, ਅਚਲ string& filename, bool rel) VL_MT_DISABLED;

    // METHODS (environment)
    // Most of these may be built into the executable with --enable-defenv,
    // see the README.  If adding new variables, also see src/Makefile_obj.in
    // Also add to V3Options::showVersion()
    ਸਥਿਰ string getenvBuiltins(ਅਚਲ string& var);
    ਸਥਿਰ string getenvMAKE();
    ਸਥਿਰ string getenvMAKEFLAGS();
    ਸਥਿਰ string getenvPERL();
    ਸਥਿਰ string getenvPYTHON3();
    ਸਥਿਰ string getenvSYSTEMC();
    ਸਥਿਰ string getenvSYSTEMC_ARCH();
    ਸਥਿਰ string getenvSYSTEMC_INCLUDE();
    ਸਥਿਰ string getenvSYSTEMC_LIBDIR();
    ਸਥਿਰ string getenv_VERILATOR_ਜੜ੍ਹ();
    ਸਥਿਰ string getenvVERILATOR_SOLVER();
    ਸਥਿਰ string get_Std_Package_ਰਸਤਾ();
    ਸਥਿਰ string get_Std_Waiver_ਰਸਤਾ();
    ਸਥਿਰ string getSupported(ਅਚਲ string& var);
    ਸਥਿਰ bool systemCSystemWide();
    ਸਥਿਰ bool systemCFound();  // SystemC installed, or environment points to it
    ਸਥਿਰ bool coroutineSupport();  // Configured C++ compiler supports coroutines
    ਸਥਿਰ bool devAsan();  // 'verilator_bin' built with AddressSanitizer
    ਸਥਿਰ bool devGcov();  // 'verilator_bin' built with code coverage for gcov
    ਸਥਿਰ bool tsanSupport();  // Configured C++ compiler supports ThreadSanitizer

    // METHODS (file utilities using these options)
    string fileExists(ਅਚਲ string& filename);
    string file_ਰਸਤਾ(File_ਸਤਰ* fl, ਅਚਲ string& modname, ਅਚਲ string& lastpath,
                    ਅਚਲ string& errmsg);
    string file_ਰਸਤਾ_Looked_ਸੁਨੇਹਾ(File_ਸਤਰ* fl, ਅਚਲ string& modname);
    V3_Lang_ਕੋਡ fileLanguage(ਅਚਲ string& filename);
    ਸਥਿਰ bool file_Stat_ਸਾਧਾਰਨ(ਅਚਲ string& filename);

    // METHODS (other OS)
    ਸਥਿਰ ਖਾਲੀ throwSigsegv();
};

//######################################################################

#ਜੇ_ਅੰਤ  // guard
