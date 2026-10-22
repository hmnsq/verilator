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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3Stats.h"

#شامل <array>
#شامل <fstream>
#شامل <list>
#شامل <memory>
#شامل <set>
#شامل <stack>
#شامل <vector>

class Ast_گره;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
عمومی:
    ایستا std::ifstream* new_ifstream(ثابت string& filename) {
        addSrcDepend(filename);
        بازگشت new_ifstream_nodepend(filename);
    }
    ایستا std::ifstream* new_ifstream_nodepend(ثابت string& filename) VL_MT_SAFE {
        بازگشت new std::ifstream{filename.c_str()};
    }
    ایستا std::ofstream* new_ofstream(ثابت string& filename, bool append = false) {
        addTgtDepend(filename);
        بازگشت new_ofstream_nodepend(filename, append);
    }
    ایستا std::ofstream* new_ofstream_nodepend(ثابت string& filename, bool append = false) {
        createMakeDirFor(filename);
        اگر (append) بازگشت new std::ofstream{filename.c_str(), std::ios::app};
        بازگشت new std::ofstream{filename.c_str()};
    }
    ایستا FILE* new_fopen_w(ثابت string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        بازگشت fopen(filename.c_str(), "w");
    }

    // Dependencies
    ایستا تهی addSrcDepend(ثابت string& filename) VL_MT_SAFE;
    ایستا تهی addTgtDepend(ثابت string& filename) VL_MT_SAFE;
    ایستا تهی writeDepend(ثابت string& filename);
    ایستا std::vector<string> getAllDeps();
    ایستا تهی writeTimes(ثابت string& filename, ثابت string& cmdlineIn);
    ایستا bool checkTimes(ثابت string& filename, ثابت string& cmdlineIn);

    // Directory utilities
    ایستا تهی createMakeDirFor(ثابت string& filename);
    ایستا تهی createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
عمومی:
    // TYPES
    using StrList = std::list<std::string>;

خصوصی:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

