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

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Const.h"
#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3ParseImp.h"  // Defines YYTYPE; before including bison header

#அந்தர்பாவ <stack>
#அந்தர்பாவ <vector>

class V3ParseGrammar final {
பொது:
    AstVar* m_varAttrp = nullptr;  // Current variable for attribute adding
    AstRange* m_gateRangep = nullptr;  // Current range for gate declarations
    Ast_முனை* m_scopedSigAttr = nullptr;  // Pointer to default signal attribute
    AstCase* m_caseAttrp = nullptr;  // Current case statement for attribute adding
    Ast_முனை_DType* m_varDTypep = nullptr;  // Pointer to data type for next signal declaration
    Ast_முனை_DType* m_memDTypep = nullptr;  // Pointer to data type for next member declaration
    VRandAttr m_memRand;  // 'rand'/'randc' qualifier for next member declaration
    AstDelay* m_netDelayp = nullptr;  // Pointer to delay for next signal declaration
    AstStrengthSpec* m_netStrengthp = nullptr;  // Pointer to strength for next net declaration
    File_வரி* m_instModuleFl = nullptr;  // Fileline of module referenced for instantiations
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
    Ast_முனை* m_modportProtoTasksp
        = nullptr;  // Prototype tasks from modport import/export with prototype

    முழு m_pin_இலக்கம் = -1;  // Pin number currently parsing
    std::அடுக்கு<முழு> m_pin_அடுக்கு;  // Queue of pin numbers being parsed

    நிலையான முழு s_type_Imp_இலக்கம்;  // Implicit type number, incremented each module

    // CONSTRUCTORS
    V3ParseGrammar() = இயல்பு;
    நிலையான V3ParseGrammar* singletonp() {
        நிலையான V3ParseGrammar singleton;
        திரும்பு &singleton;
    }

