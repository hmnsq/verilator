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

#شامل "config_build.h"
#شامل "verilatedos.h"

// clang-format off
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Os.h"
#شامل "V3String.h"
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#اگر_ختم
#شامل "V3Waiver.h"
// clang-format on

#شامل <algorithm>
#شامل <iomanip>
#شامل <limits>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_سطر_Singleton::filenameLetters(fileNameIdx_t fileno) VL_خالص {
    constexpr صحیح سائز
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    حرف برآمد[سائز];
    حرف* op = برآمد + سائز - 1;
    *--op = '\0';  // We build backwards
    صحیح عدد = fileno;
    کرو {
        *--op = 'a' + عدد % 26;
        عدد /= 26;
    } جب (عدد);
    واپس op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_سطر_Singleton::fileNameIdx_t File_سطر_Singleton::name_مقصد_نمبر(ثابت string& filename) {
    ثابت خودکار pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.پہلا->دوسرا;
    اگر (pair.دوسرا) {
        ثابت سائز_t اگلا_Idx = m_نام.سائز();
        UASSERT(اگلا_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_مقصد_Str(اگلا_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(اگلا_Idx);
        m_نام.ڈالو_عقب(filename);
        m_languages.ڈالو_عقب(V3_Lang_رمز::mostRecent());
    }
    واپس idx;
}

خالی File_سطر_Singleton::file_Name_عدد_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    برائے (ثابت خودکار& itr : m_namemap) {
        std::string filename = itr.پہلا;
        اگر (filename == V3Options::get_Std_Package_راستہ()) {
            filename = "<verilated_std>";
        } ورنہ اگر (filename == V3Options::get_Std_Waiver_راستہ()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.دوسرا) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_برآمد_Formatter::quoteNameControls(V3Os::filename_Real_راستہ(itr.پہلا)) << '"'
           << ", \"language\":\"" << نمبر_مقصد_Lang(itr.دوسرا).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& ثنائی_ہندسہ_سیٹ)
    VL_MT_محفوظ_EXCLUDES(m_mutex) {
    ثابت V3_تالا_Guard تالا_2{m_mutex};
    ثابت خودکار pair = m_interned_پیغام_En_Idxs.emplace(ثنائی_ہندسہ_سیٹ, 0);
    پیغام_En_سیٹ_Idx_t& idx = pair.پہلا->دوسرا;
    اگر (pair.دوسرا) {
        ثابت سائز_t اگلا_Idx = m_interned_پیغام_Ens.سائز();
        UASSERT(اگلا_Idx <= std::numeric_limits<پیغام_En_سیٹ_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_مقصد_Str(اگلا_Idx) + "+).");
        idx = static_cast<پیغام_En_سیٹ_Idx_t>(اگلا_Idx);
        m_interned_پیغام_Ens.ڈالو_عقب(ثنائی_ہندسہ_سیٹ);
    }
    واپس idx;
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::default_پیغام_En_اشاریہ() VL_MT_محفوظ {
    پیغام_En_ثنائی_ہندسہ_سیٹ پیغام_En_ثنائی_ہندسہ_سیٹ_2;
    برائے (صحیح i = V3_Error_رمز::EC_MIN; i < V3_Error_رمز::_ENUM_MAX; ++i) {
        ثابت V3_Error_رمز رمز{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        پیغام_En_ثنائی_ہندسہ_سیٹ_2.سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ::Subset::رمز_2, رمز, !رمز.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        پیغام_En_ثنائی_ہندسہ_سیٹ_2.سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ::Subset::CTRL, رمز, سچ);
    }
    واپس جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ_2);
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::پیغام_En_سیٹ_ثنائی_ہندسہ(پیغام_En_سیٹ_Idx_t سیٹ_Idx,
                                                                پیغام_En_ثنائی_ہندسہ_سیٹ::Subset subset,
                                                                V3_Error_رمز رمز, منطقی قدر) {
    // See if state matches existing
    منطقی same = سچ;
    رمز.forDelegateCodes([&](V3_Error_رمز subcode) {
        اگر (پیغام_En(سیٹ_Idx).test(subset, subcode) != قدر) same = جھوٹ;
    });
    اگر (same) واپس سیٹ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیغام_En_ثنائی_ہندسہ_سیٹ پیغام_En_ثنائی_ہندسہ_سیٹ_2{پیغام_En(سیٹ_Idx)};
    رمز.forDelegateCodes([&](V3_Error_رمز subcode) { پیغام_En_ثنائی_ہندسہ_سیٹ_2.سیٹ(subset, subcode, قدر); });
    واپس جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ_2);
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::پیغام_سیٹ_Ctrl_ثنائی_ہندسہ_سیٹ(پیغام_En_سیٹ_Idx_t سیٹ_Idx,
                                                                     ثابت VError_ثنائی_ہندسہ_سیٹ& bitset) {
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ::Subset subset = پیغام_En_ثنائی_ہندسہ_سیٹ::Subset::CTRL;
    // See if state matches existing
    اگر (پیغام_En(سیٹ_Idx).getAll(subset) == bitset) واپس سیٹ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیغام_En_ثنائی_ہندسہ_سیٹ پیغام_En_ثنائی_ہندسہ_سیٹ_2{پیغام_En(سیٹ_Idx)};
    پیغام_En_ثنائی_ہندسہ_سیٹ_2.سیٹ_All(subset, bitset);
    واپس جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ_2);
}

File_سطر_Singleton::پیغام_En_سیٹ_Idx_t File_سطر_Singleton::پیغام_En_And(پیغام_En_سیٹ_Idx_t lhsIdx,
                                                             پیغام_En_سیٹ_Idx_t rhsIdx) {
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& lhs = پیغام_En(lhsIdx);
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& rhs = پیغام_En(rhsIdx);
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ intersection{lhs, rhs};
    اگر (intersection == lhs) واپس lhsIdx;
    اگر (intersection == rhs) واپس rhsIdx;
    واپس جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(intersection);
}

// ######################################################################
//  VFileContents class functions

خالی VFileContent::ڈالو_متن(ثابت string& متن) {
    // Similar code in WildcardContents::pushText()
    اگر (m_سطریں.سائز() == 0) {
        m_سطریں.emplace_عقب("");  // no such thing as line [0]
        m_سطریں.emplace_عقب("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_سطریں.عقب() + متن;
    m_سطریں.نکالو_عقب();

    // Insert line-by-line
    string::سائز_type سطر_آغاز = 0;
    جب (سچ) {
        ثابت string::سائز_type سطر_انتہا = leftover.ڈھونڈو('\n', سطر_آغاز);
        اگر (سطر_انتہا != string::npos) {
            ثابت string oneline(leftover, سطر_آغاز, سطر_انتہا - سطر_آغاز + 1);
            m_سطریں.ڈالو_عقب(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_سطریں.سائز() - 1) << "]: " << oneline);
            سطر_آغاز = سطر_انتہا + 1;
        } ورنہ {
            توڑو;
        }
    }
    // Keep leftover for next time
    m_سطریں.emplace_عقب(string(leftover, سطر_آغاز));  // Might be ""
}

string VFileContent::get_سطر(صحیح lineno) ثابت VL_MT_محفوظ {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    اگر (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحیح)m_سطریں.سائز())) {
        اگر (debug() || v3Global.opt.debugCheck()) {
            واپس ("%Error-internal-contents-bad-ct" + cvt_مقصد_Str(m_id) + "-ln" + cvt_مقصد_Str(lineno));
        } ورنہ {
            واپس "";
        }
    }
    string متن = m_سطریں[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << متن);
    واپس متن;
}

std::ostream& عامل<<(std::ostream& os, VFileContent* contentp) {
    اگر (!contentp) {
        os << "ct0";
    } ورنہ {
        os << contentp->ascii();
    }
    واپس os;
}

// ######################################################################
//  FileLine class functions

File_سطر::~File_سطر() {
    اگر (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

خالی File_سطر::newContent() {
    اگر (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = نیا VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_سطر::سطر_Directive_Strg(صحیح enterExit) ثابت {
    واپس "`line "s + cvt_مقصد_Str(آخری_Lineno()) + " \""
           + V3_برآمد_Formatter::quoteNameControls(filename()) + "\" " + cvt_مقصد_Str(enterExit) + '\n';
}

خالی File_سطر::سطر_Directive(ثابت حرف* textp, صحیح& enterExitRef) {
    string newFilename;
    صحیح newLineno = -1;
    سطر_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    اگر (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } ورنہ {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

خالی File_سطر::سطر_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحیح& linenoRef,
                                  صحیح& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    کرو {
        // Skip `line
        جب (*textp && std::خلا_ہے(*textp)) ++textp;
        جب (*textp && !std::خلا_ہے(*textp)) ++textp;
        جب (*textp && std::خلا_ہے(*textp)) ++textp;

        // Grab linenumber
        صحیح سطر_No;
        ثابت حرف* ثابت ln = textp;
        جب (*textp && !std::خلا_ہے(*textp)) ++textp;
        اگر (0 == لڑی_موازنہ(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            سطر_No = lineno() + 1;
        } ورنہ اگر (std::ہندسہ_ہے(*ln)) {
            سطر_No = std::atoi(ln);
        } ورنہ {
            توڑو;  // Fail
        }
        جب (*textp && (std::خلا_ہے(*textp))) ++textp;

        // Grab filename
        اگر (*textp != '"') توڑو;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        جب (*textp && *textp != '"') ++textp;
        اگر (*textp != '"') توڑو;  // Fail
        string err_پیغام;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_پیغام);
        اگر (!err_پیغام.خالی_خانے()) یہ->v3error(err_پیغام.c_str());
        ++textp;
        جب (*textp && std::خلا_ہے(*textp)) ++textp;

        // Grab level
        اگر (!std::ہندسہ_ہے(*textp)) توڑو;  // Fail
        ثابت صحیح level = std::atoi(textp);
        اگر (level < 0 || level >= 3) توڑو;  // Fail

        linenoRef = سطر_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        واپس;
    } جب (جھوٹ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

خالی File_سطر::forwardToken(ثابت حرف* textp, سائز_t سائز, منطقی track_سطریں) {
    ساکن صحیح s_token_عدد = 1;
    m_token_عدد = s_token_عدد++;
    برائے (ثابت حرف* sp = textp; سائز && *sp; ++sp, --سائز) {
        اگر (*sp == '\n') {
            اگر (track_سطریں) linenoInc();
            m_آخری_عمود = 1;
        } ورنہ اگر (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } ورنہ {  // Tabs are considered one column; hence column means number of chars
            ++m_آخری_عمود;
        }
    }
}

خالی File_سطر::applyIgnores() {
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(یہ);  // Toggle warnings based on global config file
#اگر_ختم
}

File_سطر* File_سطر::copy_Or_Same_File_سطر_Applied() {
    applyIgnores();
    واپس copy_Or_Same_File_سطر();
}

File_سطر* File_سطر::copy_Or_Same_File_سطر() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ساکن File_سطر* s_آخری_Newp = خالی_اشاریہ;
    اگر (s_آخری_Newp && *s_آخری_Newp == *یہ) {  // Compares lineno, filename, etc
        واپس s_آخری_Newp;
    }
    File_سطر* ثابت newp = نیا File_سطر{یہ};
    s_آخری_Newp = newp;
    واپس newp;
}

string File_سطر::filebasename() ثابت VL_MT_محفوظ { واپس V3Os::filenameNonDir(filename()); }

string File_سطر::filebasenameNoExt() ثابت { واپس V3Os::filenameNonDirExt(filename()); }

string File_سطر::پہلا_عمود_Letters() ثابت VL_MT_محفوظ {
    ثابت حرف a = ((پہلا_عمود() / 26) % 26) + 'a';
    ثابت حرف b = (پہلا_عمود() % 26) + 'a';
    واپس string(1, a) + string(1, b);
}

string File_سطر::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::سائز_type pos;
    جب ((pos = name.ڈھونڈو_پہلا_not_کا(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_مقصد_Str(آخری_Lineno());
    واپس name;
}

string File_سطر::ascii_سطر_ستون() ثابت {
    واپس (cvt_مقصد_Str(پہلا_Lineno()) + "-" + cvt_مقصد_Str(آخری_Lineno()) + ":" + cvt_مقصد_Str(پہلا_عمود())
            + "-" + cvt_مقصد_Str(آخری_عمود()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_مقصد_Str(m_contentLineno) + "]");
}
string File_سطر::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    واپس filename() + ":" + cvt_مقصد_Str(آخری_Lineno()) + ":" + cvt_مقصد_Str(پہلا_عمود());
}
std::ostream& عامل<<(std::ostream& os, File_سطر* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    واپس (os);
}

string File_سطر::warnOffParse(ثابت string& msgs, منطقی turnOff) {
    string نتیجہ;
    برائے (ثابت string& پیغام_2 : VString::بانٹو(msgs, ',')) {
        ثابت V3_Error_رمز رمز{پیغام_2};
        اگر (!رمز.hardError()) {
            warnOff(رمز, turnOff);
            جاری;
        }
        // Error if not suppressed
        اگر (!v3Global.opt.isFuture(پیغام_2)) نتیجہ = VString::dot(نتیجہ, ",", پیغام_2);
    }
    واپس نتیجہ;
}

منطقی File_سطر::warnIsOff(V3_Error_رمز رمز) ثابت {
    اگر (!پیغام_En().enabled(رمز)) واپس سچ;
    اگر (!default_File_سطر().پیغام_En().enabled(رمز)) واپس سچ;  // Global overrides local
    واپس جھوٹ;
}

// cppverilator-suppress constParameter
خالی File_سطر::v3error_انتہا(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    اگر (آخری_Lineno()) nsstr << V3Error::warnContextBegin() << یہ << V3Error::warn_Context_انتہا();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    اگر (!extra.خالی_خانے()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().لمبائی_2()) << " " << ": "
                << V3Error::warn_Context_انتہا() << extra;
    }
    اگر (warnIsOff(V3Error::s().error_رمز())) {
        V3Error::s().suppressThisWarning();
    } ورنہ {
        اگر (!V3Error::s().errorContexted()) {
            ثابت string جوڑو = warnContextPrimary();
            wsstr << جوڑو;
            nsstr << جوڑو;
        }
        ثابت string waiver_متن = V3Error::strip_Meta_متن(wsstr.str(), جھوٹ);
        m_waive = V3Control::waive(یہ, V3Error::s().error_رمز(), waiver_متن);
        اگر (m_waive) {
            V3Error::s().suppressThisWarning();
        } ورنہ {
            V3Waiver::جوڑو_اندراج(V3Error::s().error_رمز(), filename(), waiver_متن);
        }
    }
    V3Error::v3error_انتہا(nsstr, extrass.str(), یہ);
}

string File_سطر::warn_مزید() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    اگر (آخری_Lineno()) {
        واپس V3Error::warnContextBegin() + V3Error::warn_مزید() + string(ascii().سائز(), ' ')
               + ": " + V3Error::warn_Context_انتہا();
    } ورنہ {
        واپس V3Error::warn_مزید();
    }
}
string File_سطر::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    اگر (آخری_Lineno()) {
        واپس V3Error::s().warnRelated(یہ) + V3Error::warnContextBegin() + V3Error::warn_مزید()
               + ascii() + ": " + V3Error::warn_Context_انتہا();
    } ورنہ {
        واپس V3Error::warn_مزید();
    }
};

string File_سطر::ماخذ() ثابت VL_MT_محفوظ {
    اگر (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        اگر (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            واپس "\n%Error: internal tracking of file contents failed";
        } ورنہ {
            واپس "";
        }
    }  // LCOV_EXCL_STOP
    واپس m_contentp->get_سطر(m_contentLineno);
}
string File_سطر::ماخذ_سابقہ(صحیح مقصد_عمود) ثابت VL_MT_محفوظ {
    ثابت std::string src = ماخذ();
    اگر (مقصد_عمود > static_cast<صحیح>(src.لمبائی_2())) مقصد_عمود = static_cast<صحیح>(src.لمبائی_2());
    اگر (مقصد_عمود < 1) واپس "";
    واپس src.substr(0, مقصد_عمود - 1);
}
string File_سطر::pretty_ماخذ() ثابت VL_MT_محفوظ {
    string برآمد = ماخذ();
    // Drop ignore trailing newline
    ثابت string::سائز_type pos = برآمد.ڈھونڈو('\n');
    اگر (pos != string::npos) برآمد = string(برآمد, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    واپس VString::spaceUnprintable(برآمد);
}

string File_سطر::warnContext() ثابت {
    اگر (!v3Global.opt.context()) واپس "";
    اگر (پہلا_Lineno() == آخری_Lineno() && پہلا_عمود()) {
        ثابت string ماخذ_سطر = pretty_ماخذ();
        string برآمد;
        // Don't show super-long lines as can fill screen and unlikely to help user
        اگر (!ماخذ_سطر.خالی_خانے() && ماخذ_سطر.لمبائی_2() < SHOW_ماخذ_MAX_لمبائی
            && ماخذ_سطر.لمبائی_2() >= static_cast<سائز_t>(آخری_عمود() - 1)) {
            string linestr = cvt_مقصد_Str(پہلا_Lineno());
            جب (linestr.سائز() < 5) linestr = ' ' + linestr;
            برآمد += linestr + " | " + ماخذ_سطر + '\n';
            برآمد += std::string(linestr.سائز(), ' ') + " | ";
            برآمد += string((پہلا_عمود() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            اگر (آخری_عمود() > پہلا_عمود()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                برآمد += string((آخری_عمود() - پہلا_عمود() - 1), '~');
            }
            برآمد += '\n';
        }
        واپس V3Error::warnContextBegin() + برآمد + V3Error::warn_Context_انتہا();
    }
    واپس "";
}

string File_سطر::warn_Context_والد() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string نتیجہ;
    برائے (File_سطر* والد_Fl = والد(); والد_Fl; والد_Fl = والد_Fl->والد()) {
        اگر (والد_Fl->filenameIsGlobal()) توڑو;
        نتیجہ += والد_Fl->warnOther() + "... note: In file included from '"
                  + والد_Fl->filebasename() + "'\n";
    }
    واپس نتیجہ;
}

#اگر_تعریف_شدہ VL_LEAK_CHECKS
std::unordered_سیٹ<File_سطر*> file_سطر_Leak_Checks;

خالی* File_سطر::عامل نیا(سائز_t سائز) {
    File_سطر* ثابت objp = static_cast<File_سطر*>(::عامل نیا(سائز));
    file_سطر_Leak_Checks.داخل_کرو(objp);
    واپس objp;
}

خالی File_سطر::عامل حذف(خالی* objp, سائز_t سائز) {
    اگر (!objp) واپس;
    File_سطر* ثابت flp = static_cast<File_سطر*>(objp);
    ثابت خودکار it = file_سطر_Leak_Checks.ڈھونڈو(flp);
    اگر (it != file_سطر_Leak_Checks.انتہا()) {
        file_سطر_Leak_Checks.erase(it);
    } ورنہ {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::عامل حذف(objp);
}
#اگر_ختم

خالی File_سطر::stats() {
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
    V3Stats::جوڑو_Stat_مجموع("FileLines, Number of filenames",
                        singleton().m_نام.سائز());  // Max m_filenameno
    V3Stats::جوڑو_Stat_مجموع("FileLines, Message enable sets",
                        singleton().m_interned_پیغام_Ens.سائز());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#اگر_ختم
}

خالی File_سطر::delete_All_بقایا() {
#اگر_تعریف_شدہ VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    جب (سچ) {
        ثابت خودکار it = file_سطر_Leak_Checks.begin();
        اگر (it == file_سطر_Leak_Checks.انتہا()) توڑو;
        حذف *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_سطر_Leak_Checks.clear();
    singleton().clear();
#اگر_ختم
}
