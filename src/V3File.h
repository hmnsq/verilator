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

#اگر_غیر_تعریف_شدہ VERILATOR_V3FILE_H_
#تعریف_کرو VERILATOR_V3FILE_H_

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

class Ast_گرہ;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
عوامی:
    ساکن std::ifstream* new_ifstream(ثابت string& filename) {
        جوڑو_Src_Depend(filename);
        واپس new_ifstream_nodepend(filename);
    }
    ساکن std::ifstream* new_ifstream_nodepend(ثابت string& filename) VL_MT_محفوظ {
        واپس new std::ifstream{filename.c_str()};
    }
    ساکن std::ofstream* new_ofstream(ثابت string& filename, bool append = false) {
        جوڑو_Tgt_Depend(filename);
        واپس new_ofstream_nodepend(filename, append);
    }
    ساکن std::ofstream* new_ofstream_nodepend(ثابت string& filename, bool append = false) {
        createMakeDirFor(filename);
        اگر (append) واپس new std::ofstream{filename.c_str(), std::ios::app};
        واپس new std::ofstream{filename.c_str()};
    }
    ساکن FILE* new_fopen_w(ثابت string& filename) {
        createMakeDirFor(filename);
        جوڑو_Tgt_Depend(filename);
        واپس fopen(filename.c_str(), "w");
    }

    // Dependencies
    ساکن خالی جوڑو_Src_Depend(ثابت string& filename) VL_MT_محفوظ;
    ساکن خالی جوڑو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ;
    ساکن خالی writeDepend(ثابت string& filename);
    ساکن std::سمتیہ<string> getAllDeps();
    ساکن خالی writeTimes(ثابت string& filename, ثابت string& cmdline_اندر);
    ساکن bool checkTimes(ثابت string& filename, ثابت string& cmdline_اندر);

    // Directory utilities
    ساکن خالی createMakeDirFor(ثابت string& filename);
    ساکن خالی createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
عوامی:
    // TYPES
    using StrList = std::list<std::string>;

نجی:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

