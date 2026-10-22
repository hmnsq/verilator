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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ERROR_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ERROR_H_

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

class V3_Error_ಸಂಕೇತ final {
ಸಾರ್ವಜನಿಕ:
    // clang-format off
    ಎಣಿಕೆ en: uint8_t  {
        EC_MIN=0,       // Keep first
        //
        EC_INFO,        // General information out
        EC_FATAL,       // Kill the program
        EC_FATALMANY,   // Kill the program, due to too many errors, suppress with --quiet-exit
        EC_FATALSRC,    // Kill the program, for internal source errors
        EC_ERROR,       // General error out, can't suppress
        EC_ಮೊದಲ_NAMED,  // Just a code so the program knows where to start info/errors
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
        EC_ಮೊದಲ_WARN,  // Just a code so the program knows where to start warnings
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
        ಬಳಸದ_2,         // Unused genvar, parameter or signal message (Backward Compatibility)
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
        ಅಗಲ_2,          // Width mismatch
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
    ಎಣಿಕೆ en m_e;
    V3_Error_ಸಂಕೇತ()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Error_ಸಂಕೇತ(en _e)
        : m_e{_e} {}
    explicit V3_Error_ಸಂಕೇತ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* msgp);  // Matching code or ERROR
    explicit V3_Error_ಸಂಕೇತ(ಸ್ಥಿರಾಂಕ std::string& ಸಂದೇಶ_2)
        : V3_Error_ಸಂಕೇತ{ಸಂದೇಶ_2.c_str()} {}
    explicit V3_Error_ಸಂಕೇತ(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_e; }
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ascii() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ಹೆಸರುಗಳು[] = {
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
        ಹಿಂತಿರುಗಿ ಹೆಸರುಗಳು[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (m_e != EC_INFO && m_e < V3_Error_ಸಂಕೇತ::EC_ಮೊದಲ_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_e == V3_Error_ಸಂಕೇತ::EC_FATAL || m_e == V3_Error_ಸಂಕೇತ::EC_FATALMANY
               || m_e == V3_Error_ಸಂಕೇತ::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_e == V3_Error_ಸಂಕೇತ::USERINFO || m_e == V3_Error_ಸಂಕೇತ::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == ಅಗಲ_2
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e >= EC_ಮೊದಲ_NAMED; }

ಖಾಸಗಿ:
    V3_Error_ಸಂಕೇತ renamed_ಗಮ್ಯ() ಸ್ಥಿರಾಂಕ {
        // Return a new error this error has been renamed to
        ಆದರೆ (m_e == LITENDIAN) ಹಿಂತಿರುಗಿ V3_Error_ಸಂಕೇತ{ASCRANGE};
        ಹಿಂತಿರುಗಿ V3_Error_ಸಂಕೇತ{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ renamed_ಗಮ್ಯ() != V3_Error_ಸಂಕೇತ{EC_MIN}; }

ಸಾರ್ವಜನಿಕ:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    ಶೂನ್ಯ forDelegateCodes(std::function<ಶೂನ್ಯ(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2)> action) ಸ್ಥಿರಾಂಕ {
        // If add to this switch, also update isUnder()
        ಆಯ್ಕೆ (m_e) {
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::E_UNSUPPORTED:
            action(V3_Error_ಸಂಕೇತ{E_UNSUPPORTED});
            action(V3_Error_ಸಂಕೇತ{COVERIGN});
            action(V3_Error_ಸಂಕೇತ{SPECIFYIGN});
            ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::I_LINT:
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = V3_Error_ಸಂಕೇತ::EC_MIN; i < V3_Error_ಸಂಕೇತ::_ENUM_MAX; ++i) {
                ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ subcode{i};
                ಆದರೆ (subcode.lintError()) action(subcode);
            }
            ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::I_STYLE:
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = V3_Error_ಸಂಕೇತ::EC_MIN; i < V3_Error_ಸಂಕೇತ::_ENUM_MAX; ++i) {
                ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ subcode{i};
                ಆದರೆ (subcode.styleError()) action(subcode);
            }
            ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::ಬಳಸದ_2:
            action(V3_Error_ಸಂಕೇತ{UNUSEDGENVAR});
            action(V3_Error_ಸಂಕೇತ{UNUSEDLOOP});
            action(V3_Error_ಸಂಕೇತ{UNUSEDPARAM});
            action(V3_Error_ಸಂಕೇತ{UNUSEDSIGNAL});
            ಹಿಂತಿರುಗಿ;
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::ಅಗಲ_2:
            action(V3_Error_ಸಂಕೇತ{WIDTHDIGITS});
            action(V3_Error_ಸಂಕೇತ{WIDTHEXPAND});
            action(V3_Error_ಸಂಕೇತ{WIDTHTRUNC});
            action(V3_Error_ಸಂಕೇತ{WIDTHXZEXPAND});
            ಹಿಂತಿರುಗಿ;
        ಪೂರ್ವನಿಯೋಜಿತ: action(*this);
        }
    }
    bool isUnder(V3_Error_ಸಂಕೇತ other) ಸ್ಥಿರಾಂಕ {
        // Reverse of forDelegateCodes, return true if this' code is under another
        ಆದರೆ (VL_LIKELY(other == m_e)) ಹಿಂತಿರುಗಿ true;
        ಆಯ್ಕೆ (other) {
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::E_UNSUPPORTED:
            ಹಿಂತಿರುಗಿ m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::I_LINT: ಹಿಂತಿರುಗಿ lintError();
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::I_STYLE: ಹಿಂತಿರುಗಿ styleError();
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::ಬಳಸದ_2:
            ಹಿಂತಿರುಗಿ m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        ಸ್ಥಿತಿ V3_Error_ಸಂಕೇತ::ಅಗಲ_2:
            ಹಿಂತಿರುಗಿ m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ false;
        }
    }
    string url() ಸ್ಥಿರಾಂಕ;
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ& lhs, ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ& lhs, V3_Error_ಸಂಕೇತ::en rhs) { ಹಿಂತಿರುಗಿ lhs.m_e == rhs; }
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(V3_Error_ಸಂಕೇತ::en lhs, ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ& rhs) { ಹಿಂತಿರುಗಿ lhs == rhs.m_e; }
ಸಾಲೊಳಗೆ std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ& rhs) {
    ಹಿಂತಿರುಗಿ os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VError_ಬಿಟ್_ಗಣ final {
    std::bitset<V3_Error_ಸಂಕೇತ::_ENUM_MAX> m_bitset;  // Enable for each code
    VError_ಬಿಟ್_ಗಣ(ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& lhs, ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

ಸಾರ್ವಜನಿಕ:
    class AllOnes {};
    VError_ಬಿಟ್_ಗಣ() = ಪೂರ್ವನಿಯೋಜಿತ;
    explicit VError_ಬಿಟ್_ಗಣ(AllOnes) { m_bitset.ಗಣ(); }
    ~VError_ಬಿಟ್_ಗಣ() = ಪೂರ್ವನಿಯೋಜಿತ;
    bool test(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bitset[ಸಂಕೇತ_2]; }
    ಶೂನ್ಯ ಗಣ(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool flag) { m_bitset[ಸಂಕೇತ_2] = flag; }
    V3Hash hash() ಸ್ಥಿರಾಂಕ {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t hash_ಸಂಕೇತ = std::hash<std::bitset<V3_Error_ಸಂಕೇತ::_ENUM_MAX>>()(m_bitset);
        ಹಿಂತಿರುಗಿ V3Hash{static_cast<uint64_t>(hash_ಸಂಕೇತ)};
    }
    VError_ಬಿಟ್_ಗಣ ಕ್ರಿಯಾಕಾರಕ&(ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& rhs) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VError_ಬಿಟ್_ಗಣ{*this, rhs}; }
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& rhs) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_bitset == rhs.m_bitset; }
    string ascii() ಸ್ಥಿರಾಂಕ {  // LCOV_EXCL_START
        string ಫಲಿತಾಂಶ;
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = V3_Error_ಸಂಕೇತ::EC_MIN; i < V3_Error_ಸಂಕೇತ::_ENUM_MAX; ++i) {
            ಆದರೆ (!test(V3_Error_ಸಂಕೇತ{i})) ಫಲಿತಾಂಶ += " !"s + V3_Error_ಸಂಕೇತ{i}.ascii();
        }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VError_ಸಂದೇಶ final {
    // TYPES
    using File_ಸಾಲುಗಳು = std::deque<ಸ್ಥಿರಾಂಕ File_ಸಾಲು*>;
    // MEMBERS
    V3_Error_ಸಂಕೇತ m_ಸಂಕೇತ;  // Which warning
    string m_ಪಠ್ಯ;  // Warning message text
    File_ಸಾಲು* m_fileline;  // Primary warning fileline
    File_ಸಾಲುಗಳು m_filelines;  // Additional referenced filelines
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VError_ಸಂದೇಶ() { clear(); }
    ~VError_ಸಂದೇಶ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ clear() { init(V3_Error_ಸಂಕೇತ::EC_MIN); }
    ಶೂನ್ಯ init(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) {
        m_ಸಂಕೇತ = ಸಂಕೇತ_2;
        m_ಪಠ್ಯ = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಕೇತ; }
    bool isClear() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಕೇತ == V3_Error_ಸಂಕೇತ::EC_MIN; }
    string ಪಠ್ಯ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಪಠ್ಯ; }
    ಶೂನ್ಯ ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ_2) { m_ಪಠ್ಯ = ಸಂದೇಶ_2; }
    File_ಸಾಲು* fileline() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_fileline; }
    ಶೂನ್ಯ fileline(File_ಸಾಲು* fl) { m_fileline = fl; }
    File_ಸಾಲುಗಳು filelines() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_filelines; }
    // Returns identifier for use in warnRelated()
    ಪೂರ್ಣಾಂಕ ತಳ್ಳು_Fileline(ಸ್ಥಿರಾಂಕ File_ಸಾಲು* fl) {
        m_filelines.emplace_ಹಿಂಭಾಗ(fl);
        ಹಿಂತಿರುಗಿ m_filelines.ಗಾತ್ರ_2() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    ಮಿತ್ರ class V3Error;

ಸಾರ್ವಜನಿಕ:
    using Messages_ಗಣ = std::ಗಣ<std::string>;
    using ErrorExitCb = ಶೂನ್ಯ (*)(ಶೂನ್ಯ);

ಖಾಸಗಿ:
    ಸ್ಥಿರ constexpr ಚಿಹ್ನೆರಹಿತ MAX_ERRORS = 50;  // Fatal after this may errors

    // MEMBERS
    // Tell user to see manual, 0=not yet, 1=doit, 2=disable
    bool m_tellManual VL_GUARDED_BY(m_mutex) = false;
    bool m_tell_ಆಂತರಿಕ VL_GUARDED_BY(m_mutex) = false;
    VError_ಸಂದೇಶ m_ಸಂದೇಶ VL_GUARDED_BY(m_mutex);  // Message being formed
    bool m_errorSuppressed VL_GUARDED_BY(m_mutex)
        = false;  // Error being formed should be suppressed
    Messages_ಗಣ m_messages VL_GUARDED_BY(m_mutex);  // Errors outputted, to remove dups
    ErrorExitCb m_errorExitCb VL_GUARDED_BY(m_mutex)
        = nullptr;  // Callback when error occurs for dumping
    bool m_errorContexted VL_GUARDED_BY(m_mutex) = false;  // Error being formed got context
    ಪೂರ್ಣಾಂಕ m_warn_ಎಣಿಕೆ VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    ಪೂರ್ಣಾಂಕ m_err_ಎಣಿಕೆ VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VError_ಬಿಟ್_ಗಣ m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VError_ಬಿಟ್_ಗಣ m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VError_ಬಿಟ್_ಗಣ m_showedSuppressed VL_GUARDED_BY(m_mutex);
    ಪೂರ್ಣಾಂಕ m_debugDefault = 0;  // Option: --debugi Default debugging level
    ಪೂರ್ಣಾಂಕ m_error_ಎಲ್ಲೆ VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    ಶೂನ್ಯ v3errorPrep(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_errorStr; }
    ಶೂನ್ಯ v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& sstr, ಸ್ಥಿರಾಂಕ string& extra, File_ಸಾಲು* fileline)
        VL_REQUIRES(m_mutex);
    ಶೂನ್ಯ v3error_ಕೊನೆ_Guts(ಸ್ಥಿರಾಂಕ std::ostringstream& sstr, ಸ್ಥಿರಾಂಕ string& extra, File_ಸಾಲು* fileline)
        VL_REQUIRES(m_mutex);

