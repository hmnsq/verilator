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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Width.h"

#अन्तर्भाव "V3Assert.h"
#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Begin.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3LinkLValue.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3Number.h"
#अन्तर्भाव "V3Randomize.h"
#अन्तर्भाव "V3String.h"
#अन्तर्भाव "V3Task.h"
#अन्तर्भाव "V3UniqueNames.h"
#अन्तर्भाव "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#अन्तर्भाव "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<स्थिरांक Ast_गाठ*, AstClass*> m_cache;

    AstClass* findCached(Ast_गाठ* nodep) {
        जर (!nodep) परत nullptr;
        स्थिरांक स्वतः it = m_cache.find(nodep);
        जर (it != m_cache.end()) परत it->second;

        AstClass* classp = nullptr;
        जर (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } अन्यथा जर (AstClass* स्थिरांक parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } अन्यथा जर (AstClassPackage* स्थिरांक packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } अन्यथा {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        परत classp;
    }

सार्वजनिक:
    AstClass* find(Ast_गाठ* nodep) {
        जर (AstClass* स्थिरांक classp = VN_CAST(nodep, Class)) परत classp;
        जर (AstClassPackage* स्थिरांक packagep = VN_CAST(nodep, ClassPackage)) {
            परत packagep->classp();
        }
        परत findCached(nodep);
    }
};

//######################################################################

गणना Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& चालक<<(std::ostream& str, स्थिरांक Stage& rhs) {
    परत str << ("-PFB"[static_cast<पूर्णांक>(rhs)]);
}

गणना Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& चालक<<(std::ostream& str, स्थिरांक Determ& rhs) {
    स्थिर constexpr स्थिरांक अक्षर* स्थिरांक s_det[] = {"SELF", "CNTX", "ASSN"};
    परत str << s_det[rhs];
}

गणना StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, संदेश) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               संदेश)

//######################################################################
// Width state, as a visitor of each AstNode

वर्ग WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_गाठ_DType* स्थिरांक m_dtypep;  // Parent's data type to resolve to
    स्थिरांक Stage m_stage;  // If true, report errors
    स्थिरांक StreamUse m_streamUse;  // Current expression may be a stream concat

