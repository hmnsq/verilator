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

#شامل "V3PchAstMT.h"

#شامل "V3Broken.h"
#شامل "V3File.h"

#شامل <iomanip>
#شامل <memory>
#شامل <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_گره::s_editCntLast = 0;
uint64_t Ast_گره::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
صحیح Ast_گره::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

منطقی VNUser1InUse::s_userBusy = نادرست;
منطقی VNUser2InUse::s_userBusy = نادرست;
منطقی VNUser3InUse::s_userBusy = نادرست;
منطقی VNUser4InUse::s_userBusy = نادرست;

صحیح Ast_گره_DType::s_unique_رقم = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(ثابت string& name) {
    برای (صحیح i = 0; i < _ENUM_MAX; ++i) {
        ثابت VCMethod method{i};
        اگر (name == method.ascii()) بازگشت method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    بازگشت VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ثابت تهی*)) ثابت {
#ifdef VL_USER_TYPE_CHECKS
    اگر (ثابت uint64_t* ثابت uip = std::get_if<uint64_t>(&m_u)) بازگشت "#"s + cvtToStr(*uip);
    اگر (تهی* ثابت* ثابت upp = std::get_if<تهی*>(&m_u)) بازگشت fmtAddrp(*upp);
    بازگشت "";
#وگرنه
    // Dumps void* representation
    اگر (!m_u.up) بازگشت "";
    بازگشت fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

ثابت VNTypeInfo VNType::s_typeInfoTable[VNType::رقم_TYPES()] = {
#شامل "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

منطقی VFwdType::is_گره_Compatible(ثابت Ast_گره* nodep) ثابت {
    ثابت Ast_گره* defp = nodep;
    اگر (ثابت AstTypedef* ثابت adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    اگر (ثابت Ast_گره_DType* ثابت adefp = VN_CAST(defp, گره_DType))
        defp = adefp->skipRefToNonRefp();
    انتخاب (m_e) {
    حالت VFwdType::NONE: بازگشت درست; بشکن;
    حالت VFwdType::ENUM: بازگشت VN_IS(defp, EnumDType); بشکن;
    حالت VFwdType::STRUCT: بازگشت VN_IS(defp, StructDType); بشکن;
    حالت VFwdType::UNION: بازگشت VN_IS(defp, UnionDType); بشکن;
    حالت VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    حالت VFwdType::CLASS: بازگشت VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); بشکن;
    حالت VFwdType::GENERIC_INTERFACE: بازگشت VN_IS(defp, IfaceRefDType); بشکن;
    پیشفرض: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    بازگشت نادرست;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ثابت std::shared_ptr<ثابت string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
ثابت std::shared_ptr<ثابت string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(منطقی useSelfForThis, ثابت string& text) {
    بازگشت useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(منطقی useSelfForThis, منطقی protect) ثابت {
    بازگشت VIdProtect::protect_واژگان_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_گره::Ast_گره(VNType t, File_سطر* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = این;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = نادرست;
    m_flags.doingWidth = نادرست;
    m_flags.protect = درست;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_شمار_Inc();
}

Ast_گره* Ast_گره::abovep() ثابت {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), این, "abovep() not allowed when in midlist");
    ثابت Ast_گره* ثابت firstp = firstAbovep() ? این : m_headtailp;
    بازگشت firstp->backp();
}
Ast_گره* Ast_گره::aboveLoopp() ثابت {
    // Returns parent node. Avoid using this, may have performance issues.
    ثابت Ast_گره* nodep = این;
    // Backwards over peers (versus parents)
    وقتی (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    بازگشت nodep->backp();
}

string Ast_گره::encodeName(ثابت string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    برای (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        اگر ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } وگرنه اگر (pos[0] == '_') {
            out += pos[0];
            اگر (pos + 1 == namein.end()) بشکن;
            اگر (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } وگرنه {
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
    بازگشت vname.hashedName();
}

string Ast_گره::encode_عدد(int64_t رقم) {
    اگر (رقم < 0) بازگشت "__02D" + cvtToStr(-رقم);  // 2D=-
    بازگشت cvtToStr(رقم);
}

string Ast_گره::nameProtect() ثابت { بازگشت VIdProtect::protectIf(name(), protect()); }
string Ast_گره::origNameProtect() ثابت { بازگشت VIdProtect::protectIf(origName(), protect()); }

string Ast_گره::shortName() ثابت {
    string pretty = name();
    string::size_type pos;
    وقتی ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    بازگشت pretty;
}

string Ast_گره::dedotName(ثابت string& namein) {
    string pretty = namein;
    string::size_type pos;
    وقتی ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    اگر (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    بازگشت pretty;
}

string Ast_گره::nameNoArray(ثابت string& namein) {
    بازگشت namein.substr(0, namein.find("__BRA__"));
}

string Ast_گره::vcdName(ثابت string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    وقتی ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    وقتی ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    بازگشت prettyName(pretty);
}

string Ast_گره::prettyName(ثابت string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    برای (ثابت نویسه* pos = namein.c_str(); *pos;) {
        اگر (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            ادامه;
        }
        اگر (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            اگر (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                ادامه;
            }
            اگر (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                ادامه;
            }
            اگر (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                ادامه;
            }
            اگر (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                ادامه;
            }
            اگر (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                ادامه;
            }
            اگر (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                ادامه;
            }
            اگر (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                نویسه مقدار = 0;
                مقدار += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                مقدار
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += مقدار;
                pos += 5;
                ادامه;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    اگر (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    اگر (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    بازگشت pretty;
}

string Ast_گره::vpiName(ثابت string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ثابت size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    منطقی inEscapedIdent = نادرست;
    صحیح lastIdent = 0;

    برای (ثابت نویسه* pos = namein.c_str() + offset; *pos;) {
        نویسه specialChar = 0;
        اگر (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } وگرنه اگر (pos[0] == '_' && pos[1] == '_') {  // __
            اگر (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } وگرنه اگر (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } وگرنه اگر (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } وگرنه اگر (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                ادامه;
            } وگرنه اگر (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                نویسه مقدار = 0;
                مقدار += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                مقدار
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                اگر (مقدار != '_') inEscapedIdent = درست;

                pretty += مقدار;
                pos += 5;
                ادامه;
            }
        } وگرنه اگر (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        اگر (specialChar) {
            اگر (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.درج(lastIdent, "\\");
                inEscapedIdent = نادرست;
            }

            pretty += specialChar;

            اگر (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = نادرست;
            }
        } وگرنه {
            pretty += pos[0];
            ++pos;
        }
    }
    اگر (inEscapedIdent) {
        pretty += " ";
        pretty.درج(lastIdent, "\\");
    }
    اگر (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    اگر (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    بازگشت pretty;
}

string Ast_گره::prettyTypeName() ثابت {
    اگر (name() == "") بازگشت typeName();
    بازگشت std::string{typeName()} + " '" + pretty_Name_پیام() + "'";
}

//######################################################################
// Insertion

تهی Ast_گره::debug_درخت_Change(ثابت Ast_گره* nodep, ثابت نویسه* prefix, صحیح lineno, منطقی next) {
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
Ast_گره* Ast_گره::addNext<Ast_گره, Ast_گره>(Ast_گره* nodep, Ast_گره* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_درخت_Change(nodep, "-addNextThs: ", __LINE__, نادرست);
    debug_درخت_Change(newp, "-addNextNew: ", __LINE__, درست);
    اگر (!nodep)  // verilog.y and lots of other places assume this
        بازگشت newp;

    // Find end of old list
    Ast_گره* oldtailp = nodep;
    اگر (oldtailp->m_nextp) {
        اگر (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } وگرنه {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            وقتی (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_گره* ثابت newtailp = newp->m_headtailp;
    Ast_گره* ثابت headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_شمار_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_درخت_Change(nodep, "-addNextOut:", __LINE__, درست);
    بازگشت nodep;
}

تهی Ast_گره::addNextHere(Ast_گره* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_درخت_Change(این, "-addHereThs: ", __LINE__, نادرست);
    debug_درخت_Change(newp, "-addHereNew: ", __LINE__, درست);
    newp->edit_شمار_Inc();

    Ast_گره* ثابت addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_گره* ثابت oldnextp = این->m_nextp;
    این->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    اگر (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = این;

    // Head/tail
    Ast_گره* ثابت oldheadtailp = این->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    اگر (oldheadtailp) {  // else in middle of list, no change
        اگر (oldheadtailp == این) {  // this was one node
            این->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } وگرنه اگر (!oldnextp) {  // this was tail
            این->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_درخت_Change(این, "-addHereOut: ", __LINE__, درست);
}

تهی Ast_گره::setOp1p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, این, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_درخت_Change(این, "-setOp1pThs: ", __LINE__, نادرست);
    debug_درخت_Change(newp, "-setOp1pNew: ", __LINE__, درست);
    m_op1p = newp;
    newp->edit_شمار_Inc();
    newp->m_backp = این;
    debug_درخت_Change(این, "-setOp1pOut: ", __LINE__, نادرست);
}

تهی Ast_گره::setOp2p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, این, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_درخت_Change(این, "-setOp2pThs: ", __LINE__, نادرست);
    debug_درخت_Change(newp, "-setOp2pNew: ", __LINE__, درست);
    m_op2p = newp;
    newp->edit_شمار_Inc();
    newp->m_backp = این;
    debug_درخت_Change(این, "-setOp2pOut: ", __LINE__, نادرست);
}

تهی Ast_گره::setOp3p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, این, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_درخت_Change(این, "-setOp3pThs: ", __LINE__, نادرست);
    debug_درخت_Change(newp, "-setOp3pNew: ", __LINE__, درست);
    m_op3p = newp;
    newp->edit_شمار_Inc();
    newp->m_backp = این;
    debug_درخت_Change(این, "-setOp3pOut: ", __LINE__, نادرست);
}

تهی Ast_گره::setOp4p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, این, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_درخت_Change(این, "-setOp4pThs: ", __LINE__, نادرست);
    debug_درخت_Change(newp, "-setOp4pNew: ", __LINE__, درست);
    m_op4p = newp;
    newp->edit_شمار_Inc();
    newp->m_backp = این;
    debug_درخت_Change(این, "-setOp4pOut: ", __LINE__, نادرست);
}

تهی Ast_گره::addOp1p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op1p) {
        op1p(newp);
    } وگرنه {
        m_op1p->addNext(newp);
    }
}

تهی Ast_گره::addOp2p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op2p) {
        op2p(newp);
    } وگرنه {
        m_op2p->addNext(newp);
    }
}

تهی Ast_گره::addOp3p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op3p) {
        op3p(newp);
    } وگرنه {
        m_op3p->addNext(newp);
    }
}

تهی Ast_گره::addOp4p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op4p) {
        op4p(newp);
    } وگرنه {
        m_op4p->addNext(newp);
    }
}

تهی Ast_گره::prependOp1p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op1p) {
        op1p(newp);
    } وگرنه {
        m_op1p->addHereThisAsNext(newp);
    }
}

تهی Ast_گره::prependOp2p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op2p) {
        op2p(newp);
    } وگرنه {
        m_op2p->addHereThisAsNext(newp);
    }
}

تهی Ast_گره::prependOp3p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op3p) {
        op3p(newp);
    } وگرنه {
        m_op3p->addHereThisAsNext(newp);
    }
}

تهی Ast_گره::prependOp4p(Ast_گره* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op4p) {
        op4p(newp);
    } وگرنه {
        m_op4p->addHereThisAsNext(newp);
    }
}

تهی Ast_گره::replaceWith(Ast_گره* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    این->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
تهی Ast_گره::replaceWithKeepDType(Ast_گره* newp) {
    newp->dtypeFrom(این);
    replaceWith(newp);
}

تهی VNRelinker::dump(std::ostream& str) ثابت {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_گره* Ast_گره::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_درخت_Change(این, "-unlinkWNextThs: ", __LINE__, درست);
    Ast_گره* ثابت oldp = این;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_شمار_Inc();
    // cppcheck-suppress shadowFunction
    Ast_گره* ثابت backp = oldp->m_backp;
    اگر (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        اگر (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } وگرنه اگر (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } وگرنه اگر (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } وگرنه اگر (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } وگرنه اگر (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } وگرنه {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    اگر (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_گره* oldtailp = backp->m_headtailp;
        اگر (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            وقتی (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_گره* ثابت oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } وگرنه اگر (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } وگرنه اگر (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } وگرنه اگر (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } وگرنه اگر (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } وگرنه {
        این->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    اگر (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_درخت_Change(oldp, "-unlinkWNextOut: ", __LINE__, درست);
    بازگشت oldp;
}

Ast_گره* Ast_گره::unlinkFrBack(VNRelinker* linkerp) {
    debug_درخت_Change(این, "-unlinkFrBkThs: ", __LINE__, درست);
    Ast_گره* ثابت oldp = این;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_شمار_Inc();
    // cppcheck-suppress shadowFunction
    Ast_گره* ثابت backp = oldp->m_backp;
    اگر (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        اگر (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        اگر (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } وگرنه اگر (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } وگرنه اگر (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } وگرنه اگر (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } وگرنه اگر (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } وگرنه {
            این->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    اگر (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        اگر (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        اگر (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } وگرنه {
        اگر (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } وگرنه اگر (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } وگرنه اگر (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } وگرنه اگر (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } وگرنه {
            این->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        اگر (oldp->m_nextp) {
            Ast_گره* ثابت newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    اگر (oldp->m_iterpp) {  // Only if no linker, point to next in list
        اگر (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_درخت_Change(oldp, "-unlinkFrBkOut: ", __LINE__, درست);
    بازگشت oldp;
}

تهی Ast_گره::relink(VNRelinker* linkerp) {
    اگر (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_گره* ثابت newp = این;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_شمار_Inc();

    اگر (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_گره* ثابت backp = linkerp->m_backp;
    debug_درخت_Change(این, "-relinkNew: ", __LINE__, درست);
    debug_درخت_Change(backp, "-relinkTre: ", __LINE__, درست);

    انتخاب (linkerp->m_chg) {
    حالت VNRelinker::RELINK_NEXT: backp->addNextHere(newp); بشکن;
    حالت VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); بشکن;
    حالت VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); بشکن;
    حالت VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); بشکن;
    حالت VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); بشکن;
    پیشفرض: این->v3fatalSrc("Relink of node without any link to change."); بشکن;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    اگر (linkerp->m_iterpp) {
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
    debug_درخت_Change(این, "-relinkOut: ", __LINE__, درست);
}

تهی Ast_گره::relinkOneLink(Ast_گره*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_گره* newp) {
    اگر (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_گره* ثابت newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_گره* ثابت oldlistlastp = pointpr->m_headtailp;
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

تهی Ast_گره::addHereThisAsNext(Ast_گره* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(این->m_backp, این, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_گره* ثابت backp = این->m_backp;
    Ast_گره* ثابت newLastp = newp->m_headtailp;
    //
    این->edit_شمار_Inc();
    // Common linkage
    newLastp->m_nextp = این;
    این->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    اگر (backp && backp->m_nextp == این) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } وگرنه {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        اگر (backp) {
            اگر (backp->m_op1p == این) {
                backp->m_op1p = newp;
            } وگرنه اگر (backp->m_op2p == این) {
                backp->m_op2p = newp;
            } وگرنه اگر (backp->m_op3p == این) {
                backp->m_op3p = newp;
            } وگرنه {
                UASSERT_OBJ(backp->m_op4p == این, این, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_گره* ثابت tailp = این->m_headtailp;
        این->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    اگر (newLastp->m_iterpp) *(newLastp->m_iterpp) = این;
    اگر (این->m_iterpp) {
        *(این->m_iterpp) = newp;
        این->m_iterpp = nullptr;
    }
    //
    debug_درخت_Change(این, "-addHereThisAsNext: ", __LINE__, درست);
}

//======================================================================
// Clone

Ast_گره* Ast_گره::clone_درخت_Iter(منطقی needPure) {
    // private: Clone single node and children
    اگر (needPure) purityCheck();
    Ast_گره* ثابت newp = این->clone();
    اگر (این->m_op1p) newp->op1p(این->m_op1p->clone_درخت_Iter_List(needPure));
    اگر (این->m_op2p) newp->op2p(این->m_op2p->clone_درخت_Iter_List(needPure));
    اگر (این->m_op3p) newp->op3p(این->m_op3p->clone_درخت_Iter_List(needPure));
    اگر (این->m_op4p) newp->op4p(این->m_op4p->clone_درخت_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(این);  // Save pointers to/from both to simplify relinking.
    این->clonep(newp);  // Save pointers to/from both to simplify relinking.
    بازگشت newp;
}

Ast_گره* Ast_گره::clone_درخت_Iter_List(منطقی needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_گره* newheadp = nullptr;
    Ast_گره* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    برای (Ast_گره* oldp = این; oldp; oldp = oldp->m_nextp) {
        Ast_گره* ثابت newp = oldp->clone_درخت_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        اگر (newtailp) newtailp->m_nextp = newp;
        اگر (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    بازگشت newheadp;
}

Ast_گره* Ast_گره::clone_درخت(منطقی cloneNextLink, منطقی needPure) {
    debug_درخت_Change(این, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_درخت();
    Ast_گره* newp;
    اگر (cloneNextLink && این->m_nextp) {
        newp = clone_درخت_Iter_List(needPure);
    } وگرنه {
        newp = clone_درخت_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_درخت();
    debug_درخت_Change(newp, "-cloneOut: ", __LINE__, درست);
    بازگشت newp;
}

تهی Ast_گره::purityCheck() {
    اگر (VL_UNLIKELY(!isPure())) {
        این->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << این->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

تهی Ast_گره::delete_گره() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_شمار_Inc();
    // Change links of old node so we coredump if used
    این->m_nextp = reinterpret_cast<Ast_گره*>(0x1);
    این->m_backp = reinterpret_cast<Ast_گره*>(0x1);
    این->m_headtailp = reinterpret_cast<Ast_گره*>(0x1);
    این->m_op1p = reinterpret_cast<Ast_گره*>(0x1);
    این->m_op2p = reinterpret_cast<Ast_گره*>(0x1);
    این->m_op3p = reinterpret_cast<Ast_گره*>(0x1);
    این->m_op4p = reinterpret_cast<Ast_گره*>(0x1);
    این->m_iterpp = reinterpret_cast<Ast_گره**>(0x1);
    اگر (
#اگر !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        درست
#وگرنه
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete این;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

تهی Ast_گره::delete_درخت_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    برای (Ast_گره *nodep = این, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        اگر (nodep->m_op1p) nodep->m_op1p->delete_درخت_Iter();
        اگر (nodep->m_op2p) nodep->m_op2p->delete_درخت_Iter();
        اگر (nodep->m_op3p) nodep->m_op3p->delete_درخت_Iter();
        اگر (nodep->m_op4p) nodep->m_op4p->delete_درخت_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_گره();
    }
}

تهی Ast_گره::delete_درخت() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_درخت_Change(این, "-delTree:  ", __LINE__, درست);
    این->edit_شمار_Inc();
    // MUST be depth first!
    delete_درخت_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
تهی* Ast_گره::operator جدید(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_گره* ثابت objp = static_cast<Ast_گره*>(::operator جدید(size));
    V3Broken::addNewed(objp);
    بازگشت objp;
}

تهی Ast_گره::operator delete(تهی* objp, size_t size) {
    اگر (!objp) بازگشت;
    ثابت Ast_گره* ثابت nodep = static_cast<Ast_گره*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
تهی* Ast_گره::operator جدید(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ایستا ثابت size_t ASTGEN_MAX_گره_SIZE = []() {
        size_t maxSize = 0;
        برای (size_t t = 0; t < VNType::رقم_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        بازگشت maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ایستا uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    اگر (صحیح seed = v3Global.opt.debugAllocRandom()) {
        ایستا V3Mutex s_mutex;
        ثابت V3LockGuard lock{s_mutex};
        ایستا std::array<تهی*, POOL_SIZE> s_گره_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_گره_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        اگر (!s_lfsr) {
            s_lfsr = seed;
            اگر (!s_lfsr) s_lfsr = ~s_lfsr;
            برای (size_t i = 0; i < POOL_SIZE; ++i) {
                s_گره_Pool[i] = ::operator جدید(ASTGEN_MAX_گره_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_گره_Pool), std::end(s_گره_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ثابت size_t index = s_lfsr % POOL_SIZE;
        Ast_گره* ثابت objp = static_cast<Ast_گره*>(s_گره_Pool[index]);
        s_گره_Pool[index] = ::operator جدید(ASTGEN_MAX_گره_SIZE + 64);  // For later new()
        بازگشت objp;
    } وگرنه {
        Ast_گره* ثابت objp = static_cast<Ast_گره*>(::operator جدید(size));
        بازگشت objp;
    }
}
تهی Ast_گره::operator delete(تهی* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (تهی)objp;
    (تهی)size;
}
#endif

//======================================================================
// Iterators

تهی Ast_گره::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    اگر (m_op1p) m_op1p->iterateAndNext(v);
    اگر (m_op2p) m_op2p->iterateAndNext(v);
    اگر (m_op3p) m_op3p->iterateAndNext(v);
    اگر (m_op4p) m_op4p->iterateAndNext(v);
}

تهی Ast_گره::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    اگر (m_op1p) m_op1p->iterateAndNextConst(v);
    اگر (m_op2p) m_op2p->iterateAndNextConst(v);
    اگر (m_op3p) m_op3p->iterateAndNextConst(v);
    اگر (m_op4p) m_op4p->iterateAndNextConst(v);
}

تهی Ast_گره::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_گره* nodep = این;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    اگر (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    وقتی (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        اگر (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_گره* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        اگر (!niterp) بازگشت;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        اگر (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } وگرنه {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

تهی Ast_گره::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_گره* nodep = این;
    وقتی (nodep->m_nextp) nodep = nodep->m_nextp;
    وقتی (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        اگر (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } وگرنه {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

تهی Ast_گره::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    اگر (m_op1p) m_op1p->iterateListBackwardsConst(v);
    اگر (m_op2p) m_op2p->iterateListBackwardsConst(v);
    اگر (m_op3p) m_op3p->iterateListBackwardsConst(v);
    اگر (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

تهی Ast_گره::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_گره* nodep = این;
    انجامبده {
        Ast_گره* ثابت nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } وقتی (nodep);
}

Ast_گره* Ast_گره::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_گره* nodep = این;  // Note "this" may point to bogus point later in this function
    اگر (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } وگرنه اگر (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ثابت tempp = جدید AstBegin{nodep->fileline(), "[EditWrapper]", nodep, نادرست};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_درخت(), tempp);
    } وگرنه {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_گره** nextnodepp = nullptr;
        اگر (این->m_backp->m_op1p == این) {
            nextnodepp = &(این->m_backp->m_op1p);
        } وگرنه اگر (این->m_backp->m_op2p == این) {
            nextnodepp = &(این->m_backp->m_op2p);
        } وگرنه اگر (این->m_backp->m_op3p == این) {
            nextnodepp = &(این->m_backp->m_op3p);
        } وگرنه اگر (این->m_backp->m_op4p == این) {
            nextnodepp = &(این->m_backp->m_op4p);
        } وگرنه اگر (این->m_backp->m_nextp == این) {
            nextnodepp = &(این->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, این, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    بازگشت nodep;
}

//======================================================================

تهی Ast_گره::clone_Relink_درخت() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    برای (Ast_گره* nodep = این; nodep; nodep = nodep->m_nextp) {
        اگر (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        اگر (nodep->m_op1p) nodep->m_op1p->clone_Relink_درخت();
        اگر (nodep->m_op2p) nodep->m_op2p->clone_Relink_درخت();
        اگر (nodep->m_op3p) nodep->m_op3p->clone_Relink_درخت();
        اگر (nodep->m_op4p) nodep->m_op4p->clone_Relink_درخت();
    }
}

//======================================================================
// Comparison

منطقی Ast_گره::gate_درخت_Iter() ثابت {
    // private: Return true if the two trees are identical
    اگر (!isGateOptimizable()) بازگشت نادرست;
    اگر (m_op1p && !m_op1p->gate_درخت_Iter()) بازگشت نادرست;
    اگر (m_op2p && !m_op2p->gate_درخت_Iter()) بازگشت نادرست;
    اگر (m_op3p && !m_op3p->gate_درخت_Iter()) بازگشت نادرست;
    اگر (m_op4p && !m_op4p->gate_درخت_Iter()) بازگشت نادرست;
    بازگشت درست;
}

منطقی Ast_گره::same_درخت_Iter(ثابت Ast_گره* node1p, ثابت Ast_گره* node2p, منطقی ignNext,
                           منطقی gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    وقتی (درست) {
        اگر (!node1p && !node2p) بازگشت درست;
        اگر (!node1p || !node2p) بازگشت نادرست;
        اگر (node1p->type() != node2p->type()) بازگشت نادرست;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        اگر (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) بازگشت نادرست;
        اگر (!node1p->same_گره(node2p) || (gateOnly && !node1p->isGateOptimizable())) بازگشت نادرست;
        اگر (!same_درخت_Iter(node1p->m_op1p, node2p->m_op1p, نادرست, gateOnly)
            || !same_درخت_Iter(node1p->m_op2p, node2p->m_op2p, نادرست, gateOnly)
            || !same_درخت_Iter(node1p->m_op3p, node2p->m_op3p, نادرست, gateOnly)
            || !same_درخت_Iter(node1p->m_op4p, node2p->m_op4p, نادرست, gateOnly)) {
            بازگشت نادرست;
        }
        اگر (ignNext) بازگشت درست;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

تهی Ast_گره::check_درخت_Iter(ثابت Ast_گره* prevBackp) ثابت VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == این->backp(), این, "Back node inconsistent");
    ثابت VNTypeInfo& typeInfo = VNType::typeInfo(این->type());
    برای (صحیح i = 1; i <= 4; i++) {
        Ast_گره* nodep = nullptr;
        انتخاب (i) {
        حالت 1: nodep = op1p(); بشکن;
        حالت 2: nodep = op2p(); بشکن;
        حالت 3: nodep = op3p(); بشکن;
        حالت 4: nodep = op4p(); بشکن;
        پیشفرض: این->v3fatalSrc("Bad case"); بشکن;
        }
        // cppcheck-suppress danglingTempReference
        ثابت نویسه* opName = typeInfo.m_opNamep[i - 1];
        انتخاب (typeInfo.m_opType[i - 1]) {
        حالت VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, این, typeInfo.m_namep << " must not use " << opName << "()");
            بشکن;
        حالت VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, این,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), این,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_درخت_Iter(این);
            بشکن;
        حالت VNTypeInfo::OP_LIST:
            اگر (ثابت Ast_گره* ثابت headp = nodep) {
                // cppcheck-suppress shadowFunction
                ثابت Ast_گره* backp = این;
                ثابت Ast_گره* tailp;
                ثابت Ast_گره* opp = headp;
                انجامبده {
                    opp->check_درخت_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } وقتی (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            بشکن;
        حالت VNTypeInfo::OP_OPTIONAL:
            اگر (nodep) {
                UASSERT_OBJ(!nodep->nextp(), این,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_درخت_Iter(این);
            }
            بشکن;
        پیشفرض: این->v3fatalSrc("Bad case"); بشکن;
        }
    }
    اگر (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        اگر (ثابت Ast_گره_عبارت* ثابت exprp = VN_CAST(این, گره_عبارت)) {
            ثابت نویسه* ثابت whyp = exprp->widthMismatch();
            اگر (whyp) {
                auto dtypeStr = [](ثابت Ast_گره_عبارت* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    بازگشت ss.str();
                };
                اگر (ثابت Ast_گره_Uniop* ثابت uniopp = VN_CAST(exprp, گره_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } وگرنه اگر (ثابت Ast_گره_Biop* ثابت biopp = VN_CAST(exprp, گره_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } وگرنه {
                    UASSERT_OBJ(نادرست, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
نویسه* Ast_گره::dump_درخت_Json_Gdb(ثابت Ast_گره* nodep) {
    اگر (!nodep) بازگشت strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_درخت_Json(nodepStream);
    ثابت std::string str = nodepStream.rdbuf()->str();
    بازگشت strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
نویسه* Ast_گره::dump_درخت_Json_Gdb(ثابت نویسه* str) { بازگشت strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
نویسه* Ast_گره::dump_درخت_Json_Gdb(intptr_t nodep) {
    اگر (!nodep) بازگشت strdup("{\"addr\":\"NULL\"}\n");
    بازگشت dump_درخت_Json_Gdb(reinterpret_cast<ثابت Ast_گره*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
تهی Ast_گره::dumpGdb(ثابت Ast_گره* nodep) {  // For GDB only  // LCOV_EXCL_START
    اگر (!nodep) {
        cout << "<nullptr>" << endl;
        بازگشت;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
تهی Ast_گره::dumpGdbHeader() ثابت {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
تهی Ast_گره::dump_درخت_Gdb(ثابت Ast_گره* nodep) {  // For GDB only  // LCOV_EXCL_START
    اگر (!nodep) {
        cout << "<nullptr>" << endl;
        بازگشت;
    }
    nodep->dumpGdbHeader();
    nodep->dump_درخت(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
تهی Ast_گره::dump_درخت_File_Gdb(ثابت Ast_گره* nodep,  // LCOV_EXCL_START
                              ثابت نویسه* filenamep) {  // For GDB only
    اگر (!nodep) {
        cout << "<nullptr>" << endl;
        بازگشت;
    }
    ثابت string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_درخت_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
تهی Ast_گره::checkIter() ثابت {
    اگر (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        این->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

تهی Ast_گره::dumpPtrs(std::ostream& os) ثابت {
    os << "This=" << typeName() << " " << cvtToHex(این);
    os << " back=" << cvtToHex(backp());
    اگر (nextp()) os << " next=" << cvtToHex(nextp());
    اگر (m_headtailp == این) {
        os << " headtail=this";
    } وگرنه {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    اگر (op1p()) os << " op1p=" << cvtToHex(op1p());
    اگر (op2p()) os << " op2p=" << cvtToHex(op2p());
    اگر (op3p()) os << " op3p=" << cvtToHex(op3p());
    اگر (op4p()) os << " op4p=" << cvtToHex(op4p());
    ثابت auto dumpUser = [&os](ثابت نویسه* prefix, ثابت VNUser& user) {
        ثابت std::string s
            = user.dumpStr([](ثابت تهی* p) -> std::string { بازگشت cvtToHex(p); });
        اگر (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    اگر (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

تهی Ast_گره::dump_درخت(std::ostream& os, ثابت string& indent, صحیح maxDepth) ثابت {
    ایستا صحیح s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << این << '\n';
    اگر (VN_DELETED(این)) بازگشت;
    اگر (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    اگر (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(این)
           << endl;  // endl intentional to do flush
        بازگشت;
    }  // LCOV_EXCL_STOP
    اگر (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    اگر (maxDepth == 1) {
        اگر (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } وگرنه {
        برای (ثابت Ast_گره* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت(os, indent + "1:", maxDepth - 1);
        }
        برای (ثابت Ast_گره* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت(os, indent + "2:", maxDepth - 1);
        }
        برای (ثابت Ast_گره* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت(os, indent + "3:", maxDepth - 1);
        }
        برای (ثابت Ast_گره* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت(os, indent + "4:", maxDepth - 1);
        }
    }
}

تهی Ast_گره::dump_درخت_And_Next(std::ostream& os, ثابت string& indent, صحیح maxDepth) ثابت {
    // Audited to make sure this is never nullptr
    برای (ثابت Ast_گره* nodep = این; nodep; nodep = nodep->nextp()) {
        nodep->dump_درخت(os, indent, maxDepth);
    }
}

تهی Ast_گره::dump_درخت_File(ثابت string& filename, منطقی doDump) {
    // Not const function as calls checkTree
    اگر (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            اگر (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_شمار_Last();
            *logsp << "> to <e" << std::dec << edit_شمار_Gbl() << ">\n";
            اگر (edit_شمار_Gbl() == edit_شمار_Last() && ::dump_درخت_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } وگرنه {
                dump_درخت(*logsp);
                edit_شمار_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

ایستا تهی drawChildren(std::ostream& os, ثابت Ast_گره* thisp, ثابت Ast_گره* childp,
                         ثابت std::string& childName) {
    اگر (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        برای (ثابت Ast_گره* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت_Dot(os);
            اگر (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

تهی Ast_گره::dump_درخت_Dot(std::ostream& os) ثابت {
    os << "\tn" << cvtToHex(این) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, این, m_op1p, "op1");
    drawChildren(os, این, m_op2p, "op2");
    drawChildren(os, این, m_op3p, "op3");
    drawChildren(os, این, m_op4p, "op4");
}

تهی Ast_گره::dump_درخت_Json_File(ثابت string& filename, منطقی doDump) {
    اگر (!doDump) بازگشت;
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    اگر (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_درخت_Json(*treejsonp);
    *treejsonp << '\n';
}

تهی Ast_گره::dumpJsonMetaFileGdb(ثابت نویسه* filename) { dumpJsonMetaFile(filename); }
تهی Ast_گره::dumpJsonMetaFile(ثابت string& filename) {
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    اگر (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_سطر::file_Name_رقم_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

تهی Ast_گره::dump_درخت_Dot_File(ثابت string& filename, منطقی doDump) {
    اگر (doDump) {
        UINFO(2, "Dumping " << filename);
        ثابت std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        اگر (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_درخت_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_گره::instanceStr() ثابت {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_شمار = 0;
    // Walk 'backp' chain
    برای (ثابت Ast_گره* currp = این; currp; currp = currp->backp(), ++iter_شمار) {
        اگر (VL_UNCOVERABLE(iter_شمار >= maxIterations)) بازگشت "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        اگر (ثابت AstScope* ثابت scopep = VN_CAST(currp, Scope)) {
            بازگشت scopep->is_بالاترین() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        اگر (ثابت AstModule* ثابت modp = VN_CAST(currp, Module)) {
            ثابت string instanceName = modp->someInstanceName();
            بازگشت instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    بازگشت "";
}
تهی Ast_گره::v3errorEnd(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    اگر (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } وگرنه {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ثابت string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        اگر (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_گره*>(این)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
تهی Ast_گره::v3errorEndFatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

تهی Ast_گره::dtypeChgSigned(منطقی flag) {
    UASSERT_OBJ(dtypep(), این, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
تهی Ast_گره::dtypeChgWidth(صحیح width, صحیح widthMin) {
    UASSERT_OBJ(dtypep(), این, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

تهی Ast_گره::dtypeChgWidthSigned(صحیح width, صحیح widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), این, "No dtype when changing width");
    ثابت Ast_گره_DType* ثابت enumDTypep = dtypep()->skipRefToEnump();
    اگر (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        بازگشت;  // Correct already
    }
    ثابت Ast_گره_DType* ثابت dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_گره_DType* Ast_گره::findBasicDType(VBasicDTypeKwd kwd) ثابت {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    بازگشت v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_گره_DType* Ast_گره::findBitDType(صحیح width, صحیح widthMin, VSigning numeric) ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_گره_DType* Ast_گره::findLogicDType(صحیح width, صحیح widthMin, VSigning numeric) ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_گره_DType* Ast_گره::findLogic2StateDType(صحیح width, صحیح widthMin, VSigning numeric) ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_گره_DType* Ast_گره::findLogicRangeDType(ثابت VNumRange& range, صحیح widthMin,
                                           VSigning numeric) ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_گره_DType* Ast_گره::findBitRangeDType(ثابت VNumRange& range, صحیح widthMin,
                                         VSigning numeric) ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_گره::find_درج_Same_DType(AstBasicDType* nodep) {
    بازگشت v3Global.rootp()->typeTablep()->find_درج_Same_DType(nodep);
}
Ast_گره_DType* Ast_گره::findConstraintRefDType() ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_گره_DType* Ast_گره::findEmptyQueueDType() ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_گره_DType* Ast_گره::findQueueIndexDType() ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_گره_DType* Ast_گره::findStreamDType() ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_گره_DType* Ast_گره::findVoidDType() ثابت {
    بازگشت v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

ایستا ثابت Ast_گره_DType* computeCastableBase(ثابت Ast_گره_DType* nodep) {
    وقتی (درست) {
        اگر (ثابت AstPackArrayDType* ثابت packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            ادامه;
        }
        اگر (ثابت Ast_گره_DType* ثابت refp = nodep->skipRefToEnump()) {
            اگر (refp != nodep) {
                nodep = refp;
                ادامه;
            }
        }
        بازگشت nodep;
    }
}

ایستا VCastable computeCastableImp(ثابت Ast_گره_DType* toDtp, ثابت Ast_گره_DType* fromDtp,
                                    ثابت Ast_گره* fromConstp, ثابت منطقی checkIfaceArgCompat) {
    ثابت VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    اگر (toDtp == fromDtp) بازگشت VCastable::SAMEISH;
    اگر (toDtp->similarDType(fromDtp)) بازگشت VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ثابت Ast_گره_DType* fromBaseDtp = computeCastableBase(fromDtp);

    ثابت منطقی fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, گره_UOr_Struct_DType);

    ثابت Ast_گره_DType* toBaseDtp = computeCastableBase(toDtp);
    ثابت منطقی toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, گره_UOr_Struct_DType);

    اگر (toBaseDtp == fromBaseDtp) بازگشت VCastable::COMPATIBLE;

    اگر (toNumericable) {
        اگر (fromNumericable) بازگشت VCastable::COMPATIBLE;
    } وگرنه اگر (VN_IS(toBaseDtp, EnumDType)) {
        اگر (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_درخت(fromDtp))
            بازگشت VCastable::ENUM_IMPLICIT;
        اگر (fromNumericable) بازگشت VCastable::ENUM_EXPLICIT;
    } وگرنه اگر (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        بازگشت VCastable::COMPATIBLE;
    } وگرنه اگر (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        اگر (fromConstp->isNull()) بازگشت VCastable::COMPATIBLE;
    } وگرنه اگر (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ثابت AstClass* ثابت toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        ثابت AstClass* ثابت fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ثابت منطقی downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        ثابت منطقی upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        اگر (upcast) بازگشت VCastable::COMPATIBLE;
        اگر (downcast) بازگشت VCastable::DYNAMIC_CLASS;
        بازگشت VCastable::INCOMPATIBLE;
    } وگرنه اگر (ثابت AstIfaceRefDType* ثابت toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(درست) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ثابت AstIfaceRefDType* ثابت fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(درست) : fromDtp, IfaceRefDType);
        اگر (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            ثابت AstUnpackArrayDType* ثابت toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            اگر (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                بازگشت VCastable::INCOMPATIBLE;
            }
            بازگشت computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        اگر (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            اگر (!checkIfaceArgCompat) بازگشت castable;
            بازگشت VCastable::INCOMPATIBLE;
        }
        ثابت منطقی sameModport = toIfp->modportp() == fromIfp->modportp();
        اگر (!checkIfaceArgCompat) {
            اگر (!toIfp->modportp() || sameModport) بازگشت VCastable::COMPATIBLE;
            بازگشت castable;
        }
        اگر (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            بازگشت VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        اگر (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            بازگشت VCastable::COMPATIBLE;
        }
        بازگشت VCastable::INCOMPATIBLE;
    } وگرنه اگر (ثابت AstUnpackArrayDType* ثابت toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        اگر (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            بازگشت castable;
        }
        ثابت VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ثابت Ast_گره_DType* ثابت toSubp = toArrayp->subDTypep()->skipRefp();
        اگر (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            بازگشت VCastable::COMPATIBLE;
        }
    }
    بازگشت castable;
}

VCastable Ast_گره::computeCastable(ثابت Ast_گره_DType* toDtp, ثابت Ast_گره_DType* fromDtp,
                                   ثابت Ast_گره* fromConstp, ثابت منطقی checkIfaceArgCompat) {
    ثابت auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    اگر (fromConstp) UINFO(9, "     const= " << fromConstp);
    بازگشت castable;
}

Ast_گره_DType* Ast_گره::getCommonClassTypep(Ast_گره* node1p, Ast_گره* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    اگر (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        ثابت VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        اگر (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            بازگشت node1p->dtypep();
        اگر (castable == VCastable::DYNAMIC_CLASS) بازگشت node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    وقتی (classDtypep1) {
        ثابت VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        اگر (castable == VCastable::COMPATIBLE) بازگشت classDtypep1;
        AstClassExtends* ثابت extendsp = classDtypep1->classp()->extendsp();
        اگر (!extendsp) بشکن;
        Ast_گره_DType* ثابت edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    بازگشت nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

رده VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<صحیح, std::string> m_internedWordWidths;  // Interned widths
    std::map<صحیح, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ثابت منطقی m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    ایستا std::string toLetters(size_t مقدار, منطقی lowerCase = نادرست) {
        ثابت نویسه base = lowerCase ? 'a' : 'A';
        std::string s;
        انجامبده { s += static_cast<نویسه>(base + مقدار % 26); } وقتی (مقدار /= 26);
        بازگشت s;
    }

    ثابت std::string& internConst(ثابت AstConst& nodep) {
        ثابت auto pair = m_internedConsts.emplace(nodep.رقم().ascii(نادرست), "");
        اگر (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        بازگشت pair.first->second;
    }

    ثابت std::string& internWordWidth(صحیح مقدار) {
        ثابت auto pair = m_internedWordWidths.emplace(مقدار, "");
        اگر (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, درست);
        بازگشت pair.first->second;
    }

    ثابت std::string& internWideWidth(صحیح مقدار) {
        ثابت auto pair = m_internedWideWidths.emplace(مقدار, "");
        اگر (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        بازگشت pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ایستا منطقی isLeaf(ثابت Ast_گره* nodep) {
        ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        برای (ثابت VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            اگر (opType != VNTypeInfo::OP_UNUSED) بازگشت نادرست;
        }
        بازگشت درست;
    }

    // Render operand, return true if not unused
    تهی renderOperand(VNTypeInfo::OpEn opType, ثابت Ast_گره* ثابت opp, uint32_t depth) {
        انتخاب (opType) {
        حالت VNTypeInfo::OP_UNUSED:  //
            بشکن;
        حالت VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            بشکن;
        حالت VNTypeInfo::OP_LIST:
            m_os << " [";
            برای (ثابت Ast_گره* nodep = opp; nodep; nodep = nodep->nextp()) {
                اگر (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            بشکن;
        حالت VNTypeInfo::OP_OPTIONAL:
            اگر (opp) {
                m_os << " ";
                render(opp, depth);
            } وگرنه {
                m_os << " nil";
            }
            بشکن;
        }
    }

    // Render the node into the stream.
    تهی render(ثابت Ast_گره* nodep, uint32_t depth) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            اگر (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } وگرنه اگر (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } وگرنه {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } وگرنه اگر (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } وگرنه اگر (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } وگرنه {
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
        ثابت Ast_گره_DType* ثابت dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        اگر (!dtypep) {
            m_os << '?';
        } وگرنه اگر (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } وگرنه {
            ثابت صحیح width = nodep->width();
            اگر (width == 1) {
                m_os << '1';
            } وگرنه اگر (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } وگرنه {
                m_os << internWideWidth(width);
            }
            اگر (m_dumpWidthMin) {
                m_os << '/';
                ثابت صحیح widthMin = nodep->widthMin();
                اگر (widthMin == 1) {
                    m_os << '1';
                } وگرنه اگر (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } وگرنه {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

عمومی:
    VNPatternString(std::ostream& os, ثابت Ast_گره* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_گره_عبارت::patternString(uint32_t depth) ثابت {
    std::ostringstream oss;
    VNPatternString{oss, این, depth};
    بازگشت oss.str();
}

//######################################################################
// VNDeleter

تهی VNDeleter::doDeletes() {
    برای (Ast_گره* ثابت nodep : m_deleteps) nodep->delete_درخت();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#شامل "V3Ast__gen_visitor_defns.h"  // From ./astgen
