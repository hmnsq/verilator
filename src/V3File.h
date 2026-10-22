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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <fstream>
#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <stack>
#ಅನ್ತರ್ಭಾವ <vector>

class Ast_ಗಂಟು;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::ifstream* new_ifstream(ಧ್ರುವ string& filename) {
        addSrcDepend(filename);
        ಪ್ರತ್ಯಾಗಮ new_ifstream_nodepend(filename);
    }
    ಸ್ಥಿರ std::ifstream* new_ifstream_nodepend(ಧ್ರುವ string& filename) VL_MT_SAFE {
        ಪ್ರತ್ಯಾಗಮ new std::ifstream{filename.c_str()};
    }
    ಸ್ಥಿರ std::ofstream* new_ofstream(ಧ್ರುವ string& filename, bool append = false) {
        addTgtDepend(filename);
        ಪ್ರತ್ಯಾಗಮ new_ofstream_nodepend(filename, append);
    }
    ಸ್ಥಿರ std::ofstream* new_ofstream_nodepend(ಧ್ರುವ string& filename, bool append = false) {
        createMakeDirFor(filename);
        ಯದಿ (append) ಪ್ರತ್ಯಾಗಮ new std::ofstream{filename.c_str(), std::ios::app};
        ಪ್ರತ್ಯಾಗಮ new std::ofstream{filename.c_str()};
    }
    ಸ್ಥಿರ FILE* new_fopen_w(ಧ್ರುವ string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        ಪ್ರತ್ಯಾಗಮ fopen(filename.c_str(), "w");
    }

    // Dependencies
    ಸ್ಥಿರ ಶೂನ್ಯ addSrcDepend(ಧ್ರುವ string& filename) VL_MT_SAFE;
    ಸ್ಥಿರ ಶೂನ್ಯ addTgtDepend(ಧ್ರುವ string& filename) VL_MT_SAFE;
    ಸ್ಥಿರ ಶೂನ್ಯ writeDepend(ಧ್ರುವ string& filename);
    ಸ್ಥಿರ std::vector<string> getAllDeps();
    ಸ್ಥಿರ ಶೂನ್ಯ writeTimes(ಧ್ರುವ string& filename, ಧ್ರುವ string& cmdlineIn);
    ಸ್ಥಿರ bool checkTimes(ಧ್ರುವ string& filename, ಧ್ರುವ string& cmdlineIn);

    // Directory utilities
    ಸ್ಥಿರ ಶೂನ್ಯ createMakeDirFor(ಧ್ರುವ string& filename);
    ಸ್ಥಿರ ಶೂನ್ಯ createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using StrList = std::list<std::string>;

ಗುಪ್ತ:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

