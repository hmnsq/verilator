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

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Const.h"
#অন্তর্ভাৱ "V3Control.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#অন্তর্ভাৱ <stack>
#অন্তর্ভাৱ <vector>

class V3ParseGrammar final {
সার্ৱজনিক:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_গ্ৰন্থি* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_গ্ৰন্থি_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_গ্ৰন্থি_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_সাৰি* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_গ্ৰন্থি* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    পূর্ণাঙ্ক m_pin_অঙ্ক = -1;  // Pin number currently parsing
    std::স্তূপ<পূর্ণাঙ্ক> m_pin_স্তূপ;  // Queue of pin numbers being parsed

    স্থির পূর্ণাঙ্ক s_type_Imp_অঙ্ক;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = স্ৱভাৱ;
    স্থির V3ParseGrammar* singletonp() {
        স্থির V3ParseGrammar singleton;
        ঘূৰাই &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_গ্ৰন্থি_ৰাশিমালা* nodep) VL_MT_DISABLED;
    bool allTracingOn(ধ্রুৱ File_সাৰি* fl) ধ্রুৱ {
        ঘূৰাই v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_গ্ৰন্থি_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_গ্ৰন্থি_Range* rangep) VL_MT_DISABLED;
    Ast_গ্ৰন্থি_Pre_Sel* scrubSel(Ast_গ্ৰন্থি_ৰাশিমালা* fromp, Ast_গ্ৰন্থি_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_গ্ৰন্থি_DType* createArray(Ast_গ্ৰন্থি_DType* basep, Ast_গ্ৰন্থি_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_সাৰি* fileline, ধ্রুৱ string& name, Ast_গ্ৰন্থি_Range* arrayp,
                           Ast_গ্ৰন্থি* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_ৰাশিমালা(File_সাৰি* fileline, ধ্রুৱ string& name, পূর্ণাঙ্ক মান) VL_MT_DISABLED;
    std::string textQuoted(File_সাৰি* fileline, ধ্রুৱ std::string& text) {
        ঘূৰাই singletonp()->unquoteString(fileline, text);
    }
    Ast_গ্ৰন্থি* createCell(File_সাৰি* fileline, ধ্রুৱ string& name, AstPin* pinlistp,
                        Ast_গ্ৰন্থি_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ধ্রুৱ nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_বৃক্ষ(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        ঘূৰাই nodep;
    }
    স্থির Ast_গ্ৰন্থি* create_Pattern_চাবি(Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        যদি (ধ্রুৱ AstParseRef* ধ্রুৱ refp = VN_CAST(exprp, ParseRef)) {
            যদি (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_গ্ৰন্থি* ধ্রুৱ textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_বৃক্ষ(), exprp);
                ঘূৰাই textp;
            }
        }
        ঘূৰাই exprp;
    }
    AstDisplay* createDisplayError(File_সাৰি* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_গ্ৰন্থি::addNext<Ast_গ্ৰন্থি, Ast_গ্ৰন্থি>(nodep, new AstStop{fileline, false});
        ঘূৰাই nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    স্থির AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_গ্ৰন্থি_ৰাশিমালা* sizep) {
        যদি (!sizep) ঘূৰাই binp;
        binp->isArray(true);
        যদি (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_বৃক্ষ(), sizep);
        } অন্যথা {
            binp->arraySizep(sizep);
        }
        ঘূৰাই binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    স্থির bool coverBinArrayUnsized(Ast_গ্ৰন্থি_ৰাশিমালা* sizep) {
        যদি (!sizep) ঘূৰাই false;
        যদি (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_বৃক্ষ(), sizep);
        ঘূৰাই true;
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* createGatePin(Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        AstRange* ধ্রুৱ rangep = m_gateRangep;
        যদি (!rangep) ঘূৰাই exprp;
        ঘূৰাই new AstGatePin{rangep->fileline(), exprp, rangep->clone_বৃক্ষ(true)};
    }
    Ast_Sen_বৃক্ষ* create_Sen_বৃক্ষ_Changed(File_সাৰি* fl, Ast_গ্ৰন্থি_ৰাশিমালা* exprp) {
        ঘূৰাই new Ast_Sen_বৃক্ষ{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_বৃক্ষ* create_Sen_বৃক্ষ_Dot_Star(File_সাৰি* fl) {
        ঘূৰাই new Ast_Sen_বৃক্ষ{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_গ্ৰন্থি_ৰাশিমালা* createGlobalClockParseRef(File_সাৰি* fl) {
        ঘূৰাই new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_বৃক্ষ* create_Global_Clock_Sen_বৃক্ষ(File_সাৰি* fl) {
        ঘূৰাই create_Sen_বৃক্ষ_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_গ্ৰন্থি* createNettype(File_সাৰি* fl, ধ্রুৱ string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ধ্রুৱ nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        ঘূৰাই nodep;
    }
    Ast_গ্ৰন্থি* createTypedef(File_সাৰি* fl, ধ্রুৱ string& name, Ast_গ্ৰন্থি* attrsp, Ast_গ্ৰন্থি_DType* basep,
                           Ast_গ্ৰন্থি_Range* rangep) {
        AstTypedef* ধ্রুৱ nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        ঘূৰাই nodep;
    }
    Ast_গ্ৰন্থি* createTypedefFwd(File_সাৰি* fl, ধ্রুৱ string& name, ধ্রুৱ VFwdType& fwdType) {
        AstTypedefFwd* ধ্রুৱ nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        ঘূৰাই nodep;
    }
    স্থির শূন্য endLabel(File_সাৰি* fl, ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    স্থির শূন্য endLabel(File_সাৰি* fl, ধ্রুৱ string& name, ধ্রুৱ string* endnamep) {
        যদি (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_গ্ৰন্থি::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    শূন্য setDType(Ast_গ্ৰন্থি_DType* dtypep) {
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
    শূন্য pin_বেৰ_কৰো(File_সাৰি* fl) {
        যদি (VL_UNCOVERABLE(m_pin_স্তূপ.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_অঙ্ক = m_pin_স্তূপ.শীৰ্ষ();
        m_pin_স্তূপ.বেৰ_কৰো();
    }
    Ast_গ্ৰন্থি_DType* addRange(AstBasicDType* dtypep, Ast_গ্ৰন্থি_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        যদি (!rangesp) ঘূৰাই dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_গ্ৰন্থি_Range* rangearraysp = nullptr;
        যদি (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } অন্যথা {
            Ast_গ্ৰন্থি_Range* finalp = rangesp;
            যেতিয়ালৈ (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            যদি (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            যদি (AstRange* ধ্রুৱ finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                যদি (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ধ্রুৱ newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_বৃক্ষ(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        ঘূৰাই createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_সাৰি* fileline, ধ্রুৱ std::string& text) VL_MT_DISABLED;
    শূন্য checkDpiVer(File_সাৰি* fileline, ধ্রুৱ string& str) {
        যদি (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_সাৰি* flp, ধ্রুৱ VDirection direction,
                                          Ast_গ্ৰন্থি_ৰাশিমালা* skewp, Ast_গ্ৰন্থি* ধ্রুৱ clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        বাবে (Ast_গ্ৰন্থি *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            যদি (nextp) nextp->unlinkFrBackWithNext();
            যদি (itemsp && skewp) skewp = skewp->clone_বৃক্ষ(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_গ্ৰন্থি::addNextNull(itemsp, itemp);
        }
        ঘূৰাই itemsp;
    }

    শূন্য setScopedSigAttr(Ast_গ্ৰন্থি* attrsp) {
        যদি (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_বৃক্ষ(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    শূন্য createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_গ্ৰন্থি* cloneScopedSigAttr() ধ্রুৱ {
        ঘূৰাই m_scopedSigAttr ? m_scopedSigAttr->clone_বৃক্ষ(true) : nullptr;
    }

    শূন্য createGenericIface(Ast_গ্ৰন্থি* ধ্রুৱ nodep, Ast_গ্ৰন্থি_Range* ধ্রুৱ rangep,
                            Ast_গ্ৰন্থি* sigAttrListp, File_সাৰি* ধ্রুৱ modportFileline = nullptr,
                            ধ্রুৱ string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ধ্রুৱ std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_গ্ৰন্থি::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_বৃক্ষ(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ধ্রুৱ refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::প্ৰবেশ;
        Ast_গ্ৰন্থি::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    স্থির AstBegin* wrapInBegin(Ast_গ্ৰন্থি_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        বাবে (Ast_গ্ৰন্থি_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), গ্ৰন্থি_Stmt);
            যদি (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            যদি (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_গ্ৰন্থি::addNext(resp, beginp);
        }
        ঘূৰাই resp;
    }
};
