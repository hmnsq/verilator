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

#यदि_अपरिभाषितम् VERILATOR_V3ERROR_H_
#परिभाषय VERILATOR_V3ERROR_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Hash.h"
#अन्तर्भाव "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <array>
#अन्तर्भाव <bitset>
#अन्तर्भाव <cassert>
#अन्तर्भाव <cctype>
#अन्तर्भाव <deque>
#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <sstream>

class V3Error;
class File_पङ्क्तिः;

//######################################################################

class V3_Error_संहिता final {
सार्वजनिक:
    // clang-format off
    गणना en: uint8_t  {
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
        अप्रयुक्तम्_2,         // Unused genvar, parameter or signal message (Backward Compatibility)
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
        विस्तारः_2,          // Width mismatch
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
    गणना en m_e;
    V3_Error_संहिता()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Error_संहिता(en _e)
        : m_e{_e} {}
    explicit V3_Error_संहिता(ध्रुव वर्ण* msgp);  // Matching code or ERROR
    explicit V3_Error_संहिता(ध्रुव std::string& सन्देशः_2)
        : V3_Error_संहिता{सन्देशः_2.c_str()} {}
    explicit V3_Error_संहिता(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr संक्रियकः en() ध्रुव VL_MT_SAFE { प्रत्यागम m_e; }
    ध्रुव वर्ण* ascii() ध्रुव VL_MT_SAFE {
        स्थिर ध्रुव वर्ण* ध्रुव नामानि[] = {
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
        प्रत्यागम नामानि[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() ध्रुव VL_MT_SAFE {
        प्रत्यागम (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() ध्रुव VL_MT_SAFE { प्रत्यागम (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() ध्रुव VL_MT_SAFE {
        प्रत्यागम (m_e != EC_INFO && m_e < V3_Error_संहिता::EC_FIRST_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_e == V3_Error_संहिता::EC_FATAL || m_e == V3_Error_संहिता::EC_FATALMANY
               || m_e == V3_Error_संहिता::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() ध्रुव VL_MT_SAFE {
        प्रत्यागम m_e == V3_Error_संहिता::USERINFO || m_e == V3_Error_संहिता::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() ध्रुव VL_MT_SAFE {
        प्रत्यागम (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() ध्रुव VL_MT_SAFE {
        प्रत्यागम (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() ध्रुव VL_MT_SAFE {
        प्रत्यागम (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == विस्तारः_2
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() ध्रुव VL_MT_SAFE {
        प्रत्यागम (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() ध्रुव { प्रत्यागम m_e >= EC_FIRST_NAMED; }

गुप्त:
    V3_Error_संहिता renamed_गन्तव्यम्() ध्रुव {
        // Return a new error this error has been renamed to
        यदि (m_e == LITENDIAN) प्रत्यागम V3_Error_संहिता{ASCRANGE};
        प्रत्यागम V3_Error_संहिता{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() ध्रुव { प्रत्यागम renamed_गन्तव्यम्() != V3_Error_संहिता{EC_MIN}; }

सार्वजनिक:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    शून्य forDelegateCodes(std::function<शून्य(V3_Error_संहिता संहिता)> action) ध्रुव {
        // If add to this switch, also update isUnder()
        विकल्प (m_e) {
        पक्ष V3_Error_संहिता::E_UNSUPPORTED:
            action(V3_Error_संहिता{E_UNSUPPORTED});
            action(V3_Error_संहिता{COVERIGN});
            action(V3_Error_संहिता{SPECIFYIGN});
            प्रत्यागम;
        पक्ष V3_Error_संहिता::I_LINT:
            प्रति (पूर्णाङ्क i = V3_Error_संहिता::EC_MIN; i < V3_Error_संहिता::_ENUM_MAX; ++i) {
                ध्रुव V3_Error_संहिता subcode{i};
                यदि (subcode.lintError()) action(subcode);
            }
            प्रत्यागम;
        पक्ष V3_Error_संहिता::I_STYLE:
            प्रति (पूर्णाङ्क i = V3_Error_संहिता::EC_MIN; i < V3_Error_संहिता::_ENUM_MAX; ++i) {
                ध्रुव V3_Error_संहिता subcode{i};
                यदि (subcode.styleError()) action(subcode);
            }
            प्रत्यागम;
        पक्ष V3_Error_संहिता::अप्रयुक्तम्_2:
            action(V3_Error_संहिता{UNUSEDGENVAR});
            action(V3_Error_संहिता{UNUSEDLOOP});
            action(V3_Error_संहिता{UNUSEDPARAM});
            action(V3_Error_संहिता{UNUSEDSIGNAL});
            प्रत्यागम;
        पक्ष V3_Error_संहिता::विस्तारः_2:
            action(V3_Error_संहिता{WIDTHDIGITS});
            action(V3_Error_संहिता{WIDTHEXPAND});
            action(V3_Error_संहिता{WIDTHTRUNC});
            action(V3_Error_संहिता{WIDTHXZEXPAND});
            प्रत्यागम;
        स्वभाव: action(*this);
        }
    }
    bool isUnder(V3_Error_संहिता other) ध्रुव {
        // Reverse of forDelegateCodes, return true if this' code is under another
        यदि (VL_LIKELY(other == m_e)) प्रत्यागम true;
        विकल्प (other) {
        पक्ष V3_Error_संहिता::E_UNSUPPORTED:
            प्रत्यागम m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        पक्ष V3_Error_संहिता::I_LINT: प्रत्यागम lintError();
        पक्ष V3_Error_संहिता::I_STYLE: प्रत्यागम styleError();
        पक्ष V3_Error_संहिता::अप्रयुक्तम्_2:
            प्रत्यागम m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        पक्ष V3_Error_संहिता::विस्तारः_2:
            प्रत्यागम m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        स्वभाव: प्रत्यागम false;
        }
    }
    string url() ध्रुव;
};
constexpr bool संक्रियकः==(ध्रुव V3_Error_संहिता& lhs, ध्रुव V3_Error_संहिता& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool संक्रियकः==(ध्रुव V3_Error_संहिता& lhs, V3_Error_संहिता::en rhs) { प्रत्यागम lhs.m_e == rhs; }
constexpr bool संक्रियकः==(V3_Error_संहिता::en lhs, ध्रुव V3_Error_संहिता& rhs) { प्रत्यागम lhs == rhs.m_e; }
अन्तःस्थ std::ostream& संक्रियकः<<(std::ostream& os, ध्रुव V3_Error_संहिता& rhs) {
    प्रत्यागम os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VError_Bit_समुच्चयः final {
    std::bitset<V3_Error_संहिता::_ENUM_MAX> m_bitset;  // Enable for each code
    VError_Bit_समुच्चयः(ध्रुव VError_Bit_समुच्चयः& lhs, ध्रुव VError_Bit_समुच्चयः& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

सार्वजनिक:
    class AllOnes {};
    VError_Bit_समुच्चयः() = स्वभाव;
    explicit VError_Bit_समुच्चयः(AllOnes) { m_bitset.समुच्चयः(); }
    ~VError_Bit_समुच्चयः() = स्वभाव;
    bool test(V3_Error_संहिता संहिता) ध्रुव { प्रत्यागम m_bitset[संहिता]; }
    शून्य समुच्चयः(V3_Error_संहिता संहिता, bool flag) { m_bitset[संहिता] = flag; }
    V3Hash hash() ध्रुव {
        ध्रुव आकार_t hash_संहिता = std::hash<std::bitset<V3_Error_संहिता::_ENUM_MAX>>()(m_bitset);
        प्रत्यागम V3Hash{static_cast<uint64_t>(hash_संहिता)};
    }
    VError_Bit_समुच्चयः संक्रियकः&(ध्रुव VError_Bit_समुच्चयः& rhs) ध्रुव { प्रत्यागम VError_Bit_समुच्चयः{*this, rhs}; }
    bool संक्रियकः==(ध्रुव VError_Bit_समुच्चयः& rhs) ध्रुव { प्रत्यागम m_bitset == rhs.m_bitset; }
    string ascii() ध्रुव {  // LCOV_EXCL_START
        string परिणाम;
        प्रति (पूर्णाङ्क i = V3_Error_संहिता::EC_MIN; i < V3_Error_संहिता::_ENUM_MAX; ++i) {
            यदि (!test(V3_Error_संहिता{i})) परिणाम += " !"s + V3_Error_संहिता{i}.ascii();
        }
        प्रत्यागम परिणाम;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VError_सन्देशः final {
    // TYPES
    using File_पङ्क्तयः = std::deque<ध्रुव File_पङ्क्तिः*>;
    // MEMBERS
    V3_Error_संहिता m_संहिता;  // Which warning
    string m_text;  // Warning message text
    File_पङ्क्तिः* m_fileline;  // Primary warning fileline
    File_पङ्क्तयः m_filelines;  // Additional referenced filelines
सार्वजनिक:
    // CONSTRUCTORS
    VError_सन्देशः() { clear(); }
    ~VError_सन्देशः() = स्वभाव;
    शून्य clear() { init(V3_Error_संहिता::EC_MIN); }
    शून्य init(V3_Error_संहिता संहिता) {
        m_संहिता = संहिता;
        m_text = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3_Error_संहिता संहिता() ध्रुव { प्रत्यागम m_संहिता; }
    bool isClear() ध्रुव { प्रत्यागम m_संहिता == V3_Error_संहिता::EC_MIN; }
    string text() ध्रुव { प्रत्यागम m_text; }
    शून्य text(ध्रुव string& सन्देशः_2) { m_text = सन्देशः_2; }
    File_पङ्क्तिः* fileline() ध्रुव { प्रत्यागम m_fileline; }
    शून्य fileline(File_पङ्क्तिः* fl) { m_fileline = fl; }
    File_पङ्क्तयः filelines() ध्रुव { प्रत्यागम m_filelines; }
    // Returns identifier for use in warnRelated()
    पूर्णाङ्क प्रक्षिप_Fileline(ध्रुव File_पङ्क्तिः* fl) {
        m_filelines.emplace_पृष्ठभागः(fl);
        प्रत्यागम m_filelines.आकार() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    मित्रम् class V3Error;

सार्वजनिक:
    using Messages_समुच्चयः = std::समुच्चयः<std::string>;
    using ErrorExitCb = शून्य (*)(शून्य);

गुप्त:
    स्थिर constexpr अचिह्नित MAX_ERRORS = 50;  // Fatal after this may errors

    // MEMBERS
    // Tell user to see manual, 0=not yet, 1=doit, 2=disable
    bool m_tellManual VL_GUARDED_BY(m_mutex) = false;
    bool m_tell_आन्तरिकम् VL_GUARDED_BY(m_mutex) = false;
    VError_सन्देशः m_सन्देशः VL_GUARDED_BY(m_mutex);  // Message being formed
    bool m_errorSuppressed VL_GUARDED_BY(m_mutex)
        = false;  // Error being formed should be suppressed
    Messages_समुच्चयः m_messages VL_GUARDED_BY(m_mutex);  // Errors outputted, to remove dups
    ErrorExitCb m_errorExitCb VL_GUARDED_BY(m_mutex)
        = nullptr;  // Callback when error occurs for dumping
    bool m_errorContexted VL_GUARDED_BY(m_mutex) = false;  // Error being formed got context
    पूर्णाङ्क m_warn_गणनम् VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    पूर्णाङ्क m_err_गणनम् VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VError_Bit_समुच्चयः m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VError_Bit_समुच्चयः m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VError_Bit_समुच्चयः m_showedSuppressed VL_GUARDED_BY(m_mutex);
    पूर्णाङ्क m_debugDefault = 0;  // Option: --debugi Default debugging level
    पूर्णाङ्क m_error_सीमा VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    शून्य v3errorPrep(V3_Error_संहिता संहिता) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { प्रत्यागम m_errorStr; }
    शून्य v3error_अन्तः(ध्रुव std::ostringstream& sstr, ध्रुव string& extra, File_पङ्क्तिः* fileline)
        VL_REQUIRES(m_mutex);
    शून्य v3error_अन्तः_Guts(ध्रुव std::ostringstream& sstr, ध्रुव string& extra, File_पङ्क्तिः* fileline)
        VL_REQUIRES(m_mutex);

सार्वजनिक:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string सन्देशः_उपसर्गः() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warn_अधिकम्_Spaces() VL_REQUIRES(m_mutex);
    शून्य execErrorExitCb() VL_REQUIRES(m_mutex) {
        यदि (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { प्रत्यागम m_errorExitCb; }
    शून्य errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3_Error_संहिता संहिता, bool supp) VL_REQUIRES(m_mutex);
    शून्य vlAbortOrExit() VL_REQUIRES(m_mutex);
    शून्य errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    शून्य incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_गणनम्; }
    शून्य incErrors() VL_REQUIRES(m_mutex) { ++m_err_गणनम्; }
    पूर्णाङ्क error_गणनम्() ध्रुव VL_REQUIRES(m_mutex) { प्रत्यागम m_err_गणनम्; }
    bool isErrorOrWarn() ध्रुव VL_REQUIRES(m_mutex) {
        प्रत्यागम error_गणनम्() || (warnFatal() && warn_गणनम्());
    }
    bool pretendError(V3_Error_संहिता संहिता) VL_REQUIRES(m_mutex) { प्रत्यागम m_pretendError.test(संहिता); }
    शून्य pretendError(V3_Error_संहिता संहिता, bool flag) VL_REQUIRES(m_mutex) {
        संहिता.forDelegateCodes([this, flag](V3_Error_संहिता subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.समुच्चयः(subcode, flag); });
    }
    पूर्णाङ्क debugDefault() ध्रुव VL_MT_SAFE { प्रत्यागम m_debugDefault; }
    शून्य debugDefault(पूर्णाङ्क level) VL_MT_UNSAFE { m_debugDefault = level; }
    पूर्णाङ्क error_सीमा() ध्रुव VL_REQUIRES(m_mutex) { प्रत्यागम m_error_सीमा; }
    शून्य error_सीमा(पूर्णाङ्क level) VL_REQUIRES(m_mutex) { m_error_सीमा = level; }
    bool warnFatal() ध्रुव VL_REQUIRES(m_mutex) { प्रत्यागम m_warnFatal; }
    शून्य warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3_Error_संहिता error_संहिता() ध्रुव VL_REQUIRES(m_mutex) { प्रत्यागम m_सन्देशः.संहिता(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { प्रत्यागम m_errorContexted; }
    पूर्णाङ्क warn_गणनम्() ध्रुव VL_REQUIRES(m_mutex) { प्रत्यागम m_warn_गणनम्; }
    bool errorSuppressed() ध्रुव VL_REQUIRES(m_mutex) { प्रत्यागम m_errorSuppressed; }
    शून्य errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3_Error_संहिता संहिता) VL_REQUIRES(m_mutex) {
        प्रत्यागम m_describedEachWarn.test(संहिता);
    }
    शून्य describedEachWarn(V3_Error_संहिता संहिता, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.समुच्चयः(संहिता, flag);
    }
    शून्य suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(ध्रुव File_पङ्क्तिः* fl) VL_REQUIRES(m_mutex) {
        ध्रुव पूर्णाङ्क id = m_सन्देशः.प्रक्षिप_Fileline(fl);
        प्रत्यागम "__WARNRELATED("s + std::गन्तव्यम्_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        प्रत्यागम "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

सार्वजनिक:
    V3Error() = delete;
    स्थिर V3ErrorGuarded& s() VL_MT_SAFE {  // Singleton
        स्थिर V3ErrorGuarded s_s;
        प्रत्यागम s_s;
    }

    // ACCESSORS
    स्थिर शून्य debugDefault(पूर्णाङ्क level) VL_MT_UNSAFE { s().debugDefault(level); }
    स्थिर पूर्णाङ्क debugDefault() VL_MT_SAFE { प्रत्यागम s().debugDefault(); }
    स्थिर शून्य error_सीमा(पूर्णाङ्क level) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().error_सीमा(level);
    }
    स्थिर पूर्णाङ्क error_सीमा() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().error_सीमा();
    }
    स्थिर शून्य warnFatal(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    स्थिर bool warnFatal() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().warnFatal();
    }
    // returns %Error/%Warn
    स्थिर string सन्देशः_उपसर्गः() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().सन्देशः_उपसर्गः();
    }
    स्थिर पूर्णाङ्क error_गणनम्() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().error_गणनम्();
    }
    स्थिर bool pretendError(V3_Error_संहिता संहिता) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().pretendError(संहिता);
    }
    स्थिर पूर्णाङ्क warn_गणनम्() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().warn_गणनम्();
    }
    स्थिर bool errorContexted() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().errorContexted();
    }
    स्थिर शून्य errorContexted(bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    स्थिर शून्य describedEachWarn(V3_Error_संहिता संहिता, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().describedEachWarn(संहिता, flag);
    }
    // METHODS
    स्थिर शून्य incErrors() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().incErrors();
    }
    स्थिर शून्य incWarnings() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().incWarnings();
    }
    स्थिर शून्य init();
    स्थिर bool isError(V3_Error_संहिता संहिता, bool supp) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().isError(संहिता, supp);
    }
    स्थिर bool isErrorOrWarn() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().isErrorOrWarn();
    }
    स्थिर शून्य abortIfErrors() {
        यदि (error_गणनम्()) abortIfWarnings();
    }
    स्थिर शून्य abortIfWarnings();
    // Suppress next %Warn if user has it off
    स्थिर शून्य suppressThisWarning() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    स्थिर शून्य pretendError(V3_Error_संहिता संहिता, bool flag) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().pretendError(संहिता, flag);
    }
    स्थिर string पङ्क्तिः_Str(ध्रुव वर्ण* filename, पूर्णाङ्क lineno) VL_विशुद्धम्;
    स्थिर string stripMetaText(ध्रुव string& text, bool stripMeta) VL_विशुद्धम्;
    स्थिर V3_Error_संहिता error_संहिता() VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        प्रत्यागम s().error_संहिता();
    }
    स्थिर शून्य errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_SAFE_EXCLUDES(s().m_mutex) {
        ध्रुव V3_Recursive_तालकम्_Guard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    स्थिर constexpr ध्रुव वर्ण* WARN_अधिकम् = "__WARNMORE__";
    स्थिर string warn_अधिकम्() VL_MT_SAFE { प्रत्यागम WARN_अधिकम्; }
    // When printing an error/warning, mark beginning of context information (can nest)
    स्थिर constexpr ध्रुव वर्ण* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    स्थिर string warnContextBegin() VL_MT_SAFE { प्रत्यागम WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    स्थिर constexpr ध्रुव वर्ण* WARN_CONTEXT_अन्तः = "__WARNEND__";
    स्थिर string warn_Context_अन्तः() VL_MT_SAFE { प्रत्यागम WARN_CONTEXT_अन्तः; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    स्थिर string warnAdditionalInfo() VL_MT_SAFE { प्रत्यागम "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    स्थिर string warnContextNone() {
        V3Error::errorContexted(true);
        प्रत्यागम "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    स्थिर std::ostringstream& v3errorPrep(V3_Error_संहिता संहिता) VL_ACQUIRE(s().m_mutex);
    स्थिर std::ostringstream& v3error_Prep_File_पङ्क्तिः(V3_Error_संहिता संहिता, ध्रुव वर्ण* file, पूर्णाङ्क पङ्क्तिः)
        VL_ACQUIRE(s().m_mutex);
    स्थिर std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { प्रत्यागम s().v3errorStr(); }
    // static, but often overridden in classes.
    स्थिर शून्य v3error_अन्तः(ध्रुव std::ostringstream& sstr, ध्रुव string& extra, File_पङ्क्तिः* fileline)
        VL_RELEASE(s().m_mutex);
    स्थिर शून्य vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
शून्य v3error_अन्तः(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE;
शून्य v3error_अन्तः_Fatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_SAFE VL_ATTR_NORETURN;

#यदि_परिभाषितम् VL_MT_DISABLED_संहिता_UNIT
#परिभाषय VL_MT_DISABLED_संहिता_UNIT_DEFINED 1
#अन्यथा
#परिभाषय VL_MT_DISABLED_संहिता_UNIT_DEFINED 0
#यदि_अन्तः

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#परिभाषय v3error_Build_सन्देशः(prep, सन्देशः_2) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << सन्देशः_2))
#परिभाषय v3warn_संहिता(संहिता, सन्देशः_2) v3error_अन्तः(v3error_Build_सन्देशः(V3Error::v3errorPrep(संहिता), सन्देशः_2))
#परिभाषय v3warn_संहिता_Fatal(संहिता, सन्देशः_2) \
    v3error_अन्तः_Fatal(v3error_Build_सन्देशः(V3Error::v3errorPrep(संहिता), सन्देशः_2))
#परिभाषय v3warn(संहिता, सन्देशः_2) v3warn_संहिता(V3_Error_संहिता::संहिता, सन्देशः_2)
#परिभाषय v3info(सन्देशः_2) v3warn_संहिता(V3_Error_संहिता::EC_INFO, सन्देशः_2)
#परिभाषय v3error(सन्देशः_2) v3warn_संहिता(V3_Error_संहिता::EC_ERROR, सन्देशः_2)
#परिभाषय v3fatal(सन्देशः_2) v3warn_संहिता_Fatal(V3_Error_संहिता::EC_FATAL, सन्देशः_2)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#परिभाषय v3fatalMany(सन्देशः_2) v3warn_संहिता_Fatal(V3_Error_संहिता::EC_FATALMANY, सन्देशः_2)
// Fatal exit; used instead of fatal() to mention the source code line
#परिभाषय v3fatalSrc(सन्देशः_2) \
    v3error_अन्तः_Fatal(v3error_Build_सन्देशः( \
        V3Error::v3error_Prep_File_पङ्क्तिः(V3_Error_संहिता::EC_FATALSRC, __FILE__, __LINE__), सन्देशः_2))
// Use this when normal v3fatal is called in static method that overrides fileline.
#परिभाषय v3fatalStatic(सन्देशः_2) \
    ::v3error_अन्तः_Fatal(v3error_Build_सन्देशः(V3Error::v3errorPrep(V3_Error_संहिता::EC_FATAL), सन्देशः_2))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#परिभाषय UINFO(level, stmsg) \
    कुरु { \
        यदि (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::पङ्क्तिः_Str(__FILE__, __LINE__) << stmsg; \
            यदि (ss.str()[ss.str().आकार() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } यावत् (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#परिभाषय UINFO_उपसर्गः(stmsg) \
    कुरु { std::cout << "- " << V3Error::पङ्क्तिः_Str(__FILE__, __LINE__) << stmsg; } यावत् (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#परिभाषय UINFOTREE(level, nodep, uinfo_सन्देशः, dumptree_सन्देशः) \
    कुरु { \
        यदि (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_सन्देशः; \
            यदि (!us.str().रिक्तानि()) UINFO(level, us.str()); \
            यदि (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_सन्देशः; \
                यदि (ss.str().रिक्तानि()) ss << uinfo_सन्देशः; \
                nodep->dump_वृक्षः("- " + V3Error::पङ्क्तिः_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } यावत् (false)

/// Compile statements only when debug build
#यदि_परिभाषितम् VL_DEBUG
#परिभाषय UDEBUGONLY(stmts) \
    कुरु { stmts } यावत् (false)
#अन्यथा
#परिभाषय UDEBUGONLY(stmts) \
    कुरु { \
        यदि (false) { stmts } \
    } यावत् (false)
#यदि_अन्तः

/// Assert without error location, generally UASSERT_OBJ preferred
#परिभाषय UASSERT(condition, stmsg) \
    कुरु { \
        यदि (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } यावत् (false)
/// Assert with object to provide error location
#परिभाषय UASSERT_OBJ(condition, obj, stmsg) \
    कुरु { \
        यदि (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } यावत् (false)
// For use in V3Ast static functions only
#परिभाषय UASSERT_STATIC(condition, stmsg) \
    कुरु { \
        यदि (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } यावत् (false)
/// Check self test values for expected value.  Safe from side-effects.
#परिभाषय UASSERT_SELFTEST(got, घातांकः) \
    कुरु { \
        ध्रुव स्वचालित g = (got); \
        ध्रुव decltype(g) e{घातांकः}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } यावत् (false)

// Error that call not supported; only for some Ast functions
#परिभाषय V3ERROR_NA \
    कुरु { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } यावत् (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#परिभाषय V3ERROR_NA_RETURN(मानम्) \
    V3ERROR_NA; \
    प्रत्यागम मानम्

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#परिभाषय VL_DEFINE_DEBUG(...) \
    VL_ATTR_अप्रयुक्तम् स्थिर पूर्णाङ्क debug##__VA_ARGS__() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (शून्य)&debug##__VA_ARGS__; \
        स्थिर पूर्णाङ्क s_level = -1; \
        यदि (VL_UNLIKELY(s_level < 0)) { \
            std::string चिह्नकम्{VL_STRINGIFY(__VA_ARGS__)}; \
            यदि (!चिह्नकम्.रिक्तानि()) चिह्नकम्[0] = std::tolower(चिह्नकम्[0]); \
            ध्रुव अचिह्नित debug_चिह्नकम् = v3Global.opt.debugLevel(चिह्नकम्); \
            ध्रुव अचिह्नित debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            ध्रुव अचिह्नित debugLevel = debug_चिह्नकम् >= debugSrc ? debug_चिह्नकम् : debugSrc; \
            यदि (!v3Global.opt.उपलब्धम्()) प्रत्यागम static_cast<पूर्णाङ्क>(debugLevel); \
            s_level = static_cast<पूर्णाङ्क>(debugLevel); \
        } \
        प्रत्यागम s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#परिभाषय VL_DEFINE_DUMP(func, चिह्नकम्) \
    VL_ATTR_अप्रयुक्तम् स्थिर पूर्णाङ्क dump##func() VL_MT_SAFE { \
        /* Don't complain this function is unused */ \
        (शून्य)&dump##func; \
        स्थिर पूर्णाङ्क s_level = -1; \
        यदि (VL_UNLIKELY(s_level < 0)) { \
            ध्रुव अचिह्नित dump_चिह्नकम् = v3Global.opt.dumpLevel(चिह्नकम्); \
            ध्रुव अचिह्नित dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            ध्रुव अचिह्नित dumpLevel = dump_चिह्नकम् >= dumpSrc ? dump_चिह्नकम् : dumpSrc; \
            यदि (!v3Global.opt.उपलब्धम्()) प्रत्यागम static_cast<पूर्णाङ्क>(dumpLevel); \
            s_level = static_cast<पूर्णाङ्क>(dumpLevel); \
        } \
        प्रत्यागम s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#परिभाषय VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(GraphLevel, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(वृक्षः_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(वृक्षः_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_अप्रयुक्तम् स्थिर पूर्णाङ्क dump_वृक्षः_Either_Level() { \
        /* Don't complain this function is unused */ (शून्य)&dump_वृक्षः_Either_Level; \
        प्रत्यागम dump_वृक्षः_Json_Level() >= dump_वृक्षः_Level() ? dump_वृक्षः_Json_Level() : dump_वृक्षः_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#यदि_अन्तः  // Guard
