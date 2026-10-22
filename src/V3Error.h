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

#جے_بنا_تعریف VERILATOR_V3ERROR_H_
#تعریف_کرو VERILATOR_V3ERROR_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Hash.h"
#ضمن "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#ضمن "V3String.h"

#ضمن <array>
#ضمن <bitset>
#ضمن <cassert>
#ضمن <cctype>
#ضمن <deque>
#ضمن <map>
#ضمن <set>
#ضمن <sstream>

class V3Error;
class File_سطر;

//######################################################################

class V3_Error_کوڈ final {
عام:
    // clang-format off
    تعداد en: uint8_t  {
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
        بن_ورتیا_2,         // Unused genvar, parameter or signal message (Backward Compatibility)
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
    تعداد en m_e;
    V3_Error_کوڈ()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Error_کوڈ(en _e)
        : m_e{_e} {}
    explicit V3_Error_کوڈ(ثابت حرف* msgp);  // Matching code or ERROR
    explicit V3_Error_کوڈ(ثابت std::string& پیغام)
        : V3_Error_کوڈ{پیغام.c_str()} {}
    explicit V3_Error_کوڈ(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت VL_MT_محفوظ { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_MT_محفوظ {
        ساكن ثابت حرف* ثابت ناں[] = {
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
        أرجع ناں[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ثابت VL_MT_محفوظ {
        أرجع (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ثابت VL_MT_محفوظ { أرجع (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ثابت VL_MT_محفوظ {
        أرجع (m_e != EC_INFO && m_e < V3_Error_کوڈ::EC_پہلا_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ثابت VL_MT_محفوظ {
        أرجع m_e == V3_Error_کوڈ::EC_FATAL || m_e == V3_Error_کوڈ::EC_FATALMANY
               || m_e == V3_Error_کوڈ::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ثابت VL_MT_محفوظ {
        أرجع m_e == V3_Error_کوڈ::USERINFO || m_e == V3_Error_کوڈ::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ثابت VL_MT_محفوظ {
        أرجع (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ثابت VL_MT_محفوظ {
        أرجع (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ثابت VL_MT_محفوظ {
        أرجع (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
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
        أرجع (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ثابت { أرجع m_e >= EC_پہلا_NAMED; }

خاص:
    V3_Error_کوڈ renamed_منزل() ثابت {
        // Return a new error this error has been renamed to
        إذا (m_e == LITENDIAN) أرجع V3_Error_کوڈ{ASCRANGE};
        أرجع V3_Error_کوڈ{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ثابت { أرجع renamed_منزل() != V3_Error_کوڈ{EC_MIN}; }

عام:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    فراغ forDelegateCodes(std::function<فراغ(V3_Error_کوڈ کوڈ)> action) ثابت {
        // If add to this switch, also update isUnder()
        بدل (m_e) {
        حالة V3_Error_کوڈ::E_UNSUPPORTED:
            action(V3_Error_کوڈ{E_UNSUPPORTED});
            action(V3_Error_کوڈ{COVERIGN});
            action(V3_Error_کوڈ{SPECIFYIGN});
            أرجع;
        حالة V3_Error_کوڈ::I_LINT:
            لكل (صحيح i = V3_Error_کوڈ::EC_MIN; i < V3_Error_کوڈ::_ENUM_MAX; ++i) {
                ثابت V3_Error_کوڈ subcode{i};
                إذا (subcode.lintError()) action(subcode);
            }
            أرجع;
        حالة V3_Error_کوڈ::I_STYLE:
            لكل (صحيح i = V3_Error_کوڈ::EC_MIN; i < V3_Error_کوڈ::_ENUM_MAX; ++i) {
                ثابت V3_Error_کوڈ subcode{i};
                إذا (subcode.styleError()) action(subcode);
            }
            أرجع;
        حالة V3_Error_کوڈ::بن_ورتیا_2:
            action(V3_Error_کوڈ{UNUSEDGENVAR});
            action(V3_Error_کوڈ{UNUSEDLOOP});
            action(V3_Error_کوڈ{UNUSEDPARAM});
            action(V3_Error_کوڈ{UNUSEDSIGNAL});
            أرجع;
        حالة V3_Error_کوڈ::چوڑائی_2:
            action(V3_Error_کوڈ{WIDTHDIGITS});
            action(V3_Error_کوڈ{WIDTHEXPAND});
            action(V3_Error_کوڈ{WIDTHTRUNC});
            action(V3_Error_کوڈ{WIDTHXZEXPAND});
            أرجع;
        افتراضي: action(*this);
        }
    }
    bool isUnder(V3_Error_کوڈ other) ثابت {
        // Reverse of forDelegateCodes, return true if this' code is under another
        إذا (VL_LIKELY(other == m_e)) أرجع true;
        بدل (other) {
        حالة V3_Error_کوڈ::E_UNSUPPORTED:
            أرجع m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        حالة V3_Error_کوڈ::I_LINT: أرجع lintError();
        حالة V3_Error_کوڈ::I_STYLE: أرجع styleError();
        حالة V3_Error_کوڈ::بن_ورتیا_2:
            أرجع m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        حالة V3_Error_کوڈ::چوڑائی_2:
            أرجع m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        افتراضي: أرجع false;
        }
    }
    string url() ثابت;
};
constexpr bool عامل==(ثابت V3_Error_کوڈ& lhs, ثابت V3_Error_کوڈ& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت V3_Error_کوڈ& lhs, V3_Error_کوڈ::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool عامل==(V3_Error_کوڈ::en lhs, ثابت V3_Error_کوڈ& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& عامل<<(std::ostream& os, ثابت V3_Error_کوڈ& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VError_ثنائی_ہندسہ_سیٹ final {
    std::bitset<V3_Error_کوڈ::_ENUM_MAX> m_bitset;  // Enable for each code
    VError_ثنائی_ہندسہ_سیٹ(ثابت VError_ثنائی_ہندسہ_سیٹ& lhs, ثابت VError_ثنائی_ہندسہ_سیٹ& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

عام:
    class AllOnes {};
    VError_ثنائی_ہندسہ_سیٹ() = افتراضي;
    explicit VError_ثنائی_ہندسہ_سیٹ(AllOnes) { m_bitset.سیٹ(); }
    ~VError_ثنائی_ہندسہ_سیٹ() = افتراضي;
    bool test(V3_Error_کوڈ کوڈ) ثابت { أرجع m_bitset[کوڈ]; }
    فراغ سیٹ(V3_Error_کوڈ کوڈ, bool flag) { m_bitset[کوڈ] = flag; }
    V3Hash hash() ثابت {
        ثابت ناپ_t hash_کوڈ = std::hash<std::bitset<V3_Error_کوڈ::_ENUM_MAX>>()(m_bitset);
        أرجع V3Hash{static_cast<uint64_t>(hash_کوڈ)};
    }
    VError_ثنائی_ہندسہ_سیٹ عامل&(ثابت VError_ثنائی_ہندسہ_سیٹ& rhs) ثابت { أرجع VError_ثنائی_ہندسہ_سیٹ{*this, rhs}; }
    bool عامل==(ثابت VError_ثنائی_ہندسہ_سیٹ& rhs) ثابت { أرجع m_bitset == rhs.m_bitset; }
    string ascii() ثابت {  // LCOV_EXCL_START
        string نتیجہ;
        لكل (صحيح i = V3_Error_کوڈ::EC_MIN; i < V3_Error_کوڈ::_ENUM_MAX; ++i) {
            إذا (!test(V3_Error_کوڈ{i})) نتیجہ += " !"s + V3_Error_کوڈ{i}.ascii();
        }
        أرجع نتیجہ;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VError_سنیہا final {
    // TYPES
    using File_سطریں = std::deque<ثابت File_سطر*>;
    // MEMBERS
    V3_Error_کوڈ m_کوڈ;  // Which warning
    string m_متن;  // Warning message text
    File_سطر* m_fileline;  // Primary warning fileline
    File_سطریں m_filelines;  // Additional referenced filelines
عام:
    // CONSTRUCTORS
    VError_سنیہا() { clear(); }
    ~VError_سنیہا() = افتراضي;
    فراغ clear() { init(V3_Error_کوڈ::EC_MIN); }
    فراغ init(V3_Error_کوڈ کوڈ) {
        m_کوڈ = کوڈ;
        m_متن = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3_Error_کوڈ کوڈ() ثابت { أرجع m_کوڈ; }
    bool isClear() ثابت { أرجع m_کوڈ == V3_Error_کوڈ::EC_MIN; }
    string متن() ثابت { أرجع m_متن; }
    فراغ متن(ثابت string& پیغام) { m_متن = پیغام; }
    File_سطر* fileline() ثابت { أرجع m_fileline; }
    فراغ fileline(File_سطر* fl) { m_fileline = fl; }
    File_سطریں filelines() ثابت { أرجع m_filelines; }
    // Returns identifier for use in warnRelated()
    صحيح ڈالو_Fileline(ثابت File_سطر* fl) {
        m_filelines.emplace_پٹھ(fl);
        أرجع m_filelines.ناپ() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    یار class V3Error;

عام:
    using Messages_سیٹ = std::سیٹ<std::string>;
    using ErrorExitCb = فراغ (*)(فراغ);

خاص:
    ساكن constexpr بنانشانی MAX_ERRORS = 50;  // Fatal after this may errors

    // MEMBERS
    // Tell user to see manual, 0=not yet, 1=doit, 2=disable
    bool m_tellManual VL_GUARDED_BY(m_mutex) = false;
    bool m_tell_اندرونی VL_GUARDED_BY(m_mutex) = false;
    VError_سنیہا m_سنیہا VL_GUARDED_BY(m_mutex);  // Message being formed
    bool m_errorSuppressed VL_GUARDED_BY(m_mutex)
        = false;  // Error being formed should be suppressed
    Messages_سیٹ m_messages VL_GUARDED_BY(m_mutex);  // Errors outputted, to remove dups
    ErrorExitCb m_errorExitCb VL_GUARDED_BY(m_mutex)
        = nullptr;  // Callback when error occurs for dumping
    bool m_errorContexted VL_GUARDED_BY(m_mutex) = false;  // Error being formed got context
    صحيح m_warn_گنتی VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    صحيح m_err_گنتی VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VError_ثنائی_ہندسہ_سیٹ m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VError_ثنائی_ہندسہ_سیٹ m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VError_ثنائی_ہندسہ_سیٹ m_showedSuppressed VL_GUARDED_BY(m_mutex);
    صحيح m_debugDefault = 0;  // Option: --debugi Default debugging level
    صحيح m_error_حد_بندی VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    فراغ v3errorPrep(V3_Error_کوڈ کوڈ) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { أرجع m_errorStr; }
    فراغ v3error_سرا(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_REQUIRES(m_mutex);
    فراغ v3error_سرا_Guts(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_REQUIRES(m_mutex);

عام:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string پیغام_اگیتر() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warn_ہور_Spaces() VL_REQUIRES(m_mutex);
    فراغ execErrorExitCb() VL_REQUIRES(m_mutex) {
        إذا (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { أرجع m_errorExitCb; }
    فراغ errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3_Error_کوڈ کوڈ, bool supp) VL_REQUIRES(m_mutex);
    فراغ vlAbortOrExit() VL_REQUIRES(m_mutex);
    فراغ errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    فراغ incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_گنتی; }
    فراغ incErrors() VL_REQUIRES(m_mutex) { ++m_err_گنتی; }
    صحيح error_گنتی() ثابت VL_REQUIRES(m_mutex) { أرجع m_err_گنتی; }
    bool isErrorOrWarn() ثابت VL_REQUIRES(m_mutex) {
        أرجع error_گنتی() || (warnFatal() && warn_گنتی());
    }
    bool pretendError(V3_Error_کوڈ کوڈ) VL_REQUIRES(m_mutex) { أرجع m_pretendError.test(کوڈ); }
    فراغ pretendError(V3_Error_کوڈ کوڈ, bool flag) VL_REQUIRES(m_mutex) {
        کوڈ.forDelegateCodes([this, flag](V3_Error_کوڈ subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.سیٹ(subcode, flag); });
    }
    صحيح debugDefault() ثابت VL_MT_محفوظ { أرجع m_debugDefault; }
    فراغ debugDefault(صحيح level) VL_MT_UNSAFE { m_debugDefault = level; }
    صحيح error_حد_بندی() ثابت VL_REQUIRES(m_mutex) { أرجع m_error_حد_بندی; }
    فراغ error_حد_بندی(صحيح level) VL_REQUIRES(m_mutex) { m_error_حد_بندی = level; }
    bool warnFatal() ثابت VL_REQUIRES(m_mutex) { أرجع m_warnFatal; }
    فراغ warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3_Error_کوڈ error_کوڈ() ثابت VL_REQUIRES(m_mutex) { أرجع m_سنیہا.کوڈ(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { أرجع m_errorContexted; }
    صحيح warn_گنتی() ثابت VL_REQUIRES(m_mutex) { أرجع m_warn_گنتی; }
    bool errorSuppressed() ثابت VL_REQUIRES(m_mutex) { أرجع m_errorSuppressed; }
    فراغ errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3_Error_کوڈ کوڈ) VL_REQUIRES(m_mutex) {
        أرجع m_describedEachWarn.test(کوڈ);
    }
    فراغ describedEachWarn(V3_Error_کوڈ کوڈ, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.سیٹ(کوڈ, flag);
    }
    فراغ suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ثابت File_سطر* fl) VL_REQUIRES(m_mutex) {
        ثابت صحيح id = m_سنیہا.ڈالو_Fileline(fl);
        أرجع "__WARNRELATED("s + std::منزل_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        أرجع "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

عام:
    V3Error() = delete;
    ساكن V3ErrorGuarded& s() VL_MT_محفوظ {  // Singleton
        ساكن V3ErrorGuarded s_s;
        أرجع s_s;
    }

    // ACCESSORS
    ساكن فراغ debugDefault(صحيح level) VL_MT_UNSAFE { s().debugDefault(level); }
    ساكن صحيح debugDefault() VL_MT_محفوظ { أرجع s().debugDefault(); }
    ساكن فراغ error_حد_بندی(صحيح level) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().error_حد_بندی(level);
    }
    ساكن صحيح error_حد_بندی() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().error_حد_بندی();
    }
    ساكن فراغ warnFatal(bool flag) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    ساكن bool warnFatal() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().warnFatal();
    }
    // returns %Error/%Warn
    ساكن string پیغام_اگیتر() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().پیغام_اگیتر();
    }
    ساكن صحيح error_گنتی() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().error_گنتی();
    }
    ساكن bool pretendError(V3_Error_کوڈ کوڈ) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().pretendError(کوڈ);
    }
    ساكن صحيح warn_گنتی() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().warn_گنتی();
    }
    ساكن bool errorContexted() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().errorContexted();
    }
    ساكن فراغ errorContexted(bool flag) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    ساكن فراغ describedEachWarn(V3_Error_کوڈ کوڈ, bool flag) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().describedEachWarn(کوڈ, flag);
    }
    // METHODS
    ساكن فراغ incErrors() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().incErrors();
    }
    ساكن فراغ incWarnings() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().incWarnings();
    }
    ساكن فراغ init();
    ساكن bool isError(V3_Error_کوڈ کوڈ, bool supp) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().isError(کوڈ, supp);
    }
    ساكن bool isErrorOrWarn() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().isErrorOrWarn();
    }
    ساكن فراغ abortIfErrors() {
        إذا (error_گنتی()) abortIfWarnings();
    }
    ساكن فراغ abortIfWarnings();
    // Suppress next %Warn if user has it off
    ساكن فراغ suppressThisWarning() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    ساكن فراغ pretendError(V3_Error_کوڈ کوڈ, bool flag) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().pretendError(کوڈ, flag);
    }
    ساكن string سطر_Str(ثابت حرف* filename, صحيح lineno) VL_خالص;
    ساكن string strip_Meta_متن(ثابت string& متن, bool stripMeta) VL_خالص;
    ساكن V3_Error_کوڈ error_کوڈ() VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        أرجع s().error_کوڈ();
    }
    ساكن فراغ errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_محفوظ_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_جندرا_Guard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    ساكن constexpr ثابت حرف* WARN_ہور = "__WARNMORE__";
    ساكن string warn_ہور() VL_MT_محفوظ { أرجع WARN_ہور; }
    // When printing an error/warning, mark beginning of context information (can nest)
    ساكن constexpr ثابت حرف* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    ساكن string warnContextBegin() VL_MT_محفوظ { أرجع WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    ساكن constexpr ثابت حرف* WARN_CONTEXT_سرا = "__WARNEND__";
    ساكن string warn_Context_سرا() VL_MT_محفوظ { أرجع WARN_CONTEXT_سرا; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    ساكن string warnAdditionalInfo() VL_MT_محفوظ { أرجع "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    ساكن string warnContextNone() {
        V3Error::errorContexted(true);
        أرجع "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    ساكن std::ostringstream& v3errorPrep(V3_Error_کوڈ کوڈ) VL_ACQUIRE(s().m_mutex);
    ساكن std::ostringstream& v3error_Prep_File_سطر(V3_Error_کوڈ کوڈ, ثابت حرف* file, صحيح سطر)
        VL_ACQUIRE(s().m_mutex);
    ساكن std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { أرجع s().v3errorStr(); }
    // static, but often overridden in classes.
    ساكن فراغ v3error_سرا(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
        VL_RELEASE(s().m_mutex);
    ساكن فراغ vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
فراغ v3error_سرا(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_محفوظ;
فراغ v3error_سرا_Fatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_محفوظ VL_ATTR_NORETURN;

#جے_تعریف_شدہ VL_MT_DISABLED_کوڈ_UNIT
#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT_DEFINED 1
#وإلا
#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT_DEFINED 0
#جے_انت

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#تعریف_کرو v3error_Build_سنیہا(prep, پیغام) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << پیغام))
#تعریف_کرو v3warn_کوڈ(کوڈ, پیغام) v3error_سرا(v3error_Build_سنیہا(V3Error::v3errorPrep(کوڈ), پیغام))
#تعریف_کرو v3warn_کوڈ_Fatal(کوڈ, پیغام) \
    v3error_سرا_Fatal(v3error_Build_سنیہا(V3Error::v3errorPrep(کوڈ), پیغام))
#تعریف_کرو v3warn(کوڈ, پیغام) v3warn_کوڈ(V3_Error_کوڈ::کوڈ, پیغام)
#تعریف_کرو v3info(پیغام) v3warn_کوڈ(V3_Error_کوڈ::EC_INFO, پیغام)
#تعریف_کرو v3error(پیغام) v3warn_کوڈ(V3_Error_کوڈ::EC_ERROR, پیغام)
#تعریف_کرو v3fatal(پیغام) v3warn_کوڈ_Fatal(V3_Error_کوڈ::EC_FATAL, پیغام)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#تعریف_کرو v3fatalMany(پیغام) v3warn_کوڈ_Fatal(V3_Error_کوڈ::EC_FATALMANY, پیغام)
// Fatal exit; used instead of fatal() to mention the source code line
#تعریف_کرو v3fatalSrc(پیغام) \
    v3error_سرا_Fatal(v3error_Build_سنیہا( \
        V3Error::v3error_Prep_File_سطر(V3_Error_کوڈ::EC_FATALSRC, __FILE__, __LINE__), پیغام))
// Use this when normal v3fatal is called in static method that overrides fileline.
#تعریف_کرو v3fatalStatic(پیغام) \
    ::v3error_سرا_Fatal(v3error_Build_سنیہا(V3Error::v3errorPrep(V3_Error_کوڈ::EC_FATAL), پیغام))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#تعریف_کرو UINFO(level, stmsg) \
    افعل { \
        إذا (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::سطر_Str(__FILE__, __LINE__) << stmsg; \
            إذا (ss.str()[ss.str().ناپ() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } طالما (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#تعریف_کرو UINFO_اگیتر(stmsg) \
    افعل { std::cout << "- " << V3Error::سطر_Str(__FILE__, __LINE__) << stmsg; } طالما (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#تعریف_کرو UINFOTREE(level, nodep, uinfo_پیغام, dumptree_پیغام) \
    افعل { \
        إذا (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_پیغام; \
            إذا (!us.str().خالی_تھاواں()) UINFO(level, us.str()); \
            إذا (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_پیغام; \
                إذا (ss.str().خالی_تھاواں()) ss << uinfo_پیغام; \
                nodep->dump_درخت("- " + V3Error::سطر_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } طالما (false)

/// Compile statements only when debug build
#جے_تعریف_شدہ VL_DEBUG
#تعریف_کرو UDEBUGONLY(stmts) \
    افعل { stmts } طالما (false)
#وإلا
#تعریف_کرو UDEBUGONLY(stmts) \
    افعل { \
        إذا (false) { stmts } \
    } طالما (false)
#جے_انت

/// Assert without error location, generally UASSERT_OBJ preferred
#تعریف_کرو UASSERT(condition, stmsg) \
    افعل { \
        إذا (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } طالما (false)
/// Assert with object to provide error location
#تعریف_کرو UASSERT_OBJ(condition, obj, stmsg) \
    افعل { \
        إذا (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } طالما (false)
// For use in V3Ast static functions only
#تعریف_کرو UASSERT_STATIC(condition, stmsg) \
    افعل { \
        إذا (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } طالما (false)
/// Check self test values for expected value.  Safe from side-effects.
#تعریف_کرو UASSERT_SELFTEST(got, قوت_نما) \
    افعل { \
        ثابت خودکار g = (got); \
        ثابت decltype(g) e{قوت_نما}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } طالما (false)

// Error that call not supported; only for some Ast functions
#تعریف_کرو V3ERROR_NA \
    افعل { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } طالما (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#تعریف_کرو V3ERROR_NA_RETURN(قدر) \
    V3ERROR_NA; \
    أرجع قدر

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#تعریف_کرو VL_DEFINE_DEBUG(...) \
    VL_ATTR_بن_ورتیا ساكن صحيح debug##__VA_ARGS__() VL_MT_محفوظ { \
        /* Don't complain this function is unused */ \
        (فراغ)&debug##__VA_ARGS__; \
        ساكن صحيح s_level = -1; \
        إذا (VL_UNLIKELY(s_level < 0)) { \
            std::string ٹیگ{VL_STRINGIFY(__VA_ARGS__)}; \
            إذا (!ٹیگ.خالی_تھاواں()) ٹیگ[0] = std::tolower(ٹیگ[0]); \
            ثابت بنانشانی debug_ٹیگ = v3Global.opt.debugLevel(ٹیگ); \
            ثابت بنانشانی debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ثابت بنانشانی debugLevel = debug_ٹیگ >= debugSrc ? debug_ٹیگ : debugSrc; \
            إذا (!v3Global.opt.دستیاب()) أرجع static_cast<صحيح>(debugLevel); \
            s_level = static_cast<صحيح>(debugLevel); \
        } \
        أرجع s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#تعریف_کرو VL_DEFINE_DUMP(func, ٹیگ) \
    VL_ATTR_بن_ورتیا ساكن صحيح dump##func() VL_MT_محفوظ { \
        /* Don't complain this function is unused */ \
        (فراغ)&dump##func; \
        ساكن صحيح s_level = -1; \
        إذا (VL_UNLIKELY(s_level < 0)) { \
            ثابت بنانشانی dump_ٹیگ = v3Global.opt.dumpLevel(ٹیگ); \
            ثابت بنانشانی dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ثابت بنانشانی dumpLevel = dump_ٹیگ >= dumpSrc ? dump_ٹیگ : dumpSrc; \
            إذا (!v3Global.opt.دستیاب()) أرجع static_cast<صحيح>(dumpLevel); \
            s_level = static_cast<صحيح>(dumpLevel); \
        } \
        أرجع s_level; \
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
    VL_ATTR_بن_ورتیا ساكن صحيح dump_درخت_Either_Level() { \
        /* Don't complain this function is unused */ (فراغ)&dump_درخت_Either_Level; \
        أرجع dump_درخت_Json_Level() >= dump_درخت_Level() ? dump_درخت_Json_Level() : dump_درخت_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#جے_انت  // Guard
