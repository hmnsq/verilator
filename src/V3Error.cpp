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
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#અન્તર્ભાવ <thread>
// clang-format on

//======================================================================

સંરચના v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(અચલ અક્ષર* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    માટે (પૂર્ણાંક codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        અચલ V3ErrorCode code{codei};
        જો (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            જો (isRenamed()) m_e = renamedTo().m_e;
            પાછા;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() અચલ {
    જો (!isNamed())
        પાછા "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_સંખ્યા_STRING;
    પાછા "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_સંખ્યા_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

તાર્કિક V3ErrorGuarded::isError(V3ErrorCode code, તાર્કિક supp) VL_REQUIRES(m_mutex) {
    જો (code.hardError()) પાછા સાચું;
    જો (supp) પાછા ખોટું;
    જો (code.severityInfo()) પાછા ખોટું;
    જો (pretendError(code)) પાછા સાચું;
    પાછા ખોટું;
}

string V3ErrorGuarded::સંદેશ_Prefix() VL_REQUIRES(m_mutex) {
    અચલ V3ErrorCode code = m_message.code();
    અચલ તાર્કિક supp = m_errorSuppressed;
    જો (supp) પાછા "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    જો (code.severityInfo()) પાછા "-Info: ";
    જો (code == V3ErrorCode::EC_FATAL) પાછા "%Error: ";
    જો (code == V3ErrorCode::EC_FATALMANY) પાછા "%Error: ";
    જો (code == V3ErrorCode::EC_FATALSRC) પાછા "%Error: Internal Error: ";
    જો (code == V3ErrorCode::EC_ERROR) પાછા "%Error: ";
    જો (isError(code, supp)) પાછા "%Error-" + std::string{code.ascii()} + ": ";
    પાછા "%Warning-" + std::string{code.ascii()} + ": ";
}

શૂન્ય V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    જો (V3Error::debugDefault()) {
        std::cerr << સંદેશ_Prefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    જો (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#નહીંતર
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    પાછા string(સંદેશ_Prefix().size(), ' ');
}

શૂન્ય V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(સાચું);
}

શૂન્ય V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = ખોટું;
    m_errorSuppressed = ખોટું;
}

શૂન્ય V3ErrorGuarded::v3errorEnd(અચલ std::ostringstream& sstr, અચલ string& extra,
                                File_લીટી* fileline) VL_REQUIRES(m_mutex) {
    સ્થિર તાર્કિક s_firedTooMany = ખોટું;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    જો (errorLimit() && error_ગણતરી() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = સાચું;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << error_ગણતરી() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        પ્રતિજ્ઞા(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
શૂન્ય V3ErrorGuarded::v3errorEndGuts(અચલ std::ostringstream& sstr, અચલ string& extra,
                                    File_લીટી* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#જો defined(__COVERITY__) || defined(__cppcheck__)
    જો (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    જો (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        જો (m_message.code().defaultsOff()) પાછા;
        જો (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) પાછા;
        m_showedSuppressed.set(m_message.code(), સાચું);
    }
    string સંદેશ
        = V3Error::warnContextBegin() + સંદેશ_Prefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string first_લીટી = સંદેશ;
    {
        અચલ string::size_type pos = first_લીટી.find('\n');
        જો (pos != string::npos) {
            first_લીટી.erase(pos, first_લીટી.length() - pos);
            first_લીટી += "...";
        }
    }
    જો (m_errorSuppressed) સંદેશ = first_લીટી;
    // Suppress duplicate messages
    જો (!m_messages.ઉમેરો(first_લીટી).second) પાછા;

    સંદેશ = VString::replaceSubstr(સંદેશ, V3Error::warnMore(), warnMoreSpaces());

    string સંદેશ_additional;
    {
        અચલ string::size_type pos = સંદેશ.find(V3Error::warnAdditionalInfo());
        જો (pos != string::npos) {
            સંદેશ_additional = સંદેશ.substr(pos + V3Error::warnAdditionalInfo().size());
            સંદેશ.erase(pos);
        }
    }
    સંદેશ += '\n';  // Trailing newlines generally not put on messages so add
    જો (!extra.empty() && !m_errorSuppressed) {
        string extra_સંદેશ = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extra_સંદેશ = warnMoreSpaces() + extra_સંદેશ + "\n";
        અચલ size_t pos = સંદેશ.find('\n');
        સંદેશ.ઉમેરો(pos + 1, extra_સંદેશ);
    }
    // Output
    string text;
    જો (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#નહીંતર
        સાચું
#endif
    )
        text += સંદેશ;
    જો (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, ખોટું);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ધકેલો_Message(m_message);
#endif
    } નહીંતર {
        અચલ તાર્કિક anError = isError(m_message.code(), m_errorSuppressed);
        જો (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), સાચું);
            જો (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } નહીંતર જો (m_err_ગણતરી >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = સાચું;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } નહીંતર જો (!m_tellManual) {
                m_tellManual = સાચું;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            જો (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                જો (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        જો (!સંદેશ_additional.empty()) text += સંદેશ_additional;
        std::cerr << V3Error::stripMetaText(text, ખોટું);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::ધકેલો_Message(m_message);
#endif

        જો (anError) {
            incErrors();
        } નહીંતર {
            incWarnings();
        }
        જો (m_message.code().severityFatal()) {
            સ્થિર તાર્કિક s_inFatal = ખોટું;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            જો (!s_inFatal) {
                s_inFatal = સાચું;
#ifndef V3ERROR_NO_GLOBAL_
                જો (dump_વૃક્ષ_Level() || dump_વૃક્ષ_Json_Level() || debug()) {
                    V3Broken::allowMidvisitorCheck(સાચું);
                    જો (dump_વૃક્ષ_Level()) {
                        v3Global.rootp()->dump_વૃક્ષ_File(v3Global.debugFilename("final.tree", 990));
                    }
                    જો (dump_વૃક્ષ_Json_Level()) {
                        v3Global.rootp()->dump_વૃક્ષ_Json_File(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    જો (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(ખોટું);
#endif
            }

            vlAbortOrExit();
        } નહીંતર જો (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

શૂન્ય V3Error::init() {
    માટે (પૂર્ણાંક i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        અચલ V3ErrorCode code{i};
        describedEachWarn(code, ખોટું);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    પ્રતિજ્ઞા(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::લીટી_Str(અચલ અક્ષર* filename, પૂર્ણાંક lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<પૂર્ણાંક>(0, 20 - static_cast<પૂર્ણાંક>(out.str().length())), ' ');
    પાછા out.str();
}

string V3Error::stripMetaText(અચલ string& text, તાર્કિક stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    પૂર્ણાંક inBegins = 0;
    માટે (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        જો (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            જો (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                જ્યારે (pos < text.size() && text[pos] != ')') ++pos;
                જો (pos < text.size() && text[pos] == ')') ++pos;
                ચાલુ;
            }
            જો (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                ચાલુ;
            }
            જો (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                ચાલુ;
            }
        }
        જો (!stripContext || inBegins == 0) {
            // Remove double newlines
            જો (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    પાછા result;
}

શૂન્ય V3Error::abortIfWarnings() {
    જો (!isErrorOrWarn()) પાછા;
    અચલ તાર્કિક exwarn = warnFatal() && warn_ગણતરી();
    જો (error_ગણતરી() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ગણતરી() << " error(s), "  //
                                      << V3Error::s().warn_ગણતરી() << " warning(s)\n");
    } નહીંતર જો (error_ગણતરી()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().error_ગણતરી() << " error(s)\n");
    } નહીંતર {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warn_ગણતરી() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

શૂન્ય V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    પાછા v3errorStr();
}
std::ostringstream& V3Error::v3error_Prep_File_લીટી(V3ErrorCode code, અચલ અક્ષર* file, પૂર્ણાંક લીટી)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << લીટી << ": ";
    પાછા v3errorStr();
}
શૂન્ય V3Error::v3errorEnd(અચલ std::ostringstream& sstr, અચલ string& extra, File_લીટી* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

શૂન્ય v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
શૂન્ય v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    પ્રતિજ્ઞા(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
