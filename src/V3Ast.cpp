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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Broken.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"

#ಅನ್ತರ್ಭಾವ <iomanip>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_ಗಂಟು::s_edit_Cnt_ಕೊನೆಯ = 0;
uint64_t Ast_ಗಂಟು::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು::s_cloneCntGbl = 0;
uint32_t VNUser1_ಒಳ_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2_ಒಳ_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3_ಒಳ_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4_ಒಳ_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent

ತಾರ್ಕಿಕ VNUser1_ಒಳ_Use::s_userBusy = ಸುಳ್ಳು;
ತಾರ್ಕಿಕ VNUser2_ಒಳ_Use::s_userBusy = ಸುಳ್ಳು;
ತಾರ್ಕಿಕ VNUser3_ಒಳ_Use::s_userBusy = ಸುಳ್ಳು;
ತಾರ್ಕಿಕ VNUser4_ಒಳ_Use::s_userBusy = ಸುಳ್ಳು;

ಪೂರ್ಣಾಂಕ Ast_ಗಂಟು_DType::s_unique_ಅಂಕೆ = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::array_ವಿಧಾನ(ಸ್ಥಿರಾಂಕ string& name) {
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < _ENUM_MAX; ++i) {
        ಸ್ಥಿರಾಂಕ VCMethod ವಿಧಾನ{i};
        ಆದರೆ (name == ವಿಧಾನ.ascii()) ಹಿಂತಿರುಗಿ ವಿಧಾನ;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    ಹಿಂತಿರುಗಿ VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*)) ಸ್ಥಿರಾಂಕ {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_USER_TYPE_CHECKS
    ಆದರೆ (ಸ್ಥಿರಾಂಕ uint64_t* ಸ್ಥಿರಾಂಕ uip = std::get_if<uint64_t>(&m_u)) ಹಿಂತಿರುಗಿ "#"s + cvt_ಗಮ್ಯ_Str(*uip);
    ಆದರೆ (ಶೂನ್ಯ* ಸ್ಥಿರಾಂಕ* ಸ್ಥಿರಾಂಕ upp = std::get_if<ಶೂನ್ಯ*>(&m_u)) ಹಿಂತಿರುಗಿ fmtAddrp(*upp);
    ಹಿಂತಿರುಗಿ "";
#ಇಲ್ಲದಿದ್ದರೆ
    // Dumps void* representation
    ಆದರೆ (!m_u.ಮೇಲೆ) ಹಿಂತಿರುಗಿ "";
    ಹಿಂತಿರುಗಿ fmtAddrp(m_u.ಮೇಲೆ);
#ಆದರೆ_ಅಂತ್ಯ
}

//######################################################################
// VNType

