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
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#شامل "V3Waiver.h"
// clang-format on

#شامل <algorithm>
#شامل <iomanip>
#شامل <limits>
#شامل <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_سطر_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr صحیح size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    نویسه out[size];
    نویسه* op = out + size - 1;
    *--op = '\0';  // We build backwards
    صحیح رقم = fileno;
    انجامبده {
        *--op = 'a' + رقم % 26;
        رقم /= 26;
    } وقتی (رقم);
    بازگشت op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_سطر_Singleton::fileNameIdx_t File_سطر_Singleton::name_To_عدد(ثابت string& filename) {
    ثابت auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    اگر (pair.second) {
        ثابت size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.افزودن_back(filename);
        m_languages.افزودن_back(V3LangCode::mostRecent());
    }
    بازگشت idx;
}

تهی File_سطر_Singleton::file_Name_رقم_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    برای (ثابت auto& itr : m_namemap) {
        std::string filename = itr.first;
        اگر (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } وگرنه اگر (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << عدد_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_سطر_Singleton::پیام_En_Set_Idx_t File_سطر_Singleton::add_پیام_En_Bit_Set(ثابت پیام_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ثابت V3LockGuard lock{m_mutex};
    ثابت auto pair = m_interned_پیام_En_Idxs.emplace(bitSet, 0);
    پیام_En_Set_Idx_t& idx = pair.first->second;
    اگر (pair.second) {
        ثابت size_t nextIdx = m_interned_پیام_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<پیام_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<پیام_En_Set_Idx_t>(nextIdx);
        m_interned_پیام_Ens.افزودن_back(bitSet);
    }
    بازگشت idx;
}

File_سطر_Singleton::پیام_En_Set_Idx_t File_سطر_Singleton::default_پیام_En_Index() VL_MT_SAFE {
    پیام_En_Bit_Set پیام_En_Bit_Set_2;
    برای (صحیح i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ثابت V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        پیام_En_Bit_Set_2.set(پیام_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        پیام_En_Bit_Set_2.set(پیام_En_Bit_Set::Subset::CTRL, code, درست);
    }
    بازگشت add_پیام_En_Bit_Set(پیام_En_Bit_Set_2);
}

File_سطر_Singleton::پیام_En_Set_Idx_t File_سطر_Singleton::پیام_En_Set_Bit(پیام_En_Set_Idx_t setIdx,
                                                                پیام_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, منطقی مقدار) {
    // See if state matches existing
    منطقی same = درست;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        اگر (پیام_En(setIdx).test(subset, subcode) != مقدار) same = نادرست;
    });
    اگر (same) بازگشت setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیام_En_Bit_Set پیام_En_Bit_Set_2{پیام_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { پیام_En_Bit_Set_2.set(subset, subcode, مقدار); });
    بازگشت add_پیام_En_Bit_Set(پیام_En_Bit_Set_2);
}

File_سطر_Singleton::پیام_En_Set_Idx_t File_سطر_Singleton::پیام_Set_Ctrl_Bit_Set(پیام_En_Set_Idx_t setIdx,
                                                                     ثابت VErrorBitSet& bitset) {
    ثابت پیام_En_Bit_Set::Subset subset = پیام_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    اگر (پیام_En(setIdx).getAll(subset) == bitset) بازگشت setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    پیام_En_Bit_Set پیام_En_Bit_Set_2{پیام_En(setIdx)};
    پیام_En_Bit_Set_2.setAll(subset, bitset);
    بازگشت add_پیام_En_Bit_Set(پیام_En_Bit_Set_2);
}

File_سطر_Singleton::پیام_En_Set_Idx_t File_سطر_Singleton::پیام_En_And(پیام_En_Set_Idx_t lhsIdx,
                                                             پیام_En_Set_Idx_t rhsIdx) {
    ثابت پیام_En_Bit_Set& lhs = پیام_En(lhsIdx);
    ثابت پیام_En_Bit_Set& rhs = پیام_En(rhsIdx);
    ثابت پیام_En_Bit_Set intersection{lhs, rhs};
    اگر (intersection == lhs) بازگشت lhsIdx;
    اگر (intersection == rhs) بازگشت rhsIdx;
    بازگشت add_پیام_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

تهی VFileContent::افزودن_Text(ثابت string& text) {
    // Similar code in WildcardContents::pushText()
    اگر (m_سطرها.size() == 0) {
        m_سطرها.emplace_back("");  // no such thing as line [0]
        m_سطرها.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_سطرها.back() + text;
    m_سطرها.برداشت_back();

    // Insert line-by-line
    string::size_type سطر_start = 0;
    وقتی (درست) {
        ثابت string::size_type سطر_end = leftover.find('\n', سطر_start);
        اگر (سطر_end != string::npos) {
            ثابت string oneline(leftover, سطر_start, سطر_end - سطر_start + 1);
            m_سطرها.افزودن_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_سطرها.size() - 1) << "]: " << oneline);
            سطر_start = سطر_end + 1;
        } وگرنه {
            بشکن;
        }
    }
    // Keep leftover for next time
    m_سطرها.emplace_back(string(leftover, سطر_start));  // Might be ""
}

string VFileContent::get_سطر(صحیح lineno) ثابت VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    اگر (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحیح)m_سطرها.size())) {
        اگر (debug() || v3Global.opt.debugCheck()) {
            بازگشت ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } وگرنه {
            بازگشت "";
        }
    }
    string text = m_سطرها[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    بازگشت text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    اگر (!contentp) {
        os << "ct0";
    } وگرنه {
        os << contentp->ascii();
    }
    بازگشت os;
}

