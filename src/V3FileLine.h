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

#که_نه_تعریف_شوی VERILATOR_V3FILELINE_H_
#تعریف_کړه VERILATOR_V3FILELINE_H_

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

class File_کرښه;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_کرښه_Singleton final {
    ملګری class File_کرښه;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using پیغام_En_سېټ_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class پیغام_En_بټ_سېټ final {
        VError_بټ_سېټ m_کوډ_En;  // Enabled by code directives/metacomments
        VError_بټ_سېټ m_ctrlEn;  // Enabled by control file

    عام:
        تعداد class Subset : uint8_t {
            کوډ_2 = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        پیغام_En_بټ_سېټ() = افتراضي;
        // Create intersection set
        پیغام_En_بټ_سېټ(ثابت پیغام_En_بټ_سېټ& a, ثابت پیغام_En_بټ_سېټ& b)
            : m_کوډ_En{a.m_کوډ_En & b.m_کوډ_En}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        بنية Hash final {
            اندازه_t چلوونکی()(ثابت پیغام_En_بټ_سېټ& توکی_2) ثابت {
                V3Hash hash{توکی_2.m_کوډ_En.hash()};
                hash += توکی_2.m_ctrlEn.hash();
                أرجع hash.ارزښت();
            }
        };

        بنية Equal final {
            bool چلوونکی()(ثابت پیغام_En_بټ_سېټ& a, ثابت پیغام_En_بټ_سېټ& b) ثابت { أرجع a == b; }
        };

        bool چلوونکی==(ثابت پیغام_En_بټ_سېټ& other) ثابت {
            أرجع m_کوډ_En == other.m_کوډ_En && m_ctrlEn == other.m_ctrlEn;
        }

        ثابت VError_بټ_سېټ& getAll(Subset subset) ثابت {
            أرجع subset == Subset::کوډ_2 ? m_کوډ_En : m_ctrlEn;
        }
        bool test(Subset subset, V3_Error_کوډ کوډ) ثابت {
            أرجع subset == Subset::کوډ_2 ? m_کوډ_En.test(کوډ) : m_ctrlEn.test(کوډ);
        }
        فراغ سېټ_All(Subset subset, ثابت VError_بټ_سېټ& bitset) {
            إذا (subset == Subset::کوډ_2) {  // LCOV_EXCL_BR_LINE
                m_کوډ_En = bitset;  // LCOV_EXCL_LINE
            } وإلا {
                m_ctrlEn = bitset;
            }
        }
        فراغ سېټ(Subset subset, V3_Error_کوډ کوډ, bool ارزښت) {
            إذا (subset == Subset::کوډ_2) {
                m_کوډ_En.سېټ(کوډ, ارزښت);
            } وإلا {
                m_ctrlEn.سېټ(کوډ, ارزښت);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3_Error_کوډ کوډ) ثابت { أرجع m_کوډ_En.test(کوډ) && m_ctrlEn.test(کوډ); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ثابت std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_نومونه;  // filename text for each filenameno
    std::deque<V3_Lang_کوډ> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<پیغام_En_بټ_سېټ, پیغام_En_سېټ_Idx_t, پیغام_En_بټ_سېټ::Hash, پیغام_En_بټ_سېټ::Equal>
        m_interned_پیغام_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::ویکټور<پیغام_En_بټ_سېټ> m_interned_پیغام_Ens;

    // CONSTRUCTORS
    File_کرښه_Singleton() = افتراضي;
    ~File_کرښه_Singleton() = افتراضي;

    fileNameIdx_t name_منزل_شمېره(ثابت string& filename);
    ثابت std::string& شمېره_منزل_Name(fileNameIdx_t filenameno) ثابت VL_MT_خوندي {
        أرجع m_نومونه[filenameno];
    }
    V3_Lang_کوډ شمېره_منزل_Lang(fileNameIdx_t filenameno) ثابت { أرجع m_languages[filenameno]; }
    فراغ شمېره_منزل_Lang(fileNameIdx_t filenameno, ثابت V3_Lang_کوډ& l) {
        m_languages[filenameno] = l;
    }
    فراغ clear() {
        m_namemap.clear();
        m_نومونه.clear();
        m_languages.clear();
    }
    فراغ file_Name_عدد_Map_Dump_Json(std::ostream& os);
    ساكن string filenameLetters(fileNameIdx_t fileno) VL_خالص;

    // Add given bitset to the interned bitsets, return interned index
    پیغام_En_سېټ_Idx_t جمع_کړه_پیغام_En_بټ_سېټ(ثابت پیغام_En_بټ_سېټ& بټ_سېټ) VL_MT_خوندي_EXCLUDES(m_mutex);
    // Add index of default bitset
    پیغام_En_سېټ_Idx_t default_پیغام_En_شاخص() VL_MT_خوندي;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    پیغام_En_سېټ_Idx_t پیغام_En_سېټ_بټ(پیغام_En_سېټ_Idx_t سېټ_Idx, پیغام_En_بټ_سېټ::Subset subset, V3_Error_کوډ کوډ,
                              bool ارزښت);
    // Bulk-set all control codes to given bitset
    پیغام_En_سېټ_Idx_t پیغام_سېټ_Ctrl_بټ_سېټ(پیغام_En_سېټ_Idx_t سېټ_Idx, ثابت VError_بټ_سېټ& bitset);
    // Return index to intersection set
    پیغام_En_سېټ_Idx_t پیغام_En_And(پیغام_En_سېټ_Idx_t lhsIdx, پیغام_En_سېټ_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ثابت پیغام_En_بټ_سېټ& پیغام_En(پیغام_En_سېټ_Idx_t idx) ثابت VL_MT_خوندي {
        أرجع m_interned_پیغام_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    ملګری class File_کرښه;
    // MEMBERS
    صحيح m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<اندازه_t> m_ref_شمېر{0};
    std::deque<string> m_کرښې;  // Source text lines
    VFileContent() {
        ساكن صحيح s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = افتراضي;
    // METHODS
    فراغ refInc() { ++m_ref_شمېر; }
    فراغ refDec() {
        إذا (!--m_ref_شمېر) delete this;
    }

عام:
    فراغ ټېل_متن(ثابت string& متن);  // Add arbitrary text (need not be line-by-line)
    string get_کرښه(صحيح lineno) ثابت VL_MT_خوندي;
    string ascii() ثابت { أرجع "ct" + cvt_منزل_Str(m_id); }
};
std::ostream& چلوونکی<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_کرښه final {

    // CONSTANTS
    ساكن constexpr بینښه SHOW_سرچینه_MAX_اوږدتیا = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_کرښه_Singleton::fileNameIdx_t;
    using پیغام_En_سېټ_Idx_t = File_کرښه_Singleton::پیغام_En_سېټ_Idx_t;
    using پیغام_En_بټ_سېټ = File_کرښه_Singleton::پیغام_En_بټ_سېټ;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    پیغام_En_سېټ_Idx_t m_پیغام_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    بینښه m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_عدد = 0;  // Token number in processing order
    صحيح m_لومړی_Lineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_لومړی_ستنه : 24;  // `line corrected token's first column number
    uint32_t m_وروستی_ستنه : 24;  // `line corrected token's last column number
    uint16_t m_وروستی_Lineno_Adder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_کرښه* m_پلار = nullptr;  // Parent line that included this line

محمي:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    ملګری class File_کرښه_Singleton;
    ملګری class V3ParseImp;
    ملګری class V3PreLex;
    ملګری class V3PreProcImp;
    ملګری class V3PreShellImp;

خاص:
    // CONSTRUCTORS
    ساكن File_کرښه_Singleton& singleton() VL_MT_خوندي {
        ساكن File_کرښه_Singleton s;
        أرجع s;
    }
    ساكن File_کرښه& default_File_کرښه() VL_MT_خوندي {
        ساكن File_کرښه s;
        أرجع s;
    }

    File_کرښه()  // Only used for defaultFileLine above
        : m_پیغام_En_Idx{singleton().default_پیغام_En_شاخص()}
        , m_filenameno{singleton().name_منزل_شمېره(File_کرښه::built_دننه_Filename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_لومړی_ستنه{0}
        , m_وروستی_ستنه{0} {}

عام:
    explicit File_کرښه(ثابت string& filename)
        : m_پیغام_En_Idx{default_File_کرښه().m_پیغام_En_Idx}
        , m_filenameno{singleton().name_منزل_شمېره(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_لومړی_ستنه{0}
        , m_وروستی_ستنه{0} {}
    explicit File_کرښه(ثابت File_کرښه& from)
        : m_پیغام_En_Idx{from.m_پیغام_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_عدد{from.m_token_عدد}
        , m_لومړی_Lineno{from.m_لومړی_Lineno}
        , m_لومړی_ستنه{from.m_لومړی_ستنه}
        , m_وروستی_ستنه{from.m_وروستی_ستنه}
        , m_وروستی_Lineno_Adder{from.m_وروستی_Lineno_Adder}
        , m_contentp{from.m_contentp}
        , m_پلار{from.m_پلار} {
        إذا (m_contentp) m_contentp->refInc();
    }
    explicit File_کرښه(File_کرښه* fromp)
        : m_پیغام_En_Idx{fromp->m_پیغام_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_عدد{fromp->m_token_عدد}
        , m_لومړی_Lineno{fromp->m_لومړی_Lineno}
        , m_لومړی_ستنه{fromp->m_لومړی_ستنه}
        , m_وروستی_ستنه{fromp->m_وروستی_ستنه}
        , m_وروستی_Lineno_Adder{fromp->m_وروستی_Lineno_Adder}
        , m_contentp{fromp->m_contentp}
        , m_پلار{fromp->m_پلار} {
        إذا (m_contentp) m_contentp->refInc();
    }
    فراغ applyIgnores();
    File_کرښه* copy_Or_Same_File_کرښه();
    File_کرښه* copy_Or_Same_File_کرښه_Applied();
    ساكن فراغ delete_All_پاتې_شوی();
    ساكن فراغ stats();
    ~File_کرښه();
#که_تعریف_شوی VL_LEAK_CHECKS
    ساكن فراغ* چلوونکی new(اندازه_t اندازه);
    ساكن فراغ چلوونکی delete(فراغ* obj, اندازه_t اندازه);
#د_که_پای
    // METHODS
    فراغ newContent();
    فراغ contentLineno(صحيح عدد) {
        lineno(عدد);
        m_contentLineno = static_cast<بینښه>(عدد);
    }
    فراغ contentLinenoFrom(ثابت File_کرښه* fromp) { m_contentLineno = fromp->m_contentLineno; }
    فراغ lineno(صحيح عدد) {
        m_لومړی_Lineno = عدد;
        m_وروستی_Lineno_Adder = 0;
        m_لومړی_ستنه = 1;
        m_وروستی_ستنه = 1;
    }
    فراغ ستنه_2(صحيح لومړی_ستنه, صحيح وروستی_ستنه) {
        m_لومړی_ستنه = لومړی_ستنه;
        m_وروستی_ستنه = وروستی_ستنه;
    }
    فراغ language(V3_Lang_کوډ lang) ثابت { singleton().شمېره_منزل_Lang(filenameno(), lang); }
    فراغ filename(ثابت string& name) { m_filenameno = singleton().name_منزل_شمېره(name); }
    فراغ پلار(File_کرښه* fileline) { m_پلار = fileline; }
    فراغ کرښه_Directive(ثابت حرف* textp, صحيح& enterExitRef);
    فراغ کرښه_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                            صحيح& enterExitRef);
    فراغ linenoInc() {
        إذا (m_وروستی_Lineno_Adder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_لومړی_Lineno += 65535 + 1;
            m_وروستی_Lineno_Adder = 0;
        } وإلا {
            ++m_وروستی_Lineno_Adder;
        }
        m_وروستی_ستنه = 1;
        ++m_contentLineno;
    }
    فراغ پیل_Token() {
        m_لومړی_Lineno = وروستی_Lineno();
        m_لومړی_ستنه = m_وروستی_ستنه;
        m_وروستی_Lineno_Adder = 0;
    }
    // Advance last line/column based on given text
    فراغ forwardToken(ثابت حرف* textp, اندازه_t اندازه, bool track_کرښې = true);
    uint32_t token_عدد() ثابت VL_MT_خوندي { أرجع m_token_عدد; }
    صحيح لومړی_Lineno() ثابت VL_MT_خوندي { أرجع m_لومړی_Lineno; }
    صحيح لومړی_ستنه() ثابت VL_MT_خوندي { أرجع static_cast<صحيح>(m_لومړی_ستنه); }
    صحيح وروستی_Lineno() ثابت VL_MT_خوندي { أرجع m_لومړی_Lineno + m_وروستی_Lineno_Adder; }
    صحيح وروستی_ستنه() ثابت VL_MT_خوندي { أرجع static_cast<صحيح>(m_وروستی_ستنه); }
    VFileContent* contentp() ثابت { أرجع m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    صحيح lineno() ثابت VL_MT_خوندي { أرجع وروستی_Lineno(); }
    string سرچینه() ثابت VL_MT_خوندي;
    string سرچینه_مختاړی(صحيح منزل_ستنه) ثابت VL_MT_خوندي;
    string pretty_سرچینه() ثابت VL_MT_خوندي;  // Source, w/stripped unprintables and newlines
    File_کرښه* پلار() ثابت VL_MT_خوندي { أرجع m_پلار; }
    V3_Lang_کوډ language() ثابت { أرجع singleton().شمېره_منزل_Lang(filenameno()); }
    string ascii() ثابت VL_MT_خوندي;
    string ascii_کرښه_ستون() ثابت;
    صحيح filenameno() ثابت VL_MT_خوندي { أرجع m_filenameno; }
    ثابت std::string& filename() ثابت VL_MT_خوندي {
        أرجع singleton().شمېره_منزل_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ثابت VL_MT_خوندي { أرجع VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ثابت VL_MT_خوندي {
        أرجع (filename() == command_کرښه_Filename() || filename() == built_دننه_Filename());
    }
    string filenameLetters() ثابت VL_MT_خوندي {
        أرجع File_کرښه_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ثابت VL_MT_خوندي;
    string filebasenameNoExt() ثابت;
    string لومړی_ستنه_Letters() ثابت VL_MT_خوندي;
    string profileFuncname() ثابت;
    string کرښه_Directive_Strg(صحيح enterExit) ثابت;

    // Turn on/off warning messages on this line.
خاص:
    فراغ warn_سېټ(پیغام_En_بټ_سېټ::Subset subset, V3_Error_کوډ کوډ, bool flag) {
        m_پیغام_En_Idx = singleton().پیغام_En_سېټ_بټ(m_پیغام_En_Idx, subset, کوډ, flag);
    }

عام:
    فراغ warnOn(V3_Error_کوډ کوډ, bool flag) { warn_سېټ(پیغام_En_بټ_سېټ::Subset::کوډ_2, کوډ, flag); }
    فراغ warn_سېټ_Ctrl_بټ_سېټ(ثابت VError_بټ_سېټ& bitset) {
        m_پیغام_En_Idx = singleton().پیغام_سېټ_Ctrl_بټ_سېټ(m_پیغام_En_Idx, bitset);
    }
    فراغ warnOff(V3_Error_کوډ کوډ, bool turnOff) { warnOn(کوډ, !turnOff); }
    string warnOffParse(ثابت string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3_Error_کوډ کوډ) ثابت VL_MT_خوندي;
    فراغ warnLintOff(bool turnOff);
    فراغ warnStyleOff(bool turnOff);
    فراغ warnStateFrom(ثابت File_کرښه& from) { m_پیغام_En_Idx = from.m_پیغام_En_Idx; }
    فراغ warnResetDefault() { warnStateFrom(default_File_کرښه()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ثابت { أرجع پیغام_En().enabled(V3_Error_کوډ::I_CELLDEFINE); }
    فراغ celldefineOn(bool flag) { warnOn(V3_Error_کوډ::I_CELLDEFINE, flag); }
    bool coverageOn() ثابت { أرجع پیغام_En().enabled(V3_Error_کوډ::I_COVERAGE); }
    فراغ coverageOn(bool flag) { warnOn(V3_Error_کوډ::I_COVERAGE, flag); }
    bool erroringOn() ثابت { أرجع پیغام_En().enabled(V3_Error_کوډ::I_ERRORING); }
    فراغ erroringOn(bool flag) { warnOn(V3_Error_کوډ::I_ERRORING, flag); }
    bool tracingOn() ثابت { أرجع پیغام_En().enabled(V3_Error_کوډ::I_TRACING); }
    فراغ tracingOn(bool flag) { warnOn(V3_Error_کوډ::I_TRACING, flag); }
    bool timingOn() ثابت { أرجع پیغام_En().enabled(V3_Error_کوډ::I_TIMING); }
    فراغ timingOn(bool flag) { warnOn(V3_Error_کوډ::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ساكن string command_کرښه_Filename() VL_MT_خوندي { أرجع "<command-line>"; }
    ساكن string built_دننه_Filename() VL_MT_خوندي { أرجع "<built-in>"; }
    ساكن فراغ globalWarnOff(V3_Error_کوډ کوډ, bool turnOff) {
        default_File_کرښه().warnOff(کوډ, turnOff);
    }
    ساكن string globalWarnOffParse(ثابت string& msgs, bool turnOff) {
        أرجع default_File_کرښه().warnOffParse(msgs, turnOff);
    }
    ساكن فراغ file_Name_عدد_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_عدد_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    فراغ modifyStateInherit(ثابت File_کرښه* fromp) {
        m_پیغام_En_Idx = singleton().پیغام_En_And(m_پیغام_En_Idx, fromp->m_پیغام_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    فراغ modifyWarnOff(V3_Error_کوډ کوډ, bool turnOff) { warnOff(کوډ, turnOff); }

    // OPERATORS
    فراغ v3error_پای_ټکی(std::ostringstream& str, ثابت string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_پای_ټکی_Fatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3error_پای_ټکی(str);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warn_نور() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        أرجع warnContext() + warn_Context_پلار();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ثابت { أرجع warnContext(); }
    bool چلوونکی==(ثابت File_کرښه& rhs) ثابت {
        أرجع (m_token_عدد == rhs.m_token_عدد && m_لومړی_Lineno == rhs.m_لومړی_Lineno
                && m_لومړی_ستنه == rhs.m_لومړی_ستنه && m_وروستی_Lineno_Adder == rhs.m_وروستی_Lineno_Adder
                && m_وروستی_ستنه == rhs.m_وروستی_ستنه && m_filenameno == rhs.m_filenameno
                && m_پیغام_En_Idx == rhs.m_پیغام_En_Idx);
    }
    bool equal_لومړی_کرښه_ستون(ثابت File_کرښه& rhs) ثابت {
        أرجع (m_filenameno == rhs.m_filenameno && m_لومړی_Lineno == rhs.m_لومړی_Lineno
                && m_لومړی_ستنه == rhs.m_لومړی_ستنه);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    صحيح operatorCompare(ثابت File_کرښه& rhs) ثابت {
        إذا (m_filenameno != rhs.m_filenameno) أرجع (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        إذا (m_لومړی_Lineno != rhs.m_لومړی_Lineno)
            أرجع (m_لومړی_Lineno < rhs.m_لومړی_Lineno) ? -1 : 1;
        إذا (m_لومړی_ستنه != rhs.m_لومړی_ستنه)
            أرجع (m_لومړی_ستنه < rhs.m_لومړی_ستنه) ? -1 : 1;
        إذا (m_وروستی_Lineno_Adder != rhs.m_وروستی_Lineno_Adder)
            أرجع (m_وروستی_Lineno_Adder < rhs.m_وروستی_Lineno_Adder) ? -1 : 1;
        إذا (m_وروستی_ستنه != rhs.m_وروستی_ستنه) أرجع (m_وروستی_ستنه < rhs.m_وروستی_ستنه) ? -1 : 1;
        ثابت پیغام_En_بټ_سېټ& lhs_پیغام_En = پیغام_En();
        ثابت پیغام_En_بټ_سېټ& rhs_پیغام_En = rhs.پیغام_En();
        لكل (اندازه_t i = 0; i < V3_Error_کوډ::_ENUM_MAX; ++i) {
            ثابت V3_Error_کوډ کوډ = static_cast<V3_Error_کوډ>(i);
            إذا (lhs_پیغام_En.enabled(کوډ) != rhs_پیغام_En.enabled(کوډ))
                أرجع rhs_پیغام_En.enabled(کوډ) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        إذا (m_token_عدد != rhs.m_token_عدد) أرجع (m_token_عدد < rhs.m_token_عدد) ? -1 : 1;
        أرجع 0;  // (*this) and rhs are equivalent
    }

خاص:
    string warnContext() ثابت;
    string warn_Context_پلار() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    ثابت پیغام_En_بټ_سېټ& پیغام_En() ثابت VL_MT_خوندي { أرجع singleton().پیغام_En(m_پیغام_En_Idx); }
};
std::ostream& چلوونکی<<(std::ostream& os, File_کرښه* fileline);

#د_که_پای  // Guard
