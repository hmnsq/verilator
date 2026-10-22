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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

// clang-format off
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#અન્તર્ભાવ "V3Waiver.h"
// clang-format on

#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_લીટી_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr પૂર્ણાંક size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    અક્ષર out[size];
    અક્ષર* op = out + size - 1;
    *--op = '\0';  // We build backwards
    પૂર્ણાંક અંક = fileno;
    કરો {
        *--op = 'a' + અંક % 26;
        અંક /= 26;
    } જ્યારે (અંક);
    પાછા op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_લીટી_Singleton::fileNameIdx_t File_લીટી_Singleton::name_To_સંખ્યા(અચલ string& filename) {
    અચલ auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    જો (pair.second) {
        અચલ size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.ધકેલો_back(filename);
        m_languages.ધકેલો_back(V3LangCode::mostRecent());
    }
    પાછા idx;
}

શૂન્ય File_લીટી_Singleton::file_Name_અંક_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    માટે (અચલ auto& itr : m_namemap) {
        std::string filename = itr.first;
        જો (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } નહીંતર જો (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << સંખ્યા_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_લીટી_Singleton::સંદેશ_En_Set_Idx_t File_લીટી_Singleton::add_સંદેશ_En_Bit_Set(અચલ સંદેશ_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    અચલ V3LockGuard lock{m_mutex};
    અચલ auto pair = m_interned_સંદેશ_En_Idxs.emplace(bitSet, 0);
    સંદેશ_En_Set_Idx_t& idx = pair.first->second;
    જો (pair.second) {
        અચલ size_t nextIdx = m_interned_સંદેશ_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<સંદેશ_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<સંદેશ_En_Set_Idx_t>(nextIdx);
        m_interned_સંદેશ_Ens.ધકેલો_back(bitSet);
    }
    પાછા idx;
}

File_લીટી_Singleton::સંદેશ_En_Set_Idx_t File_લીટી_Singleton::default_સંદેશ_En_Index() VL_MT_SAFE {
    સંદેશ_En_Bit_Set સંદેશ_En_Bit_Set_2;
    માટે (પૂર્ણાંક i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        અચલ V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        સંદેશ_En_Bit_Set_2.set(સંદેશ_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        સંદેશ_En_Bit_Set_2.set(સંદેશ_En_Bit_Set::Subset::CTRL, code, સાચું);
    }
    પાછા add_સંદેશ_En_Bit_Set(સંદેશ_En_Bit_Set_2);
}

File_લીટી_Singleton::સંદેશ_En_Set_Idx_t File_લીટી_Singleton::સંદેશ_En_Set_Bit(સંદેશ_En_Set_Idx_t setIdx,
                                                                સંદેશ_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, તાર્કિક કિંમત) {
    // See if state matches existing
    તાર્કિક same = સાચું;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        જો (સંદેશ_En(setIdx).test(subset, subcode) != કિંમત) same = ખોટું;
    });
    જો (same) પાછા setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    સંદેશ_En_Bit_Set સંદેશ_En_Bit_Set_2{સંદેશ_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { સંદેશ_En_Bit_Set_2.set(subset, subcode, કિંમત); });
    પાછા add_સંદેશ_En_Bit_Set(સંદેશ_En_Bit_Set_2);
}

File_લીટી_Singleton::સંદેશ_En_Set_Idx_t File_લીટી_Singleton::સંદેશ_Set_Ctrl_Bit_Set(સંદેશ_En_Set_Idx_t setIdx,
                                                                     અચલ VErrorBitSet& bitset) {
    અચલ સંદેશ_En_Bit_Set::Subset subset = સંદેશ_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    જો (સંદેશ_En(setIdx).getAll(subset) == bitset) પાછા setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    સંદેશ_En_Bit_Set સંદેશ_En_Bit_Set_2{સંદેશ_En(setIdx)};
    સંદેશ_En_Bit_Set_2.setAll(subset, bitset);
    પાછા add_સંદેશ_En_Bit_Set(સંદેશ_En_Bit_Set_2);
}

File_લીટી_Singleton::સંદેશ_En_Set_Idx_t File_લીટી_Singleton::સંદેશ_En_And(સંદેશ_En_Set_Idx_t lhsIdx,
                                                             સંદેશ_En_Set_Idx_t rhsIdx) {
    અચલ સંદેશ_En_Bit_Set& lhs = સંદેશ_En(lhsIdx);
    અચલ સંદેશ_En_Bit_Set& rhs = સંદેશ_En(rhsIdx);
    અચલ સંદેશ_En_Bit_Set intersection{lhs, rhs};
    જો (intersection == lhs) પાછા lhsIdx;
    જો (intersection == rhs) પાછા rhsIdx;
    પાછા add_સંદેશ_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

શૂન્ય VFileContent::ધકેલો_Text(અચલ string& text) {
    // Similar code in WildcardContents::pushText()
    જો (m_લીટીઓ.size() == 0) {
        m_લીટીઓ.emplace_back("");  // no such thing as line [0]
        m_લીટીઓ.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    અચલ string leftover = m_લીટીઓ.back() + text;
    m_લીટીઓ.કાઢો_back();

    // Insert line-by-line
    string::size_type લીટી_start = 0;
    જ્યારે (સાચું) {
        અચલ string::size_type લીટી_end = leftover.find('\n', લીટી_start);
        જો (લીટી_end != string::npos) {
            અચલ string oneline(leftover, લીટી_start, લીટી_end - લીટી_start + 1);
            m_લીટીઓ.ધકેલો_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_લીટીઓ.size() - 1) << "]: " << oneline);
            લીટી_start = લીટી_end + 1;
        } નહીંતર {
            તોડો;
        }
    }
    // Keep leftover for next time
    m_લીટીઓ.emplace_back(string(leftover, લીટી_start));  // Might be ""
}

string VFileContent::get_લીટી(પૂર્ણાંક lineno) અચલ VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    જો (VL_UNCOVERABLE(lineno < 0 || lineno >= (પૂર્ણાંક)m_લીટીઓ.size())) {
        જો (debug() || v3Global.opt.debugCheck()) {
            પાછા ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } નહીંતર {
            પાછા "";
        }
    }
    string text = m_લીટીઓ[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    પાછા text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    જો (!contentp) {
        os << "ct0";
    } નહીંતર {
        os << contentp->ascii();
    }
    પાછા os;
}

// ######################################################################
//  FileLine class functions

File_લીટી::~File_લીટી() {
    જો (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

શૂન્ય File_લીટી::newContent() {
    જો (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = નવું VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_લીટી::લીટી_Directive_Strg(પૂર્ણાંક enterExit) અચલ {
    પાછા "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

શૂન્ય File_લીટી::લીટી_Directive(અચલ અક્ષર* textp, પૂર્ણાંક& enterExitRef) {
    string newFilename;
    પૂર્ણાંક newLineno = -1;
    લીટી_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    જો (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } નહીંતર {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

શૂન્ય File_લીટી::લીટી_Directive_Parse(અચલ અક્ષર* textp, string& filenameRef, પૂર્ણાંક& linenoRef,
                                  પૂર્ણાંક& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    કરો {
        // Skip `line
        જ્યારે (*textp && std::isspace(*textp)) ++textp;
        જ્યારે (*textp && !std::isspace(*textp)) ++textp;
        જ્યારે (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        પૂર્ણાંક લીટી_No;
        અચલ અક્ષર* અચલ ln = textp;
        જ્યારે (*textp && !std::isspace(*textp)) ++textp;
        જો (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            લીટી_No = lineno() + 1;
        } નહીંતર જો (std::isdigit(*ln)) {
            લીટી_No = std::atoi(ln);
        } નહીંતર {
            તોડો;  // Fail
        }
        જ્યારે (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        જો (*textp != '"') તોડો;  // Fail
        અચલ અક્ષર* અચલ fn = ++textp;
        જ્યારે (*textp && *textp != '"') ++textp;
        જો (*textp != '"') તોડો;  // Fail
        string err_સંદેશ;
        અચલ string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_સંદેશ);
        જો (!err_સંદેશ.empty()) આ->v3error(err_સંદેશ.c_str());
        ++textp;
        જ્યારે (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        જો (!std::isdigit(*textp)) તોડો;  // Fail
        અચલ પૂર્ણાંક level = std::atoi(textp);
        જો (level < 0 || level >= 3) તોડો;  // Fail

        linenoRef = લીટી_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        પાછા;
    } જ્યારે (ખોટું);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

શૂન્ય File_લીટી::forwardToken(અચલ અક્ષર* textp, size_t size, તાર્કિક track_લીટીઓ) {
    સ્થિર પૂર્ણાંક s_token_અંક = 1;
    m_token_અંક = s_token_અંક++;
    માટે (અચલ અક્ષર* sp = textp; size && *sp; ++sp, --size) {
        જો (*sp == '\n') {
            જો (track_લીટીઓ) linenoInc();
            m_lastColumn = 1;
        } નહીંતર જો (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } નહીંતર {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

શૂન્ય File_લીટી::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(આ);  // Toggle warnings based on global config file
#endif
}

File_લીટી* File_લીટી::copy_Or_Same_File_લીટી_Applied() {
    applyIgnores();
    પાછા copy_Or_Same_File_લીટી();
}

File_લીટી* File_લીટી::copy_Or_Same_File_લીટી() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    સ્થિર File_લીટી* s_lastNewp = nullptr;
    જો (s_lastNewp && *s_lastNewp == *આ) {  // Compares lineno, filename, etc
        પાછા s_lastNewp;
    }
    File_લીટી* અચલ newp = નવું File_લીટી{આ};
    s_lastNewp = newp;
    પાછા newp;
}

string File_લીટી::filebasename() અચલ VL_MT_SAFE { પાછા V3Os::filenameNonDir(filename()); }

string File_લીટી::filebasenameNoExt() અચલ { પાછા V3Os::filenameNonDirExt(filename()); }

string File_લીટી::firstColumnLetters() અચલ VL_MT_SAFE {
    અચલ અક્ષર a = ((firstColumn() / 26) % 26) + 'a';
    અચલ અક્ષર b = (firstColumn() % 26) + 'a';
    પાછા string(1, a) + string(1, b);
}

string File_લીટી::profileFuncname() અચલ {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    જ્યારે ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    પાછા name;
}

string File_લીટી::ascii_લીટી_Col() અચલ {
    પાછા (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_લીટી::ascii() અચલ {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    પાછા filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_લીટી* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    પાછા (os);
}

string File_લીટી::warnOffParse(અચલ string& msgs, તાર્કિક turnOff) {
    string result;
    માટે (અચલ string& સંદેશ : VString::split(msgs, ',')) {
        અચલ V3ErrorCode code{સંદેશ};
        જો (!code.hardError()) {
            warnOff(code, turnOff);
            ચાલુ;
        }
        // Error if not suppressed
        જો (!v3Global.opt.isFuture(સંદેશ)) result = VString::dot(result, ",", સંદેશ);
    }
    પાછા result;
}

તાર્કિક File_લીટી::warnIsOff(V3ErrorCode code) અચલ {
    જો (!સંદેશ_En().enabled(code)) પાછા સાચું;
    જો (!default_File_લીટી().સંદેશ_En().enabled(code)) પાછા સાચું;  // Global overrides local
    પાછા ખોટું;
}

// cppverilator-suppress constParameter
શૂન્ય File_લીટી::v3errorEnd(std::ostringstream& sstr, અચલ string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    જો (lastLineno()) nsstr << V3Error::warnContextBegin() << આ << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    જો (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    જો (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } નહીંતર {
        જો (!V3Error::s().errorContexted()) {
            અચલ string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        અચલ string waiverText = V3Error::stripMetaText(wsstr.str(), ખોટું);
        m_waive = V3Control::waive(આ, V3Error::s().errorCode(), waiverText);
        જો (m_waive) {
            V3Error::s().suppressThisWarning();
        } નહીંતર {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), આ);
}

string File_લીટી::warnMore() અચલ VL_REQUIRES(V3Error::s().m_mutex) {
    જો (lastLineno()) {
        પાછા V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } નહીંતર {
        પાછા V3Error::warnMore();
    }
}
string File_લીટી::warnOther() અચલ VL_REQUIRES(V3Error::s().m_mutex) {
    જો (lastLineno()) {
        પાછા V3Error::s().warnRelated(આ) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } નહીંતર {
        પાછા V3Error::warnMore();
    }
};

string File_લીટી::source() અચલ VL_MT_SAFE {
    જો (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        જો (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            પાછા "\n%Error: internal tracking of file contents failed";
        } નહીંતર {
            પાછા "";
        }
    }  // LCOV_EXCL_STOP
    પાછા m_contentp->get_લીટી(m_contentLineno);
}
string File_લીટી::sourcePrefix(પૂર્ણાંક toColumn) અચલ VL_MT_SAFE {
    અચલ std::string src = source();
    જો (toColumn > static_cast<પૂર્ણાંક>(src.length())) toColumn = static_cast<પૂર્ણાંક>(src.length());
    જો (toColumn < 1) પાછા "";
    પાછા src.substr(0, toColumn - 1);
}
string File_લીટી::prettySource() અચલ VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    અચલ string::size_type pos = out.find('\n');
    જો (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    પાછા VString::spaceUnprintable(out);
}

string File_લીટી::warnContext() અચલ {
    જો (!v3Global.opt.context()) પાછા "";
    જો (firstLineno() == lastLineno() && firstColumn()) {
        અચલ string source_લીટી = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        જો (!source_લીટી.empty() && source_લીટી.length() < SHOW_SOURCE_MAX_LENGTH
            && source_લીટી.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            જ્યારે (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_લીટી + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            જો (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        પાછા V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    પાછા "";
}

string File_લીટી::warnContextParent() અચલ VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    માટે (File_લીટી* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        જો (parentFl->filenameIsGlobal()) તોડો;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    પાછા result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_લીટી*> file_લીટી_Leak_Checks;

શૂન્ય* File_લીટી::operator નવું(size_t size) {
    File_લીટી* અચલ objp = static_cast<File_લીટી*>(::operator નવું(size));
    file_લીટી_Leak_Checks.ઉમેરો(objp);
    પાછા objp;
}

શૂન્ય File_લીટી::operator delete(શૂન્ય* objp, size_t size) {
    જો (!objp) પાછા;
    File_લીટી* અચલ flp = static_cast<File_લીટી*>(objp);
    અચલ auto it = file_લીટી_Leak_Checks.find(flp);
    જો (it != file_લીટી_Leak_Checks.end()) {
        file_લીટી_Leak_Checks.erase(it);
    } નહીંતર {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

શૂન્ય File_લીટી::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_સંદેશ_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

શૂન્ય File_લીટી::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    જ્યારે (સાચું) {
        અચલ auto it = file_લીટી_Leak_Checks.begin();
        જો (it == file_લીટી_Leak_Checks.end()) તોડો;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_લીટી_Leak_Checks.clear();
    singleton().clear();
#endif
}
