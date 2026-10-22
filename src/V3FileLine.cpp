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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

// clang-format off
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3String.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#endif
#அந்தர்பாவ "V3Waiver.h"
// clang-format on

#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <limits>
#அந்தர்பாவ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_வரி_Singleton::filenameLetters(fileNameIdx_t fileno) VL_PURE {
    constexpr முழு size
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    எழுத்து out[size];
    எழுத்து* op = out + size - 1;
    *--op = '\0';  // We build backwards
    முழு இலக்கம் = fileno;
    செய் {
        *--op = 'a' + இலக்கம் % 26;
        இலக்கம் /= 26;
    } போது (இலக்கம்);
    திரும்பு op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_வரி_Singleton::fileNameIdx_t File_வரி_Singleton::name_To_எண்(மாறிலி string& filename) {
    மாறிலி auto pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.first->second;
    எனில் (pair.second) {
        மாறிலி size_t nextIdx = m_names.size();
        UASSERT(nextIdx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<fileNameIdx_t>(nextIdx);
        m_names.தள்ளு_back(filename);
        m_languages.தள்ளு_back(V3LangCode::mostRecent());
    }
    திரும்பு idx;
}

வெறுமை File_வரி_Singleton::file_Name_இலக்கம்_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    க்கு (மாறிலி auto& itr : m_namemap) {
        std::string filename = itr.first;
        எனில் (filename == V3Options::getStdPackagePath()) {
            filename = "<verilated_std>";
        } இல்லையெனில் எனில் (filename == V3Options::getStdWaiverPath()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.second) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3OutFormatter::quoteNameControls(V3Os::filenameRealPath(itr.first)) << '"'
           << ", \"language\":\"" << எண்_To_Lang(itr.second).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_வரி_Singleton::செய்தி_En_Set_Idx_t File_வரி_Singleton::add_செய்தி_En_Bit_Set(மாறிலி செய்தி_En_Bit_Set& bitSet)
    VL_MT_SAFE_EXCLUDES(m_mutex) {
    மாறிலி V3LockGuard lock{m_mutex};
    மாறிலி auto pair = m_interned_செய்தி_En_Idxs.emplace(bitSet, 0);
    செய்தி_En_Set_Idx_t& idx = pair.first->second;
    எனில் (pair.second) {
        மாறிலி size_t nextIdx = m_interned_செய்தி_Ens.size();
        UASSERT(nextIdx <= std::numeric_limits<செய்தி_En_Set_Idx_t>::max(),
                "Too many unique message enable sets (" + cvtToStr(nextIdx) + "+).");
        idx = static_cast<செய்தி_En_Set_Idx_t>(nextIdx);
        m_interned_செய்தி_Ens.தள்ளு_back(bitSet);
    }
    திரும்பு idx;
}

File_வரி_Singleton::செய்தி_En_Set_Idx_t File_வரி_Singleton::default_செய்தி_En_Index() VL_MT_SAFE {
    செய்தி_En_Bit_Set செய்தி_En_Bit_Set_2;
    க்கு (முழு i = V3ErrorCode::EC_MIN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        மாறிலி V3ErrorCode code{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        செய்தி_En_Bit_Set_2.set(செய்தி_En_Bit_Set::Subset::CODE, code, !code.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        செய்தி_En_Bit_Set_2.set(செய்தி_En_Bit_Set::Subset::CTRL, code, உண்மை);
    }
    திரும்பு add_செய்தி_En_Bit_Set(செய்தி_En_Bit_Set_2);
}

File_வரி_Singleton::செய்தி_En_Set_Idx_t File_வரி_Singleton::செய்தி_En_Set_Bit(செய்தி_En_Set_Idx_t setIdx,
                                                                செய்தி_En_Bit_Set::Subset subset,
                                                                V3ErrorCode code, தருக்க மதிப்பு) {
    // See if state matches existing
    தருக்க same = உண்மை;
    code.forDelegateCodes([&](V3ErrorCode subcode) {
        எனில் (செய்தி_En(setIdx).test(subset, subcode) != மதிப்பு) same = பொய்;
    });
    எனில் (same) திரும்பு setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    செய்தி_En_Bit_Set செய்தி_En_Bit_Set_2{செய்தி_En(setIdx)};
    code.forDelegateCodes([&](V3ErrorCode subcode) { செய்தி_En_Bit_Set_2.set(subset, subcode, மதிப்பு); });
    திரும்பு add_செய்தி_En_Bit_Set(செய்தி_En_Bit_Set_2);
}

File_வரி_Singleton::செய்தி_En_Set_Idx_t File_வரி_Singleton::செய்தி_Set_Ctrl_Bit_Set(செய்தி_En_Set_Idx_t setIdx,
                                                                     மாறிலி VErrorBitSet& bitset) {
    மாறிலி செய்தி_En_Bit_Set::Subset subset = செய்தி_En_Bit_Set::Subset::CTRL;
    // See if state matches existing
    எனில் (செய்தி_En(setIdx).getAll(subset) == bitset) திரும்பு setIdx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    செய்தி_En_Bit_Set செய்தி_En_Bit_Set_2{செய்தி_En(setIdx)};
    செய்தி_En_Bit_Set_2.setAll(subset, bitset);
    திரும்பு add_செய்தி_En_Bit_Set(செய்தி_En_Bit_Set_2);
}

File_வரி_Singleton::செய்தி_En_Set_Idx_t File_வரி_Singleton::செய்தி_En_And(செய்தி_En_Set_Idx_t lhsIdx,
                                                             செய்தி_En_Set_Idx_t rhsIdx) {
    மாறிலி செய்தி_En_Bit_Set& lhs = செய்தி_En(lhsIdx);
    மாறிலி செய்தி_En_Bit_Set& rhs = செய்தி_En(rhsIdx);
    மாறிலி செய்தி_En_Bit_Set intersection{lhs, rhs};
    எனில் (intersection == lhs) திரும்பு lhsIdx;
    எனில் (intersection == rhs) திரும்பு rhsIdx;
    திரும்பு add_செய்தி_En_Bit_Set(intersection);
}

// ######################################################################
//  VFileContents class functions

வெறுமை VFileContent::தள்ளு_Text(மாறிலி string& text) {
    // Similar code in WildcardContents::pushText()
    எனில் (m_வரிகள்.size() == 0) {
        m_வரிகள்.emplace_back("");  // no such thing as line [0]
        m_வரிகள்.emplace_back("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    மாறிலி string leftover = m_வரிகள்.back() + text;
    m_வரிகள்.எடு_back();

    // Insert line-by-line
    string::size_type வரி_start = 0;
    போது (உண்மை) {
        மாறிலி string::size_type வரி_end = leftover.find('\n', வரி_start);
        எனில் (வரி_end != string::npos) {
            மாறிலி string oneline(leftover, வரி_start, வரி_end - வரி_start + 1);
            m_வரிகள்.தள்ளு_back(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_வரிகள்.size() - 1) << "]: " << oneline);
            வரி_start = வரி_end + 1;
        } இல்லையெனில் {
            நிறுத்து;
        }
    }
    // Keep leftover for next time
    m_வரிகள்.emplace_back(string(leftover, வரி_start));  // Might be ""
}

string VFileContent::get_வரி(முழு lineno) மாறிலி VL_MT_SAFE {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    எனில் (VL_UNCOVERABLE(lineno < 0 || lineno >= (முழு)m_வரிகள்.size())) {
        எனில் (debug() || v3Global.opt.debugCheck()) {
            திரும்பு ("%Error-internal-contents-bad-ct" + cvtToStr(m_id) + "-ln" + cvtToStr(lineno));
        } இல்லையெனில் {
            திரும்பு "";
        }
    }
    string text = m_வரிகள்[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << text);
    திரும்பு text;
}

std::ostream& operator<<(std::ostream& os, VFileContent* contentp) {
    எனில் (!contentp) {
        os << "ct0";
    } இல்லையெனில் {
        os << contentp->ascii();
    }
    திரும்பு os;
}

// ######################################################################
//  FileLine class functions

File_வரி::~File_வரி() {
    எனில் (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

வெறுமை File_வரி::newContent() {
    எனில் (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = புதிய VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_வரி::வரி_Directive_Strg(முழு enterExit) மாறிலி {
    திரும்பு "`line "s + cvtToStr(lastLineno()) + " \""
           + V3OutFormatter::quoteNameControls(filename()) + "\" " + cvtToStr(enterExit) + '\n';
}

வெறுமை File_வரி::வரி_Directive(மாறிலி எழுத்து* textp, முழு& enterExitRef) {
    string newFilename;
    முழு newLineno = -1;
    வரி_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    எனில் (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } இல்லையெனில் {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

வெறுமை File_வரி::வரி_Directive_Parse(மாறிலி எழுத்து* textp, string& filenameRef, முழு& linenoRef,
                                  முழு& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    செய் {
        // Skip `line
        போது (*textp && std::isspace(*textp)) ++textp;
        போது (*textp && !std::isspace(*textp)) ++textp;
        போது (*textp && std::isspace(*textp)) ++textp;

        // Grab linenumber
        முழு வரி_No;
        மாறிலி எழுத்து* மாறிலி ln = textp;
        போது (*textp && !std::isspace(*textp)) ++textp;
        எனில் (0 == strncmp(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            வரி_No = lineno() + 1;
        } இல்லையெனில் எனில் (std::isdigit(*ln)) {
            வரி_No = std::atoi(ln);
        } இல்லையெனில் {
            நிறுத்து;  // Fail
        }
        போது (*textp && (std::isspace(*textp))) ++textp;

        // Grab filename
        எனில் (*textp != '"') நிறுத்து;  // Fail
        மாறிலி எழுத்து* மாறிலி fn = ++textp;
        போது (*textp && *textp != '"') ++textp;
        எனில் (*textp != '"') நிறுத்து;  // Fail
        string err_செய்தி;
        மாறிலி string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_செய்தி);
        எனில் (!err_செய்தி.empty()) இது->v3error(err_செய்தி.c_str());
        ++textp;
        போது (*textp && std::isspace(*textp)) ++textp;

        // Grab level
        எனில் (!std::isdigit(*textp)) நிறுத்து;  // Fail
        மாறிலி முழு level = std::atoi(textp);
        எனில் (level < 0 || level >= 3) நிறுத்து;  // Fail

        linenoRef = வரி_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        திரும்பு;
    } போது (பொய்);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

வெறுமை File_வரி::forwardToken(மாறிலி எழுத்து* textp, size_t size, தருக்க track_வரிகள்) {
    நிலையான முழு s_token_இலக்கம் = 1;
    m_token_இலக்கம் = s_token_இலக்கம்++;
    க்கு (மாறிலி எழுத்து* sp = textp; size && *sp; ++sp, --size) {
        எனில் (*sp == '\n') {
            எனில் (track_வரிகள்) linenoInc();
            m_lastColumn = 1;
        } இல்லையெனில் எனில் (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } இல்லையெனில் {  // Tabs are considered one column; hence column means number of chars
            ++m_lastColumn;
        }
    }
}

வெறுமை File_வரி::applyIgnores() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(இது);  // Toggle warnings based on global config file
#endif
}

File_வரி* File_வரி::copy_Or_Same_File_வரி_Applied() {
    applyIgnores();
    திரும்பு copy_Or_Same_File_வரி();
}

File_வரி* File_வரி::copy_Or_Same_File_வரி() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    நிலையான File_வரி* s_lastNewp = nullptr;
    எனில் (s_lastNewp && *s_lastNewp == *இது) {  // Compares lineno, filename, etc
        திரும்பு s_lastNewp;
    }
    File_வரி* மாறிலி newp = புதிய File_வரி{இது};
    s_lastNewp = newp;
    திரும்பு newp;
}

string File_வரி::filebasename() மாறிலி VL_MT_SAFE { திரும்பு V3Os::filenameNonDir(filename()); }

string File_வரி::filebasenameNoExt() மாறிலி { திரும்பு V3Os::filenameNonDirExt(filename()); }

string File_வரி::firstColumnLetters() மாறிலி VL_MT_SAFE {
    மாறிலி எழுத்து a = ((firstColumn() / 26) % 26) + 'a';
    மாறிலி எழுத்து b = (firstColumn() % 26) + 'a';
    திரும்பு string(1, a) + string(1, b);
}

string File_வரி::profileFuncname() மாறிலி {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::size_type pos;
    போது ((pos = name.find_first_not_of(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvtToStr(lastLineno());
    திரும்பு name;
}

string File_வரி::ascii_வரி_Col() மாறிலி {
    திரும்பு (cvtToStr(firstLineno()) + "-" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn())
            + "-" + cvtToStr(lastColumn()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvtToStr(m_contentLineno) + "]");
}
string File_வரி::ascii() மாறிலி {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    திரும்பு filename() + ":" + cvtToStr(lastLineno()) + ":" + cvtToStr(firstColumn());
}
std::ostream& operator<<(std::ostream& os, File_வரி* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    திரும்பு (os);
}

string File_வரி::warnOffParse(மாறிலி string& msgs, தருக்க turnOff) {
    string result;
    க்கு (மாறிலி string& செய்தி : VString::split(msgs, ',')) {
        மாறிலி V3ErrorCode code{செய்தி};
        எனில் (!code.hardError()) {
            warnOff(code, turnOff);
            தொடர்;
        }
        // Error if not suppressed
        எனில் (!v3Global.opt.isFuture(செய்தி)) result = VString::dot(result, ",", செய்தி);
    }
    திரும்பு result;
}

தருக்க File_வரி::warnIsOff(V3ErrorCode code) மாறிலி {
    எனில் (!செய்தி_En().enabled(code)) திரும்பு உண்மை;
    எனில் (!default_File_வரி().செய்தி_En().enabled(code)) திரும்பு உண்மை;  // Global overrides local
    திரும்பு பொய்;
}

// cppverilator-suppress constParameter
வெறுமை File_வரி::v3errorEnd(std::ostringstream& sstr, மாறிலி string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    எனில் (lastLineno()) nsstr << V3Error::warnContextBegin() << இது << V3Error::warnContextEnd();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    எனில் (!extra.empty()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().length()) << " " << ": "
                << V3Error::warnContextEnd() << extra;
    }
    எனில் (warnIsOff(V3Error::s().errorCode())) {
        V3Error::s().suppressThisWarning();
    } இல்லையெனில் {
        எனில் (!V3Error::s().errorContexted()) {
            மாறிலி string add = warnContextPrimary();
            wsstr << add;
            nsstr << add;
        }
        மாறிலி string waiverText = V3Error::stripMetaText(wsstr.str(), பொய்);
        m_waive = V3Control::waive(இது, V3Error::s().errorCode(), waiverText);
        எனில் (m_waive) {
            V3Error::s().suppressThisWarning();
        } இல்லையெனில் {
            V3Waiver::addEntry(V3Error::s().errorCode(), filename(), waiverText);
        }
    }
    V3Error::v3errorEnd(nsstr, extrass.str(), இது);
}

string File_வரி::warnMore() மாறிலி VL_REQUIRES(V3Error::s().m_mutex) {
    எனில் (lastLineno()) {
        திரும்பு V3Error::warnContextBegin() + V3Error::warnMore() + string(ascii().size(), ' ')
               + ": " + V3Error::warnContextEnd();
    } இல்லையெனில் {
        திரும்பு V3Error::warnMore();
    }
}
string File_வரி::warnOther() மாறிலி VL_REQUIRES(V3Error::s().m_mutex) {
    எனில் (lastLineno()) {
        திரும்பு V3Error::s().warnRelated(இது) + V3Error::warnContextBegin() + V3Error::warnMore()
               + ascii() + ": " + V3Error::warnContextEnd();
    } இல்லையெனில் {
        திரும்பு V3Error::warnMore();
    }
};

string File_வரி::source() மாறிலி VL_MT_SAFE {
    எனில் (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        எனில் (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            திரும்பு "\n%Error: internal tracking of file contents failed";
        } இல்லையெனில் {
            திரும்பு "";
        }
    }  // LCOV_EXCL_STOP
    திரும்பு m_contentp->get_வரி(m_contentLineno);
}
string File_வரி::sourcePrefix(முழு toColumn) மாறிலி VL_MT_SAFE {
    மாறிலி std::string src = source();
    எனில் (toColumn > static_cast<முழு>(src.length())) toColumn = static_cast<முழு>(src.length());
    எனில் (toColumn < 1) திரும்பு "";
    திரும்பு src.substr(0, toColumn - 1);
}
string File_வரி::prettySource() மாறிலி VL_MT_SAFE {
    string out = source();
    // Drop ignore trailing newline
    மாறிலி string::size_type pos = out.find('\n');
    எனில் (pos != string::npos) out = string(out, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    திரும்பு VString::spaceUnprintable(out);
}

string File_வரி::warnContext() மாறிலி {
    எனில் (!v3Global.opt.context()) திரும்பு "";
    எனில் (firstLineno() == lastLineno() && firstColumn()) {
        மாறிலி string source_வரி = prettySource();
        string out;
        // Don't show super-long lines as can fill screen and unlikely to help user
        எனில் (!source_வரி.empty() && source_வரி.length() < SHOW_SOURCE_MAX_LENGTH
            && source_வரி.length() >= static_cast<size_t>(lastColumn() - 1)) {
            string linestr = cvtToStr(firstLineno());
            போது (linestr.size() < 5) linestr = ' ' + linestr;
            out += linestr + " | " + source_வரி + '\n';
            out += std::string(linestr.size(), ' ') + " | ";
            out += string((firstColumn() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            எனில் (lastColumn() > firstColumn()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                out += string((lastColumn() - firstColumn() - 1), '~');
            }
            out += '\n';
        }
        திரும்பு V3Error::warnContextBegin() + out + V3Error::warnContextEnd();
    }
    திரும்பு "";
}

string File_வரி::warnContextParent() மாறிலி VL_REQUIRES(V3Error::s().m_mutex) {
    string result;
    க்கு (File_வரி* parentFl = parent(); parentFl; parentFl = parentFl->parent()) {
        எனில் (parentFl->filenameIsGlobal()) நிறுத்து;
        result += parentFl->warnOther() + "... note: In file included from '"
                  + parentFl->filebasename() + "'\n";
    }
    திரும்பு result;
}

#ifdef VL_LEAK_CHECKS
std::unordered_set<File_வரி*> file_வரி_Leak_Checks;

வெறுமை* File_வரி::operator புதிய(size_t size) {
    File_வரி* மாறிலி objp = static_cast<File_வரி*>(::operator புதிய(size));
    file_வரி_Leak_Checks.செருகு(objp);
    திரும்பு objp;
}

வெறுமை File_வரி::operator delete(வெறுமை* objp, size_t size) {
    எனில் (!objp) திரும்பு;
    File_வரி* மாறிலி flp = static_cast<File_வரி*>(objp);
    மாறிலி auto it = file_வரி_Leak_Checks.find(flp);
    எனில் (it != file_வரி_Leak_Checks.end()) {
        file_வரி_Leak_Checks.erase(it);
    } இல்லையெனில் {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::operator delete(objp);
}
#endif

வெறுமை File_வரி::stats() {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("FileLines, Number of filenames",
                        singleton().m_names.size());  // Max m_filenameno
    V3Stats::addStatSum("FileLines, Message enable sets",
                        singleton().m_interned_செய்தி_Ens.size());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#endif
}

வெறுமை File_வரி::deleteAllRemaining() {
#ifdef VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    போது (உண்மை) {
        மாறிலி auto it = file_வரி_Leak_Checks.begin();
        எனில் (it == file_வரி_Leak_Checks.end()) நிறுத்து;
        delete *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_வரி_Leak_Checks.clear();
    singleton().clear();
#endif
}
