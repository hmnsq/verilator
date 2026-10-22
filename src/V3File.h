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
সর্বজনীন:
    স্থির std::ifstream* new_ifstream(ধ্রুবক string& filename) {
        addSrcDepend(filename);
        ফেরত new_ifstream_nodepend(filename);
    }
    স্থির std::ifstream* new_ifstream_nodepend(ধ্রুবক string& filename) VL_MT_SAFE {
        ফেরত new std::ifstream{filename.c_str()};
    }
    স্থির std::ofstream* new_ofstream(ধ্রুবক string& filename, bool append = false) {
        addTgtDepend(filename);
        ফেরত new_ofstream_nodepend(filename, append);
    }
    স্থির std::ofstream* new_ofstream_nodepend(ধ্রুবক string& filename, bool append = false) {
        createMakeDirFor(filename);
        যদি (append) ফেরত new std::ofstream{filename.c_str(), std::ios::app};
        ফেরত new std::ofstream{filename.c_str()};
    }
    স্থির FILE* new_fopen_w(ধ্রুবক string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        ফেরত fopen(filename.c_str(), "w");
    }

    // Dependencies
    স্থির শূন্য addSrcDepend(ধ্রুবক string& filename) VL_MT_SAFE;
    স্থির শূন্য addTgtDepend(ধ্রুবক string& filename) VL_MT_SAFE;
    স্থির শূন্য writeDepend(ধ্রুবক string& filename);
    স্থির std::vector<string> getAllDeps();
    স্থির শূন্য writeTimes(ধ্রুবক string& filename, ধ্রুবক string& cmdlineIn);
    স্থির bool checkTimes(ধ্রুবক string& filename, ধ্রুবক string& cmdlineIn);

    // Directory utilities
    স্থির শূন্য createMakeDirFor(ধ্রুবক string& filename);
    স্থির শূন্য createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
সর্বজনীন:
    // TYPES
    using StrList = std::list<std::string>;

ব্যক্তিগত:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

