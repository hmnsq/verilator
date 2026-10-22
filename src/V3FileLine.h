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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Hash.h"
#अन्तर्भाव "V3LangCode.h"
#अन्तर्भाव "V3Mutex.h"

#अन्तर्भाव <atomic>
#अन्तर्भाव <deque>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <set>
#अन्तर्भाव <sstream>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

// ######################################################################

class File_पङ्क्ति;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_पङ्क्ति_Singleton final {
    मित्र class File_पङ्क्ति;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using सन्देश_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class सन्देश_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    सार्वजनिक:
        गणना class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        सन्देश_En_Bit_Set() = स्वभाव;
        // Create intersection set
        सन्देश_En_Bit_Set(अचर सन्देश_En_Bit_Set& a, अचर सन्देश_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        संरचना Hash final {
            माप_प्रकार चालक()(अचर सन्देश_En_Bit_Set& item) अचर {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                फर्काऊ hash.मान();
            }
        };

        संरचना Equal final {
            bool चालक()(अचर सन्देश_En_Bit_Set& a, अचर सन्देश_En_Bit_Set& b) अचर { फर्काऊ a == b; }
        };

        bool चालक==(अचर सन्देश_En_Bit_Set& other) अचर {
            फर्काऊ m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        अचर VErrorBitSet& getAll(Subset subset) अचर {
            फर्काऊ subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) अचर {
            फर्काऊ subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        शून्य setAll(Subset subset, अचर VErrorBitSet& bitset) {
            यदि (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } अन्यथा {
                m_ctrlEn = bitset;
            }
        }
        शून्य set(Subset subset, V3ErrorCode code, bool मान) {
            यदि (subset == Subset::CODE) {
                m_codeEn.set(code, मान);
            } अन्यथा {
                m_ctrlEn.set(code, मान);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) अचर { फर्काऊ m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<अचर std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<सन्देश_En_Bit_Set, सन्देश_En_Set_Idx_t, सन्देश_En_Bit_Set::Hash, सन्देश_En_Bit_Set::Equal>
        m_interned_सन्देश_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<सन्देश_En_Bit_Set> m_interned_सन्देश_Ens;

    // CONSTRUCTORS
    File_पङ्क्ति_Singleton() = स्वभाव;
    ~File_पङ्क्ति_Singleton() = स्वभाव;

    fileNameIdx_t name_To_सङ्ख्या(अचर string& filename);
    अचर std::string& सङ्ख्या_To_Name(fileNameIdx_t filenameno) अचर VL_MT_SAFE {
        फर्काऊ m_names[filenameno];
    }
    V3LangCode सङ्ख्या_To_Lang(fileNameIdx_t filenameno) अचर { फर्काऊ m_languages[filenameno]; }
    शून्य सङ्ख्या_To_Lang(fileNameIdx_t filenameno, अचर V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    शून्य clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    शून्य file_Name_अङ्क_Map_Dump_Json(std::ostream& os);
    स्थिर string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    सन्देश_En_Set_Idx_t add_सन्देश_En_Bit_Set(अचर सन्देश_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    सन्देश_En_Set_Idx_t default_सन्देश_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    सन्देश_En_Set_Idx_t सन्देश_En_Set_Bit(सन्देश_En_Set_Idx_t setIdx, सन्देश_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool मान);
    // Bulk-set all control codes to given bitset
    सन्देश_En_Set_Idx_t सन्देश_Set_Ctrl_Bit_Set(सन्देश_En_Set_Idx_t setIdx, अचर VErrorBitSet& bitset);
    // Return index to intersection set
    सन्देश_En_Set_Idx_t सन्देश_En_And(सन्देश_En_Set_Idx_t lhsIdx, सन्देश_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    अचर सन्देश_En_Bit_Set& सन्देश_En(सन्देश_En_Set_Idx_t idx) अचर VL_MT_SAFE {
        फर्काऊ m_interned_सन्देश_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    मित्र class File_पङ्क्ति;
    // MEMBERS
    पूर्णाङ्क m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<माप_प्रकार> m_ref_गन्ती{0};
    std::deque<string> m_पङ्क्तिहरू;  // Source text lines
    VFileContent() {
        स्थिर पूर्णाङ्क s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = स्वभाव;
    // METHODS
    शून्य refInc() { ++m_ref_गन्ती; }
    शून्य refDec() {
        यदि (!--m_ref_गन्ती) delete this;
    }

सार्वजनिक:
    शून्य थप_Text(अचर string& text);  // Add arbitrary text (need not be line-by-line)
    string get_पङ्क्ति(पूर्णाङ्क lineno) अचर VL_MT_SAFE;
    string ascii() अचर { फर्काऊ "ct" + cvtToStr(m_id); }
};
std::ostream& चालक<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_पङ्क्ति final {

    // CONSTANTS
    स्थिर constexpr अचिन्हित SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_पङ्क्ति_Singleton::fileNameIdx_t;
    using सन्देश_En_Set_Idx_t = File_पङ्क्ति_Singleton::सन्देश_En_Set_Idx_t;
    using सन्देश_En_Bit_Set = File_पङ्क्ति_Singleton::सन्देश_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    सन्देश_En_Set_Idx_t m_सन्देश_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    अचिन्हित m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_अङ्क = 0;  // Token number in processing order
    पूर्णाङ्क m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_पङ्क्ति* m_parent = nullptr;  // Parent line that included this line

रक्षित:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    मित्र class File_पङ्क्ति_Singleton;
    मित्र class V3ParseImp;
    मित्र class V3PreLex;
    मित्र class V3PreProcImp;
    मित्र class V3PreShellImp;

गुप्त:
    // CONSTRUCTORS
    स्थिर File_पङ्क्ति_Singleton& singleton() VL_MT_SAFE {
        स्थिर File_पङ्क्ति_Singleton s;
        फर्काऊ s;
    }
    स्थिर File_पङ्क्ति& default_File_पङ्क्ति() VL_MT_SAFE {
        स्थिर File_पङ्क्ति s;
        फर्काऊ s;
    }

    File_पङ्क्ति()  // Only used for defaultFileLine above
        : m_सन्देश_En_Idx{singleton().default_सन्देश_En_Index()}
        , m_filenameno{singleton().name_To_सङ्ख्या(File_पङ्क्ति::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

सार्वजनिक:
    explicit File_पङ्क्ति(अचर string& filename)
        : m_सन्देश_En_Idx{default_File_पङ्क्ति().m_सन्देश_En_Idx}
        , m_filenameno{singleton().name_To_सङ्ख्या(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_पङ्क्ति(अचर File_पङ्क्ति& from)
        : m_सन्देश_En_Idx{from.m_सन्देश_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_अङ्क{from.m_token_अङ्क}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        यदि (m_contentp) m_contentp->refInc();
    }
    explicit File_पङ्क्ति(File_पङ्क्ति* fromp)
        : m_सन्देश_En_Idx{fromp->m_सन्देश_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_अङ्क{fromp->m_token_अङ्क}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        यदि (m_contentp) m_contentp->refInc();
    }
    शून्य applyIgnores();
    File_पङ्क्ति* copy_Or_Same_File_पङ्क्ति();
    File_पङ्क्ति* copy_Or_Same_File_पङ्क्ति_Applied();
    स्थिर शून्य deleteAllRemaining();
    स्थिर शून्य stats();
    ~File_पङ्क्ति();
#ifdef VL_LEAK_CHECKS
    स्थिर शून्य* चालक new(माप_प्रकार size);
    स्थिर शून्य चालक delete(शून्य* obj, माप_प्रकार size);
#endif
    // METHODS
    शून्य newContent();
    शून्य contentLineno(पूर्णाङ्क अङ्क) {
        lineno(अङ्क);
        m_contentLineno = static_cast<अचिन्हित>(अङ्क);
    }
    शून्य contentLinenoFrom(अचर File_पङ्क्ति* fromp) { m_contentLineno = fromp->m_contentLineno; }
    शून्य lineno(पूर्णाङ्क अङ्क) {
        m_firstLineno = अङ्क;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    शून्य column(पूर्णाङ्क firstColumn, पूर्णाङ्क lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    शून्य language(V3LangCode lang) अचर { singleton().सङ्ख्या_To_Lang(filenameno(), lang); }
    शून्य filename(अचर string& name) { m_filenameno = singleton().name_To_सङ्ख्या(name); }
    शून्य parent(File_पङ्क्ति* fileline) { m_parent = fileline; }
    शून्य पङ्क्ति_Directive(अचर वर्ण* textp, पूर्णाङ्क& enterExitRef);
    शून्य पङ्क्ति_Directive_Parse(अचर वर्ण* textp, string& filenameRef, पूर्णाङ्क& linenoRef,
                            पूर्णाङ्क& enterExitRef);
    शून्य linenoInc() {
        यदि (m_lastLinenoAdder == 65535) {
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
    शून्य startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    शून्य forwardToken(अचर वर्ण* textp, माप_प्रकार size, bool track_पङ्क्तिहरू = true);
    uint32_t token_अङ्क() अचर VL_MT_SAFE { फर्काऊ m_token_अङ्क; }
    पूर्णाङ्क firstLineno() अचर VL_MT_SAFE { फर्काऊ m_firstLineno; }
    पूर्णाङ्क firstColumn() अचर VL_MT_SAFE { फर्काऊ static_cast<पूर्णाङ्क>(m_firstColumn); }
    पूर्णाङ्क lastLineno() अचर VL_MT_SAFE { फर्काऊ m_firstLineno + m_lastLinenoAdder; }
    पूर्णाङ्क lastColumn() अचर VL_MT_SAFE { फर्काऊ static_cast<पूर्णाङ्क>(m_lastColumn); }
    VFileContent* contentp() अचर { फर्काऊ m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    पूर्णाङ्क lineno() अचर VL_MT_SAFE { फर्काऊ lastLineno(); }
    string source() अचर VL_MT_SAFE;
    string sourcePrefix(पूर्णाङ्क toColumn) अचर VL_MT_SAFE;
    string prettySource() अचर VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_पङ्क्ति* parent() अचर VL_MT_SAFE { फर्काऊ m_parent; }
    V3LangCode language() अचर { फर्काऊ singleton().सङ्ख्या_To_Lang(filenameno()); }
    string ascii() अचर VL_MT_SAFE;
    string ascii_पङ्क्ति_Col() अचर;
    पूर्णाङ्क filenameno() अचर VL_MT_SAFE { फर्काऊ m_filenameno; }
    अचर std::string& filename() अचर VL_MT_SAFE {
        फर्काऊ singleton().सङ्ख्या_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() अचर VL_MT_SAFE { फर्काऊ VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() अचर VL_MT_SAFE {
        फर्काऊ (filename() == command_पङ्क्ति_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() अचर VL_MT_SAFE {
        फर्काऊ File_पङ्क्ति_Singleton::filenameLetters(filenameno());
    }
    string filebasename() अचर VL_MT_SAFE;
    string filebasenameNoExt() अचर;
    string firstColumnLetters() अचर VL_MT_SAFE;
    string profileFuncname() अचर;
    string पङ्क्ति_Directive_Strg(पूर्णाङ्क enterExit) अचर;

    // Turn on/off warning messages on this line.
गुप्त:
    शून्य warnSet(सन्देश_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_सन्देश_En_Idx = singleton().सन्देश_En_Set_Bit(m_सन्देश_En_Idx, subset, code, flag);
    }

सार्वजनिक:
    शून्य warnOn(V3ErrorCode code, bool flag) { warnSet(सन्देश_En_Bit_Set::Subset::CODE, code, flag); }
    शून्य warnSetCtrlBitSet(अचर VErrorBitSet& bitset) {
        m_सन्देश_En_Idx = singleton().सन्देश_Set_Ctrl_Bit_Set(m_सन्देश_En_Idx, bitset);
    }
    शून्य warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(अचर string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) अचर VL_MT_SAFE;
    शून्य warnLintOff(bool turnOff);
    शून्य warnStyleOff(bool turnOff);
    शून्य warnStateFrom(अचर File_पङ्क्ति& from) { m_सन्देश_En_Idx = from.m_सन्देश_En_Idx; }
    शून्य warnResetDefault() { warnStateFrom(default_File_पङ्क्ति()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() अचर { फर्काऊ सन्देश_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    शून्य celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() अचर { फर्काऊ सन्देश_En().enabled(V3ErrorCode::I_COVERAGE); }
    शून्य coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() अचर { फर्काऊ सन्देश_En().enabled(V3ErrorCode::I_ERRORING); }
    शून्य erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() अचर { फर्काऊ सन्देश_En().enabled(V3ErrorCode::I_TRACING); }
    शून्य tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() अचर { फर्काऊ सन्देश_En().enabled(V3ErrorCode::I_TIMING); }
    शून्य timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    स्थिर string command_पङ्क्ति_Filename() VL_MT_SAFE { फर्काऊ "<command-line>"; }
    स्थिर string builtInFilename() VL_MT_SAFE { फर्काऊ "<built-in>"; }
    स्थिर शून्य globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_पङ्क्ति().warnOff(code, turnOff);
    }
    स्थिर string globalWarnOffParse(अचर string& msgs, bool turnOff) {
        फर्काऊ default_File_पङ्क्ति().warnOffParse(msgs, turnOff);
    }
    स्थिर शून्य file_Name_अङ्क_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_अङ्क_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    शून्य modifyStateInherit(अचर File_पङ्क्ति* fromp) {
        m_सन्देश_En_Idx = singleton().सन्देश_En_And(m_सन्देश_En_Idx, fromp->m_सन्देश_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    शून्य modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    शून्य v3errorEnd(std::ostringstream& str, अचर string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        प्रतिज्ञा(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() अचर VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() अचर VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() अचर VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        फर्काऊ warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() अचर { फर्काऊ warnContext(); }
    bool चालक==(अचर File_पङ्क्ति& rhs) अचर {
        फर्काऊ (m_token_अङ्क == rhs.m_token_अङ्क && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_सन्देश_En_Idx == rhs.m_सन्देश_En_Idx);
    }
    bool equal_First_पङ्क्ति_Col(अचर File_पङ्क्ति& rhs) अचर {
        फर्काऊ (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    पूर्णाङ्क operatorCompare(अचर File_पङ्क्ति& rhs) अचर {
        यदि (m_filenameno != rhs.m_filenameno) फर्काऊ (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        यदि (m_firstLineno != rhs.m_firstLineno)
            फर्काऊ (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        यदि (m_firstColumn != rhs.m_firstColumn)
            फर्काऊ (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        यदि (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            फर्काऊ (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        यदि (m_lastColumn != rhs.m_lastColumn) फर्काऊ (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        अचर सन्देश_En_Bit_Set& lhs_सन्देश_En = सन्देश_En();
        अचर सन्देश_En_Bit_Set& rhs_सन्देश_En = rhs.सन्देश_En();
        लागि (माप_प्रकार i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            अचर V3ErrorCode code = static_cast<V3ErrorCode>(i);
            यदि (lhs_सन्देश_En.enabled(code) != rhs_सन्देश_En.enabled(code))
                फर्काऊ rhs_सन्देश_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        यदि (m_token_अङ्क != rhs.m_token_अङ्क) फर्काऊ (m_token_अङ्क < rhs.m_token_अङ्क) ? -1 : 1;
        फर्काऊ 0;  // (*this) and rhs are equivalent
    }

गुप्त:
    string warnContext() अचर;
    string warnContextParent() अचर VL_REQUIRES(V3Error::s().m_mutex);
    अचर सन्देश_En_Bit_Set& सन्देश_En() अचर VL_MT_SAFE { फर्काऊ singleton().सन्देश_En(m_सन्देश_En_Idx); }
};
std::ostream& चालक<<(std::ostream& os, File_पङ्क्ति* fileline);

#endif  // Guard
