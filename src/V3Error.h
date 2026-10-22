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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Hash.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <bitset>
#ಅನ್ತರ್ಭಾವ <cassert>
#ಅನ್ತರ್ಭಾವ <cctype>
#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <sstream>

class V3Error;
class File_ಸಾಲು;

//######################################################################

class V3ErrorCode final {
ಸಾರ್ವಜನಿಕ:
    // clang-format off
    ಗಣನಾ en: uint8_t  {
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
    ಗಣನಾ en m_e;
    V3ErrorCode()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3ErrorCode(en _e)
        : m_e{_e} {}
    explicit V3ErrorCode(ಧ್ರುವ ವರ್ಣ* msgp);  // Matching code or ERROR
    explicit V3ErrorCode(ಧ್ರುವ std::string& ಸಂದೇಶ)
        : V3ErrorCode{ಸಂದೇಶ.c_str()} {}
    explicit V3ErrorCode(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_e; }
    ಧ್ರುವ ವರ್ಣ* ascii() ಧ್ರುವ VL_MT_SAFE {
        ಸ್ಥಿರ ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ names[] = {
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
        ಪ್ರತ್ಯಾಗಮ names[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (m_e != EC_INFO && m_e < V3ErrorCode::EC_FIRST_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_e == V3ErrorCode::EC_FATAL || m_e == V3ErrorCode::EC_FATALMANY
               || m_e == V3ErrorCode::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_e == V3ErrorCode::USERINFO || m_e == V3ErrorCode::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == WIDTH
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e >= EC_FIRST_NAMED; }

ಗುಪ್ತ:
    V3ErrorCode renamedTo() ಧ್ರುವ {
        // Return a new error this error has been renamed to
        ಯದಿ (m_e == LITENDIAN) ಪ್ರತ್ಯಾಗಮ V3ErrorCode{ASCRANGE};
        ಪ್ರತ್ಯಾಗಮ V3ErrorCode{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ renamedTo() != V3ErrorCode{EC_MIN}; }

ಸಾರ್ವಜನಿಕ:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    ಶೂನ್ಯ forDelegateCodes(std::function<ಶೂನ್ಯ(V3ErrorCode code)> action) ಧ್ರುವ {
        // If add to this switch, also update isUnder()
        ವಿಕಲ್ಪ (m_e) {
        ಪಕ್ಷ V3ErrorCode::E_UNSUPPORTED:
            action(V3ErrorCode{E_UNSUPPORTED});
            action(V3ErrorCode{COVERIGN});
            action(V3ErrorCode{SPECIFYIGN});
            ಪ್ರತ್ಯಾಗಮ;
        ಪಕ್ಷ V3ErrorCode::I_LINT:
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                ಧ್ರುವ V3ErrorCode subcode{i};
                ಯದಿ (subcode.lintError()) action(subcode);
            }
            ಪ್ರತ್ಯಾಗಮ;
        ಪಕ್ಷ V3ErrorCode::I_STYLE:
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                ಧ್ರುವ V3ErrorCode subcode{i};
                ಯದಿ (subcode.styleError()) action(subcode);
            }
            ಪ್ರತ್ಯಾಗಮ;
        ಪಕ್ಷ V3ErrorCode::UNUSED:
            action(V3ErrorCode{UNUSEDGENVAR});
            action(V3ErrorCode{UNUSEDLOOP});
            action(V3ErrorCode{UNUSEDPARAM});
            action(V3ErrorCode{UNUSEDSIGNAL});
            ಪ್ರತ್ಯಾಗಮ;
        ಪಕ್ಷ V3ErrorCode::WIDTH:
            action(V3ErrorCode{WIDTHDIGITS});
            action(V3ErrorCode{WIDTHEXPAND});
            action(V3ErrorCode{WIDTHTRUNC});
            action(V3ErrorCode{WIDTHXZEXPAND});
            ಪ್ರತ್ಯಾಗಮ;
        ಸ್ವಭಾವ: action(*this);
        }
    }
    bool isUnder(V3ErrorCode other) ಧ್ರುವ {
        // Reverse of forDelegateCodes, return true if this' code is under another
        ಯದಿ (VL_LIKELY(other == m_e)) ಪ್ರತ್ಯಾಗಮ true;
        ವಿಕಲ್ಪ (other) {
        ಪಕ್ಷ V3ErrorCode::E_UNSUPPORTED:
            ಪ್ರತ್ಯಾಗಮ m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        ಪಕ್ಷ V3ErrorCode::I_LINT: ಪ್ರತ್ಯಾಗಮ lintError();
        ಪಕ್ಷ V3ErrorCode::I_STYLE: ಪ್ರತ್ಯಾಗಮ styleError();
        ಪಕ್ಷ V3ErrorCode::UNUSED:
            ಪ್ರತ್ಯಾಗಮ m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        ಪಕ್ಷ V3ErrorCode::WIDTH:
            ಪ್ರತ್ಯಾಗಮ m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        ಸ್ವಭಾವ: ಪ್ರತ್ಯಾಗಮ false;
        }
    }
    string url() ಧ್ರುವ;
};
constexpr bool operator==(ಧ್ರುವ V3ErrorCode& lhs, ಧ್ರುವ V3ErrorCode& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ V3ErrorCode& lhs, V3ErrorCode::en rhs) { ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs; }
constexpr bool operator==(V3ErrorCode::en lhs, ಧ್ರುವ V3ErrorCode& rhs) { ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e; }
ಅನ್ತಃಸ್ಥ std::ostream& operator<<(std::ostream& os, ಧ್ರುವ V3ErrorCode& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VErrorBitSet final {
    std::bitset<V3ErrorCode::_ENUM_MAX> m_bitset;  // Enable for each code
    VErrorBitSet(ಧ್ರುವ VErrorBitSet& lhs, ಧ್ರುವ VErrorBitSet& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

ಸಾರ್ವಜನಿಕ:
    class AllOnes {};
    VErrorBitSet() = ಸ್ವಭಾವ;
    explicit VErrorBitSet(AllOnes) { m_bitset.set(); }
    ~VErrorBitSet() = ಸ್ವಭಾವ;
    bool test(V3ErrorCode code) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bitset[code]; }
    ಶೂನ್ಯ set(V3ErrorCode code, bool flag) { m_bitset[code] = flag; }
    V3Hash hash() ಧ್ರುವ {
        ಧ್ರುವ size_t hashCode = std::hash<std::bitset<V3ErrorCode::_ENUM_MAX>>()(m_bitset);
        ಪ್ರತ್ಯಾಗಮ V3Hash{static_cast<uint64_t>(hashCode)};
    }
    VErrorBitSet operator&(ಧ್ರುವ VErrorBitSet& rhs) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VErrorBitSet{*this, rhs}; }
    bool operator==(ಧ್ರುವ VErrorBitSet& rhs) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_bitset == rhs.m_bitset; }
    string ascii() ಧ್ರುವ {  // LCOV_EXCL_START
        string result;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
            ಯದಿ (!test(V3ErrorCode{i})) result += " !"s + V3ErrorCode{i}.ascii();
        }
        ಪ್ರತ್ಯಾಗಮ result;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VErrorMessage final {
    // TYPES
    using File_ಸಾಲುಗಳು = std::deque<ಧ್ರುವ File_ಸಾಲು*>;
    // MEMBERS
    V3ErrorCode m_code;  // Which warning
    string m_text;  // Warning message text
    File_ಸಾಲು* m_fileline;  // Primary warning fileline
    File_ಸಾಲುಗಳು m_filelines;  // Additional referenced filelines
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VErrorMessage() { clear(); }
    ~VErrorMessage() = ಸ್ವಭಾವ;
    ಶೂನ್ಯ clear() { init(V3ErrorCode::EC_MIN); }
    ಶೂನ್ಯ init(V3ErrorCode code) {
        m_code = code;
        m_text = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3ErrorCode code() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_code; }
    bool isClear() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_code == V3ErrorCode::EC_MIN; }
    string text() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_text; }
    ಶೂನ್ಯ text(ಧ್ರುವ string& ಸಂದೇಶ) { m_text = ಸಂದೇಶ; }
    File_ಸಾಲು* fileline() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_fileline; }
    ಶೂನ್ಯ fileline(File_ಸಾಲು* fl) { m_fileline = fl; }
    File_ಸಾಲುಗಳು filelines() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_filelines; }
    // Returns identifier for use in warnRelated()
    ಪೂರ್ಣಾಙ್ಕ ತಳ್ಳು_Fileline(ಧ್ರುವ File_ಸಾಲು* fl) {
        m_filelines.emplace_back(fl);
        ಪ್ರತ್ಯಾಗಮ m_filelines.size() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    friend class V3Error;

ಸಾರ್ವಜನಿಕ:
    using MessagesSet = std::set<std::string>;
    using ErrorExitCb = ಶೂನ್ಯ (*)(ಶೂನ್ಯ);

ಗುಪ್ತ:
    ಸ್ಥಿರ constexpr unsigned MAX_ERRORS = 50;  // Fatal after this may errors

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
    ಪೂರ್ಣಾಙ್ಕ m_warn_ಎಣಿಕೆ VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    ಪೂರ್ಣಾಙ್ಕ m_err_ಎಣಿಕೆ VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VErrorBitSet m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VErrorBitSet m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VErrorBitSet m_showedSuppressed VL_GUARDED_BY(m_mutex);
    ಪೂರ್ಣಾಙ್ಕ m_debugDefault = 0;  // Option: --debugi Default debugging level
    ಪೂರ್ಣಾಙ್ಕ m_errorLimit VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    ಶೂನ್ಯ v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_errorStr; }
    ಶೂನ್ಯ v3errorEnd(ಧ್ರುವ std::ostringstream& sstr, ಧ್ರುವ string& extra, File_ಸಾಲು* fileline)
        VL_REQUIRES(m_mutex);
    ಶೂನ್ಯ v3errorEndGuts(ಧ್ರುವ std::ostringstream& sstr, ಧ್ರುವ string& extra, File_ಸಾಲು* fileline)
        VL_REQUIRES(m_mutex);

