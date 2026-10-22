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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3FILELINE_H_
#وصف_ڪريو VERILATOR_V3FILELINE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3Hash.h"
#ضمن "V3LangCode.h"
#ضمن "V3Mutex.h"

#ضمن <atomic>
#ضمن <deque>
#ضمن <map>
#ضمن <memory>
#ضمن <set>
#ضمن <sstream>
#ضمن <unordered_map>
#ضمن <vector>

// ######################################################################

class File_سٽ;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_سٽ_Singleton final {
    دوست class File_سٽ;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using نياپو_En_سيٽ_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class نياپو_En_ٻٽو_انگ_سيٽ final {
        VError_ٻٽو_انگ_سيٽ m_ڪوڊ_En;  // Enabled by code directives/metacomments
        VError_ٻٽو_انگ_سيٽ m_ctrlEn;  // Enabled by control file

    عام:
        تعداد class Subset : uint8_t {
            ڪوڊ_2 = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        نياپو_En_ٻٽو_انگ_سيٽ() = افتراضي;
        // Create intersection set
        نياپو_En_ٻٽو_انگ_سيٽ(ثابت نياپو_En_ٻٽو_انگ_سيٽ& a, ثابت نياپو_En_ٻٽو_انگ_سيٽ& b)
            : m_ڪوڊ_En{a.m_ڪوڊ_En & b.m_ڪوڊ_En}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        بنية Hash final {
            سائيز_t عامل()(ثابت نياپو_En_ٻٽو_انگ_سيٽ& شي_2) ثابت {
                V3Hash hash{شي_2.m_ڪوڊ_En.hash()};
                hash += شي_2.m_ctrlEn.hash();
                أرجع hash.قدر();
            }
        };

        بنية Equal final {
            bool عامل()(ثابت نياپو_En_ٻٽو_انگ_سيٽ& a, ثابت نياپو_En_ٻٽو_انگ_سيٽ& b) ثابت { أرجع a == b; }
        };

        bool عامل==(ثابت نياپو_En_ٻٽو_انگ_سيٽ& other) ثابت {
            أرجع m_ڪوڊ_En == other.m_ڪوڊ_En && m_ctrlEn == other.m_ctrlEn;
        }

        ثابت VError_ٻٽو_انگ_سيٽ& getAll(Subset subset) ثابت {
            أرجع subset == Subset::ڪوڊ_2 ? m_ڪوڊ_En : m_ctrlEn;
        }
        bool test(Subset subset, V3_Error_ڪوڊ ڪوڊ) ثابت {
            أرجع subset == Subset::ڪوڊ_2 ? m_ڪوڊ_En.test(ڪوڊ) : m_ctrlEn.test(ڪوڊ);
        }
        فراغ سيٽ_All(Subset subset, ثابت VError_ٻٽو_انگ_سيٽ& bitset) {
            إذا (subset == Subset::ڪوڊ_2) {  // LCOV_EXCL_BR_LINE
                m_ڪوڊ_En = bitset;  // LCOV_EXCL_LINE
            } وإلا {
                m_ctrlEn = bitset;
            }
        }
        فراغ سيٽ(Subset subset, V3_Error_ڪوڊ ڪوڊ, bool قدر) {
            إذا (subset == Subset::ڪوڊ_2) {
                m_ڪوڊ_En.سيٽ(ڪوڊ, قدر);
            } وإلا {
                m_ctrlEn.سيٽ(ڪوڊ, قدر);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3_Error_ڪوڊ ڪوڊ) ثابت { أرجع m_ڪوڊ_En.test(ڪوڊ) && m_ctrlEn.test(ڪوڊ); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ثابت std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_نالا;  // filename text for each filenameno
    std::deque<V3_Lang_ڪوڊ> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<نياپو_En_ٻٽو_انگ_سيٽ, نياپو_En_سيٽ_Idx_t, نياپو_En_ٻٽو_انگ_سيٽ::Hash, نياپو_En_ٻٽو_انگ_سيٽ::Equal>
        m_interned_نياپو_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::ويڪٽر<نياپو_En_ٻٽو_انگ_سيٽ> m_interned_نياپو_Ens;

    // CONSTRUCTORS
    File_سٽ_Singleton() = افتراضي;
    ~File_سٽ_Singleton() = افتراضي;

    fileNameIdx_t name_منزل_عدد(ثابت string& filename);
    ثابت std::string& عدد_منزل_Name(fileNameIdx_t filenameno) ثابت VL_MT_محفوظ {
        أرجع m_نالا[filenameno];
    }
    V3_Lang_ڪوڊ عدد_منزل_Lang(fileNameIdx_t filenameno) ثابت { أرجع m_languages[filenameno]; }
    فراغ عدد_منزل_Lang(fileNameIdx_t filenameno, ثابت V3_Lang_ڪوڊ& l) {
        m_languages[filenameno] = l;
    }
    فراغ clear() {
        m_namemap.clear();
        m_نالا.clear();
        m_languages.clear();
    }
    فراغ file_Name_انگ_Map_Dump_Json(std::ostream& os);
    ساكن string filenameLetters(fileNameIdx_t fileno) VL_خالص;

    // Add given bitset to the interned bitsets, return interned index
    نياپو_En_سيٽ_Idx_t جوڙيو_نياپو_En_ٻٽو_انگ_سيٽ(ثابت نياپو_En_ٻٽو_انگ_سيٽ& ٻٽو_انگ_سيٽ) VL_MT_محفوظ_EXCLUDES(m_mutex);
    // Add index of default bitset
    نياپو_En_سيٽ_Idx_t default_نياپو_En_اشاريو() VL_MT_محفوظ;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    نياپو_En_سيٽ_Idx_t نياپو_En_سيٽ_ٻٽو_انگ(نياپو_En_سيٽ_Idx_t سيٽ_Idx, نياپو_En_ٻٽو_انگ_سيٽ::Subset subset, V3_Error_ڪوڊ ڪوڊ,
                              bool قدر);
    // Bulk-set all control codes to given bitset
    نياپو_En_سيٽ_Idx_t نياپو_سيٽ_Ctrl_ٻٽو_انگ_سيٽ(نياپو_En_سيٽ_Idx_t سيٽ_Idx, ثابت VError_ٻٽو_انگ_سيٽ& bitset);
    // Return index to intersection set
    نياپو_En_سيٽ_Idx_t نياپو_En_And(نياپو_En_سيٽ_Idx_t lhsIdx, نياپو_En_سيٽ_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ثابت نياپو_En_ٻٽو_انگ_سيٽ& نياپو_En(نياپو_En_سيٽ_Idx_t idx) ثابت VL_MT_محفوظ {
        أرجع m_interned_نياپو_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    دوست class File_سٽ;
    // MEMBERS
    صحيح m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<سائيز_t> m_ref_ڳڻپ{0};
    std::deque<string> m_سٽون;  // Source text lines
    VFileContent() {
        ساكن صحيح s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = افتراضي;
    // METHODS
    فراغ refInc() { ++m_ref_ڳڻپ; }
    فراغ refDec() {
        إذا (!--m_ref_ڳڻپ) delete this;
    }

عام:
    فراغ ڌڪو_متن(ثابت string& متن);  // Add arbitrary text (need not be line-by-line)
    string get_سٽ(صحيح lineno) ثابت VL_MT_محفوظ;
    string ascii() ثابت { أرجع "ct" + cvt_منزل_Str(m_id); }
};
std::ostream& عامل<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_سٽ final {

    // CONSTANTS
    ساكن constexpr بنانشان SHOW_ذريعو_MAX_ڊيگهائي = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_سٽ_Singleton::fileNameIdx_t;
    using نياپو_En_سيٽ_Idx_t = File_سٽ_Singleton::نياپو_En_سيٽ_Idx_t;
    using نياپو_En_ٻٽو_انگ_سيٽ = File_سٽ_Singleton::نياپو_En_ٻٽو_انگ_سيٽ;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    نياپو_En_سيٽ_Idx_t m_نياپو_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    بنانشان m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_انگ = 0;  // Token number in processing order
    صحيح m_پهريون_Lineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_پهريون_ڪالم : 24;  // `line corrected token's first column number
    uint32_t m_آخري_ڪالم : 24;  // `line corrected token's last column number
    uint16_t m_آخري_Lineno_Adder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_سٽ* m_والد = nullptr;  // Parent line that included this line

محمي:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    دوست class File_سٽ_Singleton;
    دوست class V3ParseImp;
    دوست class V3PreLex;
    دوست class V3PreProcImp;
    دوست class V3PreShellImp;

خاص:
    // CONSTRUCTORS
    ساكن File_سٽ_Singleton& singleton() VL_MT_محفوظ {
        ساكن File_سٽ_Singleton s;
        أرجع s;
    }
    ساكن File_سٽ& default_File_سٽ() VL_MT_محفوظ {
        ساكن File_سٽ s;
        أرجع s;
    }

    File_سٽ()  // Only used for defaultFileLine above
        : m_نياپو_En_Idx{singleton().default_نياپو_En_اشاريو()}
        , m_filenameno{singleton().name_منزل_عدد(File_سٽ::built_اندريون_Filename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_پهريون_ڪالم{0}
        , m_آخري_ڪالم{0} {}

عام:
    explicit File_سٽ(ثابت string& filename)
        : m_نياپو_En_Idx{default_File_سٽ().m_نياپو_En_Idx}
        , m_filenameno{singleton().name_منزل_عدد(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_پهريون_ڪالم{0}
        , m_آخري_ڪالم{0} {}
    explicit File_سٽ(ثابت File_سٽ& from)
        : m_نياپو_En_Idx{from.m_نياپو_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_انگ{from.m_token_انگ}
        , m_پهريون_Lineno{from.m_پهريون_Lineno}
        , m_پهريون_ڪالم{from.m_پهريون_ڪالم}
        , m_آخري_ڪالم{from.m_آخري_ڪالم}
        , m_آخري_Lineno_Adder{from.m_آخري_Lineno_Adder}
        , m_contentp{from.m_contentp}
        , m_والد{from.m_والد} {
        إذا (m_contentp) m_contentp->refInc();
    }
    explicit File_سٽ(File_سٽ* fromp)
        : m_نياپو_En_Idx{fromp->m_نياپو_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_انگ{fromp->m_token_انگ}
        , m_پهريون_Lineno{fromp->m_پهريون_Lineno}
        , m_پهريون_ڪالم{fromp->m_پهريون_ڪالم}
        , m_آخري_ڪالم{fromp->m_آخري_ڪالم}
        , m_آخري_Lineno_Adder{fromp->m_آخري_Lineno_Adder}
        , m_contentp{fromp->m_contentp}
        , m_والد{fromp->m_والد} {
        إذا (m_contentp) m_contentp->refInc();
    }
    فراغ applyIgnores();
    File_سٽ* copy_Or_Same_File_سٽ();
    File_سٽ* copy_Or_Same_File_سٽ_Applied();
    ساكن فراغ delete_All_بچيل();
    ساكن فراغ stats();
    ~File_سٽ();
#جيڪڏهن_وصف_ٿيل VL_LEAK_CHECKS
    ساكن فراغ* عامل new(سائيز_t سائيز);
    ساكن فراغ عامل delete(فراغ* obj, سائيز_t سائيز);
#جيڪڏهن_پورو
    // METHODS
    فراغ newContent();
    فراغ contentLineno(صحيح انگ) {
        lineno(انگ);
        m_contentLineno = static_cast<بنانشان>(انگ);
    }
    فراغ contentLinenoFrom(ثابت File_سٽ* fromp) { m_contentLineno = fromp->m_contentLineno; }
    فراغ lineno(صحيح انگ) {
        m_پهريون_Lineno = انگ;
        m_آخري_Lineno_Adder = 0;
        m_پهريون_ڪالم = 1;
        m_آخري_ڪالم = 1;
    }
    فراغ ڪالم_2(صحيح پهريون_ڪالم, صحيح آخري_ڪالم) {
        m_پهريون_ڪالم = پهريون_ڪالم;
        m_آخري_ڪالم = آخري_ڪالم;
    }
    فراغ language(V3_Lang_ڪوڊ lang) ثابت { singleton().عدد_منزل_Lang(filenameno(), lang); }
    فراغ filename(ثابت string& name) { m_filenameno = singleton().name_منزل_عدد(name); }
    فراغ والد(File_سٽ* fileline) { m_والد = fileline; }
    فراغ سٽ_Directive(ثابت حرف* textp, صحيح& enterExitRef);
    فراغ سٽ_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                            صحيح& enterExitRef);
    فراغ linenoInc() {
        إذا (m_آخري_Lineno_Adder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_پهريون_Lineno += 65535 + 1;
            m_آخري_Lineno_Adder = 0;
        } وإلا {
            ++m_آخري_Lineno_Adder;
        }
        m_آخري_ڪالم = 1;
        ++m_contentLineno;
    }
    فراغ شروعات_Token() {
        m_پهريون_Lineno = آخري_Lineno();
        m_پهريون_ڪالم = m_آخري_ڪالم;
        m_آخري_Lineno_Adder = 0;
    }
    // Advance last line/column based on given text
    فراغ forwardToken(ثابت حرف* textp, سائيز_t سائيز, bool track_سٽون = true);
    uint32_t token_انگ() ثابت VL_MT_محفوظ { أرجع m_token_انگ; }
    صحيح پهريون_Lineno() ثابت VL_MT_محفوظ { أرجع m_پهريون_Lineno; }
    صحيح پهريون_ڪالم() ثابت VL_MT_محفوظ { أرجع static_cast<صحيح>(m_پهريون_ڪالم); }
    صحيح آخري_Lineno() ثابت VL_MT_محفوظ { أرجع m_پهريون_Lineno + m_آخري_Lineno_Adder; }
    صحيح آخري_ڪالم() ثابت VL_MT_محفوظ { أرجع static_cast<صحيح>(m_آخري_ڪالم); }
    VFileContent* contentp() ثابت { أرجع m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    صحيح lineno() ثابت VL_MT_محفوظ { أرجع آخري_Lineno(); }
    string ذريعو() ثابت VL_MT_محفوظ;
    string ذريعو_اڳياڙي(صحيح منزل_ڪالم) ثابت VL_MT_محفوظ;
    string pretty_ذريعو() ثابت VL_MT_محفوظ;  // Source, w/stripped unprintables and newlines
    File_سٽ* والد() ثابت VL_MT_محفوظ { أرجع m_والد; }
    V3_Lang_ڪوڊ language() ثابت { أرجع singleton().عدد_منزل_Lang(filenameno()); }
    string ascii() ثابت VL_MT_محفوظ;
    string ascii_سٽ_کانو() ثابت;
    صحيح filenameno() ثابت VL_MT_محفوظ { أرجع m_filenameno; }
    ثابت std::string& filename() ثابت VL_MT_محفوظ {
        أرجع singleton().عدد_منزل_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ثابت VL_MT_محفوظ { أرجع VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ثابت VL_MT_محفوظ {
        أرجع (filename() == command_سٽ_Filename() || filename() == built_اندريون_Filename());
    }
    string filenameLetters() ثابت VL_MT_محفوظ {
        أرجع File_سٽ_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ثابت VL_MT_محفوظ;
    string filebasenameNoExt() ثابت;
    string پهريون_ڪالم_Letters() ثابت VL_MT_محفوظ;
    string profileFuncname() ثابت;
    string سٽ_Directive_Strg(صحيح enterExit) ثابت;

    // Turn on/off warning messages on this line.
خاص:
    فراغ warn_سيٽ(نياپو_En_ٻٽو_انگ_سيٽ::Subset subset, V3_Error_ڪوڊ ڪوڊ, bool flag) {
        m_نياپو_En_Idx = singleton().نياپو_En_سيٽ_ٻٽو_انگ(m_نياپو_En_Idx, subset, ڪوڊ, flag);
    }

عام:
    فراغ warnOn(V3_Error_ڪوڊ ڪوڊ, bool flag) { warn_سيٽ(نياپو_En_ٻٽو_انگ_سيٽ::Subset::ڪوڊ_2, ڪوڊ, flag); }
    فراغ warn_سيٽ_Ctrl_ٻٽو_انگ_سيٽ(ثابت VError_ٻٽو_انگ_سيٽ& bitset) {
        m_نياپو_En_Idx = singleton().نياپو_سيٽ_Ctrl_ٻٽو_انگ_سيٽ(m_نياپو_En_Idx, bitset);
    }
    فراغ warnOff(V3_Error_ڪوڊ ڪوڊ, bool turnOff) { warnOn(ڪوڊ, !turnOff); }
    string warnOffParse(ثابت string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3_Error_ڪوڊ ڪوڊ) ثابت VL_MT_محفوظ;
    فراغ warnLintOff(bool turnOff);
    فراغ warnStyleOff(bool turnOff);
    فراغ warnStateFrom(ثابت File_سٽ& from) { m_نياپو_En_Idx = from.m_نياپو_En_Idx; }
    فراغ warnResetDefault() { warnStateFrom(default_File_سٽ()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ثابت { أرجع نياپو_En().enabled(V3_Error_ڪوڊ::I_CELLDEFINE); }
    فراغ celldefineOn(bool flag) { warnOn(V3_Error_ڪوڊ::I_CELLDEFINE, flag); }
    bool coverageOn() ثابت { أرجع نياپو_En().enabled(V3_Error_ڪوڊ::I_COVERAGE); }
    فراغ coverageOn(bool flag) { warnOn(V3_Error_ڪوڊ::I_COVERAGE, flag); }
    bool erroringOn() ثابت { أرجع نياپو_En().enabled(V3_Error_ڪوڊ::I_ERRORING); }
    فراغ erroringOn(bool flag) { warnOn(V3_Error_ڪوڊ::I_ERRORING, flag); }
    bool tracingOn() ثابت { أرجع نياپو_En().enabled(V3_Error_ڪوڊ::I_TRACING); }
    فراغ tracingOn(bool flag) { warnOn(V3_Error_ڪوڊ::I_TRACING, flag); }
    bool timingOn() ثابت { أرجع نياپو_En().enabled(V3_Error_ڪوڊ::I_TIMING); }
    فراغ timingOn(bool flag) { warnOn(V3_Error_ڪوڊ::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ساكن string command_سٽ_Filename() VL_MT_محفوظ { أرجع "<command-line>"; }
    ساكن string built_اندريون_Filename() VL_MT_محفوظ { أرجع "<built-in>"; }
    ساكن فراغ globalWarnOff(V3_Error_ڪوڊ ڪوڊ, bool turnOff) {
        default_File_سٽ().warnOff(ڪوڊ, turnOff);
    }
    ساكن string globalWarnOffParse(ثابت string& msgs, bool turnOff) {
        أرجع default_File_سٽ().warnOffParse(msgs, turnOff);
    }
    ساكن فراغ file_Name_انگ_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_انگ_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    فراغ modifyStateInherit(ثابت File_سٽ* fromp) {
        m_نياپو_En_Idx = singleton().نياپو_En_And(m_نياپو_En_Idx, fromp->m_نياپو_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    فراغ modifyWarnOff(V3_Error_ڪوڊ ڪوڊ, bool turnOff) { warnOff(ڪوڊ, turnOff); }

    // OPERATORS
    فراغ v3error_پڇاڙي(std::ostringstream& str, ثابت string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_پڇاڙي_Fatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3error_پڇاڙي(str);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warn_وڌيڪ() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        أرجع warnContext() + warn_Context_والد();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ثابت { أرجع warnContext(); }
    bool عامل==(ثابت File_سٽ& rhs) ثابت {
        أرجع (m_token_انگ == rhs.m_token_انگ && m_پهريون_Lineno == rhs.m_پهريون_Lineno
                && m_پهريون_ڪالم == rhs.m_پهريون_ڪالم && m_آخري_Lineno_Adder == rhs.m_آخري_Lineno_Adder
                && m_آخري_ڪالم == rhs.m_آخري_ڪالم && m_filenameno == rhs.m_filenameno
                && m_نياپو_En_Idx == rhs.m_نياپو_En_Idx);
    }
    bool equal_پهريون_سٽ_کانو(ثابت File_سٽ& rhs) ثابت {
        أرجع (m_filenameno == rhs.m_filenameno && m_پهريون_Lineno == rhs.m_پهريون_Lineno
                && m_پهريون_ڪالم == rhs.m_پهريون_ڪالم);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    صحيح operatorCompare(ثابت File_سٽ& rhs) ثابت {
        إذا (m_filenameno != rhs.m_filenameno) أرجع (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        إذا (m_پهريون_Lineno != rhs.m_پهريون_Lineno)
            أرجع (m_پهريون_Lineno < rhs.m_پهريون_Lineno) ? -1 : 1;
        إذا (m_پهريون_ڪالم != rhs.m_پهريون_ڪالم)
            أرجع (m_پهريون_ڪالم < rhs.m_پهريون_ڪالم) ? -1 : 1;
        إذا (m_آخري_Lineno_Adder != rhs.m_آخري_Lineno_Adder)
            أرجع (m_آخري_Lineno_Adder < rhs.m_آخري_Lineno_Adder) ? -1 : 1;
        إذا (m_آخري_ڪالم != rhs.m_آخري_ڪالم) أرجع (m_آخري_ڪالم < rhs.m_آخري_ڪالم) ? -1 : 1;
        ثابت نياپو_En_ٻٽو_انگ_سيٽ& lhs_نياپو_En = نياپو_En();
        ثابت نياپو_En_ٻٽو_انگ_سيٽ& rhs_نياپو_En = rhs.نياپو_En();
        لكل (سائيز_t i = 0; i < V3_Error_ڪوڊ::_ENUM_MAX; ++i) {
            ثابت V3_Error_ڪوڊ ڪوڊ = static_cast<V3_Error_ڪوڊ>(i);
            إذا (lhs_نياپو_En.enabled(ڪوڊ) != rhs_نياپو_En.enabled(ڪوڊ))
                أرجع rhs_نياپو_En.enabled(ڪوڊ) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        إذا (m_token_انگ != rhs.m_token_انگ) أرجع (m_token_انگ < rhs.m_token_انگ) ? -1 : 1;
        أرجع 0;  // (*this) and rhs are equivalent
    }

خاص:
    string warnContext() ثابت;
    string warn_Context_والد() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    ثابت نياپو_En_ٻٽو_انگ_سيٽ& نياپو_En() ثابت VL_MT_محفوظ { أرجع singleton().نياپو_En(m_نياپو_En_Idx); }
};
std::ostream& عامل<<(std::ostream& os, File_سٽ* fileline);

#جيڪڏهن_پورو  // Guard
