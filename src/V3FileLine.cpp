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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

// clang-format off
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#അന്തര്ഭാവ "V3Waiver.h"
// clang-format on

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <limits>
#അന്തര്ഭാവ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_വരി_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr പൂർണ്ണസംഖ്യ size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    അക്ഷരം out[size];
    അക്ഷരം* op = out + size - 1;
    *--op = '\0';  // We build backwards
    പൂർണ്ണസംഖ്യ അക്കം = fileno;
    ചെയ്യുക {
        *--op = 'a' + അക്കം % 26;
        അക്കം /= 26;
    } ആകുമ്പോൾ (അക്കം);
    തിരികെ op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_വരി_Singleton::fileNameIdx_t File_വരി_Singleton::name_To_സംഖ്യ(സ്ഥിരം string& filename) {
    സ്ഥിരം auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    എങ്കിൽ (pair.second) {
        സ്ഥിരം size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.തള്ളുക_back(filename);
        m_languages.തള്ളുക_back(V3LangCode::mostRecent());
    }
    തിരികെ idx;
}

ശൂന്യം File_വരി_Singleton::file_Name_അക്കം_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    വേണ്ടി (സ്ഥിരം auto& itr : m_namemap) {
        std::string filename = itr.first;
        എങ്കിൽ (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } അല്ലെങ്കിൽ എങ്കിൽ (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << സംഖ്യ_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_വരി_Singleton::സന്ദേശം_En_Set_Idx_t File_വരി_Singleton::add_സന്ദേശം_En_Bit_Set(സ്ഥിരം സന്ദേശം_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    സ്ഥിരം V3LockGuard lock{m_mutex};
    സ്ഥിരം auto pair = m_interned_സന്ദേശം_En_Idxs.emplace(bitSet, 0);
    സന്ദേശം_En_Set_Idx_t& idx = pair.first->second;
    എങ്കിൽ (pair.second) {
        സ്ഥിരം size_t nextIdx = m_interned_സന്ദേശം_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<സന്ദേശം_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<സന്ദേശം_En_Set_Idx_t>(nextIdx);
        m_interned_സന്ദേശം_Ens.തള്ളുക_back(bitSet);
    }
    തിരികെ idx;
}

File_വരി_Singleton::സന്ദേശം_En_Set_Idx_t File_വരി_Singleton::default_സന്ദേശം_En_Index() VL_MT_SAFE {
    സന്ദേശം_En_Bit_Set സന്ദേശം_En_Bit_Set_2;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        സ്ഥിരം V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        സന്ദേശം_En_Bit_Set_2.set(സന്ദേശം_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        സന്ദേശം_En_Bit_Set_2.set(സന്ദേശം_En_Bit_Set::Subset::CTRL, code, സത്യം);
    }
    തിരികെ add_സന്ദേശം_En_Bit_Set(സന്ദേശം_En_Bit_Set_2);
}

File_വരി_Singleton::സന്ദേശം_En_Set_Idx_t File_വരി_Singleton::സന്ദേശം_En_Set_Bit(സന്ദേശം_En_Set_Idx_t setIdx,
                                                                സന്ദേശം_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, തർക്കം മൂല്യം) {
    // See if state matches existing
    തർക്കം same = സത്യം;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        എങ്കിൽ (സന്ദേശം_En(setIdx).test(subset, subcode) != മൂല്യം) same = അസത്യം;
    });
    എങ്കിൽ (same) തിരികെ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    സന്ദേശം_En_Bit_Set സന്ദേശം_En_Bit_Set_2{സന്ദേശം_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { സന്ദേശം_En_Bit_Set_2.set(subset, subcode, മൂല്യം); });
    തിരികെ add_സന്ദേശം_En_Bit_Set(സന്ദേശം_En_Bit_Set_2);
}

File_വരി_Singleton::സന്ദേശം_En_Set_Idx_t File_വരി_Singleton::സന്ദേശം_Set_Ctrl_Bit_Set(സന്ദേശം_En_Set_Idx_t setIdx,
                                                                     സ്ഥിരം VErrorBitSet& bitset) {
    സ്ഥിരം സന്ദേശം_En_Bit_Set::Subset subset = സന്ദേശം_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    എങ്കിൽ (സന്ദേശം_En(setIdx).getAll(subset) == bitset) തിരികെ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    സന്ദേശം_En_Bit_Set സന്ദേശം_En_Bit_Set_2{സന്ദേശം_En(setIdx)};
    സന്ദേശം_En_Bit_Set_2.setAll(subset, bitset);
    തിരികെ add_സന്ദേശം_En_Bit_Set(സന്ദേശം_En_Bit_Set_2);
}

