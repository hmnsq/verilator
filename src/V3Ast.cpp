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

uint64_t Ast_गाठ::s_editCntLast = 0;
uint64_t Ast_गाठ::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
पूर्णांक Ast_गाठ::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

तार्किक VNUser1InUse::s_userBusy = असत्य;
तार्किक VNUser2InUse::s_userBusy = असत्य;
तार्किक VNUser3InUse::s_userBusy = असत्य;
तार्किक VNUser4InUse::s_userBusy = असत्य;

पूर्णांक Ast_गाठ_DType::s_unique_अंक = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(स्थिरांक string& name) {
    साठी (पूर्णांक i = 0; i < _ENUM_MAX; ++i) {
        स्थिरांक VCMethod method{i};
        जर (name == method.ascii()) परत method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    परत VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(स्थिरांक शून्य*)) स्थिरांक {
#ifdef VL_USER_TYPE_CHECKS
    जर (स्थिरांक uint64_t* स्थिरांक uip = std::get_if<uint64_t>(&m_u)) परत "#"s + cvtToStr(*uip);
    जर (शून्य* स्थिरांक* स्थिरांक upp = std::get_if<शून्य*>(&m_u)) परत fmtAddrp(*upp);
    परत "";
#अन्यथा
    // Dumps void* representation
    जर (!m_u.up) परत "";
    परत fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

स्थिरांक VNTypeInfo VNType::s_typeInfoTable[VNType::अंक_TYPES()] = {
#अन्तर्भाव "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& चालक<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

तार्किक VFwdType::is_गाठ_Compatible(स्थिरांक Ast_गाठ* nodep) स्थिरांक {
    स्थिरांक Ast_गाठ* defp = nodep;
    जर (स्थिरांक AstTypedef* स्थिरांक adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    जर (स्थिरांक Ast_गाठ_DType* स्थिरांक adefp = VN_CAST(defp, गाठ_DType))
        defp = adefp->skipRefToNonRefp();
    निवड (m_e) {
    स्थिती VFwdType::NONE: परत सत्य; थांबा;
    स्थिती VFwdType::ENUM: परत VN_IS(defp, EnumDType); थांबा;
    स्थिती VFwdType::STRUCT: परत VN_IS(defp, StructDType); थांबा;
    स्थिती VFwdType::UNION: परत VN_IS(defp, UnionDType); थांबा;
    स्थिती VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    स्थिती VFwdType::CLASS: परत VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); थांबा;
    स्थिती VFwdType::GENERIC_INTERFACE: परत VN_IS(defp, IfaceRefDType); थांबा;
    पूर्वनिर्धारित: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    परत असत्य;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

स्थिरांक std::shared_ptr<स्थिरांक string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
स्थिरांक std::shared_ptr<स्थिरांक string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(तार्किक useSelfForThis, स्थिरांक string& text) {
    परत useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(तार्किक useSelfForThis, तार्किक protect) स्थिरांक {
    परत VIdProtect::protect_शब्द_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_गाठ::Ast_गाठ(VNType t, File_ओळ* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = हे;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = असत्य;
    m_flags.doingWidth = असत्य;
    m_flags.protect = सत्य;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_मोजणी_Inc();
}

Ast_गाठ* Ast_गाठ::abovep() स्थिरांक {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), हे, "abovep() not allowed when in midlist");
    स्थिरांक Ast_गाठ* स्थिरांक firstp = firstAbovep() ? हे : m_headtailp;
    परत firstp->backp();
}
Ast_गाठ* Ast_गाठ::aboveLoopp() स्थिरांक {
    // Returns parent node. Avoid using this, may have performance issues.
    स्थिरांक Ast_गाठ* nodep = हे;
    // Backwards over peers (versus parents)
    पर्यंत (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    परत nodep->backp();
}

string Ast_गाठ::encodeName(स्थिरांक string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    साठी (स्वतः pos = namein.begin(); pos != namein.end(); ++pos) {
        जर ((pos == namein.begin()) ? std::है_अक्षर(pos[0])  // digits can't lead identifiers
                                    : std::है_अक्षर_अंक(pos[0])) {
            out += pos[0];
        } अन्यथा जर (pos[0] == '_') {
            out += pos[0];
            जर (pos + 1 == namein.end()) थांबा;
            जर (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } अन्यथा {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            स्थिरांक अचिन्हित val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    परत vname.hashedName();
}

string Ast_गाठ::encode_संख्या(int64_t अंक) {
    जर (अंक < 0) परत "__02D" + cvtToStr(-अंक);  // 2D=-
    परत cvtToStr(अंक);
}

string Ast_गाठ::nameProtect() स्थिरांक { परत VIdProtect::protectIf(name(), protect()); }
string Ast_गाठ::origNameProtect() स्थिरांक { परत VIdProtect::protectIf(origName(), protect()); }

string Ast_गाठ::shortName() स्थिरांक {
    string pretty = name();
    string::size_type pos;
    पर्यंत ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    परत pretty;
}

string Ast_गाठ::dedotName(स्थिरांक string& namein) {
    string pretty = namein;
    string::size_type pos;
    पर्यंत ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    जर (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    परत pretty;
}

string Ast_गाठ::nameNoArray(स्थिरांक string& namein) {
    परत namein.substr(0, namein.find("__BRA__"));
}

string Ast_गाठ::vcdName(स्थिरांक string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    पर्यंत ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    पर्यंत ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    परत prettyName(pretty);
}

string Ast_गाठ::prettyName(स्थिरांक string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    साठी (स्थिरांक अक्षर* pos = namein.c_str(); *pos;) {
        जर (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            सुरूठेवा;
        }
        जर (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            जर (0 == std::म_भेदन(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                सुरूठेवा;
            }
            जर (0 == std::म_भेदन(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                सुरूठेवा;
            }
            जर (0 == std::म_भेदन(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                सुरूठेवा;
            }
            जर (0 == std::म_भेदन(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                सुरूठेवा;
            }
            जर (0 == std::म_भेदन(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                सुरूठेवा;
            }
            जर (0 == std::म_भेदन(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                सुरूठेवा;
            }
            जर (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::है_षष्ठादशक(pos[3])
                && std::है_षष्ठादशक(pos[4])) {
                अक्षर मूल्य = 0;
                मूल्य += 16
                         * (std::है_अंक(pos[3]) ? (pos[3] - '0')
                                                 : (std::छोटे(pos[3]) - 'a' + 10));
                मूल्य
                    += (std::है_अंक(pos[4]) ? (pos[4] - '0') : (std::छोटे(pos[4]) - 'a' + 10));
                pretty += मूल्य;
                pos += 5;
                सुरूठेवा;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    जर (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    जर (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    परत pretty;
}

string Ast_गाठ::vpiName(स्थिरांक string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    स्थिरांक माप_प्रकार offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    तार्किक inEscapedIdent = असत्य;
    पूर्णांक lastIdent = 0;

    साठी (स्थिरांक अक्षर* pos = namein.c_str() + offset; *pos;) {
        अक्षर specialChar = 0;
        जर (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } अन्यथा जर (pos[0] == '_' && pos[1] == '_') {  // __
            जर (0 == std::म_भेदन(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } अन्यथा जर (0 == std::म_भेदन(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } अन्यथा जर (0 == std::म_भेदन(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } अन्यथा जर (0 == std::म_भेदन(pos, "__PVT__", 7)) {
                pos += 7;
                सुरूठेवा;
            } अन्यथा जर (0 == std::म_भेदन(pos, "__0", 3) && std::है_षष्ठादशक(pos[3])
                       && std::है_षष्ठादशक(pos[4])) {
                अक्षर मूल्य = 0;
                मूल्य += 16
                         * (std::है_अंक(pos[3]) ? (pos[3] - '0')
                                                 : (std::छोटे(pos[3]) - 'a' + 10));
                मूल्य
                    += (std::है_अंक(pos[4]) ? (pos[4] - '0') : (std::छोटे(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                जर (मूल्य != '_') inEscapedIdent = सत्य;

                pretty += मूल्य;
                pos += 5;
                सुरूठेवा;
            }
        } अन्यथा जर (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        जर (specialChar) {
            जर (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.घाला(lastIdent, "\\");
                inEscapedIdent = असत्य;
            }

            pretty += specialChar;

            जर (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = असत्य;
            }
        } अन्यथा {
            pretty += pos[0];
            ++pos;
        }
    }
    जर (inEscapedIdent) {
        pretty += " ";
        pretty.घाला(lastIdent, "\\");
    }
    जर (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    जर (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    परत pretty;
}

string Ast_गाठ::prettyTypeName() स्थिरांक {
    जर (name() == "") परत typeName();
    परत std::string{typeName()} + " '" + pretty_Name_संदेश() + "'";
}

//######################################################################
// Insertion

शून्य Ast_गाठ::debug_वृक्ष_Change(स्थिरांक Ast_गाठ* nodep, स्थिरांक अक्षर* prefix, पूर्णांक lineno, तार्किक next) {
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
Ast_गाठ* Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(Ast_गाठ* nodep, Ast_गाठ* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_वृक्ष_Change(nodep, "-addNextThs: ", __LINE__, असत्य);
    debug_वृक्ष_Change(newp, "-addNextNew: ", __LINE__, सत्य);
    जर (!nodep)  // verilog.y and lots of other places assume this
        परत newp;

    // Find end of old list
    Ast_गाठ* oldtailp = nodep;
    जर (oldtailp->m_nextp) {
        जर (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } अन्यथा {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            पर्यंत (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_गाठ* स्थिरांक newtailp = newp->m_headtailp;
    Ast_गाठ* स्थिरांक headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_मोजणी_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_वृक्ष_Change(nodep, "-addNextOut:", __LINE__, सत्य);
    परत nodep;
}

शून्य Ast_गाठ::addNextHere(Ast_गाठ* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_वृक्ष_Change(हे, "-addHereThs: ", __LINE__, असत्य);
    debug_वृक्ष_Change(newp, "-addHereNew: ", __LINE__, सत्य);
    newp->edit_मोजणी_Inc();

    Ast_गाठ* स्थिरांक addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_गाठ* स्थिरांक oldnextp = हे->m_nextp;
    हे->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    जर (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = हे;

    // Head/tail
    Ast_गाठ* स्थिरांक oldheadtailp = हे->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    जर (oldheadtailp) {  // else in middle of list, no change
        जर (oldheadtailp == हे) {  // this was one node
            हे->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } अन्यथा जर (!oldnextp) {  // this was tail
            हे->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_वृक्ष_Change(हे, "-addHereOut: ", __LINE__, सत्य);
}

शून्य Ast_गाठ::setOp1p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, हे, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_वृक्ष_Change(हे, "-setOp1pThs: ", __LINE__, असत्य);
    debug_वृक्ष_Change(newp, "-setOp1pNew: ", __LINE__, सत्य);
    m_op1p = newp;
    newp->edit_मोजणी_Inc();
    newp->m_backp = हे;
    debug_वृक्ष_Change(हे, "-setOp1pOut: ", __LINE__, असत्य);
}

शून्य Ast_गाठ::setOp2p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, हे, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_वृक्ष_Change(हे, "-setOp2pThs: ", __LINE__, असत्य);
    debug_वृक्ष_Change(newp, "-setOp2pNew: ", __LINE__, सत्य);
    m_op2p = newp;
    newp->edit_मोजणी_Inc();
    newp->m_backp = हे;
    debug_वृक्ष_Change(हे, "-setOp2pOut: ", __LINE__, असत्य);
}

शून्य Ast_गाठ::setOp3p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, हे, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_वृक्ष_Change(हे, "-setOp3pThs: ", __LINE__, असत्य);
    debug_वृक्ष_Change(newp, "-setOp3pNew: ", __LINE__, सत्य);
    m_op3p = newp;
    newp->edit_मोजणी_Inc();
    newp->m_backp = हे;
    debug_वृक्ष_Change(हे, "-setOp3pOut: ", __LINE__, असत्य);
}

शून्य Ast_गाठ::setOp4p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, हे, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_वृक्ष_Change(हे, "-setOp4pThs: ", __LINE__, असत्य);
    debug_वृक्ष_Change(newp, "-setOp4pNew: ", __LINE__, सत्य);
    m_op4p = newp;
    newp->edit_मोजणी_Inc();
    newp->m_backp = हे;
    debug_वृक्ष_Change(हे, "-setOp4pOut: ", __LINE__, असत्य);
}

शून्य Ast_गाठ::addOp1p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जर (!m_op1p) {
        op1p(newp);
    } अन्यथा {
        m_op1p->addNext(newp);
    }
}

शून्य Ast_गाठ::addOp2p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जर (!m_op2p) {
        op2p(newp);
    } अन्यथा {
        m_op2p->addNext(newp);
    }
}

शून्य Ast_गाठ::addOp3p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जर (!m_op3p) {
        op3p(newp);
    } अन्यथा {
        m_op3p->addNext(newp);
    }
}

शून्य Ast_गाठ::addOp4p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जर (!m_op4p) {
        op4p(newp);
    } अन्यथा {
        m_op4p->addNext(newp);
    }
}

शून्य Ast_गाठ::prependOp1p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जर (!m_op1p) {
        op1p(newp);
    } अन्यथा {
        m_op1p->addHereThisAsNext(newp);
    }
}

शून्य Ast_गाठ::prependOp2p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जर (!m_op2p) {
        op2p(newp);
    } अन्यथा {
        m_op2p->addHereThisAsNext(newp);
    }
}

शून्य Ast_गाठ::prependOp3p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जर (!m_op3p) {
        op3p(newp);
    } अन्यथा {
        m_op3p->addHereThisAsNext(newp);
    }
}

शून्य Ast_गाठ::prependOp4p(Ast_गाठ* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जर (!m_op4p) {
        op4p(newp);
    } अन्यथा {
        m_op4p->addHereThisAsNext(newp);
    }
}

शून्य Ast_गाठ::replaceWith(Ast_गाठ* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    हे->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
शून्य Ast_गाठ::replaceWithKeepDType(Ast_गाठ* newp) {
    newp->dtypeFrom(हे);
    replaceWith(newp);
}

शून्य VNRelinker::dump(std::ostream& str) स्थिरांक {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_गाठ* Ast_गाठ::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_वृक्ष_Change(हे, "-unlinkWNextThs: ", __LINE__, सत्य);
    Ast_गाठ* स्थिरांक oldp = हे;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_मोजणी_Inc();
    // cppcheck-suppress shadowFunction
    Ast_गाठ* स्थिरांक backp = oldp->m_backp;
    जर (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        जर (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } अन्यथा जर (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } अन्यथा जर (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } अन्यथा जर (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } अन्यथा जर (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } अन्यथा {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    जर (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_गाठ* oldtailp = backp->m_headtailp;
        जर (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            पर्यंत (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_गाठ* स्थिरांक oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } अन्यथा जर (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } अन्यथा जर (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } अन्यथा जर (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } अन्यथा जर (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } अन्यथा {
        हे->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    जर (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_वृक्ष_Change(oldp, "-unlinkWNextOut: ", __LINE__, सत्य);
    परत oldp;
}

Ast_गाठ* Ast_गाठ::unlinkFrBack(VNRelinker* linkerp) {
    debug_वृक्ष_Change(हे, "-unlinkFrBkThs: ", __LINE__, सत्य);
    Ast_गाठ* स्थिरांक oldp = हे;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_मोजणी_Inc();
    // cppcheck-suppress shadowFunction
    Ast_गाठ* स्थिरांक backp = oldp->m_backp;
    जर (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        जर (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        जर (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } अन्यथा जर (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } अन्यथा जर (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } अन्यथा जर (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } अन्यथा जर (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } अन्यथा {
            हे->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    जर (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        जर (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        जर (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } अन्यथा {
        जर (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } अन्यथा जर (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } अन्यथा जर (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } अन्यथा जर (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } अन्यथा {
            हे->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        जर (oldp->m_nextp) {
            Ast_गाठ* स्थिरांक newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    जर (oldp->m_iterpp) {  // Only if no linker, point to next in list
        जर (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_वृक्ष_Change(oldp, "-unlinkFrBkOut: ", __LINE__, सत्य);
    परत oldp;
}

शून्य Ast_गाठ::relink(VNRelinker* linkerp) {
    जर (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_गाठ* स्थिरांक newp = हे;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_मोजणी_Inc();

    जर (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_गाठ* स्थिरांक backp = linkerp->m_backp;
    debug_वृक्ष_Change(हे, "-relinkNew: ", __LINE__, सत्य);
    debug_वृक्ष_Change(backp, "-relinkTre: ", __LINE__, सत्य);

    निवड (linkerp->m_chg) {
    स्थिती VNRelinker::RELINK_NEXT: backp->addNextHere(newp); थांबा;
    स्थिती VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); थांबा;
    स्थिती VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); थांबा;
    स्थिती VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); थांबा;
    स्थिती VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); थांबा;
    पूर्वनिर्धारित: हे->v3fatalSrc("Relink of node without any link to change."); थांबा;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    जर (linkerp->m_iterpp) {
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
    debug_वृक्ष_Change(हे, "-relinkOut: ", __LINE__, सत्य);
}

शून्य Ast_गाठ::relinkOneLink(Ast_गाठ*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_गाठ* newp) {
    जर (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_गाठ* स्थिरांक newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_गाठ* स्थिरांक oldlistlastp = pointpr->m_headtailp;
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

शून्य Ast_गाठ::addHereThisAsNext(Ast_गाठ* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(हे->m_backp, हे, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_गाठ* स्थिरांक backp = हे->m_backp;
    Ast_गाठ* स्थिरांक newLastp = newp->m_headtailp;
    //
    हे->edit_मोजणी_Inc();
    // Common linkage
    newLastp->m_nextp = हे;
    हे->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    जर (backp && backp->m_nextp == हे) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } अन्यथा {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        जर (backp) {
            जर (backp->m_op1p == हे) {
                backp->m_op1p = newp;
            } अन्यथा जर (backp->m_op2p == हे) {
                backp->m_op2p = newp;
            } अन्यथा जर (backp->m_op3p == हे) {
                backp->m_op3p = newp;
            } अन्यथा {
                UASSERT_OBJ(backp->m_op4p == हे, हे, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_गाठ* स्थिरांक tailp = हे->m_headtailp;
        हे->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    जर (newLastp->m_iterpp) *(newLastp->m_iterpp) = हे;
    जर (हे->m_iterpp) {
        *(हे->m_iterpp) = newp;
        हे->m_iterpp = nullptr;
    }
    //
    debug_वृक्ष_Change(हे, "-addHereThisAsNext: ", __LINE__, सत्य);
}

//======================================================================
// Clone

Ast_गाठ* Ast_गाठ::clone_वृक्ष_Iter(तार्किक needPure) {
    // private: Clone single node and children
    जर (needPure) purityCheck();
    Ast_गाठ* स्थिरांक newp = हे->clone();
    जर (हे->m_op1p) newp->op1p(हे->m_op1p->clone_वृक्ष_Iter_List(needPure));
    जर (हे->m_op2p) newp->op2p(हे->m_op2p->clone_वृक्ष_Iter_List(needPure));
    जर (हे->m_op3p) newp->op3p(हे->m_op3p->clone_वृक्ष_Iter_List(needPure));
    जर (हे->m_op4p) newp->op4p(हे->m_op4p->clone_वृक्ष_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(हे);  // Save pointers to/from both to simplify relinking.
    हे->clonep(newp);  // Save pointers to/from both to simplify relinking.
    परत newp;
}

Ast_गाठ* Ast_गाठ::clone_वृक्ष_Iter_List(तार्किक needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_गाठ* newheadp = nullptr;
    Ast_गाठ* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    साठी (Ast_गाठ* oldp = हे; oldp; oldp = oldp->m_nextp) {
        Ast_गाठ* स्थिरांक newp = oldp->clone_वृक्ष_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        जर (newtailp) newtailp->m_nextp = newp;
        जर (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    परत newheadp;
}

Ast_गाठ* Ast_गाठ::clone_वृक्ष(तार्किक cloneNextLink, तार्किक needPure) {
    debug_वृक्ष_Change(हे, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_वृक्ष();
    Ast_गाठ* newp;
    जर (cloneNextLink && हे->m_nextp) {
        newp = clone_वृक्ष_Iter_List(needPure);
    } अन्यथा {
        newp = clone_वृक्ष_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_वृक्ष();
    debug_वृक्ष_Change(newp, "-cloneOut: ", __LINE__, सत्य);
    परत newp;
}

शून्य Ast_गाठ::purityCheck() {
    जर (VL_UNLIKELY(!isPure())) {
        हे->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << हे->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

शून्य Ast_गाठ::delete_गाठ() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_मोजणी_Inc();
    // Change links of old node so we coredump if used
    हे->m_nextp = reinterpret_cast<Ast_गाठ*>(0x1);
    हे->m_backp = reinterpret_cast<Ast_गाठ*>(0x1);
    हे->m_headtailp = reinterpret_cast<Ast_गाठ*>(0x1);
    हे->m_op1p = reinterpret_cast<Ast_गाठ*>(0x1);
    हे->m_op2p = reinterpret_cast<Ast_गाठ*>(0x1);
    हे->m_op3p = reinterpret_cast<Ast_गाठ*>(0x1);
    हे->m_op4p = reinterpret_cast<Ast_गाठ*>(0x1);
    हे->m_iterpp = reinterpret_cast<Ast_गाठ**>(0x1);
    जर (
#जर !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        सत्य
#अन्यथा
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete हे;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

शून्य Ast_गाठ::delete_वृक्ष_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    साठी (Ast_गाठ *nodep = हे, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        जर (nodep->m_op1p) nodep->m_op1p->delete_वृक्ष_Iter();
        जर (nodep->m_op2p) nodep->m_op2p->delete_वृक्ष_Iter();
        जर (nodep->m_op3p) nodep->m_op3p->delete_वृक्ष_Iter();
        जर (nodep->m_op4p) nodep->m_op4p->delete_वृक्ष_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_गाठ();
    }
}

शून्य Ast_गाठ::delete_वृक्ष() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_वृक्ष_Change(हे, "-delTree:  ", __LINE__, सत्य);
    हे->edit_मोजणी_Inc();
    // MUST be depth first!
    delete_वृक्ष_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
शून्य* Ast_गाठ::चालक नवीन(माप_प्रकार size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_गाठ* स्थिरांक objp = static_cast<Ast_गाठ*>(::चालक नवीन(size));
    V3Broken::addNewed(objp);
    परत objp;
}

शून्य Ast_गाठ::चालक delete(शून्य* objp, माप_प्रकार size) {
    जर (!objp) परत;
    स्थिरांक Ast_गाठ* स्थिरांक nodep = static_cast<Ast_गाठ*>(objp);
    V3Broken::deleted(nodep);
    ::चालक delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
शून्य* Ast_गाठ::चालक नवीन(माप_प्रकार size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    स्थिर स्थिरांक माप_प्रकार ASTGEN_MAX_गाठ_SIZE = []() {
        माप_प्रकार maxSize = 0;
        साठी (माप_प्रकार t = 0; t < VNType::अंक_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        परत maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr माप_प्रकार POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    स्थिर uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    जर (पूर्णांक seed = v3Global.opt.debugAllocRandom()) {
        स्थिर V3Mutex s_mutex;
        स्थिरांक V3LockGuard lock{s_mutex};
        स्थिर std::array<शून्य*, POOL_SIZE> s_गाठ_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_गाठ_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        जर (!s_lfsr) {
            s_lfsr = seed;
            जर (!s_lfsr) s_lfsr = ~s_lfsr;
            साठी (माप_प्रकार i = 0; i < POOL_SIZE; ++i) {
                s_गाठ_Pool[i] = ::चालक नवीन(ASTGEN_MAX_गाठ_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_गाठ_Pool), std::end(s_गाठ_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        स्थिरांक माप_प्रकार index = s_lfsr % POOL_SIZE;
        Ast_गाठ* स्थिरांक objp = static_cast<Ast_गाठ*>(s_गाठ_Pool[index]);
        s_गाठ_Pool[index] = ::चालक नवीन(ASTGEN_MAX_गाठ_SIZE + 64);  // For later new()
        परत objp;
    } अन्यथा {
        Ast_गाठ* स्थिरांक objp = static_cast<Ast_गाठ*>(::चालक नवीन(size));
        परत objp;
    }
}
शून्य Ast_गाठ::चालक delete(शून्य* objp, माप_प्रकार size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (शून्य)objp;
    (शून्य)size;
}
#endif

//======================================================================
// Iterators

शून्य Ast_गाठ::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    जर (m_op1p) m_op1p->iterateAndNext(v);
    जर (m_op2p) m_op2p->iterateAndNext(v);
    जर (m_op3p) m_op3p->iterateAndNext(v);
    जर (m_op4p) m_op4p->iterateAndNext(v);
}

शून्य Ast_गाठ::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    जर (m_op1p) m_op1p->iterateAndNextConst(v);
    जर (m_op2p) m_op2p->iterateAndNextConst(v);
    जर (m_op3p) m_op3p->iterateAndNextConst(v);
    जर (m_op4p) m_op4p->iterateAndNextConst(v);
}

शून्य Ast_गाठ::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_गाठ* nodep = हे;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    जर (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    पर्यंत (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        जर (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_गाठ* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        जर (!niterp) परत;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        जर (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } अन्यथा {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

शून्य Ast_गाठ::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_गाठ* nodep = हे;
    पर्यंत (nodep->m_nextp) nodep = nodep->m_nextp;
    पर्यंत (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        जर (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } अन्यथा {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

शून्य Ast_गाठ::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    जर (m_op1p) m_op1p->iterateListBackwardsConst(v);
    जर (m_op2p) m_op2p->iterateListBackwardsConst(v);
    जर (m_op3p) m_op3p->iterateListBackwardsConst(v);
    जर (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

शून्य Ast_गाठ::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_गाठ* nodep = हे;
    करा {
        Ast_गाठ* स्थिरांक nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } पर्यंत (nodep);
}

Ast_गाठ* Ast_गाठ::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_गाठ* nodep = हे;  // Note "this" may point to bogus point later in this function
    जर (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } अन्यथा जर (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* स्थिरांक tempp = नवीन AstBegin{nodep->fileline(), "[EditWrapper]", nodep, असत्य};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_वृक्ष(), tempp);
    } अन्यथा {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_गाठ** nextnodepp = nullptr;
        जर (हे->m_backp->m_op1p == हे) {
            nextnodepp = &(हे->m_backp->m_op1p);
        } अन्यथा जर (हे->m_backp->m_op2p == हे) {
            nextnodepp = &(हे->m_backp->m_op2p);
        } अन्यथा जर (हे->m_backp->m_op3p == हे) {
            nextnodepp = &(हे->m_backp->m_op3p);
        } अन्यथा जर (हे->m_backp->m_op4p == हे) {
            nextnodepp = &(हे->m_backp->m_op4p);
        } अन्यथा जर (हे->m_backp->m_nextp == हे) {
            nextnodepp = &(हे->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, हे, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    परत nodep;
}

//======================================================================

शून्य Ast_गाठ::clone_Relink_वृक्ष() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    साठी (Ast_गाठ* nodep = हे; nodep; nodep = nodep->m_nextp) {
        जर (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        जर (nodep->m_op1p) nodep->m_op1p->clone_Relink_वृक्ष();
        जर (nodep->m_op2p) nodep->m_op2p->clone_Relink_वृक्ष();
        जर (nodep->m_op3p) nodep->m_op3p->clone_Relink_वृक्ष();
        जर (nodep->m_op4p) nodep->m_op4p->clone_Relink_वृक्ष();
    }
}

//======================================================================
// Comparison

तार्किक Ast_गाठ::gate_वृक्ष_Iter() स्थिरांक {
    // private: Return true if the two trees are identical
    जर (!isGateOptimizable()) परत असत्य;
    जर (m_op1p && !m_op1p->gate_वृक्ष_Iter()) परत असत्य;
    जर (m_op2p && !m_op2p->gate_वृक्ष_Iter()) परत असत्य;
    जर (m_op3p && !m_op3p->gate_वृक्ष_Iter()) परत असत्य;
    जर (m_op4p && !m_op4p->gate_वृक्ष_Iter()) परत असत्य;
    परत सत्य;
}

तार्किक Ast_गाठ::same_वृक्ष_Iter(स्थिरांक Ast_गाठ* node1p, स्थिरांक Ast_गाठ* node2p, तार्किक ignNext,
                           तार्किक gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    पर्यंत (सत्य) {
        जर (!node1p && !node2p) परत सत्य;
        जर (!node1p || !node2p) परत असत्य;
        जर (node1p->type() != node2p->type()) परत असत्य;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        जर (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) परत असत्य;
        जर (!node1p->same_गाठ(node2p) || (gateOnly && !node1p->isGateOptimizable())) परत असत्य;
        जर (!same_वृक्ष_Iter(node1p->m_op1p, node2p->m_op1p, असत्य, gateOnly)
            || !same_वृक्ष_Iter(node1p->m_op2p, node2p->m_op2p, असत्य, gateOnly)
            || !same_वृक्ष_Iter(node1p->m_op3p, node2p->m_op3p, असत्य, gateOnly)
            || !same_वृक्ष_Iter(node1p->m_op4p, node2p->m_op4p, असत्य, gateOnly)) {
            परत असत्य;
        }
        जर (ignNext) परत सत्य;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

शून्य Ast_गाठ::check_वृक्ष_Iter(स्थिरांक Ast_गाठ* prevBackp) स्थिरांक VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == हे->backp(), हे, "Back node inconsistent");
    स्थिरांक VNTypeInfo& typeInfo = VNType::typeInfo(हे->type());
    साठी (पूर्णांक i = 1; i <= 4; i++) {
        Ast_गाठ* nodep = nullptr;
        निवड (i) {
        स्थिती 1: nodep = op1p(); थांबा;
        स्थिती 2: nodep = op2p(); थांबा;
        स्थिती 3: nodep = op3p(); थांबा;
        स्थिती 4: nodep = op4p(); थांबा;
        पूर्वनिर्धारित: हे->v3fatalSrc("Bad case"); थांबा;
        }
        // cppcheck-suppress danglingTempReference
        स्थिरांक अक्षर* opName = typeInfo.m_opNamep[i - 1];
        निवड (typeInfo.m_opType[i - 1]) {
        स्थिती VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, हे, typeInfo.m_namep << " must not use " << opName << "()");
            थांबा;
        स्थिती VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, हे,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), हे,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_वृक्ष_Iter(हे);
            थांबा;
        स्थिती VNTypeInfo::OP_LIST:
            जर (स्थिरांक Ast_गाठ* स्थिरांक headp = nodep) {
                // cppcheck-suppress shadowFunction
                स्थिरांक Ast_गाठ* backp = हे;
                स्थिरांक Ast_गाठ* tailp;
                स्थिरांक Ast_गाठ* opp = headp;
                करा {
                    opp->check_वृक्ष_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } पर्यंत (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            थांबा;
        स्थिती VNTypeInfo::OP_OPTIONAL:
            जर (nodep) {
                UASSERT_OBJ(!nodep->nextp(), हे,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_वृक्ष_Iter(हे);
            }
            थांबा;
        पूर्वनिर्धारित: हे->v3fatalSrc("Bad case"); थांबा;
        }
    }
    जर (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        जर (स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = VN_CAST(हे, गाठ_अभिव्यक्ती)) {
            स्थिरांक अक्षर* स्थिरांक whyp = exprp->widthMismatch();
            जर (whyp) {
                स्वतः dtypeStr = [](स्थिरांक Ast_गाठ_अभिव्यक्ती* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    परत ss.str();
                };
                जर (स्थिरांक Ast_गाठ_Uniop* स्थिरांक uniopp = VN_CAST(exprp, गाठ_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } अन्यथा जर (स्थिरांक Ast_गाठ_Biop* स्थिरांक biopp = VN_CAST(exprp, गाठ_Biop)) {
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
अक्षर* Ast_गाठ::dump_वृक्ष_Json_Gdb(स्थिरांक Ast_गाठ* nodep) {
    जर (!nodep) परत strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_वृक्ष_Json(nodepStream);
    स्थिरांक std::string str = nodepStream.rdbuf()->str();
    परत strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
अक्षर* Ast_गाठ::dump_वृक्ष_Json_Gdb(स्थिरांक अक्षर* str) { परत strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
अक्षर* Ast_गाठ::dump_वृक्ष_Json_Gdb(intptr_t nodep) {
    जर (!nodep) परत strdup("{\"addr\":\"NULL\"}\n");
    परत dump_वृक्ष_Json_Gdb(reinterpret_cast<स्थिरांक Ast_गाठ*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाठ::dumpGdb(स्थिरांक Ast_गाठ* nodep) {  // For GDB only  // LCOV_EXCL_START
    जर (!nodep) {
        cout << "<nullptr>" << endl;
        परत;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाठ::dumpGdbHeader() स्थिरांक {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाठ::dump_वृक्ष_Gdb(स्थिरांक Ast_गाठ* nodep) {  // For GDB only  // LCOV_EXCL_START
    जर (!nodep) {
        cout << "<nullptr>" << endl;
        परत;
    }
    nodep->dumpGdbHeader();
    nodep->dump_वृक्ष(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाठ::dump_वृक्ष_File_Gdb(स्थिरांक Ast_गाठ* nodep,  // LCOV_EXCL_START
                              स्थिरांक अक्षर* filenamep) {  // For GDB only
    जर (!nodep) {
        cout << "<nullptr>" << endl;
        परत;
    }
    स्थिरांक string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_वृक्ष_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_गाठ::checkIter() स्थिरांक {
    जर (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        हे->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

शून्य Ast_गाठ::dumpPtrs(std::ostream& os) स्थिरांक {
    os << "This=" << typeName() << " " << cvtToHex(हे);
    os << " back=" << cvtToHex(backp());
    जर (nextp()) os << " next=" << cvtToHex(nextp());
    जर (m_headtailp == हे) {
        os << " headtail=this";
    } अन्यथा {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    जर (op1p()) os << " op1p=" << cvtToHex(op1p());
    जर (op2p()) os << " op2p=" << cvtToHex(op2p());
    जर (op3p()) os << " op3p=" << cvtToHex(op3p());
    जर (op4p()) os << " op4p=" << cvtToHex(op4p());
    स्थिरांक स्वतः dumpUser = [&os](स्थिरांक अक्षर* prefix, स्थिरांक VNUser& user) {
        स्थिरांक std::string s
            = user.dumpStr([](स्थिरांक शून्य* p) -> std::string { परत cvtToHex(p); });
        जर (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    जर (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

शून्य Ast_गाठ::dump_वृक्ष(std::ostream& os, स्थिरांक string& indent, पूर्णांक maxDepth) स्थिरांक {
    स्थिर पूर्णांक s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << हे << '\n';
    जर (VN_DELETED(हे)) परत;
    जर (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    जर (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(हे)
           << endl;  // endl intentional to do flush
        परत;
    }  // LCOV_EXCL_STOP
    जर (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    जर (maxDepth == 1) {
        जर (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } अन्यथा {
        साठी (स्थिरांक Ast_गाठ* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्ष(os, indent + "1:", maxDepth - 1);
        }
        साठी (स्थिरांक Ast_गाठ* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्ष(os, indent + "2:", maxDepth - 1);
        }
        साठी (स्थिरांक Ast_गाठ* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्ष(os, indent + "3:", maxDepth - 1);
        }
        साठी (स्थिरांक Ast_गाठ* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्ष(os, indent + "4:", maxDepth - 1);
        }
    }
}

शून्य Ast_गाठ::dump_वृक्ष_And_Next(std::ostream& os, स्थिरांक string& indent, पूर्णांक maxDepth) स्थिरांक {
    // Audited to make sure this is never nullptr
    साठी (स्थिरांक Ast_गाठ* nodep = हे; nodep; nodep = nodep->nextp()) {
        nodep->dump_वृक्ष(os, indent, maxDepth);
    }
}

शून्य Ast_गाठ::dump_वृक्ष_File(स्थिरांक string& filename, तार्किक doDump) {
    // Not const function as calls checkTree
    जर (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            स्थिरांक std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            जर (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_मोजणी_Last();
            *logsp << "> to <e" << std::dec << edit_मोजणी_Gbl() << ">\n";
            जर (edit_मोजणी_Gbl() == edit_मोजणी_Last() && ::dump_वृक्ष_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } अन्यथा {
                dump_वृक्ष(*logsp);
                edit_मोजणी_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

स्थिर शून्य drawChildren(std::ostream& os, स्थिरांक Ast_गाठ* thisp, स्थिरांक Ast_गाठ* childp,
                         स्थिरांक std::string& childName) {
    जर (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        साठी (स्थिरांक Ast_गाठ* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्ष_Dot(os);
            जर (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

शून्य Ast_गाठ::dump_वृक्ष_Dot(std::ostream& os) स्थिरांक {
    os << "\tn" << cvtToHex(हे) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, हे, m_op1p, "op1");
    drawChildren(os, हे, m_op2p, "op2");
    drawChildren(os, हे, m_op3p, "op3");
    drawChildren(os, हे, m_op4p, "op4");
}

शून्य Ast_गाठ::dump_वृक्ष_Json_File(स्थिरांक string& filename, तार्किक doDump) {
    जर (!doDump) परत;
    UINFO(2, "Dumping " << filename);
    स्थिरांक std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    जर (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_वृक्ष_Json(*treejsonp);
    *treejsonp << '\n';
}

शून्य Ast_गाठ::dumpJsonMetaFileGdb(स्थिरांक अक्षर* filename) { dumpJsonMetaFile(filename); }
शून्य Ast_गाठ::dumpJsonMetaFile(स्थिरांक string& filename) {
    UINFO(2, "Dumping " << filename);
    स्थिरांक std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    जर (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_ओळ::file_Name_अंक_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

शून्य Ast_गाठ::dump_वृक्ष_Dot_File(स्थिरांक string& filename, तार्किक doDump) {
    जर (doDump) {
        UINFO(2, "Dumping " << filename);
        स्थिरांक std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        जर (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_वृक्ष_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_गाठ::instanceStr() स्थिरांक {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr अचिन्हित maxIterations = 10000;
    अचिन्हित iter_मोजणी = 0;
    // Walk 'backp' chain
    साठी (स्थिरांक Ast_गाठ* currp = हे; currp; currp = currp->backp(), ++iter_मोजणी) {
        जर (VL_UNCOVERABLE(iter_मोजणी >= maxIterations)) परत "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        जर (स्थिरांक AstScope* स्थिरांक scopep = VN_CAST(currp, Scope)) {
            परत scopep->is_शीर्ष() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        जर (स्थिरांक AstModule* स्थिरांक modp = VN_CAST(currp, Module)) {
            स्थिरांक string instanceName = modp->someInstanceName();
            परत instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    परत "";
}
शून्य Ast_गाठ::v3errorEnd(स्थिरांक std::ostringstream& str) स्थिरांक VL_RELEASE(V3Error::s().m_mutex) {
    जर (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } अन्यथा {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        स्थिरांक string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        जर (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_गाठ*>(हे)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
शून्य Ast_गाठ::v3errorEndFatal(स्थिरांक std::ostringstream& str) स्थिरांक
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

शून्य Ast_गाठ::dtypeChgSigned(तार्किक flag) {
    UASSERT_OBJ(dtypep(), हे, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
शून्य Ast_गाठ::dtypeChgWidth(पूर्णांक width, पूर्णांक widthMin) {
    UASSERT_OBJ(dtypep(), हे, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

शून्य Ast_गाठ::dtypeChgWidthSigned(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), हे, "No dtype when changing width");
    स्थिरांक Ast_गाठ_DType* स्थिरांक enumDTypep = dtypep()->skipRefToEnump();
    जर (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        परत;  // Correct already
    }
    स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_गाठ_DType* Ast_गाठ::findBasicDType(VBasicDTypeKwd kwd) स्थिरांक {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    परत v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_गाठ_DType* Ast_गाठ::findBitDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_गाठ_DType* Ast_गाठ::findLogicDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_गाठ_DType* Ast_गाठ::findLogic2StateDType(पूर्णांक width, पूर्णांक widthMin, VSigning numeric) स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_गाठ_DType* Ast_गाठ::findLogicRangeDType(स्थिरांक VNumRange& range, पूर्णांक widthMin,
                                           VSigning numeric) स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_गाठ_DType* Ast_गाठ::findBitRangeDType(स्थिरांक VNumRange& range, पूर्णांक widthMin,
                                         VSigning numeric) स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_गाठ::find_घाला_Same_DType(AstBasicDType* nodep) {
    परत v3Global.rootp()->typeTablep()->find_घाला_Same_DType(nodep);
}
Ast_गाठ_DType* Ast_गाठ::findConstraintRefDType() स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_गाठ_DType* Ast_गाठ::findEmptyQueueDType() स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_गाठ_DType* Ast_गाठ::findQueueIndexDType() स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_गाठ_DType* Ast_गाठ::findStreamDType() स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_गाठ_DType* Ast_गाठ::findVoidDType() स्थिरांक {
    परत v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

स्थिर स्थिरांक Ast_गाठ_DType* computeCastableBase(स्थिरांक Ast_गाठ_DType* nodep) {
    पर्यंत (सत्य) {
        जर (स्थिरांक AstPackArrayDType* स्थिरांक packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            सुरूठेवा;
        }
        जर (स्थिरांक Ast_गाठ_DType* स्थिरांक refp = nodep->skipRefToEnump()) {
            जर (refp != nodep) {
                nodep = refp;
                सुरूठेवा;
            }
        }
        परत nodep;
    }
}

स्थिर VCastable computeCastableImp(स्थिरांक Ast_गाठ_DType* toDtp, स्थिरांक Ast_गाठ_DType* fromDtp,
                                    स्थिरांक Ast_गाठ* fromConstp, स्थिरांक तार्किक checkIfaceArgCompat) {
    स्थिरांक VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    जर (toDtp == fromDtp) परत VCastable::SAMEISH;
    जर (toDtp->similarDType(fromDtp)) परत VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    स्थिरांक Ast_गाठ_DType* fromBaseDtp = computeCastableBase(fromDtp);

    स्थिरांक तार्किक fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, गाठ_UOr_Struct_DType);

    स्थिरांक Ast_गाठ_DType* toBaseDtp = computeCastableBase(toDtp);
    स्थिरांक तार्किक toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, गाठ_UOr_Struct_DType);

    जर (toBaseDtp == fromBaseDtp) परत VCastable::COMPATIBLE;

    जर (toNumericable) {
        जर (fromNumericable) परत VCastable::COMPATIBLE;
    } अन्यथा जर (VN_IS(toBaseDtp, EnumDType)) {
        जर (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_वृक्ष(fromDtp))
            परत VCastable::ENUM_IMPLICIT;
        जर (fromNumericable) परत VCastable::ENUM_EXPLICIT;
    } अन्यथा जर (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        परत VCastable::COMPATIBLE;
    } अन्यथा जर (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        जर (fromConstp->isNull()) परत VCastable::COMPATIBLE;
    } अन्यथा जर (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        स्थिरांक AstClass* स्थिरांक toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        स्थिरांक AstClass* स्थिरांक fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        स्थिरांक तार्किक downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        स्थिरांक तार्किक upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        जर (upcast) परत VCastable::COMPATIBLE;
        जर (downcast) परत VCastable::DYNAMIC_CLASS;
        परत VCastable::INCOMPATIBLE;
    } अन्यथा जर (स्थिरांक AstIfaceRefDType* स्थिरांक toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(सत्य) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        स्थिरांक AstIfaceRefDType* स्थिरांक fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(सत्य) : fromDtp, IfaceRefDType);
        जर (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            स्थिरांक AstUnpackArrayDType* स्थिरांक toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            स्थिरांक AstUnpackArrayDType* स्थिरांक fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            जर (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                परत VCastable::INCOMPATIBLE;
            }
            परत computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        जर (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            जर (!checkIfaceArgCompat) परत castable;
            परत VCastable::INCOMPATIBLE;
        }
        स्थिरांक तार्किक sameModport = toIfp->modportp() == fromIfp->modportp();
        जर (!checkIfaceArgCompat) {
            जर (!toIfp->modportp() || sameModport) परत VCastable::COMPATIBLE;
            परत castable;
        }
        जर (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            परत VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        जर (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            परत VCastable::COMPATIBLE;
        }
        परत VCastable::INCOMPATIBLE;
    } अन्यथा जर (स्थिरांक AstUnpackArrayDType* स्थिरांक toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        स्थिरांक AstUnpackArrayDType* स्थिरांक fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        जर (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            परत castable;
        }
        स्थिरांक VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        स्थिरांक Ast_गाठ_DType* स्थिरांक toSubp = toArrayp->subDTypep()->skipRefp();
        जर (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            परत VCastable::COMPATIBLE;
        }
    }
    परत castable;
}

VCastable Ast_गाठ::computeCastable(स्थिरांक Ast_गाठ_DType* toDtp, स्थिरांक Ast_गाठ_DType* fromDtp,
                                   स्थिरांक Ast_गाठ* fromConstp, स्थिरांक तार्किक checkIfaceArgCompat) {
    स्थिरांक स्वतः castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    जर (fromConstp) UINFO(9, "     const= " << fromConstp);
    परत castable;
}

Ast_गाठ_DType* Ast_गाठ::getCommonClassTypep(Ast_गाठ* node1p, Ast_गाठ* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    जर (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        स्थिरांक VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        जर (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            परत node1p->dtypep();
        जर (castable == VCastable::DYNAMIC_CLASS) परत node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    पर्यंत (classDtypep1) {
        स्थिरांक VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        जर (castable == VCastable::COMPATIBLE) परत classDtypep1;
        AstClassExtends* स्थिरांक extendsp = classDtypep1->classp()->extendsp();
        जर (!extendsp) थांबा;
        Ast_गाठ_DType* स्थिरांक edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    परत nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

वर्ग VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<पूर्णांक, std::string> m_internedWordWidths;  // Interned widths
    std::map<पूर्णांक, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    स्थिरांक तार्किक m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    स्थिर std::string toLetters(माप_प्रकार मूल्य, तार्किक lowerCase = असत्य) {
        स्थिरांक अक्षर base = lowerCase ? 'a' : 'A';
        std::string s;
        करा { s += static_cast<अक्षर>(base + मूल्य % 26); } पर्यंत (मूल्य /= 26);
        परत s;
    }

    स्थिरांक std::string& internConst(स्थिरांक AstConst& nodep) {
        स्थिरांक स्वतः pair = m_internedConsts.emplace(nodep.अंक().ascii(असत्य), "");
        जर (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        परत pair.first->second;
    }

    स्थिरांक std::string& internWordWidth(पूर्णांक मूल्य) {
        स्थिरांक स्वतः pair = m_internedWordWidths.emplace(मूल्य, "");
        जर (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, सत्य);
        परत pair.first->second;
    }

    स्थिरांक std::string& internWideWidth(पूर्णांक मूल्य) {
        स्थिरांक स्वतः pair = m_internedWideWidths.emplace(मूल्य, "");
        जर (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        परत pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    स्थिर तार्किक isLeaf(स्थिरांक Ast_गाठ* nodep) {
        स्थिरांक VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        साठी (स्थिरांक VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            जर (opType != VNTypeInfo::OP_UNUSED) परत असत्य;
        }
        परत सत्य;
    }

    // Render operand, return true if not unused
    शून्य renderOperand(VNTypeInfo::OpEn opType, स्थिरांक Ast_गाठ* स्थिरांक opp, uint32_t depth) {
        निवड (opType) {
        स्थिती VNTypeInfo::OP_UNUSED:  //
            थांबा;
        स्थिती VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            थांबा;
        स्थिती VNTypeInfo::OP_LIST:
            m_os << " [";
            साठी (स्थिरांक Ast_गाठ* nodep = opp; nodep; nodep = nodep->nextp()) {
                जर (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            थांबा;
        स्थिती VNTypeInfo::OP_OPTIONAL:
            जर (opp) {
                m_os << " ";
                render(opp, depth);
            } अन्यथा {
                m_os << " nil";
            }
            थांबा;
        }
    }

    // Render the node into the stream.
    शून्य render(स्थिरांक Ast_गाठ* nodep, uint32_t depth) {
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            जर (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } अन्यथा जर (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } अन्यथा {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } अन्यथा जर (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } अन्यथा जर (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } अन्यथा {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            स्थिरांक VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        जर (!dtypep) {
            m_os << '?';
        } अन्यथा जर (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } अन्यथा {
            स्थिरांक पूर्णांक width = nodep->width();
            जर (width == 1) {
                m_os << '1';
            } अन्यथा जर (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } अन्यथा {
                m_os << internWideWidth(width);
            }
            जर (m_dumpWidthMin) {
                m_os << '/';
                स्थिरांक पूर्णांक widthMin = nodep->widthMin();
                जर (widthMin == 1) {
                    m_os << '1';
                } अन्यथा जर (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } अन्यथा {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

सार्वजनिक:
    VNPatternString(std::ostream& os, स्थिरांक Ast_गाठ* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_गाठ_अभिव्यक्ती::patternString(uint32_t depth) स्थिरांक {
    std::ostringstream oss;
    VNPatternString{oss, हे, depth};
    परत oss.str();
}

//######################################################################
// VNDeleter

शून्य VNDeleter::doDeletes() {
    साठी (Ast_गाठ* स्थिरांक nodep : m_deleteps) nodep->delete_वृक्ष();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#अन्तर्भाव "V3Ast__gen_visitor_defns.h"  // From ./astgen
