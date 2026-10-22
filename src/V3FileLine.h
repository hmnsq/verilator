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

#నిర్వచితం_కాకపోతే VERILATOR_V3FILELINE_H_
#నిర్వచించు VERILATOR_V3FILELINE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Hash.h"
#అన్తర్భావ "V3LangCode.h"
#అన్తర్భావ "V3Mutex.h"

#అన్తర్భావ <atomic>
#అన్తర్భావ <deque>
#అన్తర్భావ <map>
#అన్తర్భావ <memory>
#అన్తర్భావ <set>
#అన్తర్భావ <sstream>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <vector>

// ######################################################################

class File_పంక్తి;

//! Singleton class with tables of per-file data.

//! This singleton class contains tables of data that are unchanging in each
//! source file (each with its own unique filename number).
class File_పంక్తి_Singleton final {
    మిత్ర class File_పంక్తి;

    // TYPES
    using fileNameIdx_t = uint16_t;  // Increase width if 64K input files are not enough
    using సందేశం_En_సమితి_Idx_t = uint16_t;  // Increase width if 64K unique message sets are not enough
    class సందేశం_En_బిట్టు_సమితి final {
        VError_బిట్టు_సమితి m_సంకేతం_En;  // Enabled by code directives/metacomments
        VError_బిట్టు_సమితి m_ctrlEn;  // Enabled by control file

    బహిరంగ:
        గణన class Subset : uint8_t {
            సంకేతం_3 = 0,  // Selects m_codeEn, the enable bits used by in-code directives/metacomments
            CTRL = 1,  // Selects m_ctrlEn, the enable bits used by control files
        };

        // Create empty set
        సందేశం_En_బిట్టు_సమితి() = అప్రమేయ;
        // Create intersection set
        సందేశం_En_బిట్టు_సమితి(స్థిరరాశి సందేశం_En_బిట్టు_సమితి& a, స్థిరరాశి సందేశం_En_బిట్టు_సమితి& b)
            : m_సంకేతం_En{a.m_సంకేతం_En & b.m_సంకేతం_En}
            , m_ctrlEn{a.m_ctrlEn & b.m_ctrlEn} {}

        నిర్మాణం Hash final {
            కొలత_t క్రియాకారకం()(స్థిరరాశి సందేశం_En_బిట్టు_సమితి& అంశం_2) స్థిరరాశి {
                V3Hash hash{అంశం_2.m_సంకేతం_En.hash()};
                hash += అంశం_2.m_ctrlEn.hash();
                తిరిగి hash.విలువ();
            }
        };

        నిర్మాణం Equal final {
            bool క్రియాకారకం()(స్థిరరాశి సందేశం_En_బిట్టు_సమితి& a, స్థిరరాశి సందేశం_En_బిట్టు_సమితి& b) స్థిరరాశి { తిరిగి a == b; }
        };

        bool క్రియాకారకం==(స్థిరరాశి సందేశం_En_బిట్టు_సమితి& other) స్థిరరాశి {
            తిరిగి m_సంకేతం_En == other.m_సంకేతం_En && m_ctrlEn == other.m_ctrlEn;
        }

        స్థిరరాశి VError_బిట్టు_సమితి& getAll(Subset subset) స్థిరరాశి {
            తిరిగి subset == Subset::సంకేతం_3 ? m_సంకేతం_En : m_ctrlEn;
        }
        bool test(Subset subset, V3_Error_సంకేతం సంకేతం_2) స్థిరరాశి {
            తిరిగి subset == Subset::సంకేతం_3 ? m_సంకేతం_En.test(సంకేతం_2) : m_ctrlEn.test(సంకేతం_2);
        }
        శూన్య సమితి_All(Subset subset, స్థిరరాశి VError_బిట్టు_సమితి& bitset) {
            ఒకవేళ (subset == Subset::సంకేతం_3) {  // LCOV_EXCL_BR_LINE
                m_సంకేతం_En = bitset;  // LCOV_EXCL_LINE
            } లేకపోతే {
                m_ctrlEn = bitset;
            }
        }
        శూన్య సమితి(Subset subset, V3_Error_సంకేతం సంకేతం_2, bool విలువ) {
            ఒకవేళ (subset == Subset::సంకేతం_3) {
                m_సంకేతం_En.సమితి(సంకేతం_2, విలువ);
            } లేకపోతే {
                m_ctrlEn.సమితి(సంకేతం_2, విలువ);
            }
        }

