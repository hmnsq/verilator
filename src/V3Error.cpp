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
#ضمن "V3Error.h"
#ضمن "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#ضمن <thread>
// clang-format on

//======================================================================

بنية v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(ثابت حرف* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        ثابت V3ErrorCode code{codei};
        إذا (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            إذا (isRenamed()) m_e = renamedTo().m_e;
            إرجاع;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() ثابت {
    إذا (!isNamed())
        إرجاع "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_العدد_STRING;
    إرجاع "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_العدد_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

منطقي V3ErrorGuarded::isError(V3ErrorCode code, منطقي supp) VL_REQUIRES(m_mutex) {
    إذا (code.hardError()) إرجاع true;
    إذا (supp) إرجاع خطأ;
    إذا (code.severityInfo()) إرجاع خطأ;
    إذا (pretendError(code)) إرجاع true;
    إرجاع خطأ;
}

string V3ErrorGuarded::رسالة_Prefix() VL_REQUIRES(m_mutex) {
    ثابت V3ErrorCode code = m_message.code();
    ثابت منطقي supp = m_errorSuppressed;
    إذا (supp) إرجاع "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    إذا (code.severityInfo()) إرجاع "-Info: ";
    إذا (code == V3ErrorCode::EC_FATAL) إرجاع "%Error: ";
    إذا (code == V3ErrorCode::EC_FATALMANY) إرجاع "%Error: ";
    إذا (code == V3ErrorCode::EC_FATALSRC) إرجاع "%Error: Internal Error: ";
    إذا (code == V3ErrorCode::EC_ERROR) إرجاع "%Error: ";
    إذا (isError(code, supp)) إرجاع "%Error-" + std::string{code.ascii()} + ": ";
    إرجاع "%Warning-" + std::string{code.ascii()} + ": ";
}

فراغ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    إذا (V3Error::debugDefault()) {
        std::cerr << رسالة_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    إذا (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#وإلا
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    إرجاع string(رسالة_Prefix().size(), ' ');
}

فراغ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(true);
}

فراغ V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = خطأ;
    m_errorSuppressed = خطأ;
}

فراغ V3ErrorGuarded::v3errorEnd(ثابت std::ostringstream& sstr, ثابت string& extra,
                                File_سطر* fileline) VL_REQUIRES(m_mutex) {
    ساكن منطقي s_firedTooMany = خطأ;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    إذا (errorLimit() && error_تعداد() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = true;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_تعداد() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
فراغ V3ErrorGuarded::v3errorEndGuts(ثابت std::ostringstream& sstr, ثابت string& extra,
                                    File_سطر* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#إذا defined(__COVERITY__) || defined(__cppcheck__)
    إذا (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    إذا (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        إذا (m_message.code().defaultsOff()) إرجاع;
        إذا (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) إرجاع;
        m_showedSuppressed.set(m_message.code(), true);
    }
    string رسالة
        = V3Error::warnContextBegin() + رسالة_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_سطر = رسالة;
    {
        ثابت string::size_type pos = first_سطر.find('\n');
        إذا (pos != string::npos) {
            first_سطر.erase(pos, first_سطر.length() - pos);
            first_سطر += "...";
        }
    }
    إذا (m_errorSuppressed) رسالة = first_سطر;
    // Suppress duplicate messages
    إذا (!m_messages.أدرج(first_سطر).second) إرجاع;

    رسالة = VString::replaceSubstr(رسالة, V3Error::warnMore(), warnMoreSpaces());

    string رسالة_additional;
    {
        ثابت string::size_type pos = رسالة.find(V3Error::warnAdditionalInfo());
        إذا (pos != string::npos) {
            رسالة_additional = رسالة.substr(pos + V3Error::warnAdditionalInfo().size());
            رسالة.erase(pos);
        }
    }
    رسالة += '\n';  // Trailing newlines generally not put on messages so add
    إذا (!extra.empty() && !m_errorSuppressed) {
        string extra_رسالة = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_رسالة = warnMoreSpaces() + extra_رسالة + "\n";
        ثابت size_t pos = رسالة.find('\n');
        رسالة.أدرج(pos + 1, extra_رسالة);
    }
    // Output
    string text;
    إذا (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#وإلا
        true
#endif
    )
        text += رسالة;
    إذا (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, خطأ);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ادفع_Message(m_message);
#endif
    } وإلا {
        ثابت منطقي anError = isError(m_message.code(), m_errorSuppressed);
        إذا (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), true);
            إذا (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } وإلا إذا (m_err_تعداد >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = true;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } وإلا إذا (!m_tellManual) {
                m_tellManual = true;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            إذا (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                إذا (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        إذا (!رسالة_additional.empty()) text += رسالة_additional;
        std::cerr << V3Error::stripMetaText(text, خطأ);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ادفع_Message(m_message);
#endif

        إذا (anError) {
            incErrors();
        } وإلا {
            incWarnings();
        }
        إذا (m_message.code().severityFatal()) {
            ساكن منطقي s_inFatal = خطأ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            إذا (!s_inFatal) {
                s_inFatal = true;
#ifndef V3ERROR_NO_GLOBAL_
                إذا (dump_شجرة_Level() || dump_شجرة_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(true);
                    إذا (dump_شجرة_Level()) {
                        v3Global.rootp()->dump_شجرة_File(v3Global.debugFilename("final.tree", 990));
                    }
                    إذا (dump_شجرة_Json_Level()) {
                        v3Global.rootp()->dump_شجرة_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    إذا (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(خطأ);
#endif
            }

            vlAbortOrExit();
        } وإلا إذا (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

فراغ V3Error::init() {
    لكل (صحيح i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        ثابت V3ErrorCode code{i};
        describedEachWarn(code, خطأ);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    أكد(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::سطر_Str(ثابت حرف* filename, صحيح lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<صحيح>(0, 20 - static_cast<صحيح>(out.str().length())), ' ');
    إرجاع out.str();
}

string V3Error::stripMetaText(ثابت string& text, منطقي stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    صحيح inBegins = 0;
    لكل (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        إذا (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            إذا (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                بينما (pos < text.size() && text[pos] != ')') ++pos;
                إذا (pos < text.size() && text[pos] == ')') ++pos;
                استمر;
            }
            إذا (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                استمر;
            }
            إذا (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                استمر;
            }
        }
        إذا (!stripContext || inBegins == 0) {
            // Remove double newlines
            إذا (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    إرجاع result;
}

فراغ V3Error::abortIfWarnings() {
    إذا (!isErrorOrWarn()) إرجاع;
    ثابت منطقي exwarn = warnFatal() && warn_تعداد();
    إذا (error_تعداد() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_تعداد() << " error(s), "  //
                                      << V3Error::s().warn_تعداد() << " warning(s)\n");
    } وإلا إذا (error_تعداد()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_تعداد() << " error(s)\n");
    } وإلا {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_تعداد() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

فراغ V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    إرجاع v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_سطر(V3ErrorCode code, ثابت حرف* file, صحيح سطر)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << سطر << ": ";
    إرجاع v3errorStr();
}
فراغ V3Error::v3errorEnd(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

فراغ v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
فراغ v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
