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

string File_पङ्क्ति_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr पूर्णाङ्क size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    वर्ण out[size];
    वर्ण* op = out + size - 1;
    *--op = '\0';  // We build backwards
    पूर्णाङ्क अङ्क = fileno;
    गर {
        *--op = 'a' + अङ्क % 26;
        अङ्क /= 26;
    } जबसम्म (अङ्क);
    फर्काऊ op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_पङ्क्ति_Singleton::fileNameIdx_t File_पङ्क्ति_Singleton::name_To_सङ्ख्या(अचर string& filename) {
    अचर स्वतः pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    यदि (pair.second) {
        अचर माप_प्रकार nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.थप_back(filename);
        m_languages.थप_back(V3LangCode::mostRecent());
    }
    फर्काऊ idx;
}

शून्य File_पङ्क्ति_Singleton::file_Name_अङ्क_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    लागि (अचर स्वतः& itr : m_namemap) {
        std::string filename = itr.first;
        यदि (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } अन्यथा यदि (filename == V3Options::getStdWaiverPath()) {
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

File_पङ्क्ति_Singleton::सन्देश_En_Set_Idx_t File_पङ्क्ति_Singleton::add_सन्देश_En_Bit_Set(अचर सन्देश_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    अचर V3LockGuard lock{m_mutex};
    अचर स्वतः pair = m_interned_सन्देश_En_Idxs.emplace(bitSet, 0);
    सन्देश_En_Set_Idx_t& idx = pair.first->second;
    यदि (pair.second) {
        अचर माप_प्रकार nextIdx = m_interned_सन्देश_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<सन्देश_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<सन्देश_En_Set_Idx_t>(nextIdx);
        m_interned_सन्देश_Ens.थप_back(bitSet);
    }
    फर्काऊ idx;
}

File_पङ्क्ति_Singleton::सन्देश_En_Set_Idx_t File_पङ्क्ति_Singleton::default_सन्देश_En_Index() VL_MT_SAFE {
    सन्देश_En_Bit_Set सन्देश_En_Bit_Set_2;
    लागि (पूर्णाङ्क i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        अचर V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        सन्देश_En_Bit_Set_2.set(सन्देश_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        सन्देश_En_Bit_Set_2.set(सन्देश_En_Bit_Set::Subset::CTRL, code, सत्य);
    }
    फर्काऊ add_सन्देश_En_Bit_Set(सन्देश_En_Bit_Set_2);
}

File_पङ्क्ति_Singleton::सन्देश_En_Set_Idx_t File_पङ्क्ति_Singleton::सन्देश_En_Set_Bit(सन्देश_En_Set_Idx_t setIdx,
                                                                सन्देश_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, तार्किक मान) {
    // See if state matches existing
    तार्किक same = सत्य;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        यदि (सन्देश_En(setIdx).test(subset, subcode) != मान) same = असत्य;
    });
    यदि (same) फर्काऊ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    सन्देश_En_Bit_Set सन्देश_En_Bit_Set_2{सन्देश_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { सन्देश_En_Bit_Set_2.set(subset, subcode, मान); });
    फर्काऊ add_सन्देश_En_Bit_Set(सन्देश_En_Bit_Set_2);
}

File_पङ्क्ति_Singleton::सन्देश_En_Set_Idx_t File_पङ्क्ति_Singleton::सन्देश_Set_Ctrl_Bit_Set(सन्देश_En_Set_Idx_t setIdx,
                                                                     अचर VErrorBitSet& bitset) {
    अचर सन्देश_En_Bit_Set::Subset subset = सन्देश_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    यदि (सन्देश_En(setIdx).getAll(subset) == bitset) फर्काऊ setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    सन्देश_En_Bit_Set सन्देश_En_Bit_Set_2{सन्देश_En(setIdx)};
    सन्देश_En_Bit_Set_2.setAll(subset, bitset);
    फर्काऊ add_सन्देश_En_Bit_Set(सन्देश_En_Bit_Set_2);
}

File_पङ्क्ति_Singleton::सन्देश_En_Set_Idx_t File_पङ्क्ति_Singleton::सन्देश_En_And(सन्देश_En_Set_Idx_t lhsIdx,
                                                             सन्देश_En_Set_Idx_t rhsIdx) {
    अचर सन्देश_En_Bit_Set& lhs = सन्देश_En(lhsIdx);
    अचर सन्देश_En_Bit_Set& rhs = सन्देश_En(rhsIdx);
    अचर सन्देश_En_Bit_Set intersection{lhs, rhs};
    यदि (intersection == lhs) फर्काऊ lhsIdx;
    यदि (intersection == rhs) फर्काऊ rhsIdx;
    फर्काऊ add_सन्देश_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

शून्य VFileContent::थप_Text(अचर string& text) {
    // Similar code in WildcardContents::pushText()
    यदि (m_पङ्क्तिहरू.size() == 0) {
        m_पङ्क्तिहरू.emplace_back("");  // no such thing as line [0]
        m_पङ्क्तिहरू.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    अचर string leftover = m_पङ्क्तिहरू.back() + text;
    m_पङ्क्तिहरू.झिक_back();

    // Insert line-by-line
    string::size_type पङ्क्ति_start = 0;
    जबसम्म (सत्य) {
        अचर string::size_type पङ्क्ति_end = leftover.find('\n', पङ्क्ति_start);
        यदि (पङ्क्ति_end != string::npos) {
            अचर string oneline(leftover, पङ्क्ति_start, पङ्क्ति_end - पङ्क्ति_start + 1);
            m_पङ्क्तिहरू.थप_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_पङ्क्तिहरू.size() - 1) << "]: " << oneline);
            पङ्क्ति_start = पङ्क्ति_end + 1;
        } अन्यथा {
            रोक;
        }
    }
    // Keep leftover for next time
    m_पङ्क्तिहरू.emplace_back(string(leftover, पङ्क्ति_start));  // Might be ""
}

string VFileContent::get_पङ्क्ति(पूर्णाङ्क lineno) अचर VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    यदि (VL_UNCOVERABLE(lineno < 0 || lineno >= (पूर्णाङ्क)m_पङ्क्तिहरू.size())) {
        यदि (debug() || v3Global.opt.debugCheck()) {
            फर्काऊ ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } अन्यथा {
            फर्काऊ "";
        }
    }
    string text = m_पङ्क्तिहरू[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    फर्काऊ text;
}

std::ostream& चालक<<(std::ostream& os, VFileContent* contentp) {
    यदि (!contentp) {
        os << "ct0";
    } अन्यथा {
        os << contentp->ascii();
    }
    फर्काऊ os;
}

// ######################################################################
//  FileLine class functions

File_पङ्क्ति::~File_पङ्क्ति() {
    यदि (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

शून्य File_पङ्क्ति::newContent() {
    यदि (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = नूतन VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_पङ्क्ति::पङ्क्ति_Directive_Strg(पूर्णाङ्क enterExit) अचर {
    फर्काऊ "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

शून्य File_पङ्क्ति::पङ्क्ति_Directive(अचर वर्ण* textp, पूर्णाङ्क& enterExitRef) {
    string newFilename;
    पूर्णाङ्क newLineno = -1;
    पङ्क्ति_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    यदि (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } अन्यथा {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

शून्य File_पङ्क्ति::पङ्क्ति_Directive_Parse(अचर वर्ण* textp, string& filenameRef, पूर्णाङ्क& linenoRef,
                                  पूर्णाङ्क& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    गर {
        // Skip `line
        जबसम्म (*textp && std::है_खाली(*textp)) ++textp;
        जबसम्म (*textp && !std::है_खाली(*textp)) ++textp;
        जबसम्म (*textp && std::है_खाली(*textp)) ++textp;

        // Grab linenumber
        पूर्णाङ्क पङ्क्ति_No;
        अचर वर्ण* अचर ln = textp;
        जबसम्म (*textp && !std::है_खाली(*textp)) ++textp;
        यदि (0 == म_भेदन(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            पङ्क्ति_No = lineno() + 1;
        } अन्यथा यदि (std::है_अंक(*ln)) {
            पङ्क्ति_No = std::म_से_प(ln);
        } अन्यथा {
            रोक;  // Fail
        }
        जबसम्म (*textp && (std::है_खाली(*textp))) ++textp;

        // Grab filename
        यदि (*textp != '"') रोक;  // Fail
        अचर वर्ण* अचर fn = ++textp;
        जबसम्म (*textp && *textp != '"') ++textp;
        यदि (*textp != '"') रोक;  // Fail
        string err_सन्देश;
        अचर string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_सन्देश);
        यदि (!err_सन्देश.empty()) एतत्->v3error(err_सन्देश.c_str());
        ++textp;
        जबसम्म (*textp && std::है_खाली(*textp)) ++textp;

        // Grab level
        यदि (!std::है_अंक(*textp)) रोक;  // Fail
        अचर पूर्णाङ्क level = std::म_से_प(textp);
        यदि (level < 0 || level >= 3) रोक;  // Fail

        linenoRef = पङ्क्ति_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        फर्काऊ;
    } जबसम्म (असत्य);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

शून्य File_पङ्क्ति::forwardToken(अचर वर्ण* textp, माप_प्रकार size, तार्किक track_पङ्क्तिहरू) {
    स्थिर पूर्णाङ्क s_token_अङ्क = 1;
    m_token_अङ्क = s_token_अङ्क++;
    लागि (अचर वर्ण* sp = textp; size && *sp; ++sp, --size) {
        यदि (*sp == '\n') {
            यदि (track_पङ्क्तिहरू) linenoInc();
            m_lastColumn = 1;
        } अन्यथा यदि (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } अन्यथा {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

शून्य File_पङ्क्ति::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(एतत्);  // Toggle warnings based on global config file
#endif
}

File_पङ्क्ति* File_पङ्क्ति::copy_Or_Same_File_पङ्क्ति_Applied() {
    applyIgnores();
    फर्काऊ copy_Or_Same_File_पङ्क्ति();
}

File_पङ्क्ति* File_पङ्क्ति::copy_Or_Same_File_पङ्क्ति() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    स्थिर File_पङ्क्ति* s_lastNewp = nullptr;
    यदि (s_lastNewp && *s_lastNewp == *एतत्) {  // Compares lineno, filename, etc
        फर्काऊ s_lastNewp;
    }
    File_पङ्क्ति* अचर newp = नूतन File_पङ्क्ति{एतत्};
    s_lastNewp = newp;
    फर्काऊ newp;
}

string File_पङ्क्ति::filebasename() अचर VL_MT_SAFE { फर्काऊ V3Os::filenameNonDir(filename()); }

string File_पङ्क्ति::filebasenameNoExt() अचर { फर्काऊ V3Os::filenameNonDirExt(filename()); }

string File_पङ्क्ति::firstColumnLetters() अचर VL_MT_SAFE {
    अचर वर्ण a = ((firstColumn() / 26) % 26) + 'a';
    अचर वर्ण b = (firstColumn() % 26) + 'a';
    फर्काऊ string(1, a) + string(1, b);
}

string File_पङ्क्ति::profileFuncname() अचर {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    जबसम्म ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    फर्काऊ name;
}

string File_पङ्क्ति::ascii_पङ्क्ति_Col() अचर {
    फर्काऊ (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_पङ्क्ति::ascii() अचर {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    फर्काऊ filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& चालक<<(std::ostream& os, File_पङ्क्ति* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    फर्काऊ (os);
}

string File_पङ्क्ति::warnOffParse(अचर string& msgs, तार्किक turnOff) {
    string result;
    लागि (अचर string& सन्देश : VString::split(msgs, ',')) {
        अचर V3ErrorCode code{सन्देश};
        यदि (!code.hardError()) {
            warnOff(code, turnOff);
            जारी;
        }
        // Error if not suppressed
        यदि (!v3Global.opt.isFuture(सन्देश)) result = VString::dot(result, ",", सन्देश);
    }
    फर्काऊ result;
}

तार्किक File_पङ्क्ति::warnIsOff(V3ErrorCode code) अचर {
    यदि (!सन्देश_En().enabled(code)) फर्काऊ सत्य;
    यदि (!default_File_पङ्क्ति().सन्देश_En().enabled(code)) फर्काऊ सत्य;  // Global overrides local
    फर्काऊ असत्य;
}

// cppverilator-suppress constParameter
शून्य File_पङ्क्ति::v3errorEnd(std::ostringstream& sstr, अचर string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    यदि (lastLineno()) nsstr << V3Error::warnContextBegin() << एतत् << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    यदि (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    यदि (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } अन्यथा {
        यदि (!V3Error::s().errorContexted()) {
            अचर string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        अचर string waiverText = V3Error::stripMetaText(wsstr.str(), असत्य);
        m_waive = V3Control::waive(एतत्, V3Error::s().errorCode(), waiverText);
        यदि (m_waive) {
            V3Error::s().suppressThisWarning();
        } अन्यथा {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), एतत्);
}

string File_पङ्क्ति::warnMore() अचर VL_REQUIRES(V3Error::s().m_mutex) {
    यदि (lastLineno()) {
        फर्काऊ V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } अन्यथा {
        फर्काऊ V3Error::warnMore();
    }
}
string File_पङ्क्ति::warnOther() अचर VL_REQUIRES(V3Error::s().m_mutex) {
    यदि (lastLineno()) {
        फर्काऊ V3Error::s().warnRelated(एतत्) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } अन्यथा {
        फर्काऊ V3Error::warnMore();
    }
};

string File_पङ्क्ति::source() अचर VL_MT_SAFE {
    यदि (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        यदि (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            फर्काऊ "\n%Error: internal tracking of file contents failed";
        } अन्यथा {
            फर्काऊ "";
        }
    }  // LCOV_EXCL_STOP
    फर्काऊ m_contentp->get_पङ्क्ति(m_contentLineno);
}
string File_पङ्क्ति::sourcePrefix(पूर्णाङ्क toColumn) अचर VL_MT_SAFE {
    अचर std::string src = source();
    यदि (toColumn > static_cast<पूर्णाङ्क>(src.length())) toColumn = static_cast<पूर्णाङ्क>(src.length());
    यदि (toColumn < 1) फर्काऊ "";
    फर्काऊ src.substr(0, toColumn - 1);
}
string File_पङ्क्ति::prettySource() अचर VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    अचर string::size_type pos = out.find('\n');
    यदि (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    फर्काऊ VString::spaceUnprintable(out);
}

string File_पङ्क्ति::warnContext() अचर {
    यदि (!v3Global.opt.context()) फर्काऊ "";
    यदि (firstLineno() == lastLineno() && firstColumn()) {
        अचर string source_पङ्क्ति = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        यदि (!source_पङ्क्ति.empty() && source_पङ्क्ति.length() < SHOW_SOURCE_MAX_LENGTH
            && source_पङ्क्ति.length() >= static_cast<माप_प्रकार>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            जबसम्म (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_पङ्क्ति + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            यदि (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        फर्काऊ V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    फर्काऊ "";
}

string File_पङ्क्ति::warnContextParent() अचर VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    लागि (File_पङ्क्ति* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        यदि (parentFl->filenameIsGlobal()) रोक;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    फर्काऊ result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_पङ्क्ति*> file_पङ्क्ति_Leak_Checks;

शून्य* File_पङ्क्ति::चालक नूतन(माप_प्रकार size) {
    File_पङ्क्ति* अचर objp = static_cast<File_पङ्क्ति*>(::चालक नूतन(size));
    file_पङ्क्ति_Leak_Checks.घुसाउ(objp);
    फर्काऊ objp;
}

शून्य File_पङ्क्ति::चालक delete(शून्य* objp, माप_प्रकार size) {
    यदि (!objp) फर्काऊ;
    File_पङ्क्ति* अचर flp = static_cast<File_पङ्क्ति*>(objp);
    अचर स्वतः it = file_पङ्क्ति_Leak_Checks.find(flp);
    यदि (it != file_पङ्क्ति_Leak_Checks.end()) {
        file_पङ्क्ति_Leak_Checks.erase(it);
    } अन्यथा {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::चालक delete(objp);
}
#endif

शून्य File_पङ्क्ति::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_सन्देश_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

शून्य File_पङ्क्ति::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    जबसम्म (सत्य) {
        अचर स्वतः it = file_पङ्क्ति_Leak_Checks.begin();
        यदि (it == file_पङ्क्ति_Leak_Checks.end()) रोक;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_पङ्क्ति_Leak_Checks.clear();
    singleton().clear();
#endif
}
