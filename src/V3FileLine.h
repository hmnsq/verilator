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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3FILELINE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3FILELINE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hash.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LangCode.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Mutex.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <atomic>
#ਅਨ੍ਤਰ੍ਭਾਵ <deque>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

// ######################################################################

class File_ਸਤਰ;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_ਸਤਰ_Singleton final {
    ਮਿੱਤਰ class File_ਸਤਰ;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ final {
        VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ m_ਕੋਡ_En;  // Enabled by code directives/metacomments
        VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ m_ctrlEn;  // Enabled by control file

    ਜਨਤਕ:
        ਗਿਣਤੀ class Subset : uint8_t {
            ਕੋਡ_2 = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ() = ਮੂਲ;
        // Create intersection set
        ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& a, ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& b)
            : m_ਕੋਡ_En{a.m_ਕੋਡ_En & b.m_ਕੋਡ_En}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        ਬਣਤਰ Hash final {
            ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& ਵਸਤੂ_3) ਅਚਲ {
                V3Hash hash{ਵਸਤੂ_3.m_ਕੋਡ_En.hash()};
                hash += ਵਸਤੂ_3.m_ctrlEn.hash();
                ਵਾਪਸ hash.ਮੁੱਲ();
            }
        };

        ਬਣਤਰ Equal final {
            bool ਸੰਚਾਲਕ()(ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& a, ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& b) ਅਚਲ { ਵਾਪਸ a == b; }
        };

