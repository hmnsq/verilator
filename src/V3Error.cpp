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
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#جيڪڏهن_پورو
#ضمن <thread>
// clang-format on

//======================================================================

بنية v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_ڪوڊ::V3_Error_ڪوڊ(ثابت حرف* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3_Error_ڪوڊ::EC_MIN; codei < V3_Error_ڪوڊ::_ENUM_MAX; codei++) {
        ثابت V3_Error_ڪوڊ ڪوڊ{codei};
        إذا (0 == VL_STRCASECMP(msgp, ڪوڊ.ascii())) {
            m_e = ڪوڊ;
            إذا (isRenamed()) m_e = renamed_منزل().m_e;
            أرجع;
        }
    }
    m_e = V3_Error_ڪوڊ::EC_ERROR;
}

string V3_Error_ڪوڊ::url() ثابت {
    إذا (!isNamed())
        أرجع "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_نسخو_عدد_STRING;
    أرجع "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_نسخو_عدد_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

منطقي V3ErrorGuarded::isError(V3_Error_ڪوڊ ڪوڊ, منطقي supp) VL_REQUIRES(m_mutex) {
    إذا (ڪوڊ.hardError()) أرجع سچ;
    إذا (supp) أرجع خطأ;
    إذا (ڪوڊ.severityInfo()) أرجع خطأ;
    إذا (pretendError(ڪوڊ)) أرجع سچ;
    أرجع خطأ;
}

string V3ErrorGuarded::نياپو_اڳياڙي() VL_REQUIRES(m_mutex) {
    ثابت V3_Error_ڪوڊ ڪوڊ = m_پيغام.ڪوڊ();
    ثابت منطقي supp = m_errorSuppressed;
    إذا (supp) أرجع "-arning-suppressed-" + std::string{ڪوڊ.ascii()} + ": ";
    إذا (ڪوڊ.severityInfo()) أرجع "-Info: ";
    إذا (ڪوڊ == V3_Error_ڪوڊ::EC_FATAL) أرجع "%Error: ";
    إذا (ڪوڊ == V3_Error_ڪوڊ::EC_FATALMANY) أرجع "%Error: ";
    إذا (ڪوڊ == V3_Error_ڪوڊ::EC_FATALSRC) أرجع "%Error: Internal Error: ";
    إذا (ڪوڊ == V3_Error_ڪوڊ::EC_ERROR) أرجع "%Error: ";
    إذا (isError(ڪوڊ, supp)) أرجع "%Error-" + std::string{ڪوڊ.ascii()} + ": ";
    أرجع "%Warning-" + std::string{ڪوڊ.ascii()} + ": ";
}

فراغ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    إذا (V3Error::debugDefault()) {
        std::cerr << نياپو_اڳياڙي() << "Aborting since under --debug" << سٽ_پڄاڻي;
        V3Error::vlAbort();
    }

#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
    إذا (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#وإلا
    std::ٻاهر_نڪرو(1);
#جيڪڏهن_پورو
}

string V3ErrorGuarded::warn_وڌيڪ_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_پيغام.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    أرجع string(نياپو_اڳياڙي().سائيز(), ' ');
}

فراغ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
    V3Stats::جوڙيو_Stat_جوڙ("Warnings, Suppressed "s + error_ڪوڊ().ascii(), 1);
#جيڪڏهن_پورو
    errorSuppressed(سچ);
}

فراغ V3ErrorGuarded::v3errorPrep(V3_Error_ڪوڊ ڪوڊ) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_پيغام.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_پيغام.init(ڪوڊ);
    m_errorContexted = خطأ;
    m_errorSuppressed = خطأ;
}

