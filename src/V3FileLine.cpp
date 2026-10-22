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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

// clang-format off
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#ಅನ್ತರ್ಭಾವ "V3Waiver.h"
// clang-format on

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_ಸಾಲು_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr ಪೂರ್ಣಾಙ್ಕ size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    ವರ್ಣ out[size];
    ವರ್ಣ* op = out + size - 1;
    *--op = '\0';  // We build backwards
    ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ = fileno;
    ಕುರು {
        *--op = 'a' + ಅಂಕೆ % 26;
        ಅಂಕೆ /= 26;
    } ಯಾವತ್ (ಅಂಕೆ);
    ಪ್ರತ್ಯಾಗಮ op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_ಸಾಲು_Singleton::fileNameIdx_t File_ಸಾಲು_Singleton::name_To_ಸಂಖ್ಯೆ(ಧ್ರುವ string& filename) {
    ಧ್ರುವ auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    ಯದಿ (pair.second) {
        ಧ್ರುವ size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.ತಳ್ಳು_back(filename);
        m_languages.ತಳ್ಳು_back(V3LangCode::mostRecent());
    }
    ಪ್ರತ್ಯಾಗಮ idx;
}

ಶೂನ್ಯ File_ಸಾಲು_Singleton::file_Name_ಅಂಕೆ_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_namemap) {
        std::string filename = itr.first;
        ಯದಿ (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } ಅನ್ಯಥಾ ಯದಿ (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << ಸಂಖ್ಯೆ_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_Set_Idx_t File_ಸಾಲು_Singleton::add_ಸಂದೇಶ_En_Bit_Set(ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ಧ್ರುವ V3LockGuard lock{m_mutex};
    ಧ್ರುವ auto pair = m_interned_ಸಂದೇಶ_En_Idxs.emplace(bitSet, 0);
    ಸಂದೇಶ_En_Set_Idx_t& idx = pair.first->second;
    ಯದಿ (pair.second) {
        ಧ್ರುವ size_t nextIdx = m_interned_ಸಂದೇಶ_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<ಸಂದೇಶ_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<ಸಂದೇಶ_En_Set_Idx_t>(nextIdx);
        m_interned_ಸಂದೇಶ_Ens.ತಳ್ಳು_back(bitSet);
    }
    ಪ್ರತ್ಯಾಗಮ idx;
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_Set_Idx_t File_ಸಾಲು_Singleton::default_ಸಂದೇಶ_En_Index() VL_MT_SAFE {
    ಸಂದೇಶ_En_Bit_Set ಸಂದೇಶ_En_Bit_Set_2;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ಧ್ರುವ V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        ಸಂದೇಶ_En_Bit_Set_2.set(ಸಂದೇಶ_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        ಸಂದೇಶ_En_Bit_Set_2.set(ಸಂದೇಶ_En_Bit_Set::Subset::CTRL, code, ಸತ್ಯ);
    }
    ಪ್ರತ್ಯಾಗಮ add_ಸಂದೇಶ_En_Bit_Set(ಸಂದೇಶ_En_Bit_Set_2);
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_Set_Idx_t File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_Set_Bit(ಸಂದೇಶ_En_Set_Idx_t setIdx,
                                                                ಸಂದೇಶ_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
    // See if state matches existing
    ತಾರ್ಕಿಕ same = ಸತ್ಯ;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        ಯದಿ (ಸಂದೇಶ_En(setIdx).test(subset, subcode) != ಮೌಲ್ಯ) same = ಅಸತ್ಯ;
    });
    ಯದಿ (same) ಪ್ರತ್ಯಾಗಮ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    ಸಂದೇಶ_En_Bit_Set ಸಂದೇಶ_En_Bit_Set_2{ಸಂದೇಶ_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { ಸಂದೇಶ_En_Bit_Set_2.set(subset, subcode, ಮೌಲ್ಯ); });
    ಪ್ರತ್ಯಾಗಮ add_ಸಂದೇಶ_En_Bit_Set(ಸಂದೇಶ_En_Bit_Set_2);
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_Set_Idx_t File_ಸಾಲು_Singleton::ಸಂದೇಶ_Set_Ctrl_Bit_Set(ಸಂದೇಶ_En_Set_Idx_t setIdx,
                                                                     ಧ್ರುವ VErrorBitSet& bitset) {
    ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set::Subset subset = ಸಂದೇಶ_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    ಯದಿ (ಸಂದೇಶ_En(setIdx).getAll(subset) == bitset) ಪ್ರತ್ಯಾಗಮ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    ಸಂದೇಶ_En_Bit_Set ಸಂದೇಶ_En_Bit_Set_2{ಸಂದೇಶ_En(setIdx)};
    ಸಂದೇಶ_En_Bit_Set_2.setAll(subset, bitset);
    ಪ್ರತ್ಯಾಗಮ add_ಸಂದೇಶ_En_Bit_Set(ಸಂದೇಶ_En_Bit_Set_2);
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_Set_Idx_t File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_And(ಸಂದೇಶ_En_Set_Idx_t lhsIdx,
                                                             ಸಂದೇಶ_En_Set_Idx_t rhsIdx) {
    ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& lhs = ಸಂದೇಶ_En(lhsIdx);
    ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& rhs = ಸಂದೇಶ_En(rhsIdx);
    ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set intersection{lhs, rhs};
    ಯದಿ (intersection == lhs) ಪ್ರತ್ಯಾಗಮ lhsIdx;
    ಯದಿ (intersection == rhs) ಪ್ರತ್ಯಾಗಮ rhsIdx;
    ಪ್ರತ್ಯಾಗಮ add_ಸಂದೇಶ_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

ಶೂನ್ಯ VFileContent::ತಳ್ಳು_Text(ಧ್ರುವ string& text) {
    // Similar code in WildcardContents::pushText()
    ಯದಿ (m_ಸಾಲುಗಳು.size() == 0) {
        m_ಸಾಲುಗಳು.emplace_back("");  // no such thing as line [0]
        m_ಸಾಲುಗಳು.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ಧ್ರುವ string leftover = m_ಸಾಲುಗಳು.back() + text;
    m_ಸಾಲುಗಳು.ತೆಗೆ_back();

    // Insert line-by-line
    string::size_type ಸಾಲು_start = 0;
    ಯಾವತ್ (ಸತ್ಯ) {
        ಧ್ರುವ string::size_type ಸಾಲು_end = leftover.find('\n', ಸಾಲು_start);
        ಯದಿ (ಸಾಲು_end != string::npos) {
            ಧ್ರುವ string oneline(leftover, ಸಾಲು_start, ಸಾಲು_end - ಸಾಲು_start + 1);
            m_ಸಾಲುಗಳು.ತಳ್ಳು_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_ಸಾಲುಗಳು.size() - 1) << "]: " << oneline);
            ಸಾಲು_start = ಸಾಲು_end + 1;
        } ಅನ್ಯಥಾ {
            ಭಙ್ಗ;
        }
    }
    // Keep leftover for next time
    m_ಸಾಲುಗಳು.emplace_back(string(leftover, ಸಾಲು_start));  // Might be ""
}

string VFileContent::get_ಸಾಲು(ಪೂರ್ಣಾಙ್ಕ lineno) ಧ್ರುವ VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    ಯದಿ (VL_UNCOVERABLE(lineno < 0 || lineno >= (ಪೂರ್ಣಾಙ್ಕ)m_ಸಾಲುಗಳು.size())) {
        ಯದಿ (debug() || v3Global.opt.debugCheck()) {
            ಪ್ರತ್ಯಾಗಮ ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ "";
        }
    }
    string text = m_ಸಾಲುಗಳು[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    ಪ್ರತ್ಯಾಗಮ text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    ಯದಿ (!contentp) {
        os << "ct0";
    } ಅನ್ಯಥಾ {
        os << contentp->ascii();
    }
    ಪ್ರತ್ಯಾಗಮ os;
}

// ######################################################################
//  FileLine class functions

File_ಸಾಲು::~File_ಸಾಲು() {
    ಯದಿ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

ಶೂನ್ಯ File_ಸಾಲು::newContent() {
    ಯದಿ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = ನೂತನ VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_ಸಾಲು::ಸಾಲು_Directive_Strg(ಪೂರ್ಣಾಙ್ಕ enterExit) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

ಶೂನ್ಯ File_ಸಾಲು::ಸಾಲು_Directive(ಧ್ರುವ ವರ್ಣ* textp, ಪೂರ್ಣಾಙ್ಕ& enterExitRef) {
    string newFilename;
    ಪೂರ್ಣಾಙ್ಕ newLineno = -1;
    ಸಾಲು_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    ಯದಿ (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } ಅನ್ಯಥಾ {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

ಶೂನ್ಯ File_ಸಾಲು::ಸಾಲು_Directive_Parse(ಧ್ರುವ ವರ್ಣ* textp, string& filenameRef, ಪೂರ್ಣಾಙ್ಕ& linenoRef,
                                  ಪೂರ್ಣಾಙ್ಕ& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    ಕುರು {
        // Skip `line
        ಯಾವತ್ (*textp && std::isspace(*textp)) ++textp;
        ಯಾವತ್ (*textp && !std::isspace(*textp)) ++textp;
        ಯಾವತ್ (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        ಪೂರ್ಣಾಙ್ಕ ಸಾಲು_No;
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ ln = textp;
        ಯಾವತ್ (*textp && !std::isspace(*textp)) ++textp;
        ಯದಿ (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            ಸಾಲು_No = lineno() + 1;
        } ಅನ್ಯಥಾ ಯದಿ (std::isdigit(*ln)) {
            ಸಾಲು_No = std::atoi(ln);
        } ಅನ್ಯಥಾ {
            ಭಙ್ಗ;  // Fail
        }
        ಯಾವತ್ (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        ಯದಿ (*textp != '"') ಭಙ್ಗ;  // Fail
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ fn = ++textp;
        ಯಾವತ್ (*textp && *textp != '"') ++textp;
        ಯದಿ (*textp != '"') ಭಙ್ಗ;  // Fail
        string err_ಸಂದೇಶ;
        ಧ್ರುವ string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_ಸಂದೇಶ);
        ಯದಿ (!err_ಸಂದೇಶ.empty()) ಏತತ್->v3error(err_ಸಂದೇಶ.c_str());
        ++textp;
        ಯಾವತ್ (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        ಯದಿ (!std::isdigit(*textp)) ಭಙ್ಗ;  // Fail
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ level = std::atoi(textp);
        ಯದಿ (level < 0 || level >= 3) ಭಙ್ಗ;  // Fail

        linenoRef = ಸಾಲು_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        ಪ್ರತ್ಯಾಗಮ;
    } ಯಾವತ್ (ಅಸತ್ಯ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

ಶೂನ್ಯ File_ಸಾಲು::forwardToken(ಧ್ರುವ ವರ್ಣ* textp, size_t size, ತಾರ್ಕಿಕ track_ಸಾಲುಗಳು) {
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_token_ಅಂಕೆ = 1;
    m_token_ಅಂಕೆ = s_token_ಅಂಕೆ++;
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* sp = textp; size && *sp; ++sp, --size) {
        ಯದಿ (*sp == '\n') {
            ಯದಿ (track_ಸಾಲುಗಳು) linenoInc();
            m_lastColumn = 1;
        } ಅನ್ಯಥಾ ಯದಿ (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } ಅನ್ಯಥಾ {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

ಶೂನ್ಯ File_ಸಾಲು::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(ಏತತ್);  // Toggle warnings based on global config file
#endif
}

File_ಸಾಲು* File_ಸಾಲು::copy_Or_Same_File_ಸಾಲು_Applied() {
    applyIgnores();
    ಪ್ರತ್ಯಾಗಮ copy_Or_Same_File_ಸಾಲು();
}

File_ಸಾಲು* File_ಸಾಲು::copy_Or_Same_File_ಸಾಲು() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ಸ್ಥಿರ File_ಸಾಲು* s_lastNewp = nullptr;
    ಯದಿ (s_lastNewp && *s_lastNewp == *ಏತತ್) {  // Compares lineno, filename, etc
        ಪ್ರತ್ಯಾಗಮ s_lastNewp;
    }
    File_ಸಾಲು* ಧ್ರುವ newp = ನೂತನ File_ಸಾಲು{ಏತತ್};
    s_lastNewp = newp;
    ಪ್ರತ್ಯಾಗಮ newp;
}

string File_ಸಾಲು::filebasename() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ V3Os::filenameNonDir(filename()); }

string File_ಸಾಲು::filebasenameNoExt() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ V3Os::filenameNonDirExt(filename()); }

string File_ಸಾಲು::firstColumnLetters() ಧ್ರುವ VL_MT_SAFE {
    ಧ್ರುವ ವರ್ಣ a = ((firstColumn() / 26) % 26) + 'a';
    ಧ್ರುವ ವರ್ಣ b = (firstColumn() % 26) + 'a';
    ಪ್ರತ್ಯಾಗಮ string(1, a) + string(1, b);
}

string File_ಸಾಲು::profileFuncname() ಧ್ರುವ {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    ಯಾವತ್ ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    ಪ್ರತ್ಯಾಗಮ name;
}

string File_ಸಾಲು::ascii_ಸಾಲು_Col() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_ಸಾಲು::ascii() ಧ್ರುವ {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    ಪ್ರತ್ಯಾಗಮ filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_ಸಾಲು* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    ಪ್ರತ್ಯಾಗಮ (os);
}

string File_ಸಾಲು::warnOffParse(ಧ್ರುವ string& msgs, ತಾರ್ಕಿಕ turnOff) {
    string result;
    ಪ್ರತಿ (ಧ್ರುವ string& ಸಂದೇಶ : VString::split(msgs, ',')) {
        ಧ್ರುವ V3ErrorCode code{ಸಂದೇಶ};
        ಯದಿ (!code.hardError()) {
            warnOff(code, turnOff);
            ಅನುವರ್ತ;
        }
        // Error if not suppressed
        ಯದಿ (!v3Global.opt.isFuture(ಸಂದೇಶ)) result = VString::dot(result, ",", ಸಂದೇಶ);
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

ತಾರ್ಕಿಕ File_ಸಾಲು::warnIsOff(V3ErrorCode code) ಧ್ರುವ {
    ಯದಿ (!ಸಂದೇಶ_En().enabled(code)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    ಯದಿ (!default_File_ಸಾಲು().ಸಂದೇಶ_En().enabled(code)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Global overrides local
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

// cppverilator-suppress constParameter
ಶೂನ್ಯ File_ಸಾಲು::v3errorEnd(std::ostringstream& sstr, ಧ್ರುವ string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    ಯದಿ (lastLineno()) nsstr << V3Error::warnContextBegin() << ಏತತ್ << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    ಯದಿ (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    ಯದಿ (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } ಅನ್ಯಥಾ {
        ಯದಿ (!V3Error::s().errorContexted()) {
            ಧ್ರುವ string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ಧ್ರುವ string waiverText = V3Error::stripMetaText(wsstr.str(), ಅಸತ್ಯ);
        m_waive = V3Control::waive(ಏತತ್, V3Error::s().errorCode(), waiverText);
        ಯದಿ (m_waive) {
            V3Error::s().suppressThisWarning();
        } ಅನ್ಯಥಾ {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), ಏತತ್);
}

string File_ಸಾಲು::warnMore() ಧ್ರುವ VL_REQUIRES(V3Error::s().m_mutex) {
    ಯದಿ (lastLineno()) {
        ಪ್ರತ್ಯಾಗಮ V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ V3Error::warnMore();
    }
}
string File_ಸಾಲು::warnOther() ಧ್ರುವ VL_REQUIRES(V3Error::s().m_mutex) {
    ಯದಿ (lastLineno()) {
        ಪ್ರತ್ಯಾಗಮ V3Error::s().warnRelated(ಏತತ್) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ V3Error::warnMore();
    }
};

string File_ಸಾಲು::source() ಧ್ರುವ VL_MT_SAFE {
    ಯದಿ (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        ಯದಿ (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            ಪ್ರತ್ಯಾಗಮ "\n%Error: internal tracking of file contents failed";
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ "";
        }
    }  // LCOV_EXCL_STOP
    ಪ್ರತ್ಯಾಗಮ m_contentp->get_ಸಾಲು(m_contentLineno);
}
string File_ಸಾಲು::sourcePrefix(ಪೂರ್ಣಾಙ್ಕ toColumn) ಧ್ರುವ VL_MT_SAFE {
    ಧ್ರುವ std::string src = source();
    ಯದಿ (toColumn > static_cast<ಪೂರ್ಣಾಙ್ಕ>(src.length())) toColumn = static_cast<ಪೂರ್ಣಾಙ್ಕ>(src.length());
    ಯದಿ (toColumn < 1) ಪ್ರತ್ಯಾಗಮ "";
    ಪ್ರತ್ಯಾಗಮ src.substr(0, toColumn - 1);
}
string File_ಸಾಲು::prettySource() ಧ್ರುವ VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ಧ್ರುವ string::size_type pos = out.find('\n');
    ಯದಿ (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    ಪ್ರತ್ಯಾಗಮ VString::spaceUnprintable(out);
}

string File_ಸಾಲು::warnContext() ಧ್ರುವ {
    ಯದಿ (!v3Global.opt.context()) ಪ್ರತ್ಯಾಗಮ "";
    ಯದಿ (firstLineno() == lastLineno() && firstColumn()) {
        ಧ್ರುವ string source_ಸಾಲು = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        ಯದಿ (!source_ಸಾಲು.empty() && source_ಸಾಲು.length() < SHOW_SOURCE_MAX_LENGTH
            && source_ಸಾಲು.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            ಯಾವತ್ (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_ಸಾಲು + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            ಯದಿ (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        ಪ್ರತ್ಯಾಗಮ V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    ಪ್ರತ್ಯಾಗಮ "";
}

string File_ಸಾಲು::warnContextParent() ಧ್ರುವ VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    ಪ್ರತಿ (File_ಸಾಲು* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        ಯದಿ (parentFl->filenameIsGlobal()) ಭಙ್ಗ;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_ಸಾಲು*> file_ಸಾಲು_Leak_Checks;

ಶೂನ್ಯ* File_ಸಾಲು::operator ನೂತನ(size_t size) {
    File_ಸಾಲು* ಧ್ರುವ objp = static_cast<File_ಸಾಲು*>(::operator ನೂತನ(size));
    file_ಸಾಲು_Leak_Checks.ಸೇರಿಸು(objp);
    ಪ್ರತ್ಯಾಗಮ objp;
}

ಶೂನ್ಯ File_ಸಾಲು::operator delete(ಶೂನ್ಯ* objp, size_t size) {
    ಯದಿ (!objp) ಪ್ರತ್ಯಾಗಮ;
    File_ಸಾಲು* ಧ್ರುವ flp = static_cast<File_ಸಾಲು*>(objp);
    ಧ್ರುವ auto it = file_ಸಾಲು_Leak_Checks.find(flp);
    ಯದಿ (it != file_ಸಾಲು_Leak_Checks.end()) {
        file_ಸಾಲು_Leak_Checks.erase(it);
    } ಅನ್ಯಥಾ {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

ಶೂನ್ಯ File_ಸಾಲು::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_ಸಂದೇಶ_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

ಶೂನ್ಯ File_ಸಾಲು::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    ಯಾವತ್ (ಸತ್ಯ) {
        ಧ್ರುವ auto it = file_ಸಾಲು_Leak_Checks.begin();
        ಯದಿ (it == file_ಸಾಲು_Leak_Checks.end()) ಭಙ್ಗ;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_ಸಾಲು_Leak_Checks.clear();
    singleton().clear();
#endif
}
