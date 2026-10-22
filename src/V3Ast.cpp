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

uint64_t Ast_غوټه::s_edit_Cnt_وروستی = 0;
uint64_t Ast_غوټه::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
صحيح Ast_غوټه::s_cloneCntGbl = 0;
uint32_t VNUser1_دننه_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2_دننه_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3_دننه_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4_دننه_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent

منطقي VNUser1_دننه_Use::s_userBusy = خطأ;
منطقي VNUser2_دننه_Use::s_userBusy = خطأ;
منطقي VNUser3_دننه_Use::s_userBusy = خطأ;
منطقي VNUser4_دننه_Use::s_userBusy = خطأ;

صحيح Ast_غوټه_DType::s_unique_عدد = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::array_لاره(ثابت string& name) {
    لكل (صحيح i = 0; i < _ENUM_MAX; ++i) {
        ثابت VCMethod لاره{i};
        إذا (name == لاره.ascii()) أرجع لاره;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    أرجع VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ثابت فراغ*)) ثابت {
#که_تعریف_شوی VL_USER_TYPE_CHECKS
    إذا (ثابت uint64_t* ثابت uip = std::get_if<uint64_t>(&m_u)) أرجع "#"s + cvt_منزل_Str(*uip);
    إذا (فراغ* ثابت* ثابت upp = std::get_if<فراغ*>(&m_u)) أرجع fmtAddrp(*upp);
    أرجع "";
#وإلا
    // Dumps void* representation
    إذا (!m_u.پورته) أرجع "";
    أرجع fmtAddrp(m_u.پورته);
#د_که_پای
}

//######################################################################
// VNType

