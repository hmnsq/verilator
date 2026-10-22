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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Hash.h"
#ಅನ್ತರ್ಭಾವ "V3LangCode.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"

#ಅನ್ತರ್ಭಾವ <atomic>
#ಅನ್ತರ್ಭಾವ <deque>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <sstream>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <vector>

// ######################################################################

class File_ಸಾಲು;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_ಸಾಲು_Singleton final {
    friend class File_ಸಾಲು;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using ಸಂದೇಶ_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class ಸಂದೇಶ_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    ಸಾರ್ವಜನಿಕ:
        ಗಣನಾ class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        ಸಂದೇಶ_En_Bit_Set() = ಸ್ವಭಾವ;
        // Create intersection set
        ಸಂದೇಶ_En_Bit_Set(ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& a, ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        ಸಂರಚನಾ Hash final {
            size_t operator()(ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& item) ಧ್ರುವ {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
            }
        };

        ಸಂರಚನಾ Equal final {
            bool operator()(ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& a, ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& b) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ a == b; }
        };

        bool operator==(ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& other) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        ಧ್ರುವ VErrorBitSet& getAll(Subset subset) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        ಶೂನ್ಯ setAll(Subset subset, ಧ್ರುವ VErrorBitSet& bitset) {
            ಯದಿ (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } ಅನ್ಯಥಾ {
                m_ctrlEn = bitset;
            }
        }
        ಶೂನ್ಯ set(Subset subset, V3ErrorCode code, bool ಮೌಲ್ಯ) {
            ಯದಿ (subset == Subset::CODE) {
                m_codeEn.set(code, ಮೌಲ್ಯ);
            } ಅನ್ಯಥಾ {
                m_ctrlEn.set(code, ಮೌಲ್ಯ);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ಧ್ರುವ std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<ಸಂದೇಶ_En_Bit_Set, ಸಂದೇಶ_En_Set_Idx_t, ಸಂದೇಶ_En_Bit_Set::Hash, ಸಂದೇಶ_En_Bit_Set::Equal>
        m_interned_ಸಂದೇಶ_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<ಸಂದೇಶ_En_Bit_Set> m_interned_ಸಂದೇಶ_Ens;

    // CONSTRUCTORS
    File_ಸಾಲು_Singleton() = ಸ್ವಭಾವ;
    ~File_ಸಾಲು_Singleton() = ಸ್ವಭಾವ;

    fileNameIdx_t name_To_ಸಂಖ್ಯೆ(ಧ್ರುವ string& filename);
    ಧ್ರುವ std::string& ಸಂಖ್ಯೆ_To_Name(fileNameIdx_t filenameno) ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_names[filenameno];
    }
    V3LangCode ಸಂಖ್ಯೆ_To_Lang(fileNameIdx_t filenameno) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_languages[filenameno]; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_To_Lang(fileNameIdx_t filenameno, ಧ್ರುವ V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    ಶೂನ್ಯ clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    ಶೂನ್ಯ file_Name_ಅಂಕೆ_Map_Dump_Json(std::ostream& os);
    ಸ್ಥಿರ string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    ಸಂದೇಶ_En_Set_Idx_t add_ಸಂದೇಶ_En_Bit_Set(ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    ಸಂದೇಶ_En_Set_Idx_t default_ಸಂದೇಶ_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    ಸಂದೇಶ_En_Set_Idx_t ಸಂದೇಶ_En_Set_Bit(ಸಂದೇಶ_En_Set_Idx_t setIdx, ಸಂದೇಶ_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool ಮೌಲ್ಯ);
    // Bulk-set all control codes to given bitset
    ಸಂದೇಶ_En_Set_Idx_t ಸಂದೇಶ_Set_Ctrl_Bit_Set(ಸಂದೇಶ_En_Set_Idx_t setIdx, ಧ್ರುವ VErrorBitSet& bitset);
    // Return index to intersection set
    ಸಂದೇಶ_En_Set_Idx_t ಸಂದೇಶ_En_And(ಸಂದೇಶ_En_Set_Idx_t lhsIdx, ಸಂದೇಶ_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& ಸಂದೇಶ_En(ಸಂದೇಶ_En_Set_Idx_t idx) ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ m_interned_ಸಂದೇಶ_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_ಸಾಲು;
    // MEMBERS
    ಪೂರ್ಣಾಙ್ಕ m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_ಎಣಿಕೆ{0};
    std::deque<string> m_ಸಾಲುಗಳು;  // Source text lines
    VFileContent() {
        ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = ಸ್ವಭಾವ;
    // METHODS
    ಶೂನ್ಯ refInc() { ++m_ref_ಎಣಿಕೆ; }
    ಶೂನ್ಯ refDec() {
        ಯದಿ (!--m_ref_ಎಣಿಕೆ) delete this;
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ ತಳ್ಳು_Text(ಧ್ರುವ string& text);  // Add arbitrary text (need not be line-by-line)
    string get_ಸಾಲು(ಪೂರ್ಣಾಙ್ಕ lineno) ಧ್ರುವ VL_MT_SAFE;
    string ascii() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_ಸಾಲು final {

    // CONSTANTS
    ಸ್ಥಿರ constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_ಸಾಲು_Singleton::fileNameIdx_t;
    using ಸಂದೇಶ_En_Set_Idx_t = File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_Set_Idx_t;
    using ಸಂದೇಶ_En_Bit_Set = File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    ಸಂದೇಶ_En_Set_Idx_t m_ಸಂದೇಶ_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_ಅಂಕೆ = 0;  // Token number in processing order
    ಪೂರ್ಣಾಙ್ಕ m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_ಸಾಲು* m_parent = nullptr;  // Parent line that included this line

ರಕ್ಷಿತ:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_ಸಾಲು_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

ಗುಪ್ತ:
    // CONSTRUCTORS
    ಸ್ಥಿರ File_ಸಾಲು_Singleton& singleton() VL_MT_SAFE {
        ಸ್ಥಿರ File_ಸಾಲು_Singleton s;
        ಪ್ರತ್ಯಾಗಮ s;
    }
    ಸ್ಥಿರ File_ಸಾಲು& default_File_ಸಾಲು() VL_MT_SAFE {
        ಸ್ಥಿರ File_ಸಾಲು s;
        ಪ್ರತ್ಯಾಗಮ s;
    }

    File_ಸಾಲು()  // Only used for defaultFileLine above
        : m_ಸಂದೇಶ_En_Idx{singleton().default_ಸಂದೇಶ_En_Index()}
        , m_filenameno{singleton().name_To_ಸಂಖ್ಯೆ(File_ಸಾಲು::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

ಸಾರ್ವಜನಿಕ:
    explicit File_ಸಾಲು(ಧ್ರುವ string& filename)
        : m_ಸಂದೇಶ_En_Idx{default_File_ಸಾಲು().m_ಸಂದೇಶ_En_Idx}
        , m_filenameno{singleton().name_To_ಸಂಖ್ಯೆ(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_ಸಾಲು(ಧ್ರುವ File_ಸಾಲು& from)
        : m_ಸಂದೇಶ_En_Idx{from.m_ಸಂದೇಶ_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_ಅಂಕೆ{from.m_token_ಅಂಕೆ}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        ಯದಿ (m_contentp) m_contentp->refInc();
    }
    explicit File_ಸಾಲು(File_ಸಾಲು* fromp)
        : m_ಸಂದೇಶ_En_Idx{fromp->m_ಸಂದೇಶ_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_ಅಂಕೆ{fromp->m_token_ಅಂಕೆ}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        ಯದಿ (m_contentp) m_contentp->refInc();
    }
    ಶೂನ್ಯ applyIgnores();
    File_ಸಾಲು* copy_Or_Same_File_ಸಾಲು();
    File_ಸಾಲು* copy_Or_Same_File_ಸಾಲು_Applied();
    ಸ್ಥಿರ ಶೂನ್ಯ deleteAllRemaining();
    ಸ್ಥಿರ ಶೂನ್ಯ stats();
    ~File_ಸಾಲು();
#ifdef VL_LEAK_CHECKS
    ಸ್ಥಿರ ಶೂನ್ಯ* operator new(size_t size);
    ಸ್ಥಿರ ಶೂನ್ಯ operator delete(ಶೂನ್ಯ* obj, size_t size);
#endif
    // METHODS
    ಶೂನ್ಯ newContent();
    ಶೂನ್ಯ contentLineno(ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ) {
        lineno(ಅಂಕೆ);
        m_contentLineno = static_cast<unsigned>(ಅಂಕೆ);
    }
    ಶೂನ್ಯ contentLinenoFrom(ಧ್ರುವ File_ಸಾಲು* fromp) { m_contentLineno = fromp->m_contentLineno; }
    ಶೂನ್ಯ lineno(ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ) {
        m_firstLineno = ಅಂಕೆ;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    ಶೂನ್ಯ column(ಪೂರ್ಣಾಙ್ಕ firstColumn, ಪೂರ್ಣಾಙ್ಕ lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    ಶೂನ್ಯ language(V3LangCode lang) ಧ್ರುವ { singleton().ಸಂಖ್ಯೆ_To_Lang(filenameno(), lang); }
    ಶೂನ್ಯ filename(ಧ್ರುವ string& name) { m_filenameno = singleton().name_To_ಸಂಖ್ಯೆ(name); }
    ಶೂನ್ಯ parent(File_ಸಾಲು* fileline) { m_parent = fileline; }
    ಶೂನ್ಯ ಸಾಲು_Directive(ಧ್ರುವ ವರ್ಣ* textp, ಪೂರ್ಣಾಙ್ಕ& enterExitRef);
    ಶೂನ್ಯ ಸಾಲು_Directive_Parse(ಧ್ರುವ ವರ್ಣ* textp, string& filenameRef, ಪೂರ್ಣಾಙ್ಕ& linenoRef,
                            ಪೂರ್ಣಾಙ್ಕ& enterExitRef);
    ಶೂನ್ಯ linenoInc() {
        ಯದಿ (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } ಅನ್ಯಥಾ {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    ಶೂನ್ಯ startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    ಶೂನ್ಯ forwardToken(ಧ್ರುವ ವರ್ಣ* textp, size_t size, bool track_ಸಾಲುಗಳು = true);
    uint32_t token_ಅಂಕೆ() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_token_ಅಂಕೆ; }
    ಪೂರ್ಣಾಙ್ಕ firstLineno() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_firstLineno; }
    ಪೂರ್ಣಾಙ್ಕ firstColumn() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_firstColumn); }
    ಪೂರ್ಣಾಙ್ಕ lastLineno() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_firstLineno + m_lastLinenoAdder; }
    ಪೂರ್ಣಾಙ್ಕ lastColumn() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_lastColumn); }
    VFileContent* contentp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    ಪೂರ್ಣಾಙ್ಕ lineno() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ lastLineno(); }
    string source() ಧ್ರುವ VL_MT_SAFE;
    string sourcePrefix(ಪೂರ್ಣಾಙ್ಕ toColumn) ಧ್ರುವ VL_MT_SAFE;
    string prettySource() ಧ್ರುವ VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_ಸಾಲು* parent() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_parent; }
    V3LangCode language() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ singleton().ಸಂಖ್ಯೆ_To_Lang(filenameno()); }
    string ascii() ಧ್ರುವ VL_MT_SAFE;
    string ascii_ಸಾಲು_Col() ಧ್ರುವ;
    ಪೂರ್ಣಾಙ್ಕ filenameno() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_filenameno; }
    ಧ್ರುವ std::string& filename() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ singleton().ಸಂಖ್ಯೆ_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ (filename() == command_ಸಾಲು_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() ಧ್ರುವ VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ File_ಸಾಲು_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ಧ್ರುವ VL_MT_SAFE;
    string filebasenameNoExt() ಧ್ರುವ;
    string firstColumnLetters() ಧ್ರುವ VL_MT_SAFE;
    string profileFuncname() ಧ್ರುವ;
    string ಸಾಲು_Directive_Strg(ಪೂರ್ಣಾಙ್ಕ enterExit) ಧ್ರುವ;

    // Turn on/off warning messages on this line.
ಗುಪ್ತ:
    ಶೂನ್ಯ warnSet(ಸಂದೇಶ_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_ಸಂದೇಶ_En_Idx = singleton().ಸಂದೇಶ_En_Set_Bit(m_ಸಂದೇಶ_En_Idx, subset, code, flag);
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ warnOn(V3ErrorCode code, bool flag) { warnSet(ಸಂದೇಶ_En_Bit_Set::Subset::CODE, code, flag); }
    ಶೂನ್ಯ warnSetCtrlBitSet(ಧ್ರುವ VErrorBitSet& bitset) {
        m_ಸಂದೇಶ_En_Idx = singleton().ಸಂದೇಶ_Set_Ctrl_Bit_Set(m_ಸಂದೇಶ_En_Idx, bitset);
    }
    ಶೂನ್ಯ warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(ಧ್ರುವ string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) ಧ್ರುವ VL_MT_SAFE;
    ಶೂನ್ಯ warnLintOff(bool turnOff);
    ಶೂನ್ಯ warnStyleOff(bool turnOff);
    ಶೂನ್ಯ warnStateFrom(ಧ್ರುವ File_ಸಾಲು& from) { m_ಸಂದೇಶ_En_Idx = from.m_ಸಂದೇಶ_En_Idx; }
    ಶೂನ್ಯ warnResetDefault() { warnStateFrom(default_File_ಸಾಲು()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಸಂದೇಶ_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    ಶೂನ್ಯ celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಸಂದೇಶ_En().enabled(V3ErrorCode::I_COVERAGE); }
    ಶೂನ್ಯ coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಸಂದೇಶ_En().enabled(V3ErrorCode::I_ERRORING); }
    ಶೂನ್ಯ erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಸಂದೇಶ_En().enabled(V3ErrorCode::I_TRACING); }
    ಶೂನ್ಯ tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ ಸಂದೇಶ_En().enabled(V3ErrorCode::I_TIMING); }
    ಶೂನ್ಯ timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ಸ್ಥಿರ string command_ಸಾಲು_Filename() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ "<command-line>"; }
    ಸ್ಥಿರ string builtInFilename() VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ "<built-in>"; }
    ಸ್ಥಿರ ಶೂನ್ಯ globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_ಸಾಲು().warnOff(code, turnOff);
    }
    ಸ್ಥಿರ string globalWarnOffParse(ಧ್ರುವ string& msgs, bool turnOff) {
        ಪ್ರತ್ಯಾಗಮ default_File_ಸಾಲು().warnOffParse(msgs, turnOff);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ file_Name_ಅಂಕೆ_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_ಅಂಕೆ_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    ಶೂನ್ಯ modifyStateInherit(ಧ್ರುವ File_ಸಾಲು* fromp) {
        m_ಸಂದೇಶ_En_Idx = singleton().ಸಂದೇಶ_En_And(m_ಸಂದೇಶ_En_Idx, fromp->m_ಸಂದೇಶ_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    ಶೂನ್ಯ modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    ಶೂನ್ಯ v3errorEnd(std::ostringstream& str, ಧ್ರುವ string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    ಶೂನ್ಯ v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() ಧ್ರುವ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ಧ್ರುವ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ಧ್ರುವ VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        ಪ್ರತ್ಯಾಗಮ warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ warnContext(); }
    bool operator==(ಧ್ರುವ File_ಸಾಲು& rhs) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (m_token_ಅಂಕೆ == rhs.m_token_ಅಂಕೆ && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_ಸಂದೇಶ_En_Idx == rhs.m_ಸಂದೇಶ_En_Idx);
    }
    bool equal_First_ಸಾಲು_Col(ಧ್ರುವ File_ಸಾಲು& rhs) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    ಪೂರ್ಣಾಙ್ಕ operatorCompare(ಧ್ರುವ File_ಸಾಲು& rhs) ಧ್ರುವ {
        ಯದಿ (m_filenameno != rhs.m_filenameno) ಪ್ರತ್ಯಾಗಮ (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        ಯದಿ (m_firstLineno != rhs.m_firstLineno)
            ಪ್ರತ್ಯಾಗಮ (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        ಯದಿ (m_firstColumn != rhs.m_firstColumn)
            ಪ್ರತ್ಯಾಗಮ (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        ಯದಿ (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            ಪ್ರತ್ಯಾಗಮ (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        ಯದಿ (m_lastColumn != rhs.m_lastColumn) ಪ್ರತ್ಯಾಗಮ (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& lhs_ಸಂದೇಶ_En = ಸಂದೇಶ_En();
        ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& rhs_ಸಂದೇಶ_En = rhs.ಸಂದೇಶ_En();
        ಪ್ರತಿ (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            ಧ್ರುವ V3ErrorCode code = static_cast<V3ErrorCode>(i);
            ಯದಿ (lhs_ಸಂದೇಶ_En.enabled(code) != rhs_ಸಂದೇಶ_En.enabled(code))
                ಪ್ರತ್ಯಾಗಮ rhs_ಸಂದೇಶ_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        ಯದಿ (m_token_ಅಂಕೆ != rhs.m_token_ಅಂಕೆ) ಪ್ರತ್ಯಾಗಮ (m_token_ಅಂಕೆ < rhs.m_token_ಅಂಕೆ) ? -1 : 1;
        ಪ್ರತ್ಯಾಗಮ 0;  // (*this) and rhs are equivalent
    }

ಗುಪ್ತ:
    string warnContext() ಧ್ರುವ;
    string warnContextParent() ಧ್ರುವ VL_REQUIRES(V3Error::s().m_mutex);
    ಧ್ರುವ ಸಂದೇಶ_En_Bit_Set& ಸಂದೇಶ_En() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ singleton().ಸಂದೇಶ_En(m_ಸಂದೇಶ_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_ಸಾಲು* fileline);

#endif  // Guard