        // Enabled iff enabled by both in-code dierctives/metacomments and control file
        bool enabled(V3_Error_సంకేతం సంకేతం_2) స్థిరరాశి { తిరిగి m_సంకేతం_En.test(సంకేతం_2) && m_ctrlEn.test(సంకేతం_2); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // protects members
    std::map<స్థిరరాశి std::string, fileNameIdx_t> m_namemap;  // filenameno for each filename
    std::deque<string> m_పేర్లు;  // filename text for each filenameno
    std::deque<V3_Lang_సంకేతం> m_languages;  // language for each filenameno

    // Map from flag set to the index in m_internedMsgEns for interning
    std::unordered_map<సందేశం_En_బిట్టు_సమితి, సందేశం_En_సమితి_Idx_t, సందేశం_En_బిట్టు_సమితి::Hash, సందేశం_En_బిట్టు_సమితి::Equal>
        m_interned_సందేశం_En_Idxs VL_GUARDED_BY(m_mutex);
    // Interned message enablement flag sets
    std::సదిశ<సందేశం_En_బిట్టు_సమితి> m_interned_సందేశం_Ens;

    // CONSTRUCTORS
    File_పంక్తి_Singleton() = అప్రమేయ;
    ~File_పంక్తి_Singleton() = అప్రమేయ;

    fileNameIdx_t name_గమ్యం_సంఖ్య(స్థిరరాశి string& filename);
    స్థిరరాశి std::string& సంఖ్య_గమ్యం_Name(fileNameIdx_t filenameno) స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_పేర్లు[filenameno];
    }
    V3_Lang_సంకేతం సంఖ్య_గమ్యం_Lang(fileNameIdx_t filenameno) స్థిరరాశి { తిరిగి m_languages[filenameno]; }
    శూన్య సంఖ్య_గమ్యం_Lang(fileNameIdx_t filenameno, స్థిరరాశి V3_Lang_సంకేతం& l) {
        m_languages[filenameno] = l;
    }
    శూన్య clear() {
        m_namemap.clear();
        m_పేర్లు.clear();
        m_languages.clear();
    }
    శూన్య file_Name_అంకె_Map_Dump_Json(std::ostream& os);
    స్థిర string filenameLetters(fileNameIdx_t fileno) VL_శుద్ధ;

    // Add given bitset to the interned bitsets, return interned index
    సందేశం_En_సమితి_Idx_t కూడు_సందేశం_En_బిట్టు_సమితి(స్థిరరాశి సందేశం_En_బిట్టు_సమితి& బిట్టు_సమితి) VL_MT_సురక్షిత_EXCLUDES(m_mutex);
    // Add index of default bitset
    సందేశం_En_సమితి_Idx_t default_సందేశం_En_సూచీ() VL_MT_సురక్షిత;
    // Set code to value in bitset at interned index setIdx, return interned index of result
    సందేశం_En_సమితి_Idx_t సందేశం_En_సమితి_బిట్టు(సందేశం_En_సమితి_Idx_t సమితి_Idx, సందేశం_En_బిట్టు_సమితి::Subset subset, V3_Error_సంకేతం సంకేతం_2,
                              bool విలువ);
    // Bulk-set all control codes to given bitset
    సందేశం_En_సమితి_Idx_t సందేశం_సమితి_Ctrl_బిట్టు_సమితి(సందేశం_En_సమితి_Idx_t సమితి_Idx, స్థిరరాశి VError_బిట్టు_సమితి& bitset);
    // Return index to intersection set
    సందేశం_En_సమితి_Idx_t సందేశం_En_And(సందేశం_En_సమితి_Idx_t lhsIdx, సందేశం_En_సమితి_Idx_t rhsIdx);
    // Retrieve interned bitset at given interned index. The returned reference is not persistent.
    స్థిరరాశి సందేశం_En_బిట్టు_సమితి& సందేశం_En(సందేశం_En_సమితి_Idx_t idx) స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి m_interned_సందేశం_Ens.at(idx);
    }
};

