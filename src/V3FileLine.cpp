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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

// clang-format off
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#ଅନ୍ତର୍ଭାଵ "V3Waiver.h"
// clang-format on

#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_ଧାଡ଼ି_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr ପୂର୍ଣ୍ଣସଂଖ୍ୟା size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    ଅକ୍ଷର out[size];
    ଅକ୍ଷର* op = out + size - 1;
    *--op = '\0';  // We build backwards
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ = fileno;
    କର {
        *--op = 'a' + ଅଙ୍କ % 26;
        ଅଙ୍କ /= 26;
    } ଯେତେବେଳେ (ଅଙ୍କ);
    ଫେରନ୍ତୁ op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_ଧାଡ଼ି_Singleton::fileNameIdx_t File_ଧାଡ଼ି_Singleton::name_To_ସଂଖ୍ୟା(ଅଚଳ string& filename) {
    ଅଚଳ auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    ଯଦି (pair.second) {
        ଅଚଳ size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.ଠେଲ_back(filename);
        m_languages.ଠେଲ_back(V3LangCode::mostRecent());
    }
    ଫେରନ୍ତୁ idx;
}

ଶୂନ୍ୟ File_ଧାଡ଼ି_Singleton::file_Name_ଅଙ୍କ_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    ପାଇଁ (ଅଚଳ auto& itr : m_namemap) {
        std::string filename = itr.first;
        ଯଦି (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } ଅନ୍ୟଥା ଯଦି (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << ସଂଖ୍ୟା_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_Set_Idx_t File_ଧାଡ଼ି_Singleton::add_ସନ୍ଦେଶ_En_Bit_Set(ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ଅଚଳ V3LockGuard lock{m_mutex};
    ଅଚଳ auto pair = m_interned_ସନ୍ଦେଶ_En_Idxs.emplace(bitSet, 0);
    ସନ୍ଦେଶ_En_Set_Idx_t& idx = pair.first->second;
    ଯଦି (pair.second) {
        ଅଚଳ size_t nextIdx = m_interned_ସନ୍ଦେଶ_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<ସନ୍ଦେଶ_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<ସନ୍ଦେଶ_En_Set_Idx_t>(nextIdx);
        m_interned_ସନ୍ଦେଶ_Ens.ଠେଲ_back(bitSet);
    }
    ଫେରନ୍ତୁ idx;
}

File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_Set_Idx_t File_ଧାଡ଼ି_Singleton::default_ସନ୍ଦେଶ_En_Index() VL_MT_SAFE {
    ସନ୍ଦେଶ_En_Bit_Set ସନ୍ଦେଶ_En_Bit_Set_2;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ଅଚଳ V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        ସନ୍ଦେଶ_En_Bit_Set_2.set(ସନ୍ଦେଶ_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        ସନ୍ଦେଶ_En_Bit_Set_2.set(ସନ୍ଦେଶ_En_Bit_Set::Subset::CTRL, code, ସତ୍ୟ);
    }
    ଫେରନ୍ତୁ add_ସନ୍ଦେଶ_En_Bit_Set(ସନ୍ଦେଶ_En_Bit_Set_2);
}

File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_Set_Idx_t File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_Set_Bit(ସନ୍ଦେଶ_En_Set_Idx_t setIdx,
                                                                ସନ୍ଦେଶ_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, ତାର୍କିକ ମୂଲ୍ୟ) {
    // See if state matches existing
    ତାର୍କିକ same = ସତ୍ୟ;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        ଯଦି (ସନ୍ଦେଶ_En(setIdx).test(subset, subcode) != ମୂଲ୍ୟ) same = ମିଥ୍ୟା;
    });
    ଯଦି (same) ଫେରନ୍ତୁ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    ସନ୍ଦେଶ_En_Bit_Set ସନ୍ଦେଶ_En_Bit_Set_2{ସନ୍ଦେଶ_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { ସନ୍ଦେଶ_En_Bit_Set_2.set(subset, subcode, ମୂଲ୍ୟ); });
    ଫେରନ୍ତୁ add_ସନ୍ଦେଶ_En_Bit_Set(ସନ୍ଦେଶ_En_Bit_Set_2);
}