فراغ V3ErrorGuarded::v3error_پڇاڙي(ثابت std::ostringstream& sstr, ثابت string& extra,
                                File_سٽ* fileline) VL_REQUIRES(m_mutex) {
    ساكن منطقي s_firedTooMany = خطأ;
    v3error_پڇاڙي_Guts(sstr, extra, fileline);
    m_پيغام.clear();
    إذا (error_حد_بندي() && error_ڳڻپ() >= error_حد_بندي() && !s_firedTooMany) {
        s_firedTooMany = سچ;
        // Recurses here
        v3error_پڇاڙي((v3errorPrep(V3_Error_ڪوڊ::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_ڳڻپ() << std::سٽ_پڄاڻي),
                    v3errorStr()),
                   "", خالي_اشارو);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
فراغ V3ErrorGuarded::v3error_پڇاڙي_Guts(ثابت std::ostringstream& sstr, ثابت string& extra,
                                    File_سٽ* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#إذا defined(__COVERITY__) || defined(__cppcheck__)
    إذا (m_error_ڪوڊ == V3_Error_ڪوڊ::EC_FATAL) __coverity_panic__(x);
#جيڪڏهن_پورو

    UASSERT_STATIC(!m_پيغام.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_پيغام.fileline(fileline);

    // Skip suppressed messages
    إذا (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        إذا (m_پيغام.ڪوڊ().defaultsOff()) أرجع;
        إذا (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_پيغام.ڪوڊ()))) أرجع;
        m_showedSuppressed.سيٽ(m_پيغام.ڪوڊ(), سچ);
    }
    string نياپو
        = V3Error::warnContextBegin() + نياپو_اڳياڙي() + V3Error::warn_Context_پڇاڙي() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string پهريون_سٽ = نياپو;
    {
        ثابت string::سائيز_type pos = پهريون_سٽ.ڳوليو('\n');
        إذا (pos != string::npos) {
            پهريون_سٽ.erase(pos, پهريون_سٽ.ڊيگهائي() - pos);
            پهريون_سٽ += "...";
        }
    }
    إذا (m_errorSuppressed) نياپو = پهريون_سٽ;
    // Suppress duplicate messages
    إذا (!m_messages.وجهو(پهريون_سٽ).ٻيو) أرجع;

    نياپو = VString::replaceSubstr(نياپو, V3Error::warn_وڌيڪ(), warn_وڌيڪ_Spaces());

    string نياپو_additional;
    {
        ثابت string::سائيز_type pos = نياپو.ڳوليو(V3Error::warnAdditionalInfo());
        إذا (pos != string::npos) {
            نياپو_additional = نياپو.substr(pos + V3Error::warnAdditionalInfo().سائيز());
            نياپو.erase(pos);
        }
    }
    نياپو += '\n';  // Trailing newlines generally not put on messages so add
    إذا (!extra.خالي_جايون() && !m_errorSuppressed) {
        string extra_نياپو = VString::replaceSubstr(extra, V3Error::warn_وڌيڪ(), warn_وڌيڪ_Spaces());
        extra_نياپو = warn_وڌيڪ_Spaces() + extra_نياپو + "\n";
        ثابت سائيز_t pos = نياپو.ڳوليو('\n');
        نياپو.وجهو(pos + 1, extra_نياپو);
    }
    // Output
    string متن;
    إذا (
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_پيغام.ڪوڊ() == V3_Error_ڪوڊ::EC_FATALMANY)
#وإلا
        سچ
#جيڪڏهن_پورو
    )
        متن += نياپو;
    إذا (m_errorSuppressed || m_پيغام.ڪوڊ().severityInfo()) {
        std::cerr << V3Error::strip_Meta_متن(متن, خطأ);
        m_پيغام.متن(متن);
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
        V3DiagSarif::ڌڪو_پيغام(m_پيغام);
#جيڪڏهن_پورو
    } وإلا {
        ثابت منطقي anError = isError(m_پيغام.ڪوڊ(), m_errorSuppressed);
        إذا (m_پيغام.ڪوڊ()
                != V3_Error_ڪوڊ::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_پيغام.ڪوڊ())) {
            m_describedEachWarn.سيٽ(m_پيغام.ڪوڊ(), سچ);
            إذا (m_پيغام.ڪوڊ() >= V3_Error_ڪوڊ::EC_پهريون_NAMED) {
                متن += warn_وڌيڪ_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_پيغام.ڪوڊ().url() + '\n';
            } وإلا إذا (m_err_ڳڻپ >= 1 && m_پيغام.ڪوڊ().severityFatal() && !m_tell_اندروني) {
                m_tell_اندروني = سچ;
                متن += warn_وڌيڪ_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } وإلا إذا (!m_tellManual) {
                m_tellManual = سچ;
                متن += warn_وڌيڪ_Spaces() + "... See the manual at " + m_پيغام.ڪوڊ().url()
                        + " for more assistance.\n";
            }
            إذا (!m_pretendError.test(m_پيغام.ڪوڊ()) && !m_پيغام.ڪوڊ().hardError()) {
                متن += warn_وڌيڪ_Spaces() + "... Use \"/* verilator lint_off "
                        + m_پيغام.ڪوڊ().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                إذا (m_پيغام.ڪوڊ().dangerous()) {
                    متن += warn_وڌيڪ_Spaces() + "*** See " + m_پيغام.ڪوڊ().url()
                            + " before disabling this,\n";
                    متن += warn_وڌيڪ_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        إذا (!نياپو_additional.خالي_جايون()) متن += نياپو_additional;
        std::cerr << V3Error::strip_Meta_متن(متن, خطأ);
        m_پيغام.متن(متن);
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
        V3DiagSarif::ڌڪو_پيغام(m_پيغام);
#جيڪڏهن_پورو

        إذا (anError) {
            incErrors();
        } وإلا {
            incWarnings();
        }
        إذا (m_پيغام.ڪوڊ().severityFatal()) {
            ساكن منطقي s_اندريون_Fatal = خطأ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            إذا (!s_اندريون_Fatal) {
                s_اندريون_Fatal = سچ;
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
                إذا (dump_وڻ_Level() || dump_وڻ_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(سچ);
                    إذا (dump_وڻ_Level()) {
                        v3Global.rootp()->dump_وڻ_File(v3Global.debugFilename("final.tree", 990));
                    }
                    إذا (dump_وڻ_Json_Level()) {
                        v3Global.rootp()->dump_وڻ_Json_File(
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
#جيڪڏهن_پورو
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
    لكل (صحيح i = 0; i < V3_Error_ڪوڊ::_ENUM_MAX; i++) {
        ثابت V3_Error_ڪوڊ ڪوڊ{i};
        describedEachWarn(ڪوڊ, خطأ);
        pretendError(ڪوڊ, ڪوڊ.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    أكد(std::string{V3_Error_ڪوڊ{V3_Error_ڪوڊ::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::سٽ_Str(ثابت حرف* filename, صحيح lineno) VL_خالص {
    std::ostringstream ٻاهريون;
    ٻاهريون << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    ٻاهريون << std::string(std::max<صحيح>(0, 20 - static_cast<صحيح>(ٻاهريون.str().ڊيگهائي())), ' ');
    أرجع ٻاهريون.str();
}

string V3Error::strip_Meta_متن(ثابت string& متن, منطقي stripContext) VL_خالص {
    string نتيجو;
    نتيجو.reserve(متن.سائيز());
    صحيح اندريون_Begins = 0;
    لكل (string::سائيز_type pos = 0; pos < متن.سائيز();) {
        // string::starts_with is C++20
        إذا (0 == متن.compare(pos, std::لڙي_ڊيگهه("__WARN"), "__WARN")) {
            إذا (0 == متن.compare(pos, std::لڙي_ڊيگهه("__WARNRELATED("), "__WARNRELATED(")) {
                طالما (pos < متن.سائيز() && متن[pos] != ')') ++pos;
                إذا (pos < متن.سائيز() && متن[pos] == ')') ++pos;
                استمر;
            }
            إذا (0
                == متن.compare(pos, std::لڙي_ڊيگهه(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::لڙي_ڊيگهه(V3Error::WARN_CONTEXT_BEGIN);
                ++اندريون_Begins;
                استمر;
            }
            إذا (0
                == متن.compare(pos, std::لڙي_ڊيگهه(V3Error::WARN_CONTEXT_پڇاڙي),
                                V3Error::WARN_CONTEXT_پڇاڙي)) {
                pos += std::لڙي_ڊيگهه(V3Error::WARN_CONTEXT_پڇاڙي);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --اندريون_Begins;
                استمر;
            }
        }
        إذا (!stripContext || اندريون_Begins == 0) {
            // Remove double newlines
            إذا (!(متن[pos] == '\n' && !نتيجو.خالي_جايون() && نتيجو[نتيجو.سائيز() - 1] == '\n'))
                نتيجو += متن[pos];
        }
        ++pos;
    }
    أرجع نتيجو;
}

فراغ V3Error::abortIfWarnings() {
    إذا (!isErrorOrWarn()) أرجع;
    ثابت منطقي exwarn = warnFatal() && warn_ڳڻپ();
    إذا (error_ڳڻپ() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ڳڻپ() << " error(s), "  //
                                      << V3Error::s().warn_ڳڻپ() << " warning(s)\n");
    } وإلا إذا (error_ڳڻپ()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ڳڻپ() << " error(s)\n");
    } وإلا {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_ڳڻپ() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

فراغ V3Error::vlAbort() {
#جيڪڏهن_وصف_نه_ٿيل V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#جيڪڏهن_پورو
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_ڪوڊ ڪوڊ) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.تالو_2();
    s().v3errorPrep(ڪوڊ);
    أرجع v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_سٽ(V3_Error_ڪوڊ ڪوڊ, ثابت حرف* file, صحيح سٽ)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(ڪوڊ) << file << ":" << std::dec << سٽ << ": ";
    أرجع v3errorStr();
}
فراغ V3Error::v3error_پڇاڙي(ثابت std::ostringstream& sstr, ثابت string& extra, File_سٽ* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_پڇاڙي(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

فراغ v3error_پڇاڙي(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_پڇاڙي(sstr, "", خالي_اشارو);
}
فراغ v3error_پڇاڙي_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_پڇاڙي(sstr, "", خالي_اشارو);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
