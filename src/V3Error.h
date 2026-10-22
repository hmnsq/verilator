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

#నిర్వచితం_కాకపోతే VERILATOR_V3ERROR_H_
#నిర్వచించు VERILATOR_V3ERROR_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Hash.h"
#అన్తర్భావ "V3Mutex.h"

// Limited V3 headers here - this is a base class for Vlc etc
#అన్తర్భావ "V3String.h"

#అన్తర్భావ <array>
#అన్తర్భావ <bitset>
#అన్తర్భావ <cassert>
#అన్తర్భావ <cctype>
#అన్తర్భావ <deque>
#అన్తర్భావ <map>
#అన్తర్భావ <set>
#అన్తర్భావ <sstream>

class V3Error;
class File_పంక్తి;

//######################################################################

class V3_Error_సంకేతం final {
బహిరంగ:
    // clang-format off
    గణన en: uint8_t  {
        EC_MIN=0,       // Keep first
        //
        EC_INFO,        // General information out
        EC_FATAL,       // Kill the program
        EC_FATALMANY,   // Kill the program, due to too many errors, suppress with --quiet-exit
        EC_FATALSRC,    // Kill the program, for internal source errors
        EC_ERROR,       // General error out, can't suppress
        EC_మొదటి_NAMED,  // Just a code so the program knows where to start info/errors
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
        EC_మొదటి_WARN,  // Just a code so the program knows where to start warnings
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
        వాడని_2,         // Unused genvar, parameter or signal message (Backward Compatibility)
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
        వెడల్పు_2,          // Width mismatch
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
    గణన en m_e;
    V3_Error_సంకేతం()
        : m_e{EC_MIN} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr V3_Error_సంకేతం(en _e)
        : m_e{_e} {}
    explicit V3_Error_సంకేతం(స్థిరరాశి అక్షరం* msgp);  // Matching code or ERROR
    explicit V3_Error_సంకేతం(స్థిరరాశి std::string& సందేశం_2)
        : V3_Error_సంకేతం{సందేశం_2.c_str()} {}
    explicit V3_Error_సంకేతం(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_e; }
    స్థిరరాశి అక్షరం* ascii() స్థిరరాశి VL_MT_సురక్షిత {
        స్థిర స్థిరరాశి అక్షరం* స్థిరరాశి పేర్లు[] = {
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
        తిరిగి పేర్లు[m_e];
    }
    // Warnings that default to off
    bool defaultsOff() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (m_e == IMPERFECTSCH || m_e == I_CELLDEFINE || m_e == I_ERRORING || styleError());
    }
    // Warnings that warn about nasty side effects
    bool dangerous() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి (m_e == COMBDLY); }
    // Insuppressible error codes that should always stop elaboration
    bool hardError() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (m_e != EC_INFO && m_e < V3_Error_సంకేతం::EC_మొదటి_WARN);
    }
    // Warning is fatal error, don't continue
    bool severityFatal() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_e == V3_Error_సంకేతం::EC_FATAL || m_e == V3_Error_సంకేతం::EC_FATALMANY
               || m_e == V3_Error_సంకేతం::EC_FATALSRC;
    }
    // Warning is -Info informational
    bool severityInfo() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_e == V3_Error_సంకేతం::USERINFO || m_e == V3_Error_సంకేతం::EC_INFO;
    }
    // Warnings we'll present to the user as errors
    // Later -Werror- options may make more of these.
    bool pretendError() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (m_e == ASSIGNIN || m_e == BADSTDPRAGMA || m_e == BADVLTPRAGMA || m_e == BLKANDNBLK
                || m_e == BLKLOOPINIT || m_e == CONTASSREG || m_e == ENCAPSULATED
                || m_e == ENDLABEL || m_e == ENUMITEMWIDTH || m_e == ENUMVALUE || m_e == FINALDLY
                || m_e == HIERPARAM || m_e == FUNCTIMECTL || m_e == IMPURE || m_e == MODMISSING
                || m_e == NOTREDOP || m_e == PARAMNODEFAULT || m_e == PINNOTFOUND
                || m_e == PKGNODECL || m_e == PROCASSWIRE || m_e == PROTOTYPEMIS
                || m_e == SUPERNFIRST || m_e == ZEROREPL);
    }
    // Warnings to mention manual
    bool mentionManual() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (pretendError() || m_e == EC_FATALSRC || m_e == SIDEEFFECT || m_e == SYMRSVDWORD
                || m_e == ZERODLY);
    }
    // Warnings that are lint only; includes all style warnings
    bool lintError() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (styleError() || m_e == ALWCOMBORDER || m_e == ASCRANGE || m_e == ASSIGNEQEXPR
                || m_e == BSSPACE || m_e == CASEINCOMPLETE || m_e == CASEOVERLAP
                || m_e == CASEWITHX || m_e == CASEX || m_e == CASTCONST || m_e == CMPCONST
                || m_e == COLONPLUS || m_e == IMPLICIT || m_e == IMPLICITSTATIC || m_e == LATCH
                || m_e == MISINDENT || m_e == NEWERSTD || m_e == PREPROCZERO || m_e == PINMISSING
                || m_e == REALCVT || m_e == STATICVAR || m_e == UNSIGNED || m_e == వెడల్పు_2
                || m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                || m_e == WIDTHXZEXPAND);
    }
    // Warnings that are style only
    bool styleError() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (m_e == ASSIGNDLY  // More than style, but for backward compatibility
                || m_e == BLKSEQ || m_e == DECLFILENAME || m_e == DEFPARAM || m_e == EOFNEWLINE
                || m_e == GENUNNAMED || m_e == IMPORTSTAR || m_e == INCABSPATH
                || m_e == MULTIDRIVENPROC || m_e == PINCONNECTEMPTY || m_e == PINNOCONNECT
                || m_e == PROCASSINIT || m_e == SIMILARNAME || m_e == SYNCASYNCNET
                || m_e == UNDRIVEN || m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP
                || m_e == UNUSEDPARAM || m_e == UNUSEDSIGNAL || m_e == VARHIDDEN);
    }
    bool isNamed() స్థిరరాశి { తిరిగి m_e >= EC_మొదటి_NAMED; }

