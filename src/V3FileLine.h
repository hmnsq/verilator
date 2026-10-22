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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Hash.h"
#அந்தர்பாவ "V3LangCode.h"
#அந்தர்பாவ "V3Mutex.h"

#அந்தர்பாவ <atomic>
#அந்தர்பாவ <deque>
#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <set>
#அந்தர்பாவ <sstream>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <vector>

// ######################################################################

class File_வரி;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_வரி_Singleton final {
    friend class File_வரி;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using செய்தி_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class செய்தி_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    பொது:
        எண்ணிக்கை class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        செய்தி_En_Bit_Set() = இயல்பு;
        // Create intersection set
        செய்தி_En_Bit_Set(மாறிலி செய்தி_En_Bit_Set& a, மாறிலி செய்தி_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        கட்டமைப்பு Hash final {
            size_t operator()(மாறிலி செய்தி_En_Bit_Set& item) மாறிலி {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                திரும்பு hash.மதிப்பு();
            }
        };

        கட்டமைப்பு Equal final {
            bool operator()(மாறிலி செய்தி_En_Bit_Set& a, மாறிலி செய்தி_En_Bit_Set& b) மாறிலி { திரும்பு a == b; }
        };

        bool operator==(மாறிலி செய்தி_En_Bit_Set& other) மாறிலி {
            திரும்பு m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        மாறிலி VErrorBitSet& getAll(Subset subset) மாறிலி {
            திரும்பு subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) மாறிலி {
            திரும்பு subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        வெறுமை setAll(Subset subset, மாறிலி VErrorBitSet& bitset) {
            எனில் (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } இல்லையெனில் {
                m_ctrlEn = bitset;
            }
        }
        வெறுமை set(Subset subset, V3ErrorCode code, bool மதிப்பு) {
            எனில் (subset == Subset::CODE) {
                m_codeEn.set(code, மதிப்பு);
            } இல்லையெனில் {
                m_ctrlEn.set(code, மதிப்பு);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) மாறிலி { திரும்பு m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<மாறிலி std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<செய்தி_En_Bit_Set, செய்தி_En_Set_Idx_t, செய்தி_En_Bit_Set::Hash, செய்தி_En_Bit_Set::Equal>
        m_interned_செய்தி_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<செய்தி_En_Bit_Set> m_interned_செய்தி_Ens;

    // CONSTRUCTORS
    File_வரி_Singleton() = இயல்பு;
    ~File_வரி_Singleton() = இயல்பு;

    fileNameIdx_t name_To_எண்(மாறிலி string& filename);
    மாறிலி std::string& எண்_To_Name(fileNameIdx_t filenameno) மாறிலி VL_MT_SAFE {
        திரும்பு m_names[filenameno];
    }
    V3LangCode எண்_To_Lang(fileNameIdx_t filenameno) மாறிலி { திரும்பு m_languages[filenameno]; }
    வெறுமை எண்_To_Lang(fileNameIdx_t filenameno, மாறிலி V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    வெறுமை clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    வெறுமை file_Name_இலக்கம்_Map_Dump_Json(std::ostream& os);
    நிலையான string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    செய்தி_En_Set_Idx_t add_செய்தி_En_Bit_Set(மாறிலி செய்தி_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    செய்தி_En_Set_Idx_t default_செய்தி_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    செய்தி_En_Set_Idx_t செய்தி_En_Set_Bit(செய்தி_En_Set_Idx_t setIdx, செய்தி_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool மதிப்பு);
    // Bulk-set all control codes to given bitset
    செய்தி_En_Set_Idx_t செய்தி_Set_Ctrl_Bit_Set(செய்தி_En_Set_Idx_t setIdx, மாறிலி VErrorBitSet& bitset);
    // Return index to intersection set
    செய்தி_En_Set_Idx_t செய்தி_En_And(செய்தி_En_Set_Idx_t lhsIdx, செய்தி_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    மாறிலி செய்தி_En_Bit_Set& செய்தி_En(செய்தி_En_Set_Idx_t idx) மாறிலி VL_MT_SAFE {
        திரும்பு m_interned_செய்தி_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_வரி;
    // MEMBERS
    முழு m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_மொத்தம்{0};
    std::deque<string> m_வரிகள்;  // Source text lines
    VFileContent() {
        நிலையான முழு s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = இயல்பு;
    // METHODS
    வெறுமை refInc() { ++m_ref_மொத்தம்; }
    வெறுமை refDec() {
        எனில் (!--m_ref_மொத்தம்) delete this;
    }

பொது:
    வெறுமை தள்ளு_Text(மாறிலி string& text);  // Add arbitrary text (need not be line-by-line)
    string get_வரி(முழு lineno) மாறிலி VL_MT_SAFE;
    string ascii() மாறிலி { திரும்பு "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_வரி final {

    // CONSTANTS
    நிலையான constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_வரி_Singleton::fileNameIdx_t;
    using செய்தி_En_Set_Idx_t = File_வரி_Singleton::செய்தி_En_Set_Idx_t;
    using செய்தி_En_Bit_Set = File_வரி_Singleton::செய்தி_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    செய்தி_En_Set_Idx_t m_செய்தி_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_இலக்கம் = 0;  // Token number in processing order
    முழு m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_வரி* m_parent = nullptr;  // Parent line that included this line

பாதுகாக்கப்பட்ட:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_வரி_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

தனிப்பட்ட:
    // CONSTRUCTORS
    நிலையான File_வரி_Singleton& singleton() VL_MT_SAFE {
        நிலையான File_வரி_Singleton s;
        திரும்பு s;
    }
    நிலையான File_வரி& default_File_வரி() VL_MT_SAFE {
        நிலையான File_வரி s;
        திரும்பு s;
    }

    File_வரி()  // Only used for defaultFileLine above
        : m_செய்தி_En_Idx{singleton().default_செய்தி_En_Index()}
        , m_filenameno{singleton().name_To_எண்(File_வரி::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

பொது:
    explicit File_வரி(மாறிலி string& filename)
        : m_செய்தி_En_Idx{default_File_வரி().m_செய்தி_En_Idx}
        , m_filenameno{singleton().name_To_எண்(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_வரி(மாறிலி File_வரி& from)
        : m_செய்தி_En_Idx{from.m_செய்தி_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_இலக்கம்{from.m_token_இலக்கம்}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        எனில் (m_contentp) m_contentp->refInc();
    }
    explicit File_வரி(File_வரி* fromp)
        : m_செய்தி_En_Idx{fromp->m_செய்தி_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_இலக்கம்{fromp->m_token_இலக்கம்}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        எனில் (m_contentp) m_contentp->refInc();
    }
    வெறுமை applyIgnores();
    File_வரி* copy_Or_Same_File_வரி();
    File_வரி* copy_Or_Same_File_வரி_Applied();
    நிலையான வெறுமை deleteAllRemaining();
    நிலையான வெறுமை stats();
    ~File_வரி();
#ifdef VL_LEAK_CHECKS
    நிலையான வெறுமை* operator new(size_t size);
    நிலையான வெறுமை operator delete(வெறுமை* obj, size_t size);
#endif
    // METHODS
    வெறுமை newContent();
    வெறுமை contentLineno(முழு இலக்கம்) {
        lineno(இலக்கம்);
        m_contentLineno = static_cast<unsigned>(இலக்கம்);
    }
    வெறுமை contentLinenoFrom(மாறிலி File_வரி* fromp) { m_contentLineno = fromp->m_contentLineno; }
    வெறுமை lineno(முழு இலக்கம்) {
        m_firstLineno = இலக்கம்;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    வெறுமை column(முழு firstColumn, முழு lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    வெறுமை language(V3LangCode lang) மாறிலி { singleton().எண்_To_Lang(filenameno(), lang); }
    வெறுமை filename(மாறிலி string& name) { m_filenameno = singleton().name_To_எண்(name); }
    வெறுமை parent(File_வரி* fileline) { m_parent = fileline; }
    வெறுமை வரி_Directive(மாறிலி எழுத்து* textp, முழு& enterExitRef);
    வெறுமை வரி_Directive_Parse(மாறிலி எழுத்து* textp, string& filenameRef, முழு& linenoRef,
                            முழு& enterExitRef);
    வெறுமை linenoInc() {
        எனில் (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } இல்லையெனில் {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    வெறுமை startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    வெறுமை forwardToken(மாறிலி எழுத்து* textp, size_t size, bool track_வரிகள் = true);
    uint32_t token_இலக்கம்() மாறிலி VL_MT_SAFE { திரும்பு m_token_இலக்கம்; }
    முழு firstLineno() மாறிலி VL_MT_SAFE { திரும்பு m_firstLineno; }
    முழு firstColumn() மாறிலி VL_MT_SAFE { திரும்பு static_cast<முழு>(m_firstColumn); }
    முழு lastLineno() மாறிலி VL_MT_SAFE { திரும்பு m_firstLineno + m_lastLinenoAdder; }
    முழு lastColumn() மாறிலி VL_MT_SAFE { திரும்பு static_cast<முழு>(m_lastColumn); }
    VFileContent* contentp() மாறிலி { திரும்பு m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    முழு lineno() மாறிலி VL_MT_SAFE { திரும்பு lastLineno(); }
    string source() மாறிலி VL_MT_SAFE;
    string sourcePrefix(முழு toColumn) மாறிலி VL_MT_SAFE;
    string prettySource() மாறிலி VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_வரி* parent() மாறிலி VL_MT_SAFE { திரும்பு m_parent; }
    V3LangCode language() மாறிலி { திரும்பு singleton().எண்_To_Lang(filenameno()); }
    string ascii() மாறிலி VL_MT_SAFE;
    string ascii_வரி_Col() மாறிலி;
    முழு filenameno() மாறிலி VL_MT_SAFE { திரும்பு m_filenameno; }
    மாறிலி std::string& filename() மாறிலி VL_MT_SAFE {
        திரும்பு singleton().எண்_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() மாறிலி VL_MT_SAFE { திரும்பு VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() மாறிலி VL_MT_SAFE {
        திரும்பு (filename() == command_வரி_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() மாறிலி VL_MT_SAFE {
        திரும்பு File_வரி_Singleton::filenameLetters(filenameno());
    }
    string filebasename() மாறிலி VL_MT_SAFE;
    string filebasenameNoExt() மாறிலி;
    string firstColumnLetters() மாறிலி VL_MT_SAFE;
    string profileFuncname() மாறிலி;
    string வரி_Directive_Strg(முழு enterExit) மாறிலி;

    // Turn on/off warning messages on this line.
தனிப்பட்ட:
    வெறுமை warnSet(செய்தி_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_செய்தி_En_Idx = singleton().செய்தி_En_Set_Bit(m_செய்தி_En_Idx, subset, code, flag);
    }

பொது:
    வெறுமை warnOn(V3ErrorCode code, bool flag) { warnSet(செய்தி_En_Bit_Set::Subset::CODE, code, flag); }
    வெறுமை warnSetCtrlBitSet(மாறிலி VErrorBitSet& bitset) {
        m_செய்தி_En_Idx = singleton().செய்தி_Set_Ctrl_Bit_Set(m_செய்தி_En_Idx, bitset);
    }
    வெறுமை warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(மாறிலி string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) மாறிலி VL_MT_SAFE;
    வெறுமை warnLintOff(bool turnOff);
    வெறுமை warnStyleOff(bool turnOff);
    வெறுமை warnStateFrom(மாறிலி File_வரி& from) { m_செய்தி_En_Idx = from.m_செய்தி_En_Idx; }
    வெறுமை warnResetDefault() { warnStateFrom(default_File_வரி()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() மாறிலி { திரும்பு செய்தி_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    வெறுமை celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() மாறிலி { திரும்பு செய்தி_En().enabled(V3ErrorCode::I_COVERAGE); }
    வெறுமை coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() மாறிலி { திரும்பு செய்தி_En().enabled(V3ErrorCode::I_ERRORING); }
    வெறுமை erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() மாறிலி { திரும்பு செய்தி_En().enabled(V3ErrorCode::I_TRACING); }
    வெறுமை tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() மாறிலி { திரும்பு செய்தி_En().enabled(V3ErrorCode::I_TIMING); }
    வெறுமை timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    நிலையான string command_வரி_Filename() VL_MT_SAFE { திரும்பு "<command-line>"; }
    நிலையான string builtInFilename() VL_MT_SAFE { திரும்பு "<built-in>"; }
    நிலையான வெறுமை globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_வரி().warnOff(code, turnOff);
    }
    நிலையான string globalWarnOffParse(மாறிலி string& msgs, bool turnOff) {
        திரும்பு default_File_வரி().warnOffParse(msgs, turnOff);
    }
    நிலையான வெறுமை file_Name_இலக்கம்_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_இலக்கம்_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    வெறுமை modifyStateInherit(மாறிலி File_வரி* fromp) {
        m_செய்தி_En_Idx = singleton().செய்தி_En_And(m_செய்தி_En_Idx, fromp->m_செய்தி_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    வெறுமை modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    வெறுமை v3errorEnd(std::ostringstream& str, மாறிலி string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    வெறுமை v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        ப்ரதிஜ்ஞா(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() மாறிலி VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() மாறிலி VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() மாறிலி VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        திரும்பு warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() மாறிலி { திரும்பு warnContext(); }
    bool operator==(மாறிலி File_வரி& rhs) மாறிலி {
        திரும்பு (m_token_இலக்கம் == rhs.m_token_இலக்கம் && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_செய்தி_En_Idx == rhs.m_செய்தி_En_Idx);
    }
    bool equal_First_வரி_Col(மாறிலி File_வரி& rhs) மாறிலி {
        திரும்பு (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    முழு operatorCompare(மாறிலி File_வரி& rhs) மாறிலி {
        எனில் (m_filenameno != rhs.m_filenameno) திரும்பு (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        எனில் (m_firstLineno != rhs.m_firstLineno)
            திரும்பு (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        எனில் (m_firstColumn != rhs.m_firstColumn)
            திரும்பு (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        எனில் (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            திரும்பு (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        எனில் (m_lastColumn != rhs.m_lastColumn) திரும்பு (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        மாறிலி செய்தி_En_Bit_Set& lhs_செய்தி_En = செய்தி_En();
        மாறிலி செய்தி_En_Bit_Set& rhs_செய்தி_En = rhs.செய்தி_En();
        க்கு (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            மாறிலி V3ErrorCode code = static_cast<V3ErrorCode>(i);
            எனில் (lhs_செய்தி_En.enabled(code) != rhs_செய்தி_En.enabled(code))
                திரும்பு rhs_செய்தி_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        எனில் (m_token_இலக்கம் != rhs.m_token_இலக்கம்) திரும்பு (m_token_இலக்கம் < rhs.m_token_இலக்கம்) ? -1 : 1;
        திரும்பு 0;  // (*this) and rhs are equivalent
    }

தனிப்பட்ட:
    string warnContext() மாறிலி;
    string warnContextParent() மாறிலி VL_REQUIRES(V3Error::s().m_mutex);
    மாறிலி செய்தி_En_Bit_Set& செய்தி_En() மாறிலி VL_MT_SAFE { திரும்பு singleton().செய்தி_En(m_செய்தி_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_வரி* fileline);

#endif  // Guard
