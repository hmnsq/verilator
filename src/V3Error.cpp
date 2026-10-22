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
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ <thread>
// clang-format on

//======================================================================

ರಚನೆ v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_ಸಂಕೇತ::V3_Error_ಸಂಕೇತ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ codei = V3_Error_ಸಂಕೇತ::EC_MIN; codei < V3_Error_ಸಂಕೇತ::_ENUM_MAX; codei++) {
        ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2{codei};
        ಆದರೆ (0 == VL_STRCASECMP(msgp, ಸಂಕೇತ_2.ascii())) {
            m_e = ಸಂಕೇತ_2;
            ಆದರೆ (isRenamed()) m_e = renamed_ಗಮ್ಯ().m_e;
            ಹಿಂತಿರುಗಿ;
        }
    }
    m_e = V3_Error_ಸಂಕೇತ::EC_ERROR;
}

string V3_Error_ಸಂಕೇತ::url() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!isNamed())
        ಹಿಂತಿರುಗಿ "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_ಆವೃತ್ತಿ_ಸಂಖ್ಯೆ_STRING;
    ಹಿಂತಿರುಗಿ "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_ಆವೃತ್ತಿ_ಸಂಖ್ಯೆ_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

ತಾರ್ಕಿಕ V3ErrorGuarded::isError(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ತಾರ್ಕಿಕ supp) VL_REQUIRES(m_mutex) {
    ಆದರೆ (ಸಂಕೇತ_2.hardError()) ಹಿಂತಿರುಗಿ ನಿಜ;
    ಆದರೆ (supp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (ಸಂಕೇತ_2.severityInfo()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (pretendError(ಸಂಕೇತ_2)) ಹಿಂತಿರುಗಿ ನಿಜ;
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
}

string V3ErrorGuarded::ಸಂದೇಶ_ಪೂರ್ವಪ್ರತ್ಯಯ() VL_REQUIRES(m_mutex) {
    ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2 = m_ಸಂದೇಶ.ಸಂಕೇತ_2();
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ supp = m_errorSuppressed;
    ಆದರೆ (supp) ಹಿಂತಿರುಗಿ "-arning-suppressed-" + std::string{ಸಂಕೇತ_2.ascii()} + ": ";
    ಆದರೆ (ಸಂಕೇತ_2.severityInfo()) ಹಿಂತಿರುಗಿ "-Info: ";
    ಆದರೆ (ಸಂಕೇತ_2 == V3_Error_ಸಂಕೇತ::EC_FATAL) ಹಿಂತಿರುಗಿ "%Error: ";
    ಆದರೆ (ಸಂಕೇತ_2 == V3_Error_ಸಂಕೇತ::EC_FATALMANY) ಹಿಂತಿರುಗಿ "%Error: ";
    ಆದರೆ (ಸಂಕೇತ_2 == V3_Error_ಸಂಕೇತ::EC_FATALSRC) ಹಿಂತಿರುಗಿ "%Error: Internal Error: ";
    ಆದರೆ (ಸಂಕೇತ_2 == V3_Error_ಸಂಕೇತ::EC_ERROR) ಹಿಂತಿರುಗಿ "%Error: ";
    ಆದರೆ (isError(ಸಂಕೇತ_2, supp)) ಹಿಂತಿರುಗಿ "%Error-" + std::string{ಸಂಕೇತ_2.ascii()} + ": ";
    ಹಿಂತಿರುಗಿ "%Warning-" + std::string{ಸಂಕೇತ_2.ascii()} + ": ";
}

ಶೂನ್ಯ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    ಆದರೆ (V3Error::debugDefault()) {
        std::cerr << ಸಂದೇಶ_ಪೂರ್ವಪ್ರತ್ಯಯ() << "Aborting since under --debug" << ಸಾಲು_ಅಂತ್ಯ;
        V3Error::vlAbort();
    }

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
    ಆದರೆ (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#ಇಲ್ಲದಿದ್ದರೆ
    std::ನಿರ್ಗಮಿಸು(1);
#ಆದರೆ_ಅಂತ್ಯ
}

string V3ErrorGuarded::warn_ಇನ್ನಷ್ಟು_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_ಸಂದೇಶ.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    ಹಿಂತಿರುಗಿ string(ಸಂದೇಶ_ಪೂರ್ವಪ್ರತ್ಯಯ().ಗಾತ್ರ_2(), ' ');
}

ಶೂನ್ಯ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
    V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ("Warnings, Suppressed "s + error_ಸಂಕೇತ().ascii(), 1);
#ಆದರೆ_ಅಂತ್ಯ
    errorSuppressed(ನಿಜ);
}

