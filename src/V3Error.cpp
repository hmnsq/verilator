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
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Os.h"
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#అయితే_ముగింపు
#అన్తర్భావ <thread>
// clang-format on

//======================================================================

నిర్మాణం v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3_Error_సంకేతం::V3_Error_సంకేతం(స్థిరరాశి అక్షరం* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    కొరకు (పూర్ణాంకం codei = V3_Error_సంకేతం::EC_MIN; codei < V3_Error_సంకేతం::_ENUM_MAX; codei++) {
        స్థిరరాశి V3_Error_సంకేతం సంకేతం_2{codei};
        ఒకవేళ (0 == VL_STRCASECMP(msgp, సంకేతం_2.ascii())) {
            m_e = సంకేతం_2;
            ఒకవేళ (isRenamed()) m_e = renamed_గమ్యం().m_e;
            తిరిగి;
        }
    }
    m_e = V3_Error_సంకేతం::EC_ERROR;
}

string V3_Error_సంకేతం::url() స్థిరరాశి {
    ఒకవేళ (!isNamed())
        తిరిగి "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_సంస్కరణ_సంఖ్య_STRING;
    తిరిగి "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_సంస్కరణ_సంఖ్య_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

తార్కిక V3ErrorGuarded::isError(V3_Error_సంకేతం సంకేతం_2, తార్కిక supp) VL_REQUIRES(m_mutex) {
    ఒకవేళ (సంకేతం_2.hardError()) తిరిగి నిజం;
    ఒకవేళ (supp) తిరిగి అబద్ధం;
    ఒకవేళ (సంకేతం_2.severityInfo()) తిరిగి అబద్ధం;
    ఒకవేళ (pretendError(సంకేతం_2)) తిరిగి నిజం;
    తిరిగి అబద్ధం;
}

string V3ErrorGuarded::సందేశం_ఉపసర్గ() VL_REQUIRES(m_mutex) {
    స్థిరరాశి V3_Error_సంకేతం సంకేతం_2 = m_సందేశం.సంకేతం_2();
    స్థిరరాశి తార్కిక supp = m_errorSuppressed;
    ఒకవేళ (supp) తిరిగి "-arning-suppressed-" + std::string{సంకేతం_2.ascii()} + ": ";
    ఒకవేళ (సంకేతం_2.severityInfo()) తిరిగి "-Info: ";
    ఒకవేళ (సంకేతం_2 == V3_Error_సంకేతం::EC_FATAL) తిరిగి "%Error: ";
    ఒకవేళ (సంకేతం_2 == V3_Error_సంకేతం::EC_FATALMANY) తిరిగి "%Error: ";
    ఒకవేళ (సంకేతం_2 == V3_Error_సంకేతం::EC_FATALSRC) తిరిగి "%Error: Internal Error: ";
    ఒకవేళ (సంకేతం_2 == V3_Error_సంకేతం::EC_ERROR) తిరిగి "%Error: ";
    ఒకవేళ (isError(సంకేతం_2, supp)) తిరిగి "%Error-" + std::string{సంకేతం_2.ascii()} + ": ";
    తిరిగి "%Warning-" + std::string{సంకేతం_2.ascii()} + ": ";
}

శూన్య V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    ఒకవేళ (V3Error::debugDefault()) {
        std::cerr << సందేశం_ఉపసర్గ() << "Aborting since under --debug" << పంక్తి_ముగింపు;
        V3Error::vlAbort();
    }

#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
    ఒకవేళ (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#లేకపోతే
    std::నిష్క్రమించు(1);
#అయితే_ముగింపు
}

string V3ErrorGuarded::warn_మరిన్ని_Spaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_సందేశం.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    తిరిగి string(సందేశం_ఉపసర్గ().కొలత(), ' ');
}

శూన్య V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
    V3Stats::కూడు_Stat_కూడిక("Warnings, Suppressed "s + error_సంకేతం().ascii(), 1);
#అయితే_ముగింపు
    errorSuppressed(నిజం);
}

శూన్య V3ErrorGuarded::v3errorPrep(V3_Error_సంకేతం సంకేతం_2) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_సందేశం.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_సందేశం.init(సంకేతం_2);
    m_errorContexted = అబద్ధం;
    m_errorSuppressed = అబద్ధం;
}

