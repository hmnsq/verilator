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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ERROR_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ERROR_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hash.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <array>
#ਅਨ੍ਤਰ੍ਭਾਵ <bitset>
#ਅਨ੍ਤਰ੍ਭਾਵ <cassert>
#ਅਨ੍ਤਰ੍ਭਾਵ <cctype>
#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>

class V3Error;
class File_ਸਤਰ;

//######################################################################

class V3_Error_ਕੋਡ final {
ਜਨਤਕ:
    // clang-format off
    ਗਿਣਤੀ en: uint8_t  {
        EC_MIN=0,       // Keep first
        //
        EC_INFO,        // General information out
        EC_FATAL,       // Kill the program
        EC_FATALMANY,   // Kill the program, due to too many errors, suppress with --quiet-exit
        EC_FATALSRC,    // Kill the program, for internal source errors
        EC_ERROR,       // General error out, can't suppress
        EC_ਪਹਿਲਾ_NAMED,  // Just a code so the program knows where to start info/errors
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
        EC_ਪਹਿਲਾ_WARN,  // Just a code so the program knows where to start warnings
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
        ਅਣਵਰਤਿਆ_2,         // Unused genvar, parameter or signal message (Backward Compatibility)
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
        ਚੌੜਾਈ_2,          // Width mismatch
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
    ਗਿਣਤੀ en m_e;
    V3_Error_ਕੋਡ()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Error_ਕੋਡ(en _e)
        : m_e{_e} {}
    explicit V3_Error_ਕੋਡ(ਅਚਲ ਅੱਖਰ* msgp);  // Matching code or ERROR
    explicit V3_Error_ਕੋਡ(ਅਚਲ std::string& ਸੁਨੇਹਾ_2)
        : V3_Error_ਕੋਡ{ਸੁਨੇਹਾ_2.c_str()} {}
    explicit V3_Error_ਕੋਡ(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_e; }
    ਅਚਲ ਅੱਖਰ* ascii() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਸਥਿਰ ਅਚਲ ਅੱਖਰ* ਅਚਲ ਨਾਂ[] = {
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
        ਵਾਪਸ ਨਾਂ[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (m_e != EC_INFO && m_e < V3_Error_ਕੋਡ::EC_ਪਹਿਲਾ_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_e == V3_Error_ਕੋਡ::EC_FATAL || m_e == V3_Error_ਕੋਡ::EC_FATALMANY
               || m_e == V3_Error_ਕੋਡ::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_e == V3_Error_ਕੋਡ::USERINFO || m_e == V3_Error_ਕੋਡ::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == ਚੌੜਾਈ_2
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ਅਚਲ { ਵਾਪਸ m_e >= EC_ਪਹਿਲਾ_NAMED; }

ਨਿਜੀ:
    V3_Error_ਕੋਡ renamed_ਮੰਜ਼ਿਲ() ਅਚਲ {
        // Return a new error this error has been renamed to
        ਜੇ (m_e == LITENDIAN) ਵਾਪਸ V3_Error_ਕੋਡ{ASCRANGE};
        ਵਾਪਸ V3_Error_ਕੋਡ{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ਅਚਲ { ਵਾਪਸ renamed_ਮੰਜ਼ਿਲ() != V3_Error_ਕੋਡ{EC_MIN}; }

ਜਨਤਕ:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    ਖਾਲੀ forDelegateCodes(std::function<ਖਾਲੀ(V3_Error_ਕੋਡ ਕੋਡ)> action) ਅਚਲ {
        // If add to this switch, also update isUnder()
        ਚੋਣ (m_e) {
        ਹਾਲਤ V3_Error_ਕੋਡ::E_UNSUPPORTED:
            action(V3_Error_ਕੋਡ{E_UNSUPPORTED});
            action(V3_Error_ਕੋਡ{COVERIGN});
            action(V3_Error_ਕੋਡ{SPECIFYIGN});
            ਵਾਪਸ;
        ਹਾਲਤ V3_Error_ਕੋਡ::I_LINT:
            ਲਈ (ਪੂਰਨ i = V3_Error_ਕੋਡ::EC_MIN; i < V3_Error_ਕੋਡ::_ENUM_MAX; ++i) {
                ਅਚਲ V3_Error_ਕੋਡ subcode{i};
                ਜੇ (subcode.lintError()) action(subcode);
            }
            ਵਾਪਸ;
        ਹਾਲਤ V3_Error_ਕੋਡ::I_STYLE:
            ਲਈ (ਪੂਰਨ i = V3_Error_ਕੋਡ::EC_MIN; i < V3_Error_ਕੋਡ::_ENUM_MAX; ++i) {
                ਅਚਲ V3_Error_ਕੋਡ subcode{i};
                ਜੇ (subcode.styleError()) action(subcode);
            }
            ਵਾਪਸ;
        ਹਾਲਤ V3_Error_ਕੋਡ::ਅਣਵਰਤਿਆ_2:
            action(V3_Error_ਕੋਡ{UNUSEDGENVAR});
            action(V3_Error_ਕੋਡ{UNUSEDLOOP});
            action(V3_Error_ਕੋਡ{UNUSEDPARAM});
            action(V3_Error_ਕੋਡ{UNUSEDSIGNAL});
            ਵਾਪਸ;
        ਹਾਲਤ V3_Error_ਕੋਡ::ਚੌੜਾਈ_2:
            action(V3_Error_ਕੋਡ{WIDTHDIGITS});
            action(V3_Error_ਕੋਡ{WIDTHEXPAND});
            action(V3_Error_ਕੋਡ{WIDTHTRUNC});
            action(V3_Error_ਕੋਡ{WIDTHXZEXPAND});
            ਵਾਪਸ;
        ਮੂਲ: action(*this);
        }
    }
    bool isUnder(V3_Error_ਕੋਡ other) ਅਚਲ {
        // Reverse of forDelegateCodes, return true if this' code is under another
        ਜੇ (VL_LIKELY(other == m_e)) ਵਾਪਸ true;
        ਚੋਣ (other) {
        ਹਾਲਤ V3_Error_ਕੋਡ::E_UNSUPPORTED:
            ਵਾਪਸ m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        ਹਾਲਤ V3_Error_ਕੋਡ::I_LINT: ਵਾਪਸ lintError();
        ਹਾਲਤ V3_Error_ਕੋਡ::I_STYLE: ਵਾਪਸ styleError();
        ਹਾਲਤ V3_Error_ਕੋਡ::ਅਣਵਰਤਿਆ_2:
            ਵਾਪਸ m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        ਹਾਲਤ V3_Error_ਕੋਡ::ਚੌੜਾਈ_2:
            ਵਾਪਸ m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        ਮੂਲ: ਵਾਪਸ false;
        }
    }
    string url() ਅਚਲ;
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ V3_Error_ਕੋਡ& lhs, ਅਚਲ V3_Error_ਕੋਡ& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ V3_Error_ਕੋਡ& lhs, V3_Error_ਕੋਡ::en rhs) { ਵਾਪਸ lhs.m_e == rhs; }
constexpr bool ਸੰਚਾਲਕ==(V3_Error_ਕੋਡ::en lhs, ਅਚਲ V3_Error_ਕੋਡ& rhs) { ਵਾਪਸ lhs == rhs.m_e; }
ਸਤਰਅੰਦਰ std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, ਅਚਲ V3_Error_ਕੋਡ& rhs) {
    ਵਾਪਸ os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ final {
    std::bitset<V3_Error_ਕੋਡ::_ENUM_MAX> m_bitset;  // Enable for each code
    VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& lhs, ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

ਜਨਤਕ:
    class AllOnes {};
    VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ() = ਮੂਲ;
    explicit VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(AllOnes) { m_bitset.ਸੈੱਟ(); }
    ~VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ() = ਮੂਲ;
    bool test(V3_Error_ਕੋਡ ਕੋਡ) ਅਚਲ { ਵਾਪਸ m_bitset[ਕੋਡ]; }
    ਖਾਲੀ ਸੈੱਟ(V3_Error_ਕੋਡ ਕੋਡ, bool flag) { m_bitset[ਕੋਡ] = flag; }
    V3Hash hash() ਅਚਲ {
        ਅਚਲ ਮਾਪ_t hash_ਕੋਡ = std::hash<std::bitset<V3_Error_ਕੋਡ::_ENUM_MAX>>()(m_bitset);
        ਵਾਪਸ V3Hash{static_cast<uint64_t>(hash_ਕੋਡ)};
    }
    VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ ਸੰਚਾਲਕ&(ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& rhs) ਅਚਲ { ਵਾਪਸ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ{*this, rhs}; }
    bool ਸੰਚਾਲਕ==(ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& rhs) ਅਚਲ { ਵਾਪਸ m_bitset == rhs.m_bitset; }
    string ascii() ਅਚਲ {  // LCOV_EXCL_START
        string ਨਤੀਜਾ;
        ਲਈ (ਪੂਰਨ i = V3_Error_ਕੋਡ::EC_MIN; i < V3_Error_ਕੋਡ::_ENUM_MAX; ++i) {
            ਜੇ (!test(V3_Error_ਕੋਡ{i})) ਨਤੀਜਾ += " !"s + V3_Error_ਕੋਡ{i}.ascii();
        }
        ਵਾਪਸ ਨਤੀਜਾ;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VError_ਸੁਨੇਹਾ final {
    // TYPES
    using File_ਸਤਰਾਂ = std::deque<ਅਚਲ File_ਸਤਰ*>;
    // MEMBERS
    V3_Error_ਕੋਡ m_ਕੋਡ;  // Which warning
    string m_ਪਾਠ;  // Warning message text
    File_ਸਤਰ* m_fileline;  // Primary warning fileline
    File_ਸਤਰਾਂ m_filelines;  // Additional referenced filelines
ਜਨਤਕ:
    // CONSTRUCTORS
    VError_ਸੁਨੇਹਾ() { clear(); }
    ~VError_ਸੁਨੇਹਾ() = ਮੂਲ;
    ਖਾਲੀ clear() { init(V3_Error_ਕੋਡ::EC_MIN); }
    ਖਾਲੀ init(V3_Error_ਕੋਡ ਕੋਡ) {
        m_ਕੋਡ = ਕੋਡ;
        m_ਪਾਠ = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3_Error_ਕੋਡ ਕੋਡ() ਅਚਲ { ਵਾਪਸ m_ਕੋਡ; }
    bool isClear() ਅਚਲ { ਵਾਪਸ m_ਕੋਡ == V3_Error_ਕੋਡ::EC_MIN; }
    string ਪਾਠ() ਅਚਲ { ਵਾਪਸ m_ਪਾਠ; }
    ਖਾਲੀ ਪਾਠ(ਅਚਲ string& ਸੁਨੇਹਾ_2) { m_ਪਾਠ = ਸੁਨੇਹਾ_2; }
    File_ਸਤਰ* fileline() ਅਚਲ { ਵਾਪਸ m_fileline; }
    ਖਾਲੀ fileline(File_ਸਤਰ* fl) { m_fileline = fl; }
    File_ਸਤਰਾਂ filelines() ਅਚਲ { ਵਾਪਸ m_filelines; }
    // Returns identifier for use in warnRelated()
    ਪੂਰਨ ਧੱਕੋ_Fileline(ਅਚਲ File_ਸਤਰ* fl) {
        m_filelines.emplace_ਪਿੱਠ(fl);
        ਵਾਪਸ m_filelines.ਮਾਪ() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    ਮਿੱਤਰ class V3Error;

ਜਨਤਕ:
    using Messages_ਸੈੱਟ = std::ਸੈੱਟ<std::string>;
    using ErrorExitCb = ਖਾਲੀ (*)(ਖਾਲੀ);

ਨਿਜੀ:
    ਸਥਿਰ constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ MAX_ERRORS = 50;  // Fatal after this may errors

    // MEMBERS
    // Tell user to see manual, 0=not yet, 1=doit, 2=disable
    bool m_tellManual VL_GUARDED_BY(m_mutex) = false;
    bool m_tell_ਅੰਦਰੂਨੀ VL_GUARDED_BY(m_mutex) = false;
    VError_ਸੁਨੇਹਾ m_ਸੁਨੇਹਾ VL_GUARDED_BY(m_mutex);  // Message being formed
    bool m_errorSuppressed VL_GUARDED_BY(m_mutex)
        = false;  // Error being formed should be suppressed
    Messages_ਸੈੱਟ m_messages VL_GUARDED_BY(m_mutex);  // Errors outputted, to remove dups
    ErrorExitCb m_errorExitCb VL_GUARDED_BY(m_mutex)
        = nullptr;  // Callback when error occurs for dumping
    bool m_errorContexted VL_GUARDED_BY(m_mutex) = false;  // Error being formed got context
    ਪੂਰਨ m_warn_ਗਿਣਤੀ VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    ਪੂਰਨ m_err_ਗਿਣਤੀ VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ m_showedSuppressed VL_GUARDED_BY(m_mutex);
    ਪੂਰਨ m_debugDefault = 0;  // Option: --debugi Default debugging level
    ਪੂਰਨ m_error_ਹੱਦਬੰਦੀ VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    ਖਾਲੀ v3errorPrep(V3_Error_ਕੋਡ ਕੋਡ) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { ਵਾਪਸ m_errorStr; }
    ਖਾਲੀ v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& sstr, ਅਚਲ string& extra, File_ਸਤਰ* fileline)
        VL_REQUIRES(m_mutex);
    ਖਾਲੀ v3error_ਸਿਰਾ_Guts(ਅਚਲ std::ostringstream& sstr, ਅਚਲ string& extra, File_ਸਤਰ* fileline)
        VL_REQUIRES(m_mutex);

ਜਨਤਕ:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string ਸੁਨੇਹਾ_ਅਗੇਤਰ() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warn_ਹੋਰ_Spaces() VL_REQUIRES(m_mutex);
    ਖਾਲੀ execErrorExitCb() VL_REQUIRES(m_mutex) {
        ਜੇ (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { ਵਾਪਸ m_errorExitCb; }
    ਖਾਲੀ errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3_Error_ਕੋਡ ਕੋਡ, bool supp) VL_REQUIRES(m_mutex);
    ਖਾਲੀ vlAbortOrExit() VL_REQUIRES(m_mutex);
    ਖਾਲੀ errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    ਖਾਲੀ incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_ਗਿਣਤੀ; }
    ਖਾਲੀ incErrors() VL_REQUIRES(m_mutex) { ++m_err_ਗਿਣਤੀ; }
    ਪੂਰਨ error_ਗਿਣਤੀ() ਅਚਲ VL_REQUIRES(m_mutex) { ਵਾਪਸ m_err_ਗਿਣਤੀ; }
    bool isErrorOrWarn() ਅਚਲ VL_REQUIRES(m_mutex) {
        ਵਾਪਸ error_ਗਿਣਤੀ() || (warnFatal() && warn_ਗਿਣਤੀ());
    }
    bool pretendError(V3_Error_ਕੋਡ ਕੋਡ) VL_REQUIRES(m_mutex) { ਵਾਪਸ m_pretendError.test(ਕੋਡ); }
    ਖਾਲੀ pretendError(V3_Error_ਕੋਡ ਕੋਡ, bool flag) VL_REQUIRES(m_mutex) {
        ਕੋਡ.forDelegateCodes([this, flag](V3_Error_ਕੋਡ subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.ਸੈੱਟ(subcode, flag); });
    }
    ਪੂਰਨ debugDefault() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_debugDefault; }
    ਖਾਲੀ debugDefault(ਪੂਰਨ level) VL_MT_UNSAFE { m_debugDefault = level; }
    ਪੂਰਨ error_ਹੱਦਬੰਦੀ() ਅਚਲ VL_REQUIRES(m_mutex) { ਵਾਪਸ m_error_ਹੱਦਬੰਦੀ; }
    ਖਾਲੀ error_ਹੱਦਬੰਦੀ(ਪੂਰਨ level) VL_REQUIRES(m_mutex) { m_error_ਹੱਦਬੰਦੀ = level; }
    bool warnFatal() ਅਚਲ VL_REQUIRES(m_mutex) { ਵਾਪਸ m_warnFatal; }
    ਖਾਲੀ warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3_Error_ਕੋਡ error_ਕੋਡ() ਅਚਲ VL_REQUIRES(m_mutex) { ਵਾਪਸ m_ਸੁਨੇਹਾ.ਕੋਡ(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { ਵਾਪਸ m_errorContexted; }
    ਪੂਰਨ warn_ਗਿਣਤੀ() ਅਚਲ VL_REQUIRES(m_mutex) { ਵਾਪਸ m_warn_ਗਿਣਤੀ; }
    bool errorSuppressed() ਅਚਲ VL_REQUIRES(m_mutex) { ਵਾਪਸ m_errorSuppressed; }
    ਖਾਲੀ errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3_Error_ਕੋਡ ਕੋਡ) VL_REQUIRES(m_mutex) {
        ਵਾਪਸ m_describedEachWarn.test(ਕੋਡ);
    }
    ਖਾਲੀ describedEachWarn(V3_Error_ਕੋਡ ਕੋਡ, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.ਸੈੱਟ(ਕੋਡ, flag);
    }
    ਖਾਲੀ suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ਅਚਲ File_ਸਤਰ* fl) VL_REQUIRES(m_mutex) {
        ਅਚਲ ਪੂਰਨ id = m_ਸੁਨੇਹਾ.ਧੱਕੋ_Fileline(fl);
        ਵਾਪਸ "__WARNRELATED("s + std::ਮੰਜ਼ਿਲ_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        ਵਾਪਸ "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

ਜਨਤਕ:
    V3Error() = delete;
    ਸਥਿਰ V3ErrorGuarded& s() VL_MT_ਸੁਰੱਖਿਅਤ {  // Singleton
        ਸਥਿਰ V3ErrorGuarded s_s;
        ਵਾਪਸ s_s;
    }

    // ACCESSORS
    ਸਥਿਰ ਖਾਲੀ debugDefault(ਪੂਰਨ level) VL_MT_UNSAFE { s().debugDefault(level); }
    ਸਥਿਰ ਪੂਰਨ debugDefault() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ s().debugDefault(); }
    ਸਥਿਰ ਖਾਲੀ error_ਹੱਦਬੰਦੀ(ਪੂਰਨ level) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().error_ਹੱਦਬੰਦੀ(level);
    }
    ਸਥਿਰ ਪੂਰਨ error_ਹੱਦਬੰਦੀ() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().error_ਹੱਦਬੰਦੀ();
    }
    ਸਥਿਰ ਖਾਲੀ warnFatal(bool flag) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    ਸਥਿਰ bool warnFatal() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().warnFatal();
    }
    // returns %Error/%Warn
    ਸਥਿਰ string ਸੁਨੇਹਾ_ਅਗੇਤਰ() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().ਸੁਨੇਹਾ_ਅਗੇਤਰ();
    }
    ਸਥਿਰ ਪੂਰਨ error_ਗਿਣਤੀ() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().error_ਗਿਣਤੀ();
    }
    ਸਥਿਰ bool pretendError(V3_Error_ਕੋਡ ਕੋਡ) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().pretendError(ਕੋਡ);
    }
    ਸਥਿਰ ਪੂਰਨ warn_ਗਿਣਤੀ() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().warn_ਗਿਣਤੀ();
    }
    ਸਥਿਰ bool errorContexted() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().errorContexted();
    }
    ਸਥਿਰ ਖਾਲੀ errorContexted(bool flag) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    ਸਥਿਰ ਖਾਲੀ describedEachWarn(V3_Error_ਕੋਡ ਕੋਡ, bool flag) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().describedEachWarn(ਕੋਡ, flag);
    }
    // METHODS
    ਸਥਿਰ ਖਾਲੀ incErrors() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().incErrors();
    }
    ਸਥਿਰ ਖਾਲੀ incWarnings() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().incWarnings();
    }
    ਸਥਿਰ ਖਾਲੀ init();
    ਸਥਿਰ bool isError(V3_Error_ਕੋਡ ਕੋਡ, bool supp) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().isError(ਕੋਡ, supp);
    }
    ਸਥਿਰ bool isErrorOrWarn() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().isErrorOrWarn();
    }
    ਸਥਿਰ ਖਾਲੀ abortIfErrors() {
        ਜੇ (error_ਗਿਣਤੀ()) abortIfWarnings();
    }
    ਸਥਿਰ ਖਾਲੀ abortIfWarnings();
    // Suppress next %Warn if user has it off
    ਸਥਿਰ ਖਾਲੀ suppressThisWarning() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    ਸਥਿਰ ਖਾਲੀ pretendError(V3_Error_ਕੋਡ ਕੋਡ, bool flag) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().pretendError(ਕੋਡ, flag);
    }
    ਸਥਿਰ string ਸਤਰ_Str(ਅਚਲ ਅੱਖਰ* filename, ਪੂਰਨ lineno) VL_ਸ਼ੁੱਧ;
    ਸਥਿਰ string strip_Meta_ਪਾਠ(ਅਚਲ string& ਪਾਠ, bool stripMeta) VL_ਸ਼ੁੱਧ;
    ਸਥਿਰ V3_Error_ਕੋਡ error_ਕੋਡ() VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        ਵਾਪਸ s().error_ਕੋਡ();
    }
    ਸਥਿਰ ਖਾਲੀ errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(s().m_mutex) {
        ਅਚਲ V3_Recursive_ਤਾਲਾ_Guard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* WARN_ਹੋਰ = "__WARNMORE__";
    ਸਥਿਰ string warn_ਹੋਰ() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ WARN_ਹੋਰ; }
    // When printing an error/warning, mark beginning of context information (can nest)
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    ਸਥਿਰ string warnContextBegin() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* WARN_CONTEXT_ਸਿਰਾ = "__WARNEND__";
    ਸਥਿਰ string warn_Context_ਸਿਰਾ() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ WARN_CONTEXT_ਸਿਰਾ; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    ਸਥਿਰ string warnAdditionalInfo() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    ਸਥਿਰ string warnContextNone() {
        V3Error::errorContexted(true);
        ਵਾਪਸ "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    ਸਥਿਰ std::ostringstream& v3errorPrep(V3_Error_ਕੋਡ ਕੋਡ) VL_ACQUIRE(s().m_mutex);
    ਸਥਿਰ std::ostringstream& v3error_Prep_File_ਸਤਰ(V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ ਅੱਖਰ* file, ਪੂਰਨ ਸਤਰ)
        VL_ACQUIRE(s().m_mutex);
    ਸਥਿਰ std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { ਵਾਪਸ s().v3errorStr(); }
    // static, but often overridden in classes.
    ਸਥਿਰ ਖਾਲੀ v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& sstr, ਅਚਲ string& extra, File_ਸਤਰ* fileline)
        VL_RELEASE(s().m_mutex);
    ਸਥਿਰ ਖਾਲੀ vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
