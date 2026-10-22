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
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#جے_انت
#ضمن <thread>
// clang-format on

//======================================================================

بنية v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_کوڈ::V3_Error_کوڈ(ثابت حرف* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3_Error_کوڈ::EC_MIN; codei < V3_Error_کوڈ::_ENUM_MAX; codei++) {
        ثابت V3_Error_کوڈ کوڈ{codei};
        إذا (0 == VL_STRCASECMP(msgp, کوڈ.ascii())) {
            m_e = کوڈ;
            إذا (isRenamed()) m_e = renamed_منزل().m_e;
            أرجع;
        }
    }
    m_e = V3_Error_کوڈ::EC_ERROR;
}

string V3_Error_کوڈ::url() ثابت {
    إذا (!isNamed())
        أرجع "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_نسخہ_نمبر_STRING;
    أرجع "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_نسخہ_نمبر_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

منطقي V3ErrorGuarded::isError(V3_Error_کوڈ کوڈ, منطقي supp) VL_REQUIRES(m_mutex) {
    إذا (کوڈ.hardError()) أرجع سچ;
    إذا (supp) أرجع خطأ;
    إذا (کوڈ.severityInfo()) أرجع خطأ;
    إذا (pretendError(کوڈ)) أرجع سچ;
    أرجع خطأ;
}

string V3ErrorGuarded::پیغام_اگیتر() VL_REQUIRES(m_mutex) {
    ثابت V3_Error_کوڈ کوڈ = m_سنیہا.کوڈ();
    ثابت منطقي supp = m_errorSuppressed;
    إذا (supp) أرجع "-arning-suppressed-" + std::string{کوڈ.ascii()} + ": ";
    إذا (کوڈ.severityInfo()) أرجع "-Info: ";
    إذا (کوڈ == V3_Error_کوڈ::EC_FATAL) أرجع "%Error: ";
    إذا (کوڈ == V3_Error_کوڈ::EC_FATALMANY) أرجع "%Error: ";
    إذا (کوڈ == V3_Error_کوڈ::EC_FATALSRC) أرجع "%Error: Internal Error: ";
    إذا (کوڈ == V3_Error_کوڈ::EC_ERROR) أرجع "%Error: ";
    إذا (isError(کوڈ, supp)) أرجع "%Error-" + std::string{کوڈ.ascii()} + ": ";
    أرجع "%Warning-" + std::string{کوڈ.ascii()} + ": ";
}

فراغ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    إذا (V3Error::debugDefault()) {
        std::cerr << پیغام_اگیتر() << "Aborting since under --debug" << سطر_انت;
        V3Error::vlAbort();
    }

#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
    إذا (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#وإلا
    std::باہر_نکلو(1);
#جے_انت
}

string V3ErrorGuarded::warn_ہور_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_سنیہا.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    أرجع string(پیغام_اگیتر().ناپ(), ' ');
}

فراغ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
    V3Stats::جوڑو_Stat_جوڑ("Warnings, Suppressed "s + error_کوڈ().ascii(), 1);
#جے_انت
    errorSuppressed(سچ);
}

فراغ V3ErrorGuarded::v3errorPrep(V3_Error_کوڈ کوڈ) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_سنیہا.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_سنیہا.init(کوڈ);
    m_errorContexted = خطأ;
    m_errorSuppressed = خطأ;
}

