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
    constexpr পূর্ণাঙ্ক size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    বর্ণ out[size];
    বর্ণ* op = out + size - 1;
    *--op = '\0';  // We build backwards
    পূর্ণাঙ্ক অঙ্ক = fileno;
    কুরু {
        *--op = 'a' + অঙ্ক % 26;
        অঙ্ক /= 26;
    } যাবত্ (অঙ্ক);
    প্রত্যাগম op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_সারি_Singleton::fileNameIdx_t File_সারি_Singleton::name_To_সংখ্যা(ধ্রুব string& filename) {
    ধ্রুব auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    যদি (pair.second) {
        ধ্রুব size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.ঠেলো_back(filename);
        m_languages.ঠেলো_back(V3LangCode::mostRecent());
    }
    প্রত্যাগম idx;
}

শূন্য File_সারি_Singleton::file_Name_অঙ্ক_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    প্রতি (ধ্রুব auto& itr : m_namemap) {
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

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::add_বার্তা_En_Bit_Set(ধ্রুব বার্তা_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ধ্রুব V3LockGuard lock{m_mutex};
    ধ্রুব auto pair = m_interned_বার্তা_En_Idxs.emplace(bitSet, 0);
    বার্তা_En_Set_Idx_t& idx = pair.first->second;
    যদি (pair.second) {
        ধ্রুব size_t nextIdx = m_interned_বার্তা_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<বার্তা_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<বার্তা_En_Set_Idx_t>(nextIdx);
        m_interned_বার্তা_Ens.ঠেলো_back(bitSet);
    }
    প্রত্যাগম idx;
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::default_বার্তা_En_Index() VL_MT_SAFE {
    বার্তা_En_Bit_Set বার্তা_En_Bit_Set_2;
    প্রতি (পূর্ণাঙ্ক i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ধ্রুব V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        বার্তা_En_Bit_Set_2.set(বার্তা_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        বার্তা_En_Bit_Set_2.set(বার্তা_En_Bit_Set::Subset::CTRL, code, সত্য);
    }
    প্রত্যাগম add_বার্তা_En_Bit_Set(বার্তা_En_Bit_Set_2);
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::বার্তা_En_Set_Bit(বার্তা_En_Set_Idx_t setIdx,
                                                                বার্তা_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, তার্কিক মান) {
    // See if state matches existing
    তার্কিক same = সত্য;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        যদি (বার্তা_En(setIdx).test(subset, subcode) != মান) same = অসত্য;
    });
    যদি (same) প্রত্যাগম setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    বার্তা_En_Bit_Set বার্তা_En_Bit_Set_2{বার্তা_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { বার্তা_En_Bit_Set_2.set(subset, subcode, মান); });
    প্রত্যাগম add_বার্তা_En_Bit_Set(বার্তা_En_Bit_Set_2);
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::বার্তা_Set_Ctrl_Bit_Set(বার্তা_En_Set_Idx_t setIdx,
                                                                     ধ্রুব VErrorBitSet& bitset) {
    ধ্রুব বার্তা_En_Bit_Set::Subset subset = বার্তা_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    যদি (বার্তা_En(setIdx).getAll(subset) == bitset) প্রত্যাগম setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    বার্তা_En_Bit_Set বার্তা_En_Bit_Set_2{বার্তা_En(setIdx)};
    বার্তা_En_Bit_Set_2.setAll(subset, bitset);
    প্রত্যাগম add_বার্তা_En_Bit_Set(বার্তা_En_Bit_Set_2);
}

File_সারি_Singleton::বার্তা_En_Set_Idx_t File_সারি_Singleton::বার্তা_En_And(বার্তা_En_Set_Idx_t lhsIdx,
                                                             বার্তা_En_Set_Idx_t rhsIdx) {
    ধ্রুব বার্তা_En_Bit_Set& lhs = বার্তা_En(lhsIdx);
    ধ্রুব বার্তা_En_Bit_Set& rhs = বার্তা_En(rhsIdx);
    ধ্রুব বার্তা_En_Bit_Set intersection{lhs, rhs};
    যদি (intersection == lhs) প্রত্যাগম lhsIdx;
    যদি (intersection == rhs) প্রত্যাগম rhsIdx;
    প্রত্যাগম add_বার্তা_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

শূন্য VFileContent::ঠেলো_Text(ধ্রুব string& text) {
    // Similar code in WildcardContents::pushText()
    যদি (m_সারিগুলি.size() == 0) {
        m_সারিগুলি.emplace_back("");  // no such thing as line [0]
        m_সারিগুলি.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ধ্রুব string leftover = m_সারিগুলি.back() + text;
    m_সারিগুলি.বের_করো_back();

    // Insert line-by-line
    string::size_type সারি_start = 0;
    যাবত্ (সত্য) {
        ধ্রুব string::size_type সারি_end = leftover.find('\n', সারি_start);
        যদি (সারি_end != string::npos) {
            ধ্রুব string oneline(leftover, সারি_start, সারি_end - সারি_start + 1);
            m_সারিগুলি.ঠেলো_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_সারিগুলি.size() - 1) << "]: " << oneline);
            সারি_start = সারি_end + 1;
        } অন্যথা {
            ভঙ্গ;
        }
    }
    // Keep leftover for next time
    m_সারিগুলি.emplace_back(string(leftover, সারি_start));  // Might be ""
}

string VFileContent::get_সারি(পূর্ণাঙ্ক lineno) ধ্রুব VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    যদি (VL_UNCOVERABLE(lineno < 0 || lineno >= (পূর্ণাঙ্ক)m_সারিগুলি.size())) {
        যদি (debug() || v3Global.opt.debugCheck()) {
            প্রত্যাগম ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } অন্যথা {
            প্রত্যাগম "";
        }
    }
    string text = m_সারিগুলি[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    প্রত্যাগম text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    যদি (!contentp) {
        os << "ct0";
    } অন্যথা {
        os << contentp->ascii();
    }
    প্রত্যাগম os;
}

// ######################################################################
//  FileLine class functions

File_সারি::~File_সারি() {
    যদি (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

শূন্য File_সারি::newContent() {
    যদি (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = নূতন VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_সারি::সারি_Directive_Strg(পূর্ণাঙ্ক enterExit) ধ্রুব {
    প্রত্যাগম "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

শূন্য File_সারি::সারি_Directive(ধ্রুব বর্ণ* textp, পূর্ণাঙ্ক& enterExitRef) {
    string newFilename;
    পূর্ণাঙ্ক newLineno = -1;
    সারি_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    যদি (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } অন্যথা {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

শূন্য File_সারি::সারি_Directive_Parse(ধ্রুব বর্ণ* textp, string& filenameRef, পূর্ণাঙ্ক& linenoRef,
                                  পূর্ণাঙ্ক& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    কুরু {
        // Skip `line
        যাবত্ (*textp && std::isspace(*textp)) ++textp;
        যাবত্ (*textp && !std::isspace(*textp)) ++textp;
        যাবত্ (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        পূর্ণাঙ্ক সারি_No;
        ধ্রুব বর্ণ* ধ্রুব ln = textp;
        যাবত্ (*textp && !std::isspace(*textp)) ++textp;
        যদি (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            সারি_No = lineno() + 1;
        } অন্যথা যদি (std::isdigit(*ln)) {
            সারি_No = std::atoi(ln);
        } অন্যথা {
            ভঙ্গ;  // Fail
        }
        যাবত্ (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        যদি (*textp != '"') ভঙ্গ;  // Fail
        ধ্রুব বর্ণ* ধ্রুব fn = ++textp;
        যাবত্ (*textp && *textp != '"') ++textp;
        যদি (*textp != '"') ভঙ্গ;  // Fail
        string err_বার্তা;
        ধ্রুব string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_বার্তা);
        যদি (!err_বার্তা.empty()) এতত্->v3error(err_বার্তা.c_str());
        ++textp;
        যাবত্ (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        যদি (!std::isdigit(*textp)) ভঙ্গ;  // Fail
        ধ্রুব পূর্ণাঙ্ক level = std::atoi(textp);
        যদি (level < 0 || level >= 3) ভঙ্গ;  // Fail

        linenoRef = সারি_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        প্রত্যাগম;
    } যাবত্ (অসত্য);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

শূন্য File_সারি::forwardToken(ধ্রুব বর্ণ* textp, size_t size, তার্কিক track_সারিগুলি) {
    স্থির পূর্ণাঙ্ক s_token_অঙ্ক = 1;
    m_token_অঙ্ক = s_token_অঙ্ক++;
    প্রতি (ধ্রুব বর্ণ* sp = textp; size && *sp; ++sp, --size) {
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
    V3Control::applyIgnores(এতত্);  // Toggle warnings based on global config file
#endif
}

File_সারি* File_সারি::copy_Or_Same_File_সারি_Applied() {
    applyIgnores();
    প্রত্যাগম copy_Or_Same_File_সারি();
}

File_সারি* File_সারি::copy_Or_Same_File_সারি() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    স্থির File_সারি* s_lastNewp = nullptr;
    যদি (s_lastNewp && *s_lastNewp == *এতত্) {  // Compares lineno, filename, etc
        প্রত্যাগম s_lastNewp;
    }
    File_সারি* ধ্রুব newp = নূতন File_সারি{এতত্};
    s_lastNewp = newp;
    প্রত্যাগম newp;
}

string File_সারি::filebasename() ধ্রুব VL_MT_SAFE { প্রত্যাগম V3Os::filenameNonDir(filename()); }

string File_সারি::filebasenameNoExt() ধ্রুব { প্রত্যাগম V3Os::filenameNonDirExt(filename()); }

string File_সারি::firstColumnLetters() ধ্রুব VL_MT_SAFE {
    ধ্রুব বর্ণ a = ((firstColumn() / 26) % 26) + 'a';
    ধ্রুব বর্ণ b = (firstColumn() % 26) + 'a';
    প্রত্যাগম string(1, a) + string(1, b);
}

string File_সারি::profileFuncname() ধ্রুব {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    যাবত্ ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    প্রত্যাগম name;
}

string File_সারি::ascii_সারি_Col() ধ্রুব {
    প্রত্যাগম (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_সারি::ascii() ধ্রুব {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    প্রত্যাগম filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_সারি* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    প্রত্যাগম (os);
}

string File_সারি::warnOffParse(ধ্রুব string& msgs, তার্কিক turnOff) {
    string result;
    প্রতি (ধ্রুব string& বার্তা : VString::split(msgs, ',')) {
        ধ্রুব V3ErrorCode code{বার্তা};
        যদি (!code.hardError()) {
            warnOff(code, turnOff);
            অনুবর্ত;
        }
        // Error if not suppressed
        যদি (!v3Global.opt.isFuture(বার্তা)) result = VString::dot(result, ",", বার্তা);
    }
    প্রত্যাগম result;
}

তার্কিক File_সারি::warnIsOff(V3ErrorCode code) ধ্রুব {
    যদি (!বার্তা_En().enabled(code)) প্রত্যাগম সত্য;
    যদি (!default_File_সারি().বার্তা_En().enabled(code)) প্রত্যাগম সত্য;  // Global overrides local
    প্রত্যাগম অসত্য;
}

// cppverilator-suppress constParameter
শূন্য File_সারি::v3errorEnd(std::ostringstream& sstr, ধ্রুব string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    যদি (lastLineno()) nsstr << V3Error::warnContextBegin() << এতত্ << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    যদি (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    যদি (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } অন্যথা {
        যদি (!V3Error::s().errorContexted()) {
            ধ্রুব string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ধ্রুব string waiverText = V3Error::stripMetaText(wsstr.str(), অসত্য);
        m_waive = V3Control::waive(এতত্, V3Error::s().errorCode(), waiverText);
        যদি (m_waive) {
            V3Error::s().suppressThisWarning();
        } অন্যথা {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), এতত্);
}

string File_সারি::warnMore() ধ্রুব VL_REQUIRES(V3Error::s().m_mutex) {
    যদি (lastLineno()) {
        প্রত্যাগম V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } অন্যথা {
        প্রত্যাগম V3Error::warnMore();
    }
}
string File_সারি::warnOther() ধ্রুব VL_REQUIRES(V3Error::s().m_mutex) {
    যদি (lastLineno()) {
        প্রত্যাগম V3Error::s().warnRelated(এতত্) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } অন্যথা {
        প্রত্যাগম V3Error::warnMore();
    }
};

string File_সারি::source() ধ্রুব VL_MT_SAFE {
    যদি (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        যদি (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            প্রত্যাগম "\n%Error: internal tracking of file contents failed";
        } অন্যথা {
            প্রত্যাগম "";
        }
    }  // LCOV_EXCL_STOP
    প্রত্যাগম m_contentp->get_সারি(m_contentLineno);
}
string File_সারি::sourcePrefix(পূর্ণাঙ্ক toColumn) ধ্রুব VL_MT_SAFE {
    ধ্রুব std::string src = source();
    যদি (toColumn > static_cast<পূর্ণাঙ্ক>(src.length())) toColumn = static_cast<পূর্ণাঙ্ক>(src.length());
    যদি (toColumn < 1) প্রত্যাগম "";
    প্রত্যাগম src.substr(0, toColumn - 1);
}
string File_সারি::prettySource() ধ্রুব VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ধ্রুব string::size_type pos = out.find('\n');
    যদি (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    প্রত্যাগম VString::spaceUnprintable(out);
}

string File_সারি::warnContext() ধ্রুব {
    যদি (!v3Global.opt.context()) প্রত্যাগম "";
    যদি (firstLineno() == lastLineno() && firstColumn()) {
        ধ্রুব string source_সারি = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        যদি (!source_সারি.empty() && source_সারি.length() < SHOW_SOURCE_MAX_LENGTH
            && source_সারি.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            যাবত্ (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_সারি + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            যদি (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        প্রত্যাগম V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    প্রত্যাগম "";
}

string File_সারি::warnContextParent() ধ্রুব VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    প্রতি (File_সারি* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        যদি (parentFl->filenameIsGlobal()) ভঙ্গ;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    প্রত্যাগম result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_সারি*> file_সারি_Leak_Checks;

শূন্য* File_সারি::operator নূতন(size_t size) {
    File_সারি* ধ্রুব objp = static_cast<File_সারি*>(::operator নূতন(size));
    file_সারি_Leak_Checks.ঢোকাও(objp);
    প্রত্যাগম objp;
}

শূন্য File_সারি::operator delete(শূন্য* objp, size_t size) {
    যদি (!objp) প্রত্যাগম;
    File_সারি* ধ্রুব flp = static_cast<File_সারি*>(objp);
    ধ্রুব auto it = file_সারি_Leak_Checks.find(flp);
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
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_বার্তা_Ens.size());  // Max m_msgEnIdx
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
    যাবত্ (সত্য) {
        ধ্রুব auto it = file_সারি_Leak_Checks.begin();
        যদি (it == file_সারি_Leak_Checks.end()) ভঙ্গ;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_সারি_Leak_Checks.clear();
    singleton().clear();
#endif
}
