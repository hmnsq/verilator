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
    Ast_گره* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_گره_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_گره_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_سطر* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
    AstPin* m_instParamp = nullptr;  // Parameters for instantiations
    string m_instModule;  // Name of module referenced for instantiations
    VVarType m_varDecl = VVarType::UNKNOWN;  // Type for next signal declaration (reg/wire/etc)
    VDirection m_varIO = VDirection::NONE;  // Direction for next signal declaration (reg/wire/etc)
    VLifetime m_varLifetime;  // Static/Automatic for next signal
    V3Control::Var_Spec_نوع m_vlt_Var_Spec_نوع = V3Control::Var_Spec_نوع::VAR;
    bool m_impliedDecl = false;  // Allow implied wire declarations
    bool m_varDeclTyped = false;  // Var got reg/wire for dedup check
    bool m_pinAnsi = false;  // In ANSI parameter or port list
    bool m_tracingParse = true;  // Tracing disable for parser
    bool m_درون_Implements = false;  // Is inside class implements list
    bool m_insideProperty = false;  // Is inside property declaration
    bool m_specifyignWarned = false;  // Issued a SPECIFYIGN warning
    bool m_typed_Property_پورت = false;  // Typed property port occurred on port lists
    bool m_modport_Imp_Exp_فعال
        = false;  // Standalone ID is a tf_identifier instead of port_identifier
    bool m_modport_Imp_Exp_آخرین_Is_Export
        = false;  // Last import_export statement in modportPortsDecl is an export
    Ast_گره* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    صحيح m_pin_رقم = -1;  // Pin number currently parsing
    std::پشته<صحيح> m_pin_پشته;  // Queue of pin numbers being parsed

    ساكن صحيح s_type_Imp_رقم;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = افتراضي;
    ساكن V3ParseGrammar* singletonp() {
        ساكن V3ParseGrammar singleton;
        أرجع &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_گره_عبارت* nodep) VL_MT_DISABLED;
    bool allTracingOn(ثابت File_سطر* fl) ثابت {
        أرجع v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_گره_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_گره_Range* rangep) VL_MT_DISABLED;
    Ast_گره_Pre_Sel* scrubSel(Ast_گره_عبارت* fromp, Ast_گره_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_گره_DType* createArray(Ast_گره_DType* basep, Ast_گره_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_سطر* fileline, ثابت string& name, Ast_گره_Range* arrayp,
                           Ast_گره* attrsp) VL_MT_DISABLED;
    Ast_تخصیص_بده_W* create_Supply_عبارت(File_سطر* fileline, ثابت string& name, صحيح مقدار) VL_MT_DISABLED;
    std::string متن_Quoted(File_سطر* fileline, ثابت std::string& متن) {
        أرجع singletonp()->unquoteString(fileline, متن);
    }
    Ast_گره* createCell(File_سطر* fileline, ثابت string& name, AstPin* pinlistp,
                        Ast_گره_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ثابت nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_درخت(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        أرجع nodep;
    }
    ساكن Ast_گره* create_الگو_کلید(Ast_گره_عبارت* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        إذا (ثابت AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            إذا (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_گره* ثابت textp = new Ast_متن{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                أرجع textp;
            }
        }
        أرجع exprp;
    }
    AstDisplay* createDisplayError(File_سطر* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_گره::جمع_کن_بعدی<Ast_گره, Ast_گره>(nodep, new AstStop{fileline, false});
        أرجع nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ساكن AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_گره_عبارت* sizep) {
        إذا (!sizep) أرجع binp;
        binp->isArray(true);
        إذا (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_درخت(), sizep);
        } وإلا {
            binp->arraySizep(sizep);
        }
        أرجع binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ساكن bool coverBinArrayUnsized(Ast_گره_عبارت* sizep) {
        إذا (!sizep) أرجع false;
        إذا (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_درخت(), sizep);
        أرجع true;
    }
    Ast_گره_عبارت* createGatePin(Ast_گره_عبارت* exprp) {
        AstRange* ثابت rangep = m_gateRangep;
        إذا (!rangep) أرجع exprp;
        أرجع new AstGatePin{rangep->fileline(), exprp, rangep->clone_درخت(true)};
    }
    Ast_Sen_درخت* create_Sen_درخت_تغییریافته(File_سطر* fl, Ast_گره_عبارت* exprp) {
        أرجع new Ast_Sen_درخت{fl, new Ast_Sen_قلم{fl, VEdgeType::ET_تغییریافته, exprp}};
    }
    Ast_Sen_درخت* create_Sen_درخت_Dot_ستاره(File_سطر* fl) {
        أرجع new Ast_Sen_درخت{fl, new Ast_Sen_قلم{fl, VEdgeType::ET_COMBO_ستاره, nullptr}};
    }
    Ast_گره_عبارت* createGlobalClockParseRef(File_سطر* fl) {
        أرجع new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_درخت* create_Global_Clock_Sen_درخت(File_سطر* fl) {
        أرجع create_Sen_درخت_تغییریافته(fl, createGlobalClockParseRef(fl));
    }
    Ast_گره* createNettype(File_سطر* fl, ثابت string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, nullptr, VFlag_فرزند_DType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->برچسب_Nodep(nodep);
        أرجع nodep;
    }
    Ast_گره* createTypedef(File_سطر* fl, ثابت string& name, Ast_گره* attrsp, Ast_گره_DType* basep,
                           Ast_گره_Range* rangep) {
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, attrsp, VFlag_فرزند_DType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->برچسب_Nodep(nodep);
        أرجع nodep;
    }
    Ast_گره* createTypedefFwd(File_سطر* fl, ثابت string& name, ثابت VFwdType& fwdType) {
        AstTypedefFwd* ثابت nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->برچسب_Nodep(nodep);
        أرجع nodep;
    }
    ساكن فراغ انتها_Label(File_سطر* fl, ثابت Ast_گره* nodep, ثابت string* endnamep) {
        انتها_Label(fl, nodep->prettyName(), endnamep);
    }
    ساكن فراغ انتها_Label(File_سطر* fl, ثابت string& name, ثابت string* endnamep) {
        إذا (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_گره::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    فراغ مجموعه_DType(Ast_گره_DType* dtypep) {
        إذا (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_درخت(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    فراغ مجموعه_Net_Delay(AstDelay* netDelayp) {
        إذا (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_درخت(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    فراغ مجموعه_Net_Strength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    فراغ pin_افزودن() {
        m_pin_پشته.افزودن(m_pin_رقم);
        m_pin_رقم = 1;
    }
    فراغ pin_برداشت(File_سطر* fl) {
        إذا (VL_UNCOVERABLE(m_pin_پشته.خالی())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_رقم = m_pin_پشته.بالاترین();
        m_pin_پشته.برداشت();
    }
    Ast_گره_DType* جمع_کن_Range(AstBasicDType* dtypep, Ast_گره_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        إذا (!rangesp) أرجع dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_گره_Range* rangearraysp = nullptr;
        إذا (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } وإلا {
            Ast_گره_Range* finalp = rangesp;
            طالما (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            إذا (finalp != rangesp) {
                finalp->unlink_Fr_پشت();
                rangearraysp = rangesp;
            }
            إذا (AstRange* ثابت finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                إذا (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ثابت newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->عرض(), dtypep->عرض_Min()};
                    VL_DO_DANGLING(dtypep->delete_درخت(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        أرجع createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_سطر* fileline, ثابت std::string& متن) VL_MT_DISABLED;
    فراغ checkDpiVer(File_سطر* fileline, ثابت string& str) {
        إذا (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    Ast_Clocking_قلم* make_Clocking_قلم_List(File_سطر* flp, ثابت VDirection direction,
                                          Ast_گره_عبارت* skewp, Ast_گره* ثابت clockingDeclps) {
        Ast_Clocking_قلم* itemsp = nullptr;
        لكل (Ast_گره *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            إذا (nextp) nextp->unlink_Fr_پشت_With_بعدی();
            إذا (itemsp && skewp) skewp = skewp->clone_درخت(false);
            Ast_Clocking_قلم* itemp = new Ast_Clocking_قلم{flp, direction, skewp, nodep};
            itemsp = Ast_گره::جمع_کن_بعدی_Null(itemsp, itemp);
        }
        أرجع itemsp;
    }

    فراغ مجموعه_Scoped_Sig_Attr(Ast_گره* attrsp) {
        إذا (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_درخت(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    فراغ createScopedSigAttr(VAttrType vattrT) {
        مجموعه_Scoped_Sig_Attr(new Ast_Attr_از{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_گره* cloneScopedSigAttr() ثابت {
        أرجع m_scopedSigAttr ? m_scopedSigAttr->clone_درخت(true) : nullptr;
    }

    فراغ createGenericIface(Ast_گره* ثابت nodep, Ast_گره_Range* ثابت rangep,
                            Ast_گره* sigAttrListp, File_سطر* ثابت modportFileline = nullptr,
                            ثابت string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        مجموعه_DType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ثابت std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_گره::جمع_کن_بعدی(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_درخت(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ثابت refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        مجموعه_DType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::درون_داد;
        Ast_گره::جمع_کن_بعدی(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ساكن AstBegin* wrap_درون_Begin(Ast_گره_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        لكل (Ast_گره_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), گره_Stmt);
            إذا (nextp) nextp->unlink_Fr_پشت_With_بعدی();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            إذا (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_گره::جمع_کن_بعدی(resp, beginp);
        }
        أرجع resp;
    }
};