ಸಾರ್ವಜನಿಕ:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string ಸಂದೇಶ_ಪೂರ್ವಪ್ರತ್ಯಯ() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warn_ಇನ್ನಷ್ಟು_Spaces() VL_REQUIRES(m_mutex);
    ಶೂನ್ಯ execErrorExitCb() VL_REQUIRES(m_mutex) {
        ಆದರೆ (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_errorExitCb; }
    ಶೂನ್ಯ errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool supp) VL_REQUIRES(m_mutex);
    ಶೂನ್ಯ vlAbortOrExit() VL_REQUIRES(m_mutex);
    ಶೂನ್ಯ errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    ಶೂನ್ಯ incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_ಎಣಿಕೆ; }
    ಶೂನ್ಯ incErrors() VL_REQUIRES(m_mutex) { ++m_err_ಎಣಿಕೆ; }
    ಪೂರ್ಣಾಂಕ error_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_err_ಎಣಿಕೆ; }
    bool isErrorOrWarn() ಸ್ಥಿರಾಂಕ VL_REQUIRES(m_mutex) {
        ಹಿಂತಿರುಗಿ error_ಎಣಿಕೆ() || (warnFatal() && warn_ಎಣಿಕೆ());
    }
    bool pretendError(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_pretendError.test(ಸಂಕೇತ_2); }
    ಶೂನ್ಯ pretendError(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool flag) VL_REQUIRES(m_mutex) {
        ಸಂಕೇತ_2.forDelegateCodes([this, flag](V3_Error_ಸಂಕೇತ subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.ಗಣ(subcode, flag); });
    }
    ಪೂರ್ಣಾಂಕ debugDefault() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_debugDefault; }
    ಶೂನ್ಯ debugDefault(ಪೂರ್ಣಾಂಕ level) VL_MT_UNSAFE { m_debugDefault = level; }
    ಪೂರ್ಣಾಂಕ error_ಎಲ್ಲೆ() ಸ್ಥಿರಾಂಕ VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_error_ಎಲ್ಲೆ; }
    ಶೂನ್ಯ error_ಎಲ್ಲೆ(ಪೂರ್ಣಾಂಕ level) VL_REQUIRES(m_mutex) { m_error_ಎಲ್ಲೆ = level; }
    bool warnFatal() ಸ್ಥಿರಾಂಕ VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_warnFatal; }
    ಶೂನ್ಯ warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3_Error_ಸಂಕೇತ error_ಸಂಕೇತ() ಸ್ಥಿರಾಂಕ VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_ಸಂದೇಶ.ಸಂಕೇತ_2(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_errorContexted; }
    ಪೂರ್ಣಾಂಕ warn_ಎಣಿಕೆ() ಸ್ಥಿರಾಂಕ VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_warn_ಎಣಿಕೆ; }
    bool errorSuppressed() ಸ್ಥಿರಾಂಕ VL_REQUIRES(m_mutex) { ಹಿಂತಿರುಗಿ m_errorSuppressed; }
    ಶೂನ್ಯ errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) VL_REQUIRES(m_mutex) {
        ಹಿಂತಿರುಗಿ m_describedEachWarn.test(ಸಂಕೇತ_2);
    }
    ಶೂನ್ಯ describedEachWarn(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.ಗಣ(ಸಂಕೇತ_2, flag);
    }
    ಶೂನ್ಯ suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ಸ್ಥಿರಾಂಕ File_ಸಾಲು* fl) VL_REQUIRES(m_mutex) {
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ id = m_ಸಂದೇಶ.ತಳ್ಳು_Fileline(fl);
        ಹಿಂತಿರುಗಿ "__WARNRELATED("s + std::ಗಮ್ಯ_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        ಹಿಂತಿರುಗಿ "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

ಸಾರ್ವಜನಿಕ:
    V3Error() = delete;
    ಸ್ಥಿರ V3ErrorGuarded& s() VL_MT_ಸುರಕ್ಷಿತ {  // Singleton
        ಸ್ಥಿರ V3ErrorGuarded s_s;
        ಹಿಂತಿರುಗಿ s_s;
    }

    // ACCESSORS
    ಸ್ಥಿರ ಶೂನ್ಯ debugDefault(ಪೂರ್ಣಾಂಕ level) VL_MT_UNSAFE { s().debugDefault(level); }
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ debugDefault() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ s().debugDefault(); }
    ಸ್ಥಿರ ಶೂನ್ಯ error_ಎಲ್ಲೆ(ಪೂರ್ಣಾಂಕ level) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().error_ಎಲ್ಲೆ(level);
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ error_ಎಲ್ಲೆ() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().error_ಎಲ್ಲೆ();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ warnFatal(bool flag) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    ಸ್ಥಿರ bool warnFatal() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().warnFatal();
    }
    // returns %Error/%Warn
    ಸ್ಥಿರ string ಸಂದೇಶ_ಪೂರ್ವಪ್ರತ್ಯಯ() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().ಸಂದೇಶ_ಪೂರ್ವಪ್ರತ್ಯಯ();
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ error_ಎಣಿಕೆ() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().error_ಎಣಿಕೆ();
    }
    ಸ್ಥಿರ bool pretendError(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().pretendError(ಸಂಕೇತ_2);
    }
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ warn_ಎಣಿಕೆ() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().warn_ಎಣಿಕೆ();
    }
    ಸ್ಥಿರ bool errorContexted() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().errorContexted();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ errorContexted(bool flag) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ describedEachWarn(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool flag) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().describedEachWarn(ಸಂಕೇತ_2, flag);
    }
    // METHODS
    ಸ್ಥಿರ ಶೂನ್ಯ incErrors() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().incErrors();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ incWarnings() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().incWarnings();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ init();
    ಸ್ಥಿರ bool isError(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool supp) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().isError(ಸಂಕೇತ_2, supp);
    }
    ಸ್ಥಿರ bool isErrorOrWarn() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().isErrorOrWarn();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ abortIfErrors() {
        ಆದರೆ (error_ಎಣಿಕೆ()) abortIfWarnings();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ abortIfWarnings();
    // Suppress next %Warn if user has it off
    ಸ್ಥಿರ ಶೂನ್ಯ suppressThisWarning() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ pretendError(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool flag) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().pretendError(ಸಂಕೇತ_2, flag);
    }
    ಸ್ಥಿರ string ಸಾಲು_Str(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* filename, ಪೂರ್ಣಾಂಕ lineno) VL_ಶುದ್ಧ;
    ಸ್ಥಿರ string strip_Meta_ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, bool stripMeta) VL_ಶುದ್ಧ;
    ಸ್ಥಿರ V3_Error_ಸಂಕೇತ error_ಸಂಕೇತ() VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        ಹಿಂತಿರುಗಿ s().error_ಸಂಕೇತ();
    }
    ಸ್ಥಿರ ಶೂನ್ಯ errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(s().m_mutex) {
        ಸ್ಥಿರಾಂಕ V3_Recursive_ಬೀಗ_Guard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* WARN_ಇನ್ನಷ್ಟು = "__WARNMORE__";
    ಸ್ಥಿರ string warn_ಇನ್ನಷ್ಟು() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ WARN_ಇನ್ನಷ್ಟು; }
    // When printing an error/warning, mark beginning of context information (can nest)
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    ಸ್ಥಿರ string warnContextBegin() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* WARN_CONTEXT_ಕೊನೆ = "__WARNEND__";
    ಸ್ಥಿರ string warn_Context_ಕೊನೆ() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ WARN_CONTEXT_ಕೊನೆ; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    ಸ್ಥಿರ string warnAdditionalInfo() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    ಸ್ಥಿರ string warnContextNone() {
        V3Error::errorContexted(true);
        ಹಿಂತಿರುಗಿ "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    ಸ್ಥಿರ std::ostringstream& v3errorPrep(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) VL_ACQUIRE(s().m_mutex);
    ಸ್ಥಿರ std::ostringstream& v3error_Prep_File_ಸಾಲು(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* file, ಪೂರ್ಣಾಂಕ ಸಾಲು)
        VL_ACQUIRE(s().m_mutex);
    ಸ್ಥಿರ std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { ಹಿಂತಿರುಗಿ s().v3errorStr(); }
    // static, but often overridden in classes.
    ಸ್ಥಿರ ಶೂನ್ಯ v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& sstr, ಸ್ಥಿರಾಂಕ string& extra, File_ಸಾಲು* fileline)
        VL_RELEASE(s().m_mutex);
    ಸ್ಥಿರ ಶೂನ್ಯ vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
