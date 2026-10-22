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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Hash.h"
#شامل "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#شامل "V3String.h"

#شامل <array>
#شامل <bitset>
#شامل <cassert>
#شامل <cctype>
#شامل <deque>
#شامل <map>
#شامل <set>
#شامل <sstream>

class V3Error;
class File_سطر;

//######################################################################

class V3ErrorCode final {
عمومی:
    // clang-format off
    شمارش en: uint8_t  {
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
    شمارش en m_e;
    V3ErrorCode()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3ErrorCode(en _e)
        : m_e{_e} {}
    explicit V3ErrorCode(ثابت نویسه* msgp);  // Matching code or ERROR
    explicit V3ErrorCode(ثابت std::string& پیام)
        : V3ErrorCode{پیام.c_str()} {}
    explicit V3ErrorCode(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت VL_MT_SAFE { بازگشت m_e; }
    ثابت نویسه* ascii() ثابت VL_MT_SAFE {
        ایستا ثابت نویسه* ثابت names[] = {
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
        بازگشت names[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ثابت VL_MT_SAFE {
        بازگشت (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ثابت VL_MT_SAFE { بازگشت (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ثابت VL_MT_SAFE {
        بازگشت (m_e != EC_INFO && m_e < V3ErrorCode::EC_FIRST_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ثابت VL_MT_SAFE {
        بازگشت m_e == V3ErrorCode::EC_FATAL || m_e == V3ErrorCode::EC_FATALMANY
               || m_e == V3ErrorCode::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ثابت VL_MT_SAFE {
        بازگشت m_e == V3ErrorCode::USERINFO || m_e == V3ErrorCode::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ثابت VL_MT_SAFE {
        بازگشت (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ثابت VL_MT_SAFE {
        بازگشت (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ثابت VL_MT_SAFE {
        بازگشت (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == WIDTH
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ثابت VL_MT_SAFE {
        بازگشت (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ثابت { بازگشت m_e >= EC_FIRST_NAMED; }

خصوصی:
    V3ErrorCode renamedTo() ثابت {
        // Return a new error this error has been renamed to
        اگر (m_e == LITENDIAN) بازگشت V3ErrorCode{ASCRANGE};
        بازگشت V3ErrorCode{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ثابت { بازگشت renamedTo() != V3ErrorCode{EC_MIN}; }

عمومی:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    تهی forDelegateCodes(std::function<تهی(V3ErrorCode code)> action) ثابت {
        // If add to this switch, also update isUnder()
        انتخاب (m_e) {
        حالت V3ErrorCode::E_UNSUPPORTED:
            action(V3ErrorCode{E_UNSUPPORTED});
            action(V3ErrorCode{COVERIGN});
            action(V3ErrorCode{SPECIFYIGN});
            بازگشت;
        حالت V3ErrorCode::I_LINT:
            برای (صحیح i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                ثابت V3ErrorCode subcode{i};
                اگر (subcode.lintError()) action(subcode);
            }
            بازگشت;
        حالت V3ErrorCode::I_STYLE:
            برای (صحیح i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
                ثابت V3ErrorCode subcode{i};
                اگر (subcode.styleError()) action(subcode);
            }
            بازگشت;
        حالت V3ErrorCode::UNUSED:
            action(V3ErrorCode{UNUSEDGENVAR});
            action(V3ErrorCode{UNUSEDLOOP});
            action(V3ErrorCode{UNUSEDPARAM});
            action(V3ErrorCode{UNUSEDSIGNAL});
            بازگشت;
        حالت V3ErrorCode::WIDTH:
            action(V3ErrorCode{WIDTHDIGITS});
            action(V3ErrorCode{WIDTHEXPAND});
            action(V3ErrorCode{WIDTHTRUNC});
            action(V3ErrorCode{WIDTHXZEXPAND});
            بازگشت;
        پیشفرض: action(*this);
        }
    }
    bool isUnder(V3ErrorCode other) ثابت {
        // Reverse of forDelegateCodes, return true if this' code is under another
        اگر (VL_LIKELY(other == m_e)) بازگشت true;
        انتخاب (other) {
        حالت V3ErrorCode::E_UNSUPPORTED:
            بازگشت m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        حالت V3ErrorCode::I_LINT: بازگشت lintError();
        حالت V3ErrorCode::I_STYLE: بازگشت styleError();
        حالت V3ErrorCode::UNUSED:
            بازگشت m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        حالت V3ErrorCode::WIDTH:
            بازگشت m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        پیشفرض: بازگشت false;
        }
    }
    string url() ثابت;
};
constexpr bool operator==(ثابت V3ErrorCode& lhs, ثابت V3ErrorCode& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت V3ErrorCode& lhs, V3ErrorCode::en rhs) { بازگشت lhs.m_e == rhs; }
constexpr bool operator==(V3ErrorCode::en lhs, ثابت V3ErrorCode& rhs) { بازگشت lhs == rhs.m_e; }
درونخطی std::ostream& operator<<(std::ostream& os, ثابت V3ErrorCode& rhs) {
    بازگشت os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VErrorBitSet final {
    std::bitset<V3ErrorCode::_ENUM_MAX> m_bitset;  // Enable for each code
    VErrorBitSet(ثابت VErrorBitSet& lhs, ثابت VErrorBitSet& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

عمومی:
    class AllOnes {};
    VErrorBitSet() = پیشفرض;
    explicit VErrorBitSet(AllOnes) { m_bitset.set(); }
    ~VErrorBitSet() = پیشفرض;
    bool test(V3ErrorCode code) ثابت { بازگشت m_bitset[code]; }
    تهی set(V3ErrorCode code, bool flag) { m_bitset[code] = flag; }
    V3Hash hash() ثابت {
        ثابت size_t hashCode = std::hash<std::bitset<V3ErrorCode::_ENUM_MAX>>()(m_bitset);
        بازگشت V3Hash{static_cast<uint64_t>(hashCode)};
    }
    VErrorBitSet operator&(ثابت VErrorBitSet& rhs) ثابت { بازگشت VErrorBitSet{*this, rhs}; }
    bool operator==(ثابت VErrorBitSet& rhs) ثابت { بازگشت m_bitset == rhs.m_bitset; }
    string ascii() ثابت {  // LCOV_EXCL_START
        string result;
        برای (صحیح i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
            اگر (!test(V3ErrorCode{i})) result += " !"s + V3ErrorCode{i}.ascii();
        }
        بازگشت result;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VErrorMessage final {
    // TYPES
    using File_سطرها = std::deque<ثابت File_سطر*>;
    // MEMBERS
    V3ErrorCode m_code;  // Which warning
    string m_text;  // Warning message text
    File_سطر* m_fileline;  // Primary warning fileline
    File_سطرها m_filelines;  // Additional referenced filelines
عمومی:
    // CONSTRUCTORS
    VErrorMessage() { clear(); }
    ~VErrorMessage() = پیشفرض;
    تهی clear() { init(V3ErrorCode::EC_MIN); }
    تهی init(V3ErrorCode code) {
        m_code = code;
        m_text = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3ErrorCode code() ثابت { بازگشت m_code; }
    bool isClear() ثابت { بازگشت m_code == V3ErrorCode::EC_MIN; }
    string text() ثابت { بازگشت m_text; }
    تهی text(ثابت string& پیام) { m_text = پیام; }
    File_سطر* fileline() ثابت { بازگشت m_fileline; }
    تهی fileline(File_سطر* fl) { m_fileline = fl; }
    File_سطرها filelines() ثابت { بازگشت m_filelines; }
    // Returns identifier for use in warnRelated()
    صحیح افزودن_Fileline(ثابت File_سطر* fl) {
        m_filelines.emplace_back(fl);
        بازگشت m_filelines.size() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    friend class V3Error;

عمومی:
    using MessagesSet = std::set<std::string>;
    using ErrorExitCb = تهی (*)(تهی);

خصوصی:
    ایستا constexpr unsigned MAX_ERRORS = 50;  // Fatal after this may errors

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
    صحیح m_warn_شمار VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    صحیح m_err_شمار VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VErrorBitSet m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VErrorBitSet m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VErrorBitSet m_showedSuppressed VL_GUARDED_BY(m_mutex);
    صحیح m_debugDefault = 0;  // Option: --debugi Default debugging level
    صحیح m_errorLimit VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    تهی v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { بازگشت m_errorStr; }
    تهی v3errorEnd(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_REQUIRES(m_mutex);
    تهی v3errorEndGuts(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_REQUIRES(m_mutex);

عمومی:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string پیام_Prefix() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warnMoreSpaces() VL_REQUIRES(m_mutex);
    تهی execErrorExitCb() VL_REQUIRES(m_mutex) {
        اگر (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { بازگشت m_errorExitCb; }
    تهی errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3ErrorCode code, bool supp) VL_REQUIRES(m_mutex);
    تهی vlAbortOrExit() VL_REQUIRES(m_mutex);
    تهی errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    تهی incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_شمار; }
    تهی incErrors() VL_REQUIRES(m_mutex) { ++m_err_شمار; }
    صحیح error_شمار() ثابت VL_REQUIRES(m_mutex) { بازگشت m_err_شمار; }
    bool isErrorOrWarn() ثابت VL_REQUIRES(m_mutex) {
        بازگشت error_شمار() || (warnFatal() && warn_شمار());
    }
    bool pretendError(V3ErrorCode code) VL_REQUIRES(m_mutex) { بازگشت m_pretendError.test(code); }
    تهی pretendError(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        code.forDelegateCodes([this, flag](V3ErrorCode subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.set(subcode, flag); });
    }
    صحیح debugDefault() ثابت VL_MT_SAFE { بازگشت m_debugDefault; }
    تهی debugDefault(صحیح level) VL_MT_UNSAFE { m_debugDefault = level; }
    صحیح errorLimit() ثابت VL_REQUIRES(m_mutex) { بازگشت m_errorLimit; }
    تهی errorLimit(صحیح level) VL_REQUIRES(m_mutex) { m_errorLimit = level; }
    bool warnFatal() ثابت VL_REQUIRES(m_mutex) { بازگشت m_warnFatal; }
    تهی warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3ErrorCode errorCode() ثابت VL_REQUIRES(m_mutex) { بازگشت m_message.code(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { بازگشت m_errorContexted; }
    صحیح warn_شمار() ثابت VL_REQUIRES(m_mutex) { بازگشت m_warn_شمار; }
    bool errorSuppressed() ثابت VL_REQUIRES(m_mutex) { بازگشت m_errorSuppressed; }
    تهی errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3ErrorCode code) VL_REQUIRES(m_mutex) {
        بازگشت m_describedEachWarn.test(code);
    }
    تهی describedEachWarn(V3ErrorCode code, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.set(code, flag);
    }
    تهی suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ثابت File_سطر* fl) VL_REQUIRES(m_mutex) {
        ثابت صحیح id = m_message.افزودن_Fileline(fl);
        بازگشت "__WARNRELATED("s + std::to_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        بازگشت "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

عمومی:
    V3Error() = delete;
    ایستا V3ErrorGuarded& s() VL_MT_SAFE {  // Singleton
        ایستا V3ErrorGuarded s_s;
        بازگشت s_s;
    }

    // ACCESSORS
    ایستا تهی debugDefault(صحیح level) VL_MT_UNSAFE { s().debugDefault(level); }
    ایستا صحیح debugDefault() VL_MT_SAFE { بازگشت s().debugDefault(); }
    ایستا تهی errorLimit(صحیح level) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().errorLimit(level);
    }
    ایستا صحیح errorLimit() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().errorLimit();
    }
    ایستا تهی warnFatal(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    ایستا bool warnFatal() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().warnFatal();
    }
    // returns %Error/%Warn
    ایستا string پیام_Prefix() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().پیام_Prefix();
    }
    ایستا صحیح error_شمار() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().error_شمار();
    }
    ایستا bool pretendError(V3ErrorCode code) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().pretendError(code);
    }
    ایستا صحیح warn_شمار() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().warn_شمار();
    }
    ایستا bool errorContexted() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().errorContexted();
    }
    ایستا تهی errorContexted(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    ایستا تهی describedEachWarn(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().describedEachWarn(code, flag);
    }
    // METHODS
    ایستا تهی incErrors() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().incErrors();
    }
    ایستا تهی incWarnings() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().incWarnings();
    }
    ایستا تهی init();
    ایستا bool isError(V3ErrorCode code, bool supp) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().isError(code, supp);
    }
    ایستا bool isErrorOrWarn() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().isErrorOrWarn();
    }
    ایستا تهی abortIfErrors() {
        اگر (error_شمار()) abortIfWarnings();
    }
    ایستا تهی abortIfWarnings();
    // Suppress next %Warn if user has it off
    ایستا تهی suppressThisWarning() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    ایستا تهی pretendError(V3ErrorCode code, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().pretendError(code, flag);
    }
    ایستا string سطر_Str(ثابت نویسه* filename, صحیح lineno) VL_PURE;
    ایستا string stripMetaText(ثابت string& text, bool stripMeta) VL_PURE;
    ایستا V3ErrorCode errorCode() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        بازگشت s().errorCode();
    }
    ایستا تهی errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ثابت V3RecursiveLockGuard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    ایستا constexpr ثابت نویسه* WARN_MORE = "__WARNMORE__";
    ایستا string warnMore() VL_MT_SAFE { بازگشت WARN_MORE; }
    // When printing an error/warning, mark beginning of context information (can nest)
    ایستا constexpr ثابت نویسه* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    ایستا string warnContextBegin() VL_MT_SAFE { بازگشت WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    ایستا constexpr ثابت نویسه* WARN_CONTEXT_END = "__WARNEND__";
    ایستا string warnContextEnd() VL_MT_SAFE { بازگشت WARN_CONTEXT_END; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    ایستا string warnAdditionalInfo() VL_MT_SAFE { بازگشت "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    ایستا string warnContextNone() {
        V3Error::errorContexted(true);
        بازگشت "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    ایستا std::ostringstream& v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex);
    ایستا std::ostringstream& v3error_Prep_File_سطر(V3ErrorCode code, ثابت نویسه* file, صحیح سطر)
        VL_ACQUIRE(s().m_mutex);
    ایستا std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { بازگشت s().v3errorStr(); }
    // static, but often overridden in classes.
    ایستا تهی v3errorEnd(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_RELEASE(s().m_mutex);
    ایستا تهی vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
تهی v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE;
تهی v3errorEndFatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE VL_ATTR_NORETURN;

#ifdef VL_MT_DISABLED_CODE_UNIT
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 1
#وگرنه
#define VL_MT_DISABLED_CODE_UNIT_DEFINED 0
#endif

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#define v3errorBuildMessage(prep, پیام) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << پیام))
#define v3warnCode(code, پیام) v3errorEnd(v3errorBuildMessage(V3Error::v3errorPrep(code), پیام))
#define v3warnCodeFatal(code, پیام) \
    v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(code), پیام))
#define v3warn(code, پیام) v3warnCode(V3ErrorCode::code, پیام)
#define v3info(پیام) v3warnCode(V3ErrorCode::EC_INFO, پیام)
#define v3error(پیام) v3warnCode(V3ErrorCode::EC_ERROR, پیام)
#define v3fatal(پیام) v3warnCodeFatal(V3ErrorCode::EC_FATAL, پیام)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#define v3fatalMany(پیام) v3warnCodeFatal(V3ErrorCode::EC_FATALMANY, پیام)
// Fatal exit; used instead of fatal() to mention the source code line
#define v3fatalSrc(پیام) \
    v3errorEndFatal(v3errorBuildMessage( \
        V3Error::v3error_Prep_File_سطر(V3ErrorCode::EC_FATALSRC, __FILE__, __LINE__), پیام))
// Use this when normal v3fatal is called in static method that overrides fileline.
#define v3fatalStatic(پیام) \
    ::v3errorEndFatal(v3errorBuildMessage(V3Error::v3errorPrep(V3ErrorCode::EC_FATAL), پیام))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#define UINFO(level, stmsg) \
    انجامبده { \
        اگر (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::سطر_Str(__FILE__, __LINE__) << stmsg; \
            اگر (ss.str()[ss.str().size() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } وقتی (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#define UINFO_PREFIX(stmsg) \
    انجامبده { std::cout << "- " << V3Error::سطر_Str(__FILE__, __LINE__) << stmsg; } وقتی (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#define UINFOTREE(level, nodep, uinfo_پیام, dumptree_پیام) \
    انجامبده { \
        اگر (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_پیام; \
            اگر (!us.str().empty()) UINFO(level, us.str()); \
            اگر (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_پیام; \
                اگر (ss.str().empty()) ss << uinfo_پیام; \
                nodep->dump_درخت("- " + V3Error::سطر_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } وقتی (false)

/// Compile statements only when debug build
#ifdef VL_DEBUG
#define UDEBUGONLY(stmts) \
    انجامبده { stmts } وقتی (false)
#وگرنه
#define UDEBUGONLY(stmts) \
    انجامبده { \
        اگر (false) { stmts } \
    } وقتی (false)
#endif

/// Assert without error location, generally UASSERT_OBJ preferred
#define UASSERT(condition, stmsg) \
    انجامبده { \
        اگر (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } وقتی (false)
/// Assert with object to provide error location
#define UASSERT_OBJ(condition, obj, stmsg) \
    انجامبده { \
        اگر (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } وقتی (false)
// For use in V3Ast static functions only
#define UASSERT_STATIC(condition, stmsg) \
    انجامبده { \
        اگر (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } وقتی (false)
/// Check self test values for expected value.  Safe from side-effects.
#define UASSERT_SELFTEST(got, exp) \
    انجامبده { \
        ثابت auto g = (got); \
        ثابت decltype(g) e{exp}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } وقتی (false)

// Error that call not supported; only for some Ast functions
#define V3ERROR_NA \
    انجامبده { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } وقتی (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#define V3ERROR_NA_RETURN(مقدار) \
    V3ERROR_NA; \
    بازگشت مقدار

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DEBUG(...) \
    VL_ATTR_UNUSED ایستا صحیح debug##__VA_ARGS__() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (تهی)&debug##__VA_ARGS__; \
        ایستا صحیح s_level = -1; \
        اگر (VL_UNLIKELY(s_level < 0)) { \
            std::string tag{VL_STRINGIFY(__VA_ARGS__)}; \
            اگر (!tag.empty()) tag[0] = std::tolower(tag[0]); \
            ثابت unsigned debugTag = v3Global.opt.debugLevel(tag); \
            ثابت unsigned debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ثابت unsigned debugLevel = debugTag >= debugSrc ? debugTag : debugSrc; \
            اگر (!v3Global.opt.available()) بازگشت static_cast<صحیح>(debugLevel); \
            s_level = static_cast<صحیح>(debugLevel); \
        } \
        بازگشت s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#define VL_DEFINE_DUMP(func, tag) \
    VL_ATTR_UNUSED ایستا صحیح dump##func() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (تهی)&dump##func; \
        ایستا صحیح s_level = -1; \
        اگر (VL_UNLIKELY(s_level < 0)) { \
            ثابت unsigned dumpTag = v3Global.opt.dumpLevel(tag); \
            ثابت unsigned dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ثابت unsigned dumpLevel = dumpTag >= dumpSrc ? dumpTag : dumpSrc; \
            اگر (!v3Global.opt.available()) بازگشت static_cast<صحیح>(dumpLevel); \
            s_level = static_cast<صحیح>(dumpLevel); \
        } \
        بازگشت s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#define VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(GraphLevel, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(درخت_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(درخت_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_UNUSED ایستا صحیح dump_درخت_Either_Level() { \
        /* Don't complain this function is unused */ (تهی)&dump_درخت_Either_Level; \
        بازگشت dump_درخت_Json_Level() >= dump_درخت_Level() ? dump_درخت_Json_Level() : dump_درخت_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#endif  // Guard
