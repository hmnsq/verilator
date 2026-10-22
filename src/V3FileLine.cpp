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

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

// clang-format off
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3String.h"
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
# include "V3Global.h"
# include "V3Control.h"
# include "V3File.h"
# include "V3Stats.h"
#అయితే_ముగింపు
#అన్తర్భావ "V3Waiver.h"
// clang-format on

#అన్తర్భావ <algorithm>
#అన్తర్భావ <iomanip>
#అన్తర్భావ <limits>
#అన్తర్భావ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// FileLineSingleton class functions

string File_పంక్తి_Singleton::filenameLetters(fileNameIdx_t fileno) VL_శుద్ధ {
    constexpr పూర్ణాంకం కొలత
        = 1 + (64 / 4);  // Each letter retires more than 4 bits of a > 64 bit number
    అక్షరం బహిర్గతం[కొలత];
    అక్షరం* op = బహిర్గతం + కొలత - 1;
    *--op = '\0';  // We build backwards
    పూర్ణాంకం అంకె = fileno;
    చేయి {
        *--op = 'a' + అంకె % 26;
        అంకె /= 26;
    } అయితే (అంకె);
    తిరిగి op;
}

//! Convert filenames to a filenameno

//! This lets us assign a nice small identifier for debug messages, but more
//! importantly lets us use a 4 byte int instead of 8 byte pointer in every
//! FileLine.

//! We associate a language with each source file, so we also set the default
//! for this.
File_పంక్తి_Singleton::fileNameIdx_t File_పంక్తి_Singleton::name_గమ్యం_సంఖ్య(స్థిరరాశి string& filename) {
    స్థిరరాశి స్వయంచాలక pair = m_namemap.emplace(filename, 0);
    fileNameIdx_t& idx = pair.మొదటి->రెండవ;
    ఒకవేళ (pair.రెండవ) {
        స్థిరరాశి కొలత_t తదుపరి_Idx = m_పేర్లు.కొలత();
        UASSERT(తదుపరి_Idx <= std::numeric_limits<fileNameIdx_t>::max(),
                "Too many input files (" + cvt_గమ్యం_Str(తదుపరి_Idx) + "+).");
        idx = static_cast<fileNameIdx_t>(తదుపరి_Idx);
        m_పేర్లు.నెట్టు_వెనుక(filename);
        m_languages.నెట్టు_వెనుక(V3_Lang_సంకేతం::mostRecent());
    }
    తిరిగి idx;
}

శూన్య File_పంక్తి_Singleton::file_Name_అంకె_Map_Dump_Json(std::ostream& os) {
    std::string sep = "\n  ";
    os << "\"files\": {";
    కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_namemap) {
        std::string filename = itr.మొదటి;
        ఒకవేళ (filename == V3Options::get_Std_Package_మార్గం()) {
            filename = "<verilated_std>";
        } లేకపోతే ఒకవేళ (filename == V3Options::get_Std_Waiver_మార్గం()) {
            filename = "<verilated_std_waiver>";
        }
        os << sep << '"' << filenameLetters(itr.రెండవ) << '"' << ": {\"filename\":\"" << filename
           << '"' << ", \"realpath\":\""
           << V3_బహిర్గతం_Formatter::quoteNameControls(V3Os::filename_Real_మార్గం(itr.మొదటి)) << '"'
           << ", \"language\":\"" << సంఖ్య_గమ్యం_Lang(itr.రెండవ).ascii() << "\"}";
        sep = ",\n  ";
    }
    os << "\n }";
}

