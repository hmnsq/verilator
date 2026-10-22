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

uint64_t Ast_گرہ::s_edit_Cnt_آخری = 0;
uint64_t Ast_گرہ::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
صحیح Ast_گرہ::s_cloneCntGbl = 0;
uint32_t VNUser1_اندر_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2_اندر_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3_اندر_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4_اندر_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent

منطقی VNUser1_اندر_Use::s_userBusy = جھوٹ;
منطقی VNUser2_اندر_Use::s_userBusy = جھوٹ;
منطقی VNUser3_اندر_Use::s_userBusy = جھوٹ;
منطقی VNUser4_اندر_Use::s_userBusy = جھوٹ;

صحیح Ast_گرہ_DType::s_unique_عدد = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::array_طریقہ(ثابت string& name) {
    برائے (صحیح i = 0; i < _ENUM_MAX; ++i) {
        ثابت VCMethod طریقہ{i};
        اگر (name == طریقہ.ascii()) واپس طریقہ;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    واپس VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ثابت خالی*)) ثابت {
#اگر_تعریف_شدہ VL_USER_TYPE_CHECKS
    اگر (ثابت uint64_t* ثابت uip = std::get_if<uint64_t>(&m_u)) واپس "#"s + cvt_مقصد_Str(*uip);
    اگر (خالی* ثابت* ثابت upp = std::get_if<خالی*>(&m_u)) واپس fmtAddrp(*upp);
    واپس "";
#ورنہ
    // Dumps void* representation
    اگر (!m_u.اوپر) واپس "";
    واپس fmtAddrp(m_u.اوپر);
#اگر_ختم
}

//######################################################################
// VNType

