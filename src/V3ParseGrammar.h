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

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#ಅನ್ತರ್ಭಾವ <stack>
#ಅನ್ತರ್ಭಾವ <vector>

class V3ParseGrammar final {
ಸಾರ್ವಜನಿಕ:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_ಗಂಟು* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_ಗಂಟು_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_ಗಂಟು_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_ಸಾಲು* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_ಗಂಟು* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    ಪೂರ್ಣಾಙ್ಕ m_pin_ಅಂಕೆ = -1;  // Pin number currently parsing
    std::ರಾಶಿ<ಪೂರ್ಣಾಙ್ಕ> m_pin_ರಾಶಿ;  // Queue of pin numbers being parsed

    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_type_Imp_ಅಂಕೆ;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = ಸ್ವಭಾವ;
    ಸ್ಥಿರ V3ParseGrammar* singletonp() {
        ಸ್ಥಿರ V3ParseGrammar singleton;
        ಪ್ರತ್ಯಾಗಮ &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) VL_MT_DISABLED;
    bool allTracingOn(ಧ್ರುವ File_ಸಾಲು* fl) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_ಗಂಟು_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_ಗಂಟು_Range* rangep) VL_MT_DISABLED;
    Ast_ಗಂಟು_Pre_Sel* scrubSel(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_ಗಂಟು_DType* createArray(Ast_ಗಂಟು_DType* basep, Ast_ಗಂಟು_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_ಸಾಲು* fileline, ಧ್ರುವ string& name, Ast_ಗಂಟು_Range* arrayp,
                           Ast_ಗಂಟು* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fileline, ಧ್ರುವ string& name, ಪೂರ್ಣಾಙ್ಕ ಮೌಲ್ಯ) VL_MT_DISABLED;
    std::string textQuoted(File_ಸಾಲು* fileline, ಧ್ರುವ std::string& text) {
        ಪ್ರತ್ಯಾಗಮ singletonp()->unquoteString(fileline, text);
    }
    Ast_ಗಂಟು* createCell(File_ಸಾಲು* fileline, ಧ್ರುವ string& name, AstPin* pinlistp,
                        Ast_ಗಂಟು_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ಧ್ರುವ nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_ವೃಕ್ಷ(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು* create_Pattern_ಕೀಲಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        ಯದಿ (ಧ್ರುವ AstParseRef* ಧ್ರುವ refp = VN_CAST(exprp, ParseRef)) {
            ಯದಿ (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_ಗಂಟು* ಧ್ರುವ textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
                ಪ್ರತ್ಯಾಗಮ textp;
            }
        }
        ಪ್ರತ್ಯಾಗಮ exprp;
    }
    AstDisplay* createDisplayError(File_ಸಾಲು* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_ಗಂಟು::addNext<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, new AstStop{fileline, false});
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ಸ್ಥಿರ AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizep) {
        ಯದಿ (!sizep) ಪ್ರತ್ಯಾಗಮ binp;
        binp->isArray(true);
        ಯದಿ (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_ವೃಕ್ಷ(), sizep);
        } ಅನ್ಯಥಾ {
            binp->arraySizep(sizep);
        }
        ಪ್ರತ್ಯಾಗಮ binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ಸ್ಥಿರ bool coverBinArrayUnsized(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizep) {
        ಯದಿ (!sizep) ಪ್ರತ್ಯಾಗಮ false;
        ಯದಿ (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_ವೃಕ್ಷ(), sizep);
        ಪ್ರತ್ಯಾಗಮ true;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createGatePin(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        AstRange* ಧ್ರುವ rangep = m_gateRangep;
        ಯದಿ (!rangep) ಪ್ರತ್ಯಾಗಮ exprp;
        ಪ್ರತ್ಯಾಗಮ new AstGatePin{rangep->fileline(), exprp, rangep->clone_ವೃಕ್ಷ(true)};
    }
    Ast_Sen_ವೃಕ್ಷ* create_Sen_ವೃಕ್ಷ_Changed(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಪ್ರತ್ಯಾಗಮ new Ast_Sen_ವೃಕ್ಷ{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_ವೃಕ್ಷ* create_Sen_ವೃಕ್ಷ_Dot_Star(File_ಸಾಲು* fl) {
        ಪ್ರತ್ಯಾಗಮ new Ast_Sen_ವೃಕ್ಷ{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createGlobalClockParseRef(File_ಸಾಲು* fl) {
        ಪ್ರತ್ಯಾಗಮ new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_ವೃಕ್ಷ* create_Global_Clock_Sen_ವೃಕ್ಷ(File_ಸಾಲು* fl) {
        ಪ್ರತ್ಯಾಗಮ create_Sen_ವೃಕ್ಷ_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_ಗಂಟು* createNettype(File_ಸಾಲು* fl, ಧ್ರುವ string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ಧ್ರುವ nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    Ast_ಗಂಟು* createTypedef(File_ಸಾಲು* fl, ಧ್ರುವ string& name, Ast_ಗಂಟು* attrsp, Ast_ಗಂಟು_DType* basep,
                           Ast_ಗಂಟು_Range* rangep) {
        AstTypedef* ಧ್ರುವ nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    Ast_ಗಂಟು* createTypedefFwd(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ VFwdType& fwdType) {
        AstTypedefFwd* ಧ್ರುವ nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ endLabel(File_ಸಾಲು* fl, ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ endLabel(File_ಸಾಲು* fl, ಧ್ರುವ string& name, ಧ್ರುವ string* endnamep) {
        ಯದಿ (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_ಗಂಟು::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    ಶೂನ್ಯ setDType(Ast_ಗಂಟು_DType* dtypep) {
        ಯದಿ (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_ವೃಕ್ಷ(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    ಶೂನ್ಯ setNetDelay(AstDelay* netDelayp) {
        ಯದಿ (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_ವೃಕ್ಷ(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    ಶೂನ್ಯ setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    ಶೂನ್ಯ pin_ತಳ್ಳು() {
        m_pin_ರಾಶಿ.ತಳ್ಳು(m_pin_ಅಂಕೆ);
        m_pin_ಅಂಕೆ = 1;
    }
    ಶೂನ್ಯ pin_ತೆಗೆ(File_ಸಾಲು* fl) {
        ಯದಿ (VL_UNCOVERABLE(m_pin_ರಾಶಿ.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_ಅಂಕೆ = m_pin_ರಾಶಿ.ತುದಿ();
        m_pin_ರಾಶಿ.ತೆಗೆ();
    }
    Ast_ಗಂಟು_DType* addRange(AstBasicDType* dtypep, Ast_ಗಂಟು_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        ಯದಿ (!rangesp) ಪ್ರತ್ಯಾಗಮ dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_ಗಂಟು_Range* rangearraysp = nullptr;
        ಯದಿ (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } ಅನ್ಯಥಾ {
            Ast_ಗಂಟು_Range* finalp = rangesp;
            ಯಾವತ್ (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            ಯದಿ (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            ಯದಿ (AstRange* ಧ್ರುವ finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                ಯದಿ (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ಧ್ರುವ newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_ವೃಕ್ಷ(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        ಪ್ರತ್ಯಾಗಮ createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_ಸಾಲು* fileline, ಧ್ರುವ std::string& text) VL_MT_DISABLED;
    ಶೂನ್ಯ checkDpiVer(File_ಸಾಲು* fileline, ಧ್ರುವ string& str) {
        ಯದಿ (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_ಸಾಲು* flp, ಧ್ರುವ VDirection direction,
                                          Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* skewp, Ast_ಗಂಟು* ಧ್ರುವ clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ಯದಿ (nextp) nextp->unlinkFrBackWithNext();
            ಯದಿ (itemsp && skewp) skewp = skewp->clone_ವೃಕ್ಷ(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_ಗಂಟು::addNextNull(itemsp, itemp);
        }
        ಪ್ರತ್ಯಾಗಮ itemsp;
    }

    ಶೂನ್ಯ setScopedSigAttr(Ast_ಗಂಟು* attrsp) {
        ಯದಿ (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_ವೃಕ್ಷ(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    ಶೂನ್ಯ createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_ಗಂಟು* cloneScopedSigAttr() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_scopedSigAttr ? m_scopedSigAttr->clone_ವೃಕ್ಷ(true) : nullptr;
    }

    ಶೂನ್ಯ createGenericIface(Ast_ಗಂಟು* ಧ್ರುವ nodep, Ast_ಗಂಟು_Range* ಧ್ರುವ rangep,
                            Ast_ಗಂಟು* sigAttrListp, File_ಸಾಲು* ಧ್ರುವ modportFileline = nullptr,
                            ಧ್ರುವ string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ಧ್ರುವ std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_ಗಂಟು::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_ವೃಕ್ಷ(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ಧ್ರುವ refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::ಒಳಹರಿವು;
        Ast_ಗಂಟು::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ಸ್ಥಿರ AstBegin* wrapInBegin(Ast_ಗಂಟು_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ಗಂಟು_Stmt);
            ಯದಿ (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            ಯದಿ (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_ಗಂಟು::addNext(resp, beginp);
        }
        ಪ್ರತ್ಯಾಗಮ resp;
    }
};
