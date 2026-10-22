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
    Ast_نود* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_نود_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_نود_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_باريس* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_نود* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    صحيح m_pin_اڠک = -1;  // Pin number currently parsing
    std::تيندنن<صحيح> m_pin_تيندنن;  // Queue of pin numbers being parsed

    ساكن صحيح s_type_Imp_اڠک;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = افتراضي;
    ساكن V3ParseGrammar* singletonp() {
        ساكن V3ParseGrammar singleton;
        أرجع &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_نود_اوڠکاڤن* nodep) VL_MT_DISABLED;
    bool allTracingOn(ثابت File_باريس* fl) ثابت {
        أرجع v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_نود_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_نود_Range* rangep) VL_MT_DISABLED;
    Ast_نود_Pre_Sel* scrubSel(Ast_نود_اوڠکاڤن* fromp, Ast_نود_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_نود_DType* createArray(Ast_نود_DType* basep, Ast_نود_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_باريس* fileline, ثابت string& name, Ast_نود_Range* arrayp,
                           Ast_نود* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_اوڠکاڤن(File_باريس* fileline, ثابت string& name, صحيح نيلاي) VL_MT_DISABLED;
    std::string textQuoted(File_باريس* fileline, ثابت std::string& text) {
        أرجع singletonp()->unquoteString(fileline, text);
    }
    Ast_نود* createCell(File_باريس* fileline, ثابت string& name, AstPin* pinlistp,
                        Ast_نود_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ثابت nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_ڤوکوق(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        أرجع nodep;
    }
    ساكن Ast_نود* create_Pattern_کونچي(Ast_نود_اوڠکاڤن* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        إذا (ثابت AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            إذا (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_نود* ثابت textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_ڤوکوق(), exprp);
                أرجع textp;
            }
        }
        أرجع exprp;
    }
    AstDisplay* createDisplayError(File_باريس* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_نود::addNext<Ast_نود, Ast_نود>(nodep, new AstStop{fileline, false});
        أرجع nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ساكن AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_نود_اوڠکاڤن* sizep) {
        إذا (!sizep) أرجع binp;
        binp->isArray(true);
        إذا (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_ڤوکوق(), sizep);
        } وإلا {
            binp->arraySizep(sizep);
        }
        أرجع binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ساكن bool coverBinArrayUnsized(Ast_نود_اوڠکاڤن* sizep) {
        إذا (!sizep) أرجع false;
        إذا (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_ڤوکوق(), sizep);
        أرجع true;
    }
    Ast_نود_اوڠکاڤن* createGatePin(Ast_نود_اوڠکاڤن* exprp) {
        AstRange* ثابت rangep = m_gateRangep;
        إذا (!rangep) أرجع exprp;
        أرجع new AstGatePin{rangep->fileline(), exprp, rangep->clone_ڤوکوق(true)};
    }
    Ast_Sen_ڤوکوق* create_Sen_ڤوکوق_Changed(File_باريس* fl, Ast_نود_اوڠکاڤن* exprp) {
        أرجع new Ast_Sen_ڤوکوق{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_ڤوکوق* create_Sen_ڤوکوق_Dot_Star(File_باريس* fl) {
        أرجع new Ast_Sen_ڤوکوق{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_نود_اوڠکاڤن* createGlobalClockParseRef(File_باريس* fl) {
        أرجع new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_ڤوکوق* create_Global_Clock_Sen_ڤوکوق(File_باريس* fl) {
        أرجع create_Sen_ڤوکوق_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_نود* createNettype(File_باريس* fl, ثابت string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        أرجع nodep;
    }
    Ast_نود* createTypedef(File_باريس* fl, ثابت string& name, Ast_نود* attrsp, Ast_نود_DType* basep,
                           Ast_نود_Range* rangep) {
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        أرجع nodep;
    }
    Ast_نود* createTypedefFwd(File_باريس* fl, ثابت string& name, ثابت VFwdType& fwdType) {
        AstTypedefFwd* ثابت nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        أرجع nodep;
    }
    ساكن فراغ endLabel(File_باريس* fl, ثابت Ast_نود* nodep, ثابت string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    ساكن فراغ endLabel(File_باريس* fl, ثابت string& name, ثابت string* endnamep) {
        إذا (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_نود::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    فراغ setDType(Ast_نود_DType* dtypep) {
        إذا (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_ڤوکوق(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    فراغ setNetDelay(AstDelay* netDelayp) {
        إذا (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_ڤوکوق(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    فراغ setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    فراغ pin_تولق() {
        m_pin_تيندنن.تولق(m_pin_اڠک);
        m_pin_اڠک = 1;
    }
    فراغ pin_کلوار(File_باريس* fl) {
        إذا (VL_UNCOVERABLE(m_pin_تيندنن.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_اڠک = m_pin_تيندنن.اتس();
        m_pin_تيندنن.کلوار();
    }
    Ast_نود_DType* addRange(AstBasicDType* dtypep, Ast_نود_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        إذا (!rangesp) أرجع dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_نود_Range* rangearraysp = nullptr;
        إذا (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } وإلا {
            Ast_نود_Range* finalp = rangesp;
            طالما (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            إذا (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            إذا (AstRange* ثابت finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                إذا (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ثابت newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_ڤوکوق(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        أرجع createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_باريس* fileline, ثابت std::string& text) VL_MT_DISABLED;
    فراغ checkDpiVer(File_باريس* fileline, ثابت string& str) {
        إذا (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_باريس* flp, ثابت VDirection direction,
                                          Ast_نود_اوڠکاڤن* skewp, Ast_نود* ثابت clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        لكل (Ast_نود *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            إذا (nextp) nextp->unlinkFrBackWithNext();
            إذا (itemsp && skewp) skewp = skewp->clone_ڤوکوق(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_نود::addNextNull(itemsp, itemp);
        }
        أرجع itemsp;
    }

    فراغ setScopedSigAttr(Ast_نود* attrsp) {
        إذا (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_ڤوکوق(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    فراغ createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_نود* cloneScopedSigAttr() ثابت {
        أرجع m_scopedSigAttr ? m_scopedSigAttr->clone_ڤوکوق(true) : nullptr;
    }

    فراغ createGenericIface(Ast_نود* ثابت nodep, Ast_نود_Range* ثابت rangep,
                            Ast_نود* sigAttrListp, File_باريس* ثابت modportFileline = nullptr,
                            ثابت string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ثابت std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_نود::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_ڤوکوق(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ثابت refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::ماسوقن;
        Ast_نود::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ساكن AstBegin* wrapInBegin(Ast_نود_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        لكل (Ast_نود_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), نود_Stmt);
            إذا (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            إذا (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_نود::addNext(resp, beginp);
        }
        أرجع resp;
    }
};
