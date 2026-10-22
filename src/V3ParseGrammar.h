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

#شامل "V3Ast.h"
#شامل "V3Const.h"
#شامل "V3Control.h"
#شامل "V3Global.h"
#شامل "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#شامل <stack>
#شامل <vector>

class V3ParseGrammar final {
عوامی:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_گرہ* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_گرہ_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_گرہ_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_سطر* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
    AstPin* m_instParamp = nullptr;  // Parameters for instantiations
    string m_instModule;  // Name of module referenced for instantiations
    VVarType m_varDecl = VVarType::UNKNOWN;  // Type for next signal declaration (reg/wire/etc)
    VDirection m_varIO = VDirection::NONE;  // Direction for next signal declaration (reg/wire/etc)
    VLifetime m_varLifetime;  // Static/Automatic for next signal
    V3Control::Var_Spec_قسم m_vlt_Var_Spec_قسم = V3Control::Var_Spec_قسم::VAR;
    bool m_impliedDecl = false;  // Allow implied wire declarations
    bool m_varDeclTyped = false;  // Var got reg/wire for dedup check
    bool m_pinAnsi = false;  // In ANSI parameter or port list
    bool m_tracingParse = true;  // Tracing disable for parser
    bool m_اندر_Implements = false;  // Is inside class implements list
    bool m_insideProperty = false;  // Is inside property declaration
    bool m_specifyignWarned = false;  // Issued a SPECIFYIGN warning
    bool m_typed_Property_پورٹ = false;  // Typed property port occurred on port lists
    bool m_modport_Imp_Exp_فعال
        = false;  // Standalone ID is a tf_identifier instead of port_identifier
    bool m_modport_Imp_Exp_آخری_Is_Export
        = false;  // Last import_export statement in modportPortsDecl is an export
    Ast_گرہ* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    صحیح m_pin_عدد = -1;  // Pin number currently parsing
    std::ڈھیر<صحیح> m_pin_ڈھیر;  // Queue of pin numbers being parsed

