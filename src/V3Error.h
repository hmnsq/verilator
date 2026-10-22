// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Error handling
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

#ifndef VERILATOR_V3ERROR_H_
#define VERILATOR_V3ERROR_H_

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Hash.h"
#অন্তর্ভাৱ "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#অন্তর্ভাৱ "V3String.h"

#অন্তর্ভাৱ <array>
#অন্তর্ভাৱ <bitset>
#অন্তর্ভাৱ <cassert>
#অন্তর্ভাৱ <cctype>
#অন্তর্ভাৱ <deque>
#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <sstream>

class V3Error;
class File_সাৰি;

//######################################################################

class V3ErrorCode final {
সার্ৱজনিক:
    // clang-format off
    গণনা en: uint8_t  {
        EC_MIN=0,       // Keep first
        //
        EC_INFO,        // General information out
        EC_FATAL,       // Kill the program
        EC_FATALMANY,   // Kill the program, due to too many errors, suppress with --quiet-exit
        EC_FATALSRC,    // Kill the program, for internal source errors
        EC_ERROR,       // General error out, can't suppress
        EC_FIRST_NAMED,  // Just a code so the program knows where to start info/errors
        // Boolean information we track per-line, but aren't errors
        I_CELLDEFINE,   // Inside cell define from `celldefine/`endcelldefine
        I_COVERAGE,     // Coverage is on/off from /*verilator coverage_on/off*/
        I_ERRORING,     // Node created to handle earlier error; suppress new errors
        I_DEF_NETTYPE_WIRE,  // `default_nettype is WIRE (false=NONE)
        I_LINT,         // All lint messages
        I_STYLE,        // All style messages
        I_TIMING,       // Enable timing from /*verilator timing_on/off*/
        I_TRACING,      // Tracing is on/off from /*verilator tracing_on/off*/
        // Error codes:
        E_CONTASSINIT,  // Error: Continuous assignment versus initialization
        E_CONSTWRITTEN, // Error: Const variable being written.
        E_LIFETIME,     // Error: Reference to a variable might outlive the variable.
        E_NEEDTIMINGOPT,  // Error: --timing/--no-timing option not specified
        E_NOTIMING,     // Timing control encountered with --no-timing
        E_PORTSHORT,    // Error: Output port is connected to a constant, electrical short
        E_TASKNSVAR,    // Error: Task I/O not simple
        E_UNSUPPORTED,  // Error: Unsupported (generally)
        //
        // Warning codes:
        EC_FIRST_WARN,  // Just a code so the program knows where to start warnings
        //
        ALWCOMBORDER,   // Always_comb with unordered statements
        ALWNEVER,       // Always will never execute
        ASCRANGE,       // Ascending bit range vector
        ASSIGNDLY,      // Assignment delays
        ASSIGNEQEXPR,   // Assignment equal (=) in expression
        ASSIGNIN,       // Assigning to input
        BADSTDPRAGMA,   // Any error related to pragmas
        BADVLTPRAGMA,   // Unknown Verilator pragma
        BLKANDNBLK,     // Blocked and non-blocking assignments to same variable
        BLKLOOPINIT,    // Delayed assignment to array inside for loops
        BLKSEQ,         // Blocking assignments in sequential block
        BSSPACE,        // Backslash space
        CASEINCOMPLETE, // Case statement has missing values
        CASEOVERLAP,    // Case statements overlap
        CASEWITHX,      // Case with X values
        CASEX,          // Casex
        CASTCONST,      // Cast is constant
        CDCRSTLOGIC,    // Logic in async reset path. Historical, never issued.
        CLKDATA,        // Clock used as data. Historical, never issued.
        CMPCONST,       // Comparison is constant due to limited range
        COLONPLUS,      // :+ instead of +:
        COMBDLY,        // Combinatorial delayed assignment
        CONSTRAINTIGN,  // Constraint ignored
        CONTASSREG,     // Continuous assignment on reg
        COVERIGN,       // Coverage ignored
        DECLFILENAME,   // Declaration doesn't match filename
        DEFOVERRIDE,    // Overriding existing define macro through command line
        DEFPARAM,       // Style: Defparam
        DEPRECATED,     // Feature will be deprecated
        ENCAPSULATED,   // Error: local/protected violation
        ENDLABEL,       // End lable name mismatch
        ENUMITEMWIDTH,  // Error: enum item width mismatch
        ENUMVALUE,      // Error: enum type needs explicit cast
        EOFNEWLINE,     // End-of-file missing newline
        FINALDLY,       // Final delayed statement
        FSMMULTI,       // Multiple FSM candidates in one always block
        FUNCTIMECTL,    // Functions cannot have timing/delay/wait
        FUTURE,         // Feature is under development and not yet supported
        GENCLK,         // Generated Clock. Historical, never issued.
        GENUNNAMED,     // Generate unnamed, without label
        HIERBLOCK,      // Ignored hierarchical block setting
        HIERPARAM,      // Parameter using hierarchical value
        IEEEMAYDEPRECATE, // Feature may be deprecated in future IEEE standard
        IFDEPTH,        // If statements too deep
        IGNOREDRETURN,  // Ignoring return value (function as task)
        IMPERFECTSCH,   // Imperfect schedule (disabled by default). Historical, never issued.
        IMPLICIT,       // Implicit wire
        IMPLICITSTATIC, // Implicit static function
        IMPORTSTAR,     // Import::* in $unit
        IMPURE,         // Impure function not being inlined
        INCABSPATH,     // Include has absolute path
        INFINITELOOP,   // Infinite loop
        INITIALDLY,     // Initial delayed statement
        INSECURE,       // Insecure options
        INSIDETRUE,     // Inside range is always true
        LATCH,          // Latch detected outside of always_latch block
        LITENDIAN,      // Little endian, renamed to ASCRANGE
        MINTYPMAXDLY,   // Unsupported: min/typ/max delay expressions
        MISINDENT,      // Misleading indentation
        MODDUP,         // Duplicate module
        MODMISSING,     // Error: missing module
        MULTIDRIVEN,    // Driven from multiple blocks
        MULTIDRIVENPROC,  // Driven from multiple plain always blocks
        MULTITOP,       // Multiple top level modules
        NEVERMATCH,     // Sequence can never match
        NEWERSTD,       // Newer language standard required
        NOEFFECT,       // Statement has no effect
        NOLATCH,        // No latch detected in always_latch block
        NONSTD,         // Non-standard feature present in other sims
        NORETURN,       // Function with no return
        NOTREDOP,       // Error: Logical not before reduction operator
        NULLPORT,       // Null port detected in module definition
        PARAMNODEFAULT, // Parameter without default
        PINCONNECTEMPTY,// Cell pin connected by name with empty reference
        PINMISSING,     // Cell pin not specified
        PINNOCONNECT,   // Cell pin not connected
        PINNOTFOUND,    // instance port name not found in it's module
        PKGNODECL,      // Error: Package/class needs to be predeclared
        PREPROCZERO,    // Preprocessor expression with zero
        PROCASSINIT,    // Procedural assignment versus initialization
        PROCASSWIRE,    // Procedural assignment on wire
        PROFOUTOFDATE,  // Profile data out of date
        PROTECTED,      // Detected `pragma protected
        PROTOTYPEMIS,   // Prototype mismatch or related
        RANDC,          // Unsupported: 'randc' converted to 'rand'
        REALCVT,        // Real conversion
        REDEFMACRO,     // Redefining existing define macro
        RISEFALLDLY,    // Unsupported: rise/fall/turn-off delays
        SELRANGE,       // Selection index out of range
        SHORTREAL,      // Shortreal not supported
        SIDEEFFECT,     // Sideeffect ignored
        SIMILARNAME,    // names differ only by case
        SPECIFYIGN,     // Specify construct ignored
        SPLITVAR,       // Cannot split the variable
        STATICVAR,      // Static variable declared in a loop with a declaration assignment
        STMTDLY,        // Delayed statement
        SUPERNFIRST,    // Super.new must be first statement
        SYMRSVDWORD,    // Symbol is Reserved Word
        SYNCASYNCNET,   // Mixed sync + async reset
        TICKCOUNT,      // Too large tick count
        TIMESCALEMOD,   // Need timescale for module
        UNDRIVEN,       // No drivers
        UNOPT,          // Unoptimizable block. Historical, never issued.
        UNOPTFLAT,      // Unoptimizable block after flattening
        UNOPTTHREADS,   // Thread partitioner unable to fill all requested threads
        UNPACKED,       // Unsupported unpacked
        UNSATCONSTR,    // Unsatisfied constraint
        UNSIGNED,       // Comparison is constant due to unsigned arithmetic
        UNUSED,         // Unused genvar, parameter or signal message (Backward Compatibility)
        UNUSEDGENVAR,   // No receivers for genvar
        UNUSEDLOOP,     // Loop is unused
        UNUSEDPARAM,    // No receivers for parameters
        UNUSEDSIGNAL,   // No receivers for signals
        USERERROR,      // Elaboration time $error
        USERFATAL,      // Elaboration time $fatal
        USERINFO,       // Elaboration time $info
        USERWARN,       // Elaboration time $warning
        VARHIDDEN,      // Hiding variable
        WAITCONST,      // Wait condition is constant
        WIDTH,          // Width mismatch
        WIDTHCONCAT,    // Unsized numbers/parameters in concatenations
        WIDTHDIGITS,    // Mismatch between literal size and its value width in bits
        WIDTHEXPAND,    // Width mismatch- lhs > rhs
        WIDTHTRUNC,     // Width mismatch- lhs < rhs
        WIDTHXZEXPAND,  // Width mismatch- lhs > rhs xz filled
        ZERODLY,        // #0 delay
        ZEROREPL,       // Replication width of zero
        _ENUM_MAX
        // ***Add new elements below also***
    };
    // clang-format on
    গণনা en m_e;
    V3ErrorCode()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3ErrorCode(en _e)
        : m_e{_e} {}
    explicit V3ErrorCode(ধ্রুৱ ৱর্ণ* msgp);  // Matching code or ERROR
    explicit V3ErrorCode(ধ্রুৱ std::string& বাৰ্তা)
        : V3ErrorCode{বাৰ্তা.c_str()} {}
    explicit V3ErrorCode(পূর্ণাঙ্ক _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_e; }
    ধ্রুৱ ৱর্ণ* ascii() ধ্রুৱ VL_MT_SAFE {
        স্থির ধ্রুৱ ৱর্ণ* ধ্রুৱ names[] = {
            // Leading spaces indicate it can't be disabled.
            " MIN", " INFO", " FATAL", " FATALMANY", " FATALSRC", " ERROR", " FIRST_NAMED",
            // Boolean
            " I_CELLDEFINE", " I_COVERAGE", " I_ERRORING", " I_DEF_NETTYPE_WIRE", " I_LINT",
            " I_STYLE", " I_TIMING", " I_TRACING",
            // Errors
            "CONTASSINIT", "CONSTWRITTEN", "LIFETIME", "NEEDTIMINGOPT", "NOTIMING", "PORTSHORT",
            "TASKNSVAR", "UNSUPPORTED",
            // Warnings
            " EC_FIRST_WARN", "ALWCOMBORDER", "ALWNEVER", "ASCRANGE", "ASSIGNDLY", "ASSIGNEQEXPR",
            "ASSIGNIN", "BADSTDPRAGMA", "BADVLTPRAGMA", "BLKANDNBLK", "BLKLOOPINIT", "BLKSEQ",
            "BSSPACE", "CASEINCOMPLETE", "CASEOVERLAP", "CASEWITHX", "CASEX", "CASTCONST",
            "CDCRSTLOGIC", "CLKDATA", "CMPCONST", "COLONPLUS", "COMBDLY", "CONSTRAINTIGN",
            "CONTASSREG", "COVERIGN", "DECLFILENAME", "DEFOVERRIDE", "DEFPARAM", "DEPRECATED",
            "ENCAPSULATED", "ENDLABEL", "ENUMITEMWIDTH", "ENUMVALUE", "EOFNEWLINE", "FINALDLY",
            "FSMMULTI", "FUNCTIMECTL", "FUTURE", "GENCLK", "GENUNNAMED", "HIERBLOCK", "HIERPARAM",
            "IEEEMAYDEPRECATE", "IFDEPTH", "IGNOREDRETURN", "IMPERFECTSCH", "IMPLICIT",
            "IMPLICITSTATIC", "IMPORTSTAR", "IMPURE", "INCABSPATH", "INFINITELOOP", "INITIALDLY",
            "INSECURE", "INSIDETRUE", "LATCH", "LITENDIAN", "MINTYPMAXDLY", "MISINDENT", "MODDUP",
            "MODMISSING", "MULTIDRIVEN", "MULTIDRIVENPROC", "MULTITOP", "NEVERMATCH", "NEWERSTD",
            "NOEFFECT", "NOLATCH", "NONSTD", "NORETURN", "NOTREDOP", "NULLPORT", "PARAMNODEFAULT",
            "PINCONNECTEMPTY", "PINMISSING", "PINNOCONNECT", "PINNOTFOUND", "PKGNODECL",
            "PREPROCZERO", "PROCASSINIT", "PROCASSWIRE", "PROFOUTOFDATE", "PROTECTED",
            "PROTOTYPEMIS", "RANDC", "REALCVT", "REDEFMACRO", "RISEFALLDLY", "SELRANGE",
            "SHORTREAL", "SIDEEFFECT", "SIMILARNAME", "SPECIFYIGN", "SPLITVAR", "STATICVAR",
            "STMTDLY", "SUPERNFIRST", "SYMRSVDWORD", "SYNCASYNCNET", "TICKCOUNT", "TIMESCALEMOD",
            "UNDRIVEN", "UNOPT", "UNOPTFLAT", "UNOPTTHREADS", "UNPACKED", "UNSATCONSTR",
            "UNSIGNED", "UNUSED", "UNUSEDGENVAR", "UNUSEDLOOP", "UNUSEDPARAM", "UNUSEDSIGNAL",
            "USERERROR", "USERFATAL", "USERINFO", "USERWARN", "VARHIDDEN", "WAITCONST", "WIDTH",
            "WIDTHCONCAT", "WIDTHDIGITS", "WIDTHEXPAND", "WIDTHTRUNC", "WIDTHXZEXPAND", "ZERODLY",
            "ZEROREPL", " MAX"};
        ঘূৰাই names[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ধ্রুৱ VL_MT_SAFE { ঘূৰাই (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই (m_e != EC_INFO && m_e < V3ErrorCode::EC_FIRST_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই m_e == V3ErrorCode::EC_FATAL || m_e == V3ErrorCode::EC_FATALMANY
               || m_e == V3ErrorCode::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই m_e == V3ErrorCode::USERINFO || m_e == V3ErrorCode::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == WIDTH
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ধ্রুৱ VL_MT_SAFE {
        ঘূৰাই (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ধ্রুৱ { ঘূৰাই m_e >= EC_FIRST_NAMED; }

গুপ্ত:
    V3ErrorCode renamedTo() ধ্রুৱ {
        // Return a new error this error has been renamed to
        যদি (m_e == LITENDIAN) ঘূৰাই V3ErrorCode{ASCRANGE};
        ঘূৰাই V3ErrorCode{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ধ্রুৱ { ঘূৰাই renamedTo() != V3ErrorCode{EC_MIN}; }

সার্ৱজনিক:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    শূন্য forDelegateCodes(std::function<শূন্য(V3ErrorCode code)> action) ধ্রুৱ {
        // If add to this switch, also update isUnder()
        ৱিকল্প (m_e) {
        পক্ষ V3ErrorCode::E_UNSUPPORTED:
            action(V3ErrorCode{E_UNSUPPORTED});
            action(V3ErrorCode{COVERIGN});
            action(V3ErrorCode{SPECIFYIGN});
            ঘূৰাই;
        পক্ষ V3ErrorCode::I_LINT:
            বাবে (পূর্ণাঙ্ক i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                ধ্রুৱ V3ErrorCode subcode{i};
                যদি (subcode.lintError()) action(subcode);
            }
            ঘূৰাই;
        পক্ষ V3ErrorCode::I_STYLE:
            বাবে (পূর্ণাঙ্ক i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                ধ্রুৱ V3ErrorCode subcode{i};
                যদি (subcode.styleError()) action(subcode);
            }
            ঘূৰাই;
        পক্ষ V3ErrorCode::UNUSED:
            action(V3ErrorCode{UNUSEDGENVAR});
            action(V3ErrorCode{UNUSEDLOOP});
            action(V3ErrorCode{UNUSEDPARAM});
            action(V3ErrorCode{UNUSEDSIGNAL});
            ঘূৰাই;
        পক্ষ V3ErrorCode::WIDTH:
            action(V3ErrorCode{WIDTHDIGITS});
            action(V3ErrorCode{WIDTHEXPAND});
            action(V3ErrorCode{WIDTHTRUNC});
            action(V3ErrorCode{WIDTHXZEXPAND});
            ঘূৰাই;
        স্ৱভাৱ: action(*this);
        }
    }
    bool isUnder(V3ErrorCode other) ধ্রুৱ {
        // Reverse of forDelegateCodes, return true if this' code is under another
        যদি (VL_LIKELY(other == m_e)) ঘূৰাই true;
        ৱিকল্প (other) {
        পক্ষ V3ErrorCode::E_UNSUPPORTED:
            ঘূৰাই m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        পক্ষ V3ErrorCode::I_LINT: ঘূৰাই lintError();
        পক্ষ V3ErrorCode::I_STYLE: ঘূৰাই styleError();
        পক্ষ V3ErrorCode::UNUSED:
            ঘূৰাই m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        পক্ষ V3ErrorCode::WIDTH:
            ঘূৰাই m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        স্ৱভাৱ: ঘূৰাই false;
        }
    }
    string url() ধ্রুৱ;
};
constexpr bool operator==(ধ্রুৱ V3ErrorCode& lhs, ধ্রুৱ V3ErrorCode& rhs) {
    ঘূৰাই lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুৱ V3ErrorCode& lhs, V3ErrorCode::en rhs) { ঘূৰাই lhs.m_e == rhs; }
constexpr bool operator==(V3ErrorCode::en lhs, ধ্রুৱ V3ErrorCode& rhs) { ঘূৰাই lhs == rhs.m_e; }
অন্তঃস্থ std::ostream& operator<<(std::ostream& os, ধ্রুৱ V3ErrorCode& rhs) {
    ঘূৰাই os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VErrorBitSet final {
    std::bitset<V3ErrorCode::_ENUM_MAX> m_bitset;  // Enable for each code
    VErrorBitSet(ধ্রুৱ VErrorBitSet& lhs, ধ্রুৱ VErrorBitSet& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

সার্ৱজনিক:
    class AllOnes {};
    VErrorBitSet() = স্ৱভাৱ;
    explicit VErrorBitSet(AllOnes) { m_bitset.set(); }
    ~VErrorBitSet() = স্ৱভাৱ;
    bool test(V3ErrorCode code) ধ্রুৱ { ঘূৰাই m_bitset[code]; }
    শূন্য set(V3ErrorCode code, bool flag) { m_bitset[code] = flag; }
    V3Hash hash() ধ্রুৱ {
        ধ্রুৱ size_t hashCode = std::hash<std::bitset<V3ErrorCode::_ENUM_MAX>>()(m_bitset);
        ঘূৰাই V3Hash{static_cast<uint64_t>(hashCode)};
    }
    VErrorBitSet operator&(ধ্রুৱ VErrorBitSet& rhs) ধ্রুৱ { ঘূৰাই VErrorBitSet{*this, rhs}; }
    bool operator==(ধ্রুৱ VErrorBitSet& rhs) ধ্রুৱ { ঘূৰাই m_bitset == rhs.m_bitset; }
    string ascii() ধ্রুৱ {  // LCOV_EXCL_START
        string result;
        বাবে (পূর্ণাঙ্ক i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
            যদি (!test(V3ErrorCode{i})) result += " !"s + V3ErrorCode{i}.ascii();
        }
        ঘূৰাই result;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VErrorMessage final {
    // TYPES
    using File_সাৰিগুলি = std::deque<ধ্রুৱ File_সাৰি*>;
    // MEMBERS
    V3ErrorCode m_code;  // Which warning
    string m_text;  // Warning message text
    File_সাৰি* m_fileline;  // Primary warning fileline
    File_সাৰিগুলি m_filelines;  // Additional referenced filelines
সার্ৱজনিক:
    // CONSTRUCTORS
    VErrorMessage() { clear(); }
    ~VErrorMessage() = স্ৱভাৱ;
    শূন্য clear() { init(V3ErrorCode::EC_MIN); }
    শূন্য init(V3ErrorCode code) {
        m_code = code;
        m_text = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3ErrorCode code() ধ্রুৱ { ঘূৰাই m_code; }
    bool isClear() ধ্রুৱ { ঘূৰাই m_code == V3ErrorCode::EC_MIN; }
    string text() ধ্রুৱ { ঘূৰাই m_text; }
    শূন্য text(ধ্রুৱ string& বাৰ্তা) { m_text = বাৰ্তা; }
    File_সাৰি* fileline() ধ্রুৱ { ঘূৰাই m_fileline; }
    শূন্য fileline(File_সাৰি* fl) { m_fileline = fl; }
    File_সাৰিগুলি filelines() ধ্রুৱ { ঘূৰাই m_filelines; }
    // Returns identifier for use in warnRelated()
    পূর্ণাঙ্ক ঠেলো_Fileline(ধ্রুৱ File_সাৰি* fl) {
        m_filelines.emplace_back(fl);
        ঘূৰাই m_filelines.size() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    friend class V3Error;

সার্ৱজনিক:
    using MessagesSet = std::set<std::string>;
    using ErrorExitCb = শূন্য (*)(শূন্য);

গুপ্ত:
    স্থির constexpr unsigned MAX_ERRORS = 50;  // Fatal after this may errors

    // MEMBERS
    // Tell user to see manual, 0=not yet, 1=doit, 2=disable
    bool m_tellManual VL_GUARDED_BY(m_mutex) = false;
    bool m_tellInternal VL_GUARDED_BY(m_mutex) = false;
    VErrorMessage m_message VL_GUARDED_BY(m_mutex);  // Message being formed
    bool m_errorSuppressed VL_GUARDED_BY(m_mutex)
        = false;  // Error being formed should be suppressed
    MessagesSet m_messages VL_GUARDED_BY(m_mutex);  // Errors outputted, to remove dups
    ErrorExitCb m_errorExitCb VL_GUARDED_BY(m_mutex)
        = nullptr;  // Callback when error occurs for dumping
    bool m_errorContexted VL_GUARDED_BY(m_mutex) = false;  // Error being formed got context
    পূর্ণাঙ্ক m_warn_গুনতি VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    পূর্ণাঙ্ক m_err_গুনতি VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VErrorBitSet m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VErrorBitSet m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VErrorBitSet m_showedSuppressed VL_GUARDED_BY(m_mutex);
    পূর্ণাঙ্ক m_debugDefault = 0;  // Option: --debugi Default debugging level
    পূর্ণাঙ্ক m_errorLimit VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    শূন্য v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { ঘূৰাই m_errorStr; }
    শূন্য v3errorEnd(ধ্রুৱ std::ostringstream& sstr, ধ্রুৱ string& extra, File_সাৰি* fileline)
        VL_REQUIRES(m_mutex);
    শূন্য v3errorEndGuts(ধ্রুৱ std::ostringstream& sstr, ধ্রুৱ string& extra, File_সাৰি* fileline)
        VL_REQUIRES(m_mutex);

সার্ৱজনিক:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string বাৰ্তা_Prefix() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warnMoreSpaces() VL_REQUIRES(m_mutex);
    শূন্য execErrorExitCb() VL_REQUIRES(m_mutex) {
        যদি (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { ঘূৰাই m_errorExitCb; }
    শূন্য errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3ErrorCode code, bool supp) VL_REQUIRES(m_mutex);
    শূন্য vlAbortOrExit() VL_REQUIRES(m_mutex);
    শূন্য errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    শূন্য incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_গুনতি; }
    শূন্য incErrors() VL_REQUIRES(m_mutex) { ++m_err_গুনতি; }
    পূর্ণাঙ্ক error_গুনতি() ধ্রুৱ VL_REQUIRES(m_mutex) { ঘূৰাই m_err_গুনতি; }
    bool isErrorOrWarn() ধ্রুৱ VL_REQUIRES(m_mutex) {
        ঘূৰাই error_গুনতি() || (warnFatal() && warn_গুনতি());
    }
    bool pretendError(V3ErrorCode code) VL_REQUIRES(m_mutex) { ঘূৰাই m_pretendError.test(code); }
    শূন্য pretendError(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        code.forDelegateCodes([this, flag](V3ErrorCode subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.set(subcode, flag); });
    }
    পূর্ণাঙ্ক debugDefault() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_debugDefault; }
    শূন্য debugDefault(পূর্ণাঙ্ক level) VL_MT_UNSAFE { m_debugDefault = level; }
    পূর্ণাঙ্ক errorLimit() ধ্রুৱ VL_REQUIRES(m_mutex) { ঘূৰাই m_errorLimit; }
    শূন্য errorLimit(পূর্ণাঙ্ক level) VL_REQUIRES(m_mutex) { m_errorLimit = level; }
    bool warnFatal() ধ্রুৱ VL_REQUIRES(m_mutex) { ঘূৰাই m_warnFatal; }
    শূন্য warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3ErrorCode errorCode() ধ্রুৱ VL_REQUIRES(m_mutex) { ঘূৰাই m_message.code(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { ঘূৰাই m_errorContexted; }
    পূর্ণাঙ্ক warn_গুনতি() ধ্রুৱ VL_REQUIRES(m_mutex) { ঘূৰাই m_warn_গুনতি; }
    bool errorSuppressed() ধ্রুৱ VL_REQUIRES(m_mutex) { ঘূৰাই m_errorSuppressed; }
    শূন্য errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3ErrorCode code) VL_REQUIRES(m_mutex) {
        ঘূৰাই m_describedEachWarn.test(code);
    }
    শূন্য describedEachWarn(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.set(code, flag);
    }
    শূন্য suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ধ্রুৱ File_সাৰি* fl) VL_REQUIRES(m_mutex) {
        ধ্রুৱ পূর্ণাঙ্ক id = m_message.ঠেলো_Fileline(fl);
        ঘূৰাই "__WARNRELATED("s + std::to_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        ঘূৰাই "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

সার্ৱজনিক:
    V3Error() = delete;
    স্থির V3ErrorGuarded& s() VL_MT_SAFE {  // Singleton
        স্থির V3ErrorGuarded s_s;
        ঘূৰাই s_s;
    }

    // ACCESSORS
    স্থির শূন্য debugDefault(পূর্ণাঙ্ক level) VL_MT_UNSAFE { s().debugDefault(level); }
    স্থির পূর্ণাঙ্ক debugDefault() VL_MT_SAFE { ঘূৰাই s().debugDefault(); }
    স্থির শূন্য errorLimit(পূর্ণাঙ্ক level) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorLimit(level);
    }
    স্থির পূর্ণাঙ্ক errorLimit() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().errorLimit();
    }
    স্থির শূন্য warnFatal(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    স্থির bool warnFatal() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().warnFatal();
    }
    // returns %Error/%Warn
    স্থির string বাৰ্তা_Prefix() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().বাৰ্তা_Prefix();
    }
    স্থির পূর্ণাঙ্ক error_গুনতি() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().error_গুনতি();
    }
    স্থির bool pretendError(V3ErrorCode code) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().pretendError(code);
    }
    স্থির পূর্ণাঙ্ক warn_গুনতি() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().warn_গুনতি();
    }
    স্থির bool errorContexted() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().errorContexted();
    }
    স্থির শূন্য errorContexted(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    স্থির শূন্য describedEachWarn(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().describedEachWarn(code, flag);
    }
    // METHODS
    স্থির শূন্য incErrors() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().incErrors();
    }
    স্থির শূন্য incWarnings() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().incWarnings();
    }
    স্থির শূন্য init();
    স্থির bool isError(V3ErrorCode code, bool supp) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().isError(code, supp);
    }
    স্থির bool isErrorOrWarn() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().isErrorOrWarn();
    }
    স্থির শূন্য abortIfErrors() {
        যদি (error_গুনতি()) abortIfWarnings();
    }
    স্থির শূন্য abortIfWarnings();
    // Suppress next %Warn if user has it off
    স্থির শূন্য suppressThisWarning() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    স্থির শূন্য pretendError(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().pretendError(code, flag);
    }
    স্থির string সাৰি_Str(ধ্রুৱ ৱর্ণ* filename, পূর্ণাঙ্ক lineno) VL_PURE;
    স্থির string stripMetaText(ধ্রুৱ string& text, bool stripMeta) VL_PURE;
    স্থির V3ErrorCode errorCode() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        ঘূৰাই s().errorCode();
    }
    স্থির শূন্য errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ধ্রুৱ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    স্থির constexpr ধ্রুৱ ৱর্ণ* WARN_MORE = "__WARNMORE__";
    স্থির string warnMore() VL_MT_SAFE { ঘূৰাই WARN_MORE; }
    // When printing an error/warning, mark beginning of context information (can nest)
    স্থির constexpr ধ্রুৱ ৱর্ণ* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    স্থির string warnContextBegin() VL_MT_SAFE { ঘূৰাই WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    স্থির constexpr ধ্রুৱ ৱর্ণ* WARN_CONTEXT_END = "__WARNEND__";
    স্থির string warnContextEnd() VL_MT_SAFE { ঘূৰাই WARN_CONTEXT_END; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    স্থির string warnAdditionalInfo() VL_MT_SAFE { ঘূৰাই "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    স্থির string warnContextNone() {
        V3Error::errorContexted(true);
        ঘূৰাই "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    স্থির std::ostringstream& v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex);
    স্থির std::ostringstream& v3error_Prep_File_সাৰি(V3ErrorCode code, ধ্রুৱ ৱর্ণ* file, পূর্ণাঙ্ক সাৰি)
        VL_ACQUIRE(s().m_mutex);
    স্থির std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { ঘূৰাই s().v3errorStr(); }
    // static, but often overridden in classes.
    স্থির শূন্য v3errorEnd(ধ্রুৱ std::ostringstream& sstr, ধ্রুৱ string& extra, File_সাৰি* fileline)
        VL_RELEASE(s().m_mutex);
    স্থির শূন্য vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
শূন্য v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE;
শূন্য v3errorEndFatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE VL_ATTR_NORETURN;

#ifdef VL_MT_DISABLED_CODE_UNIT
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 1
#অন্যথা
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 0
#endif

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#define v3errorBuildMessage(prep, বাৰ্তা) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << বাৰ্তা))
#define v3warnCode(code, বাৰ্তা) v3errorEnd(v3errorBuildMessage(V3Error::v3errorPrep(code), বাৰ্তা))
#define v3warnCodeFatal(code, বাৰ্তা) \
    v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(code), বাৰ্তা))
#define v3warn(code, বাৰ্তা) v3warnCode(V3ErrorCode::code, বাৰ্তা)
#define v3info(বাৰ্তা) v3warnCode(V3ErrorCode::EC_INFO, বাৰ্তা)
#define v3error(বাৰ্তা) v3warnCode(V3ErrorCode::EC_ERROR, বাৰ্তা)
#define v3fatal(বাৰ্তা) v3warnCodeFatal(V3ErrorCode::EC_FATAL, বাৰ্তা)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#define v3fatalMany(বাৰ্তা) v3warnCodeFatal(V3ErrorCode::EC_FATALMANY, বাৰ্তা)
// Fatal exit; used instead of fatal() to mention the source code line
#define v3fatalSrc(বাৰ্তা) \
    v3errorEndFatal(v3errorBuildMessage( \
        V3Error::v3error_Prep_File_সাৰি(V3ErrorCode::EC_FATALSRC, __FILE__, __LINE__), বাৰ্তা))
// Use this when normal v3fatal is called in static method that overrides fileline.
#define v3fatalStatic(বাৰ্তা) \
    ::v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(V3ErrorCode::EC_FATAL), বাৰ্তা))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#define UINFO(level, stmsg) \
    কৰা { \
        যদি (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::সাৰি_Str(__FILE__, __LINE__) << stmsg; \
            যদি (ss.str()[ss.str().size() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } যেতিয়ালৈ (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#define UINFO_PREFIX(stmsg) \
    কৰা { std::cout << "- " << V3Error::সাৰি_Str(__FILE__, __LINE__) << stmsg; } যেতিয়ালৈ (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#define UINFOTREE(level, nodep, uinfo_বাৰ্তা, dumptree_বাৰ্তা) \
    কৰা { \
        যদি (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_বাৰ্তা; \
            যদি (!us.str().empty()) UINFO(level, us.str()); \
            যদি (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_বাৰ্তা; \
                যদি (ss.str().empty()) ss << uinfo_বাৰ্তা; \
                nodep->dump_বৃক্ষ("- " + V3Error::সাৰি_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } যেতিয়ালৈ (false)

/// Compile statements only when debug build
#ifdef VL_DEBUG
#define UDEBUGONLY(stmts) \
    কৰা { stmts } যেতিয়ালৈ (false)
#অন্যথা
#define UDEBUGONLY(stmts) \
    কৰা { \
        যদি (false) { stmts } \
    } যেতিয়ালৈ (false)
#endif

/// Assert without error location, generally UASSERT_OBJ preferred
#define UASSERT(condition, stmsg) \
    কৰা { \
        যদি (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } যেতিয়ালৈ (false)
/// Assert with object to provide error location
#define UASSERT_OBJ(condition, obj, stmsg) \
    কৰা { \
        যদি (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } যেতিয়ালৈ (false)
// For use in V3Ast static functions only
#define UASSERT_STATIC(condition, stmsg) \
    কৰা { \
        যদি (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } যেতিয়ালৈ (false)
/// Check self test values for expected value.  Safe from side-effects.
#define UASSERT_SELFTEST(got, exp) \
    কৰা { \
        ধ্রুৱ auto g = (got); \
        ধ্রুৱ decltype(g) e{exp}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } যেতিয়ালৈ (false)

// Error that call not supported; only for some Ast functions
#define V3ERROR_NA \
    কৰা { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } যেতিয়ালৈ (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#define V3ERROR_NA_RETURN(মান) \
    V3ERROR_NA; \
    ঘূৰাই মান

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DEBUG(...) \
    VL_ATTR_UNUSED স্থির পূর্ণাঙ্ক debug##__VA_ARGS__() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (শূন্য)&debug##__VA_ARGS__; \
        স্থির পূর্ণাঙ্ক s_level = -1; \
        যদি (VL_UNLIKELY(s_level < 0)) { \
            std::string tag{VL_STRINGIFY(__VA_ARGS__)}; \
            যদি (!tag.empty()) tag[0] = std::tolower(tag[0]); \
            ধ্রুৱ unsigned debugTag = v3Global.opt.debugLevel(tag); \
            ধ্রুৱ unsigned debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ধ্রুৱ unsigned debugLevel = debugTag >= debugSrc ? debugTag : debugSrc; \
            যদি (!v3Global.opt.available()) ঘূৰাই static_cast<পূর্ণাঙ্ক>(debugLevel); \
            s_level = static_cast<পূর্ণাঙ্ক>(debugLevel); \
        } \
        ঘূৰাই s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DUMP(func, tag) \
    VL_ATTR_UNUSED স্থির পূর্ণাঙ্ক dump##func() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (শূন্য)&dump##func; \
        স্থির পূর্ণাঙ্ক s_level = -1; \
        যদি (VL_UNLIKELY(s_level < 0)) { \
            ধ্রুৱ unsigned dumpTag = v3Global.opt.dumpLevel(tag); \
            ধ্রুৱ unsigned dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ধ্রুৱ unsigned dumpLevel = dumpTag >= dumpSrc ? dumpTag : dumpSrc; \
            যদি (!v3Global.opt.available()) ঘূৰাই static_cast<পূর্ণাঙ্ক>(dumpLevel); \
            s_level = static_cast<পূর্ণাঙ্ক>(dumpLevel); \
        } \
        ঘূৰাই s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#define VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(GraphLevel, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(বৃক্ষ_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(বৃক্ষ_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_UNUSED স্থির পূর্ণাঙ্ক dump_বৃক্ষ_Either_Level() { \
        /* Don't complain this function is unused */ (শূন্য)&dump_বৃক্ষ_Either_Level; \
        ঘূৰাই dump_বৃক্ষ_Json_Level() >= dump_বৃক্ষ_Level() ? dump_বৃক্ষ_Json_Level() : dump_বৃক্ষ_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#endif  // Guard
