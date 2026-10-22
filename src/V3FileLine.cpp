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

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

// clang-format off
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Waiver.h"
// clang-format on

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <limits>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_ਸਤਰ_Singleton::filenameLetters(fileNameIdx_t fileno) VL_ਸ਼ੁੱਧ {
    constexpr ਪੂਰਨ ਮਾਪ
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    ਅੱਖਰ ਬਾਹਰਲਾ[ਮਾਪ];
    ਅੱਖਰ* op = ਬਾਹਰਲਾ + ਮਾਪ - 1;
    *--op = '\0';  // We build backwards
    ਪੂਰਨ ਅੰਕ = fileno;
    ਕਰੋ {
        *--op = 'a' + ਅੰਕ % 26;
        ਅੰਕ /= 26;
    } ਜਦੋਂ (ਅੰਕ);
    ਵਾਪਸ op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_ਸਤਰ_Singleton::fileNameIdx_t File_ਸਤਰ_Singleton::name_ਮੰਜ਼ਿਲ_ਸੰਖਿਆ(ਅਚਲ string& filename) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.ਪਹਿਲਾ->ਦੂਜਾ;
    ਜੇ (pair.ਦੂਜਾ) {
        ਅਚਲ ਮਾਪ_t ਅਗਲਾ_Idx = m_ਨਾਂ.ਮਾਪ();
        UASSERT(ਅਗਲਾ_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_ਮੰਜ਼ਿਲ_Str(ਅਗਲਾ_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(ਅਗਲਾ_Idx);
        m_ਨਾਂ.ਧੱਕੋ_ਪਿੱਠ(filename);
        m_languages.ਧੱਕੋ_ਪਿੱਠ(V3_Lang_ਕੋਡ::mostRecent());
    }
    ਵਾਪਸ idx;
}

ਖਾਲੀ File_ਸਤਰ_Singleton::file_Name_ਅੰਕ_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : m_namemap) {
        std::string filename = itr.ਪਹਿਲਾ;
        ਜੇ (filename == V3Options::get_Std_Package_ਰਸਤਾ()) {
            filename = "<verilated_std>";
        } ਨਹੀਂਤਾਂ ਜੇ (filename == V3Options::get_Std_Waiver_ਰਸਤਾ()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.ਦੂਜਾ) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(V3Os::filename_Real_ਰਸਤਾ(itr.ਪਹਿਲਾ)) << '"'
           << ", \"language\":\"" << ਸੰਖਿਆ_ਮੰਜ਼ਿਲ_Lang(itr.ਦੂਜਾ).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t File_ਸਤਰ_Singleton::ਜੋੜੋ_ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ)
    VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex) {
    ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{m_mutex};
    ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_interned_ਸੁਨੇਹਾ_En_Idxs.emplace(ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ, 0);
    ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t& idx = pair.ਪਹਿਲਾ->ਦੂਜਾ;
    ਜੇ (pair.ਦੂਜਾ) {
        ਅਚਲ ਮਾਪ_t ਅਗਲਾ_Idx = m_interned_ਸੁਨੇਹਾ_Ens.ਮਾਪ();
        UASSERT(ਅਗਲਾ_Idx <= std::numeric_limits<ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_ਮੰਜ਼ਿਲ_Str(ਅਗਲਾ_Idx) + "+).");
        idx = static_cast<ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t>(ਅਗਲਾ_Idx);
        m_interned_ਸੁਨੇਹਾ_Ens.ਧੱਕੋ_ਪਿੱਠ(ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ);
    }
    ਵਾਪਸ idx;
}

File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t File_ਸਤਰ_Singleton::default_ਸੁਨੇਹਾ_En_ਸੂਚਕਾਂਕ() VL_MT_ਸੁਰੱਖਿਅਤ {
    ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2;
    ਲਈ (ਪੂਰਨ i = V3_Error_ਕੋਡ::EC_MIN; i < V3_Error_ਕੋਡ::_ENUM_MAX; ++i) {
        ਅਚਲ V3_Error_ਕੋਡ ਕੋਡ{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2.ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Subset::ਕੋਡ_2, ਕੋਡ, !ਕੋਡ.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2.ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Subset::CTRL, ਕੋਡ, ਸੱਚ);
    }
    ਵਾਪਸ ਜੋੜੋ_ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2);
}

