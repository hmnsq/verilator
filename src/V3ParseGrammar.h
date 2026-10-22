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
عمومی:
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
    Ast_گره* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    صحیح m_pin_رقم = -1;  // Pin number currently parsing
    std::پشته<صحیح> m_pin_پشته;  // Queue of pin numbers being parsed

    ایستا صحیح s_type_Imp_رقم;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = پیشفرض;
    ایستا V3ParseGrammar* singletonp() {
        ایستا V3ParseGrammar singleton;
        بازگشت &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_گره_عبارت* nodep) VL_MT_DISABLED;
    bool allTracingOn(ثابت File_سطر* fl) ثابت {
        بازگشت v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_گره_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_گره_Range* rangep) VL_MT_DISABLED;
    Ast_گره_Pre_Sel* scrubSel(Ast_گره_عبارت* fromp, Ast_گره_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_گره_DType* createArray(Ast_گره_DType* basep, Ast_گره_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_سطر* fileline, ثابت string& name, Ast_گره_Range* arrayp,
                           Ast_گره* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_عبارت(File_سطر* fileline, ثابت string& name, صحیح مقدار) VL_MT_DISABLED;
    std::string textQuoted(File_سطر* fileline, ثابت std::string& text) {
        بازگشت singletonp()->unquoteString(fileline, text);
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
        بازگشت nodep;
    }
    ایستا Ast_گره* create_Pattern_کلید(Ast_گره_عبارت* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        اگر (ثابت AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            اگر (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_گره* ثابت textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_درخت(), exprp);
                بازگشت textp;
            }
        }
        بازگشت exprp;
    }
    AstDisplay* createDisplayError(File_سطر* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_گره::addNext<Ast_گره, Ast_گره>(nodep, new AstStop{fileline, false});
        بازگشت nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ایستا AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_گره_عبارت* sizep) {
        اگر (!sizep) بازگشت binp;
        binp->isArray(true);
        اگر (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_درخت(), sizep);
        } وگرنه {
            binp->arraySizep(sizep);
        }
        بازگشت binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ایستا bool coverBinArrayUnsized(Ast_گره_عبارت* sizep) {
        اگر (!sizep) بازگشت false;
        اگر (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_درخت(), sizep);
        بازگشت true;
    }
    Ast_گره_عبارت* createGatePin(Ast_گره_عبارت* exprp) {
        AstRange* ثابت rangep = m_gateRangep;
        اگر (!rangep) بازگشت exprp;
        بازگشت new AstGatePin{rangep->fileline(), exprp, rangep->clone_درخت(true)};
    }
    Ast_Sen_درخت* create_Sen_درخت_Changed(File_سطر* fl, Ast_گره_عبارت* exprp) {
        بازگشت new Ast_Sen_درخت{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_درخت* create_Sen_درخت_Dot_Star(File_سطر* fl) {
        بازگشت new Ast_Sen_درخت{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_گره_عبارت* createGlobalClockParseRef(File_سطر* fl) {
        بازگشت new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_درخت* create_Global_Clock_Sen_درخت(File_سطر* fl) {
        بازگشت create_Sen_درخت_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_گره* createNettype(File_سطر* fl, ثابت string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        بازگشت nodep;
    }
    Ast_گره* createTypedef(File_سطر* fl, ثابت string& name, Ast_گره* attrsp, Ast_گره_DType* basep,
                           Ast_گره_Range* rangep) {
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        بازگشت nodep;
    }
    Ast_گره* createTypedefFwd(File_سطر* fl, ثابت string& name, ثابت VFwdType& fwdType) {
        AstTypedefFwd* ثابت nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        بازگشت nodep;
    }
    ایستا تهی endLabel(File_سطر* fl, ثابت Ast_گره* nodep, ثابت string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    ایستا تهی endLabel(File_سطر* fl, ثابت string& name, ثابت string* endnamep) {
        اگر (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_گره::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    تهی setDType(Ast_گره_DType* dtypep) {
        اگر (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_درخت(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    تهی setNetDelay(AstDelay* netDelayp) {
        اگر (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_درخت(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    تهی setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    تهی pin_افزودن() {
        m_pin_پشته.افزودن(m_pin_رقم);
        m_pin_رقم = 1;
    }
    تهی pin_برداشت(File_سطر* fl) {
        اگر (VL_UNCOVERABLE(m_pin_پشته.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_رقم = m_pin_پشته.بالاترین();
        m_pin_پشته.برداشت();
    }
    Ast_گره_DType* addRange(AstBasicDType* dtypep, Ast_گره_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        اگر (!rangesp) بازگشت dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_گره_Range* rangearraysp = nullptr;
        اگر (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } وگرنه {
            Ast_گره_Range* finalp = rangesp;
            وقتی (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            اگر (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            اگر (AstRange* ثابت finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                اگر (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ثابت newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_درخت(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        بازگشت createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_سطر* fileline, ثابت std::string& text) VL_MT_DISABLED;
    تهی checkDpiVer(File_سطر* fileline, ثابت string& str) {
        اگر (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_سطر* flp, ثابت VDirection direction,
                                          Ast_گره_عبارت* skewp, Ast_گره* ثابت clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        برای (Ast_گره *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            اگر (nextp) nextp->unlinkFrBackWithNext();
            اگر (itemsp && skewp) skewp = skewp->clone_درخت(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_گره::addNextNull(itemsp, itemp);
        }
        بازگشت itemsp;
    }

    تهی setScopedSigAttr(Ast_گره* attrsp) {
        اگر (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_درخت(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    تهی createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_گره* cloneScopedSigAttr() ثابت {
        بازگشت m_scopedSigAttr ? m_scopedSigAttr->clone_درخت(true) : nullptr;
    }

    تهی createGenericIface(Ast_گره* ثابت nodep, Ast_گره_Range* ثابت rangep,
                            Ast_گره* sigAttrListp, File_سطر* ثابت modportFileline = nullptr,
                            ثابت string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ثابت std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_گره::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_درخت(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ثابت refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::درون_داد;
        Ast_گره::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ایستا AstBegin* wrapInBegin(Ast_گره_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        برای (Ast_گره_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), گره_Stmt);
            اگر (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            اگر (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_گره::addNext(resp, beginp);
        }
        بازگشت resp;
    }
};
