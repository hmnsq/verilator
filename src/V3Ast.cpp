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

#include "V3PchAstMT.h"

#include "V3Broken.h"
#include "V3File.h"

#include <iomanip>
#include <memory>
#include <sstream>

VL_DEFINE_DEBUG_FUNCTIONS;

//======================================================================
// Statics

uint64_t VIsCached::s_cachedCntGbl = 1;

uint64_t AstNode::s_editCntLast = 0;
uint64_t AstNode::s_editCntGbl = 0;  // Hot cache line

// To allow for fast clearing of all user pointers, we keep a "timestamp"
// along with each userp, and thus by bumping this count we can make it look
// as if we iterated across the entire tree to set all the userp's to null.
số_nguyên AstNode::s_cloneCntGbl = 0;
uint32_t VNUser1InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser2InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser3InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent
uint32_t VNUser4InUse::s_userCntGbl = 0;  // Hot cache line, leave adjacent

bool VNUser1InUse::s_userBusy = false;
bool VNUser2InUse::s_userBusy = false;
bool VNUser3InUse::s_userBusy = false;
bool VNUser4InUse::s_userBusy = false;

số_nguyên AstNodeDType::s_uniqueNum = 0;

//======================================================================
// VCMethod information

VCMethod VCMethod::arrayMethod(const string& name) {
    cho (số_nguyên i = 0; i < _ENUM_MAX; ++i) {
        const VCMethod method{i};
        nếu (name == method.ascii()) trả_về method;
    }
    v3fatalSrc("Not a method name known to VCMethod: '" << name << '\'');
    trả_về VCMethod{};
}

//######################################################################
// VNUser

std::string VNUser::dumpStr(std::string (*fmtAddrp)(const void*)) const {
#ifdef VL_USER_TYPE_CHECKS
    nếu (const uint64_t* const uip = std::get_if<uint64_t>(&m_u)) trả_về "#"s + cvtToStr(*uip);
    nếu (void* const* const upp = std::get_if<void*>(&m_u)) trả_về fmtAddrp(*upp);
    trả_về "";
#không_thì
    // Dumps void* representation
    nếu (!m_u.up) trả_về "";
    trả_về fmtAddrp(m_u.up);
#endif
}

//######################################################################
// VNType

const VNTypeInfo VNType::s_typeInfoTable[VNType::NUM_TYPES()] = {
#include "V3Ast__gen_type_info.h"  // From ./astgen
};

std::ostream& operator<<(std::ostream& os, VNType rhs);

//######################################################################
// VFwdType

bool VFwdType::isNodeCompatible(const AstNode* nodep) const {
    const AstNode* defp = nodep;
    nếu (const AstTypedef* const adefp = VN_CAST(defp, Typedef)) defp = adefp->subDTypep();
    nếu (const AstNodeDType* const adefp = VN_CAST(defp, NodeDType))
        defp = adefp->skipRefToNonRefp();
    switch (m_e) {
    case VFwdType::NONE: trả_về true; break;
    case VFwdType::ENUM: trả_về VN_IS(defp, EnumDType); break;
    case VFwdType::STRUCT: trả_về VN_IS(defp, StructDType); break;
    case VFwdType::UNION: trả_về VN_IS(defp, UnionDType); break;
    case VFwdType::INTERFACE_CLASS:  // FALLTHRU  // TODO: Over permissive for now
    case VFwdType::CLASS: trả_về VN_IS(defp, ClassRefDType) || VN_IS(defp, Class); break;
    case VFwdType::GENERIC_INTERFACE: trả_về VN_IS(defp, IfaceRefDType); break;
    default: v3fatalSrc("Bad case");
    }
    VL_UNREACHABLE;
    trả_về false;  // LCOV_EXCL_LINE
}

//######################################################################
// VSelfPointerText

const std::shared_ptr<const string> VSelfPointerText::s_emptyp = std::make_shared<string>("");
const std::shared_ptr<const string> VSelfPointerText::s_thisp = std::make_shared<string>("this");

string VSelfPointerText::replaceThis(bool useSelfForThis, const string& text) {
    trả_về useSelfForThis ? VString::replaceWord(text, "this", "vlSelf") : text;
}

string VSelfPointerText::protect(bool useSelfForThis, bool protect) const {
    trả_về VIdProtect::protectWordsIf(replaceThis(useSelfForThis, asString()), protect);
}

//######################################################################
// AstNode

AstNode::AstNode(VNType t, FileLine* fl)
    : m_type{t}
    , m_fileline{fl} {
    m_headtailp = this;  // When made, we're a list of only a single element
    // Attributes
    m_flags.didWidth = false;
    m_flags.doingWidth = false;
    m_flags.protect = true;
    m_flags.unused = 0;  // Initializing this avoids a read-modify-write on construction
    editCountInc();
}

AstNode* AstNode::abovep() const {
    // m_headtailp only valid at beginning or end of list
    // Avoid supporting at other locations as would require walking
    // list which is likely to cause performance issues.
    UASSERT_OBJ(!m_nextp || firstAbovep(), this, "abovep() not allowed when in midlist");
    const AstNode* const firstp = firstAbovep() ? this : m_headtailp;
    trả_về firstp->backp();
}
AstNode* AstNode::aboveLoopp() const {
    // Returns parent node. Avoid using this, may have performance issues.
    const AstNode* nodep = this;
    // Backwards over peers (versus parents)
    trong_khi (nodep->backp() && nodep->backp()->nextp() == nodep) nodep = nodep->backp();
    trả_về nodep->backp();
}

string AstNode::encodeName(const string& namein) {
    // Encode signal name raw from parser, then not called again on same signal
    string out;
    out.reserve(namein.size());
    cho (auto pos = namein.begin(); pos != namein.end(); ++pos) {
        nếu ((pos == namein.begin()) ? std::isalpha(pos[0])  // digits can't lead identifiers
                                    : std::isalnum(pos[0])) {
            out += pos[0];
        } không_thì nếu (pos[0] == '_') {
            out += pos[0];
            nếu (pos + 1 == namein.end()) break;
            nếu (pos[1] == '_') {
                ++pos;
                out += "__05F";  // hex(_) = 0x5F
            }
        } không_thì {
            // Need the leading 0 so this will never collide with
            // a user identifier nor a temp we create in Verilator.
            // We also do *NOT* use __DOT__ etc, as we search for those
            // in some replacements, and don't want to mangle the user's names.
            const unsigned val = pos[0] & 0xff;  // Mask to avoid sign extension
            std::stringstream hex;
            hex << std::setfill('0') << std::setw(2) << std::hex << val;
            out += "__0" + hex.str();
        }
    }
    // Shorten names
    // TODO long term use VName in place of "string name"
    // Then we also won't need to save the table of hashed values
    VName vname{out};
    trả_về vname.hashedName();
}

string AstNode::encodeNumber(int64_t num) {
    nếu (num < 0) trả_về "__02D" + cvtToStr(-num);  // 2D=-
    trả_về cvtToStr(num);
}

string AstNode::nameProtect() const { trả_về VIdProtect::protectIf(name(), protect()); }
string AstNode::origNameProtect() const { trả_về VIdProtect::protectIf(origName(), protect()); }

string AstNode::shortName() const {
    string pretty = name();
    string::size_type pos;
    trong_khi ((pos = pretty.find("__PVT__")) != string::npos) pretty.replace(pos, 7, "");
    trả_về pretty;
}

string AstNode::dedotName(const string& namein) {
    string pretty = namein;
    string::size_type pos;
    trong_khi ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, ".");
    nếu (pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    trả_về pretty;
}

string AstNode::nameNoArray(const string& namein) {
    trả_về namein.substr(0, namein.find("__BRA__"));
}

string AstNode::vcdName(const string& namein) {
    // VCD tracing expects space to separate hierarchy
    // Dots are reserved for dots the user put in the name
    // We earlier hashed all symbols, dehash them so user sees real name
    string pretty{VName::dehash(namein)};
    string::size_type pos;
    trong_khi ((pos = pretty.find("__DOT__")) != string::npos) pretty.replace(pos, 7, " ");
    trong_khi ((pos = pretty.find('.')) != string::npos) pretty.replace(pos, 1, " ");
    // Now convert escaped special characters, etc
    trả_về prettyName(pretty);
}

