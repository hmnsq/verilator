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
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ <thread>
// clang-format on

//======================================================================

ਬਣਤਰ v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_ਕੋਡ::V3_Error_ਕੋਡ(ਅਚਲ ਅੱਖਰ* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    ਲਈ (ਪੂਰਨ codei = V3_Error_ਕੋਡ::EC_MIN; codei < V3_Error_ਕੋਡ::_ENUM_MAX; codei++) {
        ਅਚਲ V3_Error_ਕੋਡ ਕੋਡ{codei};
        ਜੇ (0 == VL_STRCASECMP(msgp, ਕੋਡ.ascii())) {
            m_e = ਕੋਡ;
            ਜੇ (isRenamed()) m_e = renamed_ਮੰਜ਼ਿਲ().m_e;
            ਵਾਪਸ;
        }
    }
    m_e = V3_Error_ਕੋਡ::EC_ERROR;
}

string V3_Error_ਕੋਡ::url() ਅਚਲ {
    ਜੇ (!isNamed())
        ਵਾਪਸ "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_ਸੰਸਕਰਣ_ਸੰਖਿਆ_STRING;
    ਵਾਪਸ "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_ਸੰਸਕਰਣ_ਸੰਖਿਆ_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

ਤਾਰਕਿਕ V3ErrorGuarded::isError(V3_Error_ਕੋਡ ਕੋਡ, ਤਾਰਕਿਕ supp) VL_REQUIRES(m_mutex) {
    ਜੇ (ਕੋਡ.hardError()) ਵਾਪਸ ਸੱਚ;
    ਜੇ (supp) ਵਾਪਸ ਝੂਠ;
    ਜੇ (ਕੋਡ.severityInfo()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (pretendError(ਕੋਡ)) ਵਾਪਸ ਸੱਚ;
    ਵਾਪਸ ਝੂਠ;
}

string V3ErrorGuarded::ਸੁਨੇਹਾ_ਅਗੇਤਰ() VL_REQUIRES(m_mutex) {
    ਅਚਲ V3_Error_ਕੋਡ ਕੋਡ = m_ਸੁਨੇਹਾ.ਕੋਡ();
    ਅਚਲ ਤਾਰਕਿਕ supp = m_errorSuppressed;
    ਜੇ (supp) ਵਾਪਸ "-arning-suppressed-" + std::string{ਕੋਡ.ascii()} + ": ";
    ਜੇ (ਕੋਡ.severityInfo()) ਵਾਪਸ "-Info: ";
    ਜੇ (ਕੋਡ == V3_Error_ਕੋਡ::EC_FATAL) ਵਾਪਸ "%Error: ";
    ਜੇ (ਕੋਡ == V3_Error_ਕੋਡ::EC_FATALMANY) ਵਾਪਸ "%Error: ";
    ਜੇ (ਕੋਡ == V3_Error_ਕੋਡ::EC_FATALSRC) ਵਾਪਸ "%Error: Internal Error: ";
    ਜੇ (ਕੋਡ == V3_Error_ਕੋਡ::EC_ERROR) ਵਾਪਸ "%Error: ";
    ਜੇ (isError(ਕੋਡ, supp)) ਵਾਪਸ "%Error-" + std::string{ਕੋਡ.ascii()} + ": ";
    ਵਾਪਸ "%Warning-" + std::string{ਕੋਡ.ascii()} + ": ";
}

ਖਾਲੀ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    ਜੇ (V3Error::debugDefault()) {
        std::cerr << ਸੁਨੇਹਾ_ਅਗੇਤਰ() << "Aborting since under --debug" << ਸਤਰ_ਅੰਤ;
        V3Error::vlAbort();
    }

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
    ਜੇ (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#ਨਹੀਂਤਾਂ
    std::ਬਾਹਰ_ਨਿਕਲੋ(1);
#ਜੇ_ਅੰਤ
}

string V3ErrorGuarded::warn_ਹੋਰ_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_ਸੁਨੇਹਾ.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    ਵਾਪਸ string(ਸੁਨੇਹਾ_ਅਗੇਤਰ().ਮਾਪ(), ' ');
}

ਖਾਲੀ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
    V3Stats::ਜੋੜੋ_Stat_ਜੋੜ("Warnings, Suppressed "s + error_ਕੋਡ().ascii(), 1);
#ਜੇ_ਅੰਤ
    errorSuppressed(ਸੱਚ);
}

