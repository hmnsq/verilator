// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
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

#నిర్వచితం_కాకపోతే VERILATOR_V3FILE_H_
#నిర్వచించు VERILATOR_V3FILE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Stats.h"

#అన్తర్భావ <array>
#అన్తర్భావ <fstream>
#అన్తర్భావ <list>
#అన్తర్భావ <memory>
#అన్తర్భావ <set>
#అన్తర్భావ <stack>
#అన్తర్భావ <vector>

class Ast_కణుపు;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
బహిరంగ:
    స్థిర std::ifstream* new_ifstream(స్థిరరాశి string& filename) {
        కూడు_Src_Depend(filename);
        తిరిగి new_ifstream_nodepend(filename);
    }
    స్థిర std::ifstream* new_ifstream_nodepend(స్థిరరాశి string& filename) VL_MT_సురక్షిత {
        తిరిగి new std::ifstream{filename.c_str()};
    }
    స్థిర std::ofstream* new_ofstream(స్థిరరాశి string& filename, bool append = false) {
        కూడు_Tgt_Depend(filename);
        తిరిగి new_ofstream_nodepend(filename, append);
    }
    స్థిర std::ofstream* new_ofstream_nodepend(స్థిరరాశి string& filename, bool append = false) {
        createMakeDirFor(filename);
        ఒకవేళ (append) తిరిగి new std::ofstream{filename.c_str(), std::ios::app};
        తిరిగి new std::ofstream{filename.c_str()};
    }
    స్థిర FILE* new_fopen_w(స్థిరరాశి string& filename) {
        createMakeDirFor(filename);
        కూడు_Tgt_Depend(filename);
        తిరిగి fopen(filename.c_str(), "w");
    }

    // Dependencies
    స్థిర శూన్య కూడు_Src_Depend(స్థిరరాశి string& filename) VL_MT_సురక్షిత;
    స్థిర శూన్య కూడు_Tgt_Depend(స్థిరరాశి string& filename) VL_MT_సురక్షిత;
    స్థిర శూన్య writeDepend(స్థిరరాశి string& filename);
    స్థిర std::సదిశ<string> getAllDeps();
    స్థిర శూన్య writeTimes(స్థిరరాశి string& filename, స్థిరరాశి string& cmdline_లోపల);
    స్థిర bool checkTimes(స్థిరరాశి string& filename, స్థిరరాశి string& cmdline_లోపల);

    // Directory utilities
    స్థిర శూన్య createMakeDirFor(స్థిరరాశి string& filename);
    స్థిర శూన్య createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
బహిరంగ:
    // TYPES
    using StrList = std::list<std::string>;

అంతరంగ:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

