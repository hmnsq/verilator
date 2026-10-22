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
পূর্ণসংখ্যা Ast_গ্রন্থি::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

যৌক্তিক VNUser1InUse::s_userBusy = মিথ্যা;
যৌক্তিক VNUser2InUse::s_userBusy = মিথ্যা;
যৌক্তিক VNUser3InUse::s_userBusy = মিথ্যা;
যৌক্তিক VNUser4InUse::s_userBusy = মিথ্যা;

পূর্ণসংখ্যা Ast_গ্রন্থি_DType::s_unique_অঙ্ক = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(ধ্রুবক string& name) {
    জন্য (পূর্ণসংখ্যা i = 0; i < _ENUM_MAX; ++i) {
        ধ্রুবক VCMethod method{i};
        যদি (name == method.ascii()) ফেরত method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    ফেরত VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ধ্রুবক শূন্য*)) ধ্রুবক {
#ifdef VL_USER_TYPE_CHECKS
    যদি (ধ্রুবক uint64_t* ধ্রুবক uip = std::get_if<uint64_t>(&m_u)) ফেরত "#"s + cvtToStr(*uip);
    যদি (শূন্য* ধ্রুবক* ধ্রুবক upp = std::get_if<শূন্য*>(&m_u)) ফেরত fmtAddrp(*upp);
    ফেরত "";
#অন্যথা
    // Dumps void* representation
    যদি (!m_u.up) ফেরত "";
    ফেরত fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

ধ্রুবক VNTypeInfo VNType::s_typeInfoTable[VNType::অঙ্ক_TYPES()] = {
#অন্তর্ভাব "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

যৌক্তিক VFwdType::is_গ্রন্থি_Compatible(ধ্রুবক Ast_গ্রন্থি* nodep) ধ্রুবক {
    ধ্রুবক Ast_গ্রন্থি* defp = nodep;
    যদি (ধ্রুবক AstTypedef* ধ্রুবক adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    যদি (ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক adefp = VN_CAST(defp, গ্রন্থি_DType))
        defp = adefp->skipRefToNonRefp();
    নির্বাচন (m_e) {
    ক্ষেত্র VFwdType::NONE: ফেরত সত্য; বিরতি;
    ক্ষেত্র VFwdType::ENUM: ফেরত VN_IS(defp, EnumDType); বিরতি;
    ক্ষেত্র VFwdType::STRUCT: ফেরত VN_IS(defp, StructDType); বিরতি;
    ক্ষেত্র VFwdType::UNION: ফেরত VN_IS(defp, UnionDType); বিরতি;
    ক্ষেত্র VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    ক্ষেত্র VFwdType::CLASS: ফেরত VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); বিরতি;
    ক্ষেত্র VFwdType::GENERIC_INTERFACE: ফেরত VN_IS(defp, IfaceRefDType); বিরতি;
    পূর্বনির্ধারিত: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    ফেরত মিথ্যা;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ধ্রুবক std::shared_ptr<ধ্রুবক string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
ধ্রুবক std::shared_ptr<ধ্রুবক string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(যৌক্তিক useSelfForThis, ধ্রুবক string& text) {
    ফেরত useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(যৌক্তিক useSelfForThis, যৌক্তিক protect) ধ্রুবক {
    ফেরত VIdProtect::protect_শব্দগুলি_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_গ্রন্থি::Ast_গ্রন্থি(VNType t, File_সারি* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = এই;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = মিথ্যা;
    m_flags.doingWidth = মিথ্যা;
    m_flags.protect = সত্য;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_গুনতি_Inc();
}

Ast_গ্রন্থি* Ast_গ্রন্থি::abovep() ধ্রুবক {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), এই, "abovep() not allowed when in midlist");
    ধ্রুবক Ast_গ্রন্থি* ধ্রুবক firstp = firstAbovep() ? এই : m_headtailp;
    ফেরত firstp->backp();
}
Ast_গ্রন্থি* Ast_গ্রন্থি::aboveLoopp() ধ্রুবক {
    // Returns parent node. Avoid using this, may have performance issues.
    ধ্রুবক Ast_গ্রন্থি* nodep = এই;
    // Backwards over peers (versus parents)
    যখন (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    ফেরত nodep->backp();
}

string Ast_গ্রন্থি::encodeName(ধ্রুবক string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.আকার_2());
    জন্য (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        যদি ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } অন্যথা যদি (pos[0] == '_') {
            out += pos[0];
            যদি (pos + 1 == namein.end()) বিরতি;
            যদি (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } অন্যথা {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ধ্রুবক unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    ফেরত vname.hashedName();
}

string Ast_গ্রন্থি::encode_সংখ্যা(int64_t অঙ্ক) {
    যদি (অঙ্ক < 0) ফেরত "__02D" + cvtToStr(-অঙ্ক);  // 2D=-
    ফেরত cvtToStr(অঙ্ক);
}

string Ast_গ্রন্থি::nameProtect() ধ্রুবক { ফেরত VIdProtect::protectIf(name(), protect()); }
string Ast_গ্রন্থি::origNameProtect() ধ্রুবক { ফেরত VIdProtect::protectIf(origName(), protect()); }

string Ast_গ্রন্থি::shortName() ধ্রুবক {
    string pretty = name();
    string::আকার_type pos;
    যখন ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    ফেরত pretty;
}

string Ast_গ্রন্থি::dedotName(ধ্রুবক string& namein) {
    string pretty = namein;
    string::আকার_type pos;
    যখন ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    যদি (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ফেরত pretty;
}

string Ast_গ্রন্থি::nameNoArray(ধ্রুবক string& namein) {
    ফেরত namein.substr(0, namein.find("__BRA__"));
}

string Ast_গ্রন্থি::vcdName(ধ্রুবক string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::আকার_type pos;
    যখন ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    যখন ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    ফেরত prettyName(pretty);
}

string Ast_গ্রন্থি::prettyName(ধ্রুবক string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.দৈর্ঘ্য());
    জন্য (ধ্রুবক অক্ষর* pos = namein.c_str(); *pos;) {
        যদি (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            চালু;
        }
        যদি (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            যদি (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                চালু;
            }
            যদি (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                চালু;
            }
            যদি (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                চালু;
            }
            যদি (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                চালু;
            }
            যদি (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                চালু;
            }
            যদি (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                চালু;
            }
            যদি (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                অক্ষর মান = 0;
                মান += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                মান
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += মান;
                pos += 5;
                চালু;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    যদি (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    যদি (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ফেরত pretty;
}

string Ast_গ্রন্থি::vpiName(ধ্রুবক string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ধ্রুবক আকার_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.দৈর্ঘ্য());
    যৌক্তিক inEscapedIdent = মিথ্যা;
    পূর্ণসংখ্যা lastIdent = 0;

    জন্য (ধ্রুবক অক্ষর* pos = namein.c_str() + offset; *pos;) {
        অক্ষর specialChar = 0;
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
                চালু;
            } অন্যথা যদি (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                অক্ষর মান = 0;
                মান += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                মান
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                যদি (মান != '_') inEscapedIdent = সত্য;

                pretty += মান;
                pos += 5;
                চালু;
            }
        } অন্যথা যদি (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        যদি (specialChar) {
            যদি (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.ঢোকাও(lastIdent, "\\");
                inEscapedIdent = মিথ্যা;
            }

            pretty += specialChar;

            যদি (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.দৈর্ঘ্য();
                inEscapedIdent = মিথ্যা;
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
    ফেরত pretty;
}

string Ast_গ্রন্থি::prettyTypeName() ধ্রুবক {
    যদি (name() == "") ফেরত typeName();
    ফেরত std::string{typeName()} + " '" + pretty_Name_বার্তা() + "'";
}

//######################################################################
// Insertion

শূন্য Ast_গ্রন্থি::debug_বৃক্ষ_Change(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক অক্ষর* prefix, পূর্ণসংখ্যা lineno, যৌক্তিক পরবর্তী) {
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
Ast_গ্রন্থি* Ast_গ্রন্থি::add_পরবর্তী<Ast_গ্রন্থি, Ast_গ্রন্থি>(Ast_গ্রন্থি* nodep, Ast_গ্রন্থি* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_বৃক্ষ_Change(nodep, "-addNextThs: ", __LINE__, মিথ্যা);
    debug_বৃক্ষ_Change(newp, "-addNextNew: ", __LINE__, সত্য);
    যদি (!nodep)  // verilog.y and lots of other places assume this
        ফেরত newp;

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
            যখন (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_গ্রন্থি* ধ্রুবক newtailp = newp->m_headtailp;
    Ast_গ্রন্থি* ধ্রুবক headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_গুনতি_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_বৃক্ষ_Change(nodep, "-addNextOut:", __LINE__, সত্য);
    ফেরত nodep;
}

শূন্য Ast_গ্রন্থি::add_পরবর্তী_Here(Ast_গ্রন্থি* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_বৃক্ষ_Change(এই, "-addHereThs: ", __LINE__, মিথ্যা);
    debug_বৃক্ষ_Change(newp, "-addHereNew: ", __LINE__, সত্য);
    newp->edit_গুনতি_Inc();

    Ast_গ্রন্থি* ধ্রুবক addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_গ্রন্থি* ধ্রুবক oldnextp = এই->m_nextp;
    এই->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    যদি (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = এই;

    // Head/tail
    Ast_গ্রন্থি* ধ্রুবক oldheadtailp = এই->m_headtailp;
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
        যদি (oldheadtailp == এই) {  // this was one node
            এই->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } অন্যথা যদি (!oldnextp) {  // this was tail
            এই->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_বৃক্ষ_Change(এই, "-addHereOut: ", __LINE__, সত্য);
}

শূন্য Ast_গ্রন্থি::setOp1p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, এই, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_বৃক্ষ_Change(এই, "-setOp1pThs: ", __LINE__, মিথ্যা);
    debug_বৃক্ষ_Change(newp, "-setOp1pNew: ", __LINE__, সত্য);
    m_op1p = newp;
    newp->edit_গুনতি_Inc();
    newp->m_backp = এই;
    debug_বৃক্ষ_Change(এই, "-setOp1pOut: ", __LINE__, মিথ্যা);
}

শূন্য Ast_গ্রন্থি::setOp2p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, এই, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_বৃক্ষ_Change(এই, "-setOp2pThs: ", __LINE__, মিথ্যা);
    debug_বৃক্ষ_Change(newp, "-setOp2pNew: ", __LINE__, সত্য);
    m_op2p = newp;
    newp->edit_গুনতি_Inc();
    newp->m_backp = এই;
    debug_বৃক্ষ_Change(এই, "-setOp2pOut: ", __LINE__, মিথ্যা);
}

শূন্য Ast_গ্রন্থি::setOp3p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, এই, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_বৃক্ষ_Change(এই, "-setOp3pThs: ", __LINE__, মিথ্যা);
    debug_বৃক্ষ_Change(newp, "-setOp3pNew: ", __LINE__, সত্য);
    m_op3p = newp;
    newp->edit_গুনতি_Inc();
    newp->m_backp = এই;
    debug_বৃক্ষ_Change(এই, "-setOp3pOut: ", __LINE__, মিথ্যা);
}

শূন্য Ast_গ্রন্থি::setOp4p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, এই, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_বৃক্ষ_Change(এই, "-setOp4pThs: ", __LINE__, মিথ্যা);
    debug_বৃক্ষ_Change(newp, "-setOp4pNew: ", __LINE__, সত্য);
    m_op4p = newp;
    newp->edit_গুনতি_Inc();
    newp->m_backp = এই;
    debug_বৃক্ষ_Change(এই, "-setOp4pOut: ", __LINE__, মিথ্যা);
}

শূন্য Ast_গ্রন্থি::addOp1p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op1p) {
        op1p(newp);
    } অন্যথা {
        m_op1p->add_পরবর্তী(newp);
    }
}

শূন্য Ast_গ্রন্থি::addOp2p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op2p) {
        op2p(newp);
    } অন্যথা {
        m_op2p->add_পরবর্তী(newp);
    }
}

শূন্য Ast_গ্রন্থি::addOp3p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op3p) {
        op3p(newp);
    } অন্যথা {
        m_op3p->add_পরবর্তী(newp);
    }
}

শূন্য Ast_গ্রন্থি::addOp4p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op4p) {
        op4p(newp);
    } অন্যথা {
        m_op4p->add_পরবর্তী(newp);
    }
}

শূন্য Ast_গ্রন্থি::prependOp1p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op1p) {
        op1p(newp);
    } অন্যথা {
        m_op1p->add_Here_This_As_পরবর্তী(newp);
    }
}

শূন্য Ast_গ্রন্থি::prependOp2p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op2p) {
        op2p(newp);
    } অন্যথা {
        m_op2p->add_Here_This_As_পরবর্তী(newp);
    }
}

