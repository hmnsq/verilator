// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Ast node structures
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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Broken.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <iomanip>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_ਗੰਢ::s_edit_Cnt_ਆਖਰੀ = 0;
uint64_t Ast_ਗੰਢ::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
ਪੂਰਨ Ast_ਗੰਢ::s_cloneCntGbl = 0;
uint32_t VNUser1_ਵਿੱਚ_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2_ਵਿੱਚ_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3_ਵਿੱਚ_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4_ਵਿੱਚ_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent

ਤਾਰਕਿਕ VNUser1_ਵਿੱਚ_Use::s_userBusy = ਝੂਠ;
ਤਾਰਕਿਕ VNUser2_ਵਿੱਚ_Use::s_userBusy = ਝੂਠ;
ਤਾਰਕਿਕ VNUser3_ਵਿੱਚ_Use::s_userBusy = ਝੂਠ;
ਤਾਰਕਿਕ VNUser4_ਵਿੱਚ_Use::s_userBusy = ਝੂਠ;

ਪੂਰਨ Ast_ਗੰਢ_DType::s_unique_ਅੰਕ = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::array_ਵਿਧੀ(ਅਚਲ string& name) {
    ਲਈ (ਪੂਰਨ i = 0; i < _ENUM_MAX; ++i) {
        ਅਚਲ VCMethod ਵਿਧੀ{i};
        ਜੇ (name == ਵਿਧੀ.ascii()) ਵਾਪਸ ਵਿਧੀ;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    ਵਾਪਸ VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ਅਚਲ ਖਾਲੀ*)) ਅਚਲ {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_USER_TYPE_CHECKS
    ਜੇ (ਅਚਲ uint64_t* ਅਚਲ uip = std::get_if<uint64_t>(&m_u)) ਵਾਪਸ "#"s + cvt_ਮੰਜ਼ਿਲ_Str(*uip);
    ਜੇ (ਖਾਲੀ* ਅਚਲ* ਅਚਲ upp = std::get_if<ਖਾਲੀ*>(&m_u)) ਵਾਪਸ fmtAddrp(*upp);
    ਵਾਪਸ "";
#ਨਹੀਂਤਾਂ
    // Dumps void* representation
    ਜੇ (!m_u.ਉੱਪਰ) ਵਾਪਸ "";
    ਵਾਪਸ fmtAddrp(m_u.ਉੱਪਰ);
#ਜੇ_ਅੰਤ
}

//######################################################################
// VNType

