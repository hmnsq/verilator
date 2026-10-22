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

#جے_بنا_تعریف VERILATOR_V3FILE_H_
#تعریف_کرو VERILATOR_V3FILE_H_

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

class Ast_گرہ;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
عام:
    ساكن std::ifstream* new_ifstream(ثابت string& filename) {
        جوڑو_Src_Depend(filename);
        أرجع new_ifstream_nodepend(filename);
    }
    ساكن std::ifstream* new_ifstream_nodepend(ثابت string& filename) VL_MT_محفوظ {
        أرجع new std::ifstream{filename.c_str()};
    }
    ساكن std::ofstream* new_ofstream(ثابت string& filename, bool append = false) {
        جوڑو_Tgt_Depend(filename);
        أرجع new_ofstream_nodepend(filename, append);
    }
    ساكن std::ofstream* new_ofstream_nodepend(ثابت string& filename, bool append = false) {
        createMakeDirFor(filename);
        إذا (append) أرجع new std::ofstream{filename.c_str(), std::ios::app};
        أرجع new std::ofstream{filename.c_str()};
    }
    ساكن FILE* new_fopen_w(ثابت string& filename) {
        createMakeDirFor(filename);
        جوڑو_Tgt_Depend(filename);
        أرجع fopen(filename.c_str(), "w");
    }

    // Dependencies
    ساكن فراغ جوڑو_Src_Depend(ثابت string& filename) VL_MT_محفوظ;
    ساكن فراغ جوڑو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ;
    ساكن فراغ writeDepend(ثابت string& filename);
    ساكن std::سمتیہ<string> getAllDeps();
    ساكن فراغ writeTimes(ثابت string& filename, ثابت string& cmdline_وچ);
    ساكن bool checkTimes(ثابت string& filename, ثابت string& cmdline_وچ);

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

