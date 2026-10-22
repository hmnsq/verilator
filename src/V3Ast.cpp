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

uint64_t Ast_ग्रन्थिः::s_editCntLast = 0;
uint64_t Ast_ग्रन्थिः::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
पूर्णाङ्क Ast_ग्रन्थिः::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

तार्किक VNUser1InUse::s_userBusy = मिच्छ;
तार्किक VNUser2InUse::s_userBusy = मिच्छ;
तार्किक VNUser3InUse::s_userBusy = मिच्छ;
तार्किक VNUser4InUse::s_userBusy = मिच्छ;

पूर्णाङ्क Ast_ग्रन्थिः_DType::s_unique_अङ्कः = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(ध्रुव string& name) {
    प्रति (पूर्णाङ्क i = 0; i < _ENUM_MAX; ++i) {
        ध्रुव VCMethod method{i};
        जइ (name == method.ascii()) प्रत्यागम method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    प्रत्यागम VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ध्रुव शून्य*)) ध्रुव {
#ifdef VL_USER_TYPE_CHECKS
    जइ (ध्रुव uint64_t* ध्रुव uip = std::get_if<uint64_t>(&m_u)) प्रत्यागम "#"s + cvtToStr(*uip);
    जइ (शून्य* ध्रुव* ध्रुव upp = std::get_if<शून्य*>(&m_u)) प्रत्यागम fmtAddrp(*upp);
    प्रत्यागम "";
#अण्णहा
    // Dumps void* representation
    जइ (!m_u.up) प्रत्यागम "";
    प्रत्यागम fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

ध्रुव VNTypeInfo VNType::s_typeInfoTable[VNType::अङ्कः_TYPES()] = {
#अन्तर्भाव "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

तार्किक VFwdType::is_ग्रन्थिः_Compatible(ध्रुव Ast_ग्रन्थिः* nodep) ध्रुव {
    ध्रुव Ast_ग्रन्थिः* defp = nodep;
    जइ (ध्रुव AstTypedef* ध्रुव adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    जइ (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव adefp = VN_CAST(defp, ग्रन्थिः_DType))
        defp = adefp->skipRefToNonRefp();
    विकल्प (m_e) {
    पक्ष VFwdType::NONE: प्रत्यागम सच्च; भङ्ग;
    पक्ष VFwdType::ENUM: प्रत्यागम VN_IS(defp, EnumDType); भङ्ग;
    पक्ष VFwdType::STRUCT: प्रत्यागम VN_IS(defp, StructDType); भङ्ग;
    पक्ष VFwdType::UNION: प्रत्यागम VN_IS(defp, UnionDType); भङ्ग;
    पक्ष VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    पक्ष VFwdType::CLASS: प्रत्यागम VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); भङ्ग;
    पक्ष VFwdType::GENERIC_INTERFACE: प्रत्यागम VN_IS(defp, IfaceRefDType); भङ्ग;
    स्वभाव: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    प्रत्यागम मिच्छ;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ध्रुव std::shared_ptr<ध्रुव string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
ध्रुव std::shared_ptr<ध्रुव string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(तार्किक useSelfForThis, ध्रुव string& text) {
    प्रत्यागम useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(तार्किक useSelfForThis, तार्किक protect) ध्रुव {
    प्रत्यागम VIdProtect::protect_शब्दाः_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_ग्रन्थिः::Ast_ग्रन्थिः(VNType t, File_पङ्क्तिः* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = एतत्;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = मिच्छ;
    m_flags.doingWidth = मिच्छ;
    m_flags.protect = सच्च;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_गणनम्_Inc();
}

Ast_ग्रन्थिः* Ast_ग्रन्थिः::abovep() ध्रुव {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), एतत्, "abovep() not allowed when in midlist");
    ध्रुव Ast_ग्रन्थिः* ध्रुव firstp = firstAbovep() ? एतत् : m_headtailp;
    प्रत्यागम firstp->backp();
}
Ast_ग्रन्थिः* Ast_ग्रन्थिः::aboveLoopp() ध्रुव {
    // Returns parent node. Avoid using this, may have performance issues.
    ध्रुव Ast_ग्रन्थिः* nodep = एतत्;
    // Backwards over peers (versus parents)
    जाव (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    प्रत्यागम nodep->backp();
}

string Ast_ग्रन्थिः::encodeName(ध्रुव string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    प्रति (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        जइ ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } अण्णहा जइ (pos[0] == '_') {
            out += pos[0];
            जइ (pos + 1 == namein.end()) भङ्ग;
            जइ (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } अण्णहा {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ध्रुव unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    प्रत्यागम vname.hashedName();
}

string Ast_ग्रन्थिः::encode_सङ्ख्या(int64_t अङ्कः) {
    जइ (अङ्कः < 0) प्रत्यागम "__02D" + cvtToStr(-अङ्कः);  // 2D=-
    प्रत्यागम cvtToStr(अङ्कः);
}

string Ast_ग्रन्थिः::nameProtect() ध्रुव { प्रत्यागम VIdProtect::protectIf(name(), protect()); }
string Ast_ग्रन्थिः::origNameProtect() ध्रुव { प्रत्यागम VIdProtect::protectIf(origName(), protect()); }

string Ast_ग्रन्थिः::shortName() ध्रुव {
    string pretty = name();
    string::size_type pos;
    जाव ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    प्रत्यागम pretty;
}

string Ast_ग्रन्थिः::dedotName(ध्रुव string& namein) {
    string pretty = namein;
    string::size_type pos;
    जाव ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    जइ (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    प्रत्यागम pretty;
}

string Ast_ग्रन्थिः::nameNoArray(ध्रुव string& namein) {
    प्रत्यागम namein.substr(0, namein.find("__BRA__"));
}

string Ast_ग्रन्थिः::vcdName(ध्रुव string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    जाव ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    जाव ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    प्रत्यागम prettyName(pretty);
}

string Ast_ग्रन्थिः::prettyName(ध्रुव string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    प्रति (ध्रुव वर्ण* pos = namein.c_str(); *pos;) {
        जइ (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            अनुवर्त;
        }
        जइ (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            जइ (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                अनुवर्त;
            }
            जइ (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                अनुवर्त;
            }
            जइ (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                अनुवर्त;
            }
            जइ (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                अनुवर्त;
            }
            जइ (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                अनुवर्त;
            }
            जइ (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                अनुवर्त;
            }
            जइ (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                वर्ण मानम् = 0;
                मानम् += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                मानम्
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += मानम्;
                pos += 5;
                अनुवर्त;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    जइ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    जइ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    प्रत्यागम pretty;
}

string Ast_ग्रन्थिः::vpiName(ध्रुव string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ध्रुव size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    तार्किक inEscapedIdent = मिच्छ;
    पूर्णाङ्क lastIdent = 0;

    प्रति (ध्रुव वर्ण* pos = namein.c_str() + offset; *pos;) {
        वर्ण specialChar = 0;
        जइ (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } अण्णहा जइ (pos[0] == '_' && pos[1] == '_') {  // __
            जइ (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } अण्णहा जइ (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } अण्णहा जइ (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } अण्णहा जइ (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                अनुवर्त;
            } अण्णहा जइ (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                वर्ण मानम् = 0;
                मानम् += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                मानम्
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                जइ (मानम् != '_') inEscapedIdent = सच्च;

                pretty += मानम्;
                pos += 5;
                अनुवर्त;
            }
        } अण्णहा जइ (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        जइ (specialChar) {
            जइ (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.निवेशय(lastIdent, "\\");
                inEscapedIdent = मिच्छ;
            }

            pretty += specialChar;

            जइ (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = मिच्छ;
            }
        } अण्णहा {
            pretty += pos[0];
            ++pos;
        }
    }
    जइ (inEscapedIdent) {
        pretty += " ";
        pretty.निवेशय(lastIdent, "\\");
    }
    जइ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    जइ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    प्रत्यागम pretty;
}

string Ast_ग्रन्थिः::prettyTypeName() ध्रुव {
    जइ (name() == "") प्रत्यागम typeName();
    प्रत्यागम std::string{typeName()} + " '" + pretty_Name_सन्देशः() + "'";
}

//######################################################################
// Insertion

शून्य Ast_ग्रन्थिः::debug_वृक्षः_Change(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* prefix, पूर्णाङ्क lineno, तार्किक next) {
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
Ast_ग्रन्थिः* Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_वृक्षः_Change(nodep, "-addNextThs: ", __LINE__, मिच्छ);
    debug_वृक्षः_Change(newp, "-addNextNew: ", __LINE__, सच्च);
    जइ (!nodep)  // verilog.y and lots of other places assume this
        प्रत्यागम newp;

    // Find end of old list
    Ast_ग्रन्थिः* oldtailp = nodep;
    जइ (oldtailp->m_nextp) {
        जइ (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } अण्णहा {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            जाव (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_ग्रन्थिः* ध्रुव newtailp = newp->m_headtailp;
    Ast_ग्रन्थिः* ध्रुव headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_गणनम्_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_वृक्षः_Change(nodep, "-addNextOut:", __LINE__, सच्च);
    प्रत्यागम nodep;
}

शून्य Ast_ग्रन्थिः::addNextHere(Ast_ग्रन्थिः* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_वृक्षः_Change(एतत्, "-addHereThs: ", __LINE__, मिच्छ);
    debug_वृक्षः_Change(newp, "-addHereNew: ", __LINE__, सच्च);
    newp->edit_गणनम्_Inc();

    Ast_ग्रन्थिः* ध्रुव addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_ग्रन्थिः* ध्रुव oldnextp = एतत्->m_nextp;
    एतत्->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    जइ (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = एतत्;

    // Head/tail
    Ast_ग्रन्थिः* ध्रुव oldheadtailp = एतत्->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    जइ (oldheadtailp) {  // else in middle of list, no change
        जइ (oldheadtailp == एतत्) {  // this was one node
            एतत्->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } अण्णहा जइ (!oldnextp) {  // this was tail
            एतत्->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_वृक्षः_Change(एतत्, "-addHereOut: ", __LINE__, सच्च);
}

शून्य Ast_ग्रन्थिः::setOp1p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, एतत्, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_वृक्षः_Change(एतत्, "-setOp1pThs: ", __LINE__, मिच्छ);
    debug_वृक्षः_Change(newp, "-setOp1pNew: ", __LINE__, सच्च);
    m_op1p = newp;
    newp->edit_गणनम्_Inc();
    newp->m_backp = एतत्;
    debug_वृक्षः_Change(एतत्, "-setOp1pOut: ", __LINE__, मिच्छ);
}

शून्य Ast_ग्रन्थिः::setOp2p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, एतत्, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_वृक्षः_Change(एतत्, "-setOp2pThs: ", __LINE__, मिच्छ);
    debug_वृक्षः_Change(newp, "-setOp2pNew: ", __LINE__, सच्च);
    m_op2p = newp;
    newp->edit_गणनम्_Inc();
    newp->m_backp = एतत्;
    debug_वृक्षः_Change(एतत्, "-setOp2pOut: ", __LINE__, मिच्छ);
}

शून्य Ast_ग्रन्थिः::setOp3p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, एतत्, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_वृक्षः_Change(एतत्, "-setOp3pThs: ", __LINE__, मिच्छ);
    debug_वृक्षः_Change(newp, "-setOp3pNew: ", __LINE__, सच्च);
    m_op3p = newp;
    newp->edit_गणनम्_Inc();
    newp->m_backp = एतत्;
    debug_वृक्षः_Change(एतत्, "-setOp3pOut: ", __LINE__, मिच्छ);
}

शून्य Ast_ग्रन्थिः::setOp4p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, एतत्, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_वृक्षः_Change(एतत्, "-setOp4pThs: ", __LINE__, मिच्छ);
    debug_वृक्षः_Change(newp, "-setOp4pNew: ", __LINE__, सच्च);
    m_op4p = newp;
    newp->edit_गणनम्_Inc();
    newp->m_backp = एतत्;
    debug_वृक्षः_Change(एतत्, "-setOp4pOut: ", __LINE__, मिच्छ);
}

शून्य Ast_ग्रन्थिः::addOp1p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जइ (!m_op1p) {
        op1p(newp);
    } अण्णहा {
        m_op1p->addNext(newp);
    }
}

शून्य Ast_ग्रन्थिः::addOp2p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जइ (!m_op2p) {
        op2p(newp);
    } अण्णहा {
        m_op2p->addNext(newp);
    }
}

शून्य Ast_ग्रन्थिः::addOp3p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जइ (!m_op3p) {
        op3p(newp);
    } अण्णहा {
        m_op3p->addNext(newp);
    }
}

शून्य Ast_ग्रन्थिः::addOp4p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जइ (!m_op4p) {
        op4p(newp);
    } अण्णहा {
        m_op4p->addNext(newp);
    }
}

शून्य Ast_ग्रन्थिः::prependOp1p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जइ (!m_op1p) {
        op1p(newp);
    } अण्णहा {
        m_op1p->addHereThisAsNext(newp);
    }
}

शून्य Ast_ग्रन्थिः::prependOp2p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जइ (!m_op2p) {
        op2p(newp);
    } अण्णहा {
        m_op2p->addHereThisAsNext(newp);
    }
}

शून्य Ast_ग्रन्थिः::prependOp3p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जइ (!m_op3p) {
        op3p(newp);
    } अण्णहा {
        m_op3p->addHereThisAsNext(newp);
    }
}

शून्य Ast_ग्रन्थिः::prependOp4p(Ast_ग्रन्थिः* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    जइ (!m_op4p) {
        op4p(newp);
    } अण्णहा {
        m_op4p->addHereThisAsNext(newp);
    }
}

शून्य Ast_ग्रन्थिः::replaceWith(Ast_ग्रन्थिः* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    एतत्->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
शून्य Ast_ग्रन्थिः::replaceWithKeepDType(Ast_ग्रन्थिः* newp) {
    newp->dtypeFrom(एतत्);
    replaceWith(newp);
}

शून्य VNRelinker::dump(std::ostream& str) ध्रुव {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_ग्रन्थिः* Ast_ग्रन्थिः::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_वृक्षः_Change(एतत्, "-unlinkWNextThs: ", __LINE__, सच्च);
    Ast_ग्रन्थिः* ध्रुव oldp = एतत्;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_गणनम्_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ग्रन्थिः* ध्रुव backp = oldp->m_backp;
    जइ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        जइ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } अण्णहा जइ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } अण्णहा जइ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } अण्णहा जइ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } अण्णहा जइ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } अण्णहा {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    जइ (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_ग्रन्थिः* oldtailp = backp->m_headtailp;
        जइ (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            जाव (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_ग्रन्थिः* ध्रुव oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } अण्णहा जइ (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } अण्णहा जइ (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } अण्णहा जइ (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } अण्णहा जइ (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } अण्णहा {
        एतत्->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    जइ (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_वृक्षः_Change(oldp, "-unlinkWNextOut: ", __LINE__, सच्च);
    प्रत्यागम oldp;
}

Ast_ग्रन्थिः* Ast_ग्रन्थिः::unlinkFrBack(VNRelinker* linkerp) {
    debug_वृक्षः_Change(एतत्, "-unlinkFrBkThs: ", __LINE__, सच्च);
    Ast_ग्रन्थिः* ध्रुव oldp = एतत्;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_गणनम्_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ग्रन्थिः* ध्रुव backp = oldp->m_backp;
    जइ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        जइ (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        जइ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } अण्णहा जइ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } अण्णहा जइ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } अण्णहा जइ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } अण्णहा जइ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } अण्णहा {
            एतत्->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    जइ (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        जइ (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        जइ (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } अण्णहा {
        जइ (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } अण्णहा जइ (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } अण्णहा जइ (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } अण्णहा जइ (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } अण्णहा {
            एतत्->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        जइ (oldp->m_nextp) {
            Ast_ग्रन्थिः* ध्रुव newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    जइ (oldp->m_iterpp) {  // Only if no linker, point to next in list
        जइ (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_वृक्षः_Change(oldp, "-unlinkFrBkOut: ", __LINE__, सच्च);
    प्रत्यागम oldp;
}

शून्य Ast_ग्रन्थिः::relink(VNRelinker* linkerp) {
    जइ (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_ग्रन्थिः* ध्रुव newp = एतत्;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_गणनम्_Inc();

    जइ (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_ग्रन्थिः* ध्रुव backp = linkerp->m_backp;
    debug_वृक्षः_Change(एतत्, "-relinkNew: ", __LINE__, सच्च);
    debug_वृक्षः_Change(backp, "-relinkTre: ", __LINE__, सच्च);

    विकल्प (linkerp->m_chg) {
    पक्ष VNRelinker::RELINK_NEXT: backp->addNextHere(newp); भङ्ग;
    पक्ष VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); भङ्ग;
    पक्ष VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); भङ्ग;
    पक्ष VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); भङ्ग;
    पक्ष VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); भङ्ग;
    स्वभाव: एतत्->v3fatalSrc("Relink of node without any link to change."); भङ्ग;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    जइ (linkerp->m_iterpp) {
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
    debug_वृक्षः_Change(एतत्, "-relinkOut: ", __LINE__, सच्च);
}

शून्य Ast_ग्रन्थिः::relinkOneLink(Ast_ग्रन्थिः*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_ग्रन्थिः* newp) {
    जइ (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_ग्रन्थिः* ध्रुव newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_ग्रन्थिः* ध्रुव oldlistlastp = pointpr->m_headtailp;
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

शून्य Ast_ग्रन्थिः::addHereThisAsNext(Ast_ग्रन्थिः* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(एतत्->m_backp, एतत्, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_ग्रन्थिः* ध्रुव backp = एतत्->m_backp;
    Ast_ग्रन्थिः* ध्रुव newLastp = newp->m_headtailp;
    //
    एतत्->edit_गणनम्_Inc();
    // Common linkage
    newLastp->m_nextp = एतत्;
    एतत्->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    जइ (backp && backp->m_nextp == एतत्) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } अण्णहा {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        जइ (backp) {
            जइ (backp->m_op1p == एतत्) {
                backp->m_op1p = newp;
            } अण्णहा जइ (backp->m_op2p == एतत्) {
                backp->m_op2p = newp;
            } अण्णहा जइ (backp->m_op3p == एतत्) {
                backp->m_op3p = newp;
            } अण्णहा {
                UASSERT_OBJ(backp->m_op4p == एतत्, एतत्, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_ग्रन्थिः* ध्रुव tailp = एतत्->m_headtailp;
        एतत्->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    जइ (newLastp->m_iterpp) *(newLastp->m_iterpp) = एतत्;
    जइ (एतत्->m_iterpp) {
        *(एतत्->m_iterpp) = newp;
        एतत्->m_iterpp = nullptr;
    }
    //
    debug_वृक्षः_Change(एतत्, "-addHereThisAsNext: ", __LINE__, सच्च);
}

//======================================================================
// Clone

Ast_ग्रन्थिः* Ast_ग्रन्थिः::clone_वृक्षः_Iter(तार्किक needPure) {
    // private: Clone single node and children
    जइ (needPure) purityCheck();
    Ast_ग्रन्थिः* ध्रुव newp = एतत्->clone();
    जइ (एतत्->m_op1p) newp->op1p(एतत्->m_op1p->clone_वृक्षः_Iter_List(needPure));
    जइ (एतत्->m_op2p) newp->op2p(एतत्->m_op2p->clone_वृक्षः_Iter_List(needPure));
    जइ (एतत्->m_op3p) newp->op3p(एतत्->m_op3p->clone_वृक्षः_Iter_List(needPure));
    जइ (एतत्->m_op4p) newp->op4p(एतत्->m_op4p->clone_वृक्षः_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(एतत्);  // Save pointers to/from both to simplify relinking.
    एतत्->clonep(newp);  // Save pointers to/from both to simplify relinking.
    प्रत्यागम newp;
}

Ast_ग्रन्थिः* Ast_ग्रन्थिः::clone_वृक्षः_Iter_List(तार्किक needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_ग्रन्थिः* newheadp = nullptr;
    Ast_ग्रन्थिः* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    प्रति (Ast_ग्रन्थिः* oldp = एतत्; oldp; oldp = oldp->m_nextp) {
        Ast_ग्रन्थिः* ध्रुव newp = oldp->clone_वृक्षः_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        जइ (newtailp) newtailp->m_nextp = newp;
        जइ (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    प्रत्यागम newheadp;
}

Ast_ग्रन्थिः* Ast_ग्रन्थिः::clone_वृक्षः(तार्किक cloneNextLink, तार्किक needPure) {
    debug_वृक्षः_Change(एतत्, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_वृक्षः();
    Ast_ग्रन्थिः* newp;
    जइ (cloneNextLink && एतत्->m_nextp) {
        newp = clone_वृक्षः_Iter_List(needPure);
    } अण्णहा {
        newp = clone_वृक्षः_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_वृक्षः();
    debug_वृक्षः_Change(newp, "-cloneOut: ", __LINE__, सच्च);
    प्रत्यागम newp;
}

शून्य Ast_ग्रन्थिः::purityCheck() {
    जइ (VL_UNLIKELY(!isPure())) {
        एतत्->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << एतत्->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

शून्य Ast_ग्रन्थिः::delete_ग्रन्थिः() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_गणनम्_Inc();
    // Change links of old node so we coredump if used
    एतत्->m_nextp = reinterpret_cast<Ast_ग्रन्थिः*>(0x1);
    एतत्->m_backp = reinterpret_cast<Ast_ग्रन्थिः*>(0x1);
    एतत्->m_headtailp = reinterpret_cast<Ast_ग्रन्थिः*>(0x1);
    एतत्->m_op1p = reinterpret_cast<Ast_ग्रन्थिः*>(0x1);
    एतत्->m_op2p = reinterpret_cast<Ast_ग्रन्थिः*>(0x1);
    एतत्->m_op3p = reinterpret_cast<Ast_ग्रन्थिः*>(0x1);
    एतत्->m_op4p = reinterpret_cast<Ast_ग्रन्थिः*>(0x1);
    एतत्->m_iterpp = reinterpret_cast<Ast_ग्रन्थिः**>(0x1);
    जइ (
#जइ !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        सच्च
#अण्णहा
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete एतत्;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

शून्य Ast_ग्रन्थिः::delete_वृक्षः_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    प्रति (Ast_ग्रन्थिः *nodep = एतत्, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        जइ (nodep->m_op1p) nodep->m_op1p->delete_वृक्षः_Iter();
        जइ (nodep->m_op2p) nodep->m_op2p->delete_वृक्षः_Iter();
        जइ (nodep->m_op3p) nodep->m_op3p->delete_वृक्षः_Iter();
        जइ (nodep->m_op4p) nodep->m_op4p->delete_वृक्षः_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_ग्रन्थिः();
    }
}

शून्य Ast_ग्रन्थिः::delete_वृक्षः() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_वृक्षः_Change(एतत्, "-delTree:  ", __LINE__, सच्च);
    एतत्->edit_गणनम्_Inc();
    // MUST be depth first!
    delete_वृक्षः_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
शून्य* Ast_ग्रन्थिः::operator नूतन(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_ग्रन्थिः* ध्रुव objp = static_cast<Ast_ग्रन्थिः*>(::operator नूतन(size));
    V3Broken::addNewed(objp);
    प्रत्यागम objp;
}

शून्य Ast_ग्रन्थिः::operator delete(शून्य* objp, size_t size) {
    जइ (!objp) प्रत्यागम;
    ध्रुव Ast_ग्रन्थिः* ध्रुव nodep = static_cast<Ast_ग्रन्थिः*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
शून्य* Ast_ग्रन्थिः::operator नूतन(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    स्थिर ध्रुव size_t ASTGEN_MAX_ग्रन्थिः_SIZE = []() {
        size_t maxSize = 0;
        प्रति (size_t t = 0; t < VNType::अङ्कः_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        प्रत्यागम maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    स्थिर uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    जइ (पूर्णाङ्क seed = v3Global.opt.debugAllocRandom()) {
        स्थिर V3Mutex s_mutex;
        ध्रुव V3LockGuard lock{s_mutex};
        स्थिर std::array<शून्य*, POOL_SIZE> s_ग्रन्थिः_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_ग्रन्थिः_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        जइ (!s_lfsr) {
            s_lfsr = seed;
            जइ (!s_lfsr) s_lfsr = ~s_lfsr;
            प्रति (size_t i = 0; i < POOL_SIZE; ++i) {
                s_ग्रन्थिः_Pool[i] = ::operator नूतन(ASTGEN_MAX_ग्रन्थिः_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_ग्रन्थिः_Pool), std::end(s_ग्रन्थिः_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ध्रुव size_t index = s_lfsr % POOL_SIZE;
        Ast_ग्रन्थिः* ध्रुव objp = static_cast<Ast_ग्रन्थिः*>(s_ग्रन्थिः_Pool[index]);
        s_ग्रन्थिः_Pool[index] = ::operator नूतन(ASTGEN_MAX_ग्रन्थिः_SIZE + 64);  // For later new()
        प्रत्यागम objp;
    } अण्णहा {
        Ast_ग्रन्थिः* ध्रुव objp = static_cast<Ast_ग्रन्थिः*>(::operator नूतन(size));
        प्रत्यागम objp;
    }
}
शून्य Ast_ग्रन्थिः::operator delete(शून्य* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (शून्य)objp;
    (शून्य)size;
}
#endif

//======================================================================
// Iterators

शून्य Ast_ग्रन्थिः::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    जइ (m_op1p) m_op1p->iterateAndNext(v);
    जइ (m_op2p) m_op2p->iterateAndNext(v);
    जइ (m_op3p) m_op3p->iterateAndNext(v);
    जइ (m_op4p) m_op4p->iterateAndNext(v);
}

शून्य Ast_ग्रन्थिः::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    जइ (m_op1p) m_op1p->iterateAndNextConst(v);
    जइ (m_op2p) m_op2p->iterateAndNextConst(v);
    जइ (m_op3p) m_op3p->iterateAndNextConst(v);
    जइ (m_op4p) m_op4p->iterateAndNextConst(v);
}

शून्य Ast_ग्रन्थिः::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_ग्रन्थिः* nodep = एतत्;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    जइ (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    जाव (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        जइ (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_ग्रन्थिः* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        जइ (!niterp) प्रत्यागम;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        जइ (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } अण्णहा {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

शून्य Ast_ग्रन्थिः::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_ग्रन्थिः* nodep = एतत्;
    जाव (nodep->m_nextp) nodep = nodep->m_nextp;
    जाव (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        जइ (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } अण्णहा {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

शून्य Ast_ग्रन्थिः::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    जइ (m_op1p) m_op1p->iterateListBackwardsConst(v);
    जइ (m_op2p) m_op2p->iterateListBackwardsConst(v);
    जइ (m_op3p) m_op3p->iterateListBackwardsConst(v);
    जइ (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

शून्य Ast_ग्रन्थिः::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_ग्रन्थिः* nodep = एतत्;
    करेहि {
        Ast_ग्रन्थिः* ध्रुव nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } जाव (nodep);
}

Ast_ग्रन्थिः* Ast_ग्रन्थिः::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_ग्रन्थिः* nodep = एतत्;  // Note "this" may point to bogus point later in this function
    जइ (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } अण्णहा जइ (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ध्रुव tempp = नूतन AstBegin{nodep->fileline(), "[EditWrapper]", nodep, मिच्छ};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_वृक्षः(), tempp);
    } अण्णहा {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_ग्रन्थिः** nextnodepp = nullptr;
        जइ (एतत्->m_backp->m_op1p == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_op1p);
        } अण्णहा जइ (एतत्->m_backp->m_op2p == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_op2p);
        } अण्णहा जइ (एतत्->m_backp->m_op3p == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_op3p);
        } अण्णहा जइ (एतत्->m_backp->m_op4p == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_op4p);
        } अण्णहा जइ (एतत्->m_backp->m_nextp == एतत्) {
            nextnodepp = &(एतत्->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, एतत्, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    प्रत्यागम nodep;
}

//======================================================================

शून्य Ast_ग्रन्थिः::clone_Relink_वृक्षः() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    प्रति (Ast_ग्रन्थिः* nodep = एतत्; nodep; nodep = nodep->m_nextp) {
        जइ (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        जइ (nodep->m_op1p) nodep->m_op1p->clone_Relink_वृक्षः();
        जइ (nodep->m_op2p) nodep->m_op2p->clone_Relink_वृक्षः();
        जइ (nodep->m_op3p) nodep->m_op3p->clone_Relink_वृक्षः();
        जइ (nodep->m_op4p) nodep->m_op4p->clone_Relink_वृक्षः();
    }
}

//======================================================================
// Comparison

तार्किक Ast_ग्रन्थिः::gate_वृक्षः_Iter() ध्रुव {
    // private: Return true if the two trees are identical
    जइ (!isGateOptimizable()) प्रत्यागम मिच्छ;
    जइ (m_op1p && !m_op1p->gate_वृक्षः_Iter()) प्रत्यागम मिच्छ;
    जइ (m_op2p && !m_op2p->gate_वृक्षः_Iter()) प्रत्यागम मिच्छ;
    जइ (m_op3p && !m_op3p->gate_वृक्षः_Iter()) प्रत्यागम मिच्छ;
    जइ (m_op4p && !m_op4p->gate_वृक्षः_Iter()) प्रत्यागम मिच्छ;
    प्रत्यागम सच्च;
}

तार्किक Ast_ग्रन्थिः::same_वृक्षः_Iter(ध्रुव Ast_ग्रन्थिः* node1p, ध्रुव Ast_ग्रन्थिः* node2p, तार्किक ignNext,
                           तार्किक gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    जाव (सच्च) {
        जइ (!node1p && !node2p) प्रत्यागम सच्च;
        जइ (!node1p || !node2p) प्रत्यागम मिच्छ;
        जइ (node1p->type() != node2p->type()) प्रत्यागम मिच्छ;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        जइ (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) प्रत्यागम मिच्छ;
        जइ (!node1p->same_ग्रन्थिः(node2p) || (gateOnly && !node1p->isGateOptimizable())) प्रत्यागम मिच्छ;
        जइ (!same_वृक्षः_Iter(node1p->m_op1p, node2p->m_op1p, मिच्छ, gateOnly)
            || !same_वृक्षः_Iter(node1p->m_op2p, node2p->m_op2p, मिच्छ, gateOnly)
            || !same_वृक्षः_Iter(node1p->m_op3p, node2p->m_op3p, मिच्छ, gateOnly)
            || !same_वृक्षः_Iter(node1p->m_op4p, node2p->m_op4p, मिच्छ, gateOnly)) {
            प्रत्यागम मिच्छ;
        }
        जइ (ignNext) प्रत्यागम सच्च;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

शून्य Ast_ग्रन्थिः::check_वृक्षः_Iter(ध्रुव Ast_ग्रन्थिः* prevBackp) ध्रुव VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == एतत्->backp(), एतत्, "Back node inconsistent");
    ध्रुव VNTypeInfo& typeInfo = VNType::typeInfo(एतत्->type());
    प्रति (पूर्णाङ्क i = 1; i <= 4; i++) {
        Ast_ग्रन्थिः* nodep = nullptr;
        विकल्प (i) {
        पक्ष 1: nodep = op1p(); भङ्ग;
        पक्ष 2: nodep = op2p(); भङ्ग;
        पक्ष 3: nodep = op3p(); भङ्ग;
        पक्ष 4: nodep = op4p(); भङ्ग;
        स्वभाव: एतत्->v3fatalSrc("Bad case"); भङ्ग;
        }
        // cppcheck-suppress danglingTempReference
        ध्रुव वर्ण* opName = typeInfo.m_opNamep[i - 1];
        विकल्प (typeInfo.m_opType[i - 1]) {
        पक्ष VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, एतत्, typeInfo.m_namep << " must not use " << opName << "()");
            भङ्ग;
        पक्ष VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, एतत्,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), एतत्,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_वृक्षः_Iter(एतत्);
            भङ्ग;
        पक्ष VNTypeInfo::OP_LIST:
            जइ (ध्रुव Ast_ग्रन्थिः* ध्रुव headp = nodep) {
                // cppcheck-suppress shadowFunction
                ध्रुव Ast_ग्रन्थिः* backp = एतत्;
                ध्रुव Ast_ग्रन्थिः* tailp;
                ध्रुव Ast_ग्रन्थिः* opp = headp;
                करेहि {
                    opp->check_वृक्षः_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } जाव (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            भङ्ग;
        पक्ष VNTypeInfo::OP_OPTIONAL:
            जइ (nodep) {
                UASSERT_OBJ(!nodep->nextp(), एतत्,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_वृक्षः_Iter(एतत्);
            }
            भङ्ग;
        स्वभाव: एतत्->v3fatalSrc("Bad case"); भङ्ग;
        }
    }
    जइ (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        जइ (ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(एतत्, ग्रन्थिः_व्यञ्जकम्)) {
            ध्रुव वर्ण* ध्रुव whyp = exprp->widthMismatch();
            जइ (whyp) {
                auto dtypeStr = [](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    प्रत्यागम ss.str();
                };
                जइ (ध्रुव Ast_ग्रन्थिः_Uniop* ध्रुव uniopp = VN_CAST(exprp, ग्रन्थिः_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } अण्णहा जइ (ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव biopp = VN_CAST(exprp, ग्रन्थिः_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } अण्णहा {
                    UASSERT_OBJ(मिच्छ, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
वर्ण* Ast_ग्रन्थिः::dump_वृक्षः_Json_Gdb(ध्रुव Ast_ग्रन्थिः* nodep) {
    जइ (!nodep) प्रत्यागम strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_वृक्षः_Json(nodepStream);
    ध्रुव std::string str = nodepStream.rdbuf()->str();
    प्रत्यागम strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
वर्ण* Ast_ग्रन्थिः::dump_वृक्षः_Json_Gdb(ध्रुव वर्ण* str) { प्रत्यागम strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
वर्ण* Ast_ग्रन्थिः::dump_वृक्षः_Json_Gdb(intptr_t nodep) {
    जइ (!nodep) प्रत्यागम strdup("{\"addr\":\"NULL\"}\n");
    प्रत्यागम dump_वृक्षः_Json_Gdb(reinterpret_cast<ध्रुव Ast_ग्रन्थिः*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_ग्रन्थिः::dumpGdb(ध्रुव Ast_ग्रन्थिः* nodep) {  // For GDB only  // LCOV_EXCL_START
    जइ (!nodep) {
        cout << "<nullptr>" << endl;
        प्रत्यागम;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_ग्रन्थिः::dumpGdbHeader() ध्रुव {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_ग्रन्थिः::dump_वृक्षः_Gdb(ध्रुव Ast_ग्रन्थिः* nodep) {  // For GDB only  // LCOV_EXCL_START
    जइ (!nodep) {
        cout << "<nullptr>" << endl;
        प्रत्यागम;
    }
    nodep->dumpGdbHeader();
    nodep->dump_वृक्षः(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_ग्रन्थिः::dump_वृक्षः_File_Gdb(ध्रुव Ast_ग्रन्थिः* nodep,  // LCOV_EXCL_START
                              ध्रुव वर्ण* filenamep) {  // For GDB only
    जइ (!nodep) {
        cout << "<nullptr>" << endl;
        प्रत्यागम;
    }
    ध्रुव string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_वृक्षः_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
शून्य Ast_ग्रन्थिः::checkIter() ध्रुव {
    जइ (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        एतत्->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

शून्य Ast_ग्रन्थिः::dumpPtrs(std::ostream& os) ध्रुव {
    os << "This=" << typeName() << " " << cvtToHex(एतत्);
    os << " back=" << cvtToHex(backp());
    जइ (nextp()) os << " next=" << cvtToHex(nextp());
    जइ (m_headtailp == एतत्) {
        os << " headtail=this";
    } अण्णहा {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    जइ (op1p()) os << " op1p=" << cvtToHex(op1p());
    जइ (op2p()) os << " op2p=" << cvtToHex(op2p());
    जइ (op3p()) os << " op3p=" << cvtToHex(op3p());
    जइ (op4p()) os << " op4p=" << cvtToHex(op4p());
    ध्रुव auto dumpUser = [&os](ध्रुव वर्ण* prefix, ध्रुव VNUser& user) {
        ध्रुव std::string s
            = user.dumpStr([](ध्रुव शून्य* p) -> std::string { प्रत्यागम cvtToHex(p); });
        जइ (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    जइ (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

शून्य Ast_ग्रन्थिः::dump_वृक्षः(std::ostream& os, ध्रुव string& indent, पूर्णाङ्क maxDepth) ध्रुव {
    स्थिर पूर्णाङ्क s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << एतत् << '\n';
    जइ (VN_DELETED(एतत्)) प्रत्यागम;
    जइ (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    जइ (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(एतत्)
           << endl;  // endl intentional to do flush
        प्रत्यागम;
    }  // LCOV_EXCL_STOP
    जइ (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    जइ (maxDepth == 1) {
        जइ (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } अण्णहा {
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्षः(os, indent + "1:", maxDepth - 1);
        }
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्षः(os, indent + "2:", maxDepth - 1);
        }
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्षः(os, indent + "3:", maxDepth - 1);
        }
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्षः(os, indent + "4:", maxDepth - 1);
        }
    }
}

शून्य Ast_ग्रन्थिः::dump_वृक्षः_And_Next(std::ostream& os, ध्रुव string& indent, पूर्णाङ्क maxDepth) ध्रुव {
    // Audited to make sure this is never nullptr
    प्रति (ध्रुव Ast_ग्रन्थिः* nodep = एतत्; nodep; nodep = nodep->nextp()) {
        nodep->dump_वृक्षः(os, indent, maxDepth);
    }
}

शून्य Ast_ग्रन्थिः::dump_वृक्षः_File(ध्रुव string& filename, तार्किक doDump) {
    // Not const function as calls checkTree
    जइ (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ध्रुव std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            जइ (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_गणनम्_Last();
            *logsp << "> to <e" << std::dec << edit_गणनम्_Gbl() << ">\n";
            जइ (edit_गणनम्_Gbl() == edit_गणनम्_Last() && ::dump_वृक्षः_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } अण्णहा {
                dump_वृक्षः(*logsp);
                edit_गणनम्_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

स्थिर शून्य drawChildren(std::ostream& os, ध्रुव Ast_ग्रन्थिः* thisp, ध्रुव Ast_ग्रन्थिः* childp,
                         ध्रुव std::string& childName) {
    जइ (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_वृक्षः_Dot(os);
            जइ (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

शून्य Ast_ग्रन्थिः::dump_वृक्षः_Dot(std::ostream& os) ध्रुव {
    os << "\tn" << cvtToHex(एतत्) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, एतत्, m_op1p, "op1");
    drawChildren(os, एतत्, m_op2p, "op2");
    drawChildren(os, एतत्, m_op3p, "op3");
    drawChildren(os, एतत्, m_op4p, "op4");
}

शून्य Ast_ग्रन्थिः::dump_वृक्षः_Json_File(ध्रुव string& filename, तार्किक doDump) {
    जइ (!doDump) प्रत्यागम;
    UINFO(2, "Dumping " << filename);
    ध्रुव std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    जइ (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_वृक्षः_Json(*treejsonp);
    *treejsonp << '\n';
}

शून्य Ast_ग्रन्थिः::dumpJsonMetaFileGdb(ध्रुव वर्ण* filename) { dumpJsonMetaFile(filename); }
शून्य Ast_ग्रन्थिः::dumpJsonMetaFile(ध्रुव string& filename) {
    UINFO(2, "Dumping " << filename);
    ध्रुव std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    जइ (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_पङ्क्तिः::file_Name_अङ्कः_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

शून्य Ast_ग्रन्थिः::dump_वृक्षः_Dot_File(ध्रुव string& filename, तार्किक doDump) {
    जइ (doDump) {
        UINFO(2, "Dumping " << filename);
        ध्रुव std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        जइ (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_वृक्षः_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_ग्रन्थिः::instanceStr() ध्रुव {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_गणनम् = 0;
    // Walk 'backp' chain
    प्रति (ध्रुव Ast_ग्रन्थिः* currp = एतत्; currp; currp = currp->backp(), ++iter_गणनम्) {
        जइ (VL_UNCOVERABLE(iter_गणनम् >= maxIterations)) प्रत्यागम "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        जइ (ध्रुव AstScope* ध्रुव scopep = VN_CAST(currp, Scope)) {
            प्रत्यागम scopep->is_शिखरम्() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        जइ (ध्रुव AstModule* ध्रुव modp = VN_CAST(currp, Module)) {
            ध्रुव string instanceName = modp->someInstanceName();
            प्रत्यागम instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    प्रत्यागम "";
}
शून्य Ast_ग्रन्थिः::v3errorEnd(ध्रुव std::ostringstream& str) ध्रुव VL_RELEASE(V3Error::s().m_mutex) {
    जइ (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } अण्णहा {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ध्रुव string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        जइ (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_ग्रन्थिः*>(एतत्)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
शून्य Ast_ग्रन्थिः::v3errorEndFatal(ध्रुव std::ostringstream& str) ध्रुव
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    प्रतिज्ञा(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

शून्य Ast_ग्रन्थिः::dtypeChgSigned(तार्किक flag) {
    UASSERT_OBJ(dtypep(), एतत्, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
शून्य Ast_ग्रन्थिः::dtypeChgWidth(पूर्णाङ्क width, पूर्णाङ्क widthMin) {
    UASSERT_OBJ(dtypep(), एतत्, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

शून्य Ast_ग्रन्थिः::dtypeChgWidthSigned(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), एतत्, "No dtype when changing width");
    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव enumDTypep = dtypep()->skipRefToEnump();
    जइ (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        प्रत्यागम;  // Correct already
    }
    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findBasicDType(VBasicDTypeKwd kwd) ध्रुव {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    प्रत्यागम v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findBitDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findLogicDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findLogic2StateDType(पूर्णाङ्क width, पूर्णाङ्क widthMin, VSigning numeric) ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findLogicRangeDType(ध्रुव VNumRange& range, पूर्णाङ्क widthMin,
                                           VSigning numeric) ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findBitRangeDType(ध्रुव VNumRange& range, पूर्णाङ्क widthMin,
                                         VSigning numeric) ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_ग्रन्थिः::find_निवेशय_Same_DType(AstBasicDType* nodep) {
    प्रत्यागम v3Global.rootp()->typeTablep()->find_निवेशय_Same_DType(nodep);
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findConstraintRefDType() ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findEmptyQueueDType() ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findQueueIndexDType() ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findStreamDType() ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::findVoidDType() ध्रुव {
    प्रत्यागम v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

स्थिर ध्रुव Ast_ग्रन्थिः_DType* computeCastableBase(ध्रुव Ast_ग्रन्थिः_DType* nodep) {
    जाव (सच्च) {
        जइ (ध्रुव AstPackArrayDType* ध्रुव packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            अनुवर्त;
        }
        जइ (ध्रुव Ast_ग्रन्थिः_DType* ध्रुव refp = nodep->skipRefToEnump()) {
            जइ (refp != nodep) {
                nodep = refp;
                अनुवर्त;
            }
        }
        प्रत्यागम nodep;
    }
}

स्थिर VCastable computeCastableImp(ध्रुव Ast_ग्रन्थिः_DType* toDtp, ध्रुव Ast_ग्रन्थिः_DType* fromDtp,
                                    ध्रुव Ast_ग्रन्थिः* fromConstp, ध्रुव तार्किक checkIfaceArgCompat) {
    ध्रुव VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    जइ (toDtp == fromDtp) प्रत्यागम VCastable::SAMEISH;
    जइ (toDtp->similarDType(fromDtp)) प्रत्यागम VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ध्रुव Ast_ग्रन्थिः_DType* fromBaseDtp = computeCastableBase(fromDtp);

    ध्रुव तार्किक fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, ग्रन्थिः_UOr_Struct_DType);

    ध्रुव Ast_ग्रन्थिः_DType* toBaseDtp = computeCastableBase(toDtp);
    ध्रुव तार्किक toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, ग्रन्थिः_UOr_Struct_DType);

    जइ (toBaseDtp == fromBaseDtp) प्रत्यागम VCastable::COMPATIBLE;

    जइ (toNumericable) {
        जइ (fromNumericable) प्रत्यागम VCastable::COMPATIBLE;
    } अण्णहा जइ (VN_IS(toBaseDtp, EnumDType)) {
        जइ (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_वृक्षः(fromDtp))
            प्रत्यागम VCastable::ENUM_IMPLICIT;
        जइ (fromNumericable) प्रत्यागम VCastable::ENUM_EXPLICIT;
    } अण्णहा जइ (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        प्रत्यागम VCastable::COMPATIBLE;
    } अण्णहा जइ (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        जइ (fromConstp->isNull()) प्रत्यागम VCastable::COMPATIBLE;
    } अण्णहा जइ (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ध्रुव AstClass* ध्रुव toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        ध्रुव AstClass* ध्रुव fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ध्रुव तार्किक downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        ध्रुव तार्किक upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        जइ (upcast) प्रत्यागम VCastable::COMPATIBLE;
        जइ (downcast) प्रत्यागम VCastable::DYNAMIC_CLASS;
        प्रत्यागम VCastable::INCOMPATIBLE;
    } अण्णहा जइ (ध्रुव AstIfaceRefDType* ध्रुव toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(सच्च) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ध्रुव AstIfaceRefDType* ध्रुव fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(सच्च) : fromDtp, IfaceRefDType);
        जइ (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            ध्रुव AstUnpackArrayDType* ध्रुव toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            ध्रुव AstUnpackArrayDType* ध्रुव fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            जइ (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                प्रत्यागम VCastable::INCOMPATIBLE;
            }
            प्रत्यागम computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        जइ (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            जइ (!checkIfaceArgCompat) प्रत्यागम castable;
            प्रत्यागम VCastable::INCOMPATIBLE;
        }
        ध्रुव तार्किक sameModport = toIfp->modportp() == fromIfp->modportp();
        जइ (!checkIfaceArgCompat) {
            जइ (!toIfp->modportp() || sameModport) प्रत्यागम VCastable::COMPATIBLE;
            प्रत्यागम castable;
        }
        जइ (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            प्रत्यागम VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        जइ (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            प्रत्यागम VCastable::COMPATIBLE;
        }
        प्रत्यागम VCastable::INCOMPATIBLE;
    } अण्णहा जइ (ध्रुव AstUnpackArrayDType* ध्रुव toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ध्रुव AstUnpackArrayDType* ध्रुव fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        जइ (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            प्रत्यागम castable;
        }
        ध्रुव VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव toSubp = toArrayp->subDTypep()->skipRefp();
        जइ (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            प्रत्यागम VCastable::COMPATIBLE;
        }
    }
    प्रत्यागम castable;
}

VCastable Ast_ग्रन्थिः::computeCastable(ध्रुव Ast_ग्रन्थिः_DType* toDtp, ध्रुव Ast_ग्रन्थिः_DType* fromDtp,
                                   ध्रुव Ast_ग्रन्थिः* fromConstp, ध्रुव तार्किक checkIfaceArgCompat) {
    ध्रुव auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    जइ (fromConstp) UINFO(9, "     const= " << fromConstp);
    प्रत्यागम castable;
}

Ast_ग्रन्थिः_DType* Ast_ग्रन्थिः::getCommonClassTypep(Ast_ग्रन्थिः* node1p, Ast_ग्रन्थिः* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    जइ (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        ध्रुव VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        जइ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            प्रत्यागम node1p->dtypep();
        जइ (castable == VCastable::DYNAMIC_CLASS) प्रत्यागम node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    जाव (classDtypep1) {
        ध्रुव VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        जइ (castable == VCastable::COMPATIBLE) प्रत्यागम classDtypep1;
        AstClassExtends* ध्रुव extendsp = classDtypep1->classp()->extendsp();
        जइ (!extendsp) भङ्ग;
        Ast_ग्रन्थिः_DType* ध्रुव edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    प्रत्यागम nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

वर्ग VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<पूर्णाङ्क, std::string> m_internedWordWidths;  // Interned widths
    std::map<पूर्णाङ्क, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ध्रुव तार्किक m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    स्थिर std::string toLetters(size_t मानम्, तार्किक lowerCase = मिच्छ) {
        ध्रुव वर्ण base = lowerCase ? 'a' : 'A';
        std::string s;
        करेहि { s += static_cast<वर्ण>(base + मानम् % 26); } जाव (मानम् /= 26);
        प्रत्यागम s;
    }

    ध्रुव std::string& internConst(ध्रुव AstConst& nodep) {
        ध्रुव auto pair = m_internedConsts.emplace(nodep.अङ्कः().ascii(मिच्छ), "");
        जइ (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& internWordWidth(पूर्णाङ्क मानम्) {
        ध्रुव auto pair = m_internedWordWidths.emplace(मानम्, "");
        जइ (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, सच्च);
        प्रत्यागम pair.first->second;
    }

    ध्रुव std::string& internWideWidth(पूर्णाङ्क मानम्) {
        ध्रुव auto pair = m_internedWideWidths.emplace(मानम्, "");
        जइ (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        प्रत्यागम pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    स्थिर तार्किक isLeaf(ध्रुव Ast_ग्रन्थिः* nodep) {
        ध्रुव VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        प्रति (ध्रुव VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            जइ (opType != VNTypeInfo::OP_UNUSED) प्रत्यागम मिच्छ;
        }
        प्रत्यागम सच्च;
    }

    // Render operand, return true if not unused
    शून्य renderOperand(VNTypeInfo::OpEn opType, ध्रुव Ast_ग्रन्थिः* ध्रुव opp, uint32_t depth) {
        विकल्प (opType) {
        पक्ष VNTypeInfo::OP_UNUSED:  //
            भङ्ग;
        पक्ष VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            भङ्ग;
        पक्ष VNTypeInfo::OP_LIST:
            m_os << " [";
            प्रति (ध्रुव Ast_ग्रन्थिः* nodep = opp; nodep; nodep = nodep->nextp()) {
                जइ (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            भङ्ग;
        पक्ष VNTypeInfo::OP_OPTIONAL:
            जइ (opp) {
                m_os << " ";
                render(opp, depth);
            } अण्णहा {
                m_os << " nil";
            }
            भङ्ग;
        }
    }

    // Render the node into the stream.
    शून्य render(ध्रुव Ast_ग्रन्थिः* nodep, uint32_t depth) {
        जइ (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            जइ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } अण्णहा जइ (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } अण्णहा {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } अण्णहा जइ (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } अण्णहा जइ (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } अण्णहा {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ध्रुव VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        जइ (!dtypep) {
            m_os << '?';
        } अण्णहा जइ (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } अण्णहा {
            ध्रुव पूर्णाङ्क width = nodep->width();
            जइ (width == 1) {
                m_os << '1';
            } अण्णहा जइ (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } अण्णहा {
                m_os << internWideWidth(width);
            }
            जइ (m_dumpWidthMin) {
                m_os << '/';
                ध्रुव पूर्णाङ्क widthMin = nodep->widthMin();
                जइ (widthMin == 1) {
                    m_os << '1';
                } अण्णहा जइ (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } अण्णहा {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

सार्वजनिक:
    VNPatternString(std::ostream& os, ध्रुव Ast_ग्रन्थिः* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_ग्रन्थिः_व्यञ्जकम्::patternString(uint32_t depth) ध्रुव {
    std::ostringstream oss;
    VNPatternString{oss, एतत्, depth};
    प्रत्यागम oss.str();
}

//######################################################################
// VNDeleter

शून्य VNDeleter::doDeletes() {
    प्रति (Ast_ग्रन्थिः* ध्रुव nodep : m_deleteps) nodep->delete_वृक्षः();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#अन्तर्भाव "V3Ast__gen_visitor_defns.h"  // From ./astgen
