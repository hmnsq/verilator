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

#ضمن "V3PchAstMT.h"

#ضمن "V3Broken.h"
#ضمن "V3File.h"

#ضمن <iomanip>
#ضمن <memory>
#ضمن <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t AstNode::s_editCntLast = 0;
uint64_t AstNode::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
صحيح AstNode::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

منطقي VNUser1InUse::s_userBusy = خطأ;
منطقي VNUser2InUse::s_userBusy = خطأ;
منطقي VNUser3InUse::s_userBusy = خطأ;
منطقي VNUser4InUse::s_userBusy = خطأ;

صحيح AstNodeDType::s_uniqueNum = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(ثابت string& name) {
    لكل (صحيح i = 0; i < _ENUM_MAX; ++i) {
        ثابت VCMethod method{i};
        𐤀𐤌 (name == method.ascii()) أرجع method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    أرجع VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ثابت فراغ*)) ثابت {
#ifdef VL_USER_TYPE_CHECKS
    𐤀𐤌 (ثابت uint64_t* ثابت uip = std::get_if<uint64_t>(&m_u)) أرجع "#"s + cvtToStr(*uip);
    𐤀𐤌 (فراغ* ثابت* ثابت upp = std::get_if<فراغ*>(&m_u)) أرجع fmtAddrp(*upp);
    أرجع "";
#وإلا
    // Dumps void* representation
    𐤀𐤌 (!m_u.up) أرجع "";
    أرجع fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

ثابت VNTypeInfo VNType::s_typeInfoTable[VNType::NUM_TYPES()] = {
#ضمن "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

منطقي VFwdType::isNodeCompatible(ثابت AstNode* nodep) ثابت {
    ثابت AstNode* defp = nodep;
    𐤀𐤌 (ثابت AstTypedef* ثابت adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    𐤀𐤌 (ثابت AstNodeDType* ثابت adefp = VN_CAST(defp, NodeDType))
        defp = adefp->skipRefToNonRefp();
    بدل (m_e) {
    حالة VFwdType::NONE: أرجع true; اكسر;
    حالة VFwdType::ENUM: أرجع VN_IS(defp, EnumDType); اكسر;
    حالة VFwdType::STRUCT: أرجع VN_IS(defp, StructDType); اكسر;
    حالة VFwdType::UNION: أرجع VN_IS(defp, UnionDType); اكسر;
    حالة VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    حالة VFwdType::CLASS: أرجع VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); اكسر;
    حالة VFwdType::GENERIC_INTERFACE: أرجع VN_IS(defp, IfaceRefDType); اكسر;
    افتراضي: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    أرجع خطأ;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ثابت std::shared_ptr<ثابت string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
ثابت std::shared_ptr<ثابت string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(منطقي useSelfForThis, ثابت string& text) {
    أرجع useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(منطقي useSelfForThis, منطقي protect) ثابت {
    أرجع VIdProtect::protectWordsIf(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

AstNode::AstNode(VNType t, FileLine* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = هذا;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = خطأ;
    m_flags.doingWidth = خطأ;
    m_flags.protect = true;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    editCountInc();
}

AstNode* AstNode::abovep() ثابت {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), هذا, "abovep() not allowed when in midlist");
    ثابت AstNode* ثابت firstp = firstAbovep() ? هذا : m_headtailp;
    أرجع firstp->backp();
}
AstNode* AstNode::aboveLoopp() ثابت {
    // Returns parent node. Avoid using this, may have performance issues.
    ثابت AstNode* nodep = هذا;
    // Backwards over peers (versus parents)
    طالما (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    أرجع nodep->backp();
}

string AstNode::encodeName(ثابت string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    لكل (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        𐤀𐤌 ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } وإلا 𐤀𐤌 (pos[0] == '_') {
            out += pos[0];
            𐤀𐤌 (pos + 1 == namein.end()) اكسر;
            𐤀𐤌 (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } وإلا {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ثابت unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    أرجع vname.hashedName();
}

string AstNode::encodeNumber(int64_t num) {
    𐤀𐤌 (num < 0) أرجع "__02D" + cvtToStr(-num);  // 2D=-
    أرجع cvtToStr(num);
}

string AstNode::nameProtect() ثابت { أرجع VIdProtect::protectIf(name(), protect()); }
string AstNode::origNameProtect() ثابت { أرجع VIdProtect::protectIf(origName(), protect()); }

string AstNode::shortName() ثابت {
    string pretty = name();
    string::size_type pos;
    طالما ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    أرجع pretty;
}

string AstNode::dedotName(ثابت string& namein) {
    string pretty = namein;
    string::size_type pos;
    طالما ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    𐤀𐤌 (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    أرجع pretty;
}

string AstNode::nameNoArray(ثابت string& namein) {
    أرجع namein.substr(0, namein.find("__BRA__"));
}

string AstNode::vcdName(ثابت string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    طالما ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    طالما ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    أرجع prettyName(pretty);
}

string AstNode::prettyName(ثابت string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    لكل (ثابت حرف* pos = namein.c_str(); *pos;) {
        𐤀𐤌 (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            استمر;
        }
        𐤀𐤌 (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            𐤀𐤌 (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                استمر;
            }
            𐤀𐤌 (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                استمر;
            }
            𐤀𐤌 (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                استمر;
            }
            𐤀𐤌 (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                استمر;
            }
            𐤀𐤌 (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                استمر;
            }
            𐤀𐤌 (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                استمر;
            }
            𐤀𐤌 (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                حرف value = 0;
                value += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                value
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += value;
                pos += 5;
                استمر;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    𐤀𐤌 (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    𐤀𐤌 (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    أرجع pretty;
}

string AstNode::vpiName(ثابت string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ثابت size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    منطقي inEscapedIdent = خطأ;
    صحيح lastIdent = 0;

    لكل (ثابت حرف* pos = namein.c_str() + offset; *pos;) {
        حرف specialChar = 0;
        𐤀𐤌 (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } وإلا 𐤀𐤌 (pos[0] == '_' && pos[1] == '_') {  // __
            𐤀𐤌 (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } وإلا 𐤀𐤌 (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } وإلا 𐤀𐤌 (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } وإلا 𐤀𐤌 (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                استمر;
            } وإلا 𐤀𐤌 (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                حرف value = 0;
                value += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                value
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                𐤀𐤌 (value != '_') inEscapedIdent = true;

                pretty += value;
                pos += 5;
                استمر;
            }
        } وإلا 𐤀𐤌 (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        𐤀𐤌 (specialChar) {
            𐤀𐤌 (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.insert(lastIdent, "\\");
                inEscapedIdent = خطأ;
            }

            pretty += specialChar;

            𐤀𐤌 (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = خطأ;
            }
        } وإلا {
            pretty += pos[0];
            ++pos;
        }
    }
    𐤀𐤌 (inEscapedIdent) {
        pretty += " ";
        pretty.insert(lastIdent, "\\");
    }
    𐤀𐤌 (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    𐤀𐤌 (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    أرجع pretty;
}

string AstNode::prettyTypeName() ثابت {
    𐤀𐤌 (name() == "") أرجع typeName();
    أرجع std::string{typeName()} + " '" + prettyNameMsg() + "'";
}

//######################################################################
// Insertion

فراغ AstNode::debugTreeChange(ثابت AstNode* nodep, ثابت حرف* prefix, صحيح lineno, منطقي next) {
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
AstNode* AstNode::addNext<AstNode, AstNode>(AstNode* nodep, AstNode* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debugTreeChange(nodep, "-addNextThs: ", __LINE__, خطأ);
    debugTreeChange(newp, "-addNextNew: ", __LINE__, true);
    𐤀𐤌 (!nodep)  // verilog.y and lots of other places assume this
        أرجع newp;

    // Find end of old list
    AstNode* oldtailp = nodep;
    𐤀𐤌 (oldtailp->m_nextp) {
        𐤀𐤌 (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } وإلا {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            طالما (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    AstNode* ثابت newtailp = newp->m_headtailp;
    AstNode* ثابت headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->editCountInc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debugTreeChange(nodep, "-addNextOut:", __LINE__, true);
    أرجع nodep;
}

فراغ AstNode::addNextHere(AstNode* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debugTreeChange(هذا, "-addHereThs: ", __LINE__, خطأ);
    debugTreeChange(newp, "-addHereNew: ", __LINE__, true);
    newp->editCountInc();

    AstNode* ثابت addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    AstNode* ثابت oldnextp = هذا->m_nextp;
    هذا->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    𐤀𐤌 (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = هذا;

    // Head/tail
    AstNode* ثابت oldheadtailp = هذا->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    𐤀𐤌 (oldheadtailp) {  // else in middle of list, no change
        𐤀𐤌 (oldheadtailp == هذا) {  // this was one node
            هذا->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } وإلا 𐤀𐤌 (!oldnextp) {  // this was tail
            هذا->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debugTreeChange(هذا, "-addHereOut: ", __LINE__, true);
}

فراغ AstNode::setOp1p(AstNode* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, هذا, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debugTreeChange(هذا, "-setOp1pThs: ", __LINE__, خطأ);
    debugTreeChange(newp, "-setOp1pNew: ", __LINE__, true);
    m_op1p = newp;
    newp->editCountInc();
    newp->m_backp = هذا;
    debugTreeChange(هذا, "-setOp1pOut: ", __LINE__, خطأ);
}

فراغ AstNode::setOp2p(AstNode* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, هذا, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debugTreeChange(هذا, "-setOp2pThs: ", __LINE__, خطأ);
    debugTreeChange(newp, "-setOp2pNew: ", __LINE__, true);
    m_op2p = newp;
    newp->editCountInc();
    newp->m_backp = هذا;
    debugTreeChange(هذا, "-setOp2pOut: ", __LINE__, خطأ);
}

فراغ AstNode::setOp3p(AstNode* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, هذا, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debugTreeChange(هذا, "-setOp3pThs: ", __LINE__, خطأ);
    debugTreeChange(newp, "-setOp3pNew: ", __LINE__, true);
    m_op3p = newp;
    newp->editCountInc();
    newp->m_backp = هذا;
    debugTreeChange(هذا, "-setOp3pOut: ", __LINE__, خطأ);
}

فراغ AstNode::setOp4p(AstNode* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, هذا, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debugTreeChange(هذا, "-setOp4pThs: ", __LINE__, خطأ);
    debugTreeChange(newp, "-setOp4pNew: ", __LINE__, true);
    m_op4p = newp;
    newp->editCountInc();
    newp->m_backp = هذا;
    debugTreeChange(هذا, "-setOp4pOut: ", __LINE__, خطأ);
}

فراغ AstNode::addOp1p(AstNode* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    𐤀𐤌 (!m_op1p) {
        op1p(newp);
    } وإلا {
        m_op1p->addNext(newp);
    }
}

فراغ AstNode::addOp2p(AstNode* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    𐤀𐤌 (!m_op2p) {
        op2p(newp);
    } وإلا {
        m_op2p->addNext(newp);
    }
}

فراغ AstNode::addOp3p(AstNode* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    𐤀𐤌 (!m_op3p) {
        op3p(newp);
    } وإلا {
        m_op3p->addNext(newp);
    }
}

فراغ AstNode::addOp4p(AstNode* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    𐤀𐤌 (!m_op4p) {
        op4p(newp);
    } وإلا {
        m_op4p->addNext(newp);
    }
}

فراغ AstNode::prependOp1p(AstNode* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    𐤀𐤌 (!m_op1p) {
        op1p(newp);
    } وإلا {
        m_op1p->addHereThisAsNext(newp);
    }
}

فراغ AstNode::prependOp2p(AstNode* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    𐤀𐤌 (!m_op2p) {
        op2p(newp);
    } وإلا {
        m_op2p->addHereThisAsNext(newp);
    }
}

فراغ AstNode::prependOp3p(AstNode* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    𐤀𐤌 (!m_op3p) {
        op3p(newp);
    } وإلا {
        m_op3p->addHereThisAsNext(newp);
    }
}

فراغ AstNode::prependOp4p(AstNode* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    𐤀𐤌 (!m_op4p) {
        op4p(newp);
    } وإلا {
        m_op4p->addHereThisAsNext(newp);
    }
}

فراغ AstNode::replaceWith(AstNode* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    هذا->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
فراغ AstNode::replaceWithKeepDType(AstNode* newp) {
    newp->dtypeFrom(هذا);
    replaceWith(newp);
}

فراغ VNRelinker::dump(std::ostream& str) ثابت {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

AstNode* AstNode::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debugTreeChange(هذا, "-unlinkWNextThs: ", __LINE__, true);
    AstNode* ثابت oldp = هذا;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->editCountInc();
    // cppcheck-suppress shadowFunction
    AstNode* ثابت backp = oldp->m_backp;
    𐤀𐤌 (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        𐤀𐤌 (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } وإلا 𐤀𐤌 (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } وإلا 𐤀𐤌 (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } وإلا 𐤀𐤌 (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } وإلا 𐤀𐤌 (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } وإلا {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    𐤀𐤌 (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        AstNode* oldtailp = backp->m_headtailp;
        𐤀𐤌 (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            طالما (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        AstNode* ثابت oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } وإلا 𐤀𐤌 (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } وإلا 𐤀𐤌 (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } وإلا 𐤀𐤌 (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } وإلا 𐤀𐤌 (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } وإلا {
        هذا->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    𐤀𐤌 (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debugTreeChange(oldp, "-unlinkWNextOut: ", __LINE__, true);
    أرجع oldp;
}

AstNode* AstNode::unlinkFrBack(VNRelinker* linkerp) {
    debugTreeChange(هذا, "-unlinkFrBkThs: ", __LINE__, true);
    AstNode* ثابت oldp = هذا;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->editCountInc();
    // cppcheck-suppress shadowFunction
    AstNode* ثابت backp = oldp->m_backp;
    𐤀𐤌 (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        𐤀𐤌 (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        𐤀𐤌 (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } وإلا 𐤀𐤌 (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } وإلا 𐤀𐤌 (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } وإلا 𐤀𐤌 (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } وإلا 𐤀𐤌 (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } وإلا {
            هذا->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    𐤀𐤌 (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        𐤀𐤌 (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        𐤀𐤌 (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } وإلا {
        𐤀𐤌 (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } وإلا 𐤀𐤌 (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } وإلا 𐤀𐤌 (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } وإلا 𐤀𐤌 (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } وإلا {
            هذا->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        𐤀𐤌 (oldp->m_nextp) {
            AstNode* ثابت newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    𐤀𐤌 (oldp->m_iterpp) {  // Only if no linker, point to next in list
        𐤀𐤌 (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debugTreeChange(oldp, "-unlinkFrBkOut: ", __LINE__, true);
    أرجع oldp;
}

فراغ AstNode::relink(VNRelinker* linkerp) {
    𐤀𐤌 (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    AstNode* ثابت newp = هذا;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->editCountInc();

    𐤀𐤌 (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    AstNode* ثابت backp = linkerp->m_backp;
    debugTreeChange(هذا, "-relinkNew: ", __LINE__, true);
    debugTreeChange(backp, "-relinkTre: ", __LINE__, true);

    بدل (linkerp->m_chg) {
    حالة VNRelinker::RELINK_NEXT: backp->addNextHere(newp); اكسر;
    حالة VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); اكسر;
    افتراضي: هذا->v3fatalSrc("Relink of node without any link to change."); اكسر;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    𐤀𐤌 (linkerp->m_iterpp) {
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
    debugTreeChange(هذا, "-relinkOut: ", __LINE__, true);
}

فراغ AstNode::relinkOneLink(AstNode*& pointpr,  // Ref to pointer that gets set to newp
                            AstNode* newp) {
    𐤀𐤌 (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        AstNode* ثابت newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        AstNode* ثابت oldlistlastp = pointpr->m_headtailp;
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

فراغ AstNode::addHereThisAsNext(AstNode* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(هذا->m_backp, هذا, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    AstNode* ثابت backp = هذا->m_backp;
    AstNode* ثابت newLastp = newp->m_headtailp;
    //
    هذا->editCountInc();
    // Common linkage
    newLastp->m_nextp = هذا;
    هذا->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    𐤀𐤌 (backp && backp->m_nextp == هذا) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } وإلا {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        𐤀𐤌 (backp) {
            𐤀𐤌 (backp->m_op1p == هذا) {
                backp->m_op1p = newp;
            } وإلا 𐤀𐤌 (backp->m_op2p == هذا) {
                backp->m_op2p = newp;
            } وإلا 𐤀𐤌 (backp->m_op3p == هذا) {
                backp->m_op3p = newp;
            } وإلا {
                UASSERT_OBJ(backp->m_op4p == هذا, هذا, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        AstNode* ثابت tailp = هذا->m_headtailp;
        هذا->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    𐤀𐤌 (newLastp->m_iterpp) *(newLastp->m_iterpp) = هذا;
    𐤀𐤌 (هذا->m_iterpp) {
        *(هذا->m_iterpp) = newp;
        هذا->m_iterpp = nullptr;
    }
    //
    debugTreeChange(هذا, "-addHereThisAsNext: ", __LINE__, true);
}

//======================================================================
// Clone

AstNode* AstNode::cloneTreeIter(منطقي needPure) {
    // private: Clone single node and children
    𐤀𐤌 (needPure) purityCheck();
    AstNode* ثابت newp = هذا->clone();
    𐤀𐤌 (هذا->m_op1p) newp->op1p(هذا->m_op1p->cloneTreeIterList(needPure));
    𐤀𐤌 (هذا->m_op2p) newp->op2p(هذا->m_op2p->cloneTreeIterList(needPure));
    𐤀𐤌 (هذا->m_op3p) newp->op3p(هذا->m_op3p->cloneTreeIterList(needPure));
    𐤀𐤌 (هذا->m_op4p) newp->op4p(هذا->m_op4p->cloneTreeIterList(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(هذا);  // Save pointers to/from both to simplify relinking.
    هذا->clonep(newp);  // Save pointers to/from both to simplify relinking.
    أرجع newp;
}

AstNode* AstNode::cloneTreeIterList(منطقي needPure) {
    // private: Clone list of nodes, set m_headtailp
    AstNode* newheadp = nullptr;
    AstNode* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    لكل (AstNode* oldp = هذا; oldp; oldp = oldp->m_nextp) {
        AstNode* ثابت newp = oldp->cloneTreeIter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        𐤀𐤌 (newtailp) newtailp->m_nextp = newp;
        𐤀𐤌 (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    أرجع newheadp;
}

AstNode* AstNode::cloneTree(منطقي cloneNextLink, منطقي needPure) {
    debugTreeChange(هذا, "-cloneThs: ", __LINE__, cloneNextLink);
    cloneClearTree();
    AstNode* newp;
    𐤀𐤌 (cloneNextLink && هذا->m_nextp) {
        newp = cloneTreeIterList(needPure);
    } وإلا {
        newp = cloneTreeIter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->cloneRelinkTree();
    debugTreeChange(newp, "-cloneOut: ", __LINE__, true);
    أرجع newp;
}

فراغ AstNode::purityCheck() {
    𐤀𐤌 (VL_UNLIKELY(!isPure())) {
        هذا->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << هذا->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

فراغ AstNode::deleteNode() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    editCountInc();
    // Change links of old node so we coredump if used
    هذا->m_nextp = reinterpret_cast<AstNode*>(0x1);
    هذا->m_backp = reinterpret_cast<AstNode*>(0x1);
    هذا->m_headtailp = reinterpret_cast<AstNode*>(0x1);
    هذا->m_op1p = reinterpret_cast<AstNode*>(0x1);
    هذا->m_op2p = reinterpret_cast<AstNode*>(0x1);
    هذا->m_op3p = reinterpret_cast<AstNode*>(0x1);
    هذا->m_op4p = reinterpret_cast<AstNode*>(0x1);
    هذا->m_iterpp = reinterpret_cast<AstNode**>(0x1);
    𐤀𐤌 (
#𐤀𐤌 !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        true
#وإلا
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete هذا;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

فراغ AstNode::deleteTreeIter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    لكل (AstNode *nodep = هذا, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        𐤀𐤌 (nodep->m_op1p) nodep->m_op1p->deleteTreeIter();
        𐤀𐤌 (nodep->m_op2p) nodep->m_op2p->deleteTreeIter();
        𐤀𐤌 (nodep->m_op3p) nodep->m_op3p->deleteTreeIter();
        𐤀𐤌 (nodep->m_op4p) nodep->m_op4p->deleteTreeIter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->deleteNode();
    }
}

فراغ AstNode::deleteTree() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debugTreeChange(هذا, "-delTree:  ", __LINE__, true);
    هذا->editCountInc();
    // MUST be depth first!
    deleteTreeIter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
فراغ* AstNode::operator جديد(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    AstNode* ثابت objp = static_cast<AstNode*>(::operator جديد(size));
    V3Broken::addNewed(objp);
    أرجع objp;
}

فراغ AstNode::operator delete(فراغ* objp, size_t size) {
    𐤀𐤌 (!objp) أرجع;
    ثابت AstNode* ثابت nodep = static_cast<AstNode*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
فراغ* AstNode::operator جديد(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ساكن ثابت size_t ASTGEN_MAX_NODE_SIZE = []() {
        size_t maxSize = 0;
        لكل (size_t t = 0; t < VNType::NUM_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        أرجع maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ساكن uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    𐤀𐤌 (صحيح seed = v3Global.opt.debugAllocRandom()) {
        ساكن V3Mutex s_mutex;
        ثابت V3LockGuard lock{s_mutex};
        ساكن std::array<فراغ*, POOL_SIZE> s_nodePool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_NODE_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        𐤀𐤌 (!s_lfsr) {
            s_lfsr = seed;
            𐤀𐤌 (!s_lfsr) s_lfsr = ~s_lfsr;
            لكل (size_t i = 0; i < POOL_SIZE; ++i) {
                s_nodePool[i] = ::operator جديد(ASTGEN_MAX_NODE_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_nodePool), std::end(s_nodePool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ثابت size_t index = s_lfsr % POOL_SIZE;
        AstNode* ثابت objp = static_cast<AstNode*>(s_nodePool[index]);
        s_nodePool[index] = ::operator جديد(ASTGEN_MAX_NODE_SIZE + 64);  // For later new()
        أرجع objp;
    } وإلا {
        AstNode* ثابت objp = static_cast<AstNode*>(::operator جديد(size));
        أرجع objp;
    }
}
فراغ AstNode::operator delete(فراغ* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (فراغ)objp;
    (فراغ)size;
}
#endif

//======================================================================
// Iterators

فراغ AstNode::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    𐤀𐤌 (m_op1p) m_op1p->iterateAndNext(v);
    𐤀𐤌 (m_op2p) m_op2p->iterateAndNext(v);
    𐤀𐤌 (m_op3p) m_op3p->iterateAndNext(v);
    𐤀𐤌 (m_op4p) m_op4p->iterateAndNext(v);
}

فراغ AstNode::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    𐤀𐤌 (m_op1p) m_op1p->iterateAndNextConst(v);
    𐤀𐤌 (m_op2p) m_op2p->iterateAndNextConst(v);
    𐤀𐤌 (m_op3p) m_op3p->iterateAndNextConst(v);
    𐤀𐤌 (m_op4p) m_op4p->iterateAndNextConst(v);
}

فراغ AstNode::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    AstNode* nodep = هذا;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    𐤀𐤌 (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    طالما (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        𐤀𐤌 (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        AstNode* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        𐤀𐤌 (!niterp) أرجع;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        𐤀𐤌 (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } وإلا {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

فراغ AstNode::iterateListBackwardsConst(VNVisitorConst& v) {
    AstNode* nodep = هذا;
    طالما (nodep->m_nextp) nodep = nodep->m_nextp;
    طالما (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        𐤀𐤌 (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } وإلا {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

فراغ AstNode::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    𐤀𐤌 (m_op1p) m_op1p->iterateListBackwardsConst(v);
    𐤀𐤌 (m_op2p) m_op2p->iterateListBackwardsConst(v);
    𐤀𐤌 (m_op3p) m_op3p->iterateListBackwardsConst(v);
    𐤀𐤌 (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

فراغ AstNode::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    AstNode* nodep = هذا;
    𐤐𐤏𐤋 {
        AstNode* ثابت nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } طالما (nodep);
}

AstNode* AstNode::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    AstNode* nodep = هذا;  // Note "this" may point to bogus point later in this function
    𐤀𐤌 (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } وإلا 𐤀𐤌 (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ثابت tempp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", nodep, خطأ};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->deleteTree(), tempp);
    } وإلا {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        AstNode** nextnodepp = nullptr;
        𐤀𐤌 (هذا->m_backp->m_op1p == هذا) {
            nextnodepp = &(هذا->m_backp->m_op1p);
        } وإلا 𐤀𐤌 (هذا->m_backp->m_op2p == هذا) {
            nextnodepp = &(هذا->m_backp->m_op2p);
        } وإلا 𐤀𐤌 (هذا->m_backp->m_op3p == هذا) {
            nextnodepp = &(هذا->m_backp->m_op3p);
        } وإلا 𐤀𐤌 (هذا->m_backp->m_op4p == هذا) {
            nextnodepp = &(هذا->m_backp->m_op4p);
        } وإلا 𐤀𐤌 (هذا->m_backp->m_nextp == هذا) {
            nextnodepp = &(هذا->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, هذا, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    أرجع nodep;
}

//======================================================================

فراغ AstNode::cloneRelinkTree() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    لكل (AstNode* nodep = هذا; nodep; nodep = nodep->m_nextp) {
        𐤀𐤌 (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        𐤀𐤌 (nodep->m_op1p) nodep->m_op1p->cloneRelinkTree();
        𐤀𐤌 (nodep->m_op2p) nodep->m_op2p->cloneRelinkTree();
        𐤀𐤌 (nodep->m_op3p) nodep->m_op3p->cloneRelinkTree();
        𐤀𐤌 (nodep->m_op4p) nodep->m_op4p->cloneRelinkTree();
    }
}

//======================================================================
// Comparison

منطقي AstNode::gateTreeIter() ثابت {
    // private: Return true if the two trees are identical
    𐤀𐤌 (!isGateOptimizable()) أرجع خطأ;
    𐤀𐤌 (m_op1p && !m_op1p->gateTreeIter()) أرجع خطأ;
    𐤀𐤌 (m_op2p && !m_op2p->gateTreeIter()) أرجع خطأ;
    𐤀𐤌 (m_op3p && !m_op3p->gateTreeIter()) أرجع خطأ;
    𐤀𐤌 (m_op4p && !m_op4p->gateTreeIter()) أرجع خطأ;
    أرجع true;
}

منطقي AstNode::sameTreeIter(ثابت AstNode* node1p, ثابت AstNode* node2p, منطقي ignNext,
                           منطقي gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    طالما (true) {
        𐤀𐤌 (!node1p && !node2p) أرجع true;
        𐤀𐤌 (!node1p || !node2p) أرجع خطأ;
        𐤀𐤌 (node1p->type() != node2p->type()) أرجع خطأ;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        𐤀𐤌 (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) أرجع خطأ;
        𐤀𐤌 (!node1p->sameNode(node2p) || (gateOnly && !node1p->isGateOptimizable())) أرجع خطأ;
        𐤀𐤌 (!sameTreeIter(node1p->m_op1p, node2p->m_op1p, خطأ, gateOnly)
            || !sameTreeIter(node1p->m_op2p, node2p->m_op2p, خطأ, gateOnly)
            || !sameTreeIter(node1p->m_op3p, node2p->m_op3p, خطأ, gateOnly)
            || !sameTreeIter(node1p->m_op4p, node2p->m_op4p, خطأ, gateOnly)) {
            أرجع خطأ;
        }
        𐤀𐤌 (ignNext) أرجع true;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

فراغ AstNode::checkTreeIter(ثابت AstNode* prevBackp) ثابت VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == هذا->backp(), هذا, "Back node inconsistent");
    ثابت VNTypeInfo& typeInfo = VNType::typeInfo(هذا->type());
    لكل (صحيح i = 1; i <= 4; i++) {
        AstNode* nodep = nullptr;
        بدل (i) {
        حالة 1: nodep = op1p(); اكسر;
        حالة 2: nodep = op2p(); اكسر;
        حالة 3: nodep = op3p(); اكسر;
        حالة 4: nodep = op4p(); اكسر;
        افتراضي: هذا->v3fatalSrc("Bad case"); اكسر;
        }
        // cppcheck-suppress danglingTempReference
        ثابت حرف* opName = typeInfo.m_opNamep[i - 1];
        بدل (typeInfo.m_opType[i - 1]) {
        حالة VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, هذا, typeInfo.m_namep << " must not use " << opName << "()");
            اكسر;
        حالة VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, هذا,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), هذا,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->checkTreeIter(هذا);
            اكسر;
        حالة VNTypeInfo::OP_LIST:
            𐤀𐤌 (ثابت AstNode* ثابت headp = nodep) {
                // cppcheck-suppress shadowFunction
                ثابت AstNode* backp = هذا;
                ثابت AstNode* tailp;
                ثابت AstNode* opp = headp;
                𐤐𐤏𐤋 {
                    opp->checkTreeIter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } طالما (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            اكسر;
        حالة VNTypeInfo::OP_OPTIONAL:
            𐤀𐤌 (nodep) {
                UASSERT_OBJ(!nodep->nextp(), هذا,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->checkTreeIter(هذا);
            }
            اكسر;
        افتراضي: هذا->v3fatalSrc("Bad case"); اكسر;
        }
    }
    𐤀𐤌 (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        𐤀𐤌 (ثابت AstNodeExpr* ثابت exprp = VN_CAST(هذا, NodeExpr)) {
            ثابت حرف* ثابت whyp = exprp->widthMismatch();
            𐤀𐤌 (whyp) {
                auto dtypeStr = [](ثابت AstNodeExpr* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    أرجع ss.str();
                };
                𐤀𐤌 (ثابت AstNodeUniop* ثابت uniopp = VN_CAST(exprp, NodeUniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } وإلا 𐤀𐤌 (ثابت AstNodeBiop* ثابت biopp = VN_CAST(exprp, NodeBiop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } وإلا {
                    UASSERT_OBJ(خطأ, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
حرف* AstNode::dumpTreeJsonGdb(ثابت AstNode* nodep) {
    𐤀𐤌 (!nodep) أرجع strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dumpTreeJson(nodepStream);
    ثابت std::string str = nodepStream.rdbuf()->str();
    أرجع strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
حرف* AstNode::dumpTreeJsonGdb(ثابت حرف* str) { أرجع strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
حرف* AstNode::dumpTreeJsonGdb(intptr_t nodep) {
    𐤀𐤌 (!nodep) أرجع strdup("{\"addr\":\"NULL\"}\n");
    أرجع dumpTreeJsonGdb(reinterpret_cast<ثابت AstNode*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
فراغ AstNode::dumpGdb(ثابت AstNode* nodep) {  // For GDB only  // LCOV_EXCL_START
    𐤀𐤌 (!nodep) {
        cout << "<nullptr>" << endl;
        أرجع;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ AstNode::dumpGdbHeader() ثابت {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ AstNode::dumpTreeGdb(ثابت AstNode* nodep) {  // For GDB only  // LCOV_EXCL_START
    𐤀𐤌 (!nodep) {
        cout << "<nullptr>" << endl;
        أرجع;
    }
    nodep->dumpGdbHeader();
    nodep->dumpTree(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ AstNode::dumpTreeFileGdb(ثابت AstNode* nodep,  // LCOV_EXCL_START
                              ثابت حرف* filenamep) {  // For GDB only
    𐤀𐤌 (!nodep) {
        cout << "<nullptr>" << endl;
        أرجع;
    }
    ثابت string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dumpTreeFile(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
فراغ AstNode::checkIter() ثابت {
    𐤀𐤌 (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        هذا->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

فراغ AstNode::dumpPtrs(std::ostream& os) ثابت {
    os << "This=" << typeName() << " " << cvtToHex(هذا);
    os << " back=" << cvtToHex(backp());
    𐤀𐤌 (nextp()) os << " next=" << cvtToHex(nextp());
    𐤀𐤌 (m_headtailp == هذا) {
        os << " headtail=this";
    } وإلا {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    𐤀𐤌 (op1p()) os << " op1p=" << cvtToHex(op1p());
    𐤀𐤌 (op2p()) os << " op2p=" << cvtToHex(op2p());
    𐤀𐤌 (op3p()) os << " op3p=" << cvtToHex(op3p());
    𐤀𐤌 (op4p()) os << " op4p=" << cvtToHex(op4p());
    ثابت auto dumpUser = [&os](ثابت حرف* prefix, ثابت VNUser& user) {
        ثابت std::string s
            = user.dumpStr([](ثابت فراغ* p) -> std::string { أرجع cvtToHex(p); });
        𐤀𐤌 (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    𐤀𐤌 (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

فراغ AstNode::dumpTree(std::ostream& os, ثابت string& indent, صحيح maxDepth) ثابت {
    ساكن صحيح s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << هذا << '\n';
    𐤀𐤌 (VN_DELETED(هذا)) أرجع;
    𐤀𐤌 (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    𐤀𐤌 (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(هذا)
           << endl;  // endl intentional to do flush
        أرجع;
    }  // LCOV_EXCL_STOP
    𐤀𐤌 (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    𐤀𐤌 (maxDepth == 1) {
        𐤀𐤌 (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } وإلا {
        لكل (ثابت AstNode* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dumpTree(os, indent + "1:", maxDepth - 1);
        }
        لكل (ثابت AstNode* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dumpTree(os, indent + "2:", maxDepth - 1);
        }
        لكل (ثابت AstNode* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dumpTree(os, indent + "3:", maxDepth - 1);
        }
        لكل (ثابت AstNode* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dumpTree(os, indent + "4:", maxDepth - 1);
        }
    }
}

فراغ AstNode::dumpTreeAndNext(std::ostream& os, ثابت string& indent, صحيح maxDepth) ثابت {
    // Audited to make sure this is never nullptr
    لكل (ثابت AstNode* nodep = هذا; nodep; nodep = nodep->nextp()) {
        nodep->dumpTree(os, indent, maxDepth);
    }
}

فراغ AstNode::dumpTreeFile(ثابت string& filename, منطقي doDump) {
    // Not const function as calls checkTree
    𐤀𐤌 (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            𐤀𐤌 (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << editCountLast();
            *logsp << "> to <e" << std::dec << editCountGbl() << ">\n";
            𐤀𐤌 (editCountGbl() == editCountLast() && ::dumpTreeLevel() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } وإلا {
                dumpTree(*logsp);
                editCountSetLast();  // Next dump can indicate start from here
            }
        }
    }
}

ساكن فراغ drawChildren(std::ostream& os, ثابت AstNode* thisp, ثابت AstNode* childp,
                         ثابت std::string& childName) {
    𐤀𐤌 (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        لكل (ثابت AstNode* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dumpTreeDot(os);
            𐤀𐤌 (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

فراغ AstNode::dumpTreeDot(std::ostream& os) ثابت {
    os << "\tn" << cvtToHex(هذا) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, هذا, m_op1p, "op1");
    drawChildren(os, هذا, m_op2p, "op2");
    drawChildren(os, هذا, m_op3p, "op3");
    drawChildren(os, هذا, m_op4p, "op4");
}

فراغ AstNode::dumpTreeJsonFile(ثابت string& filename, منطقي doDump) {
    𐤀𐤌 (!doDump) أرجع;
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    𐤀𐤌 (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dumpTreeJson(*treejsonp);
    *treejsonp << '\n';
}

فراغ AstNode::dumpJsonMetaFileGdb(ثابت حرف* filename) { dumpJsonMetaFile(filename); }
فراغ AstNode::dumpJsonMetaFile(ثابت string& filename) {
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    𐤀𐤌 (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    FileLine::fileNameNumMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

فراغ AstNode::dumpTreeDotFile(ثابت string& filename, منطقي doDump) {
    𐤀𐤌 (doDump) {
        UINFO(2, "Dumping " << filename);
        ثابت std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        𐤀𐤌 (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dumpTreeDot(*treedotp);
        *treedotp << "}\n";
    }
}

string AstNode::instanceStr() ثابت {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iterCount = 0;
    // Walk 'backp' chain
    لكل (ثابت AstNode* currp = هذا; currp; currp = currp->backp(), ++iterCount) {
        𐤀𐤌 (VL_UNCOVERABLE(iterCount >= maxIterations)) أرجع "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        𐤀𐤌 (ثابت AstScope* ثابت scopep = VN_CAST(currp, Scope)) {
            أرجع scopep->isTop() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        𐤀𐤌 (ثابت AstModule* ثابت modp = VN_CAST(currp, Module)) {
            ثابت string instanceName = modp->someInstanceName();
            أرجع instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    أرجع "";
}
فراغ AstNode::v3errorEnd(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    𐤀𐤌 (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } وإلا {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ثابت string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        𐤀𐤌 (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<AstNode*>(هذا)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
فراغ AstNode::v3errorEndFatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

فراغ AstNode::dtypeChgSigned(منطقي flag) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
فراغ AstNode::dtypeChgWidth(صحيح width, صحيح widthMin) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

فراغ AstNode::dtypeChgWidthSigned(صحيح width, صحيح widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing width");
    ثابت AstNodeDType* ثابت enumDTypep = dtypep()->skipRefToEnump();
    𐤀𐤌 (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        أرجع;  // Correct already
    }
    ثابت AstNodeDType* ثابت dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

AstNodeDType* AstNode::findBasicDType(VBasicDTypeKwd kwd) ثابت {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    أرجع v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
AstNodeDType* AstNode::findBitDType(صحيح width, صحيح widthMin, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
AstNodeDType* AstNode::findLogicDType(صحيح width, صحيح widthMin, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
AstNodeDType* AstNode::findLogic2StateDType(صحيح width, صحيح widthMin, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
AstNodeDType* AstNode::findLogicRangeDType(ثابت VNumRange& range, صحيح widthMin,
                                           VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
AstNodeDType* AstNode::findBitRangeDType(ثابت VNumRange& range, صحيح widthMin,
                                         VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* AstNode::findInsertSameDType(AstBasicDType* nodep) {
    أرجع v3Global.rootp()->typeTablep()->findInsertSameDType(nodep);
}
AstNodeDType* AstNode::findConstraintRefDType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
AstNodeDType* AstNode::findEmptyQueueDType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
AstNodeDType* AstNode::findQueueIndexDType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
AstNodeDType* AstNode::findStreamDType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
AstNodeDType* AstNode::findVoidDType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

ساكن ثابت AstNodeDType* computeCastableBase(ثابت AstNodeDType* nodep) {
    طالما (true) {
        𐤀𐤌 (ثابت AstPackArrayDType* ثابت packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            استمر;
        }
        𐤀𐤌 (ثابت AstNodeDType* ثابت refp = nodep->skipRefToEnump()) {
            𐤀𐤌 (refp != nodep) {
                nodep = refp;
                استمر;
            }
        }
        أرجع nodep;
    }
}

ساكن VCastable computeCastableImp(ثابت AstNodeDType* toDtp, ثابت AstNodeDType* fromDtp,
                                    ثابت AstNode* fromConstp, ثابت منطقي checkIfaceArgCompat) {
    ثابت VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    𐤀𐤌 (toDtp == fromDtp) أرجع VCastable::SAMEISH;
    𐤀𐤌 (toDtp->similarDType(fromDtp)) أرجع VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ثابت AstNodeDType* fromBaseDtp = computeCastableBase(fromDtp);

    ثابت منطقي fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, NodeUOrStructDType);

    ثابت AstNodeDType* toBaseDtp = computeCastableBase(toDtp);
    ثابت منطقي toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, NodeUOrStructDType);

    𐤀𐤌 (toBaseDtp == fromBaseDtp) أرجع VCastable::COMPATIBLE;

    𐤀𐤌 (toNumericable) {
        𐤀𐤌 (fromNumericable) أرجع VCastable::COMPATIBLE;
    } وإلا 𐤀𐤌 (VN_IS(toBaseDtp, EnumDType)) {
        𐤀𐤌 (VN_IS(fromBaseDtp, EnumDType) && toDtp->sameTree(fromDtp))
            أرجع VCastable::ENUM_IMPLICIT;
        𐤀𐤌 (fromNumericable) أرجع VCastable::ENUM_EXPLICIT;
    } وإلا 𐤀𐤌 (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        أرجع VCastable::COMPATIBLE;
    } وإلا 𐤀𐤌 (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        𐤀𐤌 (fromConstp->isNull()) أرجع VCastable::COMPATIBLE;
    } وإلا 𐤀𐤌 (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ثابت AstClass* ثابت toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        ثابت AstClass* ثابت fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ثابت منطقي downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        ثابت منطقي upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        𐤀𐤌 (upcast) أرجع VCastable::COMPATIBLE;
        𐤀𐤌 (downcast) أرجع VCastable::DYNAMIC_CLASS;
        أرجع VCastable::INCOMPATIBLE;
    } وإلا 𐤀𐤌 (ثابت AstIfaceRefDType* ثابت toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(true) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ثابت AstIfaceRefDType* ثابت fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(true) : fromDtp, IfaceRefDType);
        𐤀𐤌 (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            ثابت AstUnpackArrayDType* ثابت toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            𐤀𐤌 (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                أرجع VCastable::INCOMPATIBLE;
            }
            أرجع computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        𐤀𐤌 (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            𐤀𐤌 (!checkIfaceArgCompat) أرجع castable;
            أرجع VCastable::INCOMPATIBLE;
        }
        ثابت منطقي sameModport = toIfp->modportp() == fromIfp->modportp();
        𐤀𐤌 (!checkIfaceArgCompat) {
            𐤀𐤌 (!toIfp->modportp() || sameModport) أرجع VCastable::COMPATIBLE;
            أرجع castable;
        }
        𐤀𐤌 (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            أرجع VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        𐤀𐤌 (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            أرجع VCastable::COMPATIBLE;
        }
        أرجع VCastable::INCOMPATIBLE;
    } وإلا 𐤀𐤌 (ثابت AstUnpackArrayDType* ثابت toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        𐤀𐤌 (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            أرجع castable;
        }
        ثابت VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ثابت AstNodeDType* ثابت toSubp = toArrayp->subDTypep()->skipRefp();
        𐤀𐤌 (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            أرجع VCastable::COMPATIBLE;
        }
    }
    أرجع castable;
}

VCastable AstNode::computeCastable(ثابت AstNodeDType* toDtp, ثابت AstNodeDType* fromDtp,
                                   ثابت AstNode* fromConstp, ثابت منطقي checkIfaceArgCompat) {
    ثابت auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    𐤀𐤌 (fromConstp) UINFO(9, "     const= " << fromConstp);
    أرجع castable;
}

AstNodeDType* AstNode::getCommonClassTypep(AstNode* node1p, AstNode* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    𐤀𐤌 (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        ثابت VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        𐤀𐤌 (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            أرجع node1p->dtypep();
        𐤀𐤌 (castable == VCastable::DYNAMIC_CLASS) أرجع node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    طالما (classDtypep1) {
        ثابت VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        𐤀𐤌 (castable == VCastable::COMPATIBLE) أرجع classDtypep1;
        AstClassExtends* ثابت extendsp = classDtypep1->classp()->extendsp();
        𐤀𐤌 (!extendsp) اكسر;
        AstNodeDType* ثابت edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    أرجع nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

صنف VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<صحيح, std::string> m_internedWordWidths;  // Interned widths
    std::map<صحيح, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ثابت منطقي m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    ساكن std::string toLetters(size_t value, منطقي lowerCase = خطأ) {
        ثابت حرف base = lowerCase ? 'a' : 'A';
        std::string s;
        𐤐𐤏𐤋 { s += static_cast<حرف>(base + value % 26); } طالما (value /= 26);
        أرجع s;
    }

    ثابت std::string& internConst(ثابت AstConst& nodep) {
        ثابت auto pair = m_internedConsts.emplace(nodep.num().ascii(خطأ), "");
        𐤀𐤌 (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        أرجع pair.first->second;
    }

    ثابت std::string& internWordWidth(صحيح value) {
        ثابت auto pair = m_internedWordWidths.emplace(value, "");
        𐤀𐤌 (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, true);
        أرجع pair.first->second;
    }

    ثابت std::string& internWideWidth(صحيح value) {
        ثابت auto pair = m_internedWideWidths.emplace(value, "");
        𐤀𐤌 (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        أرجع pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ساكن منطقي isLeaf(ثابت AstNode* nodep) {
        ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        لكل (ثابت VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            𐤀𐤌 (opType != VNTypeInfo::OP_UNUSED) أرجع خطأ;
        }
        أرجع true;
    }

    // Render operand, return true if not unused
    فراغ renderOperand(VNTypeInfo::OpEn opType, ثابت AstNode* ثابت opp, uint32_t depth) {
        بدل (opType) {
        حالة VNTypeInfo::OP_UNUSED:  //
            اكسر;
        حالة VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            اكسر;
        حالة VNTypeInfo::OP_LIST:
            m_os << " [";
            لكل (ثابت AstNode* nodep = opp; nodep; nodep = nodep->nextp()) {
                𐤀𐤌 (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            اكسر;
        حالة VNTypeInfo::OP_OPTIONAL:
            𐤀𐤌 (opp) {
                m_os << " ";
                render(opp, depth);
            } وإلا {
                m_os << " nil";
            }
            اكسر;
        }
    }

    // Render the node into the stream.
    فراغ render(ثابت AstNode* nodep, uint32_t depth) {
        𐤀𐤌 (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            𐤀𐤌 (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } وإلا 𐤀𐤌 (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } وإلا {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } وإلا 𐤀𐤌 (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } وإلا 𐤀𐤌 (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } وإلا {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ثابت AstNodeDType* ثابت dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        𐤀𐤌 (!dtypep) {
            m_os << '?';
        } وإلا 𐤀𐤌 (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } وإلا {
            ثابت صحيح width = nodep->width();
            𐤀𐤌 (width == 1) {
                m_os << '1';
            } وإلا 𐤀𐤌 (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } وإلا {
                m_os << internWideWidth(width);
            }
            𐤀𐤌 (m_dumpWidthMin) {
                m_os << '/';
                ثابت صحيح widthMin = nodep->widthMin();
                𐤀𐤌 (widthMin == 1) {
                    m_os << '1';
                } وإلا 𐤀𐤌 (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } وإلا {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

عام:
    VNPatternString(std::ostream& os, ثابت AstNode* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string AstNodeExpr::patternString(uint32_t depth) ثابت {
    std::ostringstream oss;
    VNPatternString{oss, هذا, depth};
    أرجع oss.str();
}

//######################################################################
// VNDeleter

فراغ VNDeleter::doDeletes() {
    لكل (AstNode* ثابت nodep : m_deleteps) nodep->deleteTree();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#ضمن "V3Ast__gen_visitor_defns.h"  // From ./astgen