    // METHODS
    AstArg* argWrapList(Ast_முனை_கோவை* nodep) VL_MT_DISABLED;
    bool allTracingOn(மாறிலி File_வரி* fl) மாறிலி {
        திரும்பு v3Global.opt.trace() && m_tracingParse && fl->tracingOn();
    }
    AstRange* scrubRange(Ast_முனை_Range* rangep) VL_MT_DISABLED;
    AstRange* scrubRangeMulti(Ast_முனை_Range* rangep) VL_MT_DISABLED;
    Ast_முனை_Pre_Sel* scrubSel(Ast_முனை_கோவை* fromp, Ast_முனை_Pre_Sel* selp) VL_MT_DISABLED;
    Ast_முனை_DType* createArray(Ast_முனை_DType* basep, Ast_முனை_Range* rangep,
                              bool isPacked) VL_MT_DISABLED;
    AstVar* createVariable(File_வரி* fileline, மாறிலி string& name, Ast_முனை_Range* arrayp,
                           Ast_முனை* attrsp) VL_MT_DISABLED;
    AstAssignW* create_Supply_கோவை(File_வரி* fileline, மாறிலி string& name, முழு மதிப்பு) VL_MT_DISABLED;
    std::string textQuoted(File_வரி* fileline, மாறிலி std::string& text) {
        திரும்பு singletonp()->unquoteString(fileline, text);
    }
    Ast_முனை* createCell(File_வரி* fileline, மாறிலி string& name, AstPin* pinlistp,
                        Ast_முனை_Range* rangelistp) {
        // Must clone m_instParamp as may be comma'ed list of instances
        AstCell* மாறிலி nodep = new AstCell{
            fileline,
            singletonp()->m_instModuleFl,
            name,
            singletonp()->m_instModule,
            pinlistp,
            (singletonp()->m_instParamp ? singletonp()->m_instParamp->clone_மரம்(true) : nullptr),
            singletonp()->scrubRangeMulti(rangelistp)};
        nodep->trace(singletonp()->allTracingOn(fileline));
        திரும்பு nodep;
    }
    நிலையான Ast_முனை* create_Pattern_சாவி(Ast_முனை_கோவை* exprp) {
        // Assignment pattern key.  A bare identifier may name a structure
        // member rather than a constant, so pass it on as a Text node for
        // V3LinkDot to resolve; anything else is a constant expression.
        எனில் (மாறிலி AstParseRef* மாறிலி refp = VN_CAST(exprp, ParseRef)) {
            எனில் (!refp->lhsp() && !refp->ftaskrefp()) {
                Ast_முனை* மாறிலி textp = new AstText{refp->fileline(), refp->name()};
                VL_DO_DANGLING(exprp->delete_மரம்(), exprp);
                திரும்பு textp;
            }
        }
        திரும்பு exprp;
    }
    AstDisplay* createDisplayError(File_வரி* fileline) {
        AstDisplay* nodep = new AstDisplay{fileline, VDisplayType::DT_ERROR, "", nullptr, nullptr};
        Ast_முனை::addNext<Ast_முனை, Ast_முனை>(nodep, new AstStop{fileline, false});
        திரும்பு nodep;
    }
    // Apply the array marker of a bins declaration of values: '[]' creates a bin per value, and
    // '[N]' N bins holding the values (IEEE 1800-2023 19.5.1)
    நிலையான AstCoverBin* coverBinArray(AstCoverBin* binp, Ast_முனை_கோவை* sizep) {
        எனில் (!sizep) திரும்பு binp;
        binp->isArray(true);
        எனில் (VN_IS(sizep, Unbounded)) {
            VL_DO_DANGLING(sizep->delete_மரம்(), sizep);
        } இல்லையெனில் {
            binp->arraySizep(sizep);
        }
        திரும்பு binp;
    }
    // The array marker of another bins declaration, which supports only '[]'; true if present
    நிலையான bool coverBinArrayUnsized(Ast_முனை_கோவை* sizep) {
        எனில் (!sizep) திரும்பு false;
        எனில் (!VN_IS(sizep, Unbounded)) {
            sizep->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size (treated as '[]')");
        }
        VL_DO_DANGLING(sizep->delete_மரம்(), sizep);
        திரும்பு true;
    }
    Ast_முனை_கோவை* createGatePin(Ast_முனை_கோவை* exprp) {
        AstRange* மாறிலி rangep = m_gateRangep;
        எனில் (!rangep) திரும்பு exprp;
        திரும்பு new AstGatePin{rangep->fileline(), exprp, rangep->clone_மரம்(true)};
    }
    Ast_Sen_மரம்* create_Sen_மரம்_Changed(File_வரி* fl, Ast_முனை_கோவை* exprp) {
        திரும்பு new Ast_Sen_மரம்{fl, new AstSenItem{fl, VEdgeType::ET_CHANGED, exprp}};
    }
    Ast_Sen_மரம்* create_Sen_மரம்_Dot_Star(File_வரி* fl) {
        திரும்பு new Ast_Sen_மரம்{fl, new AstSenItem{fl, VEdgeType::ET_COMBO_STAR, nullptr}};
    }
    Ast_முனை_கோவை* createGlobalClockParseRef(File_வரி* fl) {
        திரும்பு new AstParseRef{fl, "__024global_clock", nullptr, nullptr};
    }
    Ast_Sen_மரம்* create_Global_Clock_Sen_மரம்(File_வரி* fl) {
        திரும்பு create_Sen_மரம்_Changed(fl, createGlobalClockParseRef(fl));
    }
    Ast_முனை* createNettype(File_வரி* fl, மாறிலி string& name) {
        // As nettypes are unsupported, we just alias to logic
        AstTypedef* மாறிலி nodep = new AstTypedef{fl, name, nullptr, VFlagChildDType{},
                                                 new AstBasicDType{fl, VFlagLogicPacked{}, 1}};
        V3ParseImp::parsep()->tagNodep(nodep);
        திரும்பு nodep;
    }
    Ast_முனை* createTypedef(File_வரி* fl, மாறிலி string& name, Ast_முனை* attrsp, Ast_முனை_DType* basep,
                           Ast_முனை_Range* rangep) {
        AstTypedef* மாறிலி nodep = new AstTypedef{fl, name, attrsp, VFlagChildDType{},
                                                 singletonp()->createArray(basep, rangep, false)};
        V3ParseImp::parsep()->tagNodep(nodep);
        திரும்பு nodep;
    }
    Ast_முனை* createTypedefFwd(File_வரி* fl, மாறிலி string& name, மாறிலி VFwdType& fwdType) {
        AstTypedefFwd* மாறிலி nodep = new AstTypedefFwd{fl, name, fwdType};
        V3ParseImp::parsep()->tagNodep(nodep);
        திரும்பு nodep;
    }
    நிலையான வெறுமை endLabel(File_வரி* fl, மாறிலி Ast_முனை* nodep, மாறிலி string* endnamep) {
        endLabel(fl, nodep->prettyName(), endnamep);
    }
    நிலையான வெறுமை endLabel(File_வரி* fl, மாறிலி string& name, மாறிலி string* endnamep) {
        எனில் (fl && endnamep && *endnamep != "" && name != *endnamep
            && name != Ast_முனை::prettyName(*endnamep)) {
            fl->v3warn(ENDLABEL, "End label '" << *endnamep << "' does not match begin label '"
                                               << name << "'");
        }
    }
    வெறுமை setDType(Ast_முனை_DType* dtypep) {
        எனில் (m_varDTypep) {
            UASSERT_OBJ(!m_varDTypep->backp(), m_varDTypep, "Should not link directly");
            VL_DO_CLEAR(m_varDTypep->delete_மரம்(), m_varDTypep = nullptr);
        }
        m_varDTypep = dtypep;
    }
    வெறுமை setNetDelay(AstDelay* netDelayp) {
        எனில் (m_netDelayp) {
            UASSERT_OBJ(!m_netDelayp->backp(), m_netDelayp, "Should not link directly");
            VL_DO_CLEAR(m_netDelayp->delete_மரம்(), m_netDelayp = nullptr);
        }
        m_netDelayp = netDelayp;
    }
    வெறுமை setNetStrength(AstStrengthSpec* netStrengthp) { m_netStrengthp = netStrengthp; }
    வெறுமை pin_தள்ளு() {
        m_pin_அடுக்கு.தள்ளு(m_pin_இலக்கம்);
        m_pin_இலக்கம் = 1;
    }
    வெறுமை pin_எடு(File_வரி* fl) {
        எனில் (VL_UNCOVERABLE(m_pin_அடுக்கு.empty())) fl->v3fatalSrc("Underflow of pin stack");
        m_pin_இலக்கம் = m_pin_அடுக்கு.உச்சி();
        m_pin_அடுக்கு.எடு();
    }
    Ast_முனை_DType* addRange(AstBasicDType* dtypep, Ast_முனை_Range* rangesp, bool isPacked) {
        // If dtypep isn't basic, don't use this, call createArray() instead
        எனில் (!rangesp) திரும்பு dtypep;

        // If rangesp is "wire [3:3][2:2][1:1] foo [5:5][4:4]"
        // then [1:1] becomes the basicdtype range; everything else is arraying
        // the final [5:5][4:4] will be passed in another call to createArray
        Ast_முனை_Range* rangearraysp = nullptr;
        எனில் (dtypep->isRanged()) {
            rangearraysp = rangesp;  // Already a range; everything is an array
        } இல்லையெனில் {
            Ast_முனை_Range* finalp = rangesp;
            போது (finalp->nextp()) finalp = VN_CAST(finalp->nextp(), Range);
            எனில் (finalp != rangesp) {
                finalp->unlinkFrBack();
                rangearraysp = rangesp;
            }
            எனில் (AstRange* மாறிலி finalRangep = VN_CAST(finalp, Range)) {  // not an UnsizedRange
                எனில் (dtypep->implicit()) {
                    // It's no longer implicit but a wire logic type
                    AstBasicDType* மாறிலி newp = new AstBasicDType{
                        dtypep->fileline(), VBasicDTypeKwd::LOGIC, dtypep->numeric(),
                        dtypep->width(), dtypep->widthMin()};
                    VL_DO_DANGLING(dtypep->delete_மரம்(), dtypep);
                    dtypep = newp;
                }
                dtypep->rangep(finalRangep);
            }
        }
        திரும்பு createArray(dtypep, rangearraysp, isPacked);
    }
    string unquoteString(File_வரி* fileline, மாறிலி std::string& text) VL_MT_DISABLED;
    வெறுமை checkDpiVer(File_வரி* fileline, மாறிலி string& str) {
        எனில் (str != "DPI-C" && !v3Global.opt.bboxSys())
            fileline->v3warn(E_UNSUPPORTED, "Unsupported DPI type '"
                                                << str
                                                << "': Use 'DPI-C' (IEEE 1800-2023 35.5.4)");
    }
    // Given a list of clocking declarations, put them in clocking items
    AstClockingItem* makeClockingItemList(File_வரி* flp, மாறிலி VDirection direction,
                                          Ast_முனை_கோவை* skewp, Ast_முனை* மாறிலி clockingDeclps) {
        AstClockingItem* itemsp = nullptr;
        க்கு (Ast_முனை *nodep = clockingDeclps, *nextp; nodep; nodep = nextp) {
            nextp = nodep->nextp();
            எனில் (nextp) nextp->unlinkFrBackWithNext();
            எனில் (itemsp && skewp) skewp = skewp->clone_மரம்(false);
            AstClockingItem* itemp = new AstClockingItem{flp, direction, skewp, nodep};
            itemsp = Ast_முனை::addNextNull(itemsp, itemp);
        }
        திரும்பு itemsp;
    }

