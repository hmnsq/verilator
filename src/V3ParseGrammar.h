// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Parse syntax tree
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

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3Control.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#അന്തര്ഭാവ <stack>
#അന്തര്ഭാവ <vector>

class V3ParseGrammar final {
പൊതു:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_കണ്ണി* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_കണ്ണി_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_കണ്ണി_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_വരി* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
    AstPin* m_instParamp = nullptr;  // Parameters for instantiations
    string m_instModule;  // Name of module referenced for instantiations
    VVarType m_varDecl = VVarType::UNKNOWN;  // Type for next signal declaration (reg/wire/etc)
    VDirection m_varIO = VDirection::NONE;  // Direction for next signal declaration (reg/wire/etc)
    VLifetime m_varLifetime;  // Static/Automatic for next signal
    V3Control::VarSpecKind m_vltVarSpecKind = V3Control::VarSpecKind::VAR;
    bool m_impliedDecl = false;  // Allow implied wire declarations
    bool m_varDeclTyped = false;  // Var got reg/wire for dedup check
    bool m_pinAnsi = false;  // In ANSI parameter or port list
    bool m_tracingParse = true;  // Tracing disable for parser
    bool m_inImplements = false;  // Is inside class implements list
    bool m_insideProperty = false;  // Is inside property declaration
    bool m_specifyignWarned = false;  // Issued a SPECIFYIGN warning
    bool m_typedPropertyPort = false;  // Typed property port occurred on port lists
    bool m_modportImpExpActive
        = false;  // Standalone ID is a tf_identifier instead of port_identifier
    bool m_modportImpExpLastIsExport
        = false;  // Last import_export statement in modportPortsDecl is an export
    Ast_കണ്ണി* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    പൂർണ്ണസംഖ്യ m_pin_അക്കം = -1;  // Pin number currently parsing
    std::അട്ടി<പൂർണ്ണസംഖ്യ> m_pin_അട്ടി;  // Queue of pin numbers being parsed