File_పంక్తి_Singleton::సందేశం_En_సమితి_Idx_t File_పంక్తి_Singleton::కూడు_సందేశం_En_బిట్టు_సమితి(స్థిరరాశి సందేశం_En_బిట్టు_సమితి& బిట్టు_సమితి)
    VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
    స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
    స్థిరరాశి స్వయంచాలక pair = m_interned_సందేశం_En_Idxs.emplace(బిట్టు_సమితి, 0);
    సందేశం_En_సమితి_Idx_t& idx = pair.మొదటి->రెండవ;
    ఒకవేళ (pair.రెండవ) {
        స్థిరరాశి కొలత_t తదుపరి_Idx = m_interned_సందేశం_Ens.కొలత();
        UASSERT(తదుపరి_Idx <= std::numeric_limits<సందేశం_En_సమితి_Idx_t>::max(),
                "Too many unique message enable sets (" + cvt_గమ్యం_Str(తదుపరి_Idx) + "+).");
        idx = static_cast<సందేశం_En_సమితి_Idx_t>(తదుపరి_Idx);
        m_interned_సందేశం_Ens.నెట్టు_వెనుక(బిట్టు_సమితి);
    }
    తిరిగి idx;
}

File_పంక్తి_Singleton::సందేశం_En_సమితి_Idx_t File_పంక్తి_Singleton::default_సందేశం_En_సూచీ() VL_MT_సురక్షిత {
    సందేశం_En_బిట్టు_సమితి సందేశం_En_బిట్టు_సమితి_2;
    కొరకు (పూర్ణాంకం i = V3_Error_సంకేతం::EC_MIN; i < V3_Error_సంకేతం::_ENUM_MAX; ++i) {
        స్థిరరాశి V3_Error_సంకేతం సంకేతం_2{i};
        // "-Wall" and the like only adjust the code subset, so use default enablement there
        సందేశం_En_బిట్టు_సమితి_2.సమితి(సందేశం_En_బిట్టు_సమితి::Subset::సంకేతం_3, సంకేతం_2, !సంకేతం_2.defaultsOff());
        // The control file subset is only adjusted by the control files, everything enabled by
        // default.  (V3Control also likewise creates with this)
        సందేశం_En_బిట్టు_సమితి_2.సమితి(సందేశం_En_బిట్టు_సమితి::Subset::CTRL, సంకేతం_2, నిజం);
    }
    తిరిగి కూడు_సందేశం_En_బిట్టు_సమితి(సందేశం_En_బిట్టు_సమితి_2);
}

File_పంక్తి_Singleton::సందేశం_En_సమితి_Idx_t File_పంక్తి_Singleton::సందేశం_En_సమితి_బిట్టు(సందేశం_En_సమితి_Idx_t సమితి_Idx,
                                                                సందేశం_En_బిట్టు_సమితి::Subset subset,
                                                                V3_Error_సంకేతం సంకేతం_2, తార్కిక విలువ) {
    // See if state matches existing
    తార్కిక same = నిజం;
    సంకేతం_2.forDelegateCodes([&](V3_Error_సంకేతం subcode) {
        ఒకవేళ (సందేశం_En(సమితి_Idx).test(subset, subcode) != విలువ) same = అబద్ధం;
    });
    ఒకవేళ (same) తిరిగి సమితి_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    సందేశం_En_బిట్టు_సమితి సందేశం_En_బిట్టు_సమితి_2{సందేశం_En(సమితి_Idx)};
    సంకేతం_2.forDelegateCodes([&](V3_Error_సంకేతం subcode) { సందేశం_En_బిట్టు_సమితి_2.సమితి(subset, subcode, విలువ); });
    తిరిగి కూడు_సందేశం_En_బిట్టు_సమితి(సందేశం_En_బిట్టు_సమితి_2);
}

File_పంక్తి_Singleton::సందేశం_En_సమితి_Idx_t File_పంక్తి_Singleton::సందేశం_సమితి_Ctrl_బిట్టు_సమితి(సందేశం_En_సమితి_Idx_t సమితి_Idx,
                                                                     స్థిరరాశి VError_బిట్టు_సమితి& bitset) {
    స్థిరరాశి సందేశం_En_బిట్టు_సమితి::Subset subset = సందేశం_En_బిట్టు_సమితి::Subset::CTRL;
    // See if state matches existing
    ఒకవేళ (సందేశం_En(సమితి_Idx).getAll(subset) == bitset) తిరిగి సమితి_Idx;
    // Make new mask of all delegated codes at once (to avoid extra indices if looped above this)
    సందేశం_En_బిట్టు_సమితి సందేశం_En_బిట్టు_సమితి_2{సందేశం_En(సమితి_Idx)};
    సందేశం_En_బిట్టు_సమితి_2.సమితి_All(subset, bitset);
    తిరిగి కూడు_సందేశం_En_బిట్టు_సమితి(సందేశం_En_బిట్టు_సమితి_2);
}