అంతరంగ:
    V3_Error_సంకేతం renamed_గమ్యం() స్థిరరాశి {
        // Return a new error this error has been renamed to
        ఒకవేళ (m_e == LITENDIAN) తిరిగి V3_Error_సంకేతం{ASCRANGE};
        తిరిగి V3_Error_సంకేతం{EC_MIN};  // Not renamed; see isRenamed()
    }
    bool isRenamed() స్థిరరాశి { తిరిగి renamed_గమ్యం() != V3_Error_సంకేతం{EC_MIN}; }

బహిరంగ:
    // Call given callable on all error codes that result from given code.
    // Deals with codes which imply disabling multiple other codes.
    శూన్య forDelegateCodes(std::function<శూన్య(V3_Error_సంకేతం సంకేతం_2)> action) స్థిరరాశి {
        // If add to this switch, also update isUnder()
        ఎంపిక (m_e) {
        స్థితి V3_Error_సంకేతం::E_UNSUPPORTED:
            action(V3_Error_సంకేతం{E_UNSUPPORTED});
            action(V3_Error_సంకేతం{COVERIGN});
            action(V3_Error_సంకేతం{SPECIFYIGN});
            తిరిగి;
        స్థితి V3_Error_సంకేతం::I_LINT:
            కొరకు (పూర్ణాంకం i = V3_Error_సంకేతం::EC_MIN; i < V3_Error_సంకేతం::_ENUM_MAX; ++i) {
                స్థిరరాశి V3_Error_సంకేతం subcode{i};
                ఒకవేళ (subcode.lintError()) action(subcode);
            }
            తిరిగి;
        స్థితి V3_Error_సంకేతం::I_STYLE:
            కొరకు (పూర్ణాంకం i = V3_Error_సంకేతం::EC_MIN; i < V3_Error_సంకేతం::_ENUM_MAX; ++i) {
                స్థిరరాశి V3_Error_సంకేతం subcode{i};
                ఒకవేళ (subcode.styleError()) action(subcode);
            }
            తిరిగి;
        స్థితి V3_Error_సంకేతం::వాడని_2:
            action(V3_Error_సంకేతం{UNUSEDGENVAR});
            action(V3_Error_సంకేతం{UNUSEDLOOP});
            action(V3_Error_సంకేతం{UNUSEDPARAM});
            action(V3_Error_సంకేతం{UNUSEDSIGNAL});
            తిరిగి;
        స్థితి V3_Error_సంకేతం::వెడల్పు_2:
            action(V3_Error_సంకేతం{WIDTHDIGITS});
            action(V3_Error_సంకేతం{WIDTHEXPAND});
            action(V3_Error_సంకేతం{WIDTHTRUNC});
            action(V3_Error_సంకేతం{WIDTHXZEXPAND});
            తిరిగి;
        అప్రమేయ: action(*this);
        }
    }
    bool isUnder(V3_Error_సంకేతం other) స్థిరరాశి {
        // Reverse of forDelegateCodes, return true if this' code is under another
        ఒకవేళ (VL_LIKELY(other == m_e)) తిరిగి true;
        ఎంపిక (other) {
        స్థితి V3_Error_సంకేతం::E_UNSUPPORTED:
            తిరిగి m_e == E_UNSUPPORTED || m_e == COVERIGN || m_e == SPECIFYIGN;
        స్థితి V3_Error_సంకేతం::I_LINT: తిరిగి lintError();
        స్థితి V3_Error_సంకేతం::I_STYLE: తిరిగి styleError();
        స్థితి V3_Error_సంకేతం::వాడని_2:
            తిరిగి m_e == UNUSEDGENVAR || m_e == UNUSEDLOOP || m_e == UNUSEDPARAM
                   || m_e == UNUSEDSIGNAL;
        స్థితి V3_Error_సంకేతం::వెడల్పు_2:
            తిరిగి m_e == WIDTHDIGITS || m_e == WIDTHEXPAND || m_e == WIDTHTRUNC
                   || m_e == WIDTHXZEXPAND;
        అప్రమేయ: తిరిగి false;
        }
    }
    string url() స్థిరరాశి;
};
constexpr bool క్రియాకారకం==(స్థిరరాశి V3_Error_సంకేతం& lhs, స్థిరరాశి V3_Error_సంకేతం& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి V3_Error_సంకేతం& lhs, V3_Error_సంకేతం::en rhs) { తిరిగి lhs.m_e == rhs; }
constexpr bool క్రియాకారకం==(V3_Error_సంకేతం::en lhs, స్థిరరాశి V3_Error_సంకేతం& rhs) { తిరిగి lhs == rhs.m_e; }
అంతఃవరుస std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి V3_Error_సంకేతం& rhs) {
    తిరిగి os << rhs.ascii();
}