    സ്ഥിതം പൂർണ്ണസംഖ്യ s_type_Imp_അക്കം;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = സ്ഥിരസ്ഥിതി;
    സ്ഥിതം V3ParseGrammar* singletonp() {
        സ്ഥിതം V3ParseGrammar singleton;
        തിരികെ &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) VL_MT_DISABLED;
    bool allTracingOn(സ്ഥിരം File_വരി* fl) സ്ഥിരം {
        തിരികെ v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_കണ്ണി_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_കണ്ണി_Range* rangep) VL_MT_DISABLED;
    Ast_കണ്ണി_Pre_Sel* scrubSel(Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_കണ്ണി_DType* createArray(Ast_കണ്ണി_DType* basep, Ast_കണ്ണി_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_വരി* fileline, സ്ഥിരം string& name, Ast_കണ്ണി_Range* arrayp,
                           Ast_കണ്ണി* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_വ്യഞ്ജകം(File_വരി* fileline, സ്ഥിരം string& name, പൂർണ്ണസംഖ്യ മൂല്യം) VL_MT_DISABLED;
    std::string textQuoted(File_വരി* fileline, സ്ഥിരം std::string& text) {
        തിരികെ singletonp()->unquoteString(fileline, text);
    }
    Ast_കണ്ണി* createCell(File_വരി* fileline, സ്ഥിരം string& name, AstPin* pinlistp,
                        Ast_കണ്ണി_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* സ്ഥിരം nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_വൃക്ഷം(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        തിരികെ nodep;
    }
    സ്ഥിതം Ast_കണ്ണി* create_Pattern_താക്കോൽ(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        എങ്കിൽ (സ്ഥിരം AstParseRef* സ്ഥിരം refp = VN_CAST(exprp, ParseRef)) {
            എങ്കിൽ (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_കണ്ണി* സ്ഥിരം textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_വൃക്ഷം(), exprp);
                തിരികെ textp;
            }
        }
        തിരികെ exprp;
    }
    AstDisplay* createDisplayError(File_വരി* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_കണ്ണി::addNext<Ast_കണ്ണി, Ast_കണ്ണി>(nodep, new AstStop{fileline, false});
        തിരികെ nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    സ്ഥിതം AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_കണ്ണി_വ്യഞ്ജകം* sizep) {
        എങ്കിൽ (!sizep) തിരികെ binp;
        binp->isArray(true);
        എങ്കിൽ (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_വൃക്ഷം(), sizep);
        } അല്ലെങ്കിൽ {
            binp->arraySizep(sizep);
        }
        തിരികെ binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    സ്ഥിതം bool coverBinArrayUnsized(Ast_കണ്ണി_വ്യഞ്ജകം* sizep) {
        എങ്കിൽ (!sizep) തിരികെ false;
        എങ്കിൽ (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_വൃക്ഷം(), sizep);
        തിരികെ true;
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* createGatePin(Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        AstRange* സ്ഥിരം rangep = m_gateRangep;
        എങ്കിൽ (!rangep) തിരികെ exprp;
        തിരികെ new AstGatePin{rangep->fileline(), exprp, rangep->clone_വൃക്ഷം(true)};
    }
    Ast_Sen_വൃക്ഷം* create_Sen_വൃക്ഷം_Changed(File_വരി* fl, Ast_കണ്ണി_വ്യഞ്ജകം* exprp) {
        തിരികെ new Ast_Sen_വൃക്ഷം{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_വൃക്ഷം* create_Sen_വൃക്ഷം_Dot_Star(File_വരി* fl) {
        തിരികെ new Ast_Sen_വൃക്ഷം{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* createGlobalClockParseRef(File_വരി* fl) {
        തിരികെ new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_വൃക്ഷം* create_Global_Clock_Sen_വൃക്ഷം(File_വരി* fl) {
        തിരികെ create_Sen_വൃക്ഷം_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_കണ്ണി* createNettype(File_വരി* fl, സ്ഥിരം string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* സ്ഥിരം nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        തിരികെ nodep;
    }
    Ast_കണ്ണി* createTypedef(File_വരി* fl, സ്ഥിരം string& name, Ast_കണ്ണി* attrsp, Ast_കണ്ണി_DType* basep,
                           Ast_കണ്ണി_Range* rangep) {
        AstTypedef* സ്ഥിരം nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        തിരികെ nodep;
    }
    Ast_കണ്ണി* createTypedefFwd(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം VFwdType& fwdType) {
        AstTypedefFwd* സ്ഥിരം nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        തിരികെ nodep;
    }
    സ്ഥിതം ശൂന്യം endLabel(File_വരി* fl, സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    സ്ഥിതം ശൂന്യം endLabel(File_വരി* fl, സ്ഥിരം string& name, സ്ഥിരം string* endnamep) {
        എങ്കിൽ (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_കണ്ണി::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    ശൂന്യം setDType(Ast_കണ്ണി_DType* dtypep) {
        എങ്കിൽ (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_വൃക്ഷം(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    ശൂന്യം setNetDelay(AstDelay* netDelayp) {
        എങ്കിൽ (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_വൃക്ഷം(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    ശൂന്യം setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    ശൂന്യം pin_തള്ളുക() {
        m_pin_അട്ടി.തള്ളുക(m_pin_അക്കം);
        m_pin_അക്കം = 1;
    }
    ശൂന്യം pin_എടുക്കുക(File_വരി* fl) {
        എങ്കിൽ (VL_UNCOVERABLE(m_pin_അട്ടി.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_അക്കം = m_pin_അട്ടി.മുകൾ();
        m_pin_അട്ടി.എടുക്കുക();
    }
    Ast_കണ്ണി_DType* addRange(AstBasicDType* dtypep, Ast_കണ്ണി_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        എങ്കിൽ (!rangesp) തിരികെ dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_കണ്ണി_Range* rangearraysp = nullptr;
        എങ്കിൽ (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } അല്ലെങ്കിൽ {
            Ast_കണ്ണി_Range* finalp = rangesp;
            ആകുമ്പോൾ (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            എങ്കിൽ (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            എങ്കിൽ (AstRange* സ്ഥിരം finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                എങ്കിൽ (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* സ്ഥിരം newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_വൃക്ഷം(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        തിരികെ createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_വരി* fileline, സ്ഥിരം std::string& text) VL_MT_DISABLED;
    ശൂന്യം checkDpiVer(File_വരി* fileline, സ്ഥിരം string& str) {
        എങ്കിൽ (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_വരി* flp, സ്ഥിരം VDirection direction,
                                          Ast_കണ്ണി_വ്യഞ്ജകം* skewp, Ast_കണ്ണി* സ്ഥിരം clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        വേണ്ടി (Ast_കണ്ണി *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            എങ്കിൽ (nextp) nextp->unlinkFrBackWithNext();
            എങ്കിൽ (itemsp && skewp) skewp = skewp->clone_വൃക്ഷം(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_കണ്ണി::addNextNull(itemsp, itemp);
        }
        തിരികെ itemsp;
    }

    ശൂന്യം setScopedSigAttr(Ast_കണ്ണി* attrsp) {
        എങ്കിൽ (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_വൃക്ഷം(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    ശൂന്യം createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_കണ്ണി* cloneScopedSigAttr() സ്ഥിരം {
        തിരികെ m_scopedSigAttr ? m_scopedSigAttr->clone_വൃക്ഷം(true) : nullptr;
    }

    ശൂന്യം createGenericIface(Ast_കണ്ണി* സ്ഥിരം nodep, Ast_കണ്ണി_Range* സ്ഥിരം rangep,
                            Ast_കണ്ണി* sigAttrListp, File_വരി* സ്ഥിരം modportFileline = nullptr,
                            സ്ഥിരം string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        സ്ഥിരം std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_കണ്ണി::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_വൃക്ഷം(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* സ്ഥിരം refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::നിവേശം;
        Ast_കണ്ണി::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    സ്ഥിതം AstBegin* wrapInBegin(Ast_കണ്ണി_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        വേണ്ടി (Ast_കണ്ണി_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), കണ്ണി_Stmt);
            എങ്കിൽ (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            എങ്കിൽ (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_കണ്ണി::addNext(resp, beginp);
        }
        തിരികെ resp;
    }
};
