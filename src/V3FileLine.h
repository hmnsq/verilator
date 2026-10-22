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

#ifndef VERILATOR_V3FILELINE_H_
#define VERILATOR_V3FILELINE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3Hash.h"
#ضمن "V3LangCode.h"
#ضمن "V3Mutex.h"

#ضمن <atomic>
#ضمن <deque>
#ضمن <map>
#ضمن <memory>
#ضمن <set>
#ضمن <sstream>
#ضمن <unordered_map>
#ضمن <vector>

// ######################################################################

class FileLine;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class FileLineSingleton final {
    friend class FileLine;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using msgEnSetIdx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class MsgEnBitSet final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    عام:
        تعداد class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        MsgEnBitSet() = افتراضي;
        // Create intersection set
        MsgEnBitSet(ثابت MsgEnBitSet& a, ثابت MsgEnBitSet& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        بنية Hash final {
            size_t operator()(ثابت MsgEnBitSet& item) ثابت {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                أرجع hash.value();
            }
        };

        بنية Equal final {
            bool operator()(ثابت MsgEnBitSet& a, ثابت MsgEnBitSet& b) ثابت { أرجع a == b; }
        };

        bool operator==(ثابت MsgEnBitSet& other) ثابت {
            أرجع m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        ثابت VErrorBitSet& getAll(Subset subset) ثابت {
            أرجع subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) ثابت {
            أرجع subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        فراغ setAll(Subset subset, ثابت VErrorBitSet& bitset) {
            𐡄𐡍 (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } وإلا {
                m_ctrlEn = bitset;
            }
        }
        فراغ set(Subset subset, V3ErrorCode code, bool value) {
            𐡄𐡍 (subset == Subset::CODE) {
                m_codeEn.set(code, value);
            } وإلا {
                m_ctrlEn.set(code, value);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) ثابت { أرجع m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ثابت std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<MsgEnBitSet, msgEnSetIdx_t, MsgEnBitSet::Hash, MsgEnBitSet::Equal>
        m_internedMsgEnIdxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<MsgEnBitSet> m_internedMsgEns;

    // CONSTRUCTORS
    FileLineSingleton() = افتراضي;
    ~FileLineSingleton() = افتراضي;

    fileNameIdx_t nameToNumber(ثابت string& filename);
    ثابت std::string& numberToName(fileNameIdx_t filenameno) ثابت VL_MT_SAFE {
        أرجع m_names[filenameno];
    }
    V3LangCode numberToLang(fileNameIdx_t filenameno) ثابت { أرجع m_languages[filenameno]; }
    فراغ numberToLang(fileNameIdx_t filenameno, ثابت V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    فراغ clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    فراغ fileNameNumMapDumpJson(std::ostream& os);
    ساكن string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    msgEnSetIdx_t addMsgEnBitSet(ثابت MsgEnBitSet& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    msgEnSetIdx_t defaultMsgEnIndex() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    msgEnSetIdx_t msgEnSetBit(msgEnSetIdx_t setIdx, MsgEnBitSet::Subset subset, V3ErrorCode code,
                              bool value);
    // Bulk-set all control codes to given bitset
    msgEnSetIdx_t msgSetCtrlBitSet(msgEnSetIdx_t setIdx, ثابت VErrorBitSet& bitset);
    // Return index to intersection set
    msgEnSetIdx_t msgEnAnd(msgEnSetIdx_t lhsIdx, msgEnSetIdx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ثابت MsgEnBitSet& msgEn(msgEnSetIdx_t idx) ثابت VL_MT_SAFE {
        أرجع m_internedMsgEns.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class FileLine;
    // MEMBERS
    صحيح m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_refCount{0};
    std::deque<string> m_lines;  // Source text lines
    VFileContent() {
        ساكن صحيح s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = افتراضي;
    // METHODS
    فراغ refInc() { ++m_refCount; }
    فراغ refDec() {
        𐡄𐡍 (!--m_refCount) delete this;
    }

عام:
    فراغ pushText(ثابت string& text);  // Add arbitrary text (need not be line-by-line)
    string getLine(صحيح lineno) ثابت VL_MT_SAFE;
    string ascii() ثابت { أرجع "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class FileLine final {

    // CONSTANTS
    ساكن constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = FileLineSingleton::fileNameIdx_t;
    using msgEnSetIdx_t = FileLineSingleton::msgEnSetIdx_t;
    using MsgEnBitSet = FileLineSingleton::MsgEnBitSet;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    msgEnSetIdx_t m_msgEnIdx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_tokenNum = 0;  // Token number in processing order
    صحيح m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    FileLine* m_parent = nullptr;  // Parent line that included this line

محمي:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class FileLineSingleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

خاص:
    // CONSTRUCTORS
    ساكن FileLineSingleton& singleton() VL_MT_SAFE {
        ساكن FileLineSingleton s;
        أرجع s;
    }
    ساكن FileLine& defaultFileLine() VL_MT_SAFE {
        ساكن FileLine s;
        أرجع s;
    }

    FileLine()  // Only used for defaultFileLine above
        : m_msgEnIdx{singleton().defaultMsgEnIndex()}
        , m_filenameno{singleton().nameToNumber(FileLine::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

عام:
    explicit FileLine(ثابت string& filename)
        : m_msgEnIdx{defaultFileLine().m_msgEnIdx}
        , m_filenameno{singleton().nameToNumber(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit FileLine(ثابت FileLine& from)
        : m_msgEnIdx{from.m_msgEnIdx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_tokenNum{from.m_tokenNum}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        𐡄𐡍 (m_contentp) m_contentp->refInc();
    }
    explicit FileLine(FileLine* fromp)
        : m_msgEnIdx{fromp->m_msgEnIdx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_tokenNum{fromp->m_tokenNum}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        𐡄𐡍 (m_contentp) m_contentp->refInc();
    }
    فراغ applyIgnores();
    FileLine* copyOrSameFileLine();
    FileLine* copyOrSameFileLineApplied();
    ساكن فراغ deleteAllRemaining();
    ساكن فراغ stats();
    ~FileLine();
#ifdef VL_LEAK_CHECKS
    ساكن فراغ* operator new(size_t size);
    ساكن فراغ operator delete(فراغ* obj, size_t size);
#endif
    // METHODS
    فراغ newContent();
    فراغ contentLineno(صحيح num) {
        lineno(num);
        m_contentLineno = static_cast<unsigned>(num);
    }
    فراغ contentLinenoFrom(ثابت FileLine* fromp) { m_contentLineno = fromp->m_contentLineno; }
    فراغ lineno(صحيح num) {
        m_firstLineno = num;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    فراغ column(صحيح firstColumn, صحيح lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    فراغ language(V3LangCode lang) ثابت { singleton().numberToLang(filenameno(), lang); }
    فراغ filename(ثابت string& name) { m_filenameno = singleton().nameToNumber(name); }
    فراغ parent(FileLine* fileline) { m_parent = fileline; }
    فراغ lineDirective(ثابت حرف* textp, صحيح& enterExitRef);
    فراغ lineDirectiveParse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                            صحيح& enterExitRef);
    فراغ linenoInc() {
        𐡄𐡍 (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } وإلا {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    فراغ startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    فراغ forwardToken(ثابت حرف* textp, size_t size, bool trackLines = true);
    uint32_t tokenNum() ثابت VL_MT_SAFE { أرجع m_tokenNum; }
    صحيح firstLineno() ثابت VL_MT_SAFE { أرجع m_firstLineno; }
    صحيح firstColumn() ثابت VL_MT_SAFE { أرجع static_cast<صحيح>(m_firstColumn); }
    صحيح lastLineno() ثابت VL_MT_SAFE { أرجع m_firstLineno + m_lastLinenoAdder; }
    صحيح lastColumn() ثابت VL_MT_SAFE { أرجع static_cast<صحيح>(m_lastColumn); }
    VFileContent* contentp() ثابت { أرجع m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    صحيح lineno() ثابت VL_MT_SAFE { أرجع lastLineno(); }
    string source() ثابت VL_MT_SAFE;
    string sourcePrefix(صحيح toColumn) ثابت VL_MT_SAFE;
    string prettySource() ثابت VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    FileLine* parent() ثابت VL_MT_SAFE { أرجع m_parent; }
    V3LangCode language() ثابت { أرجع singleton().numberToLang(filenameno()); }
    string ascii() ثابت VL_MT_SAFE;
    string asciiLineCol() ثابت;
    صحيح filenameno() ثابت VL_MT_SAFE { أرجع m_filenameno; }
    ثابت std::string& filename() ثابت VL_MT_SAFE {
        أرجع singleton().numberToName(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ثابت VL_MT_SAFE { أرجع VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ثابت VL_MT_SAFE {
        أرجع (filename() == commandLineFilename() || filename() == builtInFilename());
    }
    string filenameLetters() ثابت VL_MT_SAFE {
        أرجع FileLineSingleton::filenameLetters(filenameno());
    }
    string filebasename() ثابت VL_MT_SAFE;
    string filebasenameNoExt() ثابت;
    string firstColumnLetters() ثابت VL_MT_SAFE;
    string profileFuncname() ثابت;
    string lineDirectiveStrg(صحيح enterExit) ثابت;

    // Turn on/off warning messages on this line.
خاص:
    فراغ warnSet(MsgEnBitSet::Subset subset, V3ErrorCode code, bool flag) {
        m_msgEnIdx = singleton().msgEnSetBit(m_msgEnIdx, subset, code, flag);
    }

عام:
    فراغ warnOn(V3ErrorCode code, bool flag) { warnSet(MsgEnBitSet::Subset::CODE, code, flag); }
    فراغ warnSetCtrlBitSet(ثابت VErrorBitSet& bitset) {
        m_msgEnIdx = singleton().msgSetCtrlBitSet(m_msgEnIdx, bitset);
    }
    فراغ warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(ثابت string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) ثابت VL_MT_SAFE;
    فراغ warnLintOff(bool turnOff);
    فراغ warnStyleOff(bool turnOff);
    فراغ warnStateFrom(ثابت FileLine& from) { m_msgEnIdx = from.m_msgEnIdx; }
    فراغ warnResetDefault() { warnStateFrom(defaultFileLine()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ثابت { أرجع msgEn().enabled(V3ErrorCode::I_CELLDEFINE); }
    فراغ celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() ثابت { أرجع msgEn().enabled(V3ErrorCode::I_COVERAGE); }
    فراغ coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() ثابت { أرجع msgEn().enabled(V3ErrorCode::I_ERRORING); }
    فراغ erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() ثابت { أرجع msgEn().enabled(V3ErrorCode::I_TRACING); }
    فراغ tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() ثابت { أرجع msgEn().enabled(V3ErrorCode::I_TIMING); }
    فراغ timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ساكن string commandLineFilename() VL_MT_SAFE { أرجع "<command-line>"; }
    ساكن string builtInFilename() VL_MT_SAFE { أرجع "<built-in>"; }
    ساكن فراغ globalWarnOff(V3ErrorCode code, bool turnOff) {
        defaultFileLine().warnOff(code, turnOff);
    }
    ساكن string globalWarnOffParse(ثابت string& msgs, bool turnOff) {
        أرجع defaultFileLine().warnOffParse(msgs, turnOff);
    }
    ساكن فراغ fileNameNumMapDumpJson(std::ostream& os) {
        singleton().fileNameNumMapDumpJson(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    فراغ modifyStateInherit(ثابت FileLine* fromp) {
        m_msgEnIdx = singleton().msgEnAnd(m_msgEnIdx, fromp->m_msgEnIdx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    فراغ modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    فراغ v3errorEnd(std::ostringstream& str, ثابت string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        أرجع warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ثابت { أرجع warnContext(); }
    bool operator==(ثابت FileLine& rhs) ثابت {
        أرجع (m_tokenNum == rhs.m_tokenNum && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_msgEnIdx == rhs.m_msgEnIdx);
    }
    bool equalFirstLineCol(ثابت FileLine& rhs) ثابت {
        أرجع (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    صحيح operatorCompare(ثابت FileLine& rhs) ثابت {
        𐡄𐡍 (m_filenameno != rhs.m_filenameno) أرجع (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        𐡄𐡍 (m_firstLineno != rhs.m_firstLineno)
            أرجع (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        𐡄𐡍 (m_firstColumn != rhs.m_firstColumn)
            أرجع (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        𐡄𐡍 (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            أرجع (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        𐡄𐡍 (m_lastColumn != rhs.m_lastColumn) أرجع (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        ثابت MsgEnBitSet& lhsMsgEn = msgEn();
        ثابت MsgEnBitSet& rhsMsgEn = rhs.msgEn();
        لكل (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            ثابت V3ErrorCode code = static_cast<V3ErrorCode>(i);
            𐡄𐡍 (lhsMsgEn.enabled(code) != rhsMsgEn.enabled(code))
                أرجع rhsMsgEn.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        𐡄𐡍 (m_tokenNum != rhs.m_tokenNum) أرجع (m_tokenNum < rhs.m_tokenNum) ? -1 : 1;
        أرجع 0;  // (*this) and rhs are equivalent
    }

خاص:
    string warnContext() ثابت;
    string warnContextParent() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    ثابت MsgEnBitSet& msgEn() ثابت VL_MT_SAFE { أرجع singleton().msgEn(m_msgEnIdx); }
};
std::ostream& operator<<(std::ostream& os, FileLine* fileline);

#endif  // Guard
