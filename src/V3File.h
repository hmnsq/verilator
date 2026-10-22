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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <array>
#അന്തര്ഭാവ <fstream>
#അന്തര്ഭാവ <list>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <stack>
#അന്തര്ഭാവ <vector>

class Ast_കണ്ണി;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
പൊതു:
    സ്ഥിതം std::ifstream* new_ifstream(സ്ഥിരം string& filename) {
        addSrcDepend(filename);
        തിരികെ new_ifstream_nodepend(filename);
    }
    സ്ഥിതം std::ifstream* new_ifstream_nodepend(സ്ഥിരം string& filename) VL_MT_SAFE {
        തിരികെ new std::ifstream{filename.c_str()};
    }
    സ്ഥിതം std::ofstream* new_ofstream(സ്ഥിരം string& filename, bool append = false) {
        addTgtDepend(filename);
        തിരികെ new_ofstream_nodepend(filename, append);
    }
    സ്ഥിതം std::ofstream* new_ofstream_nodepend(സ്ഥിരം string& filename, bool append = false) {
        createMakeDirFor(filename);
        എങ്കിൽ (append) തിരികെ new std::ofstream{filename.c_str(), std::ios::app};
        തിരികെ new std::ofstream{filename.c_str()};
    }
    സ്ഥിതം FILE* new_fopen_w(സ്ഥിരം string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        തിരികെ fopen(filename.c_str(), "w");
    }

    // Dependencies
    സ്ഥിതം ശൂന്യം addSrcDepend(സ്ഥിരം string& filename) VL_MT_SAFE;
    സ്ഥിതം ശൂന്യം addTgtDepend(സ്ഥിരം string& filename) VL_MT_SAFE;
    സ്ഥിതം ശൂന്യം writeDepend(സ്ഥിരം string& filename);
    സ്ഥിതം std::vector<string> getAllDeps();
    സ്ഥിതം ശൂന്യം writeTimes(സ്ഥിരം string& filename, സ്ഥിരം string& cmdlineIn);
    സ്ഥിതം bool checkTimes(സ്ഥിരം string& filename, സ്ഥിരം string& cmdlineIn);

    // Directory utilities
    സ്ഥിതം ശൂന്യം createMakeDirFor(സ്ഥിരം string& filename);
    സ്ഥിതം ശൂന്യം createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
പൊതു:
    // TYPES
    using StrList = std::list<std::string>;

സ്വകാര്യ:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

