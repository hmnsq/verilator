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
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ "V3Waiver.h"
// clang-format on

#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <limits>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_ಸಾಲು_Singleton::filenameLetters(fileNameIdx_t fileno) VL_ಶುದ್ಧ {
    constexpr ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    ಅಕ್ಷರ ಹೊರಹೋಗು[ಗಾತ್ರ_2];
    ಅಕ್ಷರ* op = ಹೊರಹೋಗು + ಗಾತ್ರ_2 - 1;
    *--op = '\0';  // We build backwards
    ಪೂರ್ಣಾಂಕ ಅಂಕೆ = fileno;
    ಮಾಡು {
        *--op = 'a' + ಅಂಕೆ % 26;
        ಅಂಕೆ /= 26;
    } ಆಗ (ಅಂಕೆ);
    ಹಿಂತಿರುಗಿ op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_ಸಾಲು_Singleton::fileNameIdx_t File_ಸಾಲು_Singleton::name_ಗಮ್ಯ_ಸಂಖ್ಯೆ(ಸ್ಥಿರಾಂಕ string& filename) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.ಮೊದಲ->ಎರಡನೇ;
    ಆದರೆ (pair.ಎರಡನೇ) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮುಂದಿನ_Idx = m_ಹೆಸರುಗಳು.ಗಾತ್ರ_2();
        UASSERT(ಮುಂದಿನ_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_ಗಮ್ಯ_Str(ಮುಂದಿನ_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(ಮುಂದಿನ_Idx);
        m_ಹೆಸರುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(filename);
        m_languages.ತಳ್ಳು_ಹಿಂಭಾಗ(V3_Lang_ಸಂಕೇತ::mostRecent());
    }
    ಹಿಂತಿರುಗಿ idx;
}

