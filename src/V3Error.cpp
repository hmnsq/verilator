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
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#اگر_ختم
#شامل <thread>
// clang-format on

//======================================================================

ساخت v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_رمز::V3_Error_رمز(ثابت حرف* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    برائے (صحیح codei = V3_Error_رمز::EC_MIN; codei < V3_Error_رمز::_ENUM_MAX; codei++) {
        ثابت V3_Error_رمز رمز{codei};
        اگر (0 == VL_STRCASECMP(msgp, رمز.ascii())) {
            m_e = رمز;
            اگر (isRenamed()) m_e = renamed_مقصد().m_e;
            واپس;
        }
    }
    m_e = V3_Error_رمز::EC_ERROR;
}

string V3_Error_رمز::url() ثابت {
    اگر (!isNamed())
        واپس "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_نسخہ_نمبر_STRING;
    واپس "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_نسخہ_نمبر_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

منطقی V3ErrorGuarded::isError(V3_Error_رمز رمز, منطقی supp) VL_REQUIRES(m_mutex) {
    اگر (رمز.hardError()) واپس سچ;
    اگر (supp) واپس جھوٹ;
    اگر (رمز.severityInfo()) واپس جھوٹ;
    اگر (pretendError(رمز)) واپس سچ;
    واپس جھوٹ;
}

string V3ErrorGuarded::پیغام_سابقہ() VL_REQUIRES(m_mutex) {
    ثابت V3_Error_رمز رمز = m_پیغام.رمز();
    ثابت منطقی supp = m_errorSuppressed;
    اگر (supp) واپس "-arning-suppressed-" + std::string{رمز.ascii()} + ": ";
    اگر (رمز.severityInfo()) واپس "-Info: ";
    اگر (رمز == V3_Error_رمز::EC_FATAL) واپس "%Error: ";
    اگر (رمز == V3_Error_رمز::EC_FATALMANY) واپس "%Error: ";
    اگر (رمز == V3_Error_رمز::EC_FATALSRC) واپس "%Error: Internal Error: ";
    اگر (رمز == V3_Error_رمز::EC_ERROR) واپس "%Error: ";
    اگر (isError(رمز, supp)) واپس "%Error-" + std::string{رمز.ascii()} + ": ";
    واپس "%Warning-" + std::string{رمز.ascii()} + ": ";
}

خالی V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    اگر (V3Error::debugDefault()) {
        std::cerr << پیغام_سابقہ() << "Aborting since under --debug" << سطر_ختم;
        V3Error::vlAbort();
    }

#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
    اگر (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#ورنہ
    std::باہر_نکلو(1);
#اگر_ختم
}

string V3ErrorGuarded::warn_مزید_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_پیغام.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    واپس string(پیغام_سابقہ().سائز(), ' ');
}

خالی V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
    V3Stats::جوڑو_Stat_مجموع("Warnings, Suppressed "s + error_رمز().ascii(), 1);
#اگر_ختم
    errorSuppressed(سچ);
}

خالی V3ErrorGuarded::v3errorPrep(V3_Error_رمز رمز) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_پیغام.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_پیغام.init(رمز);
    m_errorContexted = جھوٹ;
    m_errorSuppressed = جھوٹ;
}