string AstNode::prettyName(const string& namein) VL_PURE {
    // This function is somewhat hot, so we short-circuit some compares
    string pretty;
    pretty.reserve(namein.length());
    cho (const char* pos = namein.c_str(); *pos;) {
        nếu (pos[0] == '-' && pos[1] == '>') {  // ->
            pretty += ".";
            pos += 2;
            continue;
        }
        nếu (pos[0] == '_' && pos[1] == '_') {  // Short-circuit
            nếu (0 == std::strncmp(pos, "__BRA__", 7)) {
                pretty += "[";
                pos += 7;
                continue;
            }
            nếu (0 == std::strncmp(pos, "__KET__", 7)) {
                pretty += "]";
                pos += 7;
                continue;
            }
            nếu (0 == std::strncmp(pos, "__DOT__", 7)) {
                pretty += ".";
                pos += 7;
                continue;
            }
            nếu (0 == std::strncmp(pos, "__LIB__", 7)) {
                pretty = "";  // Trim library name before module name
                pos += 7;
                continue;
            }
            nếu (0 == std::strncmp(pos, "__PVT__", 7)) {
                pretty += "";
                pos += 7;
                continue;
            }
            nếu (0 == std::strncmp(pos, "__Viftop", 8)) {
                pretty += "";
                pos += 8;
                continue;
            }
            nếu (pos[0] == '_' && pos[1] == '_' && pos[2] == '0' && std::isxdigit(pos[3])
                && std::isxdigit(pos[4])) {
                char value = 0;
                value += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                value
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));
                pretty += value;
                pos += 5;
                continue;
            }
        }
        // Default
        pretty += pos[0];
        ++pos;
    }
    nếu (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    nếu (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    trả_về pretty;
}

string AstNode::vpiName(const string& namein) {
    // This is slightly different from prettyName, in that when we encounter escaped characters,
    // we change that identifier to an escaped identifier, wrapping it with '\' and ' '
    // as specified in LRM 23.6
    const size_t offset = VString::startsWith(namein, "__SYM__") ? 7 : 0;
    string pretty;
    pretty.reserve(namein.length());
    bool inEscapedIdent = false;
    số_nguyên lastIdent = 0;

    cho (const char* pos = namein.c_str() + offset; *pos;) {
        char specialChar = 0;
        nếu (pos[0] == '-' && pos[1] == '>') {  // ->
            specialChar = '.';
            pos += 2;
        } không_thì nếu (pos[0] == '_' && pos[1] == '_') {  // __
            nếu (0 == std::strncmp(pos, "__BRA__", 7)) {
                specialChar = '[';
                pos += 7;
            } không_thì nếu (0 == std::strncmp(pos, "__KET__", 7)) {
                specialChar = ']';
                pos += 7;
            } không_thì nếu (0 == std::strncmp(pos, "__DOT__", 7)) {
                specialChar = '.';
                pos += 7;
            } không_thì nếu (0 == std::strncmp(pos, "__PVT__", 7)) {
                pos += 7;
                continue;
            } không_thì nếu (0 == std::strncmp(pos, "__0", 3) && std::isxdigit(pos[3])
                       && std::isxdigit(pos[4])) {
                char value = 0;
                value += 16
                         * (std::isdigit(pos[3]) ? (pos[3] - '0')
                                                 : (std::tolower(pos[3]) - 'a' + 10));
                value
                    += (std::isdigit(pos[4]) ? (pos[4] - '0') : (std::tolower(pos[4]) - 'a' + 10));

                // __ doesn't always imply escaped ident
                nếu (value != '_') inEscapedIdent = true;

                pretty += value;
                pos += 5;
                continue;
            }
        } không_thì nếu (pos[0] == '.') {
            specialChar = '.';
            ++pos;
        }

        nếu (specialChar) {
            nếu (inEscapedIdent && (specialChar == '[' || specialChar == '.')) {
                pretty += " ";
                pretty.insert(lastIdent, "\\");
                inEscapedIdent = false;
            }

            pretty += specialChar;

            nếu (specialChar == ']' || specialChar == '.') {
                lastIdent = pretty.length();
                inEscapedIdent = false;
            }
        } không_thì {
            pretty += pos[0];
            ++pos;
        }
    }
    nếu (inEscapedIdent) {
        pretty += " ";
        pretty.insert(lastIdent, "\\");
    }
    nếu (pretty[0] == 'T' && pretty.substr(0, 4) == "TOP.") pretty.replace(0, 4, "");
    nếu (pretty[0] == 'T' && pretty.substr(0, 5) == "TOP->") pretty.replace(0, 5, "");
    trả_về pretty;
}

string AstNode::prettyTypeName() const {
    nếu (name() == "") trả_về typeName();
    trả_về std::string{typeName()} + " '" + prettyNameMsg() + "'";
}

//######################################################################
// Insertion

void AstNode::debugTreeChange(const AstNode* nodep, const char* prefix, số_nguyên lineno, bool next) {
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
    debugTreeChange(nodep, "-addNextThs: ", __LINE__, false);
    debugTreeChange(newp, "-addNextNew: ", __LINE__, true);
    nếu (!nodep)  // verilog.y and lots of other places assume this
        trả_về newp;

    // Find end of old list
    AstNode* oldtailp = nodep;
    nếu (oldtailp->m_nextp) {
        nếu (oldtailp->m_headtailp) {
            oldtailp = oldtailp->m_headtailp;  // This=beginning of list, jump to end
            UDEBUGONLY(UASSERT_OBJ(!oldtailp->m_nextp, nodep,
                                   "Node had next, but headtail says it shouldn't"););
        } không_thì {
            // Though inefficient, we are occasionally passed an
            // addNext in the middle of a list.
            trong_khi (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
    }
    // Link it in
    oldtailp->m_nextp = newp;
    newp->m_backp = oldtailp;
    // New tail needs the head
    AstNode* const newtailp = newp->m_headtailp;
    AstNode* const headp = oldtailp->m_headtailp;
    oldtailp->m_headtailp = nullptr;  // May be written again as new head
    newp->m_headtailp = nullptr;  // May be written again as new tail
    newtailp->m_headtailp = headp;
    headp->m_headtailp = newtailp;
    newp->editCountInc();
    // No change of m_iterpp, as only changing m_nextp of current node;
    // the current node is still the one at the iteration point

    debugTreeChange(nodep, "-addNextOut:", __LINE__, true);
    trả_về nodep;
}

void AstNode::addNextHere(AstNode* newp) {
    // Add to m_nextp on exact node passed, not at the end.
    //  'this' could be at head, tail, or both (single)
    //  'newp' could be head of single node, or list
    UASSERT(newp, "Null item passed to addNext");
    UASSERT_OBJ(!newp->backp(), newp, "New node (back) already assigned?");
    debugTreeChange(this, "-addHereThs: ", __LINE__, false);
    debugTreeChange(newp, "-addHereNew: ", __LINE__, true);
    newp->editCountInc();

    AstNode* const addlastp = newp->m_headtailp;  // Last node in list to be added
    UASSERT_OBJ(!addlastp->m_nextp, addlastp, "Headtailp tail isn't at the tail");

    // Forward links
    AstNode* const oldnextp = this->m_nextp;
    this->m_nextp = newp;
    addlastp->m_nextp = oldnextp;  // Perhaps null if 'this' is not list

    // Backward links
    nếu (oldnextp) oldnextp->m_backp = addlastp;
    newp->m_backp = this;

    // Head/tail
    AstNode* const oldheadtailp = this->m_headtailp;
    //    (!oldheadtailp)               // this was&is middle of list
    //    (oldheadtailp==this && !oldnext)// this was head AND tail (one node long list)
    //    (oldheadtailp && oldnextp)    // this was&is head of list of not just one node, not
    //    tail (oldheadtailp && !oldnextp)   // this was tail of list, might also
    //                                     be head of one-node list
    //
    newp->m_headtailp = nullptr;  // Not at head any longer
    addlastp->m_headtailp = nullptr;  // Presume middle of list
    // newp might happen to be head/tail after all, if so will be set again below
    nếu (oldheadtailp) {  // else in middle of list, no change
        nếu (oldheadtailp == this) {  // this was one node
            this->m_headtailp = addlastp;  // Was head/tail, now a tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        } không_thì nếu (!oldnextp) {  // this was tail
            this->m_headtailp = nullptr;  // No longer a tail
            oldheadtailp->m_headtailp = addlastp;  // Head gets new tail
            addlastp->m_headtailp = oldheadtailp;  // Tail needs to remember head (or nullptr)
        }  // else is head, and we're inserting into the middle, so no other change
    }

    // No change of m_iterpp, as adding after current node;
    // the current node is still the one at the iteration point
    debugTreeChange(this, "-addHereOut: ", __LINE__, true);
}

void AstNode::setOp1p(AstNode* newp) {
    UASSERT(newp, "Null item passed to setOp1p");
    UDEBUGONLY(UASSERT_OBJ(!m_op1p, this, "Adding to non-empty, non-list op1"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op1"););
    debugTreeChange(this, "-setOp1pThs: ", __LINE__, false);
    debugTreeChange(newp, "-setOp1pNew: ", __LINE__, true);
    m_op1p = newp;
    newp->editCountInc();
    newp->m_backp = this;
    debugTreeChange(this, "-setOp1pOut: ", __LINE__, false);
}

void AstNode::setOp2p(AstNode* newp) {
    UASSERT(newp, "Null item passed to setOp2p");
    UDEBUGONLY(UASSERT_OBJ(!m_op2p, this, "Adding to non-empty, non-list op2"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op2"););
    debugTreeChange(this, "-setOp2pThs: ", __LINE__, false);
    debugTreeChange(newp, "-setOp2pNew: ", __LINE__, true);
    m_op2p = newp;
    newp->editCountInc();
    newp->m_backp = this;
    debugTreeChange(this, "-setOp2pOut: ", __LINE__, false);
}

void AstNode::setOp3p(AstNode* newp) {
    UASSERT(newp, "Null item passed to setOp3p");
    UDEBUGONLY(UASSERT_OBJ(!m_op3p, this, "Adding to non-empty, non-list op3"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op3"););
    debugTreeChange(this, "-setOp3pThs: ", __LINE__, false);
    debugTreeChange(newp, "-setOp3pNew: ", __LINE__, true);
    m_op3p = newp;
    newp->editCountInc();
    newp->m_backp = this;
    debugTreeChange(this, "-setOp3pOut: ", __LINE__, false);
}

void AstNode::setOp4p(AstNode* newp) {
    UASSERT(newp, "Null item passed to setOp4p");
    UDEBUGONLY(UASSERT_OBJ(!m_op4p, this, "Adding to non-empty, non-list op4"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    UDEBUGONLY(UASSERT_OBJ(!newp->m_nextp, newp, "Adding list to non-list op4"););
    debugTreeChange(this, "-setOp4pThs: ", __LINE__, false);
    debugTreeChange(newp, "-setOp4pNew: ", __LINE__, true);
    m_op4p = newp;
    newp->editCountInc();
    newp->m_backp = this;
    debugTreeChange(this, "-setOp4pOut: ", __LINE__, false);
}

void AstNode::addOp1p(AstNode* newp) {
    UASSERT(newp, "Null item passed to addOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    nếu (!m_op1p) {
        op1p(newp);
    } không_thì {
        m_op1p->addNext(newp);
    }
}

void AstNode::addOp2p(AstNode* newp) {
    UASSERT(newp, "Null item passed to addOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    nếu (!m_op2p) {
        op2p(newp);
    } không_thì {
        m_op2p->addNext(newp);
    }
}

void AstNode::addOp3p(AstNode* newp) {
    UASSERT(newp, "Null item passed to addOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    nếu (!m_op3p) {
        op3p(newp);
    } không_thì {
        m_op3p->addNext(newp);
    }
}

void AstNode::addOp4p(AstNode* newp) {
    UASSERT(newp, "Null item passed to addOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    nếu (!m_op4p) {
        op4p(newp);
    } không_thì {
        m_op4p->addNext(newp);
    }
}

void AstNode::prependOp1p(AstNode* newp) {
    UASSERT(newp, "Null item passed to prependOp1p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    nếu (!m_op1p) {
        op1p(newp);
    } không_thì {
        m_op1p->addHereThisAsNext(newp);
    }
}

void AstNode::prependOp2p(AstNode* newp) {
    UASSERT(newp, "Null item passed to prependOp2p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    nếu (!m_op2p) {
        op2p(newp);
    } không_thì {
        m_op2p->addHereThisAsNext(newp);
    }
}

void AstNode::prependOp3p(AstNode* newp) {
    UASSERT(newp, "Null item passed to prependOp3p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    nếu (!m_op3p) {
        op3p(newp);
    } không_thì {
        m_op3p->addHereThisAsNext(newp);
    }
}

void AstNode::prependOp4p(AstNode* newp) {
    UASSERT(newp, "Null item passed to prependOp4p");
    UDEBUGONLY(UASSERT_OBJ(!newp->m_backp, newp, "Adding already linked node"););
    nếu (!m_op4p) {
        op4p(newp);
    } không_thì {
        m_op4p->addHereThisAsNext(newp);
    }
}

void AstNode::replaceWith(AstNode* newp) {
    // Replace oldp with this
    // Unlike a unlink/relink, children are changed to point to the new node.
    VNRelinker repHandle;
    this->unlinkFrBack(&repHandle);
    repHandle.relink(newp);
}
void AstNode::replaceWithKeepDType(AstNode* newp) {
    newp->dtypeFrom(this);
    replaceWith(newp);
}

void VNRelinker::dump(std::ostream& str) const {
    str << " BK=" << reinterpret_cast<uint32_t*>(m_backp);
    str << " ITER=" << reinterpret_cast<uint32_t*>(m_iterpp);
    str << " CHG=" << (m_chg == RELINK_NEXT ? "[NEXT] " : "");
    str << (m_chg == RELINK_OP1 ? "[OP1] " : "");
    str << (m_chg == RELINK_OP2 ? "[OP2] " : "");
    str << (m_chg == RELINK_OP3 ? "[OP3] " : "");
    str << (m_chg == RELINK_OP4 ? "[OP4] " : "");
}

AstNode* AstNode::unlinkFrBackWithNext(VNRelinker* linkerp) {
    debugTreeChange(this, "-unlinkWNextThs: ", __LINE__, true);
    AstNode* const oldp = this;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->editCountInc();
    // cppcheck-suppress shadowFunction
    AstNode* const backp = oldp->m_backp;
    nếu (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        linkerp->m_iterpp = oldp->m_iterpp;
        nếu (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } không_thì nếu (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } không_thì nếu (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } không_thì nếu (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } không_thì nếu (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } không_thì {
            oldp->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    nếu (backp->m_nextp == oldp) {
        backp->m_nextp = nullptr;
        // Old list gets truncated
        // New list becomes a list upon itself
        // Most common case is unlinking a entire operand tree, or all but the
        // head (else we'd probably call unlinkFrBack without next)
        AstNode* oldtailp = backp->m_headtailp;
        nếu (!oldtailp) {
            // We are in the middle of a list; we have no way to find head or tail in O(1)
            oldtailp = oldp;
            trong_khi (oldtailp->m_nextp) oldtailp = oldtailp->m_nextp;
        }
        // Create new head/tail of old list
        AstNode* const oldheadp = oldtailp->m_headtailp;
        oldheadp->m_headtailp = oldp->m_backp;
        oldheadp->m_headtailp->m_headtailp = oldheadp;
        // Create new head/tail of extracted list
        oldp->m_headtailp = oldtailp;
        oldp->m_headtailp->m_headtailp = oldp;
    } không_thì nếu (backp->m_op1p == oldp) {
        backp->m_op1p = nullptr;
    } không_thì nếu (backp->m_op2p == oldp) {
        backp->m_op2p = nullptr;
    } không_thì nếu (backp->m_op3p == oldp) {
        backp->m_op3p = nullptr;
    } không_thì nếu (backp->m_op4p == oldp) {
        backp->m_op4p = nullptr;
    } không_thì {
        this->v3fatalSrc("Unlink of node with back not pointing to it.");
    }
    // Relink
    oldp->m_backp = nullptr;
    // Iterator fixup
    nếu (oldp->m_iterpp) {
        *(oldp->m_iterpp) = nullptr;
        oldp->m_iterpp = nullptr;
    }
    debugTreeChange(oldp, "-unlinkWNextOut: ", __LINE__, true);
    trả_về oldp;
}

AstNode* AstNode::unlinkFrBack(VNRelinker* linkerp) {
    debugTreeChange(this, "-unlinkFrBkThs: ", __LINE__, true);
    AstNode* const oldp = this;
    UASSERT_OBJ(oldp->m_backp, oldp, "Node has no back, already unlinked?");
    oldp->editCountInc();
    // cppcheck-suppress shadowFunction
    AstNode* const backp = oldp->m_backp;
    nếu (linkerp) {
        linkerp->m_oldp = oldp;
        linkerp->m_backp = backp;
        nếu (oldp->m_iterpp) {  // Assumes we will always relink() if want to keep iterating
            linkerp->m_iterpp = oldp->m_iterpp;
            *(oldp->m_iterpp) = nullptr;
            oldp->m_iterpp = nullptr;
        }
        nếu (backp->m_nextp == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_NEXT;
        } không_thì nếu (backp->m_op1p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP1;
        } không_thì nếu (backp->m_op2p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP2;
        } không_thì nếu (backp->m_op3p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP3;
        } không_thì nếu (backp->m_op4p == oldp) {
            linkerp->m_chg = VNRelinker::RELINK_OP4;
        } không_thì {
            this->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
    }
    nếu (backp->m_nextp == oldp) {
        // This node gets removed from middle (or tail) of list
        // Not head, since then oldp wouldn't be a next of backp...
        backp->m_nextp = oldp->m_nextp;
        nếu (backp->m_nextp) backp->m_nextp->m_backp = backp;
        // If it was a tail, back becomes new tail
        nếu (oldp->m_headtailp) {
            backp->m_headtailp = oldp->m_headtailp;
            backp->m_headtailp->m_headtailp = backp;
        }
    } không_thì {
        nếu (backp->m_op1p == oldp) {
            backp->m_op1p = oldp->m_nextp;
        } không_thì nếu (backp->m_op2p == oldp) {
            backp->m_op2p = oldp->m_nextp;
        } không_thì nếu (backp->m_op3p == oldp) {
            backp->m_op3p = oldp->m_nextp;
        } không_thì nếu (backp->m_op4p == oldp) {
            backp->m_op4p = oldp->m_nextp;
        } không_thì {
            this->v3fatalSrc("Unlink of node with back not pointing to it.");
        }
        nếu (oldp->m_nextp) {
            AstNode* const newheadp = oldp->m_nextp;
            newheadp->m_backp = backp;
            newheadp->m_headtailp = oldp->m_headtailp;
            newheadp->m_headtailp->m_headtailp = newheadp;
        }
    }
    // Iterator fixup
    nếu (oldp->m_iterpp) {  // Only if no linker, point to next in list
        nếu (oldp->m_nextp) oldp->m_nextp->m_iterpp = oldp->m_iterpp;
        *(oldp->m_iterpp) = oldp->m_nextp;
        oldp->m_iterpp = nullptr;
    }
    // Relink
    oldp->m_nextp = nullptr;
    oldp->m_backp = nullptr;
    oldp->m_headtailp = oldp;
    debugTreeChange(oldp, "-unlinkFrBkOut: ", __LINE__, true);
    trả_về oldp;
}

void AstNode::relink(VNRelinker* linkerp) {
    nếu (debug() > 8) {
        UINFO_PREFIX(" EDIT:      relink: ");
        dumpPtrs();
    }
    AstNode* const newp = this;
    UASSERT(linkerp && linkerp->m_backp, "Need non-empty linker");
    UASSERT_OBJ(!newp->m_backp, newp, "New node already linked?");
    newp->editCountInc();

    nếu (debug() > 8) {
        linkerp->dump(cout);
        cout << endl;
    }

    // cppcheck-suppress shadowFunction
    AstNode* const backp = linkerp->m_backp;
    debugTreeChange(this, "-relinkNew: ", __LINE__, true);
    debugTreeChange(backp, "-relinkTre: ", __LINE__, true);

    switch (linkerp->m_chg) {
    case VNRelinker::RELINK_NEXT: backp->addNextHere(newp); break;
    case VNRelinker::RELINK_OP1: relinkOneLink(backp->m_op1p /*ref*/, newp); break;
    case VNRelinker::RELINK_OP2: relinkOneLink(backp->m_op2p /*ref*/, newp); break;
    case VNRelinker::RELINK_OP3: relinkOneLink(backp->m_op3p /*ref*/, newp); break;
    case VNRelinker::RELINK_OP4: relinkOneLink(backp->m_op4p /*ref*/, newp); break;
    default: this->v3fatalSrc("Relink of node without any link to change."); break;
    }
    // Relink
    newp->m_backp = backp;
    linkerp->m_backp = nullptr;
    // Iterator fixup
    nếu (linkerp->m_iterpp) {
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
    debugTreeChange(this, "-relinkOut: ", __LINE__, true);
}

void AstNode::relinkOneLink(AstNode*& pointpr,  // Ref to pointer that gets set to newp
                            AstNode* newp) {
    nếu (pointpr) {
        // We know there will be at least two elements when we are done,
        //    (newp & the old list).
        // We *ALLOW* the new node to have its own next list.
        // Likewise there may be a old list.
        // Insert the whole old list following the new node's list.
        // Thus a unlink without next, followed by relink, gives the same list.
        AstNode* const newlistlastp = newp->m_headtailp;
        UASSERT_OBJ(!(newlistlastp->m_nextp && newlistlastp != newp), newp,
                    "Headtailp tail isn't at the tail");
        AstNode* const oldlistlastp = pointpr->m_headtailp;
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

void AstNode::addHereThisAsNext(AstNode* newp) {
    // {back}->this->{next} becomes {back}->new->this->{next}
    UASSERT_OBJ(!newp->backp(), newp, "New node already linked?");
    UASSERT_OBJ(this->m_backp, this, "'this' node has no back, already unlinked?");
    UASSERT_OBJ(newp->m_headtailp, newp, "m_headtailp not set on new node");
    //
    // cppcheck-suppress shadowFunction
    AstNode* const backp = this->m_backp;
    AstNode* const newLastp = newp->m_headtailp;
    //
    this->editCountInc();
    // Common linkage
    newLastp->m_nextp = this;
    this->m_backp = newLastp;
    newp->m_backp = backp;
    // newLastp will not be the last node in the list as 'this' will follow it.
    // If newLastp == newp, then below handles newp becoming head
    newLastp->m_headtailp = nullptr;
    // Linkage dependent on position
    nếu (backp && backp->m_nextp == this) {
        // If 'this' is not at the head of a list, then the new node will also not be at the head
        // of a list, so we can just link in the new node in the middle.
        backp->m_nextp = newp;
        newp->m_headtailp = nullptr;
    } không_thì {
        // If 'this' is at the head of a list, then the new node becomes the head of that list.
        nếu (backp) {
            nếu (backp->m_op1p == this) {
                backp->m_op1p = newp;
            } không_thì nếu (backp->m_op2p == this) {
                backp->m_op2p = newp;
            } không_thì nếu (backp->m_op3p == this) {
                backp->m_op3p = newp;
            } không_thì {
                UASSERT_OBJ(backp->m_op4p == this, this, "Don't know where newp should go");
                backp->m_op4p = newp;
            }
        }
        // We also need to update m_headtailp.
        AstNode* const tailp = this->m_headtailp;
        this->m_headtailp = nullptr;
        newp->m_headtailp = tailp;
        tailp->m_headtailp = newp;
    }
    // Iterator fixup
    nếu (newLastp->m_iterpp) *(newLastp->m_iterpp) = this;
    nếu (this->m_iterpp) {
        *(this->m_iterpp) = newp;
        this->m_iterpp = nullptr;
    }
    //
    debugTreeChange(this, "-addHereThisAsNext: ", __LINE__, true);
}

//======================================================================
// Clone

AstNode* AstNode::cloneTreeIter(bool needPure) {
    // private: Clone single node and children
    nếu (needPure) purityCheck();
    AstNode* const newp = this->clone();
    nếu (this->m_op1p) newp->op1p(this->m_op1p->cloneTreeIterList(needPure));
    nếu (this->m_op2p) newp->op2p(this->m_op2p->cloneTreeIterList(needPure));
    nếu (this->m_op3p) newp->op3p(this->m_op3p->cloneTreeIterList(needPure));
    nếu (this->m_op4p) newp->op4p(this->m_op4p->cloneTreeIterList(needPure));
    newp->m_iterpp = nullptr;
    newp->clonep(this);  // Save pointers to/from both to simplify relinking.
    this->clonep(newp);  // Save pointers to/from both to simplify relinking.
    trả_về newp;
}

AstNode* AstNode::cloneTreeIterList(bool needPure) {
    // private: Clone list of nodes, set m_headtailp
    AstNode* newheadp = nullptr;
    AstNode* newtailp = nullptr;
    // Audited to make sure this is never nullptr
    cho (AstNode* oldp = this; oldp; oldp = oldp->m_nextp) {
        AstNode* const newp = oldp->cloneTreeIter(needPure);
        newp->m_headtailp = nullptr;
        newp->m_backp = newtailp;
        nếu (newtailp) newtailp->m_nextp = newp;
        nếu (!newheadp) newheadp = newp;
        newtailp = newp;
    }
    newheadp->m_headtailp = newtailp;
    newtailp->m_headtailp = newheadp;
    trả_về newheadp;
}

AstNode* AstNode::cloneTree(bool cloneNextLink, bool needPure) {
    debugTreeChange(this, "-cloneThs: ", __LINE__, cloneNextLink);
    cloneClearTree();
    AstNode* newp;
    nếu (cloneNextLink && this->m_nextp) {
        newp = cloneTreeIterList(needPure);
    } không_thì {
        newp = cloneTreeIter(needPure);
        newp->m_nextp = nullptr;
        newp->m_headtailp = newp;
    }
    newp->m_backp = nullptr;
    newp->cloneRelinkTree();
    debugTreeChange(newp, "-cloneOut: ", __LINE__, true);
    trả_về newp;
}

void AstNode::purityCheck() {
    nếu (VL_UNLIKELY(!isPure())) {
        this->v3warn(SIDEEFFECT,
                     "Expression side effect may be mishandled\n"
                         << this->warnMore()
                         << "... Suggest use a temporary variable in place of this expression");
        // this->v3fatalSrc("cloneTreePure debug backtrace");  // Comment in to debug where caused
    }
}

//======================================================================
// Delete

void AstNode::deleteNode() {
    // private: Delete single node. Publicly call deleteTree() instead.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    editCountInc();
    // Change links of old node so we coredump if used
    this->m_nextp = reinterpret_cast<AstNode*>(0x1);
    this->m_backp = reinterpret_cast<AstNode*>(0x1);
    this->m_headtailp = reinterpret_cast<AstNode*>(0x1);
    this->m_op1p = reinterpret_cast<AstNode*>(0x1);
    this->m_op2p = reinterpret_cast<AstNode*>(0x1);
    this->m_op3p = reinterpret_cast<AstNode*>(0x1);
    this->m_op4p = reinterpret_cast<AstNode*>(0x1);
    this->m_iterpp = reinterpret_cast<AstNode**>(0x1);
    nếu (
#nếu !defined(VL_DEBUG) || defined(VL_LEAK_CHECKS)
        true
#không_thì
        !v3Global.opt.debugLeak()
#endif
    ) {
        delete this;
    }
    // Else leak massively, so each pointer is unique
    // and we can debug easier.
}

void AstNode::deleteTreeIter() {
    // private: Delete list of nodes. Publicly call deleteTree() instead.
    // Audited to make sure this is never nullptr
    cho (AstNode *nodep = this, *nnextp; nodep; nodep = nnextp) {
        nnextp = nodep->m_nextp;
        // MUST be depth first!
        nếu (nodep->m_op1p) nodep->m_op1p->deleteTreeIter();
        nếu (nodep->m_op2p) nodep->m_op2p->deleteTreeIter();
        nếu (nodep->m_op3p) nodep->m_op3p->deleteTreeIter();
        nếu (nodep->m_op4p) nodep->m_op4p->deleteTreeIter();
        nodep->m_nextp = nullptr;
        nodep->m_backp = nullptr;
        nodep->deleteNode();
    }
}

void AstNode::deleteTree() {
    // deleteTree always deletes the next link, because you must have called
    // unlinkFromBack or unlinkFromBackWithNext as appropriate before calling this.
    UASSERT(!m_backp, "Delete called on node with backlink still set");
    debugTreeChange(this, "-delTree:  ", __LINE__, true);
    this->editCountInc();
    // MUST be depth first!
    deleteTreeIter();
}

//======================================================================
// Memory checks

#ifdef VL_LEAK_CHECKS
void* AstNode::operator new(size_t size) {
    // Optimization note: Aligning to cache line is a loss, due to lost packing
    AstNode* const objp = static_cast<AstNode*>(::operator new(size));
    V3Broken::addNewed(objp);
    trả_về objp;
}

void AstNode::operator delete(void* objp, size_t size) {
    nếu (!objp) trả_về;
    const AstNode* const nodep = static_cast<AstNode*>(objp);
    V3Broken::deleted(nodep);
    ::operator delete(objp);
}
#endif

#ifdef VL_ALLOC_RANDOM_CHECKS
void* AstNode::operator new(size_t size) {  // VL_MT_SAFE
    // Compute the maximum node size once and cache it
    static const size_t ASTGEN_MAX_NODE_SIZE = []() {
        size_t maxSize = 0;
        cho (size_t t = 0; t < VNType::NUM_TYPES(); ++t) {
            maxSize = std::max(maxSize, VNType::typeInfo(static_cast<VNType::en>(t)).m_sizeof);
        }
        trả_về maxSize;
    }();

    // Make the following small to debug this routine, larger for performance and better random
    constexpr size_t POOL_SIZE = 65536;  // Ideally large enough to fit all nodes used in tests
    // Randomly select from a large pool (POOL_SIZE) of max-node sized (MAX_NODE_SIZE) pointers
    static uint64_t s_lfsr = 0;  // LFSR, 0 = didn't initialize yet
    nếu (số_nguyên seed = v3Global.opt.debugAllocRandom()) {
        static V3Mutex s_mutex;
        const V3LockGuard lock{s_mutex};
        static std::array<void*, POOL_SIZE> s_nodePool VL_GUARDED_BY(s_mutex);
        constexpr uint64_t POLYNOMIAL = 0x80000000000019e2ULL;
        UASSERT_STATIC(size <= ASTGEN_MAX_NODE_SIZE, "fix ASTGEN_MAX_NODE_SIZE");
        nếu (!s_lfsr) {
            s_lfsr = seed;
            nếu (!s_lfsr) s_lfsr = ~s_lfsr;
            cho (size_t i = 0; i < POOL_SIZE; ++i) {
                s_nodePool[i] = ::operator new(ASTGEN_MAX_NODE_SIZE + 64);
            }
            // Sort, just to make it more obvious we are properly randomizing
            std::sort(std::begin(s_nodePool), std::end(s_nodePool));
        }
        // Xoroshiro128+ algorithm
        s_lfsr = (s_lfsr & 1ULL) ? ((s_lfsr >> 1ULL) ^ POLYNOMIAL) : (s_lfsr >> 1ULL);
        const size_t index = s_lfsr % POOL_SIZE;
        AstNode* const objp = static_cast<AstNode*>(s_nodePool[index]);
        s_nodePool[index] = ::operator new(ASTGEN_MAX_NODE_SIZE + 64);  // For later new()
        trả_về objp;
    } không_thì {
        AstNode* const objp = static_cast<AstNode*>(::operator new(size));
        trả_về objp;
    }
}
void AstNode::operator delete(void* objp, size_t size) {
    // Leak due to size difference between true node and MAX_NODE_SIZE
    (void)objp;
    (void)size;
}
#endif

//======================================================================
// Iterators

void AstNode::iterateChildren(VNVisitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    nếu (m_op1p) m_op1p->iterateAndNext(v);
    nếu (m_op2p) m_op2p->iterateAndNext(v);
    nếu (m_op3p) m_op3p->iterateAndNext(v);
    nếu (m_op4p) m_op4p->iterateAndNext(v);
}

void AstNode::iterateChildrenConst(VNVisitorConst& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    nếu (m_op1p) m_op1p->iterateAndNextConst(v);
    nếu (m_op2p) m_op2p->iterateAndNextConst(v);
    nếu (m_op3p) m_op3p->iterateAndNextConst(v);
    nếu (m_op4p) m_op4p->iterateAndNextConst(v);
}

void AstNode::iterateAndNext(VNVisitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    AstNode* nodep = this;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    nếu (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    trong_khi (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        nếu (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
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
        nếu (!niterp) trả_về;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        nếu (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } không_thì {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

void AstNode::iterateListBackwardsConst(VNVisitorConst& v) {
    AstNode* nodep = this;
    trong_khi (nodep->m_nextp) nodep = nodep->m_nextp;
    trong_khi (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        nếu (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } không_thì {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

void AstNode::iterateChildrenBackwardsConst(VNVisitorConst& v) {
    nếu (m_op1p) m_op1p->iterateListBackwardsConst(v);
    nếu (m_op2p) m_op2p->iterateListBackwardsConst(v);
    nếu (m_op3p) m_op3p->iterateListBackwardsConst(v);
    nếu (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

void AstNode::iterateAndNextConst(VNVisitorConst& v) {
    // Keep following the current list even if edits change it
    AstNode* nodep = this;
    do {
        AstNode* const nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } trong_khi (nodep);
}

AstNode* AstNode::iterateSubtreeReturnEdits(VNVisitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    AstNode* nodep = this;  // Note "this" may point to bogus point later in this function
    nếu (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } không_thì nếu (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* const tempp = new AstBegin{nodep->fileline(), "[EditWrapper]", nodep, false};
        // nodep to null as may be replaced
        VL_DO_DANGLING(tempp->stmtsp()->accept(v), nodep);
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->deleteTree(), tempp);
    } không_thì {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        AstNode** nextnodepp = nullptr;
        nếu (this->m_backp->m_op1p == this) {
            nextnodepp = &(this->m_backp->m_op1p);
        } không_thì nếu (this->m_backp->m_op2p == this) {
            nextnodepp = &(this->m_backp->m_op2p);
        } không_thì nếu (this->m_backp->m_op3p == this) {
            nextnodepp = &(this->m_backp->m_op3p);
        } không_thì nếu (this->m_backp->m_op4p == this) {
            nextnodepp = &(this->m_backp->m_op4p);
        } không_thì nếu (this->m_backp->m_nextp == this) {
            nextnodepp = &(this->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, this, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    trả_về nodep;
}

//======================================================================

void AstNode::cloneRelinkTree() {
    // private: Cleanup clone() operation on whole tree. Publicly call cloneTree() instead.
    cho (AstNode* nodep = this; nodep; nodep = nodep->m_nextp) {
        nếu (nodep->m_dtypep && nodep->m_dtypep->clonep()) {
            nodep->m_dtypep = nodep->m_dtypep->clonep();
        }
        nodep->cloneRelink();
        nếu (nodep->m_op1p) nodep->m_op1p->cloneRelinkTree();
        nếu (nodep->m_op2p) nodep->m_op2p->cloneRelinkTree();
        nếu (nodep->m_op3p) nodep->m_op3p->cloneRelinkTree();
        nếu (nodep->m_op4p) nodep->m_op4p->cloneRelinkTree();
    }
}

//======================================================================
// Comparison

bool AstNode::gateTreeIter() const {
    // private: Return true if the two trees are identical
    nếu (!isGateOptimizable()) trả_về false;
    nếu (m_op1p && !m_op1p->gateTreeIter()) trả_về false;
    nếu (m_op2p && !m_op2p->gateTreeIter()) trả_về false;
    nếu (m_op3p && !m_op3p->gateTreeIter()) trả_về false;
    nếu (m_op4p && !m_op4p->gateTreeIter()) trả_về false;
    trả_về true;
}

bool AstNode::sameTreeIter(const AstNode* node1p, const AstNode* node2p, bool ignNext,
                           bool gateOnly) {
    // private: Return true if the two trees are identical
    // Iterate over the list, rather than recurse, as lists might be long
    trong_khi (true) {
        nếu (!node1p && !node2p) trả_về true;
        nếu (!node1p || !node2p) trả_về false;
        nếu (node1p->type() != node2p->type()) trả_về false;
        UASSERT_OBJ(
            (!node1p->dtypep() && !node2p->dtypep()) || (node1p->dtypep() && node2p->dtypep()),
            node1p,
            "Comparison of a node with dtypep() with a node without dtypep()\n-node2=" << node2p);
        nếu (node1p->dtypep() && !node1p->dtypep()->similarDType(node2p->dtypep())) trả_về false;
        nếu (!node1p->sameNode(node2p) || (gateOnly && !node1p->isGateOptimizable())) trả_về false;
        nếu (!sameTreeIter(node1p->m_op1p, node2p->m_op1p, false, gateOnly)
            || !sameTreeIter(node1p->m_op2p, node2p->m_op2p, false, gateOnly)
            || !sameTreeIter(node1p->m_op3p, node2p->m_op3p, false, gateOnly)
            || !sameTreeIter(node1p->m_op4p, node2p->m_op4p, false, gateOnly)) {
            trả_về false;
        }
        nếu (ignNext) trả_về true;
        node1p = node1p->m_nextp;
        node2p = node2p->m_nextp;
    }
}

//======================================================================
// Debugging

void AstNode::checkTreeIter(const AstNode* prevBackp) const VL_MT_STABLE {
    // private: Check a tree and children
    UASSERT_OBJ(prevBackp == this->backp(), this, "Back node inconsistent");
    const VNTypeInfo& typeInfo = VNType::typeInfo(this->type());
    cho (số_nguyên i = 1; i <= 4; i++) {
        AstNode* nodep = nullptr;
        switch (i) {
        case 1: nodep = op1p(); break;
        case 2: nodep = op2p(); break;
        case 3: nodep = op3p(); break;
        case 4: nodep = op4p(); break;
        default: this->v3fatalSrc("Bad case"); break;
        }
        // cppcheck-suppress danglingTempReference
        const char* opName = typeInfo.m_opNamep[i - 1];
        switch (typeInfo.m_opType[i - 1]) {
        case VNTypeInfo::OP_UNUSED:
            UASSERT_OBJ(!nodep, this, typeInfo.m_namep << " must not use " << opName << "()");
            break;
        case VNTypeInfo::OP_USED:
            UASSERT_OBJ(nodep, this,
                        typeInfo.m_namep << " must have non-nullptr " << opName << "()");
            UASSERT_OBJ(!nodep->nextp(), this,
                        typeInfo.m_namep << "::" << opName
                                         << "() cannot have a non-nullptr nextp()");
            nodep->checkTreeIter(this);
            break;
        case VNTypeInfo::OP_LIST:
            nếu (const AstNode* const headp = nodep) {
                // cppcheck-suppress shadowFunction
                const AstNode* backp = this;
                const AstNode* tailp;
                const AstNode* opp = headp;
                do {
                    opp->checkTreeIter(backp);
                    UASSERT_OBJ(opp == headp || !opp->nextp() || !opp->m_headtailp, opp,
                                "Headtailp should be null in middle of lists");
                    backp = tailp = opp;
                    opp = opp->nextp();
                } trong_khi (opp);
                UASSERT_OBJ(headp->m_headtailp == tailp, headp,
                            "Tail in headtailp is inconsistent");
                UASSERT_OBJ(tailp->m_headtailp == headp, tailp,
                            "Head in headtailp is inconsistent");
            }
            break;
        case VNTypeInfo::OP_OPTIONAL:
            nếu (nodep) {
                UASSERT_OBJ(!nodep->nextp(), this,
                            typeInfo.m_namep << "::" << opName
                                             << "() cannot have a non-nullptr nextp()");
                nodep->checkTreeIter(this);
            }
            break;
        default: this->v3fatalSrc("Bad case"); break;
        }
    }
    nếu (v3Global.opt.debugWidth() && v3Global.widthMinUsage() == VWidthMinUsage::VERILOG_WIDTH) {
        nếu (const AstNodeExpr* const exprp = VN_CAST(this, NodeExpr)) {
            const char* const whyp = exprp->widthMismatch();
            nếu (whyp) {
                auto dtypeStr = [](const AstNodeExpr* exprp) VL_MT_STABLE {
                    std::ostringstream ss;
                    exprp->dtypep()->dumpSmall(ss);
                    trả_về ss.str();
                };
                nếu (const AstNodeUniop* const uniopp = VN_CAST(exprp, NodeUniop)) {
                    UASSERT_OBJ(!whyp, uniopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(uniopp)
                                                          << " LHS:" << dtypeStr(uniopp->lhsp()));
                } không_thì nếu (const AstNodeBiop* const biopp = VN_CAST(exprp, NodeBiop)) {
                    UASSERT_OBJ(!whyp, biopp,
                                "widthMismatch detected " << whyp << "OUT:" << dtypeStr(biopp)
                                                          << " LHS:" << dtypeStr(biopp->lhsp())
                                                          << " RHS:" << dtypeStr(biopp->rhsp()));
                } không_thì {
                    UASSERT_OBJ(false, exprp,
                                "widthMismatch detected " << whyp << " in an unexpected type");
                }
            }
        }
    }
}

// cppcheck-suppress unusedFunction  // Debug only
char* AstNode::dumpTreeJsonGdb(const AstNode* nodep) {
    nếu (!nodep) trả_về strdup("{\"addr\":\"NULL\"}\n");
    std::stringstream nodepStream;
    nodep->dumpTreeJson(nodepStream);
    const std::string str = nodepStream.rdbuf()->str();
    trả_về strdup(str.c_str());
}
// cppcheck-suppress unusedFunction  // Debug only
// identity func to allow for passing already done dumps to jtree
char* AstNode::dumpTreeJsonGdb(const char* str) { trả_về strdup(str); }
// cppcheck-suppress unusedFunction  // Debug only
// allow for passing pointer literals like 0x42.. without manual cast
char* AstNode::dumpTreeJsonGdb(intptr_t nodep) {
    nếu (!nodep) trả_về strdup("{\"addr\":\"NULL\"}\n");
    trả_về dumpTreeJsonGdb(reinterpret_cast<const AstNode*>(nodep));
}
// cppcheck-suppress unusedFunction  // Debug only
void AstNode::dumpGdb(const AstNode* nodep) {  // For GDB only  // LCOV_EXCL_START
    nếu (!nodep) {
        cout << "<nullptr>" << endl;
        trả_về;
    }
    nodep->dumpGdbHeader();
    cout << "  ";
    nodep->dump(cout);
    cout << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
void AstNode::dumpGdbHeader() const {  // For GDB only  // LCOV_EXCL_START
    dumpPtrs(cout);
    cout << "  Fileline = " << fileline() << endl;
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
void AstNode::dumpTreeGdb(const AstNode* nodep) {  // For GDB only  // LCOV_EXCL_START
    nếu (!nodep) {
        cout << "<nullptr>" << endl;
        trả_về;
    }
    nodep->dumpGdbHeader();
    nodep->dumpTree(cout);
}  // LCOV_EXCL_STOP
// cppcheck-suppress unusedFunction  // Debug only
void AstNode::dumpTreeFileGdb(const AstNode* nodep,  // LCOV_EXCL_START
                              const char* filenamep) {  // For GDB only
    nếu (!nodep) {
        cout << "<nullptr>" << endl;
        trả_về;
    }
    const string filename = filenamep ? filenamep : v3Global.debugFilename("debug.tree", 98);
    v3Global.rootp()->dumpTreeFile(filename);
}  // LCOV_EXCL_STOP

// cppcheck-suppress unusedFunction  // Debug only
void AstNode::checkIter() const {
    nếu (m_iterpp) {
        dumpPtrs(cout);
        // Perhaps something forgot to clear m_iterpp?
        this->v3fatalSrc("Iteration link m_iterpp should be nullptr");
    }
}

void AstNode::dumpPtrs(std::ostream& os) const {
    os << "This=" << typeName() << " " << cvtToHex(this);
    os << " back=" << cvtToHex(backp());
    nếu (nextp()) os << " next=" << cvtToHex(nextp());
    nếu (m_headtailp == this) {
        os << " headtail=this";
    } không_thì {
        os << " headtail=" << cvtToHex(m_headtailp);
    }
    nếu (op1p()) os << " op1p=" << cvtToHex(op1p());
    nếu (op2p()) os << " op2p=" << cvtToHex(op2p());
    nếu (op3p()) os << " op3p=" << cvtToHex(op3p());
    nếu (op4p()) os << " op4p=" << cvtToHex(op4p());
    const auto dumpUser = [&os](const char* prefix, const VNUser& user) {
        const std::string s
            = user.dumpStr([](const void* p) -> std::string { trả_về cvtToHex(p); });
        nếu (!s.empty()) os << prefix << s;
    };
    dumpUser(" user1p=", user1u());
    dumpUser(" user2p=", user2u());
    dumpUser(" user3p=", user3u());
    dumpUser(" user4p=", user4u());
    nếu (m_iterpp) {
        os << " iterpp=" << cvtToHex(m_iterpp);
        // This may cause address sanitizer failures as iterpp can be stale
        // os << "*=" << cvtToHex(*m_iterpp);
    }
    os << "\n";
}

void AstNode::dumpTree(std::ostream& os, const string& indent, số_nguyên maxDepth) const {
    static số_nguyên s_debugFileline = v3Global.opt.debugSrcLevel("fileline");  // --debugi-fileline 9
    os << indent << " " << this << '\n';
    nếu (VN_DELETED(this)) trả_về;
    nếu (debug() > 8) {
        os << indent << "     ";
        dumpPtrs(os);
    }
    nếu (VN_DELETED(op1p()) || VN_DELETED(op2p())  // LCOV_EXCL_START
        || VN_DELETED(op3p()) || VN_DELETED(op4p())) {
        os << indent << "1/2/3/4: %E-0x1/deleted! node " << cvtToHex(this)
           << endl;  // endl intentional to do flush
        trả_về;
    }  // LCOV_EXCL_STOP
    nếu (s_debugFileline >= 9) os << fileline()->warnContextSecondary();
    nếu (maxDepth == 1) {
        nếu (op1p() || op2p() || op3p() || op4p()) os << indent << "1: ...(maxDepth)\n";
    } không_thì {
        cho (const AstNode* nodep = op1p(); nodep; nodep = nodep->nextp()) {
            nodep->dumpTree(os, indent + "1:", maxDepth - 1);
        }
        cho (const AstNode* nodep = op2p(); nodep; nodep = nodep->nextp()) {
            nodep->dumpTree(os, indent + "2:", maxDepth - 1);
        }
        cho (const AstNode* nodep = op3p(); nodep; nodep = nodep->nextp()) {
            nodep->dumpTree(os, indent + "3:", maxDepth - 1);
        }
        cho (const AstNode* nodep = op4p(); nodep; nodep = nodep->nextp()) {
            nodep->dumpTree(os, indent + "4:", maxDepth - 1);
        }
    }
}

void AstNode::dumpTreeAndNext(std::ostream& os, const string& indent, số_nguyên maxDepth) const {
    // Audited to make sure this is never nullptr
    cho (const AstNode* nodep = this; nodep; nodep = nodep->nextp()) {
        nodep->dumpTree(os, indent, maxDepth);
    }
}

void AstNode::dumpTreeFile(const string& filename, bool doDump) {
    // Not const function as calls checkTree
    nếu (doDump) {
        {  // Write log & close
            UINFO(2, "Dumping " << filename);
            const std::unique_ptr<std::ofstream> logsp{V3File::new_ofstream(filename)};
            nếu (logsp->fail()) v3fatal("Can't write file: " << filename);
            *logsp << "Verilator Tree Dump (format 0x3900) from <e" << std::dec << editCountLast();
            *logsp << "> to <e" << std::dec << editCountGbl() << ">\n";
            nếu (editCountGbl() == editCountLast() && ::dumpTreeLevel() < 9) {
                *logsp << '\n';
                *logsp << "No changes since last dump!\n";
            } không_thì {
                dumpTree(*logsp);
                editCountSetLast();  // Next dump can indicate start from here
            }
        }
    }
}

static void drawChildren(std::ostream& os, const AstNode* thisp, const AstNode* childp,
                         const std::string& childName) {
    nếu (childp) {
        os << "\tn" << cvtToHex(thisp) << " -> n" << cvtToHex(childp) << " [" << "label=\""
           << childName << "\" color=red];\n";
        cho (const AstNode* nodep = childp; nodep; nodep = nodep->nextp()) {
            nodep->dumpTreeDot(os);
            nếu (nodep->nextp()) {
                os << "\tn" << cvtToHex(nodep) << " -> n" << cvtToHex(nodep->nextp()) << " ["
                   << "label=\"next\" color=red];\n";
                os << "\t{rank=same; n" << cvtToHex(nodep) << ", n" << cvtToHex(nodep->nextp())
                   << "}\n";
            }
        }
    }
}

void AstNode::dumpTreeDot(std::ostream& os) const {
    os << "\tn" << cvtToHex(this) << "\t[" << "label=\"" << typeName() << "\\n"
       << name() << "\"];\n";
    drawChildren(os, this, m_op1p, "op1");
    drawChildren(os, this, m_op2p, "op2");
    drawChildren(os, this, m_op3p, "op3");
    drawChildren(os, this, m_op4p, "op4");
}

void AstNode::dumpTreeJsonFile(const string& filename, bool doDump) {
    nếu (!doDump) trả_về;
    UINFO(2, "Dumping " << filename);
    const std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    nếu (treejsonp->fail()) v3fatal("Can't write file: " << filename);
    dumpTreeJson(*treejsonp);
    *treejsonp << '\n';
}

void AstNode::dumpJsonMetaFileGdb(const char* filename) { dumpJsonMetaFile(filename); }
void AstNode::dumpJsonMetaFile(const string& filename) {
    UINFO(2, "Dumping " << filename);
    const std::unique_ptr<std::ofstream> treejsonp{V3File::new_ofstream(filename)};
    nếu (treejsonp->fail()) v3fatalStatic("Can't write file: " << filename);
    *treejsonp << '{';
    FileLine::fileNameNumMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.idPtrMapDumpJson(*treejsonp);
    *treejsonp << ',';
    v3Global.ptrNamesDumpJson(*treejsonp);
    *treejsonp << "}\n";
}

void AstNode::dumpTreeDotFile(const string& filename, bool doDump) {
    nếu (doDump) {
        UINFO(2, "Dumping " << filename);
        const std::unique_ptr<std::ofstream> treedotp{V3File::new_ofstream(filename)};
        nếu (treedotp->fail()) v3fatal("Can't write file: " << filename);
        *treedotp << "digraph vTree{\n";
        *treedotp << "\tgraph\t[label=\"" << filename + ".dot" << "\",\n";
        *treedotp << "\t\t labelloc=t, labeljust=l,\n";
        *treedotp << "\t\t //size=\"7.5,10\",\n"
                  << "];\n";
        dumpTreeDot(*treedotp);
        *treedotp << "}\n";
    }
}

string AstNode::instanceStr() const {
    // Max iterations before giving up on location search,
    // in case we have some circular reference bug.
    constexpr unsigned maxIterations = 10000;
    unsigned iterCount = 0;
    // Walk 'backp' chain
    cho (const AstNode* currp = this; currp; currp = currp->backp(), ++iterCount) {
        nếu (VL_UNCOVERABLE(iterCount >= maxIterations)) trả_về "";  // LCOV_EXCL_LINE
        // Prefer the enclosing scope, if there is one. This is always under the enclosing module,
        // so just pick it up when encountered
        nếu (const AstScope* const scopep = VN_CAST(currp, Scope)) {
            trả_về scopep->isTop() ? "" : "... note: In instance " + scopep->prettyNameQ();
        }
        // If scopes don't exist, report an example instance of the enclosing module
        nếu (const AstModule* const modp = VN_CAST(currp, Module)) {
            const string instanceName = modp->someInstanceName();
            trả_về instanceName.empty() ? "" : "... note: In instance '" + instanceName + "'";
        }
    }
    trả_về "";
}
void AstNode::v3errorEnd(const std::ostringstream& str) const VL_RELEASE(V3Error::s().m_mutex) {
    nếu (!m_fileline) {
        V3Error::v3errorEnd(str, "", nullptr);
    } không_thì {
        // Don't look for instance name when warning is disabled.
        // In case of large number of warnings, this can
        // take significant amount of time
        const string instanceStrExtra
            = m_fileline->warnIsOff(V3Error::s().errorCode()) ? "" : instanceStr();
        std::ostringstream nsstr;
        nsstr << str.str();
        nếu (debug()) {
            nsstr << '\n';
            nsstr << "-node: ";
            const_cast<AstNode*>(this)->dump(nsstr);
            nsstr << '\n';
        }
        m_fileline->v3errorEnd(nsstr, instanceStrExtra);
    }
}
void AstNode::v3errorEndFatal(const std::ostringstream& str) const
    VL_RELEASE(V3Error::s().m_mutex) {
    v3errorEnd(str);
    assert(0);  // LCOV_EXCL_LINE
    VL_UNREACHABLE;
}

//======================================================================
// Data type conversion

void AstNode::dtypeChgSigned(bool flag) {
    UASSERT_OBJ(dtypep(), this, "No dtype when changing to (un)signed");
    dtypeChgWidthSigned(dtypep()->width(), dtypep()->widthMin(), VSigning::fromBool(flag));
}
void AstNode::dtypeChgWidth(số_nguyên width, số_nguyên widthMin) {
    UASSERT_OBJ(dtypep(), this, "No dtype when changing width");
    dtypeChgWidthSigned(width, widthMin, dtypep()->numeric());
}

void AstNode::dtypeChgWidthSigned(số_nguyên width, số_nguyên widthMin, VSigning numeric) {
    UASSERT_OBJ(dtypep(), this, "No dtype when changing width");
    const AstNodeDType* const enumDTypep = dtypep()->skipRefToEnump();
    nếu (width == dtypep()->width() && widthMin == dtypep()->widthMin()
        && numeric == dtypep()->numeric()
        // Enums need to become direct sizes to avoid later ENUMVALUE errors
        && !VN_IS(enumDTypep, EnumDType)) {
        trả_về;  // Correct already
    }
    const AstNodeDType* const dtypep = enumDTypep->skipRefp();
    // This cannot be asserted yet, because V3Clean tends to change width of everything
    // UASSERT_OBJ(!dtypep->isCompound(), this, "Cannot change width of compound dtype.");
    dtypeSetBitOrLogicUnsized(width, widthMin, numeric, dtypep->isFourstate());
}

AstNodeDType* AstNode::findBasicDType(VBasicDTypeKwd kwd) const {
    // For 'simple' types we use the global directory.  These are all unsized.
    // More advanced types land under the module/task/etc
    trả_về v3Global.rootp()->typeTablep()->findBasicDType(fileline(), kwd);
}
AstNodeDType* AstNode::findBitDType(số_nguyên width, số_nguyên widthMin, VSigning numeric) const {
    trả_về v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             width, widthMin, numeric);
}
AstNodeDType* AstNode::findLogicDType(số_nguyên width, số_nguyên widthMin, VSigning numeric) const {
    trả_về v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             width, widthMin, numeric);
}
AstNodeDType* AstNode::findLogic2StateDType(số_nguyên width, số_nguyên widthMin, VSigning numeric) const {
    trả_về v3Global.rootp()->typeTablep()->findLogicBitDType(
        fileline(), VBasicDTypeKwd::LOGIC2STATE, width, widthMin, numeric);
}
AstNodeDType* AstNode::findLogicRangeDType(const VNumRange& range, số_nguyên widthMin,
                                           VSigning numeric) const {
    trả_về v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::LOGIC,
                                                             range, widthMin, numeric);
}
AstNodeDType* AstNode::findBitRangeDType(const VNumRange& range, số_nguyên widthMin,
                                         VSigning numeric) const {
    trả_về v3Global.rootp()->typeTablep()->findLogicBitDType(fileline(), VBasicDTypeKwd::BIT,
                                                             range, widthMin, numeric);
}
AstBasicDType* AstNode::findInsertSameDType(AstBasicDType* nodep) {
    trả_về v3Global.rootp()->typeTablep()->findInsertSameDType(nodep);
}
AstNodeDType* AstNode::findConstraintRefDType() const {
    trả_về v3Global.rootp()->typeTablep()->findConstraintRefDType(fileline());
}
AstNodeDType* AstNode::findEmptyQueueDType() const {
    trả_về v3Global.rootp()->typeTablep()->findEmptyQueueDType(fileline());
}
AstNodeDType* AstNode::findQueueIndexDType() const {
    trả_về v3Global.rootp()->typeTablep()->findQueueIndexDType(fileline());
}
AstNodeDType* AstNode::findStreamDType() const {
    trả_về v3Global.rootp()->typeTablep()->findStreamDType(fileline());
}
AstNodeDType* AstNode::findVoidDType() const {
    trả_về v3Global.rootp()->typeTablep()->findVoidDType(fileline());
}

static const AstNodeDType* computeCastableBase(const AstNodeDType* nodep) {
    trong_khi (true) {
        nếu (const AstPackArrayDType* const packp = VN_CAST(nodep, PackArrayDType)) {
            nodep = packp->subDTypep();
            continue;
        }
        nếu (const AstNodeDType* const refp = nodep->skipRefToEnump()) {
            nếu (refp != nodep) {
                nodep = refp;
                continue;
            }
        }
        trả_về nodep;
    }
}

static VCastable computeCastableImp(const AstNodeDType* toDtp, const AstNodeDType* fromDtp,
                                    const AstNode* fromConstp, const bool checkIfaceArgCompat) {
    const VCastable castable = VCastable::UNSUPPORTED;
    toDtp = toDtp->skipRefToEnump();
    fromDtp = fromDtp->skipRefToEnump();
    nếu (toDtp == fromDtp) trả_về VCastable::SAMEISH;
    nếu (toDtp->similarDType(fromDtp)) trả_về VCastable::SAMEISH;
    // UNSUP unpacked struct/unions (treated like BasicDType)
    const AstNodeDType* fromBaseDtp = computeCastableBase(fromDtp);

    const bool fromNumericable = VN_IS(fromBaseDtp, BasicDType) || VN_IS(fromBaseDtp, EnumDType)
                                 || VN_IS(fromBaseDtp, StreamDType)
                                 || VN_IS(fromBaseDtp, NodeUOrStructDType);

    const AstNodeDType* toBaseDtp = computeCastableBase(toDtp);
    const bool toNumericable
        = VN_IS(toBaseDtp, BasicDType) || VN_IS(toBaseDtp, NodeUOrStructDType);

    nếu (toBaseDtp == fromBaseDtp) trả_về VCastable::COMPATIBLE;

    nếu (toNumericable) {
        nếu (fromNumericable) trả_về VCastable::COMPATIBLE;
    } không_thì nếu (VN_IS(toBaseDtp, EnumDType)) {
        nếu (VN_IS(fromBaseDtp, EnumDType) && toDtp->sameTree(fromDtp))
            trả_về VCastable::ENUM_IMPLICIT;
        nếu (fromNumericable) trả_về VCastable::ENUM_EXPLICIT;
    } không_thì nếu (VN_IS(toDtp, QueueDType)
               && (VN_IS(fromDtp, BasicDType) || VN_IS(fromDtp, StreamDType))) {
        trả_về VCastable::COMPATIBLE;
    } không_thì nếu (VN_IS(toDtp, ClassRefDType) && VN_IS(fromConstp, Const)) {
        nếu (fromConstp->isNull()) trả_về VCastable::COMPATIBLE;
    } không_thì nếu (VN_IS(toDtp, ClassRefDType) && VN_IS(fromDtp, ClassRefDType)) {
        const AstClass* const toClassp = VN_AS(toDtp, ClassRefDType)->classp();
        const AstClass* const fromClassp = VN_AS(fromDtp, ClassRefDType)->classp();
        const bool downcast = AstClass::isClassExtendedFrom(toClassp, fromClassp);
        const bool upcast = AstClass::isClassExtendedFrom(fromClassp, toClassp);
        nếu (upcast) trả_về VCastable::COMPATIBLE;
        nếu (downcast) trả_về VCastable::DYNAMIC_CLASS;
        trả_về VCastable::INCOMPATIBLE;
    } không_thì nếu (const AstIfaceRefDType* const toIfp
               = VN_CAST(checkIfaceArgCompat ? toDtp->elemDTypep(true) : toDtp, IfaceRefDType)) {
        // Two interface refs are compatible if they point at the same interface
        // module (and modport, if any). Pointer-equality on the dtype isn't
        // enough since every cell binding clones the dtype.
        // Argument compatibility also requires matching virtualness for ref arguments, while
        // input arguments may bind an unqualified or same-modport source to a virtual target.
        // Argument compatibility also supports fixed-size arrays of interface references.
        const AstIfaceRefDType* const fromIfp
            = VN_CAST(checkIfaceArgCompat ? fromDtp->elemDTypep(true) : fromDtp, IfaceRefDType);
        nếu (checkIfaceArgCompat && fromIfp && (toDtp != toIfp || fromDtp != fromIfp)) {
            const AstUnpackArrayDType* const toArrayp = VN_CAST(toDtp, UnpackArrayDType);
            const AstUnpackArrayDType* const fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
            // IEEE 1800-2023 6.22.2: Equal-sized fixed arrays have equivalent types.
            nếu (!toArrayp || !fromArrayp
                || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
                trả_về VCastable::INCOMPATIBLE;
            }
            trả_về computeCastableImp(toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr,
                                      checkIfaceArgCompat);
        }
        nếu (!fromIfp || toIfp->ifaceViaCellp() != fromIfp->ifaceViaCellp()) {
            nếu (!checkIfaceArgCompat) trả_về castable;
            trả_về VCastable::INCOMPATIBLE;
        }
        const bool sameModport = toIfp->modportp() == fromIfp->modportp();
        nếu (!checkIfaceArgCompat) {
            nếu (!toIfp->modportp() || sameModport) trả_về VCastable::COMPATIBLE;
            trả_về castable;
        }
        nếu (toIfp->isVirtual() == fromIfp->isVirtual() && sameModport) {
            trả_về VCastable::SAMEISH;
        }
        // An unqualified interface or virtual interface may bind to a modport-qualified
        // virtual interface.
        nếu (toIfp->isVirtual() && (!fromIfp->modportp() || sameModport)) {
            trả_về VCastable::COMPATIBLE;
        }
        trả_về VCastable::INCOMPATIBLE;
    } không_thì nếu (const AstUnpackArrayDType* const toArrayp = VN_CAST(toDtp, UnpackArrayDType)) {
        // Equivalent if the same size with equivalent elements, the bounds may differ
        // (IEEE 1800-2023 6.22.2)
        const AstUnpackArrayDType* const fromArrayp = VN_CAST(fromDtp, UnpackArrayDType);
        nếu (!fromArrayp || toArrayp->elementsConst() != fromArrayp->elementsConst()) {
            trả_về castable;
        }
        const VCastable subCastable = computeCastableImp(
            toArrayp->subDTypep(), fromArrayp->subDTypep(), nullptr, checkIfaceArgCompat);
        // Elements must match, but a nested dimension is COMPATIBLE if equivalent, and a virtual
        // interface element can refer to an interface instance
        const AstNodeDType* const toSubp = toArrayp->subDTypep()->skipRefp();
        nếu (subCastable == VCastable::SAMEISH
            || (subCastable == VCastable::COMPATIBLE
                && (VN_IS(toSubp, UnpackArrayDType) || VN_IS(toSubp, IfaceRefDType)))) {
            trả_về VCastable::COMPATIBLE;
        }
    }
    trả_về castable;
}

VCastable AstNode::computeCastable(const AstNodeDType* toDtp, const AstNodeDType* fromDtp,
                                   const AstNode* fromConstp, const bool checkIfaceArgCompat) {
    const auto castable = computeCastableImp(toDtp, fromDtp, fromConstp, checkIfaceArgCompat);
    UINFO(9, "  castable=" << castable << "  for " << toDtp);
    UINFO(9, "     =?= " << fromDtp);
    nếu (fromConstp) UINFO(9, "     const= " << fromConstp);
    trả_về castable;
}

AstNodeDType* AstNode::getCommonClassTypep(AstNode* node1p, AstNode* node2p) {
    // Return the class type that both node1p and node2p are castable to.
    // If both are null, return the type of null constant.
    // If one is a class and one is null, return AstClassRefDType that points to that class.
    // If no common class type exists, return nullptr.

    // First handle cases with null values and when one class is a super class of the other.
    nếu (VN_IS(node1p, Const)) std::swap(node1p, node2p);
    {
        const VCastable castable = computeCastable(node1p->dtypep(), node2p->dtypep(), node2p);
        nếu (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
            trả_về node1p->dtypep();
        nếu (castable == VCastable::DYNAMIC_CLASS) trả_về node2p->dtypep();
    }

    AstClassRefDType* classDtypep1 = VN_CAST(node1p->dtypep()->skipRefp(), ClassRefDType);
    trong_khi (classDtypep1) {
        const VCastable castable = computeCastable(classDtypep1, node2p->dtypep(), node2p);
        nếu (castable == VCastable::COMPATIBLE) trả_về classDtypep1;
        AstClassExtends* const extendsp = classDtypep1->classp()->extendsp();
        nếu (!extendsp) break;
        AstNodeDType* const edtp
            = extendsp->dtypep() ? extendsp->dtypep() : extendsp->childDTypep();
        classDtypep1 = VN_AS(edtp->skipRefp(), ClassRefDType);
    }
    trả_về nullptr;
}

//######################################################################
// Renders the canonical pattern S-expression for a single AstNode

class VNPatternString final {
    std::ostream& m_os;

    std::map<std::string, std::string> m_internedConsts;  // Interned constants
    std::map<số_nguyên, std::string> m_internedWordWidths;  // Interned widths
    std::map<số_nguyên, std::string> m_internedWideWidths;  // Interned widths

    // Whether to dump the widhtMin as well
    const bool m_dumpWidthMin = v3Global.widthMinUsage() != VWidthMinUsage::MATCHES_WIDTH;

    static std::string toLetters(size_t value, bool lowerCase = false) {
        const char base = lowerCase ? 'a' : 'A';
        std::string s;
        do { s += static_cast<char>(base + value % 26); } trong_khi (value /= 26);
        trả_về s;
    }

    const std::string& internConst(const AstConst& nodep) {
        const auto pair = m_internedConsts.emplace(nodep.num().ascii(false), "");
        nếu (pair.second) pair.first->second += toLetters(m_internedConsts.size() - 1);
        trả_về pair.first->second;
    }

    const std::string& internWordWidth(số_nguyên value) {
        const auto pair = m_internedWordWidths.emplace(value, "");
        nếu (pair.second) pair.first->second += toLetters(m_internedWordWidths.size() - 1, true);
        trả_về pair.first->second;
    }

    const std::string& internWideWidth(số_nguyên value) {
        const auto pair = m_internedWideWidths.emplace(value, "");
        nếu (pair.second) pair.first->second += toLetters(m_internedWideWidths.size() - 1);
        trả_về pair.first->second;
    }

    // True if the node has no operands (e.g. a VarRef or Const)
    static bool isLeaf(const AstNode* nodep) {
        const VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
        cho (const VNTypeInfo::OpEn& opType : typeInfo.m_opType) {
            nếu (opType != VNTypeInfo::OP_UNUSED) trả_về false;
        }
        trả_về true;
    }

    // Render operand, return true if not unused
    void renderOperand(VNTypeInfo::OpEn opType, const AstNode* const opp, uint32_t depth) {
        switch (opType) {
        case VNTypeInfo::OP_UNUSED:  //
            break;
        case VNTypeInfo::OP_USED:  //
            m_os << ' ';
            render(opp, depth);
            break;
        case VNTypeInfo::OP_LIST:
            m_os << " [";
            cho (const AstNode* nodep = opp; nodep; nodep = nodep->nextp()) {
                nếu (nodep != opp) m_os << ", ";
                render(opp, depth);
            }
            m_os << ']';
            break;
        case VNTypeInfo::OP_OPTIONAL:
            nếu (opp) {
                m_os << " ";
                render(opp, depth);
            } không_thì {
                m_os << " nil";
            }
            break;
        }
    }

    // Render the node into the stream.
    void render(const AstNode* nodep, uint32_t depth) {
        nếu (const AstConst* const constp = VN_CAST(nodep, Const)) {
            // Base case 1: constant
            nếu (constp->isZero()) {
                m_os << "(CONST ZERO)";
            } không_thì nếu (constp->isEqAllOnes()) {
                m_os << "(CONST ONES)";
            } không_thì {
                m_os << "(CONST #" << internConst(*constp) << ')';
            }
        } không_thì nếu (isLeaf(nodep)) {
            // Base case 2: expression with no operands (e.g. a variable reference)
            m_os << '(' << nodep->typeName() << ')';
        } không_thì nếu (depth == 0) {
            // Base case 3: deep expression
            m_os << '_';
        } không_thì {
            // Recursively print an S-expression for the expression
            m_os << '(';
            // Name
            m_os << nodep->typeName();
            // Operands
            const VNTypeInfo& typeInfo = VNType::typeInfo(nodep->type());
            renderOperand(typeInfo.m_opType[0], nodep->op1p(), depth - 1);
            renderOperand(typeInfo.m_opType[1], nodep->op2p(), depth - 1);
            renderOperand(typeInfo.m_opType[2], nodep->op3p(), depth - 1);
            renderOperand(typeInfo.m_opType[3], nodep->op4p(), depth - 1);
            // S-expression end
            m_os << ')';
        }

        // Annotate type
        m_os << ':';
        const AstNodeDType* const dtypep = nodep->dtypep() ? nodep->dtypep()->skipRefp() : nullptr;
        nếu (!dtypep) {
            m_os << '?';
        } không_thì nếu (dtypep->isCompound() || VN_IS(dtypep, UnpackArrayDType)) {
            dtypep->dumpSmall(m_os);
        } không_thì {
            const số_nguyên width = nodep->width();
            nếu (width == 1) {
                m_os << '1';
            } không_thì nếu (width <= VL_QUADSIZE) {
                m_os << internWordWidth(width);
            } không_thì {
                m_os << internWideWidth(width);
            }
            nếu (m_dumpWidthMin) {
                m_os << '/';
                const số_nguyên widthMin = nodep->widthMin();
                nếu (widthMin == 1) {
                    m_os << '1';
                } không_thì nếu (widthMin <= VL_QUADSIZE) {
                    m_os << internWordWidth(widthMin);
                } không_thì {
                    m_os << internWideWidth(widthMin);
                }
            }
        }
    }

public:
    VNPatternString(std::ostream& os, const AstNode* nodep, uint32_t depth)
        : m_os{os} {
        render(nodep, depth);
    }
};

std::string AstNodeExpr::patternString(uint32_t depth) const {
    std::ostringstream oss;
    VNPatternString{oss, this, depth};
    trả_về oss.str();
}

//######################################################################
// VNDeleter

void VNDeleter::doDeletes() {
    cho (AstNode* const nodep : m_deleteps) nodep->deleteTree();
    m_deleteps.clear();
}

//######################################################################
// VNVisitor

#include "V3Ast__gen_visitor_defns.h"  // From ./astgen
