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
    Ast_ग्रन्थिः* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_ग्रन्थिः_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_ग्रन्थिः_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_पङ्क्तिः* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_ग्रन्थिः* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    पूर्णाङ्क m_pin_अङ्कः = -1;  // Pin number currently parsing
    std::राशिः<पूर्णाङ्क> m_pin_राशिः;  // Queue of pin numbers being parsed

    स्थिर पूर्णाङ्क s_type_Imp_अङ्कः;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = स्वभाव;
    स्थिर V3ParseGrammar* singletonp() {
        स्थिर V3ParseGrammar singleton;
        प्रत्यागम &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) VL_MT_DISABLED;
    bool allTracingOn(ध्रुव File_पङ्क्तिः* fl) ध्रुव {
        प्रत्यागम v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_ग्रन्थिः_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_ग्रन्थिः_Range* rangep) VL_MT_DISABLED;
    Ast_ग्रन्थिः_Pre_Sel* scrubSel(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_ग्रन्थिः_DType* createArray(Ast_ग्रन्थिः_DType* basep, Ast_ग्रन्थिः_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_पङ्क्तिः* fileline, ध्रुव string& name, Ast_ग्रन्थिः_Range* arrayp,
                           Ast_ग्रन्थिः* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_व्यञ्जकम्(File_पङ्क्तिः* fileline, ध्रुव string& name, पूर्णाङ्क मानम्) VL_MT_DISABLED;
    std::string textQuoted(File_पङ्क्तिः* fileline, ध्रुव std::string& text) {
        प्रत्यागम singletonp()->unquoteString(fileline, text);
    }
    Ast_ग्रन्थिः* createCell(File_पङ्क्तिः* fileline, ध्रुव string& name, AstPin* pinlistp,
                        Ast_ग्रन्थिः_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ध्रुव nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_वृक्षः(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        प्रत्यागम nodep;
    }
    स्थिर Ast_ग्रन्थिः* create_Pattern_कुञ्चिका(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        जइ (ध्रुव AstParseRef* ध्रुव refp = VN_CAST(exprp, ParseRef)) {
            जइ (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_ग्रन्थिः* ध्रुव textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_वृक्षः(), exprp);
                प्रत्यागम textp;
            }
        }
        प्रत्यागम exprp;
    }
    AstDisplay* createDisplayError(File_पङ्क्तिः* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_ग्रन्थिः::addNext<Ast_ग्रन्थिः, Ast_ग्रन्थिः>(nodep, new AstStop{fileline, false});
        प्रत्यागम nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    स्थिर AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_ग्रन्थिः_व्यञ्जकम्* sizep) {
        जइ (!sizep) प्रत्यागम binp;
        binp->isArray(true);
        जइ (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_वृक्षः(), sizep);
        } अण्णहा {
            binp->arraySizep(sizep);
        }
        प्रत्यागम binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    स्थिर bool coverBinArrayUnsized(Ast_ग्रन्थिः_व्यञ्जकम्* sizep) {
        जइ (!sizep) प्रत्यागम false;
        जइ (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_वृक्षः(), sizep);
        प्रत्यागम true;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* createGatePin(Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        AstRange* ध्रुव rangep = m_gateRangep;
        जइ (!rangep) प्रत्यागम exprp;
        प्रत्यागम new AstGatePin{rangep->fileline(), exprp, rangep->clone_वृक्षः(true)};
    }
    Ast_Sen_वृक्षः* create_Sen_वृक्षः_Changed(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp) {
        प्रत्यागम new Ast_Sen_वृक्षः{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_वृक्षः* create_Sen_वृक्षः_Dot_Star(File_पङ्क्तिः* fl) {
        प्रत्यागम new Ast_Sen_वृक्षः{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* createGlobalClockParseRef(File_पङ्क्तिः* fl) {
        प्रत्यागम new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_वृक्षः* create_Global_Clock_Sen_वृक्षः(File_पङ्क्तिः* fl) {
        प्रत्यागम create_Sen_वृक्षः_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_ग्रन्थिः* createNettype(File_पङ्क्तिः* fl, ध्रुव string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ध्रुव nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        प्रत्यागम nodep;
    }
    Ast_ग्रन्थिः* createTypedef(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* attrsp, Ast_ग्रन्थिः_DType* basep,
                           Ast_ग्रन्थिः_Range* rangep) {
        AstTypedef* ध्रुव nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        प्रत्यागम nodep;
    }
    Ast_ग्रन्थिः* createTypedefFwd(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव VFwdType& fwdType) {
        AstTypedefFwd* ध्रुव nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        प्रत्यागम nodep;
    }
    स्थिर शून्य endLabel(File_पङ्क्तिः* fl, ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    स्थिर शून्य endLabel(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string* endnamep) {
        जइ (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_ग्रन्थिः::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    शून्य setDType(Ast_ग्रन्थिः_DType* dtypep) {
        जइ (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_वृक्षः(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    शून्य setNetDelay(AstDelay* netDelayp) {
        जइ (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_वृक्षः(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    शून्य setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    शून्य pin_प्रक्षिप() {
        m_pin_राशिः.प्रक्षिप(m_pin_अङ्कः);
        m_pin_अङ्कः = 1;
    }
    शून्य pin_निष्कासय(File_पङ्क्तिः* fl) {
        जइ (VL_UNCOVERABLE(m_pin_राशिः.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_अङ्कः = m_pin_राशिः.शिखरम्();
        m_pin_राशिः.निष्कासय();
    }
    Ast_ग्रन्थिः_DType* addRange(AstBasicDType* dtypep, Ast_ग्रन्थिः_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        जइ (!rangesp) प्रत्यागम dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_ग्रन्थिः_Range* rangearraysp = nullptr;
        जइ (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } अण्णहा {
            Ast_ग्रन्थिः_Range* finalp = rangesp;
            जाव (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            जइ (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            जइ (AstRange* ध्रुव finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                जइ (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ध्रुव newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_वृक्षः(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        प्रत्यागम createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_पङ्क्तिः* fileline, ध्रुव std::string& text) VL_MT_DISABLED;
    शून्य checkDpiVer(File_पङ्क्तिः* fileline, ध्रुव string& str) {
        जइ (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_पङ्क्तिः* flp, ध्रुव VDirection direction,
                                          Ast_ग्रन्थिः_व्यञ्जकम्* skewp, Ast_ग्रन्थिः* ध्रुव clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        प्रति (Ast_ग्रन्थिः *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            जइ (nextp) nextp->unlinkFrBackWithNext();
            जइ (itemsp && skewp) skewp = skewp->clone_वृक्षः(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_ग्रन्थिः::addNextNull(itemsp, itemp);
        }
        प्रत्यागम itemsp;
    }

    शून्य setScopedSigAttr(Ast_ग्रन्थिः* attrsp) {
        जइ (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_वृक्षः(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    शून्य createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_ग्रन्थिः* cloneScopedSigAttr() ध्रुव {
        प्रत्यागम m_scopedSigAttr ? m_scopedSigAttr->clone_वृक्षः(true) : nullptr;
    }

    शून्य createGenericIface(Ast_ग्रन्थिः* ध्रुव nodep, Ast_ग्रन्थिः_Range* ध्रुव rangep,
                            Ast_ग्रन्थिः* sigAttrListp, File_पङ्क्तिः* ध्रुव modportFileline = nullptr,
                            ध्रुव string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ध्रुव std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_ग्रन्थिः::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_वृक्षः(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ध्रुव refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::निवेशः;
        Ast_ग्रन्थिः::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    स्थिर AstBegin* wrapInBegin(Ast_ग्रन्थिः_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        प्रति (Ast_ग्रन्थिः_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ग्रन्थिः_Stmt);
            जइ (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            जइ (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_ग्रन्थिः::addNext(resp, beginp);
        }
        प्रत्यागम resp;
    }
};
