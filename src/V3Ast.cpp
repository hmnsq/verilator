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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Broken.h"
#अन्तर्भाव "V3File.h"

#अन्तर्भाव <iomanip>
#अन्तर्भाव <memory>
#अन्तर्भाव <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_गाँठो::s_editCntLast = 0;
uint64_t Ast_गाँठो::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
पूर्णाङ्क Ast_गाँठो::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

तार्किक VNUser1InUse::s_userBusy = असत्य;
तार्किक VNUser2InUse::s_userBusy = असत्य;
तार्किक VNUser3InUse::s_userBusy = असत्य;
तार्किक VNUser4InUse::s_userBusy = असत्य;

पूर्णाङ्क Ast_गाँठो_DType::s_unique_अङ्क = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(अचर string& name) {
    लागि (पूर्णाङ्क i = 0; i < _ENUM_MAX; ++i) {
        अचर VCMethod method{i};
        यदि (name == method.ascii()) फर्काऊ method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    फर्काऊ VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(अचर शून्य*)) अचर {
#ifdef VL_USER_TYPE_CHECKS
    यदि (अचर uint64_t* अचर uip = std::get_if<uint64_t>(&m_u)) फर्काऊ "#"s + cvtToStr(*uip);
    यदि (शून्य* अचर* अचर upp = std::get_if<शून्य*>(&m_u)) फर्काऊ fmtAddrp(*upp);
    फर्काऊ "";
#अन्यथा
    // Dumps void* representation
    यदि (!m_u.up) फर्काऊ "";
    फर्काऊ fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

अचर VNTypeInfo VNType::s_typeInfoTable[VNType::अङ्क_TYPES()] = {
#अन्तर्भाव "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& चालक<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

तार्किक VFwdType::is_गाँठो_Compatible(अचर Ast_गाँठो* nodep) अचर {
    अचर Ast_गाँठो* defp = nodep;
    यदि (अचर AstTypedef* अचर adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    यदि (अचर Ast_गाँठो_DType* अचर adefp = VN_CAST(defp, गाँठो_DType))
        defp = adefp->skipRefToNonRefp();
    विकल्प (m_e) {
    पक्ष VFwdType::NONE: फर्काऊ सत्य; रोक;
    पक्ष VFwdType::ENUM: फर्काऊ VN_IS(defp, EnumDType); रोक;
    पक्ष VFwdType::STRUCT: फर्काऊ VN_IS(defp, StructDType); रोक;
    पक्ष VFwdType::UNION: फर्काऊ VN_IS(defp, UnionDType); रोक;
    पक्ष VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    पक्ष VFwdType::CLASS: फर्काऊ VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); रोक;
    पक्ष VFwdType::GENERIC_INTERFACE: फर्काऊ VN_IS(defp, IfaceRefDType); रोक;
    स्वभाव: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    फर्काऊ असत्य;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

अचर std::shared_ptr<अचर string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
अचर std::shared_ptr<अचर string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(तार्किक useSelfForThis, अचर string& text) {
    फर्काऊ useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(तार्किक useSelfForThis, तार्किक protect) अचर {
    फर्काऊ VIdProtect::protect_शब्दहरू_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_गाँठो::Ast_गाँठो(VNType t, File_पङ्क्ति* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = एतत्;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = असत्य;
    m_flags.doingWidth = असत्य;
    m_flags.protect = सत्य;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_गन्ती_Inc();
}

Ast_गाँठो* Ast_गाँठो::abovep() अचर {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), एतत्, "abovep() not allowed when in midlist");
    अचर Ast_गाँठो* अचर firstp = firstAbovep() ? एतत् : m_headtailp;
    फर्काऊ firstp->backp();
}
Ast_गाँठो* Ast_गाँठो::aboveLoopp() अचर {
    // Returns parent node. Avoid using this, may have performance issues.
    अचर Ast_गाँठो* nodep = एतत्;
    // Backwards over peers (versus parents)
    जबसम्म (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    फर्काऊ nodep->backp();
}

string Ast_गाँठो::encodeName(अचर string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    लागि (स्वतः pos = namein.begin(); pos != namein.end(); ++pos) {
        यदि ((pos == namein.begin()) ? std::है_अक्षर(pos[0])  // digits can't lead identifiers
                                    : std::है_अक्षर_अंक(pos[0])) {
            out += pos[0];
        } अन्यथा यदि (pos[0] == '_') {
            out += pos[0];
            यदि (pos + 1 == namein.end()) रोक;
            यदि (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } अन्यथा {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            अचर अचिन्हित val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    फर्काऊ vname.hashedName();
}

string Ast_गाँठो::encode_सङ्ख्या(int64_t अङ्क) {
    यदि (अङ्क < 0) फर्काऊ "__02D" + cvtToStr(-अङ्क);  // 2D=-
    फर्काऊ cvtToStr(अङ्क);
}

string Ast_गाँठो::nameProtect() अचर { फर्काऊ VIdProtect::protectIf(name(), protect()); }
string Ast_गाँठो::origNameProtect() अचर { फर्काऊ VIdProtect::protectIf(origName(), protect()); }

string Ast_गाँठो::shortName() अचर {
    string pretty = name();
    string::size_type pos;
    जबसम्म ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    फर्काऊ pretty;
}

string Ast_गाँठो::dedotName(अचर string& namein) {
    string pretty = namein;
    string::size_type pos;
    जबसम्म ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    यदि (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    फर्काऊ pretty;
}

string Ast_गाँठो::nameNoArray(अचर string& namein) {
    फर्काऊ namein.substr(0, namein.find("__BRA__"));
}

string Ast_गाँठो::vcdName(अचर string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    जबसम्म ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    जबसम्म ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    फर्काऊ prettyName(pretty);
}

string Ast_गाँठो::prettyName(अचर string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    लागि (अचर वर्ण* pos = namein.c_str(); *pos;) {
        यदि (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            जारी;
        }
        यदि (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            यदि (0 == std::म_भेदन(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                जारी;
            }
            यदि (0 == std::म_भेदन(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                जारी;
            }
            यदि (0 == std::म_भेदन(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                जारी;
            }
            यदि (0 == std::म_भेदन(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                जारी;
            }
            यदि (0 == std::म_भेदन(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                जारी;
            }
            यदि (0 == std::म_भेदन(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                जारी;
            }
            यदि (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::है_षष्ठादशक(pos[3])
                && std::है_षष्ठादशक(pos[4])) {
                वर्ण मान = 0;
                मान += 16
                         * (std::है_अंक(pos[3]) ? (pos[3] - '0')
                                                 : (std::छोटे(pos[3]) - 'a' + 10));
                मान
                    += (std::है_अंक(pos[4]) ? (pos[4] - '0') : (std::छोटे(pos[4]) - 'a' + 10));
                pretty += मान;
                pos += 5;
                जारी;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    यदि (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    यदि (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    फर्काऊ pretty;
}

string Ast_गाँठो::vpiName(अचर string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    अचर माप_प्रकार offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    तार्किक inEscapedIdent = असत्य;
    पूर्णाङ्क lastIdent = 0;

    लागि (अचर वर्ण* pos = namein.c_str() + offset; *pos;) {
        वर्ण specialChar = 0;
        यदि (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } अन्यथा यदि (pos[0] == '_' && pos[1] == '_') {  // __
            यदि (0 == std::म_भेदन(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } अन्यथा यदि (0 == std::म_भेदन(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } अन्यथा यदि (0 == std::म_भेदन(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } अन्यथा यदि (0 == std::म_भेदन(pos, "__PVT__", 7)) {
                pos += 7;
                जारी;
            } अन्यथा यदि (0 == std::म_भेदन(pos, "__0", 3) && std::है_षष्ठादशक(pos[3])
                       && std::है_षष्ठादशक(pos[4])) {
                वर्ण मान = 0;
                मान += 16
                         * (std::है_अंक(pos[3]) ? (pos[3] - '0')
                                                 : (std::छोटे(pos[3]) - 'a' + 10));
                मान
                    += (std::है_अंक(pos[4]) ? (pos[4] - '0') : (std::छोटे(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                यदि (मान != '_') inEscapedIdent = सत्य;

                pretty += मान;
                pos += 5;
                जारी;
            }
        } अन्यथा यदि (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        यदि (specialChar) {
            यदि (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.घुसाउ(lastIdent, "\\");
                inEscapedIdent = असत्य;
            }

            pretty += specialChar;

            यदि (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = असत्य;
            }
        } अन्यथा {
            pretty += pos[0];
            ++pos;
        }
    }
    यदि (inEscapedIdent) {
        pretty += " ";
        pretty.घुसाउ(lastIdent, "\\");
    }
    यदि (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    यदि (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    फर्काऊ pretty;
}

string Ast_गाँठो::prettyTypeName() अचर {
    यदि (name() == "") फर्काऊ typeName();
    फर्काऊ std::string{typeName()} + " '" + pretty_Name_सन्देश() + "'";
}

//######################################################################
// Insertion

शून्य Ast_गाँठो::debug_रूख_Change(अचर Ast_गाँठो* nodep, अचर वर्ण* prefix, पूर्णाङ्क lineno, तार्किक next) {
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

ढाँचा <>
Ast_गाँठो* Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(Ast_गाँठो* nodep, Ast_गाँठो* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_रूख_Change(nodep, "-addNextThs: ", __LINE__, असत्य);
    debug_रूख_Change(newp, "-addNextNew: ", __LINE__, सत्य);
    यदि (!nodep)  // verilog.y and lots of other places assume this
        फर्काऊ newp;

    // Find end of old list
    Ast_गाँठो* oldtailp = nodep;
    यदि (oldtailp->m_nextp) {
        यदि (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } अन्यथा {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            जबसम्म (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_गाँठो* अचर newtailp = newp->m_headtailp;
    Ast_गाँठो* अचर headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_गन्ती_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_रूख_Change(nodep, "-addNextOut:", __LINE__, सत्य);
    फर्काऊ nodep;
}

शून्य Ast_गाँठो::addNextHere(Ast_गाँठो* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_रूख_Change(एतत्, "-addHereThs: ", __LINE__, असत्य);
    debug_रूख_Change(newp, "-addHereNew: ", __LINE__, सत्य);
    newp->edit_गन्ती_Inc();

    Ast_गाँठो* अचर addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_गाँठो* अचर oldnextp = एतत्->m_nextp;
    एतत्->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    यदि (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = एतत्;

    // Head/tail
    Ast_गाँठो* अचर oldheadtailp = एतत्->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    यदि (oldheadtailp) {  // else in middle of list, no change
        यदि (oldheadtailp == एतत्) {  // this was one node
            एतत्->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } अन्यथा यदि (!oldnextp) {  // this was tail
            एतत्->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_रूख_Change(एतत्, "-addHereOut: ", __LINE__, सत्य);
}

शून्य Ast_गाँठो::setOp1p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, एतत्, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_रूख_Change(एतत्, "-setOp1pThs: ", __LINE__, असत्य);
    debug_रूख_Change(newp, "-setOp1pNew: ", __LINE__, सत्य);
    m_op1p = newp;
    newp->edit_गन्ती_Inc();
    newp->m_backp = एतत्;
    debug_रूख_Change(एतत्, "-setOp1pOut: ", __LINE__, असत्य);
}

शून्य Ast_गाँठो::setOp2p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, एतत्, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_रूख_Change(एतत्, "-setOp2pThs: ", __LINE__, असत्य);
    debug_रूख_Change(newp, "-setOp2pNew: ", __LINE__, सत्य);
    m_op2p = newp;
    newp->edit_गन्ती_Inc();
    newp->m_backp = एतत्;
    debug_रूख_Change(एतत्, "-setOp2pOut: ", __LINE__, असत्य);
}

शून्य Ast_गाँठो::setOp3p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, एतत्, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_रूख_Change(एतत्, "-setOp3pThs: ", __LINE__, असत्य);
    debug_रूख_Change(newp, "-setOp3pNew: ", __LINE__, सत्य);
    m_op3p = newp;
    newp->edit_गन्ती_Inc();
    newp->m_backp = एतत्;
    debug_रूख_Change(एतत्, "-setOp3pOut: ", __LINE__, असत्य);
}

शून्य Ast_गाँठो::setOp4p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, एतत्, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_रूख_Change(एतत्, "-setOp4pThs: ", __LINE__, असत्य);
    debug_रूख_Change(newp, "-setOp4pNew: ", __LINE__, सत्य);
    m_op4p = newp;
    newp->edit_गन्ती_Inc();
    newp->m_backp = एतत्;
    debug_रूख_Change(एतत्, "-setOp4pOut: ", __LINE__, असत्य);
}

शून्य Ast_गाँठो::addOp1p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    यदि (!m_op1p) {
        op1p(newp);
    } अन्यथा {
        m_op1p->addNext(newp);
    }
}

शून्य Ast_गाँठो::addOp2p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    यदि (!m_op2p) {
        op2p(newp);
    } अन्यथा {
        m_op2p->addNext(newp);
    }
}

शून्य Ast_गाँठो::addOp3p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    यदि (!m_op3p) {
        op3p(newp);
    } अन्यथा {
        m_op3p->addNext(newp);
    }
}

शून्य Ast_गाँठो::addOp4p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    यदि (!m_op4p) {
        op4p(newp);
    } अन्यथा {
        m_op4p->addNext(newp);
    }
}

शून्य Ast_गाँठो::prependOp1p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    यदि (!m_op1p) {
        op1p(newp);
    } अन्यथा {
        m_op1p->addHereThisAsNext(newp);
    }
}

शून्य Ast_गाँठो::prependOp2p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    यदि (!m_op2p) {
        op2p(newp);
    } अन्यथा {
        m_op2p->addHereThisAsNext(newp);
    }
}

शून्य Ast_गाँठो::prependOp3p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    यदि (!m_op3p) {
        op3p(newp);
    } अन्यथा {
        m_op3p->addHereThisAsNext(newp);
    }
}

शून्य Ast_गाँठो::prependOp4p(Ast_गाँठो* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    यदि (!m_op4p) {
        op4p(newp);
    } अन्यथा {
        m_op4p->addHereThisAsNext(newp);
    }
}

शून्य Ast_गाँठो::replaceWith(Ast_गाँठो* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    एतत्->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
शून्य Ast_गाँठो::replaceWithKeepDType(Ast_गाँठो* newp) {
    newp->dtypeFrom(एतत्);
    replaceWith(newp);
}

शून्य VNRelinker::dump(std::ostream& str) अचर {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_गाँठो* Ast_गाँठो::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_रूख_Change(एतत्, "-unlinkWNextThs: ", __LINE__, सत्य);
    Ast_गाँठो* अचर oldp = एतत्;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_गन्ती_Inc();
    // cppcheck-suppress shadowFunction
    Ast_गाँठो* अचर backp = oldp->m_backp;
    यदि (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        यदि (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } अन्यथा यदि (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } अन्यथा यदि (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } अन्यथा यदि (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } अन्यथा यदि (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } अन्यथा {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    यदि (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_गाँठो* oldtailp = backp->m_headtailp;
        यदि (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            जबसम्म (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_गाँठो* अचर oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } अन्यथा यदि (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } अन्यथा यदि (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } अन्यथा यदि (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } अन्यथा यदि (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } अन्यथा {
        एतत्->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    यदि (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_रूख_Change(oldp, "-unlinkWNextOut: ", __LINE__, सत्य);
    फर्काऊ oldp;
}

Ast_गाँठो* Ast_गाँठो::unlinkFrBack(VNRelinker* linkerp) {
    debug_रूख_Change(एतत्, "-unlinkFrBkThs: ", __LINE__, सत्य);
    Ast_गाँठो* अचर oldp = एतत्;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_गन्ती_Inc();
    // cppcheck-suppress shadowFunction
    Ast_गाँठो* अचर backp = oldp->m_backp;
    यदि (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        यदि (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        यदि (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } अन्यथा यदि (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } अन्यथा यदि (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } अन्यथा यदि (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } अन्यथा यदि (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } अन्यथा {
            एतत्->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    यदि (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        यदि (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        यदि (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } अन्यथा {
        यदि (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } अन्यथा यदि (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } अन्यथा यदि (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } अन्यथा यदि (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } अन्यथा {
            एतत्->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        यदि (oldp->m_nextp) {
            Ast_गाँठो* अचर newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    यदि (oldp->m_iterpp) {  // Only if no linker, point to next in list
        यदि (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_रूख_Change(oldp, "-unlinkFrBkOut: ", __LINE__, सत्य);
    फर्काऊ oldp;
}

शून्य Ast_गाँठो::relink(VNRelinker* linkerp) {
    यदि (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_गाँठो* अचर newp = एतत्;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_गन्ती_Inc();

    यदि (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_गाँठो* अचर backp = linkerp->m_backp;
    debug_रूख_Change(एतत्, "-relinkNew: ", __LINE__, सत्य);
    debug_रूख_Change(backp, "-relinkTre: ", __LINE__, सत्य);

    विकल्प (linkerp->m_chg) {
    पक्ष VNRelinker::RELINK_NEXT: backp->addNextHere(newp); रोक;
    पक्ष VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); रोक;
    पक्ष VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); रोक;
    पक्ष VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); रोक;
    पक्ष VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); रोक;
    स्वभाव: एतत्->v3fatalSrc("Relink of node without any link to change."); रोक;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    यदि (linkerp->m_iterpp) {
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
    debug_रूख_Change(एतत्, "-relinkOut: ", __LINE__, सत्य);
}

शून्य Ast_गाँठो::relinkOneLink(Ast_गाँठो*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_गाँठो* newp) {
    यदि (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_गाँठो* अचर newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_गाँठो* अचर oldlistlastp = pointpr->m_headtailp;
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

शून्य Ast_गाँठो::addHereThisAsNext(Ast_गाँठो* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(एतत्->m_backp, एतत्, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_गाँठो* अचर backp = एतत्->m_backp;
    Ast_गाँठो* अचर newLastp = newp->m_headtailp;
    //
    एतत्->edit_गन्ती_Inc();
    // Common linkage
    newLastp->m_nextp = एतत्;
    एतत्->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    यदि (backp && backp->m_nextp == एतत्) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } अन्यथा {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        यदि (backp) {
            यदि (backp->m_op1p == एतत्) {
                backp->m_op1p = newp;
            } अन्यथा यदि (backp->m_op2p == एतत्) {
                backp->m_op2p = newp;
            } अन्यथा यदि (backp->m_op3p == एतत्) {
                backp->m_op3p = newp;
            } अन्यथा {
                UASSERT_OBJ(backp->m_op4p == एतत्, एतत्, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_गाँठो* अचर tailp = एतत्->m_headtailp;
        एतत्->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    यदि (newLastp->m_iterpp) *(newLastp->m_iterpp) = एतत्;
    यदि (एतत्->m_iterpp) {
        *(एतत्->m_iterpp) = newp;
        एतत्->m_iterpp = nullptr;
    }
    //
    debug_रूख_Change(एतत्, "-addHereThisAsNext: ", __LINE__, सत्य);
}

//======================================================================
// Clone

Ast_गाँठो* Ast_गाँठो::clone_रूख_Iter(तार्किक needPure) {
    // private: Clone single node and children
    यदि (needPure) purityCheck();
    Ast_गाँठो* अचर newp = एतत्->clone();
    यदि (एतत्->m_op1p) newp->op1p(एतत्->m_op1p->clone_रूख_Iter_List(needPure));
    यदि (एतत्->m_op2p) newp->op2p(एतत्->m_op2p->clone_रूख_Iter_List(needPure));
    यदि (एतत्->m_op3p) newp->op3p(एतत्->m_op3p->clone_रूख_Iter_List(needPure));
    यदि (एतत्->m_op4p) newp->op4p(एतत्->m_op4p->clone_रूख_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(एतत्);  // Save pointers to/from both to simplify relinking.
    एतत्->clonep(newp);  // Save pointers to/from both to simplify relinking.
    फर्काऊ newp;
}

Ast_गाँठो* Ast_गाँठो::clone_रूख_Iter_List(तार्किक needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_गाँठो* newheadp = nullptr;
    Ast_गाँठो* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    लागि (Ast_गाँठो* oldp = एतत्; oldp; oldp = oldp->m_nextp) {
        Ast_गाँठो* अचर newp = oldp->clone_रूख_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        यदि (newtailp) newtailp->m_nextp = newp;
        यदि (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    फर्काऊ newheadp;
}

Ast_गाँठो* Ast_गाँठो::clone_रूख(तार्किक cloneNextLink, तार्किक needPure) {
    debug_रूख_Change(एतत्, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_रूख();
    Ast_गाँठो* newp;
    यदि (cloneNextLink && एतत्->m_nextp) {
        newp = clone_रूख_Iter_List(needPure);
    } अन्यथा {
        newp = clone_रूख_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_रूख();
    debug_रूख_Change(newp, "-cloneOut: ", __LINE__, सत्य);
    फर्काऊ newp;
}

शून्य Ast_गाँठो::purityCheck() {
    यदि (VL_UNLIKELY(!isPure())) {
        एतत्->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << एतत्->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

शून्य Ast_गाँठो::delete_गाँठो() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_गन्ती_Inc();
    // Change links of old node so we coredump if used
    एतत्->m_nextp = reinterpret_cast<Ast_गाँठो*>(0x1);
    एतत्->m_backp = reinterpret_cast<Ast_गाँठो*>(0x1);
    एतत्->m_headtailp = reinterpret_cast<Ast_गाँठो*>(0x1);
    एतत्->m_op1p = reinterpret_cast<Ast_गाँठो*>(0x1);
    एतत्->m_op2p = reinterpret_cast<Ast_गाँठो*>(0x1);
    एतत्->m_op3p = reinterpret_cast<Ast_गाँठो*>(0x1);
    एतत्->m_op4p = reinterpret_cast<Ast_गाँठो*>(0x1);
    एतत्->m_iterpp = reinterpret_cast<Ast_गाँठो**>(0x1);
    यदि (
#यदि !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        सत्य
#अन्यथा
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete एतत्;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

शून्य Ast_गाँठो::delete_रूख_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    लागि (Ast_गाँठो *nodep = एतत्, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        यदि (nodep->m_op1p) nodep->m_op1p->delete_रूख_Iter();
        यदि (nodep->m_op2p) nodep->m_op2p->delete_रूख_Iter();
        यदि (nodep->m_op3p) nodep->m_op3p->delete_रूख_Iter();
        यदि (nodep->m_op4p) nodep->m_op4p->delete_रूख_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_गाँठो();
    }
}

शून्य Ast_गाँठो::delete_रूख() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_रूख_Change(एतत्, "-delTree:  ", __LINE__, सत्य);
    एतत्->edit_गन्ती_Inc();
    // MUST be depth first!
    delete_रूख_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
शून्य* Ast_गाँठो::चालक नूतन(माप_प्रकार size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_गाँठो* अचर objp = static_cast<Ast_गाँठो*>(::चालक नूतन(size));
    V3Broken::addNewed(objp);
    फर्काऊ objp;
}

शून्य Ast_गाँठो::चालक delete(शून्य* objp, माप_प्रकार size) {
    यदि (!objp) फर्काऊ;
    अचर Ast_गाँठो* अचर nodep = static_cast<Ast_गाँठो*>(objp);
    V3Broken::deleted(nodep);
    ::चालक delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
शून्य* Ast_गाँठो::चालक नूतन(माप_प्रकार size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    स्थिर अचर माप_प्रकार ASTGEN_MAX_गाँठो_SIZE = []() {
        माप_प्रकार maxSize = 0;
        लागि (माप_प्रकार t = 0; t < VNType::अङ्क_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        फर्काऊ maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr माप_प्रकार POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    स्थिर uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    यदि (पूर्णाङ्क seed = v3Global.opt.debugAllocRandom()) {
        स्थिर V3Mutex s_mutex;
        अचर V3LockGuard lock{s_mutex};
        स्थिर std::array<शून्य*, POOL_SIZE> s_गाँठो_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_गाँठो_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        यदि (!s_lfsr) {
            s_lfsr = seed;
            यदि (!s_lfsr) s_lfsr = ~s_lfsr;
            लागि (माप_प्रकार i = 0; i < POOL_SIZE; ++i) {
                s_गाँठो_Pool[i] = ::चालक नूतन(ASTGEN_MAX_गाँठो_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_गाँठो_Pool), std::end(s_गाँठो_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        अचर माप_प्रकार index = s_lfsr % POOL_SIZE;
        Ast_गाँठो* अचर objp = static_cast<Ast_गाँठो*>(s_गाँठो_Pool[index]);
        s_गाँठो_Pool[index] = ::चालक नूतन(ASTGEN_MAX_गाँठो_SIZE + 64);  // For later new()
        फर्काऊ objp;
    } अन्यथा {
        Ast_गाँठो* अचर objp = static_cast<Ast_गाँठो*>(::चालक नूतन(size));
        फर्काऊ objp;
    }
}
शून्य Ast_गाँठो::चालक delete(शून्य* objp, माप_प्रकार size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (शून्य)objp;
    (शून्य)size;
}
#endif

//======================================================================
// Iterators

शून्य Ast_गाँठो::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    यदि (m_op1p) m_op1p->iterateAndNext(v);
    यदि (m_op2p) m_op2p->iterateAndNext(v);
    यदि (m_op3p) m_op3p->iterateAndNext(v);
    यदि (m_op4p) m_op4p->iterateAndNext(v);
}

शून्य Ast_गाँठो::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    यदि (m_op1p) m_op1p->iterateAndNextConst(v);
    यदि (m_op2p) m_op2p->iterateAndNextConst(v);
    यदि (m_op3p) m_op3p->iterateAndNextConst(v);
    यदि (m_op4p) m_op4p->iterateAndNextConst(v);
}

शून्य Ast_गाँठो::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_गाँठो* nodep = एतत्;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    यदि (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    जबसम्म (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        यदि (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_गाँठो* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        यदि (!niterp) फर्काऊ;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        यदि (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } अन्यथा {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

शून्य Ast_गाँठो::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_गाँठो* nodep = एतत्;
    जबसम्म (nodep->m_nextp) nodep = nodep->m_nextp;
    जबसम्म (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        यदि (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } अन्यथा {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

शून्य Ast_गाँठो::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    यदि (m_op1p) m_op1p->iterateListBackwardsConst(v);
    यदि (m_op2p) m_op2p->iterateListBackwardsConst(v);
    यदि (m_op3p) m_op3p->iterateListBackwardsConst(v);
    यदि (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

शून्य Ast_गाँठो::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_गाँठो* nodep = एतत्;
    गर {
        Ast_गाँठो* अचर nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } जबसम्म (nodep);
}

Ast_गाँठो* Ast_गाँठो::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_गाँठो* nodep = एतत्;  // Note "this" may point to bogus point later in this function
    यदि (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } अन्यथा यदि (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* अचर tempp = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", nodep, असत्य};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_रूख(), tempp);
    } अन्यथा {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_गाँठो** nextnodepp = nullptr;
        यदि (एतत्->m_backp->m_op1p == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_op1p);
        } अन्यथा यदि (एतत्->m_backp->m_op2p == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_op2p);
        } अन्यथा यदि (एतत्->m_backp->m_op3p == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_op3p);
        } अन्यथा यदि (एतत्->m_backp->m_op4p == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_op4p);
        } अन्यथा यदि (एतत्->m_backp->m_nextp == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, एतत्, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    फर्काऊ nodep;
}

//======================================================================

शून्य Ast_गाँठो::clone_Relink_रूख() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    लागि (Ast_गाँठो* nodep = एतत्; nodep; nodep = nodep->m_nextp) {
        यदि (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        यदि (nodep->m_op1p) nodep->m_op1p->clone_Relink_रूख();
        यदि (nodep->m_op2p) nodep->m_op2p->clone_Relink_रूख();
        यदि (nodep->m_op3p) nodep->m_op3p->clone_Relink_रूख();
        यदि (nodep->m_op4p) nodep->m_op4p->clone_Relink_रूख();
    }
}

//======================================================================
// Comparison

तार्किक Ast_गाँठो::gate_रूख_Iter() अचर {
    // private: Return true if the two trees are identical
    यदि (!isGateOptimizable()) फर्काऊ असत्य;
    यदि (m_op1p && !m_op1p->gate_रूख_Iter()) फर्काऊ असत्य;
    यदि (m_op2p && !m_op2p->gate_रूख_Iter()) फर्काऊ असत्य;
    यदि (m_op3p && !m_op3p->gate_रूख_Iter()) फर्काऊ असत्य;
    यदि (m_op4p && !m_op4p->gate_रूख_Iter()) फर्काऊ असत्य;
    फर्काऊ सत्य;
}

तार्किक Ast_गाँठो::same_रूख_Iter(अचर Ast_गाँठो* node1p, अचर Ast_गाँठो* node2p, तार्किक ignNext,
                           तार्किक gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    जबसम्म (सत्य) {
        यदि (!node1p && !node2p) फर्काऊ सत्य;
        यदि (!node1p || !node2p) फर्काऊ असत्य;
        यदि (node1p->type() != node2p->type()) फर्काऊ असत्य;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        यदि (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) फर्काऊ असत्य;
        यदि (!node1p->same_गाँठो(node2p) || (gateOnly && !node1p->isGateOptimizable())) फर्काऊ असत्य;
        यदि (!same_रूख_Iter(node1p->m_op1p, node2p->m_op1p, असत्य, gateOnly)
            || !same_रूख_Iter(node1p->m_op2p, node2p->m_op2p, असत्य, gateOnly)
            || !same_रूख_Iter(node1p->m_op3p, node2p->m_op3p, असत्य, gateOnly)
            || !same_रूख_Iter(node1p->m_op4p, node2p->m_op4p, असत्य, gateOnly)) {
            फर्काऊ असत्य;
        }
        यदि (ignNext) फर्काऊ सत्य;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

शून्य Ast_गाँठो::check_रूख_Iter(अचर Ast_गाँठो* prevBackp) अचर VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == एतत्->backp(), एतत्, "Back node inconsistent");
    अचर VNTypeInfo& typeInfo = VNType::typeInfo(एतत्->type());
    लागि (पूर्णाङ्क i = 1; i <= 4; i++) {
        Ast_गाँठो* nodep = nullptr;
        विकल्प (i) {
        पक्ष 1: nodep = op1p(); रोक;
        पक्ष 2: nodep = op2p(); रोक;
        पक्ष 3: nodep = op3p(); रोक;
        पक्ष 4: nodep = op4p(); रोक;
        स्वभाव: एतत्->v3fatalSrc("Bad case"); रोक;
        }
        // cppcheck-suppress danglingTempReference
        अचर वर्ण* opName = typeInfo.m_opNamep[i - 1];
        विकल्प (typeInfo.m_opType[i - 1]) {
        पक्ष VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, एतत्, typeInfo.m_namep << " must not use " << opName << "()");
            रोक;
        पक्ष VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, एतत्,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), एतत्,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_रूख_Iter(एतत्);
            रोक;
        पक्ष VNTypeInfo::OP_LIST:
            यदि (अचर Ast_गाँठो* अचर headp = nodep) {
                // cppcheck-suppress shadowFunction
                अचर Ast_गाँठो* backp = एतत्;
                अचर Ast_गाँठो* tailp;
                अचर Ast_गाँठो* opp = headp;
                गर {
                    opp->check_रूख_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } जबसम्म (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            रोक;
        पक्ष VNTypeInfo::OP_OPTIONAL:
            यदि (nodep) {
                UASSERT_OBJ(!nodep->nextp(), एतत्,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_रूख_Iter(एतत्);
            }
            रोक;
        स्वभाव: एतत्->v3fatalSrc("Bad case"); रोक;
        }
    }
    यदि (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        यदि (अचर Ast_गाँठो_अभिव्यक्ति* अचर exprp = VN_CAST(एतत्, गाँठो_अभिव्यक्ति)) {
            अचर वर्ण* अचर whyp = exprp->widthMismatch();
            यदि (whyp) {
                स्वतः dtypeStr = [](अचर Ast_गाँठो_अभिव्यक्ति* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    फर्काऊ ss.str();
                };
                यदि (अचर Ast_गाँठो_Uniop* अचर uniopp = VN_CAST(exprp, गाँठो_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } अन्यथा यदि (अचर Ast_गाँठो_Biop* अचर biopp = VN_CAST(exprp, गाँठो_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } अन्यथा {
                    UASSERT_OBJ(असत्य, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
वर्ण* Ast_गाँठो::dump_रूख_Json_Gdb(अचर Ast_गाँठो* nodep) {
    यदि (!nodep) फर्काऊ strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_रूख_Json(nodepStream);
    अचर std::string str = nodepStream.rdbuf()->str();
    फर्काऊ strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
वर्ण* Ast_गाँठो::dump_रूख_Json_Gdb(अचर वर्ण* str) { फर्काऊ strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
वर्ण* Ast_गाँठो::dump_रूख_Json_Gdb(intptr_t nodep) {
    यदि (!nodep) फर्काऊ strdup("{\"addr\":\"NULL\"}\n");
    फर्काऊ dump_रूख_Json_Gdb(reinterpret_cast<अचर Ast_गाँठो*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाँठो::dumpGdb(अचर Ast_गाँठो* nodep) {  // For GDB only  // LCOV_EXCL_START
    यदि (!nodep) {
        cout << "<nullptr>" << endl;
        फर्काऊ;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाँठो::dumpGdbHeader() अचर {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाँठो::dump_रूख_Gdb(अचर Ast_गाँठो* nodep) {  // For GDB only  // LCOV_EXCL_START
    यदि (!nodep) {
        cout << "<nullptr>" << endl;
        फर्काऊ;
    }
    nodep->dumpGdbHeader();
    nodep->dump_रूख(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाँठो::dump_रूख_File_Gdb(अचर Ast_गाँठो* nodep,  // LCOV_EXCL_START
                              अचर वर्ण* filenamep) {  // For GDB only
    यदि (!nodep) {
        cout << "<nullptr>" << endl;
        फर्काऊ;
    }
    अचर string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_रूख_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाँठो::checkIter() अचर {
    यदि (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        एतत्->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

शून्य Ast_गाँठो::dumpPtrs(std::ostream& os) अचर {
    os << "This=" << typeName() << " " << cvtToHex(एतत्);
    os << " back=" << cvtToHex(backp());
    यदि (nextp()) os << " next=" << cvtToHex(nextp());
    यदि (m_headtailp == एतत्) {
        os << " headtail=this";
    } अन्यथा {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    यदि (op1p()) os << " op1p=" << cvtToHex(op1p());
    यदि (op2p()) os << " op2p=" << cvtToHex(op2p());
    यदि (op3p()) os << " op3p=" << cvtToHex(op3p());
    यदि (op4p()) os << " op4p=" << cvtToHex(op4p());
    अचर स्वतः dumpUser = [&os](अचर वर्ण* prefix, अचर VNUser& user) {
        अचर std::string s
            = user.dumpStr([](अचर शून्य* p) -> std::string { फर्काऊ cvtToHex(p); });
        यदि (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    यदि (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

शून्य Ast_गाँठो::dump_रूख(std::ostream& os, अचर string& indent, पूर्णाङ्क maxDepth) अचर {
    स्थिर पूर्णाङ्क s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << एतत् << '\n';
    यदि (VN_DELETED(एतत्)) फर्काऊ;
    यदि (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    यदि (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(एतत्)
           << endl;  // endl intentional to do flush
        फर्काऊ;
    }  // LCOV_EXCL_STOP
    यदि (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    यदि (maxDepth == 1) {
        यदि (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } अन्यथा {
        लागि (अचर Ast_गाँठो* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_रूख(os, indent + "1:", maxDepth - 1);
        }
        लागि (अचर Ast_गाँठो* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_रूख(os, indent + "2:", maxDepth - 1);
        }
        लागि (अचर Ast_गाँठो* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_रूख(os, indent + "3:", maxDepth - 1);
        }
        लागि (अचर Ast_गाँठो* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_रूख(os, indent + "4:", maxDepth - 1);
        }
    }
}

शून्य Ast_गाँठो::dump_रूख_And_Next(std::ostream& os, अचर string& indent, पूर्णाङ्क maxDepth) अचर {
    // Audited to make sure this is never nullptr
    लागि (अचर Ast_गाँठो* nodep = एतत्; nodep; nodep = nodep->nextp()) {
        nodep->dump_रूख(os, indent, maxDepth);
    }
}

शून्य Ast_गाँठो::dump_रूख_File(अचर string& filename, तार्किक doDump) {
    // Not const function as calls checkTree
    यदि (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            अचर std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            यदि (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_गन्ती_Last();
            *logsp << "> to <e" << std::dec << edit_गन्ती_Gbl() << ">\n";
            यदि (edit_गन्ती_Gbl() == edit_गन्ती_Last() && ::dump_रूख_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } अन्यथा {
                dump_रूख(*logsp);
                edit_गन्ती_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

स्थिर शून्य drawChildren(std::ostream& os, अचर Ast_गाँठो* thisp, अचर Ast_गाँठो* childp,
                         अचर std::string& childName) {
    यदि (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        लागि (अचर Ast_गाँठो* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_रूख_Dot(os);
            यदि (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

शून्य Ast_गाँठो::dump_रूख_Dot(std::ostream& os) अचर {
    os << "\tn" << cvtToHex(एतत्) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, एतत्, m_op1p, "op1");
    drawChildren(os, एतत्, m_op2p, "op2");
    drawChildren(os, एतत्, m_op3p, "op3");
    drawChildren(os, एतत्, m_op4p, "op4");
}

शून्य Ast_गाँठो::dump_रूख_Json_File(अचर string& filename, तार्किक doDump) {
    यदि (!doDump) फर्काऊ;
    UINFO(2, "Dumping " << filename);
    अचर std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    यदि (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_रूख_Json(*treejsonp);
    *treejsonp << '\n';
}

शून्य Ast_गाँठो::dumpJsonMetaFileGdb(अचर वर्ण* filename) { dumpJsonMetaFile(filename); }
शून्य Ast_गाँठो::dumpJsonMetaFile(अचर string& filename) {
    UINFO(2, "Dumping " << filename);
    अचर std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    यदि (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_पङ्क्ति::file_Name_अङ्क_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

शून्य Ast_गाँठो::dump_रूख_Dot_File(अचर string& filename, तार्किक doDump) {
    यदि (doDump) {
        UINFO(2, "Dumping " << filename);
        अचर std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        यदि (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_रूख_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_गाँठो::instanceStr() अचर {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr अचिन्हित maxIterations = 10000;
    अचिन्हित iter_गन्ती = 0;
    // Walk 'backp' chain
    लागि (अचर Ast_गाँठो* currp = एतत्; currp; currp = currp->backp(), ++iter_गन्ती) {
        यदि (VL_UNCOVERABLE(iter_गन्ती >= maxIterations)) फर्काऊ "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        यदि (अचर AstScope* अचर scopep = VN_CAST(currp, Scope)) {
            फर्काऊ scopep->is_शीर्ष() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        यदि (अचर AstModule* अचर modp = VN_CAST(currp, Module)) {
            अचर string instanceName = modp->someInstanceName();
            फर्काऊ instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    फर्काऊ "";
}
शून्य Ast_गाँठो::v3errorEnd(अचर std::ostringstream& str) अचर VL_RELEASE(V3Error::s().m_mutex) {
    यदि (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } अन्यथा {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        अचर string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        यदि (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_गाँठो*>(एतत्)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
शून्य Ast_गाँठो::v3errorEndFatal(अचर std::ostringstream& str) अचर
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

शून्य Ast_गाँठो::dtypeChgSigned(तार्किक flag) {
    UASSERT_OBJ(dtypep(), एतत्, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
शून्य Ast_गाँठो::dtypeChgWidth(पूर्णाङ्क width, पूर्णाङ्क widthMin) {
    UASSERT_OBJ(dtypep(), एतत्, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

शून्य Ast_गाँठो::dtypeChgWidthSigned(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), एतत्, "No dtype when changing width");
    अचर Ast_गाँठो_DType* अचर enumDTypep = dtypep()->skipRefToEnump();
    यदि (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        फर्काऊ;  // Correct already
    }
    अचर Ast_गाँठो_DType* अचर dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_गाँठो_DType* Ast_गाँठो::findBasicDType(VBasicDTypeKwd kwd) अचर {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    फर्काऊ v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_गाँठो_DType* Ast_गाँठो::findBitDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_गाँठो_DType* Ast_गाँठो::findLogicDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_गाँठो_DType* Ast_गाँठो::findLogic2StateDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_गाँठो_DType* Ast_गाँठो::findLogicRangeDType(अचर VNumRange& range, पूर्णाङ्क widthMin,
                                           VSigning numeric) अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_गाँठो_DType* Ast_गाँठो::findBitRangeDType(अचर VNumRange& range, पूर्णाङ्क widthMin,
                                         VSigning numeric) अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_गाँठो::find_घुसाउ_Same_DType(AstBasicDType* nodep) {
    फर्काऊ v3Global.rootp()->typeTablep()->find_घुसाउ_Same_DType(nodep);
}
Ast_गाँठो_DType* Ast_गाँठो::findConstraintRefDType() अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_गाँठो_DType* Ast_गाँठो::findEmptyQueueDType() अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_गाँठो_DType* Ast_गाँठो::findQueueIndexDType() अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_गाँठो_DType* Ast_गाँठो::findStreamDType() अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_गाँठो_DType* Ast_गाँठो::findVoidDType() अचर {
    फर्काऊ v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

स्थिर अचर Ast_गाँठो_DType* computeCastableBase(अचर Ast_गाँठो_DType* nodep) {
    जबसम्म (सत्य) {
        यदि (अचर AstPackArrayDType* अचर packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            जारी;
        }
        यदि (अचर Ast_गाँठो_DType* अचर refp = nodep->skipRefToEnump()) {
            यदि (refp != nodep) {
                nodep = refp;
                जारी;
            }
        }
        फर्काऊ nodep;
    }
}

स्थिर VCastable computeCastableImp(अचर Ast_गाँठो_DType* toDtp, अचर Ast_गाँठो_DType* fromDtp,
                                    अचर Ast_गाँठो* fromConstp, अचर तार्किक checkIfaceArgCompat) {
    अचर VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    यदि (toDtp == fromDtp) फर्काऊ VCastable::SAMEISH;
    यदि (toDtp->similarDType(fromDtp)) फर्काऊ VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    अचर Ast_गाँठो_DType* fromBaseDtp = computeCastableBase(fromDtp);

    अचर तार्किक fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, गाँठो_UOr_Struct_DType);

    अचर Ast_गाँठो_DType* toBaseDtp = computeCastableBase(toDtp);
    अचर तार्किक toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, गाँठो_UOr_Struct_DType);

    यदि (toBaseDtp == fromBaseDtp) फर्काऊ VCastable::COMPATIBLE;

    यदि (toNumericable) {
        यदि (fromNumericable) फर्काऊ VCastable::COMPATIBLE;
    } अन्यथा यदि (VN_IS(toBaseDtp, EnumDType)) {
        यदि (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_रूख(fromDtp))
            फर्काऊ VCastable::ENUM_IMPLICIT;
        यदि (fromNumericable) फर्काऊ VCastable::ENUM_EXPLICIT;
    } अन्यथा यदि (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        फर्काऊ VCastable::COMPATIBLE;
    } अन्यथा यदि (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        यदि (fromConstp->isNull()) फर्काऊ VCastable::COMPATIBLE;
    } अन्यथा यदि (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        अचर AstClass* अचर toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        अचर AstClass* अचर fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        अचर तार्किक downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        अचर तार्किक upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        यदि (upcast) फर्काऊ VCastable::COMPATIBLE;
        यदि (downcast) फर्काऊ VCastable::DYNAMIC_CLASS;
        फर्काऊ VCastable::INCOMPATIBLE;
    } अन्यथा यदि (अचर AstIfaceRefDType* अचर toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(सत्य) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        अचर AstIfaceRefDType* अचर fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(सत्य) : fromDtp, IfaceRefDType);
        यदि (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            अचर AstUnpackArrayDType* अचर toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            अचर AstUnpackArrayDType* अचर fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            यदि (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                फर्काऊ VCastable::INCOMPATIBLE;
            }
            फर्काऊ computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        यदि (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            यदि (!checkIfaceArgCompat) फर्काऊ castable;
            फर्काऊ VCastable::INCOMPATIBLE;
        }
        अचर तार्किक sameModport = toIfp->modportp() == fromIfp->modportp();
        यदि (!checkIfaceArgCompat) {
            यदि (!toIfp->modportp() || sameModport) फर्काऊ VCastable::COMPATIBLE;
            फर्काऊ castable;
        }
        यदि (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            फर्काऊ VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        यदि (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            फर्काऊ VCastable::COMPATIBLE;
        }
        फर्काऊ VCastable::INCOMPATIBLE;
    } अन्यथा यदि (अचर AstUnpackArrayDType* अचर toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        अचर AstUnpackArrayDType* अचर fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        यदि (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            फर्काऊ castable;
        }
        अचर VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        अचर Ast_गाँठो_DType* अचर toSubp = toArrayp->subDTypep()->skipRefp();
        यदि (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            फर्काऊ VCastable::COMPATIBLE;
        }
    }
    फर्काऊ castable;
}

VCastable Ast_गाँठो::computeCastable(अचर Ast_गाँठो_DType* toDtp, अचर Ast_गाँठो_DType* fromDtp,
                                   अचर Ast_गाँठो* fromConstp, अचर तार्किक checkIfaceArgCompat) {
    अचर स्वतः castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    यदि (fromConstp) UINFO(9, "     const= " << fromConstp);
    फर्काऊ castable;
}

Ast_गाँठो_DType* Ast_गाँठो::getCommonClassTypep(Ast_गाँठो* node1p, Ast_गाँठो* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    यदि (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        अचर VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        यदि (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            फर्काऊ node1p->dtypep();
        यदि (castable == VCastable::DYNAMIC_CLASS) फर्काऊ node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    जबसम्म (classDtypep1) {
        अचर VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        यदि (castable == VCastable::COMPATIBLE) फर्काऊ classDtypep1;
        AstClassExtends* अचर extendsp = classDtypep1->classp()->extendsp();
        यदि (!extendsp) रोक;
        Ast_गाँठो_DType* अचर edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    फर्काऊ nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

वर्ग VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<पूर्णाङ्क, std::string> m_internedWordWidths;  // Interned widths
    std::map<पूर्णाङ्क, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    अचर तार्किक m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    स्थिर std::string toLetters(माप_प्रकार मान, तार्किक lowerCase = असत्य) {
        अचर वर्ण base = lowerCase ? 'a' : 'A';
        std::string s;
        गर { s += static_cast<वर्ण>(base + मान % 26); } जबसम्म (मान /= 26);
        फर्काऊ s;
    }

    अचर std::string& internConst(अचर AstConst& nodep) {
        अचर स्वतः pair = m_internedConsts.emplace(nodep.अङ्क().ascii(असत्य), "");
        यदि (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        फर्काऊ pair.first->second;
    }

    अचर std::string& internWordWidth(पूर्णाङ्क मान) {
        अचर स्वतः pair = m_internedWordWidths.emplace(मान, "");
        यदि (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, सत्य);
        फर्काऊ pair.first->second;
    }

    अचर std::string& internWideWidth(पूर्णाङ्क मान) {
        अचर स्वतः pair = m_internedWideWidths.emplace(मान, "");
        यदि (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        फर्काऊ pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    स्थिर तार्किक isLeaf(अचर Ast_गाँठो* nodep) {
        अचर VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        लागि (अचर VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            यदि (opType != VNTypeInfo::OP_UNUSED) फर्काऊ असत्य;
        }
        फर्काऊ सत्य;
    }

    // Render operand, return true if not unused
    शून्य renderOperand(VNTypeInfo::OpEn opType, अचर Ast_गाँठो* अचर opp, uint32_t depth) {
        विकल्प (opType) {
        पक्ष VNTypeInfo::OP_UNUSED:  //
            रोक;
        पक्ष VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            रोक;
        पक्ष VNTypeInfo::OP_LIST:
            m_os << " [";
            लागि (अचर Ast_गाँठो* nodep = opp; nodep; nodep = nodep->nextp()) {
                यदि (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            रोक;
        पक्ष VNTypeInfo::OP_OPTIONAL:
            यदि (opp) {
                m_os << " ";
                render(opp, depth);
            } अन्यथा {
                m_os << " nil";
            }
            रोक;
        }
    }

    // Render the node into the stream.
    शून्य render(अचर Ast_गाँठो* nodep, uint32_t depth) {
        यदि (अचर AstConst* अचर constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            यदि (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } अन्यथा यदि (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } अन्यथा {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } अन्यथा यदि (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } अन्यथा यदि (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } अन्यथा {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            अचर VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        अचर Ast_गाँठो_DType* अचर dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        यदि (!dtypep) {
            m_os << '?';
        } अन्यथा यदि (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } अन्यथा {
            अचर पूर्णाङ्क width = nodep->width();
            यदि (width == 1) {
                m_os << '1';
            } अन्यथा यदि (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } अन्यथा {
                m_os << internWideWidth(width);
            }
            यदि (m_dumpWidthMin) {
                m_os << '/';
                अचर पूर्णाङ्क widthMin = nodep->widthMin();
                यदि (widthMin == 1) {
                    m_os << '1';
                } अन्यथा यदि (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } अन्यथा {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

सार्वजनिक:
    VNPatternString(std::ostream& os, अचर Ast_गाँठो* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_गाँठो_अभिव्यक्ति::patternString(uint32_t depth) अचर {
    std::ostringstream oss;
    VNPatternString{oss, एतत्, depth};
    फर्काऊ oss.str();
}

//######################################################################
// VNDeleter

शून्य VNDeleter::doDeletes() {
    लागि (Ast_गाँठो* अचर nodep : m_deleteps) nodep->delete_रूख();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#अन्तर्भाव "V3Ast__gen_visitor_defns.h"  // From ./astgen
