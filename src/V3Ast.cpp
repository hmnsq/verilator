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

uint64_t Ast_ڳنڍ::s_edit_Cnt_آخري = 0;
uint64_t Ast_ڳنڍ::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
صحيح Ast_ڳنڍ::s_cloneCntGbl = 0;
uint32_t VNUser1_اندريون_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2_اندريون_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3_اندريون_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4_اندريون_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent

منطقي VNUser1_اندريون_Use::s_userBusy = خطأ;
منطقي VNUser2_اندريون_Use::s_userBusy = خطأ;
منطقي VNUser3_اندريون_Use::s_userBusy = خطأ;
منطقي VNUser4_اندريون_Use::s_userBusy = خطأ;

صحيح Ast_ڳنڍ_DType::s_unique_انگ = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::array_طريقو(ثابت string& name) {
    لكل (صحيح i = 0; i < _ENUM_MAX; ++i) {
        ثابت VCMethod طريقو{i};
        إذا (name == طريقو.ascii()) أرجع طريقو;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    أرجع VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ثابت فراغ*)) ثابت {
#جيڪڏهن_وصف_ٿيل VL_USER_TYPE_CHECKS
    إذا (ثابت uint64_t* ثابت uip = std::get_if<uint64_t>(&m_u)) أرجع "#"s + cvt_منزل_Str(*uip);
    إذا (فراغ* ثابت* ثابت upp = std::get_if<فراغ*>(&m_u)) أرجع fmtAddrp(*upp);
    أرجع "";
#وإلا
    // Dumps void* representation
    إذا (!m_u.مٿي) أرجع "";
    أرجع fmtAddrp(m_u.مٿي);
#جيڪڏهن_پورو
}

//######################################################################
// VNType

