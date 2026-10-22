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

#که_نه_تعریف_شوی VERILATOR_V3FILE_H_
#تعریف_کړه VERILATOR_V3FILE_H_

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

class Ast_غوټه;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
عام:
    ساكن std::ifstream* new_ifstream(ثابت string& filename) {
        جمع_کړه_Src_Depend(filename);
        أرجع new_ifstream_nodepend(filename);
    }
    ساكن std::ifstream* new_ifstream_nodepend(ثابت string& filename) VL_MT_خوندي {
        أرجع new std::ifstream{filename.c_str()};
    }
    ساكن std::ofstream* new_ofstream(ثابت string& filename, bool append = false) {
        جمع_کړه_Tgt_Depend(filename);
        أرجع new_ofstream_nodepend(filename, append);
    }
    ساكن std::ofstream* new_ofstream_nodepend(ثابت string& filename, bool append = false) {
        createMakeDirFor(filename);
        إذا (append) أرجع new std::ofstream{filename.c_str(), std::ios::app};
        أرجع new std::ofstream{filename.c_str()};
    }
    ساكن FILE* new_fopen_w(ثابت string& filename) {
        createMakeDirFor(filename);
        جمع_کړه_Tgt_Depend(filename);
        أرجع fopen(filename.c_str(), "w");
    }

    // Dependencies
    ساكن فراغ جمع_کړه_Src_Depend(ثابت string& filename) VL_MT_خوندي;
    ساكن فراغ جمع_کړه_Tgt_Depend(ثابت string& filename) VL_MT_خوندي;
    ساكن فراغ writeDepend(ثابت string& filename);
    ساكن std::ویکټور<string> getAllDeps();
    ساكن فراغ writeTimes(ثابت string& filename, ثابت string& cmdline_دننه);
    ساكن bool checkTimes(ثابت string& filename, ثابت string& cmdline_دننه);

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

