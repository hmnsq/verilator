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
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#അന്തര്ഭാവ <thread>
// clang-format on

//======================================================================

ഘടന v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(സ്ഥിരം അക്ഷരം* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    വേണ്ടി (പൂർണ്ണസംഖ്യ codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        സ്ഥിരം V3ErrorCode code{codei};
        എങ്കിൽ (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            എങ്കിൽ (isRenamed()) m_e = renamedTo().m_e;
            തിരികെ;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() സ്ഥിരം {
    എങ്കിൽ (!isNamed())
        തിരികെ "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_സംഖ്യ_STRING;
    തിരികെ "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_സംഖ്യ_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

തർക്കം V3ErrorGuarded::isError(V3ErrorCode code, തർക്കം supp) VL_REQUIRES(m_mutex) {
    എങ്കിൽ (code.hardError()) തിരികെ സത്യം;
    എങ്കിൽ (supp) തിരികെ അസത്യം;
    എങ്കിൽ (code.severityInfo()) തിരികെ അസത്യം;
    എങ്കിൽ (pretendError(code)) തിരികെ സത്യം;
    തിരികെ അസത്യം;
}

string V3ErrorGuarded::സന്ദേശം_Prefix() VL_REQUIRES(m_mutex) {
    സ്ഥിരം V3ErrorCode code = m_message.code();
    സ്ഥിരം തർക്കം supp = m_errorSuppressed;
    എങ്കിൽ (supp) തിരികെ "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    എങ്കിൽ (code.severityInfo()) തിരികെ "-Info: ";
    എങ്കിൽ (code == V3ErrorCode::EC_FATAL) തിരികെ "%Error: ";
    എങ്കിൽ (code == V3ErrorCode::EC_FATALMANY) തിരികെ "%Error: ";
    എങ്കിൽ (code == V3ErrorCode::EC_FATALSRC) തിരികെ "%Error: Internal Error: ";
    എങ്കിൽ (code == V3ErrorCode::EC_ERROR) തിരികെ "%Error: ";
    എങ്കിൽ (isError(code, supp)) തിരികെ "%Error-" + std::string{code.ascii()} + ": ";
    തിരികെ "%Warning-" + std::string{code.ascii()} + ": ";
}

ശൂന്യം V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    എങ്കിൽ (V3Error::debugDefault()) {
        std::cerr << സന്ദേശം_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    എങ്കിൽ (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#അല്ലെങ്കിൽ
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    തിരികെ string(സന്ദേശം_Prefix().size(), ' ');
}

ശൂന്യം V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(സത്യം);
}

ശൂന്യം V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = അസത്യം;
    m_errorSuppressed = അസത്യം;
}

ശൂന്യം V3ErrorGuarded::v3errorEnd(സ്ഥിരം std::ostringstream& sstr, സ്ഥിരം string& extra,
                                File_വരി* fileline) VL_REQUIRES(m_mutex) {
    സ്ഥിതം തർക്കം s_firedTooMany = അസത്യം;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    എങ്കിൽ (errorLimit() && error_എണ്ണം() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = സത്യം;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_എണ്ണം() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        പ്രതിജ്ഞാ(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
ശൂന്യം V3ErrorGuarded::v3errorEndGuts(സ്ഥിരം std::ostringstream& sstr, സ്ഥിരം string& extra,
                                    File_വരി* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#എങ്കിൽ defined(__COVERITY__) || defined(__cppcheck__)
    എങ്കിൽ (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    എങ്കിൽ (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        എങ്കിൽ (m_message.code().defaultsOff()) തിരികെ;
        എങ്കിൽ (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) തിരികെ;
        m_showedSuppressed.set(m_message.code(), സത്യം);
    }
    string സന്ദേശം
        = V3Error::warnContextBegin() + സന്ദേശം_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_വരി = സന്ദേശം;
    {
        സ്ഥിരം string::size_type pos = first_വരി.find('\n');
        എങ്കിൽ (pos != string::npos) {
            first_വരി.erase(pos, first_വരി.length() - pos);
            first_വരി += "...";
        }
    }
    എങ്കിൽ (m_errorSuppressed) സന്ദേശം = first_വരി;
    // Suppress duplicate messages
    എങ്കിൽ (!m_messages.തിരുകുക(first_വരി).second) തിരികെ;

    സന്ദേശം = VString::replaceSubstr(സന്ദേശം, V3Error::warnMore(), warnMoreSpaces());

    string സന്ദേശം_additional;
    {
        സ്ഥിരം string::size_type pos = സന്ദേശം.find(V3Error::warnAdditionalInfo());
        എങ്കിൽ (pos != string::npos) {
            സന്ദേശം_additional = സന്ദേശം.substr(pos + V3Error::warnAdditionalInfo().size());
            സന്ദേശം.erase(pos);
        }
    }
    സന്ദേശം += '\n';  // Trailing newlines generally not put on messages so add
    എങ്കിൽ (!extra.empty() && !m_errorSuppressed) {
        string extra_സന്ദേശം = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_സന്ദേശം = warnMoreSpaces() + extra_സന്ദേശം + "\n";
        സ്ഥിരം size_t pos = സന്ദേശം.find('\n');
        സന്ദേശം.തിരുകുക(pos + 1, extra_സന്ദേശം);
    }
    // Output
    string text;
    എങ്കിൽ (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#അല്ലെങ്കിൽ
        സത്യം
#endif
    )
        text += സന്ദേശം;
    എങ്കിൽ (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, അസത്യം);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::തള്ളുക_Message(m_message);
#endif
    } അല്ലെങ്കിൽ {
        സ്ഥിരം തർക്കം anError = isError(m_message.code(), m_errorSuppressed);
        എങ്കിൽ (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), സത്യം);
            എങ്കിൽ (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } അല്ലെങ്കിൽ എങ്കിൽ (m_err_എണ്ണം >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = സത്യം;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } അല്ലെങ്കിൽ എങ്കിൽ (!m_tellManual) {
                m_tellManual = സത്യം;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            എങ്കിൽ (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                എങ്കിൽ (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        എങ്കിൽ (!സന്ദേശം_additional.empty()) text += സന്ദേശം_additional;
        std::cerr << V3Error::stripMetaText(text, അസത്യം);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::തള്ളുക_Message(m_message);
#endif

        എങ്കിൽ (anError) {
            incErrors();
        } അല്ലെങ്കിൽ {
            incWarnings();
        }
        എങ്കിൽ (m_message.code().severityFatal()) {
            സ്ഥിതം തർക്കം s_inFatal = അസത്യം;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            എങ്കിൽ (!s_inFatal) {
                s_inFatal = സത്യം;
#ifndef V3ERROR_NO_GLOBAL_
                എങ്കിൽ (dump_വൃക്ഷം_Level() || dump_വൃക്ഷം_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(സത്യം);
                    എങ്കിൽ (dump_വൃക്ഷം_Level()) {
                        v3Global.rootp()->dump_വൃക്ഷം_File(v3Global.debugFilename("final.tree", 990));
                    }
                    എങ്കിൽ (dump_വൃക്ഷം_Json_Level()) {
                        v3Global.rootp()->dump_വൃക്ഷം_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    എങ്കിൽ (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(അസത്യം);
#endif
            }

            vlAbortOrExit();
        } അല്ലെങ്കിൽ എങ്കിൽ (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

ശൂന്യം V3Error::init() {
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        സ്ഥിരം V3ErrorCode code{i};
        describedEachWarn(code, അസത്യം);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    പ്രതിജ്ഞാ(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::വരി_Str(സ്ഥിരം അക്ഷരം* filename, പൂർണ്ണസംഖ്യ lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<പൂർണ്ണസംഖ്യ>(0, 20 - static_cast<പൂർണ്ണസംഖ്യ>(out.str().length())), ' ');
    തിരികെ out.str();
}

string V3Error::stripMetaText(സ്ഥിരം string& text, തർക്കം stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    പൂർണ്ണസംഖ്യ inBegins = 0;
    വേണ്ടി (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        എങ്കിൽ (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            എങ്കിൽ (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                ആകുമ്പോൾ (pos < text.size() && text[pos] != ')') ++pos;
                എങ്കിൽ (pos < text.size() && text[pos] == ')') ++pos;
                തുടരുക;
            }
            എങ്കിൽ (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                തുടരുക;
            }
            എങ്കിൽ (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                തുടരുക;
            }
        }
        എങ്കിൽ (!stripContext || inBegins == 0) {
            // Remove double newlines
            എങ്കിൽ (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    തിരികെ result;
}

ശൂന്യം V3Error::abortIfWarnings() {
    എങ്കിൽ (!isErrorOrWarn()) തിരികെ;
    സ്ഥിരം തർക്കം exwarn = warnFatal() && warn_എണ്ണം();
    എങ്കിൽ (error_എണ്ണം() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_എണ്ണം() << " error(s), "  //
                                      << V3Error::s().warn_എണ്ണം() << " warning(s)\n");
    } അല്ലെങ്കിൽ എങ്കിൽ (error_എണ്ണം()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_എണ്ണം() << " error(s)\n");
    } അല്ലെങ്കിൽ {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_എണ്ണം() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

ശൂന്യം V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    തിരികെ v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_വരി(V3ErrorCode code, സ്ഥിരം അക്ഷരം* file, പൂർണ്ണസംഖ്യ വരി)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << വരി << ": ";
    തിരികെ v3errorStr();
}
ശൂന്യം V3Error::v3errorEnd(സ്ഥിരം std::ostringstream& sstr, സ്ഥിരം string& extra, File_വരി* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

ശൂന്യം v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
ശൂന്യം v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    പ്രതിജ്ഞാ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