خالی V3ErrorGuarded::v3error_انتہا(ثابت std::ostringstream& sstr, ثابت string& extra,
                                File_سطر* fileline) VL_REQUIRES(m_mutex) {
    ساکن منطقی s_firedTooMany = جھوٹ;
    v3error_انتہا_Guts(sstr, extra, fileline);
    m_پیغام.clear();
    اگر (error_حد_بندی() && error_گنتی() >= error_حد_بندی() && !s_firedTooMany) {
        s_firedTooMany = سچ;
        // Recurses here
        v3error_انتہا((v3errorPrep(V3_Error_رمز::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_گنتی() << std::سطر_ختم),
                    v3errorStr()),
                   "", خالی_اشاریہ);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
خالی V3ErrorGuarded::v3error_انتہا_Guts(ثابت std::ostringstream& sstr, ثابت string& extra,
                                    File_سطر* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#اگر defined(__COVERITY__) || defined(__cppcheck__)
    اگر (m_error_رمز == V3_Error_رمز::EC_FATAL) __coverity_panic__(x);
#اگر_ختم

    UASSERT_STATIC(!m_پیغام.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_پیغام.fileline(fileline);

    // Skip suppressed messages
    اگر (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        اگر (m_پیغام.رمز().defaultsOff()) واپس;
        اگر (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_پیغام.رمز()))) واپس;
        m_showedSuppressed.سیٹ(m_پیغام.رمز(), سچ);
    }
    string پیغام_2
        = V3Error::warnContextBegin() + پیغام_سابقہ() + V3Error::warn_Context_انتہا() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string پہلا_سطر = پیغام_2;
    {
        ثابت string::سائز_type pos = پہلا_سطر.ڈھونڈو('\n');
        اگر (pos != string::npos) {
            پہلا_سطر.erase(pos, پہلا_سطر.لمبائی_2() - pos);
            پہلا_سطر += "...";
        }
    }
    اگر (m_errorSuppressed) پیغام_2 = پہلا_سطر;
    // Suppress duplicate messages
    اگر (!m_messages.داخل_کرو(پہلا_سطر).دوسرا) واپس;

    پیغام_2 = VString::replaceSubstr(پیغام_2, V3Error::warn_مزید(), warn_مزید_Spaces());

    string پیغام_additional;
    {
        ثابت string::سائز_type pos = پیغام_2.ڈھونڈو(V3Error::warnAdditionalInfo());
        اگر (pos != string::npos) {
            پیغام_additional = پیغام_2.substr(pos + V3Error::warnAdditionalInfo().سائز());
            پیغام_2.erase(pos);
        }
    }
    پیغام_2 += '\n';  // Trailing newlines generally not put on messages so add
    اگر (!extra.خالی_خانے() && !m_errorSuppressed) {
        string extra_پیغام = VString::replaceSubstr(extra, V3Error::warn_مزید(), warn_مزید_Spaces());
        extra_پیغام = warn_مزید_Spaces() + extra_پیغام + "\n";
        ثابت سائز_t pos = پیغام_2.ڈھونڈو('\n');
        پیغام_2.داخل_کرو(pos + 1, extra_پیغام);
    }
    // Output
    string متن;
    اگر (
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_پیغام.رمز() == V3_Error_رمز::EC_FATALMANY)
#ورنہ
        سچ
#اگر_ختم
    )
        متن += پیغام_2;
    اگر (m_errorSuppressed || m_پیغام.رمز().severityInfo()) {
        std::cerr << V3Error::strip_Meta_متن(متن, جھوٹ);
        m_پیغام.متن(متن);
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
        V3DiagSarif::ڈالو_پیغام(m_پیغام);
#اگر_ختم
    } ورنہ {
        ثابت منطقی anError = isError(m_پیغام.رمز(), m_errorSuppressed);
        اگر (m_پیغام.رمز()
                != V3_Error_رمز::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_پیغام.رمز())) {
            m_describedEachWarn.سیٹ(m_پیغام.رمز(), سچ);
            اگر (m_پیغام.رمز() >= V3_Error_رمز::EC_پہلا_NAMED) {
                متن += warn_مزید_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_پیغام.رمز().url() + '\n';
            } ورنہ اگر (m_err_گنتی >= 1 && m_پیغام.رمز().severityFatal() && !m_tell_اندرونی) {
                m_tell_اندرونی = سچ;
                متن += warn_مزید_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } ورنہ اگر (!m_tellManual) {
                m_tellManual = سچ;
                متن += warn_مزید_Spaces() + "... See the manual at " + m_پیغام.رمز().url()
                        + " for more assistance.\n";
            }
            اگر (!m_pretendError.test(m_پیغام.رمز()) && !m_پیغام.رمز().hardError()) {
                متن += warn_مزید_Spaces() + "... Use \"/* verilator lint_off "
                        + m_پیغام.رمز().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                اگر (m_پیغام.رمز().dangerous()) {
                    متن += warn_مزید_Spaces() + "*** See " + m_پیغام.رمز().url()
                            + " before disabling this,\n";
                    متن += warn_مزید_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        اگر (!پیغام_additional.خالی_خانے()) متن += پیغام_additional;
        std::cerr << V3Error::strip_Meta_متن(متن, جھوٹ);
        m_پیغام.متن(متن);
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
        V3DiagSarif::ڈالو_پیغام(m_پیغام);
#اگر_ختم

        اگر (anError) {
            incErrors();
        } ورنہ {
            incWarnings();
        }
        اگر (m_پیغام.رمز().severityFatal()) {
            ساکن منطقی s_اندر_Fatal = جھوٹ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            اگر (!s_اندر_Fatal) {
                s_اندر_Fatal = سچ;
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
                اگر (dump_درخت_Level() || dump_درخت_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(سچ);
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
                V3DiagSarif::output(جھوٹ);
#اگر_ختم
            }

            vlAbortOrExit();
        } ورنہ اگر (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

خالی V3Error::init() {
    برائے (صحیح i = 0; i < V3_Error_رمز::_ENUM_MAX; i++) {
        ثابت V3_Error_رمز رمز{i};
        describedEachWarn(رمز, جھوٹ);
        pretendError(رمز, رمز.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    أكد(std::string{V3_Error_رمز{V3_Error_رمز::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::سطر_Str(ثابت حرف* filename, صحیح lineno) VL_خالص {
    std::ostringstream برآمد;
    برآمد << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    برآمد << std::string(std::max<صحیح>(0, 20 - static_cast<صحیح>(برآمد.str().لمبائی_2())), ' ');
    واپس برآمد.str();
}

string V3Error::strip_Meta_متن(ثابت string& متن, منطقی stripContext) VL_خالص {
    string نتیجہ;
    نتیجہ.reserve(متن.سائز());
    صحیح اندر_Begins = 0;
    برائے (string::سائز_type pos = 0; pos < متن.سائز();) {
        // string::starts_with is C++20
        اگر (0 == متن.compare(pos, std::لڑی_لمبائی("__WARN"), "__WARN")) {
            اگر (0 == متن.compare(pos, std::لڑی_لمبائی("__WARNRELATED("), "__WARNRELATED(")) {
                جب (pos < متن.سائز() && متن[pos] != ')') ++pos;
                اگر (pos < متن.سائز() && متن[pos] == ')') ++pos;
                جاری;
            }
            اگر (0
                == متن.compare(pos, std::لڑی_لمبائی(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::لڑی_لمبائی(V3Error::WARN_CONTEXT_BEGIN);
                ++اندر_Begins;
                جاری;
            }
            اگر (0
                == متن.compare(pos, std::لڑی_لمبائی(V3Error::WARN_CONTEXT_انتہا),
                                V3Error::WARN_CONTEXT_انتہا)) {
                pos += std::لڑی_لمبائی(V3Error::WARN_CONTEXT_انتہا);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --اندر_Begins;
                جاری;
            }
        }
        اگر (!stripContext || اندر_Begins == 0) {
            // Remove double newlines
            اگر (!(متن[pos] == '\n' && !نتیجہ.خالی_خانے() && نتیجہ[نتیجہ.سائز() - 1] == '\n'))
                نتیجہ += متن[pos];
        }
        ++pos;
    }
    واپس نتیجہ;
}

خالی V3Error::abortIfWarnings() {
    اگر (!isErrorOrWarn()) واپس;
    ثابت منطقی exwarn = warnFatal() && warn_گنتی();
    اگر (error_گنتی() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_گنتی() << " error(s), "  //
                                      << V3Error::s().warn_گنتی() << " warning(s)\n");
    } ورنہ اگر (error_گنتی()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_گنتی() << " error(s)\n");
    } ورنہ {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_گنتی() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

خالی V3Error::vlAbort() {
#اگر_غیر_تعریف_شدہ V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#اگر_ختم
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_رمز رمز) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.تالا_2();
    s().v3errorPrep(رمز);
    واپس v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_سطر(V3_Error_رمز رمز, ثابت حرف* file, صحیح سطر)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(رمز) << file << ":" << std::dec << سطر << ": ";
    واپس v3errorStr();
}
خالی V3Error::v3error_انتہا(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_انتہا(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

خالی v3error_انتہا(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_انتہا(sstr, "", خالی_اشاریہ);
}
خالی v3error_انتہا_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_انتہا(sstr, "", خالی_اشاریہ);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
