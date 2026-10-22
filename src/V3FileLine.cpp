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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

// clang-format off
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3FileLine.h"
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#অন্তর্ভাৱ "V3Waiver.h"
// clang-format on

#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <iomanip>
#অন্তর্ভাৱ <limits>
#অন্তর্ভাৱ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_সাৰি_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr পূর্ণাঙ্ক size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    ৱর্ণ out[size];
    ৱর্ণ* op = out + size - 1;
    *--op = '\0';  // We build backwards
    পূর্ণাঙ্ক অঙ্ক = fileno;
    কৰা {
        *--op = 'a' + অঙ্ক % 26;
        অঙ্ক /= 26;
    } যেতিয়ালৈ (অঙ্ক);
    ঘূৰাই op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_সাৰি_Singleton::fileNameIdx_t File_সাৰি_Singleton::name_To_সংখ্যা(ধ্রুৱ string& filename) {
    ধ্রুৱ auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    যদি (pair.second) {
        ধ্রুৱ size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.ঠেলো_back(filename);
        m_languages.ঠেলো_back(V3LangCode::mostRecent());
    }
    ঘূৰাই idx;
}

শূন্য File_সাৰি_Singleton::file_Name_অঙ্ক_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    বাবে (ধ্রুৱ auto& itr : m_namemap) {
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

File_সাৰি_Singleton::বাৰ্তা_En_Set_Idx_t File_সাৰি_Singleton::add_বাৰ্তা_En_Bit_Set(ধ্রুৱ বাৰ্তা_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ধ্রুৱ V3LockGuard lock{m_mutex};
    ধ্রুৱ auto pair = m_interned_বাৰ্তা_En_Idxs.emplace(bitSet, 0);
    বাৰ্তা_En_Set_Idx_t& idx = pair.first->second;
    যদি (pair.second) {
        ধ্রুৱ size_t nextIdx = m_interned_বাৰ্তা_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<বাৰ্তা_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<বাৰ্তা_En_Set_Idx_t>(nextIdx);
        m_interned_বাৰ্তা_Ens.ঠেলো_back(bitSet);
    }
    ঘূৰাই idx;
}

File_সাৰি_Singleton::বাৰ্তা_En_Set_Idx_t File_সাৰি_Singleton::default_বাৰ্তা_En_Index() VL_MT_SAFE {
    বাৰ্তা_En_Bit_Set বাৰ্তা_En_Bit_Set_2;
    বাবে (পূর্ণাঙ্ক i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ধ্রুৱ V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        বাৰ্তা_En_Bit_Set_2.set(বাৰ্তা_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        বাৰ্তা_En_Bit_Set_2.set(বাৰ্তা_En_Bit_Set::Subset::CTRL, code, সঁচা);
    }
    ঘূৰাই add_বাৰ্তা_En_Bit_Set(বাৰ্তা_En_Bit_Set_2);
}

File_সাৰি_Singleton::বাৰ্তা_En_Set_Idx_t File_সাৰি_Singleton::বাৰ্তা_En_Set_Bit(বাৰ্তা_En_Set_Idx_t setIdx,
                                                                বাৰ্তা_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, তার্কিক মান) {
    // See if state matches existing
    তার্কিক same = সঁচা;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        যদি (বাৰ্তা_En(setIdx).test(subset, subcode) != মান) same = মিছা;
    });
    যদি (same) ঘূৰাই setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    বাৰ্তা_En_Bit_Set বাৰ্তা_En_Bit_Set_2{বাৰ্তা_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { বাৰ্তা_En_Bit_Set_2.set(subset, subcode, মান); });
    ঘূৰাই add_বাৰ্তা_En_Bit_Set(বাৰ্তা_En_Bit_Set_2);
}

File_সাৰি_Singleton::বাৰ্তা_En_Set_Idx_t File_সাৰি_Singleton::বাৰ্তা_Set_Ctrl_Bit_Set(বাৰ্তা_En_Set_Idx_t setIdx,
                                                                     ধ্রুৱ VErrorBitSet& bitset) {
    ধ্রুৱ বাৰ্তা_En_Bit_Set::Subset subset = বাৰ্তা_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    যদি (বাৰ্তা_En(setIdx).getAll(subset) == bitset) ঘূৰাই setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    বাৰ্তা_En_Bit_Set বাৰ্তা_En_Bit_Set_2{বাৰ্তা_En(setIdx)};
    বাৰ্তা_En_Bit_Set_2.setAll(subset, bitset);
    ঘূৰাই add_বাৰ্তা_En_Bit_Set(বাৰ্তা_En_Bit_Set_2);
}