File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t ਸੈੱਟ_Idx,
                                                                ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Subset subset,
                                                                V3_Error_ਕੋਡ ਕੋਡ, ਤਾਰਕਿਕ ਮੁੱਲ) {
    // See if state matches existing
    ਤਾਰਕਿਕ same = ਸੱਚ;
    ਕੋਡ.forDelegateCodes([&](V3_Error_ਕੋਡ subcode) {
        ਜੇ (ਸੁਨੇਹਾ_En(ਸੈੱਟ_Idx).test(subset, subcode) != ਮੁੱਲ) same = ਝੂਠ;
    });
    ਜੇ (same) ਵਾਪਸ ਸੈੱਟ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2{ਸੁਨੇਹਾ_En(ਸੈੱਟ_Idx)};
    ਕੋਡ.forDelegateCodes([&](V3_Error_ਕੋਡ subcode) { ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2.ਸੈੱਟ(subset, subcode, ਮੁੱਲ); });
    ਵਾਪਸ ਜੋੜੋ_ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2);
}

File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_ਸੈੱਟ_Ctrl_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t ਸੈੱਟ_Idx,
                                                                     ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& bitset) {
    ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Subset subset = ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Subset::CTRL;
    // See if state matches existing
    ਜੇ (ਸੁਨੇਹਾ_En(ਸੈੱਟ_Idx).getAll(subset) == bitset) ਵਾਪਸ ਸੈੱਟ_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2{ਸੁਨੇਹਾ_En(ਸੈੱਟ_Idx)};
    ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2.ਸੈੱਟ_All(subset, bitset);
    ਵਾਪਸ ਜੋੜੋ_ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ_2);
}

File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_And(ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t lhsIdx,
                                                             ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t rhsIdx) {
    ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& lhs = ਸੁਨੇਹਾ_En(lhsIdx);
    ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& rhs = ਸੁਨੇਹਾ_En(rhsIdx);
    ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ intersection{lhs, rhs};
    ਜੇ (intersection == lhs) ਵਾਪਸ lhsIdx;
    ਜੇ (intersection == rhs) ਵਾਪਸ rhsIdx;
    ਵਾਪਸ ਜੋੜੋ_ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(intersection);
}

// ######################################################################
//  VFileContents class functions

ਖਾਲੀ VFileContent::ਧੱਕੋ_ਪਾਠ(ਅਚਲ string& ਪਾਠ) {
    // Similar code in WildcardContents::pushText()
    ਜੇ (m_ਸਤਰਾਂ.ਮਾਪ() == 0) {
        m_ਸਤਰਾਂ.emplace_ਪਿੱਠ("");  // no such thing as line [0]
        m_ਸਤਰਾਂ.emplace_ਪਿੱਠ("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ਅਚਲ string leftover = m_ਸਤਰਾਂ.ਪਿੱਠ() + ਪਾਠ;
    m_ਸਤਰਾਂ.ਕੱਢੋ_ਪਿੱਠ();

    // Insert line-by-line
    string::ਮਾਪ_type ਸਤਰ_ਸ਼ੁਰੂ = 0;
    ਜਦੋਂ (ਸੱਚ) {
        ਅਚਲ string::ਮਾਪ_type ਸਤਰ_ਸਿਰਾ = leftover.ਲੱਭੋ('\n', ਸਤਰ_ਸ਼ੁਰੂ);
        ਜੇ (ਸਤਰ_ਸਿਰਾ != string::npos) {
            ਅਚਲ string oneline(leftover, ਸਤਰ_ਸ਼ੁਰੂ, ਸਤਰ_ਸਿਰਾ - ਸਤਰ_ਸ਼ੁਰੂ + 1);
            m_ਸਤਰਾਂ.ਧੱਕੋ_ਪਿੱਠ(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_ਸਤਰਾਂ.ਮਾਪ() - 1) << "]: " << oneline);
            ਸਤਰ_ਸ਼ੁਰੂ = ਸਤਰ_ਸਿਰਾ + 1;
        } ਨਹੀਂਤਾਂ {
            ਰੋਕੋ;
        }
    }
    // Keep leftover for next time
    m_ਸਤਰਾਂ.emplace_ਪਿੱਠ(string(leftover, ਸਤਰ_ਸ਼ੁਰੂ));  // Might be ""
}

string VFileContent::get_ਸਤਰ(ਪੂਰਨ lineno) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    ਜੇ (VL_UNCOVERABLE(lineno < 0 || lineno >= (ਪੂਰਨ)m_ਸਤਰਾਂ.ਮਾਪ())) {
        ਜੇ (debug() || v3Global.opt.debugCheck()) {
            ਵਾਪਸ ("%Error-internal-contents-bad-ct" + cvt_ਮੰਜ਼ਿਲ_Str(m_id) + "-ln" + cvt_ਮੰਜ਼ਿਲ_Str(lineno));
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ "";
        }
    }
    string ਪਾਠ = m_ਸਤਰਾਂ[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << ਪਾਠ);
    ਵਾਪਸ ਪਾਠ;
}