ಶೂನ್ಯ V3ErrorGuarded::v3errorPrep(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_ಸಂದೇಶ.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_ಸಂದೇಶ.init(ಸಂಕೇತ_2);
    m_errorContexted = ಸುಳ್ಳು;
    m_errorSuppressed = ಸುಳ್ಳು;
}

ಶೂನ್ಯ V3ErrorGuarded::v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& sstr, ಸ್ಥಿರಾಂಕ string& extra,
                                File_ಸಾಲು* fileline) VL_REQUIRES(m_mutex) {
    ಸ್ಥಿರ ತಾರ್ಕಿಕ s_firedTooMany = ಸುಳ್ಳು;
    v3error_ಕೊನೆ_Guts(sstr, extra, fileline);
    m_ಸಂದೇಶ.clear();
    ಆದರೆ (error_ಎಲ್ಲೆ() && error_ಎಣಿಕೆ() >= error_ಎಲ್ಲೆ() && !s_firedTooMany) {
        s_firedTooMany = ನಿಜ;
        // Recurses here
        v3error_ಕೊನೆ((v3errorPrep(V3_Error_ಸಂಕೇತ::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_ಎಣಿಕೆ() << std::ಸಾಲು_ಅಂತ್ಯ),
                    v3errorStr()),
                   "", ಶೂನ್ಯಸೂಚಕ);
        ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
ಶೂನ್ಯ V3ErrorGuarded::v3error_ಕೊನೆ_Guts(ಸ್ಥಿರಾಂಕ std::ostringstream& sstr, ಸ್ಥಿರಾಂಕ string& extra,
                                    File_ಸಾಲು* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#ಆದರೆ defined(__COVERITY__) || defined(__cppcheck__)
    ಆದರೆ (m_error_ಸಂಕೇತ == V3_Error_ಸಂಕೇತ::EC_FATAL) __coverity_panic__(x);
#ಆದರೆ_ಅಂತ್ಯ

    UASSERT_STATIC(!m_ಸಂದೇಶ.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_ಸಂದೇಶ.fileline(fileline);

    // Skip suppressed messages
    ಆದರೆ (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        ಆದರೆ (m_ಸಂದೇಶ.ಸಂಕೇತ_2().defaultsOff()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_ಸಂದೇಶ.ಸಂಕೇತ_2()))) ಹಿಂತಿರುಗಿ;
        m_showedSuppressed.ಗಣ(m_ಸಂದೇಶ.ಸಂಕೇತ_2(), ನಿಜ);
    }
    string ಸಂದೇಶ_2
        = V3Error::warnContextBegin() + ಸಂದೇಶ_ಪೂರ್ವಪ್ರತ್ಯಯ() + V3Error::warn_Context_ಕೊನೆ() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string ಮೊದಲ_ಸಾಲು = ಸಂದೇಶ_2;
    {
        ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type pos = ಮೊದಲ_ಸಾಲು.ಕಂಡುಹಿಡಿ('\n');
        ಆದರೆ (pos != string::npos) {
            ಮೊದಲ_ಸಾಲು.erase(pos, ಮೊದಲ_ಸಾಲು.ಉದ್ದಳತೆ() - pos);
            ಮೊದಲ_ಸಾಲು += "...";
        }
    }
    ಆದರೆ (m_errorSuppressed) ಸಂದೇಶ_2 = ಮೊದಲ_ಸಾಲು;
    // Suppress duplicate messages
    ಆದರೆ (!m_messages.ಸೇರಿಸು(ಮೊದಲ_ಸಾಲು).ಎರಡನೇ) ಹಿಂತಿರುಗಿ;

    ಸಂದೇಶ_2 = VString::replaceSubstr(ಸಂದೇಶ_2, V3Error::warn_ಇನ್ನಷ್ಟು(), warn_ಇನ್ನಷ್ಟು_Spaces());

    string ಸಂದೇಶ_additional;
    {
        ಸ್ಥಿರಾಂಕ string::ಗಾತ್ರ_type pos = ಸಂದೇಶ_2.ಕಂಡುಹಿಡಿ(V3Error::warnAdditionalInfo());
        ಆದರೆ (pos != string::npos) {
            ಸಂದೇಶ_additional = ಸಂದೇಶ_2.substr(pos + V3Error::warnAdditionalInfo().ಗಾತ್ರ_2());
            ಸಂದೇಶ_2.erase(pos);
        }
    }
    ಸಂದೇಶ_2 += '\n';  // Trailing newlines generally not put on messages so add
    ಆದರೆ (!extra.ಖಾಲಿಗಳು() && !m_errorSuppressed) {
        string extra_ಸಂದೇಶ = VString::replaceSubstr(extra, V3Error::warn_ಇನ್ನಷ್ಟು(), warn_ಇನ್ನಷ್ಟು_Spaces());
        extra_ಸಂದೇಶ = warn_ಇನ್ನಷ್ಟು_Spaces() + extra_ಸಂದೇಶ + "\n";
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t pos = ಸಂದೇಶ_2.ಕಂಡುಹಿಡಿ('\n');
        ಸಂದೇಶ_2.ಸೇರಿಸು(pos + 1, extra_ಸಂದೇಶ);
    }
    // Output
    string ಪಠ್ಯ;
    ಆದರೆ (
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_ಸಂದೇಶ.ಸಂಕೇತ_2() == V3_Error_ಸಂಕೇತ::EC_FATALMANY)
#ಇಲ್ಲದಿದ್ದರೆ
        ನಿಜ
#ಆದರೆ_ಅಂತ್ಯ
    )
        ಪಠ್ಯ += ಸಂದೇಶ_2;
    ಆದರೆ (m_errorSuppressed || m_ಸಂದೇಶ.ಸಂಕೇತ_2().severityInfo()) {
        std::cerr << V3Error::strip_Meta_ಪಠ್ಯ(ಪಠ್ಯ, ಸುಳ್ಳು);
        m_ಸಂದೇಶ.ಪಠ್ಯ(ಪಠ್ಯ);
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
        V3DiagSarif::ತಳ್ಳು_ಸಂದೇಶ(m_ಸಂದೇಶ);
#ಆದರೆ_ಅಂತ್ಯ
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ anError = isError(m_ಸಂದೇಶ.ಸಂಕೇತ_2(), m_errorSuppressed);
        ಆದರೆ (m_ಸಂದೇಶ.ಸಂಕೇತ_2()
                != V3_Error_ಸಂಕೇತ::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_ಸಂದೇಶ.ಸಂಕೇತ_2())) {
            m_describedEachWarn.ಗಣ(m_ಸಂದೇಶ.ಸಂಕೇತ_2(), ನಿಜ);
            ಆದರೆ (m_ಸಂದೇಶ.ಸಂಕೇತ_2() >= V3_Error_ಸಂಕೇತ::EC_ಮೊದಲ_NAMED) {
                ಪಠ್ಯ += warn_ಇನ್ನಷ್ಟು_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_ಸಂದೇಶ.ಸಂಕೇತ_2().url() + '\n';
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (m_err_ಎಣಿಕೆ >= 1 && m_ಸಂದೇಶ.ಸಂಕೇತ_2().severityFatal() && !m_tell_ಆಂತರಿಕ) {
                m_tell_ಆಂತರಿಕ = ನಿಜ;
                ಪಠ್ಯ += warn_ಇನ್ನಷ್ಟು_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!m_tellManual) {
                m_tellManual = ನಿಜ;
                ಪಠ್ಯ += warn_ಇನ್ನಷ್ಟು_Spaces() + "... See the manual at " + m_ಸಂದೇಶ.ಸಂಕೇತ_2().url()
                        + " for more assistance.\n";
            }
            ಆದರೆ (!m_pretendError.test(m_ಸಂದೇಶ.ಸಂಕೇತ_2()) && !m_ಸಂದೇಶ.ಸಂಕೇತ_2().hardError()) {
                ಪಠ್ಯ += warn_ಇನ್ನಷ್ಟು_Spaces() + "... Use \"/* verilator lint_off "
                        + m_ಸಂದೇಶ.ಸಂಕೇತ_2().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                ಆದರೆ (m_ಸಂದೇಶ.ಸಂಕೇತ_2().dangerous()) {
                    ಪಠ್ಯ += warn_ಇನ್ನಷ್ಟು_Spaces() + "*** See " + m_ಸಂದೇಶ.ಸಂಕೇತ_2().url()
                            + " before disabling this,\n";
                    ಪಠ್ಯ += warn_ಇನ್ನಷ್ಟು_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        ಆದರೆ (!ಸಂದೇಶ_additional.ಖಾಲಿಗಳು()) ಪಠ್ಯ += ಸಂದೇಶ_additional;
        std::cerr << V3Error::strip_Meta_ಪಠ್ಯ(ಪಠ್ಯ, ಸುಳ್ಳು);
        m_ಸಂದೇಶ.ಪಠ್ಯ(ಪಠ್ಯ);
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
        V3DiagSarif::ತಳ್ಳು_ಸಂದೇಶ(m_ಸಂದೇಶ);
#ಆದರೆ_ಅಂತ್ಯ

        ಆದರೆ (anError) {
            incErrors();
        } ಇಲ್ಲದಿದ್ದರೆ {
            incWarnings();
        }
        ಆದರೆ (m_ಸಂದೇಶ.ಸಂಕೇತ_2().severityFatal()) {
            ಸ್ಥಿರ ತಾರ್ಕಿಕ s_ಒಳ_Fatal = ಸುಳ್ಳು;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            ಆದರೆ (!s_ಒಳ_Fatal) {
                s_ಒಳ_Fatal = ನಿಜ;
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
                ಆದರೆ (dump_ವೃಕ್ಷ_Level() || dump_ವೃಕ್ಷ_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(ನಿಜ);
                    ಆದರೆ (dump_ವೃಕ್ಷ_Level()) {
                        v3Global.rootp()->dump_ವೃಕ್ಷ_File(v3Global.debugFilename("final.tree", 990));
                    }
                    ಆದರೆ (dump_ವೃಕ್ಷ_Json_Level()) {
                        v3Global.rootp()->dump_ವೃಕ್ಷ_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    ಆದರೆ (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(ಸುಳ್ಳು);
#ಆದರೆ_ಅಂತ್ಯ
            }

            vlAbortOrExit();
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

ಶೂನ್ಯ V3Error::init() {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < V3_Error_ಸಂಕೇತ::_ENUM_MAX; i++) {
        ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2{i};
        describedEachWarn(ಸಂಕೇತ_2, ಸುಳ್ಳು);
        pretendError(ಸಂಕೇತ_2, ಸಂಕೇತ_2.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    ಪ್ರತಿಜ್ಞಾ(std::string{V3_Error_ಸಂಕೇತ{V3_Error_ಸಂಕೇತ::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::ಸಾಲು_Str(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* filename, ಪೂರ್ಣಾಂಕ lineno) VL_ಶುದ್ಧ {
    std::ostringstream ಹೊರಹೋಗು;
    ಹೊರಹೋಗು << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    ಹೊರಹೋಗು << std::string(std::max<ಪೂರ್ಣಾಂಕ>(0, 20 - static_cast<ಪೂರ್ಣಾಂಕ>(ಹೊರಹೋಗು.str().ಉದ್ದಳತೆ())), ' ');
    ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು.str();
}

string V3Error::strip_Meta_ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ, ತಾರ್ಕಿಕ stripContext) VL_ಶುದ್ಧ {
    string ಫಲಿತಾಂಶ;
    ಫಲಿತಾಂಶ.reserve(ಪಠ್ಯ.ಗಾತ್ರ_2());
    ಪೂರ್ಣಾಂಕ ಒಳ_Begins = 0;
    ಗಾಗಿ (string::ಗಾತ್ರ_type pos = 0; pos < ಪಠ್ಯ.ಗಾತ್ರ_2();) {
        // string::starts_with is C++20
        ಆದರೆ (0 == ಪಠ್ಯ.compare(pos, std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__WARN"), "__WARN")) {
            ಆದರೆ (0 == ಪಠ್ಯ.compare(pos, std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("__WARNRELATED("), "__WARNRELATED(")) {
                ಆಗ (pos < ಪಠ್ಯ.ಗಾತ್ರ_2() && ಪಠ್ಯ[pos] != ')') ++pos;
                ಆದರೆ (pos < ಪಠ್ಯ.ಗಾತ್ರ_2() && ಪಠ್ಯ[pos] == ')') ++pos;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (0
                == ಪಠ್ಯ.compare(pos, std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(V3Error::WARN_CONTEXT_BEGIN);
                ++ಒಳ_Begins;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (0
                == ಪಠ್ಯ.compare(pos, std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(V3Error::WARN_CONTEXT_ಕೊನೆ),
                                V3Error::WARN_CONTEXT_ಕೊನೆ)) {
                pos += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(V3Error::WARN_CONTEXT_ಕೊನೆ);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --ಒಳ_Begins;
                ಮುಂದುವರಿ;
            }
        }
        ಆದರೆ (!stripContext || ಒಳ_Begins == 0) {
            // Remove double newlines
            ಆದರೆ (!(ಪಠ್ಯ[pos] == '\n' && !ಫಲಿತಾಂಶ.ಖಾಲಿಗಳು() && ಫಲಿತಾಂಶ[ಫಲಿತಾಂಶ.ಗಾತ್ರ_2() - 1] == '\n'))
                ಫಲಿತಾಂಶ += ಪಠ್ಯ[pos];
        }
        ++pos;
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

ಶೂನ್ಯ V3Error::abortIfWarnings() {
    ಆದರೆ (!isErrorOrWarn()) ಹಿಂತಿರುಗಿ;
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ exwarn = warnFatal() && warn_ಎಣಿಕೆ();
    ಆದರೆ (error_ಎಣಿಕೆ() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ಎಣಿಕೆ() << " error(s), "  //
                                      << V3Error::s().warn_ಎಣಿಕೆ() << " warning(s)\n");
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (error_ಎಣಿಕೆ()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ಎಣಿಕೆ() << " error(s)\n");
    } ಇಲ್ಲದಿದ್ದರೆ {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_ಎಣಿಕೆ() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

ಶೂನ್ಯ V3Error::vlAbort() {
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#ಆದರೆ_ಅಂತ್ಯ
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.ಬೀಗ_2();
    s().v3errorPrep(ಸಂಕೇತ_2);
    ಹಿಂತಿರುಗಿ v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_ಸಾಲು(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* file, ಪೂರ್ಣಾಂಕ ಸಾಲು)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(ಸಂಕೇತ_2) << file << ":" << std::dec << ಸಾಲು << ": ";
    ಹಿಂತಿರುಗಿ v3errorStr();
}
ಶೂನ್ಯ V3Error::v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& sstr, ಸ್ಥಿರಾಂಕ string& extra, File_ಸಾಲು* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_ಕೊನೆ(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

ಶೂನ್ಯ v3error_ಕೊನೆ(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_ಕೊನೆ(sstr, "", ಶೂನ್ಯಸೂಚಕ);
}
ಶೂನ್ಯ v3error_ಕೊನೆ_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_ಕೊನೆ(sstr, "", ಶೂನ್ಯಸೂಚಕ);
    ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
