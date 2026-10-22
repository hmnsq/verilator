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

#ifndef VERILATOR_V3FILE_H_
#define VERILATOR_V3FILE_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3Stats.h"

#અન્તર્ભાવ <array>
#અન્તર્ભાવ <fstream>
#અન્તર્ભાવ <list>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <stack>
#અન્તર્ભાવ <vector>

class Ast_ગાંઠ;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
જાહેર:
    સ્થિર std::ifstream* new_ifstream(અચલ string& filename) {
        addSrcDepend(filename);
        પાછા new_ifstream_nodepend(filename);
    }
    સ્થિર std::ifstream* new_ifstream_nodepend(અચલ string& filename) VL_MT_SAFE {
        પાછા new std::ifstream{filename.c_str()};
    }
    સ્થિર std::ofstream* new_ofstream(અચલ string& filename, bool append = false) {
        addTgtDepend(filename);
        પાછા new_ofstream_nodepend(filename, append);
    }
    સ્થિર std::ofstream* new_ofstream_nodepend(અચલ string& filename, bool append = false) {
        createMakeDirFor(filename);
        જો (append) પાછા new std::ofstream{filename.c_str(), std::ios::app};
        પાછા new std::ofstream{filename.c_str()};
    }
    સ્થિર FILE* new_fopen_w(અચલ string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        પાછા fopen(filename.c_str(), "w");
    }

    // Dependencies
    સ્થિર શૂન્ય addSrcDepend(અચલ string& filename) VL_MT_SAFE;
    સ્થિર શૂન્ય addTgtDepend(અચલ string& filename) VL_MT_SAFE;
    સ્થિર શૂન્ય writeDepend(અચલ string& filename);
    સ્થિર std::vector<string> getAllDeps();
    સ્થિર શૂન્ય writeTimes(અચલ string& filename, અચલ string& cmdlineIn);
    સ્થિર bool checkTimes(અચલ string& filename, અચલ string& cmdlineIn);

    // Directory utilities
    સ્થિર શૂન્ય createMakeDirFor(અચલ string& filename);
    સ્થિર શૂન્ય createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
જાહેર:
    // TYPES
    using StrList = std::list<std::string>;

ખાનગી:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