    வெறுமை setScopedSigAttr(Ast_முனை* attrsp) {
        எனில் (m_scopedSigAttr) {  // clearing set attribute
            UASSERT_OBJ(!m_scopedSigAttr->backp(), m_scopedSigAttr, "Should not link directly");
            VL_DO_DANGLING(m_scopedSigAttr->delete_மரம்(), m_scopedSigAttr);
        }
        m_scopedSigAttr = attrsp;
    }

    வெறுமை createScopedSigAttr(VAttrType vattrT) {
        setScopedSigAttr(new AstAttrOf{V3ParseImp::parsep()->lexFileline(), vattrT});
    }

    Ast_முனை* cloneScopedSigAttr() மாறிலி {
        திரும்பு m_scopedSigAttr ? m_scopedSigAttr->clone_மரம்(true) : nullptr;
    }

    வெறுமை createGenericIface(Ast_முனை* மாறிலி nodep, Ast_முனை_Range* மாறிலி rangep,
                            Ast_முனை* sigAttrListp, File_வரி* மாறிலி modportFileline = nullptr,
                            மாறிலி string& modportstrp = "") {
        m_varDecl = VVarType::GPARAM;
        m_varIO = VDirection::NONE;
        setDType(new AstParseTypeDType{nodep->fileline(), VFwdType::GENERIC_INTERFACE});
        m_varDeclTyped = true;
        மாறிலி std::string uniqueName = "__VGIfaceParam" + nodep->name();
        Ast_முனை::addNext(nodep,
                         createVariable(nodep->fileline(), uniqueName, nullptr,
                                        sigAttrListp ? sigAttrListp->clone_மரம்(true) : nullptr));
        m_varDecl = VVarType::IFACEREF;
        AstIfaceGenericDType* மாறிலி refdtypep
            = new AstIfaceGenericDType{nodep->fileline(), modportFileline, modportstrp};
        setDType(refdtypep);
        m_varDeclTyped = true;
        m_varIO = VDirection::உள்ளீடு;
        Ast_முனை::addNext(nodep,
                         createVariable(nodep->fileline(), nodep->name(), rangep, sigAttrListp));
        m_varDecl = VVarType::VAR;
    }

    // Wrap all statements in the given list in an AstBegin (except those already an AstBegin)
    நிலையான AstBegin* wrapInBegin(Ast_முனை_Stmt* stmtsp) {
        AstBegin* resp = nullptr;
        க்கு (Ast_முனை_Stmt *nodep = stmtsp, *nextp; nodep; nodep = nextp) {
            nextp = VN_AS(nodep->nextp(), முனை_Stmt);
            எனில் (nextp) nextp->unlinkFrBackWithNext();
            AstBegin* beginp = VN_CAST(nodep, Begin);
            எனில் (!beginp) beginp = new AstBegin{nodep->fileline(), "", nodep, true};
            resp = Ast_முனை::addNext(resp, beginp);
        }
        திரும்பு resp;
    }
};