فراغ V3ErrorGuarded::v3error_سرا(ثابت std::ostringstream& sstr, ثابت string& extra,
                                File_سطر* fileline) VL_REQUIRES(m_mutex) {
    ساكن منطقي s_firedTooMany = خطأ;
    v3error_سرا_Guts(sstr, extra, fileline);
    m_سنیہا.clear();
    إذا (error_حد_بندی() && error_گنتی() >= error_حد_بندی() && !s_firedTooMany) {
        s_firedTooMany = سچ;
        // Recurses here
        v3error_سرا((v3errorPrep(V3_Error_کوڈ::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_گنتی() << std::سطر_انت),
                    v3errorStr()),
                   "", خالی_سوچک);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
فراغ V3ErrorGuarded::v3error_سرا_Guts(ثابت std::ostringstream& sstr, ثابت string& extra,
                                    File_سطر* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#إذا defined(__COVERITY__) || defined(__cppcheck__)
    إذا (m_error_کوڈ == V3_Error_کوڈ::EC_FATAL) __coverity_panic__(x);
#جے_انت

    UASSERT_STATIC(!m_سنیہا.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_سنیہا.fileline(fileline);

    // Skip suppressed messages
    إذا (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        إذا (m_سنیہا.کوڈ().defaultsOff()) أرجع;
        إذا (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_سنیہا.کوڈ()))) أرجع;
        m_showedSuppressed.سیٹ(m_سنیہا.کوڈ(), سچ);
    }
    string پیغام
        = V3Error::warnContextBegin() + پیغام_اگیتر() + V3Error::warn_Context_سرا() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string پہلا_سطر = پیغام;
    {
        ثابت string::ناپ_type pos = پہلا_سطر.لبھو('\n');
        إذا (pos != string::npos) {
            پہلا_سطر.erase(pos, پہلا_سطر.لمائی() - pos);
            پہلا_سطر += "...";
        }
    }
    إذا (m_errorSuppressed) پیغام = پہلا_سطر;
    // Suppress duplicate messages
    إذا (!m_messages.داخل_کرو(پہلا_سطر).دوجا) أرجع;

    پیغام = VString::replaceSubstr(پیغام, V3Error::warn_ہور(), warn_ہور_Spaces());

    string پیغام_additional;
    {
        ثابت string::ناپ_type pos = پیغام.لبھو(V3Error::warnAdditionalInfo());
        إذا (pos != string::npos) {
            پیغام_additional = پیغام.substr(pos + V3Error::warnAdditionalInfo().ناپ());
            پیغام.erase(pos);
        }
    }
    پیغام += '\n';  // Trailing newlines generally not put on messages so add
    إذا (!extra.خالی_تھاواں() && !m_errorSuppressed) {
        string extra_پیغام = VString::replaceSubstr(extra, V3Error::warn_ہور(), warn_ہور_Spaces());
        extra_پیغام = warn_ہور_Spaces() + extra_پیغام + "\n";
        ثابت ناپ_t pos = پیغام.لبھو('\n');
        پیغام.داخل_کرو(pos + 1, extra_پیغام);
    }
    // Output
    string متن;
    إذا (
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_سنیہا.کوڈ() == V3_Error_کوڈ::EC_FATALMANY)
#وإلا
        سچ
#جے_انت
    )
        متن += پیغام;
    إذا (m_errorSuppressed || m_سنیہا.کوڈ().severityInfo()) {
        std::cerr << V3Error::strip_Meta_متن(متن, خطأ);
        m_سنیہا.متن(متن);
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
        V3DiagSarif::ڈالو_سنیہا(m_سنیہا);
#جے_انت
    } وإلا {
        ثابت منطقي anError = isError(m_سنیہا.کوڈ(), m_errorSuppressed);
        إذا (m_سنیہا.کوڈ()
                != V3_Error_کوڈ::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_سنیہا.کوڈ())) {
            m_describedEachWarn.سیٹ(m_سنیہا.کوڈ(), سچ);
            إذا (m_سنیہا.کوڈ() >= V3_Error_کوڈ::EC_پہلا_NAMED) {
                متن += warn_ہور_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_سنیہا.کوڈ().url() + '\n';
            } وإلا إذا (m_err_گنتی >= 1 && m_سنیہا.کوڈ().severityFatal() && !m_tell_اندرونی) {
                m_tell_اندرونی = سچ;
                متن += warn_ہور_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } وإلا إذا (!m_tellManual) {
                m_tellManual = سچ;
                متن += warn_ہور_Spaces() + "... See the manual at " + m_سنیہا.کوڈ().url()
                        + " for more assistance.\n";
            }
            إذا (!m_pretendError.test(m_سنیہا.کوڈ()) && !m_سنیہا.کوڈ().hardError()) {
                متن += warn_ہور_Spaces() + "... Use \"/* verilator lint_off "
                        + m_سنیہا.کوڈ().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                إذا (m_سنیہا.کوڈ().dangerous()) {
                    متن += warn_ہور_Spaces() + "*** See " + m_سنیہا.کوڈ().url()
                            + " before disabling this,\n";
                    متن += warn_ہور_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        إذا (!پیغام_additional.خالی_تھاواں()) متن += پیغام_additional;
        std::cerr << V3Error::strip_Meta_متن(متن, خطأ);
        m_سنیہا.متن(متن);
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
        V3DiagSarif::ڈالو_سنیہا(m_سنیہا);
#جے_انت

        إذا (anError) {
            incErrors();
        } وإلا {
            incWarnings();
        }
        إذا (m_سنیہا.کوڈ().severityFatal()) {
            ساكن منطقي s_وچ_Fatal = خطأ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            إذا (!s_وچ_Fatal) {
                s_وچ_Fatal = سچ;
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
                إذا (dump_درخت_Level() || dump_درخت_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(سچ);
                    إذا (dump_درخت_Level()) {
                        v3Global.rootp()->dump_درخت_File(v3Global.debugFilename("final.tree", 990));
                    }
                    إذا (dump_درخت_Json_Level()) {
                        v3Global.rootp()->dump_درخت_Json_File(
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
#جے_انت
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
    لكل (صحيح i = 0; i < V3_Error_کوڈ::_ENUM_MAX; i++) {
        ثابت V3_Error_کوڈ کوڈ{i};
        describedEachWarn(کوڈ, خطأ);
        pretendError(کوڈ, کوڈ.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    أكد(std::string{V3_Error_کوڈ{V3_Error_کوڈ::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::سطر_Str(ثابت حرف* filename, صحيح lineno) VL_خالص {
    std::ostringstream باہرلا;
    باہرلا << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    باہرلا << std::string(std::max<صحيح>(0, 20 - static_cast<صحيح>(باہرلا.str().لمائی())), ' ');
    أرجع باہرلا.str();
}

string V3Error::strip_Meta_متن(ثابت string& متن, منطقي stripContext) VL_خالص {
    string نتیجہ;
    نتیجہ.reserve(متن.ناپ());
    صحيح وچ_Begins = 0;
    لكل (string::ناپ_type pos = 0; pos < متن.ناپ();) {
        // string::starts_with is C++20
        إذا (0 == متن.compare(pos, std::لڑی_لمبائی("__WARN"), "__WARN")) {
            إذا (0 == متن.compare(pos, std::لڑی_لمبائی("__WARNRELATED("), "__WARNRELATED(")) {
                طالما (pos < متن.ناپ() && متن[pos] != ')') ++pos;
                إذا (pos < متن.ناپ() && متن[pos] == ')') ++pos;
                استمر;
            }
            إذا (0
                == متن.compare(pos, std::لڑی_لمبائی(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::لڑی_لمبائی(V3Error::WARN_CONTEXT_BEGIN);
                ++وچ_Begins;
                استمر;
            }
            إذا (0
                == متن.compare(pos, std::لڑی_لمبائی(V3Error::WARN_CONTEXT_سرا),
                                V3Error::WARN_CONTEXT_سرا)) {
                pos += std::لڑی_لمبائی(V3Error::WARN_CONTEXT_سرا);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --وچ_Begins;
                استمر;
            }
        }
        إذا (!stripContext || وچ_Begins == 0) {
            // Remove double newlines
            إذا (!(متن[pos] == '\n' && !نتیجہ.خالی_تھاواں() && نتیجہ[نتیجہ.ناپ() - 1] == '\n'))
                نتیجہ += متن[pos];
        }
        ++pos;
    }
    أرجع نتیجہ;
}

فراغ V3Error::abortIfWarnings() {
    إذا (!isErrorOrWarn()) أرجع;
    ثابت منطقي exwarn = warnFatal() && warn_گنتی();
    إذا (error_گنتی() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_گنتی() << " error(s), "  //
                                      << V3Error::s().warn_گنتی() << " warning(s)\n");
    } وإلا إذا (error_گنتی()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_گنتی() << " error(s)\n");
    } وإلا {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_گنتی() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

فراغ V3Error::vlAbort() {
#جے_بنا_تعریف V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#جے_انت
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_کوڈ کوڈ) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.جندرا_2();
    s().v3errorPrep(کوڈ);
    أرجع v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_سطر(V3_Error_کوڈ کوڈ, ثابت حرف* file, صحيح سطر)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(کوڈ) << file << ":" << std::dec << سطر << ": ";
    أرجع v3errorStr();
}
فراغ V3Error::v3error_سرا(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_سرا(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

فراغ v3error_سرا(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_سرا(sstr, "", خالی_سوچک);
}
فراغ v3error_سرا_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_سرا(sstr, "", خالی_سوچک);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
