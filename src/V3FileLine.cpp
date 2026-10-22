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

string File_لي_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr صحيح size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    حرف out[size];
    حرف* op = out + size - 1;
    *--op = '\0';  // We build backwards
    صحيح لمب = fileno;
    افعل {
        *--op = 'a' + لمب % 26;
        لمب /= 26;
    } طالما (لمب);
    أرجع op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_لي_Singleton::fileNameIdx_t File_لي_Singleton::name_To_لمب(ثابت string& filename) {
    ثابت auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    إذا (pair.second) {
        ثابت size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.تور_back(filename);
        m_languages.تور_back(V3LangCode::mostRecent());
    }
    أرجع idx;
}

فراغ File_لي_Singleton::file_Name_لمب_Map_Dump_Json(std::ostream& os) {
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
           << ", \"language\":\"" << لمب_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_لي_Singleton::سكو_En_Set_Idx_t File_لي_Singleton::add_سكو_En_Bit_Set(ثابت سكو_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ثابت V3LockGuard lock{m_mutex};
    ثابت auto pair = m_interned_سكو_En_Idxs.emplace(bitSet, 0);
    سكو_En_Set_Idx_t& idx = pair.first->second;
    إذا (pair.second) {
        ثابت size_t nextIdx = m_interned_سكو_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<سكو_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<سكو_En_Set_Idx_t>(nextIdx);
        m_interned_سكو_Ens.تور_back(bitSet);
    }
    أرجع idx;
}

File_لي_Singleton::سكو_En_Set_Idx_t File_لي_Singleton::default_سكو_En_Index() VL_MT_SAFE {
    سكو_En_Bit_Set سكو_En_Bit_Set_2;
    لكل (صحيح i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ثابت V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        سكو_En_Bit_Set_2.set(سكو_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        سكو_En_Bit_Set_2.set(سكو_En_Bit_Set::Subset::CTRL, code, true);
    }
    أرجع add_سكو_En_Bit_Set(سكو_En_Bit_Set_2);
}

File_لي_Singleton::سكو_En_Set_Idx_t File_لي_Singleton::سكو_En_Set_Bit(سكو_En_Set_Idx_t setIdx,
                                                                سكو_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, منطقي درج) {
    // See if state matches existing
    منطقي same = true;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        إذا (سكو_En(setIdx).test(subset, subcode) != درج) same = خطأ;
    });
    إذا (same) أرجع setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    سكو_En_Bit_Set سكو_En_Bit_Set_2{سكو_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { سكو_En_Bit_Set_2.set(subset, subcode, درج); });
    أرجع add_سكو_En_Bit_Set(سكو_En_Bit_Set_2);
}

File_لي_Singleton::سكو_En_Set_Idx_t File_لي_Singleton::سكو_Set_Ctrl_Bit_Set(سكو_En_Set_Idx_t setIdx,
                                                                     ثابت VErrorBitSet& bitset) {
    ثابت سكو_En_Bit_Set::Subset subset = سكو_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    إذا (سكو_En(setIdx).getAll(subset) == bitset) أرجع setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    سكو_En_Bit_Set سكو_En_Bit_Set_2{سكو_En(setIdx)};
    سكو_En_Bit_Set_2.setAll(subset, bitset);
    أرجع add_سكو_En_Bit_Set(سكو_En_Bit_Set_2);
}

File_لي_Singleton::سكو_En_Set_Idx_t File_لي_Singleton::سكو_En_And(سكو_En_Set_Idx_t lhsIdx,
                                                             سكو_En_Set_Idx_t rhsIdx) {
    ثابت سكو_En_Bit_Set& lhs = سكو_En(lhsIdx);
    ثابت سكو_En_Bit_Set& rhs = سكو_En(rhsIdx);
    ثابت سكو_En_Bit_Set intersection{lhs, rhs};
    إذا (intersection == lhs) أرجع lhsIdx;
    إذا (intersection == rhs) أرجع rhsIdx;
    أرجع add_سكو_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

فراغ VFileContent::تور_Text(ثابت string& text) {
    // Similar code in WildcardContents::pushText()
    إذا (m_ليوك.size() == 0) {
        m_ليوك.emplace_back("");  // no such thing as line [0]
        m_ليوك.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_ليوك.back() + text;
    m_ليوك.چرو_back();

    // Insert line-by-line
    string::size_type لي_start = 0;
    طالما (true) {
        ثابت string::size_type لي_end = leftover.find('\n', لي_start);
        إذا (لي_end != string::npos) {
            ثابت string oneline(leftover, لي_start, لي_end - لي_start + 1);
            m_ليوك.تور_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_ليوك.size() - 1) << "]: " << oneline);
            لي_start = لي_end + 1;
        } وإلا {
            اكسر;
        }
    }
    // Keep leftover for next time
    m_ليوك.emplace_back(string(leftover, لي_start));  // Might be ""
}

