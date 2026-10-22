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
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#پایان_اگر
#ضمن <thread>
// clang-format on

//======================================================================

بنية v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_کد::V3_Error_کد(ثابت حرف* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3_Error_کد::EC_MIN; codei < V3_Error_کد::_ENUM_MAX; codei++) {
        ثابت V3_Error_کد کد{codei};
        إذا (0 == VL_STRCASECMP(msgp, کد.ascii())) {
            m_e = کد;
            إذا (isRenamed()) m_e = renamed_مقصد().m_e;
            أرجع;
        }
    }
    m_e = V3_Error_کد::EC_ERROR;
}

string V3_Error_کد::url() ثابت {
    إذا (!isNamed())
        أرجع "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_نسخه_عدد_STRING;
    أرجع "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_نسخه_عدد_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

منطقي V3ErrorGuarded::isError(V3_Error_کد کد, منطقي supp) VL_REQUIRES(m_mutex) {
    إذا (کد.hardError()) أرجع درست;
    إذا (supp) أرجع خطأ;
    إذا (کد.severityInfo()) أرجع خطأ;
    إذا (pretendError(کد)) أرجع درست;
    أرجع خطأ;
}

string V3ErrorGuarded::پیام_پیشوند() VL_REQUIRES(m_mutex) {
    ثابت V3_Error_کد کد = m_پیام.کد();
    ثابت منطقي supp = m_errorSuppressed;
    إذا (supp) أرجع "-arning-suppressed-" + std::string{کد.ascii()} + ": ";
    إذا (کد.severityInfo()) أرجع "-Info: ";
    إذا (کد == V3_Error_کد::EC_FATAL) أرجع "%Error: ";
    إذا (کد == V3_Error_کد::EC_FATALMANY) أرجع "%Error: ";
    إذا (کد == V3_Error_کد::EC_FATALSRC) أرجع "%Error: Internal Error: ";
    إذا (کد == V3_Error_کد::EC_ERROR) أرجع "%Error: ";
    إذا (isError(کد, supp)) أرجع "%Error-" + std::string{کد.ascii()} + ": ";
    أرجع "%Warning-" + std::string{کد.ascii()} + ": ";
}

فراغ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    إذا (V3Error::debugDefault()) {
        std::cerr << پیام_پیشوند() << "Aborting since under --debug" << پایان_سطر;
        V3Error::vlAbort();
    }

#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
    إذا (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#وإلا
    std::خروج(1);
#پایان_اگر
}

string V3ErrorGuarded::warn_بیشتر_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_پیام.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    أرجع string(پیام_پیشوند().اندازه(), ' ');
}

فراغ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
    V3Stats::جمع_کن_Stat_جمع("Warnings, Suppressed "s + error_کد().ascii(), 1);
#پایان_اگر
    errorSuppressed(درست);
}

فراغ V3ErrorGuarded::v3errorPrep(V3_Error_کد کد) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_پیام.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_پیام.init(کد);
    m_errorContexted = خطأ;
    m_errorSuppressed = خطأ;
}