عوامی:
    explicit VInFilter(ثابت string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ثابت string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3_برآمد_Formatter VL_NOT_FINAL {
    // TYPES
عوامی:
    شمار AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    شمار Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

نجی:
    // MEMBERS
    ثابت Language m_lang;  // Indenting Verilog code
    صحیح m_بلاک_Indent;  // Characters per block indent
    صحیح m_comma_چوڑائی;  // Width after which to break at ,'s
    صحیح m_lineno = 1;
    صحیح m_عمود = 0;
    صحیح m_nobreak = false;  // Basic operator or begin paren, don't break next
    صحیح m_ماخذ_آخری_Lineno = 0;
    صحیح m_ماخذ_آخری_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_اندر_String_Literal = false;  // Inside a quote
    bool m_اندر_Backslash = false;  // Inside a backslash
    صحیح m_indentLevel = 0;  // Current {} indentation
    std::ڈھیر<صحیح> m_parenVec;  // Stack of columns where last ( was
    صحیح m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    صحیح انتہا_سطحیں(ثابت حرف* strg);
    خالی putcNoTracking(حرف chr);

عوامی:
    explicit V3_برآمد_Formatter(Language lang);
    مجازی ~V3_برآمد_Formatter() = طےشدہ;
    // ACCESSORS
    خالی بلاک_Indent(صحیح flag) { m_بلاک_Indent = flag; }
    // METHODS
    خالی لکھو(ثابت حرف* fmt...) VL_ATTR_PRINTF(2);
    خالی puts(ثابت حرف* strg) { putns(nullptr, strg); }
    خالی puts(ثابت string& strg) { putns(nullptr, strg); }
    خالی putns(ثابت Ast_گرہ* nodep, ثابت حرف* strg);
    خالی putns(ثابت Ast_گرہ* nodep, ثابت string& strg) { putns(nodep, strg.c_str()); }
    خالی putsNoTracking(ثابت string& strg);
    خالی putsQuoted(ثابت string& strg);
    خالی putBreak();  // Print linebreak if line is too wide
    خالی put_Break_اظہاریہ();  // Print linebreak in expression if line is too wide
    خالی putbs(ثابت حرف* strg) {
        put_Break_اظہاریہ();
        putns(nullptr, strg);
    }
    خالی putbs(ثابت string& strg) {
        put_Break_اظہاریہ();
        putns(nullptr, strg);
    }
    خالی putnbs(ثابت Ast_گرہ* nodep, ثابت string& strg) {
        put_Break_اظہاریہ();
        putns(nodep, strg);
    }
    bool exceeded_چوڑائی() ثابت { واپس m_عمود > m_comma_چوڑائی; }
    خالی indentInc() { m_indentLevel += m_بلاک_Indent; }
    خالی indentDec() {
        m_indentLevel -= m_بلاک_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    خالی بلاک_Inc() { m_parenVec.ڈالو(m_indentLevel + m_بلاک_Indent); }
    خالی بلاک_Dec() {
        اگر (!m_parenVec.خالی_خانے()) m_parenVec.نکالو();
    }
    خالی ensure_New_سطر() {
        اگر (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ساکن ثابت std::string& indentSpaces(صحیح عدد);
    // Add escaped characters to strings
    ساکن string quoteNameControls(ثابت string& namein, Language lang = LA_C) VL_خالص;
    ساکن bool token_مطابقت(ثابت حرف* cp, ثابت حرف* cmp);
    ساکن bool token_Not_آغاز(ثابت حرف* cp);  // Import/export meaning no endfunction
    ساکن bool token_آغاز(ثابت حرف* cp);
    ساکن bool token_انتہا(ثابت حرف* cp);

    // CALLBACKS - MUST OVERRIDE
    مجازی خالی putcOutput(حرف chr) = 0;
    مجازی خالی putsOutput(ثابت حرف* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_برآمد_File VL_NOT_FINAL : عوامی V3_برآمد_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ساکن constexpr std::سائز_t WRITE_بفر_سائز_بائٹیں = 128 * 1024;

    // MEMBERS
    ثابت std::string m_filename;
    FILE* m_fp = nullptr;
    std::سائز_t m_مستعمل_بائٹیں = 0;  // Number of bytes stored in m_bufferp
    std::سائز_t m_written_بائٹیں = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<حرف, WRITE_بفر_سائز_بائٹیں>> m_bufferp;  // Write buffer

عوامی:
    V3_برآمد_File(ثابت string& filename, V3_برآمد_Formatter::Language lang);
    V3_برآمد_File(ثابت V3_برآمد_File&) = delete;
    V3_برآمد_File& عامل=(ثابت V3_برآمد_File&) = delete;
    V3_برآمد_File(V3_برآمد_File&&) = delete;
    V3_برآمد_File& عامل=(V3_برآمد_File&&) = delete;
    ~V3_برآمد_File() override;

    std::string filename() ثابت { واپس m_filename; }

    خالی putsForceIncs();

    خالی stat_ریکارڈ_Written() {
        write_بلاک();
        V3Stats::جوڑو_Stat_مجموع(V3Stats::STAT_CPP_CHARS, m_written_بائٹیں);
    }

نجی:
    خالی write_بلاک() {
        اگر (VL_LIKELY(m_مستعمل_بائٹیں > 0)) {
            fwrite(m_bufferp->ڈیٹا(), m_مستعمل_بائٹیں, 1, m_fp);
            m_written_بائٹیں += m_مستعمل_بائٹیں;
            m_مستعمل_بائٹیں = 0;
        }
    }
    // CALLBACKS
    خالی putcOutput(حرف chr) override {
        m_bufferp->at(m_مستعمل_بائٹیں++) = chr;
        اگر (VL_UNLIKELY(m_مستعمل_بائٹیں >= WRITE_بفر_سائز_بائٹیں)) write_بلاک();
    }
    خالی putsOutput(ثابت حرف* str) override {
        std::سائز_t len = لڑی_لمبائی(str);
        std::سائز_t دستیاب_بائٹیں = WRITE_بفر_سائز_بائٹیں - m_مستعمل_بائٹیں;
        جب (VL_UNLIKELY(len >= دستیاب_بائٹیں)) {
            std::حافظہ_نقل(m_bufferp->ڈیٹا() + m_مستعمل_بائٹیں, str, دستیاب_بائٹیں);
            m_مستعمل_بائٹیں = WRITE_بفر_سائز_بائٹیں;
            write_بلاک();
            str += دستیاب_بائٹیں;
            len -= دستیاب_بائٹیں;
            دستیاب_بائٹیں = WRITE_بفر_سائز_بائٹیں;
        }
        اگر (len > 0) {
            std::حافظہ_نقل(m_bufferp->ڈیٹا() + m_مستعمل_بائٹیں, str, len);
            m_مستعمل_بائٹیں += len;
        }
    }
};

class V3_برآمد_CFile VL_NOT_FINAL : عوامی V3_برآمد_File {
    صحیح m_guard = false;  // Created header guard
    صحیح m_نجی;  // 1 = Most recently emitted private:, 2 = public:
عوامی:
    explicit V3_برآمد_CFile(ثابت string& filename,
                        V3_برآمد_Formatter::Language lang = V3_برآمد_Formatter::LA_C)
        : V3_برآمد_File{filename, lang} {
        reset_نجی();
    }
    ~V3_برآمد_CFile() override { stat_ریکارڈ_Written(); }
    مجازی خالی puts_سرخی() { puts("// Verilated -*- C++ -*-\n"); }
    مجازی خالی puts_Int_چوٹی_Include() { putsForceIncs(); }
    مجازی خالی putsGuard();
    مجازی خالی puts_انتہا_Guard() {
        اگر (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    خالی reset_نجی() { m_نجی = 0; }
    خالی puts_نجی(bool سیٹ_نجی) {
        اگر (سیٹ_نجی && m_نجی != 1) {
            puts("private:\n");
            m_نجی = 1;
        } ورنہ اگر (!سیٹ_نجی && m_نجی != 2) {
            puts("public:\n");
            m_نجی = 2;
        }
    }
};

class V3_برآمد_Json_File final : عوامی V3_برآمد_File {
    // CONSTANTS
    ساکن constexpr ثابت حرف* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
نجی:
    std::ڈھیر<حرف> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_سابقہ;  // Prefix emitted before each line in current scope
    std::ڈھیر<bool> m_خالی_خانے;  // Current scope is empty, no comma later

عوامی:
    explicit V3_برآمد_Json_File(ثابت string& filename)
        : V3_برآمد_File{filename, V3_برآمد_Formatter::LA_JSON} {
        begin();
    }
    ~V3_برآمد_Json_File() override {
        انتہا();
        puts("\n");
    }
    مجازی خالی puts_سرخی() {}
    // cppcheck-suppress duplInheritedMember
    خالی puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    خالی puts(ثابت string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_برآمد_Json_File& begin(ثابت std::string& name, حرف type = '{') {
        comma();
        puts(m_سابقہ + "\"" + name + "\": " + type + "\n");
        m_سابقہ += INDENT;
        m_scope.ڈالو(type == '{' ? '}' : ']');
        اگر (!m_خالی_خانے.خالی_خانے()) m_خالی_خانے.چوٹی() = false;
        m_خالی_خانے.ڈالو(true);
        واپس *this;
    }
    V3_برآمد_Json_File& begin(حرف type = '{') {
        comma();
        puts(m_سابقہ + type + "\n");
        m_سابقہ += INDENT;
        m_scope.ڈالو(type == '{' ? '}' : ']');
        اگر (!m_خالی_خانے.خالی_خانے()) m_خالی_خانے.چوٹی() = false;
        m_خالی_خانے.ڈالو(true);
        واپس *this;
    }

    // Put a named value
    V3_برآمد_Json_File& put(ثابت std::string& name, ثابت حرف* valuep) {
        واپس putNamed(name, std::string{valuep}, true);
    }
    V3_برآمد_Json_File& put(ثابت std::string& name, ثابت std::string& قدر) {
        واپس putNamed(name, قدر, true);
    }
    V3_برآمد_Json_File& put(ثابت std::string& name, bool قدر) {
        واپس putNamed(name, قدر ? "true" : "false", false);
    }
    V3_برآمد_Json_File& put(ثابت std::string& name, صحیح قدر) {
        واپس putNamed(name, std::مقصد_string(قدر), false);
    }

    // Put unnamed value
    V3_برآمد_Json_File& put(ثابت std::string& قدر) { واپس putNamed("", قدر, true); }
    V3_برآمد_Json_File& put(bool قدر) { واپس putNamed("", قدر ? "true" : "false", false); }
    V3_برآمد_Json_File& put(صحیح قدر) { واپس putNamed("", std::مقصد_string(قدر), false); }

    V3_برآمد_Json_File& putList(ثابت std::string& name, ثابت std::سمتیہ<std::string>& list) {
        اگر (list.خالی_خانے()) واپس *this;
        begin(name, '[');
        برائے (ثابت std::string& str : list) put(str);
        واپس انتہا();
    }

    V3_برآمد_Json_File& انتہا() {
        UASSERT(m_سابقہ.لمبائی_2() >= لڑی_لمبائی(INDENT), "prefix underflow");
        m_سابقہ.erase(m_سابقہ.انتہا() - لڑی_لمبائی(INDENT), m_سابقہ.انتہا());
        UASSERT(!m_scope.خالی_خانے(), "end() without begin()");
        UASSERT(!m_خالی_خانے.خالی_خانے(), "end() without begin()");
        puts("\n" + m_سابقہ + m_scope.چوٹی());
        m_scope.نکالو();
        m_خالی_خانے.نکالو();
        واپس *this;
    }

    V3_برآمد_Json_File& عامل+=(V3_برآمد_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        واپس *this;
    }

نجی:
    خالی comma() {
        اگر (m_خالی_خانے.خالی_خانے()) واپس;
        اگر (!m_خالی_خانے.چوٹی()) puts(",\n");
        m_خالی_خانے.چوٹی() = true;
    }
    V3_برآمد_Json_File& putNamed(ثابت std::string& name, ثابت std::string& قدر, bool quoted) {
        comma();
        ثابت string قدر_Q
            = quoted ? "\""s + V3_برآمد_Formatter::quoteNameControls(قدر) + "\"" : قدر;
        اگر (name.خالی_خانے()) {
            puts(m_سابقہ + قدر_Q);
        } ورنہ {
            puts(m_سابقہ + "\"" + name + "\": " + قدر_Q);
        }
        m_خالی_خانے.چوٹی() = false;
        واپس *this;
    }
};

class V3_برآمد_Mk_File final : عوامی V3_برآمد_File {
عوامی:
    explicit V3_برآمد_Mk_File(ثابت string& filename)
        : V3_برآمد_File{filename, V3_برآمد_Formatter::LA_MK} {}
    ~V3_برآمد_Mk_File() override = طےشدہ;
    مجازی خالی puts_سرخی() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    خالی puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    خالی puts(ثابت string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    خالی put_سیٹ(ثابت string& var, ثابت string& قدر) {
        puts(VString::dot(var + " =", " ", قدر) + "\n");
    }
    // Put VARIABLE ?= VALUE
    خالی put_سیٹ_Q(ثابت string& var, ثابت string& قدر) {
        puts(VString::dot(var + " ?=", " ", قدر) + "\n");
    }
};

class V3_برآمد_VFile final : عوامی V3_برآمد_CFile {
عوامی:
    explicit V3_برآمد_VFile(ثابت string& filename)
        : V3_برآمد_CFile{filename, V3_برآمد_Formatter::LA_VERILOG} {}
    ~V3_برآمد_VFile() override = طےشدہ;
    خالی puts_سرخی() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3_برآمد_Xml_File final : عوامی V3_برآمد_File {
عوامی:
    explicit V3_برآمد_Xml_File(ثابت string& filename)
        : V3_برآمد_File{filename, V3_برآمد_Formatter::LA_XML} {
        بلاک_Indent(2);
    }
    ~V3_برآمد_Xml_File() override = طےشدہ;
    مجازی خالی puts_سرخی() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_برآمد_دھارا VL_NOT_FINAL : عوامی V3_برآمد_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_برآمد_دھارا);
    VL_UNMOVABLE(V3_برآمد_دھارا);

عوامی:
    V3_برآمد_دھارا(std::ostream& ostream, V3_برآمد_Formatter::Language lang);
    ~V3_برآمد_دھارا() override = طےشدہ;

    خالی putcOutput(حرف chr) override { m_ostream << chr; };
    خالی putsOutput(ثابت حرف* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
عوامی:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    ساکن string ifNoProtect(ثابت string& اندر) VL_MT_محفوظ;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ساکن string protect(ثابت string& old) VL_MT_محفوظ { واپس protectIf(old, true); }
    ساکن string protectIf(ثابت string& old, bool doIt = true) VL_MT_محفوظ;
    // Rename words to a new encoded string
    ساکن string protect_الفاظ_If(ثابت string& old, bool doIt = true) VL_MT_محفوظ;
    // Write map of renames to output file
    ساکن خالی writeMapFile(ثابت string& filename);
};

#اگر_ختم  // Guard
