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
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#अन्तर्भाव <thread>
// clang-format on

//======================================================================

संरचना v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(ध्रुव वर्ण* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    प्रति (पूर्णाङ्क codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        ध्रुव V3ErrorCode code{codei};
        जइ (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            जइ (isRenamed()) m_e = renamedTo().m_e;
            प्रत्यागम;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() ध्रुव {
    जइ (!isNamed())
        प्रत्यागम "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_सङ्ख्या_STRING;
    प्रत्यागम "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_सङ्ख्या_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

तार्किक V3ErrorGuarded::isError(V3ErrorCode code, तार्किक supp) VL_REQUIRES(m_mutex) {
    जइ (code.hardError()) प्रत्यागम सच्च;
    जइ (supp) प्रत्यागम मिच्छ;
    जइ (code.severityInfo()) प्रत्यागम मिच्छ;
    जइ (pretendError(code)) प्रत्यागम सच्च;
    प्रत्यागम मिच्छ;
}

string V3ErrorGuarded::सन्देशः_Prefix() VL_REQUIRES(m_mutex) {
    ध्रुव V3ErrorCode code = m_message.code();
    ध्रुव तार्किक supp = m_errorSuppressed;
    जइ (supp) प्रत्यागम "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    जइ (code.severityInfo()) प्रत्यागम "-Info: ";
    जइ (code == V3ErrorCode::EC_FATAL) प्रत्यागम "%Error: ";
    जइ (code == V3ErrorCode::EC_FATALMANY) प्रत्यागम "%Error: ";
    जइ (code == V3ErrorCode::EC_FATALSRC) प्रत्यागम "%Error: Internal Error: ";
    जइ (code == V3ErrorCode::EC_ERROR) प्रत्यागम "%Error: ";
    जइ (isError(code, supp)) प्रत्यागम "%Error-" + std::string{code.ascii()} + ": ";
    प्रत्यागम "%Warning-" + std::string{code.ascii()} + ": ";
}

शून्य V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    जइ (V3Error::debugDefault()) {
        std::cerr << सन्देशः_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    जइ (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#अण्णहा
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    प्रत्यागम string(सन्देशः_Prefix().size(), ' ');
}

शून्य V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(सच्च);
}

शून्य V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = मिच्छ;
    m_errorSuppressed = मिच्छ;
}

शून्य V3ErrorGuarded::v3errorEnd(ध्रुव std::ostringstream& sstr, ध्रुव string& extra,
                                File_पङ्क्तिः* fileline) VL_REQUIRES(m_mutex) {
    स्थिर तार्किक s_firedTooMany = मिच्छ;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    जइ (errorLimit() && error_गणनम्() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = सच्च;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_गणनम्() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        प्रतिज्ञा(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
शून्य V3ErrorGuarded::v3errorEndGuts(ध्रुव std::ostringstream& sstr, ध्रुव string& extra,
                                    File_पङ्क्तिः* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#जइ defined(__COVERITY__) || defined(__cppcheck__)
    जइ (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    जइ (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        जइ (m_message.code().defaultsOff()) प्रत्यागम;
        जइ (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) प्रत्यागम;
        m_showedSuppressed.set(m_message.code(), सच्च);
    }
    string सन्देशः
        = V3Error::warnContextBegin() + सन्देशः_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_पङ्क्तिः = सन्देशः;
    {
        ध्रुव string::size_type pos = first_पङ्क्तिः.find('\n');
        जइ (pos != string::npos) {
            first_पङ्क्तिः.erase(pos, first_पङ्क्तिः.length() - pos);
            first_पङ्क्तिः += "...";
        }
    }
    जइ (m_errorSuppressed) सन्देशः = first_पङ्क्तिः;
    // Suppress duplicate messages
    जइ (!m_messages.निवेशय(first_पङ्क्तिः).second) प्रत्यागम;

    सन्देशः = VString::replaceSubstr(सन्देशः, V3Error::warnMore(), warnMoreSpaces());

    string सन्देशः_additional;
    {
        ध्रुव string::size_type pos = सन्देशः.find(V3Error::warnAdditionalInfo());
        जइ (pos != string::npos) {
            सन्देशः_additional = सन्देशः.substr(pos + V3Error::warnAdditionalInfo().size());
            सन्देशः.erase(pos);
        }
    }
    सन्देशः += '\n';  // Trailing newlines generally not put on messages so add
    जइ (!extra.empty() && !m_errorSuppressed) {
        string extra_सन्देशः = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_सन्देशः = warnMoreSpaces() + extra_सन्देशः + "\n";
        ध्रुव size_t pos = सन्देशः.find('\n');
        सन्देशः.निवेशय(pos + 1, extra_सन्देशः);
    }
    // Output
    string text;
    जइ (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#अण्णहा
        सच्च
#endif
    )
        text += सन्देशः;
    जइ (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, मिच्छ);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::प्रक्षिप_Message(m_message);
#endif
    } अण्णहा {
        ध्रुव तार्किक anError = isError(m_message.code(), m_errorSuppressed);
        जइ (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), सच्च);
            जइ (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } अण्णहा जइ (m_err_गणनम् >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = सच्च;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } अण्णहा जइ (!m_tellManual) {
                m_tellManual = सच्च;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            जइ (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                जइ (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        जइ (!सन्देशः_additional.empty()) text += सन्देशः_additional;
        std::cerr << V3Error::stripMetaText(text, मिच्छ);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::प्रक्षिप_Message(m_message);
#endif

        जइ (anError) {
            incErrors();
        } अण्णहा {
            incWarnings();
        }
        जइ (m_message.code().severityFatal()) {
            स्थिर तार्किक s_inFatal = मिच्छ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            जइ (!s_inFatal) {
                s_inFatal = सच्च;
#ifndef V3ERROR_NO_GLOBAL_
                जइ (dump_वृक्षः_Level() || dump_वृक्षः_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(सच्च);
                    जइ (dump_वृक्षः_Level()) {
                        v3Global.rootp()->dump_वृक्षः_File(v3Global.debugFilename("final.tree", 990));
                    }
                    जइ (dump_वृक्षः_Json_Level()) {
                        v3Global.rootp()->dump_वृक्षः_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    जइ (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(मिच्छ);
#endif
            }

            vlAbortOrExit();
        } अण्णहा जइ (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

शून्य V3Error::init() {
    प्रति (पूर्णाङ्क i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        ध्रुव V3ErrorCode code{i};
        describedEachWarn(code, मिच्छ);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    प्रतिज्ञा(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::पङ्क्तिः_Str(ध्रुव वर्ण* filename, पूर्णाङ्क lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<पूर्णाङ्क>(0, 20 - static_cast<पूर्णाङ्क>(out.str().length())), ' ');
    प्रत्यागम out.str();
}

string V3Error::stripMetaText(ध्रुव string& text, तार्किक stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    पूर्णाङ्क inBegins = 0;
    प्रति (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        जइ (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            जइ (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                जाव (pos < text.size() && text[pos] != ')') ++pos;
                जइ (pos < text.size() && text[pos] == ')') ++pos;
                अनुवर्त;
            }
            जइ (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                अनुवर्त;
            }
            जइ (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                अनुवर्त;
            }
        }
        जइ (!stripContext || inBegins == 0) {
            // Remove double newlines
            जइ (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    प्रत्यागम result;
}

शून्य V3Error::abortIfWarnings() {
    जइ (!isErrorOrWarn()) प्रत्यागम;
    ध्रुव तार्किक exwarn = warnFatal() && warn_गणनम्();
    जइ (error_गणनम्() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_गणनम्() << " error(s), "  //
                                      << V3Error::s().warn_गणनम्() << " warning(s)\n");
    } अण्णहा जइ (error_गणनम्()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_गणनम्() << " error(s)\n");
    } अण्णहा {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_गणनम्() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

शून्य V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    प्रत्यागम v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_पङ्क्तिः(V3ErrorCode code, ध्रुव वर्ण* file, पूर्णाङ्क पङ्क्तिः)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << पङ्क्तिः << ": ";
    प्रत्यागम v3errorStr();
}
शून्य V3Error::v3errorEnd(ध्रुव std::ostringstream& sstr, ध्रुव string& extra, File_पङ्क्तिः* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

शून्य v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
शून्य v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
