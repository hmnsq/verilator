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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3Broken.h"
#அந்தர்பாவ "V3File.h"

#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_முனை::s_editCntLast = 0;
uint64_t Ast_முனை::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
முழு Ast_முனை::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

தருக்க VNUser1InUse::s_userBusy = பொய்;
தருக்க VNUser2InUse::s_userBusy = பொய்;
தருக்க VNUser3InUse::s_userBusy = பொய்;
தருக்க VNUser4InUse::s_userBusy = பொய்;

முழு Ast_முனை_DType::s_unique_இலக்கம் = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(மாறிலி string& name) {
    க்கு (முழு i = 0; i < _ENUM_MAX; ++i) {
        மாறிலி VCMethod method{i};
        எனில் (name == method.ascii()) திரும்பு method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    திரும்பு VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(மாறிலி வெறுமை*)) மாறிலி {
#ifdef VL_USER_TYPE_CHECKS
    எனில் (மாறிலி uint64_t* மாறிலி uip = std::get_if<uint64_t>(&m_u)) திரும்பு "#"s + cvtToStr(*uip);
    எனில் (வெறுமை* மாறிலி* மாறிலி upp = std::get_if<வெறுமை*>(&m_u)) திரும்பு fmtAddrp(*upp);
    திரும்பு "";
#இல்லையெனில்
    // Dumps void* representation
    எனில் (!m_u.up) திரும்பு "";
    திரும்பு fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

மாறிலி VNTypeInfo VNType::s_typeInfoTable[VNType::இலக்கம்_TYPES()] = {
#அந்தர்பாவ "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

தருக்க VFwdType::is_முனை_Compatible(மாறிலி Ast_முனை* nodep) மாறிலி {
    மாறிலி Ast_முனை* defp = nodep;
    எனில் (மாறிலி AstTypedef* மாறிலி adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    எனில் (மாறிலி Ast_முனை_DType* மாறிலி adefp = VN_CAST(defp, முனை_DType))
        defp = adefp->skipRefToNonRefp();
    தேர்வு (m_e) {
    நிலை VFwdType::NONE: திரும்பு உண்மை; நிறுத்து;
    நிலை VFwdType::ENUM: திரும்பு VN_IS(defp, EnumDType); நிறுத்து;
    நிலை VFwdType::STRUCT: திரும்பு VN_IS(defp, StructDType); நிறுத்து;
    நிலை VFwdType::UNION: திரும்பு VN_IS(defp, UnionDType); நிறுத்து;
    நிலை VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    நிலை VFwdType::CLASS: திரும்பு VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); நிறுத்து;
    நிலை VFwdType::GENERIC_INTERFACE: திரும்பு VN_IS(defp, IfaceRefDType); நிறுத்து;
    இயல்பு: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    திரும்பு பொய்;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

மாறிலி std::shared_ptr<மாறிலி string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
மாறிலி std::shared_ptr<மாறிலி string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(தருக்க useSelfForThis, மாறிலி string& text) {
    திரும்பு useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(தருக்க useSelfForThis, தருக்க protect) மாறிலி {
    திரும்பு VIdProtect::protect_சொற்கள்_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_முனை::Ast_முனை(VNType t, File_வரி* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = இது;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = பொய்;
    m_flags.doingWidth = பொய்;
    m_flags.protect = உண்மை;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_மொத்தம்_Inc();
}

Ast_முனை* Ast_முனை::abovep() மாறிலி {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), இது, "abovep() not allowed when in midlist");
    மாறிலி Ast_முனை* மாறிலி firstp = firstAbovep() ? இது : m_headtailp;
    திரும்பு firstp->backp();
}
Ast_முனை* Ast_முனை::aboveLoopp() மாறிலி {
    // Returns parent node. Avoid using this, may have performance issues.
    மாறிலி Ast_முனை* nodep = இது;
    // Backwards over peers (versus parents)
    போது (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    திரும்பு nodep->backp();
}

string Ast_முனை::encodeName(மாறிலி string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    க்கு (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        எனில் ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } இல்லையெனில் எனில் (pos[0] == '_') {
            out += pos[0];
            எனில் (pos + 1 == namein.end()) நிறுத்து;
            எனில் (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } இல்லையெனில் {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            மாறிலி unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    திரும்பு vname.hashedName();
}

string Ast_முனை::encode_எண்(int64_t இலக்கம்) {
    எனில் (இலக்கம் < 0) திரும்பு "__02D" + cvtToStr(-இலக்கம்);  // 2D=-
    திரும்பு cvtToStr(இலக்கம்);
}

string Ast_முனை::nameProtect() மாறிலி { திரும்பு VIdProtect::protectIf(name(), protect()); }
string Ast_முனை::origNameProtect() மாறிலி { திரும்பு VIdProtect::protectIf(origName(), protect()); }

string Ast_முனை::shortName() மாறிலி {
    string pretty = name();
    string::size_type pos;
    போது ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    திரும்பு pretty;
}

string Ast_முனை::dedotName(மாறிலி string& namein) {
    string pretty = namein;
    string::size_type pos;
    போது ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    எனில் (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    திரும்பு pretty;
}

string Ast_முனை::nameNoArray(மாறிலி string& namein) {
    திரும்பு namein.substr(0, namein.find("__BRA__"));
}

string Ast_முனை::vcdName(மாறிலி string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    போது ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    போது ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    திரும்பு prettyName(pretty);
}

string Ast_முனை::prettyName(மாறிலி string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    க்கு (மாறிலி எழுத்து* pos = namein.c_str(); *pos;) {
        எனில் (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            தொடர்;
        }
        எனில் (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            எனில் (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                தொடர்;
            }
            எனில் (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                தொடர்;
            }
            எனில் (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                தொடர்;
            }
            எனில் (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                தொடர்;
            }
            எனில் (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                தொடர்;
            }
            எனில் (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                தொடர்;
            }
            எனில் (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                எழுத்து மதிப்பு = 0;
                மதிப்பு += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                மதிப்பு
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += மதிப்பு;
                pos += 5;
                தொடர்;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    எனில் (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    எனில் (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    திரும்பு pretty;
}

string Ast_முனை::vpiName(மாறிலி string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    மாறிலி size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    தருக்க inEscapedIdent = பொய்;
    முழு lastIdent = 0;

    க்கு (மாறிலி எழுத்து* pos = namein.c_str() + offset; *pos;) {
        எழுத்து specialChar = 0;
        எனில் (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } இல்லையெனில் எனில் (pos[0] == '_' && pos[1] == '_') {  // __
            எனில் (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } இல்லையெனில் எனில் (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } இல்லையெனில் எனில் (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } இல்லையெனில் எனில் (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                தொடர்;
            } இல்லையெனில் எனில் (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                எழுத்து மதிப்பு = 0;
                மதிப்பு += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                மதிப்பு
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                எனில் (மதிப்பு != '_') inEscapedIdent = உண்மை;

                pretty += மதிப்பு;
                pos += 5;
                தொடர்;
            }
        } இல்லையெனில் எனில் (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        எனில் (specialChar) {
            எனில் (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.செருகு(lastIdent, "\\");
                inEscapedIdent = பொய்;
            }

            pretty += specialChar;

            எனில் (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = பொய்;
            }
        } இல்லையெனில் {
            pretty += pos[0];
            ++pos;
        }
    }
    எனில் (inEscapedIdent) {
        pretty += " ";
        pretty.செருகு(lastIdent, "\\");
    }
    எனில் (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    எனில் (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    திரும்பு pretty;
}

string Ast_முனை::prettyTypeName() மாறிலி {
    எனில் (name() == "") திரும்பு typeName();
    திரும்பு std::string{typeName()} + " '" + pretty_Name_செய்தி() + "'";
}

//######################################################################
// Insertion

வெறுமை Ast_முனை::debug_மரம்_Change(மாறிலி Ast_முனை* nodep, மாறிலி எழுத்து* prefix, முழு lineno, தருக்க next) {
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
Ast_முனை* Ast_முனை::addNext<Ast_முனை, Ast_முனை>(Ast_முனை* nodep, Ast_முனை* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_மரம்_Change(nodep, "-addNextThs: ", __LINE__, பொய்);
    debug_மரம்_Change(newp, "-addNextNew: ", __LINE__, உண்மை);
    எனில் (!nodep)  // verilog.y and lots of other places assume this
        திரும்பு newp;

    // Find end of old list
    Ast_முனை* oldtailp = nodep;
    எனில் (oldtailp->m_nextp) {
        எனில் (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } இல்லையெனில் {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            போது (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_முனை* மாறிலி newtailp = newp->m_headtailp;
    Ast_முனை* மாறிலி headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_மொத்தம்_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_மரம்_Change(nodep, "-addNextOut:", __LINE__, உண்மை);
    திரும்பு nodep;
}

வெறுமை Ast_முனை::addNextHere(Ast_முனை* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_மரம்_Change(இது, "-addHereThs: ", __LINE__, பொய்);
    debug_மரம்_Change(newp, "-addHereNew: ", __LINE__, உண்மை);
    newp->edit_மொத்தம்_Inc();

    Ast_முனை* மாறிலி addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_முனை* மாறிலி oldnextp = இது->m_nextp;
    இது->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    எனில் (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = இது;

    // Head/tail
    Ast_முனை* மாறிலி oldheadtailp = இது->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    எனில் (oldheadtailp) {  // else in middle of list, no change
        எனில் (oldheadtailp == இது) {  // this was one node
            இது->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } இல்லையெனில் எனில் (!oldnextp) {  // this was tail
            இது->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_மரம்_Change(இது, "-addHereOut: ", __LINE__, உண்மை);
}

வெறுமை Ast_முனை::setOp1p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, இது, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_மரம்_Change(இது, "-setOp1pThs: ", __LINE__, பொய்);
    debug_மரம்_Change(newp, "-setOp1pNew: ", __LINE__, உண்மை);
    m_op1p = newp;
    newp->edit_மொத்தம்_Inc();
    newp->m_backp = இது;
    debug_மரம்_Change(இது, "-setOp1pOut: ", __LINE__, பொய்);
}

வெறுமை Ast_முனை::setOp2p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, இது, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_மரம்_Change(இது, "-setOp2pThs: ", __LINE__, பொய்);
    debug_மரம்_Change(newp, "-setOp2pNew: ", __LINE__, உண்மை);
    m_op2p = newp;
    newp->edit_மொத்தம்_Inc();
    newp->m_backp = இது;
    debug_மரம்_Change(இது, "-setOp2pOut: ", __LINE__, பொய்);
}

வெறுமை Ast_முனை::setOp3p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, இது, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_மரம்_Change(இது, "-setOp3pThs: ", __LINE__, பொய்);
    debug_மரம்_Change(newp, "-setOp3pNew: ", __LINE__, உண்மை);
    m_op3p = newp;
    newp->edit_மொத்தம்_Inc();
    newp->m_backp = இது;
    debug_மரம்_Change(இது, "-setOp3pOut: ", __LINE__, பொய்);
}

வெறுமை Ast_முனை::setOp4p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, இது, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_மரம்_Change(இது, "-setOp4pThs: ", __LINE__, பொய்);
    debug_மரம்_Change(newp, "-setOp4pNew: ", __LINE__, உண்மை);
    m_op4p = newp;
    newp->edit_மொத்தம்_Inc();
    newp->m_backp = இது;
    debug_மரம்_Change(இது, "-setOp4pOut: ", __LINE__, பொய்);
}

வெறுமை Ast_முனை::addOp1p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    எனில் (!m_op1p) {
        op1p(newp);
    } இல்லையெனில் {
        m_op1p->addNext(newp);
    }
}

வெறுமை Ast_முனை::addOp2p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    எனில் (!m_op2p) {
        op2p(newp);
    } இல்லையெனில் {
        m_op2p->addNext(newp);
    }
}

வெறுமை Ast_முனை::addOp3p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    எனில் (!m_op3p) {
        op3p(newp);
    } இல்லையெனில் {
        m_op3p->addNext(newp);
    }
}

வெறுமை Ast_முனை::addOp4p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    எனில் (!m_op4p) {
        op4p(newp);
    } இல்லையெனில் {
        m_op4p->addNext(newp);
    }
}

வெறுமை Ast_முனை::prependOp1p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    எனில் (!m_op1p) {
        op1p(newp);
    } இல்லையெனில் {
        m_op1p->addHereThisAsNext(newp);
    }
}

வெறுமை Ast_முனை::prependOp2p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    எனில் (!m_op2p) {
        op2p(newp);
    } இல்லையெனில் {
        m_op2p->addHereThisAsNext(newp);
    }
}

வெறுமை Ast_முனை::prependOp3p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    எனில் (!m_op3p) {
        op3p(newp);
    } இல்லையெனில் {
        m_op3p->addHereThisAsNext(newp);
    }
}

வெறுமை Ast_முனை::prependOp4p(Ast_முனை* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    எனில் (!m_op4p) {
        op4p(newp);
    } இல்லையெனில் {
        m_op4p->addHereThisAsNext(newp);
    }
}

வெறுமை Ast_முனை::replaceWith(Ast_முனை* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    இது->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
வெறுமை Ast_முனை::replaceWithKeepDType(Ast_முனை* newp) {
    newp->dtypeFrom(இது);
    replaceWith(newp);
}

வெறுமை VNRelinker::dump(std::ostream& str) மாறிலி {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_முனை* Ast_முனை::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_மரம்_Change(இது, "-unlinkWNextThs: ", __LINE__, உண்மை);
    Ast_முனை* மாறிலி oldp = இது;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_மொத்தம்_Inc();
    // cppcheck-suppress shadowFunction
    Ast_முனை* மாறிலி backp = oldp->m_backp;
    எனில் (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        எனில் (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } இல்லையெனில் எனில் (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } இல்லையெனில் எனில் (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } இல்லையெனில் எனில் (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } இல்லையெனில் எனில் (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } இல்லையெனில் {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    எனில் (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_முனை* oldtailp = backp->m_headtailp;
        எனில் (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            போது (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_முனை* மாறிலி oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } இல்லையெனில் எனில் (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } இல்லையெனில் எனில் (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } இல்லையெனில் எனில் (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } இல்லையெனில் எனில் (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } இல்லையெனில் {
        இது->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    எனில் (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_மரம்_Change(oldp, "-unlinkWNextOut: ", __LINE__, உண்மை);
    திரும்பு oldp;
}

Ast_முனை* Ast_முனை::unlinkFrBack(VNRelinker* linkerp) {
    debug_மரம்_Change(இது, "-unlinkFrBkThs: ", __LINE__, உண்மை);
    Ast_முனை* மாறிலி oldp = இது;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_மொத்தம்_Inc();
    // cppcheck-suppress shadowFunction
    Ast_முனை* மாறிலி backp = oldp->m_backp;
    எனில் (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        எனில் (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        எனில் (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } இல்லையெனில் எனில் (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } இல்லையெனில் எனில் (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } இல்லையெனில் எனில் (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } இல்லையெனில் எனில் (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } இல்லையெனில் {
            இது->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    எனில் (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        எனில் (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        எனில் (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } இல்லையெனில் {
        எனில் (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } இல்லையெனில் எனில் (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } இல்லையெனில் எனில் (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } இல்லையெனில் எனில் (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } இல்லையெனில் {
            இது->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        எனில் (oldp->m_nextp) {
            Ast_முனை* மாறிலி newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    எனில் (oldp->m_iterpp) {  // Only if no linker, point to next in list
        எனில் (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_மரம்_Change(oldp, "-unlinkFrBkOut: ", __LINE__, உண்மை);
    திரும்பு oldp;
}

வெறுமை Ast_முனை::relink(VNRelinker* linkerp) {
    எனில் (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_முனை* மாறிலி newp = இது;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_மொத்தம்_Inc();

    எனில் (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_முனை* மாறிலி backp = linkerp->m_backp;
    debug_மரம்_Change(இது, "-relinkNew: ", __LINE__, உண்மை);
    debug_மரம்_Change(backp, "-relinkTre: ", __LINE__, உண்மை);

    தேர்வு (linkerp->m_chg) {
    நிலை VNRelinker::RELINK_NEXT: backp->addNextHere(newp); நிறுத்து;
    நிலை VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); நிறுத்து;
    நிலை VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); நிறுத்து;
    நிலை VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); நிறுத்து;
    நிலை VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); நிறுத்து;
    இயல்பு: இது->v3fatalSrc("Relink of node without any link to change."); நிறுத்து;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    எனில் (linkerp->m_iterpp) {
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
    debug_மரம்_Change(இது, "-relinkOut: ", __LINE__, உண்மை);
}

வெறுமை Ast_முனை::relinkOneLink(Ast_முனை*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_முனை* newp) {
    எனில் (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_முனை* மாறிலி newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_முனை* மாறிலி oldlistlastp = pointpr->m_headtailp;
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

வெறுமை Ast_முனை::addHereThisAsNext(Ast_முனை* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(இது->m_backp, இது, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_முனை* மாறிலி backp = இது->m_backp;
    Ast_முனை* மாறிலி newLastp = newp->m_headtailp;
    //
    இது->edit_மொத்தம்_Inc();
    // Common linkage
    newLastp->m_nextp = இது;
    இது->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    எனில் (backp && backp->m_nextp == இது) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } இல்லையெனில் {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        எனில் (backp) {
            எனில் (backp->m_op1p == இது) {
                backp->m_op1p = newp;
            } இல்லையெனில் எனில் (backp->m_op2p == இது) {
                backp->m_op2p = newp;
            } இல்லையெனில் எனில் (backp->m_op3p == இது) {
                backp->m_op3p = newp;
            } இல்லையெனில் {
                UASSERT_OBJ(backp->m_op4p == இது, இது, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_முனை* மாறிலி tailp = இது->m_headtailp;
        இது->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    எனில் (newLastp->m_iterpp) *(newLastp->m_iterpp) = இது;
    எனில் (இது->m_iterpp) {
        *(இது->m_iterpp) = newp;
        இது->m_iterpp = nullptr;
    }
    //
    debug_மரம்_Change(இது, "-addHereThisAsNext: ", __LINE__, உண்மை);
}

//======================================================================
// Clone

Ast_முனை* Ast_முனை::clone_மரம்_Iter(தருக்க needPure) {
    // private: Clone single node and children
    எனில் (needPure) purityCheck();
    Ast_முனை* மாறிலி newp = இது->clone();
    எனில் (இது->m_op1p) newp->op1p(இது->m_op1p->clone_மரம்_Iter_List(needPure));
    எனில் (இது->m_op2p) newp->op2p(இது->m_op2p->clone_மரம்_Iter_List(needPure));
    எனில் (இது->m_op3p) newp->op3p(இது->m_op3p->clone_மரம்_Iter_List(needPure));
    எனில் (இது->m_op4p) newp->op4p(இது->m_op4p->clone_மரம்_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(இது);  // Save pointers to/from both to simplify relinking.
    இது->clonep(newp);  // Save pointers to/from both to simplify relinking.
    திரும்பு newp;
}

Ast_முனை* Ast_முனை::clone_மரம்_Iter_List(தருக்க needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_முனை* newheadp = nullptr;
    Ast_முனை* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    க்கு (Ast_முனை* oldp = இது; oldp; oldp = oldp->m_nextp) {
        Ast_முனை* மாறிலி newp = oldp->clone_மரம்_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        எனில் (newtailp) newtailp->m_nextp = newp;
        எனில் (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    திரும்பு newheadp;
}

Ast_முனை* Ast_முனை::clone_மரம்(தருக்க cloneNextLink, தருக்க needPure) {
    debug_மரம்_Change(இது, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_மரம்();
    Ast_முனை* newp;
    எனில் (cloneNextLink && இது->m_nextp) {
        newp = clone_மரம்_Iter_List(needPure);
    } இல்லையெனில் {
        newp = clone_மரம்_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_மரம்();
    debug_மரம்_Change(newp, "-cloneOut: ", __LINE__, உண்மை);
    திரும்பு newp;
}

வெறுமை Ast_முனை::purityCheck() {
    எனில் (VL_UNLIKELY(!isPure())) {
        இது->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << இது->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

வெறுமை Ast_முனை::delete_முனை() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_மொத்தம்_Inc();
    // Change links of old node so we coredump if used
    இது->m_nextp = reinterpret_cast<Ast_முனை*>(0x1);
    இது->m_backp = reinterpret_cast<Ast_முனை*>(0x1);
    இது->m_headtailp = reinterpret_cast<Ast_முனை*>(0x1);
    இது->m_op1p = reinterpret_cast<Ast_முனை*>(0x1);
    இது->m_op2p = reinterpret_cast<Ast_முனை*>(0x1);
    இது->m_op3p = reinterpret_cast<Ast_முனை*>(0x1);
    இது->m_op4p = reinterpret_cast<Ast_முனை*>(0x1);
    இது->m_iterpp = reinterpret_cast<Ast_முனை**>(0x1);
    எனில் (
#எனில் !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        உண்மை
#இல்லையெனில்
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete இது;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

வெறுமை Ast_முனை::delete_மரம்_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    க்கு (Ast_முனை *nodep = இது, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        எனில் (nodep->m_op1p) nodep->m_op1p->delete_மரம்_Iter();
        எனில் (nodep->m_op2p) nodep->m_op2p->delete_மரம்_Iter();
        எனில் (nodep->m_op3p) nodep->m_op3p->delete_மரம்_Iter();
        எனில் (nodep->m_op4p) nodep->m_op4p->delete_மரம்_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_முனை();
    }
}

வெறுமை Ast_முனை::delete_மரம்() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_மரம்_Change(இது, "-delTree:  ", __LINE__, உண்மை);
    இது->edit_மொத்தம்_Inc();
    // MUST be depth first!
    delete_மரம்_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
வெறுமை* Ast_முனை::operator புதிய(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_முனை* மாறிலி objp = static_cast<Ast_முனை*>(::operator புதிய(size));
    V3Broken::addNewed(objp);
    திரும்பு objp;
}

வெறுமை Ast_முனை::operator delete(வெறுமை* objp, size_t size) {
    எனில் (!objp) திரும்பு;
    மாறிலி Ast_முனை* மாறிலி nodep = static_cast<Ast_முனை*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
வெறுமை* Ast_முனை::operator புதிய(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    நிலையான மாறிலி size_t ASTGEN_MAX_முனை_SIZE = []() {
        size_t maxSize = 0;
        க்கு (size_t t = 0; t < VNType::இலக்கம்_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        திரும்பு maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    நிலையான uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    எனில் (முழு seed = v3Global.opt.debugAllocRandom()) {
        நிலையான V3Mutex s_mutex;
        மாறிலி V3LockGuard lock{s_mutex};
        நிலையான std::array<வெறுமை*, POOL_SIZE> s_முனை_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_முனை_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        எனில் (!s_lfsr) {
            s_lfsr = seed;
            எனில் (!s_lfsr) s_lfsr = ~s_lfsr;
            க்கு (size_t i = 0; i < POOL_SIZE; ++i) {
                s_முனை_Pool[i] = ::operator புதிய(ASTGEN_MAX_முனை_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_முனை_Pool), std::end(s_முனை_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        மாறிலி size_t index = s_lfsr % POOL_SIZE;
        Ast_முனை* மாறிலி objp = static_cast<Ast_முனை*>(s_முனை_Pool[index]);
        s_முனை_Pool[index] = ::operator புதிய(ASTGEN_MAX_முனை_SIZE + 64);  // For later new()
        திரும்பு objp;
    } இல்லையெனில் {
        Ast_முனை* மாறிலி objp = static_cast<Ast_முனை*>(::operator புதிய(size));
        திரும்பு objp;
    }
}
வெறுமை Ast_முனை::operator delete(வெறுமை* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (வெறுமை)objp;
    (வெறுமை)size;
}
#endif

//======================================================================
// Iterators

வெறுமை Ast_முனை::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    எனில் (m_op1p) m_op1p->iterateAndNext(v);
    எனில் (m_op2p) m_op2p->iterateAndNext(v);
    எனில் (m_op3p) m_op3p->iterateAndNext(v);
    எனில் (m_op4p) m_op4p->iterateAndNext(v);
}

வெறுமை Ast_முனை::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    எனில் (m_op1p) m_op1p->iterateAndNextConst(v);
    எனில் (m_op2p) m_op2p->iterateAndNextConst(v);
    எனில் (m_op3p) m_op3p->iterateAndNextConst(v);
    எனில் (m_op4p) m_op4p->iterateAndNextConst(v);
}

வெறுமை Ast_முனை::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_முனை* nodep = இது;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    எனில் (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    போது (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        எனில் (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_முனை* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        எனில் (!niterp) திரும்பு;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        எனில் (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } இல்லையெனில் {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

வெறுமை Ast_முனை::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_முனை* nodep = இது;
    போது (nodep->m_nextp) nodep = nodep->m_nextp;
    போது (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        எனில் (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } இல்லையெனில் {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

வெறுமை Ast_முனை::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    எனில் (m_op1p) m_op1p->iterateListBackwardsConst(v);
    எனில் (m_op2p) m_op2p->iterateListBackwardsConst(v);
    எனில் (m_op3p) m_op3p->iterateListBackwardsConst(v);
    எனில் (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

வெறுமை Ast_முனை::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_முனை* nodep = இது;
    செய் {
        Ast_முனை* மாறிலி nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } போது (nodep);
}

Ast_முனை* Ast_முனை::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_முனை* nodep = இது;  // Note "this" may point to bogus point later in this function
    எனில் (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } இல்லையெனில் எனில் (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* மாறிலி tempp = புதிய AstBegin{nodep->fileline(), "[EditWrapper]", nodep, பொய்};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_மரம்(), tempp);
    } இல்லையெனில் {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_முனை** nextnodepp = nullptr;
        எனில் (இது->m_backp->m_op1p == இது) {
            nextnodepp = &(இது->m_backp->m_op1p);
        } இல்லையெனில் எனில் (இது->m_backp->m_op2p == இது) {
            nextnodepp = &(இது->m_backp->m_op2p);
        } இல்லையெனில் எனில் (இது->m_backp->m_op3p == இது) {
            nextnodepp = &(இது->m_backp->m_op3p);
        } இல்லையெனில் எனில் (இது->m_backp->m_op4p == இது) {
            nextnodepp = &(இது->m_backp->m_op4p);
        } இல்லையெனில் எனில் (இது->m_backp->m_nextp == இது) {
            nextnodepp = &(இது->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, இது, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    திரும்பு nodep;
}

//======================================================================

வெறுமை Ast_முனை::clone_Relink_மரம்() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    க்கு (Ast_முனை* nodep = இது; nodep; nodep = nodep->m_nextp) {
        எனில் (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        எனில் (nodep->m_op1p) nodep->m_op1p->clone_Relink_மரம்();
        எனில் (nodep->m_op2p) nodep->m_op2p->clone_Relink_மரம்();
        எனில் (nodep->m_op3p) nodep->m_op3p->clone_Relink_மரம்();
        எனில் (nodep->m_op4p) nodep->m_op4p->clone_Relink_மரம்();
    }
}

//======================================================================
// Comparison

தருக்க Ast_முனை::gate_மரம்_Iter() மாறிலி {
    // private: Return true if the two trees are identical
    எனில் (!isGateOptimizable()) திரும்பு பொய்;
    எனில் (m_op1p && !m_op1p->gate_மரம்_Iter()) திரும்பு பொய்;
    எனில் (m_op2p && !m_op2p->gate_மரம்_Iter()) திரும்பு பொய்;
    எனில் (m_op3p && !m_op3p->gate_மரம்_Iter()) திரும்பு பொய்;
    எனில் (m_op4p && !m_op4p->gate_மரம்_Iter()) திரும்பு பொய்;
    திரும்பு உண்மை;
}

தருக்க Ast_முனை::same_மரம்_Iter(மாறிலி Ast_முனை* node1p, மாறிலி Ast_முனை* node2p, தருக்க ignNext,
                           தருக்க gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    போது (உண்மை) {
        எனில் (!node1p && !node2p) திரும்பு உண்மை;
        எனில் (!node1p || !node2p) திரும்பு பொய்;
        எனில் (node1p->type() != node2p->type()) திரும்பு பொய்;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        எனில் (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) திரும்பு பொய்;
        எனில் (!node1p->same_முனை(node2p) || (gateOnly && !node1p->isGateOptimizable())) திரும்பு பொய்;
        எனில் (!same_மரம்_Iter(node1p->m_op1p, node2p->m_op1p, பொய், gateOnly)
            || !same_மரம்_Iter(node1p->m_op2p, node2p->m_op2p, பொய், gateOnly)
            || !same_மரம்_Iter(node1p->m_op3p, node2p->m_op3p, பொய், gateOnly)
            || !same_மரம்_Iter(node1p->m_op4p, node2p->m_op4p, பொய், gateOnly)) {
            திரும்பு பொய்;
        }
        எனில் (ignNext) திரும்பு உண்மை;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

வெறுமை Ast_முனை::check_மரம்_Iter(மாறிலி Ast_முனை* prevBackp) மாறிலி VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == இது->backp(), இது, "Back node inconsistent");
    மாறிலி VNTypeInfo& typeInfo = VNType::typeInfo(இது->type());
    க்கு (முழு i = 1; i <= 4; i++) {
        Ast_முனை* nodep = nullptr;
        தேர்வு (i) {
        நிலை 1: nodep = op1p(); நிறுத்து;
        நிலை 2: nodep = op2p(); நிறுத்து;
        நிலை 3: nodep = op3p(); நிறுத்து;
        நிலை 4: nodep = op4p(); நிறுத்து;
        இயல்பு: இது->v3fatalSrc("Bad case"); நிறுத்து;
        }
        // cppcheck-suppress danglingTempReference
        மாறிலி எழுத்து* opName = typeInfo.m_opNamep[i - 1];
        தேர்வு (typeInfo.m_opType[i - 1]) {
        நிலை VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, இது, typeInfo.m_namep << " must not use " << opName << "()");
            நிறுத்து;
        நிலை VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, இது,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), இது,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_மரம்_Iter(இது);
            நிறுத்து;
        நிலை VNTypeInfo::OP_LIST:
            எனில் (மாறிலி Ast_முனை* மாறிலி headp = nodep) {
                // cppcheck-suppress shadowFunction
                மாறிலி Ast_முனை* backp = இது;
                மாறிலி Ast_முனை* tailp;
                மாறிலி Ast_முனை* opp = headp;
                செய் {
                    opp->check_மரம்_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } போது (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            நிறுத்து;
        நிலை VNTypeInfo::OP_OPTIONAL:
            எனில் (nodep) {
                UASSERT_OBJ(!nodep->nextp(), இது,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_மரம்_Iter(இது);
            }
            நிறுத்து;
        இயல்பு: இது->v3fatalSrc("Bad case"); நிறுத்து;
        }
    }
    எனில் (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        எனில் (மாறிலி Ast_முனை_கோவை* மாறிலி exprp = VN_CAST(இது, முனை_கோவை)) {
            மாறிலி எழுத்து* மாறிலி whyp = exprp->widthMismatch();
            எனில் (whyp) {
                auto dtypeStr = [](மாறிலி Ast_முனை_கோவை* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    திரும்பு ss.str();
                };
                எனில் (மாறிலி Ast_முனை_Uniop* மாறிலி uniopp = VN_CAST(exprp, முனை_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } இல்லையெனில் எனில் (மாறிலி Ast_முனை_Biop* மாறிலி biopp = VN_CAST(exprp, முனை_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } இல்லையெனில் {
                    UASSERT_OBJ(பொய், exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
எழுத்து* Ast_முனை::dump_மரம்_Json_Gdb(மாறிலி Ast_முனை* nodep) {
    எனில் (!nodep) திரும்பு strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_மரம்_Json(nodepStream);
    மாறிலி std::string str = nodepStream.rdbuf()->str();
    திரும்பு strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
எழுத்து* Ast_முனை::dump_மரம்_Json_Gdb(மாறிலி எழுத்து* str) { திரும்பு strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
எழுத்து* Ast_முனை::dump_மரம்_Json_Gdb(intptr_t nodep) {
    எனில் (!nodep) திரும்பு strdup("{\"addr\":\"NULL\"}\n");
    திரும்பு dump_மரம்_Json_Gdb(reinterpret_cast<மாறிலி Ast_முனை*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
வெறுமை Ast_முனை::dumpGdb(மாறிலி Ast_முனை* nodep) {  // For GDB only  // LCOV_EXCL_START
    எனில் (!nodep) {
        cout << "<nullptr>" << endl;
        திரும்பு;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
வெறுமை Ast_முனை::dumpGdbHeader() மாறிலி {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
வெறுமை Ast_முனை::dump_மரம்_Gdb(மாறிலி Ast_முனை* nodep) {  // For GDB only  // LCOV_EXCL_START
    எனில் (!nodep) {
        cout << "<nullptr>" << endl;
        திரும்பு;
    }
    nodep->dumpGdbHeader();
    nodep->dump_மரம்(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
வெறுமை Ast_முனை::dump_மரம்_File_Gdb(மாறிலி Ast_முனை* nodep,  // LCOV_EXCL_START
                              மாறிலி எழுத்து* filenamep) {  // For GDB only
    எனில் (!nodep) {
        cout << "<nullptr>" << endl;
        திரும்பு;
    }
    மாறிலி string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_மரம்_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
வெறுமை Ast_முனை::checkIter() மாறிலி {
    எனில் (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        இது->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

வெறுமை Ast_முனை::dumpPtrs(std::ostream& os) மாறிலி {
    os << "This=" << typeName() << " " << cvtToHex(இது);
    os << " back=" << cvtToHex(backp());
    எனில் (nextp()) os << " next=" << cvtToHex(nextp());
    எனில் (m_headtailp == இது) {
        os << " headtail=this";
    } இல்லையெனில் {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    எனில் (op1p()) os << " op1p=" << cvtToHex(op1p());
    எனில் (op2p()) os << " op2p=" << cvtToHex(op2p());
    எனில் (op3p()) os << " op3p=" << cvtToHex(op3p());
    எனில் (op4p()) os << " op4p=" << cvtToHex(op4p());
    மாறிலி auto dumpUser = [&os](மாறிலி எழுத்து* prefix, மாறிலி VNUser& user) {
        மாறிலி std::string s
            = user.dumpStr([](மாறிலி வெறுமை* p) -> std::string { திரும்பு cvtToHex(p); });
        எனில் (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    எனில் (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

வெறுமை Ast_முனை::dump_மரம்(std::ostream& os, மாறிலி string& indent, முழு maxDepth) மாறிலி {
    நிலையான முழு s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << இது << '\n';
    எனில் (VN_DELETED(இது)) திரும்பு;
    எனில் (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    எனில் (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(இது)
           << endl;  // endl intentional to do flush
        திரும்பு;
    }  // LCOV_EXCL_STOP
    எனில் (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    எனில் (maxDepth == 1) {
        எனில் (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } இல்லையெனில் {
        க்கு (மாறிலி Ast_முனை* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_மரம்(os, indent + "1:", maxDepth - 1);
        }
        க்கு (மாறிலி Ast_முனை* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_மரம்(os, indent + "2:", maxDepth - 1);
        }
        க்கு (மாறிலி Ast_முனை* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_மரம்(os, indent + "3:", maxDepth - 1);
        }
        க்கு (மாறிலி Ast_முனை* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_மரம்(os, indent + "4:", maxDepth - 1);
        }
    }
}

வெறுமை Ast_முனை::dump_மரம்_And_Next(std::ostream& os, மாறிலி string& indent, முழு maxDepth) மாறிலி {
    // Audited to make sure this is never nullptr
    க்கு (மாறிலி Ast_முனை* nodep = இது; nodep; nodep = nodep->nextp()) {
        nodep->dump_மரம்(os, indent, maxDepth);
    }
}

வெறுமை Ast_முனை::dump_மரம்_File(மாறிலி string& filename, தருக்க doDump) {
    // Not const function as calls checkTree
    எனில் (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            மாறிலி std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            எனில் (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_மொத்தம்_Last();
            *logsp << "> to <e" << std::dec << edit_மொத்தம்_Gbl() << ">\n";
            எனில் (edit_மொத்தம்_Gbl() == edit_மொத்தம்_Last() && ::dump_மரம்_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } இல்லையெனில் {
                dump_மரம்(*logsp);
                edit_மொத்தம்_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

நிலையான வெறுமை drawChildren(std::ostream& os, மாறிலி Ast_முனை* thisp, மாறிலி Ast_முனை* childp,
                         மாறிலி std::string& childName) {
    எனில் (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        க்கு (மாறிலி Ast_முனை* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_மரம்_Dot(os);
            எனில் (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

வெறுமை Ast_முனை::dump_மரம்_Dot(std::ostream& os) மாறிலி {
    os << "\tn" << cvtToHex(இது) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, இது, m_op1p, "op1");
    drawChildren(os, இது, m_op2p, "op2");
    drawChildren(os, இது, m_op3p, "op3");
    drawChildren(os, இது, m_op4p, "op4");
}

வெறுமை Ast_முனை::dump_மரம்_Json_File(மாறிலி string& filename, தருக்க doDump) {
    எனில் (!doDump) திரும்பு;
    UINFO(2, "Dumping " << filename);
    மாறிலி std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    எனில் (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_மரம்_Json(*treejsonp);
    *treejsonp << '\n';
}

வெறுமை Ast_முனை::dumpJsonMetaFileGdb(மாறிலி எழுத்து* filename) { dumpJsonMetaFile(filename); }
வெறுமை Ast_முனை::dumpJsonMetaFile(மாறிலி string& filename) {
    UINFO(2, "Dumping " << filename);
    மாறிலி std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    எனில் (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_வரி::file_Name_இலக்கம்_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

வெறுமை Ast_முனை::dump_மரம்_Dot_File(மாறிலி string& filename, தருக்க doDump) {
    எனில் (doDump) {
        UINFO(2, "Dumping " << filename);
        மாறிலி std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        எனில் (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_மரம்_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_முனை::instanceStr() மாறிலி {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_மொத்தம் = 0;
    // Walk 'backp' chain
    க்கு (மாறிலி Ast_முனை* currp = இது; currp; currp = currp->backp(), ++iter_மொத்தம்) {
        எனில் (VL_UNCOVERABLE(iter_மொத்தம் >= maxIterations)) திரும்பு "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        எனில் (மாறிலி AstScope* மாறிலி scopep = VN_CAST(currp, Scope)) {
            திரும்பு scopep->is_உச்சி() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        எனில் (மாறிலி AstModule* மாறிலி modp = VN_CAST(currp, Module)) {
            மாறிலி string instanceName = modp->someInstanceName();
            திரும்பு instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    திரும்பு "";
}
வெறுமை Ast_முனை::v3errorEnd(மாறிலி std::ostringstream& str) மாறிலி VL_RELEASE(V3Error::s().m_mutex) {
    எனில் (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } இல்லையெனில் {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        மாறிலி string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        எனில் (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_முனை*>(இது)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
வெறுமை Ast_முனை::v3errorEndFatal(மாறிலி std::ostringstream& str) மாறிலி
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ப்ரதிஜ்ஞா(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

வெறுமை Ast_முனை::dtypeChgSigned(தருக்க flag) {
    UASSERT_OBJ(dtypep(), இது, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
வெறுமை Ast_முனை::dtypeChgWidth(முழு width, முழு widthMin) {
    UASSERT_OBJ(dtypep(), இது, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

வெறுமை Ast_முனை::dtypeChgWidthSigned(முழு width, முழு widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), இது, "No dtype when changing width");
    மாறிலி Ast_முனை_DType* மாறிலி enumDTypep = dtypep()->skipRefToEnump();
    எனில் (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        திரும்பு;  // Correct already
    }
    மாறிலி Ast_முனை_DType* மாறிலி dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_முனை_DType* Ast_முனை::findBasicDType(VBasicDTypeKwd kwd) மாறிலி {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    திரும்பு v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_முனை_DType* Ast_முனை::findBitDType(முழு width, முழு widthMin, VSigning numeric) மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_முனை_DType* Ast_முனை::findLogicDType(முழு width, முழு widthMin, VSigning numeric) மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_முனை_DType* Ast_முனை::findLogic2StateDType(முழு width, முழு widthMin, VSigning numeric) மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_முனை_DType* Ast_முனை::findLogicRangeDType(மாறிலி VNumRange& range, முழு widthMin,
                                           VSigning numeric) மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_முனை_DType* Ast_முனை::findBitRangeDType(மாறிலி VNumRange& range, முழு widthMin,
                                         VSigning numeric) மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_முனை::find_செருகு_Same_DType(AstBasicDType* nodep) {
    திரும்பு v3Global.rootp()->typeTablep()->find_செருகு_Same_DType(nodep);
}
Ast_முனை_DType* Ast_முனை::findConstraintRefDType() மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_முனை_DType* Ast_முனை::findEmptyQueueDType() மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_முனை_DType* Ast_முனை::findQueueIndexDType() மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_முனை_DType* Ast_முனை::findStreamDType() மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_முனை_DType* Ast_முனை::findVoidDType() மாறிலி {
    திரும்பு v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

நிலையான மாறிலி Ast_முனை_DType* computeCastableBase(மாறிலி Ast_முனை_DType* nodep) {
    போது (உண்மை) {
        எனில் (மாறிலி AstPackArrayDType* மாறிலி packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            தொடர்;
        }
        எனில் (மாறிலி Ast_முனை_DType* மாறிலி refp = nodep->skipRefToEnump()) {
            எனில் (refp != nodep) {
                nodep = refp;
                தொடர்;
            }
        }
        திரும்பு nodep;
    }
}

நிலையான VCastable computeCastableImp(மாறிலி Ast_முனை_DType* toDtp, மாறிலி Ast_முனை_DType* fromDtp,
                                    மாறிலி Ast_முனை* fromConstp, மாறிலி தருக்க checkIfaceArgCompat) {
    மாறிலி VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    எனில் (toDtp == fromDtp) திரும்பு VCastable::SAMEISH;
    எனில் (toDtp->similarDType(fromDtp)) திரும்பு VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    மாறிலி Ast_முனை_DType* fromBaseDtp = computeCastableBase(fromDtp);

    மாறிலி தருக்க fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, முனை_UOr_Struct_DType);

    மாறிலி Ast_முனை_DType* toBaseDtp = computeCastableBase(toDtp);
    மாறிலி தருக்க toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, முனை_UOr_Struct_DType);

    எனில் (toBaseDtp == fromBaseDtp) திரும்பு VCastable::COMPATIBLE;

    எனில் (toNumericable) {
        எனில் (fromNumericable) திரும்பு VCastable::COMPATIBLE;
    } இல்லையெனில் எனில் (VN_IS(toBaseDtp, EnumDType)) {
        எனில் (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_மரம்(fromDtp))
            திரும்பு VCastable::ENUM_IMPLICIT;
        எனில் (fromNumericable) திரும்பு VCastable::ENUM_EXPLICIT;
    } இல்லையெனில் எனில் (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        திரும்பு VCastable::COMPATIBLE;
    } இல்லையெனில் எனில் (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        எனில் (fromConstp->isNull()) திரும்பு VCastable::COMPATIBLE;
    } இல்லையெனில் எனில் (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        மாறிலி AstClass* மாறிலி toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        மாறிலி AstClass* மாறிலி fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        மாறிலி தருக்க downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        மாறிலி தருக்க upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        எனில் (upcast) திரும்பு VCastable::COMPATIBLE;
        எனில் (downcast) திரும்பு VCastable::DYNAMIC_CLASS;
        திரும்பு VCastable::INCOMPATIBLE;
    } இல்லையெனில் எனில் (மாறிலி AstIfaceRefDType* மாறிலி toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(உண்மை) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        மாறிலி AstIfaceRefDType* மாறிலி fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(உண்மை) : fromDtp, IfaceRefDType);
        எனில் (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            மாறிலி AstUnpackArrayDType* மாறிலி toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            மாறிலி AstUnpackArrayDType* மாறிலி fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            எனில் (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                திரும்பு VCastable::INCOMPATIBLE;
            }
            திரும்பு computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        எனில் (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            எனில் (!checkIfaceArgCompat) திரும்பு castable;
            திரும்பு VCastable::INCOMPATIBLE;
        }
        மாறிலி தருக்க sameModport = toIfp->modportp() == fromIfp->modportp();
        எனில் (!checkIfaceArgCompat) {
            எனில் (!toIfp->modportp() || sameModport) திரும்பு VCastable::COMPATIBLE;
            திரும்பு castable;
        }
        எனில் (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            திரும்பு VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        எனில் (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            திரும்பு VCastable::COMPATIBLE;
        }
        திரும்பு VCastable::INCOMPATIBLE;
    } இல்லையெனில் எனில் (மாறிலி AstUnpackArrayDType* மாறிலி toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        மாறிலி AstUnpackArrayDType* மாறிலி fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        எனில் (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            திரும்பு castable;
        }
        மாறிலி VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        மாறிலி Ast_முனை_DType* மாறிலி toSubp = toArrayp->subDTypep()->skipRefp();
        எனில் (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            திரும்பு VCastable::COMPATIBLE;
        }
    }
    திரும்பு castable;
}

VCastable Ast_முனை::computeCastable(மாறிலி Ast_முனை_DType* toDtp, மாறிலி Ast_முனை_DType* fromDtp,
                                   மாறிலி Ast_முனை* fromConstp, மாறிலி தருக்க checkIfaceArgCompat) {
    மாறிலி auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    எனில் (fromConstp) UINFO(9, "     const= " << fromConstp);
    திரும்பு castable;
}

Ast_முனை_DType* Ast_முனை::getCommonClassTypep(Ast_முனை* node1p, Ast_முனை* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    எனில் (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        மாறிலி VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        எனில் (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            திரும்பு node1p->dtypep();
        எனில் (castable == VCastable::DYNAMIC_CLASS) திரும்பு node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    போது (classDtypep1) {
        மாறிலி VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        எனில் (castable == VCastable::COMPATIBLE) திரும்பு classDtypep1;
        AstClassExtends* மாறிலி extendsp = classDtypep1->classp()->extendsp();
        எனில் (!extendsp) நிறுத்து;
        Ast_முனை_DType* மாறிலி edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    திரும்பு nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

வகுப்பு VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<முழு, std::string> m_internedWordWidths;  // Interned widths
    std::map<முழு, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    மாறிலி தருக்க m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    நிலையான std::string toLetters(size_t மதிப்பு, தருக்க lowerCase = பொய்) {
        மாறிலி எழுத்து base = lowerCase ? 'a' : 'A';
        std::string s;
        செய் { s += static_cast<எழுத்து>(base + மதிப்பு % 26); } போது (மதிப்பு /= 26);
        திரும்பு s;
    }

    மாறிலி std::string& internConst(மாறிலி AstConst& nodep) {
        மாறிலி auto pair = m_internedConsts.emplace(nodep.இலக்கம்().ascii(பொய்), "");
        எனில் (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        திரும்பு pair.first->second;
    }

    மாறிலி std::string& internWordWidth(முழு மதிப்பு) {
        மாறிலி auto pair = m_internedWordWidths.emplace(மதிப்பு, "");
        எனில் (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, உண்மை);
        திரும்பு pair.first->second;
    }

    மாறிலி std::string& internWideWidth(முழு மதிப்பு) {
        மாறிலி auto pair = m_internedWideWidths.emplace(மதிப்பு, "");
        எனில் (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        திரும்பு pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    நிலையான தருக்க isLeaf(மாறிலி Ast_முனை* nodep) {
        மாறிலி VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        க்கு (மாறிலி VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            எனில் (opType != VNTypeInfo::OP_UNUSED) திரும்பு பொய்;
        }
        திரும்பு உண்மை;
    }

    // Render operand, return true if not unused
    வெறுமை renderOperand(VNTypeInfo::OpEn opType, மாறிலி Ast_முனை* மாறிலி opp, uint32_t depth) {
        தேர்வு (opType) {
        நிலை VNTypeInfo::OP_UNUSED:  //
            நிறுத்து;
        நிலை VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            நிறுத்து;
        நிலை VNTypeInfo::OP_LIST:
            m_os << " [";
            க்கு (மாறிலி Ast_முனை* nodep = opp; nodep; nodep = nodep->nextp()) {
                எனில் (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            நிறுத்து;
        நிலை VNTypeInfo::OP_OPTIONAL:
            எனில் (opp) {
                m_os << " ";
                render(opp, depth);
            } இல்லையெனில் {
                m_os << " nil";
            }
            நிறுத்து;
        }
    }

    // Render the node into the stream.
    வெறுமை render(மாறிலி Ast_முனை* nodep, uint32_t depth) {
        எனில் (மாறிலி AstConst* மாறிலி constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            எனில் (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } இல்லையெனில் எனில் (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } இல்லையெனில் {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } இல்லையெனில் எனில் (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } இல்லையெனில் எனில் (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } இல்லையெனில் {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            மாறிலி VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        மாறிலி Ast_முனை_DType* மாறிலி dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        எனில் (!dtypep) {
            m_os << '?';
        } இல்லையெனில் எனில் (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } இல்லையெனில் {
            மாறிலி முழு width = nodep->width();
            எனில் (width == 1) {
                m_os << '1';
            } இல்லையெனில் எனில் (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } இல்லையெனில் {
                m_os << internWideWidth(width);
            }
            எனில் (m_dumpWidthMin) {
                m_os << '/';
                மாறிலி முழு widthMin = nodep->widthMin();
                எனில் (widthMin == 1) {
                    m_os << '1';
                } இல்லையெனில் எனில் (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } இல்லையெனில் {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

பொது:
    VNPatternString(std::ostream& os, மாறிலி Ast_முனை* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_முனை_கோவை::patternString(uint32_t depth) மாறிலி {
    std::ostringstream oss;
    VNPatternString{oss, இது, depth};
    திரும்பு oss.str();
}

//######################################################################
// VNDeleter

வெறுமை VNDeleter::doDeletes() {
    க்கு (Ast_முனை* மாறிலி nodep : m_deleteps) nodep->delete_மரம்();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#அந்தர்பாவ "V3Ast__gen_visitor_defns.h"  // From ./astgen
