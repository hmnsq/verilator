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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3FILELINE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3FILELINE_H_

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
    ಮಿತ್ರ class File_ಸಾಲು;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using ಸಂದೇಶ_En_ಗಣ_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ final {
        VError_ಬಿಟ್_ಗಣ m_ಸಂಕೇತ_En;  // Enabled by code directives/metacomments
        VError_ಬಿಟ್_ಗಣ m_ctrlEn;  // Enabled by control file

    ಸಾರ್ವಜನಿಕ:
        ಎಣಿಕೆ class Subset : uint8_t {
            ಸಂಕೇತ_3 = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ() = ಪೂರ್ವನಿಯೋಜಿತ;
        // Create intersection set
        ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ(ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& a, ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& b)
            : m_ಸಂಕೇತ_En{a.m_ಸಂಕೇತ_En & b.m_ಸಂಕೇತ_En}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        ರಚನೆ Hash final {
            ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& ಅಂಶ_2) ಸ್ಥಿರಾಂಕ {
                V3Hash hash{ಅಂಶ_2.m_ಸಂಕೇತ_En.hash()};
                hash += ಅಂಶ_2.m_ctrlEn.hash();
                ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
            }
        };

        ರಚನೆ Equal final {
            bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& a, ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& b) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ a == b; }
        };

        bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& other) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ m_ಸಂಕೇತ_En == other.m_ಸಂಕೇತ_En && m_ctrlEn == other.m_ctrlEn;
        }

        ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& getAll(Subset subset) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ subset == Subset::ಸಂಕೇತ_3 ? m_ಸಂಕೇತ_En : m_ctrlEn;
        }
        bool test(Subset subset, V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ subset == Subset::ಸಂಕೇತ_3 ? m_ಸಂಕೇತ_En.test(ಸಂಕೇತ_2) : m_ctrlEn.test(ಸಂಕೇತ_2);
        }
        ಶೂನ್ಯ ಗಣ_All(Subset subset, ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& bitset) {
            ಆದರೆ (subset == Subset::ಸಂಕೇತ_3) {  // LCOV_EXCL_BR_LINE
                m_ಸಂಕೇತ_En = bitset;  // LCOV_EXCL_LINE
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_ctrlEn = bitset;
            }
        }
        ಶೂನ್ಯ ಗಣ(Subset subset, V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool ಮೌಲ್ಯ) {
            ಆದರೆ (subset == Subset::ಸಂಕೇತ_3) {
                m_ಸಂಕೇತ_En.ಗಣ(ಸಂಕೇತ_2, ಮೌಲ್ಯ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_ctrlEn.ಗಣ(ಸಂಕೇತ_2, ಮೌಲ್ಯ);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸಂಕೇತ_En.test(ಸಂಕೇತ_2) && m_ctrlEn.test(ಸಂಕೇತ_2); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ಸ್ಥಿರಾಂಕ std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_ಹೆಸರುಗಳು;  // filename text for each filenameno
    std::deque<V3_Lang_ಸಂಕೇತ> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ, ಸಂದೇಶ_En_ಗಣ_Idx_t, ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Hash, ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Equal>
        m_interned_ಸಂದೇಶ_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::ಸದಿಶ<ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ> m_interned_ಸಂದೇಶ_Ens;

    // CONSTRUCTORS
    File_ಸಾಲು_Singleton() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~File_ಸಾಲು_Singleton() = ಪೂರ್ವನಿಯೋಜಿತ;

    fileNameIdx_t name_ಗಮ್ಯ_ಸಂಖ್ಯೆ(ಸ್ಥಿರಾಂಕ string& filename);
    ಸ್ಥಿರಾಂಕ std::string& ಸಂಖ್ಯೆ_ಗಮ್ಯ_Name(fileNameIdx_t filenameno) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_ಹೆಸರುಗಳು[filenameno];
    }
    V3_Lang_ಸಂಕೇತ ಸಂಖ್ಯೆ_ಗಮ್ಯ_Lang(fileNameIdx_t filenameno) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_languages[filenameno]; }
    ಶೂನ್ಯ ಸಂಖ್ಯೆ_ಗಮ್ಯ_Lang(fileNameIdx_t filenameno, ಸ್ಥಿರಾಂಕ V3_Lang_ಸಂಕೇತ& l) {
        m_languages[filenameno] = l;
    }
    ಶೂನ್ಯ clear() {
        m_namemap.clear();
        m_ಹೆಸರುಗಳು.clear();
        m_languages.clear();
    }
    ಶೂನ್ಯ file_Name_ಅಂಕೆ_Map_Dump_Json(std::ostream& os);
    ಸ್ಥಿರ string filenameLetters(fileNameIdx_t fileno) VL_ಶುದ್ಧ;

    // Add given bitset to the interned bitsets, return interned index
    ಸಂದೇಶ_En_ಗಣ_Idx_t ಕೂಡಿಸು_ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ(ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& ಬಿಟ್_ಗಣ) VL_MT_ಸುರಕ್ಷಿತ_EXCLUDES(m_mutex);
    // Add index of default bitset
    ಸಂದೇಶ_En_ಗಣ_Idx_t default_ಸಂದೇಶ_En_ಸೂಚ್ಯಂಕ() VL_MT_ಸುರಕ್ಷಿತ;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    ಸಂದೇಶ_En_ಗಣ_Idx_t ಸಂದೇಶ_En_ಗಣ_ಬಿಟ್(ಸಂದೇಶ_En_ಗಣ_Idx_t ಗಣ_Idx, ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Subset subset, V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2,
                              bool ಮೌಲ್ಯ);
    // Bulk-set all control codes to given bitset
    ಸಂದೇಶ_En_ಗಣ_Idx_t ಸಂದೇಶ_ಗಣ_Ctrl_ಬಿಟ್_ಗಣ(ಸಂದೇಶ_En_ಗಣ_Idx_t ಗಣ_Idx, ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& bitset);
    // Return index to intersection set
    ಸಂದೇಶ_En_ಗಣ_Idx_t ಸಂದೇಶ_En_And(ಸಂದೇಶ_En_ಗಣ_Idx_t lhsIdx, ಸಂದೇಶ_En_ಗಣ_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& ಸಂದೇಶ_En(ಸಂದೇಶ_En_ಗಣ_Idx_t idx) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ m_interned_ಸಂದೇಶ_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    ಮಿತ್ರ class File_ಸಾಲು;
    // MEMBERS
    ಪೂರ್ಣಾಂಕ m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<ಗಾತ್ರ_t> m_ref_ಎಣಿಕೆ{0};
    std::deque<string> m_ಸಾಲುಗಳು;  // Source text lines
    VFileContent() {
        ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = ಪೂರ್ವನಿಯೋಜಿತ;
    // METHODS
    ಶೂನ್ಯ refInc() { ++m_ref_ಎಣಿಕೆ; }
    ಶೂನ್ಯ refDec() {
        ಆದರೆ (!--m_ref_ಎಣಿಕೆ) delete this;
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ ತಳ್ಳು_ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ);  // Add arbitrary text (need not be line-by-line)
    string get_ಸಾಲು(ಪೂರ್ಣಾಂಕ lineno) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    string ascii() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ "ct" + cvt_ಗಮ್ಯ_Str(m_id); }
};
std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_ಸಾಲು final {

    // CONSTANTS
    ಸ್ಥಿರ constexpr ಚಿಹ್ನೆರಹಿತ SHOW_ಮೂಲ_MAX_ಉದ್ದಳತೆ = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_ಸಾಲು_Singleton::fileNameIdx_t;
    using ಸಂದೇಶ_En_ಗಣ_Idx_t = File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_ಗಣ_Idx_t;
    using ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ = File_ಸಾಲು_Singleton::ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    ಸಂದೇಶ_En_ಗಣ_Idx_t m_ಸಂದೇಶ_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    ಚಿಹ್ನೆರಹಿತ m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_ಅಂಕೆ = 0;  // Token number in processing order
    ಪೂರ್ಣಾಂಕ m_ಮೊದಲ_Lineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_ಮೊದಲ_ಸ್ತಂಭ : 24;  // `line corrected token's first column number
    uint32_t m_ಕೊನೆಯ_ಸ್ತಂಭ : 24;  // `line corrected token's last column number
    uint16_t m_ಕೊನೆಯ_Lineno_Adder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_ಸಾಲು* m_ಪೋಷಕ = nullptr;  // Parent line that included this line

ಸಂರಕ್ಷಿತ:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    ಮಿತ್ರ class File_ಸಾಲು_Singleton;
    ಮಿತ್ರ class V3ParseImp;
    ಮಿತ್ರ class V3PreLex;
    ಮಿತ್ರ class V3PreProcImp;
    ಮಿತ್ರ class V3PreShellImp;

ಖಾಸಗಿ:
    // CONSTRUCTORS
    ಸ್ಥಿರ File_ಸಾಲು_Singleton& singleton() VL_MT_ಸುರಕ್ಷಿತ {
        ಸ್ಥಿರ File_ಸಾಲು_Singleton s;
        ಹಿಂತಿರುಗಿ s;
    }
    ಸ್ಥಿರ File_ಸಾಲು& default_File_ಸಾಲು() VL_MT_ಸುರಕ್ಷಿತ {
        ಸ್ಥಿರ File_ಸಾಲು s;
        ಹಿಂತಿರುಗಿ s;
    }

    File_ಸಾಲು()  // Only used for defaultFileLine above
        : m_ಸಂದೇಶ_En_Idx{singleton().default_ಸಂದೇಶ_En_ಸೂಚ್ಯಂಕ()}
        , m_filenameno{singleton().name_ಗಮ್ಯ_ಸಂಖ್ಯೆ(File_ಸಾಲು::built_ಒಳ_Filename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_ಮೊದಲ_ಸ್ತಂಭ{0}
        , m_ಕೊನೆಯ_ಸ್ತಂಭ{0} {}

ಸಾರ್ವಜನಿಕ:
    explicit File_ಸಾಲು(ಸ್ಥಿರಾಂಕ string& filename)
        : m_ಸಂದೇಶ_En_Idx{default_File_ಸಾಲು().m_ಸಂದೇಶ_En_Idx}
        , m_filenameno{singleton().name_ಗಮ್ಯ_ಸಂಖ್ಯೆ(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_ಮೊದಲ_ಸ್ತಂಭ{0}
        , m_ಕೊನೆಯ_ಸ್ತಂಭ{0} {}
    explicit File_ಸಾಲು(ಸ್ಥಿರಾಂಕ File_ಸಾಲು& from)
        : m_ಸಂದೇಶ_En_Idx{from.m_ಸಂದೇಶ_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_ಅಂಕೆ{from.m_token_ಅಂಕೆ}
        , m_ಮೊದಲ_Lineno{from.m_ಮೊದಲ_Lineno}
        , m_ಮೊದಲ_ಸ್ತಂಭ{from.m_ಮೊದಲ_ಸ್ತಂಭ}
        , m_ಕೊನೆಯ_ಸ್ತಂಭ{from.m_ಕೊನೆಯ_ಸ್ತಂಭ}
        , m_ಕೊನೆಯ_Lineno_Adder{from.m_ಕೊನೆಯ_Lineno_Adder}
        , m_contentp{from.m_contentp}
        , m_ಪೋಷಕ{from.m_ಪೋಷಕ} {
        ಆದರೆ (m_contentp) m_contentp->refInc();
    }
    explicit File_ಸಾಲು(File_ಸಾಲು* fromp)
        : m_ಸಂದೇಶ_En_Idx{fromp->m_ಸಂದೇಶ_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_ಅಂಕೆ{fromp->m_token_ಅಂಕೆ}
        , m_ಮೊದಲ_Lineno{fromp->m_ಮೊದಲ_Lineno}
        , m_ಮೊದಲ_ಸ್ತಂಭ{fromp->m_ಮೊದಲ_ಸ್ತಂಭ}
        , m_ಕೊನೆಯ_ಸ್ತಂಭ{fromp->m_ಕೊನೆಯ_ಸ್ತಂಭ}
        , m_ಕೊನೆಯ_Lineno_Adder{fromp->m_ಕೊನೆಯ_Lineno_Adder}
        , m_contentp{fromp->m_contentp}
        , m_ಪೋಷಕ{fromp->m_ಪೋಷಕ} {
        ಆದರೆ (m_contentp) m_contentp->refInc();
    }
    ಶೂನ್ಯ applyIgnores();
    File_ಸಾಲು* copy_Or_Same_File_ಸಾಲು();
    File_ಸಾಲು* copy_Or_Same_File_ಸಾಲು_Applied();
    ಸ್ಥಿರ ಶೂನ್ಯ delete_All_ಉಳಿದ();
    ಸ್ಥಿರ ಶೂನ್ಯ stats();
    ~File_ಸಾಲು();
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_LEAK_CHECKS
    ಸ್ಥಿರ ಶೂನ್ಯ* ಕ್ರಿಯಾಕಾರಕ new(ಗಾತ್ರ_t ಗಾತ್ರ_2);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕ್ರಿಯಾಕಾರಕ delete(ಶೂನ್ಯ* obj, ಗಾತ್ರ_t ಗಾತ್ರ_2);
#ಆದರೆ_ಅಂತ್ಯ
    // METHODS
    ಶೂನ್ಯ newContent();
    ಶೂನ್ಯ contentLineno(ಪೂರ್ಣಾಂಕ ಅಂಕೆ) {
        lineno(ಅಂಕೆ);
        m_contentLineno = static_cast<ಚಿಹ್ನೆರಹಿತ>(ಅಂಕೆ);
    }
    ಶೂನ್ಯ contentLinenoFrom(ಸ್ಥಿರಾಂಕ File_ಸಾಲು* fromp) { m_contentLineno = fromp->m_contentLineno; }
    ಶೂನ್ಯ lineno(ಪೂರ್ಣಾಂಕ ಅಂಕೆ) {
        m_ಮೊದಲ_Lineno = ಅಂಕೆ;
        m_ಕೊನೆಯ_Lineno_Adder = 0;
        m_ಮೊದಲ_ಸ್ತಂಭ = 1;
        m_ಕೊನೆಯ_ಸ್ತಂಭ = 1;
    }
    ಶೂನ್ಯ ಸ್ತಂಭ_2(ಪೂರ್ಣಾಂಕ ಮೊದಲ_ಸ್ತಂಭ, ಪೂರ್ಣಾಂಕ ಕೊನೆಯ_ಸ್ತಂಭ) {
        m_ಮೊದಲ_ಸ್ತಂಭ = ಮೊದಲ_ಸ್ತಂಭ;
        m_ಕೊನೆಯ_ಸ್ತಂಭ = ಕೊನೆಯ_ಸ್ತಂಭ;
    }
    ಶೂನ್ಯ language(V3_Lang_ಸಂಕೇತ lang) ಸ್ಥಿರಾಂಕ { singleton().ಸಂಖ್ಯೆ_ಗಮ್ಯ_Lang(filenameno(), lang); }
    ಶೂನ್ಯ filename(ಸ್ಥಿರಾಂಕ string& name) { m_filenameno = singleton().name_ಗಮ್ಯ_ಸಂಖ್ಯೆ(name); }
    ಶೂನ್ಯ ಪೋಷಕ(File_ಸಾಲು* fileline) { m_ಪೋಷಕ = fileline; }
    ಶೂನ್ಯ ಸಾಲು_Directive(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಪೂರ್ಣಾಂಕ& enterExitRef);
    ಶೂನ್ಯ ಸಾಲು_Directive_Parse(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, string& filenameRef, ಪೂರ್ಣಾಂಕ& linenoRef,
                            ಪೂರ್ಣಾಂಕ& enterExitRef);
    ಶೂನ್ಯ linenoInc() {
        ಆದರೆ (m_ಕೊನೆಯ_Lineno_Adder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_ಮೊದಲ_Lineno += 65535 + 1;
            m_ಕೊನೆಯ_Lineno_Adder = 0;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ++m_ಕೊನೆಯ_Lineno_Adder;
        }
        m_ಕೊನೆಯ_ಸ್ತಂಭ = 1;
        ++m_contentLineno;
    }
    ಶೂನ್ಯ ಆರಂಭ_Token() {
        m_ಮೊದಲ_Lineno = ಕೊನೆಯ_Lineno();
        m_ಮೊದಲ_ಸ್ತಂಭ = m_ಕೊನೆಯ_ಸ್ತಂಭ;
        m_ಕೊನೆಯ_Lineno_Adder = 0;
    }
    // Advance last line/column based on given text
    ಶೂನ್ಯ forwardToken(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, ಗಾತ್ರ_t ಗಾತ್ರ_2, bool track_ಸಾಲುಗಳು = true);
    uint32_t token_ಅಂಕೆ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_token_ಅಂಕೆ; }
    ಪೂರ್ಣಾಂಕ ಮೊದಲ_Lineno() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಮೊದಲ_Lineno; }
    ಪೂರ್ಣಾಂಕ ಮೊದಲ_ಸ್ತಂಭ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ static_cast<ಪೂರ್ಣಾಂಕ>(m_ಮೊದಲ_ಸ್ತಂಭ); }
    ಪೂರ್ಣಾಂಕ ಕೊನೆಯ_Lineno() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಮೊದಲ_Lineno + m_ಕೊನೆಯ_Lineno_Adder; }
    ಪೂರ್ಣಾಂಕ ಕೊನೆಯ_ಸ್ತಂಭ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ static_cast<ಪೂರ್ಣಾಂಕ>(m_ಕೊನೆಯ_ಸ್ತಂಭ); }
    VFileContent* contentp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    ಪೂರ್ಣಾಂಕ lineno() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ ಕೊನೆಯ_Lineno(); }
    string ಮೂಲ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    string ಮೂಲ_ಪೂರ್ವಪ್ರತ್ಯಯ(ಪೂರ್ಣಾಂಕ ಗಮ್ಯ_ಸ್ತಂಭ) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    string pretty_ಮೂಲ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;  // Source, w/stripped unprintables and newlines
    File_ಸಾಲು* ಪೋಷಕ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ಪೋಷಕ; }
    V3_Lang_ಸಂಕೇತ language() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ singleton().ಸಂಖ್ಯೆ_ಗಮ್ಯ_Lang(filenameno()); }
    string ascii() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    string ascii_ಸಾಲು_ಲಂಬಸಾಲು() ಸ್ಥಿರಾಂಕ;
    ಪೂರ್ಣಾಂಕ filenameno() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_filenameno; }
    ಸ್ಥಿರಾಂಕ std::string& filename() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ singleton().ಸಂಖ್ಯೆ_ಗಮ್ಯ_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ (filename() == command_ಸಾಲು_Filename() || filename() == built_ಒಳ_Filename());
    }
    string filenameLetters() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ File_ಸಾಲು_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    string filebasenameNoExt() ಸ್ಥಿರಾಂಕ;
    string ಮೊದಲ_ಸ್ತಂಭ_Letters() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    string profileFuncname() ಸ್ಥಿರಾಂಕ;
    string ಸಾಲು_Directive_Strg(ಪೂರ್ಣಾಂಕ enterExit) ಸ್ಥಿರಾಂಕ;

    // Turn on/off warning messages on this line.