class V3_باہرلا_Formatter VL_NOT_FINAL {
    // TYPES
عام:
    تعداد AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    تعداد Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

خاص:
    // MEMBERS
    ثابت Language m_lang;  // Indenting Verilog code
    صحيح m_بلاک_Indent;  // Characters per block indent
    صحيح m_comma_چوڑائی;  // Width after which to break at ,'s
    صحيح m_lineno = 1;
    صحيح m_کالم = 0;
    صحيح m_nobreak = false;  // Basic operator or begin paren, don't break next
    صحيح m_سروت_آخری_Lineno = 0;
    صحيح m_سروت_آخری_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_وچ_String_Literal = false;  // Inside a quote
    bool m_وچ_Backslash = false;  // Inside a backslash
    صحيح m_indentLevel = 0;  // Current {} indentation
    std::ڈھیر<صحيح> m_parenVec;  // Stack of columns where last ( was
    صحيح m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    صحيح سرا_پدھراں(ثابت حرف* strg);
    فراغ putcNoTracking(حرف chr);

عام:
    explicit V3_باہرلا_Formatter(Language lang);
    مجازی ~V3_باہرلا_Formatter() = افتراضي;
    // ACCESSORS
    فراغ بلاک_Indent(صحيح flag) { m_بلاک_Indent = flag; }
    // METHODS
    فراغ لکھو(ثابت حرف* fmt...) VL_ATTR_PRINTF(2);
    فراغ puts(ثابت حرف* strg) { putns(nullptr, strg); }
    فراغ puts(ثابت string& strg) { putns(nullptr, strg); }
    فراغ putns(ثابت Ast_گرہ* nodep, ثابت حرف* strg);
    فراغ putns(ثابت Ast_گرہ* nodep, ثابت string& strg) { putns(nodep, strg.c_str()); }
    فراغ putsNoTracking(ثابت string& strg);
    فراغ putsQuoted(ثابت string& strg);
    فراغ putBreak();  // Print linebreak if line is too wide
    فراغ put_Break_اظہاریہ();  // Print linebreak in expression if line is too wide
    فراغ putbs(ثابت حرف* strg) {
        put_Break_اظہاریہ();
        putns(nullptr, strg);
    }
    فراغ putbs(ثابت string& strg) {
        put_Break_اظہاریہ();
        putns(nullptr, strg);
    }
    فراغ putnbs(ثابت Ast_گرہ* nodep, ثابت string& strg) {
        put_Break_اظہاریہ();
        putns(nodep, strg);
    }
    bool exceeded_چوڑائی() ثابت { أرجع m_کالم > m_comma_چوڑائی; }
    فراغ indentInc() { m_indentLevel += m_بلاک_Indent; }
    فراغ indentDec() {
        m_indentLevel -= m_بلاک_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    فراغ بلاک_Inc() { m_parenVec.ڈالو(m_indentLevel + m_بلاک_Indent); }
    فراغ بلاک_Dec() {
        إذا (!m_parenVec.خالی_تھاواں()) m_parenVec.نکالو();
    }
    فراغ ensure_New_سطر() {
        إذا (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ساكن ثابت std::string& indentSpaces(صحيح عدد);
    // Add escaped characters to strings
    ساكن string quoteNameControls(ثابت string& namein, Language lang = LA_C) VL_خالص;
    ساكن bool token_میل(ثابت حرف* cp, ثابت حرف* cmp);
    ساكن bool token_Not_شروع(ثابت حرف* cp);  // Import/export meaning no endfunction
    ساكن bool token_شروع(ثابت حرف* cp);
    ساكن bool token_سرا(ثابت حرف* cp);

    // CALLBACKS - MUST OVERRIDE
    مجازی فراغ putcOutput(حرف chr) = 0;
    مجازی فراغ putsOutput(ثابت حرف* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_باہرلا_File VL_NOT_FINAL : عام V3_باہرلا_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ساكن constexpr std::ناپ_t WRITE_بفر_ناپ_بائٹاں = 128 * 1024;

    // MEMBERS
    ثابت std::string m_filename;
    FILE* m_fp = nullptr;
    std::ناپ_t m_ورتیا_بائٹاں = 0;  // Number of bytes stored in m_bufferp
    std::ناپ_t m_written_بائٹاں = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<حرف, WRITE_بفر_ناپ_بائٹاں>> m_bufferp;  // Write buffer

عام:
    V3_باہرلا_File(ثابت string& filename, V3_باہرلا_Formatter::Language lang);
    V3_باہرلا_File(ثابت V3_باہرلا_File&) = delete;
    V3_باہرلا_File& عامل=(ثابت V3_باہرلا_File&) = delete;
    V3_باہرلا_File(V3_باہرلا_File&&) = delete;
    V3_باہرلا_File& عامل=(V3_باہرلا_File&&) = delete;
    ~V3_باہرلا_File() override;

    std::string filename() ثابت { أرجع m_filename; }

    فراغ putsForceIncs();

    فراغ stat_ریکارڈ_Written() {
        write_بلاک();
        V3Stats::جوڑو_Stat_جوڑ(V3Stats::STAT_CPP_CHARS, m_written_بائٹاں);
    }

خاص:
    فراغ write_بلاک() {
        إذا (VL_LIKELY(m_ورتیا_بائٹاں > 0)) {
            fwrite(m_bufferp->انکڑے(), m_ورتیا_بائٹاں, 1, m_fp);
            m_written_بائٹاں += m_ورتیا_بائٹاں;
            m_ورتیا_بائٹاں = 0;
        }
    }
    // CALLBACKS
    فراغ putcOutput(حرف chr) override {
        m_bufferp->at(m_ورتیا_بائٹاں++) = chr;
        إذا (VL_UNLIKELY(m_ورتیا_بائٹاں >= WRITE_بفر_ناپ_بائٹاں)) write_بلاک();
    }
    فراغ putsOutput(ثابت حرف* str) override {
        std::ناپ_t len = لڑی_لمبائی(str);
        std::ناپ_t دستیاب_بائٹاں = WRITE_بفر_ناپ_بائٹاں - m_ورتیا_بائٹاں;
        طالما (VL_UNLIKELY(len >= دستیاب_بائٹاں)) {
            std::حافظہ_نقل(m_bufferp->انکڑے() + m_ورتیا_بائٹاں, str, دستیاب_بائٹاں);
            m_ورتیا_بائٹاں = WRITE_بفر_ناپ_بائٹاں;
            write_بلاک();
            str += دستیاب_بائٹاں;
            len -= دستیاب_بائٹاں;
            دستیاب_بائٹاں = WRITE_بفر_ناپ_بائٹاں;
        }
        إذا (len > 0) {
            std::حافظہ_نقل(m_bufferp->انکڑے() + m_ورتیا_بائٹاں, str, len);
            m_ورتیا_بائٹاں += len;
        }
    }
};

class V3_باہرلا_CFile VL_NOT_FINAL : عام V3_باہرلا_File {
    صحيح m_guard = false;  // Created header guard
    صحيح m_نجی;  // 1 = Most recently emitted private:, 2 = public:
عام:
    explicit V3_باہرلا_CFile(ثابت string& filename,
                        V3_باہرلا_Formatter::Language lang = V3_باہرلا_Formatter::LA_C)
        : V3_باہرلا_File{filename, lang} {
        reset_نجی();
    }
    ~V3_باہرلا_CFile() override { stat_ریکارڈ_Written(); }
    مجازی فراغ puts_سرخی() { puts("// Verilated -*- C++ -*-\n"); }
    مجازی فراغ puts_Int_چوٹی_Include() { putsForceIncs(); }
    مجازی فراغ putsGuard();
    مجازی فراغ puts_سرا_Guard() {
        إذا (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    فراغ reset_نجی() { m_نجی = 0; }
    فراغ puts_نجی(bool سیٹ_نجی) {
        إذا (سیٹ_نجی && m_نجی != 1) {
            puts("private:\n");
            m_نجی = 1;
        } وإلا إذا (!سیٹ_نجی && m_نجی != 2) {
            puts("public:\n");
            m_نجی = 2;
        }
    }
};

class V3_باہرلا_Json_File final : عام V3_باہرلا_File {
    // CONSTANTS
    ساكن constexpr ثابت حرف* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
خاص:
    std::ڈھیر<حرف> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_اگیتر;  // Prefix emitted before each line in current scope
    std::ڈھیر<bool> m_خالی_تھاواں;  // Current scope is empty, no comma later

عام:
    explicit V3_باہرلا_Json_File(ثابت string& filename)
        : V3_باہرلا_File{filename, V3_باہرلا_Formatter::LA_JSON} {
        begin();
    }
    ~V3_باہرلا_Json_File() override {
        سرا();
        puts("\n");
    }
    مجازی فراغ puts_سرخی() {}
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_باہرلا_Json_File& begin(ثابت std::string& name, حرف type = '{') {
        comma();
        puts(m_اگیتر + "\"" + name + "\": " + type + "\n");
        m_اگیتر += INDENT;
        m_scope.ڈالو(type == '{' ? '}' : ']');
        إذا (!m_خالی_تھاواں.خالی_تھاواں()) m_خالی_تھاواں.چوٹی() = false;
        m_خالی_تھاواں.ڈالو(true);
        أرجع *this;
    }
    V3_باہرلا_Json_File& begin(حرف type = '{') {
        comma();
        puts(m_اگیتر + type + "\n");
        m_اگیتر += INDENT;
        m_scope.ڈالو(type == '{' ? '}' : ']');
        إذا (!m_خالی_تھاواں.خالی_تھاواں()) m_خالی_تھاواں.چوٹی() = false;
        m_خالی_تھاواں.ڈالو(true);
        أرجع *this;
    }

    // Put a named value
    V3_باہرلا_Json_File& put(ثابت std::string& name, ثابت حرف* valuep) {
        أرجع putNamed(name, std::string{valuep}, true);
    }
    V3_باہرلا_Json_File& put(ثابت std::string& name, ثابت std::string& قدر) {
        أرجع putNamed(name, قدر, true);
    }
    V3_باہرلا_Json_File& put(ثابت std::string& name, bool قدر) {
        أرجع putNamed(name, قدر ? "true" : "false", false);
    }
    V3_باہرلا_Json_File& put(ثابت std::string& name, صحيح قدر) {
        أرجع putNamed(name, std::منزل_string(قدر), false);
    }

    // Put unnamed value
    V3_باہرلا_Json_File& put(ثابت std::string& قدر) { أرجع putNamed("", قدر, true); }
    V3_باہرلا_Json_File& put(bool قدر) { أرجع putNamed("", قدر ? "true" : "false", false); }
    V3_باہرلا_Json_File& put(صحيح قدر) { أرجع putNamed("", std::منزل_string(قدر), false); }

    V3_باہرلا_Json_File& putList(ثابت std::string& name, ثابت std::سمتیہ<std::string>& list) {
        إذا (list.خالی_تھاواں()) أرجع *this;
        begin(name, '[');
        لكل (ثابت std::string& str : list) put(str);
        أرجع سرا();
    }

    V3_باہرلا_Json_File& سرا() {
        UASSERT(m_اگیتر.لمائی() >= لڑی_لمبائی(INDENT), "prefix underflow");
        m_اگیتر.erase(m_اگیتر.سرا() - لڑی_لمبائی(INDENT), m_اگیتر.سرا());
        UASSERT(!m_scope.خالی_تھاواں(), "end() without begin()");
        UASSERT(!m_خالی_تھاواں.خالی_تھاواں(), "end() without begin()");
        puts("\n" + m_اگیتر + m_scope.چوٹی());
        m_scope.نکالو();
        m_خالی_تھاواں.نکالو();
        أرجع *this;
    }

    V3_باہرلا_Json_File& عامل+=(V3_باہرلا_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        أرجع *this;
    }

خاص:
    فراغ comma() {
        إذا (m_خالی_تھاواں.خالی_تھاواں()) أرجع;
        إذا (!m_خالی_تھاواں.چوٹی()) puts(",\n");
        m_خالی_تھاواں.چوٹی() = true;
    }
    V3_باہرلا_Json_File& putNamed(ثابت std::string& name, ثابت std::string& قدر, bool quoted) {
        comma();
        ثابت string قدر_Q
            = quoted ? "\""s + V3_باہرلا_Formatter::quoteNameControls(قدر) + "\"" : قدر;
        إذا (name.خالی_تھاواں()) {
            puts(m_اگیتر + قدر_Q);
        } وإلا {
            puts(m_اگیتر + "\"" + name + "\": " + قدر_Q);
        }
        m_خالی_تھاواں.چوٹی() = false;
        أرجع *this;
    }
};

class V3_باہرلا_Mk_File final : عام V3_باہرلا_File {
عام:
    explicit V3_باہرلا_Mk_File(ثابت string& filename)
        : V3_باہرلا_File{filename, V3_باہرلا_Formatter::LA_MK} {}
    ~V3_باہرلا_Mk_File() override = افتراضي;
    مجازی فراغ puts_سرخی() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    فراغ put_سیٹ(ثابت string& var, ثابت string& قدر) {
        puts(VString::dot(var + " =", " ", قدر) + "\n");
    }
    // Put VARIABLE ?= VALUE
    فراغ put_سیٹ_Q(ثابت string& var, ثابت string& قدر) {
        puts(VString::dot(var + " ?=", " ", قدر) + "\n");
    }
};

class V3_باہرلا_VFile final : عام V3_باہرلا_CFile {
عام:
    explicit V3_باہرلا_VFile(ثابت string& filename)
        : V3_باہرلا_CFile{filename, V3_باہرلا_Formatter::LA_VERILOG} {}
    ~V3_باہرلا_VFile() override = افتراضي;
    فراغ puts_سرخی() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3_باہرلا_Xml_File final : عام V3_باہرلا_File {
عام:
    explicit V3_باہرلا_Xml_File(ثابت string& filename)
        : V3_باہرلا_File{filename, V3_باہرلا_Formatter::LA_XML} {
        بلاک_Indent(2);
    }
    ~V3_باہرلا_Xml_File() override = افتراضي;
    مجازی فراغ puts_سرخی() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_باہرلا_دھارا VL_NOT_FINAL : عام V3_باہرلا_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_باہرلا_دھارا);
    VL_UNMOVABLE(V3_باہرلا_دھارا);

عام:
    V3_باہرلا_دھارا(std::ostream& ostream, V3_باہرلا_Formatter::Language lang);
    ~V3_باہرلا_دھارا() override = افتراضي;

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
    ساكن string ifNoProtect(ثابت string& وچ) VL_MT_محفوظ;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ساكن string protect(ثابت string& old) VL_MT_محفوظ { أرجع protectIf(old, true); }
    ساكن string protectIf(ثابت string& old, bool doIt = true) VL_MT_محفوظ;
    // Rename words to a new encoded string
    ساكن string protect_الفاظ_If(ثابت string& old, bool doIt = true) VL_MT_محفوظ;
    // Write map of renames to output file
    ساكن فراغ writeMapFile(ثابت string& filename);
};

#جے_انت  // Guard