ثابت VNTypeInfo VNType::s_type_Info_جدول[VNType::عدد_TYPES()] = {
#ضمن "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& چلوونکی<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

منطقي VFwdType::is_غوټه_Compatible(ثابت Ast_غوټه* nodep) ثابت {
    ثابت Ast_غوټه* defp = nodep;
    إذا (ثابت AstTypedef* ثابت adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    إذا (ثابت Ast_غوټه_DType* ثابت adefp = VN_CAST(defp, غوټه_DType))
        defp = adefp->skip_Ref_منزل_Non_Refp();
    بدل (m_e) {
    حالة VFwdType::NONE: أرجع ریښتیا; اكسر;
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

ثابت std::shared_ptr<ثابت string> VSelf_اشاره_ګر_متن::s_emptyp = std::make_shared<string>("");
ثابت std::shared_ptr<ثابت string> VSelf_اشاره_ګر_متن::s_thisp = std::make_shared<string>("this");

string VSelf_اشاره_ګر_متن::replaceThis(منطقي useSelfForThis, ثابت string& متن) {
    أرجع useSelfForThis ? VString::replace_کلمه(متن, "this", "vlSelf") : متن;
}

string VSelf_اشاره_ګر_متن::protect(منطقي useSelfForThis, منطقي protect) ثابت {
    أرجع VIdProtect::protect_ټکي_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_غوټه::Ast_غوټه(VNType t, File_کرښه* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = هذا;  // When made, we're a list of only a single element
    //ځانګړتیاوې
    m_flags.did_پلنوالی = خطأ;
    m_flags.doing_پلنوالی = خطأ;
    m_flags.protect = ریښتیا;
    m_flags.ناکارول_شوی = 0;  // Initializing this avoids a read-modify-write on construction
    edit_شمېر_Inc();
}

Ast_غوټه* Ast_غوټه::abovep() ثابت {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || لومړی_Abovep(), هذا, "abovep() not allowed when in midlist");
    ثابت Ast_غوټه* ثابت firstp = لومړی_Abovep() ? هذا : m_headtailp;
    أرجع firstp->backp();
}
Ast_غوټه* Ast_غوټه::aboveLoopp() ثابت {
    // Returns parent node. Avoid using this, may have performance issues.
    ثابت Ast_غوټه* nodep = هذا;
    // Backwards over peers (versus parents)
    طالما (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    أرجع nodep->backp();
}

string Ast_غوټه::encodeName(ثابت string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string بهر;
    بهر.reserve(namein.اندازه());
    لكل (خپلکاری pos = namein.begin(); pos != namein.پای_ټکی(); ++pos) {
        إذا ((pos == namein.begin()) ? std::که_توری(pos[0])  // digits can't lead identifiers
                                    : std::که_توری_یا_عدد(pos[0])) {
            بهر += pos[0];
        } وإلا إذا (pos[0] == '_') {
            بهر += pos[0];
            إذا (pos + 1 == namein.پای_ټکی()) اكسر;
            إذا (pos[1] == '_') {
                ++pos;
                بهر += "__05F";  // hex(_) = 0x5F
            }
        } وإلا {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ثابت بینښه val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            بهر += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{بهر};
    أرجع vname.hashedName();
}

string Ast_غوټه::encode_شمېره(int64_t عدد) {
    إذا (عدد < 0) أرجع "__02D" + cvt_منزل_Str(-عدد);  // 2D=-
    أرجع cvt_منزل_Str(عدد);
}

string Ast_غوټه::nameProtect() ثابت { أرجع VIdProtect::protectIf(name(), protect()); }
string Ast_غوټه::origNameProtect() ثابت { أرجع VIdProtect::protectIf(origName(), protect()); }

string Ast_غوټه::shortName() ثابت {
    string pretty = name();
    string::اندازه_type pos;
    طالما ((pos = pretty.پیدا_کړه("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    أرجع pretty;
}

string Ast_غوټه::dedotName(ثابت string& namein) {
    string pretty = namein;
    string::اندازه_type pos;
    طالما ((pos = pretty.پیدا_کړه("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    إذا (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    أرجع pretty;
}

string Ast_غوټه::nameNoArray(ثابت string& namein) {
    أرجع namein.substr(0, namein.پیدا_کړه("__BRA__"));
}

string Ast_غوټه::vcdName(ثابت string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::اندازه_type pos;
    طالما ((pos = pretty.پیدا_کړه("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    طالما ((pos = pretty.پیدا_کړه('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    أرجع prettyName(pretty);
}

string Ast_غوټه::prettyName(ثابت string& namein) VL_خالص {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.اوږدتیا());
    لكل (ثابت حرف* pos = namein.c_str(); *pos;) {
        إذا (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            استمر;
        }
        إذا (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            إذا (0 == std::مزي_پرتله(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                استمر;
            }
            إذا (0 == std::مزي_پرتله(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                استمر;
            }
            إذا (0 == std::مزي_پرتله(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                استمر;
            }
            إذا (0 == std::مزي_پرتله(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                استمر;
            }
            إذا (0 == std::مزي_پرتله(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                استمر;
            }
            إذا (0 == std::مزي_پرتله(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                استمر;
            }
            إذا (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                حرف ارزښت = 0;
                ارزښت += 16
                         * (std::که_عدد(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ارزښت
                    += (std::که_عدد(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += ارزښت;
                pos += 5;
                استمر;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    إذا (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    إذا (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    أرجع pretty;
}

string Ast_غوټه::vpiName(ثابت string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ثابت اندازه_t دننه_واټن = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.اوږدتیا());
    منطقي دننه_Escaped_Ident = خطأ;
    صحيح وروستی_Ident = 0;

    لكل (ثابت حرف* pos = namein.c_str() + دننه_واټن; *pos;) {
        حرف specialChar = 0;
        إذا (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } وإلا إذا (pos[0] == '_' && pos[1] == '_') {  // __
            إذا (0 == std::مزي_پرتله(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } وإلا إذا (0 == std::مزي_پرتله(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } وإلا إذا (0 == std::مزي_پرتله(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } وإلا إذا (0 == std::مزي_پرتله(pos, "__PVT__", 7)) {
                pos += 7;
                استمر;
            } وإلا إذا (0 == std::مزي_پرتله(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                حرف ارزښت = 0;
                ارزښت += 16
                         * (std::که_عدد(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ارزښت
                    += (std::که_عدد(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                إذا (ارزښت != '_') دننه_Escaped_Ident = ریښتیا;

                pretty += ارزښت;
                pos += 5;
                استمر;
            }
        } وإلا إذا (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        إذا (specialChar) {
            إذا (دننه_Escaped_Ident && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.ورننباسه(وروستی_Ident, "\\");
                دننه_Escaped_Ident = خطأ;
            }

            pretty += specialChar;

            إذا (specialChar == ']' || specialChar == '.') {
                وروستی_Ident = pretty.اوږدتیا();
                دننه_Escaped_Ident = خطأ;
            }
        } وإلا {
            pretty += pos[0];
            ++pos;
        }
    }
    إذا (دننه_Escaped_Ident) {
        pretty += " ";
        pretty.ورننباسه(وروستی_Ident, "\\");
    }
    إذا (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    إذا (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    أرجع pretty;
}

string Ast_غوټه::prettyTypeName() ثابت {
    إذا (name() == "") أرجع typeName();
    أرجع std::string{typeName()} + " '" + pretty_Name_پیغام() + "'";
}

//######################################################################
// Insertion

فراغ Ast_غوټه::debug_ونه_Change(ثابت Ast_غوټه* nodep, ثابت حرف* مختاړی, صحيح lineno, منطقي بل) {
#که_تعریف_شوی VL_DEBUG
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
#د_که_پای
}

کينډۍ <>
Ast_غوټه* Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(Ast_غوټه* nodep, Ast_غوټه* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_ونه_Change(nodep, "-addNextThs: ", __LINE__, خطأ);
    debug_ونه_Change(newp, "-addNextNew: ", __LINE__, ریښتیا);
    إذا (!nodep)  // verilog.y and lots of other places assume this
        أرجع newp;

    // Find end of old list
    Ast_غوټه* oldtailp = nodep;
    إذا (oldtailp->m_nextp) {
        إذا (oldtailp->m_headtailp) {
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
    Ast_غوټه* ثابت newtailp = newp->m_headtailp;
    Ast_غوټه* ثابت headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = تش_اشاره;  // May be written again as new head
    newp->m_headtailp = تش_اشاره;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_شمېر_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_ونه_Change(nodep, "-addNextOut:", __LINE__, ریښتیا);
    أرجع nodep;
}

فراغ Ast_غوټه::جمع_کړه_بل_دلته(Ast_غوټه* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_ونه_Change(هذا, "-addHereThs: ", __LINE__, خطأ);
    debug_ونه_Change(newp, "-addHereNew: ", __LINE__, ریښتیا);
    newp->edit_شمېر_Inc();

    Ast_غوټه* ثابت addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_غوټه* ثابت oldnextp = هذا->m_nextp;
    هذا->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    إذا (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = هذا;

    // Head/tail
    Ast_غوټه* ثابت oldheadtailp = هذا->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = تش_اشاره;  // Not at head any longer
    addlastp->m_headtailp = تش_اشاره;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    إذا (oldheadtailp) {  // else in middle of list, no change
        إذا (oldheadtailp == هذا) {  // this was one node
            هذا->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } وإلا إذا (!oldnextp) {  // this was tail
            هذا->m_headtailp = تش_اشاره;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_ونه_Change(هذا, "-addHereOut: ", __LINE__, ریښتیا);
}

فراغ Ast_غوټه::سېټ_Op1p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, هذا, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_ونه_Change(هذا, "-setOp1pThs: ", __LINE__, خطأ);
    debug_ونه_Change(newp, "-setOp1pNew: ", __LINE__, ریښتیا);
    m_op1p = newp;
    newp->edit_شمېر_Inc();
    newp->m_backp = هذا;
    debug_ونه_Change(هذا, "-setOp1pOut: ", __LINE__, خطأ);
}

فراغ Ast_غوټه::سېټ_Op2p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, هذا, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_ونه_Change(هذا, "-setOp2pThs: ", __LINE__, خطأ);
    debug_ونه_Change(newp, "-setOp2pNew: ", __LINE__, ریښتیا);
    m_op2p = newp;
    newp->edit_شمېر_Inc();
    newp->m_backp = هذا;
    debug_ونه_Change(هذا, "-setOp2pOut: ", __LINE__, خطأ);
}

فراغ Ast_غوټه::سېټ_Op3p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, هذا, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_ونه_Change(هذا, "-setOp3pThs: ", __LINE__, خطأ);
    debug_ونه_Change(newp, "-setOp3pNew: ", __LINE__, ریښتیا);
    m_op3p = newp;
    newp->edit_شمېر_Inc();
    newp->m_backp = هذا;
    debug_ونه_Change(هذا, "-setOp3pOut: ", __LINE__, خطأ);
}

فراغ Ast_غوټه::سېټ_Op4p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, هذا, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_ونه_Change(هذا, "-setOp4pThs: ", __LINE__, خطأ);
    debug_ونه_Change(newp, "-setOp4pNew: ", __LINE__, ریښتیا);
    m_op4p = newp;
    newp->edit_شمېر_Inc();
    newp->m_backp = هذا;
    debug_ونه_Change(هذا, "-setOp4pOut: ", __LINE__, خطأ);
}

فراغ Ast_غوټه::جمع_کړه_Op1p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op1p) {
        op1p(newp);
    } وإلا {
        m_op1p->جمع_کړه_بل(newp);
    }
}

فراغ Ast_غوټه::جمع_کړه_Op2p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op2p) {
        op2p(newp);
    } وإلا {
        m_op2p->جمع_کړه_بل(newp);
    }
}

فراغ Ast_غوټه::جمع_کړه_Op3p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op3p) {
        op3p(newp);
    } وإلا {
        m_op3p->جمع_کړه_بل(newp);
    }
}

فراغ Ast_غوټه::جمع_کړه_Op4p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op4p) {
        op4p(newp);
    } وإلا {
        m_op4p->جمع_کړه_بل(newp);
    }
}

فراغ Ast_غوټه::prependOp1p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op1p) {
        op1p(newp);
    } وإلا {
        m_op1p->جمع_کړه_دلته_This_As_بل(newp);
    }
}

فراغ Ast_غوټه::prependOp2p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op2p) {
        op2p(newp);
    } وإلا {
        m_op2p->جمع_کړه_دلته_This_As_بل(newp);
    }
}

فراغ Ast_غوټه::prependOp3p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op3p) {
        op3p(newp);
    } وإلا {
        m_op3p->جمع_کړه_دلته_This_As_بل(newp);
    }
}

فراغ Ast_غوټه::prependOp4p(Ast_غوټه* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op4p) {
        op4p(newp);
    } وإلا {
        m_op4p->جمع_کړه_دلته_This_As_بل(newp);
    }
}

فراغ Ast_غوټه::replaceWith(Ast_غوټه* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    هذا->unlink_Fr_شا(&repHandle);
    repHandle.relink(newp);
}
فراغ Ast_غوټه::replaceWithKeepDType(Ast_غوټه* newp) {
    newp->dtypeFrom(هذا);
    replaceWith(newp);
}

فراغ VNRelinker::dump(std::ostream& str) ثابت {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_بل ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_غوټه* Ast_غوټه::unlink_Fr_شا_With_بل(VNRelinker* linkerp) {
    debug_ونه_Change(هذا, "-unlinkWNextThs: ", __LINE__, ریښتیا);
    Ast_غوټه* ثابت oldp = هذا;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_شمېر_Inc();
    // cppcheck-suppress shadowFunction
    Ast_غوټه* ثابت backp = oldp->m_backp;
    إذا (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        إذا (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_بل;
        } وإلا إذا (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } وإلا إذا (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } وإلا إذا (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } وإلا إذا (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } وإلا {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    إذا (backp->m_nextp == oldp) {
        backp->m_nextp = تش_اشاره;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_غوټه* oldtailp = backp->m_headtailp;
        إذا (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            طالما (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_غوټه* ثابت oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } وإلا إذا (backp->m_op1p == oldp) {
        backp->m_op1p = تش_اشاره;
    } وإلا إذا (backp->m_op2p == oldp) {
        backp->m_op2p = تش_اشاره;
    } وإلا إذا (backp->m_op3p == oldp) {
        backp->m_op3p = تش_اشاره;
    } وإلا إذا (backp->m_op4p == oldp) {
        backp->m_op4p = تش_اشاره;
    } وإلا {
        هذا->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = تش_اشاره;
    // Iterator fixup
    إذا (oldp->m_iterpp) {
        *(oldp->m_iterpp) = تش_اشاره;
        oldp->m_iterpp = تش_اشاره;
    }
    debug_ونه_Change(oldp, "-unlinkWNextOut: ", __LINE__, ریښتیا);
    أرجع oldp;
}

Ast_غوټه* Ast_غوټه::unlink_Fr_شا(VNRelinker* linkerp) {
    debug_ونه_Change(هذا, "-unlinkFrBkThs: ", __LINE__, ریښتیا);
    Ast_غوټه* ثابت oldp = هذا;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_شمېر_Inc();
    // cppcheck-suppress shadowFunction
    Ast_غوټه* ثابت backp = oldp->m_backp;
    إذا (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        إذا (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = تش_اشاره;
            oldp->m_iterpp = تش_اشاره;
        }
        إذا (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_بل;
        } وإلا إذا (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } وإلا إذا (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } وإلا إذا (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } وإلا إذا (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } وإلا {
            هذا->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    إذا (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        إذا (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        إذا (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } وإلا {
        إذا (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } وإلا إذا (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } وإلا إذا (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } وإلا إذا (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } وإلا {
            هذا->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        إذا (oldp->m_nextp) {
            Ast_غوټه* ثابت newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    إذا (oldp->m_iterpp) {  // Only if no linker, point to next in list
        إذا (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = تش_اشاره;
    }
    // Relink
    oldp->m_nextp = تش_اشاره;
    oldp->m_backp = تش_اشاره;
    oldp->m_headtailp = oldp;
    debug_ونه_Change(oldp, "-unlinkFrBkOut: ", __LINE__, ریښتیا);
    أرجع oldp;
}

فراغ Ast_غوټه::relink(VNRelinker* linkerp) {
    إذا (debug() > 8) {
        UINFO_مختاړی(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_غوټه* ثابت newp = هذا;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_شمېر_Inc();

    إذا (debug() > 8) {
        linkerp->dump(وتنه);
        وتنه << کرښه_پای;
    }

    // cppcheck-suppress shadowFunction
    Ast_غوټه* ثابت backp = linkerp->m_backp;
    debug_ونه_Change(هذا, "-relinkNew: ", __LINE__, ریښتیا);
    debug_ونه_Change(backp, "-relinkTre: ", __LINE__, ریښتیا);

    بدل (linkerp->m_chg) {
    حالة VNRelinker::RELINK_بل: backp->جمع_کړه_بل_دلته(newp); اكسر;
    حالة VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); اكسر;
    افتراضي: هذا->v3fatalSrc("Relink of node without any link to change."); اكسر;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = تش_اشاره;
    // Iterator fixup
    إذا (linkerp->m_iterpp) {
        // If we're iterating over a next() link, we need to follow links off the
        // NEW node, which is always assumed to be what we are relinking to.
        // This adds a unfortunate hot 8 bytes to every AstNode, but is faster than passing
        // across every function.
        // If anyone has a cleaner way, I'd be grateful.
        newp->m_iterpp = linkerp->m_iterpp;
        *(newp->m_iterpp) = newp;
    }
    // Empty the linker so not used twice accidentally
    linkerp->m_backp = تش_اشاره;
    debug_ونه_Change(هذا, "-relinkOut: ", __LINE__, ریښتیا);
}

فراغ Ast_غوټه::relinkOneLink(Ast_غوټه*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_غوټه* newp) {
    إذا (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_غوټه* ثابت newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_غوټه* ثابت oldlistlastp = pointpr->m_headtailp;
        UASSERT_OBJ(!(oldlistlastp->m_nextp && oldlistlastp != pointpr), newp,
                    "Old headtailp tail isn't at the tail");
        // Next links
        newlistlastp->m_nextp = pointpr;
        pointpr->m_backp = newlistlastp;
        // Head/tail
        pointpr->m_headtailp = تش_اشاره;  // Old head
        newlistlastp->m_headtailp = تش_اشاره;  // Old tail
        newp->m_headtailp = oldlistlastp;  // Head points to tail
        oldlistlastp->m_headtailp = newp;  // Tail points to head
    }
    pointpr = newp;
}

فراغ Ast_غوټه::جمع_کړه_دلته_This_As_بل(Ast_غوټه* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(هذا->m_backp, هذا, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_غوټه* ثابت backp = هذا->m_backp;
    Ast_غوټه* ثابت newLastp = newp->m_headtailp;
    //
    هذا->edit_شمېر_Inc();
    // Common linkage
    newLastp->m_nextp = هذا;
    هذا->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = تش_اشاره;
    // Linkage dependent on position
    إذا (backp && backp->m_nextp == هذا) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = تش_اشاره;
    } وإلا {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        إذا (backp) {
            إذا (backp->m_op1p == هذا) {
                backp->m_op1p = newp;
            } وإلا إذا (backp->m_op2p == هذا) {
                backp->m_op2p = newp;
            } وإلا إذا (backp->m_op3p == هذا) {
                backp->m_op3p = newp;
            } وإلا {
                UASSERT_OBJ(backp->m_op4p == هذا, هذا, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_غوټه* ثابت tailp = هذا->m_headtailp;
        هذا->m_headtailp = تش_اشاره;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    إذا (newLastp->m_iterpp) *(newLastp->m_iterpp) = هذا;
    إذا (هذا->m_iterpp) {
        *(هذا->m_iterpp) = newp;
        هذا->m_iterpp = تش_اشاره;
    }
    //
    debug_ونه_Change(هذا, "-addHereThisAsNext: ", __LINE__, ریښتیا);
}

//======================================================================
// Clone

Ast_غوټه* Ast_غوټه::clone_ونه_Iter(منطقي need_خالص) {
    // private: Clone single node and children
    إذا (need_خالص) purityCheck();
    Ast_غوټه* ثابت newp = هذا->clone();
    إذا (هذا->m_op1p) newp->op1p(هذا->m_op1p->clone_ونه_Iter_List(need_خالص));
    إذا (هذا->m_op2p) newp->op2p(هذا->m_op2p->clone_ونه_Iter_List(need_خالص));
    إذا (هذا->m_op3p) newp->op3p(هذا->m_op3p->clone_ونه_Iter_List(need_خالص));
    إذا (هذا->m_op4p) newp->op4p(هذا->m_op4p->clone_ونه_Iter_List(need_خالص));
    newp->m_iterpp = تش_اشاره;
    newp->clonep(هذا);  // Save pointers to/from both to simplify relinking.
    هذا->clonep(newp);  // Save pointers to/from both to simplify relinking.
    أرجع newp;
}

Ast_غوټه* Ast_غوټه::clone_ونه_Iter_List(منطقي need_خالص) {
    // private: Clone list of nodes, set m_headtailp
    Ast_غوټه* newheadp = تش_اشاره;
    Ast_غوټه* newtailp = تش_اشاره;
    // Audited to make sure this is never nullptr
    لكل (Ast_غوټه* oldp = هذا; oldp; oldp = oldp->m_nextp) {
        Ast_غوټه* ثابت newp = oldp->clone_ونه_Iter(need_خالص);
        newp->m_headtailp = تش_اشاره;
        newp->m_backp = newtailp;
        إذا (newtailp) newtailp->m_nextp = newp;
        إذا (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    أرجع newheadp;
}

Ast_غوټه* Ast_غوټه::clone_ونه(منطقي clone_بل_Link, منطقي need_خالص) {
    debug_ونه_Change(هذا, "-cloneThs: ", __LINE__, clone_بل_Link);
    clone_Clear_ونه();
    Ast_غوټه* newp;
    إذا (clone_بل_Link && هذا->m_nextp) {
        newp = clone_ونه_Iter_List(need_خالص);
    } وإلا {
        newp = clone_ونه_Iter(need_خالص);
        newp->m_nextp = تش_اشاره;
        newp->m_headtailp = newp;
    }
    newp->m_backp = تش_اشاره;
    newp->clone_Relink_ونه();
    debug_ونه_Change(newp, "-cloneOut: ", __LINE__, ریښتیا);
    أرجع newp;
}

فراغ Ast_غوټه::purityCheck() {
    إذا (VL_UNLIKELY(!is_خالص())) {
        هذا->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << هذا->warn_نور()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

فراغ Ast_غوټه::delete_غوټه() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_شمېر_Inc();
    // Change links of old node so we coredump if used
    هذا->m_nextp = reinterpret_cast<Ast_غوټه*>(0x1);
    هذا->m_backp = reinterpret_cast<Ast_غوټه*>(0x1);
    هذا->m_headtailp = reinterpret_cast<Ast_غوټه*>(0x1);
    هذا->m_op1p = reinterpret_cast<Ast_غوټه*>(0x1);
    هذا->m_op2p = reinterpret_cast<Ast_غوټه*>(0x1);
    هذا->m_op3p = reinterpret_cast<Ast_غوټه*>(0x1);
    هذا->m_op4p = reinterpret_cast<Ast_غوټه*>(0x1);
    هذا->m_iterpp = reinterpret_cast<Ast_غوټه**>(0x1);
    إذا (
#إذا !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        ریښتیا
#وإلا
        !v3Global.opt.debugLeak()
#د_که_پای
    ) {
        ړنګول هذا;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

فراغ Ast_غوټه::delete_ونه_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    لكل (Ast_غوټه *nodep = هذا, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        إذا (nodep->m_op1p) nodep->m_op1p->delete_ونه_Iter();
        إذا (nodep->m_op2p) nodep->m_op2p->delete_ونه_Iter();
        إذا (nodep->m_op3p) nodep->m_op3p->delete_ونه_Iter();
        إذا (nodep->m_op4p) nodep->m_op4p->delete_ونه_Iter();
        nodep->m_nextp = تش_اشاره;
        nodep->m_backp = تش_اشاره;
        nodep->delete_غوټه();
    }
}

فراغ Ast_غوټه::delete_ونه() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_ونه_Change(هذا, "-delTree:  ", __LINE__, ریښتیا);
    هذا->edit_شمېر_Inc();
    // MUST be depth first!
    delete_ونه_Iter();
}

//======================================================================
// Memory checks

#که_تعریف_شوی VL_LEAK_CHECKS
فراغ* Ast_غوټه::چلوونکی جديد(اندازه_t اندازه) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_غوټه* ثابت objp = static_cast<Ast_غوټه*>(::چلوونکی جديد(اندازه));
    V3Broken::جمع_کړه_Newed(objp);
    أرجع objp;
}

فراغ Ast_غوټه::چلوونکی ړنګول(فراغ* objp, اندازه_t اندازه) {
    إذا (!objp) أرجع;
    ثابت Ast_غوټه* ثابت nodep = static_cast<Ast_غوټه*>(objp);
    V3Broken::deleted(nodep);
    ::چلوونکی ړنګول(objp);
}
#د_که_پای

#که_تعریف_شوی VL_ALLOC_RANDOM_CHECKS
فراغ* Ast_غوټه::چلوونکی جديد(اندازه_t اندازه) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ساكن ثابت اندازه_t ASTGEN_MAX_غوټه_اندازه = []() {
        اندازه_t max_اندازه = 0;
        لكل (اندازه_t t = 0; t < VNType::عدد_TYPES(); ++t) {
            max_اندازه = std::max(max_اندازه, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        أرجع max_اندازه;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr اندازه_t POOL_اندازه = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ساكن uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    إذا (صحيح تخم = v3Global.opt.debugAllocRandom()) {
        ساكن V3Mutex s_mutex;
        ثابت V3_قلف_Guard قلف_2{s_mutex};
        ساكن std::array<فراغ*, POOL_اندازه> s_غوټه_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(اندازه <= ASTGEN_MAX_غوټه_اندازه, "fix ASTGEN_MAX_NODE_SIZE");
        إذا (!s_lfsr) {
            s_lfsr = تخم;
            إذا (!s_lfsr) s_lfsr = ~s_lfsr;
            لكل (اندازه_t i = 0; i < POOL_اندازه; ++i) {
                s_غوټه_Pool[i] = ::چلوونکی جديد(ASTGEN_MAX_غوټه_اندازه + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::ترتیبول(std::begin(s_غوټه_Pool), std::پای_ټکی(s_غوټه_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ثابت اندازه_t شاخص = s_lfsr % POOL_اندازه;
        Ast_غوټه* ثابت objp = static_cast<Ast_غوټه*>(s_غوټه_Pool[شاخص]);
        s_غوټه_Pool[شاخص] = ::چلوونکی جديد(ASTGEN_MAX_غوټه_اندازه + 64);  // For later new()
        أرجع objp;
    } وإلا {
        Ast_غوټه* ثابت objp = static_cast<Ast_غوټه*>(::چلوونکی جديد(اندازه));
        أرجع objp;
    }
}
فراغ Ast_غوټه::چلوونکی ړنګول(فراغ* objp, اندازه_t اندازه) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (فراغ)objp;
    (فراغ)اندازه;
}
#د_که_پای

//======================================================================
// Iterators

فراغ Ast_غوټه::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    إذا (m_op1p) m_op1p->iterate_And_بل(v);
    إذا (m_op2p) m_op2p->iterate_And_بل(v);
    إذا (m_op3p) m_op3p->iterate_And_بل(v);
    إذا (m_op4p) m_op4p->iterate_And_بل(v);
}

فراغ Ast_غوټه::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    إذا (m_op1p) m_op1p->iterate_And_بل_Const(v);
    إذا (m_op2p) m_op2p->iterate_And_بل_Const(v);
    إذا (m_op3p) m_op3p->iterate_And_بل_Const(v);
    إذا (m_op4p) m_op4p->iterate_And_بل_Const(v);
}

فراغ Ast_غوټه::iterate_And_بل(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_غوټه* nodep = هذا;
#که_تعریف_شوی VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#د_که_پای
    // cppcheck-suppress knownConditionTrueFalse
    إذا (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    طالما (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        إذا (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_غوټه* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        إذا (!niterp) أرجع;  // Perhaps node deleted inside accept
        niterp->m_iterpp = تش_اشاره;
        إذا (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } وإلا {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

فراغ Ast_غوټه::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_غوټه* nodep = هذا;
    طالما (nodep->m_nextp) nodep = nodep->m_nextp;
    طالما (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        إذا (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } وإلا {
            nodep = تش_اشاره;
        }  // else: backp points up the tree.
    }
}

فراغ Ast_غوټه::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    إذا (m_op1p) m_op1p->iterateListBackwardsConst(v);
    إذا (m_op2p) m_op2p->iterateListBackwardsConst(v);
    إذا (m_op3p) m_op3p->iterateListBackwardsConst(v);
    إذا (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

فراغ Ast_غوټه::iterate_And_بل_Const(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_غوټه* nodep = هذا;
    افعل {
        Ast_غوټه* ثابت nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } طالما (nodep);
}

Ast_غوټه* Ast_غوټه::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_غوټه* nodep = هذا;  // Note "this" may point to bogus point later in this function
    إذا (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } وإلا إذا (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ثابت tempp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", nodep, خطأ};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlink_Fr_شا_With_بل();
        VL_DO_DANGLING(tempp->delete_ونه(), tempp);
    } وإلا {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_غوټه** nextnodepp = تش_اشاره;
        إذا (هذا->m_backp->m_op1p == هذا) {
            nextnodepp = &(هذا->m_backp->m_op1p);
        } وإلا إذا (هذا->m_backp->m_op2p == هذا) {
            nextnodepp = &(هذا->m_backp->m_op2p);
        } وإلا إذا (هذا->m_backp->m_op3p == هذا) {
            nextnodepp = &(هذا->m_backp->m_op3p);
        } وإلا إذا (هذا->m_backp->m_op4p == هذا) {
            nextnodepp = &(هذا->m_backp->m_op4p);
        } وإلا إذا (هذا->m_backp->m_nextp == هذا) {
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

فراغ Ast_غوټه::clone_Relink_ونه() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    لكل (Ast_غوټه* nodep = هذا; nodep; nodep = nodep->m_nextp) {
        إذا (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        إذا (nodep->m_op1p) nodep->m_op1p->clone_Relink_ونه();
        إذا (nodep->m_op2p) nodep->m_op2p->clone_Relink_ونه();
        إذا (nodep->m_op3p) nodep->m_op3p->clone_Relink_ونه();
        إذا (nodep->m_op4p) nodep->m_op4p->clone_Relink_ونه();
    }
}

//======================================================================
// Comparison

منطقي Ast_غوټه::gate_ونه_Iter() ثابت {
    // private: Return true if the two trees are identical
    إذا (!isGateOptimizable()) أرجع خطأ;
    إذا (m_op1p && !m_op1p->gate_ونه_Iter()) أرجع خطأ;
    إذا (m_op2p && !m_op2p->gate_ونه_Iter()) أرجع خطأ;
    إذا (m_op3p && !m_op3p->gate_ونه_Iter()) أرجع خطأ;
    إذا (m_op4p && !m_op4p->gate_ونه_Iter()) أرجع خطأ;
    أرجع ریښتیا;
}

منطقي Ast_غوټه::same_ونه_Iter(ثابت Ast_غوټه* node1p, ثابت Ast_غوټه* node2p, منطقي ign_بل,
                           منطقي gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    طالما (ریښتیا) {
        إذا (!node1p && !node2p) أرجع ریښتیا;
        إذا (!node1p || !node2p) أرجع خطأ;
        إذا (node1p->type() != node2p->type()) أرجع خطأ;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        إذا (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) أرجع خطأ;
        إذا (!node1p->same_غوټه(node2p) || (gateOnly && !node1p->isGateOptimizable())) أرجع خطأ;
        إذا (!same_ونه_Iter(node1p->m_op1p, node2p->m_op1p, خطأ, gateOnly)
            || !same_ونه_Iter(node1p->m_op2p, node2p->m_op2p, خطأ, gateOnly)
            || !same_ونه_Iter(node1p->m_op3p, node2p->m_op3p, خطأ, gateOnly)
            || !same_ونه_Iter(node1p->m_op4p, node2p->m_op4p, خطأ, gateOnly)) {
            أرجع خطأ;
        }
        إذا (ign_بل) أرجع ریښتیا;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

فراغ Ast_غوټه::check_ونه_Iter(ثابت Ast_غوټه* prevBackp) ثابت VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == هذا->backp(), هذا, "Back node inconsistent");
    ثابت VNTypeInfo& typeInfo = VNType::typeInfo(هذا->type());
    لكل (صحيح i = 1; i <= 4; i++) {
        Ast_غوټه* nodep = تش_اشاره;
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
        حالة VNTypeInfo::OP_ناکارول_شوی:
            UASSERT_OBJ(!nodep, هذا, typeInfo.m_namep << " must not use " << opName << "()");
            اكسر;
        حالة VNTypeInfo::OP_کارول_شوی:
            UASSERT_OBJ(nodep, هذا,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), هذا,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_ونه_Iter(هذا);
            اكسر;
        حالة VNTypeInfo::OP_LIST:
            إذا (ثابت Ast_غوټه* ثابت headp = nodep) {
                // cppcheck-suppress shadowFunction
                ثابت Ast_غوټه* backp = هذا;
                ثابت Ast_غوټه* tailp;
                ثابت Ast_غوټه* opp = headp;
                افعل {
                    opp->check_ونه_Iter(backp);
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
            إذا (nodep) {
                UASSERT_OBJ(!nodep->nextp(), هذا,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_ونه_Iter(هذا);
            }
            اكسر;
        افتراضي: هذا->v3fatalSrc("Bad case"); اكسر;
        }
    }
    إذا (v3Global.opt.debug_پلنوالی() && v3Global.پلنوالی_Min_Usage() == VWidthMinUsage::VERILOG_پلنوالی) {
        إذا (ثابت Ast_غوټه_بیان* ثابت exprp = VN_CAST(هذا, غوټه_بیان)) {
            ثابت حرف* ثابت whyp = exprp->پلنوالی_Mismatch();
            إذا (whyp) {
                خپلکاری dtypeStr = [](ثابت Ast_غوټه_بیان* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    أرجع ss.str();
                };
                إذا (ثابت Ast_غوټه_Uniop* ثابت uniopp = VN_CAST(exprp, غوټه_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } وإلا إذا (ثابت Ast_غوټه_Biop* ثابت biopp = VN_CAST(exprp, غوټه_Biop)) {
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
حرف* Ast_غوټه::dump_ونه_Json_Gdb(ثابت Ast_غوټه* nodep) {
    إذا (!nodep) أرجع strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodep_بهیر;
    nodep->dump_ونه_Json(nodep_بهیر);
    ثابت std::string str = nodep_بهیر.rdbuf()->str();
    أرجع strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
حرف* Ast_غوټه::dump_ونه_Json_Gdb(ثابت حرف* str) { أرجع strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
حرف* Ast_غوټه::dump_ونه_Json_Gdb(intptr_t nodep) {
    إذا (!nodep) أرجع strdup("{\"addr\":\"NULL\"}\n");
    أرجع dump_ونه_Json_Gdb(reinterpret_cast<ثابت Ast_غوټه*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_غوټه::dumpGdb(ثابت Ast_غوټه* nodep) {  // For GDB only  // LCOV_EXCL_START
    إذا (!nodep) {
        وتنه << "<nullptr>" << کرښه_پای;
        أرجع;
    }
    nodep->dump_Gdb_سرلیک();
    وتنه << "  ";
    nodep->dump(وتنه);
    وتنه << کرښه_پای;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_غوټه::dump_Gdb_سرلیک() ثابت {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(وتنه);
    وتنه << "  Fileline = " << fileline() << کرښه_پای;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_غوټه::dump_ونه_Gdb(ثابت Ast_غوټه* nodep) {  // For GDB only  // LCOV_EXCL_START
    إذا (!nodep) {
        وتنه << "<nullptr>" << کرښه_پای;
        أرجع;
    }
    nodep->dump_Gdb_سرلیک();
    nodep->dump_ونه(وتنه);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_غوټه::dump_ونه_File_Gdb(ثابت Ast_غوټه* nodep,  // LCOV_EXCL_START
                              ثابت حرف* filenamep) {  // For GDB only
    إذا (!nodep) {
        وتنه << "<nullptr>" << کرښه_پای;
        أرجع;
    }
    ثابت string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_ونه_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_غوټه::checkIter() ثابت {
    إذا (m_iterpp) {
        dumpPtrs(وتنه);
        // Perhaps something forgot to clear m_iterpp?
        هذا->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

فراغ Ast_غوټه::dumpPtrs(std::ostream& os) ثابت {
    os << "This=" << typeName() << " " << cvt_منزل_Hex(هذا);
    os << " back=" << cvt_منزل_Hex(backp());
    إذا (nextp()) os << " next=" << cvt_منزل_Hex(nextp());
    إذا (m_headtailp == هذا) {
        os << " headtail=this";
    } وإلا {
        os << " headtail=" << cvt_منزل_Hex(m_headtailp);
    }
    إذا (op1p()) os << " op1p=" << cvt_منزل_Hex(op1p());
    إذا (op2p()) os << " op2p=" << cvt_منزل_Hex(op2p());
    إذا (op3p()) os << " op3p=" << cvt_منزل_Hex(op3p());
    إذا (op4p()) os << " op4p=" << cvt_منزل_Hex(op4p());
    ثابت خپلکاری dumpUser = [&os](ثابت حرف* مختاړی, ثابت VNUser& user) {
        ثابت std::string s
            = user.dumpStr([](ثابت فراغ* p) -> std::string { أرجع cvt_منزل_Hex(p); });
        إذا (!s.تشه()) os << مختاړی << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    إذا (m_iterpp) {
        os << " iterpp=" << cvt_منزل_Hex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

فراغ Ast_غوټه::dump_ونه(std::ostream& os, ثابت string& indent, صحيح max_ژوروالی) ثابت {
    ساكن صحيح s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << هذا << '\n';
    إذا (VN_DELETED(هذا)) أرجع;
    إذا (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    إذا (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvt_منزل_Hex(هذا)
           << کرښه_پای;  // endl intentional to do flush
        أرجع;
    }  // LCOV_EXCL_STOP
    إذا (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    إذا (max_ژوروالی == 1) {
        إذا (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } وإلا {
        لكل (ثابت Ast_غوټه* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ونه(os, indent + "1:", max_ژوروالی - 1);
        }
        لكل (ثابت Ast_غوټه* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ونه(os, indent + "2:", max_ژوروالی - 1);
        }
        لكل (ثابت Ast_غوټه* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ونه(os, indent + "3:", max_ژوروالی - 1);
        }
        لكل (ثابت Ast_غوټه* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ونه(os, indent + "4:", max_ژوروالی - 1);
        }
    }
}

فراغ Ast_غوټه::dump_ونه_And_بل(std::ostream& os, ثابت string& indent, صحيح max_ژوروالی) ثابت {
    // Audited to make sure this is never nullptr
    لكل (ثابت Ast_غوټه* nodep = هذا; nodep; nodep = nodep->nextp()) {
        nodep->dump_ونه(os, indent, max_ژوروالی);
    }
}

فراغ Ast_غوټه::dump_ونه_File(ثابت string& filename, منطقي doDump) {
    // Not const function as calls checkTree
    إذا (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            إذا (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_شمېر_وروستی();
            *logsp << "> to <e" << std::dec << edit_شمېر_Gbl() << ">\n";
            إذا (edit_شمېر_Gbl() == edit_شمېر_وروستی() && ::dump_ونه_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } وإلا {
                dump_ونه(*logsp);
                edit_شمېر_سېټ_وروستی();  // Next dump can indicate start from here
            }
        }
    }
}

ساكن فراغ drawChildren(std::ostream& os, ثابت Ast_غوټه* thisp, ثابت Ast_غوټه* childp,
                         ثابت std::string& ماشوم_Name) {
    إذا (childp) {
        os << "\tn" << cvt_منزل_Hex(thisp) << " -> n" << cvt_منزل_Hex(childp) << " [" << "label=\""
           << ماشوم_Name << "\" color=red];\n";
        لكل (ثابت Ast_غوټه* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_ونه_Dot(os);
            إذا (nodep->nextp()) {
                os << "\tn" << cvt_منزل_Hex(nodep) << " -> n" << cvt_منزل_Hex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvt_منزل_Hex(nodep) << ", n" << cvt_منزل_Hex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

فراغ Ast_غوټه::dump_ونه_Dot(std::ostream& os) ثابت {
    os << "\tn" << cvt_منزل_Hex(هذا) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, هذا, m_op1p, "op1");
    drawChildren(os, هذا, m_op2p, "op2");
    drawChildren(os, هذا, m_op3p, "op3");
    drawChildren(os, هذا, m_op4p, "op4");
}

فراغ Ast_غوټه::dump_ونه_Json_File(ثابت string& filename, منطقي doDump) {
    إذا (!doDump) أرجع;
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    إذا (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_ونه_Json(*treejsonp);
    *treejsonp << '\n';
}

فراغ Ast_غوټه::dumpJsonMetaFileGdb(ثابت حرف* filename) { dumpJsonMetaFile(filename); }
فراغ Ast_غوټه::dumpJsonMetaFile(ثابت string& filename) {
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    إذا (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_کرښه::file_Name_عدد_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptr_نومونه_Dump_Json(*treejsonp);
    *treejsonp << "}\n";
}

فراغ Ast_غوټه::dump_ونه_Dot_File(ثابت string& filename, منطقي doDump) {
    إذا (doDump) {
        UINFO(2, "Dumping " << filename);
        ثابت std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        إذا (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_ونه_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_غوټه::instanceStr() ثابت {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr بینښه maxIterations = 10000;
    بینښه iter_شمېر = 0;
    // Walk 'backp' chain
    لكل (ثابت Ast_غوټه* currp = هذا; currp; currp = currp->backp(), ++iter_شمېر) {
        إذا (VL_UNCOVERABLE(iter_شمېر >= maxIterations)) أرجع "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        إذا (ثابت AstScope* ثابت scopep = VN_CAST(currp, Scope)) {
            أرجع scopep->is_سر() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        إذا (ثابت AstModule* ثابت modp = VN_CAST(currp, Module)) {
            ثابت string instanceName = modp->someInstanceName();
            أرجع instanceName.تشه() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    أرجع "";
}
فراغ Ast_غوټه::v3error_پای_ټکی(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    إذا (!m_fileline) {
        V3Error::v3error_پای_ټکی(str, "", تش_اشاره);
    } وإلا {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ثابت string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().error_کوډ()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        إذا (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_غوټه*>(هذا)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3error_پای_ټکی(nsstr, instanceStrExtra);
    }
}
فراغ Ast_غوټه::v3error_پای_ټکی_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_پای_ټکی(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

فراغ Ast_غوټه::dtypeChgSigned(منطقي flag) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing to (un)signed");
    dtype_Chg_پلنوالی_Signed(dtypep()->پلنوالی(), dtypep()->پلنوالی_Min(), VSigning::fromBool(flag));
}
فراغ Ast_غوټه::dtype_Chg_پلنوالی(صحيح پلنوالی, صحيح پلنوالی_Min) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing width");
    dtype_Chg_پلنوالی_Signed(پلنوالی, پلنوالی_Min, dtypep()->numeric());
}

فراغ Ast_غوټه::dtype_Chg_پلنوالی_Signed(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing width");
    ثابت Ast_غوټه_DType* ثابت enumDTypep = dtypep()->skip_Ref_منزل_Enump();
    إذا (پلنوالی == dtypep()->پلنوالی() && پلنوالی_Min == dtypep()->پلنوالی_Min()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        أرجع;  // Correct already
    }
    ثابت Ast_غوټه_DType* ثابت dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtype_سېټ_بټ_Or_Logic_Unsized(پلنوالی, پلنوالی_Min, numeric, dtypep->isFourstate());
}

Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_Basic_DType(VBasicDTypeKwd kwd) ثابت {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_Basic_DType(fileline(), kwd);
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_بټ_DType(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_Logic_بټ_DType(fileline(), VBasicDTypeKwd::بټ,
                                                             پلنوالی, پلنوالی_Min, numeric);
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_Logic_DType(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_Logic_بټ_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             پلنوالی, پلنوالی_Min, numeric);
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_Logic2_State_DType(صحيح پلنوالی, صحيح پلنوالی_Min, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_Logic_بټ_DType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, پلنوالی, پلنوالی_Min, numeric);
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_Logic_Range_DType(ثابت VNumRange& range, صحيح پلنوالی_Min,
                                           VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_Logic_بټ_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, پلنوالی_Min, numeric);
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_بټ_Range_DType(ثابت VNumRange& range, صحيح پلنوالی_Min,
                                         VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_Logic_بټ_DType(fileline(), VBasicDTypeKwd::بټ,
                                                             range, پلنوالی_Min, numeric);
}
AstBasicDType* Ast_غوټه::پیدا_کړه_ورننباسه_Same_DType(AstBasicDType* nodep) {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_ورننباسه_Same_DType(nodep);
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_Constraint_Ref_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_Constraint_Ref_DType(fileline());
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_تشه_کتار_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_تشه_کتار_DType(fileline());
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_کتار_شاخص_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_کتار_شاخص_DType(fileline());
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_بهیر_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_بهیر_DType(fileline());
}
Ast_غوټه_DType* Ast_غوټه::پیدا_کړه_Void_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->پیدا_کړه_Void_DType(fileline());
}

ساكن ثابت Ast_غوټه_DType* compute_Castable_بنسټ(ثابت Ast_غوټه_DType* nodep) {
    طالما (ریښتیا) {
        إذا (ثابت AstPackArrayDType* ثابت packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            استمر;
        }
        إذا (ثابت Ast_غوټه_DType* ثابت refp = nodep->skip_Ref_منزل_Enump()) {
            إذا (refp != nodep) {
                nodep = refp;
                استمر;
            }
        }
        أرجع nodep;
    }
}

ساكن VCastable computeCastableImp(ثابت Ast_غوټه_DType* منزل_Dtp, ثابت Ast_غوټه_DType* fromDtp,
                                    ثابت Ast_غوټه* fromConstp, ثابت منطقي checkIfaceArgCompat) {
    ثابت VCastable castable = VCastable::UNSUPPORTED;
    منزل_Dtp = منزل_Dtp->skip_Ref_منزل_Enump();
    fromDtp = fromDtp->skip_Ref_منزل_Enump();
    إذا (منزل_Dtp == fromDtp) أرجع VCastable::SAMEISH;
    إذا (منزل_Dtp->similarDType(fromDtp)) أرجع VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ثابت Ast_غوټه_DType* from_بنسټ_Dtp = compute_Castable_بنسټ(fromDtp);

    ثابت منطقي fromNumericable = VN_IS(from_بنسټ_Dtp, BasicDType) || VN_IS(from_بنسټ_Dtp, EnumDType)
                                 || VN_IS(from_بنسټ_Dtp, بهیر_DType)
                                 || VN_IS(from_بنسټ_Dtp, غوټه_UOr_Struct_DType);

    ثابت Ast_غوټه_DType* منزل_بنسټ_Dtp = compute_Castable_بنسټ(منزل_Dtp);
    ثابت منطقي منزل_Numericable
        = VN_IS(منزل_بنسټ_Dtp, BasicDType) || VN_IS(منزل_بنسټ_Dtp, غوټه_UOr_Struct_DType);

    إذا (منزل_بنسټ_Dtp == from_بنسټ_Dtp) أرجع VCastable::COMPATIBLE;

    إذا (منزل_Numericable) {
        إذا (fromNumericable) أرجع VCastable::COMPATIBLE;
    } وإلا إذا (VN_IS(منزل_بنسټ_Dtp, EnumDType)) {
        إذا (VN_IS(from_بنسټ_Dtp, EnumDType) && منزل_Dtp->same_ونه(fromDtp))
            أرجع VCastable::ENUM_IMPLICIT;
        إذا (fromNumericable) أرجع VCastable::ENUM_EXPLICIT;
    } وإلا إذا (VN_IS(منزل_Dtp, کتار_DType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, بهیر_DType))) {
        أرجع VCastable::COMPATIBLE;
    } وإلا إذا (VN_IS(منزل_Dtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        إذا (fromConstp->isNull()) أرجع VCastable::COMPATIBLE;
    } وإلا إذا (VN_IS(منزل_Dtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ثابت AstClass* ثابت منزل_Classp = VN_AS(منزل_Dtp, ClassRefDType)->classp();
        ثابت AstClass* ثابت fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ثابت منطقي downcast = AstClass::isClassExtendedFrom(منزل_Classp, fromClassp);
        ثابت منطقي upcast = AstClass::isClassExtendedFrom(fromClassp, منزل_Classp);
        إذا (upcast) أرجع VCastable::COMPATIBLE;
        إذا (downcast) أرجع VCastable::DYNAMIC_CLASS;
        أرجع VCastable::INCOMPATIBLE;
    } وإلا إذا (ثابت AstIfaceRefDType* ثابت منزل_Ifp
               = VN_CAST(checkIfaceArgCompat ? منزل_Dtp->elemDTypep(ریښتیا) : منزل_Dtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ثابت AstIfaceRefDType* ثابت fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(ریښتیا) : fromDtp, IfaceRefDType);
        إذا (checkIfaceArgCompat && fromIfp && (منزل_Dtp != منزل_Ifp || fromDtp != fromIfp)) {
            ثابت AstUnpackArrayDType* ثابت منزل_Arrayp = VN_CAST(منزل_Dtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            إذا (!منزل_Arrayp || !fromArrayp
                || منزل_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
                أرجع VCastable::INCOMPATIBLE;
            }
            أرجع computeCastableImp(منزل_Arrayp->subDTypep(), fromArrayp->subDTypep(), تش_اشاره,
                                      checkIfaceArgCompat);
        }
        إذا (!fromIfp || منزل_Ifp->iface_له_لارې_Cellp() != fromIfp->iface_له_لارې_Cellp()) {
            إذا (!checkIfaceArgCompat) أرجع castable;
            أرجع VCastable::INCOMPATIBLE;
        }
        ثابت منطقي sameModport = منزل_Ifp->modportp() == fromIfp->modportp();
        إذا (!checkIfaceArgCompat) {
            إذا (!منزل_Ifp->modportp() || sameModport) أرجع VCastable::COMPATIBLE;
            أرجع castable;
        }
        إذا (منزل_Ifp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            أرجع VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        إذا (منزل_Ifp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            أرجع VCastable::COMPATIBLE;
        }
        أرجع VCastable::INCOMPATIBLE;
    } وإلا إذا (ثابت AstUnpackArrayDType* ثابت منزل_Arrayp = VN_CAST(منزل_Dtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        إذا (!fromArrayp || منزل_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
            أرجع castable;
        }
        ثابت VCastable subCastable = computeCastableImp(
            منزل_Arrayp->subDTypep(), fromArrayp->subDTypep(), تش_اشاره, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ثابت Ast_غوټه_DType* ثابت منزل_Subp = منزل_Arrayp->subDTypep()->skipRefp();
        إذا (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(منزل_Subp, UnpackArrayDType) || VN_IS(منزل_Subp, IfaceRefDType)))) {
            أرجع VCastable::COMPATIBLE;
        }
    }
    أرجع castable;
}

VCastable Ast_غوټه::computeCastable(ثابت Ast_غوټه_DType* منزل_Dtp, ثابت Ast_غوټه_DType* fromDtp,
                                   ثابت Ast_غوټه* fromConstp, ثابت منطقي checkIfaceArgCompat) {
    ثابت خپلکاری castable = computeCastableImp(منزل_Dtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << منزل_Dtp);
    UINFO(9, "     =?= " << fromDtp);
    إذا (fromConstp) UINFO(9, "     const= " << fromConstp);
    أرجع castable;
}

Ast_غوټه_DType* Ast_غوټه::getCommonClassTypep(Ast_غوټه* node1p, Ast_غوټه* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    إذا (VN_IS(node1p, Const)) std::بدلون(node1p, node2p);
    {
        ثابت VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        إذا (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            أرجع node1p->dtypep();
        إذا (castable == VCastable::DYNAMIC_CLASS) أرجع node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    طالما (classDtypep1) {
        ثابت VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        إذا (castable == VCastable::COMPATIBLE) أرجع classDtypep1;
        AstClassExtends* ثابت extendsp = classDtypep1->classp()->extendsp();
        إذا (!extendsp) اكسر;
        Ast_غوټه_DType* ثابت edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->ماشوم_DTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    أرجع تش_اشاره;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

صنف VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<صحيح, std::string> m_interned_کلمه_Widths;  // Interned widths
    std::map<صحيح, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ثابت منطقي m_dump_پلنوالی_Min = v3Global.پلنوالی_Min_Usage() != VWidthMinUsage::MATCHES_پلنوالی;

    ساكن std::string منزل_Letters(اندازه_t ارزښت, منطقي ښکتنی_Case = خطأ) {
        ثابت حرف بنسټ = ښکتنی_Case ? 'a' : 'A';
        std::string s;
        افعل { s += static_cast<حرف>(بنسټ + ارزښت % 26); } طالما (ارزښت /= 26);
        أرجع s;
    }

    ثابت std::string& internConst(ثابت AstConst& nodep) {
        ثابت خپلکاری pair = m_internedConsts.emplace(nodep.عدد().ascii(خطأ), "");
        إذا (pair.دوهم) pair.لومړی->دوهم += منزل_Letters(m_internedConsts.اندازه() - 1);
        أرجع pair.لومړی->دوهم;
    }

    ثابت std::string& intern_کلمه_پلنوالی(صحيح ارزښت) {
        ثابت خپلکاری pair = m_interned_کلمه_Widths.emplace(ارزښت, "");
        إذا (pair.دوهم) pair.لومړی->دوهم += منزل_Letters(m_interned_کلمه_Widths.اندازه() - 1, ریښتیا);
        أرجع pair.لومړی->دوهم;
    }

    ثابت std::string& intern_Wide_پلنوالی(صحيح ارزښت) {
        ثابت خپلکاری pair = m_internedWideWidths.emplace(ارزښت, "");
        إذا (pair.دوهم) pair.لومړی->دوهم += منزل_Letters(m_internedWideWidths.اندازه() - 1);
        أرجع pair.لومړی->دوهم;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ساكن منطقي is_ونې_پاڼه(ثابت Ast_غوټه* nodep) {
        ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        لكل (ثابت VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            إذا (opType != VNTypeInfo::OP_ناکارول_شوی) أرجع خطأ;
        }
        أرجع ریښتیا;
    }

    // Render operand, return true if not unused
    فراغ render_عملوند(VNTypeInfo::OpEn opType, ثابت Ast_غوټه* ثابت opp, uint32_t ژوروالی) {
        بدل (opType) {
        حالة VNTypeInfo::OP_ناکارول_شوی:  //
            اكسر;
        حالة VNTypeInfo::OP_کارول_شوی:  //
            m_os << ' ';
            render(opp, ژوروالی);
            اكسر;
        حالة VNTypeInfo::OP_LIST:
            m_os << " [";
            لكل (ثابت Ast_غوټه* nodep = opp; nodep; nodep = nodep->nextp()) {
                إذا (nodep != opp) m_os << ", ";
                render(opp, ژوروالی);
            }
            m_os << ']';
            اكسر;
        حالة VNTypeInfo::OP_OPTIONAL:
            إذا (opp) {
                m_os << " ";
                render(opp, ژوروالی);
            } وإلا {
                m_os << " nil";
            }
            اكسر;
        }
    }

    // Render the node into the stream.
    فراغ render(ثابت Ast_غوټه* nodep, uint32_t ژوروالی) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            إذا (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } وإلا إذا (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } وإلا {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } وإلا إذا (is_ونې_پاڼه(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } وإلا إذا (ژوروالی == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } وإلا {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            render_عملوند(typeInfo.m_opType[0], nodep->op1p(), ژوروالی - 1);
            render_عملوند(typeInfo.m_opType[1], nodep->op2p(), ژوروالی - 1);
            render_عملوند(typeInfo.m_opType[2], nodep->op3p(), ژوروالی - 1);
            render_عملوند(typeInfo.m_opType[3], nodep->op4p(), ژوروالی - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ثابت Ast_غوټه_DType* ثابت dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : تش_اشاره;
        إذا (!dtypep) {
            m_os << '?';
        } وإلا إذا (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } وإلا {
            ثابت صحيح پلنوالی = nodep->پلنوالی();
            إذا (پلنوالی == 1) {
                m_os << '1';
            } وإلا إذا (پلنوالی <= VL_QUADSIZE) {
                m_os << intern_کلمه_پلنوالی(پلنوالی);
            } وإلا {
                m_os << intern_Wide_پلنوالی(پلنوالی);
            }
            إذا (m_dump_پلنوالی_Min) {
                m_os << '/';
                ثابت صحيح پلنوالی_Min = nodep->پلنوالی_Min();
                إذا (پلنوالی_Min == 1) {
                    m_os << '1';
                } وإلا إذا (پلنوالی_Min <= VL_QUADSIZE) {
                    m_os << intern_کلمه_پلنوالی(پلنوالی_Min);
                } وإلا {
                    m_os << intern_Wide_پلنوالی(پلنوالی_Min);
                }
            }
        }
    }

عام:
    VNPatternString(std::ostream& os, ثابت Ast_غوټه* nodep, uint32_t ژوروالی)
        : m_os{os} {
        render(nodep, ژوروالی);
    }
};

std::string Ast_غوټه_بیان::نمونه_String(uint32_t ژوروالی) ثابت {
    std::ostringstream oss;
    VNPatternString{oss, هذا, ژوروالی};
    أرجع oss.str();
}

//######################################################################
// VNDeleter

فراغ VNDeleter::doDeletes() {
    لكل (Ast_غوټه* ثابت nodep : m_deleteps) nodep->delete_ونه();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#ضمن "V3Ast__gen_visitor_defns.h"  // From ./astgen