File_పంక్తి_Singleton::సందేశం_En_సమితి_Idx_t File_పంక్తి_Singleton::సందేశం_En_And(సందేశం_En_సమితి_Idx_t lhsIdx,
                                                             సందేశం_En_సమితి_Idx_t rhsIdx) {
    స్థిరరాశి సందేశం_En_బిట్టు_సమితి& lhs = సందేశం_En(lhsIdx);
    స్థిరరాశి సందేశం_En_బిట్టు_సమితి& rhs = సందేశం_En(rhsIdx);
    స్థిరరాశి సందేశం_En_బిట్టు_సమితి intersection{lhs, rhs};
    ఒకవేళ (intersection == lhs) తిరిగి lhsIdx;
    ఒకవేళ (intersection == rhs) తిరిగి rhsIdx;
    తిరిగి కూడు_సందేశం_En_బిట్టు_సమితి(intersection);
}

// ######################################################################
//  VFileContents class functions

శూన్య VFileContent::నెట్టు_పాఠ్యం(స్థిరరాశి string& పాఠ్యం) {
    // Similar code in WildcardContents::pushText()
    ఒకవేళ (m_పంక్తులు.కొలత() == 0) {
        m_పంక్తులు.emplace_వెనుక("");  // no such thing as line [0]
        m_పంక్తులు.emplace_వెనుక("");  // start with no leftover
    }

    // Any leftover text is stored on largest line (might be "")
    స్థిరరాశి string leftover = m_పంక్తులు.వెనుక() + పాఠ్యం;
    m_పంక్తులు.తీయి_వెనుక();

    // Insert line-by-line
    string::కొలత_type పంక్తి_ప్రారంభం = 0;
    అయితే (నిజం) {
        స్థిరరాశి string::కొలత_type పంక్తి_చివర = leftover.కనుగొను('\n', పంక్తి_ప్రారంభం);
        ఒకవేళ (పంక్తి_చివర != string::npos) {
            స్థిరరాశి string oneline(leftover, పంక్తి_ప్రారంభం, పంక్తి_చివర - పంక్తి_ప్రారంభం + 1);
            m_పంక్తులు.నెట్టు_వెనుక(oneline);  // Keeps newline
            UINFO(9, "PushStream[ct" << m_id << "+" << (m_పంక్తులు.కొలత() - 1) << "]: " << oneline);
            పంక్తి_ప్రారంభం = పంక్తి_చివర + 1;
        } లేకపోతే {
            ఆపు;
        }
    }
    // Keep leftover for next time
    m_పంక్తులు.emplace_వెనుక(string(leftover, పంక్తి_ప్రారంభం));  // Might be ""
}

string VFileContent::get_పంక్తి(పూర్ణాంకం lineno) స్థిరరాశి VL_MT_సురక్షిత {
    // Return error text rather than asserting so the user isn't left without a message
    // cppcheck-suppress negativeContainerIndex
    ఒకవేళ (VL_UNCOVERABLE(lineno < 0 || lineno >= (పూర్ణాంకం)m_పంక్తులు.కొలత())) {
        ఒకవేళ (debug() || v3Global.opt.debugCheck()) {
            తిరిగి ("%Error-internal-contents-bad-ct" + cvt_గమ్యం_Str(m_id) + "-ln" + cvt_గమ్యం_Str(lineno));
        } లేకపోతే {
            తిరిగి "";
        }
    }
    string పాఠ్యం = m_పంక్తులు[lineno];
    UINFO(9, "Get Stream[ct" << m_id << "+" << lineno << "]: " << పాఠ్యం);
    తిరిగి పాఠ్యం;
}