శూన్య V3ErrorGuarded::v3error_చివర(స్థిరరాశి std::ostringstream& sstr, స్థిరరాశి string& extra,
                                File_పంక్తి* fileline) VL_REQUIRES(m_mutex) {
    స్థిర తార్కిక s_firedTooMany = అబద్ధం;
    v3error_చివర_Guts(sstr, extra, fileline);
    m_సందేశం.clear();
    ఒకవేళ (error_హద్దు() && error_లెక్క() >= error_హద్దు() && !s_firedTooMany) {
        s_firedTooMany = నిజం;
        // Recurses here
        v3error_చివర((v3errorPrep(V3_Error_సంకేతం::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_లెక్క() << std::పంక్తి_ముగింపు),
                    v3errorStr()),
                   "", శూన్యసూచిక);
        ప్రతిజ్ఞా(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
శూన్య V3ErrorGuarded::v3error_చివర_Guts(స్థిరరాశి std::ostringstream& sstr, స్థిరరాశి string& extra,
                                    File_పంక్తి* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#ఒకవేళ defined(__COVERITY__) || defined(__cppcheck__)
    ఒకవేళ (m_error_సంకేతం == V3_Error_సంకేతం::EC_FATAL) __coverity_panic__(x);
#అయితే_ముగింపు

    UASSERT_STATIC(!m_సందేశం.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_సందేశం.fileline(fileline);

    // Skip suppressed messages
    ఒకవేళ (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        ఒకవేళ (m_సందేశం.సంకేతం_2().defaultsOff()) తిరిగి;
        ఒకవేళ (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_సందేశం.సంకేతం_2()))) తిరిగి;
        m_showedSuppressed.సమితి(m_సందేశం.సంకేతం_2(), నిజం);
    }
    string సందేశం_2
        = V3Error::warnContextBegin() + సందేశం_ఉపసర్గ() + V3Error::warn_Context_చివర() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string మొదటి_పంక్తి = సందేశం_2;
    {
        స్థిరరాశి string::కొలత_type pos = మొదటి_పంక్తి.కనుగొను('\n');
        ఒకవేళ (pos != string::npos) {
            మొదటి_పంక్తి.erase(pos, మొదటి_పంక్తి.నిడివి() - pos);
            మొదటి_పంక్తి += "...";
        }
    }
    ఒకవేళ (m_errorSuppressed) సందేశం_2 = మొదటి_పంక్తి;
    // Suppress duplicate messages
    ఒకవేళ (!m_messages.చొప్పించు(మొదటి_పంక్తి).రెండవ) తిరిగి;

    సందేశం_2 = VString::replaceSubstr(సందేశం_2, V3Error::warn_మరిన్ని(), warn_మరిన్ని_Spaces());

    string సందేశం_additional;
    {
        స్థిరరాశి string::కొలత_type pos = సందేశం_2.కనుగొను(V3Error::warnAdditionalInfo());
        ఒకవేళ (pos != string::npos) {
            సందేశం_additional = సందేశం_2.substr(pos + V3Error::warnAdditionalInfo().కొలత());
            సందేశం_2.erase(pos);
        }
    }
    సందేశం_2 += '\n';  // Trailing newlines generally not put on messages so add
    ఒకవేళ (!extra.ఖాళీలు() && !m_errorSuppressed) {
        string extra_సందేశం = VString::replaceSubstr(extra, V3Error::warn_మరిన్ని(), warn_మరిన్ని_Spaces());
        extra_సందేశం = warn_మరిన్ని_Spaces() + extra_సందేశం + "\n";
        స్థిరరాశి కొలత_t pos = సందేశం_2.కనుగొను('\n');
        సందేశం_2.చొప్పించు(pos + 1, extra_సందేశం);
    }
    // Output
    string పాఠ్యం;
    ఒకవేళ (
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_సందేశం.సంకేతం_2() == V3_Error_సంకేతం::EC_FATALMANY)
#లేకపోతే
        నిజం
#అయితే_ముగింపు
    )
        పాఠ్యం += సందేశం_2;
    ఒకవేళ (m_errorSuppressed || m_సందేశం.సంకేతం_2().severityInfo()) {
        std::cerr << V3Error::strip_Meta_పాఠ్యం(పాఠ్యం, అబద్ధం);
        m_సందేశం.పాఠ్యం(పాఠ్యం);
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
        V3DiagSarif::నెట్టు_సందేశం(m_సందేశం);
#అయితే_ముగింపు
    } లేకపోతే {
        స్థిరరాశి తార్కిక anError = isError(m_సందేశం.సంకేతం_2(), m_errorSuppressed);
        ఒకవేళ (m_సందేశం.సంకేతం_2()
                != V3_Error_సంకేతం::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_సందేశం.సంకేతం_2())) {
            m_describedEachWarn.సమితి(m_సందేశం.సంకేతం_2(), నిజం);
            ఒకవేళ (m_సందేశం.సంకేతం_2() >= V3_Error_సంకేతం::EC_మొదటి_NAMED) {
                పాఠ్యం += warn_మరిన్ని_Spaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_సందేశం.సంకేతం_2().url() + '\n';
            } లేకపోతే ఒకవేళ (m_err_లెక్క >= 1 && m_సందేశం.సంకేతం_2().severityFatal() && !m_tell_అంతర్గత) {
                m_tell_అంతర్గత = నిజం;
                పాఠ్యం += warn_మరిన్ని_Spaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } లేకపోతే ఒకవేళ (!m_tellManual) {
                m_tellManual = నిజం;
                పాఠ్యం += warn_మరిన్ని_Spaces() + "... See the manual at " + m_సందేశం.సంకేతం_2().url()
                        + " for more assistance.\n";
            }
            ఒకవేళ (!m_pretendError.test(m_సందేశం.సంకేతం_2()) && !m_సందేశం.సంకేతం_2().hardError()) {
                పాఠ్యం += warn_మరిన్ని_Spaces() + "... Use \"/* verilator lint_off "
                        + m_సందేశం.సంకేతం_2().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                ఒకవేళ (m_సందేశం.సంకేతం_2().dangerous()) {
                    పాఠ్యం += warn_మరిన్ని_Spaces() + "*** See " + m_సందేశం.సంకేతం_2().url()
                            + " before disabling this,\n";
                    పాఠ్యం += warn_మరిన్ని_Spaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        ఒకవేళ (!సందేశం_additional.ఖాళీలు()) పాఠ్యం += సందేశం_additional;
        std::cerr << V3Error::strip_Meta_పాఠ్యం(పాఠ్యం, అబద్ధం);
        m_సందేశం.పాఠ్యం(పాఠ్యం);
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
        V3DiagSarif::నెట్టు_సందేశం(m_సందేశం);
#అయితే_ముగింపు

        ఒకవేళ (anError) {
            incErrors();
        } లేకపోతే {
            incWarnings();
        }
        ఒకవేళ (m_సందేశం.సంకేతం_2().severityFatal()) {
            స్థిర తార్కిక s_లోపల_Fatal = అబద్ధం;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            ఒకవేళ (!s_లోపల_Fatal) {
                s_లోపల_Fatal = నిజం;
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
                ఒకవేళ (dump_వృక్షం_Level() || dump_వృక్షం_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(నిజం);
                    ఒకవేళ (dump_వృక్షం_Level()) {
                        v3Global.rootp()->dump_వృక్షం_File(v3Global.debugFilename("final.tree", 990));
                    }
                    ఒకవేళ (dump_వృక్షం_Json_Level()) {
                        v3Global.rootp()->dump_వృక్షం_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    ఒకవేళ (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(అబద్ధం);
#అయితే_ముగింపు
            }

            vlAbortOrExit();
        } లేకపోతే ఒకవేళ (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

శూన్య V3Error::init() {
    కొరకు (పూర్ణాంకం i = 0; i < V3_Error_సంకేతం::_ENUM_MAX; i++) {
        స్థిరరాశి V3_Error_సంకేతం సంకేతం_2{i};
        describedEachWarn(సంకేతం_2, అబద్ధం);
        pretendError(సంకేతం_2, సంకేతం_2.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    ప్రతిజ్ఞా(std::string{V3_Error_సంకేతం{V3_Error_సంకేతం::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::పంక్తి_Str(స్థిరరాశి అక్షరం* filename, పూర్ణాంకం lineno) VL_శుద్ధ {
    std::ostringstream బహిర్గతం;
    బహిర్గతం << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    బహిర్గతం << std::string(std::max<పూర్ణాంకం>(0, 20 - static_cast<పూర్ణాంకం>(బహిర్గతం.str().నిడివి())), ' ');
    తిరిగి బహిర్గతం.str();
}

string V3Error::strip_Meta_పాఠ్యం(స్థిరరాశి string& పాఠ్యం, తార్కిక stripContext) VL_శుద్ధ {
    string ఫలితం;
    ఫలితం.reserve(పాఠ్యం.కొలత());
    పూర్ణాంకం లోపల_Begins = 0;
    కొరకు (string::కొలత_type pos = 0; pos < పాఠ్యం.కొలత();) {
        // string::starts_with is C++20
        ఒకవేళ (0 == పాఠ్యం.compare(pos, std::అక్షరశ్రేణి_పొడవు("__WARN"), "__WARN")) {
            ఒకవేళ (0 == పాఠ్యం.compare(pos, std::అక్షరశ్రేణి_పొడవు("__WARNRELATED("), "__WARNRELATED(")) {
                అయితే (pos < పాఠ్యం.కొలత() && పాఠ్యం[pos] != ')') ++pos;
                ఒకవేళ (pos < పాఠ్యం.కొలత() && పాఠ్యం[pos] == ')') ++pos;
                కొనసాగు;
            }
            ఒకవేళ (0
                == పాఠ్యం.compare(pos, std::అక్షరశ్రేణి_పొడవు(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::అక్షరశ్రేణి_పొడవు(V3Error::WARN_CONTEXT_BEGIN);
                ++లోపల_Begins;
                కొనసాగు;
            }
            ఒకవేళ (0
                == పాఠ్యం.compare(pos, std::అక్షరశ్రేణి_పొడవు(V3Error::WARN_CONTEXT_చివర),
                                V3Error::WARN_CONTEXT_చివర)) {
                pos += std::అక్షరశ్రేణి_పొడవు(V3Error::WARN_CONTEXT_చివర);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --లోపల_Begins;
                కొనసాగు;
            }
        }
        ఒకవేళ (!stripContext || లోపల_Begins == 0) {
            // Remove double newlines
            ఒకవేళ (!(పాఠ్యం[pos] == '\n' && !ఫలితం.ఖాళీలు() && ఫలితం[ఫలితం.కొలత() - 1] == '\n'))
                ఫలితం += పాఠ్యం[pos];
        }
        ++pos;
    }
    తిరిగి ఫలితం;
}

శూన్య V3Error::abortIfWarnings() {
    ఒకవేళ (!isErrorOrWarn()) తిరిగి;
    స్థిరరాశి తార్కిక exwarn = warnFatal() && warn_లెక్క();
    ఒకవేళ (error_లెక్క() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_లెక్క() << " error(s), "  //
                                      << V3Error::s().warn_లెక్క() << " warning(s)\n");
    } లేకపోతే ఒకవేళ (error_లెక్క()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_లెక్క() << " error(s)\n");
    } లేకపోతే {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_లెక్క() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

శూన్య V3Error::vlAbort() {
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#అయితే_ముగింపు
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3_Error_సంకేతం సంకేతం_2) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.తాళం_2();
    s().v3errorPrep(సంకేతం_2);
    తిరిగి v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_పంక్తి(V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి అక్షరం* file, పూర్ణాంకం పంక్తి)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(సంకేతం_2) << file << ":" << std::dec << పంక్తి << ": ";
    తిరిగి v3errorStr();
}
శూన్య V3Error::v3error_చివర(స్థిరరాశి std::ostringstream& sstr, స్థిరరాశి string& extra, File_పంక్తి* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3error_చివర(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

శూన్య v3error_చివర(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_చివర(sstr, "", శూన్యసూచిక);
}
శూన్య v3error_చివర_Fatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3error_చివర(sstr, "", శూన్యసూచిక);
    ప్రతిజ్ఞా(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