// ######################################################################

// Store a single bit for each error code
class VError_బిట్టు_సమితి final {
    std::bitset<V3_Error_సంకేతం::_ENUM_MAX> m_bitset;  // Enable for each code
    VError_బిట్టు_సమితి(స్థిరరాశి VError_బిట్టు_సమితి& lhs, స్థిరరాశి VError_బిట్టు_సమితి& rhs)
        : m_bitset{lhs.m_bitset & rhs.m_bitset} {}

బహిరంగ:
    class AllOnes {};
    VError_బిట్టు_సమితి() = అప్రమేయ;
    explicit VError_బిట్టు_సమితి(AllOnes) { m_bitset.సమితి(); }
    ~VError_బిట్టు_సమితి() = అప్రమేయ;
    bool test(V3_Error_సంకేతం సంకేతం_2) స్థిరరాశి { తిరిగి m_bitset[సంకేతం_2]; }
    శూన్య సమితి(V3_Error_సంకేతం సంకేతం_2, bool flag) { m_bitset[సంకేతం_2] = flag; }
    V3Hash hash() స్థిరరాశి {
        స్థిరరాశి కొలత_t hash_సంకేతం = std::hash<std::bitset<V3_Error_సంకేతం::_ENUM_MAX>>()(m_bitset);
        తిరిగి V3Hash{static_cast<uint64_t>(hash_సంకేతం)};
    }
    VError_బిట్టు_సమితి క్రియాకారకం&(స్థిరరాశి VError_బిట్టు_సమితి& rhs) స్థిరరాశి { తిరిగి VError_బిట్టు_సమితి{*this, rhs}; }
    bool క్రియాకారకం==(స్థిరరాశి VError_బిట్టు_సమితి& rhs) స్థిరరాశి { తిరిగి m_bitset == rhs.m_bitset; }
    string ascii() స్థిరరాశి {  // LCOV_EXCL_START
        string ఫలితం;
        కొరకు (పూర్ణాంకం i = V3_Error_సంకేతం::EC_MIN; i < V3_Error_సంకేతం::_ENUM_MAX; ++i) {
            ఒకవేళ (!test(V3_Error_సంకేతం{i})) ఫలితం += " !"s + V3_Error_సంకేతం{i}.ascii();
        }
        తిరిగి ఫలితం;
    }  // LCOV_EXCL_STOP
};

// ######################################################################

class VError_సందేశం final {
    // TYPES
    using File_పంక్తులు = std::deque<స్థిరరాశి File_పంక్తి*>;
    // MEMBERS
    V3_Error_సంకేతం m_సంకేతం;  // Which warning
    string m_పాఠ్యం;  // Warning message text
    File_పంక్తి* m_fileline;  // Primary warning fileline
    File_పంక్తులు m_filelines;  // Additional referenced filelines
బహిరంగ:
    // CONSTRUCTORS
    VError_సందేశం() { clear(); }
    ~VError_సందేశం() = అప్రమేయ;
    శూన్య clear() { init(V3_Error_సంకేతం::EC_MIN); }
    శూన్య init(V3_Error_సంకేతం సంకేతం_2) {
        m_సంకేతం = సంకేతం_2;
        m_పాఠ్యం = "";
        m_fileline = nullptr;
        m_filelines.clear();
    }
    // ACCESSORS
    V3_Error_సంకేతం సంకేతం_2() స్థిరరాశి { తిరిగి m_సంకేతం; }
    bool isClear() స్థిరరాశి { తిరిగి m_సంకేతం == V3_Error_సంకేతం::EC_MIN; }
    string పాఠ్యం() స్థిరరాశి { తిరిగి m_పాఠ్యం; }
    శూన్య పాఠ్యం(స్థిరరాశి string& సందేశం_2) { m_పాఠ్యం = సందేశం_2; }
    File_పంక్తి* fileline() స్థిరరాశి { తిరిగి m_fileline; }
    శూన్య fileline(File_పంక్తి* fl) { m_fileline = fl; }
    File_పంక్తులు filelines() స్థిరరాశి { తిరిగి m_filelines; }
    // Returns identifier for use in warnRelated()
    పూర్ణాంకం నెట్టు_Fileline(స్థిరరాశి File_పంక్తి* fl) {
        m_filelines.emplace_వెనుక(fl);
        తిరిగి m_filelines.కొలత() - 1;
    }
};

// ######################################################################

