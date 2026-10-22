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
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#यदि_अन्तः
#अन्तर्भाव <thread>
// clang-format on

//======================================================================

संरचना v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_संहिता::V3_Error_संहिता(ध्रुव वर्ण* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    प्रति (पूर्णाङ्क codei = V3_Error_संहिता::EC_MIN; codei < V3_Error_संहिता::_ENUM_MAX; codei++) {
        ध्रुव V3_Error_संहिता संहिता{codei};
        यदि (0 == VL_STRCASECMP(msgp, संहिता.ascii())) {
            m_e = संहिता;
            यदि (isRenamed()) m_e = renamed_गन्तव्यम्().m_e;
            प्रत्यागम;
        }
    }
    m_e = V3_Error_संहिता::EC_ERROR;
}

string V3_Error_संहिता::url() ध्रुव {
    यदि (!isNamed())
        प्रत्यागम "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_संस्करणम्_सङ्ख्या_STRING;
    प्रत्यागम "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_संस्करणम्_सङ्ख्या_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

तार्किक V3ErrorGuarded::isError(V3_Error_संहिता संहिता, तार्किक supp) VL_REQUIRES(m_mutex) {
    यदि (संहिता.hardError()) प्रत्यागम सत्य;
    यदि (supp) प्रत्यागम असत्य;
    यदि (संहिता.severityInfo()) प्रत्यागम असत्य;
    यदि (pretendError(संहिता)) प्रत्यागम सत्य;
    प्रत्यागम असत्य;
}

string V3ErrorGuarded::सन्देशः_उपसर्गः() VL_REQUIRES(m_mutex) {
    ध्रुव V3_Error_संहिता संहिता = m_सन्देशः.संहिता();
    ध्रुव तार्किक supp = m_errorSuppressed;
    यदि (supp) प्रत्यागम "-arning-suppressed-" + std::string{संहिता.ascii()} + ": ";
    यदि (संहिता.severityInfo()) प्रत्यागम "-Info: ";
    यदि (संहिता == V3_Error_संहिता::EC_FATAL) प्रत्यागम "%Error: ";
    यदि (संहिता == V3_Error_संहिता::EC_FATALMANY) प्रत्यागम "%Error: ";
    यदि (संहिता == V3_Error_संहिता::EC_FATALSRC) प्रत्यागम "%Error: Internal Error: ";
    यदि (संहिता == V3_Error_संहिता::EC_ERROR) प्रत्यागम "%Error: ";
    यदि (isError(संहिता, supp)) प्रत्यागम "%Error-" + std::string{संहिता.ascii()} + ": ";
    प्रत्यागम "%Warning-" + std::string{संहिता.ascii()} + ": ";
}

शून्य V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    यदि (V3Error::debugDefault()) {
        std::cerr << सन्देशः_उपसर्गः() << "Aborting since under --debug" << पङ्क्त्यन्तः;
        V3Error::vlAbort();
    }

#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
    यदि (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#अन्यथा
    std::निर्गच्छ(1);
#यदि_अन्तः
}

string V3ErrorGuarded::warn_अधिकम्_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_सन्देशः.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    प्रत्यागम string(सन्देशः_उपसर्गः().आकार(), ' ');
}

शून्य V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
    V3Stats::सङ्कलय_Stat_योग("Warnings, Suppressed "s + error_संहिता().ascii(), 1);
#यदि_अन्तः
    errorSuppressed(सत्य);
}

शून्य V3ErrorGuarded::v3errorPrep(V3_Error_संहिता संहिता) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_सन्देशः.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_सन्देशः.init(संहिता);
    m_errorContexted = असत्य;
    m_errorSuppressed = असत्य;
}