ثابت VNTypeInfo VNType::s_type_Info_جدول[VNType::عدد_TYPES()] = {
#شامل "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& عامل<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

منطقی VFwdType::is_گرہ_Compatible(ثابت Ast_گرہ* nodep) ثابت {
    ثابت Ast_گرہ* defp = nodep;
    اگر (ثابت AstTypedef* ثابت adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    اگر (ثابت Ast_گرہ_DType* ثابت adefp = VN_CAST(defp, گرہ_DType))
        defp = adefp->skip_Ref_مقصد_Non_Refp();
    انتخاب (m_e) {
    حالت VFwdType::NONE: واپس سچ; توڑو;
    حالت VFwdType::ENUM: واپس VN_IS(defp, EnumDType); توڑو;
    حالت VFwdType::STRUCT: واپس VN_IS(defp, StructDType); توڑو;
    حالت VFwdType::UNION: واپس VN_IS(defp, UnionDType); توڑو;
    حالت VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    حالت VFwdType::CLASS: واپس VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); توڑو;
    حالت VFwdType::GENERIC_INTERFACE: واپس VN_IS(defp, IfaceRefDType); توڑو;
    طےشدہ: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    واپس جھوٹ;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ثابت std::shared_ptr<ثابت string> VSelf_اشارہ_گر_متن::s_emptyp = std::make_shared<string>("");
ثابت std::shared_ptr<ثابت string> VSelf_اشارہ_گر_متن::s_thisp = std::make_shared<string>("this");

string VSelf_اشارہ_گر_متن::replaceThis(منطقی useSelfForThis, ثابت string& متن) {
    واپس useSelfForThis ? VString::replace_لفظ(متن, "this", "vlSelf") : متن;
}

string VSelf_اشارہ_گر_متن::protect(منطقی useSelfForThis, منطقی protect) ثابت {
    واپس VIdProtect::protect_الفاظ_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_گرہ::Ast_گرہ(VNType t, File_سطر* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = یہ;  // When made, we're a list of only a single element
    //اوصاف
    m_flags.did_چوڑائی = جھوٹ;
    m_flags.doing_چوڑائی = جھوٹ;
    m_flags.protect = سچ;
    m_flags.غیر_مستعمل = 0;  // Initializing this avoids a read-modify-write on construction
    edit_گنتی_Inc();
}

Ast_گرہ* Ast_گرہ::abovep() ثابت {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || پہلا_Abovep(), یہ, "abovep() not allowed when in midlist");
    ثابت Ast_گرہ* ثابت firstp = پہلا_Abovep() ? یہ : m_headtailp;
    واپس firstp->backp();
}
Ast_گرہ* Ast_گرہ::aboveLoopp() ثابت {
    // Returns parent node. Avoid using this, may have performance issues.
    ثابت Ast_گرہ* nodep = یہ;
    // Backwards over peers (versus parents)
    جب (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    واپس nodep->backp();
}

string Ast_گرہ::encodeName(ثابت string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string برآمد;
    برآمد.reserve(namein.سائز());
    برائے (خودکار pos = namein.begin(); pos != namein.انتہا(); ++pos) {
        اگر ((pos == namein.begin()) ? std::حرف_ہے(pos[0])  // digits can't lead identifiers
                                    : std::حرف_یا_ہندسہ(pos[0])) {
            برآمد += pos[0];
        } ورنہ اگر (pos[0] == '_') {
            برآمد += pos[0];
            اگر (pos + 1 == namein.انتہا()) توڑو;
            اگر (pos[1] == '_') {
                ++pos;
                برآمد += "__05F";  // hex(_) = 0x5F
            }
        } ورنہ {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ثابت غیرعلامتی val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            برآمد += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{برآمد};
    واپس vname.hashedName();
}

string Ast_گرہ::encode_نمبر(int64_t عدد) {
    اگر (عدد < 0) واپس "__02D" + cvt_مقصد_Str(-عدد);  // 2D=-
    واپس cvt_مقصد_Str(عدد);
}

string Ast_گرہ::nameProtect() ثابت { واپس VIdProtect::protectIf(name(), protect()); }
string Ast_گرہ::origNameProtect() ثابت { واپس VIdProtect::protectIf(origName(), protect()); }

string Ast_گرہ::shortName() ثابت {
    string pretty = name();
    string::سائز_type pos;
    جب ((pos = pretty.ڈھونڈو("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    واپس pretty;
}

string Ast_گرہ::dedotName(ثابت string& namein) {
    string pretty = namein;
    string::سائز_type pos;
    جب ((pos = pretty.ڈھونڈو("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    اگر (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    واپس pretty;
}

string Ast_گرہ::nameNoArray(ثابت string& namein) {
    واپس namein.substr(0, namein.ڈھونڈو("__BRA__"));
}

string Ast_گرہ::vcdName(ثابت string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::سائز_type pos;
    جب ((pos = pretty.ڈھونڈو("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    جب ((pos = pretty.ڈھونڈو('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    واپس prettyName(pretty);
}

string Ast_گرہ::prettyName(ثابت string& namein) VL_خالص {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.لمبائی_2());
    برائے (ثابت حرف* pos = namein.c_str(); *pos;) {
        اگر (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            جاری;
        }
        اگر (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            اگر (0 == std::لڑی_موازنہ(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                جاری;
            }
            اگر (0 == std::لڑی_موازنہ(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                جاری;
            }
            اگر (0 == std::لڑی_موازنہ(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                جاری;
            }
            اگر (0 == std::لڑی_موازنہ(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                جاری;
            }
            اگر (0 == std::لڑی_موازنہ(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                جاری;
            }
            اگر (0 == std::لڑی_موازنہ(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                جاری;
            }
            اگر (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                حرف قدر = 0;
                قدر += 16
                         * (std::ہندسہ_ہے(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                قدر
                    += (std::ہندسہ_ہے(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += قدر;
                pos += 5;
                جاری;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    اگر (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    اگر (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    واپس pretty;
}

string Ast_گرہ::vpiName(ثابت string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ثابت سائز_t انحراف = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.لمبائی_2());
    منطقی اندر_Escaped_Ident = جھوٹ;
    صحیح آخری_Ident = 0;

    برائے (ثابت حرف* pos = namein.c_str() + انحراف; *pos;) {
        حرف specialChar = 0;
        اگر (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } ورنہ اگر (pos[0] == '_' && pos[1] == '_') {  // __
            اگر (0 == std::لڑی_موازنہ(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } ورنہ اگر (0 == std::لڑی_موازنہ(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } ورنہ اگر (0 == std::لڑی_موازنہ(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } ورنہ اگر (0 == std::لڑی_موازنہ(pos, "__PVT__", 7)) {
                pos += 7;
                جاری;
            } ورنہ اگر (0 == std::لڑی_موازنہ(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                حرف قدر = 0;
                قدر += 16
                         * (std::ہندسہ_ہے(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                قدر
                    += (std::ہندسہ_ہے(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                اگر (قدر != '_') اندر_Escaped_Ident = سچ;

                pretty += قدر;
                pos += 5;
                جاری;
            }
        } ورنہ اگر (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        اگر (specialChar) {
            اگر (اندر_Escaped_Ident && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.داخل_کرو(آخری_Ident, "\\");
                اندر_Escaped_Ident = جھوٹ;
            }

            pretty += specialChar;

            اگر (specialChar == ']' || specialChar == '.') {
                آخری_Ident = pretty.لمبائی_2();
                اندر_Escaped_Ident = جھوٹ;
            }
        } ورنہ {
            pretty += pos[0];
            ++pos;
        }
    }
    اگر (اندر_Escaped_Ident) {
        pretty += " ";
        pretty.داخل_کرو(آخری_Ident, "\\");
    }
    اگر (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    اگر (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    واپس pretty;
}

string Ast_گرہ::prettyTypeName() ثابت {
    اگر (name() == "") واپس typeName();
    واپس std::string{typeName()} + " '" + pretty_Name_پیغام() + "'";
}

//######################################################################
// Insertion

خالی Ast_گرہ::debug_درخت_Change(ثابت Ast_گرہ* nodep, ثابت حرف* سابقہ, صحیح lineno, منطقی اگلا) {
#اگر_تعریف_شدہ VL_DEBUG
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
#اگر_ختم
}

سانچہ <>
Ast_گرہ* Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(Ast_گرہ* nodep, Ast_گرہ* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_درخت_Change(nodep, "-addNextThs: ", __LINE__, جھوٹ);
    debug_درخت_Change(newp, "-addNextNew: ", __LINE__, سچ);
    اگر (!nodep)  // verilog.y and lots of other places assume this
        واپس newp;

    // Find end of old list
    Ast_گرہ* oldtailp = nodep;
    اگر (oldtailp->m_nextp) {
        اگر (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } ورنہ {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            جب (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_گرہ* ثابت newtailp = newp->m_headtailp;
    Ast_گرہ* ثابت headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = خالی_اشاریہ;  // May be written again as new head
    newp->m_headtailp = خالی_اشاریہ;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_گنتی_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_درخت_Change(nodep, "-addNextOut:", __LINE__, سچ);
    واپس nodep;
}

خالی Ast_گرہ::جوڑو_اگلا_یہاں(Ast_گرہ* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_درخت_Change(یہ, "-addHereThs: ", __LINE__, جھوٹ);
    debug_درخت_Change(newp, "-addHereNew: ", __LINE__, سچ);
    newp->edit_گنتی_Inc();

    Ast_گرہ* ثابت addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_گرہ* ثابت oldnextp = یہ->m_nextp;
    یہ->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    اگر (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = یہ;

    // Head/tail
    Ast_گرہ* ثابت oldheadtailp = یہ->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = خالی_اشاریہ;  // Not at head any longer
    addlastp->m_headtailp = خالی_اشاریہ;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    اگر (oldheadtailp) {  // else in middle of list, no change
        اگر (oldheadtailp == یہ) {  // this was one node
            یہ->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } ورنہ اگر (!oldnextp) {  // this was tail
            یہ->m_headtailp = خالی_اشاریہ;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_درخت_Change(یہ, "-addHereOut: ", __LINE__, سچ);
}

خالی Ast_گرہ::سیٹ_Op1p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, یہ, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_درخت_Change(یہ, "-setOp1pThs: ", __LINE__, جھوٹ);
    debug_درخت_Change(newp, "-setOp1pNew: ", __LINE__, سچ);
    m_op1p = newp;
    newp->edit_گنتی_Inc();
    newp->m_backp = یہ;
    debug_درخت_Change(یہ, "-setOp1pOut: ", __LINE__, جھوٹ);
}

خالی Ast_گرہ::سیٹ_Op2p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, یہ, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_درخت_Change(یہ, "-setOp2pThs: ", __LINE__, جھوٹ);
    debug_درخت_Change(newp, "-setOp2pNew: ", __LINE__, سچ);
    m_op2p = newp;
    newp->edit_گنتی_Inc();
    newp->m_backp = یہ;
    debug_درخت_Change(یہ, "-setOp2pOut: ", __LINE__, جھوٹ);
}

خالی Ast_گرہ::سیٹ_Op3p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, یہ, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_درخت_Change(یہ, "-setOp3pThs: ", __LINE__, جھوٹ);
    debug_درخت_Change(newp, "-setOp3pNew: ", __LINE__, سچ);
    m_op3p = newp;
    newp->edit_گنتی_Inc();
    newp->m_backp = یہ;
    debug_درخت_Change(یہ, "-setOp3pOut: ", __LINE__, جھوٹ);
}

خالی Ast_گرہ::سیٹ_Op4p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, یہ, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_درخت_Change(یہ, "-setOp4pThs: ", __LINE__, جھوٹ);
    debug_درخت_Change(newp, "-setOp4pNew: ", __LINE__, سچ);
    m_op4p = newp;
    newp->edit_گنتی_Inc();
    newp->m_backp = یہ;
    debug_درخت_Change(یہ, "-setOp4pOut: ", __LINE__, جھوٹ);
}

خالی Ast_گرہ::جوڑو_Op1p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op1p) {
        op1p(newp);
    } ورنہ {
        m_op1p->جوڑو_اگلا(newp);
    }
}

خالی Ast_گرہ::جوڑو_Op2p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op2p) {
        op2p(newp);
    } ورنہ {
        m_op2p->جوڑو_اگلا(newp);
    }
}

خالی Ast_گرہ::جوڑو_Op3p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op3p) {
        op3p(newp);
    } ورنہ {
        m_op3p->جوڑو_اگلا(newp);
    }
}

خالی Ast_گرہ::جوڑو_Op4p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op4p) {
        op4p(newp);
    } ورنہ {
        m_op4p->جوڑو_اگلا(newp);
    }
}

خالی Ast_گرہ::prependOp1p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op1p) {
        op1p(newp);
    } ورنہ {
        m_op1p->جوڑو_یہاں_This_As_اگلا(newp);
    }
}

خالی Ast_گرہ::prependOp2p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op2p) {
        op2p(newp);
    } ورنہ {
        m_op2p->جوڑو_یہاں_This_As_اگلا(newp);
    }
}

خالی Ast_گرہ::prependOp3p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op3p) {
        op3p(newp);
    } ورنہ {
        m_op3p->جوڑو_یہاں_This_As_اگلا(newp);
    }
}

خالی Ast_گرہ::prependOp4p(Ast_گرہ* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    اگر (!m_op4p) {
        op4p(newp);
    } ورنہ {
        m_op4p->جوڑو_یہاں_This_As_اگلا(newp);
    }
}

خالی Ast_گرہ::replaceWith(Ast_گرہ* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    یہ->unlink_Fr_عقب(&repHandle);
    repHandle.relink(newp);
}
خالی Ast_گرہ::replaceWithKeepDType(Ast_گرہ* newp) {
    newp->dtypeFrom(یہ);
    replaceWith(newp);
}

خالی VNRelinker::dump(std::ostream& str) ثابت {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_اگلا ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_گرہ* Ast_گرہ::unlink_Fr_عقب_With_اگلا(VNRelinker* linkerp) {
    debug_درخت_Change(یہ, "-unlinkWNextThs: ", __LINE__, سچ);
    Ast_گرہ* ثابت oldp = یہ;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_گنتی_Inc();
    // cppcheck-suppress shadowFunction
    Ast_گرہ* ثابت backp = oldp->m_backp;
    اگر (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        اگر (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_اگلا;
        } ورنہ اگر (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ورنہ اگر (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ورنہ اگر (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ورنہ اگر (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ورنہ {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    اگر (backp->m_nextp == oldp) {
        backp->m_nextp = خالی_اشاریہ;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_گرہ* oldtailp = backp->m_headtailp;
        اگر (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            جب (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_گرہ* ثابت oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } ورنہ اگر (backp->m_op1p == oldp) {
        backp->m_op1p = خالی_اشاریہ;
    } ورنہ اگر (backp->m_op2p == oldp) {
        backp->m_op2p = خالی_اشاریہ;
    } ورنہ اگر (backp->m_op3p == oldp) {
        backp->m_op3p = خالی_اشاریہ;
    } ورنہ اگر (backp->m_op4p == oldp) {
        backp->m_op4p = خالی_اشاریہ;
    } ورنہ {
        یہ->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = خالی_اشاریہ;
    // Iterator fixup
    اگر (oldp->m_iterpp) {
        *(oldp->m_iterpp) = خالی_اشاریہ;
        oldp->m_iterpp = خالی_اشاریہ;
    }
    debug_درخت_Change(oldp, "-unlinkWNextOut: ", __LINE__, سچ);
    واپس oldp;
}

Ast_گرہ* Ast_گرہ::unlink_Fr_عقب(VNRelinker* linkerp) {
    debug_درخت_Change(یہ, "-unlinkFrBkThs: ", __LINE__, سچ);
    Ast_گرہ* ثابت oldp = یہ;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_گنتی_Inc();
    // cppcheck-suppress shadowFunction
    Ast_گرہ* ثابت backp = oldp->m_backp;
    اگر (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        اگر (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = خالی_اشاریہ;
            oldp->m_iterpp = خالی_اشاریہ;
        }
        اگر (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_اگلا;
        } ورنہ اگر (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ورنہ اگر (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ورنہ اگر (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ورنہ اگر (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ورنہ {
            یہ->v3fatalSrc("Unlink of node with back not pointing to it.");
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
    } ورنہ {
        اگر (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } ورنہ اگر (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } ورنہ اگر (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } ورنہ اگر (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } ورنہ {
            یہ->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        اگر (oldp->m_nextp) {
            Ast_گرہ* ثابت newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    اگر (oldp->m_iterpp) {  // Only if no linker, point to next in list
        اگر (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = خالی_اشاریہ;
    }
    // Relink
    oldp->m_nextp = خالی_اشاریہ;
    oldp->m_backp = خالی_اشاریہ;
    oldp->m_headtailp = oldp;
    debug_درخت_Change(oldp, "-unlinkFrBkOut: ", __LINE__, سچ);
    واپس oldp;
}

خالی Ast_گرہ::relink(VNRelinker* linkerp) {
    اگر (debug() > 8) {
        UINFO_سابقہ(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_گرہ* ثابت newp = یہ;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_گنتی_Inc();

    اگر (debug() > 8) {
        linkerp->dump(خارج);
        خارج << سطر_ختم;
    }

    // cppcheck-suppress shadowFunction
    Ast_گرہ* ثابت backp = linkerp->m_backp;
    debug_درخت_Change(یہ, "-relinkNew: ", __LINE__, سچ);
    debug_درخت_Change(backp, "-relinkTre: ", __LINE__, سچ);

    انتخاب (linkerp->m_chg) {
    حالت VNRelinker::RELINK_اگلا: backp->جوڑو_اگلا_یہاں(newp); توڑو;
    حالت VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); توڑو;
    حالت VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); توڑو;
    حالت VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); توڑو;
    حالت VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); توڑو;
    طےشدہ: یہ->v3fatalSrc("Relink of node without any link to change."); توڑو;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = خالی_اشاریہ;
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
    linkerp->m_backp = خالی_اشاریہ;
    debug_درخت_Change(یہ, "-relinkOut: ", __LINE__, سچ);
}

خالی Ast_گرہ::relinkOneLink(Ast_گرہ*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_گرہ* newp) {
    اگر (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_گرہ* ثابت newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_گرہ* ثابت oldlistlastp = pointpr->m_headtailp;
        UASSERT_OBJ(!(oldlistlastp->m_nextp && oldlistlastp != pointpr), newp,
                    "Old headtailp tail isn't at the tail");
        // Next links
        newlistlastp->m_nextp = pointpr;
        pointpr->m_backp = newlistlastp;
        // Head/tail
        pointpr->m_headtailp = خالی_اشاریہ;  // Old head
        newlistlastp->m_headtailp = خالی_اشاریہ;  // Old tail
        newp->m_headtailp = oldlistlastp;  // Head points to tail
        oldlistlastp->m_headtailp = newp;  // Tail points to head
    }
    pointpr = newp;
}

خالی Ast_گرہ::جوڑو_یہاں_This_As_اگلا(Ast_گرہ* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(یہ->m_backp, یہ, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_گرہ* ثابت backp = یہ->m_backp;
    Ast_گرہ* ثابت newLastp = newp->m_headtailp;
    //
    یہ->edit_گنتی_Inc();
    // Common linkage
    newLastp->m_nextp = یہ;
    یہ->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = خالی_اشاریہ;
    // Linkage dependent on position
    اگر (backp && backp->m_nextp == یہ) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = خالی_اشاریہ;
    } ورنہ {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        اگر (backp) {
            اگر (backp->m_op1p == یہ) {
                backp->m_op1p = newp;
            } ورنہ اگر (backp->m_op2p == یہ) {
                backp->m_op2p = newp;
            } ورنہ اگر (backp->m_op3p == یہ) {
                backp->m_op3p = newp;
            } ورنہ {
                UASSERT_OBJ(backp->m_op4p == یہ, یہ, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_گرہ* ثابت tailp = یہ->m_headtailp;
        یہ->m_headtailp = خالی_اشاریہ;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    اگر (newLastp->m_iterpp) *(newLastp->m_iterpp) = یہ;
    اگر (یہ->m_iterpp) {
        *(یہ->m_iterpp) = newp;
        یہ->m_iterpp = خالی_اشاریہ;
    }
    //
    debug_درخت_Change(یہ, "-addHereThisAsNext: ", __LINE__, سچ);
}

//======================================================================
// Clone

Ast_گرہ* Ast_گرہ::clone_درخت_Iter(منطقی need_خالص) {
    // private: Clone single node and children
    اگر (need_خالص) purityCheck();
    Ast_گرہ* ثابت newp = یہ->clone();
    اگر (یہ->m_op1p) newp->op1p(یہ->m_op1p->clone_درخت_Iter_List(need_خالص));
    اگر (یہ->m_op2p) newp->op2p(یہ->m_op2p->clone_درخت_Iter_List(need_خالص));
    اگر (یہ->m_op3p) newp->op3p(یہ->m_op3p->clone_درخت_Iter_List(need_خالص));
    اگر (یہ->m_op4p) newp->op4p(یہ->m_op4p->clone_درخت_Iter_List(need_خالص));
    newp->m_iterpp = خالی_اشاریہ;
    newp->clonep(یہ);  // Save pointers to/from both to simplify relinking.
    یہ->clonep(newp);  // Save pointers to/from both to simplify relinking.
    واپس newp;
}

Ast_گرہ* Ast_گرہ::clone_درخت_Iter_List(منطقی need_خالص) {
    // private: Clone list of nodes, set m_headtailp
    Ast_گرہ* newheadp = خالی_اشاریہ;
    Ast_گرہ* newtailp = خالی_اشاریہ;
    // Audited to make sure this is never nullptr
    برائے (Ast_گرہ* oldp = یہ; oldp; oldp = oldp->m_nextp) {
        Ast_گرہ* ثابت newp = oldp->clone_درخت_Iter(need_خالص);
        newp->m_headtailp = خالی_اشاریہ;
        newp->m_backp = newtailp;
        اگر (newtailp) newtailp->m_nextp = newp;
        اگر (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    واپس newheadp;
}

Ast_گرہ* Ast_گرہ::clone_درخت(منطقی clone_اگلا_Link, منطقی need_خالص) {
    debug_درخت_Change(یہ, "-cloneThs: ", __LINE__, clone_اگلا_Link);
    clone_Clear_درخت();
    Ast_گرہ* newp;
    اگر (clone_اگلا_Link && یہ->m_nextp) {
        newp = clone_درخت_Iter_List(need_خالص);
    } ورنہ {
        newp = clone_درخت_Iter(need_خالص);
        newp->m_nextp = خالی_اشاریہ;
        newp->m_headtailp = newp;
    }
    newp->m_backp = خالی_اشاریہ;
    newp->clone_Relink_درخت();
    debug_درخت_Change(newp, "-cloneOut: ", __LINE__, سچ);
    واپس newp;
}

خالی Ast_گرہ::purityCheck() {
    اگر (VL_UNLIKELY(!is_خالص())) {
        یہ->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << یہ->warn_مزید()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

خالی Ast_گرہ::delete_گرہ() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_گنتی_Inc();
    // Change links of old node so we coredump if used
    یہ->m_nextp = reinterpret_cast<Ast_گرہ*>(0x1);
    یہ->m_backp = reinterpret_cast<Ast_گرہ*>(0x1);
    یہ->m_headtailp = reinterpret_cast<Ast_گرہ*>(0x1);
    یہ->m_op1p = reinterpret_cast<Ast_گرہ*>(0x1);
    یہ->m_op2p = reinterpret_cast<Ast_گرہ*>(0x1);
    یہ->m_op3p = reinterpret_cast<Ast_گرہ*>(0x1);
    یہ->m_op4p = reinterpret_cast<Ast_گرہ*>(0x1);
    یہ->m_iterpp = reinterpret_cast<Ast_گرہ**>(0x1);
    اگر (
#اگر !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        سچ
#ورنہ
        !v3Global.opt.debugLeak()
#اگر_ختم
    ) {
        حذف یہ;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

خالی Ast_گرہ::delete_درخت_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    برائے (Ast_گرہ *nodep = یہ, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        اگر (nodep->m_op1p) nodep->m_op1p->delete_درخت_Iter();
        اگر (nodep->m_op2p) nodep->m_op2p->delete_درخت_Iter();
        اگر (nodep->m_op3p) nodep->m_op3p->delete_درخت_Iter();
        اگر (nodep->m_op4p) nodep->m_op4p->delete_درخت_Iter();
        nodep->m_nextp = خالی_اشاریہ;
        nodep->m_backp = خالی_اشاریہ;
        nodep->delete_گرہ();
    }
}

خالی Ast_گرہ::delete_درخت() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_درخت_Change(یہ, "-delTree:  ", __LINE__, سچ);
    یہ->edit_گنتی_Inc();
    // MUST be depth first!
    delete_درخت_Iter();
}

//======================================================================
// Memory checks

#اگر_تعریف_شدہ VL_LEAK_CHECKS
خالی* Ast_گرہ::عامل نیا(سائز_t سائز) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_گرہ* ثابت objp = static_cast<Ast_گرہ*>(::عامل نیا(سائز));
    V3Broken::جوڑو_Newed(objp);
    واپس objp;
}

خالی Ast_گرہ::عامل حذف(خالی* objp, سائز_t سائز) {
    اگر (!objp) واپس;
    ثابت Ast_گرہ* ثابت nodep = static_cast<Ast_گرہ*>(objp);
    V3Broken::deleted(nodep);
    ::عامل حذف(objp);
}
#اگر_ختم

#اگر_تعریف_شدہ VL_ALLOC_RANDOM_CHECKS
خالی* Ast_گرہ::عامل نیا(سائز_t سائز) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ساکن ثابت سائز_t ASTGEN_MAX_گرہ_سائز = []() {
        سائز_t max_سائز = 0;
        برائے (سائز_t t = 0; t < VNType::عدد_TYPES(); ++t) {
            max_سائز = std::max(max_سائز, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        واپس max_سائز;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr سائز_t POOL_سائز = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ساکن uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    اگر (صحیح بیج = v3Global.opt.debugAllocRandom()) {
        ساکن V3Mutex s_mutex;
        ثابت V3_تالا_Guard تالا_2{s_mutex};
        ساکن std::array<خالی*, POOL_سائز> s_گرہ_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(سائز <= ASTGEN_MAX_گرہ_سائز, "fix ASTGEN_MAX_NODE_SIZE");
        اگر (!s_lfsr) {
            s_lfsr = بیج;
            اگر (!s_lfsr) s_lfsr = ~s_lfsr;
            برائے (سائز_t i = 0; i < POOL_سائز; ++i) {
                s_گرہ_Pool[i] = ::عامل نیا(ASTGEN_MAX_گرہ_سائز + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::ترتیب_دو(std::begin(s_گرہ_Pool), std::انتہا(s_گرہ_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ثابت سائز_t اشاریہ = s_lfsr % POOL_سائز;
        Ast_گرہ* ثابت objp = static_cast<Ast_گرہ*>(s_گرہ_Pool[اشاریہ]);
        s_گرہ_Pool[اشاریہ] = ::عامل نیا(ASTGEN_MAX_گرہ_سائز + 64);  // For later new()
        واپس objp;
    } ورنہ {
        Ast_گرہ* ثابت objp = static_cast<Ast_گرہ*>(::عامل نیا(سائز));
        واپس objp;
    }
}
خالی Ast_گرہ::عامل حذف(خالی* objp, سائز_t سائز) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (خالی)objp;
    (خالی)سائز;
}
#اگر_ختم

//======================================================================
// Iterators

خالی Ast_گرہ::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    اگر (m_op1p) m_op1p->iterate_And_اگلا(v);
    اگر (m_op2p) m_op2p->iterate_And_اگلا(v);
    اگر (m_op3p) m_op3p->iterate_And_اگلا(v);
    اگر (m_op4p) m_op4p->iterate_And_اگلا(v);
}

خالی Ast_گرہ::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    اگر (m_op1p) m_op1p->iterate_And_اگلا_Const(v);
    اگر (m_op2p) m_op2p->iterate_And_اگلا_Const(v);
    اگر (m_op3p) m_op3p->iterate_And_اگلا_Const(v);
    اگر (m_op4p) m_op4p->iterate_And_اگلا_Const(v);
}

خالی Ast_گرہ::iterate_And_اگلا(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_گرہ* nodep = یہ;
#اگر_تعریف_شدہ VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#اگر_ختم
    // cppcheck-suppress knownConditionTrueFalse
    اگر (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    جب (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        اگر (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_گرہ* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        اگر (!niterp) واپس;  // Perhaps node deleted inside accept
        niterp->m_iterpp = خالی_اشاریہ;
        اگر (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } ورنہ {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

خالی Ast_گرہ::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_گرہ* nodep = یہ;
    جب (nodep->m_nextp) nodep = nodep->m_nextp;
    جب (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        اگر (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } ورنہ {
            nodep = خالی_اشاریہ;
        }  // else: backp points up the tree.
    }
}

خالی Ast_گرہ::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    اگر (m_op1p) m_op1p->iterateListBackwardsConst(v);
    اگر (m_op2p) m_op2p->iterateListBackwardsConst(v);
    اگر (m_op3p) m_op3p->iterateListBackwardsConst(v);
    اگر (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

خالی Ast_گرہ::iterate_And_اگلا_Const(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_گرہ* nodep = یہ;
    کرو {
        Ast_گرہ* ثابت nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } جب (nodep);
}

Ast_گرہ* Ast_گرہ::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_گرہ* nodep = یہ;  // Note "this" may point to bogus point later in this function
    اگر (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } ورنہ اگر (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ثابت tempp = نیا AstBegin{nodep->fileline(), "[EditWrapper]", nodep, جھوٹ};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlink_Fr_عقب_With_اگلا();
        VL_DO_DANGLING(tempp->delete_درخت(), tempp);
    } ورنہ {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_گرہ** nextnodepp = خالی_اشاریہ;
        اگر (یہ->m_backp->m_op1p == یہ) {
            nextnodepp = &(یہ->m_backp->m_op1p);
        } ورنہ اگر (یہ->m_backp->m_op2p == یہ) {
            nextnodepp = &(یہ->m_backp->m_op2p);
        } ورنہ اگر (یہ->m_backp->m_op3p == یہ) {
            nextnodepp = &(یہ->m_backp->m_op3p);
        } ورنہ اگر (یہ->m_backp->m_op4p == یہ) {
            nextnodepp = &(یہ->m_backp->m_op4p);
        } ورنہ اگر (یہ->m_backp->m_nextp == یہ) {
            nextnodepp = &(یہ->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, یہ, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    واپس nodep;
}

//======================================================================

خالی Ast_گرہ::clone_Relink_درخت() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    برائے (Ast_گرہ* nodep = یہ; nodep; nodep = nodep->m_nextp) {
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

منطقی Ast_گرہ::gate_درخت_Iter() ثابت {
    // private: Return true if the two trees are identical
    اگر (!isGateOptimizable()) واپس جھوٹ;
    اگر (m_op1p && !m_op1p->gate_درخت_Iter()) واپس جھوٹ;
    اگر (m_op2p && !m_op2p->gate_درخت_Iter()) واپس جھوٹ;
    اگر (m_op3p && !m_op3p->gate_درخت_Iter()) واپس جھوٹ;
    اگر (m_op4p && !m_op4p->gate_درخت_Iter()) واپس جھوٹ;
    واپس سچ;
}

منطقی Ast_گرہ::same_درخت_Iter(ثابت Ast_گرہ* node1p, ثابت Ast_گرہ* node2p, منطقی ign_اگلا,
                           منطقی gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    جب (سچ) {
        اگر (!node1p && !node2p) واپس سچ;
        اگر (!node1p || !node2p) واپس جھوٹ;
        اگر (node1p->type() != node2p->type()) واپس جھوٹ;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        اگر (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) واپس جھوٹ;
        اگر (!node1p->same_گرہ(node2p) || (gateOnly && !node1p->isGateOptimizable())) واپس جھوٹ;
        اگر (!same_درخت_Iter(node1p->m_op1p, node2p->m_op1p, جھوٹ, gateOnly)
            || !same_درخت_Iter(node1p->m_op2p, node2p->m_op2p, جھوٹ, gateOnly)
            || !same_درخت_Iter(node1p->m_op3p, node2p->m_op3p, جھوٹ, gateOnly)
            || !same_درخت_Iter(node1p->m_op4p, node2p->m_op4p, جھوٹ, gateOnly)) {
            واپس جھوٹ;
        }
        اگر (ign_اگلا) واپس سچ;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

خالی Ast_گرہ::check_درخت_Iter(ثابت Ast_گرہ* پچھلا_Backp) ثابت VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(پچھلا_Backp == یہ->backp(), یہ, "Back node inconsistent");
    ثابت VNTypeInfo& typeInfo = VNType::typeInfo(یہ->type());
    برائے (صحیح i = 1; i <= 4; i++) {
        Ast_گرہ* nodep = خالی_اشاریہ;
        انتخاب (i) {
        حالت 1: nodep = op1p(); توڑو;
        حالت 2: nodep = op2p(); توڑو;
        حالت 3: nodep = op3p(); توڑو;
        حالت 4: nodep = op4p(); توڑو;
        طےشدہ: یہ->v3fatalSrc("Bad case"); توڑو;
        }
        // cppcheck-suppress danglingTempReference
        ثابت حرف* opName = typeInfo.m_opNamep[i - 1];
        انتخاب (typeInfo.m_opType[i - 1]) {
        حالت VNTypeInfo::OP_غیر_مستعمل:
            UASSERT_OBJ(!nodep, یہ, typeInfo.m_namep << " must not use " << opName << "()");
            توڑو;
        حالت VNTypeInfo::OP_مستعمل:
            UASSERT_OBJ(nodep, یہ,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), یہ,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_درخت_Iter(یہ);
            توڑو;
        حالت VNTypeInfo::OP_LIST:
            اگر (ثابت Ast_گرہ* ثابت headp = nodep) {
                // cppcheck-suppress shadowFunction
                ثابت Ast_گرہ* backp = یہ;
                ثابت Ast_گرہ* tailp;
                ثابت Ast_گرہ* opp = headp;
                کرو {
                    opp->check_درخت_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } جب (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            توڑو;
        حالت VNTypeInfo::OP_OPTIONAL:
            اگر (nodep) {
                UASSERT_OBJ(!nodep->nextp(), یہ,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_درخت_Iter(یہ);
            }
            توڑو;
        طےشدہ: یہ->v3fatalSrc("Bad case"); توڑو;
        }
    }
    اگر (v3Global.opt.debug_چوڑائی() && v3Global.چوڑائی_Min_Usage() == VWidthMinUsage::VERILOG_چوڑائی) {
        اگر (ثابت Ast_گرہ_اظہاریہ* ثابت exprp = VN_CAST(یہ, گرہ_اظہاریہ)) {
            ثابت حرف* ثابت whyp = exprp->چوڑائی_Mismatch();
            اگر (whyp) {
                خودکار dtypeStr = [](ثابت Ast_گرہ_اظہاریہ* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    واپس ss.str();
                };
                اگر (ثابت Ast_گرہ_Uniop* ثابت uniopp = VN_CAST(exprp, گرہ_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } ورنہ اگر (ثابت Ast_گرہ_Biop* ثابت biopp = VN_CAST(exprp, گرہ_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } ورنہ {
                    UASSERT_OBJ(جھوٹ, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
حرف* Ast_گرہ::dump_درخت_Json_Gdb(ثابت Ast_گرہ* nodep) {
    اگر (!nodep) واپس strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodep_دھارا;
    nodep->dump_درخت_Json(nodep_دھارا);
    ثابت std::string str = nodep_دھارا.rdbuf()->str();
    واپس strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
حرف* Ast_گرہ::dump_درخت_Json_Gdb(ثابت حرف* str) { واپس strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
حرف* Ast_گرہ::dump_درخت_Json_Gdb(intptr_t nodep) {
    اگر (!nodep) واپس strdup("{\"addr\":\"NULL\"}\n");
    واپس dump_درخت_Json_Gdb(reinterpret_cast<ثابت Ast_گرہ*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
خالی Ast_گرہ::dumpGdb(ثابت Ast_گرہ* nodep) {  // For GDB only  // LCOV_EXCL_START
    اگر (!nodep) {
        خارج << "<nullptr>" << سطر_ختم;
        واپس;
    }
    nodep->dump_Gdb_سرخی();
    خارج << "  ";
    nodep->dump(خارج);
    خارج << سطر_ختم;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
خالی Ast_گرہ::dump_Gdb_سرخی() ثابت {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(خارج);
    خارج << "  Fileline = " << fileline() << سطر_ختم;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
خالی Ast_گرہ::dump_درخت_Gdb(ثابت Ast_گرہ* nodep) {  // For GDB only  // LCOV_EXCL_START
    اگر (!nodep) {
        خارج << "<nullptr>" << سطر_ختم;
        واپس;
    }
    nodep->dump_Gdb_سرخی();
    nodep->dump_درخت(خارج);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
خالی Ast_گرہ::dump_درخت_File_Gdb(ثابت Ast_گرہ* nodep,  // LCOV_EXCL_START
                              ثابت حرف* filenamep) {  // For GDB only
    اگر (!nodep) {
        خارج << "<nullptr>" << سطر_ختم;
        واپس;
    }
    ثابت string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_درخت_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
خالی Ast_گرہ::checkIter() ثابت {
    اگر (m_iterpp) {
        dumpPtrs(خارج);
        // Perhaps something forgot to clear m_iterpp?
        یہ->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

خالی Ast_گرہ::dumpPtrs(std::ostream& os) ثابت {
    os << "This=" << typeName() << " " << cvt_مقصد_Hex(یہ);
    os << " back=" << cvt_مقصد_Hex(backp());
    اگر (nextp()) os << " next=" << cvt_مقصد_Hex(nextp());
    اگر (m_headtailp == یہ) {
        os << " headtail=this";
    } ورنہ {
        os << " headtail=" << cvt_مقصد_Hex(m_headtailp);
    }
    اگر (op1p()) os << " op1p=" << cvt_مقصد_Hex(op1p());
    اگر (op2p()) os << " op2p=" << cvt_مقصد_Hex(op2p());
    اگر (op3p()) os << " op3p=" << cvt_مقصد_Hex(op3p());
    اگر (op4p()) os << " op4p=" << cvt_مقصد_Hex(op4p());
    ثابت خودکار dumpUser = [&os](ثابت حرف* سابقہ, ثابت VNUser& user) {
        ثابت std::string s
            = user.dumpStr([](ثابت خالی* p) -> std::string { واپس cvt_مقصد_Hex(p); });
        اگر (!s.خالی_خانے()) os << سابقہ << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    اگر (m_iterpp) {
        os << " iterpp=" << cvt_مقصد_Hex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

خالی Ast_گرہ::dump_درخت(std::ostream& os, ثابت string& indent, صحیح max_گہرائی) ثابت {
    ساکن صحیح s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << یہ << '\n';
    اگر (VN_DELETED(یہ)) واپس;
    اگر (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    اگر (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvt_مقصد_Hex(یہ)
           << سطر_ختم;  // endl intentional to do flush
        واپس;
    }  // LCOV_EXCL_STOP
    اگر (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    اگر (max_گہرائی == 1) {
        اگر (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } ورنہ {
        برائے (ثابت Ast_گرہ* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت(os, indent + "1:", max_گہرائی - 1);
        }
        برائے (ثابت Ast_گرہ* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت(os, indent + "2:", max_گہرائی - 1);
        }
        برائے (ثابت Ast_گرہ* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت(os, indent + "3:", max_گہرائی - 1);
        }
        برائے (ثابت Ast_گرہ* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت(os, indent + "4:", max_گہرائی - 1);
        }
    }
}

خالی Ast_گرہ::dump_درخت_And_اگلا(std::ostream& os, ثابت string& indent, صحیح max_گہرائی) ثابت {
    // Audited to make sure this is never nullptr
    برائے (ثابت Ast_گرہ* nodep = یہ; nodep; nodep = nodep->nextp()) {
        nodep->dump_درخت(os, indent, max_گہرائی);
    }
}

خالی Ast_گرہ::dump_درخت_File(ثابت string& filename, منطقی doDump) {
    // Not const function as calls checkTree
    اگر (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ثابت std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            اگر (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_گنتی_آخری();
            *logsp << "> to <e" << std::dec << edit_گنتی_Gbl() << ">\n";
            اگر (edit_گنتی_Gbl() == edit_گنتی_آخری() && ::dump_درخت_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } ورنہ {
                dump_درخت(*logsp);
                edit_گنتی_سیٹ_آخری();  // Next dump can indicate start from here
            }
        }
    }
}

ساکن خالی drawChildren(std::ostream& os, ثابت Ast_گرہ* thisp, ثابت Ast_گرہ* childp,
                         ثابت std::string& بچہ_Name) {
    اگر (childp) {
        os << "\tn" << cvt_مقصد_Hex(thisp) << " -> n" << cvt_مقصد_Hex(childp) << " [" << "label=\""
           << بچہ_Name << "\" color=red];\n";
        برائے (ثابت Ast_گرہ* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_درخت_Dot(os);
            اگر (nodep->nextp()) {
                os << "\tn" << cvt_مقصد_Hex(nodep) << " -> n" << cvt_مقصد_Hex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvt_مقصد_Hex(nodep) << ", n" << cvt_مقصد_Hex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

خالی Ast_گرہ::dump_درخت_Dot(std::ostream& os) ثابت {
    os << "\tn" << cvt_مقصد_Hex(یہ) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, یہ, m_op1p, "op1");
    drawChildren(os, یہ, m_op2p, "op2");
    drawChildren(os, یہ, m_op3p, "op3");
    drawChildren(os, یہ, m_op4p, "op4");
}

خالی Ast_گرہ::dump_درخت_Json_File(ثابت string& filename, منطقی doDump) {
    اگر (!doDump) واپس;
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    اگر (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_درخت_Json(*treejsonp);
    *treejsonp << '\n';
}

خالی Ast_گرہ::dumpJsonMetaFileGdb(ثابت حرف* filename) { dumpJsonMetaFile(filename); }
خالی Ast_گرہ::dumpJsonMetaFile(ثابت string& filename) {
    UINFO(2, "Dumping " << filename);
    ثابت std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    اگر (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_سطر::file_Name_عدد_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptr_نام_Dump_Json(*treejsonp);
    *treejsonp << "}\n";
}

خالی Ast_گرہ::dump_درخت_Dot_File(ثابت string& filename, منطقی doDump) {
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

string Ast_گرہ::instanceStr() ثابت {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr غیرعلامتی maxIterations = 10000;
    غیرعلامتی iter_گنتی = 0;
    // Walk 'backp' chain
    برائے (ثابت Ast_گرہ* currp = یہ; currp; currp = currp->backp(), ++iter_گنتی) {
        اگر (VL_UNCOVERABLE(iter_گنتی >= maxIterations)) واپس "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        اگر (ثابت AstScope* ثابت scopep = VN_CAST(currp, Scope)) {
            واپس scopep->is_چوٹی() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        اگر (ثابت AstModule* ثابت modp = VN_CAST(currp, Module)) {
            ثابت string instanceName = modp->someInstanceName();
            واپس instanceName.خالی_خانے() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    واپس "";
}
خالی Ast_گرہ::v3error_انتہا(ثابت std::ostringstream& str) ثابت VL_RELEASE(V3Error::s().m_mutex) {
    اگر (!m_fileline) {
        V3Error::v3error_انتہا(str, "", خالی_اشاریہ);
    } ورنہ {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ثابت string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().error_رمز()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        اگر (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_گرہ*>(یہ)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3error_انتہا(nsstr, instanceStrExtra);
    }
}
خالی Ast_گرہ::v3error_انتہا_Fatal(ثابت std::ostringstream& str) ثابت
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_انتہا(str);
    أكد(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

خالی Ast_گرہ::dtypeChgSigned(منطقی flag) {
    UASSERT_OBJ(dtypep(), یہ, "No dtype when changing to (un)signed");
    dtype_Chg_چوڑائی_Signed(dtypep()->چوڑائی(), dtypep()->چوڑائی_Min(), VSigning::fromBool(flag));
}
خالی Ast_گرہ::dtype_Chg_چوڑائی(صحیح چوڑائی, صحیح چوڑائی_Min) {
    UASSERT_OBJ(dtypep(), یہ, "No dtype when changing width");
    dtype_Chg_چوڑائی_Signed(چوڑائی, چوڑائی_Min, dtypep()->numeric());
}

خالی Ast_گرہ::dtype_Chg_چوڑائی_Signed(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) {
    UASSERT_OBJ(dtypep(), یہ, "No dtype when changing width");
    ثابت Ast_گرہ_DType* ثابت enumDTypep = dtypep()->skip_Ref_مقصد_Enump();
    اگر (چوڑائی == dtypep()->چوڑائی() && چوڑائی_Min == dtypep()->چوڑائی_Min()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        واپس;  // Correct already
    }
    ثابت Ast_گرہ_DType* ثابت dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtype_سیٹ_ثنائی_ہندسہ_Or_Logic_Unsized(چوڑائی, چوڑائی_Min, numeric, dtypep->isFourstate());
}

Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_Basic_DType(VBasicDTypeKwd kwd) ثابت {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Basic_DType(fileline(), kwd);
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_ثنائی_ہندسہ_DType(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Logic_ثنائی_ہندسہ_DType(fileline(), VBasicDTypeKwd::ثنائی_ہندسہ,
                                                             چوڑائی, چوڑائی_Min, numeric);
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_Logic_DType(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Logic_ثنائی_ہندسہ_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             چوڑائی, چوڑائی_Min, numeric);
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_Logic2_State_DType(صحیح چوڑائی, صحیح چوڑائی_Min, VSigning numeric) ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Logic_ثنائی_ہندسہ_DType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, چوڑائی, چوڑائی_Min, numeric);
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_Logic_Range_DType(ثابت VNumRange& range, صحیح چوڑائی_Min,
                                           VSigning numeric) ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Logic_ثنائی_ہندسہ_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, چوڑائی_Min, numeric);
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_ثنائی_ہندسہ_Range_DType(ثابت VNumRange& range, صحیح چوڑائی_Min,
                                         VSigning numeric) ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Logic_ثنائی_ہندسہ_DType(fileline(), VBasicDTypeKwd::ثنائی_ہندسہ,
                                                             range, چوڑائی_Min, numeric);
}
AstBasicDType* Ast_گرہ::ڈھونڈو_داخل_کرو_Same_DType(AstBasicDType* nodep) {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_داخل_کرو_Same_DType(nodep);
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_Constraint_Ref_DType() ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Constraint_Ref_DType(fileline());
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_خالی_خانے_قطار_DType() ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_خالی_خانے_قطار_DType(fileline());
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_قطار_اشاریہ_DType() ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_قطار_اشاریہ_DType(fileline());
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_دھارا_DType() ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_دھارا_DType(fileline());
}
Ast_گرہ_DType* Ast_گرہ::ڈھونڈو_Void_DType() ثابت {
    واپس v3Global.rootp()->typeTablep()->ڈھونڈو_Void_DType(fileline());
}

ساکن ثابت Ast_گرہ_DType* compute_Castable_بنیاد(ثابت Ast_گرہ_DType* nodep) {
    جب (سچ) {
        اگر (ثابت AstPackArrayDType* ثابت packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            جاری;
        }
        اگر (ثابت Ast_گرہ_DType* ثابت refp = nodep->skip_Ref_مقصد_Enump()) {
            اگر (refp != nodep) {
                nodep = refp;
                جاری;
            }
        }
        واپس nodep;
    }
}

ساکن VCastable computeCastableImp(ثابت Ast_گرہ_DType* مقصد_Dtp, ثابت Ast_گرہ_DType* fromDtp,
                                    ثابت Ast_گرہ* fromConstp, ثابت منطقی checkIfaceArgCompat) {
    ثابت VCastable castable = VCastable::UNSUPPORTED;
    مقصد_Dtp = مقصد_Dtp->skip_Ref_مقصد_Enump();
    fromDtp = fromDtp->skip_Ref_مقصد_Enump();
    اگر (مقصد_Dtp == fromDtp) واپس VCastable::SAMEISH;
    اگر (مقصد_Dtp->similarDType(fromDtp)) واپس VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ثابت Ast_گرہ_DType* from_بنیاد_Dtp = compute_Castable_بنیاد(fromDtp);

    ثابت منطقی fromNumericable = VN_IS(from_بنیاد_Dtp, BasicDType) || VN_IS(from_بنیاد_Dtp, EnumDType)
                                 || VN_IS(from_بنیاد_Dtp, دھارا_DType)
                                 || VN_IS(from_بنیاد_Dtp, گرہ_UOr_Struct_DType);

    ثابت Ast_گرہ_DType* مقصد_بنیاد_Dtp = compute_Castable_بنیاد(مقصد_Dtp);
    ثابت منطقی مقصد_Numericable
        = VN_IS(مقصد_بنیاد_Dtp, BasicDType) || VN_IS(مقصد_بنیاد_Dtp, گرہ_UOr_Struct_DType);

    اگر (مقصد_بنیاد_Dtp == from_بنیاد_Dtp) واپس VCastable::COMPATIBLE;

    اگر (مقصد_Numericable) {
        اگر (fromNumericable) واپس VCastable::COMPATIBLE;
    } ورنہ اگر (VN_IS(مقصد_بنیاد_Dtp, EnumDType)) {
        اگر (VN_IS(from_بنیاد_Dtp, EnumDType) && مقصد_Dtp->same_درخت(fromDtp))
            واپس VCastable::ENUM_IMPLICIT;
        اگر (fromNumericable) واپس VCastable::ENUM_EXPLICIT;
    } ورنہ اگر (VN_IS(مقصد_Dtp, قطار_DType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, دھارا_DType))) {
        واپس VCastable::COMPATIBLE;
    } ورنہ اگر (VN_IS(مقصد_Dtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        اگر (fromConstp->isNull()) واپس VCastable::COMPATIBLE;
    } ورنہ اگر (VN_IS(مقصد_Dtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ثابت AstClass* ثابت مقصد_Classp = VN_AS(مقصد_Dtp, ClassRefDType)->classp();
        ثابت AstClass* ثابت fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ثابت منطقی downcast = AstClass::isClassExtendedFrom(مقصد_Classp, fromClassp);
        ثابت منطقی upcast = AstClass::isClassExtendedFrom(fromClassp, مقصد_Classp);
        اگر (upcast) واپس VCastable::COMPATIBLE;
        اگر (downcast) واپس VCastable::DYNAMIC_CLASS;
        واپس VCastable::INCOMPATIBLE;
    } ورنہ اگر (ثابت AstIfaceRefDType* ثابت مقصد_Ifp
               = VN_CAST(checkIfaceArgCompat ? مقصد_Dtp->elemDTypep(سچ) : مقصد_Dtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ثابت AstIfaceRefDType* ثابت fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(سچ) : fromDtp, IfaceRefDType);
        اگر (checkIfaceArgCompat && fromIfp && (مقصد_Dtp != مقصد_Ifp || fromDtp != fromIfp)) {
            ثابت AstUnpackArrayDType* ثابت مقصد_Arrayp = VN_CAST(مقصد_Dtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            اگر (!مقصد_Arrayp || !fromArrayp
                || مقصد_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
                واپس VCastable::INCOMPATIBLE;
            }
            واپس computeCastableImp(مقصد_Arrayp->subDTypep(), fromArrayp->subDTypep(), خالی_اشاریہ,
                                      checkIfaceArgCompat);
        }
        اگر (!fromIfp || مقصد_Ifp->iface_بذریعہ_Cellp() != fromIfp->iface_بذریعہ_Cellp()) {
            اگر (!checkIfaceArgCompat) واپس castable;
            واپس VCastable::INCOMPATIBLE;
        }
        ثابت منطقی sameModport = مقصد_Ifp->modportp() == fromIfp->modportp();
        اگر (!checkIfaceArgCompat) {
            اگر (!مقصد_Ifp->modportp() || sameModport) واپس VCastable::COMPATIBLE;
            واپس castable;
        }
        اگر (مقصد_Ifp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            واپس VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        اگر (مقصد_Ifp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            واپس VCastable::COMPATIBLE;
        }
        واپس VCastable::INCOMPATIBLE;
    } ورنہ اگر (ثابت AstUnpackArrayDType* ثابت مقصد_Arrayp = VN_CAST(مقصد_Dtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ثابت AstUnpackArrayDType* ثابت fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        اگر (!fromArrayp || مقصد_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
            واپس castable;
        }
        ثابت VCastable subCastable = computeCastableImp(
            مقصد_Arrayp->subDTypep(), fromArrayp->subDTypep(), خالی_اشاریہ, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ثابت Ast_گرہ_DType* ثابت مقصد_Subp = مقصد_Arrayp->subDTypep()->skipRefp();
        اگر (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(مقصد_Subp, UnpackArrayDType) || VN_IS(مقصد_Subp, IfaceRefDType)))) {
            واپس VCastable::COMPATIBLE;
        }
    }
    واپس castable;
}

VCastable Ast_گرہ::computeCastable(ثابت Ast_گرہ_DType* مقصد_Dtp, ثابت Ast_گرہ_DType* fromDtp,
                                   ثابت Ast_گرہ* fromConstp, ثابت منطقی checkIfaceArgCompat) {
    ثابت خودکار castable = computeCastableImp(مقصد_Dtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << مقصد_Dtp);
    UINFO(9, "     =?= " << fromDtp);
    اگر (fromConstp) UINFO(9, "     const= " << fromConstp);
    واپس castable;
}

Ast_گرہ_DType* Ast_گرہ::getCommonClassTypep(Ast_گرہ* node1p, Ast_گرہ* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    اگر (VN_IS(node1p, Const)) std::تبادلہ(node1p, node2p);
    {
        ثابت VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        اگر (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            واپس node1p->dtypep();
        اگر (castable == VCastable::DYNAMIC_CLASS) واپس node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    جب (classDtypep1) {
        ثابت VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        اگر (castable == VCastable::COMPATIBLE) واپس classDtypep1;
        AstClassExtends* ثابت extendsp = classDtypep1->classp()->extendsp();
        اگر (!extendsp) توڑو;
        Ast_گرہ_DType* ثابت edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->بچہ_DTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    واپس خالی_اشاریہ;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

جماعت VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<صحیح, std::string> m_interned_لفظ_Widths;  // Interned widths
    std::map<صحیح, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ثابت منطقی m_dump_چوڑائی_Min = v3Global.چوڑائی_Min_Usage() != VWidthMinUsage::MATCHES_چوڑائی;

    ساکن std::string مقصد_Letters(سائز_t قدر, منطقی زیریں_Case = جھوٹ) {
        ثابت حرف بنیاد = زیریں_Case ? 'a' : 'A';
        std::string s;
        کرو { s += static_cast<حرف>(بنیاد + قدر % 26); } جب (قدر /= 26);
        واپس s;
    }

    ثابت std::string& internConst(ثابت AstConst& nodep) {
        ثابت خودکار pair = m_internedConsts.emplace(nodep.عدد().ascii(جھوٹ), "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا += مقصد_Letters(m_internedConsts.سائز() - 1);
        واپس pair.پہلا->دوسرا;
    }

    ثابت std::string& intern_لفظ_چوڑائی(صحیح قدر) {
        ثابت خودکار pair = m_interned_لفظ_Widths.emplace(قدر, "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا += مقصد_Letters(m_interned_لفظ_Widths.سائز() - 1, سچ);
        واپس pair.پہلا->دوسرا;
    }

    ثابت std::string& intern_Wide_چوڑائی(صحیح قدر) {
        ثابت خودکار pair = m_internedWideWidths.emplace(قدر, "");
        اگر (pair.دوسرا) pair.پہلا->دوسرا += مقصد_Letters(m_internedWideWidths.سائز() - 1);
        واپس pair.پہلا->دوسرا;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ساکن منطقی is_برگ(ثابت Ast_گرہ* nodep) {
        ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        برائے (ثابت VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            اگر (opType != VNTypeInfo::OP_غیر_مستعمل) واپس جھوٹ;
        }
        واپس سچ;
    }

    // Render operand, return true if not unused
    خالی render_عملیہ(VNTypeInfo::OpEn opType, ثابت Ast_گرہ* ثابت opp, uint32_t گہرائی) {
        انتخاب (opType) {
        حالت VNTypeInfo::OP_غیر_مستعمل:  //
            توڑو;
        حالت VNTypeInfo::OP_مستعمل:  //
            m_os << ' ';
            render(opp, گہرائی);
            توڑو;
        حالت VNTypeInfo::OP_LIST:
            m_os << " [";
            برائے (ثابت Ast_گرہ* nodep = opp; nodep; nodep = nodep->nextp()) {
                اگر (nodep != opp) m_os << ", ";
                render(opp, گہرائی);
            }
            m_os << ']';
            توڑو;
        حالت VNTypeInfo::OP_OPTIONAL:
            اگر (opp) {
                m_os << " ";
                render(opp, گہرائی);
            } ورنہ {
                m_os << " nil";
            }
            توڑو;
        }
    }

    // Render the node into the stream.
    خالی render(ثابت Ast_گرہ* nodep, uint32_t گہرائی) {
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            اگر (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ورنہ اگر (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } ورنہ {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ورنہ اگر (is_برگ(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } ورنہ اگر (گہرائی == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } ورنہ {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ثابت VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            render_عملیہ(typeInfo.m_opType[0], nodep->op1p(), گہرائی - 1);
            render_عملیہ(typeInfo.m_opType[1], nodep->op2p(), گہرائی - 1);
            render_عملیہ(typeInfo.m_opType[2], nodep->op3p(), گہرائی - 1);
            render_عملیہ(typeInfo.m_opType[3], nodep->op4p(), گہرائی - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ثابت Ast_گرہ_DType* ثابت dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : خالی_اشاریہ;
        اگر (!dtypep) {
            m_os << '?';
        } ورنہ اگر (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } ورنہ {
            ثابت صحیح چوڑائی = nodep->چوڑائی();
            اگر (چوڑائی == 1) {
                m_os << '1';
            } ورنہ اگر (چوڑائی <= VL_QUADSIZE) {
                m_os << intern_لفظ_چوڑائی(چوڑائی);
            } ورنہ {
                m_os << intern_Wide_چوڑائی(چوڑائی);
            }
            اگر (m_dump_چوڑائی_Min) {
                m_os << '/';
                ثابت صحیح چوڑائی_Min = nodep->چوڑائی_Min();
                اگر (چوڑائی_Min == 1) {
                    m_os << '1';
                } ورنہ اگر (چوڑائی_Min <= VL_QUADSIZE) {
                    m_os << intern_لفظ_چوڑائی(چوڑائی_Min);
                } ورنہ {
                    m_os << intern_Wide_چوڑائی(چوڑائی_Min);
                }
            }
        }
    }

عوامی:
    VNPatternString(std::ostream& os, ثابت Ast_گرہ* nodep, uint32_t گہرائی)
        : m_os{os} {
        render(nodep, گہرائی);
    }
};

std::string Ast_گرہ_اظہاریہ::نمونہ_String(uint32_t گہرائی) ثابت {
    std::ostringstream oss;
    VNPatternString{oss, یہ, گہرائی};
    واپس oss.str();
}

//######################################################################
// VNDeleter

خالی VNDeleter::doDeletes() {
    برائے (Ast_گرہ* ثابت nodep : m_deleteps) nodep->delete_درخت();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#شامل "V3Ast__gen_visitor_defns.h"  // From ./astgen
