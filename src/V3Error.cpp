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
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#د_که_پای
#ضمن <thread>
// clang-format on

//======================================================================

بنية v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_کوډ::V3_Error_کوډ(ثابت حرف* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3_Error_کوډ::EC_MIN; codei < V3_Error_کوډ::_ENUM_MAX; codei++) {
        ثابت V3_Error_کوډ کوډ{codei};
        إذا (0 == VL_STRCASECMP(msgp, کوډ.ascii())) {
            m_e = کوډ;
            إذا (isRenamed()) m_e = renamed_منزل().m_e;
            أرجع;
        }
    }
    m_e = V3_Error_کوډ::EC_ERROR;
}

string V3_Error_کوډ::url() ثابت {
    إذا (!isNamed())
        أرجع "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_نسخه_شمېره_STRING;
    أرجع "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_نسخه_شمېره_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

منطقي V3ErrorGuarded::isError(V3_Error_کوډ کوډ, منطقي supp) VL_REQUIRES(m_mutex) {
    إذا (کوډ.hardError()) أرجع ریښتیا;
    إذا (supp) أرجع خطأ;
    إذا (کوډ.severityInfo()) أرجع خطأ;
    إذا (pretendError(کوډ)) أرجع ریښتیا;
    أرجع خطأ;
}

string V3ErrorGuarded::پیغام_مختاړی() VL_REQUIRES(m_mutex) {
    ثابت V3_Error_کوډ کوډ = m_پیغام.کوډ();
    ثابت منطقي supp = m_errorSuppressed;
    إذا (supp) أرجع "-arning-suppressed-" + std::string{کوډ.ascii()} + ": ";
    إذا (کوډ.severityInfo()) أرجع "-Info: ";
    إذا (کوډ == V3_Error_کوډ::EC_FATAL) أرجع "%Error: ";
    إذا (کوډ == V3_Error_کوډ::EC_FATALMANY) أرجع "%Error: ";
    إذا (کوډ == V3_Error_کوډ::EC_FATALSRC) أرجع "%Error: Internal Error: ";
    إذا (کوډ == V3_Error_کوډ::EC_ERROR) أرجع "%Error: ";
    إذا (isError(کوډ, supp)) أرجع "%Error-" + std::string{کوډ.ascii()} + ": ";
    أرجع "%Warning-" + std::string{کوډ.ascii()} + ": ";
}

فراغ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    إذا (V3Error::debugDefault()) {
        std::cerr << پیغام_مختاړی() << "Aborting since under --debug" << کرښه_پای;
        V3Error::vlAbort();
    }

#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
    إذا (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#وإلا
    std::وتل(1);
#د_که_پای
}

string V3ErrorGuarded::warn_نور_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_پیغام.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    أرجع string(پیغام_مختاړی().اندازه(), ' ');
}

فراغ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
    V3Stats::جمع_کړه_Stat_مجموعه("Warnings, Suppressed "s + error_کوډ().ascii(), 1);
#د_که_پای
    errorSuppressed(ریښتیا);
}

فراغ V3ErrorGuarded::v3errorPrep(V3_Error_کوډ کوډ) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_پیغام.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_پیغام.init(کوډ);
    m_errorContexted = خطأ;
    m_errorSuppressed = خطأ;
}

