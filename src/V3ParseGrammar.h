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
    Ast_عقدة* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_عقدة_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_عقدة_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
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
    Ast_عقدة* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    صحيح m_pin_رقم = -1;  // Pin number currently parsing
    std::مكدس<صحيح> m_pin_مكدس;  // Queue of pin numbers being parsed

    ساكن صحيح s_type_Imp_رقم;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = افتراضي;
    ساكن V3ParseGrammar* singletonp() {
        ساكن V3ParseGrammar singleton;
        أرجع &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_عقدة_تعبير* nodep) VL_MT_DISABLED;
    bool allTracingOn(ثابت File_سطر* fl) ثابت {
        أرجع v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_عقدة_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_عقدة_Range* rangep) VL_MT_DISABLED;
    Ast_عقدة_Pre_Sel* scrubSel(Ast_عقدة_تعبير* fromp, Ast_عقدة_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_عقدة_DType* createArray(Ast_عقدة_DType* basep, Ast_عقدة_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_سطر* fileline, ثابت string& name, Ast_عقدة_Range* arrayp,
                           Ast_عقدة* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_تعبير(File_سطر* fileline, ثابت string& name, صحيح قيمة) VL_MT_DISABLED;
    std::string textQuoted(File_سطر* fileline, ثابت std::string& text) {
        أرجع singletonp()->unquoteString(fileline, text);
    }
    Ast_عقدة* createCell(File_سطر* fileline, ثابت string& name, AstPin* pinlistp,
                        Ast_عقدة_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ثابت nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_شجرة(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        أرجع nodep;
    }
    ساكن Ast_عقدة* create_Pattern_مفتاح(Ast_عقدة_تعبير* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        إذا (ثابت AstParseRef* ثابت refp = VN_CAST(exprp, ParseRef)) {
            إذا (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_عقدة* ثابت textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_شجرة(), exprp);
                أرجع textp;
            }
        }
        أرجع exprp;
    }
    AstDisplay* createDisplayError(File_سطر* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_عقدة::addNext<Ast_عقدة, Ast_عقدة>(nodep, new AstStop{fileline, false});
        أرجع nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ساكن AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_عقدة_تعبير* sizep) {
        إذا (!sizep) أرجع binp;
        binp->isArray(true);
        إذا (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_شجرة(), sizep);
        } وإلا {
            binp->arraySizep(sizep);
        }
        أرجع binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ساكن bool coverBinArrayUnsized(Ast_عقدة_تعبير* sizep) {
        إذا (!sizep) أرجع false;
        إذا (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_شجرة(), sizep);
        أرجع true;
    }
    Ast_عقدة_تعبير* createGatePin(Ast_عقدة_تعبير* exprp) {
        AstRange* ثابت rangep = m_gateRangep;
        إذا (!rangep) أرجع exprp;
        أرجع new AstGatePin{rangep->fileline(), exprp, rangep->clone_شجرة(true)};
    }
    Ast_Sen_شجرة* create_Sen_شجرة_Changed(File_سطر* fl, Ast_عقدة_تعبير* exprp) {
        أرجع new Ast_Sen_شجرة{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_شجرة* create_Sen_شجرة_Dot_Star(File_سطر* fl) {
        أرجع new Ast_Sen_شجرة{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_عقدة_تعبير* createGlobalClockParseRef(File_سطر* fl) {
        أرجع new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_شجرة* create_Global_Clock_Sen_شجرة(File_سطر* fl) {
        أرجع create_Sen_شجرة_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_عقدة* createNettype(File_سطر* fl, ثابت string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        أرجع nodep;
    }
    Ast_عقدة* createTypedef(File_سطر* fl, ثابت string& name, Ast_عقدة* attrsp, Ast_عقدة_DType* basep,
                           Ast_عقدة_Range* rangep) {
        AstTypedef* ثابت nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        أرجع nodep;
    }
    Ast_عقدة* createTypedefFwd(File_سطر* fl, ثابت string& name, ثابت VFwdType& fwdType) {
        AstTypedefFwd* ثابت nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        أرجع nodep;
    }
    ساكن فراغ endLabel(File_سطر* fl, ثابت Ast_عقدة* nodep, ثابت string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    ساكن فراغ endLabel(File_سطر* fl, ثابت string& name, ثابت string* endnamep) {
        إذا (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_عقدة::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    فراغ setDType(Ast_عقدة_DType* dtypep) {
        إذا (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_شجرة(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    فراغ setNetDelay(AstDelay* netDelayp) {
        إذا (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_شجرة(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    فراغ setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    فراغ pin_ادفع() {
        m_pin_مكدس.ادفع(m_pin_رقم);
        m_pin_رقم = 1;
    }
    فراغ pin_اسحب(File_سطر* fl) {
        إذا (VL_UNCOVERABLE(m_pin_مكدس.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_رقم = m_pin_مكدس.قمة();
        m_pin_مكدس.اسحب();
    }
    Ast_عقدة_DType* addRange(AstBasicDType* dtypep, Ast_عقدة_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        إذا (!rangesp) أرجع dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_عقدة_Range* rangearraysp = nullptr;
        إذا (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } وإلا {
            Ast_عقدة_Range* finalp = rangesp;
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
                    VL_DO_DANGLING(dtypep->delete_شجرة(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        أرجع createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_سطر* fileline, ثابت std::string& text) VL_MT_DISABLED;
    فراغ checkDpiVer(File_سطر* fileline, ثابت string& str) {
        إذا (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_سطر* flp, ثابت VDirection direction,
                                          Ast_عقدة_تعبير* skewp, Ast_عقدة* ثابت clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        لكل (Ast_عقدة *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            إذا (nextp) nextp->unlinkFrBackWithNext();
            إذا (itemsp && skewp) skewp = skewp->clone_شجرة(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_عقدة::addNextNull(itemsp, itemp);
        }
        أرجع itemsp;
    }

    فراغ setScopedSigAttr(Ast_عقدة* attrsp) {
        إذا (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_شجرة(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    فراغ createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_عقدة* cloneScopedSigAttr() ثابت {
        أرجع m_scopedSigAttr ? m_scopedSigAttr->clone_شجرة(true) : nullptr;
    }

    فراغ createGenericIface(Ast_عقدة* ثابت nodep, Ast_عقدة_Range* ثابت rangep,
                            Ast_عقدة* sigAttrListp, File_سطر* ثابت modportFileline = nullptr,
                            ثابت string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ثابت std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_عقدة::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_شجرة(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ثابت refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::مدخل;
        Ast_عقدة::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ساكن AstBegin* wrapInBegin(Ast_عقدة_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        لكل (Ast_عقدة_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), عقدة_Stmt);
            إذا (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            إذا (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_عقدة::addNext(resp, beginp);
        }
        أرجع resp;
    }
};
