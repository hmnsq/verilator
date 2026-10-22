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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ERROR_H_
#تعریف_کرو VERILATOR_V3ERROR_H_

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

class V3_Error_رمز final {
عوامی:
    // clang-format off
    شمار en: uint8_t  {
        EC_MIN=0,       // Keep first
        //
        EC_INFO,        // General information out
        EC_FATAL,       // Kill the program
        EC_FATALMANY,   // Kill the program, due to too many errors, suppress with --quiet-exit
        EC_FATALSRC,    // Kill the program, for internal source errors
        EC_ERROR,       // General error out, can't suppress
        EC_پہلا_NAMED,  // Just a code so the program knows where to start info/errors
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
        EC_پہلا_WARN,  // Just a code so the program knows where to start warnings
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
        غیر_مستعمل_2,         // Unused genvar, parameter or signal message (Backward Compatibility)
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
        چوڑائی_2,          // Width mismatch
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
    شمار en m_e;
    V3_Error_رمز()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Error_رمز(en _e)
        : m_e{_e} {}
    explicit V3_Error_رمز(ثابت حرف* msgp);  // Matching code or ERROR
    explicit V3_Error_رمز(ثابت std::string& پیغام_2)
        : V3_Error_رمز{پیغام_2.c_str()} {}
    explicit V3_Error_رمز(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت VL_MT_محفوظ { واپس m_e; }
    ثابت حرف* ascii() ثابت VL_MT_محفوظ {
        ساکن ثابت حرف* ثابت نام[] = {
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
        واپس نام[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ثابت VL_MT_محفوظ {
        واپس (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ثابت VL_MT_محفوظ { واپس (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ثابت VL_MT_محفوظ {
        واپس (m_e != EC_INFO && m_e < V3_Error_رمز::EC_پہلا_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ثابت VL_MT_محفوظ {
        واپس m_e == V3_Error_رمز::EC_FATAL || m_e == V3_Error_رمز::EC_FATALMANY
               || m_e == V3_Error_رمز::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ثابت VL_MT_محفوظ {
        واپس m_e == V3_Error_رمز::USERINFO || m_e == V3_Error_رمز::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ثابت VL_MT_محفوظ {
        واپس (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ثابت VL_MT_محفوظ {
        واپس (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ثابت VL_MT_محفوظ {
        واپس (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == چوڑائی_2
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ثابت VL_MT_محفوظ {
        واپس (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ثابت { واپس m_e >= EC_پہلا_NAMED; }

نجی:
    V3_Error_رمز renamed_مقصد() ثابت {
        // Return a new error this error has been renamed to
        اگر (m_e == LITENDIAN) واپس V3_Error_رمز{ASCRANGE};
        واپس V3_Error_رمز{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ثابت { واپس renamed_مقصد() != V3_Error_رمز{EC_MIN}; }

عوامی:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    خالی forDelegateCodes(std::function<خالی(V3_Error_رمز رمز)> action) ثابت {
        // If add to this switch, also update isUnder()
        انتخاب (m_e) {
        حالت V3_Error_رمز::E_UNSUPPORTED:
            action(V3_Error_رمز{E_UNSUPPORTED});
            action(V3_Error_رمز{COVERIGN});
            action(V3_Error_رمز{SPECIFYIGN});
            واپس;
        حالت V3_Error_رمز::I_LINT:
            برائے (صحیح i = V3_Error_رمز::EC_MIN; i < V3_Error_رمز::_ENUM_MAX; ++i) {
                ثابت V3_Error_رمز subcode{i};
                اگر (subcode.lintError()) action(subcode);
            }
            واپس;
        حالت V3_Error_رمز::I_STYLE:
            برائے (صحیح i = V3_Error_رمز::EC_MIN; i < V3_Error_رمز::_ENUM_MAX; ++i) {
                ثابت V3_Error_رمز subcode{i};
                اگر (subcode.styleError()) action(subcode);
            }
            واپس;
        حالت V3_Error_رمز::غیر_مستعمل_2:
            action(V3_Error_رمز{UNUSEDGENVAR});
            action(V3_Error_رمز{UNUSEDLOOP});
            action(V3_Error_رمز{UNUSEDPARAM});
            action(V3_Error_رمز{UNUSEDSIGNAL});
            واپس;
        حالت V3_Error_رمز::چوڑائی_2:
            action(V3_Error_رمز{WIDTHDIGITS});
            action(V3_Error_رمز{WIDTHEXPAND});
            action(V3_Error_رمز{WIDTHTRUNC});
            action(V3_Error_رمز{WIDTHXZEXPAND});
            واپس;
        طےشدہ: action(*this);
        }
    }
    bool isUnder(V3_Error_رمز other) ثابت {
        // Reverse of forDelegateCodes, return true if this' code is under another
        اگر (VL_LIKELY(other == m_e)) واپس true;
        انتخاب (other) {
        حالت V3_Error_رمز::E_UNSUPPORTED:
            واپس m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        حالت V3_Error_رمز::I_LINT: واپس lintError();
        حالت V3_Error_رمز::I_STYLE: واپس styleError();
        حالت V3_Error_رمز::غیر_مستعمل_2:
            واپس m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        حالت V3_Error_رمز::چوڑائی_2:
            واپس m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        طےشدہ: واپس false;
        }
    }
    string url() ثابت;
};
constexpr bool عامل==(ثابت V3_Error_رمز& lhs, ثابت V3_Error_رمز& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت V3_Error_رمز& lhs, V3_Error_رمز::en rhs) { واپس lhs.m_e == rhs; }
constexpr bool عامل==(V3_Error_رمز::en lhs, ثابت V3_Error_رمز& rhs) { واپس lhs == rhs.m_e; }
اندرونی std::ostream& عامل<<(std::ostream& os, ثابت V3_Error_رمز& rhs) {
    واپس os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VError_ثنائی_ہندسہ_سیٹ final {
    std::bitset<V3_Error_رمز::_ENUM_MAX> m_bitset;  // Enable for each code
    VError_ثنائی_ہندسہ_سیٹ(ثابت VError_ثنائی_ہندسہ_سیٹ& lhs, ثابت VError_ثنائی_ہندسہ_سیٹ& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

عوامی:
    class AllOnes {};
    VError_ثنائی_ہندسہ_سیٹ() = طےشدہ;
    explicit VError_ثنائی_ہندسہ_سیٹ(AllOnes) { m_bitset.سیٹ(); }
    ~VError_ثنائی_ہندسہ_سیٹ() = طےشدہ;
    bool test(V3_Error_رمز رمز) ثابت { واپس m_bitset[رمز]; }
    خالی سیٹ(V3_Error_رمز رمز, bool flag) { m_bitset[رمز] = flag; }
    V3Hash hash() ثابت {
        ثابت سائز_t hash_رمز = std::hash<std::bitset<V3_Error_رمز::_ENUM_MAX>>()(m_bitset);
        واپس V3Hash{static_cast<uint64_t>(hash_رمز)};
    }
    VError_ثنائی_ہندسہ_سیٹ عامل&(ثابت VError_ثنائی_ہندسہ_سیٹ& rhs) ثابت { واپس VError_ثنائی_ہندسہ_سیٹ{*this, rhs}; }
    bool عامل==(ثابت VError_ثنائی_ہندسہ_سیٹ& rhs) ثابت { واپس m_bitset == rhs.m_bitset; }
    string ascii() ثابت {  // LCOV_EXCL_START
        string نتیجہ;
        برائے (صحیح i = V3_Error_رمز::EC_MIN; i < V3_Error_رمز::_ENUM_MAX; ++i) {
            اگر (!test(V3_Error_رمز{i})) نتیجہ += " !"s + V3_Error_رمز{i}.ascii();
        }
        واپس نتیجہ;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VError_پیغام final {
    // TYPES
    using File_سطریں = std::deque<ثابت File_سطر*>;
    // MEMBERS
    V3_Error_رمز m_رمز;  // Which warning
    string m_متن;  // Warning message text
    File_سطر* m_fileline;  // Primary warning fileline
    File_سطریں m_filelines;  // Additional referenced filelines
عوامی:
    // CONSTRUCTORS
    VError_پیغام() { clear(); }
    ~VError_پیغام() = طےشدہ;
    خالی clear() { init(V3_Error_رمز::EC_MIN); }
    خالی init(V3_Error_رمز رمز) {
        m_رمز = رمز;
        m_متن = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3_Error_رمز رمز() ثابت { واپس m_رمز; }
    bool isClear() ثابت { واپس m_رمز == V3_Error_رمز::EC_MIN; }
    string متن() ثابت { واپس m_متن; }
    خالی متن(ثابت string& پیغام_2) { m_متن = پیغام_2; }
    File_سطر* fileline() ثابت { واپس m_fileline; }
    خالی fileline(File_سطر* fl) { m_fileline = fl; }
    File_سطریں filelines() ثابت { واپس m_filelines; }
    // Returns identifier for use in warnRelated()
    صحیح ڈالو_Fileline(ثابت File_سطر* fl) {
        m_filelines.emplace_عقب(fl);
        واپس m_filelines.سائز() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    دوست class V3Error;

عوامی:
    using Messages_سیٹ = std::سیٹ<std::string>;
    using ErrorExitCb = خالی (*)(خالی);

نجی:
    ساکن constexpr غیرعلامتی MAX_ERRORS = 50;  // Fatal after this may errors

    // MEMBERS
    // Tell user to see manual, 0=not yet, 1=doit, 2=disable
    bool m_tellManual VL_GUARDED_BY(m_mutex) = false;
    bool m_tell_اندرونی VL_GUARDED_BY(m_mutex) = false;
    VError_پیغام m_پیغام VL_GUARDED_BY(m_mutex);  // Message being formed
    bool m_errorSuppressed VL_GUARDED_BY(m_mutex)
        = false;  // Error being formed should be suppressed
    Messages_سیٹ m_messages VL_GUARDED_BY(m_mutex);  // Errors outputted, to remove dups
    ErrorExitCb m_errorExitCb VL_GUARDED_BY(m_mutex)
        = nullptr;  // Callback when error occurs for dumping
    bool m_errorContexted VL_GUARDED_BY(m_mutex) = false;  // Error being formed got context
    صحیح m_warn_گنتی VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    صحیح m_err_گنتی VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VError_ثنائی_ہندسہ_سیٹ m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VError_ثنائی_ہندسہ_سیٹ m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VError_ثنائی_ہندسہ_سیٹ m_showedSuppressed VL_GUARDED_BY(m_mutex);
    صحیح m_debugDefault = 0;  // Option: --debugi Default debugging level
    صحیح m_error_حد_بندی VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    خالی v3errorPrep(V3_Error_رمز رمز) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { واپس m_errorStr; }
    خالی v3error_انتہا(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_REQUIRES(m_mutex);
    خالی v3error_انتہا_Guts(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_REQUIRES(m_mutex);

عوامی:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string پیغام_سابقہ() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warn_مزید_Spaces() VL_REQUIRES(m_mutex);
    خالی execErrorExitCb() VL_REQUIRES(m_mutex) {
        اگر (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { واپس m_errorExitCb; }
    خالی errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3_Error_رمز رمز, bool supp) VL_REQUIRES(m_mutex);
    خالی vlAbortOrExit() VL_REQUIRES(m_mutex);
    خالی errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    خالی incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_گنتی; }
    خالی incErrors() VL_REQUIRES(m_mutex) { ++m_err_گنتی; }
    صحیح error_گنتی() ثابت VL_REQUIRES(m_mutex) { واپس m_err_گنتی; }
    bool isErrorOrWarn() ثابت VL_REQUIRES(m_mutex) {
        واپس error_گنتی() || (warnFatal() && warn_گنتی());
    }
    bool pretendError(V3_Error_رمز رمز) VL_REQUIRES(m_mutex) { واپس m_pretendError.test(رمز); }
    خالی pretendError(V3_Error_رمز رمز, bool flag) VL_REQUIRES(m_mutex) {
        رمز.forDelegateCodes([this, flag](V3_Error_رمز subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.سیٹ(subcode, flag); });
    }
    صحیح debugDefault() ثابت VL_MT_محفوظ { واپس m_debugDefault; }
    خالی debugDefault(صحیح level) VL_MT_UNSAFE { m_debugDefault = level; }
    صحیح error_حد_بندی() ثابت VL_REQUIRES(m_mutex) { واپس m_error_حد_بندی; }
    خالی error_حد_بندی(صحیح level) VL_REQUIRES(m_mutex) { m_error_حد_بندی = level; }
    bool warnFatal() ثابت VL_REQUIRES(m_mutex) { واپس m_warnFatal; }
    خالی warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3_Error_رمز error_رمز() ثابت VL_REQUIRES(m_mutex) { واپس m_پیغام.رمز(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { واپس m_errorContexted; }
    صحیح warn_گنتی() ثابت VL_REQUIRES(m_mutex) { واپس m_warn_گنتی; }
    bool errorSuppressed() ثابت VL_REQUIRES(m_mutex) { واپس m_errorSuppressed; }
    خالی errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3_Error_رمز رمز) VL_REQUIRES(m_mutex) {
        واپس m_describedEachWarn.test(رمز);
    }
    خالی describedEachWarn(V3_Error_رمز رمز, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.سیٹ(رمز, flag);
    }
    خالی suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ثابت File_سطر* fl) VL_REQUIRES(m_mutex) {
        ثابت صحیح id = m_پیغام.ڈالو_Fileline(fl);
        واپس "__WARNRELATED("s + std::مقصد_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        واپس "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

عوامی:
    V3Error() = delete;
    ساکن V3ErrorGuarded& s() VL_MT_محفوظ {  // Singleton
        ساکن V3ErrorGuarded s_s;
        واپس s_s;
    }

    // ACCESSORS
    ساکن خالی debugDefault(صحیح level) VL_MT_UNSAFE { s().debugDefault(level); }
    ساکن صحیح debugDefault() VL_MT_محفوظ { واپس s().debugDefault(); }
    ساکن خالی error_حد_بندی(صحیح level) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().error_حد_بندی(level);
    }
    ساکن صحیح error_حد_بندی() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().error_حد_بندی();
    }
    ساکن خالی warnFatal(bool flag) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    ساکن bool warnFatal() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().warnFatal();
    }
    // returns %Error/%Warn
    ساکن string پیغام_سابقہ() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().پیغام_سابقہ();
    }
    ساکن صحیح error_گنتی() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().error_گنتی();
    }
    ساکن bool pretendError(V3_Error_رمز رمز) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().pretendError(رمز);
    }
    ساکن صحیح warn_گنتی() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().warn_گنتی();
    }
    ساکن bool errorContexted() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().errorContexted();
    }
    ساکن خالی errorContexted(bool flag) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    ساکن خالی describedEachWarn(V3_Error_رمز رمز, bool flag) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().describedEachWarn(رمز, flag);
    }
    // METHODS
    ساکن خالی incErrors() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().incErrors();
    }
    ساکن خالی incWarnings() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().incWarnings();
    }
    ساکن خالی init();
    ساکن bool isError(V3_Error_رمز رمز, bool supp) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().isError(رمز, supp);
    }
    ساکن bool isErrorOrWarn() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().isErrorOrWarn();
    }
    ساکن خالی abortIfErrors() {
        اگر (error_گنتی()) abortIfWarnings();
    }
    ساکن خالی abortIfWarnings();
    // Suppress next %Warn if user has it off
    ساکن خالی suppressThisWarning() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    ساکن خالی pretendError(V3_Error_رمز رمز, bool flag) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().pretendError(رمز, flag);
    }
    ساکن string سطر_Str(ثابت حرف* filename, صحیح lineno) VL_خالص;
    ساکن string strip_Meta_متن(ثابت string& متن, bool stripMeta) VL_خالص;
    ساکن V3_Error_رمز error_رمز() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        واپس s().error_رمز();
    }
    ساکن خالی errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_تالا_Guard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    ساکن constexpr ثابت حرف* WARN_مزید = "__WARNMORE__";
    ساکن string warn_مزید() VL_MT_محفوظ { واپس WARN_مزید; }
    // When printing an error/warning, mark beginning of context information (can nest)
    ساکن constexpr ثابت حرف* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    ساکن string warnContextBegin() VL_MT_محفوظ { واپس WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    ساکن constexpr ثابت حرف* WARN_CONTEXT_انتہا = "__WARNEND__";
    ساکن string warn_Context_انتہا() VL_MT_محفوظ { واپس WARN_CONTEXT_انتہا; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    ساکن string warnAdditionalInfo() VL_MT_محفوظ { واپس "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    ساکن string warnContextNone() {
        V3Error::errorContexted(true);
        واپس "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    ساکن std::ostringstream& v3errorPrep(V3_Error_رمز رمز) VL_ACQUIRE(s().m_mutex);
    ساکن std::ostringstream& v3error_Prep_File_سطر(V3_Error_رمز رمز, ثابت حرف* file, صحیح سطر)
        VL_ACQUIRE(s().m_mutex);
    ساکن std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { واپس s().v3errorStr(); }
    // static, but often overridden in classes.
    ساکن خالی v3error_انتہا(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_RELEASE(s().m_mutex);
    ساکن خالی vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
خالی v3error_انتہا(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_محفوظ;
خالی v3error_انتہا_Fatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_محفوظ VL_ATTR_NORETURN;

#اگر_تعریف_شدہ VL_MT_DISABLED_رمز_UNIT
#تعریف_کرو VL_MT_DISABLED_رمز_UNIT_DEFINED 1
#ورنہ
#تعریف_کرو VL_MT_DISABLED_رمز_UNIT_DEFINED 0
#اگر_ختم

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#تعریف_کرو v3error_Build_پیغام(prep, پیغام_2) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << پیغام_2))
#تعریف_کرو v3warn_رمز(رمز, پیغام_2) v3error_انتہا(v3error_Build_پیغام(V3Error::v3errorPrep(رمز), پیغام_2))
#تعریف_کرو v3warn_رمز_Fatal(رمز, پیغام_2) \
    v3error_انتہا_Fatal(v3error_Build_پیغام(V3Error::v3errorPrep(رمز), پیغام_2))
#تعریف_کرو v3warn(رمز, پیغام_2) v3warn_رمز(V3_Error_رمز::رمز, پیغام_2)
#تعریف_کرو v3info(پیغام_2) v3warn_رمز(V3_Error_رمز::EC_INFO, پیغام_2)
#تعریف_کرو v3error(پیغام_2) v3warn_رمز(V3_Error_رمز::EC_ERROR, پیغام_2)
#تعریف_کرو v3fatal(پیغام_2) v3warn_رمز_Fatal(V3_Error_رمز::EC_FATAL, پیغام_2)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#تعریف_کرو v3fatalMany(پیغام_2) v3warn_رمز_Fatal(V3_Error_رمز::EC_FATALMANY, پیغام_2)
// Fatal exit; used instead of fatal() to mention the source code line
#تعریف_کرو v3fatalSrc(پیغام_2) \
    v3error_انتہا_Fatal(v3error_Build_پیغام( \
        V3Error::v3error_Prep_File_سطر(V3_Error_رمز::EC_FATALSRC, __FILE__, __LINE__), پیغام_2))
// Use this when normal v3fatal is called in static method that overrides fileline.
#تعریف_کرو v3fatalStatic(پیغام_2) \
    ::v3error_انتہا_Fatal(v3error_Build_پیغام(V3Error::v3errorPrep(V3_Error_رمز::EC_FATAL), پیغام_2))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#تعریف_کرو UINFO(level, stmsg) \
    کرو { \
        اگر (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::سطر_Str(__FILE__, __LINE__) << stmsg; \
            اگر (ss.str()[ss.str().سائز() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } جب (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#تعریف_کرو UINFO_سابقہ(stmsg) \
    کرو { std::cout << "- " << V3Error::سطر_Str(__FILE__, __LINE__) << stmsg; } جب (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#تعریف_کرو UINFOTREE(level, nodep, uinfo_پیغام, dumptree_پیغام) \
    کرو { \
        اگر (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_پیغام; \
            اگر (!us.str().خالی_خانے()) UINFO(level, us.str()); \
            اگر (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_پیغام; \
                اگر (ss.str().خالی_خانے()) ss << uinfo_پیغام; \
                nodep->dump_درخت("- " + V3Error::سطر_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } جب (false)

/// Compile statements only when debug build
#اگر_تعریف_شدہ VL_DEBUG
#تعریف_کرو UDEBUGONLY(stmts) \
    کرو { stmts } جب (false)
#ورنہ
#تعریف_کرو UDEBUGONLY(stmts) \
    کرو { \
        اگر (false) { stmts } \
    } جب (false)
#اگر_ختم

/// Assert without error location, generally UASSERT_OBJ preferred
#تعریف_کرو UASSERT(condition, stmsg) \
    کرو { \
        اگر (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } جب (false)
/// Assert with object to provide error location
#تعریف_کرو UASSERT_OBJ(condition, obj, stmsg) \
    کرو { \
        اگر (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } جب (false)
// For use in V3Ast static functions only
#تعریف_کرو UASSERT_STATIC(condition, stmsg) \
    کرو { \
        اگر (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } جب (false)
/// Check self test values for expected value.  Safe from side-effects.
#تعریف_کرو UASSERT_SELFTEST(got, قوت_نما) \
    کرو { \
        ثابت خودکار g = (got); \
        ثابت decltype(g) e{قوت_نما}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } جب (false)

// Error that call not supported; only for some Ast functions
#تعریف_کرو V3ERROR_NA \
    کرو { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } جب (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#تعریف_کرو V3ERROR_NA_RETURN(قدر) \
    V3ERROR_NA; \
    واپس قدر

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#تعریف_کرو VL_DEFINE_DEBUG(...) \
    VL_ATTR_غیر_مستعمل ساکن صحیح debug##__VA_ARGS__() VL_MT_محفوظ { \
        /* Don't complain this function is unused */ \
        (خالی)&debug##__VA_ARGS__; \
        ساکن صحیح s_level = -1; \
        اگر (VL_UNLIKELY(s_level < 0)) { \
            std::string لیبل{VL_STRINGIFY(__VA_ARGS__)}; \
            اگر (!لیبل.خالی_خانے()) لیبل[0] = std::tolower(لیبل[0]); \
            ثابت غیرعلامتی debug_لیبل = v3Global.opt.debugLevel(لیبل); \
            ثابت غیرعلامتی debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ثابت غیرعلامتی debugLevel = debug_لیبل >= debugSrc ? debug_لیبل : debugSrc; \
            اگر (!v3Global.opt.دستیاب()) واپس static_cast<صحیح>(debugLevel); \
            s_level = static_cast<صحیح>(debugLevel); \
        } \
        واپس s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#تعریف_کرو VL_DEFINE_DUMP(func, لیبل) \
    VL_ATTR_غیر_مستعمل ساکن صحیح dump##func() VL_MT_محفوظ { \
        /* Don't complain this function is unused */ \
        (خالی)&dump##func; \
        ساکن صحیح s_level = -1; \
        اگر (VL_UNLIKELY(s_level < 0)) { \
            ثابت غیرعلامتی dump_لیبل = v3Global.opt.dumpLevel(لیبل); \
            ثابت غیرعلامتی dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ثابت غیرعلامتی dumpLevel = dump_لیبل >= dumpSrc ? dump_لیبل : dumpSrc; \
            اگر (!v3Global.opt.دستیاب()) واپس static_cast<صحیح>(dumpLevel); \
            s_level = static_cast<صحیح>(dumpLevel); \
        } \
        واپس s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#تعریف_کرو VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(گراف_Level, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(درخت_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(درخت_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_غیر_مستعمل ساکن صحیح dump_درخت_Either_Level() { \
        /* Don't complain this function is unused */ (خالی)&dump_درخت_Either_Level; \
        واپس dump_درخت_Json_Level() >= dump_درخت_Level() ? dump_درخت_Json_Level() : dump_درخت_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#اگر_ختم  // Guard