// All source lines from a file/stream, to enable errors to show sources
class VFileContent final {
    మిత్ర class File_పంక్తి;
    // MEMBERS
    పూర్ణాంకం m_id;  // Content ID number
    // Reference count for sharing (shared_ptr has size overhead that we don't want)
    std::atomic<కొలత_t> m_ref_లెక్క{0};
    std::deque<string> m_పంక్తులు;  // Source text lines
    VFileContent() {
        స్థిర పూర్ణాంకం s_id = 0;
        m_id = ++s_id;
    }
    ~VFileContent() = అప్రమేయ;
    // METHODS
    శూన్య refInc() { ++m_ref_లెక్క; }
    శూన్య refDec() {
        ఒకవేళ (!--m_ref_లెక్క) delete this;
    }

బహిరంగ:
    శూన్య నెట్టు_పాఠ్యం(స్థిరరాశి string& పాఠ్యం);  // Add arbitrary text (need not be line-by-line)
    string get_పంక్తి(పూర్ణాంకం lineno) స్థిరరాశి VL_MT_సురక్షిత;
    string ascii() స్థిరరాశి { తిరిగి "ct" + cvt_గమ్యం_Str(m_id); }
};
std::ostream& క్రియాకారకం<<(std::ostream& os, VFileContent* contentp);

// File and line number of an object, mostly for error reporting

// This class is instantiated for every source code line (potentially millions), and instances
// created at any point usually persist until the end of the program. To save space, per-file
// information (e.g. filename, source language) is held in tables in the FileLineSingleton class.
// Similarly, message enablement flags are interned in FileLineSingleton.

// WARNING: Avoid increasing the size of this class as much as possible.
class File_పంక్తి final {

    // CONSTANTS
    స్థిర constexpr గుర్తులేని SHOW_ఆధారం_MAX_నిడివి = 400;  // Don't show source lines > this long

    // TYPES
    using fileNameIdx_t = File_పంక్తి_Singleton::fileNameIdx_t;
    using సందేశం_En_సమితి_Idx_t = File_పంక్తి_Singleton::సందేశం_En_సమితి_Idx_t;
    using సందేశం_En_బిట్టు_సమితి = File_పంక్తి_Singleton::సందేశం_En_బిట్టు_సమితి;

    // MEMBERS
    // Columns here means number of chars from beginning (i.e. tabs count as one)
    // 64-bit word # 0
    సందేశం_En_సమితి_Idx_t m_సందేశం_En_Idx = 0;  // Message enable bit set (index into interned array)
    fileNameIdx_t m_filenameno = 0;  // `line corrected filename number
    bool m_waive : 1;  // Waive warning - pack next to the line number to save 8 bytes of storage
    గుర్తులేని m_contentLineno : 31;  // Line number within source stream
    // 64-bit word # 1
    uint32_t m_token_అంకె = 0;  // Token number in processing order
    పూర్ణాంకం m_మొదటి_Lineno = 0;  // `line corrected token's first line number
    // 64-bit word # 2
    uint32_t m_మొదటి_స్తంభం : 24;  // `line corrected token's first column number
    uint32_t m_చివరి_స్తంభం : 24;  // `line corrected token's last column number
    uint16_t m_చివరి_Lineno_Adder = 0;  // Last line number as offset from m_firstLineno
    // 64-bit word # 3/4
    VFileContent* m_contentp = nullptr;  // Source text contents line is within
    File_పంక్తి* m_మాతృ = nullptr;  // Parent line that included this line

రక్షిత:
    // User routines should never need to change line numbers
    // We are storing pointers, so we CAN'T change them after initial reading.
    మిత్ర class File_పంక్తి_Singleton;
    మిత్ర class V3ParseImp;
    మిత్ర class V3PreLex;
    మిత్ర class V3PreProcImp;
    మిత్ర class V3PreShellImp;

అంతరంగ:
    // CONSTRUCTORS
    స్థిర File_పంక్తి_Singleton& singleton() VL_MT_సురక్షిత {
        స్థిర File_పంక్తి_Singleton s;
        తిరిగి s;
    }
    స్థిర File_పంక్తి& default_File_పంక్తి() VL_MT_సురక్షిత {
        స్థిర File_పంక్తి s;
        తిరిగి s;
    }

