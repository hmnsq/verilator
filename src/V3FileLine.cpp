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
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#ضمن "V3Waiver.h"
// clang-format on

#ضمن <algorithm>
#ضمن <iomanip>
#ضمن <limits>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_سطر_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr صحيح size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    حرف out[size];
    حرف* op = out + size - 1;
    *--op = '\0';  // We build backwards
    صحيح رقم = fileno;
    افعل {
        *--op = 'a' + رقم % 26;
        رقم /= 26;
    } بينما (رقم);
    إرجاع op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_سطر_Singleton::fileNameIdx_t File_سطر_Singleton::name_To_العدد(ثابت string& filename) {
    ثابت auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    إذا (pair.second) {
        ثابت size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.ادفع_back(filename);
        m_languages.ادفع_back(V3LangCode::mostRecent());
    }
    إرجاع idx;
}

فراغ File_سطر_Singleton::file_Name_رقم_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    لكل (ثابت auto& itr : m_namemap) {
        std::string filename = itr.first;
        إذا (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } وإلا إذا (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << العدد_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_سطر_Singleton::رسالة_En_Set_Idx_t File_سطر_Singleton::add_رسالة_En_Bit_Set(ثابت رسالة_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ثابت V3LockGuard lock{m_mutex};
    ثابت auto pair = m_interned_رسالة_En_Idxs.emplace(bitSet, 0);
    رسالة_En_Set_Idx_t& idx = pair.first->second;
    إذا (pair.second) {
        ثابت size_t nextIdx = m_interned_رسالة_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<رسالة_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<رسالة_En_Set_Idx_t>(nextIdx);
        m_interned_رسالة_Ens.ادفع_back(bitSet);
    }
    إرجاع idx;
}

File_سطر_Singleton::رسالة_En_Set_Idx_t File_سطر_Singleton::default_رسالة_En_Index() VL_MT_SAFE {
    رسالة_En_Bit_Set رسالة_En_Bit_Set_2;
    لكل (صحيح i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ثابت V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        رسالة_En_Bit_Set_2.set(رسالة_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        رسالة_En_Bit_Set_2.set(رسالة_En_Bit_Set::Subset::CTRL, code, true);
    }
    إرجاع add_رسالة_En_Bit_Set(رسالة_En_Bit_Set_2);
}

File_سطر_Singleton::رسالة_En_Set_Idx_t File_سطر_Singleton::رسالة_En_Set_Bit(رسالة_En_Set_Idx_t setIdx,
                                                                رسالة_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, منطقي قيمة) {
    // See if state matches existing
    منطقي same = true;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        إذا (رسالة_En(setIdx).test(subset, subcode) != قيمة) same = خطأ;
    });
    إذا (same) إرجاع setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    رسالة_En_Bit_Set رسالة_En_Bit_Set_2{رسالة_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { رسالة_En_Bit_Set_2.set(subset, subcode, قيمة); });
    إرجاع add_رسالة_En_Bit_Set(رسالة_En_Bit_Set_2);
}

File_سطر_Singleton::رسالة_En_Set_Idx_t File_سطر_Singleton::رسالة_Set_Ctrl_Bit_Set(رسالة_En_Set_Idx_t setIdx,
                                                                     ثابت VErrorBitSet& bitset) {
    ثابت رسالة_En_Bit_Set::Subset subset = رسالة_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    إذا (رسالة_En(setIdx).getAll(subset) == bitset) إرجاع setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    رسالة_En_Bit_Set رسالة_En_Bit_Set_2{رسالة_En(setIdx)};
    رسالة_En_Bit_Set_2.setAll(subset, bitset);
    إرجاع add_رسالة_En_Bit_Set(رسالة_En_Bit_Set_2);
}

File_سطر_Singleton::رسالة_En_Set_Idx_t File_سطر_Singleton::رسالة_En_And(رسالة_En_Set_Idx_t lhsIdx,
                                                             رسالة_En_Set_Idx_t rhsIdx) {
    ثابت رسالة_En_Bit_Set& lhs = رسالة_En(lhsIdx);
    ثابت رسالة_En_Bit_Set& rhs = رسالة_En(rhsIdx);
    ثابت رسالة_En_Bit_Set intersection{lhs, rhs};
    إذا (intersection == lhs) إرجاع lhsIdx;
    إذا (intersection == rhs) إرجاع rhsIdx;
    إرجاع add_رسالة_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

فراغ VFileContent::ادفع_Text(ثابت string& text) {
    // Similar code in WildcardContents::pushText()
    إذا (m_أسطر.size() == 0) {
        m_أسطر.emplace_back("");  // no such thing as line [0]
        m_أسطر.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_أسطر.back() + text;
    m_أسطر.اسحب_back();

    // Insert line-by-line
    string::size_type سطر_start = 0;
    بينما (true) {
        ثابت string::size_type سطر_end = leftover.find('\n', سطر_start);
        إذا (سطر_end != string::npos) {
            ثابت string oneline(leftover, سطر_start, سطر_end - سطر_start + 1);
            m_أسطر.ادفع_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_أسطر.size() - 1) << "]: " << oneline);
            سطر_start = سطر_end + 1;
        } وإلا {
            اكسر;
        }
    }
    // Keep leftover for next time
    m_أسطر.emplace_back(string(leftover, سطر_start));  // Might be ""
}

string VFileContent::get_سطر(صحيح lineno) ثابت VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    إذا (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحيح)m_أسطر.size())) {
        إذا (debug() || v3Global.opt.debugCheck()) {
            إرجاع ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } وإلا {
            إرجاع "";
        }
    }
    string text = m_أسطر[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    إرجاع text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    إذا (!contentp) {
        os << "ct0";
    } وإلا {
        os << contentp->ascii();
    }
    إرجاع os;
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
    إرجاع "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
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
        بينما (*textp && std::isspace(*textp)) ++textp;
        بينما (*textp && !std::isspace(*textp)) ++textp;
        بينما (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        صحيح سطر_No;
        ثابت حرف* ثابت ln = textp;
        بينما (*textp && !std::isspace(*textp)) ++textp;
        إذا (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            سطر_No = lineno() + 1;
        } وإلا إذا (std::isdigit(*ln)) {
            سطر_No = std::atoi(ln);
        } وإلا {
            اكسر;  // Fail
        }
        بينما (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        إذا (*textp != '"') اكسر;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        بينما (*textp && *textp != '"') ++textp;
        إذا (*textp != '"') اكسر;  // Fail
        string err_رسالة;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_رسالة);
        إذا (!err_رسالة.empty()) هذا->v3error(err_رسالة.c_str());
        ++textp;
        بينما (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        إذا (!std::isdigit(*textp)) اكسر;  // Fail
        ثابت صحيح level = std::atoi(textp);
        إذا (level < 0 || level >= 3) اكسر;  // Fail

        linenoRef = سطر_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        إرجاع;
    } بينما (خطأ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

فراغ File_سطر::forwardToken(ثابت حرف* textp, size_t size, منطقي track_أسطر) {
    ساكن صحيح s_token_رقم = 1;
    m_token_رقم = s_token_رقم++;
    لكل (ثابت حرف* sp = textp; size && *sp; ++sp, --size) {
        إذا (*sp == '\n') {
            إذا (track_أسطر) linenoInc();
            m_lastColumn = 1;
        } وإلا إذا (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } وإلا {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

فراغ File_سطر::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(هذا);  // Toggle warnings based on global config file
#endif
}

File_سطر* File_سطر::copy_Or_Same_File_سطر_Applied() {
    applyIgnores();
    إرجاع copy_Or_Same_File_سطر();
}

File_سطر* File_سطر::copy_Or_Same_File_سطر() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ساكن File_سطر* s_lastNewp = nullptr;
    إذا (s_lastNewp && *s_lastNewp == *هذا) {  // Compares lineno, filename, etc
        إرجاع s_lastNewp;
    }
    File_سطر* ثابت newp = جديد File_سطر{هذا};
    s_lastNewp = newp;
    إرجاع newp;
}

string File_سطر::filebasename() ثابت VL_MT_SAFE { إرجاع V3Os::filenameNonDir(filename()); }

string File_سطر::filebasenameNoExt() ثابت { إرجاع V3Os::filenameNonDirExt(filename()); }

string File_سطر::firstColumnLetters() ثابت VL_MT_SAFE {
    ثابت حرف a = ((firstColumn() / 26) % 26) + 'a';
    ثابت حرف b = (firstColumn() % 26) + 'a';
    إرجاع string(1, a) + string(1, b);
}

string File_سطر::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    بينما ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    إرجاع name;
}

string File_سطر::ascii_سطر_Col() ثابت {
    إرجاع (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_سطر::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    إرجاع filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_سطر* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    إرجاع (os);
}

string File_سطر::warnOffParse(ثابت string& msgs, منطقي turnOff) {
    string result;
    لكل (ثابت string& رسالة : VString::split(msgs, ',')) {
        ثابت V3ErrorCode code{رسالة};
        إذا (!code.hardError()) {
            warnOff(code, turnOff);
            استمر;
        }
        // Error if not suppressed
        إذا (!v3Global.opt.isFuture(رسالة)) result = VString::dot(result, ",", رسالة);
    }
    إرجاع result;
}

منطقي File_سطر::warnIsOff(V3ErrorCode code) ثابت {
    إذا (!رسالة_En().enabled(code)) إرجاع true;
    إذا (!default_File_سطر().رسالة_En().enabled(code)) إرجاع true;  // Global overrides local
    إرجاع خطأ;
}

// cppverilator-suppress constParameter
فراغ File_سطر::v3errorEnd(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    إذا (lastLineno()) nsstr << V3Error::warnContextBegin() << هذا << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    إذا (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    إذا (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } وإلا {
        إذا (!V3Error::s().errorContexted()) {
            ثابت string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ثابت string waiverText = V3Error::stripMetaText(wsstr.str(), خطأ);
        m_waive = V3Control::waive(هذا, V3Error::s().errorCode(), waiverText);
        إذا (m_waive) {
            V3Error::s().suppressThisWarning();
        } وإلا {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), هذا);
}

string File_سطر::warnMore() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (lastLineno()) {
        إرجاع V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } وإلا {
        إرجاع V3Error::warnMore();
    }
}
string File_سطر::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (lastLineno()) {
        إرجاع V3Error::s().warnRelated(هذا) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } وإلا {
        إرجاع V3Error::warnMore();
    }
};

string File_سطر::source() ثابت VL_MT_SAFE {
    إذا (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        إذا (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            إرجاع "\n%Error: internal tracking of file contents failed";
        } وإلا {
            إرجاع "";
        }
    }  // LCOV_EXCL_STOP
    إرجاع m_contentp->get_سطر(m_contentLineno);
}
string File_سطر::sourcePrefix(صحيح toColumn) ثابت VL_MT_SAFE {
    ثابت std::string src = source();
    إذا (toColumn > static_cast<صحيح>(src.length())) toColumn = static_cast<صحيح>(src.length());
    إذا (toColumn < 1) إرجاع "";
    إرجاع src.substr(0, toColumn - 1);
}
string File_سطر::prettySource() ثابت VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ثابت string::size_type pos = out.find('\n');
    إذا (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    إرجاع VString::spaceUnprintable(out);
}

string File_سطر::warnContext() ثابت {
    إذا (!v3Global.opt.context()) إرجاع "";
    إذا (firstLineno() == lastLineno() && firstColumn()) {
        ثابت string source_سطر = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        إذا (!source_سطر.empty() && source_سطر.length() < SHOW_SOURCE_MAX_LENGTH
            && source_سطر.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            بينما (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_سطر + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            إذا (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        إرجاع V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    إرجاع "";
}

string File_سطر::warnContextParent() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    لكل (File_سطر* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        إذا (parentFl->filenameIsGlobal()) اكسر;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    إرجاع result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_سطر*> file_سطر_Leak_Checks;

فراغ* File_سطر::operator جديد(size_t size) {
    File_سطر* ثابت objp = static_cast<File_سطر*>(::operator جديد(size));
    file_سطر_Leak_Checks.أدرج(objp);
    إرجاع objp;
}

فراغ File_سطر::operator delete(فراغ* objp, size_t size) {
    إذا (!objp) إرجاع;
    File_سطر* ثابت flp = static_cast<File_سطر*>(objp);
    ثابت auto it = file_سطر_Leak_Checks.find(flp);
    إذا (it != file_سطر_Leak_Checks.end()) {
        file_سطر_Leak_Checks.erase(it);
    } وإلا {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

فراغ File_سطر::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_رسالة_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

فراغ File_سطر::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    بينما (true) {
        ثابت auto it = file_سطر_Leak_Checks.begin();
        إذا (it == file_سطر_Leak_Checks.end()) اكسر;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_سطر_Leak_Checks.clear();
    singleton().clear();
#endif
}