ಖಾಸಗಿ:
    ಶೂನ್ಯ warn_ಗಣ(ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Subset subset, V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool flag) {
        m_ಸಂದೇಶ_En_Idx = singleton().ಸಂದೇಶ_En_ಗಣ_ಬಿಟ್(m_ಸಂದೇಶ_En_Idx, subset, ಸಂಕೇತ_2, flag);
    }

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ warnOn(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool flag) { warn_ಗಣ(ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ::Subset::ಸಂಕೇತ_3, ಸಂಕೇತ_2, flag); }
    ಶೂನ್ಯ warn_ಗಣ_Ctrl_ಬಿಟ್_ಗಣ(ಸ್ಥಿರಾಂಕ VError_ಬಿಟ್_ಗಣ& bitset) {
        m_ಸಂದೇಶ_En_Idx = singleton().ಸಂದೇಶ_ಗಣ_Ctrl_ಬಿಟ್_ಗಣ(m_ಸಂದೇಶ_En_Idx, bitset);
    }
    ಶೂನ್ಯ warnOff(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool turnOff) { warnOn(ಸಂಕೇತ_2, !turnOff); }
    string warnOffParse(ಸ್ಥಿರಾಂಕ string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ;
    ಶೂನ್ಯ warnLintOff(bool turnOff);
    ಶೂನ್ಯ warnStyleOff(bool turnOff);
    ಶೂನ್ಯ warnStateFrom(ಸ್ಥಿರಾಂಕ File_ಸಾಲು& from) { m_ಸಂದೇಶ_En_Idx = from.m_ಸಂದೇಶ_En_Idx; }
    ಶೂನ್ಯ warnResetDefault() { warnStateFrom(default_File_ಸಾಲು()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಸಂದೇಶ_En().enabled(V3_Error_ಸಂಕೇತ::I_CELLDEFINE); }
    ಶೂನ್ಯ celldefineOn(bool flag) { warnOn(V3_Error_ಸಂಕೇತ::I_CELLDEFINE, flag); }
    bool coverageOn() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಸಂದೇಶ_En().enabled(V3_Error_ಸಂಕೇತ::I_COVERAGE); }
    ಶೂನ್ಯ coverageOn(bool flag) { warnOn(V3_Error_ಸಂಕೇತ::I_COVERAGE, flag); }
    bool erroringOn() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಸಂದೇಶ_En().enabled(V3_Error_ಸಂಕೇತ::I_ERRORING); }
    ಶೂನ್ಯ erroringOn(bool flag) { warnOn(V3_Error_ಸಂಕೇತ::I_ERRORING, flag); }
    bool tracingOn() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಸಂದೇಶ_En().enabled(V3_Error_ಸಂಕೇತ::I_TRACING); }
    ಶೂನ್ಯ tracingOn(bool flag) { warnOn(V3_Error_ಸಂಕೇತ::I_TRACING, flag); }
    bool timingOn() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ಸಂದೇಶ_En().enabled(V3_Error_ಸಂಕೇತ::I_TIMING); }
    ಶೂನ್ಯ timingOn(bool flag) { warnOn(V3_Error_ಸಂಕೇತ::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ಸ್ಥಿರ string command_ಸಾಲು_Filename() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ "<command-line>"; }
    ಸ್ಥಿರ string built_ಒಳ_Filename() VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ "<built-in>"; }
    ಸ್ಥಿರ ಶೂನ್ಯ globalWarnOff(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool turnOff) {
        default_File_ಸಾಲು().warnOff(ಸಂಕೇತ_2, turnOff);
    }
    ಸ್ಥಿರ string globalWarnOffParse(ಸ್ಥಿರಾಂಕ string& msgs, bool turnOff) {
        ಹಿಂತಿರುಗಿ default_File_ಸಾಲು().warnOffParse(msgs, turnOff);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ file_Name_ಅಂಕೆ_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_ಅಂಕೆ_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    ಶೂನ್ಯ modifyStateInherit(ಸ್ಥಿರಾಂಕ File_ಸಾಲು* fromp) {
        m_ಸಂದೇಶ_En_Idx = singleton().ಸಂದೇಶ_En_And(m_ಸಂದೇಶ_En_Idx, fromp->m_ಸಂದೇಶ_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    ಶೂನ್ಯ modifyWarnOff(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool turnOff) { warnOff(ಸಂಕೇತ_2, turnOff); }

    // OPERATORS
    ಶೂನ್ಯ v3error_ಕೊನೆ(std::ostringstream& str, ಸ್ಥಿರಾಂಕ string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    ಶೂನ್ಯ v3error_ಕೊನೆ_Fatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3error_ಕೊನೆ(str);
        ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warn_ಇನ್ನಷ್ಟು() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        ಹಿಂತಿರುಗಿ warnContext() + warn_Context_ಪೋಷಕ();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ warnContext(); }
    bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ File_ಸಾಲು& rhs) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (m_token_ಅಂಕೆ == rhs.m_token_ಅಂಕೆ && m_ಮೊದಲ_Lineno == rhs.m_ಮೊದಲ_Lineno
                && m_ಮೊದಲ_ಸ್ತಂಭ == rhs.m_ಮೊದಲ_ಸ್ತಂಭ && m_ಕೊನೆಯ_Lineno_Adder == rhs.m_ಕೊನೆಯ_Lineno_Adder
                && m_ಕೊನೆಯ_ಸ್ತಂಭ == rhs.m_ಕೊನೆಯ_ಸ್ತಂಭ && m_filenameno == rhs.m_filenameno
                && m_ಸಂದೇಶ_En_Idx == rhs.m_ಸಂದೇಶ_En_Idx);
    }
    bool equal_ಮೊದಲ_ಸಾಲು_ಲಂಬಸಾಲು(ಸ್ಥಿರಾಂಕ File_ಸಾಲು& rhs) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ (m_filenameno == rhs.m_filenameno && m_ಮೊದಲ_Lineno == rhs.m_ಮೊದಲ_Lineno
                && m_ಮೊದಲ_ಸ್ತಂಭ == rhs.m_ಮೊದಲ_ಸ್ತಂಭ);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    ಪೂರ್ಣಾಂಕ operatorCompare(ಸ್ಥಿರಾಂಕ File_ಸಾಲು& rhs) ಸ್ಥಿರಾಂಕ {
        ಆದರೆ (m_filenameno != rhs.m_filenameno) ಹಿಂತಿರುಗಿ (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        ಆದರೆ (m_ಮೊದಲ_Lineno != rhs.m_ಮೊದಲ_Lineno)
            ಹಿಂತಿರುಗಿ (m_ಮೊದಲ_Lineno < rhs.m_ಮೊದಲ_Lineno) ? -1 : 1;
        ಆದರೆ (m_ಮೊದಲ_ಸ್ತಂಭ != rhs.m_ಮೊದಲ_ಸ್ತಂಭ)
            ಹಿಂತಿರುಗಿ (m_ಮೊದಲ_ಸ್ತಂಭ < rhs.m_ಮೊದಲ_ಸ್ತಂಭ) ? -1 : 1;
        ಆದರೆ (m_ಕೊನೆಯ_Lineno_Adder != rhs.m_ಕೊನೆಯ_Lineno_Adder)
            ಹಿಂತಿರುಗಿ (m_ಕೊನೆಯ_Lineno_Adder < rhs.m_ಕೊನೆಯ_Lineno_Adder) ? -1 : 1;
        ಆದರೆ (m_ಕೊನೆಯ_ಸ್ತಂಭ != rhs.m_ಕೊನೆಯ_ಸ್ತಂಭ) ಹಿಂತಿರುಗಿ (m_ಕೊನೆಯ_ಸ್ತಂಭ < rhs.m_ಕೊನೆಯ_ಸ್ತಂಭ) ? -1 : 1;
        ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& lhs_ಸಂದೇಶ_En = ಸಂದೇಶ_En();
        ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& rhs_ಸಂದೇಶ_En = rhs.ಸಂದೇಶ_En();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < V3_Error_ಸಂಕೇತ::_ENUM_MAX; ++i) {
            ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2 = static_cast<V3_Error_ಸಂಕೇತ>(i);
            ಆದರೆ (lhs_ಸಂದೇಶ_En.enabled(ಸಂಕೇತ_2) != rhs_ಸಂದೇಶ_En.enabled(ಸಂಕೇತ_2))
                ಹಿಂತಿರುಗಿ rhs_ಸಂದೇಶ_En.enabled(ಸಂಕೇತ_2) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        ಆದರೆ (m_token_ಅಂಕೆ != rhs.m_token_ಅಂಕೆ) ಹಿಂತಿರುಗಿ (m_token_ಅಂಕೆ < rhs.m_token_ಅಂಕೆ) ? -1 : 1;
        ಹಿಂತಿರುಗಿ 0;  // (*this) and rhs are equivalent
    }

ಖಾಸಗಿ:
    string warnContext() ಸ್ಥಿರಾಂಕ;
    string warn_Context_ಪೋಷಕ() ಸ್ಥಿರಾಂಕ VL_REQUIRES(V3Error::s().m_mutex);
    ಸ್ಥಿರಾಂಕ ಸಂದೇಶ_En_ಬಿಟ್_ಗಣ& ಸಂದೇಶ_En() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ singleton().ಸಂದೇಶ_En(m_ಸಂದೇಶ_En_Idx); }
};
std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, File_ಸಾಲು* fileline);

#ಆದರೆ_ಅಂತ್ಯ  // Guard
