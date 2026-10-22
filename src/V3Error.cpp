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
#include "V3Error.h"
#include "V3Os.h"
#ifndef V3ERROR_NO_GLOBAL_
# include "V3Ast.h"
# include "V3DiagSarif.h"
# include "V3Global.h"
# include "V3Stats.h"
VL_DEFINE_DEBUG_FUNCTIONS;
#endif
#include <thread>
// clang-format on

//======================================================================

sa_nracanaa v3errorIniter final {
    v3errorIniter() { V3Error::init(); }
};
v3errorIniter v3errorInit;

//######################################################################
// ErrorCode class functions

V3ErrorCode::V3ErrorCode(const var_na* msgp) {
    // Return error encoding for given string, or ERROR, which is a bad code
    ສຳລັບ (ຈຳນວນເຕັມ codei = V3ErrorCode::EC_MIN; codei < V3ErrorCode::_ENUM_MAX; codei++) {
        const V3ErrorCode code{codei};
        ຖ້າ (0 == VL_STRCASECMP(msgp, code.ascii())) {
            m_e = code;
            ຖ້າ (isRenamed()) m_e = renamedTo().m_e;
            ສົ່ງຄືນ;
        }
    }
    m_e = V3ErrorCode::EC_ERROR;
}

string V3ErrorCode::url() const {
    ຖ້າ (!isNamed())
        ສົ່ງຄືນ "https://verilator.org/verilator_doc.html"s + "?v=" + PACKAGE_VERSION_NUMBER_STRING;
    ສົ່ງຄືນ "https://verilator.org/warn/"s + ascii() + "?v=" + PACKAGE_VERSION_NUMBER_STRING;
}

//######################################################################
// V3ErrorGuarded class functions
//

bool V3ErrorGuarded::isError(V3ErrorCode code, bool supp) VL_REQUIRES(m_mutex) {
    ຖ້າ (code.hardError()) ສົ່ງຄືນ true;
    ຖ້າ (supp) ສົ່ງຄືນ false;
    ຖ້າ (code.severityInfo()) ສົ່ງຄືນ false;
    ຖ້າ (pretendError(code)) ສົ່ງຄືນ true;
    ສົ່ງຄືນ false;
}

string V3ErrorGuarded::msgPrefix() VL_REQUIRES(m_mutex) {
    const V3ErrorCode code = m_message.code();
    const bool supp = m_errorSuppressed;
    ຖ້າ (supp) ສົ່ງຄືນ "-arning-suppressed-" + std::string{code.ascii()} + ": ";
    ຖ້າ (code.severityInfo()) ສົ່ງຄືນ "-Info: ";
    ຖ້າ (code == V3ErrorCode::EC_FATAL) ສົ່ງຄືນ "%Error: ";
    ຖ້າ (code == V3ErrorCode::EC_FATALMANY) ສົ່ງຄືນ "%Error: ";
    ຖ້າ (code == V3ErrorCode::EC_FATALSRC) ສົ່ງຄືນ "%Error: Internal Error: ";
    ຖ້າ (code == V3ErrorCode::EC_ERROR) ສົ່ງຄືນ "%Error: ";
    ຖ້າ (isError(code, supp)) ສົ່ງຄືນ "%Error-" + std::string{code.ascii()} + ": ";
    ສົ່ງຄືນ "%Warning-" + std::string{code.ascii()} + ": ";
}

void V3ErrorGuarded::vlAbortOrExit() VL_REQUIRES(m_mutex) {
    ຖ້າ (V3Error::debugDefault()) {
        std::cerr << msgPrefix() << "Aborting since under --debug" << endl;
        V3Error::vlAbort();
    }

#ifndef V3ERROR_NO_GLOBAL_
    ຖ້າ (v3Global.opt.verilateJobs() > 1 && v3Global.mainThreadId() != std::this_thread::get_id()) {
        // No static destructors are called, thus must be called manually.
        VL_GCOV_DUMP();
        // Exit without triggering any global destructors. Used to prevent
        // detached V3ThreadPool jobs accessing destroyed static objects.
        ::_exit(1);  // LCOV_EXCL_LINE
    }
    v3Global.vlExit(1);
#ບໍ່ດັ່ງນັ້ນ
    std::exit(1);
#endif
}

