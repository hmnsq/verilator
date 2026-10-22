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

#اگر_تعریف_نشده VERILATOR_V3FILELINE_H_
#تعریف_کن VERILATOR_V3FILELINE_H_

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

class File_سطر;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_سطر_Singleton final {
    دوست class File_سطر;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using پیام_En_مجموعه_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class پیام_En_بیت_مجموعه final {
        VError_بیت_مجموعه m_کد_En;  // Enabled by code directives/metacomments
        VError_بیت_مجموعه m_ctrlEn;  // Enabled by control file

    عام:
        تعداد class Subset : uint8_t {
            کد_2 = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        پیام_En_بیت_مجموعه() = افتراضي;
        // Create intersection set
        پیام_En_بیت_مجموعه(ثابت پیام_En_بیت_مجموعه& a, ثابت پیام_En_بیت_مجموعه& b)
            : m_کد_En{a.m_کد_En & b.m_کد_En}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        بنية Hash final {
            اندازه_t عملگر()(ثابت پیام_En_بیت_مجموعه& قلم_2) ثابت {
                V3Hash hash{قلم_2.m_کد_En.hash()};
                hash += قلم_2.m_ctrlEn.hash();
                أرجع hash.مقدار();
            }
        };

        بنية Equal final {
            bool عملگر()(ثابت پیام_En_بیت_مجموعه& a, ثابت پیام_En_بیت_مجموعه& b) ثابت { أرجع a == b; }
        };

        bool عملگر==(ثابت پیام_En_بیت_مجموعه& other) ثابت {
            أرجع m_کد_En == other.m_کد_En && m_ctrlEn == other.m_ctrlEn;
        }

        ثابت VError_بیت_مجموعه& getAll(Subset subset) ثابت {
            أرجع subset == Subset::کد_2 ? m_کد_En : m_ctrlEn;
        }
        bool test(Subset subset, V3_Error_کد کد) ثابت {
            أرجع subset == Subset::کد_2 ? m_کد_En.test(کد) : m_ctrlEn.test(کد);
        }
        فراغ مجموعه_All(Subset subset, ثابت VError_بیت_مجموعه& bitset) {
            إذا (subset == Subset::کد_2) {  // LCOV_EXCL_BR_LINE
                m_کد_En = bitset;  // LCOV_EXCL_LINE
            } وإلا {
                m_ctrlEn = bitset;
            }
        }
        فراغ مجموعه(Subset subset, V3_Error_کد کد, bool مقدار) {
            إذا (subset == Subset::کد_2) {
                m_کد_En.مجموعه(کد, مقدار);
            } وإلا {
                m_ctrlEn.مجموعه(کد, مقدار);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3_Error_کد کد) ثابت { أرجع m_کد_En.test(کد) && m_ctrlEn.test(کد); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<ثابت std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_نامها;  // filename text for each filenameno
    std::deque<V3_Lang_کد> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<پیام_En_بیت_مجموعه, پیام_En_مجموعه_Idx_t, پیام_En_بیت_مجموعه::Hash, پیام_En_بیت_مجموعه::Equal>
        m_interned_پیام_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::وکتور<پیام_En_بیت_مجموعه> m_interned_پیام_Ens;

    // CONSTRUCTORS
    File_سطر_Singleton() = افتراضي;
    ~File_سطر_Singleton() = افتراضي;

    fileNameIdx_t name_مقصد_عدد(ثابت string& filename);
    ثابت std::string& عدد_مقصد_Name(fileNameIdx_t filenameno) ثابت VL_MT_امن {
        أرجع m_نامها[filenameno];
    }
    V3_Lang_کد عدد_مقصد_Lang(fileNameIdx_t filenameno) ثابت { أرجع m_languages[filenameno]; }
    فراغ عدد_مقصد_Lang(fileNameIdx_t filenameno, ثابت V3_Lang_کد& l) {
        m_languages[filenameno] = l;
    }
    فراغ clear() {
        m_namemap.clear();
        m_نامها.clear();
        m_languages.clear();
    }
    فراغ file_Name_رقم_Map_Dump_Json(std::ostream& os);
    ساكن string filenameLetters(fileNameIdx_t fileno) VL_خالص;

    // Add given bitset to the interned bitsets, return interned index
    پیام_En_مجموعه_Idx_t جمع_کن_پیام_En_بیت_مجموعه(ثابت پیام_En_بیت_مجموعه& بیت_مجموعه) VL_MT_امن_EXCLUDES(m_mutex);
    // Add index of default bitset
    پیام_En_مجموعه_Idx_t default_پیام_En_شاخص() VL_MT_امن;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    پیام_En_مجموعه_Idx_t پیام_En_مجموعه_بیت(پیام_En_مجموعه_Idx_t مجموعه_Idx, پیام_En_بیت_مجموعه::Subset subset, V3_Error_کد کد,
                              bool مقدار);
    // Bulk-set all control codes to given bitset
    پیام_En_مجموعه_Idx_t پیام_مجموعه_Ctrl_بیت_مجموعه(پیام_En_مجموعه_Idx_t مجموعه_Idx, ثابت VError_بیت_مجموعه& bitset);
    // Return index to intersection set
    پیام_En_مجموعه_Idx_t پیام_En_And(پیام_En_مجموعه_Idx_t lhsIdx, پیام_En_مجموعه_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    ثابت پیام_En_بیت_مجموعه& پیام_En(پیام_En_مجموعه_Idx_t idx) ثابت VL_MT_امن {
        أرجع m_interned_پیام_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    دوست class File_سطر;
    // MEMBERS
    صحيح m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<اندازه_t> m_ref_شمار{0};
    std::deque<string> m_سطرها;  // Source text lines
    VFileContent() {
        ساكن صحيح s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = افتراضي;
    // METHODS
    فراغ refInc() { ++m_ref_شمار; }
    فراغ refDec() {
        إذا (!--m_ref_شمار) delete this;
    }

عام:
    فراغ افزودن_متن(ثابت string& متن);  // Add arbitrary text (need not be line-by-line)
    string get_سطر(صحيح lineno) ثابت VL_MT_امن;
    string ascii() ثابت { أرجع "ct" + cvt_مقصد_Str(m_id); }
};
std::ostream& عملگر<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_سطر final {

    // CONSTANTS
    ساكن constexpr بیعلامت SHOW_منبع_MAX_درازی = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_سطر_Singleton::fileNameIdx_t;
    using پیام_En_مجموعه_Idx_t = File_سطر_Singleton::پیام_En_مجموعه_Idx_t;
    using پیام_En_بیت_مجموعه = File_سطر_Singleton::پیام_En_بیت_مجموعه;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    پیام_En_مجموعه_Idx_t m_پیام_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    بیعلامت m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_رقم = 0;  // Token number in processing order
    صحيح m_اولی_Lineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_اولی_ستونی : 24;  // `line corrected token's first column number
    uint32_t m_آخرین_ستونی : 24;  // `line corrected token's last column number
    uint16_t m_آخرین_Lineno_Adder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_سطر* m_والد = nullptr;  // Parent line that included this line

محمي:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    دوست class File_سطر_Singleton;
    دوست class V3ParseImp;
    دوست class V3PreLex;
    دوست class V3PreProcImp;
    دوست class V3PreShellImp;

خاص:
    // CONSTRUCTORS
    ساكن File_سطر_Singleton& singleton() VL_MT_امن {
        ساكن File_سطر_Singleton s;
        أرجع s;
    }
    ساكن File_سطر& default_File_سطر() VL_MT_امن {
        ساكن File_سطر s;
        أرجع s;
    }

    File_سطر()  // Only used for defaultFileLine above
        : m_پیام_En_Idx{singleton().default_پیام_En_شاخص()}
        , m_filenameno{singleton().name_مقصد_عدد(File_سطر::built_درون_Filename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_اولی_ستونی{0}
        , m_آخرین_ستونی{0} {}

عام:
    explicit File_سطر(ثابت string& filename)
        : m_پیام_En_Idx{default_File_سطر().m_پیام_En_Idx}
        , m_filenameno{singleton().name_مقصد_عدد(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_اولی_ستونی{0}
        , m_آخرین_ستونی{0} {}
    explicit File_سطر(ثابت File_سطر& from)
        : m_پیام_En_Idx{from.m_پیام_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_رقم{from.m_token_رقم}
        , m_اولی_Lineno{from.m_اولی_Lineno}
        , m_اولی_ستونی{from.m_اولی_ستونی}
        , m_آخرین_ستونی{from.m_آخرین_ستونی}
        , m_آخرین_Lineno_Adder{from.m_آخرین_Lineno_Adder}
        , m_contentp{from.m_contentp}
        , m_والد{from.m_والد} {
        إذا (m_contentp) m_contentp->refInc();
    }
    explicit File_سطر(File_سطر* fromp)
        : m_پیام_En_Idx{fromp->m_پیام_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_رقم{fromp->m_token_رقم}
        , m_اولی_Lineno{fromp->m_اولی_Lineno}
        , m_اولی_ستونی{fromp->m_اولی_ستونی}
        , m_آخرین_ستونی{fromp->m_آخرین_ستونی}
        , m_آخرین_Lineno_Adder{fromp->m_آخرین_Lineno_Adder}
        , m_contentp{fromp->m_contentp}
        , m_والد{fromp->m_والد} {
        إذا (m_contentp) m_contentp->refInc();
    }
    فراغ applyIgnores();
    File_سطر* copy_Or_Same_File_سطر();
    File_سطر* copy_Or_Same_File_سطر_Applied();
    ساكن فراغ delete_All_مانده();
    ساكن فراغ stats();
    ~File_سطر();
#اگر_تعریف_شده VL_LEAK_CHECKS
    ساكن فراغ* عملگر new(اندازه_t اندازه);
    ساكن فراغ عملگر delete(فراغ* obj, اندازه_t اندازه);
#پایان_اگر
    // METHODS
    فراغ newContent();
    فراغ contentLineno(صحيح رقم) {
        lineno(رقم);
        m_contentLineno = static_cast<بیعلامت>(رقم);
    }
    فراغ contentLinenoFrom(ثابت File_سطر* fromp) { m_contentLineno = fromp->m_contentLineno; }
    فراغ lineno(صحيح رقم) {
        m_اولی_Lineno = رقم;
        m_آخرین_Lineno_Adder = 0;
        m_اولی_ستونی = 1;
        m_آخرین_ستونی = 1;
    }
    فراغ ستونی_2(صحيح اولی_ستونی, صحيح آخرین_ستونی) {
        m_اولی_ستونی = اولی_ستونی;
        m_آخرین_ستونی = آخرین_ستونی;
    }
    فراغ language(V3_Lang_کد lang) ثابت { singleton().عدد_مقصد_Lang(filenameno(), lang); }
    فراغ filename(ثابت string& name) { m_filenameno = singleton().name_مقصد_عدد(name); }
    فراغ والد(File_سطر* fileline) { m_والد = fileline; }
    فراغ سطر_Directive(ثابت حرف* textp, صحيح& enterExitRef);
    فراغ سطر_Directive_Parse(ثابت حرف* textp, string& filenameRef, صحيح& linenoRef,
                            صحيح& enterExitRef);
    فراغ linenoInc() {
        إذا (m_آخرین_Lineno_Adder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_اولی_Lineno += 65535 + 1;
            m_آخرین_Lineno_Adder = 0;
        } وإلا {
            ++m_آخرین_Lineno_Adder;
        }
        m_آخرین_ستونی = 1;
        ++m_contentLineno;
    }
    فراغ شروع_Token() {
        m_اولی_Lineno = آخرین_Lineno();
        m_اولی_ستونی = m_آخرین_ستونی;
        m_آخرین_Lineno_Adder = 0;
    }
    // Advance last line/column based on given text
    فراغ forwardToken(ثابت حرف* textp, اندازه_t اندازه, bool track_سطرها = true);
    uint32_t token_رقم() ثابت VL_MT_امن { أرجع m_token_رقم; }
    صحيح اولی_Lineno() ثابت VL_MT_امن { أرجع m_اولی_Lineno; }
    صحيح اولی_ستونی() ثابت VL_MT_امن { أرجع static_cast<صحيح>(m_اولی_ستونی); }
    صحيح آخرین_Lineno() ثابت VL_MT_امن { أرجع m_اولی_Lineno + m_آخرین_Lineno_Adder; }
    صحيح آخرین_ستونی() ثابت VL_MT_امن { أرجع static_cast<صحيح>(m_آخرین_ستونی); }
    VFileContent* contentp() ثابت { أرجع m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    صحيح lineno() ثابت VL_MT_امن { أرجع آخرین_Lineno(); }
    string منبع() ثابت VL_MT_امن;
    string منبع_پیشوند(صحيح مقصد_ستونی) ثابت VL_MT_امن;
    string pretty_منبع() ثابت VL_MT_امن;  // Source, w/stripped unprintables and newlines
    File_سطر* والد() ثابت VL_MT_امن { أرجع m_والد; }
    V3_Lang_کد language() ثابت { أرجع singleton().عدد_مقصد_Lang(filenameno()); }
    string ascii() ثابت VL_MT_امن;
    string ascii_سطر_ستون() ثابت;
    صحيح filenameno() ثابت VL_MT_امن { أرجع m_filenameno; }
    ثابت std::string& filename() ثابت VL_MT_امن {
        أرجع singleton().عدد_مقصد_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() ثابت VL_MT_امن { أرجع VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() ثابت VL_MT_امن {
        أرجع (filename() == command_سطر_Filename() || filename() == built_درون_Filename());
    }
    string filenameLetters() ثابت VL_MT_امن {
        أرجع File_سطر_Singleton::filenameLetters(filenameno());
    }
    string filebasename() ثابت VL_MT_امن;
    string filebasenameNoExt() ثابت;
    string اولی_ستونی_Letters() ثابت VL_MT_امن;
    string profileFuncname() ثابت;
    string سطر_Directive_Strg(صحيح enterExit) ثابت;

    // Turn on/off warning messages on this line.
خاص:
    فراغ warn_مجموعه(پیام_En_بیت_مجموعه::Subset subset, V3_Error_کد کد, bool flag) {
        m_پیام_En_Idx = singleton().پیام_En_مجموعه_بیت(m_پیام_En_Idx, subset, کد, flag);
    }

عام:
    فراغ warnOn(V3_Error_کد کد, bool flag) { warn_مجموعه(پیام_En_بیت_مجموعه::Subset::کد_2, کد, flag); }
    فراغ warn_مجموعه_Ctrl_بیت_مجموعه(ثابت VError_بیت_مجموعه& bitset) {
        m_پیام_En_Idx = singleton().پیام_مجموعه_Ctrl_بیت_مجموعه(m_پیام_En_Idx, bitset);
    }
    فراغ warnOff(V3_Error_کد کد, bool turnOff) { warnOn(کد, !turnOff); }
    string warnOffParse(ثابت string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3_Error_کد کد) ثابت VL_MT_امن;
    فراغ warnLintOff(bool turnOff);
    فراغ warnStyleOff(bool turnOff);
    فراغ warnStateFrom(ثابت File_سطر& from) { m_پیام_En_Idx = from.m_پیام_En_Idx; }
    فراغ warnResetDefault() { warnStateFrom(default_File_سطر()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() ثابت { أرجع پیام_En().enabled(V3_Error_کد::I_CELLDEFINE); }
    فراغ celldefineOn(bool flag) { warnOn(V3_Error_کد::I_CELLDEFINE, flag); }
    bool coverageOn() ثابت { أرجع پیام_En().enabled(V3_Error_کد::I_COVERAGE); }
    فراغ coverageOn(bool flag) { warnOn(V3_Error_کد::I_COVERAGE, flag); }
    bool erroringOn() ثابت { أرجع پیام_En().enabled(V3_Error_کد::I_ERRORING); }
    فراغ erroringOn(bool flag) { warnOn(V3_Error_کد::I_ERRORING, flag); }
    bool tracingOn() ثابت { أرجع پیام_En().enabled(V3_Error_کد::I_TRACING); }
    فراغ tracingOn(bool flag) { warnOn(V3_Error_کد::I_TRACING, flag); }
    bool timingOn() ثابت { أرجع پیام_En().enabled(V3_Error_کد::I_TIMING); }
    فراغ timingOn(bool flag) { warnOn(V3_Error_کد::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    ساكن string command_سطر_Filename() VL_MT_امن { أرجع "<command-line>"; }
    ساكن string built_درون_Filename() VL_MT_امن { أرجع "<built-in>"; }
    ساكن فراغ globalWarnOff(V3_Error_کد کد, bool turnOff) {
        default_File_سطر().warnOff(کد, turnOff);
    }
    ساكن string globalWarnOffParse(ثابت string& msgs, bool turnOff) {
        أرجع default_File_سطر().warnOffParse(msgs, turnOff);
    }
    ساكن فراغ file_Name_رقم_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_رقم_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    فراغ modifyStateInherit(ثابت File_سطر* fromp) {
        m_پیام_En_Idx = singleton().پیام_En_And(m_پیام_En_Idx, fromp->m_پیام_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    فراغ modifyWarnOff(V3_Error_کد کد, bool turnOff) { warnOff(کد, turnOff); }

    // OPERATORS
    فراغ v3error_انتها(std::ostringstream& str, ثابت string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    فراغ v3error_انتها_Fatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3error_انتها(str);
        أكد(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warn_بیشتر() ثابت VL_REQUIRES(V3Error::s().m_mutex);
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
    bool عملگر==(ثابت File_سطر& rhs) ثابت {
        أرجع (m_token_رقم == rhs.m_token_رقم && m_اولی_Lineno == rhs.m_اولی_Lineno
                && m_اولی_ستونی == rhs.m_اولی_ستونی && m_آخرین_Lineno_Adder == rhs.m_آخرین_Lineno_Adder
                && m_آخرین_ستونی == rhs.m_آخرین_ستونی && m_filenameno == rhs.m_filenameno
                && m_پیام_En_Idx == rhs.m_پیام_En_Idx);
    }
    bool equal_اولی_سطر_ستون(ثابت File_سطر& rhs) ثابت {
        أرجع (m_filenameno == rhs.m_filenameno && m_اولی_Lineno == rhs.m_اولی_Lineno
                && m_اولی_ستونی == rhs.m_اولی_ستونی);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    صحيح operatorCompare(ثابت File_سطر& rhs) ثابت {
        إذا (m_filenameno != rhs.m_filenameno) أرجع (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        إذا (m_اولی_Lineno != rhs.m_اولی_Lineno)
            أرجع (m_اولی_Lineno < rhs.m_اولی_Lineno) ? -1 : 1;
        إذا (m_اولی_ستونی != rhs.m_اولی_ستونی)
            أرجع (m_اولی_ستونی < rhs.m_اولی_ستونی) ? -1 : 1;
        إذا (m_آخرین_Lineno_Adder != rhs.m_آخرین_Lineno_Adder)
            أرجع (m_آخرین_Lineno_Adder < rhs.m_آخرین_Lineno_Adder) ? -1 : 1;
        إذا (m_آخرین_ستونی != rhs.m_آخرین_ستونی) أرجع (m_آخرین_ستونی < rhs.m_آخرین_ستونی) ? -1 : 1;
        ثابت پیام_En_بیت_مجموعه& lhs_پیام_En = پیام_En();
        ثابت پیام_En_بیت_مجموعه& rhs_پیام_En = rhs.پیام_En();
        لكل (اندازه_t i = 0; i < V3_Error_کد::_ENUM_MAX; ++i) {
            ثابت V3_Error_کد کد = static_cast<V3_Error_کد>(i);
            إذا (lhs_پیام_En.enabled(کد) != rhs_پیام_En.enabled(کد))
                أرجع rhs_پیام_En.enabled(کد) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        إذا (m_token_رقم != rhs.m_token_رقم) أرجع (m_token_رقم < rhs.m_token_رقم) ? -1 : 1;
        أرجع 0;  // (*this) and rhs are equivalent
    }

خاص:
    string warnContext() ثابت;
    string warn_Context_والد() ثابت VL_REQUIRES(V3Error::s().m_mutex);
    ثابت پیام_En_بیت_مجموعه& پیام_En() ثابت VL_MT_امن { أرجع singleton().پیام_En(m_پیام_En_Idx); }
};
std::ostream& عملگر<<(std::ostream& os, File_سطر* fileline);

#پایان_اگر  // Guard