ಶೂನ್ಯ File_ಸಾಲು_Singleton::file_Name_ಅಂಕೆ_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& itr : m_namemap) {
        std::string filename = itr.ಮೊದಲ;
        ಆದರೆ (filename == V3Options::get_Std_Package_ಮಾರ್ಗ()) {
            filename = "<verilated_std>";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (filename == V3Options::get_Std_Waiver_ಮಾರ್ಗ()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.ಎರಡನೇ) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(V3Os::filename_Real_ಮಾರ್ಗ(itr.ಮೊದಲ)) << '"'
           << ", \"language\":\"" << ಸಂಖ್ಯೆ_ಗಮ್ಯ_Lang(itr.ಎರಡನೇ).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_ಗಣ_Idx_t File_ಸಾಲು_Singleton::ಕೂಡಿಸು_ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ(ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& ಬಿಟ್_ಗಣ)
    VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex) {
    ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{m_mutex};
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_interned_ಸಂದೇಶ_En_Idxs.emplace(ಬಿಟ್_ಗಣ, 0);
    ಸಂದೇಶ_En_ಗಣ_Idx_t& idx = pair.ಮೊದಲ->ಎರಡನೇ;
    ಆದರೆ (pair.ಎರಡನೇ) {
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಮುಂದಿನ_Idx = m_interned_ಸಂದೇಶ_Ens.ಗಾತ್ರ_2();
        UASSERT(ಮುಂದಿನ_Idx <= std::numeric_limits<ಸಂದೇಶ_En_ಗಣ_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_ಗಮ್ಯ_Str(ಮುಂದಿನ_Idx) + "+).");
        idx = static_cast<ಸಂದೇಶ_En_ಗಣ_Idx_t>(ಮುಂದಿನ_Idx);
        m_interned_ಸಂದೇಶ_Ens.ತಳ್ಳು_ಹಿಂಭಾಗ(ಬಿಟ್_ಗಣ);
    }
    ಹಿಂತಿರುಗಿ idx;
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_ಗಣ_Idx_t File_ಸಾಲು_Singleton::default_ಸಂದೇಶ_En_ಸೂಚ್ಯಂಕ() VL_MT_ಸುರಕ್ಷಿತ {
    ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = V3_Error_ಸಂಕೇತ::EC_MIN; i < V3_Error_ಸಂಕೇತ::_ENUM_MAX; ++i) {
        ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2.ಗಣ(ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Subset::ಸಂಕೇತ_3, ಸಂಕೇತ_2, !ಸಂಕೇತ_2.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2.ಗಣ(ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Subset::CTRL, ಸಂಕೇತ_2, ನಿಜ);
    }
    ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ(ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2);
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_ಗಣ_Idx_t File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_ಗಣ_ಬಿಟ್(ಸಂದೇಶ_En_ಗಣ_Idx_t ಗಣ_Idx,
                                                                ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Subset subset,
                                                                V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ತಾರ್ಕಿಕ ಮೌಲ್ಯ) {
    // See if state matches existing
    ತಾರ್ಕಿಕ same = ನಿಜ;
    ಸಂಕೇತ_2.forDelegateCodes([&](V3_Error_ಸಂಕೇತ subcode) {
        ಆದರೆ (ಸಂದೇಶ_En(ಗಣ_Idx).test(subset, subcode) != ಮೌಲ್ಯ) same = ಸುಳ್ಳು;
    });
    ಆದರೆ (same) ಹಿಂತಿರುಗಿ ಗಣ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2{ಸಂದೇಶ_En(ಗಣ_Idx)};
    ಸಂಕೇತ_2.forDelegateCodes([&](V3_Error_ಸಂಕೇತ subcode) { ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2.ಗಣ(subset, subcode, ಮೌಲ್ಯ); });
    ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ(ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2);
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_ಗಣ_Idx_t File_ಸಾಲು_Singleton::ಸಂದೇಶ_ಗಣ_Ctrl_ಬಿಟ್_ಗಣ(ಸಂದೇಶ_En_ಗಣ_Idx_t ಗಣ_Idx,
                                                                     ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& bitset) {
    ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Subset subset = ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Subset::CTRL;
    // See if state matches existing
    ಆದರೆ (ಸಂದೇಶ_En(ಗಣ_Idx).getAll(subset) == bitset) ಹಿಂತಿರುಗಿ ಗಣ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2{ಸಂದೇಶ_En(ಗಣ_Idx)};
    ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2.ಗಣ_All(subset, bitset);
    ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ(ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ_2);
}

File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_ಗಣ_Idx_t File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_And(ಸಂದೇಶ_En_ಗಣ_Idx_t lhsIdx,
                                                             ಸಂದೇಶ_En_ಗಣ_Idx_t rhsIdx) {
    ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& lhs = ಸಂದೇಶ_En(lhsIdx);
    ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& rhs = ಸಂದೇಶ_En(rhsIdx);
    ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ intersection{lhs, rhs};
    ಆದರೆ (intersection == lhs) ಹಿಂತಿರುಗಿ lhsIdx;
    ಆದರೆ (intersection == rhs) ಹಿಂತಿರುಗಿ rhsIdx;
    ಹಿಂತಿರುಗಿ ಕೂಡಿಸು_ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ(intersection);
}

// ######################################################################
//  VFileContents class functions

ಶೂನ್ಯ VFileContent::ತಳ್ಳು_ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
    // Similar code in WildcardContents::pushText()
    ಆದರೆ (m_ಸಾಲುಗಳು.ಗಾತ್ರ_2() == 0) {
        m_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ("");  // no such thing as line [0]
        m_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ಸ್ಥಿರಾಂಕ string leftover = m_ಸಾಲುಗಳು.ಹಿಂಭಾಗ() + ಪಠ್ಯ;
    m_ಸಾಲುಗಳು.ತೆಗೆ_ಹಿಂಭಾಗ();

    // Insert line-by-line
    string::ಗಾತ್ರ_type ಸಾಲು_ಆರಂಭ = 0;
    ಆಗ (ನಿಜ) {
        ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type ಸಾಲು_ಕೊನೆ = leftover.ಕಂಡುಹಿಡಿ('\n', ಸಾಲು_ಆರಂಭ);
        ಆದರೆ (ಸಾಲು_ಕೊನೆ != string::npos) {
            ಸ್ಥಿರಾಂಕ string oneline(leftover, ಸಾಲು_ಆರಂಭ, ಸಾಲು_ಕೊನೆ - ಸಾಲು_ಆರಂಭ + 1);
            m_ಸಾಲುಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_ಸಾಲುಗಳು.ಗಾತ್ರ_2() - 1) << "]: " << oneline);
            ಸಾಲು_ಆರಂಭ = ಸಾಲು_ಕೊನೆ + 1;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ನಿಲ್ಲಿಸು;
        }
    }
    // Keep leftover for next time
    m_ಸಾಲುಗಳು.emplace_ಹಿಂಭಾಗ(string(leftover, ಸಾಲು_ಆರಂಭ));  // Might be ""
}

string VFileContent::get_ಸಾಲು(ಪೂರ್ಣಾಂಕ lineno) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    ಆದರೆ (VL_UNCOVERABLE(lineno < 0 || lineno >= (ಪೂರ್ಣಾಂಕ)m_ಸಾಲುಗಳು.ಗಾತ್ರ_2())) {
        ಆದರೆ (debug() || v3Global.opt.debugCheck()) {
            ಹಿಂತಿರುಗಿ ("%Error-internal-contents-bad-ct" + cvt_ಗಮ್ಯ_Str(m_id) + "-ln" + cvt_ಗಮ್ಯ_Str(lineno));
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ "";
        }
    }
    string ಪಠ್ಯ = m_ಸಾಲುಗಳು[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << ಪಠ್ಯ);
    ಹಿಂತಿರುಗಿ ಪಠ್ಯ;
}

std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, VFileContent* contentp) {
    ಆದರೆ (!contentp) {
        os << "ct0";
    } ಇಲ್ಲದಿದ್ದರೆ {
        os << contentp->ascii();
    }
    ಹಿಂತಿರುಗಿ os;
}