ਅਚਲ VNTypeInfo VNType::s_type_Info_ਸਾਰਣੀ[VNType::ਅੰਕ_TYPES()] = {
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& ਸੰਚਾਲਕ<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

ਤਾਰਕਿਕ VFwdType::is_ਗੰਢ_Compatible(ਅਚਲ Ast_ਗੰਢ* nodep) ਅਚਲ {
    ਅਚਲ Ast_ਗੰਢ* defp = nodep;
    ਜੇ (ਅਚਲ AstTypedef* ਅਚਲ adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ adefp = VN_CAST(defp, ਗੰਢ_DType))
        defp = adefp->skip_Ref_ਮੰਜ਼ਿਲ_Non_Refp();
    ਚੋਣ (m_e) {
    ਹਾਲਤ VFwdType::NONE: ਵਾਪਸ ਸੱਚ; ਰੋਕੋ;
    ਹਾਲਤ VFwdType::ENUM: ਵਾਪਸ VN_IS(defp, EnumDType); ਰੋਕੋ;
    ਹਾਲਤ VFwdType::STRUCT: ਵਾਪਸ VN_IS(defp, StructDType); ਰੋਕੋ;
    ਹਾਲਤ VFwdType::UNION: ਵਾਪਸ VN_IS(defp, UnionDType); ਰੋਕੋ;
    ਹਾਲਤ VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    ਹਾਲਤ VFwdType::CLASS: ਵਾਪਸ VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); ਰੋਕੋ;
    ਹਾਲਤ VFwdType::GENERIC_INTERFACE: ਵਾਪਸ VN_IS(defp, IfaceRefDType); ਰੋਕੋ;
    ਮੂਲ: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    ਵਾਪਸ ਝੂਠ;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ਅਚਲ std::shared_ptr<ਅਚਲ string> VSelf_ਸੂਚਕ_ਪਾਠ::s_emptyp = std::make_shared<string>("");
ਅਚਲ std::shared_ptr<ਅਚਲ string> VSelf_ਸੂਚਕ_ਪਾਠ::s_thisp = std::make_shared<string>("this");

string VSelf_ਸੂਚਕ_ਪਾਠ::replaceThis(ਤਾਰਕਿਕ useSelfForThis, ਅਚਲ string& ਪਾਠ) {
    ਵਾਪਸ useSelfForThis ? VString::replace_ਸ਼ਬਦ(ਪਾਠ, "this", "vlSelf") : ਪਾਠ;
}

string VSelf_ਸੂਚਕ_ਪਾਠ::protect(ਤਾਰਕਿਕ useSelfForThis, ਤਾਰਕਿਕ protect) ਅਚਲ {
    ਵਾਪਸ VIdProtect::protect_ਸ਼ਬਦ_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_ਗੰਢ::Ast_ਗੰਢ(VNType t, File_ਸਤਰ* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = ਇਹ;  // When made, we're a list of only a single element
    //ਗੁਣ
    m_flags.did_ਚੌੜਾਈ = ਝੂਠ;
    m_flags.doing_ਚੌੜਾਈ = ਝੂਠ;
    m_flags.protect = ਸੱਚ;
    m_flags.ਅਣਵਰਤਿਆ = 0;  // Initializing this avoids a read-modify-write on construction
    edit_ਗਿਣਤੀ_Inc();
}

Ast_ਗੰਢ* Ast_ਗੰਢ::abovep() ਅਚਲ {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || ਪਹਿਲਾ_Abovep(), ਇਹ, "abovep() not allowed when in midlist");
    ਅਚਲ Ast_ਗੰਢ* ਅਚਲ firstp = ਪਹਿਲਾ_Abovep() ? ਇਹ : m_headtailp;
    ਵਾਪਸ firstp->backp();
}
Ast_ਗੰਢ* Ast_ਗੰਢ::aboveLoopp() ਅਚਲ {
    // Returns parent node. Avoid using this, may have performance issues.
    ਅਚਲ Ast_ਗੰਢ* nodep = ਇਹ;
    // Backwards over peers (versus parents)
    ਜਦੋਂ (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    ਵਾਪਸ nodep->backp();
}

string Ast_ਗੰਢ::encodeName(ਅਚਲ string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string ਬਾਹਰਲਾ;
    ਬਾਹਰਲਾ.reserve(namein.ਮਾਪ());
    ਲਈ (ਸਵੈਚਾਲਿਤ pos = namein.begin(); pos != namein.ਸਿਰਾ(); ++pos) {
        ਜੇ ((pos == namein.begin()) ? std::ਅੱਖਰ_ਹੈ(pos[0])  // digits can't lead identifiers
                                    : std::ਅੱਖਰ_ਜਾਂ_ਅੰਕ(pos[0])) {
            ਬਾਹਰਲਾ += pos[0];
        } ਨਹੀਂਤਾਂ ਜੇ (pos[0] == '_') {
            ਬਾਹਰਲਾ += pos[0];
            ਜੇ (pos + 1 == namein.ਸਿਰਾ()) ਰੋਕੋ;
            ਜੇ (pos[1] == '_') {
                ++pos;
                ਬਾਹਰਲਾ += "__05F";  // hex(_) = 0x5F
            }
        } ਨਹੀਂਤਾਂ {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            ਬਾਹਰਲਾ += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{ਬਾਹਰਲਾ};
    ਵਾਪਸ vname.hashedName();
}

string Ast_ਗੰਢ::encode_ਸੰਖਿਆ(int64_t ਅੰਕ) {
    ਜੇ (ਅੰਕ < 0) ਵਾਪਸ "__02D" + cvt_ਮੰਜ਼ਿਲ_Str(-ਅੰਕ);  // 2D=-
    ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Str(ਅੰਕ);
}

string Ast_ਗੰਢ::nameProtect() ਅਚਲ { ਵਾਪਸ VIdProtect::protectIf(name(), protect()); }
string Ast_ਗੰਢ::origNameProtect() ਅਚਲ { ਵਾਪਸ VIdProtect::protectIf(origName(), protect()); }

string Ast_ਗੰਢ::shortName() ਅਚਲ {
    string pretty = name();
    string::ਮਾਪ_type pos;
    ਜਦੋਂ ((pos = pretty.ਲੱਭੋ("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    ਵਾਪਸ pretty;
}

string Ast_ਗੰਢ::dedotName(ਅਚਲ string& namein) {
    string pretty = namein;
    string::ਮਾਪ_type pos;
    ਜਦੋਂ ((pos = pretty.ਲੱਭੋ("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    ਜੇ (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ਵਾਪਸ pretty;
}

string Ast_ਗੰਢ::nameNoArray(ਅਚਲ string& namein) {
    ਵਾਪਸ namein.substr(0, namein.ਲੱਭੋ("__BRA__"));
}

string Ast_ਗੰਢ::vcdName(ਅਚਲ string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::ਮਾਪ_type pos;
    ਜਦੋਂ ((pos = pretty.ਲੱਭੋ("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    ਜਦੋਂ ((pos = pretty.ਲੱਭੋ('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    ਵਾਪਸ prettyName(pretty);
}

string Ast_ਗੰਢ::prettyName(ਅਚਲ string& namein) VL_ਸ਼ੁੱਧ {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.ਲੰਮਾਈ());
    ਲਈ (ਅਚਲ ਅੱਖਰ* pos = namein.c_str(); *pos;) {
        ਜੇ (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            ਜਾਰੀ;
        }
        ਜੇ (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                ਜਾਰੀ;
            }
            ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                ਜਾਰੀ;
            }
            ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                ਜਾਰੀ;
            }
            ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                ਜਾਰੀ;
            }
            ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                ਜਾਰੀ;
            }
            ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                ਜਾਰੀ;
            }
            ਜੇ (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                ਅੱਖਰ ਮੁੱਲ = 0;
                ਮੁੱਲ += 16
                         * (std::ਅੰਕ_ਹੈ(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ਮੁੱਲ
                    += (std::ਅੰਕ_ਹੈ(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += ਮੁੱਲ;
                pos += 5;
                ਜਾਰੀ;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    ਜੇ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ਜੇ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ਵਾਪਸ pretty;
}

string Ast_ਗੰਢ::vpiName(ਅਚਲ string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ਅਚਲ ਮਾਪ_t ਵਿੱਥ = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.ਲੰਮਾਈ());
    ਤਾਰਕਿਕ ਵਿੱਚ_Escaped_Ident = ਝੂਠ;
    ਪੂਰਨ ਆਖਰੀ_Ident = 0;

    ਲਈ (ਅਚਲ ਅੱਖਰ* pos = namein.c_str() + ਵਿੱਥ; *pos;) {
        ਅੱਖਰ specialChar = 0;
        ਜੇ (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } ਨਹੀਂਤਾਂ ਜੇ (pos[0] == '_' && pos[1] == '_') {  // __
            ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } ਨਹੀਂਤਾਂ ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } ਨਹੀਂਤਾਂ ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } ਨਹੀਂਤਾਂ ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__PVT__", 7)) {
                pos += 7;
                ਜਾਰੀ;
            } ਨਹੀਂਤਾਂ ਜੇ (0 == std::ਲੜੀ_ਤੁਲਨਾ(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                ਅੱਖਰ ਮੁੱਲ = 0;
                ਮੁੱਲ += 16
                         * (std::ਅੰਕ_ਹੈ(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ਮੁੱਲ
                    += (std::ਅੰਕ_ਹੈ(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                ਜੇ (ਮੁੱਲ != '_') ਵਿੱਚ_Escaped_Ident = ਸੱਚ;

                pretty += ਮੁੱਲ;
                pos += 5;
                ਜਾਰੀ;
            }
        } ਨਹੀਂਤਾਂ ਜੇ (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        ਜੇ (specialChar) {
            ਜੇ (ਵਿੱਚ_Escaped_Ident && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.ਪਾਓ(ਆਖਰੀ_Ident, "\\");
                ਵਿੱਚ_Escaped_Ident = ਝੂਠ;
            }

            pretty += specialChar;

            ਜੇ (specialChar == ']' || specialChar == '.') {
                ਆਖਰੀ_Ident = pretty.ਲੰਮਾਈ();
                ਵਿੱਚ_Escaped_Ident = ਝੂਠ;
            }
        } ਨਹੀਂਤਾਂ {
            pretty += pos[0];
            ++pos;
        }
    }
    ਜੇ (ਵਿੱਚ_Escaped_Ident) {
        pretty += " ";
        pretty.ਪਾਓ(ਆਖਰੀ_Ident, "\\");
    }
    ਜੇ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ਜੇ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ਵਾਪਸ pretty;
}

string Ast_ਗੰਢ::prettyTypeName() ਅਚਲ {
    ਜੇ (name() == "") ਵਾਪਸ typeName();
    ਵਾਪਸ std::string{typeName()} + " '" + pretty_Name_ਸੁਨੇਹਾ() + "'";
}

//######################################################################
// Insertion

ਖਾਲੀ Ast_ਗੰਢ::debug_ਰੁੱਖ_Change(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ ਅੱਖਰ* ਅਗੇਤਰ, ਪੂਰਨ lineno, ਤਾਰਕਿਕ ਅਗਲਾ) {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG
// Called on all major tree changers.
// Only for use for those really nasty bugs relating to internals
// Note this may be null.
// if (debug() && nodep) cout << "-treeChange: V3Ast.cpp:" << lineno
//          << " Tree Change for " << prefix << ": "
//          << cvtToHex(nodep) << " <e" << AstNode::s_editCntGbl << ">"
//          << "m_iterpp=" << (void*)nodep->m_iterpp << endl;
// if (debug()) {
//  cout<<"-treeChange: V3Ast.cpp:"<<lineno<<" Tree Change for "<<prefix<<endl;
//  // Commenting out the section below may crash, as the tree state
//  // between edits is not always consistent for printing
//  cout<<"-treeChange: V3Ast.cpp:"<<lineno<<" Tree Change for "<<prefix<<endl;
//  if (debug()) v3Global.rootp()->dumpTree("-  treeChange: ");
//  if (next||1) nodep->dumpTreeAndNext(cout, prefix);
//  else nodep->dumpTree(prefix);
//  nodep->checkTree();
//  v3Global.rootp()->checkTree();
//}
#ਜੇ_ਅੰਤ
}

ਸਾਂਚਾ <>
Ast_ਗੰਢ* Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_ਰੁੱਖ_Change(nodep, "-addNextThs: ", __LINE__, ਝੂਠ);
    debug_ਰੁੱਖ_Change(newp, "-addNextNew: ", __LINE__, ਸੱਚ);
    ਜੇ (!nodep)  // verilog.y and lots of other places assume this
        ਵਾਪਸ newp;

    // Find end of old list
    Ast_ਗੰਢ* oldtailp = nodep;
    ਜੇ (oldtailp->m_nextp) {
        ਜੇ (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } ਨਹੀਂਤਾਂ {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            ਜਦੋਂ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_ਗੰਢ* ਅਚਲ newtailp = newp->m_headtailp;
    Ast_ਗੰਢ* ਅਚਲ headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;  // May be written again as new head
    newp->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_ਗਿਣਤੀ_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_ਰੁੱਖ_Change(nodep, "-addNextOut:", __LINE__, ਸੱਚ);
    ਵਾਪਸ nodep;
}

ਖਾਲੀ Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(Ast_ਗੰਢ* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_ਰੁੱਖ_Change(ਇਹ, "-addHereThs: ", __LINE__, ਝੂਠ);
    debug_ਰੁੱਖ_Change(newp, "-addHereNew: ", __LINE__, ਸੱਚ);
    newp->edit_ਗਿਣਤੀ_Inc();

    Ast_ਗੰਢ* ਅਚਲ addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_ਗੰਢ* ਅਚਲ oldnextp = ਇਹ->m_nextp;
    ਇਹ->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    ਜੇ (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = ਇਹ;

    // Head/tail
    Ast_ਗੰਢ* ਅਚਲ oldheadtailp = ਇਹ->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;  // Not at head any longer
    addlastp->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    ਜੇ (oldheadtailp) {  // else in middle of list, no change
        ਜੇ (oldheadtailp == ਇਹ) {  // this was one node
            ਇਹ->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } ਨਹੀਂਤਾਂ ਜੇ (!oldnextp) {  // this was tail
            ਇਹ->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_ਰੁੱਖ_Change(ਇਹ, "-addHereOut: ", __LINE__, ਸੱਚ);
}

ਖਾਲੀ Ast_ਗੰਢ::ਸੈੱਟ_Op1p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, ਇਹ, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_ਰੁੱਖ_Change(ਇਹ, "-setOp1pThs: ", __LINE__, ਝੂਠ);
    debug_ਰੁੱਖ_Change(newp, "-setOp1pNew: ", __LINE__, ਸੱਚ);
    m_op1p = newp;
    newp->edit_ਗਿਣਤੀ_Inc();
    newp->m_backp = ਇਹ;
    debug_ਰੁੱਖ_Change(ਇਹ, "-setOp1pOut: ", __LINE__, ਝੂਠ);
}

ਖਾਲੀ Ast_ਗੰਢ::ਸੈੱਟ_Op2p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, ਇਹ, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_ਰੁੱਖ_Change(ਇਹ, "-setOp2pThs: ", __LINE__, ਝੂਠ);
    debug_ਰੁੱਖ_Change(newp, "-setOp2pNew: ", __LINE__, ਸੱਚ);
    m_op2p = newp;
    newp->edit_ਗਿਣਤੀ_Inc();
    newp->m_backp = ਇਹ;
    debug_ਰੁੱਖ_Change(ਇਹ, "-setOp2pOut: ", __LINE__, ਝੂਠ);
}

ਖਾਲੀ Ast_ਗੰਢ::ਸੈੱਟ_Op3p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, ਇਹ, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_ਰੁੱਖ_Change(ਇਹ, "-setOp3pThs: ", __LINE__, ਝੂਠ);
    debug_ਰੁੱਖ_Change(newp, "-setOp3pNew: ", __LINE__, ਸੱਚ);
    m_op3p = newp;
    newp->edit_ਗਿਣਤੀ_Inc();
    newp->m_backp = ਇਹ;
    debug_ਰੁੱਖ_Change(ਇਹ, "-setOp3pOut: ", __LINE__, ਝੂਠ);
}

ਖਾਲੀ Ast_ਗੰਢ::ਸੈੱਟ_Op4p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, ਇਹ, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_ਰੁੱਖ_Change(ਇਹ, "-setOp4pThs: ", __LINE__, ਝੂਠ);
    debug_ਰੁੱਖ_Change(newp, "-setOp4pNew: ", __LINE__, ਸੱਚ);
    m_op4p = newp;
    newp->edit_ਗਿਣਤੀ_Inc();
    newp->m_backp = ਇਹ;
    debug_ਰੁੱਖ_Change(ਇਹ, "-setOp4pOut: ", __LINE__, ਝੂਠ);
}

ਖਾਲੀ Ast_ਗੰਢ::ਜੋੜੋ_Op1p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ਜੇ (!m_op1p) {
        op1p(newp);
    } ਨਹੀਂਤਾਂ {
        m_op1p->ਜੋੜੋ_ਅਗਲਾ(newp);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::ਜੋੜੋ_Op2p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ਜੇ (!m_op2p) {
        op2p(newp);
    } ਨਹੀਂਤਾਂ {
        m_op2p->ਜੋੜੋ_ਅਗਲਾ(newp);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::ਜੋੜੋ_Op3p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ਜੇ (!m_op3p) {
        op3p(newp);
    } ਨਹੀਂਤਾਂ {
        m_op3p->ਜੋੜੋ_ਅਗਲਾ(newp);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::ਜੋੜੋ_Op4p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ਜੇ (!m_op4p) {
        op4p(newp);
    } ਨਹੀਂਤਾਂ {
        m_op4p->ਜੋੜੋ_ਅਗਲਾ(newp);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::prependOp1p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ਜੇ (!m_op1p) {
        op1p(newp);
    } ਨਹੀਂਤਾਂ {
        m_op1p->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::prependOp2p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ਜੇ (!m_op2p) {
        op2p(newp);
    } ਨਹੀਂਤਾਂ {
        m_op2p->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::prependOp3p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ਜੇ (!m_op3p) {
        op3p(newp);
    } ਨਹੀਂਤਾਂ {
        m_op3p->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::prependOp4p(Ast_ਗੰਢ* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ਜੇ (!m_op4p) {
        op4p(newp);
    } ਨਹੀਂਤਾਂ {
        m_op4p->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::replaceWith(Ast_ਗੰਢ* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    ਇਹ->unlink_Fr_ਪਿੱਠ(&repHandle);
    repHandle.relink(newp);
}
ਖਾਲੀ Ast_ਗੰਢ::replaceWithKeepDType(Ast_ਗੰਢ* newp) {
    newp->dtypeFrom(ਇਹ);
    replaceWith(newp);
}

ਖਾਲੀ VNRelinker::dump(std::ostream& str) ਅਚਲ {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_ਅਗਲਾ ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_ਗੰਢ* Ast_ਗੰਢ::unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(VNRelinker* linkerp) {
    debug_ਰੁੱਖ_Change(ਇਹ, "-unlinkWNextThs: ", __LINE__, ਸੱਚ);
    Ast_ਗੰਢ* ਅਚਲ oldp = ਇਹ;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ਗਿਣਤੀ_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ਗੰਢ* ਅਚਲ backp = oldp->m_backp;
    ਜੇ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        ਜੇ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_ਅਗਲਾ;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ਨਹੀਂਤਾਂ {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ਜੇ (backp->m_nextp == oldp) {
        backp->m_nextp = ਖਾਲੀ_ਸੂਚਕ;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_ਗੰਢ* oldtailp = backp->m_headtailp;
        ਜੇ (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            ਜਦੋਂ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_ਗੰਢ* ਅਚਲ oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op1p == oldp) {
        backp->m_op1p = ਖਾਲੀ_ਸੂਚਕ;
    } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op2p == oldp) {
        backp->m_op2p = ਖਾਲੀ_ਸੂਚਕ;
    } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op3p == oldp) {
        backp->m_op3p = ਖਾਲੀ_ਸੂਚਕ;
    } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op4p == oldp) {
        backp->m_op4p = ਖਾਲੀ_ਸੂਚਕ;
    } ਨਹੀਂਤਾਂ {
        ਇਹ->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = ਖਾਲੀ_ਸੂਚਕ;
    // Iterator fixup
    ਜੇ (oldp->m_iterpp) {
        *(oldp->m_iterpp) = ਖਾਲੀ_ਸੂਚਕ;
        oldp->m_iterpp = ਖਾਲੀ_ਸੂਚਕ;
    }
    debug_ਰੁੱਖ_Change(oldp, "-unlinkWNextOut: ", __LINE__, ਸੱਚ);
    ਵਾਪਸ oldp;
}

Ast_ਗੰਢ* Ast_ਗੰਢ::unlink_Fr_ਪਿੱਠ(VNRelinker* linkerp) {
    debug_ਰੁੱਖ_Change(ਇਹ, "-unlinkFrBkThs: ", __LINE__, ਸੱਚ);
    Ast_ਗੰਢ* ਅਚਲ oldp = ਇਹ;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ਗਿਣਤੀ_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ਗੰਢ* ਅਚਲ backp = oldp->m_backp;
    ਜੇ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        ਜੇ (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = ਖਾਲੀ_ਸੂਚਕ;
            oldp->m_iterpp = ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_ਅਗਲਾ;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ਨਹੀਂਤਾਂ {
            ਇਹ->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ਜੇ (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        ਜੇ (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        ਜੇ (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } ਨਹੀਂਤਾਂ {
        ਜੇ (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } ਨਹੀਂਤਾਂ {
            ਇਹ->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        ਜੇ (oldp->m_nextp) {
            Ast_ਗੰਢ* ਅਚਲ newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    ਜੇ (oldp->m_iterpp) {  // Only if no linker, point to next in list
        ਜੇ (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = ਖਾਲੀ_ਸੂਚਕ;
    }
    // Relink
    oldp->m_nextp = ਖਾਲੀ_ਸੂਚਕ;
    oldp->m_backp = ਖਾਲੀ_ਸੂਚਕ;
    oldp->m_headtailp = oldp;
    debug_ਰੁੱਖ_Change(oldp, "-unlinkFrBkOut: ", __LINE__, ਸੱਚ);
    ਵਾਪਸ oldp;
}

ਖਾਲੀ Ast_ਗੰਢ::relink(VNRelinker* linkerp) {
    ਜੇ (debug() > 8) {
        UINFO_ਅਗੇਤਰ(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_ਗੰਢ* ਅਚਲ newp = ਇਹ;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_ਗਿਣਤੀ_Inc();

    ਜੇ (debug() > 8) {
        linkerp->dump(ਬਾਹਰ);
        ਬਾਹਰ << ਸਤਰ_ਅੰਤ;
    }

    // cppcheck-suppress shadowFunction
    Ast_ਗੰਢ* ਅਚਲ backp = linkerp->m_backp;
    debug_ਰੁੱਖ_Change(ਇਹ, "-relinkNew: ", __LINE__, ਸੱਚ);
    debug_ਰੁੱਖ_Change(backp, "-relinkTre: ", __LINE__, ਸੱਚ);

    ਚੋਣ (linkerp->m_chg) {
    ਹਾਲਤ VNRelinker::RELINK_ਅਗਲਾ: backp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newp); ਰੋਕੋ;
    ਹਾਲਤ VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); ਰੋਕੋ;
    ਹਾਲਤ VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); ਰੋਕੋ;
    ਹਾਲਤ VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); ਰੋਕੋ;
    ਹਾਲਤ VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); ਰੋਕੋ;
    ਮੂਲ: ਇਹ->v3fatalSrc("Relink of node without any link to change."); ਰੋਕੋ;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = ਖਾਲੀ_ਸੂਚਕ;
    // Iterator fixup
    ਜੇ (linkerp->m_iterpp) {
        // If we're iterating over a next() link, we need to follow links off the
        // NEW node, which is always assumed to be what we are relinking to.
        // This adds a unfortunate hot 8 bytes to every AstNode, but is faster than passing
        // across every function.
        // If anyone has a cleaner way, I'd be grateful.
        newp->m_iterpp = linkerp->m_iterpp;
        *(newp->m_iterpp) = newp;
    }
    // Empty the linker so not used twice accidentally
    linkerp->m_backp = ਖਾਲੀ_ਸੂਚਕ;
    debug_ਰੁੱਖ_Change(ਇਹ, "-relinkOut: ", __LINE__, ਸੱਚ);
}

ਖਾਲੀ Ast_ਗੰਢ::relinkOneLink(Ast_ਗੰਢ*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_ਗੰਢ* newp) {
    ਜੇ (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_ਗੰਢ* ਅਚਲ newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_ਗੰਢ* ਅਚਲ oldlistlastp = pointpr->m_headtailp;
        UASSERT_OBJ(!(oldlistlastp->m_nextp && oldlistlastp != pointpr), newp,
                    "Old headtailp tail isn't at the tail");
        // Next links
        newlistlastp->m_nextp = pointpr;
        pointpr->m_backp = newlistlastp;
        // Head/tail
        pointpr->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;  // Old head
        newlistlastp->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;  // Old tail
        newp->m_headtailp = oldlistlastp;  // Head points to tail
        oldlistlastp->m_headtailp = newp;  // Tail points to head
    }
    pointpr = newp;
}

ਖਾਲੀ Ast_ਗੰਢ::ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(Ast_ਗੰਢ* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(ਇਹ->m_backp, ਇਹ, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_ਗੰਢ* ਅਚਲ backp = ਇਹ->m_backp;
    Ast_ਗੰਢ* ਅਚਲ newLastp = newp->m_headtailp;
    //
    ਇਹ->edit_ਗਿਣਤੀ_Inc();
    // Common linkage
    newLastp->m_nextp = ਇਹ;
    ਇਹ->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;
    // Linkage dependent on position
    ਜੇ (backp && backp->m_nextp == ਇਹ) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;
    } ਨਹੀਂਤਾਂ {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        ਜੇ (backp) {
            ਜੇ (backp->m_op1p == ਇਹ) {
                backp->m_op1p = newp;
            } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op2p == ਇਹ) {
                backp->m_op2p = newp;
            } ਨਹੀਂਤਾਂ ਜੇ (backp->m_op3p == ਇਹ) {
                backp->m_op3p = newp;
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(backp->m_op4p == ਇਹ, ਇਹ, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_ਗੰਢ* ਅਚਲ tailp = ਇਹ->m_headtailp;
        ਇਹ->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    ਜੇ (newLastp->m_iterpp) *(newLastp->m_iterpp) = ਇਹ;
    ਜੇ (ਇਹ->m_iterpp) {
        *(ਇਹ->m_iterpp) = newp;
        ਇਹ->m_iterpp = ਖਾਲੀ_ਸੂਚਕ;
    }
    //
    debug_ਰੁੱਖ_Change(ਇਹ, "-addHereThisAsNext: ", __LINE__, ਸੱਚ);
}

//======================================================================
// Clone

Ast_ਗੰਢ* Ast_ਗੰਢ::clone_ਰੁੱਖ_Iter(ਤਾਰਕਿਕ need_ਸ਼ੁੱਧ) {
    // private: Clone single node and children
    ਜੇ (need_ਸ਼ੁੱਧ) purityCheck();
    Ast_ਗੰਢ* ਅਚਲ newp = ਇਹ->clone();
    ਜੇ (ਇਹ->m_op1p) newp->op1p(ਇਹ->m_op1p->clone_ਰੁੱਖ_Iter_List(need_ਸ਼ੁੱਧ));
    ਜੇ (ਇਹ->m_op2p) newp->op2p(ਇਹ->m_op2p->clone_ਰੁੱਖ_Iter_List(need_ਸ਼ੁੱਧ));
    ਜੇ (ਇਹ->m_op3p) newp->op3p(ਇਹ->m_op3p->clone_ਰੁੱਖ_Iter_List(need_ਸ਼ੁੱਧ));
    ਜੇ (ਇਹ->m_op4p) newp->op4p(ਇਹ->m_op4p->clone_ਰੁੱਖ_Iter_List(need_ਸ਼ੁੱਧ));
    newp->m_iterpp = ਖਾਲੀ_ਸੂਚਕ;
    newp->clonep(ਇਹ);  // Save pointers to/from both to simplify relinking.
    ਇਹ->clonep(newp);  // Save pointers to/from both to simplify relinking.
    ਵਾਪਸ newp;
}

Ast_ਗੰਢ* Ast_ਗੰਢ::clone_ਰੁੱਖ_Iter_List(ਤਾਰਕਿਕ need_ਸ਼ੁੱਧ) {
    // private: Clone list of nodes, set m_headtailp
    Ast_ਗੰਢ* newheadp = ਖਾਲੀ_ਸੂਚਕ;
    Ast_ਗੰਢ* newtailp = ਖਾਲੀ_ਸੂਚਕ;
    // Audited to make sure this is never nullptr
    ਲਈ (Ast_ਗੰਢ* oldp = ਇਹ; oldp; oldp = oldp->m_nextp) {
        Ast_ਗੰਢ* ਅਚਲ newp = oldp->clone_ਰੁੱਖ_Iter(need_ਸ਼ੁੱਧ);
        newp->m_headtailp = ਖਾਲੀ_ਸੂਚਕ;
        newp->m_backp = newtailp;
        ਜੇ (newtailp) newtailp->m_nextp = newp;
        ਜੇ (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    ਵਾਪਸ newheadp;
}

Ast_ਗੰਢ* Ast_ਗੰਢ::clone_ਰੁੱਖ(ਤਾਰਕਿਕ clone_ਅਗਲਾ_Link, ਤਾਰਕਿਕ need_ਸ਼ੁੱਧ) {
    debug_ਰੁੱਖ_Change(ਇਹ, "-cloneThs: ", __LINE__, clone_ਅਗਲਾ_Link);
    clone_Clear_ਰੁੱਖ();
    Ast_ਗੰਢ* newp;
    ਜੇ (clone_ਅਗਲਾ_Link && ਇਹ->m_nextp) {
        newp = clone_ਰੁੱਖ_Iter_List(need_ਸ਼ੁੱਧ);
    } ਨਹੀਂਤਾਂ {
        newp = clone_ਰੁੱਖ_Iter(need_ਸ਼ੁੱਧ);
        newp->m_nextp = ਖਾਲੀ_ਸੂਚਕ;
        newp->m_headtailp = newp;
    }
    newp->m_backp = ਖਾਲੀ_ਸੂਚਕ;
    newp->clone_Relink_ਰੁੱਖ();
    debug_ਰੁੱਖ_Change(newp, "-cloneOut: ", __LINE__, ਸੱਚ);
    ਵਾਪਸ newp;
}

ਖਾਲੀ Ast_ਗੰਢ::purityCheck() {
    ਜੇ (VL_UNLIKELY(!is_ਸ਼ੁੱਧ())) {
        ਇਹ->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << ਇਹ->warn_ਹੋਰ()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

ਖਾਲੀ Ast_ਗੰਢ::delete_ਗੰਢ() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_ਗਿਣਤੀ_Inc();
    // Change links of old node so we coredump if used
    ਇਹ->m_nextp = reinterpret_cast<Ast_ਗੰਢ*>(0x1);
    ਇਹ->m_backp = reinterpret_cast<Ast_ਗੰਢ*>(0x1);
    ਇਹ->m_headtailp = reinterpret_cast<Ast_ਗੰਢ*>(0x1);
    ਇਹ->m_op1p = reinterpret_cast<Ast_ਗੰਢ*>(0x1);
    ਇਹ->m_op2p = reinterpret_cast<Ast_ਗੰਢ*>(0x1);
    ਇਹ->m_op3p = reinterpret_cast<Ast_ਗੰਢ*>(0x1);
    ਇਹ->m_op4p = reinterpret_cast<Ast_ਗੰਢ*>(0x1);
    ਇਹ->m_iterpp = reinterpret_cast<Ast_ਗੰਢ**>(0x1);
    ਜੇ (
#ਜੇ !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        ਸੱਚ
#ਨਹੀਂਤਾਂ
        !v3Global.opt.debugLeak()
#ਜੇ_ਅੰਤ
    ) {
        ਮਿਟਾਓ ਇਹ;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

ਖਾਲੀ Ast_ਗੰਢ::delete_ਰੁੱਖ_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    ਲਈ (Ast_ਗੰਢ *nodep = ਇਹ, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        ਜੇ (nodep->m_op1p) nodep->m_op1p->delete_ਰੁੱਖ_Iter();
        ਜੇ (nodep->m_op2p) nodep->m_op2p->delete_ਰੁੱਖ_Iter();
        ਜੇ (nodep->m_op3p) nodep->m_op3p->delete_ਰੁੱਖ_Iter();
        ਜੇ (nodep->m_op4p) nodep->m_op4p->delete_ਰੁੱਖ_Iter();
        nodep->m_nextp = ਖਾਲੀ_ਸੂਚਕ;
        nodep->m_backp = ਖਾਲੀ_ਸੂਚਕ;
        nodep->delete_ਗੰਢ();
    }
}

ਖਾਲੀ Ast_ਗੰਢ::delete_ਰੁੱਖ() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_ਰੁੱਖ_Change(ਇਹ, "-delTree:  ", __LINE__, ਸੱਚ);
    ਇਹ->edit_ਗਿਣਤੀ_Inc();
    // MUST be depth first!
    delete_ਰੁੱਖ_Iter();
}

//======================================================================
// Memory checks

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_LEAK_CHECKS
ਖਾਲੀ* Ast_ਗੰਢ::ਸੰਚਾਲਕ ਨਵਾਂ(ਮਾਪ_t ਮਾਪ) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_ਗੰਢ* ਅਚਲ objp = static_cast<Ast_ਗੰਢ*>(::ਸੰਚਾਲਕ ਨਵਾਂ(ਮਾਪ));
    V3Broken::ਜੋੜੋ_Newed(objp);
    ਵਾਪਸ objp;
}

ਖਾਲੀ Ast_ਗੰਢ::ਸੰਚਾਲਕ ਮਿਟਾਓ(ਖਾਲੀ* objp, ਮਾਪ_t ਮਾਪ) {
    ਜੇ (!objp) ਵਾਪਸ;
    ਅਚਲ Ast_ਗੰਢ* ਅਚਲ nodep = static_cast<Ast_ਗੰਢ*>(objp);
    V3Broken::deleted(nodep);
    ::ਸੰਚਾਲਕ ਮਿਟਾਓ(objp);
}
#ਜੇ_ਅੰਤ

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_ALLOC_RANDOM_CHECKS
ਖਾਲੀ* Ast_ਗੰਢ::ਸੰਚਾਲਕ ਨਵਾਂ(ਮਾਪ_t ਮਾਪ) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ਸਥਿਰ ਅਚਲ ਮਾਪ_t ASTGEN_MAX_ਗੰਢ_ਮਾਪ = []() {
        ਮਾਪ_t max_ਮਾਪ = 0;
        ਲਈ (ਮਾਪ_t t = 0; t < VNType::ਅੰਕ_TYPES(); ++t) {
            max_ਮਾਪ = std::max(max_ਮਾਪ, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        ਵਾਪਸ max_ਮਾਪ;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr ਮਾਪ_t POOL_ਮਾਪ = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ਸਥਿਰ uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    ਜੇ (ਪੂਰਨ ਬੀਜ = v3Global.opt.debugAllocRandom()) {
        ਸਥਿਰ V3Mutex s_mutex;
        ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{s_mutex};
        ਸਥਿਰ std::array<ਖਾਲੀ*, POOL_ਮਾਪ> s_ਗੰਢ_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(ਮਾਪ <= ASTGEN_MAX_ਗੰਢ_ਮਾਪ, "fix ASTGEN_MAX_NODE_SIZE");
        ਜੇ (!s_lfsr) {
            s_lfsr = ਬੀਜ;
            ਜੇ (!s_lfsr) s_lfsr = ~s_lfsr;
            ਲਈ (ਮਾਪ_t i = 0; i < POOL_ਮਾਪ; ++i) {
                s_ਗੰਢ_Pool[i] = ::ਸੰਚਾਲਕ ਨਵਾਂ(ASTGEN_MAX_ਗੰਢ_ਮਾਪ + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::ਛਾਂਟੋ(std::begin(s_ਗੰਢ_Pool), std::ਸਿਰਾ(s_ਗੰਢ_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ਅਚਲ ਮਾਪ_t ਸੂਚਕਾਂਕ = s_lfsr % POOL_ਮਾਪ;
        Ast_ਗੰਢ* ਅਚਲ objp = static_cast<Ast_ਗੰਢ*>(s_ਗੰਢ_Pool[ਸੂਚਕਾਂਕ]);
        s_ਗੰਢ_Pool[ਸੂਚਕਾਂਕ] = ::ਸੰਚਾਲਕ ਨਵਾਂ(ASTGEN_MAX_ਗੰਢ_ਮਾਪ + 64);  // For later new()
        ਵਾਪਸ objp;
    } ਨਹੀਂਤਾਂ {
        Ast_ਗੰਢ* ਅਚਲ objp = static_cast<Ast_ਗੰਢ*>(::ਸੰਚਾਲਕ ਨਵਾਂ(ਮਾਪ));
        ਵਾਪਸ objp;
    }
}
ਖਾਲੀ Ast_ਗੰਢ::ਸੰਚਾਲਕ ਮਿਟਾਓ(ਖਾਲੀ* objp, ਮਾਪ_t ਮਾਪ) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (ਖਾਲੀ)objp;
    (ਖਾਲੀ)ਮਾਪ;
}
#ਜੇ_ਅੰਤ

//======================================================================
// Iterators

ਖਾਲੀ Ast_ਗੰਢ::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ਜੇ (m_op1p) m_op1p->iterate_And_ਅਗਲਾ(v);
    ਜੇ (m_op2p) m_op2p->iterate_And_ਅਗਲਾ(v);
    ਜੇ (m_op3p) m_op3p->iterate_And_ਅਗਲਾ(v);
    ਜੇ (m_op4p) m_op4p->iterate_And_ਅਗਲਾ(v);
}

ਖਾਲੀ Ast_ਗੰਢ::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ਜੇ (m_op1p) m_op1p->iterate_And_ਅਗਲਾ_Const(v);
    ਜੇ (m_op2p) m_op2p->iterate_And_ਅਗਲਾ_Const(v);
    ਜੇ (m_op3p) m_op3p->iterate_And_ਅਗਲਾ_Const(v);
    ਜੇ (m_op4p) m_op4p->iterate_And_ਅਗਲਾ_Const(v);
}

ਖਾਲੀ Ast_ਗੰਢ::iterate_And_ਅਗਲਾ(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_ਗੰਢ* nodep = ਇਹ;
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#ਜੇ_ਅੰਤ
    // cppcheck-suppress knownConditionTrueFalse
    ਜੇ (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    ਜਦੋਂ (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        ਜੇ (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_ਗੰਢ* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
        // Desirable check, but many places where multiple iterations are OK
        // UASSERT_OBJ(!niterp->m_iterpp, niterp, "IterateAndNext under iterateAndNext may miss
        // edits"); Optimization note: Doing PREFETCH_RW on m_iterpp is a net even
        // cppcheck-suppress nullPointer
        niterp->m_iterpp = &niterp;
        niterp->accept(v);
        // accept may do a replaceNode and change niterp on us...
        // niterp maybe nullptr, so need cast if printing
        // if (niterp != nodep) UINFO(1,"iterateAndNext edited "<<cvtToHex(nodep)
        //                             <<" now into "<<cvtToHex(niterp)<<endl);
        ਜੇ (!niterp) ਵਾਪਸ;  // Perhaps node deleted inside accept
        niterp->m_iterpp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } ਨਹੀਂਤਾਂ {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

ਖਾਲੀ Ast_ਗੰਢ::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_ਗੰਢ* nodep = ਇਹ;
    ਜਦੋਂ (nodep->m_nextp) nodep = nodep->m_nextp;
    ਜਦੋਂ (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        ਜੇ (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } ਨਹੀਂਤਾਂ {
            nodep = ਖਾਲੀ_ਸੂਚਕ;
        }  // else: backp points up the tree.
    }
}

ਖਾਲੀ Ast_ਗੰਢ::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    ਜੇ (m_op1p) m_op1p->iterateListBackwardsConst(v);
    ਜੇ (m_op2p) m_op2p->iterateListBackwardsConst(v);
    ਜੇ (m_op3p) m_op3p->iterateListBackwardsConst(v);
    ਜੇ (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

ਖਾਲੀ Ast_ਗੰਢ::iterate_And_ਅਗਲਾ_Const(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_ਗੰਢ* nodep = ਇਹ;
    ਕਰੋ {
        Ast_ਗੰਢ* ਅਚਲ nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } ਜਦੋਂ (nodep);
}

Ast_ਗੰਢ* Ast_ਗੰਢ::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_ਗੰਢ* nodep = ਇਹ;  // Note "this" may point to bogus point later in this function
    ਜੇ (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } ਨਹੀਂਤਾਂ ਜੇ (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ਅਚਲ tempp = ਨਵਾਂ AstBegin{nodep->fileline(), "[EditWrapper]", nodep, ਝੂਠ};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        VL_DO_DANGLING(tempp->delete_ਰੁੱਖ(), tempp);
    } ਨਹੀਂਤਾਂ {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_ਗੰਢ** nextnodepp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਇਹ->m_backp->m_op1p == ਇਹ) {
            nextnodepp = &(ਇਹ->m_backp->m_op1p);
        } ਨਹੀਂਤਾਂ ਜੇ (ਇਹ->m_backp->m_op2p == ਇਹ) {
            nextnodepp = &(ਇਹ->m_backp->m_op2p);
        } ਨਹੀਂਤਾਂ ਜੇ (ਇਹ->m_backp->m_op3p == ਇਹ) {
            nextnodepp = &(ਇਹ->m_backp->m_op3p);
        } ਨਹੀਂਤਾਂ ਜੇ (ਇਹ->m_backp->m_op4p == ਇਹ) {
            nextnodepp = &(ਇਹ->m_backp->m_op4p);
        } ਨਹੀਂਤਾਂ ਜੇ (ਇਹ->m_backp->m_nextp == ਇਹ) {
            nextnodepp = &(ਇਹ->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, ਇਹ, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    ਵਾਪਸ nodep;
}

//======================================================================

ਖਾਲੀ Ast_ਗੰਢ::clone_Relink_ਰੁੱਖ() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    ਲਈ (Ast_ਗੰਢ* nodep = ਇਹ; nodep; nodep = nodep->m_nextp) {
        ਜੇ (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        ਜੇ (nodep->m_op1p) nodep->m_op1p->clone_Relink_ਰੁੱਖ();
        ਜੇ (nodep->m_op2p) nodep->m_op2p->clone_Relink_ਰੁੱਖ();
        ਜੇ (nodep->m_op3p) nodep->m_op3p->clone_Relink_ਰੁੱਖ();
        ਜੇ (nodep->m_op4p) nodep->m_op4p->clone_Relink_ਰੁੱਖ();
    }
}

//======================================================================
// Comparison

ਤਾਰਕਿਕ Ast_ਗੰਢ::gate_ਰੁੱਖ_Iter() ਅਚਲ {
    // private: Return true if the two trees are identical
    ਜੇ (!isGateOptimizable()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (m_op1p && !m_op1p->gate_ਰੁੱਖ_Iter()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (m_op2p && !m_op2p->gate_ਰੁੱਖ_Iter()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (m_op3p && !m_op3p->gate_ਰੁੱਖ_Iter()) ਵਾਪਸ ਝੂਠ;
    ਜੇ (m_op4p && !m_op4p->gate_ਰੁੱਖ_Iter()) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ ਸੱਚ;
}

ਤਾਰਕਿਕ Ast_ਗੰਢ::same_ਰੁੱਖ_Iter(ਅਚਲ Ast_ਗੰਢ* node1p, ਅਚਲ Ast_ਗੰਢ* node2p, ਤਾਰਕਿਕ ign_ਅਗਲਾ,
                           ਤਾਰਕਿਕ gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    ਜਦੋਂ (ਸੱਚ) {
        ਜੇ (!node1p && !node2p) ਵਾਪਸ ਸੱਚ;
        ਜੇ (!node1p || !node2p) ਵਾਪਸ ਝੂਠ;
        ਜੇ (node1p->type() != node2p->type()) ਵਾਪਸ ਝੂਠ;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        ਜੇ (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!node1p->same_ਗੰਢ(node2p) || (gateOnly && !node1p->isGateOptimizable())) ਵਾਪਸ ਝੂਠ;
        ਜੇ (!same_ਰੁੱਖ_Iter(node1p->m_op1p, node2p->m_op1p, ਝੂਠ, gateOnly)
            || !same_ਰੁੱਖ_Iter(node1p->m_op2p, node2p->m_op2p, ਝੂਠ, gateOnly)
            || !same_ਰੁੱਖ_Iter(node1p->m_op3p, node2p->m_op3p, ਝੂਠ, gateOnly)
            || !same_ਰੁੱਖ_Iter(node1p->m_op4p, node2p->m_op4p, ਝੂਠ, gateOnly)) {
            ਵਾਪਸ ਝੂਠ;
        }
        ਜੇ (ign_ਅਗਲਾ) ਵਾਪਸ ਸੱਚ;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

ਖਾਲੀ Ast_ਗੰਢ::check_ਰੁੱਖ_Iter(ਅਚਲ Ast_ਗੰਢ* prevBackp) ਅਚਲ VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == ਇਹ->backp(), ਇਹ, "Back node inconsistent");
    ਅਚਲ VNTypeInfo& typeInfo = VNType::typeInfo(ਇਹ->type());
    ਲਈ (ਪੂਰਨ i = 1; i <= 4; i++) {
        Ast_ਗੰਢ* nodep = ਖਾਲੀ_ਸੂਚਕ;
        ਚੋਣ (i) {
        ਹਾਲਤ 1: nodep = op1p(); ਰੋਕੋ;
        ਹਾਲਤ 2: nodep = op2p(); ਰੋਕੋ;
        ਹਾਲਤ 3: nodep = op3p(); ਰੋਕੋ;
        ਹਾਲਤ 4: nodep = op4p(); ਰੋਕੋ;
        ਮੂਲ: ਇਹ->v3fatalSrc("Bad case"); ਰੋਕੋ;
        }
        // cppcheck-suppress danglingTempReference
        ਅਚਲ ਅੱਖਰ* opName = typeInfo.m_opNamep[i - 1];
        ਚੋਣ (typeInfo.m_opType[i - 1]) {
        ਹਾਲਤ VNTypeInfo::OP_ਅਣਵਰਤਿਆ:
            UASSERT_OBJ(!nodep, ਇਹ, typeInfo.m_namep << " must not use " << opName << "()");
            ਰੋਕੋ;
        ਹਾਲਤ VNTypeInfo::OP_ਵਰਤਿਆ:
            UASSERT_OBJ(nodep, ਇਹ,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), ਇਹ,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_ਰੁੱਖ_Iter(ਇਹ);
            ਰੋਕੋ;
        ਹਾਲਤ VNTypeInfo::OP_LIST:
            ਜੇ (ਅਚਲ Ast_ਗੰਢ* ਅਚਲ headp = nodep) {
                // cppcheck-suppress shadowFunction
                ਅਚਲ Ast_ਗੰਢ* backp = ਇਹ;
                ਅਚਲ Ast_ਗੰਢ* tailp;
                ਅਚਲ Ast_ਗੰਢ* opp = headp;
                ਕਰੋ {
                    opp->check_ਰੁੱਖ_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } ਜਦੋਂ (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            ਰੋਕੋ;
        ਹਾਲਤ VNTypeInfo::OP_OPTIONAL:
            ਜੇ (nodep) {
                UASSERT_OBJ(!nodep->nextp(), ਇਹ,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_ਰੁੱਖ_Iter(ਇਹ);
            }
            ਰੋਕੋ;
        ਮੂਲ: ਇਹ->v3fatalSrc("Bad case"); ਰੋਕੋ;
        }
    }
    ਜੇ (v3Global.opt.debug_ਚੌੜਾਈ() && v3Global.ਚੌੜਾਈ_Min_Usage() == VWidthMinUsage::VERILOG_ਚੌੜਾਈ) {
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_CAST(ਇਹ, ਗੰਢ_ਵਿਅੰਜਕ)) {
            ਅਚਲ ਅੱਖਰ* ਅਚਲ whyp = exprp->ਚੌੜਾਈ_Mismatch();
            ਜੇ (whyp) {
                ਸਵੈਚਾਲਿਤ dtypeStr = [](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    ਵਾਪਸ ss.str();
                };
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_Uniop* ਅਚਲ uniopp = VN_CAST(exprp, ਗੰਢ_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ biopp = VN_CAST(exprp, ਗੰਢ_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } ਨਹੀਂਤਾਂ {
                    UASSERT_OBJ(ਝੂਠ, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
ਅੱਖਰ* Ast_ਗੰਢ::dump_ਰੁੱਖ_Json_Gdb(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਜੇ (!nodep) ਵਾਪਸ strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodep_ਧਾਰਾ;
    nodep->dump_ਰੁੱਖ_Json(nodep_ਧਾਰਾ);
    ਅਚਲ std::string str = nodep_ਧਾਰਾ.rdbuf()->str();
    ਵਾਪਸ strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
ਅੱਖਰ* Ast_ਗੰਢ::dump_ਰੁੱਖ_Json_Gdb(ਅਚਲ ਅੱਖਰ* str) { ਵਾਪਸ strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
ਅੱਖਰ* Ast_ਗੰਢ::dump_ਰੁੱਖ_Json_Gdb(intptr_t nodep) {
    ਜੇ (!nodep) ਵਾਪਸ strdup("{\"addr\":\"NULL\"}\n");
    ਵਾਪਸ dump_ਰੁੱਖ_Json_Gdb(reinterpret_cast<ਅਚਲ Ast_ਗੰਢ*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
ਖਾਲੀ Ast_ਗੰਢ::dumpGdb(ਅਚਲ Ast_ਗੰਢ* nodep) {  // For GDB only  // LCOV_EXCL_START
    ਜੇ (!nodep) {
        ਬਾਹਰ << "<nullptr>" << ਸਤਰ_ਅੰਤ;
        ਵਾਪਸ;
    }
    nodep->dump_Gdb_ਸਿਰਲੇਖ();
    ਬਾਹਰ << "  ";
    nodep->dump(ਬਾਹਰ);
    ਬਾਹਰ << ਸਤਰ_ਅੰਤ;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ਖਾਲੀ Ast_ਗੰਢ::dump_Gdb_ਸਿਰਲੇਖ() ਅਚਲ {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(ਬਾਹਰ);
    ਬਾਹਰ << "  Fileline = " << fileline() << ਸਤਰ_ਅੰਤ;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ਖਾਲੀ Ast_ਗੰਢ::dump_ਰੁੱਖ_Gdb(ਅਚਲ Ast_ਗੰਢ* nodep) {  // For GDB only  // LCOV_EXCL_START
    ਜੇ (!nodep) {
        ਬਾਹਰ << "<nullptr>" << ਸਤਰ_ਅੰਤ;
        ਵਾਪਸ;
    }
    nodep->dump_Gdb_ਸਿਰਲੇਖ();
    nodep->dump_ਰੁੱਖ(ਬਾਹਰ);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ਖਾਲੀ Ast_ਗੰਢ::dump_ਰੁੱਖ_File_Gdb(ਅਚਲ Ast_ਗੰਢ* nodep,  // LCOV_EXCL_START
                              ਅਚਲ ਅੱਖਰ* filenamep) {  // For GDB only
    ਜੇ (!nodep) {
        ਬਾਹਰ << "<nullptr>" << ਸਤਰ_ਅੰਤ;
        ਵਾਪਸ;
    }
    ਅਚਲ string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_ਰੁੱਖ_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
ਖਾਲੀ Ast_ਗੰਢ::checkIter() ਅਚਲ {
    ਜੇ (m_iterpp) {
        dumpPtrs(ਬਾਹਰ);
        // Perhaps something forgot to clear m_iterpp?
        ਇਹ->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

ਖਾਲੀ Ast_ਗੰਢ::dumpPtrs(std::ostream& os) ਅਚਲ {
    os << "This=" << typeName() << " " << cvt_ਮੰਜ਼ਿਲ_Hex(ਇਹ);
    os << " back=" << cvt_ਮੰਜ਼ਿਲ_Hex(backp());
    ਜੇ (nextp()) os << " next=" << cvt_ਮੰਜ਼ਿਲ_Hex(nextp());
    ਜੇ (m_headtailp == ਇਹ) {
        os << " headtail=this";
    } ਨਹੀਂਤਾਂ {
        os << " headtail=" << cvt_ਮੰਜ਼ਿਲ_Hex(m_headtailp);
    }
    ਜੇ (op1p()) os << " op1p=" << cvt_ਮੰਜ਼ਿਲ_Hex(op1p());
    ਜੇ (op2p()) os << " op2p=" << cvt_ਮੰਜ਼ਿਲ_Hex(op2p());
    ਜੇ (op3p()) os << " op3p=" << cvt_ਮੰਜ਼ਿਲ_Hex(op3p());
    ਜੇ (op4p()) os << " op4p=" << cvt_ਮੰਜ਼ਿਲ_Hex(op4p());
    ਅਚਲ ਸਵੈਚਾਲਿਤ dumpUser = [&os](ਅਚਲ ਅੱਖਰ* ਅਗੇਤਰ, ਅਚਲ VNUser& user) {
        ਅਚਲ std::string s
            = user.dumpStr([](ਅਚਲ ਖਾਲੀ* p) -> std::string { ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Hex(p); });
        ਜੇ (!s.ਖਾਲੀ_ਥਾਵਾਂ()) os << ਅਗੇਤਰ << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    ਜੇ (m_iterpp) {
        os << " iterpp=" << cvt_ਮੰਜ਼ਿਲ_Hex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

ਖਾਲੀ Ast_ਗੰਢ::dump_ਰੁੱਖ(std::ostream& os, ਅਚਲ string& indent, ਪੂਰਨ max_ਡੂੰਘਾਈ) ਅਚਲ {
    ਸਥਿਰ ਪੂਰਨ s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << ਇਹ << '\n';
    ਜੇ (VN_DELETED(ਇਹ)) ਵਾਪਸ;
    ਜੇ (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    ਜੇ (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvt_ਮੰਜ਼ਿਲ_Hex(ਇਹ)
           << ਸਤਰ_ਅੰਤ;  // endl intentional to do flush
        ਵਾਪਸ;
    }  // LCOV_EXCL_STOP
    ਜੇ (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    ਜੇ (max_ਡੂੰਘਾਈ == 1) {
        ਜੇ (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } ਨਹੀਂਤਾਂ {
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ਰੁੱਖ(os, indent + "1:", max_ਡੂੰਘਾਈ - 1);
        }
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ਰੁੱਖ(os, indent + "2:", max_ਡੂੰਘਾਈ - 1);
        }
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ਰੁੱਖ(os, indent + "3:", max_ਡੂੰਘਾਈ - 1);
        }
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ਰੁੱਖ(os, indent + "4:", max_ਡੂੰਘਾਈ - 1);
        }
    }
}

ਖਾਲੀ Ast_ਗੰਢ::dump_ਰੁੱਖ_And_ਅਗਲਾ(std::ostream& os, ਅਚਲ string& indent, ਪੂਰਨ max_ਡੂੰਘਾਈ) ਅਚਲ {
    // Audited to make sure this is never nullptr
    ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = ਇਹ; nodep; nodep = nodep->nextp()) {
        nodep->dump_ਰੁੱਖ(os, indent, max_ਡੂੰਘਾਈ);
    }
}

ਖਾਲੀ Ast_ਗੰਢ::dump_ਰੁੱਖ_File(ਅਚਲ string& filename, ਤਾਰਕਿਕ doDump) {
    // Not const function as calls checkTree
    ਜੇ (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ਅਚਲ std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            ਜੇ (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_ਗਿਣਤੀ_ਆਖਰੀ();
            *logsp << "> to <e" << std::dec << edit_ਗਿਣਤੀ_Gbl() << ">\n";
            ਜੇ (edit_ਗਿਣਤੀ_Gbl() == edit_ਗਿਣਤੀ_ਆਖਰੀ() && ::dump_ਰੁੱਖ_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } ਨਹੀਂਤਾਂ {
                dump_ਰੁੱਖ(*logsp);
                edit_ਗਿਣਤੀ_ਸੈੱਟ_ਆਖਰੀ();  // Next dump can indicate start from here
            }
        }
    }
}

ਸਥਿਰ ਖਾਲੀ drawChildren(std::ostream& os, ਅਚਲ Ast_ਗੰਢ* thisp, ਅਚਲ Ast_ਗੰਢ* childp,
                         ਅਚਲ std::string& ਬੱਚਾ_Name) {
    ਜੇ (childp) {
        os << "\tn" << cvt_ਮੰਜ਼ਿਲ_Hex(thisp) << " -> n" << cvt_ਮੰਜ਼ਿਲ_Hex(childp) << " [" << "label=\""
           << ਬੱਚਾ_Name << "\" color=red];\n";
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_ਰੁੱਖ_Dot(os);
            ਜੇ (nodep->nextp()) {
                os << "\tn" << cvt_ਮੰਜ਼ਿਲ_Hex(nodep) << " -> n" << cvt_ਮੰਜ਼ਿਲ_Hex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvt_ਮੰਜ਼ਿਲ_Hex(nodep) << ", n" << cvt_ਮੰਜ਼ਿਲ_Hex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

ਖਾਲੀ Ast_ਗੰਢ::dump_ਰੁੱਖ_Dot(std::ostream& os) ਅਚਲ {
    os << "\tn" << cvt_ਮੰਜ਼ਿਲ_Hex(ਇਹ) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, ਇਹ, m_op1p, "op1");
    drawChildren(os, ਇਹ, m_op2p, "op2");
    drawChildren(os, ਇਹ, m_op3p, "op3");
    drawChildren(os, ਇਹ, m_op4p, "op4");
}

ਖਾਲੀ Ast_ਗੰਢ::dump_ਰੁੱਖ_Json_File(ਅਚਲ string& filename, ਤਾਰਕਿਕ doDump) {
    ਜੇ (!doDump) ਵਾਪਸ;
    UINFO(2, "Dumping " << filename);
    ਅਚਲ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ਜੇ (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_ਰੁੱਖ_Json(*treejsonp);
    *treejsonp << '\n';
}

ਖਾਲੀ Ast_ਗੰਢ::dumpJsonMetaFileGdb(ਅਚਲ ਅੱਖਰ* filename) { dumpJsonMetaFile(filename); }
ਖਾਲੀ Ast_ਗੰਢ::dumpJsonMetaFile(ਅਚਲ string& filename) {
    UINFO(2, "Dumping " << filename);
    ਅਚਲ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ਜੇ (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_ਸਤਰ::file_Name_ਅੰਕ_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptr_ਨਾਂ_Dump_Json(*treejsonp);
    *treejsonp << "}\n";
}

ਖਾਲੀ Ast_ਗੰਢ::dump_ਰੁੱਖ_Dot_File(ਅਚਲ string& filename, ਤਾਰਕਿਕ doDump) {
    ਜੇ (doDump) {
        UINFO(2, "Dumping " << filename);
        ਅਚਲ std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        ਜੇ (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_ਰੁੱਖ_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_ਗੰਢ::instanceStr() ਅਚਲ {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr ਬਿਨਾਨਿਸ਼ਾਨੀ maxIterations = 10000;
    ਬਿਨਾਨਿਸ਼ਾਨੀ iter_ਗਿਣਤੀ = 0;
    // Walk 'backp' chain
    ਲਈ (ਅਚਲ Ast_ਗੰਢ* currp = ਇਹ; currp; currp = currp->backp(), ++iter_ਗਿਣਤੀ) {
        ਜੇ (VL_UNCOVERABLE(iter_ਗਿਣਤੀ >= maxIterations)) ਵਾਪਸ "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        ਜੇ (ਅਚਲ AstScope* ਅਚਲ scopep = VN_CAST(currp, Scope)) {
            ਵਾਪਸ scopep->is_ਸਿਖਰ() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        ਜੇ (ਅਚਲ AstModule* ਅਚਲ modp = VN_CAST(currp, Module)) {
            ਅਚਲ string instanceName = modp->someInstanceName();
            ਵਾਪਸ instanceName.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    ਵਾਪਸ "";
}
ਖਾਲੀ Ast_ਗੰਢ::v3error_ਸਿਰਾ(ਅਚਲ std::ostringstream& str) ਅਚਲ VL_RELEASE(V3Error::s().m_mutex) {
    ਜੇ (!m_fileline) {
        V3Error::v3error_ਸਿਰਾ(str, "", ਖਾਲੀ_ਸੂਚਕ);
    } ਨਹੀਂਤਾਂ {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ਅਚਲ string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().error_ਕੋਡ()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        ਜੇ (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_ਗੰਢ*>(ਇਹ)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3error_ਸਿਰਾ(nsstr, instanceStrExtra);
    }
}
ਖਾਲੀ Ast_ਗੰਢ::v3error_ਸਿਰਾ_Fatal(ਅਚਲ std::ostringstream& str) ਅਚਲ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_ਸਿਰਾ(str);
    ਪ੍ਰਤਿਜ੍ਞਾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

ਖਾਲੀ Ast_ਗੰਢ::dtypeChgSigned(ਤਾਰਕਿਕ flag) {
    UASSERT_OBJ(dtypep(), ਇਹ, "No dtype when changing to (un)signed");
    dtype_Chg_ਚੌੜਾਈ_Signed(dtypep()->ਚੌੜਾਈ(), dtypep()->ਚੌੜਾਈ_Min(), VSigning::fromBool(flag));
}
ਖਾਲੀ Ast_ਗੰਢ::dtype_Chg_ਚੌੜਾਈ(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min) {
    UASSERT_OBJ(dtypep(), ਇਹ, "No dtype when changing width");
    dtype_Chg_ਚੌੜਾਈ_Signed(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, dtypep()->numeric());
}

ਖਾਲੀ Ast_ਗੰਢ::dtype_Chg_ਚੌੜਾਈ_Signed(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) {
    UASSERT_OBJ(dtypep(), ਇਹ, "No dtype when changing width");
    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ enumDTypep = dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
    ਜੇ (ਚੌੜਾਈ == dtypep()->ਚੌੜਾਈ() && ਚੌੜਾਈ_Min == dtypep()->ਚੌੜਾਈ_Min()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        ਵਾਪਸ;  // Correct already
    }
    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric, dtypep->isFourstate());
}

Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_Basic_DType(VBasicDTypeKwd kwd) ਅਚਲ {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Basic_DType(fileline(), kwd);
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Logic_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(fileline(), VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ,
                                                             ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric);
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_Logic_DType(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Logic_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric);
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_Logic2_State_DType(ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric) ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Logic_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric);
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_Logic_Range_DType(ਅਚਲ VNumRange& range, ਪੂਰਨ ਚੌੜਾਈ_Min,
                                           VSigning numeric) ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Logic_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, ਚੌੜਾਈ_Min, numeric);
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Range_DType(ਅਚਲ VNumRange& range, ਪੂਰਨ ਚੌੜਾਈ_Min,
                                         VSigning numeric) ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Logic_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(fileline(), VBasicDTypeKwd::ਦੋਆਧਾਰੀ_ਅੰਕ,
                                                             range, ਚੌੜਾਈ_Min, numeric);
}
AstBasicDType* Ast_ਗੰਢ::ਲੱਭੋ_ਪਾਓ_Same_DType(AstBasicDType* nodep) {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_ਪਾਓ_Same_DType(nodep);
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_Constraint_Ref_DType() ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Constraint_Ref_DType(fileline());
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType() ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType(fileline());
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_ਕਤਾਰ_ਸੂਚਕਾਂਕ_DType() ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_ਕਤਾਰ_ਸੂਚਕਾਂਕ_DType(fileline());
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_ਧਾਰਾ_DType() ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_ਧਾਰਾ_DType(fileline());
}
Ast_ਗੰਢ_DType* Ast_ਗੰਢ::ਲੱਭੋ_Void_DType() ਅਚਲ {
    ਵਾਪਸ v3Global.rootp()->typeTablep()->ਲੱਭੋ_Void_DType(fileline());
}

ਸਥਿਰ ਅਚਲ Ast_ਗੰਢ_DType* compute_Castable_ਅਧਾਰ(ਅਚਲ Ast_ਗੰਢ_DType* nodep) {
    ਜਦੋਂ (ਸੱਚ) {
        ਜੇ (ਅਚਲ AstPackArrayDType* ਅਚਲ packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            ਜਾਰੀ;
        }
        ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ refp = nodep->skip_Ref_ਮੰਜ਼ਿਲ_Enump()) {
            ਜੇ (refp != nodep) {
                nodep = refp;
                ਜਾਰੀ;
            }
        }
        ਵਾਪਸ nodep;
    }
}

ਸਥਿਰ VCastable computeCastableImp(ਅਚਲ Ast_ਗੰਢ_DType* ਮੰਜ਼ਿਲ_Dtp, ਅਚਲ Ast_ਗੰਢ_DType* fromDtp,
                                    ਅਚਲ Ast_ਗੰਢ* fromConstp, ਅਚਲ ਤਾਰਕਿਕ checkIfaceArgCompat) {
    ਅਚਲ VCastable castable = VCastable::UNSUPPORTED;
    ਮੰਜ਼ਿਲ_Dtp = ਮੰਜ਼ਿਲ_Dtp->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
    fromDtp = fromDtp->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
    ਜੇ (ਮੰਜ਼ਿਲ_Dtp == fromDtp) ਵਾਪਸ VCastable::SAMEISH;
    ਜੇ (ਮੰਜ਼ਿਲ_Dtp->similarDType(fromDtp)) ਵਾਪਸ VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ਅਚਲ Ast_ਗੰਢ_DType* from_ਅਧਾਰ_Dtp = compute_Castable_ਅਧਾਰ(fromDtp);

    ਅਚਲ ਤਾਰਕਿਕ fromNumericable = VN_IS(from_ਅਧਾਰ_Dtp, BasicDType) || VN_IS(from_ਅਧਾਰ_Dtp, EnumDType)
                                 || VN_IS(from_ਅਧਾਰ_Dtp, ਧਾਰਾ_DType)
                                 || VN_IS(from_ਅਧਾਰ_Dtp, ਗੰਢ_UOr_Struct_DType);

    ਅਚਲ Ast_ਗੰਢ_DType* ਮੰਜ਼ਿਲ_ਅਧਾਰ_Dtp = compute_Castable_ਅਧਾਰ(ਮੰਜ਼ਿਲ_Dtp);
    ਅਚਲ ਤਾਰਕਿਕ ਮੰਜ਼ਿਲ_Numericable
        = VN_IS(ਮੰਜ਼ਿਲ_ਅਧਾਰ_Dtp, BasicDType) || VN_IS(ਮੰਜ਼ਿਲ_ਅਧਾਰ_Dtp, ਗੰਢ_UOr_Struct_DType);

    ਜੇ (ਮੰਜ਼ਿਲ_ਅਧਾਰ_Dtp == from_ਅਧਾਰ_Dtp) ਵਾਪਸ VCastable::COMPATIBLE;

    ਜੇ (ਮੰਜ਼ਿਲ_Numericable) {
        ਜੇ (fromNumericable) ਵਾਪਸ VCastable::COMPATIBLE;
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਮੰਜ਼ਿਲ_ਅਧਾਰ_Dtp, EnumDType)) {
        ਜੇ (VN_IS(from_ਅਧਾਰ_Dtp, EnumDType) && ਮੰਜ਼ਿਲ_Dtp->same_ਰੁੱਖ(fromDtp))
            ਵਾਪਸ VCastable::ENUM_IMPLICIT;
        ਜੇ (fromNumericable) ਵਾਪਸ VCastable::ENUM_EXPLICIT;
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਮੰਜ਼ਿਲ_Dtp, ਕਤਾਰ_DType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, ਧਾਰਾ_DType))) {
        ਵਾਪਸ VCastable::COMPATIBLE;
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਮੰਜ਼ਿਲ_Dtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        ਜੇ (fromConstp->isNull()) ਵਾਪਸ VCastable::COMPATIBLE;
    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਮੰਜ਼ਿਲ_Dtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ਅਚਲ AstClass* ਅਚਲ ਮੰਜ਼ਿਲ_Classp = VN_AS(ਮੰਜ਼ਿਲ_Dtp, ClassRefDType)->classp();
        ਅਚਲ AstClass* ਅਚਲ fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ਅਚਲ ਤਾਰਕਿਕ downcast = AstClass::isClassExtendedFrom(ਮੰਜ਼ਿਲ_Classp, fromClassp);
        ਅਚਲ ਤਾਰਕਿਕ upcast = AstClass::isClassExtendedFrom(fromClassp, ਮੰਜ਼ਿਲ_Classp);
        ਜੇ (upcast) ਵਾਪਸ VCastable::COMPATIBLE;
        ਜੇ (downcast) ਵਾਪਸ VCastable::DYNAMIC_CLASS;
        ਵਾਪਸ VCastable::INCOMPATIBLE;
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ ਮੰਜ਼ਿਲ_Ifp
               = VN_CAST(checkIfaceArgCompat ? ਮੰਜ਼ਿਲ_Dtp->elemDTypep(ਸੱਚ) : ਮੰਜ਼ਿਲ_Dtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ਅਚਲ AstIfaceRefDType* ਅਚਲ fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(ਸੱਚ) : fromDtp, IfaceRefDType);
        ਜੇ (checkIfaceArgCompat && fromIfp && (ਮੰਜ਼ਿਲ_Dtp != ਮੰਜ਼ਿਲ_Ifp || fromDtp != fromIfp)) {
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ ਮੰਜ਼ਿਲ_Arrayp = VN_CAST(ਮੰਜ਼ਿਲ_Dtp, UnpackArrayDType);
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            ਜੇ (!ਮੰਜ਼ਿਲ_Arrayp || !fromArrayp
                || ਮੰਜ਼ਿਲ_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
                ਵਾਪਸ VCastable::INCOMPATIBLE;
            }
            ਵਾਪਸ computeCastableImp(ਮੰਜ਼ਿਲ_Arrayp->subDTypep(), fromArrayp->subDTypep(), ਖਾਲੀ_ਸੂਚਕ,
                                      checkIfaceArgCompat);
        }
        ਜੇ (!fromIfp || ਮੰਜ਼ਿਲ_Ifp->iface_ਰਾਹੀਂ_Cellp() != fromIfp->iface_ਰਾਹੀਂ_Cellp()) {
            ਜੇ (!checkIfaceArgCompat) ਵਾਪਸ castable;
            ਵਾਪਸ VCastable::INCOMPATIBLE;
        }
        ਅਚਲ ਤਾਰਕਿਕ sameModport = ਮੰਜ਼ਿਲ_Ifp->modportp() == fromIfp->modportp();
        ਜੇ (!checkIfaceArgCompat) {
            ਜੇ (!ਮੰਜ਼ਿਲ_Ifp->modportp() || sameModport) ਵਾਪਸ VCastable::COMPATIBLE;
            ਵਾਪਸ castable;
        }
        ਜੇ (ਮੰਜ਼ਿਲ_Ifp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            ਵਾਪਸ VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        ਜੇ (ਮੰਜ਼ਿਲ_Ifp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            ਵਾਪਸ VCastable::COMPATIBLE;
        }
        ਵਾਪਸ VCastable::INCOMPATIBLE;
    } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ ਮੰਜ਼ਿਲ_Arrayp = VN_CAST(ਮੰਜ਼ਿਲ_Dtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        ਜੇ (!fromArrayp || ਮੰਜ਼ਿਲ_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
            ਵਾਪਸ castable;
        }
        ਅਚਲ VCastable subCastable = computeCastableImp(
            ਮੰਜ਼ਿਲ_Arrayp->subDTypep(), fromArrayp->subDTypep(), ਖਾਲੀ_ਸੂਚਕ, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਮੰਜ਼ਿਲ_Subp = ਮੰਜ਼ਿਲ_Arrayp->subDTypep()->skipRefp();
        ਜੇ (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(ਮੰਜ਼ਿਲ_Subp, UnpackArrayDType) || VN_IS(ਮੰਜ਼ਿਲ_Subp, IfaceRefDType)))) {
            ਵਾਪਸ VCastable::COMPATIBLE;
        }
    }
    ਵਾਪਸ castable;
}

VCastable Ast_ਗੰਢ::computeCastable(ਅਚਲ Ast_ਗੰਢ_DType* ਮੰਜ਼ਿਲ_Dtp, ਅਚਲ Ast_ਗੰਢ_DType* fromDtp,
                                   ਅਚਲ Ast_ਗੰਢ* fromConstp, ਅਚਲ ਤਾਰਕਿਕ checkIfaceArgCompat) {
    ਅਚਲ ਸਵੈਚਾਲਿਤ castable = computeCastableImp(ਮੰਜ਼ਿਲ_Dtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << ਮੰਜ਼ਿਲ_Dtp);
    UINFO(9, "     =?= " << fromDtp);
    ਜੇ (fromConstp) UINFO(9, "     const= " << fromConstp);
    ਵਾਪਸ castable;
}

Ast_ਗੰਢ_DType* Ast_ਗੰਢ::getCommonClassTypep(Ast_ਗੰਢ* node1p, Ast_ਗੰਢ* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    ਜੇ (VN_IS(node1p, Const)) std::ਅਦਲਾ_ਬਦਲੀ(node1p, node2p);
    {
        ਅਚਲ VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        ਜੇ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            ਵਾਪਸ node1p->dtypep();
        ਜੇ (castable == VCastable::DYNAMIC_CLASS) ਵਾਪਸ node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    ਜਦੋਂ (classDtypep1) {
        ਅਚਲ VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        ਜੇ (castable == VCastable::COMPATIBLE) ਵਾਪਸ classDtypep1;
        AstClassExtends* ਅਚਲ extendsp = classDtypep1->classp()->extendsp();
        ਜੇ (!extendsp) ਰੋਕੋ;
        Ast_ਗੰਢ_DType* ਅਚਲ edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->ਬੱਚਾ_DTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

ਵਰਗ VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<ਪੂਰਨ, std::string> m_interned_ਸ਼ਬਦ_Widths;  // Interned widths
    std::map<ਪੂਰਨ, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ਅਚਲ ਤਾਰਕਿਕ m_dump_ਚੌੜਾਈ_Min = v3Global.ਚੌੜਾਈ_Min_Usage() != VWidthMinUsage::MATCHES_ਚੌੜਾਈ;

    ਸਥਿਰ std::string ਮੰਜ਼ਿਲ_Letters(ਮਾਪ_t ਮੁੱਲ, ਤਾਰਕਿਕ ਹੇਠਲਾ_Case = ਝੂਠ) {
        ਅਚਲ ਅੱਖਰ ਅਧਾਰ = ਹੇਠਲਾ_Case ? 'a' : 'A';
        std::string s;
        ਕਰੋ { s += static_cast<ਅੱਖਰ>(ਅਧਾਰ + ਮੁੱਲ % 26); } ਜਦੋਂ (ਮੁੱਲ /= 26);
        ਵਾਪਸ s;
    }

    ਅਚਲ std::string& internConst(ਅਚਲ AstConst& nodep) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_internedConsts.emplace(nodep.ਅੰਕ().ascii(ਝੂਠ), "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ += ਮੰਜ਼ਿਲ_Letters(m_internedConsts.ਮਾਪ() - 1);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਅਚਲ std::string& intern_ਸ਼ਬਦ_ਚੌੜਾਈ(ਪੂਰਨ ਮੁੱਲ) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_interned_ਸ਼ਬਦ_Widths.emplace(ਮੁੱਲ, "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ += ਮੰਜ਼ਿਲ_Letters(m_interned_ਸ਼ਬਦ_Widths.ਮਾਪ() - 1, ਸੱਚ);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    ਅਚਲ std::string& intern_Wide_ਚੌੜਾਈ(ਪੂਰਨ ਮੁੱਲ) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_internedWideWidths.emplace(ਮੁੱਲ, "");
        ਜੇ (pair.ਦੂਜਾ) pair.ਪਹਿਲਾ->ਦੂਜਾ += ਮੰਜ਼ਿਲ_Letters(m_internedWideWidths.ਮਾਪ() - 1);
        ਵਾਪਸ pair.ਪਹਿਲਾ->ਦੂਜਾ;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ਸਥਿਰ ਤਾਰਕਿਕ is_ਪੱਤੀ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਅਚਲ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        ਲਈ (ਅਚਲ VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            ਜੇ (opType != VNTypeInfo::OP_ਅਣਵਰਤਿਆ) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Render operand, return true if not unused
    ਖਾਲੀ render_ਸੰਕਾਰਜ(VNTypeInfo::OpEn opType, ਅਚਲ Ast_ਗੰਢ* ਅਚਲ opp, uint32_t ਡੂੰਘਾਈ) {
        ਚੋਣ (opType) {
        ਹਾਲਤ VNTypeInfo::OP_ਅਣਵਰਤਿਆ:  //
            ਰੋਕੋ;
        ਹਾਲਤ VNTypeInfo::OP_ਵਰਤਿਆ:  //
            m_os << ' ';
            render(opp, ਡੂੰਘਾਈ);
            ਰੋਕੋ;
        ਹਾਲਤ VNTypeInfo::OP_LIST:
            m_os << " [";
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* nodep = opp; nodep; nodep = nodep->nextp()) {
                ਜੇ (nodep != opp) m_os << ", ";
                render(opp, ਡੂੰਘਾਈ);
            }
            m_os << ']';
            ਰੋਕੋ;
        ਹਾਲਤ VNTypeInfo::OP_OPTIONAL:
            ਜੇ (opp) {
                m_os << " ";
                render(opp, ਡੂੰਘਾਈ);
            } ਨਹੀਂਤਾਂ {
                m_os << " nil";
            }
            ਰੋਕੋ;
        }
    }

    // Render the node into the stream.
    ਖਾਲੀ render(ਅਚਲ Ast_ਗੰਢ* nodep, uint32_t ਡੂੰਘਾਈ) {
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            ਜੇ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ਨਹੀਂਤਾਂ ਜੇ (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } ਨਹੀਂਤਾਂ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ਨਹੀਂਤਾਂ ਜੇ (is_ਪੱਤੀ(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } ਨਹੀਂਤਾਂ ਜੇ (ਡੂੰਘਾਈ == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } ਨਹੀਂਤਾਂ {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ਅਚਲ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            render_ਸੰਕਾਰਜ(typeInfo.m_opType[0], nodep->op1p(), ਡੂੰਘਾਈ - 1);
            render_ਸੰਕਾਰਜ(typeInfo.m_opType[1], nodep->op2p(), ਡੂੰਘਾਈ - 1);
            render_ਸੰਕਾਰਜ(typeInfo.m_opType[2], nodep->op3p(), ਡੂੰਘਾਈ - 1);
            render_ਸੰਕਾਰਜ(typeInfo.m_opType[3], nodep->op4p(), ਡੂੰਘਾਈ - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!dtypep) {
            m_os << '?';
        } ਨਹੀਂਤਾਂ ਜੇ (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->ਚੌੜਾਈ();
            ਜੇ (ਚੌੜਾਈ == 1) {
                m_os << '1';
            } ਨਹੀਂਤਾਂ ਜੇ (ਚੌੜਾਈ <= VL_QUADSIZE) {
                m_os << intern_ਸ਼ਬਦ_ਚੌੜਾਈ(ਚੌੜਾਈ);
            } ਨਹੀਂਤਾਂ {
                m_os << intern_Wide_ਚੌੜਾਈ(ਚੌੜਾਈ);
            }
            ਜੇ (m_dump_ਚੌੜਾਈ_Min) {
                m_os << '/';
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Min = nodep->ਚੌੜਾਈ_Min();
                ਜੇ (ਚੌੜਾਈ_Min == 1) {
                    m_os << '1';
                } ਨਹੀਂਤਾਂ ਜੇ (ਚੌੜਾਈ_Min <= VL_QUADSIZE) {
                    m_os << intern_ਸ਼ਬਦ_ਚੌੜਾਈ(ਚੌੜਾਈ_Min);
                } ਨਹੀਂਤਾਂ {
                    m_os << intern_Wide_ਚੌੜਾਈ(ਚੌੜਾਈ_Min);
                }
            }
        }
    }

ਜਨਤਕ:
    VNPatternString(std::ostream& os, ਅਚਲ Ast_ਗੰਢ* nodep, uint32_t ਡੂੰਘਾਈ)
        : m_os{os} {
        render(nodep, ਡੂੰਘਾਈ);
    }
};

std::string Ast_ਗੰਢ_ਵਿਅੰਜਕ::ਨਮੂਨਾ_String(uint32_t ਡੂੰਘਾਈ) ਅਚਲ {
    std::ostringstream oss;
    VNPatternString{oss, ਇਹ, ਡੂੰਘਾਈ};
    ਵਾਪਸ oss.str();
}

//######################################################################
// VNDeleter

ਖਾਲੀ VNDeleter::doDeletes() {
    ਲਈ (Ast_ਗੰਢ* ਅਚਲ nodep : m_deleteps) nodep->delete_ਰੁੱਖ();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast__gen_visitor_defns.h"  // From ./astgen
