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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Hash.h"
#ଅନ୍ତର୍ଭାଵ "V3LangCode.h"
#ଅନ୍ତର୍ଭାଵ "V3Mutex.h"

#ଅନ୍ତର୍ଭାଵ <atomic>
#ଅନ୍ତର୍ଭାଵ <deque>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <sstream>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

// ######################################################################

class File_ଧାଡ଼ି;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_ଧାଡ଼ି_Singleton final {
    friend class File_ଧାଡ଼ି;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using ସନ୍ଦେଶ_En_Set_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class ସନ୍ଦେଶ_En_Bit_Set final {
        VErrorBitSet m_codeEn;  // Enabled by code directives/metacomments
        VErrorBitSet m_ctrlEn;  // Enabled by control file

    ସର୍ବସାଧାରଣ:
        ଗଣନା class Subset : uint8_t {
            CODE = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        ସନ୍ଦେଶ_En_Bit_Set() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        // Create intersection set
        ସନ୍ଦେଶ_En_Bit_Set(ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& a, ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& b)
            : m_codeEn{a.m_codeEn & b.m_codeEn}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        ସଂରଚନା Hash final {
            size_t operator()(ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& item) ଅଚଳ {
                V3Hash hash{item.m_codeEn.hash()};
                hash += item.m_ctrlEn.hash();
                ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
            }
        };

        ସଂରଚନା Equal final {
            bool operator()(ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& a, ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& b) ଅଚଳ { ଫେରନ୍ତୁ a == b; }
        };

        bool operator==(ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& other) ଅଚଳ {
            ଫେରନ୍ତୁ m_codeEn == other.m_codeEn && m_ctrlEn == other.m_ctrlEn;
        }

        ଅଚଳ VErrorBitSet& getAll(Subset subset) ଅଚଳ {
            ଫେରନ୍ତୁ subset == Subset::CODE ? m_codeEn : m_ctrlEn;
        }
        bool test(Subset subset, V3ErrorCode code) ଅଚଳ {
            ଫେରନ୍ତୁ subset == Subset::CODE ? m_codeEn.test(code) : m_ctrlEn.test(code);
        }
        ଶୂନ୍ୟ setAll(Subset subset, ଅଚଳ VErrorBitSet& bitset) {
            ଯଦି (subset == Subset::CODE) {  // LCOV_EXCL_BR_LINE
                m_codeEn = bitset;  // LCOV_EXCL_LINE
            } ଅନ୍ୟଥା {
                m_ctrlEn = bitset;
            }
        }
        ଶୂନ୍ୟ set(Subset subset, V3ErrorCode code, bool ମୂଲ୍ୟ) {
            ଯଦି (subset == Subset::CODE) {
                m_codeEn.set(code, ମୂଲ୍ୟ);
            } ଅନ୍ୟଥା {
                m_ctrlEn.set(code, ମୂଲ୍ୟ);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3ErrorCode code) ଅଚଳ { ଫେରନ୍ତୁ m_codeEn.test(code) && m_ctrlEn.test(code); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ଅଚଳ std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_names;  // filename text for each filenameno
    std::deque<V3LangCode> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<ସନ୍ଦେଶ_En_Bit_Set, ସନ୍ଦେଶ_En_Set_Idx_t, ସନ୍ଦେଶ_En_Bit_Set::Hash, ସନ୍ଦେଶ_En_Bit_Set::Equal>
        m_interned_ସନ୍ଦେଶ_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::vector<ସନ୍ଦେଶ_En_Bit_Set> m_interned_ସନ୍ଦେଶ_Ens;

    // CONSTRUCTORS
    File_ଧାଡ଼ି_Singleton() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~File_ଧାଡ଼ି_Singleton() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    fileNameIdx_t name_To_ସଂଖ୍ୟା(ଅଚଳ string& filename);
    ଅଚଳ std::string& ସଂଖ୍ୟା_To_Name(fileNameIdx_t filenameno) ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_names[filenameno];
    }
    V3LangCode ସଂଖ୍ୟା_To_Lang(fileNameIdx_t filenameno) ଅଚଳ { ଫେରନ୍ତୁ m_languages[filenameno]; }
    ଶୂନ୍ୟ ସଂଖ୍ୟା_To_Lang(fileNameIdx_t filenameno, ଅଚଳ V3LangCode& l) {
        m_languages[filenameno] = l;
    }
    ଶୂନ୍ୟ clear() {
        m_namemap.clear();
        m_names.clear();
        m_languages.clear();
    }
    ଶୂନ୍ୟ file_Name_ଅଙ୍କ_Map_Dump_Json(std::ostream& os);
    ସ୍ଥିର string filenameLetters(fileNameIdx_t fileno) VL_PURE;

    // Add given bitset to the interned bitsets, return interned index
    ସନ୍ଦେଶ_En_Set_Idx_t add_ସନ୍ଦେଶ_En_Bit_Set(ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& bitSet) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    ସନ୍ଦେଶ_En_Set_Idx_t default_ସନ୍ଦେଶ_En_Index() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    ସନ୍ଦେଶ_En_Set_Idx_t ସନ୍ଦେଶ_En_Set_Bit(ସନ୍ଦେଶ_En_Set_Idx_t setIdx, ସନ୍ଦେଶ_En_Bit_Set::Subset subset, V3ErrorCode code,
                              bool ମୂଲ୍ୟ);
    // Bulk-set all control codes to given bitset
    ସନ୍ଦେଶ_En_Set_Idx_t ସନ୍ଦେଶ_Set_Ctrl_Bit_Set(ସନ୍ଦେଶ_En_Set_Idx_t setIdx, ଅଚଳ VErrorBitSet& bitset);
    // Return index to intersection set
    ସନ୍ଦେଶ_En_Set_Idx_t ସନ୍ଦେଶ_En_And(ସନ୍ଦେଶ_En_Set_Idx_t lhsIdx, ସନ୍ଦେଶ_En_Set_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& ସନ୍ଦେଶ_En(ସନ୍ଦେଶ_En_Set_Idx_t idx) ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ m_interned_ସନ୍ଦେଶ_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    friend class File_ଧାଡ଼ି;
    // MEMBERS
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<size_t> m_ref_ଗଣନା{0};
    std::deque<string> m_ଧାଡ଼ିଗୁଡ଼ିକ;  // Source text lines
    VFileContent() {
        ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // METHODS
    ଶୂନ୍ୟ refInc() { ++m_ref_ଗଣନା; }
    ଶୂନ୍ୟ refDec() {
        ଯଦି (!--m_ref_ଗଣନା) delete this;
    }

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ ଠେଲ_Text(ଅଚଳ string& text);  // Add arbitrary text (need not be line-by-line)
    string get_ଧାଡ଼ି(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno) ଅଚଳ VL_MT_SAFE;
    string ascii() ଅଚଳ { ଫେରନ୍ତୁ "ct" + cvtToStr(m_id); }
};
std::ostream& operator<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_ଧାଡ଼ି final {

    // CONSTANTS
    ସ୍ଥିର constexpr unsigned SHOW_SOURCE_MAX_LENGTH = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_ଧାଡ଼ି_Singleton::fileNameIdx_t;
    using ସନ୍ଦେଶ_En_Set_Idx_t = File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_Set_Idx_t;
    using ସନ୍ଦେଶ_En_Bit_Set = File_ଧାଡ଼ି_Singleton::ସନ୍ଦେଶ_En_Bit_Set;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    ସନ୍ଦେଶ_En_Set_Idx_t m_ସନ୍ଦେଶ_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    unsigned m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_ଅଙ୍କ = 0;  // Token number in processing order
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_lastColumn : 24;  // `line corrected token's last column number
    uint16_t m_lastLinenoAdder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_ଧାଡ଼ି* m_parent = nullptr;  // Parent line that included this line

ସୁରକ୍ଷିତ:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    friend class File_ଧାଡ଼ି_Singleton;
    friend class V3ParseImp;
    friend class V3PreLex;
    friend class V3PreProcImp;
    friend class V3PreShellImp;

ବ୍ୟକ୍ତିଗତ:
    // CONSTRUCTORS
    ସ୍ଥିର File_ଧାଡ଼ି_Singleton& singleton() VL_MT_SAFE {
        ସ୍ଥିର File_ଧାଡ଼ି_Singleton s;
        ଫେରନ୍ତୁ s;
    }
    ସ୍ଥିର File_ଧାଡ଼ି& default_File_ଧାଡ଼ି() VL_MT_SAFE {
        ସ୍ଥିର File_ଧାଡ଼ି s;
        ଫେରନ୍ତୁ s;
    }

    File_ଧାଡ଼ି()  // Only used for defaultFileLine above
        : m_ସନ୍ଦେଶ_En_Idx{singleton().default_ସନ୍ଦେଶ_En_Index()}
        , m_filenameno{singleton().name_To_ସଂଖ୍ୟା(File_ଧାଡ଼ି::builtInFilename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}

ସର୍ବସାଧାରଣ:
    explicit File_ଧାଡ଼ି(ଅଚଳ string& filename)
        : m_ସନ୍ଦେଶ_En_Idx{default_File_ଧାଡ଼ି().m_ସନ୍ଦେଶ_En_Idx}
        , m_filenameno{singleton().name_To_ସଂଖ୍ୟା(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_lastColumn{0} {}
    explicit File_ଧାଡ଼ି(ଅଚଳ File_ଧାଡ଼ି& from)
        : m_ସନ୍ଦେଶ_En_Idx{from.m_ସନ୍ଦେଶ_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_ଅଙ୍କ{from.m_token_ଅଙ୍କ}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_lastColumn{from.m_lastColumn}
        , m_lastLinenoAdder{from.m_lastLinenoAdder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        ଯଦି (m_contentp) m_contentp->refInc();
    }
    explicit File_ଧାଡ଼ି(File_ଧାଡ଼ି* fromp)
        : m_ସନ୍ଦେଶ_En_Idx{fromp->m_ସନ୍ଦେଶ_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_ଅଙ୍କ{fromp->m_token_ଅଙ୍କ}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_lastColumn{fromp->m_lastColumn}
        , m_lastLinenoAdder{fromp->m_lastLinenoAdder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        ଯଦି (m_contentp) m_contentp->refInc();
    }
    ଶୂନ୍ୟ applyIgnores();
    File_ଧାଡ଼ି* copy_Or_Same_File_ଧାଡ଼ି();
    File_ଧାଡ଼ି* copy_Or_Same_File_ଧାଡ଼ି_Applied();
    ସ୍ଥିର ଶୂନ୍ୟ deleteAllRemaining();
    ସ୍ଥିର ଶୂନ୍ୟ stats();
    ~File_ଧାଡ଼ି();
#ifdef VL_LEAK_CHECKS
    ସ୍ଥିର ଶୂନ୍ୟ* operator new(size_t size);
    ସ୍ଥିର ଶୂନ୍ୟ operator delete(ଶୂନ୍ୟ* obj, size_t size);
#endif
    // METHODS
    ଶୂନ୍ୟ newContent();
    ଶୂନ୍ୟ contentLineno(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ) {
        lineno(ଅଙ୍କ);
        m_contentLineno = static_cast<unsigned>(ଅଙ୍କ);
    }
    ଶୂନ୍ୟ contentLinenoFrom(ଅଚଳ File_ଧାଡ଼ି* fromp) { m_contentLineno = fromp->m_contentLineno; }
    ଶୂନ୍ୟ lineno(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ) {
        m_firstLineno = ଅଙ୍କ;
        m_lastLinenoAdder = 0;
        m_firstColumn = 1;
        m_lastColumn = 1;
    }
    ଶୂନ୍ୟ column(ପୂର୍ଣ୍ଣସଂଖ୍ୟା firstColumn, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lastColumn) {
        m_firstColumn = firstColumn;
        m_lastColumn = lastColumn;
    }
    ଶୂନ୍ୟ language(V3LangCode lang) ଅଚଳ { singleton().ସଂଖ୍ୟା_To_Lang(filenameno(), lang); }
    ଶୂନ୍ୟ filename(ଅଚଳ string& name) { m_filenameno = singleton().name_To_ସଂଖ୍ୟା(name); }
    ଶୂନ୍ୟ parent(File_ଧାଡ଼ି* fileline) { m_parent = fileline; }
    ଶୂନ୍ୟ ଧାଡ଼ି_Directive(ଅଚଳ ଅକ୍ଷର* textp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& enterExitRef);
    ଶୂନ୍ୟ ଧାଡ଼ି_Directive_Parse(ଅଚଳ ଅକ୍ଷର* textp, string& filenameRef, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& linenoRef,
                            ପୂର୍ଣ୍ଣସଂଖ୍ୟା& enterExitRef);
    ଶୂନ୍ୟ linenoInc() {
        ଯଦି (m_lastLinenoAdder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_lastLinenoAdder = 0;
        } ଅନ୍ୟଥା {
            ++m_lastLinenoAdder;
        }
        m_lastColumn = 1;
        ++m_contentLineno;
    }
    ଶୂନ୍ୟ startToken() {
        m_firstLineno = lastLineno();
        m_firstColumn = m_lastColumn;
        m_lastLinenoAdder = 0;
    }
    // Advance last line/column based on given text
    ଶୂନ୍ୟ forwardToken(ଅଚଳ ଅକ୍ଷର* textp, size_t size, bool track_ଧାଡ଼ିଗୁଡ଼ିକ = true);
    uint32_t token_ଅଙ୍କ() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_token_ଅଙ୍କ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା firstLineno() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_firstLineno; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା firstColumn() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_firstColumn); }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lastLineno() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_firstLineno + m_lastLinenoAdder; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lastColumn() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_lastColumn); }
    VFileContent* contentp() ଅଚଳ { ଫେରନ୍ତୁ m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ lastLineno(); }
    string source() ଅଚଳ VL_MT_SAFE;
    string sourcePrefix(ପୂର୍ଣ୍ଣସଂଖ୍ୟା toColumn) ଅଚଳ VL_MT_SAFE;
    string prettySource() ଅଚଳ VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_ଧାଡ଼ି* parent() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_parent; }
    V3LangCode language() ଅଚଳ { ଫେରନ୍ତୁ singleton().ସଂଖ୍ୟା_To_Lang(filenameno()); }
    string ascii() ଅଚଳ VL_MT_SAFE;
    string ascii_ଧାଡ଼ି_Col() ଅଚଳ;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା filenameno() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_filenameno; }
    ଅଚଳ std::string& filename() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ singleton().ସଂଖ୍ୟା_To_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ (filename() == command_ଧାଡ଼ି_Filename() || filename() == builtInFilename());
    }
    string filenameLetters() ଅଚଳ VL_MT_SAFE {
        ଫେରନ୍ତୁ File_ଧାଡ଼ି_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ଅଚଳ VL_MT_SAFE;
    string filebasenameNoExt() ଅଚଳ;
    string firstColumnLetters() ଅଚଳ VL_MT_SAFE;
    string profileFuncname() ଅଚଳ;
    string ଧାଡ଼ି_Directive_Strg(ପୂର୍ଣ୍ଣସଂଖ୍ୟା enterExit) ଅଚଳ;

    // Turn on/off warning messages on this line.
ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ warnSet(ସନ୍ଦେଶ_En_Bit_Set::Subset subset, V3ErrorCode code, bool flag) {
        m_ସନ୍ଦେଶ_En_Idx = singleton().ସନ୍ଦେଶ_En_Set_Bit(m_ସନ୍ଦେଶ_En_Idx, subset, code, flag);
    }

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ warnOn(V3ErrorCode code, bool flag) { warnSet(ସନ୍ଦେଶ_En_Bit_Set::Subset::CODE, code, flag); }
    ଶୂନ୍ୟ warnSetCtrlBitSet(ଅଚଳ VErrorBitSet& bitset) {
        m_ସନ୍ଦେଶ_En_Idx = singleton().ସନ୍ଦେଶ_Set_Ctrl_Bit_Set(m_ସନ୍ଦେଶ_En_Idx, bitset);
    }
    ଶୂନ୍ୟ warnOff(V3ErrorCode code, bool turnOff) { warnOn(code, !turnOff); }
    string warnOffParse(ଅଚଳ string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3ErrorCode code) ଅଚଳ VL_MT_SAFE;
    ଶୂନ୍ୟ warnLintOff(bool turnOff);
    ଶୂନ୍ୟ warnStyleOff(bool turnOff);
    ଶୂନ୍ୟ warnStateFrom(ଅଚଳ File_ଧାଡ଼ି& from) { m_ସନ୍ଦେଶ_En_Idx = from.m_ସନ୍ଦେଶ_En_Idx; }
    ଶୂନ୍ୟ warnResetDefault() { warnStateFrom(default_File_ଧାଡ଼ି()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ଅଚଳ { ଫେରନ୍ତୁ ସନ୍ଦେଶ_En().enabled(V3ErrorCode::I_CELLDEFINE); }
    ଶୂନ୍ୟ celldefineOn(bool flag) { warnOn(V3ErrorCode::I_CELLDEFINE, flag); }
    bool coverageOn() ଅଚଳ { ଫେରନ୍ତୁ ସନ୍ଦେଶ_En().enabled(V3ErrorCode::I_COVERAGE); }
    ଶୂନ୍ୟ coverageOn(bool flag) { warnOn(V3ErrorCode::I_COVERAGE, flag); }
    bool erroringOn() ଅଚଳ { ଫେରନ୍ତୁ ସନ୍ଦେଶ_En().enabled(V3ErrorCode::I_ERRORING); }
    ଶୂନ୍ୟ erroringOn(bool flag) { warnOn(V3ErrorCode::I_ERRORING, flag); }
    bool tracingOn() ଅଚଳ { ଫେରନ୍ତୁ ସନ୍ଦେଶ_En().enabled(V3ErrorCode::I_TRACING); }
    ଶୂନ୍ୟ tracingOn(bool flag) { warnOn(V3ErrorCode::I_TRACING, flag); }
    bool timingOn() ଅଚଳ { ଫେରନ୍ତୁ ସନ୍ଦେଶ_En().enabled(V3ErrorCode::I_TIMING); }
    ଶୂନ୍ୟ timingOn(bool flag) { warnOn(V3ErrorCode::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ସ୍ଥିର string command_ଧାଡ଼ି_Filename() VL_MT_SAFE { ଫେରନ୍ତୁ "<command-line>"; }
    ସ୍ଥିର string builtInFilename() VL_MT_SAFE { ଫେରନ୍ତୁ "<built-in>"; }
    ସ୍ଥିର ଶୂନ୍ୟ globalWarnOff(V3ErrorCode code, bool turnOff) {
        default_File_ଧାଡ଼ି().warnOff(code, turnOff);
    }
    ସ୍ଥିର string globalWarnOffParse(ଅଚଳ string& msgs, bool turnOff) {
        ଫେରନ୍ତୁ default_File_ଧାଡ଼ି().warnOffParse(msgs, turnOff);
    }
    ସ୍ଥିର ଶୂନ୍ୟ file_Name_ଅଙ୍କ_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_ଅଙ୍କ_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    ଶୂନ୍ୟ modifyStateInherit(ଅଚଳ File_ଧାଡ଼ି* fromp) {
        m_ସନ୍ଦେଶ_En_Idx = singleton().ସନ୍ଦେଶ_En_And(m_ସନ୍ଦେଶ_En_Idx, fromp->m_ସନ୍ଦେଶ_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    ଶୂନ୍ୟ modifyWarnOff(V3ErrorCode code, bool turnOff) { warnOff(code, turnOff); }

    // OPERATORS
    ଶୂନ୍ୟ v3errorEnd(std::ostringstream& str, ଅଚଳ string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    ଶୂନ୍ୟ v3errorEndFatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3errorEnd(str);
        ପ୍ରତିଜ୍ଞା(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warnMore() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        ଫେରନ୍ତୁ warnContext() + warnContextParent();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ଅଚଳ { ଫେରନ୍ତୁ warnContext(); }
    bool operator==(ଅଚଳ File_ଧାଡ଼ି& rhs) ଅଚଳ {
        ଫେରନ୍ତୁ (m_token_ଅଙ୍କ == rhs.m_token_ଅଙ୍କ && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_lastLinenoAdder == rhs.m_lastLinenoAdder
                && m_lastColumn == rhs.m_lastColumn && m_filenameno == rhs.m_filenameno
                && m_ସନ୍ଦେଶ_En_Idx == rhs.m_ସନ୍ଦେଶ_En_Idx);
    }
    bool equal_First_ଧାଡ଼ି_Col(ଅଚଳ File_ଧାଡ଼ି& rhs) ଅଚଳ {
        ଫେରନ୍ତୁ (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା operatorCompare(ଅଚଳ File_ଧାଡ଼ି& rhs) ଅଚଳ {
        ଯଦି (m_filenameno != rhs.m_filenameno) ଫେରନ୍ତୁ (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        ଯଦି (m_firstLineno != rhs.m_firstLineno)
            ଫେରନ୍ତୁ (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        ଯଦି (m_firstColumn != rhs.m_firstColumn)
            ଫେରନ୍ତୁ (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        ଯଦି (m_lastLinenoAdder != rhs.m_lastLinenoAdder)
            ଫେରନ୍ତୁ (m_lastLinenoAdder < rhs.m_lastLinenoAdder) ? -1 : 1;
        ଯଦି (m_lastColumn != rhs.m_lastColumn) ଫେରନ୍ତୁ (m_lastColumn < rhs.m_lastColumn) ? -1 : 1;
        ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& lhs_ସନ୍ଦେଶ_En = ସନ୍ଦେଶ_En();
        ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& rhs_ସନ୍ଦେଶ_En = rhs.ସନ୍ଦେଶ_En();
        ପାଇଁ (size_t i = 0; i < V3ErrorCode::_ENUM_MAX; ++i) {
            ଅଚଳ V3ErrorCode code = static_cast<V3ErrorCode>(i);
            ଯଦି (lhs_ସନ୍ଦେଶ_En.enabled(code) != rhs_ସନ୍ଦେଶ_En.enabled(code))
                ଫେରନ୍ତୁ rhs_ସନ୍ଦେଶ_En.enabled(code) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        ଯଦି (m_token_ଅଙ୍କ != rhs.m_token_ଅଙ୍କ) ଫେରନ୍ତୁ (m_token_ଅଙ୍କ < rhs.m_token_ଅଙ୍କ) ? -1 : 1;
        ଫେରନ୍ତୁ 0;  // (*this) and rhs are equivalent
    }

ବ୍ୟକ୍ତିଗତ:
    string warnContext() ଅଚଳ;
    string warnContextParent() ଅଚଳ VL_REQUIRES(V3Error::s().m_mutex);
    ଅଚଳ ସନ୍ଦେଶ_En_Bit_Set& ସନ୍ଦେଶ_En() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ singleton().ସନ୍ଦେଶ_En(m_ସନ୍ଦେଶ_En_Idx); }
};
std::ostream& operator<<(std::ostream& os, File_ଧାଡ଼ି* fileline);

#endif  // Guard
