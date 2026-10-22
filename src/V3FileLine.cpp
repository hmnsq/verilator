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

#समावेश "config_build.h"
#समावेश "verilatedos.h"

// clang-format off
#समावेश "V3Error.h"
#समावेश "V3FileLine.h"
#समावेश "V3Os.h"
#समावेश "V3String.h"
#अगर_अघोषित V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#पूर्ण_अगर
#समावेश "V3Waiver.h"
// clang-format on

#समावेश <algorithm>
#समावेश <iomanip>
#समावेश <limits>
#समावेश <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_पंक्ति_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr पूर्णांक आकार
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    अक्षर out[आकार];
    अक्षर* op = out + आकार - 1;
    *--op = '\0';  // We build backwards
    पूर्णांक अंक = fileno;
    करो {
        *--op = 'a' + अंक % 26;
        अंक /= 26;
    } जबतक (अंक);
    वापस op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_पंक्ति_Singleton::fileNameIdx_t File_पंक्ति_Singleton::name_To_संख्या(स्थिर string& filename) {
    स्थिर स्वतः pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    अगर (pair.second) {
        स्थिर माप_प्रकार अग्र_Idx = m_names.आकार();
        UASSERT(अग्र_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(अग्र_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(अग्र_Idx);
        m_names.धकेलो_back(filename);
        m_languages.धकेलो_back(V3LangCode::mostRecent());
    }
    वापस idx;
}

व्योम File_पंक्ति_Singleton::file_Name_अंक_Map_Dump_Json(std::ब_प्रवाह& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    क्रम (स्थिर स्वतः& itr : m_namemap) {
        std::string filename = itr.first;
        अगर (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } अन्यथा अगर (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << संख्या_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_पंक्ति_Singleton::संदेश_En_Set_Idx_t File_पंक्ति_Singleton::add_संदेश_En_Bit_Set(स्थिर संदेश_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    स्थिर V3LockGuard lock{m_mutex};
    स्थिर स्वतः pair = m_interned_संदेश_En_Idxs.emplace(bitSet, 0);
    संदेश_En_Set_Idx_t& idx = pair.first->second;
    अगर (pair.second) {
        स्थिर माप_प्रकार अग्र_Idx = m_interned_संदेश_Ens.आकार();
        UASSERT(अग्र_Idx <= std::numeric_limits<संदेश_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(अग्र_Idx) + "+).");
        idx = static_cast<संदेश_En_Set_Idx_t>(अग्र_Idx);
        m_interned_संदेश_Ens.धकेलो_back(bitSet);
    }
    वापस idx;
}

File_पंक्ति_Singleton::संदेश_En_Set_Idx_t File_पंक्ति_Singleton::default_संदेश_En_सूचकांक() VL_MT_SAFE {
    संदेश_En_Bit_Set संदेश_En_Bit_Set_2;
    क्रम (पूर्णांक i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        स्थिर V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        संदेश_En_Bit_Set_2.set(संदेश_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        संदेश_En_Bit_Set_2.set(संदेश_En_Bit_Set::Subset::CTRL, code, true);
    }
    वापस add_संदेश_En_Bit_Set(संदेश_En_Bit_Set_2);
}

File_पंक्ति_Singleton::संदेश_En_Set_Idx_t File_पंक्ति_Singleton::संदेश_En_Set_Bit(संदेश_En_Set_Idx_t setIdx,
                                                                संदेश_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, bool मूल्य) {
    // See if state matches existing
    bool same = true;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        अगर (संदेश_En(setIdx).test(subset, subcode) != मूल्य) same = false;
    });
    अगर (same) वापस setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    संदेश_En_Bit_Set संदेश_En_Bit_Set_2{संदेश_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { संदेश_En_Bit_Set_2.set(subset, subcode, मूल्य); });
    वापस add_संदेश_En_Bit_Set(संदेश_En_Bit_Set_2);
}

File_पंक्ति_Singleton::संदेश_En_Set_Idx_t File_पंक्ति_Singleton::संदेश_Set_Ctrl_Bit_Set(संदेश_En_Set_Idx_t setIdx,
                                                                     स्थिर VErrorBitSet& bitset) {
    स्थिर संदेश_En_Bit_Set::Subset subset = संदेश_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    अगर (संदेश_En(setIdx).getAll(subset) == bitset) वापस setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    संदेश_En_Bit_Set संदेश_En_Bit_Set_2{संदेश_En(setIdx)};
    संदेश_En_Bit_Set_2.setAll(subset, bitset);
    वापस add_संदेश_En_Bit_Set(संदेश_En_Bit_Set_2);
}

File_पंक्ति_Singleton::संदेश_En_Set_Idx_t File_पंक्ति_Singleton::संदेश_En_And(संदेश_En_Set_Idx_t lhsIdx,
                                                             संदेश_En_Set_Idx_t rhsIdx) {
    स्थिर संदेश_En_Bit_Set& lhs = संदेश_En(lhsIdx);
    स्थिर संदेश_En_Bit_Set& rhs = संदेश_En(rhsIdx);
    स्थिर संदेश_En_Bit_Set intersection{lhs, rhs};
    अगर (intersection == lhs) वापस lhsIdx;
    अगर (intersection == rhs) वापस rhsIdx;
    वापस add_संदेश_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

व्योम VFileContent::धकेलो_Text(स्थिर string& text) {
    // Similar code in WildcardContents::pushText()
    अगर (m_पंक्तियाँ.आकार() == 0) {
        m_पंक्तियाँ.emplace_back("");  // no such thing as line [0]
        m_पंक्तियाँ.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    स्थिर string leftover = m_पंक्तियाँ.back() + text;
    m_पंक्तियाँ.निकालो_back();

    // Insert line-by-line
    string::आकार_type पंक्ति_start = 0;
    जबतक (true) {
        स्थिर string::आकार_type पंक्ति_end = leftover.find('\n', पंक्ति_start);
        अगर (पंक्ति_end != string::npos) {
            स्थिर string oneline(leftover, पंक्ति_start, पंक्ति_end - पंक्ति_start + 1);
            m_पंक्तियाँ.धकेलो_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_पंक्तियाँ.आकार() - 1) << "]: " << oneline);
            पंक्ति_start = पंक्ति_end + 1;
        } अन्यथा {
            अवरोध;
        }
    }
    // Keep leftover for next time
    m_पंक्तियाँ.emplace_back(string(leftover, पंक्ति_start));  // Might be ""
}

string VFileContent::get_पंक्ति(पूर्णांक lineno) स्थिर VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    अगर (VL_UNCOVERABLE(lineno < 0 || lineno >= (पूर्णांक)m_पंक्तियाँ.आकार())) {
        अगर (debug() || v3Global.opt.debugCheck()) {
            वापस ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } अन्यथा {
            वापस "";
        }
    }
    string text = m_पंक्तियाँ[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    वापस text;
}

std::ब_प्रवाह& चालक<<(std::ब_प्रवाह& os, VFileContent* contentp) {
    अगर (!contentp) {
        os << "ct0";
    } अन्यथा {
        os << contentp->ascii();
    }
    वापस os;
}

// ######################################################################
//  FileLine class functions

File_पंक्ति::~File_पंक्ति() {
    अगर (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

व्योम File_पंक्ति::newContent() {
    अगर (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = नया VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_पंक्ति::पंक्ति_Directive_Strg(पूर्णांक enterExit) स्थिर {
    वापस "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

व्योम File_पंक्ति::पंक्ति_Directive(स्थिर अक्षर* textp, पूर्णांक& enterExitRef) {
    string newFilename;
    पूर्णांक newLineno = -1;
    पंक्ति_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    अगर (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } अन्यथा {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

व्योम File_पंक्ति::पंक्ति_Directive_Parse(स्थिर अक्षर* textp, string& filenameRef, पूर्णांक& linenoRef,
                                  पूर्णांक& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    करो {
        // Skip `line
        जबतक (*textp && std::है_खाली(*textp)) ++textp;
        जबतक (*textp && !std::है_खाली(*textp)) ++textp;
        जबतक (*textp && std::है_खाली(*textp)) ++textp;

        // Grab linenumber
        पूर्णांक पंक्ति_No;
        स्थिर अक्षर* स्थिर ln = textp;
        जबतक (*textp && !std::है_खाली(*textp)) ++textp;
        अगर (0 == म_भेदन(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            पंक्ति_No = lineno() + 1;
        } अन्यथा अगर (std::है_अंक(*ln)) {
            पंक्ति_No = std::म_से_प(ln);
        } अन्यथा {
            अवरोध;  // Fail
        }
        जबतक (*textp && (std::है_खाली(*textp))) ++textp;

        // Grab filename
        अगर (*textp != '"') अवरोध;  // Fail
        स्थिर अक्षर* स्थिर fn = ++textp;
        जबतक (*textp && *textp != '"') ++textp;
        अगर (*textp != '"') अवरोध;  // Fail
        string err_संदेश;
        स्थिर string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_संदेश);
        अगर (!err_संदेश.empty()) यह->v3error(err_संदेश.c_str());
        ++textp;
        जबतक (*textp && std::है_खाली(*textp)) ++textp;

        // Grab level
        अगर (!std::है_अंक(*textp)) अवरोध;  // Fail
        स्थिर पूर्णांक level = std::म_से_प(textp);
        अगर (level < 0 || level >= 3) अवरोध;  // Fail

        linenoRef = पंक्ति_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        वापस;
    } जबतक (false);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

व्योम File_पंक्ति::forwardToken(स्थिर अक्षर* textp, माप_प्रकार आकार, bool track_पंक्तियाँ) {
    अटल पूर्णांक s_token_अंक = 1;
    m_token_अंक = s_token_अंक++;
    क्रम (स्थिर अक्षर* sp = textp; आकार && *sp; ++sp, --आकार) {
        अगर (*sp == '\n') {
            अगर (track_पंक्तियाँ) linenoInc();
            m_lastColumn = 1;
        } अन्यथा अगर (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } अन्यथा {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

व्योम File_पंक्ति::applyIgnores() {
#अगर_अघोषित V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(यह);  // Toggle warnings based on global config file
#पूर्ण_अगर
}

File_पंक्ति* File_पंक्ति::copy_Or_Same_File_पंक्ति_Applied() {
    applyIgnores();
    वापस copy_Or_Same_File_पंक्ति();
}

File_पंक्ति* File_पंक्ति::copy_Or_Same_File_पंक्ति() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    अटल File_पंक्ति* s_lastNewp = nullptr;
    अगर (s_lastNewp && *s_lastNewp == *यह) {  // Compares lineno, filename, etc
        वापस s_lastNewp;
    }
    File_पंक्ति* स्थिर newp = नया File_पंक्ति{यह};
    s_lastNewp = newp;
    वापस newp;
}

string File_पंक्ति::filebasename() स्थिर VL_MT_SAFE { वापस V3Os::filenameNonDir(filename()); }

string File_पंक्ति::filebasenameNoExt() स्थिर { वापस V3Os::filenameNonDirExt(filename()); }

string File_पंक्ति::firstColumnLetters() स्थिर VL_MT_SAFE {
    स्थिर अक्षर a = ((firstColumn() / 26) % 26) + 'a';
    स्थिर अक्षर b = (firstColumn() % 26) + 'a';
    वापस string(1, a) + string(1, b);
}

string File_पंक्ति::profileFuncname() स्थिर {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::आकार_type pos;
    जबतक ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    वापस name;
}

string File_पंक्ति::ascii_पंक्ति_Col() स्थिर {
    वापस (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_पंक्ति::ascii() स्थिर {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    वापस filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ब_प्रवाह& चालक<<(std::ब_प्रवाह& os, File_पंक्ति* fileline) {
    os << fileline->ascii() << ": " << std::षष्ठ;
    वापस (os);
}

string File_पंक्ति::warnOffParse(स्थिर string& msgs, bool turnOff) {
    string परिणाम;
    क्रम (स्थिर string& संदेश : VString::split(msgs, ',')) {
        स्थिर V3ErrorCode code{संदेश};
        अगर (!code.hardError()) {
            warnOff(code, turnOff);
            जारी;
        }
        // Error if not suppressed
        अगर (!v3Global.opt.isFuture(संदेश)) परिणाम = VString::dot(परिणाम, ",", संदेश);
    }
    वापस परिणाम;
}

bool File_पंक्ति::warnIsOff(V3ErrorCode code) स्थिर {
    अगर (!संदेश_En().enabled(code)) वापस true;
    अगर (!default_File_पंक्ति().संदेश_En().enabled(code)) वापस true;  // Global overrides local
    वापस false;
}

// cppverilator-suppress constParameter
व्योम File_पंक्ति::v3errorEnd(std::ostringstream& sstr, स्थिर string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    अगर (lastLineno()) nsstr << V3Error::warnContextBegin() << यह << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    अगर (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::रखो_चौड़ाई(ascii().लंबाई()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    अगर (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } अन्यथा {
        अगर (!V3Error::s().errorContexted()) {
            स्थिर string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        स्थिर string waiverText = V3Error::stripMetaText(wsstr.str(), false);
        m_waive = V3Control::waive(यह, V3Error::s().errorCode(), waiverText);
        अगर (m_waive) {
            V3Error::s().suppressThisWarning();
        } अन्यथा {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), यह);
}

string File_पंक्ति::warnMore() स्थिर VL_REQUIRES(V3Error::s().m_mutex) {
    अगर (lastLineno()) {
        वापस V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().आकार(), ' ')
               + ": " + V3Error::warnContextEnd();
    } अन्यथा {
        वापस V3Error::warnMore();
    }
}
string File_पंक्ति::warnOther() स्थिर VL_REQUIRES(V3Error::s().m_mutex) {
    अगर (lastLineno()) {
        वापस V3Error::s().warnRelated(यह) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } अन्यथा {
        वापस V3Error::warnMore();
    }
};

string File_पंक्ति::source() स्थिर VL_MT_SAFE {
    अगर (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        अगर (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            वापस "\n%Error: internal tracking of file contents failed";
        } अन्यथा {
            वापस "";
        }
    }  // LCOV_EXCL_STOP
    वापस m_contentp->get_पंक्ति(m_contentLineno);
}
string File_पंक्ति::sourcePrefix(पूर्णांक toColumn) स्थिर VL_MT_SAFE {
    स्थिर std::string src = source();
    अगर (toColumn > static_cast<पूर्णांक>(src.लंबाई())) toColumn = static_cast<पूर्णांक>(src.लंबाई());
    अगर (toColumn < 1) वापस "";
    वापस src.substr(0, toColumn - 1);
}
string File_पंक्ति::prettySource() स्थिर VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    स्थिर string::आकार_type pos = out.find('\n');
    अगर (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    वापस VString::spaceUnprintable(out);
}

string File_पंक्ति::warnContext() स्थिर {
    अगर (!v3Global.opt.context()) वापस "";
    अगर (firstLineno() == lastLineno() && firstColumn()) {
        स्थिर string source_पंक्ति = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        अगर (!source_पंक्ति.empty() && source_पंक्ति.लंबाई() < SHOW_SOURCE_MAX_लंबाई
            && source_पंक्ति.लंबाई() >= static_cast<माप_प्रकार>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            जबतक (linestr.आकार() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_पंक्ति + '\n';
            out += std::string(linestr.आकार(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            अगर (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        वापस V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    वापस "";
}

string File_पंक्ति::warnContextParent() स्थिर VL_REQUIRES(V3Error::s().m_mutex) {
    string परिणाम;
    क्रम (File_पंक्ति* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        अगर (parentFl->filenameIsGlobal()) अवरोध;
        परिणाम += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    वापस परिणाम;
}

#अगर_घोषित VL_LEAK_CHECKS
std::unordered_set<File_पंक्ति*> file_पंक्ति_Leak_Checks;

व्योम* File_पंक्ति::चालक नया(माप_प्रकार आकार) {
    File_पंक्ति* स्थिर objp = static_cast<File_पंक्ति*>(::चालक नया(आकार));
    file_पंक्ति_Leak_Checks.जोड़ो(objp);
    वापस objp;
}

व्योम File_पंक्ति::चालक मिटाओ(व्योम* objp, माप_प्रकार आकार) {
    अगर (!objp) वापस;
    File_पंक्ति* स्थिर flp = static_cast<File_पंक्ति*>(objp);
    स्थिर स्वतः it = file_पंक्ति_Leak_Checks.find(flp);
    अगर (it != file_पंक्ति_Leak_Checks.end()) {
        file_पंक्ति_Leak_Checks.erase(it);
    } अन्यथा {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::चालक मिटाओ(objp);
}
#पूर्ण_अगर

व्योम File_पंक्ति::stats() {
#अगर_अघोषित V3ERROR_NO_GLOBAL_
    V3Stats::add_Stat_योग("FileLines, Number of filenames",
                        singleton().m_names.आकार());  // Max m_filenameno
    V3Stats::add_Stat_योग("FileLines, Message enable sets",
                        singleton().m_interned_संदेश_Ens.आकार());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#पूर्ण_अगर
}

व्योम File_पंक्ति::deleteAllRemaining() {
#अगर_घोषित VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    जबतक (true) {
        स्थिर स्वतः it = file_पंक्ति_Leak_Checks.begin();
        अगर (it == file_पंक्ति_Leak_Checks.end()) अवरोध;
        मिटाओ *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_पंक्ति_Leak_Checks.सफा();
    singleton().सफा();
#पूर्ण_अगर
}