सार्वजनिक:
    WidthVP(Ast_गाठ_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_streamUse{streamUse} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    WidthVP(Determ determ, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
        : m_dtypep{nullptr}
        , m_stage{stage}
        , m_streamUse{streamUse} {
        जर (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { परत हे; }
    तार्किक selfDtm() स्थिरांक { परत m_dtypep == nullptr; }
    StreamUse streamUse() स्थिरांक { परत m_streamUse; }
    Ast_गाठ_DType* dtypep() स्थिरांक {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        परत m_dtypep;
    }
    Ast_गाठ_DType* dtypeNullp() स्थिरांक { परत m_dtypep; }
    Ast_गाठ_DType* dtypeNullSkipRefp() स्थिरांक {
        Ast_गाठ_DType* dtp = dtypeNullp();
        जर (dtp) dtp = dtp->skipRefp();
        परत dtp;
    }
    Ast_गाठ_DType* dtypeOverridep(Ast_गाठ_DType* स्थिरांक defaultp) स्थिरांक {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        जर (!m_dtypep) परत defaultp;
        जर (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            परत defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        परत m_dtypep;
    }
    पूर्णांक width() स्थिरांक {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        परत m_dtypep->width();
    }
    पूर्णांक widthMin() स्थिरांक {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        परत m_dtypep->widthMin();
    }
    तार्किक prelim() स्थिरांक { परत m_stage & PRELIM; }
    तार्किक final() स्थिरांक { परत m_stage & FINAL; }
    शून्य dump(std::ostream& str) स्थिरांक {
        जर (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } अन्यथा {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& चालक<<(std::ostream& str, स्थिरांक WidthVP* vup) {
    जर (vup) vup->dump(str);
    परत str;
}

//######################################################################

वर्ग WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    शून्य clearWidthRecurse(Ast_गाठ* nodep) {
        साठी (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(असत्य);
            जर (Ast_गाठ* स्थिरांक refp = nodep->op1p()) clearWidthRecurse(refp);
            जर (Ast_गाठ* स्थिरांक refp = nodep->op2p()) clearWidthRecurse(refp);
            जर (Ast_गाठ* स्थिरांक refp = nodep->op3p()) clearWidthRecurse(refp);
            जर (Ast_गाठ* स्थिरांक refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    आभासी ~WidthClearVisitor() = पूर्वनिर्धारित;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_गाठ_iterate_instead_of_Ast_गाठ_accept

//######################################################################

वर्ग WidthVisitor final : सार्वजनिक VNVisitor {
    // TYPES
    using PatVecMap = std::map<पूर्णांक, AstPatMember*>;
    using DTypeMap = std::map<स्थिरांक std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<स्थिरांक AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    तार्किक m_underFork = असत्य;  // Visiting under a fork
    तार्किक m_underSExpr = असत्य;  // Visiting under a sequence expression
    तार्किक m_underPackedArray = असत्य;  // Visiting under a AstPackArrayDType
    तार्किक m_underMemberSel = असत्य;  // Viting under a MemberSel
    Ast_गाठ* m_seqUnsupp = nullptr;  // Property has unsupported node
    तार्किक m_hasSExpr = असत्य;  // Property has a sequence expression
    स्थिरांक AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    स्थिरांक AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_गाठ_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_गाठ_Module* m_modep = nullptr;  // Current module
    स्थिरांक AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_गाठ_Procedure* m_procedurep = nullptr;  // Current final/always
    स्थिरांक AstWith* m_withp = nullptr;  // Current 'with' statement
    स्थिरांक AstFunc* m_funcp = nullptr;  // Current function
    स्थिरांक AstAttrOf* m_attrp = nullptr;  // Current attribute
    स्थिरांक Ast_गाठ_अभिव्यक्ती* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    स्थिरांक तार्किक m_paramsOnly;  // Computing parameter value; limit operation
    स्थिरांक तार्किक m_doGenerate;  // Do errors later inside generate statement
    तार्किक m_streamConcat = असत्य;  // True if visiting arguments of stream concatenation
    तार्किक m_inParamOverride = असत्य;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<स्थिरांक Ast_गाठ_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<स्थिरांक Ast_गाठ_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<स्थिरांक AstVar*> m_curModVars;  // Variables declared in current module

    स्थिर constexpr पूर्णांक ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    गणना ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    गणना वर्ग ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    स्थिर StreamUse streamUseForDeterm(Determ determ) {
        परत determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() स्थिरांक { परत m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    स्थिर शून्य packIfUnpacked(Ast_गाठ_अभिव्यक्ती* स्थिरांक nodep) {
        जर (AstUnpackArrayDType* स्थिरांक unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            स्थिरांक पूर्णांक elements_अंक = unpackDTypep->arrayUnpackedElements();
            स्थिरांक पूर्णांक unpackMinBits = elements_अंक * unpackDTypep->subDTypep()->widthMin();
            स्थिरांक पूर्णांक unpackBits = elements_अंक * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(नवीन AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    स्थिर Ast_गाठ_DType* fromDTypep(Ast_गाठ* fromp) {
        जर (Ast_गाठ_DType* स्थिरांक dtypep = VN_CAST(fromp, गाठ_DType))
            परत dtypep->skipRefOrNullp();
        परत fromp ? fromp->dtypep() : nullptr;
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
    शून्य visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    शून्य visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    शून्य visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    शून्य visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    शून्य visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    शून्य visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    शून्य visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    शून्य visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    शून्य visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    शून्य visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    शून्य visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    शून्य visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    शून्य visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    शून्य visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    शून्य visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    शून्य visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    शून्य visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    शून्य visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    शून्य visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    शून्य visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    शून्य visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    शून्य visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    शून्य visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    शून्य visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    शून्य visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    शून्य visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    शून्य visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    शून्य visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    शून्य visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    शून्य visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    शून्य visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    शून्य visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    शून्य visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    शून्य visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    शून्य visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    शून्य visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    शून्य visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    शून्य visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    शून्य visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    शून्य visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    शून्य visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    शून्य visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    शून्य visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    शून्य visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    शून्य visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    शून्य visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    शून्य visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    शून्य visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, सत्य, असत्य); }
    शून्य visit(AstSub* nodep) override { visit_add_sub_replace(nodep, सत्य, असत्य); }
    शून्य visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, सत्य, सत्य); }
    शून्य visit(AstMul* nodep) override { visit_add_sub_replace(nodep, सत्य, असत्य); }
    // These can't promote to real
    शून्य visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, असत्य, सत्य); }
    शून्य visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, असत्य, सत्य); }
    शून्य visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, असत्य, असत्य); }
    शून्य visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, असत्य, सत्य); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    शून्य visit(AstNegate* nodep) override { visit_negate_not(nodep, सत्य); }
    // Unary never real
    शून्य visit(AstNot* nodep) override { visit_negate_not(nodep, असत्य); }

    // Real: inputs and output real
    शून्य visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    शून्य visit(Ast_गाठ_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    शून्य visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    शून्य visit(Ast_गाठ_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    शून्य visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    शून्य visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    शून्य visit(AstShiftL* nodep) override { visit_shift(nodep); }
    शून्य visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    शून्य visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    शून्य visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    शून्य visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    शून्य visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    शून्य visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    शून्य visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        जर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    शून्य visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        जर (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    शून्य visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        जर (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    शून्य visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        जर (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    शून्य visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        जर (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    शून्य visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    शून्य visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            जर (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } अन्यथा {
                nodep->dtypeSetInteger();
            }
        }
    }
    शून्य visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    शून्य visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    शून्य visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    शून्य visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    शून्य visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    शून्य visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            नवीन AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    स्थिर तार्किक isOverridePattern(स्थिरांक Ast_गाठ* nodep) {
        जर (स्थिरांक AstPattern* स्थिरांक patternp = VN_CAST(nodep, Pattern)) {
            परत !patternp->childDTypep();
        }
        स्थिरांक AstCond* स्थिरांक condp = VN_CAST(nodep, Cond);
        परत condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    शून्य visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_अभिव्यक्ती(nodep);
        जर (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            स्थिरांक पूर्णांक errors = V3Error::error_मोजणी();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = असत्य;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->condp(), Const);
            स्थिरांक तार्किक known = constp && !constp->अंक().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            जर (!known && V3Error::error_मोजणी() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_गाठ_अभिव्यक्ती* स्थिरांक keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            परत;
        }
        जर (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            userIterateAndNext(nodep->thenp(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            userIterateAndNext(nodep->elsep(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_गाठ_DType* स्थिरांक thenDTypep = nodep->thenp()->dtypep();
            Ast_गाठ_DType* स्थिरांक elseDTypep = nodep->elsep()->dtypep();
            Ast_गाठ_DType* स्थिरांक thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_गाठ_DType* स्थिरांक elseDTypeSkippedp = elseDTypep->skipRefp();
            जर (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } अन्यथा जर (nodep->thenp()->is_Class_Handle_मूल्य()
                       || nodep->elsep()->is_Class_Handle_मूल्य()) {
                Ast_गाठ_DType* commonClassTypep = nullptr;
                जर (nodep->thenp()->is_Class_Handle_मूल्य() && nodep->elsep()->is_Class_Handle_मूल्य()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_गाठ::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                जर (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } अन्यथा {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } अन्यथा जर (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                जर (Ast_गाठ::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } अन्यथा {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    जर (!Ast_गाठ::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } अन्यथा जर (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } अन्यथा जर (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } अन्यथा {
                स्थिरांक तार्किक isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                तार्किक handled = असत्य;
                स्थिरांक तार्किक isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_गाठ_DType* स्थिरांक thenEnumDTypep = thenDTypep->skipRefToEnump();
                जर (VN_IS(thenDTypeSkippedp, गाठ_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_वृक्ष(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = सत्य;
                } अन्यथा जर (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_वृक्ष(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    जर (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = सत्य;
                    } अन्यथा {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(नवीन File_ओळ{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, सत्य);
                    }
                }
                जर (!handled) {
                    स्थिरांक पूर्णांक width = std::max(thenDTypep->width(), elseDTypep->width());
                    स्थिरांक पूर्णांक widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    स्थिरांक VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    स्थिरांक तार्किक isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    जर ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } अन्यथा जर ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } अन्यथा {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        जर (m_vup->final()) {
            Ast_गाठ_DType* स्थिरांक expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाठ_DType* स्थिरांक subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    शून्य visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_गाठ_DType* स्थिरांक vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        जर (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* स्थिरांक newp = नवीन AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            परत;
        }

        // Concat handling
        जर (m_vup->prelim()) {
            जर (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            जर (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } अन्यथा {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                जर (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_गाठ_अभिव्यक्ती::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            जर (स्थिरांक AstReplicate* स्थिरांक repp = VN_CAST(nodep->lhsp(), Replicate)) {
                जर (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
            }
            जर (स्थिरांक AstReplicate* स्थिरांक repp = VN_CAST(nodep->rhsp(), Replicate)) {
                जर (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
            }
        }
        जर (m_vup->final()) {
            जर (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_गाठ_अभिव्यक्ती* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* rhsp = nodep->rhsp()->unlinkFrBack();
                जर (!lhsp->isString()) lhsp = नवीन AstCvtPackString{lhsp->fileline(), lhsp};
                जर (!rhsp->isString()) rhsp = नवीन AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_गाठ* स्थिरांक newp = नवीन AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            }
            जर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    शून्य visit(AstConcatN* nodep) override {
        जर (nodep->didWidth()) परत;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        जर (m_vup->final()) {
            nodep->didWidth(सत्य);
            जर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    शून्य visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    शून्य visit(AstDelay* nodep) override {
        जर (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            जर (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            जर (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                परत;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            परत;
        }
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            परत;
        }
        जर (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        जर (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        जर (nodep->fileline()->timingOn()) {
            जर (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                परत;
            } अन्यथा जर (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } अन्यथा {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        जर (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    शून्य visit(AstFork* nodep) override {
        जर (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        जर (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* स्थिरांक newp = नवीन AstBegin{nodep->fileline(), nodep->name(), nullptr, असत्य};
            nodep->replaceWith(newp);
            जर (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            जर (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            जर (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा जर (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = सत्य;
            iterateChildren(nodep);
        } अन्यथा जर (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
        } अन्यथा {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    शून्य visitWaitOrDisableFork(Ast_गाठ* nodep) {
        जर (nodep->fileline()->timingOn()) {
            जर (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                परत;
            } अन्यथा जर (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    शून्य visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    शून्य visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    शून्य visit(AstToLowerN* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    शून्य visit(AstToUpperN* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    शून्य visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t बार = 1;  // IEEE replicate value is integral

            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->countp(), Const);
            जर (constp) {
                जर (constp->अंक().isFourState()
                    || (constp->dtypep()->isSigned() && constp->अंक().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } अन्यथा {
                    बार = constp->toSInt();
                }
            }

            Ast_गाठ_DType* स्थिरांक vdtypep = m_vup->dtypeNullSkipRefp();
            जर (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                जर (बार != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                जर (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                } अन्यथा {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* स्थिरांक newp = नवीन AstPattern{
                        nodep->fileline(),
                        नवीन AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    परत;
                }
            }
            जर (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            जर (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } अन्यथा {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            जर ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_गाठ* स्थिरांक newp
                    = नवीन AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            } अन्यथा {
                जर (!constp) nodep->v3error("Replication value isn't a constant.");
                जर (बार == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    बार = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    (nodep->srcp()->width() * बार), (nodep->srcp()->widthMin() * बार),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        जर (m_vup->final()) {
            जर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    शून्य visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        जर (m_vup->final()) {
            जर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    शून्य visit(Ast_गाठ_Dist_Biop* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    शून्य visit(Ast_गाठ_Dist_Triop* nodep) override {
        जर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    तार्किक streamImplicitUseAllowed(स्थिरांक Ast_गाठ_Stream* nodep) स्थिरांक {
        जर (m_streamConcat) परत सत्य;
        जर (m_vup && m_vup->streamUse() != STREAM_USE_NONE) परत सत्य;

        स्थिरांक Ast_गाठ* स्थिरांक backp = nodep->backp();
        जर (!backp) परत सत्य;  // Error elsewhere

        परत VN_IS(backp, गाठ_Assign);
    }
    शून्य visit(Ast_गाठ_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            m_streamConcat = सत्य;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = असत्य;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->rhsp(), Const)) {
                जर (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } अन्यथा {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            स्थिरांक Ast_गाठ_DType* स्थिरांक lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            जर (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, गाठ_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, गाठ_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } अन्यथा जर (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } अन्यथा {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_गाठ_अभिव्यक्ती::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        जर (m_vup->final()) {
            जर (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            जर (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                गाठ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    शून्य visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), सत्य,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), सत्य,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        जर (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            स्थिरांक पूर्णांक width = nodep->elementsConst();
            जर (nodep->fromBracket() && nodep->डावा_Const() > nodep->उजवा_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } अन्यथा जर (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            स्थिरांक तार्किक inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            स्थिरांक तार्किक inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            स्थिरांक तार्किक inTypeTable = !m_modep;
            स्थिरांक Ast_गाठ* basep = nodep->backp();
            पर्यंत (VN_IS(basep, Range)) basep = basep->backp();
            जर (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->डावा_Const() << ":" << nodep->उजवा_Const()
                                            << "]");
            }
        }
    }

    शून्य visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        जर (nodep->didWidth()) परत;
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), असत्य);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            पूर्णांक width = nodep->widthConst();
            जर (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                परत;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            स्थिरांक तार्किक isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            स्थिरांक तार्किक inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            जर (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                ढकला_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(नवीन AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            जर (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                जर (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_गाठ_DType* स्थिरांक subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    असत्य /*noerror*/);
                } अन्यथा {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << width << " bits to only "
                                                         << nodep->fromp()->width()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            पूर्णांक frommsb = nodep->fromp()->width() - 1;
            पूर्णांक fromlsb = 0;
            स्थिरांक पूर्णांक elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            जर (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } अन्यथा {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            स्थिरांक पूर्णांक selwidth = V3_संख्या::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_गाठ_DType* स्थिरांक selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            जर (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                जर (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->width() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->width() / elw)
                            << (nodep->lsbp()->width() != nodep->lsbp()->widthMin()
                                    ? " or " + cvtToStr(nodep->lsbp()->widthMin() / elw)
                                    : "")
                            << " bits.");
                    UINFO(1, "    Related node: " << nodep);
                }
            }
            जर (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                जर (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } अन्यथा जर (!inParameterizedTemplate) {
                    जर (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } अन्यथा {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                जर (!isWriteSelect) {
                    // Extend it
                    स्थिरांक पूर्णांक extendTo = nodep->msbConst() + 1;
                    Ast_गाठ_DType* स्थिरांक subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    असत्य /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            जर (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                असत्य /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    शून्य visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            जर (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->bitp(), Const)) {
                    स्थिरांक पूर्णांक bit = constp->toSInt();
                    Ast_गाठ_अभिव्यक्ती* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    जर (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        स्थिरांक AstUnpackArrayDType* स्थिरांक arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
            }
            //
            पूर्णांक frommsb;
            पूर्णांक fromlsb;
            स्थिरांक Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefp();
            जर (स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                जर (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                जर (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } अन्यथा {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            स्थिरांक पूर्णांक selwidth = V3_संख्या::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_गाठ_DType* स्थिरांक selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            जर (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                जर (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            जर (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                जर (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    जर (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                असत्य /*NOWARN*/);
            }
        }
    }

    शून्य visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            स्थिरांक Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefp();
            स्थिरांक AstAssocArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, AssocArrayDType);
            जर (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->किल्ली_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    शून्य visit(AstAlias* nodep) override {
        जर (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        स्थिरांक स्वतः check_If_अभिव्यक्ती_Ok = [हे](स्थिरांक Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp) {
            जर (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                परत असत्य;
            }

            स्थिरांक AstVarRef* स्थिरांक varRefp = VN_CAST(exprp, VarRef);
            जर (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                परत असत्य;
            }
            AstVar* स्थिरांक varp = varRefp->varp();
            जर (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                परत असत्य;
            }
            जर (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                परत असत्य;
            } अन्यथा {
                m_aliasedVars.घाला(varp);
            }
            परत सत्य;
        };

        check_If_अभिव्यक्ती_Ok(nodep->itemsp());
        Ast_गाठ_DType* स्थिरांक firstItemDtypep = nodep->itemsp()->dtypep();
        साठी (Ast_गाठ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_अभिव्यक्ती_Ok(VN_AS(itemp, गाठ_अभिव्यक्ती));
            जर (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    शून्य visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            स्थिरांक Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefp();
            स्थिरांक AstWildcardArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            जर (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            स्थिरांक AstBasicDType* स्थिरांक basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            जर (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    शून्य visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            स्थिरांक Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefp();
            स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            जर (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_गाठ_DType* स्थिरांक newDtp
                = नवीन AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          नवीन AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            जर (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                स्थिरांक पूर्णांक subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                स्थिरांक VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                जर ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(नवीन AstConsArray{
                        nodep->fileline(), newDtp,
                        नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                } अन्यथा जर ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            जर (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* स्थिरांक fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* स्थिरांक newp = नवीन AstConsArray{nodep->fileline(), newDtp};
                स्थिरांक VNumRange range = nodep->declRange();
                साठी (पूर्णांक n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_वृक्ष(असत्य));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        }
    }

    शून्य visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_गाठ* स्थिरांक selp = V3Width::widthSelNoIterEdit(nodep);
        जर (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            परत;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    शून्य visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_गाठ* स्थिरांक selp = V3Width::widthSelNoIterEdit(nodep);
        जर (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            परत;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    शून्य visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_गाठ* स्थिरांक selp = V3Width::widthSelNoIterEdit(nodep);
        जर (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            परत;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    शून्य visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_गाठ* स्थिरांक selp = V3Width::widthSelNoIterEdit(nodep);
        जर (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            परत;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    शून्य visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        जर (nodep->didWidthAndSet()) परत;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        जर (nodep->didWidthAndSet()) परत;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        जर (nodep->didWidthAndSet()) परत;
        जर (m_vup && m_vup->prelim()) {
            जर (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } अन्यथा जर (nodep->अंक().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } अन्यथा जर (nodep->अंक().isString()) {
                nodep->dtypeSetString();
            } अन्यथा जर (nodep->अंक().isDouble()) {
                nodep->dtypeChgWidth(nodep->अंक().width(), nodep->अंक().sized()
                                                               ? nodep->अंक().width()
                                                               : nodep->अंक().widthToFit());
            } अन्यथा {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->अंक().width(),
                    nodep->अंक().sized() ? nodep->अंक().width() : nodep->अंक().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->अंक().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    शून्य visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        जर (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    शून्य visit(AstFell* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstFalling* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstFuture* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    शून्य visit(AstPast* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtypeFrom(nodep->exprp());
            जर (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ सत्य);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->ticksp(), Const);
                जर (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_वृक्ष();
                } अन्यथा जर (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_वृक्ष();
                } अन्यथा जर (V3AssertCommon::check_Sva_मूल्य_Limit(nodep, constp, "$past tick value")) {
                    जर (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    शून्य visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            स्थिरांक AstConst* स्थिरांक minConstp = VN_CAST(nodep->countp(), Const);
            जर (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } अन्यथा जर (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } अन्यथा जर (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            जर (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                स्थिरांक AstConst* स्थिरांक maxConstp = VN_CAST(nodep->maxCountp(), Const);
                जर (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } अन्यथा जर (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } अन्यथा जर (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_गाठ_अभिव्यक्ती* propp = nodep->propp();
            जर (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            जर (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            स्थिरांक तार्किक loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            स्थिरांक तार्किक hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            जर (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_गाठ* स्थिरांक boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                परत;
            }
            जर (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                परत;
            }
            स्थिरांक AstConst* स्थिरांक loConstp = VN_CAST(nodep->loBoundp(), Const);
            स्थिरांक AstConst* स्थिरांक hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            जर (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            जर (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            जर (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } अन्यथा जर (loConstp) {
                V3AssertCommon::check_Sva_मूल्य_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            जर (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_मूल्य_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            जर (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            तार्किक hasPropertyOp = propp->isMultiCycleSva();
            जर (!hasPropertyOp) {
                propp->foreach([&](स्थिरांक Ast_गाठ* np) {
                    जर (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = सत्य;
                    }
                });
            }
            जर (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstRising* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstRose* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    शून्य visit(AstSampled* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    शून्य visit(AstSetuphold* nodep) override {
        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        जर (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        जर (nodep->deldatap()) {
            जर (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } अन्यथा {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        जर (!newp) {
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        nodep->replaceWith(नवीन AstAlways{newp});
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_ओळ* स्थिरांक flp, Ast_गाठ_अभिव्यक्ती* स्थिरांक evp,
                                         Ast_गाठ_अभिव्यक्ती* स्थिरांक delp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = delp->clone_वृक्ष_Pure(असत्य);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = evp->clone_वृक्ष_Pure(असत्य);
        UASSERT_OBJ(VN_IS(lhsp, गाठ_Var_Ref) || VN_IS(lhsp, गाठ_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        जर (Ast_गाठ_Var_Ref* varRefp = VN_CAST(lhsp, गाठ_Var_Ref)) {
            जर (varRefp->varp()->direction() == VDirection::आदान) {
                VL_DO_DANGLING(ढकला_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ढकला_Deletep(rhsp), rhsp);
                परत nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        जर (Ast_गाठ_Pre_Sel* selp = VN_CAST(lhsp, गाठ_Pre_Sel)) {
            जर (Ast_गाठ_Var_Ref* varRefp = VN_CAST(selp->fromp(), गाठ_Var_Ref)) {
                जर (varRefp->varp()->direction() == VDirection::आदान) {
                    VL_DO_DANGLING(ढकला_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ढकला_Deletep(rhsp), rhsp);
                    परत nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        परत नवीन AstAssignW{flp, lhsp, rhsp};
    }

    शून्य visit(AstStable* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSteady* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtypeSetBit();
        }
    }

    शून्य visit(Ast_Stmt_अभिव्यक्ती* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    शून्य visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    शून्य visit(AstUntil* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            जर (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            जर (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    शून्य visit(AstRand* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            जर (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } अन्यथा {
                nodep->dtypeSetInt();  // Says the spec
            }
            जर (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    शून्य visit(AstSEventually* nodep) override {
        जर (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::Widthed_मूल्य{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            परत;
        }
        जर (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSGotoRep* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            जर (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } अन्यथा {
                width_Check_Rep_मोजणी(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSNonConsRep* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            जर (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } अन्यथा {
                width_Check_Rep_मोजणी(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    ढाँचा <typename T_Rep>
    शून्य width_Check_Rep_मोजणी(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->countp(), Const);
        जर (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } अन्यथा जर (constp->dtypep()->isSigned() && constp->अंक().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } अन्यथा जर (constp->अंक().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } अन्यथा जर (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    ढाँचा <typename T_Rep>
    शून्य widthCheckGotoRepRange(T_Rep* nodep, स्थिरांक अक्षर* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        स्थिरांक AstConst* स्थिरांक minConstp = VN_CAST(nodep->countp(), Const);
        स्थिरांक AstConst* स्थिरांक maxConstp = VN_CAST(nodep->maxCountp(), Const);
        जर (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            परत;
        }
        जर (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            परत;
        }
        जर (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            परत;
        }
        जर (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    शून्य visit(AstSThroughout* nodep) override {
        m_hasSExpr = सत्य;
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = असत्य;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = सत्य;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = सत्य;
        m_hasSExpr = सत्य;
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            जर (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = सत्य;
        m_hasSExpr = सत्य;
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            साठी (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstURandomRange* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_गाठ_DType* स्थिरांक expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    शून्य visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        जर (VN_IS(nodep->backp(), IsUnbounded)) परत;  // Ok, leave
        जर (VN_IS(nodep->backp(), BracketArrayDType)) परत;  // Ok, leave
        जर (VN_IS(nodep->backp(), InsideRange)) परत;  // Ok, leave
        जर (स्थिरांक स्वतः* स्थिरांक varp = VN_CAST(nodep->backp(), Var)) {
            जर (varp->isParam()) परत;  // Ok, leave
        }
        Ast_गाठ* backp = nodep->backp();
        जर (VN_IS(backp, Sub)) backp = backp->backp();
        जर (स्थिरांक स्वतः* स्थिरांक selp = VN_CAST(backp, SelExtract)) {
            जर (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) परत;
        }
        जर (स्थिरांक स्वतः* स्थिरांक selp = VN_CAST(backp, SelBit)) {
            जर (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) परत;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    शून्य visit(AstInferredDisable* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) nodep->dtypeSetBit();
    }
    शून्य visit(AstIsUnbounded* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    शून्य visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        जर (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        जर (m_vup->final()) {
            Ast_गाठ_DType* स्थिरांक expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            जर (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    शून्य visit(AstCLog2* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    ढाँचा <typename T_Option>
    शून्य widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = nodep->valuep();
        स्थिरांक std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(valuep, Const)) {
            जर (!constp->अंक().isNegative()) परत;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } अन्यथा जर (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } अन्यथा {
            परत;
        }
        // Continue with the default weight
        valuep->replaceWith(नवीन AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    ढाँचा <typename T_Option>
    शून्य widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    शून्य visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* स्थिरांक cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        जर (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            परत;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        जर (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            परत;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        जर (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            जर (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } अन्यथा {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } अन्यथा जर (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = nodep->valuep();
            जर (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(नवीन AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
            }
            परत;
        } अन्यथा जर (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            परत;
        } अन्यथा जर (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->valuep(), Const);
            जर (!constp || !constp->अंक().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }
    शून्य visit(AstCoverOption* nodep) override {
        जर (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            परत;
        }
        जर (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            परत;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        जर (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    शून्य visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        जर (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    शून्य visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        जर (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    शून्य widthCovergroupRanges(Ast_गाठ* rangesp, पूर्णांक fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        स्थिरांक स्वतः fill = [&](Ast_गाठ* nodep) {
            जर (!fillWidth) परत;
            Ast_गाठ_अभिव्यक्ती* exprp = VN_AS(nodep, गाठ_अभिव्यक्ती);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        साठी (Ast_गाठ *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            जर (AstInsideRange* स्थिरांक rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } अन्यथा जर (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } अन्यथा {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    शून्य visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    शून्य visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        जर (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        जर (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    शून्य visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        जर (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        जर (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    शून्य visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक countp = nodep->countp()) {
            जर (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } अन्यथा {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    शून्य widthCoverFilter(Ast_गाठ_अभिव्यक्ती* filterp, स्थिरांक अक्षर* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), गाठ_अभिव्यक्ती);
        File_ओळ* स्थिरांक fl = filterp->fileline();
        स्थिरांक Ast_गाठ_DType* स्थिरांक resultp = filterp->dtypep()->skipRefp();
        जर (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                नवीन AstNeqD{fl, filterp, नवीन AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } अन्यथा जर (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(नवीन AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(filterp), filterp);
        } अन्यथा {
            fixWidthReduce(filterp);
        }
    }
    शून्य visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    शून्य visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    शून्य visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            जर (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                परत;
            }
            checkCvtUS(nodep->lhsp(), असत्य);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            जर (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->अंक().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        जर (m_vup->final()) {
            Ast_गाठ_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाठ_DType* स्थिरांक lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_गाठ* newp = nullptr;  // No change
            जर (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = नवीन AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } अन्यथा जर (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = नवीन AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } अन्यथा जर (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = नवीन AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            जर (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        }
    }
    शून्य visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(Ast_मोजणी_Bits* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            स्थिरांक पूर्णांक widthMin = V3_संख्या::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    शून्य visit(Ast_मोजणी_Ones* nodep) override {
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            स्थिरांक पूर्णांक widthMin = V3_संख्या::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    शून्य visit(AstCvtPackString* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        जर (Ast_गाठ_DType* स्थिरांक dt = nodep->lhsp()->dtypep()) {
            जर (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    शून्य visit(AstCvtPackedToArray* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    शून्य visit(AstCvtArrayToArray* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    शून्य visit(AstCvtArrayToPacked* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    शून्य visit(AstCvtUnpackToQueue* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    शून्य visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        जर (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    शून्य visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        स्थिरांक द्विगुण timePrescale = constp->अंक().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        स्थिरांक द्विगुण समय = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* स्थिरांक newp = नवीन AstConst{nodep->fileline(), AstConst::RealDouble{}, समय};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstEventControl* nodep) override {
        जर (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            परत;
        }
        जर (nodep->fileline()->timingOn()) {
            जर (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                परत;
            } अन्यथा जर (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } अन्यथा {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        जर (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    स्थिर string typenameExtends(स्थिरांक AstClass* classp) {
        // An interface class may extend several, so those are not named
        जर (!classp || classp->isInterfaceClass()) परत "";
        साठी (स्थिरांक AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            जर (extendsp->isImplements()) सुरूठेवा;
            स्थिरांक AstClass* स्थिरांक basep = extendsp->classOrNullp();
            परत basep ? " extends class " + basep->dtypeName(सत्य) : "";
        }
        परत "";
    }
    शून्य visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        जर (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        निवड (nodep->attrType()) {
        स्थिती VAttrType::DIM_DIMENSIONS:
        स्थिती VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_गाठ_DType* स्थिरांक dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            स्थिरांक std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(सत्य);
            स्थिरांक पूर्णांक val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(नवीन AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            थांबा;
        }
        स्थिती VAttrType::DIM_BITS_OR_संख्या: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            जर (VN_IS(nodep->fromp(), गाठ_अभिव्यक्ती)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            } अन्यथा {
                Ast_गाठ* newp = नवीन AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            परत;
        }
        स्थिती VAttrType::DIM_BITS:
        स्थिती VAttrType::DIM_वरचा:
        स्थिती VAttrType::DIM_INCREMENT:
        स्थिती VAttrType::DIM_डावा:
        स्थिती VAttrType::DIM_खालचा:
        स्थिती VAttrType::DIM_उजवा:
        स्थिती VAttrType::DIM_SIZE: {
            Ast_गाठ_DType* स्थिरांक dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            जर (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                स्थिरांक Ast_गाठ_Module* स्थिरांक ownModp = v3Global.rootp()->containingModule(dtypep);
                जर (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    परत;
                }
            }

            जर (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                निवड (nodep->attrType()) {
                स्थिती VAttrType::DIM_SIZE: {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = VN_AS(nodep->fromp()->unlinkFrBack(), गाठ_अभिव्यक्ती);
                    Ast_गाठ* स्थिरांक newp
                        = नवीन AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(सत्य);
                    newp->protect(असत्य);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    थांबा;
                }
                स्थिती VAttrType::DIM_डावा:
                स्थिती VAttrType::DIM_खालचा: {
                    Ast_गाठ* स्थिरांक newp = नवीन AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    थांबा;
                }
                स्थिती VAttrType::DIM_उजवा:
                स्थिती VAttrType::DIM_वरचा: {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = VN_AS(nodep->fromp()->unlinkFrBack(), गाठ_अभिव्यक्ती);
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक sizep
                        = नवीन AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(सत्य);
                    sizep->protect(असत्य);
                    Ast_गाठ* स्थिरांक newp
                        = नवीन AstSub{nodep->fileline(), sizep,
                                     नवीन AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    थांबा;
                }
                स्थिती VAttrType::DIM_INCREMENT: {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक newp
                        = नवीन AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    थांबा;
                }
                स्थिती VAttrType::DIM_BITS: {
                    जर (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } अन्यथा {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    थांबा;
                }
                पूर्वनिर्धारित: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } अन्यथा {
                स्थिरांक std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(सत्य);
                स्थिरांक uint32_t msbdim = dimpair.first + dimpair.second;
                जर (!nodep->dimp() || msbdim < 1) {
                    जर (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = VN_AS(nodep->fromp()->unlinkFrBack(), गाठ_अभिव्यक्ती);
                        Ast_गाठ* स्थिरांक newp = नवीन AstShiftL{
                            nodep->fileline(), नवीन AstLenN{nodep->fileline(), fromp},
                            नवीन AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    } अन्यथा {
                        Ast_गाठ_DType* स्थिरांक baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        स्थिरांक पूर्णांक dim = 1;
                        AstConst* स्थिरांक newp = dimension_मूल्य(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    }
                } अन्यथा जर (VN_IS(nodep->dimp(), Const)) {
                    स्थिरांक पूर्णांक dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* स्थिरांक newp
                        = dimension_मूल्य(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                } अन्यथा {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* स्थिरांक tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक dimp = nodep->dimp()->unlinkFrBack();
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                }
            }
            थांबा;
        }
        स्थिती VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            थांबा;
        स्थिती VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_गाठ_DType* स्थिरांक protoDtp = nodep->fromp()->dtypep();
            Ast_गाठ_DType* स्थिरांक declDtp = [&]() {
                जर (m_ftaskp->fvarp()) परत m_ftaskp->fvarp()->dtypep();
                Ast_गाठ_DType* स्थिरांक voidp = नवीन AstVoidDType{m_ftaskp->fileline()};
                ढकला_Deletep(voidp);  // Note voidp used past here
                परत voidp;
            }();
            जर (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warnMore()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnMore()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            थांबा;
        }
        स्थिती VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_गाठ_DType* स्थिरांक dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(सत्य);
            जर (स्थिरांक AstClassRefDType* स्थिरांक classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_गाठ* स्थिरांक newp = नवीन AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            थांबा;
        }
        स्थिती VAttrType::TYPEID:
            जर (Ast_गाठ_DType* dtypep = VN_CAST(nodep->fromp(), गाठ_DType)) {
                nodep->dtypep(dtypep);
            } अन्यथा {
                nodep->dtypep(VN_AS(nodep->fromp(), गाठ_अभिव्यक्ती)->dtypep());
            }
            थांबा;
        स्थिती VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            थांबा;
        पूर्वनिर्धारित: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            थांबा;
        }
        }
    }
    शून्य visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    शून्य visit(AstCellArrayRef* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(सत्य);
    }

    शून्य visit(AstCellRef* nodep) override {
        जर (nodep->didWidthAndSet()) परत;

        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक cellExprp = VN_CAST(nodep->cellp(), गाठ_अभिव्यक्ती)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = VN_CAST(nodep->exprp(), गाठ_अभिव्यक्ती)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } अन्यथा {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(सत्य);
    }

    // DTYPES
    शून्य visit(Ast_गाठ_Array_DType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed

        जर (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* स्थिरांक basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            जर (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* स्थिरांक newp = नवीन AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        जर (VN_IS(nodep, PackArrayDType)) m_underPackedArray = सत्य;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        जर (AstUnpackArrayDType* स्थिरांक adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            जर (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(सत्य);
        } अन्यथा {
            स्थिरांक Ast_गाठ_DType* स्थिरांक elemDTypep = nodep->subDTypep()->skipRefp();
            जर (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            स्थिरांक पूर्णांक width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            जर (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                स्थिरांक Ast_गाठ_DType* baseDTypep = nodep->subDTypep();
                पर्यंत (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                स्थिरांक AstBasicDType* स्थिरांक basicp = nodep->basicp();
                जर (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(सत्य));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstAssocArrayDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->किल्ली_DTypep(iterateEditMoveDTypep(nodep, nodep->किल्ली_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_गाठ_DType* स्थिरांक childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_गाठ* स्थिरांक elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_गाठ* newp;
        जर (VN_IS(elementsp, Unbounded)) {
            newp = नवीन AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_वृक्ष(), elementsp);
        } अन्यथा जर (Ast_गाठ_DType* स्थिरांक keyp = VN_CAST(elementsp, गाठ_DType)) {
            newp = नवीन AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } अन्यथा {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_ओळ* स्थिरांक elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, सत्य);
            // Must be expression that is constant, but we'll determine that later
            newp = नवीन AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                नवीन AstRange{nodep->fileline(), नवीन AstConst(elementsp->fileline(), 0),
                             नवीन AstSub{elementsNewFl, VN_AS(elementsp, गाठ_अभिव्यक्ती),
                                        नवीन AstConst(elementsNewFl, 1)},
                             सत्य}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    शून्य visit(AstConstraintRefDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    शून्य visit(AstDynArrayDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstQueueDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        जर (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_वृक्ष();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstVoidDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstUnsizedArrayDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        जर (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstWildcardArrayDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstBasicDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        जर (nodep->generic()) परत;  // Already perfect
        जर (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } अन्यथा जर (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } अन्यथा जर (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->widthForce(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstConstDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(AstRefDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(सत्य);
        जर (nodep->typeofp()) {  // type(typeofp_expression)
            जर (Ast_गाठ_DType* typeofDtp = VN_CAST(nodep->typeofp(), गाठ_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } अन्यथा {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_गाठ* स्थिरांक typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_वृक्ष(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        जर (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), nullptr);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), nullptr);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        जर (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            स्थिरांक तार्किक inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            जर (inTemplateModule) {
                nodep->doingWidth(असत्य);
                परत;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(असत्य);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    शून्य widthDTypeNameParams(स्थिरांक AstTypedef* nodep) {
        स्थिरांक VDTypeNameScopes* स्थिरांक scopesp = VDTypeNameScopes::currentp();
        जर (!scopesp) परत;  // Computing parameters, as by V3Param, so naming as found
        Ast_गाठ* scopep = nullptr;
        साठी (स्थिरांक Ast_गाठ* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            जर (स्थिरांक std::vector<Ast_गाठ*>* स्थिरांक paramsp = scopesp->paramsp(scopep)) {
                साठी (Ast_गाठ* स्थिरांक paramp : *paramsp) {
                    जर (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    शून्य visit(AstTypedef* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        जर (स्वतः* स्थिरांक refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
            परत;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        स्थिरांक तार्किक declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        जर (declares) {
            Ast_गाठ_DType* स्थिरांक dtypep = nodep->dtypep();
            जर (Ast_गाठ_UOr_Struct_DType* स्थिरांक sdtypep = VN_CAST(dtypep, गाठ_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } अन्यथा जर (AstEnumDType* स्थिरांक edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        जर (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    शून्य visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        जर (Ast_गाठ_DType* स्थिरांक dtp = VN_CAST(nodep->lhsp(), गाठ_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } अन्यथा {
            जर (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(नवीन AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_गाठ_DType* स्थिरांक toDtp = nodep->शीर्ष()->dtypep()->skipRefToEnump();
        Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_ओळ* स्थिरांक fl = nodep->fileline();
        स्थिरांक स्वतः castable = Ast_गाठ::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_गाठ* newp;
        जर (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            परत;
        } अन्यथा जर (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* स्थिरांक enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_गाठ_अभिव्यक्ती* स्थिरांक testp = enumTestValid(nodep->fromp(), enumDtp);
            File_ओळ* स्थिरांक fl_novalue = नवीन File_ओळ{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, सत्य);
            newp = नवीन AstCond{
                fl, testp,
                नवीन Ast_अभिव्यक्ती_Stmt{fl,
                                नवीन AstAssign{fl_novalue, nodep->शीर्ष()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                नवीन AstConst{fl, AstConst::Signed32{}, 1}},
                नवीन AstConst{fl, AstConst::Signed32{}, 0}};
        } अन्यथा जर ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = नवीन Ast_अभिव्यक्ती_Stmt{
                fl,
                नवीन AstAssign{fl, nodep->शीर्ष()->unlinkFrBack(),
                              नवीन AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                नवीन AstConst{fl, AstConst::Signed32{}, 1}};
        } अन्यथा जर (castable == VCastable::INCOMPATIBLE) {
            newp = नवीन AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } अन्यथा {
            newp = नवीन AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    शून्य visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        जर (AstConst* स्थिरांक constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_गाठ* स्थिरांक newp
                = नवीन AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } अन्यथा जर (Ast_गाठ_DType* स्थिरांक refp = VN_CAST(nodep->dtp(), गाठ_DType)) {
            refp->unlinkFrBack();
            Ast_गाठ* स्थिरांक newp = नवीन AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstCast* nodep) override {
        जर (nodep->didWidth()) परत;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        जर (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            जर (AstSigned* स्थिरांक fromp = VN_CAST(nodep->fromp(), Signed)) {
                जर (VN_IS(fromp->lhsp(), गाठ_Stream)) {
                    Ast_गाठ* स्थिरांक lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_वृक्ष(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_गाठ_DType* स्थिरांक toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            स्थिरांक स्वतः castable = Ast_गाठ::computeCastable(toDtp, fromDtp, nodep->fromp());
            तार्किक bad = असत्य;
            जर (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = सत्य;
            } अन्यथा जर (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } अन्यथा जर (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = सत्य;
            } अन्यथा जर (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = सत्य;
            } अन्यथा {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_गाठ_अभिव्यक्ती* newp = nullptr;
            जर (bad) {
            } अन्यथा जर (AstBasicDType* स्थिरांक basicp = toDtp->basicp()) {
                जर (!basicp->isString() && fromDtp->isString()) {
                    newp = नवीन AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } अन्यथा जर (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_गाठ_DType* स्थिरांक origDTypep = nodep->dtypep();
                    जर (!VN_IS(fromDtp, StreamDType)) {
                        स्थिरांक पूर्णांक width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } अन्यथा {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 असत्य);
                }
                जर (newp) {
                } अन्यथा जर (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    जर (nodep->fromp()->isSigned()) {
                        newp = नवीन AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } अन्यथा {
                        newp = नवीन AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } अन्यथा जर (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = नवीन AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } अन्यथा जर (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = नवीन AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } अन्यथा जर (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = नवीन AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } अन्यथा {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } अन्यथा जर (VN_IS(toDtp, QueueDType)) {
                जर (VN_IS(fromDtp, BasicDType)) {
                    newp = नवीन AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } अन्यथा जर (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    पूर्णांक srcElementBits = 1;
                    पूर्णांक dstElementBits = 1;
                    जर (Ast_गाठ_DType* स्थिरांक elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    स्थिरांक AstQueueDType* स्थिरांक dstQueueDtp = VN_AS(toDtp, QueueDType);
                    जर (Ast_गाठ_DType* स्थिरांक elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = नवीन AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  असत्य,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } अन्यथा जर (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } अन्यथा {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            जर (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        जर (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, असत्य, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_गाठ_अभिव्यक्ती* स्थिरांक underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(सत्य);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(सत्य);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            पूर्णांक width = nodep->rhsp()->toSInt();
            जर (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        जर (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_गाठ* स्थिरांक underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    शून्य visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    शून्य castSized(Ast_गाठ* nodep, Ast_गाठ* underp, पूर्णांक width) {
        स्थिरांक AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        जर (!underDtp) underDtp = underp->dtypep()->basicp();
        जर (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            स्थिरांक पूर्णांक calcWidth = std::max(width, underDtp->width());
            Ast_गाठ_DType* स्थिरांक calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         असत्य, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_गाठ_DType* स्थिरांक outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, गाठ_अभिव्यक्ती), outDtp, EXTEND_EXP,
                            असत्य);
            VL_DANGLING(underp);
        }
    }
    शून्य visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(Ast_Constraint_अभिव्यक्ती* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    शून्य visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        जर (nodep->didWidth()) परत;
        जर (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ढकला_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(सत्य);
            परत;
        }
        nodep->doingWidth(सत्य);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        जर (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.ढकला_back(nodep);
        जर (nodep->attrsp()) {
            nodep->attrsp()->foreach([हे, nodep](AstAttrOf* attrp) {
                जर (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    जर (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), गाठ_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ढकला_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        जर (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_गाठ_DType* dtp = nodep->dtypep();
            Ast_गाठ_DType* np = nullptr;
            पर्यंत (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                जर (स्थिरांक AstDynArrayDType* स्थिरांक unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    जर (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } अन्यथा {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* स्थिरांक newp
                        = नवीन AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    जर (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } अन्यथा {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } अन्यथा {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        जर (AstWildcardArrayDType* स्थिरांक wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        जर (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(सत्य);
        // Parameters if implicit untyped inherit from what they are assigned to
        स्थिरांक AstBasicDType* स्थिरांक bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        तार्किक didchk = असत्य;
        स्थिरांक तार्किक implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        जर (implicitParam) {
            जर (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                जर (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } अन्यथा जर (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } अन्यथा {
                    पूर्णांक width = 0;
                    Ast_गाठ_DType* स्थिरांक मूल्य_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(मूल्य_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    स्थिरांक AstBasicDType* स्थिरांक मूल्य_Bdtypep = मूल्य_DTypep->basicp();
                    तार्किक issigned = असत्य;
                    जर (bdtypep->isNosign()) {
                        जर (मूल्य_Bdtypep && मूल्य_Bdtypep->isSigned()) issigned = सत्य;
                    } अन्यथा {
                        issigned = bdtypep->isSigned();
                    }
                    जर (मूल्य_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        जर (स्थिरांक स्वतः* स्थिरांक constp = VN_CAST(nodep->valuep(), Const)) {
                            जर (constp->अंक().isString()) {
                                width = constp->अंक().toString().length() * 8;
                            }
                        }
                        जर (width < 8) width = 8;
                    } अन्यथा जर (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } अन्यथा {
                        जर (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    स्थिरांक तार्किक is_मूल्य_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    जर (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_मूल्य_Fourstate));
                    } अन्यथा {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_मूल्य_Fourstate);
                    }
                    didchk = सत्य;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } अन्यथा {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } अन्यथा जर (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        जर (nodep->isNet()) {
            Ast_गाठ_DType* स्थिरांक badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            जर (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        जर (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(सत्य);
        nodep->doingWidth(असत्य);
    }
    शून्य visit(Ast_गाठ_Var_Ref* nodep) override {
        जर (nodep->didWidth()) परत;
        जर (!nodep->varp()) {
            जर (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, असत्य,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                परत;
            } अन्यथा {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        जर (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        जर (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            परत;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        जर (VN_IS(nodep->backp(), गाठ_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        जर (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(सत्य);
        जर (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } अन्यथा जर (nodep->access().isWriteOrRW() && nodep->varp()->is_आदान()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            स्थिरांक तार्किक hierRef = !m_curModVars.मोजणी(nodep->varp());
            स्थिरांक तार्किक netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            स्थिरांक तार्किक contAssign = VN_IS(nodep->abovep(), AssignW);
            स्थिरांक तार्किक pinConn = VN_IS(nodep->abovep(), Pin);
            जर (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } अन्यथा जर (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        जर (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(सत्य);
    }

    शून्य visit(AstEnumDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_गाठ_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_गाठ_DType* स्थिरांक badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        जर (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->findIntDType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->widthFromSub(nodep->subDTypep());
        // Assign widths
        userIterateAndNext(nodep->itemsp(), WidthVP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_संख्या अंक(nodep, nodep->width(), 0);
        स्थिरांक V3_संख्या one{nodep, nodep->width(), 1};
        तार्किक wrapAround = असत्य;
        std::map<स्थिरांक V3_संख्या, AstEnumItem*> inits;
        साठी (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            जर (itemp->valuep()) {
                जर (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_वृक्ष("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                जर (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_वृक्ष();
                    सुरूठेवा;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = असत्य;  // Explicit value resets wrap-around tracking
            }
            जर (!itemp->valuep()) {
                जर (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                जर (अंक.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(नवीन AstConst{itemp->fileline(), अंक});
            }

            स्थिरांक AstConst* स्थिरांक constp = VN_AS(itemp->valuep(), Const);
            जर (constp->अंक().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            अंक.opAssign(constp->अंक());
            // Look for duplicates
            स्थिरांक स्वतः pair = inits.emplace(अंक, itemp);
            जर (!pair.second) {  // IEEE says illegal
                स्थिरांक Ast_गाठ* स्थिरांक otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            स्थिरांक तार्किक prevNeg = constp->अंक().isNegative();
            अंक.opAdd(one, constp->अंक());
            जर (basicp->isSigned()) {
                wrapAround = !prevNeg && अंक.isNegative();
            } अन्यथा {
                wrapAround = अंक.isEqZero();
            }
        }
    }
    शून्य visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_गाठ_DType* स्थिरांक vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        जर (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            तार्किक warnOn = सत्य;
            Ast_गाठ_अभिव्यक्ती* valuep = nodep->valuep();
            जर (स्थिरांक AstAdd* स्थिरांक anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                जर (anodep->fileline()->equal_First_ओळ_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(valuep, Const)) {
                जर (static_cast<पूर्णांक>(constp->अंक().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = असत्य;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(नवीन AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    शून्य visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        जर (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_गाठ* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            साठी (; enump; enump = enump->backp()) {
                जर (VN_IS(enump, EnumDType)) थांबा;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    शून्य visit(Ast_गाठ_Cons* nodep) override {}  // Widthed when created
    शून्य visit(Ast_गाठ_Cons_Item* nodep) override {}  // Widthed when created
    शून्य visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        साठी (Ast_गाठ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_गाठ_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        जर (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } अन्यथा जर (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } अन्यथा {
            // Take width as maximum across all items
            पूर्णांक width = nodep->exprp()->width();
            पूर्णांक mwidth = nodep->exprp()->widthMin();
            तार्किक isFourstate = nodep->exprp()->dtypep()->isFourstate();
            साठी (स्थिरांक Ast_गाठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        साठी (Ast_गाठ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            जर (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } अन्यथा {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        जर (m_constraintp) {
            तार्किक canLower = सत्य;
            साठी (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                जर (VN_IS(ditemp->rangep(), InsideRange)) सुरूठेवा;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                स्थिरांक Ast_गाठ_DType* स्थिरांक itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                जर (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = असत्य;
                    थांबा;
                }
            }
            जर (canLower) परत;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_गाठ_अभिव्यक्ती* newp = nullptr;
        साठी (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_गाठ_अभिव्यक्ती* inewp
                = insideItem(nodep, nodep->exprp()->clone_वृक्ष_Pure(असत्य), itemp->rangep());
            जर (!inewp) सुरूठेवा;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक cmpp
                = नवीन AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            नवीन AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, सत्य);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, सत्य);
            inewp = नवीन AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? नवीन AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        जर (!newp) newp = नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    शून्य visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        साठी (Ast_गाठ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_गाठ_DType* expDTypep = nullptr;

        जर (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } अन्यथा जर (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } अन्यथा {
            // Take width as maximum across all items
            पूर्णांक width = nodep->exprp()->width();
            पूर्णांक mwidth = nodep->exprp()->widthMin();
            तार्किक isFourstate = nodep->exprp()->dtypep()->isFourstate();
            साठी (स्थिरांक Ast_गाठ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            स्थिरांक VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        साठी (Ast_गाठ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            जर (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } अन्यथा जर (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_गाठ_अभिव्यक्ती* exprp;
        Ast_अभिव्यक्ती_Stmt* अभिव्यक्ती_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        जर (!m_constraintp && !nodep->exprp()->isPure()) {
            File_ओळ* स्थिरांक fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_गाठ_DType* स्थिरांक अभिव्यक्ती_Dtp = nodep->exprp()->dtypep();
            स्थिरांक पूर्णांक w = अभिव्यक्ती_Dtp->width();
            Ast_गाठ_DType* स्थिरांक tempDTypep
                = अभिव्यक्ती_Dtp->widthSized() ? अभिव्यक्ती_Dtp
                                        : nodep->findBitOrLogicDType(w, w, अभिव्यक्ती_Dtp->numeric(),
                                                                     अभिव्यक्ती_Dtp->isFourstate());
            AstVar* स्थिरांक varp
                = नवीन AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = नवीन AstVarRef{fl, varp, VAccess::READ};
            अभिव्यक्ती_Stmtp = नवीन Ast_अभिव्यक्ती_Stmt{fl,
                                        नवीन AstAssign{fl, नवीन AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            जर (m_ftaskp) {
                varp->funcLocal(सत्य);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } अन्यथा {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(अभिव्यक्ती_Stmtp);
        } अन्यथा {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_गाठ_अभिव्यक्ती* newp = nullptr;
        साठी (Ast_गाठ_अभिव्यक्ती *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), गाठ_अभिव्यक्ती);  // Will be unlinking
            Ast_गाठ_अभिव्यक्ती* inewp;
            जर (अभिव्यक्ती_Stmtp) {
                inewp = insideItem(nodep, अभिव्यक्ती_Stmtp, itemp);
                अभिव्यक्ती_Stmtp = nullptr;
            } अन्यथा {
                inewp = insideItem(nodep, exprp->clone_वृक्ष_Pure(असत्य), itemp);
            }
            जर (!inewp) सुरूठेवा;
            newp = newp ? नवीन AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        जर (अभिव्यक्ती_Stmtp) VL_DO_DANGLING(अभिव्यक्ती_Stmtp->delete_वृक्ष(), अभिव्यक्ती_Stmtp);
        जर (!newp) newp = नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    Ast_गाठ_अभिव्यक्ती* insideItem(Ast_गाठ* nodep, Ast_गाठ_अभिव्यक्ती* exprp, Ast_गाठ_अभिव्यक्ती* itemp) {
        स्थिरांक Ast_गाठ_DType* स्थिरांक itemDtp = itemp->dtypep()->skipRefp();
        जर (AstInsideRange* स्थिरांक irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            परत irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } अन्यथा जर (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_गाठ_अभिव्यक्ती* स्थिरांक inewp = नवीन AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(सत्य);
            परत inewp;
        } अन्यथा जर (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_वृक्ष(), exprp);
            परत nullptr;
        }
        परत AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    शून्य visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        जर (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } अन्यथा {
            जर (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            जर (m_vup->final()) {
                Ast_गाठ_DType* स्थिरांक expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                स्थिरांक पूर्णांक expWidth = expDTypep->width();
                स्थिरांक तार्किक waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                स्थिरांक तार्किक waiveRhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && expWidth >= nodep->rhsp()->widthMin();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    शून्य visit(AstIfaceRefDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        जर (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य visit(Ast_गाठ_UOr_Struct_DType* nodep) override {
        जर (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(असत्य);
            परत;
        }
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(सत्य);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        जर (स्थिरांक AstUnionDType* स्थिरांक unionp = VN_CAST(nodep, UnionDType)) {
            जर (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(असत्य);
        // Error checks
        साठी (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_गाठ_DType* स्थिरांक dtp = itemp->subDTypep()->skipRefp();
            जर (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            जर ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ढकला_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        स्थिरांक तार्किक isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        स्थिरांक तार्किक inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        जर (VN_IS(nodep, UnionDType) || nodep->packed()) {
            पूर्णांक lsb = 0;
            पूर्णांक width = 0;
            तार्किक first = सत्य;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            साठी (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            साठी (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                जर (itemp->isFourstate()) nodep->isFourstate(सत्य);
                itemp->lsb(lsb);
                जर (VN_IS(nodep, UnionDType)) {
                    स्थिरांक पूर्णांक itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    जर (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } अन्यथा {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = असत्य;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } अन्यथा {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(असत्य);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    शून्य visit(AstThisRef* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        जर (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    शून्य visit(AstClassRefDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        userIterateChildren(nodep, nullptr);
    }
    शून्य visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    शून्य visit(AstClassExtends* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        जर (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    शून्य visit(AstMemberDType* nodep) override {
        जर (nodep->didWidthAndSet()) परत;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        जर (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    शून्य visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        जर (nodep->didWidth()) परत;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = सत्य;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        स्थिरांक AstMemberSel* स्थिरांक fromSel = VN_CAST(nodep->fromp(), MemberSel);
        जर (AstClocking* स्थिरांक clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            जर (AstVar* स्थिरांक varp = memberSelClocking(nodep, clockingp)) {
                जर (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_वृक्ष(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                जर (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                जर (AstIfaceRefDType* स्थिरांक adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(सत्य);  // Must not visit again: will confuse scopes
                परत;
            }
        } अन्यथा जर (Ast_गाठ_UOr_Struct_DType* स्थिरांक adtypep = VN_CAST(fromDtp, गाठ_UOr_Struct_DType)) {
            जर (memberSelStruct(nodep, adtypep)) परत;
        } अन्यथा जर (AstClassRefDType* स्थिरांक adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            जर (memberSelClass(nodep, adtypep)) परत;
        } अन्यथा जर (AstIfaceRefDType* स्थिरांक adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            जर (Ast_गाठ* foundp = memberSelIface(nodep, adtypep)) {
                जर (AstClocking* स्थिरांक clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                जर (AstVar* स्थिरांक varp = VN_CAST(foundp, Var)) {
                    जर (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* स्थिरांक ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    स्थिरांक AstVarRef* स्थिरांक refp = VN_CAST(nodep->fromp(), VarRef);
                    जर (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* स्थिरांक newp = नवीन AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(सत्य);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                        परत;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(सत्य);
                    परत;
                }
                जर (AstModport* स्थिरांक modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* स्थिरांक newDtypep = नवीन AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
                जर (AstCell* स्थिरांक cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    जर (VN_IS(cellp->modp(), Iface)) {
                        स्थिरांक string viftopName = cellp->name() + "__Viftop";
                        Ast_गाठ_Module* स्थिरांक parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        साठी (Ast_गाठ* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            जर (AstVar* स्थिरांक vp = VN_CAST(itemp, Var)) {
                                जर (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    थांबा;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        जर (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_गाठ_DType* subDtypep = viftopVarp->dtypep();
                        जर (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            जर (AstIfaceRefDType* स्थिरांक subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* स्थिरांक newDtypep = नवीन AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(सत्य);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(सत्य);
                        परत;
                    }
                }
                जर (Ast_गाठ_FTask* स्थिरांक ftaskp = VN_CAST(foundp, गाठ_FTask)) {
                    AstMethodCall* स्थिरांक newp = नवीन AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    परत;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } अन्यथा जर (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_गाठ* स्थिरांक newp = नवीन AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            परत;
        } अन्यथा {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }
    तार्किक memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        जर (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* स्थिरांक newp = नवीन AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            visit(newp);
            परत सत्य;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* स्थिरांक first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        साठी (AstClass* classp = first_classp; classp;) {
            जर (Ast_गाठ* स्थिरांक foundp = m_memberMap.findMember(classp, nodep->name())) {
                जर (AstVar* स्थिरांक varp = VN_CAST(foundp, Var)) {
                    जर (!varp->didWidth()) userIterate(varp, nullptr);
                    जर (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* स्थिरांक varRefp
                            = नवीन AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                        परत सत्य;
                    }
                    जर (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(सत्य);
                    जर (nodep->fromp()->same_वृक्ष(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    परत सत्य;
                }
                जर (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(नवीन AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत सत्य;
                }
                जर (AstEnumItemRef* स्थिरांक adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_वृक्ष(असत्य));
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत सत्य;
                }
                जर (Ast_गाठ_FTask* ftaskp = VN_CAST(foundp, गाठ_FTask)) {
                    AstMethodCall* newp = नवीन AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत सत्य;
                }
                जर (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    परत सत्य;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }

        VSpellCheck speller;
        साठी (AstClass* classp = first_classp; classp;) {
            साठी (Ast_गाठ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                जर (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.ढकला_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        स्थिरांक string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        परत असत्य;  // Caller handles error
    }
    तार्किक modportAllowsItem(Ast_गाठ* nodep, स्थिरांक AstIfaceRefDType* adtypep, स्थिरांक Ast_गाठ* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        स्थिरांक AstModport* स्थिरांक modportp = adtypep->modportp();
        जर (!modportp) परत सत्य;
        जर (स्थिरांक AstVar* स्थिरांक varp = VN_CAST(itemp, Var)) {
            जर (varp->isParam()) परत सत्य;
        } अन्यथा जर (!VN_IS(itemp, गाठ_FTask) && !VN_IS(itemp, Clocking)) {
            परत सत्य;
        }
        स्वतः pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        जर (pair.second) {
            साठी (स्थिरांक Ast_गाठ* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.घाला(mpItemp->name());
            }
        }
        जर (pair.first->second.मोजणी(itemp->name())) परत सत्य;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        परत असत्य;
    }
    Ast_गाठ* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_गाठ_Module* स्थिरांक ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        साठी (Ast_गाठ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            जर (itemp->name() == nodep->name()) {
                जर (!modportAllowsItem(nodep, adtypep, itemp)) {
                    परत nullptr;  // Caller handles error
                }
                परत itemp;
            }
            जर (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ढकला_Candidate(itemp->prettyName());
            }
        }
        स्थिरांक string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        परत nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        साठी (Ast_गाठ* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            जर (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                जर (citemp->varp()->name() == nodep->name()) परत citemp->varp();
                speller.ढकला_Candidate(citemp->varp()->prettyName());
            }
        }
        स्थिरांक string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        परत nullptr;  // Caller handles error
    }
    तार्किक memberSelStruct(AstMemberSel* nodep, Ast_गाठ_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        जर (AstMemberDType* स्थिरांक memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            जर (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } अन्यथा जर (adtypep->packed()) {
                AstSel* स्थिरांक newp = नवीन AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(सत्य);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } अन्यथा {
                AstStructSel* स्थिरांक newp = नवीन AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(सत्य);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            परत सत्य;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        परत असत्य;
    }

    शून्य visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    शून्य visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        जर (nodep->didWidth()) परत;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_गाठ_DType* स्थिरांक fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* स्थिरांक basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        जर (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } अन्यथा जर (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } अन्यथा जर (AstEnumDType* स्थिरांक adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } अन्यथा जर (AstAssocArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } अन्यथा जर (AstWildcardArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } अन्यथा जर (AstDynArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } अन्यथा जर (AstQueueDType* स्थिरांक adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } अन्यथा जर (AstClassRefDType* स्थिरांक adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } अन्यथा जर (AstIfaceRefDType* स्थिरांक adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } अन्यथा जर (AstUnpackArrayDType* स्थिरांक adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } अन्यथा जर (AstConstraintRefDType* स्थिरांक adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } अन्यथा जर (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } अन्यथा जर (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_गाठ_FTask_Ref* nodep, तार्किक required, तार्किक arbReturn,
                              Ast_गाठ_DType* returnDtp, Ast_गाठ_DType* indexDtp,
                              Ast_गाठ_DType* मूल्य_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        जर (AstWith* स्थिरांक withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->मूल्य_Arg_Refp()->dtypep(मूल्य_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            परत withp;
        }
        जर (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        परत nullptr;
    }
    शून्य methodOkArguments(Ast_गाठ_FTask_Ref* nodep, पूर्णांक minArg, पूर्णांक maxArg) {
        पूर्णांक narg = 0;
        जर (AstWith* स्थिरांक withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ढकला_Deletep(withp->unlinkFrBack()), withp);
        }
        साठी (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        स्थिरांक तार्किक ok = (narg >= minArg) && (narg <= maxArg);
        जर (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            साठी (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    नवीन AstArg{nodep->fileline(), "",
                               नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            साठी (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                पर्यंत (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_वृक्ष(), argp);
            }
        }
    }

    Ast_गाठ_अभिव्यक्ती* methodArg(AstMethodCall* nodep, पूर्णांक arg) {
        AstArg* argp = nodep->argsp();
        साठी (पूर्णांक narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        परत argp->exprp();
    }

    शून्य methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        जर (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            जर (nodep->name() == "num") {
                पूर्णांक वस्तुसंच = 0;
                साठी (Ast_गाठ* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++वस्तुसंच;
                newp = नवीन AstConst(nodep->fileline(), AstConst::Signed32{}, वस्तुसंच);
            } अन्यथा जर (nodep->name() == "first") {
                स्थिरांक AstEnumItem* itemp = adtypep->itemsp();
                जर (!itemp) {
                    newp = नवीन AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } अन्यथा {
                    newp = VN_AS(itemp->valuep()->clone_वृक्ष(असत्य), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } अन्यथा जर (nodep->name() == "last") {
                स्थिरांक AstEnumItem* itemp = adtypep->itemsp();
                पर्यंत (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                जर (!itemp) {
                    newp = नवीन AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } अन्यथा {
                    newp = VN_AS(itemp->valuep()->clone_वृक्ष(असत्य), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा जर (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            जर (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } अन्यथा जर (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } अन्यथा जर (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }

            जर (nodep->name() != "name" && nodep->argsp()) {
                Ast_गाठ_अभिव्यक्ती* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, सत्य);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                जर (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(stepp), stepp);
                    stepp = newp;
                }
                स्थिरांक uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                जर (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* स्थिरांक newp = नवीन AstMethodCall{
                        nodep->fileline(),
                        नवीन AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    परत;
                } अन्यथा जर (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ढकला_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* स्थिरांक clonep = nodep->clone_वृक्ष(असत्य);
                    VN_AS(stepp, Const)->अंक().setLong(1);
                    AstConst* स्थिरांक constp = नवीन AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* स्थिरांक argp = नवीन AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* स्थिरांक newp
                        = नवीन AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                    परत;
                }
            }
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    शून्य methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        जर (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } अन्यथा जर (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } अन्यथा जर (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक index_exprp = method_Call_Wildcard_Index_अभिव्यक्ती(nodep, adtypep);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } अन्यथा जर (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            जर (!nodep->argsp()) {
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } अन्यथा {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक index_exprp = method_Call_Wildcard_Index_अभिव्यक्ती(nodep, adtypep);
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } अन्यथा जर (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } अन्यथा जर (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "map") {
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, सत्य, असत्य, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        जर (newp) {
            newp->protect(असत्य);
            newp->didWidth(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    शून्य methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        जर (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        जर (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } अन्यथा जर (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->किल्ली_DTypep(), BOTH);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक index_exprp = method_Call_Assoc_Index_अभिव्यक्ती(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->किल्ली_DTypep(), BOTH);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक index_exprp = method_Call_Assoc_Index_अभिव्यक्ती(nodep, adtypep);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } अन्यथा जर (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            जर (!nodep->argsp()) {
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } अन्यथा {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->किल्ली_DTypep(),
                                  BOTH);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक index_exprp = method_Call_Assoc_Index_अभिव्यक्ती(nodep, adtypep);
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } अन्यथा जर (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } अन्यथा जर (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* स्थिरांक withp = methodWithClause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                                    adtypep->किल्ली_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* स्थिरांक withp = methodWithClause(
                nodep, असत्य, सत्य, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            जर (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->किल्ली_DTypep()));
            } अन्यथा {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* स्थिरांक withp = methodWithClause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                                    adtypep->किल्ली_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* स्थिरांक withp = methodWithClause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                                    adtypep->किल्ली_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->किल्ली_DTypep()));
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "map") {
            AstWith* स्थिरांक withp = methodWithClause(nodep, सत्य, असत्य, adtypep->subDTypep(),
                                                    adtypep->किल्ली_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        जर (newp) {
            newp->protect(असत्य);
            newp->didWidth(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    Ast_गाठ_अभिव्यक्ती* method_Call_Assoc_Index_अभिव्यक्ती(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_गाठ* स्थिरांक index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->किल्ली_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        परत nodep->argsp()->exprp();
    }
    Ast_गाठ_अभिव्यक्ती* method_Call_Wildcard_Index_अभिव्यक्ती(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_गाठ* स्थिरांक index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        परत nodep->argsp()->exprp();
    }
    शून्य methodCallLValueRecurse(AstMethodCall* nodep, Ast_गाठ* childp, स्थिरांक VAccess& access) {
        जर (AstCMethodHard* स्थिरांक ichildp = VN_CAST(childp, CMethodHard)) {
            तार्किक methAt = असत्य;
            तार्किक methAtBack = असत्य;
            तार्किक methAtWrite = असत्य;
            निवड (ichildp->method()) {
            स्थिती VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = सत्य;
                थांबा;
            स्थिती VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = सत्य;
                थांबा;
            स्थिती VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            स्थिती VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            स्थिती VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = सत्य;
                थांबा;
            पूर्वनिर्धारित: थांबा;
            }
            जर (methAt || methAtBack || methAtWrite) {
                स्थिरांक Ast_गाठ_DType* स्थिरांक fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                जर (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    जर (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } अन्यथा जर (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                परत;
            }
        }
        जर (Ast_गाठ_Var_Ref* स्थिरांक varrefp = VN_CAST(childp, गाठ_Var_Ref)) {
            varrefp->access(access);
        } अन्यथा जर (स्थिरांक AstMemberSel* स्थिरांक ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अन्यथा जर (स्थिरांक AstStructSel* स्थिरांक ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अन्यथा जर (स्थिरांक Ast_गाठ_Sel* स्थिरांक ichildp = VN_CAST(childp, गाठ_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } अन्यथा जर (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } अन्यथा {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_गाठ_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        जर (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            जर (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, असत्य, सत्य, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* स्थिरांक withp = methodWithClause(
                nodep, असत्य, सत्य, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            जर (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } अन्यथा {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, सत्य, असत्य, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        परत newp;
    }
    शून्य methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        जर (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा जर (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा जर (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } अन्यथा जर (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर ((newp = methodCallArray(nodep, adtypep))) {
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        जर (newp) {
            newp->protect(असत्य);
            newp->didWidth(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    शून्य methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        जर (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा जर (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा जर (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } अन्यथा जर (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            जर (!nodep->argsp()) {
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } अन्यथा {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक index_exprp = method_Call_Queue_Index_अभिव्यक्ती(nodep);
                जर (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_काढा_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } अन्यथा {
                    newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } अन्यथा जर (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक index_exprp = method_Call_Queue_Index_अभिव्यक्ती(nodep);
            AstArg* स्थिरांक argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            जर (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ढकला_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } अन्यथा {
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_घाला, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } अन्यथा जर (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा जर (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* स्थिरांक argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } अन्यथा जर (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } अन्यथा जर ((newp = methodCallArray(nodep, adtypep))) {
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        जर (newp) {
            newp->protect(असत्य);
            newp->didWidth(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        }
    }
    Ast_गाठ_अभिव्यक्ती* method_Call_Queue_Index_अभिव्यक्ती(AstMethodCall* nodep) {
        Ast_गाठ* स्थिरांक index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        परत nodep->argsp()->exprp();
    }
    शून्य methodCallWarnTiming(Ast_गाठ_FTask_Ref* स्थिरांक nodep, स्थिरांक std::string& className) {
        जर (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } अन्यथा जर (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    शून्य methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* स्थिरांक ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        जर (Ast_गाठ_FTask* स्थिरांक ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), गाठ_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            जर (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                परत;
            }
            जर (adtypep->isVirtual()) {
                साठी (Ast_गाठ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    जर (AstVar* स्थिरांक mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            जर (ftaskp->isStatic()) {
                AstArg* स्थिरांक argsp = nodep->argsp();
                जर (argsp) argsp->unlinkFrBackWithNext();
                Ast_गाठ_FTask_Ref* newp = nullptr;
                जर (VN_IS(ftaskp, Task)) {
                    newp = नवीन AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } अन्यथा {
                    newp = नवीन AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            } अन्यथा {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                जर (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            परत;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    शून्य handleRandomizeArgs(Ast_गाठ_FTask_Ref* स्थिरांक nodep, AstClass* स्थिरांक classp) {
        तार्किक hasNonNullArgs = असत्य;
        AstConst* nullp = nullptr;
        साठी (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_गाठ_अभिव्यक्ती* exprp = argp->exprp();
            जर (AstConst* स्थिरांक constp = VN_CAST(exprp, Const)) {
                जर (constp->अंक().isNull()) {
                    nullp = constp;
                    सुरूठेवा;
                }
            }
            hasNonNullArgs = सत्य;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            जर (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_गाठ_अभिव्यक्ती* fromp = methodCallp->fromp();
                पर्यंत (AstMemberSel* स्थिरांक memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* स्थिरांक varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            जर (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्ष(), argp);
                सुरूठेवा;
            }
            पर्यंत (exprp) {
                जर (AstMemberSel* स्थिरांक memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } अन्यथा {
                    जर (AstVarRef* स्थिरांक varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } अन्यथा {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्ष(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                जर (randVarp && !randVarp->isRand()) randVarp->अक्रम(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            जर (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } अन्यथा जर (classp->existsMember([&](स्थिरांक AstClass*, स्थिरांक AstVar* memberVarp) {
                           परत memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } अन्यथा {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_वृक्ष(), argp);
            }
        }
        जर (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    शून्य methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* स्थिरांक first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        जर (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, असत्य, असत्य, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            साठी (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                जर (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } अन्यथा जर (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        साठी (AstClass* classp = first_classp; classp;) {
            जर (nodep->fileline()->timingOn()) {
                जर (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* स्थिरांक packagep = getItemPackage(classp);
                    // Check if it's std
                    जर (packagep && packagep->name() == "std") {
                        जर (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } अन्यथा जर (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } अन्यथा जर (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            जर (Ast_गाठ_FTask* स्थिरांक ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), गाठ_FTask)) {
                userIterate(ftaskp, nullptr);
                जर (ftaskp->isStatic()) {
                    AstArg* स्थिरांक argsp = nodep->argsp();
                    जर (argsp) argsp->unlinkFrBackWithNext();
                    Ast_गाठ_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    जर (!m_vup) {  // Called as task
                        newp = नवीन AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } अन्यथा {
                        newp = नवीन AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                } अन्यथा {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    जर (VN_IS(ftaskp, Task)) {
                        जर (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_अभिव्यक्ती)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    जर (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                परत;
            } अन्यथा जर (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(सत्य);
                v3Global.useRandomizeMethods(सत्य);
                AstCMethodHard* स्थिरांक newp
                    = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(सत्य);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            } अन्यथा जर (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(सत्य);
                v3Global.useRandomizeMethods(सत्य);
                AstCMethodHard* स्थिरांक newp
                    = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(सत्य);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            } अन्यथा जर (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(सत्य);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            साठी (AstClass* classp = first_classp; classp;) {
                साठी (Ast_गाठ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    जर (VN_IS(itemp, गाठ_FTask)) speller.ढकला_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            स्थिरांक string suggest = speller.best_Candidate_संदेश(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    शून्य methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        जर (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            जर (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } अन्यथा {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(सत्य);
        } अन्यथा {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        जर (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } अन्यथा {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(सत्य);
    }
    शून्य methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        गणना : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        जर (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } अन्यथा जर (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } अन्यथा जर (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } अन्यथा जर (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } अन्यथा जर (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        जर (methodId) {
            AstWith* स्थिरांक withp
                = methodWithClause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            जर (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* स्थिरांक newp
                    = नवीन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                जर (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(असत्य);
                newp->didWidth(सत्य);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
                परत;
            } अन्यथा {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_ओळ* स्थिरांक fl = nodep->fileline();
                Ast_गाठ_अभिव्यक्ती* newp = nullptr;
                साठी (पूर्णांक i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक arrayRef = nodep->fromp()->clone_वृक्ष_Pure(असत्य);
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक selector = नवीन AstArraySel{fl, arrayRef, i};
                    जर (!newp) {
                        newp = selector;
                    } अन्यथा {
                        निवड (methodId) {
                        स्थिती ARRAY_OR: newp = नवीन AstOr{fl, newp, selector}; थांबा;
                        स्थिती ARRAY_AND: newp = नवीन AstAnd{fl, newp, selector}; थांबा;
                        स्थिती ARRAY_XOR: newp = नवीन AstXor{fl, newp, selector}; थांबा;
                        स्थिती ARRAY_SUM: newp = नवीन AstAdd{fl, newp, selector}; थांबा;
                        स्थिती ARRAY_PRODUCT: newp = नवीन AstMul{fl, newp, selector}; थांबा;
                        पूर्वनिर्धारित: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            }
        } अन्यथा जर (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(असत्य);
            newp->didWidth(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    शून्य methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        जर (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* स्थिरांक callp = नवीन AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    शून्य methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        जर (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_गाठ* स्थिरांक newp = नवीन AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        } अन्यथा जर (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } अन्यथा जर (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } अन्यथा जर (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } अन्यथा जर (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } अन्यथा जर (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } अन्यथा जर (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_गाठ* स्थिरांक newp
                = नवीन AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा जर (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_गाठ* स्थिरांक newp
                = नवीन AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा जर (nodep->name() == "compare" || nodep->name() == "icompare") {
            स्थिरांक तार्किक ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* स्थिरांक argp = nodep->argsp();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhs = nodep->fromp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhs = argp->exprp()->unlinkFrBack();
            Ast_गाठ* स्थिरांक newp = नवीन AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा जर (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* स्थिरांक arg0p = nodep->argsp();
            AstArg* स्थिरांक arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_गाठ_Var_Ref* स्थिरांक fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* स्थिरांक varrefp
                = नवीन AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_गाठ* स्थिरांक newp = नवीन AstAssign{
                nodep->fileline(), fromp, नवीन AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ढकला_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } अन्यथा जर (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* स्थिरांक arg0p = nodep->argsp();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->fromp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा जर (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* स्थिरांक arg0p = nodep->argsp();
            AstArg* स्थिरांक arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->fromp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक thsp = arg1p->exprp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा जर (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            जर (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } अन्यथा जर (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } अन्यथा जर (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } अन्यथा जर (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } अन्यथा जर (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_गाठ* स्थिरांक newp
                = नवीन AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_गाठ_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        जर (AstQueueDType* स्थिरांक queuep = m_queueDTypeIndexed[indexDTypep]) {
            परत queuep;
        } अन्यथा {
            AstQueueDType* स्थिरांक newp
                = नवीन AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            परत newp;
        }
    }

    शून्य visit(AstNew* nodep) override {
        जर (nodep->didWidth()) परत;
        AstClass* classp = nullptr;
        तार्किक assign = असत्य;
        जर (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = सत्य;
            Ast_गाठ* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            जर (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                जर (AstClass* स्थिरांक clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* स्थिरांक adtypep
                        = नवीन AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } अन्यथा {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            जर (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, गाठ_DType)
                           ? ("data type "s + VN_AS(warnp, गाठ_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                परत;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            जर (Ast_गाठ_FTask* स्थिरांक ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } अन्यथा {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } अन्यथा {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        जर (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    शून्य visit(AstNewCopy* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        AstClassRefDType* स्थिरांक refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        जर (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            परत;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        जर (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    शून्य visit(AstNewDynamic* nodep) override {
        जर (nodep->didWidthAndSet()) परत;
        AstDynArrayDType* स्थिरांक adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        जर (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            परत;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        जर (!VN_IS(nodep->backp(), गाठ_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            परत;
        }
        nodep->dtypep(adtypep);
        जर (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        जर (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* स्थिरांक consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            जर (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        }
    }

    शून्य visit(Ast_Tagged_अभिव्यक्ती* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    शून्य visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    शून्य visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    शून्य visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    शून्य visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    शून्य visit(AstPattern* nodep) override {
        जर (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            साठी (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = असत्य;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                जर (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), गाठ_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                साठी (Ast_गाठ *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            परत;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = असत्य;
        जर (nodep->didWidthAndSet()) परत;
        UINFO(9, "PATTERN " << nodep);
        जर (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        जर (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_गाठ_DType* dtypep = nodep->dtypep();
        जर (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            जर (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                परत;
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            जर (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                परत;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            साठी (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                स्थिरांक पूर्णांक बार = visitPatMemberRep(patp);
                साठी (पूर्णांक i = 1; i < बार; i++) {
                    AstPatMember* स्थिरांक newp = patp->clone_वृक्ष(असत्य);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            साठी (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                जर (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    पर्यंत (Ast_गाठ_अभिव्यक्ती* स्थिरांक movep = VN_AS(patp->lhssp()->nextp(), गाठ_अभिव्यक्ती)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_गाठ* newkeyp = nullptr;
                        जर (patp->keyp()) newkeyp = patp->keyp()->clone_वृक्ष(सत्य);
                        AstPatMember* स्थिरांक newp
                            = नवीन AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            साठी (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* स्थिरांक nextp = VN_AS(patp->nextp(), PatMember);
                जर (patp->isDefault()) {
                    जर (defaultp) {
                        VL_DO_DANGLING(ढकला_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            पर्यंत (स्थिरांक AstConstDType* स्थिरांक vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            जर (स्वतः* स्थिरांक vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } अन्यथा जर (स्वतः* स्थिरांक vdtypep = VN_CAST(dtypep, गाठ_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } अन्यथा जर (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* स्थिरांक consp = नवीन AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } अन्यथा जर (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* स्थिरांक consp = नवीन AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } अन्यथा जर (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } अन्यथा जर (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } अन्यथा {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            जर (defaultp) VL_DO_DANGLING(ढकला_Deletep(defaultp), defaultp);
        }
    }
    शून्य patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<स्थिरांक AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            स्थिरांक AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            पर्यंत (patp) {
                करा {
                    जर (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        जर (स्थिरांक AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            जर (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                थांबा;
                            } अन्यथा {
                                स्थिरांक std::pair<PatMap::iterator, तार्किक> ret
                                    = patmap.emplace(memp, patp);
                                जर (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } अन्यथा जर (Ast_गाठ_DType* nodedtypep = VN_CAST(patp->keyp(), गाठ_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            स्थिरांक string dtype = nodedtypep->dtypep()->prettyDTypeName(सत्य);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } अन्यथा {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } अन्यथा जर (memp) {
                        // constant expr
                        स्थिरांक std::pair<PatMap::iterator, तार्किक> ret = patmap.emplace(memp, patp);
                        जर (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } अन्यथा {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } पर्यंत (असत्य);

                // Next
                जर (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_गाठ_अभिव्यक्ती* newp = nullptr;
        जर (vdtypep->packed()) {
            साठी (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                स्थिरांक स्वतः it = patmap.find(memp);
                जर (it == patmap.end()) {  // default or default_type assignment
                    जर (AstStructDType* स्थिरांक memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } अन्यथा {
                        AstPatMember* स्थिरांक patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = मूल्य_Concat_pattern_Struct(patp, newp, memp, nodep);
                        जर (patp) VL_DO_DANGLING(ढकला_Deletep(patp), patp);
                    }
                } अन्यथा {  // member assignment
                    newp = मूल्य_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } अन्यथा {  // Unpacked
            AstConsStruct* स्थिरांक consp = नवीन AstConsStruct{nodep->fileline(), vdtypep};
            साठी (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                स्थिरांक स्वतः it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                जर (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    ढकला_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ढकला_Deletep(patp);  // patp used below
                } अन्यथा {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = pattern_Member_मूल्य_Iterate(patp);
                consp->addItemsp(नवीन AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        जर (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_गाठ_अभिव्यक्ती* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_गाठ_अभिव्यक्ती* newp,
                                                 AstPattern* nodep, स्थिरांक DTypeMap& dtypemap) {
        साठी (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            जर (AstStructDType* स्थिरांक memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } अन्यथा {
                AstPatMember* स्थिरांक patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = मूल्य_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                जर (patp) VL_DO_DANGLING(ढकला_Deletep(patp), patp);
            }
        }
        परत newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, स्थिरांक DTypeMap& dtypemap) {
        स्थिरांक string memp_DType = memp->virtRefDTypep()->prettyDTypeName(सत्य);
        स्थिरांक स्वतः it = dtypemap.find(memp_DType);
        जर (it != dtypemap.end()) {
            // default_value for data_type
            परत it->second->clone_वृक्ष(असत्य);
        }
        जर (defaultp) {
            // default_value for any unmatched member yet
            परत defaultp->clone_वृक्ष(असत्य);
        }
        जर (memp->valuep()) {
            परत नवीन AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_वृक्ष(असत्य), गाठ_अभिव्यक्ती),
                                    नवीन AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        परत nullptr;
    }

    Ast_गाठ_अभिव्यक्ती* मूल्य_Concat_pattern_Struct(AstPatMember* patp, Ast_गाठ_अभिव्यक्ती* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        जर (patp) {
            patp->dtypep(memp);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = pattern_Member_मूल्य_Iterate(patp);
            जर (!newp) {
                newp = valuep;
            } अन्यथा {
                AstConcat* स्थिरांक concatp = नवीन AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, सत्य);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, सत्य);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        परत newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_गाठ_DType* elemDTypep,
                                       स्थिरांक DTypeMap& dtypemap = DTypeMap{}) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = defaultp->lhssp()->clone_वृक्ष(असत्य);
        Ast_गाठ_DType* स्थिरांक elemDTypeSkipRefp = elemDTypep->skipRefp();
        स्थिरांक AstStructDType* स्थिरांक structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        स्थिरांक तार्किक unpackedStruct = structp && !structp->packed();

        जर (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
            परत defaultp->clone_वृक्ष(असत्य);
        }
        जर (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
            परत defaultp->clone_वृक्ष(असत्य);
        }
        जर (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        तार्किक wholeElement = unpackedStruct;
        जर (Ast_गाठ_DType* स्थिरांक मूल्य_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !मूल्य_DTypep->skipRefp()->isIntegralOrPacked()
                                          : मूल्य_DTypep->similarDType(elemDTypeSkipRefp);
        }
        जर (wholeElement) {
            VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
            परत defaultp->clone_वृक्ष(असत्य);
        }

        AstPatMember* स्थिरांक nestedDefaultp
            = नवीन AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(सत्य);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        साठी (स्थिरांक स्वतः& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_वृक्ष(असत्य));
        AstPattern* स्थिरांक recursivePatternp = नवीन AstPattern{defaultp->fileline(), nestedDefaultp};
        परत नवीन AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_गाठ_अभिव्यक्ती* patternDefaultValuep(AstPatMember* defaultp, Ast_गाठ_DType* elemDTypep) {
        जर (!defaultp) परत nullptr;
        AstPatMember* स्थिरांक elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = pattern_Member_मूल्य_Iterate(elemDefaultp);
        VL_DO_DANGLING(ढकला_Deletep(elemDefaultp), elemDefaultp);
        परत valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    शून्य patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        स्थिरांक VNumRange range = arrayDtp->declRange();
        स्थिरांक पूर्णांक elements = range.elements();
        AstConsArray* स्थिरांक newp = नवीन AstConsArray{nodep->fileline(), arrayDtp};
        पूर्णांक entn = 0;  // Number of elements given so far, from the left
        तार्किक error = असत्य;  // Reported an error, so the value is not complete
        साठी (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = pattern_Member_मूल्य_Iterate(patp);
            स्थिरांक uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            स्थिरांक AstUnpackArrayDType* स्थिरांक spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            जर (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_मूल्ये(index, valuep);
                सुरूठेवा;
            }
            स्थिरांक VCastable castable = Ast_गाठ::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            जर (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = सत्य;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        जर (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = सत्य;
        }
        जर (error) {
            VL_DO_DANGLING(ढकला_Deletep(newp), newp);
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } अन्यथा {
            nodep->replaceWith(newp);
        }
    }
    शून्य patternArray(AstPattern* nodep, Ast_गाठ_Array_DType* arrayDtp, AstPatMember* defaultp) {
        जर (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            परत;
        }
        स्थिरांक VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.डावा() << " to " << range.उजवा());
        Ast_गाठ* newp = nullptr;
        तार्किक missed = असत्य;  // Some element missed, so not a complete value
        साठी (पूर्णांक entn = 0, ent = range.डावा(); entn < range.elements();
             ++entn, ent += range.डावा_To_उजवा_Inc()) {
            Ast_गाठ_अभिव्यक्ती* valuep = nullptr;
            स्थिरांक स्वतः it = patmap.find(ent);
            जर (it == patmap.end()) {
                जर (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } अन्यथा {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = सत्य;
                }
            } अन्यथा {
                AstPatMember* स्थिरांक patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_मूल्य_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                स्थिरांक AstUnpackArrayDType* स्थिरांक memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                जर (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
                    valuep = नवीन AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            जर (valuep) {
                जर (VN_IS(arrayDtp, UnpackArrayDType)) {
                    जर (!newp) newp = नवीन AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } अन्यथा {  // Packed. Convert to concat for now.
                    जर (!newp) {
                        newp = valuep;
                    } अन्यथा {
                        AstConcat* स्थिरांक concatp
                            = नवीन AstConcat{valuep->fileline(), VN_AS(newp, गाठ_अभिव्यक्ती), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        जर (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        जर (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ढकला_Deletep(newp), newp);
            newp = नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        जर (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        जर (VN_IS(patp->keyp(), गाठ_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            परत nullptr;
        }
        जर (patp->किल्ली_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->किल्ली_Exprp());  // May change
        AstConst* स्थिरांक constp = VN_CAST(patp->किल्ली_Exprp(), Const);
        जर (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        परत constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_गाठ_DType* किल्ली_DTypep) {
        जर (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            परत nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        जर (किल्ली_DTypep->skipRefp()->isIntegralOrPacked() && patp->किल्ली_Exprp()) {
            userIterate(patp->किल्ली_Exprp(), WidthVP{किल्ली_DTypep, PRELIM}.p());
            स्थिरांक पूर्णांक width = std::max(patp->किल्ली_Exprp()->width(), किल्ली_DTypep->width());
            Ast_गाठ_DType* स्थिरांक contextDTypep
                = किल्ली_DTypep->findBitOrLogicDType(width, width, किल्ली_DTypep->numeric(), सत्य);
            iterateCheck(patp, "key", patp->किल्ली_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, असत्य);
        }
        AstConst* स्थिरांक constp = patternConstKeyp(patp);
        जर (!constp) परत nullptr;
        जर (constp->अंक().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            परत nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, किल्ली_DTypep, EXTEND_EXP, असत्य);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक convp = patp->किल्ली_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_गाठ_अभिव्यक्ती* स्थिरांक keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        परत VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_गाठ_अभिव्यक्ती* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_गाठ_DType* arrayDtp) {
        Ast_गाठ_DType* स्थिरांक किल्ली_DTypep = VN_AS(arrayDtp, AssocArrayDType)->किल्ली_DTypep();
        जर (!किल्ली_DTypep->skipRefp()->isAggregateType() || !patp->किल्ली_Exprp()) {
            परत patternScalarKeyp(patp, किल्ली_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->किल्ली_Exprp(), किल्ली_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->किल्ली_Exprp());
        Ast_गाठ_अभिव्यक्ती* स्थिरांक foldedp = patp->किल्ली_Exprp();
        जर (!foldedp->is_Constant_वृक्ष()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            परत nullptr;
        }
        जर (foldedp->exists([](स्थिरांक AstConst* constp) { परत constp->अंक().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            परत nullptr;
        }
        परत foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_गाठ_DType* arrayDtp) {
        परत patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_गाठ_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        परत patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    ढाँचा <typename T_Item, typename T_Cons>
    शून्य sort_Cons_वस्तुसंच(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        पर्यंत (T_Item* स्थिरांक itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.ढकला_back(itemp);
        }
        स्थिरांक स्वतः किल्ली_Of = [](स्थिरांक T_Item* itemp) -> स्थिरांक V3_संख्या& {
            स्थिरांक Ast_गाठ* स्थिरांक keyp = itemp->keyp();
            परत VN_AS(keyp, Const)->अंक();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](स्थिरांक T_Item* ap, स्थिरांक T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            परत किल्ली_Of(ap).isLtXZ(किल्ली_Of(bp));
        });
        स्थिरांक T_Item* firstp = nullptr;  // First item with the current key
        साठी (T_Item* स्थिरांक itemp : itemps) {
            जर (firstp && किल्ली_Of(firstp).isCaseEq(किल्ली_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } अन्यथा {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    ढाँचा <typename T_Item, typename T_Cons>
    शून्य patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_गाठ_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        साठी (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            स्थिरांक स्वतः keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = pattern_Member_मूल्य_Iterate(patp);
            जर (keyp) {
                consp->addItemsp(नवीन T_Item{patp->fileline(), keyp, valuep});
            } अन्यथा {
                VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        स्थिरांक AstAssocArrayDType* स्थिरांक assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        जर (!assocDtp || !assocDtp->किल्ली_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_वस्तुसंच<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_गाठ_अभिव्यक्ती* newConsDynArrayOrQueue(File_ओळ* fileline, स्थिरांक Ast_गाठ_DType* dtypep) {
        जर (VN_IS(dtypep, DynArrayDType)) परत नवीन AstConsDynArray{fileline};
        जर (VN_IS(dtypep, QueueDType)) परत नवीन AstConsQueue{fileline};
        परत nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    ढाँचा <typename T_Item, typename T_Cons>
    शून्य addConsItem(T_Cons* consp, Ast_गाठ_अभिव्यक्ती* valuep) {
        Ast_गाठ_DType* स्थिरांक arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        स्थिरांक तार्किक isElement
            = Ast_गाठ::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        स्थिरांक AstQueueDType* स्थिरांक subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        जर (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* स्थिरांक subp = Ast_गाठ::cast<T_Cons>(valuep);
            जर (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                जर (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(ढकला_Deletep(subp), subp);
                परत;
            }
            जर (AstConsQueue* स्थिरांक queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_वस्तुसंच(consp, queuep);
                परत;
            }
            जर (AstConsDynArray* स्थिरांक dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_वस्तुसंच(consp, dynp);
                परत;
            }
        }
        T_Item* स्थिरांक itemp = नवीन T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        जर (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } अन्यथा {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    शून्य addConsItem(Ast_गाठ_अभिव्यक्ती* consp, Ast_गाठ_अभिव्यक्ती* valuep) {
        जर (AstConsDynArray* स्थिरांक dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } अन्यथा {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    ढाँचा <typename T_Sub>
    शून्य add_Cons_वस्तुसंच(Ast_गाठ_अभिव्यक्ती* consp, T_Sub* subp) {
        पर्यंत (स्वतः* स्थिरांक itemp = subp->itemsp()) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(ढकला_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(ढकला_Deletep(subp), subp);
    }
    शून्य patternDynArrayOrQueue(AstPattern* nodep, Ast_गाठ_DType* arrayp, AstPatMember* defaultp) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        स्थिरांक AstPatMember* स्थिरांक firstp = VN_AS(nodep->itemsp(), PatMember);
        स्थिरांक तार्किक keyed = defaultp || (firstp && firstp->keyp());
        जर (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            परत;
        }
        स्थिरांक अक्षर* स्थिरांक kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        जर (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        साठी (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            जर (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_मूल्य_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    शून्य patternBasic(AstPattern* nodep, Ast_गाठ_DType* vdtypep, AstPatMember* defaultp) {
        स्थिरांक AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        स्थिरांक VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_गाठ_अभिव्यक्ती* newp = nullptr;
        साठी (स्थिरांक पूर्णांक ent : range.seqHiToLo()) {
            Ast_गाठ_अभिव्यक्ती* valuep = nullptr;
            स्थिरांक स्वतः it = patmap.find(ent);
            जर (it == patmap.end()) {
                जर (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } अन्यथा {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } अन्यथा {
                AstPatMember* स्थिरांक patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_मूल्य_Iterate(patp);
            }
            जर (valuep) {  // Packed. Convert to concat for now.
                जर (!newp) {
                    newp = valuep;
                } अन्यथा {
                    AstConcat* स्थिरांक concatp = नवीन AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        जर (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        जर (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_गाठ_अभिव्यक्ती* pattern_Member_मूल्य_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_गाठ_अभिव्यक्ती* valuep = patp->lhssp()->unlinkFrBack();
        जर (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            जर (AstConst* स्थिरांक newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        परत valuep;
    }

    स्थिर शून्य checkEventAssignment(स्थिरांक Ast_गाठ_Assign* स्थिरांक asgnp) {
        string unsupEvtAsgn;
        जर (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        जर (VN_IS(asgnp->rhsp(), CReset)) परत;
        जर (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        जर (!unsupEvtAsgn.empty()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warnMore()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warnMore()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    स्थिर तार्किक usesDynamicScheduler(Ast_गाठ* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        पर्यंत (सत्य) {
            AstVarRef* स्थिरांक vrefp = VN_CAST(nodep, VarRef);
            जर (vrefp) परत usesDynamicScheduler(vrefp);
            जर (VN_IS(nodep, MemberSel)) {
                परत सत्य;
            } अन्यथा जर (Ast_गाठ_Sel* selp = VN_CAST(nodep, गाठ_Sel)) {
                nodep = selp->fromp();
            } अन्यथा {
                परत असत्य;
            }
        }
    }

    स्थिर तार्किक usesDynamicScheduler(AstVarRef* vrefp) {
        परत VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    शून्य visit(AstPatMember* nodep) override {
        Ast_गाठ_DType* स्थिरांक vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    पूर्णांक visitPatMemberRep(AstPatMember* nodep) {
        uint32_t बार = 1;
        जर (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep->repp(), Const);
            जर (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                बार = 0;
            } अन्यथा {
                बार = constp->toUInt();
            }
            जर (बार == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                बार = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_वृक्ष();  // Done with replicate before cloning
        }
        परत बार;
    }

    शून्य visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_गाठ* propp = nodep->propp();
            पर्यंत (AstLogNot* स्थिरांक notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* स्थिरांक untilp = VN_CAST(propp, Until);
            जर (untilp && VN_IS(nodep->backp(), गाठ_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(नवीन AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ढकला_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            जर (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    तार्किक firstNewStatementOkRecurse(Ast_गाठ* nodep) {
        जर (AstVar* स्थिरांक varp = VN_CAST(nodep, Var)) {
            जर (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) परत सत्य;
        }
        जर (AstAssign* स्थिरांक aitemp = VN_CAST(nodep, Assign)) {
            जर (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) परत सत्य;
        }
        जर (AstInitialStaticStmt* स्थिरांक aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            साठी (Ast_गाठ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                जर (!firstNewStatementOkRecurse(stmtp)) परत असत्य;
            }
            परत सत्य;
        }
        जर (AstInitialAutomaticStmt* स्थिरांक aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            साठी (Ast_गाठ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                जर (!firstNewStatementOkRecurse(stmtp)) परत असत्य;
            }
            परत सत्य;
        }
        परत असत्य;
    }

    //--------------------
    // Top levels

    ढाँचा <typename CaseItem>
    शून्य handleCaseType(Ast_गाठ* casep, Ast_गाठ_अभिव्यक्ती* exprp, CaseItem* itemsp) {
        AstAttrOf* स्थिरांक exprap = VN_CAST(exprp, AttrOf);
        जर (!exprap) परत;
        जर (exprap->attrType() != VAttrType::TYPEID) परत;

        स्थिरांक Ast_गाठ_DType* स्थिरांक अभिव्यक्ती_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << अभिव्यक्ती_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_ओळ* स्थिरांक newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, सत्य);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, सत्य);  // Side effect of transform
        casep->fileline(newfl);
        साठी (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाठ::as<CaseItem>(itemp->nextp())) {
            जर (itemp->isDefault()) सुरूठेवा;
            तार्किक hit = असत्य;
            साठी (Ast_गाठ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                स्थिरांक AstAttrOf* स्थिरांक condAttrp = VN_CAST(condp, AttrOf);
                जर (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } अन्यथा {
                    Ast_गाठ_DType* स्थिरांक condDtp = condAttrp->dtypep();
                    जर (Ast_गाठ::computeCastable(अभिव्यक्ती_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = सत्य;
                        थांबा;
                    }
                }
            }
            ढकला_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(नवीन AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(नवीन AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(ढकला_Deletep(exprap), exprap);
    }

    ढाँचा <typename CaseItem>
    शून्य handleCase(Ast_गाठ* casep, Ast_गाठ_अभिव्यक्ती* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_गाठ_DType* subDTypep = exprp->dtypep();
        साठी (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाठ::as<CaseItem>(itemp->nextp())) {
            साठी (Ast_गाठ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                जर (condp->dtypep() == subDTypep) सुरूठेवा;

                जर (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } अन्यथा जर (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } अन्यथा {
                    स्थिरांक पूर्णांक width = std::max(subDTypep->width(), condp->width());
                    स्थिरांक पूर्णांक mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    स्थिरांक तार्किक issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        साठी (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाठ::as<CaseItem>(itemp->nextp())) {
            साठी (Ast_गाठ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    शून्य visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        साठी (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            साठी (Ast_गाठ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    शून्य visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    शून्य visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    शून्य visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        साठी (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            साठी (Ast_गाठ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    शून्य visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_गाठ_DType* स्थिरांक itemDTypep = nodep->findUInt32DType();
        साठी (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            साठी (Ast_गाठ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    शून्य visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    शून्य visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    शून्य visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    शून्य visit(Ast_गाठ_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    शून्य visit(Ast_अभिव्यक्ती_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    शून्य visit(Ast_गाठ_Foreach* nodep) override {
        जर (nodep->didWidth()) परत;
        nodep->didWidth(सत्य);
        स्थिरांक AstForeachHeader* स्थिरांक headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_गाठ_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        साठी (Ast_गाठ *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            स्थिरांक तार्किक empty = VN_IS(argsp, Empty);
            AstVar* स्थिरांक varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            जर (varp) varp->usedLoopIdx(सत्य);
            जर (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                परत;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            जर (VN_IS(fromDtp, गाठ_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } अन्यथा जर (AstBasicDType* स्थिरांक adtypep = VN_CAST(fromDtp, BasicDType)) {
                जर (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                    परत;
                }
            } अन्यथा जर (स्थिरांक AstAssocArrayDType* स्थिरांक adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->किल्ली_DTypep());
            } अन्यथा {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
                परत;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_गाठ* स्थिरांक bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        जर (AstForeach* स्थिरांक loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            परत;
        }
    }

    शून्य visit(Ast_गाठ_Assign* nodep) override {
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
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            जर (स्थिरांक AstIfaceRefDType* स्थिरांक irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                जर (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                    परत;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            userIterateAndNext(nodep->rhsp(), WidthVP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            streamAssignLower(nodep);
            Ast_गाठ_DType* स्थिरांक lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            जर (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                स्थिरांक Ast_गाठ_DType* स्थिरांक lhsDtp = lhsDTypep->skipRefp();
                स्थिरांक Ast_गाठ_DType* स्थिरांक rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                स्थिरांक तार्किक lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                स्थिरांक तार्किक rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                जर (lhsIsUnpackArray && rhsIsUnpackArray) {
                    जर (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_गाठ_Var_Ref, Ast_गाठ_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), गाठ_Var_Ref), VN_CAST(nodep->rhsp(), गाठ_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                जर (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), गाठ_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        जर (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        जर (Ast_गाठ* स्थिरांक controlp = nodep->timingControlp()) {
            जर (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_वृक्ष(), controlp);
            } अन्यथा जर (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } अन्यथा {
                जर (nodep->fileline()->timingOn()) {
                    जर (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } अन्यथा {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_वृक्ष(), controlp);
            }
        }
        जर (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            स्थिरांक Ast_गाठ_DType* स्थिरांक lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            जर (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
                परत;
            }
            AstMethodCall* स्थिरांक newp
                = नवीन AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            परत;
        }
        जर (स्थिरांक AstNewDynamic* स्थिरांक dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            जर (!dynp->rhsp()) {
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } अन्यथा {
                newp = नवीन AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(सत्य);
            newp->protect(असत्य);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        जर (AstCMethodHard* स्थिरांक slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            जर (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } अन्यथा जर (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } अन्यथा जर (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } अन्यथा {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            जर (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_गाठ_अभिव्यक्ती* स्थिरांक lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक msbp = slicep->pinsp()->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* स्थिरांक newp
                    = नवीन AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(सत्य);
                newp->protect(असत्य);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            }
        }

        जर (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    शून्य visit(AstRSRule* nodep) override {
        जर (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    शून्य visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    शून्य visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    स्थिर तार्किक isFormatNonNumericArg(स्थिरांक Ast_गाठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        परत dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, गाठ_UOr_Struct_DType)
                   && !VN_AS(dtypep, गाठ_UOr_Struct_DType)->packed());
    }

    शून्य checkFormatNumericArg(Ast_गाठ* argp, अक्षर ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        जर (std::छोटे(ch) == 'd') परत;
        // In contrast, %x of a class reference, causes errors on several simulators.
        जर (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    शून्य visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    शून्य visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    शून्य visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        जर (m_vup && !m_vup->prelim()) परत;  // Can be called as statement or function
        जर (nodep->didWidthAndSet()) परत;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        जर (!nodep->अभिव्यक्ती_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_गाठ_अभिव्यक्ती* oldExprsp = nodep->exprsp();
        जर (nodep->अभिव्यक्ती_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), गाठ_अभिव्यक्ती);
        }
        जर (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        पर्यंत (Ast_गाठ_अभिव्यक्ती* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), गाठ_अभिव्यक्ती);
            जर (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            जर (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                सुरूठेवा;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_गाठ_DType* स्थिरांक dtypep = argp->dtypep()->skipRefp();
            जर (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } अन्यथा जर (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } अन्यथा जर (isFormatNonNumericArg(dtypep)) {
                जर (AstClassRefDType* स्थिरांक classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    जर (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(सत्य);
                    }
                } अन्यथा {
                    // Class handles inside containers print type names, not their members.
                    Ast_गाठ_DType* गाठ_Dtypep = dtypep;
                    पर्यंत (गाठ_Dtypep) {
                        गाठ_Dtypep = गाठ_Dtypep->skipRefp();
                        जर (Ast_गाठ_UOr_Struct_DType* स्थिरांक uOrStructDTypep
                            = VN_CAST(गाठ_Dtypep, गाठ_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(सत्य);
                        }
                        गाठ_Dtypep = गाठ_Dtypep->subDTypep();
                    }
                }
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } अन्यथा जर (nodep->अभिव्यक्ती_Format_2()) {
                जर (AstEnumDType* स्थिरांक enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    सुरूठेवा;
                }
            }
            जर (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            जर (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } अन्यथा {
                nodep->addExprsp(नवीन AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    शून्य visit(AstCReset* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    शून्य visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    शून्य visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    शून्य visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        जर (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        जर (!m_paramsOnly) {
            nodep->foreach([हे](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    नवीन AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            जर (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            निवड (nodep->displayType()) {
            स्थिती VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); थांबा;
            स्थिती VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); थांबा;
            स्थिती VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); थांबा;
            स्थिती VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); थांबा;
            पूर्वनिर्धारित: UASSERT_OBJ(असत्य, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
        }
    }
    शून्य visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        जर (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    शून्य visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    शून्य visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    शून्य visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    शून्य visit(AstFError* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(AstFEof* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    शून्य visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        जर (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    शून्य visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    शून्य visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    शून्य visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    शून्य visit(AstFGetC* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    शून्य visit(AstFGetS* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(AstFUngetC* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    शून्य visit(AstFRead* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            जर (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            जर (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    शून्य visit(AstFScanF* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(AstSScanF* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(Ast_चळत_Trace_F* nodep) override { nodep->dtypeSetString(); }
    शून्य visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstSystemF* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(Ast_गाठ_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        स्थिरांक Ast_गाठ_DType* subp = nullptr;
        जर (स्थिरांक AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            जर (!adtypep->किल्ली_DTypep()->skipRefp()->basicp()
                || !adtypep->किल्ली_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } अन्यथा जर (स्थिरांक AstUnpackArrayDType* स्थिरांक adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } अन्यथा जर (स्थिरांक AstDynArrayDType* स्थिरांक adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } अन्यथा {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        जर (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstTestPlusArgs* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    शून्य visit(Ast_मूल्य_Plus_Args* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    शून्य visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        जर (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        जर (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        जर (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        जर (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    शून्य visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    शून्य visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    शून्य visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    शून्य visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    शून्य visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    तार्किक pinInstArrayElement(AstPin* nodep, स्थिरांक AstRange* rangep, पूर्णांक idx) {
        Ast_गाठ_DType* स्थिरांक modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_गाठ_DType* स्थिरांक conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        जर (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) परत सत्य;
        // Types don't match, but all dimensions done, still need the usual checks
        जर (!rangep) परत असत्य;

        स्थिरांक AstRange* स्थिरांक subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        पूर्णांक subElems = 1;
        साठी (स्थिरांक AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        स्थिरांक पूर्णांक elements = rangep->elementsConst();
        स्थिरांक पूर्णांक from_डावा = idx / subElems;

        स्थिरांक AstUnpackArrayDType* स्थिरांक arrp = VN_CAST(conDTypep, UnpackArrayDType);
        स्थिरांक uint32_t conDims = conDTypep->dimensions(असत्य).second;
        स्थिरांक uint32_t modDims = modDTypep->dimensions(असत्य).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        जर (arrp && conDims != modDims) {
            जर (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                परत सत्य;
            }
            // ArraySel index is counted from lo
            स्थिरांक पूर्णांक bit = arrp->declRange().ascending() ? from_डावा : elements - 1 - from_डावा;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = VN_AS(nodep->exprp(), गाठ_अभिव्यक्ती)->unlinkFrBack();
            जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ढकला_Deletep(fromp), fromp);
            } अन्यथा {
                AstArraySel* स्थिरांक selp = नवीन AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(सत्य);
                nodep->exprp(selp);
            }
            परत pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        जर (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) परत असत्य;

        // Packed connection
        स्थिरांक पूर्णांक modwidth = modDTypep->width();
        स्थिरांक पूर्णांक conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        जर (conwidth == modwidth) परत असत्य;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        जर (conwidth == modwidth * elements * subElems) {
            जर (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->डावा_Const() << ":"
                                                     << rangep->उजवा_Const() << "]");
            }
            स्थिरांक पूर्णांक selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक fromp = VN_AS(nodep->exprp(), गाठ_अभिव्यक्ती)->unlinkFrBack();
            AstSel* स्थिरांक selp = नवीन AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_डावा), selwidth};
            selp->didWidth(सत्य);
            nodep->exprp(selp);
            परत pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        परत सत्य;
    }

    शून्य visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        जर (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* स्थिरांक concatp = VN_CAST(nodep->exprp(), Replicate);
            स्थिरांक AstConst* स्थिरांक countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            जर (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* स्थिरांक patternp = नवीन AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ढकला_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } अन्यथा जर (!m_paramsOnly) {
            जर (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            जर (!nodep->exprp()) {  // No-connect
                जर (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                परत;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            स्थिरांक VDirection pinDirection = nodep->modVarp()->direction();
            जर (VN_IS(nodep->exprp(), गाठ_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_गाठ_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_गाठ_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            जर (स्थिरांक AstIfaceRefDType* स्थिरांक modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                स्थिरांक AstIfaceRefDType* स्थिरांक conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                जर (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_गाठ_DType* subDTypep = modDTypep;
            स्थिरांक पूर्णांक modwidth = modDTypep->width();
            पूर्णांक conwidth = conDTypep->width();
            जर (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } अन्यथा {
                जर (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    जर (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        जर (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        परत;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                जर (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } अन्यथा जर (nodep->modVarp()->isTristate()) {
                    जर (modwidth != conwidth) {
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
                } अन्यथा जर (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                जर ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                स्थिरांक Ast_गाठ_Array_DType* स्थिरांक अभिव्यक्ती_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                स्थिरांक Ast_गाठ_Array_DType* स्थिरांक modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                जर (अभिव्यक्ती_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } अन्यथा जर ((अभिव्यक्ती_Arrayp && !modArrayp) || (!अभिव्यक्ती_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (अभिव्यक्ती_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } अन्यथा {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_गाठ_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), गाठ_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            जर (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    शून्य visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        जर (!m_paramsOnly) {
            जर (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_संदेश(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            जर (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            जर (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ढकला_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    शून्य visit(AstGatePin* nodep) override {
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            स्थिरांक Ast_गाठ_DType* स्थिरांक pinDTypep = nodep->exprp()->dtypep();
            स्थिरांक पूर्णांक अंक_Insts = nodep->rangep()->elementsConst();
            स्थिरांक पूर्णांक modwidth = अंक_Insts;
            स्थिरांक पूर्णांक pinwidth = pinDTypep->width();
            जर (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_गाठ_DType* स्थिरांक subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_गाठ* स्थिरांक newp
                    = नवीन AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(अंक_Insts)};
                nodep->replaceWith(newp);
            } अन्यथा {
                जर (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                userIterateAndNext(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            }
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }
    शून्य visit(Ast_गाठ_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        जर (nodep->didWidth()) परत;
        जर (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(सत्य);
            nodep->didWidth(सत्य);
            परत;
        }
        जर (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } अन्यथा जर (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        जर (nodep->classMethod() && nodep->name() == "new") {
            जर (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            जर (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_गाठ* firstp = nullptr;
            साठी (Ast_गाठ* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                जर (Ast_Stmt_अभिव्यक्ती* स्थिरांक sep = VN_CAST(itemp, Stmt_अभिव्यक्ती)) {
                    जर (AstNew* स्थिरांक newp = VN_CAST(sep->exprp(), New)) {
                        जर (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            थांबा;
                        }
                    }
                    सुरूठेवा;
                }
                जर (firstNewStatementOkRecurse(itemp)) सुरूठेवा;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(सत्य);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        जर (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        जर (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } अन्यथा जर (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } अन्यथा जर (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        जर (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(सत्य);
        nodep->doingWidth(असत्य);
        जर (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        जर (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](स्थिरांक AstVarRef* varrefp) {
                   परत varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        साठी (Ast_गाठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstVar* स्थिरांक portp = VN_CAST(stmtp, Var)) {
                जर (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } अन्यथा जर (AstAttrOf* स्थिरांक attrp = VN_CAST(stmtp, AttrOf)) {
                जर (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                जर (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        जर (protop) {
            साठी (माप_प्रकार i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                जर (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    थांबा;
                } अन्यथा {
                    AstVar* स्थिरांक portp = ports[i];
                    AstAttrOf* स्थिरांक rprotop = protos[i];
                    Ast_गाठ_DType* स्थिरांक declDtp = portp->dtypep();
                    Ast_गाठ_DType* स्थिरांक protoDtp = rprotop->fromp()->dtypep();
                    जर (portp->name() != rprotop->name()) {
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
                    } अन्यथा जर (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warnMore() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warnMore() << "... Declaration data type: "
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
    शून्य visit(AstConstraint* nodep) override {
        जर (nodep->didWidth()) परत;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(सत्य);
    }
    शून्य visit(AstProperty* nodep) override {
        जर (nodep->didWidth()) परत;
        जर (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(सत्य);
            nodep->didWidth(सत्य);
            परत;
        }
        nodep->doingWidth(सत्य);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        साठी (Ast_गाठ* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            जर (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } अन्यथा जर (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } अन्यथा जर (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } अन्यथा {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(सत्य);
        nodep->doingWidth(असत्य);
    }
    शून्य visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    शून्य visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        जर (nodep->didWidth()) परत;
        nodep->dtypeSetBit();
        साठी (Ast_गाठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } अन्यथा जर (VN_IS(stmtp, गाठ_अभिव्यक्ती)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } अन्यथा {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(सत्य);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_गाठ* pkgItemp) {
        पर्यंत (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        परत VN_CAST(pkgItemp->backp(), Package);
    }
    शून्य visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_गाठ_FTask_Ref*>(nodep));
        जर (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            जर (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_अभिव्यक्ती))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } अन्यथा {
            nodep->dtypeFrom(nodep->taskp());
        }
        जर (nodep->fileline()->timingOn()) {
            Ast_गाठ_Module* स्थिरांक classp = nodep->classOrPackagep();
            जर (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* स्थिरांक packagep = getItemPackage(classp);
                जर (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    स्थिर तार्किक areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        स्थिरांक std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        स्थिरांक std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        जर (dims0.size() != dims1.size()) परत असत्य;
        साठी (माप_प्रकार i = 0; i < dims0.size(); ++i) {
            जर (dims0[i]->elementsConst() != dims1[i]->elementsConst()) परत असत्य;
        }
        परत सत्य;
    }
    // Makes sure that port and pin have same size and same datatype
    शून्य checkUnpackedArrayArgs(AstVar* portp, Ast_गाठ* pinp) {
        जर (AstUnpackArrayDType* स्थिरांक portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            जर (AstUnpackArrayDType* स्थिरांक pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                जर (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                स्थिरांक AstBasicDType* स्थिरांक portBasicp = portDtypep->basicp();
                स्थिरांक AstBasicDType* स्थिरांक pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                स्थिरांक तार्किक sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                जर (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } अन्यथा {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    शून्य processFTaskRefArgs(Ast_गाठ_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        जर (nodep->didWidth()) परत;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        करा {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            स्थिरांक V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            जर (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            साठी (स्थिरांक स्वतः& tconnect : tconnects) {
                स्थिरांक AstVar* स्थिरांक portp = tconnect.first;
                AstArg* स्थिरांक argp = tconnect.second;
                Ast_गाठ_अभिव्यक्ती* pinp = argp->exprp();
                जर (!pinp) सुरूठेवा;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                जर (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_गाठ_अभिव्यक्ती* argsp = nullptr;
                    पर्यंत (AstArg* स्थिरांक nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_वृक्ष();  // Remove the call's Arg wrapper
                    }
                    string format;
                    जर (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->अंक().toString();
                    } अन्यथा {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ढकला_Deletep(argp), argp);
                    AstSFormatF* स्थिरांक newp
                        = नवीन AstSFormatF{nodep->fileline(), format, असत्य, argsp};
                    जर (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(नवीन AstScopeName{newp->fileline(), सत्य});
                    }
                    handle.relink(नवीन AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    जा reloop;  // so exit early; next loop will correct it
                }  //
                अन्यथा जर (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                जर (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } पर्यंत (असत्य);
        // Stage 2
        {
            स्थिरांक V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            साठी (स्थिरांक स्वतः& tconnect : tconnects) {
                AstVar* स्थिरांक portp = tconnect.first;
                स्थिरांक AstArg* स्थिरांक argp = tconnect.second;
                Ast_गाठ_अभिव्यक्ती* स्थिरांक pinp = argp->exprp();
                जर (!pinp) सुरूठेवा;  // Argument error we'll find later
                // Change data types based on above accept completion
                जर (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                जर (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            स्थिरांक V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            साठी (स्थिरांक स्वतः& tconnect : tconnects) {
                स्थिरांक AstVar* स्थिरांक portp = tconnect.first;
                स्थिरांक AstArg* स्थिरांक argp = tconnect.second;
                Ast_गाठ_अभिव्यक्ती* स्थिरांक pinp = argp->exprp();
                जर (!pinp) सुरूठेवा;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        जर (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            स्थिरांक V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            साठी (स्थिरांक स्वतः& tconnect : tconnects) {
                स्थिरांक AstVar* स्थिरांक portp = tconnect.first;
                स्थिरांक AstArg* स्थिरांक argp = tconnect.second;
                Ast_गाठ_अभिव्यक्ती* pinp = argp->exprp();
                जर (!pinp) सुरूठेवा;  // Argument error we'll find later
                Ast_गाठ_DType* स्थिरांक portDTypep = portp->dtypep()->skipRefToEnump();
                जर (VN_IS(pinp, EmptyQueue)) {
                    Ast_गाठ_अभिव्यक्ती* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    जर (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = नवीन AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(pinp), pinp);
                    pinp = newp;
                }
                स्थिरांक Ast_गाठ_DType* स्थिरांक pinDTypep = pinp->dtypep()->skipRefToEnump();
                स्थिरांक AstIfaceRefDType* स्थिरांक portIfacep
                    = VN_CAST(portDTypep->elemDTypep(सत्य), IfaceRefDType);
                स्थिरांक VCastable ifaceCastable
                    = portIfacep ? Ast_गाठ::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ सत्य)
                                 : VCastable{VCastable::UNSUPPORTED};
                स्थिरांक तार्किक matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                स्थिरांक तार्किक pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->अंक().isNull();
                जर ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    जर (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } अन्यथा {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } अन्यथा जर (portIfacep && portIfacep->isVirtual() && portp->is_आदान() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } अन्यथा जर (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                जर (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    सुरूठेवा;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                जर (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } अन्यथा जर (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } अन्यथा {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    शून्य handleStdRandomizeArgs(Ast_गाठ_FTask_Ref* स्थिरांक nodep) {
        AstConst* nullp = nullptr;
        साठी (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = argp->exprp();
            जर (AstConst* स्थिरांक constp = VN_CAST(exprp, Const)) {
                जर (constp->अंक().isNull()) {
                    nullp = constp;
                    सुरूठेवा;
                }
            }
            जर (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                सुरूठेवा;
            }
            जर (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                सुरूठेवा;
            }
            जर (स्थिरांक AstCMethodHard* स्थिरांक methodp = VN_CAST(exprp, CMethodHard)) {
                जर (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    सुरूठेवा;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        जर (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    शून्य visit(Ast_गाठ_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        जर (nodep->didWidth()) परत;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        जर (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(सत्य);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            परत;  // Handled in V3Randomize
        } अन्यथा जर (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* स्थिरांक classp = VN_CAST(nodep->classOrPackagep(), Class);
            जर (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(सत्य);
                Ast_गाठ_DType* स्थिरांक adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, असत्य, असत्य, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                साठी (स्थिरांक AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                जर (withp) nodep->withp(withp);
                nodep->didWidth(सत्य);
                परत;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            जर (nodep->name() == "randomize") {
                AstClassRefDType* स्थिरांक adtypep
                    = नवीन AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, असत्य, असत्य, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                साठी (स्थिरांक AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } अन्यथा जर (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } अन्यथा जर (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(सत्य);
                v3Global.useRandomizeMethods(सत्य);
                AstCExpr* स्थिरांक newp
                    = नवीन AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            } अन्यथा जर (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(सत्य);
                v3Global.useRandomizeMethods(सत्य);
                AstCExpr* स्थिरांक newp
                    = नवीन AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            } अन्यथा {
                UASSERT_OBJ(असत्य, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        जर (nodep->didWidth()) {
            जर (withp) nodep->withp(withp);
            परत;
        }
        जर ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            तार्किक allow = असत्य;
            जर (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                जर (स्थिरांक AstFuncRef* स्थिरांक funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } अन्यथा जर (स्थिरांक AstTaskRef* स्थिरांक taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                जर (!allow) {
                    स्थिरांक AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    जर (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    स्थिरांक AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    जर (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            जर (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                स्थिरांक AstClass* स्थिरांक cgClassp = m_containingClassFinder.find(nodep);
                जर (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    स्थिरांक AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    जर (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            जर (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        जर (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(नवीन AstScopeName{nodep->fileline(), असत्य});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        जर (withp) nodep->withp(withp);
        nodep->didWidth(सत्य);
        // See steps that follow in visit(AstFuncRef*)
    }
    शून्य visit(Ast_गाठ_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    शून्य visit(AstSenItem* nodep) override {
        जर (nodep->isComboStar()) परत;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        जर (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            स्वतः* स्थिरांक concatOrReplp = VN_CAST(nodep->sensp(), गाठ_Biop);
            स्वतः* स्थिरांक rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            जर (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                स्वतः* स्थिरांक lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(नवीन AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(नवीन AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
        } अन्यथा {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            जर (nodep->edgeType().anEdge()) {
                Ast_गाठ_DType* स्थिरांक sensDtp = nodep->sensp()->dtypep()->skipRefp();
                जर (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } अन्यथा जर (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    शून्य visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* स्थिरांक refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    शून्य visit(AstWait* nodep) override {
        जर (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_वृक्ष(), nodep);
            परत;
        }
        जर (nodep->fileline()->timingOn()) {
            जर (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                जर (Ast_गाठ_Var_Ref* स्थिरांक varrefp = VN_CAST(nodep->condp(), गाठ_Var_Ref)) {
                    जर (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                परत;
            } अन्यथा जर (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } अन्यथा {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        जर (Ast_गाठ* स्थिरांक stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }
    शून्य visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_गाठ_DType* स्थिरांक vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->मूल्य_Arg_Refp(), nullptr);
        जर (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } अन्यथा {
            जर (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } अन्यथा {
                जर (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } अन्यथा {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            जर (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } अन्यथा {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    शून्य visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        जर (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } अन्यथा {
            nodep->dtypeFrom(m_withp->मूल्य_Arg_Refp());
        }
    }
    शून्य visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    शून्य visit(Ast_गाठ_अभिव्यक्ती* nodep) override {
        जर (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    शून्य visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        जर (nodep->didWidthAndSet()) परत;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        जर (Ast_गाठ_DType* स्थिरांक vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } अन्यथा जर (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->width() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->width() << " bits -> " << nodep->width()
                                    << " bits" << endl);

        // Final processing
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, FINAL}.p());
        nodep->didWidth(सत्य);
    }
    शून्य visitClass(AstClass* nodep) {
        जर (nodep->didWidthAndSet()) परत;

        // If the class is std::process
        जर (nodep->name() == "process") {
            AstPackage* स्थिरांक packagep = getItemPackage(nodep);
            जर (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                जर (AstVar* स्थिरांक varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_गाठ_DType* स्थिरांक dtypep = varp->getChildDTypep();
                    जर (!varp->dtypep()) {
                        VL_DO_DANGLING(ढकला_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* स्थिरांक newdtypep = नवीन AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                जर (Ast_गाठ_FTask* स्थिरांक ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), गाठ_FTask)) {
                    ftaskp->setNeedProcess();
                    v3Global.setUsesTiming();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        userIterateAndNext(nodep->extendsp(), nullptr);
        userIterateChildren(nodep, nullptr);  // First size all members
    }
    शून्य visit(Ast_गाठ_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        जर (AstClass* स्थिरांक classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } अन्यथा {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([हे](स्थिरांक AstVar* varp) { m_curModVars.घाला(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    शून्य visit(Ast_गाठ* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    शून्य visit_Or_Lu64(Ast_गाठ_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_गाठ_DType* स्थिरांक subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    शून्य visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            जर (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    नवीन AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
            }
        }
    }
    शून्य visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    शून्य visit_Os32_Lr(Ast_गाठ_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    शून्य visit_Ou64_Lr(Ast_गाठ_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    शून्य visit_log_not(AstLogNot* nodep) {
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
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            जर (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* स्थिरांक newp
                    = नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            }
        }
    }
    शून्य visit_log_and_or(Ast_गाठ_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_गाठ_अभिव्यक्ती::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    शून्य visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit_red_and_or(Ast_गाठ_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    शून्य visit_red_unknown(Ast_गाठ_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    तार्किक isEquivalentDType(स्थिरांक Ast_गाठ_DType* lhs, स्थिरांक Ast_गाठ_DType* rhs) {
        // a) If two types match, they are equivalent.
        जर (!lhs || !rhs) परत असत्य;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        जर (lhs == rhs) परत सत्य;
        // If both are basic types, check if they are the same type
        जर (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            स्थिरांक स्वतः* lb = VN_CAST(lhs, BasicDType);
            स्थिरांक स्वतः* rb = VN_CAST(rhs, BasicDType);
            जर (lb->isString() != rb->isString()) परत असत्य;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        स्थिरांक तार्किक lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        स्थिरांक तार्किक rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        जर (lhsIsUnpackArray || rhsIsUnpackArray) {
            जर (lhsIsUnpackArray && rhsIsUnpackArray) {
                स्थिरांक AstUnpackArrayDType* स्थिरांक lhsp = VN_CAST(lhs, UnpackArrayDType);
                स्थिरांक AstUnpackArrayDType* स्थिरांक rhsp = VN_CAST(rhs, UnpackArrayDType);
                स्थिरांक पूर्णांक lsz = lhsp->elementsConst();
                स्थिरांक पूर्णांक rsz = rhsp->elementsConst();
                जर (lsz >= 0 && rsz >= 0 && lsz != rsz) परत असत्य;
                परत isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            परत असत्य;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        स्थिरांक तार्किक lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        स्थिरांक तार्किक rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        स्थिरांक तार्किक lhsIsQueue = VN_IS(lhs, QueueDType);
        स्थिरांक तार्किक rhsIsQueue = VN_IS(rhs, QueueDType);
        स्थिरांक तार्किक lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        स्थिरांक तार्किक rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        जर (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            जर (स्थिरांक AstDynArrayDType* स्थिरांक lhsp = VN_CAST(lhs, DynArrayDType)) {
                जर (स्थिरांक AstDynArrayDType* स्थिरांक rhsp = VN_CAST(rhs, DynArrayDType)) {
                    परत isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            जर (स्थिरांक AstQueueDType* स्थिरांक lhsp = VN_CAST(lhs, QueueDType)) {
                जर (स्थिरांक AstQueueDType* स्थिरांक rhsp = VN_CAST(rhs, QueueDType)) {
                    परत isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            जर (स्थिरांक AstAssocArrayDType* स्थिरांक lhsp = VN_CAST(lhs, AssocArrayDType)) {
                जर (स्थिरांक AstAssocArrayDType* स्थिरांक rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    परत isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->किल्ली_DTypep(), rhsp->किल्ली_DTypep());
                }
            }

            परत असत्य;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        जर (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            जर (lhs->width() != rhs->width()) परत असत्य;
            जर (lhs->isFourstate() != rhs->isFourstate()) परत असत्य;
            जर (lhs->isSigned() != rhs->isSigned()) परत असत्य;
            परत सत्य;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        जर (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) परत असत्य;
        परत !(VN_IS(lhs, गाठ_UOr_Struct_DType) || VN_IS(rhs, गाठ_UOr_Struct_DType));
    }

    स्थिर तार्किक isAggregateType(स्थिरांक Ast_गाठ* nodep) {
        जर (!nodep) परत असत्य;
        स्थिरांक Ast_गाठ_DType* dtypep = nodep->dtypep();
        जर (!dtypep) परत असत्य;
        dtypep = dtypep->skipRefp();
        जर (!dtypep) परत असत्य;
        परत dtypep->isAggregateType();
    }

    शून्य visit_cmp_eq_gt(Ast_गाठ_Biop* nodep, स्थिरांक ComparisonType cmpType) {
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
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            स्थिरांक तार्किक lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            स्थिरांक तार्किक rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            जर (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } अन्यथा जर (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } अन्यथा {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            स्थिरांक तार्किक isAggrLhs = isAggregateType(nodep->lhsp());
            स्थिरांक तार्किक isAggrRhs = isAggregateType(nodep->rhsp());

            जर ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                स्थिरांक Ast_गाठ_DType* स्थिरांक lhsDType = nodep->lhsp()->dtypep();
                स्थिरांक Ast_गाठ_DType* स्थिरांक rhsDType = nodep->rhsp()->dtypep();

                जर (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_गाठ* स्थिरांक newp
                        = नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } अन्यथा जर (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                जर (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_गाठ* स्थिरांक newp
                        = नवीन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                    परत;
                }
                जर (Ast_गाठ_Biop* स्थिरांक newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } अन्यथा जर (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                जर (Ast_गाठ_Biop* स्थिरांक newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } अन्यथा {
                स्थिरांक तार्किक signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                जर (Ast_गाठ_Biop* स्थिरांक newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                स्थिरांक पूर्णांक width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                स्थिरांक पूर्णांक ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                तार्किक warnOn = सत्य;
                जर (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    जर ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = असत्य;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    जर ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = असत्य;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->findBitOrLogicDType(width, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->findBitOrLogicDType(width, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtypeSetBitOrLogic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::NORMAL_EQ
                                               || cmpType == ComparisonType::NORMAL_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    शून्य visit_cmp_real(Ast_गाठ_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit_cmp_string(Ast_गाठ_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    शून्य visit_cmp_type(Ast_गाठ_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            स्थिरांक AstAttrOf* स्थिरांक lhsap = VN_AS(nodep->lhsp(), AttrOf);
            स्थिरांक AstAttrOf* स्थिरांक rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            स्थिरांक Ast_गाठ_DType* स्थिरांक lhsDtp = lhsap->dtypep();
            स्थिरांक Ast_गाठ_DType* स्थिरांक rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            स्थिरांक तार्किक invert = VN_IS(nodep, NeqT);
            स्थिरांक तार्किक identical
                = Ast_गाठ::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            स्थिरांक तार्किक eq = invert ^ identical;
            Ast_गाठ* स्थिरांक newp = नवीन AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        }
    }

    शून्य visit_negate_not(Ast_गाठ_Uniop* nodep, तार्किक real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            जर (!real_ok) checkCvtUS(nodep->lhsp(), असत्य);
        }
        जर (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            जर (Ast_गाठ_Uniop* स्थिरांक newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                परत;
            }
        } अन्यथा {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        जर (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        जर (m_vup->final()) {
            Ast_गाठ_DType* स्थिरांक expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_गाठ_DType* स्थिरांक subDTypep = expDTypep;
            // Some warning suppressions
            तार्किक lhsWarn = सत्य;
            जर (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                जर (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = असत्य;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    शून्य visit_signed_unsigned(Ast_गाठ_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), सत्य);
            स्थिरांक पूर्णांक width = nodep->lhsp()->width();
            Ast_गाठ_DType* स्थिरांक expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_गाठ_DType* स्थिरांक subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    शून्य visit_shift(Ast_गाठ_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        स्थिरांक Ast_गाठ_Biop* स्थिरांक newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (शून्य)newp;  // Ununused
    }
    शून्य iterate_shift_prelim(Ast_गाठ_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), असत्य);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            जर (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_गाठ_Biop* iterate_shift_final(Ast_गाठ_Biop* nodep) {
        // Nodep maybe edited
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->final()) {
            Ast_गाठ_DType* स्थिरांक expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाठ_DType* स्थिरांक subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            जर (VN_IS(nodep, ShiftRS)) {
                जर (Ast_गाठ_Biop* स्थिरांक newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            तार्किक warnOn = सत्य;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            जर (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), गाठ_Assign)) warnOn = असत्य;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* स्थिरांक shiftp = VN_CAST(nodep->rhsp(), Const);
            जर (shiftp && !shiftp->अंक().isFourState() && shiftp->width() <= 32) {
                स्थिरांक int64_t shiftVal = shiftp->अंक().toSQuad();
                जर (VN_IS(nodep, ShiftL)) {
                    जर (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = असत्य;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            जर (nodep->rhsp()->width() > 32) {
                जर (shiftp && shiftp->अंक().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_संख्या अंक{shiftp, 32, shiftp->अंक()};
                    Ast_गाठ* स्थिरांक shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(नवीन AstConst{shiftrhsp->fileline(), अंक});
                    VL_DO_DANGLING(shiftrhsp->delete_वृक्ष(), shiftrhsp);
                }
            }
        }
        परत nodep;  // May edit
    }

    शून्य visit_boolexpr_and_or(Ast_गाठ_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), असत्य);
            checkCvtUS(nodep->rhsp(), असत्य);
            स्थिरांक पूर्णांक width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            स्थिरांक पूर्णांक mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            स्थिरांक तार्किक expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_गाठ_अभिव्यक्ती::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        जर (m_vup->final()) {
            Ast_गाठ_DType* स्थिरांक expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाठ_DType* स्थिरांक subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    शून्य visit_add_sub_replace(Ast_गाठ_Biop* nodep, तार्किक real_ok, स्थिरांक तार्किक modOrDiv) {
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
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            जर (!real_ok) {
                checkCvtUS(nodep->lhsp(), असत्य);
                checkCvtUS(nodep->rhsp(), असत्य);
            }
            जर (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                जर (Ast_गाठ_Biop* स्थिरांक newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                परत;
            } अन्यथा जर (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत;
            } अन्यथा {
                स्थिरांक पूर्णांक width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                स्थिरांक पूर्णांक mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                स्थिरांक तार्किक expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_गाठ_अभिव्यक्ती::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        जर (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_गाठ_DType* स्थिरांक expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_गाठ_DType* स्थिरांक subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            जर (Ast_गाठ_Biop* स्थिरांक newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            तार्किक lhsWarn = सत्य;
            तार्किक rhsWarn = सत्य;
            जर (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                जर (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = असत्य;
                जर (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = असत्य;
                जर (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = असत्य;  // do_increment + ...
                जर (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = असत्य;  // ... + do_increment
            } अन्यथा जर (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                जर (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = असत्य;
                जर (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = असत्य;
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
    शून्य visit_real_add_sub(Ast_गाठ_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    शून्य visit_real_neg_ceil(Ast_गाठ_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_अभिव्यक्ती(nodep);
        जर (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    शून्य visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->अभिव्यक्ती_Format_2(), nodep, "Assumes constant format");
        तार्किक inPct = असत्य;
        Ast_गाठ_अभिव्यक्ती* argp = nodep->exprsp();
        string newFormat;
        साठी (अक्षर ch : nodep->text()) {
            जर (!inPct && ch == '%') {
                inPct = सत्य;
                newFormat += ch;
            } अन्यथा जर (inPct && (std::है_अंक(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } अन्यथा जर (!inPct) {  // Normal text
                newFormat += ch;
            } अन्यथा {
                inPct = असत्य;
                Ast_गाठ_अभिव्यक्ती* स्थिरांक nextp = argp ? VN_AS(argp->nextp(), गाठ_अभिव्यक्ती) : nullptr;
                AstSFormatArg* स्थिरांक fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_गाठ_अभिव्यक्ती* subargp = fargp ? fargp->exprp() : argp;
                स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::छोटे(ch);
                निवड (ch) {
                स्थिती '?':  // Unspecified by user, guess
                    जर (dtypep->isDouble()) {
                        ch = 'g';
                    } अन्यथा जर (dtypep->isString()) {
                        ch = 's';
                    } अन्यथा जर (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } अन्यथा {
                        ch = 'p';
                    }
                    argp = nextp;
                    थांबा;
                स्थिती '%': थांबा;  // %% - just output a %
                स्थिती 'm': थांबा;  // %m - auto insert "name"
                स्थिती 'l': थांबा;  // %m - auto insert "library"
                स्थिती 'd':
                    जर (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        जर (subargp->isDouble()) spliceCvtS(subargp, सत्य, 64);
                        argp = nextp;
                    }
                    थांबा;
                स्थिती 'b':  // FALLTHRU
                स्थिती 'o':  // FALLTHRU
                स्थिती 'x':
                    जर (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    थांबा;
                स्थिती 't':  // Decimal/float time
                    जर (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    थांबा;
                स्थिती 'e':  // FALLTHRU
                स्थिती 'f':  // FALLTHRU
                स्थिती 'g':
                    जर (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        जर (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    थांबा;
                स्थिती 'p':  // FALLTHRU
                स्थिती 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    जर (subargp && !fargp) {
                        जर (AstEnumDType* स्थिरांक enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    थांबा;
                पूर्वनिर्धारित:  // Most operators, just move to next argument
                    argp = nextp;
                    थांबा;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    स्थिर AstEnumDType* formatEnumDType(Ast_गाठ_अभिव्यक्ती* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        जर (!enumDtp) {
            जर (स्थिरांक AstVarRef* स्थिरांक varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        परत enumDtp;
    }

    Ast_अभिव्यक्ती_Stmt* newEnumCapture(Ast_गाठ_अभिव्यक्ती* valuep, AstEnumDType* dtypep) {
        File_ओळ* स्थिरांक flp = valuep->fileline();
        AstVar* स्थिरांक varp = नवीन AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(सत्य);
        जर (m_ftaskp) {
            varp->funcLocal(सत्य);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } अन्यथा {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_अभिव्यक्ती_Stmt* स्थिरांक exprp = नवीन Ast_अभिव्यक्ती_Stmt{
            flp, नवीन AstAssign{flp, नवीन AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            नवीन AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        परत exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_गाठ_अभिव्यक्ती* valuep, AstEnumDType* dtypep) {
        जर (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_गाठ_अभिव्यक्ती* keyp;
        जर (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_वृक्ष(असत्य);
        } अन्यथा {
            Ast_अभिव्यक्ती_Stmt* स्थिरांक capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_वृक्ष_Pure(असत्य);
        }
        स्थिरांक VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* स्थिरांक argp = नवीन AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        परत argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    तार्किक widthBad(Ast_गाठ* nodep, Ast_गाठ_DType* expDTypep) {
        स्थिरांक पूर्णांक expWidth = expDTypep->width();
        पूर्णांक expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        जर (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) परत असत्य;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        जर (expWidthMin == 0) expWidthMin = expWidth;
        जर (nodep->dtypep()->width() == expWidth) परत असत्य;
        जर (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) परत सत्य;
        जर (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) परत सत्य;
        परत असत्य;
    }

    शून्य fixWidthExtend(Ast_गाठ_अभिव्यक्ती* nodep, Ast_गाठ_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<पूर्णांक>(extendRule) << ") old: " << nodep);
        जर (extendRule == EXTEND_OFF) परत;
        AstConst* स्थिरांक constp = VN_CAST(nodep, Const);
        स्थिरांक पूर्णांक expWidth = expDTypep->width();
        जर (constp && !constp->अंक().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_संख्या अंक{nodep, expWidth, constp->अंक()};
            अंक.isSigned(असत्य);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstConst{nodep->fileline(), अंक};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ढकला_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } अन्यथा जर (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(सत्य);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } अन्यथा {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            तार्किक doSigned = असत्य;
            निवड (extendRule) {
            स्थिती EXTEND_ZERO: doSigned = असत्य; थांबा;
            स्थिती EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); थांबा;
            स्थिती EXTEND_LHS: doSigned = nodep->isSigned(); थांबा;
            पूर्वनिर्धारित: nodep->v3fatalSrc("bad case");
            }
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp
                = (doSigned ? static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_गाठ_अभिव्यक्ती*>(नवीन AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(सत्य);
            linker.relink(newp);
            nodep = newp;
        }
        जर (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = spliceCvtD(nodep);
            nodep = newp;
        }
        जर (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } अन्यथा {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    शून्य fixWidthReduce(Ast_गाठ_अभिव्यक्ती* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        स्थिरांक पूर्णांक expWidth = 1;
        स्थिरांक पूर्णांक expSigned = असत्य;
        जर (nodep->width() == expWidth) परत;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* स्थिरांक constp = VN_CAST(nodep, Const);
        जर (constp) {
            V3_संख्या अंक(nodep, expWidth);
            अंक.opRedOr(constp->अंक());
            अंक.isSigned(expSigned);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstConst{nodep->fileline(), अंक};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } अन्यथा {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_गाठ_अभिव्यक्ती* newp;
            Ast_गाठ_DType* स्थिरांक dtypeSkipp = nodep->dtypep()->skipRefp();
            जर (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = नवीन AstNeq{nodep->fileline(),
                                  नवीन AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } अन्यथा {
                // But, AstRedOr can be faster on wide
                newp = नवीन AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    तार्किक fixAutoExtend(Ast_गाठ_अभिव्यक्ती*& nodepr, पूर्णांक expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        जर (AstConst* स्थिरांक constp = VN_CAST(nodepr, Const)) {
            जर (constp->अंक().autoExtend() && !constp->अंक().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_संख्या अंक(constp, expWidth);
                अंक.opRepl(constp->अंक(), expWidth);  // {width{'1}}
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstConst{constp->fileline(), अंक};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                परत सत्य;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            अन्यथा जर (!constp->अंक().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->अंक().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_संख्या अंक(constp, expWidth);
                अंक.opExtendXZ(constp->अंक(), constp->width());
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstConst{constp->fileline(), अंक};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_वृक्ष(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                परत सत्य;
            }
        }
        परत असत्य;  // No change
    }
    तार्किक isBaseClassRecurse(स्थिरांक AstClass* स्थिरांक cls1p, स्थिरांक AstClass* स्थिरांक cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        जर (cls1p == cls2p) परत सत्य;
        साठी (स्थिरांक AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            जर (isBaseClassRecurse(cls1p, cextp->classp())) परत सत्य;
        }
        परत असत्य;
    }
    // Lines to end a message on nodep with the data types of its operands
    स्थिर string warnDTypes(स्थिरांक Ast_गाठ* nodep, स्थिरांक Ast_गाठ_DType* lhsDtp,
                             स्थिरांक Ast_गाठ_DType* rhsDtp, स्थिरांक string& lhsName = "Left-hand",
                             स्थिरांक string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        जर (lhsState == rhsState) lhsState = rhsState = "";
        परत "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    स्थिर तार्किक unpackedArraySizesMatch(स्थिरांक Ast_गाठ_DType* lhsDtp, स्थिरांक Ast_गाठ_DType* rhsDtp) {
        पर्यंत (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            स्थिरांक AstUnpackArrayDType* स्थिरांक lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            स्थिरांक AstUnpackArrayDType* स्थिरांक rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            जर (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                परत असत्य;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        परत सत्य;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    ढाँचा <typename T, typename N>
    शून्य checkUnpackedArrayAssignmentCompatible(स्थिरांक Ast_गाठ* nodep, स्थिरांक T* स्थिरांक lhsRefp,
                                                स्थिरांक N* स्थिरांक rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::मूल्य || std::is_same<T, Ast_गाठ_Var_Ref>::मूल्य)
                && (std::is_same<N, AstVar>::मूल्य || std::is_same<N, Ast_गाठ_Var_Ref>::मूल्य),
            "Unsupported types provided.");
        जर (!lhsRefp || !rhsRefp) परत;
        std::string lhsName;
        std::string rhsName;
        जर (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } अन्यथा {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        स्थिरांक Ast_गाठ_DType* स्थिरांक lhsDtp = lhsRefp->dtypep()->skipRefp();
        स्थिरांक Ast_गाठ_DType* स्थिरांक rhsDtp = rhsRefp->dtypep()->skipRefp();
        स्थिरांक तार्किक isLhsAggregate = lhsDtp->isAggregateType();
        स्थिरांक तार्किक isRhsAggregate = rhsDtp->isAggregateType();
        जर (!isLhsAggregate && !isRhsAggregate) परत;
        जर (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            परत;
        } अन्यथा जर (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            परत;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(असत्य),
                                      rhsDim = rhsDtp->dimensions(असत्य);
        // Check if unpacked array dimensions are matching
        जर (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            परत;
        }

        स्थिरांक Ast_गाठ_DType* lhsDtpIterp = lhsDtp;
        स्थिरांक Ast_गाठ_DType* rhsDtpIterp = rhsDtp;
        साठी (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            जर (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                परत;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        जर (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    शून्य checkClassAssign(स्थिरांक Ast_गाठ* nodep, स्थिरांक अक्षर* side, Ast_गाठ* rhsp,
                          Ast_गाठ_DType* स्थिरांक lhsDTypep, तार्किक isOutputArg = असत्य) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        स्थिरांक Ast_गाठ_DType* स्थिरांक lhsRawDTypep = lhsDTypep->skipRefp();
        स्थिरांक Ast_गाठ_DType* स्थिरांक rhsRawDTypep = rhsp->dtypep()->skipRefp();
        जर (स्थिरांक AstClassRefDType* स्थिरांक lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            जर (स्थिरांक AstClassRefDType* स्थिरांक rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                स्थिरांक AstClass* स्थिरांक dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                स्थिरांक AstClass* स्थिरांक srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                जर (isBaseClassRecurse(dstp, srcp)) परत;
            } अन्यथा जर (rhsp->isNull()) {
                परत;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } अन्यथा जर (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        जर (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                नवीन AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, गाठ_अभिव्यक्ती), lhsDTypep});
        }
    }
    स्थिर तार्किक similarDTypeRecurse(स्थिरांक Ast_गाठ_DType* स्थिरांक node1p,
                                    स्थिरांक Ast_गाठ_DType* स्थिरांक node2p) {
        परत node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    शून्य iterateCheckFileDesc(Ast_गाठ* parentp, Ast_गाठ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_गाठ_DType* स्थिरांक expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckReal(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    शून्य iterateCheckSigned8(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    शून्य iterateCheckSigned32(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    शून्य iterateCheckUInt32(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    शून्य iterateCheckDelay(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        जर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        जर (stage & FINAL) {
            Ast_गाठ_DType* expDTypep;
            जर (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } अन्यथा {
                File_ओळ* स्थिरांक newFl = नवीन File_ओळ{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, सत्य);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, असत्य);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckString(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    शून्य iterateCheckTyped(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp,
                           Ast_गाठ_DType* expDTypep, Stage stage) {
        जर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        जर (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckTypedSelfPrelim(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp,
                                     Ast_गाठ_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        जर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        जर (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckIntegralSelf(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, सत्य);
    }
    शून्य iterateCheckSelf(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Determ determ,
                          Stage stage, तार्किक integralOnly = असत्य) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        जर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, गाठ_अभिव्यक्ती) ? checkCvtUS(VN_AS(underp, गाठ_अभिव्यक्ती), integralOnly) : underp;
        Ast_गाठ_DType* स्थिरांक expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckSizedSelf(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Determ determ,
                               Stage stage, तार्किक deferUntyped = असत्य) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        जर (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, गाठ_अभिव्यक्ती) ? checkCvtUS(VN_AS(underp, गाठ_अभिव्यक्ती), असत्य) : underp;
        Ast_गाठ_DType* स्थिरांक expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_गाठ_DType* स्थिरांक checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        जर (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckAssign(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* rhsp, Stage stage,
                            Ast_गाठ_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        जर (AstConst* constp = VN_CAST(rhsp, Const)) {
            जर (स्थिरांक AstUnpackArrayDType* स्थिरांक arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                जर (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    जर (basicp->width() == 8 && constp->अंक().isFromString()) {
                        AstConsArray* newp = नवीन AstConsArray{
                            constp->fileline(), lhsDTypep,
                            नवीन AstConst{constp->fileline(), AstConst::Widthed_मूल्य{}, 8, 0}};
                        साठी (स्थिरांक पूर्णांक aindex : arrayp->declRange().seqLoToHi()) {
                            पूर्णांक cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_संख्या selected{constp, 8};
                            selected.opSel(constp->अंक(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            जर (!selected.isFourState()) {
                                जर (स्थिरांक uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 नवीन AstConst{constp->fileline(),
                                                              AstConst::Widthed_मूल्य{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ढकला_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        स्थिरांक तार्किक lhsStream = (VN_IS(parentp, गाठ_Assign)
                                && VN_IS(VN_AS(parentp, गाठ_Assign)->lhsp(), गाठ_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, सत्य, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (शून्य)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_गाठ_अभिव्यक्ती* convertUnpackedOrder(Ast_गाठ_अभिव्यक्ती* exprp, Ast_गाठ_DType* dtypep) {
        स्थिरांक AstUnpackArrayDType* स्थिरांक arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        जर (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) परत exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        परत newp;
    }

    स्थिर तार्किक isLogicalOperator(स्थिरांक Ast_गाठ* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        परत VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    शून्य iterateCheckBool(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_गाठ_DType* स्थिरांक underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        जर (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_गाठ* स्थिरांक newp
                = नवीन AstNeqD{parentp->fileline(), VN_AS(underp, गाठ_अभिव्यक्ती),
                              नवीन AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } अन्यथा जर (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_गाठ* स्थिरांक newp
                = नवीन AstNeqN{parentp->fileline(), VN_AS(underp, गाठ_अभिव्यक्ती),
                              नवीन AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } अन्यथा जर (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, गाठ_अभिव्यक्ती)), underp);  // Changed
        } अन्यथा जर (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(नवीन AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ढकला_Deletep(underp), underp);
        } अन्यथा {
            स्थिरांक तार्किक bad = widthBad(underp, parentp->findBitDType());
            जर (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->width(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->width()
                                             << (underp->width() != underp->widthMin()
                                                     ? " or " + cvtToStr(underp->widthMin())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            जर (Ast_अभिव्यक्ती_Stmt* स्थिरांक अभिव्यक्ती_Stmtp = VN_CAST(underp, अभिव्यक्ती_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(अभिव्यक्ती_Stmtp->resultp());
                अभिव्यक्ती_Stmtp->dtypeFrom(अभिव्यक्ती_Stmtp->resultp());
            } अन्यथा जर (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, गाठ_अभिव्यक्ती)), underp);  // Changed
            }
        }
    }

    स्थिर शून्य checkEnum(Ast_गाठ* nodep, Ast_गाठ* parentp, स्थिरांक Ast_गाठ_DType* targetDTypep,
                          स्थिरांक Ast_गाठ_DType* sourceDTypep, स्थिरांक Ast_गाठ_अभिव्यक्ती* sourceExprp) {
        जर (स्थिरांक AstEnumDType* स्थिरांक targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            स्थिरांक VCastable castable
                = Ast_गाठ::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            जर (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << targetDTypep->prettyDTypeNameQ() << " from "
                                             << sourceDTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warnMore()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_गाठ* iterateCheck(Ast_गाठ* parentp, स्थिरांक अक्षर* side, Ast_गाठ* underp, Determ determ,
                          Stage stage, Ast_गाठ_DType* expDTypep, ExtendRule extendRule,
                          तार्किक warnOn = सत्य, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        स्थिरांक StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        जर (rhsAccess.isReadOnly()) {
            जर (Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = VN_CAST(underp, गाठ_अभिव्यक्ती)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        जर (VN_IS(underp, गाठ_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } अन्यथा जर (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } अन्यथा जर (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } अन्यथा जर (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_गाठ* स्थिरांक oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, गाठ_अभिव्यक्ती));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अन्यथा जर (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_गाठ* स्थिरांक oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, गाठ_अभिव्यक्ती), सत्य, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अन्यथा जर (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_गाठ* स्थिरांक oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, गाठ_अभिव्यक्ती));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } अन्यथा {
            स्थिरांक Ast_गाठ_DType* स्थिरांक underDtp = underp->dtypep()->skipRefp();
            जर (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_गाठ* स्थिरांक newp
                    = नवीन AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ढकला_Deletep(underp), underp);
                underp = newp;
            }
            स्थिरांक Ast_गाठ_DType* स्थिरांक expDtp = expDTypep->skipRefp();
            जर (determ == ASSIGN && VN_IS(expDtp, गाठ_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            जर (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            स्थिरांक AstBasicDType* स्थिरांक expBasicp = expDTypep->basicp();
            स्थिरांक AstBasicDType* स्थिरांक underBasicp = underp->dtypep()->basicp();
            जर (expBasicp && underBasicp) {
                जर (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    जर (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, गाठ_अभिव्यक्ती));
                    }
                    जर (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_गाठ_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                जर (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } अन्यथा जर (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    जर (underp->isSigned() != subDTypep->isSigned()
                        || underp->width() != subDTypep->width()) {
                        subDTypep = parentp->findBitOrLogicDType(
                            std::max(subDTypep->width(), underp->width()),
                            std::max(subDTypep->widthMin(), underp->widthMin()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                } अन्यथा {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, गाठ_अभिव्यक्ती), expDTypep, extendRule,
                                warnOn);
            } अन्यथा जर (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } अन्यथा जर (स्थिरांक AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                स्थिरांक AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                जर (VN_IS(underp, Const) && VN_AS(underp, Const)->अंक().isNull()) {
                    // '= null' is ok
                } अन्यथा जर (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } अन्यथा जर (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } अन्यथा जर (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } अन्यथा {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } अन्यथा {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        परत underp;
    }

    शून्य
    widthCheckSized(Ast_गाठ* parentp, स्थिरांक अक्षर* side,
                    Ast_गाठ_अभिव्यक्ती* underp,  // Node to be checked or have typecast added in front of
                    Ast_गाठ_DType* expDTypep, ExtendRule extendRule, तार्किक warnOn = सत्य) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        स्थिरांक AstBasicDType* स्थिरांक expBasicp = expDTypep->basicp();
        स्थिरांक AstBasicDType* स्थिरांक underBasicp = underp->dtypep()->basicp();
        जर (expDTypep == underp->dtypep()) {
            परत;  // Same type must match
        } अन्यथा जर (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#जर VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            परत;
        } अन्यथा {
            स्थिरांक पूर्णांक expWidth = expDTypep->width();
            पूर्णांक expWidthMin = expDTypep->widthMin();
            जर (expWidthMin == 0) expWidthMin = expWidth;
            स्थिरांक तार्किक bad = widthBad(underp, expDTypep);
            जर ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                परत;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            जर (VN_IS(parentp->backp(), CastSize)) warnOn = असत्य;
            जर (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = असत्य;
            जर (VN_IS(underp, Const) && VN_AS(underp, Const)->अंक().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = असत्य;
            }
            जर ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::म_भेद(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = असत्य;
            }
            जर (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    expWidth, underp->width(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << expWidth
                        << (expWidth != expWidthMin ? " or " + cvtToStr(expWidthMin) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->width()
                        << (underp->width() != underp->widthMin()
                                ? " or " + cvtToStr(underp->widthMin())
                                : "")
                        << " bits.");
            }
            जर (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                स्थिरांक Ast_गाठ_Assign* assignp = VN_CAST(parentp, गाठ_Assign);
                स्थिरांक AstPin* pinp = VN_CAST(parentp, Pin);
                जर (assignp && VN_IS(assignp->lhsp(), गाठ_Stream)) {
                } अन्यथा जर (pinp && pinp->modVarp()->direction() != VDirection::आदान) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } अन्यथा जर (assignp && VN_IS(underp, गाठ_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    स्थिरांक पूर्णांक shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* स्थिरांक widenedp = नवीन AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(सत्य);
                    // Shift left so zeros fill on the right
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक shiftedp = नवीन AstShiftL{
                        underp->fileline(), widenedp,
                        नवीन AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } अन्यथा {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_गाठ_अभिव्यक्ती* checkCvtUS(Ast_गाठ_अभिव्यक्ती* nodep, तार्किक fatal) {
        जर (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            जर (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } अन्यथा {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, असत्य, 32);
        }
        परत nodep;
    }

    Ast_गाठ_अभिव्यक्ती* spliceCvtD(Ast_गाठ_अभिव्यक्ती* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        जर (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_गाठ_अभिव्यक्ती* newp;
            जर (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = नवीन AstISToRD{nodep->fileline(), nodep};
            } अन्यथा {
                newp = नवीन AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            परत newp;
        } अन्यथा {
            परत nodep;
        }
    }
    Ast_गाठ_अभिव्यक्ती* spliceCvtS(Ast_गाठ_अभिव्यक्ती* nodep, तार्किक warnOn, पूर्णांक width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        जर (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                जर (constp->isDouble() && V3_संख्या::epsilonIntegral(constp->अंक().toDouble())) {
                    warnOn = असत्य;
                }
            }
            जर (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            परत newp;
        } अन्यथा {
            परत nodep;
        }
    }
    Ast_गाठ_अभिव्यक्ती* spliceCvtString(Ast_गाठ_अभिव्यक्ती* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        जर (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            परत newp;
        } अन्यथा {
            परत nodep;
        }
    }
    Ast_गाठ_Biop* replaceWithUOrSVersion(Ast_गाठ_Biop* nodep, तार्किक signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        जर (signedFlavorNeeded == nodep->signedFlavor()) परत nullptr;
        जर (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        निवड (nodep->type()) {
        स्थिती VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        स्थिती VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        स्थिती VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        स्थिती VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        स्थिती VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        स्थिती VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        स्थिती VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        स्थिती VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        स्थिती VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); परत nullptr;
        पूर्वनिर्धारित: थांबा;
        }
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_Biop* newp = nullptr;
        निवड (nodep->type()) {
        स्थिती VNType::Gt: newp = नवीन AstGtS{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::GtS: newp = नवीन AstGt{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Gte: newp = नवीन AstGteS{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::GteS: newp = नवीन AstGte{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Lt: newp = नवीन AstLtS{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::LtS: newp = नवीन AstLt{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Lte: newp = नवीन AstLteS{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::LteS: newp = नवीन AstLte{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Div: newp = नवीन AstDivS{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::DivS: newp = नवीन AstDiv{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::ModDiv: newp = नवीन AstModDivS{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::ModDivS: newp = नवीन AstModDiv{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Mul: newp = नवीन AstMulS{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::MulS: newp = नवीन AstMul{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::ShiftR: newp = नवीन AstShiftRS{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::ShiftRS: newp = नवीन AstShiftR{fl, lhsp, rhsp}; थांबा;
        पूर्वनिर्धारित:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            थांबा;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत newp;
    }
    Ast_गाठ_Biop* replaceWithDVersion(Ast_गाठ_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        जर (nodep->doubleFlavor()) परत nullptr;
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        निवड (nodep->type()) {
        स्थिती VNType::Add: newp = नवीन AstAddD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Sub: newp = नवीन AstSubD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Pow: newp = नवीन AstPowD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Eq:
        स्थिती VNType::EqCase: newp = नवीन AstEqD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Neq:
        स्थिती VNType::NeqCase: newp = नवीन AstNeqD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Gt:
        स्थिती VNType::GtS: newp = नवीन AstGtD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Gte:
        स्थिती VNType::GteS: newp = नवीन AstGteD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Lt:
        स्थिती VNType::LtS: newp = नवीन AstLtD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Lte:
        स्थिती VNType::LteS: newp = नवीन AstLteD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Div:
        स्थिती VNType::DivS: newp = नवीन AstDivD{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Mul:
        स्थिती VNType::MulS: newp = नवीन AstMulD{fl, lhsp, rhsp}; थांबा;
        पूर्वनिर्धारित:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            थांबा;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत newp;
    }
    Ast_गाठ_Biop* replaceWithNVersion(Ast_गाठ_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        जर (nodep->stringFlavor()) परत nullptr;
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_गाठ_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        निवड (nodep->type()) {
        स्थिती VNType::Eq:
        स्थिती VNType::EqCase:
        स्थिती VNType::EqWild: newp = नवीन AstEqN{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Neq:
        स्थिती VNType::NeqCase:
        स्थिती VNType::NeqWild: newp = नवीन AstNeqN{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Gt:
        स्थिती VNType::GtS: newp = नवीन AstGtN{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Gte:
        स्थिती VNType::GteS: newp = नवीन AstGteN{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Lt:
        स्थिती VNType::LtS: newp = नवीन AstLtN{fl, lhsp, rhsp}; थांबा;
        स्थिती VNType::Lte:
        स्थिती VNType::LteS: newp = नवीन AstLteN{fl, lhsp, rhsp}; थांबा;
        पूर्वनिर्धारित:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            थांबा;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत newp;
    }
    Ast_गाठ_Uniop* replaceWithDVersion(Ast_गाठ_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        जर (nodep->doubleFlavor()) परत nullptr;
        File_ओळ* स्थिरांक fl = nodep->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_गाठ_Uniop* newp = nullptr;
        निवड (nodep->type()) {
        स्थिती VNType::Negate: newp = नवीन AstNegateD{fl, lhsp}; थांबा;
        पूर्वनिर्धारित:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            थांबा;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
        परत newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    शून्य replaceWithSFormat(AstMethodCall* nodep, स्थिरांक string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* स्थिरांक argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_गाठ_Var_Ref* स्थिरांक fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_गाठ* स्थिरांक newp = नवीन AstAssign{
            nodep->fileline(), fromp,
            नवीन AstSFormatF{nodep->fileline(), format, असत्य, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        ढकला_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_गाठ_DType* iterateEditMoveDTypep(Ast_गाठ* parentp, Ast_गाठ_DType* dtnodep) {
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
        स्थिरांक तार्किक child1 = (parentp->getChildDTypep() == dtnodep);
        स्थिरांक तार्किक child2 = (parentp->getChild2DTypep() == dtnodep);
        जर (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, nullptr), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->getChildDTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlinkFrBack();
            v3Global.rootp()->typeTablep()->addTypesp(dtnodep);
        }
        जर (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        परत dtnodep;
    }

    AstConst* dimension_मूल्य(File_ओळ* fileline, Ast_गाठ_DType* nodep, VAttrType attrType,
                             पूर्णांक dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_गाठ_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        साठी (पूर्णांक i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            जर (स्थिरांक Ast_गाठ_Array_DType* स्थिरांक adtypep = VN_CAST(dtypep, गाठ_Array_DType)) {
                declRange = adtypep->declRange();
                जर (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                सुरूठेवा;
            } अन्यथा जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक adtypep
                       = VN_CAST(dtypep, गाठ_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                थांबा;  // Sub elements don't look like arrays and can't iterate into
            } अन्यथा जर (स्थिरांक AstBasicDType* स्थिरांक adtypep = VN_CAST(dtypep, BasicDType)) {
                जर (adtypep->isRanged()) declRange = adtypep->declRange();
                थांबा;
            }
            थांबा;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        पूर्णांक val = 0;
        निवड (attrType) {
        स्थिती VAttrType::DIM_BITS: {
            पूर्णांक bits = 1;
            पर्यंत (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                जर (स्थिरांक Ast_गाठ_Array_DType* स्थिरांक adtypep = VN_CAST(dtypep, गाठ_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    सुरूठेवा;
                } अन्यथा जर (स्थिरांक Ast_गाठ_UOr_Struct_DType* स्थिरांक adtypep
                           = VN_CAST(dtypep, गाठ_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    थांबा;
                } अन्यथा जर (स्थिरांक AstBasicDType* स्थिरांक adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    थांबा;
                }
                थांबा;
            }
            जर (dim == 0) {
                val = 0;
            } अन्यथा जर (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } अन्यथा {
                val = bits;
            }
            थांबा;  // LCOV_EXCL_LINE
        }
        स्थिती VAttrType::DIM_वरचा: val = !declRange.ranged() ? 0 : declRange.hi(); थांबा;
        स्थिती VAttrType::DIM_डावा: val = !declRange.ranged() ? 0 : declRange.डावा(); थांबा;
        स्थिती VAttrType::DIM_खालचा: val = !declRange.ranged() ? 0 : declRange.lo(); थांबा;
        स्थिती VAttrType::DIM_उजवा: val = !declRange.ranged() ? 0 : declRange.उजवा(); थांबा;
        स्थिती VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            थांबा;
        स्थिती VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); थांबा;
        पूर्वनिर्धारित: nodep->v3fatalSrc("Missing DIM ATTR type case"); थांबा;
        }
        जर (!valp) valp = नवीन AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        परत valp;
    }
    AstVarRef* dimensionVarRefp(Ast_गाठ_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        जर (cachedpr) परत cachedpr->clone_वृक्ष(असत्य);

        Ast_गाठ_Array_DType* स्थिरांक vardtypep = नवीन AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), नवीन AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* स्थिरांक consp = नवीन AstConsArray{nodep->fileline(), vardtypep};
        साठी (अचिन्हित i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_मूल्य(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_वृक्ष(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(सत्य);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(सत्य);
        ढकला_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        परत cachedpr->clone_वृक्ष(असत्य);
    }
    स्थिर uint64_t enum_Max_मूल्य(स्थिरांक Ast_गाठ* errNodep, स्थिरांक AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        साठी (स्थिरांक AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            स्थिरांक AstConst* स्थिरांक vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            जर (!vconstp->अंक().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        जर (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            परत 64;
        }
        परत maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* स्थिरांक nodep, VAttrType attrType, तार्किक assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        जर (cachedpr) परत cachedpr->clone_वृक्ष(असत्य);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_ओळ* स्थिरांक flp = nodep->fileline();
        Ast_गाठ_DType* basep;
        जर (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } अन्यथा जर (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } अन्यथा {
            basep = nodep->dtypep();
        }
        Ast_गाठ_DType* vardtypep;
        जर (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = नवीन AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } अन्यथा {
            vardtypep = नवीन AstUnpackArrayDType{flp, basep, नवीन AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        जर (attrType == VAttrType::ENUM_NAME) {
            defaultp = नवीन AstConst{flp, AstConst::String{}, ""};
        } अन्यथा जर (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = नवीन AstConst{flp, V3_संख्या{nodep, nodep->width(), 0}};
        } अन्यथा जर (attrType == VAttrType::ENUM_VALID) {
            defaultp = नवीन AstConst{flp, AstConst::BitFalse{}};
        } अन्यथा {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_गाठ_अभिव्यक्ती*> मूल्ये;
        {
            uint64_t ordinal = 0;
            AstEnumItem* स्थिरांक firstp = nodep->itemsp();
            स्थिरांक AstEnumItem* prevp = firstp;  // Prev must start with last item
            पर्यंत (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            साठी (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* स्थिरांक nextp = VN_AS(itemp->nextp(), EnumItem);
                स्थिरांक AstConst* स्थिरांक vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                जर (!vconstp->अंक().isAnyXZ()) {  // Can 2-state runtime decode
                    स्थिरांक uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    जर (attrType == VAttrType::ENUM_NAME) {
                        मूल्ये[i] = नवीन AstConst{flp, AstConst::String{},
                                                 V3_संख्या::displayedEnumName(itemp)};
                    } अन्यथा जर (attrType == VAttrType::ENUM_NEXT) {
                        मूल्ये[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_वृक्ष(असत्य);  // A const
                    } अन्यथा जर (attrType == VAttrType::ENUM_PREV) {
                        मूल्ये[i] = prevp->valuep()->clone_वृक्ष(असत्य);  // A const
                    } अन्यथा जर (attrType == VAttrType::ENUM_VALID) {
                        मूल्ये[i] = नवीन AstConst{flp, AstConst::BitTrue{}};
                    } अन्यथा {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_गाठ_अभिव्यक्ती* valuep = nullptr;
        जर (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* स्थिरांक consp = नवीन AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            साठी (स्थिरांक स्वतः& itr : मूल्ये) {
                V3_संख्या किल्ली{nodep, nodep->width()};
                किल्ली.setQuad(itr.first);
                consp->addItemsp(नवीन AstConsAssocItem{flp, नवीन AstConst{flp, किल्ली}, itr.second});
            }
            valuep = consp;
        } अन्यथा {
            AstConsArray* स्थिरांक consp = नवीन AstConsArray{flp, vardtypep, defaultp};
            साठी (uint64_t i = 0; i < (msbdim + 1); ++i) {
                जर (मूल्ये[i]) consp->valuep(i, मूल्ये[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(ढकला_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(सत्य);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(सत्य);
        ढकला_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        परत cachedpr->clone_वृक्ष(असत्य);
    }

    Ast_गाठ_अभिव्यक्ती* enumSelect(Ast_गाठ_अभिव्यक्ती* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        जर (attrType == VAttrType::ENUM_NAME) {
            जर (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->अंक().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
                परत newp;
            }
        }
        जर (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_अभिव्यक्ती_Stmt* स्थिरांक capturep = newEnumCapture(nodep, adtypep);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            परत capturep;
        }
        जर (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_गाठ_अभिव्यक्ती* indexp = नवीन AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            साठी (स्थिरांक AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                स्थिरांक AstConst* स्थिरांक constp = VN_AS(itemp->valuep(), Const);
                जर (constp->अंक().isAnyXZ()) सुरूठेवा;
                indexp = नवीन AstCond{nodep->fileline(),
                                     नवीन AstEq{nodep->fileline(), nodep->clone_वृक्ष(असत्य),
                                               itemp->valuep()->clone_वृक्ष(असत्य)},
                                     नवीन AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* स्थिरांक tabRefp = enumVarRefp(adtypep, attrType, असत्य, ordinal);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newp = नवीन AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
            परत newp;
        }
        स्थिरांक uint64_t msbdim = enum_Max_मूल्य(nodep, adtypep);
        स्थिरांक तार्किक assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_गाठ_अभिव्यक्ती* newp;
        जर (assoc) {
            AstVarRef* स्थिरांक tabRefp = enumVarRefp(adtypep, attrType, सत्य, 0);
            newp = नवीन AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } अन्यथा {
            स्थिरांक पूर्णांक selwidth = V3_संख्या::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* स्थिरांक tabRefp
                = enumVarRefp(adtypep, attrType, असत्य, (1ULL << selwidth) - 1);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? नवीन AstGt{nodep->fileline(), nodep->clone_वृक्ष(असत्य),
                                  नवीन AstConst{nodep->fileline(),
                                               V3_संख्या{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = नवीन AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                नवीन AstSel{nodep->fileline(), nodep, 0, selwidth}};
            जर (boundp) {
                newp = नवीन AstCond{nodep->fileline(), boundp,
                                   नवीन AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        जर (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } अन्यथा जर (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } अन्यथा {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        परत newp;
    }
    Ast_गाठ_अभिव्यक्ती* enumTestValid(Ast_गाठ_अभिव्यक्ती* valp, AstEnumDType* enumDtp) {
        स्थिरांक uint64_t maxval = enum_Max_मूल्य(valp, enumDtp);
        स्थिरांक तार्किक assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_गाठ_अभिव्यक्ती* testp = nullptr;
        File_ओळ* स्थिरांक fl_novalue = नवीन File_ओळ{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, सत्य);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, सत्य);
        जर (assoc) {
            AstVarRef* स्थिरांक tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, सत्य, 0);
            testp = नवीन AstAssocSel{fl_novalue, tabRefp, valp->clone_वृक्ष_Pure(असत्य)};
        } अन्यथा {
            स्थिरांक पूर्णांक selwidth = V3_संख्या::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* स्थिरांक tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, असत्य, (1ULL << selwidth) - 1);
            File_ओळ* स्थिरांक fl_nowidth = नवीन File_ओळ{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, सत्य);
            testp = नवीन AstCond{
                fl_novalue,
                नवीन AstGt{fl_nowidth, valp->clone_वृक्ष_Pure(असत्य),
                          नवीन AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                नवीन AstConst{fl_novalue, AstConst::BitFalse{}},
                नवीन AstArraySel{fl_novalue, tabRefp,
                                नवीन AstSel{fl_novalue, valp->clone_वृक्ष_Pure(असत्य), 0, selwidth}}};
        }
        परत testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, स्थिरांक VNumRange& range) {
        PatVecMap patmap;
        पूर्णांक element = range.डावा();
        साठी (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            जर (patp->keyp()) {
                जर (स्थिरांक AstConst* स्थिरांक constp = patternConstKeyp(patp)) {
                    जर (constp->अंक().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } अन्यथा {
                        element = constp->toSInt();
                    }
                }
            }
            स्थिरांक तार्किक newEntry = patmap.emplace(element, patp).second;
            जर (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.डावा_To_उजवा_Inc();
        }
        परत patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    शून्य patConcatConvertRecurse(AstPattern* patternp, Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (AstConcat* स्थिरांक concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } अन्यथा {
            AstPatMember* स्थिरांक newp
                = नवीन AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(सत्य);
            patternp->addItemsp(newp);
        }
    }

    शून्य makeOpenArrayShell(Ast_गाठ_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_गाठ_FTask* स्थिरांक oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_गाठ_FTask* स्थिरांक newTaskp = oldTaskp->clone_वृक्ष(असत्य);
        newTaskp->dpiOpenChild(सत्य);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        स्थिरांक V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        साठी (स्थिरांक स्वतः& tconnect : tconnects) {
            AstVar* स्थिरांक portp = tconnect.first;
            स्थिरांक AstArg* स्थिरांक argp = tconnect.second;
            स्थिरांक Ast_गाठ* स्थिरांक pinp = argp->exprp();
            जर (!pinp) सुरूठेवा;  // Argument error we'll find later
            जर (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_गाठ_DType* स्थिरांक oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_गाठ* nodep) {
                    जर (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    तार्किक markHasOpenArray(Ast_गाठ_FTask* nodep) {
        तार्किक hasOpen = असत्य;
        साठी (Ast_गाठ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            जर (AstVar* स्थिरांक portp = VN_CAST(stmtp, Var)) {
                जर (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(सत्य);
                    hasOpen = सत्य;
                }
            }
        }
        परत hasOpen;
    }
    तार्किक hasOpenArrayDTypeRecurse(Ast_गाठ_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        जर (VN_IS(nodep, UnsizedArrayDType)) परत सत्य;
        जर (nodep->subDTypep()) परत hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        परत असत्य;
    }
    Ast_गाठ_DType* dtypeNotIntAtomOrVecRecurse(Ast_गाठ_DType* nodep, तार्किक ranged = असत्य) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        जर (AstBasicDType* स्थिरांक dtp = VN_CAST(nodep, BasicDType)) {
            जर (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                परत dtp;  // Packed when already packed
            जर (dtp->keyword().isIntNumeric()) परत nullptr;
            परत dtp;
        } अन्यथा जर (AstPackArrayDType* स्थिरांक dtp = VN_CAST(nodep, PackArrayDType)) {
            जर (ranged) परत dtp;  // Packed when already packed
            परत dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), सत्य);
        }
        परत nodep;
    }
    Ast_गाठ_DType* dtypeNot4StateIntegralRecurse(Ast_गाठ_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        जर (AstBasicDType* स्थिरांक dtp = VN_CAST(nodep, BasicDType)) {
            जर (!dtp->keyword().isFourstate()) परत dtp;
            परत nullptr;
        } अन्यथा जर (Ast_गाठ_Array_DType* स्थिरांक dtp = VN_CAST(nodep, गाठ_Array_DType)) {
            परत dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } अन्यथा जर (Ast_गाठ_UOr_Struct_DType* स्थिरांक dtp = VN_CAST(nodep, गाठ_UOr_Struct_DType)) {
            साठी (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_गाठ_DType* स्थिरांक badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                जर (badDtp) परत badDtp;
            }
            परत nullptr;
        }
        परत nodep;
    }
    शून्य checkForceReleaseLhs(Ast_गाठ* nodep, Ast_गाठ* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        जर (Ast_गाठ* स्थिरांक selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    शून्य assert_At_अभिव्यक्ती(Ast_गाठ* nodep) {
        जर (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    शून्य assertAtStatement(Ast_गाठ* nodep) {
        जर (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    शून्य checkConstantOrReplace(Ast_गाठ* nodep, तार्किक noFourState, स्थिरांक string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* स्थिरांक constp = VN_CAST(nodep, Const);
        जर (!constp || (noFourState && constp->अंक().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(नवीन AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ढकला_Deletep(nodep), nodep, constp);
            परत;
        }
    }
    Ast_गाठ* गाठ_For_Unsized_Warning(Ast_गाठ* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        जर (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            परत nodep->op1p();
        } अन्यथा जर (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            परत nodep->op2p();
        }
        परत nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_गाठ* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        जर (VL_LIKELY(!nodep)) परत nullptr;
        जर (स्वतः* स्थिरांक refp = VN_CAST(nodep, RefDType)) {
            जर (refp->typedefp() == typedefp) परत refp;
        }
        जर (स्वतः* स्थिरांक refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) परत refp;
        जर (स्वतः* स्थिरांक refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) परत refp;
        जर (स्वतः* स्थिरांक refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) परत refp;
        जर (स्वतः* स्थिरांक refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) परत refp;
        परत nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    स्थिर तार्किक isIfaceArrayDType(स्थिरांक Ast_गाठ_DType* dtypep) {
        जर (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) परत असत्य;
        स्थिरांक AstIfaceRefDType* स्थिरांक irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        परत irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    स्थिर std::string unpackedArraySize(स्थिरांक Ast_गाठ_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        पर्यंत (स्थिरांक AstUnpackArrayDType* स्थिरांक arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            जर (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        परत size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    स्थिर Ast_गाठ_अभिव्यक्ती* newIfaceArrayElement(Ast_गाठ_अभिव्यक्ती* fromp, पूर्णांक bit) {
        AstConsArray* स्थिरांक consp = VN_CAST(fromp, ConsArray);
        जर (!consp || !isIfaceArrayDType(consp->dtypep())) परत nullptr;
        स्थिरांक AstUnpackArrayDType* स्थिरांक arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        जर (bit < 0 || bit >= arrp->elementsConst()) परत nullptr;
        परत consp->valuep(bit)->clone_वृक्ष(असत्य);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_गाठ_अभिव्यक्ती* new_Iface_Array_मूल्य(स्थिरांक Ast_गाठ_Var_Ref* refp, Ast_गाठ_DType* dtypep,
                                    स्थिरांक std::string& name, स्थिरांक std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_ओळ* स्थिरांक flp = refp->fileline();
        AstUnpackArrayDType* स्थिरांक arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        जर (!arrp) {
            AstVar* स्थिरांक varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_गाठ_Var_Ref* newp;
            जर (स्थिरांक AstVarXRef* स्थिरांक xrefp = VN_CAST(refp, VarXRef)) {
                newp = नवीन AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } अन्यथा {
                newp = नवीन AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            परत newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* स्थिरांक consp = नवीन AstConsArray{flp, arrp};
        स्थिरांक VNumRange range = arrp->declRange();
        साठी (स्थिरांक पूर्णांक i : range.seq_डावा_To_उजवा()) {
            स्थिरांक std::string s = name + "__BRA__" + Ast_गाठ::encode_संख्या(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_मूल्य(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        परत consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    शून्य replaceSplitIfaceArrayRef(Ast_गाठ_Var_Ref* nodep) {
        AstVar* स्थिरांक varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        जर (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_मूल्य(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ढकला_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    शून्य expandIfaceArrayPinDimensions(AstPin* nodep, स्थिरांक Ast_गाठ_DType* dtypep,
                                       Ast_गाठ_अभिव्यक्ती* conp, स्थिरांक std::string& suffix,
                                       AstVar*& elemVarpr) {
        स्थिरांक AstUnpackArrayDType* स्थिरांक arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        जर (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* स्थिरांक newp = nodep->clone_वृक्ष(असत्य);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            जर (conp) newp->exprp(conp->clone_वृक्ष(असत्य));
            nodep->addNextHere(newp);
            परत;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        स्थिरांक VNumRange range = arrp->declRange();
        साठी (पूर्णांक n = range.elements() - 1; n >= 0; --n) {
            स्थिरांक पूर्णांक i = range.डावा() + n * range.डावा_To_उजवा_Inc();
            स्थिरांक std::string s = suffix + "__BRA__" + Ast_गाठ::encode_संख्या(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_गाठ_अभिव्यक्ती* स्थिरांक elemConnp = [&]() -> Ast_गाठ_अभिव्यक्ती* {
                जर (!conp) परत nullptr;
                स्थिरांक AstUnpackArrayDType* स्थिरांक conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                स्थिरांक पूर्णांक elements = conArrp->elementsConst();
                // Counted from lo
                Ast_गाठ_अभिव्यक्ती* स्थिरांक resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                परत resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    शून्य expandIfaceArrayPin(AstPin* nodep) {
        AstVar* स्थिरांक portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        साठी (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = VN_AS(nodep->exprp(), गाठ_अभिव्यक्ती);
        जर (exprp) ढकला_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        स्थिरांक तार्किक connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ढकला_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_गाठ* userIterateSubtreeReturnEdits(Ast_गाठ* nodep, WidthVP* vup) {
        जर (!nodep) परत nullptr;
        Ast_गाठ* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        परत ret;
    }
    शून्य userIterate(Ast_गाठ* nodep, WidthVP* vup) {
        जर (!nodep) परत;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    शून्य userIterateAndNext(Ast_गाठ* nodep, WidthVP* vup) {
        जर (!nodep) परत;
        जर (nodep->didWidth()) परत;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    शून्य userIterateChildren(Ast_गाठ* nodep, WidthVP* vup) {
        जर (!nodep) परत;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    शून्य userIterateChildrenBackwardsConst(Ast_गाठ* nodep, WidthVP* vup) {
        जर (!nodep) परत;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    WidthVisitor(तार्किक paramsOnly,  // [in] TRUE if we are considering parameters only.
                 तार्किक doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_गाठ* mainAcceptEdit(Ast_गाठ* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        साठी (AstVar* स्थिरांक varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(ढकला_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        परत nodep;
    }
    स्थिर तार्किक lowerAsFixedAggregate(स्थिरांक Ast_गाठ_DType* स्थिरांक dtypep) {
        परत dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    शून्य streamAssignLower(Ast_गाठ_Assign* nodep) {
        Ast_गाठ_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        जर (Ast_गाठ_Stream* स्थिरांक streamp = VN_CAST(nodep->rhsp(), गाठ_Stream)) {
            Ast_गाठ_DType* स्थिरांक lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            स्थिरांक पूर्णांक lwidth = lhsDTypeSkippedRefp->widthStream();
            स्थिरांक पूर्णांक rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            जर (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            जर (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                स्थिरांक पूर्णांक queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            जर (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    नवीन AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        जर (Ast_गाठ_Stream* स्थिरांक streamp = VN_CAST(nodep->lhsp(), गाठ_Stream)) {
            स्थिरांक Ast_गाठ_DType* स्थिरांक rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_गाठ_DType* स्थिरांक lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            स्थिरांक पूर्णांक lwidth = lhsStreamDTypep->widthStream();
            स्थिरांक पूर्णांक rwidth = rhsDTypep->widthStream();
            जर (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            जर (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक dstp = streamp->lhsp()->unlinkFrBack();
                Ast_गाठ_अभिव्यक्ती* srcp = nodep->rhsp()->unlinkFrBack();
                जर (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_गाठ_अभिव्यक्ती::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } अन्यथा {
                    जर (srcp->width() > lwidth) {
                        srcp = नवीन AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ढकला_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(नवीन AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } अन्यथा जर (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(नवीन AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = पूर्वनिर्धारित;
};

//######################################################################
// Width class functions

शून्य V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        स्थिरांक WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        स्थिरांक VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{असत्य, असत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (शून्य)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("width", 0, dump_वृक्ष_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_गाठ* V3Width::widthParamsEdit(Ast_गाठ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{सत्य, असत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    परत nodep;
}

शून्य V3Width::streamAssignLowerEdit(Ast_गाठ_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{असत्य, असत्य};
    visitor.streamAssignLower(nodep);
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
Ast_गाठ* V3Width::widthGenerateParamsEdit(
    Ast_गाठ* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{सत्य, सत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    परत nodep;
}

Ast_गाठ* V3Width::selectNonConstantRecurse(Ast_गाठ* nodep, तार्किक inSel) {
    // If node has a non-constant select, return that select
    Ast_गाठ* resultp = nullptr;
    जर (Ast_गाठ_Sel* स्थिरांक anodep = VN_CAST(nodep, गाठ_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        जर (resultp) परत resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), सत्य);
    } अन्यथा जर (AstSel* स्थिरांक anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        जर (resultp) परत resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), सत्य);
    } अन्यथा जर (Ast_गाठ_Var_Ref* स्थिरांक anodep = VN_CAST(nodep, गाठ_Var_Ref)) {
        जर (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) परत anodep;
    } अन्यथा {
        जर (Ast_गाठ* स्थिरांक refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        जर (resultp) परत resultp;
        जर (Ast_गाठ* स्थिरांक refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        जर (resultp) परत resultp;
        जर (Ast_गाठ* स्थिरांक refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        जर (resultp) परत resultp;
        जर (Ast_गाठ* स्थिरांक refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    परत resultp;
}
