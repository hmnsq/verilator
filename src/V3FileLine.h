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

#अगर_अघोषित VERILATOR_V3FILELINE_H_
#घोषणा VERILATOR_V3FILELINE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3Hash.h"
#समावेश "V3LangCode.h"
#समावेश "V3Mutex.h"

#समावेश <atomic>
#समावेश <deque>
#समावेश <map>
#समावेश <memory>
#समावेश <set>
#समावेश <sstream>
#समावेश <unordered_map>
#समावेश <vector>

// ######################################################################

class File_पंक्ति;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_पंक्ति_Singleton final {
    मित्र class File_पंक्ति;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using संदेश_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class संदेश_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    खुला:
        क्रमागत class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        संदेश_En_Bit_Set() = शेष;
        // Create intersection set
        संदेश_En_Bit_Set(स्थिर संदेश_En_Bit_Set& a, स्थिर संदेश_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        काष्ठा Hash final {
            माप_प्रकार चालक()(स्थिर संदेश_En_Bit_Set& item) स्थिर {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                वापस hash.मूल्य();
            }
        };

        काष्ठा Equal final {
            bool चालक()(स्थिर संदेश_En_Bit_Set& a, स्थिर संदेश_En_Bit_Set& b) स्थिर { वापस a == b; }
        };

        bool चालक==(स्थिर संदेश_En_Bit_Set& other) स्थिर {
            वापस m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        स्थिर VErrorBitSet& getAll(Subset subset) स्थिर {
            वापस subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) स्थिर {
            वापस subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        व्योम setAll(Subset subset, स्थिर VErrorBitSet& bitset) {
            अगर (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } अन्यथा {
                m_ctrlEn = bitset;
            }
        }
        व्योम set(Subset subset, V3ErrorCode code, bool मूल्य) {
            अगर (subset == Subset::CODE) {
                m_codeEn.set(code, मूल्य);
            } अन्यथा {
                m_ctrlEn.set(code, मूल्य);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) स्थिर { वापस m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<स्थिर std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<संदेश_En_Bit_Set, संदेश_En_Set_Idx_t, संदेश_En_Bit_Set::Hash, संदेश_En_Bit_Set::Equal>
        m_interned_संदेश_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<संदेश_En_Bit_Set> m_interned_संदेश_Ens;

    // CONSTRUCTORS
    File_पंक्ति_Singleton() = शेष;
    ~File_पंक्ति_Singleton() = शेष;

    fileNameIdx_t name_To_संख्या(स्थिर string& filename);
    स्थिर std::string& संख्या_To_Name(fileNameIdx_t filenameno) स्थिर VL_MT_SAFE {
        वापस m_names[filenameno];
    }
    V3LangCode संख्या_To_Lang(fileNameIdx_t filenameno) स्थिर { वापस m_languages[filenameno]; }
    व्योम संख्या_To_Lang(fileNameIdx_t filenameno, स्थिर V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    व्योम clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    व्योम file_Name_अंक_Map_Dump_Json(std::ostream& os);
    अटल string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    संदेश_En_Set_Idx_t add_संदेश_En_Bit_Set(स्थिर संदेश_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    संदेश_En_Set_Idx_t default_संदेश_En_सूचकांक() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    संदेश_En_Set_Idx_t संदेश_En_Set_Bit(संदेश_En_Set_Idx_t setIdx, संदेश_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool मूल्य);
    // Bulk-set all control codes to given bitset
    संदेश_En_Set_Idx_t संदेश_Set_Ctrl_Bit_Set(संदेश_En_Set_Idx_t setIdx, स्थिर VErrorBitSet& bitset);
    // Return index to intersection set
    संदेश_En_Set_Idx_t संदेश_En_And(संदेश_En_Set_Idx_t lhsIdx, संदेश_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    स्थिर संदेश_En_Bit_Set& संदेश_En(संदेश_En_Set_Idx_t idx) स्थिर VL_MT_SAFE {
        वापस m_interned_संदेश_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    मित्र class File_पंक्ति;
    // MEMBERS
    पूर्णांक m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<माप_प्रकार> m_ref_गिनती{0};
    std::deque<string> m_पंक्तियाँ;  // Source text lines
    VFileContent() {
        अटल पूर्णांक s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = शेष;
    // METHODS
    व्योम refInc() { ++m_ref_गिनती; }
    व्योम refDec() {
        अगर (!--m_ref_गिनती) delete this;
    }

खुला:
    व्योम धकेलो_Text(स्थिर string& text);  // Add arbitrary text (need not be line-by-line)
    string get_पंक्ति(पूर्णांक lineno) स्थिर VL_MT_SAFE;
    string ascii() स्थिर { वापस "ct" + cvtToStr(m_id); }
};
std::ostream& चालक<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_पंक्ति final {

    // CONSTANTS
    अटल constexpr अचिन्हित SHOW_SOURCE_MAX_लंबाई = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_पंक्ति_Singleton::fileNameIdx_t;
    using संदेश_En_Set_Idx_t = File_पंक्ति_Singleton::संदेश_En_Set_Idx_t;
    using संदेश_En_Bit_Set = File_पंक्ति_Singleton::संदेश_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    संदेश_En_Set_Idx_t m_संदेश_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    अचिन्हित m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_अंक = 0;  // Token number in processing order
    पूर्णांक m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_पंक्ति* m_parent = nullptr;  // Parent line that included this line

रक्षित:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    मित्र class File_पंक्ति_Singleton;
    मित्र class V3ParseImp;
    मित्र class V3PreLex;
    मित्र class V3PreProcImp;
    मित्र class V3PreShellImp;

निजी:
    // CONSTRUCTORS
    अटल File_पंक्ति_Singleton& singleton() VL_MT_SAFE {
        अटल File_पंक्ति_Singleton s;
        वापस s;
    }
    अटल File_पंक्ति& default_File_पंक्ति() VL_MT_SAFE {
        अटल File_पंक्ति s;
        वापस s;
    }

    File_पंक्ति()  // Only used for defaultFileLine above
        : m_संदेश_En_Idx{singleton().default_संदेश_En_सूचकांक()}
        , m_filenameno{singleton().name_To_संख्या(File_पंक्ति::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

खुला:
    explicit File_पंक्ति(स्थिर string& filename)
        : m_संदेश_En_Idx{default_File_पंक्ति().m_संदेश_En_Idx}
        , m_filenameno{singleton().name_To_संख्या(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_पंक्ति(स्थिर File_पंक्ति& from)
        : m_संदेश_En_Idx{from.m_संदेश_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_अंक{from.m_token_अंक}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        अगर (m_contentp) m_contentp->refInc();
    }
    explicit File_पंक्ति(File_पंक्ति* fromp)
        : m_संदेश_En_Idx{fromp->m_संदेश_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_अंक{fromp->m_token_अंक}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        अगर (m_contentp) m_contentp->refInc();
    }
    व्योम applyIgnores();
    File_पंक्ति* copy_Or_Same_File_पंक्ति();
    File_पंक्ति* copy_Or_Same_File_पंक्ति_Applied();
    अटल व्योम deleteAllRemaining();
    अटल व्योम stats();
    ~File_पंक्ति();
#अगर_घोषित VL_LEAK_CHECKS
    अटल व्योम* चालक new(माप_प्रकार आकार);
    अटल व्योम चालक delete(व्योम* obj, माप_प्रकार आकार);
#पूर्ण_अगर
    // METHODS
    व्योम newContent();
    व्योम contentLineno(पूर्णांक अंक) {
        lineno(अंक);
        m_contentLineno = static_cast<अचिन्हित>(अंक);
    }
    व्योम contentLinenoFrom(स्थिर File_पंक्ति* fromp) { m_contentLineno = fromp->m_contentLineno; }
    व्योम lineno(पूर्णांक अंक) {
        m_firstLineno = अंक;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    व्योम column(पूर्णांक firstColumn, पूर्णांक lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    व्योम language(V3LangCode lang) स्थिर { singleton().संख्या_To_Lang(filenameno(), lang); }
    व्योम filename(स्थिर string& name) { m_filenameno = singleton().name_To_संख्या(name); }
    व्योम parent(File_पंक्ति* fileline) { m_parent = fileline; }
    व्योम पंक्ति_Directive(स्थिर अक्षर* textp, पूर्णांक& enterExitRef);
    व्योम पंक्ति_Directive_Parse(स्थिर अक्षर* textp, string& filenameRef, पूर्णांक& linenoRef,
                            पूर्णांक& enterExitRef);
    व्योम linenoInc() {
        अगर (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } अन्यथा {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    व्योम startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    व्योम forwardToken(स्थिर अक्षर* textp, माप_प्रकार आकार, bool track_पंक्तियाँ = true);
    uint32_t token_अंक() स्थिर VL_MT_SAFE { वापस m_token_अंक; }
    पूर्णांक firstLineno() स्थिर VL_MT_SAFE { वापस m_firstLineno; }
    पूर्णांक firstColumn() स्थिर VL_MT_SAFE { वापस static_cast<पूर्णांक>(m_firstColumn); }
    पूर्णांक lastLineno() स्थिर VL_MT_SAFE { वापस m_firstLineno + m_lastLinenoAdder; }
    पूर्णांक lastColumn() स्थिर VL_MT_SAFE { वापस static_cast<पूर्णांक>(m_lastColumn); }
    VFileContent* contentp() स्थिर { वापस m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    पूर्णांक lineno() स्थिर VL_MT_SAFE { वापस lastLineno(); }
    string source() स्थिर VL_MT_SAFE;
    string sourcePrefix(पूर्णांक toColumn) स्थिर VL_MT_SAFE;
    string prettySource() स्थिर VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_पंक्ति* parent() स्थिर VL_MT_SAFE { वापस m_parent; }
    V3LangCode language() स्थिर { वापस singleton().संख्या_To_Lang(filenameno()); }
    string ascii() स्थिर VL_MT_SAFE;
    string ascii_पंक्ति_Col() स्थिर;
    पूर्णांक filenameno() स्थिर VL_MT_SAFE { वापस m_filenameno; }
    स्थिर std::string& filename() स्थिर VL_MT_SAFE {
        वापस singleton().संख्या_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() स्थिर VL_MT_SAFE { वापस VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() स्थिर VL_MT_SAFE {
        वापस (filename() == command_पंक्ति_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() स्थिर VL_MT_SAFE {
        वापस File_पंक्ति_Singleton::filenameLetters(filenameno());
    }
    string filebasename() स्थिर VL_MT_SAFE;
    string filebasenameNoExt() स्थिर;
    string firstColumnLetters() स्थिर VL_MT_SAFE;
    string profileFuncname() स्थिर;
    string पंक्ति_Directive_Strg(पूर्णांक enterExit) स्थिर;

    // Turn on/off warning messages on this line.
निजी:
    व्योम warnSet(संदेश_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_संदेश_En_Idx = singleton().संदेश_En_Set_Bit(m_संदेश_En_Idx, subset, code, flag);
    }

खुला:
    व्योम warnOn(V3ErrorCode code, bool flag) { warnSet(संदेश_En_Bit_Set::Subset::CODE, code, flag); }
    व्योम warnSetCtrlBitSet(स्थिर VErrorBitSet& bitset) {
        m_संदेश_En_Idx = singleton().संदेश_Set_Ctrl_Bit_Set(m_संदेश_En_Idx, bitset);
    }
    व्योम warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(स्थिर string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) स्थिर VL_MT_SAFE;
    व्योम warnLintOff(bool turnOff);
    व्योम warnStyleOff(bool turnOff);
    व्योम warnStateFrom(स्थिर File_पंक्ति& from) { m_संदेश_En_Idx = from.m_संदेश_En_Idx; }
    व्योम warnResetDefault() { warnStateFrom(default_File_पंक्ति()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() स्थिर { वापस संदेश_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    व्योम celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() स्थिर { वापस संदेश_En().enabled(V3ErrorCode::I_COVERAGE); }
    व्योम coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() स्थिर { वापस संदेश_En().enabled(V3ErrorCode::I_ERRORING); }
    व्योम erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() स्थिर { वापस संदेश_En().enabled(V3ErrorCode::I_TRACING); }
    व्योम tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() स्थिर { वापस संदेश_En().enabled(V3ErrorCode::I_TIMING); }
    व्योम timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    अटल string command_पंक्ति_Filename() VL_MT_SAFE { वापस "<command-line>"; }
    अटल string builtInFilename() VL_MT_SAFE { वापस "<built-in>"; }
    अटल व्योम globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_पंक्ति().warnOff(code, turnOff);
    }
    अटल string globalWarnOffParse(स्थिर string& msgs, bool turnOff) {
        वापस default_File_पंक्ति().warnOffParse(msgs, turnOff);
    }
    अटल व्योम file_Name_अंक_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_अंक_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    व्योम modifyStateInherit(स्थिर File_पंक्ति* fromp) {
        m_संदेश_En_Idx = singleton().संदेश_En_And(m_संदेश_En_Idx, fromp->m_संदेश_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    व्योम modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    व्योम v3errorEnd(std::ostringstream& str, स्थिर string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    व्योम v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        निश्चित(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() स्थिर VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() स्थिर VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() स्थिर VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        वापस warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() स्थिर { वापस warnContext(); }
    bool चालक==(स्थिर File_पंक्ति& rhs) स्थिर {
        वापस (m_token_अंक == rhs.m_token_अंक && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_संदेश_En_Idx == rhs.m_संदेश_En_Idx);
    }
    bool equal_First_पंक्ति_Col(स्थिर File_पंक्ति& rhs) स्थिर {
        वापस (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    पूर्णांक operatorCompare(स्थिर File_पंक्ति& rhs) स्थिर {
        अगर (m_filenameno != rhs.m_filenameno) वापस (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        अगर (m_firstLineno != rhs.m_firstLineno)
            वापस (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        अगर (m_firstColumn != rhs.m_firstColumn)
            वापस (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        अगर (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            वापस (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        अगर (m_lastColumn != rhs.m_lastColumn) वापस (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        स्थिर संदेश_En_Bit_Set& lhs_संदेश_En = संदेश_En();
        स्थिर संदेश_En_Bit_Set& rhs_संदेश_En = rhs.संदेश_En();
        क्रम (माप_प्रकार i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            स्थिर V3ErrorCode code = static_cast<V3ErrorCode>(i);
            अगर (lhs_संदेश_En.enabled(code) != rhs_संदेश_En.enabled(code))
                वापस rhs_संदेश_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        अगर (m_token_अंक != rhs.m_token_अंक) वापस (m_token_अंक < rhs.m_token_अंक) ? -1 : 1;
        वापस 0;  // (*this) and rhs are equivalent
    }

निजी:
    string warnContext() स्थिर;
    string warnContextParent() स्थिर VL_REQUIRES(V3Error::s().m_mutex);
    स्थिर संदेश_En_Bit_Set& संदेश_En() स्थिर VL_MT_SAFE { वापस singleton().संदेश_En(m_संदेश_En_Idx); }
};
std::ostream& चालक<<(std::ostream& os, File_पंक्ति* fileline);

#पूर्ण_अगर  // Guard