// ######################################################################
//  FileLine class functions

File_ಸಾಲು::~File_ಸಾಲು() {
    ಆದರೆ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

ಶೂನ್ಯ File_ಸಾಲು::newContent() {
    ಆದರೆ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = ಹೊಸ VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_ಸಾಲು::ಸಾಲು_Directive_Strg(ಪೂರ್ಣಾಂಕ enterExit) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ "`line "s + cvt_ಗಮ್ಯ_Str(ಕೊನೆಯ_Lineno()) + " \""
           + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(filename()) + "\" " + cvt_ಗಮ್ಯ_Str(enterExit) + '\n';
}

ಶೂನ್ಯ File_ಸಾಲು::ಸಾಲು_Directive(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಪೂರ್ಣಾಂಕ& enterExitRef) {
    string newFilename;
    ಪೂರ್ಣಾಂಕ newLineno = -1;
    ಸಾಲು_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    ಆದರೆ (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } ಇಲ್ಲದಿದ್ದರೆ {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

ಶೂನ್ಯ File_ಸಾಲು::ಸಾಲು_Directive_Parse(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, string& filenameRef, ಪೂರ್ಣಾಂಕ& linenoRef,
                                  ಪೂರ್ಣಾಂಕ& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    ಮಾಡು {
        // Skip `line
        ಆಗ (*textp && std::ಖಾಲಿಯೇ(*textp)) ++textp;
        ಆಗ (*textp && !std::ಖಾಲಿಯೇ(*textp)) ++textp;
        ಆಗ (*textp && std::ಖಾಲಿಯೇ(*textp)) ++textp;

        // Grab linenumber
        ಪೂರ್ಣಾಂಕ ಸಾಲು_No;
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ ln = textp;
        ಆಗ (*textp && !std::ಖಾಲಿಯೇ(*textp)) ++textp;
        ಆದರೆ (0 == ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            ಸಾಲು_No = lineno() + 1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (std::ಅಂಕಿಯೇ(*ln)) {
            ಸಾಲು_No = std::atoi(ln);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ನಿಲ್ಲಿಸು;  // Fail
        }
        ಆಗ (*textp && (std::ಖಾಲಿಯೇ(*textp))) ++textp;

        // Grab filename
        ಆದರೆ (*textp != '"') ನಿಲ್ಲಿಸು;  // Fail
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ fn = ++textp;
        ಆಗ (*textp && *textp != '"') ++textp;
        ಆದರೆ (*textp != '"') ನಿಲ್ಲಿಸು;  // Fail
        string err_ಸಂದೇಶ;
        ಸ್ಥಿರಾಂಕ string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_ಸಂದೇಶ);
        ಆದರೆ (!err_ಸಂದೇಶ.ಖಾಲಿಗಳು()) ಇದು->v3error(err_ಸಂದೇಶ.c_str());
        ++textp;
        ಆಗ (*textp && std::ಖಾಲಿಯೇ(*textp)) ++textp;

        // Grab level
        ಆದರೆ (!std::ಅಂಕಿಯೇ(*textp)) ನಿಲ್ಲಿಸು;  // Fail
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ level = std::atoi(textp);
        ಆದರೆ (level < 0 || level >= 3) ನಿಲ್ಲಿಸು;  // Fail

        linenoRef = ಸಾಲು_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        ಹಿಂತಿರುಗಿ;
    } ಆಗ (ಸುಳ್ಳು);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

ಶೂನ್ಯ File_ಸಾಲು::forwardToken(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಗಾತ್ರ_t ಗಾತ್ರ_2, ತಾರ್ಕಿಕ track_ಸಾಲುಗಳು) {
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_token_ಅಂಕೆ = 1;
    m_token_ಅಂಕೆ = s_token_ಅಂಕೆ++;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* sp = textp; ಗಾತ್ರ_2 && *sp; ++sp, --ಗಾತ್ರ_2) {
        ಆದರೆ (*sp == '\n') {
            ಆದರೆ (track_ಸಾಲುಗಳು) linenoInc();
            m_ಕೊನೆಯ_ಸ್ತಂಭ = 1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } ಇಲ್ಲದಿದ್ದರೆ {  // Tabs are considered one column; hence column means number of chars
            ++m_ಕೊನೆಯ_ಸ್ತಂಭ;
        }
    }
}

ಶೂನ್ಯ File_ಸಾಲು::applyIgnores() {
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(ಇದು);  // Toggle warnings based on global config file
#ಆದರೆ_ಅಂತ್ಯ
}

File_ಸಾಲು* File_ಸಾಲು::copy_Or_Same_File_ಸಾಲು_Applied() {
    applyIgnores();
    ಹಿಂತಿರುಗಿ copy_Or_Same_File_ಸಾಲು();
}

File_ಸಾಲು* File_ಸಾಲು::copy_Or_Same_File_ಸಾಲು() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ಸ್ಥಿರ File_ಸಾಲು* s_ಕೊನೆಯ_Newp = ಶೂನ್ಯಸೂಚಕ;
    ಆದರೆ (s_ಕೊನೆಯ_Newp && *s_ಕೊನೆಯ_Newp == *ಇದು) {  // Compares lineno, filename, etc
        ಹಿಂತಿರುಗಿ s_ಕೊನೆಯ_Newp;
    }
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ File_ಸಾಲು{ಇದು};
    s_ಕೊನೆಯ_Newp = newp;
    ಹಿಂತಿರುಗಿ newp;
}

string File_ಸಾಲು::filebasename() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ V3Os::filenameNonDir(filename()); }

string File_ಸಾಲು::filebasenameNoExt() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ V3Os::filenameNonDirExt(filename()); }

