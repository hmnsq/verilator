// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Expression width calculations
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
// V3Width's Transformations:
//      Top down traversal:
//          Determine width of sub-expressions
//              width() = # bits upper expression wants, 0 for anything-goes
//              widthUnsized() = # bits for unsized constant, or 0 if it's sized
//              widthMin() = Alternative acceptable width for linting, or width() if sized
//              Determine this subop's width, can be either:
//                  Fixed width X
//                  Unsized, min width X   ('d5 is unsized, min 3 bits.)
//              Pass up:
//                  width() = # bits this expression generates
//                  widthSized() = true if all constants sized, else false
//          Compute size of this expression
//          Lint warn about mismatches
//              If expr size != subop fixed, bad
//              If expr size  < subop unsized minimum, bad
//              If expr size != subop, edit netlist
//                      For == and similar ops, if multibit underneath, add a REDOR
//                      If subop larger, add a EXTRACT
//                      If subop smaller, add a EXTEND
//          Pass size to sub-expressions if required (+/-* etc)
//              FINAL = true.
//              Subexpressions lint and extend as needed
//
//*************************************************************************
// Signedness depends on:
//      Decimal numbers are signed
//      Based numbers are unsigned unless 's' prefix
//      Comparison results are unsigned
//      Bit&Part selects are unsigned, even if whole
//      Concatenates are unsigned
//      Ignore signedness of self-determined:
//              shift rhs, ** rhs, x?: lhs, concat and replicate members
//      Else, if any operand unsigned, output unsigned
//
// Real number rules:
//      Real numbers are real (duh)
//      Reals convert to integers by rounding
//      Reals init to 0.0
//      Logicals convert compared to zero
//      If any operand is real, result is real
//*************************************************************************
// V3Width is the only visitor that uses vup.  We could switch to using userp,
// though note some iterators operate on next() and so would need to pass the
// same value on each nextp().
//*************************************************************************
// See notes in internal.txt about misuse of iterateAndNext and use of
// iterateSubtreeReturnEdits.
//*************************************************************************

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Width.h"

#అన్తర్భావ "V3Assert.h"
#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Begin.h"
#అన్తర్భావ "V3Const.h"
#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3LinkLValue.h"
#అన్తర్భావ "V3MemberMap.h"
#అన్తర్భావ "V3Number.h"
#అన్తర్భావ "V3Randomize.h"
#అన్తర్భావ "V3String.h"
#అన్తర్భావ "V3Task.h"
#అన్తర్భావ "V3UniqueNames.h"
#అన్తర్భావ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#నిర్వచించు VERILATOR_V3_వెడల్పు_CPP
#అన్తర్భావ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<స్థిరరాశి Ast_కణుపు*, AstClass*> m_cache;

    AstClass* కనుగొను_Cached(Ast_కణుపు* nodep) {
        ఒకవేళ (!nodep) తిరిగి శూన్యసూచిక;
        స్థిరరాశి స్వయంచాలక it = m_cache.కనుగొను(nodep);
        ఒకవేళ (it != m_cache.చివర()) తిరిగి it->రెండవ;

        AstClass* classp = శూన్యసూచిక;
        ఒకవేళ (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = కనుగొను_Cached(nodep->backp());
        } లేకపోతే ఒకవేళ (AstClass* స్థిరరాశి parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } లేకపోతే ఒకవేళ (AstClassPackage* స్థిరరాశి packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } లేకపోతే {
            classp = కనుగొను_Cached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        తిరిగి classp;
    }

బహిరంగ:
    AstClass* కనుగొను(Ast_కణుపు* nodep) {
        ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(nodep, Class)) తిరిగి classp;
        ఒకవేళ (AstClassPackage* స్థిరరాశి packagep = VN_CAST(nodep, ClassPackage)) {
            తిరిగి packagep->classp();
        }
        తిరిగి కనుగొను_Cached(nodep);
    }
};

//######################################################################

గణన Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& క్రియాకారకం<<(std::ostream& str, స్థిరరాశి Stage& rhs) {
    తిరిగి str << ("-PFB"[static_cast<పూర్ణాంకం>(rhs)]);
}

గణన Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    కేటాయించు_3  // Assignment-like where sign comes from RHS only
};
std::ostream& క్రియాకారకం<<(std::ostream& str, స్థిరరాశి Determ& rhs) {
    స్థిర constexpr స్థిరరాశి అక్షరం* స్థిరరాశి s_det[] = {"SELF", "CNTX", "ASSN"};
    తిరిగి str << s_det[rhs];
}

గణన ప్రవాహం_Use : uint8_t {
    ప్రవాహం_USE_NONE,  // Stream concatenation is not allowed in this context
    ప్రవాహం_USE_కేటాయించు,  // Assignment-like context permits stream concatenation
    ప్రవాహం_USE_CAST,  // Explicit cast context permits stream concatenation
};

#నిర్వచించు v3widthWarn(lhs, rhs, సందేశం_2) \
    v3warn_సంకేతం(((lhs) < (rhs)   ? V3_Error_సంకేతం::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3_Error_సంకేతం::WIDTHEXPAND \
                                : V3_Error_సంకేతం::వెడల్పు_2), \
               సందేశం_2)

//######################################################################
// Width state, as a visitor of each AstNode

తరగతి వెడల్పు_VP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_కణుపు_DType* స్థిరరాశి m_dtypep;  // Parent's data type to resolve to
    స్థిరరాశి Stage m_stage;  // If true, report errors
    స్థిరరాశి ప్రవాహం_Use m_ప్రవాహం_Use;  // Current expression may be a stream concat

బహిరంగ:
    వెడల్పు_VP(Ast_కణుపు_DType* dtypep, Stage stage, ప్రవాహం_Use ప్రవాహం_Use_2 = ప్రవాహం_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_ప్రవాహం_Use{ప్రవాహం_Use_2} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    వెడల్పు_VP(Determ determ, Stage stage, ప్రవాహం_Use ప్రవాహం_Use_2 = ప్రవాహం_USE_NONE)
        : m_dtypep{శూన్యసూచిక}
        , m_stage{stage}
        , m_ప్రవాహం_Use{ప్రవాహం_Use_2} {
        ఒకవేళ (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    వెడల్పు_VP* p() { తిరిగి ఇది; }
    తార్కిక selfDtm() స్థిరరాశి { తిరిగి m_dtypep == శూన్యసూచిక; }
    ప్రవాహం_Use ప్రవాహం_Use_2() స్థిరరాశి { తిరిగి m_ప్రవాహం_Use; }
    Ast_కణుపు_DType* dtypep() స్థిరరాశి {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        తిరిగి m_dtypep;
    }
    Ast_కణుపు_DType* dtypeNullp() స్థిరరాశి { తిరిగి m_dtypep; }
    Ast_కణుపు_DType* dtypeNullSkipRefp() స్థిరరాశి {
        Ast_కణుపు_DType* dtp = dtypeNullp();
        ఒకవేళ (dtp) dtp = dtp->skipRefp();
        తిరిగి dtp;
    }
    Ast_కణుపు_DType* dtypeOverridep(Ast_కణుపు_DType* స్థిరరాశి defaultp) స్థిరరాశి {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        ఒకవేళ (!m_dtypep) తిరిగి defaultp;
        ఒకవేళ (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            తిరిగి defaultp->కనుగొను_బిట్టు_Or_Logic_DType(m_dtypep->వెడల్పు(), m_dtypep->వెడల్పు_Min(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        తిరిగి m_dtypep;
    }
    పూర్ణాంకం వెడల్పు() స్థిరరాశి {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        తిరిగి m_dtypep->వెడల్పు();
    }
    పూర్ణాంకం వెడల్పు_Min() స్థిరరాశి {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        తిరిగి m_dtypep->వెడల్పు_Min();
    }
    తార్కిక prelim() స్థిరరాశి { తిరిగి m_stage & PRELIM; }
    తార్కిక final() స్థిరరాశి { తిరిగి m_stage & FINAL; }
    శూన్య dump(std::ostream& str) స్థిరరాశి {
        ఒకవేళ (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } లేకపోతే {
            str << "  VUP(s=" << m_stage << ",dt=" << cvt_గమ్యం_Hex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& క్రియాకారకం<<(std::ostream& str, స్థిరరాశి వెడల్పు_VP* vup) {
    ఒకవేళ (vup) vup->dump(str);
    తిరిగి str;
}

//######################################################################

తరగతి వెడల్పు_Clear_Visitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    శూన్య clear_వెడల్పు_Recurse(Ast_కణుపు* nodep) {
        కొరకు (; nodep; nodep = nodep->nextp()) {
            nodep->did_వెడల్పు(అబద్ధం);
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op1p()) clear_వెడల్పు_Recurse(refp);
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op2p()) clear_వెడల్పు_Recurse(refp);
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op3p()) clear_వెడల్పు_Recurse(refp);
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op4p()) clear_వెడల్పు_Recurse(refp);
        }
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన వెడల్పు_Clear_Visitor(AstNetlist* nodep) { clear_వెడల్పు_Recurse(nodep); }
    మాయా ~వెడల్పు_Clear_Visitor() = అప్రమేయ;
};

//######################################################################

#నిర్వచించు accept లోపల_వెడల్పు_Visitor_use_Ast_కణుపు_iterate_instead_యొక్క_Ast_కణుపు_accept

//######################################################################

తరగతి వెడల్పు_Visitor final : బహిరంగ VNVisitor {
    // TYPES
    ఉపయోగించి PatVecMap = std::map<పూర్ణాంకం, AstPatMember*>;
    ఉపయోగించి DTypeMap = std::map<స్థిరరాశి std::string, AstPatMember*>;

    // STATE
    V3_Unique_పేర్లు m_enum_తాత్కాలిక_పేర్లు;  // Captured enum expressions
    V3_Unique_పేర్లు m_inside_తాత్కాలిక_పేర్లు;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<స్థిరరాశి AstModport*, std::unordered_సమితి<string>>
        m_modport_పేర్లు;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    వెడల్పు_VP* m_vup = శూన్యసూచిక;  // Current node state
    తార్కిక m_underFork = అబద్ధం;  // Visiting under a fork
    తార్కిక m_underSExpr = అబద్ధం;  // Visiting under a sequence expression
    తార్కిక m_underPackedArray = అబద్ధం;  // Visiting under a AstPackArrayDType
    తార్కిక m_underMemberSel = అబద్ధం;  // Viting under a MemberSel
    Ast_కణుపు* m_seqUnsupp = శూన్యసూచిక;  // Property has unsupported node
    తార్కిక m_hasSExpr = అబద్ధం;  // Property has a sequence expression
    స్థిరరాశి AstCell* m_cellp = శూన్యసూచిక;  // Current cell for arrayed instantiations
    std::సదిశ<AstVar*> m_విభజించు_Iface_Array_Varps;  // Interface arrays split by V3Param
    స్థిరరాశి Ast_Enum_అంశం* m_enumItemp = శూన్యసూచిక;  // Current enum item
    Ast_కణుపు_FTask* m_ftaskp = శూన్యసూచిక;  // Current function/task
    Ast_కణుపు_Module* m_modep = శూన్యసూచిక;  // Current module
    స్థిరరాశి AstConstraint* m_constraintp = శూన్యసూచిక;  // Current constraint
    Ast_కణుపు_Procedure* m_procedurep = శూన్యసూచిక;  // Current final/always
    స్థిరరాశి AstWith* m_withp = శూన్యసూచిక;  // Current 'with' statement
    స్థిరరాశి AstFunc* m_funcp = శూన్యసూచిక;  // Current function
    స్థిరరాశి Ast_Attr_యొక్క* m_attrp = శూన్యసూచిక;  // Current attribute
    స్థిరరాశి Ast_కణుపు_సమాసం* m_randomizeFromp = శూన్యసూచిక;  // Current randomize method call fromp
    స్థిరరాశి తార్కిక m_paramsOnly;  // Computing parameter value; limit operation
    స్థిరరాశి తార్కిక m_doGenerate;  // Do errors later inside generate statement
    తార్కిక m_ప్రవాహం_Concat = అబద్ధం;  // True if visiting arguments of stream concatenation
    తార్కిక m_లోపల_Param_Override = అబద్ధం;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<స్థిరరాశి Ast_కణుపు_DType*, VAttrType>, AstVarRef*> m_పట్టిక_Map;
    // Queues with given index type
    std::map<స్థిరరాశి Ast_కణుపు_DType*, Ast_వరుస_DType*> m_వరుస_DType_సూచీకృత;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_సమితి<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_సమితి<స్థిరరాశి AstVar*> m_curModVars;  // Variables declared in current module

    స్థిర constexpr పూర్ణాంకం ENUM_LOOKUP_బిట్లు = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    గణన Extend_నియమం : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    గణన తరగతి ComparisonType : uint8_t {
        సాధారణ_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        సాధారణ_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    స్థిర ప్రవాహం_Use ప్రవాహం_Use_For_Determ(Determ determ) {
        తిరిగి determ == కేటాయించు_3 ? ప్రవాహం_USE_కేటాయించు : ప్రవాహం_USE_NONE;
    }

    ప్రవాహం_Use current_ప్రవాహం_Use() స్థిరరాశి { తిరిగి m_vup ? m_vup->ప్రవాహం_Use_2() : ప్రవాహం_USE_NONE; }

    స్థిర శూన్య packIfUnpacked(Ast_కణుపు_సమాసం* స్థిరరాశి nodep) {
        ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            స్థిరరాశి పూర్ణాంకం elements_అంకె = unpackDTypep->arrayUnpackedElements();
            స్థిరరాశి పూర్ణాంకం unpack_Min_బిట్లు = elements_అంకె * unpackDTypep->subDTypep()->వెడల్పు_Min();
            స్థిరరాశి పూర్ణాంకం unpack_బిట్లు = elements_అంకె * unpackDTypep->subDTypep()->వెడల్పు();
            VNRelinker relinker;
            nodep->unlink_Fr_వెనుక(&relinker);
            relinker.relink(కొత్త Ast_Cvt_Array_గమ్యం_Packed{
                nodep->fileline(), nodep,
                nodep->కనుగొను_బిట్టు_Or_Logic_DType(unpack_బిట్లు, unpack_Min_బిట్లు, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    స్థిర Ast_కణుపు_DType* fromDTypep(Ast_కణుపు* fromp) {
        ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtypep = VN_CAST(fromp, కణుపు_DType))
            తిరిగి dtypep->skipRefOrNullp();
        తిరిగి fromp ? fromp->dtypep() : శూన్యసూచిక;
    }
    // VISITORS
    //   Naming:  width_O{outputtype}_L{lhstype}_R{rhstype}_W{widthing}_S{signing}
    //          Where type:
    //                  _O1=boolean (width 1 unsigned)
    //                  _Ou=unsigned
    //                  _Os=signed
    //                  _Ous=unsigned or signed
    //                  _Or=real
    //                  _Ox=anything

    // Widths: 1 bit out, lhs 1 bit; Real: converts via compare with 0
    శూన్య సందర్శించు(AstLogNot* nodep) override { సందర్శించు_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    శూన్య సందర్శించు(AstLogAnd* nodep) override { సందర్శించు_log_and_or(nodep); }
    శూన్య సందర్శించు(AstLogOr* nodep) override { సందర్శించు_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    శూన్య సందర్శించు(AstSAnd* nodep) override { సందర్శించు_log_and_or(nodep); }
    శూన్య సందర్శించు(AstSIntersect* nodep) override { సందర్శించు_log_and_or(nodep); }
    శూన్య సందర్శించు(AstSOr* nodep) override { సందర్శించు_log_and_or(nodep); }
    శూన్య సందర్శించు(AstSWithin* nodep) override { సందర్శించు_log_and_or(nodep); }
    శూన్య సందర్శించు(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        సందర్శించు_log_and_or(nodep);
    }
    శూన్య సందర్శించు(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        సందర్శించు_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    శూన్య సందర్శించు(AstRedAnd* nodep) override { సందర్శించు_red_and_or(nodep); }
    శూన్య సందర్శించు(AstRedOr* nodep) override { సందర్శించు_red_and_or(nodep); }
    శూన్య సందర్శించు(AstRedXor* nodep) override { సందర్శించు_red_and_or(nodep); }
    శూన్య సందర్శించు(AstOneHot* nodep) override { సందర్శించు_red_and_or(nodep); }
    శూన్య సందర్శించు(AstOneHot0* nodep) override { సందర్శించు_red_and_or(nodep); }
    శూన్య సందర్శించు(AstIsUnknown* nodep) override {
        సందర్శించు_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    శూన్య సందర్శించు(AstEq* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_EQ); }
    శూన్య సందర్శించు(AstNeq* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_EQ); }
    శూన్య సందర్శించు(AstGt* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_GL); }
    శూన్య సందర్శించు(AstGte* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_GL); }
    శూన్య సందర్శించు(AstLt* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_GL); }
    శూన్య సందర్శించు(AstLte* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_GL); }
    శూన్య సందర్శించు(AstGtS* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_GL); }
    శూన్య సందర్శించు(AstGteS* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_GL); }
    శూన్య సందర్శించు(AstLtS* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_GL); }
    శూన్య సందర్శించు(AstLteS* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::సాధారణ_GL); }
    శూన్య సందర్శించు(AstEqCase* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::CASE); }
    శూన్య సందర్శించు(AstNeqCase* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    శూన్య సందర్శించు(AstEqWild* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::WILD); }
    శూన్య సందర్శించు(AstNeqWild* nodep) override { సందర్శించు_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    శూన్య సందర్శించు(AstEqD* nodep) override { సందర్శించు_cmp_real(nodep); }
    శూన్య సందర్శించు(AstNeqD* nodep) override { సందర్శించు_cmp_real(nodep); }
    శూన్య సందర్శించు(AstLtD* nodep) override { సందర్శించు_cmp_real(nodep); }
    శూన్య సందర్శించు(AstLteD* nodep) override { సందర్శించు_cmp_real(nodep); }
    శూన్య సందర్శించు(AstGtD* nodep) override { సందర్శించు_cmp_real(nodep); }
    శూన్య సందర్శించు(AstGteD* nodep) override { సందర్శించు_cmp_real(nodep); }
    // ...    String compares
    శూన్య సందర్శించు(AstEqN* nodep) override { సందర్శించు_cmp_string(nodep); }
    శూన్య సందర్శించు(AstNeqN* nodep) override { సందర్శించు_cmp_string(nodep); }
    శూన్య సందర్శించు(AstLtN* nodep) override { సందర్శించు_cmp_string(nodep); }
    శూన్య సందర్శించు(AstLteN* nodep) override { సందర్శించు_cmp_string(nodep); }
    శూన్య సందర్శించు(AstGtN* nodep) override { సందర్శించు_cmp_string(nodep); }
    శూన్య సందర్శించు(AstGteN* nodep) override { సందర్శించు_cmp_string(nodep); }
    // ...    Data type compares
    శూన్య సందర్శించు(AstEqT* nodep) override { సందర్శించు_cmp_type(nodep); }
    శూన్య సందర్శించు(AstNeqT* nodep) override { సందర్శించు_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    శూన్య సందర్శించు(AstAnd* nodep) override { సందర్శించు_boolexpr_and_or(nodep); }
    శూన్య సందర్శించు(AstOr* nodep) override { సందర్శించు_boolexpr_and_or(nodep); }
    శూన్య సందర్శించు(AstXor* nodep) override { సందర్శించు_boolexpr_and_or(nodep); }
    శూన్య సందర్శించు(AstBufIf1* nodep) override {
        సందర్శించు_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    శూన్య సందర్శించు(Ast_కూడు* nodep) override { సందర్శించు_కూడు_sub_replace(nodep, నిజం, అబద్ధం); }
    శూన్య సందర్శించు(AstSub* nodep) override { సందర్శించు_కూడు_sub_replace(nodep, నిజం, అబద్ధం); }
    శూన్య సందర్శించు(AstDiv* nodep) override { సందర్శించు_కూడు_sub_replace(nodep, నిజం, నిజం); }
    శూన్య సందర్శించు(AstMul* nodep) override { సందర్శించు_కూడు_sub_replace(nodep, నిజం, అబద్ధం); }
    // These can't promote to real
    శూన్య సందర్శించు(AstModDiv* nodep) override { సందర్శించు_కూడు_sub_replace(nodep, అబద్ధం, నిజం); }
    శూన్య సందర్శించు(AstModDivS* nodep) override { సందర్శించు_కూడు_sub_replace(nodep, అబద్ధం, నిజం); }
    శూన్య సందర్శించు(AstMulS* nodep) override { సందర్శించు_కూడు_sub_replace(nodep, అబద్ధం, అబద్ధం); }
    శూన్య సందర్శించు(AstDivS* nodep) override { సందర్శించు_కూడు_sub_replace(nodep, అబద్ధం, నిజం); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    శూన్య సందర్శించు(AstNegate* nodep) override { సందర్శించు_negate_not(nodep, నిజం); }
    // Unary never real
    శూన్య సందర్శించు(AstNot* nodep) override { సందర్శించు_negate_not(nodep, అబద్ధం); }

    // Real: inputs and output real
    శూన్య సందర్శించు(Ast_కూడు_D* nodep) override { సందర్శించు_real_కూడు_sub(nodep); }
    శూన్య సందర్శించు(AstSubD* nodep) override { సందర్శించు_real_కూడు_sub(nodep); }
    శూన్య సందర్శించు(AstDivD* nodep) override { సందర్శించు_real_కూడు_sub(nodep); }
    శూన్య సందర్శించు(AstMulD* nodep) override { సందర్శించు_real_కూడు_sub(nodep); }
    శూన్య సందర్శించు(AstPowD* nodep) override { సందర్శించు_real_కూడు_sub(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_System_Biop_D* nodep) override { సందర్శించు_real_కూడు_sub(nodep); }
    // Real: Output real
    శూన్య సందర్శించు(AstNegateD* nodep) override { సందర్శించు_real_neg_ceil(nodep); }
    శూన్య సందర్శించు(Ast_కణుపు_System_Uniop_D* nodep) override { సందర్శించు_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    శూన్య సందర్శించు(AstSigned* nodep) override { సందర్శించు_signed_unsigned(nodep, VSigning::SIGNED); }
    శూన్య సందర్శించు(AstUnsigned* nodep) override { సందర్శించు_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    శూన్య సందర్శించు(Ast_జరుపు_L* nodep) override { సందర్శించు_జరుపు_2(nodep); }
    శూన్య సందర్శించు(Ast_జరుపు_R* nodep) override { సందర్శించు_జరుపు_2(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    శూన్య సందర్శించు(Ast_జరుపు_RS* nodep) override { సందర్శించు_జరుపు_2(nodep); }

    //========
    // Widths: Output real, input integer signed
    శూన్య సందర్శించు(Ast_బిట్లు_గమ్యం_Real_D* nodep) override { సందర్శించు_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    శూన్య సందర్శించు(AstRToIS* nodep) override { సందర్శించు_Os32_Lr(nodep); }
    శూన్య సందర్శించు(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->వెడల్పు_Sized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    శూన్య సందర్శించు(Ast_Real_గమ్యం_బిట్లు* nodep) override { సందర్శించు_Ou64_Lr(nodep); }

    // Output integer, input string
    శూన్య సందర్శించు(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        ఒకవేళ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_సమితి_Int();
        }
    }
    శూన్య సందర్శించు(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ఒకవేళ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_సమితి_String();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    శూన్య సందర్శించు(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ఒకవేళ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_బిట్టు_Sized(8, VSigning::UNSIGNED);
        }
    }
    శూన్య సందర్శించు(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ఒకవేళ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_బిట్టు_Sized(8, VSigning::UNSIGNED);
        }
    }
    శూన్య సందర్శించు(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ఒకవేళ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_సమితి_String();
        }
    }
    శూన్య సందర్శించు(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_Int();
        }
    }
    శూన్య సందర్శించు(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            ఒకవేళ (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtype_సమితి_Double();
            } లేకపోతే {
                nodep->dtype_సమితి_Integer();
            }
        }
    }
    శూన్య సందర్శించు(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    శూన్య సందర్శించు(Ast_సమయం* nodep) override { nodep->dtype_సమితి_Time2_State(); }
    శూన్య సందర్శించు(Ast_సమయం_D* nodep) override { nodep->dtype_సమితి_Double(); }
    శూన్య సందర్శించు(Ast_సమయం_Precision* nodep) override { nodep->dtype_సమితి_Integer2_State(); }
    శూన్య సందర్శించు(Ast_Get_Initial_Random_విత్తనం* nodep) override { nodep->dtype_సమితి_Int(); }
    శూన్య సందర్శించు(Ast_సమయం_Unit* nodep) override {
        nodep->replaceWith(
            కొత్త AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().power_యొక్క_Ten()});
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstScopeName* nodep) override {
        nodep->dtype_సమితి_UInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    స్థిర తార్కిక is_Override_నమూనా(స్థిరరాశి Ast_కణుపు* nodep) {
        ఒకవేళ (స్థిరరాశి Ast_నమూనా* స్థిరరాశి patternp = VN_CAST(nodep, నమూనా)) {
            తిరిగి !patternp->బిడ్డ_DTypep();
        }
        స్థిరరాశి AstCond* స్థిరరాశి condp = VN_CAST(nodep, Cond);
        తిరిగి condp && is_Override_నమూనా(condp->thenp()) && is_Override_నమూనా(condp->elsep());
    }
    శూన్య సందర్శించు(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_లోపల_Param_Override) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            స్థిరరాశి పూర్ణాంకం errors = V3Error::error_లెక్క();
            {
                VL_RESTORER(m_లోపల_Param_Override);
                m_లోపల_Param_Override = అబద్ధం;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->condp(), Const);
            స్థిరరాశి తార్కిక తెలిసిన = constp && !constp->అంకె().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            ఒకవేళ (!తెలిసిన && V3Error::error_లెక్క() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_కణుపు_సమాసం* స్థిరరాశి keepp
                = (తెలిసిన && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlink_Fr_వెనుక();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            userIterate(keepp, వెడల్పు_VP{SELF, BOTH}.p());
            తిరిగి;
        }
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_తదుపరి(nodep->thenp(), వెడల్పు_VP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_తదుపరి(nodep->elsep(), వెడల్పు_VP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_కణుపు_DType* స్థిరరాశి thenDTypep = nodep->thenp()->dtypep();
            Ast_కణుపు_DType* స్థిరరాశి elseDTypep = nodep->elsep()->dtypep();
            Ast_కణుపు_DType* స్థిరరాశి thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_కణుపు_DType* స్థిరరాశి elseDTypeSkippedp = elseDTypep->skipRefp();
            ఒకవేళ (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->కనుగొను_Null_DType());
            } లేకపోతే ఒకవేళ (nodep->thenp()->is_Class_Handle_విలువ()
                       || nodep->elsep()->is_Class_Handle_విలువ()) {
                Ast_కణుపు_DType* commonClassTypep = శూన్యసూచిక;
                ఒకవేళ (nodep->thenp()->is_Class_Handle_విలువ() && nodep->elsep()->is_Class_Handle_విలువ()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_కణుపు::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                ఒకవేళ (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } లేకపోతే {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } లేకపోతే ఒకవేళ (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                ఒకవేళ (Ast_కణుపు::computeCastable(elseDTypep, thenDTypep, శూన్యసూచిక).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } లేకపోతే {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    ఒకవేళ (!Ast_కణుపు::computeCastable(thenDTypep, elseDTypep, శూన్యసూచిక)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } లేకపోతే ఒకవేళ (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtype_సమితి_Double();
            } లేకపోతే ఒకవేళ (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtype_సమితి_String();
            } లేకపోతే {
                స్థిరరాశి తార్కిక is_Any_ఫలితం_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                తార్కిక handled = అబద్ధం;
                స్థిరరాశి తార్కిక isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_కణుపు_DType* స్థిరరాశి thenEnumDTypep = thenDTypep->skip_Ref_గమ్యం_Enump();
                ఒకవేళ (VN_IS(thenDTypeSkippedp, కణుపు_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_వృక్షం(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = నిజం;
                } లేకపోతే ఒకవేళ (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_వృక్షం(elseDTypep->skip_Ref_గమ్యం_Enump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    ఒకవేళ (is_Any_ఫలితం_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = నిజం;
                    } లేకపోతే {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(కొత్త File_పంక్తి{nodep->fileline()});
                        nodep->fileline()->warnOff(V3_Error_సంకేతం::ENUMVALUE, నిజం);
                    }
                }
                ఒకవేళ (!handled) {
                    స్థిరరాశి పూర్ణాంకం వెడల్పు = std::max(thenDTypep->వెడల్పు(), elseDTypep->వెడల్పు());
                    స్థిరరాశి పూర్ణాంకం వెడల్పు_Min = std::max(thenDTypep->వెడల్పు_Min(), elseDTypep->వెడల్పు_Min());
                    స్థిరరాశి VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    స్థిరరాశి తార్కిక isFourstate = is_Any_ఫలితం_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    ఒకవేళ ((వెడల్పు == elseDTypeSkippedp->వెడల్పు()
                         && వెడల్పు_Min == elseDTypeSkippedp->వెడల్పు_Min())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } లేకపోతే ఒకవేళ ((వెడల్పు == thenDTypeSkippedp->వెడల్పు()
                                && వెడల్పు_Min == thenDTypeSkippedp->వెడల్పు_Min())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } లేకపోతే {
                        // None of the children dtypes match generate one
                        nodep->dtype_సమితి_బిట్టు_Or_Logic_Unsized(వెడల్పు, వెడల్పు_Min, numeric, isFourstate);
                    }
                }
            }
        }
        ఒకవేళ (m_vup->final()) {
            Ast_కణుపు_DType* స్థిరరాశి expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_కణుపు_DType* స్థిరరాశి subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    శూన్య సందర్శించు(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_కణుపు_DType* స్థిరరాశి vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, వెడల్పు_VP{SELF, BOTH}.p());
        // Conversions
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి newp = new_Cons_Dyn_Array_Or_వరుస(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{vdtypep, PRELIM}.p());
            user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            కూడు_Cons_అంశం(newp, nodep->lhsp()->unlink_Fr_వెనుక());
            కూడు_Cons_అంశం(newp, nodep->rhsp()->unlink_Fr_వెనుక());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        ఒకవేళ (VN_IS(vdtypep, UnpackArrayDType)) {
            Ast_నమూనా* స్థిరరాశి newp = కొత్త Ast_నమూనా{nodep->fileline(), శూన్యసూచిక};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            తిరిగి;
        }

        // Concat handling
        ఒకవేళ (m_vup->prelim()) {
            ఒకవేళ (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, వరుస_DType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            ఒకవేళ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtype_సమితి_String();
            } లేకపోతే {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                ఒకవేళ (m_ప్రవాహం_Concat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtype_సమితి_బిట్టు_Or_Logic_Unsized(
                    nodep->lhsp()->వెడల్పు() + nodep->rhsp()->వెడల్పు(),
                    nodep->lhsp()->వెడల్పు_Min() + nodep->rhsp()->వెడల్పు_Min(), VSigning::UNSIGNED,
                    Ast_కణుపు_సమాసం::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            ఒకవేళ (స్థిరరాశి AstReplicate* స్థిరరాశి repp = VN_CAST(nodep->lhsp(), Replicate)) {
                ఒకవేళ (repp->వెడల్పు() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlink_Fr_వెనుక());
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
            }
            ఒకవేళ (స్థిరరాశి AstReplicate* స్థిరరాశి repp = VN_CAST(nodep->rhsp(), Replicate)) {
                ఒకవేళ (repp->వెడల్పు() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlink_Fr_వెనుక());
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
            }
        }
        ఒకవేళ (m_vup->final()) {
            ఒకవేళ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_కణుపు_సమాసం* lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
                ఒకవేళ (!lhsp->isString()) lhsp = కొత్త AstCvtPackString{lhsp->fileline(), lhsp};
                ఒకవేళ (!rhsp->isString()) rhsp = కొత్త AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_కణుపు* స్థిరరాశి newp = కొత్త AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }
            ఒకవేళ (!nodep->dtypep()->వెడల్పు_Sized()) {
                // See also error in V3Number
                కణుపు_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    శూన్య సందర్శించు(AstConcatN* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_String();
        }
        ఒకవేళ (m_vup->final()) {
            nodep->did_వెడల్పు(నిజం);
            ఒకవేళ (!nodep->dtypep()->వెడల్పు_Sized()) {
                // See also error in V3Number
                కణుపు_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    శూన్య సందర్శించు(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    శూన్య సందర్శించు(AstDelay* nodep) override {
        ఒకవేళ (nodep->is_చక్రం_Delay() && m_underSExpr) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            ఒకవేళ (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            ఒకవేళ (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                తిరిగి;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::check_Sva_చక్రం_Delay(nodep, nodep);
            తిరిగి;
        }
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            తిరిగి;
        }
        ఒకవేళ (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        ఒకవేళ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        ఒకవేళ (nodep->fileline()->timingOn()) {
            ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_తదుపరి_Null(nodep->stmtsp());
                తిరిగి;
            } లేకపోతే ఒకవేళ (v3Global.opt.timing().is_సమితి_False()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } లేకపోతే {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        ఒకవేళ (nodep->stmtsp()) nodep->కూడు_తదుపరి_ఇక్కడ(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        ఒకవేళ (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        ఒకవేళ (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().is_సమితి_True()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* స్థిరరాశి newp = కొత్త AstBegin{nodep->fileline(), nodep->name(), శూన్యసూచిక, అబద్ధం};
            nodep->replaceWith(newp);
            ఒకవేళ (nodep->declsp()) newp->కూడు_Declsp(nodep->declsp()->unlink_Fr_వెనుక_With_తదుపరి());
            ఒకవేళ (nodep->stmtsp()) newp->కూడు_Stmtsp(nodep->stmtsp()->unlink_Fr_వెనుక_With_తదుపరి());
            ఒకవేళ (nodep->forksp()) newp->కూడు_Stmtsp(nodep->forksp()->unlink_Fr_వెనుక_With_తదుపరి());
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
            VL_RESTORER(m_underFork);
            m_underFork = నిజం;
            iterateChildren(nodep);
        } లేకపోతే ఒకవేళ (v3Global.opt.timing().is_సమితి_False()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
        } లేకపోతే {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    శూన్య సందర్శించు_Wait_Or_Disable_Fork(Ast_కణుపు* nodep) {
        ఒకవేళ (nodep->fileline()->timingOn()) {
            ఒకవేళ (v3Global.opt.timing().is_సమితి_False()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
                తిరిగి;
            } లేకపోతే ఒకవేళ (!v3Global.opt.timing().is_సమితి_True()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(AstDisableFork* nodep) override { సందర్శించు_Wait_Or_Disable_Fork(nodep); }
    శూన్య సందర్శించు(AstWaitFork* nodep) override { సందర్శించు_Wait_Or_Disable_Fork(nodep); }
    శూన్య సందర్శించు(Ast_గమ్యం_క్రింది_N* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_సమితి_String();
        }
    }
    శూన్య సందర్శించు(Ast_గమ్యం_పై_N* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_సమితి_String();
        }
    }
    శూన్య సందర్శించు(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->countp(), Const);
            ఒకవేళ (constp) {
                ఒకవేళ (constp->అంకె().isFourState()
                    || (constp->dtypep()->isSigned() && constp->అంకె().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } లేకపోతే {
                    times = constp->గమ్యం_SInt();
                }
            }

            Ast_కణుపు_DType* స్థిరరాశి vdtypep = m_vup->dtypeNullSkipRefp();
            ఒకవేళ (VN_IS(vdtypep, వరుస_DType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                ఒకవేళ (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                ఒకవేళ (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    user_Iterate_And_తదుపరి(nodep->srcp(), వెడల్పు_VP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlink_Fr_వెనుక());
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                } లేకపోతే {  // int a[] = {lhs} -> same as '{lhs}
                    Ast_నమూనా* స్థిరరాశి newp = కొత్త Ast_నమూనా{
                        nodep->fileline(),
                        కొత్త AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlink_Fr_వెనుక(),
                                         శూన్యసూచిక, శూన్యసూచిక}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    తిరిగి;
                }
            }
            ఒకవేళ (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            ఒకవేళ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } లేకపోతే {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            ఒకవేళ ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_కణుపు* స్థిరరాశి newp
                    = కొత్త AstReplicateN{nodep->fileline(), nodep->srcp()->unlink_Fr_వెనుక(),
                                        nodep->countp()->unlink_Fr_వెనుక()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            } లేకపోతే {
                ఒకవేళ (!constp) nodep->v3error("Replication value isn't a constant.");
                ఒకవేళ (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtype_సమితి_బిట్టు_Or_Logic_Unsized(
                    (nodep->srcp()->వెడల్పు() * times), (nodep->srcp()->వెడల్పు_Min() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        ఒకవేళ (m_vup->final()) {
            ఒకవేళ (!nodep->dtypep()->వెడల్పు_Sized()) {
                // See also error in V3Number
                కణుపు_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    శూన్య సందర్శించు(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtype_సమితి_String();
        }
        ఒకవేళ (m_vup->final()) {
            ఒకవేళ (!nodep->dtypep()->వెడల్పు_Sized()) {
                // See also error in V3Number
                కణుపు_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Dist_Biop* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_Dist_Triop* nodep) override {
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    తార్కిక ప్రవాహం_Implicit_Use_Allowed(స్థిరరాశి Ast_కణుపు_ప్రవాహం* nodep) స్థిరరాశి {
        ఒకవేళ (m_ప్రవాహం_Concat) తిరిగి నిజం;
        ఒకవేళ (m_vup && m_vup->ప్రవాహం_Use_2() != ప్రవాహం_USE_NONE) తిరిగి నిజం;

        స్థిరరాశి Ast_కణుపు* స్థిరరాశి backp = nodep->backp();
        ఒకవేళ (!backp) తిరిగి నిజం;  // Error elsewhere

        తిరిగి VN_IS(backp, కణుపు_కేటాయించు);
    }
    శూన్య సందర్శించు(Ast_కణుపు_ప్రవాహం* nodep) override {
        VL_RESTORER(m_ప్రవాహం_Concat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            m_ప్రవాహం_Concat = నిజం;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_ప్రవాహం_Concat = అబద్ధం;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->rhsp(), Const)) {
                ఒకవేళ (constp->గమ్యం_UInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } లేకపోతే {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి lhsDtypep = nodep->lhsp()->dtypep()->skip_Ref_గమ్యం_Enump();
            ఒకవేళ (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, వరుస_DType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, కణుపు_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, కణుపు_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtype_సమితి_ప్రవాహం();
            } లేకపోతే ఒకవేళ (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtype_సమితి_ప్రవాహం();
            } లేకపోతే {
                nodep->dtype_సమితి_బిట్టు_Or_Logic_Unsized(
                    nodep->lhsp()->వెడల్పు(), nodep->lhsp()->వెడల్పు_Min(), VSigning::UNSIGNED,
                    Ast_కణుపు_సమాసం::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ఒకవేళ (m_vup->final()) {
            ఒకవేళ (!ప్రవాహం_Implicit_Use_Allowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warn_మరిన్ని() << "... Suggest use a cast");
            }
            ఒకవేళ (!nodep->dtypep()->వెడల్పు_Sized()) {
                // See also error in V3Number
                కణుపు_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    శూన్య సందర్శించు(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), నిజం,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), నిజం,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        ఒకవేళ (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->elementsConst();
            ఒకవేళ (nodep->fromBracket() && nodep->ఎడమ_Const() > nodep->కుడి_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-వెడల్పు + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } లేకపోతే ఒకవేళ (వెడల్పు > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << వెడల్పు << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            స్థిరరాశి తార్కిక లోపల_Dead_Module = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            స్థిరరాశి తార్కిక లోపల_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            స్థిరరాశి తార్కిక లోపల_Type_పట్టిక = !m_modep;
            స్థిరరాశి Ast_కణుపు* basep = nodep->backp();
            అయితే (VN_IS(basep, Range)) basep = basep->backp();
            ఒకవేళ (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !లోపల_Dead_Module && !లోపల_Parameterized_Template && !లోపల_Type_పట్టిక) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->ఎడమ_Const() << ":" << nodep->కుడి_Const()
                                            << "]");
            }
        }
    }

    శూన్య సందర్శించు(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_తదుపరి(nodep->lsbp(), వెడల్పు_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), అబద్ధం);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            పూర్ణాంకం వెడల్పు = nodep->వెడల్పు_Const();
            ఒకవేళ (వెడల్పు <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtype_సమితి_బిట్టు();
                తిరిగి;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            స్థిరరాశి తార్కిక isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            స్థిరరాశి తార్కిక లోపల_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            ఒకవేళ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                వెడల్పు = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtype_సమితి_బిట్టు_Or_Logic_Sized(వెడల్పు, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->వెడల్పు_Const(వెడల్పు);
                నెట్టు_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(కొత్త AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            ఒకవేళ (nodep->fromp()->వెడల్పు() < వెడల్పు && !లోపల_Parameterized_Template) {
                // If it is not a lvalue, extend it
                ఒకవేళ (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << వెడల్పు << " bits from only "
                                                          << nodep->fromp()->వెడల్పు()
                                                          << " bit number");
                    Ast_కణుపు_DType* స్థిరరాశి subDTypep = nodep->కనుగొను_బిట్టు_Or_Logic_DType(
                        వెడల్పు, వెడల్పు, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    వెడల్పు_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    అబద్ధం /*noerror*/);
                } లేకపోతే {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << వెడల్పు << " bits to only "
                                                         << nodep->fromp()->వెడల్పు()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            పూర్ణాంకం frommsb = nodep->fromp()->వెడల్పు() - 1;
            పూర్ణాంకం fromlsb = 0;
            స్థిరరాశి పూర్ణాంకం elw = nodep->decl_El_వెడల్పు();  // Must adjust to tell user bit ranges
            ఒకవేళ (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } లేకపోతే {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            స్థిరరాశి పూర్ణాంకం selwidth = V3_సంఖ్య::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_కణుపు_DType* స్థిరరాశి selwidthDTypep = nodep->కనుగొను_బిట్టు_Or_Logic_DType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, FINAL}.p());
            user_Iterate_And_తదుపరి(nodep->lsbp(), వెడల్పు_VP{SELF, FINAL}.p());
            ఒకవేళ (వెడల్పు_Bad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->వెడల్పు() != 32) {
                ఒకవేళ (!nodep->fileline()->warnIsOff(V3_Error_సంకేతం::వెడల్పు_2)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->వెడల్పు() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->వెడల్పు() / elw)
                            << (nodep->lsbp()->వెడల్పు() != nodep->lsbp()->వెడల్పు_Min()
                                    ? " or " + cvt_గమ్యం_Str(nodep->lsbp()->వెడల్పు_Min() / elw)
                                    : "")
                            << " bits.");
                    UINFO(1, "    Related node: " << nodep);
                }
            }
            ఒకవేళ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                ఒకవేళ (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } లేకపోతే ఒకవేళ (!లోపల_Parameterized_Template) {
                    ఒకవేళ (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } లేకపోతే {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                ఒకవేళ (!isWriteSelect) {
                    // Extend it
                    స్థిరరాశి పూర్ణాంకం extend_గమ్యం = nodep->msbConst() + 1;
                    Ast_కణుపు_DType* స్థిరరాశి subDTypep = nodep->కనుగొను_బిట్టు_Or_Logic_DType(
                        extend_గమ్యం, extend_గమ్యం, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    వెడల్పు_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    అబద్ధం /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            ఒకవేళ (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                వెడల్పు_Check_Sized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                అబద్ధం /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    శూన్య సందర్శించు(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            ఒకవేళ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->bitp(), Const)) {
                    స్థిరరాశి పూర్ణాంకం బిట్టు_2 = constp->గమ్యం_SInt();
                    Ast_కణుపు_సమాసం* newp = newIfaceArrayElement(nodep->fromp(), బిట్టు_2);
                    ఒకవేళ (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << బిట్టు_2 + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
            }
            //
            పూర్ణాంకం frommsb;
            పూర్ణాంకం fromlsb;
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                ఒకవేళ (fromlsb > frommsb) std::మార్పిడి(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                ఒకవేళ (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } లేకపోతే {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            స్థిరరాశి పూర్ణాంకం selwidth = V3_సంఖ్య::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_కణుపు_DType* స్థిరరాశి selwidthDTypep = nodep->కనుగొను_బిట్టు_Or_Logic_DType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            ఒకవేళ (వెడల్పు_Bad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->వెడల్పు() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->వెడల్పు(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->వెడల్పు()
                                       << (nodep->bitp()->వెడల్పు() != nodep->bitp()->వెడల్పు_Min()
                                               ? " or " + cvt_గమ్యం_Str(nodep->bitp()->వెడల్పు_Min())
                                               : "")
                                       << " bits.");
                ఒకవేళ (!nodep->fileline()->warnIsOff(V3_Error_సంకేతం::వెడల్పు_2)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            ఒకవేళ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ఒకవేళ (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->గమ్యం_SInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->గమ్యం_SInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    ఒకవేళ (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->గమ్యం_SInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                వెడల్పు_Check_Sized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                అబద్ధం /*NOWARN*/);
            }
        }
    }

    శూన్య సందర్శించు(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skipRefp();
            స్థిరరాశి AstAssocArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, AssocArrayDType);
            ఒకవేళ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->కీలకం_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    శూన్య సందర్శించు(AstAlias* nodep) override {
        ఒకవేళ (!nodep->did_వెడల్పు_And_సమితి()) {
            user_Iterate_And_తదుపరి(nodep->itemsp(), వెడల్పు_VP{SELF, BOTH}.p());
        }

        స్థిరరాశి స్వయంచాలక check_If_సమాసం_Ok = [ఇది](స్థిరరాశి Ast_కణుపు_సమాసం* స్థిరరాశి exprp) {
            ఒకవేళ (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                తిరిగి అబద్ధం;
            }

            స్థిరరాశి AstVarRef* స్థిరరాశి varRefp = VN_CAST(exprp, VarRef);
            ఒకవేళ (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                తిరిగి అబద్ధం;
            }
            AstVar* స్థిరరాశి varp = varRefp->varp();
            ఒకవేళ (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                తిరిగి అబద్ధం;
            }
            ఒకవేళ (m_aliasedVars.కనుగొను(varp) != m_aliasedVars.చివర()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                తిరిగి అబద్ధం;
            } లేకపోతే {
                m_aliasedVars.చొప్పించు(varp);
            }
            తిరిగి నిజం;
        };

        check_If_సమాసం_Ok(nodep->itemsp());
        Ast_కణుపు_DType* స్థిరరాశి మొదటి_అంశం_Dtypep = nodep->itemsp()->dtypep();
        కొరకు (Ast_కణుపు* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_సమాసం_Ok(VN_AS(itemp, కణుపు_సమాసం));
            ఒకవేళ (!మొదటి_అంశం_Dtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << మొదటి_అంశం_Dtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    శూన్య సందర్శించు(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skipRefp();
            స్థిరరాశి AstWildcardArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            ఒకవేళ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            ఒకవేళ (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->కనుగొను_String_DType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    శూన్య సందర్శించు(Ast_ముక్క_Sel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skipRefp();
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            ఒకవేళ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_కణుపు_DType* స్థిరరాశి newDtp
                = కొత్త AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          కొత్త AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(newDtp);
            nodep->dtypeFrom(newDtp);

            ఒకవేళ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                స్థిరరాశి పూర్ణాంకం subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                స్థిరరాశి VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                ఒకవేళ ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(కొత్త AstConsArray{
                        nodep->fileline(), newDtp,
                        కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}}});
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                } లేకపోతే ఒకవేళ ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            ఒకవేళ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* స్థిరరాశి fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* స్థిరరాశి newp = కొత్త AstConsArray{nodep->fileline(), newDtp};
                స్థిరరాశి VNumRange range = nodep->declRange();
                కొరకు (పూర్ణాంకం n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_వృక్షం(అబద్ధం));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        }
    }

    శూన్య సందర్శించు(Ast_Sel_బిట్టు* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->bitp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->thsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->attrp(), వెడల్పు_VP{SELF, BOTH}.p());
        Ast_కణుపు* స్థిరరాశి selp = V3_వెడల్పు::వెడల్పు_Sel_No_Iter_Edit(nodep);
        ఒకవేళ (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            తిరిగి;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    శూన్య సందర్శించు(Ast_Sel_వెలికితీయి* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->leftp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->rightp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->attrp(), వెడల్పు_VP{SELF, BOTH}.p());
        Ast_కణుపు* స్థిరరాశి selp = V3_వెడల్పు::వెడల్పు_Sel_No_Iter_Edit(nodep);
        ఒకవేళ (selp != nodep) {
            nodep = శూన్యసూచిక;
            userIterate(selp, m_vup);
            తిరిగి;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    శూన్య సందర్శించు(AstSelPlus* nodep) override {
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->bitp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->widthp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->attrp(), వెడల్పు_VP{SELF, BOTH}.p());
        Ast_కణుపు* స్థిరరాశి selp = V3_వెడల్పు::వెడల్పు_Sel_No_Iter_Edit(nodep);
        ఒకవేళ (selp != nodep) {
            nodep = శూన్యసూచిక;
            userIterate(selp, m_vup);
            తిరిగి;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    శూన్య సందర్శించు(AstSelMinus* nodep) override {
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->bitp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->widthp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_తదుపరి(nodep->attrp(), వెడల్పు_VP{SELF, BOTH}.p());
        Ast_కణుపు* స్థిరరాశి selp = V3_వెడల్పు::వెడల్పు_Sel_No_Iter_Edit(nodep);
        ఒకవేళ (selp != nodep) {
            nodep = శూన్యసూచిక;
            userIterate(selp, m_vup);
            తిరిగి;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    శూన్య సందర్శించు(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        ఒకవేళ (m_vup && m_vup->prelim()) {
            ఒకవేళ (VN_IS(nodep->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } లేకపోతే ఒకవేళ (nodep->అంకె().isNull()) {
                nodep->dtypep(nodep->కనుగొను_Null_DType());
            } లేకపోతే ఒకవేళ (nodep->అంకె().isString()) {
                nodep->dtype_సమితి_String();
            } లేకపోతే ఒకవేళ (nodep->అంకె().isDouble()) {
                nodep->dtype_Chg_వెడల్పు(nodep->అంకె().వెడల్పు(), nodep->అంకె().sized()
                                                               ? nodep->అంకె().వెడల్పు()
                                                               : nodep->అంకె().వెడల్పు_గమ్యం_Fit());
            } లేకపోతే {
                nodep->dtype_సమితి_బిట్టు_Or_Logic_Unsized(
                    nodep->అంకె().వెడల్పు(),
                    nodep->అంకె().sized() ? nodep->అంకె().వెడల్పు() : nodep->అంకె().వెడల్పు_గమ్యం_Fit(),
                    nodep->dtypep()->numeric(), nodep->అంకె().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    శూన్య సందర్శించు(Ast_ఖాళీలు_వరుస* nodep) override {
        nodep->dtype_సమితి_ఖాళీలు_వరుస();
        ఒకవేళ (!VN_IS(nodep->backp(), కేటాయించు) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    శూన్య సందర్శించు(AstFell* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            userIterate(nodep->sentreep(), శూన్యసూచిక);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstFalling* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstFuture* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            userIterate(nodep->sentreep(), శూన్యసూచిక);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    శూన్య సందర్శించు(AstPast* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            nodep->dtypeFrom(nodep->exprp());
            ఒకవేళ (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ నిజం);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->ticksp(), Const);
                ఒకవేళ (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_వెనుక()->delete_వృక్షం();
                } లేకపోతే ఒకవేళ (constp->గమ్యం_SInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_వెనుక()->delete_వృక్షం();
                } లేకపోతే ఒకవేళ (V3AssertCommon::check_Sva_విలువ_హద్దు(nodep, constp, "$past tick value")) {
                    ఒకవేళ (constp->గమ్యం_SInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->గమ్యం_SInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), శూన్యసూచిక);
        }
    }
    శూన్య సందర్శించు(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{SELF, BOTH}.p());
            user_Iterate_And_తదుపరి(nodep->countp(), వెడల్పు_VP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            స్థిరరాశి AstConst* స్థిరరాశి minConstp = VN_CAST(nodep->countp(), Const);
            ఒకవేళ (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } లేకపోతే ఒకవేళ (minConstp->గమ్యం_SInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } లేకపోతే ఒకవేళ (!nodep->unbounded() && !nodep->maxCountp() && minConstp->గమ్యం_SInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            ఒకవేళ (nodep->maxCountp()) {
                user_Iterate_And_తదుపరి(nodep->maxCountp(), వెడల్పు_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                స్థిరరాశి AstConst* స్థిరరాశి maxConstp = VN_CAST(nodep->maxCountp(), Const);
                ఒకవేళ (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } లేకపోతే ఒకవేళ (minConstp && maxConstp->గమ్యం_SInt() < minConstp->గమ్యం_SInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } లేకపోతే ఒకవేళ (maxConstp->గమ్యం_SInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->propp(), వెడల్పు_VP{SELF, BOTH}.p());
            Ast_కణుపు_సమాసం* propp = nodep->propp();
            ఒకవేళ (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_తదుపరి(nodep->loBoundp(), వెడల్పు_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            ఒకవేళ (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_తదుపరి(nodep->hiBoundp(), వెడల్పు_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            స్థిరరాశి తార్కిక loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            స్థిరరాశి తార్కిక hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            ఒకవేళ (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_కణుపు* స్థిరరాశి boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtype_సమితి_బిట్టు();
                తిరిగి;
            }
            ఒకవేళ (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtype_సమితి_బిట్టు();
                తిరిగి;
            }
            స్థిరరాశి AstConst* స్థిరరాశి loConstp = VN_CAST(nodep->loBoundp(), Const);
            స్థిరరాశి AstConst* స్థిరరాశి hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            ఒకవేళ (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ఒకవేళ (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ఒకవేళ (loConstp && loConstp->గమ్యం_SInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } లేకపోతే ఒకవేళ (loConstp) {
                V3AssertCommon::check_Sva_విలువ_హద్దు(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            ఒకవేళ (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_విలువ_హద్దు(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            ఒకవేళ (!hiUnbounded && loConstp && hiConstp && hiConstp->గమ్యం_SInt() < loConstp->గమ్యం_SInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            తార్కిక hasPropertyOp = propp->is_Multi_చక్రం_Sva();
            ఒకవేళ (!hasPropertyOp) {
                propp->foreach([&](స్థిరరాశి Ast_కణుపు* np) {
                    ఒకవేళ (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = నిజం;
                    }
                });
            }
            ఒకవేళ (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fix_వెడల్పు_Reduce(propp), propp);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstRising* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstRose* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            userIterate(nodep->sentreep(), శూన్యసూచిక);
            nodep->dtype_సమితి_బిట్టు();
        }
    }

    శూన్య సందర్శించు(AstSampled* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    శూన్య సందర్శించు(AstSetuphold* nodep) override {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కేటాయించు_W* newp = శూన్యసూచిక;
        ఒకవేళ (nodep->delrefp()) {
            newp = convert_Setuphold_గమ్యం_కేటాయించు(flp, nodep->refevp(), nodep->delrefp());
        }
        ఒకవేళ (nodep->deldatap()) {
            ఒకవేళ (!newp) {
                newp = convert_Setuphold_గమ్యం_కేటాయించు(flp, nodep->dataevp(), nodep->deldatap());
            } లేకపోతే {
                newp->కూడు_తదుపరి_ఇక్కడ(
                    convert_Setuphold_గమ్యం_కేటాయించు(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        ఒకవేళ (!newp) {
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        nodep->replaceWith(కొత్త AstAlways{newp});
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    Ast_కేటాయించు_W* convert_Setuphold_గమ్యం_కేటాయించు(File_పంక్తి* స్థిరరాశి flp, Ast_కణుపు_సమాసం* స్థిరరాశి evp,
                                         Ast_కణుపు_సమాసం* స్థిరరాశి delp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = delp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = evp->clone_వృక్షం_శుద్ధ(అబద్ధం);
        UASSERT_OBJ(VN_IS(lhsp, కణుపు_Var_Ref) || VN_IS(lhsp, కణుపు_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        ఒకవేళ (Ast_కణుపు_Var_Ref* varRefp = VN_CAST(lhsp, కణుపు_Var_Ref)) {
            ఒకవేళ (varRefp->varp()->direction() == VDirection::ఆదానం) {
                VL_DO_DANGLING(నెట్టు_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(నెట్టు_Deletep(rhsp), rhsp);
                తిరిగి శూన్యసూచిక;
            }
            varRefp->access(VAccess::WRITE);
        }
        ఒకవేళ (Ast_కణుపు_Pre_Sel* selp = VN_CAST(lhsp, కణుపు_Pre_Sel)) {
            ఒకవేళ (Ast_కణుపు_Var_Ref* varRefp = VN_CAST(selp->fromp(), కణుపు_Var_Ref)) {
                ఒకవేళ (varRefp->varp()->direction() == VDirection::ఆదానం) {
                    VL_DO_DANGLING(నెట్టు_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(నెట్టు_Deletep(rhsp), rhsp);
                    తిరిగి శూన్యసూచిక;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        తిరిగి కొత్త Ast_కేటాయించు_W{flp, lhsp, rhsp};
    }

    శూన్య సందర్శించు(AstStable* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            userIterate(nodep->sentreep(), శూన్యసూచిక);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstSteady* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ నిజం);
            nodep->dtype_సమితి_బిట్టు();
        }
    }

    శూన్య సందర్శించు(Ast_Stmt_సమాసం* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{SELF, BOTH}.p());
    }

    శూన్య సందర్శించు(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_బిట్టు();
        }
    }

    శూన్య సందర్శించు(AstUntil* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            ఒకవేళ (nodep->lhsp()->వెడల్పు() != 1) fix_వెడల్పు_Reduce(nodep->lhsp());
            ఒకవేళ (nodep->rhsp()->వెడల్పు() != 1) fix_వెడల్పు_Reduce(nodep->rhsp());
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstAbortOn* nodep) override { సందర్శించు_Abort_Prop(nodep); }

    శూన్య సందర్శించు(AstRand* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            ఒకవేళ (nodep->urandom()) {
                nodep->dtype_సమితి_UInt32();  // Says the spec
            } లేకపోతే {
                nodep->dtype_సమితి_Int();  // Says the spec
            }
            ఒకవేళ (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    శూన్య సందర్శించు(AstSEventually* nodep) override {
        ఒకవేళ (v3Global.opt.timing().is_సమితి_False() || !v3Global.opt.timing().is_సమితి_True()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::Widthed_విలువ{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstSGotoRep* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_తదుపరి(nodep->countp(), వెడల్పు_VP{SELF, BOTH}.p());
            ఒకవేళ (nodep->maxCountp()) {
                వెడల్పు_Check_Goto_Rep_Range(nodep, "Goto");
            } లేకపోతే {
                వెడల్పు_Check_Rep_లెక్క(nodep);
            }
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstSNonConsRep* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_తదుపరి(nodep->countp(), వెడల్పు_VP{SELF, BOTH}.p());
            ఒకవేళ (nodep->maxCountp()) {
                వెడల్పు_Check_Goto_Rep_Range(nodep, "Nonconsecutive");
            } లేకపోతే {
                వెడల్పు_Check_Rep_లెక్క(nodep);
            }
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    మూస <రకం_పేరు T_Rep>
    శూన్య వెడల్పు_Check_Rep_లెక్క(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->countp(), Const);
        ఒకవేళ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } లేకపోతే ఒకవేళ (constp->dtypep()->isSigned() && constp->అంకె().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } లేకపోతే ఒకవేళ (constp->అంకె().most_సమితి_బిట్టు_P1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } లేకపోతే ఒకవేళ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    మూస <రకం_పేరు T_Rep>
    శూన్య వెడల్పు_Check_Goto_Rep_Range(T_Rep* nodep, స్థిరరాశి అక్షరం* రకం) {
        user_Iterate_And_తదుపరి(nodep->maxCountp(), వెడల్పు_VP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        స్థిరరాశి AstConst* స్థిరరాశి minConstp = VN_CAST(nodep->countp(), Const);
        స్థిరరాశి AstConst* స్థిరరాశి maxConstp = VN_CAST(nodep->maxCountp(), Const);
        ఒకవేళ (!minConstp || !maxConstp) {
            nodep->v3error(std::string{రకం}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            తిరిగి;
        }
        ఒకవేళ (minConstp->గమ్యం_SInt() < 0) {
            nodep->v3error(std::string{రకం}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            తిరిగి;
        }
        ఒకవేళ (maxConstp->గమ్యం_SInt() < minConstp->గమ్యం_SInt()) {
            nodep->v3error(std::string{రకం}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            తిరిగి;
        }
        ఒకవేళ (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + రకం
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    శూన్య సందర్శించు(AstSThroughout* nodep) override {
        m_hasSExpr = నిజం;
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = అబద్ధం;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = నిజం;
            iterate(nodep->rhsp());
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = నిజం;
        m_hasSExpr = నిజం;
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            ఒకవేళ (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = నిజం;
        m_hasSExpr = నిజం;
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            కొరకు (Ast_Sen_అంశం* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), Sen_అంశం)) {
                user_Iterate_And_తదుపరి(senp->sensp(), వెడల్పు_VP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstURandomRange* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            nodep->dtype_సమితి_UInt32();  // Says the spec
            Ast_కణుపు_DType* స్థిరరాశి expDTypep = nodep->కనుగొను_UInt32_DType();
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    శూన్య సందర్శించు(AstUnbounded* nodep) override {
        nodep->dtype_సమితి_Int();  // Used in int context
        ఒకవేళ (VN_IS(nodep->backp(), IsUnbounded)) తిరిగి;  // Ok, leave
        ఒకవేళ (VN_IS(nodep->backp(), BracketArrayDType)) తిరిగి;  // Ok, leave
        ఒకవేళ (VN_IS(nodep->backp(), InsideRange)) తిరిగి;  // Ok, leave
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి varp = VN_CAST(nodep->backp(), Var)) {
            ఒకవేళ (varp->isParam()) తిరిగి;  // Ok, leave
        }
        Ast_కణుపు* backp = nodep->backp();
        ఒకవేళ (VN_IS(backp, Sub)) backp = backp->backp();
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి selp = VN_CAST(backp, Sel_వెలికితీయి)) {
            ఒకవేళ (VN_IS(selp->fromp()->dtypep()->skipRefp(), వరుస_DType)) తిరిగి;
        }
        ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి selp = VN_CAST(backp, Sel_బిట్టు)) {
            ఒకవేళ (VN_IS(selp->fromp()->dtypep()->skipRefp(), వరుస_DType)) తిరిగి;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    శూన్య సందర్శించు(AstInferredDisable* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) nodep->dtype_సమితి_బిట్టు();
    }
    శూన్య సందర్శించు(AstIsUnbounded* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        ఒకవేళ (m_vup && m_vup->prelim()) {
            nodep->dtype_సమితి_బిట్టు_Unsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        }
        ఒకవేళ (m_vup->final()) {
            Ast_కణుపు_DType* స్థిరరాశి expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            ఒకవేళ (nodep->వెడల్పు() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    శూన్య సందర్శించు(AstCLog2* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    మూస <రకం_పేరు T_Option>
    శూన్య వెడల్పు_Cover_బరువు(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = nodep->valuep();
        స్థిరరాశి std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const)) {
            ఒకవేళ (!constp->అంకె().isNegative()) తిరిగి;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->గమ్యం_SInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } లేకపోతే ఒకవేళ (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } లేకపోతే {
            తిరిగి;
        }
        // Continue with the default weight
        valuep->replaceWith(కొత్త AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    మూస <రకం_పేరు T_Option>
    శూన్య వెడల్పు_Cover_Int(T_Option* nodep) {
        user_Iterate_And_తదుపరి(nodep->valuep(), వెడల్పు_VP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    శూన్య సందర్శించు(Ast_Cg_Option_కేటాయించు* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* స్థిరరాశి cgClassp = m_containingClassFinder.కనుగొను(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        ఒకవేళ (nodep->optType() == VCoverOptionType::బరువు) {
            వెడల్పు_Cover_బరువు(nodep);
            తిరిగి;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        ఒకవేళ (nodep->optType() == VCoverOptionType::AT_కనిష్ఠ) {
            వెడల్పు_Cover_Int(nodep);
            తిరిగి;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        ఒకవేళ (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            ఒకవేళ (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->గమ్యం_SInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->గమ్యం_SInt() << పంక్తి_ముగింపు);
            } లేకపోతే {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } లేకపోతే ఒకవేళ (nodep->optType() == VCoverOptionType::విలీనం_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_కణుపు_సమాసం* స్థిరరాశి valuep = nodep->valuep();
            ఒకవేళ (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(కొత్త AstConst{valuep->fileline(), AstConst::బిట్టు_False{}});
                VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
            }
            తిరిగి;
        } లేకపోతే ఒకవేళ (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            తిరిగి;
        } లేకపోతే ఒకవేళ (nodep->optType() == VCoverOptionType::DISTRIBUTE_మొదటి) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->valuep(), Const);
            ఒకవేళ (!constp || !constp->అంకె().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }
    శూన్య సందర్శించు(AstCoverOption* nodep) override {
        ఒకవేళ (nodep->optType() == VCoverOptionType::బరువు) {
            వెడల్పు_Cover_బరువు(nodep);
            తిరిగి;
        }
        ఒకవేళ (nodep->optType() == VCoverOptionType::AT_కనిష్ఠ
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            వెడల్పు_Cover_Int(nodep);
            తిరిగి;
        }
        user_Iterate_And_తదుపరి(nodep->valuep(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstCoverCross* nodep) override {
        user_Iterate_And_తదుపరి(nodep->itemsp(), శూన్యసూచిక);
        ఒకవేళ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_తదుపరి(nodep->optionsp(), శూన్యసూచిక);
        user_Iterate_And_తదుపరి(nodep->binsp(), శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstCoverCrossBin* nodep) override {
        user_Iterate_And_తదుపరి(nodep->selectp(), శూన్యసూచిక);
        ఒకవేళ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    శూన్య సందర్శించు(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_తదుపరి(nodep->binsp(), వెడల్పు_VP{nodep->exprp()->dtypep(), BOTH}.p());
        ఒకవేళ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_తదుపరి(nodep->optionsp(), శూన్యసూచిక);
    }
    శూన్య వెడల్పు_Covergroup_Ranges(Ast_కణుపు* rangesp, పూర్ణాంకం fill_వెడల్పు) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        స్థిరరాశి స్వయంచాలక fill = [&](Ast_కణుపు* nodep) {
            ఒకవేళ (!fill_వెడల్పు) తిరిగి;
            Ast_కణుపు_సమాసం* exprp = VN_AS(nodep, కణుపు_సమాసం);
            fixAutoExtend(exprp /*ref*/, fill_వెడల్పు);
        };
        కొరకు (Ast_కణుపు *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ఒకవేళ (AstInsideRange* స్థిరరాశి rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, శూన్యసూచిక);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } లేకపోతే ఒకవేళ (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } లేకపోతే {
                itemp = userIterateSubtreeReturnEdits(itemp, వెడల్పు_VP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    శూన్య సందర్శించు(AstCoverBinsof* nodep) override {
        user_Iterate_And_తదుపరి(nodep->pointp(), శూన్యసూచిక);
        వెడల్పు_Covergroup_Ranges(nodep->rangesp(), 0);
    }
    శూన్య సందర్శించు(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        వెడల్పు_Covergroup_Ranges(nodep->rangesp(), m_vup ? m_vup->dtypep()->వెడల్పు() : 0);
        ఒకవేళ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        ఒకవేళ (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_తదుపరి(nodep->arraySizep(), వెడల్పు_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_తదుపరి(nodep->transp(), m_vup);
    }
    శూన్య సందర్శించు(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_తదుపరి(nodep->itemp(), శూన్యసూచిక);
        ఒకవేళ (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        ఒకవేళ (!VN_IS(nodep->subp(), CoverpointRef)) {
            వెడల్పు_Covergroup_Ranges(nodep->subp(), m_vup->dtypep()->వెడల్పు());
        }
        వెడల్పు_Cover_Filter(nodep->filterp(), "Bin");
    }
    శూన్య సందర్శించు(AstCoverCrossWith* nodep) override {
        user_Iterate_And_తదుపరి(nodep->selectp(), శూన్యసూచిక);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_తదుపరి(nodep->varsp(), శూన్యసూచిక);
        వెడల్పు_Cover_Filter(nodep->filterp(), "Cross bin");
        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి countp = nodep->countp()) {
            ఒకవేళ (VN_IS(countp, Unbounded)) {
                countp->dtype_సమితి_Int();  // 'matches $'
            } లేకపోతే {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_తదుపరి(countp, వెడల్పు_VP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    శూన్య వెడల్పు_Cover_Filter(Ast_కణుపు_సమాసం* filterp, స్థిరరాశి అక్షరం* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, వెడల్పు_VP{SELF, BOTH}.p()), కణుపు_సమాసం);
        File_పంక్తి* స్థిరరాశి fl = filterp->fileline();
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి resultp = filterp->dtypep()->skipRefp();
        ఒకవేళ (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlink_Fr_వెనుక(&relinker);
            relinker.relink(
                కొత్త AstNeqD{fl, filterp, కొత్త AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } లేకపోతే ఒకవేళ (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(కొత్త AstConst{fl, AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(filterp), filterp);
        } లేకపోతే {
            fix_వెడల్పు_Reduce(filterp);
        }
    }
    శూన్య సందర్శించు(Ast_Cover_Trans_సమితి* nodep) override { user_Iterate_And_తదుపరి(nodep->itemsp(), m_vup); }
    శూన్య సందర్శించు(Ast_Cover_Trans_అంశం* nodep) override {
        వెడల్పు_Covergroup_Ranges(nodep->valuesp(), m_vup ? m_vup->dtypep()->వెడల్పు() : 0);
    }
    శూన్య సందర్శించు(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            ఒకవేళ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                తిరిగి;
            }
            checkCvtUS(nodep->lhsp(), అబద్ధం);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ఒకవేళ (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->అంకె().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtype_సమితి_Logic_Unsized(nodep->వెడల్పు(), nodep->వెడల్పు_Min(),
                                            nodep->dtypep()->numeric());
            }
        }

        ఒకవేళ (m_vup->final()) {
            Ast_కణుపు_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_కణుపు_DType* స్థిరరాశి lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->కనుగొను_బిట్టు_Or_Logic_DType(expDTypep->వెడల్పు(), expDTypep->వెడల్పు_Min(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_కణుపు* newp = శూన్యసూచిక;  // No change
            ఒకవేళ (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = కొత్త AstPowSS{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(),
                                    nodep->rhsp()->unlink_Fr_వెనుక()};
            } లేకపోతే ఒకవేళ (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = కొత్త AstPowSU{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(),
                                    nodep->rhsp()->unlink_Fr_వెనుక()};
            } లేకపోతే ఒకవేళ (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = కొత్త AstPowUS{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(),
                                    nodep->rhsp()->unlink_Fr_వెనుక()};
            }
            ఒకవేళ (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        }
    }
    శూన్య సందర్శించు(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
        user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
        user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
        user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(Ast_లెక్క_బిట్లు* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            స్థిరరాశి పూర్ణాంకం వెడల్పు_Min = V3_సంఖ్య::log2b(nodep->lhsp()->వెడల్పు()) + 1;
            nodep->dtype_సమితి_బిట్టు_Unsized(32, వెడల్పు_Min, VSigning::SIGNED);
        }
    }
    శూన్య సందర్శించు(Ast_లెక్క_Ones* nodep) override {
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            స్థిరరాశి పూర్ణాంకం వెడల్పు_Min = V3_సంఖ్య::log2b(nodep->lhsp()->వెడల్పు()) + 1;
            nodep->dtype_సమితి_బిట్టు_Unsized(32, వెడల్పు_Min, VSigning::SIGNED);
        }
    }
    శూన్య సందర్శించు(AstCvtPackString* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        // Opaque returns, so arbitrary
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH, current_ప్రవాహం_Use()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dt = nodep->lhsp()->dtypep()) {
            ఒకవేళ (VN_IS(dt->skip_Ref_గమ్యం_Enump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    శూన్య సందర్శించు(Ast_Cvt_Packed_గమ్యం_Array* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        // Opaque returns, so arbitrary
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, BOTH, current_ప్రవాహం_Use()}.p());
        // Type set in constructor
    }
    శూన్య సందర్శించు(Ast_Cvt_Array_గమ్యం_Array* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        // Opaque returns, so arbitrary
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, BOTH, current_ప్రవాహం_Use()}.p());
        // Type set in constructor
    }
    శూన్య సందర్శించు(Ast_Cvt_Array_గమ్యం_Packed* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        // Opaque returns, so arbitrary
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, BOTH, current_ప్రవాహం_Use()}.p());
        // Type set in constructor
    }
    శూన్య సందర్శించు(Ast_Cvt_Unpack_గమ్యం_వరుస* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        // Opaque returns, so arbitrary
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, BOTH}.p());
        // Type set in constructor
    }
    శూన్య సందర్శించు(Ast_Cvt_Unpack_గమ్యం_Unpack* nodep) override {
        // Type set when created, the value converted is of its own type
        ఒకవేళ (m_vup->final()) {
            user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    శూన్య సందర్శించు(Ast_సమయం_Import* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        స్థిరరాశి ద్విగుణ సమయం_Prescale = constp->అంకె().గమ్యం_Double();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        స్థిరరాశి ద్విగుణ సమయం_2 = సమయం_Prescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), AstConst::RealDouble{}, సమయం_2};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstEventControl* nodep) override {
        ఒకవేళ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        ఒకవేళ (nodep->fileline()->timingOn()) {
            ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
                iterateChildren(nodep);
                తిరిగి;
            } లేకపోతే ఒకవేళ (v3Global.opt.timing().is_సమితి_False()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warn_మరిన్ని()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } లేకపోతే {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        ఒకవేళ (nodep->stmtsp()) nodep->కూడు_తదుపరి_ఇక్కడ(nodep->stmtsp()->unlink_Fr_వెనుక());
        VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    స్థిర string typenameExtends(స్థిరరాశి AstClass* classp) {
        // An interface class may extend several, so those are not named
        ఒకవేళ (!classp || classp->isInterfaceClass()) తిరిగి "";
        కొరకు (స్థిరరాశి AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ఒకవేళ (extendsp->isImplements()) కొనసాగు;
            స్థిరరాశి AstClass* స్థిరరాశి basep = extendsp->classOrNullp();
            తిరిగి basep ? " extends class " + basep->dtypeName(నిజం) : "";
        }
        తిరిగి "";
    }
    శూన్య సందర్శించు(Ast_Attr_యొక్క* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, BOTH}.p());
        ఒకవేళ (nodep->dimp()) user_Iterate_And_తదుపరి(nodep->dimp(), వెడల్పు_VP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        ఎంపిక (nodep->attrType()) {
        స్థితి VAttrType::DIM_DIMENSIONS:
        స్థితి VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_కణుపు_DType* స్థిరరాశి dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            స్థిరరాశి std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(నిజం);
            స్థిరరాశి పూర్ణాంకం val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.రెండవ
                                                                       : (dim.మొదటి + dim.రెండవ));
            nodep->replaceWith(కొత్త AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            ఆపు;
        }
        స్థితి VAttrType::DIM_బిట్లు_OR_సంఖ్య: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            ఒకవేళ (VN_IS(nodep->fromp(), కణుపు_సమాసం)) {
                nodep->replaceWith(nodep->fromp()->unlink_Fr_వెనుక());
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            } లేకపోతే {
                Ast_కణుపు* newp = కొత్త Ast_Attr_యొక్క{nodep->fileline(), VAttrType::DIM_బిట్లు,
                                              nodep->fromp()->unlink_Fr_వెనుక()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                user_Iterate_And_తదుపరి(newp, వెడల్పు_VP{SELF, BOTH}.p());  // Convert AttrOf
            }
            తిరిగి;
        }
        స్థితి VAttrType::DIM_బిట్లు:
        స్థితి VAttrType::DIM_ఎగువ:
        స్థితి VAttrType::DIM_INCREMENT:
        స్థితి VAttrType::DIM_ఎడమ:
        స్థితి VAttrType::DIM_దిగువ:
        స్థితి VAttrType::DIM_కుడి:
        స్థితి VAttrType::DIM_కొలత: {
            Ast_కణుపు_DType* స్థిరరాశి dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            ఒకవేళ (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి ownModp = v3Global.rootp()->containingModule(dtypep);
                ఒకవేళ (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtype_సమితి_Int();
                    తిరిగి;
                }
            }

            ఒకవేళ (VN_IS(dtypep, వరుస_DType) || VN_IS(dtypep, DynArrayDType)) {
                ఎంపిక (nodep->attrType()) {
                స్థితి VAttrType::DIM_కొలత: {
                    Ast_కణుపు_సమాసం* స్థిరరాశి fromp = VN_AS(nodep->fromp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
                    Ast_కణుపు* స్థిరరాశి newp
                        = కొత్త AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_కొలత};
                    newp->dtype_సమితి_Int();
                    newp->did_వెడల్పు(నిజం);
                    newp->protect(అబద్ధం);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                    ఆపు;
                }
                స్థితి VAttrType::DIM_ఎడమ:
                స్థితి VAttrType::DIM_దిగువ: {
                    Ast_కణుపు* స్థిరరాశి newp = కొత్త AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    ఆపు;
                }
                స్థితి VAttrType::DIM_కుడి:
                స్థితి VAttrType::DIM_ఎగువ: {
                    Ast_కణుపు_సమాసం* స్థిరరాశి fromp = VN_AS(nodep->fromp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
                    Ast_కణుపు_సమాసం* స్థిరరాశి sizep
                        = కొత్త AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_కొలత};
                    sizep->dtype_సమితి_Int();
                    sizep->did_వెడల్పు(నిజం);
                    sizep->protect(అబద్ధం);
                    Ast_కణుపు* స్థిరరాశి newp
                        = కొత్త AstSub{nodep->fileline(), sizep,
                                     కొత్త AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                    ఆపు;
                }
                స్థితి VAttrType::DIM_INCREMENT: {
                    Ast_కణుపు_సమాసం* స్థిరరాశి newp
                        = కొత్త AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    ఆపు;
                }
                స్థితి VAttrType::DIM_బిట్లు: {
                    ఒకవేళ (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } లేకపోతే {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    ఆపు;
                }
                అప్రమేయ: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } లేకపోతే {
                స్థిరరాశి std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(నిజం);
                స్థిరరాశి uint32_t msbdim = dimpair.మొదటి + dimpair.రెండవ;
                ఒకవేళ (!nodep->dimp() || msbdim < 1) {
                    ఒకవేళ (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = VN_AS(nodep->fromp()->unlink_Fr_వెనుక(), కణుపు_సమాసం);
                        Ast_కణుపు* స్థిరరాశి newp = కొత్త Ast_జరుపు_L{
                            nodep->fileline(), కొత్త AstLenN{nodep->fileline(), fromp},
                            కొత్త AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    } లేకపోతే {
                        Ast_కణుపు_DType* స్థిరరాశి పునాది_DTypep = dtypep->skipRefp();
                        UASSERT_OBJ(పునాది_DTypep, nodep, "Unsized expression");
                        స్థిరరాశి పూర్ణాంకం dim = 1;
                        AstConst* స్థిరరాశి newp = dimension_విలువ(nodep->fileline(), పునాది_DTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    }
                } లేకపోతే ఒకవేళ (VN_IS(nodep->dimp(), Const)) {
                    స్థిరరాశి పూర్ణాంకం dim = VN_AS(nodep->dimp(), Const)->గమ్యం_SInt();
                    AstConst* స్థిరరాశి newp
                        = dimension_విలువ(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                } లేకపోతే {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* స్థిరరాశి tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_కణుపు_సమాసం* స్థిరరాశి dimp = nodep->dimp()->unlink_Fr_వెనుక();
                    Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                }
            }
            ఆపు;
        }
        స్థితి VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ఆపు;
        స్థితి VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_కణుపు_DType* స్థిరరాశి protoDtp = nodep->fromp()->dtypep();
            Ast_కణుపు_DType* స్థిరరాశి declDtp = [&]() {
                ఒకవేళ (m_ftaskp->fvarp()) తిరిగి m_ftaskp->fvarp()->dtypep();
                Ast_కణుపు_DType* స్థిరరాశి voidp = కొత్త AstVoidDType{m_ftaskp->fileline()};
                నెట్టు_Deletep(voidp);  // Note voidp used past here
                తిరిగి voidp;
            }();
            ఒకవేళ (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warn_మరిన్ని()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warn_మరిన్ని()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ఆపు;
        }
        స్థితి VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->fromp()->dtypep();
            string ఫలితం = dtypep->prettyDTypeName(నిజం);
            ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                ఫలితం += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << ఫలితం << "' from " << dtypep);
            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), AstConst::String{}, ఫలితం};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            ఆపు;
        }
        స్థితి VAttrType::TYPEID:
            ఒకవేళ (Ast_కణుపు_DType* dtypep = VN_CAST(nodep->fromp(), కణుపు_DType)) {
                nodep->dtypep(dtypep);
            } లేకపోతే {
                nodep->dtypep(VN_AS(nodep->fromp(), కణుపు_సమాసం)->dtypep());
            }
            ఆపు;
        స్థితి VAttrType::VAR_పునాది:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            ఆపు;
        అప్రమేయ: {
            // Everything else resolved earlier
            nodep->dtype_సమితి_బిట్టు_Unsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            ఆపు;
        }
        }
    }
    శూన్య సందర్శించు(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(Ast_పాఠ్యం* nodep) override {
        // Only used in CStmts which don't care....
    }

    శూన్య సందర్శించు(AstCellArrayRef* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        user_Iterate_And_తదుపరి(nodep->selp(), వెడల్పు_VP{SELF, PRELIM}.p());
        user_Iterate_And_తదుపరి(nodep->selp(), వెడల్పు_VP{SELF, FINAL}.p());
        nodep->dtype_సమితి_Void();  // placeholder; this node shouldnt survive beyond linking
        nodep->did_వెడల్పు(నిజం);
    }

    శూన్య సందర్శించు(AstCellRef* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;

        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి cellExprp = VN_CAST(nodep->cellp(), కణుపు_సమాసం)) {
            user_Iterate_And_తదుపరి(cellExprp, వెడల్పు_VP{SELF, PRELIM}.p());
            user_Iterate_And_తదుపరి(cellExprp, వెడల్పు_VP{SELF, FINAL}.p());
        }

        ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_CAST(nodep->exprp(), కణుపు_సమాసం)) {
            user_Iterate_And_తదుపరి(exprp, వెడల్పు_VP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_తదుపరి(exprp, వెడల్పు_VP{SELF, FINAL}.p());
        } లేకపోతే {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->did_వెడల్పు(నిజం);
    }

    // DTYPES
    శూన్య సందర్శించు(Ast_కణుపు_Array_DType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed

        ఒకవేళ (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* స్థిరరాశి basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            ఒకవేళ (basicp->implicit()) {
                UASSERT_OBJ(basicp->వెడల్పు() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->వెడల్పు() << " bits");
                AstBasicDType* స్థిరరాశి newp = కొత్త AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->వెడల్పు_Force(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        ఒకవేళ (VN_IS(nodep, PackArrayDType)) m_underPackedArray = నిజం;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_తదుపరి(nodep->rangep(), వెడల్పు_VP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->వెడల్పు_From_Sub(nodep->subDTypep());
            ఒకవేళ (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(నిజం);
        } లేకపోతే {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి elemDTypep = nodep->subDTypep()->skipRefp();
            ఒకవేళ (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->subDTypep()->వెడల్పు() * nodep->rangep()->elementsConst();
            nodep->వెడల్పు_Force(వెడల్పు, వెడల్పు);
            ఒకవేళ (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                స్థిరరాశి Ast_కణుపు_DType* పునాది_DTypep = nodep->subDTypep();
                అయితే (!VN_IS(పునాది_DTypep, RefDType) && పునాది_DTypep->subDTypep()) {
                    పునాది_DTypep = పునాది_DTypep->subDTypep();
                }
                స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = nodep->basicp();
                ఒకవేళ (!VN_IS(పునాది_DTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(నిజం));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(AstAssocArrayDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->కీలకం_DTypep(iterateEditMoveDTypep(nodep, nodep->కీలకం_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_కణుపు_DType* స్థిరరాశి childp = nodep->బిడ్డ_DTypep();
        childp->unlink_Fr_వెనుక();
        Ast_కణుపు* స్థిరరాశి elementsp = nodep->elementsp()->unlink_Fr_వెనుక();
        Ast_కణుపు* newp;
        ఒకవేళ (VN_IS(elementsp, Unbounded)) {
            newp = కొత్త Ast_వరుస_DType{nodep->fileline(), VFlag_బిడ్డ_DType{}, childp, శూన్యసూచిక};
            VL_DO_DANGLING(elementsp->delete_వృక్షం(), elementsp);
        } లేకపోతే ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి keyp = VN_CAST(elementsp, కణుపు_DType)) {
            newp = కొత్త AstAssocArrayDType{nodep->fileline(), VFlag_బిడ్డ_DType{}, childp, keyp};
        } లేకపోతే {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_పంక్తి* స్థిరరాశి elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3_Error_సంకేతం::WIDTHEXPAND, నిజం);
            // Must be expression that is constant, but we'll determine that later
            newp = కొత్త AstUnpackArrayDType{
                nodep->fileline(), VFlag_బిడ్డ_DType{}, childp,
                కొత్త AstRange{nodep->fileline(), కొత్త AstConst(elementsp->fileline(), 0),
                             కొత్త AstSub{elementsNewFl, VN_AS(elementsp, కణుపు_సమాసం),
                                        కొత్త AstConst(elementsNewFl, 1)},
                             నిజం}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, శూన్యసూచిక), newp);
    }
    శూన్య సందర్శించు(AstConstraintRefDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    శూన్య సందర్శించు(AstDynArrayDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(Ast_వరుస_DType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_తదుపరి(nodep->boundp(), వెడల్పు_VP{SELF, BOTH}.p());
        ఒకవేళ (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlink_Fr_వెనుక()->delete_వృక్షం();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(AstVoidDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(AstUnsizedArrayDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        ఒకవేళ (nodep->బిడ్డ_DTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(AstWildcardArrayDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(AstBasicDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        ఒకవేళ (nodep->generic()) తిరిగి;  // Already perfect
        ఒకవేళ (nodep->rangep()) {
            user_Iterate_And_తదుపరి(nodep->rangep(), వెడల్పు_VP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->వెడల్పు_Force(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } లేకపోతే ఒకవేళ (nodep->isRanged()) {
            nodep->వెడల్పు_Force(nodep->nrange().elements(), nodep->nrange().elements());
        } లేకపోతే ఒకవేళ (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->వెడల్పు_Force(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(AstConstDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, శూన్యసూచిక);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->వెడల్పు_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(AstRefDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_వెడల్పు(నిజం);
        ఒకవేళ (nodep->typeofp()) {  // type(typeofp_expression)
            ఒకవేళ (Ast_కణుపు_DType* typeofDtp = VN_CAST(nodep->typeofp(), కణుపు_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } లేకపోతే {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_తదుపరి(nodep->typeofp(), వెడల్పు_VP{SELF, BOTH}.p());
                Ast_కణుపు* స్థిరరాశి typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlink_Fr_వెనుక()->delete_వృక్షం(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, శూన్యసూచిక);
        ఒకవేళ (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), శూన్యసూచిక);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), శూన్యసూచిక);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        ఒకవేళ (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            స్థిరరాశి తార్కిక లోపల_Template_Module = !m_modep || m_modep->parameterizedTemplate();
            ఒకవేళ (లోపల_Template_Module) {
                nodep->doing_వెడల్పు(అబద్ధం);
                తిరిగి;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->వెడల్పు_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doing_వెడల్పు(అబద్ధం);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    శూన్య వెడల్పు_DType_Name_Params(స్థిరరాశి AstTypedef* nodep) {
        స్థిరరాశి VDTypeNameScopes* స్థిరరాశి scopesp = VDTypeNameScopes::currentp();
        ఒకవేళ (!scopesp) తిరిగి;  // Computing parameters, as by V3Param, so naming as found
        Ast_కణుపు* scopep = శూన్యసూచిక;
        కొరకు (స్థిరరాశి Ast_కణుపు* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            ఒకవేళ (స్థిరరాశి std::సదిశ<Ast_కణుపు*>* స్థిరరాశి paramsp = scopesp->paramsp(scopep)) {
                కొరకు (Ast_కణుపు* స్థిరరాశి paramp : *paramsp) {
                    ఒకవేళ (!paramp->did_వెడల్పు()) userIterate(paramp, శూన్యసూచిక);
                }
            }
        }
    }
    శూన్య సందర్శించు(AstTypedef* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి refp = check_Ref_గమ్యం_Typedef_Recurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(శూన్యసూచిక);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
            తిరిగి;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        స్థిరరాశి తార్కిక declares = nodep->బిడ్డ_DTypep() != శూన్యసూచిక;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ఒకవేళ (declares) {
            Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->dtypep();
            ఒకవేళ (Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి sdtypep = VN_CAST(dtypep, కణుపు_UOr_Struct_DType)) {
                వెడల్పు_DType_Name_Params(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } లేకపోతే ఒకవేళ (AstEnumDType* స్థిరరాశి edtypep = VN_CAST(dtypep, EnumDType)) {
                వెడల్పు_DType_Name_Params(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstParamTypeDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, శూన్యసూచిక);
        nodep->వెడల్పు_From_Sub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        ఒకవేళ (nodep->dtypep() && nodep->బిడ్డ_DTypep()) { nodep->బిడ్డ_DTypep(శూన్యసూచిక); }
    }
    శూన్య సందర్శించు(AstRequireDType* nodep) override {
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
        ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి dtp = VN_CAST(nodep->lhsp(), కణుపు_DType)) {
            nodep->replaceWith(dtp->unlink_Fr_వెనుక());
        } లేకపోతే {
            ఒకవేళ (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(కొత్త AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        Ast_కణుపు_DType* స్థిరరాశి గమ్యం_Dtp = nodep->శిఖరం()->dtypep()->skip_Ref_గమ్యం_Enump();
        Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skip_Ref_గమ్యం_Enump();
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        స్థిరరాశి స్వయంచాలక castable = Ast_కణుపు::computeCastable(గమ్యం_Dtp, fromDtp, nodep->fromp());
        Ast_కణుపు* newp;
        ఒకవేళ (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            తిరిగి;
        } లేకపోతే ఒకవేళ (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* స్థిరరాశి enumDtp = VN_AS(గమ్యం_Dtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_కణుపు_సమాసం* స్థిరరాశి testp = enumTestValid(nodep->fromp(), enumDtp);
            File_పంక్తి* స్థిరరాశి fl_novalue = కొత్త File_పంక్తి{fl};
            fl_novalue->warnOff(V3_Error_సంకేతం::ENUMVALUE, నిజం);
            newp = కొత్త AstCond{
                fl, testp,
                కొత్త Ast_సమాసం_Stmt{fl,
                                కొత్త Ast_కేటాయించు{fl_novalue, nodep->శిఖరం()->unlink_Fr_వెనుక(),
                                              nodep->fromp()->unlink_Fr_వెనుక()},
                                కొత్త AstConst{fl, AstConst::Signed32{}, 1}},
                కొత్త AstConst{fl, AstConst::Signed32{}, 0}};
        } లేకపోతే ఒకవేళ ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(గమ్యం_Dtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << గమ్యం_Dtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warn_మరిన్ని() << "... Suggest static cast");
            newp = కొత్త Ast_సమాసం_Stmt{
                fl,
                కొత్త Ast_కేటాయించు{fl, nodep->శిఖరం()->unlink_Fr_వెనుక(),
                              కొత్త AstCast{fl, nodep->fromp()->unlink_Fr_వెనుక(), గమ్యం_Dtp}},
                కొత్త AstConst{fl, AstConst::Signed32{}, 1}};
        } లేకపోతే ఒకవేళ (castable == VCastable::INCOMPATIBLE) {
            newp = కొత్త AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << గమ్యం_Dtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } లేకపోతే {
            newp = కొత్త AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << గమ్యం_Dtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warn_మరిన్ని()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    శూన్య సందర్శించు(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlink_Fr_వెనుక();
            Ast_కణుపు* స్థిరరాశి newp
                = కొత్త Ast_Cast_కొలత{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } లేకపోతే ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి refp = VN_CAST(nodep->dtp(), కణుపు_DType)) {
            refp->unlink_Fr_వెనుక();
            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstCast{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(),
                                              VFlag_బిడ్డ_DType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } లేకపోతే {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(AstCast* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ఒకవేళ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            ఒకవేళ (AstSigned* స్థిరరాశి fromp = VN_CAST(nodep->fromp(), Signed)) {
                ఒకవేళ (VN_IS(fromp->lhsp(), కణుపు_ప్రవాహం)) {
                    Ast_కణుపు* స్థిరరాశి lhsp = fromp->lhsp()->unlink_Fr_వెనుక();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_వృక్షం(), fromp);
                }
            }
            user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, PRELIM, ప్రవాహం_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_కణుపు_DType* స్థిరరాశి గమ్యం_Dtp = nodep->dtypep()->skip_Ref_గమ్యం_Enump();
            Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skip_Ref_గమ్యం_Enump();
            స్థిరరాశి స్వయంచాలక castable = Ast_కణుపు::computeCastable(గమ్యం_Dtp, fromDtp, nodep->fromp());
            తార్కిక bad = అబద్ధం;
            ఒకవేళ (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << గమ్యం_Dtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = నిజం;
            } లేకపోతే ఒకవేళ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } లేకపోతే ఒకవేళ (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << గమ్యం_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warn_మరిన్ని() << "... Suggest dynamic $cast");
                bad = నిజం;
            } లేకపోతే ఒకవేళ (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << గమ్యం_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = నిజం;
            } లేకపోతే {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
            ఒకవేళ (bad) {
            } లేకపోతే ఒకవేళ (AstBasicDType* స్థిరరాశి basicp = గమ్యం_Dtp->basicp()) {
                ఒకవేళ (!basicp->isString() && fromDtp->isString()) {
                    newp = కొత్త AstNToI{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(), గమ్యం_Dtp};
                } లేకపోతే ఒకవేళ (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_కణుపు_DType* స్థిరరాశి origDTypep = nodep->dtypep();
                    ఒకవేళ (!VN_IS(fromDtp, ప్రవాహం_DType)) {
                        స్థిరరాశి పూర్ణాంకం వెడల్పు = గమ్యం_Dtp->వెడల్పు();
                        castSized(nodep, nodep->fromp(), వెడల్పు);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } లేకపోతే {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 అబద్ధం);
                }
                ఒకవేళ (newp) {
                } లేకపోతే ఒకవేళ (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    ఒకవేళ (nodep->fromp()->isSigned()) {
                        newp = కొత్త AstISToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక()};
                    } లేకపోతే {
                        newp = కొత్త AstIToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక()};
                    }
                } లేకపోతే ఒకవేళ (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = కొత్త AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక()};
                    newp->dtypep(basicp);
                } లేకపోతే ఒకవేళ (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = కొత్త AstSigned{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక()};
                } లేకపోతే ఒకవేళ (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = కొత్త AstUnsigned{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక()};
                } లేకపోతే {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } లేకపోతే ఒకవేళ (VN_IS(గమ్యం_Dtp, వరుస_DType)) {
                ఒకవేళ (VN_IS(fromDtp, BasicDType)) {
                    newp = కొత్త Ast_Cvt_Packed_గమ్యం_Array{nodep->fileline(),
                                                   nodep->fromp()->unlink_Fr_వెనుక(), గమ్యం_Dtp};
                } లేకపోతే ఒకవేళ (VN_IS(fromDtp, వరుస_DType) || VN_IS(fromDtp, ప్రవాహం_DType)) {
                    పూర్ణాంకం src_Element_బిట్లు = 1;
                    పూర్ణాంకం dst_Element_బిట్లు = 1;
                    ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి elemDtp = fromDtp->subDTypep()) {
                        src_Element_బిట్లు = elemDtp->వెడల్పు();
                    }
                    స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి dst_వరుస_Dtp = VN_AS(గమ్యం_Dtp, వరుస_DType);
                    ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి elemDtp = dst_వరుస_Dtp->subDTypep()) {
                        dst_Element_బిట్లు = elemDtp->వెడల్పు();
                    }
                    newp = కొత్త Ast_Cvt_Array_గమ్యం_Array{nodep->fileline(),
                                                  nodep->fromp()->unlink_Fr_వెనుక(),
                                                  గమ్యం_Dtp,
                                                  అబద్ధం,
                                                  1,
                                                  dst_Element_బిట్లు,
                                                  src_Element_బిట్లు};
                }
            } లేకపోతే ఒకవేళ (VN_IS(గమ్యం_Dtp, ClassRefDType)) {
                // Can just remove cast
            } లేకపోతే {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << గమ్యం_Dtp->prettyDTypeNameQ());
            }
            ఒకవేళ (!newp) newp = nodep->fromp()->unlink_Fr_వెనుక();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        ఒకవేళ (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, అబద్ధం, ప్రవాహం_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_కణుపు_సమాసం* స్థిరరాశి underp = nodep->fromp()->unlink_Fr_వెనుక();
            underp->dtypeFrom(nodep);
            underp->did_వెడల్పు(నిజం);
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstCastWrap{nodep->fileline(), underp};
            newp->did_వెడల్పు(నిజం);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(Ast_Cast_కొలత* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            పూర్ణాంకం వెడల్పు = nodep->rhsp()->గమ్యం_SInt();
            ఒకవేళ (వెడల్పు < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << వెడల్పు);
                వెడల్పు = 1;
            }
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, PRELIM, ప్రవాహం_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), వెడల్పు);  // lhsp may change
        }
        ఒకవేళ (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_కణుపు* స్థిరరాశి underp = nodep->lhsp()->unlink_Fr_వెనుక();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    శూన్య సందర్శించు(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{nodep->dtypep(), BOTH, ప్రవాహం_USE_CAST}.p());
    }
    శూన్య castSized(Ast_కణుపు* nodep, Ast_కణుపు* underp, పూర్ణాంకం వెడల్పు) {
        స్థిరరాశి AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        ఒకవేళ (!underDtp) underDtp = underp->dtypep()->basicp();
        ఒకవేళ (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->కనుగొను_బిట్టు_DType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            స్థిరరాశి పూర్ణాంకం calc_వెడల్పు = std::max(వెడల్పు, underDtp->వెడల్పు());
            Ast_కణుపు_DType* స్థిరరాశి calcDtp = nodep->కనుగొను_బిట్టు_Or_Logic_DType(
                calc_వెడల్పు, calc_వెడల్పు, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         అబద్ధం, ప్రవాహం_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_కణుపు_DType* స్థిరరాశి బహిర్గతం_Dtp = nodep->కనుగొను_బిట్టు_Or_Logic_DType(
                వెడల్పు, వెడల్పు, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(బహిర్గతం_Dtp);
            // We ignore warnings as that is sort of the point of a cast
            వెడల్పు_Check_Sized(nodep, "Cast expr", VN_AS(underp, కణుపు_సమాసం), బహిర్గతం_Dtp, EXTEND_EXP,
                            అబద్ధం);
            VL_DANGLING(underp);
        }
    }
    శూన్య సందర్శించు(AstConstraintBefore* nodep) override {
        user_Iterate_And_తదుపరి(nodep->lhssp(), వెడల్పు_VP{SELF, BOTH}.p());
        user_Iterate_And_తదుపరి(nodep->rhssp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(Ast_Constraint_సమాసం* nodep) override {
        user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstConstraintUnique* nodep) override {
        user_Iterate_And_తదుపరి(nodep->rangesp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(Ast_Dist_అంశం* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), వెడల్పు_VP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        ఒకవేళ (nodep->doing_వెడల్పు()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            నెట్టు_Deletep(nodep->valuep()->unlink_Fr_వెనుక());
            nodep->valuep(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->did_వెడల్పు(నిజం);
            తిరిగి;
        }
        nodep->doing_వెడల్పు(నిజం);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        ఒకవేళ (!m_paramsOnly && nodep->is_Iface_Array_విభజించు()) m_విభజించు_Iface_Array_Varps.నెట్టు_వెనుక(nodep);
        ఒకవేళ (nodep->attrsp()) {
            nodep->attrsp()->foreach([ఇది, nodep](Ast_Attr_యొక్క* attrp) {
                ఒకవేళ (attrp->attrType() == VAttrType::VAR_పోర్టు_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    ఒకవేళ (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), కణుపు_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(నెట్టు_Deletep(attrp->unlink_Fr_వెనుక()), attrp);
                }
            });
        }
        ఒకవేళ (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_కణుపు_DType* dtp = nodep->dtypep();
            Ast_కణుపు_DType* np = శూన్యసూచిక;
            అయితే (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                ఒకవేళ (స్థిరరాశి AstDynArrayDType* స్థిరరాశి unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    ఒకవేళ (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } లేకపోతే {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* స్థిరరాశి newp
                        = కొత్త AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    ఒకవేళ (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } లేకపోతే {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->కూడు_Typesp(newp);
                    np = newp;
                } లేకపోతే {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        ఒకవేళ (AstWildcardArrayDType* స్థిరరాశి wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        ఒకవేళ (VN_IS(nodep->dtypep()->skip_Ref_గమ్యం_Constp(), ConstDType)) nodep->isConst(నిజం);
        // Parameters if implicit untyped inherit from what they are assigned to
        స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        తార్కిక didchk = అబద్ధం;
        స్థిరరాశి తార్కిక implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        ఒకవేళ (implicitParam) {
            ఒకవేళ (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                user_Iterate_And_తదుపరి(nodep->valuep(), వెడల్పు_VP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                ఒకవేళ (nodep->valuep()->isDouble()) {
                    nodep->dtype_సమితి_Double();
                    VL_DANGLING(bdtypep);
                } లేకపోతే ఒకవేళ (nodep->valuep()->isString()) {
                    nodep->dtype_సమితి_String();
                    VL_DANGLING(bdtypep);
                } లేకపోతే {
                    పూర్ణాంకం వెడల్పు = 0;
                    Ast_కణుపు_DType* స్థిరరాశి విలువ_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(విలువ_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    స్థిరరాశి AstBasicDType* స్థిరరాశి విలువ_Bdtypep = విలువ_DTypep->basicp();
                    తార్కిక issigned = అబద్ధం;
                    ఒకవేళ (bdtypep->isNosign()) {
                        ఒకవేళ (విలువ_Bdtypep && విలువ_Bdtypep->isSigned()) issigned = నిజం;
                    } లేకపోతే {
                        issigned = bdtypep->isSigned();
                    }
                    ఒకవేళ (విలువ_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        ఒకవేళ (స్థిరరాశి స్వయంచాలక* స్థిరరాశి constp = VN_CAST(nodep->valuep(), Const)) {
                            ఒకవేళ (constp->అంకె().isString()) {
                                వెడల్పు = constp->అంకె().గమ్యం_String().నిడివి() * 8;
                            }
                        }
                        ఒకవేళ (వెడల్పు < 8) వెడల్పు = 8;
                    } లేకపోతే ఒకవేళ (nodep->valuep()->dtypep()->వెడల్పు_Sized()) {
                        వెడల్పు = nodep->valuep()->వెడల్పు();
                    } లేకపోతే {
                        ఒకవేళ (nodep->valuep()->వెడల్పు() > 32) {
                            nodep->valuep()->v3warn(
                                వెడల్పు_2,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        వెడల్పు = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    స్థిరరాశి తార్కిక is_విలువ_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    ఒకవేళ (వెడల్పు == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->కనుగొను_బిట్టు_Or_Logic_Range_DType(
                            VNumRange{0, 0}, nodep->valuep()->వెడల్పు_Min(),
                            VSigning::fromBool(issigned), is_విలువ_Fourstate));
                    } లేకపోతే {
                        nodep->dtype_సమితి_బిట్టు_Or_Logic_Unsized(వెడల్పు, nodep->valuep()->వెడల్పు_Min(),
                                                         VSigning::fromBool(issigned),
                                                         is_విలువ_Fourstate);
                    }
                    didchk = నిజం;
                }
                iterate_Check_కేటాయించు(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } లేకపోతే {
                // Or, if nothing assigned, they're integral
                nodep->dtype_సమితి_Integer();
                VL_DANGLING(bdtypep);
            }
        } లేకపోతే ఒకవేళ (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtype_సమితి_Logic_Sized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        ఒకవేళ (nodep->isNet()) {
            Ast_కణుపు_DType* స్థిరరాశి badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            ఒకవేళ (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        ఒకవేళ (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            user_Iterate_And_తదుపరి(nodep->valuep(), వెడల్పు_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_కేటాయించు(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_తదుపరి(nodep->delayp(), వెడల్పు_VP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->did_వెడల్పు(నిజం);
        nodep->doing_వెడల్పు(అబద్ధం);
    }
    శూన్య సందర్శించు(Ast_కణుపు_Var_Ref* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        ఒకవేళ (!nodep->varp()) {
            ఒకవేళ (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, అబద్ధం,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                తిరిగి;
            } లేకపోతే {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        ఒకవేళ (!nodep->varp()->did_వెడల్పు()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), శూన్యసూచిక);
        }
        ఒకవేళ (!m_paramsOnly && nodep->varp()->is_Iface_Array_విభజించు()) {
            replace_విభజించు_Iface_Array_Ref(nodep);
            తిరిగి;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        ఒకవేళ (VN_IS(nodep->backp(), కణుపు_కేటాయించు) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        ఒకవేళ (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(నిజం);
        ఒకవేళ (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } లేకపోతే ఒకవేళ (nodep->access().isWriteOrRW() && nodep->varp()->is_ఆదానం()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), కేటాయించు_Force) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            స్థిరరాశి తార్కిక hierRef = !m_curModVars.లెక్క(nodep->varp());
            స్థిరరాశి తార్కిక net_పోర్టు
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::పోర్టు;
            స్థిరరాశి తార్కిక cont_కేటాయించు = VN_IS(nodep->abovep(), కేటాయించు_W);
            స్థిరరాశి తార్కిక pinConn = VN_IS(nodep->abovep(), Pin);
            ఒకవేళ (!(hierRef && net_పోర్టు && (cont_కేటాయించు || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } లేకపోతే ఒకవేళ (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        ఒకవేళ (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->did_వెడల్పు(నిజం);
    }

    శూన్య సందర్శించు(AstEnumDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_కణుపు_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_కణుపు_DType* స్థిరరాశి badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        ఒకవేళ (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->కనుగొను_Int_DType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->వెడల్పు_From_Sub(nodep->subDTypep());
        // Assign widths
        user_Iterate_And_తదుపరి(nodep->itemsp(), వెడల్పు_VP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_సంఖ్య అంకె(nodep, nodep->వెడల్పు(), 0);
        స్థిరరాశి V3_సంఖ్య one{nodep, nodep->వెడల్పు(), 1};
        తార్కిక wrapAround = అబద్ధం;
        std::map<స్థిరరాశి V3_సంఖ్య, Ast_Enum_అంశం*> inits;
        కొరకు (Ast_Enum_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
            ఒకవేళ (itemp->valuep()) {
                ఒకవేళ (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_వృక్షం("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                ఒకవేళ (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlink_Fr_వెనుక()->delete_వృక్షం();
                    కొనసాగు;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = అబద్ధం;  // Explicit value resets wrap-around tracking
            }
            ఒకవేళ (!itemp->valuep()) {
                ఒకవేళ (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                ఒకవేళ (అంకె.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(కొత్త AstConst{itemp->fileline(), అంకె});
            }

            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(itemp->valuep(), Const);
            ఒకవేళ (constp->అంకె().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            అంకె.op_కేటాయించు(constp->అంకె());
            // Look for duplicates
            స్థిరరాశి స్వయంచాలక pair = inits.emplace(అంకె, itemp);
            ఒకవేళ (!pair.రెండవ) {  // IEEE says illegal
                స్థిరరాశి Ast_కణుపు* స్థిరరాశి otherp = pair.మొదటి->రెండవ;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            స్థిరరాశి తార్కిక prevNeg = constp->అంకె().isNegative();
            అంకె.op_కూడు(one, constp->అంకె());
            ఒకవేళ (basicp->isSigned()) {
                wrapAround = !prevNeg && అంకె.isNegative();
            } లేకపోతే {
                wrapAround = అంకె.isEqZero();
            }
        }
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_కణుపు_DType* స్థిరరాశి vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        ఒకవేళ (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_తదుపరి(nodep->valuep(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            తార్కిక warnOn = నిజం;
            Ast_కణుపు_సమాసం* valuep = nodep->valuep();
            ఒకవేళ (స్థిరరాశి Ast_కూడు* స్థిరరాశి anodep = VN_CAST(valuep, కూడు_2)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                ఒకవేళ (anodep->fileline()->equal_మొదటి_పంక్తి_నిలువు(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(valuep, Const)) {
                ఒకవేళ (static_cast<పూర్ణాంకం>(constp->అంకె().most_సమితి_బిట్టు_P1()) > nodep->వెడల్పు()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = అబద్ధం;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(కొత్త AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlink_Fr_వెనుక(),
                                      nodep->dtypep()});
        }
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం_Ref* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        ఒకవేళ (!nodep->itemp()->did_వెడల్పు()) {
            // We need to do the whole enum en masse
            Ast_కణుపు* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            కొరకు (; enump; enump = enump->backp()) {
                ఒకవేళ (VN_IS(enump, EnumDType)) ఆపు;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cons* nodep) override {}  // Widthed when created
    శూన్య సందర్శించు(Ast_కణుపు_Cons_అంశం* nodep) override {}  // Widthed when created
    శూన్య సందర్శించు(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
        కొరకు (Ast_కణుపు *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, వెడల్పు_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_కణుపు_DType* subDTypep = శూన్యసూచిక;
        nodep->dtype_సమితి_బిట్టు();

        ఒకవేళ (dtype && dtype->isString()) {
            nodep->dtype_సమితి_String();
            subDTypep = nodep->కనుగొను_String_DType();
        } లేకపోతే ఒకవేళ (dtype && dtype->isDouble()) {
            nodep->dtype_సమితి_Double();
            subDTypep = nodep->కనుగొను_Double_DType();
        } లేకపోతే {
            // Take width as maximum across all items
            పూర్ణాంకం వెడల్పు = nodep->exprp()->వెడల్పు();
            పూర్ణాంకం mwidth = nodep->exprp()->వెడల్పు_Min();
            తార్కిక isFourstate = nodep->exprp()->dtypep()->isFourstate();
            కొరకు (స్థిరరాశి Ast_కణుపు* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                వెడల్పు = std::max(వెడల్పు, itemp->వెడల్పు());
                mwidth = std::max(mwidth, itemp->వెడల్పు_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->కనుగొను_బిట్టు_Or_Logic_DType(
                వెడల్పు, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        కొరకు (Ast_కణుపు *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, Dist_అంశం)->rangep();
            ఒకవేళ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, వెడల్పు_VP{subDTypep, FINAL}.p());
            } లేకపోతే {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        ఒకవేళ (m_constraintp) {
            తార్కిక can_క్రింది = నిజం;
            కొరకు (Ast_Dist_అంశం* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), Dist_అంశం)) {
                ఒకవేళ (VN_IS(ditemp->rangep(), InsideRange)) కొనసాగు;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి అంశం_Dtp = ditemp->rangep()->dtypep()->skipRefp();
                ఒకవేళ (!అంశం_Dtp->isIntegralOrPacked() || !ditemp->rangep()->is_శుద్ధ()) {
                    can_క్రింది = అబద్ధం;
                    ఆపు;
                }
            }
            ఒకవేళ (can_క్రింది) తిరిగి;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
        కొరకు (Ast_Dist_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Dist_అంశం)) {
            Ast_కణుపు_సమాసం* inewp
                = inside_అంశం(nodep, nodep->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం), itemp->rangep());
            ఒకవేళ (!inewp) కొనసాగు;
            Ast_కణుపు_సమాసం* స్థిరరాశి cmpp
                = కొత్త AstGt{itemp->fileline(), itemp->weightp()->unlink_Fr_వెనుక(),
                            కొత్త AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3_Error_సంకేతం::UNSIGNED, నిజం);
            cmpp->fileline()->modifyWarnOff(V3_Error_సంకేతం::CMPCONST, నిజం);
            inewp = కొత్త AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? కొత్త AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ఒకవేళ (!newp) newp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    శూన్య సందర్శించు(AstInside* nodep) override {
        user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
        కొరకు (Ast_కణుపు *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, వెడల్పు_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_కణుపు_DType* expDTypep = శూన్యసూచిక;

        ఒకవేళ (dtype && dtype->isString()) {
            nodep->dtype_సమితి_String();
            expDTypep = nodep->కనుగొను_String_DType();
        } లేకపోతే ఒకవేళ (dtype && dtype->isDouble()) {
            nodep->dtype_సమితి_Double();
            expDTypep = nodep->కనుగొను_Double_DType();
        } లేకపోతే {
            // Take width as maximum across all items
            పూర్ణాంకం వెడల్పు = nodep->exprp()->వెడల్పు();
            పూర్ణాంకం mwidth = nodep->exprp()->వెడల్పు_Min();
            తార్కిక isFourstate = nodep->exprp()->dtypep()->isFourstate();
            కొరకు (స్థిరరాశి Ast_కణుపు* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                వెడల్పు = std::max(వెడల్పు, itemp->వెడల్పు());
                mwidth = std::max(mwidth, itemp->వెడల్పు_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtype_సమితి_బిట్టు();
            స్థిరరాశి VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->కనుగొను_బిట్టు_Or_Logic_DType(వెడల్పు, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        కొరకు (Ast_కణుపు *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            ఒకవేళ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, వెడల్పు_VP{expDTypep, FINAL}.p());
            } లేకపోతే ఒకవేళ (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_కణుపు_సమాసం* exprp;
        Ast_సమాసం_Stmt* సమాసం_Stmtp = శూన్యసూచిక;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        ఒకవేళ (!m_constraintp && !nodep->exprp()->is_శుద్ధ()) {
            File_పంక్తి* స్థిరరాశి fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_కణుపు_DType* స్థిరరాశి సమాసం_Dtp = nodep->exprp()->dtypep();
            స్థిరరాశి పూర్ణాంకం w = సమాసం_Dtp->వెడల్పు();
            Ast_కణుపు_DType* స్థిరరాశి తాత్కాలిక_DTypep
                = సమాసం_Dtp->వెడల్పు_Sized() ? సమాసం_Dtp
                                        : nodep->కనుగొను_బిట్టు_Or_Logic_DType(w, w, సమాసం_Dtp->numeric(),
                                                                     సమాసం_Dtp->isFourstate());
            AstVar* స్థిరరాశి varp
                = కొత్త AstVar{fl, VVarType::XTEMP, m_inside_తాత్కాలిక_పేర్లు.get(nodep), తాత్కాలిక_DTypep};
            exprp = కొత్త AstVarRef{fl, varp, VAccess::READ};
            సమాసం_Stmtp = కొత్త Ast_సమాసం_Stmt{fl,
                                        కొత్త Ast_కేటాయించు{fl, కొత్త AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlink_Fr_వెనుక()},
                                        exprp};
            ఒకవేళ (m_ftaskp) {
                varp->funcLocal(నిజం);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } లేకపోతే {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(సమాసం_Stmtp);
        } లేకపోతే {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
        కొరకు (Ast_కణుపు_సమాసం *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), కణుపు_సమాసం);  // Will be unlinking
            Ast_కణుపు_సమాసం* inewp;
            ఒకవేళ (సమాసం_Stmtp) {
                inewp = inside_అంశం(nodep, సమాసం_Stmtp, itemp);
                సమాసం_Stmtp = శూన్యసూచిక;
            } లేకపోతే {
                inewp = inside_అంశం(nodep, exprp->clone_వృక్షం_శుద్ధ(అబద్ధం), itemp);
            }
            ఒకవేళ (!inewp) కొనసాగు;
            newp = newp ? కొత్త AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ఒకవేళ (సమాసం_Stmtp) VL_DO_DANGLING(సమాసం_Stmtp->delete_వృక్షం(), సమాసం_Stmtp);
        ఒకవేళ (!newp) newp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    Ast_కణుపు_సమాసం* inside_అంశం(Ast_కణుపు* nodep, Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_సమాసం* itemp) {
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి అంశం_Dtp = itemp->dtypep()->skipRefp();
        ఒకవేళ (AstInsideRange* స్థిరరాశి irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            తిరిగి irangep->newAndFromInside(exprp, irangep->lhsp()->unlink_Fr_వెనుక(),
                                             irangep->rhsp()->unlink_Fr_వెనుక());
        } లేకపోతే ఒకవేళ (VN_IS(అంశం_Dtp, UnpackArrayDType) || VN_IS(అంశం_Dtp, DynArrayDType)
                   || VN_IS(అంశం_Dtp, వరుస_DType)) {
            // Unsupported in parameters
            Ast_కణుపు_సమాసం* స్థిరరాశి inewp = కొత్త AstCMethodHard{nodep->fileline(), itemp->unlink_Fr_వెనుక(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, అంశం_Dtp->subDTypep(), BOTH);
            inewp->dtype_సమితి_బిట్టు();
            inewp->did_వెడల్పు(నిజం);
            తిరిగి inewp;
        } లేకపోతే ఒకవేళ (VN_IS(అంశం_Dtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_వృక్షం(), exprp);
            తిరిగి శూన్యసూచిక;
        }
        తిరిగి AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlink_Fr_వెనుక());
    }
    శూన్య సందర్శించు(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        ఒకవేళ (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } లేకపోతే {
            ఒకవేళ (m_vup->prelim()) {
                user_Iterate_And_తదుపరి(nodep->lhsp(), m_vup);
                user_Iterate_And_తదుపరి(nodep->rhsp(), m_vup);
            }
            ఒకవేళ (m_vup->final()) {
                Ast_కణుపు_DType* స్థిరరాశి expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                స్థిరరాశి పూర్ణాంకం exp_వెడల్పు = expDTypep->వెడల్పు();
                స్థిరరాశి తార్కిక waiveLhs = exp_వెడల్పు == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->వెడల్పు_Min() >= nodep->lhsp()->వెడల్పు();
                స్థిరరాశి తార్కిక waiveRhs = exp_వెడల్పు == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && exp_వెడల్పు >= nodep->rhsp()->వెడల్పు_Min();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        ఒకవేళ (nodep->isVirtual()) nodep->iface_ద్వారా_Cellp()->సమితి_Has_Virtual_Ref();
        UINFO(4, "dtWidthed " << nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {
        ఒకవేళ (nodep->doing_వెడల్పు()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtype_సమితి_బిట్టు();
            nodep->doing_వెడల్పు(అబద్ధం);
            తిరిగి;
        }
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_వెడల్పు(నిజం);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        ఒకవేళ (స్థిరరాశి AstUnionDType* స్థిరరాశి unionp = VN_CAST(nodep, UnionDType)) {
            ఒకవేళ (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, శూన్యసూచిక);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(అబద్ధం);
        // Error checks
        కొరకు (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_కణుపు_DType* స్థిరరాశి dtp = itemp->subDTypep()->skipRefp();
            ఒకవేళ (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            ఒకవేళ ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                నెట్టు_Deletep(itemp->valuep()->unlink_Fr_వెనుక());
            }
        }
        స్థిరరాశి తార్కిక isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        స్థిరరాశి తార్కిక లోపల_Template_Module = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        ఒకవేళ (VN_IS(nodep, UnionDType) || nodep->packed()) {
            పూర్ణాంకం lsb = 0;
            పూర్ణాంకం వెడల్పు = 0;
            తార్కిక మొదటి = నిజం;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            కొరకు (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            కొరకు (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                ఒకవేళ (itemp->isFourstate()) nodep->isFourstate(నిజం);
                itemp->lsb(lsb);
                ఒకవేళ (VN_IS(nodep, UnionDType)) {
                    స్థిరరాశి పూర్ణాంకం అంశం_వెడల్పు = itemp->వెడల్పు();
                    // Skip union size check for template modules with unresolved parameters
                    ఒకవేళ (!మొదటి && isHardPackedUnion && అంశం_వెడల్పు != వెడల్పు && !లోపల_Template_Module) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    వెడల్పు = std::max(వెడల్పు, అంశం_వెడల్పు);
                } లేకపోతే {
                    lsb += itemp->వెడల్పు();
                    వెడల్పు += itemp->వెడల్పు();
                }
                మొదటి = అబద్ధం;
            }
            nodep->వెడల్పు_Force(వెడల్పు, వెడల్పు);  // Signing stays as-is, as parsed from declaration
        } లేకపోతే {
            nodep->వెడల్పు_Force(1, 1);
        }
        nodep->doing_వెడల్పు(అబద్ధం);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    శూన్య సందర్శించు(AstThisRef* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->బిడ్డ_DTypep()));
        ఒకవేళ (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    శూన్య సందర్శించు(AstClassRefDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        userIterateChildren(nodep, శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warn_మరిన్ని() << "... Suggest use a typedef");
        nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}});
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    శూన్య సందర్శించు(AstClassExtends* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        ఒకవేళ (VN_IS(nodep->బిడ్డ_DTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->బిడ్డ_DTypep()));
        }
    }
    శూన్య సందర్శించు(AstMemberDType* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->వెడల్పు_From_Sub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        ఒకవేళ (nodep->valuep()) {
            user_Iterate_And_తదుపరి(nodep->valuep(), వెడల్పు_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_కేటాయించు(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    శూన్య సందర్శించు(AstStructSel* nodep) override {
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = నిజం;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skip_Ref_గమ్యం_Enump();
        UINFO(9, "     from dt " << fromDtp);
        స్థిరరాశి AstMemberSel* స్థిరరాశి fromSel = VN_CAST(nodep->fromp(), MemberSel);
        ఒకవేళ (AstClocking* స్థిరరాశి clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->మొదటి_Abovep(), Clocking)
                                               : శూన్యసూచిక) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            ఒకవేళ (AstVar* స్థిరరాశి varp = memberSelClocking(nodep, clockingp)) {
                ఒకవేళ (!varp->did_వెడల్పు()) userIterate(varp, శూన్యసూచిక);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlink_Fr_వెనుక());
                VL_DO_DANGLING(fromp->delete_వృక్షం(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                ఒకవేళ (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::link_LValue_సమితి(nodep, nodep->access());
                }
                ఒకవేళ (AstIfaceRefDType* స్థిరరాశి adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->did_వెడల్పు(నిజం);  // Must not visit again: will confuse scopes
                తిరిగి;
            }
        } లేకపోతే ఒకవేళ (Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి adtypep = VN_CAST(fromDtp, కణుపు_UOr_Struct_DType)) {
            ఒకవేళ (memberSelStruct(nodep, adtypep)) తిరిగి;
        } లేకపోతే ఒకవేళ (AstClassRefDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            ఒకవేళ (memberSelClass(nodep, adtypep)) తిరిగి;
        } లేకపోతే ఒకవేళ (AstIfaceRefDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            ఒకవేళ (Ast_కణుపు* foundp = memberSelIface(nodep, adtypep)) {
                ఒకవేళ (AstClocking* స్థిరరాశి clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(foundp, Var)) {
                    ఒకవేళ (!varp->did_వెడల్పు()) userIterate(varp, శూన్యసూచిక);
                    AstIface* స్థిరరాశి ifacep = adtypep->iface_ద్వారా_Cellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    స్థిరరాశి AstVarRef* స్థిరరాశి refp = VN_CAST(nodep->fromp(), VarRef);
                    ఒకవేళ (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* స్థిరరాశి newp = కొత్త AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->did_వెడల్పు(నిజం);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                        తిరిగి;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_వెడల్పు(నిజం);
                    తిరిగి;
                }
                ఒకవేళ (AstModport* స్థిరరాశి modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* స్థిరరాశి newDtypep = కొత్త AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->కూడు_Typesp(newDtypep);
                    Ast_కణుపు_సమాసం* స్థిరరాశి fromp = nodep->fromp()->unlink_Fr_వెనుక();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
                ఒకవేళ (AstCell* స్థిరరాశి cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    ఒకవేళ (VN_IS(cellp->modp(), Iface)) {
                        స్థిరరాశి string viftopName = cellp->name() + "__Viftop";
                        Ast_కణుపు_Module* స్థిరరాశి మాతృ_Ifacep = adtypep->iface_ద్వారా_Cellp();
                        AstVar* viftopVarp = శూన్యసూచిక;
                        కొరకు (Ast_కణుపు* itemp = మాతృ_Ifacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            ఒకవేళ (AstVar* స్థిరరాశి vp = VN_CAST(itemp, Var)) {
                                ఒకవేళ (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    ఆపు;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        ఒకవేళ (!viftopVarp->did_వెడల్పు()) userIterate(viftopVarp, శూన్యసూచిక);
                        Ast_కణుపు_DType* subDtypep = viftopVarp->dtypep();
                        ఒకవేళ (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            ఒకవేళ (AstIfaceRefDType* స్థిరరాశి subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* స్థిరరాశి newDtypep = కొత్త AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(నిజం);
                                v3Global.rootp()->typeTablep()->కూడు_Typesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->did_వెడల్పు(నిజం);
                        తిరిగి;
                    }
                }
                ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp = VN_CAST(foundp, కణుపు_FTask)) {
                    Ast_పద్ధతి_Call* స్థిరరాశి newp = కొత్త Ast_పద్ధతి_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    తిరిగి;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } లేకపోతే ఒకవేళ (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, వరుస_DType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_కణుపు* స్థిరరాశి newp = కొత్త Ast_పద్ధతి_Call{nodep->fileline(),
                                                    nodep->fromp()->unlink_Fr_వెనుక(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            తిరిగి;
        } లేకపోతే {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }
    తార్కిక memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        ఒకవేళ (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            Ast_పద్ధతి_Call* స్థిరరాశి newp = కొత్త Ast_పద్ధతి_Call{
                nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            సందర్శించు(newp);
            తిరిగి నిజం;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* స్థిరరాశి మొదటి_classp = adtypep->classp();
        UASSERT_OBJ(మొదటి_classp, nodep, "Unlinked");
        కొరకు (AstClass* classp = మొదటి_classp; classp;) {
            ఒకవేళ (Ast_కణుపు* స్థిరరాశి foundp = m_memberMap.కనుగొను_Member(classp, nodep->name())) {
                ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(foundp, Var)) {
                    ఒకవేళ (!varp->did_వెడల్పు()) userIterate(varp, శూన్యసూచిక);
                    ఒకవేళ (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* స్థిరరాశి varRefp
                            = కొత్త AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                        తిరిగి నిజం;
                    }
                    ఒకవేళ (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_వెడల్పు(నిజం);
                    ఒకవేళ (nodep->fromp()->same_వృక్షం(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::link_LValue_సమితి(nodep);
                    తిరిగి నిజం;
                }
                ఒకవేళ (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(కొత్త AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(), constrp});
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి నిజం;
                }
                ఒకవేళ (Ast_Enum_అంశం_Ref* స్థిరరాశి adfoundp = VN_CAST(foundp, Enum_అంశం_Ref)) {
                    nodep->replaceWith(adfoundp->clone_వృక్షం(అబద్ధం));
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి నిజం;
                }
                ఒకవేళ (Ast_కణుపు_FTask* ftaskp = VN_CAST(foundp, కణుపు_FTask)) {
                    Ast_పద్ధతి_Call* newp = కొత్త Ast_పద్ధతి_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి నిజం;
                }
                ఒకవేళ (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->కనుగొను_Constraint_Ref_DType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    తిరిగి నిజం;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : శూన్యసూచిక;
        }

        VSpellCheck speller;
        కొరకు (AstClass* classp = మొదటి_classp; classp;) {
            కొరకు (Ast_కణుపు* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                ఒకవేళ (VN_IS(itemp, Constraint) || VN_IS(itemp, Enum_అంశం_Ref) || VN_IS(itemp, Var)) {
                    speller.నెట్టు_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : శూన్యసూచిక;
        }
        స్థిరరాశి string suggest = speller.ఉత్తమ_Candidate_సందేశం(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << మొదటి_classp->verilogKwd()
                      << " " << మొదటి_classp->prettyNameQ() << "\n"
                      << (suggest.ఖాళీలు() ? "" : nodep->fileline()->warn_మరిన్ని() + suggest));
        తిరిగి అబద్ధం;  // Caller handles error
    }
    తార్కిక modport_Allows_అంశం(Ast_కణుపు* nodep, స్థిరరాశి AstIfaceRefDType* adtypep, స్థిరరాశి Ast_కణుపు* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        స్థిరరాశి AstModport* స్థిరరాశి modportp = adtypep->modportp();
        ఒకవేళ (!modportp) తిరిగి నిజం;
        ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(itemp, Var)) {
            ఒకవేళ (varp->isParam()) తిరిగి నిజం;
        } లేకపోతే ఒకవేళ (!VN_IS(itemp, కణుపు_FTask) && !VN_IS(itemp, Clocking)) {
            తిరిగి నిజం;
        }
        స్వయంచాలక pair = m_modport_పేర్లు.emplace(modportp, std::unordered_సమితి<string>{});
        ఒకవేళ (pair.రెండవ) {
            కొరకు (స్థిరరాశి Ast_కణుపు* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.మొదటి->రెండవ.చొప్పించు(mpItemp->name());
            }
        }
        ఒకవేళ (pair.మొదటి->రెండవ.లెక్క(itemp->name())) తిరిగి నిజం;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->iface_ద్వారా_Cellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        తిరిగి అబద్ధం;
    }
    Ast_కణుపు* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_కణుపు_Module* స్థిరరాశి ifacep = adtypep->iface_ద్వారా_Cellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        కొరకు (Ast_కణుపు* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (itemp->name() == nodep->name()) {
                ఒకవేళ (!modport_Allows_అంశం(nodep, adtypep, itemp)) {
                    తిరిగి శూన్యసూచిక;  // Caller handles error
                }
                తిరిగి itemp;
            }
            ఒకవేళ (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.నెట్టు_Candidate(itemp->prettyName());
            }
        }
        స్థిరరాశి string suggest = speller.ఉత్తమ_Candidate_సందేశం(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.ఖాళీలు() ? "" : nodep->fileline()->warn_మరిన్ని() + suggest));
        తిరిగి శూన్యసూచిక;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        కొరకు (Ast_కణుపు* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (Ast_Clocking_అంశం* citemp = VN_CAST(itemp, Clocking_అంశం)) {
                ఒకవేళ (citemp->varp()->name() == nodep->name()) తిరిగి citemp->varp();
                speller.నెట్టు_Candidate(citemp->varp()->prettyName());
            }
        }
        స్థిరరాశి string suggest = speller.ఉత్తమ_Candidate_సందేశం(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.ఖాళీలు() ? "" : nodep->fileline()->warn_మరిన్ని() + suggest));
        తిరిగి శూన్యసూచిక;  // Caller handles error
    }
    తార్కిక memberSelStruct(AstMemberSel* nodep, Ast_కణుపు_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        ఒకవేళ (AstMemberDType* స్థిరరాశి memberp
            = VN_CAST(m_memberMap.కనుగొను_Member(adtypep, nodep->name()), MemberDType)) {
            ఒకవేళ (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } లేకపోతే ఒకవేళ (adtypep->packed()) {
                AstSel* స్థిరరాశి newp = కొత్త AstSel{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                                memberp->lsb(), memberp->వెడల్పు()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_గమ్యం_Enump());
                newp->did_వెడల్పు(నిజం);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } లేకపోతే {
                AstStructSel* స్థిరరాశి newp = కొత్త AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_గమ్యం_Enump());
                newp->did_వెడల్పు(నిజం);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            తిరిగి నిజం;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        తిరిగి అబద్ధం;
    }

    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    శూన్య సందర్శించు(Ast_పద్ధతి_Call* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), వెడల్పు_VP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_కణుపు_DType* స్థిరరాశి fromDtp = nodep->fromp()->dtypep()->skip_Ref_గమ్యం_Enump();
        AstBasicDType* స్థిరరాశి basicp = fromDtp ? fromDtp->basicp() : శూన్యసూచిక;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, వెడల్పు_VP{SELF, BOTH}.p());
        ఒకవేళ (nodep->name() == "rand_mode") {
            పద్ధతి_Call_Rand_బహుళకం(nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "constraint_mode") {
            పద్ధతి_Call_Constraint(nodep, శూన్యసూచిక);
        } లేకపోతే ఒకవేళ (AstEnumDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, EnumDType)) {
            పద్ధతి_Call_Enum(nodep, adtypep);
        } లేకపోతే ఒకవేళ (AstAssocArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            పద్ధతి_Call_Assoc(nodep, adtypep);
        } లేకపోతే ఒకవేళ (AstWildcardArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            పద్ధతి_Call_Wildcard(nodep, adtypep);
        } లేకపోతే ఒకవేళ (AstDynArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            పద్ధతి_Call_Dyn(nodep, adtypep);
        } లేకపోతే ఒకవేళ (Ast_వరుస_DType* స్థిరరాశి adtypep = VN_CAST(fromDtp, వరుస_DType)) {
            పద్ధతి_Call_వరుస(nodep, adtypep);
        } లేకపోతే ఒకవేళ (AstClassRefDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            పద్ధతి_Call_Class(nodep, adtypep);
        } లేకపోతే ఒకవేళ (AstIfaceRefDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            పద్ధతి_Call_Iface_Ref(nodep, adtypep);
        } లేకపోతే ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            పద్ధతి_Call_Unpack(nodep, adtypep);
        } లేకపోతే ఒకవేళ (AstConstraintRefDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            పద్ధతి_Call_Constraint(nodep, adtypep);
        } లేకపోతే ఒకవేళ (basicp && basicp->isEvent()) {
            పద్ధతి_Call_Event(nodep, basicp);
        } లేకపోతే ఒకవేళ (basicp && basicp->isString()) {
            పద్ధతి_Call_String(nodep, basicp);
        } లేకపోతే {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtype_సమితి_Void();
        }
    }
    AstWith* పద్ధతి_With_Clause(Ast_కణుపు_FTask_Ref* nodep, తార్కిక required, తార్కిక arbReturn,
                              Ast_కణుపు_DType* returnDtp, Ast_కణుపు_DType* సూచీ_Dtp,
                              Ast_కణుపు_DType* విలువ_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        ఒకవేళ (AstWith* స్థిరరాశి withp = nodep->withp()) {
            withp->సూచీ_Arg_Refp()->dtypep(సూచీ_Dtp);
            withp->విలువ_Arg_Refp()->dtypep(విలువ_Dtp);
            userIterate(withp, వెడల్పు_VP{returnDtp, BOTH}.p());
            withp->unlink_Fr_వెనుక();
            తిరిగి withp;
        }
        ఒకవేళ (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        తిరిగి శూన్యసూచిక;
    }
    శూన్య పద్ధతి_Ok_Arguments(Ast_కణుపు_FTask_Ref* nodep, పూర్ణాంకం minArg, పూర్ణాంకం maxArg) {
        పూర్ణాంకం narg = 0;
        ఒకవేళ (AstWith* స్థిరరాశి withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(నెట్టు_Deletep(withp->unlink_Fr_వెనుక()), withp);
        }
        కొరకు (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        స్థిరరాశి తార్కిక ok = (narg >= minArg) && (narg <= maxArg);
        ఒకవేళ (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvt_గమ్యం_Str(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvt_గమ్యం_Str(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            కొరకు (; narg < minArg; ++narg) {
                nodep->కూడు_Argsp(
                    కొత్త AstArg{nodep->fileline(), "",
                               కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}}});
            }
            కొరకు (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                అయితే (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlink_Fr_వెనుక();
                VL_DO_DANGLING(argp->delete_వృక్షం(), argp);
            }
        }
    }

    Ast_కణుపు_సమాసం* పద్ధతి_Arg(Ast_పద్ధతి_Call* nodep, పూర్ణాంకం arg) {
        AstArg* argp = nodep->argsp();
        కొరకు (పూర్ణాంకం narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        తిరిగి argp->exprp();
    }

    శూన్య పద్ధతి_Call_Enum(Ast_పద్ధతి_Call* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        ఒకవేళ (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = శూన్యసూచిక;
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            ఒకవేళ (nodep->name() == "num") {
                పూర్ణాంకం వస్తువులు = 0;
                కొరకు (Ast_కణుపు* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++వస్తువులు;
                newp = కొత్త AstConst(nodep->fileline(), AstConst::Signed32{}, వస్తువులు);
            } లేకపోతే ఒకవేళ (nodep->name() == "first") {
                స్థిరరాశి Ast_Enum_అంశం* itemp = adtypep->itemsp();
                ఒకవేళ (!itemp) {
                    newp = కొత్త AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } లేకపోతే {
                    newp = VN_AS(itemp->valuep()->clone_వృక్షం(అబద్ధం), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } లేకపోతే ఒకవేళ (nodep->name() == "last") {
                స్థిరరాశి Ast_Enum_అంశం* itemp = adtypep->itemsp();
                అయితే (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), Enum_అంశం);
                ఒకవేళ (!itemp) {
                    newp = కొత్త AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } లేకపోతే {
                    newp = VN_AS(itemp->valuep()->clone_వృక్షం(అబద్ధం), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            ఒకవేళ (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                పద్ధతి_Ok_Arguments(nodep, 0, 0);
            } లేకపోతే ఒకవేళ (nodep->name() == "next") {
                attrType = VAttrType::ENUM_తదుపరి;
                పద్ధతి_Ok_Arguments(nodep, 0, 1);
            } లేకపోతే ఒకవేళ (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                పద్ధతి_Ok_Arguments(nodep, 0, 1);
            } లేకపోతే {
                nodep->v3fatalSrc("Bad case");
            }

            ఒకవేళ (nodep->name() != "name" && nodep->argsp()) {
                Ast_కణుపు_సమాసం* stepp = పద్ధతి_Arg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = పద్ధతి_Arg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3_Error_సంకేతం::WIDTHEXPAND, నిజం);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                ఒకవేళ (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(stepp), stepp);
                    stepp = newp;
                }
                స్థిరరాశి uint32_t అడుగు_వెడల్పు = VN_AS(stepp, Const)->గమ్యం_UInt();
                ఒకవేళ (అడుగు_వెడల్పు == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    Ast_పద్ధతి_Call* స్థిరరాశి newp = కొత్త Ast_పద్ధతి_Call{
                        nodep->fileline(),
                        కొత్త Ast_పద్ధతి_Call{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                    తిరిగి;
                } లేకపోతే ఒకవేళ (అడుగు_వెడల్పు != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    నెట్టు_Deletep(nodep->argsp()->unlink_Fr_వెనుక());
                    Ast_పద్ధతి_Call* స్థిరరాశి clonep = nodep->clone_వృక్షం(అబద్ధం);
                    VN_AS(stepp, Const)->అంకె().సమితి_Long(1);
                    AstConst* స్థిరరాశి constp = కొత్త AstConst(nodep->fileline(), అడుగు_వెడల్పు - 1);
                    AstArg* స్థిరరాశి argp = కొత్త AstArg{nodep->fileline(), "", constp};
                    Ast_పద్ధతి_Call* స్థిరరాశి newp
                        = కొత్త Ast_పద్ధతి_Call{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                    తిరిగి;
                }
            }
            Ast_కణుపు_సమాసం* స్థిరరాశి newp
                = enumSelect(nodep->fromp()->unlink_Fr_వెనుక(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    శూన్య పద్ధతి_Call_Wildcard(Ast_పద్ధతి_Call* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = శూన్యసూచిక;
        ఒకవేళ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ASSOC_కొలత};  // So don't need num()
            newp->dtype_సమితి_Int();
        } లేకపోతే ఒకవేళ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } లేకపోతే ఒకవేళ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", పద్ధతి_Arg(nodep, 0), SELF, BOTH);
            Ast_కణుపు_సమాసం* స్థిరరాశి సూచీ_exprp = పద్ధతి_Call_Wildcard_సూచీ_సమాసం(nodep, adtypep);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ASSOC_EXISTS, సూచీ_exprp->unlink_Fr_వెనుక()};
            newp->dtype_సమితి_బిట్టు_Unsized(32, 1, VSigning::SIGNED);
        } లేకపోతే ఒకవేళ (nodep->name() == "delete") {  // function void delete([input integer index])
            పద్ధతి_Ok_Arguments(nodep, 0, 1);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ఒకవేళ (!nodep->argsp()) {
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_సమితి_Void();
            } లేకపోతే {
                iterateCheckSelf(nodep, "argument", పద్ధతి_Arg(nodep, 0), SELF, BOTH);
                Ast_కణుపు_సమాసం* స్థిరరాశి సూచీ_exprp = పద్ధతి_Call_Wildcard_సూచీ_సమాసం(nodep, adtypep);
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                          VCMethod::ASSOC_ERASE, సూచీ_exprp->unlink_Fr_వెనుక()};
                newp->dtype_సమితి_Void();
            }
        } లేకపోతే ఒకవేళ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } లేకపోతే ఒకవేళ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, అబద్ధం, అబద్ధం, adtypep->subDTypep(),
                                   adtypep->కనుగొను_String_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->dtypep(వరుస_DType_సూచీకృత_By(adtypep->subDTypep()));
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, నిజం, అబద్ధం, nodep->కనుగొను_బిట్టు_DType(),
                                   adtypep->కనుగొను_String_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->withp(withp);
            newp->dtypep(వరుస_DType_సూచీకృత_By(adtypep->subDTypep()));
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "map") {
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, నిజం, అబద్ధం, adtypep->subDTypep(),
                                   adtypep->కనుగొను_String_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(వరుస_DType_సూచీకృత_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ఒకవేళ (newp) {
            newp->protect(అబద్ధం);
            newp->did_వెడల్పు(నిజం);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        }
    }
    శూన్య పద్ధతి_Call_Assoc(Ast_పద్ధతి_Call* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = శూన్యసూచిక;
        ఒకవేళ (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        ఒకవేళ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ASSOC_కొలత};  // So don't need num()
            newp->dtype_సమితి_Int();
        } లేకపోతే ఒకవేళ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", పద్ధతి_Arg(nodep, 0), adtypep->కీలకం_DTypep(), BOTH);
            Ast_కణుపు_సమాసం* స్థిరరాశి సూచీ_exprp = పద్ధతి_Call_Assoc_సూచీ_సమాసం(nodep, adtypep);
            పద్ధతి_Call_LValue_Recurse(nodep, సూచీ_exprp, VAccess::READWRITE);
            newp
                = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                     VCMethod::array_పద్ధతి(nodep->name()),  // first/last/next/prev
                                     సూచీ_exprp->unlink_Fr_వెనుక()};
            newp->dtype_సమితి_Int();
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", పద్ధతి_Arg(nodep, 0), adtypep->కీలకం_DTypep(), BOTH);
            Ast_కణుపు_సమాసం* స్థిరరాశి సూచీ_exprp = పద్ధతి_Call_Assoc_సూచీ_సమాసం(nodep, adtypep);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ASSOC_EXISTS, సూచీ_exprp->unlink_Fr_వెనుక()};
            newp->dtype_సమితి_బిట్టు_Unsized(32, 1, VSigning::SIGNED);
        } లేకపోతే ఒకవేళ (nodep->name() == "delete") {  // function void delete([input integer index])
            పద్ధతి_Ok_Arguments(nodep, 0, 1);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ఒకవేళ (!nodep->argsp()) {
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_సమితి_Void();
            } లేకపోతే {
                iterateCheckTyped(nodep, "argument", పద్ధతి_Arg(nodep, 0), adtypep->కీలకం_DTypep(),
                                  BOTH);
                Ast_కణుపు_సమాసం* స్థిరరాశి సూచీ_exprp = పద్ధతి_Call_Assoc_సూచీ_సమాసం(nodep, adtypep);
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                          VCMethod::ASSOC_ERASE, సూచీ_exprp->unlink_Fr_వెనుక()};
                newp->dtype_సమితి_Void();
            }
        } లేకపోతే ఒకవేళ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } లేకపోతే ఒకవేళ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* స్థిరరాశి withp = పద్ధతి_With_Clause(nodep, అబద్ధం, అబద్ధం, adtypep->subDTypep(),
                                                    adtypep->కీలకం_DTypep(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* స్థిరరాశి withp = పద్ధతి_With_Clause(
                nodep, అబద్ధం, నిజం, శూన్యసూచిక, nodep->కనుగొను_UInt32_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->withp(withp);
            ఒకవేళ (nodep->name() == "unique_index") {
                newp->dtypep(వరుస_DType_సూచీకృత_By(adtypep->కీలకం_DTypep()));
            } లేకపోతే {
                newp->dtypep(వరుస_DType_సూచీకృత_By(adtypep->subDTypep()));
            }
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* స్థిరరాశి withp = పద్ధతి_With_Clause(nodep, నిజం, అబద్ధం, nodep->కనుగొను_బిట్టు_DType(),
                                                    adtypep->కీలకం_DTypep(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->withp(withp);
            newp->dtypep(వరుస_DType_సూచీకృత_By(adtypep->subDTypep()));
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* స్థిరరాశి withp = పద్ధతి_With_Clause(nodep, నిజం, అబద్ధం, nodep->కనుగొను_బిట్టు_DType(),
                                                    adtypep->కీలకం_DTypep(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->withp(withp);
            newp->dtypep(వరుస_DType_సూచీకృత_By(adtypep->కీలకం_DTypep()));
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "map") {
            AstWith* స్థిరరాశి withp = పద్ధతి_With_Clause(nodep, నిజం, అబద్ధం, adtypep->subDTypep(),
                                                    adtypep->కీలకం_DTypep(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(వరుస_DType_సూచీకృత_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ఒకవేళ (newp) {
            newp->protect(అబద్ధం);
            newp->did_వెడల్పు(నిజం);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        }
    }
    Ast_కణుపు_సమాసం* పద్ధతి_Call_Assoc_సూచీ_సమాసం(Ast_పద్ధతి_Call* nodep, AstAssocArrayDType* adtypep) {
        Ast_కణుపు* స్థిరరాశి సూచీ_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", సూచీ_exprp, CONTEXT_DET, FINAL, adtypep->కీలకం_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(సూచీ_exprp);  // May have been edited
        తిరిగి nodep->argsp()->exprp();
    }
    Ast_కణుపు_సమాసం* పద్ధతి_Call_Wildcard_సూచీ_సమాసం(Ast_పద్ధతి_Call* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_కణుపు* స్థిరరాశి సూచీ_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", సూచీ_exprp, CONTEXT_DET, FINAL, adtypep->కనుగొను_String_DType(),
                     EXTEND_EXP);
        VL_DANGLING(సూచీ_exprp);  // May have been edited
        తిరిగి nodep->argsp()->exprp();
    }
    శూన్య పద్ధతి_Call_LValue_Recurse(Ast_పద్ధతి_Call* nodep, Ast_కణుపు* childp, స్థిరరాశి VAccess& access) {
        ఒకవేళ (AstCMethodHard* స్థిరరాశి ichildp = VN_CAST(childp, CMethodHard)) {
            తార్కిక methAt = అబద్ధం;
            తార్కిక meth_At_వెనుక = అబద్ధం;
            తార్కిక methAtWrite = అబద్ధం;
            ఎంపిక (ichildp->పద్ధతి()) {
            స్థితి VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = నిజం;
                ఆపు;
            స్థితి VCMethod::ARRAY_AT_వెనుక:  // FALLTHRU
                meth_At_వెనుక = నిజం;
                ఆపు;
            స్థితి VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            స్థితి VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            స్థితి VCMethod::DYN_AT_WRITE_APPEND_వెనుక:  //
                methAtWrite = నిజం;
                ఆపు;
            అప్రమేయ: ఆపు;
            }
            ఒకవేళ (methAt || meth_At_వెనుక || methAtWrite) {
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                ఒకవేళ (VN_IS(fromDtypep, వరుస_DType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    ఒకవేళ (methAt) {
                        ichildp->పద్ధతి(VCMethod::ARRAY_AT_WRITE);
                    } లేకపోతే ఒకవేళ (meth_At_వెనుక) {
                        ichildp->పద్ధతి(VCMethod::DYN_AT_WRITE_APPEND_వెనుక);
                    }
                }
                పద్ధతి_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
                తిరిగి;
            }
        }
        ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(childp, కణుపు_Var_Ref)) {
            varrefp->access(access);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstMemberSel* స్థిరరాశి ichildp = VN_CAST(childp, MemberSel)) {
            పద్ధతి_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstStructSel* స్థిరరాశి ichildp = VN_CAST(childp, StructSel)) {
            పద్ధతి_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_Sel* స్థిరరాశి ichildp = VN_CAST(childp, కణుపు_Sel)) {
            పద్ధతి_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } లేకపోతే ఒకవేళ (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } లేకపోతే {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* పద్ధతి_Call_Array(Ast_పద్ధతి_Call* nodep, Ast_కణుపు_DType* adtypep) {
        AstCMethodHard* newp = శూన్యసూచిక;

        ఒకవేళ (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = శూన్యసూచిక;
            ఒకవేళ (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = పద్ధతి_With_Clause(nodep, అబద్ధం, నిజం, శూన్యసూచిక, nodep->కనుగొను_UInt32_DType(),
                                         adtypep->subDTypep());
            }
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->withp(withp);
            newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* స్థిరరాశి withp = పద్ధతి_With_Clause(
                nodep, అబద్ధం, నిజం, శూన్యసూచిక, nodep->కనుగొను_UInt32_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->withp(withp);
            ఒకవేళ (nodep->name() == "unique_index") {
                newp->dtypep(newp->కనుగొను_వరుస_సూచీ_DType());
            } లేకపోతే {
                newp->dtypep(వరుస_DType_సూచీకృత_By(adtypep->subDTypep()));
            }
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, నిజం, అబద్ధం, nodep->కనుగొను_బిట్టు_DType(),
                                   nodep->కనుగొను_UInt32_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->withp(withp);
            newp->dtypep(వరుస_DType_సూచీకృత_By(adtypep->subDTypep()));
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, నిజం, అబద్ధం, nodep->కనుగొను_బిట్టు_DType(),
                                   nodep->కనుగొను_UInt32_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->కనుగొను_వరుస_సూచీ_DType());
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, నిజం, అబద్ధం, adtypep->subDTypep(),
                                   nodep->కనుగొను_UInt32_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(వరుస_DType_సూచీకృత_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        }
        తిరిగి newp;
    }
    శూన్య పద్ధతి_Call_Dyn(Ast_పద్ధతి_Call* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = శూన్యసూచిక;
        ఒకవేళ (nodep->name() == "at") {  // Created internally for []
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } లేకపోతే ఒకవేళ (nodep->name() == "atWrite") {  // Created internally for []
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } లేకపోతే ఒకవేళ (nodep->name() == "size") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::DYN_కొలత};
            newp->dtype_సమితి_Int();
        } లేకపోతే ఒకవేళ (nodep->name() == "delete") {  // function void delete()
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::DYN_CLEAR};
            newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, అబద్ధం, అబద్ధం, adtypep->subDTypep(),
                                   nodep->కనుగొను_UInt32_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ ((newp = పద్ధతి_Call_Array(nodep, adtypep))) {
        } లేకపోతే {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ఒకవేళ (newp) {
            newp->protect(అబద్ధం);
            newp->did_వెడల్పు(నిజం);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        }
    }
    శూన్య పద్ధతి_Call_వరుస(Ast_పద్ధతి_Call* nodep, Ast_వరుస_DType* adtypep) {
        AstCMethodHard* newp = శూన్యసూచిక;
        ఒకవేళ (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } లేకపోతే ఒకవేళ (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } లేకపోతే ఒకవేళ (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::DYN_కొలత};
            newp->dtype_సమితి_Int();
        } లేకపోతే ఒకవేళ (nodep->name() == "delete") {  // function void delete([input integer index])
            పద్ధతి_Ok_Arguments(nodep, 0, 1);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ఒకవేళ (!nodep->argsp()) {
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                          VCMethod::DYN_CLEAR};
                newp->dtype_సమితి_Void();
            } లేకపోతే {
                iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
                Ast_కణుపు_సమాసం* స్థిరరాశి సూచీ_exprp = పద్ధతి_Call_వరుస_సూచీ_సమాసం(nodep);
                ఒకవేళ (సూచీ_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                              VCMethod::DYN_తీయి_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtype_సమితి_Void();
                } లేకపోతే {
                    newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                              VCMethod::DYN_ERASE, సూచీ_exprp->unlink_Fr_వెనుక()};
                    newp->dtype_సమితి_Void();
                }
            }
        } లేకపోతే ఒకవేళ (nodep->name() == "insert") {
            పద్ధతి_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", పద్ధతి_Arg(nodep, 0), BOTH);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_కణుపు_సమాసం* స్థిరరాశి సూచీ_exprp = పద్ధతి_Call_వరుస_సూచీ_సమాసం(nodep);
            AstArg* స్థిరరాశి argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            ఒకవేళ (సూచీ_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                          VCMethod::DYN_నెట్టు_FRONT, argp->exprp()->unlink_Fr_వెనుక()};
                newp->dtype_సమితి_Void();
            } లేకపోతే {
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                          VCMethod::DYN_చొప్పించు, సూచీ_exprp->unlink_Fr_వెనుక()};
                newp->కూడు_Pinsp(argp->exprp()->unlink_Fr_వెనుక());
                newp->dtype_సమితి_Void();
            }
        } లేకపోతే ఒకవేళ (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } లేకపోతే ఒకవేళ (nodep->name() == "push_back" || nodep->name() == "push_front") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", పద్ధతి_Arg(nodep, 0), adtypep->subDTypep(), BOTH);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* స్థిరరాశి argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి(nodep->name()),
                                      argp->exprp()->unlink_Fr_వెనుక()};
            newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, అబద్ధం, అబద్ధం, adtypep->subDTypep(),
                                   nodep->కనుగొను_UInt32_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                      VCMethod::array_పద్ధతి("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ ((newp = పద్ధతి_Call_Array(nodep, adtypep))) {
        } లేకపోతే {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ఒకవేళ (newp) {
            newp->protect(అబద్ధం);
            newp->did_వెడల్పు(నిజం);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        }
    }
    Ast_కణుపు_సమాసం* పద్ధతి_Call_వరుస_సూచీ_సమాసం(Ast_పద్ధతి_Call* nodep) {
        Ast_కణుపు* స్థిరరాశి సూచీ_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", సూచీ_exprp, BOTH);
        VL_DANGLING(సూచీ_exprp);  // May have been edited
        తిరిగి nodep->argsp()->exprp();
    }
    శూన్య పద్ధతి_Call_Warn_Timing(Ast_కణుపు_FTask_Ref* స్థిరరాశి nodep, స్థిరరాశి std::string& className) {
        ఒకవేళ (v3Global.opt.timing().is_సమితి_False()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } లేకపోతే ఒకవేళ (!v3Global.opt.timing().is_సమితి_True()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    శూన్య పద్ధతి_Call_Iface_Ref(Ast_పద్ధతి_Call* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* స్థిరరాశి ifacep = adtypep->iface_ద్వారా_Cellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp
            = VN_CAST(m_memberMap.కనుగొను_Member(ifacep, nodep->name()), కణుపు_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            ఒకవేళ (!modport_Allows_అంశం(nodep, adtypep, ftaskp)) {
                nodep->dtype_సమితి_Void();
                తిరిగి;
            }
            ఒకవేళ (adtypep->isVirtual()) {
                కొరకు (Ast_కణుపు* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    ఒకవేళ (AstVar* స్థిరరాశి mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, శూన్యసూచిక);
            ఒకవేళ (ftaskp->isStatic()) {
                AstArg* స్థిరరాశి argsp = nodep->argsp();
                ఒకవేళ (argsp) argsp->unlink_Fr_వెనుక_With_తదుపరి();
                Ast_కణుపు_FTask_Ref* newp = శూన్యసూచిక;
                ఒకవేళ (VN_IS(ftaskp, Task)) {
                    newp = కొత్త AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } లేకపోతే {
                    newp = కొత్త AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            } లేకపోతే {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                ఒకవేళ (VN_IS(ftaskp, Task)) nodep->dtype_సమితి_Void();
                process_FTask_Ref_వాదనలు(nodep);
            }
            తిరిగి;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    శూన్య handle_Randomize_వాదనలు(Ast_కణుపు_FTask_Ref* స్థిరరాశి nodep, AstClass* స్థిరరాశి classp) {
        తార్కిక has_Non_Null_వాదనలు = అబద్ధం;
        AstConst* nullp = శూన్యసూచిక;
        కొరకు (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = శూన్యసూచిక;
            Ast_కణుపు_సమాసం* exprp = argp->exprp();
            ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(exprp, Const)) {
                ఒకవేళ (constp->అంకె().isNull()) {
                    nullp = constp;
                    కొనసాగు;
                }
            }
            has_Non_Null_వాదనలు = నిజం;
            AstVar* fromVarp = శూన్యసూచిక;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            ఒకవేళ (Ast_పద్ధతి_Call* పద్ధతి_Callp = VN_CAST(nodep, పద్ధతి_Call)) {
                Ast_కణుపు_సమాసం* fromp = పద్ధతి_Callp->fromp();
                అయితే (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* స్థిరరాశి varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            ఒకవేళ (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);
                కొనసాగు;
            }
            అయితే (exprp) {
                ఒకవేళ (AstMemberSel* స్థిరరాశి memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } లేకపోతే {
                    ఒకవేళ (AstVarRef* స్థిరరాశి varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } లేకపోతే {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);
                    }
                    exprp = శూన్యసూచిక;
                }
                // All variables in the dot hierarchy must be randomizable
                ఒకవేళ (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            ఒకవేళ (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } లేకపోతే ఒకవేళ (classp->existsMember([&](స్థిరరాశి AstClass*, స్థిరరాశి AstVar* memberVarp) {
                           తిరిగి memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } లేకపోతే {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_వెనుక()->delete_వృక్షం(), argp);
            }
        }
        ఒకవేళ (nullp && has_Non_Null_వాదనలు) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    శూన్య పద్ధతి_Call_Class(Ast_పద్ధతి_Call* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* స్థిరరాశి మొదటి_classp = adtypep->classp();
        AstWith* withp = శూన్యసూచిక;
        ఒకవేళ (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = పద్ధతి_With_Clause(nodep, అబద్ధం, అబద్ధం, adtypep->కనుగొను_Void_DType(),
                                     adtypep->కనుగొను_బిట్టు_DType(), adtypep);
            కొరకు (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                ఒకవేళ (argp->exprp()) userIterate(argp->exprp(), వెడల్పు_VP{SELF, BOTH}.p());
            }
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, మొదటి_classp);
            handle_Randomize_వాదనలు(nodep, మొదటి_classp);
        } లేకపోతే ఒకవేళ (nodep->name() == "srandom") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, మొదటి_classp);
        }
        UASSERT_OBJ(మొదటి_classp, nodep, "Unlinked");
        కొరకు (AstClass* classp = మొదటి_classp; classp;) {
            ఒకవేళ (nodep->fileline()->timingOn()) {
                ఒకవేళ (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* స్థిరరాశి packagep = get_అంశం_Package(classp);
                    // Check if it's std
                    ఒకవేళ (packagep && packagep->name() == "std") {
                        ఒకవేళ (classp->name() == "process") {
                            పద్ధతి_Call_Warn_Timing(nodep, "process");
                        } లేకపోతే ఒకవేళ (classp->name() == "semaphore" && nodep->name() == "get") {
                            పద్ధతి_Call_Warn_Timing(nodep, "semaphore");
                        } లేకపోతే ఒకవేళ (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            పద్ధతి_Call_Warn_Timing(nodep, "mailbox");
                        }
                    }
                }
            }
            ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp
                = VN_CAST(m_memberMap.కనుగొను_Member(classp, nodep->name()), కణుపు_FTask)) {
                userIterate(ftaskp, శూన్యసూచిక);
                ఒకవేళ (ftaskp->isStatic()) {
                    AstArg* స్థిరరాశి argsp = nodep->argsp();
                    ఒకవేళ (argsp) argsp->unlink_Fr_వెనుక_With_తదుపరి();
                    Ast_కణుపు_FTask_Ref* newp = శూన్యసూచిక;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    ఒకవేళ (!m_vup) {  // Called as task
                        newp = కొత్త AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } లేకపోతే {
                        newp = కొత్త AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                } లేకపోతే {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    ఒకవేళ (VN_IS(ftaskp, Task)) {
                        ఒకవేళ (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_సమాసం)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtype_సమితి_Void();
                    }
                    ఒకవేళ (withp) nodep->withp(withp);
                    process_FTask_Ref_వాదనలు(nodep);
                }
                తిరిగి;
            } లేకపోతే ఒకవేళ (nodep->name() == "get_randstate") {
                పద్ధతి_Ok_Arguments(nodep, 0, 0);
                మొదటి_classp->పునాది_Most_Classp()->needRNG(నిజం);
                v3Global.useRandomizeMethods(నిజం);
                AstCMethodHard* స్థిరరాశి newp
                    = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                         VCMethod::RNG_GET_RANDSTATE, శూన్యసూచిక};
                newp->usePtr(నిజం);
                newp->dtype_సమితి_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            } లేకపోతే ఒకవేళ (nodep->name() == "set_randstate") {
                పద్ధతి_Ok_Arguments(nodep, 1, 1);
                Ast_కణుపు_సమాసం* స్థిరరాశి expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->argsp()->exprp();
                మొదటి_classp->పునాది_Most_Classp()->needRNG(నిజం);
                v3Global.useRandomizeMethods(నిజం);
                AstCMethodHard* స్థిరరాశి newp
                    = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                         VCMethod::RNG_సమితి_RANDSTATE, exprp->unlink_Fr_వెనుక()};
                newp->usePtr(నిజం);
                newp->dtype_సమితి_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            } లేకపోతే ఒకవేళ (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(నిజం);
                nodep->dtypep(nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : శూన్యసూచిక;
        }
        {
            VSpellCheck speller;
            కొరకు (AstClass* classp = మొదటి_classp; classp;) {
                కొరకు (Ast_కణుపు* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    ఒకవేళ (VN_IS(itemp, కణుపు_FTask)) speller.నెట్టు_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : శూన్యసూచిక;
            }
            స్థిరరాశి string suggest = speller.ఉత్తమ_Candidate_సందేశం(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << మొదటి_classp->verilogKwd() << " " << మొదటి_classp->prettyNameQ()
                           << "\n"
                           << (suggest.ఖాళీలు() ? "" : nodep->fileline()->warn_మరిన్ని() + suggest));
        }
        nodep->dtype_సమితి_Int();  // Guess on error
    }
    శూన్య పద్ధతి_Call_Constraint(Ast_పద్ధతి_Call* nodep, AstConstraintRefDType*) {
        ఒకవేళ (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            పద్ధతి_Ok_Arguments(nodep, 0, 1);
            ఒకవేళ (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
                nodep->dtypep(nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::INT));
            } లేకపోతే {
                nodep->dtype_సమితి_Void();
            }
            v3Global.useRandomizeMethods(నిజం);
        } లేకపోతే {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య పద్ధతి_Call_Rand_బహుళకం(Ast_పద్ధతి_Call* nodep) {
        పద్ధతి_Ok_Arguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        ఒకవేళ (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            nodep->dtype_సమితి_Void();
        } లేకపోతే {
            nodep->dtypep(nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(నిజం);
    }
    శూన్య పద్ధతి_Call_Unpack(Ast_పద్ధతి_Call* nodep, AstUnpackArrayDType* adtypep) {
        గణన : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_కూడిక,
            ARRAY_లబ్ధం
        } పద్ధతి_Id;

        పద్ధతి_Id = UNKNOWN;
        ఒకవేళ (nodep->name() == "or") {
            పద్ధతి_Id = ARRAY_OR;
        } లేకపోతే ఒకవేళ (nodep->name() == "and") {
            పద్ధతి_Id = ARRAY_AND;
        } లేకపోతే ఒకవేళ (nodep->name() == "xor") {
            పద్ధతి_Id = ARRAY_XOR;
        } లేకపోతే ఒకవేళ (nodep->name() == "sum") {
            పద్ధతి_Id = ARRAY_కూడిక;
        } లేకపోతే ఒకవేళ (nodep->name() == "product") {
            పద్ధతి_Id = ARRAY_లబ్ధం;
        }

        ఒకవేళ (పద్ధతి_Id) {
            AstWith* స్థిరరాశి withp
                = పద్ధతి_With_Clause(nodep, అబద్ధం, అబద్ధం, adtypep->subDTypep(),
                                   nodep->కనుగొను_UInt32_DType(), adtypep->subDTypep());
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            ఒకవేళ (withp) {
                పద్ధతి_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* స్థిరరాశి newp
                    = కొత్త AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(),
                                         VCMethod::array_పద్ధతి("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                ఒకవేళ (!nodep->మొదటి_Abovep()) newp->dtype_సమితి_Void();
                newp->protect(అబద్ధం);
                newp->did_వెడల్పు(నిజం);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
                తిరిగి;
            } లేకపోతే {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
                Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
                కొరకు (పూర్ణాంకం i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_కణుపు_సమాసం* స్థిరరాశి arrayRef = nodep->fromp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
                    Ast_కణుపు_సమాసం* స్థిరరాశి selector = కొత్త AstArraySel{fl, arrayRef, i};
                    ఒకవేళ (!newp) {
                        newp = selector;
                    } లేకపోతే {
                        ఎంపిక (పద్ధతి_Id) {
                        స్థితి ARRAY_OR: newp = కొత్త AstOr{fl, newp, selector}; ఆపు;
                        స్థితి ARRAY_AND: newp = కొత్త AstAnd{fl, newp, selector}; ఆపు;
                        స్థితి ARRAY_XOR: newp = కొత్త AstXor{fl, newp, selector}; ఆపు;
                        స్థితి ARRAY_కూడిక: newp = కొత్త Ast_కూడు{fl, newp, selector}; ఆపు;
                        స్థితి ARRAY_లబ్ధం: newp = కొత్త AstMul{fl, newp, selector}; ఆపు;
                        అప్రమేయ: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            }
        } లేకపోతే ఒకవేళ (AstCMethodHard* newp = పద్ధతి_Call_Array(nodep, adtypep)) {
            newp->protect(అబద్ధం);
            newp->did_వెడల్పు(నిజం);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    శూన్య పద్ధతి_Call_Event(Ast_పద్ధతి_Call* nodep, AstBasicDType*) {
        // Method call on event
        ఒకవేళ (nodep->name() == "triggered") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            AstCMethodHard* స్థిరరాశి callp = కొత్త AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtype_సమితి_బిట్టు();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    శూన్య పద్ధతి_Call_String(Ast_పద్ధతి_Call* nodep, AstBasicDType*) {
        // Method call on string
        ఒకవేళ (nodep->name() == "len") {
            // Constant value
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstLenN{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "itoa") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "hextoa") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "octtoa") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "bintoa") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "realtoa") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "tolower") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            Ast_కణుపు* స్థిరరాశి newp
                = కొత్త Ast_గమ్యం_క్రింది_N{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "toupper") {
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            Ast_కణుపు* స్థిరరాశి newp
                = కొత్త Ast_గమ్యం_పై_N{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "compare" || nodep->name() == "icompare") {
            స్థిరరాశి తార్కిక ignoreCase = nodep->name()[0] == 'i';
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            AstArg* స్థిరరాశి argp = nodep->argsp();
            Ast_కణుపు_సమాసం* స్థిరరాశి lhs = nodep->fromp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhs = argp->exprp()->unlink_Fr_వెనుక();
            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "putc") {
            పద్ధతి_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", పద్ధతి_Arg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", పద్ధతి_Arg(nodep, 1), BOTH);
            AstArg* స్థిరరాశి arg0p = nodep->argsp();
            AstArg* స్థిరరాశి arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_కణుపు_Var_Ref* స్థిరరాశి fromp = VN_AS(nodep->fromp()->unlink_Fr_వెనుక(), VarRef);
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = arg0p->exprp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి thsp = arg1p->exprp()->unlink_Fr_వెనుక();
            AstVarRef* స్థిరరాశి varrefp
                = కొత్త AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_కణుపు* స్థిరరాశి newp = కొత్త Ast_కేటాయించు{
                nodep->fileline(), fromp, కొత్త AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            నెట్టు_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "getc") {
            పద్ధతి_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", పద్ధతి_Arg(nodep, 0), BOTH);
            AstArg* స్థిరరాశి arg0p = nodep->argsp();
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->fromp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = arg0p->exprp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "substr") {
            పద్ధతి_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", పద్ధతి_Arg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", పద్ధతి_Arg(nodep, 1), BOTH);
            AstArg* స్థిరరాశి arg0p = nodep->argsp();
            AstArg* స్థిరరాశి arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->fromp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = arg0p->exprp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి thsp = arg1p->exprp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే ఒకవేళ (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            ఒకవేళ (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } లేకపోతే ఒకవేళ (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } లేకపోతే ఒకవేళ (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } లేకపోతే ఒకవేళ (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } లేకపోతే ఒకవేళ (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } లేకపోతే {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            పద్ధతి_Ok_Arguments(nodep, 0, 0);
            Ast_కణుపు* స్థిరరాశి newp
                = కొత్త AstAtoN{nodep->fileline(), nodep->fromp()->unlink_Fr_వెనుక(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    Ast_వరుస_DType* వరుస_DType_సూచీకృత_By(Ast_కణుపు_DType* సూచీ_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        ఒకవేళ (Ast_వరుస_DType* స్థిరరాశి queuep = m_వరుస_DType_సూచీకృత[సూచీ_DTypep]) {
            తిరిగి queuep;
        } లేకపోతే {
            Ast_వరుస_DType* స్థిరరాశి newp
                = కొత్త Ast_వరుస_DType{సూచీ_DTypep->fileline(), సూచీ_DTypep, శూన్యసూచిక};
            v3Global.rootp()->typeTablep()->కూడు_Typesp(newp);
            m_వరుస_DType_సూచీకృత[సూచీ_DTypep] = newp;
            తిరిగి newp;
        }
    }

    శూన్య సందర్శించు(AstNew* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        AstClass* classp = శూన్యసూచిక;
        తార్కిక కేటాయించు_2 = అబద్ధం;
        ఒకవేళ (VN_IS(nodep->backp(), కేటాయించు)) {  // assignment case
            కేటాయించు_2 = నిజం;
            Ast_కణుపు* warnp = శూన్యసూచిక;
            AstClassRefDType* refp = శూన్యసూచిక;

            ఒకవేళ (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                ఒకవేళ (AstClass* స్థిరరాశి clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* స్థిరరాశి adtypep
                        = కొత్త AstClassRefDType{nodep->fileline(), clsp, శూన్యసూచిక};
                    v3Global.rootp()->typeTablep()->కూడు_Typesp(adtypep);
                    refp = adtypep;
                }
            } లేకపోతే {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : శూన్యసూచిక;
            }
            ఒకవేళ (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, కణుపు_DType)
                           ? ("data type "s + VN_AS(warnp, కణుపు_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                తిరిగి;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp
                = VN_CAST(m_memberMap.కనుగొను_Member(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } లేకపోతే {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } లేకపోతే {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        process_FTask_Ref_వాదనలు(nodep);
        ఒకవేళ (!కేటాయించు_2) nodep->dtypeFrom(nodep->taskp());
    }
    శూన్య సందర్శించు(AstNewCopy* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        AstClassRefDType* స్థిరరాశి refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        ఒకవేళ (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            తిరిగి;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        ఒకవేళ (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    శూన్య సందర్శించు(AstNewDynamic* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        AstDynArrayDType* స్థిరరాశి adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        ఒకవేళ (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            తిరిగి;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        ఒకవేళ (!VN_IS(nodep->backp(), కణుపు_కేటాయించు)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            తిరిగి;
        }
        nodep->dtypep(adtypep);
        ఒకవేళ (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        ఒకవేళ (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* స్థిరరాశి consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            ఒకవేళ (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlink_Fr_వెనుక());
                nodep->replaceWith(consp->unlink_Fr_వెనుక());
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        }
    }

    శూన్య సందర్శించు(Ast_Tagged_సమాసం* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtype_సమితి_బిట్టు();
        userIterateChildren(nodep, m_vup);
    }
    శూన్య సందర్శించు(Ast_Tagged_నమూనా* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtype_సమితి_బిట్టు();
        userIterateChildren(nodep, m_vup);
    }
    శూన్య సందర్శించు(Ast_నమూనా_Var* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtype_సమితి_బిట్టు();
    }
    శూన్య సందర్శించు(Ast_నమూనా_నక్షత్రం* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtype_సమితి_బిట్టు();
    }
    శూన్య సందర్శించు(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtype_సమితి_బిట్టు();
        userIterateChildren(nodep, m_vup);
    }

    శూన్య సందర్శించు(Ast_నమూనా* nodep) override {
        ఒకవేళ (m_లోపల_Param_Override && !nodep->బిడ్డ_DTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_లోపల_Param_Override);
            కొరకు (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_లోపల_Param_Override = అబద్ధం;
                user_Iterate_And_తదుపరి(patp->repp(), వెడల్పు_VP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                ఒకవేళ (!VN_IS(patp->keyp(), పాఠ్యం_2) && !VN_IS(patp->keyp(), కణుపు_DType)) {
                    user_Iterate_And_తదుపరి(patp->keyp(), వెడల్పు_VP{SELF, BOTH}.p());
                }
                కొరకు (Ast_కణుపు *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_లోపల_Param_Override = VN_IS(valuep, నమూనా);
                    userIterate(valuep, వెడల్పు_VP{SELF, BOTH}.p());
                }
            }
            తిరిగి;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_లోపల_Param_Override);
        m_లోపల_Param_Override = అబద్ధం;
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        UINFO(9, "PATTERN " << nodep);
        ఒకవేళ (nodep->బిడ్డ_DTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        ఒకవేళ (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_కణుపు_DType* dtypep = nodep->dtypep();
        ఒకవేళ (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            ఒకవేళ (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                తిరిగి;
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            ఒకవేళ (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                తిరిగి;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            కొరకు (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                స్థిరరాశి పూర్ణాంకం times = సందర్శించు_Pat_Member_Rep(patp);
                కొరకు (పూర్ణాంకం i = 1; i < times; i++) {
                    AstPatMember* స్థిరరాశి newp = patp->clone_వృక్షం(అబద్ధం);
                    patp->కూడు_తదుపరి_ఇక్కడ(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            కొరకు (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ఒకవేళ (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlink_Fr_వెనుక(&relinkHandle);
                    అయితే (Ast_కణుపు_సమాసం* స్థిరరాశి movep = VN_AS(patp->lhssp()->nextp(), కణుపు_సమాసం)) {
                        movep->unlink_Fr_వెనుక();  // Not unlinkFrBackWithNext, just one
                        Ast_కణుపు* newkeyp = శూన్యసూచిక;
                        ఒకవేళ (patp->keyp()) newkeyp = patp->keyp()->clone_వృక్షం(నిజం);
                        AstPatMember* స్థిరరాశి newp
                            = కొత్త AstPatMember{patp->fileline(), movep, newkeyp, శూన్యసూచిక};
                        patp->కూడు_తదుపరి(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = శూన్యసూచిక;
            కొరకు (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* స్థిరరాశి nextp = VN_AS(patp->nextp(), PatMember);
                ఒకవేళ (patp->isDefault()) {
                    ఒకవేళ (defaultp) {
                        VL_DO_DANGLING(నెట్టు_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlink_Fr_వెనుక();
                }
                patp = nextp;
            }
            అయితే (స్థిరరాశి AstConstDType* స్థిరరాశి vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, వెడల్పు_VP{SELF, BOTH}.p());

            ఒకవేళ (స్వయంచాలక* స్థిరరాశి vdtypep = VN_CAST(dtypep, StructDType)) {
                నమూనా_Struct(nodep, vdtypep, defaultp);
            } లేకపోతే ఒకవేళ (స్వయంచాలక* స్థిరరాశి vdtypep = VN_CAST(dtypep, కణుపు_Array_DType)) {
                నమూనా_Array(nodep, vdtypep, defaultp);
            } లేకపోతే ఒకవేళ (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* స్థిరరాశి consp = కొత్త AstConsAssoc{nodep->fileline()};
                నమూనా_Keyed<Ast_Cons_Assoc_అంశం>(nodep, consp, dtypep, defaultp);
            } లేకపోతే ఒకవేళ (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* స్థిరరాశి consp = కొత్త AstConsWildcard{nodep->fileline()};
                నమూనా_Keyed<Ast_Cons_Wildcard_అంశం>(nodep, consp, dtypep, defaultp);
            } లేకపోతే ఒకవేళ (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, వరుస_DType)) {
                నమూనా_Dyn_Array_Or_వరుస(nodep, dtypep, defaultp);
            } లేకపోతే ఒకవేళ (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                నమూనా_Basic(nodep, dtypep, defaultp);
            } లేకపోతే {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            ఒకవేళ (defaultp) VL_DO_DANGLING(నెట్టు_Deletep(defaultp), defaultp);
        }
    }
    శూన్య నమూనా_Struct(Ast_నమూనా* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        ఉపయోగించి PatMap = std::map<స్థిరరాశి AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            స్థిరరాశి AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            అయితే (patp) {
                చేయి {
                    ఒకవేళ (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        ఒకవేళ (స్థిరరాశి Ast_పాఠ్యం* textp = VN_CAST(patp->keyp(), పాఠ్యం_2)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.కనుగొను_Member(vdtypep, textp->పాఠ్యం()),
                                           MemberDType);
                            ఒకవేళ (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->పాఠ్యం() << "' not found as member");
                                ఆపు;
                            } లేకపోతే {
                                స్థిరరాశి std::pair<PatMap::iterator, తార్కిక> ret
                                    = patmap.emplace(memp, patp);
                                ఒకవేళ (!ret.రెండవ) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), పాఠ్యం_2)->పాఠ్యం());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } లేకపోతే ఒకవేళ (Ast_కణుపు_DType* nodedtypep = VN_CAST(patp->keyp(), కణుపు_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, వెడల్పు_VP{SELF, BOTH}.p());
                            స్థిరరాశి string dtype = nodedtypep->dtypep()->prettyDTypeName(నిజం);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } లేకపోతే {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } లేకపోతే ఒకవేళ (memp) {
                        // constant expr
                        స్థిరరాశి std::pair<PatMap::iterator, తార్కిక> ret = patmap.emplace(memp, patp);
                        ఒకవేళ (!ret.రెండవ) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), పాఠ్యం_2)->పాఠ్యం());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } లేకపోతే {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } అయితే (అబద్ధం);

                //తదుపరి
                ఒకవేళ (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
        ఒకవేళ (vdtypep->packed()) {
            కొరకు (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                స్థిరరాశి స్వయంచాలక it = patmap.కనుగొను(memp);
                ఒకవేళ (it == patmap.చివర()) {  // default or default_type assignment
                    ఒకవేళ (AstStructDType* స్థిరరాశి memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalue_Concat_నమూనా_Struct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } లేకపోతే {
                        AstPatMember* స్థిరరాశి patp
                            = default_Patp_నమూనా_Struct(nodep, memp, defaultp, dtypemap);
                        newp = విలువ_Concat_నమూనా_Struct(patp, newp, memp, nodep);
                        ఒకవేళ (patp) VL_DO_DANGLING(నెట్టు_Deletep(patp), patp);
                    }
                } లేకపోతే {  // member assignment
                    newp = విలువ_Concat_నమూనా_Struct(it->రెండవ, newp, memp, nodep);
                }
            }
        } లేకపోతే {  // Unpacked
            AstConsStruct* స్థిరరాశి consp = కొత్త AstConsStruct{nodep->fileline(), vdtypep};
            కొరకు (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                స్థిరరాశి స్వయంచాలక it = patmap.కనుగొను(memp);
                AstPatMember* patp = శూన్యసూచిక;
                ఒకవేళ (it == patmap.చివర()) {  // Default or default_type assignment
                    patp = default_Patp_నమూనా_Struct(nodep, memp, defaultp, dtypemap);
                    నెట్టు_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    నెట్టు_Deletep(patp);  // patp used below
                } లేకపోతే {
                    patp = it->రెండవ;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_కణుపు_సమాసం* స్థిరరాశి valuep = నమూనా_Member_విలువ_Iterate(patp);
                consp->కూడు_Itemsp(కొత్త Ast_Cons_Struct_అంశం{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        ఒకవేళ (newp) {
            nodep->replaceWith(newp);
        } లేకపోతే {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_కణుపు_సమాసం* nestedvalue_Concat_నమూనా_Struct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_కణుపు_సమాసం* newp,
                                                 Ast_నమూనా* nodep, స్థిరరాశి DTypeMap& dtypemap) {
        కొరకు (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            ఒకవేళ (AstStructDType* స్థిరరాశి memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalue_Concat_నమూనా_Struct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } లేకపోతే {
                AstPatMember* స్థిరరాశి patp
                    = default_Patp_నమూనా_Struct(nodep, memp_nested, defaultp, dtypemap);
                newp = విలువ_Concat_నమూనా_Struct(patp, newp, memp_nested, nodep);
                ఒకవేళ (patp) VL_DO_DANGLING(నెట్టు_Deletep(patp), patp);
            }
        }
        తిరిగి newp;
    }

    AstPatMember* default_Patp_నమూనా_Struct(Ast_నమూనా* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, స్థిరరాశి DTypeMap& dtypemap) {
        స్థిరరాశి string memp_DType = memp->virtRefDTypep()->prettyDTypeName(నిజం);
        స్థిరరాశి స్వయంచాలక it = dtypemap.కనుగొను(memp_DType);
        ఒకవేళ (it != dtypemap.చివర()) {
            // default_value for data_type
            తిరిగి it->రెండవ->clone_వృక్షం(అబద్ధం);
        }
        ఒకవేళ (defaultp) {
            // default_value for any unmatched member yet
            తిరిగి defaultp->clone_వృక్షం(అబద్ధం);
        }
        ఒకవేళ (memp->valuep()) {
            తిరిగి కొత్త AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_వృక్షం(అబద్ధం), కణుపు_సమాసం),
                                    కొత్త Ast_పాఠ్యం{nodep->fileline(), memp->name()}, శూన్యసూచిక};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        తిరిగి శూన్యసూచిక;
    }

    Ast_కణుపు_సమాసం* విలువ_Concat_నమూనా_Struct(AstPatMember* patp, Ast_కణుపు_సమాసం* newp,
                                           AstMemberDType* memp, Ast_నమూనా* nodep) {
        ఒకవేళ (patp) {
            patp->dtypep(memp);
            Ast_కణుపు_సమాసం* స్థిరరాశి valuep = నమూనా_Member_విలువ_Iterate(patp);
            ఒకవేళ (!newp) {
                newp = valuep;
            } లేకపోతే {
                AstConcat* స్థిరరాశి concatp = కొత్త AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3_Error_సంకేతం::WIDTHCONCAT, నిజం);
                valuep->fileline()->modifyWarnOff(V3_Error_సంకేతం::WIDTHCONCAT, నిజం);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        తిరిగి newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_కణుపు_DType* elemDTypep,
                                       స్థిరరాశి DTypeMap& dtypemap = DTypeMap{}) {
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = defaultp->lhssp()->clone_వృక్షం(అబద్ధం);
        Ast_కణుపు_DType* స్థిరరాశి elemDTypeSkipRefp = elemDTypep->skipRefp();
        స్థిరరాశి AstStructDType* స్థిరరాశి structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        స్థిరరాశి తార్కిక unpackedStruct = structp && !structp->packed();

        ఒకవేళ (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
            తిరిగి defaultp->clone_వృక్షం(అబద్ధం);
        }
        ఒకవేళ (VN_IS(valuep, నమూనా)) {
            VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
            తిరిగి defaultp->clone_వృక్షం(అబద్ధం);
        }
        ఒకవేళ (!valuep->dtypep()) userIterate(valuep, వెడల్పు_VP{SELF, BOTH}.p());
        తార్కిక సంపూర్ణ_Element = unpackedStruct;
        ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి విలువ_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            సంపూర్ణ_Element = unpackedStruct ? !విలువ_DTypep->skipRefp()->isIntegralOrPacked()
                                          : విలువ_DTypep->similarDType(elemDTypeSkipRefp);
        }
        ఒకవేళ (సంపూర్ణ_Element) {
            VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
            తిరిగి defaultp->clone_వృక్షం(అబద్ధం);
        }

        AstPatMember* స్థిరరాశి nestedDefaultp
            = కొత్త AstPatMember{defaultp->fileline(), valuep, శూన్యసూచిక, శూన్యసూచిక};
        nestedDefaultp->isDefault(నిజం);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : dtypemap) nestedDefaultp->కూడు_తదుపరి(నమోదు_2.రెండవ->clone_వృక్షం(అబద్ధం));
        Ast_నమూనా* స్థిరరాశి recursivePatternp = కొత్త Ast_నమూనా{defaultp->fileline(), nestedDefaultp};
        తిరిగి కొత్త AstPatMember{defaultp->fileline(), recursivePatternp, శూన్యసూచిక, శూన్యసూచిక};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_కణుపు_సమాసం* నమూనా_Default_Valuep(AstPatMember* defaultp, Ast_కణుపు_DType* elemDTypep) {
        ఒకవేళ (!defaultp) తిరిగి శూన్యసూచిక;
        AstPatMember* స్థిరరాశి elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_కణుపు_సమాసం* స్థిరరాశి valuep = నమూనా_Member_విలువ_Iterate(elemDefaultp);
        VL_DO_DANGLING(నెట్టు_Deletep(elemDefaultp), elemDefaultp);
        తిరిగి valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    శూన్య నమూనా_Concat_Array(Ast_నమూనా* nodep, AstUnpackArrayDType* arrayDtp) {
        స్థిరరాశి VNumRange range = arrayDtp->declRange();
        స్థిరరాశి పూర్ణాంకం elements = range.elements();
        AstConsArray* స్థిరరాశి newp = కొత్త AstConsArray{nodep->fileline(), arrayDtp};
        పూర్ణాంకం entn = 0;  // Number of elements given so far, from the left
        తార్కిక error = అబద్ధం;  // Reported an error, so the value is not complete
        కొరకు (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_కణుపు_సమాసం* స్థిరరాశి valuep = నమూనా_Member_విలువ_Iterate(patp);
            స్థిరరాశి uint64_t సూచీ = range.ascending() ? entn : elements - 1 - entn;
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ఒకవేళ (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_విలువలు(సూచీ, valuep);
                కొనసాగు;
            }
            స్థిరరాశి VCastable castable = Ast_కణుపు::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            ఒకవేళ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = నిజం;
            }
            newp->valuep(సూచీ, valuep);
            ++entn;
        }
        ఒకవేళ (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = నిజం;
        }
        ఒకవేళ (error) {
            VL_DO_DANGLING(నెట్టు_Deletep(newp), newp);
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}});
        } లేకపోతే {
            nodep->replaceWith(newp);
        }
    }
    శూన్య నమూనా_Array(Ast_నమూనా* nodep, Ast_కణుపు_Array_DType* arrayDtp, AstPatMember* defaultp) {
        ఒకవేళ (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            నమూనా_Concat_Array(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            తిరిగి;
        }
        స్థిరరాశి VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = pat_సదిశ_Map(nodep, range);
        UINFO(9, "ent " << range.ఎడమ() << " to " << range.కుడి());
        Ast_కణుపు* newp = శూన్యసూచిక;
        తార్కిక missed = అబద్ధం;  // Some element missed, so not a complete value
        కొరకు (పూర్ణాంకం entn = 0, ent = range.ఎడమ(); entn < range.elements();
             ++entn, ent += range.ఎడమ_గమ్యం_కుడి_Inc()) {
            Ast_కణుపు_సమాసం* valuep = శూన్యసూచిక;
            స్థిరరాశి స్వయంచాలక it = patmap.కనుగొను(ent);
            ఒకవేళ (it == patmap.చివర()) {
                ఒకవేళ (defaultp) {
                    valuep = నమూనా_Default_Valuep(defaultp, arrayDtp->subDTypep());
                } లేకపోతే {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = నిజం;
                }
            } లేకపోతే {
                AstPatMember* స్థిరరాశి patp = it->రెండవ;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = నమూనా_Member_విలువ_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                ఒకవేళ (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warn_మరిన్ని()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
                    valuep = కొత్త AstConst{patp->fileline(), AstConst::బిట్టు_False_Erroring{}};
                }
            }

            ఒకవేళ (valuep) {
                ఒకవేళ (VN_IS(arrayDtp, UnpackArrayDType)) {
                    ఒకవేళ (!newp) newp = కొత్త AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } లేకపోతే {  // Packed. Convert to concat for now.
                    ఒకవేళ (!newp) {
                        newp = valuep;
                    } లేకపోతే {
                        AstConcat* స్థిరరాశి concatp
                            = కొత్త AstConcat{valuep->fileline(), VN_AS(newp, కణుపు_సమాసం), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        ఒకవేళ (!patmap.ఖాళీలు()) nodep->v3error("Assignment pattern with too many elements");
        ఒకవేళ (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(నెట్టు_Deletep(newp), newp);
            newp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}};
        }
        ఒకవేళ (newp) {
            nodep->replaceWith(newp);
        } లేకపోతే {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* నమూనా_Const_Keyp(AstPatMember* patp) {
        ఒకవేళ (VN_IS(patp->keyp(), కణుపు_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            తిరిగి శూన్యసూచిక;
        }
        ఒకవేళ (patp->కీలకం_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->కీలకం_Exprp());  // May change
        AstConst* స్థిరరాశి constp = VN_CAST(patp->కీలకం_Exprp(), Const);
        ఒకవేళ (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        తిరిగి constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* నమూనా_Scalar_Keyp(AstPatMember* patp, Ast_కణుపు_DType* కీలకం_DTypep) {
        ఒకవేళ (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            తిరిగి శూన్యసూచిక;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        ఒకవేళ (కీలకం_DTypep->skipRefp()->isIntegralOrPacked() && patp->కీలకం_Exprp()) {
            userIterate(patp->కీలకం_Exprp(), వెడల్పు_VP{కీలకం_DTypep, PRELIM}.p());
            స్థిరరాశి పూర్ణాంకం వెడల్పు = std::max(patp->కీలకం_Exprp()->వెడల్పు(), కీలకం_DTypep->వెడల్పు());
            Ast_కణుపు_DType* స్థిరరాశి contextDTypep
                = కీలకం_DTypep->కనుగొను_బిట్టు_Or_Logic_DType(వెడల్పు, వెడల్పు, కీలకం_DTypep->numeric(), నిజం);
            iterateCheck(patp, "key", patp->కీలకం_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, అబద్ధం);
        }
        AstConst* స్థిరరాశి constp = నమూనా_Const_Keyp(patp);
        ఒకవేళ (!constp) తిరిగి శూన్యసూచిక;
        ఒకవేళ (constp->అంకె().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            తిరిగి శూన్యసూచిక;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, కీలకం_DTypep, EXTEND_EXP, అబద్ధం);
        Ast_కణుపు_సమాసం* స్థిరరాశి convp = patp->కీలకం_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_కణుపు_సమాసం* స్థిరరాశి keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        తిరిగి VN_AS(keyp->unlink_Fr_వెనుక(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_కణుపు_సమాసం* నమూనా_Keyp(AstConsAssoc*, AstPatMember* patp, Ast_కణుపు_DType* arrayDtp) {
        Ast_కణుపు_DType* స్థిరరాశి కీలకం_DTypep = VN_AS(arrayDtp, AssocArrayDType)->కీలకం_DTypep();
        ఒకవేళ (!కీలకం_DTypep->skipRefp()->isAggregateType() || !patp->కీలకం_Exprp()) {
            తిరిగి నమూనా_Scalar_Keyp(patp, కీలకం_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->కీలకం_Exprp(), కీలకం_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->కీలకం_Exprp());
        Ast_కణుపు_సమాసం* స్థిరరాశి foldedp = patp->కీలకం_Exprp();
        ఒకవేళ (!foldedp->is_Constant_వృక్షం()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            తిరిగి శూన్యసూచిక;
        }
        ఒకవేళ (foldedp->exists([](స్థిరరాశి AstConst* constp) { తిరిగి constp->అంకె().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            తిరిగి శూన్యసూచిక;
        }
        తిరిగి foldedp->unlink_Fr_వెనుక();
    }
    AstConst* నమూనా_Keyp(AstConsWildcard*, AstPatMember* patp, Ast_కణుపు_DType* arrayDtp) {
        తిరిగి నమూనా_Scalar_Keyp(patp, arrayDtp->కనుగొను_String_DType());
    }
    AstConst* నమూనా_Keyp(AstConsDynArray*, AstPatMember* patp, Ast_కణుపు_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        తిరిగి నమూనా_Scalar_Keyp(patp, arrayDtp->కనుగొను_Basic_DType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    మూస <రకం_పేరు T_అంశం, రకం_పేరు T_Cons>
    శూన్య క్రమీకరించు_Cons_వస్తువులు(T_Cons* nodep) {
        std::సదిశ<T_అంశం*> itemps;
        అయితే (T_అంశం* స్థిరరాశి itemp = nodep->itemsp()) {
            itemp->unlink_Fr_వెనుక();
            itemps.నెట్టు_వెనుక(itemp);
        }
        స్థిరరాశి స్వయంచాలక కీలకం_యొక్క = [](స్థిరరాశి T_అంశం* itemp) -> స్థిరరాశి V3_సంఖ్య& {
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి keyp = itemp->keyp();
            తిరిగి VN_AS(keyp, Const)->అంకె();
        };
        std::stable_క్రమీకరించు(itemps.begin(), itemps.చివర(), [&](స్థిరరాశి T_అంశం* ap, స్థిరరాశి T_అంశం* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            తిరిగి కీలకం_యొక్క(ap).isLtXZ(కీలకం_యొక్క(bp));
        });
        స్థిరరాశి T_అంశం* firstp = శూన్యసూచిక;  // First item with the current key
        కొరకు (T_అంశం* స్థిరరాశి itemp : itemps) {
            ఒకవేళ (firstp && కీలకం_యొక్క(firstp).isCaseEq(కీలకం_యొక్క(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } లేకపోతే {
                firstp = itemp;
            }
            nodep->కూడు_Itemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    మూస <రకం_పేరు T_అంశం, రకం_పేరు T_Cons>
    శూన్య నమూనా_Keyed(Ast_నమూనా* nodep, T_Cons* consp, Ast_కణుపు_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(నమూనా_Default_Valuep(defaultp, arrayDtp->subDTypep()));
        కొరకు (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            స్థిరరాశి స్వయంచాలక keyp = నమూనా_Keyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_కణుపు_సమాసం* స్థిరరాశి valuep = నమూనా_Member_విలువ_Iterate(patp);
            ఒకవేళ (keyp) {
                consp->కూడు_Itemsp(కొత్త T_అంశం{patp->fileline(), keyp, valuep});
            } లేకపోతే {
                VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        స్థిరరాశి AstAssocArrayDType* స్థిరరాశి assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        ఒకవేళ (!assocDtp || !assocDtp->కీలకం_DTypep()->skipRefp()->isAggregateType()) {
            క్రమీకరించు_Cons_వస్తువులు<T_అంశం>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_కణుపు_సమాసం* new_Cons_Dyn_Array_Or_వరుస(File_పంక్తి* fileline, స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        ఒకవేళ (VN_IS(dtypep, DynArrayDType)) తిరిగి కొత్త AstConsDynArray{fileline};
        ఒకవేళ (VN_IS(dtypep, వరుస_DType)) తిరిగి కొత్త Ast_Cons_వరుస{fileline};
        తిరిగి శూన్యసూచిక;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    మూస <రకం_పేరు T_అంశం, రకం_పేరు T_Cons>
    శూన్య కూడు_Cons_అంశం(T_Cons* consp, Ast_కణుపు_సమాసం* valuep) {
        Ast_కణుపు_DType* స్థిరరాశి arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        స్థిరరాశి తార్కిక isElement
            = Ast_కణుపు::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), శూన్యసూచిక)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి sub_వరుస_Dtp = VN_CAST(valuep->dtypep()->skipRefp(), వరుస_DType);
        ఒకవేళ (!isElement && !(sub_వరుస_Dtp && sub_వరుస_Dtp->boundp())) {
            T_Cons* స్థిరరాశి subp = Ast_కణుపు::cast<T_Cons>(valuep);
            ఒకవేళ (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                ఒకవేళ (subp->itemsp()) consp->కూడు_Itemsp(subp->itemsp()->unlink_Fr_వెనుక_With_తదుపరి());
                VL_DO_DANGLING(నెట్టు_Deletep(subp), subp);
                తిరిగి;
            }
            ఒకవేళ (Ast_Cons_వరుస* స్థిరరాశి queuep = VN_CAST(valuep, Cons_వరుస)) {
                కూడు_Cons_వస్తువులు(consp, queuep);
                తిరిగి;
            }
            ఒకవేళ (AstConsDynArray* స్థిరరాశి dynp = VN_CAST(valuep, ConsDynArray)) {
                కూడు_Cons_వస్తువులు(consp, dynp);
                తిరిగి;
            }
        }
        T_అంశం* స్థిరరాశి itemp = కొత్త T_అంశం{valuep->fileline(), valuep};
        consp->కూడు_Itemsp(itemp);
        // Width the value now, as constructors are widthed when created
        ఒకవేళ (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } లేకపోతే {
            user_Iterate_And_తదుపరి(valuep, వెడల్పు_VP{arrayDtp, FINAL}.p());
        }
    }
    శూన్య కూడు_Cons_అంశం(Ast_కణుపు_సమాసం* consp, Ast_కణుపు_సమాసం* valuep) {
        ఒకవేళ (AstConsDynArray* స్థిరరాశి dynp = VN_CAST(consp, ConsDynArray)) {
            కూడు_Cons_అంశం<Ast_Cons_Dyn_Array_అంశం>(dynp, valuep);
        } లేకపోతే {
            కూడు_Cons_అంశం<Ast_Cons_వరుస_అంశం>(VN_AS(consp, Cons_వరుస), valuep);
        }
    }
    మూస <రకం_పేరు T_Sub>
    శూన్య కూడు_Cons_వస్తువులు(Ast_కణుపు_సమాసం* consp, T_Sub* subp) {
        అయితే (స్వయంచాలక* స్థిరరాశి itemp = subp->itemsp()) {
            Ast_కణుపు_సమాసం* స్థిరరాశి valuep = itemp->valuep()->unlink_Fr_వెనుక();
            VL_DO_DANGLING(నెట్టు_Deletep(itemp->unlink_Fr_వెనుక()), itemp);
            కూడు_Cons_అంశం(consp, valuep);
        }
        VL_DO_DANGLING(నెట్టు_Deletep(subp), subp);
    }
    శూన్య నమూనా_Dyn_Array_Or_వరుస(Ast_నమూనా* nodep, Ast_కణుపు_DType* arrayp, AstPatMember* defaultp) {
        Ast_కణుపు_సమాసం* స్థిరరాశి newp = new_Cons_Dyn_Array_Or_వరుస(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        స్థిరరాశి AstPatMember* స్థిరరాశి firstp = VN_AS(nodep->itemsp(), PatMember);
        స్థిరరాశి తార్కిక keyed = defaultp || (firstp && firstp->keyp());
        ఒకవేళ (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            నమూనా_Keyed<Ast_Cons_Dyn_Array_అంశం>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            తిరిగి;
        }
        స్థిరరాశి అక్షరం* స్థిరరాశి kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        ఒకవేళ (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        కొరకు (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ఒకవేళ (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            కూడు_Cons_అంశం(newp, నమూనా_Member_విలువ_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    శూన్య నమూనా_Basic(Ast_నమూనా* nodep, Ast_కణుపు_DType* vdtypep, AstPatMember* defaultp) {
        స్థిరరాశి AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        స్థిరరాశి VNumRange range = bdtypep->declRange();
        PatVecMap patmap = pat_సదిశ_Map(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_కణుపు_సమాసం* newp = శూన్యసూచిక;
        కొరకు (స్థిరరాశి పూర్ణాంకం ent : range.seq_Hi_గమ్యం_Lo()) {
            Ast_కణుపు_సమాసం* valuep = శూన్యసూచిక;
            స్థిరరాశి స్వయంచాలక it = patmap.కనుగొను(ent);
            ఒకవేళ (it == patmap.చివర()) {
                ఒకవేళ (defaultp) {
                    valuep = నమూనా_Default_Valuep(defaultp, nodep->కనుగొను_బిట్టు_DType());
                } లేకపోతే {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } లేకపోతే {
                AstPatMember* స్థిరరాశి patp = it->రెండవ;
                patmap.erase(it);
                patp->dtypep(nodep->కనుగొను_బిట్టు_DType());
                valuep = నమూనా_Member_విలువ_Iterate(patp);
            }
            ఒకవేళ (valuep) {  // Packed. Convert to concat for now.
                ఒకవేళ (!newp) {
                    newp = valuep;
                } లేకపోతే {
                    AstConcat* స్థిరరాశి concatp = కొత్త AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        ఒకవేళ (!patmap.ఖాళీలు()) nodep->v3error("Assignment pattern with too many elements");
        ఒకవేళ (newp) {
            nodep->replaceWith(newp);
        } లేకపోతే {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_కణుపు_సమాసం* నమూనా_Member_విలువ_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, వెడల్పు_VP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_కణుపు_సమాసం* valuep = patp->lhssp()->unlink_Fr_వెనుక();
        ఒకవేళ (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            ఒకవేళ (AstConst* స్థిరరాశి newp = V3_వెడల్పు_Commit::new_If_Const_Commit_కొలత(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        తిరిగి valuep;
    }

    స్థిర శూన్య checkEventAssignment(స్థిరరాశి Ast_కణుపు_కేటాయించు* స్థిరరాశి asgnp) {
        string unsupEvtAsgn;
        ఒకవేళ (!ఉపయోగిస్తుంది_Dynamic_Scheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        ఒకవేళ (VN_IS(asgnp->rhsp(), CReset)) తిరిగి;
        ఒకవేళ (asgnp->rhsp()->dtypep()->isEvent() && !ఉపయోగిస్తుంది_Dynamic_Scheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.ఖాళీలు() ? "from" : " and from");
        }
        ఒకవేళ (!unsupEvtAsgn.ఖాళీలు()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warn_మరిన్ని()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warn_మరిన్ని()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    స్థిర తార్కిక ఉపయోగిస్తుంది_Dynamic_Scheduler(Ast_కణుపు* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        అయితే (నిజం) {
            AstVarRef* స్థిరరాశి vrefp = VN_CAST(nodep, VarRef);
            ఒకవేళ (vrefp) తిరిగి ఉపయోగిస్తుంది_Dynamic_Scheduler(vrefp);
            ఒకవేళ (VN_IS(nodep, MemberSel)) {
                తిరిగి నిజం;
            } లేకపోతే ఒకవేళ (Ast_కణుపు_Sel* selp = VN_CAST(nodep, కణుపు_Sel)) {
                nodep = selp->fromp();
            } లేకపోతే {
                తిరిగి అబద్ధం;
            }
        }
    }

    స్థిర తార్కిక ఉపయోగిస్తుంది_Dynamic_Scheduler(AstVarRef* vrefp) {
        తిరిగి VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    శూన్య సందర్శించు(AstPatMember* nodep) override {
        Ast_కణుపు_DType* స్థిరరాశి vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, వెడల్పు_VP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), కేటాయించు_3, FINAL, vdtypep, EXTEND_LHS);
    }
    పూర్ణాంకం సందర్శించు_Pat_Member_Rep(AstPatMember* nodep) {
        uint32_t times = 1;
        ఒకవేళ (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep->repp(), Const);
            ఒకవేళ (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } లేకపోతే {
                times = constp->గమ్యం_UInt();
            }
            ఒకవేళ (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlink_Fr_వెనుక_With_తదుపరి()
                ->delete_వృక్షం();  // Done with replicate before cloning
        }
        తిరిగి times;
    }

    శూన్య సందర్శించు(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_కణుపు* propp = nodep->propp();
            అయితే (AstLogNot* స్థిరరాశి notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* స్థిరరాశి untilp = VN_CAST(propp, Until);
            ఒకవేళ (untilp && VN_IS(nodep->backp(), కణుపు_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().is_సమితి_False() || !v3Global.opt.timing().is_సమితి_True())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(కొత్త AstConst{untilp->fileline(), AstConst::బిట్టు_False{}});
                VL_DO_DANGLING(నెట్టు_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_తదుపరి(nodep->sensesp(), శూన్యసూచిక);
            ఒకవేళ (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtype_సమితి_బిట్టు();
        }
    }

    తార్కిక మొదటి_New_Statement_Ok_Recurse(Ast_కణుపు* nodep) {
        ఒకవేళ (AstVar* స్థిరరాశి varp = VN_CAST(nodep, Var)) {
            ఒకవేళ (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) తిరిగి నిజం;
        }
        ఒకవేళ (Ast_కేటాయించు* స్థిరరాశి aitemp = VN_CAST(nodep, కేటాయించు)) {
            ఒకవేళ (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) తిరిగి నిజం;
        }
        ఒకవేళ (AstInitialStaticStmt* స్థిరరాశి aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            కొరకు (Ast_కణుపు* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ఒకవేళ (!మొదటి_New_Statement_Ok_Recurse(stmtp)) తిరిగి అబద్ధం;
            }
            తిరిగి నిజం;
        }
        ఒకవేళ (AstInitialAutomaticStmt* స్థిరరాశి aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            కొరకు (Ast_కణుపు* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ఒకవేళ (!మొదటి_New_Statement_Ok_Recurse(stmtp)) తిరిగి అబద్ధం;
            }
            తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }

    //--------------------
    // Top levels

    మూస <రకం_పేరు Case_అంశం>
    శూన్య handleCaseType(Ast_కణుపు* casep, Ast_కణుపు_సమాసం* exprp, Case_అంశం* itemsp) {
        Ast_Attr_యొక్క* స్థిరరాశి exprap = VN_CAST(exprp, Attr_యొక్క);
        ఒకవేళ (!exprap) తిరిగి;
        ఒకవేళ (exprap->attrType() != VAttrType::TYPEID) తిరిగి;

        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి సమాసం_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << సమాసం_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_పంక్తి* స్థిరరాశి newfl = casep->fileline();
        newfl->warnOff(V3_Error_సంకేతం::CASEINCOMPLETE, నిజం);  // Side effect of transform
        newfl->warnOff(V3_Error_సంకేతం::CASEOVERLAP, నిజం);  // Side effect of transform
        casep->fileline(newfl);
        కొరకు (Case_అంశం* itemp = itemsp; itemp; itemp = Ast_కణుపు::as<Case_అంశం>(itemp->nextp())) {
            ఒకవేళ (itemp->isDefault()) కొనసాగు;
            తార్కిక తగిలింది = అబద్ధం;
            కొరకు (Ast_కణుపు* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                స్థిరరాశి Ast_Attr_యొక్క* స్థిరరాశి condAttrp = VN_CAST(condp, Attr_యొక్క);
                ఒకవేళ (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } లేకపోతే {
                    Ast_కణుపు_DType* స్థిరరాశి condDtp = condAttrp->dtypep();
                    ఒకవేళ (Ast_కణుపు::computeCastable(సమాసం_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        తగిలింది = నిజం;
                        ఆపు;
                    }
                }
            }
            నెట్టు_Deletep(itemp->condsp()->unlink_Fr_వెనుక_With_తదుపరి());
            // Item condition becomes constant 1 if hits else 0
            itemp->కూడు_Condsp(కొత్త AstConst{newfl, AstConst::బిట్టు_True{}, తగిలింది});
        }
        exprap->replaceWith(కొత్త AstConst{newfl, AstConst::బిట్టు_True{}});
        VL_DO_DANGLING(నెట్టు_Deletep(exprap), exprap);
    }

    మూస <రకం_పేరు Case_అంశం>
    శూన్య handleCase(Ast_కణుపు* casep, Ast_కణుపు_సమాసం* exprp, Case_అంశం* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_కణుపు_DType* subDTypep = exprp->dtypep();
        కొరకు (Case_అంశం* itemp = itemsp; itemp; itemp = Ast_కణుపు::as<Case_అంశం>(itemp->nextp())) {
            కొరకు (Ast_కణుపు* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ఒకవేళ (condp->dtypep() == subDTypep) కొనసాగు;

                ఒకవేళ (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->కనుగొను_Double_DType();
                } లేకపోతే ఒకవేళ (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->కనుగొను_String_DType();
                } లేకపోతే {
                    స్థిరరాశి పూర్ణాంకం వెడల్పు = std::max(subDTypep->వెడల్పు(), condp->వెడల్పు());
                    స్థిరరాశి పూర్ణాంకం mwidth = std::max(subDTypep->వెడల్పు_Min(), condp->వెడల్పు_Min());
                    స్థిరరాశి తార్కిక issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->కనుగొను_బిట్టు_Or_Logic_DType(
                        వెడల్పు, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        కొరకు (Case_అంశం* itemp = itemsp; itemp; itemp = Ast_కణుపు::as<Case_అంశం>(itemp->nextp())) {
            కొరకు (Ast_కణుపు *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    శూన్య సందర్శించు(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
        కొరకు (Ast_Gen_Case_అంశం *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), Gen_Case_అంశం);
            కొరకు (Ast_కణుపు *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, వెడల్పు_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    శూన్య సందర్శించు(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_తదుపరి(nodep->initsp(), శూన్యసూచిక);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_తదుపరి(nodep->incsp(), శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    శూన్య సందర్శించు(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
        కొరకు (Ast_Case_అంశం *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_అంశం);  // Prelim may cause the node to get replaced
            user_Iterate_And_తదుపరి(itemp->stmtsp(), శూన్యసూచిక);
            కొరకు (Ast_కణుపు *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, వెడల్పు_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    శూన్య సందర్శించు(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_కణుపు_DType* స్థిరరాశి అంశం_DTypep = nodep->కనుగొను_UInt32_DType();
        కొరకు (Ast_Case_అంశం *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_అంశం);  // Prelim may cause the node to get replaced
            user_Iterate_And_తదుపరి(itemp->stmtsp(), శూన్యసూచిక);
            కొరకు (Ast_కణుపు *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, అంశం_DTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    శూన్య సందర్శించు(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_తదుపరి(nodep->stmtsp(), శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    శూన్య సందర్శించు(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    శూన్య సందర్శించు(Ast_కణుపు_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_తదుపరి(nodep->thensp(), శూన్యసూచిక);
        user_Iterate_And_తదుపరి(nodep->elsesp(), శూన్యసూచిక);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    శూన్య సందర్శించు(Ast_సమాసం_Stmt* nodep) override {
        user_Iterate_And_తదుపరి(nodep->stmtsp(), శూన్యసూచిక);
        // expected result is same as parent's expected result
        user_Iterate_And_తదుపరి(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Foreach* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        nodep->did_వెడల్పు(నిజం);
        స్థిరరాశి Ast_Foreach_శీర్షిక* స్థిరరాశి headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_తదుపరి(headerp->fromp(), వెడల్పు_VP{SELF, BOTH}.p());
        Ast_కణుపు_సమాసం* స్థిరరాశి fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_కణుపు_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        కొరకు (Ast_కణుపు *argsp = headerp->elementsp(), *తదుపరి_argsp; argsp; argsp = తదుపరి_argsp) {
            తదుపరి_argsp = argsp->nextp();
            స్థిరరాశి తార్కిక ఖాళీలు = VN_IS(argsp, ఖాళీలు_2);
            AstVar* స్థిరరాశి varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || ఖాళీలు, argsp, "Missing foreach loop variable");
            ఒకవేళ (varp) varp->వాడిన_Loop_Idx(నిజం);
            ఒకవేళ (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
                తిరిగి;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            ఒకవేళ (VN_IS(fromDtp, కణుపు_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, వరుస_DType)) {
                // Nothing special here
            } లేకపోతే ఒకవేళ (AstBasicDType* స్థిరరాశి adtypep = VN_CAST(fromDtp, BasicDType)) {
                ఒకవేళ (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
                    తిరిగి;
                }
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstAssocArrayDType* స్థిరరాశి adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->కీలకం_DTypep());
            } లేకపోతే {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
                తిరిగి;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_కణుపు* స్థిరరాశి bodyp = nodep->bodyp();
        user_Iterate_And_తదుపరి(bodyp, శూన్యసూచిక);
        ఒకవేళ (AstForeach* స్థిరరాశి loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convert_గమ్యం_While(loopp), loopp, nodep);
            తిరిగి;
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు_కేటాయించు* nodep) override {
        // IEEE-2012 10.7, 11.8.2, 11.8.3, 11.5:  (Careful of 11.8.1 which is
        //                  only one step; final dtype depends on assign LHS.)
        //    Determine RHS type width and signing
        //    Propagate type down to *non-self-determined* operators
        //       Real propagates only across one operator if one side is real -
        //       handled in each visitor.
        //    Then LHS sign-extends only if *RHS* is signed
        assertAtStatement(nodep);
        {
            // UINFOTREE(1, nodep, "", "assin:");
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->వెడల్పు_Sized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                ఒకవేళ (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                    తిరిగి;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            ప్రవాహం_కేటాయించు_క్రింది(nodep);
            Ast_కణుపు_DType* స్థిరరాశి lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            ఒకవేళ (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి lhsDtp = lhsDTypep->skipRefp();
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                స్థిరరాశి తార్కిక lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                స్థిరరాశి తార్కిక rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                ఒకవేళ (lhsIsUnpackArray && rhsIsUnpackArray) {
                    ఒకవేళ (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_కణుపు_Var_Ref, Ast_కణుపు_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), కణుపు_Var_Ref), VN_CAST(nodep->rhsp(), కణుపు_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                ఒకవేళ (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), కణుపు_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterate_Check_కేటాయించు(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        ఒకవేళ (VN_IS(nodep, కేటాయించు_Force)) checkForceReleaseLhs(nodep, nodep->lhsp());
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి controlp = nodep->timingControlp()) {
            ఒకవేళ (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం(), controlp);
            } లేకపోతే ఒకవేళ (nodep->fileline()->timingOn() && v3Global.opt.timing().is_సమితి_True()) {
                iterateNull(controlp);
            } లేకపోతే {
                ఒకవేళ (nodep->fileline()->timingOn()) {
                    ఒకవేళ (v3Global.opt.timing().is_సమితి_False()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } లేకపోతే {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_వెనుక_With_తదుపరి()->delete_వృక్షం(), controlp);
            }
        }
        ఒకవేళ (VN_IS(nodep->rhsp(), ఖాళీలు_వరుస)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            ఒకవేళ (!VN_IS(lhsDtp, వరుస_DType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
                తిరిగి;
            }
            Ast_పద్ధతి_Call* స్థిరరాశి newp
                = కొత్త Ast_పద్ధతి_Call{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(), "delete"};
            newp->dtype_సమితి_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            // Need to now convert it
            సందర్శించు(newp);
            తిరిగి;
        }
        ఒకవేళ (స్థిరరాశి AstNewDynamic* స్థిరరాశి dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            ఒకవేళ (!dynp->rhsp()) {
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlink_Fr_వెనుక()};
            } లేకపోతే {
                newp = కొత్త AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlink_Fr_వెనుక()};
                newp->కూడు_Pinsp(dynp->rhsp()->unlink_Fr_వెనుక());
            }
            newp->did_వెడల్పు(నిజం);
            newp->protect(అబద్ధం);
            newp->dtype_సమితి_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        ఒకవేళ (AstCMethodHard* స్థిరరాశి slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod కేటాయించు_పద్ధతి;
            ఒకవేళ (slicep->పద్ధతి() == VCMethod::DYN_ముక్క) {
                కేటాయించు_పద్ధతి = VCMethod::DYN_ముక్క_కేటాయించు;
            } లేకపోతే ఒకవేళ (slicep->పద్ధతి() == VCMethod::DYN_ముక్క_వెనుక_వెనుక) {
                కేటాయించు_పద్ధతి = VCMethod::DYN_ముక్క_కేటాయించు_వెనుక_వెనుక;
            } లేకపోతే ఒకవేళ (slicep->పద్ధతి() == VCMethod::DYN_ముక్క_FRONT_వెనుక) {
                కేటాయించు_పద్ధతి = VCMethod::DYN_ముక్క_కేటాయించు_FRONT_వెనుక;
            } లేకపోతే {
                కేటాయించు_పద్ధతి = VCMethod::_ENUM_MAX;  // not a slice
            }
            ఒకవేళ (కేటాయించు_పద్ధతి != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_కణుపు_సమాసం* స్థిరరాశి fromp = slicep->fromp()->unlink_Fr_వెనుక();
                // Collect existing slice index pins (lsb, msb)
                Ast_కణుపు_సమాసం* స్థిరరాశి lsbp = slicep->pinsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి msbp = slicep->pinsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
                AstCMethodHard* స్థిరరాశి newp
                    = కొత్త AstCMethodHard{nodep->fileline(), fromp, కేటాయించు_పద్ధతి};
                newp->కూడు_Pinsp(lsbp);
                newp->కూడు_Pinsp(msbp);
                newp->కూడు_Pinsp(rhsp);
                newp->did_వెడల్పు(నిజం);
                newp->protect(అబద్ధం);
                newp->dtype_సమితి_Void();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            }
        }

        ఒకవేళ (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.సమితి_Assigns_Events();
        }
    }

    శూన్య సందర్శించు(AstRSRule* nodep) override {
        ఒకవేళ (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_తదుపరి(nodep->prodlistsp(), శూన్యసూచిక);
        user_Iterate_And_తదుపరి(nodep->బరువు_Stmtsp(), శూన్యసూచిక);
    }

    శూన్య సందర్శించు(AstRelease* nodep) override {
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->వెడల్పు_Sized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    శూన్య సందర్శించు(AstDeassign* nodep) override {
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->వెడల్పు_Sized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    స్థిర తార్కిక isFormatNonNumericArg(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        తిరిగి dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, వరుస_DType)  //
               || (VN_IS(dtypep, కణుపు_UOr_Struct_DType)
                   && !VN_AS(dtypep, కణుపు_UOr_Struct_DType)->packed());
    }

    శూన్య checkFormatNumericArg(Ast_కణుపు* argp, అక్షరం ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        ఒకవేళ (std::tolower(ch) == 'd') తిరిగి;
        // In contrast, %x of a class reference, causes errors on several simulators.
        ఒకవేళ (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warn_మరిన్ని() << "... Suggest use '%s'");
    }

    శూన్య సందర్శించు(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    శూన్య సందర్శించు(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_తదుపరి(nodep->fmtp(), వెడల్పు_VP{SELF, BOTH}.p());
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(Ast_గమ్యం_String_N* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtype_సమితి_String();
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        ఒకవేళ (m_vup && !m_vup->prelim()) తిరిగి;  // Can be called as statement or function
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;
        //
        UINFO(9, "  Display in " << nodep->పాఠ్యం());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        ఒకవేళ (!nodep->సమాసం_Format_2()) {  // Non-constant format, can't check format string types
            సందర్శించు_sformatf_format(nodep);
        }

        Ast_కణుపు_సమాసం* oldExprsp = nodep->exprsp();
        ఒకవేళ (nodep->సమాసం_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), కణుపు_సమాసం);
        }
        ఒకవేళ (oldExprsp) oldExprsp->unlink_Fr_వెనుక_With_తదుపరి();
        అయితే (Ast_కణుపు_సమాసం* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), కణుపు_సమాసం);
            ఒకవేళ (oldExprsp) oldExprsp->unlink_Fr_వెనుక_With_తదుపరి();
            ఒకవేళ (VN_IS(argp, SFormatArg)) {
                nodep->కూడు_Exprsp(argp);
                కొనసాగు;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_కణుపు_DType* స్థిరరాశి dtypep = argp->dtypep()->skipRefp();
            ఒకవేళ (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } లేకపోతే ఒకవేళ (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } లేకపోతే ఒకవేళ (isFormatNonNumericArg(dtypep)) {
                ఒకవేళ (AstClassRefDType* స్థిరరాశి classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ఒకవేళ (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(నిజం);
                    }
                } లేకపోతే {
                    // Class handles inside containers print type names, not their members.
                    Ast_కణుపు_DType* కణుపు_Dtypep = dtypep;
                    అయితే (కణుపు_Dtypep) {
                        కణుపు_Dtypep = కణుపు_Dtypep->skipRefp();
                        ఒకవేళ (Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి uOrStructDTypep
                            = VN_CAST(కణుపు_Dtypep, కణుపు_UOr_Struct_DType)) {
                            uOrStructDTypep->సమితి_Emit_గమ్యం_String();
                            v3Global.hasPrintedObjects(నిజం);
                        }
                        కణుపు_Dtypep = కణుపు_Dtypep->subDTypep();
                    }
                }
                Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త Ast_గమ్యం_String_N{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } లేకపోతే ఒకవేళ (nodep->సమాసం_Format_2()) {
                ఒకవేళ (AstEnumDType* స్థిరరాశి enumDtp = formatEnumDType(argp)) {
                    nodep->కూడు_Exprsp(newFormatEnumArg(argp, enumDtp));
                    కొనసాగు;
                }
            }
            ఒకవేళ (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            ఒకవేళ (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->కూడు_Exprsp(argp);
            } లేకపోతే {
                nodep->కూడు_Exprsp(కొత్త AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->పాఠ్యం());
    }
    శూన్య సందర్శించు(AstCReset* nodep) override {
        assert_At_సమాసం(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    శూన్య సందర్శించు(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    శూన్య సందర్శించు(AstConstraintRef* nodep) override { userIterateChildren(nodep, శూన్యసూచిక); }
    శూన్య సందర్శించు(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        ఒకవేళ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        ఒకవేళ (!m_paramsOnly) {
            nodep->foreach([ఇది](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    కొత్త AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string పాఠ్యం = VString::dequotePercent(nodep->fmtp()->పాఠ్యం());
            ఒకవేళ (పాఠ్యం.ఖాళీలు()) పాఠ్యం = "Elaboration system task message (IEEE 1800-2023 20.11)";
            ఎంపిక (nodep->displayType()) {
            స్థితి VDisplayType::DT_INFO: nodep->v3warn(USERINFO, పాఠ్యం); ఆపు;
            స్థితి VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, పాఠ్యం); ఆపు;
            స్థితి VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, పాఠ్యం); ఆపు;
            స్థితి VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, పాఠ్యం); ఆపు;
            అప్రమేయ: UASSERT_OBJ(అబద్ధం, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
        }
    }
    శూన్య సందర్శించు(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        ఒకవేళ (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    శూన్య సందర్శించు(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    శూన్య సందర్శించు(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    శూన్య సందర్శించు(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    శూన్య సందర్శించు(AstFError* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_తదుపరి(nodep->strp(), వెడల్పు_VP{SELF, BOTH}.p());
        }
    }
    శూన్య సందర్శించు(AstFEof* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    శూన్య సందర్శించు(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        ఒకవేళ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    శూన్య సందర్శించు(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    శూన్య సందర్శించు(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    శూన్య సందర్శించు(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->స్థానభ్రంశం(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    శూన్య సందర్శించు(AstFGetC* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    శూన్య సందర్శించు(AstFGetS* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_తదుపరి(nodep->strgp(), వెడల్పు_VP{SELF, BOTH}.p());
        }
    }
    శూన్య సందర్శించు(AstFUngetC* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    శూన్య సందర్శించు(AstFRead* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->memp(), వెడల్పు_VP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            ఒకవేళ (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            ఒకవేళ (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    శూన్య సందర్శించు(AstFScanF* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_తదుపరి(nodep->exprsp(), వెడల్పు_VP{SELF, BOTH}.p());
        }
    }
    శూన్య సందర్శించు(AstSScanF* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->fromp(), వెడల్పు_VP{SELF, BOTH}.p());
            user_Iterate_And_తదుపరి(nodep->exprsp(), వెడల్పు_VP{SELF, BOTH}.p());
        }
    }
    శూన్య సందర్శించు(Ast_దొంతర_Trace_F* nodep) override { nodep->dtype_సమితి_String(); }
    శూన్య సందర్శించు(AstSysIgnore* nodep) override {
        user_Iterate_And_తదుపరి(nodep->exprsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstSystemF* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(Ast_కణుపు_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_తదుపరి(nodep->memp(), వెడల్పు_VP{SELF, BOTH}.p());
        స్థిరరాశి Ast_కణుపు_DType* subp = శూన్యసూచిక;
        ఒకవేళ (స్థిరరాశి AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            ఒకవేళ (!adtypep->కీలకం_DTypep()->skipRefp()->basicp()
                || !adtypep->కీలకం_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } లేకపోతే ఒకవేళ (స్థిరరాశి AstDynArrayDType* స్థిరరాశి adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } లేకపోతే {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        ఒకవేళ (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        user_Iterate_And_తదుపరి(nodep->lsbp(), వెడల్పు_VP{SELF, BOTH}.p());
        user_Iterate_And_తదుపరి(nodep->msbp(), వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(Ast_Test_Plus_వాదనలు* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    శూన్య సందర్శించు(Ast_విలువ_Plus_వాదనలు* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_తదుపరి(nodep->outp(), వెడల్పు_VP{SELF, BOTH}.p());
        }
    }
    శూన్య సందర్శించు(Ast_సమయం_Format* nodep) override {
        assertAtStatement(nodep);
        ఒకవేళ (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        ఒకవేళ (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        ఒకవేళ (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        ఒకవేళ (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    శూన్య సందర్శించు(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_తదుపరి(nodep->passsp(), శూన్యసూచిక);
        user_Iterate_And_తదుపరి(nodep->failsp(), శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
    }
    శూన్య సందర్శించు(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_తదుపరి(nodep->passsp(), శూన్యసూచిక);
        user_Iterate_And_తదుపరి(nodep->failsp(), శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_తదుపరి(nodep->passsp(), శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    తార్కిక pinInstArrayElement(AstPin* nodep, స్థిరరాశి AstRange* rangep, పూర్ణాంకం idx) {
        Ast_కణుపు_DType* స్థిరరాశి modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_కణుపు_DType* స్థిరరాశి conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        ఒకవేళ (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) తిరిగి నిజం;
        // Types don't match, but all dimensions done, still need the usual checks
        ఒకవేళ (!rangep) తిరిగి అబద్ధం;

        స్థిరరాశి AstRange* స్థిరరాశి subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        పూర్ణాంకం subElems = 1;
        కొరకు (స్థిరరాశి AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        స్థిరరాశి పూర్ణాంకం elements = rangep->elementsConst();
        స్థిరరాశి పూర్ణాంకం from_ఎడమ = idx / subElems;

        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrp = VN_CAST(conDTypep, UnpackArrayDType);
        స్థిరరాశి uint32_t conDims = conDTypep->dimensions(అబద్ధం).రెండవ;
        స్థిరరాశి uint32_t modDims = modDTypep->dimensions(అబద్ధం).రెండవ;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        ఒకవేళ (arrp && conDims != modDims) {
            ఒకవేళ (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                user_Iterate_And_తదుపరి(nodep->exprp(),
                                   వెడల్పు_VP{conDTypep, FINAL, ప్రవాహం_USE_కేటాయించు}.p());
                తిరిగి నిజం;
            }
            // ArraySel index is counted from lo
            స్థిరరాశి పూర్ణాంకం బిట్టు_2 = arrp->declRange().ascending() ? from_ఎడమ : elements - 1 - from_ఎడమ;
            // Connection is self-determined
            user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{conDTypep, FINAL, ప్రవాహం_USE_కేటాయించు}.p());
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = VN_AS(nodep->exprp(), కణుపు_సమాసం)->unlink_Fr_వెనుక();
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి elemp = newIfaceArrayElement(fromp, బిట్టు_2)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(నెట్టు_Deletep(fromp), fromp);
            } లేకపోతే {
                AstArraySel* స్థిరరాశి selp = కొత్త AstArraySel{fromp->fileline(), fromp, బిట్టు_2};
                selp->did_వెడల్పు(నిజం);
                nodep->exprp(selp);
            }
            తిరిగి pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        ఒకవేళ (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) తిరిగి అబద్ధం;

        // Packed connection
        స్థిరరాశి పూర్ణాంకం modwidth = modDTypep->వెడల్పు();
        స్థిరరాశి పూర్ణాంకం conwidth = conDTypep->వెడల్పు();

        // Connection has the width of the port, connect it to every element
        ఒకవేళ (conwidth == modwidth) తిరిగి అబద్ధం;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        ఒకవేళ (conwidth == modwidth * elements * subElems) {
            ఒకవేళ (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->ఎడమ_Const() << ":"
                                                     << rangep->కుడి_Const() << "]");
            }
            స్థిరరాశి పూర్ణాంకం selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{conDTypep, FINAL, ప్రవాహం_USE_కేటాయించు}.p());
            Ast_కణుపు_సమాసం* స్థిరరాశి fromp = VN_AS(nodep->exprp(), కణుపు_సమాసం)->unlink_Fr_వెనుక();
            AstSel* స్థిరరాశి selp = కొత్త AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_ఎడమ), selwidth};
            selp->did_వెడల్పు(నిజం);
            nodep->exprp(selp);
            తిరిగి pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{conDTypep, FINAL, ప్రవాహం_USE_కేటాయించు}.p());
        తిరిగి నిజం;
    }

    శూన్య సందర్శించు(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        ఒకవేళ (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* స్థిరరాశి concatp = VN_CAST(nodep->exprp(), Replicate);
            స్థిరరాశి AstConst* స్థిరరాశి countp = concatp ? VN_CAST(concatp->countp(), Const) : శూన్యసూచిక;
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            ఒకవేళ (countp && countp->గమ్యం_UInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                Ast_నమూనా* స్థిరరాశి patternp = కొత్త Ast_నమూనా{concatp->fileline(), శూన్యసూచిక};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(నెట్టు_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_లోపల_Param_Override);
            m_లోపల_Param_Override = is_Override_నమూనా(nodep->exprp());
            userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        } లేకపోతే ఒకవేళ (!m_paramsOnly) {
            ఒకవేళ (!nodep->modVarp()->did_వెడల్పు()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), శూన్యసూచిక);
            }
            ఒకవేళ (!nodep->exprp()) {  // No-connect
                ఒకవేళ (nodep->modVarp()->is_Iface_Array_విభజించు()) expandIfaceArrayPin(nodep);
                తిరిగి;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            స్థిరరాశి VDirection pinDirection = nodep->modVarp()->direction();
            ఒకవేళ (VN_IS(nodep->exprp(), కణుపు_ప్రవాహం) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_కణుపు_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_కణుపు_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            ఒకవేళ (స్థిరరాశి AstIfaceRefDType* స్థిరరాశి modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                స్థిరరాశి AstIfaceRefDType* స్థిరరాశి conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                ఒకవేళ (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_కణుపు_DType* subDTypep = modDTypep;
            స్థిరరాశి పూర్ణాంకం modwidth = modDTypep->వెడల్పు();
            పూర్ణాంకం conwidth = conDTypep->వెడల్పు();
            ఒకవేళ (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_తదుపరి(nodep->exprp(),
                                   వెడల్పు_VP{subDTypep, FINAL, ప్రవాహం_USE_కేటాయించు}.p());
            } లేకపోతే {
                ఒకవేళ (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    ఒకవేళ (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        ఒకవేళ (nodep->modVarp()->is_Iface_Array_విభజించు()) expandIfaceArrayPin(nodep);
                        తిరిగి;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->వెడల్పు();
                }
                ఒకవేళ (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } లేకపోతే ఒకవేళ (nodep->modVarp()->isTristate()) {
                    ఒకవేళ (modwidth != conwidth) {
                        // Ideally should call pinReconnectSimple which would tolerate this
                        // then have a conversion warning
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                      << " to inout signal requires " << modwidth
                                                      << " bits, but connection's "
                                                      << nodep->exprp()->prettyTypeName()
                                                      << " generates " << conwidth << " bits.");
                        // otherwise would need some mess to force both sides to proper size
                    }
                } లేకపోతే ఒకవేళ (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                ఒకవేళ ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి సమాసం_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                ఒకవేళ (సమాసం_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_కొలత(conDTypep) != unpacked_Array_కొలత(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_కొలత(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_కొలత(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } లేకపోతే ఒకవేళ ((సమాసం_Arrayp && !modArrayp) || (!సమాసం_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (సమాసం_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } లేకపోతే {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_కణుపు_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), కణుపు_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterate_Check_కేటాయించు(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            ఒకవేళ (nodep->modVarp()->is_Iface_Array_విభజించు()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        ఒకవేళ (!m_paramsOnly) {
            ఒకవేళ (VN_IS(nodep->modp(), Not_దొరికింది_Module)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_మార్గం_Looked_సందేశం(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            ఒకవేళ (nodep->rangep()) user_Iterate_And_తదుపరి(nodep->rangep(), వెడల్పు_VP{SELF, BOTH}.p());
            user_Iterate_And_తదుపరి(nodep->pinsp(), శూన్యసూచిక);
            // If element of an instance array, pins are now connected, no longer need the ranges
            ఒకవేళ (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                నెట్టు_Deletep(nodep->rangep()->unlink_Fr_వెనుక_With_తదుపరి());
            }
        }
        user_Iterate_And_తదుపరి(nodep->paramsp(), శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstGatePin* nodep) override {
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->rangep(), వెడల్పు_VP{SELF, BOTH}.p());
            user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి pinDTypep = nodep->exprp()->dtypep();
            స్థిరరాశి పూర్ణాంకం అంకె_Insts = nodep->rangep()->elementsConst();
            స్థిరరాశి పూర్ణాంకం modwidth = అంకె_Insts;
            స్థిరరాశి పూర్ణాంకం pinwidth = pinDTypep->వెడల్పు();
            ఒకవేళ (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_కణుపు_DType* స్థిరరాశి subDTypep = nodep->కనుగొను_Logic_DType(1, 1, pinDTypep->numeric());
                user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{subDTypep, FINAL}.p());
                Ast_కణుపు* స్థిరరాశి newp
                    = కొత్త AstReplicate{nodep->fileline(), nodep->exprp()->unlink_Fr_వెనుక(),
                                       static_cast<uint32_t>(అంకె_Insts)};
                nodep->replaceWith(newp);
            } లేకపోతే {
                ఒకవేళ (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                user_Iterate_And_తదుపరి(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlink_Fr_వెనుక());
            }
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        ఒకవేళ (nodep->doing_వెడల్పు()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(నిజం);
            nodep->did_వెడల్పు(నిజం);
            తిరిగి;
        }
        ఒకవేళ (nodep->class_పద్ధతి() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } లేకపోతే ఒకవేళ (nodep->class_పద్ధతి() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        ఒకవేళ (nodep->class_పద్ధతి() && nodep->name() == "new") {
            ఒకవేళ (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            ఒకవేళ (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_కణుపు* firstp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ఒకవేళ (Ast_Stmt_సమాసం* స్థిరరాశి sep = VN_CAST(itemp, Stmt_సమాసం)) {
                    ఒకవేళ (AstNew* స్థిరరాశి newp = VN_CAST(sep->exprp(), New)) {
                        ఒకవేళ (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            ఆపు;
                        }
                    }
                    కొనసాగు;
                }
                ఒకవేళ (మొదటి_New_Statement_Ok_Recurse(itemp)) కొనసాగు;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doing_వెడల్పు(నిజం);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        ఒకవేళ (nodep->fvarp()) userIterate(nodep->fvarp(), శూన్యసూచిక);
        ఒకవేళ (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtype_సమితి_Void();
        } లేకపోతే ఒకవేళ (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } లేకపోతే ఒకవేళ (VN_IS(nodep, Property)) {
            nodep->dtype_సమితి_బిట్టు();
        }
        వెడల్పు_VP* vup = శూన్యసూచిక;
        ఒకవేళ (VN_IS(nodep, Property)) vup = వెడల్పు_VP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->did_వెడల్పు(నిజం);
        nodep->doing_వెడల్పు(అబద్ధం);
        ఒకవేళ (nodep->dpiImport() && !nodep->dpi_Open_మాతృ() && markHasOpenArray(nodep)) {
            nodep->dpi_Open_మాతృ_Inc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        ఒకవేళ (nodep->fvarp() && !nodep->dpiImport() && !nodep->శుద్ధ_Virtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_తదుపరి([&](స్థిరరాశి AstVarRef* varrefp) {
                   తిరిగి varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::సదిశ<AstVar*> ports;
        std::సదిశ<Ast_Attr_యొక్క*> protos;
        Ast_Attr_యొక్క* protop = శూన్యసూచిక;  // Base prototype
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_వెనుక(portp); }
            } లేకపోతే ఒకవేళ (Ast_Attr_యొక్క* స్థిరరాశి attrp = VN_CAST(stmtp, Attr_యొక్క)) {
                ఒకవేళ (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_వెనుక(attrp); }
                ఒకవేళ (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        ఒకవేళ (protop) {
            కొరకు (కొలత_t i = 0; i < std::max(ports.కొలత(), protos.కొలత()); ++i) {
                ఒకవేళ (i >= ports.కొలత() || i >= protos.కొలత()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    ఆపు;
                } లేకపోతే {
                    AstVar* స్థిరరాశి portp = ports[i];
                    Ast_Attr_యొక్క* స్థిరరాశి rprotop = protos[i];
                    Ast_కణుపు_DType* స్థిరరాశి declDtp = portp->dtypep();
                    Ast_కణుపు_DType* స్థిరరాశి protoDtp = rprotop->fromp()->dtypep();
                    ఒకవేళ (portp->name() != rprotop->name()) {
                        protoDtp->v3warn(PROTOTYPEMIS,
                                         "In prototype for "
                                             << nodep->prettyNameQ() << ", argument " << (i + 1)
                                             << " named " << rprotop->prettyNameQ()
                                             << " mismatches out-of-block argument name "
                                             << portp->prettyNameQ() << "  (IEEE 1800-2023 8.24)\n"
                                             << protoDtp->warnContextPrimary() << '\n'
                                             << declDtp->warnOther()
                                             << "... Location of out-of-block declaration\n"
                                             << declDtp->warnContextSecondary());
                    } లేకపోతే ఒకవేళ (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warn_మరిన్ని() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warn_మరిన్ని() << "... Declaration data type: "
                                << declDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warnContextPrimary() << '\n'
                                << declDtp->warnOther()
                                << "... Location of out-of-block declaration\n"
                                << declDtp->warnContextSecondary());
                    }
                }
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
        }
    }
    శూన్య సందర్శించు(AstConstraint* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, శూన్యసూచిక);
        nodep->did_వెడల్పు(నిజం);
    }
    శూన్య సందర్శించు(AstProperty* nodep) override {
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        ఒకవేళ (nodep->doing_వెడల్పు()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(నిజం);
            nodep->did_వెడల్పు(నిజం);
            తిరిగి;
        }
        nodep->doing_వెడల్పు(నిజం);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtype_సమితి_బిట్టు();
        కొరకు (Ast_కణుపు* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            ఒకవేళ (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, శూన్యసూచిక);
            } లేకపోతే ఒకవేళ (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } లేకపోతే ఒకవేళ (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, శూన్యసూచిక);
            } లేకపోతే {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->did_వెడల్పు(నిజం);
        nodep->doing_వెడల్పు(అబద్ధం);
    }
    శూన్య సందర్శించు(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    శూన్య సందర్శించు(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        nodep->dtype_సమితి_బిట్టు();
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (VN_IS(stmtp, Var)) {
                userIterate(stmtp, శూన్యసూచిక);
            } లేకపోతే ఒకవేళ (VN_IS(stmtp, కణుపు_సమాసం)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } లేకపోతే {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->did_వెడల్పు(నిజం);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* get_అంశం_Package(Ast_కణుపు* pkgItemp) {
        అయితే (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        తిరిగి VN_CAST(pkgItemp->backp(), Package);
    }
    శూన్య సందర్శించు(AstFuncRef* nodep) override {
        సందర్శించు(static_cast<Ast_కణుపు_FTask_Ref*>(nodep));
        ఒకవేళ (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            ఒకవేళ (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_సమాసం))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtype_సమితి_Void();
        } లేకపోతే {
            nodep->dtypeFrom(nodep->taskp());
        }
        ఒకవేళ (nodep->fileline()->timingOn()) {
            Ast_కణుపు_Module* స్థిరరాశి classp = nodep->classOrPackagep();
            ఒకవేళ (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* స్థిరరాశి packagep = get_అంశం_Package(classp);
                ఒకవేళ (packagep && packagep->name() == "std") {
                    పద్ధతి_Call_Warn_Timing(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    స్థిర తార్కిక are_Same_కొలత(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        స్థిరరాశి std::సదిశ<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        స్థిరరాశి std::సదిశ<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        ఒకవేళ (dims0.కొలత() != dims1.కొలత()) తిరిగి అబద్ధం;
        కొరకు (కొలత_t i = 0; i < dims0.కొలత(); ++i) {
            ఒకవేళ (dims0[i]->elementsConst() != dims1[i]->elementsConst()) తిరిగి అబద్ధం;
        }
        తిరిగి నిజం;
    }
    // Makes sure that port and pin have same size and same datatype
    శూన్య check_Unpacked_Array_వాదనలు(AstVar* portp, Ast_కణుపు* pinp) {
        ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి పోర్టు_Dtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            ఒకవేళ (AstUnpackArrayDType* స్థిరరాశి pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                ఒకవేళ (!are_Same_కొలత(పోర్టు_Dtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << పోర్టు_Dtypep->prettyDTypeNameQ() << ")");
                }
                స్థిరరాశి AstBasicDType* స్థిరరాశి పోర్టు_Basicp = పోర్టు_Dtypep->basicp();
                స్థిరరాశి AstBasicDType* స్థిరరాశి pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                స్థిరరాశి తార్కిక sameElemDType
                    = పోర్టు_Basicp && pinBasicp
                          ? పోర్టు_Basicp->వెడల్పు() == pinBasicp->వెడల్పు()
                                && పోర్టు_Basicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(పోర్టు_Dtypep->elemDTypep());
                ఒకవేళ (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, పోర్టు_Dtypep, "Argument", "Parameter"));
                }
            } లేకపోతే {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    శూన్య process_FTask_Ref_వాదనలు(Ast_కణుపు_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        userIterate(nodep->taskp(), శూన్యసూచిక);
        //
        // And do the arguments to the task/function too
        చేయి {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            స్థిరరాశి V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            ఒకవేళ (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
                స్థిరరాశి AstVar* స్థిరరాశి portp = tconnect.మొదటి;
                AstArg* స్థిరరాశి argp = tconnect.రెండవ;
                Ast_కణుపు_సమాసం* pinp = argp->exprp();
                ఒకవేళ (!pinp) కొనసాగు;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                ఒకవేళ (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_వెనుక_With_తదుపరి(&handle);  // Format + additional args, if any
                    Ast_కణుపు_సమాసం* argsp = శూన్యసూచిక;
                    అయితే (AstArg* స్థిరరాశి nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->కూడు_తదుపరి(nextargp->exprp()->unlink_Fr_వెనుక_With_తదుపరి());
                        nextargp->unlink_Fr_వెనుక()->delete_వృక్షం();  // Remove the call's Arg wrapper
                    }
                    string format;
                    ఒకవేళ (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->అంకె().గమ్యం_String();
                    } లేకపోతే {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(నెట్టు_Deletep(argp), argp);
                    AstSFormatF* స్థిరరాశి newp
                        = కొత్త AstSFormatF{nodep->fileline(), format, అబద్ధం, argsp};
                    ఒకవేళ (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(కొత్త AstScopeName{newp->fileline(), నిజం});
                    }
                    handle.relink(కొత్త AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    వెళ్ళు reloop;  // so exit early; next loop will correct it
                }  //
                లేకపోతే ఒకవేళ (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlink_Fr_వెనుక(&handle);  // No next, that's the next pin
                    Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                ఒకవేళ (portp->isWritable()) {
                    V3LinkLValue::link_LValue_సమితి(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, వెడల్పు_VP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } అయితే (అబద్ధం);
        // Stage 2
        {
            స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
                AstVar* స్థిరరాశి portp = tconnect.మొదటి;
                స్థిరరాశి AstArg* స్థిరరాశి argp = tconnect.రెండవ;
                Ast_కణుపు_సమాసం* స్థిరరాశి pinp = argp->exprp();
                ఒకవేళ (!pinp) కొనసాగు;  // Argument error we'll find later
                // Change data types based on above accept completion
                ఒకవేళ (nodep->taskp()->dpiImport()) check_Unpacked_Array_వాదనలు(portp, pinp);
                ఒకవేళ (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
                స్థిరరాశి AstVar* స్థిరరాశి portp = tconnect.మొదటి;
                స్థిరరాశి AstArg* స్థిరరాశి argp = tconnect.రెండవ;
                Ast_కణుపు_సమాసం* స్థిరరాశి pinp = argp->exprp();
                ఒకవేళ (!pinp) కొనసాగు;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, వెడల్పు_VP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        ఒకవేళ (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
                స్థిరరాశి AstVar* స్థిరరాశి portp = tconnect.మొదటి;
                స్థిరరాశి AstArg* స్థిరరాశి argp = tconnect.రెండవ;
                Ast_కణుపు_సమాసం* pinp = argp->exprp();
                ఒకవేళ (!pinp) కొనసాగు;  // Argument error we'll find later
                Ast_కణుపు_DType* స్థిరరాశి పోర్టు_DTypep = portp->dtypep()->skip_Ref_గమ్యం_Enump();
                ఒకవేళ (VN_IS(pinp, ఖాళీలు_వరుస)) {
                    Ast_కణుపు_సమాసం* newp = new_Cons_Dyn_Array_Or_వరుస(pinp->fileline(), పోర్టు_DTypep);
                    ఒకవేళ (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = కొత్త AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(పోర్టు_DTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(pinp), pinp);
                    pinp = newp;
                }
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి pinDTypep = pinp->dtypep()->skip_Ref_గమ్యం_Enump();
                స్థిరరాశి AstIfaceRefDType* స్థిరరాశి పోర్టు_Ifacep
                    = VN_CAST(పోర్టు_DTypep->elemDTypep(నిజం), IfaceRefDType);
                స్థిరరాశి VCastable ifaceCastable
                    = పోర్టు_Ifacep ? Ast_కణుపు::computeCastable(పోర్టు_DTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ నిజం)
                                 : VCastable{VCastable::UNSUPPORTED};
                స్థిరరాశి తార్కిక matchingRefDTypes = పోర్టు_Ifacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(పోర్టు_DTypep, pinDTypep);
                స్థిరరాశి తార్కిక pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->అంకె().isNull();
                ఒకవేళ ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    ఒకవేళ (పోర్టు_Ifacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } లేకపోతే {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << పోర్టు_DTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } లేకపోతే ఒకవేళ (పోర్టు_Ifacep && పోర్టు_Ifacep->isVirtual() && portp->is_ఆదానం() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } లేకపోతే ఒకవేళ (portp->isWritable() && pinp->వెడల్పు() != portp->వెడల్పు()) {
                    pinp->v3widthWarn(portp->వెడల్పు(), pinp->వెడల్పు(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->వెడల్పు()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->వెడల్పు() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlink_Fr_వెనుక(&relinkHandle);
                    Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                ఒకవేళ (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(పోర్టు_DTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && పోర్టు_DTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << పోర్టు_DTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    కొనసాగు;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                ఒకవేళ (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } లేకపోతే ఒకవేళ (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    check_Class_కేటాయించు(nodep, "Function Argument", pinp, పోర్టు_DTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, వెడల్పు_VP{పోర్టు_DTypep, FINAL, ప్రవాహం_USE_కేటాయించు}.p());
                } లేకపోతే {
                    iterate_Check_కేటాయించు(nodep, "Function Argument", pinp, FINAL, పోర్టు_DTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    శూన్య handle_Std_Randomize_వాదనలు(Ast_కణుపు_FTask_Ref* స్థిరరాశి nodep) {
        AstConst* nullp = శూన్యసూచిక;
        కొరకు (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_కణుపు_సమాసం* స్థిరరాశి exprp = argp->exprp();
            ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(exprp, Const)) {
                ఒకవేళ (constp->అంకె().isNull()) {
                    nullp = constp;
                    కొనసాగు;
                }
            }
            ఒకవేళ (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                కొనసాగు;
            }
            ఒకవేళ (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                కొనసాగు;
            }
            ఒకవేళ (స్థిరరాశి AstCMethodHard* స్థిరరాశి methodp = VN_CAST(exprp, CMethodHard)) {
                ఒకవేళ (methodp->పద్ధతి() == VCMethod::ARRAY_AT
                    || methodp->పద్ధతి() == VCMethod::ARRAY_AT_WRITE) {
                    కొనసాగు;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        ఒకవేళ (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = శూన్యసూచిక;
        ఒకవేళ (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(నిజం);
            nodep->dtypep(nodep->కనుగొను_Basic_DType(VBasicDTypeKwd::INT));
            తిరిగి;  // Handled in V3Randomize
        } లేకపోతే ఒకవేళ (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* స్థిరరాశి classp = VN_CAST(nodep->classOrPackagep(), Class);
            ఒకవేళ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(నిజం);
                Ast_కణుపు_DType* స్థిరరాశి adtypep = nodep->కనుగొను_బిట్టు_DType();
                withp = పద్ధతి_With_Clause(nodep, అబద్ధం, అబద్ధం, adtypep->కనుగొను_Void_DType(),
                                         adtypep->కనుగొను_బిట్టు_DType(), adtypep);
                కొరకు (స్థిరరాశి AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_తదుపరి(argp->exprp(), వెడల్పు_VP{SELF, BOTH}.p());
                }
                handle_Std_Randomize_వాదనలు(nodep);  // Provided args should be in current scope
                process_FTask_Ref_వాదనలు(nodep);
                ఒకవేళ (withp) nodep->withp(withp);
                nodep->did_వెడల్పు(నిజం);
                తిరిగి;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            ఒకవేళ (nodep->name() == "randomize") {
                AstClassRefDType* స్థిరరాశి adtypep
                    = కొత్త AstClassRefDType{nodep->fileline(), classp, శూన్యసూచిక};
                v3Global.rootp()->typeTablep()->కూడు_Typesp(adtypep);
                withp = పద్ధతి_With_Clause(nodep, అబద్ధం, అబద్ధం, adtypep->కనుగొను_Void_DType(),
                                         adtypep->కనుగొను_బిట్టు_DType(), adtypep);
                కొరకు (స్థిరరాశి AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_తదుపరి(argp->exprp(), వెడల్పు_VP{SELF, BOTH}.p());
                }
                handle_Randomize_వాదనలు(nodep, classp);
            } లేకపోతే ఒకవేళ (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } లేకపోతే ఒకవేళ (nodep->name() == "get_randstate") {
                పద్ధతి_Ok_Arguments(nodep, 0, 0);
                classp->పునాది_Most_Classp()->needRNG(నిజం);
                v3Global.useRandomizeMethods(నిజం);
                AstCExpr* స్థిరరాశి newp
                    = కొత్త AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtype_సమితి_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            } లేకపోతే ఒకవేళ (nodep->name() == "set_randstate") {
                పద్ధతి_Ok_Arguments(nodep, 1, 1);
                Ast_కణుపు_సమాసం* స్థిరరాశి expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_కణుపు_సమాసం* స్థిరరాశి exprp = nodep->argsp()->exprp();
                classp->పునాది_Most_Classp()->needRNG(నిజం);
                v3Global.useRandomizeMethods(నిజం);
                AstCExpr* స్థిరరాశి newp
                    = కొత్త AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->కూడు(exprp->unlink_Fr_వెనుక());
                newp->కూడు(")");
                newp->dtype_సమితి_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            } లేకపోతే {
                UASSERT_OBJ(అబద్ధం, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ఒకవేళ (nodep->did_వెడల్పు()) {
            ఒకవేళ (withp) nodep->withp(withp);
            తిరిగి;
        }
        ఒకవేళ ((nodep->taskp()->class_పద్ధతి() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            తార్కిక allow = అబద్ధం;
            ఒకవేళ (m_ftaskp && m_ftaskp->class_పద్ధతి() && !m_ftaskp->isStatic()) {
                ఒకవేళ (స్థిరరాశి AstFuncRef* స్థిరరాశి funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstTaskRef* స్థిరరాశి taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                ఒకవేళ (!allow) {
                    స్థిరరాశి AstClass* callerClassp = m_containingClassFinder.కనుగొను(m_ftaskp);
                    ఒకవేళ (!callerClassp) {
                        callerClassp = m_containingClassFinder.కనుగొను(m_ftaskp->classOrPackagep());
                    }
                    స్థిరరాశి AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ఒకవేళ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.కనుగొను(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            ఒకవేళ (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                స్థిరరాశి AstClass* స్థిరరాశి cgClassp = m_containingClassFinder.కనుగొను(nodep);
                ఒకవేళ (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    స్థిరరాశి AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ఒకవేళ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.కనుగొను(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            ఒకవేళ (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        ఒకవేళ (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(కొత్త AstScopeName{nodep->fileline(), అబద్ధం});
        }
        // And do the arguments to the task/function too
        process_FTask_Ref_వాదనలు(nodep);
        ఒకవేళ (withp) nodep->withp(withp);
        nodep->did_వెడల్పు(నిజం);
        // See steps that follow in visit(AstFuncRef*)
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, శూన్యసూచిక);
    }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        ఒకవేళ (nodep->is_Combo_నక్షత్రం()) తిరిగి;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        ఒకవేళ (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            స్వయంచాలక* స్థిరరాశి concatOrReplp = VN_CAST(nodep->sensp(), కణుపు_Biop);
            స్వయంచాలక* స్థిరరాశి rhsp = concatOrReplp->rhsp()->unlink_Fr_వెనుక();
            ఒకవేళ (nodep->అంచు_Type() == VEdgeType::ET_మారింది) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                స్వయంచాలక* స్థిరరాశి lhsp = concatOrReplp->lhsp()->unlink_Fr_వెనుక();
                nodep->కూడు_తదుపరి_ఇక్కడ(కొత్త Ast_Sen_అంశం{lhsp->fileline(), nodep->అంచు_Type(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(కొత్త Ast_Sen_అంశం{rhsp->fileline(), nodep->అంచు_Type(), rhsp});
            VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
        } లేకపోతే {
            userIterateChildren(nodep, వెడల్పు_VP{SELF, BOTH}.p());
            ఒకవేళ (nodep->అంచు_Type().an_అంచు()) {
                Ast_కణుపు_DType* స్థిరరాశి sensDtp = nodep->sensp()->dtypep()->skipRefp();
                ఒకవేళ (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } లేకపోతే ఒకవేళ (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    శూన్య సందర్శించు(Ast_Clocking_అంశం* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* స్థిరరాశి refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, వెడల్పు_VP{SELF, PRELIM}.p());
    }
    శూన్య సందర్శించు(AstWait* nodep) override {
        ఒకవేళ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_వెనుక()->delete_వృక్షం(), nodep);
            తిరిగి;
        }
        ఒకవేళ (nodep->fileline()->timingOn()) {
            ఒకవేళ (v3Global.opt.timing().is_సమితి_True()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి varrefp = VN_CAST(nodep->condp(), కణుపు_Var_Ref)) {
                    ఒకవేళ (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warn_మరిన్ని() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                తిరిగి;
            } లేకపోతే ఒకవేళ (v3Global.opt.timing().is_సమితి_False()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } లేకపోతే {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_వెనుక_With_తదుపరి();
            nodep->replaceWith(stmtsp);
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }
    శూన్య సందర్శించు(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_కణుపు_DType* స్థిరరాశి vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->సూచీ_Arg_Refp(), శూన్యసూచిక);
        userIterateChildren(nodep->విలువ_Arg_Refp(), శూన్యసూచిక);
        ఒకవేళ (!nodep->exprp()) {
            nodep->dtype_సమితి_Void();
        } లేకపోతే {
            ఒకవేళ (!nodep->exprp()->hasDType()) {
                user_Iterate_And_తదుపరి(nodep->exprp(), శూన్యసూచిక);
            } లేకపోతే {
                ఒకవేళ (vdtypep) {
                    user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{nodep->dtypep(), PRELIM}.p());
                } లేకపోతే {  // 'sort with' allows arbitrary type
                    user_Iterate_And_తదుపరి(nodep->exprp(), వెడల్పు_VP{SELF, PRELIM}.p());
                }
            }

            ఒకవేళ (!nodep->exprp()->hasDType()) {
                nodep->dtype_సమితి_Void();
            } లేకపోతే {
                nodep->dtypeFrom(nodep->exprp());
                iterate_Check_కేటాయించు(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    శూన్య సందర్శించు(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        ఒకవేళ (nodep->సూచీ()) {
            nodep->dtypeFrom(m_withp->సూచీ_Arg_Refp());
        } లేకపోతే {
            nodep->dtypeFrom(m_withp->విలువ_Arg_Refp());
        }
    }
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, శూన్యసూచిక);
    }

    //--------------------
    // Default
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        ఒకవేళ (!nodep->did_వెడల్పు()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, శూన్యసూచిక);
    }
    శూన్య సందర్శించు(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << పంక్తి_ముగింపు);
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        ఒకవేళ (Ast_కణుపు_DType* స్థిరరాశి vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } లేకపోతే ఒకవేళ (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->వెడల్పు() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->వెడల్పు() << " bits -> " << nodep->వెడల్పు()
                                    << " bits" << పంక్తి_ముగింపు);

        // Final processing
        user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, FINAL}.p());
        nodep->did_వెడల్పు(నిజం);
    }
    శూన్య సందర్శించు_Class(AstClass* nodep) {
        ఒకవేళ (nodep->did_వెడల్పు_And_సమితి()) తిరిగి;

        // If the class is std::process
        ఒకవేళ (nodep->name() == "process") {
            AstPackage* స్థిరరాశి packagep = get_అంశం_Package(nodep);
            ఒకవేళ (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                ఒకవేళ (AstVar* స్థిరరాశి varp
                    = VN_CAST(m_memberMap.కనుగొను_Member(nodep, "m_process"), Var)) {
                    Ast_కణుపు_DType* స్థిరరాశి dtypep = varp->get_బిడ్డ_DTypep();
                    ఒకవేళ (!varp->dtypep()) {
                        VL_DO_DANGLING(నెట్టు_Deletep(dtypep->unlink_Fr_వెనుక()), dtypep);
                    }
                    AstBasicDType* స్థిరరాశి newdtypep = కొత్త AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->కూడు_Typesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                ఒకవేళ (Ast_కణుపు_FTask* స్థిరరాశి ftaskp
                    = VN_CAST(m_memberMap.కనుగొను_Member(nodep, "self"), కణుపు_FTask)) {
                    ftaskp->సమితి_Need_Process();
                    v3Global.సమితి_ఉపయోగిస్తుంది_Timing();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_తదుపరి(nodep->extendsp(), శూన్యసూచిక);
        userIterateChildren(nodep, శూన్యసూచిక);  // First size all members
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_తాత్కాలిక_పేర్లు);
        VL_RESTORER_COPY(m_inside_తాత్కాలిక_పేర్లు);
        ఒకవేళ (AstClass* స్థిరరాశి classp = VN_CAST(nodep, Class)) {
            సందర్శించు_Class(classp);
        } లేకపోతే {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([ఇది](స్థిరరాశి AstVar* varp) { m_curModVars.చొప్పించు(varp); });
            userIterateChildren(nodep, శూన్యసూచిక);
        }
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, శూన్యసూచిక);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    శూన్య సందర్శించు_Or_Lu64(Ast_కణుపు_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_సమితి_Double();
            Ast_కణుపు_DType* స్థిరరాశి subDTypep = nodep->కనుగొను_బిట్టు_DType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    శూన్య సందర్శించు(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_సమితి_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, PRELIM}.p());
            ఒకవేళ (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    కొత్త AstISToRD{nodep->fileline(), nodep->lhsp()->unlink_Fr_వెనుక()});
                VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
            }
        }
    }
    శూన్య సందర్శించు(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_సమితి_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, PRELIM}.p());
        }
    }
    శూన్య సందర్శించు_Os32_Lr(Ast_కణుపు_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_సమితి_Integer2_State();
        }
    }
    శూన్య సందర్శించు_Ou64_Lr(Ast_కణుపు_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_సమితి_UInt64();
        }
    }

    శూన్య సందర్శించు_log_not(AstLogNot* nodep) {
        // CALLER: LogNot
        // Width-check: lhs 1 bit
        // Real: Allowed; implicitly compares with zero
        // We calculate the width of the UNDER expression.
        // We then check its width to see if it's legal, and edit if not
        // We finally set the width of our output
        // IEEE-2012: Table 11-21 and 11.8.1 (same as RedAnd):
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtype_సమితి_బిట్టు_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            ఒకవేళ (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* స్థిరరాశి newp
                    = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            }
        }
    }
    శూన్య సందర్శించు_log_and_or(Ast_కణుపు_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_బిట్టు_Or_Logic(
                Ast_కణుపు_సమాసం::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    శూన్య సందర్శించు_Abort_Prop(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు_red_and_or(Ast_కణుపు_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtype_సమితి_బిట్టు_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    శూన్య సందర్శించు_red_unknown(Ast_కణుపు_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            nodep->dtype_సమితి_బిట్టు();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    తార్కిక isEquivalentDType(స్థిరరాశి Ast_కణుపు_DType* lhs, స్థిరరాశి Ast_కణుపు_DType* rhs) {
        // a) If two types match, they are equivalent.
        ఒకవేళ (!lhs || !rhs) తిరిగి అబద్ధం;
        lhs = lhs->skip_Ref_గమ్యం_Enump();
        rhs = rhs->skip_Ref_గమ్యం_Enump();
        ఒకవేళ (lhs == rhs) తిరిగి నిజం;
        // If both are basic types, check if they are the same type
        ఒకవేళ (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            స్థిరరాశి స్వయంచాలక* lb = VN_CAST(lhs, BasicDType);
            స్థిరరాశి స్వయంచాలక* rb = VN_CAST(rhs, BasicDType);
            ఒకవేళ (lb->isString() != rb->isString()) తిరిగి అబద్ధం;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        స్థిరరాశి తార్కిక lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        స్థిరరాశి తార్కిక rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        ఒకవేళ (lhsIsUnpackArray || rhsIsUnpackArray) {
            ఒకవేళ (lhsIsUnpackArray && rhsIsUnpackArray) {
                స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి lhsp = VN_CAST(lhs, UnpackArrayDType);
                స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి rhsp = VN_CAST(rhs, UnpackArrayDType);
                స్థిరరాశి పూర్ణాంకం lsz = lhsp->elementsConst();
                స్థిరరాశి పూర్ణాంకం rsz = rhsp->elementsConst();
                ఒకవేళ (lsz >= 0 && rsz >= 0 && lsz != rsz) తిరిగి అబద్ధం;
                తిరిగి isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            తిరిగి అబద్ధం;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        స్థిరరాశి తార్కిక lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        స్థిరరాశి తార్కిక rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        స్థిరరాశి తార్కిక lhs_Is_వరుస = VN_IS(lhs, వరుస_DType);
        స్థిరరాశి తార్కిక rhs_Is_వరుస = VN_IS(rhs, వరుస_DType);
        స్థిరరాశి తార్కిక lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        స్థిరరాశి తార్కిక rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        ఒకవేళ (lhsIsDynArray || rhsIsDynArray || lhs_Is_వరుస || rhs_Is_వరుస || lhsIsAssocArray
            || rhsIsAssocArray) {
            ఒకవేళ (స్థిరరాశి AstDynArrayDType* స్థిరరాశి lhsp = VN_CAST(lhs, DynArrayDType)) {
                ఒకవేళ (స్థిరరాశి AstDynArrayDType* స్థిరరాశి rhsp = VN_CAST(rhs, DynArrayDType)) {
                    తిరిగి isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ఒకవేళ (స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి lhsp = VN_CAST(lhs, వరుస_DType)) {
                ఒకవేళ (స్థిరరాశి Ast_వరుస_DType* స్థిరరాశి rhsp = VN_CAST(rhs, వరుస_DType)) {
                    తిరిగి isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ఒకవేళ (స్థిరరాశి AstAssocArrayDType* స్థిరరాశి lhsp = VN_CAST(lhs, AssocArrayDType)) {
                ఒకవేళ (స్థిరరాశి AstAssocArrayDType* స్థిరరాశి rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    తిరిగి isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->కీలకం_DTypep(), rhsp->కీలకం_DTypep());
                }
            }

            తిరిగి అబద్ధం;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        ఒకవేళ (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            ఒకవేళ (lhs->వెడల్పు() != rhs->వెడల్పు()) తిరిగి అబద్ధం;
            ఒకవేళ (lhs->isFourstate() != rhs->isFourstate()) తిరిగి అబద్ధం;
            ఒకవేళ (lhs->isSigned() != rhs->isSigned()) తిరిగి అబద్ధం;
            తిరిగి నిజం;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        ఒకవేళ (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) తిరిగి అబద్ధం;
        తిరిగి !(VN_IS(lhs, కణుపు_UOr_Struct_DType) || VN_IS(rhs, కణుపు_UOr_Struct_DType));
    }

    స్థిర తార్కిక isAggregateType(స్థిరరాశి Ast_కణుపు* nodep) {
        ఒకవేళ (!nodep) తిరిగి అబద్ధం;
        స్థిరరాశి Ast_కణుపు_DType* dtypep = nodep->dtypep();
        ఒకవేళ (!dtypep) తిరిగి అబద్ధం;
        dtypep = dtypep->skipRefp();
        ఒకవేళ (!dtypep) తిరిగి అబద్ధం;
        తిరిగి dtypep->isAggregateType();
    }

    శూన్య సందర్శించు_cmp_eq_gt(Ast_కణుపు_Biop* nodep, స్థిరరాశి ComparisonType cmpType) {
        // CALLER: AstEq, AstGt, ..., AstLtS
        // Real allowed if and only if real_lhs set
        // See IEEE-2012 11.4.4, and 11.8.1:
        //   Widths: 1 bit out, width is max of LHS or RHS
        //   Sign:  signed compare (not output) if both signed, compare is signed,
        //             width mismatches sign extend
        //             else, compare is unsigned, **zero-extends**
        //   Real:  If either real, other side becomes real and real compare
        //   TODO: chandle/class handle/iface handle: WildEq/WildNeq same as Eq/Neq
        //   TODO: chandle/class handle/iface handle only allowed to self-compare or against null
        //   TODO: chandle/class handle/iface handle no relational compares
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            స్థిరరాశి తార్కిక lhsIsPat = VN_IS(nodep->lhsp(), నమూనా);
            స్థిరరాశి తార్కిక rhsIsPat = VN_IS(nodep->rhsp(), నమూనా);
            ఒకవేళ (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } లేకపోతే ఒకవేళ (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } లేకపోతే {
                user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            }

            స్థిరరాశి తార్కిక isAggrLhs = isAggregateType(nodep->lhsp());
            స్థిరరాశి తార్కిక isAggrRhs = isAggregateType(nodep->rhsp());

            ఒకవేళ ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::సాధారణ_EQ || cmpType == ComparisonType::CASE)) {
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి lhsDType = nodep->lhsp()->dtypep();
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి rhsDType = nodep->rhsp()->dtypep();

                ఒకవేళ (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_కణుపు* స్థిరరాశి newp
                        = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convert_Unpacked_క్రమం(nodep->rhsp(), nodep->lhsp()->dtypep());
            } లేకపోతే ఒకవేళ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                ఒకవేళ (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_కణుపు* స్థిరరాశి newp
                        = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                    తిరిగి;
                }
                ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } లేకపోతే ఒకవేళ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } లేకపోతే {
                స్థిరరాశి తార్కిక signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                స్థిరరాశి పూర్ణాంకం వెడల్పు = std::max(nodep->lhsp()->వెడల్పు(), nodep->rhsp()->వెడల్పు());
                స్థిరరాశి పూర్ణాంకం ewidth = std::max(nodep->lhsp()->వెడల్పు_Min(), nodep->rhsp()->వెడల్పు_Min());
                తార్కిక warnOn = నిజం;
                ఒకవేళ (!signedFl && వెడల్పు == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    ఒకవేళ ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->వెడల్పు() >= nodep->rhsp()->వెడల్పు_Min())) {
                        warnOn = అబద్ధం;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    ఒకవేళ ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->వెడల్పు_Min() >= nodep->rhsp()->వెడల్పు())) {
                        warnOn = అబద్ధం;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->కనుగొను_బిట్టు_Or_Logic_DType(వెడల్పు, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->కనుగొను_బిట్టు_Or_Logic_DType(వెడల్పు, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtype_సమితి_బిట్టు_Or_Logic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::సాధారణ_EQ
                                               || cmpType == ComparisonType::సాధారణ_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    శూన్య సందర్శించు_cmp_real(Ast_కణుపు_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు_cmp_string(Ast_కణుపు_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_బిట్టు();
        }
    }
    శూన్య సందర్శించు_cmp_type(Ast_కణుపు_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{SELF, BOTH}.p());
            స్థిరరాశి Ast_Attr_యొక్క* స్థిరరాశి lhsap = VN_AS(nodep->lhsp(), Attr_యొక్క);
            స్థిరరాశి Ast_Attr_యొక్క* స్థిరరాశి rhsap = VN_AS(nodep->rhsp(), Attr_యొక్క);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి lhsDtp = lhsap->dtypep();
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            స్థిరరాశి తార్కిక invert = VN_IS(nodep, NeqT);
            స్థిరరాశి తార్కిక identical
                = Ast_కణుపు::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            స్థిరరాశి తార్కిక eq = invert ^ identical;
            Ast_కణుపు* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_True{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        }
    }

    శూన్య సందర్శించు_negate_not(Ast_కణుపు_Uniop* nodep, తార్కిక real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            ఒకవేళ (!real_ok) checkCvtUS(nodep->lhsp(), అబద్ధం);
        }
        ఒకవేళ (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            ఒకవేళ (Ast_కణుపు_Uniop* స్థిరరాశి newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtype_సమితి_Double();
                తిరిగి;
            }
        } లేకపోతే {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        ఒకవేళ (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        ఒకవేళ (m_vup->final()) {
            Ast_కణుపు_DType* స్థిరరాశి expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_కణుపు_DType* స్థిరరాశి subDTypep = expDTypep;
            // Some warning suppressions
            తార్కిక lhsWarn = నిజం;
            ఒకవేళ (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                ఒకవేళ (subDTypep->వెడల్పు_Min() == (nodep->lhsp()->వెడల్పు_Min() + 1)) lhsWarn = అబద్ధం;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    శూన్య సందర్శించు_signed_unsigned(Ast_కణుపు_Uniop* nodep, VSigning rs_బహిర్గతం) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), నిజం);
            స్థిరరాశి పూర్ణాంకం వెడల్పు = nodep->lhsp()->వెడల్పు();
            Ast_కణుపు_DType* స్థిరరాశి expDTypep = nodep->కనుగొను_బిట్టు_Or_Logic_DType(
                వెడల్పు, వెడల్పు, rs_బహిర్గతం, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_కణుపు_DType* స్థిరరాశి subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    శూన్య సందర్శించు_జరుపు_2(Ast_కణుపు_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_జరుపు_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        స్థిరరాశి Ast_కణుపు_Biop* స్థిరరాశి newp = iterate_జరుపు_final(nodep);
        VL_DANGLING(nodep);
        (శూన్య)newp;  // Ununused
    }
    శూన్య iterate_జరుపు_prelim(Ast_కణుపు_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), అబద్ధం);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ఒకవేళ (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtype_సమితి_Logic_Unsized(nodep->వెడల్పు(), nodep->వెడల్పు_Min(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_కణుపు_Biop* iterate_జరుపు_final(Ast_కణుపు_Biop* nodep) {
        // Nodep maybe edited
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->final()) {
            Ast_కణుపు_DType* స్థిరరాశి expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_కణుపు_DType* స్థిరరాశి subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            ఒకవేళ (VN_IS(nodep, జరుపు_RS)) {
                ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            తార్కిక warnOn = నిజం;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            ఒకవేళ (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), కణుపు_కేటాయించు)) warnOn = అబద్ధం;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* స్థిరరాశి shiftp = VN_CAST(nodep->rhsp(), Const);
            ఒకవేళ (shiftp && !shiftp->అంకె().isFourState() && shiftp->వెడల్పు() <= 32) {
                స్థిరరాశి int64_t జరుపు_Val = shiftp->అంకె().గమ్యం_SQuad();
                ఒకవేళ (VN_IS(nodep, జరుపు_L)) {
                    ఒకవేళ (జరుపు_Val > 0 && nodep->వెడల్పు() == nodep->lhsp()->వెడల్పు() + జరుపు_Val)
                        warnOn = అబద్ధం;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            ఒకవేళ (nodep->rhsp()->వెడల్పు() > 32) {
                ఒకవేళ (shiftp && shiftp->అంకె().most_సమితి_బిట్టు_P1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_సంఖ్య అంకె{shiftp, 32, shiftp->అంకె()};
                    Ast_కణుపు* స్థిరరాశి shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(కొత్త AstConst{shiftrhsp->fileline(), అంకె});
                    VL_DO_DANGLING(shiftrhsp->delete_వృక్షం(), shiftrhsp);
                }
            }
        }
        తిరిగి nodep;  // May edit
    }

    శూన్య సందర్శించు_boolexpr_and_or(Ast_కణుపు_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), అబద్ధం);
            checkCvtUS(nodep->rhsp(), అబద్ధం);
            స్థిరరాశి పూర్ణాంకం వెడల్పు = std::max(nodep->lhsp()->వెడల్పు(), nodep->rhsp()->వెడల్పు());
            స్థిరరాశి పూర్ణాంకం mwidth = std::max(nodep->lhsp()->వెడల్పు_Min(), nodep->rhsp()->వెడల్పు_Min());
            స్థిరరాశి తార్కిక expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtype_సమితి_బిట్టు_Or_Logic_Unsized(
                వెడల్పు, mwidth, VSigning::fromBool(expSigned),
                Ast_కణుపు_సమాసం::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        ఒకవేళ (m_vup->final()) {
            Ast_కణుపు_DType* స్థిరరాశి expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_కణుపు_DType* స్థిరరాశి subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    శూన్య సందర్శించు_కూడు_sub_replace(Ast_కణుపు_Biop* nodep, తార్కిక real_ok, స్థిరరాశి తార్కిక modOrDiv) {
        // CALLER: (real_ok=false) AddS, SubS, ...
        // CALLER: (real_ok=true) Add, Sub, ...
        // Widths: out width = lhs width = rhs width
        // Signed: Replace operator with signed operator, or signed to unsigned
        // Real: Replace operator with real operator
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        //
        // UINFOTREE(9, nodep, "rus " << m_vup, "rusin");
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_తదుపరి(nodep->lhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_తదుపరి(nodep->rhsp(), వెడల్పు_VP{CONTEXT_DET, PRELIM}.p());
            ఒకవేళ (!real_ok) {
                checkCvtUS(nodep->lhsp(), అబద్ధం);
                checkCvtUS(nodep->rhsp(), అబద్ధం);
            }
            ఒకవేళ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtype_సమితి_Double();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                తిరిగి;
            } లేకపోతే ఒకవేళ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, కూడు_2)
                            ? (nodep->warn_మరిన్ని()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి;
            } లేకపోతే {
                స్థిరరాశి పూర్ణాంకం వెడల్పు = std::max(nodep->lhsp()->వెడల్పు(), nodep->rhsp()->వెడల్పు());
                స్థిరరాశి పూర్ణాంకం mwidth = std::max(nodep->lhsp()->వెడల్పు_Min(), nodep->rhsp()->వెడల్పు_Min());
                స్థిరరాశి తార్కిక expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtype_సమితి_బిట్టు_Or_Logic_Unsized(
                    వెడల్పు, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_కణుపు_సమాసం::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ఒకవేళ (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_కణుపు_DType* స్థిరరాశి expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_కణుపు_DType* స్థిరరాశి subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            ఒకవేళ (Ast_కణుపు_Biop* స్థిరరాశి newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            తార్కిక lhsWarn = నిజం;
            తార్కిక rhsWarn = నిజం;
            ఒకవేళ (VN_IS(nodep, కూడు_2) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                ఒకవేళ (subDTypep->వెడల్పు_Min() == (nodep->lhsp()->వెడల్పు_Min() + 1)) lhsWarn = అబద్ధం;
                ఒకవేళ (subDTypep->వెడల్పు_Min() == (nodep->rhsp()->వెడల్పు_Min() + 1)) rhsWarn = అబద్ధం;
                ఒకవేళ (VN_IS(nodep, కూడు_2) && nodep->lhsp()->వెడల్పు() == 1
                    && nodep->rhsp()->వెడల్పు() != 1)
                    lhsWarn = అబద్ధం;  // do_increment + ...
                ఒకవేళ (nodep->rhsp()->వెడల్పు() == 1 && nodep->lhsp()->వెడల్పు() != 1)
                    rhsWarn = అబద్ధం;  // ... + do_increment
            } లేకపోతే ఒకవేళ (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                ఒకవేళ (subDTypep->వెడల్పు_Min() >= (nodep->lhsp()->వెడల్పు_Min())) lhsWarn = అబద్ధం;
                ఒకవేళ (subDTypep->వెడల్పు_Min() >= (nodep->rhsp()->వెడల్పు_Min())) rhsWarn = అబద్ధం;
            }
            // Final call, so make sure children check their sizes
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         rhsWarn);
        }
        // UINFOTREE(9, nodep, "", "rusou");
    }
    శూన్య సందర్శించు_real_కూడు_sub(Ast_కణుపు_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_సమితి_Double();
        }
    }
    శూన్య సందర్శించు_real_neg_ceil(Ast_కణుపు_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_సమాసం(nodep);
        ఒకవేళ (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_సమితి_Double();
        }
    }

    శూన్య సందర్శించు_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->సమాసం_Format_2(), nodep, "Assumes constant format");
        తార్కిక లోపల_Pct = అబద్ధం;
        Ast_కణుపు_సమాసం* argp = nodep->exprsp();
        string newFormat;
        కొరకు (అక్షరం ch : nodep->పాఠ్యం()) {
            ఒకవేళ (!లోపల_Pct && ch == '%') {
                లోపల_Pct = నిజం;
                newFormat += ch;
            } లేకపోతే ఒకవేళ (లోపల_Pct && (std::అంకెనా(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } లేకపోతే ఒకవేళ (!లోపల_Pct) {  // Normal text
                newFormat += ch;
            } లేకపోతే {
                లోపల_Pct = అబద్ధం;
                Ast_కణుపు_సమాసం* స్థిరరాశి nextp = argp ? VN_AS(argp->nextp(), కణుపు_సమాసం) : శూన్యసూచిక;
                AstSFormatArg* స్థిరరాశి fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_కణుపు_సమాసం* subargp = fargp ? fargp->exprp() : argp;
                స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : శూన్యసూచిక;
                ch = std::tolower(ch);
                ఎంపిక (ch) {
                స్థితి '?':  // Unspecified by user, guess
                    ఒకవేళ (dtypep->isDouble()) {
                        ch = 'g';
                    } లేకపోతే ఒకవేళ (dtypep->isString()) {
                        ch = 's';
                    } లేకపోతే ఒకవేళ (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } లేకపోతే {
                        ch = 'p';
                    }
                    argp = nextp;
                    ఆపు;
                స్థితి '%': ఆపు;  // %% - just output a %
                స్థితి 'm': ఆపు;  // %m - auto insert "name"
                స్థితి 'l': ఆపు;  // %m - auto insert "library"
                స్థితి 'd':
                    ఒకవేళ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ఒకవేళ (subargp->isDouble()) spliceCvtS(subargp, నిజం, 64);
                        argp = nextp;
                    }
                    ఆపు;
                స్థితి 'b':  // FALLTHRU
                స్థితి 'o':  // FALLTHRU
                స్థితి 'x':
                    ఒకవేళ (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    ఆపు;
                స్థితి 't':  // Decimal/float time
                    ఒకవేళ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    ఆపు;
                స్థితి 'e':  // FALLTHRU
                స్థితి 'f':  // FALLTHRU
                స్థితి 'g':
                    ఒకవేళ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ఒకవేళ (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    ఆపు;
                స్థితి 'p':  // FALLTHRU
                స్థితి 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    ఒకవేళ (subargp && !fargp) {
                        ఒకవేళ (AstEnumDType* స్థిరరాశి enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlink_Fr_వెనుక(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    ఆపు;
                అప్రమేయ:  // Most operators, just move to next argument
                    argp = nextp;
                    ఆపు;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->పాఠ్యం(newFormat);
    }

    స్థిర AstEnumDType* formatEnumDType(Ast_కణుపు_సమాసం* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType);
        ఒకవేళ (!enumDtp) {
            ఒకవేళ (స్థిరరాశి AstVarRef* స్థిరరాశి varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType);
            }
        }
        తిరిగి enumDtp;
    }

    Ast_సమాసం_Stmt* newEnumCapture(Ast_కణుపు_సమాసం* valuep, AstEnumDType* dtypep) {
        File_పంక్తి* స్థిరరాశి flp = valuep->fileline();
        AstVar* స్థిరరాశి varp = కొత్త AstVar{flp, VVarType::XTEMP, m_enum_తాత్కాలిక_పేర్లు.get(valuep), dtypep};
        varp->noSubst(నిజం);
        ఒకవేళ (m_ftaskp) {
            varp->funcLocal(నిజం);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } లేకపోతే {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->కూడు_Stmtsp(varp);
        }
        iterate(varp);
        Ast_సమాసం_Stmt* స్థిరరాశి exprp = కొత్త Ast_సమాసం_Stmt{
            flp, కొత్త Ast_కేటాయించు{flp, కొత్త AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            కొత్త AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        తిరిగి exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_కణుపు_సమాసం* valuep, AstEnumDType* dtypep) {
        ఒకవేళ (valuep->is_శుద్ధ()) valuep = V3Const::constifyEdit(valuep);
        Ast_కణుపు_సమాసం* keyp;
        ఒకవేళ (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_వృక్షం(అబద్ధం);
        } లేకపోతే {
            Ast_సమాసం_Stmt* స్థిరరాశి capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        }
        స్థిరరాశి VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* స్థిరరాశి argp = కొత్త AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        తిరిగి argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    తార్కిక వెడల్పు_Bad(Ast_కణుపు* nodep, Ast_కణుపు_DType* expDTypep) {
        స్థిరరాశి పూర్ణాంకం exp_వెడల్పు = expDTypep->వెడల్పు();
        పూర్ణాంకం exp_వెడల్పు_Min = expDTypep->వెడల్పు_Min();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        ఒకవేళ (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) తిరిగి అబద్ధం;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->వెడల్పు() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(exp_వెడల్పు != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        ఒకవేళ (exp_వెడల్పు_Min == 0) exp_వెడల్పు_Min = exp_వెడల్పు;
        ఒకవేళ (nodep->dtypep()->వెడల్పు() == exp_వెడల్పు) తిరిగి అబద్ధం;
        ఒకవేళ (nodep->dtypep()->వెడల్పు_Sized() && nodep->వెడల్పు() != exp_వెడల్పు_Min) తిరిగి నిజం;
        ఒకవేళ (!nodep->dtypep()->వెడల్పు_Sized() && nodep->వెడల్పు_Min() > exp_వెడల్పు_Min) తిరిగి నిజం;
        తిరిగి అబద్ధం;
    }

    శూన్య fix_వెడల్పు_Extend(Ast_కణుపు_సమాసం* nodep, Ast_కణుపు_DType* expDTypep, Extend_నియమం extend_నియమం) {
        // Fix the width mismatch by extending or truncating bits
        // *ONLY* call this from checkWidth()
        // Truncation is rarer, but can occur:  parameter [3:0] FOO = 64'h12312;
        // A(CONSTwide)+B becomes  A(CONSTwidened)+B
        // A(somewide)+B  becomes  A(TRUNC(somewide,width))+B
        //                    or   A(EXTRACT(somewide,width,0))+B
        // Sign extension depends on the type of the *present*
        // node, while the output dtype is the *expected* sign.
        // It is reasonable to have sign extension with unsigned output,
        // for example $unsigned(a)+$signed(b), the SIGNED(B) will be unsigned dtype out
        UINFO(4, "  widthExtend_(r=" << static_cast<పూర్ణాంకం>(extend_నియమం) << ") old: " << nodep);
        ఒకవేళ (extend_నియమం == EXTEND_OFF) తిరిగి;
        AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const);
        స్థిరరాశి పూర్ణాంకం exp_వెడల్పు = expDTypep->వెడల్పు();
        ఒకవేళ (constp && !constp->అంకె().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_సంఖ్య అంకె{nodep, exp_వెడల్పు, constp->అంకె()};
            అంకె.isSigned(అబద్ధం);
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), అంకె};
            constp->replaceWith(newp);
            VL_DO_DANGLING(నెట్టు_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } లేకపోతే ఒకవేళ (exp_వెడల్పు < nodep->వెడల్పు()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlink_Fr_వెనుక(&linker);
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstSel{nodep->fileline(), nodep, 0, exp_వెడల్పు};
            newp->did_వెడల్పు(నిజం);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } లేకపోతే {
            // Extend
            VNRelinker linker;
            nodep->unlink_Fr_వెనుక(&linker);
            తార్కిక doSigned = అబద్ధం;
            ఎంపిక (extend_నియమం) {
            స్థితి EXTEND_ZERO: doSigned = అబద్ధం; ఆపు;
            స్థితి EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); ఆపు;
            స్థితి EXTEND_LHS: doSigned = nodep->isSigned(); ఆపు;
            అప్రమేయ: nodep->v3fatalSrc("bad case");
            }
            Ast_కణుపు_సమాసం* స్థిరరాశి newp
                = (doSigned ? static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_కణుపు_సమాసం*>(కొత్త AstExtend{nodep->fileline(), nodep}));
            newp->did_వెడల్పు(నిజం);
            linker.relink(newp);
            nodep = newp;
        }
        ఒకవేళ (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = spliceCvtD(nodep);
            nodep = newp;
        }
        ఒకవేళ (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtype_Chg_వెడల్పు_Signed(exp_వెడల్పు, expDTypep->వెడల్పు_Min(), expDTypep->numeric());
        } లేకపోతే {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    శూన్య fix_వెడల్పు_Reduce(Ast_కణుపు_సమాసం* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        స్థిరరాశి పూర్ణాంకం exp_వెడల్పు = 1;
        స్థిరరాశి పూర్ణాంకం expSigned = అబద్ధం;
        ఒకవేళ (nodep->వెడల్పు() == exp_వెడల్పు) తిరిగి;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const);
        ఒకవేళ (constp) {
            V3_సంఖ్య అంకె(nodep, exp_వెడల్పు);
            అంకె.opRedOr(constp->అంకె());
            అంకె.isSigned(expSigned);
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), అంకె};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_వృక్షం(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } లేకపోతే {
            VNRelinker linker;
            nodep->unlink_Fr_వెనుక(&linker);
            Ast_కణుపు_సమాసం* newp;
            Ast_కణుపు_DType* స్థిరరాశి dtypeSkipp = nodep->dtypep()->skipRefp();
            ఒకవేళ (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = కొత్త AstNeq{nodep->fileline(),
                                  కొత్త AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } లేకపోతే {
                // But, AstRedOr can be faster on wide
                newp = కొత్త AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtype_Chg_వెడల్పు_Signed(exp_వెడల్పు, exp_వెడల్పు, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    తార్కిక fixAutoExtend(Ast_కణుపు_సమాసం*& nodepr, పూర్ణాంకం exp_వెడల్పు) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        ఒకవేళ (AstConst* స్థిరరాశి constp = VN_CAST(nodepr, Const)) {
            ఒకవేళ (constp->అంకె().autoExtend() && !constp->అంకె().sized() && constp->వెడల్పు() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_సంఖ్య అంకె(constp, exp_వెడల్పు);
                అంకె.opRepl(constp->అంకె(), exp_వెడల్పు);  // {width{'1}}
                Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstConst{constp->fileline(), అంకె};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_వృక్షం(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                తిరిగి నిజం;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            లేకపోతే ఒకవేళ (!constp->అంకె().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && exp_వెడల్పు > 32 && constp->అంకె().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << exp_వెడల్పు
                                                  << " bits: " << constp->prettyName());
                V3_సంఖ్య అంకె(constp, exp_వెడల్పు);
                అంకె.opExtendXZ(constp->అంకె(), constp->వెడల్పు());
                Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstConst{constp->fileline(), అంకె};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_వృక్షం(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;  // No change
    }
    తార్కిక is_పునాది_Class_Recurse(స్థిరరాశి AstClass* స్థిరరాశి cls1p, స్థిరరాశి AstClass* స్థిరరాశి cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        ఒకవేళ (cls1p == cls2p) తిరిగి నిజం;
        కొరకు (స్థిరరాశి AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            ఒకవేళ (is_పునాది_Class_Recurse(cls1p, cextp->classp())) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    // Lines to end a message on nodep with the data types of its operands
    స్థిర string warnDTypes(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి Ast_కణుపు_DType* lhsDtp,
                             స్థిరరాశి Ast_కణుపు_DType* rhsDtp, స్థిరరాశి string& lhsName = "Left-hand",
                             స్థిరరాశి string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        ఒకవేళ (lhsState == rhsState) lhsState = rhsState = "";
        తిరిగి "\n" + nodep->warn_మరిన్ని() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warn_మరిన్ని()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    స్థిర తార్కిక unpacked_Array_Sizes_సరిపోలిక(స్థిరరాశి Ast_కణుపు_DType* lhsDtp, స్థిరరాశి Ast_కణుపు_DType* rhsDtp) {
        అయితే (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            ఒకవేళ (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                తిరిగి అబద్ధం;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        తిరిగి నిజం;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    మూస <రకం_పేరు T, రకం_పేరు N>
    శూన్య checkUnpackedArrayAssignmentCompatible(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి T* స్థిరరాశి lhsRefp,
                                                స్థిరరాశి N* స్థిరరాశి rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::విలువ || std::is_same<T, Ast_కణుపు_Var_Ref>::విలువ)
                && (std::is_same<N, AstVar>::విలువ || std::is_same<N, Ast_కణుపు_Var_Ref>::విలువ),
            "Unsupported types provided.");
        ఒకవేళ (!lhsRefp || !rhsRefp) తిరిగి;
        std::string lhsName;
        std::string rhsName;
        ఒకవేళ (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } లేకపోతే {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి lhsDtp = lhsRefp->dtypep()->skipRefp();
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి rhsDtp = rhsRefp->dtypep()->skipRefp();
        స్థిరరాశి తార్కిక isLhsAggregate = lhsDtp->isAggregateType();
        స్థిరరాశి తార్కిక isRhsAggregate = rhsDtp->isAggregateType();
        ఒకవేళ (!isLhsAggregate && !isRhsAggregate) తిరిగి;
        ఒకవేళ (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            తిరిగి;
        } లేకపోతే ఒకవేళ (VN_IS(lhsDtp, వరుస_DType) && VN_IS(rhsDtp, ఖాళీలు_వరుస_DType)) {
            తిరిగి;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(అబద్ధం),
                                      rhsDim = rhsDtp->dimensions(అబద్ధం);
        // Check if unpacked array dimensions are matching
        ఒకవేళ (lhsDim.రెండవ != rhsDim.రెండవ) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            తిరిగి;
        }

        స్థిరరాశి Ast_కణుపు_DType* lhsDtpIterp = lhsDtp;
        స్థిరరాశి Ast_కణుపు_DType* rhsDtpIterp = rhsDtp;
        కొరకు (uint32_t dim = 0; dim < rhsDim.రెండవ; dim++) {
            // Associative arrays are compatible only with each other
            ఒకవేళ (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                తిరిగి;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        ఒకవేళ (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    శూన్య check_Class_కేటాయించు(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి అక్షరం* side, Ast_కణుపు* rhsp,
                          Ast_కణుపు_DType* స్థిరరాశి lhsDTypep, తార్కిక isOutputArg = అబద్ధం) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి lhsRawDTypep = lhsDTypep->skipRefp();
        స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి rhsRawDTypep = rhsp->dtypep()->skipRefp();
        ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            ఒకవేళ (స్థిరరాశి AstClassRefDType* స్థిరరాశి rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                స్థిరరాశి AstClass* స్థిరరాశి dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                స్థిరరాశి AstClass* స్థిరరాశి srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                ఒకవేళ (is_పునాది_Class_Recurse(dstp, srcp)) తిరిగి;
            } లేకపోతే ఒకవేళ (rhsp->isNull()) {
                తిరిగి;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } లేకపోతే ఒకవేళ (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        ఒకవేళ (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlink_Fr_వెనుక(&relinker);
            relinker.relink(
                కొత్త Ast_Cvt_Unpack_గమ్యం_వరుస{rhsp->fileline(), VN_AS(rhsp, కణుపు_సమాసం), lhsDTypep});
        }
    }
    స్థిర తార్కిక similarDTypeRecurse(స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి node1p,
                                    స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి node2p) {
        తిరిగి node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    శూన్య iterateCheckFileDesc(Ast_కణుపు* parentp, Ast_కణుపు* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, వెడల్పు_VP{SELF, PRELIM}.p());
        Ast_కణుపు_DType* స్థిరరాశి expDTypep = underp->కనుగొను_UInt32_DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (శూన్య)underp;  // cppcheck
    }
    శూన్య iterateCheckReal(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->కనుగొను_Double_DType(), stage);
    }
    శూన్య iterateCheckSigned8(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->కనుగొను_Signed8_DType(), stage);
    }
    శూన్య iterateCheckSigned32(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->కనుగొను_Int_DType(), stage);
    }
    శూన్య iterateCheckUInt32(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->కనుగొను_UInt32_DType(), stage);
    }
    శూన్య iterateCheckDelay(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        ఒకవేళ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, వెడల్పు_VP{SELF, PRELIM}.p());
        }
        ఒకవేళ (stage & FINAL) {
            Ast_కణుపు_DType* expDTypep;
            ఒకవేళ (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->కనుగొను_Double_DType();
            } లేకపోతే {
                File_పంక్తి* స్థిరరాశి newFl = కొత్త File_పంక్తి{underp->fileline()};
                newFl->warnOff(V3_Error_సంకేతం::WIDTHEXPAND, నిజం);
                underp->fileline(newFl);
                expDTypep = parentp->కనుగొను_బిట్టు_DType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, అబద్ధం);
        }
        (శూన్య)underp;  // cppcheck
    }
    శూన్య iterateCheckString(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->కనుగొను_String_DType(), stage);
    }
    శూన్య iterateCheckTyped(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp,
                           Ast_కణుపు_DType* expDTypep, Stage stage) {
        ఒకవేళ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, వెడల్పు_VP{expDTypep, PRELIM}.p());
        }
        ఒకవేళ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (శూన్య)underp;  // cppcheck
    }
    శూన్య iterateCheckTypedSelfPrelim(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp,
                                     Ast_కణుపు_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        ఒకవేళ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, వెడల్పు_VP{SELF, PRELIM}.p());
        }
        ఒకవేళ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (శూన్య)underp;  // cppcheck
    }
    శూన్య iterateCheckIntegralSelf(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, నిజం);
    }
    శూన్య iterateCheckSelf(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Determ determ,
                          Stage stage, తార్కిక integralOnly = అబద్ధం) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ఒకవేళ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, వెడల్పు_VP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, కణుపు_సమాసం) ? checkCvtUS(VN_AS(underp, కణుపు_సమాసం), integralOnly) : underp;
        Ast_కణుపు_DType* స్థిరరాశి expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (శూన్య)underp;  // cppcheck
    }
    శూన్య iterateCheckSizedSelf(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Determ determ,
                               Stage stage, తార్కిక deferUntyped = అబద్ధం) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ఒకవేళ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, వెడల్పు_VP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, కణుపు_సమాసం) ? checkCvtUS(VN_AS(underp, కణుపు_సమాసం), అబద్ధం) : underp;
        Ast_కణుపు_DType* స్థిరరాశి expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_కణుపు_DType* స్థిరరాశి checkDtp = expDTypep->skip_Ref_గమ్యం_Enump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        ఒకవేళ (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (శూన్య)underp;  // cppcheck
    }
    శూన్య iterate_Check_కేటాయించు(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* rhsp, Stage stage,
                            Ast_కణుపు_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        ఒకవేళ (AstConst* constp = VN_CAST(rhsp, Const)) {
            ఒకవేళ (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                ఒకవేళ (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    ఒకవేళ (basicp->వెడల్పు() == 8 && constp->అంకె().isFromString()) {
                        AstConsArray* newp = కొత్త AstConsArray{
                            constp->fileline(), lhsDTypep,
                            కొత్త AstConst{constp->fileline(), AstConst::Widthed_విలువ{}, 8, 0}};
                        కొరకు (స్థిరరాశి పూర్ణాంకం aindex : arrayp->declRange().seq_Lo_గమ్యం_Hi()) {
                            పూర్ణాంకం cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_సంఖ్య selected{constp, 8};
                            selected.opSel(constp->అంకె(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            ఒకవేళ (!selected.isFourState()) {
                                ఒకవేళ (స్థిరరాశి uint32_t c = selected.గమ్యం_UInt()) {
                                    newp->valuep(aindex,
                                                 కొత్త AstConst{constp->fileline(),
                                                              AstConst::Widthed_విలువ{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(నెట్టు_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        check_Class_కేటాయించు(parentp, side, rhsp, lhsDTypep);
        స్థిరరాశి తార్కిక lhs_ప్రవాహం = (VN_IS(parentp, కణుపు_కేటాయించు)
                                && VN_IS(VN_AS(parentp, కణుపు_కేటాయించు)->lhsp(), కణుపు_ప్రవాహం));
        rhsp = iterateCheck(parentp, side, rhsp, కేటాయించు_3, FINAL, lhsDTypep,
                            lhs_ప్రవాహం ? EXTEND_OFF : EXTEND_LHS, నిజం, ప్రవాహం_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (శూన్య)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_కణుపు_సమాసం* convert_Unpacked_క్రమం(Ast_కణుపు_సమాసం* exprp, Ast_కణుపు_DType* dtypep) {
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ఒకవేళ (!arrayp || !arrayp->is_తిరగేయి_యొక్క(exprp->dtypep())) తిరిగి exprp;
        VNRelinker relinker;
        exprp->unlink_Fr_వెనుక(&relinker);
        Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త Ast_Cvt_Unpack_గమ్యం_Unpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        తిరిగి newp;
    }

    స్థిర తార్కిక isLogicalOperator(స్థిరరాశి Ast_కణుపు* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        తిరిగి VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    శూన్య iterateCheckBool(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, వెడల్పు_VP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_కణుపు_DType* స్థిరరాశి underVDTypep = underp ? underp->dtypep()->skipRefp() : శూన్యసూచిక;
        ఒకవేళ (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_వెనుక(&linker);
            Ast_కణుపు* స్థిరరాశి newp
                = కొత్త AstNeqD{parentp->fileline(), VN_AS(underp, కణుపు_సమాసం),
                              కొత్త AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } లేకపోతే ఒకవేళ (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_వెనుక(&linker);
            Ast_కణుపు* స్థిరరాశి newp
                = కొత్త AstNeqN{parentp->fileline(), VN_AS(underp, కణుపు_సమాసం),
                              కొత్త AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } లేకపోతే ఒకవేళ (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fix_వెడల్పు_Reduce(VN_AS(underp, కణుపు_సమాసం)), underp);  //మారింది
        } లేకపోతే ఒకవేళ (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(కొత్త AstConst{parentp->fileline(), AstConst::బిట్టు_False_Erroring{}});
            VL_DO_DANGLING(నెట్టు_Deletep(underp), underp);
        } లేకపోతే {
            స్థిరరాశి తార్కిక bad = వెడల్పు_Bad(underp, parentp->కనుగొను_బిట్టు_DType());
            ఒకవేళ (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->వెడల్పు(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->వెడల్పు()
                                             << (underp->వెడల్పు() != underp->వెడల్పు_Min()
                                                     ? " or " + cvt_గమ్యం_Str(underp->వెడల్పు_Min())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            ఒకవేళ (Ast_సమాసం_Stmt* స్థిరరాశి సమాసం_Stmtp = VN_CAST(underp, సమాసం_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fix_వెడల్పు_Reduce(సమాసం_Stmtp->resultp());
                సమాసం_Stmtp->dtypeFrom(సమాసం_Stmtp->resultp());
            } లేకపోతే ఒకవేళ (underp->వెడల్పు() != 1) {
                VL_DO_DANGLING(fix_వెడల్పు_Reduce(VN_AS(underp, కణుపు_సమాసం)), underp);  //మారింది
            }
        }
    }

    స్థిర శూన్య checkEnum(Ast_కణుపు* nodep, Ast_కణుపు* parentp, స్థిరరాశి Ast_కణుపు_DType* లక్ష్యం_DTypep,
                          స్థిరరాశి Ast_కణుపు_DType* ఆధారం_DTypep, స్థిరరాశి Ast_కణుపు_సమాసం* ఆధారం_Exprp) {
        ఒకవేళ (స్థిరరాశి AstEnumDType* స్థిరరాశి లక్ష్యం_Enump
            = VN_CAST(లక్ష్యం_DTypep->skip_Ref_గమ్యం_Enump(), EnumDType)) {
            స్థిరరాశి VCastable castable
                = Ast_కణుపు::computeCastable(లక్ష్యం_Enump, ఆధారం_DTypep, ఆధారం_Exprp);
            ఒకవేళ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << లక్ష్యం_DTypep->prettyDTypeNameQ() << " from "
                                             << ఆధారం_DTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warn_మరిన్ని()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_కణుపు* iterateCheck(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side, Ast_కణుపు* underp, Determ determ,
                          Stage stage, Ast_కణుపు_DType* expDTypep, Extend_నియమం extend_నియమం,
                          తార్కిక warnOn = నిజం, ప్రవాహం_Use ప్రవాహం_Use_2 = ప్రవాహం_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        స్థిరరాశి ప్రవాహం_Use బిడ్డ_ప్రవాహం_Use
            = ప్రవాహం_Use_2 != ప్రవాహం_USE_NONE ? ప్రవాహం_Use_2 : ప్రవాహం_Use_For_Determ(determ);
        ఒకవేళ (rhsAccess.isReadOnly()) {
            ఒకవేళ (Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_CAST(underp, కణుపు_సమాసం)) {
                // A value only read is converted to a reversed expected type.
                underp = convert_Unpacked_క్రమం(exprp, expDTypep);
            }
        }
        ఒకవేళ (VN_IS(underp, కణుపు_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } లేకపోతే ఒకవేళ (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   వెడల్పు_VP{expDTypep, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
        } లేకపోతే ఒకవేళ (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   వెడల్పు_VP{expDTypep, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
        } లేకపోతే ఒకవేళ (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_కణుపు* స్థిరరాశి oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, కణుపు_సమాసం));
            underp = userIterateSubtreeReturnEdits(oldp, వెడల్పు_VP{SELF, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
        } లేకపోతే ఒకవేళ (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_కణుపు* స్థిరరాశి oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, కణుపు_సమాసం), నిజం, expDTypep->వెడల్పు());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, వెడల్పు_VP{SELF, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
        } లేకపోతే ఒకవేళ (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_కణుపు* స్థిరరాశి oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, కణుపు_సమాసం));
            underp = userIterateSubtreeReturnEdits(oldp, వెడల్పు_VP{SELF, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
        } లేకపోతే {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి underDtp = underp->dtypep()->skipRefp();
            ఒకవేళ (determ != కేటాయించు_3 && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_కణుపు* స్థిరరాశి newp
                    = కొత్త AstConst{underp->fileline(), AstConst::బిట్టు_False_Erroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(నెట్టు_Deletep(underp), underp);
                underp = newp;
            }
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి expDtp = expDTypep->skipRefp();
            ఒకవేళ (determ == కేటాయించు_3 && VN_IS(expDtp, కణుపు_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            ఒకవేళ (determ == కేటాయించు_3 && !isIfaceArrayDType(expDtp)
                && !unpacked_Array_Sizes_సరిపోలిక(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            స్థిరరాశి AstBasicDType* స్థిరరాశి expBasicp = expDTypep->basicp();
            స్థిరరాశి AstBasicDType* స్థిరరాశి underBasicp = underp->dtypep()->basicp();
            ఒకవేళ (expBasicp && underBasicp) {
                ఒకవేళ (!m_enumItemp && !parentp->fileline()->warnIsOff(V3_Error_సంకేతం::ENUMVALUE)
                    && warnOn) {
                    ఒకవేళ (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, కణుపు_సమాసం));
                    }
                    ఒకవేళ (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, శూన్యసూచిక);
                    }
                }
                Ast_కణుపు_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                ఒకవేళ (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, వెడల్పు_VP{SELF, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
                } లేకపోతే ఒకవేళ (determ == కేటాయించు_3) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    ఒకవేళ (underp->isSigned() != subDTypep->isSigned()
                        || underp->వెడల్పు() != subDTypep->వెడల్పు()) {
                        subDTypep = parentp->కనుగొను_బిట్టు_Or_Logic_DType(
                            std::max(subDTypep->వెడల్పు(), underp->వెడల్పు()),
                            std::max(subDTypep->వెడల్పు_Min(), underp->వెడల్పు_Min()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, వెడల్పు_VP{subDTypep, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
                } లేకపోతే {
                    underp = userIterateSubtreeReturnEdits(
                        underp, వెడల్పు_VP{subDTypep, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                వెడల్పు_Check_Sized(parentp, side, VN_AS(underp, కణుపు_సమాసం), expDTypep, extend_నియమం,
                                warnOn);
            } లేకపోతే ఒకవేళ (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                స్థిరరాశి AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                ఒకవేళ (VN_IS(underp, Const) && VN_AS(underp, Const)->అంకె().isNull()) {
                    // '= null' is ok
                } లేకపోతే ఒకవేళ (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_ద్వారా_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } లేకపోతే ఒకవేళ (expIfaceRefp->iface_ద్వారా_Cellp() != underIfaceRefp->iface_ద్వారా_Cellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_ద్వారా_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->iface_ద్వారా_Cellp()->prettyNameQ() << ").");
                } లేకపోతే ఒకవేళ (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } లేకపోతే {
                    underp = userIterateSubtreeReturnEdits(
                        underp, వెడల్పు_VP{expDTypep, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
                }
            } లేకపోతే {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, వెడల్పు_VP{expDTypep, FINAL, బిడ్డ_ప్రవాహం_Use}.p());
            }
        }
        తిరిగి underp;
    }

    శూన్య
    వెడల్పు_Check_Sized(Ast_కణుపు* parentp, స్థిరరాశి అక్షరం* side,
                    Ast_కణుపు_సమాసం* underp,  // Node to be checked or have typecast added in front of
                    Ast_కణుపు_DType* expDTypep, Extend_నియమం extend_నియమం, తార్కిక warnOn = నిజం) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        స్థిరరాశి AstBasicDType* స్థిరరాశి expBasicp = expDTypep->basicp();
        స్థిరరాశి AstBasicDType* స్థిరరాశి underBasicp = underp->dtypep()->basicp();
        ఒకవేళ (expDTypep == underp->dtypep()) {
            తిరిగి;  // Same type must match
        } లేకపోతే ఒకవేళ (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#ఒకవేళ VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#అయితే_ముగింపు
            తిరిగి;
        } లేకపోతే {
            స్థిరరాశి పూర్ణాంకం exp_వెడల్పు = expDTypep->వెడల్పు();
            పూర్ణాంకం exp_వెడల్పు_Min = expDTypep->వెడల్పు_Min();
            ఒకవేళ (exp_వెడల్పు_Min == 0) exp_వెడల్పు_Min = exp_వెడల్పు;
            స్థిరరాశి తార్కిక bad = వెడల్పు_Bad(underp, expDTypep);
            ఒకవేళ ((bad || underp->వెడల్పు() != exp_వెడల్పు) && fixAutoExtend(underp /*ref*/, exp_వెడల్పు)) {
                underp = శూన్యసూచిక;  // Changes underp
                తిరిగి;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            ఒకవేళ (VN_IS(parentp->backp(), Cast_కొలత)) warnOn = అబద్ధం;
            ఒకవేళ (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = అబద్ధం;
            ఒకవేళ (VN_IS(underp, Const) && VN_AS(underp, Const)->అంకె().isFromString()
                && exp_వెడల్పు > underp->వెడల్పు()
                && (((exp_వెడల్పు - underp->వెడల్పు()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = అబద్ధం;
            }
            ఒకవేళ ((VN_IS(parentp, కూడు_2) && underp->వెడల్పు() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->వెడల్పు() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = అబద్ధం;
            }
            ఒకవేళ (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    exp_వెడల్పు, underp->వెడల్పు(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << exp_వెడల్పు
                        << (exp_వెడల్పు != exp_వెడల్పు_Min ? " or " + cvt_గమ్యం_Str(exp_వెడల్పు_Min) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->వెడల్పు()
                        << (underp->వెడల్పు() != underp->వెడల్పు_Min()
                                ? " or " + cvt_గమ్యం_Str(underp->వెడల్పు_Min())
                                : "")
                        << " bits.");
            }
            ఒకవేళ (bad || underp->వెడల్పు() != exp_వెడల్పు) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                స్థిరరాశి Ast_కణుపు_కేటాయించు* assignp = VN_CAST(parentp, కణుపు_కేటాయించు);
                స్థిరరాశి AstPin* pinp = VN_CAST(parentp, Pin);
                ఒకవేళ (assignp && VN_IS(assignp->lhsp(), కణుపు_ప్రవాహం)) {
                } లేకపోతే ఒకవేళ (pinp && pinp->modVarp()->direction() != VDirection::ఆదానం) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } లేకపోతే ఒకవేళ (assignp && VN_IS(underp, కణుపు_ప్రవాహం) && exp_వెడల్పు > underp->వెడల్పు()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    స్థిరరాశి పూర్ణాంకం జరుపు_2 = exp_వెడల్పు - underp->వెడల్పు();
                    underp->unlink_Fr_వెనుక(&linker);
                    AstExtend* స్థిరరాశి widenedp = కొత్త AstExtend{underp->fileline(), underp};
                    widenedp->did_వెడల్పు(నిజం);
                    // Shift left so zeros fill on the right
                    Ast_కణుపు_సమాసం* స్థిరరాశి shiftedp = కొత్త Ast_జరుపు_L{
                        underp->fileline(), widenedp,
                        కొత్త AstConst{underp->fileline(), static_cast<uint32_t>(జరుపు_2)}, exp_వెడల్పు};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } లేకపోతే {
                    VL_DO_DANGLING(fix_వెడల్పు_Extend(underp, expDTypep, extend_నియమం), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_కణుపు_సమాసం* checkCvtUS(Ast_కణుపు_సమాసం* nodep, తార్కిక fatal) {
        ఒకవేళ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            ఒకవేళ (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } లేకపోతే {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, అబద్ధం, 32);
        }
        తిరిగి nodep;
    }

    Ast_కణుపు_సమాసం* spliceCvtD(Ast_కణుపు_సమాసం* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        ఒకవేళ (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_వెనుక(&linker);
            Ast_కణుపు_సమాసం* newp;
            ఒకవేళ (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = కొత్త AstISToRD{nodep->fileline(), nodep};
            } లేకపోతే {
                newp = కొత్త AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            తిరిగి newp;
        } లేకపోతే {
            తిరిగి nodep;
        }
    }
    Ast_కణుపు_సమాసం* spliceCvtS(Ast_కణుపు_సమాసం* nodep, తార్కిక warnOn, పూర్ణాంకం వెడల్పు) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ఒకవేళ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_వెనుక(&linker);
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                ఒకవేళ (constp->isDouble() && V3_సంఖ్య::epsilonIntegral(constp->అంకె().గమ్యం_Double())) {
                    warnOn = అబద్ధం;
                }
            }
            ఒకవేళ (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtype_సమితి_బిట్టు_Sized(వెడల్పు, VSigning::SIGNED);
            తిరిగి newp;
        } లేకపోతే {
            తిరిగి nodep;
        }
    }
    Ast_కణుపు_సమాసం* spliceCvtString(Ast_కణుపు_సమాసం* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ఒకవేళ (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_వెనుక(&linker);
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            తిరిగి newp;
        } లేకపోతే {
            తిరిగి nodep;
        }
    }
    Ast_కణుపు_Biop* replaceWithUOrSVersion(Ast_కణుపు_Biop* nodep, తార్కిక signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ఒకవేళ (signedFlavorNeeded == nodep->signedFlavor()) తిరిగి శూన్యసూచిక;
        ఒకవేళ (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        ఎంపిక (nodep->type()) {
        స్థితి VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        స్థితి VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        స్థితి VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        స్థితి VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        స్థితి VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        స్థితి VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        స్థితి VNType::కూడు_2: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        స్థితి VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        స్థితి VNType::జరుపు_L: nodep->dtypeChgSigned(signedFlavorNeeded); తిరిగి శూన్యసూచిక;
        అప్రమేయ: ఆపు;
        }
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_Biop* newp = శూన్యసూచిక;
        ఎంపిక (nodep->type()) {
        స్థితి VNType::Gt: newp = కొత్త AstGtS{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::GtS: newp = కొత్త AstGt{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Gte: newp = కొత్త AstGteS{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::GteS: newp = కొత్త AstGte{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Lt: newp = కొత్త AstLtS{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::LtS: newp = కొత్త AstLt{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Lte: newp = కొత్త AstLteS{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::LteS: newp = కొత్త AstLte{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Div: newp = కొత్త AstDivS{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::DivS: newp = కొత్త AstDiv{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::ModDiv: newp = కొత్త AstModDivS{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::ModDivS: newp = కొత్త AstModDiv{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Mul: newp = కొత్త AstMulS{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::MulS: newp = కొత్త AstMul{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::జరుపు_R: newp = కొత్త Ast_జరుపు_RS{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::జరుపు_RS: newp = కొత్త Ast_జరుపు_R{fl, lhsp, rhsp}; ఆపు;
        అప్రమేయ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            ఆపు;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి newp;
    }
    Ast_కణుపు_Biop* replaceWithDVersion(Ast_కణుపు_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ఒకవేళ (nodep->doubleFlavor()) తిరిగి శూన్యసూచిక;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_Biop* newp = శూన్యసూచిక;
        // No width change on output;...                // All below have bool or double outputs
        ఎంపిక (nodep->type()) {
        స్థితి VNType::కూడు_2: newp = కొత్త Ast_కూడు_D{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Sub: newp = కొత్త AstSubD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Pow: newp = కొత్త AstPowD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Eq:
        స్థితి VNType::EqCase: newp = కొత్త AstEqD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Neq:
        స్థితి VNType::NeqCase: newp = కొత్త AstNeqD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Gt:
        స్థితి VNType::GtS: newp = కొత్త AstGtD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Gte:
        స్థితి VNType::GteS: newp = కొత్త AstGteD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Lt:
        స్థితి VNType::LtS: newp = కొత్త AstLtD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Lte:
        స్థితి VNType::LteS: newp = కొత్త AstLteD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Div:
        స్థితి VNType::DivS: newp = కొత్త AstDivD{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Mul:
        స్థితి VNType::MulS: newp = కొత్త AstMulD{fl, lhsp, rhsp}; ఆపు;
        అప్రమేయ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ఆపు;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి newp;
    }
    Ast_కణుపు_Biop* replaceWithNVersion(Ast_కణుపు_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        ఒకవేళ (nodep->stringFlavor()) తిరిగి శూన్యసూచిక;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_Biop* newp = శూన్యసూచిక;
        // No width change on output;...                // All below have bool or double outputs
        ఎంపిక (nodep->type()) {
        స్థితి VNType::Eq:
        స్థితి VNType::EqCase:
        స్థితి VNType::EqWild: newp = కొత్త AstEqN{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Neq:
        స్థితి VNType::NeqCase:
        స్థితి VNType::NeqWild: newp = కొత్త AstNeqN{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Gt:
        స్థితి VNType::GtS: newp = కొత్త AstGtN{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Gte:
        స్థితి VNType::GteS: newp = కొత్త AstGteN{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Lt:
        స్థితి VNType::LtS: newp = కొత్త AstLtN{fl, lhsp, rhsp}; ఆపు;
        స్థితి VNType::Lte:
        స్థితి VNType::LteS: newp = కొత్త AstLteN{fl, lhsp, rhsp}; ఆపు;
        అప్రమేయ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            ఆపు;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి newp;
    }
    Ast_కణుపు_Uniop* replaceWithDVersion(Ast_కణుపు_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ఒకవేళ (nodep->doubleFlavor()) తిరిగి శూన్యసూచిక;
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = nodep->lhsp()->unlink_Fr_వెనుక();
        Ast_కణుపు_Uniop* newp = శూన్యసూచిక;
        ఎంపిక (nodep->type()) {
        స్థితి VNType::Negate: newp = కొత్త AstNegateD{fl, lhsp}; ఆపు;
        అప్రమేయ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ఆపు;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
        తిరిగి newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    శూన్య replaceWithSFormat(Ast_పద్ధతి_Call* nodep, స్థిరరాశి string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* స్థిరరాశి argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_కణుపు_Var_Ref* స్థిరరాశి fromp = VN_AS(nodep->fromp()->unlink_Fr_వెనుక(), VarRef);
        Ast_కణుపు* స్థిరరాశి newp = కొత్త Ast_కేటాయించు{
            nodep->fileline(), fromp,
            కొత్త AstSFormatF{nodep->fileline(), format, అబద్ధం, argp->exprp()->unlink_Fr_వెనుక()}};
        fromp->access(VAccess::WRITE);
        నెట్టు_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_కణుపు_DType* iterateEditMoveDTypep(Ast_కణుపు* parentp, Ast_కణుపు_DType* dtnodep) {
        UASSERT_OBJ(dtnodep, parentp, "Caller should check for nullptr before computing dtype");
        // Iterate into a data type to resolve that type.
        // The data type may either:
        // 1. Be a child (typically getChildDTypep() returns it)
        //    DTypes at parse time get added as these to some node types
        //    such as AstVars.
        //    This function will move it to global scope (that is #2
        //    will now apply).
        // 2. Be under the Netlist and pointed to by an Ast member variable
        //    (typically refDTypep() or dtypep() returns it)
        //    so removing/changing a variable won't lose the dtype

        // Case #1 above applies?
        స్థిరరాశి తార్కిక child1 = (parentp->get_బిడ్డ_DTypep() == dtnodep);
        స్థిరరాశి తార్కిక child2 = (parentp->getChild2DTypep() == dtnodep);
        ఒకవేళ (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, శూన్యసూచిక), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->get_బిడ్డ_DTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->did_వెడల్పు(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlink_Fr_వెనుక();
            v3Global.rootp()->typeTablep()->కూడు_Typesp(dtnodep);
        }
        ఒకవేళ (!dtnodep->did_వెడల్పు()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, శూన్యసూచిక);
            UASSERT_OBJ(dtnodep->did_వెడల్పు(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        తిరిగి dtnodep;
    }

    AstConst* dimension_విలువ(File_పంక్తి* fileline, Ast_కణుపు_DType* nodep, VAttrType attrType,
                             పూర్ణాంకం dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_కణుపు_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        కొరకు (పూర్ణాంకం i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            ఒకవేళ (స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి adtypep = VN_CAST(dtypep, కణుపు_Array_DType)) {
                declRange = adtypep->declRange();
                ఒకవేళ (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                కొనసాగు;
            } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి adtypep
                       = VN_CAST(dtypep, కణుపు_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                ఆపు;  // Sub elements don't look like arrays and can't iterate into
            } లేకపోతే ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి adtypep = VN_CAST(dtypep, BasicDType)) {
                ఒకవేళ (adtypep->isRanged()) declRange = adtypep->declRange();
                ఆపు;
            }
            ఆపు;  // LCOV_EXCL_LINE
        }
        AstConst* valp = శూన్యసూచిక;  // If nullptr, construct from val
        పూర్ణాంకం val = 0;
        ఎంపిక (attrType) {
        స్థితి VAttrType::DIM_బిట్లు: {
            పూర్ణాంకం బిట్లు = 1;
            అయితే (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                ఒకవేళ (స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి adtypep = VN_CAST(dtypep, కణుపు_Array_DType)) {
                    బిట్లు *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    కొనసాగు;
                } లేకపోతే ఒకవేళ (స్థిరరాశి Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి adtypep
                           = VN_CAST(dtypep, కణుపు_UOr_Struct_DType)) {
                    బిట్లు *= adtypep->isStreamableFixedAggregate() ? adtypep->వెడల్పు_ప్రవాహం()
                                                                  : adtypep->వెడల్పు();
                    ఆపు;
                } లేకపోతే ఒకవేళ (స్థిరరాశి AstBasicDType* స్థిరరాశి adtypep = VN_CAST(dtypep, BasicDType)) {
                    బిట్లు *= adtypep->వెడల్పు();
                    ఆపు;
                }
                ఆపు;
            }
            ఒకవేళ (dim == 0) {
                val = 0;
            } లేకపోతే ఒకవేళ (dim == 1 && !declRange.ranged()
                       && బిట్లు == 1) {  // $bits should be sane for non-arrays
                val = nodep->వెడల్పు();
            } లేకపోతే {
                val = బిట్లు;
            }
            ఆపు;  // LCOV_EXCL_LINE
        }
        స్థితి VAttrType::DIM_ఎగువ: val = !declRange.ranged() ? 0 : declRange.hi(); ఆపు;
        స్థితి VAttrType::DIM_ఎడమ: val = !declRange.ranged() ? 0 : declRange.ఎడమ(); ఆపు;
        స్థితి VAttrType::DIM_దిగువ: val = !declRange.ranged() ? 0 : declRange.lo(); ఆపు;
        స్థితి VAttrType::DIM_కుడి: val = !declRange.ranged() ? 0 : declRange.కుడి(); ఆపు;
        స్థితి VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            ఆపు;
        స్థితి VAttrType::DIM_కొలత: val = !declRange.ranged() ? 0 : declRange.elements(); ఆపు;
        అప్రమేయ: nodep->v3fatalSrc("Missing DIM ATTR type case"); ఆపు;
        }
        ఒకవేళ (!valp) valp = కొత్త AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvt_గమ్యం_Hex(dtypep) << "," << dim
                                << ")=" << valp);
        తిరిగి valp;
    }
    AstVarRef* dimensionVarRefp(Ast_కణుపు_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_పట్టిక_Map[std::make_pair(nodep, attrType)];
        ఒకవేళ (cachedpr) తిరిగి cachedpr->clone_వృక్షం(అబద్ధం);

        Ast_కణుపు_Array_DType* స్థిరరాశి vardtypep = కొత్త AstUnpackArrayDType{
            nodep->fileline(), nodep->కనుగొను_Int_DType(), కొత్త AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(vardtypep);
        userIterate(vardtypep, వెడల్పు_VP{SELF, BOTH}.p());
        AstConsArray* స్థిరరాశి consp = కొత్త AstConsArray{nodep->fileline(), vardtypep};
        కొరకు (గుర్తులేని i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_విలువ(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::కనుగొను(consp);
        VL_DO_DANGLING(consp->delete_వృక్షం(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->did_వెడల్పు(నిజం);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_వెడల్పు(నిజం);
        నెట్టు_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        తిరిగి cachedpr->clone_వృక్షం(అబద్ధం);
    }
    స్థిర uint64_t enum_Max_విలువ(స్థిరరాశి Ast_కణుపు* errNodep, స్థిరరాశి AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        కొరకు (స్థిరరాశి Ast_Enum_అంశం* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
            స్థిరరాశి AstConst* స్థిరరాశి vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            ఒకవేళ (!vconstp->అంకె().isAnyXZ() && vconstp->గమ్యం_UQuad() >= maxval)
                maxval = vconstp->గమ్యం_UQuad();
        }
        ఒకవేళ (adtypep->itemsp()->వెడల్పు() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            తిరిగి 64;
        }
        తిరిగి maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* స్థిరరాశి nodep, VAttrType attrType, తార్కిక assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_పట్టిక_Map[std::make_pair(nodep, attrType)];
        ఒకవేళ (cachedpr) తిరిగి cachedpr->clone_వృక్షం(అబద్ధం);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        Ast_కణుపు_DType* basep;
        ఒకవేళ (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->కనుగొను_String_DType();
        } లేకపోతే ఒకవేళ (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->కనుగొను_బిట్టు_DType();
        } లేకపోతే {
            basep = nodep->dtypep();
        }
        Ast_కణుపు_DType* vardtypep;
        ఒకవేళ (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = కొత్త AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } లేకపోతే {
            vardtypep = కొత్త AstUnpackArrayDType{flp, basep, కొత్త AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->కూడు_Typesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = శూన్యసూచిక;
        ఒకవేళ (attrType == VAttrType::ENUM_NAME) {
            defaultp = కొత్త AstConst{flp, AstConst::String{}, ""};
        } లేకపోతే ఒకవేళ (attrType == VAttrType::ENUM_తదుపరి || attrType == VAttrType::ENUM_PREV) {
            defaultp = కొత్త AstConst{flp, V3_సంఖ్య{nodep, nodep->వెడల్పు(), 0}};
        } లేకపోతే ఒకవేళ (attrType == VAttrType::ENUM_VALID) {
            defaultp = కొత్త AstConst{flp, AstConst::బిట్టు_False{}};
        } లేకపోతే {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_కణుపు_సమాసం*> విలువలు;
        {
            uint64_t ordinal = 0;
            Ast_Enum_అంశం* స్థిరరాశి firstp = nodep->itemsp();
            స్థిరరాశి Ast_Enum_అంశం* prevp = firstp;  // Prev must start with last item
            అయితే (prevp->nextp()) prevp = VN_AS(prevp->nextp(), Enum_అంశం);
            కొరకు (Ast_Enum_అంశం* itemp = firstp; itemp;) {
                Ast_Enum_అంశం* స్థిరరాశి nextp = VN_AS(itemp->nextp(), Enum_అంశం);
                స్థిరరాశి AstConst* స్థిరరాశి vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                ఒకవేళ (!vconstp->అంకె().isAnyXZ()) {  // Can 2-state runtime decode
                    స్థిరరాశి uint64_t i = nodep->isWide() ? ++ordinal : vconstp->గమ్యం_UQuad();
                    ఒకవేళ (attrType == VAttrType::ENUM_NAME) {
                        విలువలు[i] = కొత్త AstConst{flp, AstConst::String{},
                                                 V3_సంఖ్య::displayedEnumName(itemp)};
                    } లేకపోతే ఒకవేళ (attrType == VAttrType::ENUM_తదుపరి) {
                        విలువలు[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_వృక్షం(అబద్ధం);  // A const
                    } లేకపోతే ఒకవేళ (attrType == VAttrType::ENUM_PREV) {
                        విలువలు[i] = prevp->valuep()->clone_వృక్షం(అబద్ధం);  // A const
                    } లేకపోతే ఒకవేళ (attrType == VAttrType::ENUM_VALID) {
                        విలువలు[i] = కొత్త AstConst{flp, AstConst::బిట్టు_True{}};
                    } లేకపోతే {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_కణుపు_సమాసం* valuep = శూన్యసూచిక;
        ఒకవేళ (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* స్థిరరాశి consp = కొత్త AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            కొరకు (స్థిరరాశి స్వయంచాలక& itr : విలువలు) {
                V3_సంఖ్య కీలకం{nodep, nodep->వెడల్పు()};
                కీలకం.సమితి_Quad(itr.మొదటి);
                consp->కూడు_Itemsp(కొత్త Ast_Cons_Assoc_అంశం{flp, కొత్త AstConst{flp, కీలకం}, itr.రెండవ});
            }
            valuep = consp;
        } లేకపోతే {
            AstConsArray* స్థిరరాశి consp = కొత్త AstConsArray{flp, vardtypep, defaultp};
            కొరకు (uint64_t i = 0; i < (msbdim + 1); ++i) {
                ఒకవేళ (విలువలు[i]) consp->valuep(i, విలువలు[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::కనుగొను(valuep);
        VL_DO_DANGLING(నెట్టు_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->did_వెడల్పు(నిజం);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_వెడల్పు(నిజం);
        నెట్టు_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        తిరిగి cachedpr->clone_వృక్షం(అబద్ధం);
    }

    Ast_కణుపు_సమాసం* enumSelect(Ast_కణుపు_సమాసం* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        ఒకవేళ (attrType == VAttrType::ENUM_NAME) {
            ఒకవేళ (nodep->is_శుద్ధ()) nodep = V3Const::constifyEdit(nodep);
            ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->అంకె().displayedEnumName(adtypep)};
                VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
                తిరిగి newp;
            }
        }
        ఒకవేళ (attrType == VAttrType::ENUM_NAME && !nodep->is_శుద్ధ() && (m_modep || m_ftaskp)) {
            Ast_సమాసం_Stmt* స్థిరరాశి capturep = newEnumCapture(nodep, adtypep);
            Ast_కణుపు_సమాసం* స్థిరరాశి valuep = capturep->resultp()->unlink_Fr_వెనుక();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtype_సమితి_String();
            తిరిగి capturep;
        }
        ఒకవేళ (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_కణుపు_సమాసం* indexp = కొత్త AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            కొరకు (స్థిరరాశి Ast_Enum_అంశం* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_అంశం)) {
                స్థిరరాశి AstConst* స్థిరరాశి constp = VN_AS(itemp->valuep(), Const);
                ఒకవేళ (constp->అంకె().isAnyXZ()) కొనసాగు;
                indexp = కొత్త AstCond{nodep->fileline(),
                                     కొత్త AstEq{nodep->fileline(), nodep->clone_వృక్షం(అబద్ధం),
                                               itemp->valuep()->clone_వృక్షం(అబద్ధం)},
                                     కొత్త AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* స్థిరరాశి tabRefp = enumVarRefp(adtypep, attrType, అబద్ధం, ordinal);
            Ast_కణుపు_సమాసం* స్థిరరాశి newp = కొత్త AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtype_సమితి_String();
            VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
            తిరిగి newp;
        }
        స్థిరరాశి uint64_t msbdim = enum_Max_విలువ(nodep, adtypep);
        స్థిరరాశి తార్కిక assoc = msbdim > ENUM_LOOKUP_బిట్లు;
        Ast_కణుపు_సమాసం* newp;
        ఒకవేళ (assoc) {
            AstVarRef* స్థిరరాశి tabRefp = enumVarRefp(adtypep, attrType, నిజం, 0);
            newp = కొత్త AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } లేకపోతే {
            స్థిరరాశి పూర్ణాంకం selwidth = V3_సంఖ్య::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* స్థిరరాశి tabRefp
                = enumVarRefp(adtypep, attrType, అబద్ధం, (1ULL << selwidth) - 1);
            Ast_కణుపు_సమాసం* స్థిరరాశి boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->వెడల్పు() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? కొత్త AstGt{nodep->fileline(), nodep->clone_వృక్షం(అబద్ధం),
                                  కొత్త AstConst{nodep->fileline(),
                                               V3_సంఖ్య{nodep, adtypep->వెడల్పు(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : శూన్యసూచిక;
            newp = కొత్త AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                కొత్త AstSel{nodep->fileline(), nodep, 0, selwidth}};
            ఒకవేళ (boundp) {
                newp = కొత్త AstCond{nodep->fileline(), boundp,
                                   కొత్త AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        ఒకవేళ (attrType == VAttrType::ENUM_NAME) {
            newp->dtype_సమితి_String();
        } లేకపోతే ఒకవేళ (attrType == VAttrType::ENUM_VALID) {
            newp->dtype_సమితి_బిట్టు();
        } లేకపోతే {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        తిరిగి newp;
    }
    Ast_కణుపు_సమాసం* enumTestValid(Ast_కణుపు_సమాసం* valp, AstEnumDType* enumDtp) {
        స్థిరరాశి uint64_t maxval = enum_Max_విలువ(valp, enumDtp);
        స్థిరరాశి తార్కిక assoc = maxval > ENUM_LOOKUP_బిట్లు;
        Ast_కణుపు_సమాసం* testp = శూన్యసూచిక;
        File_పంక్తి* స్థిరరాశి fl_novalue = కొత్త File_పంక్తి{valp->fileline()};
        fl_novalue->warnOff(V3_Error_సంకేతం::ENUMVALUE, నిజం);
        fl_novalue->warnOff(V3_Error_సంకేతం::CMPCONST, నిజం);
        ఒకవేళ (assoc) {
            AstVarRef* స్థిరరాశి tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, నిజం, 0);
            testp = కొత్త AstAssocSel{fl_novalue, tabRefp, valp->clone_వృక్షం_శుద్ధ(అబద్ధం)};
        } లేకపోతే {
            స్థిరరాశి పూర్ణాంకం selwidth = V3_సంఖ్య::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* స్థిరరాశి tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, అబద్ధం, (1ULL << selwidth) - 1);
            File_పంక్తి* స్థిరరాశి fl_nowidth = కొత్త File_పంక్తి{fl_novalue};
            fl_nowidth->warnOff(V3_Error_సంకేతం::వెడల్పు_2, నిజం);
            testp = కొత్త AstCond{
                fl_novalue,
                కొత్త AstGt{fl_nowidth, valp->clone_వృక్షం_శుద్ధ(అబద్ధం),
                          కొత్త AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                కొత్త AstConst{fl_novalue, AstConst::బిట్టు_False{}},
                కొత్త AstArraySel{fl_novalue, tabRefp,
                                కొత్త AstSel{fl_novalue, valp->clone_వృక్షం_శుద్ధ(అబద్ధం), 0, selwidth}}};
        }
        తిరిగి testp;
    }

    PatVecMap pat_సదిశ_Map(Ast_నమూనా* nodep, స్థిరరాశి VNumRange& range) {
        PatVecMap patmap;
        పూర్ణాంకం element = range.ఎడమ();
        కొరకు (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ఒకవేళ (patp->keyp()) {
                ఒకవేళ (స్థిరరాశి AstConst* స్థిరరాశి constp = నమూనా_Const_Keyp(patp)) {
                    ఒకవేళ (constp->అంకె().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } లేకపోతే {
                        element = constp->గమ్యం_SInt();
                    }
                }
            }
            స్థిరరాశి తార్కిక new_నమోదు = patmap.emplace(element, patp).రెండవ;
            ఒకవేళ (!new_నమోదు) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.ఎడమ_గమ్యం_కుడి_Inc();
        }
        తిరిగి patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    శూన్య patConcatConvertRecurse(Ast_నమూనా* patternp, Ast_కణుపు_సమాసం* nodep) {
        ఒకవేళ (AstConcat* స్థిరరాశి concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } లేకపోతే {
            AstPatMember* స్థిరరాశి newp
                = కొత్త AstPatMember{nodep->fileline(), nodep->unlink_Fr_వెనుక(), శూన్యసూచిక, శూన్యసూచిక};
            newp->isConcat(నిజం);
            patternp->కూడు_Itemsp(newp);
        }
    }

    శూన్య makeOpenArrayShell(Ast_కణుపు_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_కణుపు_FTask* స్థిరరాశి oldTaskp = nodep->taskp();
        oldTaskp->dpi_Open_మాతృ_Inc();
        UASSERT_OBJ(!oldTaskp->dpi_Open_బిడ్డ(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_కణుపు_FTask* స్థిరరాశి newTaskp = oldTaskp->clone_వృక్షం(అబద్ధం);
        newTaskp->dpi_Open_బిడ్డ(నిజం);
        newTaskp->dpi_Open_మాతృ_Clear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvt_గమ్యం_Str(oldTaskp->dpi_Open_మాతృ()));
        oldTaskp->కూడు_తదుపరి_ఇక్కడ(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        స్థిరరాశి V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        కొరకు (స్థిరరాశి స్వయంచాలక& tconnect : tconnects) {
            AstVar* స్థిరరాశి portp = tconnect.మొదటి;
            స్థిరరాశి AstArg* స్థిరరాశి argp = tconnect.రెండవ;
            స్థిరరాశి Ast_కణుపు* స్థిరరాశి pinp = argp->exprp();
            ఒకవేళ (!pinp) కొనసాగు;  // Argument error we'll find later
            ఒకవేళ (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_కణుపు_DType* స్థిరరాశి oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_కణుపు* nodep) {
                    ఒకవేళ (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    తార్కిక markHasOpenArray(Ast_కణుపు_FTask* nodep) {
        తార్కిక hasOpen = అబద్ధం;
        కొరకు (Ast_కణుపు* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (AstVar* స్థిరరాశి portp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(నిజం);
                    hasOpen = నిజం;
                }
            }
        }
        తిరిగి hasOpen;
    }
    తార్కిక hasOpenArrayDTypeRecurse(Ast_కణుపు_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        ఒకవేళ (VN_IS(nodep, UnsizedArrayDType)) తిరిగి నిజం;
        ఒకవేళ (nodep->subDTypep()) తిరిగి hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        తిరిగి అబద్ధం;
    }
    Ast_కణుపు_DType* dtypeNotIntAtomOrVecRecurse(Ast_కణుపు_DType* nodep, తార్కిక ranged = అబద్ధం) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skip_Ref_గమ్యం_Enump();
        ఒకవేళ (AstBasicDType* స్థిరరాశి dtp = VN_CAST(nodep, BasicDType)) {
            ఒకవేళ (ranged && (!dtp->is_బిట్టు_Logic() || dtp->isRanged()))
                తిరిగి dtp;  // Packed when already packed
            ఒకవేళ (dtp->keyword().isIntNumeric()) తిరిగి శూన్యసూచిక;
            తిరిగి dtp;
        } లేకపోతే ఒకవేళ (AstPackArrayDType* స్థిరరాశి dtp = VN_CAST(nodep, PackArrayDType)) {
            ఒకవేళ (ranged) తిరిగి dtp;  // Packed when already packed
            తిరిగి dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), నిజం);
        }
        తిరిగి nodep;
    }
    Ast_కణుపు_DType* dtypeNot4StateIntegralRecurse(Ast_కణుపు_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        ఒకవేళ (AstBasicDType* స్థిరరాశి dtp = VN_CAST(nodep, BasicDType)) {
            ఒకవేళ (!dtp->keyword().isFourstate()) తిరిగి dtp;
            తిరిగి శూన్యసూచిక;
        } లేకపోతే ఒకవేళ (Ast_కణుపు_Array_DType* స్థిరరాశి dtp = VN_CAST(nodep, కణుపు_Array_DType)) {
            తిరిగి dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } లేకపోతే ఒకవేళ (Ast_కణుపు_UOr_Struct_DType* స్థిరరాశి dtp = VN_CAST(nodep, కణుపు_UOr_Struct_DType)) {
            కొరకు (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_కణుపు_DType* స్థిరరాశి badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                ఒకవేళ (badDtp) తిరిగి badDtp;
            }
            తిరిగి శూన్యసూచిక;
        }
        తిరిగి nodep;
    }
    శూన్య checkForceReleaseLhs(Ast_కణుపు* nodep, Ast_కణుపు* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి selNodep = V3_వెడల్పు::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    శూన్య assert_At_సమాసం(Ast_కణుపు* nodep) {
        ఒకవేళ (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    శూన్య assertAtStatement(Ast_కణుపు* nodep) {
        ఒకవేళ (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    శూన్య checkConstantOrReplace(Ast_కణుపు* nodep, తార్కిక noFourState, స్థిరరాశి string& సందేశం) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* స్థిరరాశి constp = VN_CAST(nodep, Const);
        ఒకవేళ (!constp || (noFourState && constp->అంకె().isFourState())) {
            nodep->v3error(సందేశం);
            nodep->replaceWith(కొత్త AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(నెట్టు_Deletep(nodep), nodep, constp);
            తిరిగి;
        }
    }
    Ast_కణుపు* కణుపు_For_Unsized_Warning(Ast_కణుపు* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        ఒకవేళ (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->వెడల్పు_Sized()) {
            తిరిగి nodep->op1p();
        } లేకపోతే ఒకవేళ (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->వెడల్పు_Sized()) {
            తిరిగి nodep->op2p();
        }
        తిరిగి nodep;  // By default return this
    }
    AstRefDType* check_Ref_గమ్యం_Typedef_Recurse(Ast_కణుపు* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        ఒకవేళ (VL_LIKELY(!nodep)) తిరిగి శూన్యసూచిక;
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి refp = VN_CAST(nodep, RefDType)) {
            ఒకవేళ (refp->typedefp() == typedefp) తిరిగి refp;
        }
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి refp = check_Ref_గమ్యం_Typedef_Recurse(nodep->op1p(), typedefp)) తిరిగి refp;
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి refp = check_Ref_గమ్యం_Typedef_Recurse(nodep->op2p(), typedefp)) తిరిగి refp;
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి refp = check_Ref_గమ్యం_Typedef_Recurse(nodep->op3p(), typedefp)) తిరిగి refp;
        ఒకవేళ (స్వయంచాలక* స్థిరరాశి refp = check_Ref_గమ్యం_Typedef_Recurse(nodep->op4p(), typedefp)) తిరిగి refp;
        తిరిగి శూన్యసూచిక;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    స్థిర తార్కిక isIfaceArrayDType(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        ఒకవేళ (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) తిరిగి అబద్ధం;
        స్థిరరాశి AstIfaceRefDType* స్థిరరాశి irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        తిరిగి irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    స్థిర std::string unpacked_Array_కొలత(స్థిరరాశి Ast_కణుపు_DType* dtypep) {
        std::string కొలత;
        dtypep = dtypep->skipRefp();
        అయితే (స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            ఒకవేళ (!కొలత.ఖాళీలు()) కొలత += "x";
            కొలత += std::గమ్యం_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        తిరిగి కొలత;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    స్థిర Ast_కణుపు_సమాసం* newIfaceArrayElement(Ast_కణుపు_సమాసం* fromp, పూర్ణాంకం బిట్టు_2) {
        AstConsArray* స్థిరరాశి consp = VN_CAST(fromp, ConsArray);
        ఒకవేళ (!consp || !isIfaceArrayDType(consp->dtypep())) తిరిగి శూన్యసూచిక;
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        ఒకవేళ (బిట్టు_2 < 0 || బిట్టు_2 >= arrp->elementsConst()) తిరిగి శూన్యసూచిక;
        తిరిగి consp->valuep(బిట్టు_2)->clone_వృక్షం(అబద్ధం);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_కణుపు_సమాసం* new_Iface_Array_విలువ(స్థిరరాశి Ast_కణుపు_Var_Ref* refp, Ast_కణుపు_DType* dtypep,
                                    స్థిరరాశి std::string& name, స్థిరరాశి std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_పంక్తి* స్థిరరాశి flp = refp->fileline();
        AstUnpackArrayDType* స్థిరరాశి arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        ఒకవేళ (!arrp) {
            AstVar* స్థిరరాశి varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_కణుపు_Var_Ref* newp;
            ఒకవేళ (స్థిరరాశి AstVarXRef* స్థిరరాశి xrefp = VN_CAST(refp, VarXRef)) {
                newp = కొత్త AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } లేకపోతే {
                newp = కొత్త AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, వెడల్పు_VP{SELF, BOTH}.p());
            తిరిగి newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* స్థిరరాశి consp = కొత్త AstConsArray{flp, arrp};
        స్థిరరాశి VNumRange range = arrp->declRange();
        కొరకు (స్థిరరాశి పూర్ణాంకం i : range.seq_ఎడమ_గమ్యం_కుడి()) {
            స్థిరరాశి std::string s = name + "__BRA__" + Ast_కణుపు::encode_సంఖ్య(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_విలువ(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        తిరిగి consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    శూన్య replace_విభజించు_Iface_Array_Ref(Ast_కణుపు_Var_Ref* nodep) {
        AstVar* స్థిరరాశి varp = nodep->varp();
        UASSERT_OBJ(varp->is_Iface_Array_విభజించు(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        ఒకవేళ (varp->is_Iface_మాతృ()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_విలువ(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(నెట్టు_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    శూన్య expandIfaceArrayPinDimensions(AstPin* nodep, స్థిరరాశి Ast_కణుపు_DType* dtypep,
                                       Ast_కణుపు_సమాసం* conp, స్థిరరాశి std::string& suffix,
                                       AstVar*& elemVarpr) {
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        ఒకవేళ (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* స్థిరరాశి newp = nodep->clone_వృక్షం(అబద్ధం);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            ఒకవేళ (conp) newp->exprp(conp->clone_వృక్షం(అబద్ధం));
            nodep->కూడు_తదుపరి_ఇక్కడ(newp);
            తిరిగి;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        స్థిరరాశి VNumRange range = arrp->declRange();
        కొరకు (పూర్ణాంకం n = range.elements() - 1; n >= 0; --n) {
            స్థిరరాశి పూర్ణాంకం i = range.ఎడమ() + n * range.ఎడమ_గమ్యం_కుడి_Inc();
            స్థిరరాశి std::string s = suffix + "__BRA__" + Ast_కణుపు::encode_సంఖ్య(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_కణుపు_సమాసం* స్థిరరాశి elemConnp = [&]() -> Ast_కణుపు_సమాసం* {
                ఒకవేళ (!conp) తిరిగి శూన్యసూచిక;
                స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                స్థిరరాశి పూర్ణాంకం elements = conArrp->elementsConst();
                // Counted from lo
                Ast_కణుపు_సమాసం* స్థిరరాశి resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                తిరిగి resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    శూన్య expandIfaceArrayPin(AstPin* nodep) {
        AstVar* స్థిరరాశి portp = nodep->modVarp();
        UASSERT_OBJ(portp->is_Iface_Array_విభజించు(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        కొరకు (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_కణుపు_సమాసం* స్థిరరాశి exprp = VN_AS(nodep->exprp(), కణుపు_సమాసం);
        ఒకవేళ (exprp) నెట్టు_Deletep(exprp->unlink_Fr_వెనుక());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        స్థిరరాశి తార్కిక connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_కొలత(exprp->dtypep()) == unpacked_Array_కొలత(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : శూన్యసూచిక, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(నెట్టు_Deletep(nodep->unlink_Fr_వెనుక()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_కణుపు* userIterateSubtreeReturnEdits(Ast_కణుపు* nodep, వెడల్పు_VP* vup) {
        ఒకవేళ (!nodep) తిరిగి శూన్యసూచిక;
        Ast_కణుపు* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        తిరిగి ret;
    }
    శూన్య userIterate(Ast_కణుపు* nodep, వెడల్పు_VP* vup) {
        ఒకవేళ (!nodep) తిరిగి;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    శూన్య user_Iterate_And_తదుపరి(Ast_కణుపు* nodep, వెడల్పు_VP* vup) {
        ఒకవేళ (!nodep) తిరిగి;
        ఒకవేళ (nodep->did_వెడల్పు()) తిరిగి;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_తదుపరి_Null(nodep);
    }
    శూన్య userIterateChildren(Ast_కణుపు* nodep, వెడల్పు_VP* vup) {
        ఒకవేళ (!nodep) తిరిగి;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    శూన్య userIterateChildrenBackwardsConst(Ast_కణుపు* nodep, వెడల్పు_VP* vup) {
        ఒకవేళ (!nodep) తిరిగి;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

బహిరంగ:
    // CONSTRUCTORS
    వెడల్పు_Visitor(తార్కిక paramsOnly,  // [in] TRUE if we are considering parameters only.
                 తార్కిక doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_తాత్కాలిక_పేర్లు{"__Venum"}
        , m_inside_తాత్కాలిక_పేర్లు{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_కణుపు* mainAcceptEdit(Ast_కణుపు* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, వెడల్పు_VP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        కొరకు (AstVar* స్థిరరాశి varp : m_విభజించు_Iface_Array_Varps) {
            VL_DO_DANGLING(నెట్టు_Deletep(varp->unlink_Fr_వెనుక()), varp);
        }
        m_విభజించు_Iface_Array_Varps.clear();
        తిరిగి nodep;
    }
    స్థిర తార్కిక క్రింది_As_Fixed_Aggregate(స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి dtypep) {
        తిరిగి dtypep->isStreamableFixedAggregate() && dtypep->కలిగియుంది_Unpacked_Struct();
    }
    శూన్య ప్రవాహం_కేటాయించు_క్రింది(Ast_కణుపు_కేటాయించు* nodep) {
        Ast_కణుపు_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        ఒకవేళ (Ast_కణుపు_ప్రవాహం* స్థిరరాశి streamp = VN_CAST(nodep->rhsp(), కణుపు_ప్రవాహం)) {
            Ast_కణుపు_DType* స్థిరరాశి lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            స్థిరరాశి పూర్ణాంకం lwidth = lhsDTypeSkippedRefp->వెడల్పు_ప్రవాహం();
            స్థిరరాశి పూర్ణాంకం rwidth = streamp->lhsp()->dtypep()->skipRefp()->వెడల్పు_ప్రవాహం();
            ఒకవేళ (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            ఒకవేళ (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), వరుస_DType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), వరుస_DType)) {
                స్థిరరాశి పూర్ణాంకం వరుస_Element_కొలత = streamp->lhsp()->dtypep()->subDTypep()->వెడల్పు();
                UASSERT_OBJ(వరుస_Element_కొలత <= lwidth, nodep, "LHS < RHS");
            }
            ఒకవేళ (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || క్రింది_As_Fixed_Aggregate(lhsDTypeSkippedRefp)) {
                streamp->unlink_Fr_వెనుక();
                nodep->rhsp(
                    కొత్త Ast_Cvt_Packed_గమ్యం_Array{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        ఒకవేళ (Ast_కణుపు_ప్రవాహం* స్థిరరాశి streamp = VN_CAST(nodep->lhsp(), కణుపు_ప్రవాహం)) {
            స్థిరరాశి Ast_కణుపు_DType* స్థిరరాశి rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_కణుపు_DType* స్థిరరాశి lhs_ప్రవాహం_DTypep = streamp->lhsp()->dtypep()->skipRefp();
            స్థిరరాశి పూర్ణాంకం lwidth = lhs_ప్రవాహం_DTypep->వెడల్పు_ప్రవాహం();
            స్థిరరాశి పూర్ణాంకం rwidth = rhsDTypep->వెడల్పు_ప్రవాహం();
            ఒకవేళ (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            ఒకవేళ (క్రింది_As_Fixed_Aggregate(lhs_ప్రవాహం_DTypep)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి ప్రవాహం_Exprp = nodep->lhsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* స్థిరరాశి dstp = streamp->lhsp()->unlink_Fr_వెనుక();
                Ast_కణుపు_సమాసం* srcp = nodep->rhsp()->unlink_Fr_వెనుక();
                ఒకవేళ (VN_IS(streamp, ప్రవాహం_L)) {
                    streamp->lhsp(srcp);
                    streamp->dtype_సమితి_బిట్టు_Or_Logic_Unsized(
                        srcp->వెడల్పు(), srcp->వెడల్పు_Min(), VSigning::UNSIGNED,
                        Ast_కణుపు_సమాసం::isFourstateOrNoDtype(ప్రవాహం_Exprp, srcp));
                    srcp = ప్రవాహం_Exprp;
                } లేకపోతే {
                    ఒకవేళ (srcp->వెడల్పు() > lwidth) {
                        srcp = కొత్త AstSel{streamp->fileline(), srcp, srcp->వెడల్పు() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(నెట్టు_Deletep(ప్రవాహం_Exprp), ప్రవాహం_Exprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(కొత్త Ast_Cvt_Packed_గమ్యం_Array{srcp->fileline(), srcp, lhs_ప్రవాహం_DTypep});
                nodep->dtypeFrom(dstp);
            } లేకపోతే ఒకవేళ (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = nodep->rhsp()->unlink_Fr_వెనుక();
                nodep->rhsp(కొత్త Ast_Cvt_Array_గమ్యం_Packed{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~వెడల్పు_Visitor() override = అప్రమేయ;
};

//######################################################################
// Width class functions

శూన్య V3_వెడల్పు::వెడల్పు(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        స్థిరరాశి వెడల్పు_Clear_Visitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        స్థిరరాశి VDTypeNameScopes scopes{nodep};
        వెడల్పు_Visitor visitor{అబద్ధం, అబద్ధం};
        (శూన్య)visitor.mainAcceptEdit(nodep);
        వెడల్పు_Remove_Visitor rvisitor;
        (శూన్య)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("width", 0, dump_వృక్షం_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_కణుపు* V3_వెడల్పు::వెడల్పు_Params_Edit(Ast_కణుపు* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    వెడల్పు_Visitor visitor{నిజం, అబద్ధం};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    తిరిగి nodep;
}

శూన్య V3_వెడల్పు::ప్రవాహం_కేటాయించు_క్రింది_Edit(Ast_కణుపు_కేటాయించు* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    వెడల్పు_Visitor visitor{అబద్ధం, అబద్ధం};
    visitor.ప్రవాహం_కేటాయించు_క్రింది(nodep);
}

//! Single node parameter propagation for generate blocks.
//! Smaller step... Only do a single node for parameter propagation
//! If we are inside a generated "if", "case" or "for", we don't want to
//! trigger warnings when we deal with the width. It is possible that
//! these are spurious, existing within sub-expressions that will not
//! actually be generated. Since such occurrences, must be constant, in
//! order to be something a generate block can depend on, we can wait until
//! later to do the width check.
//! @return  Pointer to the edited node.
Ast_కణుపు* V3_వెడల్పు::వెడల్పు_Generate_Params_Edit(
    Ast_కణుపు* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    వెడల్పు_Visitor visitor{నిజం, నిజం};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    తిరిగి nodep;
}

Ast_కణుపు* V3_వెడల్పు::selectNonConstantRecurse(Ast_కణుపు* nodep, తార్కిక లోపల_Sel) {
    // If node has a non-constant select, return that select
    Ast_కణుపు* resultp = శూన్యసూచిక;
    ఒకవేళ (Ast_కణుపు_Sel* స్థిరరాశి anodep = VN_CAST(nodep, కణుపు_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), లోపల_Sel);
        ఒకవేళ (resultp) తిరిగి resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), నిజం);
    } లేకపోతే ఒకవేళ (AstSel* స్థిరరాశి anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), లోపల_Sel);
        ఒకవేళ (resultp) తిరిగి resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), నిజం);
    } లేకపోతే ఒకవేళ (Ast_కణుపు_Var_Ref* స్థిరరాశి anodep = VN_CAST(nodep, కణుపు_Var_Ref)) {
        ఒకవేళ (లోపల_Sel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) తిరిగి anodep;
    } లేకపోతే {
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, లోపల_Sel);
        ఒకవేళ (resultp) తిరిగి resultp;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, లోపల_Sel);
        ఒకవేళ (resultp) తిరిగి resultp;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, లోపల_Sel);
        ఒకవేళ (resultp) తిరిగి resultp;
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, లోపల_Sel);
    }
    తిరిగి resultp;
}