class V3ErrorGuarded final {
    // Should only be used by V3ErrorGuarded::m_mutex is already locked
    // contains guarded members
    మిత్ర class V3Error;

బహిరంగ:
    using Messages_సమితి = std::సమితి<std::string>;
    using ErrorExitCb = శూన్య (*)(శూన్య);

అంతరంగ:
    స్థిర constexpr గుర్తులేని MAX_ERRORS = 50;  // Fatal after this may errors

    // MEMBERS
    // Tell user to see manual, 0=not yet, 1=doit, 2=disable
    bool m_tellManual VL_GUARDED_BY(m_mutex) = false;
    bool m_tell_అంతర్గత VL_GUARDED_BY(m_mutex) = false;
    VError_సందేశం m_సందేశం VL_GUARDED_BY(m_mutex);  // Message being formed
    bool m_errorSuppressed VL_GUARDED_BY(m_mutex)
        = false;  // Error being formed should be suppressed
    Messages_సమితి m_messages VL_GUARDED_BY(m_mutex);  // Errors outputted, to remove dups
    ErrorExitCb m_errorExitCb VL_GUARDED_BY(m_mutex)
        = nullptr;  // Callback when error occurs for dumping
    bool m_errorContexted VL_GUARDED_BY(m_mutex) = false;  // Error being formed got context
    పూర్ణాంకం m_warn_లెక్క VL_GUARDED_BY(m_mutex) = 0;  // Warning count
    పూర్ణాంకం m_err_లెక్క VL_GUARDED_BY(m_mutex) = 0;  // Error count
    // Pretend this warning is an error
    VError_బిట్టు_సమితి m_pretendError VL_GUARDED_BY(m_mutex);
    // Told user specifics about this warning
    VError_బిట్టు_సమితి m_describedEachWarn VL_GUARDED_BY(m_mutex);
    // Debug about suppressed this warning
    VError_బిట్టు_సమితి m_showedSuppressed VL_GUARDED_BY(m_mutex);
    పూర్ణాంకం m_debugDefault = 0;  // Option: --debugi Default debugging level
    పూర్ణాంకం m_error_హద్దు VL_GUARDED_BY(m_mutex)
        = MAX_ERRORS;  // Option: --error-limit Number of errors before exit
    bool m_warnFatal VL_GUARDED_BY(m_mutex) = true;  // Option: --warnFatal Warnings are fatal
    std::ostringstream m_errorStr VL_GUARDED_BY(m_mutex);  // Error string being formed

    // METHODS
    శూన్య v3errorPrep(V3_Error_సంకేతం సంకేతం_2) VL_REQUIRES(m_mutex);
    std::ostringstream& v3errorStr() VL_REQUIRES(m_mutex) { తిరిగి m_errorStr; }
    శూన్య v3error_చివర(స్థిరరాశి std::ostringstream& sstr, స్థిరరాశి string& extra, File_పంక్తి* fileline)
        VL_REQUIRES(m_mutex);
    శూన్య v3error_చివర_Guts(స్థిరరాశి std::ostringstream& sstr, స్థిరరాశి string& extra, File_పంక్తి* fileline)
        VL_REQUIRES(m_mutex);

బహిరంగ:
    V3RecursiveMutex m_mutex;  // Make sure only single thread is in class