ಸಾರ್ವಜನಿಕ:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string ಸಂದೇಶ_Prefix() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warnMoreSpaces() VL_REQUIRES(m_mutex);
    ಶೂನ್ಯ execErrorExitCb() VL_REQUIRES(m_mutex) {
        ಯದಿ (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_errorExitCb; }
    ಶೂನ್ಯ errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3ErrorCode code, bool supp) VL_REQUIRES(m_mutex);
    ಶೂನ್ಯ vlAbortOrExit() VL_REQUIRES(m_mutex);
    ಶೂನ್ಯ errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    ಶೂನ್ಯ incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_ಎಣಿಕೆ; }
    ಶೂನ್ಯ incErrors() VL_REQUIRES(m_mutex) { ++m_err_ಎಣಿಕೆ; }
    ಪೂರ್ಣಾಙ್ಕ error_ಎಣಿಕೆ() ಧ್ರುವ VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_err_ಎಣಿಕೆ; }
    bool isErrorOrWarn() ಧ್ರುವ VL_REQUIRES(m_mutex) {
        ಪ್ರತ್ಯಾಗಮ error_ಎಣಿಕೆ() || (warnFatal() && warn_ಎಣಿಕೆ());
    }
    bool pretendError(V3ErrorCode code) VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_pretendError.test(code); }
    ಶೂನ್ಯ pretendError(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        code.forDelegateCodes([this, flag](V3ErrorCode subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.set(subcode, flag); });
    }
    ಪೂರ್ಣಾಙ್ಕ debugDefault() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_debugDefault; }
    ಶೂನ್ಯ debugDefault(ಪೂರ್ಣಾಙ್ಕ level) VL_MT_UNSAFE { m_debugDefault = level; }
    ಪೂರ್ಣಾಙ್ಕ errorLimit() ಧ್ರುವ VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_errorLimit; }
    ಶೂನ್ಯ errorLimit(ಪೂರ್ಣಾಙ್ಕ level) VL_REQUIRES(m_mutex) { m_errorLimit = level; }
    bool warnFatal() ಧ್ರುವ VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_warnFatal; }
    ಶೂನ್ಯ warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3ErrorCode errorCode() ಧ್ರುವ VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_message.code(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_errorContexted; }
    ಪೂರ್ಣಾಙ್ಕ warn_ಎಣಿಕೆ() ಧ್ರುವ VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_warn_ಎಣಿಕೆ; }
    bool errorSuppressed() ಧ್ರುವ VL_REQUIRES(m_mutex) { ಪ್ರತ್ಯಾಗಮ m_errorSuppressed; }
    ಶೂನ್ಯ errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3ErrorCode code) VL_REQUIRES(m_mutex) {
        ಪ್ರತ್ಯಾಗಮ m_describedEachWarn.test(code);
    }
    ಶೂನ್ಯ describedEachWarn(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.set(code, flag);
    }
    ಶೂನ್ಯ suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ಧ್ರುವ File_ಸಾಲು* fl) VL_REQUIRES(m_mutex) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ id = m_message.ತಳ್ಳು_Fileline(fl);
        ಪ್ರತ್ಯಾಗಮ "__WARNRELATED("s + std::to_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        ಪ್ರತ್ಯಾಗಮ "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

