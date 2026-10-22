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

#ضمن "V3Ast.h"
#ضمن "V3Const.h"
#ضمن "V3Control.h"
#ضمن "V3Global.h"
#ضمن "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#ضمن <stack>
#ضمن <vector>

class V3ParseGrammar final {
عام:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_غوټه* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_غوټه_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_غوټه_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_کرښه* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
    AstPin* m_instParamp = nullptr;  // Parameters for instantiations
    string m_instModule;  // Name of module referenced for instantiations
    VVarType m_varDecl = VVarType::UNKNOWN;  // Type for next signal declaration (reg/wire/etc)
    VDirection m_varIO = VDirection::NONE;  // Direction for next signal declaration (reg/wire/etc)
    VLifetime m_varLifetime;  // Static/Automatic for next signal
    V3Control::Var_Spec_ډول m_vlt_Var_Spec_ډول = V3Control::Var_Spec_ډول::VAR;
    bool m_impliedDecl = false;  // Allow implied wire declarations
    bool m_varDeclTyped = false;  // Var got reg/wire for dedup check
    bool m_pinAnsi = false;  // In ANSI parameter or port list
    bool m_tracingParse = true;  // Tracing disable for parser
    bool m_دننه_Implements = false;  // Is inside class implements list
    bool m_insideProperty = false;  // Is inside property declaration
    bool m_specifyignWarned = false;  // Issued a SPECIFYIGN warning
    bool m_typed_Property_پورټ = false;  // Typed property port occurred on port lists
    bool m_modport_Imp_Exp_فعال
        = false;  // Standalone ID is a tf_identifier instead of port_identifier
    bool m_modport_Imp_Exp_وروستی_Is_Export
        = false;  // Last import_export statement in modportPortsDecl is an export
    Ast_غوټه* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    صحيح m_pin_عدد = -1;  // Pin number currently parsing
    std::ډېری<صحيح> m_pin_ډېری;  // Queue of pin numbers being parsed