std::ostream& క్రియాకారకం<<(std::ostream& os, VFileContent* contentp) {
    ఒకవేళ (!contentp) {
        os << "ct0";
    } లేకపోతే {
        os << contentp->ascii();
    }
    తిరిగి os;
}

// ######################################################################
//  FileLine class functions

File_పంక్తి::~File_పంక్తి() {
    ఒకవేళ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
}

శూన్య File_పంక్తి::newContent() {
    ఒకవేళ (m_contentp) VL_DO_DANGLING(m_contentp->refDec(), m_contentp);
    m_contentp = కొత్త VFileContent;
    m_contentp->refInc();
    m_contentLineno = 1;
}

string File_పంక్తి::పంక్తి_Directive_Strg(పూర్ణాంకం enterExit) స్థిరరాశి {
    తిరిగి "`line "s + cvt_గమ్యం_Str(చివరి_Lineno()) + " \""
           + V3_బహిర్గతం_Formatter::quoteNameControls(filename()) + "\" " + cvt_గమ్యం_Str(enterExit) + '\n';
}

శూన్య File_పంక్తి::పంక్తి_Directive(స్థిరరాశి అక్షరం* textp, పూర్ణాంకం& enterExitRef) {
    string newFilename;
    పూర్ణాంకం newLineno = -1;
    పంక్తి_Directive_Parse(textp, newFilename /*ref*/, newLineno /*ref*/, enterExitRef);
    ఒకవేళ (enterExitRef != -1) {
        filename(newFilename);
        lineno(newLineno);
    } లేకపోతే {  // Advance line number to account for bogus `line
        linenoInc();
    }
}

శూన్య File_పంక్తి::పంక్తి_Directive_Parse(స్థిరరాశి అక్షరం* textp, string& filenameRef, పూర్ణాంకం& linenoRef,
                                  పూర్ణాంకం& enterExitRef) {
    // Handle `line directive
    // Does not parse streamNumber/streamLineno as the next input token
    // will come from the same stream as the previous line.
    చేయి {
        // Skip `line
        అయితే (*textp && std::ఖాళీనా(*textp)) ++textp;
        అయితే (*textp && !std::ఖాళీనా(*textp)) ++textp;
        అయితే (*textp && std::ఖాళీనా(*textp)) ++textp;

        // Grab linenumber
        పూర్ణాంకం పంక్తి_No;
        స్థిరరాశి అక్షరం* స్థిరరాశి ln = textp;
        అయితే (*textp && !std::ఖాళీనా(*textp)) ++textp;
        ఒకవేళ (0 == అక్షరశ్రేణి_పోలిక(ln, "`__LINE__", textp - ln)) {
            // Special case - see docs - don't change other than accounting for `line itself
            పంక్తి_No = lineno() + 1;
        } లేకపోతే ఒకవేళ (std::అంకెనా(*ln)) {
            పంక్తి_No = std::atoi(ln);
        } లేకపోతే {
            ఆపు;  // Fail
        }
        అయితే (*textp && (std::ఖాళీనా(*textp))) ++textp;

        // Grab filename
        ఒకవేళ (*textp != '"') ఆపు;  // Fail
        స్థిరరాశి అక్షరం* స్థిరరాశి fn = ++textp;
        అయితే (*textp && *textp != '"') ++textp;
        ఒకవేళ (*textp != '"') ఆపు;  // Fail
        string err_సందేశం;
        స్థిరరాశి string& parsedFilename = VString::unquoteSVString(string{fn, textp}, err_సందేశం);
        ఒకవేళ (!err_సందేశం.ఖాళీలు()) ఇది->v3error(err_సందేశం.c_str());
        ++textp;
        అయితే (*textp && std::ఖాళీనా(*textp)) ++textp;

        // Grab level
        ఒకవేళ (!std::అంకెనా(*textp)) ఆపు;  // Fail
        స్థిరరాశి పూర్ణాంకం level = std::atoi(textp);
        ఒకవేళ (level < 0 || level >= 3) ఆపు;  // Fail

        linenoRef = పంక్తి_No;
        filenameRef = parsedFilename;
        enterExitRef = level;
        తిరిగి;
    } అయితే (అబద్ధం);

    // Fail
    v3error("`line was not properly formed with '`line number \"filename\" level'\n");
    enterExitRef = -1;
}