ಸಾರ್ವಜನಿಕ:
    V3Error() = delete;
    ಸ್ಥಿರ V3ErrorGuarded& s() VL_MT_SAFE {  // Singleton
        ಸ್ಥಿರ V3ErrorGuarded s_s;
        ಪ್ರತ್ಯಾಗಮ s_s;
    }

    // ACCESSORS
    ಸ್ಥಿರ ಶೂನ್ಯ debugDefault(ಪೂರ್ಣಾಙ್ಕ level) VL_MT_UNSAFE { s().debugDefault(level); }
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ debugDefault() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ s().debugDefault(); }
    ಸ್ಥಿರ ಶೂನ್ಯ errorLimit(ಪೂರ್ಣಾಙ್ಕ level) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorLimit(level);
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ errorLimit() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().errorLimit();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ warnFatal(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    ಸ್ಥಿರ bool warnFatal() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().warnFatal();
    }
    // returns %Error/%Warn
    ಸ್ಥಿರ string ಸಂದೇಶ_Prefix() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().ಸಂದೇಶ_Prefix();
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ error_ಎಣಿಕೆ() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().error_ಎಣಿಕೆ();
    }
    ಸ್ಥಿರ bool pretendError(V3ErrorCode code) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().pretendError(code);
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ warn_ಎಣಿಕೆ() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().warn_ಎಣಿಕೆ();
    }
    ಸ್ಥಿರ bool errorContexted() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().errorContexted();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ errorContexted(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ describedEachWarn(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().describedEachWarn(code, flag);
    }
    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ incErrors() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().incErrors();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ incWarnings() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().incWarnings();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ init();
    ಸ್ಥಿರ bool isError(V3ErrorCode code, bool supp) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().isError(code, supp);
    }
    ಸ್ಥಿರ bool isErrorOrWarn() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().isErrorOrWarn();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ abortIfErrors() {
        ಯದಿ (error_ಎಣಿಕೆ()) abortIfWarnings();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ abortIfWarnings();
    // Suppress next %Warn if user has it off
    ಸ್ಥಿರ ಶೂನ್ಯ suppressThisWarning() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ pretendError(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().pretendError(code, flag);
    }
    ಸ್ಥಿರ string ಸಾಲು_Str(ಧ್ರುವ ವರ್ಣ* filename, ಪೂರ್ಣಾಙ್ಕ lineno) VL_PURE;
    ಸ್ಥಿರ string stripMetaText(ಧ್ರುವ string& text, bool stripMeta) VL_PURE;
    ಸ್ಥಿರ V3ErrorCode errorCode() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        ಪ್ರತ್ಯಾಗಮ s().errorCode();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ಧ್ರುವ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* WARN_MORE = "__WARNMORE__";
    ಸ್ಥಿರ string warnMore() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ WARN_MORE; }
    // When printing an error/warning, mark beginning of context information (can nest)
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    ಸ್ಥಿರ string warnContextBegin() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* WARN_CONTEXT_END = "__WARNEND__";
    ಸ್ಥಿರ string warnContextEnd() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ WARN_CONTEXT_END; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    ಸ್ಥಿರ string warnAdditionalInfo() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    ಸ್ಥಿರ string warnContextNone() {
        V3Error::errorContexted(true);
        ಪ್ರತ್ಯಾಗಮ "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    ಸ್ಥಿರ std::ostringstream& v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex);
    ಸ್ಥಿರ std::ostringstream& v3error_Prep_File_ಸಾಲು(V3ErrorCode code, ಧ್ರುವ ವರ್ಣ* file, ಪೂರ್ಣಾಙ್ಕ ಸಾಲು)
        VL_ACQUIRE(s().m_mutex);
    ಸ್ಥಿರ std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { ಪ್ರತ್ಯಾಗಮ s().v3errorStr(); }
    // static, but often overridden in classes.
    ಸ್ಥಿರ ಶೂನ್ಯ v3errorEnd(ಧ್ರುವ std::ostringstream& sstr, ಧ್ರುವ string& extra, File_ಸಾಲು* fileline)
        VL_RELEASE(s().m_mutex);
    ಸ್ಥಿರ ಶೂನ್ಯ vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
ಶೂನ್ಯ v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE;
ಶೂನ್ಯ v3errorEndFatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE VL_ATTR_NORETURN;

#ifdef VL_MT_DISABLED_CODE_UNIT
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 1
#ಅನ್ಯಥಾ
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 0
#endif

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#define v3errorBuildMessage(prep, ಸಂದೇಶ) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << ಸಂದೇಶ))
#define v3warnCode(code, ಸಂದೇಶ) v3errorEnd(v3errorBuildMessage(V3Error::v3errorPrep(code), ಸಂದೇಶ))
#define v3warnCodeFatal(code, ಸಂದೇಶ) \
    v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(code), ಸಂದೇಶ))
