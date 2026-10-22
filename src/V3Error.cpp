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
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#ଅନ୍ତର୍ଭାଵ <thread>
// clang-format on

//======================================================================

ସଂରଚନା v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(ଅଚଳ ଅକ୍ଷର* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        ଅଚଳ V3ErrorCode code{codei};
        ଯଦି (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            ଯଦି (isRenamed()) m_e = renamedTo().m_e;
            ଫେରନ୍ତୁ;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() ଅଚଳ {
    ଯଦି (!isNamed())
        ଫେରନ୍ତୁ "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_ସଂଖ୍ୟା_STRING;
    ଫେରନ୍ତୁ "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_ସଂଖ୍ୟା_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

ତାର୍କିକ V3ErrorGuarded::isError(V3ErrorCode code, ତାର୍କିକ supp) VL_REQUIRES(m_mutex) {
    ଯଦି (code.hardError()) ଫେରନ୍ତୁ ସତ୍ୟ;
    ଯଦି (supp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (code.severityInfo()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (pretendError(code)) ଫେରନ୍ତୁ ସତ୍ୟ;
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
}

string V3ErrorGuarded::ସନ୍ଦେଶ_Prefix() VL_REQUIRES(m_mutex) {
    ଅଚଳ V3ErrorCode code = m_message.code();
    ଅଚଳ ତାର୍କିକ supp = m_errorSuppressed;
    ଯଦି (supp) ଫେରନ୍ତୁ "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    ଯଦି (code.severityInfo()) ଫେରନ୍ତୁ "-Info: ";
    ଯଦି (code == V3ErrorCode::EC_FATAL) ଫେରନ୍ତୁ "%Error: ";
    ଯଦି (code == V3ErrorCode::EC_FATALMANY) ଫେରନ୍ତୁ "%Error: ";
    ଯଦି (code == V3ErrorCode::EC_FATALSRC) ଫେରନ୍ତୁ "%Error: Internal Error: ";
    ଯଦି (code == V3ErrorCode::EC_ERROR) ଫେରନ୍ତୁ "%Error: ";
    ଯଦି (isError(code, supp)) ଫେରନ୍ତୁ "%Error-" + std::string{code.ascii()} + ": ";
    ଫେରନ୍ତୁ "%Warning-" + std::string{code.ascii()} + ": ";
}

ଶୂନ୍ୟ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    ଯଦି (V3Error::debugDefault()) {
        std::cerr << ସନ୍ଦେଶ_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    ଯଦି (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#ଅନ୍ୟଥା
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    ଫେରନ୍ତୁ string(ସନ୍ଦେଶ_Prefix().size(), ' ');
}

ଶୂନ୍ୟ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(ସତ୍ୟ);
}

ଶୂନ୍ୟ V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = ମିଥ୍ୟା;
    m_errorSuppressed = ମିଥ୍ୟା;
}

ଶୂନ୍ୟ V3ErrorGuarded::v3errorEnd(ଅଚଳ std::ostringstream& sstr, ଅଚଳ string& extra,
                                File_ଧାଡ଼ି* fileline) VL_REQUIRES(m_mutex) {
    ସ୍ଥିର ତାର୍କିକ s_firedTooMany = ମିଥ୍ୟା;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    ଯଦି (errorLimit() && error_ଗଣନା() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = ସତ୍ୟ;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_ଗଣନା() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        ପ୍ରତିଜ୍ଞା(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
ଶୂନ୍ୟ V3ErrorGuarded::v3errorEndGuts(ଅଚଳ std::ostringstream& sstr, ଅଚଳ string& extra,
                                    File_ଧାଡ଼ି* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#ଯଦି defined(__COVERITY__) || defined(__cppcheck__)
    ଯଦି (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    ଯଦି (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        ଯଦି (m_message.code().defaultsOff()) ଫେରନ୍ତୁ;
        ଯଦି (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) ଫେରନ୍ତୁ;
        m_showedSuppressed.set(m_message.code(), ସତ୍ୟ);
    }
    string ସନ୍ଦେଶ
        = V3Error::warnContextBegin() + ସନ୍ଦେଶ_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_ଧାଡ଼ି = ସନ୍ଦେଶ;
    {
        ଅଚଳ string::size_type pos = first_ଧାଡ଼ି.find('\n');
        ଯଦି (pos != string::npos) {
            first_ଧାଡ଼ି.erase(pos, first_ଧାଡ଼ି.length() - pos);
            first_ଧାଡ଼ି += "...";
        }
    }
    ଯଦି (m_errorSuppressed) ସନ୍ଦେଶ = first_ଧାଡ଼ି;
    // Suppress duplicate messages
    ଯଦି (!m_messages.ଭର୍ତ୍ତି(first_ଧାଡ଼ି).second) ଫେରନ୍ତୁ;

    ସନ୍ଦେଶ = VString::replaceSubstr(ସନ୍ଦେଶ, V3Error::warnMore(), warnMoreSpaces());

    string ସନ୍ଦେଶ_additional;
    {
        ଅଚଳ string::size_type pos = ସନ୍ଦେଶ.find(V3Error::warnAdditionalInfo());
        ଯଦି (pos != string::npos) {
            ସନ୍ଦେଶ_additional = ସନ୍ଦେଶ.substr(pos + V3Error::warnAdditionalInfo().size());
            ସନ୍ଦେଶ.erase(pos);
        }
    }
    ସନ୍ଦେଶ += '\n';  // Trailing newlines generally not put on messages so add
    ଯଦି (!extra.empty() && !m_errorSuppressed) {
        string extra_ସନ୍ଦେଶ = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_ସନ୍ଦେଶ = warnMoreSpaces() + extra_ସନ୍ଦେଶ + "\n";
        ଅଚଳ size_t pos = ସନ୍ଦେଶ.find('\n');
        ସନ୍ଦେଶ.ଭର୍ତ୍ତି(pos + 1, extra_ସନ୍ଦେଶ);
    }
    // Output
    string text;
    ଯଦି (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#ଅନ୍ୟଥା
        ସତ୍ୟ
#endif
    )
        text += ସନ୍ଦେଶ;
    ଯଦି (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, ମିଥ୍ୟା);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ଠେଲ_Message(m_message);
#endif
    } ଅନ୍ୟଥା {
        ଅଚଳ ତାର୍କିକ anError = isError(m_message.code(), m_errorSuppressed);
        ଯଦି (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), ସତ୍ୟ);
            ଯଦି (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } ଅନ୍ୟଥା ଯଦି (m_err_ଗଣନା >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = ସତ୍ୟ;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } ଅନ୍ୟଥା ଯଦି (!m_tellManual) {
                m_tellManual = ସତ୍ୟ;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            ଯଦି (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                ଯଦି (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        ଯଦି (!ସନ୍ଦେଶ_additional.empty()) text += ସନ୍ଦେଶ_additional;
        std::cerr << V3Error::stripMetaText(text, ମିଥ୍ୟା);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ଠେଲ_Message(m_message);
#endif

        ଯଦି (anError) {
            incErrors();
        } ଅନ୍ୟଥା {
            incWarnings();
        }
        ଯଦି (m_message.code().severityFatal()) {
            ସ୍ଥିର ତାର୍କିକ s_inFatal = ମିଥ୍ୟା;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            ଯଦି (!s_inFatal) {
                s_inFatal = ସତ୍ୟ;
#ifndef V3ERROR_NO_GLOBAL_
                ଯଦି (dump_ବୃକ୍ଷ_Level() || dump_ବୃକ୍ଷ_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(ସତ୍ୟ);
                    ଯଦି (dump_ବୃକ୍ଷ_Level()) {
                        v3Global.rootp()->dump_ବୃକ୍ଷ_File(v3Global.debugFilename("final.tree", 990));
                    }
                    ଯଦି (dump_ବୃକ୍ଷ_Json_Level()) {
                        v3Global.rootp()->dump_ବୃକ୍ଷ_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    ଯଦି (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(ମିଥ୍ୟା);
#endif
            }

            vlAbortOrExit();
        } ଅନ୍ୟଥା ଯଦି (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

ଶୂନ୍ୟ V3Error::init() {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        ଅଚଳ V3ErrorCode code{i};
        describedEachWarn(code, ମିଥ୍ୟା);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    ପ୍ରତିଜ୍ଞା(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::ଧାଡ଼ି_Str(ଅଚଳ ଅକ୍ଷର* filename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(0, 20 - static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(out.str().length())), ' ');
    ଫେରନ୍ତୁ out.str();
}

string V3Error::stripMetaText(ଅଚଳ string& text, ତାର୍କିକ stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା inBegins = 0;
    ପାଇଁ (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        ଯଦି (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            ଯଦି (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                ଯେତେବେଳେ (pos < text.size() && text[pos] != ')') ++pos;
                ଯଦି (pos < text.size() && text[pos] == ')') ++pos;
                ଜାରି;
            }
            ଯଦି (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                ଜାରି;
            }
            ଯଦି (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                ଜାରି;
            }
        }
        ଯଦି (!stripContext || inBegins == 0) {
            // Remove double newlines
            ଯଦି (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    ଫେରନ୍ତୁ result;
}

ଶୂନ୍ୟ V3Error::abortIfWarnings() {
    ଯଦି (!isErrorOrWarn()) ଫେରନ୍ତୁ;
    ଅଚଳ ତାର୍କିକ exwarn = warnFatal() && warn_ଗଣନା();
    ଯଦି (error_ଗଣନା() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ଗଣନା() << " error(s), "  //
                                      << V3Error::s().warn_ଗଣନା() << " warning(s)\n");
    } ଅନ୍ୟଥା ଯଦି (error_ଗଣନା()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ଗଣନା() << " error(s)\n");
    } ଅନ୍ୟଥା {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_ଗଣନା() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

ଶୂନ୍ୟ V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    ଫେରନ୍ତୁ v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_ଧାଡ଼ି(V3ErrorCode code, ଅଚଳ ଅକ୍ଷର* file, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଧାଡ଼ି)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << ଧାଡ଼ି << ": ";
    ଫେରନ୍ତୁ v3errorStr();
}
ଶୂନ୍ୟ V3Error::v3errorEnd(ଅଚଳ std::ostringstream& sstr, ଅଚଳ string& extra, File_ଧାଡ଼ି* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

ଶୂନ୍ୟ v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
ଶୂନ୍ୟ v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    ପ୍ରତିଜ୍ଞା(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