    File_పంక్తి()  // Only used for defaultFileLine above
        : m_సందేశం_En_Idx{singleton().default_సందేశం_En_సూచీ()}
        , m_filenameno{singleton().name_గమ్యం_సంఖ్య(File_పంక్తి::built_లోపల_Filename())}
        , m_waive{false}
        , m_contentLineno{0}
        , m_మొదటి_స్తంభం{0}
        , m_చివరి_స్తంభం{0} {}

బహిరంగ:
    explicit File_పంక్తి(స్థిరరాశి string& filename)
        : m_సందేశం_En_Idx{default_File_పంక్తి().m_సందేశం_En_Idx}
        , m_filenameno{singleton().name_గమ్యం_సంఖ్య(filename)}
        , m_waive{false}
        , m_contentLineno{0}
        , m_మొదటి_స్తంభం{0}
        , m_చివరి_స్తంభం{0} {}
    explicit File_పంక్తి(స్థిరరాశి File_పంక్తి& from)
        : m_సందేశం_En_Idx{from.m_సందేశం_En_Idx}
        , m_filenameno{from.m_filenameno}
        , m_waive{from.m_waive}
        , m_contentLineno{from.m_contentLineno}
        , m_token_అంకె{from.m_token_అంకె}
        , m_మొదటి_Lineno{from.m_మొదటి_Lineno}
        , m_మొదటి_స్తంభం{from.m_మొదటి_స్తంభం}
        , m_చివరి_స్తంభం{from.m_చివరి_స్తంభం}
        , m_చివరి_Lineno_Adder{from.m_చివరి_Lineno_Adder}
        , m_contentp{from.m_contentp}
        , m_మాతృ{from.m_మాతృ} {
        ఒకవేళ (m_contentp) m_contentp->refInc();
    }
    explicit File_పంక్తి(File_పంక్తి* fromp)
        : m_సందేశం_En_Idx{fromp->m_సందేశం_En_Idx}
        , m_filenameno{fromp->m_filenameno}
        , m_waive{fromp->m_waive}
        , m_contentLineno{fromp->m_contentLineno}
        , m_token_అంకె{fromp->m_token_అంకె}
        , m_మొదటి_Lineno{fromp->m_మొదటి_Lineno}
        , m_మొదటి_స్తంభం{fromp->m_మొదటి_స్తంభం}
        , m_చివరి_స్తంభం{fromp->m_చివరి_స్తంభం}
        , m_చివరి_Lineno_Adder{fromp->m_చివరి_Lineno_Adder}
        , m_contentp{fromp->m_contentp}
        , m_మాతృ{fromp->m_మాతృ} {
        ఒకవేళ (m_contentp) m_contentp->refInc();
    }
    శూన్య applyIgnores();
    File_పంక్తి* copy_Or_Same_File_పంక్తి();
    File_పంక్తి* copy_Or_Same_File_పంక్తి_Applied();
    స్థిర శూన్య delete_All_మిగిలిన();
    స్థిర శూన్య stats();
    ~File_పంక్తి();
#నిర్వచితమైతే VL_LEAK_CHECKS
    స్థిర శూన్య* క్రియాకారకం new(కొలత_t కొలత);
    స్థిర శూన్య క్రియాకారకం delete(శూన్య* obj, కొలత_t కొలత);
#అయితే_ముగింపు
    // METHODS
    శూన్య newContent();
    శూన్య contentLineno(పూర్ణాంకం అంకె) {
        lineno(అంకె);
        m_contentLineno = static_cast<గుర్తులేని>(అంకె);
    }
    శూన్య contentLinenoFrom(స్థిరరాశి File_పంక్తి* fromp) { m_contentLineno = fromp->m_contentLineno; }
    శూన్య lineno(పూర్ణాంకం అంకె) {
        m_మొదటి_Lineno = అంకె;
        m_చివరి_Lineno_Adder = 0;
        m_మొదటి_స్తంభం = 1;
        m_చివరి_స్తంభం = 1;
    }
    శూన్య స్తంభం_2(పూర్ణాంకం మొదటి_స్తంభం, పూర్ణాంకం చివరి_స్తంభం) {
        m_మొదటి_స్తంభం = మొదటి_స్తంభం;
        m_చివరి_స్తంభం = చివరి_స్తంభం;
    }
    శూన్య language(V3_Lang_సంకేతం lang) స్థిరరాశి { singleton().సంఖ్య_గమ్యం_Lang(filenameno(), lang); }
    శూన్య filename(స్థిరరాశి string& name) { m_filenameno = singleton().name_గమ్యం_సంఖ్య(name); }
    శూన్య మాతృ(File_పంక్తి* fileline) { m_మాతృ = fileline; }
    శూన్య పంక్తి_Directive(స్థిరరాశి అక్షరం* textp, పూర్ణాంకం& enterExitRef);
    శూన్య పంక్తి_Directive_Parse(స్థిరరాశి అక్షరం* textp, string& filenameRef, పూర్ణాంకం& linenoRef,
                            పూర్ణాంకం& enterExitRef);
    శూన్య linenoInc() {
        ఒకవేళ (m_చివరి_Lineno_Adder == 65535) {
            // Overflow can only happen on super-long comment, adjust
            // starting first line to deal with it this will make token
            // start later than in reality, but nothing should refer to it,
            // and the overall line number will not drift.
            m_మొదటి_Lineno += 65535 + 1;
            m_చివరి_Lineno_Adder = 0;
        } లేకపోతే {
            ++m_చివరి_Lineno_Adder;
        }
        m_చివరి_స్తంభం = 1;
        ++m_contentLineno;
    }
    శూన్య ప్రారంభం_Token() {
        m_మొదటి_Lineno = చివరి_Lineno();
        m_మొదటి_స్తంభం = m_చివరి_స్తంభం;
        m_చివరి_Lineno_Adder = 0;
    }
    // Advance last line/column based on given text
    శూన్య forwardToken(స్థిరరాశి అక్షరం* textp, కొలత_t కొలత, bool track_పంక్తులు = true);
    uint32_t token_అంకె() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_token_అంకె; }
    పూర్ణాంకం మొదటి_Lineno() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_మొదటి_Lineno; }
    పూర్ణాంకం మొదటి_స్తంభం() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి static_cast<పూర్ణాంకం>(m_మొదటి_స్తంభం); }
    పూర్ణాంకం చివరి_Lineno() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_మొదటి_Lineno + m_చివరి_Lineno_Adder; }
    పూర్ణాంకం చివరి_స్తంభం() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి static_cast<పూర్ణాంకం>(m_చివరి_స్తంభం); }
    VFileContent* contentp() స్థిరరాశి { తిరిగి m_contentp; }
    // If not otherwise more specific, use last lineno for errors etc,
    // as the parser errors etc generally make more sense pointing at the last parse point
    పూర్ణాంకం lineno() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి చివరి_Lineno(); }
    string ఆధారం() స్థిరరాశి VL_MT_సురక్షిత;
    string ఆధారం_ఉపసర్గ(పూర్ణాంకం గమ్యం_స్తంభం) స్థిరరాశి VL_MT_సురక్షిత;
    string pretty_ఆధారం() స్థిరరాశి VL_MT_సురక్షిత;  // Source, w/stripped unprintables and newlines
    File_పంక్తి* మాతృ() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_మాతృ; }
    V3_Lang_సంకేతం language() స్థిరరాశి { తిరిగి singleton().సంఖ్య_గమ్యం_Lang(filenameno()); }
    string ascii() స్థిరరాశి VL_MT_సురక్షిత;
    string ascii_పంక్తి_నిలువు() స్థిరరాశి;
    పూర్ణాంకం filenameno() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_filenameno; }
    స్థిరరాశి std::string& filename() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి singleton().సంఖ్య_గమ్యం_Name(filenameno());
    }
    // Filename with C string escapes
    string filenameEsc() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి VString::quoteBackslash(filename()); }
    bool filenameIsGlobal() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి (filename() == command_పంక్తి_Filename() || filename() == built_లోపల_Filename());
    }
    string filenameLetters() స్థిరరాశి VL_MT_సురక్షిత {
        తిరిగి File_పంక్తి_Singleton::filenameLetters(filenameno());
    }
    string filebasename() స్థిరరాశి VL_MT_సురక్షిత;
    string filebasenameNoExt() స్థిరరాశి;
    string మొదటి_స్తంభం_Letters() స్థిరరాశి VL_MT_సురక్షిత;
    string profileFuncname() స్థిరరాశి;
    string పంక్తి_Directive_Strg(పూర్ణాంకం enterExit) స్థిరరాశి;

    // Turn on/off warning messages on this line.