শূন্য Ast_গ্রন্থি::prependOp3p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op3p) {
        op3p(newp);
    } অন্যথা {
        m_op3p->add_Here_This_As_পরবর্তী(newp);
    }
}

শূন্য Ast_গ্রন্থি::prependOp4p(Ast_গ্রন্থি* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    যদি (!m_op4p) {
        op4p(newp);
    } অন্যথা {
        m_op4p->add_Here_This_As_পরবর্তী(newp);
    }
}

শূন্য Ast_গ্রন্থি::replaceWith(Ast_গ্রন্থি* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    এই->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
শূন্য Ast_গ্রন্থি::replaceWithKeepDType(Ast_গ্রন্থি* newp) {
    newp->dtypeFrom(এই);
    replaceWith(newp);
}

শূন্য VNRelinker::dump(std::ostream& str) ধ্রুবক {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_পরবর্তী ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_গ্রন্থি* Ast_গ্রন্থি::unlink_Fr_Back_With_পরবর্তী(VNRelinker* linkerp) {
    debug_বৃক্ষ_Change(এই, "-unlinkWNextThs: ", __LINE__, সত্য);
    Ast_গ্রন্থি* ধ্রুবক oldp = এই;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_গুনতি_Inc();
    // cppcheck-suppress shadowFunction
    Ast_গ্রন্থি* ধ্রুবক backp = oldp->m_backp;
    যদি (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        যদি (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_পরবর্তী;
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
            যখন (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_গ্রন্থি* ধ্রুবক oldheadp = oldtailp->m_headtailp;
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
        এই->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    যদি (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_বৃক্ষ_Change(oldp, "-unlinkWNextOut: ", __LINE__, সত্য);
    ফেরত oldp;
}

Ast_গ্রন্থি* Ast_গ্রন্থি::unlinkFrBack(VNRelinker* linkerp) {
    debug_বৃক্ষ_Change(এই, "-unlinkFrBkThs: ", __LINE__, সত্য);
    Ast_গ্রন্থি* ধ্রুবক oldp = এই;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_গুনতি_Inc();
    // cppcheck-suppress shadowFunction
    Ast_গ্রন্থি* ধ্রুবক backp = oldp->m_backp;
    যদি (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        যদি (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        যদি (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_পরবর্তী;
        } অন্যথা যদি (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } অন্যথা যদি (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } অন্যথা যদি (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } অন্যথা যদি (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } অন্যথা {
            এই->v3fatalSrc("Unlink of node with back not pointing to it.");
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
            এই->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        যদি (oldp->m_nextp) {
            Ast_গ্রন্থি* ধ্রুবক newheadp = oldp->m_nextp;
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
    ফেরত oldp;
}

শূন্য Ast_গ্রন্থি::relink(VNRelinker* linkerp) {
    যদি (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_গ্রন্থি* ধ্রুবক newp = এই;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_গুনতি_Inc();

    যদি (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_গ্রন্থি* ধ্রুবক backp = linkerp->m_backp;
    debug_বৃক্ষ_Change(এই, "-relinkNew: ", __LINE__, সত্য);
    debug_বৃক্ষ_Change(backp, "-relinkTre: ", __LINE__, সত্য);

    নির্বাচন (linkerp->m_chg) {
    ক্ষেত্র VNRelinker::RELINK_পরবর্তী: backp->add_পরবর্তী_Here(newp); বিরতি;
    ক্ষেত্র VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); বিরতি;
    ক্ষেত্র VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); বিরতি;
    ক্ষেত্র VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); বিরতি;
    ক্ষেত্র VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); বিরতি;
    পূর্বনির্ধারিত: এই->v3fatalSrc("Relink of node without any link to change."); বিরতি;
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
    debug_বৃক্ষ_Change(এই, "-relinkOut: ", __LINE__, সত্য);
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
        Ast_গ্রন্থি* ধ্রুবক newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_গ্রন্থি* ধ্রুবক oldlistlastp = pointpr->m_headtailp;
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

শূন্য Ast_গ্রন্থি::add_Here_This_As_পরবর্তী(Ast_গ্রন্থি* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(এই->m_backp, এই, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_গ্রন্থি* ধ্রুবক backp = এই->m_backp;
    Ast_গ্রন্থি* ধ্রুবক newLastp = newp->m_headtailp;
    //
    এই->edit_গুনতি_Inc();
    // Common linkage
    newLastp->m_nextp = এই;
    এই->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    যদি (backp && backp->m_nextp == এই) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } অন্যথা {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        যদি (backp) {
            যদি (backp->m_op1p == এই) {
                backp->m_op1p = newp;
            } অন্যথা যদি (backp->m_op2p == এই) {
                backp->m_op2p = newp;
            } অন্যথা যদি (backp->m_op3p == এই) {
                backp->m_op3p = newp;
            } অন্যথা {
                UASSERT_OBJ(backp->m_op4p == এই, এই, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_গ্রন্থি* ধ্রুবক tailp = এই->m_headtailp;
        এই->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    যদি (newLastp->m_iterpp) *(newLastp->m_iterpp) = এই;
    যদি (এই->m_iterpp) {
        *(এই->m_iterpp) = newp;
        এই->m_iterpp = nullptr;
    }
    //
    debug_বৃক্ষ_Change(এই, "-addHereThisAsNext: ", __LINE__, সত্য);
}

//======================================================================
// Clone

Ast_গ্রন্থি* Ast_গ্রন্থি::clone_বৃক্ষ_Iter(যৌক্তিক needPure) {
    // private: Clone single node and children
    যদি (needPure) purityCheck();
    Ast_গ্রন্থি* ধ্রুবক newp = এই->clone();
    যদি (এই->m_op1p) newp->op1p(এই->m_op1p->clone_বৃক্ষ_Iter_List(needPure));
    যদি (এই->m_op2p) newp->op2p(এই->m_op2p->clone_বৃক্ষ_Iter_List(needPure));
    যদি (এই->m_op3p) newp->op3p(এই->m_op3p->clone_বৃক্ষ_Iter_List(needPure));
    যদি (এই->m_op4p) newp->op4p(এই->m_op4p->clone_বৃক্ষ_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(এই);  // Save pointers to/from both to simplify relinking.
    এই->clonep(newp);  // Save pointers to/from both to simplify relinking.
    ফেরত newp;
}

Ast_গ্রন্থি* Ast_গ্রন্থি::clone_বৃক্ষ_Iter_List(যৌক্তিক needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_গ্রন্থি* newheadp = nullptr;
    Ast_গ্রন্থি* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    জন্য (Ast_গ্রন্থি* oldp = এই; oldp; oldp = oldp->m_nextp) {
        Ast_গ্রন্থি* ধ্রুবক newp = oldp->clone_বৃক্ষ_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        যদি (newtailp) newtailp->m_nextp = newp;
        যদি (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    ফেরত newheadp;
}

Ast_গ্রন্থি* Ast_গ্রন্থি::clone_বৃক্ষ(যৌক্তিক clone_পরবর্তী_Link, যৌক্তিক needPure) {
    debug_বৃক্ষ_Change(এই, "-cloneThs: ", __LINE__, clone_পরবর্তী_Link);
    clone_Clear_বৃক্ষ();
    Ast_গ্রন্থি* newp;
    যদি (clone_পরবর্তী_Link && এই->m_nextp) {
        newp = clone_বৃক্ষ_Iter_List(needPure);
    } অন্যথা {
        newp = clone_বৃক্ষ_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_বৃক্ষ();
    debug_বৃক্ষ_Change(newp, "-cloneOut: ", __LINE__, সত্য);
    ফেরত newp;
}

শূন্য Ast_গ্রন্থি::purityCheck() {
    যদি (VL_UNLIKELY(!isPure())) {
        এই->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << এই->warnMore()
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
    এই->m_nextp = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এই->m_backp = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এই->m_headtailp = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এই->m_op1p = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এই->m_op2p = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এই->m_op3p = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এই->m_op4p = reinterpret_cast<Ast_গ্রন্থি*>(0x1);
    এই->m_iterpp = reinterpret_cast<Ast_গ্রন্থি**>(0x1);
    যদি (
#যদি !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        সত্য
#অন্যথা
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete এই;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

শূন্য Ast_গ্রন্থি::delete_বৃক্ষ_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    জন্য (Ast_গ্রন্থি *nodep = এই, *nnextp; nodep; nodep = nnextp) {
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
    debug_বৃক্ষ_Change(এই, "-delTree:  ", __LINE__, সত্য);
    এই->edit_গুনতি_Inc();
    // MUST be depth first!
    delete_বৃক্ষ_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
শূন্য* Ast_গ্রন্থি::operator নতুন(আকার_t আকার_2) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_গ্রন্থি* ধ্রুবক objp = static_cast<Ast_গ্রন্থি*>(::operator নতুন(আকার_2));
    V3Broken::addNewed(objp);
    ফেরত objp;
}

শূন্য Ast_গ্রন্থি::operator delete(শূন্য* objp, আকার_t আকার_2) {
    যদি (!objp) ফেরত;
    ধ্রুবক Ast_গ্রন্থি* ধ্রুবক nodep = static_cast<Ast_গ্রন্থি*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
শূন্য* Ast_গ্রন্থি::operator নতুন(আকার_t আকার_2) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    স্থির ধ্রুবক আকার_t ASTGEN_MAX_গ্রন্থি_আকার = []() {
        আকার_t max_আকার = 0;
        জন্য (আকার_t t = 0; t < VNType::অঙ্ক_TYPES(); ++t) {
            max_আকার = std::max(max_আকার, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        ফেরত max_আকার;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr আকার_t POOL_আকার = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    স্থির uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    যদি (পূর্ণসংখ্যা seed = v3Global.opt.debugAllocRandom()) {
        স্থির V3Mutex s_mutex;
        ধ্রুবক V3LockGuard lock{s_mutex};
        স্থির std::array<শূন্য*, POOL_আকার> s_গ্রন্থি_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(আকার_2 <= ASTGEN_MAX_গ্রন্থি_আকার, "fix ASTGEN_MAX_NODE_SIZE");
        যদি (!s_lfsr) {
            s_lfsr = seed;
            যদি (!s_lfsr) s_lfsr = ~s_lfsr;
            জন্য (আকার_t i = 0; i < POOL_আকার; ++i) {
                s_গ্রন্থি_Pool[i] = ::operator নতুন(ASTGEN_MAX_গ্রন্থি_আকার + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_গ্রন্থি_Pool), std::end(s_গ্রন্থি_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ধ্রুবক আকার_t সূচক = s_lfsr % POOL_আকার;
        Ast_গ্রন্থি* ধ্রুবক objp = static_cast<Ast_গ্রন্থি*>(s_গ্রন্থি_Pool[সূচক]);
        s_গ্রন্থি_Pool[সূচক] = ::operator নতুন(ASTGEN_MAX_গ্রন্থি_আকার + 64);  // For later new()
        ফেরত objp;
    } অন্যথা {
        Ast_গ্রন্থি* ধ্রুবক objp = static_cast<Ast_গ্রন্থি*>(::operator নতুন(আকার_2));
        ফেরত objp;
    }
}
শূন্য Ast_গ্রন্থি::operator delete(শূন্য* objp, আকার_t আকার_2) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (শূন্য)objp;
    (শূন্য)আকার_2;
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
    যদি (m_op1p) m_op1p->iterate_And_পরবর্তী(v);
    যদি (m_op2p) m_op2p->iterate_And_পরবর্তী(v);
    যদি (m_op3p) m_op3p->iterate_And_পরবর্তী(v);
    যদি (m_op4p) m_op4p->iterate_And_পরবর্তী(v);
}

শূন্য Ast_গ্রন্থি::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    যদি (m_op1p) m_op1p->iterate_And_পরবর্তী_Const(v);
    যদি (m_op2p) m_op2p->iterate_And_পরবর্তী_Const(v);
    যদি (m_op3p) m_op3p->iterate_And_পরবর্তী_Const(v);
    যদি (m_op4p) m_op4p->iterate_And_পরবর্তী_Const(v);
}

শূন্য Ast_গ্রন্থি::iterate_And_পরবর্তী(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_গ্রন্থি* nodep = এই;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    যদি (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    যখন (nodep) {  // effectively: if (!this) return;  // Callers rely on this
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
        যদি (!niterp) ফেরত;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        যদি (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } অন্যথা {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

শূন্য Ast_গ্রন্থি::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_গ্রন্থি* nodep = এই;
    যখন (nodep->m_nextp) nodep = nodep->m_nextp;
    যখন (nodep) {
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

শূন্য Ast_গ্রন্থি::iterate_And_পরবর্তী_Const(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_গ্রন্থি* nodep = এই;
    করো {
        Ast_গ্রন্থি* ধ্রুবক nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } যখন (nodep);
}

Ast_গ্রন্থি* Ast_গ্রন্থি::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_গ্রন্থি* nodep = এই;  // Note "this" may point to bogus point later in this function
    যদি (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } অন্যথা যদি (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ধ্রুবক tempp = নতুন AstBegin{nodep->fileline(), "[EditWrapper]", nodep, মিথ্যা};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlink_Fr_Back_With_পরবর্তী();
        VL_DO_DANGLING(tempp->delete_বৃক্ষ(), tempp);
    } অন্যথা {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_গ্রন্থি** nextnodepp = nullptr;
        যদি (এই->m_backp->m_op1p == এই) {
            nextnodepp = &(এই->m_backp->m_op1p);
        } অন্যথা যদি (এই->m_backp->m_op2p == এই) {
            nextnodepp = &(এই->m_backp->m_op2p);
        } অন্যথা যদি (এই->m_backp->m_op3p == এই) {
            nextnodepp = &(এই->m_backp->m_op3p);
        } অন্যথা যদি (এই->m_backp->m_op4p == এই) {
            nextnodepp = &(এই->m_backp->m_op4p);
        } অন্যথা যদি (এই->m_backp->m_nextp == এই) {
            nextnodepp = &(এই->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, এই, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    ফেরত nodep;
}

//======================================================================

শূন্য Ast_গ্রন্থি::clone_Relink_বৃক্ষ() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    জন্য (Ast_গ্রন্থি* nodep = এই; nodep; nodep = nodep->m_nextp) {
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

যৌক্তিক Ast_গ্রন্থি::gate_বৃক্ষ_Iter() ধ্রুবক {
    // private: Return true if the two trees are identical
    যদি (!isGateOptimizable()) ফেরত মিথ্যা;
    যদি (m_op1p && !m_op1p->gate_বৃক্ষ_Iter()) ফেরত মিথ্যা;
    যদি (m_op2p && !m_op2p->gate_বৃক্ষ_Iter()) ফেরত মিথ্যা;
    যদি (m_op3p && !m_op3p->gate_বৃক্ষ_Iter()) ফেরত মিথ্যা;
    যদি (m_op4p && !m_op4p->gate_বৃক্ষ_Iter()) ফেরত মিথ্যা;
    ফেরত সত্য;
}

যৌক্তিক Ast_গ্রন্থি::same_বৃক্ষ_Iter(ধ্রুবক Ast_গ্রন্থি* node1p, ধ্রুবক Ast_গ্রন্থি* node2p, যৌক্তিক ign_পরবর্তী,
                           যৌক্তিক gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    যখন (সত্য) {
        যদি (!node1p && !node2p) ফেরত সত্য;
        যদি (!node1p || !node2p) ফেরত মিথ্যা;
        যদি (node1p->type() != node2p->type()) ফেরত মিথ্যা;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        যদি (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) ফেরত মিথ্যা;
        যদি (!node1p->same_গ্রন্থি(node2p) || (gateOnly && !node1p->isGateOptimizable())) ফেরত মিথ্যা;
        যদি (!same_বৃক্ষ_Iter(node1p->m_op1p, node2p->m_op1p, মিথ্যা, gateOnly)
            || !same_বৃক্ষ_Iter(node1p->m_op2p, node2p->m_op2p, মিথ্যা, gateOnly)
            || !same_বৃক্ষ_Iter(node1p->m_op3p, node2p->m_op3p, মিথ্যা, gateOnly)
            || !same_বৃক্ষ_Iter(node1p->m_op4p, node2p->m_op4p, মিথ্যা, gateOnly)) {
            ফেরত মিথ্যা;
        }
        যদি (ign_পরবর্তী) ফেরত সত্য;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

শূন্য Ast_গ্রন্থি::check_বৃক্ষ_Iter(ধ্রুবক Ast_গ্রন্থি* পূর্ব_Backp) ধ্রুবক VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(পূর্ব_Backp == এই->backp(), এই, "Back node inconsistent");
    ধ্রুবক VNTypeInfo& typeInfo = VNType::typeInfo(এই->type());
    জন্য (পূর্ণসংখ্যা i = 1; i <= 4; i++) {
        Ast_গ্রন্থি* nodep = nullptr;
        নির্বাচন (i) {
        ক্ষেত্র 1: nodep = op1p(); বিরতি;
        ক্ষেত্র 2: nodep = op2p(); বিরতি;
        ক্ষেত্র 3: nodep = op3p(); বিরতি;
        ক্ষেত্র 4: nodep = op4p(); বিরতি;
        পূর্বনির্ধারিত: এই->v3fatalSrc("Bad case"); বিরতি;
        }
        // cppcheck-suppress danglingTempReference
        ধ্রুবক অক্ষর* opName = typeInfo.m_opNamep[i - 1];
        নির্বাচন (typeInfo.m_opType[i - 1]) {
        ক্ষেত্র VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, এই, typeInfo.m_namep << " must not use " << opName << "()");
            বিরতি;
        ক্ষেত্র VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, এই,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), এই,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_বৃক্ষ_Iter(এই);
            বিরতি;
        ক্ষেত্র VNTypeInfo::OP_LIST:
            যদি (ধ্রুবক Ast_গ্রন্থি* ধ্রুবক headp = nodep) {
                // cppcheck-suppress shadowFunction
                ধ্রুবক Ast_গ্রন্থি* backp = এই;
                ধ্রুবক Ast_গ্রন্থি* tailp;
                ধ্রুবক Ast_গ্রন্থি* opp = headp;
                করো {
                    opp->check_বৃক্ষ_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } যখন (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            বিরতি;
        ক্ষেত্র VNTypeInfo::OP_OPTIONAL:
            যদি (nodep) {
                UASSERT_OBJ(!nodep->nextp(), এই,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_বৃক্ষ_Iter(এই);
            }
            বিরতি;
        পূর্বনির্ধারিত: এই->v3fatalSrc("Bad case"); বিরতি;
        }
    }
    যদি (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        যদি (ধ্রুবক Ast_গ্রন্থি_রাশিমালা* ধ্রুবক exprp = VN_CAST(এই, গ্রন্থি_রাশিমালা)) {
            ধ্রুবক অক্ষর* ধ্রুবক whyp = exprp->widthMismatch();
            যদি (whyp) {
                auto dtypeStr = [](ধ্রুবক Ast_গ্রন্থি_রাশিমালা* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    ফেরত ss.str();
                };
                যদি (ধ্রুবক Ast_গ্রন্থি_Uniop* ধ্রুবক uniopp = VN_CAST(exprp, গ্রন্থি_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } অন্যথা যদি (ধ্রুবক Ast_গ্রন্থি_Biop* ধ্রুবক biopp = VN_CAST(exprp, গ্রন্থি_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } অন্যথা {
                    UASSERT_OBJ(মিথ্যা, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
অক্ষর* Ast_গ্রন্থি::dump_বৃক্ষ_Json_Gdb(ধ্রুবক Ast_গ্রন্থি* nodep) {
    যদি (!nodep) ফেরত strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_বৃক্ষ_Json(nodepStream);
    ধ্রুবক std::string str = nodepStream.rdbuf()->str();
    ফেরত strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
অক্ষর* Ast_গ্রন্থি::dump_বৃক্ষ_Json_Gdb(ধ্রুবক অক্ষর* str) { ফেরত strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
অক্ষর* Ast_গ্রন্থি::dump_বৃক্ষ_Json_Gdb(intptr_t nodep) {
    যদি (!nodep) ফেরত strdup("{\"addr\":\"NULL\"}\n");
    ফেরত dump_বৃক্ষ_Json_Gdb(reinterpret_cast<ধ্রুবক Ast_গ্রন্থি*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::dumpGdb(ধ্রুবক Ast_গ্রন্থি* nodep) {  // For GDB only  // LCOV_EXCL_START
    যদি (!nodep) {
        cout << "<nullptr>" << endl;
        ফেরত;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::dumpGdbHeader() ধ্রুবক {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_Gdb(ধ্রুবক Ast_গ্রন্থি* nodep) {  // For GDB only  // LCOV_EXCL_START
    যদি (!nodep) {
        cout << "<nullptr>" << endl;
        ফেরত;
    }
    nodep->dumpGdbHeader();
    nodep->dump_বৃক্ষ(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_File_Gdb(ধ্রুবক Ast_গ্রন্থি* nodep,  // LCOV_EXCL_START
                              ধ্রুবক অক্ষর* filenamep) {  // For GDB only
    যদি (!nodep) {
        cout << "<nullptr>" << endl;
        ফেরত;
    }
    ধ্রুবক string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_বৃক্ষ_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
শূন্য Ast_গ্রন্থি::checkIter() ধ্রুবক {
    যদি (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        এই->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

শূন্য Ast_গ্রন্থি::dumpPtrs(std::ostream& os) ধ্রুবক {
    os << "This=" << typeName() << " " << cvtToHex(এই);
    os << " back=" << cvtToHex(backp());
    যদি (nextp()) os << " next=" << cvtToHex(nextp());
    যদি (m_headtailp == এই) {
        os << " headtail=this";
    } অন্যথা {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    যদি (op1p()) os << " op1p=" << cvtToHex(op1p());
    যদি (op2p()) os << " op2p=" << cvtToHex(op2p());
    যদি (op3p()) os << " op3p=" << cvtToHex(op3p());
    যদি (op4p()) os << " op4p=" << cvtToHex(op4p());
    ধ্রুবক auto dumpUser = [&os](ধ্রুবক অক্ষর* prefix, ধ্রুবক VNUser& user) {
        ধ্রুবক std::string s
            = user.dumpStr([](ধ্রুবক শূন্য* p) -> std::string { ফেরত cvtToHex(p); });
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

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ(std::ostream& os, ধ্রুবক string& indent, পূর্ণসংখ্যা maxDepth) ধ্রুবক {
    স্থির পূর্ণসংখ্যা s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << এই << '\n';
    যদি (VN_DELETED(এই)) ফেরত;
    যদি (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    যদি (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(এই)
           << endl;  // endl intentional to do flush
        ফেরত;
    }  // LCOV_EXCL_STOP
    যদি (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    যদি (maxDepth == 1) {
        যদি (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } অন্যথা {
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ(os, indent + "1:", maxDepth - 1);
        }
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ(os, indent + "2:", maxDepth - 1);
        }
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ(os, indent + "3:", maxDepth - 1);
        }
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_বৃক্ষ(os, indent + "4:", maxDepth - 1);
        }
    }
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_And_পরবর্তী(std::ostream& os, ধ্রুবক string& indent, পূর্ণসংখ্যা maxDepth) ধ্রুবক {
    // Audited to make sure this is never nullptr
    জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = এই; nodep; nodep = nodep->nextp()) {
        nodep->dump_বৃক্ষ(os, indent, maxDepth);
    }
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_File(ধ্রুবক string& filename, যৌক্তিক doDump) {
    // Not const function as calls checkTree
    যদি (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ধ্রুবক std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
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

স্থির শূন্য drawChildren(std::ostream& os, ধ্রুবক Ast_গ্রন্থি* thisp, ধ্রুবক Ast_গ্রন্থি* childp,
                         ধ্রুবক std::string& childName) {
    যদি (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = childp; nodep; nodep = nodep->nextp()) {
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

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_Dot(std::ostream& os) ধ্রুবক {
    os << "\tn" << cvtToHex(এই) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, এই, m_op1p, "op1");
    drawChildren(os, এই, m_op2p, "op2");
    drawChildren(os, এই, m_op3p, "op3");
    drawChildren(os, এই, m_op4p, "op4");
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_Json_File(ধ্রুবক string& filename, যৌক্তিক doDump) {
    যদি (!doDump) ফেরত;
    UINFO(2, "Dumping " << filename);
    ধ্রুবক std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    যদি (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_বৃক্ষ_Json(*treejsonp);
    *treejsonp << '\n';
}

শূন্য Ast_গ্রন্থি::dumpJsonMetaFileGdb(ধ্রুবক অক্ষর* filename) { dumpJsonMetaFile(filename); }
শূন্য Ast_গ্রন্থি::dumpJsonMetaFile(ধ্রুবক string& filename) {
    UINFO(2, "Dumping " << filename);
    ধ্রুবক std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    যদি (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_সারি::file_Name_অঙ্ক_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

শূন্য Ast_গ্রন্থি::dump_বৃক্ষ_Dot_File(ধ্রুবক string& filename, যৌক্তিক doDump) {
    যদি (doDump) {
        UINFO(2, "Dumping " << filename);
        ধ্রুবক std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
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

string Ast_গ্রন্থি::instanceStr() ধ্রুবক {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_গুনতি = 0;
    // Walk 'backp' chain
    জন্য (ধ্রুবক Ast_গ্রন্থি* currp = এই; currp; currp = currp->backp(), ++iter_গুনতি) {
        যদি (VL_UNCOVERABLE(iter_গুনতি >= maxIterations)) ফেরত "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        যদি (ধ্রুবক AstScope* ধ্রুবক scopep = VN_CAST(currp, Scope)) {
            ফেরত scopep->is_শীর্ষ() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        যদি (ধ্রুবক AstModule* ধ্রুবক modp = VN_CAST(currp, Module)) {
            ধ্রুবক string instanceName = modp->someInstanceName();
            ফেরত instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    ফেরত "";
}
শূন্য Ast_গ্রন্থি::v3errorEnd(ধ্রুবক std::ostringstream& str) ধ্রুবক VL_RELEASE(V3Error::s().m_mutex) {
    যদি (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } অন্যথা {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ধ্রুবক string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        যদি (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_গ্রন্থি*>(এই)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
শূন্য Ast_গ্রন্থি::v3errorEndFatal(ধ্রুবক std::ostringstream& str) ধ্রুবক
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    প্রতিজ্ঞা(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

শূন্য Ast_গ্রন্থি::dtypeChgSigned(যৌক্তিক flag) {
    UASSERT_OBJ(dtypep(), এই, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
শূন্য Ast_গ্রন্থি::dtypeChgWidth(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin) {
    UASSERT_OBJ(dtypep(), এই, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

শূন্য Ast_গ্রন্থি::dtypeChgWidthSigned(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), এই, "No dtype when changing width");
    ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক enumDTypep = dtypep()->skipRefToEnump();
    যদি (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        ফেরত;  // Correct already
    }
    ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findBasicDType(VBasicDTypeKwd kwd) ধ্রুবক {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    ফেরত v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findBitDType(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findLogicDType(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findLogic2StateDType(পূর্ণসংখ্যা width, পূর্ণসংখ্যা widthMin, VSigning numeric) ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findLogicRangeDType(ধ্রুবক VNumRange& range, পূর্ণসংখ্যা widthMin,
                                           VSigning numeric) ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findBitRangeDType(ধ্রুবক VNumRange& range, পূর্ণসংখ্যা widthMin,
                                         VSigning numeric) ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_গ্রন্থি::find_ঢোকাও_Same_DType(AstBasicDType* nodep) {
    ফেরত v3Global.rootp()->typeTablep()->find_ঢোকাও_Same_DType(nodep);
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findConstraintRefDType() ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findEmptyQueueDType() ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::find_Queue_সূচক_DType() ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->find_Queue_সূচক_DType(fileline());
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findStreamDType() ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_গ্রন্থি_DType* Ast_গ্রন্থি::findVoidDType() ধ্রুবক {
    ফেরত v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

স্থির ধ্রুবক Ast_গ্রন্থি_DType* computeCastableBase(ধ্রুবক Ast_গ্রন্থি_DType* nodep) {
    যখন (সত্য) {
        যদি (ধ্রুবক AstPackArrayDType* ধ্রুবক packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            চালু;
        }
        যদি (ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক refp = nodep->skipRefToEnump()) {
            যদি (refp != nodep) {
                nodep = refp;
                চালু;
            }
        }
        ফেরত nodep;
    }
}

স্থির VCastable computeCastableImp(ধ্রুবক Ast_গ্রন্থি_DType* toDtp, ধ্রুবক Ast_গ্রন্থি_DType* fromDtp,
                                    ধ্রুবক Ast_গ্রন্থি* fromConstp, ধ্রুবক যৌক্তিক checkIfaceArgCompat) {
    ধ্রুবক VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    যদি (toDtp == fromDtp) ফেরত VCastable::SAMEISH;
    যদি (toDtp->similarDType(fromDtp)) ফেরত VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ধ্রুবক Ast_গ্রন্থি_DType* fromBaseDtp = computeCastableBase(fromDtp);

    ধ্রুবক যৌক্তিক fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, গ্রন্থি_UOr_Struct_DType);

    ধ্রুবক Ast_গ্রন্থি_DType* toBaseDtp = computeCastableBase(toDtp);
    ধ্রুবক যৌক্তিক toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, গ্রন্থি_UOr_Struct_DType);

    যদি (toBaseDtp == fromBaseDtp) ফেরত VCastable::COMPATIBLE;

    যদি (toNumericable) {
        যদি (fromNumericable) ফেরত VCastable::COMPATIBLE;
    } অন্যথা যদি (VN_IS(toBaseDtp, EnumDType)) {
        যদি (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_বৃক্ষ(fromDtp))
            ফেরত VCastable::ENUM_IMPLICIT;
        যদি (fromNumericable) ফেরত VCastable::ENUM_EXPLICIT;
    } অন্যথা যদি (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        ফেরত VCastable::COMPATIBLE;
    } অন্যথা যদি (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        যদি (fromConstp->isNull()) ফেরত VCastable::COMPATIBLE;
    } অন্যথা যদি (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ধ্রুবক AstClass* ধ্রুবক toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        ধ্রুবক AstClass* ধ্রুবক fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ধ্রুবক যৌক্তিক downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        ধ্রুবক যৌক্তিক upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        যদি (upcast) ফেরত VCastable::COMPATIBLE;
        যদি (downcast) ফেরত VCastable::DYNAMIC_CLASS;
        ফেরত VCastable::INCOMPATIBLE;
    } অন্যথা যদি (ধ্রুবক AstIfaceRefDType* ধ্রুবক toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(সত্য) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ধ্রুবক AstIfaceRefDType* ধ্রুবক fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(সত্য) : fromDtp, IfaceRefDType);
        যদি (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            ধ্রুবক AstUnpackArrayDType* ধ্রুবক fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            যদি (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                ফেরত VCastable::INCOMPATIBLE;
            }
            ফেরত computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        যদি (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            যদি (!checkIfaceArgCompat) ফেরত castable;
            ফেরত VCastable::INCOMPATIBLE;
        }
        ধ্রুবক যৌক্তিক sameModport = toIfp->modportp() == fromIfp->modportp();
        যদি (!checkIfaceArgCompat) {
            যদি (!toIfp->modportp() || sameModport) ফেরত VCastable::COMPATIBLE;
            ফেরত castable;
        }
        যদি (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            ফেরত VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        যদি (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            ফেরত VCastable::COMPATIBLE;
        }
        ফেরত VCastable::INCOMPATIBLE;
    } অন্যথা যদি (ধ্রুবক AstUnpackArrayDType* ধ্রুবক toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ধ্রুবক AstUnpackArrayDType* ধ্রুবক fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        যদি (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            ফেরত castable;
        }
        ধ্রুবক VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক toSubp = toArrayp->subDTypep()->skipRefp();
        যদি (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            ফেরত VCastable::COMPATIBLE;
        }
    }
    ফেরত castable;
}

VCastable Ast_গ্রন্থি::computeCastable(ধ্রুবক Ast_গ্রন্থি_DType* toDtp, ধ্রুবক Ast_গ্রন্থি_DType* fromDtp,
                                   ধ্রুবক Ast_গ্রন্থি* fromConstp, ধ্রুবক যৌক্তিক checkIfaceArgCompat) {
    ধ্রুবক auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    যদি (fromConstp) UINFO(9, "     const= " << fromConstp);
    ফেরত castable;
}

Ast_গ্রন্থি_DType* Ast_গ্রন্থি::getCommonClassTypep(Ast_গ্রন্থি* node1p, Ast_গ্রন্থি* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    যদি (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        ধ্রুবক VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        যদি (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            ফেরত node1p->dtypep();
        যদি (castable == VCastable::DYNAMIC_CLASS) ফেরত node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    যখন (classDtypep1) {
        ধ্রুবক VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        যদি (castable == VCastable::COMPATIBLE) ফেরত classDtypep1;
        AstClassExtends* ধ্রুবক extendsp = classDtypep1->classp()->extendsp();
        যদি (!extendsp) বিরতি;
        Ast_গ্রন্থি_DType* ধ্রুবক edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    ফেরত nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

শ্রেণী VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<পূর্ণসংখ্যা, std::string> m_internedWordWidths;  // Interned widths
    std::map<পূর্ণসংখ্যা, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ধ্রুবক যৌক্তিক m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    স্থির std::string toLetters(আকার_t মান, যৌক্তিক lowerCase = মিথ্যা) {
        ধ্রুবক অক্ষর base = lowerCase ? 'a' : 'A';
        std::string s;
        করো { s += static_cast<অক্ষর>(base + মান % 26); } যখন (মান /= 26);
        ফেরত s;
    }

    ধ্রুবক std::string& internConst(ধ্রুবক AstConst& nodep) {
        ধ্রুবক auto pair = m_internedConsts.emplace(nodep.অঙ্ক().ascii(মিথ্যা), "");
        যদি (pair.second) pair.first->second += toLetters(m_internedConsts.আকার_2() - 1);
        ফেরত pair.first->second;
    }

    ধ্রুবক std::string& internWordWidth(পূর্ণসংখ্যা মান) {
        ধ্রুবক auto pair = m_internedWordWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWordWidths.আকার_2() - 1, সত্য);
        ফেরত pair.first->second;
    }

    ধ্রুবক std::string& internWideWidth(পূর্ণসংখ্যা মান) {
        ধ্রুবক auto pair = m_internedWideWidths.emplace(মান, "");
        যদি (pair.second) pair.first->second += toLetters(m_internedWideWidths.আকার_2() - 1);
        ফেরত pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    স্থির যৌক্তিক isLeaf(ধ্রুবক Ast_গ্রন্থি* nodep) {
        ধ্রুবক VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        জন্য (ধ্রুবক VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            যদি (opType != VNTypeInfo::OP_UNUSED) ফেরত মিথ্যা;
        }
        ফেরত সত্য;
    }

    // Render operand, return true if not unused
    শূন্য renderOperand(VNTypeInfo::OpEn opType, ধ্রুবক Ast_গ্রন্থি* ধ্রুবক opp, uint32_t depth) {
        নির্বাচন (opType) {
        ক্ষেত্র VNTypeInfo::OP_UNUSED:  //
            বিরতি;
        ক্ষেত্র VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            বিরতি;
        ক্ষেত্র VNTypeInfo::OP_LIST:
            m_os << " [";
            জন্য (ধ্রুবক Ast_গ্রন্থি* nodep = opp; nodep; nodep = nodep->nextp()) {
                যদি (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            বিরতি;
        ক্ষেত্র VNTypeInfo::OP_OPTIONAL:
            যদি (opp) {
                m_os << " ";
                render(opp, depth);
            } অন্যথা {
                m_os << " nil";
            }
            বিরতি;
        }
    }

    // Render the node into the stream.
    শূন্য render(ধ্রুবক Ast_গ্রন্থি* nodep, uint32_t depth) {
        যদি (ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(nodep, Const)) {
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
            ধ্রুবক VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ধ্রুবক Ast_গ্রন্থি_DType* ধ্রুবক dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        যদি (!dtypep) {
            m_os << '?';
        } অন্যথা যদি (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } অন্যথা {
            ধ্রুবক পূর্ণসংখ্যা width = nodep->width();
            যদি (width == 1) {
                m_os << '1';
            } অন্যথা যদি (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } অন্যথা {
                m_os << internWideWidth(width);
            }
            যদি (m_dumpWidthMin) {
                m_os << '/';
                ধ্রুবক পূর্ণসংখ্যা widthMin = nodep->widthMin();
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

সর্বজনীন:
    VNPatternString(std::ostream& os, ধ্রুবক Ast_গ্রন্থি* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_গ্রন্থি_রাশিমালা::patternString(uint32_t depth) ধ্রুবক {
    std::ostringstream oss;
    VNPatternString{oss, এই, depth};
    ফেরত oss.str();
}

//######################################################################
// VNDeleter

শূন্য VNDeleter::doDeletes() {
    জন্য (Ast_গ্রন্থি* ধ্রুবক nodep : m_deleteps) nodep->delete_বৃক্ষ();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#অন্তর্ভাব "V3Ast__gen_visitor_defns.h"  // From ./astgen