శూన్య File_పంక్తి::forwardToken(స్థిరరాశి అక్షరం* textp, కొలత_t కొలత, తార్కిక track_పంక్తులు) {
    స్థిర పూర్ణాంకం s_token_అంకె = 1;
    m_token_అంకె = s_token_అంకె++;
    కొరకు (స్థిరరాశి అక్షరం* sp = textp; కొలత && *sp; ++sp, --కొలత) {
        ఒకవేళ (*sp == '\n') {
            ఒకవేళ (track_పంక్తులు) linenoInc();
            m_చివరి_స్తంభం = 1;
        } లేకపోతే ఒకవేళ (VL_UNCOVERABLE(*sp == '\r')) {  // Generally stripped by preproc
        } లేకపోతే {  // Tabs are considered one column; hence column means number of chars
            ++m_చివరి_స్తంభం;
        }
    }
}

శూన్య File_పంక్తి::applyIgnores() {
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
    V3Control::applyIgnores(ఇది);  // Toggle warnings based on global config file
#అయితే_ముగింపు
}

File_పంక్తి* File_పంక్తి::copy_Or_Same_File_పంక్తి_Applied() {
    applyIgnores();
    తిరిగి copy_Or_Same_File_పంక్తి();
}

File_పంక్తి* File_పంక్తి::copy_Or_Same_File_పంక్తి() {
    // When a fileline is "used" to produce a node, calls this function.
    // Return this, or a copy of this
    // There are often more than one token per line, thus we use the
    // same pointer as long as we're on the same line, file & warn state.
    స్థిర File_పంక్తి* s_చివరి_Newp = శూన్యసూచిక;
    ఒకవేళ (s_చివరి_Newp && *s_చివరి_Newp == *ఇది) {  // Compares lineno, filename, etc
        తిరిగి s_చివరి_Newp;
    }
    File_పంక్తి* స్థిరరాశి newp = కొత్త File_పంక్తి{ఇది};
    s_చివరి_Newp = newp;
    తిరిగి newp;
}

string File_పంక్తి::filebasename() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి V3Os::filenameNonDir(filename()); }

string File_పంక్తి::filebasenameNoExt() స్థిరరాశి { తిరిగి V3Os::filenameNonDirExt(filename()); }

string File_పంక్తి::మొదటి_స్తంభం_Letters() స్థిరరాశి VL_MT_సురక్షిత {
    స్థిరరాశి అక్షరం a = ((మొదటి_స్తంభం() / 26) % 26) + 'a';
    స్థిరరాశి అక్షరం b = (మొదటి_స్తంభం() % 26) + 'a';
    తిరిగి string(1, a) + string(1, b);
}

string File_పంక్తి::profileFuncname() స్థిరరాశి {
    // Return string that is OK as a function name - for profiling
    string name = filebasenameNoExt();
    string::కొలత_type pos;
    అయితే ((pos = name.కనుగొను_మొదటి_not_యొక్క(
                "abcdefghijlkmnopqrstuvwxyzABCDEFGHIJLKMNOPQRSTUVWXYZ0123456789_"))
           != string::npos) {
        name.replace(pos, 1, "_");
    }
    name += "__l" + cvt_గమ్యం_Str(చివరి_Lineno());
    తిరిగి name;
}