అంతరంగ:
    శూన్య warn_సమితి(సందేశం_En_బిట్టు_సమితి::Subset subset, V3_Error_సంకేతం సంకేతం_2, bool flag) {
        m_సందేశం_En_Idx = singleton().సందేశం_En_సమితి_బిట్టు(m_సందేశం_En_Idx, subset, సంకేతం_2, flag);
    }

బహిరంగ:
    శూన్య warnOn(V3_Error_సంకేతం సంకేతం_2, bool flag) { warn_సమితి(సందేశం_En_బిట్టు_సమితి::Subset::సంకేతం_3, సంకేతం_2, flag); }
    శూన్య warn_సమితి_Ctrl_బిట్టు_సమితి(స్థిరరాశి VError_బిట్టు_సమితి& bitset) {
        m_సందేశం_En_Idx = singleton().సందేశం_సమితి_Ctrl_బిట్టు_సమితి(m_సందేశం_En_Idx, bitset);
    }
    శూన్య warnOff(V3_Error_సంకేతం సంకేతం_2, bool turnOff) { warnOn(సంకేతం_2, !turnOff); }
    string warnOffParse(స్థిరరాశి string& msgs, bool turnOff);  // Returns "" if ok
    bool warnIsOff(V3_Error_సంకేతం సంకేతం_2) స్థిరరాశి VL_MT_సురక్షిత;
    శూన్య warnLintOff(bool turnOff);
    శూన్య warnStyleOff(bool turnOff);
    శూన్య warnStateFrom(స్థిరరాశి File_పంక్తి& from) { m_సందేశం_En_Idx = from.m_సందేశం_En_Idx; }
    శూన్య warnResetDefault() { warnStateFrom(default_File_పంక్తి()); }

    // Specific flag ACCESSORS/METHODS
    bool celldefineOn() స్థిరరాశి { తిరిగి సందేశం_En().enabled(V3_Error_సంకేతం::I_CELLDEFINE); }
    శూన్య celldefineOn(bool flag) { warnOn(V3_Error_సంకేతం::I_CELLDEFINE, flag); }
    bool coverageOn() స్థిరరాశి { తిరిగి సందేశం_En().enabled(V3_Error_సంకేతం::I_COVERAGE); }
    శూన్య coverageOn(bool flag) { warnOn(V3_Error_సంకేతం::I_COVERAGE, flag); }
    bool erroringOn() స్థిరరాశి { తిరిగి సందేశం_En().enabled(V3_Error_సంకేతం::I_ERRORING); }
    శూన్య erroringOn(bool flag) { warnOn(V3_Error_సంకేతం::I_ERRORING, flag); }
    bool tracingOn() స్థిరరాశి { తిరిగి సందేశం_En().enabled(V3_Error_సంకేతం::I_TRACING); }
    శూన్య tracingOn(bool flag) { warnOn(V3_Error_సంకేతం::I_TRACING, flag); }
    bool timingOn() స్థిరరాశి { తిరిగి సందేశం_En().enabled(V3_Error_సంకేతం::I_TIMING); }
    శూన్య timingOn(bool flag) { warnOn(V3_Error_సంకేతం::I_TIMING, flag); }

    // METHODS - Global
    // <command-line> and <built-in> match what GCC outputs
    స్థిర string command_పంక్తి_Filename() VL_MT_సురక్షిత { తిరిగి "<command-line>"; }
    స్థిర string built_లోపల_Filename() VL_MT_సురక్షిత { తిరిగి "<built-in>"; }
    స్థిర శూన్య globalWarnOff(V3_Error_సంకేతం సంకేతం_2, bool turnOff) {
        default_File_పంక్తి().warnOff(సంకేతం_2, turnOff);
    }
    స్థిర string globalWarnOffParse(స్థిరరాశి string& msgs, bool turnOff) {
        తిరిగి default_File_పంక్తి().warnOffParse(msgs, turnOff);
    }
    స్థిర శూన్య file_Name_అంకె_Map_Dump_Json(std::ostream& os) {
        singleton().file_Name_అంకె_Map_Dump_Json(os);
    }

    // METHODS - Called from netlist
    // Merge warning disables from another fileline
    శూన్య modifyStateInherit(స్థిరరాశి File_పంక్తి* fromp) {
        m_సందేశం_En_Idx = singleton().సందేశం_En_And(m_సందేశం_En_Idx, fromp->m_సందేశం_En_Idx);
    }
    // Change the current fileline due to actions discovered after parsing
    // and may have side effects on other nodes sharing this FileLine.
    // Use only when this is intended
    శూన్య modifyWarnOff(V3_Error_సంకేతం సంకేతం_2, bool turnOff) { warnOff(సంకేతం_2, turnOff); }

    // OPERATORS
    శూన్య v3error_చివర(std::ostringstream& str, స్థిరరాశి string& extra = "")
        VL_RELEASE(V3Error::s().m_mutex);
    శూన్య v3error_చివర_Fatal(std::ostringstream& str) VL_ATTR_NORETURN
        VL_RELEASE(V3Error::s().m_mutex) {
        v3error_చివర(str);
        ప్రతిజ్ఞా(0);  // LCOV_EXCL_LINE
        VL_UNREACHABLE;
    }
    /// When building an error, prefix for printing continuation lines
    /// e.g. information referring to the same FileLine as before
    string warn_మరిన్ని() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, prefix for printing secondary information
    /// from a different FileLine than the original error
    string warnOther() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex);
    /// When building an error, current location in include etc
    /// If not used in a given error, automatically pasted at end of error
    string warnContextPrimary() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex) {
        V3Error::s().errorContexted(true);
        తిరిగి warnContext() + warn_Context_మాతృ();
    }
    /// When building an error, additional location for additional references
    /// Simplified information vs warnContextPrimary() to make dump clearer
    string warnContextSecondary() స్థిరరాశి { తిరిగి warnContext(); }
    bool క్రియాకారకం==(స్థిరరాశి File_పంక్తి& rhs) స్థిరరాశి {
        తిరిగి (m_token_అంకె == rhs.m_token_అంకె && m_మొదటి_Lineno == rhs.m_మొదటి_Lineno
                && m_మొదటి_స్తంభం == rhs.m_మొదటి_స్తంభం && m_చివరి_Lineno_Adder == rhs.m_చివరి_Lineno_Adder
                && m_చివరి_స్తంభం == rhs.m_చివరి_స్తంభం && m_filenameno == rhs.m_filenameno
                && m_సందేశం_En_Idx == rhs.m_సందేశం_En_Idx);
    }
    bool equal_మొదటి_పంక్తి_నిలువు(స్థిరరాశి File_పంక్తి& rhs) స్థిరరాశి {
        తిరిగి (m_filenameno == rhs.m_filenameno && m_మొదటి_Lineno == rhs.m_మొదటి_Lineno
                && m_మొదటి_స్తంభం == rhs.m_మొదటి_స్తంభం);
    }
    // Returns -1 if (*this) should come before rhs after sorted. 1 for the opposite case. 0 for
    // equivalent.
    పూర్ణాంకం operatorCompare(స్థిరరాశి File_పంక్తి& rhs) స్థిరరాశి {
        ఒకవేళ (m_filenameno != rhs.m_filenameno) తిరిగి (m_filenameno < rhs.m_filenameno) ? -1 : 1;
        ఒకవేళ (m_మొదటి_Lineno != rhs.m_మొదటి_Lineno)
            తిరిగి (m_మొదటి_Lineno < rhs.m_మొదటి_Lineno) ? -1 : 1;
        ఒకవేళ (m_మొదటి_స్తంభం != rhs.m_మొదటి_స్తంభం)
            తిరిగి (m_మొదటి_స్తంభం < rhs.m_మొదటి_స్తంభం) ? -1 : 1;
        ఒకవేళ (m_చివరి_Lineno_Adder != rhs.m_చివరి_Lineno_Adder)
            తిరిగి (m_చివరి_Lineno_Adder < rhs.m_చివరి_Lineno_Adder) ? -1 : 1;
        ఒకవేళ (m_చివరి_స్తంభం != rhs.m_చివరి_స్తంభం) తిరిగి (m_చివరి_స్తంభం < rhs.m_చివరి_స్తంభం) ? -1 : 1;
        స్థిరరాశి సందేశం_En_బిట్టు_సమితి& lhs_సందేశం_En = సందేశం_En();
        స్థిరరాశి సందేశం_En_బిట్టు_సమితి& rhs_సందేశం_En = rhs.సందేశం_En();
        కొరకు (కొలత_t i = 0; i < V3_Error_సంకేతం::_ENUM_MAX; ++i) {
            స్థిరరాశి V3_Error_సంకేతం సంకేతం_2 = static_cast<V3_Error_సంకేతం>(i);
            ఒకవేళ (lhs_సందేశం_En.enabled(సంకేతం_2) != rhs_సందేశం_En.enabled(సంకేతం_2))
                తిరిగి rhs_సందేశం_En.enabled(సంకేతం_2) ? -1 : 1;
        }
        // TokenNum is compared last as makes more logical sort order by file/line first
        ఒకవేళ (m_token_అంకె != rhs.m_token_అంకె) తిరిగి (m_token_అంకె < rhs.m_token_అంకె) ? -1 : 1;
        తిరిగి 0;  // (*this) and rhs are equivalent
    }

అంతరంగ:
    string warnContext() స్థిరరాశి;
    string warn_Context_మాతృ() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex);
    స్థిరరాశి సందేశం_En_బిట్టు_సమితి& సందేశం_En() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి singleton().సందేశం_En(m_సందేశం_En_Idx); }
};
std::ostream& క్రియాకారకం<<(std::ostream& os, File_పంక్తి* fileline);

#అయితే_ముగింపు  // Guard