// ######################################################################
//  FileLine class functions

File_سطر::~File_سطر() {
    اگر (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

تهی File_سطر::newContent() {
    اگر (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = جدید VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_سطر::سطر_Directive_Strg(صحیح enterExit) ثابت {
    بازگشت "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

تهی File_سطر::سطر_Directive(ثابت نویسه* textp, صحیح& enterExitRef) {
    string newFilename;
    صحیح newLineno = -1;
    سطر_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    اگر (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } وگرنه {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

تهی File_سطر::سطر_Directive_Parse(ثابت نویسه* textp, string& filenameRef, صحیح& linenoRef,
                                  صحیح& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    انجامبده {
        // Skip `line
        وقتی (*textp && std::isspace(*textp)) ++textp;
        وقتی (*textp && !std::isspace(*textp)) ++textp;
        وقتی (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        صحیح سطر_No;
        ثابت نویسه* ثابت ln = textp;
        وقتی (*textp && !std::isspace(*textp)) ++textp;
        اگر (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            سطر_No = lineno() + 1;
        } وگرنه اگر (std::isdigit(*ln)) {
            سطر_No = std::atoi(ln);
        } وگرنه {
            بشکن;  // Fail
        }
        وقتی (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        اگر (*textp != '"') بشکن;  // Fail
        ثابت نویسه* ثابت fn = ++textp;
        وقتی (*textp && *textp != '"') ++textp;
        اگر (*textp != '"') بشکن;  // Fail
        string err_پیام;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_پیام);
        اگر (!err_پیام.empty()) این->v3error(err_پیام.c_str());
        ++textp;
        وقتی (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        اگر (!std::isdigit(*textp)) بشکن;  // Fail
        ثابت صحیح level = std::atoi(textp);
        اگر (level < 0 || level >= 3) بشکن;  // Fail

        linenoRef = سطر_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        بازگشت;
    } وقتی (نادرست);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

تهی File_سطر::forwardToken(ثابت نویسه* textp, size_t size, منطقی track_سطرها) {
    ایستا صحیح s_token_رقم = 1;
    m_token_رقم = s_token_رقم++;
    برای (ثابت نویسه* sp = textp; size && *sp; ++sp, --size) {
        اگر (*sp == '\n') {
            اگر (track_سطرها) linenoInc();
            m_lastColumn = 1;
        } وگرنه اگر (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } وگرنه {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

تهی File_سطر::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(این);  // Toggle warnings based on global config file
#endif
}

File_سطر* File_سطر::copy_Or_Same_File_سطر_Applied() {
    applyIgnores();
    بازگشت copy_Or_Same_File_سطر();
}

File_سطر* File_سطر::copy_Or_Same_File_سطر() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ایستا File_سطر* s_lastNewp = nullptr;
    اگر (s_lastNewp && *s_lastNewp == *این) {  // Compares lineno, filename, etc
        بازگشت s_lastNewp;
    }
    File_سطر* ثابت newp = جدید File_سطر{این};
    s_lastNewp = newp;
    بازگشت newp;
}

string File_سطر::filebasename() ثابت VL_MT_SAFE { بازگشت V3Os::filenameNonDir(filename()); }

string File_سطر::filebasenameNoExt() ثابت { بازگشت V3Os::filenameNonDirExt(filename()); }

string File_سطر::firstColumnLetters() ثابت VL_MT_SAFE {
    ثابت نویسه a = ((firstColumn() / 26) % 26) + 'a';
    ثابت نویسه b = (firstColumn() % 26) + 'a';
    بازگشت string(1, a) + string(1, b);
}

string File_سطر::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    وقتی ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    بازگشت name;
}

string File_سطر::ascii_سطر_Col() ثابت {
    بازگشت (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_سطر::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    بازگشت filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_سطر* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    بازگشت (os);
}

string File_سطر::warnOffParse(ثابت string& msgs, منطقی turnOff) {
    string result;
    برای (ثابت string& پیام : VString::split(msgs, ',')) {
        ثابت V3ErrorCode code{پیام};
        اگر (!code.hardError()) {
            warnOff(code, turnOff);
            ادامه;
        }
        // Error if not suppressed
        اگر (!v3Global.opt.isFuture(پیام)) result = VString::dot(result, ",", پیام);
    }
    بازگشت result;
}

منطقی File_سطر::warnIsOff(V3ErrorCode code) ثابت {
    اگر (!پیام_En().enabled(code)) بازگشت درست;
    اگر (!default_File_سطر().پیام_En().enabled(code)) بازگشت درست;  // Global overrides local
    بازگشت نادرست;
}

// cppverilator-suppress constParameter
تهی File_سطر::v3errorEnd(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    اگر (lastLineno()) nsstr << V3Error::warnContextBegin() << این << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    اگر (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    اگر (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } وگرنه {
        اگر (!V3Error::s().errorContexted()) {
            ثابت string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ثابت string waiverText = V3Error::stripMetaText(wsstr.str(), نادرست);
        m_waive = V3Control::waive(این, V3Error::s().errorCode(), waiverText);
        اگر (m_waive) {
            V3Error::s().suppressThisWarning();
        } وگرنه {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), این);
}

string File_سطر::warnMore() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    اگر (lastLineno()) {
        بازگشت V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } وگرنه {
        بازگشت V3Error::warnMore();
    }
}
string File_سطر::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    اگر (lastLineno()) {
        بازگشت V3Error::s().warnRelated(این) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } وگرنه {
        بازگشت V3Error::warnMore();
    }
};

string File_سطر::source() ثابت VL_MT_SAFE {
    اگر (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        اگر (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            بازگشت "\n%Error: internal tracking of file contents failed";
        } وگرنه {
            بازگشت "";
        }
    }  // LCOV_EXCL_STOP
    بازگشت m_contentp->get_سطر(m_contentLineno);
}
string File_سطر::sourcePrefix(صحیح toColumn) ثابت VL_MT_SAFE {
    ثابت std::string src = source();
    اگر (toColumn > static_cast<صحیح>(src.length())) toColumn = static_cast<صحیح>(src.length());
    اگر (toColumn < 1) بازگشت "";
    بازگشت src.substr(0, toColumn - 1);
}
string File_سطر::prettySource() ثابت VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ثابت string::size_type pos = out.find('\n');
    اگر (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    بازگشت VString::spaceUnprintable(out);
}

string File_سطر::warnContext() ثابت {
    اگر (!v3Global.opt.context()) بازگشت "";
    اگر (firstLineno() == lastLineno() && firstColumn()) {
        ثابت string source_سطر = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        اگر (!source_سطر.empty() && source_سطر.length() < SHOW_SOURCE_MAX_LENGTH
            && source_سطر.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            وقتی (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_سطر + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            اگر (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        بازگشت V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    بازگشت "";
}

string File_سطر::warnContextParent() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    برای (File_سطر* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        اگر (parentFl->filenameIsGlobal()) بشکن;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    بازگشت result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_سطر*> file_سطر_Leak_Checks;

تهی* File_سطر::operator جدید(size_t size) {
    File_سطر* ثابت objp = static_cast<File_سطر*>(::operator جدید(size));
    file_سطر_Leak_Checks.درج(objp);
    بازگشت objp;
}

تهی File_سطر::operator delete(تهی* objp, size_t size) {
    اگر (!objp) بازگشت;
    File_سطر* ثابت flp = static_cast<File_سطر*>(objp);
    ثابت auto it = file_سطر_Leak_Checks.find(flp);
    اگر (it != file_سطر_Leak_Checks.end()) {
        file_سطر_Leak_Checks.erase(it);
    } وگرنه {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

تهی File_سطر::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_پیام_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

تهی File_سطر::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    وقتی (درست) {
        ثابت auto it = file_سطر_Leak_Checks.begin();
        اگر (it == file_سطر_Leak_Checks.end()) بشکن;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_سطر_Leak_Checks.clear();
    singleton().clear();
#endif
}