فراغ V3ErrorGuarded::v3error_پای_ټکی(ثابت std::ostringstream& sstr, ثابت string& extra,
                                File_کرښه* fileline) VL_REQUIRES(m_mutex) {
    ساكن منطقي s_firedTooMany = خطأ;
    v3error_پای_ټکی_Guts(sstr, extra, fileline);
    m_پیغام.clear();
    إذا (error_حد() && error_شمېر() >= error_حد() && !s_firedTooMany) {
        s_firedTooMany = ریښتیا;
        // Recurses here
        v3error_پای_ټکی((v3errorPrep(V3_Error_کوډ::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_شمېر() << std::کرښه_پای),
                    v3errorStr()),
                   "", تش_اشاره);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
فراغ V3ErrorGuarded::v3error_پای_ټکی_Guts(ثابت std::ostringstream& sstr, ثابت string& extra,
                                    File_کرښه* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#إذا defined(__COVERITY__) || defined(__cppcheck__)
    إذا (m_error_کوډ == V3_Error_کوډ::EC_FATAL) __coverity_panic__(x);
#د_که_پای

    UASSERT_STATIC(!m_پیغام.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_پیغام.fileline(fileline);

    // Skip suppressed messages
    إذا (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        إذا (m_پیغام.کوډ().defaultsOff()) أرجع;
        إذا (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_پیغام.کوډ()))) أرجع;
        m_showedSuppressed.سېټ(m_پیغام.کوډ(), ریښتیا);
    }
    string پیغام_2
        = V3Error::warnContextBegin() + پیغام_مختاړی() + V3Error::warn_Context_پای_ټکی() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string لومړی_کرښه = پیغام_2;
    {
        ثابت string::اندازه_type pos = لومړی_کرښه.پیدا_کړه('\n');
        إذا (pos != string::npos) {
            لومړی_کرښه.erase(pos, لومړی_کرښه.اوږدتیا() - pos);
            لومړی_کرښه += "...";
        }
    }
    إذا (m_errorSuppressed) پیغام_2 = لومړی_کرښه;
    // Suppress duplicate messages
    إذا (!m_messages.ورننباسه(لومړی_کرښه).دوهم) أرجع;

    پیغام_2 = VString::replaceSubstr(پیغام_2, V3Error::warn_نور(), warn_نور_Spaces());

    string پیغام_additional;
    {
        ثابت string::اندازه_type pos = پیغام_2.پیدا_کړه(V3Error::warnAdditionalInfo());
        إذا (pos != string::npos) {
            پیغام_additional = پیغام_2.substr(pos + V3Error::warnAdditionalInfo().اندازه());
            پیغام_2.erase(pos);
        }
    }
    پیغام_2 += '\n';  // Trailing newlines generally not put on messages so add
    إذا (!extra.تشه() && !m_errorSuppressed) {
        string extra_پیغام = VString::replaceSubstr(extra, V3Error::warn_نور(), warn_نور_Spaces());
        extra_پیغام = warn_نور_Spaces() + extra_پیغام + "\n";
        ثابت اندازه_t pos = پیغام_2.پیدا_کړه('\n');
        پیغام_2.ورننباسه(pos + 1, extra_پیغام);
    }
    // Output
    string متن;
    إذا (
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_پیغام.کوډ() == V3_Error_کوډ::EC_FATALMANY)
#وإلا
        ریښتیا
#د_که_پای
    )
        متن += پیغام_2;
    إذا (m_errorSuppressed || m_پیغام.کوډ().severityInfo()) {
        std::cerr << V3Error::strip_Meta_متن(متن, خطأ);
        m_پیغام.متن(متن);
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
        V3DiagSarif::ټېل_پیغام(m_پیغام);
#د_که_پای
    } وإلا {
        ثابت منطقي anError = isError(m_پیغام.کوډ(), m_errorSuppressed);
        إذا (m_پیغام.کوډ()
                != V3_Error_کوډ::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_پیغام.کوډ())) {
            m_describedEachWarn.سېټ(m_پیغام.کوډ(), ریښتیا);
            إذا (m_پیغام.کوډ() >= V3_Error_کوډ::EC_لومړی_NAMED) {
                متن += warn_نور_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_پیغام.کوډ().url() + '\n';
            } وإلا إذا (m_err_شمېر >= 1 && m_پیغام.کوډ().severityFatal() && !m_tell_دننی) {
                m_tell_دننی = ریښتیا;
                متن += warn_نور_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } وإلا إذا (!m_tellManual) {
                m_tellManual = ریښتیا;
                متن += warn_نور_Spaces() + "... See the manual at " + m_پیغام.کوډ().url()
                        + " for more assistance.\n";
            }
            إذا (!m_pretendError.test(m_پیغام.کوډ()) && !m_پیغام.کوډ().hardError()) {
                متن += warn_نور_Spaces() + "... Use \"/* verilator lint_off "
                        + m_پیغام.کوډ().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                إذا (m_پیغام.کوډ().dangerous()) {
                    متن += warn_نور_Spaces() + "*** See " + m_پیغام.کوډ().url()
                            + " before disabling this,\n";
                    متن += warn_نور_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        إذا (!پیغام_additional.تشه()) متن += پیغام_additional;
        std::cerr << V3Error::strip_Meta_متن(متن, خطأ);
        m_پیغام.متن(متن);
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
        V3DiagSarif::ټېل_پیغام(m_پیغام);
#د_که_پای

        إذا (anError) {
            incErrors();
        } وإلا {
            incWarnings();
        }
        إذا (m_پیغام.کوډ().severityFatal()) {
            ساكن منطقي s_دننه_Fatal = خطأ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            إذا (!s_دننه_Fatal) {
                s_دننه_Fatal = ریښتیا;
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
                إذا (dump_ونه_Level() || dump_ونه_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(ریښتیا);
                    إذا (dump_ونه_Level()) {
                        v3Global.rootp()->dump_ونه_File(v3Global.debugFilename("final.tree", 990));
                    }
                    إذا (dump_ونه_Json_Level()) {
                        v3Global.rootp()->dump_ونه_Json_File(
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
#د_که_پای
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
    لكل (صحيح i = 0; i < V3_Error_کوډ::_ENUM_MAX; i++) {
        ثابت V3_Error_کوډ کوډ{i};
        describedEachWarn(کوډ, خطأ);
        pretendError(کوډ, کوډ.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    أكد(std::string{V3_Error_کوډ{V3_Error_کوډ::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::کرښه_Str(ثابت حرف* filename, صحيح lineno) VL_خالص {
    std::ostringstream بهر;
    بهر << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    بهر << std::string(std::max<صحيح>(0, 20 - static_cast<صحيح>(بهر.str().اوږدتیا())), ' ');
    أرجع بهر.str();
}

string V3Error::strip_Meta_متن(ثابت string& متن, منطقي stripContext) VL_خالص {
    string پایله;
    پایله.reserve(متن.اندازه());
    صحيح دننه_Begins = 0;
    لكل (string::اندازه_type pos = 0; pos < متن.اندازه();) {
        // string::starts_with is C++20
        إذا (0 == متن.compare(pos, std::مزي_اوږدوالی("__WARN"), "__WARN")) {
            إذا (0 == متن.compare(pos, std::مزي_اوږدوالی("__WARNRELATED("), "__WARNRELATED(")) {
                طالما (pos < متن.اندازه() && متن[pos] != ')') ++pos;
                إذا (pos < متن.اندازه() && متن[pos] == ')') ++pos;
                استمر;
            }
            إذا (0
                == متن.compare(pos, std::مزي_اوږدوالی(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::مزي_اوږدوالی(V3Error::WARN_CONTEXT_BEGIN);
                ++دننه_Begins;
                استمر;
            }
            إذا (0
                == متن.compare(pos, std::مزي_اوږدوالی(V3Error::WARN_CONTEXT_پای_ټکی),
                                V3Error::WARN_CONTEXT_پای_ټکی)) {
                pos += std::مزي_اوږدوالی(V3Error::WARN_CONTEXT_پای_ټکی);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --دننه_Begins;
                استمر;
            }
        }
        إذا (!stripContext || دننه_Begins == 0) {
            // Remove double newlines
            إذا (!(متن[pos] == '\n' && !پایله.تشه() && پایله[پایله.اندازه() - 1] == '\n'))
                پایله += متن[pos];
        }
        ++pos;
    }
    أرجع پایله;
}

فراغ V3Error::abortIfWarnings() {
    إذا (!isErrorOrWarn()) أرجع;
    ثابت منطقي exwarn = warnFatal() && warn_شمېر();
    إذا (error_شمېر() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_شمېر() << " error(s), "  //
                                      << V3Error::s().warn_شمېر() << " warning(s)\n");
    } وإلا إذا (error_شمېر()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_شمېر() << " error(s)\n");
    } وإلا {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_شمېر() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

فراغ V3Error::vlAbort() {
#که_نه_تعریف_شوی V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#د_که_پای
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_کوډ کوډ) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.قلف_2();
    s().v3errorPrep(کوډ);
    أرجع v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_کرښه(V3_Error_کوډ کوډ, ثابت حرف* file, صحيح کرښه)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(کوډ) << file << ":" << std::dec << کرښه << ": ";
    أرجع v3errorStr();
}
فراغ V3Error::v3error_پای_ټکی(ثابت std::ostringstream& sstr, ثابت string& extra, File_کرښه* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_پای_ټکی(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

فراغ v3error_پای_ټکی(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_پای_ټکی(sstr, "", تش_اشاره);
}
فراغ v3error_پای_ټکی_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_پای_ټکی(sstr, "", تش_اشاره);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
