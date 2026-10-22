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

#اگر_غیر_تعریف_شدہ VERILATOR_V3FILELINE_H_
#تعریف_کرو VERILATOR_V3FILELINE_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3Hash.h"
#شامل "V3LangCode.h"
#شامل "V3Mutex.h"

#شامل <atomic>
#شامل <deque>
#شامل <map>
#شامل <memory>
#شامل <set>
#شامل <sstream>
#شامل <unordered_map>
#شامل <vector>

// ######################################################################

class File_سطر;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_سطر_Singleton final {
    دوست class File_سطر;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using پیغام_En_سیٹ_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class پیغام_En_ثنائی_ہندسہ_سیٹ final {
        VError_ثنائی_ہندسہ_سیٹ m_رمز_En;  // Enabled by code directives/metacomments
        VError_ثنائی_ہندسہ_سیٹ m_ctrlEn;  // Enabled by control file

    عوامی:
        شمار class Subset : uint8_t {
            رمز_2 = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        پیغام_En_ثنائی_ہندسہ_سیٹ() = طےشدہ;
        // Create intersection set
        پیغام_En_ثنائی_ہندسہ_سیٹ(ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& a, ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& b)
            : m_رمز_En{a.m_رمز_En & b.m_رمز_En}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        ساخت Hash final {
            سائز_t عامل()(ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& شے_3) ثابت {
                V3Hash hash{شے_3.m_رمز_En.hash()};
                hash += شے_3.m_ctrlEn.hash();
                واپس hash.قدر();
            }
        };

        ساخت Equal final {
            bool عامل()(ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& a, ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& b) ثابت { واپس a == b; }
        };

        bool عامل==(ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& other) ثابت {
            واپس m_رمز_En == other.m_رمز_En && m_ctrlEn == other.m_ctrlEn;
        }

        ثابت VError_ثنائی_ہندسہ_سیٹ& getAll(Subset subset) ثابت {
            واپس subset == Subset::رمز_2 ? m_رمز_En : m_ctrlEn;
        }
        bool test(Subset subset, V3_Error_رمز رمز) ثابت {
            واپس subset == Subset::رمز_2 ? m_رمز_En.test(رمز) : m_ctrlEn.test(رمز);
        }
        خالی سیٹ_All(Subset subset, ثابت VError_ثنائی_ہندسہ_سیٹ& bitset) {
            اگر (subset == Subset::رمز_2) {  // LCOV_EXCL_BR_LINE
                m_رمز_En = bitset;  // LCOV_EXCL_LINE
            } ورنہ {
                m_ctrlEn = bitset;
            }
        }
        خالی سیٹ(Subset subset, V3_Error_رمز رمز, bool قدر) {
            اگر (subset == Subset::رمز_2) {
                m_رمز_En.سیٹ(رمز, قدر);
            } ورنہ {
                m_ctrlEn.سیٹ(رمز, قدر);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3_Error_رمز رمز) ثابت { واپس m_رمز_En.test(رمز) && m_ctrlEn.test(رمز); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ثابت std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_نام;  // filename text for each filenameno
    std::deque<V3_Lang_رمز> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<پیغام_En_ثنائی_ہندسہ_سیٹ, پیغام_En_سیٹ_Idx_t, پیغام_En_ثنائی_ہندسہ_سیٹ::Hash, پیغام_En_ثنائی_ہندسہ_سیٹ::Equal>
        m_interned_پیغام_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::سمتیہ<پیغام_En_ثنائی_ہندسہ_سیٹ> m_interned_پیغام_Ens;

    // CONSTRUCTORS
    File_سطر_Singleton() = طےشدہ;
    ~File_سطر_Singleton() = طےشدہ;

    fileNameIdx_t name_مقصد_نمبر(ثابت string& filename);
    ثابت std::string& نمبر_مقصد_Name(fileNameIdx_t filenameno) ثابت VL_MT_محفوظ {
        واپس m_نام[filenameno];
    }
    V3_Lang_رمز نمبر_مقصد_Lang(fileNameIdx_t filenameno) ثابت { واپس m_languages[filenameno]; }
    خالی نمبر_مقصد_Lang(fileNameIdx_t filenameno, ثابت V3_Lang_رمز& l) {
        m_languages[filenameno] = l;
    }
    خالی clear() {
        m_namemap.clear();
        m_نام.clear();
        m_languages.clear();
    }
    خالی file_Name_عدد_Map_Dump_Json(std::ostream& os);
    ساکن string filenameLetters(fileNameIdx_t fileno) VL_خالص;

    // Add given bitset to the interned bitsets, return interned index
    پیغام_En_سیٹ_Idx_t جوڑو_پیغام_En_ثنائی_ہندسہ_سیٹ(ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& ثنائی_ہندسہ_سیٹ) VL_MT_محفوظ_EXCLUDES(m_mutex);
    // Add index of default bitset
    پیغام_En_سیٹ_Idx_t default_پیغام_En_اشاریہ() VL_MT_محفوظ;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    پیغام_En_سیٹ_Idx_t پیغام_En_سیٹ_ثنائی_ہندسہ(پیغام_En_سیٹ_Idx_t سیٹ_Idx, پیغام_En_ثنائی_ہندسہ_سیٹ::Subset subset, V3_Error_رمز رمز,
                              bool قدر);
    // Bulk-set all control codes to given bitset
    پیغام_En_سیٹ_Idx_t پیغام_سیٹ_Ctrl_ثنائی_ہندسہ_سیٹ(پیغام_En_سیٹ_Idx_t سیٹ_Idx, ثابت VError_ثنائی_ہندسہ_سیٹ& bitset);
    // Return index to intersection set
    پیغام_En_سیٹ_Idx_t پیغام_En_And(پیغام_En_سیٹ_Idx_t lhsIdx, پیغام_En_سیٹ_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& پیغام_En(پیغام_En_سیٹ_Idx_t idx) ثابت VL_MT_محفوظ {
        واپس m_interned_پیغام_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    دوست class File_سطر;
    // MEMBERS
    صحیح m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<سائز_t> m_ref_گنتی{0};
    std::deque<string> m_سطریں;  // Source text lines
    VFileContent() {
        ساکن صحیح s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = طےشدہ;
    // METHODS
    خالی refInc() { ++m_ref_گنتی; }
    خالی refDec() {
        اگر (!--m_ref_گنتی) delete this;
    }

عوامی:
    خالی ڈالو_متن(ثابت string& متن);  // Add arbitrary text (need not be line-by-line)
    string get_سطر(صحیح lineno) ثابت VL_MT_محفوظ;
    string ascii() ثابت { واپس "ct" + cvt_مقصد_Str(m_id); }
};
std::ostream& عامل<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_سطر final {

    // CONSTANTS
    ساکن constexpr غیرعلامتی SHOW_ماخذ_MAX_لمبائی = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_سطر_Singleton::fileNameIdx_t;
    using پیغام_En_سیٹ_Idx_t = File_سطر_Singleton::پیغام_En_سیٹ_Idx_t;
    using پیغام_En_ثنائی_ہندسہ_سیٹ = File_سطر_Singleton::پیغام_En_ثنائی_ہندسہ_سیٹ;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    پیغام_En_سیٹ_Idx_t m_پیغام_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    غیرعلامتی m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_عدد = 0;  // Token number in processing order
    صحیح m_پہلا_Lineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_پہلا_عمود : 24;  // `line corrected token's first column number
    uint32_t m_آخری_عمود : 24;  // `line corrected token's last column number
    uint16_t m_آخری_Lineno_Adder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_سطر* m_والد = nullptr;  // Parent line that included this line

محفوظ:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    دوست class File_سطر_Singleton;
    دوست class V3ParseImp;
    دوست class V3PreLex;
    دوست class V3PreProcImp;
    دوست class V3PreShellImp;

نجی:
    // CONSTRUCTORS
    ساکن File_سطر_Singleton& singleton() VL_MT_محفوظ {
        ساکن File_سطر_Singleton s;
        واپس s;
    }
    ساکن File_سطر& default_File_سطر() VL_MT_محفوظ {
        ساکن File_سطر s;
        واپس s;
    }

    File_سطر()  // Only used for defaultFileLine above
        : m_پیغام_En_Idx{singleton().default_پیغام_En_اشاریہ()}
        , m_filenameno{singleton().name_مقصد_نمبر(File_سطر::built_اندر_Filename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_پہلا_عمود{0}
        , m_آخری_عمود{0} {}

عوامی:
    explicit File_سطر(ثابت string& filename)
        : m_پیغام_En_Idx{default_File_سطر().m_پیغام_En_Idx}
        , m_filenameno{singleton().name_مقصد_نمبر(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_پہلا_عمود{0}
        , m_آخری_عمود{0} {}
    explicit File_سطر(ثابت File_سطر& from)
        : m_پیغام_En_Idx{from.m_پیغام_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_عدد{from.m_token_عدد}
        , m_پہلا_Lineno{from.m_پہلا_Lineno}
        , m_پہلا_عمود{from.m_پہلا_عمود}
        , m_آخری_عمود{from.m_آخری_عمود}
        , m_آخری_Lineno_Adder{from.m_آخری_Lineno_Adder}
        , m_contentp{from.m_contentp}
        , m_والد{from.m_والد} {
        اگر (m_contentp) m_contentp->refInc();
    }
    explicit File_سطر(File_سطر* fromp)
        : m_پیغام_En_Idx{fromp->m_پیغام_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_عدد{fromp->m_token_عدد}
        , m_پہلا_Lineno{fromp->m_پہلا_Lineno}
        , m_پہلا_عمود{fromp->m_پہلا_عمود}
        , m_آخری_عمود{fromp->m_آخری_عمود}
        , m_آخری_Lineno_Adder{fromp->m_آخری_Lineno_Adder}
        , m_contentp{fromp->m_contentp}
        , m_والد{fromp->m_والد} {
        اگر (m_contentp) m_contentp->refInc();
    }
    خالی applyIgnores();
    File_سطر* copy_Or_Same_File_سطر();
    File_سطر* copy_Or_Same_File_سطر_Applied();
    ساکن خالی delete_All_بقایا();
    ساکن خالی stats();
    ~File_سطر();
#اگر_تعریف_شدہ VL_LEAK_CHECKS
    ساکن خالی* عامل new(سائز_t سائز);
    ساکن خالی عامل delete(خالی* obj, سائز_t سائز);
#اگر_ختم
    // METHODS
    خالی newContent();
    خالی contentLineno(صحیح عدد) {
        lineno(عدد);
        m_contentLineno = static_cast<غیرعلامتی>(عدد);
    }
    خالی contentLinenoFrom(ثابت File_سطر* fromp) { m_contentLineno = fromp->m_contentLineno; }
    خالی lineno(صحیح عدد) {
        m_پہلا_Lineno = عدد;
        m_آخری_Lineno_Adder = 0;
        m_پہلا_عمود = 1;
        m_آخری_عمود = 1;
    }
    خالی عمود_2(صحیح پہلا_عمود, صحیح آخری_عمود) {
        m_پہلا_عمود = پہلا_عمود;
        m_آخری_عمود = آخری_عمود;
    }
    خالی language(V3_Lang_رمز lang) ثابت { singleton().نمبر_مقصد_Lang(filenameno(), lang); }
    خالی filename(ثابت string& name) { m_filenameno = singleton().name_مقصد_نمبر(name); }
    خالی والد(File_سطر* fileline) { m_والد = fileline; }
    خالی سطر_Directive(ثابت حرف* textp, صحیح& enterExitRef);
    خالی سطر_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحیح& linenoRef,
                            صحیح& enterExitRef);
    خالی linenoInc() {
        اگر (m_آخری_Lineno_Adder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_پہلا_Lineno += 65535 + 1;
            m_آخری_Lineno_Adder = 0;
        } ورنہ {
            ++m_آخری_Lineno_Adder;
        }
        m_آخری_عمود = 1;
        ++m_contentLineno;
    }
    خالی آغاز_Token() {
        m_پہلا_Lineno = آخری_Lineno();
        m_پہلا_عمود = m_آخری_عمود;
        m_آخری_Lineno_Adder = 0;
    }
    // Advance last line/column based on given text
    خالی forwardToken(ثابت حرف* textp, سائز_t سائز, bool track_سطریں = true);
    uint32_t token_عدد() ثابت VL_MT_محفوظ { واپس m_token_عدد; }
    صحیح پہلا_Lineno() ثابت VL_MT_محفوظ { واپس m_پہلا_Lineno; }
    صحیح پہلا_عمود() ثابت VL_MT_محفوظ { واپس static_cast<صحیح>(m_پہلا_عمود); }
    صحیح آخری_Lineno() ثابت VL_MT_محفوظ { واپس m_پہلا_Lineno + m_آخری_Lineno_Adder; }
    صحیح آخری_عمود() ثابت VL_MT_محفوظ { واپس static_cast<صحیح>(m_آخری_عمود); }
    VFileContent* contentp() ثابت { واپس m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    صحیح lineno() ثابت VL_MT_محفوظ { واپس آخری_Lineno(); }
    string ماخذ() ثابت VL_MT_محفوظ;
    string ماخذ_سابقہ(صحیح مقصد_عمود) ثابت VL_MT_محفوظ;
    string pretty_ماخذ() ثابت VL_MT_محفوظ;  // Source, w/stripped unprintables and newlines
    File_سطر* والد() ثابت VL_MT_محفوظ { واپس m_والد; }
    V3_Lang_رمز language() ثابت { واپس singleton().نمبر_مقصد_Lang(filenameno()); }
    string ascii() ثابت VL_MT_محفوظ;
    string ascii_سطر_ستون() ثابت;
    صحیح filenameno() ثابت VL_MT_محفوظ { واپس m_filenameno; }
    ثابت std::string& filename() ثابت VL_MT_محفوظ {
        واپس singleton().نمبر_مقصد_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ثابت VL_MT_محفوظ { واپس VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ثابت VL_MT_محفوظ {
        واپس (filename() == command_سطر_Filename() || filename() == built_اندر_Filename());
    }
    string filenameLetters() ثابت VL_MT_محفوظ {
        واپس File_سطر_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ثابت VL_MT_محفوظ;
    string filebasenameNoExt() ثابت;
    string پہلا_عمود_Letters() ثابت VL_MT_محفوظ;
    string profileFuncname() ثابت;
    string سطر_Directive_Strg(صحیح enterExit) ثابت;

    // Turn on/off warning messages on this line.
نجی:
    خالی warn_سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ::Subset subset, V3_Error_رمز رمز, bool flag) {
        m_پیغام_En_Idx = singleton().پیغام_En_سیٹ_ثنائی_ہندسہ(m_پیغام_En_Idx, subset, رمز, flag);
    }

عوامی:
    خالی warnOn(V3_Error_رمز رمز, bool flag) { warn_سیٹ(پیغام_En_ثنائی_ہندسہ_سیٹ::Subset::رمز_2, رمز, flag); }
    خالی warn_سیٹ_Ctrl_ثنائی_ہندسہ_سیٹ(ثابت VError_ثنائی_ہندسہ_سیٹ& bitset) {
        m_پیغام_En_Idx = singleton().پیغام_سیٹ_Ctrl_ثنائی_ہندسہ_سیٹ(m_پیغام_En_Idx, bitset);
    }
    خالی warnOff(V3_Error_رمز رمز, bool turnOff) { warnOn(رمز, !turnOff); }
    string warnOffParse(ثابت string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3_Error_رمز رمز) ثابت VL_MT_محفوظ;
    خالی warnLintOff(bool turnOff);
    خالی warnStyleOff(bool turnOff);
    خالی warnStateFrom(ثابت File_سطر& from) { m_پیغام_En_Idx = from.m_پیغام_En_Idx; }
    خالی warnResetDefault() { warnStateFrom(default_File_سطر()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ثابت { واپس پیغام_En().enabled(V3_Error_رمز::I_CELLDEFINE); }
    خالی celldefineOn(bool flag) { warnOn(V3_Error_رمز::I_CELLDEFINE, flag); }
    bool coverageOn() ثابت { واپس پیغام_En().enabled(V3_Error_رمز::I_COVERAGE); }
    خالی coverageOn(bool flag) { warnOn(V3_Error_رمز::I_COVERAGE, flag); }
    bool erroringOn() ثابت { واپس پیغام_En().enabled(V3_Error_رمز::I_ERRORING); }
    خالی erroringOn(bool flag) { warnOn(V3_Error_رمز::I_ERRORING, flag); }
    bool tracingOn() ثابت { واپس پیغام_En().enabled(V3_Error_رمز::I_TRACING); }
    خالی tracingOn(bool flag) { warnOn(V3_Error_رمز::I_TRACING, flag); }
    bool timingOn() ثابت { واپس پیغام_En().enabled(V3_Error_رمز::I_TIMING); }
    خالی timingOn(bool flag) { warnOn(V3_Error_رمز::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ساکن string command_سطر_Filename() VL_MT_محفوظ { واپس "<command-line>"; }
    ساکن string built_اندر_Filename() VL_MT_محفوظ { واپس "<built-in>"; }
    ساکن خالی globalWarnOff(V3_Error_رمز رمز, bool turnOff) {
        default_File_سطر().warnOff(رمز, turnOff);
    }
    ساکن string globalWarnOffParse(ثابت string& msgs, bool turnOff) {
        واپس default_File_سطر().warnOffParse(msgs, turnOff);
    }
    ساکن خالی file_Name_عدد_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_عدد_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    خالی modifyStateInherit(ثابت File_سطر* fromp) {
        m_پیغام_En_Idx = singleton().پیغام_En_And(m_پیغام_En_Idx, fromp->m_پیغام_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    خالی modifyWarnOff(V3_Error_رمز رمز, bool turnOff) { warnOff(رمز, turnOff); }

    // OPERATORS
    خالی v3error_انتہا(std::ostringstream& str, ثابت string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    خالی v3error_انتہا_Fatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3error_انتہا(str);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warn_مزید() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() ثابت VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        واپس warnContext() + warn_Context_والد();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() ثابت { واپس warnContext(); }
    bool عامل==(ثابت File_سطر& rhs) ثابت {
        واپس (m_token_عدد == rhs.m_token_عدد && m_پہلا_Lineno == rhs.m_پہلا_Lineno
                && m_پہلا_عمود == rhs.m_پہلا_عمود && m_آخری_Lineno_Adder == rhs.m_آخری_Lineno_Adder
                && m_آخری_عمود == rhs.m_آخری_عمود && m_filenameno == rhs.m_filenameno
                && m_پیغام_En_Idx == rhs.m_پیغام_En_Idx);
    }
    bool equal_پہلا_سطر_ستون(ثابت File_سطر& rhs) ثابت {
        واپس (m_filenameno == rhs.m_filenameno && m_پہلا_Lineno == rhs.m_پہلا_Lineno
                && m_پہلا_عمود == rhs.m_پہلا_عمود);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    صحیح operatorCompare(ثابت File_سطر& rhs) ثابت {
        اگر (m_filenameno != rhs.m_filenameno) واپس (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        اگر (m_پہلا_Lineno != rhs.m_پہلا_Lineno)
            واپس (m_پہلا_Lineno < rhs.m_پہلا_Lineno) ? -1 : 1;
        اگر (m_پہلا_عمود != rhs.m_پہلا_عمود)
            واپس (m_پہلا_عمود < rhs.m_پہلا_عمود) ? -1 : 1;
        اگر (m_آخری_Lineno_Adder != rhs.m_آخری_Lineno_Adder)
            واپس (m_آخری_Lineno_Adder < rhs.m_آخری_Lineno_Adder) ? -1 : 1;
        اگر (m_آخری_عمود != rhs.m_آخری_عمود) واپس (m_آخری_عمود < rhs.m_آخری_عمود) ? -1 : 1;
        ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& lhs_پیغام_En = پیغام_En();
        ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& rhs_پیغام_En = rhs.پیغام_En();
        برائے (سائز_t i = 0; i < V3_Error_رمز::_ENUM_MAX; ++i) {
            ثابت V3_Error_رمز رمز = static_cast<V3_Error_رمز>(i);
            اگر (lhs_پیغام_En.enabled(رمز) != rhs_پیغام_En.enabled(رمز))
                واپس rhs_پیغام_En.enabled(رمز) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        اگر (m_token_عدد != rhs.m_token_عدد) واپس (m_token_عدد < rhs.m_token_عدد) ? -1 : 1;
        واپس 0;  // (*this) and rhs are equivalent
    }

نجی:
    string warnContext() ثابت;
    string warn_Context_والد() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    ثابت پیغام_En_ثنائی_ہندسہ_سیٹ& پیغام_En() ثابت VL_MT_محفوظ { واپس singleton().پیغام_En(m_پیغام_En_Idx); }
};
std::ostream& عامل<<(std::ostream& os, File_سطر* fileline);

#اگر_ختم  // Guard
