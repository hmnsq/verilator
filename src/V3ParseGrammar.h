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

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#అన్తర్భావ <stack>
#అన్తర్భావ <vector>

class V3ParseGrammar final {
బహిరంగ:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_కణుపు* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_కణుపు_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_కణుపు_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_పంక్తి* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
    AstPin* m_instParamp = nullptr;  // Parameters for instantiations
    string m_instModule;  // Name of module referenced for instantiations
    VVarType m_varDecl = VVarType::UNKNOWN;  // Type for next signal declaration (reg/wire/etc)
    VDirection m_varIO = VDirection::NONE;  // Direction for next signal declaration (reg/wire/etc)
    VLifetime m_varLifetime;  // Static/Automatic for next signal
    V3Control::Var_Spec_రకం m_vlt_Var_Spec_రకం = V3Control::Var_Spec_రకం::VAR;
    bool m_impliedDecl = false;  // Allow implied wire declarations
    bool m_varDeclTyped = false;  // Var got reg/wire for dedup check
    bool m_pinAnsi = false;  // In ANSI parameter or port list
    bool m_tracingParse = true;  // Tracing disable for parser
    bool m_లోపల_Implements = false;  // Is inside class implements list
    bool m_insideProperty = false;  // Is inside property declaration
    bool m_specifyignWarned = false;  // Issued a SPECIFYIGN warning
    bool m_typed_Property_పోర్టు = false;  // Typed property port occurred on port lists
    bool m_modport_Imp_Exp_చురుకైన
        = false;  // Standalone ID is a tf_identifier instead of port_identifier
    bool m_modport_Imp_Exp_చివరి_Is_Export
        = false;  // Last import_export statement in modportPortsDecl is an export
    Ast_కణుపు* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    పూర్ణాంకం m_pin_అంకె = -1;  // Pin number currently parsing
    std::దొంతర<పూర్ణాంకం> m_pin_దొంతర;  // Queue of pin numbers being parsed