File_সাৰি_Singleton::বাৰ্তা_En_Set_Idx_t File_সাৰি_Singleton::বাৰ্তা_En_And(বাৰ্তা_En_Set_Idx_t lhsIdx,
                                                             বাৰ্তা_En_Set_Idx_t rhsIdx) {
    ধ্রুৱ বাৰ্তা_En_Bit_Set& lhs = বাৰ্তা_En(lhsIdx);
    ধ্রুৱ বাৰ্তা_En_Bit_Set& rhs = বাৰ্তা_En(rhsIdx);
    ধ্রুৱ বাৰ্তা_En_Bit_Set intersection{lhs, rhs};
    যদি (intersection == lhs) ঘূৰাই lhsIdx;
    যদি (intersection == rhs) ঘূৰাই rhsIdx;
    ঘূৰাই add_বাৰ্তা_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

শূন্য VFileContent::ঠেলো_Text(ধ্রুৱ string& text) {
    // Similar code in WildcardContents::pushText()
    যদি (m_সাৰিগুলি.size() == 0) {
        m_সাৰিগুলি.emplace_back("");  // no such thing as line [0]
        m_সাৰিগুলি.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ধ্রুৱ string leftover = m_সাৰিগুলি.back() + text;
    m_সাৰিগুলি.বেৰ_কৰো_back();

    // Insert line-by-line
    string::size_type সাৰি_start = 0;
    যেতিয়ালৈ (সঁচা) {
        ধ্রুৱ string::size_type সাৰি_end = leftover.find('\n', সাৰি_start);
        যদি (সাৰি_end != string::npos) {
            ধ্রুৱ string oneline(leftover, সাৰি_start, সাৰি_end - সাৰি_start + 1);
            m_সাৰিগুলি.ঠেলো_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_সাৰিগুলি.size() - 1) << "]: " << oneline);
            সাৰি_start = সাৰি_end + 1;
        } অন্যথা {
            ভঙ্গ;
        }
    }
    // Keep leftover for next time
    m_সাৰিগুলি.emplace_back(string(leftover, সাৰি_start));  // Might be ""
}

string VFileContent::get_সাৰি(পূর্ণাঙ্ক lineno) ধ্রুৱ VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    যদি (VL_UNCOVERABLE(lineno < 0 || lineno >= (পূর্ণাঙ্ক)m_সাৰিগুলি.size())) {
        যদি (debug() || v3Global.opt.debugCheck()) {
            ঘূৰাই ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } অন্যথা {
            ঘূৰাই "";
        }
    }
    string text = m_সাৰিগুলি[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    ঘূৰাই text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    যদি (!contentp) {
        os << "ct0";
    } অন্যথা {
        os << contentp->ascii();
    }
    ঘূৰাই os;
}

// ######################################################################
//  FileLine class functions