File_വരി_Singleton::സന്ദേശം_En_Set_Idx_t File_വരി_Singleton::സന്ദേശം_En_And(സന്ദേശം_En_Set_Idx_t lhsIdx,
                                                             സന്ദേശം_En_Set_Idx_t rhsIdx) {
    സ്ഥിരം സന്ദേശം_En_Bit_Set& lhs = സന്ദേശം_En(lhsIdx);
    സ്ഥിരം സന്ദേശം_En_Bit_Set& rhs = സന്ദേശം_En(rhsIdx);
    സ്ഥിരം സന്ദേശം_En_Bit_Set intersection{lhs, rhs};
    എങ്കിൽ (intersection == lhs) തിരികെ lhsIdx;
    എങ്കിൽ (intersection == rhs) തിരികെ rhsIdx;
    തിരികെ add_സന്ദേശം_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

ശൂന്യം VFileContent::തള്ളുക_Text(സ്ഥിരം string& text) {
    // Similar code in WildcardContents::pushText()
    എങ്കിൽ (m_വരികൾ.size() == 0) {
        m_വരികൾ.emplace_back("");  // no such thing as line [0]
        m_വരികൾ.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    സ്ഥിരം string leftover = m_വരികൾ.back() + text;
    m_വരികൾ.എടുക്കുക_back();

    // Insert line-by-line
    string::size_type വരി_start = 0;
    ആകുമ്പോൾ (സത്യം) {
        സ്ഥിരം string::size_type വരി_end = leftover.find('\n', വരി_start);
        എങ്കിൽ (വരി_end != string::npos) {
            സ്ഥിരം string oneline(leftover, വരി_start, വരി_end - വരി_start + 1);
            m_വരികൾ.തള്ളുക_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_വരികൾ.size() - 1) << "]: " << oneline);
            വരി_start = വരി_end + 1;
        } അല്ലെങ്കിൽ {
            നിർത്തുക;
        }
    }
    // Keep leftover for next time
    m_വരികൾ.emplace_back(string(leftover, വരി_start));  // Might be ""
}

string VFileContent::get_വരി(പൂർണ്ണസംഖ്യ lineno) സ്ഥിരം VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    എങ്കിൽ (VL_UNCOVERABLE(lineno < 0 || lineno >= (പൂർണ്ണസംഖ്യ)m_വരികൾ.size())) {
        എങ്കിൽ (debug() || v3Global.opt.debugCheck()) {
            തിരികെ ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } അല്ലെങ്കിൽ {
            തിരികെ "";
        }
    }
    string text = m_വരികൾ[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    തിരികെ text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    എങ്കിൽ (!contentp) {
        os << "ct0";
    } അല്ലെങ്കിൽ {
        os << contentp->ascii();
    }
    തിരികെ os;
}

// ######################################################################
//  FileLine class functions

