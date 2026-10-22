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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3Stats.h"

#অন্তর্ভাব <array>
#অন্তর্ভাব <fstream>
#অন্তর্ভাব <list>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <set>
#অন্তর্ভাব <stack>
#অন্তর্ভাব <vector>

class Ast_গ্রন্থি;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
সার্বজনিক:
    স্থির std::ifstream* new_ifstream(ধ্রুব string& filename) {
        addSrcDepend(filename);
        প্রত্যাগম new_ifstream_nodepend(filename);
    }
    স্থির std::ifstream* new_ifstream_nodepend(ধ্রুব string& filename) VL_MT_SAFE {
        প্রত্যাগম new std::ifstream{filename.c_str()};
    }
    স্থির std::ofstream* new_ofstream(ধ্রুব string& filename, bool append = false) {
        addTgtDepend(filename);
        প্রত্যাগম new_ofstream_nodepend(filename, append);
    }
    স্থির std::ofstream* new_ofstream_nodepend(ধ্রুব string& filename, bool append = false) {
        createMakeDirFor(filename);
        যদি (append) প্রত্যাগম new std::ofstream{filename.c_str(), std::ios::app};
        প্রত্যাগম new std::ofstream{filename.c_str()};
    }
    স্থির FILE* new_fopen_w(ধ্রুব string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        প্রত্যাগম fopen(filename.c_str(), "w");
    }

    // Dependencies
    স্থির শূন্য addSrcDepend(ধ্রুব string& filename) VL_MT_SAFE;
    স্থির শূন্য addTgtDepend(ধ্রুব string& filename) VL_MT_SAFE;
    স্থির শূন্য writeDepend(ধ্রুব string& filename);
    স্থির std::vector<string> getAllDeps();
    স্থির শূন্য writeTimes(ধ্রুব string& filename, ধ্রুব string& cmdlineIn);
    স্থির bool checkTimes(ধ্রুব string& filename, ধ্রুব string& cmdlineIn);

    // Directory utilities
    স্থির শূন্য createMakeDirFor(ধ্রুব string& filename);
    স্থির শূন্য createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
সার্বজনিক:
    // TYPES
    using StrList = std::list<std::string>;

গুপ্ত:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

