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
    Ast_गाँठो* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_गाँठो_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_गाँठो_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_पङ्क्ति* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_गाँठो* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    पूर्णाङ्क m_pin_अङ्क = -1;  // Pin number currently parsing
    std::थाक<पूर्णाङ्क> m_pin_थाक;  // Queue of pin numbers being parsed

    स्थिर पूर्णाङ्क s_type_Imp_अङ्क;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = स्वभाव;
    स्थिर V3ParseGrammar* singletonp() {
        स्थिर V3ParseGrammar singleton;
        फर्काऊ &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_गाँठो_अभिव्यक्ति* nodep) VL_MT_DISABLED;
    bool allTracingOn(अचर File_पङ्क्ति* fl) अचर {
        फर्काऊ v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_गाँठो_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_गाँठो_Range* rangep) VL_MT_DISABLED;
    Ast_गाँठो_Pre_Sel* scrubSel(Ast_गाँठो_अभिव्यक्ति* fromp, Ast_गाँठो_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_गाँठो_DType* createArray(Ast_गाँठो_DType* basep, Ast_गाँठो_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_पङ्क्ति* fileline, अचर string& name, Ast_गाँठो_Range* arrayp,
                           Ast_गाँठो* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_अभिव्यक्ति(File_पङ्क्ति* fileline, अचर string& name, पूर्णाङ्क मान) VL_MT_DISABLED;
    std::string textQuoted(File_पङ्क्ति* fileline, अचर std::string& text) {
        फर्काऊ singletonp()->unquoteString(fileline, text);
    }
    Ast_गाँठो* createCell(File_पङ्क्ति* fileline, अचर string& name, AstPin* pinlistp,
                        Ast_गाँठो_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* अचर nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_रूख(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        फर्काऊ nodep;
    }
    स्थिर Ast_गाँठो* create_Pattern_साँचो(Ast_गाँठो_अभिव्यक्ति* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        यदि (अचर AstParseRef* अचर refp = VN_CAST(exprp, ParseRef)) {
            यदि (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_गाँठो* अचर textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_रूख(), exprp);
                फर्काऊ textp;
            }
        }
        फर्काऊ exprp;
    }
    AstDisplay* createDisplayError(File_पङ्क्ति* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_गाँठो::addNext<Ast_गाँठो, Ast_गाँठो>(nodep, new AstStop{fileline, false});
        फर्काऊ nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    स्थिर AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_गाँठो_अभिव्यक्ति* sizep) {
        यदि (!sizep) फर्काऊ binp;
        binp->isArray(true);
        यदि (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_रूख(), sizep);
        } अन्यथा {
            binp->arraySizep(sizep);
        }
        फर्काऊ binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    स्थिर bool coverBinArrayUnsized(Ast_गाँठो_अभिव्यक्ति* sizep) {
        यदि (!sizep) फर्काऊ false;
        यदि (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_रूख(), sizep);
        फर्काऊ true;
    }
    Ast_गाँठो_अभिव्यक्ति* createGatePin(Ast_गाँठो_अभिव्यक्ति* exprp) {
        AstRange* अचर rangep = m_gateRangep;
        यदि (!rangep) फर्काऊ exprp;
        फर्काऊ new AstGatePin{rangep->fileline(), exprp, rangep->clone_रूख(true)};
    }
    Ast_Sen_रूख* create_Sen_रूख_Changed(File_पङ्क्ति* fl, Ast_गाँठो_अभिव्यक्ति* exprp) {
        फर्काऊ new Ast_Sen_रूख{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_रूख* create_Sen_रूख_Dot_Star(File_पङ्क्ति* fl) {
        फर्काऊ new Ast_Sen_रूख{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_गाँठो_अभिव्यक्ति* createGlobalClockParseRef(File_पङ्क्ति* fl) {
        फर्काऊ new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_रूख* create_Global_Clock_Sen_रूख(File_पङ्क्ति* fl) {
        फर्काऊ create_Sen_रूख_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_गाँठो* createNettype(File_पङ्क्ति* fl, अचर string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* अचर nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        फर्काऊ nodep;
    }
    Ast_गाँठो* createTypedef(File_पङ्क्ति* fl, अचर string& name, Ast_गाँठो* attrsp, Ast_गाँठो_DType* basep,
                           Ast_गाँठो_Range* rangep) {
        AstTypedef* अचर nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        फर्काऊ nodep;
    }
    Ast_गाँठो* createTypedefFwd(File_पङ्क्ति* fl, अचर string& name, अचर VFwdType& fwdType) {
        AstTypedefFwd* अचर nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        फर्काऊ nodep;
    }
    स्थिर शून्य endLabel(File_पङ्क्ति* fl, अचर Ast_गाँठो* nodep, अचर string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    स्थिर शून्य endLabel(File_पङ्क्ति* fl, अचर string& name, अचर string* endnamep) {
        यदि (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_गाँठो::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    शून्य setDType(Ast_गाँठो_DType* dtypep) {
        यदि (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_रूख(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    शून्य setNetDelay(AstDelay* netDelayp) {
        यदि (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_रूख(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    शून्य setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    शून्य pin_थप() {
        m_pin_थाक.थप(m_pin_अङ्क);
        m_pin_अङ्क = 1;
    }
    शून्य pin_झिक(File_पङ्क्ति* fl) {
        यदि (VL_UNCOVERABLE(m_pin_थाक.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_अङ्क = m_pin_थाक.शीर्ष();
        m_pin_थाक.झिक();
    }
    Ast_गाँठो_DType* addRange(AstBasicDType* dtypep, Ast_गाँठो_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        यदि (!rangesp) फर्काऊ dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_गाँठो_Range* rangearraysp = nullptr;
        यदि (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } अन्यथा {
            Ast_गाँठो_Range* finalp = rangesp;
            जबसम्म (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            यदि (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            यदि (AstRange* अचर finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                यदि (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* अचर newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_रूख(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        फर्काऊ createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_पङ्क्ति* fileline, अचर std::string& text) VL_MT_DISABLED;
    शून्य checkDpiVer(File_पङ्क्ति* fileline, अचर string& str) {
        यदि (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_पङ्क्ति* flp, अचर VDirection direction,
                                          Ast_गाँठो_अभिव्यक्ति* skewp, Ast_गाँठो* अचर clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        लागि (Ast_गाँठो *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            यदि (nextp) nextp->unlinkFrBackWithNext();
            यदि (itemsp && skewp) skewp = skewp->clone_रूख(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_गाँठो::addNextNull(itemsp, itemp);
        }
        फर्काऊ itemsp;
    }

    शून्य setScopedSigAttr(Ast_गाँठो* attrsp) {
        यदि (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_रूख(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    शून्य createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_गाँठो* cloneScopedSigAttr() अचर {
        फर्काऊ m_scopedSigAttr ? m_scopedSigAttr->clone_रूख(true) : nullptr;
    }

    शून्य createGenericIface(Ast_गाँठो* अचर nodep, Ast_गाँठो_Range* अचर rangep,
                            Ast_गाँठो* sigAttrListp, File_पङ्क्ति* अचर modportFileline = nullptr,
                            अचर string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        अचर std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_गाँठो::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_रूख(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* अचर refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::आगत;
        Ast_गाँठो::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    स्थिर AstBegin* wrapInBegin(Ast_गाँठो_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        लागि (Ast_गाँठो_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), गाँठो_Stmt);
            यदि (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            यदि (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_गाँठो::addNext(resp, beginp);
        }
        फर्काऊ resp;
    }
};