#define v3warn(code, ಸಂದೇಶ) v3warnCode(V3ErrorCode::code, ಸಂದೇಶ)
#define v3info(ಸಂದೇಶ) v3warnCode(V3ErrorCode::EC_INFO, ಸಂದೇಶ)
#define v3error(ಸಂದೇಶ) v3warnCode(V3ErrorCode::EC_ERROR, ಸಂದೇಶ)
#define v3fatal(ಸಂದೇಶ) v3warnCodeFatal(V3ErrorCode::EC_FATAL, ಸಂದೇಶ)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#define v3fatalMany(ಸಂದೇಶ) v3warnCodeFatal(V3ErrorCode::EC_FATALMANY, ಸಂದೇಶ)
// Fatal exit; used instead of fatal() to mention the source code line
#define v3fatalSrc(ಸಂದೇಶ) \
    v3errorEndFatal(v3errorBuildMessage( \
        V3Error::v3error_Prep_File_ಸಾಲು(V3ErrorCode::EC_FATALSRC, __FILE__, __LINE__), ಸಂದೇಶ))
// Use this when normal v3fatal is called in static method that overrides fileline.
#define v3fatalStatic(ಸಂದೇಶ) \
    ::v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(V3ErrorCode::EC_FATAL), ಸಂದೇಶ))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#define UINFO(level, stmsg) \
    ಕುರು { \
        ಯದಿ (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::ಸಾಲು_Str(__FILE__, __LINE__) << stmsg; \
            ಯದಿ (ss.str()[ss.str().size() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } ಯಾವತ್ (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#define UINFO_PREFIX(stmsg) \
    ಕುರು { std::cout << "- " << V3Error::ಸಾಲು_Str(__FILE__, __LINE__) << stmsg; } ಯಾವತ್ (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#define UINFOTREE(level, nodep, uinfo_ಸಂದೇಶ, dumptree_ಸಂದೇಶ) \
    ಕುರು { \
        ಯದಿ (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_ಸಂದೇಶ; \
            ಯದಿ (!us.str().empty()) UINFO(level, us.str()); \
            ಯದಿ (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_ಸಂದೇಶ; \
                ಯದಿ (ss.str().empty()) ss << uinfo_ಸಂದೇಶ; \
                nodep->dump_ವೃಕ್ಷ("- " + V3Error::ಸಾಲು_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } ಯಾವತ್ (false)

/// Compile statements only when debug build
#ifdef VL_DEBUG
#define UDEBUGONLY(stmts) \
    ಕುರು { stmts } ಯಾವತ್ (false)
#ಅನ್ಯಥಾ
#define UDEBUGONLY(stmts) \
    ಕುರು { \
        ಯದಿ (false) { stmts } \
    } ಯಾವತ್ (false)
#endif

/// Assert without error location, generally UASSERT_OBJ preferred
#define UASSERT(condition, stmsg) \
    ಕುರು { \
        ಯದಿ (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } ಯಾವತ್ (false)
/// Assert with object to provide error location
#define UASSERT_OBJ(condition, obj, stmsg) \
    ಕುರು { \
        ಯದಿ (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } ಯಾವತ್ (false)
// For use in V3Ast static functions only
#define UASSERT_STATIC(condition, stmsg) \
    ಕುರು { \
        ಯದಿ (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } ಯಾವತ್ (false)
/// Check self test values for expected value.  Safe from side-effects.
#define UASSERT_SELFTEST(got, exp) \
    ಕುರು { \
        ಧ್ರುವ auto g = (got); \
        ಧ್ರುವ decltype(g) e{exp}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } ಯಾವತ್ (false)

// Error that call not supported; only for some Ast functions
#define V3ERROR_NA \
    ಕುರು { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } ಯಾವತ್ (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#define V3ERROR_NA_RETURN(ಮೌಲ್ಯ) \
    V3ERROR_NA; \
    ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DEBUG(...) \
    VL_ATTR_UNUSED ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ debug##__VA_ARGS__() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (ಶೂನ್ಯ)&debug##__VA_ARGS__; \
        ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_level = -1; \
        ಯದಿ (VL_UNLIKELY(s_level < 0)) { \
            std::string tag{VL_STRINGIFY(__VA_ARGS__)}; \
            ಯದಿ (!tag.empty()) tag[0] = std::tolower(tag[0]); \
            ಧ್ರುವ unsigned debugTag = v3Global.opt.debugLevel(tag); \
            ಧ್ರುವ unsigned debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ಧ್ರುವ unsigned debugLevel = debugTag >= debugSrc ? debugTag : debugSrc; \
            ಯದಿ (!v3Global.opt.available()) ಪ್ರತ್ಯಾಗಮ static_cast<ಪೂರ್ಣಾಙ್ಕ>(debugLevel); \
            s_level = static_cast<ಪೂರ್ಣಾಙ್ಕ>(debugLevel); \
        } \
        ಪ್ರತ್ಯಾಗಮ s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DUMP(func, tag) \
    VL_ATTR_UNUSED ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ dump##func() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (ಶೂನ್ಯ)&dump##func; \
        ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_level = -1; \
        ಯದಿ (VL_UNLIKELY(s_level < 0)) { \
            ಧ್ರುವ unsigned dumpTag = v3Global.opt.dumpLevel(tag); \
            ಧ್ರುವ unsigned dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ಧ್ರುವ unsigned dumpLevel = dumpTag >= dumpSrc ? dumpTag : dumpSrc; \
            ಯದಿ (!v3Global.opt.available()) ಪ್ರತ್ಯಾಗಮ static_cast<ಪೂರ್ಣಾಙ್ಕ>(dumpLevel); \
            s_level = static_cast<ಪೂರ್ಣಾಙ್ಕ>(dumpLevel); \
        } \
        ಪ್ರತ್ಯಾಗಮ s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#define VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(GraphLevel, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(ವೃಕ್ಷ_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(ವೃಕ್ಷ_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_UNUSED ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ dump_ವೃಕ್ಷ_Either_Level() { \
        /* Don't complain this function is unused */ (ಶೂನ್ಯ)&dump_ವೃಕ್ಷ_Either_Level; \
        ಪ್ರತ್ಯಾಗಮ dump_ವೃಕ್ಷ_Json_Level() >= dump_ವೃಕ್ಷ_Level() ? dump_ವೃಕ್ಷ_Json_Level() : dump_ವೃಕ್ಷ_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#endif  // Guard