    ساكن صحيح s_type_Imp_عدد;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = افتراضي;
    ساكن V3ParseGrammar* singletonp() {
        ساكن V3ParseGrammar singleton;
        أرجع &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_غوټه_بیان* nodep) VL_MT_DISABLED;
    bool allTracingOn(ثابت File_کرښه* fl) ثابت {
        أرجع v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_غوټه_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_غوټه_Range* rangep) VL_MT_DISABLED;
    Ast_غوټه_Pre_Sel* scrubSel(Ast_غوټه_بیان* fromp, Ast_غوټه_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_غوټه_DType* createArray(Ast_غوټه_DType* basep, Ast_غوټه_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_کرښه* fileline, ثابت string& name, Ast_غوټه_Range* arrayp,
                           Ast_غوټه* attrsp) VL_MT_DISABLED;
    Ast_ټاکل_W* create_Supply_بیان(File_کرښه* fileline, ثابت string& name, صحيح ارزښت) VL_MT_DISABLED;
    std::string متن_Quoted(File_کرښه* fileline, ثابت std::string& متن) {
        أرجع singletonp()->unquoteString(fileline, متن);
    }
    Ast_غوټه* createCell(File_کرښه* fileline, ثابت string& name, AstPin* pinlistp,
                        Ast_غوټه_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ثابت nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_ونه(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        أرجع nodep;
    }
    ساكن Ast_غوټه* create_نمونه_کلی(Ast_غوټه_بیان* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        إذا (ثابت AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            إذا (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_غوټه* ثابت textp = new Ast_متن{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_ونه(), exprp);
                أرجع textp;
            }
        }
        أرجع exprp;
    }
    AstDisplay* createDisplayError(File_کرښه* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_غوټه::جمع_کړه_بل<Ast_غوټه, Ast_غوټه>(nodep, new AstStop{fileline, false});
        أرجع nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ساكن AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_غوټه_بیان* sizep) {
        إذا (!sizep) أرجع binp;
        binp->isArray(true);
        إذا (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_ونه(), sizep);
        } وإلا {
            binp->arraySizep(sizep);
        }
        أرجع binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ساكن bool coverBinArrayUnsized(Ast_غوټه_بیان* sizep) {
        إذا (!sizep) أرجع false;
        إذا (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_ونه(), sizep);
        أرجع true;
    }
    Ast_غوټه_بیان* createGatePin(Ast_غوټه_بیان* exprp) {
        AstRange* ثابت rangep = m_gateRangep;
        إذا (!rangep) أرجع exprp;
        أرجع new AstGatePin{rangep->fileline(), exprp, rangep->clone_ونه(true)};
    }
    Ast_Sen_ونه* create_Sen_ونه_بدل_شوی(File_کرښه* fl, Ast_غوټه_بیان* exprp) {
        أرجع new Ast_Sen_ونه{fl, new Ast_Sen_توکی{fl, VEdgeType::ET_بدل_شوی, exprp}};
    }
    Ast_Sen_ونه* create_Sen_ونه_Dot_ستوری(File_کرښه* fl) {
        أرجع new Ast_Sen_ونه{fl, new Ast_Sen_توکی{fl, VEdgeType::ET_COMBO_ستوری, nullptr}};
    }
    Ast_غوټه_بیان* createGlobalClockParseRef(File_کرښه* fl) {
        أرجع new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_ونه* create_Global_Clock_Sen_ونه(File_کرښه* fl) {
        أرجع create_Sen_ونه_بدل_شوی(fl, createGlobalClockParseRef(fl));
    }
    Ast_غوټه* createNettype(File_کرښه* fl, ثابت string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, nullptr, VFlag_ماشوم_DType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->نښان_Nodep(nodep);
        أرجع nodep;
    }
    Ast_غوټه* createTypedef(File_کرښه* fl, ثابت string& name, Ast_غوټه* attrsp, Ast_غوټه_DType* basep,
                           Ast_غوټه_Range* rangep) {
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, attrsp, VFlag_ماشوم_DType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->نښان_Nodep(nodep);
        أرجع nodep;
    }
    Ast_غوټه* createTypedefFwd(File_کرښه* fl, ثابت string& name, ثابت VFwdType& fwdType) {
        AstTypedefFwd* ثابت nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->نښان_Nodep(nodep);
        أرجع nodep;
    }
    ساكن فراغ پای_ټکی_Label(File_کرښه* fl, ثابت Ast_غوټه* nodep, ثابت string* endnamep) {
        پای_ټکی_Label(fl, nodep->prettyName(), endnamep);
    }
    ساكن فراغ پای_ټکی_Label(File_کرښه* fl, ثابت string& name, ثابت string* endnamep) {
        إذا (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_غوټه::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    فراغ سېټ_DType(Ast_غوټه_DType* dtypep) {
        إذا (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_ونه(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    فراغ سېټ_Net_Delay(AstDelay* netDelayp) {
        إذا (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_ونه(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    فراغ سېټ_Net_Strength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    فراغ pin_ټېل() {
        m_pin_ډېری.ټېل(m_pin_عدد);
        m_pin_عدد = 1;
    }
    فراغ pin_راوباسه(File_کرښه* fl) {
        إذا (VL_UNCOVERABLE(m_pin_ډېری.تشه())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_عدد = m_pin_ډېری.سر();
        m_pin_ډېری.راوباسه();
    }
    Ast_غوټه_DType* جمع_کړه_Range(AstBasicDType* dtypep, Ast_غوټه_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        إذا (!rangesp) أرجع dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_غوټه_Range* rangearraysp = nullptr;
        إذا (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } وإلا {
            Ast_غوټه_Range* finalp = rangesp;
            طالما (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            إذا (finalp != rangesp) {
                finalp->unlink_Fr_شا();
                rangearraysp = rangesp;
            }
            إذا (AstRange* ثابت finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                إذا (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ثابت newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->پلنوالی(), dtypep->پلنوالی_Min()};
                    VL_DO_DANGLING(dtypep->delete_ونه(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        أرجع createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_کرښه* fileline, ثابت std::string& متن) VL_MT_DISABLED;
    فراغ checkDpiVer(File_کرښه* fileline, ثابت string& str) {
        إذا (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    Ast_Clocking_توکی* make_Clocking_توکی_List(File_کرښه* flp, ثابت VDirection direction,
                                          Ast_غوټه_بیان* skewp, Ast_غوټه* ثابت clockingDeclps) {
        Ast_Clocking_توکی* itemsp = nullptr;
        لكل (Ast_غوټه *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            إذا (nextp) nextp->unlink_Fr_شا_With_بل();
            إذا (itemsp && skewp) skewp = skewp->clone_ونه(false);
            Ast_Clocking_توکی* itemp = new Ast_Clocking_توکی{flp, direction, skewp, nodep};
            itemsp = Ast_غوټه::جمع_کړه_بل_Null(itemsp, itemp);
        }
        أرجع itemsp;
    }

    فراغ سېټ_Scoped_Sig_Attr(Ast_غوټه* attrsp) {
        إذا (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_ونه(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    فراغ createScopedSigAttr(VAttrType vattrT) {
        سېټ_Scoped_Sig_Attr(new Ast_Attr_د{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_غوټه* cloneScopedSigAttr() ثابت {
        أرجع m_scopedSigAttr ? m_scopedSigAttr->clone_ونه(true) : nullptr;
    }

    فراغ createGenericIface(Ast_غوټه* ثابت nodep, Ast_غوټه_Range* ثابت rangep,
                            Ast_غوټه* sigAttrListp, File_کرښه* ثابت modportFileline = nullptr,
                            ثابت string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        سېټ_DType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ثابت std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_غوټه::جمع_کړه_بل(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_ونه(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ثابت refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        سېټ_DType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::ننوت;
        Ast_غوټه::جمع_کړه_بل(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ساكن AstBegin* wrap_دننه_Begin(Ast_غوټه_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        لكل (Ast_غوټه_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), غوټه_Stmt);
            إذا (nextp) nextp->unlink_Fr_شا_With_بل();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            إذا (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_غوټه::جمع_کړه_بل(resp, beginp);
        }
        أرجع resp;
    }
};