সর্বজনীন:
    explicit VInFilter(ধ্রুবক string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ধ্রুবক string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
সর্বজনীন:
    গণনা AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    গণনা Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

ব্যক্তিগত:
    // MEMBERS
    ধ্রুবক Language m_lang;  // Indenting Verilog code
    পূর্ণসংখ্যা m_blockIndent;  // Characters per block indent
    পূর্ণসংখ্যা m_commaWidth;  // Width after which to break at ,'s
    পূর্ণসংখ্যা m_lineno = 1;
    পূর্ণসংখ্যা m_column = 0;
    পূর্ণসংখ্যা m_nobreak = false;  // Basic operator or begin paren, don't break next
    পূর্ণসংখ্যা m_sourceLastLineno = 0;
    পূর্ণসংখ্যা m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    পূর্ণসংখ্যা m_indentLevel = 0;  // Current {} indentation
    std::স্তূপ<পূর্ণসংখ্যা> m_parenVec;  // Stack of columns where last ( was
    পূর্ণসংখ্যা m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    পূর্ণসংখ্যা endLevels(ধ্রুবক অক্ষর* strg);
    শূন্য putcNoTracking(অক্ষর chr);

সর্বজনীন:
    explicit V3OutFormatter(Language lang);
    আভাসী ~V3OutFormatter() = পূর্বনির্ধারিত;
    // ACCESSORS
    শূন্য blockIndent(পূর্ণসংখ্যা flag) { m_blockIndent = flag; }
    // METHODS
    শূন্য printf(ধ্রুবক অক্ষর* fmt...) VL_ATTR_PRINTF(2);
    শূন্য puts(ধ্রুবক অক্ষর* strg) { putns(nullptr, strg); }
    শূন্য puts(ধ্রুবক string& strg) { putns(nullptr, strg); }
    শূন্য putns(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক অক্ষর* strg);
    শূন্য putns(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক string& strg) { putns(nodep, strg.c_str()); }
    শূন্য putsNoTracking(ধ্রুবক string& strg);
    শূন্য putsQuoted(ধ্রুবক string& strg);
    শূন্য putBreak();  // Print linebreak if line is too wide
    শূন্য put_Break_রাশিমালা();  // Print linebreak in expression if line is too wide
    শূন্য putbs(ধ্রুবক অক্ষর* strg) {
        put_Break_রাশিমালা();
        putns(nullptr, strg);
    }
    শূন্য putbs(ধ্রুবক string& strg) {
        put_Break_রাশিমালা();
        putns(nullptr, strg);
    }
    শূন্য putnbs(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক string& strg) {
        put_Break_রাশিমালা();
        putns(nodep, strg);
    }
    bool exceededWidth() ধ্রুবক { ফেরত m_column > m_commaWidth; }
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
    স্থির ধ্রুবক std::string& indentSpaces(পূর্ণসংখ্যা অঙ্ক);
    // Add escaped characters to strings
    স্থির string quoteNameControls(ধ্রুবক string& namein, Language lang = LA_C) VL_PURE;
    স্থির bool tokenMatch(ধ্রুবক অক্ষর* cp, ধ্রুবক অক্ষর* cmp);
    স্থির bool tokenNotStart(ধ্রুবক অক্ষর* cp);  // Import/export meaning no endfunction
    স্থির bool tokenStart(ধ্রুবক অক্ষর* cp);
    স্থির bool tokenEnd(ধ্রুবক অক্ষর* cp);

    // CALLBACKS - MUST OVERRIDE
    আভাসী শূন্য putcOutput(অক্ষর chr) = 0;
    আভাসী শূন্য putsOutput(ধ্রুবক অক্ষর* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : সর্বজনীন V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    স্থির constexpr std::আকার_t WRITE_বাফার_আকার_BYTES = 128 * 1024;

    // MEMBERS
    ধ্রুবক std::string m_filename;
    FILE* m_fp = nullptr;
    std::আকার_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::আকার_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<অক্ষর, WRITE_বাফার_আকার_BYTES>> m_bufferp;  // Write buffer

সর্বজনীন:
    V3OutFile(ধ্রুবক string& filename, V3OutFormatter::Language lang);
    V3OutFile(ধ্রুবক V3OutFile&) = delete;
    V3OutFile& operator=(ধ্রুবক V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() ধ্রুবক { ফেরত m_filename; }

    শূন্য putsForceIncs();

    শূন্য statRecordWritten() {
        writeBlock();
        V3Stats::add_Stat_যোগ(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

ব্যক্তিগত:
    শূন্য writeBlock() {
        যদি (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->তথ্য(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    শূন্য putcOutput(অক্ষর chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        যদি (VL_UNLIKELY(m_usedBytes >= WRITE_বাফার_আকার_BYTES)) writeBlock();
    }
    শূন্য putsOutput(ধ্রুবক অক্ষর* str) override {
        std::আকার_t len = strlen(str);
        std::আকার_t availableBytes = WRITE_বাফার_আকার_BYTES - m_usedBytes;
        যখন (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->তথ্য() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_বাফার_আকার_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_বাফার_আকার_BYTES;
        }
        যদি (len > 0) {
            std::memcpy(m_bufferp->তথ্য() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : সর্বজনীন V3OutFile {
    পূর্ণসংখ্যা m_guard = false;  // Created header guard
    পূর্ণসংখ্যা m_private;  // 1 = Most recently emitted private:, 2 = public:
সর্বজনীন:
    explicit V3OutCFile(ধ্রুবক string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    আভাসী শূন্য putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    আভাসী শূন্য puts_Int_শীর্ষ_Include() { putsForceIncs(); }
    আভাসী শূন্য putsGuard();
    আভাসী শূন্য putsEndGuard() {
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

class V3OutJsonFile final : সর্বজনীন V3OutFile {
    // CONSTANTS
    স্থির constexpr ধ্রুবক অক্ষর* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
ব্যক্তিগত:
    std::স্তূপ<অক্ষর> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::স্তূপ<bool> m_empty;  // Current scope is empty, no comma later

সর্বজনীন:
    explicit V3OutJsonFile(ধ্রুবক string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    আভাসী শূন্য putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    শূন্য puts(ধ্রুবক অক্ষর* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    শূন্য puts(ধ্রুবক string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(ধ্রুবক std::string& name, অক্ষর type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.ঠেলো(type == '{' ? '}' : ']');
        যদি (!m_empty.empty()) m_empty.শীর্ষ() = false;
        m_empty.ঠেলো(true);
        ফেরত *this;
    }
    V3OutJsonFile& begin(অক্ষর type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.ঠেলো(type == '{' ? '}' : ']');
        যদি (!m_empty.empty()) m_empty.শীর্ষ() = false;
        m_empty.ঠেলো(true);
        ফেরত *this;
    }

    // Put a named value
    V3OutJsonFile& put(ধ্রুবক std::string& name, ধ্রুবক অক্ষর* valuep) {
        ফেরত putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(ধ্রুবক std::string& name, ধ্রুবক std::string& মান) {
        ফেরত putNamed(name, মান, true);
    }
    V3OutJsonFile& put(ধ্রুবক std::string& name, bool মান) {
        ফেরত putNamed(name, মান ? "true" : "false", false);
    }
    V3OutJsonFile& put(ধ্রুবক std::string& name, পূর্ণসংখ্যা মান) {
        ফেরত putNamed(name, std::to_string(মান), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(ধ্রুবক std::string& মান) { ফেরত putNamed("", মান, true); }
    V3OutJsonFile& put(bool মান) { ফেরত putNamed("", মান ? "true" : "false", false); }
    V3OutJsonFile& put(পূর্ণসংখ্যা মান) { ফেরত putNamed("", std::to_string(মান), false); }

    V3OutJsonFile& putList(ধ্রুবক std::string& name, ধ্রুবক std::vector<std::string>& list) {
        যদি (list.empty()) ফেরত *this;
        begin(name, '[');
        জন্য (ধ্রুবক std::string& str : list) put(str);
        ফেরত end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.দৈর্ঘ্য() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.শীর্ষ());
        m_scope.বের_করো();
        m_empty.বের_করো();
        ফেরত *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        ফেরত *this;
    }

ব্যক্তিগত:
    শূন্য comma() {
        যদি (m_empty.empty()) ফেরত;
        যদি (!m_empty.শীর্ষ()) puts(",\n");
        m_empty.শীর্ষ() = true;
    }
    V3OutJsonFile& putNamed(ধ্রুবক std::string& name, ধ্রুবক std::string& মান, bool quoted) {
        comma();
        ধ্রুবক string মান_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(মান) + "\"" : মান;
        যদি (name.empty()) {
            puts(m_prefix + মান_Q);
        } অন্যথা {
            puts(m_prefix + "\"" + name + "\": " + মান_Q);
        }
        m_empty.শীর্ষ() = false;
        ফেরত *this;
    }
};

class V3OutMkFile final : সর্বজনীন V3OutFile {
সর্বজনীন:
    explicit V3OutMkFile(ধ্রুবক string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = পূর্বনির্ধারিত;
    আভাসী শূন্য putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    শূন্য puts(ধ্রুবক অক্ষর* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    শূন্য puts(ধ্রুবক string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    শূন্য putSet(ধ্রুবক string& var, ধ্রুবক string& মান) {
        puts(VString::dot(var + " =", " ", মান) + "\n");
    }
    // Put VARIABLE ?= VALUE
    শূন্য putSetQ(ধ্রুবক string& var, ধ্রুবক string& মান) {
        puts(VString::dot(var + " ?=", " ", মান) + "\n");
    }
};

class V3OutVFile final : সর্বজনীন V3OutCFile {
সর্বজনীন:
    explicit V3OutVFile(ধ্রুবক string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = পূর্বনির্ধারিত;
    শূন্য putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : সর্বজনীন V3OutFile {
সর্বজনীন:
    explicit V3OutXmlFile(ধ্রুবক string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = পূর্বনির্ধারিত;
    আভাসী শূন্য putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : সর্বজনীন V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

সর্বজনীন:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = পূর্বনির্ধারিত;

    শূন্য putcOutput(অক্ষর chr) override { m_ostream << chr; };
    শূন্য putsOutput(ধ্রুবক অক্ষর* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
সর্বজনীন:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    স্থির string ifNoProtect(ধ্রুবক string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    স্থির string protect(ধ্রুবক string& old) VL_MT_SAFE { ফেরত protectIf(old, true); }
    স্থির string protectIf(ধ্রুবক string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    স্থির string protect_শব্দগুলি_If(ধ্রুবক string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    স্থির শূন্য writeMapFile(ধ্রুবক string& filename);
};

#endif  // Guard