string File_ಸಾಲು::ಮೊದಲ_ಸ್ತಂಭ_Letters() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ a = ((ಮೊದಲ_ಸ್ತಂಭ() / 26) % 26) + 'a';
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ b = (ಮೊದಲ_ಸ್ತಂಭ() % 26) + 'a';
    ಹಿಂತಿರುಗಿ string(1, a) + string(1, b);
}

string File_ಸಾಲು::profileFuncname() ಸ್ಥಿರಾಂಕ {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::ಗಾತ್ರ_type pos;
    ಆಗ ((pos = name.ಕಂಡುಹಿಡಿ_ಮೊದಲ_not_ದ(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_ಗಮ್ಯ_Str(ಕೊನೆಯ_Lineno());
    ಹಿಂತಿರುಗಿ name;
}

string File_ಸಾಲು::ascii_ಸಾಲು_ಲಂಬಸಾಲು() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ (cvt_ಗಮ್ಯ_Str(ಮೊದಲ_Lineno()) + "-" + cvt_ಗಮ್ಯ_Str(ಕೊನೆಯ_Lineno()) + ":" + cvt_ಗಮ್ಯ_Str(ಮೊದಲ_ಸ್ತಂಭ())
            + "-" + cvt_ಗಮ್ಯ_Str(ಕೊನೆಯ_ಸ್ತಂಭ()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_ಗಮ್ಯ_Str(m_contentLineno) + "]");
}
string File_ಸಾಲು::ascii() ಸ್ಥಿರಾಂಕ {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    ಹಿಂತಿರುಗಿ filename() + ":" + cvt_ಗಮ್ಯ_Str(ಕೊನೆಯ_Lineno()) + ":" + cvt_ಗಮ್ಯ_Str(ಮೊದಲ_ಸ್ತಂಭ());
}
std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, File_ಸಾಲು* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    ಹಿಂತಿರುಗಿ (os);
}

string File_ಸಾಲು::warnOffParse(ಸ್ಥಿರಾಂಕ string& msgs, ತಾರ್ಕಿಕ turnOff) {
    string ಫಲಿತಾಂಶ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ_2 : VString::ವಿಭಜಿಸು(msgs, ',')) {
        ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2{ಸಂದೇಶ_2};
        ಆದರೆ (!ಸಂಕೇತ_2.hardError()) {
            warnOff(ಸಂಕೇತ_2, turnOff);
            ಮುಂದುವರಿ;
        }
        // Error if not suppressed
        ಆದರೆ (!v3Global.opt.isFuture(ಸಂದೇಶ_2)) ಫಲಿತಾಂಶ = VString::dot(ಫಲಿತಾಂಶ, ",", ಸಂದೇಶ_2);
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ತಾರ್ಕಿಕ File_ಸಾಲು::warnIsOff(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!ಸಂದೇಶ_En().enabled(ಸಂಕೇತ_2)) ಹಿಂತಿರುಗಿ ನಿಜ;
    ಆದರೆ (!default_File_ಸಾಲು().ಸಂದೇಶ_En().enabled(ಸಂಕೇತ_2)) ಹಿಂತಿರುಗಿ ನಿಜ;  // Global overrides local
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

// cppverilator-suppress constParameter
ಶೂನ್ಯ File_ಸಾಲು::v3error_ಕೊನೆ(std::ostringstream& sstr, ಸ್ಥಿರಾಂಕ string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    ಆದರೆ (ಕೊನೆಯ_Lineno()) nsstr << V3Error::warnContextBegin() << ಇದು << V3Error::warn_Context_ಕೊನೆ();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    ಆದರೆ (!extra.ಖಾಲಿಗಳು()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().ಉದ್ದಳತೆ()) << " " << ": "
                << V3Error::warn_Context_ಕೊನೆ() << extra;
    }
    ಆದರೆ (warnIsOff(V3Error::s().error_ಸಂಕೇತ())) {
        V3Error::s().suppressThisWarning();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (!V3Error::s().errorContexted()) {
            ಸ್ಥಿರಾಂಕ string ಕೂಡಿಸು = warnContextPrimary();
            wsstr << ಕೂಡಿಸು;
            nsstr << ಕೂಡಿಸು;
        }
        ಸ್ಥಿರಾಂಕ string waiver_ಪಠ್ಯ = V3Error::strip_Meta_ಪಠ್ಯ(wsstr.str(), ಸುಳ್ಳು);
        m_waive = V3Control::waive(ಇದು, V3Error::s().error_ಸಂಕೇತ(), waiver_ಪಠ್ಯ);
        ಆದರೆ (m_waive) {
            V3Error::s().suppressThisWarning();
        } ಇಲ್ಲದಿದ್ದರೆ {
            V3Waiver::ಕೂಡಿಸು_ನಮೂದು(V3Error::s().error_ಸಂಕೇತ(), filename(), waiver_ಪಠ್ಯ);
        }
    }
    V3Error::v3error_ಕೊನೆ(nsstr, extrass.str(), ಇದು);
}

string File_ಸಾಲು::warn_ಇನ್ನಷ್ಟು() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex) {
    ಆದರೆ (ಕೊನೆಯ_Lineno()) {
        ಹಿಂತಿರುಗಿ V3Error::warnContextBegin() + V3Error::warn_ಇನ್ನಷ್ಟು() + string(ascii().ಗಾತ್ರ_2(), ' ')
               + ": " + V3Error::warn_Context_ಕೊನೆ();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ V3Error::warn_ಇನ್ನಷ್ಟು();
    }
}
string File_ಸಾಲು::warnOther() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex) {
    ಆದರೆ (ಕೊನೆಯ_Lineno()) {
        ಹಿಂತಿರುಗಿ V3Error::s().warnRelated(ಇದು) + V3Error::warnContextBegin() + V3Error::warn_ಇನ್ನಷ್ಟು()
               + ascii() + ": " + V3Error::warn_Context_ಕೊನೆ();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ V3Error::warn_ಇನ್ನಷ್ಟು();
    }
};