std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, VFileContent* contentp) {
    ਜੇ (!contentp) {
        os << "ct0";
    } ਨਹੀਂਤਾਂ {
        os << contentp->ascii();
    }
    ਵਾਪਸ os;
}

// ######################################################################
//  FileLine class functions

File_ਸਤਰ::~File_ਸਤਰ() {
    ਜੇ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

ਖਾਲੀ File_ਸਤਰ::newContent() {
    ਜੇ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = ਨਵਾਂ VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_ਸਤਰ::ਸਤਰ_Directive_Strg(ਪੂਰਨ enterExit) ਅਚਲ {
    ਵਾਪਸ "`line "s + cvt_ਮੰਜ਼ਿਲ_Str(ਆਖਰੀ_Lineno()) + " \""
           + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(filename()) + "\" " + cvt_ਮੰਜ਼ਿਲ_Str(enterExit) + '\n';
}

ਖਾਲੀ File_ਸਤਰ::ਸਤਰ_Directive(ਅਚਲ ਅੱਖਰ* textp, ਪੂਰਨ& enterExitRef) {
    string newFilename;
    ਪੂਰਨ newLineno = -1;
    ਸਤਰ_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    ਜੇ (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } ਨਹੀਂਤਾਂ {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

ਖਾਲੀ File_ਸਤਰ::ਸਤਰ_Directive_Parse(ਅਚਲ ਅੱਖਰ* textp, string& filenameRef, ਪੂਰਨ& linenoRef,
                                  ਪੂਰਨ& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    ਕਰੋ {
        // Skip `line
        ਜਦੋਂ (*textp && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*textp)) ++textp;
        ਜਦੋਂ (*textp && !std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*textp)) ++textp;
        ਜਦੋਂ (*textp && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*textp)) ++textp;

        // Grab linenumber
        ਪੂਰਨ ਸਤਰ_No;
        ਅਚਲ ਅੱਖਰ* ਅਚਲ ln = textp;
        ਜਦੋਂ (*textp && !std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*textp)) ++textp;
        ਜੇ (0 == ਲੜੀ_ਤੁਲਨਾ(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            ਸਤਰ_No = lineno() + 1;
        } ਨਹੀਂਤਾਂ ਜੇ (std::ਅੰਕ_ਹੈ(*ln)) {
            ਸਤਰ_No = std::atoi(ln);
        } ਨਹੀਂਤਾਂ {
            ਰੋਕੋ;  // Fail
        }
        ਜਦੋਂ (*textp && (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*textp))) ++textp;

        // Grab filename
        ਜੇ (*textp != '"') ਰੋਕੋ;  // Fail
        ਅਚਲ ਅੱਖਰ* ਅਚਲ fn = ++textp;
        ਜਦੋਂ (*textp && *textp != '"') ++textp;
        ਜੇ (*textp != '"') ਰੋਕੋ;  // Fail
        string err_ਸੁਨੇਹਾ;
        ਅਚਲ string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_ਸੁਨੇਹਾ);
        ਜੇ (!err_ਸੁਨੇਹਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਇਹ->v3error(err_ਸੁਨੇਹਾ.c_str());
        ++textp;
        ਜਦੋਂ (*textp && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*textp)) ++textp;

        // Grab level
        ਜੇ (!std::ਅੰਕ_ਹੈ(*textp)) ਰੋਕੋ;  // Fail
        ਅਚਲ ਪੂਰਨ level = std::atoi(textp);
        ਜੇ (level < 0 || level >= 3) ਰੋਕੋ;  // Fail

        linenoRef = ਸਤਰ_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        ਵਾਪਸ;
    } ਜਦੋਂ (ਝੂਠ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

ਖਾਲੀ File_ਸਤਰ::forwardToken(ਅਚਲ ਅੱਖਰ* textp, ਮਾਪ_t ਮਾਪ, ਤਾਰਕਿਕ track_ਸਤਰਾਂ) {
    ਸਥਿਰ ਪੂਰਨ s_token_ਅੰਕ = 1;
    m_token_ਅੰਕ = s_token_ਅੰਕ++;
    ਲਈ (ਅਚਲ ਅੱਖਰ* sp = textp; ਮਾਪ && *sp; ++sp, --ਮਾਪ) {
        ਜੇ (*sp == '\n') {
            ਜੇ (track_ਸਤਰਾਂ) linenoInc();
            m_ਆਖਰੀ_ਕਾਲਮ = 1;
        } ਨਹੀਂਤਾਂ ਜੇ (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } ਨਹੀਂਤਾਂ {  // Tabs are considered one column; hence column means number of chars
            ++m_ਆਖਰੀ_ਕਾਲਮ;
        }
    }
}

ਖਾਲੀ File_ਸਤਰ::applyIgnores() {
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(ਇਹ);  // Toggle warnings based on global config file
#ਜੇ_ਅੰਤ
}

File_ਸਤਰ* File_ਸਤਰ::copy_Or_Same_File_ਸਤਰ_Applied() {
    applyIgnores();
    ਵਾਪਸ copy_Or_Same_File_ਸਤਰ();
}

File_ਸਤਰ* File_ਸਤਰ::copy_Or_Same_File_ਸਤਰ() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ਸਥਿਰ File_ਸਤਰ* s_ਆਖਰੀ_Newp = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (s_ਆਖਰੀ_Newp && *s_ਆਖਰੀ_Newp == *ਇਹ) {  // Compares lineno, filename, etc
        ਵਾਪਸ s_ਆਖਰੀ_Newp;
    }
    File_ਸਤਰ* ਅਚਲ newp = ਨਵਾਂ File_ਸਤਰ{ਇਹ};
    s_ਆਖਰੀ_Newp = newp;
    ਵਾਪਸ newp;
}

