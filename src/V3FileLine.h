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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Hash.h"
#അന്തര്ഭാവ "V3LangCode.h"
#അന്തര്ഭാവ "V3Mutex.h"

#അന്തര്ഭാവ <atomic>
#അന്തര്ഭാവ <deque>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <sstream>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <vector>

// ######################################################################

class File_വരി;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_വരി_Singleton final {
    friend class File_വരി;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using സന്ദേശം_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class സന്ദേശം_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    പൊതു:
        എണ്ണൽ class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        സന്ദേശം_En_Bit_Set() = സ്ഥിരസ്ഥിതി;
        // Create intersection set
        സന്ദേശം_En_Bit_Set(സ്ഥിരം സന്ദേശം_En_Bit_Set& a, സ്ഥിരം സന്ദേശം_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        ഘടന Hash final {
            size_t operator()(സ്ഥിരം സന്ദേശം_En_Bit_Set& item) സ്ഥിരം {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                തിരികെ hash.മൂല്യം();
            }
        };

        ഘടന Equal final {
            bool operator()(സ്ഥിരം സന്ദേശം_En_Bit_Set& a, സ്ഥിരം സന്ദേശം_En_Bit_Set& b) സ്ഥിരം { തിരികെ a == b; }
        };

        bool operator==(സ്ഥിരം സന്ദേശം_En_Bit_Set& other) സ്ഥിരം {
            തിരികെ m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        സ്ഥിരം VErrorBitSet& getAll(Subset subset) സ്ഥിരം {
            തിരികെ subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) സ്ഥിരം {
            തിരികെ subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        ശൂന്യം setAll(Subset subset, സ്ഥിരം VErrorBitSet& bitset) {
            എങ്കിൽ (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } അല്ലെങ്കിൽ {
                m_ctrlEn = bitset;
            }
        }
        ശൂന്യം set(Subset subset, V3ErrorCode code, bool മൂല്യം) {
            എങ്കിൽ (subset == Subset::CODE) {
                m_codeEn.set(code, മൂല്യം);
            } അല്ലെങ്കിൽ {
                m_ctrlEn.set(code, മൂല്യം);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) സ്ഥിരം { തിരികെ m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<സ്ഥിരം std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<സന്ദേശം_En_Bit_Set, സന്ദേശം_En_Set_Idx_t, സന്ദേശം_En_Bit_Set::Hash, സന്ദേശം_En_Bit_Set::Equal>
        m_interned_സന്ദേശം_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<സന്ദേശം_En_Bit_Set> m_interned_സന്ദേശം_Ens;

    // CONSTRUCTORS
    File_വരി_Singleton() = സ്ഥിരസ്ഥിതി;
    ~File_വരി_Singleton() = സ്ഥിരസ്ഥിതി;

    fileNameIdx_t name_To_സംഖ്യ(സ്ഥിരം string& filename);
    സ്ഥിരം std::string& സംഖ്യ_To_Name(fileNameIdx_t filenameno) സ്ഥിരം VL_MT_SAFE {
        തിരികെ m_names[filenameno];
    }
    V3LangCode സംഖ്യ_To_Lang(fileNameIdx_t filenameno) സ്ഥിരം { തിരികെ m_languages[filenameno]; }
    ശൂന്യം സംഖ്യ_To_Lang(fileNameIdx_t filenameno, സ്ഥിരം V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    ശൂന്യം clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    ശൂന്യം file_Name_അക്കം_Map_Dump_Json(std::ostream& os);
    സ്ഥിതം string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    സന്ദേശം_En_Set_Idx_t add_സന്ദേശം_En_Bit_Set(സ്ഥിരം സന്ദേശം_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    സന്ദേശം_En_Set_Idx_t default_സന്ദേശം_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    സന്ദേശം_En_Set_Idx_t സന്ദേശം_En_Set_Bit(സന്ദേശം_En_Set_Idx_t setIdx, സന്ദേശം_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool മൂല്യം);
    // Bulk-set all control codes to given bitset
    സന്ദേശം_En_Set_Idx_t സന്ദേശം_Set_Ctrl_Bit_Set(സന്ദേശം_En_Set_Idx_t setIdx, സ്ഥിരം VErrorBitSet& bitset);
    // Return index to intersection set
    സന്ദേശം_En_Set_Idx_t സന്ദേശം_En_And(സന്ദേശം_En_Set_Idx_t lhsIdx, സന്ദേശം_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    സ്ഥിരം സന്ദേശം_En_Bit_Set& സന്ദേശം_En(സന്ദേശം_En_Set_Idx_t idx) സ്ഥിരം VL_MT_SAFE {
        തിരികെ m_interned_സന്ദേശം_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_വരി;
    // MEMBERS
    പൂർണ്ണസംഖ്യ m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_എണ്ണം{0};
    std::deque<string> m_വരികൾ;  // Source text lines
    VFileContent() {
        സ്ഥിതം പൂർണ്ണസംഖ്യ s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = സ്ഥിരസ്ഥിതി;
    // METHODS
    ശൂന്യം refInc() { ++m_ref_എണ്ണം; }
    ശൂന്യം refDec() {
        എങ്കിൽ (!--m_ref_എണ്ണം) delete this;
    }

പൊതു:
    ശൂന്യം തള്ളുക_Text(സ്ഥിരം string& text);  // Add arbitrary text (need not be line-by-line)
    string get_വരി(പൂർണ്ണസംഖ്യ lineno) സ്ഥിരം VL_MT_SAFE;
    string ascii() സ്ഥിരം { തിരികെ "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_വരി final {

    // CONSTANTS
    സ്ഥിതം constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_വരി_Singleton::fileNameIdx_t;
    using സന്ദേശം_En_Set_Idx_t = File_വരി_Singleton::സന്ദേശം_En_Set_Idx_t;
    using സന്ദേശം_En_Bit_Set = File_വരി_Singleton::സന്ദേശം_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    സന്ദേശം_En_Set_Idx_t m_സന്ദേശം_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_അക്കം = 0;  // Token number in processing order
    പൂർണ്ണസംഖ്യ m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_വരി* m_parent = nullptr;  // Parent line that included this line

സംരക്ഷിത:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_വരി_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

സ്വകാര്യ:
    // CONSTRUCTORS
    സ്ഥിതം File_വരി_Singleton& singleton() VL_MT_SAFE {
        സ്ഥിതം File_വരി_Singleton s;
        തിരികെ s;
    }
    സ്ഥിതം File_വരി& default_File_വരി() VL_MT_SAFE {
        സ്ഥിതം File_വരി s;
        തിരികെ s;
    }

    File_വരി()  // Only used for defaultFileLine above
        : m_സന്ദേശം_En_Idx{singleton().default_സന്ദേശം_En_Index()}
        , m_filenameno{singleton().name_To_സംഖ്യ(File_വരി::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

പൊതു:
    explicit File_വരി(സ്ഥിരം string& filename)
        : m_സന്ദേശം_En_Idx{default_File_വരി().m_സന്ദേശം_En_Idx}
        , m_filenameno{singleton().name_To_സംഖ്യ(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_വരി(സ്ഥിരം File_വരി& from)
        : m_സന്ദേശം_En_Idx{from.m_സന്ദേശം_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_അക്കം{from.m_token_അക്കം}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        എങ്കിൽ (m_contentp) m_contentp->refInc();
    }
    explicit File_വരി(File_വരി* fromp)
        : m_സന്ദേശം_En_Idx{fromp->m_സന്ദേശം_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_അക്കം{fromp->m_token_അക്കം}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        എങ്കിൽ (m_contentp) m_contentp->refInc();
    }
    ശൂന്യം applyIgnores();
    File_വരി* copy_Or_Same_File_വരി();
    File_വരി* copy_Or_Same_File_വരി_Applied();
    സ്ഥിതം ശൂന്യം deleteAllRemaining();
    സ്ഥിതം ശൂന്യം stats();
    ~File_വരി();
#ifdef VL_LEAK_CHECKS
    സ്ഥിതം ശൂന്യം* operator new(size_t size);
    സ്ഥിതം ശൂന്യം operator delete(ശൂന്യം* obj, size_t size);
#endif
    // METHODS
    ശൂന്യം newContent();
    ശൂന്യം contentLineno(പൂർണ്ണസംഖ്യ അക്കം) {
        lineno(അക്കം);
        m_contentLineno = static_cast<unsigned>(അക്കം);
    }
    ശൂന്യം contentLinenoFrom(സ്ഥിരം File_വരി* fromp) { m_contentLineno = fromp->m_contentLineno; }
    ശൂന്യം lineno(പൂർണ്ണസംഖ്യ അക്കം) {
        m_firstLineno = അക്കം;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    ശൂന്യം column(പൂർണ്ണസംഖ്യ firstColumn, പൂർണ്ണസംഖ്യ lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    ശൂന്യം language(V3LangCode lang) സ്ഥിരം { singleton().സംഖ്യ_To_Lang(filenameno(), lang); }
    ശൂന്യം filename(സ്ഥിരം string& name) { m_filenameno = singleton().name_To_സംഖ്യ(name); }
    ശൂന്യം parent(File_വരി* fileline) { m_parent = fileline; }
    ശൂന്യം വരി_Directive(സ്ഥിരം അക്ഷരം* textp, പൂർണ്ണസംഖ്യ& enterExitRef);
    ശൂന്യം വരി_Directive_Parse(സ്ഥിരം അക്ഷരം* textp, string& filenameRef, പൂർണ്ണസംഖ്യ& linenoRef,
                            പൂർണ്ണസംഖ്യ& enterExitRef);
    ശൂന്യം linenoInc() {
        എങ്കിൽ (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } അല്ലെങ്കിൽ {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    ശൂന്യം startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    ശൂന്യം forwardToken(സ്ഥിരം അക്ഷരം* textp, size_t size, bool track_വരികൾ = true);
    uint32_t token_അക്കം() സ്ഥിരം VL_MT_SAFE { തിരികെ m_token_അക്കം; }
    പൂർണ്ണസംഖ്യ firstLineno() സ്ഥിരം VL_MT_SAFE { തിരികെ m_firstLineno; }
    പൂർണ്ണസംഖ്യ firstColumn() സ്ഥിരം VL_MT_SAFE { തിരികെ static_cast<പൂർണ്ണസംഖ്യ>(m_firstColumn); }
    പൂർണ്ണസംഖ്യ lastLineno() സ്ഥിരം VL_MT_SAFE { തിരികെ m_firstLineno + m_lastLinenoAdder; }
    പൂർണ്ണസംഖ്യ lastColumn() സ്ഥിരം VL_MT_SAFE { തിരികെ static_cast<പൂർണ്ണസംഖ്യ>(m_lastColumn); }
    VFileContent* contentp() സ്ഥിരം { തിരികെ m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    പൂർണ്ണസംഖ്യ lineno() സ്ഥിരം VL_MT_SAFE { തിരികെ lastLineno(); }
    string source() സ്ഥിരം VL_MT_SAFE;
    string sourcePrefix(പൂർണ്ണസംഖ്യ toColumn) സ്ഥിരം VL_MT_SAFE;
    string prettySource() സ്ഥിരം VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_വരി* parent() സ്ഥിരം VL_MT_SAFE { തിരികെ m_parent; }
    V3LangCode language() സ്ഥിരം { തിരികെ singleton().സംഖ്യ_To_Lang(filenameno()); }
    string ascii() സ്ഥിരം VL_MT_SAFE;
    string ascii_വരി_Col() സ്ഥിരം;
    പൂർണ്ണസംഖ്യ filenameno() സ്ഥിരം VL_MT_SAFE { തിരികെ m_filenameno; }
    സ്ഥിരം std::string& filename() സ്ഥിരം VL_MT_SAFE {
        തിരികെ singleton().സംഖ്യ_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() സ്ഥിരം VL_MT_SAFE { തിരികെ VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() സ്ഥിരം VL_MT_SAFE {
        തിരികെ (filename() == command_വരി_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() സ്ഥിരം VL_MT_SAFE {
        തിരികെ File_വരി_Singleton::filenameLetters(filenameno());
    }
    string filebasename() സ്ഥിരം VL_MT_SAFE;
    string filebasenameNoExt() സ്ഥിരം;
    string firstColumnLetters() സ്ഥിരം VL_MT_SAFE;
    string profileFuncname() സ്ഥിരം;
    string വരി_Directive_Strg(പൂർണ്ണസംഖ്യ enterExit) സ്ഥിരം;

    // Turn on/off warning messages on this line.
സ്വകാര്യ:
    ശൂന്യം warnSet(സന്ദേശം_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_സന്ദേശം_En_Idx = singleton().സന്ദേശം_En_Set_Bit(m_സന്ദേശം_En_Idx, subset, code, flag);
    }

പൊതു:
    ശൂന്യം warnOn(V3ErrorCode code, bool flag) { warnSet(സന്ദേശം_En_Bit_Set::Subset::CODE, code, flag); }
    ശൂന്യം warnSetCtrlBitSet(സ്ഥിരം VErrorBitSet& bitset) {
        m_സന്ദേശം_En_Idx = singleton().സന്ദേശം_Set_Ctrl_Bit_Set(m_സന്ദേശം_En_Idx, bitset);
    }
    ശൂന്യം warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(സ്ഥിരം string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) സ്ഥിരം VL_MT_SAFE;
    ശൂന്യം warnLintOff(bool turnOff);
    ശൂന്യം warnStyleOff(bool turnOff);
    ശൂന്യം warnStateFrom(സ്ഥിരം File_വരി& from) { m_സന്ദേശം_En_Idx = from.m_സന്ദേശം_En_Idx; }
    ശൂന്യം warnResetDefault() { warnStateFrom(default_File_വരി()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() സ്ഥിരം { തിരികെ സന്ദേശം_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    ശൂന്യം celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() സ്ഥിരം { തിരികെ സന്ദേശം_En().enabled(V3ErrorCode::I_COVERAGE); }
    ശൂന്യം coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() സ്ഥിരം { തിരികെ സന്ദേശം_En().enabled(V3ErrorCode::I_ERRORING); }
    ശൂന്യം erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() സ്ഥിരം { തിരികെ സന്ദേശം_En().enabled(V3ErrorCode::I_TRACING); }
    ശൂന്യം tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() സ്ഥിരം { തിരികെ സന്ദേശം_En().enabled(V3ErrorCode::I_TIMING); }
    ശൂന്യം timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    സ്ഥിതം string command_വരി_Filename() VL_MT_SAFE { തിരികെ "<command-line>"; }
    സ്ഥിതം string builtInFilename() VL_MT_SAFE { തിരികെ "<built-in>"; }
    സ്ഥിതം ശൂന്യം globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_വരി().warnOff(code, turnOff);
    }
    സ്ഥിതം string globalWarnOffParse(സ്ഥിരം string& msgs, bool turnOff) {
        തിരികെ default_File_വരി().warnOffParse(msgs, turnOff);
    }
    സ്ഥിതം ശൂന്യം file_Name_അക്കം_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_അക്കം_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    ശൂന്യം modifyStateInherit(സ്ഥിരം File_വരി* fromp) {
        m_സന്ദേശം_En_Idx = singleton().സന്ദേശം_En_And(m_സന്ദേശം_En_Idx, fromp->m_സന്ദേശം_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    ശൂന്യം modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    ശൂന്യം v3errorEnd(std::ostringstream& str, സ്ഥിരം string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    ശൂന്യം v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        പ്രതിജ്ഞാ(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        തിരികെ warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() സ്ഥിരം { തിരികെ warnContext(); }
    bool operator==(സ്ഥിരം File_വരി& rhs) സ്ഥിരം {
        തിരികെ (m_token_അക്കം == rhs.m_token_അക്കം && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_സന്ദേശം_En_Idx == rhs.m_സന്ദേശം_En_Idx);
    }
    bool equal_First_വരി_Col(സ്ഥിരം File_വരി& rhs) സ്ഥിരം {
        തിരികെ (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    പൂർണ്ണസംഖ്യ operatorCompare(സ്ഥിരം File_വരി& rhs) സ്ഥിരം {
        എങ്കിൽ (m_filenameno != rhs.m_filenameno) തിരികെ (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        എങ്കിൽ (m_firstLineno != rhs.m_firstLineno)
            തിരികെ (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        എങ്കിൽ (m_firstColumn != rhs.m_firstColumn)
            തിരികെ (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        എങ്കിൽ (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            തിരികെ (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        എങ്കിൽ (m_lastColumn != rhs.m_lastColumn) തിരികെ (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        സ്ഥിരം സന്ദേശം_En_Bit_Set& lhs_സന്ദേശം_En = സന്ദേശം_En();
        സ്ഥിരം സന്ദേശം_En_Bit_Set& rhs_സന്ദേശം_En = rhs.സന്ദേശം_En();
        വേണ്ടി (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            സ്ഥിരം V3ErrorCode code = static_cast<V3ErrorCode>(i);
            എങ്കിൽ (lhs_സന്ദേശം_En.enabled(code) != rhs_സന്ദേശം_En.enabled(code))
                തിരികെ rhs_സന്ദേശം_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        എങ്കിൽ (m_token_അക്കം != rhs.m_token_അക്കം) തിരികെ (m_token_അക്കം < rhs.m_token_അക്കം) ? -1 : 1;
        തിരികെ 0;  // (*this) and rhs are equivalent
    }

സ്വകാര്യ:
    string warnContext() സ്ഥിരം;
    string warnContextParent() സ്ഥിരം VL_REQUIRES(V3Error::s().m_mutex);
    സ്ഥിരം സന്ദേശം_En_Bit_Set& സന്ദേശം_En() സ്ഥിരം VL_MT_SAFE { തിരികെ singleton().സന്ദേശം_En(m_സന്ദേശം_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_വരി* fileline);

#endif  // Guard
