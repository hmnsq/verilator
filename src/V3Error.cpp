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

// clang-format off
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#அந்தர்பாவ <thread>
// clang-format on

//======================================================================

கட்டமைப்பு v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(மாறிலி எழுத்து* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    க்கு (முழு codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        மாறிலி V3ErrorCode code{codei};
        எனில் (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            எனில் (isRenamed()) m_e = renamedTo().m_e;
            திரும்பு;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() மாறிலி {
    எனில் (!isNamed())
        திரும்பு "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_எண்_STRING;
    திரும்பு "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_எண்_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

தருக்க V3ErrorGuarded::isError(V3ErrorCode code, தருக்க supp) VL_REQUIRES(m_mutex) {
    எனில் (code.hardError()) திரும்பு உண்மை;
    எனில் (supp) திரும்பு பொய்;
    எனில் (code.severityInfo()) திரும்பு பொய்;
    எனில் (pretendError(code)) திரும்பு உண்மை;
    திரும்பு பொய்;
}

string V3ErrorGuarded::செய்தி_Prefix() VL_REQUIRES(m_mutex) {
    மாறிலி V3ErrorCode code = m_message.code();
    மாறிலி தருக்க supp = m_errorSuppressed;
    எனில் (supp) திரும்பு "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    எனில் (code.severityInfo()) திரும்பு "-Info: ";
    எனில் (code == V3ErrorCode::EC_FATAL) திரும்பு "%Error: ";
    எனில் (code == V3ErrorCode::EC_FATALMANY) திரும்பு "%Error: ";
    எனில் (code == V3ErrorCode::EC_FATALSRC) திரும்பு "%Error: Internal Error: ";
    எனில் (code == V3ErrorCode::EC_ERROR) திரும்பு "%Error: ";
    எனில் (isError(code, supp)) திரும்பு "%Error-" + std::string{code.ascii()} + ": ";
    திரும்பு "%Warning-" + std::string{code.ascii()} + ": ";
}

வெறுமை V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    எனில் (V3Error::debugDefault()) {
        std::cerr << செய்தி_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    எனில் (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#இல்லையெனில்
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    திரும்பு string(செய்தி_Prefix().size(), ' ');
}

வெறுமை V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(உண்மை);
}

வெறுமை V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = பொய்;
    m_errorSuppressed = பொய்;
}

வெறுமை V3ErrorGuarded::v3errorEnd(மாறிலி std::ostringstream& sstr, மாறிலி string& extra,
                                File_வரி* fileline) VL_REQUIRES(m_mutex) {
    நிலையான தருக்க s_firedTooMany = பொய்;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    எனில் (errorLimit() && error_மொத்தம்() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = உண்மை;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_மொத்தம்() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        ப்ரதிஜ்ஞா(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
வெறுமை V3ErrorGuarded::v3errorEndGuts(மாறிலி std::ostringstream& sstr, மாறிலி string& extra,
                                    File_வரி* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#எனில் defined(__COVERITY__) || defined(__cppcheck__)
    எனில் (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    எனில் (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        எனில் (m_message.code().defaultsOff()) திரும்பு;
        எனில் (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) திரும்பு;
        m_showedSuppressed.set(m_message.code(), உண்மை);
    }
    string செய்தி
        = V3Error::warnContextBegin() + செய்தி_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_வரி = செய்தி;
    {
        மாறிலி string::size_type pos = first_வரி.find('\n');
        எனில் (pos != string::npos) {
            first_வரி.erase(pos, first_வரி.length() - pos);
            first_வரி += "...";
        }
    }
    எனில் (m_errorSuppressed) செய்தி = first_வரி;
    // Suppress duplicate messages
    எனில் (!m_messages.செருகு(first_வரி).second) திரும்பு;

    செய்தி = VString::replaceSubstr(செய்தி, V3Error::warnMore(), warnMoreSpaces());

    string செய்தி_additional;
    {
        மாறிலி string::size_type pos = செய்தி.find(V3Error::warnAdditionalInfo());
        எனில் (pos != string::npos) {
            செய்தி_additional = செய்தி.substr(pos + V3Error::warnAdditionalInfo().size());
            செய்தி.erase(pos);
        }
    }
    செய்தி += '\n';  // Trailing newlines generally not put on messages so add
    எனில் (!extra.empty() && !m_errorSuppressed) {
        string extra_செய்தி = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_செய்தி = warnMoreSpaces() + extra_செய்தி + "\n";
        மாறிலி size_t pos = செய்தி.find('\n');
        செய்தி.செருகு(pos + 1, extra_செய்தி);
    }
    // Output
    string text;
    எனில் (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#இல்லையெனில்
        உண்மை
#endif
    )
        text += செய்தி;
    எனில் (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, பொய்);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::தள்ளு_Message(m_message);
#endif
    } இல்லையெனில் {
        மாறிலி தருக்க anError = isError(m_message.code(), m_errorSuppressed);
        எனில் (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), உண்மை);
            எனில் (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } இல்லையெனில் எனில் (m_err_மொத்தம் >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = உண்மை;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } இல்லையெனில் எனில் (!m_tellManual) {
                m_tellManual = உண்மை;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            எனில் (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                எனில் (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        எனில் (!செய்தி_additional.empty()) text += செய்தி_additional;
        std::cerr << V3Error::stripMetaText(text, பொய்);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::தள்ளு_Message(m_message);
#endif

        எனில் (anError) {
            incErrors();
        } இல்லையெனில் {
            incWarnings();
        }
        எனில் (m_message.code().severityFatal()) {
            நிலையான தருக்க s_inFatal = பொய்;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            எனில் (!s_inFatal) {
                s_inFatal = உண்மை;
#ifndef V3ERROR_NO_GLOBAL_
                எனில் (dump_மரம்_Level() || dump_மரம்_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(உண்மை);
                    எனில் (dump_மரம்_Level()) {
                        v3Global.rootp()->dump_மரம்_File(v3Global.debugFilename("final.tree", 990));
                    }
                    எனில் (dump_மரம்_Json_Level()) {
                        v3Global.rootp()->dump_மரம்_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    எனில் (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(பொய்);
#endif
            }

            vlAbortOrExit();
        } இல்லையெனில் எனில் (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

வெறுமை V3Error::init() {
    க்கு (முழு i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        மாறிலி V3ErrorCode code{i};
        describedEachWarn(code, பொய்);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    ப்ரதிஜ்ஞா(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::வரி_Str(மாறிலி எழுத்து* filename, முழு lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<முழு>(0, 20 - static_cast<முழு>(out.str().length())), ' ');
    திரும்பு out.str();
}

string V3Error::stripMetaText(மாறிலி string& text, தருக்க stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    முழு inBegins = 0;
    க்கு (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        எனில் (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            எனில் (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                போது (pos < text.size() && text[pos] != ')') ++pos;
                எனில் (pos < text.size() && text[pos] == ')') ++pos;
                தொடர்;
            }
            எனில் (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                தொடர்;
            }
            எனில் (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                தொடர்;
            }
        }
        எனில் (!stripContext || inBegins == 0) {
            // Remove double newlines
            எனில் (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    திரும்பு result;
}

வெறுமை V3Error::abortIfWarnings() {
    எனில் (!isErrorOrWarn()) திரும்பு;
    மாறிலி தருக்க exwarn = warnFatal() && warn_மொத்தம்();
    எனில் (error_மொத்தம்() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_மொத்தம்() << " error(s), "  //
                                      << V3Error::s().warn_மொத்தம்() << " warning(s)\n");
    } இல்லையெனில் எனில் (error_மொத்தம்()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_மொத்தம்() << " error(s)\n");
    } இல்லையெனில் {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_மொத்தம்() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

வெறுமை V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    திரும்பு v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_வரி(V3ErrorCode code, மாறிலி எழுத்து* file, முழு வரி)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << வரி << ": ";
    திரும்பு v3errorStr();
}
வெறுமை V3Error::v3errorEnd(மாறிலி std::ostringstream& sstr, மாறிலி string& extra, File_வரி* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

வெறுமை v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
வெறுமை v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    ப்ரதிஜ்ஞா(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