string V3ErrorGuarded::warnMoreSpaces() VL_REQUIRES(m_mutex) {
    UASSERT_STATIC(!m_message.isClear(), "warnMore() outside of v3errorPrep...v3errorEnd");
    ສົ່ງຄືນ string(msgPrefix().size(), ' ');
}

void V3ErrorGuarded::suppressThisWarning() VL_REQUIRES(m_mutex) {
#ifndef V3ERROR_NO_GLOBAL_
    V3Stats::addStatSum("Warnings, Suppressed "s + errorCode().ascii(), 1);
#endif
    errorSuppressed(true);
}

void V3ErrorGuarded::v3errorPrep(V3ErrorCode code) VL_REQUIRES(m_mutex) {
    m_errorStr.str("");
    UASSERT_STATIC(m_message.isClear(), "Attempted v3errorPrep inside v3errorPrep...v3errorEnd");
    m_message.init(code);
    m_errorContexted = false;
    m_errorSuppressed = false;
}

void V3ErrorGuarded::v3errorEnd(const std::ostringstream& sstr, const string& extra,
                                FileLine* fileline) VL_REQUIRES(m_mutex) {
    static bool s_firedTooMany = false;
    v3errorEndGuts(sstr, extra, fileline);
    m_message.clear();
    ຖ້າ (errorLimit() && errorCount() >= errorLimit() && !s_firedTooMany) {
        s_firedTooMany = true;
        // Recurses here
        v3errorEnd((v3errorPrep(V3ErrorCode::EC_FATALMANY),
                    (v3errorStr() << "Exiting due to too many errors encountered; --error-limit="
                                  << errorCount() << std::endl),
                    v3errorStr()),
                   "", nullptr);
        assert(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
}

// cppcheck-has-bug-suppress constParameter
void V3ErrorGuarded::v3errorEndGuts(const std::ostringstream& sstr, const string& extra,
                                    FileLine* fileline) VL_REQUIRES(m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
#ຖ້າ defined(__COVERITY__) || defined(__cppcheck__)
    ຖ້າ (m_errorCode == V3ErrorCode::EC_FATAL) __coverity_panic__(x);
#endif

    UASSERT_STATIC(!m_message.isClear(), "v3errorEnd() outside of v3errorPrep...v3errorEnd");
    m_message.fileline(fileline);

    // Skip suppressed messages
    ຖ້າ (m_errorSuppressed) {
        // On debug, show only non default-off warning to prevent pages of warnings
        ຖ້າ (m_message.code().defaultsOff()) ສົ່ງຄືນ;
        ຖ້າ (debug() < 3 || (debug() < 9 && m_showedSuppressed.test(m_message.code()))) ສົ່ງຄືນ;
        m_showedSuppressed.set(m_message.code(), true);
    }
    string msg
        = V3Error::warnContextBegin() + msgPrefix() + V3Error::warnContextEnd() + sstr.str();

    // If suppressed print only first line to reduce verbosity
    string firstLine = msg;
    {
        const string::size_type pos = firstLine.find('\n');
        ຖ້າ (pos != string::npos) {
            firstLine.erase(pos, firstLine.length() - pos);
            firstLine += "...";
        }
    }
    ຖ້າ (m_errorSuppressed) msg = firstLine;
    // Suppress duplicate messages
    ຖ້າ (!m_messages.insert(firstLine).second) ສົ່ງຄືນ;

    msg = VString::replaceSubstr(msg, V3Error::warnMore(), warnMoreSpaces());

    string msg_additional;
    {
        const string::size_type pos = msg.find(V3Error::warnAdditionalInfo());
        ຖ້າ (pos != string::npos) {
            msg_additional = msg.substr(pos + V3Error::warnAdditionalInfo().size());
            msg.erase(pos);
        }
    }
    msg += '\n';  // Trailing newlines generally not put on messages so add
    ຖ້າ (!extra.empty() && !m_errorSuppressed) {
        string extraMsg = VString::replaceSubstr(extra, V3Error::warnMore(), warnMoreSpaces());
        extraMsg = warnMoreSpaces() + extraMsg + "\n";
        const size_t pos = msg.find('\n');
        msg.insert(pos + 1, extraMsg);
    }
    // Output
    string text;
    ຖ້າ (
#ifndef V3ERROR_NO_GLOBAL_
        !(v3Global.opt.quietExit() && m_message.code() == V3ErrorCode::EC_FATALMANY)
#ບໍ່ດັ່ງນັ້ນ
        true
#endif
    )
        text += msg;
    ຖ້າ (m_errorSuppressed || m_message.code().severityInfo()) {
        std::cerr << V3Error::stripMetaText(text, false);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::pushMessage(m_message);
#endif
    } ບໍ່ດັ່ງນັ້ນ {
        const bool anError = isError(m_message.code(), m_errorSuppressed);
        ຖ້າ (m_message.code()
                != V3ErrorCode::EC_FATALMANY  // Not verbose on final too-many-errors error
            && !m_describedEachWarn.test(m_message.code())) {
            m_describedEachWarn.set(m_message.code(), true);
            ຖ້າ (m_message.code() >= V3ErrorCode::EC_FIRST_NAMED) {
                text += warnMoreSpaces() + "... For " + (anError ? "error" : "warning")
                        + " description see " + m_message.code().url() + '\n';
            } ບໍ່ດັ່ງນັ້ນ ຖ້າ (m_errCount >= 1 && m_message.code().severityFatal() && !m_tellInternal) {
                m_tellInternal = true;
                text += warnMoreSpaces()
                        + "... This fatal error may be caused by the earlier error(s);"
                          " resolve those first.\n";
            } ບໍ່ດັ່ງນັ້ນ ຖ້າ (!m_tellManual) {
                m_tellManual = true;
                text += warnMoreSpaces() + "... See the manual at " + m_message.code().url()
                        + " for more assistance.\n";
            }
            ຖ້າ (!m_pretendError.test(m_message.code()) && !m_message.code().hardError()) {
                text += warnMoreSpaces() + "... Use \"/* verilator lint_off "
                        + m_message.code().ascii()
                        + " */\" and lint_on around source to disable this message.\n";
                ຖ້າ (m_message.code().dangerous()) {
                    text += warnMoreSpaces() + "*** See " + m_message.code().url()
                            + " before disabling this,\n";
                    text += warnMoreSpaces() + "else you may end up with different sim results.\n";
                }
            }
        }
        ຖ້າ (!msg_additional.empty()) text += msg_additional;
        std::cerr << V3Error::stripMetaText(text, false);
        m_message.text(text);
#ifndef V3ERROR_NO_GLOBAL_
        V3DiagSarif::pushMessage(m_message);
#endif

        ຖ້າ (anError) {
            incErrors();
        } ບໍ່ດັ່ງນັ້ນ {
            incWarnings();
        }
        ຖ້າ (m_message.code().severityFatal()) {
            static bool s_inFatal = false;
            // cppcheck-suppress duplicateConditionalAssign // Used by VlcMain.cpp
            ຖ້າ (!s_inFatal) {
                s_inFatal = true;
#ifndef V3ERROR_NO_GLOBAL_
                ຖ້າ (dumpTreeLevel() || dumpTreeJsonLevel() || debug()) {
                    V3Broken::allowMidvisitorCheck(true);
                    ຖ້າ (dumpTreeLevel()) {
                        v3Global.rootp()->dumpTreeFile(v3Global.debugFilename("final.tree", 990));
                    }
                    ຖ້າ (dumpTreeJsonLevel()) {
                        v3Global.rootp()->dumpTreeJsonFile(
                            v3Global.debugFilename("final.tree.json", 990));
                    }
                    ຖ້າ (debug()) {
                        execErrorExitCb();
                        V3Stats::statsStageAll(v3Global.rootp(), "Final");
                        V3Stats::statsReport();
                    }
                    vlAbortOrExit();
                }
                V3DiagSarif::output(false);
#endif
            }

            vlAbortOrExit();
        } ບໍ່ດັ່ງນັ້ນ ຖ້າ (anError) {
            // We don't dump tree on any error because a Visitor may be in middle of
            // a tree cleanup and cause a false broken problem.
            execErrorExitCb();
        }
    }
}

//######################################################################
// V3Error class functions

void V3Error::init() {
    ສຳລັບ (ຈຳນວນເຕັມ i = 0; i < V3ErrorCode::_ENUM_MAX; i++) {
        const V3ErrorCode code{i};
        describedEachWarn(code, false);
        pretendError(code, code.pretendError());
    }
    // Not an UASSERT as failure would call V3Error and it's broken due to this
    assert(std::string{V3ErrorCode{V3ErrorCode::_ENUM_MAX}.ascii()} == " MAX");
}

string V3Error::lineStr(const var_na* filename, ຈຳນວນເຕັມ lineno) VL_PURE {
    std::ostringstream out;
    out << V3Os::filenameNonDir(filename) << ":" << std::dec << lineno << ":";
    out << std::string(std::max<ຈຳນວນເຕັມ>(0, 20 - static_cast<ຈຳນວນເຕັມ>(out.str().length())), ' ');
    ສົ່ງຄືນ out.str();
}

string V3Error::stripMetaText(const string& text, bool stripContext) VL_PURE {
    string result;
    result.reserve(text.size());
    ຈຳນວນເຕັມ inBegins = 0;
    ສຳລັບ (string::size_type pos = 0; pos < text.size();) {
        // string::starts_with is C++20
        ຖ້າ (0 == text.compare(pos, std::strlen("__WARN"), "__WARN")) {
            ຖ້າ (0 == text.compare(pos, std::strlen("__WARNRELATED("), "__WARNRELATED(")) {
                ໃນຂະນະທີ່ (pos < text.size() && text[pos] != ')') ++pos;
                ຖ້າ (pos < text.size() && text[pos] == ')') ++pos;
                continue;
            }
            ຖ້າ (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_BEGIN),
                                V3Error::WARN_CONTEXT_BEGIN)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_BEGIN);
                ++inBegins;
                continue;
            }
            ຖ້າ (0
                == text.compare(pos, std::strlen(V3Error::WARN_CONTEXT_END),
                                V3Error::WARN_CONTEXT_END)) {
                pos += std::strlen(V3Error::WARN_CONTEXT_END);
                // No assert, is not VL_PURE
                // UASSERT_STATIC(inBegins, "warnContextEnd() outside of warnContextBegin()");
                --inBegins;
                continue;
            }
        }
        ຖ້າ (!stripContext || inBegins == 0) {
            // Remove double newlines
            ຖ້າ (!(text[pos] == '\n' && !result.empty() && result[result.size() - 1] == '\n'))
                result += text[pos];
        }
        ++pos;
    }
    ສົ່ງຄືນ result;
}

