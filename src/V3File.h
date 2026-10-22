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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3Stats.h"

#ضمن <array>
#ضمن <fstream>
#ضمن <list>
#ضمن <memory>
#ضمن <set>
#ضمن <stack>
#ضمن <vector>

class Ast_عقدة;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
عام:
    ساكن std::ifstream* new_ifstream(ثابت string& filename) {
        addSrcDepend(filename);
        أرجع new_ifstream_nodepend(filename);
    }
    ساكن std::ifstream* new_ifstream_nodepend(ثابت string& filename) VL_MT_SAFE {
        أرجع new std::ifstream{filename.c_str()};
    }
    ساكن std::ofstream* new_ofstream(ثابت string& filename, bool append = false) {
        addTgtDepend(filename);
        أرجع new_ofstream_nodepend(filename, append);
    }
    ساكن std::ofstream* new_ofstream_nodepend(ثابت string& filename, bool append = false) {
        createMakeDirFor(filename);
        إذا (append) أرجع new std::ofstream{filename.c_str(), std::ios::app};
        أرجع new std::ofstream{filename.c_str()};
    }
    ساكن FILE* new_fopen_w(ثابت string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        أرجع fopen(filename.c_str(), "w");
    }

    // Dependencies
    ساكن فراغ addSrcDepend(ثابت string& filename) VL_MT_SAFE;
    ساكن فراغ addTgtDepend(ثابت string& filename) VL_MT_SAFE;
    ساكن فراغ writeDepend(ثابت string& filename);
    ساكن std::vector<string> getAllDeps();
    ساكن فراغ writeTimes(ثابت string& filename, ثابت string& cmdlineIn);
    ساكن bool checkTimes(ثابت string& filename, ثابت string& cmdlineIn);

    // Directory utilities
    ساكن فراغ createMakeDirFor(ثابت string& filename);
    ساكن فراغ createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
عام:
    // TYPES
    using StrList = std::list<std::string>;

خاص:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

عام:
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
عام:
    تعداد AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    تعداد Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

خاص:
    // MEMBERS
    ثابت Language m_lang;  // Indenting Verilog code
    صحيح m_blockIndent;  // Characters per block indent
    صحيح m_commaWidth;  // Width after which to break at ,'s
    صحيح m_lineno = 1;
    صحيح m_column = 0;
    صحيح m_nobreak = false;  // Basic operator or begin paren, don't break next
    صحيح m_sourceLastLineno = 0;
    صحيح m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    صحيح m_indentLevel = 0;  // Current {} indentation
    std::مكدس<صحيح> m_parenVec;  // Stack of columns where last ( was
    صحيح m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    صحيح endLevels(ثابت حرف* strg);
    فراغ putcNoTracking(حرف chr);

