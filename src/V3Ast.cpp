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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3Broken.h"
#అన్తర్భావ "V3File.h"

#అన్తర్భావ <iomanip>
#అన్తర్భావ <memory>
#అన్తర్భావ <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_కణుపు::s_edit_Cnt_చివరి = 0;
uint64_t Ast_కణుపు::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
పూర్ణాంకం Ast_కణుపు::s_cloneCntGbl = 0;
uint32_t VNUser1_లోపల_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2_లోపల_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3_లోపల_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4_లోపల_Use::s_userCntGbl = 0;  // Hot cache line, leave adjacent

తార్కిక VNUser1_లోపల_Use::s_userBusy = అబద్ధం;
తార్కిక VNUser2_లోపల_Use::s_userBusy = అబద్ధం;
తార్కిక VNUser3_లోపల_Use::s_userBusy = అబద్ధం;
తార్కిక VNUser4_లోపల_Use::s_userBusy = అబద్ధం;

పూర్ణాంకం Ast_కణుపు_DType::s_unique_అంకె = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::array_పద్ధతి(స్థిరరాశి string& name) {
    కొరకు (పూర్ణాంకం i = 0; i < _ENUM_MAX; ++i) {
        స్థిరరాశి VCMethod పద్ధతి{i};
        ఒకవేళ (name == పద్ధతి.ascii()) తిరిగి పద్ధతి;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    తిరిగి VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(స్థిరరాశి శూన్య*)) స్థిరరాశి {
#నిర్వచితమైతే VL_USER_TYPE_CHECKS
    ఒకవేళ (స్థిరరాశి uint64_t* స్థిరరాశి uip = std::get_if<uint64_t>(&m_u)) తిరిగి "#"s + cvt_గమ్యం_Str(*uip);
    ఒకవేళ (శూన్య* స్థిరరాశి* స్థిరరాశి upp = std::get_if<శూన్య*>(&m_u)) తిరిగి fmtAddrp(*upp);
    తిరిగి "";
#లేకపోతే
    // Dumps void* representation
    ఒకవేళ (!m_u.పైకి) తిరిగి "";
    తిరిగి fmtAddrp(m_u.పైకి);
#అయితే_ముగింపు
}

//######################################################################
// VNType

స్థిరరాశి VNTypeInfo VNType::s_type_Info_పట్టిక[VNType::అంకె_TYPES()] = {
#అన్తర్భావ "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& క్రియాకారకం<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

తార్కిక VFwdType::is_కణుపు_Compatible(స్థిరరాశి Ast_కణుపు* nodep) స్థిరరాశి {
    స్థిరరాశి Ast_కణుపు* defp = nodep;
    ఒకవేళ (స్థిరరాశి AstTypedef* స్థిరరాశి adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి adefp = VN_CAST(defp, కణుపు_DType))
        defp = adefp->skip_Ref_గమ్యం_Non_Refp();
    ఎంపిక (m_e) {
    స్థితి VFwdType::NONE: తిరిగి నిజం; ఆపు;
    స్థితి VFwdType::ENUM: తిరిగి VN_IS(defp, EnumDType); ఆపు;
    స్థితి VFwdType::STRUCT: తిరిగి VN_IS(defp, StructDType); ఆపు;
    స్థితి VFwdType::UNION: తిరిగి VN_IS(defp, UnionDType); ఆపు;
    స్థితి VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    స్థితి VFwdType::CLASS: తిరిగి VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); ఆపు;
    స్థితి VFwdType::GENERIC_INTERFACE: తిరిగి VN_IS(defp, IfaceRefDType); ఆపు;
    అప్రమేయ: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    తిరిగి అబద్ధం;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

స్థిరరాశి std::shared_ptr<స్థిరరాశి string> VSelf_సూచిక_పాఠ్యం::s_emptyp = std::make_shared<string>("");
స్థిరరాశి std::shared_ptr<స్థిరరాశి string> VSelf_సూచిక_పాఠ్యం::s_thisp = std::make_shared<string>("this");

string VSelf_సూచిక_పాఠ్యం::replaceThis(తార్కిక useSelfForThis, స్థిరరాశి string& పాఠ్యం) {
    తిరిగి useSelfForThis ? VString::replace_మాట(పాఠ్యం, "this", "vlSelf") : పాఠ్యం;
}

string VSelf_సూచిక_పాఠ్యం::protect(తార్కిక useSelfForThis, తార్కిక protect) స్థిరరాశి {
    తిరిగి VIdProtect::protect_పదాలు_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_కణుపు::Ast_కణుపు(VNType t, File_పంక్తి* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = ఇది;  // When made, we're a list of only a single element
    //లక్షణాలు
    m_flags.did_వెడల్పు = అబద్ధం;
    m_flags.doing_వెడల్పు = అబద్ధం;
    m_flags.protect = నిజం;
    m_flags.వాడని = 0;  // Initializing this avoids a read-modify-write on construction
    edit_లెక్క_Inc();
}

Ast_కణుపు* Ast_కణుపు::abovep() స్థిరరాశి {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || మొదటి_Abovep(), ఇది, "abovep() not allowed when in midlist");
    స్థిరరాశి Ast_కణుపు* స్థిరరాశి firstp = మొదటి_Abovep() ? ఇది : m_headtailp;
    తిరిగి firstp->backp();
}
Ast_కణుపు* Ast_కణుపు::aboveLoopp() స్థిరరాశి {
    // Returns parent node. Avoid using this, may have performance issues.
    స్థిరరాశి Ast_కణుపు* nodep = ఇది;
    // Backwards over peers (versus parents)
    అయితే (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    తిరిగి nodep->backp();
}

string Ast_కణుపు::encodeName(స్థిరరాశి string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string బహిర్గతం;
    బహిర్గతం.reserve(namein.కొలత());
    కొరకు (స్వయంచాలక pos = namein.begin(); pos != namein.చివర(); ++pos) {
        ఒకవేళ ((pos == namein.begin()) ? std::అక్షరమా(pos[0])  // digits can't lead identifiers
                                    : std::అక్షరమా_అంకెనా(pos[0])) {
            బహిర్గతం += pos[0];
        } లేకపోతే ఒకవేళ (pos[0] == '_') {
            బహిర్గతం += pos[0];
            ఒకవేళ (pos + 1 == namein.చివర()) ఆపు;
            ఒకవేళ (pos[1] == '_') {
                ++pos;
                బహిర్గతం += "__05F";  // hex(_) = 0x5F
            }
        } లేకపోతే {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            స్థిరరాశి గుర్తులేని val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            బహిర్గతం += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{బహిర్గతం};
    తిరిగి vname.hashedName();
}

string Ast_కణుపు::encode_సంఖ్య(int64_t అంకె) {
    ఒకవేళ (అంకె < 0) తిరిగి "__02D" + cvt_గమ్యం_Str(-అంకె);  // 2D=-
    తిరిగి cvt_గమ్యం_Str(అంకె);
}

string Ast_కణుపు::nameProtect() స్థిరరాశి { తిరిగి VIdProtect::protectIf(name(), protect()); }
string Ast_కణుపు::origNameProtect() స్థిరరాశి { తిరిగి VIdProtect::protectIf(origName(), protect()); }

string Ast_కణుపు::shortName() స్థిరరాశి {
    string pretty = name();
    string::కొలత_type pos;
    అయితే ((pos = pretty.కనుగొను("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    తిరిగి pretty;
}

string Ast_కణుపు::dedotName(స్థిరరాశి string& namein) {
    string pretty = namein;
    string::కొలత_type pos;
    అయితే ((pos = pretty.కనుగొను("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    ఒకవేళ (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    తిరిగి pretty;
}

string Ast_కణుపు::nameNoArray(స్థిరరాశి string& namein) {
    తిరిగి namein.substr(0, namein.కనుగొను("__BRA__"));
}

string Ast_కణుపు::vcdName(స్థిరరాశి string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::కొలత_type pos;
    అయితే ((pos = pretty.కనుగొను("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    అయితే ((pos = pretty.కనుగొను('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    తిరిగి prettyName(pretty);
}

string Ast_కణుపు::prettyName(స్థిరరాశి string& namein) VL_శుద్ధ {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.నిడివి());
    కొరకు (స్థిరరాశి అక్షరం* pos = namein.c_str(); *pos;) {
        ఒకవేళ (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            కొనసాగు;
        }
        ఒకవేళ (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                కొనసాగు;
            }
            ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                కొనసాగు;
            }
            ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                కొనసాగు;
            }
            ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                కొనసాగు;
            }
            ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                కొనసాగు;
            }
            ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                కొనసాగు;
            }
            ఒకవేళ (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                అక్షరం విలువ = 0;
                విలువ += 16
                         * (std::అంకెనా(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                విలువ
                    += (std::అంకెనా(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += విలువ;
                pos += 5;
                కొనసాగు;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    ఒకవేళ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ఒకవేళ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    తిరిగి pretty;
}

string Ast_కణుపు::vpiName(స్థిరరాశి string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    స్థిరరాశి కొలత_t స్థానభ్రంశం = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.నిడివి());
    తార్కిక లోపల_Escaped_Ident = అబద్ధం;
    పూర్ణాంకం చివరి_Ident = 0;

    కొరకు (స్థిరరాశి అక్షరం* pos = namein.c_str() + స్థానభ్రంశం; *pos;) {
        అక్షరం specialChar = 0;
        ఒకవేళ (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } లేకపోతే ఒకవేళ (pos[0] == '_' && pos[1] == '_') {  // __
            ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } లేకపోతే ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } లేకపోతే ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } లేకపోతే ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__PVT__", 7)) {
                pos += 7;
                కొనసాగు;
            } లేకపోతే ఒకవేళ (0 == std::అక్షరశ్రేణి_పోలిక(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                అక్షరం విలువ = 0;
                విలువ += 16
                         * (std::అంకెనా(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                విలువ
                    += (std::అంకెనా(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                ఒకవేళ (విలువ != '_') లోపల_Escaped_Ident = నిజం;

                pretty += విలువ;
                pos += 5;
                కొనసాగు;
            }
        } లేకపోతే ఒకవేళ (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        ఒకవేళ (specialChar) {
            ఒకవేళ (లోపల_Escaped_Ident && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.చొప్పించు(చివరి_Ident, "\\");
                లోపల_Escaped_Ident = అబద్ధం;
            }

            pretty += specialChar;

            ఒకవేళ (specialChar == ']' || specialChar == '.') {
                చివరి_Ident = pretty.నిడివి();
                లోపల_Escaped_Ident = అబద్ధం;
            }
        } లేకపోతే {
            pretty += pos[0];
            ++pos;
        }
    }
    ఒకవేళ (లోపల_Escaped_Ident) {
        pretty += " ";
        pretty.చొప్పించు(చివరి_Ident, "\\");
    }
    ఒకవేళ (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ఒకవేళ (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    తిరిగి pretty;
}

string Ast_కణుపు::prettyTypeName() స్థిరరాశి {
    ఒకవేళ (name() == "") తిరిగి typeName();
    తిరిగి std::string{typeName()} + " '" + pretty_Name_సందేశం() + "'";
}

//######################################################################
// Insertion

శూన్య Ast_కణుపు::debug_వృక్షం_Change(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి అక్షరం* ఉపసర్గ, పూర్ణాంకం lineno, తార్కిక తదుపరి) {
#నిర్వచితమైతే VL_DEBUG
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
#అయితే_ముగింపు
}

మూస <>
Ast_కణుపు* Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(Ast_కణుపు* nodep, Ast_కణుపు* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_వృక్షం_Change(nodep, "-addNextThs: ", __LINE__, అబద్ధం);
    debug_వృక్షం_Change(newp, "-addNextNew: ", __LINE__, నిజం);
    ఒకవేళ (!nodep)  // verilog.y and lots of other places assume this
        తిరిగి newp;

    // Find end of old list
    Ast_కణుపు* oldtailp = nodep;
    ఒకవేళ (oldtailp->m_nextp) {
        ఒకవేళ (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } లేకపోతే {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            అయితే (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_కణుపు* స్థిరరాశి newtailp = newp->m_headtailp;
    Ast_కణుపు* స్థిరరాశి headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = శూన్యసూచిక;  // May be written again as new head
    newp->m_headtailp = శూన్యసూచిక;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_లెక్క_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_వృక్షం_Change(nodep, "-addNextOut:", __LINE__, నిజం);
    తిరిగి nodep;
}

శూన్య Ast_కణుపు::కూడు_తదుపరి_ఇక్కడ(Ast_కణుపు* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_వృక్షం_Change(ఇది, "-addHereThs: ", __LINE__, అబద్ధం);
    debug_వృక్షం_Change(newp, "-addHereNew: ", __LINE__, నిజం);
    newp->edit_లెక్క_Inc();

    Ast_కణుపు* స్థిరరాశి addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_కణుపు* స్థిరరాశి oldnextp = ఇది->m_nextp;
    ఇది->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    ఒకవేళ (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = ఇది;

    // Head/tail
    Ast_కణుపు* స్థిరరాశి oldheadtailp = ఇది->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = శూన్యసూచిక;  // Not at head any longer
    addlastp->m_headtailp = శూన్యసూచిక;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    ఒకవేళ (oldheadtailp) {  // else in middle of list, no change
        ఒకవేళ (oldheadtailp == ఇది) {  // this was one node
            ఇది->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } లేకపోతే ఒకవేళ (!oldnextp) {  // this was tail
            ఇది->m_headtailp = శూన్యసూచిక;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_వృక్షం_Change(ఇది, "-addHereOut: ", __LINE__, నిజం);
}

శూన్య Ast_కణుపు::సమితి_Op1p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, ఇది, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_వృక్షం_Change(ఇది, "-setOp1pThs: ", __LINE__, అబద్ధం);
    debug_వృక్షం_Change(newp, "-setOp1pNew: ", __LINE__, నిజం);
    m_op1p = newp;
    newp->edit_లెక్క_Inc();
    newp->m_backp = ఇది;
    debug_వృక్షం_Change(ఇది, "-setOp1pOut: ", __LINE__, అబద్ధం);
}

శూన్య Ast_కణుపు::సమితి_Op2p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, ఇది, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_వృక్షం_Change(ఇది, "-setOp2pThs: ", __LINE__, అబద్ధం);
    debug_వృక్షం_Change(newp, "-setOp2pNew: ", __LINE__, నిజం);
    m_op2p = newp;
    newp->edit_లెక్క_Inc();
    newp->m_backp = ఇది;
    debug_వృక్షం_Change(ఇది, "-setOp2pOut: ", __LINE__, అబద్ధం);
}

శూన్య Ast_కణుపు::సమితి_Op3p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, ఇది, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_వృక్షం_Change(ఇది, "-setOp3pThs: ", __LINE__, అబద్ధం);
    debug_వృక్షం_Change(newp, "-setOp3pNew: ", __LINE__, నిజం);
    m_op3p = newp;
    newp->edit_లెక్క_Inc();
    newp->m_backp = ఇది;
    debug_వృక్షం_Change(ఇది, "-setOp3pOut: ", __LINE__, అబద్ధం);
}

శూన్య Ast_కణుపు::సమితి_Op4p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, ఇది, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_వృక్షం_Change(ఇది, "-setOp4pThs: ", __LINE__, అబద్ధం);
    debug_వృక్షం_Change(newp, "-setOp4pNew: ", __LINE__, నిజం);
    m_op4p = newp;
    newp->edit_లెక్క_Inc();
    newp->m_backp = ఇది;
    debug_వృక్షం_Change(ఇది, "-setOp4pOut: ", __LINE__, అబద్ధం);
}

శూన్య Ast_కణుపు::కూడు_Op1p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ఒకవేళ (!m_op1p) {
        op1p(newp);
    } లేకపోతే {
        m_op1p->కూడు_తదుపరి(newp);
    }
}

శూన్య Ast_కణుపు::కూడు_Op2p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ఒకవేళ (!m_op2p) {
        op2p(newp);
    } లేకపోతే {
        m_op2p->కూడు_తదుపరి(newp);
    }
}

శూన్య Ast_కణుపు::కూడు_Op3p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ఒకవేళ (!m_op3p) {
        op3p(newp);
    } లేకపోతే {
        m_op3p->కూడు_తదుపరి(newp);
    }
}

శూన్య Ast_కణుపు::కూడు_Op4p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ఒకవేళ (!m_op4p) {
        op4p(newp);
    } లేకపోతే {
        m_op4p->కూడు_తదుపరి(newp);
    }
}

శూన్య Ast_కణుపు::prependOp1p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ఒకవేళ (!m_op1p) {
        op1p(newp);
    } లేకపోతే {
        m_op1p->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
    }
}

శూన్య Ast_కణుపు::prependOp2p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ఒకవేళ (!m_op2p) {
        op2p(newp);
    } లేకపోతే {
        m_op2p->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
    }
}

శూన్య Ast_కణుపు::prependOp3p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ఒకవేళ (!m_op3p) {
        op3p(newp);
    } లేకపోతే {
        m_op3p->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
    }
}

శూన్య Ast_కణుపు::prependOp4p(Ast_కణుపు* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ఒకవేళ (!m_op4p) {
        op4p(newp);
    } లేకపోతే {
        m_op4p->కూడు_ఇక్కడ_This_As_తదుపరి(newp);
    }
}

శూన్య Ast_కణుపు::replaceWith(Ast_కణుపు* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    ఇది->unlink_Fr_వెనుక(&repHandle);
    repHandle.relink(newp);
}
శూన్య Ast_కణుపు::replaceWithKeepDType(Ast_కణుపు* newp) {
    newp->dtypeFrom(ఇది);
    replaceWith(newp);
}

శూన్య VNRelinker::dump(std::ostream& str) స్థిరరాశి {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_తదుపరి ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_కణుపు* Ast_కణుపు::unlink_Fr_వెనుక_With_తదుపరి(VNRelinker* linkerp) {
    debug_వృక్షం_Change(ఇది, "-unlinkWNextThs: ", __LINE__, నిజం);
    Ast_కణుపు* స్థిరరాశి oldp = ఇది;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_లెక్క_Inc();
    // cppcheck-suppress shadowFunction
    Ast_కణుపు* స్థిరరాశి backp = oldp->m_backp;
    ఒకవేళ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        ఒకవేళ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_తదుపరి;
        } లేకపోతే ఒకవేళ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } లేకపోతే ఒకవేళ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } లేకపోతే ఒకవేళ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } లేకపోతే ఒకవేళ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } లేకపోతే {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ఒకవేళ (backp->m_nextp == oldp) {
        backp->m_nextp = శూన్యసూచిక;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_కణుపు* oldtailp = backp->m_headtailp;
        ఒకవేళ (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            అయితే (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_కణుపు* స్థిరరాశి oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } లేకపోతే ఒకవేళ (backp->m_op1p == oldp) {
        backp->m_op1p = శూన్యసూచిక;
    } లేకపోతే ఒకవేళ (backp->m_op2p == oldp) {
        backp->m_op2p = శూన్యసూచిక;
    } లేకపోతే ఒకవేళ (backp->m_op3p == oldp) {
        backp->m_op3p = శూన్యసూచిక;
    } లేకపోతే ఒకవేళ (backp->m_op4p == oldp) {
        backp->m_op4p = శూన్యసూచిక;
    } లేకపోతే {
        ఇది->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = శూన్యసూచిక;
    // Iterator fixup
    ఒకవేళ (oldp->m_iterpp) {
        *(oldp->m_iterpp) = శూన్యసూచిక;
        oldp->m_iterpp = శూన్యసూచిక;
    }
    debug_వృక్షం_Change(oldp, "-unlinkWNextOut: ", __LINE__, నిజం);
    తిరిగి oldp;
}

Ast_కణుపు* Ast_కణుపు::unlink_Fr_వెనుక(VNRelinker* linkerp) {
    debug_వృక్షం_Change(ఇది, "-unlinkFrBkThs: ", __LINE__, నిజం);
    Ast_కణుపు* స్థిరరాశి oldp = ఇది;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_లెక్క_Inc();
    // cppcheck-suppress shadowFunction
    Ast_కణుపు* స్థిరరాశి backp = oldp->m_backp;
    ఒకవేళ (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        ఒకవేళ (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = శూన్యసూచిక;
            oldp->m_iterpp = శూన్యసూచిక;
        }
        ఒకవేళ (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_తదుపరి;
        } లేకపోతే ఒకవేళ (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } లేకపోతే ఒకవేళ (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } లేకపోతే ఒకవేళ (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } లేకపోతే ఒకవేళ (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } లేకపోతే {
            ఇది->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ఒకవేళ (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        ఒకవేళ (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        ఒకవేళ (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } లేకపోతే {
        ఒకవేళ (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } లేకపోతే ఒకవేళ (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } లేకపోతే ఒకవేళ (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } లేకపోతే ఒకవేళ (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } లేకపోతే {
            ఇది->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        ఒకవేళ (oldp->m_nextp) {
            Ast_కణుపు* స్థిరరాశి newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    ఒకవేళ (oldp->m_iterpp) {  // Only if no linker, point to next in list
        ఒకవేళ (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = శూన్యసూచిక;
    }
    // Relink
    oldp->m_nextp = శూన్యసూచిక;
    oldp->m_backp = శూన్యసూచిక;
    oldp->m_headtailp = oldp;
    debug_వృక్షం_Change(oldp, "-unlinkFrBkOut: ", __LINE__, నిజం);
    తిరిగి oldp;
}

శూన్య Ast_కణుపు::relink(VNRelinker* linkerp) {
    ఒకవేళ (debug() > 8) {
        UINFO_ఉపసర్గ(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_కణుపు* స్థిరరాశి newp = ఇది;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_లెక్క_Inc();

    ఒకవేళ (debug() > 8) {
        linkerp->dump(బయటకు);
        బయటకు << పంక్తి_ముగింపు;
    }

    // cppcheck-suppress shadowFunction
    Ast_కణుపు* స్థిరరాశి backp = linkerp->m_backp;
    debug_వృక్షం_Change(ఇది, "-relinkNew: ", __LINE__, నిజం);
    debug_వృక్షం_Change(backp, "-relinkTre: ", __LINE__, నిజం);

    ఎంపిక (linkerp->m_chg) {
    స్థితి VNRelinker::RELINK_తదుపరి: backp->కూడు_తదుపరి_ఇక్కడ(newp); ఆపు;
    స్థితి VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); ఆపు;
    స్థితి VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); ఆపు;
    స్థితి VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); ఆపు;
    స్థితి VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); ఆపు;
    అప్రమేయ: ఇది->v3fatalSrc("Relink of node without any link to change."); ఆపు;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = శూన్యసూచిక;
    // Iterator fixup
    ఒకవేళ (linkerp->m_iterpp) {
        // If we're iterating over a next() link, we need to follow links off the
        // NEW node, which is always assumed to be what we are relinking to.
        // This adds a unfortunate hot 8 bytes to every AstNode, but is faster than passing
        // across every function.
        // If anyone has a cleaner way, I'd be grateful.
        newp->m_iterpp = linkerp->m_iterpp;
        *(newp->m_iterpp) = newp;
    }
    // Empty the linker so not used twice accidentally
    linkerp->m_backp = శూన్యసూచిక;
    debug_వృక్షం_Change(ఇది, "-relinkOut: ", __LINE__, నిజం);
}

శూన్య Ast_కణుపు::relinkOneLink(Ast_కణుపు*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_కణుపు* newp) {
    ఒకవేళ (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_కణుపు* స్థిరరాశి newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_కణుపు* స్థిరరాశి oldlistlastp = pointpr->m_headtailp;
        UASSERT_OBJ(!(oldlistlastp->m_nextp && oldlistlastp != pointpr), newp,
                    "Old headtailp tail isn't at the tail");
        // Next links
        newlistlastp->m_nextp = pointpr;
        pointpr->m_backp = newlistlastp;
        // Head/tail
        pointpr->m_headtailp = శూన్యసూచిక;  // Old head
        newlistlastp->m_headtailp = శూన్యసూచిక;  // Old tail
        newp->m_headtailp = oldlistlastp;  // Head points to tail
        oldlistlastp->m_headtailp = newp;  // Tail points to head
    }
    pointpr = newp;
}

శూన్య Ast_కణుపు::కూడు_ఇక్కడ_This_As_తదుపరి(Ast_కణుపు* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(ఇది->m_backp, ఇది, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_కణుపు* స్థిరరాశి backp = ఇది->m_backp;
    Ast_కణుపు* స్థిరరాశి newLastp = newp->m_headtailp;
    //
    ఇది->edit_లెక్క_Inc();
    // Common linkage
    newLastp->m_nextp = ఇది;
    ఇది->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = శూన్యసూచిక;
    // Linkage dependent on position
    ఒకవేళ (backp && backp->m_nextp == ఇది) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = శూన్యసూచిక;
    } లేకపోతే {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        ఒకవేళ (backp) {
            ఒకవేళ (backp->m_op1p == ఇది) {
                backp->m_op1p = newp;
            } లేకపోతే ఒకవేళ (backp->m_op2p == ఇది) {
                backp->m_op2p = newp;
            } లేకపోతే ఒకవేళ (backp->m_op3p == ఇది) {
                backp->m_op3p = newp;
            } లేకపోతే {
                UASSERT_OBJ(backp->m_op4p == ఇది, ఇది, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_కణుపు* స్థిరరాశి tailp = ఇది->m_headtailp;
        ఇది->m_headtailp = శూన్యసూచిక;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    ఒకవేళ (newLastp->m_iterpp) *(newLastp->m_iterpp) = ఇది;
    ఒకవేళ (ఇది->m_iterpp) {
        *(ఇది->m_iterpp) = newp;
        ఇది->m_iterpp = శూన్యసూచిక;
    }
    //
    debug_వృక్షం_Change(ఇది, "-addHereThisAsNext: ", __LINE__, నిజం);
}

//======================================================================
// Clone

Ast_కణుపు* Ast_కణుపు::clone_వృక్షం_Iter(తార్కిక need_శుద్ధ) {
    // private: Clone single node and children
    ఒకవేళ (need_శుద్ధ) purityCheck();
    Ast_కణుపు* స్థిరరాశి newp = ఇది->clone();
    ఒకవేళ (ఇది->m_op1p) newp->op1p(ఇది->m_op1p->clone_వృక్షం_Iter_List(need_శుద్ధ));
    ఒకవేళ (ఇది->m_op2p) newp->op2p(ఇది->m_op2p->clone_వృక్షం_Iter_List(need_శుద్ధ));
    ఒకవేళ (ఇది->m_op3p) newp->op3p(ఇది->m_op3p->clone_వృక్షం_Iter_List(need_శుద్ధ));
    ఒకవేళ (ఇది->m_op4p) newp->op4p(ఇది->m_op4p->clone_వృక్షం_Iter_List(need_శుద్ధ));
    newp->m_iterpp = శూన్యసూచిక;
    newp->clonep(ఇది);  // Save pointers to/from both to simplify relinking.
    ఇది->clonep(newp);  // Save pointers to/from both to simplify relinking.
    తిరిగి newp;
}

Ast_కణుపు* Ast_కణుపు::clone_వృక్షం_Iter_List(తార్కిక need_శుద్ధ) {
    // private: Clone list of nodes, set m_headtailp
    Ast_కణుపు* newheadp = శూన్యసూచిక;
    Ast_కణుపు* newtailp = శూన్యసూచిక;
    // Audited to make sure this is never nullptr
    కొరకు (Ast_కణుపు* oldp = ఇది; oldp; oldp = oldp->m_nextp) {
        Ast_కణుపు* స్థిరరాశి newp = oldp->clone_వృక్షం_Iter(need_శుద్ధ);
        newp->m_headtailp = శూన్యసూచిక;
        newp->m_backp = newtailp;
        ఒకవేళ (newtailp) newtailp->m_nextp = newp;
        ఒకవేళ (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    తిరిగి newheadp;
}

Ast_కణుపు* Ast_కణుపు::clone_వృక్షం(తార్కిక clone_తదుపరి_Link, తార్కిక need_శుద్ధ) {
    debug_వృక్షం_Change(ఇది, "-cloneThs: ", __LINE__, clone_తదుపరి_Link);
    clone_Clear_వృక్షం();
    Ast_కణుపు* newp;
    ఒకవేళ (clone_తదుపరి_Link && ఇది->m_nextp) {
        newp = clone_వృక్షం_Iter_List(need_శుద్ధ);
    } లేకపోతే {
        newp = clone_వృక్షం_Iter(need_శుద్ధ);
        newp->m_nextp = శూన్యసూచిక;
        newp->m_headtailp = newp;
    }
    newp->m_backp = శూన్యసూచిక;
    newp->clone_Relink_వృక్షం();
    debug_వృక్షం_Change(newp, "-cloneOut: ", __LINE__, నిజం);
    తిరిగి newp;
}

శూన్య Ast_కణుపు::purityCheck() {
    ఒకవేళ (VL_UNLIKELY(!is_శుద్ధ())) {
        ఇది->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << ఇది->warn_మరిన్ని()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

శూన్య Ast_కణుపు::delete_కణుపు() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_లెక్క_Inc();
    // Change links of old node so we coredump if used
    ఇది->m_nextp = reinterpret_cast<Ast_కణుపు*>(0x1);
    ఇది->m_backp = reinterpret_cast<Ast_కణుపు*>(0x1);
    ఇది->m_headtailp = reinterpret_cast<Ast_కణుపు*>(0x1);
    ఇది->m_op1p = reinterpret_cast<Ast_కణుపు*>(0x1);
    ఇది->m_op2p = reinterpret_cast<Ast_కణుపు*>(0x1);
    ఇది->m_op3p = reinterpret_cast<Ast_కణుపు*>(0x1);
    ఇది->m_op4p = reinterpret_cast<Ast_కణుపు*>(0x1);
    ఇది->m_iterpp = reinterpret_cast<Ast_కణుపు**>(0x1);
    ఒకవేళ (
#ఒకవేళ !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        నిజం
#లేకపోతే
        !v3Global.opt.debugLeak()
#అయితే_ముగింపు
    ) {
        తొలగించు ఇది;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

శూన్య Ast_కణుపు::delete_వృక్షం_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    కొరకు (Ast_కణుపు *nodep = ఇది, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        ఒకవేళ (nodep->m_op1p) nodep->m_op1p->delete_వృక్షం_Iter();
        ఒకవేళ (nodep->m_op2p) nodep->m_op2p->delete_వృక్షం_Iter();
        ఒకవేళ (nodep->m_op3p) nodep->m_op3p->delete_వృక్షం_Iter();
        ఒకవేళ (nodep->m_op4p) nodep->m_op4p->delete_వృక్షం_Iter();
        nodep->m_nextp = శూన్యసూచిక;
        nodep->m_backp = శూన్యసూచిక;
        nodep->delete_కణుపు();
    }
}

శూన్య Ast_కణుపు::delete_వృక్షం() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_వృక్షం_Change(ఇది, "-delTree:  ", __LINE__, నిజం);
    ఇది->edit_లెక్క_Inc();
    // MUST be depth first!
    delete_వృక్షం_Iter();
}

//======================================================================
// Memory checks

#నిర్వచితమైతే VL_LEAK_CHECKS
శూన్య* Ast_కణుపు::క్రియాకారకం కొత్త(కొలత_t కొలత) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_కణుపు* స్థిరరాశి objp = static_cast<Ast_కణుపు*>(::క్రియాకారకం కొత్త(కొలత));
    V3Broken::కూడు_Newed(objp);
    తిరిగి objp;
}

శూన్య Ast_కణుపు::క్రియాకారకం తొలగించు(శూన్య* objp, కొలత_t కొలత) {
    ఒకవేళ (!objp) తిరిగి;
    స్థిరరాశి Ast_కణుపు* స్థిరరాశి nodep = static_cast<Ast_కణుపు*>(objp);
    V3Broken::deleted(nodep);
    ::క్రియాకారకం తొలగించు(objp);
}
#అయితే_ముగింపు

#నిర్వచితమైతే VL_ALLOC_RANDOM_CHECKS
శూన్య* Ast_కణుపు::క్రియాకారకం కొత్త(కొలత_t కొలత) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    స్థిర స్థిరరాశి కొలత_t ASTGEN_MAX_కణుపు_కొలత = []() {
        కొలత_t max_కొలత = 0;
        కొరకు (కొలత_t t = 0; t < VNType::అంకె_TYPES(); ++t) {
            max_కొలత = std::max(max_కొలత, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        తిరిగి max_కొలత;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr కొలత_t POOL_కొలత = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    స్థిర uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    ఒకవేళ (పూర్ణాంకం విత్తనం = v3Global.opt.debugAllocRandom()) {
        స్థిర V3Mutex s_mutex;
        స్థిరరాశి V3_తాళం_Guard తాళం_2{s_mutex};
        స్థిర std::array<శూన్య*, POOL_కొలత> s_కణుపు_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(కొలత <= ASTGEN_MAX_కణుపు_కొలత, "fix ASTGEN_MAX_NODE_SIZE");
        ఒకవేళ (!s_lfsr) {
            s_lfsr = విత్తనం;
            ఒకవేళ (!s_lfsr) s_lfsr = ~s_lfsr;
            కొరకు (కొలత_t i = 0; i < POOL_కొలత; ++i) {
                s_కణుపు_Pool[i] = ::క్రియాకారకం కొత్త(ASTGEN_MAX_కణుపు_కొలత + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::క్రమీకరించు(std::begin(s_కణుపు_Pool), std::చివర(s_కణుపు_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        స్థిరరాశి కొలత_t సూచీ = s_lfsr % POOL_కొలత;
        Ast_కణుపు* స్థిరరాశి objp = static_cast<Ast_కణుపు*>(s_కణుపు_Pool[సూచీ]);
        s_కణుపు_Pool[సూచీ] = ::క్రియాకారకం కొత్త(ASTGEN_MAX_కణుపు_కొలత + 64);  // For later new()
        తిరిగి objp;
    } లేకపోతే {
        Ast_కణుపు* స్థిరరాశి objp = static_cast<Ast_కణుపు*>(::క్రియాకారకం కొత్త(కొలత));
        తిరిగి objp;
    }
}
శూన్య Ast_కణుపు::క్రియాకారకం తొలగించు(శూన్య* objp, కొలత_t కొలత) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (శూన్య)objp;
    (శూన్య)కొలత;
}
#అయితే_ముగింపు

//======================================================================
// Iterators

శూన్య Ast_కణుపు::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ఒకవేళ (m_op1p) m_op1p->iterate_And_తదుపరి(v);
    ఒకవేళ (m_op2p) m_op2p->iterate_And_తదుపరి(v);
    ఒకవేళ (m_op3p) m_op3p->iterate_And_తదుపరి(v);
    ఒకవేళ (m_op4p) m_op4p->iterate_And_తదుపరి(v);
}

శూన్య Ast_కణుపు::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ఒకవేళ (m_op1p) m_op1p->iterate_And_తదుపరి_Const(v);
    ఒకవేళ (m_op2p) m_op2p->iterate_And_తదుపరి_Const(v);
    ఒకవేళ (m_op3p) m_op3p->iterate_And_తదుపరి_Const(v);
    ఒకవేళ (m_op4p) m_op4p->iterate_And_తదుపరి_Const(v);
}

శూన్య Ast_కణుపు::iterate_And_తదుపరి(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_కణుపు* nodep = ఇది;
#నిర్వచితమైతే VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#అయితే_ముగింపు
    // cppcheck-suppress knownConditionTrueFalse
    ఒకవేళ (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    అయితే (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        ఒకవేళ (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_కణుపు* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        ఒకవేళ (!niterp) తిరిగి;  // Perhaps node deleted inside accept
        niterp->m_iterpp = శూన్యసూచిక;
        ఒకవేళ (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } లేకపోతే {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

శూన్య Ast_కణుపు::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_కణుపు* nodep = ఇది;
    అయితే (nodep->m_nextp) nodep = nodep->m_nextp;
    అయితే (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        ఒకవేళ (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } లేకపోతే {
            nodep = శూన్యసూచిక;
        }  // else: backp points up the tree.
    }
}

శూన్య Ast_కణుపు::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    ఒకవేళ (m_op1p) m_op1p->iterateListBackwardsConst(v);
    ఒకవేళ (m_op2p) m_op2p->iterateListBackwardsConst(v);
    ఒకవేళ (m_op3p) m_op3p->iterateListBackwardsConst(v);
    ఒకవేళ (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

శూన్య Ast_కణుపు::iterate_And_తదుపరి_Const(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_కణుపు* nodep = ఇది;
    చేయి {
        Ast_కణుపు* స్థిరరాశి nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } అయితే (nodep);
}

Ast_కణుపు* Ast_కణుపు::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_కణుపు* nodep = ఇది;  // Note "this" may point to bogus point later in this function
    ఒకవేళ (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } లేకపోతే ఒకవేళ (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* స్థిరరాశి tempp = కొత్త AstBegin{nodep->fileline(), "[EditWrapper]", nodep, అబద్ధం};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి();
        VL_DO_DANGLING(tempp->delete_వృక్షం(), tempp);
    } లేకపోతే {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_కణుపు** nextnodepp = శూన్యసూచిక;
        ఒకవేళ (ఇది->m_backp->m_op1p == ఇది) {
            nextnodepp = &(ఇది->m_backp->m_op1p);
        } లేకపోతే ఒకవేళ (ఇది->m_backp->m_op2p == ఇది) {
            nextnodepp = &(ఇది->m_backp->m_op2p);
        } లేకపోతే ఒకవేళ (ఇది->m_backp->m_op3p == ఇది) {
            nextnodepp = &(ఇది->m_backp->m_op3p);
        } లేకపోతే ఒకవేళ (ఇది->m_backp->m_op4p == ఇది) {
            nextnodepp = &(ఇది->m_backp->m_op4p);
        } లేకపోతే ఒకవేళ (ఇది->m_backp->m_nextp == ఇది) {
            nextnodepp = &(ఇది->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, ఇది, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    తిరిగి nodep;
}

//======================================================================

శూన్య Ast_కణుపు::clone_Relink_వృక్షం() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    కొరకు (Ast_కణుపు* nodep = ఇది; nodep; nodep = nodep->m_nextp) {
        ఒకవేళ (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        ఒకవేళ (nodep->m_op1p) nodep->m_op1p->clone_Relink_వృక్షం();
        ఒకవేళ (nodep->m_op2p) nodep->m_op2p->clone_Relink_వృక్షం();
        ఒకవేళ (nodep->m_op3p) nodep->m_op3p->clone_Relink_వృక్షం();
        ఒకవేళ (nodep->m_op4p) nodep->m_op4p->clone_Relink_వృక్షం();
    }
}

//======================================================================
// Comparison

తార్కిక Ast_కణుపు::gate_వృక్షం_Iter() స్థిరరాశి {
    // private: Return true if the two trees are identical
    ఒకవేళ (!isGateOptimizable()) తిరిగి అబద్ధం;
    ఒకవేళ (m_op1p && !m_op1p->gate_వృక్షం_Iter()) తిరిగి అబద్ధం;
    ఒకవేళ (m_op2p && !m_op2p->gate_వృక్షం_Iter()) తిరిగి అబద్ధం;
    ఒకవేళ (m_op3p && !m_op3p->gate_వృక్షం_Iter()) తిరిగి అబద్ధం;
    ఒకవేళ (m_op4p && !m_op4p->gate_వృక్షం_Iter()) తిరిగి అబద్ధం;
    తిరిగి నిజం;
}

తార్కిక Ast_కణుపు::same_వృక్షం_Iter(స్థిరరాశి Ast_కణుపు* node1p, స్థిరరాశి Ast_కణుపు* node2p, తార్కిక ign_తదుపరి,
                           తార్కిక gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    అయితే (నిజం) {
        ఒకవేళ (!node1p && !node2p) తిరిగి నిజం;
        ఒకవేళ (!node1p || !node2p) తిరిగి అబద్ధం;
        ఒకవేళ (node1p->type() != node2p->type()) తిరిగి అబద్ధం;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        ఒకవేళ (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) తిరిగి అబద్ధం;
        ఒకవేళ (!node1p->same_కణుపు(node2p) || (gateOnly && !node1p->isGateOptimizable())) తిరిగి అబద్ధం;
        ఒకవేళ (!same_వృక్షం_Iter(node1p->m_op1p, node2p->m_op1p, అబద్ధం, gateOnly)
            || !same_వృక్షం_Iter(node1p->m_op2p, node2p->m_op2p, అబద్ధం, gateOnly)
            || !same_వృక్షం_Iter(node1p->m_op3p, node2p->m_op3p, అబద్ధం, gateOnly)
            || !same_వృక్షం_Iter(node1p->m_op4p, node2p->m_op4p, అబద్ధం, gateOnly)) {
            తిరిగి అబద్ధం;
        }
        ఒకవేళ (ign_తదుపరి) తిరిగి నిజం;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

శూన్య Ast_కణుపు::check_వృక్షం_Iter(స్థిరరాశి Ast_కణుపు* prevBackp) స్థిరరాశి VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == ఇది->backp(), ఇది, "Back node inconsistent");
    స్థిరరాశి VNTypeInfo& typeInfo = VNType::typeInfo(ఇది->type());
    కొరకు (పూర్ణాంకం i = 1; i <= 4; i++) {
        Ast_కణుపు* nodep = శూన్యసూచిక;
        ఎంపిక (i) {
        స్థితి 1: nodep = op1p(); ఆపు;
        స్థితి 2: nodep = op2p(); ఆపు;
        స్థితి 3: nodep = op3p(); ఆపు;
        స్థితి 4: nodep = op4p(); ఆపు;
        అప్రమేయ: ఇది->v3fatalSrc("Bad case"); ఆపు;
        }
        // cppcheck-suppress danglingTempReference
        స్థిరరాశి అక్షరం* opName = typeInfo.m_opNamep[i - 1];
        ఎంపిక (typeInfo.m_opType[i - 1]) {
        స్థితి VNTypeInfo::OP_వాడని:
            UASSERT_OBJ(!nodep, ఇది, typeInfo.m_namep << " must not use " << opName << "()");
            ఆపు;
        స్థితి VNTypeInfo::OP_వాడిన:
            UASSERT_OBJ(nodep, ఇది,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), ఇది,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_వృక్షం_Iter(ఇది);
            ఆపు;
        స్థితి VNTypeInfo::OP_LIST:
            ఒకవేళ (స్థిరరాశి Ast_కణుపు* స్థిరరాశి headp = nodep) {
                // cppcheck-suppress shadowFunction
                స్థిరరాశి Ast_కణుపు* backp = ఇది;
                స్థిరరాశి Ast_కణుపు* tailp;
                స్థిరరాశి Ast_కణుపు* opp = headp;
                చేయి {
                    opp->check_వృక్షం_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } అయితే (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            ఆపు;
        స్థితి VNTypeInfo::OP_OPTIONAL:
            ఒకవేళ (nodep) {
                UASSERT_OBJ(!nodep->nextp(), ఇది,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_వృక్షం_Iter(ఇది);
            }
            ఆపు;
        అప్రమేయ: ఇది->v3fatalSrc("Bad case"); ఆపు;
        }
    }
    ఒకవేళ (v3Global.opt.debug_వెడల్పు() && v3Global.వెడల్పు_Min_Usage() == VWidthMinUsage::VERILOG_వెడల్పు) {
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_CAST(ఇది, కణుపు_సమాసం)) {
            స్థిరరాశి అక్షరం* స్థిరరాశి whyp = exprp->వెడల్పు_Mismatch();
            ఒకవేళ (whyp) {
                స్వయంచాలక dtypeStr = [](స్థిరరాశి Ast_కణుపు_సమాసం* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    తిరిగి ss.str();
                };
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_Uniop* స్థిరరాశి uniopp = VN_CAST(exprp, కణుపు_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి biopp = VN_CAST(exprp, కణుపు_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } లేకపోతే {
                    UASSERT_OBJ(అబద్ధం, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
అక్షరం* Ast_కణుపు::dump_వృక్షం_Json_Gdb(స్థిరరాశి Ast_కణుపు* nodep) {
    ఒకవేళ (!nodep) తిరిగి strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodep_ప్రవాహం;
    nodep->dump_వృక్షం_Json(nodep_ప్రవాహం);
    స్థిరరాశి std::string str = nodep_ప్రవాహం.rdbuf()->str();
    తిరిగి strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
అక్షరం* Ast_కణుపు::dump_వృక్షం_Json_Gdb(స్థిరరాశి అక్షరం* str) { తిరిగి strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
అక్షరం* Ast_కణుపు::dump_వృక్షం_Json_Gdb(intptr_t nodep) {
    ఒకవేళ (!nodep) తిరిగి strdup("{\"addr\":\"NULL\"}\n");
    తిరిగి dump_వృక్షం_Json_Gdb(reinterpret_cast<స్థిరరాశి Ast_కణుపు*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
శూన్య Ast_కణుపు::dumpGdb(స్థిరరాశి Ast_కణుపు* nodep) {  // For GDB only  // LCOV_EXCL_START
    ఒకవేళ (!nodep) {
        బయటకు << "<nullptr>" << పంక్తి_ముగింపు;
        తిరిగి;
    }
    nodep->dump_Gdb_శీర్షిక();
    బయటకు << "  ";
    nodep->dump(బయటకు);
    బయటకు << పంక్తి_ముగింపు;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
శూన్య Ast_కణుపు::dump_Gdb_శీర్షిక() స్థిరరాశి {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(బయటకు);
    బయటకు << "  Fileline = " << fileline() << పంక్తి_ముగింపు;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
శూన్య Ast_కణుపు::dump_వృక్షం_Gdb(స్థిరరాశి Ast_కణుపు* nodep) {  // For GDB only  // LCOV_EXCL_START
    ఒకవేళ (!nodep) {
        బయటకు << "<nullptr>" << పంక్తి_ముగింపు;
        తిరిగి;
    }
    nodep->dump_Gdb_శీర్షిక();
    nodep->dump_వృక్షం(బయటకు);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
శూన్య Ast_కణుపు::dump_వృక్షం_File_Gdb(స్థిరరాశి Ast_కణుపు* nodep,  // LCOV_EXCL_START
                              స్థిరరాశి అక్షరం* filenamep) {  // For GDB only
    ఒకవేళ (!nodep) {
        బయటకు << "<nullptr>" << పంక్తి_ముగింపు;
        తిరిగి;
    }
    స్థిరరాశి string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_వృక్షం_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
శూన్య Ast_కణుపు::checkIter() స్థిరరాశి {
    ఒకవేళ (m_iterpp) {
        dumpPtrs(బయటకు);
        // Perhaps something forgot to clear m_iterpp?
        ఇది->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

శూన్య Ast_కణుపు::dumpPtrs(std::ostream& os) స్థిరరాశి {
    os << "This=" << typeName() << " " << cvt_గమ్యం_Hex(ఇది);
    os << " back=" << cvt_గమ్యం_Hex(backp());
    ఒకవేళ (nextp()) os << " next=" << cvt_గమ్యం_Hex(nextp());
    ఒకవేళ (m_headtailp == ఇది) {
        os << " headtail=this";
    } లేకపోతే {
        os << " headtail=" << cvt_గమ్యం_Hex(m_headtailp);
    }
    ఒకవేళ (op1p()) os << " op1p=" << cvt_గమ్యం_Hex(op1p());
    ఒకవేళ (op2p()) os << " op2p=" << cvt_గమ్యం_Hex(op2p());
    ఒకవేళ (op3p()) os << " op3p=" << cvt_గమ్యం_Hex(op3p());
    ఒకవేళ (op4p()) os << " op4p=" << cvt_గమ్యం_Hex(op4p());
    స్థిరరాశి స్వయంచాలక dumpUser = [&os](స్థిరరాశి అక్షరం* ఉపసర్గ, స్థిరరాశి VNUser& user) {
        స్థిరరాశి std::string s
            = user.dumpStr([](స్థిరరాశి శూన్య* p) -> std::string { తిరిగి cvt_గమ్యం_Hex(p); });
        ఒకవేళ (!s.ఖాళీలు()) os << ఉపసర్గ << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    ఒకవేళ (m_iterpp) {
        os << " iterpp=" << cvt_గమ్యం_Hex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

శూన్య Ast_కణుపు::dump_వృక్షం(std::ostream& os, స్థిరరాశి string& indent, పూర్ణాంకం max_లోతు) స్థిరరాశి {
    స్థిర పూర్ణాంకం s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << ఇది << '\n';
    ఒకవేళ (VN_DELETED(ఇది)) తిరిగి;
    ఒకవేళ (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    ఒకవేళ (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvt_గమ్యం_Hex(ఇది)
           << పంక్తి_ముగింపు;  // endl intentional to do flush
        తిరిగి;
    }  // LCOV_EXCL_STOP
    ఒకవేళ (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    ఒకవేళ (max_లోతు == 1) {
        ఒకవేళ (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } లేకపోతే {
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_వృక్షం(os, indent + "1:", max_లోతు - 1);
        }
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_వృక్షం(os, indent + "2:", max_లోతు - 1);
        }
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_వృక్షం(os, indent + "3:", max_లోతు - 1);
        }
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_వృక్షం(os, indent + "4:", max_లోతు - 1);
        }
    }
}

శూన్య Ast_కణుపు::dump_వృక్షం_And_తదుపరి(std::ostream& os, స్థిరరాశి string& indent, పూర్ణాంకం max_లోతు) స్థిరరాశి {
    // Audited to make sure this is never nullptr
    కొరకు (స్థిరరాశి Ast_కణుపు* nodep = ఇది; nodep; nodep = nodep->nextp()) {
        nodep->dump_వృక్షం(os, indent, max_లోతు);
    }
}

శూన్య Ast_కణుపు::dump_వృక్షం_File(స్థిరరాశి string& filename, తార్కిక doDump) {
    // Not const function as calls checkTree
    ఒకవేళ (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            స్థిరరాశి std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            ఒకవేళ (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_లెక్క_చివరి();
            *logsp << "> to <e" << std::dec << edit_లెక్క_Gbl() << ">\n";
            ఒకవేళ (edit_లెక్క_Gbl() == edit_లెక్క_చివరి() && ::dump_వృక్షం_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } లేకపోతే {
                dump_వృక్షం(*logsp);
                edit_లెక్క_సమితి_చివరి();  // Next dump can indicate start from here
            }
        }
    }
}

స్థిర శూన్య drawChildren(std::ostream& os, స్థిరరాశి Ast_కణుపు* thisp, స్థిరరాశి Ast_కణుపు* childp,
                         స్థిరరాశి std::string& బిడ్డ_Name) {
    ఒకవేళ (childp) {
        os << "\tn" << cvt_గమ్యం_Hex(thisp) << " -> n" << cvt_గమ్యం_Hex(childp) << " [" << "label=\""
           << బిడ్డ_Name << "\" color=red];\n";
        కొరకు (స్థిరరాశి Ast_కణుపు* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_వృక్షం_Dot(os);
            ఒకవేళ (nodep->nextp()) {
                os << "\tn" << cvt_గమ్యం_Hex(nodep) << " -> n" << cvt_గమ్యం_Hex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvt_గమ్యం_Hex(nodep) << ", n" << cvt_గమ్యం_Hex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

శూన్య Ast_కణుపు::dump_వృక్షం_Dot(std::ostream& os) స్థిరరాశి {
    os << "\tn" << cvt_గమ్యం_Hex(ఇది) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, ఇది, m_op1p, "op1");
    drawChildren(os, ఇది, m_op2p, "op2");
    drawChildren(os, ఇది, m_op3p, "op3");
    drawChildren(os, ఇది, m_op4p, "op4");
}

శూన్య Ast_కణుపు::dump_వృక్షం_Json_File(స్థిరరాశి string& filename, తార్కిక doDump) {
    ఒకవేళ (!doDump) తిరిగి;
    UINFO(2, "Dumping " << filename);
    స్థిరరాశి std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ఒకవేళ (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_వృక్షం_Json(*treejsonp);
    *treejsonp << '\n';
}

శూన్య Ast_కణుపు::dumpJsonMetaFileGdb(స్థిరరాశి అక్షరం* filename) { dumpJsonMetaFile(filename); }
శూన్య Ast_కణుపు::dumpJsonMetaFile(స్థిరరాశి string& filename) {
    UINFO(2, "Dumping " << filename);
    స్థిరరాశి std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ఒకవేళ (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_పంక్తి::file_Name_అంకె_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptr_పేర్లు_Dump_Json(*treejsonp);
    *treejsonp << "}\n";
}

శూన్య Ast_కణుపు::dump_వృక్షం_Dot_File(స్థిరరాశి string& filename, తార్కిక doDump) {
    ఒకవేళ (doDump) {
        UINFO(2, "Dumping " << filename);
        స్థిరరాశి std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        ఒకవేళ (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_వృక్షం_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_కణుపు::instanceStr() స్థిరరాశి {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr గుర్తులేని maxIterations = 10000;
    గుర్తులేని iter_లెక్క = 0;
    // Walk 'backp' chain
    కొరకు (స్థిరరాశి Ast_కణుపు* currp = ఇది; currp; currp = currp->backp(), ++iter_లెక్క) {
        ఒకవేళ (VL_UNCOVERABLE(iter_లెక్క >= maxIterations)) తిరిగి "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        ఒకవేళ (స్థిరరాశి AstScope* స్థిరరాశి scopep = VN_CAST(currp, Scope)) {
            తిరిగి scopep->is_శిఖరం() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        ఒకవేళ (స్థిరరాశి AstModule* స్థిరరాశి modp = VN_CAST(currp, Module)) {
            స్థిరరాశి string instanceName = modp->someInstanceName();
            తిరిగి instanceName.ఖాళీలు() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    తిరిగి "";
}
శూన్య Ast_కణుపు::v3error_చివర(స్థిరరాశి std::ostringstream& str) స్థిరరాశి VL_RELEASE(V3Error::s().m_mutex) {
    ఒకవేళ (!m_fileline) {
        V3Error::v3error_చివర(str, "", శూన్యసూచిక);
    } లేకపోతే {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        స్థిరరాశి string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().error_సంకేతం()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        ఒకవేళ (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_కణుపు*>(ఇది)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3error_చివర(nsstr, instanceStrExtra);
    }
}
శూన్య Ast_కణుపు::v3error_చివర_Fatal(స్థిరరాశి std::ostringstream& str) స్థిరరాశి
    VL_RELEASE(V3Error::s().m_mutex) {
    v3error_చివర(str);
    ప్రతిజ్ఞా(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

శూన్య Ast_కణుపు::dtypeChgSigned(తార్కిక flag) {
    UASSERT_OBJ(dtypep(), ఇది, "No dtype when changing to (un)signed");
    dtype_Chg_వెడల్పు_Signed(dtypep()->వెడల్పు(), dtypep()->వెడల్పు_Min(), VSigning::fromBool(flag));
}
శూన్య Ast_కణుపు::dtype_Chg_వెడల్పు(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min) {
    UASSERT_OBJ(dtypep(), ఇది, "No dtype when changing width");
    dtype_Chg_వెడల్పు_Signed(వెడల్పు, వెడల్పు_Min, dtypep()->numeric());
}

శూన్య Ast_కణుపు::dtype_Chg_వెడల్పు_Signed(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) {
    UASSERT_OBJ(dtypep(), ఇది, "No dtype when changing width");
    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి enumDTypep = dtypep()->skip_Ref_గమ్యం_Enump();
    ఒకవేళ (వెడల్పు == dtypep()->వెడల్పు() && వెడల్పు_Min == dtypep()->వెడల్పు_Min()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        తిరిగి;  // Correct already
    }
    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtype_సమితి_బిట్టు_Or_Logic_Unsized(వెడల్పు, వెడల్పు_Min, numeric, dtypep->isFourstate());
}

Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_Basic_DType(VBasicDTypeKwd kwd) స్థిరరాశి {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Basic_DType(fileline(), kwd);
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_బిట్టు_DType(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Logic_బిట్టు_DType(fileline(), VBasicDTypeKwd::బిట్టు,
                                                             వెడల్పు, వెడల్పు_Min, numeric);
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_Logic_DType(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Logic_బిట్టు_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             వెడల్పు, వెడల్పు_Min, numeric);
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_Logic2_State_DType(పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min, VSigning numeric) స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Logic_బిట్టు_DType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, వెడల్పు, వెడల్పు_Min, numeric);
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_Logic_Range_DType(స్థిరరాశి VNumRange& range, పూర్ణాంకం వెడల్పు_Min,
                                           VSigning numeric) స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Logic_బిట్టు_DType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, వెడల్పు_Min, numeric);
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_బిట్టు_Range_DType(స్థిరరాశి VNumRange& range, పూర్ణాంకం వెడల్పు_Min,
                                         VSigning numeric) స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Logic_బిట్టు_DType(fileline(), VBasicDTypeKwd::బిట్టు,
                                                             range, వెడల్పు_Min, numeric);
}
AstBasicDType* Ast_కణుపు::కనుగొను_చొప్పించు_Same_DType(AstBasicDType* nodep) {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_చొప్పించు_Same_DType(nodep);
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_Constraint_Ref_DType() స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Constraint_Ref_DType(fileline());
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_ఖాళీలు_వరుస_DType() స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_ఖాళీలు_వరుస_DType(fileline());
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_వరుస_సూచీ_DType() స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_వరుస_సూచీ_DType(fileline());
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_ప్రవాహం_DType() స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_ప్రవాహం_DType(fileline());
}
Ast_కణుపు_DType* Ast_కణుపు::కనుగొను_Void_DType() స్థిరరాశి {
    తిరిగి v3Global.rootp()->typeTablep()->కనుగొను_Void_DType(fileline());
}

స్థిర స్థిరరాశి Ast_కణుపు_DType* compute_Castable_పునాది(స్థిరరాశి Ast_కణుపు_DType* nodep) {
    అయితే (నిజం) {
        ఒకవేళ (స్థిరరాశి AstPackArrayDType* స్థిరరాశి packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            కొనసాగు;
        }
        ఒకవేళ (స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి refp = nodep->skip_Ref_గమ్యం_Enump()) {
            ఒకవేళ (refp != nodep) {
                nodep = refp;
                కొనసాగు;
            }
        }
        తిరిగి nodep;
    }
}

స్థిర VCastable computeCastableImp(స్థిరరాశి Ast_కణుపు_DType* గమ్యం_Dtp, స్థిరరాశి Ast_కణుపు_DType* fromDtp,
                                    స్థిరరాశి Ast_కణుపు* fromConstp, స్థిరరాశి తార్కిక checkIfaceArgCompat) {
    స్థిరరాశి VCastable castable = VCastable::UNSUPPORTED;
    గమ్యం_Dtp = గమ్యం_Dtp->skip_Ref_గమ్యం_Enump();
    fromDtp = fromDtp->skip_Ref_గమ్యం_Enump();
    ఒకవేళ (గమ్యం_Dtp == fromDtp) తిరిగి VCastable::SAMEISH;
    ఒకవేళ (గమ్యం_Dtp->similarDType(fromDtp)) తిరిగి VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    స్థిరరాశి Ast_కణుపు_DType* from_పునాది_Dtp = compute_Castable_పునాది(fromDtp);

    స్థిరరాశి తార్కిక fromNumericable = VN_IS(from_పునాది_Dtp, BasicDType) || VN_IS(from_పునాది_Dtp, EnumDType)
                                 || VN_IS(from_పునాది_Dtp, ప్రవాహం_DType)
                                 || VN_IS(from_పునాది_Dtp, కణుపు_UOr_Struct_DType);

    స్థిరరాశి Ast_కణుపు_DType* గమ్యం_పునాది_Dtp = compute_Castable_పునాది(గమ్యం_Dtp);
    స్థిరరాశి తార్కిక గమ్యం_Numericable
        = VN_IS(గమ్యం_పునాది_Dtp, BasicDType) || VN_IS(గమ్యం_పునాది_Dtp, కణుపు_UOr_Struct_DType);

    ఒకవేళ (గమ్యం_పునాది_Dtp == from_పునాది_Dtp) తిరిగి VCastable::COMPATIBLE;

    ఒకవేళ (గమ్యం_Numericable) {
        ఒకవేళ (fromNumericable) తిరిగి VCastable::COMPATIBLE;
    } లేకపోతే ఒకవేళ (VN_IS(గమ్యం_పునాది_Dtp, EnumDType)) {
        ఒకవేళ (VN_IS(from_పునాది_Dtp, EnumDType) && గమ్యం_Dtp->same_వృక్షం(fromDtp))
            తిరిగి VCastable::ENUM_IMPLICIT;
        ఒకవేళ (fromNumericable) తిరిగి VCastable::ENUM_EXPLICIT;
    } లేకపోతే ఒకవేళ (VN_IS(గమ్యం_Dtp, వరుస_DType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, ప్రవాహం_DType))) {
        తిరిగి VCastable::COMPATIBLE;
    } లేకపోతే ఒకవేళ (VN_IS(గమ్యం_Dtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        ఒకవేళ (fromConstp->isNull()) తిరిగి VCastable::COMPATIBLE;
    } లేకపోతే ఒకవేళ (VN_IS(గమ్యం_Dtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        స్థిరరాశి AstClass* స్థిరరాశి గమ్యం_Classp = VN_AS(గమ్యం_Dtp, ClassRefDType)->classp();
        స్థిరరాశి AstClass* స్థిరరాశి fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        స్థిరరాశి తార్కిక downcast = AstClass::isClassExtendedFrom(గమ్యం_Classp, fromClassp);
        స్థిరరాశి తార్కిక upcast = AstClass::isClassExtendedFrom(fromClassp, గమ్యం_Classp);
        ఒకవేళ (upcast) తిరిగి VCastable::COMPATIBLE;
        ఒకవేళ (downcast) తిరిగి VCastable::DYNAMIC_CLASS;
        తిరిగి VCastable::INCOMPATIBLE;
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి గమ్యం_Ifp
               = VN_CAST(checkIfaceArgCompat ? గమ్యం_Dtp->elemDTypep(నిజం) : గమ్యం_Dtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(నిజం) : fromDtp, IfaceRefDType);
        ఒకవేళ (checkIfaceArgCompat && fromIfp && (గమ్యం_Dtp != గమ్యం_Ifp || fromDtp != fromIfp)) {
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి గమ్యం_Arrayp = VN_CAST(గమ్యం_Dtp, UnpackArrayDType);
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            ఒకవేళ (!గమ్యం_Arrayp || !fromArrayp
                || గమ్యం_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
                తిరిగి VCastable::INCOMPATIBLE;
            }
            తిరిగి computeCastableImp(గమ్యం_Arrayp->subDTypep(), fromArrayp->subDTypep(), శూన్యసూచిక,
                                      checkIfaceArgCompat);
        }
        ఒకవేళ (!fromIfp || గమ్యం_Ifp->iface_ద్వారా_Cellp() != fromIfp->iface_ద్వారా_Cellp()) {
            ఒకవేళ (!checkIfaceArgCompat) తిరిగి castable;
            తిరిగి VCastable::INCOMPATIBLE;
        }
        స్థిరరాశి తార్కిక sameModport = గమ్యం_Ifp->modportp() == fromIfp->modportp();
        ఒకవేళ (!checkIfaceArgCompat) {
            ఒకవేళ (!గమ్యం_Ifp->modportp() || sameModport) తిరిగి VCastable::COMPATIBLE;
            తిరిగి castable;
        }
        ఒకవేళ (గమ్యం_Ifp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            తిరిగి VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        ఒకవేళ (గమ్యం_Ifp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            తిరిగి VCastable::COMPATIBLE;
        }
        తిరిగి VCastable::INCOMPATIBLE;
    } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి గమ్యం_Arrayp = VN_CAST(గమ్యం_Dtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        ఒకవేళ (!fromArrayp || గమ్యం_Arrayp->elementsConst() != fromArrayp->elementsConst()) {
            తిరిగి castable;
        }
        స్థిరరాశి VCastable subCastable = computeCastableImp(
            గమ్యం_Arrayp->subDTypep(), fromArrayp->subDTypep(), శూన్యసూచిక, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి గమ్యం_Subp = గమ్యం_Arrayp->subDTypep()->skipRefp();
        ఒకవేళ (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(గమ్యం_Subp, UnpackArrayDType) || VN_IS(గమ్యం_Subp, IfaceRefDType)))) {
            తిరిగి VCastable::COMPATIBLE;
        }
    }
    తిరిగి castable;
}

VCastable Ast_కణుపు::computeCastable(స్థిరరాశి Ast_కణుపు_DType* గమ్యం_Dtp, స్థిరరాశి Ast_కణుపు_DType* fromDtp,
                                   స్థిరరాశి Ast_కణుపు* fromConstp, స్థిరరాశి తార్కిక checkIfaceArgCompat) {
    స్థిరరాశి స్వయంచాలక castable = computeCastableImp(గమ్యం_Dtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << గమ్యం_Dtp);
    UINFO(9, "     =?= " << fromDtp);
    ఒకవేళ (fromConstp) UINFO(9, "     const= " << fromConstp);
    తిరిగి castable;
}

Ast_కణుపు_DType* Ast_కణుపు::getCommonClassTypep(Ast_కణుపు* node1p, Ast_కణుపు* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    ఒకవేళ (VN_IS(node1p, Const)) std::మార్పిడి(node1p, node2p);
    {
        స్థిరరాశి VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        ఒకవేళ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            తిరిగి node1p->dtypep();
        ఒకవేళ (castable == VCastable::DYNAMIC_CLASS) తిరిగి node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    అయితే (classDtypep1) {
        స్థిరరాశి VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        ఒకవేళ (castable == VCastable::COMPATIBLE) తిరిగి classDtypep1;
        AstClassExtends* స్థిరరాశి extendsp = classDtypep1->classp()->extendsp();
        ఒకవేళ (!extendsp) ఆపు;
        Ast_కణుపు_DType* స్థిరరాశి edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->బిడ్డ_DTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    తిరిగి శూన్యసూచిక;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

తరగతి VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<పూర్ణాంకం, std::string> m_interned_మాట_Widths;  // Interned widths
    std::map<పూర్ణాంకం, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    స్థిరరాశి తార్కిక m_dump_వెడల్పు_Min = v3Global.వెడల్పు_Min_Usage() != VWidthMinUsage::MATCHES_వెడల్పు;

    స్థిర std::string గమ్యం_Letters(కొలత_t విలువ, తార్కిక క్రింది_Case = అబద్ధం) {
        స్థిరరాశి అక్షరం పునాది = క్రింది_Case ? 'a' : 'A';
        std::string s;
        చేయి { s += static_cast<అక్షరం>(పునాది + విలువ % 26); } అయితే (విలువ /= 26);
        తిరిగి s;
    }

    స్థిరరాశి std::string& internConst(స్థిరరాశి AstConst& nodep) {
        స్థిరరాశి స్వయంచాలక pair = m_internedConsts.emplace(nodep.అంకె().ascii(అబద్ధం), "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ += గమ్యం_Letters(m_internedConsts.కొలత() - 1);
        తిరిగి pair.మొదటి->రెండవ;
    }

    స్థిరరాశి std::string& intern_మాట_వెడల్పు(పూర్ణాంకం విలువ) {
        స్థిరరాశి స్వయంచాలక pair = m_interned_మాట_Widths.emplace(విలువ, "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ += గమ్యం_Letters(m_interned_మాట_Widths.కొలత() - 1, నిజం);
        తిరిగి pair.మొదటి->రెండవ;
    }

    స్థిరరాశి std::string& intern_Wide_వెడల్పు(పూర్ణాంకం విలువ) {
        స్థిరరాశి స్వయంచాలక pair = m_internedWideWidths.emplace(విలువ, "");
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ += గమ్యం_Letters(m_internedWideWidths.కొలత() - 1);
        తిరిగి pair.మొదటి->రెండవ;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    స్థిర తార్కిక is_ఆకు(స్థిరరాశి Ast_కణుపు* nodep) {
        స్థిరరాశి VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        కొరకు (స్థిరరాశి VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            ఒకవేళ (opType != VNTypeInfo::OP_వాడని) తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }

    // Render operand, return true if not unused
    శూన్య render_నిర్వాహ్యం(VNTypeInfo::OpEn opType, స్థిరరాశి Ast_కణుపు* స్థిరరాశి opp, uint32_t లోతు) {
        ఎంపిక (opType) {
        స్థితి VNTypeInfo::OP_వాడని:  //
            ఆపు;
        స్థితి VNTypeInfo::OP_వాడిన:  //
            m_os << ' ';
            render(opp, లోతు);
            ఆపు;
        స్థితి VNTypeInfo::OP_LIST:
            m_os << " [";
            కొరకు (స్థిరరాశి Ast_కణుపు* nodep = opp; nodep; nodep = nodep->nextp()) {
                ఒకవేళ (nodep != opp) m_os << ", ";
                render(opp, లోతు);
            }
            m_os << ']';
            ఆపు;
        స్థితి VNTypeInfo::OP_OPTIONAL:
            ఒకవేళ (opp) {
                m_os << " ";
                render(opp, లోతు);
            } లేకపోతే {
                m_os << " nil";
            }
            ఆపు;
        }
    }

    // Render the node into the stream.
    శూన్య render(స్థిరరాశి Ast_కణుపు* nodep, uint32_t లోతు) {
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            ఒకవేళ (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } లేకపోతే ఒకవేళ (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } లేకపోతే {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } లేకపోతే ఒకవేళ (is_ఆకు(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } లేకపోతే ఒకవేళ (లోతు == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } లేకపోతే {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            స్థిరరాశి VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            render_నిర్వాహ్యం(typeInfo.m_opType[0], nodep->op1p(), లోతు - 1);
            render_నిర్వాహ్యం(typeInfo.m_opType[1], nodep->op2p(), లోతు - 1);
            render_నిర్వాహ్యం(typeInfo.m_opType[2], nodep->op3p(), లోతు - 1);
            render_నిర్వాహ్యం(typeInfo.m_opType[3], nodep->op4p(), లోతు - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : శూన్యసూచిక;
        ఒకవేళ (!dtypep) {
            m_os << '?';
        } లేకపోతే ఒకవేళ (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } లేకపోతే {
            స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->వెడల్పు();
            ఒకవేళ (వెడల్పు == 1) {
                m_os << '1';
            } లేకపోతే ఒకవేళ (వెడల్పు <= VL_QUADSIZE) {
                m_os << intern_మాట_వెడల్పు(వెడల్పు);
            } లేకపోతే {
                m_os << intern_Wide_వెడల్పు(వెడల్పు);
            }
            ఒకవేళ (m_dump_వెడల్పు_Min) {
                m_os << '/';
                స్థిరరాశి పూర్ణాంకం వెడల్పు_Min = nodep->వెడల్పు_Min();
                ఒకవేళ (వెడల్పు_Min == 1) {
                    m_os << '1';
                } లేకపోతే ఒకవేళ (వెడల్పు_Min <= VL_QUADSIZE) {
                    m_os << intern_మాట_వెడల్పు(వెడల్పు_Min);
                } లేకపోతే {
                    m_os << intern_Wide_వెడల్పు(వెడల్పు_Min);
                }
            }
        }
    }

బహిరంగ:
    VNPatternString(std::ostream& os, స్థిరరాశి Ast_కణుపు* nodep, uint32_t లోతు)
        : m_os{os} {
        render(nodep, లోతు);
    }
};

std::string Ast_కణుపు_సమాసం::నమూనా_String(uint32_t లోతు) స్థిరరాశి {
    std::ostringstream oss;
    VNPatternString{oss, ఇది, లోతు};
    తిరిగి oss.str();
}

//######################################################################
// VNDeleter

శూన్య VNDeleter::doDeletes() {
    కొరకు (Ast_కణుపు* స్థిరరాశి nodep : m_deleteps) nodep->delete_వృక్షం();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#అన్తర్భావ "V3Ast__gen_visitor_defns.h"  // From ./astgen