જાહેર:
    explicit VInFilter(અચલ string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(અચલ string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
જાહેર:
    ગણના AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    ગણના Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

ખાનગી:
    // MEMBERS
    અચલ Language m_lang;  // Indenting Verilog code
    પૂર્ણાંક m_blockIndent;  // Characters per block indent
    પૂર્ણાંક m_commaWidth;  // Width after which to break at ,'s
    પૂર્ણાંક m_lineno = 1;
    પૂર્ણાંક m_column = 0;
    પૂર્ણાંક m_nobreak = false;  // Basic operator or begin paren, don't break next
    પૂર્ણાંક m_sourceLastLineno = 0;
    પૂર્ણાંક m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    પૂર્ણાંક m_indentLevel = 0;  // Current {} indentation
    std::ઢગલો<પૂર્ણાંક> m_parenVec;  // Stack of columns where last ( was
    પૂર્ણાંક m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    પૂર્ણાંક endLevels(અચલ અક્ષર* strg);
    શૂન્ય putcNoTracking(અક્ષર chr);

જાહેર:
    explicit V3OutFormatter(Language lang);
    આભાસી ~V3OutFormatter() = મૂળભૂત;
    // ACCESSORS
    શૂન્ય blockIndent(પૂર્ણાંક flag) { m_blockIndent = flag; }
    // METHODS
    શૂન્ય printf(અચલ અક્ષર* fmt...) VL_ATTR_PRINTF(2);
    શૂન્ય puts(અચલ અક્ષર* strg) { putns(nullptr, strg); }
    શૂન્ય puts(અચલ string& strg) { putns(nullptr, strg); }
    શૂન્ય putns(અચલ Ast_ગાંઠ* nodep, અચલ અક્ષર* strg);
    શૂન્ય putns(અચલ Ast_ગાંઠ* nodep, અચલ string& strg) { putns(nodep, strg.c_str()); }
    શૂન્ય putsNoTracking(અચલ string& strg);
    શૂન્ય putsQuoted(અચલ string& strg);
    શૂન્ય putBreak();  // Print linebreak if line is too wide
    શૂન્ય put_Break_પદાવલિ();  // Print linebreak in expression if line is too wide
    શૂન્ય putbs(અચલ અક્ષર* strg) {
        put_Break_પદાવલિ();
        putns(nullptr, strg);
    }
    શૂન્ય putbs(અચલ string& strg) {
        put_Break_પદાવલિ();
        putns(nullptr, strg);
    }
    શૂન્ય putnbs(અચલ Ast_ગાંઠ* nodep, અચલ string& strg) {
        put_Break_પદાવલિ();
        putns(nodep, strg);
    }
    bool exceededWidth() અચલ { પાછા m_column > m_commaWidth; }
    શૂન્ય indentInc() { m_indentLevel += m_blockIndent; }
    શૂન્ય indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    શૂન્ય blockInc() { m_parenVec.ધકેલો(m_indentLevel + m_blockIndent); }
    શૂન્ય blockDec() {
        જો (!m_parenVec.empty()) m_parenVec.કાઢો();
    }
    શૂન્ય ensure_New_લીટી() {
        જો (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    સ્થિર અચલ std::string& indentSpaces(પૂર્ણાંક અંક);
    // Add escaped characters to strings
    સ્થિર string quoteNameControls(અચલ string& namein, Language lang = LA_C) VL_PURE;
    સ્થિર bool tokenMatch(અચલ અક્ષર* cp, અચલ અક્ષર* cmp);
    સ્થિર bool tokenNotStart(અચલ અક્ષર* cp);  // Import/export meaning no endfunction
    સ્થિર bool tokenStart(અચલ અક્ષર* cp);
    સ્થિર bool tokenEnd(અચલ અક્ષર* cp);

    // CALLBACKS - MUST OVERRIDE
    આભાસી શૂન્ય putcOutput(અક્ષર chr) = 0;
    આભાસી શૂન્ય putsOutput(અચલ અક્ષર* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : જાહેર V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    સ્થિર constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    અચલ std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<અક્ષર, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

જાહેર:
    V3OutFile(અચલ string& filename, V3OutFormatter::Language lang);
    V3OutFile(અચલ V3OutFile&) = delete;
    V3OutFile& operator=(અચલ V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() અચલ { પાછા m_filename; }

    શૂન્ય putsForceIncs();

    શૂન્ય statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

ખાનગી:
    શૂન્ય writeBlock() {
        જો (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    શૂન્ય putcOutput(અક્ષર chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        જો (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    શૂન્ય putsOutput(અચલ અક્ષર* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        જ્યારે (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        જો (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : જાહેર V3OutFile {
    પૂર્ણાંક m_guard = false;  // Created header guard
    પૂર્ણાંક m_private;  // 1 = Most recently emitted private:, 2 = public:
જાહેર:
    explicit V3OutCFile(અચલ string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    આભાસી શૂન્ય putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    આભાસી શૂન્ય puts_Int_ટોચ_Include() { putsForceIncs(); }
    આભાસી શૂન્ય putsGuard();
    આભાસી શૂન્ય putsEndGuard() {
        જો (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    શૂન્ય resetPrivate() { m_private = 0; }
    શૂન્ય putsPrivate(bool setPrivate) {
        જો (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } નહીંતર જો (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : જાહેર V3OutFile {
    // CONSTANTS
    સ્થિર constexpr અચલ અક્ષર* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
ખાનગી:
    std::ઢગલો<અક્ષર> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::ઢગલો<bool> m_empty;  // Current scope is empty, no comma later

જાહેર:
    explicit V3OutJsonFile(અચલ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    આભાસી શૂન્ય putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    શૂન્ય puts(અચલ અક્ષર* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    શૂન્ય puts(અચલ string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(અચલ std::string& name, અક્ષર type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.ધકેલો(type == '{' ? '}' : ']');
        જો (!m_empty.empty()) m_empty.ટોચ() = false;
        m_empty.ધકેલો(true);
        પાછા *this;
    }
    V3OutJsonFile& begin(અક્ષર type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.ધકેલો(type == '{' ? '}' : ']');
        જો (!m_empty.empty()) m_empty.ટોચ() = false;
        m_empty.ધકેલો(true);
        પાછા *this;
    }

    // Put a named value
    V3OutJsonFile& put(અચલ std::string& name, અચલ અક્ષર* valuep) {
        પાછા putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(અચલ std::string& name, અચલ std::string& કિંમત) {
        પાછા putNamed(name, કિંમત, true);
    }
    V3OutJsonFile& put(અચલ std::string& name, bool કિંમત) {
        પાછા putNamed(name, કિંમત ? "true" : "false", false);
    }
    V3OutJsonFile& put(અચલ std::string& name, પૂર્ણાંક કિંમત) {
        પાછા putNamed(name, std::to_string(કિંમત), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(અચલ std::string& કિંમત) { પાછા putNamed("", કિંમત, true); }
    V3OutJsonFile& put(bool કિંમત) { પાછા putNamed("", કિંમત ? "true" : "false", false); }
    V3OutJsonFile& put(પૂર્ણાંક કિંમત) { પાછા putNamed("", std::to_string(કિંમત), false); }

    V3OutJsonFile& putList(અચલ std::string& name, અચલ std::vector<std::string>& list) {
        જો (list.empty()) પાછા *this;
        begin(name, '[');
        માટે (અચલ std::string& str : list) put(str);
        પાછા end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.ટોચ());
        m_scope.કાઢો();
        m_empty.કાઢો();
        પાછા *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        પાછા *this;
    }

ખાનગી:
    શૂન્ય comma() {
        જો (m_empty.empty()) પાછા;
        જો (!m_empty.ટોચ()) puts(",\n");
        m_empty.ટોચ() = true;
    }
    V3OutJsonFile& putNamed(અચલ std::string& name, અચલ std::string& કિંમત, bool quoted) {
        comma();
        અચલ string કિંમત_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(કિંમત) + "\"" : કિંમત;
        જો (name.empty()) {
            puts(m_prefix + કિંમત_Q);
        } નહીંતર {
            puts(m_prefix + "\"" + name + "\": " + કિંમત_Q);
        }
        m_empty.ટોચ() = false;
        પાછા *this;
    }
};

class V3OutMkFile final : જાહેર V3OutFile {
જાહેર:
    explicit V3OutMkFile(અચલ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = મૂળભૂત;
    આભાસી શૂન્ય putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    શૂન્ય puts(અચલ અક્ષર* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    શૂન્ય puts(અચલ string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    શૂન્ય putSet(અચલ string& var, અચલ string& કિંમત) {
        puts(VString::dot(var + " =", " ", કિંમત) + "\n");
    }
    // Put VARIABLE ?= VALUE
    શૂન્ય putSetQ(અચલ string& var, અચલ string& કિંમત) {
        puts(VString::dot(var + " ?=", " ", કિંમત) + "\n");
    }
};

class V3OutVFile final : જાહેર V3OutCFile {
જાહેર:
    explicit V3OutVFile(અચલ string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = મૂળભૂત;
    શૂન્ય putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : જાહેર V3OutFile {
જાહેર:
    explicit V3OutXmlFile(અચલ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = મૂળભૂત;
    આભાસી શૂન્ય putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : જાહેર V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

જાહેર:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = મૂળભૂત;

    શૂન્ય putcOutput(અક્ષર chr) override { m_ostream << chr; };
    શૂન્ય putsOutput(અચલ અક્ષર* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
જાહેર:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    સ્થિર string ifNoProtect(અચલ string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    સ્થિર string protect(અચલ string& old) VL_MT_SAFE { પાછા protectIf(old, true); }
    સ્થિર string protectIf(અચલ string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    સ્થિર string protect_શબ્દો_If(અચલ string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    સ્થિર શૂન્ય writeMapFile(અચલ string& filename);
};

#endif  // Guard
