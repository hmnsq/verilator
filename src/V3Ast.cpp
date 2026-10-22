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

uint64_t Ast_ಗಂಟು::s_editCntLast = 0;
uint64_t Ast_ಗಂಟು::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

ತಾರ್ಕಿಕ VNUser1InUse::s_userBusy = ಅಸತ್ಯ;
ತಾರ್ಕಿಕ VNUser2InUse::s_userBusy = ಅಸತ್ಯ;
ತಾರ್ಕಿಕ VNUser3InUse::s_userBusy = ಅಸತ್ಯ;
ತಾರ್ಕಿಕ VNUser4InUse::s_userBusy = ಅಸತ್ಯ;

ಪೂರ್ಣಾಙ್ಕ Ast_ಗಂಟು_DType::s_unique_ಅಂಕೆ = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(ಧ್ರುವ string& name) {
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < _ENUM_MAX; ++i) {
        ಧ್ರುವ VCMethod method{i};
        ಯದಿ (name == method.ascii()) ಪ್ರತ್ಯಾಗಮ method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    ಪ್ರತ್ಯಾಗಮ VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ಧ್ರುವ ಶೂನ್ಯ*)) ಧ್ರುವ {
#ifdef VL_USER_TYPE_CHECKS
    ಯದಿ (ಧ್ರುವ uint64_t* ಧ್ರುವ uip = std::get_if<uint64_t>(&m_u)) ಪ್ರತ್ಯಾಗಮ "#"s + cvtToStr(*uip);
    ಯದಿ (ಶೂನ್ಯ* ಧ್ರುವ* ಧ್ರುವ upp = std::get_if<ಶೂನ್ಯ*>(&m_u)) ಪ್ರತ್ಯಾಗಮ fmtAddrp(*upp);
    ಪ್ರತ್ಯಾಗಮ "";
#ಅನ್ಯಥಾ
    // Dumps void* representation
    ಯದಿ (!m_u.up) ಪ್ರತ್ಯಾಗಮ "";
    ಪ್ರತ್ಯಾಗಮ fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

ಧ್ರುವ VNTypeInfo VNType::s_typeInfoTable[VNType::ಅಂಕೆ_TYPES()] = {
#ಅನ್ತರ್ಭಾವ "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

ತಾರ್ಕಿಕ VFwdType::is_ಗಂಟು_Compatible(ಧ್ರುವ Ast_ಗಂಟು* nodep) ಧ್ರುವ {
    ಧ್ರುವ Ast_ಗಂಟು* defp = nodep;
    ಯದಿ (ಧ್ರುವ AstTypedef* ಧ್ರುವ adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ adefp = VN_CAST(defp, ಗಂಟು_DType))
        defp = adefp->skipRefToNonRefp();
    ವಿಕಲ್ಪ (m_e) {
    ಪಕ್ಷ VFwdType::NONE: ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ; ಭಙ್ಗ;
    ಪಕ್ಷ VFwdType::ENUM: ಪ್ರತ್ಯಾಗಮ VN_IS(defp, EnumDType); ಭಙ್ಗ;
    ಪಕ್ಷ VFwdType::STRUCT: ಪ್ರತ್ಯಾಗಮ VN_IS(defp, StructDType); ಭಙ್ಗ;
    ಪಕ್ಷ VFwdType::UNION: ಪ್ರತ್ಯಾಗಮ VN_IS(defp, UnionDType); ಭಙ್ಗ;
    ಪಕ್ಷ VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    ಪಕ್ಷ VFwdType::CLASS: ಪ್ರತ್ಯಾಗಮ VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); ಭಙ್ಗ;
    ಪಕ್ಷ VFwdType::GENERIC_INTERFACE: ಪ್ರತ್ಯಾಗಮ VN_IS(defp, IfaceRefDType); ಭಙ್ಗ;
    ಸ್ವಭಾವ: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ಧ್ರುವ std::shared_ptr<ಧ್ರುವ string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
ಧ್ರುವ std::shared_ptr<ಧ್ರುವ string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(ತಾರ್ಕಿಕ useSelfForThis, ಧ್ರುವ string& text) {
    ಪ್ರತ್ಯಾಗಮ useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(ತಾರ್ಕಿಕ useSelfForThis, ತಾರ್ಕಿಕ protect) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ VIdProtect::protect_ಪದಗಳು_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_ಗಂಟು::Ast_ಗಂಟು(VNType t, File_ಸಾಲು* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = ಏತತ್;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = ಅಸತ್ಯ;
    m_flags.doingWidth = ಅಸತ್ಯ;
    m_flags.protect = ಸತ್ಯ;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_ಎಣಿಕೆ_Inc();
}

Ast_ಗಂಟು* Ast_ಗಂಟು::abovep() ಧ್ರುವ {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), ಏತತ್, "abovep() not allowed when in midlist");
    ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ firstp = firstAbovep() ? ಏತತ್ : m_headtailp;
    ಪ್ರತ್ಯಾಗಮ firstp->backp();
}
Ast_ಗಂಟು* Ast_ಗಂಟು::aboveLoopp() ಧ್ರುವ {
    // Returns parent node. Avoid using this, may have performance issues.
    ಧ್ರುವ Ast_ಗಂಟು* nodep = ಏತತ್;
    // Backwards over peers (versus parents)
    ಯಾವತ್ (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    ಪ್ರತ್ಯಾಗಮ nodep->backp();
}

string Ast_ಗಂಟು::encodeName(ಧ್ರುವ string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    ಪ್ರತಿ (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        ಯದಿ ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } ಅನ್ಯಥಾ ಯದಿ (pos[0] == '_') {
            out += pos[0];
            ಯದಿ (pos + 1 == namein.end()) ಭಙ್ಗ;
            ಯದಿ (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } ಅನ್ಯಥಾ {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ಧ್ರುವ unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    ಪ್ರತ್ಯಾಗಮ vname.hashedName();
}

string Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(int64_t ಅಂಕೆ) {
    ಯದಿ (ಅಂಕೆ < 0) ಪ್ರತ್ಯಾಗಮ "__02D" + cvtToStr(-ಅಂಕೆ);  // 2D=-
    ಪ್ರತ್ಯಾಗಮ cvtToStr(ಅಂಕೆ);
}

string Ast_ಗಂಟು::nameProtect() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VIdProtect::protectIf(name(), protect()); }
string Ast_ಗಂಟು::origNameProtect() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ VIdProtect::protectIf(origName(), protect()); }

string Ast_ಗಂಟು::shortName() ಧ್ರುವ {
    string pretty = name();
    string::size_type pos;
    ಯಾವತ್ ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    ಪ್ರತ್ಯಾಗಮ pretty;
}

string Ast_ಗಂಟು::dedotName(ಧ್ರುವ string& namein) {
    string pretty = namein;
    string::size_type pos;
    ಯಾವತ್ ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    ಯದಿ (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ಪ್ರತ್ಯಾಗಮ pretty;
}

string Ast_ಗಂಟು::nameNoArray(ಧ್ರುವ string& namein) {
    ಪ್ರತ್ಯಾಗಮ namein.substr(0, namein.find("__BRA__"));
}

string Ast_ಗಂಟು::vcdName(ಧ್ರುವ string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    ಯಾವತ್ ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    ಯಾವತ್ ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    ಪ್ರತ್ಯಾಗಮ prettyName(pretty);
}

string Ast_ಗಂಟು::prettyName(ಧ್ರುವ string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* pos = namein.c_str(); *pos;) {
        ಯದಿ (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            ಅನುವರ್ತ;
        }
        ಯದಿ (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            ಯದಿ (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                ಅನುವರ್ತ;
            }
            ಯದಿ (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                ಅನುವರ್ತ;
            }
            ಯದಿ (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                ಅನುವರ್ತ;
            }
            ಯದಿ (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                ಅನುವರ್ತ;
            }
            ಯದಿ (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                ಅನುವರ್ತ;
            }
            ಯದಿ (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                ಅನುವರ್ತ;
            }
            ಯದಿ (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                ವರ್ಣ ಮೌಲ್ಯ = 0;
                ಮೌಲ್ಯ += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ಮೌಲ್ಯ
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += ಮೌಲ್ಯ;
                pos += 5;
                ಅನುವರ್ತ;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    ಯದಿ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ಯದಿ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ಪ್ರತ್ಯಾಗಮ pretty;
}

string Ast_ಗಂಟು::vpiName(ಧ್ರುವ string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ಧ್ರುವ size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    ತಾರ್ಕಿಕ inEscapedIdent = ಅಸತ್ಯ;
    ಪೂರ್ಣಾಙ್ಕ lastIdent = 0;

    ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* pos = namein.c_str() + offset; *pos;) {
        ವರ್ಣ specialChar = 0;
        ಯದಿ (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } ಅನ್ಯಥಾ ಯದಿ (pos[0] == '_' && pos[1] == '_') {  // __
            ಯದಿ (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } ಅನ್ಯಥಾ ಯದಿ (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } ಅನ್ಯಥಾ ಯದಿ (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } ಅನ್ಯಥಾ ಯದಿ (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                ಅನುವರ್ತ;
            } ಅನ್ಯಥಾ ಯದಿ (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                ವರ್ಣ ಮೌಲ್ಯ = 0;
                ಮೌಲ್ಯ += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ಮೌಲ್ಯ
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                ಯದಿ (ಮೌಲ್ಯ != '_') inEscapedIdent = ಸತ್ಯ;

                pretty += ಮೌಲ್ಯ;
                pos += 5;
                ಅನುವರ್ತ;
            }
        } ಅನ್ಯಥಾ ಯದಿ (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        ಯದಿ (specialChar) {
            ಯದಿ (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.ಸೇರಿಸು(lastIdent, "\\");
                inEscapedIdent = ಅಸತ್ಯ;
            }

            pretty += specialChar;

            ಯದಿ (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = ಅಸತ್ಯ;
            }
        } ಅನ್ಯಥಾ {
            pretty += pos[0];
            ++pos;
        }
    }
    ಯದಿ (inEscapedIdent) {
        pretty += " ";
        pretty.ಸೇರಿಸು(lastIdent, "\\");
    }
    ಯದಿ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ಯದಿ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ಪ್ರತ್ಯಾಗಮ pretty;
}

string Ast_ಗಂಟು::prettyTypeName() ಧ್ರುವ {
    ಯದಿ (name() == "") ಪ್ರತ್ಯಾಗಮ typeName();
    ಪ್ರತ್ಯಾಗಮ std::string{typeName()} + " '" + pretty_Name_ಸಂದೇಶ() + "'";
}

//######################################################################
// Insertion

ಶೂನ್ಯ Ast_ಗಂಟು::debug_ವೃಕ್ಷ_Change(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ ವರ್ಣ* prefix, ಪೂರ್ಣಾಙ್ಕ lineno, ತಾರ್ಕಿಕ next) {
#ifdef VL_DEBUG
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
#endif
}

template <>
Ast_ಗಂಟು* Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_ವೃಕ್ಷ_Change(nodep, "-addNextThs: ", __LINE__, ಅಸತ್ಯ);
    debug_ವೃಕ್ಷ_Change(newp, "-addNextNew: ", __LINE__, ಸತ್ಯ);
    ಯದಿ (!nodep)  // verilog.y and lots of other places assume this
        ಪ್ರತ್ಯಾಗಮ newp;

    // Find end of old list
    Ast_ಗಂಟು* oldtailp = nodep;
    ಯದಿ (oldtailp->m_nextp) {
        ಯದಿ (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } ಅನ್ಯಥಾ {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            ಯಾವತ್ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_ಗಂಟು* ಧ್ರುವ newtailp = newp->m_headtailp;
    Ast_ಗಂಟು* ಧ್ರುವ headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_ಎಣಿಕೆ_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_ವೃಕ್ಷ_Change(nodep, "-addNextOut:", __LINE__, ಸತ್ಯ);
    ಪ್ರತ್ಯಾಗಮ nodep;
}

ಶೂನ್ಯ Ast_ಗಂಟು::addNextHere(Ast_ಗಂಟು* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-addHereThs: ", __LINE__, ಅಸತ್ಯ);
    debug_ವೃಕ್ಷ_Change(newp, "-addHereNew: ", __LINE__, ಸತ್ಯ);
    newp->edit_ಎಣಿಕೆ_Inc();

    Ast_ಗಂಟು* ಧ್ರುವ addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_ಗಂಟು* ಧ್ರುವ oldnextp = ಏತತ್->m_nextp;
    ಏತತ್->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    ಯದಿ (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = ಏತತ್;

    // Head/tail
    Ast_ಗಂಟು* ಧ್ರುವ oldheadtailp = ಏತತ್->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    ಯದಿ (oldheadtailp) {  // else in middle of list, no change
        ಯದಿ (oldheadtailp == ಏತತ್) {  // this was one node
            ಏತತ್->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } ಅನ್ಯಥಾ ಯದಿ (!oldnextp) {  // this was tail
            ಏತತ್->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-addHereOut: ", __LINE__, ಸತ್ಯ);
}

ಶೂನ್ಯ Ast_ಗಂಟು::setOp1p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, ಏತತ್, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-setOp1pThs: ", __LINE__, ಅಸತ್ಯ);
    debug_ವೃಕ್ಷ_Change(newp, "-setOp1pNew: ", __LINE__, ಸತ್ಯ);
    m_op1p = newp;
    newp->edit_ಎಣಿಕೆ_Inc();
    newp->m_backp = ಏತತ್;
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-setOp1pOut: ", __LINE__, ಅಸತ್ಯ);
}

ಶೂನ್ಯ Ast_ಗಂಟು::setOp2p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, ಏತತ್, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-setOp2pThs: ", __LINE__, ಅಸತ್ಯ);
    debug_ವೃಕ್ಷ_Change(newp, "-setOp2pNew: ", __LINE__, ಸತ್ಯ);
    m_op2p = newp;
    newp->edit_ಎಣಿಕೆ_Inc();
    newp->m_backp = ಏತತ್;
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-setOp2pOut: ", __LINE__, ಅಸತ್ಯ);
}

ಶೂನ್ಯ Ast_ಗಂಟು::setOp3p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, ಏತತ್, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-setOp3pThs: ", __LINE__, ಅಸತ್ಯ);
    debug_ವೃಕ್ಷ_Change(newp, "-setOp3pNew: ", __LINE__, ಸತ್ಯ);
    m_op3p = newp;
    newp->edit_ಎಣಿಕೆ_Inc();
    newp->m_backp = ಏತತ್;
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-setOp3pOut: ", __LINE__, ಅಸತ್ಯ);
}

ಶೂನ್ಯ Ast_ಗಂಟು::setOp4p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, ಏತತ್, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-setOp4pThs: ", __LINE__, ಅಸತ್ಯ);
    debug_ವೃಕ್ಷ_Change(newp, "-setOp4pNew: ", __LINE__, ಸತ್ಯ);
    m_op4p = newp;
    newp->edit_ಎಣಿಕೆ_Inc();
    newp->m_backp = ಏತತ್;
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-setOp4pOut: ", __LINE__, ಅಸತ್ಯ);
}

ಶೂನ್ಯ Ast_ಗಂಟು::addOp1p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಯದಿ (!m_op1p) {
        op1p(newp);
    } ಅನ್ಯಥಾ {
        m_op1p->addNext(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::addOp2p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಯದಿ (!m_op2p) {
        op2p(newp);
    } ಅನ್ಯಥಾ {
        m_op2p->addNext(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::addOp3p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಯದಿ (!m_op3p) {
        op3p(newp);
    } ಅನ್ಯಥಾ {
        m_op3p->addNext(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::addOp4p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಯದಿ (!m_op4p) {
        op4p(newp);
    } ಅನ್ಯಥಾ {
        m_op4p->addNext(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::prependOp1p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಯದಿ (!m_op1p) {
        op1p(newp);
    } ಅನ್ಯಥಾ {
        m_op1p->addHereThisAsNext(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::prependOp2p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಯದಿ (!m_op2p) {
        op2p(newp);
    } ಅನ್ಯಥಾ {
        m_op2p->addHereThisAsNext(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::prependOp3p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಯದಿ (!m_op3p) {
        op3p(newp);
    } ಅನ್ಯಥಾ {
        m_op3p->addHereThisAsNext(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::prependOp4p(Ast_ಗಂಟು* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ಯದಿ (!m_op4p) {
        op4p(newp);
    } ಅನ್ಯಥಾ {
        m_op4p->addHereThisAsNext(newp);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::replaceWith(Ast_ಗಂಟು* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    ಏತತ್->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
ಶೂನ್ಯ Ast_ಗಂಟು::replaceWithKeepDType(Ast_ಗಂಟು* newp) {
    newp->dtypeFrom(ಏತತ್);
    replaceWith(newp);
}

ಶೂನ್ಯ VNRelinker::dump(std::ostream& str) ಧ್ರುವ {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_ಗಂಟು* Ast_ಗಂಟು::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-unlinkWNextThs: ", __LINE__, ಸತ್ಯ);
    Ast_ಗಂಟು* ಧ್ರುವ oldp = ಏತತ್;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ಎಣಿಕೆ_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ಗಂಟು* ಧ್ರುವ backp = oldp->m_backp;
    ಯದಿ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        ಯದಿ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ಅನ್ಯಥಾ {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ಯದಿ (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_ಗಂಟು* oldtailp = backp->m_headtailp;
        ಯದಿ (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            ಯಾವತ್ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_ಗಂಟು* ಧ್ರುವ oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } ಅನ್ಯಥಾ ಯದಿ (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } ಅನ್ಯಥಾ ಯದಿ (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } ಅನ್ಯಥಾ ಯದಿ (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } ಅನ್ಯಥಾ ಯದಿ (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } ಅನ್ಯಥಾ {
        ಏತತ್->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    ಯದಿ (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_ವೃಕ್ಷ_Change(oldp, "-unlinkWNextOut: ", __LINE__, ಸತ್ಯ);
    ಪ್ರತ್ಯಾಗಮ oldp;
}

Ast_ಗಂಟು* Ast_ಗಂಟು::unlinkFrBack(VNRelinker* linkerp) {
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-unlinkFrBkThs: ", __LINE__, ಸತ್ಯ);
    Ast_ಗಂಟು* ಧ್ರುವ oldp = ಏತತ್;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ಎಣಿಕೆ_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ಗಂಟು* ಧ್ರುವ backp = oldp->m_backp;
    ಯದಿ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        ಯದಿ (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        ಯದಿ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ಅನ್ಯಥಾ {
            ಏತತ್->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ಯದಿ (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        ಯದಿ (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        ಯದಿ (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } ಅನ್ಯಥಾ {
        ಯದಿ (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } ಅನ್ಯಥಾ ಯದಿ (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } ಅನ್ಯಥಾ {
            ಏತತ್->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        ಯದಿ (oldp->m_nextp) {
            Ast_ಗಂಟು* ಧ್ರುವ newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    ಯದಿ (oldp->m_iterpp) {  // Only if no linker, point to next in list
        ಯದಿ (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_ವೃಕ್ಷ_Change(oldp, "-unlinkFrBkOut: ", __LINE__, ಸತ್ಯ);
    ಪ್ರತ್ಯಾಗಮ oldp;
}

ಶೂನ್ಯ Ast_ಗಂಟು::relink(VNRelinker* linkerp) {
    ಯದಿ (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_ಗಂಟು* ಧ್ರುವ newp = ಏತತ್;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_ಎಣಿಕೆ_Inc();

    ಯದಿ (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_ಗಂಟು* ಧ್ರುವ backp = linkerp->m_backp;
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-relinkNew: ", __LINE__, ಸತ್ಯ);
    debug_ವೃಕ್ಷ_Change(backp, "-relinkTre: ", __LINE__, ಸತ್ಯ);

    ವಿಕಲ್ಪ (linkerp->m_chg) {
    ಪಕ್ಷ VNRelinker::RELINK_NEXT: backp->addNextHere(newp); ಭಙ್ಗ;
    ಪಕ್ಷ VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); ಭಙ್ಗ;
    ಪಕ್ಷ VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); ಭಙ್ಗ;
    ಪಕ್ಷ VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); ಭಙ್ಗ;
    ಪಕ್ಷ VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); ಭಙ್ಗ;
    ಸ್ವಭಾವ: ಏತತ್->v3fatalSrc("Relink of node without any link to change."); ಭಙ್ಗ;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    ಯದಿ (linkerp->m_iterpp) {
        // If we're iterating over a next() link, we need to follow links off the
        // NEW node, which is always assumed to be what we are relinking to.
        // This adds a unfortunate hot 8 bytes to every AstNode, but is faster than passing
        // across every function.
        // If anyone has a cleaner way, I'd be grateful.
        newp->m_iterpp = linkerp->m_iterpp;
        *(newp->m_iterpp) = newp;
    }
    // Empty the linker so not used twice accidentally
    linkerp->m_backp = nullptr;
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-relinkOut: ", __LINE__, ಸತ್ಯ);
}

ಶೂನ್ಯ Ast_ಗಂಟು::relinkOneLink(Ast_ಗಂಟು*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_ಗಂಟು* newp) {
    ಯದಿ (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_ಗಂಟು* ಧ್ರುವ newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_ಗಂಟು* ಧ್ರುವ oldlistlastp = pointpr->m_headtailp;
        UASSERT_OBJ(!(oldlistlastp->m_nextp && oldlistlastp != pointpr), newp,
                    "Old headtailp tail isn't at the tail");
        // Next links
        newlistlastp->m_nextp = pointpr;
        pointpr->m_backp = newlistlastp;
        // Head/tail
        pointpr->m_headtailp = nullptr;  // Old head
        newlistlastp->m_headtailp = nullptr;  // Old tail
        newp->m_headtailp = oldlistlastp;  // Head points to tail
        oldlistlastp->m_headtailp = newp;  // Tail points to head
    }
    pointpr = newp;
}

ಶೂನ್ಯ Ast_ಗಂಟು::addHereThisAsNext(Ast_ಗಂಟು* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(ಏತತ್->m_backp, ಏತತ್, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_ಗಂಟು* ಧ್ರುವ backp = ಏತತ್->m_backp;
    Ast_ಗಂಟು* ಧ್ರುವ newLastp = newp->m_headtailp;
    //
    ಏತತ್->edit_ಎಣಿಕೆ_Inc();
    // Common linkage
    newLastp->m_nextp = ಏತತ್;
    ಏತತ್->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    ಯದಿ (backp && backp->m_nextp == ಏತತ್) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } ಅನ್ಯಥಾ {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        ಯದಿ (backp) {
            ಯದಿ (backp->m_op1p == ಏತತ್) {
                backp->m_op1p = newp;
            } ಅನ್ಯಥಾ ಯದಿ (backp->m_op2p == ಏತತ್) {
                backp->m_op2p = newp;
            } ಅನ್ಯಥಾ ಯದಿ (backp->m_op3p == ಏತತ್) {
                backp->m_op3p = newp;
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(backp->m_op4p == ಏತತ್, ಏತತ್, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_ಗಂಟು* ಧ್ರುವ tailp = ಏತತ್->m_headtailp;
        ಏತತ್->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    ಯದಿ (newLastp->m_iterpp) *(newLastp->m_iterpp) = ಏತತ್;
    ಯದಿ (ಏತತ್->m_iterpp) {
        *(ಏತತ್->m_iterpp) = newp;
        ಏತತ್->m_iterpp = nullptr;
    }
    //
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-addHereThisAsNext: ", __LINE__, ಸತ್ಯ);
}

//======================================================================
// Clone

Ast_ಗಂಟು* Ast_ಗಂಟು::clone_ವೃಕ್ಷ_Iter(ತಾರ್ಕಿಕ needPure) {
    // private: Clone single node and children
    ಯದಿ (needPure) purityCheck();
    Ast_ಗಂಟು* ಧ್ರುವ newp = ಏತತ್->clone();
    ಯದಿ (ಏತತ್->m_op1p) newp->op1p(ಏತತ್->m_op1p->clone_ವೃಕ್ಷ_Iter_List(needPure));
    ಯದಿ (ಏತತ್->m_op2p) newp->op2p(ಏತತ್->m_op2p->clone_ವೃಕ್ಷ_Iter_List(needPure));
    ಯದಿ (ಏತತ್->m_op3p) newp->op3p(ಏತತ್->m_op3p->clone_ವೃಕ್ಷ_Iter_List(needPure));
    ಯದಿ (ಏತತ್->m_op4p) newp->op4p(ಏತತ್->m_op4p->clone_ವೃಕ್ಷ_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(ಏತತ್);  // Save pointers to/from both to simplify relinking.
    ಏತತ್->clonep(newp);  // Save pointers to/from both to simplify relinking.
    ಪ್ರತ್ಯಾಗಮ newp;
}

Ast_ಗಂಟು* Ast_ಗಂಟು::clone_ವೃಕ್ಷ_Iter_List(ತಾರ್ಕಿಕ needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_ಗಂಟು* newheadp = nullptr;
    Ast_ಗಂಟು* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    ಪ್ರತಿ (Ast_ಗಂಟು* oldp = ಏತತ್; oldp; oldp = oldp->m_nextp) {
        Ast_ಗಂಟು* ಧ್ರುವ newp = oldp->clone_ವೃಕ್ಷ_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        ಯದಿ (newtailp) newtailp->m_nextp = newp;
        ಯದಿ (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    ಪ್ರತ್ಯಾಗಮ newheadp;
}

Ast_ಗಂಟು* Ast_ಗಂಟು::clone_ವೃಕ್ಷ(ತಾರ್ಕಿಕ cloneNextLink, ತಾರ್ಕಿಕ needPure) {
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_ವೃಕ್ಷ();
    Ast_ಗಂಟು* newp;
    ಯದಿ (cloneNextLink && ಏತತ್->m_nextp) {
        newp = clone_ವೃಕ್ಷ_Iter_List(needPure);
    } ಅನ್ಯಥಾ {
        newp = clone_ವೃಕ್ಷ_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_ವೃಕ್ಷ();
    debug_ವೃಕ್ಷ_Change(newp, "-cloneOut: ", __LINE__, ಸತ್ಯ);
    ಪ್ರತ್ಯಾಗಮ newp;
}

ಶೂನ್ಯ Ast_ಗಂಟು::purityCheck() {
    ಯದಿ (VL_UNLIKELY(!isPure())) {
        ಏತತ್->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << ಏತತ್->warnMore()
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
    ಏತತ್->m_nextp = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಏತತ್->m_backp = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಏತತ್->m_headtailp = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಏತತ್->m_op1p = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಏತತ್->m_op2p = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಏತತ್->m_op3p = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಏತತ್->m_op4p = reinterpret_cast<Ast_ಗಂಟು*>(0x1);
    ಏತತ್->m_iterpp = reinterpret_cast<Ast_ಗಂಟು**>(0x1);
    ಯದಿ (
#ಯದಿ !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        ಸತ್ಯ
#ಅನ್ಯಥಾ
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete ಏತತ್;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

ಶೂನ್ಯ Ast_ಗಂಟು::delete_ವೃಕ್ಷ_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    ಪ್ರತಿ (Ast_ಗಂಟು *nodep = ಏತತ್, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        ಯದಿ (nodep->m_op1p) nodep->m_op1p->delete_ವೃಕ್ಷ_Iter();
        ಯದಿ (nodep->m_op2p) nodep->m_op2p->delete_ವೃಕ್ಷ_Iter();
        ಯದಿ (nodep->m_op3p) nodep->m_op3p->delete_ವೃಕ್ಷ_Iter();
        ಯದಿ (nodep->m_op4p) nodep->m_op4p->delete_ವೃಕ್ಷ_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_ಗಂಟು();
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::delete_ವೃಕ್ಷ() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_ವೃಕ್ಷ_Change(ಏತತ್, "-delTree:  ", __LINE__, ಸತ್ಯ);
    ಏತತ್->edit_ಎಣಿಕೆ_Inc();
    // MUST be depth first!
    delete_ವೃಕ್ಷ_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
ಶೂನ್ಯ* Ast_ಗಂಟು::operator ನೂತನ(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_ಗಂಟು* ಧ್ರುವ objp = static_cast<Ast_ಗಂಟು*>(::operator ನೂತನ(size));
    V3Broken::addNewed(objp);
    ಪ್ರತ್ಯಾಗಮ objp;
}

ಶೂನ್ಯ Ast_ಗಂಟು::operator delete(ಶೂನ್ಯ* objp, size_t size) {
    ಯದಿ (!objp) ಪ್ರತ್ಯಾಗಮ;
    ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ nodep = static_cast<Ast_ಗಂಟು*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
ಶೂನ್ಯ* Ast_ಗಂಟು::operator ನೂತನ(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ಸ್ಥಿರ ಧ್ರುವ size_t ASTGEN_MAX_ಗಂಟು_SIZE = []() {
        size_t maxSize = 0;
        ಪ್ರತಿ (size_t t = 0; t < VNType::ಅಂಕೆ_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        ಪ್ರತ್ಯಾಗಮ maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ಸ್ಥಿರ uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    ಯದಿ (ಪೂರ್ಣಾಙ್ಕ seed = v3Global.opt.debugAllocRandom()) {
        ಸ್ಥಿರ V3Mutex s_mutex;
        ಧ್ರುವ V3LockGuard lock{s_mutex};
        ಸ್ಥಿರ std::array<ಶೂನ್ಯ*, POOL_SIZE> s_ಗಂಟು_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_ಗಂಟು_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        ಯದಿ (!s_lfsr) {
            s_lfsr = seed;
            ಯದಿ (!s_lfsr) s_lfsr = ~s_lfsr;
            ಪ್ರತಿ (size_t i = 0; i < POOL_SIZE; ++i) {
                s_ಗಂಟು_Pool[i] = ::operator ನೂತನ(ASTGEN_MAX_ಗಂಟು_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_ಗಂಟು_Pool), std::end(s_ಗಂಟು_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ಧ್ರುವ size_t index = s_lfsr % POOL_SIZE;
        Ast_ಗಂಟು* ಧ್ರುವ objp = static_cast<Ast_ಗಂಟು*>(s_ಗಂಟು_Pool[index]);
        s_ಗಂಟು_Pool[index] = ::operator ನೂತನ(ASTGEN_MAX_ಗಂಟು_SIZE + 64);  // For later new()
        ಪ್ರತ್ಯಾಗಮ objp;
    } ಅನ್ಯಥಾ {
        Ast_ಗಂಟು* ಧ್ರುವ objp = static_cast<Ast_ಗಂಟು*>(::operator ನೂತನ(size));
        ಪ್ರತ್ಯಾಗಮ objp;
    }
}
ಶೂನ್ಯ Ast_ಗಂಟು::operator delete(ಶೂನ್ಯ* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (ಶೂನ್ಯ)objp;
    (ಶೂನ್ಯ)size;
}
#endif

//======================================================================
// Iterators

ಶೂನ್ಯ Ast_ಗಂಟು::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ಯದಿ (m_op1p) m_op1p->iterateAndNext(v);
    ಯದಿ (m_op2p) m_op2p->iterateAndNext(v);
    ಯದಿ (m_op3p) m_op3p->iterateAndNext(v);
    ಯದಿ (m_op4p) m_op4p->iterateAndNext(v);
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ಯದಿ (m_op1p) m_op1p->iterateAndNextConst(v);
    ಯದಿ (m_op2p) m_op2p->iterateAndNextConst(v);
    ಯದಿ (m_op3p) m_op3p->iterateAndNextConst(v);
    ಯದಿ (m_op4p) m_op4p->iterateAndNextConst(v);
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_ಗಂಟು* nodep = ಏತತ್;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    ಯದಿ (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    ಯಾವತ್ (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        ಯದಿ (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
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
        ಯದಿ (!niterp) ಪ್ರತ್ಯಾಗಮ;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        ಯದಿ (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } ಅನ್ಯಥಾ {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_ಗಂಟು* nodep = ಏತತ್;
    ಯಾವತ್ (nodep->m_nextp) nodep = nodep->m_nextp;
    ಯಾವತ್ (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        ಯದಿ (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } ಅನ್ಯಥಾ {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    ಯದಿ (m_op1p) m_op1p->iterateListBackwardsConst(v);
    ಯದಿ (m_op2p) m_op2p->iterateListBackwardsConst(v);
    ಯದಿ (m_op3p) m_op3p->iterateListBackwardsConst(v);
    ಯದಿ (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

ಶೂನ್ಯ Ast_ಗಂಟು::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_ಗಂಟು* nodep = ಏತತ್;
    ಕುರು {
        Ast_ಗಂಟು* ಧ್ರುವ nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } ಯಾವತ್ (nodep);
}

Ast_ಗಂಟು* Ast_ಗಂಟು::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_ಗಂಟು* nodep = ಏತತ್;  // Note "this" may point to bogus point later in this function
    ಯದಿ (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } ಅನ್ಯಥಾ ಯದಿ (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ಧ್ರುವ tempp = ನೂತನ AstBegin{nodep->fileline(), "[EditWrapper]", nodep, ಅಸತ್ಯ};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_ವೃಕ್ಷ(), tempp);
    } ಅನ್ಯಥಾ {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_ಗಂಟು** nextnodepp = nullptr;
        ಯದಿ (ಏತತ್->m_backp->m_op1p == ಏತತ್) {
            nextnodepp = &(ಏತತ್->m_backp->m_op1p);
        } ಅನ್ಯಥಾ ಯದಿ (ಏತತ್->m_backp->m_op2p == ಏತತ್) {
            nextnodepp = &(ಏತತ್->m_backp->m_op2p);
        } ಅನ್ಯಥಾ ಯದಿ (ಏತತ್->m_backp->m_op3p == ಏತತ್) {
            nextnodepp = &(ಏತತ್->m_backp->m_op3p);
        } ಅನ್ಯಥಾ ಯದಿ (ಏತತ್->m_backp->m_op4p == ಏತತ್) {
            nextnodepp = &(ಏತತ್->m_backp->m_op4p);
        } ಅನ್ಯಥಾ ಯದಿ (ಏತತ್->m_backp->m_nextp == ಏತತ್) {
            nextnodepp = &(ಏತತ್->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, ಏತತ್, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    ಪ್ರತ್ಯಾಗಮ nodep;
}

//======================================================================

ಶೂನ್ಯ Ast_ಗಂಟು::clone_Relink_ವೃಕ್ಷ() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    ಪ್ರತಿ (Ast_ಗಂಟು* nodep = ಏತತ್; nodep; nodep = nodep->m_nextp) {
        ಯದಿ (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        ಯದಿ (nodep->m_op1p) nodep->m_op1p->clone_Relink_ವೃಕ್ಷ();
        ಯದಿ (nodep->m_op2p) nodep->m_op2p->clone_Relink_ವೃಕ್ಷ();
        ಯದಿ (nodep->m_op3p) nodep->m_op3p->clone_Relink_ವೃಕ್ಷ();
        ಯದಿ (nodep->m_op4p) nodep->m_op4p->clone_Relink_ವೃಕ್ಷ();
    }
}

//======================================================================
// Comparison

ತಾರ್ಕಿಕ Ast_ಗಂಟು::gate_ವೃಕ್ಷ_Iter() ಧ್ರುವ {
    // private: Return true if the two trees are identical
    ಯದಿ (!isGateOptimizable()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (m_op1p && !m_op1p->gate_ವೃಕ್ಷ_Iter()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (m_op2p && !m_op2p->gate_ವೃಕ್ಷ_Iter()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (m_op3p && !m_op3p->gate_ವೃಕ್ಷ_Iter()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (m_op4p && !m_op4p->gate_ವೃಕ್ಷ_Iter()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

ತಾರ್ಕಿಕ Ast_ಗಂಟು::same_ವೃಕ್ಷ_Iter(ಧ್ರುವ Ast_ಗಂಟು* node1p, ಧ್ರುವ Ast_ಗಂಟು* node2p, ತಾರ್ಕಿಕ ignNext,
                           ತಾರ್ಕಿಕ gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    ಯಾವತ್ (ಸತ್ಯ) {
        ಯದಿ (!node1p && !node2p) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (!node1p || !node2p) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (node1p->type() != node2p->type()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        ಯದಿ (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!node1p->same_ಗಂಟು(node2p) || (gateOnly && !node1p->isGateOptimizable())) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (!same_ವೃಕ್ಷ_Iter(node1p->m_op1p, node2p->m_op1p, ಅಸತ್ಯ, gateOnly)
            || !same_ವೃಕ್ಷ_Iter(node1p->m_op2p, node2p->m_op2p, ಅಸತ್ಯ, gateOnly)
            || !same_ವೃಕ್ಷ_Iter(node1p->m_op3p, node2p->m_op3p, ಅಸತ್ಯ, gateOnly)
            || !same_ವೃಕ್ಷ_Iter(node1p->m_op4p, node2p->m_op4p, ಅಸತ್ಯ, gateOnly)) {
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಯದಿ (ignNext) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

ಶೂನ್ಯ Ast_ಗಂಟು::check_ವೃಕ್ಷ_Iter(ಧ್ರುವ Ast_ಗಂಟು* prevBackp) ಧ್ರುವ VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == ಏತತ್->backp(), ಏತತ್, "Back node inconsistent");
    ಧ್ರುವ VNTypeInfo& typeInfo = VNType::typeInfo(ಏತತ್->type());
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1; i <= 4; i++) {
        Ast_ಗಂಟು* nodep = nullptr;
        ವಿಕಲ್ಪ (i) {
        ಪಕ್ಷ 1: nodep = op1p(); ಭಙ್ಗ;
        ಪಕ್ಷ 2: nodep = op2p(); ಭಙ್ಗ;
        ಪಕ್ಷ 3: nodep = op3p(); ಭಙ್ಗ;
        ಪಕ್ಷ 4: nodep = op4p(); ಭಙ್ಗ;
        ಸ್ವಭಾವ: ಏತತ್->v3fatalSrc("Bad case"); ಭಙ್ಗ;
        }
        // cppcheck-suppress danglingTempReference
        ಧ್ರುವ ವರ್ಣ* opName = typeInfo.m_opNamep[i - 1];
        ವಿಕಲ್ಪ (typeInfo.m_opType[i - 1]) {
        ಪಕ್ಷ VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, ಏತತ್, typeInfo.m_namep << " must not use " << opName << "()");
            ಭಙ್ಗ;
        ಪಕ್ಷ VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, ಏತತ್,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), ಏತತ್,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_ವೃಕ್ಷ_Iter(ಏತತ್);
            ಭಙ್ಗ;
        ಪಕ್ಷ VNTypeInfo::OP_LIST:
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ headp = nodep) {
                // cppcheck-suppress shadowFunction
                ಧ್ರುವ Ast_ಗಂಟು* backp = ಏತತ್;
                ಧ್ರುವ Ast_ಗಂಟು* tailp;
                ಧ್ರುವ Ast_ಗಂಟು* opp = headp;
                ಕುರು {
                    opp->check_ವೃಕ್ಷ_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } ಯಾವತ್ (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ VNTypeInfo::OP_OPTIONAL:
            ಯದಿ (nodep) {
                UASSERT_OBJ(!nodep->nextp(), ಏತತ್,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_ವೃಕ್ಷ_Iter(ಏತತ್);
            }
            ಭಙ್ಗ;
        ಸ್ವಭಾವ: ಏತತ್->v3fatalSrc("Bad case"); ಭಙ್ಗ;
        }
    }
    ಯದಿ (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_CAST(ಏತತ್, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ whyp = exprp->widthMismatch();
            ಯದಿ (whyp) {
                auto dtypeStr = [](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    ಪ್ರತ್ಯಾಗಮ ss.str();
                };
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Uniop* ಧ್ರುವ uniopp = VN_CAST(exprp, ಗಂಟು_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ biopp = VN_CAST(exprp, ಗಂಟು_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } ಅನ್ಯಥಾ {
                    UASSERT_OBJ(ಅಸತ್ಯ, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
ವರ್ಣ* Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Json_Gdb(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
    ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_ವೃಕ್ಷ_Json(nodepStream);
    ಧ್ರುವ std::string str = nodepStream.rdbuf()->str();
    ಪ್ರತ್ಯಾಗಮ strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
ವರ್ಣ* Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Json_Gdb(ಧ್ರುವ ವರ್ಣ* str) { ಪ್ರತ್ಯಾಗಮ strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
ವರ್ಣ* Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Json_Gdb(intptr_t nodep) {
    ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ strdup("{\"addr\":\"NULL\"}\n");
    ಪ್ರತ್ಯಾಗಮ dump_ವೃಕ್ಷ_Json_Gdb(reinterpret_cast<ಧ್ರುವ Ast_ಗಂಟು*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::dumpGdb(ಧ್ರುವ Ast_ಗಂಟು* nodep) {  // For GDB only  // LCOV_EXCL_START
    ಯದಿ (!nodep) {
        cout << "<nullptr>" << endl;
        ಪ್ರತ್ಯಾಗಮ;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::dumpGdbHeader() ಧ್ರುವ {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Gdb(ಧ್ರುವ Ast_ಗಂಟು* nodep) {  // For GDB only  // LCOV_EXCL_START
    ಯದಿ (!nodep) {
        cout << "<nullptr>" << endl;
        ಪ್ರತ್ಯಾಗಮ;
    }
    nodep->dumpGdbHeader();
    nodep->dump_ವೃಕ್ಷ(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_File_Gdb(ಧ್ರುವ Ast_ಗಂಟು* nodep,  // LCOV_EXCL_START
                              ಧ್ರುವ ವರ್ಣ* filenamep) {  // For GDB only
    ಯದಿ (!nodep) {
        cout << "<nullptr>" << endl;
        ಪ್ರತ್ಯಾಗಮ;
    }
    ಧ್ರುವ string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_ವೃಕ್ಷ_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
ಶೂನ್ಯ Ast_ಗಂಟು::checkIter() ಧ್ರುವ {
    ಯದಿ (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        ಏತತ್->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::dumpPtrs(std::ostream& os) ಧ್ರುವ {
    os << "This=" << typeName() << " " << cvtToHex(ಏತತ್);
    os << " back=" << cvtToHex(backp());
    ಯದಿ (nextp()) os << " next=" << cvtToHex(nextp());
    ಯದಿ (m_headtailp == ಏತತ್) {
        os << " headtail=this";
    } ಅನ್ಯಥಾ {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    ಯದಿ (op1p()) os << " op1p=" << cvtToHex(op1p());
    ಯದಿ (op2p()) os << " op2p=" << cvtToHex(op2p());
    ಯದಿ (op3p()) os << " op3p=" << cvtToHex(op3p());
    ಯದಿ (op4p()) os << " op4p=" << cvtToHex(op4p());
    ಧ್ರುವ auto dumpUser = [&os](ಧ್ರುವ ವರ್ಣ* prefix, ಧ್ರುವ VNUser& user) {
        ಧ್ರುವ std::string s
            = user.dumpStr([](ಧ್ರುವ ಶೂನ್ಯ* p) -> std::string { ಪ್ರತ್ಯಾಗಮ cvtToHex(p); });
        ಯದಿ (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    ಯದಿ (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ(std::ostream& os, ಧ್ರುವ string& indent, ಪೂರ್ಣಾಙ್ಕ maxDepth) ಧ್ರುವ {
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << ಏತತ್ << '\n';
    ಯದಿ (VN_DELETED(ಏತತ್)) ಪ್ರತ್ಯಾಗಮ;
    ಯದಿ (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    ಯದಿ (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(ಏತತ್)
           << endl;  // endl intentional to do flush
        ಪ್ರತ್ಯಾಗಮ;
    }  // LCOV_EXCL_STOP
    ಯದಿ (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    ಯದಿ (maxDepth == 1) {
        ಯದಿ (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } ಅನ್ಯಥಾ {
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ(os, indent + "1:", maxDepth - 1);
        }
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ(os, indent + "2:", maxDepth - 1);
        }
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ(os, indent + "3:", maxDepth - 1);
        }
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ(os, indent + "4:", maxDepth - 1);
        }
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_And_Next(std::ostream& os, ಧ್ರುವ string& indent, ಪೂರ್ಣಾಙ್ಕ maxDepth) ಧ್ರುವ {
    // Audited to make sure this is never nullptr
    ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = ಏತತ್; nodep; nodep = nodep->nextp()) {
        nodep->dump_ವೃಕ್ಷ(os, indent, maxDepth);
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_File(ಧ್ರುವ string& filename, ತಾರ್ಕಿಕ doDump) {
    // Not const function as calls checkTree
    ಯದಿ (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ಧ್ರುವ std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            ಯದಿ (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_ಎಣಿಕೆ_Last();
            *logsp << "> to <e" << std::dec << edit_ಎಣಿಕೆ_Gbl() << ">\n";
            ಯದಿ (edit_ಎಣಿಕೆ_Gbl() == edit_ಎಣಿಕೆ_Last() && ::dump_ವೃಕ್ಷ_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } ಅನ್ಯಥಾ {
                dump_ವೃಕ್ಷ(*logsp);
                edit_ಎಣಿಕೆ_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

ಸ್ಥಿರ ಶೂನ್ಯ drawChildren(std::ostream& os, ಧ್ರುವ Ast_ಗಂಟು* thisp, ಧ್ರುವ Ast_ಗಂಟು* childp,
                         ಧ್ರುವ std::string& childName) {
    ಯದಿ (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_ವೃಕ್ಷ_Dot(os);
            ಯದಿ (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Dot(std::ostream& os) ಧ್ರುವ {
    os << "\tn" << cvtToHex(ಏತತ್) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, ಏತತ್, m_op1p, "op1");
    drawChildren(os, ಏತತ್, m_op2p, "op2");
    drawChildren(os, ಏತತ್, m_op3p, "op3");
    drawChildren(os, ಏತತ್, m_op4p, "op4");
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Json_File(ಧ್ರುವ string& filename, ತಾರ್ಕಿಕ doDump) {
    ಯದಿ (!doDump) ಪ್ರತ್ಯಾಗಮ;
    UINFO(2, "Dumping " << filename);
    ಧ್ರುವ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ಯದಿ (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_ವೃಕ್ಷ_Json(*treejsonp);
    *treejsonp << '\n';
}

ಶೂನ್ಯ Ast_ಗಂಟು::dumpJsonMetaFileGdb(ಧ್ರುವ ವರ್ಣ* filename) { dumpJsonMetaFile(filename); }
ಶೂನ್ಯ Ast_ಗಂಟು::dumpJsonMetaFile(ಧ್ರುವ string& filename) {
    UINFO(2, "Dumping " << filename);
    ಧ್ರುವ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ಯದಿ (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_ಸಾಲು::file_Name_ಅಂಕೆ_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

ಶೂನ್ಯ Ast_ಗಂಟು::dump_ವೃಕ್ಷ_Dot_File(ಧ್ರುವ string& filename, ತಾರ್ಕಿಕ doDump) {
    ಯದಿ (doDump) {
        UINFO(2, "Dumping " << filename);
        ಧ್ರುವ std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        ಯದಿ (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_ವೃಕ್ಷ_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_ಗಂಟು::instanceStr() ಧ್ರುವ {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_ಎಣಿಕೆ = 0;
    // Walk 'backp' chain
    ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* currp = ಏತತ್; currp; currp = currp->backp(), ++iter_ಎಣಿಕೆ) {
        ಯದಿ (VL_UNCOVERABLE(iter_ಎಣಿಕೆ >= maxIterations)) ಪ್ರತ್ಯಾಗಮ "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        ಯದಿ (ಧ್ರುವ AstScope* ಧ್ರುವ scopep = VN_CAST(currp, Scope)) {
            ಪ್ರತ್ಯಾಗಮ scopep->is_ತುದಿ() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        ಯದಿ (ಧ್ರುವ AstModule* ಧ್ರುವ modp = VN_CAST(currp, Module)) {
            ಧ್ರುವ string instanceName = modp->someInstanceName();
            ಪ್ರತ್ಯಾಗಮ instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    ಪ್ರತ್ಯಾಗಮ "";
}
ಶೂನ್ಯ Ast_ಗಂಟು::v3errorEnd(ಧ್ರುವ std::ostringstream& str) ಧ್ರುವ VL_RELEASE(V3Error::s().m_mutex) {
    ಯದಿ (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } ಅನ್ಯಥಾ {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ಧ್ರುವ string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        ಯದಿ (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_ಗಂಟು*>(ಏತತ್)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
ಶೂನ್ಯ Ast_ಗಂಟು::v3errorEndFatal(ಧ್ರುವ std::ostringstream& str) ಧ್ರುವ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ಪ್ರತಿಜ್ಞಾ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

ಶೂನ್ಯ Ast_ಗಂಟು::dtypeChgSigned(ತಾರ್ಕಿಕ flag) {
    UASSERT_OBJ(dtypep(), ಏತತ್, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
ಶೂನ್ಯ Ast_ಗಂಟು::dtypeChgWidth(ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin) {
    UASSERT_OBJ(dtypep(), ಏತತ್, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

ಶೂನ್ಯ Ast_ಗಂಟು::dtypeChgWidthSigned(ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), ಏತತ್, "No dtype when changing width");
    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ enumDTypep = dtypep()->skipRefToEnump();
    ಯದಿ (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        ಪ್ರತ್ಯಾಗಮ;  // Correct already
    }
    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findBasicDType(VBasicDTypeKwd kwd) ಧ್ರುವ {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findBitDType(ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin, VSigning numeric) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findLogicDType(ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin, VSigning numeric) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findLogic2StateDType(ಪೂರ್ಣಾಙ್ಕ width, ಪೂರ್ಣಾಙ್ಕ widthMin, VSigning numeric) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findLogicRangeDType(ಧ್ರುವ VNumRange& range, ಪೂರ್ಣಾಙ್ಕ widthMin,
                                           VSigning numeric) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findBitRangeDType(ಧ್ರುವ VNumRange& range, ಪೂರ್ಣಾಙ್ಕ widthMin,
                                         VSigning numeric) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_ಗಂಟು::find_ಸೇರಿಸು_Same_DType(AstBasicDType* nodep) {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->find_ಸೇರಿಸು_Same_DType(nodep);
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findConstraintRefDType() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findEmptyQueueDType() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findQueueIndexDType() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findStreamDType() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_ಗಂಟು_DType* Ast_ಗಂಟು::findVoidDType() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

ಸ್ಥಿರ ಧ್ರುವ Ast_ಗಂಟು_DType* computeCastableBase(ಧ್ರುವ Ast_ಗಂಟು_DType* nodep) {
    ಯಾವತ್ (ಸತ್ಯ) {
        ಯದಿ (ಧ್ರುವ AstPackArrayDType* ಧ್ರುವ packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            ಅನುವರ್ತ;
        }
        ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ refp = nodep->skipRefToEnump()) {
            ಯದಿ (refp != nodep) {
                nodep = refp;
                ಅನುವರ್ತ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
}

ಸ್ಥಿರ VCastable computeCastableImp(ಧ್ರುವ Ast_ಗಂಟು_DType* toDtp, ಧ್ರುವ Ast_ಗಂಟು_DType* fromDtp,
                                    ಧ್ರುವ Ast_ಗಂಟು* fromConstp, ಧ್ರುವ ತಾರ್ಕಿಕ checkIfaceArgCompat) {
    ಧ್ರುವ VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    ಯದಿ (toDtp == fromDtp) ಪ್ರತ್ಯಾಗಮ VCastable::SAMEISH;
    ಯದಿ (toDtp->similarDType(fromDtp)) ಪ್ರತ್ಯಾಗಮ VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ಧ್ರುವ Ast_ಗಂಟು_DType* fromBaseDtp = computeCastableBase(fromDtp);

    ಧ್ರುವ ತಾರ್ಕಿಕ fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, ಗಂಟು_UOr_Struct_DType);

    ಧ್ರುವ Ast_ಗಂಟು_DType* toBaseDtp = computeCastableBase(toDtp);
    ಧ್ರುವ ತಾರ್ಕಿಕ toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, ಗಂಟು_UOr_Struct_DType);

    ಯದಿ (toBaseDtp == fromBaseDtp) ಪ್ರತ್ಯಾಗಮ VCastable::COMPATIBLE;

    ಯದಿ (toNumericable) {
        ಯದಿ (fromNumericable) ಪ್ರತ್ಯಾಗಮ VCastable::COMPATIBLE;
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(toBaseDtp, EnumDType)) {
        ಯದಿ (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_ವೃಕ್ಷ(fromDtp))
            ಪ್ರತ್ಯಾಗಮ VCastable::ENUM_IMPLICIT;
        ಯದಿ (fromNumericable) ಪ್ರತ್ಯಾಗಮ VCastable::ENUM_EXPLICIT;
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        ಪ್ರತ್ಯಾಗಮ VCastable::COMPATIBLE;
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        ಯದಿ (fromConstp->isNull()) ಪ್ರತ್ಯಾಗಮ VCastable::COMPATIBLE;
    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ಧ್ರುವ AstClass* ಧ್ರುವ toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        ಧ್ರುವ AstClass* ಧ್ರುವ fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ಧ್ರುವ ತಾರ್ಕಿಕ downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        ಧ್ರುವ ತಾರ್ಕಿಕ upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        ಯದಿ (upcast) ಪ್ರತ್ಯಾಗಮ VCastable::COMPATIBLE;
        ಯದಿ (downcast) ಪ್ರತ್ಯಾಗಮ VCastable::DYNAMIC_CLASS;
        ಪ್ರತ್ಯಾಗಮ VCastable::INCOMPATIBLE;
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(ಸತ್ಯ) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(ಸತ್ಯ) : fromDtp, IfaceRefDType);
        ಯದಿ (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            ಯದಿ (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                ಪ್ರತ್ಯಾಗಮ VCastable::INCOMPATIBLE;
            }
            ಪ್ರತ್ಯಾಗಮ computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        ಯದಿ (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            ಯದಿ (!checkIfaceArgCompat) ಪ್ರತ್ಯಾಗಮ castable;
            ಪ್ರತ್ಯಾಗಮ VCastable::INCOMPATIBLE;
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ sameModport = toIfp->modportp() == fromIfp->modportp();
        ಯದಿ (!checkIfaceArgCompat) {
            ಯದಿ (!toIfp->modportp() || sameModport) ಪ್ರತ್ಯಾಗಮ VCastable::COMPATIBLE;
            ಪ್ರತ್ಯಾಗಮ castable;
        }
        ಯದಿ (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            ಪ್ರತ್ಯಾಗಮ VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        ಯದಿ (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            ಪ್ರತ್ಯಾಗಮ VCastable::COMPATIBLE;
        }
        ಪ್ರತ್ಯಾಗಮ VCastable::INCOMPATIBLE;
    } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        ಯದಿ (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            ಪ್ರತ್ಯಾಗಮ castable;
        }
        ಧ್ರುವ VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ toSubp = toArrayp->subDTypep()->skipRefp();
        ಯದಿ (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            ಪ್ರತ್ಯಾಗಮ VCastable::COMPATIBLE;
        }
    }
    ಪ್ರತ್ಯಾಗಮ castable;
}

VCastable Ast_ಗಂಟು::computeCastable(ಧ್ರುವ Ast_ಗಂಟು_DType* toDtp, ಧ್ರುವ Ast_ಗಂಟು_DType* fromDtp,
                                   ಧ್ರುವ Ast_ಗಂಟು* fromConstp, ಧ್ರುವ ತಾರ್ಕಿಕ checkIfaceArgCompat) {
    ಧ್ರುವ auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    ಯದಿ (fromConstp) UINFO(9, "     const= " << fromConstp);
    ಪ್ರತ್ಯಾಗಮ castable;
}

Ast_ಗಂಟು_DType* Ast_ಗಂಟು::getCommonClassTypep(Ast_ಗಂಟು* node1p, Ast_ಗಂಟು* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    ಯದಿ (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        ಧ್ರುವ VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        ಯದಿ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            ಪ್ರತ್ಯಾಗಮ node1p->dtypep();
        ಯದಿ (castable == VCastable::DYNAMIC_CLASS) ಪ್ರತ್ಯಾಗಮ node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    ಯಾವತ್ (classDtypep1) {
        ಧ್ರುವ VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        ಯದಿ (castable == VCastable::COMPATIBLE) ಪ್ರತ್ಯಾಗಮ classDtypep1;
        AstClassExtends* ಧ್ರುವ extendsp = classDtypep1->classp()->extendsp();
        ಯದಿ (!extendsp) ಭಙ್ಗ;
        Ast_ಗಂಟು_DType* ಧ್ರುವ edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    ಪ್ರತ್ಯಾಗಮ nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

ವರ್ಗ VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<ಪೂರ್ಣಾಙ್ಕ, std::string> m_internedWordWidths;  // Interned widths
    std::map<ಪೂರ್ಣಾಙ್ಕ, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ಧ್ರುವ ತಾರ್ಕಿಕ m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    ಸ್ಥಿರ std::string toLetters(size_t ಮೌಲ್ಯ, ತಾರ್ಕಿಕ lowerCase = ಅಸತ್ಯ) {
        ಧ್ರುವ ವರ್ಣ base = lowerCase ? 'a' : 'A';
        std::string s;
        ಕುರು { s += static_cast<ವರ್ಣ>(base + ಮೌಲ್ಯ % 26); } ಯಾವತ್ (ಮೌಲ್ಯ /= 26);
        ಪ್ರತ್ಯಾಗಮ s;
    }

    ಧ್ರುವ std::string& internConst(ಧ್ರುವ AstConst& nodep) {
        ಧ್ರುವ auto pair = m_internedConsts.emplace(nodep.ಅಂಕೆ().ascii(ಅಸತ್ಯ), "");
        ಯದಿ (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಧ್ರುವ std::string& internWordWidth(ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) {
        ಧ್ರುವ auto pair = m_internedWordWidths.emplace(ಮೌಲ್ಯ, "");
        ಯದಿ (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, ಸತ್ಯ);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ಧ್ರುವ std::string& internWideWidth(ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) {
        ಧ್ರುವ auto pair = m_internedWideWidths.emplace(ಮೌಲ್ಯ, "");
        ಯದಿ (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isLeaf(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಧ್ರುವ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        ಪ್ರತಿ (ಧ್ರುವ VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            ಯದಿ (opType != VNTypeInfo::OP_UNUSED) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Render operand, return true if not unused
    ಶೂನ್ಯ renderOperand(VNTypeInfo::OpEn opType, ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ opp, uint32_t depth) {
        ವಿಕಲ್ಪ (opType) {
        ಪಕ್ಷ VNTypeInfo::OP_UNUSED:  //
            ಭಙ್ಗ;
        ಪಕ್ಷ VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            ಭಙ್ಗ;
        ಪಕ್ಷ VNTypeInfo::OP_LIST:
            m_os << " [";
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* nodep = opp; nodep; nodep = nodep->nextp()) {
                ಯದಿ (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            ಭಙ್ಗ;
        ಪಕ್ಷ VNTypeInfo::OP_OPTIONAL:
            ಯದಿ (opp) {
                m_os << " ";
                render(opp, depth);
            } ಅನ್ಯಥಾ {
                m_os << " nil";
            }
            ಭಙ್ಗ;
        }
    }

    // Render the node into the stream.
    ಶೂನ್ಯ render(ಧ್ರುವ Ast_ಗಂಟು* nodep, uint32_t depth) {
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            ಯದಿ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ಅನ್ಯಥಾ ಯದಿ (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } ಅನ್ಯಥಾ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ಅನ್ಯಥಾ ಯದಿ (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } ಅನ್ಯಥಾ ಯದಿ (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } ಅನ್ಯಥಾ {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ಧ್ರುವ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        ಯದಿ (!dtypep) {
            m_os << '?';
        } ಅನ್ಯಥಾ ಯದಿ (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->width();
            ಯದಿ (width == 1) {
                m_os << '1';
            } ಅನ್ಯಥಾ ಯದಿ (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } ಅನ್ಯಥಾ {
                m_os << internWideWidth(width);
            }
            ಯದಿ (m_dumpWidthMin) {
                m_os << '/';
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthMin = nodep->widthMin();
                ಯದಿ (widthMin == 1) {
                    m_os << '1';
                } ಅನ್ಯಥಾ ಯದಿ (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } ಅನ್ಯಥಾ {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

ಸಾರ್ವಜನಿಕ:
    VNPatternString(std::ostream& os, ಧ್ರುವ Ast_ಗಂಟು* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::patternString(uint32_t depth) ಧ್ರುವ {
    std::ostringstream oss;
    VNPatternString{oss, ಏತತ್, depth};
    ಪ್ರತ್ಯಾಗಮ oss.str();
}

//######################################################################
// VNDeleter

ಶೂನ್ಯ VNDeleter::doDeletes() {
    ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ nodep : m_deleteps) nodep->delete_ವೃಕ್ಷ();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#ಅನ್ತರ್ಭಾವ "V3Ast__gen_visitor_defns.h"  // From ./astgen
