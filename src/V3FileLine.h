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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Hash.h"
#અન્તર્ભાવ "V3LangCode.h"
#અન્તર્ભાવ "V3Mutex.h"

#અન્તર્ભાવ <atomic>
#અન્તર્ભાવ <deque>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <sstream>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

// ######################################################################

class File_લીટી;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_લીટી_Singleton final {
    friend class File_લીટી;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using સંદેશ_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class સંદેશ_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    જાહેર:
        ગણના class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        સંદેશ_En_Bit_Set() = મૂળભૂત;
        // Create intersection set
        સંદેશ_En_Bit_Set(અચલ સંદેશ_En_Bit_Set& a, અચલ સંદેશ_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        સંરચના Hash final {
            size_t operator()(અચલ સંદેશ_En_Bit_Set& item) અચલ {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                પાછા hash.કિંમત();
            }
        };

        સંરચના Equal final {
            bool operator()(અચલ સંદેશ_En_Bit_Set& a, અચલ સંદેશ_En_Bit_Set& b) અચલ { પાછા a == b; }
        };

        bool operator==(અચલ સંદેશ_En_Bit_Set& other) અચલ {
            પાછા m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        અચલ VErrorBitSet& getAll(Subset subset) અચલ {
            પાછા subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) અચલ {
            પાછા subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        શૂન્ય setAll(Subset subset, અચલ VErrorBitSet& bitset) {
            જો (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } નહીંતર {
                m_ctrlEn = bitset;
            }
        }
        શૂન્ય set(Subset subset, V3ErrorCode code, bool કિંમત) {
            જો (subset == Subset::CODE) {
                m_codeEn.set(code, કિંમત);
            } નહીંતર {
                m_ctrlEn.set(code, કિંમત);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) અચલ { પાછા m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<અચલ std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<સંદેશ_En_Bit_Set, સંદેશ_En_Set_Idx_t, સંદેશ_En_Bit_Set::Hash, સંદેશ_En_Bit_Set::Equal>
        m_interned_સંદેશ_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<સંદેશ_En_Bit_Set> m_interned_સંદેશ_Ens;

    // CONSTRUCTORS
    File_લીટી_Singleton() = મૂળભૂત;
    ~File_લીટી_Singleton() = મૂળભૂત;

    fileNameIdx_t name_To_સંખ્યા(અચલ string& filename);
    અચલ std::string& સંખ્યા_To_Name(fileNameIdx_t filenameno) અચલ VL_MT_SAFE {
        પાછા m_names[filenameno];
    }
    V3LangCode સંખ્યા_To_Lang(fileNameIdx_t filenameno) અચલ { પાછા m_languages[filenameno]; }
    શૂન્ય સંખ્યા_To_Lang(fileNameIdx_t filenameno, અચલ V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    શૂન્ય clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    શૂન્ય file_Name_અંક_Map_Dump_Json(std::ostream& os);
    સ્થિર string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    સંદેશ_En_Set_Idx_t add_સંદેશ_En_Bit_Set(અચલ સંદેશ_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    સંદેશ_En_Set_Idx_t default_સંદેશ_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    સંદેશ_En_Set_Idx_t સંદેશ_En_Set_Bit(સંદેશ_En_Set_Idx_t setIdx, સંદેશ_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool કિંમત);
    // Bulk-set all control codes to given bitset
    સંદેશ_En_Set_Idx_t સંદેશ_Set_Ctrl_Bit_Set(સંદેશ_En_Set_Idx_t setIdx, અચલ VErrorBitSet& bitset);
    // Return index to intersection set
    સંદેશ_En_Set_Idx_t સંદેશ_En_And(સંદેશ_En_Set_Idx_t lhsIdx, સંદેશ_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    અચલ સંદેશ_En_Bit_Set& સંદેશ_En(સંદેશ_En_Set_Idx_t idx) અચલ VL_MT_SAFE {
        પાછા m_interned_સંદેશ_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_લીટી;
    // MEMBERS
    પૂર્ણાંક m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_ગણતરી{0};
    std::deque<string> m_લીટીઓ;  // Source text lines
    VFileContent() {
        સ્થિર પૂર્ણાંક s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = મૂળભૂત;
    // METHODS
    શૂન્ય refInc() { ++m_ref_ગણતરી; }
    શૂન્ય refDec() {
        જો (!--m_ref_ગણતરી) delete this;
    }

જાહેર:
    શૂન્ય ધકેલો_Text(અચલ string& text);  // Add arbitrary text (need not be line-by-line)
    string get_લીટી(પૂર્ણાંક lineno) અચલ VL_MT_SAFE;
    string ascii() અચલ { પાછા "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_લીટી final {

    // CONSTANTS
    સ્થિર constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_લીટી_Singleton::fileNameIdx_t;
    using સંદેશ_En_Set_Idx_t = File_લીટી_Singleton::સંદેશ_En_Set_Idx_t;
    using સંદેશ_En_Bit_Set = File_લીટી_Singleton::સંદેશ_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    સંદેશ_En_Set_Idx_t m_સંદેશ_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_અંક = 0;  // Token number in processing order
    પૂર્ણાંક m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_લીટી* m_parent = nullptr;  // Parent line that included this line

સુરક્ષિત:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_લીટી_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

ખાનગી:
    // CONSTRUCTORS
    સ્થિર File_લીટી_Singleton& singleton() VL_MT_SAFE {
        સ્થિર File_લીટી_Singleton s;
        પાછા s;
    }
    સ્થિર File_લીટી& default_File_લીટી() VL_MT_SAFE {
        સ્થિર File_લીટી s;
        પાછા s;
    }

    File_લીટી()  // Only used for defaultFileLine above
        : m_સંદેશ_En_Idx{singleton().default_સંદેશ_En_Index()}
        , m_filenameno{singleton().name_To_સંખ્યા(File_લીટી::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

જાહેર:
    explicit File_લીટી(અચલ string& filename)
        : m_સંદેશ_En_Idx{default_File_લીટી().m_સંદેશ_En_Idx}
        , m_filenameno{singleton().name_To_સંખ્યા(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_લીટી(અચલ File_લીટી& from)
        : m_સંદેશ_En_Idx{from.m_સંદેશ_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_અંક{from.m_token_અંક}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        જો (m_contentp) m_contentp->refInc();
    }
    explicit File_લીટી(File_લીટી* fromp)
        : m_સંદેશ_En_Idx{fromp->m_સંદેશ_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_અંક{fromp->m_token_અંક}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        જો (m_contentp) m_contentp->refInc();
    }
    શૂન્ય applyIgnores();
    File_લીટી* copy_Or_Same_File_લીટી();
    File_લીટી* copy_Or_Same_File_લીટી_Applied();
    સ્થિર શૂન્ય deleteAllRemaining();
    સ્થિર શૂન્ય stats();
    ~File_લીટી();
#ifdef VL_LEAK_CHECKS
    સ્થિર શૂન્ય* operator new(size_t size);
    સ્થિર શૂન્ય operator delete(શૂન્ય* obj, size_t size);
#endif
    // METHODS
    શૂન્ય newContent();
    શૂન્ય contentLineno(પૂર્ણાંક અંક) {
        lineno(અંક);
        m_contentLineno = static_cast<unsigned>(અંક);
    }
    શૂન્ય contentLinenoFrom(અચલ File_લીટી* fromp) { m_contentLineno = fromp->m_contentLineno; }
    શૂન્ય lineno(પૂર્ણાંક અંક) {
        m_firstLineno = અંક;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    શૂન્ય column(પૂર્ણાંક firstColumn, પૂર્ણાંક lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    શૂન્ય language(V3LangCode lang) અચલ { singleton().સંખ્યા_To_Lang(filenameno(), lang); }
    શૂન્ય filename(અચલ string& name) { m_filenameno = singleton().name_To_સંખ્યા(name); }
    શૂન્ય parent(File_લીટી* fileline) { m_parent = fileline; }
    શૂન્ય લીટી_Directive(અચલ અક્ષર* textp, પૂર્ણાંક& enterExitRef);
    શૂન્ય લીટી_Directive_Parse(અચલ અક્ષર* textp, string& filenameRef, પૂર્ણાંક& linenoRef,
                            પૂર્ણાંક& enterExitRef);
    શૂન્ય linenoInc() {
        જો (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } નહીંતર {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    શૂન્ય startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    શૂન્ય forwardToken(અચલ અક્ષર* textp, size_t size, bool track_લીટીઓ = true);
    uint32_t token_અંક() અચલ VL_MT_SAFE { પાછા m_token_અંક; }
    પૂર્ણાંક firstLineno() અચલ VL_MT_SAFE { પાછા m_firstLineno; }
    પૂર્ણાંક firstColumn() અચલ VL_MT_SAFE { પાછા static_cast<પૂર્ણાંક>(m_firstColumn); }
    પૂર્ણાંક lastLineno() અચલ VL_MT_SAFE { પાછા m_firstLineno + m_lastLinenoAdder; }
    પૂર્ણાંક lastColumn() અચલ VL_MT_SAFE { પાછા static_cast<પૂર્ણાંક>(m_lastColumn); }
    VFileContent* contentp() અચલ { પાછા m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    પૂર્ણાંક lineno() અચલ VL_MT_SAFE { પાછા lastLineno(); }
    string source() અચલ VL_MT_SAFE;
    string sourcePrefix(પૂર્ણાંક toColumn) અચલ VL_MT_SAFE;
    string prettySource() અચલ VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_લીટી* parent() અચલ VL_MT_SAFE { પાછા m_parent; }
    V3LangCode language() અચલ { પાછા singleton().સંખ્યા_To_Lang(filenameno()); }
    string ascii() અચલ VL_MT_SAFE;
    string ascii_લીટી_Col() અચલ;
    પૂર્ણાંક filenameno() અચલ VL_MT_SAFE { પાછા m_filenameno; }
    અચલ std::string& filename() અચલ VL_MT_SAFE {
        પાછા singleton().સંખ્યા_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() અચલ VL_MT_SAFE { પાછા VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() અચલ VL_MT_SAFE {
        પાછા (filename() == command_લીટી_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() અચલ VL_MT_SAFE {
        પાછા File_લીટી_Singleton::filenameLetters(filenameno());
    }
    string filebasename() અચલ VL_MT_SAFE;
    string filebasenameNoExt() અચલ;
    string firstColumnLetters() અચલ VL_MT_SAFE;
    string profileFuncname() અચલ;
    string લીટી_Directive_Strg(પૂર્ણાંક enterExit) અચલ;

    // Turn on/off warning messages on this line.
ખાનગી:
    શૂન્ય warnSet(સંદેશ_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_સંદેશ_En_Idx = singleton().સંદેશ_En_Set_Bit(m_સંદેશ_En_Idx, subset, code, flag);
    }

જાહેર:
    શૂન્ય warnOn(V3ErrorCode code, bool flag) { warnSet(સંદેશ_En_Bit_Set::Subset::CODE, code, flag); }
    શૂન્ય warnSetCtrlBitSet(અચલ VErrorBitSet& bitset) {
        m_સંદેશ_En_Idx = singleton().સંદેશ_Set_Ctrl_Bit_Set(m_સંદેશ_En_Idx, bitset);
    }
    શૂન્ય warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(અચલ string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) અચલ VL_MT_SAFE;
    શૂન્ય warnLintOff(bool turnOff);
    શૂન્ય warnStyleOff(bool turnOff);
    શૂન્ય warnStateFrom(અચલ File_લીટી& from) { m_સંદેશ_En_Idx = from.m_સંદેશ_En_Idx; }
    શૂન્ય warnResetDefault() { warnStateFrom(default_File_લીટી()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() અચલ { પાછા સંદેશ_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    શૂન્ય celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() અચલ { પાછા સંદેશ_En().enabled(V3ErrorCode::I_COVERAGE); }
    શૂન્ય coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() અચલ { પાછા સંદેશ_En().enabled(V3ErrorCode::I_ERRORING); }
    શૂન્ય erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() અચલ { પાછા સંદેશ_En().enabled(V3ErrorCode::I_TRACING); }
    શૂન્ય tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() અચલ { પાછા સંદેશ_En().enabled(V3ErrorCode::I_TIMING); }
    શૂન્ય timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    સ્થિર string command_લીટી_Filename() VL_MT_SAFE { પાછા "<command-line>"; }
    સ્થિર string builtInFilename() VL_MT_SAFE { પાછા "<built-in>"; }
    સ્થિર શૂન્ય globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_લીટી().warnOff(code, turnOff);
    }
    સ્થિર string globalWarnOffParse(અચલ string& msgs, bool turnOff) {
        પાછા default_File_લીટી().warnOffParse(msgs, turnOff);
    }
    સ્થિર શૂન્ય file_Name_અંક_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_અંક_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    શૂન્ય modifyStateInherit(અચલ File_લીટી* fromp) {
        m_સંદેશ_En_Idx = singleton().સંદેશ_En_And(m_સંદેશ_En_Idx, fromp->m_સંદેશ_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    શૂન્ય modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    શૂન્ય v3errorEnd(std::ostringstream& str, અચલ string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    શૂન્ય v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        પ્રતિજ્ઞા(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() અચલ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() અચલ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() અચલ VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        પાછા warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() અચલ { પાછા warnContext(); }
    bool operator==(અચલ File_લીટી& rhs) અચલ {
        પાછા (m_token_અંક == rhs.m_token_અંક && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_સંદેશ_En_Idx == rhs.m_સંદેશ_En_Idx);
    }
    bool equal_First_લીટી_Col(અચલ File_લીટી& rhs) અચલ {
        પાછા (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    પૂર્ણાંક operatorCompare(અચલ File_લીટી& rhs) અચલ {
        જો (m_filenameno != rhs.m_filenameno) પાછા (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        જો (m_firstLineno != rhs.m_firstLineno)
            પાછા (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        જો (m_firstColumn != rhs.m_firstColumn)
            પાછા (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        જો (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            પાછા (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        જો (m_lastColumn != rhs.m_lastColumn) પાછા (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        અચલ સંદેશ_En_Bit_Set& lhs_સંદેશ_En = સંદેશ_En();
        અચલ સંદેશ_En_Bit_Set& rhs_સંદેશ_En = rhs.સંદેશ_En();
        માટે (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            અચલ V3ErrorCode code = static_cast<V3ErrorCode>(i);
            જો (lhs_સંદેશ_En.enabled(code) != rhs_સંદેશ_En.enabled(code))
                પાછા rhs_સંદેશ_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        જો (m_token_અંક != rhs.m_token_અંક) પાછા (m_token_અંક < rhs.m_token_અંક) ? -1 : 1;
        પાછા 0;  // (*this) and rhs are equivalent
    }

ખાનગી:
    string warnContext() અચલ;
    string warnContextParent() અચલ VL_REQUIRES(V3Error::s().m_mutex);
    અચલ સંદેશ_En_Bit_Set& સંદેશ_En() અચલ VL_MT_SAFE { પાછા singleton().સંદેશ_En(m_સંદેશ_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_લીટી* fileline);

#endif  // Guard
