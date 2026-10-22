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
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#ضمن <thread>
// clang-format on

//======================================================================

بنية v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(ثابت حرف* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        ثابت V3ErrorCode code{codei};
        𐤀𐤌 (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            𐤀𐤌 (isRenamed()) m_e = renamedTo().m_e;
            أرجع;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() ثابت {
    𐤀𐤌 (!isNamed())
        أرجع "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_NUMBER_STRING;
    أرجع "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_NUMBER_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

منطقي V3ErrorGuarded::isError(V3ErrorCode code, منطقي supp) VL_REQUIRES(m_mutex) {
    𐤀𐤌 (code.hardError()) أرجع true;
    𐤀𐤌 (supp) أرجع خطأ;
    𐤀𐤌 (code.severityInfo()) أرجع خطأ;
    𐤀𐤌 (pretendError(code)) أرجع true;
    أرجع خطأ;
}

string V3ErrorGuarded::msgPrefix() VL_REQUIRES(m_mutex) {
    ثابت V3ErrorCode code = m_message.code();
    ثابت منطقي supp = m_errorSuppressed;
    𐤀𐤌 (supp) أرجع "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    𐤀𐤌 (code.severityInfo()) أرجع "-Info: ";
    𐤀𐤌 (code == V3ErrorCode::EC_FATAL) أرجع "%Error: ";
    𐤀𐤌 (code == V3ErrorCode::EC_FATALMANY) أرجع "%Error: ";
    𐤀𐤌 (code == V3ErrorCode::EC_FATALSRC) أرجع "%Error: Internal Error: ";
    𐤀𐤌 (code == V3ErrorCode::EC_ERROR) أرجع "%Error: ";
    𐤀𐤌 (isError(code, supp)) أرجع "%Error-" + std::string{code.ascii()} + ": ";
    أرجع "%Warning-" + std::string{code.ascii()} + ": ";
}

فراغ V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    𐤀𐤌 (V3Error::debugDefault()) {
        std::cerr << msgPrefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    𐤀𐤌 (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#وإلا
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    أرجع string(msgPrefix().size(), ' ');
}

فراغ V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(true);
}

فراغ V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = خطأ;
    m_errorSuppressed = خطأ;
}

فراغ V3ErrorGuarded::v3errorEnd(ثابت std::ostringstream& sstr, ثابت string& extra,
                                FileLine* fileline) VL_REQUIRES(m_mutex) {
    ساكن منطقي s_firedTooMany = خطأ;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    𐤀𐤌 (errorLimit() && errorCount() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = true;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << errorCount() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
فراغ V3ErrorGuarded::v3errorEndGuts(ثابت std::ostringstream& sstr, ثابت string& extra,
                                    FileLine* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#𐤀𐤌 defined(__COVERITY__) || defined(__cppcheck__)
    𐤀𐤌 (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    𐤀𐤌 (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        𐤀𐤌 (m_message.code().defaultsOff()) أرجع;
        𐤀𐤌 (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) أرجع;
        m_showedSuppressed.set(m_message.code(), true);
    }
    string msg
        = V3Error::warnContextBegin() + msgPrefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string firstLine = msg;
    {
        ثابت string::size_type pos = firstLine.find('\n');
        𐤀𐤌 (pos != string::npos) {
            firstLine.erase(pos, firstLine.length() - pos);
            firstLine += "...";
        }
    }
    𐤀𐤌 (m_errorSuppressed) msg = firstLine;
    // Suppress duplicate messages
    𐤀𐤌 (!m_messages.insert(firstLine).second) أرجع;

    msg = VString::replaceSubstr(msg, V3Error::warnMore(), warnMoreSpaces());

    string msg_additional;
    {
        ثابت string::size_type pos = msg.find(V3Error::warnAdditionalInfo());
        𐤀𐤌 (pos != string::npos) {
            msg_additional = msg.substr(pos + V3Error::warnAdditionalInfo().size());
            msg.erase(pos);
        }
    }
    msg += '\n';  // Trailing newlines generally not put on messages so add
    𐤀𐤌 (!extra.empty() && !m_errorSuppressed) {
        string extraMsg = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extraMsg = warnMoreSpaces() + extraMsg + "\n";
        ثابت size_t pos = msg.find('\n');
        msg.insert(pos + 1, extraMsg);
    }
    // Output
    string text;
    𐤀𐤌 (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#وإلا
        true
#endif
    )
        text += msg;
    𐤀𐤌 (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, خطأ);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::pushMessage(m_message);
#endif
    } وإلا {
        ثابت منطقي anError = isError(m_message.code(), m_errorSuppressed);
        𐤀𐤌 (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), true);
            𐤀𐤌 (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } وإلا 𐤀𐤌 (m_errCount >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = true;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } وإلا 𐤀𐤌 (!m_tellManual) {
                m_tellManual = true;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            𐤀𐤌 (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                𐤀𐤌 (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        𐤀𐤌 (!msg_additional.empty()) text += msg_additional;
        std::cerr << V3Error::stripMetaText(text, خطأ);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::pushMessage(m_message);
#endif

        𐤀𐤌 (anError) {
            incErrors();
        } وإلا {
            incWarnings();
        }
        𐤀𐤌 (m_message.code().severityFatal()) {
            ساكن منطقي s_inFatal = خطأ;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            𐤀𐤌 (!s_inFatal) {
                s_inFatal = true;
#ifndef V3ERROR_NO_GLOBAL_
                𐤀𐤌 (dumpTreeLevel() || dumpTreeJsonLevel() || debug()) {
                    V3Broken::allowMidvisitorCheck(true);
                    𐤀𐤌 (dumpTreeLevel()) {
                        v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("final.tree", 990));
                    }
                    𐤀𐤌 (dumpTreeJsonLevel()) {
                        v3Global.rootp()->dumpTreeJsonFile(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    𐤀𐤌 (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(خطأ);
#endif
            }

            vlAbortOrExit();
        } وإلا 𐤀𐤌 (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

فراغ V3Error::init() {
    لكل (صحيح i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        ثابت V3ErrorCode code{i};
        describedEachWarn(code, خطأ);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    أكد(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::lineStr(ثابت حرف* filename, صحيح lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<صحيح>(0, 20 - static_cast<صحيح>(out.str().length())), ' ');
    أرجع out.str();
}

string V3Error::stripMetaText(ثابت string& text, منطقي stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    صحيح inBegins = 0;
    لكل (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        𐤀𐤌 (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            𐤀𐤌 (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                طالما (pos < text.size() && text[pos] != ')') ++pos;
                𐤀𐤌 (pos < text.size() && text[pos] == ')') ++pos;
                استمر;
            }
            𐤀𐤌 (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                استمر;
            }
            𐤀𐤌 (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                استمر;
            }
        }
        𐤀𐤌 (!stripContext || inBegins == 0) {
            // Remove double newlines
            𐤀𐤌 (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    أرجع result;
}

فراغ V3Error::abortIfWarnings() {
    𐤀𐤌 (!isErrorOrWarn()) أرجع;
    ثابت منطقي exwarn = warnFatal() && warnCount();
    𐤀𐤌 (errorCount() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().errorCount() << " error(s), "  //
                                      << V3Error::s().warnCount() << " warning(s)\n");
    } وإلا 𐤀𐤌 (errorCount()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().errorCount() << " error(s)\n");
    } وإلا {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warnCount() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

فراغ V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    أرجع v3errorStr();
}
std::ostringstream& V3Error::v3errorPrepFileLine(V3ErrorCode code, ثابت حرف* file, صحيح line)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << line << ": ";
    أرجع v3errorStr();
}
فراغ V3Error::v3errorEnd(ثابت std::ostringstream& sstr, ثابت string& extra, FileLine* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

فراغ v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
فراغ v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
