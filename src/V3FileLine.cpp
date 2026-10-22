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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

// clang-format off
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3Os.h"
#অন্তর্ভাব "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#অন্তর্ভাব "V3Waiver.h"
// clang-format on

#অন্তর্ভাব <algorithm>
#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <limits>
#অন্তর্ভাব <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_সারি_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr পূর্ণসংখ্যা আকার_2
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    অক্ষর out[আকার_2];
    অক্ষর* op = out + আকার_2 - 1;
    *--op = '\0';  // We build backwards
    পূর্ণসংখ্যা অঙ্ক = fileno;
    করো {
        *--op = 'a' + অঙ্ক % 26;
        অঙ্ক /= 26;
    } যখন (অঙ্ক);
    ফেরত op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_সারি_Singleton::fileNameIdx_t File_সারি_Singleton::name_To_সংখ্যা(ধ্রুবক string& filename) {
    ধ্রুবক auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    যদি (pair.second) {
        ধ্রুবক আকার_t পরবর্তী_Idx = m_names.আকার_2();
        UASSERT(পরবর্তী_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(পরবর্তী_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(পরবর্তী_Idx);
        m_names.ঠেলো_back(filename);
        m_languages.ঠেলো_back(V3LangCode::mostRecent());
    }
    ফেরত idx;
}

শূন্য File_সারি_Singleton::file_Name_অঙ্ক_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    জন্য (ধ্রুবক auto& itr : m_namemap) {
        std::string filename = itr.first;
        যদি (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } অন্যথা যদি (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << সংখ্যা_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::add_বার্তা_En_Bit_Set(ধ্রুবক বার্তা_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ধ্রুবক V3LockGuard lock{m_mutex};
    ধ্রুবক auto pair = m_interned_বার্তা_En_Idxs.emplace(bitSet, 0);
    বার্তা_En_Set_Idx_t& idx = pair.first->second;
    যদি (pair.second) {
        ধ্রুবক আকার_t পরবর্তী_Idx = m_interned_বার্তা_Ens.আকার_2();
        UASSERT(পরবর্তী_Idx <= std::numeric_limits<বার্তা_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(পরবর্তী_Idx) + "+).");
        idx = static_cast<বার্তা_En_Set_Idx_t>(পরবর্তী_Idx);
        m_interned_বার্তা_Ens.ঠেলো_back(bitSet);
    }
    ফেরত idx;
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::default_বার্তা_En_সূচক() VL_MT_SAFE {
    বার্তা_En_Bit_Set বার্তা_En_Bit_Set_2;
    জন্য (পূর্ণসংখ্যা i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ধ্রুবক V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        বার্তা_En_Bit_Set_2.set(বার্তা_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        বার্তা_En_Bit_Set_2.set(বার্তা_En_Bit_Set::Subset::CTRL, code, সত্য);
    }
    ফেরত add_বার্তা_En_Bit_Set(বার্তা_En_Bit_Set_2);
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::বার্তা_En_Set_Bit(বার্তা_En_Set_Idx_t setIdx,
                                                                বার্তা_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, যৌক্তিক মান) {
    // See if state matches existing
    যৌক্তিক same = সত্য;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        যদি (বার্তা_En(setIdx).test(subset, subcode) != মান) same = মিথ্যা;
    });
    যদি (same) ফেরত setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    বার্তা_En_Bit_Set বার্তা_En_Bit_Set_2{বার্তা_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { বার্তা_En_Bit_Set_2.set(subset, subcode, মান); });
    ফেরত add_বার্তা_En_Bit_Set(বার্তা_En_Bit_Set_2);
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::বার্তা_Set_Ctrl_Bit_Set(বার্তা_En_Set_Idx_t setIdx,
                                                                     ধ্রুবক VErrorBitSet& bitset) {
    ধ্রুবক বার্তা_En_Bit_Set::Subset subset = বার্তা_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    যদি (বার্তা_En(setIdx).getAll(subset) == bitset) ফেরত setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    বার্তা_En_Bit_Set বার্তা_En_Bit_Set_2{বার্তা_En(setIdx)};
    বার্তা_En_Bit_Set_2.setAll(subset, bitset);
    ফেরত add_বার্তা_En_Bit_Set(বার্তা_En_Bit_Set_2);
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::বার্তা_En_And(বার্তা_En_Set_Idx_t lhsIdx,
                                                             বার্তা_En_Set_Idx_t rhsIdx) {
    ধ্রুবক বার্তা_En_Bit_Set& lhs = বার্তা_En(lhsIdx);
    ধ্রুবক বার্তা_En_Bit_Set& rhs = বার্তা_En(rhsIdx);
    ধ্রুবক বার্তা_En_Bit_Set intersection{lhs, rhs};
    যদি (intersection == lhs) ফেরত lhsIdx;
    যদি (intersection == rhs) ফেরত rhsIdx;
    ফেরত add_বার্তা_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

শূন্য VFileContent::ঠেলো_Text(ধ্রুবক string& text) {
    // Similar code in WildcardContents::pushText()
    যদি (m_সারিগুলি.আকার_2() == 0) {
        m_সারিগুলি.emplace_back("");  // no such thing as line [0]
        m_সারিগুলি.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ধ্রুবক string leftover = m_সারিগুলি.back() + text;
    m_সারিগুলি.বের_করো_back();

    // Insert line-by-line
    string::আকার_type সারি_start = 0;
    যখন (সত্য) {
        ধ্রুবক string::আকার_type সারি_end = leftover.find('\n', সারি_start);
        যদি (সারি_end != string::npos) {
            ধ্রুবক string oneline(leftover, সারি_start, সারি_end - সারি_start + 1);
            m_সারিগুলি.ঠেলো_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_সারিগুলি.আকার_2() - 1) << "]: " << oneline);
            সারি_start = সারি_end + 1;
        } অন্যথা {
            বিরতি;
        }
    }
    // Keep leftover for next time
    m_সারিগুলি.emplace_back(string(leftover, সারি_start));  // Might be ""
}

string VFileContent::get_সারি(পূর্ণসংখ্যা lineno) ধ্রুবক VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    যদি (VL_UNCOVERABLE(lineno < 0 || lineno >= (পূর্ণসংখ্যা)m_সারিগুলি.আকার_2())) {
        যদি (debug() || v3Global.opt.debugCheck()) {
            ফেরত ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } অন্যথা {
            ফেরত "";
        }
    }
    string text = m_সারিগুলি[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    ফেরত text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    যদি (!contentp) {
        os << "ct0";
    } অন্যথা {
        os << contentp->ascii();
    }
    ফেরত os;
}

// ######################################################################
//  FileLine class functions

File_সারি::~File_সারি() {
    যদি (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

শূন্য File_সারি::newContent() {
    যদি (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = নতুন VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_সারি::সারি_Directive_Strg(পূর্ণসংখ্যা enterExit) ধ্রুবক {
    ফেরত "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

শূন্য File_সারি::সারি_Directive(ধ্রুবক অক্ষর* textp, পূর্ণসংখ্যা& enterExitRef) {
    string newFilename;
    পূর্ণসংখ্যা newLineno = -1;
    সারি_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    যদি (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } অন্যথা {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

শূন্য File_সারি::সারি_Directive_Parse(ধ্রুবক অক্ষর* textp, string& filenameRef, পূর্ণসংখ্যা& linenoRef,
                                  পূর্ণসংখ্যা& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    করো {
        // Skip `line
        যখন (*textp && std::isspace(*textp)) ++textp;
        যখন (*textp && !std::isspace(*textp)) ++textp;
        যখন (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        পূর্ণসংখ্যা সারি_No;
        ধ্রুবক অক্ষর* ধ্রুবক ln = textp;
        যখন (*textp && !std::isspace(*textp)) ++textp;
        যদি (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            সারি_No = lineno() + 1;
        } অন্যথা যদি (std::isdigit(*ln)) {
            সারি_No = std::atoi(ln);
        } অন্যথা {
            বিরতি;  // Fail
        }
        যখন (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        যদি (*textp != '"') বিরতি;  // Fail
        ধ্রুবক অক্ষর* ধ্রুবক fn = ++textp;
        যখন (*textp && *textp != '"') ++textp;
        যদি (*textp != '"') বিরতি;  // Fail
        string err_বার্তা;
        ধ্রুবক string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_বার্তা);
        যদি (!err_বার্তা.empty()) এই->v3error(err_বার্তা.c_str());
        ++textp;
        যখন (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        যদি (!std::isdigit(*textp)) বিরতি;  // Fail
        ধ্রুবক পূর্ণসংখ্যা level = std::atoi(textp);
        যদি (level < 0 || level >= 3) বিরতি;  // Fail

        linenoRef = সারি_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        ফেরত;
    } যখন (মিথ্যা);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

শূন্য File_সারি::forwardToken(ধ্রুবক অক্ষর* textp, আকার_t আকার_2, যৌক্তিক track_সারিগুলি) {
    স্থির পূর্ণসংখ্যা s_token_অঙ্ক = 1;
    m_token_অঙ্ক = s_token_অঙ্ক++;
    জন্য (ধ্রুবক অক্ষর* sp = textp; আকার_2 && *sp; ++sp, --আকার_2) {
        যদি (*sp == '\n') {
            যদি (track_সারিগুলি) linenoInc();
            m_lastColumn = 1;
        } অন্যথা যদি (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } অন্যথা {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

শূন্য File_সারি::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(এই);  // Toggle warnings based on global config file
#endif
}

File_সারি* File_সারি::copy_Or_Same_File_সারি_Applied() {
    applyIgnores();
    ফেরত copy_Or_Same_File_সারি();
}

File_সারি* File_সারি::copy_Or_Same_File_সারি() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    স্থির File_সারি* s_lastNewp = nullptr;
    যদি (s_lastNewp && *s_lastNewp == *এই) {  // Compares lineno, filename, etc
        ফেরত s_lastNewp;
    }
    File_সারি* ধ্রুবক newp = নতুন File_সারি{এই};
    s_lastNewp = newp;
    ফেরত newp;
}

string File_সারি::filebasename() ধ্রুবক VL_MT_SAFE { ফেরত V3Os::filenameNonDir(filename()); }

string File_সারি::filebasenameNoExt() ধ্রুবক { ফেরত V3Os::filenameNonDirExt(filename()); }

string File_সারি::firstColumnLetters() ধ্রুবক VL_MT_SAFE {
    ধ্রুবক অক্ষর a = ((firstColumn() / 26) % 26) + 'a';
    ধ্রুবক অক্ষর b = (firstColumn() % 26) + 'a';
    ফেরত string(1, a) + string(1, b);
}

string File_সারি::profileFuncname() ধ্রুবক {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::আকার_type pos;
    যখন ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    ফেরত name;
}

string File_সারি::ascii_সারি_Col() ধ্রুবক {
    ফেরত (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_সারি::ascii() ধ্রুবক {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    ফেরত filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_সারি* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    ফেরত (os);
}

string File_সারি::warnOffParse(ধ্রুবক string& msgs, যৌক্তিক turnOff) {
    string ফলাফল;
    জন্য (ধ্রুবক string& বার্তা : VString::split(msgs, ',')) {
        ধ্রুবক V3ErrorCode code{বার্তা};
        যদি (!code.hardError()) {
            warnOff(code, turnOff);
            চালু;
        }
        // Error if not suppressed
        যদি (!v3Global.opt.isFuture(বার্তা)) ফলাফল = VString::dot(ফলাফল, ",", বার্তা);
    }
    ফেরত ফলাফল;
}

যৌক্তিক File_সারি::warnIsOff(V3ErrorCode code) ধ্রুবক {
    যদি (!বার্তা_En().enabled(code)) ফেরত সত্য;
    যদি (!default_File_সারি().বার্তা_En().enabled(code)) ফেরত সত্য;  // Global overrides local
    ফেরত মিথ্যা;
}

// cppverilator-suppress constParameter
শূন্য File_সারি::v3errorEnd(std::ostringstream& sstr, ধ্রুবক string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    যদি (lastLineno()) nsstr << V3Error::warnContextBegin() << এই << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    যদি (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().দৈর্ঘ্য()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    যদি (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } অন্যথা {
        যদি (!V3Error::s().errorContexted()) {
            ধ্রুবক string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ধ্রুবক string waiverText = V3Error::stripMetaText(wsstr.str(), মিথ্যা);
        m_waive = V3Control::waive(এই, V3Error::s().errorCode(), waiverText);
        যদি (m_waive) {
            V3Error::s().suppressThisWarning();
        } অন্যথা {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), এই);
}

string File_সারি::warnMore() ধ্রুবক VL_REQUIRES(V3Error::s().m_mutex) {
    যদি (lastLineno()) {
        ফেরত V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().আকার_2(), ' ')
               + ": " + V3Error::warnContextEnd();
    } অন্যথা {
        ফেরত V3Error::warnMore();
    }
}
string File_সারি::warnOther() ধ্রুবক VL_REQUIRES(V3Error::s().m_mutex) {
    যদি (lastLineno()) {
        ফেরত V3Error::s().warnRelated(এই) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } অন্যথা {
        ফেরত V3Error::warnMore();
    }
};

string File_সারি::source() ধ্রুবক VL_MT_SAFE {
    যদি (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        যদি (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            ফেরত "\n%Error: internal tracking of file contents failed";
        } অন্যথা {
            ফেরত "";
        }
    }  // LCOV_EXCL_STOP
    ফেরত m_contentp->get_সারি(m_contentLineno);
}
string File_সারি::sourcePrefix(পূর্ণসংখ্যা toColumn) ধ্রুবক VL_MT_SAFE {
    ধ্রুবক std::string src = source();
    যদি (toColumn > static_cast<পূর্ণসংখ্যা>(src.দৈর্ঘ্য())) toColumn = static_cast<পূর্ণসংখ্যা>(src.দৈর্ঘ্য());
    যদি (toColumn < 1) ফেরত "";
    ফেরত src.substr(0, toColumn - 1);
}
string File_সারি::prettySource() ধ্রুবক VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ধ্রুবক string::আকার_type pos = out.find('\n');
    যদি (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    ফেরত VString::spaceUnprintable(out);
}

string File_সারি::warnContext() ধ্রুবক {
    যদি (!v3Global.opt.context()) ফেরত "";
    যদি (firstLineno() == lastLineno() && firstColumn()) {
        ধ্রুবক string source_সারি = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        যদি (!source_সারি.empty() && source_সারি.দৈর্ঘ্য() < SHOW_SOURCE_MAX_দৈর্ঘ্য
            && source_সারি.দৈর্ঘ্য() >= static_cast<আকার_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            যখন (linestr.আকার_2() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_সারি + '\n';
            out += std::string(linestr.আকার_2(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            যদি (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        ফেরত V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    ফেরত "";
}

string File_সারি::warnContextParent() ধ্রুবক VL_REQUIRES(V3Error::s().m_mutex) {
    string ফলাফল;
    জন্য (File_সারি* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        যদি (parentFl->filenameIsGlobal()) বিরতি;
        ফলাফল += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    ফেরত ফলাফল;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_সারি*> file_সারি_Leak_Checks;

শূন্য* File_সারি::operator নতুন(আকার_t আকার_2) {
    File_সারি* ধ্রুবক objp = static_cast<File_সারি*>(::operator নতুন(আকার_2));
    file_সারি_Leak_Checks.ঢোকাও(objp);
    ফেরত objp;
}

শূন্য File_সারি::operator delete(শূন্য* objp, আকার_t আকার_2) {
    যদি (!objp) ফেরত;
    File_সারি* ধ্রুবক flp = static_cast<File_সারি*>(objp);
    ধ্রুবক auto it = file_সারি_Leak_Checks.find(flp);
    যদি (it != file_সারি_Leak_Checks.end()) {
        file_সারি_Leak_Checks.erase(it);
    } অন্যথা {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

শূন্য File_সারি::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::add_Stat_যোগ("FileLines, Number of filenames",
                        singleton().m_names.আকার_2());  // Max m_filenameno
    V3Stats::add_Stat_যোগ("FileLines, Message enable sets",
                        singleton().m_interned_বার্তা_Ens.আকার_2());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

শূন্য File_সারি::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    যখন (সত্য) {
        ধ্রুবক auto it = file_সারি_Leak_Checks.begin();
        যদি (it == file_সারি_Leak_Checks.end()) বিরতি;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_সারি_Leak_Checks.clear();
    singleton().clear();
#endif
}
