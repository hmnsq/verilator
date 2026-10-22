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

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#अन्तर्भाव <stack>
#अन्तर्भाव <vector>

class V3ParseGrammar final {
सार्वजनिक:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_गाठ* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_गाठ_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_गाठ_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_ओळ* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_गाठ* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    पूर्णांक m_pin_अंक = -1;  // Pin number currently parsing
    std::चळत<पूर्णांक> m_pin_चळत;  // Queue of pin numbers being parsed

    स्थिर पूर्णांक s_type_Imp_अंक;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = पूर्वनिर्धारित;
    स्थिर V3ParseGrammar* singletonp() {
        स्थिर V3ParseGrammar singleton;
        परत &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_गाठ_अभिव्यक्ती* nodep) VL_MT_DISABLED;
    bool allTracingOn(स्थिरांक File_ओळ* fl) स्थिरांक {
        परत v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_गाठ_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_गाठ_Range* rangep) VL_MT_DISABLED;
    Ast_गाठ_Pre_Sel* scrubSel(Ast_गाठ_अभिव्यक्ती* fromp, Ast_गाठ_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_गाठ_DType* createArray(Ast_गाठ_DType* basep, Ast_गाठ_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_ओळ* fileline, स्थिरांक string& name, Ast_गाठ_Range* arrayp,
                           Ast_गाठ* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_अभिव्यक्ती(File_ओळ* fileline, स्थिरांक string& name, पूर्णांक मूल्य) VL_MT_DISABLED;
    std::string textQuoted(File_ओळ* fileline, स्थिरांक std::string& text) {
        परत singletonp()->unquoteString(fileline, text);
    }
    Ast_गाठ* createCell(File_ओळ* fileline, स्थिरांक string& name, AstPin* pinlistp,
                        Ast_गाठ_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* स्थिरांक nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_वृक्ष(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        परत nodep;
    }
    स्थिर Ast_गाठ* create_Pattern_किल्ली(Ast_गाठ_अभिव्यक्ती* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        जर (स्थिरांक AstParseRef* स्थिरांक refp = VN_CAST(exprp, ParseRef)) {
            जर (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_गाठ* स्थिरांक textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_वृक्ष(), exprp);
                परत textp;
            }
        }
        परत exprp;
    }
    AstDisplay* createDisplayError(File_ओळ* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_गाठ::addNext<Ast_गाठ, Ast_गाठ>(nodep, new AstStop{fileline, false});
        परत nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    स्थिर AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_गाठ_अभिव्यक्ती* sizep) {
        जर (!sizep) परत binp;
        binp->isArray(true);
        जर (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_वृक्ष(), sizep);
        } अन्यथा {
            binp->arraySizep(sizep);
        }
        परत binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    स्थिर bool coverBinArrayUnsized(Ast_गाठ_अभिव्यक्ती* sizep) {
        जर (!sizep) परत false;
        जर (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_वृक्ष(), sizep);
        परत true;
    }
    Ast_गाठ_अभिव्यक्ती* createGatePin(Ast_गाठ_अभिव्यक्ती* exprp) {
        AstRange* स्थिरांक rangep = m_gateRangep;
        जर (!rangep) परत exprp;
        परत new AstGatePin{rangep->fileline(), exprp, rangep->clone_वृक्ष(true)};
    }
    Ast_Sen_वृक्ष* create_Sen_वृक्ष_Changed(File_ओळ* fl, Ast_गाठ_अभिव्यक्ती* exprp) {
        परत new Ast_Sen_वृक्ष{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_वृक्ष* create_Sen_वृक्ष_Dot_Star(File_ओळ* fl) {
        परत new Ast_Sen_वृक्ष{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_गाठ_अभिव्यक्ती* createGlobalClockParseRef(File_ओळ* fl) {
        परत new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_वृक्ष* create_Global_Clock_Sen_वृक्ष(File_ओळ* fl) {
        परत create_Sen_वृक्ष_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_गाठ* createNettype(File_ओळ* fl, स्थिरांक string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* स्थिरांक nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        परत nodep;
    }
    Ast_गाठ* createTypedef(File_ओळ* fl, स्थिरांक string& name, Ast_गाठ* attrsp, Ast_गाठ_DType* basep,
                           Ast_गाठ_Range* rangep) {
        AstTypedef* स्थिरांक nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        परत nodep;
    }
    Ast_गाठ* createTypedefFwd(File_ओळ* fl, स्थिरांक string& name, स्थिरांक VFwdType& fwdType) {
        AstTypedefFwd* स्थिरांक nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        परत nodep;
    }
    स्थिर शून्य endLabel(File_ओळ* fl, स्थिरांक Ast_गाठ* nodep, स्थिरांक string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    स्थिर शून्य endLabel(File_ओळ* fl, स्थिरांक string& name, स्थिरांक string* endnamep) {
        जर (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_गाठ::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    शून्य setDType(Ast_गाठ_DType* dtypep) {
        जर (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_वृक्ष(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    शून्य setNetDelay(AstDelay* netDelayp) {
        जर (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_वृक्ष(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    शून्य setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    शून्य pin_ढकला() {
        m_pin_चळत.ढकला(m_pin_अंक);
        m_pin_अंक = 1;
    }
    शून्य pin_काढा(File_ओळ* fl) {
        जर (VL_UNCOVERABLE(m_pin_चळत.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_अंक = m_pin_चळत.शीर्ष();
        m_pin_चळत.काढा();
    }
    Ast_गाठ_DType* addRange(AstBasicDType* dtypep, Ast_गाठ_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        जर (!rangesp) परत dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_गाठ_Range* rangearraysp = nullptr;
        जर (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } अन्यथा {
            Ast_गाठ_Range* finalp = rangesp;
            पर्यंत (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            जर (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            जर (AstRange* स्थिरांक finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                जर (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* स्थिरांक newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_वृक्ष(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        परत createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_ओळ* fileline, स्थिरांक std::string& text) VL_MT_DISABLED;
    शून्य checkDpiVer(File_ओळ* fileline, स्थिरांक string& str) {
        जर (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_ओळ* flp, स्थिरांक VDirection direction,
                                          Ast_गाठ_अभिव्यक्ती* skewp, Ast_गाठ* स्थिरांक clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        साठी (Ast_गाठ *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            जर (nextp) nextp->unlinkFrBackWithNext();
            जर (itemsp && skewp) skewp = skewp->clone_वृक्ष(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_गाठ::addNextNull(itemsp, itemp);
        }
        परत itemsp;
    }

    शून्य setScopedSigAttr(Ast_गाठ* attrsp) {
        जर (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_वृक्ष(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    शून्य createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_गाठ* cloneScopedSigAttr() स्थिरांक {
        परत m_scopedSigAttr ? m_scopedSigAttr->clone_वृक्ष(true) : nullptr;
    }

    शून्य createGenericIface(Ast_गाठ* स्थिरांक nodep, Ast_गाठ_Range* स्थिरांक rangep,
                            Ast_गाठ* sigAttrListp, File_ओळ* स्थिरांक modportFileline = nullptr,
                            स्थिरांक string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        स्थिरांक std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_गाठ::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_वृक्ष(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* स्थिरांक refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::आदान;
        Ast_गाठ::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    स्थिर AstBegin* wrapInBegin(Ast_गाठ_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        साठी (Ast_गाठ_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), गाठ_Stmt);
            जर (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            जर (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_गाठ::addNext(resp, beginp);
        }
        परत resp;
    }
};
