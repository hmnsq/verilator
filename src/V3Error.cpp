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
#شامل "V3Error.h"
#شامل "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#شامل <thread>
// clang-format on

//======================================================================

ساختار v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(ثابت نویسه* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    برای (صحیح codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        ثابت V3ErrorCode code{codei};
        اگر (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            اگر (isRenamed()) m_e = renamedTo().m_e;
            بازگشت;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() ثابت {
    اگر (!isNamed())
        بازگشت "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_عدد_STRING;
    بازگشت "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_عدد_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

منطقی V3ErrorGuarded::isError(V3ErrorCode code, منطقی supp) VL_REQUIRES(m_mutex) {
    اگر (code.hardError()) بازگشت درست;
    اگر (supp) بازگشت نادرست;
    اگر (code.severityInfo()) بازگشت نادرست;
    اگر (pretendError(code)) بازگشت درست;
    بازگشت نادرست;
}

string V3ErrorGuarded::پیام_Prefix() VL_REQUIRES(m_mutex) {
    ثابت V3ErrorCode code = m_message.code();
    ثابت منطقی supp = m_errorSuppressed;
    اگر (supp) بازگشت "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    اگر (code.severityInfo()) بازگشت "-Info: ";
    اگر (code == V3ErrorCode::EC_FATAL) بازگشت "%Error: ";
    اگر (code == V3ErrorCode::EC_FATALMANY) بازگشت "%Error: ";
    اگر (code == V3ErrorCode::EC_FATALSRC) بازگشت "%Error: Internal Error: ";
    اگر (code == V3ErrorCode::EC_ERROR) بازگشت "%Error: ";
    اگر (isError(code, supp)) بازگشت "%Error-" + std::string{code.ascii()} + ": ";
    بازگشت "%Warning-" + std::string{code.ascii()} + ": ";
}

تهی V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    اگر (V3Error::debugDefault()) {
        std::cerr << پیام_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    اگر (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#وگرنه
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    بازگشت string(پیام_Prefix().size(), ' ');
}

تهی V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(درست);
}

تهی V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = نادرست;
    m_errorSuppressed = نادرست;
}

تهی V3ErrorGuarded::v3errorEnd(ثابت std::ostringstream& sstr, ثابت string& extra,
                                File_سطر* fileline) VL_REQUIRES(m_mutex) {
    ایستا منطقی s_firedTooMany = نادرست;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    اگر (errorLimit() && error_شمار() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = درست;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_شمار() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
تهی V3ErrorGuarded::v3errorEndGuts(ثابت std::ostringstream& sstr, ثابت string& extra,
                                    File_سطر* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#اگر defined(__COVERITY__) || defined(__cppcheck__)
    اگر (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    اگر (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        اگر (m_message.code().defaultsOff()) بازگشت;
        اگر (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) بازگشت;
        m_showedSuppressed.set(m_message.code(), درست);
    }
    string پیام
        = V3Error::warnContextBegin() + پیام_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_سطر = پیام;
    {
        ثابت string::size_type pos = first_سطر.find('\n');
        اگر (pos != string::npos) {
            first_سطر.erase(pos, first_سطر.length() - pos);
            first_سطر += "...";
        }
    }
    اگر (m_errorSuppressed) پیام = first_سطر;
    // Suppress duplicate messages
    اگر (!m_messages.درج(first_سطر).second) بازگشت;

    پیام = VString::replaceSubstr(پیام, V3Error::warnMore(), warnMoreSpaces());

    string پیام_additional;
    {
        ثابت string::size_type pos = پیام.find(V3Error::warnAdditionalInfo());
        اگر (pos != string::npos) {
            پیام_additional = پیام.substr(pos + V3Error::warnAdditionalInfo().size());
            پیام.erase(pos);
        }
    }
    پیام += '\n';  // Trailing newlines generally not put on messages so add
    اگر (!extra.empty() && !m_errorSuppressed) {
        string extra_پیام = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_پیام = warnMoreSpaces() + extra_پیام + "\n";
        ثابت size_t pos = پیام.find('\n');
        پیام.درج(pos + 1, extra_پیام);
    }
    // Output
    string text;
    اگر (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#وگرنه
        درست
#endif
    )
        text += پیام;
    اگر (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, نادرست);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::افزودن_Message(m_message);
#endif
    } وگرنه {
        ثابت منطقی anError = isError(m_message.code(), m_errorSuppressed);
        اگر (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), درست);
            اگر (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } وگرنه اگر (m_err_شمار >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = درست;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } وگرنه اگر (!m_tellManual) {
                m_tellManual = درست;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            اگر (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                اگر (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        اگر (!پیام_additional.empty()) text += پیام_additional;
        std::cerr << V3Error::stripMetaText(text, نادرست);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::افزودن_Message(m_message);
#endif

        اگر (anError) {
            incErrors();
        } وگرنه {
            incWarnings();
        }
        اگر (m_message.code().severityFatal()) {
            ایستا منطقی s_inFatal = نادرست;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            اگر (!s_inFatal) {
                s_inFatal = درست;
#ifndef V3ERROR_NO_GLOBAL_
                اگر (dump_درخت_Level() || dump_درخت_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(درست);
                    اگر (dump_درخت_Level()) {
                        v3Global.rootp()->dump_درخت_File(v3Global.debugFilename("final.tree", 990));
                    }
                    اگر (dump_درخت_Json_Level()) {
                        v3Global.rootp()->dump_درخت_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    اگر (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(نادرست);
#endif
            }

            vlAbortOrExit();
        } وگرنه اگر (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

تهی V3Error::init() {
    برای (صحیح i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        ثابت V3ErrorCode code{i};
        describedEachWarn(code, نادرست);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    أكد(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::سطر_Str(ثابت نویسه* filename, صحیح lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<صحیح>(0, 20 - static_cast<صحیح>(out.str().length())), ' ');
    بازگشت out.str();
}

string V3Error::stripMetaText(ثابت string& text, منطقی stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    صحیح inBegins = 0;
    برای (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        اگر (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            اگر (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                وقتی (pos < text.size() && text[pos] != ')') ++pos;
                اگر (pos < text.size() && text[pos] == ')') ++pos;
                ادامه;
            }
            اگر (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                ادامه;
            }
            اگر (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                ادامه;
            }
        }
        اگر (!stripContext || inBegins == 0) {
            // Remove double newlines
            اگر (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    بازگشت result;
}

تهی V3Error::abortIfWarnings() {
    اگر (!isErrorOrWarn()) بازگشت;
    ثابت منطقی exwarn = warnFatal() && warn_شمار();
    اگر (error_شمار() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_شمار() << " error(s), "  //
                                      << V3Error::s().warn_شمار() << " warning(s)\n");
    } وگرنه اگر (error_شمار()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_شمار() << " error(s)\n");
    } وگرنه {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_شمار() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

تهی V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    بازگشت v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_سطر(V3ErrorCode code, ثابت نویسه* file, صحیح سطر)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << سطر << ": ";
    بازگشت v3errorStr();
}
تهی V3Error::v3errorEnd(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

تهی v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
تهی v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