ಸ್ಥಿರಾಂಕ VNTypeInfo VNType::s_type_Info_ಕೋಷ್ಟಕ[VNType::ಅಂಕೆ_TYPES()] = {
#ಅನ್ತರ್ಭಾವ "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& ಕ್ರಿಯಾಕಾರಕ<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

ತಾರ್ಕಿಕ VFwdType::is_ಗಂಟು_Compatible(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) ಸ್ಥಿರಾಂಕ {
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* defp = nodep;
    ಆದರೆ (ಸ್ಥಿರಾಂಕ AstTypedef* ಸ್ಥಿರಾಂಕ adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ adefp = VN_CAST(defp, ಗಂಟು_DType))
        defp = adefp->skip_Ref_ಗಮ್ಯ_Non_Refp();
    ಆಯ್ಕೆ (m_e) {
    ಸ್ಥಿತಿ VFwdType::NONE: ಹಿಂತಿರುಗಿ ನಿಜ; ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VFwdType::ENUM: ಹಿಂತಿರುಗಿ VN_IS(defp, EnumDType); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VFwdType::STRUCT: ಹಿಂತಿರುಗಿ VN_IS(defp, StructDType); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VFwdType::UNION: ಹಿಂತಿರುಗಿ VN_IS(defp, UnionDType); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    ಸ್ಥಿತಿ VFwdType::CLASS: ಹಿಂತಿರುಗಿ VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VFwdType::GENERIC_INTERFACE: ಹಿಂತಿರುಗಿ VN_IS(defp, IfaceRefDType); ನಿಲ್ಲಿಸು;
    ಪೂರ್ವನಿಯೋಜಿತ: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ಸ್ಥಿರಾಂಕ std::shared_ptr<ಸ್ಥಿರಾಂಕ string> VSelf_ಸೂಚಕ_ಪಠ್ಯ::s_emptyp = std::make_shared<string>("");
ಸ್ಥಿರಾಂಕ std::shared_ptr<ಸ್ಥಿರಾಂಕ string> VSelf_ಸೂಚಕ_ಪಠ್ಯ::s_thisp = std::make_shared<string>("this");

string VSelf_ಸೂಚಕ_ಪಠ್ಯ::replaceThis(ತಾರ್ಕಿಕ useSelfForThis, ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ) {
    ಹಿಂತಿರುಗಿ useSelfForThis ? VString::replace_ಶಬ್ದ(ಪಠ್ಯ, "this", "vlSelf") : ಪಠ್ಯ;
}

string VSelf_ಸೂಚಕ_ಪಠ್ಯ::protect(ತಾರ್ಕಿಕ useSelfForThis, ತಾರ್ಕಿಕ protect) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ VIdProtect::protect_ಪದಗಳು_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_ಗಂಟು::Ast_ಗಂಟು(VNType t, File_ಸಾಲು* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = ಇದು;  // When made, we're a list of only a single element
    //ಗುಣಲಕ್ಷಣಗಳು
    m_flags.did_ಅಗಲ = ಸುಳ್ಳು;
    m_flags.doing_ಅಗಲ = ಸುಳ್ಳು;
    m_flags.protect = ನಿಜ;
    m_flags.ಬಳಸದ = 0;  // Initializing this avoids a read-modify-write on construction
    edit_ಎಣಿಕೆ_Inc();
}

Ast_ಗಂಟು* Ast_ಗಂಟು::abovep() ಸ್ಥಿರಾಂಕ {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || ಮೊದಲ_Abovep(), ಇದು, "abovep() not allowed when in midlist");
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ firstp = ಮೊದಲ_Abovep() ? ಇದು : m_headtailp;
    ಹಿಂತಿರುಗಿ firstp->backp();
}
Ast_ಗಂಟು* Ast_ಗಂಟು::aboveLoopp() ಸ್ಥಿರಾಂಕ {
    // Returns parent node. Avoid using this, may have performance issues.
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = ಇದು;
    // Backwards over peers (versus parents)
    ಆಗ (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    ಹಿಂತಿರುಗಿ nodep->backp();
}

string Ast_ಗಂಟು::encodeName(ಸ್ಥಿರಾಂಕ string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string ಹೊರಹೋಗು;
    ಹೊರಹೋಗು.reserve(namein.ಗಾತ್ರ_2());
    ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ pos = namein.begin(); pos != namein.ಕೊನೆ(); ++pos) {
        ಆದರೆ ((pos == namein.begin()) ? std::ಅಕ್ಷರವೇ(pos[0])  // digits can't lead identifiers
                                    : std::ಅಕ್ಷರವೇ_ಅಂಕಿಯೇ(pos[0])) {
            ಹೊರಹೋಗು += pos[0];
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pos[0] == '_') {
            ಹೊರಹೋಗು += pos[0];
            ಆದರೆ (pos + 1 == namein.ಕೊನೆ()) ನಿಲ್ಲಿಸು;
            ಆದರೆ (pos[1] == '_') {
                ++pos;
                ಹೊರಹೋಗು += "__05F";  // hex(_) = 0x5F
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            ಹೊರಹೋಗು += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{ಹೊರಹೋಗು};
    ಹಿಂತಿರುಗಿ vname.hashedName();
}

string Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(int64_t ಅಂಕೆ) {
    ಆದರೆ (ಅಂಕೆ < 0) ಹಿಂತಿರುಗಿ "__02D" + cvt_ಗಮ್ಯ_Str(-ಅಂಕೆ);  // 2D=-
    ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Str(ಅಂಕೆ);
}

string Ast_ಗಂಟು::nameProtect() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VIdProtect::protectIf(name(), protect()); }
string Ast_ಗಂಟು::origNameProtect() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ VIdProtect::protectIf(origName(), protect()); }

string Ast_ಗಂಟು::shortName() ಸ್ಥಿರಾಂಕ {
    string pretty = name();
    string::ಗಾತ್ರ_type pos;
    ಆಗ ((pos = pretty.ಕಂಡುಹಿಡಿ("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    ಹಿಂತಿರುಗಿ pretty;
}

string Ast_ಗಂಟು::dedotName(ಸ್ಥಿರಾಂಕ string& namein) {
    string pretty = namein;
    string::ಗಾತ್ರ_type pos;
    ಆಗ ((pos = pretty.ಕಂಡುಹಿಡಿ("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    ಆದರೆ (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ಹಿಂತಿರುಗಿ pretty;
}

string Ast_ಗಂಟು::nameNoArray(ಸ್ಥಿರಾಂಕ string& namein) {
    ಹಿಂತಿರುಗಿ namein.substr(0, namein.ಕಂಡುಹಿಡಿ("__BRA__"));
}

string Ast_ಗಂಟು::vcdName(ಸ್ಥಿರಾಂಕ string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::ಗಾತ್ರ_type pos;
    ಆಗ ((pos = pretty.ಕಂಡುಹಿಡಿ("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    ಆಗ ((pos = pretty.ಕಂಡುಹಿಡಿ('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    ಹಿಂತಿರುಗಿ prettyName(pretty);
}

string Ast_ಗಂಟು::prettyName(ಸ್ಥಿರಾಂಕ string& namein) VL_ಶುದ್ಧ {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.ಉದ್ದಳತೆ());
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* pos = namein.c_str(); *pos;) {
        ಆದರೆ (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            ಮುಂದುವರಿ;
        }
        ಆದರೆ (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                ಅಕ್ಷರ ಮೌಲ್ಯ = 0;
                ಮೌಲ್ಯ += 16
                         * (std::ಅಂಕಿಯೇ(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ಮೌಲ್ಯ
                    += (std::ಅಂಕಿಯೇ(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += ಮೌಲ್ಯ;
                pos += 5;
                ಮುಂದುವರಿ;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    ಆದರೆ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ಆದರೆ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ಹಿಂತಿರುಗಿ pretty;
}

string Ast_ಗಂಟು::vpiName(ಸ್ಥಿರಾಂಕ string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಸ್ಥಾನಾಂತರ = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.ಉದ್ದಳತೆ());
    ತಾರ್ಕಿಕ ಒಳ_Escaped_Ident = ಸುಳ್ಳು;
    ಪೂರ್ಣಾಂಕ ಕೊನೆಯ_Ident = 0;

    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* pos = namein.c_str() + ಸ್ಥಾನಾಂತರ; *pos;) {
        ಅಕ್ಷರ specialChar = 0;
        ಆದರೆ (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pos[0] == '_' && pos[1] == '_') {  // __
            ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__PVT__", 7)) {
                pos += 7;
                ಮುಂದುವರಿ;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (0 == std::ಅಕ್ಷರಸರಣಿ_ಹೋಲಿಕೆ(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                ಅಕ್ಷರ ಮೌಲ್ಯ = 0;
                ಮೌಲ್ಯ += 16
                         * (std::ಅಂಕಿಯೇ(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ಮೌಲ್ಯ
                    += (std::ಅಂಕಿಯೇ(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                ಆದರೆ (ಮೌಲ್ಯ != '_') ಒಳ_Escaped_Ident = ನಿಜ;

                pretty += ಮೌಲ್ಯ;
                pos += 5;
                ಮುಂದುವರಿ;
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        ಆದರೆ (specialChar) {
            ಆದರೆ (ಒಳ_Escaped_Ident && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.ಸೇರಿಸು(ಕೊನೆಯ_Ident, "\\");
                ಒಳ_Escaped_Ident = ಸುಳ್ಳು;
            }

            pretty += specialChar;

            ಆದರೆ (specialChar == ']' || specialChar == '.') {
                ಕೊನೆಯ_Ident = pretty.ಉದ್ದಳತೆ();
                ಒಳ_Escaped_Ident = ಸುಳ್ಳು;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            pretty += pos[0];
            ++pos;
        }
    }
    ಆದರೆ (ಒಳ_Escaped_Ident) {
        pretty += " ";
        pretty.ಸೇರಿಸು(ಕೊನೆಯ_Ident, "\\");
    }
    ಆದರೆ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ಆದರೆ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ಹಿಂತಿರುಗಿ pretty;
}

string Ast_ಗಂಟು::prettyTypeName() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (name() == "") ಹಿಂತಿರುಗಿ typeName();
    ಹಿಂತಿರುಗಿ std::string{typeName()} + " '" + pretty_Name_ಸಂದೇಶ() + "'";
}

//######################################################################
// Insertion

ಶೂನ್ಯ Ast_ಗಂಟು::debug_ವೃಕ್ಷ_Change(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪೂರ್ವಪ್ರತ್ಯಯ, ಪೂರ್ಣಾಂಕ lineno, ತಾರ್ಕಿಕ ಮುಂದಿನ) {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG
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
#ಆದರೆ_ಅಂತ್ಯ
}

ಮಾದರಿ <>
Ast_ಗಂಟು* Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_ವೃಕ್ಷ_Change(nodep, "-addNextThs: ", __LINE__, ಸುಳ್ಳು);
    debug_ವೃಕ್ಷ_Change(newp, "-addNextNew: ", __LINE__, ನಿಜ);
    ಆದರೆ (!nodep)  // verilog.y and lots of other places assume this
        ಹಿಂತಿರುಗಿ newp;

    // Find end of old list
    Ast_ಗಂಟು* oldtailp = nodep;
    ಆದರೆ (oldtailp->m_nextp) {
        ಆದರೆ (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            ಆಗ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newtailp = newp->m_headtailp;
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = ಶೂನ್ಯಸೂಚಕ;  // May be written again as new head
    newp->m_headtailp = ಶೂನ್ಯಸೂಚಕ;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_ಎಣಿಕೆ_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_ವೃಕ್ಷ_Change(nodep, "-addNextOut:", __LINE__, ನಿಜ);
    ಹಿಂತಿರುಗಿ nodep;
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(Ast_ಗಂಟು* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_ವೃಕ್ಷ_Change(ಇದು, "-addHereThs: ", __LINE__, ಸುಳ್ಳು);
    debug_ವೃಕ್ಷ_Change(newp, "-addHereNew: ", __LINE__, ನಿಜ);
    newp->edit_ಎಣಿಕೆ_Inc();

    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldnextp = ಇದು->m_nextp;
    ಇದು->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    ಆದರೆ (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = ಇದು;

    // Head/tail
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldheadtailp = ಇದು->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = ಶೂನ್ಯಸೂಚಕ;  // Not at head any longer
    addlastp->m_headtailp = ಶೂನ್ಯಸೂಚಕ;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    ಆದರೆ (oldheadtailp) {  // else in middle of list, no change
        ಆದರೆ (oldheadtailp == ಇದು) {  // this was one node
            ಇದು->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!oldnextp) {  // this was tail
            ಇದು->m_headtailp = ಶೂನ್ಯಸೂಚಕ;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_ವೃಕ್ಷ_Change(ಇದು, "-addHereOut: ", __LINE__, ನಿಜ);
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಗಣ_Op1p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, ಇದು, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_ವೃಕ್ಷ_Change(ಇದು, "-setOp1pThs: ", __LINE__, ಸುಳ್ಳು);
    debug_ವೃಕ್ಷ_Change(newp, "-setOp1pNew: ", __LINE__, ನಿಜ);
    m_op1p = newp;
    newp->edit_ಎಣಿಕೆ_Inc();
    newp->m_backp = ಇದು;
    debug_ವೃಕ್ಷ_Change(ಇದು, "-setOp1pOut: ", __LINE__, ಸುಳ್ಳು);
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಗಣ_Op2p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, ಇದು, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_ವೃಕ್ಷ_Change(ಇದು, "-setOp2pThs: ", __LINE__, ಸುಳ್ಳು);
    debug_ವೃಕ್ಷ_Change(newp, "-setOp2pNew: ", __LINE__, ನಿಜ);
    m_op2p = newp;
    newp->edit_ಎಣಿಕೆ_Inc();
    newp->m_backp = ಇದು;
    debug_ವೃಕ್ಷ_Change(ಇದು, "-setOp2pOut: ", __LINE__, ಸುಳ್ಳು);
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಗಣ_Op3p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, ಇದು, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_ವೃಕ್ಷ_Change(ಇದು, "-setOp3pThs: ", __LINE__, ಸುಳ್ಳು);
    debug_ವೃಕ್ಷ_Change(newp, "-setOp3pNew: ", __LINE__, ನಿಜ);
    m_op3p = newp;
    newp->edit_ಎಣಿಕೆ_Inc();
    newp->m_backp = ಇದು;
    debug_ವೃಕ್ಷ_Change(ಇದು, "-setOp3pOut: ", __LINE__, ಸುಳ್ಳು);
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಗಣ_Op4p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, ಇದು, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_ವೃಕ್ಷ_Change(ಇದು, "-setOp4pThs: ", __LINE__, ಸುಳ್ಳು);
    debug_ವೃಕ್ಷ_Change(newp, "-setOp4pNew: ", __LINE__, ನಿಜ);
    m_op4p = newp;
    newp->edit_ಎಣಿಕೆ_Inc();
    newp->m_backp = ಇದು;
    debug_ವೃಕ್ಷ_Change(ಇದು, "-setOp4pOut: ", __LINE__, ಸುಳ್ಳು);
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಕೂಡಿಸು_Op1p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಆದರೆ (!m_op1p) {
        op1p(newp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_op1p->ಕೂಡಿಸು_ಮುಂದಿನ(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಕೂಡಿಸು_Op2p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಆದರೆ (!m_op2p) {
        op2p(newp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_op2p->ಕೂಡಿಸು_ಮುಂದಿನ(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಕೂಡಿಸು_Op3p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಆದರೆ (!m_op3p) {
        op3p(newp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_op3p->ಕೂಡಿಸು_ಮುಂದಿನ(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಕೂಡಿಸು_Op4p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಆದರೆ (!m_op4p) {
        op4p(newp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_op4p->ಕೂಡಿಸು_ಮುಂದಿನ(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::prependOp1p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಆದರೆ (!m_op1p) {
        op1p(newp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_op1p->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::prependOp2p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಆದರೆ (!m_op2p) {
        op2p(newp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_op2p->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::prependOp3p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಆದರೆ (!m_op3p) {
        op3p(newp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_op3p->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::prependOp4p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಆದರೆ (!m_op4p) {
        op4p(newp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        m_op4p->ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::replaceWith(Ast_ಗಂಟು* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    ಇದು->unlink_Fr_ಹಿಂಭಾಗ(&repHandle);
    repHandle.relink(newp);
}
ಶೂನ್ಯ Ast_ಗಂಟು::replaceWithKeepDType(Ast_ಗಂಟು* newp) {
    newp->dtypeFrom(ಇದು);
    replaceWith(newp);
}

ಶೂನ್ಯ VNRelinker::dump(std::ostream& str) ಸ್ಥಿರಾಂಕ {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_ಮುಂದಿನ ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_ಗಂಟು* Ast_ಗಂಟು::unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ(VNRelinker* linkerp) {
    debug_ವೃಕ್ಷ_Change(ಇದು, "-unlinkWNextThs: ", __LINE__, ನಿಜ);
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldp = ಇದು;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ಎಣಿಕೆ_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = oldp->m_backp;
    ಆದರೆ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        ಆದರೆ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_ಮುಂದಿನ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ಇಲ್ಲದಿದ್ದರೆ {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ಆದರೆ (backp->m_nextp == oldp) {
        backp->m_nextp = ಶೂನ್ಯಸೂಚಕ;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_ಗಂಟು* oldtailp = backp->m_headtailp;
        ಆದರೆ (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            ಆಗ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op1p == oldp) {
        backp->m_op1p = ಶೂನ್ಯಸೂಚಕ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op2p == oldp) {
        backp->m_op2p = ಶೂನ್ಯಸೂಚಕ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op3p == oldp) {
        backp->m_op3p = ಶೂನ್ಯಸೂಚಕ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op4p == oldp) {
        backp->m_op4p = ಶೂನ್ಯಸೂಚಕ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಇದು->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = ಶೂನ್ಯಸೂಚಕ;
    // Iterator fixup
    ಆದರೆ (oldp->m_iterpp) {
        *(oldp->m_iterpp) = ಶೂನ್ಯಸೂಚಕ;
        oldp->m_iterpp = ಶೂನ್ಯಸೂಚಕ;
    }
    debug_ವೃಕ್ಷ_Change(oldp, "-unlinkWNextOut: ", __LINE__, ನಿಜ);
    ಹಿಂತಿರುಗಿ oldp;
}

Ast_ಗಂಟು* Ast_ಗಂಟು::unlink_Fr_ಹಿಂಭಾಗ(VNRelinker* linkerp) {
    debug_ವೃಕ್ಷ_Change(ಇದು, "-unlinkFrBkThs: ", __LINE__, ನಿಜ);
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldp = ಇದು;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ಎಣಿಕೆ_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = oldp->m_backp;
    ಆದರೆ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        ಆದರೆ (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = ಶೂನ್ಯಸೂಚಕ;
            oldp->m_iterpp = ಶೂನ್ಯಸೂಚಕ;
        }
        ಆದರೆ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_ಮುಂದಿನ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಇದು->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ಆದರೆ (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        ಆದರೆ (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        ಆದರೆ (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಆದರೆ (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಇದು->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        ಆದರೆ (oldp->m_nextp) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    ಆದರೆ (oldp->m_iterpp) {  // Only if no linker, point to next in list
        ಆದರೆ (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = ಶೂನ್ಯಸೂಚಕ;
    }
    // Relink
    oldp->m_nextp = ಶೂನ್ಯಸೂಚಕ;
    oldp->m_backp = ಶೂನ್ಯಸೂಚಕ;
    oldp->m_headtailp = oldp;
    debug_ವೃಕ್ಷ_Change(oldp, "-unlinkFrBkOut: ", __LINE__, ನಿಜ);
    ಹಿಂತಿರುಗಿ oldp;
}

ಶೂನ್ಯ Ast_ಗಂಟು::relink(VNRelinker* linkerp) {
    ಆದರೆ (debug() > 8) {
        UINFO_ಪೂರ್ವಪ್ರತ್ಯಯ(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಇದು;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_ಎಣಿಕೆ_Inc();

    ಆದರೆ (debug() > 8) {
        linkerp->dump(ಹೊರಗೆ);
        ಹೊರಗೆ << ಸಾಲು_ಅಂತ್ಯ;
    }

    // cppcheck-suppress shadowFunction
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = linkerp->m_backp;
    debug_ವೃಕ್ಷ_Change(ಇದು, "-relinkNew: ", __LINE__, ನಿಜ);
    debug_ವೃಕ್ಷ_Change(backp, "-relinkTre: ", __LINE__, ನಿಜ);

    ಆಯ್ಕೆ (linkerp->m_chg) {
    ಸ್ಥಿತಿ VNRelinker::RELINK_ಮುಂದಿನ: backp->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(newp); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); ನಿಲ್ಲಿಸು;
    ಸ್ಥಿತಿ VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); ನಿಲ್ಲಿಸು;
    ಪೂರ್ವನಿಯೋಜಿತ: ಇದು->v3fatalSrc("Relink of node without any link to change."); ನಿಲ್ಲಿಸು;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = ಶೂನ್ಯಸೂಚಕ;
    // Iterator fixup
    ಆದರೆ (linkerp->m_iterpp) {
        // If we're iterating over a next() link, we need to follow links off the
        // NEW node, which is always assumed to be what we are relinking to.
        // This adds a unfortunate hot 8 bytes to every AstNode, but is faster than passing
        // across every function.
        // If anyone has a cleaner way, I'd be grateful.
        newp->m_iterpp = linkerp->m_iterpp;
        *(newp->m_iterpp) = newp;
    }
    // Empty the linker so not used twice accidentally
    linkerp->m_backp = ಶೂನ್ಯಸೂಚಕ;
    debug_ವೃಕ್ಷ_Change(ಇದು, "-relinkOut: ", __LINE__, ನಿಜ);
}

ಶೂನ್ಯ Ast_ಗಂಟು::relinkOneLink(Ast_ಗಂಟು*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_ಗಂಟು* newp) {
    ಆದರೆ (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ oldlistlastp = pointpr->m_headtailp;
        UASSERT_OBJ(!(oldlistlastp->m_nextp && oldlistlastp != pointpr), newp,
                    "Old headtailp tail isn't at the tail");
        // Next links
        newlistlastp->m_nextp = pointpr;
        pointpr->m_backp = newlistlastp;
        // Head/tail
        pointpr->m_headtailp = ಶೂನ್ಯಸೂಚಕ;  // Old head
        newlistlastp->m_headtailp = ಶೂನ್ಯಸೂಚಕ;  // Old tail
        newp->m_headtailp = oldlistlastp;  // Head points to tail
        oldlistlastp->m_headtailp = newp;  // Tail points to head
    }
    pointpr = newp;
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಕೂಡಿಸು_ಇಲ್ಲಿ_This_As_ಮುಂದಿನ(Ast_ಗಂಟು* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(ಇದು->m_backp, ಇದು, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ backp = ಇದು->m_backp;
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newLastp = newp->m_headtailp;
    //
    ಇದು->edit_ಎಣಿಕೆ_Inc();
    // Common linkage
    newLastp->m_nextp = ಇದು;
    ಇದು->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = ಶೂನ್ಯಸೂಚಕ;
    // Linkage dependent on position
    ಆದರೆ (backp && backp->m_nextp == ಇದು) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = ಶೂನ್ಯಸೂಚಕ;
    } ಇಲ್ಲದಿದ್ದರೆ {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        ಆದರೆ (backp) {
            ಆದರೆ (backp->m_op1p == ಇದು) {
                backp->m_op1p = newp;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op2p == ಇದು) {
                backp->m_op2p = newp;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (backp->m_op3p == ಇದು) {
                backp->m_op3p = newp;
            } ಇಲ್ಲದಿದ್ದರೆ {
                UASSERT_OBJ(backp->m_op4p == ಇದು, ಇದು, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ tailp = ಇದು->m_headtailp;
        ಇದು->m_headtailp = ಶೂನ್ಯಸೂಚಕ;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    ಆದರೆ (newLastp->m_iterpp) *(newLastp->m_iterpp) = ಇದು;
    ಆದರೆ (ಇದು->m_iterpp) {
        *(ಇದು->m_iterpp) = newp;
        ಇದು->m_iterpp = ಶೂನ್ಯಸೂಚಕ;
    }
    //
    debug_ವೃಕ್ಷ_Change(ಇದು, "-addHereThisAsNext: ", __LINE__, ನಿಜ);
}

//======================================================================
// Clone

Ast_ಗಂಟು* Ast_ಗಂಟು::clone_ವೃಕ್ಷ_Iter(ತಾರ್ಕಿಕ need_ಶುದ್ಧ) {
    // private: Clone single node and children
    ಆದರೆ (need_ಶುದ್ಧ) purityCheck();
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = ಇದು->clone();
    ಆದರೆ (ಇದು->m_op1p) newp->op1p(ಇದು->m_op1p->clone_ವೃಕ್ಷ_Iter_List(need_ಶುದ್ಧ));
    ಆದರೆ (ಇದು->m_op2p) newp->op2p(ಇದು->m_op2p->clone_ವೃಕ್ಷ_Iter_List(need_ಶುದ್ಧ));
    ಆದರೆ (ಇದು->m_op3p) newp->op3p(ಇದು->m_op3p->clone_ವೃಕ್ಷ_Iter_List(need_ಶುದ್ಧ));
    ಆದರೆ (ಇದು->m_op4p) newp->op4p(ಇದು->m_op4p->clone_ವೃಕ್ಷ_Iter_List(need_ಶುದ್ಧ));
    newp->m_iterpp = ಶೂನ್ಯಸೂಚಕ;
    newp->clonep(ಇದು);  // Save pointers to/from both to simplify relinking.
    ಇದು->clonep(newp);  // Save pointers to/from both to simplify relinking.
    ಹಿಂತಿರುಗಿ newp;
}

Ast_ಗಂಟು* Ast_ಗಂಟು::clone_ವೃಕ್ಷ_Iter_List(ತಾರ್ಕಿಕ need_ಶುದ್ಧ) {
    // private: Clone list of nodes, set m_headtailp
    Ast_ಗಂಟು* newheadp = ಶೂನ್ಯಸೂಚಕ;
    Ast_ಗಂಟು* newtailp = ಶೂನ್ಯಸೂಚಕ;
    // Audited to make sure this is never nullptr
    ಗಾಗಿ (Ast_ಗಂಟು* oldp = ಇದು; oldp; oldp = oldp->m_nextp) {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ newp = oldp->clone_ವೃಕ್ಷ_Iter(need_ಶುದ್ಧ);
        newp->m_headtailp = ಶೂನ್ಯಸೂಚಕ;
        newp->m_backp = newtailp;
        ಆದರೆ (newtailp) newtailp->m_nextp = newp;
        ಆದರೆ (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    ಹಿಂತಿರುಗಿ newheadp;
}

Ast_ಗಂಟು* Ast_ಗಂಟು::clone_ವೃಕ್ಷ(ತಾರ್ಕಿಕ clone_ಮುಂದಿನ_Link, ತಾರ್ಕಿಕ need_ಶುದ್ಧ) {
    debug_ವೃಕ್ಷ_Change(ಇದು, "-cloneThs: ", __LINE__, clone_ಮುಂದಿನ_Link);
    clone_Clear_ವೃಕ್ಷ();
    Ast_ಗಂಟು* newp;
    ಆದರೆ (clone_ಮುಂದಿನ_Link && ಇದು->m_nextp) {
        newp = clone_ವೃಕ್ಷ_Iter_List(need_ಶುದ್ಧ);
    } ಇಲ್ಲದಿದ್ದರೆ {
        newp = clone_ವೃಕ್ಷ_Iter(need_ಶುದ್ಧ);
        newp->m_nextp = ಶೂನ್ಯಸೂಚಕ;
        newp->m_headtailp = newp;
    }
    newp->m_backp = ಶೂನ್ಯಸೂಚಕ;
    newp->clone_Relink_ವೃಕ್ಷ();
    debug_ವೃಕ್ಷ_Change(newp, "-cloneOut: ", __LINE__, ನಿಜ);
    ಹಿಂತಿರುಗಿ newp;
}

ಶೂನ್ಯ Ast_ಗಂಟು::purityCheck() {
    ಆದರೆ (VL_UNLIKELY(!is_ಶುದ್ಧ())) {
        ಇದು->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << ಇದು->warn_ಇನ್ನಷ್ಟು()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

ಶೂನ್ಯ Ast_ಗಂಟು::delete_ಗಂಟು() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_ಎಣಿಕೆ_Inc();
    // Change links of old node so we coredump if used
    ಇದು->m_nextp = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಇದು->m_backp = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಇದು->m_headtailp = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಇದು->m_op1p = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಇದು->m_op2p = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಇದು->m_op3p = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಇದು->m_op4p = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಇದು->m_iterpp = reinterpret_cast<Ast_ಗಂಟು**>(0x1);
    ಆದರೆ (
#ಆದರೆ !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        ನಿಜ
#ಇಲ್ಲದಿದ್ದರೆ
        !v3Global.opt.debugLeak()
#ಆದರೆ_ಅಂತ್ಯ
    ) {
        ಅಳಿಸು ಇದು;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

ಶೂನ್ಯ Ast_ಗಂಟು::delete_ವೃಕ್ಷ_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    ಗಾಗಿ (Ast_ಗಂಟು *nodep = ಇದು, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        ಆದರೆ (nodep->m_op1p) nodep->m_op1p->delete_ವೃಕ್ಷ_Iter();
        ಆದರೆ (nodep->m_op2p) nodep->m_op2p->delete_ವೃಕ್ಷ_Iter();
        ಆದರೆ (nodep->m_op3p) nodep->m_op3p->delete_ವೃಕ್ಷ_Iter();
        ಆದರೆ (nodep->m_op4p) nodep->m_op4p->delete_ವೃಕ್ಷ_Iter();
        nodep->m_nextp = ಶೂನ್ಯಸೂಚಕ;
        nodep->m_backp = ಶೂನ್ಯಸೂಚಕ;
        nodep->delete_ಗಂಟು();
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::delete_ವೃಕ್ಷ() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_ವೃಕ್ಷ_Change(ಇದು, "-delTree:  ", __LINE__, ನಿಜ);
    ಇದು->edit_ಎಣಿಕೆ_Inc();
    // MUST be depth first!
    delete_ವೃಕ್ಷ_Iter();
}

//======================================================================
// Memory checks

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_LEAK_CHECKS
ಶೂನ್ಯ* Ast_ಗಂಟು::ಕ್ರಿಯಾಕಾರಕ ಹೊಸ(ಗಾತ್ರ_t ಗಾತ್ರ_2) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ objp = static_cast<Ast_ಗಂಟು*>(::ಕ್ರಿಯಾಕಾರಕ ಹೊಸ(ಗಾತ್ರ_2));
    V3Broken::ಕೂಡಿಸು_Newed(objp);
    ಹಿಂತಿರುಗಿ objp;
}

ಶೂನ್ಯ Ast_ಗಂಟು::ಕ್ರಿಯಾಕಾರಕ ಅಳಿಸು(ಶೂನ್ಯ* objp, ಗಾತ್ರ_t ಗಾತ್ರ_2) {
    ಆದರೆ (!objp) ಹಿಂತಿರುಗಿ;
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep = static_cast<Ast_ಗಂಟು*>(objp);
    V3Broken::deleted(nodep);
    ::ಕ್ರಿಯಾಕಾರಕ ಅಳಿಸು(objp);
}
#ಆದರೆ_ಅಂತ್ಯ

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_ALLOC_RANDOM_CHECKS
ಶೂನ್ಯ* Ast_ಗಂಟು::ಕ್ರಿಯಾಕಾರಕ ಹೊಸ(ಗಾತ್ರ_t ಗಾತ್ರ_2) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ASTGEN_MAX_ಗಂಟು_ಗಾತ್ರ = []() {
        ಗಾತ್ರ_t max_ಗಾತ್ರ = 0;
        ಗಾಗಿ (ಗಾತ್ರ_t t = 0; t < VNType::ಅಂಕೆ_TYPES(); ++t) {
            max_ಗಾತ್ರ = std::max(max_ಗಾತ್ರ, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        ಹಿಂತಿರುಗಿ max_ಗಾತ್ರ;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr ಗಾತ್ರ_t POOL_ಗಾತ್ರ = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ಸ್ಥಿರ uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    ಆದರೆ (ಪೂರ್ಣಾಂಕ ಬೀಜ = v3Global.opt.debugAllocRandom()) {
        ಸ್ಥಿರ V3Mutex s_mutex;
        ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{s_mutex};
        ಸ್ಥಿರ std::array<ಶೂನ್ಯ*, POOL_ಗಾತ್ರ> s_ಗಂಟು_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(ಗಾತ್ರ_2 <= ASTGEN_MAX_ಗಂಟು_ಗಾತ್ರ, "fix ASTGEN_MAX_NODE_SIZE");
        ಆದರೆ (!s_lfsr) {
            s_lfsr = ಬೀಜ;
            ಆದರೆ (!s_lfsr) s_lfsr = ~s_lfsr;
            ಗಾಗಿ (ಗಾತ್ರ_t i = 0; i < POOL_ಗಾತ್ರ; ++i) {
                s_ಗಂಟು_Pool[i] = ::ಕ್ರಿಯಾಕಾರಕ ಹೊಸ(ASTGEN_MAX_ಗಂಟು_ಗಾತ್ರ + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::ವಿಂಗಡಿಸು(std::begin(s_ಗಂಟು_Pool), std::ಕೊನೆ(s_ಗಂಟು_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಸೂಚ್ಯಂಕ = s_lfsr % POOL_ಗಾತ್ರ;
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ objp = static_cast<Ast_ಗಂಟು*>(s_ಗಂಟು_Pool[ಸೂಚ್ಯಂಕ]);
        s_ಗಂಟು_Pool[ಸೂಚ್ಯಂಕ] = ::ಕ್ರಿಯಾಕಾರಕ ಹೊಸ(ASTGEN_MAX_ಗಂಟು_ಗಾತ್ರ + 64);  // For later new()
        ಹಿಂತಿರುಗಿ objp;
    } ಇಲ್ಲದಿದ್ದರೆ {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ objp = static_cast<Ast_ಗಂಟು*>(::ಕ್ರಿಯಾಕಾರಕ ಹೊಸ(ಗಾತ್ರ_2));
        ಹಿಂತಿರುಗಿ objp;
    }
}
ಶೂನ್ಯ Ast_ಗಂಟು::ಕ್ರಿಯಾಕಾರಕ ಅಳಿಸು(ಶೂನ್ಯ* objp, ಗಾತ್ರ_t ಗಾತ್ರ_2) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (ಶೂನ್ಯ)objp;
    (ಶೂನ್ಯ)ಗಾತ್ರ_2;
}
#ಆದರೆ_ಅಂತ್ಯ

//======================================================================
// Iterators

ಶೂನ್ಯ Ast_ಗಂಟು::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ಆದರೆ (m_op1p) m_op1p->iterate_And_ಮುಂದಿನ(v);
    ಆದರೆ (m_op2p) m_op2p->iterate_And_ಮುಂದಿನ(v);
    ಆದರೆ (m_op3p) m_op3p->iterate_And_ಮುಂದಿನ(v);
    ಆದರೆ (m_op4p) m_op4p->iterate_And_ಮುಂದಿನ(v);
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ಆದರೆ (m_op1p) m_op1p->iterate_And_ಮುಂದಿನ_Const(v);
    ಆದರೆ (m_op2p) m_op2p->iterate_And_ಮುಂದಿನ_Const(v);
    ಆದರೆ (m_op3p) m_op3p->iterate_And_ಮುಂದಿನ_Const(v);
    ಆದರೆ (m_op4p) m_op4p->iterate_And_ಮುಂದಿನ_Const(v);
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterate_And_ಮುಂದಿನ(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_ಗಂಟು* nodep = ಇದು;
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#ಆದರೆ_ಅಂತ್ಯ
    // cppcheck-suppress knownConditionTrueFalse
    ಆದರೆ (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    ಆಗ (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        ಆದರೆ (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_ಗಂಟು* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        ಆದರೆ (!niterp) ಹಿಂತಿರುಗಿ;  // Perhaps node deleted inside accept
        niterp->m_iterpp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } ಇಲ್ಲದಿದ್ದರೆ {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_ಗಂಟು* nodep = ಇದು;
    ಆಗ (nodep->m_nextp) nodep = nodep->m_nextp;
    ಆಗ (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        ಆದರೆ (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep = ಶೂನ್ಯಸೂಚಕ;
        }  // else: backp points up the tree.
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    ಆದರೆ (m_op1p) m_op1p->iterateListBackwardsConst(v);
    ಆದರೆ (m_op2p) m_op2p->iterateListBackwardsConst(v);
    ಆದರೆ (m_op3p) m_op3p->iterateListBackwardsConst(v);
    ಆದರೆ (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterate_And_ಮುಂದಿನ_Const(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_ಗಂಟು* nodep = ಇದು;
    ಮಾಡು {
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } ಆಗ (nodep);
}

Ast_ಗಂಟು* Ast_ಗಂಟು::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_ಗಂಟು* nodep = ಇದು;  // Note "this" may point to bogus point later in this function
    ಆದರೆ (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ಸ್ಥಿರಾಂಕ tempp = ಹೊಸ AstBegin{nodep->fileline(), "[EditWrapper]", nodep, ಸುಳ್ಳು};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
        VL_DO_DANGLING(tempp->delete_ವೃಕ್ಷ(), tempp);
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_ಗಂಟು** nextnodepp = ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (ಇದು->m_backp->m_op1p == ಇದು) {
            nextnodepp = &(ಇದು->m_backp->m_op1p);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಇದು->m_backp->m_op2p == ಇದು) {
            nextnodepp = &(ಇದು->m_backp->m_op2p);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಇದು->m_backp->m_op3p == ಇದು) {
            nextnodepp = &(ಇದು->m_backp->m_op3p);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಇದು->m_backp->m_op4p == ಇದು) {
            nextnodepp = &(ಇದು->m_backp->m_op4p);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಇದು->m_backp->m_nextp == ಇದು) {
            nextnodepp = &(ಇದು->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, ಇದು, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    ಹಿಂತಿರುಗಿ nodep;
}

//======================================================================

ಶೂನ್ಯ Ast_ಗಂಟು::clone_Relink_ವೃಕ್ಷ() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    ಗಾಗಿ (Ast_ಗಂಟು* nodep = ಇದು; nodep; nodep = nodep->m_nextp) {
        ಆದರೆ (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        ಆದರೆ (nodep->m_op1p) nodep->m_op1p->clone_Relink_ವೃಕ್ಷ();
        ಆದರೆ (nodep->m_op2p) nodep->m_op2p->clone_Relink_ವೃಕ್ಷ();
        ಆದರೆ (nodep->m_op3p) nodep->m_op3p->clone_Relink_ವೃಕ್ಷ();
        ಆದರೆ (nodep->m_op4p) nodep->m_op4p->clone_Relink_ವೃಕ್ಷ();
    }
}

//======================================================================
// Comparison

ತಾರ್ಕಿಕ Ast_ಗಂಟು::gate_ವೃಕ್ಷ_Iter() ಸ್ಥಿರಾಂಕ {
    // private: Return true if the two trees are identical
    ಆದರೆ (!isGateOptimizable()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (m_op1p && !m_op1p->gate_ವೃಕ್ಷ_Iter()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (m_op2p && !m_op2p->gate_ವೃಕ್ಷ_Iter()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (m_op3p && !m_op3p->gate_ವೃಕ್ಷ_Iter()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (m_op4p && !m_op4p->gate_ವೃಕ್ಷ_Iter()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

ತಾರ್ಕಿಕ Ast_ಗಂಟು::same_ವೃಕ್ಷ_Iter(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node1p, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* node2p, ತಾರ್ಕಿಕ ign_ಮುಂದಿನ,
                           ತಾರ್ಕಿಕ gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    ಆಗ (ನಿಜ) {
        ಆದರೆ (!node1p && !node2p) ಹಿಂತಿರುಗಿ ನಿಜ;
        ಆದರೆ (!node1p || !node2p) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (node1p->type() != node2p->type()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        ಆದರೆ (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!node1p->same_ಗಂಟು(node2p) || (gateOnly && !node1p->isGateOptimizable())) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (!same_ವೃಕ್ಷ_Iter(node1p->m_op1p, node2p->m_op1p, ಸುಳ್ಳು, gateOnly)
            || !same_ವೃಕ್ಷ_Iter(node1p->m_op2p, node2p->m_op2p, ಸುಳ್ಳು, gateOnly)
            || !same_ವೃಕ್ಷ_Iter(node1p->m_op3p, node2p->m_op3p, ಸುಳ್ಳು, gateOnly)
            || !same_ವೃಕ್ಷ_Iter(node1p->m_op4p, node2p->m_op4p, ಸುಳ್ಳು, gateOnly)) {
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಆದರೆ (ign_ಮುಂದಿನ) ಹಿಂತಿರುಗಿ ನಿಜ;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

ಶೂನ್ಯ Ast_ಗಂಟು::check_ವೃಕ್ಷ_Iter(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* prevBackp) ಸ್ಥಿರಾಂಕ VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == ಇದು->backp(), ಇದು, "Back node inconsistent");
    ಸ್ಥಿರಾಂಕ VNTypeInfo& typeInfo = VNType::typeInfo(ಇದು->type());
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 1; i <= 4; i++) {
        Ast_ಗಂಟು* nodep = ಶೂನ್ಯಸೂಚಕ;
        ಆಯ್ಕೆ (i) {
        ಸ್ಥಿತಿ 1: nodep = op1p(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 2: nodep = op2p(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 3: nodep = op3p(); ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ 4: nodep = op4p(); ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: ಇದು->v3fatalSrc("Bad case"); ನಿಲ್ಲಿಸು;
        }
        // cppcheck-suppress danglingTempReference
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* opName = typeInfo.m_opNamep[i - 1];
        ಆಯ್ಕೆ (typeInfo.m_opType[i - 1]) {
        ಸ್ಥಿತಿ VNTypeInfo::OP_ಬಳಸದ:
            UASSERT_OBJ(!nodep, ಇದು, typeInfo.m_namep << " must not use " << opName << "()");
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNTypeInfo::OP_ಬಳಸಿದ:
            UASSERT_OBJ(nodep, ಇದು,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), ಇದು,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_ವೃಕ್ಷ_Iter(ಇದು);
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNTypeInfo::OP_LIST:
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ headp = nodep) {
                // cppcheck-suppress shadowFunction
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* backp = ಇದು;
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* tailp;
                ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* opp = headp;
                ಮಾಡು {
                    opp->check_ವೃಕ್ಷ_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } ಆಗ (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNTypeInfo::OP_OPTIONAL:
            ಆದರೆ (nodep) {
                UASSERT_OBJ(!nodep->nextp(), ಇದು,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_ವೃಕ್ಷ_Iter(ಇದು);
            }
            ನಿಲ್ಲಿಸು;
        ಪೂರ್ವನಿಯೋಜಿತ: ಇದು->v3fatalSrc("Bad case"); ನಿಲ್ಲಿಸು;
        }
    }
    ಆದರೆ (v3Global.opt.debug_ಅಗಲ() && v3Global.ಅಗಲ_Min_Usage() == VWidthMinUsage::VERILOG_ಅಗಲ) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ exprp = VN_CAST(ಇದು, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ whyp = exprp->ಅಗಲ_Mismatch();
            ಆದರೆ (whyp) {
                ಸ್ವಯಂಚಾಲಿತ dtypeStr = [](ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    ಹಿಂತಿರುಗಿ ss.str();
                };
                ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Uniop* ಸ್ಥಿರಾಂಕ uniopp = VN_CAST(exprp, ಗಂಟು_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Biop* ಸ್ಥಿರಾಂಕ biopp = VN_CAST(exprp, ಗಂಟು_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } ಇಲ್ಲದಿದ್ದರೆ {
                    UASSERT_OBJ(ಸುಳ್ಳು, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
ಅಕ್ಷರ* Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Json_Gdb(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
    ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodep_ಹರಿವು;
    nodep->dump_ವೃಕ್ಷ_Json(nodep_ಹರಿವು);
    ಸ್ಥಿರಾಂಕ std::string str = nodep_ಹರಿವು.rdbuf()->str();
    ಹಿಂತಿರುಗಿ strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
ಅಕ್ಷರ* Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Json_Gdb(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* str) { ಹಿಂತಿರುಗಿ strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
ಅಕ್ಷರ* Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Json_Gdb(intptr_t nodep) {
    ಆದರೆ (!nodep) ಹಿಂತಿರುಗಿ strdup("{\"addr\":\"NULL\"}\n");
    ಹಿಂತಿರುಗಿ dump_ವೃಕ್ಷ_Json_Gdb(reinterpret_cast<ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::dumpGdb(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {  // For GDB only  // LCOV_EXCL_START
    ಆದರೆ (!nodep) {
        ಹೊರಗೆ << "<nullptr>" << ಸಾಲು_ಅಂತ್ಯ;
        ಹಿಂತಿರುಗಿ;
    }
    nodep->dump_Gdb_ಶೀರ್ಷಿಕೆ();
    ಹೊರಗೆ << "  ";
    nodep->dump(ಹೊರಗೆ);
    ಹೊರಗೆ << ಸಾಲು_ಅಂತ್ಯ;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::dump_Gdb_ಶೀರ್ಷಿಕೆ() ಸ್ಥಿರಾಂಕ {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(ಹೊರಗೆ);
    ಹೊರಗೆ << "  Fileline = " << fileline() << ಸಾಲು_ಅಂತ್ಯ;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Gdb(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {  // For GDB only  // LCOV_EXCL_START
    ಆದರೆ (!nodep) {
        ಹೊರಗೆ << "<nullptr>" << ಸಾಲು_ಅಂತ್ಯ;
        ಹಿಂತಿರುಗಿ;
    }
    nodep->dump_Gdb_ಶೀರ್ಷಿಕೆ();
    nodep->dump_ವೃಕ್ಷ(ಹೊರಗೆ);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_File_Gdb(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep,  // LCOV_EXCL_START
                              ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* filenamep) {  // For GDB only
    ಆದರೆ (!nodep) {
        ಹೊರಗೆ << "<nullptr>" << ಸಾಲು_ಅಂತ್ಯ;
        ಹಿಂತಿರುಗಿ;
    }
    ಸ್ಥಿರಾಂಕ string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_ವೃಕ್ಷ_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::checkIter() ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (m_iterpp) {
        dumpPtrs(ಹೊರಗೆ);
        // Perhaps something forgot to clear m_iterpp?
        ಇದು->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::dumpPtrs(std::ostream& os) ಸ್ಥಿರಾಂಕ {
    os << "This=" << typeName() << " " << cvt_ಗಮ್ಯ_Hex(ಇದು);
    os << " back=" << cvt_ಗಮ್ಯ_Hex(backp());
    ಆದರೆ (nextp()) os << " next=" << cvt_ಗಮ್ಯ_Hex(nextp());
    ಆದರೆ (m_headtailp == ಇದು) {
        os << " headtail=this";
    } ಇಲ್ಲದಿದ್ದರೆ {
        os << " headtail=" << cvt_ಗಮ್ಯ_Hex(m_headtailp);
    }
    ಆದರೆ (op1p()) os << " op1p=" << cvt_ಗಮ್ಯ_Hex(op1p());
    ಆದರೆ (op2p()) os << " op2p=" << cvt_ಗಮ್ಯ_Hex(op2p());
    ಆದರೆ (op3p()) os << " op3p=" << cvt_ಗಮ್ಯ_Hex(op3p());
    ಆದರೆ (op4p()) os << " op4p=" << cvt_ಗಮ್ಯ_Hex(op4p());
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ dumpUser = [&os](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪೂರ್ವಪ್ರತ್ಯಯ, ಸ್ಥಿರಾಂಕ VNUser& user) {
        ಸ್ಥಿರಾಂಕ std::string s
            = user.dumpStr([](ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* p) -> std::string { ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Hex(p); });
        ಆದರೆ (!s.ಖಾಲಿಗಳು()) os << ಪೂರ್ವಪ್ರತ್ಯಯ << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    ಆದರೆ (m_iterpp) {
        os << " iterpp=" << cvt_ಗಮ್ಯ_Hex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ(std::ostream& os, ಸ್ಥಿರಾಂಕ string& indent, ಪೂರ್ಣಾಂಕ max_ಆಳ) ಸ್ಥಿರಾಂಕ {
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << ಇದು << '\n';
    ಆದರೆ (VN_DELETED(ಇದು)) ಹಿಂತಿರುಗಿ;
    ಆದರೆ (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    ಆದರೆ (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvt_ಗಮ್ಯ_Hex(ಇದು)
           << ಸಾಲು_ಅಂತ್ಯ;  // endl intentional to do flush
        ಹಿಂತಿರುಗಿ;
    }  // LCOV_EXCL_STOP
    ಆದರೆ (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    ಆದರೆ (max_ಆಳ == 1) {
        ಆದರೆ (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ(os, indent + "1:", max_ಆಳ - 1);
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ(os, indent + "2:", max_ಆಳ - 1);
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ(os, indent + "3:", max_ಆಳ - 1);
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ(os, indent + "4:", max_ಆಳ - 1);
        }
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_And_ಮುಂದಿನ(std::ostream& os, ಸ್ಥಿರಾಂಕ string& indent, ಪೂರ್ಣಾಂಕ max_ಆಳ) ಸ್ಥಿರಾಂಕ {
    // Audited to make sure this is never nullptr
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = ಇದು; nodep; nodep = nodep->nextp()) {
        nodep->dump_ವೃಕ್ಷ(os, indent, max_ಆಳ);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_File(ಸ್ಥಿರಾಂಕ string& filename, ತಾರ್ಕಿಕ doDump) {
    // Not const function as calls checkTree
    ಆದರೆ (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            ಆದರೆ (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_ಎಣಿಕೆ_ಕೊನೆಯ();
            *logsp << "> to <e" << std::dec << edit_ಎಣಿಕೆ_Gbl() << ">\n";
            ಆದರೆ (edit_ಎಣಿಕೆ_Gbl() == edit_ಎಣಿಕೆ_ಕೊನೆಯ() && ::dump_ವೃಕ್ಷ_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } ಇಲ್ಲದಿದ್ದರೆ {
                dump_ವೃಕ್ಷ(*logsp);
                edit_ಎಣಿಕೆ_ಗಣ_ಕೊನೆಯ();  // Next dump can indicate start from here
            }
        }
    }
}

ಸ್ಥಿರ ಶೂನ್ಯ drawChildren(std::ostream& os, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* thisp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* childp,
                         ಸ್ಥಿರಾಂಕ std::string& ಮಗು_Name) {
    ಆದರೆ (childp) {
        os << "\tn" << cvt_ಗಮ್ಯ_Hex(thisp) << " -> n" << cvt_ಗಮ್ಯ_Hex(childp) << " [" << "label=\""
           << ಮಗು_Name << "\" color=red];\n";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ_Dot(os);
            ಆದರೆ (nodep->nextp()) {
                os << "\tn" << cvt_ಗಮ್ಯ_Hex(nodep) << " -> n" << cvt_ಗಮ್ಯ_Hex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvt_ಗಮ್ಯ_Hex(nodep) << ", n" << cvt_ಗಮ್ಯ_Hex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Dot(std::ostream& os) ಸ್ಥಿರಾಂಕ {
    os << "\tn" << cvt_ಗಮ್ಯ_Hex(ಇದು) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, ಇದು, m_op1p, "op1");
    drawChildren(os, ಇದು, m_op2p, "op2");
    drawChildren(os, ಇದು, m_op3p, "op3");
    drawChildren(os, ಇದು, m_op4p, "op4");
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Json_File(ಸ್ಥಿರಾಂಕ string& filename, ತಾರ್ಕಿಕ doDump) {
    ಆದರೆ (!doDump) ಹಿಂತಿರುಗಿ;
    UINFO(2, "Dumping " << filename);
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ಆದರೆ (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_ವೃಕ್ಷ_Json(*treejsonp);
    *treejsonp << '\n';
}

ಶೂನ್ಯ Ast_ಗಂಟು::dumpJsonMetaFileGdb(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* filename) { dumpJsonMetaFile(filename); }
ಶೂನ್ಯ Ast_ಗಂಟು::dumpJsonMetaFile(ಸ್ಥಿರಾಂಕ string& filename) {
    UINFO(2, "Dumping " << filename);
    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ಆದರೆ (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_ಸಾಲು::file_Name_ಅಂಕೆ_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptr_ಹೆಸರುಗಳು_Dump_Json(*treejsonp);
    *treejsonp << "}\n";
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Dot_File(ಸ್ಥಿರಾಂಕ string& filename, ತಾರ್ಕಿಕ doDump) {
    ಆದರೆ (doDump) {
        UINFO(2, "Dumping " << filename);
        ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        ಆದರೆ (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_ವೃಕ್ಷ_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_ಗಂಟು::instanceStr() ಸ್ಥಿರಾಂಕ {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr ಚಿಹ್ನೆರಹಿತ maxIterations = 10000;
    ಚಿಹ್ನೆರಹಿತ iter_ಎಣಿಕೆ = 0;
    // Walk 'backp' chain
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* currp = ಇದು; currp; currp = currp->backp(), ++iter_ಎಣಿಕೆ) {
        ಆದರೆ (VL_UNCOVERABLE(iter_ಎಣಿಕೆ >= maxIterations)) ಹಿಂತಿರುಗಿ "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstScope* ಸ್ಥಿರಾಂಕ scopep = VN_CAST(currp, Scope)) {
            ಹಿಂತಿರುಗಿ scopep->is_ತುದಿ() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstModule* ಸ್ಥಿರಾಂಕ modp = VN_CAST(currp, Module)) {
            ಸ್ಥಿರಾಂಕ string instanceName = modp->someInstanceName();
            ಹಿಂತಿರುಗಿ instanceName.ಖಾಲಿಗಳು() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    ಹಿಂತಿರುಗಿ "";
}
ಶೂನ್ಯ Ast_ಗಂಟು::v3error_ಕೊನೆ(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ VL_RELEASE(V3Error::s().m_mutex) {
    ಆದರೆ (!m_fileline) {
        V3Error::v3error_ಕೊನೆ(str, "", ಶೂನ್ಯಸೂಚಕ);
    } ಇಲ್ಲದಿದ್ದರೆ {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ಸ್ಥಿರಾಂಕ string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().error_ಸಂಕೇತ()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        ಆದರೆ (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_ಗಂಟು*>(ಇದು)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3error_ಕೊನೆ(nsstr, instanceStrExtra);
    }
}
ಶೂನ್ಯ Ast_ಗಂಟು::v3error_ಕೊನೆ_Fatal(ಸ್ಥಿರಾಂಕ std::ostringstream& str) ಸ್ಥಿರಾಂಕ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_ಕೊನೆ(str);
    ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

ಶೂನ್ಯ Ast_ಗಂಟು::dtypeChgSigned(ತಾರ್ಕಿಕ flag) {
    UASSERT_OBJ(dtypep(), ಇದು, "No dtype when changing to (un)signed");
    dtype_Chg_ಅಗಲ_Signed(dtypep()->ಅಗಲ(), dtypep()->ಅಗಲ_Min(), VSigning::fromBool(flag));
}
ಶೂನ್ಯ Ast_ಗಂಟು::dtype_Chg_ಅಗಲ(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min) {
    UASSERT_OBJ(dtypep(), ಇದು, "No dtype when changing width");
    dtype_Chg_ಅಗಲ_Signed(ಅಗಲ, ಅಗಲ_Min, dtypep()->numeric());
}

ಶೂನ್ಯ Ast_ಗಂಟು::dtype_Chg_ಅಗಲ_Signed(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) {
    UASSERT_OBJ(dtypep(), ಇದು, "No dtype when changing width");
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ enumDTypep = dtypep()->skip_Ref_ಗಮ್ಯ_Enump();
    ಆದರೆ (ಅಗಲ == dtypep()->ಅಗಲ() && ಅಗಲ_Min == dtypep()->ಅಗಲ_Min()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        ಹಿಂತಿರುಗಿ;  // Correct already
    }
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtype_ಗಣ_ಬಿಟ್_Or_Logic_Unsized(ಅಗಲ, ಅಗಲ_Min, numeric, dtypep->isFourstate());
}

Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_Basic_DType(VBasicDTypeKwd kwd) ಸ್ಥಿರಾಂಕ {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Basic_DType(fileline(), kwd);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Logic_ಬಿಟ್_DType(fileline(), VBasicDTypeKwd::ಬಿಟ್,
                                                             ಅಗಲ, ಅಗಲ_Min, numeric);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_Logic_DType(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Logic_ಬಿಟ್_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             ಅಗಲ, ಅಗಲ_Min, numeric);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_Logic2_State_DType(ಪೂರ್ಣಾಂಕ ಅಗಲ, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min, VSigning numeric) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Logic_ಬಿಟ್_DType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, ಅಗಲ, ಅಗಲ_Min, numeric);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_Logic_Range_DType(ಸ್ಥಿರಾಂಕ VNumRange& range, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min,
                                           VSigning numeric) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Logic_ಬಿಟ್_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, ಅಗಲ_Min, numeric);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_ಬಿಟ್_Range_DType(ಸ್ಥಿರಾಂಕ VNumRange& range, ಪೂರ್ಣಾಂಕ ಅಗಲ_Min,
                                         VSigning numeric) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Logic_ಬಿಟ್_DType(fileline(), VBasicDTypeKwd::ಬಿಟ್,
                                                             range, ಅಗಲ_Min, numeric);
}
AstBasicDType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_ಸೇರಿಸು_Same_DType(AstBasicDType* nodep) {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_ಸೇರಿಸು_Same_DType(nodep);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_Constraint_Ref_DType() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Constraint_Ref_DType(fileline());
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_ಖಾಲಿಗಳು_ಸರತಿ_DType() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_ಖಾಲಿಗಳು_ಸರತಿ_DType(fileline());
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_ಸರತಿ_ಸೂಚ್ಯಂಕ_DType() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_ಸರತಿ_ಸೂಚ್ಯಂಕ_DType(fileline());
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_ಹರಿವು_DType() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_ಹರಿವು_DType(fileline());
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::ಕಂಡುಹಿಡಿ_Void_DType() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ v3Global.rootp()->typeTablep()->ಕಂಡುಹಿಡಿ_Void_DType(fileline());
}

ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* compute_Castable_ತಳ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* nodep) {
    ಆಗ (ನಿಜ) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstPackArrayDType* ಸ್ಥಿರಾಂಕ packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            ಮುಂದುವರಿ;
        }
        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ refp = nodep->skip_Ref_ಗಮ್ಯ_Enump()) {
            ಆದರೆ (refp != nodep) {
                nodep = refp;
                ಮುಂದುವರಿ;
            }
        }
        ಹಿಂತಿರುಗಿ nodep;
    }
}

ಸ್ಥಿರ VCastable computeCastableImp(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಗಮ್ಯ_Dtp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* fromDtp,
                                    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromConstp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ checkIfaceArgCompat) {
    ಸ್ಥಿರಾಂಕ VCastable castable = VCastable::UNSUPPORTED;
    ಗಮ್ಯ_Dtp = ಗಮ್ಯ_Dtp->skip_Ref_ಗಮ್ಯ_Enump();
    fromDtp = fromDtp->skip_Ref_ಗಮ್ಯ_Enump();
    ಆದರೆ (ಗಮ್ಯ_Dtp == fromDtp) ಹಿಂತಿರುಗಿ VCastable::SAMEISH;
    ಆದರೆ (ಗಮ್ಯ_Dtp->similarDType(fromDtp)) ಹಿಂತಿರುಗಿ VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* from_ತಳ_Dtp = compute_Castable_ತಳ(fromDtp);

    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ fromNumericable = VN_IS(from_ತಳ_Dtp, BasicDType) || VN_IS(from_ತಳ_Dtp, EnumDType)
                                 || VN_IS(from_ತಳ_Dtp, ಹರಿವು_DType)
                                 || VN_IS(from_ತಳ_Dtp, ಗಂಟು_UOr_Struct_DType);

    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಗಮ್ಯ_ತಳ_Dtp = compute_Castable_ತಳ(ಗಮ್ಯ_Dtp);
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಗಮ್ಯ_Numericable
        = VN_IS(ಗಮ್ಯ_ತಳ_Dtp, BasicDType) || VN_IS(ಗಮ್ಯ_ತಳ_Dtp, ಗಂಟು_UOr_Struct_DType);

    ಆದರೆ (ಗಮ್ಯ_ತಳ_Dtp == from_ತಳ_Dtp) ಹಿಂತಿರುಗಿ VCastable::COMPATIBLE;

    ಆದರೆ (ಗಮ್ಯ_Numericable) {
        ಆದರೆ (fromNumericable) ಹಿಂತಿರುಗಿ VCastable::COMPATIBLE;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಗಮ್ಯ_ತಳ_Dtp, EnumDType)) {
        ಆದರೆ (VN_IS(from_ತಳ_Dtp, EnumDType) && ಗಮ್ಯ_Dtp->same_ವೃಕ್ಷ(fromDtp))
            ಹಿಂತಿರುಗಿ VCastable::ENUM_IMPLICIT;
        ಆದರೆ (fromNumericable) ಹಿಂತಿರುಗಿ VCastable::ENUM_EXPLICIT;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಗಮ್ಯ_Dtp, ಸರತಿ_DType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, ಹರಿವು_DType))) {
        ಹಿಂತಿರುಗಿ VCastable::COMPATIBLE;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಗಮ್ಯ_Dtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        ಆದರೆ (fromConstp->isNull()) ಹಿಂತಿರುಗಿ VCastable::COMPATIBLE;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VN_IS(ಗಮ್ಯ_Dtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Classp = VN_AS(ಗಮ್ಯ_Dtp, ClassRefDType)->classp();
        ಸ್ಥಿರಾಂಕ AstClass* ಸ್ಥಿರಾಂಕ fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ downcast = AstClass::isClassExtendedFrom(ಗಮ್ಯ_Classp, fromClassp);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ upcast = AstClass::isClassExtendedFrom(fromClassp, ಗಮ್ಯ_Classp);
        ಆದರೆ (upcast) ಹಿಂತಿರುಗಿ VCastable::COMPATIBLE;
        ಆದರೆ (downcast) ಹಿಂತಿರುಗಿ VCastable::DYNAMIC_CLASS;
        ಹಿಂತಿರುಗಿ VCastable::INCOMPATIBLE;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Ifp
               = VN_CAST(checkIfaceArgCompat ? ಗಮ್ಯ_Dtp->elemDTypep(ನಿಜ) : ಗಮ್ಯ_Dtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ಸ್ಥಿರಾಂಕ AstIfaceRefDType* ಸ್ಥಿರಾಂಕ fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(ನಿಜ) : fromDtp, IfaceRefDType);
        ಆದರೆ (checkIfaceArgCompat && fromIfp && (ಗಮ್ಯ_Dtp != ಗಮ್ಯ_Ifp || fromDtp != fromIfp)) {
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Arrayp = VN_CAST(ಗಮ್ಯ_Dtp, UnpackArrayDType);
            ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            ಆದರೆ (!ಗಮ್ಯ_Arrayp || !fromArrayp
                || ಗಮ್ಯ_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
                ಹಿಂತಿರುಗಿ VCastable::INCOMPATIBLE;
            }
            ಹಿಂತಿರುಗಿ computeCastableImp(ಗಮ್ಯ_Arrayp->subDTypep(), fromArrayp->subDTypep(), ಶೂನ್ಯಸೂಚಕ,
                                      checkIfaceArgCompat);
        }
        ಆದರೆ (!fromIfp || ಗಮ್ಯ_Ifp->iface_ಮೂಲಕ_Cellp() != fromIfp->iface_ಮೂಲಕ_Cellp()) {
            ಆದರೆ (!checkIfaceArgCompat) ಹಿಂತಿರುಗಿ castable;
            ಹಿಂತಿರುಗಿ VCastable::INCOMPATIBLE;
        }
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ sameModport = ಗಮ್ಯ_Ifp->modportp() == fromIfp->modportp();
        ಆದರೆ (!checkIfaceArgCompat) {
            ಆದರೆ (!ಗಮ್ಯ_Ifp->modportp() || sameModport) ಹಿಂತಿರುಗಿ VCastable::COMPATIBLE;
            ಹಿಂತಿರುಗಿ castable;
        }
        ಆದರೆ (ಗಮ್ಯ_Ifp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            ಹಿಂತಿರುಗಿ VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        ಆದರೆ (ಗಮ್ಯ_Ifp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            ಹಿಂತಿರುಗಿ VCastable::COMPATIBLE;
        }
        ಹಿಂತಿರುಗಿ VCastable::INCOMPATIBLE;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Arrayp = VN_CAST(ಗಮ್ಯ_Dtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ಸ್ಥಿರಾಂಕ AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        ಆದರೆ (!fromArrayp || ಗಮ್ಯ_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
            ಹಿಂತಿರುಗಿ castable;
        }
        ಸ್ಥಿರಾಂಕ VCastable subCastable = computeCastableImp(
            ಗಮ್ಯ_Arrayp->subDTypep(), fromArrayp->subDTypep(), ಶೂನ್ಯಸೂಚಕ, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಗಮ್ಯ_Subp = ಗಮ್ಯ_Arrayp->subDTypep()->skipRefp();
        ಆದರೆ (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(ಗಮ್ಯ_Subp, UnpackArrayDType) || VN_IS(ಗಮ್ಯ_Subp, IfaceRefDType)))) {
            ಹಿಂತಿರುಗಿ VCastable::COMPATIBLE;
        }
    }
    ಹಿಂತಿರುಗಿ castable;
}

VCastable Ast_ಗಂಟು::computeCastable(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಗಮ್ಯ_Dtp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* fromDtp,
                                   ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* fromConstp, ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ checkIfaceArgCompat) {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ castable = computeCastableImp(ಗಮ್ಯ_Dtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << ಗಮ್ಯ_Dtp);
    UINFO(9, "     =?= " << fromDtp);
    ಆದರೆ (fromConstp) UINFO(9, "     const= " << fromConstp);
    ಹಿಂತಿರುಗಿ castable;
}

Ast_ಗಂಟು_DType* Ast_ಗಂಟು::getCommonClassTypep(Ast_ಗಂಟು* node1p, Ast_ಗಂಟು* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    ಆದರೆ (VN_IS(node1p, Const)) std::ಅದಲುಬದಲು(node1p, node2p);
    {
        ಸ್ಥಿರಾಂಕ VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        ಆದರೆ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            ಹಿಂತಿರುಗಿ node1p->dtypep();
        ಆದರೆ (castable == VCastable::DYNAMIC_CLASS) ಹಿಂತಿರುಗಿ node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    ಆಗ (classDtypep1) {
        ಸ್ಥಿರಾಂಕ VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        ಆದರೆ (castable == VCastable::COMPATIBLE) ಹಿಂತಿರುಗಿ classDtypep1;
        AstClassExtends* ಸ್ಥಿರಾಂಕ extendsp = classDtypep1->classp()->extendsp();
        ಆದರೆ (!extendsp) ನಿಲ್ಲಿಸು;
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->ಮಗು_DTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

ವರ್ಗ VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<ಪೂರ್ಣಾಂಕ, std::string> m_interned_ಶಬ್ದ_Widths;  // Interned widths
    std::map<ಪೂರ್ಣಾಂಕ, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ m_dump_ಅಗಲ_Min = v3Global.ಅಗಲ_Min_Usage() != VWidthMinUsage::MATCHES_ಅಗಲ;

    ಸ್ಥಿರ std::string ಗಮ್ಯ_Letters(ಗಾತ್ರ_t ಮೌಲ್ಯ, ತಾರ್ಕಿಕ ಅಧೋ_Case = ಸುಳ್ಳು) {
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ ತಳ = ಅಧೋ_Case ? 'a' : 'A';
        std::string s;
        ಮಾಡು { s += static_cast<ಅಕ್ಷರ>(ತಳ + ಮೌಲ್ಯ % 26); } ಆಗ (ಮೌಲ್ಯ /= 26);
        ಹಿಂತಿರುಗಿ s;
    }

    ಸ್ಥಿರಾಂಕ std::string& internConst(ಸ್ಥಿರಾಂಕ AstConst& nodep) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_internedConsts.emplace(nodep.ಅಂಕೆ().ascii(ಸುಳ್ಳು), "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ += ಗಮ್ಯ_Letters(m_internedConsts.ಗಾತ್ರ_2() - 1);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಸ್ಥಿರಾಂಕ std::string& intern_ಶಬ್ದ_ಅಗಲ(ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_interned_ಶಬ್ದ_Widths.emplace(ಮೌಲ್ಯ, "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ += ಗಮ್ಯ_Letters(m_interned_ಶಬ್ದ_Widths.ಗಾತ್ರ_2() - 1, ನಿಜ);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    ಸ್ಥಿರಾಂಕ std::string& intern_Wide_ಅಗಲ(ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_internedWideWidths.emplace(ಮೌಲ್ಯ, "");
        ಆದರೆ (pair.ಎರಡನೇ) pair.ಮೊದಲ->ಎರಡನೇ += ಗಮ್ಯ_Letters(m_internedWideWidths.ಗಾತ್ರ_2() - 1);
        ಹಿಂತಿರುಗಿ pair.ಮೊದಲ->ಎರಡನೇ;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ಸ್ಥಿರ ತಾರ್ಕಿಕ is_ಎಲೆ(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ಸ್ಥಿರಾಂಕ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            ಆದರೆ (opType != VNTypeInfo::OP_ಬಳಸದ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Render operand, return true if not unused
    ಶೂನ್ಯ render_ಕಾರ್ಯಾಂಶ(VNTypeInfo::OpEn opType, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ opp, uint32_t ಆಳ) {
        ಆಯ್ಕೆ (opType) {
        ಸ್ಥಿತಿ VNTypeInfo::OP_ಬಳಸದ:  //
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNTypeInfo::OP_ಬಳಸಿದ:  //
            m_os << ' ';
            render(opp, ಆಳ);
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNTypeInfo::OP_LIST:
            m_os << " [";
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep = opp; nodep; nodep = nodep->nextp()) {
                ಆದರೆ (nodep != opp) m_os << ", ";
                render(opp, ಆಳ);
            }
            m_os << ']';
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ VNTypeInfo::OP_OPTIONAL:
            ಆದರೆ (opp) {
                m_os << " ";
                render(opp, ಆಳ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_os << " nil";
            }
            ನಿಲ್ಲಿಸು;
        }
    }

    // Render the node into the stream.
    ಶೂನ್ಯ render(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, uint32_t ಆಳ) {
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            ಆದರೆ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (is_ಎಲೆ(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಆಳ == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ಸ್ಥಿರಾಂಕ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            render_ಕಾರ್ಯಾಂಶ(typeInfo.m_opType[0], nodep->op1p(), ಆಳ - 1);
            render_ಕಾರ್ಯಾಂಶ(typeInfo.m_opType[1], nodep->op2p(), ಆಳ - 1);
            render_ಕಾರ್ಯಾಂಶ(typeInfo.m_opType[2], nodep->op3p(), ಆಳ - 1);
            render_ಕಾರ್ಯಾಂಶ(typeInfo.m_opType[3], nodep->op4p(), ಆಳ - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : ಶೂನ್ಯಸೂಚಕ;
        ಆದರೆ (!dtypep) {
            m_os << '?';
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = nodep->ಅಗಲ();
            ಆದರೆ (ಅಗಲ == 1) {
                m_os << '1';
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಗಲ <= VL_QUADSIZE) {
                m_os << intern_ಶಬ್ದ_ಅಗಲ(ಅಗಲ);
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_os << intern_Wide_ಅಗಲ(ಅಗಲ);
            }
            ಆದರೆ (m_dump_ಅಗಲ_Min) {
                m_os << '/';
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ_Min = nodep->ಅಗಲ_Min();
                ಆದರೆ (ಅಗಲ_Min == 1) {
                    m_os << '1';
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (ಅಗಲ_Min <= VL_QUADSIZE) {
                    m_os << intern_ಶಬ್ದ_ಅಗಲ(ಅಗಲ_Min);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    m_os << intern_Wide_ಅಗಲ(ಅಗಲ_Min);
                }
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    VNPatternString(std::ostream& os, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, uint32_t ಆಳ)
        : m_os{os} {
        render(nodep, ಆಳ);
    }
};

std::string Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::ನಮೂನೆ_String(uint32_t ಆಳ) ಸ್ಥಿರಾಂಕ {
    std::ostringstream oss;
    VNPatternString{oss, ಇದು, ಆಳ};
    ಹಿಂತಿರುಗಿ oss.str();
}

//######################################################################
// VNDeleter

ಶೂನ್ಯ VNDeleter::doDeletes() {
    ಗಾಗಿ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep : m_deleteps) nodep->delete_ವೃಕ್ಷ();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#ಅನ್ತರ್ಭಾವ "V3Ast__gen_visitor_defns.h"  // From ./astgen
