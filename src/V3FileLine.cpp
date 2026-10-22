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
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#جيڪڏهن_پورو
#ضمن "V3Waiver.h"
// clang-format on

#ضمن <algorithm>
#ضمن <iomanip>
#ضمن <limits>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_سٽ_Singleton::filenameLetters(fileNameIdx_t fileno) VL_خالص {
    constexpr صحيح سائيز
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    حرف ٻاهريون[سائيز];
    حرف* op = ٻاهريون + سائيز - 1;
    *--op = '\0';  // We build backwards
    صحيح انگ = fileno;
    افعل {
        *--op = 'a' + انگ % 26;
        انگ /= 26;
    } طالما (انگ);
    أرجع op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_سٽ_Singleton::fileNameIdx_t File_سٽ_Singleton::name_منزل_عدد(ثابت string& filename) {
    ثابت خودڪار pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.پهريون->ٻيو;
    إذا (pair.ٻيو) {
        ثابت سائيز_t ايندڙ_Idx = m_نالا.سائيز();
        UASSERT(ايندڙ_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_منزل_Str(ايندڙ_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(ايندڙ_Idx);
        m_نالا.ڌڪو_پٺ(filename);
        m_languages.ڌڪو_پٺ(V3_Lang_ڪوڊ::mostRecent());
    }
    أرجع idx;
}

فراغ File_سٽ_Singleton::file_Name_انگ_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    لكل (ثابت خودڪار& itr : m_namemap) {
        std::string filename = itr.پهريون;
        إذا (filename == V3Options::get_Std_Package_رستو()) {
            filename = "<verilated_std>";
        } وإلا إذا (filename == V3Options::get_Std_Waiver_رستو()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.ٻيو) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_ٻاهريون_Formatter::quoteNameControls(V3Os::filename_Real_رستو(itr.پهريون)) << '"'
           << ", \"language\":\"" << عدد_منزل_Lang(itr.ٻيو).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_سٽ_Singleton::نياپو_En_سيٽ_Idx_t File_سٽ_Singleton::جوڙيو_نياپو_En_ٻٽو_انگ_سيٽ(ثابت نياپو_En_ٻٽو_انگ_سيٽ& ٻٽو_انگ_سيٽ)
    VL_MT_محفوظ_EXCLUDES(m_mutex) {
    ثابت V3_تالو_Guard تالو_2{m_mutex};
    ثابت خودڪار pair = m_interned_نياپو_En_Idxs.emplace(ٻٽو_انگ_سيٽ, 0);
    نياپو_En_سيٽ_Idx_t& idx = pair.پهريون->ٻيو;
    إذا (pair.ٻيو) {
        ثابت سائيز_t ايندڙ_Idx = m_interned_نياپو_Ens.سائيز();
        UASSERT(ايندڙ_Idx <= std::numeric_limits<نياپو_En_سيٽ_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_منزل_Str(ايندڙ_Idx) + "+).");
        idx = static_cast<نياپو_En_سيٽ_Idx_t>(ايندڙ_Idx);
        m_interned_نياپو_Ens.ڌڪو_پٺ(ٻٽو_انگ_سيٽ);
    }
    أرجع idx;
}

File_سٽ_Singleton::نياپو_En_سيٽ_Idx_t File_سٽ_Singleton::default_نياپو_En_اشاريو() VL_MT_محفوظ {
    نياپو_En_ٻٽو_انگ_سيٽ نياپو_En_ٻٽو_انگ_سيٽ_2;
    لكل (صحيح i = V3_Error_ڪوڊ::EC_MIN; i < V3_Error_ڪوڊ::_ENUM_MAX; ++i) {
        ثابت V3_Error_ڪوڊ ڪوڊ{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        نياپو_En_ٻٽو_انگ_سيٽ_2.سيٽ(نياپو_En_ٻٽو_انگ_سيٽ::Subset::ڪوڊ_2, ڪوڊ, !ڪوڊ.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        نياپو_En_ٻٽو_انگ_سيٽ_2.سيٽ(نياپو_En_ٻٽو_انگ_سيٽ::Subset::CTRL, ڪوڊ, سچ);
    }
    أرجع جوڙيو_نياپو_En_ٻٽو_انگ_سيٽ(نياپو_En_ٻٽو_انگ_سيٽ_2);
}

File_سٽ_Singleton::نياپو_En_سيٽ_Idx_t File_سٽ_Singleton::نياپو_En_سيٽ_ٻٽو_انگ(نياپو_En_سيٽ_Idx_t سيٽ_Idx,
                                                                نياپو_En_ٻٽو_انگ_سيٽ::Subset subset,
                                                                V3_Error_ڪوڊ ڪوڊ, منطقي قدر) {
    // See if state matches existing
    منطقي same = سچ;
    ڪوڊ.forDelegateCodes([&](V3_Error_ڪوڊ subcode) {
        إذا (نياپو_En(سيٽ_Idx).test(subset, subcode) != قدر) same = خطأ;
    });
    إذا (same) أرجع سيٽ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    نياپو_En_ٻٽو_انگ_سيٽ نياپو_En_ٻٽو_انگ_سيٽ_2{نياپو_En(سيٽ_Idx)};
    ڪوڊ.forDelegateCodes([&](V3_Error_ڪوڊ subcode) { نياپو_En_ٻٽو_انگ_سيٽ_2.سيٽ(subset, subcode, قدر); });
    أرجع جوڙيو_نياپو_En_ٻٽو_انگ_سيٽ(نياپو_En_ٻٽو_انگ_سيٽ_2);
}

File_سٽ_Singleton::نياپو_En_سيٽ_Idx_t File_سٽ_Singleton::نياپو_سيٽ_Ctrl_ٻٽو_انگ_سيٽ(نياپو_En_سيٽ_Idx_t سيٽ_Idx,
                                                                     ثابت VError_ٻٽو_انگ_سيٽ& bitset) {
    ثابت نياپو_En_ٻٽو_انگ_سيٽ::Subset subset = نياپو_En_ٻٽو_انگ_سيٽ::Subset::CTRL;
    // See if state matches existing
    إذا (نياپو_En(سيٽ_Idx).getAll(subset) == bitset) أرجع سيٽ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    نياپو_En_ٻٽو_انگ_سيٽ نياپو_En_ٻٽو_انگ_سيٽ_2{نياپو_En(سيٽ_Idx)};
    نياپو_En_ٻٽو_انگ_سيٽ_2.سيٽ_All(subset, bitset);
    أرجع جوڙيو_نياپو_En_ٻٽو_انگ_سيٽ(نياپو_En_ٻٽو_انگ_سيٽ_2);
}

File_سٽ_Singleton::نياپو_En_سيٽ_Idx_t File_سٽ_Singleton::نياپو_En_And(نياپو_En_سيٽ_Idx_t lhsIdx,
                                                             نياپو_En_سيٽ_Idx_t rhsIdx) {
    ثابت نياپو_En_ٻٽو_انگ_سيٽ& lhs = نياپو_En(lhsIdx);
    ثابت نياپو_En_ٻٽو_انگ_سيٽ& rhs = نياپو_En(rhsIdx);
    ثابت نياپو_En_ٻٽو_انگ_سيٽ intersection{lhs, rhs};
    إذا (intersection == lhs) أرجع lhsIdx;
    إذا (intersection == rhs) أرجع rhsIdx;
    أرجع جوڙيو_نياپو_En_ٻٽو_انگ_سيٽ(intersection);
}

// ######################################################################
//  VFileContents class functions

فراغ VFileContent::ڌڪو_متن(ثابت string& متن) {
    // Similar code in WildcardContents::pushText()
    إذا (m_سٽون.سائيز() == 0) {
        m_سٽون.emplace_پٺ("");  // no such thing as line [0]
        m_سٽون.emplace_پٺ("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_سٽون.پٺ() + متن;
    m_سٽون.ڪڍو_پٺ();

    // Insert line-by-line
    string::سائيز_type سٽ_شروعات = 0;
    طالما (سچ) {
        ثابت string::سائيز_type سٽ_پڇاڙي = leftover.ڳوليو('\n', سٽ_شروعات);
        إذا (سٽ_پڇاڙي != string::npos) {
            ثابت string oneline(leftover, سٽ_شروعات, سٽ_پڇاڙي - سٽ_شروعات + 1);
            m_سٽون.ڌڪو_پٺ(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_سٽون.سائيز() - 1) << "]: " << oneline);
            سٽ_شروعات = سٽ_پڇاڙي + 1;
        } وإلا {
            اكسر;
        }
    }
    // Keep leftover for next time
    m_سٽون.emplace_پٺ(string(leftover, سٽ_شروعات));  // Might be ""
}

string VFileContent::get_سٽ(صحيح lineno) ثابت VL_MT_محفوظ {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    إذا (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحيح)m_سٽون.سائيز())) {
        إذا (debug() || v3Global.opt.debugCheck()) {
            أرجع ("%Error-internal-contents-bad-ct" + cvt_منزل_Str(m_id) + "-ln" + cvt_منزل_Str(lineno));
        } وإلا {
            أرجع "";
        }
    }
    string متن = m_سٽون[lineno];
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

File_سٽ::~File_سٽ() {
    إذا (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

فراغ File_سٽ::newContent() {
    إذا (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = جديد VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_سٽ::سٽ_Directive_Strg(صحيح enterExit) ثابت {
    أرجع "`line "s + cvt_منزل_Str(آخري_Lineno()) + " \""
           + V3_ٻاهريون_Formatter::quoteNameControls(filename()) + "\" " + cvt_منزل_Str(enterExit) + '\n';
}

فراغ File_سٽ::سٽ_Directive(ثابت حرف* textp, صحيح& enterExitRef) {
    string newFilename;
    صحيح newLineno = -1;
    سٽ_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    إذا (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } وإلا {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

فراغ File_سٽ::سٽ_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                                  صحيح& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    افعل {
        // Skip `line
        طالما (*textp && std::خال_آهي(*textp)) ++textp;
        طالما (*textp && !std::خال_آهي(*textp)) ++textp;
        طالما (*textp && std::خال_آهي(*textp)) ++textp;

        // Grab linenumber
        صحيح سٽ_No;
        ثابت حرف* ثابت ln = textp;
        طالما (*textp && !std::خال_آهي(*textp)) ++textp;
        إذا (0 == لڙي_ڀيٽ(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            سٽ_No = lineno() + 1;
        } وإلا إذا (std::انگ_آهي(*ln)) {
            سٽ_No = std::atoi(ln);
        } وإلا {
            اكسر;  // Fail
        }
        طالما (*textp && (std::خال_آهي(*textp))) ++textp;

        // Grab filename
        إذا (*textp != '"') اكسر;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        طالما (*textp && *textp != '"') ++textp;
        إذا (*textp != '"') اكسر;  // Fail
        string err_نياپو;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_نياپو);
        إذا (!err_نياپو.خالي_جايون()) هذا->v3error(err_نياپو.c_str());
        ++textp;
        طالما (*textp && std::خال_آهي(*textp)) ++textp;

        // Grab level
        إذا (!std::انگ_آهي(*textp)) اكسر;  // Fail
        ثابت صحيح level = std::atoi(textp);
        إذا (level < 0 || level >= 3) اكسر;  // Fail

        linenoRef = سٽ_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        أرجع;
    } طالما (خطأ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

فراغ File_سٽ::forwardToken(ثابت حرف* textp, سائيز_t سائيز, منطقي track_سٽون) {
    ساكن صحيح s_token_انگ = 1;
    m_token_انگ = s_token_انگ++;
    لكل (ثابت حرف* sp = textp; سائيز && *sp; ++sp, --سائيز) {
        إذا (*sp == '\n') {
            إذا (track_سٽون) linenoInc();
            m_آخري_ڪالم = 1;
        } وإلا إذا (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } وإلا {  // Tabs are considered one column; hence column means number of chars
            ++m_آخري_ڪالم;
        }
    }
}

فراغ File_سٽ::applyIgnores() {
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(هذا);  // Toggle warnings based on global config file
#جيڪڏهن_پورو
}

File_سٽ* File_سٽ::copy_Or_Same_File_سٽ_Applied() {
    applyIgnores();
    أرجع copy_Or_Same_File_سٽ();
}

File_سٽ* File_سٽ::copy_Or_Same_File_سٽ() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ساكن File_سٽ* s_آخري_Newp = خالي_اشارو;
    إذا (s_آخري_Newp && *s_آخري_Newp == *هذا) {  // Compares lineno, filename, etc
        أرجع s_آخري_Newp;
    }
    File_سٽ* ثابت newp = جديد File_سٽ{هذا};
    s_آخري_Newp = newp;
    أرجع newp;
}

string File_سٽ::filebasename() ثابت VL_MT_محفوظ { أرجع V3Os::filenameNonDir(filename()); }

string File_سٽ::filebasenameNoExt() ثابت { أرجع V3Os::filenameNonDirExt(filename()); }

string File_سٽ::پهريون_ڪالم_Letters() ثابت VL_MT_محفوظ {
    ثابت حرف a = ((پهريون_ڪالم() / 26) % 26) + 'a';
    ثابت حرف b = (پهريون_ڪالم() % 26) + 'a';
    أرجع string(1, a) + string(1, b);
}

string File_سٽ::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::سائيز_type pos;
    طالما ((pos = name.ڳوليو_پهريون_not_جو(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_منزل_Str(آخري_Lineno());
    أرجع name;
}

string File_سٽ::ascii_سٽ_کانو() ثابت {
    أرجع (cvt_منزل_Str(پهريون_Lineno()) + "-" + cvt_منزل_Str(آخري_Lineno()) + ":" + cvt_منزل_Str(پهريون_ڪالم())
            + "-" + cvt_منزل_Str(آخري_ڪالم()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_منزل_Str(m_contentLineno) + "]");
}
string File_سٽ::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    أرجع filename() + ":" + cvt_منزل_Str(آخري_Lineno()) + ":" + cvt_منزل_Str(پهريون_ڪالم());
}
std::ostream& عامل<<(std::ostream& os, File_سٽ* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    أرجع (os);
}

string File_سٽ::warnOffParse(ثابت string& msgs, منطقي turnOff) {
    string نتيجو;
    لكل (ثابت string& نياپو : VString::ورهايو(msgs, ',')) {
        ثابت V3_Error_ڪوڊ ڪوڊ{نياپو};
        إذا (!ڪوڊ.hardError()) {
            warnOff(ڪوڊ, turnOff);
            استمر;
        }
        // Error if not suppressed
        إذا (!v3Global.opt.isFuture(نياپو)) نتيجو = VString::dot(نتيجو, ",", نياپو);
    }
    أرجع نتيجو;
}

منطقي File_سٽ::warnIsOff(V3_Error_ڪوڊ ڪوڊ) ثابت {
    إذا (!نياپو_En().enabled(ڪوڊ)) أرجع سچ;
    إذا (!default_File_سٽ().نياپو_En().enabled(ڪوڊ)) أرجع سچ;  // Global overrides local
    أرجع خطأ;
}

// cppverilator-suppress constParameter
فراغ File_سٽ::v3error_پڇاڙي(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    إذا (آخري_Lineno()) nsstr << V3Error::warnContextBegin() << هذا << V3Error::warn_Context_پڇاڙي();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    إذا (!extra.خالي_جايون()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().ڊيگهائي()) << " " << ": "
                << V3Error::warn_Context_پڇاڙي() << extra;
    }
    إذا (warnIsOff(V3Error::s().error_ڪوڊ())) {
        V3Error::s().suppressThisWarning();
    } وإلا {
        إذا (!V3Error::s().errorContexted()) {
            ثابت string جوڙيو = warnContextPrimary();
            wsstr << جوڙيو;
            nsstr << جوڙيو;
        }
        ثابت string waiver_متن = V3Error::strip_Meta_متن(wsstr.str(), خطأ);
        m_waive = V3Control::waive(هذا, V3Error::s().error_ڪوڊ(), waiver_متن);
        إذا (m_waive) {
            V3Error::s().suppressThisWarning();
        } وإلا {
            V3Waiver::جوڙيو_داخلا_2(V3Error::s().error_ڪوڊ(), filename(), waiver_متن);
        }
    }
    V3Error::v3error_پڇاڙي(nsstr, extrass.str(), هذا);
}

string File_سٽ::warn_وڌيڪ() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (آخري_Lineno()) {
        أرجع V3Error::warnContextBegin() + V3Error::warn_وڌيڪ() + string(ascii().سائيز(), ' ')
               + ": " + V3Error::warn_Context_پڇاڙي();
    } وإلا {
        أرجع V3Error::warn_وڌيڪ();
    }
}
string File_سٽ::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (آخري_Lineno()) {
        أرجع V3Error::s().warnRelated(هذا) + V3Error::warnContextBegin() + V3Error::warn_وڌيڪ()
               + ascii() + ": " + V3Error::warn_Context_پڇاڙي();
    } وإلا {
        أرجع V3Error::warn_وڌيڪ();
    }
};

string File_سٽ::ذريعو() ثابت VL_MT_محفوظ {
    إذا (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        إذا (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            أرجع "\n%Error: internal tracking of file contents failed";
        } وإلا {
            أرجع "";
        }
    }  // LCOV_EXCL_STOP
    أرجع m_contentp->get_سٽ(m_contentLineno);
}
string File_سٽ::ذريعو_اڳياڙي(صحيح منزل_ڪالم) ثابت VL_MT_محفوظ {
    ثابت std::string src = ذريعو();
    إذا (منزل_ڪالم > static_cast<صحيح>(src.ڊيگهائي())) منزل_ڪالم = static_cast<صحيح>(src.ڊيگهائي());
    إذا (منزل_ڪالم < 1) أرجع "";
    أرجع src.substr(0, منزل_ڪالم - 1);
}
string File_سٽ::pretty_ذريعو() ثابت VL_MT_محفوظ {
    string ٻاهريون = ذريعو();
    // Drop ignore trailing newline
    ثابت string::سائيز_type pos = ٻاهريون.ڳوليو('\n');
    إذا (pos != string::npos) ٻاهريون = string(ٻاهريون, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    أرجع VString::spaceUnprintable(ٻاهريون);
}

string File_سٽ::warnContext() ثابت {
    إذا (!v3Global.opt.context()) أرجع "";
    إذا (پهريون_Lineno() == آخري_Lineno() && پهريون_ڪالم()) {
        ثابت string ذريعو_سٽ = pretty_ذريعو();
        string ٻاهريون;
        // Don't show super-long lines as can fill screen and unlikely to help user
        إذا (!ذريعو_سٽ.خالي_جايون() && ذريعو_سٽ.ڊيگهائي() < SHOW_ذريعو_MAX_ڊيگهائي
            && ذريعو_سٽ.ڊيگهائي() >= static_cast<سائيز_t>(آخري_ڪالم() - 1)) {
            string linestr = cvt_منزل_Str(پهريون_Lineno());
            طالما (linestr.سائيز() < 5) linestr = ' ' + linestr;
            ٻاهريون += linestr + " | " + ذريعو_سٽ + '\n';
            ٻاهريون += std::string(linestr.سائيز(), ' ') + " | ";
            ٻاهريون += string((پهريون_ڪالم() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            إذا (آخري_ڪالم() > پهريون_ڪالم()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                ٻاهريون += string((آخري_ڪالم() - پهريون_ڪالم() - 1), '~');
            }
            ٻاهريون += '\n';
        }
        أرجع V3Error::warnContextBegin() + ٻاهريون + V3Error::warn_Context_پڇاڙي();
    }
    أرجع "";
}

string File_سٽ::warn_Context_والد() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string نتيجو;
    لكل (File_سٽ* والد_Fl = والد(); والد_Fl; والد_Fl = والد_Fl->والد()) {
        إذا (والد_Fl->filenameIsGlobal()) اكسر;
        نتيجو += والد_Fl->warnOther() + "... note: In file included from '"
                  + والد_Fl->filebasename() + "'\n";
    }
    أرجع نتيجو;
}

#جيڪڏهن_وصف_ٿيل VL_LEAK_CHECKS
std::unordered_سيٽ<File_سٽ*> file_سٽ_Leak_Checks;

فراغ* File_سٽ::عامل جديد(سائيز_t سائيز) {
    File_سٽ* ثابت objp = static_cast<File_سٽ*>(::عامل جديد(سائيز));
    file_سٽ_Leak_Checks.وجهو(objp);
    أرجع objp;
}

فراغ File_سٽ::عامل ميٽيو(فراغ* objp, سائيز_t سائيز) {
    إذا (!objp) أرجع;
    File_سٽ* ثابت flp = static_cast<File_سٽ*>(objp);
    ثابت خودڪار it = file_سٽ_Leak_Checks.ڳوليو(flp);
    إذا (it != file_سٽ_Leak_Checks.پڇاڙي()) {
        file_سٽ_Leak_Checks.erase(it);
    } وإلا {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::عامل ميٽيو(objp);
}
#جيڪڏهن_پورو

فراغ File_سٽ::stats() {
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
    V3Stats::جوڙيو_Stat_جوڙ("FileLines, Number of filenames",
                        singleton().m_نالا.سائيز());  // Max m_filenameno
    V3Stats::جوڙيو_Stat_جوڙ("FileLines, Message enable sets",
                        singleton().m_interned_نياپو_Ens.سائيز());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#جيڪڏهن_پورو
}

فراغ File_سٽ::delete_All_بچيل() {
#جيڪڏهن_وصف_ٿيل VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    طالما (سچ) {
        ثابت خودڪار it = file_سٽ_Leak_Checks.begin();
        إذا (it == file_سٽ_Leak_Checks.پڇاڙي()) اكسر;
        ميٽيو *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_سٽ_Leak_Checks.clear();
    singleton().clear();
#جيڪڏهن_پورو
}