void V3Error::abortIfWarnings() {
    ຖ້າ (!isErrorOrWarn()) ສົ່ງຄືນ;
    const bool exwarn = warnFatal() && warnCount();
    ຖ້າ (errorCount() && exwarn) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().errorCount() << " error(s), "  //
                                      << V3Error::s().warnCount() << " warning(s)\n");
    } ບໍ່ດັ່ງນັ້ນ ຖ້າ (errorCount()) {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().errorCount() << " error(s)\n");
    } ບໍ່ດັ່ງນັ້ນ {
        v3fatalMany("Exiting due to " << std::dec << V3Error::s().warnCount() << " warning(s)\n");
    }
}

//======================================================================
// Abort/exit

void V3Error::vlAbort() {
#ifndef V3ERROR_NO_GLOBAL_
    v3Global.shutdown();
#endif
    VL_GCOV_DUMP();
    std::abort();  // LCOV_EXCL_LINE
}
std::ostringstream& V3Error::v3errorPrep(V3ErrorCode code) VL_ACQUIRE(s().m_mutex) {
    V3Error::s().m_mutex.lock();
    s().v3errorPrep(code);
    ສົ່ງຄືນ v3errorStr();
}
std::ostringstream& V3Error::v3errorPrepFileLine(V3ErrorCode code, const var_na* file, ຈຳນວນເຕັມ line)
    VL_ACQUIRE(s().m_mutex) {
    v3errorPrep(code) << file << ":" << std::dec << line << ": ";
    ສົ່ງຄືນ v3errorStr();
}
void V3Error::v3errorEnd(const std::ostringstream& sstr, const string& extra, FileLine* fileline)
    VL_RELEASE(s().m_mutex) {
    s().v3errorEnd(sstr, extra, fileline);
    V3Error::s().m_mutex.unlock();
}

void v3errorEnd(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
}
void v3errorEndFatal(std::ostringstream& sstr) VL_RELEASE(V3Error::s().m_mutex) {
    V3Error::v3errorEnd(sstr, "", nullptr);
    assert(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}
