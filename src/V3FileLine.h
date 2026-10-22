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
    मित्र class File_ओळ;

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
        संदेश_En_Bit_Set() = पूर्वनिर्धारित;
        // Create intersection set
        संदेश_En_Bit_Set(स्थिरांक संदेश_En_Bit_Set& a, स्थिरांक संदेश_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        संरचना Hash final {
            माप_प्रकार चालक()(स्थिरांक संदेश_En_Bit_Set& item) स्थिरांक {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                परत hash.मूल्य();
            }
        };

        संरचना Equal final {
            bool चालक()(स्थिरांक संदेश_En_Bit_Set& a, स्थिरांक संदेश_En_Bit_Set& b) स्थिरांक { परत a == b; }
        };

        bool चालक==(स्थिरांक संदेश_En_Bit_Set& other) स्थिरांक {
            परत m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        स्थिरांक VErrorBitSet& getAll(Subset subset) स्थिरांक {
            परत subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) स्थिरांक {
            परत subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        शून्य setAll(Subset subset, स्थिरांक VErrorBitSet& bitset) {
            जर (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } अन्यथा {
                m_ctrlEn = bitset;
            }
        }
        शून्य set(Subset subset, V3ErrorCode code, bool मूल्य) {
            जर (subset == Subset::CODE) {
                m_codeEn.set(code, मूल्य);
            } अन्यथा {
                m_ctrlEn.set(code, मूल्य);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) स्थिरांक { परत m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<स्थिरांक std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<संदेश_En_Bit_Set, संदेश_En_Set_Idx_t, संदेश_En_Bit_Set::Hash, संदेश_En_Bit_Set::Equal>
        m_interned_संदेश_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<संदेश_En_Bit_Set> m_interned_संदेश_Ens;

    // CONSTRUCTORS
    File_ओळ_Singleton() = पूर्वनिर्धारित;
    ~File_ओळ_Singleton() = पूर्वनिर्धारित;

    fileNameIdx_t name_To_संख्या(स्थिरांक string& filename);
    स्थिरांक std::string& संख्या_To_Name(fileNameIdx_t filenameno) स्थिरांक VL_MT_SAFE {
        परत m_names[filenameno];
    }
    V3LangCode संख्या_To_Lang(fileNameIdx_t filenameno) स्थिरांक { परत m_languages[filenameno]; }
    शून्य संख्या_To_Lang(fileNameIdx_t filenameno, स्थिरांक V3LangCode& l) {
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
    संदेश_En_Set_Idx_t add_संदेश_En_Bit_Set(स्थिरांक संदेश_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    संदेश_En_Set_Idx_t default_संदेश_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    संदेश_En_Set_Idx_t संदेश_En_Set_Bit(संदेश_En_Set_Idx_t setIdx, संदेश_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool मूल्य);
    // Bulk-set all control codes to given bitset
    संदेश_En_Set_Idx_t संदेश_Set_Ctrl_Bit_Set(संदेश_En_Set_Idx_t setIdx, स्थिरांक VErrorBitSet& bitset);
    // Return index to intersection set
    संदेश_En_Set_Idx_t संदेश_En_And(संदेश_En_Set_Idx_t lhsIdx, संदेश_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    स्थिरांक संदेश_En_Bit_Set& संदेश_En(संदेश_En_Set_Idx_t idx) स्थिरांक VL_MT_SAFE {
        परत m_interned_संदेश_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    मित्र class File_ओळ;
    // MEMBERS
    पूर्णांक m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<माप_प्रकार> m_ref_मोजणी{0};
    std::deque<string> m_ओळी;  // Source text lines
    VFileContent() {
        स्थिर पूर्णांक s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = पूर्वनिर्धारित;
    // METHODS
    शून्य refInc() { ++m_ref_मोजणी; }
    शून्य refDec() {
        जर (!--m_ref_मोजणी) delete this;
    }

सार्वजनिक:
    शून्य ढकला_Text(स्थिरांक string& text);  // Add arbitrary text (need not be line-by-line)
    string get_ओळ(पूर्णांक lineno) स्थिरांक VL_MT_SAFE;
    string ascii() स्थिरांक { परत "ct" + cvtToStr(m_id); }
};
std::ostream& चालक<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_ओळ final {

    // CONSTANTS
    स्थिर constexpr अचिन्हित SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

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
    File_ओळ* m_parent = nullptr;  // Parent line that included this line

संरक्षित:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    मित्र class File_ओळ_Singleton;
    मित्र class V3ParseImp;
    मित्र class V3PreLex;
    मित्र class V3PreProcImp;
    मित्र class V3PreShellImp;

खाजगी:
    // CONSTRUCTORS
    स्थिर File_ओळ_Singleton& singleton() VL_MT_SAFE {
        स्थिर File_ओळ_Singleton s;
        परत s;
    }
    स्थिर File_ओळ& default_File_ओळ() VL_MT_SAFE {
        स्थिर File_ओळ s;
        परत s;
    }

    File_ओळ()  // Only used for defaultFileLine above
        : m_संदेश_En_Idx{singleton().default_संदेश_En_Index()}
        , m_filenameno{singleton().name_To_संख्या(File_ओळ::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

सार्वजनिक:
    explicit File_ओळ(स्थिरांक string& filename)
        : m_संदेश_En_Idx{default_File_ओळ().m_संदेश_En_Idx}
        , m_filenameno{singleton().name_To_संख्या(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_ओळ(स्थिरांक File_ओळ& from)
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
        जर (m_contentp) m_contentp->refInc();
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
        जर (m_contentp) m_contentp->refInc();
    }
    शून्य applyIgnores();
    File_ओळ* copy_Or_Same_File_ओळ();
    File_ओळ* copy_Or_Same_File_ओळ_Applied();
    स्थिर शून्य deleteAllRemaining();
    स्थिर शून्य stats();
    ~File_ओळ();
#ifdef VL_LEAK_CHECKS
    स्थिर शून्य* चालक new(माप_प्रकार size);
    स्थिर शून्य चालक delete(शून्य* obj, माप_प्रकार size);
#endif
    // METHODS
    शून्य newContent();
    शून्य contentLineno(पूर्णांक अंक) {
        lineno(अंक);
        m_contentLineno = static_cast<अचिन्हित>(अंक);
    }
    शून्य contentLinenoFrom(स्थिरांक File_ओळ* fromp) { m_contentLineno = fromp->m_contentLineno; }
    शून्य lineno(पूर्णांक अंक) {
        m_firstLineno = अंक;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    शून्य column(पूर्णांक firstColumn, पूर्णांक lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    शून्य language(V3LangCode lang) स्थिरांक { singleton().संख्या_To_Lang(filenameno(), lang); }
    शून्य filename(स्थिरांक string& name) { m_filenameno = singleton().name_To_संख्या(name); }
    शून्य parent(File_ओळ* fileline) { m_parent = fileline; }
    शून्य ओळ_Directive(स्थिरांक अक्षर* textp, पूर्णांक& enterExitRef);
    शून्य ओळ_Directive_Parse(स्थिरांक अक्षर* textp, string& filenameRef, पूर्णांक& linenoRef,
                            पूर्णांक& enterExitRef);
    शून्य linenoInc() {
        जर (m_lastLinenoAdder == 65535) {
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
    शून्य forwardToken(स्थिरांक अक्षर* textp, माप_प्रकार size, bool track_ओळी = true);
    uint32_t token_अंक() स्थिरांक VL_MT_SAFE { परत m_token_अंक; }
    पूर्णांक firstLineno() स्थिरांक VL_MT_SAFE { परत m_firstLineno; }
    पूर्णांक firstColumn() स्थिरांक VL_MT_SAFE { परत static_cast<पूर्णांक>(m_firstColumn); }
    पूर्णांक lastLineno() स्थिरांक VL_MT_SAFE { परत m_firstLineno + m_lastLinenoAdder; }
    पूर्णांक lastColumn() स्थिरांक VL_MT_SAFE { परत static_cast<पूर्णांक>(m_lastColumn); }
    VFileContent* contentp() स्थिरांक { परत m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    पूर्णांक lineno() स्थिरांक VL_MT_SAFE { परत lastLineno(); }
    string source() स्थिरांक VL_MT_SAFE;
    string sourcePrefix(पूर्णांक toColumn) स्थिरांक VL_MT_SAFE;
    string prettySource() स्थिरांक VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_ओळ* parent() स्थिरांक VL_MT_SAFE { परत m_parent; }
    V3LangCode language() स्थिरांक { परत singleton().संख्या_To_Lang(filenameno()); }
    string ascii() स्थिरांक VL_MT_SAFE;
    string ascii_ओळ_Col() स्थिरांक;
    पूर्णांक filenameno() स्थिरांक VL_MT_SAFE { परत m_filenameno; }
    स्थिरांक std::string& filename() स्थिरांक VL_MT_SAFE {
        परत singleton().संख्या_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() स्थिरांक VL_MT_SAFE { परत VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() स्थिरांक VL_MT_SAFE {
        परत (filename() == command_ओळ_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() स्थिरांक VL_MT_SAFE {
        परत File_ओळ_Singleton::filenameLetters(filenameno());
    }
    string filebasename() स्थिरांक VL_MT_SAFE;
    string filebasenameNoExt() स्थिरांक;
    string firstColumnLetters() स्थिरांक VL_MT_SAFE;
    string profileFuncname() स्थिरांक;
    string ओळ_Directive_Strg(पूर्णांक enterExit) स्थिरांक;

    // Turn on/off warning messages on this line.
खाजगी:
    शून्य warnSet(संदेश_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_संदेश_En_Idx = singleton().संदेश_En_Set_Bit(m_संदेश_En_Idx, subset, code, flag);
    }

सार्वजनिक:
    शून्य warnOn(V3ErrorCode code, bool flag) { warnSet(संदेश_En_Bit_Set::Subset::CODE, code, flag); }
    शून्य warnSetCtrlBitSet(स्थिरांक VErrorBitSet& bitset) {
        m_संदेश_En_Idx = singleton().संदेश_Set_Ctrl_Bit_Set(m_संदेश_En_Idx, bitset);
    }
    शून्य warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(स्थिरांक string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) स्थिरांक VL_MT_SAFE;
    शून्य warnLintOff(bool turnOff);
    शून्य warnStyleOff(bool turnOff);
    शून्य warnStateFrom(स्थिरांक File_ओळ& from) { m_संदेश_En_Idx = from.m_संदेश_En_Idx; }
    शून्य warnResetDefault() { warnStateFrom(default_File_ओळ()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() स्थिरांक { परत संदेश_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    शून्य celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() स्थिरांक { परत संदेश_En().enabled(V3ErrorCode::I_COVERAGE); }
    शून्य coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() स्थिरांक { परत संदेश_En().enabled(V3ErrorCode::I_ERRORING); }
    शून्य erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() स्थिरांक { परत संदेश_En().enabled(V3ErrorCode::I_TRACING); }
    शून्य tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() स्थिरांक { परत संदेश_En().enabled(V3ErrorCode::I_TIMING); }
    शून्य timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    स्थिर string command_ओळ_Filename() VL_MT_SAFE { परत "<command-line>"; }
    स्थिर string builtInFilename() VL_MT_SAFE { परत "<built-in>"; }
    स्थिर शून्य globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_ओळ().warnOff(code, turnOff);
    }
    स्थिर string globalWarnOffParse(स्थिरांक string& msgs, bool turnOff) {
        परत default_File_ओळ().warnOffParse(msgs, turnOff);
    }
    स्थिर शून्य file_Name_अंक_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_अंक_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    शून्य modifyStateInherit(स्थिरांक File_ओळ* fromp) {
        m_संदेश_En_Idx = singleton().संदेश_En_And(m_संदेश_En_Idx, fromp->m_संदेश_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    शून्य modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    शून्य v3errorEnd(std::ostringstream& str, स्थिरांक string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        प्रतिज्ञा(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        परत warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() स्थिरांक { परत warnContext(); }
    bool चालक==(स्थिरांक File_ओळ& rhs) स्थिरांक {
        परत (m_token_अंक == rhs.m_token_अंक && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_संदेश_En_Idx == rhs.m_संदेश_En_Idx);
    }
    bool equal_First_ओळ_Col(स्थिरांक File_ओळ& rhs) स्थिरांक {
        परत (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    पूर्णांक operatorCompare(स्थिरांक File_ओळ& rhs) स्थिरांक {
        जर (m_filenameno != rhs.m_filenameno) परत (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        जर (m_firstLineno != rhs.m_firstLineno)
            परत (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        जर (m_firstColumn != rhs.m_firstColumn)
            परत (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        जर (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            परत (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        जर (m_lastColumn != rhs.m_lastColumn) परत (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        स्थिरांक संदेश_En_Bit_Set& lhs_संदेश_En = संदेश_En();
        स्थिरांक संदेश_En_Bit_Set& rhs_संदेश_En = rhs.संदेश_En();
        साठी (माप_प्रकार i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            स्थिरांक V3ErrorCode code = static_cast<V3ErrorCode>(i);
            जर (lhs_संदेश_En.enabled(code) != rhs_संदेश_En.enabled(code))
                परत rhs_संदेश_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        जर (m_token_अंक != rhs.m_token_अंक) परत (m_token_अंक < rhs.m_token_अंक) ? -1 : 1;
        परत 0;  // (*this) and rhs are equivalent
    }

खाजगी:
    string warnContext() स्थिरांक;
    string warnContextParent() स्थिरांक VL_REQUIRES(V3Error::s().m_mutex);
    स्थिरांक संदेश_En_Bit_Set& संदेश_En() स्थिरांक VL_MT_SAFE { परत singleton().संदेश_En(m_संदेश_En_Idx); }
};
std::ostream& चालक<<(std::ostream& os, File_ओळ* fileline);

#endif  // Guard
