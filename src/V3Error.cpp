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

V3ErrorCode::V3ErrorCode(स्थिरांक अक्षर* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    साठी (पूर्णांक codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        स्थिरांक V3ErrorCode code{codei};
        जर (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            जर (isRenamed()) m_e = renamedTo().m_e;
            परत;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() स्थिरांक {
    जर (!isNamed())
        परत "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_संख्या_STRING;
    परत "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_संख्या_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

तार्किक V3ErrorGuarded::isError(V3ErrorCode code, तार्किक supp) VL_REQUIRES(m_mutex) {
    जर (code.hardError()) परत सत्य;
    जर (supp) परत असत्य;
    जर (code.severityInfo()) परत असत्य;
    जर (pretendError(code)) परत सत्य;
    परत असत्य;
}

string V3ErrorGuarded::संदेश_Prefix() VL_REQUIRES(m_mutex) {
    स्थिरांक V3ErrorCode code = m_message.code();
    स्थिरांक तार्किक supp = m_errorSuppressed;
    जर (supp) परत "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    जर (code.severityInfo()) परत "-Info: ";
    जर (code == V3ErrorCode::EC_FATAL) परत "%Error: ";
    जर (code == V3ErrorCode::EC_FATALMANY) परत "%Error: ";
    जर (code == V3ErrorCode::EC_FATALSRC) परत "%Error: Internal Error: ";
    जर (code == V3ErrorCode::EC_ERROR) परत "%Error: ";
    जर (isError(code, supp)) परत "%Error-" + std::string{code.ascii()} + ": ";
    परत "%Warning-" + std::string{code.ascii()} + ": ";
}

शून्य V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    जर (V3Error::debugDefault()) {
        std::cerr << संदेश_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    जर (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#अन्यथा
    std::निकास(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    परत string(संदेश_Prefix().size(), ' ');
}

शून्य V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(सत्य);
}

शून्य V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = असत्य;
    m_errorSuppressed = असत्य;
}

शून्य V3ErrorGuarded::v3errorEnd(स्थिरांक std::ostringstream& sstr, स्थिरांक string& extra,
                                File_ओळ* fileline) VL_REQUIRES(m_mutex) {
    स्थिर तार्किक s_firedTooMany = असत्य;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    जर (errorLimit() && error_मोजणी() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = सत्य;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_मोजणी() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        प्रतिज्ञा(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
शून्य V3ErrorGuarded::v3errorEndGuts(स्थिरांक std::ostringstream& sstr, स्थिरांक string& extra,
                                    File_ओळ* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#जर defined(__COVERITY__) || defined(__cppcheck__)
    जर (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    जर (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        जर (m_message.code().defaultsOff()) परत;
        जर (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) परत;
        m_showedSuppressed.set(m_message.code(), सत्य);
    }
    string संदेश
        = V3Error::warnContextBegin() + संदेश_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_ओळ = संदेश;
    {
        स्थिरांक string::size_type pos = first_ओळ.find('\n');
        जर (pos != string::npos) {
            first_ओळ.erase(pos, first_ओळ.length() - pos);
            first_ओळ += "...";
        }
    }
    जर (m_errorSuppressed) संदेश = first_ओळ;
    // Suppress duplicate messages
    जर (!m_messages.घाला(first_ओळ).second) परत;

    संदेश = VString::replaceSubstr(संदेश, V3Error::warnMore(), warnMoreSpaces());

    string संदेश_additional;
    {
        स्थिरांक string::size_type pos = संदेश.find(V3Error::warnAdditionalInfo());
        जर (pos != string::npos) {
            संदेश_additional = संदेश.substr(pos + V3Error::warnAdditionalInfo().size());
            संदेश.erase(pos);
        }
    }
    संदेश += '\n';  // Trailing newlines generally not put on messages so add
    जर (!extra.empty() && !m_errorSuppressed) {
        string extra_संदेश = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_संदेश = warnMoreSpaces() + extra_संदेश + "\n";
        स्थिरांक माप_प्रकार pos = संदेश.find('\n');
        संदेश.घाला(pos + 1, extra_संदेश);
    }
    // Output
    string text;
    जर (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#अन्यथा
        सत्य
#endif
    )
        text += संदेश;
    जर (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, असत्य);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ढकला_Message(m_message);
#endif
    } अन्यथा {
        स्थिरांक तार्किक anError = isError(m_message.code(), m_errorSuppressed);
        जर (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), सत्य);
            जर (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } अन्यथा जर (m_err_मोजणी >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = सत्य;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } अन्यथा जर (!m_tellManual) {
                m_tellManual = सत्य;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            जर (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                जर (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        जर (!संदेश_additional.empty()) text += संदेश_additional;
        std::cerr << V3Error::stripMetaText(text, असत्य);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ढकला_Message(m_message);
#endif

        जर (anError) {
            incErrors();
        } अन्यथा {
            incWarnings();
        }
        जर (m_message.code().severityFatal()) {
            स्थिर तार्किक s_inFatal = असत्य;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            जर (!s_inFatal) {
                s_inFatal = सत्य;
#ifndef V3ERROR_NO_GLOBAL_
                जर (dump_वृक्ष_Level() || dump_वृक्ष_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(सत्य);
                    जर (dump_वृक्ष_Level()) {
                        v3Global.rootp()->dump_वृक्ष_File(v3Global.debugFilename("final.tree", 990));
                    }
                    जर (dump_वृक्ष_Json_Level()) {
                        v3Global.rootp()->dump_वृक्ष_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    जर (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(असत्य);
#endif
            }

            vlAbortOrExit();
        } अन्यथा जर (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

शून्य V3Error::init() {
    साठी (पूर्णांक i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        स्थिरांक V3ErrorCode code{i};
        describedEachWarn(code, असत्य);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    प्रतिज्ञा(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::ओळ_Str(स्थिरांक अक्षर* filename, पूर्णांक lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<पूर्णांक>(0, 20 - static_cast<पूर्णांक>(out.str().length())), ' ');
    परत out.str();
}

string V3Error::stripMetaText(स्थिरांक string& text, तार्किक stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    पूर्णांक inBegins = 0;
    साठी (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        जर (0 == text.compare(pos, std::म_माप("__WARN"), "__WARN")) {
            जर (0 == text.compare(pos, std::म_माप("__WARNRELATED("), "__WARNRELATED(")) {
                पर्यंत (pos < text.size() && text[pos] != ')') ++pos;
                जर (pos < text.size() && text[pos] == ')') ++pos;
                सुरूठेवा;
            }
            जर (0
                == text.compare(pos, std::म_माप(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::म_माप(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                सुरूठेवा;
            }
            जर (0
                == text.compare(pos, std::म_माप(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::म_माप(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                सुरूठेवा;
            }
        }
        जर (!stripContext || inBegins == 0) {
            // Remove double newlines
            जर (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    परत result;
}

शून्य V3Error::abortIfWarnings() {
    जर (!isErrorOrWarn()) परत;
    स्थिरांक तार्किक exwarn = warnFatal() && warn_मोजणी();
    जर (error_मोजणी() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_मोजणी() << " error(s), "  //
                                      << V3Error::s().warn_मोजणी() << " warning(s)\n");
    } अन्यथा जर (error_मोजणी()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_मोजणी() << " error(s)\n");
    } अन्यथा {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_मोजणी() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

शून्य V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::पात();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    परत v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_ओळ(V3ErrorCode code, स्थिरांक अक्षर* file, पूर्णांक ओळ)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << ओळ << ": ";
    परत v3errorStr();
}
शून्य V3Error::v3errorEnd(स्थिरांक std::ostringstream& sstr, स्थिरांक string& extra, File_ओळ* fileline)
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