عام:
    explicit V3OutFormatter(Language lang);
    virtual ~V3OutFormatter() = افتراضي;
    // ACCESSORS
    فراغ blockIndent(صحيح flag) { m_blockIndent = flag; }
    // METHODS
    فراغ printf(ثابت حرف* fmt...) VL_ATTR_PRINTF(2);
    فراغ puts(ثابت حرف* strg) { putns(nullptr, strg); }
    فراغ puts(ثابت string& strg) { putns(nullptr, strg); }
    فراغ putns(ثابت Ast_عقدة* nodep, ثابت حرف* strg);
    فراغ putns(ثابت Ast_عقدة* nodep, ثابت string& strg) { putns(nodep, strg.c_str()); }
    فراغ putsNoTracking(ثابت string& strg);
    فراغ putsQuoted(ثابت string& strg);
    فراغ putBreak();  // Print linebreak if line is too wide
    فراغ put_Break_تعبير();  // Print linebreak in expression if line is too wide
    فراغ putbs(ثابت حرف* strg) {
        put_Break_تعبير();
        putns(nullptr, strg);
    }
    فراغ putbs(ثابت string& strg) {
        put_Break_تعبير();
        putns(nullptr, strg);
    }
    فراغ putnbs(ثابت Ast_عقدة* nodep, ثابت string& strg) {
        put_Break_تعبير();
        putns(nodep, strg);
    }
    bool exceededWidth() ثابت { أرجع m_column > m_commaWidth; }
    فراغ indentInc() { m_indentLevel += m_blockIndent; }
    فراغ indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    فراغ blockInc() { m_parenVec.ادفع(m_indentLevel + m_blockIndent); }
    فراغ blockDec() {
        إذا (!m_parenVec.empty()) m_parenVec.اسحب();
    }
    فراغ ensure_New_سطر() {
        إذا (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ساكن ثابت std::string& indentSpaces(صحيح رقم);
    // Add escaped characters to strings
    ساكن string quoteNameControls(ثابت string& namein, Language lang = LA_C) VL_PURE;
    ساكن bool tokenMatch(ثابت حرف* cp, ثابت حرف* cmp);
    ساكن bool tokenNotStart(ثابت حرف* cp);  // Import/export meaning no endfunction
    ساكن bool tokenStart(ثابت حرف* cp);
    ساكن bool tokenEnd(ثابت حرف* cp);

    // CALLBACKS - MUST OVERRIDE
    virtual فراغ putcOutput(حرف chr) = 0;
    virtual فراغ putsOutput(ثابت حرف* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : عام V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ساكن constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    ثابت std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<حرف, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

عام:
    V3OutFile(ثابت string& filename, V3OutFormatter::Language lang);
    V3OutFile(ثابت V3OutFile&) = delete;
    V3OutFile& operator=(ثابت V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() ثابت { أرجع m_filename; }

    فراغ putsForceIncs();

    فراغ statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

خاص:
    فراغ writeBlock() {
        إذا (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    فراغ putcOutput(حرف chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        إذا (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    فراغ putsOutput(ثابت حرف* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        طالما (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        إذا (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : عام V3OutFile {
    صحيح m_guard = false;  // Created header guard
    صحيح m_private;  // 1 = Most recently emitted private:, 2 = public:
عام:
    explicit V3OutCFile(ثابت string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    virtual فراغ putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    virtual فراغ puts_Int_قمة_Include() { putsForceIncs(); }
    virtual فراغ putsGuard();
    virtual فراغ putsEndGuard() {
        إذا (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    فراغ resetPrivate() { m_private = 0; }
    فراغ putsPrivate(bool setPrivate) {
        إذا (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } وإلا إذا (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : عام V3OutFile {
    // CONSTANTS
    ساكن constexpr ثابت حرف* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
خاص:
    std::مكدس<حرف> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::مكدس<bool> m_empty;  // Current scope is empty, no comma later

عام:
    explicit V3OutJsonFile(ثابت string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    virtual فراغ putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(ثابت std::string& name, حرف type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.ادفع(type == '{' ? '}' : ']');
        إذا (!m_empty.empty()) m_empty.قمة() = false;
        m_empty.ادفع(true);
        أرجع *this;
    }
    V3OutJsonFile& begin(حرف type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.ادفع(type == '{' ? '}' : ']');
        إذا (!m_empty.empty()) m_empty.قمة() = false;
        m_empty.ادفع(true);
        أرجع *this;
    }

    // Put a named value
    V3OutJsonFile& put(ثابت std::string& name, ثابت حرف* valuep) {
        أرجع putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(ثابت std::string& name, ثابت std::string& قيمة) {
        أرجع putNamed(name, قيمة, true);
    }
    V3OutJsonFile& put(ثابت std::string& name, bool قيمة) {
        أرجع putNamed(name, قيمة ? "true" : "false", false);
    }
    V3OutJsonFile& put(ثابت std::string& name, صحيح قيمة) {
        أرجع putNamed(name, std::to_string(قيمة), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(ثابت std::string& قيمة) { أرجع putNamed("", قيمة, true); }
    V3OutJsonFile& put(bool قيمة) { أرجع putNamed("", قيمة ? "true" : "false", false); }
    V3OutJsonFile& put(صحيح قيمة) { أرجع putNamed("", std::to_string(قيمة), false); }

    V3OutJsonFile& putList(ثابت std::string& name, ثابت std::vector<std::string>& list) {
        إذا (list.empty()) أرجع *this;
        begin(name, '[');
        لكل (ثابت std::string& str : list) put(str);
        أرجع end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.قمة());
        m_scope.اسحب();
        m_empty.اسحب();
        أرجع *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        أرجع *this;
    }

خاص:
    فراغ comma() {
        إذا (m_empty.empty()) أرجع;
        إذا (!m_empty.قمة()) puts(",\n");
        m_empty.قمة() = true;
    }
    V3OutJsonFile& putNamed(ثابت std::string& name, ثابت std::string& قيمة, bool quoted) {
        comma();
        ثابت string قيمة_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(قيمة) + "\"" : قيمة;
        إذا (name.empty()) {
            puts(m_prefix + قيمة_Q);
        } وإلا {
            puts(m_prefix + "\"" + name + "\": " + قيمة_Q);
        }
        m_empty.قمة() = false;
        أرجع *this;
    }
};

class V3OutMkFile final : عام V3OutFile {
عام:
    explicit V3OutMkFile(ثابت string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = افتراضي;
    virtual فراغ putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    فراغ putSet(ثابت string& var, ثابت string& قيمة) {
        puts(VString::dot(var + " =", " ", قيمة) + "\n");
    }
    // Put VARIABLE ?= VALUE
    فراغ putSetQ(ثابت string& var, ثابت string& قيمة) {
        puts(VString::dot(var + " ?=", " ", قيمة) + "\n");
    }
};

class V3OutVFile final : عام V3OutCFile {
عام:
    explicit V3OutVFile(ثابت string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = افتراضي;
    فراغ putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : عام V3OutFile {
عام:
    explicit V3OutXmlFile(ثابت string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = افتراضي;
    virtual فراغ putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : عام V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

عام:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = افتراضي;

    فراغ putcOutput(حرف chr) override { m_ostream << chr; };
    فراغ putsOutput(ثابت حرف* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
عام:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    ساكن string ifNoProtect(ثابت string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ساكن string protect(ثابت string& old) VL_MT_SAFE { أرجع protectIf(old, true); }
    ساكن string protectIf(ثابت string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    ساكن string protect_كلمات_If(ثابت string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    ساكن فراغ writeMapFile(ثابت string& filename);
};

#endif  // Guard