        bool ਸੰਚਾਲਕ==(ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& other) ਅਚਲ {
            ਵਾਪਸ m_ਕੋਡ_En == other.m_ਕੋਡ_En && m_ctrlEn == other.m_ctrlEn;
        }

        ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& getAll(Subset subset) ਅਚਲ {
            ਵਾਪਸ subset == Subset::ਕੋਡ_2 ? m_ਕੋਡ_En : m_ctrlEn;
        }
        bool test(Subset subset, V3_Error_ਕੋਡ ਕੋਡ) ਅਚਲ {
            ਵਾਪਸ subset == Subset::ਕੋਡ_2 ? m_ਕੋਡ_En.test(ਕੋਡ) : m_ctrlEn.test(ਕੋਡ);
        }
        ਖਾਲੀ ਸੈੱਟ_All(Subset subset, ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& bitset) {
            ਜੇ (subset == Subset::ਕੋਡ_2) {  // LCOV_EXCL_BR_LINE
                m_ਕੋਡ_En = bitset;  // LCOV_EXCL_LINE
            } ਨਹੀਂਤਾਂ {
                m_ctrlEn = bitset;
            }
        }
        ਖਾਲੀ ਸੈੱਟ(Subset subset, V3_Error_ਕੋਡ ਕੋਡ, bool ਮੁੱਲ) {
            ਜੇ (subset == Subset::ਕੋਡ_2) {
                m_ਕੋਡ_En.ਸੈੱਟ(ਕੋਡ, ਮੁੱਲ);
            } ਨਹੀਂਤਾਂ {
                m_ctrlEn.ਸੈੱਟ(ਕੋਡ, ਮੁੱਲ);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3_Error_ਕੋਡ ਕੋਡ) ਅਚਲ { ਵਾਪਸ m_ਕੋਡ_En.test(ਕੋਡ) && m_ctrlEn.test(ਕੋਡ); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ਅਚਲ std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_ਨਾਂ;  // filename text for each filenameno
    std::deque<V3_Lang_ਕੋਡ> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ, ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t, ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Hash, ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Equal>
        m_interned_ਸੁਨੇਹਾ_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::ਸਦਿਸ਼<ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ> m_interned_ਸੁਨੇਹਾ_Ens;

    // CONSTRUCTORS
    File_ਸਤਰ_Singleton() = ਮੂਲ;
    ~File_ਸਤਰ_Singleton() = ਮੂਲ;

    fileNameIdx_t name_ਮੰਜ਼ਿਲ_ਸੰਖਿਆ(ਅਚਲ string& filename);
    ਅਚਲ std::string& ਸੰਖਿਆ_ਮੰਜ਼ਿਲ_Name(fileNameIdx_t filenameno) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_ਨਾਂ[filenameno];
    }
    V3_Lang_ਕੋਡ ਸੰਖਿਆ_ਮੰਜ਼ਿਲ_Lang(fileNameIdx_t filenameno) ਅਚਲ { ਵਾਪਸ m_languages[filenameno]; }
    ਖਾਲੀ ਸੰਖਿਆ_ਮੰਜ਼ਿਲ_Lang(fileNameIdx_t filenameno, ਅਚਲ V3_Lang_ਕੋਡ& l) {
        m_languages[filenameno] = l;
    }
    ਖਾਲੀ clear() {
        m_namemap.clear();
        m_ਨਾਂ.clear();
        m_languages.clear();
    }
    ਖਾਲੀ file_Name_ਅੰਕ_Map_Dump_Json(std::ostream& os);
    ਸਥਿਰ string filenameLetters(fileNameIdx_t fileno) VL_ਸ਼ੁੱਧ;

    // Add given bitset to the interned bitsets, return interned index
    ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t ਜੋੜੋ_ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ) VL_MT_ਸੁਰੱਖਿਅਤ_EXCLUDES(m_mutex);
    // Add index of default bitset
    ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t default_ਸੁਨੇਹਾ_En_ਸੂਚਕਾਂਕ() VL_MT_ਸੁਰੱਖਿਅਤ;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t ਸੁਨੇਹਾ_En_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t ਸੈੱਟ_Idx, ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Subset subset, V3_Error_ਕੋਡ ਕੋਡ,
                              bool ਮੁੱਲ);
    // Bulk-set all control codes to given bitset
    ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t ਸੁਨੇਹਾ_ਸੈੱਟ_Ctrl_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t ਸੈੱਟ_Idx, ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& bitset);
    // Return index to intersection set
    ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t ਸੁਨੇਹਾ_En_And(ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t lhsIdx, ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& ਸੁਨੇਹਾ_En(ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t idx) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ m_interned_ਸੁਨੇਹਾ_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    ਮਿੱਤਰ class File_ਸਤਰ;
    // MEMBERS
    ਪੂਰਨ m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<ਮਾਪ_t> m_ref_ਗਿਣਤੀ{0};
    std::deque<string> m_ਸਤਰਾਂ;  // Source text lines
    VFileContent() {
        ਸਥਿਰ ਪੂਰਨ s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = ਮੂਲ;
    // METHODS
    ਖਾਲੀ refInc() { ++m_ref_ਗਿਣਤੀ; }
    ਖਾਲੀ refDec() {
        ਜੇ (!--m_ref_ਗਿਣਤੀ) delete this;
    }

ਜਨਤਕ:
    ਖਾਲੀ ਧੱਕੋ_ਪਾਠ(ਅਚਲ string& ਪਾਠ);  // Add arbitrary text (need not be line-by-line)
    string get_ਸਤਰ(ਪੂਰਨ lineno) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    string ascii() ਅਚਲ { ਵਾਪਸ "ct" + cvt_ਮੰਜ਼ਿਲ_Str(m_id); }
};
std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_ਸਤਰ final {

    // CONSTANTS
    ਸਥਿਰ constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ SHOW_ਸਰੋਤ_MAX_ਲੰਮਾਈ = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_ਸਤਰ_Singleton::fileNameIdx_t;
    using ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t = File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t;
    using ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ = File_ਸਤਰ_Singleton::ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    ਸੁਨੇਹਾ_En_ਸੈੱਟ_Idx_t m_ਸੁਨੇਹਾ_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    ਬਿਨਾਨਿਸ਼ਾਨੀ m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_ਅੰਕ = 0;  // Token number in processing order
    ਪੂਰਨ m_ਪਹਿਲਾ_Lineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_ਪਹਿਲਾ_ਕਾਲਮ : 24;  // `line corrected token's first column number
    uint32_t m_ਆਖਰੀ_ਕਾਲਮ : 24;  // `line corrected token's last column number
    uint16_t m_ਆਖਰੀ_Lineno_Adder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_ਸਤਰ* m_ਮਾਪਾ = nullptr;  // Parent line that included this line

ਸੁਰੱਖਿਅਤ:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    ਮਿੱਤਰ class File_ਸਤਰ_Singleton;
    ਮਿੱਤਰ class V3ParseImp;
    ਮਿੱਤਰ class V3PreLex;
    ਮਿੱਤਰ class V3PreProcImp;
    ਮਿੱਤਰ class V3PreShellImp;

ਨਿਜੀ:
    // CONSTRUCTORS
    ਸਥਿਰ File_ਸਤਰ_Singleton& singleton() VL_MT_ਸੁਰੱਖਿਅਤ {
        ਸਥਿਰ File_ਸਤਰ_Singleton s;
        ਵਾਪਸ s;
    }
    ਸਥਿਰ File_ਸਤਰ& default_File_ਸਤਰ() VL_MT_ਸੁਰੱਖਿਅਤ {
        ਸਥਿਰ File_ਸਤਰ s;
        ਵਾਪਸ s;
    }

    File_ਸਤਰ()  // Only used for defaultFileLine above
        : m_ਸੁਨੇਹਾ_En_Idx{singleton().default_ਸੁਨੇਹਾ_En_ਸੂਚਕਾਂਕ()}
        , m_filenameno{singleton().name_ਮੰਜ਼ਿਲ_ਸੰਖਿਆ(File_ਸਤਰ::built_ਵਿੱਚ_Filename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_ਪਹਿਲਾ_ਕਾਲਮ{0}
        , m_ਆਖਰੀ_ਕਾਲਮ{0} {}

ਜਨਤਕ:
    explicit File_ਸਤਰ(ਅਚਲ string& filename)
        : m_ਸੁਨੇਹਾ_En_Idx{default_File_ਸਤਰ().m_ਸੁਨੇਹਾ_En_Idx}
        , m_filenameno{singleton().name_ਮੰਜ਼ਿਲ_ਸੰਖਿਆ(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_ਪਹਿਲਾ_ਕਾਲਮ{0}
        , m_ਆਖਰੀ_ਕਾਲਮ{0} {}
    explicit File_ਸਤਰ(ਅਚਲ File_ਸਤਰ& from)
        : m_ਸੁਨੇਹਾ_En_Idx{from.m_ਸੁਨੇਹਾ_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_ਅੰਕ{from.m_token_ਅੰਕ}
        , m_ਪਹਿਲਾ_Lineno{from.m_ਪਹਿਲਾ_Lineno}
        , m_ਪਹਿਲਾ_ਕਾਲਮ{from.m_ਪਹਿਲਾ_ਕਾਲਮ}
        , m_ਆਖਰੀ_ਕਾਲਮ{from.m_ਆਖਰੀ_ਕਾਲਮ}
        , m_ਆਖਰੀ_Lineno_Adder{from.m_ਆਖਰੀ_Lineno_Adder}
        , m_contentp{from.m_contentp}
        , m_ਮਾਪਾ{from.m_ਮਾਪਾ} {
        ਜੇ (m_contentp) m_contentp->refInc();
    }
    explicit File_ਸਤਰ(File_ਸਤਰ* fromp)
        : m_ਸੁਨੇਹਾ_En_Idx{fromp->m_ਸੁਨੇਹਾ_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_ਅੰਕ{fromp->m_token_ਅੰਕ}
        , m_ਪਹਿਲਾ_Lineno{fromp->m_ਪਹਿਲਾ_Lineno}
        , m_ਪਹਿਲਾ_ਕਾਲਮ{fromp->m_ਪਹਿਲਾ_ਕਾਲਮ}
        , m_ਆਖਰੀ_ਕਾਲਮ{fromp->m_ਆਖਰੀ_ਕਾਲਮ}
        , m_ਆਖਰੀ_Lineno_Adder{fromp->m_ਆਖਰੀ_Lineno_Adder}
        , m_contentp{fromp->m_contentp}
        , m_ਮਾਪਾ{fromp->m_ਮਾਪਾ} {
        ਜੇ (m_contentp) m_contentp->refInc();
    }
    ਖਾਲੀ applyIgnores();
    File_ਸਤਰ* copy_Or_Same_File_ਸਤਰ();
    File_ਸਤਰ* copy_Or_Same_File_ਸਤਰ_Applied();
    ਸਥਿਰ ਖਾਲੀ delete_All_ਬਚਿਆ();
    ਸਥਿਰ ਖਾਲੀ stats();
    ~File_ਸਤਰ();
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
    ਸਥਿਰ ਖਾਲੀ* ਸੰਚਾਲਕ new(ਮਾਪ_t ਮਾਪ);
    ਸਥਿਰ ਖਾਲੀ ਸੰਚਾਲਕ delete(ਖਾਲੀ* obj, ਮਾਪ_t ਮਾਪ);
#ਜੇ_ਅੰਤ
    // METHODS
    ਖਾਲੀ newContent();
    ਖਾਲੀ contentLineno(ਪੂਰਨ ਅੰਕ) {
        lineno(ਅੰਕ);
        m_contentLineno = static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ>(ਅੰਕ);
    }
    ਖਾਲੀ contentLinenoFrom(ਅਚਲ File_ਸਤਰ* fromp) { m_contentLineno = fromp->m_contentLineno; }
    ਖਾਲੀ lineno(ਪੂਰਨ ਅੰਕ) {
        m_ਪਹਿਲਾ_Lineno = ਅੰਕ;
        m_ਆਖਰੀ_Lineno_Adder = 0;
        m_ਪਹਿਲਾ_ਕਾਲਮ = 1;
        m_ਆਖਰੀ_ਕਾਲਮ = 1;
    }
    ਖਾਲੀ ਕਾਲਮ_2(ਪੂਰਨ ਪਹਿਲਾ_ਕਾਲਮ, ਪੂਰਨ ਆਖਰੀ_ਕਾਲਮ) {
        m_ਪਹਿਲਾ_ਕਾਲਮ = ਪਹਿਲਾ_ਕਾਲਮ;
        m_ਆਖਰੀ_ਕਾਲਮ = ਆਖਰੀ_ਕਾਲਮ;
    }
    ਖਾਲੀ language(V3_Lang_ਕੋਡ lang) ਅਚਲ { singleton().ਸੰਖਿਆ_ਮੰਜ਼ਿਲ_Lang(filenameno(), lang); }
    ਖਾਲੀ filename(ਅਚਲ string& name) { m_filenameno = singleton().name_ਮੰਜ਼ਿਲ_ਸੰਖਿਆ(name); }
    ਖਾਲੀ ਮਾਪਾ(File_ਸਤਰ* fileline) { m_ਮਾਪਾ = fileline; }
    ਖਾਲੀ ਸਤਰ_Directive(ਅਚਲ ਅੱਖਰ* textp, ਪੂਰਨ& enterExitRef);
    ਖਾਲੀ ਸਤਰ_Directive_Parse(ਅਚਲ ਅੱਖਰ* textp, string& filenameRef, ਪੂਰਨ& linenoRef,
                            ਪੂਰਨ& enterExitRef);
    ਖਾਲੀ linenoInc() {
        ਜੇ (m_ਆਖਰੀ_Lineno_Adder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_ਪਹਿਲਾ_Lineno += 65535 + 1;
            m_ਆਖਰੀ_Lineno_Adder = 0;
        } ਨਹੀਂਤਾਂ {
            ++m_ਆਖਰੀ_Lineno_Adder;
        }
        m_ਆਖਰੀ_ਕਾਲਮ = 1;
        ++m_contentLineno;
    }
    ਖਾਲੀ ਸ਼ੁਰੂ_Token() {
        m_ਪਹਿਲਾ_Lineno = ਆਖਰੀ_Lineno();
        m_ਪਹਿਲਾ_ਕਾਲਮ = m_ਆਖਰੀ_ਕਾਲਮ;
        m_ਆਖਰੀ_Lineno_Adder = 0;
    }
    // Advance last line/column based on given text
    ਖਾਲੀ forwardToken(ਅਚਲ ਅੱਖਰ* textp, ਮਾਪ_t ਮਾਪ, bool track_ਸਤਰਾਂ = true);
    uint32_t token_ਅੰਕ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_token_ਅੰਕ; }
    ਪੂਰਨ ਪਹਿਲਾ_Lineno() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਪਹਿਲਾ_Lineno; }
    ਪੂਰਨ ਪਹਿਲਾ_ਕਾਲਮ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ static_cast<ਪੂਰਨ>(m_ਪਹਿਲਾ_ਕਾਲਮ); }
    ਪੂਰਨ ਆਖਰੀ_Lineno() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਪਹਿਲਾ_Lineno + m_ਆਖਰੀ_Lineno_Adder; }
    ਪੂਰਨ ਆਖਰੀ_ਕਾਲਮ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ static_cast<ਪੂਰਨ>(m_ਆਖਰੀ_ਕਾਲਮ); }
    VFileContent* contentp() ਅਚਲ { ਵਾਪਸ m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    ਪੂਰਨ lineno() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ ਆਖਰੀ_Lineno(); }
    string ਸਰੋਤ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    string ਸਰੋਤ_ਅਗੇਤਰ(ਪੂਰਨ ਮੰਜ਼ਿਲ_ਕਾਲਮ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    string pretty_ਸਰੋਤ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;  // Source, w/stripped unprintables and newlines
    File_ਸਤਰ* ਮਾਪਾ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਮਾਪਾ; }
    V3_Lang_ਕੋਡ language() ਅਚਲ { ਵਾਪਸ singleton().ਸੰਖਿਆ_ਮੰਜ਼ਿਲ_Lang(filenameno()); }
    string ascii() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    string ascii_ਸਤਰ_ਖਾਨਾ() ਅਚਲ;
    ਪੂਰਨ filenameno() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_filenameno; }
    ਅਚਲ std::string& filename() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ singleton().ਸੰਖਿਆ_ਮੰਜ਼ਿਲ_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ (filename() == command_ਸਤਰ_Filename() || filename() == built_ਵਿੱਚ_Filename());
    }
    string filenameLetters() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ File_ਸਤਰ_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    string filebasenameNoExt() ਅਚਲ;
    string ਪਹਿਲਾ_ਕਾਲਮ_Letters() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    string profileFuncname() ਅਚਲ;
    string ਸਤਰ_Directive_Strg(ਪੂਰਨ enterExit) ਅਚਲ;

    // Turn on/off warning messages on this line.
ਨਿਜੀ:
    ਖਾਲੀ warn_ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Subset subset, V3_Error_ਕੋਡ ਕੋਡ, bool flag) {
        m_ਸੁਨੇਹਾ_En_Idx = singleton().ਸੁਨੇਹਾ_En_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ(m_ਸੁਨੇਹਾ_En_Idx, subset, ਕੋਡ, flag);
    }