ਖਾਲੀ v3error_ਸਿਰਾ(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_ਸੁਰੱਖਿਅਤ;
ਖਾਲੀ v3error_ਸਿਰਾ_Fatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_ਸੁਰੱਖਿਅਤ VL_ATTR_NORETURN;

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_MT_DISABLED_ਕੋਡ_UNIT
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT_DEFINED 1
#ਨਹੀਂਤਾਂ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT_DEFINED 0
#ਜੇ_ਅੰਤ

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3error_Build_ਸੁਨੇਹਾ(prep, ਸੁਨੇਹਾ_2) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << ਸੁਨੇਹਾ_2))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3warn_ਕੋਡ(ਕੋਡ, ਸੁਨੇਹਾ_2) v3error_ਸਿਰਾ(v3error_Build_ਸੁਨੇਹਾ(V3Error::v3errorPrep(ਕੋਡ), ਸੁਨੇਹਾ_2))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3warn_ਕੋਡ_Fatal(ਕੋਡ, ਸੁਨੇਹਾ_2) \
    v3error_ਸਿਰਾ_Fatal(v3error_Build_ਸੁਨੇਹਾ(V3Error::v3errorPrep(ਕੋਡ), ਸੁਨੇਹਾ_2))
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3warn(ਕੋਡ, ਸੁਨੇਹਾ_2) v3warn_ਕੋਡ(V3_Error_ਕੋਡ::ਕੋਡ, ਸੁਨੇਹਾ_2)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3info(ਸੁਨੇਹਾ_2) v3warn_ਕੋਡ(V3_Error_ਕੋਡ::EC_INFO, ਸੁਨੇਹਾ_2)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3error(ਸੁਨੇਹਾ_2) v3warn_ਕੋਡ(V3_Error_ਕੋਡ::EC_ERROR, ਸੁਨੇਹਾ_2)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3fatal(ਸੁਨੇਹਾ_2) v3warn_ਕੋਡ_Fatal(V3_Error_ਕੋਡ::EC_FATAL, ਸੁਨੇਹਾ_2)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3fatalMany(ਸੁਨੇਹਾ_2) v3warn_ਕੋਡ_Fatal(V3_Error_ਕੋਡ::EC_FATALMANY, ਸੁਨੇਹਾ_2)
// Fatal exit; used instead of fatal() to mention the source code line
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3fatalSrc(ਸੁਨੇਹਾ_2) \
    v3error_ਸਿਰਾ_Fatal(v3error_Build_ਸੁਨੇਹਾ( \
        V3Error::v3error_Prep_File_ਸਤਰ(V3_Error_ਕੋਡ::EC_FATALSRC, __FILE__, __LINE__), ਸੁਨੇਹਾ_2))
