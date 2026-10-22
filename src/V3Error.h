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

#که_نه_تعریف_شوی VERILATOR_V3ERROR_H_
#تعریف_کړه VERILATOR_V3ERROR_H_

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
class File_کرښه;

//######################################################################

class V3_Error_کوډ final {
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
        EC_لومړی_NAMED,  // Just a code so the program knows where to start info/errors
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
        EC_لومړی_WARN,  // Just a code so the program knows where to start warnings
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
        ناکارول_شوی_2,         // Unused genvar, parameter or signal message (Backward Compatibility)
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
        پلنوالی_2,          // Width mismatch
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
    V3_Error_کوډ()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Error_کوډ(en _e)
        : m_e{_e} {}
    explicit V3_Error_کوډ(ثابت حرف* msgp);  // Matching code or ERROR
    explicit V3_Error_کوډ(ثابت std::string& پیغام_2)
        : V3_Error_کوډ{پیغام_2.c_str()} {}
    explicit V3_Error_کوډ(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr چلوونکی en() ثابت VL_MT_خوندي { أرجع m_e; }
    ثابت حرف* ascii() ثابت VL_MT_خوندي {
        ساكن ثابت حرف* ثابت نومونه[] = {
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
        أرجع نومونه[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ثابت VL_MT_خوندي {
        أرجع (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ثابت VL_MT_خوندي { أرجع (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ثابت VL_MT_خوندي {
        أرجع (m_e != EC_INFO && m_e < V3_Error_کوډ::EC_لومړی_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ثابت VL_MT_خوندي {
        أرجع m_e == V3_Error_کوډ::EC_FATAL || m_e == V3_Error_کوډ::EC_FATALMANY
               || m_e == V3_Error_کوډ::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ثابت VL_MT_خوندي {
        أرجع m_e == V3_Error_کوډ::USERINFO || m_e == V3_Error_کوډ::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ثابت VL_MT_خوندي {
        أرجع (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ثابت VL_MT_خوندي {
        أرجع (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ثابت VL_MT_خوندي {
        أرجع (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == پلنوالی_2
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ثابت VL_MT_خوندي {
        أرجع (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ثابت { أرجع m_e >= EC_لومړی_NAMED; }

خاص:
    V3_Error_کوډ renamed_منزل() ثابت {
        // Return a new error this error has been renamed to
        إذا (m_e == LITENDIAN) أرجع V3_Error_کوډ{ASCRANGE};
        أرجع V3_Error_کوډ{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ثابت { أرجع renamed_منزل() != V3_Error_کوډ{EC_MIN}; }

عام:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    فراغ forDelegateCodes(std::function<فراغ(V3_Error_کوډ کوډ)> action) ثابت {
        // If add to this switch, also update isUnder()
        بدل (m_e) {
        حالة V3_Error_کوډ::E_UNSUPPORTED:
            action(V3_Error_کوډ{E_UNSUPPORTED});
            action(V3_Error_کوډ{COVERIGN});
            action(V3_Error_کوډ{SPECIFYIGN});
            أرجع;
        حالة V3_Error_کوډ::I_LINT:
            لكل (صحيح i = V3_Error_کوډ::EC_MIN; i < V3_Error_کوډ::_ENUM_MAX; ++i) {
                ثابت V3_Error_کوډ subcode{i};
                إذا (subcode.lintError()) action(subcode);
            }
            أرجع;
        حالة V3_Error_کوډ::I_STYLE:
            لكل (صحيح i = V3_Error_کوډ::EC_MIN; i < V3_Error_کوډ::_ENUM_MAX; ++i) {
                ثابت V3_Error_کوډ subcode{i};
                إذا (subcode.styleError()) action(subcode);
            }
            أرجع;
        حالة V3_Error_کوډ::ناکارول_شوی_2:
            action(V3_Error_کوډ{UNUSEDGENVAR});
            action(V3_Error_کوډ{UNUSEDLOOP});
            action(V3_Error_کوډ{UNUSEDPARAM});
            action(V3_Error_کوډ{UNUSEDSIGNAL});
            أرجع;
        حالة V3_Error_کوډ::پلنوالی_2:
            action(V3_Error_کوډ{WIDTHDIGITS});
            action(V3_Error_کوډ{WIDTHEXPAND});
            action(V3_Error_کوډ{WIDTHTRUNC});
            action(V3_Error_کوډ{WIDTHXZEXPAND});
            أرجع;
        افتراضي: action(*this);
        }
    }
    bool isUnder(V3_Error_کوډ other) ثابت {
        // Reverse of forDelegateCodes, return true if this' code is under another
        إذا (VL_LIKELY(other == m_e)) أرجع true;
        بدل (other) {
        حالة V3_Error_کوډ::E_UNSUPPORTED:
            أرجع m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        حالة V3_Error_کوډ::I_LINT: أرجع lintError();
        حالة V3_Error_کوډ::I_STYLE: أرجع styleError();
        حالة V3_Error_کوډ::ناکارول_شوی_2:
            أرجع m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        حالة V3_Error_کوډ::پلنوالی_2:
            أرجع m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        افتراضي: أرجع false;
        }
    }
    string url() ثابت;
};
constexpr bool چلوونکی==(ثابت V3_Error_کوډ& lhs, ثابت V3_Error_کوډ& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool چلوونکی==(ثابت V3_Error_کوډ& lhs, V3_Error_کوډ::en rhs) { أرجع lhs.m_e == rhs; }
constexpr bool چلوونکی==(V3_Error_کوډ::en lhs, ثابت V3_Error_کوډ& rhs) { أرجع lhs == rhs.m_e; }
مضمن std::ostream& چلوونکی<<(std::ostream& os, ثابت V3_Error_کوډ& rhs) {
    أرجع os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VError_بټ_سېټ final {
    std::bitset<V3_Error_کوډ::_ENUM_MAX> m_bitset;  // Enable for each code
    VError_بټ_سېټ(ثابت VError_بټ_سېټ& lhs, ثابت VError_بټ_سېټ& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

عام:
    class AllOnes {};
    VError_بټ_سېټ() = افتراضي;
    explicit VError_بټ_سېټ(AllOnes) { m_bitset.سېټ(); }
    ~VError_بټ_سېټ() = افتراضي;
    bool test(V3_Error_کوډ کوډ) ثابت { أرجع m_bitset[کوډ]; }
    فراغ سېټ(V3_Error_کوډ کوډ, bool flag) { m_bitset[کوډ] = flag; }
    V3Hash hash() ثابت {
        ثابت اندازه_t hash_کوډ = std::hash<std::bitset<V3_Error_کوډ::_ENUM_MAX>>()(m_bitset);
        أرجع V3Hash{static_cast<uint64_t>(hash_کوډ)};
    }
    VError_بټ_سېټ چلوونکی&(ثابت VError_بټ_سېټ& rhs) ثابت { أرجع VError_بټ_سېټ{*this, rhs}; }
    bool چلوونکی==(ثابت VError_بټ_سېټ& rhs) ثابت { أرجع m_bitset == rhs.m_bitset; }
    string ascii() ثابت {  // LCOV_EXCL_START
        string پایله;
        لكل (صحيح i = V3_Error_کوډ::EC_MIN; i < V3_Error_کوډ::_ENUM_MAX; ++i) {
            إذا (!test(V3_Error_کوډ{i})) پایله += " !"s + V3_Error_کوډ{i}.ascii();
        }
        أرجع پایله;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VError_پیغام final {
    // TYPES
    using File_کرښې = std::deque<ثابت File_کرښه*>;
    // MEMBERS
    V3_Error_کوډ m_کوډ;  // Which warning
    string m_متن;  // Warning message text
    File_کرښه* m_fileline;  // Primary warning fileline
    File_کرښې m_filelines;  // Additional referenced filelines
عام:
    // CONSTRUCTORS
    VError_پیغام() { clear(); }
    ~VError_پیغام() = افتراضي;
    فراغ clear() { init(V3_Error_کوډ::EC_MIN); }
    فراغ init(V3_Error_کوډ کوډ) {
        m_کوډ = کوډ;
        m_متن = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3_Error_کوډ کوډ() ثابت { أرجع m_کوډ; }
    bool isClear() ثابت { أرجع m_کوډ == V3_Error_کوډ::EC_MIN; }
    string متن() ثابت { أرجع m_متن; }
    فراغ متن(ثابت string& پیغام_2) { m_متن = پیغام_2; }
    File_کرښه* fileline() ثابت { أرجع m_fileline; }
    فراغ fileline(File_کرښه* fl) { m_fileline = fl; }
    File_کرښې filelines() ثابت { أرجع m_filelines; }
    // Returns identifier for use in warnRelated()
    صحيح ټېل_Fileline(ثابت File_کرښه* fl) {
        m_filelines.emplace_شا(fl);
        أرجع m_filelines.اندازه() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    ملګری class V3Error;

عام:
    using Messages_سېټ = std::سېټ<std::string>;
    using ErrorExitCb = فراغ (*)(فراغ);

خاص:
    ساكن constexpr بینښه MAX_ERRORS = 50;  // Fatal after this may errors

    // MEMBERS
    // Tell user to see manual, 0=not yet, 1=doit, 2=disable
    bool m_tellManual VL_GUARDED_BY(m_mutex) = false;
    bool m_tell_دننی VL_GUARDED_BY(m_mutex) = false;
    VError_پیغام m_پیغام VL_GUARDED_BY(m_mutex);  // Message being formed
    bool m_errorSuppressed VL_GUARDED_BY(m_mutex)
        = false;  // Error being formed should be suppressed
    Messages_سېټ m_messages VL_GUARDED_BY(m_mutex);  // Errors outputted, to remove dups
    ErrorExitCb m_errorExitCb VL_GUARDED_BY(m_mutex)
        = nullptr;  // Callback when error occurs for dumping
    bool m_errorContexted VL_GUARDED_BY(m_mutex) = false;  // Error being formed got context
    صحيح m_warn_شمېر VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    صحيح m_err_شمېر VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VError_بټ_سېټ m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VError_بټ_سېټ m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VError_بټ_سېټ m_showedSuppressed VL_GUARDED_BY(m_mutex);
    صحيح m_debugDefault = 0;  // Option: --debugi Default debugging level
    صحيح m_error_حد VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    فراغ v3errorPrep(V3_Error_کوډ کوډ) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { أرجع m_errorStr; }
    فراغ v3error_پای_ټکی(ثابت std::ostringstream& sstr, ثابت string& extra, File_کرښه* fileline)
        VL_REQUIRES(m_mutex);
    فراغ v3error_پای_ټکی_Guts(ثابت std::ostringstream& sstr, ثابت string& extra, File_کرښه* fileline)
        VL_REQUIRES(m_mutex);

عام:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string پیغام_مختاړی() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warn_نور_Spaces() VL_REQUIRES(m_mutex);
    فراغ execErrorExitCb() VL_REQUIRES(m_mutex) {
        إذا (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { أرجع m_errorExitCb; }
    فراغ errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3_Error_کوډ کوډ, bool supp) VL_REQUIRES(m_mutex);
    فراغ vlAbortOrExit() VL_REQUIRES(m_mutex);
    فراغ errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    فراغ incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_شمېر; }
    فراغ incErrors() VL_REQUIRES(m_mutex) { ++m_err_شمېر; }
    صحيح error_شمېر() ثابت VL_REQUIRES(m_mutex) { أرجع m_err_شمېر; }
    bool isErrorOrWarn() ثابت VL_REQUIRES(m_mutex) {
        أرجع error_شمېر() || (warnFatal() && warn_شمېر());
    }
    bool pretendError(V3_Error_کوډ کوډ) VL_REQUIRES(m_mutex) { أرجع m_pretendError.test(کوډ); }
    فراغ pretendError(V3_Error_کوډ کوډ, bool flag) VL_REQUIRES(m_mutex) {
        کوډ.forDelegateCodes([this, flag](V3_Error_کوډ subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.سېټ(subcode, flag); });
    }
    صحيح debugDefault() ثابت VL_MT_خوندي { أرجع m_debugDefault; }
    فراغ debugDefault(صحيح level) VL_MT_UNSAFE { m_debugDefault = level; }
    صحيح error_حد() ثابت VL_REQUIRES(m_mutex) { أرجع m_error_حد; }
    فراغ error_حد(صحيح level) VL_REQUIRES(m_mutex) { m_error_حد = level; }
    bool warnFatal() ثابت VL_REQUIRES(m_mutex) { أرجع m_warnFatal; }
    فراغ warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3_Error_کوډ error_کوډ() ثابت VL_REQUIRES(m_mutex) { أرجع m_پیغام.کوډ(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { أرجع m_errorContexted; }
    صحيح warn_شمېر() ثابت VL_REQUIRES(m_mutex) { أرجع m_warn_شمېر; }
    bool errorSuppressed() ثابت VL_REQUIRES(m_mutex) { أرجع m_errorSuppressed; }
    فراغ errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3_Error_کوډ کوډ) VL_REQUIRES(m_mutex) {
        أرجع m_describedEachWarn.test(کوډ);
    }
    فراغ describedEachWarn(V3_Error_کوډ کوډ, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.سېټ(کوډ, flag);
    }
    فراغ suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ثابت File_کرښه* fl) VL_REQUIRES(m_mutex) {
        ثابت صحيح id = m_پیغام.ټېل_Fileline(fl);
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
    ساكن V3ErrorGuarded& s() VL_MT_خوندي {  // Singleton
        ساكن V3ErrorGuarded s_s;
        أرجع s_s;
    }

    // ACCESSORS
    ساكن فراغ debugDefault(صحيح level) VL_MT_UNSAFE { s().debugDefault(level); }
    ساكن صحيح debugDefault() VL_MT_خوندي { أرجع s().debugDefault(); }
    ساكن فراغ error_حد(صحيح level) VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().error_حد(level);
    }
    ساكن صحيح error_حد() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().error_حد();
    }
    ساكن فراغ warnFatal(bool flag) VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    ساكن bool warnFatal() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().warnFatal();
    }
    // returns %Error/%Warn
    ساكن string پیغام_مختاړی() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().پیغام_مختاړی();
    }
    ساكن صحيح error_شمېر() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().error_شمېر();
    }
    ساكن bool pretendError(V3_Error_کوډ کوډ) VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().pretendError(کوډ);
    }
    ساكن صحيح warn_شمېر() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().warn_شمېر();
    }
    ساكن bool errorContexted() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().errorContexted();
    }
    ساكن فراغ errorContexted(bool flag) VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    ساكن فراغ describedEachWarn(V3_Error_کوډ کوډ, bool flag) VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().describedEachWarn(کوډ, flag);
    }
    // METHODS
    ساكن فراغ incErrors() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().incErrors();
    }
    ساكن فراغ incWarnings() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().incWarnings();
    }
    ساكن فراغ init();
    ساكن bool isError(V3_Error_کوډ کوډ, bool supp) VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().isError(کوډ, supp);
    }
    ساكن bool isErrorOrWarn() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().isErrorOrWarn();
    }
    ساكن فراغ abortIfErrors() {
        إذا (error_شمېر()) abortIfWarnings();
    }
    ساكن فراغ abortIfWarnings();
    // Suppress next %Warn if user has it off
    ساكن فراغ suppressThisWarning() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    ساكن فراغ pretendError(V3_Error_کوډ کوډ, bool flag) VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().pretendError(کوډ, flag);
    }
    ساكن string کرښه_Str(ثابت حرف* filename, صحيح lineno) VL_خالص;
    ساكن string strip_Meta_متن(ثابت string& متن, bool stripMeta) VL_خالص;
    ساكن V3_Error_کوډ error_کوډ() VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        أرجع s().error_کوډ();
    }
    ساكن فراغ errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_خوندي_EXCLUDES(s().m_mutex) {
        ثابت V3_Recursive_قلف_Guard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    ساكن constexpr ثابت حرف* WARN_نور = "__WARNMORE__";
    ساكن string warn_نور() VL_MT_خوندي { أرجع WARN_نور; }
    // When printing an error/warning, mark beginning of context information (can nest)
    ساكن constexpr ثابت حرف* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    ساكن string warnContextBegin() VL_MT_خوندي { أرجع WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    ساكن constexpr ثابت حرف* WARN_CONTEXT_پای_ټکی = "__WARNEND__";
    ساكن string warn_Context_پای_ټکی() VL_MT_خوندي { أرجع WARN_CONTEXT_پای_ټکی; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    ساكن string warnAdditionalInfo() VL_MT_خوندي { أرجع "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    ساكن string warnContextNone() {
        V3Error::errorContexted(true);
        أرجع "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    ساكن std::ostringstream& v3errorPrep(V3_Error_کوډ کوډ) VL_ACQUIRE(s().m_mutex);
    ساكن std::ostringstream& v3error_Prep_File_کرښه(V3_Error_کوډ کوډ, ثابت حرف* file, صحيح کرښه)
        VL_ACQUIRE(s().m_mutex);
    ساكن std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { أرجع s().v3errorStr(); }
    // static, but often overridden in classes.
    ساكن فراغ v3error_پای_ټکی(ثابت std::ostringstream& sstr, ثابت string& extra, File_کرښه* fileline)
        VL_RELEASE(s().m_mutex);
    ساكن فراغ vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
فراغ v3error_پای_ټکی(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_خوندي;
فراغ v3error_پای_ټکی_Fatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_خوندي VL_ATTR_NORETURN;

#که_تعریف_شوی VL_MT_DISABLED_کوډ_UNIT
#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT_DEFINED 1
#وإلا
#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT_DEFINED 0
#د_که_پای

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#تعریف_کړه v3error_Build_پیغام(prep, پیغام_2) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << پیغام_2))
#تعریف_کړه v3warn_کوډ(کوډ, پیغام_2) v3error_پای_ټکی(v3error_Build_پیغام(V3Error::v3errorPrep(کوډ), پیغام_2))
#تعریف_کړه v3warn_کوډ_Fatal(کوډ, پیغام_2) \
    v3error_پای_ټکی_Fatal(v3error_Build_پیغام(V3Error::v3errorPrep(کوډ), پیغام_2))
#تعریف_کړه v3warn(کوډ, پیغام_2) v3warn_کوډ(V3_Error_کوډ::کوډ, پیغام_2)
#تعریف_کړه v3info(پیغام_2) v3warn_کوډ(V3_Error_کوډ::EC_INFO, پیغام_2)
#تعریف_کړه v3error(پیغام_2) v3warn_کوډ(V3_Error_کوډ::EC_ERROR, پیغام_2)
#تعریف_کړه v3fatal(پیغام_2) v3warn_کوډ_Fatal(V3_Error_کوډ::EC_FATAL, پیغام_2)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#تعریف_کړه v3fatalMany(پیغام_2) v3warn_کوډ_Fatal(V3_Error_کوډ::EC_FATALMANY, پیغام_2)
// Fatal exit; used instead of fatal() to mention the source code line
#تعریف_کړه v3fatalSrc(پیغام_2) \
    v3error_پای_ټکی_Fatal(v3error_Build_پیغام( \
        V3Error::v3error_Prep_File_کرښه(V3_Error_کوډ::EC_FATALSRC, __FILE__, __LINE__), پیغام_2))
// Use this when normal v3fatal is called in static method that overrides fileline.
#تعریف_کړه v3fatalStatic(پیغام_2) \
    ::v3error_پای_ټکی_Fatal(v3error_Build_پیغام(V3Error::v3errorPrep(V3_Error_کوډ::EC_FATAL), پیغام_2))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#تعریف_کړه UINFO(level, stmsg) \
    افعل { \
        إذا (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::کرښه_Str(__FILE__, __LINE__) << stmsg; \
            إذا (ss.str()[ss.str().اندازه() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } طالما (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#تعریف_کړه UINFO_مختاړی(stmsg) \
    افعل { std::cout << "- " << V3Error::کرښه_Str(__FILE__, __LINE__) << stmsg; } طالما (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#تعریف_کړه UINFOTREE(level, nodep, uinfo_پیغام, dumptree_پیغام) \
    افعل { \
        إذا (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_پیغام; \
            إذا (!us.str().تشه()) UINFO(level, us.str()); \
            إذا (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_پیغام; \
                إذا (ss.str().تشه()) ss << uinfo_پیغام; \
                nodep->dump_ونه("- " + V3Error::کرښه_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } طالما (false)

/// Compile statements only when debug build
#که_تعریف_شوی VL_DEBUG
#تعریف_کړه UDEBUGONLY(stmts) \
    افعل { stmts } طالما (false)
#وإلا
#تعریف_کړه UDEBUGONLY(stmts) \
    افعل { \
        إذا (false) { stmts } \
    } طالما (false)
#د_که_پای

/// Assert without error location, generally UASSERT_OBJ preferred
#تعریف_کړه UASSERT(condition, stmsg) \
    افعل { \
        إذا (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } طالما (false)
/// Assert with object to provide error location
#تعریف_کړه UASSERT_OBJ(condition, obj, stmsg) \
    افعل { \
        إذا (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } طالما (false)
// For use in V3Ast static functions only
#تعریف_کړه UASSERT_STATIC(condition, stmsg) \
    افعل { \
        إذا (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } طالما (false)
/// Check self test values for expected value.  Safe from side-effects.
#تعریف_کړه UASSERT_SELFTEST(got, توانیز_تابع) \
    افعل { \
        ثابت خپلکاری g = (got); \
        ثابت decltype(g) e{توانیز_تابع}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } طالما (false)

// Error that call not supported; only for some Ast functions
#تعریف_کړه V3ERROR_NA \
    افعل { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } طالما (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#تعریف_کړه V3ERROR_NA_RETURN(ارزښت) \
    V3ERROR_NA; \
    أرجع ارزښت

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#تعریف_کړه VL_DEFINE_DEBUG(...) \
    VL_ATTR_ناکارول_شوی ساكن صحيح debug##__VA_ARGS__() VL_MT_خوندي { \
        /* Don't complain this function is unused */ \
        (فراغ)&debug##__VA_ARGS__; \
        ساكن صحيح s_level = -1; \
        إذا (VL_UNLIKELY(s_level < 0)) { \
            std::string نښان{VL_STRINGIFY(__VA_ARGS__)}; \
            إذا (!نښان.تشه()) نښان[0] = std::tolower(نښان[0]); \
            ثابت بینښه debug_نښان = v3Global.opt.debugLevel(نښان); \
            ثابت بینښه debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ثابت بینښه debugLevel = debug_نښان >= debugSrc ? debug_نښان : debugSrc; \
            إذا (!v3Global.opt.شته()) أرجع static_cast<صحيح>(debugLevel); \
            s_level = static_cast<صحيح>(debugLevel); \
        } \
        أرجع s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#تعریف_کړه VL_DEFINE_DUMP(func, نښان) \
    VL_ATTR_ناکارول_شوی ساكن صحيح dump##func() VL_MT_خوندي { \
        /* Don't complain this function is unused */ \
        (فراغ)&dump##func; \
        ساكن صحيح s_level = -1; \
        إذا (VL_UNLIKELY(s_level < 0)) { \
            ثابت بینښه dump_نښان = v3Global.opt.dumpLevel(نښان); \
            ثابت بینښه dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ثابت بینښه dumpLevel = dump_نښان >= dumpSrc ? dump_نښان : dumpSrc; \
            إذا (!v3Global.opt.شته()) أرجع static_cast<صحيح>(dumpLevel); \
            s_level = static_cast<صحيح>(dumpLevel); \
        } \
        أرجع s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#تعریف_کړه VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(ګراف_Level, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(ونه_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(ونه_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_ناکارول_شوی ساكن صحيح dump_ونه_Either_Level() { \
        /* Don't complain this function is unused */ (فراغ)&dump_ونه_Either_Level; \
        أرجع dump_ونه_Json_Level() >= dump_ونه_Level() ? dump_ونه_Json_Level() : dump_ونه_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#د_که_پای  // Guard