ಶೂನ್ಯ v3error_ಕೊನೆ(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_ಸುರಕ್ಷಿತ;
ಶೂನ್ಯ v3error_ಕೊನೆ_Fatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_ಸುರಕ್ಷಿತ VL_ATTR_NORETURN;

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_MT_DISABLED_ಸಂಕೇತ_UNIT
#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT_DEFINED 1
#ಇಲ್ಲದಿದ್ದರೆ
#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT_DEFINED 0
#ಆದರೆ_ಅಂತ್ಯ

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#ವ್ಯಾಖ್ಯಾನಿಸು v3error_Build_ಸಂದೇಶ(prep, ಸಂದೇಶ_2) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << ಸಂದೇಶ_2))
#ವ್ಯಾಖ್ಯಾನಿಸು v3warn_ಸಂಕೇತ(ಸಂಕೇತ_2, ಸಂದೇಶ_2) v3error_ಕೊನೆ(v3error_Build_ಸಂದೇಶ(V3Error::v3errorPrep(ಸಂಕೇತ_2), ಸಂದೇಶ_2))
#ವ್ಯಾಖ್ಯಾನಿಸು v3warn_ಸಂಕೇತ_Fatal(ಸಂಕೇತ_2, ಸಂದೇಶ_2) \
    v3error_ಕೊನೆ_Fatal(v3error_Build_ಸಂದೇಶ(V3Error::v3errorPrep(ಸಂಕೇತ_2), ಸಂದೇಶ_2))
#ವ್ಯಾಖ್ಯಾನಿಸು v3warn(ಸಂಕೇತ_2, ಸಂದೇಶ_2) v3warn_ಸಂಕೇತ(V3_Error_ಸಂಕೇತ::ಸಂಕೇತ_2, ಸಂದೇಶ_2)
#ವ್ಯಾಖ್ಯಾನಿಸು v3info(ಸಂದೇಶ_2) v3warn_ಸಂಕೇತ(V3_Error_ಸಂಕೇತ::EC_INFO, ಸಂದೇಶ_2)
#ವ್ಯಾಖ್ಯಾನಿಸು v3error(ಸಂದೇಶ_2) v3warn_ಸಂಕೇತ(V3_Error_ಸಂಕೇತ::EC_ERROR, ಸಂದೇಶ_2)
#ವ್ಯಾಖ್ಯಾನಿಸು v3fatal(ಸಂದೇಶ_2) v3warn_ಸಂಕೇತ_Fatal(V3_Error_ಸಂಕೇತ::EC_FATAL, ಸಂದೇಶ_2)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#ವ್ಯಾಖ್ಯಾನಿಸು v3fatalMany(ಸಂದೇಶ_2) v3warn_ಸಂಕೇತ_Fatal(V3_Error_ಸಂಕೇತ::EC_FATALMANY, ಸಂದೇಶ_2)
// Fatal exit; used instead of fatal() to mention the source code line
#ವ್ಯಾಖ್ಯಾನಿಸು v3fatalSrc(ಸಂದೇಶ_2) \
    v3error_ಕೊನೆ_Fatal(v3error_Build_ಸಂದೇಶ( \
        V3Error::v3error_Prep_File_ಸಾಲು(V3_Error_ಸಂಕೇತ::EC_FATALSRC, __FILE__, __LINE__), ಸಂದೇಶ_2))
// Use this when normal v3fatal is called in static method that overrides fileline.
#ವ್ಯಾಖ್ಯಾನಿಸು v3fatalStatic(ಸಂದೇಶ_2) \
    ::v3error_ಕೊನೆ_Fatal(v3error_Build_ಸಂದೇಶ(V3Error::v3errorPrep(V3_Error_ಸಂಕೇತ::EC_FATAL), ಸಂದೇಶ_2))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#ವ್ಯಾಖ್ಯಾನಿಸು UINFO(level, stmsg) \
    ಮಾಡು { \
        ಆದರೆ (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::ಸಾಲು_Str(__FILE__, __LINE__) << stmsg; \
            ಆದರೆ (ss.str()[ss.str().ಗಾತ್ರ_2() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } ಆಗ (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#ವ್ಯಾಖ್ಯಾನಿಸು UINFO_ಪೂರ್ವಪ್ರತ್ಯಯ(stmsg) \
    ಮಾಡು { std::cout << "- " << V3Error::ಸಾಲು_Str(__FILE__, __LINE__) << stmsg; } ಆಗ (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#ವ್ಯಾಖ್ಯಾನಿಸು UINFOTREE(level, nodep, uinfo_ಸಂದೇಶ, dumptree_ಸಂದೇಶ) \
    ಮಾಡು { \
        ಆದರೆ (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_ಸಂದೇಶ; \
            ಆದರೆ (!us.str().ಖಾಲಿಗಳು()) UINFO(level, us.str()); \
            ಆದರೆ (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_ಸಂದೇಶ; \
                ಆದರೆ (ss.str().ಖಾಲಿಗಳು()) ss << uinfo_ಸಂದೇಶ; \
                nodep->dump_ವೃಕ್ಷ("- " + V3Error::ಸಾಲು_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } ಆಗ (false)

/// Compile statements only when debug build
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
#ವ್ಯಾಖ್ಯಾನಿಸು UDEBUGONLY(stmts) \
    ಮಾಡು { stmts } ಆಗ (false)
#ಇಲ್ಲದಿದ್ದರೆ
#ವ್ಯಾಖ್ಯಾನಿಸು UDEBUGONLY(stmts) \
    ಮಾಡು { \
        ಆದರೆ (false) { stmts } \
    } ಆಗ (false)
#ಆದರೆ_ಅಂತ್ಯ

/// Assert without error location, generally UASSERT_OBJ preferred
#ವ್ಯಾಖ್ಯಾನಿಸು UASSERT(condition, stmsg) \
    ಮಾಡು { \
        ಆದರೆ (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } ಆಗ (false)
/// Assert with object to provide error location
#ವ್ಯಾಖ್ಯಾನಿಸು UASSERT_OBJ(condition, obj, stmsg) \
    ಮಾಡು { \
        ಆದರೆ (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } ಆಗ (false)
// For use in V3Ast static functions only
#ವ್ಯಾಖ್ಯಾನಿಸು UASSERT_STATIC(condition, stmsg) \
    ಮಾಡು { \
        ಆದರೆ (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } ಆಗ (false)
/// Check self test values for expected value.  Safe from side-effects.
#ವ್ಯಾಖ್ಯಾನಿಸು UASSERT_SELFTEST(got, ಘಾತಾಂಕ) \
    ಮಾಡು { \
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ g = (got); \
        ಸ್ಥಿರಾಂಕ decltype(g) e{ಘಾತಾಂಕ}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } ಆಗ (false)

// Error that call not supported; only for some Ast functions
#ವ್ಯಾಖ್ಯಾನಿಸು V3ERROR_NA \
    ಮಾಡು { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } ಆಗ (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#ವ್ಯಾಖ್ಯಾನಿಸು V3ERROR_NA_RETURN(ಮೌಲ್ಯ) \
    V3ERROR_NA; \
    ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು VL_DEFINE_DEBUG(...) \
    VL_ATTR_ಬಳಸದ ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ debug##__VA_ARGS__() VL_MT_ಸುರಕ್ಷಿತ { \
        /* Don't complain this function is unused */ \
        (ಶೂನ್ಯ)&debug##__VA_ARGS__; \
        ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_level = -1; \
        ಆದರೆ (VL_UNLIKELY(s_level < 0)) { \
            std::string ಟ್ಯಾಗ್{VL_STRINGIFY(__VA_ARGS__)}; \
            ಆದರೆ (!ಟ್ಯಾಗ್.ಖಾಲಿಗಳು()) ಟ್ಯಾಗ್[0] = std::tolower(ಟ್ಯಾಗ್[0]); \
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ debug_ಟ್ಯಾಗ್ = v3Global.opt.debugLevel(ಟ್ಯಾಗ್); \
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ debugLevel = debug_ಟ್ಯಾಗ್ >= debugSrc ? debug_ಟ್ಯಾಗ್ : debugSrc; \
            ಆದರೆ (!v3Global.opt.ಲಭ್ಯ()) ಹಿಂತಿರುಗಿ static_cast<ಪೂರ್ಣಾಂಕ>(debugLevel); \
            s_level = static_cast<ಪೂರ್ಣಾಂಕ>(debugLevel); \
        } \
        ಹಿಂತಿರುಗಿ s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#ವ್ಯಾಖ್ಯಾನಿಸು VL_DEFINE_DUMP(func, ಟ್ಯಾಗ್) \
    VL_ATTR_ಬಳಸದ ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ dump##func() VL_MT_ಸುರಕ್ಷಿತ { \
        /* Don't complain this function is unused */ \
        (ಶೂನ್ಯ)&dump##func; \
        ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_level = -1; \
        ಆದರೆ (VL_UNLIKELY(s_level < 0)) { \
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ dump_ಟ್ಯಾಗ್ = v3Global.opt.dumpLevel(ಟ್ಯಾಗ್); \
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ dumpLevel = dump_ಟ್ಯಾಗ್ >= dumpSrc ? dump_ಟ್ಯಾಗ್ : dumpSrc; \
            ಆದರೆ (!v3Global.opt.ಲಭ್ಯ()) ಹಿಂತಿರುಗಿ static_cast<ಪೂರ್ಣಾಂಕ>(dumpLevel); \
            s_level = static_cast<ಪೂರ್ಣಾಂಕ>(dumpLevel); \
        } \
        ಹಿಂತಿರುಗಿ s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#ವ್ಯಾಖ್ಯಾನಿಸು VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(ನಕ್ಷೆ_Level, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(ವೃಕ್ಷ_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(ವೃಕ್ಷ_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_ಬಳಸದ ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ dump_ವೃಕ್ಷ_Either_Level() { \
        /* Don't complain this function is unused */ (ಶೂನ್ಯ)&dump_ವೃಕ್ಷ_Either_Level; \
        ಹಿಂತಿರುಗಿ dump_ವೃಕ್ಷ_Json_Level() >= dump_ವೃಕ್ಷ_Level() ? dump_ವೃಕ್ಷ_Json_Level() : dump_ವೃಕ್ಷ_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#ಆದರೆ_ಅಂತ್ಯ  // Guard
