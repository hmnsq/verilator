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

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Const.h"
#অন্তর্ভাব "V3Control.h"
#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#অন্তর্ভাব <stack>
#অন্তর্ভাব <vector>

class V3ParseGrammar final {
সার্বজনিক:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_গ্রন্থি* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_গ্রন্থি_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_গ্রন্থি_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_সারি* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_গ্রন্থি* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    পূর্ণাঙ্ক m_pin_অঙ্ক = -1;  // Pin number currently parsing
    std::স্তূপ<পূর্ণাঙ্ক> m_pin_স্তূপ;  // Queue of pin numbers being parsed

    স্থির পূর্ণাঙ্ক s_type_Imp_অঙ্ক;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = স্বভাব;
    স্থির V3ParseGrammar* singletonp() {
        স্থির V3ParseGrammar singleton;
        প্রত্যাগম &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_গ্রন্থি_রাশিমালা* nodep) VL_MT_DISABLED;
    bool allTracingOn(ধ্রুব File_সারি* fl) ধ্রুব {
        প্রত্যাগম v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_গ্রন্থি_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_গ্রন্থি_Range* rangep) VL_MT_DISABLED;
    Ast_গ্রন্থি_Pre_Sel* scrubSel(Ast_গ্রন্থি_রাশিমালা* fromp, Ast_গ্রন্থি_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_গ্রন্থি_DType* createArray(Ast_গ্রন্থি_DType* basep, Ast_গ্রন্থি_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_সারি* fileline, ধ্রুব string& name, Ast_গ্রন্থি_Range* arrayp,
                           Ast_গ্রন্থি* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_রাশিমালা(File_সারি* fileline, ধ্রুব string& name, পূর্ণাঙ্ক মান) VL_MT_DISABLED;
    std::string textQuoted(File_সারি* fileline, ধ্রুব std::string& text) {
        প্রত্যাগম singletonp()->unquoteString(fileline, text);
    }
    Ast_গ্রন্থি* createCell(File_সারি* fileline, ধ্রুব string& name, AstPin* pinlistp,
                        Ast_গ্রন্থি_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ধ্রুব nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_বৃক্ষ(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        প্রত্যাগম nodep;
    }
    স্থির Ast_গ্রন্থি* create_Pattern_চাবি(Ast_গ্রন্থি_রাশিমালা* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        যদি (ধ্রুব AstParseRef* ধ্রুব refp = VN_CAST(exprp, ParseRef)) {
            যদি (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_গ্রন্থি* ধ্রুব textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
                প্রত্যাগম textp;
            }
        }
        প্রত্যাগম exprp;
    }
    AstDisplay* createDisplayError(File_সারি* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_গ্রন্থি::addNext<Ast_গ্রন্থি, Ast_গ্রন্থি>(nodep, new AstStop{fileline, false});
        প্রত্যাগম nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    স্থির AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_গ্রন্থি_রাশিমালা* sizep) {
        যদি (!sizep) প্রত্যাগম binp;
        binp->isArray(true);
        যদি (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_বৃক্ষ(), sizep);
        } অন্যথা {
            binp->arraySizep(sizep);
        }
        প্রত্যাগম binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    স্থির bool coverBinArrayUnsized(Ast_গ্রন্থি_রাশিমালা* sizep) {
        যদি (!sizep) প্রত্যাগম false;
        যদি (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_বৃক্ষ(), sizep);
        প্রত্যাগম true;
    }
    Ast_গ্রন্থি_রাশিমালা* createGatePin(Ast_গ্রন্থি_রাশিমালা* exprp) {
        AstRange* ধ্রুব rangep = m_gateRangep;
        যদি (!rangep) প্রত্যাগম exprp;
        প্রত্যাগম new AstGatePin{rangep->fileline(), exprp, rangep->clone_বৃক্ষ(true)};
    }
    Ast_Sen_বৃক্ষ* create_Sen_বৃক্ষ_Changed(File_সারি* fl, Ast_গ্রন্থি_রাশিমালা* exprp) {
        প্রত্যাগম new Ast_Sen_বৃক্ষ{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_বৃক্ষ* create_Sen_বৃক্ষ_Dot_Star(File_সারি* fl) {
        প্রত্যাগম new Ast_Sen_বৃক্ষ{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_গ্রন্থি_রাশিমালা* createGlobalClockParseRef(File_সারি* fl) {
        প্রত্যাগম new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_বৃক্ষ* create_Global_Clock_Sen_বৃক্ষ(File_সারি* fl) {
        প্রত্যাগম create_Sen_বৃক্ষ_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_গ্রন্থি* createNettype(File_সারি* fl, ধ্রুব string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ধ্রুব nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        প্রত্যাগম nodep;
    }
    Ast_গ্রন্থি* createTypedef(File_সারি* fl, ধ্রুব string& name, Ast_গ্রন্থি* attrsp, Ast_গ্রন্থি_DType* basep,
                           Ast_গ্রন্থি_Range* rangep) {
        AstTypedef* ধ্রুব nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        প্রত্যাগম nodep;
    }
    Ast_গ্রন্থি* createTypedefFwd(File_সারি* fl, ধ্রুব string& name, ধ্রুব VFwdType& fwdType) {
        AstTypedefFwd* ধ্রুব nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        প্রত্যাগম nodep;
    }
    স্থির শূন্য endLabel(File_সারি* fl, ধ্রুব Ast_গ্রন্থি* nodep, ধ্রুব string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    স্থির শূন্য endLabel(File_সারি* fl, ধ্রুব string& name, ধ্রুব string* endnamep) {
        যদি (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_গ্রন্থি::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    শূন্য setDType(Ast_গ্রন্থি_DType* dtypep) {
        যদি (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_বৃক্ষ(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    শূন্য setNetDelay(AstDelay* netDelayp) {
        যদি (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_বৃক্ষ(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    শূন্য setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    শূন্য pin_ঠেলো() {
        m_pin_স্তূপ.ঠেলো(m_pin_অঙ্ক);
        m_pin_অঙ্ক = 1;
    }
    শূন্য pin_বের_করো(File_সারি* fl) {
        যদি (VL_UNCOVERABLE(m_pin_স্তূপ.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_অঙ্ক = m_pin_স্তূপ.শীর্ষ();
        m_pin_স্তূপ.বের_করো();
    }
    Ast_গ্রন্থি_DType* addRange(AstBasicDType* dtypep, Ast_গ্রন্থি_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        যদি (!rangesp) প্রত্যাগম dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_গ্রন্থি_Range* rangearraysp = nullptr;
        যদি (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } অন্যথা {
            Ast_গ্রন্থি_Range* finalp = rangesp;
            যাবত্ (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            যদি (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            যদি (AstRange* ধ্রুব finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                যদি (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ধ্রুব newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_বৃক্ষ(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        প্রত্যাগম createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_সারি* fileline, ধ্রুব std::string& text) VL_MT_DISABLED;
    শূন্য checkDpiVer(File_সারি* fileline, ধ্রুব string& str) {
        যদি (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_সারি* flp, ধ্রুব VDirection direction,
                                          Ast_গ্রন্থি_রাশিমালা* skewp, Ast_গ্রন্থি* ধ্রুব clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        প্রতি (Ast_গ্রন্থি *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            যদি (nextp) nextp->unlinkFrBackWithNext();
            যদি (itemsp && skewp) skewp = skewp->clone_বৃক্ষ(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_গ্রন্থি::addNextNull(itemsp, itemp);
        }
        প্রত্যাগম itemsp;
    }

    শূন্য setScopedSigAttr(Ast_গ্রন্থি* attrsp) {
        যদি (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_বৃক্ষ(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    শূন্য createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_গ্রন্থি* cloneScopedSigAttr() ধ্রুব {
        প্রত্যাগম m_scopedSigAttr ? m_scopedSigAttr->clone_বৃক্ষ(true) : nullptr;
    }

    শূন্য createGenericIface(Ast_গ্রন্থি* ধ্রুব nodep, Ast_গ্রন্থি_Range* ধ্রুব rangep,
                            Ast_গ্রন্থি* sigAttrListp, File_সারি* ধ্রুব modportFileline = nullptr,
                            ধ্রুব string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ধ্রুব std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_গ্রন্থি::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_বৃক্ষ(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ধ্রুব refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::প্রবেশ;
        Ast_গ্রন্থি::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    স্থির AstBegin* wrapInBegin(Ast_গ্রন্থি_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        প্রতি (Ast_গ্রন্থি_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), গ্রন্থি_Stmt);
            যদি (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            যদি (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_গ্রন্থি::addNext(resp, beginp);
        }
        প্রত্যাগম resp;
    }
};
