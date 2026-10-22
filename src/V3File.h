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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3Stats.h"

#அந்தர்பாவ <array>
#அந்தர்பாவ <fstream>
#அந்தர்பாவ <list>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <set>
#அந்தர்பாவ <stack>
#அந்தர்பாவ <vector>

class Ast_முனை;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
பொது:
    நிலையான std::ifstream* new_ifstream(மாறிலி string& filename) {
        addSrcDepend(filename);
        திரும்பு new_ifstream_nodepend(filename);
    }
    நிலையான std::ifstream* new_ifstream_nodepend(மாறிலி string& filename) VL_MT_SAFE {
        திரும்பு new std::ifstream{filename.c_str()};
    }
    நிலையான std::ofstream* new_ofstream(மாறிலி string& filename, bool append = false) {
        addTgtDepend(filename);
        திரும்பு new_ofstream_nodepend(filename, append);
    }
    நிலையான std::ofstream* new_ofstream_nodepend(மாறிலி string& filename, bool append = false) {
        createMakeDirFor(filename);
        எனில் (append) திரும்பு new std::ofstream{filename.c_str(), std::ios::app};
        திரும்பு new std::ofstream{filename.c_str()};
    }
    நிலையான FILE* new_fopen_w(மாறிலி string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        திரும்பு fopen(filename.c_str(), "w");
    }

    // Dependencies
    நிலையான வெறுமை addSrcDepend(மாறிலி string& filename) VL_MT_SAFE;
    நிலையான வெறுமை addTgtDepend(மாறிலி string& filename) VL_MT_SAFE;
    நிலையான வெறுமை writeDepend(மாறிலி string& filename);
    நிலையான std::vector<string> getAllDeps();
    நிலையான வெறுமை writeTimes(மாறிலி string& filename, மாறிலி string& cmdlineIn);
    நிலையான bool checkTimes(மாறிலி string& filename, மாறிலி string& cmdlineIn);

    // Directory utilities
    நிலையான வெறுமை createMakeDirFor(மாறிலி string& filename);
    நிலையான வெறுமை createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
பொது:
    // TYPES
    using StrList = std::list<std::string>;

தனிப்பட்ட:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