    స్థిర పూర్ణాంకం s_type_Imp_అంకె;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = అప్రమేయ;
    స్థిర V3ParseGrammar* singletonp() {
        స్థిర V3ParseGrammar singleton;
        తిరిగి &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_కణుపు_సమాసం* nodep) VL_MT_DISABLED;
    bool allTracingOn(స్థిరరాశి File_పంక్తి* fl) స్థిరరాశి {
        తిరిగి v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_కణుపు_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_కణుపు_Range* rangep) VL_MT_DISABLED;
    Ast_కణుపు_Pre_Sel* scrubSel(Ast_కణుపు_సమాసం* fromp, Ast_కణుపు_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_కణుపు_DType* createArray(Ast_కణుపు_DType* basep, Ast_కణుపు_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_పంక్తి* fileline, స్థిరరాశి string& name, Ast_కణుపు_Range* arrayp,
                           Ast_కణుపు* attrsp) VL_MT_DISABLED;
    Ast_కేటాయించు_W* create_Supply_సమాసం(File_పంక్తి* fileline, స్థిరరాశి string& name, పూర్ణాంకం విలువ) VL_MT_DISABLED;
    std::string పాఠ్యం_Quoted(File_పంక్తి* fileline, స్థిరరాశి std::string& పాఠ్యం) {
        తిరిగి singletonp()->unquoteString(fileline, పాఠ్యం);
    }
    Ast_కణుపు* createCell(File_పంక్తి* fileline, స్థిరరాశి string& name, AstPin* pinlistp,
                        Ast_కణుపు_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* స్థిరరాశి nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_వృక్షం(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        తిరిగి nodep;
    }
    స్థిర Ast_కణుపు* create_నమూనా_కీలకం(Ast_కణుపు_సమాసం* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        ఒకవేళ (స్థిరరాశి AstParseRef* స్థిరరాశి refp = VN_CAST(exprp, ParseRef)) {
            ఒకవేళ (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_కణుపు* స్థిరరాశి textp = new Ast_పాఠ్యం{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_వృక్షం(), exprp);
                తిరిగి textp;
            }
        }
        తిరిగి exprp;
    }
    AstDisplay* createDisplayError(File_పంక్తి* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_కణుపు::కూడు_తదుపరి<Ast_కణుపు, Ast_కణుపు>(nodep, new AstStop{fileline, false});
        తిరిగి nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    స్థిర AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_కణుపు_సమాసం* sizep) {
        ఒకవేళ (!sizep) తిరిగి binp;
        binp->isArray(true);
        ఒకవేళ (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_వృక్షం(), sizep);
        } లేకపోతే {
            binp->arraySizep(sizep);
        }
        తిరిగి binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    స్థిర bool coverBinArrayUnsized(Ast_కణుపు_సమాసం* sizep) {
        ఒకవేళ (!sizep) తిరిగి false;
        ఒకవేళ (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_వృక్షం(), sizep);
        తిరిగి true;
    }
    Ast_కణుపు_సమాసం* createGatePin(Ast_కణుపు_సమాసం* exprp) {
        AstRange* స్థిరరాశి rangep = m_gateRangep;
        ఒకవేళ (!rangep) తిరిగి exprp;
        తిరిగి new AstGatePin{rangep->fileline(), exprp, rangep->clone_వృక్షం(true)};
    }
    Ast_Sen_వృక్షం* create_Sen_వృక్షం_మారింది(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp) {
        తిరిగి new Ast_Sen_వృక్షం{fl, new Ast_Sen_అంశం{fl, VEdgeType::ET_మారింది, exprp}};
    }
    Ast_Sen_వృక్షం* create_Sen_వృక్షం_Dot_నక్షత్రం(File_పంక్తి* fl) {
        తిరిగి new Ast_Sen_వృక్షం{fl, new Ast_Sen_అంశం{fl, VEdgeType::ET_COMBO_నక్షత్రం, nullptr}};
    }
    Ast_కణుపు_సమాసం* createGlobalClockParseRef(File_పంక్తి* fl) {
        తిరిగి new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_వృక్షం* create_Global_Clock_Sen_వృక్షం(File_పంక్తి* fl) {
        తిరిగి create_Sen_వృక్షం_మారింది(fl, createGlobalClockParseRef(fl));
    }
    Ast_కణుపు* createNettype(File_పంక్తి* fl, స్థిరరాశి string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* స్థిరరాశి nodep = new AstTypedef{fl, name, nullptr, VFlag_బిడ్డ_DType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->ట్యాగ్_Nodep(nodep);
        తిరిగి nodep;
    }
    Ast_కణుపు* createTypedef(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* attrsp, Ast_కణుపు_DType* basep,
                           Ast_కణుపు_Range* rangep) {
        AstTypedef* స్థిరరాశి nodep = new AstTypedef{fl, name, attrsp, VFlag_బిడ్డ_DType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->ట్యాగ్_Nodep(nodep);
        తిరిగి nodep;
    }
    Ast_కణుపు* createTypedefFwd(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి VFwdType& fwdType) {
        AstTypedefFwd* స్థిరరాశి nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->ట్యాగ్_Nodep(nodep);
        తిరిగి nodep;
    }
    స్థిర శూన్య చివర_Label(File_పంక్తి* fl, స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string* endnamep) {
        చివర_Label(fl, nodep->prettyName(), endnamep);
    }
    స్థిర శూన్య చివర_Label(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string* endnamep) {
        ఒకవేళ (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_కణుపు::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    శూన్య సమితి_DType(Ast_కణుపు_DType* dtypep) {
        ఒకవేళ (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_వృక్షం(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    శూన్య సమితి_Net_Delay(AstDelay* netDelayp) {
        ఒకవేళ (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_వృక్షం(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    శూన్య సమితి_Net_Strength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    శూన్య pin_నెట్టు() {
        m_pin_దొంతర.నెట్టు(m_pin_అంకె);
        m_pin_అంకె = 1;
    }
    శూన్య pin_తీయి(File_పంక్తి* fl) {
        ఒకవేళ (VL_UNCOVERABLE(m_pin_దొంతర.ఖాళీలు())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_అంకె = m_pin_దొంతర.శిఖరం();
        m_pin_దొంతర.తీయి();
    }
    Ast_కణుపు_DType* కూడు_Range(AstBasicDType* dtypep, Ast_కణుపు_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        ఒకవేళ (!rangesp) తిరిగి dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_కణుపు_Range* rangearraysp = nullptr;
        ఒకవేళ (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } లేకపోతే {
            Ast_కణుపు_Range* finalp = rangesp;
            అయితే (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            ఒకవేళ (finalp != rangesp) {
                finalp->unlink_Fr_వెనుక();
                rangearraysp = rangesp;
            }
            ఒకవేళ (AstRange* స్థిరరాశి finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                ఒకవేళ (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* స్థిరరాశి newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->వెడల్పు(), dtypep->వెడల్పు_Min()};
                    VL_DO_DANGLING(dtypep->delete_వృక్షం(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        తిరిగి createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_పంక్తి* fileline, స్థిరరాశి std::string& పాఠ్యం) VL_MT_DISABLED;
    శూన్య checkDpiVer(File_పంక్తి* fileline, స్థిరరాశి string& str) {
        ఒకవేళ (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    Ast_Clocking_అంశం* make_Clocking_అంశం_List(File_పంక్తి* flp, స్థిరరాశి VDirection direction,
                                          Ast_కణుపు_సమాసం* skewp, Ast_కణుపు* స్థిరరాశి clockingDeclps) {
        Ast_Clocking_అంశం* itemsp = nullptr;
        కొరకు (Ast_కణుపు *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ఒకవేళ (nextp) nextp->unlink_Fr_వెనుక_With_తదుపరి();
            ఒకవేళ (itemsp && skewp) skewp = skewp->clone_వృక్షం(false);
            Ast_Clocking_అంశం* itemp = new Ast_Clocking_అంశం{flp, direction, skewp, nodep};
            itemsp = Ast_కణుపు::కూడు_తదుపరి_Null(itemsp, itemp);
        }
        తిరిగి itemsp;
    }

    శూన్య సమితి_Scoped_Sig_Attr(Ast_కణుపు* attrsp) {
        ఒకవేళ (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_వృక్షం(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    శూన్య createScopedSigAttr(VAttrType vattrT) {
        సమితి_Scoped_Sig_Attr(new Ast_Attr_యొక్క{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_కణుపు* cloneScopedSigAttr() స్థిరరాశి {
        తిరిగి m_scopedSigAttr ? m_scopedSigAttr->clone_వృక్షం(true) : nullptr;
    }

    శూన్య createGenericIface(Ast_కణుపు* స్థిరరాశి nodep, Ast_కణుపు_Range* స్థిరరాశి rangep,
                            Ast_కణుపు* sigAttrListp, File_పంక్తి* స్థిరరాశి modportFileline = nullptr,
                            స్థిరరాశి string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        సమితి_DType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        స్థిరరాశి std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_కణుపు::కూడు_తదుపరి(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_వృక్షం(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* స్థిరరాశి refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        సమితి_DType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::ఆదానం;
        Ast_కణుపు::కూడు_తదుపరి(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    స్థిర AstBegin* wrap_లోపల_Begin(Ast_కణుపు_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        కొరకు (Ast_కణుపు_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), కణుపు_Stmt);
            ఒకవేళ (nextp) nextp->unlink_Fr_వెనుక_With_తదుపరి();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            ఒకవేళ (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_కణుపు::కూడు_తదుపరి(resp, beginp);
        }
        తిరిగి resp;
    }
};
