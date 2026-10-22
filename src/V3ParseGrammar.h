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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#ਅਨ੍ਤਰ੍ਭਾਵ <stack>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class V3ParseGrammar final {
ਜਨਤਕ:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_ਗੰਢ* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_ਗੰਢ_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_ਗੰਢ_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_ਸਤਰ* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
    AstPin* m_instParamp = nullptr;  // Parameters for instantiations
    string m_instModule;  // Name of module referenced for instantiations
    VVarType m_varDecl = VVarType::UNKNOWN;  // Type for next signal declaration (reg/wire/etc)
    VDirection m_varIO = VDirection::NONE;  // Direction for next signal declaration (reg/wire/etc)
    VLifetime m_varLifetime;  // Static/Automatic for next signal
    V3Control::Var_Spec_ਕਿਸਮ m_vlt_Var_Spec_ਕਿਸਮ = V3Control::Var_Spec_ਕਿਸਮ::VAR;
    bool m_impliedDecl = false;  // Allow implied wire declarations
    bool m_varDeclTyped = false;  // Var got reg/wire for dedup check
    bool m_pinAnsi = false;  // In ANSI parameter or port list
    bool m_tracingParse = true;  // Tracing disable for parser
    bool m_ਵਿੱਚ_Implements = false;  // Is inside class implements list
    bool m_insideProperty = false;  // Is inside property declaration
    bool m_specifyignWarned = false;  // Issued a SPECIFYIGN warning
    bool m_typed_Property_ਪੋਰਟ = false;  // Typed property port occurred on port lists
    bool m_modport_Imp_Exp_ਸਰਗਰਮ
        = false;  // Standalone ID is a tf_identifier instead of port_identifier
    bool m_modport_Imp_Exp_ਆਖਰੀ_Is_Export
        = false;  // Last import_export statement in modportPortsDecl is an export
    Ast_ਗੰਢ* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    ਪੂਰਨ m_pin_ਅੰਕ = -1;  // Pin number currently parsing
    std::ਢੇਰ<ਪੂਰਨ> m_pin_ਢੇਰ;  // Queue of pin numbers being parsed

    ਸਥਿਰ ਪੂਰਨ s_type_Imp_ਅੰਕ;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = ਮੂਲ;
    ਸਥਿਰ V3ParseGrammar* singletonp() {
        ਸਥਿਰ V3ParseGrammar singleton;
        ਵਾਪਸ &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) VL_MT_DISABLED;
    bool allTracingOn(ਅਚਲ File_ਸਤਰ* fl) ਅਚਲ {
        ਵਾਪਸ v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_ਗੰਢ_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_ਗੰਢ_Range* rangep) VL_MT_DISABLED;
    Ast_ਗੰਢ_Pre_Sel* scrubSel(Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_ਗੰਢ_DType* createArray(Ast_ਗੰਢ_DType* basep, Ast_ਗੰਢ_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_ਸਤਰ* fileline, ਅਚਲ string& name, Ast_ਗੰਢ_Range* arrayp,
                           Ast_ਗੰਢ* attrsp) VL_MT_DISABLED;
    Ast_ਸੌਂਪੋ_W* create_Supply_ਵਿਅੰਜਕ(File_ਸਤਰ* fileline, ਅਚਲ string& name, ਪੂਰਨ ਮੁੱਲ) VL_MT_DISABLED;
    std::string ਪਾਠ_Quoted(File_ਸਤਰ* fileline, ਅਚਲ std::string& ਪਾਠ) {
        ਵਾਪਸ singletonp()->unquoteString(fileline, ਪਾਠ);
    }
    Ast_ਗੰਢ* createCell(File_ਸਤਰ* fileline, ਅਚਲ string& name, AstPin* pinlistp,
                        Ast_ਗੰਢ_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ਅਚਲ nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_ਰੁੱਖ(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        ਵਾਪਸ nodep;
    }
    ਸਥਿਰ Ast_ਗੰਢ* create_ਨਮੂਨਾ_ਕੁੰਜੀ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        ਜੇ (ਅਚਲ AstParseRef* ਅਚਲ refp = VN_CAST(exprp, ParseRef)) {
            ਜੇ (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_ਗੰਢ* ਅਚਲ textp = new Ast_ਪਾਠ{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_ਰੁੱਖ(), exprp);
                ਵਾਪਸ textp;
            }
        }
        ਵਾਪਸ exprp;
    }
    AstDisplay* createDisplayError(File_ਸਤਰ* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ<Ast_ਗੰਢ, Ast_ਗੰਢ>(nodep, new AstStop{fileline, false});
        ਵਾਪਸ nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ਸਥਿਰ AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* sizep) {
        ਜੇ (!sizep) ਵਾਪਸ binp;
        binp->isArray(true);
        ਜੇ (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_ਰੁੱਖ(), sizep);
        } ਨਹੀਂਤਾਂ {
            binp->arraySizep(sizep);
        }
        ਵਾਪਸ binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ਸਥਿਰ bool coverBinArrayUnsized(Ast_ਗੰਢ_ਵਿਅੰਜਕ* sizep) {
        ਜੇ (!sizep) ਵਾਪਸ false;
        ਜੇ (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_ਰੁੱਖ(), sizep);
        ਵਾਪਸ true;
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* createGatePin(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        AstRange* ਅਚਲ rangep = m_gateRangep;
        ਜੇ (!rangep) ਵਾਪਸ exprp;
        ਵਾਪਸ new AstGatePin{rangep->fileline(), exprp, rangep->clone_ਰੁੱਖ(true)};
    }
    Ast_Sen_ਰੁੱਖ* create_Sen_ਰੁੱਖ_ਬਦਲਿਆ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp) {
        ਵਾਪਸ new Ast_Sen_ਰੁੱਖ{fl, new Ast_Sen_ਵਸਤੂ{fl, VEdgeType::ET_ਬਦਲਿਆ, exprp}};
    }
    Ast_Sen_ਰੁੱਖ* create_Sen_ਰੁੱਖ_Dot_ਤਾਰਾ(File_ਸਤਰ* fl) {
        ਵਾਪਸ new Ast_Sen_ਰੁੱਖ{fl, new Ast_Sen_ਵਸਤੂ{fl, VEdgeType::ET_COMBO_ਤਾਰਾ, nullptr}};
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* createGlobalClockParseRef(File_ਸਤਰ* fl) {
        ਵਾਪਸ new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_ਰੁੱਖ* create_Global_Clock_Sen_ਰੁੱਖ(File_ਸਤਰ* fl) {
        ਵਾਪਸ create_Sen_ਰੁੱਖ_ਬਦਲਿਆ(fl, createGlobalClockParseRef(fl));
    }
    Ast_ਗੰਢ* createNettype(File_ਸਤਰ* fl, ਅਚਲ string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ਅਚਲ nodep = new AstTypedef{fl, name, nullptr, VFlag_ਬੱਚਾ_DType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->ਟੈਗ_Nodep(nodep);
        ਵਾਪਸ nodep;
    }
    Ast_ਗੰਢ* createTypedef(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* attrsp, Ast_ਗੰਢ_DType* basep,
                           Ast_ਗੰਢ_Range* rangep) {
        AstTypedef* ਅਚਲ nodep = new AstTypedef{fl, name, attrsp, VFlag_ਬੱਚਾ_DType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->ਟੈਗ_Nodep(nodep);
        ਵਾਪਸ nodep;
    }
    Ast_ਗੰਢ* createTypedefFwd(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ VFwdType& fwdType) {
        AstTypedefFwd* ਅਚਲ nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->ਟੈਗ_Nodep(nodep);
        ਵਾਪਸ nodep;
    }
    ਸਥਿਰ ਖਾਲੀ ਸਿਰਾ_Label(File_ਸਤਰ* fl, ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string* endnamep) {
        ਸਿਰਾ_Label(fl, nodep->prettyName(), endnamep);
    }
    ਸਥਿਰ ਖਾਲੀ ਸਿਰਾ_Label(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string* endnamep) {
        ਜੇ (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_ਗੰਢ::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    ਖਾਲੀ ਸੈੱਟ_DType(Ast_ਗੰਢ_DType* dtypep) {
        ਜੇ (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_ਰੁੱਖ(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    ਖਾਲੀ ਸੈੱਟ_Net_Delay(AstDelay* netDelayp) {
        ਜੇ (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_ਰੁੱਖ(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    ਖਾਲੀ ਸੈੱਟ_Net_Strength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    ਖਾਲੀ pin_ਧੱਕੋ() {
        m_pin_ਢੇਰ.ਧੱਕੋ(m_pin_ਅੰਕ);
        m_pin_ਅੰਕ = 1;
    }
    ਖਾਲੀ pin_ਕੱਢੋ(File_ਸਤਰ* fl) {
        ਜੇ (VL_UNCOVERABLE(m_pin_ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_ਅੰਕ = m_pin_ਢੇਰ.ਸਿਖਰ();
        m_pin_ਢੇਰ.ਕੱਢੋ();
    }
    Ast_ਗੰਢ_DType* ਜੋੜੋ_Range(AstBasicDType* dtypep, Ast_ਗੰਢ_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        ਜੇ (!rangesp) ਵਾਪਸ dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_ਗੰਢ_Range* rangearraysp = nullptr;
        ਜੇ (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } ਨਹੀਂਤਾਂ {
            Ast_ਗੰਢ_Range* finalp = rangesp;
            ਜਦੋਂ (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            ਜੇ (finalp != rangesp) {
                finalp->unlink_Fr_ਪਿੱਠ();
                rangearraysp = rangesp;
            }
            ਜੇ (AstRange* ਅਚਲ finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                ਜੇ (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ਅਚਲ newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->ਚੌੜਾਈ(), dtypep->ਚੌੜਾਈ_Min()};
                    VL_DO_DANGLING(dtypep->delete_ਰੁੱਖ(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        ਵਾਪਸ createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_ਸਤਰ* fileline, ਅਚਲ std::string& ਪਾਠ) VL_MT_DISABLED;
    ਖਾਲੀ checkDpiVer(File_ਸਤਰ* fileline, ਅਚਲ string& str) {
        ਜੇ (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    Ast_Clocking_ਵਸਤੂ* make_Clocking_ਵਸਤੂ_List(File_ਸਤਰ* flp, ਅਚਲ VDirection direction,
                                          Ast_ਗੰਢ_ਵਿਅੰਜਕ* skewp, Ast_ਗੰਢ* ਅਚਲ clockingDeclps) {
        Ast_Clocking_ਵਸਤੂ* itemsp = nullptr;
        ਲਈ (Ast_ਗੰਢ *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ਜੇ (nextp) nextp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            ਜੇ (itemsp && skewp) skewp = skewp->clone_ਰੁੱਖ(false);
            Ast_Clocking_ਵਸਤੂ* itemp = new Ast_Clocking_ਵਸਤੂ{flp, direction, skewp, nodep};
            itemsp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ_Null(itemsp, itemp);
        }
        ਵਾਪਸ itemsp;
    }

    ਖਾਲੀ ਸੈੱਟ_Scoped_Sig_Attr(Ast_ਗੰਢ* attrsp) {
        ਜੇ (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_ਰੁੱਖ(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    ਖਾਲੀ createScopedSigAttr(VAttrType vattrT) {
        ਸੈੱਟ_Scoped_Sig_Attr(new Ast_Attr_ਦਾ{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_ਗੰਢ* cloneScopedSigAttr() ਅਚਲ {
        ਵਾਪਸ m_scopedSigAttr ? m_scopedSigAttr->clone_ਰੁੱਖ(true) : nullptr;
    }

    ਖਾਲੀ createGenericIface(Ast_ਗੰਢ* ਅਚਲ nodep, Ast_ਗੰਢ_Range* ਅਚਲ rangep,
                            Ast_ਗੰਢ* sigAttrListp, File_ਸਤਰ* ਅਚਲ modportFileline = nullptr,
                            ਅਚਲ string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        ਸੈੱਟ_DType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ਅਚਲ std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_ਰੁੱਖ(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ਅਚਲ refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        ਸੈੱਟ_DType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::ਨਿਵੇਸ਼;
        Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ਸਥਿਰ AstBegin* wrap_ਵਿੱਚ_Begin(Ast_ਗੰਢ_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        ਲਈ (Ast_ਗੰਢ_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ਗੰਢ_Stmt);
            ਜੇ (nextp) nextp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            ਜੇ (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_ਗੰਢ::ਜੋੜੋ_ਅਗਲਾ(resp, beginp);
        }
        ਵਾਪਸ resp;
    }
};