string VFileContent::get_لي(صحيح lineno) ثابت VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    إذا (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحيح)m_ليوك.size())) {
        إذا (debug() || v3Global.opt.debugCheck()) {
            أرجع ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } وإلا {
            أرجع "";
        }
    }
    string text = m_ليوك[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    أرجع text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    إذا (!contentp) {
        os << "ct0";
    } وإلا {
        os << contentp->ascii();
    }
    أرجع os;
}

// ######################################################################
//  FileLine class functions

File_لي::~File_لي() {
    إذا (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

فراغ File_لي::newContent() {
    إذا (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = جديد VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_لي::لي_Directive_Strg(صحيح enterExit) ثابت {
    أرجع "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

فراغ File_لي::لي_Directive(ثابت حرف* textp, صحيح& enterExitRef) {
    string newFilename;
    صحيح newLineno = -1;
    لي_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    إذا (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } وإلا {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

فراغ File_لي::لي_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                                  صحيح& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    افعل {
        // Skip `line
        طالما (*textp && std::isspace(*textp)) ++textp;
        طالما (*textp && !std::isspace(*textp)) ++textp;
        طالما (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        صحيح لي_No;
        ثابت حرف* ثابت ln = textp;
        طالما (*textp && !std::isspace(*textp)) ++textp;
        إذا (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            لي_No = lineno() + 1;
        } وإلا إذا (std::isdigit(*ln)) {
            لي_No = std::atoi(ln);
        } وإلا {
            اكسر;  // Fail
        }
        طالما (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        إذا (*textp != '"') اكسر;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        طالما (*textp && *textp != '"') ++textp;
        إذا (*textp != '"') اكسر;  // Fail
        string err_سكو;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_سكو);
        إذا (!err_سكو.empty()) هذا->v3error(err_سكو.c_str());
        ++textp;
        طالما (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        إذا (!std::isdigit(*textp)) اكسر;  // Fail
        ثابت صحيح level = std::atoi(textp);
        إذا (level < 0 || level >= 3) اكسر;  // Fail

        linenoRef = لي_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        أرجع;
    } طالما (خطأ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

فراغ File_لي::forwardToken(ثابت حرف* textp, size_t size, منطقي track_ليوك) {
    ساكن صحيح s_token_لمب = 1;
    m_token_لمب = s_token_لمب++;
    لكل (ثابت حرف* sp = textp; size && *sp; ++sp, --size) {
        إذا (*sp == '\n') {
            إذا (track_ليوك) linenoInc();
            m_lastColumn = 1;
        } وإلا إذا (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } وإلا {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

فراغ File_لي::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(هذا);  // Toggle warnings based on global config file
#endif
}

File_لي* File_لي::copy_Or_Same_File_لي_Applied() {
    applyIgnores();
    أرجع copy_Or_Same_File_لي();
}

File_لي* File_لي::copy_Or_Same_File_لي() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ساكن File_لي* s_lastNewp = nullptr;
    إذا (s_lastNewp && *s_lastNewp == *هذا) {  // Compares lineno, filename, etc
        أرجع s_lastNewp;
    }
    File_لي* ثابت newp = جديد File_لي{هذا};
    s_lastNewp = newp;
    أرجع newp;
}

string File_لي::filebasename() ثابت VL_MT_SAFE { أرجع V3Os::filenameNonDir(filename()); }

string File_لي::filebasenameNoExt() ثابت { أرجع V3Os::filenameNonDirExt(filename()); }

string File_لي::firstColumnLetters() ثابت VL_MT_SAFE {
    ثابت حرف a = ((firstColumn() / 26) % 26) + 'a';
    ثابت حرف b = (firstColumn() % 26) + 'a';
    أرجع string(1, a) + string(1, b);
}

string File_لي::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    طالما ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    أرجع name;
}

string File_لي::ascii_لي_Col() ثابت {
    أرجع (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_لي::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    أرجع filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_لي* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    أرجع (os);
}

string File_لي::warnOffParse(ثابت string& msgs, منطقي turnOff) {
    string result;
    لكل (ثابت string& سكو : VString::split(msgs, ',')) {
        ثابت V3ErrorCode code{سكو};
        إذا (!code.hardError()) {
            warnOff(code, turnOff);
            استمر;
        }
        // Error if not suppressed
        إذا (!v3Global.opt.isFuture(سكو)) result = VString::dot(result, ",", سكو);
    }
    أرجع result;
}

منطقي File_لي::warnIsOff(V3ErrorCode code) ثابت {
    إذا (!سكو_En().enabled(code)) أرجع true;
    إذا (!default_File_لي().سكو_En().enabled(code)) أرجع true;  // Global overrides local
    أرجع خطأ;
}

// cppverilator-suppress constParameter
فراغ File_لي::v3errorEnd(std::ostringstream& sstr, ثابت string& extra)
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

string File_لي::warnMore() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (lastLineno()) {
        أرجع V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } وإلا {
        أرجع V3Error::warnMore();
    }
}
string File_لي::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    إذا (lastLineno()) {
        أرجع V3Error::s().warnRelated(هذا) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } وإلا {
        أرجع V3Error::warnMore();
    }
};

string File_لي::source() ثابت VL_MT_SAFE {
    إذا (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        إذا (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            أرجع "\n%Error: internal tracking of file contents failed";
        } وإلا {
            أرجع "";
        }
    }  // LCOV_EXCL_STOP
    أرجع m_contentp->get_لي(m_contentLineno);
}
string File_لي::sourcePrefix(صحيح toColumn) ثابت VL_MT_SAFE {
    ثابت std::string src = source();
    إذا (toColumn > static_cast<صحيح>(src.length())) toColumn = static_cast<صحيح>(src.length());
    إذا (toColumn < 1) أرجع "";
    أرجع src.substr(0, toColumn - 1);
}
string File_لي::prettySource() ثابت VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ثابت string::size_type pos = out.find('\n');
    إذا (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    أرجع VString::spaceUnprintable(out);
}

string File_لي::warnContext() ثابت {
    إذا (!v3Global.opt.context()) أرجع "";
    إذا (firstLineno() == lastLineno() && firstColumn()) {
        ثابت string source_لي = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        إذا (!source_لي.empty() && source_لي.length() < SHOW_SOURCE_MAX_LENGTH
            && source_لي.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            طالما (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_لي + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            إذا (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        أرجع V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    أرجع "";
}

string File_لي::warnContextParent() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    لكل (File_لي* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        إذا (parentFl->filenameIsGlobal()) اكسر;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    أرجع result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_لي*> file_لي_Leak_Checks;

فراغ* File_لي::operator جديد(size_t size) {
    File_لي* ثابت objp = static_cast<File_لي*>(::operator جديد(size));
    file_لي_Leak_Checks.سك(objp);
    أرجع objp;
}

فراغ File_لي::operator delete(فراغ* objp, size_t size) {
    إذا (!objp) أرجع;
    File_لي* ثابت flp = static_cast<File_لي*>(objp);
    ثابت auto it = file_لي_Leak_Checks.find(flp);
    إذا (it != file_لي_Leak_Checks.end()) {
        file_لي_Leak_Checks.erase(it);
    } وإلا {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

فراغ File_لي::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_سكو_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

فراغ File_لي::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    طالما (true) {
        ثابت auto it = file_لي_Leak_Checks.begin();
        إذا (it == file_لي_Leak_Checks.end()) اكسر;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_لي_Leak_Checks.clear();
    singleton().clear();
#endif
}