शून्य V3ErrorGuarded::v3error_अन्तः(ध्रुव std::ostringstream& sstr, ध्रुव string& extra,
                                File_पङ्क्तिः* fileline) VL_REQUIRES(m_mutex) {
    स्थिर तार्किक s_firedTooMany = असत्य;
    v3error_अन्तः_Guts(sstr, extra, fileline);
    m_सन्देशः.clear();
    यदि (error_सीमा() && error_गणनम्() >= error_सीमा() && !s_firedTooMany) {
        s_firedTooMany = सत्य;
        // Recurses here
        v3error_अन्तः((v3errorPrep(V3_Error_संहिता::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_गणनम्() << std::पङ्क्त्यन्तः),
                    v3errorStr()),
                   "", शून्यसूचकः);
        प्रतिज्ञा(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
शून्य V3ErrorGuarded::v3error_अन्तः_Guts(ध्रुव std::ostringstream& sstr, ध्रुव string& extra,
                                    File_पङ्क्तिः* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#यदि defined(__COVERITY__) || defined(__cppcheck__)
    यदि (m_error_संहिता == V3_Error_संहिता::EC_FATAL) __coverity_panic__(x);
#यदि_अन्तः

    UASSERT_STATIC(!m_सन्देशः.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_सन्देशः.fileline(fileline);

    // Skip suppressed messages
    यदि (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        यदि (m_सन्देशः.संहिता().defaultsOff()) प्रत्यागम;
        यदि (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_सन्देशः.संहिता()))) प्रत्यागम;
        m_showedSuppressed.समुच्चयः(m_सन्देशः.संहिता(), सत्य);
    }
    string सन्देशः_2
        = V3Error::warnContextBegin() + सन्देशः_उपसर्गः() + V3Error::warn_Context_अन्तः() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_पङ्क्तिः = सन्देशः_2;
    {
        ध्रुव string::आकार_type pos = first_पङ्क्तिः.find('\n');
        यदि (pos != string::npos) {
            first_पङ्क्तिः.erase(pos, first_पङ्क्तिः.दीर्घता_2() - pos);
            first_पङ्क्तिः += "...";
        }
    }
    यदि (m_errorSuppressed) सन्देशः_2 = first_पङ्क्तिः;
    // Suppress duplicate messages
    यदि (!m_messages.निवेशय(first_पङ्क्तिः).second) प्रत्यागम;

    सन्देशः_2 = VString::replaceSubstr(सन्देशः_2, V3Error::warn_अधिकम्(), warn_अधिकम्_Spaces());

    string सन्देशः_additional;
    {
        ध्रुव string::आकार_type pos = सन्देशः_2.find(V3Error::warnAdditionalInfo());
        यदि (pos != string::npos) {
            सन्देशः_additional = सन्देशः_2.substr(pos + V3Error::warnAdditionalInfo().आकार());
            सन्देशः_2.erase(pos);
        }
    }
    सन्देशः_2 += '\n';  // Trailing newlines generally not put on messages so add
    यदि (!extra.रिक्तानि() && !m_errorSuppressed) {
        string extra_सन्देशः = VString::replaceSubstr(extra, V3Error::warn_अधिकम्(), warn_अधिकम्_Spaces());
        extra_सन्देशः = warn_अधिकम्_Spaces() + extra_सन्देशः + "\n";
        ध्रुव आकार_t pos = सन्देशः_2.find('\n');
        सन्देशः_2.निवेशय(pos + 1, extra_सन्देशः);
    }
    // Output
    string text;
    यदि (
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_सन्देशः.संहिता() == V3_Error_संहिता::EC_FATALMANY)
#अन्यथा
        सत्य
#यदि_अन्तः
    )
        text += सन्देशः_2;
    यदि (m_errorSuppressed || m_सन्देशः.संहिता().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, असत्य);
        m_सन्देशः.text(text);
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
        V3DiagSarif::प्रक्षिप_सन्देशः(m_सन्देशः);
#यदि_अन्तः
    } अन्यथा {
        ध्रुव तार्किक anError = isError(m_सन्देशः.संहिता(), m_errorSuppressed);
        यदि (m_सन्देशः.संहिता()
                != V3_Error_संहिता::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_सन्देशः.संहिता())) {
            m_describedEachWarn.समुच्चयः(m_सन्देशः.संहिता(), सत्य);
            यदि (m_सन्देशः.संहिता() >= V3_Error_संहिता::EC_FIRST_NAMED) {
                text += warn_अधिकम्_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_सन्देशः.संहिता().url() + '\n';
            } अन्यथा यदि (m_err_गणनम् >= 1 && m_सन्देशः.संहिता().severityFatal() && !m_tell_आन्तरिकम्) {
                m_tell_आन्तरिकम् = सत्य;
                text += warn_अधिकम्_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } अन्यथा यदि (!m_tellManual) {
                m_tellManual = सत्य;
                text += warn_अधिकम्_Spaces() + "... See the manual at " + m_सन्देशः.संहिता().url()
                        + " for more assistance.\n";
            }
            यदि (!m_pretendError.test(m_सन्देशः.संहिता()) && !m_सन्देशः.संहिता().hardError()) {
                text += warn_अधिकम्_Spaces() + "... Use \"/* verilator lint_off "
                        + m_सन्देशः.संहिता().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                यदि (m_सन्देशः.संहिता().dangerous()) {
                    text += warn_अधिकम्_Spaces() + "*** See " + m_सन्देशः.संहिता().url()
                            + " before disabling this,\n";
                    text += warn_अधिकम्_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        यदि (!सन्देशः_additional.रिक्तानि()) text += सन्देशः_additional;
        std::cerr << V3Error::stripMetaText(text, असत्य);
        m_सन्देशः.text(text);
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
        V3DiagSarif::प्रक्षिप_सन्देशः(m_सन्देशः);
#यदि_अन्तः

        यदि (anError) {
            incErrors();
        } अन्यथा {
            incWarnings();
        }
        यदि (m_सन्देशः.संहिता().severityFatal()) {
            स्थिर तार्किक s_अन्तर्_Fatal = असत्य;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            यदि (!s_अन्तर्_Fatal) {
                s_अन्तर्_Fatal = सत्य;
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
                यदि (dump_वृक्षः_Level() || dump_वृक्षः_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(सत्य);
                    यदि (dump_वृक्षः_Level()) {
                        v3Global.rootp()->dump_वृक्षः_File(v3Global.debugFilename("final.tree", 990));
                    }
                    यदि (dump_वृक्षः_Json_Level()) {
                        v3Global.rootp()->dump_वृक्षः_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    यदि (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(असत्य);
#यदि_अन्तः
            }

            vlAbortOrExit();
        } अन्यथा यदि (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

शून्य V3Error::init() {
    प्रति (पूर्णाङ्क i = 0; i < V3_Error_संहिता::_ENUM_MAX; i++) {
        ध्रुव V3_Error_संहिता संहिता{i};
        describedEachWarn(संहिता, असत्य);
        pretendError(संहिता, संहिता.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    प्रतिज्ञा(std::string{V3_Error_संहिता{V3_Error_संहिता::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::पङ्क्तिः_Str(ध्रुव वर्ण* filename, पूर्णाङ्क lineno) VL_विशुद्धम् {
    std::ostringstream बहिर्गमनम्;
    बहिर्गमनम् << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    बहिर्गमनम् << std::string(std::max<पूर्णाङ्क>(0, 20 - static_cast<पूर्णाङ्क>(बहिर्गमनम्.str().दीर्घता_2())), ' ');
    प्रत्यागम बहिर्गमनम्.str();
}

string V3Error::stripMetaText(ध्रुव string& text, तार्किक stripContext) VL_विशुद्धम् {
    string परिणाम;
    परिणाम.reserve(text.आकार());
    पूर्णाङ्क अन्तर्_Begins = 0;
    प्रति (string::आकार_type pos = 0; pos < text.आकार();) {
        // string::starts_with is C++20
        यदि (0 == text.compare(pos, std::वाक्यदैर्घ्यम्("__WARN"), "__WARN")) {
            यदि (0 == text.compare(pos, std::वाक्यदैर्घ्यम्("__WARNRELATED("), "__WARNRELATED(")) {
                यावत् (pos < text.आकार() && text[pos] != ')') ++pos;
                यदि (pos < text.आकार() && text[pos] == ')') ++pos;
                अनुवर्त;
            }
            यदि (0
                == text.compare(pos, std::वाक्यदैर्घ्यम्(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::वाक्यदैर्घ्यम्(V3Error::WARN_CONTEXT_BEGIN);
                ++अन्तर्_Begins;
                अनुवर्त;
            }
            यदि (0
                == text.compare(pos, std::वाक्यदैर्घ्यम्(V3Error::WARN_CONTEXT_अन्तः),
                                V3Error::WARN_CONTEXT_अन्तः)) {
                pos += std::वाक्यदैर्घ्यम्(V3Error::WARN_CONTEXT_अन्तः);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --अन्तर्_Begins;
                अनुवर्त;
            }
        }
        यदि (!stripContext || अन्तर्_Begins == 0) {
            // Remove double newlines
            यदि (!(text[pos] == '\n' && !परिणाम.रिक्तानि() && परिणाम[परिणाम.आकार() - 1] == '\n'))
                परिणाम += text[pos];
        }
        ++pos;
    }
    प्रत्यागम परिणाम;
}

शून्य V3Error::abortIfWarnings() {
    यदि (!isErrorOrWarn()) प्रत्यागम;
    ध्रुव तार्किक exwarn = warnFatal() && warn_गणनम्();
    यदि (error_गणनम्() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_गणनम्() << " error(s), "  //
                                      << V3Error::s().warn_गणनम्() << " warning(s)\n");
    } अन्यथा यदि (error_गणनम्()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_गणनम्() << " error(s)\n");
    } अन्यथा {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_गणनम्() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

शून्य V3Error::vlAbort() {
#यदि_अपरिभाषितम् V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#यदि_अन्तः
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_संहिता संहिता) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.तालकम्_2();
    s().v3errorPrep(संहिता);
    प्रत्यागम v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_पङ्क्तिः(V3_Error_संहिता संहिता, ध्रुव वर्ण* file, पूर्णाङ्क पङ्क्तिः)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(संहिता) << file << ":" << std::dec << पङ्क्तिः << ": ";
    प्रत्यागम v3errorStr();
}
शून्य V3Error::v3error_अन्तः(ध्रुव std::ostringstream& sstr, ध्रुव string& extra, File_पङ्क्तिः* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_अन्तः(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

शून्य v3error_अन्तः(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_अन्तः(sstr, "", शून्यसूचकः);
}
शून्य v3error_अन्तः_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_अन्तः(sstr, "", शून्यसूचकः);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
