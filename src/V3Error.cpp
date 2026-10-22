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
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#ಅನ್ತರ್ಭಾವ <thread>
// clang-format on

//======================================================================

ಸಂರಚನಾ v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(ಧ್ರುವ ವರ್ಣ* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        ಧ್ರುವ V3ErrorCode code{codei};
        ಯದಿ (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            ಯದಿ (isRenamed()) m_e = renamedTo().m_e;
            ಪ್ರತ್ಯಾಗಮ;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() ಧ್ರುವ {
    ಯದಿ (!isNamed())
        ಪ್ರತ್ಯಾಗಮ "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_ಸಂಖ್ಯೆ_STRING;
    ಪ್ರತ್ಯಾಗಮ "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_ಸಂಖ್ಯೆ_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

ತಾರ್ಕಿಕ V3ErrorGuarded::isError(V3ErrorCode code, ತಾರ್ಕಿಕ supp) VL_REQUIRES(m_mutex) {
    ಯದಿ (code.hardError()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    ಯದಿ (supp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (code.severityInfo()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (pretendError(code)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
}

string V3ErrorGuarded::ಸಂದೇಶ_Prefix() VL_REQUIRES(m_mutex) {
    ಧ್ರುವ V3ErrorCode code = m_message.code();
    ಧ್ರುವ ತಾರ್ಕಿಕ supp = m_errorSuppressed;
    ಯದಿ (supp) ಪ್ರತ್ಯಾಗಮ "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    ಯದಿ (code.severityInfo()) ಪ್ರತ್ಯಾಗಮ "-Info: ";
    ಯದಿ (code == V3ErrorCode::EC_FATAL) ಪ್ರತ್ಯಾಗಮ "%Error: ";
    ಯದಿ (code == V3ErrorCode::EC_FATALMANY) ಪ್ರತ್ಯಾಗಮ "%Error: ";
    ಯದಿ (code == V3ErrorCode::EC_FATALSRC) ಪ್ರತ್ಯಾಗಮ "%Error: Internal Error: ";
    ಯದಿ (code == V3ErrorCode::EC_ERROR) ಪ್ರತ್ಯಾಗಮ "%Error: ";
    ಯದಿ (isError(code, supp)) ಪ್ರತ್ಯಾಗಮ "%Error-" + std::string{code.ascii()} + ": ";
    ಪ್ರತ್ಯಾಗಮ "%Warning-" + std::string{code.ascii()} + ": ";
}

ಶೂನ್ಯ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    ಯದಿ (V3Error::debugDefault()) {
        std::cerr << ಸಂದೇಶ_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    ಯದಿ (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#ಅನ್ಯಥಾ
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    ಪ್ರತ್ಯಾಗಮ string(ಸಂದೇಶ_Prefix().size(), ' ');
}

ಶೂನ್ಯ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(ಸತ್ಯ);
}

ಶೂನ್ಯ V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = ಅಸತ್ಯ;
    m_errorSuppressed = ಅಸತ್ಯ;
}

ಶೂನ್ಯ V3ErrorGuarded::v3errorEnd(ಧ್ರುವ std::ostringstream& sstr, ಧ್ರುವ string& extra,
                                File_ಸಾಲು* fileline) VL_REQUIRES(m_mutex) {
    ಸ್ಥಿರ ತಾರ್ಕಿಕ s_firedTooMany = ಅಸತ್ಯ;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    ಯದಿ (errorLimit() && error_ಎಣಿಕೆ() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = ಸತ್ಯ;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_ಎಣಿಕೆ() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
ಶೂನ್ಯ V3ErrorGuarded::v3errorEndGuts(ಧ್ರುವ std::ostringstream& sstr, ಧ್ರುವ string& extra,
                                    File_ಸಾಲು* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#ಯದಿ defined(__COVERITY__) || defined(__cppcheck__)
    ಯದಿ (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    ಯದಿ (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        ಯದಿ (m_message.code().defaultsOff()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) ಪ್ರತ್ಯಾಗಮ;
        m_showedSuppressed.set(m_message.code(), ಸತ್ಯ);
    }
    string ಸಂದೇಶ
        = V3Error::warnContextBegin() + ಸಂದೇಶ_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_ಸಾಲು = ಸಂದೇಶ;
    {
        ಧ್ರುವ string::size_type pos = first_ಸಾಲು.find('\n');
        ಯದಿ (pos != string::npos) {
            first_ಸಾಲು.erase(pos, first_ಸಾಲು.length() - pos);
            first_ಸಾಲು += "...";
        }
    }
    ಯದಿ (m_errorSuppressed) ಸಂದೇಶ = first_ಸಾಲು;
    // Suppress duplicate messages
    ಯದಿ (!m_messages.ಸೇರಿಸು(first_ಸಾಲು).second) ಪ್ರತ್ಯಾಗಮ;

    ಸಂದೇಶ = VString::replaceSubstr(ಸಂದೇಶ, V3Error::warnMore(), warnMoreSpaces());

    string ಸಂದೇಶ_additional;
    {
        ಧ್ರುವ string::size_type pos = ಸಂದೇಶ.find(V3Error::warnAdditionalInfo());
        ಯದಿ (pos != string::npos) {
            ಸಂದೇಶ_additional = ಸಂದೇಶ.substr(pos + V3Error::warnAdditionalInfo().size());
            ಸಂದೇಶ.erase(pos);
        }
    }
    ಸಂದೇಶ += '\n';  // Trailing newlines generally not put on messages so add
    ಯದಿ (!extra.empty() && !m_errorSuppressed) {
        string extra_ಸಂದೇಶ = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_ಸಂದೇಶ = warnMoreSpaces() + extra_ಸಂದೇಶ + "\n";
        ಧ್ರುವ size_t pos = ಸಂದೇಶ.find('\n');
        ಸಂದೇಶ.ಸೇರಿಸು(pos + 1, extra_ಸಂದೇಶ);
    }
    // Output
    string text;
    ಯದಿ (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#ಅನ್ಯಥಾ
        ಸತ್ಯ
#endif
    )
        text += ಸಂದೇಶ;
    ಯದಿ (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, ಅಸತ್ಯ);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ತಳ್ಳು_Message(m_message);
#endif
    } ಅನ್ಯಥಾ {
        ಧ್ರುವ ತಾರ್ಕಿಕ anError = isError(m_message.code(), m_errorSuppressed);
        ಯದಿ (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), ಸತ್ಯ);
            ಯದಿ (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } ಅನ್ಯಥಾ ಯದಿ (m_err_ಎಣಿಕೆ >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = ಸತ್ಯ;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } ಅನ್ಯಥಾ ಯದಿ (!m_tellManual) {
                m_tellManual = ಸತ್ಯ;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            ಯದಿ (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                ಯದಿ (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        ಯದಿ (!ಸಂದೇಶ_additional.empty()) text += ಸಂದೇಶ_additional;
        std::cerr << V3Error::stripMetaText(text, ಅಸತ್ಯ);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ತಳ್ಳು_Message(m_message);
#endif

        ಯದಿ (anError) {
            incErrors();
        } ಅನ್ಯಥಾ {
            incWarnings();
        }
        ಯದಿ (m_message.code().severityFatal()) {
            ಸ್ಥಿರ ತಾರ್ಕಿಕ s_inFatal = ಅಸತ್ಯ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            ಯದಿ (!s_inFatal) {
                s_inFatal = ಸತ್ಯ;
#ifndef V3ERROR_NO_GLOBAL_
                ಯದಿ (dump_ವೃಕ್ಷ_Level() || dump_ವೃಕ್ಷ_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(ಸತ್ಯ);
                    ಯದಿ (dump_ವೃಕ್ಷ_Level()) {
                        v3Global.rootp()->dump_ವೃಕ್ಷ_File(v3Global.debugFilename("final.tree", 990));
                    }
                    ಯದಿ (dump_ವೃಕ್ಷ_Json_Level()) {
                        v3Global.rootp()->dump_ವೃಕ್ಷ_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    ಯದಿ (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(ಅಸತ್ಯ);
#endif
            }

            vlAbortOrExit();
        } ಅನ್ಯಥಾ ಯದಿ (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

ಶೂನ್ಯ V3Error::init() {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        ಧ್ರುವ V3ErrorCode code{i};
        describedEachWarn(code, ಅಸತ್ಯ);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    ಪ್ರತಿಜ್ಞಾ(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::ಸಾಲು_Str(ಧ್ರುವ ವರ್ಣ* filename, ಪೂರ್ಣಾಙ್ಕ lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<ಪೂರ್ಣಾಙ್ಕ>(0, 20 - static_cast<ಪೂರ್ಣಾಙ್ಕ>(out.str().length())), ' ');
    ಪ್ರತ್ಯಾಗಮ out.str();
}

string V3Error::stripMetaText(ಧ್ರುವ string& text, ತಾರ್ಕಿಕ stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    ಪೂರ್ಣಾಙ್ಕ inBegins = 0;
    ಪ್ರತಿ (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        ಯದಿ (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            ಯದಿ (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                ಯಾವತ್ (pos < text.size() && text[pos] != ')') ++pos;
                ಯದಿ (pos < text.size() && text[pos] == ')') ++pos;
                ಅನುವರ್ತ;
            }
            ಯದಿ (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                ಅನುವರ್ತ;
            }
            ಯದಿ (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                ಅನುವರ್ತ;
            }
        }
        ಯದಿ (!stripContext || inBegins == 0) {
            // Remove double newlines
            ಯದಿ (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

ಶೂನ್ಯ V3Error::abortIfWarnings() {
    ಯದಿ (!isErrorOrWarn()) ಪ್ರತ್ಯಾಗಮ;
    ಧ್ರುವ ತಾರ್ಕಿಕ exwarn = warnFatal() && warn_ಎಣಿಕೆ();
    ಯದಿ (error_ಎಣಿಕೆ() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ಎಣಿಕೆ() << " error(s), "  //
                                      << V3Error::s().warn_ಎಣಿಕೆ() << " warning(s)\n");
    } ಅನ್ಯಥಾ ಯದಿ (error_ಎಣಿಕೆ()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ಎಣಿಕೆ() << " error(s)\n");
    } ಅನ್ಯಥಾ {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_ಎಣಿಕೆ() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

ಶೂನ್ಯ V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    ಪ್ರತ್ಯಾಗಮ v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_ಸಾಲು(V3ErrorCode code, ಧ್ರುವ ವರ್ಣ* file, ಪೂರ್ಣಾಙ್ಕ ಸಾಲು)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << ಸಾಲು << ": ";
    ಪ್ರತ್ಯಾಗಮ v3errorStr();
}
ಶೂನ್ಯ V3Error::v3errorEnd(ಧ್ರುವ std::ostringstream& sstr, ಧ್ರುವ string& extra, File_ಸಾಲು* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

ಶೂನ್ಯ v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
ಶೂನ್ಯ v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
