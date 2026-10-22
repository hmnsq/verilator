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
    V3Control::Var_Spec_ಬಗೆ m_vlt_Var_Spec_ಬಗೆ = V3Control::Var_Spec_ಬಗೆ::VAR;
    bool m_impliedDecl = false;  // Allow implied wire declarations
    bool m_varDeclTyped = false;  // Var got reg/wire for dedup check
    bool m_pinAnsi = false;  // In ANSI parameter or port list
    bool m_tracingParse = true;  // Tracing disable for parser
    bool m_ಒಳ_Implements = false;  // Is inside class implements list
    bool m_insideProperty = false;  // Is inside property declaration
    bool m_specifyignWarned = false;  // Issued a SPECIFYIGN warning
    bool m_typed_Property_ಪೋರ್ಟ್ = false;  // Typed property port occurred on port lists
    bool m_modport_Imp_Exp_ಸಕ್ರಿಯ
        = false;  // Standalone ID is a tf_identifier instead of port_identifier
    bool m_modport_Imp_Exp_ಕೊನೆಯ_Is_Export
        = false;  // Last import_export statement in modportPortsDecl is an export
    Ast_ಗಂಟು* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    ಪೂರ್ಣಾಂಕ m_pin_ಅಂಕೆ = -1;  // Pin number currently parsing
    std::ರಾಶಿ<ಪೂರ್ಣಾಂಕ> m_pin_ರಾಶಿ;  // Queue of pin numbers being parsed

    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_type_Imp_ಅಂಕೆ;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಸ್ಥಿರ V3ParseGrammar* singletonp() {
        ಸ್ಥಿರ V3ParseGrammar singleton;
        ಹಿಂತಿರುಗಿ &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) VL_MT_DISABLED;
    bool allTracingOn(ಸ್ಥಿರಾಂಕ File_ಸಾಲು* fl) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_ಗಂಟು_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_ಗಂಟು_Range* rangep) VL_MT_DISABLED;
    Ast_ಗಂಟು_Pre_Sel* scrubSel(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, Ast_ಗಂಟು_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_ಗಂಟು_DType* createArray(Ast_ಗಂಟು_DType* basep, Ast_ಗಂಟು_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು_Range* arrayp,
                           Ast_ಗಂಟು* attrsp) VL_MT_DISABLED;
    Ast_ನಿಯೋಜಿಸು_W* create_Supply_ಅಭಿವ್ಯಕ್ತಿ(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& name, ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) VL_MT_DISABLED;
    std::string ಪಠ್ಯ_Quoted(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) {
        ಹಿಂತಿರುಗಿ singletonp()->unquoteString(fileline, ಪಠ್ಯ);
    }
    Ast_ಗಂಟು* createCell(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& name, AstPin* pinlistp,
                        Ast_ಗಂಟು_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ಸ್ಥಿರಾಂಕ nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_ವೃಕ್ಷ(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        ಹಿಂತಿರುಗಿ nodep;
    }
    ಸ್ಥಿರ Ast_ಗಂಟು* create_ನಮೂನೆ_ಕೀಲಿ(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        ಆದರೆ (ಸ್ಥಿರಾಂಕ AstParseRef* ಸ್ಥಿರಾಂಕ refp = VN_CAST(exprp, ParseRef)) {
            ಆದರೆ (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ textp = new Ast_ಪಠ್ಯ{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
                ಹಿಂತಿರುಗಿ textp;
            }
        }
        ಹಿಂತಿರುಗಿ exprp;
    }
    AstDisplay* createDisplayError(File_ಸಾಲು* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ<Ast_ಗಂಟು, Ast_ಗಂಟು>(nodep, new AstStop{fileline, false});
        ಹಿಂತಿರುಗಿ nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ಸ್ಥಿರ AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizep) {
        ಆದರೆ (!sizep) ಹಿಂತಿರುಗಿ binp;
        binp->isArray(true);
        ಆದರೆ (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_ವೃಕ್ಷ(), sizep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            binp->arraySizep(sizep);
        }
        ಹಿಂತಿರುಗಿ binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ಸ್ಥಿರ bool coverBinArrayUnsized(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sizep) {
        ಆದರೆ (!sizep) ಹಿಂತಿರುಗಿ false;
        ಆದರೆ (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_ವೃಕ್ಷ(), sizep);
        ಹಿಂತಿರುಗಿ true;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createGatePin(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        AstRange* ಸ್ಥಿರಾಂಕ rangep = m_gateRangep;
        ಆದರೆ (!rangep) ಹಿಂತಿರುಗಿ exprp;
        ಹಿಂತಿರುಗಿ new AstGatePin{rangep->fileline(), exprp, rangep->clone_ವೃಕ್ಷ(true)};
    }
    Ast_Sen_ವೃಕ್ಷ* create_Sen_ವೃಕ್ಷ_ಬದಲಾಯಿತು(File_ಸಾಲು* fl, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp) {
        ಹಿಂತಿರುಗಿ new Ast_Sen_ವೃಕ್ಷ{fl, new Ast_Sen_ಅಂಶ{fl, VEdgeType::ET_ಬದಲಾಯಿತು, exprp}};
    }
    Ast_Sen_ವೃಕ್ಷ* create_Sen_ವೃಕ್ಷ_Dot_ನಕ್ಷತ್ರ(File_ಸಾಲು* fl) {
        ಹಿಂತಿರುಗಿ new Ast_Sen_ವೃಕ್ಷ{fl, new Ast_Sen_ಅಂಶ{fl, VEdgeType::ET_COMBO_ನಕ್ಷತ್ರ, nullptr}};
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* createGlobalClockParseRef(File_ಸಾಲು* fl) {
        ಹಿಂತಿರುಗಿ new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_ವೃಕ್ಷ* create_Global_Clock_Sen_ವೃಕ್ಷ(File_ಸಾಲು* fl) {
        ಹಿಂತಿರುಗಿ create_Sen_ವೃಕ್ಷ_ಬದಲಾಯಿತು(fl, createGlobalClockParseRef(fl));
    }
    Ast_ಗಂಟು* createNettype(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ಸ್ಥಿರಾಂಕ nodep = new AstTypedef{fl, name, nullptr, VFlag_ಮಗು_DType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->ಟ್ಯಾಗ್_Nodep(nodep);
        ಹಿಂತಿರುಗಿ nodep;
    }
    Ast_ಗಂಟು* createTypedef(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, Ast_ಗಂಟು* attrsp, Ast_ಗಂಟು_DType* basep,
                           Ast_ಗಂಟು_Range* rangep) {
        AstTypedef* ಸ್ಥಿರಾಂಕ nodep = new AstTypedef{fl, name, attrsp, VFlag_ಮಗು_DType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->ಟ್ಯಾಗ್_Nodep(nodep);
        ಹಿಂತಿರುಗಿ nodep;
    }
    Ast_ಗಂಟು* createTypedefFwd(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ VFwdType& fwdType) {
        AstTypedefFwd* ಸ್ಥಿರಾಂಕ nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->ಟ್ಯಾಗ್_Nodep(nodep);
        ಹಿಂತಿರುಗಿ nodep;
    }
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೊನೆ_Label(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string* endnamep) {
        ಕೊನೆ_Label(fl, nodep->prettyName(), endnamep);
    }
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೊನೆ_Label(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& name, ಸ್ಥಿರಾಂಕ string* endnamep) {
        ಆದರೆ (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_ಗಂಟು::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    ಶೂನ್ಯ ಗಣ_DType(Ast_ಗಂಟು_DType* dtypep) {
        ಆದರೆ (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_ವೃಕ್ಷ(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    ಶೂನ್ಯ ಗಣ_Net_Delay(AstDelay* netDelayp) {
        ಆದರೆ (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_ವೃಕ್ಷ(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    ಶೂನ್ಯ ಗಣ_Net_Strength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    ಶೂನ್ಯ pin_ತಳ್ಳು() {
        m_pin_ರಾಶಿ.ತಳ್ಳು(m_pin_ಅಂಕೆ);
        m_pin_ಅಂಕೆ = 1;
    }
    ಶೂನ್ಯ pin_ತೆಗೆ(File_ಸಾಲು* fl) {
        ಆದರೆ (VL_UNCOVERABLE(m_pin_ರಾಶಿ.ಖಾಲಿಗಳು())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_ಅಂಕೆ = m_pin_ರಾಶಿ.ತುದಿ();
        m_pin_ರಾಶಿ.ತೆಗೆ_2();
    }
    Ast_ಗಂಟು_DType* ಕೂಡಿಸು_Range(AstBasicDType* dtypep, Ast_ಗಂಟು_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        ಆದರೆ (!rangesp) ಹಿಂತಿರುಗಿ dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_ಗಂಟು_Range* rangearraysp = nullptr;
        ಆದರೆ (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } ಇಲ್ಲದಿದ್ದರೆ {
            Ast_ಗಂಟು_Range* finalp = rangesp;
            ಆಗ (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            ಆದರೆ (finalp != rangesp) {
                finalp->unlink_Fr_ಹಿಂಭಾಗ();
                rangearraysp = rangesp;
            }
            ಆದರೆ (AstRange* ಸ್ಥಿರಾಂಕ finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                ಆದರೆ (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ಸ್ಥಿರಾಂಕ newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->ಅಗಲ(), dtypep->ಅಗಲ_Min()};
                    VL_DO_DANGLING(dtypep->delete_ವೃಕ್ಷ(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        ಹಿಂತಿರುಗಿ createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ std::string& ಪಠ್ಯ) VL_MT_DISABLED;
    ಶೂನ್ಯ checkDpiVer(File_ಸಾಲು* fileline, ಸ್ಥಿರಾಂಕ string& str) {
        ಆದರೆ (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    Ast_Clocking_ಅಂಶ* make_Clocking_ಅಂಶ_List(File_ಸಾಲು* flp, ಸ್ಥಿರಾಂಕ VDirection direction,
                                          Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* skewp, Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ clockingDeclps) {
        Ast_Clocking_ಅಂಶ* itemsp = nullptr;
        ಗಾಗಿ (Ast_ಗಂಟು *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ಆದರೆ (nextp) nextp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            ಆದರೆ (itemsp && skewp) skewp = skewp->clone_ವೃಕ್ಷ(false);
            Ast_Clocking_ಅಂಶ* itemp = new Ast_Clocking_ಅಂಶ{flp, direction, skewp, nodep};
            itemsp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ_Null(itemsp, itemp);
        }
        ಹಿಂತಿರುಗಿ itemsp;
    }

    ಶೂನ್ಯ ಗಣ_Scoped_Sig_Attr(Ast_ಗಂಟು* attrsp) {
        ಆದರೆ (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_ವೃಕ್ಷ(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    ಶೂನ್ಯ createScopedSigAttr(VAttrType vattrT) {
        ಗಣ_Scoped_Sig_Attr(new Ast_Attr_ದ{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_ಗಂಟು* cloneScopedSigAttr() ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_scopedSigAttr ? m_scopedSigAttr->clone_ವೃಕ್ಷ(true) : nullptr;
    }

    ಶೂನ್ಯ createGenericIface(Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ nodep, Ast_ಗಂಟು_Range* ಸ್ಥಿರಾಂಕ rangep,
                            Ast_ಗಂಟು* sigAttrListp, File_ಸಾಲು* ಸ್ಥಿರಾಂಕ modportFileline = nullptr,
                            ಸ್ಥಿರಾಂಕ string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        ಗಣ_DType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ಸ್ಥಿರಾಂಕ std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_ವೃಕ್ಷ(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ಸ್ಥಿರಾಂಕ refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        ಗಣ_DType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::ಒಳಹರಿವು;
        Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ಸ್ಥಿರ AstBegin* wrap_ಒಳ_Begin(Ast_ಗಂಟು_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        ಗಾಗಿ (Ast_ಗಂಟು_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ಗಂಟು_Stmt);
            ಆದರೆ (nextp) nextp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            ಆದರೆ (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_ಗಂಟು::ಕೂಡಿಸು_ಮುಂದಿನ(resp, beginp);
        }
        ಹಿಂತಿರುಗಿ resp;
    }
};