string File_ಸಾಲು::ಮೂಲ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಆದರೆ (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        ಆದರೆ (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            ಹಿಂತಿರುಗಿ "\n%Error: internal tracking of file contents failed";
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹಿಂತಿರುಗಿ "";
        }
    }  // LCOV_EXCL_STOP
    ಹಿಂತಿರುಗಿ m_contentp->get_ಸಾಲು(m_contentLineno);
}
string File_ಸಾಲು::ಮೂಲ_ಪೂರ್ವಪ್ರತ್ಯಯ(ಪೂರ್ಣಾಂಕ ಗಮ್ಯ_ಸ್ತಂಭ) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಸ್ಥಿರಾಂಕ std::string src = ಮೂಲ();
    ಆದರೆ (ಗಮ್ಯ_ಸ್ತಂಭ > static_cast<ಪೂರ್ಣಾಂಕ>(src.ಉದ್ದಳತೆ())) ಗಮ್ಯ_ಸ್ತಂಭ = static_cast<ಪೂರ್ಣಾಂಕ>(src.ಉದ್ದಳತೆ());
    ಆದರೆ (ಗಮ್ಯ_ಸ್ತಂಭ < 1) ಹಿಂತಿರುಗಿ "";
    ಹಿಂತಿರುಗಿ src.substr(0, ಗಮ್ಯ_ಸ್ತಂಭ - 1);
}
string File_ಸಾಲು::pretty_ಮೂಲ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    string ಹೊರಹೋಗು = ಮೂಲ();
    // Drop ignore trailing newline
    ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type pos = ಹೊರಹೋಗು.ಕಂಡುಹಿಡಿ('\n');
    ಆದರೆ (pos != string::npos) ಹೊರಹೋಗು = string(ಹೊರಹೋಗು, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    ಹಿಂತಿರುಗಿ VString::spaceUnprintable(ಹೊರಹೋಗು);
}