പൊതു:
    explicit VInFilter(സ്ഥിരം string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(സ്ഥിരം string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
പൊതു:
    എണ്ണൽ AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    എണ്ണൽ Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

സ്വകാര്യ:
    // MEMBERS
    സ്ഥിരം Language m_lang;  // Indenting Verilog code
    പൂർണ്ണസംഖ്യ m_blockIndent;  // Characters per block indent
    പൂർണ്ണസംഖ്യ m_commaWidth;  // Width after which to break at ,'s
    പൂർണ്ണസംഖ്യ m_lineno = 1;
    പൂർണ്ണസംഖ്യ m_column = 0;
    പൂർണ്ണസംഖ്യ m_nobreak = false;  // Basic operator or begin paren, don't break next
    പൂർണ്ണസംഖ്യ m_sourceLastLineno = 0;
    പൂർണ്ണസംഖ്യ m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    പൂർണ്ണസംഖ്യ m_indentLevel = 0;  // Current {} indentation
    std::അട്ടി<പൂർണ്ണസംഖ്യ> m_parenVec;  // Stack of columns where last ( was
    പൂർണ്ണസംഖ്യ m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    പൂർണ്ണസംഖ്യ endLevels(സ്ഥിരം അക്ഷരം* strg);
    ശൂന്യം putcNoTracking(അക്ഷരം chr);

പൊതു:
    explicit V3OutFormatter(Language lang);
    മായാ ~V3OutFormatter() = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    ശൂന്യം blockIndent(പൂർണ്ണസംഖ്യ flag) { m_blockIndent = flag; }
    // METHODS
    ശൂന്യം printf(സ്ഥിരം അക്ഷരം* fmt...) VL_ATTR_PRINTF(2);
    ശൂന്യം puts(സ്ഥിരം അക്ഷരം* strg) { putns(nullptr, strg); }
    ശൂന്യം puts(സ്ഥിരം string& strg) { putns(nullptr, strg); }
    ശൂന്യം putns(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം അക്ഷരം* strg);
    ശൂന്യം putns(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& strg) { putns(nodep, strg.c_str()); }
    ശൂന്യം putsNoTracking(സ്ഥിരം string& strg);
    ശൂന്യം putsQuoted(സ്ഥിരം string& strg);
    ശൂന്യം putBreak();  // Print linebreak if line is too wide
    ശൂന്യം put_Break_വ്യഞ്ജകം();  // Print linebreak in expression if line is too wide
    ശൂന്യം putbs(സ്ഥിരം അക്ഷരം* strg) {
        put_Break_വ്യഞ്ജകം();
        putns(nullptr, strg);
    }
    ശൂന്യം putbs(സ്ഥിരം string& strg) {
        put_Break_വ്യഞ്ജകം();
        putns(nullptr, strg);
    }
    ശൂന്യം putnbs(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& strg) {
        put_Break_വ്യഞ്ജകം();
        putns(nodep, strg);
    }
    bool exceededWidth() സ്ഥിരം { തിരികെ m_column > m_commaWidth; }
    ശൂന്യം indentInc() { m_indentLevel += m_blockIndent; }
    ശൂന്യം indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    ശൂന്യം blockInc() { m_parenVec.തള്ളുക(m_indentLevel + m_blockIndent); }
    ശൂന്യം blockDec() {
        എങ്കിൽ (!m_parenVec.empty()) m_parenVec.എടുക്കുക();
    }
    ശൂന്യം ensure_New_വരി() {
        എങ്കിൽ (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    സ്ഥിതം സ്ഥിരം std::string& indentSpaces(പൂർണ്ണസംഖ്യ അക്കം);
    // Add escaped characters to strings
    സ്ഥിതം string quoteNameControls(സ്ഥിരം string& namein, Language lang = LA_C) VL_PURE;
    സ്ഥിതം bool tokenMatch(സ്ഥിരം അക്ഷരം* cp, സ്ഥിരം അക്ഷരം* cmp);
    സ്ഥിതം bool tokenNotStart(സ്ഥിരം അക്ഷരം* cp);  // Import/export meaning no endfunction
    സ്ഥിതം bool tokenStart(സ്ഥിരം അക്ഷരം* cp);
    സ്ഥിതം bool tokenEnd(സ്ഥിരം അക്ഷരം* cp);

    // CALLBACKS - MUST OVERRIDE
    മായാ ശൂന്യം putcOutput(അക്ഷരം chr) = 0;
    മായാ ശൂന്യം putsOutput(സ്ഥിരം അക്ഷരം* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : പൊതു V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    സ്ഥിതം constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    സ്ഥിരം std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<അക്ഷരം, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

പൊതു:
    V3OutFile(സ്ഥിരം string& filename, V3OutFormatter::Language lang);
    V3OutFile(സ്ഥിരം V3OutFile&) = delete;
    V3OutFile& operator=(സ്ഥിരം V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() സ്ഥിരം { തിരികെ m_filename; }

    ശൂന്യം putsForceIncs();

    ശൂന്യം statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

സ്വകാര്യ:
    ശൂന്യം writeBlock() {
        എങ്കിൽ (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    ശൂന്യം putcOutput(അക്ഷരം chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        എങ്കിൽ (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    ശൂന്യം putsOutput(സ്ഥിരം അക്ഷരം* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        ആകുമ്പോൾ (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        എങ്കിൽ (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : പൊതു V3OutFile {
    പൂർണ്ണസംഖ്യ m_guard = false;  // Created header guard
    പൂർണ്ണസംഖ്യ m_private;  // 1 = Most recently emitted private:, 2 = public:
പൊതു:
    explicit V3OutCFile(സ്ഥിരം string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    മായാ ശൂന്യം putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    മായാ ശൂന്യം puts_Int_മുകൾ_Include() { putsForceIncs(); }
    മായാ ശൂന്യം putsGuard();
    മായാ ശൂന്യം putsEndGuard() {
        എങ്കിൽ (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    ശൂന്യം resetPrivate() { m_private = 0; }
    ശൂന്യം putsPrivate(bool setPrivate) {
        എങ്കിൽ (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : പൊതു V3OutFile {
    // CONSTANTS
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
സ്വകാര്യ:
    std::അട്ടി<അക്ഷരം> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::അട്ടി<bool> m_empty;  // Current scope is empty, no comma later

പൊതു:
    explicit V3OutJsonFile(സ്ഥിരം string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    മായാ ശൂന്യം putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    ശൂന്യം puts(സ്ഥിരം അക്ഷരം* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ശൂന്യം puts(സ്ഥിരം string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(സ്ഥിരം std::string& name, അക്ഷരം type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.തള്ളുക(type == '{' ? '}' : ']');
        എങ്കിൽ (!m_empty.empty()) m_empty.മുകൾ() = false;
        m_empty.തള്ളുക(true);
        തിരികെ *this;
    }
    V3OutJsonFile& begin(അക്ഷരം type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.തള്ളുക(type == '{' ? '}' : ']');
        എങ്കിൽ (!m_empty.empty()) m_empty.മുകൾ() = false;
        m_empty.തള്ളുക(true);
        തിരികെ *this;
    }

    // Put a named value
    V3OutJsonFile& put(സ്ഥിരം std::string& name, സ്ഥിരം അക്ഷരം* valuep) {
        തിരികെ putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(സ്ഥിരം std::string& name, സ്ഥിരം std::string& മൂല്യം) {
        തിരികെ putNamed(name, മൂല്യം, true);
    }
    V3OutJsonFile& put(സ്ഥിരം std::string& name, bool മൂല്യം) {
        തിരികെ putNamed(name, മൂല്യം ? "true" : "false", false);
    }
    V3OutJsonFile& put(സ്ഥിരം std::string& name, പൂർണ്ണസംഖ്യ മൂല്യം) {
        തിരികെ putNamed(name, std::to_string(മൂല്യം), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(സ്ഥിരം std::string& മൂല്യം) { തിരികെ putNamed("", മൂല്യം, true); }
    V3OutJsonFile& put(bool മൂല്യം) { തിരികെ putNamed("", മൂല്യം ? "true" : "false", false); }
    V3OutJsonFile& put(പൂർണ്ണസംഖ്യ മൂല്യം) { തിരികെ putNamed("", std::to_string(മൂല്യം), false); }

    V3OutJsonFile& putList(സ്ഥിരം std::string& name, സ്ഥിരം std::vector<std::string>& list) {
        എങ്കിൽ (list.empty()) തിരികെ *this;
        begin(name, '[');
        വേണ്ടി (സ്ഥിരം std::string& str : list) put(str);
        തിരികെ end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.മുകൾ());
        m_scope.എടുക്കുക();
        m_empty.എടുക്കുക();
        തിരികെ *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        തിരികെ *this;
    }

സ്വകാര്യ:
    ശൂന്യം comma() {
        എങ്കിൽ (m_empty.empty()) തിരികെ;
        എങ്കിൽ (!m_empty.മുകൾ()) puts(",\n");
        m_empty.മുകൾ() = true;
    }
    V3OutJsonFile& putNamed(സ്ഥിരം std::string& name, സ്ഥിരം std::string& മൂല്യം, bool quoted) {
        comma();
        സ്ഥിരം string മൂല്യം_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(മൂല്യം) + "\"" : മൂല്യം;
        എങ്കിൽ (name.empty()) {
            puts(m_prefix + മൂല്യം_Q);
        } അല്ലെങ്കിൽ {
            puts(m_prefix + "\"" + name + "\": " + മൂല്യം_Q);
        }
        m_empty.മുകൾ() = false;
        തിരികെ *this;
    }
};

class V3OutMkFile final : പൊതു V3OutFile {
പൊതു:
    explicit V3OutMkFile(സ്ഥിരം string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = സ്ഥിരസ്ഥിതി;
    മായാ ശൂന്യം putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    ശൂന്യം puts(സ്ഥിരം അക്ഷരം* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ശൂന്യം puts(സ്ഥിരം string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    ശൂന്യം putSet(സ്ഥിരം string& var, സ്ഥിരം string& മൂല്യം) {
        puts(VString::dot(var + " =", " ", മൂല്യം) + "\n");
    }
    // Put VARIABLE ?= VALUE
    ശൂന്യം putSetQ(സ്ഥിരം string& var, സ്ഥിരം string& മൂല്യം) {
        puts(VString::dot(var + " ?=", " ", മൂല്യം) + "\n");
    }
};

class V3OutVFile final : പൊതു V3OutCFile {
പൊതു:
    explicit V3OutVFile(സ്ഥിരം string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = സ്ഥിരസ്ഥിതി;
    ശൂന്യം putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : പൊതു V3OutFile {
പൊതു:
    explicit V3OutXmlFile(സ്ഥിരം string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = സ്ഥിരസ്ഥിതി;
    മായാ ശൂന്യം putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : പൊതു V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

പൊതു:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = സ്ഥിരസ്ഥിതി;

    ശൂന്യം putcOutput(അക്ഷരം chr) override { m_ostream << chr; };
    ശൂന്യം putsOutput(സ്ഥിരം അക്ഷരം* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
പൊതു:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    സ്ഥിതം string ifNoProtect(സ്ഥിരം string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    സ്ഥിതം string protect(സ്ഥിരം string& old) VL_MT_SAFE { തിരികെ protectIf(old, true); }
    സ്ഥിതം string protectIf(സ്ഥിരം string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    സ്ഥിതം string protect_വാക്കുകൾ_If(സ്ഥിരം string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    സ്ഥിതം ശൂന്യം writeMapFile(സ്ഥിരം string& filename);
};

#endif  // Guard
