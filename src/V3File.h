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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

#ଅନ୍ତର୍ଭାଵ <array>
#ଅନ୍ତର୍ଭାଵ <fstream>
#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <stack>
#ଅନ୍ତର୍ଭାଵ <vector>

class Ast_ଗଣ୍ଠି;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର std::ifstream* new_ifstream(ଅଚଳ string& filename) {
        addSrcDepend(filename);
        ଫେରନ୍ତୁ new_ifstream_nodepend(filename);
    }
    ସ୍ଥିର std::ifstream* new_ifstream_nodepend(ଅଚଳ string& filename) VL_MT_SAFE {
        ଫେରନ୍ତୁ new std::ifstream{filename.c_str()};
    }
    ସ୍ଥିର std::ofstream* new_ofstream(ଅଚଳ string& filename, bool append = false) {
        addTgtDepend(filename);
        ଫେରନ୍ତୁ new_ofstream_nodepend(filename, append);
    }
    ସ୍ଥିର std::ofstream* new_ofstream_nodepend(ଅଚଳ string& filename, bool append = false) {
        createMakeDirFor(filename);
        ଯଦି (append) ଫେରନ୍ତୁ new std::ofstream{filename.c_str(), std::ios::app};
        ଫେରନ୍ତୁ new std::ofstream{filename.c_str()};
    }
    ସ୍ଥିର FILE* new_fopen_w(ଅଚଳ string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        ଫେରନ୍ତୁ fopen(filename.c_str(), "w");
    }

    // Dependencies
    ସ୍ଥିର ଶୂନ୍ୟ addSrcDepend(ଅଚଳ string& filename) VL_MT_SAFE;
    ସ୍ଥିର ଶୂନ୍ୟ addTgtDepend(ଅଚଳ string& filename) VL_MT_SAFE;
    ସ୍ଥିର ଶୂନ୍ୟ writeDepend(ଅଚଳ string& filename);
    ସ୍ଥିର std::vector<string> getAllDeps();
    ସ୍ଥିର ଶୂନ୍ୟ writeTimes(ଅଚଳ string& filename, ଅଚଳ string& cmdlineIn);
    ସ୍ଥିର bool checkTimes(ଅଚଳ string& filename, ଅଚଳ string& cmdlineIn);

    // Directory utilities
    ସ୍ଥିର ଶୂନ୍ୟ createMakeDirFor(ଅଚଳ string& filename);
    ସ୍ଥିର ଶୂନ୍ୟ createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
ସର୍ବସାଧାରଣ:
    // TYPES
    using StrList = std::list<std::string>;

ବ୍ୟକ୍ତିଗତ:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