string File_ਸਤਰ::filebasename() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ V3Os::filenameNonDir(filename()); }

string File_ਸਤਰ::filebasenameNoExt() ਅਚਲ { ਵਾਪਸ V3Os::filenameNonDirExt(filename()); }

string File_ਸਤਰ::ਪਹਿਲਾ_ਕਾਲਮ_Letters() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਅਚਲ ਅੱਖਰ a = ((ਪਹਿਲਾ_ਕਾਲਮ() / 26) % 26) + 'a';
    ਅਚਲ ਅੱਖਰ b = (ਪਹਿਲਾ_ਕਾਲਮ() % 26) + 'a';
    ਵਾਪਸ string(1, a) + string(1, b);
}

string File_ਸਤਰ::profileFuncname() ਅਚਲ {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::ਮਾਪ_type pos;
    ਜਦੋਂ ((pos = name.ਲੱਭੋ_ਪਹਿਲਾ_not_ਦਾ(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_ਮੰਜ਼ਿਲ_Str(ਆਖਰੀ_Lineno());
    ਵਾਪਸ name;
}

string File_ਸਤਰ::ascii_ਸਤਰ_ਖਾਨਾ() ਅਚਲ {
    ਵਾਪਸ (cvt_ਮੰਜ਼ਿਲ_Str(ਪਹਿਲਾ_Lineno()) + "-" + cvt_ਮੰਜ਼ਿਲ_Str(ਆਖਰੀ_Lineno()) + ":" + cvt_ਮੰਜ਼ਿਲ_Str(ਪਹਿਲਾ_ਕਾਲਮ())
            + "-" + cvt_ਮੰਜ਼ਿਲ_Str(ਆਖਰੀ_ਕਾਲਮ()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_ਮੰਜ਼ਿਲ_Str(m_contentLineno) + "]");
}
string File_ਸਤਰ::ascii() ਅਚਲ {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    ਵਾਪਸ filename() + ":" + cvt_ਮੰਜ਼ਿਲ_Str(ਆਖਰੀ_Lineno()) + ":" + cvt_ਮੰਜ਼ਿਲ_Str(ਪਹਿਲਾ_ਕਾਲਮ());
}
std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, File_ਸਤਰ* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    ਵਾਪਸ (os);
}

string File_ਸਤਰ::warnOffParse(ਅਚਲ string& msgs, ਤਾਰਕਿਕ turnOff) {
    string ਨਤੀਜਾ;
    ਲਈ (ਅਚਲ string& ਸੁਨੇਹਾ_2 : VString::ਵੰਡੋ(msgs, ',')) {
        ਅਚਲ V3_Error_ਕੋਡ ਕੋਡ{ਸੁਨੇਹਾ_2};
        ਜੇ (!ਕੋਡ.hardError()) {
            warnOff(ਕੋਡ, turnOff);
            ਜਾਰੀ;
        }
        // Error if not suppressed
        ਜੇ (!v3Global.opt.isFuture(ਸੁਨੇਹਾ_2)) ਨਤੀਜਾ = VString::dot(ਨਤੀਜਾ, ",", ਸੁਨੇਹਾ_2);
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਤਾਰਕਿਕ File_ਸਤਰ::warnIsOff(V3_Error_ਕੋਡ ਕੋਡ) ਅਚਲ {
    ਜੇ (!ਸੁਨੇਹਾ_En().enabled(ਕੋਡ)) ਵਾਪਸ ਸੱਚ;
    ਜੇ (!default_File_ਸਤਰ().ਸੁਨੇਹਾ_En().enabled(ਕੋਡ)) ਵਾਪਸ ਸੱਚ;  // Global overrides local
    ਵਾਪਸ ਝੂਠ;
}

// cppverilator-suppress constParameter
ਖਾਲੀ File_ਸਤਰ::v3error_ਸਿਰਾ(std::ostringstream& sstr, ਅਚਲ string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    ਜੇ (ਆਖਰੀ_Lineno()) nsstr << V3Error::warnContextBegin() << ਇਹ << V3Error::warn_Context_ਸਿਰਾ();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    ਜੇ (!extra.ਖਾਲੀ_ਥਾਵਾਂ()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().ਲੰਮਾਈ()) << " " << ": "
                << V3Error::warn_Context_ਸਿਰਾ() << extra;
    }
    ਜੇ (warnIsOff(V3Error::s().error_ਕੋਡ())) {
        V3Error::s().suppressThisWarning();
    } ਨਹੀਂਤਾਂ {
        ਜੇ (!V3Error::s().errorContexted()) {
            ਅਚਲ string ਜੋੜੋ = warnContextPrimary();
            wsstr << ਜੋੜੋ;
            nsstr << ਜੋੜੋ;
        }
        ਅਚਲ string waiver_ਪਾਠ = V3Error::strip_Meta_ਪਾਠ(wsstr.str(), ਝੂਠ);
        m_waive = V3Control::waive(ਇਹ, V3Error::s().error_ਕੋਡ(), waiver_ਪਾਠ);
        ਜੇ (m_waive) {
            V3Error::s().suppressThisWarning();
        } ਨਹੀਂਤਾਂ {
            V3Waiver::ਜੋੜੋ_ਇੰਦਰਾਜ(V3Error::s().error_ਕੋਡ(), filename(), waiver_ਪਾਠ);
        }
    }
    V3Error::v3error_ਸਿਰਾ(nsstr, extrass.str(), ਇਹ);
}

string File_ਸਤਰ::warn_ਹੋਰ() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex) {
    ਜੇ (ਆਖਰੀ_Lineno()) {
        ਵਾਪਸ V3Error::warnContextBegin() + V3Error::warn_ਹੋਰ() + string(ascii().ਮਾਪ(), ' ')
               + ": " + V3Error::warn_Context_ਸਿਰਾ();
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ V3Error::warn_ਹੋਰ();
    }
}
string File_ਸਤਰ::warnOther() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex) {
    ਜੇ (ਆਖਰੀ_Lineno()) {
        ਵਾਪਸ V3Error::s().warnRelated(ਇਹ) + V3Error::warnContextBegin() + V3Error::warn_ਹੋਰ()
               + ascii() + ": " + V3Error::warn_Context_ਸਿਰਾ();
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ V3Error::warn_ਹੋਰ();
    }
};

string File_ਸਤਰ::ਸਰੋਤ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਜੇ (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        ਜੇ (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            ਵਾਪਸ "\n%Error: internal tracking of file contents failed";
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ "";
        }
    }  // LCOV_EXCL_STOP
    ਵਾਪਸ m_contentp->get_ਸਤਰ(m_contentLineno);
}
string File_ਸਤਰ::ਸਰੋਤ_ਅਗੇਤਰ(ਪੂਰਨ ਮੰਜ਼ਿਲ_ਕਾਲਮ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਅਚਲ std::string src = ਸਰੋਤ();
    ਜੇ (ਮੰਜ਼ਿਲ_ਕਾਲਮ > static_cast<ਪੂਰਨ>(src.ਲੰਮਾਈ())) ਮੰਜ਼ਿਲ_ਕਾਲਮ = static_cast<ਪੂਰਨ>(src.ਲੰਮਾਈ());
    ਜੇ (ਮੰਜ਼ਿਲ_ਕਾਲਮ < 1) ਵਾਪਸ "";
    ਵਾਪਸ src.substr(0, ਮੰਜ਼ਿਲ_ਕਾਲਮ - 1);
}
string File_ਸਤਰ::pretty_ਸਰੋਤ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    string ਬਾਹਰਲਾ = ਸਰੋਤ();
    // Drop ignore trailing newline
    ਅਚਲ string::ਮਾਪ_type pos = ਬਾਹਰਲਾ.ਲੱਭੋ('\n');
    ਜੇ (pos != string::npos) ਬਾਹਰਲਾ = string(ਬਾਹਰਲਾ, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    ਵਾਪਸ VString::spaceUnprintable(ਬਾਹਰਲਾ);
}

string File_ਸਤਰ::warnContext() ਅਚਲ {
    ਜੇ (!v3Global.opt.context()) ਵਾਪਸ "";
    ਜੇ (ਪਹਿਲਾ_Lineno() == ਆਖਰੀ_Lineno() && ਪਹਿਲਾ_ਕਾਲਮ()) {
        ਅਚਲ string ਸਰੋਤ_ਸਤਰ = pretty_ਸਰੋਤ();
        string ਬਾਹਰਲਾ;
        // Don't show super-long lines as can fill screen and unlikely to help user
        ਜੇ (!ਸਰੋਤ_ਸਤਰ.ਖਾਲੀ_ਥਾਵਾਂ() && ਸਰੋਤ_ਸਤਰ.ਲੰਮਾਈ() < SHOW_ਸਰੋਤ_MAX_ਲੰਮਾਈ
            && ਸਰੋਤ_ਸਤਰ.ਲੰਮਾਈ() >= static_cast<ਮਾਪ_t>(ਆਖਰੀ_ਕਾਲਮ() - 1)) {
            string linestr = cvt_ਮੰਜ਼ਿਲ_Str(ਪਹਿਲਾ_Lineno());
            ਜਦੋਂ (linestr.ਮਾਪ() < 5) linestr = ' ' + linestr;
            ਬਾਹਰਲਾ += linestr + " | " + ਸਰੋਤ_ਸਤਰ + '\n';
            ਬਾਹਰਲਾ += std::string(linestr.ਮਾਪ(), ' ') + " | ";
            ਬਾਹਰਲਾ += string((ਪਹਿਲਾ_ਕਾਲਮ() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            ਜੇ (ਆਖਰੀ_ਕਾਲਮ() > ਪਹਿਲਾ_ਕਾਲਮ()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                ਬਾਹਰਲਾ += string((ਆਖਰੀ_ਕਾਲਮ() - ਪਹਿਲਾ_ਕਾਲਮ() - 1), '~');
            }
            ਬਾਹਰਲਾ += '\n';
        }
        ਵਾਪਸ V3Error::warnContextBegin() + ਬਾਹਰਲਾ + V3Error::warn_Context_ਸਿਰਾ();
    }
    ਵਾਪਸ "";
}

string File_ਸਤਰ::warn_Context_ਮਾਪਾ() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex) {
    string ਨਤੀਜਾ;
    ਲਈ (File_ਸਤਰ* ਮਾਪਾ_Fl = ਮਾਪਾ(); ਮਾਪਾ_Fl; ਮਾਪਾ_Fl = ਮਾਪਾ_Fl->ਮਾਪਾ()) {
        ਜੇ (ਮਾਪਾ_Fl->filenameIsGlobal()) ਰੋਕੋ;
        ਨਤੀਜਾ += ਮਾਪਾ_Fl->warnOther() + "... note: In file included from '"
                  + ਮਾਪਾ_Fl->filebasename() + "'\n";
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
std::unordered_ਸੈੱਟ<File_ਸਤਰ*> file_ਸਤਰ_Leak_Checks;

ਖਾਲੀ* File_ਸਤਰ::ਸੰਚਾਲਕ ਨਵਾਂ(ਮਾਪ_t ਮਾਪ) {
    File_ਸਤਰ* ਅਚਲ objp = static_cast<File_ਸਤਰ*>(::ਸੰਚਾਲਕ ਨਵਾਂ(ਮਾਪ));
    file_ਸਤਰ_Leak_Checks.ਪਾਓ(objp);
    ਵਾਪਸ objp;
}

ਖਾਲੀ File_ਸਤਰ::ਸੰਚਾਲਕ ਮਿਟਾਓ(ਖਾਲੀ* objp, ਮਾਪ_t ਮਾਪ) {
    ਜੇ (!objp) ਵਾਪਸ;
    File_ਸਤਰ* ਅਚਲ flp = static_cast<File_ਸਤਰ*>(objp);
    ਅਚਲ ਸਵੈਚਾਲਿਤ it = file_ਸਤਰ_Leak_Checks.ਲੱਭੋ(flp);
    ਜੇ (it != file_ਸਤਰ_Leak_Checks.ਸਿਰਾ()) {
        file_ਸਤਰ_Leak_Checks.erase(it);
    } ਨਹੀਂਤਾਂ {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::ਸੰਚਾਲਕ ਮਿਟਾਓ(objp);
}
#ਜੇ_ਅੰਤ

ਖਾਲੀ File_ਸਤਰ::stats() {
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
    V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("FileLines, Number of filenames",
                        singleton().m_ਨਾਂ.ਮਾਪ());  // Max m_filenameno
    V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("FileLines, Message enable sets",
                        singleton().m_interned_ਸੁਨੇਹਾ_Ens.ਮਾਪ());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#ਜੇ_ਅੰਤ
}

ਖਾਲੀ File_ਸਤਰ::delete_All_ਬਚਿਆ() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    ਜਦੋਂ (ਸੱਚ) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = file_ਸਤਰ_Leak_Checks.begin();
        ਜੇ (it == file_ਸਤਰ_Leak_Checks.ਸਿਰਾ()) ਰੋਕੋ;
        ਮਿਟਾਓ *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_ਸਤਰ_Leak_Checks.clear();
    singleton().clear();
#ਜੇ_ਅੰਤ
}