ثابت VNTypeInfo VNType::s_type_Info_جدول[VNType::انگ_TYPES()] = {
#ضمن "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& عامل<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

منطقي VFwdType::is_ڳنڍ_Compatible(ثابت Ast_ڳنڍ* nodep) ثابت {
    ثابت Ast_ڳنڍ* defp = nodep;
    إذا (ثابت AstTypedef* ثابت adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    إذا (ثابت Ast_ڳنڍ_DType* ثابت adefp = VN_CAST(defp, ڳنڍ_DType))
        defp = adefp->skip_Ref_منزل_Non_Refp();
    بدل (m_e) {
    حالة VFwdType::NONE: أرجع سچ; اكسر;
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

ثابت std::shared_ptr<ثابت string> VSelf_اشارو_متن::s_emptyp = std::make_shared<string>("");
ثابت std::shared_ptr<ثابت string> VSelf_اشارو_متن::s_thisp = std::make_shared<string>("this");

string VSelf_اشارو_متن::replaceThis(منطقي useSelfForThis, ثابت string& متن) {
    أرجع useSelfForThis ? VString::replace_لفظ(متن, "this", "vlSelf") : متن;
}

string VSelf_اشارو_متن::protect(منطقي useSelfForThis, منطقي protect) ثابت {
    أرجع VIdProtect::protect_لفظ_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_ڳنڍ::Ast_ڳنڍ(VNType t, File_سٽ* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = هذا;  // When made, we're a list of only a single element
    //خاصيتون
    m_flags.did_ويڪر = خطأ;
    m_flags.doing_ويڪر = خطأ;
    m_flags.protect = سچ;
    m_flags.اڻ_استعمال_ٿيل = 0;  // Initializing this avoids a read-modify-write on construction
    edit_ڳڻپ_Inc();
}

Ast_ڳنڍ* Ast_ڳنڍ::abovep() ثابت {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || پهريون_Abovep(), هذا, "abovep() not allowed when in midlist");
    ثابت Ast_ڳنڍ* ثابت firstp = پهريون_Abovep() ? هذا : m_headtailp;
    أرجع firstp->backp();
}
Ast_ڳنڍ* Ast_ڳنڍ::aboveLoopp() ثابت {
    // Returns parent node. Avoid using this, may have performance issues.
    ثابت Ast_ڳنڍ* nodep = هذا;
    // Backwards over peers (versus parents)
    طالما (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    أرجع nodep->backp();
}

string Ast_ڳنڍ::encodeName(ثابت string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string ٻاهريون;
    ٻاهريون.reserve(namein.سائيز());
    لكل (خودڪار pos = namein.begin(); pos != namein.پڇاڙي(); ++pos) {
        إذا ((pos == namein.begin()) ? std::اکر_آهي(pos[0])  // digits can't lead identifiers
                                    : std::اکر_يا_انگ(pos[0])) {
            ٻاهريون += pos[0];
        } وإلا إذا (pos[0] == '_') {
            ٻاهريون += pos[0];
            إذا (pos + 1 == namein.پڇاڙي()) اكسر;
            إذا (pos[1] == '_') {
                ++pos;
                ٻاهريون += "__05F";  // hex(_) = 0x5F
            }
        } وإلا {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ثابت بنانشان val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            ٻاهريون += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{ٻاهريون};
    أرجع vname.hashedName();
}

string Ast_ڳنڍ::encode_عدد(int64_t انگ) {
    إذا (انگ < 0) أرجع "__02D" + cvt_منزل_Str(-انگ);  // 2D=-
    أرجع cvt_منزل_Str(انگ);
}

string Ast_ڳنڍ::nameProtect() ثابت { أرجع VIdProtect::protectIf(name(), protect()); }
string Ast_ڳنڍ::origNameProtect() ثابت { أرجع VIdProtect::protectIf(origName(), protect()); }

string Ast_ڳنڍ::shortName() ثابت {
    string pretty = name();
    string::سائيز_type pos;
    طالما ((pos = pretty.ڳوليو("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    أرجع pretty;
}

string Ast_ڳنڍ::dedotName(ثابت string& namein) {
    string pretty = namein;
    string::سائيز_type pos;
    طالما ((pos = pretty.ڳوليو("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    إذا (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    أرجع pretty;
}

string Ast_ڳنڍ::nameNoArray(ثابت string& namein) {
    أرجع namein.substr(0, namein.ڳوليو("__BRA__"));
}

string Ast_ڳنڍ::vcdName(ثابت string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::سائيز_type pos;
    طالما ((pos = pretty.ڳوليو("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    طالما ((pos = pretty.ڳوليو('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    أرجع prettyName(pretty);
}

string Ast_ڳنڍ::prettyName(ثابت string& namein) VL_خالص {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.ڊيگهائي());
    لكل (ثابت حرف* pos = namein.c_str(); *pos;) {
        إذا (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            استمر;
        }
        إذا (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            إذا (0 == std::لڙي_ڀيٽ(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                استمر;
            }
            إذا (0 == std::لڙي_ڀيٽ(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                استمر;
            }
            إذا (0 == std::لڙي_ڀيٽ(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                استمر;
            }
            إذا (0 == std::لڙي_ڀيٽ(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                استمر;
            }
            إذا (0 == std::لڙي_ڀيٽ(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                استمر;
            }
            إذا (0 == std::لڙي_ڀيٽ(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                استمر;
            }
            إذا (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                حرف قدر = 0;
                قدر += 16
                         * (std::انگ_آهي(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                قدر
                    += (std::انگ_آهي(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += قدر;
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

string Ast_ڳنڍ::vpiName(ثابت string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ثابت سائيز_t وٿي = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.ڊيگهائي());
    منطقي اندريون_Escaped_Ident = خطأ;
    صحيح آخري_Ident = 0;

    لكل (ثابت حرف* pos = namein.c_str() + وٿي; *pos;) {
        حرف specialChar = 0;
        إذا (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } وإلا إذا (pos[0] == '_' && pos[1] == '_') {  // __
            إذا (0 == std::لڙي_ڀيٽ(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } وإلا إذا (0 == std::لڙي_ڀيٽ(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } وإلا إذا (0 == std::لڙي_ڀيٽ(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } وإلا إذا (0 == std::لڙي_ڀيٽ(pos, "__PVT__", 7)) {
                pos += 7;
                استمر;
            } وإلا إذا (0 == std::لڙي_ڀيٽ(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                حرف قدر = 0;
                قدر += 16
                         * (std::انگ_آهي(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                قدر
                    += (std::انگ_آهي(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                إذا (قدر != '_') اندريون_Escaped_Ident = سچ;

                pretty += قدر;
                pos += 5;
                استمر;
            }
        } وإلا إذا (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        إذا (specialChar) {
            إذا (اندريون_Escaped_Ident && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.وجهو(آخري_Ident, "\\");
                اندريون_Escaped_Ident = خطأ;
            }

            pretty += specialChar;

            إذا (specialChar == ']' || specialChar == '.') {
                آخري_Ident = pretty.ڊيگهائي();
                اندريون_Escaped_Ident = خطأ;
            }
        } وإلا {
            pretty += pos[0];
            ++pos;
        }
    }
    إذا (اندريون_Escaped_Ident) {
        pretty += " ";
        pretty.وجهو(آخري_Ident, "\\");
    }
    إذا (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    إذا (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    أرجع pretty;
}

string Ast_ڳنڍ::prettyTypeName() ثابت {
    إذا (name() == "") أرجع typeName();
    أرجع std::string{typeName()} + " '" + pretty_Name_نياپو() + "'";
}

//######################################################################
// Insertion

فراغ Ast_ڳنڍ::debug_وڻ_Change(ثابت Ast_ڳنڍ* nodep, ثابت حرف* اڳياڙي, صحيح lineno, منطقي ايندڙ) {
#جيڪڏهن_وصف_ٿيل VL_DEBUG
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
#جيڪڏهن_پورو
}

سانچو <>
Ast_ڳنڍ* Ast_ڳنڍ::جوڙيو_ايندڙ<Ast_ڳنڍ, Ast_ڳنڍ>(Ast_ڳنڍ* nodep, Ast_ڳنڍ* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_وڻ_Change(nodep, "-addNextThs: ", __LINE__, خطأ);
    debug_وڻ_Change(newp, "-addNextNew: ", __LINE__, سچ);
    إذا (!nodep)  // verilog.y and lots of other places assume this
        أرجع newp;

    // Find end of old list
    Ast_ڳنڍ* oldtailp = nodep;
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
    Ast_ڳنڍ* ثابت newtailp = newp->m_headtailp;
    Ast_ڳنڍ* ثابت headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = خالي_اشارو;  // May be written again as new head
    newp->m_headtailp = خالي_اشارو;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_ڳڻپ_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_وڻ_Change(nodep, "-addNextOut:", __LINE__, سچ);
    أرجع nodep;
}

فراغ Ast_ڳنڍ::جوڙيو_ايندڙ_هتي(Ast_ڳنڍ* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_وڻ_Change(هذا, "-addHereThs: ", __LINE__, خطأ);
    debug_وڻ_Change(newp, "-addHereNew: ", __LINE__, سچ);
    newp->edit_ڳڻپ_Inc();

    Ast_ڳنڍ* ثابت addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_ڳنڍ* ثابت oldnextp = هذا->m_nextp;
    هذا->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    إذا (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = هذا;

    // Head/tail
    Ast_ڳنڍ* ثابت oldheadtailp = هذا->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = خالي_اشارو;  // Not at head any longer
    addlastp->m_headtailp = خالي_اشارو;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    إذا (oldheadtailp) {  // else in middle of list, no change
        إذا (oldheadtailp == هذا) {  // this was one node
            هذا->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } وإلا إذا (!oldnextp) {  // this was tail
            هذا->m_headtailp = خالي_اشارو;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_وڻ_Change(هذا, "-addHereOut: ", __LINE__, سچ);
}

فراغ Ast_ڳنڍ::سيٽ_Op1p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, هذا, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_وڻ_Change(هذا, "-setOp1pThs: ", __LINE__, خطأ);
    debug_وڻ_Change(newp, "-setOp1pNew: ", __LINE__, سچ);
    m_op1p = newp;
    newp->edit_ڳڻپ_Inc();
    newp->m_backp = هذا;
    debug_وڻ_Change(هذا, "-setOp1pOut: ", __LINE__, خطأ);
}

فراغ Ast_ڳنڍ::سيٽ_Op2p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, هذا, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_وڻ_Change(هذا, "-setOp2pThs: ", __LINE__, خطأ);
    debug_وڻ_Change(newp, "-setOp2pNew: ", __LINE__, سچ);
    m_op2p = newp;
    newp->edit_ڳڻپ_Inc();
    newp->m_backp = هذا;
    debug_وڻ_Change(هذا, "-setOp2pOut: ", __LINE__, خطأ);
}

فراغ Ast_ڳنڍ::سيٽ_Op3p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, هذا, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_وڻ_Change(هذا, "-setOp3pThs: ", __LINE__, خطأ);
    debug_وڻ_Change(newp, "-setOp3pNew: ", __LINE__, سچ);
    m_op3p = newp;
    newp->edit_ڳڻپ_Inc();
    newp->m_backp = هذا;
    debug_وڻ_Change(هذا, "-setOp3pOut: ", __LINE__, خطأ);
}

فراغ Ast_ڳنڍ::سيٽ_Op4p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, هذا, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_وڻ_Change(هذا, "-setOp4pThs: ", __LINE__, خطأ);
    debug_وڻ_Change(newp, "-setOp4pNew: ", __LINE__, سچ);
    m_op4p = newp;
    newp->edit_ڳڻپ_Inc();
    newp->m_backp = هذا;
    debug_وڻ_Change(هذا, "-setOp4pOut: ", __LINE__, خطأ);
}

فراغ Ast_ڳنڍ::جوڙيو_Op1p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op1p) {
        op1p(newp);
    } وإلا {
        m_op1p->جوڙيو_ايندڙ(newp);
    }
}

فراغ Ast_ڳنڍ::جوڙيو_Op2p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op2p) {
        op2p(newp);
    } وإلا {
        m_op2p->جوڙيو_ايندڙ(newp);
    }
}

فراغ Ast_ڳنڍ::جوڙيو_Op3p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op3p) {
        op3p(newp);
    } وإلا {
        m_op3p->جوڙيو_ايندڙ(newp);
    }
}

فراغ Ast_ڳنڍ::جوڙيو_Op4p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op4p) {
        op4p(newp);
    } وإلا {
        m_op4p->جوڙيو_ايندڙ(newp);
    }
}

فراغ Ast_ڳنڍ::prependOp1p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op1p) {
        op1p(newp);
    } وإلا {
        m_op1p->جوڙيو_هتي_This_As_ايندڙ(newp);
    }
}

فراغ Ast_ڳنڍ::prependOp2p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op2p) {
        op2p(newp);
    } وإلا {
        m_op2p->جوڙيو_هتي_This_As_ايندڙ(newp);
    }
}

فراغ Ast_ڳنڍ::prependOp3p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op3p) {
        op3p(newp);
    } وإلا {
        m_op3p->جوڙيو_هتي_This_As_ايندڙ(newp);
    }
}

فراغ Ast_ڳنڍ::prependOp4p(Ast_ڳنڍ* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    إذا (!m_op4p) {
        op4p(newp);
    } وإلا {
        m_op4p->جوڙيو_هتي_This_As_ايندڙ(newp);
    }
}

فراغ Ast_ڳنڍ::replaceWith(Ast_ڳنڍ* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    هذا->unlink_Fr_پٺ(&repHandle);
    repHandle.relink(newp);
}
فراغ Ast_ڳنڍ::replaceWithKeepDType(Ast_ڳنڍ* newp) {
    newp->dtypeFrom(هذا);
    replaceWith(newp);
}

فراغ VNRelinker::dump(std::ostream& str) ثابت {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_ايندڙ ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_ڳنڍ* Ast_ڳنڍ::unlink_Fr_پٺ_With_ايندڙ(VNRelinker* linkerp) {
    debug_وڻ_Change(هذا, "-unlinkWNextThs: ", __LINE__, سچ);
    Ast_ڳنڍ* ثابت oldp = هذا;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ڳڻپ_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ڳنڍ* ثابت backp = oldp->m_backp;
    إذا (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        إذا (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_ايندڙ;
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
        backp->m_nextp = خالي_اشارو;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_ڳنڍ* oldtailp = backp->m_headtailp;
        إذا (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            طالما (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_ڳنڍ* ثابت oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } وإلا إذا (backp->m_op1p == oldp) {
        backp->m_op1p = خالي_اشارو;
    } وإلا إذا (backp->m_op2p == oldp) {
        backp->m_op2p = خالي_اشارو;
    } وإلا إذا (backp->m_op3p == oldp) {
        backp->m_op3p = خالي_اشارو;
    } وإلا إذا (backp->m_op4p == oldp) {
        backp->m_op4p = خالي_اشارو;
    } وإلا {
        هذا->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = خالي_اشارو;
    // Iterator fixup
    إذا (oldp->m_iterpp) {
        *(oldp->m_iterpp) = خالي_اشارو;
        oldp->m_iterpp = خالي_اشارو;
    }
    debug_وڻ_Change(oldp, "-unlinkWNextOut: ", __LINE__, سچ);
    أرجع oldp;
}

Ast_ڳنڍ* Ast_ڳنڍ::unlink_Fr_پٺ(VNRelinker* linkerp) {
    debug_وڻ_Change(هذا, "-unlinkFrBkThs: ", __LINE__, سچ);
    Ast_ڳنڍ* ثابت oldp = هذا;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ڳڻپ_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ڳنڍ* ثابت backp = oldp->m_backp;
    إذا (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        إذا (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = خالي_اشارو;
            oldp->m_iterpp = خالي_اشارو;
        }
        إذا (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_ايندڙ;
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
            Ast_ڳنڍ* ثابت newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    إذا (oldp->m_iterpp) {  // Only if no linker, point to next in list
        إذا (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = خالي_اشارو;
    }
    // Relink
    oldp->m_nextp = خالي_اشارو;
    oldp->m_backp = خالي_اشارو;
    oldp->m_headtailp = oldp;
    debug_وڻ_Change(oldp, "-unlinkFrBkOut: ", __LINE__, سچ);
    أرجع oldp;
}

فراغ Ast_ڳنڍ::relink(VNRelinker* linkerp) {
    إذا (debug() > 8) {
        UINFO_اڳياڙي(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_ڳنڍ* ثابت newp = هذا;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_ڳڻپ_Inc();

    إذا (debug() > 8) {
        linkerp->dump(ٻاهر);
        ٻاهر << سٽ_پڄاڻي;
    }

    // cppcheck-suppress shadowFunction
    Ast_ڳنڍ* ثابت backp = linkerp->m_backp;
    debug_وڻ_Change(هذا, "-relinkNew: ", __LINE__, سچ);
    debug_وڻ_Change(backp, "-relinkTre: ", __LINE__, سچ);

    بدل (linkerp->m_chg) {
    حالة VNRelinker::RELINK_ايندڙ: backp->جوڙيو_ايندڙ_هتي(newp); اكسر;
    حالة VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); اكسر;
    حالة VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); اكسر;
    افتراضي: هذا->v3fatalSrc("Relink of node without any link to change."); اكسر;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = خالي_اشارو;
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
    linkerp->m_backp = خالي_اشارو;
    debug_وڻ_Change(هذا, "-relinkOut: ", __LINE__, سچ);
}

فراغ Ast_ڳنڍ::relinkOneLink(Ast_ڳنڍ*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_ڳنڍ* newp) {
    إذا (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_ڳنڍ* ثابت newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_ڳنڍ* ثابت oldlistlastp = pointpr->m_headtailp;
        UASSERT_OBJ(!(oldlistlastp->m_nextp && oldlistlastp != pointpr), newp,
                    "Old headtailp tail isn't at the tail");
        // Next links
        newlistlastp->m_nextp = pointpr;
        pointpr->m_backp = newlistlastp;
        // Head/tail
        pointpr->m_headtailp = خالي_اشارو;  // Old head
        newlistlastp->m_headtailp = خالي_اشارو;  // Old tail
        newp->m_headtailp = oldlistlastp;  // Head points to tail
        oldlistlastp->m_headtailp = newp;  // Tail points to head
    }
    pointpr = newp;
}

فراغ Ast_ڳنڍ::جوڙيو_هتي_This_As_ايندڙ(Ast_ڳنڍ* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(هذا->m_backp, هذا, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_ڳنڍ* ثابت backp = هذا->m_backp;
    Ast_ڳنڍ* ثابت newLastp = newp->m_headtailp;
    //
    هذا->edit_ڳڻپ_Inc();
    // Common linkage
    newLastp->m_nextp = هذا;
    هذا->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = خالي_اشارو;
    // Linkage dependent on position
    إذا (backp && backp->m_nextp == هذا) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = خالي_اشارو;
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
        Ast_ڳنڍ* ثابت tailp = هذا->m_headtailp;
        هذا->m_headtailp = خالي_اشارو;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    إذا (newLastp->m_iterpp) *(newLastp->m_iterpp) = هذا;
    إذا (هذا->m_iterpp) {
        *(هذا->m_iterpp) = newp;
        هذا->m_iterpp = خالي_اشارو;
    }
    //
    debug_وڻ_Change(هذا, "-addHereThisAsNext: ", __LINE__, سچ);
}

//======================================================================
// Clone

Ast_ڳنڍ* Ast_ڳنڍ::clone_وڻ_Iter(منطقي need_خالص) {
    // private: Clone single node and children
    إذا (need_خالص) purityCheck();
    Ast_ڳنڍ* ثابت newp = هذا->clone();
    إذا (هذا->m_op1p) newp->op1p(هذا->m_op1p->clone_وڻ_Iter_List(need_خالص));
    إذا (هذا->m_op2p) newp->op2p(هذا->m_op2p->clone_وڻ_Iter_List(need_خالص));
    إذا (هذا->m_op3p) newp->op3p(هذا->m_op3p->clone_وڻ_Iter_List(need_خالص));
    إذا (هذا->m_op4p) newp->op4p(هذا->m_op4p->clone_وڻ_Iter_List(need_خالص));
    newp->m_iterpp = خالي_اشارو;
    newp->clonep(هذا);  // Save pointers to/from both to simplify relinking.
    هذا->clonep(newp);  // Save pointers to/from both to simplify relinking.
    أرجع newp;
}

Ast_ڳنڍ* Ast_ڳنڍ::clone_وڻ_Iter_List(منطقي need_خالص) {
    // private: Clone list of nodes, set m_headtailp
    Ast_ڳنڍ* newheadp = خالي_اشارو;
    Ast_ڳنڍ* newtailp = خالي_اشارو;
    // Audited to make sure this is never nullptr
    لكل (Ast_ڳنڍ* oldp = هذا; oldp; oldp = oldp->m_nextp) {
        Ast_ڳنڍ* ثابت newp = oldp->clone_وڻ_Iter(need_خالص);
        newp->m_headtailp = خالي_اشارو;
        newp->m_backp = newtailp;
        إذا (newtailp) newtailp->m_nextp = newp;
        إذا (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    أرجع newheadp;
}

Ast_ڳنڍ* Ast_ڳنڍ::clone_وڻ(منطقي clone_ايندڙ_Link, منطقي need_خالص) {
    debug_وڻ_Change(هذا, "-cloneThs: ", __LINE__, clone_ايندڙ_Link);
    clone_Clear_وڻ();
    Ast_ڳنڍ* newp;
    إذا (clone_ايندڙ_Link && هذا->m_nextp) {
        newp = clone_وڻ_Iter_List(need_خالص);
    } وإلا {
        newp = clone_وڻ_Iter(need_خالص);
        newp->m_nextp = خالي_اشارو;
        newp->m_headtailp = newp;
    }
    newp->m_backp = خالي_اشارو;
    newp->clone_Relink_وڻ();
    debug_وڻ_Change(newp, "-cloneOut: ", __LINE__, سچ);
    أرجع newp;
}

فراغ Ast_ڳنڍ::purityCheck() {
    إذا (VL_UNLIKELY(!is_خالص())) {
        هذا->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << هذا->warn_وڌيڪ()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

فراغ Ast_ڳنڍ::delete_ڳنڍ() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_ڳڻپ_Inc();
    // Change links of old node so we coredump if used
    هذا->m_nextp = reinterpret_cast<Ast_ڳنڍ*>(0x1);
    هذا->m_backp = reinterpret_cast<Ast_ڳنڍ*>(0x1);
    هذا->m_headtailp = reinterpret_cast<Ast_ڳنڍ*>(0x1);
    هذا->m_op1p = reinterpret_cast<Ast_ڳنڍ*>(0x1);
    هذا->m_op2p = reinterpret_cast<Ast_ڳنڍ*>(0x1);
    هذا->m_op3p = reinterpret_cast<Ast_ڳنڍ*>(0x1);
    هذا->m_op4p = reinterpret_cast<Ast_ڳنڍ*>(0x1);
    هذا->m_iterpp = reinterpret_cast<Ast_ڳنڍ**>(0x1);
    إذا (
#إذا !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        سچ
#وإلا
        !v3Global.opt.debugLeak()
#جيڪڏهن_پورو
    ) {
        ميٽيو هذا;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

فراغ Ast_ڳنڍ::delete_وڻ_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    لكل (Ast_ڳنڍ *nodep = هذا, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        إذا (nodep->m_op1p) nodep->m_op1p->delete_وڻ_Iter();
        إذا (nodep->m_op2p) nodep->m_op2p->delete_وڻ_Iter();
        إذا (nodep->m_op3p) nodep->m_op3p->delete_وڻ_Iter();
        إذا (nodep->m_op4p) nodep->m_op4p->delete_وڻ_Iter();
        nodep->m_nextp = خالي_اشارو;
        nodep->m_backp = خالي_اشارو;
        nodep->delete_ڳنڍ();
    }
}

فراغ Ast_ڳنڍ::delete_وڻ() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_وڻ_Change(هذا, "-delTree:  ", __LINE__, سچ);
    هذا->edit_ڳڻپ_Inc();
    // MUST be depth first!
    delete_وڻ_Iter();
}

//======================================================================
// Memory checks

#جيڪڏهن_وصف_ٿيل VL_LEAK_CHECKS
فراغ* Ast_ڳنڍ::عامل جديد(سائيز_t سائيز) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_ڳنڍ* ثابت objp = static_cast<Ast_ڳنڍ*>(::عامل جديد(سائيز));
    V3Broken::جوڙيو_Newed(objp);
    أرجع objp;
}

فراغ Ast_ڳنڍ::عامل ميٽيو(فراغ* objp, سائيز_t سائيز) {
    إذا (!objp) أرجع;
    ثابت Ast_ڳنڍ* ثابت nodep = static_cast<Ast_ڳنڍ*>(objp);
    V3Broken::deleted(nodep);
    ::عامل ميٽيو(objp);
}
#جيڪڏهن_پورو

#جيڪڏهن_وصف_ٿيل VL_ALLOC_RANDOM_CHECKS
فراغ* Ast_ڳنڍ::عامل جديد(سائيز_t سائيز) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ساكن ثابت سائيز_t ASTGEN_MAX_ڳنڍ_سائيز = []() {
        سائيز_t max_سائيز = 0;
        لكل (سائيز_t t = 0; t < VNType::انگ_TYPES(); ++t) {
            max_سائيز = std::max(max_سائيز, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        أرجع max_سائيز;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr سائيز_t POOL_سائيز = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ساكن uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    إذا (صحيح ٻج = v3Global.opt.debugAllocRandom()) {
        ساكن V3Mutex s_mutex;
        ثابت V3_تالو_Guard تالو_2{s_mutex};
        ساكن std::array<فراغ*, POOL_سائيز> s_ڳنڍ_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(سائيز <= ASTGEN_MAX_ڳنڍ_سائيز, "fix ASTGEN_MAX_NODE_SIZE");
        إذا (!s_lfsr) {
            s_lfsr = ٻج;
            إذا (!s_lfsr) s_lfsr = ~s_lfsr;
            لكل (سائيز_t i = 0; i < POOL_سائيز; ++i) {
                s_ڳنڍ_Pool[i] = ::عامل جديد(ASTGEN_MAX_ڳنڍ_سائيز + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::ترتيب_ڏيو(std::begin(s_ڳنڍ_Pool), std::پڇاڙي(s_ڳنڍ_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ثابت سائيز_t اشاريو = s_lfsr % POOL_سائيز;
        Ast_ڳنڍ* ثابت objp = static_cast<Ast_ڳنڍ*>(s_ڳنڍ_Pool[اشاريو]);
        s_ڳنڍ_Pool[اشاريو] = ::عامل جديد(ASTGEN_MAX_ڳنڍ_سائيز + 64);  // For later new()
        أرجع objp;
    } وإلا {
        Ast_ڳنڍ* ثابت objp = static_cast<Ast_ڳنڍ*>(::عامل جديد(سائيز));
        أرجع objp;
    }
}
فراغ Ast_ڳنڍ::عامل ميٽيو(فراغ* objp, سائيز_t سائيز) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (فراغ)objp;
    (فراغ)سائيز;
}
#جيڪڏهن_پورو

//======================================================================
// Iterators

فراغ Ast_ڳنڍ::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    إذا (m_op1p) m_op1p->iterate_And_ايندڙ(v);
    إذا (m_op2p) m_op2p->iterate_And_ايندڙ(v);
    إذا (m_op3p) m_op3p->iterate_And_ايندڙ(v);
    إذا (m_op4p) m_op4p->iterate_And_ايندڙ(v);
}

فراغ Ast_ڳنڍ::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    إذا (m_op1p) m_op1p->iterate_And_ايندڙ_Const(v);
    إذا (m_op2p) m_op2p->iterate_And_ايندڙ_Const(v);
    إذا (m_op3p) m_op3p->iterate_And_ايندڙ_Const(v);
    إذا (m_op4p) m_op4p->iterate_And_ايندڙ_Const(v);
}

فراغ Ast_ڳنڍ::iterate_And_ايندڙ(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_ڳنڍ* nodep = هذا;
#جيڪڏهن_وصف_ٿيل VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#جيڪڏهن_پورو
    // cppcheck-suppress knownConditionTrueFalse
    إذا (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    طالما (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        إذا (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_ڳنڍ* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        niterp->m_iterpp = خالي_اشارو;
        إذا (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } وإلا {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

فراغ Ast_ڳنڍ::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_ڳنڍ* nodep = هذا;
    طالما (nodep->m_nextp) nodep = nodep->m_nextp;
    طالما (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        إذا (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } وإلا {
            nodep = خالي_اشارو;
        }  // else: backp points up the tree.
    }
}

فراغ Ast_ڳنڍ::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    إذا (m_op1p) m_op1p->iterateListBackwardsConst(v);
    إذا (m_op2p) m_op2p->iterateListBackwardsConst(v);
    إذا (m_op3p) m_op3p->iterateListBackwardsConst(v);
    إذا (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

فراغ Ast_ڳنڍ::iterate_And_ايندڙ_Const(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_ڳنڍ* nodep = هذا;
    افعل {
        Ast_ڳنڍ* ثابت nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } طالما (nodep);
}

Ast_ڳنڍ* Ast_ڳنڍ::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_ڳنڍ* nodep = هذا;  // Note "this" may point to bogus point later in this function
    إذا (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } وإلا إذا (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ثابت tempp = جديد AstBegin{nodep->fileline(), "[EditWrapper]", nodep, خطأ};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ();
        VL_DO_DANGLING(tempp->delete_وڻ(), tempp);
    } وإلا {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_ڳنڍ** nextnodepp = خالي_اشارو;
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

فراغ Ast_ڳنڍ::clone_Relink_وڻ() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    لكل (Ast_ڳنڍ* nodep = هذا; nodep; nodep = nodep->m_nextp) {
        إذا (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        إذا (nodep->m_op1p) nodep->m_op1p->clone_Relink_وڻ();
        إذا (nodep->m_op2p) nodep->m_op2p->clone_Relink_وڻ();
        إذا (nodep->m_op3p) nodep->m_op3p->clone_Relink_وڻ();
        إذا (nodep->m_op4p) nodep->m_op4p->clone_Relink_وڻ();
    }
}

//======================================================================
// Comparison

منطقي Ast_ڳنڍ::gate_وڻ_Iter() ثابت {
    // private: Return true if the two trees are identical
    إذا (!isGateOptimizable()) أرجع خطأ;
    إذا (m_op1p && !m_op1p->gate_وڻ_Iter()) أرجع خطأ;
    إذا (m_op2p && !m_op2p->gate_وڻ_Iter()) أرجع خطأ;
    إذا (m_op3p && !m_op3p->gate_وڻ_Iter()) أرجع خطأ;
    إذا (m_op4p && !m_op4p->gate_وڻ_Iter()) أرجع خطأ;
    أرجع سچ;
}

منطقي Ast_ڳنڍ::same_وڻ_Iter(ثابت Ast_ڳنڍ* node1p, ثابت Ast_ڳنڍ* node2p, منطقي ign_ايندڙ,
                           منطقي gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    طالما (سچ) {
        إذا (!node1p && !node2p) أرجع سچ;
        إذا (!node1p || !node2p) أرجع خطأ;
        إذا (node1p->type() != node2p->type()) أرجع خطأ;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        إذا (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) أرجع خطأ;
        إذا (!node1p->same_ڳنڍ(node2p) || (gateOnly && !node1p->isGateOptimizable())) أرجع خطأ;
        إذا (!same_وڻ_Iter(node1p->m_op1p, node2p->m_op1p, خطأ, gateOnly)
            || !same_وڻ_Iter(node1p->m_op2p, node2p->m_op2p, خطأ, gateOnly)
            || !same_وڻ_Iter(node1p->m_op3p, node2p->m_op3p, خطأ, gateOnly)
            || !same_وڻ_Iter(node1p->m_op4p, node2p->m_op4p, خطأ, gateOnly)) {
            أرجع خطأ;
        }
        إذا (ign_ايندڙ) أرجع سچ;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

فراغ Ast_ڳنڍ::check_وڻ_Iter(ثابت Ast_ڳنڍ* prevBackp) ثابت VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == هذا->backp(), هذا, "Back node inconsistent");
    ثابت VNTypeInfo& typeInfo = VNType::typeInfo(هذا->type());
    لكل (صحيح i = 1; i <= 4; i++) {
        Ast_ڳنڍ* nodep = خالي_اشارو;
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
        حالة VNTypeInfo::OP_اڻ_استعمال_ٿيل:
            UASSERT_OBJ(!nodep, هذا, typeInfo.m_namep << " must not use " << opName << "()");
            اكسر;
        حالة VNTypeInfo::OP_استعمال_ٿيل:
            UASSERT_OBJ(nodep, هذا,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), هذا,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_وڻ_Iter(هذا);
            اكسر;
        حالة VNTypeInfo::OP_LIST:
            إذا (ثابت Ast_ڳنڍ* ثابت headp = nodep) {
                // cppcheck-suppress shadowFunction
                ثابت Ast_ڳنڍ* backp = هذا;
                ثابت Ast_ڳنڍ* tailp;
                ثابت Ast_ڳنڍ* opp = headp;
                افعل {
                    opp->check_وڻ_Iter(backp);
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
                nodep->check_وڻ_Iter(هذا);
            }
            اكسر;
        افتراضي: هذا->v3fatalSrc("Bad case"); اكسر;
        }
    }
    إذا (v3Global.opt.debug_ويڪر() && v3Global.ويڪر_Min_Usage() == VWidthMinUsage::VERILOG_ويڪر) {
        إذا (ثابت Ast_ڳنڍ_اظهار* ثابت exprp = VN_CAST(هذا, ڳنڍ_اظهار)) {
            ثابت حرف* ثابت whyp = exprp->ويڪر_Mismatch();
            إذا (whyp) {
                خودڪار dtypeStr = [](ثابت Ast_ڳنڍ_اظهار* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    أرجع ss.str();
                };
                إذا (ثابت Ast_ڳنڍ_Uniop* ثابت uniopp = VN_CAST(exprp, ڳنڍ_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } وإلا إذا (ثابت Ast_ڳنڍ_Biop* ثابت biopp = VN_CAST(exprp, ڳنڍ_Biop)) {
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
حرف* Ast_ڳنڍ::dump_وڻ_Json_Gdb(ثابت Ast_ڳنڍ* nodep) {
    إذا (!nodep) أرجع strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodep_وهڪرو;
    nodep->dump_وڻ_Json(nodep_وهڪرو);
    ثابت std::string str = nodep_وهڪرو.rdbuf()->str();
    أرجع strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
حرف* Ast_ڳنڍ::dump_وڻ_Json_Gdb(ثابت حرف* str) { أرجع strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
حرف* Ast_ڳنڍ::dump_وڻ_Json_Gdb(intptr_t nodep) {
    إذا (!nodep) أرجع strdup("{\"addr\":\"NULL\"}\n");
    أرجع dump_وڻ_Json_Gdb(reinterpret_cast<ثابت Ast_ڳنڍ*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_ڳنڍ::dumpGdb(ثابت Ast_ڳنڍ* nodep) {  // For GDB only  // LCOV_EXCL_START
    إذا (!nodep) {
        ٻاهر << "<nullptr>" << سٽ_پڄاڻي;
        أرجع;
    }
    nodep->dump_Gdb_سرو();
    ٻاهر << "  ";
    nodep->dump(ٻاهر);
    ٻاهر << سٽ_پڄاڻي;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_ڳنڍ::dump_Gdb_سرو() ثابت {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(ٻاهر);
    ٻاهر << "  Fileline = " << fileline() << سٽ_پڄاڻي;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_ڳنڍ::dump_وڻ_Gdb(ثابت Ast_ڳنڍ* nodep) {  // For GDB only  // LCOV_EXCL_START
    إذا (!nodep) {
        ٻاهر << "<nullptr>" << سٽ_پڄاڻي;
        أرجع;
    }
    nodep->dump_Gdb_سرو();
    nodep->dump_وڻ(ٻاهر);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_ڳنڍ::dump_وڻ_File_Gdb(ثابت Ast_ڳنڍ* nodep,  // LCOV_EXCL_START
                              ثابت حرف* filenamep) {  // For GDB only
    إذا (!nodep) {
        ٻاهر << "<nullptr>" << سٽ_پڄاڻي;
        أرجع;
    }
    ثابت string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_وڻ_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
فراغ Ast_ڳنڍ::checkIter() ثابت {
    إذا (m_iterpp) {
        dumpPtrs(ٻاهر);
        // Perhaps something forgot to clear m_iterpp?
        هذا->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

فراغ Ast_ڳنڍ::dumpPtrs(std::ostream& os) ثابت {
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
    ثابت خودڪار dumpUser = [&os](ثابت حرف* اڳياڙي, ثابت VNUser& user) {
        ثابت std::string s
            = user.dumpStr([](ثابت فراغ* p) -> std::string { أرجع cvt_منزل_Hex(p); });
        إذا (!s.خالي_جايون()) os << اڳياڙي << s;
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

فراغ Ast_ڳنڍ::dump_وڻ(std::ostream& os, ثابت string& indent, صحيح max_اونهائي) ثابت {
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
           << سٽ_پڄاڻي;  // endl intentional to do flush
        أرجع;
    }  // LCOV_EXCL_STOP
    إذا (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    إذا (max_اونهائي == 1) {
        إذا (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } وإلا {
        لكل (ثابت Ast_ڳنڍ* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_وڻ(os, indent + "1:", max_اونهائي - 1);
        }
        لكل (ثابت Ast_ڳنڍ* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_وڻ(os, indent + "2:", max_اونهائي - 1);
        }
        لكل (ثابت Ast_ڳنڍ* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_وڻ(os, indent + "3:", max_اونهائي - 1);
        }
        لكل (ثابت Ast_ڳنڍ* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_وڻ(os, indent + "4:", max_اونهائي - 1);
        }
    }
}

فراغ Ast_ڳنڍ::dump_وڻ_And_ايندڙ(std::ostream& os, ثابت string& indent, صحيح max_اونهائي) ثابت {
    // Audited to make sure this is never nullptr
    لكل (ثابت Ast_ڳنڍ* nodep = هذا; nodep; nodep = nodep->nextp()) {
        nodep->dump_وڻ(os, indent, max_اونهائي);
    }
}

فراغ Ast_ڳنڍ::dump_وڻ_File(ثابت string& filename, منطقي doDump) {
    // Not const function as calls checkTree
    إذا (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            إذا (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_ڳڻپ_آخري();
            *logsp << "> to <e" << std::dec << edit_ڳڻپ_Gbl() << ">\n";
            إذا (edit_ڳڻپ_Gbl() == edit_ڳڻپ_آخري() && ::dump_وڻ_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } وإلا {
                dump_وڻ(*logsp);
                edit_ڳڻپ_سيٽ_آخري();  // Next dump can indicate start from here
            }
        }
    }
}

ساكن فراغ drawChildren(std::ostream& os, ثابت Ast_ڳنڍ* thisp, ثابت Ast_ڳنڍ* childp,
                         ثابت std::string& ٻار_Name) {
    إذا (childp) {
        os << "\tn" << cvt_منزل_Hex(thisp) << " -> n" << cvt_منزل_Hex(childp) << " [" << "label=\""
           << ٻار_Name << "\" color=red];\n";
        لكل (ثابت Ast_ڳنڍ* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_وڻ_Dot(os);
            إذا (nodep->nextp()) {
                os << "\tn" << cvt_منزل_Hex(nodep) << " -> n" << cvt_منزل_Hex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvt_منزل_Hex(nodep) << ", n" << cvt_منزل_Hex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

فراغ Ast_ڳنڍ::dump_وڻ_Dot(std::ostream& os) ثابت {
    os << "\tn" << cvt_منزل_Hex(هذا) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, هذا, m_op1p, "op1");
    drawChildren(os, هذا, m_op2p, "op2");
    drawChildren(os, هذا, m_op3p, "op3");
    drawChildren(os, هذا, m_op4p, "op4");
}

فراغ Ast_ڳنڍ::dump_وڻ_Json_File(ثابت string& filename, منطقي doDump) {
    إذا (!doDump) أرجع;
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    إذا (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_وڻ_Json(*treejsonp);
    *treejsonp << '\n';
}

فراغ Ast_ڳنڍ::dumpJsonMetaFileGdb(ثابت حرف* filename) { dumpJsonMetaFile(filename); }
فراغ Ast_ڳنڍ::dumpJsonMetaFile(ثابت string& filename) {
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    إذا (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_سٽ::file_Name_انگ_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptr_نالا_Dump_Json(*treejsonp);
    *treejsonp << "}\n";
}

فراغ Ast_ڳنڍ::dump_وڻ_Dot_File(ثابت string& filename, منطقي doDump) {
    إذا (doDump) {
        UINFO(2, "Dumping " << filename);
        ثابت std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        إذا (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_وڻ_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_ڳنڍ::instanceStr() ثابت {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr بنانشان maxIterations = 10000;
    بنانشان iter_ڳڻپ = 0;
    // Walk 'backp' chain
    لكل (ثابت Ast_ڳنڍ* currp = هذا; currp; currp = currp->backp(), ++iter_ڳڻپ) {
        إذا (VL_UNCOVERABLE(iter_ڳڻپ >= maxIterations)) أرجع "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        إذا (ثابت AstScope* ثابت scopep = VN_CAST(currp, Scope)) {
            أرجع scopep->is_چوٽي() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        إذا (ثابت AstModule* ثابت modp = VN_CAST(currp, Module)) {
            ثابت string instanceName = modp->someInstanceName();
            أرجع instanceName.خالي_جايون() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    أرجع "";
}
فراغ Ast_ڳنڍ::v3error_پڇاڙي(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    إذا (!m_fileline) {
        V3Error::v3error_پڇاڙي(str, "", خالي_اشارو);
    } وإلا {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ثابت string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().error_ڪوڊ()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        إذا (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_ڳنڍ*>(هذا)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3error_پڇاڙي(nsstr, instanceStrExtra);
    }
}
فراغ Ast_ڳنڍ::v3error_پڇاڙي_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_پڇاڙي(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

فراغ Ast_ڳنڍ::dtypeChgSigned(منطقي flag) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing to (un)signed");
    dtype_Chg_ويڪر_Signed(dtypep()->ويڪر(), dtypep()->ويڪر_Min(), VSigning::fromBool(flag));
}
فراغ Ast_ڳنڍ::dtype_Chg_ويڪر(صحيح ويڪر, صحيح ويڪر_Min) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing width");
    dtype_Chg_ويڪر_Signed(ويڪر, ويڪر_Min, dtypep()->numeric());
}

فراغ Ast_ڳنڍ::dtype_Chg_ويڪر_Signed(صحيح ويڪر, صحيح ويڪر_Min, VSigning numeric) {
    UASSERT_OBJ(dtypep(), هذا, "No dtype when changing width");
    ثابت Ast_ڳنڍ_DType* ثابت enumDTypep = dtypep()->skip_Ref_منزل_Enump();
    إذا (ويڪر == dtypep()->ويڪر() && ويڪر_Min == dtypep()->ويڪر_Min()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        أرجع;  // Correct already
    }
    ثابت Ast_ڳنڍ_DType* ثابت dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtype_سيٽ_ٻٽو_انگ_Or_Logic_Unsized(ويڪر, ويڪر_Min, numeric, dtypep->isFourstate());
}

Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_Basic_DType(VBasicDTypeKwd kwd) ثابت {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_Basic_DType(fileline(), kwd);
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_ٻٽو_انگ_DType(صحيح ويڪر, صحيح ويڪر_Min, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_Logic_ٻٽو_انگ_DType(fileline(), VBasicDTypeKwd::ٻٽو_انگ,
                                                             ويڪر, ويڪر_Min, numeric);
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_Logic_DType(صحيح ويڪر, صحيح ويڪر_Min, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_Logic_ٻٽو_انگ_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             ويڪر, ويڪر_Min, numeric);
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_Logic2_State_DType(صحيح ويڪر, صحيح ويڪر_Min, VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_Logic_ٻٽو_انگ_DType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, ويڪر, ويڪر_Min, numeric);
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_Logic_Range_DType(ثابت VNumRange& range, صحيح ويڪر_Min,
                                           VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_Logic_ٻٽو_انگ_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, ويڪر_Min, numeric);
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_ٻٽو_انگ_Range_DType(ثابت VNumRange& range, صحيح ويڪر_Min,
                                         VSigning numeric) ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_Logic_ٻٽو_انگ_DType(fileline(), VBasicDTypeKwd::ٻٽو_انگ,
                                                             range, ويڪر_Min, numeric);
}
AstBasicDType* Ast_ڳنڍ::ڳوليو_وجهو_Same_DType(AstBasicDType* nodep) {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_وجهو_Same_DType(nodep);
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_Constraint_Ref_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_Constraint_Ref_DType(fileline());
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_خالي_جايون_قطار_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_خالي_جايون_قطار_DType(fileline());
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_قطار_اشاريو_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_قطار_اشاريو_DType(fileline());
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_وهڪرو_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_وهڪرو_DType(fileline());
}
Ast_ڳنڍ_DType* Ast_ڳنڍ::ڳوليو_Void_DType() ثابت {
    أرجع v3Global.rootp()->typeTablep()->ڳوليو_Void_DType(fileline());
}

ساكن ثابت Ast_ڳنڍ_DType* compute_Castable_بنياد(ثابت Ast_ڳنڍ_DType* nodep) {
    طالما (سچ) {
        إذا (ثابت AstPackArrayDType* ثابت packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            استمر;
        }
        إذا (ثابت Ast_ڳنڍ_DType* ثابت refp = nodep->skip_Ref_منزل_Enump()) {
            إذا (refp != nodep) {
                nodep = refp;
                استمر;
            }
        }
        أرجع nodep;
    }
}

ساكن VCastable computeCastableImp(ثابت Ast_ڳنڍ_DType* منزل_Dtp, ثابت Ast_ڳنڍ_DType* fromDtp,
                                    ثابت Ast_ڳنڍ* fromConstp, ثابت منطقي checkIfaceArgCompat) {
    ثابت VCastable castable = VCastable::UNSUPPORTED;
    منزل_Dtp = منزل_Dtp->skip_Ref_منزل_Enump();
    fromDtp = fromDtp->skip_Ref_منزل_Enump();
    إذا (منزل_Dtp == fromDtp) أرجع VCastable::SAMEISH;
    إذا (منزل_Dtp->similarDType(fromDtp)) أرجع VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ثابت Ast_ڳنڍ_DType* from_بنياد_Dtp = compute_Castable_بنياد(fromDtp);

    ثابت منطقي fromNumericable = VN_IS(from_بنياد_Dtp, BasicDType) || VN_IS(from_بنياد_Dtp, EnumDType)
                                 || VN_IS(from_بنياد_Dtp, وهڪرو_DType)
                                 || VN_IS(from_بنياد_Dtp, ڳنڍ_UOr_Struct_DType);

    ثابت Ast_ڳنڍ_DType* منزل_بنياد_Dtp = compute_Castable_بنياد(منزل_Dtp);
    ثابت منطقي منزل_Numericable
        = VN_IS(منزل_بنياد_Dtp, BasicDType) || VN_IS(منزل_بنياد_Dtp, ڳنڍ_UOr_Struct_DType);

    إذا (منزل_بنياد_Dtp == from_بنياد_Dtp) أرجع VCastable::COMPATIBLE;

    إذا (منزل_Numericable) {
        إذا (fromNumericable) أرجع VCastable::COMPATIBLE;
    } وإلا إذا (VN_IS(منزل_بنياد_Dtp, EnumDType)) {
        إذا (VN_IS(from_بنياد_Dtp, EnumDType) && منزل_Dtp->same_وڻ(fromDtp))
            أرجع VCastable::ENUM_IMPLICIT;
        إذا (fromNumericable) أرجع VCastable::ENUM_EXPLICIT;
    } وإلا إذا (VN_IS(منزل_Dtp, قطار_DType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, وهڪرو_DType))) {
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
               = VN_CAST(checkIfaceArgCompat ? منزل_Dtp->elemDTypep(سچ) : منزل_Dtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ثابت AstIfaceRefDType* ثابت fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(سچ) : fromDtp, IfaceRefDType);
        إذا (checkIfaceArgCompat && fromIfp && (منزل_Dtp != منزل_Ifp || fromDtp != fromIfp)) {
            ثابت AstUnpackArrayDType* ثابت منزل_Arrayp = VN_CAST(منزل_Dtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            إذا (!منزل_Arrayp || !fromArrayp
                || منزل_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
                أرجع VCastable::INCOMPATIBLE;
            }
            أرجع computeCastableImp(منزل_Arrayp->subDTypep(), fromArrayp->subDTypep(), خالي_اشارو,
                                      checkIfaceArgCompat);
        }
        إذا (!fromIfp || منزل_Ifp->iface_ذريعي_Cellp() != fromIfp->iface_ذريعي_Cellp()) {
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
            منزل_Arrayp->subDTypep(), fromArrayp->subDTypep(), خالي_اشارو, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ثابت Ast_ڳنڍ_DType* ثابت منزل_Subp = منزل_Arrayp->subDTypep()->skipRefp();
        إذا (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(منزل_Subp, UnpackArrayDType) || VN_IS(منزل_Subp, IfaceRefDType)))) {
            أرجع VCastable::COMPATIBLE;
        }
    }
    أرجع castable;
}

VCastable Ast_ڳنڍ::computeCastable(ثابت Ast_ڳنڍ_DType* منزل_Dtp, ثابت Ast_ڳنڍ_DType* fromDtp,
                                   ثابت Ast_ڳنڍ* fromConstp, ثابت منطقي checkIfaceArgCompat) {
    ثابت خودڪار castable = computeCastableImp(منزل_Dtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << منزل_Dtp);
    UINFO(9, "     =?= " << fromDtp);
    إذا (fromConstp) UINFO(9, "     const= " << fromConstp);
    أرجع castable;
}

Ast_ڳنڍ_DType* Ast_ڳنڍ::getCommonClassTypep(Ast_ڳنڍ* node1p, Ast_ڳنڍ* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    إذا (VN_IS(node1p, Const)) std::مٽا_سٽا(node1p, node2p);
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
        Ast_ڳنڍ_DType* ثابت edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->ٻار_DTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    أرجع خالي_اشارو;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

صنف VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<صحيح, std::string> m_interned_لفظ_Widths;  // Interned widths
    std::map<صحيح, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ثابت منطقي m_dump_ويڪر_Min = v3Global.ويڪر_Min_Usage() != VWidthMinUsage::MATCHES_ويڪر;

    ساكن std::string منزل_Letters(سائيز_t قدر, منطقي هيٺاهون_Case = خطأ) {
        ثابت حرف بنياد = هيٺاهون_Case ? 'a' : 'A';
        std::string s;
        افعل { s += static_cast<حرف>(بنياد + قدر % 26); } طالما (قدر /= 26);
        أرجع s;
    }

    ثابت std::string& internConst(ثابت AstConst& nodep) {
        ثابت خودڪار pair = m_internedConsts.emplace(nodep.انگ().ascii(خطأ), "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو += منزل_Letters(m_internedConsts.سائيز() - 1);
        أرجع pair.پهريون->ٻيو;
    }

    ثابت std::string& intern_لفظ_ويڪر(صحيح قدر) {
        ثابت خودڪار pair = m_interned_لفظ_Widths.emplace(قدر, "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو += منزل_Letters(m_interned_لفظ_Widths.سائيز() - 1, سچ);
        أرجع pair.پهريون->ٻيو;
    }

    ثابت std::string& intern_Wide_ويڪر(صحيح قدر) {
        ثابت خودڪار pair = m_internedWideWidths.emplace(قدر, "");
        إذا (pair.ٻيو) pair.پهريون->ٻيو += منزل_Letters(m_internedWideWidths.سائيز() - 1);
        أرجع pair.پهريون->ٻيو;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ساكن منطقي is_پن(ثابت Ast_ڳنڍ* nodep) {
        ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        لكل (ثابت VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            إذا (opType != VNTypeInfo::OP_اڻ_استعمال_ٿيل) أرجع خطأ;
        }
        أرجع سچ;
    }

    // Render operand, return true if not unused
    فراغ render_عمليو(VNTypeInfo::OpEn opType, ثابت Ast_ڳنڍ* ثابت opp, uint32_t اونهائي) {
        بدل (opType) {
        حالة VNTypeInfo::OP_اڻ_استعمال_ٿيل:  //
            اكسر;
        حالة VNTypeInfo::OP_استعمال_ٿيل:  //
            m_os << ' ';
            render(opp, اونهائي);
            اكسر;
        حالة VNTypeInfo::OP_LIST:
            m_os << " [";
            لكل (ثابت Ast_ڳنڍ* nodep = opp; nodep; nodep = nodep->nextp()) {
                إذا (nodep != opp) m_os << ", ";
                render(opp, اونهائي);
            }
            m_os << ']';
            اكسر;
        حالة VNTypeInfo::OP_OPTIONAL:
            إذا (opp) {
                m_os << " ";
                render(opp, اونهائي);
            } وإلا {
                m_os << " nil";
            }
            اكسر;
        }
    }

    // Render the node into the stream.
    فراغ render(ثابت Ast_ڳنڍ* nodep, uint32_t اونهائي) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            إذا (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } وإلا إذا (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } وإلا {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } وإلا إذا (is_پن(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } وإلا إذا (اونهائي == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } وإلا {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            render_عمليو(typeInfo.m_opType[0], nodep->op1p(), اونهائي - 1);
            render_عمليو(typeInfo.m_opType[1], nodep->op2p(), اونهائي - 1);
            render_عمليو(typeInfo.m_opType[2], nodep->op3p(), اونهائي - 1);
            render_عمليو(typeInfo.m_opType[3], nodep->op4p(), اونهائي - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : خالي_اشارو;
        إذا (!dtypep) {
            m_os << '?';
        } وإلا إذا (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } وإلا {
            ثابت صحيح ويڪر = nodep->ويڪر();
            إذا (ويڪر == 1) {
                m_os << '1';
            } وإلا إذا (ويڪر <= VL_QUADSIZE) {
                m_os << intern_لفظ_ويڪر(ويڪر);
            } وإلا {
                m_os << intern_Wide_ويڪر(ويڪر);
            }
            إذا (m_dump_ويڪر_Min) {
                m_os << '/';
                ثابت صحيح ويڪر_Min = nodep->ويڪر_Min();
                إذا (ويڪر_Min == 1) {
                    m_os << '1';
                } وإلا إذا (ويڪر_Min <= VL_QUADSIZE) {
                    m_os << intern_لفظ_ويڪر(ويڪر_Min);
                } وإلا {
                    m_os << intern_Wide_ويڪر(ويڪر_Min);
                }
            }
        }
    }

عام:
    VNPatternString(std::ostream& os, ثابت Ast_ڳنڍ* nodep, uint32_t اونهائي)
        : m_os{os} {
        render(nodep, اونهائي);
    }
};

std::string Ast_ڳنڍ_اظهار::نمونو_String(uint32_t اونهائي) ثابت {
    std::ostringstream oss;
    VNPatternString{oss, هذا, اونهائي};
    أرجع oss.str();
}

//######################################################################
// VNDeleter

فراغ VNDeleter::doDeletes() {
    لكل (Ast_ڳنڍ* ثابت nodep : m_deleteps) nodep->delete_وڻ();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#ضمن "V3Ast__gen_visitor_defns.h"  // From ./astgen
