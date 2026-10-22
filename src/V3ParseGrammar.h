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

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Const.h"
#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#અન્તર્ભાવ <stack>
#અન્તર્ભાવ <vector>

class V3ParseGrammar final {
જાહેર:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_ગાંઠ* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_ગાંઠ_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_ગાંઠ_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_લીટી* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_ગાંઠ* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    પૂર્ણાંક m_pin_અંક = -1;  // Pin number currently parsing
    std::ઢગલો<પૂર્ણાંક> m_pin_ઢગલો;  // Queue of pin numbers being parsed

    સ્થિર પૂર્ણાંક s_type_Imp_અંક;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = મૂળભૂત;
    સ્થિર V3ParseGrammar* singletonp() {
        સ્થિર V3ParseGrammar singleton;
        પાછા &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_ગાંઠ_પદાવલિ* nodep) VL_MT_DISABLED;
    bool allTracingOn(અચલ File_લીટી* fl) અચલ {
        પાછા v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_ગાંઠ_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_ગાંઠ_Range* rangep) VL_MT_DISABLED;
    Ast_ગાંઠ_Pre_Sel* scrubSel(Ast_ગાંઠ_પદાવલિ* fromp, Ast_ગાંઠ_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_ગાંઠ_DType* createArray(Ast_ગાંઠ_DType* basep, Ast_ગાંઠ_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_લીટી* fileline, અચલ string& name, Ast_ગાંઠ_Range* arrayp,
                           Ast_ગાંઠ* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_પદાવલિ(File_લીટી* fileline, અચલ string& name, પૂર્ણાંક કિંમત) VL_MT_DISABLED;
    std::string textQuoted(File_લીટી* fileline, અચલ std::string& text) {
        પાછા singletonp()->unquoteString(fileline, text);
    }
    Ast_ગાંઠ* createCell(File_લીટી* fileline, અચલ string& name, AstPin* pinlistp,
                        Ast_ગાંઠ_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* અચલ nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_વૃક્ષ(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        પાછા nodep;
    }
    સ્થિર Ast_ગાંઠ* create_Pattern_ચાવી(Ast_ગાંઠ_પદાવલિ* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        જો (અચલ AstParseRef* અચલ refp = VN_CAST(exprp, ParseRef)) {
            જો (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_ગાંઠ* અચલ textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_વૃક્ષ(), exprp);
                પાછા textp;
            }
        }
        પાછા exprp;
    }
    AstDisplay* createDisplayError(File_લીટી* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_ગાંઠ::addNext<Ast_ગાંઠ, Ast_ગાંઠ>(nodep, new AstStop{fileline, false});
        પાછા nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    સ્થિર AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_ગાંઠ_પદાવલિ* sizep) {
        જો (!sizep) પાછા binp;
        binp->isArray(true);
        જો (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_વૃક્ષ(), sizep);
        } નહીંતર {
            binp->arraySizep(sizep);
        }
        પાછા binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    સ્થિર bool coverBinArrayUnsized(Ast_ગાંઠ_પદાવલિ* sizep) {
        જો (!sizep) પાછા false;
        જો (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_વૃક્ષ(), sizep);
        પાછા true;
    }
    Ast_ગાંઠ_પદાવલિ* createGatePin(Ast_ગાંઠ_પદાવલિ* exprp) {
        AstRange* અચલ rangep = m_gateRangep;
        જો (!rangep) પાછા exprp;
        પાછા new AstGatePin{rangep->fileline(), exprp, rangep->clone_વૃક્ષ(true)};
    }
    Ast_Sen_વૃક્ષ* create_Sen_વૃક્ષ_Changed(File_લીટી* fl, Ast_ગાંઠ_પદાવલિ* exprp) {
        પાછા new Ast_Sen_વૃક્ષ{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_વૃક્ષ* create_Sen_વૃક્ષ_Dot_Star(File_લીટી* fl) {
        પાછા new Ast_Sen_વૃક્ષ{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_ગાંઠ_પદાવલિ* createGlobalClockParseRef(File_લીટી* fl) {
        પાછા new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_વૃક્ષ* create_Global_Clock_Sen_વૃક્ષ(File_લીટી* fl) {
        પાછા create_Sen_વૃક્ષ_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_ગાંઠ* createNettype(File_લીટી* fl, અચલ string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* અચલ nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        પાછા nodep;
    }
    Ast_ગાંઠ* createTypedef(File_લીટી* fl, અચલ string& name, Ast_ગાંઠ* attrsp, Ast_ગાંઠ_DType* basep,
                           Ast_ગાંઠ_Range* rangep) {
        AstTypedef* અચલ nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        પાછા nodep;
    }
    Ast_ગાંઠ* createTypedefFwd(File_લીટી* fl, અચલ string& name, અચલ VFwdType& fwdType) {
        AstTypedefFwd* અચલ nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        પાછા nodep;
    }
    સ્થિર શૂન્ય endLabel(File_લીટી* fl, અચલ Ast_ગાંઠ* nodep, અચલ string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    સ્થિર શૂન્ય endLabel(File_લીટી* fl, અચલ string& name, અચલ string* endnamep) {
        જો (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_ગાંઠ::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    શૂન્ય setDType(Ast_ગાંઠ_DType* dtypep) {
        જો (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_વૃક્ષ(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    શૂન્ય setNetDelay(AstDelay* netDelayp) {
        જો (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_વૃક્ષ(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    શૂન્ય setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    શૂન્ય pin_ધકેલો() {
        m_pin_ઢગલો.ધકેલો(m_pin_અંક);
        m_pin_અંક = 1;
    }
    શૂન્ય pin_કાઢો(File_લીટી* fl) {
        જો (VL_UNCOVERABLE(m_pin_ઢગલો.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_અંક = m_pin_ઢગલો.ટોચ();
        m_pin_ઢગલો.કાઢો();
    }
    Ast_ગાંઠ_DType* addRange(AstBasicDType* dtypep, Ast_ગાંઠ_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        જો (!rangesp) પાછા dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_ગાંઠ_Range* rangearraysp = nullptr;
        જો (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } નહીંતર {
            Ast_ગાંઠ_Range* finalp = rangesp;
            જ્યારે (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            જો (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            જો (AstRange* અચલ finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                જો (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* અચલ newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_વૃક્ષ(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        પાછા createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_લીટી* fileline, અચલ std::string& text) VL_MT_DISABLED;
    શૂન્ય checkDpiVer(File_લીટી* fileline, અચલ string& str) {
        જો (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_લીટી* flp, અચલ VDirection direction,
                                          Ast_ગાંઠ_પદાવલિ* skewp, Ast_ગાંઠ* અચલ clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        માટે (Ast_ગાંઠ *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            જો (nextp) nextp->unlinkFrBackWithNext();
            જો (itemsp && skewp) skewp = skewp->clone_વૃક્ષ(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_ગાંઠ::addNextNull(itemsp, itemp);
        }
        પાછા itemsp;
    }

    શૂન્ય setScopedSigAttr(Ast_ગાંઠ* attrsp) {
        જો (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_વૃક્ષ(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    શૂન્ય createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_ગાંઠ* cloneScopedSigAttr() અચલ {
        પાછા m_scopedSigAttr ? m_scopedSigAttr->clone_વૃક્ષ(true) : nullptr;
    }

    શૂન્ય createGenericIface(Ast_ગાંઠ* અચલ nodep, Ast_ગાંઠ_Range* અચલ rangep,
                            Ast_ગાંઠ* sigAttrListp, File_લીટી* અચલ modportFileline = nullptr,
                            અચલ string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        અચલ std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_ગાંઠ::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_વૃક્ષ(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* અચલ refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::આગત;
        Ast_ગાંઠ::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    સ્થિર AstBegin* wrapInBegin(Ast_ગાંઠ_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        માટે (Ast_ગાંઠ_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ગાંઠ_Stmt);
            જો (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            જો (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_ગાંઠ::addNext(resp, beginp);
        }
        પાછા resp;
    }
};
