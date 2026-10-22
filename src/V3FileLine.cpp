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
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#پایان_اگر
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
    constexpr صحيح اندازه
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    حرف بیرون[اندازه];
    حرف* op = بیرون + اندازه - 1;
    *--op = '\0';  // We build backwards
    صحيح رقم = fileno;
    افعل {
        *--op = 'a' + رقم % 26;
        رقم /= 26;
    } طالما (رقم);
    أرجع op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_سطر_Singleton::fileNameIdx_t File_سطر_Singleton::name_مقصد_عدد(ثابت string& filename) {
    ثابت خودکار pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.اولی->دومی;
    إذا (pair.دومی) {
        ثابت اندازه_t بعدی_Idx = m_نامها.اندازه();
        UASSERT(بعدی_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_مقصد_Str(بعدی_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(بعدی_Idx);
        m_نامها.افزودن_پشت(filename);
        m_languages.افزودن_پشت(V3_Lang_کد::mostRecent());
    }
    أرجع idx;
}

فراغ File_سطر_Singleton::file_Name_رقم_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    لكل (ثابت خودکار& itr : m_namemap) {
        std::string filename = itr.اولی;
        إذا (filename == V3Options::get_Std_Package_مسیر()) {
            filename = "<verilated_std>";
        } وإلا إذا (filename == V3Options::get_Std_Waiver_مسیر()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.دومی) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_بیرون_Formatter::quoteNameControls(V3Os::filename_Real_مسیر(itr.اولی)) << '"'
           << ", \"language\":\"" << عدد_مقصد_Lang(itr.دومی).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_سطر_Singleton::پیام_En_مجموعه_Idx_t File_سطر_Singleton::جمع_کن_پیام_En_بیت_مجموعه(ثابت پیام_En_بیت_مجموعه& بیت_مجموعه)
    VL_MT_امن_EXCLUDES(m_mutex) {
    ثابت V3_قفل_Guard قفل_2{m_mutex};
    ثابت خودکار pair = m_interned_پیام_En_Idxs.emplace(بیت_مجموعه, 0);
    پیام_En_مجموعه_Idx_t& idx = pair.اولی->دومی;
    إذا (pair.دومی) {
        ثابت اندازه_t بعدی_Idx = m_interned_پیام_Ens.اندازه();
        UASSERT(بعدی_Idx <= std::numeric_limits<پیام_En_مجموعه_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_مقصد_Str(بعدی_Idx) + "+).");
        idx = static_cast<پیام_En_مجموعه_Idx_t>(بعدی_Idx);
        m_interned_پیام_Ens.افزودن_پشت(بیت_مجموعه);
    }
    أرجع idx;
}

File_سطر_Singleton::پیام_En_مجموعه_Idx_t File_سطر_Singleton::default_پیام_En_شاخص() VL_MT_امن {
    پیام_En_بیت_مجموعه پیام_En_بیت_مجموعه_2;
    لكل (صحيح i = V3_Error_کد::EC_MIN; i < V3_Error_کد::_ENUM_MAX; ++i) {
        ثابت V3_Error_کد کد{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        پیام_En_بیت_مجموعه_2.مجموعه(پیام_En_بیت_مجموعه::Subset::کد_2, کد, !کد.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        پیام_En_بیت_مجموعه_2.مجموعه(پیام_En_بیت_مجموعه::Subset::CTRL, کد, درست);
    }
    أرجع جمع_کن_پیام_En_بیت_مجموعه(پیام_En_بیت_مجموعه_2);
}

File_سطر_Singleton::پیام_En_مجموعه_Idx_t File_سطر_Singleton::پیام_En_مجموعه_بیت(پیام_En_مجموعه_Idx_t مجموعه_Idx,
                                                                پیام_En_بیت_مجموعه::Subset subset,
                                                                V3_Error_کد کد, منطقي مقدار) {
    // See if state matches existing
    منطقي same = درست;
    کد.forDelegateCodes([&](V3_Error_کد subcode) {
        إذا (پیام_En(مجموعه_Idx).test(subset, subcode) != مقدار) same = خطأ;
    });
    إذا (same) أرجع مجموعه_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیام_En_بیت_مجموعه پیام_En_بیت_مجموعه_2{پیام_En(مجموعه_Idx)};
    کد.forDelegateCodes([&](V3_Error_کد subcode) { پیام_En_بیت_مجموعه_2.مجموعه(subset, subcode, مقدار); });
    أرجع جمع_کن_پیام_En_بیت_مجموعه(پیام_En_بیت_مجموعه_2);
}

File_سطر_Singleton::پیام_En_مجموعه_Idx_t File_سطر_Singleton::پیام_مجموعه_Ctrl_بیت_مجموعه(پیام_En_مجموعه_Idx_t مجموعه_Idx,
                                                                     ثابت VError_بیت_مجموعه& bitset) {
    ثابت پیام_En_بیت_مجموعه::Subset subset = پیام_En_بیت_مجموعه::Subset::CTRL;
    // See if state matches existing
    إذا (پیام_En(مجموعه_Idx).getAll(subset) == bitset) أرجع مجموعه_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیام_En_بیت_مجموعه پیام_En_بیت_مجموعه_2{پیام_En(مجموعه_Idx)};
    پیام_En_بیت_مجموعه_2.مجموعه_All(subset, bitset);
    أرجع جمع_کن_پیام_En_بیت_مجموعه(پیام_En_بیت_مجموعه_2);
}

File_سطر_Singleton::پیام_En_مجموعه_Idx_t File_سطر_Singleton::پیام_En_And(پیام_En_مجموعه_Idx_t lhsIdx,
                                                             پیام_En_مجموعه_Idx_t rhsIdx) {
    ثابت پیام_En_بیت_مجموعه& lhs = پیام_En(lhsIdx);
    ثابت پیام_En_بیت_مجموعه& rhs = پیام_En(rhsIdx);
    ثابت پیام_En_بیت_مجموعه intersection{lhs, rhs};
    إذا (intersection == lhs) أرجع lhsIdx;
    إذا (intersection == rhs) أرجع rhsIdx;
    أرجع جمع_کن_پیام_En_بیت_مجموعه(intersection);
}

// ######################################################################
//  VFileContents class functions

فراغ VFileContent::افزودن_متن(ثابت string& متن) {
    // Similar code in WildcardContents::pushText()
    إذا (m_سطرها.اندازه() == 0) {
        m_سطرها.emplace_پشت("");  // no such thing as line [0]
        m_سطرها.emplace_پشت("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_سطرها.پشت() + متن;
    m_سطرها.برداشت_پشت();

    // Insert line-by-line
    string::اندازه_type سطر_شروع = 0;
    طالما (درست) {
        ثابت string::اندازه_type سطر_انتها = leftover.پیدا_کن('\n', سطر_شروع);
        إذا (سطر_انتها != string::npos) {
            ثابت string oneline(leftover, سطر_شروع, سطر_انتها - سطر_شروع + 1);
            m_سطرها.افزودن_پشت(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_سطرها.اندازه() - 1) << "]: " << oneline);
            سطر_شروع = سطر_انتها + 1;
        } وإلا {
            اكسر;
        }
    }
    // Keep leftover for next time
    m_سطرها.emplace_پشت(string(leftover, سطر_شروع));  // Might be ""
}

string VFileContent::get_سطر(صحيح lineno) ثابت VL_MT_امن {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    إذا (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحيح)m_سطرها.اندازه())) {
        إذا (debug() || v3Global.opt.debugCheck()) {
            أرجع ("%Error-internal-contents-bad-ct" + cvt_مقصد_Str(m_id) + "-ln" + cvt_مقصد_Str(lineno));
        } وإلا {
            أرجع "";
        }
    }
    string متن = m_سطرها[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << متن);
    أرجع متن;
}

std::ostream& عملگر<<(std::ostream& os, VFileContent* contentp) {
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
    أرجع "`line "s + cvt_مقصد_Str(آخرین_Lineno()) + " \""
           + V3_بیرون_Formatter::quoteNameControls(filename()) + "\" " + cvt_مقصد_Str(enterExit) + '\n';
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
        طالما (*textp && std::آیا_فاصله(*textp)) ++textp;
        طالما (*textp && !std::آیا_فاصله(*textp)) ++textp;
        طالما (*textp && std::آیا_فاصله(*textp)) ++textp;

        // Grab linenumber
        صحيح سطر_No;
        ثابت حرف* ثابت ln = textp;
        طالما (*textp && !std::آیا_فاصله(*textp)) ++textp;
        إذا (0 == مقایسه_رشته(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            سطر_No = lineno() + 1;
        } وإلا إذا (std::آیا_رقم(*ln)) {
            سطر_No = std::atoi(ln);
        } وإلا {
            اكسر;  // Fail
        }
        طالما (*textp && (std::آیا_فاصله(*textp))) ++textp;

        // Grab filename
        إذا (*textp != '"') اكسر;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        طالما (*textp && *textp != '"') ++textp;
        إذا (*textp != '"') اكسر;  // Fail
        string err_پیام;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_پیام);
        إذا (!err_پیام.خالی()) هذا->v3error(err_پیام.c_str());
        ++textp;
        طالما (*textp && std::آیا_فاصله(*textp)) ++textp;

        // Grab level
        إذا (!std::آیا_رقم(*textp)) اكسر;  // Fail
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

فراغ File_سطر::forwardToken(ثابت حرف* textp, اندازه_t اندازه, منطقي track_سطرها) {
    ساكن صحيح s_token_رقم = 1;
    m_token_رقم = s_token_رقم++;
    لكل (ثابت حرف* sp = textp; اندازه && *sp; ++sp, --اندازه) {
        إذا (*sp == '\n') {
            إذا (track_سطرها) linenoInc();
            m_آخرین_ستونی = 1;
        } وإلا إذا (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } وإلا {  // Tabs are considered one column; hence column means number of chars
            ++m_آخرین_ستونی;
        }
    }
}

فراغ File_سطر::applyIgnores() {
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(هذا);  // Toggle warnings based on global config file
#پایان_اگر
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
    ساكن File_سطر* s_آخرین_Newp = اشاره_تهی;
    إذا (s_آخرین_Newp && *s_آخرین_Newp == *هذا) {  // Compares lineno, filename, etc
        أرجع s_آخرین_Newp;
    }
    File_سطر* ثابت newp = جديد File_سطر{هذا};
    s_آخرین_Newp = newp;
    أرجع newp;
}

string File_سطر::filebasename() ثابت VL_MT_امن { أرجع V3Os::filenameNonDir(filename()); }

string File_سطر::filebasenameNoExt() ثابت { أرجع V3Os::filenameNonDirExt(filename()); }

string File_سطر::اولی_ستونی_Letters() ثابت VL_MT_امن {
    ثابت حرف a = ((اولی_ستونی() / 26) % 26) + 'a';
    ثابت حرف b = (اولی_ستونی() % 26) + 'a';
    أرجع string(1, a) + string(1, b);
}

string File_سطر::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::اندازه_type pos;
    طالما ((pos = name.پیدا_کن_اولی_not_از(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_مقصد_Str(آخرین_Lineno());
    أرجع name;
}

string File_سطر::ascii_سطر_ستون() ثابت {
    أرجع (cvt_مقصد_Str(اولی_Lineno()) + "-" + cvt_مقصد_Str(آخرین_Lineno()) + ":" + cvt_مقصد_Str(اولی_ستونی())
            + "-" + cvt_مقصد_Str(آخرین_ستونی()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_مقصد_Str(m_contentLineno) + "]");
}
string File_سطر::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    أرجع filename() + ":" + cvt_مقصد_Str(آخرین_Lineno()) + ":" + cvt_مقصد_Str(اولی_ستونی());
}
std::ostream& عملگر<<(std::ostream& os, File_سطر* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    أرجع (os);
}

string File_سطر::warnOffParse(ثابت string& msgs, منطقي turnOff) {
    string نتیجه;
    لكل (ثابت string& پیام_2 : VString::تقسیم(msgs, ',')) {
        ثابت V3_Error_کد کد{پیام_2};
        إذا (!کد.hardError()) {
            warnOff(کد, turnOff);
            استمر;
        }
        // Error if not suppressed
        إذا (!v3Global.opt.isFuture(پیام_2)) نتیجه = VString::dot(نتیجه, ",", پیام_2);
    }
    أرجع نتیجه;
}

منطقي File_سطر::warnIsOff(V3_Error_کد کد) ثابت {
    إذا (!پیام_En().enabled(کد)) أرجع درست;
    إذا (!default_File_سطر().پیام_En().enabled(کد)) أرجع درست;  // Global overrides local
    أرجع خطأ;
}

// cppverilator-suppress constParameter
فراغ File_سطر::v3error_انتها(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    إذا (آخرین_Lineno()) nsstr << V3Error::warnContextBegin() << هذا << V3Error::warn_Context_انتها();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    إذا (!extra.خالی()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().درازی()) << " " << ": "
                << V3Error::warn_Context_انتها() << extra;
    }
    إذا (warnIsOff(V3Error::s().error_کد())) {
        V3Error::s().suppressThisWarning();
    } وإلا {
        إذا (!V3Error::s().errorContexted()) {
            ثابت string جمع_کن = warnContextPrimary();
            wsstr << جمع_کن;
            nsstr << جمع_کن;
        }
        ثابت string waiver_متن = V3Error::strip_Meta_متن(wsstr.str(), خطأ);
        m_waive = V3Control::waive(هذا, V3Error::s().error_کد(), waiver_متن);
        إذا (m_waive) {
            V3Error::s().suppressThisWarning();
        } وإلا {
            V3Waiver::جمع_کن_مدخل(V3Error::s().error_کد(), filename(), waiver_متن);
        }
    }
    V3Error::v3error_انتها(nsstr, extrass.str(), هذا);
}

string File_سطر::warn_بیشتر() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (آخرین_Lineno()) {
        أرجع V3Error::warnContextBegin() + V3Error::warn_بیشتر() + string(ascii().اندازه(), ' ')
               + ": " + V3Error::warn_Context_انتها();
    } وإلا {
        أرجع V3Error::warn_بیشتر();
    }
}
string File_سطر::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (آخرین_Lineno()) {
        أرجع V3Error::s().warnRelated(هذا) + V3Error::warnContextBegin() + V3Error::warn_بیشتر()
               + ascii() + ": " + V3Error::warn_Context_انتها();
    } وإلا {
        أرجع V3Error::warn_بیشتر();
    }
};

string File_سطر::منبع() ثابت VL_MT_امن {
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
string File_سطر::منبع_پیشوند(صحيح مقصد_ستونی) ثابت VL_MT_امن {
    ثابت std::string src = منبع();
    إذا (مقصد_ستونی > static_cast<صحيح>(src.درازی())) مقصد_ستونی = static_cast<صحيح>(src.درازی());
    إذا (مقصد_ستونی < 1) أرجع "";
    أرجع src.substr(0, مقصد_ستونی - 1);
}
string File_سطر::pretty_منبع() ثابت VL_MT_امن {
    string بیرون = منبع();
    // Drop ignore trailing newline
    ثابت string::اندازه_type pos = بیرون.پیدا_کن('\n');
    إذا (pos != string::npos) بیرون = string(بیرون, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    أرجع VString::spaceUnprintable(بیرون);
}

string File_سطر::warnContext() ثابت {
    إذا (!v3Global.opt.context()) أرجع "";
    إذا (اولی_Lineno() == آخرین_Lineno() && اولی_ستونی()) {
        ثابت string منبع_سطر = pretty_منبع();
        string بیرون;
        // Don't show super-long lines as can fill screen and unlikely to help user
        إذا (!منبع_سطر.خالی() && منبع_سطر.درازی() < SHOW_منبع_MAX_درازی
            && منبع_سطر.درازی() >= static_cast<اندازه_t>(آخرین_ستونی() - 1)) {
            string linestr = cvt_مقصد_Str(اولی_Lineno());
            طالما (linestr.اندازه() < 5) linestr = ' ' + linestr;
            بیرون += linestr + " | " + منبع_سطر + '\n';
            بیرون += std::string(linestr.اندازه(), ' ') + " | ";
            بیرون += string((اولی_ستونی() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            إذا (آخرین_ستونی() > اولی_ستونی()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                بیرون += string((آخرین_ستونی() - اولی_ستونی() - 1), '~');
            }
            بیرون += '\n';
        }
        أرجع V3Error::warnContextBegin() + بیرون + V3Error::warn_Context_انتها();
    }
    أرجع "";
}

string File_سطر::warn_Context_والد() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string نتیجه;
    لكل (File_سطر* والد_Fl = والد(); والد_Fl; والد_Fl = والد_Fl->والد()) {
        إذا (والد_Fl->filenameIsGlobal()) اكسر;
        نتیجه += والد_Fl->warnOther() + "... note: In file included from '"
                  + والد_Fl->filebasename() + "'\n";
    }
    أرجع نتیجه;
}

#اگر_تعریف_شده VL_LEAK_CHECKS
std::unordered_مجموعه<File_سطر*> file_سطر_Leak_Checks;

فراغ* File_سطر::عملگر جديد(اندازه_t اندازه) {
    File_سطر* ثابت objp = static_cast<File_سطر*>(::عملگر جديد(اندازه));
    file_سطر_Leak_Checks.درج(objp);
    أرجع objp;
}

فراغ File_سطر::عملگر حذف(فراغ* objp, اندازه_t اندازه) {
    إذا (!objp) أرجع;
    File_سطر* ثابت flp = static_cast<File_سطر*>(objp);
    ثابت خودکار it = file_سطر_Leak_Checks.پیدا_کن(flp);
    إذا (it != file_سطر_Leak_Checks.انتها()) {
        file_سطر_Leak_Checks.erase(it);
    } وإلا {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::عملگر حذف(objp);
}
#پایان_اگر

فراغ File_سطر::stats() {
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
    V3Stats::جمع_کن_Stat_جمع("FileLines, Number of filenames",
                        singleton().m_نامها.اندازه());  // Max m_filenameno
    V3Stats::جمع_کن_Stat_جمع("FileLines, Message enable sets",
                        singleton().m_interned_پیام_Ens.اندازه());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#پایان_اگر
}

فراغ File_سطر::delete_All_مانده() {
#اگر_تعریف_شده VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    طالما (درست) {
        ثابت خودکار it = file_سطر_Leak_Checks.begin();
        إذا (it == file_سطر_Leak_Checks.انتها()) اكسر;
        حذف *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_سطر_Leak_Checks.clear();
    singleton().clear();
#پایان_اگر
}
