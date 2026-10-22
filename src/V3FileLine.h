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

#include "config_build.h"
#include "verilatedos.h"

#include "V3Error.h"
#include "V3Hash.h"
#include "V3LangCode.h"
#include "V3Mutex.h"

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

// ######################################################################

class File_लिन्या;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_लिन्या_Singleton final {
    friend class File_लिन्या;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using मेनसाहे_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class मेनसाहे_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    public:
        enum class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        मेनसाहे_En_Bit_Set() = default;
        // Create intersection set
        मेनसाहे_En_Bit_Set(const मेनसाहे_En_Bit_Set& a, const मेनसाहे_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        struct Hash final {
            size_t operator()(const मेनसाहे_En_Bit_Set& item) const {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                return hash.हालागा();
            }
        };

        struct Equal final {
            bool operator()(const मेनसाहे_En_Bit_Set& a, const मेनसाहे_En_Bit_Set& b) const { return a == b; }
        };

        bool operator==(const मेनसाहे_En_Bit_Set& other) const {
            return m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        const VErrorBitSet& getAll(Subset subset) const {
            return subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) const {
            return subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        void setAll(Subset subset, const VErrorBitSet& bitset) {
            if (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } else {
                m_ctrlEn = bitset;
            }
        }
        void set(Subset subset, V3ErrorCode code, bool हालागा) {
            if (subset == Subset::CODE) {
                m_codeEn.set(code, हालागा);
            } else {
                m_ctrlEn.set(code, हालागा);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) const { return m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<const std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<मेनसाहे_En_Bit_Set, मेनसाहे_En_Set_Idx_t, मेनसाहे_En_Bit_Set::Hash, मेनसाहे_En_Bit_Set::Equal>
        m_interned_मेनसाहे_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<मेनसाहे_En_Bit_Set> m_interned_मेनसाहे_Ens;

    // CONSTRUCTORS
    File_लिन्या_Singleton() = default;
    ~File_लिन्या_Singleton() = default;

    fileNameIdx_t name_To_नुमेरो(const string& filename);
    const std::string& नुमेरो_To_Name(fileNameIdx_t filenameno) const VL_MT_SAFE {
        return m_names[filenameno];
    }
    V3LangCode नुमेरो_To_Lang(fileNameIdx_t filenameno) const { return m_languages[filenameno]; }
    void नुमेरो_To_Lang(fileNameIdx_t filenameno, const V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    void clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    void file_Name_नुमेरो_Map_Dump_Json(std::ostream& os);
    static string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    मेनसाहे_En_Set_Idx_t add_मेनसाहे_En_Bit_Set(const मेनसाहे_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    मेनसाहे_En_Set_Idx_t default_मेनसाहे_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    मेनसाहे_En_Set_Idx_t मेनसाहे_En_Set_Bit(मेनसाहे_En_Set_Idx_t setIdx, मेनसाहे_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool हालागा);
    // Bulk-set all control codes to given bitset
    मेनसाहे_En_Set_Idx_t मेनसाहे_Set_Ctrl_Bit_Set(मेनसाहे_En_Set_Idx_t setIdx, const VErrorBitSet& bitset);
    // Return index to intersection set
    मेनसाहे_En_Set_Idx_t मेनसाहे_En_And(मेनसाहे_En_Set_Idx_t lhsIdx, मेनसाहे_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    const मेनसाहे_En_Bit_Set& मेनसाहे_En(मेनसाहे_En_Set_Idx_t idx) const VL_MT_SAFE {
        return m_interned_मेनसाहे_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_लिन्या;
    // MEMBERS
    int m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_बिलांग{0};
    std::deque<string> m_मगा_लिन्या;  // Source text lines
    VFileContent() {
        static int s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = default;
    // METHODS
    void refInc() { ++m_ref_बिलांग; }
    void refDec() {
        if (!--m_ref_बिलांग) delete this;
    }

public:
    void इतुलाक_Text(const string& text);  // Add arbitrary text (need not be line-by-line)
    string get_लिन्या(int lineno) const VL_MT_SAFE;
    string ascii() const { return "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_लिन्या final {

    // CONSTANTS
    static constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_लिन्या_Singleton::fileNameIdx_t;
    using मेनसाहे_En_Set_Idx_t = File_लिन्या_Singleton::मेनसाहे_En_Set_Idx_t;
    using मेनसाहे_En_Bit_Set = File_लिन्या_Singleton::मेनसाहे_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    मेनसाहे_En_Set_Idx_t m_मेनसाहे_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_नुमेरो = 0;  // Token number in processing order
    int m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_लिन्या* m_parent = nullptr;  // Parent line that included this line

protected:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_लिन्या_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

private:
    // CONSTRUCTORS
    static File_लिन्या_Singleton& singleton() VL_MT_SAFE {
        static File_लिन्या_Singleton s;
        return s;
    }
    static File_लिन्या& default_File_लिन्या() VL_MT_SAFE {
        static File_लिन्या s;
        return s;
    }

    File_लिन्या()  // Only used for defaultFileLine above
        : m_मेनसाहे_En_Idx{singleton().default_मेनसाहे_En_Index()}
        , m_filenameno{singleton().name_To_नुमेरो(File_लिन्या::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

public:
    explicit File_लिन्या(const string& filename)
        : m_मेनसाहे_En_Idx{default_File_लिन्या().m_मेनसाहे_En_Idx}
        , m_filenameno{singleton().name_To_नुमेरो(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_लिन्या(const File_लिन्या& from)
        : m_मेनसाहे_En_Idx{from.m_मेनसाहे_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_नुमेरो{from.m_token_नुमेरो}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        if (m_contentp) m_contentp->refInc();
    }
    explicit File_लिन्या(File_लिन्या* fromp)
        : m_मेनसाहे_En_Idx{fromp->m_मेनसाहे_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_नुमेरो{fromp->m_token_नुमेरो}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        if (m_contentp) m_contentp->refInc();
    }
    void applyIgnores();
    File_लिन्या* copy_Or_Same_File_लिन्या();
    File_लिन्या* copy_Or_Same_File_लिन्या_Applied();
    static void deleteAllRemaining();
    static void stats();
    ~File_लिन्या();
#ifdef VL_LEAK_CHECKS
    static void* operator new(size_t size);
    static void operator delete(void* obj, size_t size);
#endif
    // METHODS
    void newContent();
    void contentLineno(int नुमेरो) {
        lineno(नुमेरो);
        m_contentLineno = static_cast<unsigned>(नुमेरो);
    }
    void contentLinenoFrom(const File_लिन्या* fromp) { m_contentLineno = fromp->m_contentLineno; }
    void lineno(int नुमेरो) {
        m_firstLineno = नुमेरो;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    void column(int firstColumn, int lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    void language(V3LangCode lang) const { singleton().नुमेरो_To_Lang(filenameno(), lang); }
    void filename(const string& name) { m_filenameno = singleton().name_To_नुमेरो(name); }
    void parent(File_लिन्या* fileline) { m_parent = fileline; }
    void लिन्या_Directive(const char* textp, int& enterExitRef);
    void लिन्या_Directive_Parse(const char* textp, string& filenameRef, int& linenoRef,
                            int& enterExitRef);
    void linenoInc() {
        if (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } else {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    void startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    void forwardToken(const char* textp, size_t size, bool track_मगा_लिन्या = true);
    uint32_t token_नुमेरो() const VL_MT_SAFE { return m_token_नुमेरो; }
    int firstLineno() const VL_MT_SAFE { return m_firstLineno; }
    int firstColumn() const VL_MT_SAFE { return static_cast<int>(m_firstColumn); }
    int lastLineno() const VL_MT_SAFE { return m_firstLineno + m_lastLinenoAdder; }
    int lastColumn() const VL_MT_SAFE { return static_cast<int>(m_lastColumn); }
    VFileContent* contentp() const { return m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    int lineno() const VL_MT_SAFE { return lastLineno(); }
    string source() const VL_MT_SAFE;
    string sourcePrefix(int toColumn) const VL_MT_SAFE;
    string prettySource() const VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_लिन्या* parent() const VL_MT_SAFE { return m_parent; }
    V3LangCode language() const { return singleton().नुमेरो_To_Lang(filenameno()); }
    string ascii() const VL_MT_SAFE;
    string ascii_लिन्या_Col() const;
    int filenameno() const VL_MT_SAFE { return m_filenameno; }
    const std::string& filename() const VL_MT_SAFE {
        return singleton().नुमेरो_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() const VL_MT_SAFE { return VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() const VL_MT_SAFE {
        return (filename() == command_लिन्या_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() const VL_MT_SAFE {
        return File_लिन्या_Singleton::filenameLetters(filenameno());
    }
    string filebasename() const VL_MT_SAFE;
    string filebasenameNoExt() const;
    string firstColumnLetters() const VL_MT_SAFE;
    string profileFuncname() const;
    string लिन्या_Directive_Strg(int enterExit) const;

    // Turn on/off warning messages on this line.
private:
    void warnSet(मेनसाहे_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_मेनसाहे_En_Idx = singleton().मेनसाहे_En_Set_Bit(m_मेनसाहे_En_Idx, subset, code, flag);
    }

public:
    void warnOn(V3ErrorCode code, bool flag) { warnSet(मेनसाहे_En_Bit_Set::Subset::CODE, code, flag); }
    void warnSetCtrlBitSet(const VErrorBitSet& bitset) {
        m_मेनसाहे_En_Idx = singleton().मेनसाहे_Set_Ctrl_Bit_Set(m_मेनसाहे_En_Idx, bitset);
    }
    void warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(const string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) const VL_MT_SAFE;
    void warnLintOff(bool turnOff);
    void warnStyleOff(bool turnOff);
    void warnStateFrom(const File_लिन्या& from) { m_मेनसाहे_En_Idx = from.m_मेनसाहे_En_Idx; }
    void warnResetDefault() { warnStateFrom(default_File_लिन्या()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() const { return मेनसाहे_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    void celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() const { return मेनसाहे_En().enabled(V3ErrorCode::I_COVERAGE); }
    void coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() const { return मेनसाहे_En().enabled(V3ErrorCode::I_ERRORING); }
    void erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() const { return मेनसाहे_En().enabled(V3ErrorCode::I_TRACING); }
    void tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() const { return मेनसाहे_En().enabled(V3ErrorCode::I_TIMING); }
    void timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    static string command_लिन्या_Filename() VL_MT_SAFE { return "<command-line>"; }
    static string builtInFilename() VL_MT_SAFE { return "<built-in>"; }
    static void globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_लिन्या().warnOff(code, turnOff);
    }
    static string globalWarnOffParse(const string& msgs, bool turnOff) {
        return default_File_लिन्या().warnOffParse(msgs, turnOff);
    }
    static void file_Name_नुमेरो_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_नुमेरो_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    void modifyStateInherit(const File_लिन्या* fromp) {
        m_मेनसाहे_En_Idx = singleton().मेनसाहे_En_And(m_मेनसाहे_En_Idx, fromp->m_मेनसाहे_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    void modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    void v3errorEnd(std::ostringstream& str, const string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    void v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        assert(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() const VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() const VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() const VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        return warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() const { return warnContext(); }
    bool operator==(const File_लिन्या& rhs) const {
        return (m_token_नुमेरो == rhs.m_token_नुमेरो && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_मेनसाहे_En_Idx == rhs.m_मेनसाहे_En_Idx);
    }
    bool equal_First_लिन्या_Col(const File_लिन्या& rhs) const {
        return (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    int operatorCompare(const File_लिन्या& rhs) const {
        if (m_filenameno != rhs.m_filenameno) return (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        if (m_firstLineno != rhs.m_firstLineno)
            return (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        if (m_firstColumn != rhs.m_firstColumn)
            return (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        if (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            return (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        if (m_lastColumn != rhs.m_lastColumn) return (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        const मेनसाहे_En_Bit_Set& lhs_मेनसाहे_En = मेनसाहे_En();
        const मेनसाहे_En_Bit_Set& rhs_मेनसाहे_En = rhs.मेनसाहे_En();
        for (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            const V3ErrorCode code = static_cast<V3ErrorCode>(i);
            if (lhs_मेनसाहे_En.enabled(code) != rhs_मेनसाहे_En.enabled(code))
                return rhs_मेनसाहे_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        if (m_token_नुमेरो != rhs.m_token_नुमेरो) return (m_token_नुमेरो < rhs.m_token_नुमेरो) ? -1 : 1;
        return 0;  // (*this) and rhs are equivalent
    }

private:
    string warnContext() const;
    string warnContextParent() const VL_REQUIRES(V3Error::s().m_mutex);
    const मेनसाहे_En_Bit_Set& मेनसाहे_En() const VL_MT_SAFE { return singleton().मेनसाहे_En(m_मेनसाहे_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_लिन्या* fileline);

#endif  // Guard
