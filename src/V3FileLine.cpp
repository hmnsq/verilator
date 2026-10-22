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
    𐡏𐡁𐡃 {
        *--op = 'a' + num % 26;
        num /= 26;
    } طالما (num);
    أرجع op;
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
    𐡄𐡍 (pair.second) {
        ثابت size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.push_back(filename);
        m_languages.push_back(V3LangCode::mostRecent());
    }
    أرجع idx;
}

فراغ FileLineSingleton::fileNameNumMapDumpJson(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    لكل (ثابت auto& itr : m_namemap) {
        std::string filename = itr.first;
        𐡄𐡍 (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } وإلا 𐡄𐡍 (filename == V3Options::getStdWaiverPath()) {
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
    𐡄𐡍 (pair.second) {
        ثابت size_t nextIdx = m_internedMsgEns.size();
        UASSERT(nextIdx <= std::numeric_limits<msgEnSetIdx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<msgEnSetIdx_t>(nextIdx);
        m_internedMsgEns.push_back(bitSet);
    }
    أرجع idx;
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::defaultMsgEnIndex() VL_MT_SAFE {
    MsgEnBitSet msgEnBitSet;
    لكل (صحيح i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        ثابت V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        msgEnBitSet.set(MsgEnBitSet::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        msgEnBitSet.set(MsgEnBitSet::Subset::CTRL, code, 𐡒𐡔𐡈);
    }
    أرجع addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgEnSetBit(msgEnSetIdx_t setIdx,
                                                                MsgEnBitSet::Subset subset,
                                                                V3ErrorCode code, منطقي value) {
    // See if state matches existing
    منطقي same = 𐡒𐡔𐡈;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        𐡄𐡍 (msgEn(setIdx).test(subset, subcode) != value) same = خطأ;
    });
    𐡄𐡍 (same) أرجع setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    MsgEnBitSet msgEnBitSet{msgEn(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { msgEnBitSet.set(subset, subcode, value); });
    أرجع addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgSetCtrlBitSet(msgEnSetIdx_t setIdx,
                                                                     ثابت VErrorBitSet& bitset) {
    ثابت MsgEnBitSet::Subset subset = MsgEnBitSet::Subset::CTRL;
    // See if state matches existing
    𐡄𐡍 (msgEn(setIdx).getAll(subset) == bitset) أرجع setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    MsgEnBitSet msgEnBitSet{msgEn(setIdx)};
    msgEnBitSet.setAll(subset, bitset);
    أرجع addMsgEnBitSet(msgEnBitSet);
}

FileLineSingleton::msgEnSetIdx_t FileLineSingleton::msgEnAnd(msgEnSetIdx_t lhsIdx,
                                                             msgEnSetIdx_t rhsIdx) {
    ثابت MsgEnBitSet& lhs = msgEn(lhsIdx);
    ثابت MsgEnBitSet& rhs = msgEn(rhsIdx);
    ثابت MsgEnBitSet intersection{lhs, rhs};
    𐡄𐡍 (intersection == lhs) أرجع lhsIdx;
    𐡄𐡍 (intersection == rhs) أرجع rhsIdx;
    أرجع addMsgEnBitSet(intersection);
}

// ######################################################################
//  VFileContents class functions

فراغ VFileContent::pushText(ثابت string& text) {
    // Similar code in WildcardContents::pushText()
    𐡄𐡍 (m_lines.size() == 0) {
        m_lines.emplace_back("");  // no such thing as line [0]
        m_lines.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ثابت string leftover = m_lines.back() + text;
    m_lines.pop_back();

    // Insert line-by-line
    string::size_type line_start = 0;
    طالما (𐡒𐡔𐡈) {
        ثابت string::size_type line_end = leftover.find('\n', line_start);
        𐡄𐡍 (line_end != string::npos) {
            ثابت string oneline(leftover, line_start, line_end - line_start + 1);
            m_lines.push_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_lines.size() - 1) << "]: " << oneline);
            line_start = line_end + 1;
        } وإلا {
            اكسر;
        }
    }
    // Keep leftover for next time
    m_lines.emplace_back(string(leftover, line_start));  // Might be ""
}

string VFileContent::getLine(صحيح lineno) ثابت VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    𐡄𐡍 (VL_UNCOVERABLE(lineno < 0 || lineno >= (صحيح)m_lines.size())) {
        𐡄𐡍 (debug() || v3Global.opt.debugCheck()) {
            أرجع ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } وإلا {
            أرجع "";
        }
    }
    string text = m_lines[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    أرجع text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    𐡄𐡍 (!contentp) {
        os << "ct0";
    } وإلا {
        os << contentp->ascii();
    }
    أرجع os;
}

// ######################################################################
//  FileLine class functions

FileLine::~FileLine() {
    𐡄𐡍 (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

فراغ FileLine::newContent() {
    𐡄𐡍 (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = جديد VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string FileLine::lineDirectiveStrg(صحيح enterExit) ثابت {
    أرجع "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

فراغ FileLine::lineDirective(ثابت حرف* textp, صحيح& enterExitRef) {
    string newFilename;
    صحيح newLineno = -1;
    lineDirectiveParse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    𐡄𐡍 (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } وإلا {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

فراغ FileLine::lineDirectiveParse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                                  صحيح& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    𐡏𐡁𐡃 {
        // Skip `line
        طالما (*textp && std::isspace(*textp)) ++textp;
        طالما (*textp && !std::isspace(*textp)) ++textp;
        طالما (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        صحيح lineNo;
        ثابت حرف* ثابت ln = textp;
        طالما (*textp && !std::isspace(*textp)) ++textp;
        𐡄𐡍 (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            lineNo = lineno() + 1;
        } وإلا 𐡄𐡍 (std::isdigit(*ln)) {
            lineNo = std::atoi(ln);
        } وإلا {
            اكسر;  // Fail
        }
        طالما (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        𐡄𐡍 (*textp != '"') اكسر;  // Fail
        ثابت حرف* ثابت fn = ++textp;
        طالما (*textp && *textp != '"') ++textp;
        𐡄𐡍 (*textp != '"') اكسر;  // Fail
        string errMsg;
        ثابت string& parsedFilename = VString::unquoteSVString(string{fn, textp}, errMsg);
        𐡄𐡍 (!errMsg.empty()) هذا->v3error(errMsg.c_str());
        ++textp;
        طالما (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        𐡄𐡍 (!std::isdigit(*textp)) اكسر;  // Fail
        ثابت صحيح level = std::atoi(textp);
        𐡄𐡍 (level < 0 || level >= 3) اكسر;  // Fail

        linenoRef = lineNo;
        filenameRef = parsedFilename;
        enterExitRef = level;
        أرجع;
    } طالما (خطأ);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

فراغ FileLine::forwardToken(ثابت حرف* textp, size_t size, منطقي trackLines) {
    ساكن صحيح s_tokenNum = 1;
    m_tokenNum = s_tokenNum++;
    لكل (ثابت حرف* sp = textp; size && *sp; ++sp, --size) {
        𐡄𐡍 (*sp == '\n') {
            𐡄𐡍 (trackLines) linenoInc();
            m_lastColumn = 1;
        } وإلا 𐡄𐡍 (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } وإلا {  // Tabs are considered one column; hence column means number of chars
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
    أرجع copyOrSameFileLine();
}

FileLine* FileLine::copyOrSameFileLine() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    ساكن FileLine* s_lastNewp = nullptr;
    𐡄𐡍 (s_lastNewp && *s_lastNewp == *هذا) {  // Compares lineno, filename, etc
        أرجع s_lastNewp;
    }
    FileLine* ثابت newp = جديد FileLine{هذا};
    s_lastNewp = newp;
    أرجع newp;
}

string FileLine::filebasename() ثابت VL_MT_SAFE { أرجع V3Os::filenameNonDir(filename()); }

string FileLine::filebasenameNoExt() ثابت { أرجع V3Os::filenameNonDirExt(filename()); }

string FileLine::firstColumnLetters() ثابت VL_MT_SAFE {
    ثابت حرف a = ((firstColumn() / 26) % 26) + 'a';
    ثابت حرف b = (firstColumn() % 26) + 'a';
    أرجع string(1, a) + string(1, b);
}

string FileLine::profileFuncname() ثابت {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    طالما ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    أرجع name;
}

string FileLine::asciiLineCol() ثابت {
    أرجع (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string FileLine::ascii() ثابت {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    أرجع filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, FileLine* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    أرجع (os);
}

string FileLine::warnOffParse(ثابت string& msgs, منطقي turnOff) {
    string result;
    لكل (ثابت string& msg : VString::split(msgs, ',')) {
        ثابت V3ErrorCode code{msg};
        𐡄𐡍 (!code.hardError()) {
            warnOff(code, turnOff);
            استمر;
        }
        // Error if not suppressed
        𐡄𐡍 (!v3Global.opt.isFuture(msg)) result = VString::dot(result, ",", msg);
    }
    أرجع result;
}

منطقي FileLine::warnIsOff(V3ErrorCode code) ثابت {
    𐡄𐡍 (!msgEn().enabled(code)) أرجع 𐡒𐡔𐡈;
    𐡄𐡍 (!defaultFileLine().msgEn().enabled(code)) أرجع 𐡒𐡔𐡈;  // Global overrides local
    أرجع خطأ;
}

// cppverilator-suppress constParameter
فراغ FileLine::v3errorEnd(std::ostringstream& sstr, ثابت string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    𐡄𐡍 (lastLineno()) nsstr << V3Error::warnContextBegin() << هذا << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    𐡄𐡍 (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    𐡄𐡍 (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } وإلا {
        𐡄𐡍 (!V3Error::s().errorContexted()) {
            ثابت string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        ثابت string waiverText = V3Error::stripMetaText(wsstr.str(), خطأ);
        m_waive = V3Control::waive(هذا, V3Error::s().errorCode(), waiverText);
        𐡄𐡍 (m_waive) {
            V3Error::s().suppressThisWarning();
        } وإلا {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), هذا);
}

string FileLine::warnMore() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    𐡄𐡍 (lastLineno()) {
        أرجع V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } وإلا {
        أرجع V3Error::warnMore();
    }
}
string FileLine::warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    𐡄𐡍 (lastLineno()) {
        أرجع V3Error::s().warnRelated(هذا) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } وإلا {
        أرجع V3Error::warnMore();
    }
};

string FileLine::source() ثابت VL_MT_SAFE {
    𐡄𐡍 (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        𐡄𐡍 (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            أرجع "\n%Error: internal tracking of file contents failed";
        } وإلا {
            أرجع "";
        }
    }  // LCOV_EXCL_STOP
    أرجع m_contentp->getLine(m_contentLineno);
}
string FileLine::sourcePrefix(صحيح toColumn) ثابت VL_MT_SAFE {
    ثابت std::string src = source();
    𐡄𐡍 (toColumn > static_cast<صحيح>(src.length())) toColumn = static_cast<صحيح>(src.length());
    𐡄𐡍 (toColumn < 1) أرجع "";
    أرجع src.substr(0, toColumn - 1);
}
string FileLine::prettySource() ثابت VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    ثابت string::size_type pos = out.find('\n');
    𐡄𐡍 (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    أرجع VString::spaceUnprintable(out);
}

string FileLine::warnContext() ثابت {
    𐡄𐡍 (!v3Global.opt.context()) أرجع "";
    𐡄𐡍 (firstLineno() == lastLineno() && firstColumn()) {
        ثابت string sourceLine = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        𐡄𐡍 (!sourceLine.empty() && sourceLine.length() < SHOW_SOURCE_MAX_LENGTH
            && sourceLine.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            طالما (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + sourceLine + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            𐡄𐡍 (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        أرجع V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    أرجع "";
}

string FileLine::warnContextParent() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    لكل (FileLine* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        𐡄𐡍 (parentFl->filenameIsGlobal()) اكسر;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    أرجع result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<FileLine*> fileLineLeakChecks;

فراغ* FileLine::operator جديد(size_t size) {
    FileLine* ثابت objp = static_cast<FileLine*>(::operator جديد(size));
    fileLineLeakChecks.insert(objp);
    أرجع objp;
}

فراغ FileLine::operator delete(فراغ* objp, size_t size) {
    𐡄𐡍 (!objp) أرجع;
    FileLine* ثابت flp = static_cast<FileLine*>(objp);
    ثابت auto it = fileLineLeakChecks.find(flp);
    𐡄𐡍 (it != fileLineLeakChecks.end()) {
        fileLineLeakChecks.erase(it);
    } وإلا {
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
    طالما (𐡒𐡔𐡈) {
        ثابت auto it = fileLineLeakChecks.begin();
        𐡄𐡍 (it == fileLineLeakChecks.end()) اكسر;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    fileLineLeakChecks.clear();
    singleton().clear();
#endif
}
