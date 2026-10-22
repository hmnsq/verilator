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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

// clang-format off
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Os.h"
#ضمن "V3String.h"
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#د_که_پای
#ضمن "V3Waiver.h"
// clang-format on

#ضمن <algorithm>
#ضمن <iomanip>
#ضمن <limits>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_کرښه_Singleton::filenameLetters(fileNameIdx_t fileno) VL_خالص {
    constexpr صحيح اندازه
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    حرف بهر[اندازه];
    حرف* op = بهر + اندازه - 1;
    *--op = '\0';  // We build backwards
    صحيح عدد = fileno;
    افعل {
        *--op = 'a' + عدد % 26;
        عدد /= 26;
    } طالما (عدد);
    أرجع op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_کرښه_Singleton::fileNameIdx_t File_کرښه_Singleton::name_منزل_شمېره(ثابت string& filename) {
    ثابت خپلکاری pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.لومړی->دوهم;
    إذا (pair.دوهم) {
        ثابت اندازه_t بل_Idx = m_نومونه.اندازه();
        UASSERT(بل_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_منزل_Str(بل_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(بل_Idx);
        m_نومونه.ټېل_شا(filename);
        m_languages.ټېل_شا(V3_Lang_کوډ::mostRecent());
    }
    أرجع idx;
}

فراغ File_کرښه_Singleton::file_Name_عدد_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    لكل (ثابت خپلکاری& itr : m_namemap) {
        std::string filename = itr.لومړی;
        إذا (filename == V3Options::get_Std_Package_لار()) {
            filename = "<verilated_std>";
        } وإلا إذا (filename == V3Options::get_Std_Waiver_لار()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.دوهم) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_بهر_Formatter::quoteNameControls(V3Os::filename_Real_لار(itr.لومړی)) << '"'
           << ", \"language\":\"" << شمېره_منزل_Lang(itr.دوهم).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_کرښه_Singleton::پیغام_En_سېټ_Idx_t File_کرښه_Singleton::جمع_کړه_پیغام_En_بټ_سېټ(ثابت پیغام_En_بټ_سېټ& بټ_سېټ)
    VL_MT_خوندي_EXCLUDES(m_mutex) {
    ثابت V3_قلف_Guard قلف_2{m_mutex};
    ثابت خپلکاری pair = m_interned_پیغام_En_Idxs.emplace(بټ_سېټ, 0);
    پیغام_En_سېټ_Idx_t& idx = pair.لومړی->دوهم;
    إذا (pair.دوهم) {
        ثابت اندازه_t بل_Idx = m_interned_پیغام_Ens.اندازه();
        UASSERT(بل_Idx <= std::numeric_limits<پیغام_En_سېټ_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_منزل_Str(بل_Idx) + "+).");
        idx = static_cast<پیغام_En_سېټ_Idx_t>(بل_Idx);
        m_interned_پیغام_Ens.ټېل_شا(بټ_سېټ);
    }
    أرجع idx;
}

File_کرښه_Singleton::پیغام_En_سېټ_Idx_t File_کرښه_Singleton::default_پیغام_En_شاخص() VL_MT_خوندي {
    پیغام_En_بټ_سېټ پیغام_En_بټ_سېټ_2;
    لكل (صحيح i = V3_Error_کوډ::EC_MIN; i < V3_Error_کوډ::_ENUM_MAX; ++i) {
        ثابت V3_Error_کوډ کوډ{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        پیغام_En_بټ_سېټ_2.سېټ(پیغام_En_بټ_سېټ::Subset::کوډ_2, کوډ, !کوډ.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        پیغام_En_بټ_سېټ_2.سېټ(پیغام_En_بټ_سېټ::Subset::CTRL, کوډ, ریښتیا);
    }
    أرجع جمع_کړه_پیغام_En_بټ_سېټ(پیغام_En_بټ_سېټ_2);
}

File_کرښه_Singleton::پیغام_En_سېټ_Idx_t File_کرښه_Singleton::پیغام_En_سېټ_بټ(پیغام_En_سېټ_Idx_t سېټ_Idx,
                                                                پیغام_En_بټ_سېټ::Subset subset,
                                                                V3_Error_کوډ کوډ, منطقي ارزښت) {
    // See if state matches existing
    منطقي same = ریښتیا;
    کوډ.forDelegateCodes([&](V3_Error_کوډ subcode) {
        إذا (پیغام_En(سېټ_Idx).test(subset, subcode) != ارزښت) same = خطأ;
    });
    إذا (same) أرجع سېټ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیغام_En_بټ_سېټ پیغام_En_بټ_سېټ_2{پیغام_En(سېټ_Idx)};
    کوډ.forDelegateCodes([&](V3_Error_کوډ subcode) { پیغام_En_بټ_سېټ_2.سېټ(subset, subcode, ارزښت); });
    أرجع جمع_کړه_پیغام_En_بټ_سېټ(پیغام_En_بټ_سېټ_2);
}

File_کرښه_Singleton::پیغام_En_سېټ_Idx_t File_کرښه_Singleton::پیغام_سېټ_Ctrl_بټ_سېټ(پیغام_En_سېټ_Idx_t سېټ_Idx,
                                                                     ثابت VError_بټ_سېټ& bitset) {
    ثابت پیغام_En_بټ_سېټ::Subset subset = پیغام_En_بټ_سېټ::Subset::CTRL;
    // See if state matches existing
    إذا (پیغام_En(سېټ_Idx).getAll(subset) == bitset) أرجع سېټ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیغام_En_بټ_سېټ پیغام_En_بټ_سېټ_2{پیغام_En(سېټ_Idx)};
    پیغام_En_بټ_سېټ_2.سېټ_All(subset, bitset);
    أرجع جمع_کړه_پیغام_En_بټ_سېټ(پیغام_En_بټ_سېټ_2);
}

File_کرښه_Singleton::پیغام_En_سېټ_Idx_t File_کرښه_Singleton::پیغام_En_And(پیغام_En_سېټ_Idx_t lhsIdx,
                                                             پیغام_En_سېټ_Idx_t rhsIdx) {
    ثابت پیغام_En_بټ_سېټ& lhs = پیغام_En(lhsIdx);
    ثابت پیغام_En_بټ_سېټ& rhs = پیغام_En(rhsIdx);
    ثابت پیغام_En_بټ_سېټ intersection{lhs, rhs};
    إذا (intersection == lhs) أرجع lhsIdx;
    إذا (intersection == rhs) أرجع rhsIdx;
    أرجع جمع_کړه_پیغام_En_بټ_سېټ(intersection);
}

// ######################################################################
//  VFileContents class functions

فراغ VFileContent::ټېل_متن(ثابت string& متن) {
    // Similar code in WildcardContents::pushText()
    إذا (m_کرښې.اندازه() == 0) {
        m_کرښې.emplace_شا("");  // no such thing as line [0]
        m_کرښې.emplace_شا("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_کرښې.شا() + متن;
    m_کرښې.راوباسه_شا();

    // Insert line-by-line
    string::اندازه_type کرښه_پیل = 0;
    طالما (ریښتیا) {
        ثابت string::اندازه_type کرښه_پای_ټکی = leftover.پیدا_کړه('\n', کرښه_پیل);
        إذا (کرښه_پای_ټکی != string::npos) {
            ثابت string oneline(leftover, کرښه_پیل, کرښه_پای_ټکی - کرښه_پیل + 1);
            m_کرښې.ټېل_شا(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_کرښې.اندازه() - 1) << "]: " << oneline);
            کرښه_پیل = کرښه_پای_ټکی + 1;
        } وإلا {
            اكسر;
        }
    }
    // Keep leftover for next time
    m_کرښې.emplace_شا(string(leftover, کرښه_پیل));  // Might be ""
}

string VFileContent::get_کرښه(صحيح lineno) ثابت VL_MT_خوندي {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    إذا (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحيح)m_کرښې.اندازه())) {
        إذا (debug() || v3Global.opt.debugCheck()) {
            أرجع ("%Error-internal-contents-bad-ct" + cvt_منزل_Str(m_id) + "-ln" + cvt_منزل_Str(lineno));
        } وإلا {
            أرجع "";
        }
    }
    string متن = m_کرښې[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << متن);
    أرجع متن;
}

std::ostream& چلوونکی<<(std::ostream& os, VFileContent* contentp) {
    إذا (!contentp) {
        os << "ct0";
    } وإلا {
        os << contentp->ascii();
    }
    أرجع os;
}

// ######################################################################
//  FileLine class functions

File_کرښه::~File_کرښه() {
    إذا (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

فراغ File_کرښه::newContent() {
    إذا (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = جديد VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_کرښه::کرښه_Directive_Strg(صحيح enterExit) ثابت {
    أرجع "`line "s + cvt_منزل_Str(وروستی_Lineno()) + " \""
           + V3_بهر_Formatter::quoteNameControls(filename()) + "\" " + cvt_منزل_Str(enterExit) + '\n';
}

فراغ File_کرښه::کرښه_Directive(ثابت حرف* textp, صحيح& enterExitRef) {
    string newFilename;
    صحيح newLineno = -1;
    کرښه_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    إذا (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } وإلا {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

فراغ File_کرښه::کرښه_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                                  صحيح& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    افعل {
        // Skip `line
        طالما (*textp && std::که_تشه(*textp)) ++textp;
        طالما (*textp && !std::که_تشه(*textp)) ++textp;
        طالما (*textp && std::که_تشه(*textp)) ++textp;

        // Grab linenumber
        صحيح کرښه_No;
        ثابت حرف* ثابت ln = textp;
        طالما (*textp && !std::که_تشه(*textp)) ++textp;
        إذا (0 == مزي_پرتله(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            کرښه_No = lineno() + 1;
        } وإلا إذا (std::که_عدد(*ln)) {
            کرښه_No = std::atoi(ln);
        } وإلا {
            اكسر;  // Fail
        }
        طالما (*textp && (std::که_تشه(*textp))) ++textp;

        // Grab filename
        إذا (*textp != '"') اكسر;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        طالما (*textp && *textp != '"') ++textp;
        إذا (*textp != '"') اكسر;  // Fail
        string err_پیغام;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_پیغام);
        إذا (!err_پیغام.تشه()) هذا->v3error(err_پیغام.c_str());
        ++textp;
        طالما (*textp && std::که_تشه(*textp)) ++textp;

        // Grab level
        إذا (!std::که_عدد(*textp)) اكسر;  // Fail
        ثابت صحيح level = std::atoi(textp);
        إذا (level < 0 || level >= 3) اكسر;  // Fail

        linenoRef = کرښه_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        أرجع;
    } طالما (خطأ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

فراغ File_کرښه::forwardToken(ثابت حرف* textp, اندازه_t اندازه, منطقي track_کرښې) {
    ساكن صحيح s_token_عدد = 1;
    m_token_عدد = s_token_عدد++;
    لكل (ثابت حرف* sp = textp; اندازه && *sp; ++sp, --اندازه) {
        إذا (*sp == '\n') {
            إذا (track_کرښې) linenoInc();
            m_وروستی_ستنه = 1;
        } وإلا إذا (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } وإلا {  // Tabs are considered one column; hence column means number of chars
            ++m_وروستی_ستنه;
        }
    }
}

فراغ File_کرښه::applyIgnores() {
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(هذا);  // Toggle warnings based on global config file
#د_که_پای
}

File_کرښه* File_کرښه::copy_Or_Same_File_کرښه_Applied() {
    applyIgnores();
    أرجع copy_Or_Same_File_کرښه();
}

File_کرښه* File_کرښه::copy_Or_Same_File_کرښه() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ساكن File_کرښه* s_وروستی_Newp = تش_اشاره;
    إذا (s_وروستی_Newp && *s_وروستی_Newp == *هذا) {  // Compares lineno, filename, etc
        أرجع s_وروستی_Newp;
    }
    File_کرښه* ثابت newp = جديد File_کرښه{هذا};
    s_وروستی_Newp = newp;
    أرجع newp;
}

string File_کرښه::filebasename() ثابت VL_MT_خوندي { أرجع V3Os::filenameNonDir(filename()); }

string File_کرښه::filebasenameNoExt() ثابت { أرجع V3Os::filenameNonDirExt(filename()); }

string File_کرښه::لومړی_ستنه_Letters() ثابت VL_MT_خوندي {
    ثابت حرف a = ((لومړی_ستنه() / 26) % 26) + 'a';
    ثابت حرف b = (لومړی_ستنه() % 26) + 'a';
    أرجع string(1, a) + string(1, b);
}

string File_کرښه::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::اندازه_type pos;
    طالما ((pos = name.پیدا_کړه_لومړی_not_د(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_منزل_Str(وروستی_Lineno());
    أرجع name;
}

string File_کرښه::ascii_کرښه_ستون() ثابت {
    أرجع (cvt_منزل_Str(لومړی_Lineno()) + "-" + cvt_منزل_Str(وروستی_Lineno()) + ":" + cvt_منزل_Str(لومړی_ستنه())
            + "-" + cvt_منزل_Str(وروستی_ستنه()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_منزل_Str(m_contentLineno) + "]");
}
string File_کرښه::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    أرجع filename() + ":" + cvt_منزل_Str(وروستی_Lineno()) + ":" + cvt_منزل_Str(لومړی_ستنه());
}
std::ostream& چلوونکی<<(std::ostream& os, File_کرښه* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    أرجع (os);
}

string File_کرښه::warnOffParse(ثابت string& msgs, منطقي turnOff) {
    string پایله;
    لكل (ثابت string& پیغام_2 : VString::وېشل(msgs, ',')) {
        ثابت V3_Error_کوډ کوډ{پیغام_2};
        إذا (!کوډ.hardError()) {
            warnOff(کوډ, turnOff);
            استمر;
        }
        // Error if not suppressed
        إذا (!v3Global.opt.isFuture(پیغام_2)) پایله = VString::dot(پایله, ",", پیغام_2);
    }
    أرجع پایله;
}

منطقي File_کرښه::warnIsOff(V3_Error_کوډ کوډ) ثابت {
    إذا (!پیغام_En().enabled(کوډ)) أرجع ریښتیا;
    إذا (!default_File_کرښه().پیغام_En().enabled(کوډ)) أرجع ریښتیا;  // Global overrides local
    أرجع خطأ;
}

// cppverilator-suppress constParameter
فراغ File_کرښه::v3error_پای_ټکی(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    إذا (وروستی_Lineno()) nsstr << V3Error::warnContextBegin() << هذا << V3Error::warn_Context_پای_ټکی();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    إذا (!extra.تشه()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().اوږدتیا()) << " " << ": "
                << V3Error::warn_Context_پای_ټکی() << extra;
    }
    إذا (warnIsOff(V3Error::s().error_کوډ())) {
        V3Error::s().suppressThisWarning();
    } وإلا {
        إذا (!V3Error::s().errorContexted()) {
            ثابت string جمع_کړه = warnContextPrimary();
            wsstr << جمع_کړه;
            nsstr << جمع_کړه;
        }
        ثابت string waiver_متن = V3Error::strip_Meta_متن(wsstr.str(), خطأ);
        m_waive = V3Control::waive(هذا, V3Error::s().error_کوډ(), waiver_متن);
        إذا (m_waive) {
            V3Error::s().suppressThisWarning();
        } وإلا {
            V3Waiver::جمع_کړه_ثبت(V3Error::s().error_کوډ(), filename(), waiver_متن);
        }
    }
    V3Error::v3error_پای_ټکی(nsstr, extrass.str(), هذا);
}

string File_کرښه::warn_نور() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (وروستی_Lineno()) {
        أرجع V3Error::warnContextBegin() + V3Error::warn_نور() + string(ascii().اندازه(), ' ')
               + ": " + V3Error::warn_Context_پای_ټکی();
    } وإلا {
        أرجع V3Error::warn_نور();
    }
}
string File_کرښه::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (وروستی_Lineno()) {
        أرجع V3Error::s().warnRelated(هذا) + V3Error::warnContextBegin() + V3Error::warn_نور()
               + ascii() + ": " + V3Error::warn_Context_پای_ټکی();
    } وإلا {
        أرجع V3Error::warn_نور();
    }
};

string File_کرښه::سرچینه() ثابت VL_MT_خوندي {
    إذا (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        إذا (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            أرجع "\n%Error: internal tracking of file contents failed";
        } وإلا {
            أرجع "";
        }
    }  // LCOV_EXCL_STOP
    أرجع m_contentp->get_کرښه(m_contentLineno);
}
string File_کرښه::سرچینه_مختاړی(صحيح منزل_ستنه) ثابت VL_MT_خوندي {
    ثابت std::string src = سرچینه();
    إذا (منزل_ستنه > static_cast<صحيح>(src.اوږدتیا())) منزل_ستنه = static_cast<صحيح>(src.اوږدتیا());
    إذا (منزل_ستنه < 1) أرجع "";
    أرجع src.substr(0, منزل_ستنه - 1);
}
string File_کرښه::pretty_سرچینه() ثابت VL_MT_خوندي {
    string بهر = سرچینه();
    // Drop ignore trailing newline
    ثابت string::اندازه_type pos = بهر.پیدا_کړه('\n');
    إذا (pos != string::npos) بهر = string(بهر, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    أرجع VString::spaceUnprintable(بهر);
}

string File_کرښه::warnContext() ثابت {
    إذا (!v3Global.opt.context()) أرجع "";
    إذا (لومړی_Lineno() == وروستی_Lineno() && لومړی_ستنه()) {
        ثابت string سرچینه_کرښه = pretty_سرچینه();
        string بهر;
        // Don't show super-long lines as can fill screen and unlikely to help user
        إذا (!سرچینه_کرښه.تشه() && سرچینه_کرښه.اوږدتیا() < SHOW_سرچینه_MAX_اوږدتیا
            && سرچینه_کرښه.اوږدتیا() >= static_cast<اندازه_t>(وروستی_ستنه() - 1)) {
            string linestr = cvt_منزل_Str(لومړی_Lineno());
            طالما (linestr.اندازه() < 5) linestr = ' ' + linestr;
            بهر += linestr + " | " + سرچینه_کرښه + '\n';
            بهر += std::string(linestr.اندازه(), ' ') + " | ";
            بهر += string((لومړی_ستنه() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            إذا (وروستی_ستنه() > لومړی_ستنه()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                بهر += string((وروستی_ستنه() - لومړی_ستنه() - 1), '~');
            }
            بهر += '\n';
        }
        أرجع V3Error::warnContextBegin() + بهر + V3Error::warn_Context_پای_ټکی();
    }
    أرجع "";
}

string File_کرښه::warn_Context_پلار() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string پایله;
    لكل (File_کرښه* پلار_Fl = پلار(); پلار_Fl; پلار_Fl = پلار_Fl->پلار()) {
        إذا (پلار_Fl->filenameIsGlobal()) اكسر;
        پایله += پلار_Fl->warnOther() + "... note: In file included from '"
                  + پلار_Fl->filebasename() + "'\n";
    }
    أرجع پایله;
}

#که_تعریف_شوی VL_LEAK_CHECKS
std::unordered_سېټ<File_کرښه*> file_کرښه_Leak_Checks;

فراغ* File_کرښه::چلوونکی جديد(اندازه_t اندازه) {
    File_کرښه* ثابت objp = static_cast<File_کرښه*>(::چلوونکی جديد(اندازه));
    file_کرښه_Leak_Checks.ورننباسه(objp);
    أرجع objp;
}

فراغ File_کرښه::چلوونکی ړنګول(فراغ* objp, اندازه_t اندازه) {
    إذا (!objp) أرجع;
    File_کرښه* ثابت flp = static_cast<File_کرښه*>(objp);
    ثابت خپلکاری it = file_کرښه_Leak_Checks.پیدا_کړه(flp);
    إذا (it != file_کرښه_Leak_Checks.پای_ټکی()) {
        file_کرښه_Leak_Checks.erase(it);
    } وإلا {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::چلوونکی ړنګول(objp);
}
#د_که_پای

فراغ File_کرښه::stats() {
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
    V3Stats::جمع_کړه_Stat_مجموعه("FileLines, Number of filenames",
                        singleton().m_نومونه.اندازه());  // Max m_filenameno
    V3Stats::جمع_کړه_Stat_مجموعه("FileLines, Message enable sets",
                        singleton().m_interned_پیغام_Ens.اندازه());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#د_که_پای
}

فراغ File_کرښه::delete_All_پاتې_شوی() {
#که_تعریف_شوی VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    طالما (ریښتیا) {
        ثابت خپلکاری it = file_کرښه_Leak_Checks.begin();
        إذا (it == file_کرښه_Leak_Checks.پای_ټکی()) اكسر;
        ړنګول *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_کرښه_Leak_Checks.clear();
    singleton().clear();
#د_که_پای
}
