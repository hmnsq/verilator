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

string FileLineSingleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr ຈຳນວນເຕັມ size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    var_na out[size];
    var_na* op = out + size - 1;
    *--op = '\0';  // We build backwards
    ຈຳນວນເຕັມ num = fileno;
    do {
        *--op = 'a' + num % 26;
        num /= 26;
    } ໃນຂະນະທີ່ (num);
    ສົ່ງຄືນ op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
FileLineSingleton::fileNameIdx_t FileLineSingleton::nameToNumber(const string& filename) {
    const auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    ຖ້າ (pair.second) {
        const size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.push_back(filename);
        m_languages.push_back(V3LangCode::mostRecent());
    }
    ສົ່ງຄືນ idx;
}

void FileLineSingleton::fileNameNumMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    ສຳລັບ (const auto& itr : m_namemap) {
        std::string filename = itr.first;
        ຖ້າ (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << numberToLang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::addMsgEnBitSet(const MsgEnBitSet& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    const V3LockGuard lock{m_mutex};
    const auto pair = m_internedMsgEnIdxs.emplace(bitSet, 0);
    msgEnSetIdx_t& idx = pair.first->second;
    ຖ້າ (pair.second) {
        const size_t nextIdx = m_internedMsgEns.size();
        UASSERT(nextIdx <= std::numeric_limits<msgEnSetIdx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<msgEnSetIdx_t>(nextIdx);
        m_internedMsgEns.push_back(bitSet);
    }
    ສົ່ງຄືນ idx;
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::defaultMsgEnIndex() VL_MT_SAFE {
    MsgEnBitSet msgEnBitSet;
    ສຳລັບ (ຈຳນວນເຕັມ i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        const V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        msgEnBitSet.set(MsgEnBitSet::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        msgEnBitSet.set(MsgEnBitSet::Subset::CTRL, code, true);
    }
    ສົ່ງຄືນ addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgEnSetBit(msgEnSetIdx_t setIdx,
                                                                MsgEnBitSet::Subset subset,
                                                                V3ErrorCode code, bool value) {
    // See if state matches existing
    bool same = true;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        ຖ້າ (msgEn(setIdx).test(subset, subcode) != value) same = false;
    });
    ຖ້າ (same) ສົ່ງຄືນ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    MsgEnBitSet msgEnBitSet{msgEn(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { msgEnBitSet.set(subset, subcode, value); });
    ສົ່ງຄືນ addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgSetCtrlBitSet(msgEnSetIdx_t setIdx,
                                                                     const VErrorBitSet& bitset) {
    const MsgEnBitSet::Subset subset = MsgEnBitSet::Subset::CTRL;
    // See if state matches existing
    ຖ້າ (msgEn(setIdx).getAll(subset) == bitset) ສົ່ງຄືນ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    MsgEnBitSet msgEnBitSet{msgEn(setIdx)};
    msgEnBitSet.setAll(subset, bitset);
    ສົ່ງຄືນ addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgEnAnd(msgEnSetIdx_t lhsIdx,
                                                             msgEnSetIdx_t rhsIdx) {
    const MsgEnBitSet& lhs = msgEn(lhsIdx);
    const MsgEnBitSet& rhs = msgEn(rhsIdx);
    const MsgEnBitSet intersection{lhs, rhs};
    ຖ້າ (intersection == lhs) ສົ່ງຄືນ lhsIdx;
    ຖ້າ (intersection == rhs) ສົ່ງຄືນ rhsIdx;
    ສົ່ງຄືນ addMsgEnBitSet(intersection);
}

// ######################################################################
//  VFileContents class functions

void VFileContent::pushText(const string& text) {
    // Similar code in WildcardContents::pushText()
    ຖ້າ (m_lines.size() == 0) {
        m_lines.emplace_back("");  // no such thing as line [0]
        m_lines.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    const string leftover = m_lines.back() + text;
    m_lines.pop_back();

    // Insert line-by-line
    string::size_type line_start = 0;
    ໃນຂະນະທີ່ (true) {
        const string::size_type line_end = leftover.find('\n', line_start);
        ຖ້າ (line_end != string::npos) {
            const string oneline(leftover, line_start, line_end - line_start + 1);
            m_lines.push_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_lines.size() - 1) << "]: " << oneline);
            line_start = line_end + 1;
        } ບໍ່ດັ່ງນັ້ນ {
            bha_nga;
        }
    }
    // Keep leftover for next time
    m_lines.emplace_back(string(leftover, line_start));  // Might be ""
}

string VFileContent::getLine(ຈຳນວນເຕັມ lineno) const VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    ຖ້າ (VL_UNCOVERABLE(lineno < 0 || lineno >= (ຈຳນວນເຕັມ)m_lines.size())) {
        ຖ້າ (debug() || v3Global.opt.debugCheck()) {
            ສົ່ງຄືນ ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } ບໍ່ດັ່ງນັ້ນ {
            ສົ່ງຄືນ "";
        }
    }
    string text = m_lines[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    ສົ່ງຄືນ text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    ຖ້າ (!contentp) {
        os << "ct0";
    } ບໍ່ດັ່ງນັ້ນ {
        os << contentp->ascii();
    }
    ສົ່ງຄືນ os;
}

// ######################################################################
//  FileLine class functions

FileLine::~FileLine() {
    ຖ້າ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

void FileLine::newContent() {
    ຖ້າ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = new VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string FileLine::lineDirectiveStrg(ຈຳນວນເຕັມ enterExit) const {
    ສົ່ງຄືນ "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

void FileLine::lineDirective(const var_na* textp, ຈຳນວນເຕັມ& enterExitRef) {
    string newFilename;
    ຈຳນວນເຕັມ newLineno = -1;
    lineDirectiveParse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    ຖ້າ (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } ບໍ່ດັ່ງນັ້ນ {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

void FileLine::lineDirectiveParse(const var_na* textp, string& filenameRef, ຈຳນວນເຕັມ& linenoRef,
                                  ຈຳນວນເຕັມ& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    do {
        // Skip `line
        ໃນຂະນະທີ່ (*textp && std::isspace(*textp)) ++textp;
        ໃນຂະນະທີ່ (*textp && !std::isspace(*textp)) ++textp;
        ໃນຂະນະທີ່ (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        ຈຳນວນເຕັມ lineNo;
        const var_na* const ln = textp;
        ໃນຂະນະທີ່ (*textp && !std::isspace(*textp)) ++textp;
        ຖ້າ (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            lineNo = lineno() + 1;
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (std::isdigit(*ln)) {
            lineNo = std::atoi(ln);
        } ບໍ່ດັ່ງນັ້ນ {
            bha_nga;  // Fail
        }
        ໃນຂະນະທີ່ (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        ຖ້າ (*textp != '"') bha_nga;  // Fail
        const var_na* const fn = ++textp;
        ໃນຂະນະທີ່ (*textp && *textp != '"') ++textp;
        ຖ້າ (*textp != '"') bha_nga;  // Fail
        string errMsg;
        const string& parsedFilename = VString::unquoteSVString(string{fn, textp}, errMsg);
        ຖ້າ (!errMsg.empty()) this->v3error(errMsg.c_str());
        ++textp;
        ໃນຂະນະທີ່ (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        ຖ້າ (!std::isdigit(*textp)) bha_nga;  // Fail
        const ຈຳນວນເຕັມ level = std::atoi(textp);
        ຖ້າ (level < 0 || level >= 3) bha_nga;  // Fail

        linenoRef = lineNo;
        filenameRef = parsedFilename;
        enterExitRef = level;
        ສົ່ງຄືນ;
    } ໃນຂະນະທີ່ (false);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

void FileLine::forwardToken(const var_na* textp, size_t size, bool trackLines) {
    static ຈຳນວນເຕັມ s_tokenNum = 1;
    m_tokenNum = s_tokenNum++;
    ສຳລັບ (const var_na* sp = textp; size && *sp; ++sp, --size) {
        ຖ້າ (*sp == '\n') {
            ຖ້າ (trackLines) linenoInc();
            m_lastColumn = 1;
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } ບໍ່ດັ່ງນັ້ນ {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

void FileLine::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(this);  // Toggle warnings based on global config file
#endif
}

FileLine* FileLine::copyOrSameFileLineApplied() {
    applyIgnores();
    ສົ່ງຄືນ copyOrSameFileLine();
}

FileLine* FileLine::copyOrSameFileLine() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    static FileLine* s_lastNewp = nullptr;
    ຖ້າ (s_lastNewp && *s_lastNewp == *this) {  // Compares lineno, filename, etc
        ສົ່ງຄືນ s_lastNewp;
    }
    FileLine* const newp = new FileLine{this};
    s_lastNewp = newp;
    ສົ່ງຄືນ newp;
}

string FileLine::filebasename() const VL_MT_SAFE { ສົ່ງຄືນ V3Os::filenameNonDir(filename()); }

string FileLine::filebasenameNoExt() const { ສົ່ງຄືນ V3Os::filenameNonDirExt(filename()); }

string FileLine::firstColumnLetters() const VL_MT_SAFE {
    const var_na a = ((firstColumn() / 26) % 26) + 'a';
    const var_na b = (firstColumn() % 26) + 'a';
    ສົ່ງຄືນ string(1, a) + string(1, b);
}

string FileLine::profileFuncname() const {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    ໃນຂະນະທີ່ ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    ສົ່ງຄືນ name;
}

string FileLine::asciiLineCol() const {
    ສົ່ງຄືນ (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string FileLine::ascii() const {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    ສົ່ງຄືນ filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, FileLine* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    ສົ່ງຄືນ (os);
}

string FileLine::warnOffParse(const string& msgs, bool turnOff) {
    string result;
    ສຳລັບ (const string& msg : VString::split(msgs, ',')) {
        const V3ErrorCode code{msg};
        ຖ້າ (!code.hardError()) {
            warnOff(code, turnOff);
            continue;
        }
        // Error if not suppressed
        ຖ້າ (!v3Global.opt.isFuture(msg)) result = VString::dot(result, ",", msg);
    }
    ສົ່ງຄືນ result;
}

bool FileLine::warnIsOff(V3ErrorCode code) const {
    ຖ້າ (!msgEn().enabled(code)) ສົ່ງຄືນ true;
    ຖ້າ (!defaultFileLine().msgEn().enabled(code)) ສົ່ງຄືນ true;  // Global overrides local
    ສົ່ງຄືນ false;
}

// cppverilator-suppress constParameter
void FileLine::v3errorEnd(std::ostringstream& sstr, const string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    ຖ້າ (lastLineno()) nsstr << V3Error::warnContextBegin() << this << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    ຖ້າ (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    ຖ້າ (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } ບໍ່ດັ່ງນັ້ນ {
        ຖ້າ (!V3Error::s().errorContexted()) {
            const string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        const string waiverText = V3Error::stripMetaText(wsstr.str(), false);
        m_waive = V3Control::waive(this, V3Error::s().errorCode(), waiverText);
        ຖ້າ (m_waive) {
            V3Error::s().suppressThisWarning();
        } ບໍ່ດັ່ງນັ້ນ {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), this);
}

string FileLine::warnMore() const VL_REQUIRES(V3Error::s().m_mutex) {
    ຖ້າ (lastLineno()) {
        ສົ່ງຄືນ V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } ບໍ່ດັ່ງນັ້ນ {
        ສົ່ງຄືນ V3Error::warnMore();
    }
}
string FileLine::warnOther() const VL_REQUIRES(V3Error::s().m_mutex) {
    ຖ້າ (lastLineno()) {
        ສົ່ງຄືນ V3Error::s().warnRelated(this) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } ບໍ່ດັ່ງນັ້ນ {
        ສົ່ງຄືນ V3Error::warnMore();
    }
};

string FileLine::source() const VL_MT_SAFE {
    ຖ້າ (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        ຖ້າ (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            ສົ່ງຄືນ "\n%Error: internal tracking of file contents failed";
        } ບໍ່ດັ່ງນັ້ນ {
            ສົ່ງຄືນ "";
        }
    }  // LCOV_EXCL_STOP
    ສົ່ງຄືນ m_contentp->getLine(m_contentLineno);
}
string FileLine::sourcePrefix(ຈຳນວນເຕັມ toColumn) const VL_MT_SAFE {
    const std::string src = source();
    ຖ້າ (toColumn > static_cast<ຈຳນວນເຕັມ>(src.length())) toColumn = static_cast<ຈຳນວນເຕັມ>(src.length());
    ຖ້າ (toColumn < 1) ສົ່ງຄືນ "";
    ສົ່ງຄືນ src.substr(0, toColumn - 1);
}
string FileLine::prettySource() const VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    const string::size_type pos = out.find('\n');
    ຖ້າ (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    ສົ່ງຄືນ VString::spaceUnprintable(out);
}

string FileLine::warnContext() const {
    ຖ້າ (!v3Global.opt.context()) ສົ່ງຄືນ "";
    ຖ້າ (firstLineno() == lastLineno() && firstColumn()) {
        const string sourceLine = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        ຖ້າ (!sourceLine.empty() && sourceLine.length() < SHOW_SOURCE_MAX_LENGTH
            && sourceLine.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            ໃນຂະນະທີ່ (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + sourceLine + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            ຖ້າ (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        ສົ່ງຄືນ V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    ສົ່ງຄືນ "";
}

string FileLine::warnContextParent() const VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    ສຳລັບ (FileLine* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        ຖ້າ (parentFl->filenameIsGlobal()) bha_nga;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    ສົ່ງຄືນ result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<FileLine*> fileLineLeakChecks;

void* FileLine::operator new(size_t size) {
    FileLine* const objp = static_cast<FileLine*>(::operator new(size));
    fileLineLeakChecks.insert(objp);
    ສົ່ງຄືນ objp;
}

void FileLine::operator delete(void* objp, size_t size) {
    ຖ້າ (!objp) ສົ່ງຄືນ;
    FileLine* const flp = static_cast<FileLine*>(objp);
    const auto it = fileLineLeakChecks.find(flp);
    ຖ້າ (it != fileLineLeakChecks.end()) {
        fileLineLeakChecks.erase(it);
    } ບໍ່ດັ່ງນັ້ນ {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

void FileLine::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_internedMsgEns.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

void FileLine::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    ໃນຂະນະທີ່ (true) {
        const auto it = fileLineLeakChecks.begin();
        ຖ້າ (it == fileLineLeakChecks.end()) bha_nga;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    fileLineLeakChecks.clear();
    singleton().clear();
#endif
}