ਜਨਤਕ:
    ਖਾਲੀ warnOn(V3_Error_ਕੋਡ ਕੋਡ, bool flag) { warn_ਸੈੱਟ(ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ::Subset::ਕੋਡ_2, ਕੋਡ, flag); }
    ਖਾਲੀ warn_ਸੈੱਟ_Ctrl_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(ਅਚਲ VError_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& bitset) {
        m_ਸੁਨੇਹਾ_En_Idx = singleton().ਸੁਨੇਹਾ_ਸੈੱਟ_Ctrl_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ(m_ਸੁਨੇਹਾ_En_Idx, bitset);
    }
    ਖਾਲੀ warnOff(V3_Error_ਕੋਡ ਕੋਡ, bool turnOff) { warnOn(ਕੋਡ, !turnOff); }
    string warnOffParse(ਅਚਲ string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3_Error_ਕੋਡ ਕੋਡ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ;
    ਖਾਲੀ warnLintOff(bool turnOff);
    ਖਾਲੀ warnStyleOff(bool turnOff);
    ਖਾਲੀ warnStateFrom(ਅਚਲ File_ਸਤਰ& from) { m_ਸੁਨੇਹਾ_En_Idx = from.m_ਸੁਨੇਹਾ_En_Idx; }
    ਖਾਲੀ warnResetDefault() { warnStateFrom(default_File_ਸਤਰ()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ਅਚਲ { ਵਾਪਸ ਸੁਨੇਹਾ_En().enabled(V3_Error_ਕੋਡ::I_CELLDEFINE); }
    ਖਾਲੀ celldefineOn(bool flag) { warnOn(V3_Error_ਕੋਡ::I_CELLDEFINE, flag); }
    bool coverageOn() ਅਚਲ { ਵਾਪਸ ਸੁਨੇਹਾ_En().enabled(V3_Error_ਕੋਡ::I_COVERAGE); }
    ਖਾਲੀ coverageOn(bool flag) { warnOn(V3_Error_ਕੋਡ::I_COVERAGE, flag); }
    bool erroringOn() ਅਚਲ { ਵਾਪਸ ਸੁਨੇਹਾ_En().enabled(V3_Error_ਕੋਡ::I_ERRORING); }
    ਖਾਲੀ erroringOn(bool flag) { warnOn(V3_Error_ਕੋਡ::I_ERRORING, flag); }
    bool tracingOn() ਅਚਲ { ਵਾਪਸ ਸੁਨੇਹਾ_En().enabled(V3_Error_ਕੋਡ::I_TRACING); }
    ਖਾਲੀ tracingOn(bool flag) { warnOn(V3_Error_ਕੋਡ::I_TRACING, flag); }
    bool timingOn() ਅਚਲ { ਵਾਪਸ ਸੁਨੇਹਾ_En().enabled(V3_Error_ਕੋਡ::I_TIMING); }
    ਖਾਲੀ timingOn(bool flag) { warnOn(V3_Error_ਕੋਡ::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ਸਥਿਰ string command_ਸਤਰ_Filename() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ "<command-line>"; }
    ਸਥਿਰ string built_ਵਿੱਚ_Filename() VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ "<built-in>"; }
    ਸਥਿਰ ਖਾਲੀ globalWarnOff(V3_Error_ਕੋਡ ਕੋਡ, bool turnOff) {
        default_File_ਸਤਰ().warnOff(ਕੋਡ, turnOff);
    }
    ਸਥਿਰ string globalWarnOffParse(ਅਚਲ string& msgs, bool turnOff) {
        ਵਾਪਸ default_File_ਸਤਰ().warnOffParse(msgs, turnOff);
    }
    ਸਥਿਰ ਖਾਲੀ file_Name_ਅੰਕ_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_ਅੰਕ_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    ਖਾਲੀ modifyStateInherit(ਅਚਲ File_ਸਤਰ* fromp) {
        m_ਸੁਨੇਹਾ_En_Idx = singleton().ਸੁਨੇਹਾ_En_And(m_ਸੁਨੇਹਾ_En_Idx, fromp->m_ਸੁਨੇਹਾ_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    ਖਾਲੀ modifyWarnOff(V3_Error_ਕੋਡ ਕੋਡ, bool turnOff) { warnOff(ਕੋਡ, turnOff); }

    // OPERATORS
    ਖਾਲੀ v3error_ਸਿਰਾ(std::ostringstream& str, ਅਚਲ string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    ਖਾਲੀ v3error_ਸਿਰਾ_Fatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3error_ਸਿਰਾ(str);
        ਪ੍ਰਤਿਜ੍ਞਾ(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warn_ਹੋਰ() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        ਵਾਪਸ warnContext() + warn_Context_ਮਾਪਾ();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ਅਚਲ { ਵਾਪਸ warnContext(); }
    bool ਸੰਚਾਲਕ==(ਅਚਲ File_ਸਤਰ& rhs) ਅਚਲ {
        ਵਾਪਸ (m_token_ਅੰਕ == rhs.m_token_ਅੰਕ && m_ਪਹਿਲਾ_Lineno == rhs.m_ਪਹਿਲਾ_Lineno
                && m_ਪਹਿਲਾ_ਕਾਲਮ == rhs.m_ਪਹਿਲਾ_ਕਾਲਮ && m_ਆਖਰੀ_Lineno_Adder == rhs.m_ਆਖਰੀ_Lineno_Adder
                && m_ਆਖਰੀ_ਕਾਲਮ == rhs.m_ਆਖਰੀ_ਕਾਲਮ && m_filenameno == rhs.m_filenameno
                && m_ਸੁਨੇਹਾ_En_Idx == rhs.m_ਸੁਨੇਹਾ_En_Idx);
    }
    bool equal_ਪਹਿਲਾ_ਸਤਰ_ਖਾਨਾ(ਅਚਲ File_ਸਤਰ& rhs) ਅਚਲ {
        ਵਾਪਸ (m_filenameno == rhs.m_filenameno && m_ਪਹਿਲਾ_Lineno == rhs.m_ਪਹਿਲਾ_Lineno
                && m_ਪਹਿਲਾ_ਕਾਲਮ == rhs.m_ਪਹਿਲਾ_ਕਾਲਮ);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    ਪੂਰਨ operatorCompare(ਅਚਲ File_ਸਤਰ& rhs) ਅਚਲ {
        ਜੇ (m_filenameno != rhs.m_filenameno) ਵਾਪਸ (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        ਜੇ (m_ਪਹਿਲਾ_Lineno != rhs.m_ਪਹਿਲਾ_Lineno)
            ਵਾਪਸ (m_ਪਹਿਲਾ_Lineno < rhs.m_ਪਹਿਲਾ_Lineno) ? -1 : 1;
        ਜੇ (m_ਪਹਿਲਾ_ਕਾਲਮ != rhs.m_ਪਹਿਲਾ_ਕਾਲਮ)
            ਵਾਪਸ (m_ਪਹਿਲਾ_ਕਾਲਮ < rhs.m_ਪਹਿਲਾ_ਕਾਲਮ) ? -1 : 1;
        ਜੇ (m_ਆਖਰੀ_Lineno_Adder != rhs.m_ਆਖਰੀ_Lineno_Adder)
            ਵਾਪਸ (m_ਆਖਰੀ_Lineno_Adder < rhs.m_ਆਖਰੀ_Lineno_Adder) ? -1 : 1;
        ਜੇ (m_ਆਖਰੀ_ਕਾਲਮ != rhs.m_ਆਖਰੀ_ਕਾਲਮ) ਵਾਪਸ (m_ਆਖਰੀ_ਕਾਲਮ < rhs.m_ਆਖਰੀ_ਕਾਲਮ) ? -1 : 1;
        ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& lhs_ਸੁਨੇਹਾ_En = ਸੁਨੇਹਾ_En();
        ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& rhs_ਸੁਨੇਹਾ_En = rhs.ਸੁਨੇਹਾ_En();
        ਲਈ (ਮਾਪ_t i = 0; i < V3_Error_ਕੋਡ::_ENUM_MAX; ++i) {
            ਅਚਲ V3_Error_ਕੋਡ ਕੋਡ = static_cast<V3_Error_ਕੋਡ>(i);
            ਜੇ (lhs_ਸੁਨੇਹਾ_En.enabled(ਕੋਡ) != rhs_ਸੁਨੇਹਾ_En.enabled(ਕੋਡ))
                ਵਾਪਸ rhs_ਸੁਨੇਹਾ_En.enabled(ਕੋਡ) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        ਜੇ (m_token_ਅੰਕ != rhs.m_token_ਅੰਕ) ਵਾਪਸ (m_token_ਅੰਕ < rhs.m_token_ਅੰਕ) ? -1 : 1;
        ਵਾਪਸ 0;  // (*this) and rhs are equivalent
    }

ਨਿਜੀ:
    string warnContext() ਅਚਲ;
    string warn_Context_ਮਾਪਾ() ਅਚਲ VL_REQUIRES(V3Error::s().m_mutex);
    ਅਚਲ ਸੁਨੇਹਾ_En_ਦੋਆਧਾਰੀ_ਅੰਕ_ਸੈੱਟ& ਸੁਨੇਹਾ_En() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ singleton().ਸੁਨੇਹਾ_En(m_ਸੁਨੇਹਾ_En_Idx); }
};
std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, File_ਸਤਰ* fileline);

#ਜੇ_ਅੰਤ  // Guard
