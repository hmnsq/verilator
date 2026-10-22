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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

// clang-format off
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Os.h"
#ضمن "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#ضمن "V3Waiver.h"
// clang-format on

#ضمن <algorithm>
#ضمن <iomanip>
#ضمن <limits>
#ضمن <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string FileLineSingleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr صحيح size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    حرف out[size];
    حرف* op = out + size - 1;
    *--op = '\0';  // We build backwards
    صحيح num = fileno;
    ܥܒܕ {
        *--op = 'a' + num % 26;
        num /= 26;
    } ܟܡܐ (num);
    ܦܢܐ op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
FileLineSingleton::fileNameIdx_t FileLineSingleton::nameToNumber(ثابت string& filename) {
    ثابت auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    ܐܢ (pair.second) {
        ثابت size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.push_back(filename);
        m_languages.push_back(V3LangCode::mostRecent());
    }
    ܦܢܐ idx;
}

فراغ FileLineSingleton::fileNameNumMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    لكل (ثابت auto& itr : m_namemap) {
        std::string filename = itr.first;
        ܐܢ (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } ܐܚܪܢܐ ܐܢ (filename == V3Options::getStdWaiverPath()) {
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

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::addMsgEnBitSet(ثابت MsgEnBitSet& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ثابت V3LockGuard lock{m_mutex};
    ثابت auto pair = m_internedMsgEnIdxs.emplace(bitSet, 0);
    msgEnSetIdx_t& idx = pair.first->second;
    ܐܢ (pair.second) {
        ثابت size_t nextIdx = m_internedMsgEns.size();
        UASSERT(nextIdx <= std::numeric_limits<msgEnSetIdx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<msgEnSetIdx_t>(nextIdx);
        m_internedMsgEns.push_back(bitSet);
    }
    ܦܢܐ idx;
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::defaultMsgEnIndex() VL_MT_SAFE {
    MsgEnBitSet msgEnBitSet;
    لكل (صحيح i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ثابت V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        msgEnBitSet.set(MsgEnBitSet::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        msgEnBitSet.set(MsgEnBitSet::Subset::CTRL, code, ܫܪܪܐ);
    }
    ܦܢܐ addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgEnSetBit(msgEnSetIdx_t setIdx,
                                                                MsgEnBitSet::Subset subset,
                                                                V3ErrorCode code, منطقي value) {
    // See if state matches existing
    منطقي same = ܫܪܪܐ;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        ܐܢ (msgEn(setIdx).test(subset, subcode) != value) same = ܕܓܠܐ;
    });
    ܐܢ (same) ܦܢܐ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    MsgEnBitSet msgEnBitSet{msgEn(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { msgEnBitSet.set(subset, subcode, value); });
    ܦܢܐ addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgSetCtrlBitSet(msgEnSetIdx_t setIdx,
                                                                     ثابت VErrorBitSet& bitset) {
    ثابت MsgEnBitSet::Subset subset = MsgEnBitSet::Subset::CTRL;
    // See if state matches existing
    ܐܢ (msgEn(setIdx).getAll(subset) == bitset) ܦܢܐ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    MsgEnBitSet msgEnBitSet{msgEn(setIdx)};
    msgEnBitSet.setAll(subset, bitset);
    ܦܢܐ addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgEnAnd(msgEnSetIdx_t lhsIdx,
                                                             msgEnSetIdx_t rhsIdx) {
    ثابت MsgEnBitSet& lhs = msgEn(lhsIdx);
    ثابت MsgEnBitSet& rhs = msgEn(rhsIdx);
    ثابت MsgEnBitSet intersection{lhs, rhs};
    ܐܢ (intersection == lhs) ܦܢܐ lhsIdx;
    ܐܢ (intersection == rhs) ܦܢܐ rhsIdx;
    ܦܢܐ addMsgEnBitSet(intersection);
}

// ######################################################################
//  VFileContents class functions

فراغ VFileContent::pushText(ثابت string& text) {
    // Similar code in WildcardContents::pushText()
    ܐܢ (m_lines.size() == 0) {
        m_lines.emplace_back("");  // no such thing as line [0]
        m_lines.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_lines.back() + text;
    m_lines.pop_back();

    // Insert line-by-line
    string::size_type line_start = 0;
    ܟܡܐ (ܫܪܪܐ) {
        ثابت string::size_type line_end = leftover.find('\n', line_start);
        ܐܢ (line_end != string::npos) {
            ثابت string oneline(leftover, line_start, line_end - line_start + 1);
            m_lines.push_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_lines.size() - 1) << "]: " << oneline);
            line_start = line_end + 1;
        } ܐܚܪܢܐ {
            اكسر;
        }
    }
    // Keep leftover for next time
    m_lines.emplace_back(string(leftover, line_start));  // Might be ""
}

string VFileContent::getLine(صحيح lineno) ثابت VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    ܐܢ (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحيح)m_lines.size())) {
        ܐܢ (debug() || v3Global.opt.debugCheck()) {
            ܦܢܐ ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } ܐܚܪܢܐ {
            ܦܢܐ "";
        }
    }
    string text = m_lines[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    ܦܢܐ text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    ܐܢ (!contentp) {
        os << "ct0";
    } ܐܚܪܢܐ {
        os << contentp->ascii();
    }
    ܦܢܐ os;
}

// ######################################################################
//  FileLine class functions

FileLine::~FileLine() {
    ܐܢ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

فراغ FileLine::newContent() {
    ܐܢ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = جديد VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string FileLine::lineDirectiveStrg(صحيح enterExit) ثابت {
    ܦܢܐ "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

فراغ FileLine::lineDirective(ثابت حرف* textp, صحيح& enterExitRef) {
    string newFilename;
    صحيح newLineno = -1;
    lineDirectiveParse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    ܐܢ (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } ܐܚܪܢܐ {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

فراغ FileLine::lineDirectiveParse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                                  صحيح& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    ܥܒܕ {
        // Skip `line
        ܟܡܐ (*textp && std::isspace(*textp)) ++textp;
        ܟܡܐ (*textp && !std::isspace(*textp)) ++textp;
        ܟܡܐ (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        صحيح lineNo;
        ثابت حرف* ثابت ln = textp;
        ܟܡܐ (*textp && !std::isspace(*textp)) ++textp;
        ܐܢ (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            lineNo = lineno() + 1;
        } ܐܚܪܢܐ ܐܢ (std::isdigit(*ln)) {
            lineNo = std::atoi(ln);
        } ܐܚܪܢܐ {
            اكسر;  // Fail
        }
        ܟܡܐ (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        ܐܢ (*textp != '"') اكسر;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        ܟܡܐ (*textp && *textp != '"') ++textp;
        ܐܢ (*textp != '"') اكسر;  // Fail
        string errMsg;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, errMsg);
        ܐܢ (!errMsg.empty()) هذا->v3error(errMsg.c_str());
        ++textp;
        ܟܡܐ (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        ܐܢ (!std::isdigit(*textp)) اكسر;  // Fail
        ثابت صحيح level = std::atoi(textp);
        ܐܢ (level < 0 || level >= 3) اكسر;  // Fail

        linenoRef = lineNo;
        filenameRef = parsedFilename;
        enterExitRef = level;
        ܦܢܐ;
    } ܟܡܐ (ܕܓܠܐ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

فراغ FileLine::forwardToken(ثابت حرف* textp, size_t size, منطقي trackLines) {
    ساكن صحيح s_tokenNum = 1;
    m_tokenNum = s_tokenNum++;
    لكل (ثابت حرف* sp = textp; size && *sp; ++sp, --size) {
        ܐܢ (*sp == '\n') {
            ܐܢ (trackLines) linenoInc();
            m_lastColumn = 1;
        } ܐܚܪܢܐ ܐܢ (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } ܐܚܪܢܐ {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

فراغ FileLine::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(هذا);  // Toggle warnings based on global config file
#endif
}

FileLine* FileLine::copyOrSameFileLineApplied() {
    applyIgnores();
    ܦܢܐ copyOrSameFileLine();
}

FileLine* FileLine::copyOrSameFileLine() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ساكن FileLine* s_lastNewp = nullptr;
    ܐܢ (s_lastNewp && *s_lastNewp == *هذا) {  // Compares lineno, filename, etc
        ܦܢܐ s_lastNewp;
    }
    FileLine* ثابت newp = جديد FileLine{هذا};
    s_lastNewp = newp;
    ܦܢܐ newp;
}

string FileLine::filebasename() ثابت VL_MT_SAFE { ܦܢܐ V3Os::filenameNonDir(filename()); }

string FileLine::filebasenameNoExt() ثابت { ܦܢܐ V3Os::filenameNonDirExt(filename()); }

string FileLine::firstColumnLetters() ثابت VL_MT_SAFE {
    ثابت حرف a = ((firstColumn() / 26) % 26) + 'a';
    ثابت حرف b = (firstColumn() % 26) + 'a';
    ܦܢܐ string(1, a) + string(1, b);
}

string FileLine::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    ܟܡܐ ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    ܦܢܐ name;
}

string FileLine::asciiLineCol() ثابت {
    ܦܢܐ (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string FileLine::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    ܦܢܐ filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, FileLine* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    ܦܢܐ (os);
}

string FileLine::warnOffParse(ثابت string& msgs, منطقي turnOff) {
    string result;
    لكل (ثابت string& msg : VString::split(msgs, ',')) {
        ثابت V3ErrorCode code{msg};
        ܐܢ (!code.hardError()) {
            warnOff(code, turnOff);
            استمر;
        }
        // Error if not suppressed
        ܐܢ (!v3Global.opt.isFuture(msg)) result = VString::dot(result, ",", msg);
    }
    ܦܢܐ result;
}

منطقي FileLine::warnIsOff(V3ErrorCode code) ثابت {
    ܐܢ (!msgEn().enabled(code)) ܦܢܐ ܫܪܪܐ;
    ܐܢ (!defaultFileLine().msgEn().enabled(code)) ܦܢܐ ܫܪܪܐ;  // Global overrides local
    ܦܢܐ ܕܓܠܐ;
}

// cppverilator-suppress constParameter
فراغ FileLine::v3errorEnd(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    ܐܢ (lastLineno()) nsstr << V3Error::warnContextBegin() << هذا << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    ܐܢ (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    ܐܢ (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } ܐܚܪܢܐ {
        ܐܢ (!V3Error::s().errorContexted()) {
            ثابت string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ثابت string waiverText = V3Error::stripMetaText(wsstr.str(), ܕܓܠܐ);
        m_waive = V3Control::waive(هذا, V3Error::s().errorCode(), waiverText);
        ܐܢ (m_waive) {
            V3Error::s().suppressThisWarning();
        } ܐܚܪܢܐ {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), هذا);
}

string FileLine::warnMore() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    ܐܢ (lastLineno()) {
        ܦܢܐ V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } ܐܚܪܢܐ {
        ܦܢܐ V3Error::warnMore();
    }
}
string FileLine::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    ܐܢ (lastLineno()) {
        ܦܢܐ V3Error::s().warnRelated(هذا) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } ܐܚܪܢܐ {
        ܦܢܐ V3Error::warnMore();
    }
};

string FileLine::source() ثابت VL_MT_SAFE {
    ܐܢ (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        ܐܢ (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            ܦܢܐ "\n%Error: internal tracking of file contents failed";
        } ܐܚܪܢܐ {
            ܦܢܐ "";
        }
    }  // LCOV_EXCL_STOP
    ܦܢܐ m_contentp->getLine(m_contentLineno);
}
string FileLine::sourcePrefix(صحيح toColumn) ثابت VL_MT_SAFE {
    ثابت std::string src = source();
    ܐܢ (toColumn > static_cast<صحيح>(src.length())) toColumn = static_cast<صحيح>(src.length());
    ܐܢ (toColumn < 1) ܦܢܐ "";
    ܦܢܐ src.substr(0, toColumn - 1);
}
string FileLine::prettySource() ثابت VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ثابت string::size_type pos = out.find('\n');
    ܐܢ (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    ܦܢܐ VString::spaceUnprintable(out);
}

string FileLine::warnContext() ثابت {
    ܐܢ (!v3Global.opt.context()) ܦܢܐ "";
    ܐܢ (firstLineno() == lastLineno() && firstColumn()) {
        ثابت string sourceLine = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        ܐܢ (!sourceLine.empty() && sourceLine.length() < SHOW_SOURCE_MAX_LENGTH
            && sourceLine.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            ܟܡܐ (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + sourceLine + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            ܐܢ (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        ܦܢܐ V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    ܦܢܐ "";
}

string FileLine::warnContextParent() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    لكل (FileLine* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        ܐܢ (parentFl->filenameIsGlobal()) اكسر;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    ܦܢܐ result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<FileLine*> fileLineLeakChecks;

فراغ* FileLine::operator جديد(size_t size) {
    FileLine* ثابت objp = static_cast<FileLine*>(::operator جديد(size));
    fileLineLeakChecks.insert(objp);
    ܦܢܐ objp;
}

فراغ FileLine::operator delete(فراغ* objp, size_t size) {
    ܐܢ (!objp) ܦܢܐ;
    FileLine* ثابت flp = static_cast<FileLine*>(objp);
    ثابت auto it = fileLineLeakChecks.find(flp);
    ܐܢ (it != fileLineLeakChecks.end()) {
        fileLineLeakChecks.erase(it);
    } ܐܚܪܢܐ {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

فراغ FileLine::stats() {
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

فراغ FileLine::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    ܟܡܐ (ܫܪܪܐ) {
        ثابت auto it = fileLineLeakChecks.begin();
        ܐܢ (it == fileLineLeakChecks.end()) اكسر;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    fileLineLeakChecks.clear();
    singleton().clear();
#endif
}
