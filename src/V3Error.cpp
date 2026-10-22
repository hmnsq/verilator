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
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#অন্তর্ভাৱ <thread>
// clang-format on

//======================================================================

সংরচনা v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(ধ্রুৱ ৱর্ণ* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    বাবে (পূর্ণাঙ্ক codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        ধ্রুৱ V3ErrorCode code{codei};
        যদি (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            যদি (isRenamed()) m_e = renamedTo().m_e;
            ঘূৰাই;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() ধ্রুৱ {
    যদি (!isNamed())
        ঘূৰাই "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_সংখ্যা_STRING;
    ঘূৰাই "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_সংখ্যা_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

তার্কিক V3ErrorGuarded::isError(V3ErrorCode code, তার্কিক supp) VL_REQUIRES(m_mutex) {
    যদি (code.hardError()) ঘূৰাই সঁচা;
    যদি (supp) ঘূৰাই মিছা;
    যদি (code.severityInfo()) ঘূৰাই মিছা;
    যদি (pretendError(code)) ঘূৰাই সঁচা;
    ঘূৰাই মিছা;
}

string V3ErrorGuarded::বাৰ্তা_Prefix() VL_REQUIRES(m_mutex) {
    ধ্রুৱ V3ErrorCode code = m_message.code();
    ধ্রুৱ তার্কিক supp = m_errorSuppressed;
    যদি (supp) ঘূৰাই "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    যদি (code.severityInfo()) ঘূৰাই "-Info: ";
    যদি (code == V3ErrorCode::EC_FATAL) ঘূৰাই "%Error: ";
    যদি (code == V3ErrorCode::EC_FATALMANY) ঘূৰাই "%Error: ";
    যদি (code == V3ErrorCode::EC_FATALSRC) ঘূৰাই "%Error: Internal Error: ";
    যদি (code == V3ErrorCode::EC_ERROR) ঘূৰাই "%Error: ";
    যদি (isError(code, supp)) ঘূৰাই "%Error-" + std::string{code.ascii()} + ": ";
    ঘূৰাই "%Warning-" + std::string{code.ascii()} + ": ";
}

শূন্য V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    যদি (V3Error::debugDefault()) {
        std::cerr << বাৰ্তা_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    যদি (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#অন্যথা
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    ঘূৰাই string(বাৰ্তা_Prefix().size(), ' ');
}

শূন্য V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(সঁচা);
}

শূন্য V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = মিছা;
    m_errorSuppressed = মিছা;
}

শূন্য V3ErrorGuarded::v3errorEnd(ধ্রুৱ std::ostringstream& sstr, ধ্রুৱ string& extra,
                                File_সাৰি* fileline) VL_REQUIRES(m_mutex) {
    স্থির তার্কিক s_firedTooMany = মিছা;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    যদি (errorLimit() && error_গুনতি() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = সঁচা;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_গুনতি() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        প্রতিজ্ঞা(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
শূন্য V3ErrorGuarded::v3errorEndGuts(ধ্রুৱ std::ostringstream& sstr, ধ্রুৱ string& extra,
                                    File_সাৰি* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#যদি defined(__COVERITY__) || defined(__cppcheck__)
    যদি (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    যদি (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        যদি (m_message.code().defaultsOff()) ঘূৰাই;
        যদি (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) ঘূৰাই;
        m_showedSuppressed.set(m_message.code(), সঁচা);
    }
    string বাৰ্তা
        = V3Error::warnContextBegin() + বাৰ্তা_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_সাৰি = বাৰ্তা;
    {
        ধ্রুৱ string::size_type pos = first_সাৰি.find('\n');
        যদি (pos != string::npos) {
            first_সাৰি.erase(pos, first_সাৰি.length() - pos);
            first_সাৰি += "...";
        }
    }
    যদি (m_errorSuppressed) বাৰ্তা = first_সাৰি;
    // Suppress duplicate messages
    যদি (!m_messages.ঢোকাও(first_সাৰি).second) ঘূৰাই;

    বাৰ্তা = VString::replaceSubstr(বাৰ্তা, V3Error::warnMore(), warnMoreSpaces());

    string বাৰ্তা_additional;
    {
        ধ্রুৱ string::size_type pos = বাৰ্তা.find(V3Error::warnAdditionalInfo());
        যদি (pos != string::npos) {
            বাৰ্তা_additional = বাৰ্তা.substr(pos + V3Error::warnAdditionalInfo().size());
            বাৰ্তা.erase(pos);
        }
    }
    বাৰ্তা += '\n';  // Trailing newlines generally not put on messages so add
    যদি (!extra.empty() && !m_errorSuppressed) {
        string extra_বাৰ্তা = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_বাৰ্তা = warnMoreSpaces() + extra_বাৰ্তা + "\n";
        ধ্রুৱ size_t pos = বাৰ্তা.find('\n');
        বাৰ্তা.ঢোকাও(pos + 1, extra_বাৰ্তা);
    }
    // Output
    string text;
    যদি (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#অন্যথা
        সঁচা
#endif
    )
        text += বাৰ্তা;
    যদি (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, মিছা);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ঠেলো_Message(m_message);
#endif
    } অন্যথা {
        ধ্রুৱ তার্কিক anError = isError(m_message.code(), m_errorSuppressed);
        যদি (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), সঁচা);
            যদি (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } অন্যথা যদি (m_err_গুনতি >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = সঁচা;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } অন্যথা যদি (!m_tellManual) {
                m_tellManual = সঁচা;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            যদি (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                যদি (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        যদি (!বাৰ্তা_additional.empty()) text += বাৰ্তা_additional;
        std::cerr << V3Error::stripMetaText(text, মিছা);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ঠেলো_Message(m_message);
#endif

        যদি (anError) {
            incErrors();
        } অন্যথা {
            incWarnings();
        }
        যদি (m_message.code().severityFatal()) {
            স্থির তার্কিক s_inFatal = মিছা;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            যদি (!s_inFatal) {
                s_inFatal = সঁচা;
#ifndef V3ERROR_NO_GLOBAL_
                যদি (dump_বৃক্ষ_Level() || dump_বৃক্ষ_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(সঁচা);
                    যদি (dump_বৃক্ষ_Level()) {
                        v3Global.rootp()->dump_বৃক্ষ_File(v3Global.debugFilename("final.tree", 990));
                    }
                    যদি (dump_বৃক্ষ_Json_Level()) {
                        v3Global.rootp()->dump_বৃক্ষ_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    যদি (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(মিছা);
#endif
            }

            vlAbortOrExit();
        } অন্যথা যদি (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

শূন্য V3Error::init() {
    বাবে (পূর্ণাঙ্ক i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        ধ্রুৱ V3ErrorCode code{i};
        describedEachWarn(code, মিছা);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    প্রতিজ্ঞা(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::সাৰি_Str(ধ্রুৱ ৱর্ণ* filename, পূর্ণাঙ্ক lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<পূর্ণাঙ্ক>(0, 20 - static_cast<পূর্ণাঙ্ক>(out.str().length())), ' ');
    ঘূৰাই out.str();
}

string V3Error::stripMetaText(ধ্রুৱ string& text, তার্কিক stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    পূর্ণাঙ্ক inBegins = 0;
    বাবে (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        যদি (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            যদি (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                যেতিয়ালৈ (pos < text.size() && text[pos] != ')') ++pos;
                যদি (pos < text.size() && text[pos] == ')') ++pos;
                অনুৱর্ত;
            }
            যদি (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                অনুৱর্ত;
            }
            যদি (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                অনুৱর্ত;
            }
        }
        যদি (!stripContext || inBegins == 0) {
            // Remove double newlines
            যদি (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    ঘূৰাই result;
}

শূন্য V3Error::abortIfWarnings() {
    যদি (!isErrorOrWarn()) ঘূৰাই;
    ধ্রুৱ তার্কিক exwarn = warnFatal() && warn_গুনতি();
    যদি (error_গুনতি() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_গুনতি() << " error(s), "  //
                                      << V3Error::s().warn_গুনতি() << " warning(s)\n");
    } অন্যথা যদি (error_গুনতি()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_গুনতি() << " error(s)\n");
    } অন্যথা {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_গুনতি() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

শূন্য V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    ঘূৰাই v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_সাৰি(V3ErrorCode code, ধ্রুৱ ৱর্ণ* file, পূর্ণাঙ্ক সাৰি)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << সাৰি << ": ";
    ঘূৰাই v3errorStr();
}
শূন্য V3Error::v3errorEnd(ধ্রুৱ std::ostringstream& sstr, ধ্রুৱ string& extra, File_সাৰি* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

শূন্য v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
শূন্য v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    প্রতিজ্ঞা(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