// Use this when normal v3fatal is called in static method that overrides fileline.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3fatalStatic(ਸੁਨੇਹਾ_2) \
    ::v3error_ਸਿਰਾ_Fatal(v3error_Build_ਸੁਨੇਹਾ(V3Error::v3errorPrep(V3_Error_ਕੋਡ::EC_FATAL), ਸੁਨੇਹਾ_2))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UINFO(level, stmsg) \
    ਕਰੋ { \
        ਜੇ (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::ਸਤਰ_Str(__FILE__, __LINE__) << stmsg; \
            ਜੇ (ss.str()[ss.str().ਮਾਪ() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } ਜਦੋਂ (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UINFO_ਅਗੇਤਰ(stmsg) \
    ਕਰੋ { std::cout << "- " << V3Error::ਸਤਰ_Str(__FILE__, __LINE__) << stmsg; } ਜਦੋਂ (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UINFOTREE(level, nodep, uinfo_ਸੁਨੇਹਾ, dumptree_ਸੁਨੇਹਾ) \
    ਕਰੋ { \
        ਜੇ (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_ਸੁਨੇਹਾ; \
            ਜੇ (!us.str().ਖਾਲੀ_ਥਾਵਾਂ()) UINFO(level, us.str()); \
            ਜੇ (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_ਸੁਨੇਹਾ; \
                ਜੇ (ss.str().ਖਾਲੀ_ਥਾਵਾਂ()) ss << uinfo_ਸੁਨੇਹਾ; \
                nodep->dump_ਰੁੱਖ("- " + V3Error::ਸਤਰ_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } ਜਦੋਂ (false)

/// Compile statements only when debug build
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UDEBUGONLY(stmts) \
    ਕਰੋ { stmts } ਜਦੋਂ (false)
#ਨਹੀਂਤਾਂ
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UDEBUGONLY(stmts) \
    ਕਰੋ { \
        ਜੇ (false) { stmts } \
    } ਜਦੋਂ (false)
#ਜੇ_ਅੰਤ

/// Assert without error location, generally UASSERT_OBJ preferred
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UASSERT(condition, stmsg) \
    ਕਰੋ { \
        ਜੇ (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } ਜਦੋਂ (false)
/// Assert with object to provide error location
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UASSERT_OBJ(condition, obj, stmsg) \
    ਕਰੋ { \
        ਜੇ (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } ਜਦੋਂ (false)
// For use in V3Ast static functions only
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UASSERT_STATIC(condition, stmsg) \
    ਕਰੋ { \
        ਜੇ (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } ਜਦੋਂ (false)
/// Check self test values for expected value.  Safe from side-effects.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ UASSERT_SELFTEST(got, ਘਾਤਾਂਕ) \
    ਕਰੋ { \
        ਅਚਲ ਸਵੈਚਾਲਿਤ g = (got); \
        ਅਚਲ decltype(g) e{ਘਾਤਾਂਕ}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } ਜਦੋਂ (false)

// Error that call not supported; only for some Ast functions
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3ERROR_NA \
    ਕਰੋ { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } ਜਦੋਂ (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3ERROR_NA_RETURN(ਮੁੱਲ) \
    V3ERROR_NA; \
    ਵਾਪਸ ਮੁੱਲ

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_DEFINE_DEBUG(...) \
    VL_ATTR_ਅਣਵਰਤਿਆ ਸਥਿਰ ਪੂਰਨ debug##__VA_ARGS__() VL_MT_ਸੁਰੱਖਿਅਤ { \
        /* Don't complain this function is unused */ \
        (ਖਾਲੀ)&debug##__VA_ARGS__; \
        ਸਥਿਰ ਪੂਰਨ s_level = -1; \
        ਜੇ (VL_UNLIKELY(s_level < 0)) { \
            std::string ਟੈਗ{VL_STRINGIFY(__VA_ARGS__)}; \
            ਜੇ (!ਟੈਗ.ਖਾਲੀ_ਥਾਵਾਂ()) ਟੈਗ[0] = std::tolower(ਟੈਗ[0]); \
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ debug_ਟੈਗ = v3Global.opt.debugLevel(ਟੈਗ); \
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ debugLevel = debug_ਟੈਗ >= debugSrc ? debug_ਟੈਗ : debugSrc; \
            ਜੇ (!v3Global.opt.ਉਪਲਬਧ()) ਵਾਪਸ static_cast<ਪੂਰਨ>(debugLevel); \
            s_level = static_cast<ਪੂਰਨ>(debugLevel); \
        } \
        ਵਾਪਸ s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_DEFINE_DUMP(func, ਟੈਗ) \
    VL_ATTR_ਅਣਵਰਤਿਆ ਸਥਿਰ ਪੂਰਨ dump##func() VL_MT_ਸੁਰੱਖਿਅਤ { \
        /* Don't complain this function is unused */ \
        (ਖਾਲੀ)&dump##func; \
        ਸਥਿਰ ਪੂਰਨ s_level = -1; \
        ਜੇ (VL_UNLIKELY(s_level < 0)) { \
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ dump_ਟੈਗ = v3Global.opt.dumpLevel(ਟੈਗ); \
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ dumpLevel = dump_ਟੈਗ >= dumpSrc ? dump_ਟੈਗ : dumpSrc; \
            ਜੇ (!v3Global.opt.ਉਪਲਬਧ()) ਵਾਪਸ static_cast<ਪੂਰਨ>(dumpLevel); \
            s_level = static_cast<ਪੂਰਨ>(dumpLevel); \
        } \
        ਵਾਪਸ s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(ਗ੍ਰਾਫ_Level, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(ਰੁੱਖ_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(ਰੁੱਖ_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_ਅਣਵਰਤਿਆ ਸਥਿਰ ਪੂਰਨ dump_ਰੁੱਖ_Either_Level() { \
        /* Don't complain this function is unused */ (ਖਾਲੀ)&dump_ਰੁੱਖ_Either_Level; \
        ਵਾਪਸ dump_ਰੁੱਖ_Json_Level() >= dump_ਰੁੱਖ_Level() ? dump_ਰੁੱਖ_Json_Level() : dump_ਰੁੱਖ_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#ਜੇ_ਅੰਤ  // Guard
