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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Hash.h"
#અન્તર્ભાવ "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#અન્તર્ભાવ "V3String.h"

#અન્તર્ભાવ <array>
#અન્તર્ભાવ <bitset>
#અન્તર્ભાવ <cassert>
#અન્તર્ભાવ <cctype>
#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <sstream>

class V3Error;
class File_લીટી;

//######################################################################

class V3ErrorCode final {
જાહેર:
    // clang-format off
    ગણના en: uint8_t  {
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
    ગણના en m_e;
    V3ErrorCode()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3ErrorCode(en _e)
        : m_e{_e} {}
    explicit V3ErrorCode(અચલ અક્ષર* msgp);  // Matching code or ERROR
    explicit V3ErrorCode(અચલ std::string& સંદેશ)
        : V3ErrorCode{સંદેશ.c_str()} {}
    explicit V3ErrorCode(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ VL_MT_SAFE { પાછા m_e; }
    અચલ અક્ષર* ascii() અચલ VL_MT_SAFE {
        સ્થિર અચલ અક્ષર* અચલ names[] = {
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
        પાછા names[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() અચલ VL_MT_SAFE {
        પાછા (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() અચલ VL_MT_SAFE { પાછા (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() અચલ VL_MT_SAFE {
        પાછા (m_e != EC_INFO && m_e < V3ErrorCode::EC_FIRST_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() અચલ VL_MT_SAFE {
        પાછા m_e == V3ErrorCode::EC_FATAL || m_e == V3ErrorCode::EC_FATALMANY
               || m_e == V3ErrorCode::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() અચલ VL_MT_SAFE {
        પાછા m_e == V3ErrorCode::USERINFO || m_e == V3ErrorCode::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() અચલ VL_MT_SAFE {
        પાછા (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() અચલ VL_MT_SAFE {
        પાછા (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() અચલ VL_MT_SAFE {
        પાછા (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == WIDTH
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() અચલ VL_MT_SAFE {
        પાછા (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() અચલ { પાછા m_e >= EC_FIRST_NAMED; }

ખાનગી:
    V3ErrorCode renamedTo() અચલ {
        // Return a new error this error has been renamed to
        જો (m_e == LITENDIAN) પાછા V3ErrorCode{ASCRANGE};
        પાછા V3ErrorCode{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() અચલ { પાછા renamedTo() != V3ErrorCode{EC_MIN}; }

જાહેર:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    શૂન્ય forDelegateCodes(std::function<શૂન્ય(V3ErrorCode code)> action) અચલ {
        // If add to this switch, also update isUnder()
        પસંદગી (m_e) {
        સ્થિતિ V3ErrorCode::E_UNSUPPORTED:
            action(V3ErrorCode{E_UNSUPPORTED});
            action(V3ErrorCode{COVERIGN});
            action(V3ErrorCode{SPECIFYIGN});
            પાછા;
        સ્થિતિ V3ErrorCode::I_LINT:
            માટે (પૂર્ણાંક i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                અચલ V3ErrorCode subcode{i};
                જો (subcode.lintError()) action(subcode);
            }
            પાછા;
        સ્થિતિ V3ErrorCode::I_STYLE:
            માટે (પૂર્ણાંક i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                અચલ V3ErrorCode subcode{i};
                જો (subcode.styleError()) action(subcode);
            }
            પાછા;
        સ્થિતિ V3ErrorCode::UNUSED:
            action(V3ErrorCode{UNUSEDGENVAR});
            action(V3ErrorCode{UNUSEDLOOP});
            action(V3ErrorCode{UNUSEDPARAM});
            action(V3ErrorCode{UNUSEDSIGNAL});
            પાછા;
        સ્થિતિ V3ErrorCode::WIDTH:
            action(V3ErrorCode{WIDTHDIGITS});
            action(V3ErrorCode{WIDTHEXPAND});
            action(V3ErrorCode{WIDTHTRUNC});
            action(V3ErrorCode{WIDTHXZEXPAND});
            પાછા;
        મૂળભૂત: action(*this);
        }
    }
    bool isUnder(V3ErrorCode other) અચલ {
        // Reverse of forDelegateCodes, return true if this' code is under another
        જો (VL_LIKELY(other == m_e)) પાછા true;
        પસંદગી (other) {
        સ્થિતિ V3ErrorCode::E_UNSUPPORTED:
            પાછા m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        સ્થિતિ V3ErrorCode::I_LINT: પાછા lintError();
        સ્થિતિ V3ErrorCode::I_STYLE: પાછા styleError();
        સ્થિતિ V3ErrorCode::UNUSED:
            પાછા m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        સ્થિતિ V3ErrorCode::WIDTH:
            પાછા m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        મૂળભૂત: પાછા false;
        }
    }
    string url() અચલ;
};
constexpr bool operator==(અચલ V3ErrorCode& lhs, અચલ V3ErrorCode& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ V3ErrorCode& lhs, V3ErrorCode::en rhs) { પાછા lhs.m_e == rhs; }
constexpr bool operator==(V3ErrorCode::en lhs, અચલ V3ErrorCode& rhs) { પાછા lhs == rhs.m_e; }
અંતઃરેખ std::ostream& operator<<(std::ostream& os, અચલ V3ErrorCode& rhs) {
    પાછા os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VErrorBitSet final {
    std::bitset<V3ErrorCode::_ENUM_MAX> m_bitset;  // Enable for each code
    VErrorBitSet(અચલ VErrorBitSet& lhs, અચલ VErrorBitSet& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

જાહેર:
    class AllOnes {};
    VErrorBitSet() = મૂળભૂત;
    explicit VErrorBitSet(AllOnes) { m_bitset.set(); }
    ~VErrorBitSet() = મૂળભૂત;
    bool test(V3ErrorCode code) અચલ { પાછા m_bitset[code]; }
    શૂન્ય set(V3ErrorCode code, bool flag) { m_bitset[code] = flag; }
    V3Hash hash() અચલ {
        અચલ size_t hashCode = std::hash<std::bitset<V3ErrorCode::_ENUM_MAX>>()(m_bitset);
        પાછા V3Hash{static_cast<uint64_t>(hashCode)};
    }
    VErrorBitSet operator&(અચલ VErrorBitSet& rhs) અચલ { પાછા VErrorBitSet{*this, rhs}; }
    bool operator==(અચલ VErrorBitSet& rhs) અચલ { પાછા m_bitset == rhs.m_bitset; }
    string ascii() અચલ {  // LCOV_EXCL_START
        string result;
        માટે (પૂર્ણાંક i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
            જો (!test(V3ErrorCode{i})) result += " !"s + V3ErrorCode{i}.ascii();
        }
        પાછા result;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VErrorMessage final {
    // TYPES
    using File_લીટીઓ = std::deque<અચલ File_લીટી*>;
    // MEMBERS
    V3ErrorCode m_code;  // Which warning
    string m_text;  // Warning message text
    File_લીટી* m_fileline;  // Primary warning fileline
    File_લીટીઓ m_filelines;  // Additional referenced filelines
જાહેર:
    // CONSTRUCTORS
    VErrorMessage() { clear(); }
    ~VErrorMessage() = મૂળભૂત;
    શૂન્ય clear() { init(V3ErrorCode::EC_MIN); }
    શૂન્ય init(V3ErrorCode code) {
        m_code = code;
        m_text = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3ErrorCode code() અચલ { પાછા m_code; }
    bool isClear() અચલ { પાછા m_code == V3ErrorCode::EC_MIN; }
    string text() અચલ { પાછા m_text; }
    શૂન્ય text(અચલ string& સંદેશ) { m_text = સંદેશ; }
    File_લીટી* fileline() અચલ { પાછા m_fileline; }
    શૂન્ય fileline(File_લીટી* fl) { m_fileline = fl; }
    File_લીટીઓ filelines() અચલ { પાછા m_filelines; }
    // Returns identifier for use in warnRelated()
    પૂર્ણાંક ધકેલો_Fileline(અચલ File_લીટી* fl) {
        m_filelines.emplace_back(fl);
        પાછા m_filelines.size() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    friend class V3Error;

જાહેર:
    using MessagesSet = std::set<std::string>;
    using ErrorExitCb = શૂન્ય (*)(શૂન્ય);

ખાનગી:
    સ્થિર constexpr unsigned MAX_ERRORS = 50;  // Fatal after this may errors

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
    પૂર્ણાંક m_warn_ગણતરી VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    પૂર્ણાંક m_err_ગણતરી VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VErrorBitSet m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VErrorBitSet m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VErrorBitSet m_showedSuppressed VL_GUARDED_BY(m_mutex);
    પૂર્ણાંક m_debugDefault = 0;  // Option: --debugi Default debugging level
    પૂર્ણાંક m_errorLimit VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    શૂન્ય v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { પાછા m_errorStr; }
    શૂન્ય v3errorEnd(અચલ std::ostringstream& sstr, અચલ string& extra, File_લીટી* fileline)
        VL_REQUIRES(m_mutex);
    શૂન્ય v3errorEndGuts(અચલ std::ostringstream& sstr, અચલ string& extra, File_લીટી* fileline)
        VL_REQUIRES(m_mutex);

જાહેર:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string સંદેશ_Prefix() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warnMoreSpaces() VL_REQUIRES(m_mutex);
    શૂન્ય execErrorExitCb() VL_REQUIRES(m_mutex) {
        જો (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { પાછા m_errorExitCb; }
    શૂન્ય errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3ErrorCode code, bool supp) VL_REQUIRES(m_mutex);
    શૂન્ય vlAbortOrExit() VL_REQUIRES(m_mutex);
    શૂન્ય errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    શૂન્ય incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_ગણતરી; }
    શૂન્ય incErrors() VL_REQUIRES(m_mutex) { ++m_err_ગણતરી; }
    પૂર્ણાંક error_ગણતરી() અચલ VL_REQUIRES(m_mutex) { પાછા m_err_ગણતરી; }
    bool isErrorOrWarn() અચલ VL_REQUIRES(m_mutex) {
        પાછા error_ગણતરી() || (warnFatal() && warn_ગણતરી());
    }
    bool pretendError(V3ErrorCode code) VL_REQUIRES(m_mutex) { પાછા m_pretendError.test(code); }
    શૂન્ય pretendError(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        code.forDelegateCodes([this, flag](V3ErrorCode subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.set(subcode, flag); });
    }
    પૂર્ણાંક debugDefault() અચલ VL_MT_SAFE { પાછા m_debugDefault; }
    શૂન્ય debugDefault(પૂર્ણાંક level) VL_MT_UNSAFE { m_debugDefault = level; }
    પૂર્ણાંક errorLimit() અચલ VL_REQUIRES(m_mutex) { પાછા m_errorLimit; }
    શૂન્ય errorLimit(પૂર્ણાંક level) VL_REQUIRES(m_mutex) { m_errorLimit = level; }
    bool warnFatal() અચલ VL_REQUIRES(m_mutex) { પાછા m_warnFatal; }
    શૂન્ય warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3ErrorCode errorCode() અચલ VL_REQUIRES(m_mutex) { પાછા m_message.code(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { પાછા m_errorContexted; }
    પૂર્ણાંક warn_ગણતરી() અચલ VL_REQUIRES(m_mutex) { પાછા m_warn_ગણતરી; }
    bool errorSuppressed() અચલ VL_REQUIRES(m_mutex) { પાછા m_errorSuppressed; }
    શૂન્ય errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3ErrorCode code) VL_REQUIRES(m_mutex) {
        પાછા m_describedEachWarn.test(code);
    }
    શૂન્ય describedEachWarn(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.set(code, flag);
    }
    શૂન્ય suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(અચલ File_લીટી* fl) VL_REQUIRES(m_mutex) {
        અચલ પૂર્ણાંક id = m_message.ધકેલો_Fileline(fl);
        પાછા "__WARNRELATED("s + std::to_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        પાછા "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

જાહેર:
    V3Error() = delete;
    સ્થિર V3ErrorGuarded& s() VL_MT_SAFE {  // Singleton
        સ્થિર V3ErrorGuarded s_s;
        પાછા s_s;
    }

    // ACCESSORS
    સ્થિર શૂન્ય debugDefault(પૂર્ણાંક level) VL_MT_UNSAFE { s().debugDefault(level); }
    સ્થિર પૂર્ણાંક debugDefault() VL_MT_SAFE { પાછા s().debugDefault(); }
    સ્થિર શૂન્ય errorLimit(પૂર્ણાંક level) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorLimit(level);
    }
    સ્થિર પૂર્ણાંક errorLimit() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().errorLimit();
    }
    સ્થિર શૂન્ય warnFatal(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    સ્થિર bool warnFatal() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().warnFatal();
    }
    // returns %Error/%Warn
    સ્થિર string સંદેશ_Prefix() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().સંદેશ_Prefix();
    }
    સ્થિર પૂર્ણાંક error_ગણતરી() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().error_ગણતરી();
    }
    સ્થિર bool pretendError(V3ErrorCode code) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().pretendError(code);
    }
    સ્થિર પૂર્ણાંક warn_ગણતરી() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().warn_ગણતરી();
    }
    સ્થિર bool errorContexted() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().errorContexted();
    }
    સ્થિર શૂન્ય errorContexted(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    સ્થિર શૂન્ય describedEachWarn(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().describedEachWarn(code, flag);
    }
    // METHODS
    સ્થિર શૂન્ય incErrors() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().incErrors();
    }
    સ્થિર શૂન્ય incWarnings() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().incWarnings();
    }
    સ્થિર શૂન્ય init();
    સ્થિર bool isError(V3ErrorCode code, bool supp) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().isError(code, supp);
    }
    સ્થિર bool isErrorOrWarn() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().isErrorOrWarn();
    }
    સ્થિર શૂન્ય abortIfErrors() {
        જો (error_ગણતરી()) abortIfWarnings();
    }
    સ્થિર શૂન્ય abortIfWarnings();
    // Suppress next %Warn if user has it off
    સ્થિર શૂન્ય suppressThisWarning() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    સ્થિર શૂન્ય pretendError(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().pretendError(code, flag);
    }
    સ્થિર string લીટી_Str(અચલ અક્ષર* filename, પૂર્ણાંક lineno) VL_PURE;
    સ્થિર string stripMetaText(અચલ string& text, bool stripMeta) VL_PURE;
    સ્થિર V3ErrorCode errorCode() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        પાછા s().errorCode();
    }
    સ્થિર શૂન્ય errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        અચલ V3RecursiveLockGuard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    સ્થિર constexpr અચલ અક્ષર* WARN_MORE = "__WARNMORE__";
    સ્થિર string warnMore() VL_MT_SAFE { પાછા WARN_MORE; }
    // When printing an error/warning, mark beginning of context information (can nest)
    સ્થિર constexpr અચલ અક્ષર* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    સ્થિર string warnContextBegin() VL_MT_SAFE { પાછા WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    સ્થિર constexpr અચલ અક્ષર* WARN_CONTEXT_END = "__WARNEND__";
    સ્થિર string warnContextEnd() VL_MT_SAFE { પાછા WARN_CONTEXT_END; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    સ્થિર string warnAdditionalInfo() VL_MT_SAFE { પાછા "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    સ્થિર string warnContextNone() {
        V3Error::errorContexted(true);
        પાછા "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    સ્થિર std::ostringstream& v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex);
    સ્થિર std::ostringstream& v3error_Prep_File_લીટી(V3ErrorCode code, અચલ અક્ષર* file, પૂર્ણાંક લીટી)
        VL_ACQUIRE(s().m_mutex);
    સ્થિર std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { પાછા s().v3errorStr(); }
    // static, but often overridden in classes.
    સ્થિર શૂન્ય v3errorEnd(અચલ std::ostringstream& sstr, અચલ string& extra, File_લીટી* fileline)
        VL_RELEASE(s().m_mutex);
    સ્થિર શૂન્ય vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
શૂન્ય v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE;
શૂન્ય v3errorEndFatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE VL_ATTR_NORETURN;

#ifdef VL_MT_DISABLED_CODE_UNIT
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 1
#નહીંતર
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 0
#endif

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#define v3errorBuildMessage(prep, સંદેશ) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << સંદેશ))
#define v3warnCode(code, સંદેશ) v3errorEnd(v3errorBuildMessage(V3Error::v3errorPrep(code), સંદેશ))
#define v3warnCodeFatal(code, સંદેશ) \
    v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(code), સંદેશ))
#define v3warn(code, સંદેશ) v3warnCode(V3ErrorCode::code, સંદેશ)
#define v3info(સંદેશ) v3warnCode(V3ErrorCode::EC_INFO, સંદેશ)
#define v3error(સંદેશ) v3warnCode(V3ErrorCode::EC_ERROR, સંદેશ)
#define v3fatal(સંદેશ) v3warnCodeFatal(V3ErrorCode::EC_FATAL, સંદેશ)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#define v3fatalMany(સંદેશ) v3warnCodeFatal(V3ErrorCode::EC_FATALMANY, સંદેશ)
// Fatal exit; used instead of fatal() to mention the source code line
#define v3fatalSrc(સંદેશ) \
    v3errorEndFatal(v3errorBuildMessage( \
        V3Error::v3error_Prep_File_લીટી(V3ErrorCode::EC_FATALSRC, __FILE__, __LINE__), સંદેશ))
// Use this when normal v3fatal is called in static method that overrides fileline.
#define v3fatalStatic(સંદેશ) \
    ::v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(V3ErrorCode::EC_FATAL), સંદેશ))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#define UINFO(level, stmsg) \
    કરો { \
        જો (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::લીટી_Str(__FILE__, __LINE__) << stmsg; \
            જો (ss.str()[ss.str().size() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } જ્યારે (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#define UINFO_PREFIX(stmsg) \
    કરો { std::cout << "- " << V3Error::લીટી_Str(__FILE__, __LINE__) << stmsg; } જ્યારે (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#define UINFOTREE(level, nodep, uinfo_સંદેશ, dumptree_સંદેશ) \
    કરો { \
        જો (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_સંદેશ; \
            જો (!us.str().empty()) UINFO(level, us.str()); \
            જો (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_સંદેશ; \
                જો (ss.str().empty()) ss << uinfo_સંદેશ; \
                nodep->dump_વૃક્ષ("- " + V3Error::લીટી_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } જ્યારે (false)

/// Compile statements only when debug build
#ifdef VL_DEBUG
#define UDEBUGONLY(stmts) \
    કરો { stmts } જ્યારે (false)
#નહીંતર
#define UDEBUGONLY(stmts) \
    કરો { \
        જો (false) { stmts } \
    } જ્યારે (false)
#endif

/// Assert without error location, generally UASSERT_OBJ preferred
#define UASSERT(condition, stmsg) \
    કરો { \
        જો (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } જ્યારે (false)
/// Assert with object to provide error location
#define UASSERT_OBJ(condition, obj, stmsg) \
    કરો { \
        જો (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } જ્યારે (false)
// For use in V3Ast static functions only
#define UASSERT_STATIC(condition, stmsg) \
    કરો { \
        જો (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } જ્યારે (false)
/// Check self test values for expected value.  Safe from side-effects.
#define UASSERT_SELFTEST(got, exp) \
    કરો { \
        અચલ auto g = (got); \
        અચલ decltype(g) e{exp}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } જ્યારે (false)

// Error that call not supported; only for some Ast functions
#define V3ERROR_NA \
    કરો { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } જ્યારે (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#define V3ERROR_NA_RETURN(કિંમત) \
    V3ERROR_NA; \
    પાછા કિંમત

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DEBUG(...) \
    VL_ATTR_UNUSED સ્થિર પૂર્ણાંક debug##__VA_ARGS__() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (શૂન્ય)&debug##__VA_ARGS__; \
        સ્થિર પૂર્ણાંક s_level = -1; \
        જો (VL_UNLIKELY(s_level < 0)) { \
            std::string tag{VL_STRINGIFY(__VA_ARGS__)}; \
            જો (!tag.empty()) tag[0] = std::tolower(tag[0]); \
            અચલ unsigned debugTag = v3Global.opt.debugLevel(tag); \
            અચલ unsigned debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            અચલ unsigned debugLevel = debugTag >= debugSrc ? debugTag : debugSrc; \
            જો (!v3Global.opt.available()) પાછા static_cast<પૂર્ણાંક>(debugLevel); \
            s_level = static_cast<પૂર્ણાંક>(debugLevel); \
        } \
        પાછા s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DUMP(func, tag) \
    VL_ATTR_UNUSED સ્થિર પૂર્ણાંક dump##func() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (શૂન્ય)&dump##func; \
        સ્થિર પૂર્ણાંક s_level = -1; \
        જો (VL_UNLIKELY(s_level < 0)) { \
            અચલ unsigned dumpTag = v3Global.opt.dumpLevel(tag); \
            અચલ unsigned dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            અચલ unsigned dumpLevel = dumpTag >= dumpSrc ? dumpTag : dumpSrc; \
            જો (!v3Global.opt.available()) પાછા static_cast<પૂર્ણાંક>(dumpLevel); \
            s_level = static_cast<પૂર્ણાંક>(dumpLevel); \
        } \
        પાછા s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#define VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(GraphLevel, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(વૃક્ષ_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(વૃક્ષ_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_UNUSED સ્થિર પૂર્ણાંક dump_વૃક્ષ_Either_Level() { \
        /* Don't complain this function is unused */ (શૂન્ય)&dump_વૃક્ષ_Either_Level; \
        પાછા dump_વૃક્ષ_Json_Level() >= dump_વૃક્ષ_Level() ? dump_વૃક્ષ_Json_Level() : dump_વૃક્ષ_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#endif  // Guard