బహిరంగ:
    explicit VInFilter(స్థిరరాశి string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(స్థిరరాశి string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3_బహిర్గతం_Formatter VL_NOT_FINAL {
    // TYPES
బహిరంగ:
    గణన AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    గణన Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

అంతరంగ:
    // MEMBERS
    స్థిరరాశి Language m_lang;  // Indenting Verilog code
    పూర్ణాంకం m_ఖండం_Indent;  // Characters per block indent
    పూర్ణాంకం m_comma_వెడల్పు;  // Width after which to break at ,'s
    పూర్ణాంకం m_lineno = 1;
    పూర్ణాంకం m_స్తంభం = 0;
    పూర్ణాంకం m_nobreak = false;  // Basic operator or begin paren, don't break next
    పూర్ణాంకం m_ఆధారం_చివరి_Lineno = 0;
    పూర్ణాంకం m_ఆధారం_చివరి_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_లోపల_String_Literal = false;  // Inside a quote
    bool m_లోపల_Backslash = false;  // Inside a backslash
    పూర్ణాంకం m_indentLevel = 0;  // Current {} indentation
    std::దొంతర<పూర్ణాంకం> m_parenVec;  // Stack of columns where last ( was
    పూర్ణాంకం m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    పూర్ణాంకం చివర_స్థాయిలు(స్థిరరాశి అక్షరం* strg);
    శూన్య putcNoTracking(అక్షరం chr);

బహిరంగ:
    explicit V3_బహిర్గతం_Formatter(Language lang);
    మాయా ~V3_బహిర్గతం_Formatter() = అప్రమేయ;
    // ACCESSORS
    శూన్య ఖండం_Indent(పూర్ణాంకం flag) { m_ఖండం_Indent = flag; }
    // METHODS
    శూన్య రాయి(స్థిరరాశి అక్షరం* fmt...) VL_ATTR_PRINTF(2);
    శూన్య puts(స్థిరరాశి అక్షరం* strg) { putns(nullptr, strg); }
    శూన్య puts(స్థిరరాశి string& strg) { putns(nullptr, strg); }
    శూన్య putns(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి అక్షరం* strg);
    శూన్య putns(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string& strg) { putns(nodep, strg.c_str()); }
    శూన్య putsNoTracking(స్థిరరాశి string& strg);
    శూన్య putsQuoted(స్థిరరాశి string& strg);
    శూన్య putBreak();  // Print linebreak if line is too wide
    శూన్య put_Break_సమాసం();  // Print linebreak in expression if line is too wide
    శూన్య putbs(స్థిరరాశి అక్షరం* strg) {
        put_Break_సమాసం();
        putns(nullptr, strg);
    }
    శూన్య putbs(స్థిరరాశి string& strg) {
        put_Break_సమాసం();
        putns(nullptr, strg);
    }
    శూన్య putnbs(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string& strg) {
        put_Break_సమాసం();
        putns(nodep, strg);
    }
    bool exceeded_వెడల్పు() స్థిరరాశి { తిరిగి m_స్తంభం > m_comma_వెడల్పు; }
    శూన్య indentInc() { m_indentLevel += m_ఖండం_Indent; }
    శూన్య indentDec() {
        m_indentLevel -= m_ఖండం_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    శూన్య ఖండం_Inc() { m_parenVec.నెట్టు(m_indentLevel + m_ఖండం_Indent); }
    శూన్య ఖండం_Dec() {
        ఒకవేళ (!m_parenVec.ఖాళీలు()) m_parenVec.తీయి();
    }
    శూన్య ensure_New_పంక్తి() {
        ఒకవేళ (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    స్థిర స్థిరరాశి std::string& indentSpaces(పూర్ణాంకం అంకె);
    // Add escaped characters to strings
    స్థిర string quoteNameControls(స్థిరరాశి string& namein, Language lang = LA_C) VL_శుద్ధ;
    స్థిర bool token_సరిపోలిక(స్థిరరాశి అక్షరం* cp, స్థిరరాశి అక్షరం* cmp);
    స్థిర bool token_Not_ప్రారంభం(స్థిరరాశి అక్షరం* cp);  // Import/export meaning no endfunction
    స్థిర bool token_ప్రారంభం(స్థిరరాశి అక్షరం* cp);
    స్థిర bool token_చివర(స్థిరరాశి అక్షరం* cp);

    // CALLBACKS - MUST OVERRIDE
    మాయా శూన్య putcOutput(అక్షరం chr) = 0;
    మాయా శూన్య putsOutput(స్థిరరాశి అక్షరం* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_బహిర్గతం_File VL_NOT_FINAL : బహిరంగ V3_బహిర్గతం_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    స్థిర constexpr std::కొలత_t WRITE_బఫరు_కొలత_బైట్లు = 128 * 1024;

    // MEMBERS
    స్థిరరాశి std::string m_filename;
    FILE* m_fp = nullptr;
    std::కొలత_t m_వాడిన_బైట్లు = 0;  // Number of bytes stored in m_bufferp
    std::కొలత_t m_written_బైట్లు = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<అక్షరం, WRITE_బఫరు_కొలత_బైట్లు>> m_bufferp;  // Write buffer

బహిరంగ:
    V3_బహిర్గతం_File(స్థిరరాశి string& filename, V3_బహిర్గతం_Formatter::Language lang);
    V3_బహిర్గతం_File(స్థిరరాశి V3_బహిర్గతం_File&) = delete;
    V3_బహిర్గతం_File& క్రియాకారకం=(స్థిరరాశి V3_బహిర్గతం_File&) = delete;
    V3_బహిర్గతం_File(V3_బహిర్గతం_File&&) = delete;
    V3_బహిర్గతం_File& క్రియాకారకం=(V3_బహిర్గతం_File&&) = delete;
    ~V3_బహిర్గతం_File() override;

    std::string filename() స్థిరరాశి { తిరిగి m_filename; }

    శూన్య putsForceIncs();

    శూన్య stat_రికార్డు_Written() {
        write_ఖండం();
        V3Stats::కూడు_Stat_కూడిక(V3Stats::STAT_CPP_CHARS, m_written_బైట్లు);
    }

అంతరంగ:
    శూన్య write_ఖండం() {
        ఒకవేళ (VL_LIKELY(m_వాడిన_బైట్లు > 0)) {
            fwrite(m_bufferp->దత్తాంశం(), m_వాడిన_బైట్లు, 1, m_fp);
            m_written_బైట్లు += m_వాడిన_బైట్లు;
            m_వాడిన_బైట్లు = 0;
        }
    }
    // CALLBACKS
    శూన్య putcOutput(అక్షరం chr) override {
        m_bufferp->at(m_వాడిన_బైట్లు++) = chr;
        ఒకవేళ (VL_UNLIKELY(m_వాడిన_బైట్లు >= WRITE_బఫరు_కొలత_బైట్లు)) write_ఖండం();
    }
    శూన్య putsOutput(స్థిరరాశి అక్షరం* str) override {
        std::కొలత_t len = అక్షరశ్రేణి_పొడవు(str);
        std::కొలత_t అందుబాటు_బైట్లు = WRITE_బఫరు_కొలత_బైట్లు - m_వాడిన_బైట్లు;
        అయితే (VL_UNLIKELY(len >= అందుబాటు_బైట్లు)) {
            std::జ్ఞాపకం_నకలు(m_bufferp->దత్తాంశం() + m_వాడిన_బైట్లు, str, అందుబాటు_బైట్లు);
            m_వాడిన_బైట్లు = WRITE_బఫరు_కొలత_బైట్లు;
            write_ఖండం();
            str += అందుబాటు_బైట్లు;
            len -= అందుబాటు_బైట్లు;
            అందుబాటు_బైట్లు = WRITE_బఫరు_కొలత_బైట్లు;
        }
        ఒకవేళ (len > 0) {
            std::జ్ఞాపకం_నకలు(m_bufferp->దత్తాంశం() + m_వాడిన_బైట్లు, str, len);
            m_వాడిన_బైట్లు += len;
        }
    }
};

class V3_బహిర్గతం_CFile VL_NOT_FINAL : బహిరంగ V3_బహిర్గతం_File {
    పూర్ణాంకం m_guard = false;  // Created header guard
    పూర్ణాంకం m_వ్యక్తిగత;  // 1 = Most recently emitted private:, 2 = public:
బహిరంగ:
    explicit V3_బహిర్గతం_CFile(స్థిరరాశి string& filename,
                        V3_బహిర్గతం_Formatter::Language lang = V3_బహిర్గతం_Formatter::LA_C)
        : V3_బహిర్గతం_File{filename, lang} {
        reset_వ్యక్తిగత();
    }
    ~V3_బహిర్గతం_CFile() override { stat_రికార్డు_Written(); }
    మాయా శూన్య puts_శీర్షిక() { puts("// Verilated -*- C++ -*-\n"); }
    మాయా శూన్య puts_Int_శిఖరం_Include() { putsForceIncs(); }
    మాయా శూన్య putsGuard();
    మాయా శూన్య puts_చివర_Guard() {
        ఒకవేళ (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    శూన్య reset_వ్యక్తిగత() { m_వ్యక్తిగత = 0; }
    శూన్య puts_వ్యక్తిగత(bool సమితి_వ్యక్తిగత) {
        ఒకవేళ (సమితి_వ్యక్తిగత && m_వ్యక్తిగత != 1) {
            puts("private:\n");
            m_వ్యక్తిగత = 1;
        } లేకపోతే ఒకవేళ (!సమితి_వ్యక్తిగత && m_వ్యక్తిగత != 2) {
            puts("public:\n");
            m_వ్యక్తిగత = 2;
        }
    }
};

class V3_బహిర్గతం_Json_File final : బహిరంగ V3_బహిర్గతం_File {
    // CONSTANTS
    స్థిర constexpr స్థిరరాశి అక్షరం* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
అంతరంగ:
    std::దొంతర<అక్షరం> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_ఉపసర్గ;  // Prefix emitted before each line in current scope
    std::దొంతర<bool> m_ఖాళీలు;  // Current scope is empty, no comma later

బహిరంగ:
    explicit V3_బహిర్గతం_Json_File(స్థిరరాశి string& filename)
        : V3_బహిర్గతం_File{filename, V3_బహిర్గతం_Formatter::LA_JSON} {
        begin();
    }
    ~V3_బహిర్గతం_Json_File() override {
        చివర();
        puts("\n");
    }
    మాయా శూన్య puts_శీర్షిక() {}
    // cppcheck-suppress duplInheritedMember
    శూన్య puts(స్థిరరాశి అక్షరం* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    శూన్య puts(స్థిరరాశి string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_బహిర్గతం_Json_File& begin(స్థిరరాశి std::string& name, అక్షరం type = '{') {
        comma();
        puts(m_ఉపసర్గ + "\"" + name + "\": " + type + "\n");
        m_ఉపసర్గ += INDENT;
        m_scope.నెట్టు(type == '{' ? '}' : ']');
        ఒకవేళ (!m_ఖాళీలు.ఖాళీలు()) m_ఖాళీలు.శిఖరం() = false;
        m_ఖాళీలు.నెట్టు(true);
        తిరిగి *this;
    }
    V3_బహిర్గతం_Json_File& begin(అక్షరం type = '{') {
        comma();
        puts(m_ఉపసర్గ + type + "\n");
        m_ఉపసర్గ += INDENT;
        m_scope.నెట్టు(type == '{' ? '}' : ']');
        ఒకవేళ (!m_ఖాళీలు.ఖాళీలు()) m_ఖాళీలు.శిఖరం() = false;
        m_ఖాళీలు.నెట్టు(true);
        తిరిగి *this;
    }

    // Put a named value
    V3_బహిర్గతం_Json_File& put(స్థిరరాశి std::string& name, స్థిరరాశి అక్షరం* valuep) {
        తిరిగి putNamed(name, std::string{valuep}, true);
    }
    V3_బహిర్గతం_Json_File& put(స్థిరరాశి std::string& name, స్థిరరాశి std::string& విలువ) {
        తిరిగి putNamed(name, విలువ, true);
    }
    V3_బహిర్గతం_Json_File& put(స్థిరరాశి std::string& name, bool విలువ) {
        తిరిగి putNamed(name, విలువ ? "true" : "false", false);
    }
    V3_బహిర్గతం_Json_File& put(స్థిరరాశి std::string& name, పూర్ణాంకం విలువ) {
        తిరిగి putNamed(name, std::గమ్యం_string(విలువ), false);
    }

    // Put unnamed value
    V3_బహిర్గతం_Json_File& put(స్థిరరాశి std::string& విలువ) { తిరిగి putNamed("", విలువ, true); }
    V3_బహిర్గతం_Json_File& put(bool విలువ) { తిరిగి putNamed("", విలువ ? "true" : "false", false); }
    V3_బహిర్గతం_Json_File& put(పూర్ణాంకం విలువ) { తిరిగి putNamed("", std::గమ్యం_string(విలువ), false); }

    V3_బహిర్గతం_Json_File& putList(స్థిరరాశి std::string& name, స్థిరరాశి std::సదిశ<std::string>& list) {
        ఒకవేళ (list.ఖాళీలు()) తిరిగి *this;
        begin(name, '[');
        కొరకు (స్థిరరాశి std::string& str : list) put(str);
        తిరిగి చివర();
    }

    V3_బహిర్గతం_Json_File& చివర() {
        UASSERT(m_ఉపసర్గ.నిడివి() >= అక్షరశ్రేణి_పొడవు(INDENT), "prefix underflow");
        m_ఉపసర్గ.erase(m_ఉపసర్గ.చివర() - అక్షరశ్రేణి_పొడవు(INDENT), m_ఉపసర్గ.చివర());
        UASSERT(!m_scope.ఖాళీలు(), "end() without begin()");
        UASSERT(!m_ఖాళీలు.ఖాళీలు(), "end() without begin()");
        puts("\n" + m_ఉపసర్గ + m_scope.శిఖరం());
        m_scope.తీయి();
        m_ఖాళీలు.తీయి();
        తిరిగి *this;
    }

    V3_బహిర్గతం_Json_File& క్రియాకారకం+=(V3_బహిర్గతం_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        తిరిగి *this;
    }

అంతరంగ:
    శూన్య comma() {
        ఒకవేళ (m_ఖాళీలు.ఖాళీలు()) తిరిగి;
        ఒకవేళ (!m_ఖాళీలు.శిఖరం()) puts(",\n");
        m_ఖాళీలు.శిఖరం() = true;
    }
    V3_బహిర్గతం_Json_File& putNamed(స్థిరరాశి std::string& name, స్థిరరాశి std::string& విలువ, bool quoted) {
        comma();
        స్థిరరాశి string విలువ_Q
            = quoted ? "\""s + V3_బహిర్గతం_Formatter::quoteNameControls(విలువ) + "\"" : విలువ;
        ఒకవేళ (name.ఖాళీలు()) {
            puts(m_ఉపసర్గ + విలువ_Q);
        } లేకపోతే {
            puts(m_ఉపసర్గ + "\"" + name + "\": " + విలువ_Q);
        }
        m_ఖాళీలు.శిఖరం() = false;
        తిరిగి *this;
    }
};

class V3_బహిర్గతం_Mk_File final : బహిరంగ V3_బహిర్గతం_File {
బహిరంగ:
    explicit V3_బహిర్గతం_Mk_File(స్థిరరాశి string& filename)
        : V3_బహిర్గతం_File{filename, V3_బహిర్గతం_Formatter::LA_MK} {}
    ~V3_బహిర్గతం_Mk_File() override = అప్రమేయ;
    మాయా శూన్య puts_శీర్షిక() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    శూన్య puts(స్థిరరాశి అక్షరం* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    శూన్య puts(స్థిరరాశి string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    శూన్య put_సమితి(స్థిరరాశి string& var, స్థిరరాశి string& విలువ) {
        puts(VString::dot(var + " =", " ", విలువ) + "\n");
    }
    // Put VARIABLE ?= VALUE
    శూన్య put_సమితి_Q(స్థిరరాశి string& var, స్థిరరాశి string& విలువ) {
        puts(VString::dot(var + " ?=", " ", విలువ) + "\n");
    }
};

class V3_బహిర్గతం_VFile final : బహిరంగ V3_బహిర్గతం_CFile {
బహిరంగ:
    explicit V3_బహిర్గతం_VFile(స్థిరరాశి string& filename)
        : V3_బహిర్గతం_CFile{filename, V3_బహిర్గతం_Formatter::LA_VERILOG} {}
    ~V3_బహిర్గతం_VFile() override = అప్రమేయ;
    శూన్య puts_శీర్షిక() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3_బహిర్గతం_Xml_File final : బహిరంగ V3_బహిర్గతం_File {
బహిరంగ:
    explicit V3_బహిర్గతం_Xml_File(స్థిరరాశి string& filename)
        : V3_బహిర్గతం_File{filename, V3_బహిర్గతం_Formatter::LA_XML} {
        ఖండం_Indent(2);
    }
    ~V3_బహిర్గతం_Xml_File() override = అప్రమేయ;
    మాయా శూన్య puts_శీర్షిక() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_బహిర్గతం_ప్రవాహం VL_NOT_FINAL : బహిరంగ V3_బహిర్గతం_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_బహిర్గతం_ప్రవాహం);
    VL_UNMOVABLE(V3_బహిర్గతం_ప్రవాహం);

బహిరంగ:
    V3_బహిర్గతం_ప్రవాహం(std::ostream& ostream, V3_బహిర్గతం_Formatter::Language lang);
    ~V3_బహిర్గతం_ప్రవాహం() override = అప్రమేయ;

    శూన్య putcOutput(అక్షరం chr) override { m_ostream << chr; };
    శూన్య putsOutput(స్థిరరాశి అక్షరం* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
బహిరంగ:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    స్థిర string ifNoProtect(స్థిరరాశి string& లోపల) VL_MT_సురక్షిత;
    // Rename to a new encoded string (unless earlier passthru'ed)
    స్థిర string protect(స్థిరరాశి string& old) VL_MT_సురక్షిత { తిరిగి protectIf(old, true); }
    స్థిర string protectIf(స్థిరరాశి string& old, bool doIt = true) VL_MT_సురక్షిత;
    // Rename words to a new encoded string
    స్థిర string protect_పదాలు_If(స్థిరరాశి string& old, bool doIt = true) VL_MT_సురక్షిత;
    // Write map of renames to output file
    స్థిర శూన్య writeMapFile(స్థిరరాశి string& filename);
};

#అయితే_ముగింపు  // Guard
