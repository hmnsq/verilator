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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Broken.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"

#ଅନ୍ତର୍ଭାଵ <iomanip>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t Ast_ଗଣ୍ଠି::s_editCntLast = 0;
uint64_t Ast_ଗଣ୍ଠି::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

ତାର୍କିକ VNUser1InUse::s_userBusy = ମିଥ୍ୟା;
ତାର୍କିକ VNUser2InUse::s_userBusy = ମିଥ୍ୟା;
ତାର୍କିକ VNUser3InUse::s_userBusy = ମିଥ୍ୟା;
ତାର୍କିକ VNUser4InUse::s_userBusy = ମିଥ୍ୟା;

ପୂର୍ଣ୍ଣସଂଖ୍ୟା Ast_ଗଣ୍ଠି_DType::s_unique_ଅଙ୍କ = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(ଅଚଳ string& name) {
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < _ENUM_MAX; ++i) {
        ଅଚଳ VCMethod method{i};
        ଯଦି (name == method.ascii()) ଫେରନ୍ତୁ method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    ଫେରନ୍ତୁ VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(ଅଚଳ ଶୂନ୍ୟ*)) ଅଚଳ {
#ifdef VL_USER_TYPE_CHECKS
    ଯଦି (ଅଚଳ uint64_t* ଅଚଳ uip = std::get_if<uint64_t>(&m_u)) ଫେରନ୍ତୁ "#"s + cvtToStr(*uip);
    ଯଦି (ଶୂନ୍ୟ* ଅଚଳ* ଅଚଳ upp = std::get_if<ଶୂନ୍ୟ*>(&m_u)) ଫେରନ୍ତୁ fmtAddrp(*upp);
    ଫେରନ୍ତୁ "";
#ଅନ୍ୟଥା
    // Dumps void* representation
    ଯଦି (!m_u.up) ଫେରନ୍ତୁ "";
    ଫେରନ୍ତୁ fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

ଅଚଳ VNTypeInfo VNType::s_typeInfoTable[VNType::ଅଙ୍କ_TYPES()] = {
#ଅନ୍ତର୍ଭାଵ "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

ତାର୍କିକ VFwdType::is_ଗଣ୍ଠି_Compatible(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) ଅଚଳ {
    ଅଚଳ Ast_ଗଣ୍ଠି* defp = nodep;
    ଯଦି (ଅଚଳ AstTypedef* ଅଚଳ adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ adefp = VN_CAST(defp, ଗଣ୍ଠି_DType))
        defp = adefp->skipRefToNonRefp();
    ବାଛ (m_e) {
    ସ୍ଥିତି VFwdType::NONE: ଫେରନ୍ତୁ ସତ୍ୟ; ଭାଙ୍ଗ;
    ସ୍ଥିତି VFwdType::ENUM: ଫେରନ୍ତୁ VN_IS(defp, EnumDType); ଭାଙ୍ଗ;
    ସ୍ଥିତି VFwdType::STRUCT: ଫେରନ୍ତୁ VN_IS(defp, StructDType); ଭାଙ୍ଗ;
    ସ୍ଥିତି VFwdType::UNION: ଫେରନ୍ତୁ VN_IS(defp, UnionDType); ଭାଙ୍ଗ;
    ସ୍ଥିତି VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    ସ୍ଥିତି VFwdType::CLASS: ଫେରନ୍ତୁ VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); ଭାଙ୍ଗ;
    ସ୍ଥିତି VFwdType::GENERIC_INTERFACE: ଫେରନ୍ତୁ VN_IS(defp, IfaceRefDType); ଭାଙ୍ଗ;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    ଫେରନ୍ତୁ ମିଥ୍ୟା;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

ଅଚଳ std::shared_ptr<ଅଚଳ string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
ଅଚଳ std::shared_ptr<ଅଚଳ string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(ତାର୍କିକ useSelfForThis, ଅଚଳ string& text) {
    ଫେରନ୍ତୁ useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(ତାର୍କିକ useSelfForThis, ତାର୍କିକ protect) ଅଚଳ {
    ଫେରନ୍ତୁ VIdProtect::protect_ଶବ୍ଦଗୁଡ଼ିକ_If(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

Ast_ଗଣ୍ଠି::Ast_ଗଣ୍ଠି(VNType t, File_ଧାଡ଼ି* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = ଏହି;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = ମିଥ୍ୟା;
    m_flags.doingWidth = ମିଥ୍ୟା;
    m_flags.protect = ସତ୍ୟ;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    edit_ଗଣନା_Inc();
}

Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::abovep() ଅଚଳ {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), ଏହି, "abovep() not allowed when in midlist");
    ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ firstp = firstAbovep() ? ଏହି : m_headtailp;
    ଫେରନ୍ତୁ firstp->backp();
}
Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::aboveLoopp() ଅଚଳ {
    // Returns parent node. Avoid using this, may have performance issues.
    ଅଚଳ Ast_ଗଣ୍ଠି* nodep = ଏହି;
    // Backwards over peers (versus parents)
    ଯେତେବେଳେ (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    ଫେରନ୍ତୁ nodep->backp();
}

string Ast_ଗଣ୍ଠି::encodeName(ଅଚଳ string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    ପାଇଁ (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        ଯଦି ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } ଅନ୍ୟଥା ଯଦି (pos[0] == '_') {
            out += pos[0];
            ଯଦି (pos + 1 == namein.end()) ଭାଙ୍ଗ;
            ଯଦି (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } ଅନ୍ୟଥା {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            ଅଚଳ unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    ଫେରନ୍ତୁ vname.hashedName();
}

string Ast_ଗଣ୍ଠି::encode_ସଂଖ୍ୟା(int64_t ଅଙ୍କ) {
    ଯଦି (ଅଙ୍କ < 0) ଫେରନ୍ତୁ "__02D" + cvtToStr(-ଅଙ୍କ);  // 2D=-
    ଫେରନ୍ତୁ cvtToStr(ଅଙ୍କ);
}

string Ast_ଗଣ୍ଠି::nameProtect() ଅଚଳ { ଫେରନ୍ତୁ VIdProtect::protectIf(name(), protect()); }
string Ast_ଗଣ୍ଠି::origNameProtect() ଅଚଳ { ଫେରନ୍ତୁ VIdProtect::protectIf(origName(), protect()); }

string Ast_ଗଣ୍ଠି::shortName() ଅଚଳ {
    string pretty = name();
    string::size_type pos;
    ଯେତେବେଳେ ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    ଫେରନ୍ତୁ pretty;
}

string Ast_ଗଣ୍ଠି::dedotName(ଅଚଳ string& namein) {
    string pretty = namein;
    string::size_type pos;
    ଯେତେବେଳେ ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    ଯଦି (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ଫେରନ୍ତୁ pretty;
}

string Ast_ଗଣ୍ଠି::nameNoArray(ଅଚଳ string& namein) {
    ଫେରନ୍ତୁ namein.substr(0, namein.find("__BRA__"));
}

string Ast_ଗଣ୍ଠି::vcdName(ଅଚଳ string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    ଯେତେବେଳେ ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    ଯେତେବେଳେ ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    ଫେରନ୍ତୁ prettyName(pretty);
}

string Ast_ଗଣ୍ଠି::prettyName(ଅଚଳ string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* pos = namein.c_str(); *pos;) {
        ଯଦି (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            ଜାରି;
        }
        ଯଦି (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            ଯଦି (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                ଜାରି;
            }
            ଯଦି (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                ଜାରି;
            }
            ଯଦି (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                ଜାରି;
            }
            ଯଦି (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                ଜାରି;
            }
            ଯଦି (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                ଜାରି;
            }
            ଯଦି (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                ଜାରି;
            }
            ଯଦି (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                ଅକ୍ଷର ମୂଲ୍ୟ = 0;
                ମୂଲ୍ୟ += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ମୂଲ୍ୟ
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += ମୂଲ୍ୟ;
                pos += 5;
                ଜାରି;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    ଯଦି (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ଯଦି (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ଫେରନ୍ତୁ pretty;
}

string Ast_ଗଣ୍ଠି::vpiName(ଅଚଳ string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    ଅଚଳ size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    ତାର୍କିକ inEscapedIdent = ମିଥ୍ୟା;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା lastIdent = 0;

    ପାଇଁ (ଅଚଳ ଅକ୍ଷର* pos = namein.c_str() + offset; *pos;) {
        ଅକ୍ଷର specialChar = 0;
        ଯଦି (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } ଅନ୍ୟଥା ଯଦି (pos[0] == '_' && pos[1] == '_') {  // __
            ଯଦି (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } ଅନ୍ୟଥା ଯଦି (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } ଅନ୍ୟଥା ଯଦି (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } ଅନ୍ୟଥା ଯଦି (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                ଜାରି;
            } ଅନ୍ୟଥା ଯଦି (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                ଅକ୍ଷର ମୂଲ୍ୟ = 0;
                ମୂଲ୍ୟ += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                ମୂଲ୍ୟ
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                ଯଦି (ମୂଲ୍ୟ != '_') inEscapedIdent = ସତ୍ୟ;

                pretty += ମୂଲ୍ୟ;
                pos += 5;
                ଜାରି;
            }
        } ଅନ୍ୟଥା ଯଦି (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        ଯଦି (specialChar) {
            ଯଦି (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.ଭର୍ତ୍ତି(lastIdent, "\\");
                inEscapedIdent = ମିଥ୍ୟା;
            }

            pretty += specialChar;

            ଯଦି (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = ମିଥ୍ୟା;
            }
        } ଅନ୍ୟଥା {
            pretty += pos[0];
            ++pos;
        }
    }
    ଯଦି (inEscapedIdent) {
        pretty += " ";
        pretty.ଭର୍ତ୍ତି(lastIdent, "\\");
    }
    ଯଦି (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    ଯଦି (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    ଫେରନ୍ତୁ pretty;
}

string Ast_ଗଣ୍ଠି::prettyTypeName() ଅଚଳ {
    ଯଦି (name() == "") ଫେରନ୍ତୁ typeName();
    ଫେରନ୍ତୁ std::string{typeName()} + " '" + pretty_Name_ସନ୍ଦେଶ() + "'";
}

//######################################################################
// Insertion

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::debug_ବୃକ୍ଷ_Change(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ ଅକ୍ଷର* prefix, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno, ତାର୍କିକ next) {
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
Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(Ast_ଗଣ୍ଠି* nodep, Ast_ଗଣ୍ଠି* newp) {
    // Add to m_nextp, returns this
    UDEBUGONLY(UASSERT_OBJ(newp, nodep, "Null item passed to addNext"););
    debug_ବୃକ୍ଷ_Change(nodep, "-addNextThs: ", __LINE__, ମିଥ୍ୟା);
    debug_ବୃକ୍ଷ_Change(newp, "-addNextNew: ", __LINE__, ସତ୍ୟ);
    ଯଦି (!nodep)  // verilog.y and lots of other places assume this
        ଫେରନ୍ତୁ newp;

    // Find end of old list
    Ast_ଗଣ୍ଠି* oldtailp = nodep;
    ଯଦି (oldtailp->m_nextp) {
        ଯଦି (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } ଅନ୍ୟଥା {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            ଯେତେବେଳେ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    Ast_ଗଣ୍ଠି* ଅଚଳ newtailp = newp->m_headtailp;
    Ast_ଗଣ୍ଠି* ଅଚଳ headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->edit_ଗଣନା_Inc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debug_ବୃକ୍ଷ_Change(nodep, "-addNextOut:", __LINE__, ସତ୍ୟ);
    ଫେରନ୍ତୁ nodep;
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::addNextHere(Ast_ଗଣ୍ଠି* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debug_ବୃକ୍ଷ_Change(ଏହି, "-addHereThs: ", __LINE__, ମିଥ୍ୟା);
    debug_ବୃକ୍ଷ_Change(newp, "-addHereNew: ", __LINE__, ସତ୍ୟ);
    newp->edit_ଗଣନା_Inc();

    Ast_ଗଣ୍ଠି* ଅଚଳ addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    Ast_ଗଣ୍ଠି* ଅଚଳ oldnextp = ଏହି->m_nextp;
    ଏହି->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    ଯଦି (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = ଏହି;

    // Head/tail
    Ast_ଗଣ୍ଠି* ଅଚଳ oldheadtailp = ଏହି->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    ଯଦି (oldheadtailp) {  // else in middle of list, no change
        ଯଦି (oldheadtailp == ଏହି) {  // this was one node
            ଏହି->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } ଅନ୍ୟଥା ଯଦି (!oldnextp) {  // this was tail
            ଏହି->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debug_ବୃକ୍ଷ_Change(ଏହି, "-addHereOut: ", __LINE__, ସତ୍ୟ);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::setOp1p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, ଏହି, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debug_ବୃକ୍ଷ_Change(ଏହି, "-setOp1pThs: ", __LINE__, ମିଥ୍ୟା);
    debug_ବୃକ୍ଷ_Change(newp, "-setOp1pNew: ", __LINE__, ସତ୍ୟ);
    m_op1p = newp;
    newp->edit_ଗଣନା_Inc();
    newp->m_backp = ଏହି;
    debug_ବୃକ୍ଷ_Change(ଏହି, "-setOp1pOut: ", __LINE__, ମିଥ୍ୟା);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::setOp2p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, ଏହି, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debug_ବୃକ୍ଷ_Change(ଏହି, "-setOp2pThs: ", __LINE__, ମିଥ୍ୟା);
    debug_ବୃକ୍ଷ_Change(newp, "-setOp2pNew: ", __LINE__, ସତ୍ୟ);
    m_op2p = newp;
    newp->edit_ଗଣନା_Inc();
    newp->m_backp = ଏହି;
    debug_ବୃକ୍ଷ_Change(ଏହି, "-setOp2pOut: ", __LINE__, ମିଥ୍ୟା);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::setOp3p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, ଏହି, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debug_ବୃକ୍ଷ_Change(ଏହି, "-setOp3pThs: ", __LINE__, ମିଥ୍ୟା);
    debug_ବୃକ୍ଷ_Change(newp, "-setOp3pNew: ", __LINE__, ସତ୍ୟ);
    m_op3p = newp;
    newp->edit_ଗଣନା_Inc();
    newp->m_backp = ଏହି;
    debug_ବୃକ୍ଷ_Change(ଏହି, "-setOp3pOut: ", __LINE__, ମିଥ୍ୟା);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::setOp4p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, ଏହି, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debug_ବୃକ୍ଷ_Change(ଏହି, "-setOp4pThs: ", __LINE__, ମିଥ୍ୟା);
    debug_ବୃକ୍ଷ_Change(newp, "-setOp4pNew: ", __LINE__, ସତ୍ୟ);
    m_op4p = newp;
    newp->edit_ଗଣନା_Inc();
    newp->m_backp = ଏହି;
    debug_ବୃକ୍ଷ_Change(ଏହି, "-setOp4pOut: ", __LINE__, ମିଥ୍ୟା);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::addOp1p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ଯଦି (!m_op1p) {
        op1p(newp);
    } ଅନ୍ୟଥା {
        m_op1p->addNext(newp);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::addOp2p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ଯଦି (!m_op2p) {
        op2p(newp);
    } ଅନ୍ୟଥା {
        m_op2p->addNext(newp);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::addOp3p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ଯଦି (!m_op3p) {
        op3p(newp);
    } ଅନ୍ୟଥା {
        m_op3p->addNext(newp);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::addOp4p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ଯଦି (!m_op4p) {
        op4p(newp);
    } ଅନ୍ୟଥା {
        m_op4p->addNext(newp);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::prependOp1p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ଯଦି (!m_op1p) {
        op1p(newp);
    } ଅନ୍ୟଥା {
        m_op1p->addHereThisAsNext(newp);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::prependOp2p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ଯଦି (!m_op2p) {
        op2p(newp);
    } ଅନ୍ୟଥା {
        m_op2p->addHereThisAsNext(newp);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::prependOp3p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ଯଦି (!m_op3p) {
        op3p(newp);
    } ଅନ୍ୟଥା {
        m_op3p->addHereThisAsNext(newp);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::prependOp4p(Ast_ଗଣ୍ଠି* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    ଯଦି (!m_op4p) {
        op4p(newp);
    } ଅନ୍ୟଥା {
        m_op4p->addHereThisAsNext(newp);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::replaceWith(Ast_ଗଣ୍ଠି* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    ଏହି->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::replaceWithKeepDType(Ast_ଗଣ୍ଠି* newp) {
    newp->dtypeFrom(ଏହି);
    replaceWith(newp);
}

ଶୂନ୍ୟ VNRelinker::dump(std::ostream& str) ଅଚଳ {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debug_ବୃକ୍ଷ_Change(ଏହି, "-unlinkWNextThs: ", __LINE__, ସତ୍ୟ);
    Ast_ଗଣ୍ଠି* ଅଚଳ oldp = ଏହି;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ଗଣନା_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ଗଣ୍ଠି* ଅଚଳ backp = oldp->m_backp;
    ଯଦି (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        ଯଦି (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ଅନ୍ୟଥା {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ଯଦି (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        Ast_ଗଣ୍ଠି* oldtailp = backp->m_headtailp;
        ଯଦି (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            ଯେତେବେଳେ (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        Ast_ଗଣ୍ଠି* ଅଚଳ oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } ଅନ୍ୟଥା ଯଦି (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } ଅନ୍ୟଥା ଯଦି (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } ଅନ୍ୟଥା ଯଦି (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } ଅନ୍ୟଥା ଯଦି (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } ଅନ୍ୟଥା {
        ଏହି->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    ଯଦି (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debug_ବୃକ୍ଷ_Change(oldp, "-unlinkWNextOut: ", __LINE__, ସତ୍ୟ);
    ଫେରନ୍ତୁ oldp;
}

Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::unlinkFrBack(VNRelinker* linkerp) {
    debug_ବୃକ୍ଷ_Change(ଏହି, "-unlinkFrBkThs: ", __LINE__, ସତ୍ୟ);
    Ast_ଗଣ୍ଠି* ଅଚଳ oldp = ଏହି;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->edit_ଗଣନା_Inc();
    // cppcheck-suppress shadowFunction
    Ast_ଗଣ୍ଠି* ଅଚଳ backp = oldp->m_backp;
    ଯଦି (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        ଯଦି (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        ଯଦି (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } ଅନ୍ୟଥା {
            ଏହି->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    ଯଦି (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        ଯଦି (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        ଯଦି (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } ଅନ୍ୟଥା {
        ଯଦି (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } ଅନ୍ୟଥା ଯଦି (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } ଅନ୍ୟଥା {
            ଏହି->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        ଯଦି (oldp->m_nextp) {
            Ast_ଗଣ୍ଠି* ଅଚଳ newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    ଯଦି (oldp->m_iterpp) {  // Only if no linker, point to next in list
        ଯଦି (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debug_ବୃକ୍ଷ_Change(oldp, "-unlinkFrBkOut: ", __LINE__, ସତ୍ୟ);
    ଫେରନ୍ତୁ oldp;
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::relink(VNRelinker* linkerp) {
    ଯଦି (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    Ast_ଗଣ୍ଠି* ଅଚଳ newp = ଏହି;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->edit_ଗଣନା_Inc();

    ଯଦି (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    Ast_ଗଣ୍ଠି* ଅଚଳ backp = linkerp->m_backp;
    debug_ବୃକ୍ଷ_Change(ଏହି, "-relinkNew: ", __LINE__, ସତ୍ୟ);
    debug_ବୃକ୍ଷ_Change(backp, "-relinkTre: ", __LINE__, ସତ୍ୟ);

    ବାଛ (linkerp->m_chg) {
    ସ୍ଥିତି VNRelinker::RELINK_NEXT: backp->addNextHere(newp); ଭାଙ୍ଗ;
    ସ୍ଥିତି VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); ଭାଙ୍ଗ;
    ସ୍ଥିତି VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); ଭାଙ୍ଗ;
    ସ୍ଥିତି VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); ଭାଙ୍ଗ;
    ସ୍ଥିତି VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); ଭାଙ୍ଗ;
    ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଏହି->v3fatalSrc("Relink of node without any link to change."); ଭାଙ୍ଗ;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    ଯଦି (linkerp->m_iterpp) {
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
    debug_ବୃକ୍ଷ_Change(ଏହି, "-relinkOut: ", __LINE__, ସତ୍ୟ);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::relinkOneLink(Ast_ଗଣ୍ଠି*& pointpr,  // Ref to pointer that gets set to newp
                            Ast_ଗଣ୍ଠି* newp) {
    ଯଦି (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        Ast_ଗଣ୍ଠି* ଅଚଳ newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        Ast_ଗଣ୍ଠି* ଅଚଳ oldlistlastp = pointpr->m_headtailp;
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

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::addHereThisAsNext(Ast_ଗଣ୍ଠି* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(ଏହି->m_backp, ଏହି, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    Ast_ଗଣ୍ଠି* ଅଚଳ backp = ଏହି->m_backp;
    Ast_ଗଣ୍ଠି* ଅଚଳ newLastp = newp->m_headtailp;
    //
    ଏହି->edit_ଗଣନା_Inc();
    // Common linkage
    newLastp->m_nextp = ଏହି;
    ଏହି->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    ଯଦି (backp && backp->m_nextp == ଏହି) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } ଅନ୍ୟଥା {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        ଯଦି (backp) {
            ଯଦି (backp->m_op1p == ଏହି) {
                backp->m_op1p = newp;
            } ଅନ୍ୟଥା ଯଦି (backp->m_op2p == ଏହି) {
                backp->m_op2p = newp;
            } ଅନ୍ୟଥା ଯଦି (backp->m_op3p == ଏହି) {
                backp->m_op3p = newp;
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(backp->m_op4p == ଏହି, ଏହି, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        Ast_ଗଣ୍ଠି* ଅଚଳ tailp = ଏହି->m_headtailp;
        ଏହି->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    ଯଦି (newLastp->m_iterpp) *(newLastp->m_iterpp) = ଏହି;
    ଯଦି (ଏହି->m_iterpp) {
        *(ଏହି->m_iterpp) = newp;
        ଏହି->m_iterpp = nullptr;
    }
    //
    debug_ବୃକ୍ଷ_Change(ଏହି, "-addHereThisAsNext: ", __LINE__, ସତ୍ୟ);
}

//======================================================================
// Clone

Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::clone_ବୃକ୍ଷ_Iter(ତାର୍କିକ needPure) {
    // private: Clone single node and children
    ଯଦି (needPure) purityCheck();
    Ast_ଗଣ୍ଠି* ଅଚଳ newp = ଏହି->clone();
    ଯଦି (ଏହି->m_op1p) newp->op1p(ଏହି->m_op1p->clone_ବୃକ୍ଷ_Iter_List(needPure));
    ଯଦି (ଏହି->m_op2p) newp->op2p(ଏହି->m_op2p->clone_ବୃକ୍ଷ_Iter_List(needPure));
    ଯଦି (ଏହି->m_op3p) newp->op3p(ଏହି->m_op3p->clone_ବୃକ୍ଷ_Iter_List(needPure));
    ଯଦି (ଏହି->m_op4p) newp->op4p(ଏହି->m_op4p->clone_ବୃକ୍ଷ_Iter_List(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(ଏହି);  // Save pointers to/from both to simplify relinking.
    ଏହି->clonep(newp);  // Save pointers to/from both to simplify relinking.
    ଫେରନ୍ତୁ newp;
}

Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::clone_ବୃକ୍ଷ_Iter_List(ତାର୍କିକ needPure) {
    // private: Clone list of nodes, set m_headtailp
    Ast_ଗଣ୍ଠି* newheadp = nullptr;
    Ast_ଗଣ୍ଠି* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    ପାଇଁ (Ast_ଗଣ୍ଠି* oldp = ଏହି; oldp; oldp = oldp->m_nextp) {
        Ast_ଗଣ୍ଠି* ଅଚଳ newp = oldp->clone_ବୃକ୍ଷ_Iter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        ଯଦି (newtailp) newtailp->m_nextp = newp;
        ଯଦି (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    ଫେରନ୍ତୁ newheadp;
}

Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::clone_ବୃକ୍ଷ(ତାର୍କିକ cloneNextLink, ତାର୍କିକ needPure) {
    debug_ବୃକ୍ଷ_Change(ଏହି, "-cloneThs: ", __LINE__, cloneNextLink);
    clone_Clear_ବୃକ୍ଷ();
    Ast_ଗଣ୍ଠି* newp;
    ଯଦି (cloneNextLink && ଏହି->m_nextp) {
        newp = clone_ବୃକ୍ଷ_Iter_List(needPure);
    } ଅନ୍ୟଥା {
        newp = clone_ବୃକ୍ଷ_Iter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->clone_Relink_ବୃକ୍ଷ();
    debug_ବୃକ୍ଷ_Change(newp, "-cloneOut: ", __LINE__, ସତ୍ୟ);
    ଫେରନ୍ତୁ newp;
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::purityCheck() {
    ଯଦି (VL_UNLIKELY(!isPure())) {
        ଏହି->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << ଏହି->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::delete_ଗଣ୍ଠି() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    edit_ଗଣନା_Inc();
    // Change links of old node so we coredump if used
    ଏହି->m_nextp = reinterpret_cast<Ast_ଗଣ୍ଠି*>(0x1);
    ଏହି->m_backp = reinterpret_cast<Ast_ଗଣ୍ଠି*>(0x1);
    ଏହି->m_headtailp = reinterpret_cast<Ast_ଗଣ୍ଠି*>(0x1);
    ଏହି->m_op1p = reinterpret_cast<Ast_ଗଣ୍ଠି*>(0x1);
    ଏହି->m_op2p = reinterpret_cast<Ast_ଗଣ୍ଠି*>(0x1);
    ଏହି->m_op3p = reinterpret_cast<Ast_ଗଣ୍ଠି*>(0x1);
    ଏହି->m_op4p = reinterpret_cast<Ast_ଗଣ୍ଠି*>(0x1);
    ଏହି->m_iterpp = reinterpret_cast<Ast_ଗଣ୍ଠି**>(0x1);
    ଯଦି (
#ଯଦି !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        ସତ୍ୟ
#ଅନ୍ୟଥା
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete ଏହି;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::delete_ବୃକ୍ଷ_Iter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = ଏହି, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        ଯଦି (nodep->m_op1p) nodep->m_op1p->delete_ବୃକ୍ଷ_Iter();
        ଯଦି (nodep->m_op2p) nodep->m_op2p->delete_ବୃକ୍ଷ_Iter();
        ଯଦି (nodep->m_op3p) nodep->m_op3p->delete_ବୃକ୍ଷ_Iter();
        ଯଦି (nodep->m_op4p) nodep->m_op4p->delete_ବୃକ୍ଷ_Iter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->delete_ଗଣ୍ଠି();
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::delete_ବୃକ୍ଷ() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debug_ବୃକ୍ଷ_Change(ଏହି, "-delTree:  ", __LINE__, ସତ୍ୟ);
    ଏହି->edit_ଗଣନା_Inc();
    // MUST be depth first!
    delete_ବୃକ୍ଷ_Iter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
ଶୂନ୍ୟ* Ast_ଗଣ୍ଠି::operator ନୂତନ(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    Ast_ଗଣ୍ଠି* ଅଚଳ objp = static_cast<Ast_ଗଣ୍ଠି*>(::operator ନୂତନ(size));
    V3Broken::addNewed(objp);
    ଫେରନ୍ତୁ objp;
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::operator delete(ଶୂନ୍ୟ* objp, size_t size) {
    ଯଦି (!objp) ଫେରନ୍ତୁ;
    ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ nodep = static_cast<Ast_ଗଣ୍ଠି*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
ଶୂନ୍ୟ* Ast_ଗଣ୍ଠି::operator ନୂତନ(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    ସ୍ଥିର ଅଚଳ size_t ASTGEN_MAX_ଗଣ୍ଠି_SIZE = []() {
        size_t maxSize = 0;
        ପାଇଁ (size_t t = 0; t < VNType::ଅଙ୍କ_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        ଫେରନ୍ତୁ maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    ସ୍ଥିର uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    ଯଦି (ପୂର୍ଣ୍ଣସଂଖ୍ୟା seed = v3Global.opt.debugAllocRandom()) {
        ସ୍ଥିର V3Mutex s_mutex;
        ଅଚଳ V3LockGuard lock{s_mutex};
        ସ୍ଥିର std::array<ଶୂନ୍ୟ*, POOL_SIZE> s_ଗଣ୍ଠି_Pool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_ଗଣ୍ଠି_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        ଯଦି (!s_lfsr) {
            s_lfsr = seed;
            ଯଦି (!s_lfsr) s_lfsr = ~s_lfsr;
            ପାଇଁ (size_t i = 0; i < POOL_SIZE; ++i) {
                s_ଗଣ୍ଠି_Pool[i] = ::operator ନୂତନ(ASTGEN_MAX_ଗଣ୍ଠି_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_ଗଣ୍ଠି_Pool), std::end(s_ଗଣ୍ଠି_Pool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        ଅଚଳ size_t index = s_lfsr % POOL_SIZE;
        Ast_ଗଣ୍ଠି* ଅଚଳ objp = static_cast<Ast_ଗଣ୍ଠି*>(s_ଗଣ୍ଠି_Pool[index]);
        s_ଗଣ୍ଠି_Pool[index] = ::operator ନୂତନ(ASTGEN_MAX_ଗଣ୍ଠି_SIZE + 64);  // For later new()
        ଫେରନ୍ତୁ objp;
    } ଅନ୍ୟଥା {
        Ast_ଗଣ୍ଠି* ଅଚଳ objp = static_cast<Ast_ଗଣ୍ଠି*>(::operator ନୂତନ(size));
        ଫେରନ୍ତୁ objp;
    }
}
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::operator delete(ଶୂନ୍ୟ* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (ଶୂନ୍ୟ)objp;
    (ଶୂନ୍ୟ)size;
}
#endif

//======================================================================
// Iterators

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ଯଦି (m_op1p) m_op1p->iterateAndNext(v);
    ଯଦି (m_op2p) m_op2p->iterateAndNext(v);
    ଯଦି (m_op3p) m_op3p->iterateAndNext(v);
    ଯଦି (m_op4p) m_op4p->iterateAndNext(v);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    ଯଦି (m_op1p) m_op1p->iterateAndNextConst(v);
    ଯଦି (m_op2p) m_op2p->iterateAndNextConst(v);
    ଯଦି (m_op3p) m_op3p->iterateAndNextConst(v);
    ଯଦି (m_op4p) m_op4p->iterateAndNextConst(v);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    Ast_ଗଣ୍ଠି* nodep = ଏହି;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    ଯଦି (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    ଯେତେବେଳେ (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        ଯଦି (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        Ast_ଗଣ୍ଠି* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
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
        ଯଦି (!niterp) ଫେରନ୍ତୁ;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        ଯଦି (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } ଅନ୍ୟଥା {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::iterateListBackwardsConst(VNVisitorConst& v) {
    Ast_ଗଣ୍ଠି* nodep = ଏହି;
    ଯେତେବେଳେ (nodep->m_nextp) nodep = nodep->m_nextp;
    ଯେତେବେଳେ (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        ଯଦି (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } ଅନ୍ୟଥା {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    ଯଦି (m_op1p) m_op1p->iterateListBackwardsConst(v);
    ଯଦି (m_op2p) m_op2p->iterateListBackwardsConst(v);
    ଯଦି (m_op3p) m_op3p->iterateListBackwardsConst(v);
    ଯଦି (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    Ast_ଗଣ୍ଠି* nodep = ଏହି;
    କର {
        Ast_ଗଣ୍ଠି* ଅଚଳ nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } ଯେତେବେଳେ (nodep);
}

Ast_ଗଣ୍ଠି* Ast_ଗଣ୍ଠି::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    Ast_ଗଣ୍ଠି* nodep = ଏହି;  // Note "this" may point to bogus point later in this function
    ଯଦି (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } ଅନ୍ୟଥା ଯଦି (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* ଅଚଳ tempp = ନୂତନ AstBegin{nodep->fileline(), "[EditWrapper]", nodep, ମିଥ୍ୟା};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->delete_ବୃକ୍ଷ(), tempp);
    } ଅନ୍ୟଥା {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        Ast_ଗଣ୍ଠି** nextnodepp = nullptr;
        ଯଦି (ଏହି->m_backp->m_op1p == ଏହି) {
            nextnodepp = &(ଏହି->m_backp->m_op1p);
        } ଅନ୍ୟଥା ଯଦି (ଏହି->m_backp->m_op2p == ଏହି) {
            nextnodepp = &(ଏହି->m_backp->m_op2p);
        } ଅନ୍ୟଥା ଯଦି (ଏହି->m_backp->m_op3p == ଏହି) {
            nextnodepp = &(ଏହି->m_backp->m_op3p);
        } ଅନ୍ୟଥା ଯଦି (ଏହି->m_backp->m_op4p == ଏହି) {
            nextnodepp = &(ଏହି->m_backp->m_op4p);
        } ଅନ୍ୟଥା ଯଦି (ଏହି->m_backp->m_nextp == ଏହି) {
            nextnodepp = &(ଏହି->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, ଏହି, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    ଫେରନ୍ତୁ nodep;
}

//======================================================================

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::clone_Relink_ବୃକ୍ଷ() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = ଏହି; nodep; nodep = nodep->m_nextp) {
        ଯଦି (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        ଯଦି (nodep->m_op1p) nodep->m_op1p->clone_Relink_ବୃକ୍ଷ();
        ଯଦି (nodep->m_op2p) nodep->m_op2p->clone_Relink_ବୃକ୍ଷ();
        ଯଦି (nodep->m_op3p) nodep->m_op3p->clone_Relink_ବୃକ୍ଷ();
        ଯଦି (nodep->m_op4p) nodep->m_op4p->clone_Relink_ବୃକ୍ଷ();
    }
}

//======================================================================
// Comparison

ତାର୍କିକ Ast_ଗଣ୍ଠି::gate_ବୃକ୍ଷ_Iter() ଅଚଳ {
    // private: Return true if the two trees are identical
    ଯଦି (!isGateOptimizable()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (m_op1p && !m_op1p->gate_ବୃକ୍ଷ_Iter()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (m_op2p && !m_op2p->gate_ବୃକ୍ଷ_Iter()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (m_op3p && !m_op3p->gate_ବୃକ୍ଷ_Iter()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (m_op4p && !m_op4p->gate_ବୃକ୍ଷ_Iter()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

ତାର୍କିକ Ast_ଗଣ୍ଠି::same_ବୃକ୍ଷ_Iter(ଅଚଳ Ast_ଗଣ୍ଠି* node1p, ଅଚଳ Ast_ଗଣ୍ଠି* node2p, ତାର୍କିକ ignNext,
                           ତାର୍କିକ gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    ଯେତେବେଳେ (ସତ୍ୟ) {
        ଯଦି (!node1p && !node2p) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (!node1p || !node2p) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (node1p->type() != node2p->type()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        ଯଦି (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!node1p->same_ଗଣ୍ଠି(node2p) || (gateOnly && !node1p->isGateOptimizable())) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (!same_ବୃକ୍ଷ_Iter(node1p->m_op1p, node2p->m_op1p, ମିଥ୍ୟା, gateOnly)
            || !same_ବୃକ୍ଷ_Iter(node1p->m_op2p, node2p->m_op2p, ମିଥ୍ୟା, gateOnly)
            || !same_ବୃକ୍ଷ_Iter(node1p->m_op3p, node2p->m_op3p, ମିଥ୍ୟା, gateOnly)
            || !same_ବୃକ୍ଷ_Iter(node1p->m_op4p, node2p->m_op4p, ମିଥ୍ୟା, gateOnly)) {
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଯଦି (ignNext) ଫେରନ୍ତୁ ସତ୍ୟ;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::check_ବୃକ୍ଷ_Iter(ଅଚଳ Ast_ଗଣ୍ଠି* prevBackp) ଅଚଳ VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == ଏହି->backp(), ଏହି, "Back node inconsistent");
    ଅଚଳ VNTypeInfo& typeInfo = VNType::typeInfo(ଏହି->type());
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 1; i <= 4; i++) {
        Ast_ଗଣ୍ଠି* nodep = nullptr;
        ବାଛ (i) {
        ସ୍ଥିତି 1: nodep = op1p(); ଭାଙ୍ଗ;
        ସ୍ଥିତି 2: nodep = op2p(); ଭାଙ୍ଗ;
        ସ୍ଥିତି 3: nodep = op3p(); ଭାଙ୍ଗ;
        ସ୍ଥିତି 4: nodep = op4p(); ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଏହି->v3fatalSrc("Bad case"); ଭାଙ୍ଗ;
        }
        // cppcheck-suppress danglingTempReference
        ଅଚଳ ଅକ୍ଷର* opName = typeInfo.m_opNamep[i - 1];
        ବାଛ (typeInfo.m_opType[i - 1]) {
        ସ୍ଥିତି VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, ଏହି, typeInfo.m_namep << " must not use " << opName << "()");
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, ଏହି,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), ଏହି,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->check_ବୃକ୍ଷ_Iter(ଏହି);
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VNTypeInfo::OP_LIST:
            ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ headp = nodep) {
                // cppcheck-suppress shadowFunction
                ଅଚଳ Ast_ଗଣ୍ଠି* backp = ଏହି;
                ଅଚଳ Ast_ଗଣ୍ଠି* tailp;
                ଅଚଳ Ast_ଗଣ୍ଠି* opp = headp;
                କର {
                    opp->check_ବୃକ୍ଷ_Iter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } ଯେତେବେଳେ (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VNTypeInfo::OP_OPTIONAL:
            ଯଦି (nodep) {
                UASSERT_OBJ(!nodep->nextp(), ଏହି,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->check_ବୃକ୍ଷ_Iter(ଏହି);
            }
            ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ: ଏହି->v3fatalSrc("Bad case"); ଭାଙ୍ଗ;
        }
    }
    ଯଦି (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ exprp = VN_CAST(ଏହି, ଗଣ୍ଠି_ପଦସମଷ୍ଟି)) {
            ଅଚଳ ଅକ୍ଷର* ଅଚଳ whyp = exprp->widthMismatch();
            ଯଦି (whyp) {
                auto dtypeStr = [](ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    ଫେରନ୍ତୁ ss.str();
                };
                ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Uniop* ଅଚଳ uniopp = VN_CAST(exprp, ଗଣ୍ଠି_Uniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } ଅନ୍ୟଥା ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Biop* ଅଚଳ biopp = VN_CAST(exprp, ଗଣ୍ଠି_Biop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } ଅନ୍ୟଥା {
                    UASSERT_OBJ(ମିଥ୍ୟା, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
ଅକ୍ଷର* Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_Json_Gdb(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
    ଯଦି (!nodep) ଫେରନ୍ତୁ strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dump_ବୃକ୍ଷ_Json(nodepStream);
    ଅଚଳ std::string str = nodepStream.rdbuf()->str();
    ଫେରନ୍ତୁ strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
ଅକ୍ଷର* Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_Json_Gdb(ଅଚଳ ଅକ୍ଷର* str) { ଫେରନ୍ତୁ strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
ଅକ୍ଷର* Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_Json_Gdb(intptr_t nodep) {
    ଯଦି (!nodep) ଫେରନ୍ତୁ strdup("{\"addr\":\"NULL\"}\n");
    ଫେରନ୍ତୁ dump_ବୃକ୍ଷ_Json_Gdb(reinterpret_cast<ଅଚଳ Ast_ଗଣ୍ଠି*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dumpGdb(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {  // For GDB only  // LCOV_EXCL_START
    ଯଦି (!nodep) {
        cout << "<nullptr>" << endl;
        ଫେରନ୍ତୁ;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dumpGdbHeader() ଅଚଳ {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_Gdb(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {  // For GDB only  // LCOV_EXCL_START
    ଯଦି (!nodep) {
        cout << "<nullptr>" << endl;
        ଫେରନ୍ତୁ;
    }
    nodep->dumpGdbHeader();
    nodep->dump_ବୃକ୍ଷ(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_File_Gdb(ଅଚଳ Ast_ଗଣ୍ଠି* nodep,  // LCOV_EXCL_START
                              ଅଚଳ ଅକ୍ଷର* filenamep) {  // For GDB only
    ଯଦି (!nodep) {
        cout << "<nullptr>" << endl;
        ଫେରନ୍ତୁ;
    }
    ଅଚଳ string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dump_ବୃକ୍ଷ_File(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::checkIter() ଅଚଳ {
    ଯଦି (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        ଏହି->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dumpPtrs(std::ostream& os) ଅଚଳ {
    os << "This=" << typeName() << " " << cvtToHex(ଏହି);
    os << " back=" << cvtToHex(backp());
    ଯଦି (nextp()) os << " next=" << cvtToHex(nextp());
    ଯଦି (m_headtailp == ଏହି) {
        os << " headtail=this";
    } ଅନ୍ୟଥା {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    ଯଦି (op1p()) os << " op1p=" << cvtToHex(op1p());
    ଯଦି (op2p()) os << " op2p=" << cvtToHex(op2p());
    ଯଦି (op3p()) os << " op3p=" << cvtToHex(op3p());
    ଯଦି (op4p()) os << " op4p=" << cvtToHex(op4p());
    ଅଚଳ auto dumpUser = [&os](ଅଚଳ ଅକ୍ଷର* prefix, ଅଚଳ VNUser& user) {
        ଅଚଳ std::string s
            = user.dumpStr([](ଅଚଳ ଶୂନ୍ୟ* p) -> std::string { ଫେରନ୍ତୁ cvtToHex(p); });
        ଯଦି (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    ଯଦି (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ(std::ostream& os, ଅଚଳ string& indent, ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxDepth) ଅଚଳ {
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << ଏହି << '\n';
    ଯଦି (VN_DELETED(ଏହି)) ଫେରନ୍ତୁ;
    ଯଦି (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    ଯଦି (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(ଏହି)
           << endl;  // endl intentional to do flush
        ଫେରନ୍ତୁ;
    }  // LCOV_EXCL_STOP
    ଯଦି (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    ଯଦି (maxDepth == 1) {
        ଯଦି (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } ଅନ୍ୟଥା {
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ବୃକ୍ଷ(os, indent + "1:", maxDepth - 1);
        }
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ବୃକ୍ଷ(os, indent + "2:", maxDepth - 1);
        }
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ବୃକ୍ଷ(os, indent + "3:", maxDepth - 1);
        }
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dump_ବୃକ୍ଷ(os, indent + "4:", maxDepth - 1);
        }
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_And_Next(std::ostream& os, ଅଚଳ string& indent, ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxDepth) ଅଚଳ {
    // Audited to make sure this is never nullptr
    ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = ଏହି; nodep; nodep = nodep->nextp()) {
        nodep->dump_ବୃକ୍ଷ(os, indent, maxDepth);
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_File(ଅଚଳ string& filename, ତାର୍କିକ doDump) {
    // Not const function as calls checkTree
    ଯଦି (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            ଅଚଳ std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            ଯଦି (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << edit_ଗଣନା_Last();
            *logsp << "> to <e" << std::dec << edit_ଗଣନା_Gbl() << ">\n";
            ଯଦି (edit_ଗଣନା_Gbl() == edit_ଗଣନା_Last() && ::dump_ବୃକ୍ଷ_Level() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } ଅନ୍ୟଥା {
                dump_ବୃକ୍ଷ(*logsp);
                edit_ଗଣନା_Set_Last();  // Next dump can indicate start from here
            }
        }
    }
}

ସ୍ଥିର ଶୂନ୍ୟ drawChildren(std::ostream& os, ଅଚଳ Ast_ଗଣ୍ଠି* thisp, ଅଚଳ Ast_ଗଣ୍ଠି* childp,
                         ଅଚଳ std::string& childName) {
    ଯଦି (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dump_ବୃକ୍ଷ_Dot(os);
            ଯଦି (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_Dot(std::ostream& os) ଅଚଳ {
    os << "\tn" << cvtToHex(ଏହି) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, ଏହି, m_op1p, "op1");
    drawChildren(os, ଏହି, m_op2p, "op2");
    drawChildren(os, ଏହି, m_op3p, "op3");
    drawChildren(os, ଏହି, m_op4p, "op4");
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_Json_File(ଅଚଳ string& filename, ତାର୍କିକ doDump) {
    ଯଦି (!doDump) ଫେରନ୍ତୁ;
    UINFO(2, "Dumping " << filename);
    ଅଚଳ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ଯଦି (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dump_ବୃକ୍ଷ_Json(*treejsonp);
    *treejsonp << '\n';
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dumpJsonMetaFileGdb(ଅଚଳ ଅକ୍ଷର* filename) { dumpJsonMetaFile(filename); }
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dumpJsonMetaFile(ଅଚଳ string& filename) {
    UINFO(2, "Dumping " << filename);
    ଅଚଳ std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    ଯଦି (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    File_ଧାଡ଼ି::file_Name_ଅଙ୍କ_Map_Dump_Json(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dump_ବୃକ୍ଷ_Dot_File(ଅଚଳ string& filename, ତାର୍କିକ doDump) {
    ଯଦି (doDump) {
        UINFO(2, "Dumping " << filename);
        ଅଚଳ std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        ଯଦି (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dump_ବୃକ୍ଷ_Dot(*treedotp);
        *treedotp << "}\n";
    }
}

string Ast_ଗଣ୍ଠି::instanceStr() ଅଚଳ {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iter_ଗଣନା = 0;
    // Walk 'backp' chain
    ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* currp = ଏହି; currp; currp = currp->backp(), ++iter_ଗଣନା) {
        ଯଦି (VL_UNCOVERABLE(iter_ଗଣନା >= maxIterations)) ଫେରନ୍ତୁ "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        ଯଦି (ଅଚଳ AstScope* ଅଚଳ scopep = VN_CAST(currp, Scope)) {
            ଫେରନ୍ତୁ scopep->is_ଶୀର୍ଷ() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        ଯଦି (ଅଚଳ AstModule* ଅଚଳ modp = VN_CAST(currp, Module)) {
            ଅଚଳ string instanceName = modp->someInstanceName();
            ଫେରନ୍ତୁ instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    ଫେରନ୍ତୁ "";
}
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::v3errorEnd(ଅଚଳ std::ostringstream& str) ଅଚଳ VL_RELEASE(V3Error::s().m_mutex) {
    ଯଦି (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } ଅନ୍ୟଥା {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        ଅଚଳ string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        ଯଦି (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<Ast_ଗଣ୍ଠି*>(ଏହି)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::v3errorEndFatal(ଅଚଳ std::ostringstream& str) ଅଚଳ
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    ପ୍ରତିଜ୍ଞା(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dtypeChgSigned(ତାର୍କିକ flag) {
    UASSERT_OBJ(dtypep(), ଏହି, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dtypeChgWidth(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin) {
    UASSERT_OBJ(dtypep(), ଏହି, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

ଶୂନ୍ୟ Ast_ଗଣ୍ଠି::dtypeChgWidthSigned(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), ଏହି, "No dtype when changing width");
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ enumDTypep = dtypep()->skipRefToEnump();
    ଯଦି (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        ଫେରନ୍ତୁ;  // Correct already
    }
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findBasicDType(VBasicDTypeKwd kwd) ଅଚଳ {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findBitDType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findLogicDType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findLogic2StateDType(ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric) ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findLogicRangeDType(ଅଚଳ VNumRange& range, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin,
                                           VSigning numeric) ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findBitRangeDType(ଅଚଳ VNumRange& range, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin,
                                         VSigning numeric) ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* Ast_ଗଣ୍ଠି::find_ଭର୍ତ୍ତି_Same_DType(AstBasicDType* nodep) {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->find_ଭର୍ତ୍ତି_Same_DType(nodep);
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findConstraintRefDType() ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findEmptyQueueDType() ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findQueueIndexDType() ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findStreamDType() ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::findVoidDType() ଅଚଳ {
    ଫେରନ୍ତୁ v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

ସ୍ଥିର ଅଚଳ Ast_ଗଣ୍ଠି_DType* computeCastableBase(ଅଚଳ Ast_ଗଣ୍ଠି_DType* nodep) {
    ଯେତେବେଳେ (ସତ୍ୟ) {
        ଯଦି (ଅଚଳ AstPackArrayDType* ଅଚଳ packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            ଜାରି;
        }
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ refp = nodep->skipRefToEnump()) {
            ଯଦି (refp != nodep) {
                nodep = refp;
                ଜାରି;
            }
        }
        ଫେରନ୍ତୁ nodep;
    }
}

ସ୍ଥିର VCastable computeCastableImp(ଅଚଳ Ast_ଗଣ୍ଠି_DType* toDtp, ଅଚଳ Ast_ଗଣ୍ଠି_DType* fromDtp,
                                    ଅଚଳ Ast_ଗଣ୍ଠି* fromConstp, ଅଚଳ ତାର୍କିକ checkIfaceArgCompat) {
    ଅଚଳ VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    ଯଦି (toDtp == fromDtp) ଫେରନ୍ତୁ VCastable::SAMEISH;
    ଯଦି (toDtp->similarDType(fromDtp)) ଫେରନ୍ତୁ VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    ଅଚଳ Ast_ଗଣ୍ଠି_DType* fromBaseDtp = computeCastableBase(fromDtp);

    ଅଚଳ ତାର୍କିକ fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, ଗଣ୍ଠି_UOr_Struct_DType);

    ଅଚଳ Ast_ଗଣ୍ଠି_DType* toBaseDtp = computeCastableBase(toDtp);
    ଅଚଳ ତାର୍କିକ toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, ଗଣ୍ଠି_UOr_Struct_DType);

    ଯଦି (toBaseDtp == fromBaseDtp) ଫେରନ୍ତୁ VCastable::COMPATIBLE;

    ଯଦି (toNumericable) {
        ଯଦି (fromNumericable) ଫେରନ୍ତୁ VCastable::COMPATIBLE;
    } ଅନ୍ୟଥା ଯଦି (VN_IS(toBaseDtp, EnumDType)) {
        ଯଦି (VN_IS(fromBaseDtp, EnumDType) && toDtp->same_ବୃକ୍ଷ(fromDtp))
            ଫେରନ୍ତୁ VCastable::ENUM_IMPLICIT;
        ଯଦି (fromNumericable) ଫେରନ୍ତୁ VCastable::ENUM_EXPLICIT;
    } ଅନ୍ୟଥା ଯଦି (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        ଫେରନ୍ତୁ VCastable::COMPATIBLE;
    } ଅନ୍ୟଥା ଯଦି (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        ଯଦି (fromConstp->isNull()) ଫେରନ୍ତୁ VCastable::COMPATIBLE;
    } ଅନ୍ୟଥା ଯଦି (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        ଅଚଳ AstClass* ଅଚଳ toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        ଅଚଳ AstClass* ଅଚଳ fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        ଅଚଳ ତାର୍କିକ downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        ଅଚଳ ତାର୍କିକ upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        ଯଦି (upcast) ଫେରନ୍ତୁ VCastable::COMPATIBLE;
        ଯଦି (downcast) ଫେରନ୍ତୁ VCastable::DYNAMIC_CLASS;
        ଫେରନ୍ତୁ VCastable::INCOMPATIBLE;
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstIfaceRefDType* ଅଚଳ toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(ସତ୍ୟ) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        ଅଚଳ AstIfaceRefDType* ଅଚଳ fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(ସତ୍ୟ) : fromDtp, IfaceRefDType);
        ଯଦି (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            ଅଚଳ AstUnpackArrayDType* ଅଚଳ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            ଯଦି (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                ଫେରନ୍ତୁ VCastable::INCOMPATIBLE;
            }
            ଫେରନ୍ତୁ computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        ଯଦି (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            ଯଦି (!checkIfaceArgCompat) ଫେରନ୍ତୁ castable;
            ଫେରନ୍ତୁ VCastable::INCOMPATIBLE;
        }
        ଅଚଳ ତାର୍କିକ sameModport = toIfp->modportp() == fromIfp->modportp();
        ଯଦି (!checkIfaceArgCompat) {
            ଯଦି (!toIfp->modportp() || sameModport) ଫେରନ୍ତୁ VCastable::COMPATIBLE;
            ଫେରନ୍ତୁ castable;
        }
        ଯଦି (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            ଫେରନ୍ତୁ VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        ଯଦି (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            ଫେରନ୍ତୁ VCastable::COMPATIBLE;
        }
        ଫେରନ୍ତୁ VCastable::INCOMPATIBLE;
    } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstUnpackArrayDType* ଅଚଳ toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        ଅଚଳ AstUnpackArrayDType* ଅଚଳ fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        ଯଦି (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            ଫେରନ୍ତୁ castable;
        }
        ଅଚଳ VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ toSubp = toArrayp->subDTypep()->skipRefp();
        ଯଦି (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            ଫେରନ୍ତୁ VCastable::COMPATIBLE;
        }
    }
    ଫେରନ୍ତୁ castable;
}

VCastable Ast_ଗଣ୍ଠି::computeCastable(ଅଚଳ Ast_ଗଣ୍ଠି_DType* toDtp, ଅଚଳ Ast_ଗଣ୍ଠି_DType* fromDtp,
                                   ଅଚଳ Ast_ଗଣ୍ଠି* fromConstp, ଅଚଳ ତାର୍କିକ checkIfaceArgCompat) {
    ଅଚଳ auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    ଯଦି (fromConstp) UINFO(9, "     const= " << fromConstp);
    ଫେରନ୍ତୁ castable;
}

Ast_ଗଣ୍ଠି_DType* Ast_ଗଣ୍ଠି::getCommonClassTypep(Ast_ଗଣ୍ଠି* node1p, Ast_ଗଣ୍ଠି* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    ଯଦି (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        ଅଚଳ VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        ଯଦି (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            ଫେରନ୍ତୁ node1p->dtypep();
        ଯଦି (castable == VCastable::DYNAMIC_CLASS) ଫେରନ୍ତୁ node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    ଯେତେବେଳେ (classDtypep1) {
        ଅଚଳ VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        ଯଦି (castable == VCastable::COMPATIBLE) ଫେରନ୍ତୁ classDtypep1;
        AstClassExtends* ଅଚଳ extendsp = classDtypep1->classp()->extendsp();
        ଯଦି (!extendsp) ଭାଙ୍ଗ;
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    ଫେରନ୍ତୁ nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

ଶ୍ରେଣୀ VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, std::string> m_internedWordWidths;  // Interned widths
    std::map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    ଅଚଳ ତାର୍କିକ m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    ସ୍ଥିର std::string toLetters(size_t ମୂଲ୍ୟ, ତାର୍କିକ lowerCase = ମିଥ୍ୟା) {
        ଅଚଳ ଅକ୍ଷର base = lowerCase ? 'a' : 'A';
        std::string s;
        କର { s += static_cast<ଅକ୍ଷର>(base + ମୂଲ୍ୟ % 26); } ଯେତେବେଳେ (ମୂଲ୍ୟ /= 26);
        ଫେରନ୍ତୁ s;
    }

    ଅଚଳ std::string& internConst(ଅଚଳ AstConst& nodep) {
        ଅଚଳ auto pair = m_internedConsts.emplace(nodep.ଅଙ୍କ().ascii(ମିଥ୍ୟା), "");
        ଯଦି (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        ଫେରନ୍ତୁ pair.first->second;
    }

    ଅଚଳ std::string& internWordWidth(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) {
        ଅଚଳ auto pair = m_internedWordWidths.emplace(ମୂଲ୍ୟ, "");
        ଯଦି (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, ସତ୍ୟ);
        ଫେରନ୍ତୁ pair.first->second;
    }

    ଅଚଳ std::string& internWideWidth(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) {
        ଅଚଳ auto pair = m_internedWideWidths.emplace(ମୂଲ୍ୟ, "");
        ଯଦି (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        ଫେରନ୍ତୁ pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    ସ୍ଥିର ତାର୍କିକ isLeaf(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        ଅଚଳ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        ପାଇଁ (ଅଚଳ VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            ଯଦି (opType != VNTypeInfo::OP_UNUSED) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Render operand, return true if not unused
    ଶୂନ୍ୟ renderOperand(VNTypeInfo::OpEn opType, ଅଚଳ Ast_ଗଣ୍ଠି* ଅଚଳ opp, uint32_t depth) {
        ବାଛ (opType) {
        ସ୍ଥିତି VNTypeInfo::OP_UNUSED:  //
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VNTypeInfo::OP_LIST:
            m_os << " [";
            ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* nodep = opp; nodep; nodep = nodep->nextp()) {
                ଯଦି (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            ଭାଙ୍ଗ;
        ସ୍ଥିତି VNTypeInfo::OP_OPTIONAL:
            ଯଦି (opp) {
                m_os << " ";
                render(opp, depth);
            } ଅନ୍ୟଥା {
                m_os << " nil";
            }
            ଭାଙ୍ଗ;
        }
    }

    // Render the node into the stream.
    ଶୂନ୍ୟ render(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, uint32_t depth) {
        ଯଦି (ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            ଯଦି (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } ଅନ୍ୟଥା ଯଦି (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } ଅନ୍ୟଥା {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } ଅନ୍ୟଥା ଯଦି (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } ଅନ୍ୟଥା ଯଦି (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } ଅନ୍ୟଥା {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            ଅଚଳ VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        ଯଦି (!dtypep) {
            m_os << '?';
        } ଅନ୍ୟଥା ଯଦି (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } ଅନ୍ୟଥା {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = nodep->width();
            ଯଦି (width == 1) {
                m_os << '1';
            } ଅନ୍ୟଥା ଯଦି (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } ଅନ୍ୟଥା {
                m_os << internWideWidth(width);
            }
            ଯଦି (m_dumpWidthMin) {
                m_os << '/';
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin = nodep->widthMin();
                ଯଦି (widthMin == 1) {
                    m_os << '1';
                } ଅନ୍ୟଥା ଯଦି (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } ଅନ୍ୟଥା {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

ସର୍ବସାଧାରଣ:
    VNPatternString(std::ostream& os, ଅଚଳ Ast_ଗଣ୍ଠି* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି::patternString(uint32_t depth) ଅଚଳ {
    std::ostringstream oss;
    VNPatternString{oss, ଏହି, depth};
    ଫେରନ୍ତୁ oss.str();
}

//######################################################################
// VNDeleter

ଶୂନ୍ୟ VNDeleter::doDeletes() {
    ପାଇଁ (Ast_ଗଣ୍ଠି* ଅଚଳ nodep : m_deleteps) nodep->delete_ବୃକ୍ଷ();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#ଅନ୍ତର୍ଭାଵ "V3Ast__gen_visitor_defns.h"  // From ./astgen