عمومی:
    explicit VInFilter(ثابت string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ثابت string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
عمومی:
    شمارش AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    شمارش Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

خصوصی:
    // MEMBERS
    ثابت Language m_lang;  // Indenting Verilog code
    صحیح m_blockIndent;  // Characters per block indent
    صحیح m_commaWidth;  // Width after which to break at ,'s
    صحیح m_lineno = 1;
    صحیح m_column = 0;
    صحیح m_nobreak = false;  // Basic operator or begin paren, don't break next
    صحیح m_sourceLastLineno = 0;
    صحیح m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    صحیح m_indentLevel = 0;  // Current {} indentation
    std::پشته<صحیح> m_parenVec;  // Stack of columns where last ( was
    صحیح m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    صحیح endLevels(ثابت نویسه* strg);
    تهی putcNoTracking(نویسه chr);

عمومی:
    explicit V3OutFormatter(Language lang);
    مجازی ~V3OutFormatter() = پیشفرض;
    // ACCESSORS
    تهی blockIndent(صحیح flag) { m_blockIndent = flag; }
    // METHODS
    تهی printf(ثابت نویسه* fmt...) VL_ATTR_PRINTF(2);
    تهی puts(ثابت نویسه* strg) { putns(nullptr, strg); }
    تهی puts(ثابت string& strg) { putns(nullptr, strg); }
    تهی putns(ثابت Ast_گره* nodep, ثابت نویسه* strg);
    تهی putns(ثابت Ast_گره* nodep, ثابت string& strg) { putns(nodep, strg.c_str()); }
    تهی putsNoTracking(ثابت string& strg);
    تهی putsQuoted(ثابت string& strg);
    تهی putBreak();  // Print linebreak if line is too wide
    تهی put_Break_عبارت();  // Print linebreak in expression if line is too wide
    تهی putbs(ثابت نویسه* strg) {
        put_Break_عبارت();
        putns(nullptr, strg);
    }
    تهی putbs(ثابت string& strg) {
        put_Break_عبارت();
        putns(nullptr, strg);
    }
    تهی putnbs(ثابت Ast_گره* nodep, ثابت string& strg) {
        put_Break_عبارت();
        putns(nodep, strg);
    }
    bool exceededWidth() ثابت { بازگشت m_column > m_commaWidth; }
    تهی indentInc() { m_indentLevel += m_blockIndent; }
    تهی indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    تهی blockInc() { m_parenVec.افزودن(m_indentLevel + m_blockIndent); }
    تهی blockDec() {
        اگر (!m_parenVec.empty()) m_parenVec.برداشت();
    }
    تهی ensure_New_سطر() {
        اگر (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ایستا ثابت std::string& indentSpaces(صحیح رقم);
    // Add escaped characters to strings
    ایستا string quoteNameControls(ثابت string& namein, Language lang = LA_C) VL_PURE;
    ایستا bool tokenMatch(ثابت نویسه* cp, ثابت نویسه* cmp);
    ایستا bool tokenNotStart(ثابت نویسه* cp);  // Import/export meaning no endfunction
    ایستا bool tokenStart(ثابت نویسه* cp);
    ایستا bool tokenEnd(ثابت نویسه* cp);

    // CALLBACKS - MUST OVERRIDE
    مجازی تهی putcOutput(نویسه chr) = 0;
    مجازی تهی putsOutput(ثابت نویسه* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : عمومی V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ایستا constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    ثابت std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<نویسه, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

عمومی:
    V3OutFile(ثابت string& filename, V3OutFormatter::Language lang);
    V3OutFile(ثابت V3OutFile&) = delete;
    V3OutFile& operator=(ثابت V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() ثابت { بازگشت m_filename; }

    تهی putsForceIncs();

    تهی statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

خصوصی:
    تهی writeBlock() {
        اگر (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    تهی putcOutput(نویسه chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        اگر (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    تهی putsOutput(ثابت نویسه* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        وقتی (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        اگر (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : عمومی V3OutFile {
    صحیح m_guard = false;  // Created header guard
    صحیح m_private;  // 1 = Most recently emitted private:, 2 = public:
عمومی:
    explicit V3OutCFile(ثابت string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    مجازی تهی putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    مجازی تهی puts_Int_بالاترین_Include() { putsForceIncs(); }
    مجازی تهی putsGuard();
    مجازی تهی putsEndGuard() {
        اگر (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    تهی resetPrivate() { m_private = 0; }
    تهی putsPrivate(bool setPrivate) {
        اگر (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } وگرنه اگر (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : عمومی V3OutFile {
    // CONSTANTS
    ایستا constexpr ثابت نویسه* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
خصوصی:
    std::پشته<نویسه> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::پشته<bool> m_empty;  // Current scope is empty, no comma later

عمومی:
    explicit V3OutJsonFile(ثابت string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    مجازی تهی putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    تهی puts(ثابت نویسه* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    تهی puts(ثابت string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(ثابت std::string& name, نویسه type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.افزودن(type == '{' ? '}' : ']');
        اگر (!m_empty.empty()) m_empty.بالاترین() = false;
        m_empty.افزودن(true);
        بازگشت *this;
    }
    V3OutJsonFile& begin(نویسه type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.افزودن(type == '{' ? '}' : ']');
        اگر (!m_empty.empty()) m_empty.بالاترین() = false;
        m_empty.افزودن(true);
        بازگشت *this;
    }

    // Put a named value
    V3OutJsonFile& put(ثابت std::string& name, ثابت نویسه* valuep) {
        بازگشت putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(ثابت std::string& name, ثابت std::string& مقدار) {
        بازگشت putNamed(name, مقدار, true);
    }
    V3OutJsonFile& put(ثابت std::string& name, bool مقدار) {
        بازگشت putNamed(name, مقدار ? "true" : "false", false);
    }
    V3OutJsonFile& put(ثابت std::string& name, صحیح مقدار) {
        بازگشت putNamed(name, std::to_string(مقدار), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(ثابت std::string& مقدار) { بازگشت putNamed("", مقدار, true); }
    V3OutJsonFile& put(bool مقدار) { بازگشت putNamed("", مقدار ? "true" : "false", false); }
    V3OutJsonFile& put(صحیح مقدار) { بازگشت putNamed("", std::to_string(مقدار), false); }

    V3OutJsonFile& putList(ثابت std::string& name, ثابت std::vector<std::string>& list) {
        اگر (list.empty()) بازگشت *this;
        begin(name, '[');
        برای (ثابت std::string& str : list) put(str);
        بازگشت end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.بالاترین());
        m_scope.برداشت();
        m_empty.برداشت();
        بازگشت *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        بازگشت *this;
    }

خصوصی:
    تهی comma() {
        اگر (m_empty.empty()) بازگشت;
        اگر (!m_empty.بالاترین()) puts(",\n");
        m_empty.بالاترین() = true;
    }
    V3OutJsonFile& putNamed(ثابت std::string& name, ثابت std::string& مقدار, bool quoted) {
        comma();
        ثابت string مقدار_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(مقدار) + "\"" : مقدار;
        اگر (name.empty()) {
            puts(m_prefix + مقدار_Q);
        } وگرنه {
            puts(m_prefix + "\"" + name + "\": " + مقدار_Q);
        }
        m_empty.بالاترین() = false;
        بازگشت *this;
    }
};

class V3OutMkFile final : عمومی V3OutFile {
عمومی:
    explicit V3OutMkFile(ثابت string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = پیشفرض;
    مجازی تهی putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    تهی puts(ثابت نویسه* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    تهی puts(ثابت string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    تهی putSet(ثابت string& var, ثابت string& مقدار) {
        puts(VString::dot(var + " =", " ", مقدار) + "\n");
    }
    // Put VARIABLE ?= VALUE
    تهی putSetQ(ثابت string& var, ثابت string& مقدار) {
        puts(VString::dot(var + " ?=", " ", مقدار) + "\n");
    }
};

class V3OutVFile final : عمومی V3OutCFile {
عمومی:
    explicit V3OutVFile(ثابت string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = پیشفرض;
    تهی putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : عمومی V3OutFile {
عمومی:
    explicit V3OutXmlFile(ثابت string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = پیشفرض;
    مجازی تهی putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : عمومی V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

عمومی:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = پیشفرض;

    تهی putcOutput(نویسه chr) override { m_ostream << chr; };
    تهی putsOutput(ثابت نویسه* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
عمومی:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    ایستا string ifNoProtect(ثابت string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ایستا string protect(ثابت string& old) VL_MT_SAFE { بازگشت protectIf(old, true); }
    ایستا string protectIf(ثابت string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    ایستا string protect_واژگان_If(ثابت string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    ایستا تهی writeMapFile(ثابت string& filename);
};

#endif  // Guard