string File_పంక్తి::ascii_పంక్తి_నిలువు() స్థిరరాశి {
    తిరిగి (cvt_గమ్యం_Str(మొదటి_Lineno()) + "-" + cvt_గమ్యం_Str(చివరి_Lineno()) + ":" + cvt_గమ్యం_Str(మొదటి_స్తంభం())
            + "-" + cvt_గమ్యం_Str(చివరి_స్తంభం()) + "[" + (m_contentp ? m_contentp->ascii() : "ct0") + "+"
            + cvt_గమ్యం_Str(m_contentLineno) + "]");
}
string File_పంక్తి::ascii() స్థిరరాశి {
    // For most errors especially in the parser the lastLineno is more accurate than firstLineno
    తిరిగి filename() + ":" + cvt_గమ్యం_Str(చివరి_Lineno()) + ":" + cvt_గమ్యం_Str(మొదటి_స్తంభం());
}
std::ostream& క్రియాకారకం<<(std::ostream& os, File_పంక్తి* fileline) {
    os << fileline->ascii() << ": " << std::hex;
    తిరిగి (os);
}

string File_పంక్తి::warnOffParse(స్థిరరాశి string& msgs, తార్కిక turnOff) {
    string ఫలితం;
    కొరకు (స్థిరరాశి string& సందేశం_2 : VString::విభజించు(msgs, ',')) {
        స్థిరరాశి V3_Error_సంకేతం సంకేతం_2{సందేశం_2};
        ఒకవేళ (!సంకేతం_2.hardError()) {
            warnOff(సంకేతం_2, turnOff);
            కొనసాగు;
        }
        // Error if not suppressed
        ఒకవేళ (!v3Global.opt.isFuture(సందేశం_2)) ఫలితం = VString::dot(ఫలితం, ",", సందేశం_2);
    }
    తిరిగి ఫలితం;
}

తార్కిక File_పంక్తి::warnIsOff(V3_Error_సంకేతం సంకేతం_2) స్థిరరాశి {
    ఒకవేళ (!సందేశం_En().enabled(సంకేతం_2)) తిరిగి నిజం;
    ఒకవేళ (!default_File_పంక్తి().సందేశం_En().enabled(సంకేతం_2)) తిరిగి నిజం;  // Global overrides local
    తిరిగి అబద్ధం;
}

// cppverilator-suppress constParameter
శూన్య File_పంక్తి::v3error_చివర(std::ostringstream& sstr, స్థిరరాశి string& extra)
    VL_RELEASE(V3Error::s().m_mutex) {
    // 'extra' is appended to the message, and is is excluded in check for
    // duplicate messages. Currently used for reporting instance name.
    std::ostringstream nsstr;  // sstr with fileline prefix and context
    std::ostringstream wsstr;  // sstr for waiver (no fileline) with context
    ఒకవేళ (చివరి_Lineno()) nsstr << V3Error::warnContextBegin() << ఇది << V3Error::warn_Context_చివర();
    nsstr << sstr.str();
    wsstr << sstr.str();
    nsstr << '\n';
    wsstr << '\n';
    std::ostringstream extrass;  // extra spaced out for prefix
    ఒకవేళ (!extra.ఖాళీలు()) {
        extrass << V3Error::warnContextBegin() << std::setw(ascii().నిడివి()) << " " << ": "
                << V3Error::warn_Context_చివర() << extra;
    }
    ఒకవేళ (warnIsOff(V3Error::s().error_సంకేతం())) {
        V3Error::s().suppressThisWarning();
    } లేకపోతే {
        ఒకవేళ (!V3Error::s().errorContexted()) {
            స్థిరరాశి string కూడు = warnContextPrimary();
            wsstr << కూడు;
            nsstr << కూడు;
        }
        స్థిరరాశి string waiver_పాఠ్యం = V3Error::strip_Meta_పాఠ్యం(wsstr.str(), అబద్ధం);
        m_waive = V3Control::waive(ఇది, V3Error::s().error_సంకేతం(), waiver_పాఠ్యం);
        ఒకవేళ (m_waive) {
            V3Error::s().suppressThisWarning();
        } లేకపోతే {
            V3Waiver::కూడు_నమోదు(V3Error::s().error_సంకేతం(), filename(), waiver_పాఠ్యం);
        }
    }
    V3Error::v3error_చివర(nsstr, extrass.str(), ఇది);
}