File_വരി::~File_വരി() {
    എങ്കിൽ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

ശൂന്യം File_വരി::newContent() {
    എങ്കിൽ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = പുതിയ VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_വരി::വരി_Directive_Strg(പൂർണ്ണസംഖ്യ enterExit) സ്ഥിരം {
    തിരികെ "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

ശൂന്യം File_വരി::വരി_Directive(സ്ഥിരം അക്ഷരം* textp, പൂർണ്ണസംഖ്യ& enterExitRef) {
    string newFilename;
    പൂർണ്ണസംഖ്യ newLineno = -1;
    വരി_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    എങ്കിൽ (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } അല്ലെങ്കിൽ {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

ശൂന്യം File_വരി::വരി_Directive_Parse(സ്ഥിരം അക്ഷരം* textp, string& filenameRef, പൂർണ്ണസംഖ്യ& linenoRef,
                                  പൂർണ്ണസംഖ്യ& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    ചെയ്യുക {
        // Skip `line
        ആകുമ്പോൾ (*textp && std::isspace(*textp)) ++textp;
        ആകുമ്പോൾ (*textp && !std::isspace(*textp)) ++textp;
        ആകുമ്പോൾ (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        പൂർണ്ണസംഖ്യ വരി_No;
        സ്ഥിരം അക്ഷരം* സ്ഥിരം ln = textp;
        ആകുമ്പോൾ (*textp && !std::isspace(*textp)) ++textp;
        എങ്കിൽ (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            വരി_No = lineno() + 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (std::isdigit(*ln)) {
            വരി_No = std::atoi(ln);
        } അല്ലെങ്കിൽ {
            നിർത്തുക;  // Fail
        }
        ആകുമ്പോൾ (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        എങ്കിൽ (*textp != '"') നിർത്തുക;  // Fail
        സ്ഥിരം അക്ഷരം* സ്ഥിരം fn = ++textp;
        ആകുമ്പോൾ (*textp && *textp != '"') ++textp;
        എങ്കിൽ (*textp != '"') നിർത്തുക;  // Fail
        string err_സന്ദേശം;
        സ്ഥിരം string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_സന്ദേശം);
        എങ്കിൽ (!err_സന്ദേശം.empty()) ഇത്->v3error(err_സന്ദേശം.c_str());
        ++textp;
        ആകുമ്പോൾ (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        എങ്കിൽ (!std::isdigit(*textp)) നിർത്തുക;  // Fail
        സ്ഥിരം പൂർണ്ണസംഖ്യ level = std::atoi(textp);
        എങ്കിൽ (level < 0 || level >= 3) നിർത്തുക;  // Fail

        linenoRef = വരി_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        തിരികെ;
    } ആകുമ്പോൾ (അസത്യം);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

ശൂന്യം File_വരി::forwardToken(സ്ഥിരം അക്ഷരം* textp, size_t size, തർക്കം track_വരികൾ) {
    സ്ഥിതം പൂർണ്ണസംഖ്യ s_token_അക്കം = 1;
    m_token_അക്കം = s_token_അക്കം++;
    വേണ്ടി (സ്ഥിരം അക്ഷരം* sp = textp; size && *sp; ++sp, --size) {
        എങ്കിൽ (*sp == '\n') {
            എങ്കിൽ (track_വരികൾ) linenoInc();
            m_lastColumn = 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } അല്ലെങ്കിൽ {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

ശൂന്യം File_വരി::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(ഇത്);  // Toggle warnings based on global config file
#endif
}

File_വരി* File_വരി::copy_Or_Same_File_വരി_Applied() {
    applyIgnores();
    തിരികെ copy_Or_Same_File_വരി();
}

File_വരി* File_വരി::copy_Or_Same_File_വരി() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    സ്ഥിതം File_വരി* s_lastNewp = nullptr;
    എങ്കിൽ (s_lastNewp && *s_lastNewp == *ഇത്) {  // Compares lineno, filename, etc
        തിരികെ s_lastNewp;
    }
    File_വരി* സ്ഥിരം newp = പുതിയ File_വരി{ഇത്};
    s_lastNewp = newp;
    തിരികെ newp;
}

string File_വരി::filebasename() സ്ഥിരം VL_MT_SAFE { തിരികെ V3Os::filenameNonDir(filename()); }

string File_വരി::filebasenameNoExt() സ്ഥിരം { തിരികെ V3Os::filenameNonDirExt(filename()); }

string File_വരി::firstColumnLetters() സ്ഥിരം VL_MT_SAFE {
    സ്ഥിരം അക്ഷരം a = ((firstColumn() / 26) % 26) + 'a';
    സ്ഥിരം അക്ഷരം b = (firstColumn() % 26) + 'a';
    തിരികെ string(1, a) + string(1, b);
}

string File_വരി::profileFuncname() സ്ഥിരം {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    ആകുമ്പോൾ ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    തിരികെ name;
}

string File_വരി::ascii_വരി_Col() സ്ഥിരം {
    തിരികെ (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_വരി::ascii() സ്ഥിരം {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    തിരികെ filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_വരി* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    തിരികെ (os);
}

string File_വരി::warnOffParse(സ്ഥിരം string& msgs, തർക്കം turnOff) {
    string result;
    വേണ്ടി (സ്ഥിരം string& സന്ദേശം : VString::split(msgs, ',')) {
        സ്ഥിരം V3ErrorCode code{സന്ദേശം};
        എങ്കിൽ (!code.hardError()) {
            warnOff(code, turnOff);
            തുടരുക;
        }
        // Error if not suppressed
        എങ്കിൽ (!v3Global.opt.isFuture(സന്ദേശം)) result = VString::dot(result, ",", സന്ദേശം);
    }
    തിരികെ result;
}

തർക്കം File_വരി::warnIsOff(V3ErrorCode code) സ്ഥിരം {
    എങ്കിൽ (!സന്ദേശം_En().enabled(code)) തിരികെ സത്യം;
    എങ്കിൽ (!default_File_വരി().സന്ദേശം_En().enabled(code)) തിരികെ സത്യം;  // Global overrides local
    തിരികെ അസത്യം;
}

// cppverilator-suppress constParameter
ശൂന്യം File_വരി::v3errorEnd(std::ostringstream& sstr, സ്ഥിരം string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    എങ്കിൽ (lastLineno()) nsstr << V3Error::warnContextBegin() << ഇത് << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    എങ്കിൽ (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    എങ്കിൽ (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (!V3Error::s().errorContexted()) {
            സ്ഥിരം string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        സ്ഥിരം string waiverText = V3Error::stripMetaText(wsstr.str(), അസത്യം);
        m_waive = V3Control::waive(ഇത്, V3Error::s().errorCode(), waiverText);
        എങ്കിൽ (m_waive) {
            V3Error::s().suppressThisWarning();
        } അല്ലെങ്കിൽ {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), ഇത്);
}

string File_വരി::warnMore() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex) {
    എങ്കിൽ (lastLineno()) {
        തിരികെ V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } അല്ലെങ്കിൽ {
        തിരികെ V3Error::warnMore();
    }
}
string File_വരി::warnOther() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex) {
    എങ്കിൽ (lastLineno()) {
        തിരികെ V3Error::s().warnRelated(ഇത്) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } അല്ലെങ്കിൽ {
        തിരികെ V3Error::warnMore();
    }
};

string File_വരി::source() സ്ഥിരം VL_MT_SAFE {
    എങ്കിൽ (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        എങ്കിൽ (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            തിരികെ "\n%Error: internal tracking of file contents failed";
        } അല്ലെങ്കിൽ {
            തിരികെ "";
        }
    }  // LCOV_EXCL_STOP
    തിരികെ m_contentp->get_വരി(m_contentLineno);
}
string File_വരി::sourcePrefix(പൂർണ്ണസംഖ്യ toColumn) സ്ഥിരം VL_MT_SAFE {
    സ്ഥിരം std::string src = source();
    എങ്കിൽ (toColumn > static_cast<പൂർണ്ണസംഖ്യ>(src.length())) toColumn = static_cast<പൂർണ്ണസംഖ്യ>(src.length());
    എങ്കിൽ (toColumn < 1) തിരികെ "";
    തിരികെ src.substr(0, toColumn - 1);
}
string File_വരി::prettySource() സ്ഥിരം VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    സ്ഥിരം string::size_type pos = out.find('\n');
    എങ്കിൽ (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    തിരികെ VString::spaceUnprintable(out);
}

string File_വരി::warnContext() സ്ഥിരം {
    എങ്കിൽ (!v3Global.opt.context()) തിരികെ "";
    എങ്കിൽ (firstLineno() == lastLineno() && firstColumn()) {
        സ്ഥിരം string source_വരി = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        എങ്കിൽ (!source_വരി.empty() && source_വരി.length() < SHOW_SOURCE_MAX_LENGTH
            && source_വരി.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            ആകുമ്പോൾ (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_വരി + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            എങ്കിൽ (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        തിരികെ V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    തിരികെ "";
}

string File_വരി::warnContextParent() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    വേണ്ടി (File_വരി* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        എങ്കിൽ (parentFl->filenameIsGlobal()) നിർത്തുക;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    തിരികെ result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_വരി*> file_വരി_Leak_Checks;

ശൂന്യം* File_വരി::operator പുതിയ(size_t size) {
    File_വരി* സ്ഥിരം objp = static_cast<File_വരി*>(::operator പുതിയ(size));
    file_വരി_Leak_Checks.തിരുകുക(objp);
    തിരികെ objp;
}

ശൂന്യം File_വരി::operator delete(ശൂന്യം* objp, size_t size) {
    എങ്കിൽ (!objp) തിരികെ;
    File_വരി* സ്ഥിരം flp = static_cast<File_വരി*>(objp);
    സ്ഥിരം auto it = file_വരി_Leak_Checks.find(flp);
    എങ്കിൽ (it != file_വരി_Leak_Checks.end()) {
        file_വരി_Leak_Checks.erase(it);
    } അല്ലെങ്കിൽ {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

ശൂന്യം File_വരി::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_സന്ദേശം_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

ശൂന്യം File_വരി::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    ആകുമ്പോൾ (സത്യം) {
        സ്ഥിരം auto it = file_വരി_Leak_Checks.begin();
        എങ്കിൽ (it == file_വരി_Leak_Checks.end()) നിർത്തുക;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_വരി_Leak_Checks.clear();
    singleton().clear();
#endif
}
