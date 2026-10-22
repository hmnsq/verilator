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

string File_ओळ_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr पूर्णांक size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    अक्षर out[size];
    अक्षर* op = out + size - 1;
    *--op = '\0';  // We build backwards
    पूर्णांक अंक = fileno;
    करा {
        *--op = 'a' + अंक % 26;
        अंक /= 26;
    } पर्यंत (अंक);
    परत op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_ओळ_Singleton::fileNameIdx_t File_ओळ_Singleton::name_To_संख्या(स्थिरांक string& filename) {
    स्थिरांक स्वतः pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    जर (pair.second) {
        स्थिरांक माप_प्रकार nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.ढकला_back(filename);
        m_languages.ढकला_back(V3LangCode::mostRecent());
    }
    परत idx;
}

शून्य File_ओळ_Singleton::file_Name_अंक_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    साठी (स्थिरांक स्वतः& itr : m_namemap) {
        std::string filename = itr.first;
        जर (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } अन्यथा जर (filename == V3Options::getStdWaiverPath()) {
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

File_ओळ_Singleton::संदेश_En_Set_Idx_t File_ओळ_Singleton::add_संदेश_En_Bit_Set(स्थिरांक संदेश_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    स्थिरांक V3LockGuard lock{m_mutex};
    स्थिरांक स्वतः pair = m_interned_संदेश_En_Idxs.emplace(bitSet, 0);
    संदेश_En_Set_Idx_t& idx = pair.first->second;
    जर (pair.second) {
        स्थिरांक माप_प्रकार nextIdx = m_interned_संदेश_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<संदेश_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<संदेश_En_Set_Idx_t>(nextIdx);
        m_interned_संदेश_Ens.ढकला_back(bitSet);
    }
    परत idx;
}

File_ओळ_Singleton::संदेश_En_Set_Idx_t File_ओळ_Singleton::default_संदेश_En_Index() VL_MT_SAFE {
    संदेश_En_Bit_Set संदेश_En_Bit_Set_2;
    साठी (पूर्णांक i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        स्थिरांक V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        संदेश_En_Bit_Set_2.set(संदेश_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        संदेश_En_Bit_Set_2.set(संदेश_En_Bit_Set::Subset::CTRL, code, सत्य);
    }
    परत add_संदेश_En_Bit_Set(संदेश_En_Bit_Set_2);
}

File_ओळ_Singleton::संदेश_En_Set_Idx_t File_ओळ_Singleton::संदेश_En_Set_Bit(संदेश_En_Set_Idx_t setIdx,
                                                                संदेश_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, तार्किक मूल्य) {
    // See if state matches existing
    तार्किक same = सत्य;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        जर (संदेश_En(setIdx).test(subset, subcode) != मूल्य) same = असत्य;
    });
    जर (same) परत setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    संदेश_En_Bit_Set संदेश_En_Bit_Set_2{संदेश_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { संदेश_En_Bit_Set_2.set(subset, subcode, मूल्य); });
    परत add_संदेश_En_Bit_Set(संदेश_En_Bit_Set_2);
}

File_ओळ_Singleton::संदेश_En_Set_Idx_t File_ओळ_Singleton::संदेश_Set_Ctrl_Bit_Set(संदेश_En_Set_Idx_t setIdx,
                                                                     स्थिरांक VErrorBitSet& bitset) {
    स्थिरांक संदेश_En_Bit_Set::Subset subset = संदेश_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    जर (संदेश_En(setIdx).getAll(subset) == bitset) परत setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    संदेश_En_Bit_Set संदेश_En_Bit_Set_2{संदेश_En(setIdx)};
    संदेश_En_Bit_Set_2.setAll(subset, bitset);
    परत add_संदेश_En_Bit_Set(संदेश_En_Bit_Set_2);
}

File_ओळ_Singleton::संदेश_En_Set_Idx_t File_ओळ_Singleton::संदेश_En_And(संदेश_En_Set_Idx_t lhsIdx,
                                                             संदेश_En_Set_Idx_t rhsIdx) {
    स्थिरांक संदेश_En_Bit_Set& lhs = संदेश_En(lhsIdx);
    स्थिरांक संदेश_En_Bit_Set& rhs = संदेश_En(rhsIdx);
    स्थिरांक संदेश_En_Bit_Set intersection{lhs, rhs};
    जर (intersection == lhs) परत lhsIdx;
    जर (intersection == rhs) परत rhsIdx;
    परत add_संदेश_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

शून्य VFileContent::ढकला_Text(स्थिरांक string& text) {
    // Similar code in WildcardContents::pushText()
    जर (m_ओळी.size() == 0) {
        m_ओळी.emplace_back("");  // no such thing as line [0]
        m_ओळी.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    स्थिरांक string leftover = m_ओळी.back() + text;
    m_ओळी.काढा_back();

    // Insert line-by-line
    string::size_type ओळ_start = 0;
    पर्यंत (सत्य) {
        स्थिरांक string::size_type ओळ_end = leftover.find('\n', ओळ_start);
        जर (ओळ_end != string::npos) {
            स्थिरांक string oneline(leftover, ओळ_start, ओळ_end - ओळ_start + 1);
            m_ओळी.ढकला_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_ओळी.size() - 1) << "]: " << oneline);
            ओळ_start = ओळ_end + 1;
        } अन्यथा {
            थांबा;
        }
    }
    // Keep leftover for next time
    m_ओळी.emplace_back(string(leftover, ओळ_start));  // Might be ""
}

string VFileContent::get_ओळ(पूर्णांक lineno) स्थिरांक VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    जर (VL_UNCOVERABLE(lineno < 0 || lineno >= (पूर्णांक)m_ओळी.size())) {
        जर (debug() || v3Global.opt.debugCheck()) {
            परत ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } अन्यथा {
            परत "";
        }
    }
    string text = m_ओळी[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    परत text;
}

std::ostream& चालक<<(std::ostream& os, VFileContent* contentp) {
    जर (!contentp) {
        os << "ct0";
    } अन्यथा {
        os << contentp->ascii();
    }
    परत os;
}

// ######################################################################
//  FileLine class functions

File_ओळ::~File_ओळ() {
    जर (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

शून्य File_ओळ::newContent() {
    जर (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = नवीन VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_ओळ::ओळ_Directive_Strg(पूर्णांक enterExit) स्थिरांक {
    परत "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

शून्य File_ओळ::ओळ_Directive(स्थिरांक अक्षर* textp, पूर्णांक& enterExitRef) {
    string newFilename;
    पूर्णांक newLineno = -1;
    ओळ_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    जर (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } अन्यथा {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

शून्य File_ओळ::ओळ_Directive_Parse(स्थिरांक अक्षर* textp, string& filenameRef, पूर्णांक& linenoRef,
                                  पूर्णांक& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    करा {
        // Skip `line
        पर्यंत (*textp && std::है_खाली(*textp)) ++textp;
        पर्यंत (*textp && !std::है_खाली(*textp)) ++textp;
        पर्यंत (*textp && std::है_खाली(*textp)) ++textp;

        // Grab linenumber
        पूर्णांक ओळ_No;
        स्थिरांक अक्षर* स्थिरांक ln = textp;
        पर्यंत (*textp && !std::है_खाली(*textp)) ++textp;
        जर (0 == म_भेदन(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            ओळ_No = lineno() + 1;
        } अन्यथा जर (std::है_अंक(*ln)) {
            ओळ_No = std::म_से_प(ln);
        } अन्यथा {
            थांबा;  // Fail
        }
        पर्यंत (*textp && (std::है_खाली(*textp))) ++textp;

        // Grab filename
        जर (*textp != '"') थांबा;  // Fail
        स्थिरांक अक्षर* स्थिरांक fn = ++textp;
        पर्यंत (*textp && *textp != '"') ++textp;
        जर (*textp != '"') थांबा;  // Fail
        string err_संदेश;
        स्थिरांक string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_संदेश);
        जर (!err_संदेश.empty()) हे->v3error(err_संदेश.c_str());
        ++textp;
        पर्यंत (*textp && std::है_खाली(*textp)) ++textp;

        // Grab level
        जर (!std::है_अंक(*textp)) थांबा;  // Fail
        स्थिरांक पूर्णांक level = std::म_से_प(textp);
        जर (level < 0 || level >= 3) थांबा;  // Fail

        linenoRef = ओळ_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        परत;
    } पर्यंत (असत्य);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

शून्य File_ओळ::forwardToken(स्थिरांक अक्षर* textp, माप_प्रकार size, तार्किक track_ओळी) {
    स्थिर पूर्णांक s_token_अंक = 1;
    m_token_अंक = s_token_अंक++;
    साठी (स्थिरांक अक्षर* sp = textp; size && *sp; ++sp, --size) {
        जर (*sp == '\n') {
            जर (track_ओळी) linenoInc();
            m_lastColumn = 1;
        } अन्यथा जर (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } अन्यथा {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

शून्य File_ओळ::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(हे);  // Toggle warnings based on global config file
#endif
}

File_ओळ* File_ओळ::copy_Or_Same_File_ओळ_Applied() {
    applyIgnores();
    परत copy_Or_Same_File_ओळ();
}

File_ओळ* File_ओळ::copy_Or_Same_File_ओळ() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    स्थिर File_ओळ* s_lastNewp = nullptr;
    जर (s_lastNewp && *s_lastNewp == *हे) {  // Compares lineno, filename, etc
        परत s_lastNewp;
    }
    File_ओळ* स्थिरांक newp = नवीन File_ओळ{हे};
    s_lastNewp = newp;
    परत newp;
}

string File_ओळ::filebasename() स्थिरांक VL_MT_SAFE { परत V3Os::filenameNonDir(filename()); }

string File_ओळ::filebasenameNoExt() स्थिरांक { परत V3Os::filenameNonDirExt(filename()); }

string File_ओळ::firstColumnLetters() स्थिरांक VL_MT_SAFE {
    स्थिरांक अक्षर a = ((firstColumn() / 26) % 26) + 'a';
    स्थिरांक अक्षर b = (firstColumn() % 26) + 'a';
    परत string(1, a) + string(1, b);
}

string File_ओळ::profileFuncname() स्थिरांक {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    पर्यंत ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    परत name;
}

string File_ओळ::ascii_ओळ_Col() स्थिरांक {
    परत (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_ओळ::ascii() स्थिरांक {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    परत filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& चालक<<(std::ostream& os, File_ओळ* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    परत (os);
}

string File_ओळ::warnOffParse(स्थिरांक string& msgs, तार्किक turnOff) {
    string result;
    साठी (स्थिरांक string& संदेश : VString::split(msgs, ',')) {
        स्थिरांक V3ErrorCode code{संदेश};
        जर (!code.hardError()) {
            warnOff(code, turnOff);
            सुरूठेवा;
        }
        // Error if not suppressed
        जर (!v3Global.opt.isFuture(संदेश)) result = VString::dot(result, ",", संदेश);
    }
    परत result;
}

तार्किक File_ओळ::warnIsOff(V3ErrorCode code) स्थिरांक {
    जर (!संदेश_En().enabled(code)) परत सत्य;
    जर (!default_File_ओळ().संदेश_En().enabled(code)) परत सत्य;  // Global overrides local
    परत असत्य;
}

// cppverilator-suppress constParameter
शून्य File_ओळ::v3errorEnd(std::ostringstream& sstr, स्थिरांक string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    जर (lastLineno()) nsstr << V3Error::warnContextBegin() << हे << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    जर (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    जर (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } अन्यथा {
        जर (!V3Error::s().errorContexted()) {
            स्थिरांक string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        स्थिरांक string waiverText = V3Error::stripMetaText(wsstr.str(), असत्य);
        m_waive = V3Control::waive(हे, V3Error::s().errorCode(), waiverText);
        जर (m_waive) {
            V3Error::s().suppressThisWarning();
        } अन्यथा {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), हे);
}

string File_ओळ::warnMore() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex) {
    जर (lastLineno()) {
        परत V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } अन्यथा {
        परत V3Error::warnMore();
    }
}
string File_ओळ::warnOther() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex) {
    जर (lastLineno()) {
        परत V3Error::s().warnRelated(हे) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } अन्यथा {
        परत V3Error::warnMore();
    }
};

string File_ओळ::source() स्थिरांक VL_MT_SAFE {
    जर (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        जर (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            परत "\n%Error: internal tracking of file contents failed";
        } अन्यथा {
            परत "";
        }
    }  // LCOV_EXCL_STOP
    परत m_contentp->get_ओळ(m_contentLineno);
}
string File_ओळ::sourcePrefix(पूर्णांक toColumn) स्थिरांक VL_MT_SAFE {
    स्थिरांक std::string src = source();
    जर (toColumn > static_cast<पूर्णांक>(src.length())) toColumn = static_cast<पूर्णांक>(src.length());
    जर (toColumn < 1) परत "";
    परत src.substr(0, toColumn - 1);
}
string File_ओळ::prettySource() स्थिरांक VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    स्थिरांक string::size_type pos = out.find('\n');
    जर (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    परत VString::spaceUnprintable(out);
}

string File_ओळ::warnContext() स्थिरांक {
    जर (!v3Global.opt.context()) परत "";
    जर (firstLineno() == lastLineno() && firstColumn()) {
        स्थिरांक string source_ओळ = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        जर (!source_ओळ.empty() && source_ओळ.length() < SHOW_SOURCE_MAX_LENGTH
            && source_ओळ.length() >= static_cast<माप_प्रकार>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            पर्यंत (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_ओळ + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            जर (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        परत V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    परत "";
}

string File_ओळ::warnContextParent() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    साठी (File_ओळ* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        जर (parentFl->filenameIsGlobal()) थांबा;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    परत result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_ओळ*> file_ओळ_Leak_Checks;

शून्य* File_ओळ::चालक नवीन(माप_प्रकार size) {
    File_ओळ* स्थिरांक objp = static_cast<File_ओळ*>(::चालक नवीन(size));
    file_ओळ_Leak_Checks.घाला(objp);
    परत objp;
}

शून्य File_ओळ::चालक delete(शून्य* objp, माप_प्रकार size) {
    जर (!objp) परत;
    File_ओळ* स्थिरांक flp = static_cast<File_ओळ*>(objp);
    स्थिरांक स्वतः it = file_ओळ_Leak_Checks.find(flp);
    जर (it != file_ओळ_Leak_Checks.end()) {
        file_ओळ_Leak_Checks.erase(it);
    } अन्यथा {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::चालक delete(objp);
}
#endif

शून्य File_ओळ::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_संदेश_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

शून्य File_ओळ::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    पर्यंत (सत्य) {
        स्थिरांक स्वतः it = file_ओळ_Leak_Checks.begin();
        जर (it == file_ओळ_Leak_Checks.end()) थांबा;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_ओळ_Leak_Checks.clear();
    singleton().clear();
#endif
}
