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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3FILE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3FILE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <array>
#ਅਨ੍ਤਰ੍ਭਾਵ <fstream>
#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <stack>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class Ast_ਗੰਢ;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
ਜਨਤਕ:
    ਸਥਿਰ std::ifstream* new_ifstream(ਅਚਲ string& filename) {
        ਜੋੜੋ_Src_Depend(filename);
        ਵਾਪਸ new_ifstream_nodepend(filename);
    }
    ਸਥਿਰ std::ifstream* new_ifstream_nodepend(ਅਚਲ string& filename) VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ new std::ifstream{filename.c_str()};
    }
    ਸਥਿਰ std::ofstream* new_ofstream(ਅਚਲ string& filename, bool append = false) {
        ਜੋੜੋ_Tgt_Depend(filename);
        ਵਾਪਸ new_ofstream_nodepend(filename, append);
    }
    ਸਥਿਰ std::ofstream* new_ofstream_nodepend(ਅਚਲ string& filename, bool append = false) {
        createMakeDirFor(filename);
        ਜੇ (append) ਵਾਪਸ new std::ofstream{filename.c_str(), std::ios::app};
        ਵਾਪਸ new std::ofstream{filename.c_str()};
    }
    ਸਥਿਰ FILE* new_fopen_w(ਅਚਲ string& filename) {
        createMakeDirFor(filename);
        ਜੋੜੋ_Tgt_Depend(filename);
        ਵਾਪਸ fopen(filename.c_str(), "w");
    }

    // Dependencies
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Src_Depend(ਅਚਲ string& filename) VL_MT_ਸੁਰੱਖਿਅਤ;
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Tgt_Depend(ਅਚਲ string& filename) VL_MT_ਸੁਰੱਖਿਅਤ;
    ਸਥਿਰ ਖਾਲੀ writeDepend(ਅਚਲ string& filename);
    ਸਥਿਰ std::ਸਦਿਸ਼<string> getAllDeps();
    ਸਥਿਰ ਖਾਲੀ writeTimes(ਅਚਲ string& filename, ਅਚਲ string& cmdline_ਵਿੱਚ);
    ਸਥਿਰ bool checkTimes(ਅਚਲ string& filename, ਅਚਲ string& cmdline_ਵਿੱਚ);

    // Directory utilities
    ਸਥਿਰ ਖਾਲੀ createMakeDirFor(ਅਚਲ string& filename);
    ਸਥਿਰ ਖਾਲੀ createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
ਜਨਤਕ:
    // TYPES
    using StrList = std::list<std::string>;

ਨਿਜੀ:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

