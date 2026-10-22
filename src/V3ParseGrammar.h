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

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Const.h"
#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#ଅନ୍ତର୍ଭାଵ <stack>
#ଅନ୍ତର୍ଭାଵ <vector>

class V3ParseGrammar final {
ସର୍ବସାଧାରଣ:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_ଗଣ୍ଠି* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_ଗଣ୍ଠି_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_ଗଣ୍ଠି_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_ଧାଡ଼ି* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_ଗଣ୍ଠି* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_pin_ଅଙ୍କ = -1;  // Pin number currently parsing
    std::ଗଦା<ପୂର୍ଣ୍ଣସଂଖ୍ୟା> m_pin_ଗଦା;  // Queue of pin numbers being parsed

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_type_Imp_ଅଙ୍କ;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ସ୍ଥିର V3ParseGrammar* singletonp() {
        ସ୍ଥିର V3ParseGrammar singleton;
        ଫେରନ୍ତୁ &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* nodep) VL_MT_DISABLED;
    bool allTracingOn(ଅଚଳ File_ଧାଡ଼ି* fl) ଅଚଳ {
        ଫେରନ୍ତୁ v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_ଗଣ୍ଠି_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_ଗଣ୍ଠି_Range* rangep) VL_MT_DISABLED;
    Ast_ଗଣ୍ଠି_Pre_Sel* scrubSel(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* fromp, Ast_ଗଣ୍ଠି_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_ଗଣ୍ଠି_DType* createArray(Ast_ଗଣ୍ଠି_DType* basep, Ast_ଗଣ୍ଠି_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_ଧାଡ଼ି* fileline, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_Range* arrayp,
                           Ast_ଗଣ୍ଠି* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_ପଦସମଷ୍ଟି(File_ଧାଡ଼ି* fileline, ଅଚଳ string& name, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) VL_MT_DISABLED;
    std::string textQuoted(File_ଧାଡ଼ି* fileline, ଅଚଳ std::string& text) {
        ଫେରନ୍ତୁ singletonp()->unquoteString(fileline, text);
    }
    Ast_ଗଣ୍ଠି* createCell(File_ଧାଡ଼ି* fileline, ଅଚଳ string& name, AstPin* pinlistp,
                        Ast_ଗଣ୍ଠି_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* ଅଚଳ nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_ବୃକ୍ଷ(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        ଫେରନ୍ତୁ nodep;
    }
    ସ୍ଥିର Ast_ଗଣ୍ଠି* create_Pattern_ଚାବି(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        ଯଦି (ଅଚଳ AstParseRef* ଅଚଳ refp = VN_CAST(exprp, ParseRef)) {
            ଯଦି (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_ଗଣ୍ଠି* ଅଚଳ textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_ବୃକ୍ଷ(), exprp);
                ଫେରନ୍ତୁ textp;
            }
        }
        ଫେରନ୍ତୁ exprp;
    }
    AstDisplay* createDisplayError(File_ଧାଡ଼ି* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_ଗଣ୍ଠି::addNext<Ast_ଗଣ୍ଠି, Ast_ଗଣ୍ଠି>(nodep, new AstStop{fileline, false});
        ଫେରନ୍ତୁ nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    ସ୍ଥିର AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sizep) {
        ଯଦି (!sizep) ଫେରନ୍ତୁ binp;
        binp->isArray(true);
        ଯଦି (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_ବୃକ୍ଷ(), sizep);
        } ଅନ୍ୟଥା {
            binp->arraySizep(sizep);
        }
        ଫେରନ୍ତୁ binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    ସ୍ଥିର bool coverBinArrayUnsized(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* sizep) {
        ଯଦି (!sizep) ଫେରନ୍ତୁ false;
        ଯଦି (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_ବୃକ୍ଷ(), sizep);
        ଫେରନ୍ତୁ true;
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* createGatePin(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        AstRange* ଅଚଳ rangep = m_gateRangep;
        ଯଦି (!rangep) ଫେରନ୍ତୁ exprp;
        ଫେରନ୍ତୁ new AstGatePin{rangep->fileline(), exprp, rangep->clone_ବୃକ୍ଷ(true)};
    }
    Ast_Sen_ବୃକ୍ଷ* create_Sen_ବୃକ୍ଷ_Changed(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp) {
        ଫେରନ୍ତୁ new Ast_Sen_ବୃକ୍ଷ{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_ବୃକ୍ଷ* create_Sen_ବୃକ୍ଷ_Dot_Star(File_ଧାଡ଼ି* fl) {
        ଫେରନ୍ତୁ new Ast_Sen_ବୃକ୍ଷ{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* createGlobalClockParseRef(File_ଧାଡ଼ି* fl) {
        ଫେରନ୍ତୁ new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_ବୃକ୍ଷ* create_Global_Clock_Sen_ବୃକ୍ଷ(File_ଧାଡ଼ି* fl) {
        ଫେରନ୍ତୁ create_Sen_ବୃକ୍ଷ_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_ଗଣ୍ଠି* createNettype(File_ଧାଡ଼ି* fl, ଅଚଳ string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* ଅଚଳ nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        ଫେରନ୍ତୁ nodep;
    }
    Ast_ଗଣ୍ଠି* createTypedef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* attrsp, Ast_ଗଣ୍ଠି_DType* basep,
                           Ast_ଗଣ୍ଠି_Range* rangep) {
        AstTypedef* ଅଚଳ nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        ଫେରନ୍ତୁ nodep;
    }
    Ast_ଗଣ୍ଠି* createTypedefFwd(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ VFwdType& fwdType) {
        AstTypedefFwd* ଅଚଳ nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        ଫେରନ୍ତୁ nodep;
    }
    ସ୍ଥିର ଶୂନ୍ୟ endLabel(File_ଧାଡ଼ି* fl, ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    ସ୍ଥିର ଶୂନ୍ୟ endLabel(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string* endnamep) {
        ଯଦି (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_ଗଣ୍ଠି::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    ଶୂନ୍ୟ setDType(Ast_ଗଣ୍ଠି_DType* dtypep) {
        ଯଦି (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_ବୃକ୍ଷ(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    ଶୂନ୍ୟ setNetDelay(AstDelay* netDelayp) {
        ଯଦି (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_ବୃକ୍ଷ(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    ଶୂନ୍ୟ setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    ଶୂନ୍ୟ pin_ଠେଲ() {
        m_pin_ଗଦା.ଠେଲ(m_pin_ଅଙ୍କ);
        m_pin_ଅଙ୍କ = 1;
    }
    ଶୂନ୍ୟ pin_କାଢ଼(File_ଧାଡ଼ି* fl) {
        ଯଦି (VL_UNCOVERABLE(m_pin_ଗଦା.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_ଅଙ୍କ = m_pin_ଗଦା.ଶୀର୍ଷ();
        m_pin_ଗଦା.କାଢ଼();
    }
    Ast_ଗଣ୍ଠି_DType* addRange(AstBasicDType* dtypep, Ast_ଗଣ୍ଠି_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        ଯଦି (!rangesp) ଫେରନ୍ତୁ dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_ଗଣ୍ଠି_Range* rangearraysp = nullptr;
        ଯଦି (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_Range* finalp = rangesp;
            ଯେତେବେଳେ (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            ଯଦି (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            ଯଦି (AstRange* ଅଚଳ finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                ଯଦି (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* ଅଚଳ newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_ବୃକ୍ଷ(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        ଫେରନ୍ତୁ createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_ଧାଡ଼ି* fileline, ଅଚଳ std::string& text) VL_MT_DISABLED;
    ଶୂନ୍ୟ checkDpiVer(File_ଧାଡ଼ି* fileline, ଅଚଳ string& str) {
        ଯଦି (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_ଧାଡ଼ି* flp, ଅଚଳ VDirection direction,
                                          Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* skewp, Ast_ଗଣ୍ଠି* ଅଚଳ clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            ଯଦି (nextp) nextp->unlinkFrBackWithNext();
            ଯଦି (itemsp && skewp) skewp = skewp->clone_ବୃକ୍ଷ(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_ଗଣ୍ଠି::addNextNull(itemsp, itemp);
        }
        ଫେରନ୍ତୁ itemsp;
    }

    ଶୂନ୍ୟ setScopedSigAttr(Ast_ଗଣ୍ଠି* attrsp) {
        ଯଦି (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_ବୃକ୍ଷ(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    ଶୂନ୍ୟ createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_ଗଣ୍ଠି* cloneScopedSigAttr() ଅଚଳ {
        ଫେରନ୍ତୁ m_scopedSigAttr ? m_scopedSigAttr->clone_ବୃକ୍ଷ(true) : nullptr;
    }

    ଶୂନ୍ୟ createGenericIface(Ast_ଗଣ୍ଠି* ଅଚଳ nodep, Ast_ଗଣ୍ଠି_Range* ଅଚଳ rangep,
                            Ast_ଗଣ୍ଠି* sigAttrListp, File_ଧାଡ଼ି* ଅଚଳ modportFileline = nullptr,
                            ଅଚଳ string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        ଅଚଳ std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_ଗଣ୍ଠି::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_ବୃକ୍ଷ(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* ଅଚଳ refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::ନିବେଶ;
        Ast_ଗଣ୍ଠି::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    ସ୍ଥିର AstBegin* wrapInBegin(Ast_ଗଣ୍ଠି_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        ପାଇଁ (Ast_ଗଣ୍ଠି_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), ଗଣ୍ଠି_Stmt);
            ଯଦି (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            ଯଦି (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_ଗଣ୍ଠି::addNext(resp, beginp);
        }
        ଫେରନ୍ତୁ resp;
    }
};
