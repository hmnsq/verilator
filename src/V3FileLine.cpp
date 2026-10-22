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

#include "config_build.h"
#include "verilatedos.h"

// clang-format off
#include "V3Error.h"
#include "V3FileLine.h"
#include "V3Os.h"
#include "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#include "V3Waiver.h"
// clang-format on

#include <algorithm>
#include <iomanip>
#include <limits>
#include <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_लिन्या_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr int size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    char out[size];
    char* op = out + size - 1;
    *--op = '\0';  // We build backwards
    int नुमेरो = fileno;
    do {
        *--op = 'a' + नुमेरो % 26;
        नुमेरो /= 26;
    } while (नुमेरो);
    return op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_लिन्या_Singleton::fileNameIdx_t File_लिन्या_Singleton::name_To_नुमेरो(const string& filename) {
    const auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    if (pair.second) {
        const size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.इतुलाक_back(filename);
        m_languages.इतुलाक_back(V3LangCode::mostRecent());
    }
    return idx;
}

void File_लिन्या_Singleton::file_Name_नुमेरो_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    for (const auto& itr : m_namemap) {
        std::string filename = itr.first;
        if (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } else if (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << नुमेरो_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_लिन्या_Singleton::मेनसाहे_En_Set_Idx_t File_लिन्या_Singleton::add_मेनसाहे_En_Bit_Set(const मेनसाहे_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    const V3LockGuard lock{m_mutex};
    const auto pair = m_interned_मेनसाहे_En_Idxs.emplace(bitSet, 0);
    मेनसाहे_En_Set_Idx_t& idx = pair.first->second;
    if (pair.second) {
        const size_t nextIdx = m_interned_मेनसाहे_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<मेनसाहे_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<मेनसाहे_En_Set_Idx_t>(nextIdx);
        m_interned_मेनसाहे_Ens.इतुलाक_back(bitSet);
    }
    return idx;
}

File_लिन्या_Singleton::मेनसाहे_En_Set_Idx_t File_लिन्या_Singleton::default_मेनसाहे_En_Index() VL_MT_SAFE {
    मेनसाहे_En_Bit_Set मेनसाहे_En_Bit_Set_2;
    for (int i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        const V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        मेनसाहे_En_Bit_Set_2.set(मेनसाहे_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        मेनसाहे_En_Bit_Set_2.set(मेनसाहे_En_Bit_Set::Subset::CTRL, code, true);
    }
    return add_मेनसाहे_En_Bit_Set(मेनसाहे_En_Bit_Set_2);
}

File_लिन्या_Singleton::मेनसाहे_En_Set_Idx_t File_लिन्या_Singleton::मेनसाहे_En_Set_Bit(मेनसाहे_En_Set_Idx_t setIdx,
                                                                मेनसाहे_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, bool हालागा) {
    // See if state matches existing
    bool same = true;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        if (मेनसाहे_En(setIdx).test(subset, subcode) != हालागा) same = false;
    });
    if (same) return setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    मेनसाहे_En_Bit_Set मेनसाहे_En_Bit_Set_2{मेनसाहे_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { मेनसाहे_En_Bit_Set_2.set(subset, subcode, हालागा); });
    return add_मेनसाहे_En_Bit_Set(मेनसाहे_En_Bit_Set_2);
}

File_लिन्या_Singleton::मेनसाहे_En_Set_Idx_t File_लिन्या_Singleton::मेनसाहे_Set_Ctrl_Bit_Set(मेनसाहे_En_Set_Idx_t setIdx,
                                                                     const VErrorBitSet& bitset) {
    const मेनसाहे_En_Bit_Set::Subset subset = मेनसाहे_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    if (मेनसाहे_En(setIdx).getAll(subset) == bitset) return setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    मेनसाहे_En_Bit_Set मेनसाहे_En_Bit_Set_2{मेनसाहे_En(setIdx)};
    मेनसाहे_En_Bit_Set_2.setAll(subset, bitset);
    return add_मेनसाहे_En_Bit_Set(मेनसाहे_En_Bit_Set_2);
}

File_लिन्या_Singleton::मेनसाहे_En_Set_Idx_t File_लिन्या_Singleton::मेनसाहे_En_And(मेनसाहे_En_Set_Idx_t lhsIdx,
                                                             मेनसाहे_En_Set_Idx_t rhsIdx) {
    const मेनसाहे_En_Bit_Set& lhs = मेनसाहे_En(lhsIdx);
    const मेनसाहे_En_Bit_Set& rhs = मेनसाहे_En(rhsIdx);
    const मेनसाहे_En_Bit_Set intersection{lhs, rhs};
    if (intersection == lhs) return lhsIdx;
    if (intersection == rhs) return rhsIdx;
    return add_मेनसाहे_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

void VFileContent::इतुलाक_Text(const string& text) {
    // Similar code in WildcardContents::pushText()
    if (m_मगा_लिन्या.size() == 0) {
        m_मगा_लिन्या.emplace_back("");  // no such thing as line [0]
        m_मगा_लिन्या.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    const string leftover = m_मगा_लिन्या.back() + text;
    m_मगा_लिन्या.आलिसिन_back();

    // Insert line-by-line
    string::size_type लिन्या_start = 0;
    while (true) {
        const string::size_type लिन्या_end = leftover.find('\n', लिन्या_start);
        if (लिन्या_end != string::npos) {
            const string oneline(leftover, लिन्या_start, लिन्या_end - लिन्या_start + 1);
            m_मगा_लिन्या.इतुलाक_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_मगा_लिन्या.size() - 1) << "]: " << oneline);
            लिन्या_start = लिन्या_end + 1;
        } else {
            break;
        }
    }
    // Keep leftover for next time
    m_मगा_लिन्या.emplace_back(string(leftover, लिन्या_start));  // Might be ""
}

string VFileContent::get_लिन्या(int lineno) const VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    if (VL_UNCOVERABLE(lineno < 0 || lineno >= (int)m_मगा_लिन्या.size())) {
        if (debug() || v3Global.opt.debugCheck()) {
            return ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } else {
            return "";
        }
    }
    string text = m_मगा_लिन्या[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    return text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    if (!contentp) {
        os << "ct0";
    } else {
        os << contentp->ascii();
    }
    return os;
}

// ######################################################################
//  FileLine class functions

File_लिन्या::~File_लिन्या() {
    if (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

void File_लिन्या::newContent() {
    if (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = new VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_लिन्या::लिन्या_Directive_Strg(int enterExit) const {
    return "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

void File_लिन्या::लिन्या_Directive(const char* textp, int& enterExitRef) {
    string newFilename;
    int newLineno = -1;
    लिन्या_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    if (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } else {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

void File_लिन्या::लिन्या_Directive_Parse(const char* textp, string& filenameRef, int& linenoRef,
                                  int& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    do {
        // Skip `line
        while (*textp && std::isspace(*textp)) ++textp;
        while (*textp && !std::isspace(*textp)) ++textp;
        while (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        int लिन्या_No;
        const char* const ln = textp;
        while (*textp && !std::isspace(*textp)) ++textp;
        if (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            लिन्या_No = lineno() + 1;
        } else if (std::isdigit(*ln)) {
            लिन्या_No = std::atoi(ln);
        } else {
            break;  // Fail
        }
        while (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        if (*textp != '"') break;  // Fail
        const char* const fn = ++textp;
        while (*textp && *textp != '"') ++textp;
        if (*textp != '"') break;  // Fail
        string err_मेनसाहे;
        const string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_मेनसाहे);
        if (!err_मेनसाहे.empty()) this->v3error(err_मेनसाहे.c_str());
        ++textp;
        while (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        if (!std::isdigit(*textp)) break;  // Fail
        const int level = std::atoi(textp);
        if (level < 0 || level >= 3) break;  // Fail

        linenoRef = लिन्या_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        return;
    } while (false);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

void File_लिन्या::forwardToken(const char* textp, size_t size, bool track_मगा_लिन्या) {
    static int s_token_नुमेरो = 1;
    m_token_नुमेरो = s_token_नुमेरो++;
    for (const char* sp = textp; size && *sp; ++sp, --size) {
        if (*sp == '\n') {
            if (track_मगा_लिन्या) linenoInc();
            m_lastColumn = 1;
        } else if (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } else {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

void File_लिन्या::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(this);  // Toggle warnings based on global config file
#endif
}

File_लिन्या* File_लिन्या::copy_Or_Same_File_लिन्या_Applied() {
    applyIgnores();
    return copy_Or_Same_File_लिन्या();
}

File_लिन्या* File_लिन्या::copy_Or_Same_File_लिन्या() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    static File_लिन्या* s_lastNewp = nullptr;
    if (s_lastNewp && *s_lastNewp == *this) {  // Compares lineno, filename, etc
        return s_lastNewp;
    }
    File_लिन्या* const newp = new File_लिन्या{this};
    s_lastNewp = newp;
    return newp;
}

string File_लिन्या::filebasename() const VL_MT_SAFE { return V3Os::filenameNonDir(filename()); }

string File_लिन्या::filebasenameNoExt() const { return V3Os::filenameNonDirExt(filename()); }

string File_लिन्या::firstColumnLetters() const VL_MT_SAFE {
    const char a = ((firstColumn() / 26) % 26) + 'a';
    const char b = (firstColumn() % 26) + 'a';
    return string(1, a) + string(1, b);
}

string File_लिन्या::profileFuncname() const {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    while ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    return name;
}

string File_लिन्या::ascii_लिन्या_Col() const {
    return (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_लिन्या::ascii() const {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    return filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_लिन्या* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    return (os);
}

string File_लिन्या::warnOffParse(const string& msgs, bool turnOff) {
    string result;
    for (const string& मेनसाहे : VString::split(msgs, ',')) {
        const V3ErrorCode code{मेनसाहे};
        if (!code.hardError()) {
            warnOff(code, turnOff);
            continue;
        }
        // Error if not suppressed
        if (!v3Global.opt.isFuture(मेनसाहे)) result = VString::dot(result, ",", मेनसाहे);
    }
    return result;
}

bool File_लिन्या::warnIsOff(V3ErrorCode code) const {
    if (!मेनसाहे_En().enabled(code)) return true;
    if (!default_File_लिन्या().मेनसाहे_En().enabled(code)) return true;  // Global overrides local
    return false;
}

// cppverilator-suppress constParameter
void File_लिन्या::v3errorEnd(std::ostringstream& sstr, const string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    if (lastLineno()) nsstr << V3Error::warnContextBegin() << this << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    if (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    if (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } else {
        if (!V3Error::s().errorContexted()) {
            const string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        const string waiverText = V3Error::stripMetaText(wsstr.str(), false);
        m_waive = V3Control::waive(this, V3Error::s().errorCode(), waiverText);
        if (m_waive) {
            V3Error::s().suppressThisWarning();
        } else {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), this);
}

string File_लिन्या::warnMore() const VL_REQUIRES(V3Error::s().m_mutex) {
    if (lastLineno()) {
        return V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } else {
        return V3Error::warnMore();
    }
}
string File_लिन्या::warnOther() const VL_REQUIRES(V3Error::s().m_mutex) {
    if (lastLineno()) {
        return V3Error::s().warnRelated(this) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } else {
        return V3Error::warnMore();
    }
};

string File_लिन्या::source() const VL_MT_SAFE {
    if (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        if (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            return "\n%Error: internal tracking of file contents failed";
        } else {
            return "";
        }
    }  // LCOV_EXCL_STOP
    return m_contentp->get_लिन्या(m_contentLineno);
}
string File_लिन्या::sourcePrefix(int toColumn) const VL_MT_SAFE {
    const std::string src = source();
    if (toColumn > static_cast<int>(src.length())) toColumn = static_cast<int>(src.length());
    if (toColumn < 1) return "";
    return src.substr(0, toColumn - 1);
}
string File_लिन्या::prettySource() const VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    const string::size_type pos = out.find('\n');
    if (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    return VString::spaceUnprintable(out);
}

string File_लिन्या::warnContext() const {
    if (!v3Global.opt.context()) return "";
    if (firstLineno() == lastLineno() && firstColumn()) {
        const string source_लिन्या = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        if (!source_लिन्या.empty() && source_लिन्या.length() < SHOW_SOURCE_MAX_LENGTH
            && source_लिन्या.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            while (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_लिन्या + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            if (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        return V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    return "";
}

string File_लिन्या::warnContextParent() const VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    for (File_लिन्या* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        if (parentFl->filenameIsGlobal()) break;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    return result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_लिन्या*> file_लिन्या_Leak_Checks;

void* File_लिन्या::operator new(size_t size) {
    File_लिन्या* const objp = static_cast<File_लिन्या*>(::operator new(size));
    file_लिन्या_Leak_Checks.इसिंगित(objp);
    return objp;
}

void File_लिन्या::operator delete(void* objp, size_t size) {
    if (!objp) return;
    File_लिन्या* const flp = static_cast<File_लिन्या*>(objp);
    const auto it = file_लिन्या_Leak_Checks.find(flp);
    if (it != file_लिन्या_Leak_Checks.end()) {
        file_लिन्या_Leak_Checks.erase(it);
    } else {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

void File_लिन्या::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_मेनसाहे_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

void File_लिन्या::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    while (true) {
        const auto it = file_लिन्या_Leak_Checks.begin();
        if (it == file_लिन्या_Leak_Checks.end()) break;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_लिन्या_Leak_Checks.clear();
    singleton().clear();
#endif
}
