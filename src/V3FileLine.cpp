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
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#جے_انت
#ضمن "V3Waiver.h"
// clang-format on

#ضمن <algorithm>
#ضمن <iomanip>
#ضمن <limits>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_سطر_Singleton::filenameLetters(fileNameIdx_t fileno) VL_خالص {
    constexpr صحيح ناپ
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    حرف باہرلا[ناپ];
    حرف* op = باہرلا + ناپ - 1;
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
File_سطر_Singleton::fileNameIdx_t File_سطر_Singleton::name_منزل_نمبر(ثابت string& filename) {
    ثابت خودکار pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.پہلا->دوجا;
    إذا (pair.دوجا) {
        ثابت ناپ_t اگلا_Idx = m_ناں.ناپ();
        UASSERT(اگلا_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_منزل_Str(اگلا_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(اگلا_Idx);
        m_ناں.ڈالو_پٹھ(filename);
        m_languages.ڈالو_پٹھ(V3_Lang_کوڈ::mostRecent());
    }
    أرجع idx;
}

فراغ File_سطر_Singleton::file_Name_عدد_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    لكل (ثابت خودکار& itr : m_namemap) {
        std::string filename = itr.پہلا;
        إذا (filename == V3Options::get_Std_Package_رستہ()) {
            filename = "<verilated_std>";
        } وإلا إذا (filename == V3Options::get_Std_Waiver_رستہ()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.دوجا) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_باہرلا_Formatter::quoteNameControls(V3Os::filename_Real_رستہ(itr.پہلا)) << '"'
           << ", \"language\":\"" << نمبر_منزل_Lang(itr.دوجا).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& ثنائی_ہندسہ_سیٹ)
    VL_MT_محفوظ_EXCLUDES(m_mutex) {
    ثابت V3_جندرا_Guard جندرا_2{m_mutex};
    ثابت خودکار pair = m_interned_پیغام_En_Idxs.emplace(ثنائی_ہندسہ_سیٹ, 0);
    پیغام_En_سیٹ_Idx_t& idx = pair.پہلا->دوجا;
    إذا (pair.دوجا) {
        ثابت ناپ_t اگلا_Idx = m_interned_پیغام_Ens.ناپ();
        UASSERT(اگلا_Idx <= std::numeric_limits<پیغام_En_سیٹ_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_منزل_Str(اگلا_Idx) + "+).");
        idx = static_cast<پیغام_En_سیٹ_Idx_t>(اگلا_Idx);
        m_interned_پیغام_Ens.ڈالو_پٹھ(ثنائی_ہندسہ_سیٹ);
    }
    أرجع idx;
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::default_پیغام_En_اشاریہ() VL_MT_محفوظ {
    پیغام_En_ثنائی_ہندسہ_سیٹ پیغام_En_ثنائی_ہندسہ_سیٹ_2;
    لكل (صحيح i = V3_Error_کوڈ::EC_MIN; i < V3_Error_کوڈ::_ENUM_MAX; ++i) {
        ثابت V3_Error_کوڈ کوڈ{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        پیغام_En_ثنائی_ہندسہ_سیٹ_2.سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ::Subset::کوڈ_2, کوڈ, !کوڈ.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        پیغام_En_ثنائی_ہندسہ_سیٹ_2.سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ::Subset::CTRL, کوڈ, سچ);
    }
    أرجع جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ_2);
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::پیغام_En_سیٹ_ثنائی_ہندسہ(پیغام_En_سیٹ_Idx_t سیٹ_Idx,
                                                                پیغام_En_ثنائی_ہندسہ_سیٹ::Subset subset,
                                                                V3_Error_کوڈ کوڈ, منطقي قدر) {
    // See if state matches existing
    منطقي same = سچ;
    کوڈ.forDelegateCodes([&](V3_Error_کوڈ subcode) {
        إذا (پیغام_En(سیٹ_Idx).test(subset, subcode) != قدر) same = خطأ;
    });
    إذا (same) أرجع سیٹ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیغام_En_ثنائی_ہندسہ_سیٹ پیغام_En_ثنائی_ہندسہ_سیٹ_2{پیغام_En(سیٹ_Idx)};
    کوڈ.forDelegateCodes([&](V3_Error_کوڈ subcode) { پیغام_En_ثنائی_ہندسہ_سیٹ_2.سیٹ(subset, subcode, قدر); });
    أرجع جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ_2);
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::پیغام_سیٹ_Ctrl_ثنائی_ہندسہ_سیٹ(پیغام_En_سیٹ_Idx_t سیٹ_Idx,
                                                                     ثابت VError_ثنائی_ہندسہ_سیٹ& bitset) {
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ::Subset subset = پیغام_En_ثنائی_ہندسہ_سیٹ::Subset::CTRL;
    // See if state matches existing
    إذا (پیغام_En(سیٹ_Idx).getAll(subset) == bitset) أرجع سیٹ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیغام_En_ثنائی_ہندسہ_سیٹ پیغام_En_ثنائی_ہندسہ_سیٹ_2{پیغام_En(سیٹ_Idx)};
    پیغام_En_ثنائی_ہندسہ_سیٹ_2.سیٹ_All(subset, bitset);
    أرجع جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ_2);
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::پیغام_En_And(پیغام_En_سیٹ_Idx_t lhsIdx,
                                                             پیغام_En_سیٹ_Idx_t rhsIdx) {
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& lhs = پیغام_En(lhsIdx);
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& rhs = پیغام_En(rhsIdx);
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ intersection{lhs, rhs};
    إذا (intersection == lhs) أرجع lhsIdx;
    إذا (intersection == rhs) أرجع rhsIdx;
    أرجع جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(intersection);
}

// ######################################################################
//  VFileContents class functions

فراغ VFileContent::ڈالو_متن(ثابت string& متن) {
    // Similar code in WildcardContents::pushText()
    إذا (m_سطریں.ناپ() == 0) {
        m_سطریں.emplace_پٹھ("");  // no such thing as line [0]
        m_سطریں.emplace_پٹھ("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_سطریں.پٹھ() + متن;
    m_سطریں.نکالو_پٹھ();

    // Insert line-by-line
    string::ناپ_type سطر_شروع = 0;
    طالما (سچ) {
        ثابت string::ناپ_type سطر_سرا = leftover.لبھو('\n', سطر_شروع);
        إذا (سطر_سرا != string::npos) {
            ثابت string oneline(leftover, سطر_شروع, سطر_سرا - سطر_شروع + 1);
            m_سطریں.ڈالو_پٹھ(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_سطریں.ناپ() - 1) << "]: " << oneline);
            سطر_شروع = سطر_سرا + 1;
        } وإلا {
            اكسر;
        }
    }
    // Keep leftover for next time
    m_سطریں.emplace_پٹھ(string(leftover, سطر_شروع));  // Might be ""
}

string VFileContent::get_سطر(صحيح lineno) ثابت VL_MT_محفوظ {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    إذا (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحيح)m_سطریں.ناپ())) {
        إذا (debug() || v3Global.opt.debugCheck()) {
            أرجع ("%Error-internal-contents-bad-ct" + cvt_منزل_Str(m_id) + "-ln" + cvt_منزل_Str(lineno));
        } وإلا {
            أرجع "";
        }
    }
    string متن = m_سطریں[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << متن);
    أرجع متن;
}

std::ostream& عامل<<(std::ostream& os, VFileContent* contentp) {
    إذا (!contentp) {
        os << "ct0";
    } وإلا {
        os << contentp->ascii();
    }
    أرجع os;
}

// ######################################################################
//  FileLine class functions

File_سطر::~File_سطر() {
    إذا (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

فراغ File_سطر::newContent() {
    إذا (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = جديد VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_سطر::سطر_Directive_Strg(صحيح enterExit) ثابت {
    أرجع "`line "s + cvt_منزل_Str(آخری_Lineno()) + " \""
           + V3_باہرلا_Formatter::quoteNameControls(filename()) + "\" " + cvt_منزل_Str(enterExit) + '\n';
}

فراغ File_سطر::سطر_Directive(ثابت حرف* textp, صحيح& enterExitRef) {
    string newFilename;
    صحيح newLineno = -1;
    سطر_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    إذا (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } وإلا {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

فراغ File_سطر::سطر_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                                  صحيح& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    افعل {
        // Skip `line
        طالما (*textp && std::خلا_اے(*textp)) ++textp;
        طالما (*textp && !std::خلا_اے(*textp)) ++textp;
        طالما (*textp && std::خلا_اے(*textp)) ++textp;

        // Grab linenumber
        صحيح سطر_No;
        ثابت حرف* ثابت ln = textp;
        طالما (*textp && !std::خلا_اے(*textp)) ++textp;
        إذا (0 == لڑی_مقابلہ(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            سطر_No = lineno() + 1;
        } وإلا إذا (std::ہندسہ_اے(*ln)) {
            سطر_No = std::atoi(ln);
        } وإلا {
            اكسر;  // Fail
        }
        طالما (*textp && (std::خلا_اے(*textp))) ++textp;

        // Grab filename
        إذا (*textp != '"') اكسر;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        طالما (*textp && *textp != '"') ++textp;
        إذا (*textp != '"') اكسر;  // Fail
        string err_پیغام;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_پیغام);
        إذا (!err_پیغام.خالی_تھاواں()) هذا->v3error(err_پیغام.c_str());
        ++textp;
        طالما (*textp && std::خلا_اے(*textp)) ++textp;

        // Grab level
        إذا (!std::ہندسہ_اے(*textp)) اكسر;  // Fail
        ثابت صحيح level = std::atoi(textp);
        إذا (level < 0 || level >= 3) اكسر;  // Fail

        linenoRef = سطر_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        أرجع;
    } طالما (خطأ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

فراغ File_سطر::forwardToken(ثابت حرف* textp, ناپ_t ناپ, منطقي track_سطریں) {
    ساكن صحيح s_token_عدد = 1;
    m_token_عدد = s_token_عدد++;
    لكل (ثابت حرف* sp = textp; ناپ && *sp; ++sp, --ناپ) {
        إذا (*sp == '\n') {
            إذا (track_سطریں) linenoInc();
            m_آخری_کالم = 1;
        } وإلا إذا (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } وإلا {  // Tabs are considered one column; hence column means number of chars
            ++m_آخری_کالم;
        }
    }
}

فراغ File_سطر::applyIgnores() {
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(هذا);  // Toggle warnings based on global config file
#جے_انت
}

File_سطر* File_سطر::copy_Or_Same_File_سطر_Applied() {
    applyIgnores();
    أرجع copy_Or_Same_File_سطر();
}

File_سطر* File_سطر::copy_Or_Same_File_سطر() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ساكن File_سطر* s_آخری_Newp = خالی_سوچک;
    إذا (s_آخری_Newp && *s_آخری_Newp == *هذا) {  // Compares lineno, filename, etc
        أرجع s_آخری_Newp;
    }
    File_سطر* ثابت newp = جديد File_سطر{هذا};
    s_آخری_Newp = newp;
    أرجع newp;
}

string File_سطر::filebasename() ثابت VL_MT_محفوظ { أرجع V3Os::filenameNonDir(filename()); }

string File_سطر::filebasenameNoExt() ثابت { أرجع V3Os::filenameNonDirExt(filename()); }

string File_سطر::پہلا_کالم_Letters() ثابت VL_MT_محفوظ {
    ثابت حرف a = ((پہلا_کالم() / 26) % 26) + 'a';
    ثابت حرف b = (پہلا_کالم() % 26) + 'a';
    أرجع string(1, a) + string(1, b);
}

string File_سطر::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::ناپ_type pos;
    طالما ((pos = name.لبھو_پہلا_not_دا(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_منزل_Str(آخری_Lineno());
    أرجع name;
}

string File_سطر::ascii_سطر_خانہ() ثابت {
    أرجع (cvt_منزل_Str(پہلا_Lineno()) + "-" + cvt_منزل_Str(آخری_Lineno()) + ":" + cvt_منزل_Str(پہلا_کالم())
            + "-" + cvt_منزل_Str(آخری_کالم()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_منزل_Str(m_contentLineno) + "]");
}
string File_سطر::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    أرجع filename() + ":" + cvt_منزل_Str(آخری_Lineno()) + ":" + cvt_منزل_Str(پہلا_کالم());
}
std::ostream& عامل<<(std::ostream& os, File_سطر* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    أرجع (os);
}

string File_سطر::warnOffParse(ثابت string& msgs, منطقي turnOff) {
    string نتیجہ;
    لكل (ثابت string& پیغام : VString::ونڈو(msgs, ',')) {
        ثابت V3_Error_کوڈ کوڈ{پیغام};
        إذا (!کوڈ.hardError()) {
            warnOff(کوڈ, turnOff);
            استمر;
        }
        // Error if not suppressed
        إذا (!v3Global.opt.isFuture(پیغام)) نتیجہ = VString::dot(نتیجہ, ",", پیغام);
    }
    أرجع نتیجہ;
}

منطقي File_سطر::warnIsOff(V3_Error_کوڈ کوڈ) ثابت {
    إذا (!پیغام_En().enabled(کوڈ)) أرجع سچ;
    إذا (!default_File_سطر().پیغام_En().enabled(کوڈ)) أرجع سچ;  // Global overrides local
    أرجع خطأ;
}

// cppverilator-suppress constParameter
فراغ File_سطر::v3error_سرا(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    إذا (آخری_Lineno()) nsstr << V3Error::warnContextBegin() << هذا << V3Error::warn_Context_سرا();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    إذا (!extra.خالی_تھاواں()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().لمائی()) << " " << ": "
                << V3Error::warn_Context_سرا() << extra;
    }
    إذا (warnIsOff(V3Error::s().error_کوڈ())) {
        V3Error::s().suppressThisWarning();
    } وإلا {
        إذا (!V3Error::s().errorContexted()) {
            ثابت string جوڑو = warnContextPrimary();
            wsstr << جوڑو;
            nsstr << جوڑو;
        }
        ثابت string waiver_متن = V3Error::strip_Meta_متن(wsstr.str(), خطأ);
        m_waive = V3Control::waive(هذا, V3Error::s().error_کوڈ(), waiver_متن);
        إذا (m_waive) {
            V3Error::s().suppressThisWarning();
        } وإلا {
            V3Waiver::جوڑو_اندراج(V3Error::s().error_کوڈ(), filename(), waiver_متن);
        }
    }
    V3Error::v3error_سرا(nsstr, extrass.str(), هذا);
}

string File_سطر::warn_ہور() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (آخری_Lineno()) {
        أرجع V3Error::warnContextBegin() + V3Error::warn_ہور() + string(ascii().ناپ(), ' ')
               + ": " + V3Error::warn_Context_سرا();
    } وإلا {
        أرجع V3Error::warn_ہور();
    }
}
string File_سطر::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (آخری_Lineno()) {
        أرجع V3Error::s().warnRelated(هذا) + V3Error::warnContextBegin() + V3Error::warn_ہور()
               + ascii() + ": " + V3Error::warn_Context_سرا();
    } وإلا {
        أرجع V3Error::warn_ہور();
    }
};

string File_سطر::سروت() ثابت VL_MT_محفوظ {
    إذا (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        إذا (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            أرجع "\n%Error: internal tracking of file contents failed";
        } وإلا {
            أرجع "";
        }
    }  // LCOV_EXCL_STOP
    أرجع m_contentp->get_سطر(m_contentLineno);
}
string File_سطر::سروت_اگیتر(صحيح منزل_کالم) ثابت VL_MT_محفوظ {
    ثابت std::string src = سروت();
    إذا (منزل_کالم > static_cast<صحيح>(src.لمائی())) منزل_کالم = static_cast<صحيح>(src.لمائی());
    إذا (منزل_کالم < 1) أرجع "";
    أرجع src.substr(0, منزل_کالم - 1);
}
string File_سطر::pretty_سروت() ثابت VL_MT_محفوظ {
    string باہرلا = سروت();
    // Drop ignore trailing newline
    ثابت string::ناپ_type pos = باہرلا.لبھو('\n');
    إذا (pos != string::npos) باہرلا = string(باہرلا, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    أرجع VString::spaceUnprintable(باہرلا);
}

string File_سطر::warnContext() ثابت {
    إذا (!v3Global.opt.context()) أرجع "";
    إذا (پہلا_Lineno() == آخری_Lineno() && پہلا_کالم()) {
        ثابت string سروت_سطر = pretty_سروت();
        string باہرلا;
        // Don't show super-long lines as can fill screen and unlikely to help user
        إذا (!سروت_سطر.خالی_تھاواں() && سروت_سطر.لمائی() < SHOW_سروت_MAX_لمائی
            && سروت_سطر.لمائی() >= static_cast<ناپ_t>(آخری_کالم() - 1)) {
            string linestr = cvt_منزل_Str(پہلا_Lineno());
            طالما (linestr.ناپ() < 5) linestr = ' ' + linestr;
            باہرلا += linestr + " | " + سروت_سطر + '\n';
            باہرلا += std::string(linestr.ناپ(), ' ') + " | ";
            باہرلا += string((پہلا_کالم() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            إذا (آخری_کالم() > پہلا_کالم()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                باہرلا += string((آخری_کالم() - پہلا_کالم() - 1), '~');
            }
            باہرلا += '\n';
        }
        أرجع V3Error::warnContextBegin() + باہرلا + V3Error::warn_Context_سرا();
    }
    أرجع "";
}

string File_سطر::warn_Context_ماپا() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string نتیجہ;
    لكل (File_سطر* ماپا_Fl = ماپا(); ماپا_Fl; ماپا_Fl = ماپا_Fl->ماپا()) {
        إذا (ماپا_Fl->filenameIsGlobal()) اكسر;
        نتیجہ += ماپا_Fl->warnOther() + "... note: In file included from '"
                  + ماپا_Fl->filebasename() + "'\n";
    }
    أرجع نتیجہ;
}

#جے_تعریف_شدہ VL_LEAK_CHECKS
std::unordered_سیٹ<File_سطر*> file_سطر_Leak_Checks;

فراغ* File_سطر::عامل جديد(ناپ_t ناپ) {
    File_سطر* ثابت objp = static_cast<File_سطر*>(::عامل جديد(ناپ));
    file_سطر_Leak_Checks.داخل_کرو(objp);
    أرجع objp;
}

فراغ File_سطر::عامل مٹاؤ(فراغ* objp, ناپ_t ناپ) {
    إذا (!objp) أرجع;
    File_سطر* ثابت flp = static_cast<File_سطر*>(objp);
    ثابت خودکار it = file_سطر_Leak_Checks.لبھو(flp);
    إذا (it != file_سطر_Leak_Checks.سرا()) {
        file_سطر_Leak_Checks.erase(it);
    } وإلا {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::عامل مٹاؤ(objp);
}
#جے_انت

فراغ File_سطر::stats() {
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
    V3Stats::جوڑو_Stat_جوڑ("FileLines, Number of filenames",
                        singleton().m_ناں.ناپ());  // Max m_filenameno
    V3Stats::جوڑو_Stat_جوڑ("FileLines, Message enable sets",
                        singleton().m_interned_پیغام_Ens.ناپ());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#جے_انت
}

فراغ File_سطر::delete_All_بچیا() {
#جے_تعریف_شدہ VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    طالما (سچ) {
        ثابت خودکار it = file_سطر_Leak_Checks.begin();
        إذا (it == file_سطر_Leak_Checks.سرا()) اكسر;
        مٹاؤ *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_سطر_Leak_Checks.clear();
    singleton().clear();
#جے_انت
}
