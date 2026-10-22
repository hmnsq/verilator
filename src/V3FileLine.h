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

#यदि_अपरिभाषितम् VERILATOR_V3FILELINE_H_
#परिभाषय VERILATOR_V3FILELINE_H_

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

class File_पङ्क्तिः;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_पङ्क्तिः_Singleton final {
    मित्रम् class File_पङ्क्तिः;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using सन्देशः_En_समुच्चयः_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class सन्देशः_En_Bit_समुच्चयः final {
        VError_Bit_समुच्चयः m_संहिता_En;  // Enabled by code directives/metacomments
        VError_Bit_समुच्चयः m_ctrlEn;  // Enabled by control file

    सार्वजनिक:
        गणना class Subset : uint8_t {
            संहिता_2 = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        सन्देशः_En_Bit_समुच्चयः() = स्वभाव;
        // Create intersection set
        सन्देशः_En_Bit_समुच्चयः(ध्रुव सन्देशः_En_Bit_समुच्चयः& a, ध्रुव सन्देशः_En_Bit_समुच्चयः& b)
            : m_संहिता_En{a.m_संहिता_En & b.m_संहिता_En}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        संरचना Hash final {
            आकार_t संक्रियकः()(ध्रुव सन्देशः_En_Bit_समुच्चयः& item) ध्रुव {
                V3Hash hash{item.m_संहिता_En.hash()};
                hash += item.m_ctrlEn.hash();
                प्रत्यागम hash.मानम्();
            }
        };

        संरचना Equal final {
            bool संक्रियकः()(ध्रुव सन्देशः_En_Bit_समुच्चयः& a, ध्रुव सन्देशः_En_Bit_समुच्चयः& b) ध्रुव { प्रत्यागम a == b; }
        };

        bool संक्रियकः==(ध्रुव सन्देशः_En_Bit_समुच्चयः& other) ध्रुव {
            प्रत्यागम m_संहिता_En == other.m_संहिता_En && m_ctrlEn == other.m_ctrlEn;
        }

        ध्रुव VError_Bit_समुच्चयः& getAll(Subset subset) ध्रुव {
            प्रत्यागम subset == Subset::संहिता_2 ? m_संहिता_En : m_ctrlEn;
        }
        bool test(Subset subset, V3_Error_संहिता संहिता) ध्रुव {
            प्रत्यागम subset == Subset::संहिता_2 ? m_संहिता_En.test(संहिता) : m_ctrlEn.test(संहिता);
        }
        शून्य समुच्चयः_All(Subset subset, ध्रुव VError_Bit_समुच्चयः& bitset) {
            यदि (subset == Subset::संहिता_2) {  // LCOV_EXCL_BR_LINE
                m_संहिता_En = bitset;  // LCOV_EXCL_LINE
            } अन्यथा {
                m_ctrlEn = bitset;
            }
        }
        शून्य समुच्चयः(Subset subset, V3_Error_संहिता संहिता, bool मानम्) {
            यदि (subset == Subset::संहिता_2) {
                m_संहिता_En.समुच्चयः(संहिता, मानम्);
            } अन्यथा {
                m_ctrlEn.समुच्चयः(संहिता, मानम्);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3_Error_संहिता संहिता) ध्रुव { प्रत्यागम m_संहिता_En.test(संहिता) && m_ctrlEn.test(संहिता); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ध्रुव std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_नामानि;  // filename text for each filenameno
    std::deque<V3_Lang_संहिता> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<सन्देशः_En_Bit_समुच्चयः, सन्देशः_En_समुच्चयः_Idx_t, सन्देशः_En_Bit_समुच्चयः::Hash, सन्देशः_En_Bit_समुच्चयः::Equal>
        m_interned_सन्देशः_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::सदिशम्<सन्देशः_En_Bit_समुच्चयः> m_interned_सन्देशः_Ens;

    // CONSTRUCTORS
    File_पङ्क्तिः_Singleton() = स्वभाव;
    ~File_पङ्क्तिः_Singleton() = स्वभाव;

    fileNameIdx_t name_गन्तव्यम्_सङ्ख्या(ध्रुव string& filename);
    ध्रुव std::string& सङ्ख्या_गन्तव्यम्_Name(fileNameIdx_t filenameno) ध्रुव VL_MT_SAFE {
        प्रत्यागम m_नामानि[filenameno];
    }
    V3_Lang_संहिता सङ्ख्या_गन्तव्यम्_Lang(fileNameIdx_t filenameno) ध्रुव { प्रत्यागम m_languages[filenameno]; }
    शून्य सङ्ख्या_गन्तव्यम्_Lang(fileNameIdx_t filenameno, ध्रुव V3_Lang_संहिता& l) {
        m_languages[filenameno] = l;
    }
    शून्य clear() {
        m_namemap.clear();
        m_नामानि.clear();
        m_languages.clear();
    }
    शून्य file_Name_अङ्कः_Map_Dump_Json(std::ostream& os);
    स्थिर string filenameLetters(fileNameIdx_t fileno) VL_विशुद्धम्;

    // Add given bitset to the interned bitsets, return interned index
    सन्देशः_En_समुच्चयः_Idx_t सङ्कलय_सन्देशः_En_Bit_समुच्चयः(ध्रुव सन्देशः_En_Bit_समुच्चयः& bit_समुच्चयः) VL_MT_SAFE_EXCLUDES(m_mutex);
    // Add index of default bitset
    सन्देशः_En_समुच्चयः_Idx_t default_सन्देशः_En_सूचकांक() VL_MT_SAFE;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    सन्देशः_En_समुच्चयः_Idx_t सन्देशः_En_समुच्चयः_Bit(सन्देशः_En_समुच्चयः_Idx_t समुच्चयः_Idx, सन्देशः_En_Bit_समुच्चयः::Subset subset, V3_Error_संहिता संहिता,
                              bool मानम्);
    // Bulk-set all control codes to given bitset
    सन्देशः_En_समुच्चयः_Idx_t सन्देशः_समुच्चयः_Ctrl_Bit_समुच्चयः(सन्देशः_En_समुच्चयः_Idx_t समुच्चयः_Idx, ध्रुव VError_Bit_समुच्चयः& bitset);
    // Return index to intersection set
    सन्देशः_En_समुच्चयः_Idx_t सन्देशः_En_And(सन्देशः_En_समुच्चयः_Idx_t lhsIdx, सन्देशः_En_समुच्चयः_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ध्रुव सन्देशः_En_Bit_समुच्चयः& सन्देशः_En(सन्देशः_En_समुच्चयः_Idx_t idx) ध्रुव VL_MT_SAFE {
        प्रत्यागम m_interned_सन्देशः_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    मित्रम् class File_पङ्क्तिः;
    // MEMBERS
    पूर्णाङ्क m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<आकार_t> m_ref_गणनम्{0};
    std::deque<string> m_पङ्क्तयः;  // Source text lines
    VFileContent() {
        स्थिर पूर्णाङ्क s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = स्वभाव;
    // METHODS
    शून्य refInc() { ++m_ref_गणनम्; }
    शून्य refDec() {
        यदि (!--m_ref_गणनम्) delete this;
    }

सार्वजनिक:
    शून्य प्रक्षिप_Text(ध्रुव string& text);  // Add arbitrary text (need not be line-by-line)
    string get_पङ्क्तिः(पूर्णाङ्क lineno) ध्रुव VL_MT_SAFE;
    string ascii() ध्रुव { प्रत्यागम "ct" + cvt_गन्तव्यम्_Str(m_id); }
};
std::ostream& संक्रियकः<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_पङ्क्तिः final {

    // CONSTANTS
    स्थिर constexpr अचिह्नित SHOW_SOURCE_MAX_दीर्घता = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_पङ्क्तिः_Singleton::fileNameIdx_t;
    using सन्देशः_En_समुच्चयः_Idx_t = File_पङ्क्तिः_Singleton::सन्देशः_En_समुच्चयः_Idx_t;
    using सन्देशः_En_Bit_समुच्चयः = File_पङ्क्तिः_Singleton::सन्देशः_En_Bit_समुच्चयः;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    सन्देशः_En_समुच्चयः_Idx_t m_सन्देशः_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    अचिह्नित m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_अङ्कः = 0;  // Token number in processing order
    पूर्णाङ्क m_firstLineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_firstColumn : 24;  // `line corrected token's first column number
    uint32_t m_चरमः_Column : 24;  // `line corrected token's last column number
    uint16_t m_चरमः_Lineno_Adder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_पङ्क्तिः* m_parent = nullptr;  // Parent line that included this line

रक्षित:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    मित्रम् class File_पङ्क्तिः_Singleton;
    मित्रम् class V3ParseImp;
    मित्रम् class V3PreLex;
    मित्रम् class V3PreProcImp;
    मित्रम् class V3PreShellImp;

गुप्त:
    // CONSTRUCTORS
    स्थिर File_पङ्क्तिः_Singleton& singleton() VL_MT_SAFE {
        स्थिर File_पङ्क्तिः_Singleton s;
        प्रत्यागम s;
    }
    स्थिर File_पङ्क्तिः& default_File_पङ्क्तिः() VL_MT_SAFE {
        स्थिर File_पङ्क्तिः s;
        प्रत्यागम s;
    }

    File_पङ्क्तिः()  // Only used for defaultFileLine above
        : m_सन्देशः_En_Idx{singleton().default_सन्देशः_En_सूचकांक()}
        , m_filenameno{singleton().name_गन्तव्यम्_सङ्ख्या(File_पङ्क्तिः::built_अन्तर्_Filename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_चरमः_Column{0} {}

सार्वजनिक:
    explicit File_पङ्क्तिः(ध्रुव string& filename)
        : m_सन्देशः_En_Idx{default_File_पङ्क्तिः().m_सन्देशः_En_Idx}
        , m_filenameno{singleton().name_गन्तव्यम्_सङ्ख्या(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_firstColumn{0}
        , m_चरमः_Column{0} {}
    explicit File_पङ्क्तिः(ध्रुव File_पङ्क्तिः& from)
        : m_सन्देशः_En_Idx{from.m_सन्देशः_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_अङ्कः{from.m_token_अङ्कः}
        , m_firstLineno{from.m_firstLineno}
        , m_firstColumn{from.m_firstColumn}
        , m_चरमः_Column{from.m_चरमः_Column}
        , m_चरमः_Lineno_Adder{from.m_चरमः_Lineno_Adder}
        , m_contentp{from.m_contentp}
        , m_parent{from.m_parent} {
        यदि (m_contentp) m_contentp->refInc();
    }
    explicit File_पङ्क्तिः(File_पङ्क्तिः* fromp)
        : m_सन्देशः_En_Idx{fromp->m_सन्देशः_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_अङ्कः{fromp->m_token_अङ्कः}
        , m_firstLineno{fromp->m_firstLineno}
        , m_firstColumn{fromp->m_firstColumn}
        , m_चरमः_Column{fromp->m_चरमः_Column}
        , m_चरमः_Lineno_Adder{fromp->m_चरमः_Lineno_Adder}
        , m_contentp{fromp->m_contentp}
        , m_parent{fromp->m_parent} {
        यदि (m_contentp) m_contentp->refInc();
    }
    शून्य applyIgnores();
    File_पङ्क्तिः* copy_Or_Same_File_पङ्क्तिः();
    File_पङ्क्तिः* copy_Or_Same_File_पङ्क्तिः_Applied();
    स्थिर शून्य deleteAllRemaining();
    स्थिर शून्य stats();
    ~File_पङ्क्तिः();
#यदि_परिभाषितम् VL_LEAK_CHECKS
    स्थिर शून्य* संक्रियकः new(आकार_t आकार);
    स्थिर शून्य संक्रियकः delete(शून्य* obj, आकार_t आकार);
#यदि_अन्तः
    // METHODS
    शून्य newContent();
    शून्य contentLineno(पूर्णाङ्क अङ्कः) {
        lineno(अङ्कः);
        m_contentLineno = static_cast<अचिह्नित>(अङ्कः);
    }
    शून्य contentLinenoFrom(ध्रुव File_पङ्क्तिः* fromp) { m_contentLineno = fromp->m_contentLineno; }
    शून्य lineno(पूर्णाङ्क अङ्कः) {
        m_firstLineno = अङ्कः;
        m_चरमः_Lineno_Adder = 0;
        m_firstColumn = 1;
        m_चरमः_Column = 1;
    }
    शून्य column(पूर्णाङ्क firstColumn, पूर्णाङ्क चरमः_Column) {
        m_firstColumn = firstColumn;
        m_चरमः_Column = चरमः_Column;
    }
    शून्य language(V3_Lang_संहिता lang) ध्रुव { singleton().सङ्ख्या_गन्तव्यम्_Lang(filenameno(), lang); }
    शून्य filename(ध्रुव string& name) { m_filenameno = singleton().name_गन्तव्यम्_सङ्ख्या(name); }
    शून्य parent(File_पङ्क्तिः* fileline) { m_parent = fileline; }
    शून्य पङ्क्तिः_Directive(ध्रुव वर्ण* textp, पूर्णाङ्क& enterExitRef);
    शून्य पङ्क्तिः_Directive_Parse(ध्रुव वर्ण* textp, string& filenameRef, पूर्णाङ्क& linenoRef,
                            पूर्णाङ्क& enterExitRef);
    शून्य linenoInc() {
        यदि (m_चरमः_Lineno_Adder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_firstLineno += 65535 + 1;
            m_चरमः_Lineno_Adder = 0;
        } अन्यथा {
            ++m_चरमः_Lineno_Adder;
        }
        m_चरमः_Column = 1;
        ++m_contentLineno;
    }
    शून्य startToken() {
        m_firstLineno = चरमः_Lineno();
        m_firstColumn = m_चरमः_Column;
        m_चरमः_Lineno_Adder = 0;
    }
    // Advance last line/column based on given text
    शून्य forwardToken(ध्रुव वर्ण* textp, आकार_t आकार, bool track_पङ्क्तयः = true);
    uint32_t token_अङ्कः() ध्रुव VL_MT_SAFE { प्रत्यागम m_token_अङ्कः; }
    पूर्णाङ्क firstLineno() ध्रुव VL_MT_SAFE { प्रत्यागम m_firstLineno; }
    पूर्णाङ्क firstColumn() ध्रुव VL_MT_SAFE { प्रत्यागम static_cast<पूर्णाङ्क>(m_firstColumn); }
    पूर्णाङ्क चरमः_Lineno() ध्रुव VL_MT_SAFE { प्रत्यागम m_firstLineno + m_चरमः_Lineno_Adder; }
    पूर्णाङ्क चरमः_Column() ध्रुव VL_MT_SAFE { प्रत्यागम static_cast<पूर्णाङ्क>(m_चरमः_Column); }
    VFileContent* contentp() ध्रुव { प्रत्यागम m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    पूर्णाङ्क lineno() ध्रुव VL_MT_SAFE { प्रत्यागम चरमः_Lineno(); }
    string source() ध्रुव VL_MT_SAFE;
    string source_उपसर्गः(पूर्णाङ्क गन्तव्यम्_Column) ध्रुव VL_MT_SAFE;
    string prettySource() ध्रुव VL_MT_SAFE;  // Source, w/stripped unprintables and newlines
    File_पङ्क्तिः* parent() ध्रुव VL_MT_SAFE { प्रत्यागम m_parent; }
    V3_Lang_संहिता language() ध्रुव { प्रत्यागम singleton().सङ्ख्या_गन्तव्यम्_Lang(filenameno()); }
    string ascii() ध्रुव VL_MT_SAFE;
    string ascii_पङ्क्तिः_Col() ध्रुव;
    पूर्णाङ्क filenameno() ध्रुव VL_MT_SAFE { प्रत्यागम m_filenameno; }
    ध्रुव std::string& filename() ध्रुव VL_MT_SAFE {
        प्रत्यागम singleton().सङ्ख्या_गन्तव्यम्_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ध्रुव VL_MT_SAFE { प्रत्यागम VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ध्रुव VL_MT_SAFE {
        प्रत्यागम (filename() == command_पङ्क्तिः_Filename() || filename() == built_अन्तर्_Filename());
    }
    string filenameLetters() ध्रुव VL_MT_SAFE {
        प्रत्यागम File_पङ्क्तिः_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ध्रुव VL_MT_SAFE;
    string filebasenameNoExt() ध्रुव;
    string firstColumnLetters() ध्रुव VL_MT_SAFE;
    string profileFuncname() ध्रुव;
    string पङ्क्तिः_Directive_Strg(पूर्णाङ्क enterExit) ध्रुव;

    // Turn on/off warning messages on this line.
गुप्त:
    शून्य warn_समुच्चयः(सन्देशः_En_Bit_समुच्चयः::Subset subset, V3_Error_संहिता संहिता, bool flag) {
        m_सन्देशः_En_Idx = singleton().सन्देशः_En_समुच्चयः_Bit(m_सन्देशः_En_Idx, subset, संहिता, flag);
    }

सार्वजनिक:
    शून्य warnOn(V3_Error_संहिता संहिता, bool flag) { warn_समुच्चयः(सन्देशः_En_Bit_समुच्चयः::Subset::संहिता_2, संहिता, flag); }
    शून्य warn_समुच्चयः_Ctrl_Bit_समुच्चयः(ध्रुव VError_Bit_समुच्चयः& bitset) {
        m_सन्देशः_En_Idx = singleton().सन्देशः_समुच्चयः_Ctrl_Bit_समुच्चयः(m_सन्देशः_En_Idx, bitset);
    }
    शून्य warnOff(V3_Error_संहिता संहिता, bool turnOff) { warnOn(संहिता, !turnOff); }
    string warnOffParse(ध्रुव string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3_Error_संहिता संहिता) ध्रुव VL_MT_SAFE;
    शून्य warnLintOff(bool turnOff);
    शून्य warnStyleOff(bool turnOff);
    शून्य warnStateFrom(ध्रुव File_पङ्क्तिः& from) { m_सन्देशः_En_Idx = from.m_सन्देशः_En_Idx; }
    शून्य warnResetDefault() { warnStateFrom(default_File_पङ्क्तिः()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ध्रुव { प्रत्यागम सन्देशः_En().enabled(V3_Error_संहिता::I_CELLDEFINE); }
    शून्य celldefineOn(bool flag) { warnOn(V3_Error_संहिता::I_CELLDEFINE, flag); }
    bool coverageOn() ध्रुव { प्रत्यागम सन्देशः_En().enabled(V3_Error_संहिता::I_COVERAGE); }
    शून्य coverageOn(bool flag) { warnOn(V3_Error_संहिता::I_COVERAGE, flag); }
    bool erroringOn() ध्रुव { प्रत्यागम सन्देशः_En().enabled(V3_Error_संहिता::I_ERRORING); }
    शून्य erroringOn(bool flag) { warnOn(V3_Error_संहिता::I_ERRORING, flag); }
    bool tracingOn() ध्रुव { प्रत्यागम सन्देशः_En().enabled(V3_Error_संहिता::I_TRACING); }
    शून्य tracingOn(bool flag) { warnOn(V3_Error_संहिता::I_TRACING, flag); }
    bool timingOn() ध्रुव { प्रत्यागम सन्देशः_En().enabled(V3_Error_संहिता::I_TIMING); }
    शून्य timingOn(bool flag) { warnOn(V3_Error_संहिता::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    स्थिर string command_पङ्क्तिः_Filename() VL_MT_SAFE { प्रत्यागम "<command-line>"; }
    स्थिर string built_अन्तर्_Filename() VL_MT_SAFE { प्रत्यागम "<built-in>"; }
    स्थिर शून्य globalWarnOff(V3_Error_संहिता संहिता, bool turnOff) {
        default_File_पङ्क्तिः().warnOff(संहिता, turnOff);
    }
    स्थिर string globalWarnOffParse(ध्रुव string& msgs, bool turnOff) {
        प्रत्यागम default_File_पङ्क्तिः().warnOffParse(msgs, turnOff);
    }
    स्थिर शून्य file_Name_अङ्कः_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_अङ्कः_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    शून्य modifyStateInherit(ध्रुव File_पङ्क्तिः* fromp) {
        m_सन्देशः_En_Idx = singleton().सन्देशः_En_And(m_सन्देशः_En_Idx, fromp->m_सन्देशः_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    शून्य modifyWarnOff(V3_Error_संहिता संहिता, bool turnOff) { warnOff(संहिता, turnOff); }

    // OPERATORS
    शून्य v3error_अन्तः(std::ostringstream& str, ध्रुव string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    शून्य v3error_अन्तः_Fatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3error_अन्तः(str);
        प्रतिज्ञा(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warn_अधिकम्() ध्रुव VL_REQUIRES(V3Error::s().m_mutex);
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
    bool संक्रियकः==(ध्रुव File_पङ्क्तिः& rhs) ध्रुव {
        प्रत्यागम (m_token_अङ्कः == rhs.m_token_अङ्कः && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn && m_चरमः_Lineno_Adder == rhs.m_चरमः_Lineno_Adder
                && m_चरमः_Column == rhs.m_चरमः_Column && m_filenameno == rhs.m_filenameno
                && m_सन्देशः_En_Idx == rhs.m_सन्देशः_En_Idx);
    }
    bool equal_First_पङ्क्तिः_Col(ध्रुव File_पङ्क्तिः& rhs) ध्रुव {
        प्रत्यागम (m_filenameno == rhs.m_filenameno && m_firstLineno == rhs.m_firstLineno
                && m_firstColumn == rhs.m_firstColumn);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    पूर्णाङ्क operatorCompare(ध्रुव File_पङ्क्तिः& rhs) ध्रुव {
        यदि (m_filenameno != rhs.m_filenameno) प्रत्यागम (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        यदि (m_firstLineno != rhs.m_firstLineno)
            प्रत्यागम (m_firstLineno < rhs.m_firstLineno) ? -1 : 1;
        यदि (m_firstColumn != rhs.m_firstColumn)
            प्रत्यागम (m_firstColumn < rhs.m_firstColumn) ? -1 : 1;
        यदि (m_चरमः_Lineno_Adder != rhs.m_चरमः_Lineno_Adder)
            प्रत्यागम (m_चरमः_Lineno_Adder < rhs.m_चरमः_Lineno_Adder) ? -1 : 1;
        यदि (m_चरमः_Column != rhs.m_चरमः_Column) प्रत्यागम (m_चरमः_Column < rhs.m_चरमः_Column) ? -1 : 1;
        ध्रुव सन्देशः_En_Bit_समुच्चयः& lhs_सन्देशः_En = सन्देशः_En();
        ध्रुव सन्देशः_En_Bit_समुच्चयः& rhs_सन्देशः_En = rhs.सन्देशः_En();
        प्रति (आकार_t i = 0; i < V3_Error_संहिता::_ENUM_MAX; ++i) {
            ध्रुव V3_Error_संहिता संहिता = static_cast<V3_Error_संहिता>(i);
            यदि (lhs_सन्देशः_En.enabled(संहिता) != rhs_सन्देशः_En.enabled(संहिता))
                प्रत्यागम rhs_सन्देशः_En.enabled(संहिता) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        यदि (m_token_अङ्कः != rhs.m_token_अङ्कः) प्रत्यागम (m_token_अङ्कः < rhs.m_token_अङ्कः) ? -1 : 1;
        प्रत्यागम 0;  // (*this) and rhs are equivalent
    }

गुप्त:
    string warnContext() ध्रुव;
    string warnContextParent() ध्रुव VL_REQUIRES(V3Error::s().m_mutex);
    ध्रुव सन्देशः_En_Bit_समुच्चयः& सन्देशः_En() ध्रुव VL_MT_SAFE { प्रत्यागम singleton().सन्देशः_En(m_सन्देशः_En_Idx); }
};
std::ostream& संक्रियकः<<(std::ostream& os, File_पङ्क्तिः* fileline);

#यदि_अन्तः  // Guard