class V3_بهر_Formatter VL_NOT_FINAL {
    // TYPES
عام:
    تعداد AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    تعداد Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

خاص:
    // MEMBERS
    ثابت Language m_lang;  // Indenting Verilog code
    صحيح m_بلاک_Indent;  // Characters per block indent
    صحيح m_comma_پلنوالی;  // Width after which to break at ,'s
    صحيح m_lineno = 1;
    صحيح m_ستنه = 0;
    صحيح m_nobreak = false;  // Basic operator or begin paren, don't break next
    صحيح m_سرچینه_وروستی_Lineno = 0;
    صحيح m_سرچینه_وروستی_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_دننه_String_Literal = false;  // Inside a quote
    bool m_دننه_Backslash = false;  // Inside a backslash
    صحيح m_indentLevel = 0;  // Current {} indentation
    std::ډېری<صحيح> m_parenVec;  // Stack of columns where last ( was
    صحيح m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    صحيح پای_ټکی_پوړونه(ثابت حرف* strg);
    فراغ putcNoTracking(حرف chr);

عام:
    explicit V3_بهر_Formatter(Language lang);
    مجازي ~V3_بهر_Formatter() = افتراضي;
    // ACCESSORS
    فراغ بلاک_Indent(صحيح flag) { m_بلاک_Indent = flag; }
    // METHODS
    فراغ چاپ_کړه(ثابت حرف* fmt...) VL_ATTR_PRINTF(2);
    فراغ puts(ثابت حرف* strg) { putns(nullptr, strg); }
    فراغ puts(ثابت string& strg) { putns(nullptr, strg); }
    فراغ putns(ثابت Ast_غوټه* nodep, ثابت حرف* strg);
    فراغ putns(ثابت Ast_غوټه* nodep, ثابت string& strg) { putns(nodep, strg.c_str()); }
    فراغ putsNoTracking(ثابت string& strg);
    فراغ putsQuoted(ثابت string& strg);
    فراغ putBreak();  // Print linebreak if line is too wide
    فراغ put_Break_بیان();  // Print linebreak in expression if line is too wide
    فراغ putbs(ثابت حرف* strg) {
        put_Break_بیان();
        putns(nullptr, strg);
    }
    فراغ putbs(ثابت string& strg) {
        put_Break_بیان();
        putns(nullptr, strg);
    }
    فراغ putnbs(ثابت Ast_غوټه* nodep, ثابت string& strg) {
        put_Break_بیان();
        putns(nodep, strg);
    }
    bool exceeded_پلنوالی() ثابت { أرجع m_ستنه > m_comma_پلنوالی; }
    فراغ indentInc() { m_indentLevel += m_بلاک_Indent; }
    فراغ indentDec() {
        m_indentLevel -= m_بلاک_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    فراغ بلاک_Inc() { m_parenVec.ټېل(m_indentLevel + m_بلاک_Indent); }
    فراغ بلاک_Dec() {
        إذا (!m_parenVec.تشه()) m_parenVec.راوباسه();
    }
    فراغ ensure_New_کرښه() {
        إذا (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ساكن ثابت std::string& indentSpaces(صحيح عدد);
    // Add escaped characters to strings
    ساكن string quoteNameControls(ثابت string& namein, Language lang = LA_C) VL_خالص;
    ساكن bool token_سمون(ثابت حرف* cp, ثابت حرف* cmp);
    ساكن bool token_Not_پیل(ثابت حرف* cp);  // Import/export meaning no endfunction
    ساكن bool token_پیل(ثابت حرف* cp);
    ساكن bool token_پای_ټکی(ثابت حرف* cp);

    // CALLBACKS - MUST OVERRIDE
    مجازي فراغ putcOutput(حرف chr) = 0;
    مجازي فراغ putsOutput(ثابت حرف* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_بهر_File VL_NOT_FINAL : عام V3_بهر_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ساكن constexpr std::اندازه_t WRITE_بفر_اندازه_بایټونه = 128 * 1024;

    // MEMBERS
    ثابت std::string m_filename;
    FILE* m_fp = nullptr;
    std::اندازه_t m_کارول_شوی_بایټونه = 0;  // Number of bytes stored in m_bufferp
    std::اندازه_t m_written_بایټونه = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<حرف, WRITE_بفر_اندازه_بایټونه>> m_bufferp;  // Write buffer

عام:
    V3_بهر_File(ثابت string& filename, V3_بهر_Formatter::Language lang);
    V3_بهر_File(ثابت V3_بهر_File&) = delete;
    V3_بهر_File& چلوونکی=(ثابت V3_بهر_File&) = delete;
    V3_بهر_File(V3_بهر_File&&) = delete;
    V3_بهر_File& چلوونکی=(V3_بهر_File&&) = delete;
    ~V3_بهر_File() override;

    std::string filename() ثابت { أرجع m_filename; }

    فراغ putsForceIncs();

    فراغ stat_ریکارډ_Written() {
        write_بلاک();
        V3Stats::جمع_کړه_Stat_مجموعه(V3Stats::STAT_CPP_CHARS, m_written_بایټونه);
    }

خاص:
    فراغ write_بلاک() {
        إذا (VL_LIKELY(m_کارول_شوی_بایټونه > 0)) {
            fwrite(m_bufferp->معلومات(), m_کارول_شوی_بایټونه, 1, m_fp);
            m_written_بایټونه += m_کارول_شوی_بایټونه;
            m_کارول_شوی_بایټونه = 0;
        }
    }
    // CALLBACKS
    فراغ putcOutput(حرف chr) override {
        m_bufferp->at(m_کارول_شوی_بایټونه++) = chr;
        إذا (VL_UNLIKELY(m_کارول_شوی_بایټونه >= WRITE_بفر_اندازه_بایټونه)) write_بلاک();
    }
    فراغ putsOutput(ثابت حرف* str) override {
        std::اندازه_t len = مزي_اوږدوالی(str);
        std::اندازه_t شته_بایټونه = WRITE_بفر_اندازه_بایټونه - m_کارول_شوی_بایټونه;
        طالما (VL_UNLIKELY(len >= شته_بایټونه)) {
            std::حافظه_نقل(m_bufferp->معلومات() + m_کارول_شوی_بایټونه, str, شته_بایټونه);
            m_کارول_شوی_بایټونه = WRITE_بفر_اندازه_بایټونه;
            write_بلاک();
            str += شته_بایټونه;
            len -= شته_بایټونه;
            شته_بایټونه = WRITE_بفر_اندازه_بایټونه;
        }
        إذا (len > 0) {
            std::حافظه_نقل(m_bufferp->معلومات() + m_کارول_شوی_بایټونه, str, len);
            m_کارول_شوی_بایټونه += len;
        }
    }
};

class V3_بهر_CFile VL_NOT_FINAL : عام V3_بهر_File {
    صحيح m_guard = false;  // Created header guard
    صحيح m_شخصي;  // 1 = Most recently emitted private:, 2 = public:
عام:
    explicit V3_بهر_CFile(ثابت string& filename,
                        V3_بهر_Formatter::Language lang = V3_بهر_Formatter::LA_C)
        : V3_بهر_File{filename, lang} {
        reset_شخصي();
    }
    ~V3_بهر_CFile() override { stat_ریکارډ_Written(); }
    مجازي فراغ puts_سرلیک() { puts("// Verilated -*- C++ -*-\n"); }
    مجازي فراغ puts_Int_سر_Include() { putsForceIncs(); }
    مجازي فراغ putsGuard();
    مجازي فراغ puts_پای_ټکی_Guard() {
        إذا (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    فراغ reset_شخصي() { m_شخصي = 0; }
    فراغ puts_شخصي(bool سېټ_شخصي) {
        إذا (سېټ_شخصي && m_شخصي != 1) {
            puts("private:\n");
            m_شخصي = 1;
        } وإلا إذا (!سېټ_شخصي && m_شخصي != 2) {
            puts("public:\n");
            m_شخصي = 2;
        }
    }
};

class V3_بهر_Json_File final : عام V3_بهر_File {
    // CONSTANTS
    ساكن constexpr ثابت حرف* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
خاص:
    std::ډېری<حرف> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_مختاړی;  // Prefix emitted before each line in current scope
    std::ډېری<bool> m_تشه;  // Current scope is empty, no comma later

عام:
    explicit V3_بهر_Json_File(ثابت string& filename)
        : V3_بهر_File{filename, V3_بهر_Formatter::LA_JSON} {
        begin();
    }
    ~V3_بهر_Json_File() override {
        پای_ټکی();
        puts("\n");
    }
    مجازي فراغ puts_سرلیک() {}
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_بهر_Json_File& begin(ثابت std::string& name, حرف type = '{') {
        comma();
        puts(m_مختاړی + "\"" + name + "\": " + type + "\n");
        m_مختاړی += INDENT;
        m_scope.ټېل(type == '{' ? '}' : ']');
        إذا (!m_تشه.تشه()) m_تشه.سر() = false;
        m_تشه.ټېل(true);
        أرجع *this;
    }
    V3_بهر_Json_File& begin(حرف type = '{') {
        comma();
        puts(m_مختاړی + type + "\n");
        m_مختاړی += INDENT;
        m_scope.ټېل(type == '{' ? '}' : ']');
        إذا (!m_تشه.تشه()) m_تشه.سر() = false;
        m_تشه.ټېل(true);
        أرجع *this;
    }

    // Put a named value
    V3_بهر_Json_File& put(ثابت std::string& name, ثابت حرف* valuep) {
        أرجع putNamed(name, std::string{valuep}, true);
    }
    V3_بهر_Json_File& put(ثابت std::string& name, ثابت std::string& ارزښت) {
        أرجع putNamed(name, ارزښت, true);
    }
    V3_بهر_Json_File& put(ثابت std::string& name, bool ارزښت) {
        أرجع putNamed(name, ارزښت ? "true" : "false", false);
    }
    V3_بهر_Json_File& put(ثابت std::string& name, صحيح ارزښت) {
        أرجع putNamed(name, std::منزل_string(ارزښت), false);
    }

    // Put unnamed value
    V3_بهر_Json_File& put(ثابت std::string& ارزښت) { أرجع putNamed("", ارزښت, true); }
    V3_بهر_Json_File& put(bool ارزښت) { أرجع putNamed("", ارزښت ? "true" : "false", false); }
    V3_بهر_Json_File& put(صحيح ارزښت) { أرجع putNamed("", std::منزل_string(ارزښت), false); }

    V3_بهر_Json_File& putList(ثابت std::string& name, ثابت std::ویکټور<std::string>& list) {
        إذا (list.تشه()) أرجع *this;
        begin(name, '[');
        لكل (ثابت std::string& str : list) put(str);
        أرجع پای_ټکی();
    }

    V3_بهر_Json_File& پای_ټکی() {
        UASSERT(m_مختاړی.اوږدتیا() >= مزي_اوږدوالی(INDENT), "prefix underflow");
        m_مختاړی.erase(m_مختاړی.پای_ټکی() - مزي_اوږدوالی(INDENT), m_مختاړی.پای_ټکی());
        UASSERT(!m_scope.تشه(), "end() without begin()");
        UASSERT(!m_تشه.تشه(), "end() without begin()");
        puts("\n" + m_مختاړی + m_scope.سر());
        m_scope.راوباسه();
        m_تشه.راوباسه();
        أرجع *this;
    }

    V3_بهر_Json_File& چلوونکی+=(V3_بهر_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        أرجع *this;
    }

خاص:
    فراغ comma() {
        إذا (m_تشه.تشه()) أرجع;
        إذا (!m_تشه.سر()) puts(",\n");
        m_تشه.سر() = true;
    }
    V3_بهر_Json_File& putNamed(ثابت std::string& name, ثابت std::string& ارزښت, bool quoted) {
        comma();
        ثابت string ارزښت_Q
            = quoted ? "\""s + V3_بهر_Formatter::quoteNameControls(ارزښت) + "\"" : ارزښت;
        إذا (name.تشه()) {
            puts(m_مختاړی + ارزښت_Q);
        } وإلا {
            puts(m_مختاړی + "\"" + name + "\": " + ارزښت_Q);
        }
        m_تشه.سر() = false;
        أرجع *this;
    }
};

class V3_بهر_Mk_File final : عام V3_بهر_File {
عام:
    explicit V3_بهر_Mk_File(ثابت string& filename)
        : V3_بهر_File{filename, V3_بهر_Formatter::LA_MK} {}
    ~V3_بهر_Mk_File() override = افتراضي;
    مجازي فراغ puts_سرلیک() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    فراغ put_سېټ(ثابت string& var, ثابت string& ارزښت) {
        puts(VString::dot(var + " =", " ", ارزښت) + "\n");
    }
    // Put VARIABLE ?= VALUE
    فراغ put_سېټ_Q(ثابت string& var, ثابت string& ارزښت) {
        puts(VString::dot(var + " ?=", " ", ارزښت) + "\n");
    }
};

class V3_بهر_VFile final : عام V3_بهر_CFile {
عام:
    explicit V3_بهر_VFile(ثابت string& filename)
        : V3_بهر_CFile{filename, V3_بهر_Formatter::LA_VERILOG} {}
    ~V3_بهر_VFile() override = افتراضي;
    فراغ puts_سرلیک() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3_بهر_Xml_File final : عام V3_بهر_File {
عام:
    explicit V3_بهر_Xml_File(ثابت string& filename)
        : V3_بهر_File{filename, V3_بهر_Formatter::LA_XML} {
        بلاک_Indent(2);
    }
    ~V3_بهر_Xml_File() override = افتراضي;
    مجازي فراغ puts_سرلیک() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_بهر_بهیر VL_NOT_FINAL : عام V3_بهر_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_بهر_بهیر);
    VL_UNMOVABLE(V3_بهر_بهیر);

عام:
    V3_بهر_بهیر(std::ostream& ostream, V3_بهر_Formatter::Language lang);
    ~V3_بهر_بهیر() override = افتراضي;

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
    ساكن string ifNoProtect(ثابت string& دننه) VL_MT_خوندي;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ساكن string protect(ثابت string& old) VL_MT_خوندي { أرجع protectIf(old, true); }
    ساكن string protectIf(ثابت string& old, bool doIt = true) VL_MT_خوندي;
    // Rename words to a new encoded string
    ساكن string protect_ټکي_If(ثابت string& old, bool doIt = true) VL_MT_خوندي;
    // Write map of renames to output file
    ساكن فراغ writeMapFile(ثابت string& filename);
};

#د_که_پای  // Guard