ಸಾರ್ವಜನಿಕ:
    explicit VInFilter(ಧ್ರುವ string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ಧ್ರುವ string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    ಗಣನಾ Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

ಗುಪ್ತ:
    // MEMBERS
    ಧ್ರುವ Language m_lang;  // Indenting Verilog code
    ಪೂರ್ಣಾಙ್ಕ m_blockIndent;  // Characters per block indent
    ಪೂರ್ಣಾಙ್ಕ m_commaWidth;  // Width after which to break at ,'s
    ಪೂರ್ಣಾಙ್ಕ m_lineno = 1;
    ಪೂರ್ಣಾಙ್ಕ m_column = 0;
    ಪೂರ್ಣಾಙ್ಕ m_nobreak = false;  // Basic operator or begin paren, don't break next
    ಪೂರ್ಣಾಙ್ಕ m_sourceLastLineno = 0;
    ಪೂರ್ಣಾಙ್ಕ m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    ಪೂರ್ಣಾಙ್ಕ m_indentLevel = 0;  // Current {} indentation
    std::ರಾಶಿ<ಪೂರ್ಣಾಙ್ಕ> m_parenVec;  // Stack of columns where last ( was
    ಪೂರ್ಣಾಙ್ಕ m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    ಪೂರ್ಣಾಙ್ಕ endLevels(ಧ್ರುವ ವರ್ಣ* strg);
    ಶೂನ್ಯ putcNoTracking(ವರ್ಣ chr);

ಸಾರ್ವಜನಿಕ:
    explicit V3OutFormatter(Language lang);
    ಆಭಾಸ ~V3OutFormatter() = ಸ್ವಭಾವ;
    // ACCESSORS
    ಶೂನ್ಯ blockIndent(ಪೂರ್ಣಾಙ್ಕ flag) { m_blockIndent = flag; }
    // METHODS
    ಶೂನ್ಯ printf(ಧ್ರುವ ವರ್ಣ* fmt...) VL_ATTR_PRINTF(2);
    ಶೂನ್ಯ puts(ಧ್ರುವ ವರ್ಣ* strg) { putns(nullptr, strg); }
    ಶೂನ್ಯ puts(ಧ್ರುವ string& strg) { putns(nullptr, strg); }
    ಶೂನ್ಯ putns(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ ವರ್ಣ* strg);
    ಶೂನ್ಯ putns(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& strg) { putns(nodep, strg.c_str()); }
    ಶೂನ್ಯ putsNoTracking(ಧ್ರುವ string& strg);
    ಶೂನ್ಯ putsQuoted(ಧ್ರುವ string& strg);
    ಶೂನ್ಯ putBreak();  // Print linebreak if line is too wide
    ಶೂನ್ಯ put_Break_ಅಭಿವ್ಯಕ್ತಿ();  // Print linebreak in expression if line is too wide
    ಶೂನ್ಯ putbs(ಧ್ರುವ ವರ್ಣ* strg) {
        put_Break_ಅಭಿವ್ಯಕ್ತಿ();
        putns(nullptr, strg);
    }
    ಶೂನ್ಯ putbs(ಧ್ರುವ string& strg) {
        put_Break_ಅಭಿವ್ಯಕ್ತಿ();
        putns(nullptr, strg);
    }
    ಶೂನ್ಯ putnbs(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& strg) {
        put_Break_ಅಭಿವ್ಯಕ್ತಿ();
        putns(nodep, strg);
    }
    bool exceededWidth() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_column > m_commaWidth; }
    ಶೂನ್ಯ indentInc() { m_indentLevel += m_blockIndent; }
    ಶೂನ್ಯ indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    ಶೂನ್ಯ blockInc() { m_parenVec.ತಳ್ಳು(m_indentLevel + m_blockIndent); }
    ಶೂನ್ಯ blockDec() {
        ಯದಿ (!m_parenVec.empty()) m_parenVec.ತೆಗೆ();
    }
    ಶೂನ್ಯ ensure_New_ಸಾಲು() {
        ಯದಿ (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ಸ್ಥಿರ ಧ್ರುವ std::string& indentSpaces(ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ);
    // Add escaped characters to strings
    ಸ್ಥಿರ string quoteNameControls(ಧ್ರುವ string& namein, Language lang = LA_C) VL_PURE;
    ಸ್ಥಿರ bool tokenMatch(ಧ್ರುವ ವರ್ಣ* cp, ಧ್ರುವ ವರ್ಣ* cmp);
    ಸ್ಥಿರ bool tokenNotStart(ಧ್ರುವ ವರ್ಣ* cp);  // Import/export meaning no endfunction
    ಸ್ಥಿರ bool tokenStart(ಧ್ರುವ ವರ್ಣ* cp);
    ಸ್ಥಿರ bool tokenEnd(ಧ್ರುವ ವರ್ಣ* cp);

    // CALLBACKS - MUST OVERRIDE
    ಆಭಾಸ ಶೂನ್ಯ putcOutput(ವರ್ಣ chr) = 0;
    ಆಭಾಸ ಶೂನ್ಯ putsOutput(ಧ್ರುವ ವರ್ಣ* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ಸ್ಥಿರ constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    ಧ್ರುವ std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<ವರ್ಣ, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

ಸಾರ್ವಜನಿಕ:
    V3OutFile(ಧ್ರುವ string& filename, V3OutFormatter::Language lang);
    V3OutFile(ಧ್ರುವ V3OutFile&) = delete;
    V3OutFile& operator=(ಧ್ರುವ V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_filename; }

    ಶೂನ್ಯ putsForceIncs();

    ಶೂನ್ಯ statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

ಗುಪ್ತ:
    ಶೂನ್ಯ writeBlock() {
        ಯದಿ (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    ಶೂನ್ಯ putcOutput(ವರ್ಣ chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        ಯದಿ (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    ಶೂನ್ಯ putsOutput(ಧ್ರುವ ವರ್ಣ* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        ಯಾವತ್ (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        ಯದಿ (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3OutFile {
    ಪೂರ್ಣಾಙ್ಕ m_guard = false;  // Created header guard
    ಪೂರ್ಣಾಙ್ಕ m_private;  // 1 = Most recently emitted private:, 2 = public:
ಸಾರ್ವಜನಿಕ:
    explicit V3OutCFile(ಧ್ರುವ string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    ಆಭಾಸ ಶೂನ್ಯ putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    ಆಭಾಸ ಶೂನ್ಯ puts_Int_ತುದಿ_Include() { putsForceIncs(); }
    ಆಭಾಸ ಶೂನ್ಯ putsGuard();
    ಆಭಾಸ ಶೂನ್ಯ putsEndGuard() {
        ಯದಿ (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    ಶೂನ್ಯ resetPrivate() { m_private = 0; }
    ಶೂನ್ಯ putsPrivate(bool setPrivate) {
        ಯದಿ (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } ಅನ್ಯಥಾ ಯದಿ (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : ಸಾರ್ವಜನಿಕ V3OutFile {
    // CONSTANTS
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
ಗುಪ್ತ:
    std::ರಾಶಿ<ವರ್ಣ> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::ರಾಶಿ<bool> m_empty;  // Current scope is empty, no comma later

ಸಾರ್ವಜನಿಕ:
    explicit V3OutJsonFile(ಧ್ರುವ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    ಆಭಾಸ ಶೂನ್ಯ putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ puts(ಧ್ರುವ ವರ್ಣ* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ puts(ಧ್ರುವ string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(ಧ್ರುವ std::string& name, ವರ್ಣ type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.ತಳ್ಳು(type == '{' ? '}' : ']');
        ಯದಿ (!m_empty.empty()) m_empty.ತುದಿ() = false;
        m_empty.ತಳ್ಳು(true);
        ಪ್ರತ್ಯಾಗಮ *this;
    }
    V3OutJsonFile& begin(ವರ್ಣ type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.ತಳ್ಳು(type == '{' ? '}' : ']');
        ಯದಿ (!m_empty.empty()) m_empty.ತುದಿ() = false;
        m_empty.ತಳ್ಳು(true);
        ಪ್ರತ್ಯಾಗಮ *this;
    }

    // Put a named value
    V3OutJsonFile& put(ಧ್ರುವ std::string& name, ಧ್ರುವ ವರ್ಣ* valuep) {
        ಪ್ರತ್ಯಾಗಮ putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(ಧ್ರುವ std::string& name, ಧ್ರುವ std::string& ಮೌಲ್ಯ) {
        ಪ್ರತ್ಯಾಗಮ putNamed(name, ಮೌಲ್ಯ, true);
    }
    V3OutJsonFile& put(ಧ್ರುವ std::string& name, bool ಮೌಲ್ಯ) {
        ಪ್ರತ್ಯಾಗಮ putNamed(name, ಮೌಲ್ಯ ? "true" : "false", false);
    }
    V3OutJsonFile& put(ಧ್ರುವ std::string& name, ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) {
        ಪ್ರತ್ಯಾಗಮ putNamed(name, std::to_string(ಮೌಲ್ಯ), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(ಧ್ರುವ std::string& ಮೌಲ್ಯ) { ಪ್ರತ್ಯಾಗಮ putNamed("", ಮೌಲ್ಯ, true); }
    V3OutJsonFile& put(bool ಮೌಲ್ಯ) { ಪ್ರತ್ಯಾಗಮ putNamed("", ಮೌಲ್ಯ ? "true" : "false", false); }
    V3OutJsonFile& put(ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) { ಪ್ರತ್ಯಾಗಮ putNamed("", std::to_string(ಮೌಲ್ಯ), false); }

    V3OutJsonFile& putList(ಧ್ರುವ std::string& name, ಧ್ರುವ std::vector<std::string>& list) {
        ಯದಿ (list.empty()) ಪ್ರತ್ಯಾಗಮ *this;
        begin(name, '[');
        ಪ್ರತಿ (ಧ್ರುವ std::string& str : list) put(str);
        ಪ್ರತ್ಯಾಗಮ end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.ತುದಿ());
        m_scope.ತೆಗೆ();
        m_empty.ತೆಗೆ();
        ಪ್ರತ್ಯಾಗಮ *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        ಪ್ರತ್ಯಾಗಮ *this;
    }

ಗುಪ್ತ:
    ಶೂನ್ಯ comma() {
        ಯದಿ (m_empty.empty()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!m_empty.ತುದಿ()) puts(",\n");
        m_empty.ತುದಿ() = true;
    }
    V3OutJsonFile& putNamed(ಧ್ರುವ std::string& name, ಧ್ರುವ std::string& ಮೌಲ್ಯ, bool quoted) {
        comma();
        ಧ್ರುವ string ಮೌಲ್ಯ_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(ಮೌಲ್ಯ) + "\"" : ಮೌಲ್ಯ;
        ಯದಿ (name.empty()) {
            puts(m_prefix + ಮೌಲ್ಯ_Q);
        } ಅನ್ಯಥಾ {
            puts(m_prefix + "\"" + name + "\": " + ಮೌಲ್ಯ_Q);
        }
        m_empty.ತುದಿ() = false;
        ಪ್ರತ್ಯಾಗಮ *this;
    }
};

class V3OutMkFile final : ಸಾರ್ವಜನಿಕ V3OutFile {
ಸಾರ್ವಜನಿಕ:
    explicit V3OutMkFile(ಧ್ರುವ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = ಸ್ವಭಾವ;
    ಆಭಾಸ ಶೂನ್ಯ putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ puts(ಧ್ರುವ ವರ್ಣ* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ puts(ಧ್ರುವ string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    ಶೂನ್ಯ putSet(ಧ್ರುವ string& var, ಧ್ರುವ string& ಮೌಲ್ಯ) {
        puts(VString::dot(var + " =", " ", ಮೌಲ್ಯ) + "\n");
    }
    // Put VARIABLE ?= VALUE
    ಶೂನ್ಯ putSetQ(ಧ್ರುವ string& var, ಧ್ರುವ string& ಮೌಲ್ಯ) {
        puts(VString::dot(var + " ?=", " ", ಮೌಲ್ಯ) + "\n");
    }
};

class V3OutVFile final : ಸಾರ್ವಜನಿಕ V3OutCFile {
ಸಾರ್ವಜನಿಕ:
    explicit V3OutVFile(ಧ್ರುವ string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = ಸ್ವಭಾವ;
    ಶೂನ್ಯ putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : ಸಾರ್ವಜನಿಕ V3OutFile {
ಸಾರ್ವಜನಿಕ:
    explicit V3OutXmlFile(ಧ್ರುವ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = ಸ್ವಭಾವ;
    ಆಭಾಸ ಶೂನ್ಯ putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

ಸಾರ್ವಜನಿಕ:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = ಸ್ವಭಾವ;

    ಶೂನ್ಯ putcOutput(ವರ್ಣ chr) override { m_ostream << chr; };
    ಶೂನ್ಯ putsOutput(ಧ್ರುವ ವರ್ಣ* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
ಸಾರ್ವಜನಿಕ:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    ಸ್ಥಿರ string ifNoProtect(ಧ್ರುವ string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ಸ್ಥಿರ string protect(ಧ್ರುವ string& old) VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ protectIf(old, true); }
    ಸ್ಥಿರ string protectIf(ಧ್ರುವ string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    ಸ್ಥಿರ string protect_ಪದಗಳು_If(ಧ್ರುವ string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    ಸ್ಥಿರ ಶೂನ್ಯ writeMapFile(ಧ್ರುವ string& filename);
};

#endif  // Guard