    string సందేశం_ఉపసర్గ() VL_REQUIRES(m_mutex);  // returns %Error/%Warn
    string warn_మరిన్ని_Spaces() VL_REQUIRES(m_mutex);
    శూన్య execErrorExitCb() VL_REQUIRES(m_mutex) {
        ఒకవేళ (m_errorExitCb) m_errorExitCb();
    }
    ErrorExitCb errorExitCb() VL_REQUIRES(m_mutex) { తిరిగి m_errorExitCb; }
    శూన్య errorExitCb(ErrorExitCb cb) VL_REQUIRES(m_mutex) { m_errorExitCb = cb; }
    bool isError(V3_Error_సంకేతం సంకేతం_2, bool supp) VL_REQUIRES(m_mutex);
    శూన్య vlAbortOrExit() VL_REQUIRES(m_mutex);
    శూన్య errorContexted(bool flag) VL_REQUIRES(m_mutex) { m_errorContexted = flag; }
    శూన్య incWarnings() VL_REQUIRES(m_mutex) { ++m_warn_లెక్క; }
    శూన్య incErrors() VL_REQUIRES(m_mutex) { ++m_err_లెక్క; }
    పూర్ణాంకం error_లెక్క() స్థిరరాశి VL_REQUIRES(m_mutex) { తిరిగి m_err_లెక్క; }
    bool isErrorOrWarn() స్థిరరాశి VL_REQUIRES(m_mutex) {
        తిరిగి error_లెక్క() || (warnFatal() && warn_లెక్క());
    }
    bool pretendError(V3_Error_సంకేతం సంకేతం_2) VL_REQUIRES(m_mutex) { తిరిగి m_pretendError.test(సంకేతం_2); }
    శూన్య pretendError(V3_Error_సంకేతం సంకేతం_2, bool flag) VL_REQUIRES(m_mutex) {
        సంకేతం_2.forDelegateCodes([this, flag](V3_Error_సంకేతం subcode)
                                  VL_REQUIRES(m_mutex) { m_pretendError.సమితి(subcode, flag); });
    }
    పూర్ణాంకం debugDefault() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_debugDefault; }
    శూన్య debugDefault(పూర్ణాంకం level) VL_MT_UNSAFE { m_debugDefault = level; }
    పూర్ణాంకం error_హద్దు() స్థిరరాశి VL_REQUIRES(m_mutex) { తిరిగి m_error_హద్దు; }
    శూన్య error_హద్దు(పూర్ణాంకం level) VL_REQUIRES(m_mutex) { m_error_హద్దు = level; }
    bool warnFatal() స్థిరరాశి VL_REQUIRES(m_mutex) { తిరిగి m_warnFatal; }
    శూన్య warnFatal(bool flag) VL_REQUIRES(m_mutex) { m_warnFatal = flag; }
    V3_Error_సంకేతం error_సంకేతం() స్థిరరాశి VL_REQUIRES(m_mutex) { తిరిగి m_సందేశం.సంకేతం_2(); }
    bool errorContexted() VL_REQUIRES(m_mutex) { తిరిగి m_errorContexted; }
    పూర్ణాంకం warn_లెక్క() స్థిరరాశి VL_REQUIRES(m_mutex) { తిరిగి m_warn_లెక్క; }
    bool errorSuppressed() స్థిరరాశి VL_REQUIRES(m_mutex) { తిరిగి m_errorSuppressed; }
    శూన్య errorSuppressed(bool flag) VL_REQUIRES(m_mutex) { m_errorSuppressed = flag; }
    bool describedEachWarn(V3_Error_సంకేతం సంకేతం_2) VL_REQUIRES(m_mutex) {
        తిరిగి m_describedEachWarn.test(సంకేతం_2);
    }
    శూన్య describedEachWarn(V3_Error_సంకేతం సంకేతం_2, bool flag) VL_REQUIRES(m_mutex) {
        m_describedEachWarn.సమితి(సంకేతం_2, flag);
    }
    శూన్య suppressThisWarning() VL_REQUIRES(m_mutex);
    string warnRelated(స్థిరరాశి File_పంక్తి* fl) VL_REQUIRES(m_mutex) {
        స్థిరరాశి పూర్ణాంకం id = m_సందేశం.నెట్టు_Fileline(fl);
        తిరిగి "__WARNRELATED("s + std::గమ్యం_string(id) + ")";
    }
    string warnContextNone() VL_REQUIRES(m_mutex) {
        errorContexted(true);
        తిరిగి "";
    }
};

// ######################################################################

class V3Error final {
    // Static members only

బహిరంగ:
    V3Error() = delete;
    స్థిర V3ErrorGuarded& s() VL_MT_సురక్షిత {  // Singleton
        స్థిర V3ErrorGuarded s_s;
        తిరిగి s_s;
    }