File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_Set_Idx_t File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_Set_Ctrl_Bit_Set(ସନ୍ଦେଶ_En_Set_Idx_t setIdx,
                                                                     ଅଚଳ VErrorBitSet& bitset) {
    ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set::Subset subset = ସନ୍ଦେଶ_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    ଯଦି (ସନ୍ଦେଶ_En(setIdx).getAll(subset) == bitset) ଫେରନ୍ତୁ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    ସନ୍ଦେଶ_En_Bit_Set ସନ୍ଦେଶ_En_Bit_Set_2{ସନ୍ଦେଶ_En(setIdx)};
    ସନ୍ଦେଶ_En_Bit_Set_2.setAll(subset, bitset);
    ଫେରନ୍ତୁ add_ସନ୍ଦେଶ_En_Bit_Set(ସନ୍ଦେଶ_En_Bit_Set_2);
}

File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_Set_Idx_t File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_And(ସନ୍ଦେଶ_En_Set_Idx_t lhsIdx,
                                                             ସନ୍ଦେଶ_En_Set_Idx_t rhsIdx) {
    ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& lhs = ସନ୍ଦେଶ_En(lhsIdx);
    ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& rhs = ସନ୍ଦେଶ_En(rhsIdx);
    ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set intersection{lhs, rhs};
    ଯଦି (intersection == lhs) ଫେରନ୍ତୁ lhsIdx;
    ଯଦି (intersection == rhs) ଫେରନ୍ତୁ rhsIdx;
    ଫେରନ୍ତୁ add_ସନ୍ଦେଶ_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

ଶୂନ୍ୟ VFileContent::ଠେଲ_Text(ଅଚଳ string& text) {
    // Similar code in WildcardContents::pushText()
    ଯଦି (m_ଧାଡ଼ିଗୁଡ଼ିକ.size() == 0) {
        m_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back("");  // no such thing as line [0]
        m_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ଅଚଳ string leftover = m_ଧାଡ଼ିଗୁଡ଼ିକ.back() + text;
    m_ଧାଡ଼ିଗୁଡ଼ିକ.କାଢ଼_back();

    // Insert line-by-line
    string::size_type ଧାଡ଼ି_start = 0;
    ଯେତେବେଳେ (ସତ୍ୟ) {
        ଅଚଳ string::size_type ଧାଡ଼ି_end = leftover.find('\n', ଧାଡ଼ି_start);
        ଯଦି (ଧାଡ଼ି_end != string::npos) {
            ଅଚଳ string oneline(leftover, ଧାଡ଼ି_start, ଧାଡ଼ି_end - ଧାଡ଼ି_start + 1);
            m_ଧାଡ଼ିଗୁଡ଼ିକ.ଠେଲ_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_ଧାଡ଼ିଗୁଡ଼ିକ.size() - 1) << "]: " << oneline);
            ଧାଡ଼ି_start = ଧାଡ଼ି_end + 1;
        } ଅନ୍ୟଥା {
            ଭାଙ୍ଗ;
        }
    }
    // Keep leftover for next time
    m_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back(string(leftover, ଧାଡ଼ି_start));  // Might be ""
}

string VFileContent::get_ଧାଡ଼ି(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno) ଅଚଳ VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    ଯଦି (VL_UNCOVERABLE(lineno < 0 || lineno >= (ପୂର୍ଣ୍ଣସଂଖ୍ୟା)m_ଧାଡ଼ିଗୁଡ଼ିକ.size())) {
        ଯଦି (debug() || v3Global.opt.debugCheck()) {
            ଫେରନ୍ତୁ ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ "";
        }
    }
    string text = m_ଧାଡ଼ିଗୁଡ଼ିକ[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    ଫେରନ୍ତୁ text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    ଯଦି (!contentp) {
        os << "ct0";
    } ଅନ୍ୟଥା {
        os << contentp->ascii();
    }
    ଫେରନ୍ତୁ os;
}

// ######################################################################
//  FileLine class functions