ସର୍ବସାଧାରଣ:
    explicit VInFilter(ଅଚଳ string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ଅଚଳ string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
ସର୍ବସାଧାରଣ:
    ଗଣନା AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    ଗଣନା Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    ଅଚଳ Language m_lang;  // Indenting Verilog code
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_blockIndent;  // Characters per block indent
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_commaWidth;  // Width after which to break at ,'s
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_lineno = 1;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_column = 0;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_nobreak = false;  // Basic operator or begin paren, don't break next
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_sourceLastLineno = 0;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_indentLevel = 0;  // Current {} indentation
    std::ଗଦା<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> m_parenVec;  // Stack of columns where last ( was
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା endLevels(ଅଚଳ ଅକ୍ଷର* strg);
    ଶୂନ୍ୟ putcNoTracking(ଅକ୍ଷର chr);

ସର୍ବସାଧାରଣ:
    explicit V3OutFormatter(Language lang);
    ଆଭାସୀ ~V3OutFormatter() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    ଶୂନ୍ୟ blockIndent(ପୂର୍ଣ୍ଣସଂଖ୍ୟା flag) { m_blockIndent = flag; }
    // METHODS
    ଶୂନ୍ୟ printf(ଅଚଳ ଅକ୍ଷର* fmt...) VL_ATTR_PRINTF(2);
    ଶୂନ୍ୟ puts(ଅଚଳ ଅକ୍ଷର* strg) { putns(nullptr, strg); }
    ଶୂନ୍ୟ puts(ଅଚଳ string& strg) { putns(nullptr, strg); }
    ଶୂନ୍ୟ putns(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ ଅକ୍ଷର* strg);
    ଶୂନ୍ୟ putns(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& strg) { putns(nodep, strg.c_str()); }
    ଶୂନ୍ୟ putsNoTracking(ଅଚଳ string& strg);
    ଶୂନ୍ୟ putsQuoted(ଅଚଳ string& strg);
    ଶୂନ୍ୟ putBreak();  // Print linebreak if line is too wide
    ଶୂନ୍ୟ put_Break_ପଦସମଷ୍ଟି();  // Print linebreak in expression if line is too wide
    ଶୂନ୍ୟ putbs(ଅଚଳ ଅକ୍ଷର* strg) {
        put_Break_ପଦସମଷ୍ଟି();
        putns(nullptr, strg);
    }
    ଶୂନ୍ୟ putbs(ଅଚଳ string& strg) {
        put_Break_ପଦସମଷ୍ଟି();
        putns(nullptr, strg);
    }
    ଶୂନ୍ୟ putnbs(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& strg) {
        put_Break_ପଦସମଷ୍ଟି();
        putns(nodep, strg);
    }
    bool exceededWidth() ଅଚଳ { ଫେରନ୍ତୁ m_column > m_commaWidth; }
    ଶୂନ୍ୟ indentInc() { m_indentLevel += m_blockIndent; }
    ଶୂନ୍ୟ indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    ଶୂନ୍ୟ blockInc() { m_parenVec.ଠେଲ(m_indentLevel + m_blockIndent); }
    ଶୂନ୍ୟ blockDec() {
        ଯଦି (!m_parenVec.empty()) m_parenVec.କାଢ଼();
    }
    ଶୂନ୍ୟ ensure_New_ଧାଡ଼ି() {
        ଯଦି (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ସ୍ଥିର ଅଚଳ std::string& indentSpaces(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ);
    // Add escaped characters to strings
    ସ୍ଥିର string quoteNameControls(ଅଚଳ string& namein, Language lang = LA_C) VL_PURE;
    ସ୍ଥିର bool tokenMatch(ଅଚଳ ଅକ୍ଷର* cp, ଅଚଳ ଅକ୍ଷର* cmp);
    ସ୍ଥିର bool tokenNotStart(ଅଚଳ ଅକ୍ଷର* cp);  // Import/export meaning no endfunction
    ସ୍ଥିର bool tokenStart(ଅଚଳ ଅକ୍ଷର* cp);
    ସ୍ଥିର bool tokenEnd(ଅଚଳ ଅକ୍ଷର* cp);

    // CALLBACKS - MUST OVERRIDE
    ଆଭାସୀ ଶୂନ୍ୟ putcOutput(ଅକ୍ଷର chr) = 0;
    ଆଭାସୀ ଶୂନ୍ୟ putsOutput(ଅଚଳ ଅକ୍ଷର* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ସ୍ଥିର constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    ଅଚଳ std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<ଅକ୍ଷର, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

ସର୍ବସାଧାରଣ:
    V3OutFile(ଅଚଳ string& filename, V3OutFormatter::Language lang);
    V3OutFile(ଅଚଳ V3OutFile&) = delete;
    V3OutFile& operator=(ଅଚଳ V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() ଅଚଳ { ଫେରନ୍ତୁ m_filename; }

    ଶୂନ୍ୟ putsForceIncs();

    ଶୂନ୍ୟ statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ writeBlock() {
        ଯଦି (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    ଶୂନ୍ୟ putcOutput(ଅକ୍ଷର chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        ଯଦି (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    ଶୂନ୍ୟ putsOutput(ଅଚଳ ଅକ୍ଷର* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        ଯେତେବେଳେ (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        ଯଦି (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3OutFile {
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_guard = false;  // Created header guard
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_private;  // 1 = Most recently emitted private:, 2 = public:
ସର୍ବସାଧାରଣ:
    explicit V3OutCFile(ଅଚଳ string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    ଆଭାସୀ ଶୂନ୍ୟ putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    ଆଭାସୀ ଶୂନ୍ୟ puts_Int_ଶୀର୍ଷ_Include() { putsForceIncs(); }
    ଆଭାସୀ ଶୂନ୍ୟ putsGuard();
    ଆଭାସୀ ଶୂନ୍ୟ putsEndGuard() {
        ଯଦି (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    ଶୂନ୍ୟ resetPrivate() { m_private = 0; }
    ଶୂନ୍ୟ putsPrivate(bool setPrivate) {
        ଯଦି (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } ଅନ୍ୟଥା ଯଦି (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : ସର୍ବସାଧାରଣ V3OutFile {
    // CONSTANTS
    ସ୍ଥିର constexpr ଅଚଳ ଅକ୍ଷର* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
ବ୍ୟକ୍ତିଗତ:
    std::ଗଦା<ଅକ୍ଷର> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::ଗଦା<bool> m_empty;  // Current scope is empty, no comma later

ସର୍ବସାଧାରଣ:
    explicit V3OutJsonFile(ଅଚଳ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    ଆଭାସୀ ଶୂନ୍ୟ putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    ଶୂନ୍ୟ puts(ଅଚଳ ଅକ୍ଷର* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ଶୂନ୍ୟ puts(ଅଚଳ string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(ଅଚଳ std::string& name, ଅକ୍ଷର type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.ଠେଲ(type == '{' ? '}' : ']');
        ଯଦି (!m_empty.empty()) m_empty.ଶୀର୍ଷ() = false;
        m_empty.ଠେଲ(true);
        ଫେରନ୍ତୁ *this;
    }
    V3OutJsonFile& begin(ଅକ୍ଷର type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.ଠେଲ(type == '{' ? '}' : ']');
        ଯଦି (!m_empty.empty()) m_empty.ଶୀର୍ଷ() = false;
        m_empty.ଠେଲ(true);
        ଫେରନ୍ତୁ *this;
    }

    // Put a named value
    V3OutJsonFile& put(ଅଚଳ std::string& name, ଅଚଳ ଅକ୍ଷର* valuep) {
        ଫେରନ୍ତୁ putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(ଅଚଳ std::string& name, ଅଚଳ std::string& ମୂଲ୍ୟ) {
        ଫେରନ୍ତୁ putNamed(name, ମୂଲ୍ୟ, true);
    }
    V3OutJsonFile& put(ଅଚଳ std::string& name, bool ମୂଲ୍ୟ) {
        ଫେରନ୍ତୁ putNamed(name, ମୂଲ୍ୟ ? "true" : "false", false);
    }
    V3OutJsonFile& put(ଅଚଳ std::string& name, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) {
        ଫେରନ୍ତୁ putNamed(name, std::to_string(ମୂଲ୍ୟ), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(ଅଚଳ std::string& ମୂଲ୍ୟ) { ଫେରନ୍ତୁ putNamed("", ମୂଲ୍ୟ, true); }
    V3OutJsonFile& put(bool ମୂଲ୍ୟ) { ଫେରନ୍ତୁ putNamed("", ମୂଲ୍ୟ ? "true" : "false", false); }
    V3OutJsonFile& put(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) { ଫେରନ୍ତୁ putNamed("", std::to_string(ମୂଲ୍ୟ), false); }

    V3OutJsonFile& putList(ଅଚଳ std::string& name, ଅଚଳ std::vector<std::string>& list) {
        ଯଦି (list.empty()) ଫେରନ୍ତୁ *this;
        begin(name, '[');
        ପାଇଁ (ଅଚଳ std::string& str : list) put(str);
        ଫେରନ୍ତୁ end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.ଶୀର୍ଷ());
        m_scope.କାଢ଼();
        m_empty.କାଢ଼();
        ଫେରନ୍ତୁ *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        ଫେରନ୍ତୁ *this;
    }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ comma() {
        ଯଦି (m_empty.empty()) ଫେରନ୍ତୁ;
        ଯଦି (!m_empty.ଶୀର୍ଷ()) puts(",\n");
        m_empty.ଶୀର୍ଷ() = true;
    }
    V3OutJsonFile& putNamed(ଅଚଳ std::string& name, ଅଚଳ std::string& ମୂଲ୍ୟ, bool quoted) {
        comma();
        ଅଚଳ string ମୂଲ୍ୟ_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(ମୂଲ୍ୟ) + "\"" : ମୂଲ୍ୟ;
        ଯଦି (name.empty()) {
            puts(m_prefix + ମୂଲ୍ୟ_Q);
        } ଅନ୍ୟଥା {
            puts(m_prefix + "\"" + name + "\": " + ମୂଲ୍ୟ_Q);
        }
        m_empty.ଶୀର୍ଷ() = false;
        ଫେରନ୍ତୁ *this;
    }
};

class V3OutMkFile final : ସର୍ବସାଧାରଣ V3OutFile {
ସର୍ବସାଧାରଣ:
    explicit V3OutMkFile(ଅଚଳ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଆଭାସୀ ଶୂନ୍ୟ putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    ଶୂନ୍ୟ puts(ଅଚଳ ଅକ୍ଷର* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ଶୂନ୍ୟ puts(ଅଚଳ string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    ଶୂନ୍ୟ putSet(ଅଚଳ string& var, ଅଚଳ string& ମୂଲ୍ୟ) {
        puts(VString::dot(var + " =", " ", ମୂଲ୍ୟ) + "\n");
    }
    // Put VARIABLE ?= VALUE
    ଶୂନ୍ୟ putSetQ(ଅଚଳ string& var, ଅଚଳ string& ମୂଲ୍ୟ) {
        puts(VString::dot(var + " ?=", " ", ମୂଲ୍ୟ) + "\n");
    }
};

class V3OutVFile final : ସର୍ବସାଧାରଣ V3OutCFile {
ସର୍ବସାଧାରଣ:
    explicit V3OutVFile(ଅଚଳ string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : ସର୍ବସାଧାରଣ V3OutFile {
ସର୍ବସାଧାରଣ:
    explicit V3OutXmlFile(ଅଚଳ string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଆଭାସୀ ଶୂନ୍ୟ putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

ସର୍ବସାଧାରଣ:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଶୂନ୍ୟ putcOutput(ଅକ୍ଷର chr) override { m_ostream << chr; };
    ଶୂନ୍ୟ putsOutput(ଅଚଳ ଅକ୍ଷର* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
ସର୍ବସାଧାରଣ:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    ସ୍ଥିର string ifNoProtect(ଅଚଳ string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ସ୍ଥିର string protect(ଅଚଳ string& old) VL_MT_SAFE { ଫେରନ୍ତୁ protectIf(old, true); }
    ସ୍ଥିର string protectIf(ଅଚଳ string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    ସ୍ଥିର string protect_ଶବ୍ଦଗୁଡ଼ିକ_If(ଅଚଳ string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    ସ୍ଥିର ଶୂନ୍ୟ writeMapFile(ଅଚଳ string& filename);
};

#endif  // Guard
