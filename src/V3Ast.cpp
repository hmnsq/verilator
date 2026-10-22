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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3Broken.h"
#અન્તર્ભાવ "V3File.h"

#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_ગાંઠ::s_editCntLast = 0;
uint64_t Ast_ગાંઠ::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
પૂર્ણાંક Ast_ગાંઠ::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

તાર્કિક VNUser1InUse::s_userBusy = ખોટું;
તાર્કિક VNUser2InUse::s_userBusy = ખોટું;
તાર્કિક VNUser3InUse::s_userBusy = ખોટું;
તાર્કિક VNUser4InUse::s_userBusy = ખોટું;

પૂર્ણાંક Ast_ગાંઠ_DType::s_unique_અંક = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(અચલ string& name) {
    માટે (પૂર્ણાંક i = 0; i < _ENUM_MAX; ++i) {
        અચલ VCMethod method{i};
        જો (name == method.ascii()) પાછા method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    પાછા VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(અચલ શૂન્ય*)) અચલ {
#ifdef VL_USER_TYPE_CHECKS
    જો (અચલ uint64_t* અચલ uip = std::get_if<uint64_t>(&m_u)) પાછા "#"s + cvtToStr(*uip);
    જો (શૂન્ય* અચલ* અચલ upp = std::get_if<શૂન્ય*>(&m_u)) પાછા fmtAddrp(*upp);
    પાછા "";
#નહીંતર
    // Dumps void* representation
    જો (!m_u.up) પાછા "";
    પાછા fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

અચલ VNTypeInfo VNType::s_typeInfoTable[VNType::અંક_TYPES()] = {
#અન્તર્ભાવ "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

તાર્કિક VFwdType::is_ગાંઠ_Compatible(અચલ Ast_ગાંઠ* nodep) અચલ {
    અચલ Ast_ગાંઠ* defp = nodep;
    જો (અચલ AstTypedef* અચલ adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    જો (અચલ Ast_ગાંઠ_DType* અચલ adefp = VN_CAST(defp, ગાંઠ_DType))
        defp = adefp->skipRefToNonRefp();
    પસંદગી (m_e) {
    સ્થિતિ VFwdType::NONE: પાછા સાચું; તોડો;
    સ્થિતિ VFwdType::ENUM: પાછા VN_IS(defp, EnumDType); તોડો;
    સ્થિતિ VFwdType::STRUCT: પાછા VN_IS(defp, StructDType); તોડો;
    સ્થિતિ VFwdType::UNION: પાછા VN_IS(defp, UnionDType); તોડો;
    સ્થિતિ VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    સ્થિતિ VFwdType::CLASS: પાછા VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); તોડો;
    સ્થિતિ VFwdType::GENERIC_INTERFACE: પાછા VN_IS(defp, IfaceRefDType); તોડો;
    મૂળભૂત: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    પાછા ખોટું;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

અચલ std::shared_ptr<અચલ string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
અચલ std::shared_ptr<અચલ string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(તાર્કિક useSelfForThis, અચલ string& text) {
    પાછા useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(તાર્કિક useSelfForThis, તાર્કિક protect) અચલ {
    પાછા VIdProtect::protect_શબ્દો_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_ગાંઠ::Ast_ગાંઠ(VNType t, File_લીટી* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = આ;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = ખોટું;
    m_flags.doingWidth = ખોટું;
    m_flags.protect = સાચું;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_ગણતરી_Inc();
}

Ast_ગાંઠ* Ast_ગાંઠ::abovep() અચલ {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), આ, "abovep() not allowed when in midlist");
    અચલ Ast_ગાંઠ* અચલ firstp = firstAbovep() ? આ : m_headtailp;
    પાછા firstp->backp();
}
Ast_ગાંઠ* Ast_ગાંઠ::aboveLoopp() અચલ {
    // Returns parent node. Avoid using this, may have performance issues.
    અચલ Ast_ગાંઠ* nodep = આ;
    // Backwards over peers (versus parents)
    જ્યારે (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    પાછા nodep->backp();
}

string Ast_ગાંઠ::encodeName(અચલ string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    માટે (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        જો ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } નહીંતર જો (pos[0] == '_') {
            out += pos[0];
            જો (pos + 1 == namein.end()) તોડો;
            જો (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } નહીંતર {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            અચલ unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    પાછા vname.hashedName();
}

string Ast_ગાંઠ::encode_સંખ્યા(int64_t અંક) {
    જો (અંક < 0) પાછા "__02D" + cvtToStr(-અંક);  // 2D=-
    પાછા cvtToStr(અંક);
}

string Ast_ગાંઠ::nameProtect() અચલ { પાછા VIdProtect::protectIf(name(), protect()); }
string Ast_ગાંઠ::origNameProtect() અચલ { પાછા VIdProtect::protectIf(origName(), protect()); }

string Ast_ગાંઠ::shortName() અચલ {
    string pretty = name();
    string::size_type pos;
    જ્યારે ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    પાછા pretty;
}

string Ast_ગાંઠ::dedotName(અચલ string& namein) {
    string pretty = namein;
    string::size_type pos;
    જ્યારે ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    જો (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    પાછા pretty;
}

string Ast_ગાંઠ::nameNoArray(અચલ string& namein) {
    પાછા namein.substr(0, namein.find("__BRA__"));
}

string Ast_ગાંઠ::vcdName(અચલ string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    જ્યારે ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    જ્યારે ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    પાછા prettyName(pretty);
}

string Ast_ગાંઠ::prettyName(અચલ string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    માટે (અચલ અક્ષર* pos = namein.c_str(); *pos;) {
        જો (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            ચાલુ;
        }
        જો (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            જો (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                ચાલુ;
            }
            જો (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                ચાલુ;
            }
            જો (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                ચાલુ;
            }
            જો (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                ચાલુ;
            }
            જો (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                ચાલુ;
            }
            જો (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                ચાલુ;
            }
            જો (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                અક્ષર કિંમત = 0;
                કિંમત += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                કિંમત
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += કિંમત;
                pos += 5;
                ચાલુ;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    જો (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    જો (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    પાછા pretty;
}

string Ast_ગાંઠ::vpiName(અચલ string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    અચલ size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    તાર્કિક inEscapedIdent = ખોટું;
    પૂર્ણાંક lastIdent = 0;

    માટે (અચલ અક્ષર* pos = namein.c_str() + offset; *pos;) {
        અક્ષર specialChar = 0;
        જો (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } નહીંતર જો (pos[0] == '_' && pos[1] == '_') {  // __
            જો (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } નહીંતર જો (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } નહીંતર જો (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } નહીંતર જો (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                ચાલુ;
            } નહીંતર જો (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                અક્ષર કિંમત = 0;
                કિંમત += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                કિંમત
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                જો (કિંમત != '_') inEscapedIdent = સાચું;

                pretty += કિંમત;
                pos += 5;
                ચાલુ;
            }
        } નહીંતર જો (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        જો (specialChar) {
            જો (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.ઉમેરો(lastIdent, "\\");
                inEscapedIdent = ખોટું;
            }

            pretty += specialChar;

            જો (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = ખોટું;
            }
        } નહીંતર {
            pretty += pos[0];
            ++pos;
        }
    }
    જો (inEscapedIdent) {
        pretty += " ";
        pretty.ઉમેરો(lastIdent, "\\");
    }
    જો (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    જો (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    પાછા pretty;
}

string Ast_ગાંઠ::prettyTypeName() અચલ {
    જો (name() == "") પાછા typeName();
    પાછા std::string{typeName()} + " '" + pretty_Name_સંદેશ() + "'";
}

//######################################################################
// Insertion

શૂન્ય Ast_ગાંઠ::debug_વૃક્ષ_Change(અચલ Ast_ગાંઠ* nodep, અચલ અક્ષર* prefix, પૂર્ણાંક lineno, તાર્કિક next) {
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
Ast_ગાંઠ* Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(Ast_ગાંઠ* nodep, Ast_ગાંઠ* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_વૃક્ષ_Change(nodep, "-addNextThs: ", __LINE__, ખોટું);
    debug_વૃક્ષ_Change(newp, "-addNextNew: ", __LINE__, સાચું);
    જો (!nodep)  // verilog.y and lots of other places assume this
        પાછા newp;

    // Find end of old list
    Ast_ગાંઠ* oldtailp = nodep;
    જો (oldtailp->m_nextp) {
        જો (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } નહીંતર {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            જ્યારે (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_ગાંઠ* અચલ newtailp = newp->m_headtailp;
    Ast_ગાંઠ* અચલ headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_ગણતરી_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_વૃક્ષ_Change(nodep, "-addNextOut:", __LINE__, સાચું);
    પાછા nodep;
}

શૂન્ય Ast_ગાંઠ::addNextHere(Ast_ગાંઠ* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_વૃક્ષ_Change(આ, "-addHereThs: ", __LINE__, ખોટું);
    debug_વૃક્ષ_Change(newp, "-addHereNew: ", __LINE__, સાચું);
    newp->edit_ગણતરી_Inc();

    Ast_ગાંઠ* અચલ addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_ગાંઠ* અચલ oldnextp = આ->m_nextp;
    આ->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    જો (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = આ;

    // Head/tail
    Ast_ગાંઠ* અચલ oldheadtailp = આ->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    જો (oldheadtailp) {  // else in middle of list, no change
        જો (oldheadtailp == આ) {  // this was one node
            આ->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } નહીંતર જો (!oldnextp) {  // this was tail
            આ->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_વૃક્ષ_Change(આ, "-addHereOut: ", __LINE__, સાચું);
}

શૂન્ય Ast_ગાંઠ::setOp1p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, આ, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_વૃક્ષ_Change(આ, "-setOp1pThs: ", __LINE__, ખોટું);
    debug_વૃક્ષ_Change(newp, "-setOp1pNew: ", __LINE__, સાચું);
    m_op1p = newp;
    newp->edit_ગણતરી_Inc();
    newp->m_backp = આ;
    debug_વૃક્ષ_Change(આ, "-setOp1pOut: ", __LINE__, ખોટું);
}

શૂન્ય Ast_ગાંઠ::setOp2p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, આ, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_વૃક્ષ_Change(આ, "-setOp2pThs: ", __LINE__, ખોટું);
    debug_વૃક્ષ_Change(newp, "-setOp2pNew: ", __LINE__, સાચું);
    m_op2p = newp;
    newp->edit_ગણતરી_Inc();
    newp->m_backp = આ;
    debug_વૃક્ષ_Change(આ, "-setOp2pOut: ", __LINE__, ખોટું);
}

શૂન્ય Ast_ગાંઠ::setOp3p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, આ, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_વૃક્ષ_Change(આ, "-setOp3pThs: ", __LINE__, ખોટું);
    debug_વૃક્ષ_Change(newp, "-setOp3pNew: ", __LINE__, સાચું);
    m_op3p = newp;
    newp->edit_ગણતરી_Inc();
    newp->m_backp = આ;
    debug_વૃક્ષ_Change(આ, "-setOp3pOut: ", __LINE__, ખોટું);
}

શૂન્ય Ast_ગાંઠ::setOp4p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, આ, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_વૃક્ષ_Change(આ, "-setOp4pThs: ", __LINE__, ખોટું);
    debug_વૃક્ષ_Change(newp, "-setOp4pNew: ", __LINE__, સાચું);
    m_op4p = newp;
    newp->edit_ગણતરી_Inc();
    newp->m_backp = આ;
    debug_વૃક્ષ_Change(આ, "-setOp4pOut: ", __LINE__, ખોટું);
}

શૂન્ય Ast_ગાંઠ::addOp1p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    જો (!m_op1p) {
        op1p(newp);
    } નહીંતર {
        m_op1p->addNext(newp);
    }
}

શૂન્ય Ast_ગાંઠ::addOp2p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    જો (!m_op2p) {
        op2p(newp);
    } નહીંતર {
        m_op2p->addNext(newp);
    }
}

શૂન્ય Ast_ગાંઠ::addOp3p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    જો (!m_op3p) {
        op3p(newp);
    } નહીંતર {
        m_op3p->addNext(newp);
    }
}

શૂન્ય Ast_ગાંઠ::addOp4p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    જો (!m_op4p) {
        op4p(newp);
    } નહીંતર {
        m_op4p->addNext(newp);
    }
}

શૂન્ય Ast_ગાંઠ::prependOp1p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    જો (!m_op1p) {
        op1p(newp);
    } નહીંતર {
        m_op1p->addHereThisAsNext(newp);
    }
}

શૂન્ય Ast_ગાંઠ::prependOp2p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    જો (!m_op2p) {
        op2p(newp);
    } નહીંતર {
        m_op2p->addHereThisAsNext(newp);
    }
}

શૂન્ય Ast_ગાંઠ::prependOp3p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    જો (!m_op3p) {
        op3p(newp);
    } નહીંતર {
        m_op3p->addHereThisAsNext(newp);
    }
}

શૂન્ય Ast_ગાંઠ::prependOp4p(Ast_ગાંઠ* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    જો (!m_op4p) {
        op4p(newp);
    } નહીંતર {
        m_op4p->addHereThisAsNext(newp);
    }
}

શૂન્ય Ast_ગાંઠ::replaceWith(Ast_ગાંઠ* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    આ->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
શૂન્ય Ast_ગાંઠ::replaceWithKeepDType(Ast_ગાંઠ* newp) {
    newp->dtypeFrom(આ);
    replaceWith(newp);
}

શૂન્ય VNRelinker::dump(std::ostream& str) અચલ {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_ગાંઠ* Ast_ગાંઠ::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_વૃક્ષ_Change(આ, "-unlinkWNextThs: ", __LINE__, સાચું);
    Ast_ગાંઠ* અચલ oldp = આ;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ગણતરી_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ગાંઠ* અચલ backp = oldp->m_backp;
    જો (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        જો (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } નહીંતર જો (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } નહીંતર જો (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } નહીંતર જો (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } નહીંતર જો (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } નહીંતર {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    જો (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_ગાંઠ* oldtailp = backp->m_headtailp;
        જો (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            જ્યારે (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_ગાંઠ* અચલ oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } નહીંતર જો (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } નહીંતર જો (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } નહીંતર જો (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } નહીંતર જો (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } નહીંતર {
        આ->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    જો (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_વૃક્ષ_Change(oldp, "-unlinkWNextOut: ", __LINE__, સાચું);
    પાછા oldp;
}

Ast_ગાંઠ* Ast_ગાંઠ::unlinkFrBack(VNRelinker* linkerp) {
    debug_વૃક્ષ_Change(આ, "-unlinkFrBkThs: ", __LINE__, સાચું);
    Ast_ગાંઠ* અચલ oldp = આ;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ગણતરી_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ગાંઠ* અચલ backp = oldp->m_backp;
    જો (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        જો (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        જો (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } નહીંતર જો (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } નહીંતર જો (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } નહીંતર જો (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } નહીંતર જો (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } નહીંતર {
            આ->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    જો (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        જો (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        જો (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } નહીંતર {
        જો (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } નહીંતર જો (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } નહીંતર જો (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } નહીંતર જો (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } નહીંતર {
            આ->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        જો (oldp->m_nextp) {
            Ast_ગાંઠ* અચલ newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    જો (oldp->m_iterpp) {  // Only if no linker, point to next in list
        જો (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_વૃક્ષ_Change(oldp, "-unlinkFrBkOut: ", __LINE__, સાચું);
    પાછા oldp;
}

શૂન્ય Ast_ગાંઠ::relink(VNRelinker* linkerp) {
    જો (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_ગાંઠ* અચલ newp = આ;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_ગણતરી_Inc();

    જો (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_ગાંઠ* અચલ backp = linkerp->m_backp;
    debug_વૃક્ષ_Change(આ, "-relinkNew: ", __LINE__, સાચું);
    debug_વૃક્ષ_Change(backp, "-relinkTre: ", __LINE__, સાચું);

    પસંદગી (linkerp->m_chg) {
    સ્થિતિ VNRelinker::RELINK_NEXT: backp->addNextHere(newp); તોડો;
    સ્થિતિ VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); તોડો;
    સ્થિતિ VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); તોડો;
    સ્થિતિ VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); તોડો;
    સ્થિતિ VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); તોડો;
    મૂળભૂત: આ->v3fatalSrc("Relink of node without any link to change."); તોડો;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    જો (linkerp->m_iterpp) {
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
    debug_વૃક્ષ_Change(આ, "-relinkOut: ", __LINE__, સાચું);
}

શૂન્ય Ast_ગાંઠ::relinkOneLink(Ast_ગાંઠ*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_ગાંઠ* newp) {
    જો (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_ગાંઠ* અચલ newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_ગાંઠ* અચલ oldlistlastp = pointpr->m_headtailp;
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

શૂન્ય Ast_ગાંઠ::addHereThisAsNext(Ast_ગાંઠ* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(આ->m_backp, આ, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_ગાંઠ* અચલ backp = આ->m_backp;
    Ast_ગાંઠ* અચલ newLastp = newp->m_headtailp;
    //
    આ->edit_ગણતરી_Inc();
    // Common linkage
    newLastp->m_nextp = આ;
    આ->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    જો (backp && backp->m_nextp == આ) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } નહીંતર {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        જો (backp) {
            જો (backp->m_op1p == આ) {
                backp->m_op1p = newp;
            } નહીંતર જો (backp->m_op2p == આ) {
                backp->m_op2p = newp;
            } નહીંતર જો (backp->m_op3p == આ) {
                backp->m_op3p = newp;
            } નહીંતર {
                UASSERT_OBJ(backp->m_op4p == આ, આ, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_ગાંઠ* અચલ tailp = આ->m_headtailp;
        આ->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    જો (newLastp->m_iterpp) *(newLastp->m_iterpp) = આ;
    જો (આ->m_iterpp) {
        *(આ->m_iterpp) = newp;
        આ->m_iterpp = nullptr;
    }
    //
    debug_વૃક્ષ_Change(આ, "-addHereThisAsNext: ", __LINE__, સાચું);
}

//======================================================================
// Clone

Ast_ગાંઠ* Ast_ગાંઠ::clone_વૃક્ષ_Iter(તાર્કિક needPure) {
    // private: Clone single node and children
    જો (needPure) purityCheck();
    Ast_ગાંઠ* અચલ newp = આ->clone();
    જો (આ->m_op1p) newp->op1p(આ->m_op1p->clone_વૃક્ષ_Iter_List(needPure));
    જો (આ->m_op2p) newp->op2p(આ->m_op2p->clone_વૃક્ષ_Iter_List(needPure));
    જો (આ->m_op3p) newp->op3p(આ->m_op3p->clone_વૃક્ષ_Iter_List(needPure));
    જો (આ->m_op4p) newp->op4p(આ->m_op4p->clone_વૃક્ષ_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(આ);  // Save pointers to/from both to simplify relinking.
    આ->clonep(newp);  // Save pointers to/from both to simplify relinking.
    પાછા newp;
}

Ast_ગાંઠ* Ast_ગાંઠ::clone_વૃક્ષ_Iter_List(તાર્કિક needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_ગાંઠ* newheadp = nullptr;
    Ast_ગાંઠ* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    માટે (Ast_ગાંઠ* oldp = આ; oldp; oldp = oldp->m_nextp) {
        Ast_ગાંઠ* અચલ newp = oldp->clone_વૃક્ષ_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        જો (newtailp) newtailp->m_nextp = newp;
        જો (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    પાછા newheadp;
}

Ast_ગાંઠ* Ast_ગાંઠ::clone_વૃક્ષ(તાર્કિક cloneNextLink, તાર્કિક needPure) {
    debug_વૃક્ષ_Change(આ, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_વૃક્ષ();
    Ast_ગાંઠ* newp;
    જો (cloneNextLink && આ->m_nextp) {
        newp = clone_વૃક્ષ_Iter_List(needPure);
    } નહીંતર {
        newp = clone_વૃક્ષ_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_વૃક્ષ();
    debug_વૃક્ષ_Change(newp, "-cloneOut: ", __LINE__, સાચું);
    પાછા newp;
}

શૂન્ય Ast_ગાંઠ::purityCheck() {
    જો (VL_UNLIKELY(!isPure())) {
        આ->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << આ->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

શૂન્ય Ast_ગાંઠ::delete_ગાંઠ() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_ગણતરી_Inc();
    // Change links of old node so we coredump if used
    આ->m_nextp = reinterpret_cast<Ast_ગાંઠ*>(0x1);
    આ->m_backp = reinterpret_cast<Ast_ગાંઠ*>(0x1);
    આ->m_headtailp = reinterpret_cast<Ast_ગાંઠ*>(0x1);
    આ->m_op1p = reinterpret_cast<Ast_ગાંઠ*>(0x1);
    આ->m_op2p = reinterpret_cast<Ast_ગાંઠ*>(0x1);
    આ->m_op3p = reinterpret_cast<Ast_ગાંઠ*>(0x1);
    આ->m_op4p = reinterpret_cast<Ast_ગાંઠ*>(0x1);
    આ->m_iterpp = reinterpret_cast<Ast_ગાંઠ**>(0x1);
    જો (
#જો !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        સાચું
#નહીંતર
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete આ;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

શૂન્ય Ast_ગાંઠ::delete_વૃક્ષ_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    માટે (Ast_ગાંઠ *nodep = આ, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        જો (nodep->m_op1p) nodep->m_op1p->delete_વૃક્ષ_Iter();
        જો (nodep->m_op2p) nodep->m_op2p->delete_વૃક્ષ_Iter();
        જો (nodep->m_op3p) nodep->m_op3p->delete_વૃક્ષ_Iter();
        જો (nodep->m_op4p) nodep->m_op4p->delete_વૃક્ષ_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_ગાંઠ();
    }
}

શૂન્ય Ast_ગાંઠ::delete_વૃક્ષ() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_વૃક્ષ_Change(આ, "-delTree:  ", __LINE__, સાચું);
    આ->edit_ગણતરી_Inc();
    // MUST be depth first!
    delete_વૃક્ષ_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
શૂન્ય* Ast_ગાંઠ::operator નવું(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_ગાંઠ* અચલ objp = static_cast<Ast_ગાંઠ*>(::operator નવું(size));
    V3Broken::addNewed(objp);
    પાછા objp;
}

શૂન્ય Ast_ગાંઠ::operator delete(શૂન્ય* objp, size_t size) {
    જો (!objp) પાછા;
    અચલ Ast_ગાંઠ* અચલ nodep = static_cast<Ast_ગાંઠ*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
શૂન્ય* Ast_ગાંઠ::operator નવું(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    સ્થિર અચલ size_t ASTGEN_MAX_ગાંઠ_SIZE = []() {
        size_t maxSize = 0;
        માટે (size_t t = 0; t < VNType::અંક_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        પાછા maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    સ્થિર uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    જો (પૂર્ણાંક seed = v3Global.opt.debugAllocRandom()) {
        સ્થિર V3Mutex s_mutex;
        અચલ V3LockGuard lock{s_mutex};
        સ્થિર std::array<શૂન્ય*, POOL_SIZE> s_ગાંઠ_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_ગાંઠ_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        જો (!s_lfsr) {
            s_lfsr = seed;
            જો (!s_lfsr) s_lfsr = ~s_lfsr;
            માટે (size_t i = 0; i < POOL_SIZE; ++i) {
                s_ગાંઠ_Pool[i] = ::operator નવું(ASTGEN_MAX_ગાંઠ_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_ગાંઠ_Pool), std::end(s_ગાંઠ_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        અચલ size_t index = s_lfsr % POOL_SIZE;
        Ast_ગાંઠ* અચલ objp = static_cast<Ast_ગાંઠ*>(s_ગાંઠ_Pool[index]);
        s_ગાંઠ_Pool[index] = ::operator નવું(ASTGEN_MAX_ગાંઠ_SIZE + 64);  // For later new()
        પાછા objp;
    } નહીંતર {
        Ast_ગાંઠ* અચલ objp = static_cast<Ast_ગાંઠ*>(::operator નવું(size));
        પાછા objp;
    }
}
શૂન્ય Ast_ગાંઠ::operator delete(શૂન્ય* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (શૂન્ય)objp;
    (શૂન્ય)size;
}
#endif

//======================================================================
// Iterators

શૂન્ય Ast_ગાંઠ::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    જો (m_op1p) m_op1p->iterateAndNext(v);
    જો (m_op2p) m_op2p->iterateAndNext(v);
    જો (m_op3p) m_op3p->iterateAndNext(v);
    જો (m_op4p) m_op4p->iterateAndNext(v);
}

શૂન્ય Ast_ગાંઠ::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    જો (m_op1p) m_op1p->iterateAndNextConst(v);
    જો (m_op2p) m_op2p->iterateAndNextConst(v);
    જો (m_op3p) m_op3p->iterateAndNextConst(v);
    જો (m_op4p) m_op4p->iterateAndNextConst(v);
}

શૂન્ય Ast_ગાંઠ::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_ગાંઠ* nodep = આ;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    જો (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    જ્યારે (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        જો (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_ગાંઠ* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        જો (!niterp) પાછા;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        જો (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } નહીંતર {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

શૂન્ય Ast_ગાંઠ::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_ગાંઠ* nodep = આ;
    જ્યારે (nodep->m_nextp) nodep = nodep->m_nextp;
    જ્યારે (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        જો (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } નહીંતર {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

શૂન્ય Ast_ગાંઠ::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    જો (m_op1p) m_op1p->iterateListBackwardsConst(v);
    જો (m_op2p) m_op2p->iterateListBackwardsConst(v);
    જો (m_op3p) m_op3p->iterateListBackwardsConst(v);
    જો (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

શૂન્ય Ast_ગાંઠ::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_ગાંઠ* nodep = આ;
    કરો {
        Ast_ગાંઠ* અચલ nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } જ્યારે (nodep);
}

Ast_ગાંઠ* Ast_ગાંઠ::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_ગાંઠ* nodep = આ;  // Note "this" may point to bogus point later in this function
    જો (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } નહીંતર જો (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* અચલ tempp = નવું AstBegin{nodep->fileline(), "[EditWrapper]", nodep, ખોટું};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_વૃક્ષ(), tempp);
    } નહીંતર {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_ગાંઠ** nextnodepp = nullptr;
        જો (આ->m_backp->m_op1p == આ) {
            nextnodepp = &(આ->m_backp->m_op1p);
        } નહીંતર જો (આ->m_backp->m_op2p == આ) {
            nextnodepp = &(આ->m_backp->m_op2p);
        } નહીંતર જો (આ->m_backp->m_op3p == આ) {
            nextnodepp = &(આ->m_backp->m_op3p);
        } નહીંતર જો (આ->m_backp->m_op4p == આ) {
            nextnodepp = &(આ->m_backp->m_op4p);
        } નહીંતર જો (આ->m_backp->m_nextp == આ) {
            nextnodepp = &(આ->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, આ, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    પાછા nodep;
}

//======================================================================

શૂન્ય Ast_ગાંઠ::clone_Relink_વૃક્ષ() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    માટે (Ast_ગાંઠ* nodep = આ; nodep; nodep = nodep->m_nextp) {
        જો (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        જો (nodep->m_op1p) nodep->m_op1p->clone_Relink_વૃક્ષ();
        જો (nodep->m_op2p) nodep->m_op2p->clone_Relink_વૃક્ષ();
        જો (nodep->m_op3p) nodep->m_op3p->clone_Relink_વૃક્ષ();
        જો (nodep->m_op4p) nodep->m_op4p->clone_Relink_વૃક્ષ();
    }
}

//======================================================================
// Comparison

તાર્કિક Ast_ગાંઠ::gate_વૃક્ષ_Iter() અચલ {
    // private: Return true if the two trees are identical
    જો (!isGateOptimizable()) પાછા ખોટું;
    જો (m_op1p && !m_op1p->gate_વૃક્ષ_Iter()) પાછા ખોટું;
    જો (m_op2p && !m_op2p->gate_વૃક્ષ_Iter()) પાછા ખોટું;
    જો (m_op3p && !m_op3p->gate_વૃક્ષ_Iter()) પાછા ખોટું;
    જો (m_op4p && !m_op4p->gate_વૃક્ષ_Iter()) પાછા ખોટું;
    પાછા સાચું;
}

તાર્કિક Ast_ગાંઠ::same_વૃક્ષ_Iter(અચલ Ast_ગાંઠ* node1p, અચલ Ast_ગાંઠ* node2p, તાર્કિક ignNext,
                           તાર્કિક gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    જ્યારે (સાચું) {
        જો (!node1p && !node2p) પાછા સાચું;
        જો (!node1p || !node2p) પાછા ખોટું;
        જો (node1p->type() != node2p->type()) પાછા ખોટું;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        જો (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) પાછા ખોટું;
        જો (!node1p->same_ગાંઠ(node2p) || (gateOnly && !node1p->isGateOptimizable())) પાછા ખોટું;
        જો (!same_વૃક્ષ_Iter(node1p->m_op1p, node2p->m_op1p, ખોટું, gateOnly)
            || !same_વૃક્ષ_Iter(node1p->m_op2p, node2p->m_op2p, ખોટું, gateOnly)
            || !same_વૃક્ષ_Iter(node1p->m_op3p, node2p->m_op3p, ખોટું, gateOnly)
            || !same_વૃક્ષ_Iter(node1p->m_op4p, node2p->m_op4p, ખોટું, gateOnly)) {
            પાછા ખોટું;
        }
        જો (ignNext) પાછા સાચું;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

શૂન્ય Ast_ગાંઠ::check_વૃક્ષ_Iter(અચલ Ast_ગાંઠ* prevBackp) અચલ VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == આ->backp(), આ, "Back node inconsistent");
    અચલ VNTypeInfo& typeInfo = VNType::typeInfo(આ->type());
    માટે (પૂર્ણાંક i = 1; i <= 4; i++) {
        Ast_ગાંઠ* nodep = nullptr;
        પસંદગી (i) {
        સ્થિતિ 1: nodep = op1p(); તોડો;
        સ્થિતિ 2: nodep = op2p(); તોડો;
        સ્થિતિ 3: nodep = op3p(); તોડો;
        સ્થિતિ 4: nodep = op4p(); તોડો;
        મૂળભૂત: આ->v3fatalSrc("Bad case"); તોડો;
        }
        // cppcheck-suppress danglingTempReference
        અચલ અક્ષર* opName = typeInfo.m_opNamep[i - 1];
        પસંદગી (typeInfo.m_opType[i - 1]) {
        સ્થિતિ VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, આ, typeInfo.m_namep << " must not use " << opName << "()");
            તોડો;
        સ્થિતિ VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, આ,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), આ,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_વૃક્ષ_Iter(આ);
            તોડો;
        સ્થિતિ VNTypeInfo::OP_LIST:
            જો (અચલ Ast_ગાંઠ* અચલ headp = nodep) {
                // cppcheck-suppress shadowFunction
                અચલ Ast_ગાંઠ* backp = આ;
                અચલ Ast_ગાંઠ* tailp;
                અચલ Ast_ગાંઠ* opp = headp;
                કરો {
                    opp->check_વૃક્ષ_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } જ્યારે (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            તોડો;
        સ્થિતિ VNTypeInfo::OP_OPTIONAL:
            જો (nodep) {
                UASSERT_OBJ(!nodep->nextp(), આ,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_વૃક્ષ_Iter(આ);
            }
            તોડો;
        મૂળભૂત: આ->v3fatalSrc("Bad case"); તોડો;
        }
    }
    જો (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        જો (અચલ Ast_ગાંઠ_પદાવલિ* અચલ exprp = VN_CAST(આ, ગાંઠ_પદાવલિ)) {
            અચલ અક્ષર* અચલ whyp = exprp->widthMismatch();
            જો (whyp) {
                auto dtypeStr = [](અચલ Ast_ગાંઠ_પદાવલિ* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    પાછા ss.str();
                };
                જો (અચલ Ast_ગાંઠ_Uniop* અચલ uniopp = VN_CAST(exprp, ગાંઠ_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } નહીંતર જો (અચલ Ast_ગાંઠ_Biop* અચલ biopp = VN_CAST(exprp, ગાંઠ_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } નહીંતર {
                    UASSERT_OBJ(ખોટું, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
અક્ષર* Ast_ગાંઠ::dump_વૃક્ષ_Json_Gdb(અચલ Ast_ગાંઠ* nodep) {
    જો (!nodep) પાછા strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_વૃક્ષ_Json(nodepStream);
    અચલ std::string str = nodepStream.rdbuf()->str();
    પાછા strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
અક્ષર* Ast_ગાંઠ::dump_વૃક્ષ_Json_Gdb(અચલ અક્ષર* str) { પાછા strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
અક્ષર* Ast_ગાંઠ::dump_વૃક્ષ_Json_Gdb(intptr_t nodep) {
    જો (!nodep) પાછા strdup("{\"addr\":\"NULL\"}\n");
    પાછા dump_વૃક્ષ_Json_Gdb(reinterpret_cast<અચલ Ast_ગાંઠ*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
શૂન્ય Ast_ગાંઠ::dumpGdb(અચલ Ast_ગાંઠ* nodep) {  // For GDB only  // LCOV_EXCL_START
    જો (!nodep) {
        cout << "<nullptr>" << endl;
        પાછા;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
શૂન્ય Ast_ગાંઠ::dumpGdbHeader() અચલ {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
શૂન્ય Ast_ગાંઠ::dump_વૃક્ષ_Gdb(અચલ Ast_ગાંઠ* nodep) {  // For GDB only  // LCOV_EXCL_START
    જો (!nodep) {
        cout << "<nullptr>" << endl;
        પાછા;
    }
    nodep->dumpGdbHeader();
    nodep->dump_વૃક્ષ(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
શૂન્ય Ast_ગાંઠ::dump_વૃક્ષ_File_Gdb(અચલ Ast_ગાંઠ* nodep,  // LCOV_EXCL_START
                              અચલ અક્ષર* filenamep) {  // For GDB only
    જો (!nodep) {
        cout << "<nullptr>" << endl;
        પાછા;
    }
    અચલ string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_વૃક્ષ_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
શૂન્ય Ast_ગાંઠ::checkIter() અચલ {
    જો (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        આ->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

શૂન્ય Ast_ગાંઠ::dumpPtrs(std::ostream& os) અચલ {
    os << "This=" << typeName() << " " << cvtToHex(આ);
    os << " back=" << cvtToHex(backp());
    જો (nextp()) os << " next=" << cvtToHex(nextp());
    જો (m_headtailp == આ) {
        os << " headtail=this";
    } નહીંતર {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    જો (op1p()) os << " op1p=" << cvtToHex(op1p());
    જો (op2p()) os << " op2p=" << cvtToHex(op2p());
    જો (op3p()) os << " op3p=" << cvtToHex(op3p());
    જો (op4p()) os << " op4p=" << cvtToHex(op4p());
    અચલ auto dumpUser = [&os](અચલ અક્ષર* prefix, અચલ VNUser& user) {
        અચલ std::string s
            = user.dumpStr([](અચલ શૂન્ય* p) -> std::string { પાછા cvtToHex(p); });
        જો (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    જો (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

શૂન્ય Ast_ગાંઠ::dump_વૃક્ષ(std::ostream& os, અચલ string& indent, પૂર્ણાંક maxDepth) અચલ {
    સ્થિર પૂર્ણાંક s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << આ << '\n';
    જો (VN_DELETED(આ)) પાછા;
    જો (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    જો (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(આ)
           << endl;  // endl intentional to do flush
        પાછા;
    }  // LCOV_EXCL_STOP
    જો (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    જો (maxDepth == 1) {
        જો (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } નહીંતર {
        માટે (અચલ Ast_ગાંઠ* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_વૃક્ષ(os, indent + "1:", maxDepth - 1);
        }
        માટે (અચલ Ast_ગાંઠ* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_વૃક્ષ(os, indent + "2:", maxDepth - 1);
        }
        માટે (અચલ Ast_ગાંઠ* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_વૃક્ષ(os, indent + "3:", maxDepth - 1);
        }
        માટે (અચલ Ast_ગાંઠ* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_વૃક્ષ(os, indent + "4:", maxDepth - 1);
        }
    }
}

શૂન્ય Ast_ગાંઠ::dump_વૃક્ષ_And_Next(std::ostream& os, અચલ string& indent, પૂર્ણાંક maxDepth) અચલ {
    // Audited to make sure this is never nullptr
    માટે (અચલ Ast_ગાંઠ* nodep = આ; nodep; nodep = nodep->nextp()) {
        nodep->dump_વૃક્ષ(os, indent, maxDepth);
    }
}

શૂન્ય Ast_ગાંઠ::dump_વૃક્ષ_File(અચલ string& filename, તાર્કિક doDump) {
    // Not const function as calls checkTree
    જો (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            અચલ std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            જો (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_ગણતરી_Last();
            *logsp << "> to <e" << std::dec << edit_ગણતરી_Gbl() << ">\n";
            જો (edit_ગણતરી_Gbl() == edit_ગણતરી_Last() && ::dump_વૃક્ષ_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } નહીંતર {
                dump_વૃક્ષ(*logsp);
                edit_ગણતરી_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

સ્થિર શૂન્ય drawChildren(std::ostream& os, અચલ Ast_ગાંઠ* thisp, અચલ Ast_ગાંઠ* childp,
                         અચલ std::string& childName) {
    જો (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        માટે (અચલ Ast_ગાંઠ* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_વૃક્ષ_Dot(os);
            જો (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

શૂન્ય Ast_ગાંઠ::dump_વૃક્ષ_Dot(std::ostream& os) અચલ {
    os << "\tn" << cvtToHex(આ) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, આ, m_op1p, "op1");
    drawChildren(os, આ, m_op2p, "op2");
    drawChildren(os, આ, m_op3p, "op3");
    drawChildren(os, આ, m_op4p, "op4");
}

શૂન્ય Ast_ગાંઠ::dump_વૃક્ષ_Json_File(અચલ string& filename, તાર્કિક doDump) {
    જો (!doDump) પાછા;
    UINFO(2, "Dumping " << filename);
    અચલ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    જો (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_વૃક્ષ_Json(*treejsonp);
    *treejsonp << '\n';
}

શૂન્ય Ast_ગાંઠ::dumpJsonMetaFileGdb(અચલ અક્ષર* filename) { dumpJsonMetaFile(filename); }
શૂન્ય Ast_ગાંઠ::dumpJsonMetaFile(અચલ string& filename) {
    UINFO(2, "Dumping " << filename);
    અચલ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    જો (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_લીટી::file_Name_અંક_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

શૂન્ય Ast_ગાંઠ::dump_વૃક્ષ_Dot_File(અચલ string& filename, તાર્કિક doDump) {
    જો (doDump) {
        UINFO(2, "Dumping " << filename);
        અચલ std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        જો (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_વૃક્ષ_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_ગાંઠ::instanceStr() અચલ {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_ગણતરી = 0;
    // Walk 'backp' chain
    માટે (અચલ Ast_ગાંઠ* currp = આ; currp; currp = currp->backp(), ++iter_ગણતરી) {
        જો (VL_UNCOVERABLE(iter_ગણતરી >= maxIterations)) પાછા "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        જો (અચલ AstScope* અચલ scopep = VN_CAST(currp, Scope)) {
            પાછા scopep->is_ટોચ() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        જો (અચલ AstModule* અચલ modp = VN_CAST(currp, Module)) {
            અચલ string instanceName = modp->someInstanceName();
            પાછા instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    પાછા "";
}
શૂન્ય Ast_ગાંઠ::v3errorEnd(અચલ std::ostringstream& str) અચલ VL_RELEASE(V3Error::s().m_mutex) {
    જો (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } નહીંતર {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        અચલ string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        જો (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_ગાંઠ*>(આ)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
શૂન્ય Ast_ગાંઠ::v3errorEndFatal(અચલ std::ostringstream& str) અચલ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    પ્રતિજ્ઞા(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

શૂન્ય Ast_ગાંઠ::dtypeChgSigned(તાર્કિક flag) {
    UASSERT_OBJ(dtypep(), આ, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
શૂન્ય Ast_ગાંઠ::dtypeChgWidth(પૂર્ણાંક width, પૂર્ણાંક widthMin) {
    UASSERT_OBJ(dtypep(), આ, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

શૂન્ય Ast_ગાંઠ::dtypeChgWidthSigned(પૂર્ણાંક width, પૂર્ણાંક widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), આ, "No dtype when changing width");
    અચલ Ast_ગાંઠ_DType* અચલ enumDTypep = dtypep()->skipRefToEnump();
    જો (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        પાછા;  // Correct already
    }
    અચલ Ast_ગાંઠ_DType* અચલ dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_ગાંઠ_DType* Ast_ગાંઠ::findBasicDType(VBasicDTypeKwd kwd) અચલ {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    પાછા v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findBitDType(પૂર્ણાંક width, પૂર્ણાંક widthMin, VSigning numeric) અચલ {
    પાછા v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findLogicDType(પૂર્ણાંક width, પૂર્ણાંક widthMin, VSigning numeric) અચલ {
    પાછા v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findLogic2StateDType(પૂર્ણાંક width, પૂર્ણાંક widthMin, VSigning numeric) અચલ {
    પાછા v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findLogicRangeDType(અચલ VNumRange& range, પૂર્ણાંક widthMin,
                                           VSigning numeric) અચલ {
    પાછા v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findBitRangeDType(અચલ VNumRange& range, પૂર્ણાંક widthMin,
                                         VSigning numeric) અચલ {
    પાછા v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_ગાંઠ::find_ઉમેરો_Same_DType(AstBasicDType* nodep) {
    પાછા v3Global.rootp()->typeTablep()->find_ઉમેરો_Same_DType(nodep);
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findConstraintRefDType() અચલ {
    પાછા v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findEmptyQueueDType() અચલ {
    પાછા v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findQueueIndexDType() અચલ {
    પાછા v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findStreamDType() અચલ {
    પાછા v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_ગાંઠ_DType* Ast_ગાંઠ::findVoidDType() અચલ {
    પાછા v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

સ્થિર અચલ Ast_ગાંઠ_DType* computeCastableBase(અચલ Ast_ગાંઠ_DType* nodep) {
    જ્યારે (સાચું) {
        જો (અચલ AstPackArrayDType* અચલ packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            ચાલુ;
        }
        જો (અચલ Ast_ગાંઠ_DType* અચલ refp = nodep->skipRefToEnump()) {
            જો (refp != nodep) {
                nodep = refp;
                ચાલુ;
            }
        }
        પાછા nodep;
    }
}

સ્થિર VCastable computeCastableImp(અચલ Ast_ગાંઠ_DType* toDtp, અચલ Ast_ગાંઠ_DType* fromDtp,
                                    અચલ Ast_ગાંઠ* fromConstp, અચલ તાર્કિક checkIfaceArgCompat) {
    અચલ VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    જો (toDtp == fromDtp) પાછા VCastable::SAMEISH;
    જો (toDtp->similarDType(fromDtp)) પાછા VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    અચલ Ast_ગાંઠ_DType* fromBaseDtp = computeCastableBase(fromDtp);

    અચલ તાર્કિક fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, ગાંઠ_UOr_Struct_DType);

    અચલ Ast_ગાંઠ_DType* toBaseDtp = computeCastableBase(toDtp);
    અચલ તાર્કિક toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, ગાંઠ_UOr_Struct_DType);

    જો (toBaseDtp == fromBaseDtp) પાછા VCastable::COMPATIBLE;

    જો (toNumericable) {
        જો (fromNumericable) પાછા VCastable::COMPATIBLE;
    } નહીંતર જો (VN_IS(toBaseDtp, EnumDType)) {
        જો (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_વૃક્ષ(fromDtp))
            પાછા VCastable::ENUM_IMPLICIT;
        જો (fromNumericable) પાછા VCastable::ENUM_EXPLICIT;
    } નહીંતર જો (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        પાછા VCastable::COMPATIBLE;
    } નહીંતર જો (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        જો (fromConstp->isNull()) પાછા VCastable::COMPATIBLE;
    } નહીંતર જો (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        અચલ AstClass* અચલ toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        અચલ AstClass* અચલ fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        અચલ તાર્કિક downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        અચલ તાર્કિક upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        જો (upcast) પાછા VCastable::COMPATIBLE;
        જો (downcast) પાછા VCastable::DYNAMIC_CLASS;
        પાછા VCastable::INCOMPATIBLE;
    } નહીંતર જો (અચલ AstIfaceRefDType* અચલ toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(સાચું) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        અચલ AstIfaceRefDType* અચલ fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(સાચું) : fromDtp, IfaceRefDType);
        જો (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            અચલ AstUnpackArrayDType* અચલ toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            અચલ AstUnpackArrayDType* અચલ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            જો (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                પાછા VCastable::INCOMPATIBLE;
            }
            પાછા computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        જો (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            જો (!checkIfaceArgCompat) પાછા castable;
            પાછા VCastable::INCOMPATIBLE;
        }
        અચલ તાર્કિક sameModport = toIfp->modportp() == fromIfp->modportp();
        જો (!checkIfaceArgCompat) {
            જો (!toIfp->modportp() || sameModport) પાછા VCastable::COMPATIBLE;
            પાછા castable;
        }
        જો (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            પાછા VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        જો (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            પાછા VCastable::COMPATIBLE;
        }
        પાછા VCastable::INCOMPATIBLE;
    } નહીંતર જો (અચલ AstUnpackArrayDType* અચલ toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        અચલ AstUnpackArrayDType* અચલ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        જો (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            પાછા castable;
        }
        અચલ VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        અચલ Ast_ગાંઠ_DType* અચલ toSubp = toArrayp->subDTypep()->skipRefp();
        જો (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            પાછા VCastable::COMPATIBLE;
        }
    }
    પાછા castable;
}

VCastable Ast_ગાંઠ::computeCastable(અચલ Ast_ગાંઠ_DType* toDtp, અચલ Ast_ગાંઠ_DType* fromDtp,
                                   અચલ Ast_ગાંઠ* fromConstp, અચલ તાર્કિક checkIfaceArgCompat) {
    અચલ auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    જો (fromConstp) UINFO(9, "     const= " << fromConstp);
    પાછા castable;
}

Ast_ગાંઠ_DType* Ast_ગાંઠ::getCommonClassTypep(Ast_ગાંઠ* node1p, Ast_ગાંઠ* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    જો (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        અચલ VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        જો (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            પાછા node1p->dtypep();
        જો (castable == VCastable::DYNAMIC_CLASS) પાછા node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    જ્યારે (classDtypep1) {
        અચલ VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        જો (castable == VCastable::COMPATIBLE) પાછા classDtypep1;
        AstClassExtends* અચલ extendsp = classDtypep1->classp()->extendsp();
        જો (!extendsp) તોડો;
        Ast_ગાંઠ_DType* અચલ edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    પાછા nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

વર્ગ VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<પૂર્ણાંક, std::string> m_internedWordWidths;  // Interned widths
    std::map<પૂર્ણાંક, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    અચલ તાર્કિક m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    સ્થિર std::string toLetters(size_t કિંમત, તાર્કિક lowerCase = ખોટું) {
        અચલ અક્ષર base = lowerCase ? 'a' : 'A';
        std::string s;
        કરો { s += static_cast<અક્ષર>(base + કિંમત % 26); } જ્યારે (કિંમત /= 26);
        પાછા s;
    }

    અચલ std::string& internConst(અચલ AstConst& nodep) {
        અચલ auto pair = m_internedConsts.emplace(nodep.અંક().ascii(ખોટું), "");
        જો (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        પાછા pair.first->second;
    }

    અચલ std::string& internWordWidth(પૂર્ણાંક કિંમત) {
        અચલ auto pair = m_internedWordWidths.emplace(કિંમત, "");
        જો (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, સાચું);
        પાછા pair.first->second;
    }

    અચલ std::string& internWideWidth(પૂર્ણાંક કિંમત) {
        અચલ auto pair = m_internedWideWidths.emplace(કિંમત, "");
        જો (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        પાછા pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    સ્થિર તાર્કિક isLeaf(અચલ Ast_ગાંઠ* nodep) {
        અચલ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        માટે (અચલ VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            જો (opType != VNTypeInfo::OP_UNUSED) પાછા ખોટું;
        }
        પાછા સાચું;
    }

    // Render operand, return true if not unused
    શૂન્ય renderOperand(VNTypeInfo::OpEn opType, અચલ Ast_ગાંઠ* અચલ opp, uint32_t depth) {
        પસંદગી (opType) {
        સ્થિતિ VNTypeInfo::OP_UNUSED:  //
            તોડો;
        સ્થિતિ VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            તોડો;
        સ્થિતિ VNTypeInfo::OP_LIST:
            m_os << " [";
            માટે (અચલ Ast_ગાંઠ* nodep = opp; nodep; nodep = nodep->nextp()) {
                જો (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            તોડો;
        સ્થિતિ VNTypeInfo::OP_OPTIONAL:
            જો (opp) {
                m_os << " ";
                render(opp, depth);
            } નહીંતર {
                m_os << " nil";
            }
            તોડો;
        }
    }

    // Render the node into the stream.
    શૂન્ય render(અચલ Ast_ગાંઠ* nodep, uint32_t depth) {
        જો (અચલ AstConst* અચલ constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            જો (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } નહીંતર જો (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } નહીંતર {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } નહીંતર જો (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } નહીંતર જો (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } નહીંતર {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            અચલ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        અચલ Ast_ગાંઠ_DType* અચલ dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        જો (!dtypep) {
            m_os << '?';
        } નહીંતર જો (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } નહીંતર {
            અચલ પૂર્ણાંક width = nodep->width();
            જો (width == 1) {
                m_os << '1';
            } નહીંતર જો (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } નહીંતર {
                m_os << internWideWidth(width);
            }
            જો (m_dumpWidthMin) {
                m_os << '/';
                અચલ પૂર્ણાંક widthMin = nodep->widthMin();
                જો (widthMin == 1) {
                    m_os << '1';
                } નહીંતર જો (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } નહીંતર {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

જાહેર:
    VNPatternString(std::ostream& os, અચલ Ast_ગાંઠ* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_ગાંઠ_પદાવલિ::patternString(uint32_t depth) અચલ {
    std::ostringstream oss;
    VNPatternString{oss, આ, depth};
    પાછા oss.str();
}

//######################################################################
// VNDeleter

શૂન્ય VNDeleter::doDeletes() {
    માટે (Ast_ગાંઠ* અચલ nodep : m_deleteps) nodep->delete_વૃક્ષ();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#અન્તર્ભાવ "V3Ast__gen_visitor_defns.h"  // From ./astgen