string File_పంక్తి::warn_మరిన్ని() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex) {
    ఒకవేళ (చివరి_Lineno()) {
        తిరిగి V3Error::warnContextBegin() + V3Error::warn_మరిన్ని() + string(ascii().కొలత(), ' ')
               + ": " + V3Error::warn_Context_చివర();
    } లేకపోతే {
        తిరిగి V3Error::warn_మరిన్ని();
    }
}
string File_పంక్తి::warnOther() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex) {
    ఒకవేళ (చివరి_Lineno()) {
        తిరిగి V3Error::s().warnRelated(ఇది) + V3Error::warnContextBegin() + V3Error::warn_మరిన్ని()
               + ascii() + ": " + V3Error::warn_Context_చివర();
    } లేకపోతే {
        తిరిగి V3Error::warn_మరిన్ని();
    }
};

string File_పంక్తి::ఆధారం() స్థిరరాశి VL_MT_సురక్షిత {
    ఒకవేళ (VL_UNCOVERABLE(!m_contentp)) {  // LCOV_EXCL_START
        ఒకవేళ (debug() || v3Global.opt.debugCheck()) {
            // The newline here is to work around the " <line#> | "
            తిరిగి "\n%Error: internal tracking of file contents failed";
        } లేకపోతే {
            తిరిగి "";
        }
    }  // LCOV_EXCL_STOP
    తిరిగి m_contentp->get_పంక్తి(m_contentLineno);
}
string File_పంక్తి::ఆధారం_ఉపసర్గ(పూర్ణాంకం గమ్యం_స్తంభం) స్థిరరాశి VL_MT_సురక్షిత {
    స్థిరరాశి std::string src = ఆధారం();
    ఒకవేళ (గమ్యం_స్తంభం > static_cast<పూర్ణాంకం>(src.నిడివి())) గమ్యం_స్తంభం = static_cast<పూర్ణాంకం>(src.నిడివి());
    ఒకవేళ (గమ్యం_స్తంభం < 1) తిరిగి "";
    తిరిగి src.substr(0, గమ్యం_స్తంభం - 1);
}
string File_పంక్తి::pretty_ఆధారం() స్థిరరాశి VL_MT_సురక్షిత {
    string బహిర్గతం = ఆధారం();
    // Drop ignore trailing newline
    స్థిరరాశి string::కొలత_type pos = బహిర్గతం.కనుగొను('\n');
    ఒకవేళ (pos != string::npos) బహిర్గతం = string(బహిర్గతం, 0, pos);
    // Column tracking counts tabs = 1, so match that when print source
    తిరిగి VString::spaceUnprintable(బహిర్గతం);
}

string File_పంక్తి::warnContext() స్థిరరాశి {
    ఒకవేళ (!v3Global.opt.context()) తిరిగి "";
    ఒకవేళ (మొదటి_Lineno() == చివరి_Lineno() && మొదటి_స్తంభం()) {
        స్థిరరాశి string ఆధారం_పంక్తి = pretty_ఆధారం();
        string బహిర్గతం;
        // Don't show super-long lines as can fill screen and unlikely to help user
        ఒకవేళ (!ఆధారం_పంక్తి.ఖాళీలు() && ఆధారం_పంక్తి.నిడివి() < SHOW_ఆధారం_MAX_నిడివి
            && ఆధారం_పంక్తి.నిడివి() >= static_cast<కొలత_t>(చివరి_స్తంభం() - 1)) {
            string linestr = cvt_గమ్యం_Str(మొదటి_Lineno());
            అయితే (linestr.కొలత() < 5) linestr = ' ' + linestr;
            బహిర్గతం += linestr + " | " + ఆధారం_పంక్తి + '\n';
            బహిర్గతం += std::string(linestr.కొలత(), ' ') + " | ";
            బహిర్గతం += string((మొదటి_స్తంభం() - 1), ' ') + '^';
            // Can't use UASSERT_OBJ used in warnings already inside the error end handler
            ఒకవేళ (చివరి_స్తంభం() > మొదటి_స్తంభం()) {
                // Note lastColumn() can be <= firstColumn() in some weird preproc expansions
                బహిర్గతం += string((చివరి_స్తంభం() - మొదటి_స్తంభం() - 1), '~');
            }
            బహిర్గతం += '\n';
        }
        తిరిగి V3Error::warnContextBegin() + బహిర్గతం + V3Error::warn_Context_చివర();
    }
    తిరిగి "";
}

