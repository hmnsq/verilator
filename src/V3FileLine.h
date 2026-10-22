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

class File_سطر;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_سطر_Singleton final {
    friend class File_سطر;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using رسالة_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class رسالة_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    عام:
        تعداد class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        رسالة_En_Bit_Set() = افتراضي;
        // Create intersection set
        رسالة_En_Bit_Set(ثابت رسالة_En_Bit_Set& a, ثابت رسالة_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        بنية Hash final {
            size_t operator()(ثابت رسالة_En_Bit_Set& item) ثابت {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                إرجاع hash.قيمة();
            }
        };

        بنية Equal final {
            bool operator()(ثابت رسالة_En_Bit_Set& a, ثابت رسالة_En_Bit_Set& b) ثابت { إرجاع a == b; }
        };

        bool operator==(ثابت رسالة_En_Bit_Set& other) ثابت {
            إرجاع m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        ثابت VErrorBitSet& getAll(Subset subset) ثابت {
            إرجاع subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) ثابت {
            إرجاع subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        فراغ setAll(Subset subset, ثابت VErrorBitSet& bitset) {
            إذا (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } وإلا {
                m_ctrlEn = bitset;
            }
        }
        فراغ set(Subset subset, V3ErrorCode code, bool قيمة) {
            إذا (subset == Subset::CODE) {
                m_codeEn.set(code, قيمة);
            } وإلا {
                m_ctrlEn.set(code, قيمة);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) ثابت { إرجاع m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ثابت std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<رسالة_En_Bit_Set, رسالة_En_Set_Idx_t, رسالة_En_Bit_Set::Hash, رسالة_En_Bit_Set::Equal>
        m_interned_رسالة_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<رسالة_En_Bit_Set> m_interned_رسالة_Ens;

    // CONSTRUCTORS
    File_سطر_Singleton() = افتراضي;
    ~File_سطر_Singleton() = افتراضي;

    fileNameIdx_t name_To_العدد(ثابت string& filename);
    ثابت std::string& العدد_To_Name(fileNameIdx_t filenameno) ثابت VL_MT_SAFE {
        إرجاع m_names[filenameno];
    }
    V3LangCode العدد_To_Lang(fileNameIdx_t filenameno) ثابت { إرجاع m_languages[filenameno]; }
    فراغ العدد_To_Lang(fileNameIdx_t filenameno, ثابت V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    فراغ clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    فراغ file_Name_رقم_Map_Dump_Json(std::ostream& os);
    ساكن string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    رسالة_En_Set_Idx_t add_رسالة_En_Bit_Set(ثابت رسالة_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    رسالة_En_Set_Idx_t default_رسالة_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    رسالة_En_Set_Idx_t رسالة_En_Set_Bit(رسالة_En_Set_Idx_t setIdx, رسالة_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool قيمة);
    // Bulk-set all control codes to given bitset
    رسالة_En_Set_Idx_t رسالة_Set_Ctrl_Bit_Set(رسالة_En_Set_Idx_t setIdx, ثابت VErrorBitSet& bitset);
    // Return index to intersection set
    رسالة_En_Set_Idx_t رسالة_En_And(رسالة_En_Set_Idx_t lhsIdx, رسالة_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ثابت رسالة_En_Bit_Set& رسالة_En(رسالة_En_Set_Idx_t idx) ثابت VL_MT_SAFE {
        إرجاع m_interned_رسالة_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_سطر;
    // MEMBERS
    صحيح m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_تعداد{0};
    std::deque<string> m_أسطر;  // Source text lines
    VFileContent() {
        ساكن صحيح s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = افتراضي;
    // METHODS
    فراغ refInc() { ++m_ref_تعداد; }
    فراغ refDec() {
        إذا (!--m_ref_تعداد) delete this;
    }

عام:
    فراغ ادفع_Text(ثابت string& text);  // Add arbitrary text (need not be line-by-line)
    string get_سطر(صحيح lineno) ثابت VL_MT_SAFE;
    string ascii() ثابت { إرجاع "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_سطر final {

    // CONSTANTS
    ساكن constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_سطر_Singleton::fileNameIdx_t;
    using رسالة_En_Set_Idx_t = File_سطر_Singleton::رسالة_En_Set_Idx_t;
    using رسالة_En_Bit_Set = File_سطر_Singleton::رسالة_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    رسالة_En_Set_Idx_t m_رسالة_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_رقم = 0;  // Token number in processing order
    صحيح m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_سطر* m_parent = nullptr;  // Parent line that included this line

محمي:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_سطر_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

خاص:
    // CONSTRUCTORS
    ساكن File_سطر_Singleton& singleton() VL_MT_SAFE {
        ساكن File_سطر_Singleton s;
        إرجاع s;
    }
    ساكن File_سطر& default_File_سطر() VL_MT_SAFE {
        ساكن File_سطر s;
        إرجاع s;
    }

    File_سطر()  // Only used for defaultFileLine above
        : m_رسالة_En_Idx{singleton().default_رسالة_En_Index()}
        , m_filenameno{singleton().name_To_العدد(File_سطر::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

عام:
    explicit File_سطر(ثابت string& filename)
        : m_رسالة_En_Idx{default_File_سطر().m_رسالة_En_Idx}
        , m_filenameno{singleton().name_To_العدد(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_سطر(ثابت File_سطر& from)
        : m_رسالة_En_Idx{from.m_رسالة_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_رقم{from.m_token_رقم}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        إذا (m_contentp) m_contentp->refInc();
    }
    explicit File_سطر(File_سطر* fromp)
        : m_رسالة_En_Idx{fromp->m_رسالة_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_رقم{fromp->m_token_رقم}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        إذا (m_contentp) m_contentp->refInc();
    }
    فراغ applyIgnores();
    File_سطر* copy_Or_Same_File_سطر();
    File_سطر* copy_Or_Same_File_سطر_Applied();
    ساكن فراغ deleteAllRemaining();
    ساكن فراغ stats();
    ~File_سطر();
#ifdef VL_LEAK_CHECKS
    ساكن فراغ* operator new(size_t size);
    ساكن فراغ operator delete(فراغ* obj, size_t size);
#endif
    // METHODS
    فراغ newContent();
    فراغ contentLineno(صحيح رقم) {
        lineno(رقم);
        m_contentLineno = static_cast<unsigned>(رقم);
    }
    فراغ contentLinenoFrom(ثابت File_سطر* fromp) { m_contentLineno = fromp->m_contentLineno; }
    فراغ lineno(صحيح رقم) {
        m_firstLineno = رقم;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    فراغ column(صحيح firstColumn, صحيح lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    فراغ language(V3LangCode lang) ثابت { singleton().العدد_To_Lang(filenameno(), lang); }
    فراغ filename(ثابت string& name) { m_filenameno = singleton().name_To_العدد(name); }
    فراغ parent(File_سطر* fileline) { m_parent = fileline; }
    فراغ سطر_Directive(ثابت حرف* textp, صحيح& enterExitRef);
    فراغ سطر_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                            صحيح& enterExitRef);
    فراغ linenoInc() {
        إذا (m_lastLinenoAdder == 65535) {
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
    فراغ forwardToken(ثابت حرف* textp, size_t size, bool track_أسطر = true);
    uint32_t token_رقم() ثابت VL_MT_SAFE { إرجاع m_token_رقم; }
    صحيح firstLineno() ثابت VL_MT_SAFE { إرجاع m_firstLineno; }
    صحيح firstColumn() ثابت VL_MT_SAFE { إرجاع static_cast<صحيح>(m_firstColumn); }
    صحيح lastLineno() ثابت VL_MT_SAFE { إرجاع m_firstLineno + m_lastLinenoAdder; }
    صحيح lastColumn() ثابت VL_MT_SAFE { إرجاع static_cast<صحيح>(m_lastColumn); }
    VFileContent* contentp() ثابت { إرجاع m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    صحيح lineno() ثابت VL_MT_SAFE { إرجاع lastLineno(); }
    string source() ثابت VL_MT_SAFE;
    string sourcePrefix(صحيح toColumn) ثابت VL_MT_SAFE;
    string prettySource() ثابت VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_سطر* parent() ثابت VL_MT_SAFE { إرجاع m_parent; }
    V3LangCode language() ثابت { إرجاع singleton().العدد_To_Lang(filenameno()); }
    string ascii() ثابت VL_MT_SAFE;
    string ascii_سطر_Col() ثابت;
    صحيح filenameno() ثابت VL_MT_SAFE { إرجاع m_filenameno; }
    ثابت std::string& filename() ثابت VL_MT_SAFE {
        إرجاع singleton().العدد_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ثابت VL_MT_SAFE { إرجاع VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ثابت VL_MT_SAFE {
        إرجاع (filename() == command_سطر_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() ثابت VL_MT_SAFE {
        إرجاع File_سطر_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ثابت VL_MT_SAFE;
    string filebasenameNoExt() ثابت;
    string firstColumnLetters() ثابت VL_MT_SAFE;
    string profileFuncname() ثابت;
    string سطر_Directive_Strg(صحيح enterExit) ثابت;

    // Turn on/off warning messages on this line.
خاص:
    فراغ warnSet(رسالة_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_رسالة_En_Idx = singleton().رسالة_En_Set_Bit(m_رسالة_En_Idx, subset, code, flag);
    }

عام:
    فراغ warnOn(V3ErrorCode code, bool flag) { warnSet(رسالة_En_Bit_Set::Subset::CODE, code, flag); }
    فراغ warnSetCtrlBitSet(ثابت VErrorBitSet& bitset) {
        m_رسالة_En_Idx = singleton().رسالة_Set_Ctrl_Bit_Set(m_رسالة_En_Idx, bitset);
    }
    فراغ warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(ثابت string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) ثابت VL_MT_SAFE;
    فراغ warnLintOff(bool turnOff);
    فراغ warnStyleOff(bool turnOff);
    فراغ warnStateFrom(ثابت File_سطر& from) { m_رسالة_En_Idx = from.m_رسالة_En_Idx; }
    فراغ warnResetDefault() { warnStateFrom(default_File_سطر()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ثابت { إرجاع رسالة_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    فراغ celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() ثابت { إرجاع رسالة_En().enabled(V3ErrorCode::I_COVERAGE); }
    فراغ coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() ثابت { إرجاع رسالة_En().enabled(V3ErrorCode::I_ERRORING); }
    فراغ erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() ثابت { إرجاع رسالة_En().enabled(V3ErrorCode::I_TRACING); }
    فراغ tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() ثابت { إرجاع رسالة_En().enabled(V3ErrorCode::I_TIMING); }
    فراغ timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ساكن string command_سطر_Filename() VL_MT_SAFE { إرجاع "<command-line>"; }
    ساكن string builtInFilename() VL_MT_SAFE { إرجاع "<built-in>"; }
    ساكن فراغ globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_سطر().warnOff(code, turnOff);
    }
    ساكن string globalWarnOffParse(ثابت string& msgs, bool turnOff) {
        إرجاع default_File_سطر().warnOffParse(msgs, turnOff);
    }
    ساكن فراغ file_Name_رقم_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_رقم_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    فراغ modifyStateInherit(ثابت File_سطر* fromp) {
        m_رسالة_En_Idx = singleton().رسالة_En_And(m_رسالة_En_Idx, fromp->m_رسالة_En_Idx);
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
        إرجاع warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ثابت { إرجاع warnContext(); }
    bool operator==(ثابت File_سطر& rhs) ثابت {
        إرجاع (m_token_رقم == rhs.m_token_رقم && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_رسالة_En_Idx == rhs.m_رسالة_En_Idx);
    }
    bool equal_First_سطر_Col(ثابت File_سطر& rhs) ثابت {
        إرجاع (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    صحيح operatorCompare(ثابت File_سطر& rhs) ثابت {
        إذا (m_filenameno != rhs.m_filenameno) إرجاع (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        إذا (m_firstLineno != rhs.m_firstLineno)
            إرجاع (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        إذا (m_firstColumn != rhs.m_firstColumn)
            إرجاع (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        إذا (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            إرجاع (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        إذا (m_lastColumn != rhs.m_lastColumn) إرجاع (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        ثابت رسالة_En_Bit_Set& lhs_رسالة_En = رسالة_En();
        ثابت رسالة_En_Bit_Set& rhs_رسالة_En = rhs.رسالة_En();
        لكل (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            ثابت V3ErrorCode code = static_cast<V3ErrorCode>(i);
            إذا (lhs_رسالة_En.enabled(code) != rhs_رسالة_En.enabled(code))
                إرجاع rhs_رسالة_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        إذا (m_token_رقم != rhs.m_token_رقم) إرجاع (m_token_رقم < rhs.m_token_رقم) ? -1 : 1;
        إرجاع 0;  // (*this) and rhs are equivalent
    }

خاص:
    string warnContext() ثابت;
    string warnContextParent() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    ثابت رسالة_En_Bit_Set& رسالة_En() ثابت VL_MT_SAFE { إرجاع singleton().رسالة_En(m_رسالة_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_سطر* fileline);

#endif  // Guard