    // ACCESSORS
    స్థిర శూన్య debugDefault(పూర్ణాంకం level) VL_MT_UNSAFE { s().debugDefault(level); }
    స్థిర పూర్ణాంకం debugDefault() VL_MT_సురక్షిత { తిరిగి s().debugDefault(); }
    స్థిర శూన్య error_హద్దు(పూర్ణాంకం level) VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().error_హద్దు(level);
    }
    స్థిర పూర్ణాంకం error_హద్దు() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().error_హద్దు();
    }
    స్థిర శూన్య warnFatal(bool flag) VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().warnFatal(flag);
    }
    స్థిర bool warnFatal() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().warnFatal();
    }
    // returns %Error/%Warn
    స్థిర string సందేశం_ఉపసర్గ() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().సందేశం_ఉపసర్గ();
    }
    స్థిర పూర్ణాంకం error_లెక్క() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().error_లెక్క();
    }
    స్థిర bool pretendError(V3_Error_సంకేతం సంకేతం_2) VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().pretendError(సంకేతం_2);
    }
    స్థిర పూర్ణాంకం warn_లెక్క() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().warn_లెక్క();
    }
    స్థిర bool errorContexted() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().errorContexted();
    }
    స్థిర శూన్య errorContexted(bool flag) VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().errorContexted(flag);
    }
    స్థిర శూన్య describedEachWarn(V3_Error_సంకేతం సంకేతం_2, bool flag) VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().describedEachWarn(సంకేతం_2, flag);
    }
    // METHODS
    స్థిర శూన్య incErrors() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().incErrors();
    }
    స్థిర శూన్య incWarnings() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().incWarnings();
    }
    స్థిర శూన్య init();
    స్థిర bool isError(V3_Error_సంకేతం సంకేతం_2, bool supp) VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().isError(సంకేతం_2, supp);
    }
    స్థిర bool isErrorOrWarn() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().isErrorOrWarn();
    }
    స్థిర శూన్య abortIfErrors() {
        ఒకవేళ (error_లెక్క()) abortIfWarnings();
    }
    స్థిర శూన్య abortIfWarnings();
    // Suppress next %Warn if user has it off
    స్థిర శూన్య suppressThisWarning() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().suppressThisWarning();
    }
    స్థిర శూన్య pretendError(V3_Error_సంకేతం సంకేతం_2, bool flag) VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().pretendError(సంకేతం_2, flag);
    }
    స్థిర string పంక్తి_Str(స్థిరరాశి అక్షరం* filename, పూర్ణాంకం lineno) VL_శుద్ధ;
    స్థిర string strip_Meta_పాఠ్యం(స్థిరరాశి string& పాఠ్యం, bool stripMeta) VL_శుద్ధ;
    స్థిర V3_Error_సంకేతం error_సంకేతం() VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        తిరిగి s().error_సంకేతం();
    }
    స్థిర శూన్య errorExitCb(V3ErrorGuarded::ErrorExitCb cb) VL_MT_సురక్షిత_EXCLUDES(s().m_mutex) {
        స్థిరరాశి V3_Recursive_తాళం_Guard guard{s().m_mutex};
        s().errorExitCb(cb);
    }

    // When printing an error/warning, print prefix for multiline message
    స్థిర constexpr స్థిరరాశి అక్షరం* WARN_మరిన్ని = "__WARNMORE__";
    స్థిర string warn_మరిన్ని() VL_MT_సురక్షిత { తిరిగి WARN_మరిన్ని; }
    // When printing an error/warning, mark beginning of context information (can nest)
    స్థిర constexpr స్థిరరాశి అక్షరం* WARN_CONTEXT_BEGIN = "__WARNBEGIN__";
    స్థిర string warnContextBegin() VL_MT_సురక్షిత { తిరిగి WARN_CONTEXT_BEGIN; }
    // When printing an error/warning, mark end of context information (can nest)
    స్థిర constexpr స్థిరరాశి అక్షరం* WARN_CONTEXT_చివర = "__WARNEND__";
    స్థిర string warn_Context_చివర() VL_MT_సురక్షిత { తిరిగి WARN_CONTEXT_చివర; }
    // This function marks place in error message from which point message
    // should be printed after information on the error code.
    // The post-processing is done in v3errorEnd function.
    స్థిర string warnAdditionalInfo() VL_MT_సురక్షిత { తిరిగి "__WARNADDITIONALINFO__"; }
    /// When building an error, don't show context info
    స్థిర string warnContextNone() {
        V3Error::errorContexted(true);
        తిరిగి "";
    }

    // Internals for v3error()/v3fatal() macros only
    // Error end takes the string stream to output, be careful to seek() as needed
    స్థిర std::ostringstream& v3errorPrep(V3_Error_సంకేతం సంకేతం_2) VL_ACQUIRE(s().m_mutex);
    స్థిర std::ostringstream& v3error_Prep_File_పంక్తి(V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి అక్షరం* file, పూర్ణాంకం పంక్తి)
        VL_ACQUIRE(s().m_mutex);
    స్థిర std::ostringstream& v3errorStr() VL_REQUIRES(s().m_mutex) { తిరిగి s().v3errorStr(); }
    // static, but often overridden in classes.
    స్థిర శూన్య v3error_చివర(స్థిరరాశి std::ostringstream& sstr, స్థిరరాశి string& extra, File_పంక్తి* fileline)
        VL_RELEASE(s().m_mutex);
    స్థిర శూన్య vlAbort();
};

// Global versions, so that if the class doesn't define an operator, we get the functions anyway.
శూన్య v3error_చివర(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) VL_MT_సురక్షిత;
శూన్య v3error_చివర_Fatal(std::ostringstream& sstr)
    VL_RELEASE(V3Error::s().m_mutex) VL_MT_సురక్షిత VL_ATTR_NORETURN;

#నిర్వచితమైతే VL_MT_DISABLED_సంకేతం_UNIT
#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT_DEFINED 1
#లేకపోతే
#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT_DEFINED 0
#అయితే_ముగింపు

// Theses allow errors using << operators: v3error("foo"<<"bar");
// Careful, you can't put () around msg, as you would in most macro definitions.
// 'V3Error::v3errorPrep(code) << msg' could be more efficient but the order of function calls in a
// single statement can be arbitrary until C++17, thus make it possible to execute
// V3Error::v3errorPrep that acquires the lock after functions in the msg may require it. So we use
// the comma operator (,) to guarantee the execution order here.
#నిర్వచించు v3error_Build_సందేశం(prep, సందేశం_2) \
    (prep, static_cast<std::ostringstream&>(V3Error::v3errorStr() << సందేశం_2))
#నిర్వచించు v3warn_సంకేతం(సంకేతం_2, సందేశం_2) v3error_చివర(v3error_Build_సందేశం(V3Error::v3errorPrep(సంకేతం_2), సందేశం_2))
#నిర్వచించు v3warn_సంకేతం_Fatal(సంకేతం_2, సందేశం_2) \
    v3error_చివర_Fatal(v3error_Build_సందేశం(V3Error::v3errorPrep(సంకేతం_2), సందేశం_2))
