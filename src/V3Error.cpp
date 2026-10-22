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

V3ErrorCode::V3ErrorCode(अचर वर्ण* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    लागि (पूर्णाङ्क codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        अचर V3ErrorCode code{codei};
        यदि (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            यदि (isRenamed()) m_e = renamedTo().m_e;
            फर्काऊ;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() अचर {
    यदि (!isNamed())
        फर्काऊ "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_सङ्ख्या_STRING;
    फर्काऊ "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_सङ्ख्या_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

तार्किक V3ErrorGuarded::isError(V3ErrorCode code, तार्किक supp) VL_REQUIRES(m_mutex) {
    यदि (code.hardError()) फर्काऊ सत्य;
    यदि (supp) फर्काऊ असत्य;
    यदि (code.severityInfo()) फर्काऊ असत्य;
    यदि (pretendError(code)) फर्काऊ सत्य;
    फर्काऊ असत्य;
}

string V3ErrorGuarded::सन्देश_Prefix() VL_REQUIRES(m_mutex) {
    अचर V3ErrorCode code = m_message.code();
    अचर तार्किक supp = m_errorSuppressed;
    यदि (supp) फर्काऊ "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    यदि (code.severityInfo()) फर्काऊ "-Info: ";
    यदि (code == V3ErrorCode::EC_FATAL) फर्काऊ "%Error: ";
    यदि (code == V3ErrorCode::EC_FATALMANY) फर्काऊ "%Error: ";
    यदि (code == V3ErrorCode::EC_FATALSRC) फर्काऊ "%Error: Internal Error: ";
    यदि (code == V3ErrorCode::EC_ERROR) फर्काऊ "%Error: ";
    यदि (isError(code, supp)) फर्काऊ "%Error-" + std::string{code.ascii()} + ": ";
    फर्काऊ "%Warning-" + std::string{code.ascii()} + ": ";
}

शून्य V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    यदि (V3Error::debugDefault()) {
        std::cerr << सन्देश_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    यदि (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
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
    फर्काऊ string(सन्देश_Prefix().size(), ' ');
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

शून्य V3ErrorGuarded::v3errorEnd(अचर std::ostringstream& sstr, अचर string& extra,
                                File_पङ्क्ति* fileline) VL_REQUIRES(m_mutex) {
    स्थिर तार्किक s_firedTooMany = असत्य;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    यदि (errorLimit() && error_गन्ती() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = सत्य;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_गन्ती() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        प्रतिज्ञा(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
शून्य V3ErrorGuarded::v3errorEndGuts(अचर std::ostringstream& sstr, अचर string& extra,
                                    File_पङ्क्ति* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#यदि defined(__COVERITY__) || defined(__cppcheck__)
    यदि (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    यदि (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        यदि (m_message.code().defaultsOff()) फर्काऊ;
        यदि (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) फर्काऊ;
        m_showedSuppressed.set(m_message.code(), सत्य);
    }
    string सन्देश
        = V3Error::warnContextBegin() + सन्देश_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_पङ्क्ति = सन्देश;
    {
        अचर string::size_type pos = first_पङ्क्ति.find('\n');
        यदि (pos != string::npos) {
            first_पङ्क्ति.erase(pos, first_पङ्क्ति.length() - pos);
            first_पङ्क्ति += "...";
        }
    }
    यदि (m_errorSuppressed) सन्देश = first_पङ्क्ति;
    // Suppress duplicate messages
    यदि (!m_messages.घुसाउ(first_पङ्क्ति).second) फर्काऊ;

    सन्देश = VString::replaceSubstr(सन्देश, V3Error::warnMore(), warnMoreSpaces());

    string सन्देश_additional;
    {
        अचर string::size_type pos = सन्देश.find(V3Error::warnAdditionalInfo());
        यदि (pos != string::npos) {
            सन्देश_additional = सन्देश.substr(pos + V3Error::warnAdditionalInfo().size());
            सन्देश.erase(pos);
        }
    }
    सन्देश += '\n';  // Trailing newlines generally not put on messages so add
    यदि (!extra.empty() && !m_errorSuppressed) {
        string extra_सन्देश = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_सन्देश = warnMoreSpaces() + extra_सन्देश + "\n";
        अचर माप_प्रकार pos = सन्देश.find('\n');
        सन्देश.घुसाउ(pos + 1, extra_सन्देश);
    }
    // Output
    string text;
    यदि (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#अन्यथा
        सत्य
#endif
    )
        text += सन्देश;
    यदि (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, असत्य);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::थप_Message(m_message);
#endif
    } अन्यथा {
        अचर तार्किक anError = isError(m_message.code(), m_errorSuppressed);
        यदि (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), सत्य);
            यदि (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } अन्यथा यदि (m_err_गन्ती >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = सत्य;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } अन्यथा यदि (!m_tellManual) {
                m_tellManual = सत्य;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            यदि (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                यदि (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        यदि (!सन्देश_additional.empty()) text += सन्देश_additional;
        std::cerr << V3Error::stripMetaText(text, असत्य);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::थप_Message(m_message);
#endif

        यदि (anError) {
            incErrors();
        } अन्यथा {
            incWarnings();
        }
        यदि (m_message.code().severityFatal()) {
            स्थिर तार्किक s_inFatal = असत्य;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            यदि (!s_inFatal) {
                s_inFatal = सत्य;
#ifndef V3ERROR_NO_GLOBAL_
                यदि (dump_रूख_Level() || dump_रूख_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(सत्य);
                    यदि (dump_रूख_Level()) {
                        v3Global.rootp()->dump_रूख_File(v3Global.debugFilename("final.tree", 990));
                    }
                    यदि (dump_रूख_Json_Level()) {
                        v3Global.rootp()->dump_रूख_Json_File(
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
#endif
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
    लागि (पूर्णाङ्क i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        अचर V3ErrorCode code{i};
        describedEachWarn(code, असत्य);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    प्रतिज्ञा(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::पङ्क्ति_Str(अचर वर्ण* filename, पूर्णाङ्क lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<पूर्णाङ्क>(0, 20 - static_cast<पूर्णाङ्क>(out.str().length())), ' ');
    फर्काऊ out.str();
}

string V3Error::stripMetaText(अचर string& text, तार्किक stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    पूर्णाङ्क inBegins = 0;
    लागि (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        यदि (0 == text.compare(pos, std::म_माप("__WARN"), "__WARN")) {
            यदि (0 == text.compare(pos, std::म_माप("__WARNRELATED("), "__WARNRELATED(")) {
                जबसम्म (pos < text.size() && text[pos] != ')') ++pos;
                यदि (pos < text.size() && text[pos] == ')') ++pos;
                जारी;
            }
            यदि (0
                == text.compare(pos, std::म_माप(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::म_माप(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                जारी;
            }
            यदि (0
                == text.compare(pos, std::म_माप(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::म_माप(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                जारी;
            }
        }
        यदि (!stripContext || inBegins == 0) {
            // Remove double newlines
            यदि (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    फर्काऊ result;
}

शून्य V3Error::abortIfWarnings() {
    यदि (!isErrorOrWarn()) फर्काऊ;
    अचर तार्किक exwarn = warnFatal() && warn_गन्ती();
    यदि (error_गन्ती() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_गन्ती() << " error(s), "  //
                                      << V3Error::s().warn_गन्ती() << " warning(s)\n");
    } अन्यथा यदि (error_गन्ती()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_गन्ती() << " error(s)\n");
    } अन्यथा {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_गन्ती() << " warning(s)\n");
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
    फर्काऊ v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_पङ्क्ति(V3ErrorCode code, अचर वर्ण* file, पूर्णाङ्क पङ्क्ति)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << पङ्क्ति << ": ";
    फर्काऊ v3errorStr();
}
शून्य V3Error::v3errorEnd(अचर std::ostringstream& sstr, अचर string& extra, File_पङ्क्ति* fileline)
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
