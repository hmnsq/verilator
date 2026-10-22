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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Hash.h"
#অন্তর্ভাব "V3LangCode.h"
#অন্তর্ভাব "V3Mutex.h"

#অন্তর্ভাব <atomic>
#অন্তর্ভাব <deque>
#অন্তর্ভাব <map>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <set>
#অন্তর্ভাব <sstream>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <vector>

// ######################################################################

class File_সারি;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_সারি_Singleton final {
    friend class File_সারি;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using বার্তা_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class বার্তা_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    সার্বজনিক:
        গণনা class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        বার্তা_En_Bit_Set() = স্বভাব;
        // Create intersection set
        বার্তা_En_Bit_Set(ধ্রুব বার্তা_En_Bit_Set& a, ধ্রুব বার্তা_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        সংরচনা Hash final {
            size_t operator()(ধ্রুব বার্তা_En_Bit_Set& item) ধ্রুব {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                প্রত্যাগম hash.মান();
            }
        };

        সংরচনা Equal final {
            bool operator()(ধ্রুব বার্তা_En_Bit_Set& a, ধ্রুব বার্তা_En_Bit_Set& b) ধ্রুব { প্রত্যাগম a == b; }
        };

        bool operator==(ধ্রুব বার্তা_En_Bit_Set& other) ধ্রুব {
            প্রত্যাগম m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        ধ্রুব VErrorBitSet& getAll(Subset subset) ধ্রুব {
            প্রত্যাগম subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) ধ্রুব {
            প্রত্যাগম subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        শূন্য setAll(Subset subset, ধ্রুব VErrorBitSet& bitset) {
            যদি (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } অন্যথা {
                m_ctrlEn = bitset;
            }
        }
        শূন্য set(Subset subset, V3ErrorCode code, bool মান) {
            যদি (subset == Subset::CODE) {
                m_codeEn.set(code, মান);
            } অন্যথা {
                m_ctrlEn.set(code, মান);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) ধ্রুব { প্রত্যাগম m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ধ্রুব std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<বার্তা_En_Bit_Set, বার্তা_En_Set_Idx_t, বার্তা_En_Bit_Set::Hash, বার্তা_En_Bit_Set::Equal>
        m_interned_বার্তা_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<বার্তা_En_Bit_Set> m_interned_বার্তা_Ens;

    // CONSTRUCTORS
    File_সারি_Singleton() = স্বভাব;
    ~File_সারি_Singleton() = স্বভাব;

    fileNameIdx_t name_To_সংখ্যা(ধ্রুব string& filename);
    ধ্রুব std::string& সংখ্যা_To_Name(fileNameIdx_t filenameno) ধ্রুব VL_MT_SAFE {
        প্রত্যাগম m_names[filenameno];
    }
    V3LangCode সংখ্যা_To_Lang(fileNameIdx_t filenameno) ধ্রুব { প্রত্যাগম m_languages[filenameno]; }
    শূন্য সংখ্যা_To_Lang(fileNameIdx_t filenameno, ধ্রুব V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    শূন্য clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    শূন্য file_Name_অঙ্ক_Map_Dump_Json(std::ostream& os);
    স্থির string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    বার্তা_En_Set_Idx_t add_বার্তা_En_Bit_Set(ধ্রুব বার্তা_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    বার্তা_En_Set_Idx_t default_বার্তা_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    বার্তা_En_Set_Idx_t বার্তা_En_Set_Bit(বার্তা_En_Set_Idx_t setIdx, বার্তা_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool মান);
    // Bulk-set all control codes to given bitset
    বার্তা_En_Set_Idx_t বার্তা_Set_Ctrl_Bit_Set(বার্তা_En_Set_Idx_t setIdx, ধ্রুব VErrorBitSet& bitset);
    // Return index to intersection set
    বার্তা_En_Set_Idx_t বার্তা_En_And(বার্তা_En_Set_Idx_t lhsIdx, বার্তা_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ধ্রুব বার্তা_En_Bit_Set& বার্তা_En(বার্তা_En_Set_Idx_t idx) ধ্রুব VL_MT_SAFE {
        প্রত্যাগম m_interned_বার্তা_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_সারি;
    // MEMBERS
    পূর্ণাঙ্ক m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_গুনতি{0};
    std::deque<string> m_সারিগুলি;  // Source text lines
    VFileContent() {
        স্থির পূর্ণাঙ্ক s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = স্বভাব;
    // METHODS
    শূন্য refInc() { ++m_ref_গুনতি; }
    শূন্য refDec() {
        যদি (!--m_ref_গুনতি) delete this;
    }

সার্বজনিক:
    শূন্য ঠেলো_Text(ধ্রুব string& text);  // Add arbitrary text (need not be line-by-line)
    string get_সারি(পূর্ণাঙ্ক lineno) ধ্রুব VL_MT_SAFE;
    string ascii() ধ্রুব { প্রত্যাগম "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_সারি final {

    // CONSTANTS
    স্থির constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_সারি_Singleton::fileNameIdx_t;
    using বার্তা_En_Set_Idx_t = File_সারি_Singleton::বার্তা_En_Set_Idx_t;
    using বার্তা_En_Bit_Set = File_সারি_Singleton::বার্তা_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    বার্তা_En_Set_Idx_t m_বার্তা_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_অঙ্ক = 0;  // Token number in processing order
    পূর্ণাঙ্ক m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_সারি* m_parent = nullptr;  // Parent line that included this line

রক্ষিত:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_সারি_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

গুপ্ত:
    // CONSTRUCTORS
    স্থির File_সারি_Singleton& singleton() VL_MT_SAFE {
        স্থির File_সারি_Singleton s;
        প্রত্যাগম s;
    }
    স্থির File_সারি& default_File_সারি() VL_MT_SAFE {
        স্থির File_সারি s;
        প্রত্যাগম s;
    }

    File_সারি()  // Only used for defaultFileLine above
        : m_বার্তা_En_Idx{singleton().default_বার্তা_En_Index()}
        , m_filenameno{singleton().name_To_সংখ্যা(File_সারি::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

সার্বজনিক:
    explicit File_সারি(ধ্রুব string& filename)
        : m_বার্তা_En_Idx{default_File_সারি().m_বার্তা_En_Idx}
        , m_filenameno{singleton().name_To_সংখ্যা(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_সারি(ধ্রুব File_সারি& from)
        : m_বার্তা_En_Idx{from.m_বার্তা_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_অঙ্ক{from.m_token_অঙ্ক}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        যদি (m_contentp) m_contentp->refInc();
    }
    explicit File_সারি(File_সারি* fromp)
        : m_বার্তা_En_Idx{fromp->m_বার্তা_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_অঙ্ক{fromp->m_token_অঙ্ক}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        যদি (m_contentp) m_contentp->refInc();
    }
    শূন্য applyIgnores();
    File_সারি* copy_Or_Same_File_সারি();
    File_সারি* copy_Or_Same_File_সারি_Applied();
    স্থির শূন্য deleteAllRemaining();
    স্থির শূন্য stats();
    ~File_সারি();
#ifdef VL_LEAK_CHECKS
    স্থির শূন্য* operator new(size_t size);
    স্থির শূন্য operator delete(শূন্য* obj, size_t size);
#endif
    // METHODS
    শূন্য newContent();
    শূন্য contentLineno(পূর্ণাঙ্ক অঙ্ক) {
        lineno(অঙ্ক);
        m_contentLineno = static_cast<unsigned>(অঙ্ক);
    }
    শূন্য contentLinenoFrom(ধ্রুব File_সারি* fromp) { m_contentLineno = fromp->m_contentLineno; }
    শূন্য lineno(পূর্ণাঙ্ক অঙ্ক) {
        m_firstLineno = অঙ্ক;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    শূন্য column(পূর্ণাঙ্ক firstColumn, পূর্ণাঙ্ক lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    শূন্য language(V3LangCode lang) ধ্রুব { singleton().সংখ্যা_To_Lang(filenameno(), lang); }
    শূন্য filename(ধ্রুব string& name) { m_filenameno = singleton().name_To_সংখ্যা(name); }
    শূন্য parent(File_সারি* fileline) { m_parent = fileline; }
    শূন্য সারি_Directive(ধ্রুব বর্ণ* textp, পূর্ণাঙ্ক& enterExitRef);
    শূন্য সারি_Directive_Parse(ধ্রুব বর্ণ* textp, string& filenameRef, পূর্ণাঙ্ক& linenoRef,
                            পূর্ণাঙ্ক& enterExitRef);
    শূন্য linenoInc() {
        যদি (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } অন্যথা {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    শূন্য startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    শূন্য forwardToken(ধ্রুব বর্ণ* textp, size_t size, bool track_সারিগুলি = true);
    uint32_t token_অঙ্ক() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_token_অঙ্ক; }
    পূর্ণাঙ্ক firstLineno() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_firstLineno; }
    পূর্ণাঙ্ক firstColumn() ধ্রুব VL_MT_SAFE { প্রত্যাগম static_cast<পূর্ণাঙ্ক>(m_firstColumn); }
    পূর্ণাঙ্ক lastLineno() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_firstLineno + m_lastLinenoAdder; }
    পূর্ণাঙ্ক lastColumn() ধ্রুব VL_MT_SAFE { প্রত্যাগম static_cast<পূর্ণাঙ্ক>(m_lastColumn); }
    VFileContent* contentp() ধ্রুব { প্রত্যাগম m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    পূর্ণাঙ্ক lineno() ধ্রুব VL_MT_SAFE { প্রত্যাগম lastLineno(); }
    string source() ধ্রুব VL_MT_SAFE;
    string sourcePrefix(পূর্ণাঙ্ক toColumn) ধ্রুব VL_MT_SAFE;
    string prettySource() ধ্রুব VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_সারি* parent() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_parent; }
    V3LangCode language() ধ্রুব { প্রত্যাগম singleton().সংখ্যা_To_Lang(filenameno()); }
    string ascii() ধ্রুব VL_MT_SAFE;
    string ascii_সারি_Col() ধ্রুব;
    পূর্ণাঙ্ক filenameno() ধ্রুব VL_MT_SAFE { প্রত্যাগম m_filenameno; }
    ধ্রুব std::string& filename() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম singleton().সংখ্যা_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ধ্রুব VL_MT_SAFE { প্রত্যাগম VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম (filename() == command_সারি_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() ধ্রুব VL_MT_SAFE {
        প্রত্যাগম File_সারি_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ধ্রুব VL_MT_SAFE;
    string filebasenameNoExt() ধ্রুব;
    string firstColumnLetters() ধ্রুব VL_MT_SAFE;
    string profileFuncname() ধ্রুব;
    string সারি_Directive_Strg(পূর্ণাঙ্ক enterExit) ধ্রুব;

    // Turn on/off warning messages on this line.
গুপ্ত:
    শূন্য warnSet(বার্তা_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_বার্তা_En_Idx = singleton().বার্তা_En_Set_Bit(m_বার্তা_En_Idx, subset, code, flag);
    }

সার্বজনিক:
    শূন্য warnOn(V3ErrorCode code, bool flag) { warnSet(বার্তা_En_Bit_Set::Subset::CODE, code, flag); }
    শূন্য warnSetCtrlBitSet(ধ্রুব VErrorBitSet& bitset) {
        m_বার্তা_En_Idx = singleton().বার্তা_Set_Ctrl_Bit_Set(m_বার্তা_En_Idx, bitset);
    }
    শূন্য warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(ধ্রুব string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) ধ্রুব VL_MT_SAFE;
    শূন্য warnLintOff(bool turnOff);
    শূন্য warnStyleOff(bool turnOff);
    শূন্য warnStateFrom(ধ্রুব File_সারি& from) { m_বার্তা_En_Idx = from.m_বার্তা_En_Idx; }
    শূন্য warnResetDefault() { warnStateFrom(default_File_সারি()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ধ্রুব { প্রত্যাগম বার্তা_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    শূন্য celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() ধ্রুব { প্রত্যাগম বার্তা_En().enabled(V3ErrorCode::I_COVERAGE); }
    শূন্য coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() ধ্রুব { প্রত্যাগম বার্তা_En().enabled(V3ErrorCode::I_ERRORING); }
    শূন্য erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() ধ্রুব { প্রত্যাগম বার্তা_En().enabled(V3ErrorCode::I_TRACING); }
    শূন্য tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() ধ্রুব { প্রত্যাগম বার্তা_En().enabled(V3ErrorCode::I_TIMING); }
    শূন্য timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    স্থির string command_সারি_Filename() VL_MT_SAFE { প্রত্যাগম "<command-line>"; }
    স্থির string builtInFilename() VL_MT_SAFE { প্রত্যাগম "<built-in>"; }
    স্থির শূন্য globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_সারি().warnOff(code, turnOff);
    }
    স্থির string globalWarnOffParse(ধ্রুব string& msgs, bool turnOff) {
        প্রত্যাগম default_File_সারি().warnOffParse(msgs, turnOff);
    }
    স্থির শূন্য file_Name_অঙ্ক_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_অঙ্ক_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    শূন্য modifyStateInherit(ধ্রুব File_সারি* fromp) {
        m_বার্তা_En_Idx = singleton().বার্তা_En_And(m_বার্তা_En_Idx, fromp->m_বার্তা_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    শূন্য modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    শূন্য v3errorEnd(std::ostringstream& str, ধ্রুব string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    শূন্য v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        প্রতিজ্ঞা(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() ধ্রুব VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ধ্রুব VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ধ্রুব VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        প্রত্যাগম warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ধ্রুব { প্রত্যাগম warnContext(); }
    bool operator==(ধ্রুব File_সারি& rhs) ধ্রুব {
        প্রত্যাগম (m_token_অঙ্ক == rhs.m_token_অঙ্ক && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_বার্তা_En_Idx == rhs.m_বার্তা_En_Idx);
    }
    bool equal_First_সারি_Col(ধ্রুব File_সারি& rhs) ধ্রুব {
        প্রত্যাগম (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    পূর্ণাঙ্ক operatorCompare(ধ্রুব File_সারি& rhs) ধ্রুব {
        যদি (m_filenameno != rhs.m_filenameno) প্রত্যাগম (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        যদি (m_firstLineno != rhs.m_firstLineno)
            প্রত্যাগম (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        যদি (m_firstColumn != rhs.m_firstColumn)
            প্রত্যাগম (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        যদি (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            প্রত্যাগম (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        যদি (m_lastColumn != rhs.m_lastColumn) প্রত্যাগম (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        ধ্রুব বার্তা_En_Bit_Set& lhs_বার্তা_En = বার্তা_En();
        ধ্রুব বার্তা_En_Bit_Set& rhs_বার্তা_En = rhs.বার্তা_En();
        প্রতি (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            ধ্রুব V3ErrorCode code = static_cast<V3ErrorCode>(i);
            যদি (lhs_বার্তা_En.enabled(code) != rhs_বার্তা_En.enabled(code))
                প্রত্যাগম rhs_বার্তা_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        যদি (m_token_অঙ্ক != rhs.m_token_অঙ্ক) প্রত্যাগম (m_token_অঙ্ক < rhs.m_token_অঙ্ক) ? -1 : 1;
        প্রত্যাগম 0;  // (*this) and rhs are equivalent
    }

গুপ্ত:
    string warnContext() ধ্রুব;
    string warnContextParent() ধ্রুব VL_REQUIRES(V3Error::s().m_mutex);
    ধ্রুব বার্তা_En_Bit_Set& বার্তা_En() ধ্রুব VL_MT_SAFE { প্রত্যাগম singleton().বার্তা_En(m_বার্তা_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_সারি* fileline);

#endif  // Guard
