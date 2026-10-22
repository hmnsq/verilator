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

#समावेश "V3Ast.h"
#समावेश "V3Const.h"
#समावेश "V3Control.h"
#समावेश "V3Global.h"
#समावेश "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#समावेश <stack>
#समावेश <vector>

class V3ParseGrammar final {
खुला:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_गाँठ* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_गाँठ_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_गाँठ_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_पंक्ति* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_गाँठ* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    पूर्णांक m_pin_अंक = -1;  // Pin number currently parsing
    std::ढेर<पूर्णांक> m_pin_ढेर;  // Queue of pin numbers being parsed

    अटल पूर्णांक s_type_Imp_अंक;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = शेष;
    अटल V3ParseGrammar* singletonp() {
        अटल V3ParseGrammar singleton;
        वापस &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_गाँठ_व्यंजक* nodep) VL_MT_DISABLED;
    bool allTracingOn(स्थिर File_पंक्ति* fl) स्थिर {
        वापस v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_गाँठ_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_गाँठ_Range* rangep) VL_MT_DISABLED;
    Ast_गाँठ_Pre_Sel* scrubSel(Ast_गाँठ_व्यंजक* fromp, Ast_गाँठ_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_गाँठ_DType* createArray(Ast_गाँठ_DType* basep, Ast_गाँठ_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_पंक्ति* fileline, स्थिर string& name, Ast_गाँठ_Range* arrayp,
                           Ast_गाँठ* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_व्यंजक(File_पंक्ति* fileline, स्थिर string& name, पूर्णांक मूल्य) VL_MT_DISABLED;
    std::string textQuoted(File_पंक्ति* fileline, स्थिर std::string& text) {
        वापस singletonp()->unquoteString(fileline, text);
    }
    Ast_गाँठ* createCell(File_पंक्ति* fileline, स्थिर string& name, AstPin* pinlistp,
                        Ast_गाँठ_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* स्थिर nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_वृक्ष(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        वापस nodep;
    }
    अटल Ast_गाँठ* create_Pattern_कुंजी(Ast_गाँठ_व्यंजक* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        अगर (स्थिर AstParseRef* स्थिर refp = VN_CAST(exprp, ParseRef)) {
            अगर (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_गाँठ* स्थिर textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_वृक्ष(), exprp);
                वापस textp;
            }
        }
        वापस exprp;
    }
    AstDisplay* createDisplayError(File_पंक्ति* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_गाँठ::add_अग्र<Ast_गाँठ, Ast_गाँठ>(nodep, new AstStop{fileline, false});
        वापस nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    अटल AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_गाँठ_व्यंजक* sizep) {
        अगर (!sizep) वापस binp;
        binp->isArray(true);
        अगर (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_वृक्ष(), sizep);
        } अन्यथा {
            binp->arraySizep(sizep);
        }
        वापस binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    अटल bool coverBinArrayUnsized(Ast_गाँठ_व्यंजक* sizep) {
        अगर (!sizep) वापस false;
        अगर (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_वृक्ष(), sizep);
        वापस true;
    }
    Ast_गाँठ_व्यंजक* createGatePin(Ast_गाँठ_व्यंजक* exprp) {
        AstRange* स्थिर rangep = m_gateRangep;
        अगर (!rangep) वापस exprp;
        वापस new AstGatePin{rangep->fileline(), exprp, rangep->clone_वृक्ष(true)};
    }
    Ast_Sen_वृक्ष* create_Sen_वृक्ष_Changed(File_पंक्ति* fl, Ast_गाँठ_व्यंजक* exprp) {
        वापस new Ast_Sen_वृक्ष{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_वृक्ष* create_Sen_वृक्ष_Dot_Star(File_पंक्ति* fl) {
        वापस new Ast_Sen_वृक्ष{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_गाँठ_व्यंजक* createGlobalClockParseRef(File_पंक्ति* fl) {
        वापस new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_वृक्ष* create_Global_Clock_Sen_वृक्ष(File_पंक्ति* fl) {
        वापस create_Sen_वृक्ष_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_गाँठ* createNettype(File_पंक्ति* fl, स्थिर string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* स्थिर nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        वापस nodep;
    }
    Ast_गाँठ* createTypedef(File_पंक्ति* fl, स्थिर string& name, Ast_गाँठ* attrsp, Ast_गाँठ_DType* basep,
                           Ast_गाँठ_Range* rangep) {
        AstTypedef* स्थिर nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        वापस nodep;
    }
    Ast_गाँठ* createTypedefFwd(File_पंक्ति* fl, स्थिर string& name, स्थिर VFwdType& fwdType) {
        AstTypedefFwd* स्थिर nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        वापस nodep;
    }
    अटल व्योम endLabel(File_पंक्ति* fl, स्थिर Ast_गाँठ* nodep, स्थिर string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    अटल व्योम endLabel(File_पंक्ति* fl, स्थिर string& name, स्थिर string* endnamep) {
        अगर (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_गाँठ::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    व्योम setDType(Ast_गाँठ_DType* dtypep) {
        अगर (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_वृक्ष(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    व्योम setNetDelay(AstDelay* netDelayp) {
        अगर (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_वृक्ष(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    व्योम setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    व्योम pin_धकेलो() {
        m_pin_ढेर.धकेलो(m_pin_अंक);
        m_pin_अंक = 1;
    }
    व्योम pin_निकालो(File_पंक्ति* fl) {
        अगर (VL_UNCOVERABLE(m_pin_ढेर.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_अंक = m_pin_ढेर.शीर्ष();
        m_pin_ढेर.निकालो();
    }
    Ast_गाँठ_DType* addRange(AstBasicDType* dtypep, Ast_गाँठ_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        अगर (!rangesp) वापस dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_गाँठ_Range* rangearraysp = nullptr;
        अगर (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } अन्यथा {
            Ast_गाँठ_Range* finalp = rangesp;
            जबतक (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            अगर (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            अगर (AstRange* स्थिर finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                अगर (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* स्थिर newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_वृक्ष(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        वापस createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_पंक्ति* fileline, स्थिर std::string& text) VL_MT_DISABLED;
    व्योम checkDpiVer(File_पंक्ति* fileline, स्थिर string& str) {
        अगर (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_पंक्ति* flp, स्थिर VDirection direction,
                                          Ast_गाँठ_व्यंजक* skewp, Ast_गाँठ* स्थिर clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        क्रम (Ast_गाँठ *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            अगर (nextp) nextp->unlink_Fr_Back_With_अग्र();
            अगर (itemsp && skewp) skewp = skewp->clone_वृक्ष(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_गाँठ::add_अग्र_Null(itemsp, itemp);
        }
        वापस itemsp;
    }

    व्योम setScopedSigAttr(Ast_गाँठ* attrsp) {
        अगर (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_वृक्ष(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    व्योम createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_गाँठ* cloneScopedSigAttr() स्थिर {
        वापस m_scopedSigAttr ? m_scopedSigAttr->clone_वृक्ष(true) : nullptr;
    }

    व्योम createGenericIface(Ast_गाँठ* स्थिर nodep, Ast_गाँठ_Range* स्थिर rangep,
                            Ast_गाँठ* sigAttrListp, File_पंक्ति* स्थिर modportFileline = nullptr,
                            स्थिर string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        स्थिर std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_गाँठ::add_अग्र(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_वृक्ष(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* स्थिर refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::निवेश;
        Ast_गाँठ::add_अग्र(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    अटल AstBegin* wrapInBegin(Ast_गाँठ_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        क्रम (Ast_गाँठ_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), गाँठ_Stmt);
            अगर (nextp) nextp->unlink_Fr_Back_With_अग्र();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            अगर (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_गाँठ::add_अग्र(resp, beginp);
        }
        वापस resp;
    }
};
