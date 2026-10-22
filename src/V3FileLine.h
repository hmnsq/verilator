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

class File_ओळ;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_ओळ_Singleton final {
    friend class File_ओळ;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using संदेश_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class संदेश_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    सार्वजनिक:
        गणना class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        संदेश_En_Bit_Set() = स्वभाव;
        // Create intersection set
        संदेश_En_Bit_Set(ध्रुव संदेश_En_Bit_Set& a, ध्रुव संदेश_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        संरचना Hash final {
            size_t operator()(ध्रुव संदेश_En_Bit_Set& item) ध्रुव {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                प्रत्यागम hash.मूल्य();
            }
        };

        संरचना Equal final {
            bool operator()(ध्रुव संदेश_En_Bit_Set& a, ध्रुव संदेश_En_Bit_Set& b) ध्रुव { प्रत्यागम a == b; }
        };

        bool operator==(ध्रुव संदेश_En_Bit_Set& other) ध्रुव {
            प्रत्यागम m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        ध्रुव VErrorBitSet& getAll(Subset subset) ध्रुव {
            प्रत्यागम subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) ध्रुव {
            प्रत्यागम subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        शून्य setAll(Subset subset, ध्रुव VErrorBitSet& bitset) {
            यदि (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } अन्यथा {
                m_ctrlEn = bitset;
            }
        }
        शून्य set(Subset subset, V3ErrorCode code, bool मूल्य) {
            यदि (subset == Subset::CODE) {
                m_codeEn.set(code, मूल्य);
            } अन्यथा {
                m_ctrlEn.set(code, मूल्य);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) ध्रुव { प्रत्यागम m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ध्रुव std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<संदेश_En_Bit_Set, संदेश_En_Set_Idx_t, संदेश_En_Bit_Set::Hash, संदेश_En_Bit_Set::Equal>
        m_interned_संदेश_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<संदेश_En_Bit_Set> m_interned_संदेश_Ens;

    // CONSTRUCTORS
    File_ओळ_Singleton() = स्वभाव;
    ~File_ओळ_Singleton() = स्वभाव;

    fileNameIdx_t name_To_संख्या(ध्रुव string& filename);
    ध्रुव std::string& संख्या_To_Name(fileNameIdx_t filenameno) ध्रुव VL_MT_SAFE {
        प्रत्यागम m_names[filenameno];
    }
    V3LangCode संख्या_To_Lang(fileNameIdx_t filenameno) ध्रुव { प्रत्यागम m_languages[filenameno]; }
    शून्य संख्या_To_Lang(fileNameIdx_t filenameno, ध्रुव V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    शून्य clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    शून्य file_Name_अंक_Map_Dump_Json(std::ostream& os);
    स्थिर string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    संदेश_En_Set_Idx_t add_संदेश_En_Bit_Set(ध्रुव संदेश_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    संदेश_En_Set_Idx_t default_संदेश_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    संदेश_En_Set_Idx_t संदेश_En_Set_Bit(संदेश_En_Set_Idx_t setIdx, संदेश_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool मूल्य);
    // Bulk-set all control codes to given bitset
    संदेश_En_Set_Idx_t संदेश_Set_Ctrl_Bit_Set(संदेश_En_Set_Idx_t setIdx, ध्रुव VErrorBitSet& bitset);
    // Return index to intersection set
    संदेश_En_Set_Idx_t संदेश_En_And(संदेश_En_Set_Idx_t lhsIdx, संदेश_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ध्रुव संदेश_En_Bit_Set& संदेश_En(संदेश_En_Set_Idx_t idx) ध्रुव VL_MT_SAFE {
        प्रत्यागम m_interned_संदेश_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_ओळ;
    // MEMBERS
    पूर्णाङ्क m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_मोजणी{0};
    std::deque<string> m_ओळी;  // Source text lines
    VFileContent() {
        स्थिर पूर्णाङ्क s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = स्वभाव;
    // METHODS
    शून्य refInc() { ++m_ref_मोजणी; }
    शून्य refDec() {
        यदि (!--m_ref_मोजणी) delete this;
    }

सार्वजनिक:
    शून्य ढकला_Text(ध्रुव string& text);  // Add arbitrary text (need not be line-by-line)
    string get_ओळ(पूर्णाङ्क lineno) ध्रुव VL_MT_SAFE;
    string ascii() ध्रुव { प्रत्यागम "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_ओळ final {

    // CONSTANTS
    स्थिर constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_ओळ_Singleton::fileNameIdx_t;
    using संदेश_En_Set_Idx_t = File_ओळ_Singleton::संदेश_En_Set_Idx_t;
    using संदेश_En_Bit_Set = File_ओळ_Singleton::संदेश_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    संदेश_En_Set_Idx_t m_संदेश_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_अंक = 0;  // Token number in processing order
    पूर्णाङ्क m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_ओळ* m_parent = nullptr;  // Parent line that included this line

रक्षित:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_ओळ_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

गुप्त:
    // CONSTRUCTORS
    स्थिर File_ओळ_Singleton& singleton() VL_MT_SAFE {
        स्थिर File_ओळ_Singleton s;
        प्रत्यागम s;
    }
    स्थिर File_ओळ& default_File_ओळ() VL_MT_SAFE {
        स्थिर File_ओळ s;
        प्रत्यागम s;
    }

    File_ओळ()  // Only used for defaultFileLine above
        : m_संदेश_En_Idx{singleton().default_संदेश_En_Index()}
        , m_filenameno{singleton().name_To_संख्या(File_ओळ::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

सार्वजनिक:
    explicit File_ओळ(ध्रुव string& filename)
        : m_संदेश_En_Idx{default_File_ओळ().m_संदेश_En_Idx}
        , m_filenameno{singleton().name_To_संख्या(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_ओळ(ध्रुव File_ओळ& from)
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
        यदि (m_contentp) m_contentp->refInc();
    }
    explicit File_ओळ(File_ओळ* fromp)
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
        यदि (m_contentp) m_contentp->refInc();
    }
    शून्य applyIgnores();
    File_ओळ* copy_Or_Same_File_ओळ();
    File_ओळ* copy_Or_Same_File_ओळ_Applied();
    स्थिर शून्य deleteAllRemaining();
    स्थिर शून्य stats();
    ~File_ओळ();
#ifdef VL_LEAK_CHECKS
    स्थिर शून्य* operator new(size_t size);
    स्थिर शून्य operator delete(शून्य* obj, size_t size);
#endif
    // METHODS
    शून्य newContent();
    शून्य contentLineno(पूर्णाङ्क अंक) {
        lineno(अंक);
        m_contentLineno = static_cast<unsigned>(अंक);
    }
    शून्य contentLinenoFrom(ध्रुव File_ओळ* fromp) { m_contentLineno = fromp->m_contentLineno; }
    शून्य lineno(पूर्णाङ्क अंक) {
        m_firstLineno = अंक;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    शून्य column(पूर्णाङ्क firstColumn, पूर्णाङ्क lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    शून्य language(V3LangCode lang) ध्रुव { singleton().संख्या_To_Lang(filenameno(), lang); }
    शून्य filename(ध्रुव string& name) { m_filenameno = singleton().name_To_संख्या(name); }
    शून्य parent(File_ओळ* fileline) { m_parent = fileline; }
    शून्य ओळ_Directive(ध्रुव वर्ण* textp, पूर्णाङ्क& enterExitRef);
    शून्य ओळ_Directive_Parse(ध्रुव वर्ण* textp, string& filenameRef, पूर्णाङ्क& linenoRef,
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
    शून्य forwardToken(ध्रुव वर्ण* textp, size_t size, bool track_ओळी = true);
    uint32_t token_अंक() ध्रुव VL_MT_SAFE { प्रत्यागम m_token_अंक; }
    पूर्णाङ्क firstLineno() ध्रुव VL_MT_SAFE { प्रत्यागम m_firstLineno; }
    पूर्णाङ्क firstColumn() ध्रुव VL_MT_SAFE { प्रत्यागम static_cast<पूर्णाङ्क>(m_firstColumn); }
    पूर्णाङ्क lastLineno() ध्रुव VL_MT_SAFE { प्रत्यागम m_firstLineno + m_lastLinenoAdder; }
    पूर्णाङ्क lastColumn() ध्रुव VL_MT_SAFE { प्रत्यागम static_cast<पूर्णाङ्क>(m_lastColumn); }
    VFileContent* contentp() ध्रुव { प्रत्यागम m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    पूर्णाङ्क lineno() ध्रुव VL_MT_SAFE { प्रत्यागम lastLineno(); }
    string source() ध्रुव VL_MT_SAFE;
    string sourcePrefix(पूर्णाङ्क toColumn) ध्रुव VL_MT_SAFE;
    string prettySource() ध्रुव VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_ओळ* parent() ध्रुव VL_MT_SAFE { प्रत्यागम m_parent; }
    V3LangCode language() ध्रुव { प्रत्यागम singleton().संख्या_To_Lang(filenameno()); }
    string ascii() ध्रुव VL_MT_SAFE;
    string ascii_ओळ_Col() ध्रुव;
    पूर्णाङ्क filenameno() ध्रुव VL_MT_SAFE { प्रत्यागम m_filenameno; }
    ध्रुव std::string& filename() ध्रुव VL_MT_SAFE {
        प्रत्यागम singleton().संख्या_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ध्रुव VL_MT_SAFE { प्रत्यागम VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ध्रुव VL_MT_SAFE {
        प्रत्यागम (filename() == command_ओळ_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() ध्रुव VL_MT_SAFE {
        प्रत्यागम File_ओळ_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ध्रुव VL_MT_SAFE;
    string filebasenameNoExt() ध्रुव;
    string firstColumnLetters() ध्रुव VL_MT_SAFE;
    string profileFuncname() ध्रुव;
    string ओळ_Directive_Strg(पूर्णाङ्क enterExit) ध्रुव;

    // Turn on/off warning messages on this line.
गुप्त:
    शून्य warnSet(संदेश_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_संदेश_En_Idx = singleton().संदेश_En_Set_Bit(m_संदेश_En_Idx, subset, code, flag);
    }

सार्वजनिक:
    शून्य warnOn(V3ErrorCode code, bool flag) { warnSet(संदेश_En_Bit_Set::Subset::CODE, code, flag); }
    शून्य warnSetCtrlBitSet(ध्रुव VErrorBitSet& bitset) {
        m_संदेश_En_Idx = singleton().संदेश_Set_Ctrl_Bit_Set(m_संदेश_En_Idx, bitset);
    }
    शून्य warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(ध्रुव string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) ध्रुव VL_MT_SAFE;
    शून्य warnLintOff(bool turnOff);
    शून्य warnStyleOff(bool turnOff);
    शून्य warnStateFrom(ध्रुव File_ओळ& from) { m_संदेश_En_Idx = from.m_संदेश_En_Idx; }
    शून्य warnResetDefault() { warnStateFrom(default_File_ओळ()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ध्रुव { प्रत्यागम संदेश_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    शून्य celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() ध्रुव { प्रत्यागम संदेश_En().enabled(V3ErrorCode::I_COVERAGE); }
    शून्य coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() ध्रुव { प्रत्यागम संदेश_En().enabled(V3ErrorCode::I_ERRORING); }
    शून्य erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() ध्रुव { प्रत्यागम संदेश_En().enabled(V3ErrorCode::I_TRACING); }
    शून्य tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() ध्रुव { प्रत्यागम संदेश_En().enabled(V3ErrorCode::I_TIMING); }
    शून्य timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    स्थिर string command_ओळ_Filename() VL_MT_SAFE { प्रत्यागम "<command-line>"; }
    स्थिर string builtInFilename() VL_MT_SAFE { प्रत्यागम "<built-in>"; }
    स्थिर शून्य globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_ओळ().warnOff(code, turnOff);
    }
    स्थिर string globalWarnOffParse(ध्रुव string& msgs, bool turnOff) {
        प्रत्यागम default_File_ओळ().warnOffParse(msgs, turnOff);
    }
    स्थिर शून्य file_Name_अंक_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_अंक_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    शून्य modifyStateInherit(ध्रुव File_ओळ* fromp) {
        m_संदेश_En_Idx = singleton().संदेश_En_And(m_संदेश_En_Idx, fromp->m_संदेश_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    शून्य modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    शून्य v3errorEnd(std::ostringstream& str, ध्रुव string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        प्रतिज्ञा(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() ध्रुव VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ध्रुव VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ध्रुव VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        प्रत्यागम warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ध्रुव { प्रत्यागम warnContext(); }
    bool operator==(ध्रुव File_ओळ& rhs) ध्रुव {
        प्रत्यागम (m_token_अंक == rhs.m_token_अंक && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_संदेश_En_Idx == rhs.m_संदेश_En_Idx);
    }
    bool equal_First_ओळ_Col(ध्रुव File_ओळ& rhs) ध्रुव {
        प्रत्यागम (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    पूर्णाङ्क operatorCompare(ध्रुव File_ओळ& rhs) ध्रुव {
        यदि (m_filenameno != rhs.m_filenameno) प्रत्यागम (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        यदि (m_firstLineno != rhs.m_firstLineno)
            प्रत्यागम (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        यदि (m_firstColumn != rhs.m_firstColumn)
            प्रत्यागम (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        यदि (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            प्रत्यागम (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        यदि (m_lastColumn != rhs.m_lastColumn) प्रत्यागम (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        ध्रुव संदेश_En_Bit_Set& lhs_संदेश_En = संदेश_En();
        ध्रुव संदेश_En_Bit_Set& rhs_संदेश_En = rhs.संदेश_En();
        प्रति (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            ध्रुव V3ErrorCode code = static_cast<V3ErrorCode>(i);
            यदि (lhs_संदेश_En.enabled(code) != rhs_संदेश_En.enabled(code))
                प्रत्यागम rhs_संदेश_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        यदि (m_token_अंक != rhs.m_token_अंक) प्रत्यागम (m_token_अंक < rhs.m_token_अंक) ? -1 : 1;
        प्रत्यागम 0;  // (*this) and rhs are equivalent
    }

गुप्त:
    string warnContext() ध्रुव;
    string warnContextParent() ध्रुव VL_REQUIRES(V3Error::s().m_mutex);
    ध्रुव संदेश_En_Bit_Set& संदेश_En() ध्रुव VL_MT_SAFE { प्रत्यागम singleton().संदेश_En(m_संदेश_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_ओळ* fileline);

#endif  // Guard