#నిర్వచించు v3warn(సంకేతం_2, సందేశం_2) v3warn_సంకేతం(V3_Error_సంకేతం::సంకేతం_2, సందేశం_2)
#నిర్వచించు v3info(సందేశం_2) v3warn_సంకేతం(V3_Error_సంకేతం::EC_INFO, సందేశం_2)
#నిర్వచించు v3error(సందేశం_2) v3warn_సంకేతం(V3_Error_సంకేతం::EC_ERROR, సందేశం_2)
#నిర్వచించు v3fatal(సందేశం_2) v3warn_సంకేతం_Fatal(V3_Error_సంకేతం::EC_FATAL, సందేశం_2)
// Fatal exit; used instead of fatal() if message gets suppressed with --quiet-exit
#నిర్వచించు v3fatalMany(సందేశం_2) v3warn_సంకేతం_Fatal(V3_Error_సంకేతం::EC_FATALMANY, సందేశం_2)
// Fatal exit; used instead of fatal() to mention the source code line
#నిర్వచించు v3fatalSrc(సందేశం_2) \
    v3error_చివర_Fatal(v3error_Build_సందేశం( \
        V3Error::v3error_Prep_File_పంక్తి(V3_Error_సంకేతం::EC_FATALSRC, __FILE__, __LINE__), సందేశం_2))
// Use this when normal v3fatal is called in static method that overrides fileline.
#నిర్వచించు v3fatalStatic(సందేశం_2) \
    ::v3error_చివర_Fatal(v3error_Build_సందేశం(V3Error::v3errorPrep(V3_Error_సంకేతం::EC_FATAL), సందేశం_2))

/// Print a message when debug() >= level.  stmsg is stream; e.g. use as '"foo=" << foo'
//
// Requires debug() function to exist in current scope, to hack this in temporarily:
// auto debug = []() -> bool { return V3Error::debugDefault(); };
#నిర్వచించు UINFO(level, stmsg) \
    చేయి { \
        ఒకవేళ (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream ss; \
            ss << "- " << V3Error::పంక్తి_Str(__FILE__, __LINE__) << stmsg; \
            ఒకవేళ (ss.str()[ss.str().కొలత() - 1] != '\n') ss << '\n'; \
            std::cout << ss.str(); \
        } \
    } అయితే (false)

/// Print the prefix of UINFO, but no newline.  No level argument, as
/// always need an if() at the caller site to determine if rest of the line
/// gets printed or not
#నిర్వచించు UINFO_ఉపసర్గ(stmsg) \
    చేయి { std::cout << "- " << V3Error::పంక్తి_Str(__FILE__, __LINE__) << stmsg; } అయితే (false)

/// Based on debug level, call UINFO then dumpTree on nodep, using given message prefix
/// If uinfo_msg = "", suppress the first UINFO
/// If dumptree_msg = "", use the uinfo_msg; note any uinfo_msg side effects will happen twice.
#నిర్వచించు UINFOTREE(level, nodep, uinfo_సందేశం, dumptree_సందేశం) \
    చేయి { \
        ఒకవేళ (VL_UNCOVERABLE(debug() >= (level))) { \
            std::ostringstream us; \
            us << uinfo_సందేశం; \
            ఒకవేళ (!us.str().ఖాళీలు()) UINFO(level, us.str()); \
            ఒకవేళ (nodep) { \
                std::ostringstream ss; \
                ss << dumptree_సందేశం; \
                ఒకవేళ (ss.str().ఖాళీలు()) ss << uinfo_సందేశం; \
                nodep->dump_వృక్షం("- " + V3Error::పంక్తి_Str(__FILE__, __LINE__) + ss.str() + " - "); \
            } \
        } \
    } అయితే (false)

/// Compile statements only when debug build
#నిర్వచితమైతే VL_DEBUG
#నిర్వచించు UDEBUGONLY(stmts) \
    చేయి { stmts } అయితే (false)
#లేకపోతే
#నిర్వచించు UDEBUGONLY(stmts) \
    చేయి { \
        ఒకవేళ (false) { stmts } \
    } అయితే (false)
#అయితే_ముగింపు

/// Assert without error location, generally UASSERT_OBJ preferred
#నిర్వచించు UASSERT(condition, stmsg) \
    చేయి { \
        ఒకవేళ (VL_UNCOVERABLE(!(condition))) v3fatalSrc(stmsg); \
    } అయితే (false)
/// Assert with object to provide error location
#నిర్వచించు UASSERT_OBJ(condition, obj, stmsg) \
    చేయి { \
        ఒకవేళ (VL_UNCOVERABLE(!(condition))) (obj)->v3fatalSrc(stmsg); \
    } అయితే (false)
// For use in V3Ast static functions only
#నిర్వచించు UASSERT_STATIC(condition, stmsg) \
    చేయి { \
        ఒకవేళ (VL_UNCOVERABLE(!(condition))) { \
            std::cerr << "Internal Error: " << __FILE__ << ":" << std::dec << __LINE__ << ":" \
                      << (stmsg) << std::endl; \
            V3Error::vlAbort(); \
        } \
    } అయితే (false)
/// Check self test values for expected value.  Safe from side-effects.
#నిర్వచించు UASSERT_SELFTEST(got, ఘాతాంకం) \
    చేయి { \
        స్థిరరాశి స్వయంచాలక g = (got); \
        స్థిరరాశి decltype(g) e{ఘాతాంకం}; \
        UASSERT(g == e, "Self-test failed '" #got "==" #exp "'" \
                        " got=" \
                            << g << " expected=" << e); \
    } అయితే (false)