    ساکن صحیح s_type_Imp_عدد;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = طےشدہ;
    ساکن V3ParseGrammar* singletonp() {
        ساکن V3ParseGrammar singleton;
        واپس &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_گرہ_اظہاریہ* nodep) VL_MT_DISABLED;
    bool allTracingOn(ثابت File_سطر* fl) ثابت {
        واپس v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_گرہ_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_گرہ_Range* rangep) VL_MT_DISABLED;
    Ast_گرہ_Pre_Sel* scrubSel(Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_گرہ_DType* createArray(Ast_گرہ_DType* basep, Ast_گرہ_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_سطر* fileline, ثابت string& name, Ast_گرہ_Range* arrayp,
                           Ast_گرہ* attrsp) VL_MT_DISABLED;
    Ast_تفویض_W* create_Supply_اظہاریہ(File_سطر* fileline, ثابت string& name, صحیح قدر) VL_MT_DISABLED;
    std::string متن_Quoted(File_سطر* fileline, ثابت std::string& متن) {
        واپس singletonp()->unquoteString(fileline, متن);
    }
    Ast_گرہ* createCell(File_سطر* fileline, ثابت string& name, AstPin* pinlistp,
                        Ast_گرہ_Range* rangelistp) {
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
        واپس nodep;
    }
    ساکن Ast_گرہ* create_نمونہ_کلید(Ast_گرہ_اظہاریہ* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        اگر (ثابت AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            اگر (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_گرہ* ثابت textp = new Ast_متن{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                واپس textp;
            }
        }
        واپس exprp;
    }
    AstDisplay* createDisplayError(File_سطر* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_گرہ::جوڑو_اگلا<Ast_گرہ, Ast_گرہ>(nodep, new AstStop{fileline, false});
        واپس nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ساکن AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_گرہ_اظہاریہ* sizep) {
        اگر (!sizep) واپس binp;
        binp->isArray(true);
        اگر (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_درخت(), sizep);
        } ورنہ {
            binp->arraySizep(sizep);
        }
        واپس binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ساکن bool coverBinArrayUnsized(Ast_گرہ_اظہاریہ* sizep) {
        اگر (!sizep) واپس false;
        اگر (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_درخت(), sizep);
        واپس true;
    }
    Ast_گرہ_اظہاریہ* createGatePin(Ast_گرہ_اظہاریہ* exprp) {
        AstRange* ثابت rangep = m_gateRangep;
        اگر (!rangep) واپس exprp;
        واپس new AstGatePin{rangep->fileline(), exprp, rangep->clone_درخت(true)};
    }
    Ast_Sen_درخت* create_Sen_درخت_بدلا(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp) {
        واپس new Ast_Sen_درخت{fl, new Ast_Sen_شے{fl, VEdgeType::ET_بدلا, exprp}};
    }
    Ast_Sen_درخت* create_Sen_درخت_Dot_ستارہ(File_سطر* fl) {
        واپس new Ast_Sen_درخت{fl, new Ast_Sen_شے{fl, VEdgeType::ET_COMBO_ستارہ, nullptr}};
    }
    Ast_گرہ_اظہاریہ* createGlobalClockParseRef(File_سطر* fl) {
        واپس new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_درخت* create_Global_Clock_Sen_درخت(File_سطر* fl) {
        واپس create_Sen_درخت_بدلا(fl, createGlobalClockParseRef(fl));
    }
    Ast_گرہ* createNettype(File_سطر* fl, ثابت string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, nullptr, VFlag_بچہ_DType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->لیبل_Nodep(nodep);
        واپس nodep;
    }
    Ast_گرہ* createTypedef(File_سطر* fl, ثابت string& name, Ast_گرہ* attrsp, Ast_گرہ_DType* basep,
                           Ast_گرہ_Range* rangep) {
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, attrsp, VFlag_بچہ_DType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->لیبل_Nodep(nodep);
        واپس nodep;
    }
    Ast_گرہ* createTypedefFwd(File_سطر* fl, ثابت string& name, ثابت VFwdType& fwdType) {
        AstTypedefFwd* ثابت nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->لیبل_Nodep(nodep);
        واپس nodep;
    }
    ساکن خالی انتہا_Label(File_سطر* fl, ثابت Ast_گرہ* nodep, ثابت string* endnamep) {
        انتہا_Label(fl, nodep->prettyName(), endnamep);
    }
    ساکن خالی انتہا_Label(File_سطر* fl, ثابت string& name, ثابت string* endnamep) {
        اگر (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_گرہ::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    خالی سیٹ_DType(Ast_گرہ_DType* dtypep) {
        اگر (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_درخت(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    خالی سیٹ_Net_Delay(AstDelay* netDelayp) {
        اگر (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_درخت(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    خالی سیٹ_Net_Strength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    خالی pin_ڈالو() {
        m_pin_ڈھیر.ڈالو(m_pin_عدد);
        m_pin_عدد = 1;
    }
    خالی pin_نکالو(File_سطر* fl) {
        اگر (VL_UNCOVERABLE(m_pin_ڈھیر.خالی_خانے())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_عدد = m_pin_ڈھیر.چوٹی();
        m_pin_ڈھیر.نکالو();
    }
    Ast_گرہ_DType* جوڑو_Range(AstBasicDType* dtypep, Ast_گرہ_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        اگر (!rangesp) واپس dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_گرہ_Range* rangearraysp = nullptr;
        اگر (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } ورنہ {
            Ast_گرہ_Range* finalp = rangesp;
            جب (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            اگر (finalp != rangesp) {
                finalp->unlink_Fr_عقب();
                rangearraysp = rangesp;
            }
            اگر (AstRange* ثابت finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                اگر (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ثابت newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->چوڑائی(), dtypep->چوڑائی_Min()};
                    VL_DO_DANGLING(dtypep->delete_درخت(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        واپس createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_سطر* fileline, ثابت std::string& متن) VL_MT_DISABLED;
    خالی checkDpiVer(File_سطر* fileline, ثابت string& str) {
        اگر (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    Ast_Clocking_شے* make_Clocking_شے_List(File_سطر* flp, ثابت VDirection direction,
                                          Ast_گرہ_اظہاریہ* skewp, Ast_گرہ* ثابت clockingDeclps) {
        Ast_Clocking_شے* itemsp = nullptr;
        برائے (Ast_گرہ *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            اگر (nextp) nextp->unlink_Fr_عقب_With_اگلا();
            اگر (itemsp && skewp) skewp = skewp->clone_درخت(false);
            Ast_Clocking_شے* itemp = new Ast_Clocking_شے{flp, direction, skewp, nodep};
            itemsp = Ast_گرہ::جوڑو_اگلا_Null(itemsp, itemp);
        }
        واپس itemsp;
    }

    خالی سیٹ_Scoped_Sig_Attr(Ast_گرہ* attrsp) {
        اگر (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_درخت(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    خالی createScopedSigAttr(VAttrType vattrT) {
        سیٹ_Scoped_Sig_Attr(new Ast_Attr_کا{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_گرہ* cloneScopedSigAttr() ثابت {
        واپس m_scopedSigAttr ? m_scopedSigAttr->clone_درخت(true) : nullptr;
    }

    خالی createGenericIface(Ast_گرہ* ثابت nodep, Ast_گرہ_Range* ثابت rangep,
                            Ast_گرہ* sigAttrListp, File_سطر* ثابت modportFileline = nullptr,
                            ثابت string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        سیٹ_DType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ثابت std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_گرہ::جوڑو_اگلا(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_درخت(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ثابت refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        سیٹ_DType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::داخلہ;
        Ast_گرہ::جوڑو_اگلا(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ساکن AstBegin* wrap_اندر_Begin(Ast_گرہ_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        برائے (Ast_گرہ_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), گرہ_Stmt);
            اگر (nextp) nextp->unlink_Fr_عقب_With_اگلا();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            اگر (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_گرہ::جوڑو_اگلا(resp, beginp);
        }
        واپس resp;
    }
};