பொது:
    explicit VInFilter(மாறிலி string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(மாறிலி string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
பொது:
    எண்ணிக்கை AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    எண்ணிக்கை Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

தனிப்பட்ட:
    // MEMBERS
    மாறிலி Language m_lang;  // Indenting Verilog code
    முழு m_blockIndent;  // Characters per block indent
    முழு m_commaWidth;  // Width after which to break at ,'s
    முழு m_lineno = 1;
    முழு m_column = 0;
    முழு m_nobreak = false;  // Basic operator or begin paren, don't break next
    முழு m_sourceLastLineno = 0;
    முழு m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    முழு m_indentLevel = 0;  // Current {} indentation
    std::அடுக்கு<முழு> m_parenVec;  // Stack of columns where last ( was
    முழு m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    முழு endLevels(மாறிலி எழுத்து* strg);
    வெறுமை putcNoTracking(எழுத்து chr);

பொது:
    explicit V3OutFormatter(Language lang);
    மெய்நிகர் ~V3OutFormatter() = இயல்பு;
    // ACCESSORS
    வெறுமை blockIndent(முழு flag) { m_blockIndent = flag; }
    // METHODS
    வெறுமை printf(மாறிலி எழுத்து* fmt...) VL_ATTR_PRINTF(2);
    வெறுமை puts(மாறிலி எழுத்து* strg) { putns(nullptr, strg); }
    வெறுமை puts(மாறிலி string& strg) { putns(nullptr, strg); }
    வெறுமை putns(மாறிலி Ast_முனை* nodep, மாறிலி எழுத்து* strg);
    வெறுமை putns(மாறிலி Ast_முனை* nodep, மாறிலி string& strg) { putns(nodep, strg.c_str()); }
    வெறுமை putsNoTracking(மாறிலி string& strg);
    வெறுமை putsQuoted(மாறிலி string& strg);
    வெறுமை putBreak();  // Print linebreak if line is too wide
    வெறுமை put_Break_கோவை();  // Print linebreak in expression if line is too wide
    வெறுமை putbs(மாறிலி எழுத்து* strg) {
        put_Break_கோவை();
        putns(nullptr, strg);
    }
    வெறுமை putbs(மாறிலி string& strg) {
        put_Break_கோவை();
        putns(nullptr, strg);
    }
    வெறுமை putnbs(மாறிலி Ast_முனை* nodep, மாறிலி string& strg) {
        put_Break_கோவை();
        putns(nodep, strg);
    }
    bool exceededWidth() மாறிலி { திரும்பு m_column > m_commaWidth; }
    வெறுமை indentInc() { m_indentLevel += m_blockIndent; }
    வெறுமை indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    வெறுமை blockInc() { m_parenVec.தள்ளு(m_indentLevel + m_blockIndent); }
    வெறுமை blockDec() {
        எனில் (!m_parenVec.empty()) m_parenVec.எடு();
    }
    வெறுமை ensure_New_வரி() {
        எனில் (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    நிலையான மாறிலி std::string& indentSpaces(முழு இலக்கம்);
    // Add escaped characters to strings
    நிலையான string quoteNameControls(மாறிலி string& namein, Language lang = LA_C) VL_PURE;
    நிலையான bool tokenMatch(மாறிலி எழுத்து* cp, மாறிலி எழுத்து* cmp);
    நிலையான bool tokenNotStart(மாறிலி எழுத்து* cp);  // Import/export meaning no endfunction
    நிலையான bool tokenStart(மாறிலி எழுத்து* cp);
    நிலையான bool tokenEnd(மாறிலி எழுத்து* cp);

    // CALLBACKS - MUST OVERRIDE
    மெய்நிகர் வெறுமை putcOutput(எழுத்து chr) = 0;
    மெய்நிகர் வெறுமை putsOutput(மாறிலி எழுத்து* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : பொது V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    நிலையான constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    மாறிலி std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<எழுத்து, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

பொது:
    V3OutFile(மாறிலி string& filename, V3OutFormatter::Language lang);
    V3OutFile(மாறிலி V3OutFile&) = delete;
    V3OutFile& operator=(மாறிலி V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() மாறிலி { திரும்பு m_filename; }

    வெறுமை putsForceIncs();

    வெறுமை statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

தனிப்பட்ட:
    வெறுமை writeBlock() {
        எனில் (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    வெறுமை putcOutput(எழுத்து chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        எனில் (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    வெறுமை putsOutput(மாறிலி எழுத்து* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        போது (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        எனில் (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : பொது V3OutFile {
    முழு m_guard = false;  // Created header guard
    முழு m_private;  // 1 = Most recently emitted private:, 2 = public:
பொது:
    explicit V3OutCFile(மாறிலி string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    மெய்நிகர் வெறுமை putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    மெய்நிகர் வெறுமை puts_Int_உச்சி_Include() { putsForceIncs(); }
    மெய்நிகர் வெறுமை putsGuard();
    மெய்நிகர் வெறுமை putsEndGuard() {
        எனில் (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    வெறுமை resetPrivate() { m_private = 0; }
    வெறுமை putsPrivate(bool setPrivate) {
        எனில் (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } இல்லையெனில் எனில் (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : பொது V3OutFile {
    // CONSTANTS
    நிலையான constexpr மாறிலி எழுத்து* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
தனிப்பட்ட:
    std::அடுக்கு<எழுத்து> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::அடுக்கு<bool> m_empty;  // Current scope is empty, no comma later

பொது:
    explicit V3OutJsonFile(மாறிலி string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    மெய்நிகர் வெறுமை putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    வெறுமை puts(மாறிலி எழுத்து* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    வெறுமை puts(மாறிலி string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(மாறிலி std::string& name, எழுத்து type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.தள்ளு(type == '{' ? '}' : ']');
        எனில் (!m_empty.empty()) m_empty.உச்சி() = false;
        m_empty.தள்ளு(true);
        திரும்பு *this;
    }
    V3OutJsonFile& begin(எழுத்து type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.தள்ளு(type == '{' ? '}' : ']');
        எனில் (!m_empty.empty()) m_empty.உச்சி() = false;
        m_empty.தள்ளு(true);
        திரும்பு *this;
    }

    // Put a named value
    V3OutJsonFile& put(மாறிலி std::string& name, மாறிலி எழுத்து* valuep) {
        திரும்பு putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(மாறிலி std::string& name, மாறிலி std::string& மதிப்பு) {
        திரும்பு putNamed(name, மதிப்பு, true);
    }
    V3OutJsonFile& put(மாறிலி std::string& name, bool மதிப்பு) {
        திரும்பு putNamed(name, மதிப்பு ? "true" : "false", false);
    }
    V3OutJsonFile& put(மாறிலி std::string& name, முழு மதிப்பு) {
        திரும்பு putNamed(name, std::to_string(மதிப்பு), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(மாறிலி std::string& மதிப்பு) { திரும்பு putNamed("", மதிப்பு, true); }
    V3OutJsonFile& put(bool மதிப்பு) { திரும்பு putNamed("", மதிப்பு ? "true" : "false", false); }
    V3OutJsonFile& put(முழு மதிப்பு) { திரும்பு putNamed("", std::to_string(மதிப்பு), false); }

    V3OutJsonFile& putList(மாறிலி std::string& name, மாறிலி std::vector<std::string>& list) {
        எனில் (list.empty()) திரும்பு *this;
        begin(name, '[');
        க்கு (மாறிலி std::string& str : list) put(str);
        திரும்பு end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.உச்சி());
        m_scope.எடு();
        m_empty.எடு();
        திரும்பு *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        திரும்பு *this;
    }

தனிப்பட்ட:
    வெறுமை comma() {
        எனில் (m_empty.empty()) திரும்பு;
        எனில் (!m_empty.உச்சி()) puts(",\n");
        m_empty.உச்சி() = true;
    }
    V3OutJsonFile& putNamed(மாறிலி std::string& name, மாறிலி std::string& மதிப்பு, bool quoted) {
        comma();
        மாறிலி string மதிப்பு_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(மதிப்பு) + "\"" : மதிப்பு;
        எனில் (name.empty()) {
            puts(m_prefix + மதிப்பு_Q);
        } இல்லையெனில் {
            puts(m_prefix + "\"" + name + "\": " + மதிப்பு_Q);
        }
        m_empty.உச்சி() = false;
        திரும்பு *this;
    }
};

class V3OutMkFile final : பொது V3OutFile {
பொது:
    explicit V3OutMkFile(மாறிலி string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = இயல்பு;
    மெய்நிகர் வெறுமை putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    வெறுமை puts(மாறிலி எழுத்து* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    வெறுமை puts(மாறிலி string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    வெறுமை putSet(மாறிலி string& var, மாறிலி string& மதிப்பு) {
        puts(VString::dot(var + " =", " ", மதிப்பு) + "\n");
    }
    // Put VARIABLE ?= VALUE
    வெறுமை putSetQ(மாறிலி string& var, மாறிலி string& மதிப்பு) {
        puts(VString::dot(var + " ?=", " ", மதிப்பு) + "\n");
    }
};

class V3OutVFile final : பொது V3OutCFile {
பொது:
    explicit V3OutVFile(மாறிலி string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = இயல்பு;
    வெறுமை putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : பொது V3OutFile {
பொது:
    explicit V3OutXmlFile(மாறிலி string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = இயல்பு;
    மெய்நிகர் வெறுமை putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : பொது V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

பொது:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = இயல்பு;

    வெறுமை putcOutput(எழுத்து chr) override { m_ostream << chr; };
    வெறுமை putsOutput(மாறிலி எழுத்து* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
பொது:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    நிலையான string ifNoProtect(மாறிலி string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    நிலையான string protect(மாறிலி string& old) VL_MT_SAFE { திரும்பு protectIf(old, true); }
    நிலையான string protectIf(மாறிலி string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    நிலையான string protect_சொற்கள்_If(மாறிலி string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    நிலையான வெறுமை writeMapFile(மாறிலி string& filename);
};

#endif  // Guard