ਖਾਲੀ V3ErrorGuarded::v3errorPrep(V3_Error_ਕੋਡ ਕੋਡ) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_ਸੁਨੇਹਾ.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_ਸੁਨੇਹਾ.init(ਕੋਡ);
    m_errorContexted = ਝੂਠ;
    m_errorSuppressed = ਝੂਠ;
}

ਖਾਲੀ V3ErrorGuarded::v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& sstr, ਅਚਲ string& extra,
                                File_ਸਤਰ* fileline) VL_REQUIRES(m_mutex) {
    ਸਥਿਰ ਤਾਰਕਿਕ s_firedTooMany = ਝੂਠ;
    v3error_ਸਿਰਾ_Guts(sstr, extra, fileline);
    m_ਸੁਨੇਹਾ.clear();
    ਜੇ (error_ਹੱਦਬੰਦੀ() && error_ਗਿਣਤੀ() >= error_ਹੱਦਬੰਦੀ() && !s_firedTooMany) {
        s_firedTooMany = ਸੱਚ;
        // Recurses here
        v3error_ਸਿਰਾ((v3errorPrep(V3_Error_ਕੋਡ::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_ਗਿਣਤੀ() << std::ਸਤਰ_ਅੰਤ),
                    v3errorStr()),
                   "", ਖਾਲੀ_ਸੂਚਕ);
        ਪ੍ਰਤਿਜ੍ਞਾ(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
ਖਾਲੀ V3ErrorGuarded::v3error_ਸਿਰਾ_Guts(ਅਚਲ std::ostringstream& sstr, ਅਚਲ string& extra,
                                    File_ਸਤਰ* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#ਜੇ defined(__COVERITY__) || defined(__cppcheck__)
    ਜੇ (m_error_ਕੋਡ == V3_Error_ਕੋਡ::EC_FATAL) __coverity_panic__(x);
#ਜੇ_ਅੰਤ

    UASSERT_STATIC(!m_ਸੁਨੇਹਾ.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_ਸੁਨੇਹਾ.fileline(fileline);

    // Skip suppressed messages
    ਜੇ (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        ਜੇ (m_ਸੁਨੇਹਾ.ਕੋਡ().defaultsOff()) ਵਾਪਸ;
        ਜੇ (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_ਸੁਨੇਹਾ.ਕੋਡ()))) ਵਾਪਸ;
        m_showedSuppressed.ਸੈੱਟ(m_ਸੁਨੇਹਾ.ਕੋਡ(), ਸੱਚ);
    }
    string ਸੁਨੇਹਾ_2
        = V3Error::warnContextBegin() + ਸੁਨੇਹਾ_ਅਗੇਤਰ() + V3Error::warn_Context_ਸਿਰਾ() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string ਪਹਿਲਾ_ਸਤਰ = ਸੁਨੇਹਾ_2;
    {
        ਅਚਲ string::ਮਾਪ_type pos = ਪਹਿਲਾ_ਸਤਰ.ਲੱਭੋ('\n');
        ਜੇ (pos != string::npos) {
            ਪਹਿਲਾ_ਸਤਰ.erase(pos, ਪਹਿਲਾ_ਸਤਰ.ਲੰਮਾਈ() - pos);
            ਪਹਿਲਾ_ਸਤਰ += "...";
        }
    }
    ਜੇ (m_errorSuppressed) ਸੁਨੇਹਾ_2 = ਪਹਿਲਾ_ਸਤਰ;
    // Suppress duplicate messages
    ਜੇ (!m_messages.ਪਾਓ(ਪਹਿਲਾ_ਸਤਰ).ਦੂਜਾ) ਵਾਪਸ;

    ਸੁਨੇਹਾ_2 = VString::replaceSubstr(ਸੁਨੇਹਾ_2, V3Error::warn_ਹੋਰ(), warn_ਹੋਰ_Spaces());

    string ਸੁਨੇਹਾ_additional;
    {
        ਅਚਲ string::ਮਾਪ_type pos = ਸੁਨੇਹਾ_2.ਲੱਭੋ(V3Error::warnAdditionalInfo());
        ਜੇ (pos != string::npos) {
            ਸੁਨੇਹਾ_additional = ਸੁਨੇਹਾ_2.substr(pos + V3Error::warnAdditionalInfo().ਮਾਪ());
            ਸੁਨੇਹਾ_2.erase(pos);
        }
    }
    ਸੁਨੇਹਾ_2 += '\n';  // Trailing newlines generally not put on messages so add
    ਜੇ (!extra.ਖਾਲੀ_ਥਾਵਾਂ() && !m_errorSuppressed) {
        string extra_ਸੁਨੇਹਾ = VString::replaceSubstr(extra, V3Error::warn_ਹੋਰ(), warn_ਹੋਰ_Spaces());
        extra_ਸੁਨੇਹਾ = warn_ਹੋਰ_Spaces() + extra_ਸੁਨੇਹਾ + "\n";
        ਅਚਲ ਮਾਪ_t pos = ਸੁਨੇਹਾ_2.ਲੱਭੋ('\n');
        ਸੁਨੇਹਾ_2.ਪਾਓ(pos + 1, extra_ਸੁਨੇਹਾ);
    }
    // Output
    string ਪਾਠ;
    ਜੇ (
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_ਸੁਨੇਹਾ.ਕੋਡ() == V3_Error_ਕੋਡ::EC_FATALMANY)
#ਨਹੀਂਤਾਂ
        ਸੱਚ
#ਜੇ_ਅੰਤ
    )
        ਪਾਠ += ਸੁਨੇਹਾ_2;
    ਜੇ (m_errorSuppressed || m_ਸੁਨੇਹਾ.ਕੋਡ().severityInfo()) {
        std::cerr << V3Error::strip_Meta_ਪਾਠ(ਪਾਠ, ਝੂਠ);
        m_ਸੁਨੇਹਾ.ਪਾਠ(ਪਾਠ);
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
        V3DiagSarif::ਧੱਕੋ_ਸੁਨੇਹਾ(m_ਸੁਨੇਹਾ);
#ਜੇ_ਅੰਤ
    } ਨਹੀਂਤਾਂ {
        ਅਚਲ ਤਾਰਕਿਕ anError = isError(m_ਸੁਨੇਹਾ.ਕੋਡ(), m_errorSuppressed);
        ਜੇ (m_ਸੁਨੇਹਾ.ਕੋਡ()
                != V3_Error_ਕੋਡ::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_ਸੁਨੇਹਾ.ਕੋਡ())) {
            m_describedEachWarn.ਸੈੱਟ(m_ਸੁਨੇਹਾ.ਕੋਡ(), ਸੱਚ);
            ਜੇ (m_ਸੁਨੇਹਾ.ਕੋਡ() >= V3_Error_ਕੋਡ::EC_ਪਹਿਲਾ_NAMED) {
                ਪਾਠ += warn_ਹੋਰ_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_ਸੁਨੇਹਾ.ਕੋਡ().url() + '\n';
            } ਨਹੀਂਤਾਂ ਜੇ (m_err_ਗਿਣਤੀ >= 1 && m_ਸੁਨੇਹਾ.ਕੋਡ().severityFatal() && !m_tell_ਅੰਦਰੂਨੀ) {
                m_tell_ਅੰਦਰੂਨੀ = ਸੱਚ;
                ਪਾਠ += warn_ਹੋਰ_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } ਨਹੀਂਤਾਂ ਜੇ (!m_tellManual) {
                m_tellManual = ਸੱਚ;
                ਪਾਠ += warn_ਹੋਰ_Spaces() + "... See the manual at " + m_ਸੁਨੇਹਾ.ਕੋਡ().url()
                        + " for more assistance.\n";
            }
            ਜੇ (!m_pretendError.test(m_ਸੁਨੇਹਾ.ਕੋਡ()) && !m_ਸੁਨੇਹਾ.ਕੋਡ().hardError()) {
                ਪਾਠ += warn_ਹੋਰ_Spaces() + "... Use \"/* verilator lint_off "
                        + m_ਸੁਨੇਹਾ.ਕੋਡ().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                ਜੇ (m_ਸੁਨੇਹਾ.ਕੋਡ().dangerous()) {
                    ਪਾਠ += warn_ਹੋਰ_Spaces() + "*** See " + m_ਸੁਨੇਹਾ.ਕੋਡ().url()
                            + " before disabling this,\n";
                    ਪਾਠ += warn_ਹੋਰ_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        ਜੇ (!ਸੁਨੇਹਾ_additional.ਖਾਲੀ_ਥਾਵਾਂ()) ਪਾਠ += ਸੁਨੇਹਾ_additional;
        std::cerr << V3Error::strip_Meta_ਪਾਠ(ਪਾਠ, ਝੂਠ);
        m_ਸੁਨੇਹਾ.ਪਾਠ(ਪਾਠ);
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
        V3DiagSarif::ਧੱਕੋ_ਸੁਨੇਹਾ(m_ਸੁਨੇਹਾ);
#ਜੇ_ਅੰਤ

        ਜੇ (anError) {
            incErrors();
        } ਨਹੀਂਤਾਂ {
            incWarnings();
        }
        ਜੇ (m_ਸੁਨੇਹਾ.ਕੋਡ().severityFatal()) {
            ਸਥਿਰ ਤਾਰਕਿਕ s_ਵਿੱਚ_Fatal = ਝੂਠ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            ਜੇ (!s_ਵਿੱਚ_Fatal) {
                s_ਵਿੱਚ_Fatal = ਸੱਚ;
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
                ਜੇ (dump_ਰੁੱਖ_Level() || dump_ਰੁੱਖ_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(ਸੱਚ);
                    ਜੇ (dump_ਰੁੱਖ_Level()) {
                        v3Global.rootp()->dump_ਰੁੱਖ_File(v3Global.debugFilename("final.tree", 990));
                    }
                    ਜੇ (dump_ਰੁੱਖ_Json_Level()) {
                        v3Global.rootp()->dump_ਰੁੱਖ_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    ਜੇ (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(ਝੂਠ);
#ਜੇ_ਅੰਤ
            }

            vlAbortOrExit();
        } ਨਹੀਂਤਾਂ ਜੇ (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

ਖਾਲੀ V3Error::init() {
    ਲਈ (ਪੂਰਨ i = 0; i < V3_Error_ਕੋਡ::_ENUM_MAX; i++) {
        ਅਚਲ V3_Error_ਕੋਡ ਕੋਡ{i};
        describedEachWarn(ਕੋਡ, ਝੂਠ);
        pretendError(ਕੋਡ, ਕੋਡ.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    ਪ੍ਰਤਿਜ੍ਞਾ(std::string{V3_Error_ਕੋਡ{V3_Error_ਕੋਡ::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::ਸਤਰ_Str(ਅਚਲ ਅੱਖਰ* filename, ਪੂਰਨ lineno) VL_ਸ਼ੁੱਧ {
    std::ostringstream ਬਾਹਰਲਾ;
    ਬਾਹਰਲਾ << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    ਬਾਹਰਲਾ << std::string(std::max<ਪੂਰਨ>(0, 20 - static_cast<ਪੂਰਨ>(ਬਾਹਰਲਾ.str().ਲੰਮਾਈ())), ' ');
    ਵਾਪਸ ਬਾਹਰਲਾ.str();
}

string V3Error::strip_Meta_ਪਾਠ(ਅਚਲ string& ਪਾਠ, ਤਾਰਕਿਕ stripContext) VL_ਸ਼ੁੱਧ {
    string ਨਤੀਜਾ;
    ਨਤੀਜਾ.reserve(ਪਾਠ.ਮਾਪ());
    ਪੂਰਨ ਵਿੱਚ_Begins = 0;
    ਲਈ (string::ਮਾਪ_type pos = 0; pos < ਪਾਠ.ਮਾਪ();) {
        // string::starts_with is C++20
        ਜੇ (0 == ਪਾਠ.compare(pos, std::ਲੜੀ_ਲੰਬਾਈ("__WARN"), "__WARN")) {
            ਜੇ (0 == ਪਾਠ.compare(pos, std::ਲੜੀ_ਲੰਬਾਈ("__WARNRELATED("), "__WARNRELATED(")) {
                ਜਦੋਂ (pos < ਪਾਠ.ਮਾਪ() && ਪਾਠ[pos] != ')') ++pos;
                ਜੇ (pos < ਪਾਠ.ਮਾਪ() && ਪਾਠ[pos] == ')') ++pos;
                ਜਾਰੀ;
            }
            ਜੇ (0
                == ਪਾਠ.compare(pos, std::ਲੜੀ_ਲੰਬਾਈ(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::ਲੜੀ_ਲੰਬਾਈ(V3Error::WARN_CONTEXT_BEGIN);
                ++ਵਿੱਚ_Begins;
                ਜਾਰੀ;
            }
            ਜੇ (0
                == ਪਾਠ.compare(pos, std::ਲੜੀ_ਲੰਬਾਈ(V3Error::WARN_CONTEXT_ਸਿਰਾ),
                                V3Error::WARN_CONTEXT_ਸਿਰਾ)) {
                pos += std::ਲੜੀ_ਲੰਬਾਈ(V3Error::WARN_CONTEXT_ਸਿਰਾ);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --ਵਿੱਚ_Begins;
                ਜਾਰੀ;
            }
        }
        ਜੇ (!stripContext || ਵਿੱਚ_Begins == 0) {
            // Remove double newlines
            ਜੇ (!(ਪਾਠ[pos] == '\n' && !ਨਤੀਜਾ.ਖਾਲੀ_ਥਾਵਾਂ() && ਨਤੀਜਾ[ਨਤੀਜਾ.ਮਾਪ() - 1] == '\n'))
                ਨਤੀਜਾ += ਪਾਠ[pos];
        }
        ++pos;
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

ਖਾਲੀ V3Error::abortIfWarnings() {
    ਜੇ (!isErrorOrWarn()) ਵਾਪਸ;
    ਅਚਲ ਤਾਰਕਿਕ exwarn = warnFatal() && warn_ਗਿਣਤੀ();
    ਜੇ (error_ਗਿਣਤੀ() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ਗਿਣਤੀ() << " error(s), "  //
                                      << V3Error::s().warn_ਗਿਣਤੀ() << " warning(s)\n");
    } ਨਹੀਂਤਾਂ ਜੇ (error_ਗਿਣਤੀ()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ਗਿਣਤੀ() << " error(s)\n");
    } ਨਹੀਂਤਾਂ {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_ਗਿਣਤੀ() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

ਖਾਲੀ V3Error::vlAbort() {
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#ਜੇ_ਅੰਤ
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_ਕੋਡ ਕੋਡ) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.ਤਾਲਾ_2();
    s().v3errorPrep(ਕੋਡ);
    ਵਾਪਸ v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_ਸਤਰ(V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ ਅੱਖਰ* file, ਪੂਰਨ ਸਤਰ)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(ਕੋਡ) << file << ":" << std::dec << ਸਤਰ << ": ";
    ਵਾਪਸ v3errorStr();
}
ਖਾਲੀ V3Error::v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& sstr, ਅਚਲ string& extra, File_ਸਤਰ* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_ਸਿਰਾ(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

ਖਾਲੀ v3error_ਸਿਰਾ(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_ਸਿਰਾ(sstr, "", ਖਾਲੀ_ਸੂਚਕ);
}
ਖਾਲੀ v3error_ਸਿਰਾ_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_ਸਿਰਾ(sstr, "", ਖਾਲੀ_ਸੂਚਕ);
    ਪ੍ਰਤਿਜ੍ਞਾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
