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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Hash.h"
#ଅନ୍ତର୍ଭାଵ "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ଅନ୍ତର୍ଭାଵ <array>
#ଅନ୍ତର୍ଭାଵ <bitset>
#ଅନ୍ତର୍ଭାଵ <cassert>
#ଅନ୍ତର୍ଭାଵ <cctype>
#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <sstream>

class V3Error;
class File_ଧାଡ଼ି;

//######################################################################

class V3ErrorCode final {
ସର୍ବସାଧାରଣ:
    // clang-format off
    ଗଣନା en: uint8_t  {
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
    ଗଣନା en m_e;
    V3ErrorCode()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3ErrorCode(en _e)
        : m_e{_e} {}
    explicit V3ErrorCode(ଅଚଳ ଅକ୍ଷର* msgp);  // Matching code or ERROR
    explicit V3ErrorCode(ଅଚଳ std::string& ସନ୍ଦେଶ)
        : V3ErrorCode{ସନ୍ଦେଶ.c_str()} {}
    explicit V3ErrorCode(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_e; }
    ଅଚଳ ଅକ୍ଷର* ascii() ଅଚଳ VL_MT_SAFE {
        ସ୍ଥିର ଅଚଳ ଅକ୍ଷର* ଅଚଳ names[] = {
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
        ଫେରନ୍ତୁ names[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (m_e != EC_INFO && m_e < V3ErrorCode::EC_FIRST_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_e == V3ErrorCode::EC_FATAL || m_e == V3ErrorCode::EC_FATALMANY
               || m_e == V3ErrorCode::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_e == V3ErrorCode::USERINFO || m_e == V3ErrorCode::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == WIDTH
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ଅଚଳ { ଫେରନ୍ତୁ m_e >= EC_FIRST_NAMED; }

ବ୍ୟକ୍ତିଗତ:
    V3ErrorCode renamedTo() ଅଚଳ {
        // Return a new error this error has been renamed to
        ଯଦି (m_e == LITENDIAN) ଫେରନ୍ତୁ V3ErrorCode{ASCRANGE};
        ଫେରନ୍ତୁ V3ErrorCode{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ଅଚଳ { ଫେରନ୍ତୁ renamedTo() != V3ErrorCode{EC_MIN}; }

ସର୍ବସାଧାରଣ:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    ଶୂନ୍ୟ forDelegateCodes(std::function<ଶୂନ୍ୟ(V3ErrorCode code)> action) ଅଚଳ {
        // If add to this switch, also update isUnder()
        ବାଛ (m_e) {
        ସ୍ଥିତି V3ErrorCode::E_UNSUPPORTED:
            action(V3ErrorCode{E_UNSUPPORTED});
            action(V3ErrorCode{COVERIGN});
            action(V3ErrorCode{SPECIFYIGN});
            ଫେରନ୍ତୁ;
        ସ୍ଥିତି V3ErrorCode::I_LINT:
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                ଅଚଳ V3ErrorCode subcode{i};
                ଯଦି (subcode.lintError()) action(subcode);
            }
            ଫେରନ୍ତୁ;
        ସ୍ଥିତି V3ErrorCode::I_STYLE:
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                ଅଚଳ V3ErrorCode subcode{i};
                ଯଦି (subcode.styleError()) action(subcode);
            }
            ଫେରନ୍ତୁ;
        ସ୍ଥିତି V3ErrorCode::UNUSED:
            action(V3ErrorCode{UNUSEDGENVAR});
            action(V3ErrorCode{UNUSEDLOOP});
            action(V3ErrorCode{UNUSEDPARAM});
            action(V3ErrorCode{UNUSEDSIGNAL});
            ଫେରନ୍ତୁ;
        ସ୍ଥିତି V3ErrorCode::WIDTH:
            action(V3ErrorCode{WIDTHDIGITS});
            action(V3ErrorCode{WIDTHEXPAND});
            action(V3ErrorCode{WIDTHTRUNC});
            action(V3ErrorCode{WIDTHXZEXPAND});
            ଫେରନ୍ତୁ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: action(*this);
        }
    }
    bool isUnder(V3ErrorCode other) ଅଚଳ {
        // Reverse of forDelegateCodes, return true if this' code is under another
        ଯଦି (VL_LIKELY(other == m_e)) ଫେରନ୍ତୁ true;
        ବାଛ (other) {
        ସ୍ଥିତି V3ErrorCode::E_UNSUPPORTED:
            ଫେରନ୍ତୁ m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        ସ୍ଥିତି V3ErrorCode::I_LINT: ଫେରନ୍ତୁ lintError();
        ସ୍ଥିତି V3ErrorCode::I_STYLE: ଫେରନ୍ତୁ styleError();
        ସ୍ଥିତି V3ErrorCode::UNUSED:
            ଫେରନ୍ତୁ m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        ସ୍ଥିତି V3ErrorCode::WIDTH:
            ଫେରନ୍ତୁ m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଫେରନ୍ତୁ false;
        }
    }
    string url() ଅଚଳ;
};
constexpr bool operator==(ଅଚଳ V3ErrorCode& lhs, ଅଚଳ V3ErrorCode& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ V3ErrorCode& lhs, V3ErrorCode::en rhs) { ଫେରନ୍ତୁ lhs.m_e == rhs; }
constexpr bool operator==(V3ErrorCode::en lhs, ଅଚଳ V3ErrorCode& rhs) { ଫେରନ୍ତୁ lhs == rhs.m_e; }
ଧାଡ଼ିଭିତରେ std::ostream& operator<<(std::ostream& os, ଅଚଳ V3ErrorCode& rhs) {
    ଫେରନ୍ତୁ os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VErrorBitSet final {
    std::bitset<V3ErrorCode::_ENUM_MAX> m_bitset;  // Enable for each code
    VErrorBitSet(ଅଚଳ VErrorBitSet& lhs, ଅଚଳ VErrorBitSet& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

ସର୍ବସାଧାରଣ:
    class AllOnes {};
    VErrorBitSet() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    explicit VErrorBitSet(AllOnes) { m_bitset.set(); }
    ~VErrorBitSet() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    bool test(V3ErrorCode code) ଅଚଳ { ଫେରନ୍ତୁ m_bitset[code]; }
    ଶୂନ୍ୟ set(V3ErrorCode code, bool flag) { m_bitset[code] = flag; }
    V3Hash hash() ଅଚଳ {
        ଅଚଳ size_t hashCode = std::hash<std::bitset<V3ErrorCode::_ENUM_MAX>>()(m_bitset);
        ଫେରନ୍ତୁ V3Hash{static_cast<uint64_t>(hashCode)};
    }
    VErrorBitSet operator&(ଅଚଳ VErrorBitSet& rhs) ଅଚଳ { ଫେରନ୍ତୁ VErrorBitSet{*this, rhs}; }
    bool operator==(ଅଚଳ VErrorBitSet& rhs) ଅଚଳ { ଫେରନ୍ତୁ m_bitset == rhs.m_bitset; }
    string ascii() ଅଚଳ {  // LCOV_EXCL_START
        string result;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
            ଯଦି (!test(V3ErrorCode{i})) result += " !"s + V3ErrorCode{i}.ascii();
        }
        ଫେରନ୍ତୁ result;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VErrorMessage final {
    // TYPES
    using File_ଧାଡ଼ିଗୁଡ଼ିକ = std::deque<ଅଚଳ File_ଧାଡ଼ି*>;
    // MEMBERS
    V3ErrorCode m_code;  // Which warning
    string m_text;  // Warning message text
    File_ଧାଡ଼ି* m_fileline;  // Primary warning fileline
    File_ଧାଡ଼ିଗୁଡ଼ିକ m_filelines;  // Additional referenced filelines
ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    VErrorMessage() { clear(); }
    ~VErrorMessage() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ clear() { init(V3ErrorCode::EC_MIN); }
    ଶୂନ୍ୟ init(V3ErrorCode code) {
        m_code = code;
        m_text = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3ErrorCode code() ଅଚଳ { ଫେରନ୍ତୁ m_code; }
    bool isClear() ଅଚଳ { ଫେରନ୍ତୁ m_code == V3ErrorCode::EC_MIN; }
    string text() ଅଚଳ { ଫେରନ୍ତୁ m_text; }
    ଶୂନ୍ୟ text(ଅଚଳ string& ସନ୍ଦେଶ) { m_text = ସନ୍ଦେଶ; }
    File_ଧାଡ଼ି* fileline() ଅଚଳ { ଫେରନ୍ତୁ m_fileline; }
    ଶୂନ୍ୟ fileline(File_ଧାଡ଼ି* fl) { m_fileline = fl; }
    File_ଧାଡ଼ିଗୁଡ଼ିକ filelines() ଅଚଳ { ଫେରନ୍ତୁ m_filelines; }
    // Returns identifier for use in warnRelated()
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଠେଲ_Fileline(ଅଚଳ File_ଧାଡ଼ି* fl) {
        m_filelines.emplace_back(fl);
        ଫେରନ୍ତୁ m_filelines.size() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    friend class V3Error;

ସର୍ବସାଧାରଣ:
    using MessagesSet = std::set<std::string>;
    using ErrorExitCb = ଶୂନ୍ୟ (*)(ଶୂନ୍ୟ);

ବ୍ୟକ୍ତିଗତ:
    ସ୍ଥିର constexpr unsigned MAX_ERRORS = 50;  // Fatal after this may errors

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
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_warn_ଗଣନା VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_err_ଗଣନା VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VErrorBitSet m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VErrorBitSet m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VErrorBitSet m_showedSuppressed VL_GUARDED_BY(m_mutex);
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_debugDefault = 0;  // Option: --debugi Default debugging level
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_errorLimit VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    ଶୂନ୍ୟ v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_errorStr; }
    ଶୂନ୍ୟ v3errorEnd(ଅଚଳ std::ostringstream& sstr, ଅଚଳ string& extra, File_ଧାଡ଼ି* fileline)
        VL_REQUIRES(m_mutex);
    ଶୂନ୍ୟ v3errorEndGuts(ଅଚଳ std::ostringstream& sstr, ଅଚଳ string& extra, File_ଧାଡ଼ି* fileline)
        VL_REQUIRES(m_mutex);

ସର୍ବସାଧାରଣ:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string ସନ୍ଦେଶ_Prefix() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warnMoreSpaces() VL_REQUIRES(m_mutex);
    ଶୂନ୍ୟ execErrorExitCb() VL_REQUIRES(m_mutex) {
        ଯଦି (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_errorExitCb; }
    ଶୂନ୍ୟ errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3ErrorCode code, bool supp) VL_REQUIRES(m_mutex);
    ଶୂନ୍ୟ vlAbortOrExit() VL_REQUIRES(m_mutex);
    ଶୂନ୍ୟ errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    ଶୂନ୍ୟ incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_ଗଣନା; }
    ଶୂନ୍ୟ incErrors() VL_REQUIRES(m_mutex) { ++m_err_ଗଣନା; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା error_ଗଣନା() ଅଚଳ VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_err_ଗଣନା; }
    bool isErrorOrWarn() ଅଚଳ VL_REQUIRES(m_mutex) {
        ଫେରନ୍ତୁ error_ଗଣନା() || (warnFatal() && warn_ଗଣନା());
    }
    bool pretendError(V3ErrorCode code) VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_pretendError.test(code); }
    ଶୂନ୍ୟ pretendError(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        code.forDelegateCodes([this, flag](V3ErrorCode subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.set(subcode, flag); });
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା debugDefault() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_debugDefault; }
    ଶୂନ୍ୟ debugDefault(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level) VL_MT_UNSAFE { m_debugDefault = level; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା errorLimit() ଅଚଳ VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_errorLimit; }
    ଶୂନ୍ୟ errorLimit(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level) VL_REQUIRES(m_mutex) { m_errorLimit = level; }
    bool warnFatal() ଅଚଳ VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_warnFatal; }
    ଶୂନ୍ୟ warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3ErrorCode errorCode() ଅଚଳ VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_message.code(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_errorContexted; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା warn_ଗଣନା() ଅଚଳ VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_warn_ଗଣନା; }
    bool errorSuppressed() ଅଚଳ VL_REQUIRES(m_mutex) { ଫେରନ୍ତୁ m_errorSuppressed; }
    ଶୂନ୍ୟ errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3ErrorCode code) VL_REQUIRES(m_mutex) {
        ଫେରନ୍ତୁ m_describedEachWarn.test(code);
    }
    ଶୂନ୍ୟ describedEachWarn(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.set(code, flag);
    }
    ଶୂନ୍ୟ suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ଅଚଳ File_ଧାଡ଼ି* fl) VL_REQUIRES(m_mutex) {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା id = m_message.ଠେଲ_Fileline(fl);
        ଫେରନ୍ତୁ "__WARNRELATED("s + std::to_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        ଫେରନ୍ତୁ "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

ସର୍ବସାଧାରଣ:
    V3Error() = delete;
    ସ୍ଥିର V3ErrorGuarded& s() VL_MT_SAFE {  // Singleton
        ସ୍ଥିର V3ErrorGuarded s_s;
        ଫେରନ୍ତୁ s_s;
    }

    // ACCESSORS
    ସ୍ଥିର ଶୂନ୍ୟ debugDefault(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level) VL_MT_UNSAFE { s().debugDefault(level); }
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା debugDefault() VL_MT_SAFE { ଫେରନ୍ତୁ s().debugDefault(); }
    ସ୍ଥିର ଶୂନ୍ୟ errorLimit(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorLimit(level);
    }
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା errorLimit() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().errorLimit();
    }
    ସ୍ଥିର ଶୂନ୍ୟ warnFatal(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    ସ୍ଥିର bool warnFatal() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().warnFatal();
    }
    // returns %Error/%Warn
    ସ୍ଥିର string ସନ୍ଦେଶ_Prefix() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().ସନ୍ଦେଶ_Prefix();
    }
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା error_ଗଣନା() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().error_ଗଣନା();
    }
    ସ୍ଥିର bool pretendError(V3ErrorCode code) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().pretendError(code);
    }
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା warn_ଗଣନା() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().warn_ଗଣନା();
    }
    ସ୍ଥିର bool errorContexted() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().errorContexted();
    }
    ସ୍ଥିର ଶୂନ୍ୟ errorContexted(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    ସ୍ଥିର ଶୂନ୍ୟ describedEachWarn(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().describedEachWarn(code, flag);
    }
    // METHODS
    ସ୍ଥିର ଶୂନ୍ୟ incErrors() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().incErrors();
    }
    ସ୍ଥିର ଶୂନ୍ୟ incWarnings() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().incWarnings();
    }
    ସ୍ଥିର ଶୂନ୍ୟ init();
    ସ୍ଥିର bool isError(V3ErrorCode code, bool supp) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().isError(code, supp);
    }
    ସ୍ଥିର bool isErrorOrWarn() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().isErrorOrWarn();
    }
    ସ୍ଥିର ଶୂନ୍ୟ abortIfErrors() {
        ଯଦି (error_ଗଣନା()) abortIfWarnings();
    }
    ସ୍ଥିର ଶୂନ୍ୟ abortIfWarnings();
    // Suppress next %Warn if user has it off
    ସ୍ଥିର ଶୂନ୍ୟ suppressThisWarning() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    ସ୍ଥିର ଶୂନ୍ୟ pretendError(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().pretendError(code, flag);
    }
    ସ୍ଥିର string ଧାଡ଼ି_Str(ଅଚଳ ଅକ୍ଷର* filename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno) VL_PURE;
    ସ୍ଥିର string stripMetaText(ଅଚଳ string& text, bool stripMeta) VL_PURE;
    ସ୍ଥିର V3ErrorCode errorCode() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        ଫେରନ୍ତୁ s().errorCode();
    }
    ସ୍ଥିର ଶୂନ୍ୟ errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ଅଚଳ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* WARN_MORE = "__WARNMORE__";
    ସ୍ଥିର string warnMore() VL_MT_SAFE { ଫେରନ୍ତୁ WARN_MORE; }
    // When printing an error/warning, mark beginning of context information (can nest)
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    ସ୍ଥିର string warnContextBegin() VL_MT_SAFE { ଫେରନ୍ତୁ WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* WARN_CONTEXT_END = "__WARNEND__";
    ସ୍ଥିର string warnContextEnd() VL_MT_SAFE { ଫେରନ୍ତୁ WARN_CONTEXT_END; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    ସ୍ଥିର string warnAdditionalInfo() VL_MT_SAFE { ଫେରନ୍ତୁ "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    ସ୍ଥିର string warnContextNone() {
        V3Error::errorContexted(true);
        ଫେରନ୍ତୁ "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    ସ୍ଥିର std::ostringstream& v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex);
    ସ୍ଥିର std::ostringstream& v3error_Prep_File_ଧାଡ଼ି(V3ErrorCode code, ଅଚଳ ଅକ୍ଷର* file, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଧାଡ଼ି)
        VL_ACQUIRE(s().m_mutex);
    ସ୍ଥିର std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { ଫେରନ୍ତୁ s().v3errorStr(); }
    // static, but often overridden in classes.
    ସ୍ଥିର ଶୂନ୍ୟ v3errorEnd(ଅଚଳ std::ostringstream& sstr, ଅଚଳ string& extra, File_ଧାଡ଼ି* fileline)
        VL_RELEASE(s().m_mutex);
    ସ୍ଥିର ଶୂନ୍ୟ vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
ଶୂନ୍ୟ v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE;
ଶୂନ୍ୟ v3errorEndFatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE VL_ATTR_NORETURN;

#ifdef VL_MT_DISABLED_CODE_UNIT
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 1
#ଅନ୍ୟଥା
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 0
#endif

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#define v3errorBuildMessage(prep, ସନ୍ଦେଶ) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << ସନ୍ଦେଶ))
#define v3warnCode(code, ସନ୍ଦେଶ) v3errorEnd(v3errorBuildMessage(V3Error::v3errorPrep(code), ସନ୍ଦେଶ))
#define v3warnCodeFatal(code, ସନ୍ଦେଶ) \
    v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(code), ସନ୍ଦେଶ))
#define v3warn(code, ସନ୍ଦେଶ) v3warnCode(V3ErrorCode::code, ସନ୍ଦେଶ)
#define v3info(ସନ୍ଦେଶ) v3warnCode(V3ErrorCode::EC_INFO, ସନ୍ଦେଶ)
#define v3error(ସନ୍ଦେଶ) v3warnCode(V3ErrorCode::EC_ERROR, ସନ୍ଦେଶ)
#define v3fatal(ସନ୍ଦେଶ) v3warnCodeFatal(V3ErrorCode::EC_FATAL, ସନ୍ଦେଶ)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#define v3fatalMany(ସନ୍ଦେଶ) v3warnCodeFatal(V3ErrorCode::EC_FATALMANY, ସନ୍ଦେଶ)
// Fatal exit; used instead of fatal() to mention the source code line
#define v3fatalSrc(ସନ୍ଦେଶ) \
    v3errorEndFatal(v3errorBuildMessage( \
        V3Error::v3error_Prep_File_ଧାଡ଼ି(V3ErrorCode::EC_FATALSRC, __FILE__, __LINE__), ସନ୍ଦେଶ))
// Use this when normal v3fatal is called in static method that overrides fileline.
#define v3fatalStatic(ସନ୍ଦେଶ) \
    ::v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(V3ErrorCode::EC_FATAL), ସନ୍ଦେଶ))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#define UINFO(level, stmsg) \
    କର { \
        ଯଦି (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::ଧାଡ଼ି_Str(__FILE__, __LINE__) << stmsg; \
            ଯଦି (ss.str()[ss.str().size() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } ଯେତେବେଳେ (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#define UINFO_PREFIX(stmsg) \
    କର { std::cout << "- " << V3Error::ଧାଡ଼ି_Str(__FILE__, __LINE__) << stmsg; } ଯେତେବେଳେ (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#define UINFOTREE(level, nodep, uinfo_ସନ୍ଦେଶ, dumptree_ସନ୍ଦେଶ) \
    କର { \
        ଯଦି (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_ସନ୍ଦେଶ; \
            ଯଦି (!us.str().empty()) UINFO(level, us.str()); \
            ଯଦି (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_ସନ୍ଦେଶ; \
                ଯଦି (ss.str().empty()) ss << uinfo_ସନ୍ଦେଶ; \
                nodep->dump_ବୃକ୍ଷ("- " + V3Error::ଧାଡ଼ି_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } ଯେତେବେଳେ (false)

/// Compile statements only when debug build
#ifdef VL_DEBUG
#define UDEBUGONLY(stmts) \
    କର { stmts } ଯେତେବେଳେ (false)
#ଅନ୍ୟଥା
#define UDEBUGONLY(stmts) \
    କର { \
        ଯଦି (false) { stmts } \
    } ଯେତେବେଳେ (false)
#endif

/// Assert without error location, generally UASSERT_OBJ preferred
#define UASSERT(condition, stmsg) \
    କର { \
        ଯଦି (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } ଯେତେବେଳେ (false)
/// Assert with object to provide error location
#define UASSERT_OBJ(condition, obj, stmsg) \
    କର { \
        ଯଦି (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } ଯେତେବେଳେ (false)
// For use in V3Ast static functions only
#define UASSERT_STATIC(condition, stmsg) \
    କର { \
        ଯଦି (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } ଯେତେବେଳେ (false)
/// Check self test values for expected value.  Safe from side-effects.
#define UASSERT_SELFTEST(got, exp) \
    କର { \
        ଅଚଳ auto g = (got); \
        ଅଚଳ decltype(g) e{exp}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } ଯେତେବେଳେ (false)

// Error that call not supported; only for some Ast functions
#define V3ERROR_NA \
    କର { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } ଯେତେବେଳେ (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#define V3ERROR_NA_RETURN(ମୂଲ୍ୟ) \
    V3ERROR_NA; \
    ଫେରନ୍ତୁ ମୂଲ୍ୟ

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DEBUG(...) \
    VL_ATTR_UNUSED ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା debug##__VA_ARGS__() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (ଶୂନ୍ୟ)&debug##__VA_ARGS__; \
        ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_level = -1; \
        ଯଦି (VL_UNLIKELY(s_level < 0)) { \
            std::string tag{VL_STRINGIFY(__VA_ARGS__)}; \
            ଯଦି (!tag.empty()) tag[0] = std::tolower(tag[0]); \
            ଅଚଳ unsigned debugTag = v3Global.opt.debugLevel(tag); \
            ଅଚଳ unsigned debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ଅଚଳ unsigned debugLevel = debugTag >= debugSrc ? debugTag : debugSrc; \
            ଯଦି (!v3Global.opt.available()) ଫେରନ୍ତୁ static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(debugLevel); \
            s_level = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(debugLevel); \
        } \
        ଫେରନ୍ତୁ s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DUMP(func, tag) \
    VL_ATTR_UNUSED ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା dump##func() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (ଶୂନ୍ୟ)&dump##func; \
        ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_level = -1; \
        ଯଦି (VL_UNLIKELY(s_level < 0)) { \
            ଅଚଳ unsigned dumpTag = v3Global.opt.dumpLevel(tag); \
            ଅଚଳ unsigned dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ଅଚଳ unsigned dumpLevel = dumpTag >= dumpSrc ? dumpTag : dumpSrc; \
            ଯଦି (!v3Global.opt.available()) ଫେରନ୍ତୁ static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(dumpLevel); \
            s_level = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(dumpLevel); \
        } \
        ଫେରନ୍ତୁ s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#define VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(GraphLevel, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(ବୃକ୍ଷ_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(ବୃକ୍ଷ_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_UNUSED ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା dump_ବୃକ୍ଷ_Either_Level() { \
        /* Don't complain this function is unused */ (ଶୂନ୍ୟ)&dump_ବୃକ୍ଷ_Either_Level; \
        ଫେରନ୍ତୁ dump_ବୃକ୍ଷ_Json_Level() >= dump_ବୃକ୍ଷ_Level() ? dump_ବୃକ୍ଷ_Json_Level() : dump_ବୃକ୍ଷ_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#endif  // Guard