ਜਨਤਕ:
    explicit VInFilter(ਅਚਲ string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ਅਚਲ string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3_ਬਾਹਰਲਾ_Formatter VL_NOT_FINAL {
    // TYPES
ਜਨਤਕ:
    ਗਿਣਤੀ AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    ਗਿਣਤੀ Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

ਨਿਜੀ:
    // MEMBERS
    ਅਚਲ Language m_lang;  // Indenting Verilog code
    ਪੂਰਨ m_ਖੰਡ_Indent;  // Characters per block indent
    ਪੂਰਨ m_comma_ਚੌੜਾਈ;  // Width after which to break at ,'s
    ਪੂਰਨ m_lineno = 1;
    ਪੂਰਨ m_ਕਾਲਮ = 0;
    ਪੂਰਨ m_nobreak = false;  // Basic operator or begin paren, don't break next
    ਪੂਰਨ m_ਸਰੋਤ_ਆਖਰੀ_Lineno = 0;
    ਪੂਰਨ m_ਸਰੋਤ_ਆਖਰੀ_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_ਵਿੱਚ_String_Literal = false;  // Inside a quote
    bool m_ਵਿੱਚ_Backslash = false;  // Inside a backslash
    ਪੂਰਨ m_indentLevel = 0;  // Current {} indentation
    std::ਢੇਰ<ਪੂਰਨ> m_parenVec;  // Stack of columns where last ( was
    ਪੂਰਨ m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    ਪੂਰਨ ਸਿਰਾ_ਪੱਧਰ(ਅਚਲ ਅੱਖਰ* strg);
    ਖਾਲੀ putcNoTracking(ਅੱਖਰ chr);

ਜਨਤਕ:
    explicit V3_ਬਾਹਰਲਾ_Formatter(Language lang);
    ਆਭਾਸੀ ~V3_ਬਾਹਰਲਾ_Formatter() = ਮੂਲ;
    // ACCESSORS
    ਖਾਲੀ ਖੰਡ_Indent(ਪੂਰਨ flag) { m_ਖੰਡ_Indent = flag; }
    // METHODS
    ਖਾਲੀ ਲਿਖੋ(ਅਚਲ ਅੱਖਰ* fmt...) VL_ATTR_PRINTF(2);
    ਖਾਲੀ puts(ਅਚਲ ਅੱਖਰ* strg) { putns(nullptr, strg); }
    ਖਾਲੀ puts(ਅਚਲ string& strg) { putns(nullptr, strg); }
    ਖਾਲੀ putns(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ ਅੱਖਰ* strg);
    ਖਾਲੀ putns(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& strg) { putns(nodep, strg.c_str()); }
    ਖਾਲੀ putsNoTracking(ਅਚਲ string& strg);
    ਖਾਲੀ putsQuoted(ਅਚਲ string& strg);
    ਖਾਲੀ putBreak();  // Print linebreak if line is too wide
    ਖਾਲੀ put_Break_ਵਿਅੰਜਕ();  // Print linebreak in expression if line is too wide
    ਖਾਲੀ putbs(ਅਚਲ ਅੱਖਰ* strg) {
        put_Break_ਵਿਅੰਜਕ();
        putns(nullptr, strg);
    }
    ਖਾਲੀ putbs(ਅਚਲ string& strg) {
        put_Break_ਵਿਅੰਜਕ();
        putns(nullptr, strg);
    }
    ਖਾਲੀ putnbs(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& strg) {
        put_Break_ਵਿਅੰਜਕ();
        putns(nodep, strg);
    }
    bool exceeded_ਚੌੜਾਈ() ਅਚਲ { ਵਾਪਸ m_ਕਾਲਮ > m_comma_ਚੌੜਾਈ; }
    ਖਾਲੀ indentInc() { m_indentLevel += m_ਖੰਡ_Indent; }
    ਖਾਲੀ indentDec() {
        m_indentLevel -= m_ਖੰਡ_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    ਖਾਲੀ ਖੰਡ_Inc() { m_parenVec.ਧੱਕੋ(m_indentLevel + m_ਖੰਡ_Indent); }
    ਖਾਲੀ ਖੰਡ_Dec() {
        ਜੇ (!m_parenVec.ਖਾਲੀ_ਥਾਵਾਂ()) m_parenVec.ਕੱਢੋ();
    }
    ਖਾਲੀ ensure_New_ਸਤਰ() {
        ਜੇ (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ਸਥਿਰ ਅਚਲ std::string& indentSpaces(ਪੂਰਨ ਅੰਕ);
    // Add escaped characters to strings
    ਸਥਿਰ string quoteNameControls(ਅਚਲ string& namein, Language lang = LA_C) VL_ਸ਼ੁੱਧ;
    ਸਥਿਰ bool token_ਮੇਲ(ਅਚਲ ਅੱਖਰ* cp, ਅਚਲ ਅੱਖਰ* cmp);
    ਸਥਿਰ bool token_Not_ਸ਼ੁਰੂ(ਅਚਲ ਅੱਖਰ* cp);  // Import/export meaning no endfunction
    ਸਥਿਰ bool token_ਸ਼ੁਰੂ(ਅਚਲ ਅੱਖਰ* cp);
    ਸਥਿਰ bool token_ਸਿਰਾ(ਅਚਲ ਅੱਖਰ* cp);

    // CALLBACKS - MUST OVERRIDE
    ਆਭਾਸੀ ਖਾਲੀ putcOutput(ਅੱਖਰ chr) = 0;
    ਆਭਾਸੀ ਖਾਲੀ putsOutput(ਅਚਲ ਅੱਖਰ* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_ਬਾਹਰਲਾ_File VL_NOT_FINAL : ਜਨਤਕ V3_ਬਾਹਰਲਾ_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ਸਥਿਰ constexpr std::ਮਾਪ_t WRITE_ਬਫਰ_ਮਾਪ_ਬਾਈਟਾਂ = 128 * 1024;

    // MEMBERS
    ਅਚਲ std::string m_filename;
    FILE* m_fp = nullptr;
    std::ਮਾਪ_t m_ਵਰਤਿਆ_ਬਾਈਟਾਂ = 0;  // Number of bytes stored in m_bufferp
    std::ਮਾਪ_t m_written_ਬਾਈਟਾਂ = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<ਅੱਖਰ, WRITE_ਬਫਰ_ਮਾਪ_ਬਾਈਟਾਂ>> m_bufferp;  // Write buffer

ਜਨਤਕ:
    V3_ਬਾਹਰਲਾ_File(ਅਚਲ string& filename, V3_ਬਾਹਰਲਾ_Formatter::Language lang);
    V3_ਬਾਹਰਲਾ_File(ਅਚਲ V3_ਬਾਹਰਲਾ_File&) = delete;
    V3_ਬਾਹਰਲਾ_File& ਸੰਚਾਲਕ=(ਅਚਲ V3_ਬਾਹਰਲਾ_File&) = delete;
    V3_ਬਾਹਰਲਾ_File(V3_ਬਾਹਰਲਾ_File&&) = delete;
    V3_ਬਾਹਰਲਾ_File& ਸੰਚਾਲਕ=(V3_ਬਾਹਰਲਾ_File&&) = delete;
    ~V3_ਬਾਹਰਲਾ_File() override;

    std::string filename() ਅਚਲ { ਵਾਪਸ m_filename; }

    ਖਾਲੀ putsForceIncs();

    ਖਾਲੀ stat_ਰਿਕਾਰਡ_Written() {
        write_ਖੰਡ();
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ(V3Stats::STAT_CPP_CHARS, m_written_ਬਾਈਟਾਂ);
    }

ਨਿਜੀ:
    ਖਾਲੀ write_ਖੰਡ() {
        ਜੇ (VL_LIKELY(m_ਵਰਤਿਆ_ਬਾਈਟਾਂ > 0)) {
            fwrite(m_bufferp->ਅੰਕੜੇ(), m_ਵਰਤਿਆ_ਬਾਈਟਾਂ, 1, m_fp);
            m_written_ਬਾਈਟਾਂ += m_ਵਰਤਿਆ_ਬਾਈਟਾਂ;
            m_ਵਰਤਿਆ_ਬਾਈਟਾਂ = 0;
        }
    }
    // CALLBACKS
    ਖਾਲੀ putcOutput(ਅੱਖਰ chr) override {
        m_bufferp->at(m_ਵਰਤਿਆ_ਬਾਈਟਾਂ++) = chr;
        ਜੇ (VL_UNLIKELY(m_ਵਰਤਿਆ_ਬਾਈਟਾਂ >= WRITE_ਬਫਰ_ਮਾਪ_ਬਾਈਟਾਂ)) write_ਖੰਡ();
    }
    ਖਾਲੀ putsOutput(ਅਚਲ ਅੱਖਰ* str) override {
        std::ਮਾਪ_t len = ਲੜੀ_ਲੰਬਾਈ(str);
        std::ਮਾਪ_t ਉਪਲਬਧ_ਬਾਈਟਾਂ = WRITE_ਬਫਰ_ਮਾਪ_ਬਾਈਟਾਂ - m_ਵਰਤਿਆ_ਬਾਈਟਾਂ;
        ਜਦੋਂ (VL_UNLIKELY(len >= ਉਪਲਬਧ_ਬਾਈਟਾਂ)) {
            std::ਸਮ੍ਰਿਤੀ_ਨਕਲ(m_bufferp->ਅੰਕੜੇ() + m_ਵਰਤਿਆ_ਬਾਈਟਾਂ, str, ਉਪਲਬਧ_ਬਾਈਟਾਂ);
            m_ਵਰਤਿਆ_ਬਾਈਟਾਂ = WRITE_ਬਫਰ_ਮਾਪ_ਬਾਈਟਾਂ;
            write_ਖੰਡ();
            str += ਉਪਲਬਧ_ਬਾਈਟਾਂ;
            len -= ਉਪਲਬਧ_ਬਾਈਟਾਂ;
            ਉਪਲਬਧ_ਬਾਈਟਾਂ = WRITE_ਬਫਰ_ਮਾਪ_ਬਾਈਟਾਂ;
        }
        ਜੇ (len > 0) {
            std::ਸਮ੍ਰਿਤੀ_ਨਕਲ(m_bufferp->ਅੰਕੜੇ() + m_ਵਰਤਿਆ_ਬਾਈਟਾਂ, str, len);
            m_ਵਰਤਿਆ_ਬਾਈਟਾਂ += len;
        }
    }
};

class V3_ਬਾਹਰਲਾ_CFile VL_NOT_FINAL : ਜਨਤਕ V3_ਬਾਹਰਲਾ_File {
    ਪੂਰਨ m_guard = false;  // Created header guard
    ਪੂਰਨ m_ਨਿੱਜੀ;  // 1 = Most recently emitted private:, 2 = public:
ਜਨਤਕ:
    explicit V3_ਬਾਹਰਲਾ_CFile(ਅਚਲ string& filename,
                        V3_ਬਾਹਰਲਾ_Formatter::Language lang = V3_ਬਾਹਰਲਾ_Formatter::LA_C)
        : V3_ਬਾਹਰਲਾ_File{filename, lang} {
        reset_ਨਿੱਜੀ();
    }
    ~V3_ਬਾਹਰਲਾ_CFile() override { stat_ਰਿਕਾਰਡ_Written(); }
    ਆਭਾਸੀ ਖਾਲੀ puts_ਸਿਰਲੇਖ() { puts("// Verilated -*- C++ -*-\n"); }
    ਆਭਾਸੀ ਖਾਲੀ puts_Int_ਸਿਖਰ_Include() { putsForceIncs(); }
    ਆਭਾਸੀ ਖਾਲੀ putsGuard();
    ਆਭਾਸੀ ਖਾਲੀ puts_ਸਿਰਾ_Guard() {
        ਜੇ (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    ਖਾਲੀ reset_ਨਿੱਜੀ() { m_ਨਿੱਜੀ = 0; }
    ਖਾਲੀ puts_ਨਿੱਜੀ(bool ਸੈੱਟ_ਨਿੱਜੀ) {
        ਜੇ (ਸੈੱਟ_ਨਿੱਜੀ && m_ਨਿੱਜੀ != 1) {
            puts("private:\n");
            m_ਨਿੱਜੀ = 1;
        } ਨਹੀਂਤਾਂ ਜੇ (!ਸੈੱਟ_ਨਿੱਜੀ && m_ਨਿੱਜੀ != 2) {
            puts("public:\n");
            m_ਨਿੱਜੀ = 2;
        }
    }
};

class V3_ਬਾਹਰਲਾ_Json_File final : ਜਨਤਕ V3_ਬਾਹਰਲਾ_File {
    // CONSTANTS
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
ਨਿਜੀ:
    std::ਢੇਰ<ਅੱਖਰ> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_ਅਗੇਤਰ;  // Prefix emitted before each line in current scope
    std::ਢੇਰ<bool> m_ਖਾਲੀ_ਥਾਵਾਂ;  // Current scope is empty, no comma later

ਜਨਤਕ:
    explicit V3_ਬਾਹਰਲਾ_Json_File(ਅਚਲ string& filename)
        : V3_ਬਾਹਰਲਾ_File{filename, V3_ਬਾਹਰਲਾ_Formatter::LA_JSON} {
        begin();
    }
    ~V3_ਬਾਹਰਲਾ_Json_File() override {
        ਸਿਰਾ();
        puts("\n");
    }
    ਆਭਾਸੀ ਖਾਲੀ puts_ਸਿਰਲੇਖ() {}
    // cppcheck-suppress duplInheritedMember
    ਖਾਲੀ puts(ਅਚਲ ਅੱਖਰ* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ਖਾਲੀ puts(ਅਚਲ string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_ਬਾਹਰਲਾ_Json_File& begin(ਅਚਲ std::string& name, ਅੱਖਰ type = '{') {
        comma();
        puts(m_ਅਗੇਤਰ + "\"" + name + "\": " + type + "\n");
        m_ਅਗੇਤਰ += INDENT;
        m_scope.ਧੱਕੋ(type == '{' ? '}' : ']');
        ਜੇ (!m_ਖਾਲੀ_ਥਾਵਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) m_ਖਾਲੀ_ਥਾਵਾਂ.ਸਿਖਰ() = false;
        m_ਖਾਲੀ_ਥਾਵਾਂ.ਧੱਕੋ(true);
        ਵਾਪਸ *this;
    }
    V3_ਬਾਹਰਲਾ_Json_File& begin(ਅੱਖਰ type = '{') {
        comma();
        puts(m_ਅਗੇਤਰ + type + "\n");
        m_ਅਗੇਤਰ += INDENT;
        m_scope.ਧੱਕੋ(type == '{' ? '}' : ']');
        ਜੇ (!m_ਖਾਲੀ_ਥਾਵਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) m_ਖਾਲੀ_ਥਾਵਾਂ.ਸਿਖਰ() = false;
        m_ਖਾਲੀ_ਥਾਵਾਂ.ਧੱਕੋ(true);
        ਵਾਪਸ *this;
    }

    // Put a named value
    V3_ਬਾਹਰਲਾ_Json_File& put(ਅਚਲ std::string& name, ਅਚਲ ਅੱਖਰ* valuep) {
        ਵਾਪਸ putNamed(name, std::string{valuep}, true);
    }
    V3_ਬਾਹਰਲਾ_Json_File& put(ਅਚਲ std::string& name, ਅਚਲ std::string& ਮੁੱਲ) {
        ਵਾਪਸ putNamed(name, ਮੁੱਲ, true);
    }
    V3_ਬਾਹਰਲਾ_Json_File& put(ਅਚਲ std::string& name, bool ਮੁੱਲ) {
        ਵਾਪਸ putNamed(name, ਮੁੱਲ ? "true" : "false", false);
    }
    V3_ਬਾਹਰਲਾ_Json_File& put(ਅਚਲ std::string& name, ਪੂਰਨ ਮੁੱਲ) {
        ਵਾਪਸ putNamed(name, std::ਮੰਜ਼ਿਲ_string(ਮੁੱਲ), false);
    }

    // Put unnamed value
    V3_ਬਾਹਰਲਾ_Json_File& put(ਅਚਲ std::string& ਮੁੱਲ) { ਵਾਪਸ putNamed("", ਮੁੱਲ, true); }
    V3_ਬਾਹਰਲਾ_Json_File& put(bool ਮੁੱਲ) { ਵਾਪਸ putNamed("", ਮੁੱਲ ? "true" : "false", false); }
    V3_ਬਾਹਰਲਾ_Json_File& put(ਪੂਰਨ ਮੁੱਲ) { ਵਾਪਸ putNamed("", std::ਮੰਜ਼ਿਲ_string(ਮੁੱਲ), false); }

    V3_ਬਾਹਰਲਾ_Json_File& putList(ਅਚਲ std::string& name, ਅਚਲ std::ਸਦਿਸ਼<std::string>& list) {
        ਜੇ (list.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ *this;
        begin(name, '[');
        ਲਈ (ਅਚਲ std::string& str : list) put(str);
        ਵਾਪਸ ਸਿਰਾ();
    }

    V3_ਬਾਹਰਲਾ_Json_File& ਸਿਰਾ() {
        UASSERT(m_ਅਗੇਤਰ.ਲੰਮਾਈ() >= ਲੜੀ_ਲੰਬਾਈ(INDENT), "prefix underflow");
        m_ਅਗੇਤਰ.erase(m_ਅਗੇਤਰ.ਸਿਰਾ() - ਲੜੀ_ਲੰਬਾਈ(INDENT), m_ਅਗੇਤਰ.ਸਿਰਾ());
        UASSERT(!m_scope.ਖਾਲੀ_ਥਾਵਾਂ(), "end() without begin()");
        UASSERT(!m_ਖਾਲੀ_ਥਾਵਾਂ.ਖਾਲੀ_ਥਾਵਾਂ(), "end() without begin()");
        puts("\n" + m_ਅਗੇਤਰ + m_scope.ਸਿਖਰ());
        m_scope.ਕੱਢੋ();
        m_ਖਾਲੀ_ਥਾਵਾਂ.ਕੱਢੋ();
        ਵਾਪਸ *this;
    }

    V3_ਬਾਹਰਲਾ_Json_File& ਸੰਚਾਲਕ+=(V3_ਬਾਹਰਲਾ_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        ਵਾਪਸ *this;
    }

ਨਿਜੀ:
    ਖਾਲੀ comma() {
        ਜੇ (m_ਖਾਲੀ_ਥਾਵਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;
        ਜੇ (!m_ਖਾਲੀ_ਥਾਵਾਂ.ਸਿਖਰ()) puts(",\n");
        m_ਖਾਲੀ_ਥਾਵਾਂ.ਸਿਖਰ() = true;
    }
    V3_ਬਾਹਰਲਾ_Json_File& putNamed(ਅਚਲ std::string& name, ਅਚਲ std::string& ਮੁੱਲ, bool quoted) {
        comma();
        ਅਚਲ string ਮੁੱਲ_Q
            = quoted ? "\""s + V3_ਬਾਹਰਲਾ_Formatter::quoteNameControls(ਮੁੱਲ) + "\"" : ਮੁੱਲ;
        ਜੇ (name.ਖਾਲੀ_ਥਾਵਾਂ()) {
            puts(m_ਅਗੇਤਰ + ਮੁੱਲ_Q);
        } ਨਹੀਂਤਾਂ {
            puts(m_ਅਗੇਤਰ + "\"" + name + "\": " + ਮੁੱਲ_Q);
        }
        m_ਖਾਲੀ_ਥਾਵਾਂ.ਸਿਖਰ() = false;
        ਵਾਪਸ *this;
    }
};

class V3_ਬਾਹਰਲਾ_Mk_File final : ਜਨਤਕ V3_ਬਾਹਰਲਾ_File {
ਜਨਤਕ:
    explicit V3_ਬਾਹਰਲਾ_Mk_File(ਅਚਲ string& filename)
        : V3_ਬਾਹਰਲਾ_File{filename, V3_ਬਾਹਰਲਾ_Formatter::LA_MK} {}
    ~V3_ਬਾਹਰਲਾ_Mk_File() override = ਮੂਲ;
    ਆਭਾਸੀ ਖਾਲੀ puts_ਸਿਰਲੇਖ() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    ਖਾਲੀ puts(ਅਚਲ ਅੱਖਰ* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ਖਾਲੀ puts(ਅਚਲ string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    ਖਾਲੀ put_ਸੈੱਟ(ਅਚਲ string& var, ਅਚਲ string& ਮੁੱਲ) {
        puts(VString::dot(var + " =", " ", ਮੁੱਲ) + "\n");
    }
    // Put VARIABLE ?= VALUE
    ਖਾਲੀ put_ਸੈੱਟ_Q(ਅਚਲ string& var, ਅਚਲ string& ਮੁੱਲ) {
        puts(VString::dot(var + " ?=", " ", ਮੁੱਲ) + "\n");
    }
};

class V3_ਬਾਹਰਲਾ_VFile final : ਜਨਤਕ V3_ਬਾਹਰਲਾ_CFile {
ਜਨਤਕ:
    explicit V3_ਬਾਹਰਲਾ_VFile(ਅਚਲ string& filename)
        : V3_ਬਾਹਰਲਾ_CFile{filename, V3_ਬਾਹਰਲਾ_Formatter::LA_VERILOG} {}
    ~V3_ਬਾਹਰਲਾ_VFile() override = ਮੂਲ;
    ਖਾਲੀ puts_ਸਿਰਲੇਖ() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3_ਬਾਹਰਲਾ_Xml_File final : ਜਨਤਕ V3_ਬਾਹਰਲਾ_File {
ਜਨਤਕ:
    explicit V3_ਬਾਹਰਲਾ_Xml_File(ਅਚਲ string& filename)
        : V3_ਬਾਹਰਲਾ_File{filename, V3_ਬਾਹਰਲਾ_Formatter::LA_XML} {
        ਖੰਡ_Indent(2);
    }
    ~V3_ਬਾਹਰਲਾ_Xml_File() override = ਮੂਲ;
    ਆਭਾਸੀ ਖਾਲੀ puts_ਸਿਰਲੇਖ() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_ਬਾਹਰਲਾ_ਧਾਰਾ VL_NOT_FINAL : ਜਨਤਕ V3_ਬਾਹਰਲਾ_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_ਬਾਹਰਲਾ_ਧਾਰਾ);
    VL_UNMOVABLE(V3_ਬਾਹਰਲਾ_ਧਾਰਾ);

ਜਨਤਕ:
    V3_ਬਾਹਰਲਾ_ਧਾਰਾ(std::ostream& ostream, V3_ਬਾਹਰਲਾ_Formatter::Language lang);
    ~V3_ਬਾਹਰਲਾ_ਧਾਰਾ() override = ਮੂਲ;

    ਖਾਲੀ putcOutput(ਅੱਖਰ chr) override { m_ostream << chr; };
    ਖਾਲੀ putsOutput(ਅਚਲ ਅੱਖਰ* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
ਜਨਤਕ:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    ਸਥਿਰ string ifNoProtect(ਅਚਲ string& ਵਿੱਚ_2) VL_MT_ਸੁਰੱਖਿਅਤ;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ਸਥਿਰ string protect(ਅਚਲ string& old) VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ protectIf(old, true); }
    ਸਥਿਰ string protectIf(ਅਚਲ string& old, bool doIt = true) VL_MT_ਸੁਰੱਖਿਅਤ;
    // Rename words to a new encoded string
    ਸਥਿਰ string protect_ਸ਼ਬਦ_If(ਅਚਲ string& old, bool doIt = true) VL_MT_ਸੁਰੱਖਿਅਤ;
    // Write map of renames to output file
    ਸਥਿਰ ਖਾਲੀ writeMapFile(ਅਚਲ string& filename);
};

#ਜੇ_ਅੰਤ  // Guard