// Error that call not supported; only for some Ast functions
#నిర్వచించు V3ERROR_NA \
    చేయి { \
        v3error("Internal: Unexpected Call"); \
        v3fatalSrc("Unexpected Call"); \
    } అయితే (false)

/// Throw fatal and return a value. The return value will never really be
/// needed, but required to avoid compiler error.
#నిర్వచించు V3ERROR_NA_RETURN(విలువ) \
    V3ERROR_NA; \
    తిరిగి విలువ

// Helper macros for VL_DEFINE_DEBUG_FUNCTIONS
// Takes an optional "name" (as __VA_ARGS__)
#నిర్వచించు VL_DEFINE_DEBUG(...) \
    VL_ATTR_వాడని స్థిర పూర్ణాంకం debug##__VA_ARGS__() VL_MT_సురక్షిత { \
        /* Don't complain this function is unused */ \
        (శూన్య)&debug##__VA_ARGS__; \
        స్థిర పూర్ణాంకం s_level = -1; \
        ఒకవేళ (VL_UNLIKELY(s_level < 0)) { \
            std::string ట్యాగ్{VL_STRINGIFY(__VA_ARGS__)}; \
            ఒకవేళ (!ట్యాగ్.ఖాళీలు()) ట్యాగ్[0] = std::tolower(ట్యాగ్[0]); \
            స్థిరరాశి గుర్తులేని debug_ట్యాగ్ = v3Global.opt.debugLevel(ట్యాగ్); \
            స్థిరరాశి గుర్తులేని debugSrc = v3Global.opt.debugSrcLevel(__FILE__); \
            స్థిరరాశి గుర్తులేని debugLevel = debug_ట్యాగ్ >= debugSrc ? debug_ట్యాగ్ : debugSrc; \
            ఒకవేళ (!v3Global.opt.అందుబాటు()) తిరిగి static_cast<పూర్ణాంకం>(debugLevel); \
            s_level = static_cast<పూర్ణాంకం>(debugLevel); \
        } \
        తిరిగి s_level; \
    } \
    static_assert(true, "")

// Takes an optional "name" (as __VA_ARGS__)
#నిర్వచించు VL_DEFINE_DUMP(func, ట్యాగ్) \
    VL_ATTR_వాడని స్థిర పూర్ణాంకం dump##func() VL_MT_సురక్షిత { \
        /* Don't complain this function is unused */ \
        (శూన్య)&dump##func; \
        స్థిర పూర్ణాంకం s_level = -1; \
        ఒకవేళ (VL_UNLIKELY(s_level < 0)) { \
            స్థిరరాశి గుర్తులేని dump_ట్యాగ్ = v3Global.opt.dumpLevel(ట్యాగ్); \
            స్థిరరాశి గుర్తులేని dumpSrc = v3Global.opt.dumpSrcLevel(__FILE__); \
            స్థిరరాశి గుర్తులేని dumpLevel = dump_ట్యాగ్ >= dumpSrc ? dump_ట్యాగ్ : dumpSrc; \
            ఒకవేళ (!v3Global.opt.అందుబాటు()) తిరిగి static_cast<పూర్ణాంకం>(dumpLevel); \
            s_level = static_cast<పూర్ణాంకం>(dumpLevel); \
        } \
        తిరిగి s_level; \
    } \
    static_assert(true, "")

// Define debug*() and dump*() routines. This needs to be added to every compilation unit so that
// --debugi-<tag/srcfile> and --dumpi-<tag/srcfile> can be used to control debug prints and dumping
#నిర్వచించు VL_DEFINE_DEBUG_FUNCTIONS \
    VL_DEFINE_DEBUG(); /* Define 'int debug()' for --debugi */ \
    VL_DEFINE_DUMP(Level, ""); /* Define 'int dumpLevel()' for --dumpi */ \
    VL_DEFINE_DUMP(DfgLevel, "dfg"); /* Define 'int dumpDfgLevel()' for --dumpi-level */ \
    VL_DEFINE_DUMP(లేఖాచిత్రం_Level, "graph"); /* Define 'int dumpGraphLevel()' for dumpi-graph */ \
    VL_DEFINE_DUMP(వృక్షం_Level, "tree"); /* Define 'int dumpTreeLevel()' for dumpi-tree */ \
    VL_DEFINE_DUMP(వృక్షం_Json_Level, \
                   "tree-json"); /* Define 'int dumpTreeJsonLevel()' for dumpi-tree-json */ \
    VL_ATTR_వాడని స్థిర పూర్ణాంకం dump_వృక్షం_Either_Level() { \
        /* Don't complain this function is unused */ (శూన్య)&dump_వృక్షం_Either_Level; \
        తిరిగి dump_వృక్షం_Json_Level() >= dump_వృక్షం_Level() ? dump_వృక్షం_Json_Level() : dump_వృక్షం_Level(); \
    } \
    static_assert(true, "")

//----------------------------------------------------------------------

#అయితే_ముగింపు  // Guard
