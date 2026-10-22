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

#অন্তর্ভাব "V3PchAstMT.h"

#অন্তর্ভাব "V3Broken.h"
#অন্তর্ভাব "V3File.h"

#অন্তর্ভাব <iomanip>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_গ্রন্থি::s_editCntLast = 0;
uint64_t Ast_গ্রন্থি::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
পূর্ণাঙ্ক Ast_গ্রন্থি::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

তার্কিক VNUser1InUse::s_userBusy = অসত্য;
তার্কিক VNUser2InUse::s_userBusy = অসত্য;
তার্কিক VNUser3InUse::s_userBusy = অসত্য;
তার্কিক VNUser4InUse::s_userBusy = অসত্য;

পূর্ণাঙ্ক Ast_গ্রন্থি_DType::s_unique_অঙ্ক = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(ধ্রুব string& name) {
    প্রতি (পূর্ণাঙ্ক i = 0; i < _ENUM_MAX; ++i) {
        ধ্রুব VCMethod method{i};
        যদি (name == method.ascii()) প্রত্যাগম method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    প্রত্যাগম VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ধ্রুব শূন্য*)) ধ্রুব {
#ifdef VL_USER_TYPE_CHECKS
    যদি (ধ্রুব uint64_t* ধ্রুব uip = std::get_if<uint64_t>(&m_u)) প্রত্যাগম "#"s + cvtToStr(*uip);
    যদি (শূন্য* ধ্রুব* ধ্রুব upp = std::get_if<শূন্য*>(&m_u)) প্রত্যাগম fmtAddrp(*upp);
    প্রত্যাগম "";
#অন্যথা
    // Dumps void* representation
    যদি (!m_u.up) প্রত্যাগম "";
    প্রত্যাগম fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

ধ্রুব VNTypeInfo VNType::s_typeInfoTable[VNType::অঙ্ক_TYPES()] = {
#অন্তর্ভাব "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

তার্কিক VFwdType::is_গ্রন্থি_Compatible(ধ্রুব Ast_গ্রন্থি* nodep) ধ্রুব {
    ধ্রুব Ast_গ্রন্থি* defp = nodep;
    যদি (ধ্রুব AstTypedef* ধ্রুব adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    যদি (ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব adefp = VN_CAST(defp, গ্রন্থি_DType))
        defp = adefp->skipRefToNonRefp();
    বিকল্প (m_e) {
    পক্ষ VFwdType::NONE: প্রত্যাগম সত্য; ভঙ্গ;
    পক্ষ VFwdType::ENUM: প্রত্যাগম VN_IS(defp, EnumDType); ভঙ্গ;
    পক্ষ VFwdType::STRUCT: প্রত্যাগম VN_IS(defp, StructDType); ভঙ্গ;
    পক্ষ VFwdType::UNION: প্রত্যাগম VN_IS(defp, UnionDType); ভঙ্গ;
    পক্ষ VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    পক্ষ VFwdType::CLASS: প্রত্যাগম VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); ভঙ্গ;
    পক্ষ VFwdType::GENERIC_INTERFACE: প্রত্যাগম VN_IS(defp, IfaceRefDType); ভঙ্গ;
    স্বভাব: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    প্রত্যাগম অসত্য;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ধ্রুব std::shared_ptr<ধ্রুব string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
ধ্রুব std::shared_ptr<ধ্রুব string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(তার্কিক useSelfForThis, ধ্রুব string& text) {
    প্রত্যাগম useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(তার্কিক useSelfForThis, তার্কিক protect) ধ্রুব {
    প্রত্যাগম VIdProtect::protect_শব্দগুলি_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_গ্রন্থি::Ast_গ্রন্থি(VNType t, File_সারি* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = এতত্;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = অসত্য;
    m_flags.doingWidth = অসত্য;
    m_flags.protect = সত্য;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_গুনতি_Inc();
}

Ast_গ্রন্থি* Ast_গ্রন্থি::abovep() ধ্রুব {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), এতত্, "abovep() not allowed when in midlist");
    ধ্রুব Ast_গ্রন্থি* ধ্রুব firstp = firstAbovep() ? এতত্ : m_headtailp;
    প্রত্যাগম firstp->backp();
}
Ast_গ্রন্থি* Ast_গ্রন্থি::aboveLoopp() ধ্রুব {
    // Returns parent node. Avoid using this, may have performance issues.
    ধ্রুব Ast_গ্রন্থি* nodep = এতত্;
    // Backwards over peers (versus parents)
    যাবত্ (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    প্রত্যাগম nodep->backp();
}

string Ast_গ্রন্থি::encodeName(ধ্রুব string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    প্রতি (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        যদি ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } অন্যথা যদি (pos[0] == '_') {
            out += pos[0];
            যদি (pos + 1 == namein.end()) ভঙ্গ;
            যদি (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } অন্যথা {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ধ্রুব unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    প্রত্যাগম vname.hashedName();
}

string Ast_গ্রন্থি::encode_সংখ্যা(int64_t অঙ্ক) {
    যদি (অঙ্ক < 0) প্রত্যাগম "__02D" + cvtToStr(-অঙ্ক);  // 2D=-
    প্রত্যাগম cvtToStr(অঙ্ক);
}

string Ast_গ্রন্থি::nameProtect() ধ্রুব { প্রত্যাগম VIdProtect::protectIf(name(), protect()); }
string Ast_গ্রন্থি::origNameProtect() ধ্রুব { প্রত্যাগম VIdProtect::protectIf(origName(), protect()); }

string Ast_গ্রন্থি::shortName() ধ্রুব {
    string pretty = name();
    string::size_type pos;
    যাবত্ ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    প্রত্যাগম pretty;
}

string Ast_গ্রন্থি::dedotName(ধ্রুব string& namein) {
    string pretty = namein;
    string::size_type pos;
    যাবত্ ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    যদি (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    প্রত্যাগম pretty;
}

string Ast_গ্রন্থি::nameNoArray(ধ্রুব string& namein) {
    প্রত্যাগম namein.substr(0, namein.find("__BRA__"));
}

string Ast_গ্রন্থি::vcdName(ধ্রুব string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    যাবত্ ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    যাবত্ ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    প্রত্যাগম prettyName(pretty);
}

string Ast_গ্রন্থি::prettyName(ধ্রুব string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    প্রতি (ধ্রুব বর্ণ* pos = namein.c_str(); *pos;) {
        যদি (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            অনুবর্ত;
        }
        যদি (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            যদি (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                অনুবর্ত;
            }
            যদি (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                অনুবর্ত;
            }
            যদি (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                অনুবর্ত;
            }
            যদি (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                অনুবর্ত;
            }
            যদি (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                অনুবর্ত;
            }
            যদি (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                অনুবর্ত;
            }
            যদি (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                বর্ণ মান = 0;
                মান += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                মান
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += মান;
                pos += 5;
                অনুবর্ত;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    যদি (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    যদি (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    প্রত্যাগম pretty;
}

string Ast_গ্রন্থি::vpiName(ধ্রুব string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ধ্রুব size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    তার্কিক inEscapedIdent = অসত্য;
    পূর্ণাঙ্ক lastIdent = 0;

    প্রতি (ধ্রুব বর্ণ* pos = namein.c_str() + offset; *pos;) {
        বর্ণ specialChar = 0;
        যদি (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } অন্যথা যদি (pos[0] == '_' && pos[1] == '_') {  // __
            যদি (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } অন্যথা যদি (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } অন্যথা যদি (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } অন্যথা যদি (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                অনুবর্ত;
            } অন্যথা যদি (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                বর্ণ মান = 0;
                মান += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                মান
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                যদি (মান != '_') inEscapedIdent = সত্য;

                pretty += মান;
                pos += 5;
                অনুবর্ত;
            }
        } অন্যথা যদি (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        যদি (specialChar) {
            যদি (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.ঢোকাও(lastIdent, "\\");
                inEscapedIdent = অসত্য;
            }

            pretty += specialChar;

            যদি (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = অসত্য;
            }
        } অন্যথা {
            pretty += pos[0];
            ++pos;
        }
    }
    যদি (inEscapedIdent) {
        pretty += " ";
        pretty.ঢোকাও(lastIdent, "\\");
    }
    যদি (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    যদি (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    প্রত্যাগম pretty;
}

string Ast_গ্রন্থি::prettyTypeName() ধ্রুব {
    যদি (name() == "") প্রত্যাগম typeName();
    প্রত্যাগম std::string{typeName()} + " '" + pretty_Name_বার্তা() + "'";
}

//######################################################################
// Insertion

শূন্য Ast_গ্রন্থি::debug_বৃক্ষ_Change(ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব বর্ণ* prefix, পূর্ণাঙ্ক lineno, তার্কিক next) {
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
Ast_গ্রন্থি* Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_বৃক্ষ_Change(nodep, "-addNextThs: ", __LINE__, অসত্য);
    debug_বৃক্ষ_Change(newp, "-addNextNew: ", __LINE__, সত্য);
    যদি (!nodep)  // verilog.y and lots of other places assume this
        প্রত্যাগম newp;

    // Find end of old list
    Ast_গ্রন্থি* oldtailp = nodep;
    যদি (oldtailp->m_nextp) {
        যদি (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } অন্যথা {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            যাবত্ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_গ্রন্থি* ধ্রুব newtailp = newp->m_headtailp;
    Ast_গ্রন্থি* ধ্রুব headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_গুনতি_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_বৃক্ষ_Change(nodep, "-addNextOut:", __LINE__, সত্য);
    প্রত্যাগম nodep;
}

শূন্য Ast_গ্রন্থি::addNextHere(Ast_গ্রন্থি* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_বৃক্ষ_Change(এতত্, "-addHereThs: ", __LINE__, অসত্য);
    debug_বৃক্ষ_Change(newp, "-addHereNew: ", __LINE__, সত্য);
    newp->edit_গুনতি_Inc();

    Ast_গ্রন্থি* ধ্রুব addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_গ্রন্থি* ধ্রুব oldnextp = এতত্->m_nextp;
    এতত্->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    যদি (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = এতত্;

    // Head/tail
    Ast_গ্রন্থি* ধ্রুব oldheadtailp = এতত্->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    যদি (oldheadtailp) {  // else in middle of list, no change
        যদি (oldheadtailp == এতত্) {  // this was one node
            এতত্->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } অন্যথা যদি (!oldnextp) {  // this was tail
            এতত্->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_বৃক্ষ_Change(এতত্, "-addHereOut: ", __LINE__, সত্য);
}

শূন্য Ast_গ্রন্থি::setOp1p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, এতত্, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_বৃক্ষ_Change(এতত্, "-setOp1pThs: ", __LINE__, অসত্য);
    debug_বৃক্ষ_Change(newp, "-setOp1pNew: ", __LINE__, সত্য);
    m_op1p = newp;
    newp->edit_গুনতি_Inc();
    newp->m_backp = এতত্;
    debug_বৃক্ষ_Change(এতত্, "-setOp1pOut: ", __LINE__, অসত্য);
}

শূন্য Ast_গ্রন্থি::setOp2p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, এতত্, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_বৃক্ষ_Change(এতত্, "-setOp2pThs: ", __LINE__, অসত্য);
    debug_বৃক্ষ_Change(newp, "-setOp2pNew: ", __LINE__, সত্য);
    m_op2p = newp;
    newp->edit_গুনতি_Inc();
    newp->m_backp = এতত্;
    debug_বৃক্ষ_Change(এতত্, "-setOp2pOut: ", __LINE__, অসত্য);
}

শূন্য Ast_গ্রন্থি::setOp3p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, এতত্, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_বৃক্ষ_Change(এতত্, "-setOp3pThs: ", __LINE__, অসত্য);
    debug_বৃক্ষ_Change(newp, "-setOp3pNew: ", __LINE__, সত্য);
    m_op3p = newp;
    newp->edit_গুনতি_Inc();
    newp->m_backp = এতত্;
    debug_বৃক্ষ_Change(এতত্, "-setOp3pOut: ", __LINE__, অসত্য);
}

শূন্য Ast_গ্রন্থি::setOp4p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, এতত্, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_বৃক্ষ_Change(এতত্, "-setOp4pThs: ", __LINE__, অসত্য);
    debug_বৃক্ষ_Change(newp, "-setOp4pNew: ", __LINE__, সত্য);
    m_op4p = newp;
    newp->edit_গুনতি_Inc();
    newp->m_backp = এতত্;
    debug_বৃক্ষ_Change(এতত্, "-setOp4pOut: ", __LINE__, অসত্য);
}

শূন্য Ast_গ্রন্থি::addOp1p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op1p) {
        op1p(newp);
    } অন্যথা {
        m_op1p->addNext(newp);
    }
}

শূন্য Ast_গ্রন্থি::addOp2p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op2p) {
        op2p(newp);
    } অন্যথা {
        m_op2p->addNext(newp);
    }
}

শূন্য Ast_গ্রন্থি::addOp3p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op3p) {
        op3p(newp);
    } অন্যথা {
        m_op3p->addNext(newp);
    }
}

শূন্য Ast_গ্রন্থি::addOp4p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op4p) {
        op4p(newp);
    } অন্যথা {
        m_op4p->addNext(newp);
    }
}

শূন্য Ast_গ্রন্থি::prependOp1p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op1p) {
        op1p(newp);
    } অন্যথা {
        m_op1p->addHereThisAsNext(newp);
    }
}

শূন্য Ast_গ্রন্থি::prependOp2p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op2p) {
        op2p(newp);
    } অন্যথা {
        m_op2p->addHereThisAsNext(newp);
    }
}

শূন্য Ast_গ্রন্থি::prependOp3p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op3p) {
        op3p(newp);
    } অন্যথা {
        m_op3p->addHereThisAsNext(newp);
    }
}

শূন্য Ast_গ্রন্থি::prependOp4p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op4p) {
        op4p(newp);
    } অন্যথা {
        m_op4p->addHereThisAsNext(newp);
    }
}

শূন্য Ast_গ্রন্থি::replaceWith(Ast_গ্রন্থি* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    এতত্->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
শূন্য Ast_গ্রন্থি::replaceWithKeepDType(Ast_গ্রন্থি* newp) {
    newp->dtypeFrom(এতত্);
    replaceWith(newp);
}

শূন্য VNRelinker::dump(std::ostream& str) ধ্রুব {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_গ্রন্থি* Ast_গ্রন্থি::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_বৃক্ষ_Change(এতত্, "-unlinkWNextThs: ", __LINE__, সত্য);
    Ast_গ্রন্থি* ধ্রুব oldp = এতত্;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_গুনতি_Inc();
    // cppcheck-suppress shadowFunction
    Ast_গ্রন্থি* ধ্রুব backp = oldp->m_backp;
    যদি (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        যদি (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } অন্যথা যদি (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } অন্যথা যদি (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } অন্যথা যদি (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } অন্যথা যদি (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } অন্যথা {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    যদি (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_গ্রন্থি* oldtailp = backp->m_headtailp;
        যদি (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            যাবত্ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_গ্রন্থি* ধ্রুব oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } অন্যথা যদি (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } অন্যথা যদি (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } অন্যথা যদি (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } অন্যথা যদি (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } অন্যথা {
        এতত্->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    যদি (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_বৃক্ষ_Change(oldp, "-unlinkWNextOut: ", __LINE__, সত্য);
    প্রত্যাগম oldp;
}

Ast_গ্রন্থি* Ast_গ্রন্থি::unlinkFrBack(VNRelinker* linkerp) {
    debug_বৃক্ষ_Change(এতত্, "-unlinkFrBkThs: ", __LINE__, সত্য);
    Ast_গ্রন্থি* ধ্রুব oldp = এতত্;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_গুনতি_Inc();
    // cppcheck-suppress shadowFunction
    Ast_গ্রন্থি* ধ্রুব backp = oldp->m_backp;
    যদি (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        যদি (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        যদি (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } অন্যথা যদি (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } অন্যথা যদি (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } অন্যথা যদি (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } অন্যথা যদি (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } অন্যথা {
            এতত্->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    যদি (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        যদি (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        যদি (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } অন্যথা {
        যদি (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } অন্যথা যদি (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } অন্যথা যদি (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } অন্যথা যদি (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } অন্যথা {
            এতত্->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        যদি (oldp->m_nextp) {
            Ast_গ্রন্থি* ধ্রুব newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    যদি (oldp->m_iterpp) {  // Only if no linker, point to next in list
        যদি (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_বৃক্ষ_Change(oldp, "-unlinkFrBkOut: ", __LINE__, সত্য);
    প্রত্যাগম oldp;
}

শূন্য Ast_গ্রন্থি::relink(VNRelinker* linkerp) {
    যদি (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_গ্রন্থি* ধ্রুব newp = এতত্;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_গুনতি_Inc();

    যদি (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_গ্রন্থি* ধ্রুব backp = linkerp->m_backp;
    debug_বৃক্ষ_Change(এতত্, "-relinkNew: ", __LINE__, সত্য);
    debug_বৃক্ষ_Change(backp, "-relinkTre: ", __LINE__, সত্য);

    বিকল্প (linkerp->m_chg) {
    পক্ষ VNRelinker::RELINK_NEXT: backp->addNextHere(newp); ভঙ্গ;
    পক্ষ VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); ভঙ্গ;
    পক্ষ VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); ভঙ্গ;
    পক্ষ VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); ভঙ্গ;
    পক্ষ VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); ভঙ্গ;
    স্বভাব: এতত্->v3fatalSrc("Relink of node without any link to change."); ভঙ্গ;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    যদি (linkerp->m_iterpp) {
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
    debug_বৃক্ষ_Change(এতত্, "-relinkOut: ", __LINE__, সত্য);
}

শূন্য Ast_গ্রন্থি::relinkOneLink(Ast_গ্রন্থি*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_গ্রন্থি* newp) {
    যদি (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_গ্রন্থি* ধ্রুব newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_গ্রন্থি* ধ্রুব oldlistlastp = pointpr->m_headtailp;
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

শূন্য Ast_গ্রন্থি::addHereThisAsNext(Ast_গ্রন্থি* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(এতত্->m_backp, এতত্, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_গ্রন্থি* ধ্রুব backp = এতত্->m_backp;
    Ast_গ্রন্থি* ধ্রুব newLastp = newp->m_headtailp;
    //
    এতত্->edit_গুনতি_Inc();
    // Common linkage
    newLastp->m_nextp = এতত্;
    এতত্->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    যদি (backp && backp->m_nextp == এতত্) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } অন্যথা {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        যদি (backp) {
            যদি (backp->m_op1p == এতত্) {
                backp->m_op1p = newp;
            } অন্যথা যদি (backp->m_op2p == এতত্) {
                backp->m_op2p = newp;
            } অন্যথা যদি (backp->m_op3p == এতত্) {
                backp->m_op3p = newp;
            } অন্যথা {
                UASSERT_OBJ(backp->m_op4p == এতত্, এতত্, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_গ্রন্থি* ধ্রুব tailp = এতত্->m_headtailp;
        এতত্->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    যদি (newLastp->m_iterpp) *(newLastp->m_iterpp) = এতত্;
    যদি (এতত্->m_iterpp) {
        *(এতত্->m_iterpp) = newp;
        এতত্->m_iterpp = nullptr;
    }
    //
    debug_বৃক্ষ_Change(এতত্, "-addHereThisAsNext: ", __LINE__, সত্য);
}

//======================================================================
// Clone

Ast_গ্রন্থি* Ast_গ্রন্থি::clone_বৃক্ষ_Iter(তার্কিক needPure) {
    // private: Clone single node and children
    যদি (needPure) purityCheck();
    Ast_গ্রন্থি* ধ্রুব newp = এতত্->clone();
    যদি (এতত্->m_op1p) newp->op1p(এতত্->m_op1p->clone_বৃক্ষ_Iter_List(needPure));
    যদি (এতত্->m_op2p) newp->op2p(এতত্->m_op2p->clone_বৃক্ষ_Iter_List(needPure));
    যদি (এতত্->m_op3p) newp->op3p(এতত্->m_op3p->clone_বৃক্ষ_Iter_List(needPure));
    যদি (এতত্->m_op4p) newp->op4p(এতত্->m_op4p->clone_বৃক্ষ_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(এতত্);  // Save pointers to/from both to simplify relinking.
    এতত্->clonep(newp);  // Save pointers to/from both to simplify relinking.
    প্রত্যাগম newp;
}

Ast_গ্রন্থি* Ast_গ্রন্থি::clone_বৃক্ষ_Iter_List(তার্কিক needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_গ্রন্থি* newheadp = nullptr;
    Ast_গ্রন্থি* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    প্রতি (Ast_গ্রন্থি* oldp = এতত্; oldp; oldp = oldp->m_nextp) {
        Ast_গ্রন্থি* ধ্রুব newp = oldp->clone_বৃক্ষ_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        যদি (newtailp) newtailp->m_nextp = newp;
        যদি (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    প্রত্যাগম newheadp;
}

Ast_গ্রন্থি* Ast_গ্রন্থি::clone_বৃক্ষ(তার্কিক cloneNextLink, তার্কিক needPure) {
    debug_বৃক্ষ_Change(এতত্, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_বৃক্ষ();
    Ast_গ্রন্থি* newp;
    যদি (cloneNextLink && এতত্->m_nextp) {
        newp = clone_বৃক্ষ_Iter_List(needPure);
    } অন্যথা {
        newp = clone_বৃক্ষ_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_বৃক্ষ();
    debug_বৃক্ষ_Change(newp, "-cloneOut: ", __LINE__, সত্য);
    প্রত্যাগম newp;
}

শূন্য Ast_গ্রন্থি::purityCheck() {
    যদি (VL_UNLIKELY(!isPure())) {
        এতত্->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << এতত্->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

শূন্য Ast_গ্রন্থি::delete_গ্রন্থি() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_গুনতি_Inc();
    // Change links of old node so we coredump if used
    এতত্->m_nextp = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এতত্->m_backp = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এতত্->m_headtailp = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এতত্->m_op1p = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এতত্->m_op2p = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এতত্->m_op3p = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এতত্->m_op4p = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এতত্->m_iterpp = reinterpret_cast<Ast_গ্রন্থি**>(0x1);
    যদি (
#যদি !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        সত্য
#অন্যথা
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete এতত্;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

শূন্য Ast_গ্রন্থি::delete_বৃক্ষ_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    প্রতি (Ast_গ্রন্থি *nodep = এতত্, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        যদি (nodep->m_op1p) nodep->m_op1p->delete_বৃক্ষ_Iter();
        যদি (nodep->m_op2p) nodep->m_op2p->delete_বৃক্ষ_Iter();
        যদি (nodep->m_op3p) nodep->m_op3p->delete_বৃক্ষ_Iter();
        যদি (nodep->m_op4p) nodep->m_op4p->delete_বৃক্ষ_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_গ্রন্থি();
    }
}

শূন্য Ast_গ্রন্থি::delete_বৃক্ষ() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_বৃক্ষ_Change(এতত্, "-delTree:  ", __LINE__, সত্য);
    এতত্->edit_গুনতি_Inc();
    // MUST be depth first!
    delete_বৃক্ষ_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
শূন্য* Ast_গ্রন্থি::operator নূতন(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_গ্রন্থি* ধ্রুব objp = static_cast<Ast_গ্রন্থি*>(::operator নূতন(size));
    V3Broken::addNewed(objp);
    প্রত্যাগম objp;
}

শূন্য Ast_গ্রন্থি::operator delete(শূন্য* objp, size_t size) {
    যদি (!objp) প্রত্যাগম;
    ধ্রুব Ast_গ্রন্থি* ধ্রুব nodep = static_cast<Ast_গ্রন্থি*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
শূন্য* Ast_গ্রন্থি::operator নূতন(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    স্থির ধ্রুব size_t ASTGEN_MAX_গ্রন্থি_SIZE = []() {
        size_t maxSize = 0;
        প্রতি (size_t t = 0; t < VNType::অঙ্ক_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        প্রত্যাগম maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    স্থির uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    যদি (পূর্ণাঙ্ক seed = v3Global.opt.debugAllocRandom()) {
        স্থির V3Mutex s_mutex;
        ধ্রুব V3LockGuard lock{s_mutex};
        স্থির std::array<শূন্য*, POOL_SIZE> s_গ্রন্থি_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_গ্রন্থি_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        যদি (!s_lfsr) {
            s_lfsr = seed;
            যদি (!s_lfsr) s_lfsr = ~s_lfsr;
            প্রতি (size_t i = 0; i < POOL_SIZE; ++i) {
                s_গ্রন্থি_Pool[i] = ::operator নূতন(ASTGEN_MAX_গ্রন্থি_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_গ্রন্থি_Pool), std::end(s_গ্রন্থি_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ধ্রুব size_t index = s_lfsr % POOL_SIZE;
        Ast_গ্রন্থি* ধ্রুব objp = static_cast<Ast_গ্রন্থি*>(s_গ্রন্থি_Pool[index]);
        s_গ্রন্থি_Pool[index] = ::operator নূতন(ASTGEN_MAX_গ্রন্থি_SIZE + 64);  // For later new()
        প্রত্যাগম objp;
    } অন্যথা {
        Ast_গ্রন্থি* ধ্রুব objp = static_cast<Ast_গ্রন্থি*>(::operator নূতন(size));
        প্রত্যাগম objp;
    }
}
শূন্য Ast_গ্রন্থি::operator delete(শূন্য* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (শূন্য)objp;
    (শূন্য)size;
}
#endif

//======================================================================
// Iterators

শূন্য Ast_গ্রন্থি::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    যদি (m_op1p) m_op1p->iterateAndNext(v);
    যদি (m_op2p) m_op2p->iterateAndNext(v);
    যদি (m_op3p) m_op3p->iterateAndNext(v);
    যদি (m_op4p) m_op4p->iterateAndNext(v);
}

শূন্য Ast_গ্রন্থি::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    যদি (m_op1p) m_op1p->iterateAndNextConst(v);
    যদি (m_op2p) m_op2p->iterateAndNextConst(v);
    যদি (m_op3p) m_op3p->iterateAndNextConst(v);
    যদি (m_op4p) m_op4p->iterateAndNextConst(v);
}

শূন্য Ast_গ্রন্থি::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_গ্রন্থি* nodep = এতত্;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    যদি (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    যাবত্ (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        যদি (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_গ্রন্থি* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        যদি (!niterp) প্রত্যাগম;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        যদি (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } অন্যথা {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

শূন্য Ast_গ্রন্থি::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_গ্রন্থি* nodep = এতত্;
    যাবত্ (nodep->m_nextp) nodep = nodep->m_nextp;
    যাবত্ (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        যদি (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } অন্যথা {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

শূন্য Ast_গ্রন্থি::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    যদি (m_op1p) m_op1p->iterateListBackwardsConst(v);
    যদি (m_op2p) m_op2p->iterateListBackwardsConst(v);
    যদি (m_op3p) m_op3p->iterateListBackwardsConst(v);
    যদি (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

শূন্য Ast_গ্রন্থি::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_গ্রন্থি* nodep = এতত্;
    কুরু {
        Ast_গ্রন্থি* ধ্রুব nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } যাবত্ (nodep);
}

Ast_গ্রন্থি* Ast_গ্রন্থি::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_গ্রন্থি* nodep = এতত্;  // Note "this" may point to bogus point later in this function
    যদি (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } অন্যথা যদি (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ধ্রুব tempp = নূতন AstBegin{nodep->fileline(), "[EditWrapper]", nodep, অসত্য};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_বৃক্ষ(), tempp);
    } অন্যথা {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_গ্রন্থি** nextnodepp = nullptr;
        যদি (এতত্->m_backp->m_op1p == এতত্) {
            nextnodepp = &(এতত্->m_backp->m_op1p);
        } অন্যথা যদি (এতত্->m_backp->m_op2p == এতত্) {
            nextnodepp = &(এতত্->m_backp->m_op2p);
        } অন্যথা যদি (এতত্->m_backp->m_op3p == এতত্) {
            nextnodepp = &(এতত্->m_backp->m_op3p);
        } অন্যথা যদি (এতত্->m_backp->m_op4p == এতত্) {
            nextnodepp = &(এতত্->m_backp->m_op4p);
        } অন্যথা যদি (এতত্->m_backp->m_nextp == এতত্) {
            nextnodepp = &(এতত্->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, এতত্, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    প্রত্যাগম nodep;
}

//======================================================================

শূন্য Ast_গ্রন্থি::clone_Relink_বৃক্ষ() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    প্রতি (Ast_গ্রন্থি* nodep = এতত্; nodep; nodep = nodep->m_nextp) {
        যদি (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        যদি (nodep->m_op1p) nodep->m_op1p->clone_Relink_বৃক্ষ();
        যদি (nodep->m_op2p) nodep->m_op2p->clone_Relink_বৃক্ষ();
        যদি (nodep->m_op3p) nodep->m_op3p->clone_Relink_বৃক্ষ();
        যদি (nodep->m_op4p) nodep->m_op4p->clone_Relink_বৃক্ষ();
    }
}

//======================================================================
// Comparison

তার্কিক Ast_গ্রন্থি::gate_বৃক্ষ_Iter() ধ্রুব {
    // private: Return true if the two trees are identical
    যদি (!isGateOptimizable()) প্রত্যাগম অসত্য;
    যদি (m_op1p && !m_op1p->gate_বৃক্ষ_Iter()) প্রত্যাগম অসত্য;
    যদি (m_op2p && !m_op2p->gate_বৃক্ষ_Iter()) প্রত্যাগম অসত্য;
    যদি (m_op3p && !m_op3p->gate_বৃক্ষ_Iter()) প্রত্যাগম অসত্য;
    যদি (m_op4p && !m_op4p->gate_বৃক্ষ_Iter()) প্রত্যাগম অসত্য;
    প্রত্যাগম সত্য;
}

তার্কিক Ast_গ্রন্থি::same_বৃক্ষ_Iter(ধ্রুব Ast_গ্রন্থি* node1p, ধ্রুব Ast_গ্রন্থি* node2p, তার্কিক ignNext,
                           তার্কিক gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    যাবত্ (সত্য) {
        যদি (!node1p && !node2p) প্রত্যাগম সত্য;
        যদি (!node1p || !node2p) প্রত্যাগম অসত্য;
        যদি (node1p->type() != node2p->type()) প্রত্যাগম অসত্য;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        যদি (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) প্রত্যাগম অসত্য;
        যদি (!node1p->same_গ্রন্থি(node2p) || (gateOnly && !node1p->isGateOptimizable())) প্রত্যাগম অসত্য;
        যদি (!same_বৃক্ষ_Iter(node1p->m_op1p, node2p->m_op1p, অসত্য, gateOnly)
            || !same_বৃক্ষ_Iter(node1p->m_op2p, node2p->m_op2p, অসত্য, gateOnly)
            || !same_বৃক্ষ_Iter(node1p->m_op3p, node2p->m_op3p, অসত্য, gateOnly)
            || !same_বৃক্ষ_Iter(node1p->m_op4p, node2p->m_op4p, অসত্য, gateOnly)) {
            প্রত্যাগম অসত্য;
        }
        যদি (ignNext) প্রত্যাগম সত্য;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

শূন্য Ast_গ্রন্থি::check_বৃক্ষ_Iter(ধ্রুব Ast_গ্রন্থি* prevBackp) ধ্রুব VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == এতত্->backp(), এতত্, "Back node inconsistent");
    ধ্রুব VNTypeInfo& typeInfo = VNType::typeInfo(এতত্->type());
    প্রতি (পূর্ণাঙ্ক i = 1; i <= 4; i++) {
        Ast_গ্রন্থি* nodep = nullptr;
        বিকল্প (i) {
        পক্ষ 1: nodep = op1p(); ভঙ্গ;
        পক্ষ 2: nodep = op2p(); ভঙ্গ;
        পক্ষ 3: nodep = op3p(); ভঙ্গ;
        পক্ষ 4: nodep = op4p(); ভঙ্গ;
        স্বভাব: এতত্->v3fatalSrc("Bad case"); ভঙ্গ;
        }
        // cppcheck-suppress danglingTempReference
        ধ্রুব বর্ণ* opName = typeInfo.m_opNamep[i - 1];
        বিকল্প (typeInfo.m_opType[i - 1]) {
        পক্ষ VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, এতত্, typeInfo.m_namep << " must not use " << opName << "()");
            ভঙ্গ;
        পক্ষ VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, এতত্,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), এতত্,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_বৃক্ষ_Iter(এতত্);
            ভঙ্গ;
        পক্ষ VNTypeInfo::OP_LIST:
            যদি (ধ্রুব Ast_গ্রন্থি* ধ্রুব headp = nodep) {
                // cppcheck-suppress shadowFunction
                ধ্রুব Ast_গ্রন্থি* backp = এতত্;
                ধ্রুব Ast_গ্রন্থি* tailp;
                ধ্রুব Ast_গ্রন্থি* opp = headp;
                কুরু {
                    opp->check_বৃক্ষ_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } যাবত্ (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            ভঙ্গ;
        পক্ষ VNTypeInfo::OP_OPTIONAL:
            যদি (nodep) {
                UASSERT_OBJ(!nodep->nextp(), এতত্,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_বৃক্ষ_Iter(এতত্);
            }
            ভঙ্গ;
        স্বভাব: এতত্->v3fatalSrc("Bad case"); ভঙ্গ;
        }
    }
    যদি (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        যদি (ধ্রুব Ast_গ্রন্থি_রাশিমালা* ধ্রুব exprp = VN_CAST(এতত্, গ্রন্থি_রাশিমালা)) {
            ধ্রুব বর্ণ* ধ্রুব whyp = exprp->widthMismatch();
            যদি (whyp) {
                auto dtypeStr = [](ধ্রুব Ast_গ্রন্থি_রাশিমালা* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    প্রত্যাগম ss.str();
                };
                যদি (ধ্রুব Ast_গ্রন্থি_Uniop* ধ্রুব uniopp = VN_CAST(exprp, গ্রন্থি_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } অন্যথা যদি (ধ্রুব Ast_গ্রন্থি_Biop* ধ্রুব biopp = VN_CAST(exprp, গ্রন্থি_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } অন্যথা {
                    UASSERT_OBJ(অসত্য, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
বর্ণ* Ast_গ্রন্থি::dump_বৃক্ষ_Json_Gdb(ধ্রুব Ast_গ্রন্থি* nodep) {
    যদি (!nodep) প্রত্যাগম strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_বৃক্ষ_Json(nodepStream);
    ধ্রুব std::string str = nodepStream.rdbuf()->str();
    প্রত্যাগম strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
বর্ণ* Ast_গ্রন্থি::dump_বৃক্ষ_Json_Gdb(ধ্রুব বর্ণ* str) { প্রত্যাগম strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
বর্ণ* Ast_গ্রন্থি::dump_বৃক্ষ_Json_Gdb(intptr_t nodep) {
    যদি (!nodep) প্রত্যাগম strdup("{\"addr\":\"NULL\"}\n");
    প্রত্যাগম dump_বৃক্ষ_Json_Gdb(reinterpret_cast<ধ্রুব Ast_গ্রন্থি*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::dumpGdb(ধ্রুব Ast_গ্রন্থি* nodep) {  // For GDB only  // LCOV_EXCL_START
    যদি (!nodep) {
        cout << "<nullptr>" << endl;
        প্রত্যাগম;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::dumpGdbHeader() ধ্রুব {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_Gdb(ধ্রুব Ast_গ্রন্থি* nodep) {  // For GDB only  // LCOV_EXCL_START
    যদি (!nodep) {
        cout << "<nullptr>" << endl;
        প্রত্যাগম;
    }
    nodep->dumpGdbHeader();
    nodep->dump_বৃক্ষ(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_File_Gdb(ধ্রুব Ast_গ্রন্থি* nodep,  // LCOV_EXCL_START
                              ধ্রুব বর্ণ* filenamep) {  // For GDB only
    যদি (!nodep) {
        cout << "<nullptr>" << endl;
        প্রত্যাগম;
    }
    ধ্রুব string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_বৃক্ষ_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::checkIter() ধ্রুব {
    যদি (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        এতত্->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

শূন্য Ast_গ্রন্থি::dumpPtrs(std::ostream& os) ধ্রুব {
    os << "This=" << typeName() << " " << cvtToHex(এতত্);
    os << " back=" << cvtToHex(backp());
    যদি (nextp()) os << " next=" << cvtToHex(nextp());
    যদি (m_headtailp == এতত্) {
        os << " headtail=this";
    } অন্যথা {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    যদি (op1p()) os << " op1p=" << cvtToHex(op1p());
    যদি (op2p()) os << " op2p=" << cvtToHex(op2p());
    যদি (op3p()) os << " op3p=" << cvtToHex(op3p());
    যদি (op4p()) os << " op4p=" << cvtToHex(op4p());
    ধ্রুব auto dumpUser = [&os](ধ্রুব বর্ণ* prefix, ধ্রুব VNUser& user) {
        ধ্রুব std::string s
            = user.dumpStr([](ধ্রুব শূন্য* p) -> std::string { প্রত্যাগম cvtToHex(p); });
        যদি (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    যদি (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ(std::ostream& os, ধ্রুব string& indent, পূর্ণাঙ্ক maxDepth) ধ্রুব {
    স্থির পূর্ণাঙ্ক s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << এতত্ << '\n';
    যদি (VN_DELETED(এতত্)) প্রত্যাগম;
    যদি (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    যদি (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(এতত্)
           << endl;  // endl intentional to do flush
        প্রত্যাগম;
    }  // LCOV_EXCL_STOP
    যদি (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    যদি (maxDepth == 1) {
        যদি (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } অন্যথা {
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ(os, indent + "1:", maxDepth - 1);
        }
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ(os, indent + "2:", maxDepth - 1);
        }
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ(os, indent + "3:", maxDepth - 1);
        }
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ(os, indent + "4:", maxDepth - 1);
        }
    }
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_And_Next(std::ostream& os, ধ্রুব string& indent, পূর্ণাঙ্ক maxDepth) ধ্রুব {
    // Audited to make sure this is never nullptr
    প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = এতত্; nodep; nodep = nodep->nextp()) {
        nodep->dump_বৃক্ষ(os, indent, maxDepth);
    }
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_File(ধ্রুব string& filename, তার্কিক doDump) {
    // Not const function as calls checkTree
    যদি (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ধ্রুব std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            যদি (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_গুনতি_Last();
            *logsp << "> to <e" << std::dec << edit_গুনতি_Gbl() << ">\n";
            যদি (edit_গুনতি_Gbl() == edit_গুনতি_Last() && ::dump_বৃক্ষ_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } অন্যথা {
                dump_বৃক্ষ(*logsp);
                edit_গুনতি_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

স্থির শূন্য drawChildren(std::ostream& os, ধ্রুব Ast_গ্রন্থি* thisp, ধ্রুব Ast_গ্রন্থি* childp,
                         ধ্রুব std::string& childName) {
    যদি (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ_Dot(os);
            যদি (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_Dot(std::ostream& os) ধ্রুব {
    os << "\tn" << cvtToHex(এতত্) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, এতত্, m_op1p, "op1");
    drawChildren(os, এতত্, m_op2p, "op2");
    drawChildren(os, এতত্, m_op3p, "op3");
    drawChildren(os, এতত্, m_op4p, "op4");
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_Json_File(ধ্রুব string& filename, তার্কিক doDump) {
    যদি (!doDump) প্রত্যাগম;
    UINFO(2, "Dumping " << filename);
    ধ্রুব std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    যদি (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_বৃক্ষ_Json(*treejsonp);
    *treejsonp << '\n';
}

শূন্য Ast_গ্রন্থি::dumpJsonMetaFileGdb(ধ্রুব বর্ণ* filename) { dumpJsonMetaFile(filename); }
শূন্য Ast_গ্রন্থি::dumpJsonMetaFile(ধ্রুব string& filename) {
    UINFO(2, "Dumping " << filename);
    ধ্রুব std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    যদি (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_সারি::file_Name_অঙ্ক_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_Dot_File(ধ্রুব string& filename, তার্কিক doDump) {
    যদি (doDump) {
        UINFO(2, "Dumping " << filename);
        ধ্রুব std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        যদি (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_বৃক্ষ_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_গ্রন্থি::instanceStr() ধ্রুব {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_গুনতি = 0;
    // Walk 'backp' chain
    প্রতি (ধ্রুব Ast_গ্রন্থি* currp = এতত্; currp; currp = currp->backp(), ++iter_গুনতি) {
        যদি (VL_UNCOVERABLE(iter_গুনতি >= maxIterations)) প্রত্যাগম "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        যদি (ধ্রুব AstScope* ধ্রুব scopep = VN_CAST(currp, Scope)) {
            প্রত্যাগম scopep->is_শীর্ষ() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        যদি (ধ্রুব AstModule* ধ্রুব modp = VN_CAST(currp, Module)) {
            ধ্রুব string instanceName = modp->someInstanceName();
            প্রত্যাগম instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    প্রত্যাগম "";
}
শূন্য Ast_গ্রন্থি::v3errorEnd(ধ্রুব std::ostringstream& str) ধ্রুব VL_RELEASE(V3Error::s().m_mutex) {
    যদি (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } অন্যথা {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ধ্রুব string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        যদি (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_গ্রন্থি*>(এতত্)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
শূন্য Ast_গ্রন্থি::v3errorEndFatal(ধ্রুব std::ostringstream& str) ধ্রুব
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    প্রতিজ্ঞা(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

শূন্য Ast_গ্রন্থি::dtypeChgSigned(তার্কিক flag) {
    UASSERT_OBJ(dtypep(), এতত্, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
শূন্য Ast_গ্রন্থি::dtypeChgWidth(পূর্ণাঙ্ক width, পূর্ণাঙ্ক widthMin) {
    UASSERT_OBJ(dtypep(), এতত্, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

শূন্য Ast_গ্রন্থি::dtypeChgWidthSigned(পূর্ণাঙ্ক width, পূর্ণাঙ্ক widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), এতত্, "No dtype when changing width");
    ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব enumDTypep = dtypep()->skipRefToEnump();
    যদি (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        প্রত্যাগম;  // Correct already
    }
    ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findBasicDType(VBasicDTypeKwd kwd) ধ্রুব {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    প্রত্যাগম v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findBitDType(পূর্ণাঙ্ক width, পূর্ণাঙ্ক widthMin, VSigning numeric) ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findLogicDType(পূর্ণাঙ্ক width, পূর্ণাঙ্ক widthMin, VSigning numeric) ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findLogic2StateDType(পূর্ণাঙ্ক width, পূর্ণাঙ্ক widthMin, VSigning numeric) ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findLogicRangeDType(ধ্রুব VNumRange& range, পূর্ণাঙ্ক widthMin,
                                           VSigning numeric) ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findBitRangeDType(ধ্রুব VNumRange& range, পূর্ণাঙ্ক widthMin,
                                         VSigning numeric) ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_গ্রন্থি::find_ঢোকাও_Same_DType(AstBasicDType* nodep) {
    প্রত্যাগম v3Global.rootp()->typeTablep()->find_ঢোকাও_Same_DType(nodep);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findConstraintRefDType() ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findEmptyQueueDType() ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findQueueIndexDType() ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findStreamDType() ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findVoidDType() ধ্রুব {
    প্রত্যাগম v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

স্থির ধ্রুব Ast_গ্রন্থি_DType* computeCastableBase(ধ্রুব Ast_গ্রন্থি_DType* nodep) {
    যাবত্ (সত্য) {
        যদি (ধ্রুব AstPackArrayDType* ধ্রুব packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            অনুবর্ত;
        }
        যদি (ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব refp = nodep->skipRefToEnump()) {
            যদি (refp != nodep) {
                nodep = refp;
                অনুবর্ত;
            }
        }
        প্রত্যাগম nodep;
    }
}

স্থির VCastable computeCastableImp(ধ্রুব Ast_গ্রন্থি_DType* toDtp, ধ্রুব Ast_গ্রন্থি_DType* fromDtp,
                                    ধ্রুব Ast_গ্রন্থি* fromConstp, ধ্রুব তার্কিক checkIfaceArgCompat) {
    ধ্রুব VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    যদি (toDtp == fromDtp) প্রত্যাগম VCastable::SAMEISH;
    যদি (toDtp->similarDType(fromDtp)) প্রত্যাগম VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ধ্রুব Ast_গ্রন্থি_DType* fromBaseDtp = computeCastableBase(fromDtp);

    ধ্রুব তার্কিক fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, গ্রন্থি_UOr_Struct_DType);

    ধ্রুব Ast_গ্রন্থি_DType* toBaseDtp = computeCastableBase(toDtp);
    ধ্রুব তার্কিক toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, গ্রন্থি_UOr_Struct_DType);

    যদি (toBaseDtp == fromBaseDtp) প্রত্যাগম VCastable::COMPATIBLE;

    যদি (toNumericable) {
        যদি (fromNumericable) প্রত্যাগম VCastable::COMPATIBLE;
    } অন্যথা যদি (VN_IS(toBaseDtp, EnumDType)) {
        যদি (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_বৃক্ষ(fromDtp))
            প্রত্যাগম VCastable::ENUM_IMPLICIT;
        যদি (fromNumericable) প্রত্যাগম VCastable::ENUM_EXPLICIT;
    } অন্যথা যদি (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        প্রত্যাগম VCastable::COMPATIBLE;
    } অন্যথা যদি (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        যদি (fromConstp->isNull()) প্রত্যাগম VCastable::COMPATIBLE;
    } অন্যথা যদি (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ধ্রুব AstClass* ধ্রুব toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        ধ্রুব AstClass* ধ্রুব fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ধ্রুব তার্কিক downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        ধ্রুব তার্কিক upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        যদি (upcast) প্রত্যাগম VCastable::COMPATIBLE;
        যদি (downcast) প্রত্যাগম VCastable::DYNAMIC_CLASS;
        প্রত্যাগম VCastable::INCOMPATIBLE;
    } অন্যথা যদি (ধ্রুব AstIfaceRefDType* ধ্রুব toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(সত্য) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ধ্রুব AstIfaceRefDType* ধ্রুব fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(সত্য) : fromDtp, IfaceRefDType);
        যদি (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            ধ্রুব AstUnpackArrayDType* ধ্রুব toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            ধ্রুব AstUnpackArrayDType* ধ্রুব fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            যদি (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                প্রত্যাগম VCastable::INCOMPATIBLE;
            }
            প্রত্যাগম computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        যদি (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            যদি (!checkIfaceArgCompat) প্রত্যাগম castable;
            প্রত্যাগম VCastable::INCOMPATIBLE;
        }
        ধ্রুব তার্কিক sameModport = toIfp->modportp() == fromIfp->modportp();
        যদি (!checkIfaceArgCompat) {
            যদি (!toIfp->modportp() || sameModport) প্রত্যাগম VCastable::COMPATIBLE;
            প্রত্যাগম castable;
        }
        যদি (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            প্রত্যাগম VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        যদি (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            প্রত্যাগম VCastable::COMPATIBLE;
        }
        প্রত্যাগম VCastable::INCOMPATIBLE;
    } অন্যথা যদি (ধ্রুব AstUnpackArrayDType* ধ্রুব toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ধ্রুব AstUnpackArrayDType* ধ্রুব fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        যদি (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            প্রত্যাগম castable;
        }
        ধ্রুব VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব toSubp = toArrayp->subDTypep()->skipRefp();
        যদি (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            প্রত্যাগম VCastable::COMPATIBLE;
        }
    }
    প্রত্যাগম castable;
}

VCastable Ast_গ্রন্থি::computeCastable(ধ্রুব Ast_গ্রন্থি_DType* toDtp, ধ্রুব Ast_গ্রন্থি_DType* fromDtp,
                                   ধ্রুব Ast_গ্রন্থি* fromConstp, ধ্রুব তার্কিক checkIfaceArgCompat) {
    ধ্রুব auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    যদি (fromConstp) UINFO(9, "     const= " << fromConstp);
    প্রত্যাগম castable;
}

Ast_গ্রন্থি_DType* Ast_গ্রন্থি::getCommonClassTypep(Ast_গ্রন্থি* node1p, Ast_গ্রন্থি* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    যদি (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        ধ্রুব VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        যদি (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            প্রত্যাগম node1p->dtypep();
        যদি (castable == VCastable::DYNAMIC_CLASS) প্রত্যাগম node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    যাবত্ (classDtypep1) {
        ধ্রুব VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        যদি (castable == VCastable::COMPATIBLE) প্রত্যাগম classDtypep1;
        AstClassExtends* ধ্রুব extendsp = classDtypep1->classp()->extendsp();
        যদি (!extendsp) ভঙ্গ;
        Ast_গ্রন্থি_DType* ধ্রুব edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    প্রত্যাগম nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

বর্গ VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<পূর্ণাঙ্ক, std::string> m_internedWordWidths;  // Interned widths
    std::map<পূর্ণাঙ্ক, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ধ্রুব তার্কিক m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    স্থির std::string toLetters(size_t মান, তার্কিক lowerCase = অসত্য) {
        ধ্রুব বর্ণ base = lowerCase ? 'a' : 'A';
        std::string s;
        কুরু { s += static_cast<বর্ণ>(base + মান % 26); } যাবত্ (মান /= 26);
        প্রত্যাগম s;
    }

    ধ্রুব std::string& internConst(ধ্রুব AstConst& nodep) {
        ধ্রুব auto pair = m_internedConsts.emplace(nodep.অঙ্ক().ascii(অসত্য), "");
        যদি (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        প্রত্যাগম pair.first->second;
    }

    ধ্রুব std::string& internWordWidth(পূর্ণাঙ্ক মান) {
        ধ্রুব auto pair = m_internedWordWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, সত্য);
        প্রত্যাগম pair.first->second;
    }

    ধ্রুব std::string& internWideWidth(পূর্ণাঙ্ক মান) {
        ধ্রুব auto pair = m_internedWideWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        প্রত্যাগম pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    স্থির তার্কিক isLeaf(ধ্রুব Ast_গ্রন্থি* nodep) {
        ধ্রুব VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        প্রতি (ধ্রুব VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            যদি (opType != VNTypeInfo::OP_UNUSED) প্রত্যাগম অসত্য;
        }
        প্রত্যাগম সত্য;
    }

    // Render operand, return true if not unused
    শূন্য renderOperand(VNTypeInfo::OpEn opType, ধ্রুব Ast_গ্রন্থি* ধ্রুব opp, uint32_t depth) {
        বিকল্প (opType) {
        পক্ষ VNTypeInfo::OP_UNUSED:  //
            ভঙ্গ;
        পক্ষ VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            ভঙ্গ;
        পক্ষ VNTypeInfo::OP_LIST:
            m_os << " [";
            প্রতি (ধ্রুব Ast_গ্রন্থি* nodep = opp; nodep; nodep = nodep->nextp()) {
                যদি (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            ভঙ্গ;
        পক্ষ VNTypeInfo::OP_OPTIONAL:
            যদি (opp) {
                m_os << " ";
                render(opp, depth);
            } অন্যথা {
                m_os << " nil";
            }
            ভঙ্গ;
        }
    }

    // Render the node into the stream.
    শূন্য render(ধ্রুব Ast_গ্রন্থি* nodep, uint32_t depth) {
        যদি (ধ্রুব AstConst* ধ্রুব constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            যদি (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } অন্যথা যদি (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } অন্যথা {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } অন্যথা যদি (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } অন্যথা যদি (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } অন্যথা {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ধ্রুব VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ধ্রুব Ast_গ্রন্থি_DType* ধ্রুব dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        যদি (!dtypep) {
            m_os << '?';
        } অন্যথা যদি (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } অন্যথা {
            ধ্রুব পূর্ণাঙ্ক width = nodep->width();
            যদি (width == 1) {
                m_os << '1';
            } অন্যথা যদি (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } অন্যথা {
                m_os << internWideWidth(width);
            }
            যদি (m_dumpWidthMin) {
                m_os << '/';
                ধ্রুব পূর্ণাঙ্ক widthMin = nodep->widthMin();
                যদি (widthMin == 1) {
                    m_os << '1';
                } অন্যথা যদি (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } অন্যথা {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

সার্বজনিক:
    VNPatternString(std::ostream& os, ধ্রুব Ast_গ্রন্থি* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_গ্রন্থি_রাশিমালা::patternString(uint32_t depth) ধ্রুব {
    std::ostringstream oss;
    VNPatternString{oss, এতত্, depth};
    প্রত্যাগম oss.str();
}

//######################################################################
// VNDeleter

শূন্য VNDeleter::doDeletes() {
    প্রতি (Ast_গ্রন্থি* ধ্রুব nodep : m_deleteps) nodep->delete_বৃক্ষ();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#অন্তর্ভাব "V3Ast__gen_visitor_defns.h"  // From ./astgen