فراغ V3ErrorGuarded::v3error_انتها(ثابت std::ostringstream& sstr, ثابت string& extra,
                                File_سطر* fileline) VL_REQUIRES(m_mutex) {
    ساكن منطقي s_firedTooMany = خطأ;
    v3error_انتها_Guts(sstr, extra, fileline);
    m_پیام.clear();
    إذا (error_حد() && error_شمار() >= error_حد() && !s_firedTooMany) {
        s_firedTooMany = درست;
        // Recurses here
        v3error_انتها((v3errorPrep(V3_Error_کد::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_شمار() << std::پایان_سطر),
                    v3errorStr()),
                   "", اشاره_تهی);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
فراغ V3ErrorGuarded::v3error_انتها_Guts(ثابت std::ostringstream& sstr, ثابت string& extra,
                                    File_سطر* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#إذا defined(__COVERITY__) || defined(__cppcheck__)
    إذا (m_error_کد == V3_Error_کد::EC_FATAL) __coverity_panic__(x);
#پایان_اگر

    UASSERT_STATIC(!m_پیام.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_پیام.fileline(fileline);

    // Skip suppressed messages
    إذا (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        إذا (m_پیام.کد().defaultsOff()) أرجع;
        إذا (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_پیام.کد()))) أرجع;
        m_showedSuppressed.مجموعه(m_پیام.کد(), درست);
    }
    string پیام_2
        = V3Error::warnContextBegin() + پیام_پیشوند() + V3Error::warn_Context_انتها() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string اولی_سطر = پیام_2;
    {
        ثابت string::اندازه_type pos = اولی_سطر.پیدا_کن('\n');
        إذا (pos != string::npos) {
            اولی_سطر.erase(pos, اولی_سطر.درازی() - pos);
            اولی_سطر += "...";
        }
    }
    إذا (m_errorSuppressed) پیام_2 = اولی_سطر;
    // Suppress duplicate messages
    إذا (!m_messages.درج(اولی_سطر).دومی) أرجع;

    پیام_2 = VString::replaceSubstr(پیام_2, V3Error::warn_بیشتر(), warn_بیشتر_Spaces());

    string پیام_additional;
    {
        ثابت string::اندازه_type pos = پیام_2.پیدا_کن(V3Error::warnAdditionalInfo());
        إذا (pos != string::npos) {
            پیام_additional = پیام_2.substr(pos + V3Error::warnAdditionalInfo().اندازه());
            پیام_2.erase(pos);
        }
    }
    پیام_2 += '\n';  // Trailing newlines generally not put on messages so add
    إذا (!extra.خالی() && !m_errorSuppressed) {
        string extra_پیام = VString::replaceSubstr(extra, V3Error::warn_بیشتر(), warn_بیشتر_Spaces());
        extra_پیام = warn_بیشتر_Spaces() + extra_پیام + "\n";
        ثابت اندازه_t pos = پیام_2.پیدا_کن('\n');
        پیام_2.درج(pos + 1, extra_پیام);
    }
    // Output
    string متن;
    إذا (
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_پیام.کد() == V3_Error_کد::EC_FATALMANY)
#وإلا
        درست
#پایان_اگر
    )
        متن += پیام_2;
    إذا (m_errorSuppressed || m_پیام.کد().severityInfo()) {
        std::cerr << V3Error::strip_Meta_متن(متن, خطأ);
        m_پیام.متن(متن);
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
        V3DiagSarif::افزودن_پیام(m_پیام);
#پایان_اگر
    } وإلا {
        ثابت منطقي anError = isError(m_پیام.کد(), m_errorSuppressed);
        إذا (m_پیام.کد()
                != V3_Error_کد::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_پیام.کد())) {
            m_describedEachWarn.مجموعه(m_پیام.کد(), درست);
            إذا (m_پیام.کد() >= V3_Error_کد::EC_اولی_NAMED) {
                متن += warn_بیشتر_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_پیام.کد().url() + '\n';
            } وإلا إذا (m_err_شمار >= 1 && m_پیام.کد().severityFatal() && !m_tell_داخلی) {
                m_tell_داخلی = درست;
                متن += warn_بیشتر_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } وإلا إذا (!m_tellManual) {
                m_tellManual = درست;
                متن += warn_بیشتر_Spaces() + "... See the manual at " + m_پیام.کد().url()
                        + " for more assistance.\n";
            }
            إذا (!m_pretendError.test(m_پیام.کد()) && !m_پیام.کد().hardError()) {
                متن += warn_بیشتر_Spaces() + "... Use \"/* verilator lint_off "
                        + m_پیام.کد().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                إذا (m_پیام.کد().dangerous()) {
                    متن += warn_بیشتر_Spaces() + "*** See " + m_پیام.کد().url()
                            + " before disabling this,\n";
                    متن += warn_بیشتر_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        إذا (!پیام_additional.خالی()) متن += پیام_additional;
        std::cerr << V3Error::strip_Meta_متن(متن, خطأ);
        m_پیام.متن(متن);
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
        V3DiagSarif::افزودن_پیام(m_پیام);
#پایان_اگر

        إذا (anError) {
            incErrors();
        } وإلا {
            incWarnings();
        }
        إذا (m_پیام.کد().severityFatal()) {
            ساكن منطقي s_درون_Fatal = خطأ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            إذا (!s_درون_Fatal) {
                s_درون_Fatal = درست;
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
                إذا (dump_درخت_Level() || dump_درخت_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(درست);
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
#پایان_اگر
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
    لكل (صحيح i = 0; i < V3_Error_کد::_ENUM_MAX; i++) {
        ثابت V3_Error_کد کد{i};
        describedEachWarn(کد, خطأ);
        pretendError(کد, کد.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    أكد(std::string{V3_Error_کد{V3_Error_کد::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::سطر_Str(ثابت حرف* filename, صحيح lineno) VL_خالص {
    std::ostringstream بیرون;
    بیرون << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    بیرون << std::string(std::max<صحيح>(0, 20 - static_cast<صحيح>(بیرون.str().درازی())), ' ');
    أرجع بیرون.str();
}

string V3Error::strip_Meta_متن(ثابت string& متن, منطقي stripContext) VL_خالص {
    string نتیجه;
    نتیجه.reserve(متن.اندازه());
    صحيح درون_Begins = 0;
    لكل (string::اندازه_type pos = 0; pos < متن.اندازه();) {
        // string::starts_with is C++20
        إذا (0 == متن.compare(pos, std::طول_رشته("__WARN"), "__WARN")) {
            إذا (0 == متن.compare(pos, std::طول_رشته("__WARNRELATED("), "__WARNRELATED(")) {
                طالما (pos < متن.اندازه() && متن[pos] != ')') ++pos;
                إذا (pos < متن.اندازه() && متن[pos] == ')') ++pos;
                استمر;
            }
            إذا (0
                == متن.compare(pos, std::طول_رشته(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::طول_رشته(V3Error::WARN_CONTEXT_BEGIN);
                ++درون_Begins;
                استمر;
            }
            إذا (0
                == متن.compare(pos, std::طول_رشته(V3Error::WARN_CONTEXT_انتها),
                                V3Error::WARN_CONTEXT_انتها)) {
                pos += std::طول_رشته(V3Error::WARN_CONTEXT_انتها);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --درون_Begins;
                استمر;
            }
        }
        إذا (!stripContext || درون_Begins == 0) {
            // Remove double newlines
            إذا (!(متن[pos] == '\n' && !نتیجه.خالی() && نتیجه[نتیجه.اندازه() - 1] == '\n'))
                نتیجه += متن[pos];
        }
        ++pos;
    }
    أرجع نتیجه;
}

فراغ V3Error::abortIfWarnings() {
    إذا (!isErrorOrWarn()) أرجع;
    ثابت منطقي exwarn = warnFatal() && warn_شمار();
    إذا (error_شمار() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_شمار() << " error(s), "  //
                                      << V3Error::s().warn_شمار() << " warning(s)\n");
    } وإلا إذا (error_شمار()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_شمار() << " error(s)\n");
    } وإلا {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_شمار() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

فراغ V3Error::vlAbort() {
#اگر_تعریف_نشده V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#پایان_اگر
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_کد کد) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.قفل_2();
    s().v3errorPrep(کد);
    أرجع v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_سطر(V3_Error_کد کد, ثابت حرف* file, صحيح سطر)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(کد) << file << ":" << std::dec << سطر << ": ";
    أرجع v3errorStr();
}
فراغ V3Error::v3error_انتها(ثابت std::ostringstream& sstr, ثابت string& extra, File_سطر* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_انتها(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

فراغ v3error_انتها(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_انتها(sstr, "", اشاره_تهی);
}
فراغ v3error_انتها_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_انتها(sstr, "", اشاره_تهی);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