string File_ಸಾಲು::warnContext() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!v3Global.opt.context()) ಹಿಂತಿರುಗಿ "";
    ಆದರೆ (ಮೊದಲ_Lineno() == ಕೊನೆಯ_Lineno() && ಮೊದಲ_ಸ್ತಂಭ()) {
        ಸ್ಥಿರಾಂಕ string ಮೂಲ_ಸಾಲು = pretty_ಮೂಲ();
        string ಹೊರಹೋಗು;
        // Don't show super-long lines as can fill screen and unlikely to help user
        ಆದರೆ (!ಮೂಲ_ಸಾಲು.ಖಾಲಿಗಳು() && ಮೂಲ_ಸಾಲು.ಉದ್ದಳತೆ() < SHOW_ಮೂಲ_MAX_ಉದ್ದಳತೆ
            && ಮೂಲ_ಸಾಲು.ಉದ್ದಳತೆ() >= static_cast<ಗಾತ್ರ_t>(ಕೊನೆಯ_ಸ್ತಂಭ() - 1)) {
            string linestr = cvt_ಗಮ್ಯ_Str(ಮೊದಲ_Lineno());
            ಆಗ (linestr.ಗಾತ್ರ_2() < 5) linestr = ' ' + linestr;
            ಹೊರಹೋಗು += linestr + " | " + ಮೂಲ_ಸಾಲು + '\n';
            ಹೊರಹೋಗು += std::string(linestr.ಗಾತ್ರ_2(), ' ') + " | ";
            ಹೊರಹೋಗು += string((ಮೊದಲ_ಸ್ತಂಭ() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            ಆದರೆ (ಕೊನೆಯ_ಸ್ತಂಭ() > ಮೊದಲ_ಸ್ತಂಭ()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                ಹೊರಹೋಗು += string((ಕೊನೆಯ_ಸ್ತಂಭ() - ಮೊದಲ_ಸ್ತಂಭ() - 1), '~');
            }
            ಹೊರಹೋಗು += '\n';
        }
        ಹಿಂತಿರುಗಿ V3Error::warnContextBegin() + ಹೊರಹೋಗು + V3Error::warn_Context_ಕೊನೆ();
    }
    ಹಿಂತಿರುಗಿ "";
}

string File_ಸಾಲು::warn_Context_ಪೋಷಕ() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex) {
    string ಫಲಿತಾಂಶ;
    ಗಾಗಿ (File_ಸಾಲು* ಪೋಷಕ_Fl = ಪೋಷಕ(); ಪೋಷಕ_Fl; ಪೋಷಕ_Fl = ಪೋಷಕ_Fl->ಪೋಷಕ()) {
        ಆದರೆ (ಪೋಷಕ_Fl->filenameIsGlobal()) ನಿಲ್ಲಿಸು;
        ಫಲಿತಾಂಶ += ಪೋಷಕ_Fl->warnOther() + "... note: In file included from '"
                  + ಪೋಷಕ_Fl->filebasename() + "'\n";
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_LEAK_CHECKS
std::unordered_ಗಣ<File_ಸಾಲು*> file_ಸಾಲು_Leak_Checks;

ಶೂನ್ಯ* File_ಸಾಲು::ಕ್ರಿಯಾಕಾರಕ ಹೊಸ(ಗಾತ್ರ_t ಗಾತ್ರ_2) {
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ objp = static_cast<File_ಸಾಲು*>(::ಕ್ರಿಯಾಕಾರಕ ಹೊಸ(ಗಾತ್ರ_2));
    file_ಸಾಲು_Leak_Checks.ಸೇರಿಸು(objp);
    ಹಿಂತಿರುಗಿ objp;
}

ಶೂನ್ಯ File_ಸಾಲು::ಕ್ರಿಯಾಕಾರಕ ಅಳಿಸು(ಶೂನ್ಯ* objp, ಗಾತ್ರ_t ಗಾತ್ರ_2) {
    ಆದರೆ (!objp) ಹಿಂತಿರುಗಿ;
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = static_cast<File_ಸಾಲು*>(objp);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = file_ಸಾಲು_Leak_Checks.ಕಂಡುಹಿಡಿ(flp);
    ಆದರೆ (it != file_ಸಾಲು_Leak_Checks.ಕೊನೆ()) {
        file_ಸಾಲು_Leak_Checks.erase(it);
    } ಇಲ್ಲದಿದ್ದರೆ {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::ಕ್ರಿಯಾಕಾರಕ ಅಳಿಸು(objp);
}
#ಆದರೆ_ಅಂತ್ಯ

ಶೂನ್ಯ File_ಸಾಲು::stats() {
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
    V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("FileLines, Number of filenames",
                        singleton().m_ಹೆಸರುಗಳು.ಗಾತ್ರ_2());  // Max m_filenameno
    V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("FileLines, Message enable sets",
                        singleton().m_interned_ಸಂದೇಶ_Ens.ಗಾತ್ರ_2());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#ಆದರೆ_ಅಂತ್ಯ
}

ಶೂನ್ಯ File_ಸಾಲು::delete_All_ಉಳಿದ() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    ಆಗ (ನಿಜ) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = file_ಸಾಲು_Leak_Checks.begin();
        ಆದರೆ (it == file_ಸಾಲು_Leak_Checks.ಕೊನೆ()) ನಿಲ್ಲಿಸು;
        ಅಳಿಸು *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_ಸಾಲು_Leak_Checks.clear();
    singleton().clear();
#ಆದರೆ_ಅಂತ್ಯ
}
