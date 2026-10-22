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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Hash.h"
#அந்தர்பாவ "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#அந்தர்பாவ "V3String.h"

#அந்தர்பாவ <array>
#அந்தர்பாவ <bitset>
#அந்தர்பாவ <cassert>
#அந்தர்பாவ <cctype>
#அந்தர்பாவ <deque>
#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <sstream>

class V3Error;
class File_வரி;

//######################################################################

class V3ErrorCode final {
பொது:
    // clang-format off
    எண்ணிக்கை en: uint8_t  {
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
    எண்ணிக்கை en m_e;
    V3ErrorCode()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3ErrorCode(en _e)
        : m_e{_e} {}
    explicit V3ErrorCode(மாறிலி எழுத்து* msgp);  // Matching code or ERROR
    explicit V3ErrorCode(மாறிலி std::string& செய்தி)
        : V3ErrorCode{செய்தி.c_str()} {}
    explicit V3ErrorCode(முழு _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() மாறிலி VL_MT_SAFE { திரும்பு m_e; }
    மாறிலி எழுத்து* ascii() மாறிலி VL_MT_SAFE {
        நிலையான மாறிலி எழுத்து* மாறிலி names[] = {
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
        திரும்பு names[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() மாறிலி VL_MT_SAFE {
        திரும்பு (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() மாறிலி VL_MT_SAFE { திரும்பு (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() மாறிலி VL_MT_SAFE {
        திரும்பு (m_e != EC_INFO && m_e < V3ErrorCode::EC_FIRST_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() மாறிலி VL_MT_SAFE {
        திரும்பு m_e == V3ErrorCode::EC_FATAL || m_e == V3ErrorCode::EC_FATALMANY
               || m_e == V3ErrorCode::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() மாறிலி VL_MT_SAFE {
        திரும்பு m_e == V3ErrorCode::USERINFO || m_e == V3ErrorCode::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() மாறிலி VL_MT_SAFE {
        திரும்பு (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() மாறிலி VL_MT_SAFE {
        திரும்பு (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() மாறிலி VL_MT_SAFE {
        திரும்பு (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == WIDTH
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() மாறிலி VL_MT_SAFE {
        திரும்பு (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() மாறிலி { திரும்பு m_e >= EC_FIRST_NAMED; }

தனிப்பட்ட:
    V3ErrorCode renamedTo() மாறிலி {
        // Return a new error this error has been renamed to
        எனில் (m_e == LITENDIAN) திரும்பு V3ErrorCode{ASCRANGE};
        திரும்பு V3ErrorCode{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() மாறிலி { திரும்பு renamedTo() != V3ErrorCode{EC_MIN}; }

பொது:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    வெறுமை forDelegateCodes(std::function<வெறுமை(V3ErrorCode code)> action) மாறிலி {
        // If add to this switch, also update isUnder()
        தேர்வு (m_e) {
        நிலை V3ErrorCode::E_UNSUPPORTED:
            action(V3ErrorCode{E_UNSUPPORTED});
            action(V3ErrorCode{COVERIGN});
            action(V3ErrorCode{SPECIFYIGN});
            திரும்பு;
        நிலை V3ErrorCode::I_LINT:
            க்கு (முழு i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                மாறிலி V3ErrorCode subcode{i};
                எனில் (subcode.lintError()) action(subcode);
            }
            திரும்பு;
        நிலை V3ErrorCode::I_STYLE:
            க்கு (முழு i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                மாறிலி V3ErrorCode subcode{i};
                எனில் (subcode.styleError()) action(subcode);
            }
            திரும்பு;
        நிலை V3ErrorCode::UNUSED:
            action(V3ErrorCode{UNUSEDGENVAR});
            action(V3ErrorCode{UNUSEDLOOP});
            action(V3ErrorCode{UNUSEDPARAM});
            action(V3ErrorCode{UNUSEDSIGNAL});
            திரும்பு;
        நிலை V3ErrorCode::WIDTH:
            action(V3ErrorCode{WIDTHDIGITS});
            action(V3ErrorCode{WIDTHEXPAND});
            action(V3ErrorCode{WIDTHTRUNC});
            action(V3ErrorCode{WIDTHXZEXPAND});
            திரும்பு;
        இயல்பு: action(*this);
        }
    }
    bool isUnder(V3ErrorCode other) மாறிலி {
        // Reverse of forDelegateCodes, return true if this' code is under another
        எனில் (VL_LIKELY(other == m_e)) திரும்பு true;
        தேர்வு (other) {
        நிலை V3ErrorCode::E_UNSUPPORTED:
            திரும்பு m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        நிலை V3ErrorCode::I_LINT: திரும்பு lintError();
        நிலை V3ErrorCode::I_STYLE: திரும்பு styleError();
        நிலை V3ErrorCode::UNUSED:
            திரும்பு m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        நிலை V3ErrorCode::WIDTH:
            திரும்பு m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        இயல்பு: திரும்பு false;
        }
    }
    string url() மாறிலி;
};
constexpr bool operator==(மாறிலி V3ErrorCode& lhs, மாறிலி V3ErrorCode& rhs) {
    திரும்பு lhs.m_e == rhs.m_e;
}
constexpr bool operator==(மாறிலி V3ErrorCode& lhs, V3ErrorCode::en rhs) { திரும்பு lhs.m_e == rhs; }
constexpr bool operator==(V3ErrorCode::en lhs, மாறிலி V3ErrorCode& rhs) { திரும்பு lhs == rhs.m_e; }
உள்வரி std::ostream& operator<<(std::ostream& os, மாறிலி V3ErrorCode& rhs) {
    திரும்பு os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VErrorBitSet final {
    std::bitset<V3ErrorCode::_ENUM_MAX> m_bitset;  // Enable for each code
    VErrorBitSet(மாறிலி VErrorBitSet& lhs, மாறிலி VErrorBitSet& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

பொது:
    class AllOnes {};
    VErrorBitSet() = இயல்பு;
    explicit VErrorBitSet(AllOnes) { m_bitset.set(); }
    ~VErrorBitSet() = இயல்பு;
    bool test(V3ErrorCode code) மாறிலி { திரும்பு m_bitset[code]; }
    வெறுமை set(V3ErrorCode code, bool flag) { m_bitset[code] = flag; }
    V3Hash hash() மாறிலி {
        மாறிலி size_t hashCode = std::hash<std::bitset<V3ErrorCode::_ENUM_MAX>>()(m_bitset);
        திரும்பு V3Hash{static_cast<uint64_t>(hashCode)};
    }
    VErrorBitSet operator&(மாறிலி VErrorBitSet& rhs) மாறிலி { திரும்பு VErrorBitSet{*this, rhs}; }
    bool operator==(மாறிலி VErrorBitSet& rhs) மாறிலி { திரும்பு m_bitset == rhs.m_bitset; }
    string ascii() மாறிலி {  // LCOV_EXCL_START
        string result;
        க்கு (முழு i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
            எனில் (!test(V3ErrorCode{i})) result += " !"s + V3ErrorCode{i}.ascii();
        }
        திரும்பு result;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VErrorMessage final {
    // TYPES
    using File_வரிகள் = std::deque<மாறிலி File_வரி*>;
    // MEMBERS
    V3ErrorCode m_code;  // Which warning
    string m_text;  // Warning message text
    File_வரி* m_fileline;  // Primary warning fileline
    File_வரிகள் m_filelines;  // Additional referenced filelines
பொது:
    // CONSTRUCTORS
    VErrorMessage() { clear(); }
    ~VErrorMessage() = இயல்பு;
    வெறுமை clear() { init(V3ErrorCode::EC_MIN); }
    வெறுமை init(V3ErrorCode code) {
        m_code = code;
        m_text = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3ErrorCode code() மாறிலி { திரும்பு m_code; }
    bool isClear() மாறிலி { திரும்பு m_code == V3ErrorCode::EC_MIN; }
    string text() மாறிலி { திரும்பு m_text; }
    வெறுமை text(மாறிலி string& செய்தி) { m_text = செய்தி; }
    File_வரி* fileline() மாறிலி { திரும்பு m_fileline; }
    வெறுமை fileline(File_வரி* fl) { m_fileline = fl; }
    File_வரிகள் filelines() மாறிலி { திரும்பு m_filelines; }
    // Returns identifier for use in warnRelated()
    முழு தள்ளு_Fileline(மாறிலி File_வரி* fl) {
        m_filelines.emplace_back(fl);
        திரும்பு m_filelines.size() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    friend class V3Error;

பொது:
    using MessagesSet = std::set<std::string>;
    using ErrorExitCb = வெறுமை (*)(வெறுமை);

தனிப்பட்ட:
    நிலையான constexpr unsigned MAX_ERRORS = 50;  // Fatal after this may errors

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
    முழு m_warn_மொத்தம் VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    முழு m_err_மொத்தம் VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VErrorBitSet m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VErrorBitSet m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VErrorBitSet m_showedSuppressed VL_GUARDED_BY(m_mutex);
    முழு m_debugDefault = 0;  // Option: --debugi Default debugging level
    முழு m_errorLimit VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    வெறுமை v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { திரும்பு m_errorStr; }
    வெறுமை v3errorEnd(மாறிலி std::ostringstream& sstr, மாறிலி string& extra, File_வரி* fileline)
        VL_REQUIRES(m_mutex);
    வெறுமை v3errorEndGuts(மாறிலி std::ostringstream& sstr, மாறிலி string& extra, File_வரி* fileline)
        VL_REQUIRES(m_mutex);

பொது:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string செய்தி_Prefix() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warnMoreSpaces() VL_REQUIRES(m_mutex);
    வெறுமை execErrorExitCb() VL_REQUIRES(m_mutex) {
        எனில் (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { திரும்பு m_errorExitCb; }
    வெறுமை errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3ErrorCode code, bool supp) VL_REQUIRES(m_mutex);
    வெறுமை vlAbortOrExit() VL_REQUIRES(m_mutex);
    வெறுமை errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    வெறுமை incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_மொத்தம்; }
    வெறுமை incErrors() VL_REQUIRES(m_mutex) { ++m_err_மொத்தம்; }
    முழு error_மொத்தம்() மாறிலி VL_REQUIRES(m_mutex) { திரும்பு m_err_மொத்தம்; }
    bool isErrorOrWarn() மாறிலி VL_REQUIRES(m_mutex) {
        திரும்பு error_மொத்தம்() || (warnFatal() && warn_மொத்தம்());
    }
    bool pretendError(V3ErrorCode code) VL_REQUIRES(m_mutex) { திரும்பு m_pretendError.test(code); }
    வெறுமை pretendError(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        code.forDelegateCodes([this, flag](V3ErrorCode subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.set(subcode, flag); });
    }
    முழு debugDefault() மாறிலி VL_MT_SAFE { திரும்பு m_debugDefault; }
    வெறுமை debugDefault(முழு level) VL_MT_UNSAFE { m_debugDefault = level; }
    முழு errorLimit() மாறிலி VL_REQUIRES(m_mutex) { திரும்பு m_errorLimit; }
    வெறுமை errorLimit(முழு level) VL_REQUIRES(m_mutex) { m_errorLimit = level; }
    bool warnFatal() மாறிலி VL_REQUIRES(m_mutex) { திரும்பு m_warnFatal; }
    வெறுமை warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3ErrorCode errorCode() மாறிலி VL_REQUIRES(m_mutex) { திரும்பு m_message.code(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { திரும்பு m_errorContexted; }
    முழு warn_மொத்தம்() மாறிலி VL_REQUIRES(m_mutex) { திரும்பு m_warn_மொத்தம்; }
    bool errorSuppressed() மாறிலி VL_REQUIRES(m_mutex) { திரும்பு m_errorSuppressed; }
    வெறுமை errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3ErrorCode code) VL_REQUIRES(m_mutex) {
        திரும்பு m_describedEachWarn.test(code);
    }
    வெறுமை describedEachWarn(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.set(code, flag);
    }
    வெறுமை suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(மாறிலி File_வரி* fl) VL_REQUIRES(m_mutex) {
        மாறிலி முழு id = m_message.தள்ளு_Fileline(fl);
        திரும்பு "__WARNRELATED("s + std::to_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        திரும்பு "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

பொது:
    V3Error() = delete;
    நிலையான V3ErrorGuarded& s() VL_MT_SAFE {  // Singleton
        நிலையான V3ErrorGuarded s_s;
        திரும்பு s_s;
    }

    // ACCESSORS
    நிலையான வெறுமை debugDefault(முழு level) VL_MT_UNSAFE { s().debugDefault(level); }
    நிலையான முழு debugDefault() VL_MT_SAFE { திரும்பு s().debugDefault(); }
    நிலையான வெறுமை errorLimit(முழு level) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().errorLimit(level);
    }
    நிலையான முழு errorLimit() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().errorLimit();
    }
    நிலையான வெறுமை warnFatal(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    நிலையான bool warnFatal() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().warnFatal();
    }
    // returns %Error/%Warn
    நிலையான string செய்தி_Prefix() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().செய்தி_Prefix();
    }
    நிலையான முழு error_மொத்தம்() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().error_மொத்தம்();
    }
    நிலையான bool pretendError(V3ErrorCode code) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().pretendError(code);
    }
    நிலையான முழு warn_மொத்தம்() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().warn_மொத்தம்();
    }
    நிலையான bool errorContexted() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().errorContexted();
    }
    நிலையான வெறுமை errorContexted(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    நிலையான வெறுமை describedEachWarn(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().describedEachWarn(code, flag);
    }
    // METHODS
    நிலையான வெறுமை incErrors() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().incErrors();
    }
    நிலையான வெறுமை incWarnings() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().incWarnings();
    }
    நிலையான வெறுமை init();
    நிலையான bool isError(V3ErrorCode code, bool supp) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().isError(code, supp);
    }
    நிலையான bool isErrorOrWarn() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().isErrorOrWarn();
    }
    நிலையான வெறுமை abortIfErrors() {
        எனில் (error_மொத்தம்()) abortIfWarnings();
    }
    நிலையான வெறுமை abortIfWarnings();
    // Suppress next %Warn if user has it off
    நிலையான வெறுமை suppressThisWarning() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    நிலையான வெறுமை pretendError(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().pretendError(code, flag);
    }
    நிலையான string வரி_Str(மாறிலி எழுத்து* filename, முழு lineno) VL_PURE;
    நிலையான string stripMetaText(மாறிலி string& text, bool stripMeta) VL_PURE;
    நிலையான V3ErrorCode errorCode() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        திரும்பு s().errorCode();
    }
    நிலையான வெறுமை errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        மாறிலி V3RecursiveLockGuard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    நிலையான constexpr மாறிலி எழுத்து* WARN_MORE = "__WARNMORE__";
    நிலையான string warnMore() VL_MT_SAFE { திரும்பு WARN_MORE; }
    // When printing an error/warning, mark beginning of context information (can nest)
    நிலையான constexpr மாறிலி எழுத்து* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    நிலையான string warnContextBegin() VL_MT_SAFE { திரும்பு WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    நிலையான constexpr மாறிலி எழுத்து* WARN_CONTEXT_END = "__WARNEND__";
    நிலையான string warnContextEnd() VL_MT_SAFE { திரும்பு WARN_CONTEXT_END; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    நிலையான string warnAdditionalInfo() VL_MT_SAFE { திரும்பு "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    நிலையான string warnContextNone() {
        V3Error::errorContexted(true);
        திரும்பு "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    நிலையான std::ostringstream& v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex);
    நிலையான std::ostringstream& v3error_Prep_File_வரி(V3ErrorCode code, மாறிலி எழுத்து* file, முழு வரி)
        VL_ACQUIRE(s().m_mutex);
    நிலையான std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { திரும்பு s().v3errorStr(); }
    // static, but often overridden in classes.
    நிலையான வெறுமை v3errorEnd(மாறிலி std::ostringstream& sstr, மாறிலி string& extra, File_வரி* fileline)
        VL_RELEASE(s().m_mutex);
    நிலையான வெறுமை vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
வெறுமை v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE;
வெறுமை v3errorEndFatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE VL_ATTR_NORETURN;

#ifdef VL_MT_DISABLED_CODE_UNIT
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 1
#இல்லையெனில்
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 0
#endif

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#define v3errorBuildMessage(prep, செய்தி) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << செய்தி))
#define v3warnCode(code, செய்தி) v3errorEnd(v3errorBuildMessage(V3Error::v3errorPrep(code), செய்தி))
#define v3warnCodeFatal(code, செய்தி) \
    v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(code), செய்தி))
#define v3warn(code, செய்தி) v3warnCode(V3ErrorCode::code, செய்தி)
#define v3info(செய்தி) v3warnCode(V3ErrorCode::EC_INFO, செய்தி)
#define v3error(செய்தி) v3warnCode(V3ErrorCode::EC_ERROR, செய்தி)
#define v3fatal(செய்தி) v3warnCodeFatal(V3ErrorCode::EC_FATAL, செய்தி)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#define v3fatalMany(செய்தி) v3warnCodeFatal(V3ErrorCode::EC_FATALMANY, செய்தி)
// Fatal exit; used instead of fatal() to mention the source code line
#define v3fatalSrc(செய்தி) \
    v3errorEndFatal(v3errorBuildMessage( \
        V3Error::v3error_Prep_File_வரி(V3ErrorCode::EC_FATALSRC, __FILE__, __LINE__), செய்தி))
// Use this when normal v3fatal is called in static method that overrides fileline.
#define v3fatalStatic(செய்தி) \
    ::v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(V3ErrorCode::EC_FATAL), செய்தி))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#define UINFO(level, stmsg) \
    செய் { \
        எனில் (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::வரி_Str(__FILE__, __LINE__) << stmsg; \
            எனில் (ss.str()[ss.str().size() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } போது (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#define UINFO_PREFIX(stmsg) \
    செய் { std::cout << "- " << V3Error::வரி_Str(__FILE__, __LINE__) << stmsg; } போது (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#define UINFOTREE(level, nodep, uinfo_செய்தி, dumptree_செய்தி) \
    செய் { \
        எனில் (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_செய்தி; \
            எனில் (!us.str().empty()) UINFO(level, us.str()); \
            எனில் (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_செய்தி; \
                எனில் (ss.str().empty()) ss << uinfo_செய்தி; \
                nodep->dump_மரம்("- " + V3Error::வரி_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } போது (false)

/// Compile statements only when debug build
#ifdef VL_DEBUG
#define UDEBUGONLY(stmts) \
    செய் { stmts } போது (false)
#இல்லையெனில்
#define UDEBUGONLY(stmts) \
    செய் { \
        எனில் (false) { stmts } \
    } போது (false)
#endif

/// Assert without error location, generally UASSERT_OBJ preferred
#define UASSERT(condition, stmsg) \
    செய் { \
        எனில் (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } போது (false)
/// Assert with object to provide error location
#define UASSERT_OBJ(condition, obj, stmsg) \
    செய் { \
        எனில் (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } போது (false)
// For use in V3Ast static functions only
#define UASSERT_STATIC(condition, stmsg) \
    செய் { \
        எனில் (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } போது (false)
/// Check self test values for expected value.  Safe from side-effects.
#define UASSERT_SELFTEST(got, exp) \
    செய் { \
        மாறிலி auto g = (got); \
        மாறிலி decltype(g) e{exp}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } போது (false)

// Error that call not supported; only for some Ast functions
#define V3ERROR_NA \
    செய் { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } போது (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#define V3ERROR_NA_RETURN(மதிப்பு) \
    V3ERROR_NA; \
    திரும்பு மதிப்பு

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DEBUG(...) \
    VL_ATTR_UNUSED நிலையான முழு debug##__VA_ARGS__() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (வெறுமை)&debug##__VA_ARGS__; \
        நிலையான முழு s_level = -1; \
        எனில் (VL_UNLIKELY(s_level < 0)) { \
            std::string tag{VL_STRINGIFY(__VA_ARGS__)}; \
            எனில் (!tag.empty()) tag[0] = std::tolower(tag[0]); \
            மாறிலி unsigned debugTag = v3Global.opt.debugLevel(tag); \
            மாறிலி unsigned debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            மாறிலி unsigned debugLevel = debugTag >= debugSrc ? debugTag : debugSrc; \
            எனில் (!v3Global.opt.available()) திரும்பு static_cast<முழு>(debugLevel); \
            s_level = static_cast<முழு>(debugLevel); \
        } \
        திரும்பு s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DUMP(func, tag) \
    VL_ATTR_UNUSED நிலையான முழு dump##func() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (வெறுமை)&dump##func; \
        நிலையான முழு s_level = -1; \
        எனில் (VL_UNLIKELY(s_level < 0)) { \
            மாறிலி unsigned dumpTag = v3Global.opt.dumpLevel(tag); \
            மாறிலி unsigned dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            மாறிலி unsigned dumpLevel = dumpTag >= dumpSrc ? dumpTag : dumpSrc; \
            எனில் (!v3Global.opt.available()) திரும்பு static_cast<முழு>(dumpLevel); \
            s_level = static_cast<முழு>(dumpLevel); \
        } \
        திரும்பு s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#define VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(GraphLevel, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(மரம்_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(மரம்_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_UNUSED நிலையான முழு dump_மரம்_Either_Level() { \
        /* Don't complain this function is unused */ (வெறுமை)&dump_மரம்_Either_Level; \
        திரும்பு dump_மரம்_Json_Level() >= dump_மரம்_Level() ? dump_மரம்_Json_Level() : dump_மரம்_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#endif  // Guard
