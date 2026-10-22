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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

// clang-format off
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#अन्तर्भाव "V3Waiver.h"
// clang-format on

#अन्तर्भाव <algorithm>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <limits>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_पङ्क्तिः_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr पूर्णाङ्क size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    वर्ण out[size];
    वर्ण* op = out + size - 1;
    *--op = '\0';  // We build backwards
    पूर्णाङ्क अङ्कः = fileno;
    करेहि {
        *--op = 'a' + अङ्कः % 26;
        अङ्कः /= 26;
    } जाव (अङ्कः);
    प्रत्यागम op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_पङ्क्तिः_Singleton::fileNameIdx_t File_पङ्क्तिः_Singleton::name_To_सङ्ख्या(ध्रुव string& filename) {
    ध्रुव auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    जइ (pair.second) {
        ध्रुव size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.प्रक्षिप_back(filename);
        m_languages.प्रक्षिप_back(V3LangCode::mostRecent());
    }
    प्रत्यागम idx;
}

शून्य File_पङ्क्तिः_Singleton::file_Name_अङ्कः_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    प्रति (ध्रुव auto& itr : m_namemap) {
        std::string filename = itr.first;
        जइ (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } अण्णहा जइ (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << सङ्ख्या_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_पङ्क्तिः_Singleton::सन्देशः_En_Set_Idx_t File_पङ्क्तिः_Singleton::add_सन्देशः_En_Bit_Set(ध्रुव सन्देशः_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ध्रुव V3LockGuard lock{m_mutex};
    ध्रुव auto pair = m_interned_सन्देशः_En_Idxs.emplace(bitSet, 0);
    सन्देशः_En_Set_Idx_t& idx = pair.first->second;
    जइ (pair.second) {
        ध्रुव size_t nextIdx = m_interned_सन्देशः_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<सन्देशः_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<सन्देशः_En_Set_Idx_t>(nextIdx);
        m_interned_सन्देशः_Ens.प्रक्षिप_back(bitSet);
    }
    प्रत्यागम idx;
}

File_पङ्क्तिः_Singleton::सन्देशः_En_Set_Idx_t File_पङ्क्तिः_Singleton::default_सन्देशः_En_Index() VL_MT_SAFE {
    सन्देशः_En_Bit_Set सन्देशः_En_Bit_Set_2;
    प्रति (पूर्णाङ्क i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ध्रुव V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        सन्देशः_En_Bit_Set_2.set(सन्देशः_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        सन्देशः_En_Bit_Set_2.set(सन्देशः_En_Bit_Set::Subset::CTRL, code, सच्च);
    }
    प्रत्यागम add_सन्देशः_En_Bit_Set(सन्देशः_En_Bit_Set_2);
}

File_पङ्क्तिः_Singleton::सन्देशः_En_Set_Idx_t File_पङ्क्तिः_Singleton::सन्देशः_En_Set_Bit(सन्देशः_En_Set_Idx_t setIdx,
                                                                सन्देशः_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, तार्किक मानम्) {
    // See if state matches existing
    तार्किक same = सच्च;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        जइ (सन्देशः_En(setIdx).test(subset, subcode) != मानम्) same = मिच्छ;
    });
    जइ (same) प्रत्यागम setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    सन्देशः_En_Bit_Set सन्देशः_En_Bit_Set_2{सन्देशः_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { सन्देशः_En_Bit_Set_2.set(subset, subcode, मानम्); });
    प्रत्यागम add_सन्देशः_En_Bit_Set(सन्देशः_En_Bit_Set_2);
}

File_पङ्क्तिः_Singleton::सन्देशः_En_Set_Idx_t File_पङ्क्तिः_Singleton::सन्देशः_Set_Ctrl_Bit_Set(सन्देशः_En_Set_Idx_t setIdx,
                                                                     ध्रुव VErrorBitSet& bitset) {
    ध्रुव सन्देशः_En_Bit_Set::Subset subset = सन्देशः_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    जइ (सन्देशः_En(setIdx).getAll(subset) == bitset) प्रत्यागम setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    सन्देशः_En_Bit_Set सन्देशः_En_Bit_Set_2{सन्देशः_En(setIdx)};
    सन्देशः_En_Bit_Set_2.setAll(subset, bitset);
    प्रत्यागम add_सन्देशः_En_Bit_Set(सन्देशः_En_Bit_Set_2);
}

File_पङ्क्तिः_Singleton::सन्देशः_En_Set_Idx_t File_पङ्क्तिः_Singleton::सन्देशः_En_And(सन्देशः_En_Set_Idx_t lhsIdx,
                                                             सन्देशः_En_Set_Idx_t rhsIdx) {
    ध्रुव सन्देशः_En_Bit_Set& lhs = सन्देशः_En(lhsIdx);
    ध्रुव सन्देशः_En_Bit_Set& rhs = सन्देशः_En(rhsIdx);
    ध्रुव सन्देशः_En_Bit_Set intersection{lhs, rhs};
    जइ (intersection == lhs) प्रत्यागम lhsIdx;
    जइ (intersection == rhs) प्रत्यागम rhsIdx;
    प्रत्यागम add_सन्देशः_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

शून्य VFileContent::प्रक्षिप_Text(ध्रुव string& text) {
    // Similar code in WildcardContents::pushText()
    जइ (m_पङ्क्तयः.size() == 0) {
        m_पङ्क्तयः.emplace_back("");  // no such thing as line [0]
        m_पङ्क्तयः.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ध्रुव string leftover = m_पङ्क्तयः.back() + text;
    m_पङ्क्तयः.निष्कासय_back();

    // Insert line-by-line
    string::size_type पङ्क्तिः_start = 0;
    जाव (सच्च) {
        ध्रुव string::size_type पङ्क्तिः_end = leftover.find('\n', पङ्क्तिः_start);
        जइ (पङ्क्तिः_end != string::npos) {
            ध्रुव string oneline(leftover, पङ्क्तिः_start, पङ्क्तिः_end - पङ्क्तिः_start + 1);
            m_पङ्क्तयः.प्रक्षिप_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_पङ्क्तयः.size() - 1) << "]: " << oneline);
            पङ्क्तिः_start = पङ्क्तिः_end + 1;
        } अण्णहा {
            भङ्ग;
        }
    }
    // Keep leftover for next time
    m_पङ्क्तयः.emplace_back(string(leftover, पङ्क्तिः_start));  // Might be ""
}

string VFileContent::get_पङ्क्तिः(पूर्णाङ्क lineno) ध्रुव VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    जइ (VL_UNCOVERABLE(lineno < 0 || lineno >= (पूर्णाङ्क)m_पङ्क्तयः.size())) {
        जइ (debug() || v3Global.opt.debugCheck()) {
            प्रत्यागम ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } अण्णहा {
            प्रत्यागम "";
        }
    }
    string text = m_पङ्क्तयः[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    प्रत्यागम text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    जइ (!contentp) {
        os << "ct0";
    } अण्णहा {
        os << contentp->ascii();
    }
    प्रत्यागम os;
}

// ######################################################################
//  FileLine class functions

File_पङ्क्तिः::~File_पङ्क्तिः() {
    जइ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

शून्य File_पङ्क्तिः::newContent() {
    जइ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = नूतन VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_पङ्क्तिः::पङ्क्तिः_Directive_Strg(पूर्णाङ्क enterExit) ध्रुव {
    प्रत्यागम "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

शून्य File_पङ्क्तिः::पङ्क्तिः_Directive(ध्रुव वर्ण* textp, पूर्णाङ्क& enterExitRef) {
    string newFilename;
    पूर्णाङ्क newLineno = -1;
    पङ्क्तिः_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    जइ (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } अण्णहा {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

शून्य File_पङ्क्तिः::पङ्क्तिः_Directive_Parse(ध्रुव वर्ण* textp, string& filenameRef, पूर्णाङ्क& linenoRef,
                                  पूर्णाङ्क& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    करेहि {
        // Skip `line
        जाव (*textp && std::isspace(*textp)) ++textp;
        जाव (*textp && !std::isspace(*textp)) ++textp;
        जाव (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        पूर्णाङ्क पङ्क्तिः_No;
        ध्रुव वर्ण* ध्रुव ln = textp;
        जाव (*textp && !std::isspace(*textp)) ++textp;
        जइ (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            पङ्क्तिः_No = lineno() + 1;
        } अण्णहा जइ (std::isdigit(*ln)) {
            पङ्क्तिः_No = std::atoi(ln);
        } अण्णहा {
            भङ्ग;  // Fail
        }
        जाव (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        जइ (*textp != '"') भङ्ग;  // Fail
        ध्रुव वर्ण* ध्रुव fn = ++textp;
        जाव (*textp && *textp != '"') ++textp;
        जइ (*textp != '"') भङ्ग;  // Fail
        string err_सन्देशः;
        ध्रुव string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_सन्देशः);
        जइ (!err_सन्देशः.empty()) एतत्->v3error(err_सन्देशः.c_str());
        ++textp;
        जाव (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        जइ (!std::isdigit(*textp)) भङ्ग;  // Fail
        ध्रुव पूर्णाङ्क level = std::atoi(textp);
        जइ (level < 0 || level >= 3) भङ्ग;  // Fail

        linenoRef = पङ्क्तिः_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        प्रत्यागम;
    } जाव (मिच्छ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

शून्य File_पङ्क्तिः::forwardToken(ध्रुव वर्ण* textp, size_t size, तार्किक track_पङ्क्तयः) {
    स्थिर पूर्णाङ्क s_token_अङ्कः = 1;
    m_token_अङ्कः = s_token_अङ्कः++;
    प्रति (ध्रुव वर्ण* sp = textp; size && *sp; ++sp, --size) {
        जइ (*sp == '\n') {
            जइ (track_पङ्क्तयः) linenoInc();
            m_lastColumn = 1;
        } अण्णहा जइ (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } अण्णहा {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

शून्य File_पङ्क्तिः::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(एतत्);  // Toggle warnings based on global config file
#endif
}

File_पङ्क्तिः* File_पङ्क्तिः::copy_Or_Same_File_पङ्क्तिः_Applied() {
    applyIgnores();
    प्रत्यागम copy_Or_Same_File_पङ्क्तिः();
}

File_पङ्क्तिः* File_पङ्क्तिः::copy_Or_Same_File_पङ्क्तिः() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    स्थिर File_पङ्क्तिः* s_lastNewp = nullptr;
    जइ (s_lastNewp && *s_lastNewp == *एतत्) {  // Compares lineno, filename, etc
        प्रत्यागम s_lastNewp;
    }
    File_पङ्क्तिः* ध्रुव newp = नूतन File_पङ्क्तिः{एतत्};
    s_lastNewp = newp;
    प्रत्यागम newp;
}

string File_पङ्क्तिः::filebasename() ध्रुव VL_MT_SAFE { प्रत्यागम V3Os::filenameNonDir(filename()); }

string File_पङ्क्तिः::filebasenameNoExt() ध्रुव { प्रत्यागम V3Os::filenameNonDirExt(filename()); }

string File_पङ्क्तिः::firstColumnLetters() ध्रुव VL_MT_SAFE {
    ध्रुव वर्ण a = ((firstColumn() / 26) % 26) + 'a';
    ध्रुव वर्ण b = (firstColumn() % 26) + 'a';
    प्रत्यागम string(1, a) + string(1, b);
}

string File_पङ्क्तिः::profileFuncname() ध्रुव {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    जाव ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    प्रत्यागम name;
}

string File_पङ्क्तिः::ascii_पङ्क्तिः_Col() ध्रुव {
    प्रत्यागम (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_पङ्क्तिः::ascii() ध्रुव {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    प्रत्यागम filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_पङ्क्तिः* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    प्रत्यागम (os);
}

string File_पङ्क्तिः::warnOffParse(ध्रुव string& msgs, तार्किक turnOff) {
    string result;
    प्रति (ध्रुव string& सन्देशः : VString::split(msgs, ',')) {
        ध्रुव V3ErrorCode code{सन्देशः};
        जइ (!code.hardError()) {
            warnOff(code, turnOff);
            अनुवर्त;
        }
        // Error if not suppressed
        जइ (!v3Global.opt.isFuture(सन्देशः)) result = VString::dot(result, ",", सन्देशः);
    }
    प्रत्यागम result;
}

तार्किक File_पङ्क्तिः::warnIsOff(V3ErrorCode code) ध्रुव {
    जइ (!सन्देशः_En().enabled(code)) प्रत्यागम सच्च;
    जइ (!default_File_पङ्क्तिः().सन्देशः_En().enabled(code)) प्रत्यागम सच्च;  // Global overrides local
    प्रत्यागम मिच्छ;
}

// cppverilator-suppress constParameter
शून्य File_पङ्क्तिः::v3errorEnd(std::ostringstream& sstr, ध्रुव string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    जइ (lastLineno()) nsstr << V3Error::warnContextBegin() << एतत् << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    जइ (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    जइ (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } अण्णहा {
        जइ (!V3Error::s().errorContexted()) {
            ध्रुव string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ध्रुव string waiverText = V3Error::stripMetaText(wsstr.str(), मिच्छ);
        m_waive = V3Control::waive(एतत्, V3Error::s().errorCode(), waiverText);
        जइ (m_waive) {
            V3Error::s().suppressThisWarning();
        } अण्णहा {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), एतत्);
}

string File_पङ्क्तिः::warnMore() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) {
    जइ (lastLineno()) {
        प्रत्यागम V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } अण्णहा {
        प्रत्यागम V3Error::warnMore();
    }
}
string File_पङ्क्तिः::warnOther() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) {
    जइ (lastLineno()) {
        प्रत्यागम V3Error::s().warnRelated(एतत्) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } अण्णहा {
        प्रत्यागम V3Error::warnMore();
    }
};

string File_पङ्क्तिः::source() ध्रुव VL_MT_SAFE {
    जइ (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        जइ (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            प्रत्यागम "\n%Error: internal tracking of file contents failed";
        } अण्णहा {
            प्रत्यागम "";
        }
    }  // LCOV_EXCL_STOP
    प्रत्यागम m_contentp->get_पङ्क्तिः(m_contentLineno);
}
string File_पङ्क्तिः::sourcePrefix(पूर्णाङ्क toColumn) ध्रुव VL_MT_SAFE {
    ध्रुव std::string src = source();
    जइ (toColumn > static_cast<पूर्णाङ्क>(src.length())) toColumn = static_cast<पूर्णाङ्क>(src.length());
    जइ (toColumn < 1) प्रत्यागम "";
    प्रत्यागम src.substr(0, toColumn - 1);
}
string File_पङ्क्तिः::prettySource() ध्रुव VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ध्रुव string::size_type pos = out.find('\n');
    जइ (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    प्रत्यागम VString::spaceUnprintable(out);
}

string File_पङ्क्तिः::warnContext() ध्रुव {
    जइ (!v3Global.opt.context()) प्रत्यागम "";
    जइ (firstLineno() == lastLineno() && firstColumn()) {
        ध्रुव string source_पङ्क्तिः = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        जइ (!source_पङ्क्तिः.empty() && source_पङ्क्तिः.length() < SHOW_SOURCE_MAX_LENGTH
            && source_पङ्क्तिः.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            जाव (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_पङ्क्तिः + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            जइ (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        प्रत्यागम V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    प्रत्यागम "";
}

string File_पङ्क्तिः::warnContextParent() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    प्रति (File_पङ्क्तिः* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        जइ (parentFl->filenameIsGlobal()) भङ्ग;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    प्रत्यागम result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_पङ्क्तिः*> file_पङ्क्तिः_Leak_Checks;

शून्य* File_पङ्क्तिः::operator नूतन(size_t size) {
    File_पङ्क्तिः* ध्रुव objp = static_cast<File_पङ्क्तिः*>(::operator नूतन(size));
    file_पङ्क्तिः_Leak_Checks.निवेशय(objp);
    प्रत्यागम objp;
}

शून्य File_पङ्क्तिः::operator delete(शून्य* objp, size_t size) {
    जइ (!objp) प्रत्यागम;
    File_पङ्क्तिः* ध्रुव flp = static_cast<File_पङ्क्तिः*>(objp);
    ध्रुव auto it = file_पङ्क्तिः_Leak_Checks.find(flp);
    जइ (it != file_पङ्क्तिः_Leak_Checks.end()) {
        file_पङ्क्तिः_Leak_Checks.erase(it);
    } अण्णहा {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

शून्य File_पङ्क्तिः::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_सन्देशः_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

शून्य File_पङ्क्तिः::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    जाव (सच्च) {
        ध्रुव auto it = file_पङ्क्तिः_Leak_Checks.begin();
        जइ (it == file_पङ्क्तिः_Leak_Checks.end()) भङ्ग;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_पङ्क्तिः_Leak_Checks.clear();
    singleton().clear();
#endif
}
