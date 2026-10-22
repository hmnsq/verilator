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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3Broken.h"
#അന്തര്ഭാവ "V3File.h"

#അന്തര്ഭാവ <iomanip>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_കണ്ണി::s_editCntLast = 0;
uint64_t Ast_കണ്ണി::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
പൂർണ്ണസംഖ്യ Ast_കണ്ണി::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

തർക്കം VNUser1InUse::s_userBusy = അസത്യം;
തർക്കം VNUser2InUse::s_userBusy = അസത്യം;
തർക്കം VNUser3InUse::s_userBusy = അസത്യം;
തർക്കം VNUser4InUse::s_userBusy = അസത്യം;

പൂർണ്ണസംഖ്യ Ast_കണ്ണി_DType::s_unique_അക്കം = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(സ്ഥിരം string& name) {
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < _ENUM_MAX; ++i) {
        സ്ഥിരം VCMethod method{i};
        എങ്കിൽ (name == method.ascii()) തിരികെ method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    തിരികെ VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(സ്ഥിരം ശൂന്യം*)) സ്ഥിരം {
#ifdef VL_USER_TYPE_CHECKS
    എങ്കിൽ (സ്ഥിരം uint64_t* സ്ഥിരം uip = std::get_if<uint64_t>(&m_u)) തിരികെ "#"s + cvtToStr(*uip);
    എങ്കിൽ (ശൂന്യം* സ്ഥിരം* സ്ഥിരം upp = std::get_if<ശൂന്യം*>(&m_u)) തിരികെ fmtAddrp(*upp);
    തിരികെ "";
#അല്ലെങ്കിൽ
    // Dumps void* representation
    എങ്കിൽ (!m_u.up) തിരികെ "";
    തിരികെ fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

സ്ഥിരം VNTypeInfo VNType::s_typeInfoTable[VNType::അക്കം_TYPES()] = {
#അന്തര്ഭാവ "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

തർക്കം VFwdType::is_കണ്ണി_Compatible(സ്ഥിരം Ast_കണ്ണി* nodep) സ്ഥിരം {
    സ്ഥിരം Ast_കണ്ണി* defp = nodep;
    എങ്കിൽ (സ്ഥിരം AstTypedef* സ്ഥിരം adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം adefp = VN_CAST(defp, കണ്ണി_DType))
        defp = adefp->skipRefToNonRefp();
    തിരഞ്ഞെടുപ്പ് (m_e) {
    അവസ്ഥ VFwdType::NONE: തിരികെ സത്യം; നിർത്തുക;
    അവസ്ഥ VFwdType::ENUM: തിരികെ VN_IS(defp, EnumDType); നിർത്തുക;
    അവസ്ഥ VFwdType::STRUCT: തിരികെ VN_IS(defp, StructDType); നിർത്തുക;
    അവസ്ഥ VFwdType::UNION: തിരികെ VN_IS(defp, UnionDType); നിർത്തുക;
    അവസ്ഥ VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    അവസ്ഥ VFwdType::CLASS: തിരികെ VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); നിർത്തുക;
    അവസ്ഥ VFwdType::GENERIC_INTERFACE: തിരികെ VN_IS(defp, IfaceRefDType); നിർത്തുക;
    സ്ഥിരസ്ഥിതി: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    തിരികെ അസത്യം;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

സ്ഥിരം std::shared_ptr<സ്ഥിരം string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
സ്ഥിരം std::shared_ptr<സ്ഥിരം string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(തർക്കം useSelfForThis, സ്ഥിരം string& text) {
    തിരികെ useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(തർക്കം useSelfForThis, തർക്കം protect) സ്ഥിരം {
    തിരികെ VIdProtect::protect_വാക്കുകൾ_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_കണ്ണി::Ast_കണ്ണി(VNType t, File_വരി* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = ഇത്;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = അസത്യം;
    m_flags.doingWidth = അസത്യം;
    m_flags.protect = സത്യം;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_എണ്ണം_Inc();
}

Ast_കണ്ണി* Ast_കണ്ണി::abovep() സ്ഥിരം {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), ഇത്, "abovep() not allowed when in midlist");
    സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം firstp = firstAbovep() ? ഇത് : m_headtailp;
    തിരികെ firstp->backp();
}
Ast_കണ്ണി* Ast_കണ്ണി::aboveLoopp() സ്ഥിരം {
    // Returns parent node. Avoid using this, may have performance issues.
    സ്ഥിരം Ast_കണ്ണി* nodep = ഇത്;
    // Backwards over peers (versus parents)
    ആകുമ്പോൾ (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    തിരികെ nodep->backp();
}

string Ast_കണ്ണി::encodeName(സ്ഥിരം string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    വേണ്ടി (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        എങ്കിൽ ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } അല്ലെങ്കിൽ എങ്കിൽ (pos[0] == '_') {
            out += pos[0];
            എങ്കിൽ (pos + 1 == namein.end()) നിർത്തുക;
            എങ്കിൽ (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } അല്ലെങ്കിൽ {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            സ്ഥിരം unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    തിരികെ vname.hashedName();
}

string Ast_കണ്ണി::encode_സംഖ്യ(int64_t അക്കം) {
    എങ്കിൽ (അക്കം < 0) തിരികെ "__02D" + cvtToStr(-അക്കം);  // 2D=-
    തിരികെ cvtToStr(അക്കം);
}

string Ast_കണ്ണി::nameProtect() സ്ഥിരം { തിരികെ VIdProtect::protectIf(name(), protect()); }
string Ast_കണ്ണി::origNameProtect() സ്ഥിരം { തിരികെ VIdProtect::protectIf(origName(), protect()); }

string Ast_കണ്ണി::shortName() സ്ഥിരം {
    string pretty = name();
    string::size_type pos;
    ആകുമ്പോൾ ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    തിരികെ pretty;
}

string Ast_കണ്ണി::dedotName(സ്ഥിരം string& namein) {
    string pretty = namein;
    string::size_type pos;
    ആകുമ്പോൾ ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    എങ്കിൽ (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    തിരികെ pretty;
}

string Ast_കണ്ണി::nameNoArray(സ്ഥിരം string& namein) {
    തിരികെ namein.substr(0, namein.find("__BRA__"));
}

string Ast_കണ്ണി::vcdName(സ്ഥിരം string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    ആകുമ്പോൾ ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    ആകുമ്പോൾ ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    തിരികെ prettyName(pretty);
}

string Ast_കണ്ണി::prettyName(സ്ഥിരം string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    വേണ്ടി (സ്ഥിരം അക്ഷരം* pos = namein.c_str(); *pos;) {
        എങ്കിൽ (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            തുടരുക;
        }
        എങ്കിൽ (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            എങ്കിൽ (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                തുടരുക;
            }
            എങ്കിൽ (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                തുടരുക;
            }
            എങ്കിൽ (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                തുടരുക;
            }
            എങ്കിൽ (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                തുടരുക;
            }
            എങ്കിൽ (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                തുടരുക;
            }
            എങ്കിൽ (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                തുടരുക;
            }
            എങ്കിൽ (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                അക്ഷരം മൂല്യം = 0;
                മൂല്യം += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                മൂല്യം
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += മൂല്യം;
                pos += 5;
                തുടരുക;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    എങ്കിൽ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    എങ്കിൽ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    തിരികെ pretty;
}

string Ast_കണ്ണി::vpiName(സ്ഥിരം string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    സ്ഥിരം size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    തർക്കം inEscapedIdent = അസത്യം;
    പൂർണ്ണസംഖ്യ lastIdent = 0;

    വേണ്ടി (സ്ഥിരം അക്ഷരം* pos = namein.c_str() + offset; *pos;) {
        അക്ഷരം specialChar = 0;
        എങ്കിൽ (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } അല്ലെങ്കിൽ എങ്കിൽ (pos[0] == '_' && pos[1] == '_') {  // __
            എങ്കിൽ (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } അല്ലെങ്കിൽ എങ്കിൽ (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } അല്ലെങ്കിൽ എങ്കിൽ (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } അല്ലെങ്കിൽ എങ്കിൽ (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                തുടരുക;
            } അല്ലെങ്കിൽ എങ്കിൽ (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                അക്ഷരം മൂല്യം = 0;
                മൂല്യം += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                മൂല്യം
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                എങ്കിൽ (മൂല്യം != '_') inEscapedIdent = സത്യം;

                pretty += മൂല്യം;
                pos += 5;
                തുടരുക;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        എങ്കിൽ (specialChar) {
            എങ്കിൽ (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.തിരുകുക(lastIdent, "\\");
                inEscapedIdent = അസത്യം;
            }

            pretty += specialChar;

            എങ്കിൽ (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = അസത്യം;
            }
        } അല്ലെങ്കിൽ {
            pretty += pos[0];
            ++pos;
        }
    }
    എങ്കിൽ (inEscapedIdent) {
        pretty += " ";
        pretty.തിരുകുക(lastIdent, "\\");
    }
    എങ്കിൽ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    എങ്കിൽ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    തിരികെ pretty;
}

string Ast_കണ്ണി::prettyTypeName() സ്ഥിരം {
    എങ്കിൽ (name() == "") തിരികെ typeName();
    തിരികെ std::string{typeName()} + " '" + pretty_Name_സന്ദേശം() + "'";
}

//######################################################################
// Insertion

ശൂന്യം Ast_കണ്ണി::debug_വൃക്ഷം_Change(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം അക്ഷരം* prefix, പൂർണ്ണസംഖ്യ lineno, തർക്കം next) {
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
Ast_കണ്ണി* Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(Ast_കണ്ണി* nodep, Ast_കണ്ണി* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_വൃക്ഷം_Change(nodep, "-addNextThs: ", __LINE__, അസത്യം);
    debug_വൃക്ഷം_Change(newp, "-addNextNew: ", __LINE__, സത്യം);
    എങ്കിൽ (!nodep)  // verilog.y and lots of other places assume this
        തിരികെ newp;

    // Find end of old list
    Ast_കണ്ണി* oldtailp = nodep;
    എങ്കിൽ (oldtailp->m_nextp) {
        എങ്കിൽ (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } അല്ലെങ്കിൽ {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            ആകുമ്പോൾ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_കണ്ണി* സ്ഥിരം newtailp = newp->m_headtailp;
    Ast_കണ്ണി* സ്ഥിരം headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_എണ്ണം_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_വൃക്ഷം_Change(nodep, "-addNextOut:", __LINE__, സത്യം);
    തിരികെ nodep;
}

ശൂന്യം Ast_കണ്ണി::addNextHere(Ast_കണ്ണി* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_വൃക്ഷം_Change(ഇത്, "-addHereThs: ", __LINE__, അസത്യം);
    debug_വൃക്ഷം_Change(newp, "-addHereNew: ", __LINE__, സത്യം);
    newp->edit_എണ്ണം_Inc();

    Ast_കണ്ണി* സ്ഥിരം addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_കണ്ണി* സ്ഥിരം oldnextp = ഇത്->m_nextp;
    ഇത്->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    എങ്കിൽ (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = ഇത്;

    // Head/tail
    Ast_കണ്ണി* സ്ഥിരം oldheadtailp = ഇത്->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    എങ്കിൽ (oldheadtailp) {  // else in middle of list, no change
        എങ്കിൽ (oldheadtailp == ഇത്) {  // this was one node
            ഇത്->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } അല്ലെങ്കിൽ എങ്കിൽ (!oldnextp) {  // this was tail
            ഇത്->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_വൃക്ഷം_Change(ഇത്, "-addHereOut: ", __LINE__, സത്യം);
}

ശൂന്യം Ast_കണ്ണി::setOp1p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, ഇത്, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_വൃക്ഷം_Change(ഇത്, "-setOp1pThs: ", __LINE__, അസത്യം);
    debug_വൃക്ഷം_Change(newp, "-setOp1pNew: ", __LINE__, സത്യം);
    m_op1p = newp;
    newp->edit_എണ്ണം_Inc();
    newp->m_backp = ഇത്;
    debug_വൃക്ഷം_Change(ഇത്, "-setOp1pOut: ", __LINE__, അസത്യം);
}

ശൂന്യം Ast_കണ്ണി::setOp2p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, ഇത്, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_വൃക്ഷം_Change(ഇത്, "-setOp2pThs: ", __LINE__, അസത്യം);
    debug_വൃക്ഷം_Change(newp, "-setOp2pNew: ", __LINE__, സത്യം);
    m_op2p = newp;
    newp->edit_എണ്ണം_Inc();
    newp->m_backp = ഇത്;
    debug_വൃക്ഷം_Change(ഇത്, "-setOp2pOut: ", __LINE__, അസത്യം);
}

ശൂന്യം Ast_കണ്ണി::setOp3p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, ഇത്, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_വൃക്ഷം_Change(ഇത്, "-setOp3pThs: ", __LINE__, അസത്യം);
    debug_വൃക്ഷം_Change(newp, "-setOp3pNew: ", __LINE__, സത്യം);
    m_op3p = newp;
    newp->edit_എണ്ണം_Inc();
    newp->m_backp = ഇത്;
    debug_വൃക്ഷം_Change(ഇത്, "-setOp3pOut: ", __LINE__, അസത്യം);
}

ശൂന്യം Ast_കണ്ണി::setOp4p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, ഇത്, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_വൃക്ഷം_Change(ഇത്, "-setOp4pThs: ", __LINE__, അസത്യം);
    debug_വൃക്ഷം_Change(newp, "-setOp4pNew: ", __LINE__, സത്യം);
    m_op4p = newp;
    newp->edit_എണ്ണം_Inc();
    newp->m_backp = ഇത്;
    debug_വൃക്ഷം_Change(ഇത്, "-setOp4pOut: ", __LINE__, അസത്യം);
}

ശൂന്യം Ast_കണ്ണി::addOp1p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    എങ്കിൽ (!m_op1p) {
        op1p(newp);
    } അല്ലെങ്കിൽ {
        m_op1p->addNext(newp);
    }
}

ശൂന്യം Ast_കണ്ണി::addOp2p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    എങ്കിൽ (!m_op2p) {
        op2p(newp);
    } അല്ലെങ്കിൽ {
        m_op2p->addNext(newp);
    }
}

ശൂന്യം Ast_കണ്ണി::addOp3p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    എങ്കിൽ (!m_op3p) {
        op3p(newp);
    } അല്ലെങ്കിൽ {
        m_op3p->addNext(newp);
    }
}

ശൂന്യം Ast_കണ്ണി::addOp4p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    എങ്കിൽ (!m_op4p) {
        op4p(newp);
    } അല്ലെങ്കിൽ {
        m_op4p->addNext(newp);
    }
}

ശൂന്യം Ast_കണ്ണി::prependOp1p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    എങ്കിൽ (!m_op1p) {
        op1p(newp);
    } അല്ലെങ്കിൽ {
        m_op1p->addHereThisAsNext(newp);
    }
}

ശൂന്യം Ast_കണ്ണി::prependOp2p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    എങ്കിൽ (!m_op2p) {
        op2p(newp);
    } അല്ലെങ്കിൽ {
        m_op2p->addHereThisAsNext(newp);
    }
}

ശൂന്യം Ast_കണ്ണി::prependOp3p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    എങ്കിൽ (!m_op3p) {
        op3p(newp);
    } അല്ലെങ്കിൽ {
        m_op3p->addHereThisAsNext(newp);
    }
}

ശൂന്യം Ast_കണ്ണി::prependOp4p(Ast_കണ്ണി* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    എങ്കിൽ (!m_op4p) {
        op4p(newp);
    } അല്ലെങ്കിൽ {
        m_op4p->addHereThisAsNext(newp);
    }
}

ശൂന്യം Ast_കണ്ണി::replaceWith(Ast_കണ്ണി* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    ഇത്->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
ശൂന്യം Ast_കണ്ണി::replaceWithKeepDType(Ast_കണ്ണി* newp) {
    newp->dtypeFrom(ഇത്);
    replaceWith(newp);
}

ശൂന്യം VNRelinker::dump(std::ostream& str) സ്ഥിരം {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_കണ്ണി* Ast_കണ്ണി::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_വൃക്ഷം_Change(ഇത്, "-unlinkWNextThs: ", __LINE__, സത്യം);
    Ast_കണ്ണി* സ്ഥിരം oldp = ഇത്;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_എണ്ണം_Inc();
    // cppcheck-suppress shadowFunction
    Ast_കണ്ണി* സ്ഥിരം backp = oldp->m_backp;
    എങ്കിൽ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        എങ്കിൽ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } അല്ലെങ്കിൽ {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    എങ്കിൽ (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_കണ്ണി* oldtailp = backp->m_headtailp;
        എങ്കിൽ (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            ആകുമ്പോൾ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_കണ്ണി* സ്ഥിരം oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } അല്ലെങ്കിൽ {
        ഇത്->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    എങ്കിൽ (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_വൃക്ഷം_Change(oldp, "-unlinkWNextOut: ", __LINE__, സത്യം);
    തിരികെ oldp;
}

Ast_കണ്ണി* Ast_കണ്ണി::unlinkFrBack(VNRelinker* linkerp) {
    debug_വൃക്ഷം_Change(ഇത്, "-unlinkFrBkThs: ", __LINE__, സത്യം);
    Ast_കണ്ണി* സ്ഥിരം oldp = ഇത്;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_എണ്ണം_Inc();
    // cppcheck-suppress shadowFunction
    Ast_കണ്ണി* സ്ഥിരം backp = oldp->m_backp;
    എങ്കിൽ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        എങ്കിൽ (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        എങ്കിൽ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } അല്ലെങ്കിൽ {
            ഇത്->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    എങ്കിൽ (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        എങ്കിൽ (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        എങ്കിൽ (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } അല്ലെങ്കിൽ {
            ഇത്->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        എങ്കിൽ (oldp->m_nextp) {
            Ast_കണ്ണി* സ്ഥിരം newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    എങ്കിൽ (oldp->m_iterpp) {  // Only if no linker, point to next in list
        എങ്കിൽ (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_വൃക്ഷം_Change(oldp, "-unlinkFrBkOut: ", __LINE__, സത്യം);
    തിരികെ oldp;
}

ശൂന്യം Ast_കണ്ണി::relink(VNRelinker* linkerp) {
    എങ്കിൽ (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_കണ്ണി* സ്ഥിരം newp = ഇത്;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_എണ്ണം_Inc();

    എങ്കിൽ (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_കണ്ണി* സ്ഥിരം backp = linkerp->m_backp;
    debug_വൃക്ഷം_Change(ഇത്, "-relinkNew: ", __LINE__, സത്യം);
    debug_വൃക്ഷം_Change(backp, "-relinkTre: ", __LINE__, സത്യം);

    തിരഞ്ഞെടുപ്പ് (linkerp->m_chg) {
    അവസ്ഥ VNRelinker::RELINK_NEXT: backp->addNextHere(newp); നിർത്തുക;
    അവസ്ഥ VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); നിർത്തുക;
    അവസ്ഥ VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); നിർത്തുക;
    അവസ്ഥ VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); നിർത്തുക;
    അവസ്ഥ VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); നിർത്തുക;
    സ്ഥിരസ്ഥിതി: ഇത്->v3fatalSrc("Relink of node without any link to change."); നിർത്തുക;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    എങ്കിൽ (linkerp->m_iterpp) {
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
    debug_വൃക്ഷം_Change(ഇത്, "-relinkOut: ", __LINE__, സത്യം);
}

ശൂന്യം Ast_കണ്ണി::relinkOneLink(Ast_കണ്ണി*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_കണ്ണി* newp) {
    എങ്കിൽ (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_കണ്ണി* സ്ഥിരം newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_കണ്ണി* സ്ഥിരം oldlistlastp = pointpr->m_headtailp;
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

ശൂന്യം Ast_കണ്ണി::addHereThisAsNext(Ast_കണ്ണി* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(ഇത്->m_backp, ഇത്, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_കണ്ണി* സ്ഥിരം backp = ഇത്->m_backp;
    Ast_കണ്ണി* സ്ഥിരം newLastp = newp->m_headtailp;
    //
    ഇത്->edit_എണ്ണം_Inc();
    // Common linkage
    newLastp->m_nextp = ഇത്;
    ഇത്->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    എങ്കിൽ (backp && backp->m_nextp == ഇത്) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } അല്ലെങ്കിൽ {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        എങ്കിൽ (backp) {
            എങ്കിൽ (backp->m_op1p == ഇത്) {
                backp->m_op1p = newp;
            } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op2p == ഇത്) {
                backp->m_op2p = newp;
            } അല്ലെങ്കിൽ എങ്കിൽ (backp->m_op3p == ഇത്) {
                backp->m_op3p = newp;
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(backp->m_op4p == ഇത്, ഇത്, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_കണ്ണി* സ്ഥിരം tailp = ഇത്->m_headtailp;
        ഇത്->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    എങ്കിൽ (newLastp->m_iterpp) *(newLastp->m_iterpp) = ഇത്;
    എങ്കിൽ (ഇത്->m_iterpp) {
        *(ഇത്->m_iterpp) = newp;
        ഇത്->m_iterpp = nullptr;
    }
    //
    debug_വൃക്ഷം_Change(ഇത്, "-addHereThisAsNext: ", __LINE__, സത്യം);
}

//======================================================================
// Clone

Ast_കണ്ണി* Ast_കണ്ണി::clone_വൃക്ഷം_Iter(തർക്കം needPure) {
    // private: Clone single node and children
    എങ്കിൽ (needPure) purityCheck();
    Ast_കണ്ണി* സ്ഥിരം newp = ഇത്->clone();
    എങ്കിൽ (ഇത്->m_op1p) newp->op1p(ഇത്->m_op1p->clone_വൃക്ഷം_Iter_List(needPure));
    എങ്കിൽ (ഇത്->m_op2p) newp->op2p(ഇത്->m_op2p->clone_വൃക്ഷം_Iter_List(needPure));
    എങ്കിൽ (ഇത്->m_op3p) newp->op3p(ഇത്->m_op3p->clone_വൃക്ഷം_Iter_List(needPure));
    എങ്കിൽ (ഇത്->m_op4p) newp->op4p(ഇത്->m_op4p->clone_വൃക്ഷം_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(ഇത്);  // Save pointers to/from both to simplify relinking.
    ഇത്->clonep(newp);  // Save pointers to/from both to simplify relinking.
    തിരികെ newp;
}

Ast_കണ്ണി* Ast_കണ്ണി::clone_വൃക്ഷം_Iter_List(തർക്കം needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_കണ്ണി* newheadp = nullptr;
    Ast_കണ്ണി* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    വേണ്ടി (Ast_കണ്ണി* oldp = ഇത്; oldp; oldp = oldp->m_nextp) {
        Ast_കണ്ണി* സ്ഥിരം newp = oldp->clone_വൃക്ഷം_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        എങ്കിൽ (newtailp) newtailp->m_nextp = newp;
        എങ്കിൽ (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    തിരികെ newheadp;
}

Ast_കണ്ണി* Ast_കണ്ണി::clone_വൃക്ഷം(തർക്കം cloneNextLink, തർക്കം needPure) {
    debug_വൃക്ഷം_Change(ഇത്, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_വൃക്ഷം();
    Ast_കണ്ണി* newp;
    എങ്കിൽ (cloneNextLink && ഇത്->m_nextp) {
        newp = clone_വൃക്ഷം_Iter_List(needPure);
    } അല്ലെങ്കിൽ {
        newp = clone_വൃക്ഷം_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_വൃക്ഷം();
    debug_വൃക്ഷം_Change(newp, "-cloneOut: ", __LINE__, സത്യം);
    തിരികെ newp;
}

ശൂന്യം Ast_കണ്ണി::purityCheck() {
    എങ്കിൽ (VL_UNLIKELY(!isPure())) {
        ഇത്->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << ഇത്->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

ശൂന്യം Ast_കണ്ണി::delete_കണ്ണി() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_എണ്ണം_Inc();
    // Change links of old node so we coredump if used
    ഇത്->m_nextp = reinterpret_cast<Ast_കണ്ണി*>(0x1);
    ഇത്->m_backp = reinterpret_cast<Ast_കണ്ണി*>(0x1);
    ഇത്->m_headtailp = reinterpret_cast<Ast_കണ്ണി*>(0x1);
    ഇത്->m_op1p = reinterpret_cast<Ast_കണ്ണി*>(0x1);
    ഇത്->m_op2p = reinterpret_cast<Ast_കണ്ണി*>(0x1);
    ഇത്->m_op3p = reinterpret_cast<Ast_കണ്ണി*>(0x1);
    ഇത്->m_op4p = reinterpret_cast<Ast_കണ്ണി*>(0x1);
    ഇത്->m_iterpp = reinterpret_cast<Ast_കണ്ണി**>(0x1);
    എങ്കിൽ (
#എങ്കിൽ !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        സത്യം
#അല്ലെങ്കിൽ
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete ഇത്;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

ശൂന്യം Ast_കണ്ണി::delete_വൃക്ഷം_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    വേണ്ടി (Ast_കണ്ണി *nodep = ഇത്, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        എങ്കിൽ (nodep->m_op1p) nodep->m_op1p->delete_വൃക്ഷം_Iter();
        എങ്കിൽ (nodep->m_op2p) nodep->m_op2p->delete_വൃക്ഷം_Iter();
        എങ്കിൽ (nodep->m_op3p) nodep->m_op3p->delete_വൃക്ഷം_Iter();
        എങ്കിൽ (nodep->m_op4p) nodep->m_op4p->delete_വൃക്ഷം_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_കണ്ണി();
    }
}

ശൂന്യം Ast_കണ്ണി::delete_വൃക്ഷം() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_വൃക്ഷം_Change(ഇത്, "-delTree:  ", __LINE__, സത്യം);
    ഇത്->edit_എണ്ണം_Inc();
    // MUST be depth first!
    delete_വൃക്ഷം_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
ശൂന്യം* Ast_കണ്ണി::operator പുതിയ(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_കണ്ണി* സ്ഥിരം objp = static_cast<Ast_കണ്ണി*>(::operator പുതിയ(size));
    V3Broken::addNewed(objp);
    തിരികെ objp;
}

ശൂന്യം Ast_കണ്ണി::operator delete(ശൂന്യം* objp, size_t size) {
    എങ്കിൽ (!objp) തിരികെ;
    സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം nodep = static_cast<Ast_കണ്ണി*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
ശൂന്യം* Ast_കണ്ണി::operator പുതിയ(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    സ്ഥിതം സ്ഥിരം size_t ASTGEN_MAX_കണ്ണി_SIZE = []() {
        size_t maxSize = 0;
        വേണ്ടി (size_t t = 0; t < VNType::അക്കം_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        തിരികെ maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    സ്ഥിതം uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    എങ്കിൽ (പൂർണ്ണസംഖ്യ seed = v3Global.opt.debugAllocRandom()) {
        സ്ഥിതം V3Mutex s_mutex;
        സ്ഥിരം V3LockGuard lock{s_mutex};
        സ്ഥിതം std::array<ശൂന്യം*, POOL_SIZE> s_കണ്ണി_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_കണ്ണി_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        എങ്കിൽ (!s_lfsr) {
            s_lfsr = seed;
            എങ്കിൽ (!s_lfsr) s_lfsr = ~s_lfsr;
            വേണ്ടി (size_t i = 0; i < POOL_SIZE; ++i) {
                s_കണ്ണി_Pool[i] = ::operator പുതിയ(ASTGEN_MAX_കണ്ണി_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_കണ്ണി_Pool), std::end(s_കണ്ണി_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        സ്ഥിരം size_t index = s_lfsr % POOL_SIZE;
        Ast_കണ്ണി* സ്ഥിരം objp = static_cast<Ast_കണ്ണി*>(s_കണ്ണി_Pool[index]);
        s_കണ്ണി_Pool[index] = ::operator പുതിയ(ASTGEN_MAX_കണ്ണി_SIZE + 64);  // For later new()
        തിരികെ objp;
    } അല്ലെങ്കിൽ {
        Ast_കണ്ണി* സ്ഥിരം objp = static_cast<Ast_കണ്ണി*>(::operator പുതിയ(size));
        തിരികെ objp;
    }
}
ശൂന്യം Ast_കണ്ണി::operator delete(ശൂന്യം* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (ശൂന്യം)objp;
    (ശൂന്യം)size;
}
#endif

//======================================================================
// Iterators

ശൂന്യം Ast_കണ്ണി::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    എങ്കിൽ (m_op1p) m_op1p->iterateAndNext(v);
    എങ്കിൽ (m_op2p) m_op2p->iterateAndNext(v);
    എങ്കിൽ (m_op3p) m_op3p->iterateAndNext(v);
    എങ്കിൽ (m_op4p) m_op4p->iterateAndNext(v);
}

ശൂന്യം Ast_കണ്ണി::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    എങ്കിൽ (m_op1p) m_op1p->iterateAndNextConst(v);
    എങ്കിൽ (m_op2p) m_op2p->iterateAndNextConst(v);
    എങ്കിൽ (m_op3p) m_op3p->iterateAndNextConst(v);
    എങ്കിൽ (m_op4p) m_op4p->iterateAndNextConst(v);
}

ശൂന്യം Ast_കണ്ണി::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_കണ്ണി* nodep = ഇത്;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    എങ്കിൽ (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    ആകുമ്പോൾ (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        എങ്കിൽ (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_കണ്ണി* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        എങ്കിൽ (!niterp) തിരികെ;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        എങ്കിൽ (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } അല്ലെങ്കിൽ {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

ശൂന്യം Ast_കണ്ണി::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_കണ്ണി* nodep = ഇത്;
    ആകുമ്പോൾ (nodep->m_nextp) nodep = nodep->m_nextp;
    ആകുമ്പോൾ (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        എങ്കിൽ (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } അല്ലെങ്കിൽ {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

ശൂന്യം Ast_കണ്ണി::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    എങ്കിൽ (m_op1p) m_op1p->iterateListBackwardsConst(v);
    എങ്കിൽ (m_op2p) m_op2p->iterateListBackwardsConst(v);
    എങ്കിൽ (m_op3p) m_op3p->iterateListBackwardsConst(v);
    എങ്കിൽ (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

ശൂന്യം Ast_കണ്ണി::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_കണ്ണി* nodep = ഇത്;
    ചെയ്യുക {
        Ast_കണ്ണി* സ്ഥിരം nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } ആകുമ്പോൾ (nodep);
}

Ast_കണ്ണി* Ast_കണ്ണി::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_കണ്ണി* nodep = ഇത്;  // Note "this" may point to bogus point later in this function
    എങ്കിൽ (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* സ്ഥിരം tempp = പുതിയ AstBegin{nodep->fileline(), "[EditWrapper]", nodep, അസത്യം};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_വൃക്ഷം(), tempp);
    } അല്ലെങ്കിൽ {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_കണ്ണി** nextnodepp = nullptr;
        എങ്കിൽ (ഇത്->m_backp->m_op1p == ഇത്) {
            nextnodepp = &(ഇത്->m_backp->m_op1p);
        } അല്ലെങ്കിൽ എങ്കിൽ (ഇത്->m_backp->m_op2p == ഇത്) {
            nextnodepp = &(ഇത്->m_backp->m_op2p);
        } അല്ലെങ്കിൽ എങ്കിൽ (ഇത്->m_backp->m_op3p == ഇത്) {
            nextnodepp = &(ഇത്->m_backp->m_op3p);
        } അല്ലെങ്കിൽ എങ്കിൽ (ഇത്->m_backp->m_op4p == ഇത്) {
            nextnodepp = &(ഇത്->m_backp->m_op4p);
        } അല്ലെങ്കിൽ എങ്കിൽ (ഇത്->m_backp->m_nextp == ഇത്) {
            nextnodepp = &(ഇത്->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, ഇത്, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    തിരികെ nodep;
}

//======================================================================

ശൂന്യം Ast_കണ്ണി::clone_Relink_വൃക്ഷം() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    വേണ്ടി (Ast_കണ്ണി* nodep = ഇത്; nodep; nodep = nodep->m_nextp) {
        എങ്കിൽ (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        എങ്കിൽ (nodep->m_op1p) nodep->m_op1p->clone_Relink_വൃക്ഷം();
        എങ്കിൽ (nodep->m_op2p) nodep->m_op2p->clone_Relink_വൃക്ഷം();
        എങ്കിൽ (nodep->m_op3p) nodep->m_op3p->clone_Relink_വൃക്ഷം();
        എങ്കിൽ (nodep->m_op4p) nodep->m_op4p->clone_Relink_വൃക്ഷം();
    }
}

//======================================================================
// Comparison

തർക്കം Ast_കണ്ണി::gate_വൃക്ഷം_Iter() സ്ഥിരം {
    // private: Return true if the two trees are identical
    എങ്കിൽ (!isGateOptimizable()) തിരികെ അസത്യം;
    എങ്കിൽ (m_op1p && !m_op1p->gate_വൃക്ഷം_Iter()) തിരികെ അസത്യം;
    എങ്കിൽ (m_op2p && !m_op2p->gate_വൃക്ഷം_Iter()) തിരികെ അസത്യം;
    എങ്കിൽ (m_op3p && !m_op3p->gate_വൃക്ഷം_Iter()) തിരികെ അസത്യം;
    എങ്കിൽ (m_op4p && !m_op4p->gate_വൃക്ഷം_Iter()) തിരികെ അസത്യം;
    തിരികെ സത്യം;
}

തർക്കം Ast_കണ്ണി::same_വൃക്ഷം_Iter(സ്ഥിരം Ast_കണ്ണി* node1p, സ്ഥിരം Ast_കണ്ണി* node2p, തർക്കം ignNext,
                           തർക്കം gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    ആകുമ്പോൾ (സത്യം) {
        എങ്കിൽ (!node1p && !node2p) തിരികെ സത്യം;
        എങ്കിൽ (!node1p || !node2p) തിരികെ അസത്യം;
        എങ്കിൽ (node1p->type() != node2p->type()) തിരികെ അസത്യം;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        എങ്കിൽ (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) തിരികെ അസത്യം;
        എങ്കിൽ (!node1p->same_കണ്ണി(node2p) || (gateOnly && !node1p->isGateOptimizable())) തിരികെ അസത്യം;
        എങ്കിൽ (!same_വൃക്ഷം_Iter(node1p->m_op1p, node2p->m_op1p, അസത്യം, gateOnly)
            || !same_വൃക്ഷം_Iter(node1p->m_op2p, node2p->m_op2p, അസത്യം, gateOnly)
            || !same_വൃക്ഷം_Iter(node1p->m_op3p, node2p->m_op3p, അസത്യം, gateOnly)
            || !same_വൃക്ഷം_Iter(node1p->m_op4p, node2p->m_op4p, അസത്യം, gateOnly)) {
            തിരികെ അസത്യം;
        }
        എങ്കിൽ (ignNext) തിരികെ സത്യം;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

ശൂന്യം Ast_കണ്ണി::check_വൃക്ഷം_Iter(സ്ഥിരം Ast_കണ്ണി* prevBackp) സ്ഥിരം VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == ഇത്->backp(), ഇത്, "Back node inconsistent");
    സ്ഥിരം VNTypeInfo& typeInfo = VNType::typeInfo(ഇത്->type());
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1; i <= 4; i++) {
        Ast_കണ്ണി* nodep = nullptr;
        തിരഞ്ഞെടുപ്പ് (i) {
        അവസ്ഥ 1: nodep = op1p(); നിർത്തുക;
        അവസ്ഥ 2: nodep = op2p(); നിർത്തുക;
        അവസ്ഥ 3: nodep = op3p(); നിർത്തുക;
        അവസ്ഥ 4: nodep = op4p(); നിർത്തുക;
        സ്ഥിരസ്ഥിതി: ഇത്->v3fatalSrc("Bad case"); നിർത്തുക;
        }
        // cppcheck-suppress danglingTempReference
        സ്ഥിരം അക്ഷരം* opName = typeInfo.m_opNamep[i - 1];
        തിരഞ്ഞെടുപ്പ് (typeInfo.m_opType[i - 1]) {
        അവസ്ഥ VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, ഇത്, typeInfo.m_namep << " must not use " << opName << "()");
            നിർത്തുക;
        അവസ്ഥ VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, ഇത്,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), ഇത്,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_വൃക്ഷം_Iter(ഇത്);
            നിർത്തുക;
        അവസ്ഥ VNTypeInfo::OP_LIST:
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം headp = nodep) {
                // cppcheck-suppress shadowFunction
                സ്ഥിരം Ast_കണ്ണി* backp = ഇത്;
                സ്ഥിരം Ast_കണ്ണി* tailp;
                സ്ഥിരം Ast_കണ്ണി* opp = headp;
                ചെയ്യുക {
                    opp->check_വൃക്ഷം_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } ആകുമ്പോൾ (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            നിർത്തുക;
        അവസ്ഥ VNTypeInfo::OP_OPTIONAL:
            എങ്കിൽ (nodep) {
                UASSERT_OBJ(!nodep->nextp(), ഇത്,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_വൃക്ഷം_Iter(ഇത്);
            }
            നിർത്തുക;
        സ്ഥിരസ്ഥിതി: ഇത്->v3fatalSrc("Bad case"); നിർത്തുക;
        }
    }
    എങ്കിൽ (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_CAST(ഇത്, കണ്ണി_വ്യഞ്ജകം)) {
            സ്ഥിരം അക്ഷരം* സ്ഥിരം whyp = exprp->widthMismatch();
            എങ്കിൽ (whyp) {
                auto dtypeStr = [](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    തിരികെ ss.str();
                };
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Uniop* സ്ഥിരം uniopp = VN_CAST(exprp, കണ്ണി_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം biopp = VN_CAST(exprp, കണ്ണി_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } അല്ലെങ്കിൽ {
                    UASSERT_OBJ(അസത്യം, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
അക്ഷരം* Ast_കണ്ണി::dump_വൃക്ഷം_Json_Gdb(സ്ഥിരം Ast_കണ്ണി* nodep) {
    എങ്കിൽ (!nodep) തിരികെ strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_വൃക്ഷം_Json(nodepStream);
    സ്ഥിരം std::string str = nodepStream.rdbuf()->str();
    തിരികെ strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
അക്ഷരം* Ast_കണ്ണി::dump_വൃക്ഷം_Json_Gdb(സ്ഥിരം അക്ഷരം* str) { തിരികെ strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
അക്ഷരം* Ast_കണ്ണി::dump_വൃക്ഷം_Json_Gdb(intptr_t nodep) {
    എങ്കിൽ (!nodep) തിരികെ strdup("{\"addr\":\"NULL\"}\n");
    തിരികെ dump_വൃക്ഷം_Json_Gdb(reinterpret_cast<സ്ഥിരം Ast_കണ്ണി*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
ശൂന്യം Ast_കണ്ണി::dumpGdb(സ്ഥിരം Ast_കണ്ണി* nodep) {  // For GDB only  // LCOV_EXCL_START
    എങ്കിൽ (!nodep) {
        cout << "<nullptr>" << endl;
        തിരികെ;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ശൂന്യം Ast_കണ്ണി::dumpGdbHeader() സ്ഥിരം {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ശൂന്യം Ast_കണ്ണി::dump_വൃക്ഷം_Gdb(സ്ഥിരം Ast_കണ്ണി* nodep) {  // For GDB only  // LCOV_EXCL_START
    എങ്കിൽ (!nodep) {
        cout << "<nullptr>" << endl;
        തിരികെ;
    }
    nodep->dumpGdbHeader();
    nodep->dump_വൃക്ഷം(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ശൂന്യം Ast_കണ്ണി::dump_വൃക്ഷം_File_Gdb(സ്ഥിരം Ast_കണ്ണി* nodep,  // LCOV_EXCL_START
                              സ്ഥിരം അക്ഷരം* filenamep) {  // For GDB only
    എങ്കിൽ (!nodep) {
        cout << "<nullptr>" << endl;
        തിരികെ;
    }
    സ്ഥിരം string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_വൃക്ഷം_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
ശൂന്യം Ast_കണ്ണി::checkIter() സ്ഥിരം {
    എങ്കിൽ (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        ഇത്->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

ശൂന്യം Ast_കണ്ണി::dumpPtrs(std::ostream& os) സ്ഥിരം {
    os << "This=" << typeName() << " " << cvtToHex(ഇത്);
    os << " back=" << cvtToHex(backp());
    എങ്കിൽ (nextp()) os << " next=" << cvtToHex(nextp());
    എങ്കിൽ (m_headtailp == ഇത്) {
        os << " headtail=this";
    } അല്ലെങ്കിൽ {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    എങ്കിൽ (op1p()) os << " op1p=" << cvtToHex(op1p());
    എങ്കിൽ (op2p()) os << " op2p=" << cvtToHex(op2p());
    എങ്കിൽ (op3p()) os << " op3p=" << cvtToHex(op3p());
    എങ്കിൽ (op4p()) os << " op4p=" << cvtToHex(op4p());
    സ്ഥിരം auto dumpUser = [&os](സ്ഥിരം അക്ഷരം* prefix, സ്ഥിരം VNUser& user) {
        സ്ഥിരം std::string s
            = user.dumpStr([](സ്ഥിരം ശൂന്യം* p) -> std::string { തിരികെ cvtToHex(p); });
        എങ്കിൽ (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    എങ്കിൽ (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

ശൂന്യം Ast_കണ്ണി::dump_വൃക്ഷം(std::ostream& os, സ്ഥിരം string& indent, പൂർണ്ണസംഖ്യ maxDepth) സ്ഥിരം {
    സ്ഥിതം പൂർണ്ണസംഖ്യ s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << ഇത് << '\n';
    എങ്കിൽ (VN_DELETED(ഇത്)) തിരികെ;
    എങ്കിൽ (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    എങ്കിൽ (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(ഇത്)
           << endl;  // endl intentional to do flush
        തിരികെ;
    }  // LCOV_EXCL_STOP
    എങ്കിൽ (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    എങ്കിൽ (maxDepth == 1) {
        എങ്കിൽ (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } അല്ലെങ്കിൽ {
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_വൃക്ഷം(os, indent + "1:", maxDepth - 1);
        }
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_വൃക്ഷം(os, indent + "2:", maxDepth - 1);
        }
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_വൃക്ഷം(os, indent + "3:", maxDepth - 1);
        }
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_വൃക്ഷം(os, indent + "4:", maxDepth - 1);
        }
    }
}

ശൂന്യം Ast_കണ്ണി::dump_വൃക്ഷം_And_Next(std::ostream& os, സ്ഥിരം string& indent, പൂർണ്ണസംഖ്യ maxDepth) സ്ഥിരം {
    // Audited to make sure this is never nullptr
    വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = ഇത്; nodep; nodep = nodep->nextp()) {
        nodep->dump_വൃക്ഷം(os, indent, maxDepth);
    }
}

ശൂന്യം Ast_കണ്ണി::dump_വൃക്ഷം_File(സ്ഥിരം string& filename, തർക്കം doDump) {
    // Not const function as calls checkTree
    എങ്കിൽ (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            സ്ഥിരം std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            എങ്കിൽ (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_എണ്ണം_Last();
            *logsp << "> to <e" << std::dec << edit_എണ്ണം_Gbl() << ">\n";
            എങ്കിൽ (edit_എണ്ണം_Gbl() == edit_എണ്ണം_Last() && ::dump_വൃക്ഷം_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } അല്ലെങ്കിൽ {
                dump_വൃക്ഷം(*logsp);
                edit_എണ്ണം_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

സ്ഥിതം ശൂന്യം drawChildren(std::ostream& os, സ്ഥിരം Ast_കണ്ണി* thisp, സ്ഥിരം Ast_കണ്ണി* childp,
                         സ്ഥിരം std::string& childName) {
    എങ്കിൽ (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_വൃക്ഷം_Dot(os);
            എങ്കിൽ (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

ശൂന്യം Ast_കണ്ണി::dump_വൃക്ഷം_Dot(std::ostream& os) സ്ഥിരം {
    os << "\tn" << cvtToHex(ഇത്) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, ഇത്, m_op1p, "op1");
    drawChildren(os, ഇത്, m_op2p, "op2");
    drawChildren(os, ഇത്, m_op3p, "op3");
    drawChildren(os, ഇത്, m_op4p, "op4");
}

ശൂന്യം Ast_കണ്ണി::dump_വൃക്ഷം_Json_File(സ്ഥിരം string& filename, തർക്കം doDump) {
    എങ്കിൽ (!doDump) തിരികെ;
    UINFO(2, "Dumping " << filename);
    സ്ഥിരം std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    എങ്കിൽ (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_വൃക്ഷം_Json(*treejsonp);
    *treejsonp << '\n';
}

ശൂന്യം Ast_കണ്ണി::dumpJsonMetaFileGdb(സ്ഥിരം അക്ഷരം* filename) { dumpJsonMetaFile(filename); }
ശൂന്യം Ast_കണ്ണി::dumpJsonMetaFile(സ്ഥിരം string& filename) {
    UINFO(2, "Dumping " << filename);
    സ്ഥിരം std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    എങ്കിൽ (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_വരി::file_Name_അക്കം_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

ശൂന്യം Ast_കണ്ണി::dump_വൃക്ഷം_Dot_File(സ്ഥിരം string& filename, തർക്കം doDump) {
    എങ്കിൽ (doDump) {
        UINFO(2, "Dumping " << filename);
        സ്ഥിരം std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        എങ്കിൽ (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_വൃക്ഷം_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_കണ്ണി::instanceStr() സ്ഥിരം {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_എണ്ണം = 0;
    // Walk 'backp' chain
    വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* currp = ഇത്; currp; currp = currp->backp(), ++iter_എണ്ണം) {
        എങ്കിൽ (VL_UNCOVERABLE(iter_എണ്ണം >= maxIterations)) തിരികെ "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        എങ്കിൽ (സ്ഥിരം AstScope* സ്ഥിരം scopep = VN_CAST(currp, Scope)) {
            തിരികെ scopep->is_മുകൾ() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        എങ്കിൽ (സ്ഥിരം AstModule* സ്ഥിരം modp = VN_CAST(currp, Module)) {
            സ്ഥിരം string instanceName = modp->someInstanceName();
            തിരികെ instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    തിരികെ "";
}
ശൂന്യം Ast_കണ്ണി::v3errorEnd(സ്ഥിരം std::ostringstream& str) സ്ഥിരം VL_RELEASE(V3Error::s().m_mutex) {
    എങ്കിൽ (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } അല്ലെങ്കിൽ {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        സ്ഥിരം string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        എങ്കിൽ (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_കണ്ണി*>(ഇത്)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
ശൂന്യം Ast_കണ്ണി::v3errorEndFatal(സ്ഥിരം std::ostringstream& str) സ്ഥിരം
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    പ്രതിജ്ഞാ(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

ശൂന്യം Ast_കണ്ണി::dtypeChgSigned(തർക്കം flag) {
    UASSERT_OBJ(dtypep(), ഇത്, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
ശൂന്യം Ast_കണ്ണി::dtypeChgWidth(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin) {
    UASSERT_OBJ(dtypep(), ഇത്, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

ശൂന്യം Ast_കണ്ണി::dtypeChgWidthSigned(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), ഇത്, "No dtype when changing width");
    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം enumDTypep = dtypep()->skipRefToEnump();
    എങ്കിൽ (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        തിരികെ;  // Correct already
    }
    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_കണ്ണി_DType* Ast_കണ്ണി::findBasicDType(VBasicDTypeKwd kwd) സ്ഥിരം {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    തിരികെ v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findBitDType(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findLogicDType(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findLogic2StateDType(പൂർണ്ണസംഖ്യ width, പൂർണ്ണസംഖ്യ widthMin, VSigning numeric) സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findLogicRangeDType(സ്ഥിരം VNumRange& range, പൂർണ്ണസംഖ്യ widthMin,
                                           VSigning numeric) സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findBitRangeDType(സ്ഥിരം VNumRange& range, പൂർണ്ണസംഖ്യ widthMin,
                                         VSigning numeric) സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_കണ്ണി::find_തിരുകുക_Same_DType(AstBasicDType* nodep) {
    തിരികെ v3Global.rootp()->typeTablep()->find_തിരുകുക_Same_DType(nodep);
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findConstraintRefDType() സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findEmptyQueueDType() സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findQueueIndexDType() സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findStreamDType() സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_കണ്ണി_DType* Ast_കണ്ണി::findVoidDType() സ്ഥിരം {
    തിരികെ v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

സ്ഥിതം സ്ഥിരം Ast_കണ്ണി_DType* computeCastableBase(സ്ഥിരം Ast_കണ്ണി_DType* nodep) {
    ആകുമ്പോൾ (സത്യം) {
        എങ്കിൽ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            തുടരുക;
        }
        എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം refp = nodep->skipRefToEnump()) {
            എങ്കിൽ (refp != nodep) {
                nodep = refp;
                തുടരുക;
            }
        }
        തിരികെ nodep;
    }
}

സ്ഥിതം VCastable computeCastableImp(സ്ഥിരം Ast_കണ്ണി_DType* toDtp, സ്ഥിരം Ast_കണ്ണി_DType* fromDtp,
                                    സ്ഥിരം Ast_കണ്ണി* fromConstp, സ്ഥിരം തർക്കം checkIfaceArgCompat) {
    സ്ഥിരം VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    എങ്കിൽ (toDtp == fromDtp) തിരികെ VCastable::SAMEISH;
    എങ്കിൽ (toDtp->similarDType(fromDtp)) തിരികെ VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    സ്ഥിരം Ast_കണ്ണി_DType* fromBaseDtp = computeCastableBase(fromDtp);

    സ്ഥിരം തർക്കം fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, കണ്ണി_UOr_Struct_DType);

    സ്ഥിരം Ast_കണ്ണി_DType* toBaseDtp = computeCastableBase(toDtp);
    സ്ഥിരം തർക്കം toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, കണ്ണി_UOr_Struct_DType);

    എങ്കിൽ (toBaseDtp == fromBaseDtp) തിരികെ VCastable::COMPATIBLE;

    എങ്കിൽ (toNumericable) {
        എങ്കിൽ (fromNumericable) തിരികെ VCastable::COMPATIBLE;
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(toBaseDtp, EnumDType)) {
        എങ്കിൽ (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_വൃക്ഷം(fromDtp))
            തിരികെ VCastable::ENUM_IMPLICIT;
        എങ്കിൽ (fromNumericable) തിരികെ VCastable::ENUM_EXPLICIT;
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        തിരികെ VCastable::COMPATIBLE;
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        എങ്കിൽ (fromConstp->isNull()) തിരികെ VCastable::COMPATIBLE;
    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        സ്ഥിരം AstClass* സ്ഥിരം toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        സ്ഥിരം AstClass* സ്ഥിരം fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        സ്ഥിരം തർക്കം downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        സ്ഥിരം തർക്കം upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        എങ്കിൽ (upcast) തിരികെ VCastable::COMPATIBLE;
        എങ്കിൽ (downcast) തിരികെ VCastable::DYNAMIC_CLASS;
        തിരികെ VCastable::INCOMPATIBLE;
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(സത്യം) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        സ്ഥിരം AstIfaceRefDType* സ്ഥിരം fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(സത്യം) : fromDtp, IfaceRefDType);
        എങ്കിൽ (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            എങ്കിൽ (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                തിരികെ VCastable::INCOMPATIBLE;
            }
            തിരികെ computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        എങ്കിൽ (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            എങ്കിൽ (!checkIfaceArgCompat) തിരികെ castable;
            തിരികെ VCastable::INCOMPATIBLE;
        }
        സ്ഥിരം തർക്കം sameModport = toIfp->modportp() == fromIfp->modportp();
        എങ്കിൽ (!checkIfaceArgCompat) {
            എങ്കിൽ (!toIfp->modportp() || sameModport) തിരികെ VCastable::COMPATIBLE;
            തിരികെ castable;
        }
        എങ്കിൽ (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            തിരികെ VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        എങ്കിൽ (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            തിരികെ VCastable::COMPATIBLE;
        }
        തിരികെ VCastable::INCOMPATIBLE;
    } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        എങ്കിൽ (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            തിരികെ castable;
        }
        സ്ഥിരം VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം toSubp = toArrayp->subDTypep()->skipRefp();
        എങ്കിൽ (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            തിരികെ VCastable::COMPATIBLE;
        }
    }
    തിരികെ castable;
}

VCastable Ast_കണ്ണി::computeCastable(സ്ഥിരം Ast_കണ്ണി_DType* toDtp, സ്ഥിരം Ast_കണ്ണി_DType* fromDtp,
                                   സ്ഥിരം Ast_കണ്ണി* fromConstp, സ്ഥിരം തർക്കം checkIfaceArgCompat) {
    സ്ഥിരം auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    എങ്കിൽ (fromConstp) UINFO(9, "     const= " << fromConstp);
    തിരികെ castable;
}

Ast_കണ്ണി_DType* Ast_കണ്ണി::getCommonClassTypep(Ast_കണ്ണി* node1p, Ast_കണ്ണി* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    എങ്കിൽ (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        സ്ഥിരം VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        എങ്കിൽ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            തിരികെ node1p->dtypep();
        എങ്കിൽ (castable == VCastable::DYNAMIC_CLASS) തിരികെ node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    ആകുമ്പോൾ (classDtypep1) {
        സ്ഥിരം VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        എങ്കിൽ (castable == VCastable::COMPATIBLE) തിരികെ classDtypep1;
        AstClassExtends* സ്ഥിരം extendsp = classDtypep1->classp()->extendsp();
        എങ്കിൽ (!extendsp) നിർത്തുക;
        Ast_കണ്ണി_DType* സ്ഥിരം edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    തിരികെ nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

വർഗ്ഗം VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<പൂർണ്ണസംഖ്യ, std::string> m_internedWordWidths;  // Interned widths
    std::map<പൂർണ്ണസംഖ്യ, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    സ്ഥിരം തർക്കം m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    സ്ഥിതം std::string toLetters(size_t മൂല്യം, തർക്കം lowerCase = അസത്യം) {
        സ്ഥിരം അക്ഷരം base = lowerCase ? 'a' : 'A';
        std::string s;
        ചെയ്യുക { s += static_cast<അക്ഷരം>(base + മൂല്യം % 26); } ആകുമ്പോൾ (മൂല്യം /= 26);
        തിരികെ s;
    }

    സ്ഥിരം std::string& internConst(സ്ഥിരം AstConst& nodep) {
        സ്ഥിരം auto pair = m_internedConsts.emplace(nodep.അക്കം().ascii(അസത്യം), "");
        എങ്കിൽ (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        തിരികെ pair.first->second;
    }

    സ്ഥിരം std::string& internWordWidth(പൂർണ്ണസംഖ്യ മൂല്യം) {
        സ്ഥിരം auto pair = m_internedWordWidths.emplace(മൂല്യം, "");
        എങ്കിൽ (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, സത്യം);
        തിരികെ pair.first->second;
    }

    സ്ഥിരം std::string& internWideWidth(പൂർണ്ണസംഖ്യ മൂല്യം) {
        സ്ഥിരം auto pair = m_internedWideWidths.emplace(മൂല്യം, "");
        എങ്കിൽ (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        തിരികെ pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    സ്ഥിതം തർക്കം isLeaf(സ്ഥിരം Ast_കണ്ണി* nodep) {
        സ്ഥിരം VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        വേണ്ടി (സ്ഥിരം VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            എങ്കിൽ (opType != VNTypeInfo::OP_UNUSED) തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }

    // Render operand, return true if not unused
    ശൂന്യം renderOperand(VNTypeInfo::OpEn opType, സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം opp, uint32_t depth) {
        തിരഞ്ഞെടുപ്പ് (opType) {
        അവസ്ഥ VNTypeInfo::OP_UNUSED:  //
            നിർത്തുക;
        അവസ്ഥ VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            നിർത്തുക;
        അവസ്ഥ VNTypeInfo::OP_LIST:
            m_os << " [";
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* nodep = opp; nodep; nodep = nodep->nextp()) {
                എങ്കിൽ (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            നിർത്തുക;
        അവസ്ഥ VNTypeInfo::OP_OPTIONAL:
            എങ്കിൽ (opp) {
                m_os << " ";
                render(opp, depth);
            } അല്ലെങ്കിൽ {
                m_os << " nil";
            }
            നിർത്തുക;
        }
    }

    // Render the node into the stream.
    ശൂന്യം render(സ്ഥിരം Ast_കണ്ണി* nodep, uint32_t depth) {
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            എങ്കിൽ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } അല്ലെങ്കിൽ എങ്കിൽ (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } അല്ലെങ്കിൽ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } അല്ലെങ്കിൽ എങ്കിൽ (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } അല്ലെങ്കിൽ {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            സ്ഥിരം VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        എങ്കിൽ (!dtypep) {
            m_os << '?';
        } അല്ലെങ്കിൽ എങ്കിൽ (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } അല്ലെങ്കിൽ {
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->width();
            എങ്കിൽ (width == 1) {
                m_os << '1';
            } അല്ലെങ്കിൽ എങ്കിൽ (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } അല്ലെങ്കിൽ {
                m_os << internWideWidth(width);
            }
            എങ്കിൽ (m_dumpWidthMin) {
                m_os << '/';
                സ്ഥിരം പൂർണ്ണസംഖ്യ widthMin = nodep->widthMin();
                എങ്കിൽ (widthMin == 1) {
                    m_os << '1';
                } അല്ലെങ്കിൽ എങ്കിൽ (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } അല്ലെങ്കിൽ {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

പൊതു:
    VNPatternString(std::ostream& os, സ്ഥിരം Ast_കണ്ണി* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_കണ്ണി_വ്യഞ്ജകം::patternString(uint32_t depth) സ്ഥിരം {
    std::ostringstream oss;
    VNPatternString{oss, ഇത്, depth};
    തിരികെ oss.str();
}

//######################################################################
// VNDeleter

ശൂന്യം VNDeleter::doDeletes() {
    വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം nodep : m_deleteps) nodep->delete_വൃക്ഷം();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#അന്തര്ഭാവ "V3Ast__gen_visitor_defns.h"  // From ./astgen