সার্বজনিক:
    explicit VInFilter(ধ্রুব string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ধ্রুব string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
সার্বজনিক:
    গণনা AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    গণনা Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

গুপ্ত:
    // MEMBERS
    ধ্রুব Language m_lang;  // Indenting Verilog code
    পূর্ণাঙ্ক m_blockIndent;  // Characters per block indent
    পূর্ণাঙ্ক m_commaWidth;  // Width after which to break at ,'s
    পূর্ণাঙ্ক m_lineno = 1;
    পূর্ণাঙ্ক m_column = 0;
    পূর্ণাঙ্ক m_nobreak = false;  // Basic operator or begin paren, don't break next
    পূর্ণাঙ্ক m_sourceLastLineno = 0;
    পূর্ণাঙ্ক m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    পূর্ণাঙ্ক m_indentLevel = 0;  // Current {} indentation
    std::স্তূপ<পূর্ণাঙ্ক> m_parenVec;  // Stack of columns where last ( was
    পূর্ণাঙ্ক m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    পূর্ণাঙ্ক endLevels(ধ্রুব বর্ণ* strg);
    শূন্য putcNoTracking(বর্ণ chr);

সার্বজনিক:
    explicit V3OutFormatter(Language lang);
    আভাস ~V3OutFormatter() = স্বভাব;
    // ACCESSORS
    শূন্য blockIndent(পূর্ণাঙ্ক flag) { m_blockIndent = flag; }
    // METHODS
    শূন্য printf(ধ্রুব বর্ণ* fmt...) VL_ATTR_PRINTF(2);
    শূন্য puts(ধ্রুব বর্ণ* strg) { putns(nullptr, strg); }
    শূন্য puts(ধ্রুব string& strg) { putns(nullptr, strg); }
    শূন্য putns(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব বর্ণ* strg);
    শূন্য putns(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব string& strg) { putns(nodep, strg.c_str()); }
    শূন্য putsNoTracking(ধ্রুব string& strg);
    শূন্য putsQuoted(ধ্রুব string& strg);
    শূন্য putBreak();  // Print linebreak if line is too wide
    শূন্য put_Break_রাশিমালা();  // Print linebreak in expression if line is too wide
    শূন্য putbs(ধ্রুব বর্ণ* strg) {
        put_Break_রাশিমালা();
        putns(nullptr, strg);
    }
    শূন্য putbs(ধ্রুব string& strg) {
        put_Break_রাশিমালা();
        putns(nullptr, strg);
    }
    শূন্য putnbs(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব string& strg) {
        put_Break_রাশিমালা();
        putns(nodep, strg);
    }
    bool exceededWidth() ধ্রুব { প্রত্যাগম m_column > m_commaWidth; }
    শূন্য indentInc() { m_indentLevel += m_blockIndent; }
    শূন্য indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    শূন্য blockInc() { m_parenVec.ঠেলো(m_indentLevel + m_blockIndent); }
    শূন্য blockDec() {
        যদি (!m_parenVec.empty()) m_parenVec.বের_করো();
    }
    শূন্য ensure_New_সারি() {
        যদি (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    স্থির ধ্রুব std::string& indentSpaces(পূর্ণাঙ্ক অঙ্ক);
    // Add escaped characters to strings
    স্থির string quoteNameControls(ধ্রুব string& namein, Language lang = LA_C) VL_PURE;
    স্থির bool tokenMatch(ধ্রুব বর্ণ* cp, ধ্রুব বর্ণ* cmp);
    স্থির bool tokenNotStart(ধ্রুব বর্ণ* cp);  // Import/export meaning no endfunction
    স্থির bool tokenStart(ধ্রুব বর্ণ* cp);
    স্থির bool tokenEnd(ধ্রুব বর্ণ* cp);

    // CALLBACKS - MUST OVERRIDE
    আভাস শূন্য putcOutput(বর্ণ chr) = 0;
    আভাস শূন্য putsOutput(ধ্রুব বর্ণ* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : সার্বজনিক V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    স্থির constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    ধ্রুব std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<বর্ণ, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

সার্বজনিক:
    V3OutFile(ধ্রুব string& filename, V3OutFormatter::Language lang);
    V3OutFile(ধ্রুব V3OutFile&) = delete;
    V3OutFile& operator=(ধ্রুব V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() ধ্রুব { প্রত্যাগম m_filename; }

    শূন্য putsForceIncs();

    শূন্য statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

গুপ্ত:
    শূন্য writeBlock() {
        যদি (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    শূন্য putcOutput(বর্ণ chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        যদি (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    শূন্য putsOutput(ধ্রুব বর্ণ* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        যাবত্ (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        যদি (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : সার্বজনিক V3OutFile {
    পূর্ণাঙ্ক m_guard = false;  // Created header guard
    পূর্ণাঙ্ক m_private;  // 1 = Most recently emitted private:, 2 = public:
সার্বজনিক:
    explicit V3OutCFile(ধ্রুব string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    আভাস শূন্য putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    আভাস শূন্য puts_Int_শীর্ষ_Include() { putsForceIncs(); }
    আভাস শূন্য putsGuard();
    আভাস শূন্য putsEndGuard() {
        যদি (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    শূন্য resetPrivate() { m_private = 0; }
    শূন্য putsPrivate(bool setPrivate) {
        যদি (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } অন্যথা যদি (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : সার্বজনিক V3OutFile {
    // CONSTANTS
    স্থির constexpr ধ্রুব বর্ণ* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
গুপ্ত:
    std::স্তূপ<বর্ণ> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::স্তূপ<bool> m_empty;  // Current scope is empty, no comma later

সার্বজনিক:
    explicit V3OutJsonFile(ধ্রুব string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    আভাস শূন্য putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    শূন্য puts(ধ্রুব বর্ণ* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    শূন্য puts(ধ্রুব string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(ধ্রুব std::string& name, বর্ণ type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.ঠেলো(type == '{' ? '}' : ']');
        যদি (!m_empty.empty()) m_empty.শীর্ষ() = false;
        m_empty.ঠেলো(true);
        প্রত্যাগম *this;
    }
    V3OutJsonFile& begin(বর্ণ type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.ঠেলো(type == '{' ? '}' : ']');
        যদি (!m_empty.empty()) m_empty.শীর্ষ() = false;
        m_empty.ঠেলো(true);
        প্রত্যাগম *this;
    }

    // Put a named value
    V3OutJsonFile& put(ধ্রুব std::string& name, ধ্রুব বর্ণ* valuep) {
        প্রত্যাগম putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(ধ্রুব std::string& name, ধ্রুব std::string& মান) {
        প্রত্যাগম putNamed(name, মান, true);
    }
    V3OutJsonFile& put(ধ্রুব std::string& name, bool মান) {
        প্রত্যাগম putNamed(name, মান ? "true" : "false", false);
    }
    V3OutJsonFile& put(ধ্রুব std::string& name, পূর্ণাঙ্ক মান) {
        প্রত্যাগম putNamed(name, std::to_string(মান), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(ধ্রুব std::string& মান) { প্রত্যাগম putNamed("", মান, true); }
    V3OutJsonFile& put(bool মান) { প্রত্যাগম putNamed("", মান ? "true" : "false", false); }
    V3OutJsonFile& put(পূর্ণাঙ্ক মান) { প্রত্যাগম putNamed("", std::to_string(মান), false); }

    V3OutJsonFile& putList(ধ্রুব std::string& name, ধ্রুব std::vector<std::string>& list) {
        যদি (list.empty()) প্রত্যাগম *this;
        begin(name, '[');
        প্রতি (ধ্রুব std::string& str : list) put(str);
        প্রত্যাগম end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.শীর্ষ());
        m_scope.বের_করো();
        m_empty.বের_করো();
        প্রত্যাগম *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        প্রত্যাগম *this;
    }

গুপ্ত:
    শূন্য comma() {
        যদি (m_empty.empty()) প্রত্যাগম;
        যদি (!m_empty.শীর্ষ()) puts(",\n");
        m_empty.শীর্ষ() = true;
    }
    V3OutJsonFile& putNamed(ধ্রুব std::string& name, ধ্রুব std::string& মান, bool quoted) {
        comma();
        ধ্রুব string মান_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(মান) + "\"" : মান;
        যদি (name.empty()) {
            puts(m_prefix + মান_Q);
        } অন্যথা {
            puts(m_prefix + "\"" + name + "\": " + মান_Q);
        }
        m_empty.শীর্ষ() = false;
        প্রত্যাগম *this;
    }
};

class V3OutMkFile final : সার্বজনিক V3OutFile {
সার্বজনিক:
    explicit V3OutMkFile(ধ্রুব string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = স্বভাব;
    আভাস শূন্য putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    শূন্য puts(ধ্রুব বর্ণ* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    শূন্য puts(ধ্রুব string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    শূন্য putSet(ধ্রুব string& var, ধ্রুব string& মান) {
        puts(VString::dot(var + " =", " ", মান) + "\n");
    }
    // Put VARIABLE ?= VALUE
    শূন্য putSetQ(ধ্রুব string& var, ধ্রুব string& মান) {
        puts(VString::dot(var + " ?=", " ", মান) + "\n");
    }
};

class V3OutVFile final : সার্বজনিক V3OutCFile {
সার্বজনিক:
    explicit V3OutVFile(ধ্রুব string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = স্বভাব;
    শূন্য putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : সার্বজনিক V3OutFile {
সার্বজনিক:
    explicit V3OutXmlFile(ধ্রুব string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = স্বভাব;
    আভাস শূন্য putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : সার্বজনিক V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

সার্বজনিক:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = স্বভাব;

    শূন্য putcOutput(বর্ণ chr) override { m_ostream << chr; };
    শূন্য putsOutput(ধ্রুব বর্ণ* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
সার্বজনিক:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    স্থির string ifNoProtect(ধ্রুব string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    স্থির string protect(ধ্রুব string& old) VL_MT_SAFE { প্রত্যাগম protectIf(old, true); }
    স্থির string protectIf(ধ্রুব string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    স্থির string protect_শব্দগুলি_If(ধ্রুব string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    স্থির শূন্য writeMapFile(ধ্রুব string& filename);
};

#endif  // Guard