string File_పంక్తి::warn_Context_మాతృ() స్థిరరాశి VL_REQUIRES(V3Error::s().m_mutex) {
    string ఫలితం;
    కొరకు (File_పంక్తి* మాతృ_Fl = మాతృ(); మాతృ_Fl; మాతృ_Fl = మాతృ_Fl->మాతృ()) {
        ఒకవేళ (మాతృ_Fl->filenameIsGlobal()) ఆపు;
        ఫలితం += మాతృ_Fl->warnOther() + "... note: In file included from '"
                  + మాతృ_Fl->filebasename() + "'\n";
    }
    తిరిగి ఫలితం;
}

#నిర్వచితమైతే VL_LEAK_CHECKS
std::unordered_సమితి<File_పంక్తి*> file_పంక్తి_Leak_Checks;

శూన్య* File_పంక్తి::క్రియాకారకం కొత్త(కొలత_t కొలత) {
    File_పంక్తి* స్థిరరాశి objp = static_cast<File_పంక్తి*>(::క్రియాకారకం కొత్త(కొలత));
    file_పంక్తి_Leak_Checks.చొప్పించు(objp);
    తిరిగి objp;
}

శూన్య File_పంక్తి::క్రియాకారకం తొలగించు(శూన్య* objp, కొలత_t కొలత) {
    ఒకవేళ (!objp) తిరిగి;
    File_పంక్తి* స్థిరరాశి flp = static_cast<File_పంక్తి*>(objp);
    స్థిరరాశి స్వయంచాలక it = file_పంక్తి_Leak_Checks.కనుగొను(flp);
    ఒకవేళ (it != file_పంక్తి_Leak_Checks.చివర()) {
        file_పంక్తి_Leak_Checks.erase(it);
    } లేకపోతే {
        flp->v3fatalSrc("Deleting FileLine object that was never tracked");
    }
    ::క్రియాకారకం తొలగించు(objp);
}
#అయితే_ముగింపు

శూన్య File_పంక్తి::stats() {
#నిర్వచితం_కాకపోతే V3ERROR_NO_GLOBAL_
    V3Stats::కూడు_Stat_కూడిక("FileLines, Number of filenames",
                        singleton().m_పేర్లు.కొలత());  // Max m_filenameno
    V3Stats::కూడు_Stat_కూడిక("FileLines, Message enable sets",
                        singleton().m_interned_సందేశం_Ens.కొలత());  // Max m_msgEnIdx
    // Don't currently have a good path to recording max line/column,
    // Infrequently useful, alternatively we could keep globals we update as make each FileLine
    // or could use fileLineLeakChecks.
#అయితే_ముగింపు
}

శూన్య File_పంక్తి::delete_All_మిగిలిన() {
#నిర్వచితమైతే VL_LEAK_CHECKS
    // FileLines are allocated, but never nicely freed, as it's much faster
    // that way.  Unfortunately this makes our leak checking a big mess, so
    // only when leak checking we'll track them all and cleanup.
    అయితే (నిజం) {
        స్థిరరాశి స్వయంచాలక it = file_పంక్తి_Leak_Checks.begin();
        ఒకవేళ (it == file_పంక్తి_Leak_Checks.చివర()) ఆపు;
        తొలగించు *it;
        // Operator delete will remove the iterated object from the list.
        // Eventually the list will be empty and terminate the loop.
    }
    file_పంక్తి_Leak_Checks.clear();
    singleton().clear();
#అయితే_ముగింపు
}
