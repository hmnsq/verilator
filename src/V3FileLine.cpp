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
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#यदि_अन्तः
#अन्तर्भाव "V3Waiver.h"
// clang-format on

#अन्तर्भाव <algorithm>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <limits>
#अन्तर्भाव <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_पङ्क्तिः_Singleton::filenameLetters(fileNameIdx_t fileno) VL_विशुद्धम् {
    constexpr पूर्णाङ्क आकार
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    वर्ण बहिर्गमनम्[आकार];
    वर्ण* op = बहिर्गमनम् + आकार - 1;
    *--op = '\0';  // We build backwards
    पूर्णाङ्क अङ्कः = fileno;
    कुरु {
        *--op = 'a' + अङ्कः % 26;
        अङ्कः /= 26;
    } यावत् (अङ्कः);
    प्रत्यागम op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_पङ्क्तिः_Singleton::fileNameIdx_t File_पङ्क्तिः_Singleton::name_गन्तव्यम्_सङ्ख्या(ध्रुव string& filename) {
    ध्रुव स्वचालित pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    यदि (pair.second) {
        ध्रुव आकार_t अग्र_Idx = m_नामानि.आकार();
        UASSERT(अग्र_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_गन्तव्यम्_Str(अग्र_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(अग्र_Idx);
        m_नामानि.प्रक्षिप_पृष्ठभागः(filename);
        m_languages.प्रक्षिप_पृष्ठभागः(V3_Lang_संहिता::mostRecent());
    }
    प्रत्यागम idx;
}

शून्य File_पङ्क्तिः_Singleton::file_Name_अङ्कः_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    प्रति (ध्रुव स्वचालित& itr : m_namemap) {
        std::string filename = itr.first;
        यदि (filename == V3Options::get_Std_Package_मार्गः()) {
            filename = "<verilated_std>";
        } अन्यथा यदि (filename == V3Options::get_Std_Waiver_मार्गः()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_बहिर्गमनम्_Formatter::quoteNameControls(V3Os::filename_Real_मार्गः(itr.first)) << '"'
           << ", \"language\":\"" << सङ्ख्या_गन्तव्यम्_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_पङ्क्तिः_Singleton::सन्देशः_En_समुच्चयः_Idx_t File_पङ्क्तिः_Singleton::सङ्कलय_सन्देशः_En_Bit_समुच्चयः(ध्रुव सन्देशः_En_Bit_समुच्चयः& bit_समुच्चयः)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
    ध्रुव स्वचालित pair = m_interned_सन्देशः_En_Idxs.emplace(bit_समुच्चयः, 0);
    सन्देशः_En_समुच्चयः_Idx_t& idx = pair.first->second;
    यदि (pair.second) {
        ध्रुव आकार_t अग्र_Idx = m_interned_सन्देशः_Ens.आकार();
        UASSERT(अग्र_Idx <= std::numeric_limits<सन्देशः_En_समुच्चयः_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_गन्तव्यम्_Str(अग्र_Idx) + "+).");
        idx = static_cast<सन्देशः_En_समुच्चयः_Idx_t>(अग्र_Idx);
        m_interned_सन्देशः_Ens.प्रक्षिप_पृष्ठभागः(bit_समुच्चयः);
    }
    प्रत्यागम idx;
}

File_पङ्क्तिः_Singleton::सन्देशः_En_समुच्चयः_Idx_t File_पङ्क्तिः_Singleton::default_सन्देशः_En_सूचकांक() VL_MT_SAFE {
    सन्देशः_En_Bit_समुच्चयः सन्देशः_En_Bit_समुच्चयः_2;
    प्रति (पूर्णाङ्क i = V3_Error_संहिता::EC_MIN; i < V3_Error_संहिता::_ENUM_MAX; ++i) {
        ध्रुव V3_Error_संहिता संहिता{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        सन्देशः_En_Bit_समुच्चयः_2.समुच्चयः(सन्देशः_En_Bit_समुच्चयः::Subset::संहिता_2, संहिता, !संहिता.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        सन्देशः_En_Bit_समुच्चयः_2.समुच्चयः(सन्देशः_En_Bit_समुच्चयः::Subset::CTRL, संहिता, सत्य);
    }
    प्रत्यागम सङ्कलय_सन्देशः_En_Bit_समुच्चयः(सन्देशः_En_Bit_समुच्चयः_2);
}

File_पङ्क्तिः_Singleton::सन्देशः_En_समुच्चयः_Idx_t File_पङ्क्तिः_Singleton::सन्देशः_En_समुच्चयः_Bit(सन्देशः_En_समुच्चयः_Idx_t समुच्चयः_Idx,
                                                                सन्देशः_En_Bit_समुच्चयः::Subset subset,
                                                                V3_Error_संहिता संहिता, तार्किक मानम्) {
    // See if state matches existing
    तार्किक same = सत्य;
    संहिता.forDelegateCodes([&](V3_Error_संहिता subcode) {
        यदि (सन्देशः_En(समुच्चयः_Idx).test(subset, subcode) != मानम्) same = असत्य;
    });
    यदि (same) प्रत्यागम समुच्चयः_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    सन्देशः_En_Bit_समुच्चयः सन्देशः_En_Bit_समुच्चयः_2{सन्देशः_En(समुच्चयः_Idx)};
    संहिता.forDelegateCodes([&](V3_Error_संहिता subcode) { सन्देशः_En_Bit_समुच्चयः_2.समुच्चयः(subset, subcode, मानम्); });
    प्रत्यागम सङ्कलय_सन्देशः_En_Bit_समुच्चयः(सन्देशः_En_Bit_समुच्चयः_2);
}

File_पङ्क्तिः_Singleton::सन्देशः_En_समुच्चयः_Idx_t File_पङ्क्तिः_Singleton::सन्देशः_समुच्चयः_Ctrl_Bit_समुच्चयः(सन्देशः_En_समुच्चयः_Idx_t समुच्चयः_Idx,
                                                                     ध्रुव VError_Bit_समुच्चयः& bitset) {
    ध्रुव सन्देशः_En_Bit_समुच्चयः::Subset subset = सन्देशः_En_Bit_समुच्चयः::Subset::CTRL;
    // See if state matches existing
    यदि (सन्देशः_En(समुच्चयः_Idx).getAll(subset) == bitset) प्रत्यागम समुच्चयः_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    सन्देशः_En_Bit_समुच्चयः सन्देशः_En_Bit_समुच्चयः_2{सन्देशः_En(समुच्चयः_Idx)};
    सन्देशः_En_Bit_समुच्चयः_2.समुच्चयः_All(subset, bitset);
    प्रत्यागम सङ्कलय_सन्देशः_En_Bit_समुच्चयः(सन्देशः_En_Bit_समुच्चयः_2);
}

File_पङ्क्तिः_Singleton::सन्देशः_En_समुच्चयः_Idx_t File_पङ्क्तिः_Singleton::सन्देशः_En_And(सन्देशः_En_समुच्चयः_Idx_t lhsIdx,
                                                             सन्देशः_En_समुच्चयः_Idx_t rhsIdx) {
    ध्रुव सन्देशः_En_Bit_समुच्चयः& lhs = सन्देशः_En(lhsIdx);
    ध्रुव सन्देशः_En_Bit_समुच्चयः& rhs = सन्देशः_En(rhsIdx);
    ध्रुव सन्देशः_En_Bit_समुच्चयः intersection{lhs, rhs};
    यदि (intersection == lhs) प्रत्यागम lhsIdx;
    यदि (intersection == rhs) प्रत्यागम rhsIdx;
    प्रत्यागम सङ्कलय_सन्देशः_En_Bit_समुच्चयः(intersection);
}

// ######################################################################
//  VFileContents class functions

शून्य VFileContent::प्रक्षिप_Text(ध्रुव string& text) {
    // Similar code in WildcardContents::pushText()
    यदि (m_पङ्क्तयः.आकार() == 0) {
        m_पङ्क्तयः.emplace_पृष्ठभागः("");  // no such thing as line [0]
        m_पङ्क्तयः.emplace_पृष्ठभागः("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    ध्रुव string leftover = m_पङ्क्तयः.पृष्ठभागः() + text;
    m_पङ्क्तयः.निष्कासय_पृष्ठभागः();

    // Insert line-by-line
    string::आकार_type पङ्क्तिः_start = 0;
    यावत् (सत्य) {
        ध्रुव string::आकार_type पङ्क्तिः_अन्तः = leftover.find('\n', पङ्क्तिः_start);
        यदि (पङ्क्तिः_अन्तः != string::npos) {
            ध्रुव string oneline(leftover, पङ्क्तिः_start, पङ्क्तिः_अन्तः - पङ्क्तिः_start + 1);
            m_पङ्क्तयः.प्रक्षिप_पृष्ठभागः(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_पङ्क्तयः.आकार() - 1) << "]: " << oneline);
            पङ्क्तिः_start = पङ्क्तिः_अन्तः + 1;
        } अन्यथा {
            भङ्ग;
        }
    }
    // Keep leftover for next time
    m_पङ्क्तयः.emplace_पृष्ठभागः(string(leftover, पङ्क्तिः_start));  // Might be ""
}

string VFileContent::get_पङ्क्तिः(पूर्णाङ्क lineno) ध्रुव VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    यदि (VL_UNCOVERABLE(lineno < 0 || lineno >= (पूर्णाङ्क)m_पङ्क्तयः.आकार())) {
        यदि (debug() || v3Global.opt.debugCheck()) {
            प्रत्यागम ("%Error-internal-contents-bad-ct" + cvt_गन्तव्यम्_Str(m_id) + "-ln" + cvt_गन्तव्यम्_Str(lineno));
        } अन्यथा {
            प्रत्यागम "";
        }
    }
    string text = m_पङ्क्तयः[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    प्रत्यागम text;
}

std::ostream& संक्रियकः<<(std::ostream& os, VFileContent* contentp) {
    यदि (!contentp) {
        os << "ct0";
    } अन्यथा {
        os << contentp->ascii();
    }
    प्रत्यागम os;
}

// ######################################################################
//  FileLine class functions

File_पङ्क्तिः::~File_पङ्क्तिः() {
    यदि (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

शून्य File_पङ्क्तिः::newContent() {
    यदि (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = नूतन VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_पङ्क्तिः::पङ्क्तिः_Directive_Strg(पूर्णाङ्क enterExit) ध्रुव {
    प्रत्यागम "`line "s + cvt_गन्तव्यम्_Str(चरमः_Lineno()) + " \""
           + V3_बहिर्गमनम्_Formatter::quoteNameControls(filename()) + "\" " + cvt_गन्तव्यम्_Str(enterExit) + '\n';
}

शून्य File_पङ्क्तिः::पङ्क्तिः_Directive(ध्रुव वर्ण* textp, पूर्णाङ्क& enterExitRef) {
    string newFilename;
    पूर्णाङ्क newLineno = -1;
    पङ्क्तिः_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    यदि (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } अन्यथा {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

शून्य File_पङ्क्तिः::पङ्क्तिः_Directive_Parse(ध्रुव वर्ण* textp, string& filenameRef, पूर्णाङ्क& linenoRef,
                                  पूर्णाङ्क& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    कुरु {
        // Skip `line
        यावत् (*textp && std::रिक्तम्_वा(*textp)) ++textp;
        यावत् (*textp && !std::रिक्तम्_वा(*textp)) ++textp;
        यावत् (*textp && std::रिक्तम्_वा(*textp)) ++textp;

        // Grab linenumber
        पूर्णाङ्क पङ्क्तिः_No;
        ध्रुव वर्ण* ध्रुव ln = textp;
        यावत् (*textp && !std::रिक्तम्_वा(*textp)) ++textp;
        यदि (0 == वाक्ये_तुलय(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            पङ्क्तिः_No = lineno() + 1;
        } अन्यथा यदि (std::अङ्कः_वा(*ln)) {
            पङ्क्तिः_No = std::atoi(ln);
        } अन्यथा {
            भङ्ग;  // Fail
        }
        यावत् (*textp && (std::रिक्तम्_वा(*textp))) ++textp;

        // Grab filename
        यदि (*textp != '"') भङ्ग;  // Fail
        ध्रुव वर्ण* ध्रुव fn = ++textp;
        यावत् (*textp && *textp != '"') ++textp;
        यदि (*textp != '"') भङ्ग;  // Fail
        string err_सन्देशः;
        ध्रुव string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_सन्देशः);
        यदि (!err_सन्देशः.रिक्तानि()) एतत्->v3error(err_सन्देशः.c_str());
        ++textp;
        यावत् (*textp && std::रिक्तम्_वा(*textp)) ++textp;

        // Grab level
        यदि (!std::अङ्कः_वा(*textp)) भङ्ग;  // Fail
        ध्रुव पूर्णाङ्क level = std::atoi(textp);
        यदि (level < 0 || level >= 3) भङ्ग;  // Fail

        linenoRef = पङ्क्तिः_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        प्रत्यागम;
    } यावत् (असत्य);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

शून्य File_पङ्क्तिः::forwardToken(ध्रुव वर्ण* textp, आकार_t आकार, तार्किक track_पङ्क्तयः) {
    स्थिर पूर्णाङ्क s_token_अङ्कः = 1;
    m_token_अङ्कः = s_token_अङ्कः++;
    प्रति (ध्रुव वर्ण* sp = textp; आकार && *sp; ++sp, --आकार) {
        यदि (*sp == '\n') {
            यदि (track_पङ्क्तयः) linenoInc();
            m_चरमः_Column = 1;
        } अन्यथा यदि (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } अन्यथा {  // Tabs are considered one column; hence column means number of chars
            ++m_चरमः_Column;
        }
    }
}

शून्य File_पङ्क्तिः::applyIgnores() {
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(एतत्);  // Toggle warnings based on global config file
#यदि_अन्तः
}

File_पङ्क्तिः* File_पङ्क्तिः::copy_Or_Same_File_पङ्क्तिः_Applied() {
    applyIgnores();
    प्रत्यागम copy_Or_Same_File_पङ्क्तिः();
}

File_पङ्क्तिः* File_पङ्क्तिः::copy_Or_Same_File_पङ्क्तिः() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    स्थिर File_पङ्क्तिः* s_चरमः_Newp = शून्यसूचकः;
    यदि (s_चरमः_Newp && *s_चरमः_Newp == *एतत्) {  // Compares lineno, filename, etc
        प्रत्यागम s_चरमः_Newp;
    }
    File_पङ्क्तिः* ध्रुव newp = नूतन File_पङ्क्तिः{एतत्};
    s_चरमः_Newp = newp;
    प्रत्यागम newp;
}

string File_पङ्क्तिः::filebasename() ध्रुव VL_MT_SAFE { प्रत्यागम V3Os::filenameNonDir(filename()); }

string File_पङ्क्तिः::filebasenameNoExt() ध्रुव { प्रत्यागम V3Os::filenameNonDirExt(filename()); }

string File_पङ्क्तिः::firstColumnLetters() ध्रुव VL_MT_SAFE {
    ध्रुव वर्ण a = ((firstColumn() / 26) % 26) + 'a';
    ध्रुव वर्ण b = (firstColumn() % 26) + 'a';
    प्रत्यागम string(1, a) + string(1, b);
}

string File_पङ्क्तिः::profileFuncname() ध्रुव {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::आकार_type pos;
    यावत् ((pos = name.find_first_not_सम्बन्धि(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_गन्तव्यम्_Str(चरमः_Lineno());
    प्रत्यागम name;
}

string File_पङ्क्तिः::ascii_पङ्क्तिः_Col() ध्रुव {
    प्रत्यागम (cvt_गन्तव्यम्_Str(firstLineno()) + "-" + cvt_गन्तव्यम्_Str(चरमः_Lineno()) + ":" + cvt_गन्तव्यम्_Str(firstColumn())
            + "-" + cvt_गन्तव्यम्_Str(चरमः_Column()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_गन्तव्यम्_Str(m_contentLineno) + "]");
}
string File_पङ्क्तिः::ascii() ध्रुव {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    प्रत्यागम filename() + ":" + cvt_गन्तव्यम्_Str(चरमः_Lineno()) + ":" + cvt_गन्तव्यम्_Str(firstColumn());
}
std::ostream& संक्रियकः<<(std::ostream& os, File_पङ्क्तिः* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    प्रत्यागम (os);
}

string File_पङ्क्तिः::warnOffParse(ध्रुव string& msgs, तार्किक turnOff) {
    string परिणाम;
    प्रति (ध्रुव string& सन्देशः_2 : VString::विभज(msgs, ',')) {
        ध्रुव V3_Error_संहिता संहिता{सन्देशः_2};
        यदि (!संहिता.hardError()) {
            warnOff(संहिता, turnOff);
            अनुवर्त;
        }
        // Error if not suppressed
        यदि (!v3Global.opt.isFuture(सन्देशः_2)) परिणाम = VString::dot(परिणाम, ",", सन्देशः_2);
    }
    प्रत्यागम परिणाम;
}

तार्किक File_पङ्क्तिः::warnIsOff(V3_Error_संहिता संहिता) ध्रुव {
    यदि (!सन्देशः_En().enabled(संहिता)) प्रत्यागम सत्य;
    यदि (!default_File_पङ्क्तिः().सन्देशः_En().enabled(संहिता)) प्रत्यागम सत्य;  // Global overrides local
    प्रत्यागम असत्य;
}

// cppverilator-suppress constParameter
शून्य File_पङ्क्तिः::v3error_अन्तः(std::ostringstream& sstr, ध्रुव string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    यदि (चरमः_Lineno()) nsstr << V3Error::warnContextBegin() << एतत् << V3Error::warn_Context_अन्तः();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    यदि (!extra.रिक्तानि()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().दीर्घता_2()) << " " << ": "
                << V3Error::warn_Context_अन्तः() << extra;
    }
    यदि (warnIsOff(V3Error::s().error_संहिता())) {
        V3Error::s().suppressThisWarning();
    } अन्यथा {
        यदि (!V3Error::s().errorContexted()) {
            ध्रुव string सङ्कलय = warnContextPrimary();
            wsstr << सङ्कलय;
            nsstr << सङ्कलय;
        }
        ध्रुव string waiverText = V3Error::stripMetaText(wsstr.str(), असत्य);
        m_waive = V3Control::waive(एतत्, V3Error::s().error_संहिता(), waiverText);
        यदि (m_waive) {
            V3Error::s().suppressThisWarning();
        } अन्यथा {
            V3Waiver::सङ्कलय_प्रविष्टिः(V3Error::s().error_संहिता(), filename(), waiverText);
        }
    }
    V3Error::v3error_अन्तः(nsstr, extrass.str(), एतत्);
}

string File_पङ्क्तिः::warn_अधिकम्() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) {
    यदि (चरमः_Lineno()) {
        प्रत्यागम V3Error::warnContextBegin() + V3Error::warn_अधिकम्() + string(ascii().आकार(), ' ')
               + ": " + V3Error::warn_Context_अन्तः();
    } अन्यथा {
        प्रत्यागम V3Error::warn_अधिकम्();
    }
}
string File_पङ्क्तिः::warnOther() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) {
    यदि (चरमः_Lineno()) {
        प्रत्यागम V3Error::s().warnRelated(एतत्) + V3Error::warnContextBegin() + V3Error::warn_अधिकम्()
               + ascii() + ": " + V3Error::warn_Context_अन्तः();
    } अन्यथा {
        प्रत्यागम V3Error::warn_अधिकम्();
    }
};

string File_पङ्क्तिः::source() ध्रुव VL_MT_SAFE {
    यदि (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        यदि (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            प्रत्यागम "\n%Error: internal tracking of file contents failed";
        } अन्यथा {
            प्रत्यागम "";
        }
    }  // LCOV_EXCL_STOP
    प्रत्यागम m_contentp->get_पङ्क्तिः(m_contentLineno);
}
string File_पङ्क्तिः::source_उपसर्गः(पूर्णाङ्क गन्तव्यम्_Column) ध्रुव VL_MT_SAFE {
    ध्रुव std::string src = source();
    यदि (गन्तव्यम्_Column > static_cast<पूर्णाङ्क>(src.दीर्घता_2())) गन्तव्यम्_Column = static_cast<पूर्णाङ्क>(src.दीर्घता_2());
    यदि (गन्तव्यम्_Column < 1) प्रत्यागम "";
    प्रत्यागम src.substr(0, गन्तव्यम्_Column - 1);
}
string File_पङ्क्तिः::prettySource() ध्रुव VL_MT_SAFE {
    string बहिर्गमनम् = source();
    // Drop ignore trailing newline
    ध्रुव string::आकार_type pos = बहिर्गमनम्.find('\n');
    यदि (pos != string::npos) बहिर्गमनम् = string(बहिर्गमनम्, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    प्रत्यागम VString::spaceUnprintable(बहिर्गमनम्);
}

string File_पङ्क्तिः::warnContext() ध्रुव {
    यदि (!v3Global.opt.context()) प्रत्यागम "";
    यदि (firstLineno() == चरमः_Lineno() && firstColumn()) {
        ध्रुव string source_पङ्क्तिः = prettySource();
        string बहिर्गमनम्;
        // Don't show super-long lines as can fill screen and unlikely to help user
        यदि (!source_पङ्क्तिः.रिक्तानि() && source_पङ्क्तिः.दीर्घता_2() < SHOW_SOURCE_MAX_दीर्घता
            && source_पङ्क्तिः.दीर्घता_2() >= static_cast<आकार_t>(चरमः_Column() - 1)) {
            string linestr = cvt_गन्तव्यम्_Str(firstLineno());
            यावत् (linestr.आकार() < 5) linestr = ' ' + linestr;
            बहिर्गमनम् += linestr + " | " + source_पङ्क्तिः + '\n';
            बहिर्गमनम् += std::string(linestr.आकार(), ' ') + " | ";
            बहिर्गमनम् += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            यदि (चरमः_Column() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                बहिर्गमनम् += string((चरमः_Column() - firstColumn() - 1), '~');
            }
            बहिर्गमनम् += '\n';
        }
        प्रत्यागम V3Error::warnContextBegin() + बहिर्गमनम् + V3Error::warn_Context_अन्तः();
    }
    प्रत्यागम "";
}

string File_पङ्क्तिः::warnContextParent() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) {
    string परिणाम;
    प्रति (File_पङ्क्तिः* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        यदि (parentFl->filenameIsGlobal()) भङ्ग;
        परिणाम += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    प्रत्यागम परिणाम;
}

#यदि_परिभाषितम् VL_LEAK_CHECKS
std::unordered_समुच्चयः<File_पङ्क्तिः*> file_पङ्क्तिः_Leak_Checks;

शून्य* File_पङ्क्तिः::संक्रियकः नूतन(आकार_t आकार) {
    File_पङ्क्तिः* ध्रुव objp = static_cast<File_पङ्क्तिः*>(::संक्रियकः नूतन(आकार));
    file_पङ्क्तिः_Leak_Checks.निवेशय(objp);
    प्रत्यागम objp;
}

शून्य File_पङ्क्तिः::संक्रियकः विलोपय(शून्य* objp, आकार_t आकार) {
    यदि (!objp) प्रत्यागम;
    File_पङ्क्तिः* ध्रुव flp = static_cast<File_पङ्क्तिः*>(objp);
    ध्रुव स्वचालित it = file_पङ्क्तिः_Leak_Checks.find(flp);
    यदि (it != file_पङ्क्तिः_Leak_Checks.अन्तः_2()) {
        file_पङ्क्तिः_Leak_Checks.erase(it);
    } अन्यथा {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::संक्रियकः विलोपय(objp);
}
#यदि_अन्तः

शून्य File_पङ्क्तिः::stats() {
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
    V3Stats::सङ्कलय_Stat_योग("FileLines, Number of filenames",
                        singleton().m_नामानि.आकार());  // Max m_filenameno
    V3Stats::सङ्कलय_Stat_योग("FileLines, Message enable sets",
                        singleton().m_interned_सन्देशः_Ens.आकार());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#यदि_अन्तः
}

शून्य File_पङ्क्तिः::deleteAllRemaining() {
#यदि_परिभाषितम् VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    यावत् (सत्य) {
        ध्रुव स्वचालित it = file_पङ्क्तिः_Leak_Checks.begin();
        यदि (it == file_पङ्क्तिः_Leak_Checks.अन्तः_2()) भङ्ग;
        विलोपय *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_पङ्क्तिः_Leak_Checks.clear();
    singleton().clear();
#यदि_अन्तः
}