File_ଧାଡ଼ି::~File_ଧାଡ଼ି() {
    ଯଦି (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

ଶୂନ୍ୟ File_ଧାଡ଼ି::newContent() {
    ଯଦି (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = ନୂତନ VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_ଧାଡ଼ି::ଧାଡ଼ି_Directive_Strg(ପୂର୍ଣ୍ଣସଂଖ୍ୟା enterExit) ଅଚଳ {
    ଫେରନ୍ତୁ "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

ଶୂନ୍ୟ File_ଧାଡ଼ି::ଧାଡ଼ି_Directive(ଅଚଳ ଅକ୍ଷର* textp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& enterExitRef) {
    string newFilename;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା newLineno = -1;
    ଧାଡ଼ି_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    ଯଦି (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } ଅନ୍ୟଥା {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

ଶୂନ୍ୟ File_ଧାଡ଼ି::ଧାଡ଼ି_Directive_Parse(ଅଚଳ ଅକ୍ଷର* textp, string& filenameRef, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& linenoRef,
                                  ପୂର୍ଣ୍ଣସଂଖ୍ୟା& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    କର {
        // Skip `line
        ଯେତେବେଳେ (*textp && std::isspace(*textp)) ++textp;
        ଯେତେବେଳେ (*textp && !std::isspace(*textp)) ++textp;
        ଯେତେବେଳେ (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଧାଡ଼ି_No;
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ ln = textp;
        ଯେତେବେଳେ (*textp && !std::isspace(*textp)) ++textp;
        ଯଦି (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            ଧାଡ଼ି_No = lineno() + 1;
        } ଅନ୍ୟଥା ଯଦି (std::isdigit(*ln)) {
            ଧାଡ଼ି_No = std::atoi(ln);
        } ଅନ୍ୟଥା {
            ଭାଙ୍ଗ;  // Fail
        }
        ଯେତେବେଳେ (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        ଯଦି (*textp != '"') ଭାଙ୍ଗ;  // Fail
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ fn = ++textp;
        ଯେତେବେଳେ (*textp && *textp != '"') ++textp;
        ଯଦି (*textp != '"') ଭାଙ୍ଗ;  // Fail
        string err_ସନ୍ଦେଶ;
        ଅଚଳ string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_ସନ୍ଦେଶ);
        ଯଦି (!err_ସନ୍ଦେଶ.empty()) ଏହି->v3error(err_ସନ୍ଦେଶ.c_str());
        ++textp;
        ଯେତେବେଳେ (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        ଯଦି (!std::isdigit(*textp)) ଭାଙ୍ଗ;  // Fail
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା level = std::atoi(textp);
        ଯଦି (level < 0 || level >= 3) ଭାଙ୍ଗ;  // Fail

        linenoRef = ଧାଡ଼ି_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        ଫେରନ୍ତୁ;
    } ଯେତେବେଳେ (ମିଥ୍ୟା);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

ଶୂନ୍ୟ File_ଧାଡ଼ି::forwardToken(ଅଚଳ ଅକ୍ଷର* textp, size_t size, ତାର୍କିକ track_ଧାଡ଼ିଗୁଡ଼ିକ) {
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_token_ଅଙ୍କ = 1;
    m_token_ଅଙ୍କ = s_token_ଅଙ୍କ++;
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* sp = textp; size && *sp; ++sp, --size) {
        ଯଦି (*sp == '\n') {
            ଯଦି (track_ଧାଡ଼ିଗୁଡ଼ିକ) linenoInc();
            m_lastColumn = 1;
        } ଅନ୍ୟଥା ଯଦି (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } ଅନ୍ୟଥା {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

ଶୂନ୍ୟ File_ଧାଡ଼ି::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(ଏହି);  // Toggle warnings based on global config file
#endif
}

File_ଧାଡ଼ି* File_ଧାଡ଼ି::copy_Or_Same_File_ଧାଡ଼ି_Applied() {
    applyIgnores();
    ଫେରନ୍ତୁ copy_Or_Same_File_ଧାଡ଼ି();
}

File_ଧାଡ଼ି* File_ଧାଡ଼ି::copy_Or_Same_File_ଧାଡ଼ି() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ସ୍ଥିର File_ଧାଡ଼ି* s_lastNewp = nullptr;
    ଯଦି (s_lastNewp && *s_lastNewp == *ଏହି) {  // Compares lineno, filename, etc
        ଫେରନ୍ତୁ s_lastNewp;
    }
    File_ଧାଡ଼ି* ଅଚଳ newp = ନୂତନ File_ଧାଡ଼ି{ଏହି};
    s_lastNewp = newp;
    ଫେରନ୍ତୁ newp;
}

string File_ଧାଡ଼ି::filebasename() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ V3Os::filenameNonDir(filename()); }

string File_ଧାଡ଼ି::filebasenameNoExt() ଅଚଳ { ଫେରନ୍ତୁ V3Os::filenameNonDirExt(filename()); }

string File_ଧାଡ଼ି::firstColumnLetters() ଅଚଳ VL_MT_SAFE {
    ଅଚଳ ଅକ୍ଷର a = ((firstColumn() / 26) % 26) + 'a';
    ଅଚଳ ଅକ୍ଷର b = (firstColumn() % 26) + 'a';
    ଫେରନ୍ତୁ string(1, a) + string(1, b);
}

string File_ଧାଡ଼ି::profileFuncname() ଅଚଳ {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    ଯେତେବେଳେ ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    ଫେରନ୍ତୁ name;
}

string File_ଧାଡ଼ି::ascii_ଧାଡ଼ି_Col() ଅଚଳ {
    ଫେରନ୍ତୁ (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_ଧାଡ଼ି::ascii() ଅଚଳ {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    ଫେରନ୍ତୁ filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_ଧାଡ଼ି* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    ଫେରନ୍ତୁ (os);
}

string File_ଧାଡ଼ି::warnOffParse(ଅଚଳ string& msgs, ତାର୍କିକ turnOff) {
    string result;
    ପାଇଁ (ଅଚଳ string& ସନ୍ଦେଶ : VString::split(msgs, ',')) {
        ଅଚଳ V3ErrorCode code{ସନ୍ଦେଶ};
        ଯଦି (!code.hardError()) {
            warnOff(code, turnOff);
            ଜାରି;
        }
        // Error if not suppressed
        ଯଦି (!v3Global.opt.isFuture(ସନ୍ଦେଶ)) result = VString::dot(result, ",", ସନ୍ଦେଶ);
    }
    ଫେରନ୍ତୁ result;
}

ତାର୍କିକ File_ଧାଡ଼ି::warnIsOff(V3ErrorCode code) ଅଚଳ {
    ଯଦି (!ସନ୍ଦେଶ_En().enabled(code)) ଫେରନ୍ତୁ ସତ୍ୟ;
    ଯଦି (!default_File_ଧାଡ଼ି().ସନ୍ଦେଶ_En().enabled(code)) ଫେରନ୍ତୁ ସତ୍ୟ;  // Global overrides local
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

// cppverilator-suppress constParameter
ଶୂନ୍ୟ File_ଧାଡ଼ି::v3errorEnd(std::ostringstream& sstr, ଅଚଳ string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    ଯଦି (lastLineno()) nsstr << V3Error::warnContextBegin() << ଏହି << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    ଯଦି (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    ଯଦି (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } ଅନ୍ୟଥା {
        ଯଦି (!V3Error::s().errorContexted()) {
            ଅଚଳ string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ଅଚଳ string waiverText = V3Error::stripMetaText(wsstr.str(), ମିଥ୍ୟା);
        m_waive = V3Control::waive(ଏହି, V3Error::s().errorCode(), waiverText);
        ଯଦି (m_waive) {
            V3Error::s().suppressThisWarning();
        } ଅନ୍ୟଥା {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), ଏହି);
}

string File_ଧାଡ଼ି::warnMore() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex) {
    ଯଦି (lastLineno()) {
        ଫେରନ୍ତୁ V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ V3Error::warnMore();
    }
}
string File_ଧାଡ଼ି::warnOther() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex) {
    ଯଦି (lastLineno()) {
        ଫେରନ୍ତୁ V3Error::s().warnRelated(ଏହି) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ V3Error::warnMore();
    }
};

string File_ଧାଡ଼ି::source() ଅଚଳ VL_MT_SAFE {
    ଯଦି (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        ଯଦି (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            ଫେରନ୍ତୁ "\n%Error: internal tracking of file contents failed";
        } ଅନ୍ୟଥା {
            ଫେରନ୍ତୁ "";
        }
    }  // LCOV_EXCL_STOP
    ଫେରନ୍ତୁ m_contentp->get_ଧାଡ଼ି(m_contentLineno);
}
string File_ଧାଡ଼ି::sourcePrefix(ପୂର୍ଣ୍ଣସଂଖ୍ୟା toColumn) ଅଚଳ VL_MT_SAFE {
    ଅଚଳ std::string src = source();
    ଯଦି (toColumn > static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(src.length())) toColumn = static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(src.length());
    ଯଦି (toColumn < 1) ଫେରନ୍ତୁ "";
    ଫେରନ୍ତୁ src.substr(0, toColumn - 1);
}
string File_ଧାଡ଼ି::prettySource() ଅଚଳ VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ଅଚଳ string::size_type pos = out.find('\n');
    ଯଦି (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    ଫେରନ୍ତୁ VString::spaceUnprintable(out);
}

string File_ଧାଡ଼ି::warnContext() ଅଚଳ {
    ଯଦି (!v3Global.opt.context()) ଫେରନ୍ତୁ "";
    ଯଦି (firstLineno() == lastLineno() && firstColumn()) {
        ଅଚଳ string source_ଧାଡ଼ି = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        ଯଦି (!source_ଧାଡ଼ି.empty() && source_ଧାଡ଼ି.length() < SHOW_SOURCE_MAX_LENGTH
            && source_ଧାଡ଼ି.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            ଯେତେବେଳେ (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_ଧାଡ଼ି + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            ଯଦି (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        ଫେରନ୍ତୁ V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    ଫେରନ୍ତୁ "";
}

string File_ଧାଡ଼ି::warnContextParent() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    ପାଇଁ (File_ଧାଡ଼ି* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        ଯଦି (parentFl->filenameIsGlobal()) ଭାଙ୍ଗ;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    ଫେରନ୍ତୁ result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_ଧାଡ଼ି*> file_ଧାଡ଼ି_Leak_Checks;

ଶୂନ୍ୟ* File_ଧାଡ଼ି::operator ନୂତନ(size_t size) {
    File_ଧାଡ଼ି* ଅଚଳ objp = static_cast<File_ଧାଡ଼ି*>(::operator ନୂତନ(size));
    file_ଧାଡ଼ି_Leak_Checks.ଭର୍ତ୍ତି(objp);
    ଫେରନ୍ତୁ objp;
}

ଶୂନ୍ୟ File_ଧାଡ଼ି::operator delete(ଶୂନ୍ୟ* objp, size_t size) {
    ଯଦି (!objp) ଫେରନ୍ତୁ;
    File_ଧାଡ଼ି* ଅଚଳ flp = static_cast<File_ଧାଡ଼ି*>(objp);
    ଅଚଳ auto it = file_ଧାଡ଼ି_Leak_Checks.find(flp);
    ଯଦି (it != file_ଧାଡ଼ି_Leak_Checks.end()) {
        file_ଧାଡ଼ି_Leak_Checks.erase(it);
    } ଅନ୍ୟଥା {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

ଶୂନ୍ୟ File_ଧାଡ଼ି::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_ସନ୍ଦେଶ_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

ଶୂନ୍ୟ File_ଧାଡ଼ି::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    ଯେତେବେଳେ (ସତ୍ୟ) {
        ଅଚଳ auto it = file_ଧାଡ଼ି_Leak_Checks.begin();
        ଯଦି (it == file_ଧାଡ଼ି_Leak_Checks.end()) ଭାଙ୍ଗ;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_ଧାଡ଼ି_Leak_Checks.clear();
    singleton().clear();
#endif
}