File_সাৰি::~File_সাৰি() {
    যদি (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

শূন্য File_সাৰি::newContent() {
    যদি (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = নূতন VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_সাৰি::সাৰি_Directive_Strg(পূর্ণাঙ্ক enterExit) ধ্রুৱ {
    ঘূৰাই "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

শূন্য File_সাৰি::সাৰি_Directive(ধ্রুৱ ৱর্ণ* textp, পূর্ণাঙ্ক& enterExitRef) {
    string newFilename;
    পূর্ণাঙ্ক newLineno = -1;
    সাৰি_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    যদি (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } অন্যথা {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

শূন্য File_সাৰি::সাৰি_Directive_Parse(ধ্রুৱ ৱর্ণ* textp, string& filenameRef, পূর্ণাঙ্ক& linenoRef,
                                  পূর্ণাঙ্ক& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    কৰা {
        // Skip `line
        যেতিয়ালৈ (*textp && std::isspace(*textp)) ++textp;
        যেতিয়ালৈ (*textp && !std::isspace(*textp)) ++textp;
        যেতিয়ালৈ (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        পূর্ণাঙ্ক সাৰি_No;
        ধ্রুৱ ৱর্ণ* ধ্রুৱ ln = textp;
        যেতিয়ালৈ (*textp && !std::isspace(*textp)) ++textp;
        যদি (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            সাৰি_No = lineno() + 1;
        } অন্যথা যদি (std::isdigit(*ln)) {
            সাৰি_No = std::atoi(ln);
        } অন্যথা {
            ভঙ্গ;  // Fail
        }
        যেতিয়ালৈ (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        যদি (*textp != '"') ভঙ্গ;  // Fail
        ধ্রুৱ ৱর্ণ* ধ্রুৱ fn = ++textp;
        যেতিয়ালৈ (*textp && *textp != '"') ++textp;
        যদি (*textp != '"') ভঙ্গ;  // Fail
        string err_বাৰ্তা;
        ধ্রুৱ string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_বাৰ্তা);
        যদি (!err_বাৰ্তা.empty()) এতত্->v3error(err_বাৰ্তা.c_str());
        ++textp;
        যেতিয়ালৈ (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        যদি (!std::isdigit(*textp)) ভঙ্গ;  // Fail
        ধ্রুৱ পূর্ণাঙ্ক level = std::atoi(textp);
        যদি (level < 0 || level >= 3) ভঙ্গ;  // Fail

        linenoRef = সাৰি_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        ঘূৰাই;
    } যেতিয়ালৈ (মিছা);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

শূন্য File_সাৰি::forwardToken(ধ্রুৱ ৱর্ণ* textp, size_t size, তার্কিক track_সাৰিগুলি) {
    স্থির পূর্ণাঙ্ক s_token_অঙ্ক = 1;
    m_token_অঙ্ক = s_token_অঙ্ক++;
    বাবে (ধ্রুৱ ৱর্ণ* sp = textp; size && *sp; ++sp, --size) {
        যদি (*sp == '\n') {
            যদি (track_সাৰিগুলি) linenoInc();
            m_lastColumn = 1;
        } অন্যথা যদি (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } অন্যথা {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

শূন্য File_সাৰি::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(এতত্);  // Toggle warnings based on global config file
#endif
}

File_সাৰি* File_সাৰি::copy_Or_Same_File_সাৰি_Applied() {
    applyIgnores();
    ঘূৰাই copy_Or_Same_File_সাৰি();
}

File_সাৰি* File_সাৰি::copy_Or_Same_File_সাৰি() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    স্থির File_সাৰি* s_lastNewp = nullptr;
    যদি (s_lastNewp && *s_lastNewp == *এতত্) {  // Compares lineno, filename, etc
        ঘূৰাই s_lastNewp;
    }
    File_সাৰি* ধ্রুৱ newp = নূতন File_সাৰি{এতত্};
    s_lastNewp = newp;
    ঘূৰাই newp;
}

string File_সাৰি::filebasename() ধ্রুৱ VL_MT_SAFE { ঘূৰাই V3Os::filenameNonDir(filename()); }

string File_সাৰি::filebasenameNoExt() ধ্রুৱ { ঘূৰাই V3Os::filenameNonDirExt(filename()); }

string File_সাৰি::firstColumnLetters() ধ্রুৱ VL_MT_SAFE {
    ধ্রুৱ ৱর্ণ a = ((firstColumn() / 26) % 26) + 'a';
    ধ্রুৱ ৱর্ণ b = (firstColumn() % 26) + 'a';
    ঘূৰাই string(1, a) + string(1, b);
}

string File_সাৰি::profileFuncname() ধ্রুৱ {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    যেতিয়ালৈ ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    ঘূৰাই name;
}

string File_সাৰি::ascii_সাৰি_Col() ধ্রুৱ {
    ঘূৰাই (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_সাৰি::ascii() ধ্রুৱ {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    ঘূৰাই filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_সাৰি* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    ঘূৰাই (os);
}

string File_সাৰি::warnOffParse(ধ্রুৱ string& msgs, তার্কিক turnOff) {
    string result;
    বাবে (ধ্রুৱ string& বাৰ্তা : VString::split(msgs, ',')) {
        ধ্রুৱ V3ErrorCode code{বাৰ্তা};
        যদি (!code.hardError()) {
            warnOff(code, turnOff);
            অনুৱর্ত;
        }
        // Error if not suppressed
        যদি (!v3Global.opt.isFuture(বাৰ্তা)) result = VString::dot(result, ",", বাৰ্তা);
    }
    ঘূৰাই result;
}

তার্কিক File_সাৰি::warnIsOff(V3ErrorCode code) ধ্রুৱ {
    যদি (!বাৰ্তা_En().enabled(code)) ঘূৰাই সঁচা;
    যদি (!default_File_সাৰি().বাৰ্তা_En().enabled(code)) ঘূৰাই সঁচা;  // Global overrides local
    ঘূৰাই মিছা;
}

// cppverilator-suppress constParameter
শূন্য File_সাৰি::v3errorEnd(std::ostringstream& sstr, ধ্রুৱ string& extra)
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
            ধ্রুৱ string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ধ্রুৱ string waiverText = V3Error::stripMetaText(wsstr.str(), মিছা);
        m_waive = V3Control::waive(এতত্, V3Error::s().errorCode(), waiverText);
        যদি (m_waive) {
            V3Error::s().suppressThisWarning();
        } অন্যথা {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), এতত্);
}

string File_সাৰি::warnMore() ধ্রুৱ VL_REQUIRES(V3Error::s().m_mutex) {
    যদি (lastLineno()) {
        ঘূৰাই V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } অন্যথা {
        ঘূৰাই V3Error::warnMore();
    }
}
string File_সাৰি::warnOther() ধ্রুৱ VL_REQUIRES(V3Error::s().m_mutex) {
    যদি (lastLineno()) {
        ঘূৰাই V3Error::s().warnRelated(এতত্) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } অন্যথা {
        ঘূৰাই V3Error::warnMore();
    }
};

string File_সাৰি::source() ধ্রুৱ VL_MT_SAFE {
    যদি (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        যদি (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            ঘূৰাই "\n%Error: internal tracking of file contents failed";
        } অন্যথা {
            ঘূৰাই "";
        }
    }  // LCOV_EXCL_STOP
    ঘূৰাই m_contentp->get_সাৰি(m_contentLineno);
}
string File_সাৰি::sourcePrefix(পূর্ণাঙ্ক toColumn) ধ্রুৱ VL_MT_SAFE {
    ধ্রুৱ std::string src = source();
    যদি (toColumn > static_cast<পূর্ণাঙ্ক>(src.length())) toColumn = static_cast<পূর্ণাঙ্ক>(src.length());
    যদি (toColumn < 1) ঘূৰাই "";
    ঘূৰাই src.substr(0, toColumn - 1);
}
string File_সাৰি::prettySource() ধ্রুৱ VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ধ্রুৱ string::size_type pos = out.find('\n');
    যদি (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    ঘূৰাই VString::spaceUnprintable(out);
}

string File_সাৰি::warnContext() ধ্রুৱ {
    যদি (!v3Global.opt.context()) ঘূৰাই "";
    যদি (firstLineno() == lastLineno() && firstColumn()) {
        ধ্রুৱ string source_সাৰি = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        যদি (!source_সাৰি.empty() && source_সাৰি.length() < SHOW_SOURCE_MAX_LENGTH
            && source_সাৰি.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            যেতিয়ালৈ (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_সাৰি + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            যদি (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        ঘূৰাই V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    ঘূৰাই "";
}

string File_সাৰি::warnContextParent() ধ্রুৱ VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    বাবে (File_সাৰি* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        যদি (parentFl->filenameIsGlobal()) ভঙ্গ;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    ঘূৰাই result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_সাৰি*> file_সাৰি_Leak_Checks;

শূন্য* File_সাৰি::operator নূতন(size_t size) {
    File_সাৰি* ধ্রুৱ objp = static_cast<File_সাৰি*>(::operator নূতন(size));
    file_সাৰি_Leak_Checks.ঢোকাও(objp);
    ঘূৰাই objp;
}

শূন্য File_সাৰি::operator delete(শূন্য* objp, size_t size) {
    যদি (!objp) ঘূৰাই;
    File_সাৰি* ধ্রুৱ flp = static_cast<File_সাৰি*>(objp);
    ধ্রুৱ auto it = file_সাৰি_Leak_Checks.find(flp);
    যদি (it != file_সাৰি_Leak_Checks.end()) {
        file_সাৰি_Leak_Checks.erase(it);
    } অন্যথা {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

শূন্য File_সাৰি::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_বাৰ্তা_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

শূন্য File_সাৰি::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    যেতিয়ালৈ (সঁচা) {
        ধ্রুৱ auto it = file_সাৰি_Leak_Checks.begin();
        যদি (it == file_সাৰি_Leak_Checks.end()) ভঙ্গ;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_সাৰি_Leak_Checks.clear();
    singleton().clear();
#endif
}
